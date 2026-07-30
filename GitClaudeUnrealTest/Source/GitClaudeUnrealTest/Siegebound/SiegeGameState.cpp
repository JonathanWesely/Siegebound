// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeGameState.h"

#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GitClaudeUnrealTest.h"
#include "Net/UnrealNetwork.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8: declared in the session subsystem, usable by any M8 code)

namespace
{
	/** §6 overtime sting (TASK-179) — a 2D one-shot fired once at 7:00; null-safe until S_OvertimeSting lands (TASK-180). M8: fired once per MACHINE (authority latch / client OnRep edge). */
	const TCHAR* OvertimeStingSoundPath = TEXT("/Game/Audio/S_OvertimeSting");
}

ASiegeGameState::ASiegeGameState()
{
	// The match clock is tick-driven (GDD §3.2): accumulating true DeltaSeconds
	// never drifts against real elapsed time the way a repeating 1 s timer
	// would, and there is no timer handle for any cleanup sweep to kill.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	// AGameStateBase already replicates (engine default); no net tuning needed in
	// P1 — the basis-triple design is event-driven (~3 writes/match, doc D8/D13).
}

void ASiegeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The M8 P1 match-flow set (TASK-356, doc §3.3). All plain conditions: tiny,
	// event-driven state every client needs. Pairs that must apply atomically
	// (base seconds + base server time; ended + winner) ride the same actor
	// property bunch — the engine applies a bunch's properties before OnReps fire.
	DOREPLIFETIME(ASiegeGameState, ClockBaseSeconds);
	DOREPLIFETIME(ASiegeGameState, ClockBaseServerTime);
	DOREPLIFETIME(ASiegeGameState, bClockRunning);
	DOREPLIFETIME(ASiegeGameState, bOvertimeActive);
	DOREPLIFETIME(ASiegeGameState, bMatchEnded);
	DOREPLIFETIME(ASiegeGameState, WinningTeam);
}

void ASiegeGameState::BeginPlay()
{
	Super::BeginPlay();

	// Clock-basis write 1 of ~3 (doc D8): the match clock starts running at world
	// begin — publish the (0, now, running) basis so a client joining the running
	// match derives the correct elapsed time from its first frame. Standalone:
	// two float writes nothing reads — a provable no-op (doc §10).
	if (HasAuthority())
	{
		PublishClockBasis();
	}
}

void ASiegeGameState::PublishClockBasis()
{
	// AUTHORITY-only writer (all callers are authority-gated; belt here anyway).
	if (!HasAuthority())
	{
		return;
	}

	ClockBaseSeconds = MatchClockSeconds;
	// AGameStateBase::GetServerWorldTimeSeconds — the engine's client-synced
	// server clock (double); on the server it IS world real time, so the pair
	// (value at moment, moment) lets any client derive elapsed = base +
	// (nowSynced - at). Explicit float narrowing: match-length horizons keep
	// well within float precision for the DELTA math the clients run.
	ClockBaseServerTime = static_cast<float>(GetServerWorldTimeSeconds());
}

void ASiegeGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// ── M8 non-authority fork (TASK-356, doc §3.3 — kills the audit-§3.2 client
	// clock/overtime fork): a CLIENT copy never accumulates, never latches
	// overtime, never plays the sting. It derives the DISPLAY value from the
	// replicated basis triple and reuses the whole-second change detection so
	// the existing HUD delegate fires exactly as on the host. Standalone is
	// authority ⇒ this branch is unreachable (byte-identity, doc §10).
	if (!HasAuthority())
	{
		const float Derived = bClockRunning
			? ClockBaseSeconds + static_cast<float>(GetServerWorldTimeSeconds() - ClockBaseServerTime)
			: ClockBaseSeconds;
		const int32 WholeSeconds = FMath::FloorToInt32(FMath::Max(Derived, 0.f));
		if (WholeSeconds != LastBroadcastWholeSeconds)
		{
			LastBroadcastWholeSeconds = WholeSeconds;
			OnMatchClockChanged.Broadcast(WholeSeconds);
		}
		return;
	}

	// Frozen by StopClock() at match end (§3.9) until ResetClock() (Play Again).
	if (!bClockRunning)
	{
		return;
	}

	MatchClockSeconds += DeltaSeconds;

	// One broadcast per NEW whole second. A long hitch that skips several
	// seconds broadcasts only the newest value — listeners display the latest
	// clock, never replay a stale count-up.
	const int32 WholeSeconds = FMath::FloorToInt32(MatchClockSeconds);
	if (WholeSeconds != LastBroadcastWholeSeconds)
	{
		LastBroadcastWholeSeconds = WholeSeconds;
		OnMatchClockChanged.Broadcast(WholeSeconds);
	}

	// Overtime latch (GDD §3.2, 7:00): base income doubles for both players.
	// Latch first, then broadcast — exactly once per match; only ResetClock()
	// re-arms it. ASiegePlayerState reads the latch LIVE for accrual and binds
	// this broadcast purely to notify its rate listeners.
	if (!bOvertimeActive && MatchClockSeconds >= OvertimeStartSeconds)
	{
		bOvertimeActive = true;

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Overtime started at %.1f s (threshold %.1f s, GDD §3.2) — base income doubles."),
			*GetNameSafe(this), MatchClockSeconds, OvertimeStartSeconds);

		// §6 overtime sting (TASK-179): fires exactly once at 7:00 (this branch is latched). 2D, null-safe.
		USiegeFeedbackLibrary::PlaySound2D(this, OvertimeStingSoundPath);

		OnOvertimeStarted.Broadcast();
	}
}

void ASiegeGameState::StopClock()
{
	// M8 authority belt (TASK-356): the clock is server state; the GameMode (its
	// only caller) is server-only anyway. Standalone: authority ⇒ unchanged.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Warning, TEXT("[%s] StopClock refused on a non-authority copy (server drives the clock, doc D8)."), *GetNameSafe(this));
		return;
	}

	bClockRunning = false;

	// Clock-basis write 2 of ~3 (doc D8): freeze clients at the same final time.
	PublishClockBasis();
}

void ASiegeGameState::ResetClock()
{
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Warning, TEXT("[%s] ResetClock refused on a non-authority copy (server drives the clock, doc D8)."), *GetNameSafe(this));
		return;
	}

	MatchClockSeconds = 0.f;
	LastBroadcastWholeSeconds = 0;
	bOvertimeActive = false;
	bClockRunning = true;

	// Clock-basis write 3 of ~3 (doc D8): clients snap to 0 via OnRep_ClockBase.
	PublishClockBasis();

	// Reset-path broadcast (CONVENTIONS delegate law): displays snap back to
	// 0:00 immediately instead of waiting for the first elapsed second.
	OnMatchClockChanged.Broadcast(0);
}

void ASiegeGameState::OnRep_ClockBase()
{
	// CLIENT snap (doc §3.3): a fresh basis means a state change (join, stop,
	// reset) — recompute the derived display second NOW and broadcast so the HUD
	// never waits up to a second (and the Play-Again reset lands 0 immediately).
	// ClockBaseServerTime arrived in the same bunch (applied before this OnRep).
	const float Derived = bClockRunning
		? ClockBaseSeconds + static_cast<float>(GetServerWorldTimeSeconds() - ClockBaseServerTime)
		: ClockBaseSeconds;
	const int32 WholeSeconds = FMath::FloorToInt32(FMath::Max(Derived, 0.f));
	LastBroadcastWholeSeconds = WholeSeconds;
	OnMatchClockChanged.Broadcast(WholeSeconds);
}

void ASiegeGameState::OnRep_OvertimeActive()
{
	// CLIENT edge (doc §3.3): OnReps fire only on a CHANGE, so value==true IS the
	// false→true 7:00 edge — sting + delegate, once per machine (TASK-357 gate d).
	// The true→false edge (Play-Again reset) is silent, matching ResetClock's
	// re-arm semantic (the reset displays converge via OnRep_ClockBase + the
	// match-reset notify).
	if (bOvertimeActive)
	{
		USiegeFeedbackLibrary::PlaySound2D(this, OvertimeStingSoundPath);
		OnOvertimeStarted.Broadcast();
	}
}

void ASiegeGameState::SetMatchResult(ETeamId Winner)
{
	// AUTHORITY write (doc §3.3): double-end calls are already latched by the
	// GameMode, but this state is its own guard too (defensive).
	if (!HasAuthority() || bMatchEnded)
	{
		return;
	}

	WinningTeam = Winner;
	bMatchEnded = true;

	// The HOST's screen (and standalone's — the same single local PC the retired
	// GameMode direct push resolved, doc §10): server-side copies never get
	// OnReps, so the local fan-out runs here.
	NotifyLocalControllersMatchEnd();
}

void ASiegeGameState::ClearMatchResult()
{
	if (!HasAuthority() || !bMatchEnded)
	{
		return;
	}

	bMatchEnded = false;
	// WinningTeam is left as-is (meaningful only while bMatchEnded). No server-side
	// notify: ASiegeGameMode::PlayAgain step 6 already walks the server-side
	// controllers (doc §3.4.2); the CLIENT's local reset rides OnRep_MatchEnded.
}

void ASiegeGameState::OnRep_MatchEnded()
{
	// CLIENT match-flow edge (doc §3.4): WinningTeam rode the same bunch.
	if (bMatchEnded)
	{
		NotifyLocalControllersMatchEnd();
	}
	else
	{
		NotifyLocalControllersMatchReset();
	}
}

void ASiegeGameState::NotifyLocalControllersMatchEnd()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Ban-compliant fan-out (doc §3.3): iterate ALL player controllers, act on the
	// LOCAL ASiegePlayerControllers only — at most ONE is local on any machine
	// (no splitscreen), and in standalone that one is exactly the controller the
	// retired direct push notified (byte-identity, doc §10).
	bool bNotifiedAnyController = false;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(It->Get());
		if (SiegePC && SiegePC->IsLocalController())
		{
			SiegePC->HandleMatchEnd(WinningTeam);
			bNotifiedAnyController = true;
		}
	}

	if (!bNotifiedAnyController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Match result latched but no LOCAL ASiegePlayerController was found to show the end screen."), *GetNameSafe(this));
	}
}

void ASiegeGameState::NotifyLocalControllersMatchReset()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// The client-local mirror of PlayAgain step 6 (doc §3.4.2): the server-side
	// controller walk cannot reach a REMOTE machine's local PC — this OnRep-driven
	// fan-out is how the client's end screen drops and its local deck resets.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(It->Get());
		if (SiegePC && SiegePC->IsLocalController())
		{
			SiegePC->PerformLocalMatchReset();
		}
	}
}

float ASiegeGameState::GetMatchClockSeconds() const
{
	// AUTHORITY: the accumulated value, exactly as through M7. CLIENT: derive from
	// the replicated basis (doc D8) — correct elapsed time on late join by
	// construction, frozen at the final time while stopped.
	if (HasAuthority())
	{
		return MatchClockSeconds;
	}

	const float Derived = bClockRunning
		? ClockBaseSeconds + static_cast<float>(GetServerWorldTimeSeconds() - ClockBaseServerTime)
		: ClockBaseSeconds;
	return FMath::Max(Derived, 0.f);
}

ASiegePlayerState* ASiegeGameState::GetPlayerStateForTeam(ETeamId Team) const
{
	// Iterate PlayerArray (TASK-043 multi-team economy): the Blue player state
	// and — once the M3 bot spawns (TASK-045) — the Red bot state both live
	// here. Return the first whose Team tag matches. The loop var is named
	// IterPlayerState, NOT PlayerState: AGameStateBase has no reflected
	// PlayerState member, but the CONVENTIONS no-shadow rule is applied anyway.
	// M8 (TASK-356, audit §9 flag 2 — accepted): a defensive one-shot warn when
	// MORE than one PS claims the requested team (a seat-latch/bot-gate bug would
	// make this first-match resolve order-dependent and silent). Diagnostic only —
	// the first match is still returned, exactly as before.
	ASiegePlayerState* FirstMatch = nullptr;
	int32 MatchCount = 0;
	for (APlayerState* IterPlayerState : PlayerArray)
	{
		if (ASiegePlayerState* SiegePS = Cast<ASiegePlayerState>(IterPlayerState))
		{
			if (SiegePS->GetTeam() == Team)
			{
				if (!FirstMatch)
				{
					FirstMatch = SiegePS;
				}
				++MatchCount;
			}
		}
	}

	if (MatchCount > 1 && !bWarnedDuplicateTeamPS)
	{
		bWarnedDuplicateTeamPS = true;
		UE_LOG(LogSiegeNet, Warning,
			TEXT("[%s] GetPlayerStateForTeam(%s): %d PlayerStates claim that team — the resolve is order-dependent (seat latch / SpawnBot gate bug? doc §2). Warned once."),
			*GetNameSafe(this), Team == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"), MatchCount);
	}

	if (FirstMatch)
	{
		return FirstMatch;
	}

	// None carries this team. For Blue this only happens before the player
	// state exists; for Red it is the normal M2 result (no bot) and the brief
	// pre-spawn window in M3 — callers that poll re-resolve. Logged so a
	// genuinely mis-teamed consumer (a miner for a team with no economy) is
	// visible rather than silently untracked.
	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("[%s] GetPlayerStateForTeam(%s): no ASiegePlayerState carries that team (normal for Red until the M3 bot spawns, TASK-045)."),
		*GetNameSafe(this), Team == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));

	return nullptr;
}
