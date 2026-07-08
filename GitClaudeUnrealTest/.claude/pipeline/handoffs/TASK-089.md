# TASK-089 handoff — Economy balance: StartingGold 10 + base income 1 gold per 2 s (gameplay-programmer, 2026-07-08)

Files only, no editor, no compile (compile is TASK-090's bounce). Implements the Jonathan 2026-07-08 URGENT
balance directive exactly per the manager rulings 1–6 in the TASKBOARD chain header.

## Files touched (before → after)

### Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h
| Member | Before | After |
|---|---|---|
| `StartingGold` | 50 | **10** — keep-in-sync comment added tying it to the private `Gold` initializer (pre-BeginPlay seed value) |
| `Gold` (private field initializer) | 50 | **10** — reciprocal keep-in-sync comment |
| `GoldPerTick` | 2 | **1** — doc comment redefined: base gold per BASE-INCOME GRANT (one grant every `BaseIncomeTickPeriod` income ticks), doubled by `OvertimeIncomeMultiplier` in overtime; GDD §3.2 tag kept, marked "(amended)" with the 2026-07-08 directive noted |
| `BaseIncomeTickPeriod` | — (new) | **NEW** `UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "1")) int32 BaseIncomeTickPeriod = 2;` — income ticks between base grants (2 ⇒ every 2 s; 1 = legacy every-tick) |
| `BaseIncomeTickCounter` | — (new) | **NEW private plain `int32 = 0`** — transient tick-parity counter, deliberately NON-REFLECTED (CachedGoldRate pattern; runtime bookkeeping, not tunable state) |

Doc-comment updates in the header (no code effect): class doc block (gold 50/2/s bullet → 10 / 1-per-2-s
cadence), `FOnGoldRateChanged` delegate doc (display-average semantics — see flagged decision F2),
`GetGoldRate()` doc (display rate + round-up rule), `ResetEconomy()` doc (counter reset), `HandleGoldTick()`
doc (decomposition, "does NOT call GetGoldRate()"), `OvertimeIncomeMultiplier` doc (stale "2 -> 4 gold/tick"
→ "1 -> 2 per BaseIncomeTickPeriod ticks = 1 gold/s"; **value untouched** — see F3).

### Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp
- **`HandleGoldTick()` (decomposed):** keeps the `bIncomePaused` gate; per-tick grant =
  `(MinerIncomeCount * MinerGoldPerTick) + FlatIncomePerTick` (EVERY tick — miner/DeepMine per-second income
  literally untouched); `++BaseIncomeTickCounter`, and when it reaches `BaseIncomeTickPeriod` it zeroes and
  adds `GoldPerTick × OvertimeIncomeMultiplier if overtime else GoldPerTick` (overtime latch read LIVE on the
  grant tick, multiply-per-grant — ruling 3). Exactly **ONE** `SetGold(Gold + TickGrant)` per tick; SetGold
  remains the single Gold writer; a zero-grant tick is a harmless no-op (no change ⇒ no broadcast). Comments
  state explicitly that it no longer calls `GetGoldRate()`.
- **`GetGoldRate()` (redefined as DISPLAY rate, ruling 4):**
  `FMath::DivideAndRoundUp(EffectiveBase, BaseIncomeTickPeriod) + MinerIncomeCount*MinerGoldPerTick + FlatIncomePerTick`
  where `EffectiveBase = GoldPerTick × (overtime ? OvertimeIncomeMultiplier : 1)`, latch read live. Doc states:
  per-second average, base rounded UP for display, no longer exact per-tick accrual.
- **`ResetEconomy()`:** `BaseIncomeTickCounter = 0` added (counter-reset placement — see below); stale
  "lands on the base 2/s" comment corrected.
- Stale comment fixes: `BeginPlay` overtime-bind comment ("read LIVE in GetGoldRate ⇒ accrual correct" → live
  read now happens in HandleGoldTick for accrual, GetGoldRate for display); `HandleOvertimeStarted` comment
  (notes that with default tuning the rounded display base is 1 on both sides of 7:00, so change detection
  correctly stays silent — the 7:00 signal is the overtime HUD indicator, ruling 4).

### Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp — COMMENT-ONLY (verified via git diff: 3 hunks, all `//` lines)
- ~L106: "seeds gold to StartingGold = 50" → "= 10, TASK-089".
- ~L579: Play-Again clock-reset rationale "lands the rate on the base 2/s" → "pre-overtime base (display +1/s round-up, true 1 gold per 2 s)".
- ~L867: Sandbox grant "normal +2/s economy stands" → "normal base economy stands (1 gold per 2 s, TASK-089)".

## Implementation choice (ruling 2 — recorded)
**Every-Nth-tick base grant**, per the manager's binding ruling. `GoldTickInterval` stays **1.0f** so the
1-second tick heartbeat is untouched: miners and DeepMine keep their exact per-second accrual and the GDD
§3.3/§8 "+N/s" semantics. The rejected alternative (GoldTickInterval → 2.0 s) would have halved miner/DeepMine
per-second income and turned the HUD "+N/s" into a 2× lie. Gold stays int32 end to end — no float gold.

## Counter-reset placement (ruling 5 — chosen spot + rationale)
`BaseIncomeTickCounter = 0` lives in **`ResetEconomy()`**, immediately after `FlatIncomePerTick = 0` and
before the reset-path broadcasts. Rationale: (a) the counter is economy STATE exactly like the miner/flat
counts that ResetEconomy already zeroes — ResetGold() is gold-value + timer concerns only; (b) Play Again
always runs `ResetEconomy() → ResetGold() → ResumeIncome()` (after `ResetClock()`), so the first post-reset
base grant lands exactly on the `BaseIncomeTickPeriod`-th tick — deterministic cadence, never inheriting the
prior match's parity. First BOOT is equally deterministic: the member initializer is 0 and BeginPlay's
ResetGold starts the timer, so the first base grant lands on tick 2 (t≈2 s).

## Display-rounding rule for the HUD (ruling 4 — recorded)
`GetGoldRate()` is now the DISPLAY rate: the per-second AVERAGE with the base contribution rounded **UP**
(`FMath::DivideAndRoundUp`). With defaults: pre-overtime shows **+1/s** while the true base is 0.5/s (max
error 0.5 — ruled acceptable; "+0/s" over a visibly rising counter reads as broken); overtime shows an exact
**+1/s** base. The value is STABLE (never alternates), so RefreshGoldRate change detection is unaffected.
Consequence (documented in code): HandleGoldTick does NOT call GetGoldRate() for accrual anymore. With default
tuning the overtime flip does not move the display base (1 → 1), so OnGoldRateChanged correctly stays silent
for a miner-less economy — the 7:00 signal is the overtime HUD indicator.

## Explicitly-unchanged verification (spec point 8 — checked post-edit via grep + diff)
`GoldTickInterval = 1.0f` ✓ · `OvertimeIncomeMultiplier = 2` ✓ · `MinerGoldPerTick = 1` ✓ · `MaxGold = 999` ✓ ·
`MaxActiveMiners = 6` ✓ · `DeepMine.h` NOT in the diff (untouched, DeepMineIncome 2) ✓ · `SiegeGameMode.h` NOT
in the diff (SandboxStartingGold 9999) ✓ · `FOnGoldChanged`/`FOnGoldRateChanged` declarations byte-identical
(int32 params; only the doc PROSE above FOnGoldRateChanged changed — F2) ✓ · SpendGold/AddGold/AddIncome/
RemoveIncome/PauseIncome/ResumeIncome bodies untouched ✓ · zero UMG changes ✓.

## Acceptance walk-through (by inspection)
- Pre-overtime, no miners: ticks 1,2,3,4 → grants 0,+1,0,+1 — exactly 1 gold per 2 ticks (2 s). ✓
- Overtime, no miners: grant tick adds 1×2 = +2 per 2 ticks = 1 gold/s — exactly double. ✓
- One arrived miner: +1 EVERY tick, plus the base cadence on top (per-second miner value unchanged). ✓
- DeepMine: FlatIncomePerTick composes every tick, value owned by DeepMine.h (untouched). ✓
- Boot gold 10 (field init + BeginPlay ResetGold), Play Again gold 10 (ResetGold ← StartingGold). ✓
- Exactly one SetGold per tick; SetGold is still the only Gold writer. ✓

## Flagged decisions for QA (explicit rulings requested)
- **F1 — `>=` instead of `==` on the counter threshold** (HandleGoldTick): if `BaseIncomeTickPeriod` is
  tuned DOWN in-editor mid-session, a counter already above the new threshold can never hit `==` and base
  income would stall forever; `>=` makes the very next tick grant and re-sync. Identical behavior to `==` in
  all normal play. Judged defensive, not a deviation.
- **F2 — FOnGoldRateChanged doc comment updated** (header prose only): spec 7 says the delegate is
  "untouched"; I read that as the SIGNATURE/contract (which is byte-identical) and updated the now-false
  prose ("NewRate is the gold added per income tick") to the display-average semantics — leaving it would
  contradict spec 6's "display semantics documented" mandate. The DECLARE line itself is character-identical.
- **F3 — OvertimeIncomeMultiplier doc comment corrected** (value untouched at 2): the old comment said
  "2 -> 4 gold/tick", false after this change. Comment hygiene beyond spec 9's enumerated list; same rationale
  as F2. Also corrected on the same grounds: BeginPlay overtime-bind comment, HandleOvertimeStarted comment,
  ResetEconomy header/cpp comments — all listed in "Files touched" above.
- **F4 — shadow-law self-scan (CONVENTIONS C4457/58/59)**: new identifiers = `BaseIncomeTickPeriod`,
  `BaseIncomeTickCounter` (members), `TickGrant`, `EffectiveBase`, `BaseRate`, `SiegeGameState`, `bOvertime`
  (locals). None shadow an inherited reflected UPROPERTY (APlayerState: Score/PlayerId/PlayerNamePrivate/
  bIsABot/StartTime/…; AActor: Owner/Instigator/Tags/…). The `SiegeGameState`/`bOvertime` local names are the
  pre-existing QA-passed pattern in this same class (BeginPlay/GetGoldRate). QA to confirm.
- **F5 — worktree residue not mine**: TASKBOARD.md, WBP_MainMenu.uasset, BP_Unit_Footman/Archer.uasset were
  already dirty before this task (ruling 9 adjudicates them in TASK-090's bounce window). I touched none.

## What QA should scrutinize
1. HandleGoldTick: one-SetGold-per-tick invariant + the counter cadence math (grants land on ticks N, 2N, …).
2. GetGoldRate: DivideAndRoundUp arguments (EffectiveBase, BaseIncomeTickPeriod) — ClampMin "1" guards /0 from
   the editor; the C++ default is 2.
3. Shadow scan per F4 (mandatory per the chain's dispatch shape).
4. That the SiegeGameMode.cpp diff is comment-only (it is — 3 hunks, all `//`).
