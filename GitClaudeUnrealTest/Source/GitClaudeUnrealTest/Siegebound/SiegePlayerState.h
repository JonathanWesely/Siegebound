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
 *  Broadcast whenever the composed DISPLAY gold rate actually changes (a miner
 *  arriving at its node, an arrived miner dying, flat income add/remove, or an
 *  overtime flip that moves the rounded display base — with default tuning it
 *  does NOT: the display base is 1 on both sides of 7:00, so the overtime
 *  signal is the HUD's overtime indicator, TASK-089 ruling) and unconditionally
 *  on ResetEconomy (reset-path broadcast, CONVENTIONS delegate law). NewRate is
 *  GetGoldRate()'s per-second AVERAGE with the base rounded UP (TASK-089) —
 *  the HUD's "+N/s" text (TASK-033), no longer the exact per-tick accrual.
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
 *  - Gold starts at 10 (TASK-089 2026-07-08 balance directive; was 50); every
 *    income tick (1.0 s) adds MinerGoldPerTick (1/s) per ARRIVED miner (§3.3)
 *    plus the flat non-miner income (§8), and every BaseIncomeTickPeriod-th
 *    tick (1 ⇒ every 1 s) additionally grants the base GoldPerTick (1 — so
 *    base income is 1 gold per 1 s; TASK-278 2026-07-24 reverted the TASK-089
 *    1-per-2s change), doubled while the shared ASiegeGameState overtime latch
 *    is active (§3.2, 7:00 → 2 gold/s — read LIVE on the grant tick, so accrual
 *    can never desync from the clock).
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

	/**
	 *  Adds Amount to gold, routed through the SAME private SetGold() choke point as
	 *  every other mutation — so the [0, MaxGold] clamp and the OnGoldChanged
	 *  broadcast are always honored (NOT a raw Gold field write). The additive
	 *  sibling of SpendGold. A non-positive Amount is refused + logged (grants must
	 *  be positive; use SpendGold to deduct). Does NOT touch the composed gold rate
	 *  (no AddIncome), so a lump grant never perturbs +N/s accrual.
	 *
	 *  Sole caller: the dev/test Sandbox starting-gold grant
	 *  (ASiegeGameMode::SandboxStartingGold, TASK-071) — the normal match never
	 *  raises gold this way (accrual is the income tick; reset is ResetGold).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void AddGold(int32 Amount);

	// --- Economy v2 (GDD §3.2/§3.3, TASK-024): rate-composed income ---

	/**
	 *  DISPLAY rate for the HUD's "+N/s" text (TASK-089 2026-07-08): the
	 *  per-second AVERAGE of the composed income — NO LONGER the exact per-tick
	 *  accrual, and HandleGoldTick no longer calls it. Base contribution =
	 *  GoldPerTick, doubled while the ASiegeGameState overtime latch is active
	 *  (§3.2 — read LIVE every call, never cached), averaged over
	 *  BaseIncomeTickPeriod and rounded UP for display: with the 2026-07-24
	 *  defaults (BaseIncomeTickPeriod=1, TASK-278) the average is EXACT — a
	 *  truthful +1/s pre-overtime (true base is now 1/s, so the round-up is a
	 *  no-op) and +2/s in overtime. (Pre-TASK-278 the period-2 base averaged
	 *  0.5/s and the round-up still displayed +1/s over that 0.5/s true accrual;
	 *  the accrual now matches the display.) Round-up is STABLE (never alternates),
	 *  so RefreshGoldRate change detection is unaffected. Miner income
	 *  (MinerGoldPerTick per ARRIVED miner, §3.3) and flat income (§8 Deep
	 *  Mine, TASK-057) still land every 1.0 s tick, so their contribution here
	 *  is exact per-second.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
	int32 GetGoldRate() const;

	// --- Flat non-miner income (GDD §8 Deep Mine, TASK-057) ---
	//
	// A SECOND, SEPARATE income accumulator that composes into GetGoldRate()
	// additively — exactly like arrived-miner income — but is NOT a miner: it
	// never touches AliveMinerCount, MaxActiveMiners, CanAddMiner, or the miner
	// delegates, so the M2/M3 miner economy + cap are byte-for-byte unchanged.
	// ADeepMine registers +DeepMineIncome here on placement and removes it on
	// death (§8 raidable economy). Deliberately GENERIC (an int32 gold-per-tick
	// delta) so any future flat-income source can reuse it (TASK-059/060).

	/**
	 *  Adds GoldPerTickDelta to the flat (non-miner) income accumulator, then
	 *  broadcasts OnGoldRateChanged if the composed rate actually changed.
	 *  ADeepMine calls AddIncome(DeepMineIncome) at BeginPlay — the §8 +2/s
	 *  starts the instant the mine is placed (no walk, unlike a miner). A
	 *  non-positive delta is refused + logged (register/unregister must stay
	 *  symmetric and positive). No miner count/cap interaction.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void AddIncome(int32 GoldPerTickDelta);

	/**
	 *  Removes GoldPerTickDelta from the flat (non-miner) income accumulator,
	 *  then broadcasts OnGoldRateChanged if the composed rate actually changed.
	 *  ADeepMine calls RemoveIncome(DeepMineIncome) on death (§8 raidable). A
	 *  non-positive delta, or an amount that would drive the accumulator below
	 *  0, is refused + logged (a caller removed more than it added). No miner
	 *  count/cap interaction.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void RemoveIncome(int32 GoldPerTickDelta);

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
	 *  0, the base-income tick-parity counter back to 0 (TASK-089 — the first
	 *  post-reset base grant lands exactly on the BaseIncomeTickPeriod-th
	 *  tick), and the composed display rate re-derived. The game mode calls
	 *  ASiegeGameState::ResetClock() FIRST, so the overtime latch is already
	 *  cleared and the rate lands on the pre-overtime base display value
	 *  (+1/s round-up with defaults, TASK-089). Broadcasts
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

	/** Gold at match start and after a Play Again reset (10 per the 2026-07-08 balance directive, TASK-089; was 50). KEEP IN SYNC with the private Gold field initializer below — it is the pre-BeginPlay seed value. // GDD §3.2 (amended) */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 StartingGold = 10;

	/** BASE gold added per BASE-INCOME GRANT — one grant every BaseIncomeTickPeriod income ticks (defaults: 1 gold per 1 s — 2026-07-24 balance directive TASK-278, reverts the TASK-089 2026-07-08 1-per-2s income change; was 2 every tick pre-089) — doubled by OvertimeIncomeMultiplier while overtime is active (→ 2 gold/s in overtime); arrived miners add MinerGoldPerTick each EVERY tick on top (TASK-024 rate composition). // GDD §3.2 (amended) */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 GoldPerTick = 1;

	/** Number of GoldTickInterval income ticks between base-income grants: 1 ⇒ the base lands every 1 s (2026-07-24 balance directive TASK-278 — reverts the TASK-089 2026-07-08 1-per-2s income change; was 2 ⇒ every 2 s). Miner and flat (Deep Mine) income are NOT affected — they land every tick. // GDD §3.2 (amended) */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "1"))
	int32 BaseIncomeTickPeriod = 1;

	/** Seconds between passive income ticks. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0.05"))
	float GoldTickInterval = 1.0f;

	/** Hard cap — gold never exceeds this value (GDD §3.2). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 MaxGold = 999;

	/** Active miner cap — ALIVE miners count toward it, en route or arrived (M2 ruling). Enforced at play time via CanAddMiner (TASK-030). // GDD §3.3 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Miners", meta = (ClampMin = "0"))
	int32 MaxActiveMiners = 6;

	/** Base-income multiplier while the ASiegeGameState overtime latch is active: base grant 1 -> 2 per BaseIncomeTickPeriod ticks = 2 gold/s with the 2026-07-24 defaults (TASK-278 period=1; was 1 gold/s at the old 1-per-2s base) (miner/flat bonuses unchanged). Applied per grant, on the grant tick (TASK-089). // GDD §3.2 — at 7:00 base accrual doubles */
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

	/**
	 *  Passive income timer callback (TASK-089 decomposition): miner + flat
	 *  income accrue EVERY tick; the base (GoldPerTick, × overtime multiplier
	 *  read live) lands only on every BaseIncomeTickPeriod-th tick, driven by
	 *  the transient BaseIncomeTickCounter. Exactly ONE SetGold per tick
	 *  (choke-point law; a zero-grant tick is a harmless no-op), clamped at
	 *  MaxGold. Does NOT call GetGoldRate() — that is now the HUD's rounded-up
	 *  display average, not the exact accrual. Gated on bIncomePaused
	 *  (match-end freeze).
	 */
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

	/** Current gold. Mutate ONLY via SetGold(). Initializer KEPT IN SYNC with StartingGold (the pre-BeginPlay seed value; both 10 per TASK-089). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Gold", meta = (AllowPrivateAccess = "true"))
	int32 Gold = 10;

	/** Miners alive for this player, en route + arrived (§3.3 cap basis). Mutated only by Register/UnregisterMinerAlive and ResetEconomy. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Miners", meta = (AllowPrivateAccess = "true"))
	int32 AliveMinerCount = 0;

	/** Miners whose +1/s income is active (arrived at the node and still alive, §3.3). Never legitimately exceeds AliveMinerCount. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Miners", meta = (AllowPrivateAccess = "true"))
	int32 MinerIncomeCount = 0;

	/**
	 *  Flat non-miner income in gold per tick (§8 Deep Mine, TASK-057): the sum
	 *  of every AddIncome minus RemoveIncome. SEPARATE from the miner counts and
	 *  the MaxActiveMiners cap — composed into GetGoldRate() additively and
	 *  zeroed by ResetEconomy (Play Again). Never doubled by overtime (like
	 *  miner income — only the BASE rate doubles at 7:00, §3.2). Mutated only by
	 *  AddIncome/RemoveIncome and ResetEconomy.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Gold", meta = (AllowPrivateAccess = "true"))
	int32 FlatIncomePerTick = 0;

	/** Last composed rate broadcast through OnGoldRateChanged (change detection; seeded in BeginPlay). */
	int32 CachedGoldRate = 0;

	/**
	 *  Transient tick-parity counter for the base-income cadence (TASK-089):
	 *  counts income ticks since the last base grant; HandleGoldTick grants the
	 *  base when it reaches BaseIncomeTickPeriod, then zeroes it. Deliberately
	 *  NON-REFLECTED (plain member, the CachedGoldRate pattern — runtime
	 *  bookkeeping, not tunable state). Reset by ResetEconomy so the Play Again
	 *  base cadence is deterministic.
	 */
	int32 BaseIncomeTickCounter = 0;

	/** True between PauseIncome() (match end) and ResumeIncome() (Play Again): HandleGoldTick refuses to accrue even if something restarts the timer (ResetGold does — M1 law). */
	bool bIncomePaused = false;

	/** Handle for the repeating passive income timer. */
	FTimerHandle GoldTickTimerHandle;
};
