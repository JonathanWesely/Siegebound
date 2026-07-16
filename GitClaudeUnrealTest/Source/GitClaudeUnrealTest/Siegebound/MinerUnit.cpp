// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/MinerUnit.h"

#include "AIController.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GitClaudeUnrealTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "Siegebound/GoldNode.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegePlayerState.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	/** §6 miner "clink" mining loop (TASK-179) — null-safe soft path; the loop flag is authored on the asset (TASK-180). */
	const TCHAR* MinerClinkSoundPath = TEXT("/Game/Audio/S_MinerClink");
}

AMinerUnit::AMinerUnit()
{
	// Per-card class (TASK-025 names block: BP_Unit_Miner with row Miner): the
	// row IDENTITY is the class's nature, so it defaults here; every stat on
	// that row still binds from DT_Cards at BeginPlay, never from code (§3.0).
	// TASK-034's BP sets the same value (no-op) and TASK-030's deferred-spawn
	// InitUnit(Team, "Miner") agrees with it.
	CardID = FName(TEXT("Miner"));

	// --- Structural no-attack-path seals (qa/TASK-021-report.md WARN-1, BINDING) ---
	// The base's combat surfaces (LoadStatsAndStart/UpdateState/EnterAttack/
	// PerformAttack, the state/attack timer handles) are private and frozen
	// qa-passed, so the machine is disarmed through protected DATA instead:
	//
	// Seal #1 — the state timer can never be armed. FTimerManager::SetTimer
	// with a rate <= 0 CLEARS the handle instead of scheduling (the very
	// semantics WARN-1 documents — used here deliberately, as a disarm).
	// LoadStatsAndStart is the ONLY place that timer is ever set, and the
	// bStatsLoaded latch means it can run at most once per lifetime — with
	// this value, that one arming call is a no-op. The editor-only ClampMin
	// (0.05) on the property does not constrain C++ constructor writes; this
	// is a deliberate out-of-band value, and BP_Unit_Miner must not touch the
	// property (TASK-034: nothing stat-like on the BP). Seals #2/#3 cover it
	// even if a serialized value ever re-legalized the interval.
	StateCheckInterval = 0.f;

	// Seal #2 — acquisition is dead. AcquireTarget skips every candidate
	// farther than AggroRadius, so at 0 it can never return one and
	// CurrentTarget stays null forever — and UpdateState only ever enters
	// Attack for a non-null CurrentTarget. This holds even for the ONE
	// synchronous UpdateState that LoadStatsAndStart makes inside
	// Super::BeginPlay: the miner is constitutionally incapable of targeting
	// (§3.3 non-combat; the row's Profile is None, not Standard).
	AggroRadius = 0.f;

	// §6 mining "clink" loop (TASK-179): inactive until arrival (StartMiningClink);
	// the sound is soft-resolved then, so the component just exists here. Attached to
	// the capsule (root) — spatialized at the miner. Auto-destroyed with the actor.
	ClinkAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ClinkAudio"));
	ClinkAudio->SetupAttachment(GetRootComponent());
	ClinkAudio->bAutoActivate = false;

	// (Seal #3 — the post-Super timer sweep — lives in BeginPlay.)
	//
	// NOTE: the base clamps row Cadence to a 0.05 s minimum, so the Miner
	// row's Cadence 0 would NOT keep an attack timer unarmed by itself — if
	// this unit ever reached EnterAttack it would swing 20×/s. These seals are
	// load-bearing, not belt-and-braces.
}

void AMinerUnit::BeginPlay()
{
	// Binds the card stats "as usual" (GDD §3.0: 30 HP / 350 speed from
	// /Game/Data/DT_Cards row Miner) and caches the rest pose. With seals
	// #1/#2 the embedded state-machine start is inert: the timer never arms,
	// and the one synchronous UpdateState can at most issue an acquisition-
	// dead Advance toward the enemy castle — a discarded path request that
	// EnsureWalkingToNode below replaces within this same call stack, before
	// any movement tick consumes it.
	Super::BeginPlay();

	// Seal #3: kill anything armed during Super::BeginPlay (defense in depth
	// for a serialized StateCheckInterval; TASK-024 precedent — the
	// FTimerManager per-object sweep is the sanctioned lever when the handle
	// itself is private). MUST run before any miner timer is armed. The only
	// timers that can exist on this actor here are the base's; ACharacter
	// internals do not key timers on the actor.
	GetWorldTimerManager().ClearAllTimersForObject(this);

	// A frozen (or somehow dead) unit must not register, walk, or poll —
	// FreezeAI's "idles until destroyed" contract extends to late BeginPlay.
	if (IsUnitDead() || IsAIFrozen())
	{
		return;
	}

	// §3.3 bookkeeping — RegisterMinerAlive on the owning team's player state,
	// exactly once, at BeginPlay (handoffs/TASK-024.md contract). If the state
	// is not resolvable yet (PlayerArray timing), the poll below retries.
	TryRegisterWithOwnerState();

	// walk to the SAME-team gold node (actor iteration per spec; placed as
	// GoldNode_Blue / GoldNode_Red in TASK-036). None in the level: log and
	// idle — the poll still runs (early-outs on the missing node) so a
	// deferred registration can complete.
	if (AGoldNode* Node = FindNearestSameTeamGoldNode())
	{
		TargetGoldNode = Node;
		EnsureWalkingToNode(Node);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("AMinerUnit '%s': no same-team AGoldNode in the level (GoldNode_Blue/GoldNode_Red are placed in TASK-036) — miner idles."),
			*GetNameSafe(this));
	}

	// arrival/upkeep poll — the miner's replacement for the sealed combat
	// state timer: ~0.25 s cadence, never per-tick (TASK-004 law). Rate is
	// clamped strictly positive at arm time: SetTimer with <= 0 would CLEAR
	// instead of schedule (qa/TASK-021 WARN-1 guard rule for looping timers).
	GetWorldTimerManager().SetTimer(MiningPollTimerHandle, this, &AMinerUnit::UpdateMining,
		FMath::Max(ArrivalCheckInterval, 0.05f), /*bLoop=*/ true);
}

void AMinerUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MiningPollTimerHandle);

	// §6 (TASK-179): silence the clink on death/removal (explicit — the component also
	// auto-stops with the actor). Covers combat death, the PlayAgain sweep, and KillZ.
	StopMiningClink();

	// §3.3 death bookkeeping at the single removal-from-play choke point:
	// combat death (base HandleDeath → Destroy), the PlayAgain unit sweep, and
	// a KillZ fall all arrive here with reason == Destroyed. World teardown
	// (PIE end / quit / travel) deliberately does NOT run bookkeeping — the
	// counts die with the player state.
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (ASiegePlayerState* OwnerState = CachedOwnerState.Get())
		{
			// income first, then alive: MinerIncomeCount ⊆ AliveMinerCount
			// holds at every intermediate step (AddMinerIncome's diagnostic
			// reads that invariant on the player state).
			if (bIncomeActive)
			{
				// only a miner that had ARRIVED removes income — a miner
				// killed en route changes the rate not at all (GDD §3.3,
				// handoffs/TASK-024.md contract).
				OwnerState->RemoveMinerIncome();
				bIncomeActive = false;
			}
			if (bRegisteredAlive)
			{
				// ALWAYS on death, arrived or not (TASK-024 contract).
				OwnerState->UnregisterMinerAlive();
				bRegisteredAlive = false;
			}
		}
		else if (bRegisteredAlive)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AMinerUnit '%s': owner player state gone before death bookkeeping — alive/income counts could not be decremented."),
				*GetNameSafe(this));
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AMinerUnit::FreezeAI()
{
	// Base (TASK-028): clears the state/attack timers, cancels any lunge to
	// the exact rest pose, StopMovement — which halts the gold-node walk —
	// parks Idle, and latches the frozen flag. (It early-outs when already
	// frozen or dead; the clear below is idempotent either way.)
	Super::FreezeAI();

	// Subclass drive (the handoffs/TASK-028.md extension rule): stop the
	// arrival/upkeep poll so nothing can re-issue MoveToActor under the
	// match-end freeze. An arrived miner's income stays configured — accrual
	// is stopped by TASK-024's PauseIncome, not by a rate change (its flagged
	// decision 5), and PlayAgain's destroy sweep runs the death bookkeeping.
	GetWorldTimerManager().ClearTimer(MiningPollTimerHandle);

	// §6 (TASK-179): a frozen miner mines no more — silence the clink loop.
	StopMiningClink();
}

void AMinerUnit::StartMiningClink()
{
	if (!ClinkAudio)
	{
		return;
	}

	// Resolve S_MinerClink null-safe (cached, logged once). Missing = no clink,
	// never a crash. Idempotent: don't restart an already-playing loop.
	if (ClinkAudio->IsPlaying())
	{
		return;
	}
	if (USoundBase* ClinkSound = USiegeFeedbackLibrary::ResolveSound(MinerClinkSoundPath))
	{
		ClinkAudio->SetSound(ClinkSound);
		ClinkAudio->Play();
	}
}

void AMinerUnit::StopMiningClink()
{
	if (ClinkAudio && ClinkAudio->IsPlaying())
	{
		ClinkAudio->Stop();
	}
}

void AMinerUnit::UpdateMining()
{
	// FreezeAI/EndPlay clear this timer; these gates make a stray fire inert
	// (the base UpdateState's bDead/bAIFrozen gate pattern, via the public
	// getters — the flags themselves are private).
	if (IsUnitDead() || IsAIFrozen())
	{
		return;
	}

	// registration retry: closes the "player state not yet in PlayerArray at
	// BeginPlay" ordering edge. Latched — no-op once registered. Runs BEFORE
	// the arrival check so arrival can never observe a resolvable-but-
	// unregistered owner.
	if (!bRegisteredAlive)
	{
		TryRegisterWithOwnerState();
	}

	AGoldNode* Node = TargetGoldNode.Get();
	if (!Node)
	{
		// never found (already logged at BeginPlay): stay idle, silently.
		// found-then-removed (stale weak): warn once — nothing in M2 destroys
		// gold nodes, so this is a level-authoring diagnostic.
		if (!TargetGoldNode.IsExplicitlyNull() && !bWarnedNodeLost)
		{
			bWarnedNodeLost = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AMinerUnit '%s': gold node disappeared mid-match — miner idles."),
				*GetNameSafe(this));
		}
		return;
	}

	// Arrival test in 2D: the arena is flat and the node's origin sits at
	// ground level while a character's location is its capsule CENTER (~90
	// units up) — a 3D test would burn most of the 150-unit budget vertically
	// and sit knife-edged against the 0.8 × ArrivalRadius walk acceptance.
	const float DistanceToNode = static_cast<float>(FVector::Dist2D(GetActorLocation(), Node->GetActorLocation()));

	if (DistanceToNode <= ArrivalRadius)
	{
		if (!bArrivedAtNode)
		{
			// one-way latch: arrival happens exactly once per miner
			bArrivedAtNode = true;

			// stand at the node — idle mining. The poll can observe arrival before
			// path-following finishes (arrival ring 150 > walk acceptance 120), so
			// stop explicitly.
			if (AAIController* AI = Cast<AAIController>(GetController()))
			{
				AI->StopMovement();
			}

			// §6 mining "clink" loop (TASK-179): starts ON ARRIVAL (§3.3), stops on
			// death/freeze. Null-safe until S_MinerClink lands (TASK-180).
			StartMiningClink();

			// +1 gold/s activates ONLY on arrival (GDD §3.3): AddMinerIncome
			// exactly once, on the SAME player state we registered with
			// (handoffs/TASK-024.md contract).
			ASiegePlayerState* OwnerState = CachedOwnerState.Get();
			if (bRegisteredAlive && OwnerState)
			{
				OwnerState->AddMinerIncome();
				bIncomeActive = true;
			}
			else if (!bWarnedIncomeSkipped)
			{
				bWarnedIncomeSkipped = true;
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("AMinerUnit '%s': arrived at '%s' with no registered owner player state — mining activates no income (its team's ASiegePlayerState was never resolvable; TASK-043 resolves both Blue and Red)."),
					*GetNameSafe(this), *GetNameSafe(Node));
			}
		}
		// arrived and inside the ring: stand (a walk-back finishes on its own
		// at the 0.8 × ArrivalRadius acceptance)
		return;
	}

	// outside the ring: still walking (heal a failed/finished-short/hijacked
	// move), or displaced after arrival (walk back and stand again — income is
	// arrival-latched and unaffected by displacement, §3.3)
	EnsureWalkingToNode(Node);
}

void AMinerUnit::EnsureWalkingToNode(AGoldNode* Node)
{
	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		// AutoPossessAI possession can land after BeginPlay — the poll retries
		if (!bWarnedNoWalkController)
		{
			bWarnedNoWalkController = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AMinerUnit '%s': no AAIController possessing the miner (yet) — cannot walk to the gold node; the poll keeps retrying."),
				*GetNameSafe(this));
		}
		return;
	}
	bWarnedNoWalkController = false; // possession arrived — re-arm the warning (base pattern)

	// re-path only when path following is idle (finished/failed/never started)
	// or when the current move is pointed at a DIFFERENT goal — the base
	// EnterAdvance re-path gate plus a goal check. The goal check is what
	// replaces the one discarded castle-bound Advance from Super::BeginPlay,
	// and what heals any hypothetical hijack of the walk.
	const UPathFollowingComponent* PathFollow = AI->GetPathFollowingComponent();
	const bool bWalkingToNode = PathFollow
		&& AI->GetMoveStatus() != EPathFollowingStatus::Idle
		&& PathFollow->GetMoveGoal() == Node;
	if (bWalkingToNode)
	{
		return;
	}

	// acceptance 0.8 × ArrivalRadius (the house fraction, base EnterAdvance):
	// the natural stop lands well inside the arrival ring. bStopOnOverlap
	// false: the node deliberately carries NO collision to overlap-test
	// against (its collision must never block arrival — TASK-025 hard rule).
	// Partial paths allowed: a temporarily blocked route (M2b walls) walks as
	// close as possible and the poll re-paths as the dynamic navmesh updates.
	const EPathFollowingRequestResult::Type Result = AI->MoveToActor(Node, ArrivalRadius * 0.8f,
		/*bStopOnOverlap=*/ false, /*bUsePathfinding=*/ true, /*bCanStrafe=*/ true,
		/*FilterClass=*/ nullptr, /*bAllowPartialPath=*/ true);

	if (Result == EPathFollowingRequestResult::Failed && !bWarnedMoveFailed)
	{
		bWarnedMoveFailed = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AMinerUnit '%s': MoveToActor toward '%s' failed — is the NavMeshBoundsVolume covering L_Arena (TASK-015/TASK-036)? The poll keeps retrying."),
			*GetNameSafe(this), *GetNameSafe(Node));
	}
}

void AMinerUnit::TryRegisterWithOwnerState()
{
	if (bRegisteredAlive)
	{
		return;
	}

	ASiegePlayerState* OwnerState = ResolveOwningPlayerState();
	if (!OwnerState)
	{
		return; // unresolvable (warned once in the resolver) — poll retries
	}

	// latch BEFORE anything can re-enter: RegisterMinerAlive runs exactly once
	// per miner (handoffs/TASK-024.md contract; the cap itself is TASK-030's
	// play-time CanAddMiner gate — never checked here).
	CachedOwnerState = OwnerState;
	bRegisteredAlive = true;
	OwnerState->RegisterMinerAlive();
}

ASiegePlayerState* AMinerUnit::ResolveOwningPlayerState()
{
	// Multi-team economy (TASK-043): resolve the OWNING team's player state
	// through ASiegeGameState::GetPlayerStateForTeam(Team) rather than grabbing
	// the first player state (M2 assumed a single economy). With only the Blue
	// player present this returns the same single Blue state M2 resolved —
	// byte-identical income behaviour — while a Red bot miner (M3) resolves the
	// bot's Red state, so a Red miner raises only the bot's rate and vice-versa.
	const UWorld* World = GetWorld();
	ASiegeGameState* SiegeGameState = World ? World->GetGameState<ASiegeGameState>() : nullptr;
	if (!SiegeGameState)
	{
		// No game state yet (early-spawn ordering edge) — the arrival poll retries.
		if (!bWarnedNoOwnerState)
		{
			bWarnedNoOwnerState = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AMinerUnit '%s': no ASiegeGameState yet — economy registration retries on the arrival poll (GameStateClass = ASiegeGameState, TASK-024)."),
				*GetNameSafe(this));
		}
		return nullptr;
	}

	// GetPlayerStateForTeam logs its own "none for this team" warning (TASK-043),
	// so no duplicate here. In every real flow the owning economy exists before
	// any miner can spawn (Blue from match start; the M3 bot's Red before it
	// ever plays an 8-gold Miner), so this resolves on the first call and the
	// poll never re-queries.
	//
	// TASK-044 (closes qa/TASK-043 WARN): a genuinely mis-teamed miner would return
	// null here on EVERY 0.25 s upkeep poll, and GetPlayerStateForTeam logs
	// unconditionally on its not-found path — ~4 Warning lines/sec forever. One-shot
	// the team lookup: once it fails with a LIVE GameState, stop re-querying so the
	// accessor logs exactly once. This does NOT touch the no-GameState-yet retry above
	// (a separate branch that keeps retrying); it only stops the pointless re-query of
	// a miner whose team has no player state — which never happens in the designed
	// flows, where the owning economy exists before the miner spawns.
	if (bWarnedNoTeamPlayerState)
	{
		return nullptr;
	}

	ASiegePlayerState* OwnerState = SiegeGameState->GetPlayerStateForTeam(Team);
	if (!OwnerState)
	{
		bWarnedNoTeamPlayerState = true;
	}
	return OwnerState;
}

AGoldNode* AMinerUnit::FindNearestSameTeamGoldNode() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	AGoldNode* BestNode = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	// actor iteration per spec — two nodes in L_Arena, trivially cheap once at
	// BeginPlay. 2D distance for consistency with the arrival metric (flat
	// arena; the node has no collision for a closest-point measure anyway).
	for (TActorIterator<AGoldNode> It(World); It; ++It)
	{
		AGoldNode* Node = *It;
		if (!IsValid(Node) || Node->GetTeam() != Team)
		{
			continue;
		}

		const float Distance = static_cast<float>(FVector::Dist2D(MyLocation, Node->GetActorLocation()));
		if (Distance < BestDistance)
		{
			BestNode = Node;
			BestDistance = Distance;
		}
	}

	return BestNode;
}
