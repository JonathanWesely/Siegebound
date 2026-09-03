// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/MinerUnit.h"

#include "AIController.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GitClaudeUnrealTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "Siegebound/Castle.h" // TASK-398 Defend (E): FindNearestCastleForTeam / GetInteriorAnchorLocation
#include "Siegebound/GoldNode.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegeInvisibilityStatics.h" // TASK-829 (WITCH-§3a): ESiegeVeilBreakReason at the arrival-claim break site (also reached via MinerUnit.h -> SummonedUnit.h, which needs the complete enum for BreakInvisibility's parameter — explicit per IWYU, the SiegeStuckStatics.h precedent below)
#include "Siegebound/SiegePlayerController.h" // TASK-397 command seam: FindControllerForTeam / FindUnitGroup / HasIssuedCommand
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SiegeStuckStatics.h" // TASK-533: FSiegeStuckStatics::ComputeSidestepGoal + ESiegeStuckAction at the rung switch (also reached via MinerUnit.h, which needs the complete enum for the override's parameter — explicit per IWYU)
#include "Siegebound/UnitCommand.h" // TASK-397 command seam: FSiegeUnitGroup (complete type at the FindUnitGroup call)
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	/** §6 miner "clink" mining loop (TASK-179) — null-safe soft path; the loop flag is authored on the asset (TASK-180). */
	const TCHAR* MinerClinkSoundPath = TEXT("/Game/Audio/S_MinerClink");

	//~ TASK-398: NO formation constants live here. The golden-angle sunflower is
	//~ NOT re-derived in this class — the miner reads the station the controller
	//~ actually pushed into it (ASummonedUnit::GetGroupStationOffset, the public
	//~ read TASK-396 added for this call site). One computation, one owner, zero
	//~ drift: that is what makes "the miner uses Follow the same way as all other
	//~ commandable units" true by CONSTRUCTION rather than by two implementations
	//~ agreeing. GoldenAngleRadians / FollowFormationSlots stay private
	//~ implementation details of SiegePlayerController.cpp, where they belong.
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

	// (Seal #3 — the post-Super timer sweep — lives in BeginPlay.
	//  Seal #4 — CanEverAttack() -> false — is a header one-liner, TASK-397.)
	//
	// NOTE: the base clamps row Cadence to a 0.05 s minimum, so the Miner
	// row's Cadence 0 would NOT keep an attack timer unarmed by itself — if
	// this unit ever reached EnterAttack it would swing 20×/s. These seals are
	// load-bearing, not belt-and-braces.
	//
	// TASK-397 (CONVENTIONS §6 approach (B)): all three constructor/BeginPlay
	// seals are DELIBERATELY UNCHANGED. The miner becomes commandable through
	// its OWN poll (UpdateMining → ResolveMinerOrder), so it never needs the
	// base state timer — which is what keeps the 20×/s trap shut, keeps the
	// Profile-None fall-through to the castle-marching legacy body unreachable,
	// and keeps ONE driver on the movement component. Seal #4 is added ON TOP.
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
	//
	// ⚠️ THIS CALL ALSO RUNS CONVENTIONS §2's FOLLOW AUTO-ENROLL, and that is the
	// whole reason the next statement exists. See below.
	Super::BeginPlay();

	// ══ 🚩 THE §5 MINER SPAWN-DEFAULT RULING, ENFORCED (TASK-398, manager ruling 7,
	//    item 1 on Jonathan's playtest gate) ══
	//
	// CONVENTIONS §2 makes Follow the spawn default for every follow-eligible Blue
	// unit, enrolled from ASummonedUnit::BeginPlay — i.e. from the Super call one
	// line above. Applied literally to a miner that means a 24-gold ECONOMY card
	// that walks to the hero and earns NOTHING until personally micro'd. Ruling 7:
	// a miner SPAWNS MINING.
	//
	// The lever is CanFollowHero(), which answers bFollowOnSpawn until this line
	// runs: the enroll's IsFollowCommandEligible() gate refused, so this miner was
	// never added to the follow group NOR to its Members array — which is the part
	// that matters, because a stale Members entry would make
	// EnrollInDefaultFollowGroup's Contains() early-out swallow every future C press
	// (full reasoning in CanFollowHero()'s doc). The window is exactly one
	// synchronous call stack wide and closes HERE, unconditionally and before any
	// early-out below, so from this instant the miner is fully follow-eligible and
	// pressing C over it enrolls it for real.
	bSpawnFollowEnrollWindowClosed = true;

	// TRIPWIRE (not a fix — a fix would have to reach into the controller's Members
	// array, which this batch's file-ownership law forbids). If the auto-enroll ever
	// moves off IsFollowCommandEligible(), or is deferred past this call stack, a
	// miner will arrive here already carrying a group id. Clearing it keeps the
	// SHIPPED ruling (it mines) instead of silently deleting the economy, and the
	// Warning makes the regression loud at QA/PIE instead of invisible.
	if (!bFollowOnSpawn && GetCommandGroupId() != INDEX_NONE)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AMinerUnit '%s': spawned already enrolled in group %d — the §5 spawn-default window did not cover the follow auto-enroll (TASK-398). Cleared so the miner still MINES; the enroll site or CanFollowHero() needs re-checking."),
			*GetNameSafe(this), GetCommandGroupId());
		ClearCommandGroup();
	}

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

	// walk to the best NEUTRAL mine (W1-PREP TASK-254): FindBestMineFor is
	// THE finder (TASK-253) — tier-1 nearest minable-now, tier-2 nearest
	// enemy-occupied wait target. Null (every mine depleted, or none exist)
	// is a NORMAL state now, logged at Log inside SeekBestMine (demoted from
	// the M2 Error per the plan): the miner idles and the poll re-seeks.
	if (AGoldNode* Node = SeekBestMine())
	{
		EnsureWalkingToNode(Node);
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
		// Mine-side bookkeeping FIRST (TASK-254): drop out of the mine's
		// arrived registry BEFORE the player-state counts move, so the mine
		// never keeps draining for a miner whose income is being removed
		// (releases the claim + stops the drain when this was the last
		// occupant). UnregisterArrivedMiner is null-safe + idempotent: a
		// never-arrived, WAITING, or already-EVICTED miner (Deplete() emptied
		// the registry before NotifyMineDepleted cleared our latches, and
		// eviction nulls the target anyway) is a clean no-op — never a
		// double-unregister. World teardown deliberately skips this with the
		// rest of the Destroyed block (the mine tears down its own registry).
		if (AGoldNode* Mine = TargetGoldNode.Get())
		{
			Mine->UnregisterArrivedMiner(this);
		}

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

	// ---- THE STALL WATCHDOG (TASK-533; law: CONVENTIONS NAV-§3) -----------
	// ⛔ ZERO NEW TIMERS. The ladder rides THIS poll — the ArrivalCheckInterval
	// one that already exists, at the same 0.25 s cadence the base state timer
	// would have run at. NAV-§3 forbids a second 0.25 s driver on one
	// UPathFollowingComponent, and a SetTimer in this feature is a finding.
	//
	// ⛔⛔ THE DELTA IS THE WORLD CLOCK, VIA ConsumeStuckDeltaSeconds(), AND IT
	// IS NEVER StateCheckInterval. This class SEALS StateCheckInterval to 0
	// (constructor, seal #1) precisely so the base state machine has no driver
	// at all — so a delta read from it would be silently ZERO on every miner:
	// Evaluate's clamp (SiegeStuckStatics.cpp:60) would accumulate nothing, no
	// rung would EVER fire, and the whole feature would be a no-op on exactly
	// the unit class most likely to walk into a rock. ⚠️ It would fail SILENTLY
	// — no crash, no log, no assert — which is why TASK-532 put the clock read
	// behind ONE named helper instead of a literal duplicated across two files.
	//
	// ⚠️ AND THIS IS THE *ONLY* CONSUMER OF THAT HELPER ON A MINER, which is
	// what makes the delta whole rather than split between two call sites: seal
	// #1 means ASummonedUnit::UpdateState's own call site never runs on this
	// class (its timer is never armed, and seal #3's post-BeginPlay
	// ClearAllTimersForObject sweep covers anything that somehow was). The lone
	// synchronous UpdateState inside Super::BeginPlay is the one exception and
	// it is INERT by construction — the first call on any unit returns 0.
	TickStuckWatchdog(ConsumeStuckDeltaSeconds());

	// ⚠️ THE SIDESTEP LEASE — A DECLARED ADDITION OVER THE DISPATCH'S "TWO
	// THINGS", AND WITHOUT IT THE Sidestep RUNG IS A GUARANTEED SILENT NO-OP.
	// Exactly one movement driver runs per poll below (StandInPlace,
	// DriveToPoint or EnsureWalkingToNode — approach (B)'s one-driver promise),
	// and EVERY one of them would cancel an in-flight sidestep in the SAME call
	// stack that just issued it: EnsureWalkingToNode re-issues MoveToActor
	// because the live goal is a LOCATION and not the node, and DriveToPoint
	// re-issues because the order's point has drifted ~SidestepDistance from
	// the sidestep goal. While the lease is live this poll's STEERING is
	// suppressed so the rescue move can actually run. It reuses the base's
	// existing member — no new state: TickStuckWatchdog above drained it on the
	// same clock as the ladder, and the base clears it at every exit (EnterIdle,
	// EnterAttack, HandleDeath, FreezeAI, ApplyFreeze), so it cannot outlive
	// the stall or the unit.
	//
	// ⚠️ WHAT IT DELAYS, STATED HONESTLY RATHER THAN CLAIMED AWAY: the
	// alive-registration retry runs ABOVE this line; eviction is push-driven
	// (NotifyMineDepleted) and death runs through EndPlay, so neither can be
	// delayed at all. What CAN be delayed, by at most SidestepLeaseSeconds
	// (2.0 s), is the AT-THE-RING test — arrival/registration and the wait-mode
	// retry — and only for a miner that is BY CONSTRUCTION wedged and being
	// steered laterally away from that ring. ⛔ No latch, no rate, no cap and no
	// tenure rule changes: only WHEN this miner's next arrival poll happens.
	if (SidestepLeaseRemaining > 0.f)
	{
		return;
	}

	// ---- THE COMMAND SEAM (TASK-397 plumbing, TASK-398 semantics) ---------
	// ONE read of the owning-team controller's live command state, producing
	// this poll's destination decision. Placed AFTER the alive-registration
	// retry (the §3.3 alive count is lifetime bookkeeping and must run under
	// every order) and BEFORE the retarget gate (the first thing an order can
	// legitimately change is WHICH destination this poll pursues).
	const FMinerOrder Order = ResolveMinerOrder();

	// ---- THE FIVE-COMMAND DISPATCH (TASK-398; CONVENTIONS §5's table) -----
	// Exactly ONE movement driver runs per poll — that is approach (B)'s core
	// promise and why the TASK-280/282 re-path mill cannot reappear here.
	if (Order.Mode == EMinerOrderMode::Stand || Order.Mode == EMinerOrderMode::GoToPoint)
	{
		// Walking away from mining ENDS THE TENURE, in the EndPlay(Destroyed)
		// order: mine-side release first (the exclusive claim + the drain), then
		// income/latches, then drop the target. Without it a miner would earn
		// +1 gold/s while hiding inside its own castle AND pin that mine's claim
		// against every other miner on the map. Idempotent — free after the
		// first poll of this order.
		LeaveMining();

		if (Order.Mode == EMinerOrderMode::Stand)
		{
			// The two RULED no-destination cases: a dead/unresolvable hero under
			// Follow (CONVENTIONS §4 — HOLD POSITION, resume the instant a live
			// pawn resolves) and a destroyed own castle under Defend (§5 — idle
			// in place). Never a read failure: those answer Mine.
			StandInPlace();
		}
		else
		{
			// Defend (the castle interior anchor) and Follow (the hero's live
			// station). Same body, different point; the anti-repath band comes
			// with the order because only Follow's anchor moves.
			DriveToPoint(Order.Point, Order.RepathTolerance);
		}
		return;
	}

	// Mine (Attack / no command yet) and MineInDisc (Hold / Ambush) BOTH run the
	// mining body below. They differ in exactly two places, both marked: which
	// finder the retarget gate consults, and what happens when it comes back
	// empty. Everything from the arrival test down is SHARED — which is what
	// makes income, tenure and eviction identical under every mining order.

	// ---- retarget gate (TASK-254) ----------------------------------------
	AGoldNode* Node = TargetGoldNode.Get();

	// ⚠️ DIFFERENCE 1a (TASK-398, MineInDisc only): a mine OUTSIDE the position
	// circle is not a legal destination, so it is treated exactly like a depleted
	// one. This is what re-homes a miner when the player draws a NEW Hold circle
	// somewhere else — mines never move, but the circle does. Boundary inclusive,
	// 2D, matching AGoldNode::FindBestMineInDisc's own gate (they MUST agree, or a
	// mine could be legal to the finder and illegal to this test and the miner
	// would oscillate).
	const bool bOutOfOrderDisc = Node
		&& Order.Mode == EMinerOrderMode::MineInDisc
		&& static_cast<float>(FVector::DistSquared2D(Node->GetActorLocation(), Order.Point)) > FMath::Square(Order.Radius);

	// ONE dead-target predicate for both steps below (they MUST agree: a
	// re-seek may only run on a tenure that was already ended, or the
	// arrival/income flags would carry over to the new target). Short-
	// circuits: reserve is only read on a valid node.
	const bool bTargetDead = !Node || Node->IsDepleted() || Node->GetGoldReserve() <= 0 || bOutOfOrderDisc;

	// Defensive tenure break: ARRIVED at a mine that vanished, or that reads
	// depleted/empty without having evicted us. No designed flow reaches
	// this — ClearScatter only destroys mines after the PlayAgain sweep
	// killed every miner, and Deplete() evicts synchronously
	// (NotifyMineDepleted clears bArrivedAtNode) before any poll can observe
	// its latch. Kept so the arrival/income flags can never outlive their
	// mine: un-arrive locally, then fall through to the re-seek. Silent by
	// design (a normal state now, not the M2 level-authoring diagnostic).
	//
	// ⚠️ TASK-398 upgraded EndMineTenure() to LeaveMining() here — the SAME call
	// plus the mine-side UnregisterArrivedMiner — because bOutOfOrderDisc adds a
	// REACHABLE way to reach this line with a perfectly healthy, still-claimed
	// mine (a new Hold circle drawn elsewhere), and that mine must be released or
	// its exclusive claim leaks for the rest of the match. On the pre-existing
	// paths the extra call is provably a no-op: the mine is either destroyed
	// (weak pointer null) or depleted (Deplete() emptied its registry before
	// NotifyMineDepleted ran), so UnregisterArrivedMiner has nothing to remove.
	if (bArrivedAtNode && bTargetDead)
	{
		LeaveMining();
		Node = nullptr; // LeaveMining dropped the target; the re-seek below owns it
	}

	if (bTargetDead)
	{
		// ⚠️ DIFFERENCE 1b (TASK-398): WHICH finder. Same call shape, same
		// tier-1/tier-2 rules, same no-churn tiebreak — the disc variant just
		// adds the in-circle gate.
		Node = (Order.Mode == EMinerOrderMode::MineInDisc) ? SeekMineInDisc(Order) : SeekBestMine();
		if (!Node)
		{
			// ⚠️ DIFFERENCE 2 (TASK-398): what an EMPTY finder means.
			if (Order.Mode == EMinerOrderMode::MineInDisc)
			{
				// Hold/Ambush ladder rung 2 — no mine in the circle, so go stand
				// at this miner's own slot inside it. Jonathan: "it only goes to
				// the position circle and will mine a mine if there is one in the
				// position circle." The poll keeps re-seeking, so a mine freed by
				// an enemy miner leaving IS picked up without a new order.
				// Tolerance 0: the station is a fixed point, so an in-flight move
				// toward it is never re-issued.
				DriveToPoint(Order.Station, 0.f);
			}
			// Mine: unchanged — the all-depleted endgame. Idle in place (logged
			// once inside SeekBestMine) and keep polling: the intended income death.
			return;
		}
	}
	else if (!bArrivedAtNode && !Node->CanTeamMine(Team))
	{
		// Un-arrived and pointed at an enemy-claimed mine (walking to it, or
		// queued at its ring): consult the finder for an UPGRADE. Switch ONLY
		// when it returns a minable-now (tier-1) mine; a tier-2 result — even
		// a nearer queue — keeps the current target. The no-churn rule: never
		// flip between wait targets or equal options (the finder's strict-<
		// tiebreak already pins exact ties). ARRIVED miners never retarget.
		// ⚠️ DIFFERENCE 1c (TASK-398): the upgrade is searched in the SAME space
		// the order legalises, so a Hold miner can never be upgraded OUT of its
		// circle (which would then fail bOutOfOrderDisc next poll and oscillate).
		AGoldNode* const Upgrade = (Order.Mode == EMinerOrderMode::MineInDisc)
			? AGoldNode::FindBestMineInDisc(GetWorld(), Team, Order.Point, Order.Radius, GetActorLocation())
			: AGoldNode::FindBestMineFor(GetWorld(), Team, GetActorLocation());
		if (Upgrade)
		{
			if (Upgrade != Node && Upgrade->CanTeamMine(Team))
			{
				Node = Upgrade;
				TargetGoldNode = Upgrade;
				bLoggedWaitingAtMine = false; // new target — the next queue is a new episode
			}
		}
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
			// At the ring: registration is the atomic claim point (TASK-253
			// TryRegisterArrivedMiner — claims the mine on the 0 -> 1
			// registry transition). Success runs the M2 arrival block below
			// unchanged (TASK-024/179 contract); refusal is WAIT MODE.
			if (Node->TryRegisterArrivedMiner(this))
			{
				// ══ VEIL BREAK — `Mine` (TASK-829; ⛔ WITCH-§3a TRAP 3 of 3) ══════════════════
				// ⛔⛔⛔ THE BREAK BELONGS AT THE ⛔ ARRIVAL, ⛔ NOT AT THE PAYOUT — and the
				// obvious wiring point is not merely wrong, it is ⛔ catastrophic. ⛔ GOLD IS
				// ⛔ NEVER GRANTED ON THE MINER'S CALL STACK: the node's reserve drains on the
				// NODE's timer (AGoldNode::HandleDrainTick) and the player's +gold/s is granted
				// on the PLAYER STATE's 1 s timer (ASiegePlayerState::HandleGoldTick → SetGold).
				// ⇒ a hook on "where the gold appears" would break the veil of ⛔ EVERY MINER THE
				// PLAYER OWNS, ⛔ including ones still walking across the field.
				// ⭐ THIS arm fires ⛔ ONCE PER TENURE, on the ⛔ ACTING miner, at the atomic claim
				// — the exact moment his sentence means by "mine". ⭐ It is inside the SUCCESS
				// branch: a miner REFUSED by an enemy-claimed node falls to WAIT MODE below and
				// ⛔ stays veiled, because waiting is standing still.
				// ⚠️ Walking to the node is ⛔ NOT mining (his carve-out), and the per-second
				// income tick that follows needs ⛔ no belt of its own — ApplyBreak is idempotent,
				// and this arm already fired for this tenure.
				BreakInvisibility(ESiegeVeilBreakReason::Mine);

				bLoggedWaitingAtMine = false; // tenure starts — re-arm for any later queue

				// per-tenure latch (TASK-254; was one-way per lifetime in
				// M2): arrival happens exactly once per TENURE — cleared only
				// by eviction, never by displacement
				bArrivedAtNode = true;

				// stand at the node — idle mining. The poll can observe arrival before
				// path-following finishes (arrival ring 150 > walk acceptance 120), so
				// stop explicitly.
				if (AAIController* AI = Cast<AAIController>(GetController()))
				{
					AI->StopMovement();
				}

				// §6 mining "clink" loop (TASK-179): starts ON ARRIVAL (§3.3), stops on
				// death/freeze/eviction. Null-safe until S_MinerClink lands (TASK-180).
				StartMiningClink();

				// +1 gold/s activates ONLY on registered arrival (GDD §3.3):
				// AddMinerIncome exactly once per tenure, on the SAME player
				// state we registered with (handoffs/TASK-024.md contract).
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
			else
			{
				// WAIT MODE (TASK-254): the mine refused us — enemy-claimed
				// (or it depleted this very tick; the next poll's retarget
				// gate catches that case). Stand at the ring — the in-flight
				// move finishes at its 0.8 × acceptance on its own, and the
				// outside-ring branch walks a displaced waiter back — and
				// retry every poll: the instant the last enemy occupant
				// leaves or dies, UnregisterArrivedMiner releases the claim
				// and this TryRegister succeeds (auto-claim).
				if (!bLoggedWaitingAtMine)
				{
					bLoggedWaitingAtMine = true;
					UE_LOG(LogGitClaudeUnrealTest, Log,
						TEXT("AMinerUnit '%s': waiting at enemy-claimed mine '%s' — standing at the ring until it frees."),
						*GetNameSafe(this), *GetNameSafe(Node));
				}
			}
		}
		// arrived (or waiting) and inside the ring: stand (a walk-back
		// finishes on its own at the 0.8 × ArrivalRadius acceptance)
		return;
	}

	// outside the ring: still walking (heal a failed/finished-short/hijacked
	// move), or displaced after arrival / while waiting (walk back and stand
	// again — an arrived tenure's income is arrival-latched and unaffected by
	// displacement, §3.3)
	EnsureWalkingToNode(Node);
}

FMinerOrder AMinerUnit::ResolveMinerOrder()
{
	// ══ THE COMMAND SEAM (TASK-397 plumbing; CONVENTIONS "FOLLOW command …
	//    (2026-08-02)" §5's table wired on by TASK-398, approach (B) — see the
	//    class doc for why (A) was rejected) ══
	//
	// TASK-398 changed RETURNS ONLY: the branch structure, the precedence and the
	// degrade rule are all TASK-397's, unmodified.
	FMinerOrder Order; // EMinerOrderMode::Mine — today's loop, and the default answer

	// The command surface is BLUE-ONLY today, by shipped law: both
	// IsGroupCommandEligible() and UpdateStateStandardCommanded's gate carry
	// `Team == ETeamId::Blue` (the Red human's stance surface is M8 P2 scope).
	// The bot is an AAIController and is never in the player-controller
	// iterator, so a Red resolve would return null anyway — this early-out just
	// makes a RED BOT MINER's poll provably free of the seam entirely: one enum
	// compare, no world query, no iteration.
	if (Team != ETeamId::Blue)
	{
		return Order;
	}

	UWorld* const World = GetWorld();
	const ASiegePlayerController* const PC =
		World ? ASiegePlayerController::FindControllerForTeam(World, Team) : nullptr;
	if (!PC)
	{
		// No owning-team controller resolvable (early spawn before possession,
		// teardown). THE DEGRADE RULE: an unreadable order answers Mine — a
		// miner that cannot hear its orders keeps EARNING. Never a stall, never
		// a crash. FindControllerForTeam is silent and allocation-free, so this
		// adds no log traffic on the retry path (unlike the resolver above it,
		// which one-shots its warnings).
		return Order;
	}

	// ── (1) THE PER-UNIT GROUP ORDER (Hold / Ambush / Follow) ──────────────
	// Precedence mirrors the base deliberately: ASummonedUnit::UpdateState
	// dispatches group orders ABOVE the team-wide stance gate, so a circled
	// miner must not be overridden by a later T/E press it was never part of.
	// GetCommandGroupId() is the public accessor (CommandGroupId is private on
	// the base) — no base-class change needed to read it.
	const int32 GroupId = GetCommandGroupId();
	if (GroupId != INDEX_NONE)
	{
		// Live read, NEVER cached: the returned pointer aliases into the
		// controller's UnitGroups array, which mutates on confirm/steal/prune.
		if (const FSiegeUnitGroup* const Group = PC->FindUnitGroup(GroupId))
		{
			switch (Group->Type)
			{
			case ESiegeGroupCommandType::Hold:
			case ESiegeGroupCommandType::Ambush:
			{
				// ⚠️ HOLD AND AMBUSH COLLAPSE FOR A MINER — Jonathan said so in as
				// many words ("'Ambush' is the same thing as 'hold'"), and they
				// share this one case label rather than a duplicated body:
				// IMPLEMENTED ONCE, so they cannot drift. The leash-exemption that
				// distinguishes them for a fighter is meaningless with no target.
				//
				// THE ATTACKING PORTION IS SKIPPED ENTIRELY: this returns a
				// destination decision and nothing else — there is no tier-1
				// attack-zone acquisition and no tier-2 position-zone acquisition
				// anywhere in this class, and Group->AttackCenter/AttackRadius are
				// deliberately never read.
				Order.Mode = EMinerOrderMode::MineInDisc;
				Order.Point = Group->PositionCenter;
				Order.Radius = FMath::Max(Group->PositionRadius, 0.f);

				// Ladder rung 2, precomputed here so the body never has to re-enter
				// the group: PositionCenter + the station the CONTROLLER pushed into
				// this unit at the stage-3 confirm — the identical expression the
				// base's own tier-3 station keeping uses. Read, never re-derived
				// (ASummonedUnit::GetGroupStationOffset, the public accessor TASK-396
				// added for this line): a second sunflower implementation here could
				// disagree with the controller's, and nothing would catch it.
				Order.Station = Group->PositionCenter + GetGroupStationOffset();
				return Order;
			}

			case ESiegeGroupCommandType::Follow:
			{
				// "The miner will use the 'follow' command the same way as all
				// other commandable units follow it." (Jonathan, §5.)
				//
				// THE ANCHOR IS RESOLVED LIVE, EVERY POLL, AND NEVER CACHED
				// (CONVENTIONS §4): the respawn path may hand back a DIFFERENT pawn
				// actor, so a cached pointer would follow a corpse forever — and
				// resolving live is exactly what makes respawn work for free.
				const AActor* const Anchor = PC->GetFollowAnchor();
				if (!Anchor)
				{
					// HERO-DEATH RULING (§4, manager ruling 8): hold position while
					// the hero is down — no march, no target, no attack — and resume
					// the instant a live pawn resolves. Deliberately NOT a fall
					// through to mining: a following miner the player pulled off the
					// mines must not silently go back to work because the hero died.
					Order.Mode = EMinerOrderMode::Stand;
					return Order;
				}

				// ⚠️ THE SAME EXPRESSION ASummonedUnit::UpdateStateFollow USES —
				// `Anchor->GetActorLocation() + GroupStationOffset` — reached through
				// the public getter because the miner runs in its OWN poll rather than
				// in the base state machine. That identity is how "the miner will use
				// the 'follow' command the same way as all other commandable units"
				// (Jonathan) is guaranteed rather than merely intended: same anchor,
				// same offset, same 150 uu arrival, same anti-repath band.
				// FollowFormationRadius is deliberately NOT read here — the controller
				// already scaled the offset by it at enroll.
				Order.Mode = EMinerOrderMode::GoToPoint;
				Order.Point = Anchor->GetActorLocation() + GetGroupStationOffset();

				// ⚠️ THE ANTI-REPATH BAND, and the ONLY order that needs one: the
				// hero MOVES, so the recomputed station moves every poll. Carried on
				// the order so DriveToPoint stays a dumb mover. Read LIVE off the
				// controller (one tunable drives every follower, §8).
				Order.RepathTolerance = FMath::Max(PC->GetFollowRepathTolerance(), 0.f);
				return Order;
			}

			default:
				// An unrecognised group type (a future enum value reaching an old
				// miner). THE DEGRADE RULE: answer Mine — keep earning, never stall.
				return Order;
			}
		}

		// Dead id — the group was released by T/E, emptied by a steal, pruned
		// all-dead, or wiped by Play Again. SELF-HEAL and fall through to the
		// stance read in this SAME poll (never a stall), exactly as
		// ASummonedUnit::UpdateState's group dispatch does.
		// ClearCommandGroup also ZEROES GroupStationOffset, so the station this class
		// reads can never outlive the group it belonged to — no local cache to
		// invalidate, which is the second reason for reading rather than deriving.
		ClearCommandGroup();
	}

	// ── (2) THE TEAM-WIDE STANCE (T / E) ───────────────────────────────────
	// HasIssuedCommand() is false until the player's first press, and the
	// shipped law for that window is "run the legacy body" (TASK-275's
	// zero-behavior-change gate). For a miner the legacy body IS the mining
	// loop, so a pre-first-press miner mines — which is also what the flagged
	// §5 MINER SPAWN-DEFAULT ruling wants.
	if (PC->HasIssuedCommand())
	{
		switch (PC->GetCurrentCommand())
		{
		case ESiegeUnitCommand::Defend:
		{
			// "'Defend' means they come back to the castle and hide inside of it."
			//
			// ⚠️ THIS INVENTS NO MECHANIC — IT REUSES TASK-350 (§5, explicit). The
			// 3× castle is hollow and walk-in and own-team units already enter
			// through the shipped team gating; "inside" resolves to exactly one
			// concrete thing, ACastle::GetInteriorAnchorLocation(), and the walk is
			// an ordinary unfiltered MoveToLocation. No mining (LeaveMining runs on
			// the way in), no attacking (this class cannot).
			//
			// ⚠️ ACastle::FindNearestCastleForTeam rather than the base's
			// FindOwnCastle(): that one is PRIVATE on ASummonedUnit and its file is
			// another task's this batch (the same private-surface wall that killed
			// approach (A)). The new static is a faithful mirror living on ACastle —
			// see its doc.
			ACastle* const OwnCastle = ACastle::FindNearestCastleForTeam(World, Team, GetActorLocation());
			if (!OwnCastle)
			{
				// §5: "Own castle destroyed ⇒ idle in place." (The finder skips
				// destroyed castles, so this is that case — and also the
				// no-castle-placed sandbox map.)
				if (!bLoggedNoOwnCastle)
				{
					bLoggedNoOwnCastle = true;
					UE_LOG(LogGitClaudeUnrealTest, Log,
						TEXT("AMinerUnit '%s': DEFEND ordered but this team has no standing castle to hide in — idling in place (TASK-398, CONVENTIONS §5)."),
						*GetNameSafe(this));
				}
				Order.Mode = EMinerOrderMode::Stand;
				return Order;
			}
			bLoggedNoOwnCastle = false; // a castle resolved again (Play Again) — re-arm

			Order.Mode = EMinerOrderMode::GoToPoint;
			Order.Point = OwnCastle->GetInteriorAnchorLocation();
			// RepathTolerance stays 0: a castle does not move, so an in-flight walk
			// to it is never re-issued (drift 0 <= 0).

			// THE §5 "MEASURE, DO NOT ASSUME" DUTY, discharged at runtime: ONE Log
			// line per miner lifetime carrying the RESOLVED world point, so the PIE
			// task can read the anchor back from the log without an editor probe.
			// At the shipped ZeroVector default that reads (−25000, 0, 0) for
			// Castle_Blue and (25000, 0, 0) for Castle_Red — the derivation is in
			// handoffs/TASK-398-programmer.md; THIS LINE is what proves it.
			if (!bLoggedInteriorAnchor)
			{
				bLoggedInteriorAnchor = true;
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("AMinerUnit '%s': DEFEND — hiding inside '%s'; interior anchor resolves to %s (castle at %s) (TASK-398)."),
					*GetNameSafe(this), *GetNameSafe(OwnCastle),
					*Order.Point.ToCompactString(), *OwnCastle->GetActorLocation().ToCompactString());
			}
			return Order;
		}

		case ESiegeUnitCommand::Attack:
			// "'Attack' means they find the nearest mine and start mining." — which
			// IS today's shipped loop (SeekBestMine → AGoldNode::FindBestMineFor →
			// walk → register → income), reached by returning the SAME
			// EMinerOrderMode::Mine the un-commanded path returns. §5 calls that
			// equivalence out as the cheapest regression proof in the batch, and it
			// is an identity here rather than a copy: one mode, one body.
			return Order;

		case ESiegeUnitCommand::Hold:
		default:
			// NOTHING LATCHES THE HOLD STANCE since TASK-344 — the enum member
			// survives only because WBP_HUD's switch pins the byte layout. R opens
			// the group pick instead, so Hold reaches a miner as a GROUP order
			// (above), never here. Degrade rule: keep earning.
			return Order;
		}
	}

	// No command issued yet this match. The shipped law for that window is "run
	// the legacy body" (TASK-275's zero-behavior-change gate), and for a miner the
	// legacy body IS the mining loop — which is also exactly what §5's flagged
	// MINER SPAWN-DEFAULT ruling wants. The two agree, so nothing special is done
	// here; the spawn-side half of that ruling lives in BeginPlay + bFollowOnSpawn.
	return Order;
}

void AMinerUnit::LeaveMining()
{
	// THE ORDER IS LOAD-BEARING and is the same one EndPlay(Destroyed) uses.
	//
	// 1) MINE SIDE FIRST: release this miner's share of the exclusive claim (and,
	//    when it was the last occupant, stop the drain). Null-safe + idempotent —
	//    a never-arrived, WAITING or already-evicted miner is a clean no-op.
	//    Skipping this would pin the mine against every other miner on the map for
	//    as long as this one stands somewhere else.
	if (AGoldNode* const Mine = TargetGoldNode.Get())
	{
		Mine->UnregisterArrivedMiner(this);
	}

	// 2) THEN the income/latch side: RemoveMinerIncome iff this tenure had income,
	//    per-tenure latches cleared, clink stopped. Idempotent. Skipping it would
	//    leave a miner earning +1 gold/s while hiding inside its own castle.
	EndMineTenure();

	// 3) THEN drop the target — on the way OUT of mining, never on the way back
	//    in. Returning to a mining order is a FRESH tenure, and the retarget gate
	//    re-seeks the moment it sees a null target.
	TargetGoldNode = nullptr;

	// Deliberately NOT touched here: bHasIssuedPointGoal. This runs on EVERY poll
	// of a non-mining order, and clearing the anti-repath latch here would defeat
	// the band entirely — a fresh MoveToLocation every 0.25 s at a moving hero is
	// precisely the TASK-280/282 mill. StandInPlace and EnsureWalkingToNode are the
	// two places that invalidate it, because those are the real mode changes.
}

void AMinerUnit::StandInPlace()
{
	if (AAIController* const AI = Cast<AAIController>(GetController()))
	{
		// Guarded so a standing miner does not spam StopMovement at 4 Hz.
		if (AI->GetMoveStatus() != EPathFollowingStatus::Idle)
		{
			AI->StopMovement();
		}
	}

	// No destination is currently intended, so the next GoToPoint must re-path
	// from scratch rather than measure drift against a goal we abandoned.
	bHasIssuedPointGoal = false;
}

void AMinerUnit::DriveToPoint(const FVector& Point, float RepathTolerance)
{
	AAIController* const AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		// AutoPossessAI possession can land after BeginPlay — the poll retries.
		// Shares EnsureWalkingToNode's one-shot guard deliberately: it is the same
		// condition on the same actor, and two latches would double the log line.
		if (!bWarnedNoWalkController)
		{
			bWarnedNoWalkController = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AMinerUnit '%s': no AAIController possessing the miner (yet) — cannot walk to its ordered point; the poll keeps retrying."),
				*GetNameSafe(this));
		}
		return;
	}
	bWarnedNoWalkController = false; // possession arrived — re-arm the warning (base pattern)

	// ARRIVED: the shipped 150 uu tolerance, measured in 2D for the same reason the
	// mine arrival test is (the arena is flat and a character's location is its
	// capsule CENTRE ~90 uu up, so a 3D test would burn most of the budget
	// vertically). Stop and stand — do NOT clear the goal latch: the miner is
	// holding this exact point, and the hero drifting a few units must not
	// re-trigger a walk.
	if (static_cast<float>(FVector::Dist2D(GetActorLocation(), Point)) <= ArrivalRadius)
	{
		if (AI->GetMoveStatus() != EPathFollowingStatus::Idle)
		{
			AI->StopMovement();
		}
		return;
	}

	// ⚠️ THE ANTI-REPATH BAND (CONVENTIONS §4, manager ruling 10 — a QA criterion,
	// not polish). While a move is IN FLIGHT, re-issue ONLY once the destination
	// has drifted further than RepathTolerance from the goal we last issued.
	// Measured goal-to-goal, never goal-to-miner: the miner is supposed to be far
	// from its goal while walking, and testing that distance would re-path forever.
	//
	// Follow passes ASiegePlayerController::FollowRepathTolerance (250 uu shipped);
	// the static orders pass 0, which still suppresses every re-issue while in
	// flight because a fixed goal's drift is exactly 0.
	const bool bMoveInFlight = AI->GetMoveStatus() != EPathFollowingStatus::Idle;
	const bool bWithinBand = bHasIssuedPointGoal
		&& static_cast<float>(FVector::DistSquared2D(Point, LastIssuedPointGoal)) <= FMath::Square(RepathTolerance);
	if (bMoveInFlight && bWithinBand)
	{
		return;
	}

	// Acceptance 0.8 × ArrivalRadius (the house fraction, base EnterAdvance): the
	// natural stop lands well inside the arrival ring above.
	// bProjectDestinationToNavigation TRUE — the castle interior anchor is a raw
	// ground point and a follow station is an offset from a walking pawn, so
	// neither is guaranteed to sit on the navmesh. FilterClass null = an UNFILTERED
	// query, which is exactly what lets the OWN team path across its castle's
	// interior nav area (TASK-350's UNavFilter_Team* excludes only the ENEMY).
	// Partial paths allowed: a blocked route walks as close as possible and the
	// poll re-paths as the dynamic navmesh updates.
	const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Point, ArrivalRadius * 0.8f,
		/*bStopOnOverlap=*/ false, /*bUsePathfinding=*/ true, /*bProjectDestinationToNavigation=*/ true,
		/*bCanStrafe=*/ true, /*FilterClass=*/ nullptr, /*bAllowPartialPath=*/ true);

	LastIssuedPointGoal = Point;
	bHasIssuedPointGoal = true;

	if (Result == EPathFollowingRequestResult::Failed && !bWarnedPointMoveFailed)
	{
		// §5 rules a nav failure that strands the miner ACCEPTABLE — log once, keep
		// polling. It must NEVER crash and must never fall back to attacking (this
		// class structurally cannot attack at all).
		bWarnedPointMoveFailed = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AMinerUnit '%s': MoveToLocation toward %s failed — no navmesh path (a castle-interior anchor outside the navmesh will strand the miner at the gate; that is accepted, TASK-398/§5). The poll keeps retrying."),
			*GetNameSafe(this), *Point.ToCompactString());
	}
}

void AMinerUnit::EnsureWalkingToNode(AGoldNode* Node)
{
	// TASK-398: this poll is NODE-bound, so any point goal DriveToPoint latched is
	// stale. Invalidating it here is what keeps the anti-repath band from ever
	// suppressing the FIRST move of a new point order — without this, switching
	// mining → Follow while a node walk is in flight could leave the miner walking
	// to the mine (the move status reads "in flight", and a hero standing near the
	// abandoned goal would read "within band"). Cleared unconditionally, including
	// on the early-outs below: the mode has changed either way.
	bHasIssuedPointGoal = false;

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

void AMinerUnit::HandleStuckEscalation(ESiegeStuckAction Action)
{
	// ⛔ THE DECISION IS THE BASE'S AND STAYS THERE. FSiegeStuckStatics::Evaluate
	// already chose this rung, already applied both anti-mill brakes, and
	// TickStuckWatchdog already emitted the pinned `escalate:` line (TASK-532 logs it
	// at the call site precisely so this override gets the same tokens for free).
	// ⛔ Only the ACTION is ours — see the header comment for why it HAS to be.
	switch (Action)
	{
	case ESiegeStuckAction::Sidestep:
	{
		// The sidestep is PERPENDICULAR to the direction of travel, so it needs the
		// point this miner is actually walking at. This class holds exactly two kinds
		// of intent and they are mutually exclusive by construction:
		//   - a MINE (Mine / MineInDisc)                 -> the node's location;
		//   - a POINT (Defend / Follow / the MineInDisc
		//     station)                                   -> the goal DriveToPoint
		//                                                   last issued.
		// LeaveMining() nulls TargetGoldNode on the way INTO the point orders, so the
		// node source is live only under a mining order. A degenerate input — neither
		// available — is answered by FSiegeStuckStatics with a stable +Y fallback axis
		// rather than a NaN (its pinned contract), so no extra guard is owed here.
		FVector StalledGoal = GetActorLocation();
		if (const AGoldNode* const Node = TargetGoldNode.Get())
		{
			StalledGoal = Node->GetActorLocation();
		}
		else if (bHasIssuedPointGoal)
		{
			StalledGoal = LastIssuedPointGoal;
		}

		// The same Attempt idiom the base uses, for the same two reasons, and against the
		// SAME inherited counter (SidestepAttemptCount is ASummonedUnit's — this class adds
		// no member of its own): the free-running attempt term ALTERNATES THIS MINER'S OWN
		// SIDE ACROSS SUCCESSIVE STALLS, and the unit-id term DE-CORRELATES NEIGHBOURS so a
		// clump of miners wedged on the SAME rock does not all sidestep the same way into
		// each other — one stuck miner becoming several.
		// ⛔ The counter deliberately does NOT live in FSiegeStuckState: Reset clears that
		// struct on every Abandon and every re-anchor, and the first shipped version's
		// StuckState.EscalationLevel term was therefore a per-unit CONSTANT (always 1 here),
		// so a wedged miner stepped the same way into the same rock forever
		// (qa/TASK-537.md, the BLOCKER). The uint8 wrap at 255 -> 0 is harmless — only the
		// parity is read. Non-negative by construction, so the callee's parity test is well
		// defined.
		const int32 Attempt = static_cast<int32>(SidestepAttemptCount++)
			+ static_cast<int32>(GetUniqueID() % 2);

		const FVector SidestepPoint = FSiegeStuckStatics::ComputeSidestepGoal(
			GetActorLocation(), StalledGoal, StuckTuning.SidestepDistance, Attempt);

		// ⛔ ARM THE LEASE ONLY IF THERE IS ACTUALLY SOMEWHERE TO WALK. DriveToPoint
		// treats anything inside ArrivalRadius as ARRIVED and answers with
		// StopMovement instead of a move, so a SidestepDistance tuned at or below the
		// 150 uu ring — or the degenerate `return Location` ComputeSidestepGoal gives
		// for a non-positive/NaN distance — would PARK the miner for the whole lease
		// and call that a rescue. FSiegeStuckTuning is EditDefaultsOnly and Jonathan
		// tunes it without a recompile, so this rung has to be safe under arbitrary
		// values (NAV-§7), not just the shipped 350.
		if (static_cast<float>(FVector::Dist2D(GetActorLocation(), SidestepPoint)) > ArrivalRadius)
		{
			SidestepLeaseRemaining = StuckTuning.SidestepLeaseSeconds;
		}

		// ⭐ ONE move, through the MINER'S OWN mover and its SHIPPED anti-repath band
		// (:841-856) — ⛔ never EnterAdvanceToLocation, which is the base's mover and
		// would put a SECOND steering authority on this UPathFollowingComponent.
		// Tolerance 0 reuses that band exactly as the static orders do: it suppresses
		// a re-issue only when the destination has not moved at all, which is what
		// keeps this rung to EXACTLY ONE path request and inside NAV-§3's ceiling.
		DriveToPoint(SidestepPoint, 0.f);
		break;
	}

	case ESiegeStuckAction::WidenAndRepath:
	{
		// The ladder has climbed PAST the sidestep, so the sidestep is over: drop the
		// lease FIRST or the poll would return on it and the re-path this rung exists
		// to cause would never reach a mover — and because EscalationLevel is
		// MONOTONIC the rung could never fire again. With the shipped defaults the
		// lease IS still live here (armed at 1.5 s for 2.0 s; Widen fires at 3.0 s),
		// so this ordering is the difference between a working rung and a silent
		// no-op. Same argument as TASK-532's base rung.
		SidestepLeaseRemaining = 0.f;

		// A GENUINE re-path toward the ORIGINAL destination. ⛔ The goal itself is NOT
		// changed and the standing ORDER is not cancelled — only the "we already asked
		// for this" bookkeeping that would otherwise swallow the re-issue. This class
		// holds TWO such latches and the rung has to break BOTH, because which one is
		// live depends on the order the miner is under:
		//
		//   1. bHasIssuedPointGoal — DriveToPoint's anti-repath band (:851). Cleared
		//      here so the next Defend/Follow/station drive re-issues MoveToLocation
		//      instead of measuring drift against a goal we are wedged short of. For
		//      those orders this clear IS the whole re-path: UpdateMining calls
		//      DriveToPoint again later in this very poll and the band no longer
		//      suppresses it.
		bHasIssuedPointGoal = false;

		//   2. THE LIVE PATH REQUEST ITSELF — EnsureWalkingToNode's goal check
		//      (:917-924) returns early while a move toward THIS node is in flight,
		//      and a wedged miner's move IS in flight: that is what makes bAdvancing
		//      true, which is what let the ladder climb this far in the first place.
		//      ⇒ Without this StopMovement the mining half of the rung is a no-op.
		//      ⛔ It is NOT a second driver: it is the one driver cancelling its own
		//      request, and it is what guarantees the re-issue below is ONE request
		//      rather than two concurrent ones (NAV-§3's rung-2 prohibition).
		AGoldNode* const Node = TargetGoldNode.Get();
		if (AAIController* const AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}

		// Re-issue the SAME node walk, once. ⛔ Null-guarded: under Defend/Follow
		// LeaveMining() has already nulled the target, and EnsureWalkingToNode would
		// hand MoveToActor a null goal — Failed, plus a misleading "is the
		// NavMeshBoundsVolume covering L_Arena" warning latched for the match.
		if (Node)
		{
			EnsureWalkingToNode(Node);
		}
		break;
	}

	case ESiegeStuckAction::Abandon:
	{
		// Both earlier rungs are spent; neither may hold this one back (same ordering
		// argument as WidenAndRepath above).
		SidestepLeaseRemaining = 0.f;

		// Stop the walk and drop the point-goal latch so whatever runs next re-paths
		// from scratch. Idempotent and null-safe. ⛔ This rung issues NO path request
		// of its own, exactly as NAV-§3 requires of Abandon.
		StandInPlace();

		// ⛔⛔ THE GUARD THAT KEEPS THIS RUNG OUT OF THE INCOME PATH — a miner holding
		// a TENURE never re-seeks. SeekBestMine overwrites TargetGoldNode, and a null
		// finder (every mine depleted) would make the next poll read
		// `bArrivedAtNode && bTargetDead` -> LeaveMining() -> RemoveMinerIncome: this
		// rung would END A TENURE and change the gold rate, which is squarely outside
		// this task (CONVENTIONS §5 — the command layer only chooses WHERE the miner
		// is steered).
		// ⭐ AND IT COSTS NOTHING, because the case is all but unreachable: arrival
		// calls StopMovement, so an ARRIVED miner reads GetMoveStatus() == Idle ->
		// bAdvancing false -> Evaluate re-anchors and returns None however long it
		// stands there. The ONLY path that can reach a rung with a tenure held is a
		// DISPLACED arrived miner walking back, and this guard is what makes that case
		// provably safe instead of merely argued safe.
		if (!bArrivedAtNode)
		{
			// Six seconds of not moving while genuinely trying to: re-consult the
			// finder from where the miner stands NOW. It may well answer with the same
			// node — mines do not move — and that is ACCEPTABLE: Evaluate has already
			// Reset the ladder, so a second Abandon has to earn the whole 6 s climb
			// again. What DOES change is that the next poll issues a path request from
			// a fresh start poly against the navmesh as it stands now, instead of the
			// byte-identical re-request NAV-§1 Cause 2 names as the livelock.
			//
			// ⚠️ Under Hold/Ambush this can briefly target a mine OUTSIDE the position
			// circle, because SeekBestMine is the map-wide finder. The retarget gate's
			// bOutOfOrderDisc test catches it on the very next poll and re-seeks with
			// SeekMineInDisc, so it self-heals in 0.25 s with no bookkeeping involved
			// (bArrivedAtNode is false here, so that gate's LeaveMining branch cannot
			// run). Choosing between the two finders here would mean a second
			// ResolveMinerOrder on the same poll for a case that corrects itself.
			SeekBestMine();
		}
		break;
	}

	case ESiegeStuckAction::None:
	default:
		// TickStuckWatchdog never dispatches None; enumerated so the switch is total
		// and a rung added to the enum later cannot silently fall through here.
		break;
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

AGoldNode* AMinerUnit::SeekBestMine()
{
	// THE single finder (TASK-253, shared with the bot): tier-1 nearest (2D)
	// mine this team can mine NOW, tier-2 nearest enemy-occupied non-depleted
	// mine (a wait target), null = everything depleted or none exist. The
	// old FindNearestSameTeamGoldNode died here (TASK-254): the team filter
	// is gone — occupancy replaces it.
	AGoldNode* Best = AGoldNode::FindBestMineFor(GetWorld(), Team, GetActorLocation());
	TargetGoldNode = Best;

	if (Best)
	{
		// re-arm the endgame log (defensive: depletion is one-way, so a null
		// finder normally never turns non-null again within one match)
		bLoggedNoMineAvailable = false;
	}
	else if (!bLoggedNoMineAvailable)
	{
		// the all-depleted endgame — the INTENDED income death (plan-of-
		// record). Log, not Error/Warning (demoted per the plan: this is a
		// normal state now); the miner idles in place, the poll re-seeks.
		bLoggedNoMineAvailable = true;
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("AMinerUnit '%s': no minable or waitable mine (all depleted, or none placed) — idling in place; the poll keeps re-seeking."),
			*GetNameSafe(this));
	}

	return Best;
}

AGoldNode* AMinerUnit::SeekMineInDisc(const FMinerOrder& Order)
{
	// SeekBestMine's in-circle twin (Hold/Ambush). Same shape on purpose — the two
	// are read side by side — but SeekBestMine is left BYTE-IDENTICAL for the
	// Attack/spawn-default path (CONVENTIONS §5's cheapest-regression-proof clause),
	// so this is a sibling rather than a parameterisation.
	AGoldNode* const Best = AGoldNode::FindBestMineInDisc(
		GetWorld(), Team, Order.Point, Order.Radius, GetActorLocation());
	TargetGoldNode = Best;

	if (Best)
	{
		bLoggedNoMineInDisc = false; // re-arm: a mine is available in the circle again
	}
	else if (!bLoggedNoMineInDisc)
	{
		// ⚠️ A DIFFERENT NULL FROM SeekBestMine'S. That one means the all-depleted
		// ENDGAME; this one means "the circle you drew has no mine in it", which is
		// an ordinary, expected outcome of a Hold order placed on empty ground. The
		// miner stations inside the circle (ladder rung 2) and keeps re-seeking, so
		// a mine freed by an enemy occupant leaving is picked up with no new order.
		bLoggedNoMineInDisc = true;
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("AMinerUnit '%s': no minable or waitable mine inside the ordered position circle (centre %s, radius %.0f) — stationing inside it and re-seeking (TASK-398)."),
			*GetNameSafe(this), *Order.Point.ToCompactString(), Order.Radius);
	}

	return Best;
}

void AMinerUnit::EndMineTenure()
{
	// Un-arrive (idempotent — every flag write below is a no-op the second
	// time): shared by the NotifyMineDepleted eviction seam and the poll's
	// defensive stale-mine path. NEVER touches the mine registry — callers
	// own that side (Deplete() already emptied it; EndPlay unregisters
	// explicitly; the stale path has no mine left to talk to).
	if (bIncomeActive)
	{
		if (ASiegePlayerState* OwnerState = CachedOwnerState.Get())
		{
			// the SAME state we registered with (TASK-024 contract). Income ⊆
			// alive holds at every step: the alive count is untouched here —
			// only death (EndPlay) unregisters it.
			OwnerState->RemoveMinerIncome();
		}
		// else: the owning state vanished — its counters died with it. The
		// flag still clears so death bookkeeping can never double-Remove;
		// EndPlay owns the vanished-state warning (no duplicate here).
		bIncomeActive = false;
	}

	// per-tenure latch OFF (TASK-254): the next registered arrival re-latches
	// and re-adds income — the M2 one-way-per-lifetime semantics are gone
	// (header doc)
	bArrivedAtNode = false;

	// §6 (TASK-179): an evicted miner mines no more — clink off until the
	// next tenure's arrival restarts it
	StopMiningClink();

	// whatever queue comes next is a new wait episode
	bLoggedWaitingAtMine = false;
}

void AMinerUnit::NotifyMineDepleted(AGoldNode* DepletedMine)
{
	// TASK-253's PINNED call-site contract (GoldNode.cpp Deplete()): when
	// this fires, the mine has ALREADY latched bDepleted, emptied its
	// registry and released its claim — so no UnregisterArrivedMiner call
	// here (it would be a no-op), and the re-seek below can never re-pick
	// this mine (FindBestMineFor skips depleted mines). Fired at most once
	// per miner per depletion, only for still-valid miners.
	//
	// Runs on FROZEN miners too (the accepted post-match drain quirk: an
	// occupied mine can deplete after the match-end freeze while frozen
	// miners stand at its ring). The books stay balanced — EndPlay's death
	// bookkeeping sees bIncomeActive == false afterward, so the PlayAgain
	// sweep never double-Removes — and no movement follows: FreezeAI cleared
	// the poll, so the re-seek never fires for a frozen miner.
	EndMineTenure();

	// Drop the dead target. It IS DepletedMine in every reachable flow (a
	// miner only ever registers at TargetGoldNode, and an ARRIVED miner never
	// retargets); nulled unconditionally so even a hypothetical mismatch
	// forces a clean re-seek. The 0.25 s poll re-seeks via SeekBestMine —
	// walk the next mine, queue at an enemy-claimed one, or idle out the
	// all-depleted endgame.
	TargetGoldNode = nullptr;

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("AMinerUnit '%s': evicted from depleted mine '%s' — re-seeking on the next poll."),
		*GetNameSafe(this), *GetNameSafe(DepletedMine));
}
