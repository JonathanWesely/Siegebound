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
 *  M8 P1 (TASK-356, per the SIGNED TASK-353 doc D7/D8 + §3.3): this class is
 *  now the ONE replication surface for match flow.
 *  - CLOCK = a replicated BASIS TRIPLE (ClockBaseSeconds / ClockBaseServerTime /
 *    bClockRunning) against AGameStateBase::GetServerWorldTimeSeconds() — the
 *    engine's synced server clock — written ~3×/match (BeginPlay start,
 *    StopClock, ResetClock) by PublishClockBasis(). NEVER a per-frame replicated
 *    float. Non-authority Tick is DISPLAY-ONLY: it derives the elapsed value
 *    from the basis and broadcasts OnMatchClockChanged on whole-second changes;
 *    it never accumulates, never latches overtime, never plays the sting (the
 *    audit §3.2 client-clock fork is dead).
 *  - OVERTIME reaches clients via OnRep_OvertimeActive (true edge → sting +
 *    OnOvertimeStarted, once per machine — TASK-357 gate d).
 *  - MATCH RESULT (new state — match end previously lived only in the GameMode's
 *    direct controller push): bMatchEnded + WinningTeam replicate; the OnRep
 *    fans out to the LOCAL controller(s) only (NotifyLocalControllers* — at most
 *    one local PC per machine; ban-compliant iteration). The server-side notify
 *    in SetMatchResult covers the HOST's screen; standalone resolves the same
 *    single local PC the old direct loop did (byte-identity, doc §10).
 *  Standalone: HasAuthority() is true everywhere, nothing replicates, no OnRep
 *  fires — the M7 behavior byte-for-byte.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	ASiegeGameState();

	/**
	 *  ⚖️ NET RELEVANCY TIER: **A — inherited from the engine, VERIFIED not set
	 *  here** (CONVENTIONS NET RELEVANCY LAW declaration duty; the law's explicit
	 *  "verify and document, do NOT blind-set" clause). Evidence, read from the
	 *  installed UE 5.8 source this pass: `AGameStateBase::AGameStateBase` sets
	 *  `bReplicates = true` AND `bAlwaysRelevant = true`
	 *  (Engine/Private/GameStateBase.cpp:25-26). Corroborated empirically by
	 *  TASK-357: the match-end/clock state crossed the 500 m arena correctly in
	 *  the very run where the castle and scatter (both engine-default relevancy)
	 *  did not. Adding a redundant assignment here would only hide that fact.
	 */

	/** Registers the M8 P1 replicated set (clock basis triple, overtime latch, match result — doc §3.3). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 *  AUTHORITY match-end state write (TASK-356, doc §3.3/§3.4.1): latches
	 *  bMatchEnded + WinningTeam (replicated to clients, whose OnRep_MatchEnded
	 *  shows their end screen) and notifies the LOCAL controller(s) on the server
	 *  side (the host's screen; in standalone = THE player, same observable call
	 *  as the retired GameMode direct push). Guarded: non-authority and double
	 *  calls are no-ops. ASiegeGameMode::OnCastleDestroyedHandler is the caller.
	 */
	void SetMatchResult(ETeamId Winner);

	/**
	 *  AUTHORITY match-reset state write (Play Again, doc §3.4.2): clears
	 *  bMatchEnded (the false-edge OnRep drives each CLIENT's local reset via
	 *  PerformLocalMatchReset). No server-side notify here — GameMode's PlayAgain
	 *  step 6 already walks the server-side controllers. Guarded like SetMatchResult.
	 */
	void ClearMatchResult();

	/** True from SetMatchResult until ClearMatchResult (replicated). The winner is GetWinningTeam(). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	bool HasMatchResult() const { return bMatchEnded; }

	/** The winning team of the latched match result (meaningful only while HasMatchResult()). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	ETeamId GetWinningTeam() const { return WinningTeam; }

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

	/**
	 *  Seconds since match start (frozen at the final time while stopped). HUD
	 *  seeds its clock from this, then binds OnMatchClockChanged. M8 (TASK-356):
	 *  out-of-line — the authority returns its accumulated MatchClockSeconds
	 *  exactly as before; a CLIENT derives the value from the replicated basis
	 *  triple (doc D8), so a late join reads the correct elapsed time by
	 *  construction.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	float GetMatchClockSeconds() const;

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

	/** Publishes the initial clock basis on the authority (write 1 of ~3 — doc D8). */
	virtual void BeginPlay() override;

	/** AUTHORITY: advances the clock, broadcasts new whole seconds, latches overtime. NON-AUTHORITY (M8): display-only — derives the basis-triple value and broadcasts whole seconds; never accumulates/latches (doc §3.3). */
	virtual void Tick(float DeltaSeconds) override;

	/** Match time in seconds at which overtime begins and base income doubles. // GDD §3.2 — 7:00 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Match", meta = (ClampMin = "0"))
	float OvertimeStartSeconds = 420.f;

private:

	/**
	 *  CLIENT clock snap (doc §3.3): recompute the derived display second from the
	 *  fresh basis and broadcast OnMatchClockChanged immediately — snaps displays
	 *  right after join and on reset (the reset basis carries 0). ClockBaseServerTime
	 *  rides the same actor property bunch, applied before OnReps fire (atomic pair).
	 */
	UFUNCTION()
	void OnRep_ClockBase();

	/**
	 *  CLIENT overtime edge (doc §3.3): value flipped TRUE → play the §6 sting +
	 *  broadcast OnOvertimeStarted (once per machine — OnReps only fire on change;
	 *  the HUD indicator and the PS rate re-derive hang off the existing delegate).
	 *  Value flipped FALSE (Play-Again reset) → silent, matching ResetClock's
	 *  re-arm semantic; displays converge via OnRep_ClockBase's 0-broadcast and
	 *  the match-reset notify.
	 */
	UFUNCTION()
	void OnRep_OvertimeActive();

	/** CLIENT match-flow edge (doc §3.4): true → notify local controller(s) HandleMatchEnd(WinningTeam); false → PerformLocalMatchReset. WinningTeam rides the same bunch (atomic pair). */
	UFUNCTION()
	void OnRep_MatchEnded();

	/** AUTHORITY: stamps the replicated basis triple from the live clock state (called at exactly the ~3 state changes — BeginPlay start, StopClock, ResetClock). */
	void PublishClockBasis();

	/** Fans HandleMatchEnd(WinningTeam) out to the LOCAL ASiegePlayerController(s) only — at most one per machine; GetPlayerControllerIterator + IsLocalController (ban-compliant, doc §3.3). Warns when none was notified (parity with the retired direct push). */
	void NotifyLocalControllersMatchEnd();

	/** Mirror of NotifyLocalControllersMatchEnd for the reset edge → PerformLocalMatchReset() (the client-local half of PlayAgain step 6). */
	void NotifyLocalControllersMatchReset();

	/** Seconds since match start; accumulates DeltaSeconds while bClockRunning (§3.2). AUTHORITY-only state (clients derive from the basis). Read via GetMatchClockSeconds(). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Match", meta = (AllowPrivateAccess = "true"))
	float MatchClockSeconds = 0.f;

	/** Latched at the overtime threshold — guards the exactly-once OnOvertimeStarted broadcast. Cleared only by ResetClock(). M8: replicated so the client HUD/sting fire at 7:00 (OnRep_OvertimeActive). */
	UPROPERTY(ReplicatedUsing = OnRep_OvertimeActive, VisibleInstanceOnly, Transient, Category = "Siegebound|Match", meta = (AllowPrivateAccess = "true"))
	bool bOvertimeActive = false;

	/** True while the clock advances: from spawn until StopClock() (match end), and again after ResetClock() (Play Again). M8: replicated (part of the clock basis — clients freeze/run their derived display with it). */
	UPROPERTY(Replicated)
	bool bClockRunning = true;

	/** Accumulated clock value at the last authority state change (doc D8 basis triple, 1/3). */
	UPROPERTY(ReplicatedUsing = OnRep_ClockBase)
	float ClockBaseSeconds = 0.f;

	/** GetServerWorldTimeSeconds() at that moment (basis triple, 2/3 — same bunch as ClockBaseSeconds, applied atomically before the OnRep). */
	UPROPERTY(Replicated)
	float ClockBaseServerTime = 0.f;

	/** M8 match result (doc §3.3): true from SetMatchResult (first castle destruction) until ClearMatchResult (Play Again). The OnRep drives each client's end screen / reset. */
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, VisibleInstanceOnly, Transient, Category = "Siegebound|Match", meta = (AllowPrivateAccess = "true"))
	bool bMatchEnded = false;

	/** The latched winner (meaningful while bMatchEnded; same bunch as it — atomic pair). */
	UPROPERTY(Replicated)
	ETeamId WinningTeam = ETeamId::Blue;

	/** Last whole-second value broadcast through OnMatchClockChanged (change detection; per-machine display state — never replicated). */
	int32 LastBroadcastWholeSeconds = 0;

	/** One-shot latch for the duplicate-team-PS defensive warn (audit §9 flag 2 — accepted; dev diagnostic, not flow control). */
	mutable bool bWarnedDuplicateTeamPS = false;
};
