// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeGameState.h"

#include "GitClaudeUnrealTest.h"

ASiegeGameState::ASiegeGameState()
{
	// The match clock is tick-driven (GDD §3.2): accumulating true DeltaSeconds
	// never drifts against real elapsed time the way a repeating 1 s timer
	// would, and there is no timer handle for any cleanup sweep to kill.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASiegeGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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

		OnOvertimeStarted.Broadcast();
	}
}

void ASiegeGameState::StopClock()
{
	bClockRunning = false;
}

void ASiegeGameState::ResetClock()
{
	MatchClockSeconds = 0.f;
	LastBroadcastWholeSeconds = 0;
	bOvertimeActive = false;
	bClockRunning = true;

	// Reset-path broadcast (CONVENTIONS delegate law): displays snap back to
	// 0:00 immediately instead of waiting for the first elapsed second.
	OnMatchClockChanged.Broadcast(0);
}
