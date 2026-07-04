// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Siegebound/TeamId.h"
#include "SiegeGameState.generated.h"

class ASiegePlayerState;

/**
 *  Broadcast exactly once per match when the clock crosses OvertimeStartSeconds
 *  (GDD §3.2, 7:00): base income doubles for BOTH players (miner bonuses
 *  unchanged) and the HUD shows the overtime indicator (TASK-033). The latch is
 *  cleared — and this delegate re-armed — only by ResetClock() (Play Again).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOvertimeStarted);

/**
 *  Broadcast each time the match clock enters a new whole second, carrying the
 *  new whole-second value (floor of MatchClockSeconds). A long frame hitch
 *  broadcasts only the newest second — listeners display the latest value,
 *  never a stale count-up. Also fired with 0 on ResetClock (reset-path
 *  broadcast, CONVENTIONS delegate law). UI consumers seed from
 *  GetMatchClockSeconds() first, THEN bind (seed-then-bind law).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchClockChanged, int32, WholeSeconds);

/**
 *  Siegebound game state (GDD §3.2, TASK-024) — owns the match clock and the
 *  overtime latch. Installed by ASiegeGameMode (GameStateClass), so exactly one
 *  exists per match and every ASiegePlayerState composes its gold rate against
 *  the same clock (§3.2 "for both players").
 *
 *  - MatchClockSeconds ticks up from 0 from match start. Tick-driven (true
 *    DeltaSeconds accumulation), not a 1 s timer — no drift against real
 *    elapsed time, nothing for a timer sweep to kill.
 *  - At OvertimeStartSeconds (420 = 7:00 // GDD §3.2) bOvertimeActive latches
 *    and OnOvertimeStarted broadcasts EXACTLY once. ASiegePlayerState reads
 *    IsOvertimeActive() LIVE in GetGoldRate(), so accrual can never desync
 *    from the clock even if a broadcast was missed.
 *  - StopClock() freezes the clock at the final time (match-end freeze, §3.9,
 *    called by ASiegeGameMode); ResetClock() returns to 0 with the overtime
 *    latch cleared and the clock running again (Play Again — §3.9 "full state
 *    reset: ... match clock").
 *
 *  Local-only through M7 (no replicated members added); M8 multiplayer must
 *  revisit clock/overtime replication.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	ASiegeGameState();

	/** Fired exactly once per match at the §3.2 overtime threshold. Re-armed only by ResetClock(). HUD overtime indicator binds here (TASK-033), seeded from IsOvertimeActive() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Match")
	FOnOvertimeStarted OnOvertimeStarted;

	/** Fired once per elapsed whole second with the new value (and with 0 on ResetClock). Seed clock displays from GetMatchClockSeconds() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Match")
	FOnMatchClockChanged OnMatchClockChanged;

	/** Match-end freeze (§3.9, TASK-024): freezes the clock at the final time. Idempotent; only ResetClock() restarts it. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void StopClock();

	/**
	 *  Play Again (§3.9): clock back to 0, overtime latch cleared (the §3.2
	 *  doubling and its one-shot broadcast re-arm for the new match), clock
	 *  running again. Broadcasts OnMatchClockChanged(0) unconditionally
	 *  (reset-path broadcast, CONVENTIONS delegate law) so displays snap back
	 *  to 0:00. The game mode calls this BEFORE the player states'
	 *  ResetEconomy(), which re-derives the gold rate against this latch.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void ResetClock();

	/** Seconds since match start (frozen at the final time while stopped). HUD seeds its clock from this, then binds OnMatchClockChanged. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	float GetMatchClockSeconds() const { return MatchClockSeconds; }

	/** True from the §3.2 overtime threshold until ResetClock(). ASiegePlayerState::GetGoldRate() reads this live to double the base income. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	bool IsOvertimeActive() const { return bOvertimeActive; }

	/**
	 *  Multi-team economy accessor (TASK-043): returns the ASiegePlayerState
	 *  whose Team tag matches, iterating this game state's PlayerArray. In a
	 *  single-player-state world (M2) it returns the one Blue player state for
	 *  Team==Blue and nullptr for Red — identical to the pre-M3 assumption of a
	 *  single economy — while the M3 bot's Red player state makes both teams
	 *  resolve. Consumers (AMinerUnit, and later the bot) bind income to the
	 *  right side through this instead of grabbing the first player state.
	 *  Returns nullptr and logs when no player state carries the requested team.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	ASiegePlayerState* GetPlayerStateForTeam(ETeamId Team) const;

protected:

	/** Advances the clock while it runs: broadcasts new whole seconds, latches overtime at the threshold. */
	virtual void Tick(float DeltaSeconds) override;

	/** Match time in seconds at which overtime begins and base income doubles. // GDD §3.2 — 7:00 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Match", meta = (ClampMin = "0"))
	float OvertimeStartSeconds = 420.f;

private:

	/** Seconds since match start; accumulates DeltaSeconds while bClockRunning (§3.2). Read via GetMatchClockSeconds(). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Match", meta = (AllowPrivateAccess = "true"))
	float MatchClockSeconds = 0.f;

	/** Latched at the overtime threshold — guards the exactly-once OnOvertimeStarted broadcast. Cleared only by ResetClock(). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Match", meta = (AllowPrivateAccess = "true"))
	bool bOvertimeActive = false;

	/** True while the clock advances: from spawn until StopClock() (match end), and again after ResetClock() (Play Again). */
	bool bClockRunning = true;

	/** Last whole-second value broadcast through OnMatchClockChanged (change detection; the clock only moves forward between resets). */
	int32 LastBroadcastWholeSeconds = 0;
};
