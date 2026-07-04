// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Siegebound/TeamId.h"
#include "SiegePlayerState.generated.h"

class ASiegeGameState;

/**
 *  Broadcast whenever the player's gold value actually changes
 *  (passive income tick, spend, reset). NewGold is the post-change,
 *  clamped value — always in [0, MaxGold].
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

/**
 *  Broadcast whenever the composed gold rate actually changes (overtime
 *  doubling at 7:00, a miner arriving at its node, an arrived miner dying)
 *  and unconditionally on ResetEconomy (reset-path broadcast, CONVENTIONS
 *  delegate law). NewRate is the gold added per income tick — with the
 *  default 1.0 s tick, gold per second (the HUD's "+N/s" text, TASK-033).
 *  UI consumers seed from GetGoldRate() first, THEN bind (seed-then-bind law).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldRateChanged, int32, NewRate);

/**
 *  Broadcast whenever the number of ALIVE miners actually changes (register
 *  on spawn, unregister on death) and unconditionally on ResetEconomy.
 *  AliveCount counts miners registered via RegisterMinerAlive — en-route AND
 *  arrived alike (the §3.3 cap counts ALIVE miners, M2 ruling). The HUD's
 *  "x/6" counter binds here (TASK-033), seeded from GetAliveMinerCount() first.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMinerCountChanged, int32, AliveCount);

/**
 *  Siegebound player state — owns the gold economy (GDD §3.2/§3.3, M2 scope,
 *  TASK-005 base + TASK-024 rate composition).
 *
 *  - Gold starts at 50; every income tick (1.0 s) adds the COMPOSED rate:
 *    base GoldPerTick (2/s), doubled while the shared ASiegeGameState overtime
 *    latch is active (§3.2, 7:00 — read LIVE, so accrual can never desync from
 *    the clock), plus MinerGoldPerTick (1/s) per ARRIVED miner (§3.3).
 *  - Miner bookkeeping is two separate counts (TASK-025 drives both): ALIVE
 *    miners (RegisterMinerAlive/UnregisterMinerAlive — the MaxActiveMiners = 6
 *    cap basis, checked at play time via CanAddMiner, TASK-030) vs ARRIVED
 *    miner income (AddMinerIncome/RemoveMinerIncome — the +1/s each).
 *  - Match-end freeze (§3.9): PauseIncome() stops accrual under the Victory
 *    screen; Play Again runs ResetEconomy() + ResetGold() + ResumeIncome()
 *    (the game mode calls them in that order, after ASiegeGameState::ResetClock()).
 *  - Gold is hard-capped at 999 and can never go negative.
 *  - ALL gold mutations route through the private SetGold() so no code path
 *    can skip the clamp or the OnGoldChanged broadcast.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegePlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	/** Fired on every gold mutation with the new value. The HUD (WBP_HUD, TASK-011) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Gold")
	FOnGoldChanged OnGoldChanged;

	/** Fired on every actual composed-rate change and on ResetEconomy. The HUD's "+N/s" gold-rate text binds here (TASK-033), seeded from GetGoldRate() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Gold")
	FOnGoldRateChanged OnGoldRateChanged;

	/** Fired on every actual alive-miner-count change and on ResetEconomy. The HUD's "x/6" miner counter binds here (TASK-033), seeded from GetAliveMinerCount() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Miners")
	FOnMinerCountChanged OnMinerCountChanged;

	// --- Multi-team economy (GDD §4 bot opponent, TASK-043) ---

	/**
	 *  The team this economy belongs to (CONVENTIONS team contract): the local
	 *  player is ALWAYS Blue, the M3 bot is Red. ASiegeGameMode tags each player
	 *  state at creation (Blue for the local player; Red for the bot's PS once
	 *  TASK-045 lands), and ASiegeGameState::GetPlayerStateForTeam(Team) resolves
	 *  the owning economy by it — so a miner (or any team-economy consumer) binds
	 *  income to the right side instead of assuming the single/first player state
	 *  (M2 assumed one). Identity, NOT economy state: never touched by
	 *  ResetEconomy/ResetGold (Play Again keeps each side's team).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Team")
	ETeamId GetTeam() const { return Team; }

	/** Sets the owning team (ASiegeGameMode calls this once per player state at creation — TASK-043). Team is identity; no economy delegate fires. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Team")
	void SetTeam(ETeamId InTeam) { Team = InTeam; }

	/** Current gold, always in [0, MaxGold]. HUD should call this once on construct to seed its display, then rely on OnGoldChanged. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
	int32 GetGold() const { return Gold; }

	/** True if Cost is non-negative and no greater than the current gold. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
	bool CanAfford(int32 Cost) const;

	/**
	 *  Deducts Cost from Gold. Refuses and returns false — changing nothing and
	 *  broadcasting nothing — if Cost is negative or unaffordable. Gold never goes negative.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	bool SpendGold(int32 Cost);

	/** Play Again (GDD §3.9): resets gold to StartingGold and (re)starts the passive income timer. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void ResetGold();

	// --- Economy v2 (GDD §3.2/§3.3, TASK-024): rate-composed income ---

	/**
	 *  Gold added per income tick: base GoldPerTick (2), doubled while the
	 *  ASiegeGameState overtime latch is active (§3.2 — the latch is read LIVE
	 *  every call, never cached, so the rate cannot desync from the shared
	 *  clock), plus MinerGoldPerTick (1) per ARRIVED miner (§3.3). With the
	 *  default 1.0 s tick this is gold per second. This is exactly what the
	 *  next income tick will add — HandleGoldTick calls this same function.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
	int32 GetGoldRate() const;

	/**
	 *  +1 miner income (§3.3): AMinerUnit calls this exactly once when it
	 *  REACHES its gold node (TASK-025) — never on spawn ("+1 gold/s activates
	 *  only when it arrives"). Broadcasts OnGoldRateChanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Miners")
	void AddMinerIncome();

	/**
	 *  -1 miner income (§3.3 "if killed, its +1/s is removed"): AMinerUnit
	 *  calls this on death ONLY if it had arrived (its AddMinerIncome ran) —
	 *  a miner killed en route changes the rate not at all (TASK-025).
	 *  Underflow below 0 is refused and logged. Broadcasts OnGoldRateChanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Miners")
	void RemoveMinerIncome();

	/**
	 *  +1 alive miner: AMinerUnit calls this once from BeginPlay (TASK-025).
	 *  Exceeding MaxActiveMiners only WARNS — the cap is enforced BEFORE any
	 *  spawn, at play time, via CanAddMiner (TASK-030; §3.0 net-zero refusal).
	 *  Broadcasts OnMinerCountChanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Miners")
	void RegisterMinerAlive();

	/** -1 alive miner: AMinerUnit calls this once on death — always, arrived or not (TASK-025). Underflow refused + logged. Broadcasts OnMinerCountChanged. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Miners")
	void UnregisterMinerAlive();

	/** Miners currently alive for this player — en route and arrived alike (§3.3 cap basis; the HUD's "x/6", TASK-033). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Miners")
	int32 GetAliveMinerCount() const { return AliveMinerCount; }

	/** True while another miner may be played: alive miners < MaxActiveMiners (6 // GDD §3.3). TASK-030 checks this BEFORE gold moves ("Miner limit reached" refusal, net-zero gold). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Miners")
	bool CanAddMiner() const { return AliveMinerCount < MaxActiveMiners; }

	/**
	 *  Match-end freeze (§3.9, TASK-024): stops passive income entirely — the
	 *  income timer is cleared AND the tick callback is gated, so gold cannot
	 *  accrue under the Victory screen even if something restarts the timer
	 *  (ResetGold legitimately does — M1 law, qa/TASK-005-report.md major 1).
	 *  Idempotent. Getters and rate/miner delegates keep describing the
	 *  CONFIGURED economy (the rate does not report 0 while paused — pausing
	 *  accrual is not a rate change). ResumeIncome() (Play Again) restarts.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void PauseIncome();

	/** Play Again: lifts the match-end pause and (re)starts the income timer. Idempotent — SetTimer on the same handle replaces, never stacks. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void ResumeIncome();

	/**
	 *  Play Again (§3.9): alive-miner and arrived-miner-income counts back to
	 *  0 and the composed rate re-derived. The game mode calls
	 *  ASiegeGameState::ResetClock() FIRST, so the overtime latch is already
	 *  cleared and the rate lands on the base 2/s. Broadcasts
	 *  OnMinerCountChanged and OnGoldRateChanged unconditionally (reset-path
	 *  broadcast, CONVENTIONS delegate law). Gold itself is ResetGold()'s job.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void ResetEconomy();

protected:

	/** Seeds gold to StartingGold, starts the passive income timer, and binds the §3.2 overtime doubling to the ASiegeGameState clock. */
	virtual void BeginPlay() override;

	/** Stops the passive income timer and unbinds the overtime handler. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  Owning team (TASK-043). Default Blue so a single-player-state world (M2)
	 *  resolves the local economy for Blue and behaves byte-for-byte as before;
	 *  ASiegeGameMode sets it explicitly at creation and tags the bot PS Red
	 *  (TASK-045). Mutate via SetTeam(). // GDD §4 — two coexisting economies
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Gold at match start and after a Play Again reset (GDD §3.2). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 StartingGold = 50;

	/** BASE passive income added on each timer tick — doubled by OvertimeIncomeMultiplier while overtime is active; arrived miners add MinerGoldPerTick each on top (TASK-024 rate composition). // GDD §3.2 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 GoldPerTick = 2;

	/** Seconds between passive income ticks. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0.05"))
	float GoldTickInterval = 1.0f;

	/** Hard cap — gold never exceeds this value (GDD §3.2). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 MaxGold = 999;

	/** Active miner cap — ALIVE miners count toward it, en route or arrived (M2 ruling). Enforced at play time via CanAddMiner (TASK-030). // GDD §3.3 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Miners", meta = (ClampMin = "0"))
	int32 MaxActiveMiners = 6;

	/** Base-income multiplier while the ASiegeGameState overtime latch is active: 2 -> 4 gold/tick (miner bonuses unchanged). // GDD §3.2 — at 7:00 base accrual doubles */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "1"))
	int32 OvertimeIncomeMultiplier = 2;

	/** Gold added per income tick per ARRIVED miner. // GDD §3.3 — +1 gold/s activates only when the miner arrives at its node */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Miners", meta = (ClampMin = "0"))
	int32 MinerGoldPerTick = 1;

private:

	/**
	 *  The ONLY place Gold is written. Clamps NewGold to [0, MaxGold] and, if the
	 *  stored value actually changed, broadcasts OnGoldChanged with the new value.
	 *  A write that clamps back to the current value (e.g. an income tick while
	 *  pinned at the cap) is not a mutation and does not broadcast.
	 */
	void SetGold(int32 NewGold);

	/** Passive income timer callback: adds the composed GetGoldRate() (TASK-024), clamped at MaxGold. Gated on bIncomePaused (match-end freeze). */
	void HandleGoldTick();

	/** (Re)starts the repeating passive income timer. */
	void StartIncomeTimer();

	/** ASiegeGameState overtime handler (bound in BeginPlay): the base rate just doubled — rebroadcast the composed rate to listeners. Accrual itself reads the latch live and needs no handler. */
	UFUNCTION()
	void HandleOvertimeStarted();

	/** Broadcasts OnGoldRateChanged when the composed rate differs from the last broadcast value — or unconditionally when forced (reset paths, CONVENTIONS delegate law). */
	void RefreshGoldRate(bool bForceBroadcast);

	/** The world's ASiegeGameState, resolved live each call (never cached — Play Again / PIE safe), or nullptr (BeginPlay warns once when missing). */
	ASiegeGameState* GetSiegeGameState() const;

	/** Current gold. Mutate ONLY via SetGold(). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Gold", meta = (AllowPrivateAccess = "true"))
	int32 Gold = 50;

	/** Miners alive for this player, en route + arrived (§3.3 cap basis). Mutated only by Register/UnregisterMinerAlive and ResetEconomy. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Miners", meta = (AllowPrivateAccess = "true"))
	int32 AliveMinerCount = 0;

	/** Miners whose +1/s income is active (arrived at the node and still alive, §3.3). Never legitimately exceeds AliveMinerCount. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Miners", meta = (AllowPrivateAccess = "true"))
	int32 MinerIncomeCount = 0;

	/** Last composed rate broadcast through OnGoldRateChanged (change detection; seeded in BeginPlay). */
	int32 CachedGoldRate = 0;

	/** True between PauseIncome() (match end) and ResumeIncome() (Play Again): HandleGoldTick refuses to accrue even if something restarts the timer (ResetGold does — M1 law). */
	bool bIncomePaused = false;

	/** Handle for the repeating passive income timer. */
	FTimerHandle GoldTickTimerHandle;
};
