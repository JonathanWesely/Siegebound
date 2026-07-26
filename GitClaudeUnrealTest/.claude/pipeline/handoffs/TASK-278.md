# TASK-278 — Economy balance: base income 1 gold/s + all 28 card costs ×3 + bot bank-threshold audit

**Agent:** gameplay-programmer
**Branch:** m7.6-arena10x
**Status:** ready-for-qa
**Scope:** FILE-ONLY — no compile, no DT_Cards reimport, no Git (all TASK-279's job).

Reverts the INCOME half of the 2026-07-08 TASK-089 directive (1-per-2s → 1-per-1s). `StartingGold` stays 10 (FLAG 1 — intentionally NOT changed).

---

## Files touched (exactly four — verified via `git diff --stat`)
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h` — `BaseIncomeTickPeriod` 2→1 + 5 stale-comment updates
- `Docs/Data/cards.csv` — `Cost` column only, 28 rows ×3
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` — `AttackBankThreshold` 12→36 + comment
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` — 1 stale comment (cpp:56)

`SiegePlayerState.cpp` was NOT touched (out of declared file scope; its logic is data-driven off the properties and stays correct — see the out-of-scope note at the end).

---

## Change 1 — passive gold 1/2s → 1/1s (`SiegePlayerState.h`)
- **The one value change:** `int32 BaseIncomeTickPeriod = 2;` → `= 1;` (now line 285).
- `GoldPerTick=1`, `GoldTickInterval=1.0f`, `StartingGold=10`, `MaxGold=999`, `OvertimeIncomeMultiplier=2`, `MinerGoldPerTick=1` — all UNCHANGED (verified).
- **Result:** base grant of 1 gold now lands every 1s = 1 gold/s (was 1 per 2s). Overtime still doubles the base → 2 gold/s at 7:00 (`OvertimeIncomeMultiplier=2` intact).
- **No logic edited.** `HandleGoldTick`/`GetGoldRate` live in `SiegePlayerState.cpp` and derive from these properties — they pick up the new period automatically. `GetGoldRate()` displayed "+1/s" pre-overtime BEFORE this change (`DivideAndRoundUp(1,2)=1` round-up) and still shows "+1/s" — but the accrual is now truthful (true base is 1/s exactly, round-up is a no-op). In overtime the display now reads +2/s (was +1/s) — the intended, now-truthful doubling.

### Doc comments updated in SiegePlayerState.h (record-of-truth kept consistent)
The spec named h:276 + h:280. The same header carried three MORE comments citing the now-false "1 gold per 2 s / 0.5/s true base / 1 gold/s overtime" default; leaving them would contradict the record-of-truth (the header doc comments ARE the record per the board block). All five updated, **no behavior touched**, each flagged here for QA:
1. **BaseIncomeTickPeriod comment** (was h:280) — "2 ⇒ every 2 s" → "1 ⇒ every 1 s (TASK-278 reverts the TASK-089 1-per-2s change; was 2 ⇒ every 2 s)".
2. **GoldPerTick comment** (was h:276) — "1 gold per 2 s" → "1 gold per 1 s … (→ 2 gold/s in overtime)".
3. **Class-level doc** (h:45-51) — "tick (2 ⇒ every 2 s) … base income is 1 gold per 2 s; was 2/s" → "tick (1 ⇒ every 1 s) … 1 gold per 1 s … (§3.2, 7:00 → 2 gold/s)".
4. **GetGoldRate() display doc** (h:138-150) — removed the stale "true base is 0.5/s (max error 0.5)" + "exact +1/s in overtime"; now: "with the 2026-07-24 defaults the average is EXACT — +1/s pre-overtime (round-up is a no-op), +2/s in overtime", with the pre-TASK-278 0.5/s behavior kept as a historical parenthetical.
5. **OvertimeIncomeMultiplier comment** (h:296) — "= 1 gold/s with defaults" → "= 2 gold/s with the 2026-07-24 defaults (was 1 gold/s at the old 1-per-2s base)".

The only remaining "per-2s / 0.5/s / every 2 s" strings in the file are DELIBERATE historical references (describing what the value WAS), never the current default.

---

## Change 2 — all 28 card costs ×3 (`Docs/Data/cards.csv`, `Cost` column ONLY)
Verified via `git diff`: exactly 28 data rows changed, each ONLY in the 4th field (`Cost`); header + every other column (MaxCopies/HP/Damage/DeckCount/GoldSteal/SpellDelivery/…) byte-identical. Pickpocket's `GoldSteal=10` confirmed untouched (it sits in the GoldSteal column, not Cost).

| Card | old→new | Card | old→new |
|------|--------|------|--------|
| Footman | 3→9 | Masons | 8→24 |
| Archer | 4→12 | SharpenedBlade | 6→18 |
| Knight | 6→18 | PlateArmor | 6→18 |
| Miner | 8→24 | SwiftBoots | 5→15 |
| ArrowTower | 5→15 | WarBanner | 8→24 |
| Wall | 4→12 | Fireball | 7→21 |
| MilitiaMob | 5→15 | FrostNova | 6→18 |
| Pikeman | 5→15 | Lightning | 8→24 |
| Sapper | 5→15 | BattleCry | 5→15 |
| Cavalry | 7→21 | Pickpocket | 6→18 |
| Longbowman | 6→18 | CrystalTower | 9→27 |
| Cleric | 6→18 | Ogre | 12→36 |
| BombTower | 8→24 | Barracks | 10→30 |
| BallistaTower | 7→21 | DeepMine | 15→45 |

All 28 = exact integer ×3. **NOT reimported** — build-master TASK-279 owns the DT_Cards CSV-sync.

**EOL note for build-master:** `git diff` emitted the standard `LF will be replaced by CRLF` autocrlf notice, but the diff shows ONLY the 28 changed rows (header + unchanged rows did NOT flip), so there is no whole-file line-ending churn. Unreal's CSV reimporter is EOL-agnostic.

---

## Change 3 — bot bank threshold (the load-bearing fix)
- `SiegeBotController.h:230` — `int32 AttackBankThreshold = 12;` → `= 36;` (= new Ogre cost, 12×3). Comment updated to explain the ×3 scaling and why (preserves "bank toward the priciest bankable unit" so Rule-4 waves grow toward Knight 18 / Cavalry 21 / Ogre 36 instead of dumping on the cheapest affordable unit the instant gold hits the gate).
- `SiegeBotController.cpp:56` — stale comment "an Ogre needs 12 gold" → "an Ogre needs 36 gold post-TASK-278 ×3". (Documentation only; the code path here is data-driven.)
- Used at `SiegeBotController.cpp:758` `if (Gold >= AttackBankThreshold)` — reads the property, picks up 36 automatically. No literal there.

---

## THE SWEEP — every gold/cost literal inspected in SiegeBotController.{h,cpp} + verdict

Method: read both files end-to-end + grepped the whole Siegebound module for `Gold`/`Cost` comparisons and numeric `>=`/`<=` literals.

### The ONLY hardcoded gold/bank heuristic literal that assumed old costs
- **`AttackBankThreshold = 12`** → **SCALED to 36.** (Rule-4 bank gate. This is the one the board flagged.)

### Data-driven reads that auto-scale — LEFT AS-IS (correct)
Every affordability/spend read uses `Row->Cost` / `Chosen.Row->Cost` and therefore auto-scales with the CSV ×3. Inspected and confirmed no hardcoded cost:
- `FindCheapestDefensiveCard` (cpp:83,93,94) — `Row->Cost > Gold`, `Row->Cost < Best->Cost`, tie compare. Data-driven.
- `FindMostExpensiveUnitCard` (cpp:115,119) — `Row->Cost > Gold`, `Row->Cost > …Cost`. Data-driven (Rule 4's card pick).
- `FindAffordableCardByID` (cpp:137) — `Row->Cost <= Gold`. Data-driven (Miner + Fireball/Lightning).
- `FindAffordableEconomyBuildingCard` (cpp:155) — `Row->Cost <= Gold`. Data-driven (Deep Mine).
- `FindMostExpensiveUnplayableCard` (cpp:183) — `Row->Cost` compare (Rule-5 discard pick). Data-driven.
- All `SpendGold(Chosen.Row->Cost)` (Rule 3a cpp:685, Rule 3b cpp:731) and `SpawnBotCardActor(…, Chosen.Row->Cost, …)` (Rules 1/2/4) — data-driven.

### Gold literals that are NOT card costs — LEFT AS-IS (per spec)
- **`BotDiscardCost = 1`** (h:234) — the §3.6 fixed 1-gold swap fee (NOT a card cost). Used at cpp:812 `Gold >= BotDiscardCost` and cpp:822 `SpendGold(BotDiscardCost)`. **LEFT** (spec instruction).

### Non-gold literals inspected and confirmed unrelated — LEFT AS-IS
- `TargetMinerCount = 3` (h:226) — a COUNT, not gold (spec: STAY). LEFT.
- `FireballClusterMinUnits = 3` (h:268), `LightningTowerMinUnits = 2` (h:272) — unit-count gates, not gold. LEFT.
- `DecisionIntervalSeconds = 2.f` (h:220) — a time cadence. LEFT.
- All placement/geometry knobs (`BotCastleSpawnOffset 1750`, `BotSpawnLaneSpread 900`, `SwarmSpawnRadius 300`, `MinerNodeApproachOffset 400`, `TowerDefenseStandoff 750`, `BuildingClearance 200`, `CastlePlinthClearance 420`, `SpawnBoxHalfExtent (840,840)`, `SpawnBoxAnchorInset 40`, `UnitSpawnClearance 150`, `CastleRedFallbackLocation (25000,0,0)`, `NavProjectionExtent`, `RingRadii {0,250,500,800,1100}`, `RingDirections 8`, standoff math `+150`/`-100`, the spawn-Z trace bounds `+1000`/`-5000`) — spatial, not economic. LEFT (and explicitly must not disturb the TASK-265 spawn-clamp logic — untouched).
- Deck-composition `Entry(TEXT("<card>"), N)` args (cpp:241-269) — COPY COUNTS (deck legality = sum of counts == 50, each ≤ MaxCopies), NOT costs; unaffected by the cost ×3. LEFT. (Their inline `// 3 x12 = 36` cost annotations + the `avg cost ~4.72 / ~7.02` header comments now cite OLD costs — see the note below.)
- `UDeckLibrary::GetDeckAverageCost` logging (cpp:301-303) — reads `Cost` live from DT_Cards → auto-scales; the logged number just grows ×3. Data-driven, LEFT.

### Other files (QA-implied "no other file hardcodes a cost/rate") — grepped the module
- `SiegePlayerController.cpp:1839` `if (TargetingCost > 0)` and `:2216` `if (Row.Cost > 0)` — data-driven `Row.Cost`, compared to 0 (a >0 guard, not an old cost). LEFT.
- `SiegePlayerState.cpp:60` `return Cost >= 0 && Cost <= Gold;` (CanAfford) — data-driven. LEFT.
- No other file compares gold/cost to a hardcoded old-cost integer. `MinerGoldPerTick`/`DeepMine +2/s` yields and `Pickpocket GoldSteal=10` are income/steal values, not card costs — out of scope, unchanged.

**Sweep conclusion: exactly ONE hardcoded gold/cost heuristic literal existed (`AttackBankThreshold=12`); it is scaled to 36. Nothing else required scaling or data-driving.**

---

## For QA to scrutinize
1. **28 Cost cells** — verify against the board's exact table (esp. Ogre 36, DeepMine 45, CrystalTower 27, Barracks 30) and that NO non-Cost column drifted. Pickpocket GoldSteal must still read 10.
2. **`AttackBankThreshold = 36`** at h:230.
3. **Value guardrails** — confirm `StartingGold=10`, `GoldPerTick=1`, `GoldTickInterval=1.0f`, `MaxGold=999`, `OvertimeIncomeMultiplier=2` are UNCHANGED.
4. **Scope decision to flag (my judgment call, QA/manager ruling welcome):** the board spec named only h:276+h:280 for comment edits; I additionally updated 3 more stale comments in the SAME header (class doc, GetGoldRate doc, OvertimeIncomeMultiplier doc) that cited the now-false 1-per-2s default, to keep the header's record-of-truth internally consistent. All comment-only, zero behavior change. If QA prefers the literal minimal-scope edit, these three are the ones to revert — but they would then contradict the shipped value.
5. **Out-of-scope stale comments NOT edited** (reported, not touched — outside the declared file list): `SiegePlayerState.cpp:134` ("default 2 ⇒ 1 gold per 2 s") and `:178` ("GoldPerTick (1) per BaseIncomeTickPeriod (2) ticks") — illustrative comments; the code there is data-driven and correct. `SiegeBotController.cpp` deck-composition cost annotations (cpp:236-269, e.g. `// 3 x12 = 36`, `avg cost ~4.72/~7.02`) now cite old costs — documentation only, deck legality uses COUNTS not costs, and 28+ annotation edits are well outside this task's scope. Surfacing both for a possible future comment-hygiene pass; neither affects behavior.

## NOT done (by design — TASK-279)
Compile · DT_Cards reimport/CSV-sync · Git · PIE economy verify · balance-ledger note.

## No CONVENTIONS change
No new asset/class identifiers introduced.
