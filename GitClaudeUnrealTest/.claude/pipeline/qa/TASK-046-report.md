# QA Report — TASK-046

**Task:** Bot decision loop — 2 s ordered rules + placement + LogSiegeBot decision trace (C++)
**Reviewer:** qa-reviewer · **Date:** 2026-07-04 · **Milestone:** M3 (`main`)
**Files reviewed:** `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` + `.cpp` (EvaluateDecisions body + private helpers)
**Compared read-only against:** `SiegePlayerController.cpp/.h` (TASK-030 placement), `TASKBOARD.md` (§ TASK-046 + M3 rulings), `handoffs/TASK-046.md`, `cards.csv`, and the called-API headers.
**Compile status:** NOT compiled (pre-compile gate; batches at TASK-051 with 042/043/044/045/047).

## Verdict: PASS — 0 BLOCKER · 2 WARN · 3 NIT

FAIL requires ≥1 BLOCKER; there are none. The two hard invariants hold, placement is faithfully mirrored, and all called symbols resolve.

---

## Hard-invariant confirmation (the two that cost an engine/game-integrity failure)

**1. NEVER plays an unaffordable card — CONFIRMED.**
Every rule filters affordability before spending:
- Rule 1 `FindCheapestDefensiveCard` skips any `Row->Cost > Gold`; Rule 2 `FindAffordableMinerCard` requires `Cost <= Gold`; Rule 3 `FindMostExpensiveUnitCard` skips `Cost > Gold`; Rule 4 gates on `Gold >= BotDiscardCost`.
- `Gold` is read once (`BotState->GetGold()`) and cannot drift downward before the spend, because every firing rule `return`s — no rule spends then falls through to another.
- `SpendGold` is the LAST gate in `SpawnBotCardActor` (and re-checks `CanAfford` internally), destroying the deferred actor on refusal. Gold moves **iff** an actor is produced. Belt-and-suspenders even against a theoretical race.

**2. NEVER spawns on the Blue (enemy) half — CONFIRMED.**
The only writer of a spawn point is `ComputeValidBotSpawnPoint`, which returns a point **only** after `IsBotHalfPointClear` passes. That helper's first test is `IsOnOwnHalf(Point.X)` → `X >= BotHalfBoundaryX (0)` for Red, applied to the **navmesh-snapped** `Projected.Location` (not the pre-projection guess), and again to `Desired` in the no-navmesh degrade-open path. There is no code path that emits a spawn point with `X < 0`. The rule-1 tower standoff geometry additionally stays on the castle→intruder segment (both endpoints `X >= 0`), and any stray is caught by the same validity gate.

**3. No gold spent on a failed/missing-BP spawn — CONFIRMED.**
`SpawnBotCardActor` order: `ResolveBotCardActorClass` (missing/incompatible BP → log on `LogGitClaudeUnrealTest` + `return nullptr`, no spend) → `SpawnActorDeferred` (null → return, no spend) → `SpendGold` (last gate, destroy-on-fail) → `Init*`/`FinishSpawning`. `ConfirmPlayFromHand`/`DiscardFromHand` (the draw) run in `EvaluateDecisions` only **after** a non-null return. Missing BP = skip, no gold, no draw. Matches the CONVENTIONS composed soft-class law.

---

## Placement-reimplementation fidelity vs TASK-030

| Rule | Player (SiegePlayerController, TASK-030) | Bot (SiegeBotController) | Match |
|---|---|---|---|
| Own half | `X <= 0` (Blue, PlacementMaxX=0) | `X >= 0` (Red, BotHalfBoundaryX=0) | ✅ mirrored |
| Plinth keep-out | 2D box, `CastlePlinthClearance=420`, all `ACastle`, IsValid-only | 2D box, `CastlePlinthClearance=420`, all `ACastle`, IsValid-only | ✅ identical |
| Building clearance | 2D `DistSquared2D < BuildingClearance²`, `BuildingClearance=200`, all live `ABuilding`, `IsBuildingDestroyed` skip, **buildings only** | same, `BuildingClearance=200`, buildings only | ✅ identical |
| Castle ≠ building for clearance | `ACastle` disjoint from `ABuilding` iteration | same | ✅ identical |
| Nav projection | `ProjectPointToNavigation(pt, out, NavProjectionExtent)` | same call, ring search | ✅ same API |
| Nav-projection **extent** | `(50, 50, 50)` | `(200, 200, 1000)` | ⚠️ **diverges** — see WARN-1 |

Faithful on all three placement invariants. The extent value diverges (WARN-1) but is justified and cannot produce an invalid placement, since own-half/plinth/clearance are re-tested on the *snapped* point. `MinerCardID="Miner"` matches. **The programmer did NOT edit `SiegePlayerController` for this task:** its TASK-030 logic (`IsPointOnNavmesh`, `IsPointInsideCastlePlinth`, `HasBuildingClearance`) is intact and contains zero bot code — the rules are independently reimplemented bot-side. (The `M` git status on that file is attributable to earlier M3 tasks, not TASK-046.)

---

## Rulings on the 5 flagged decisions (handoffs/TASK-046.md)

1. **Enemy HERO counts as a rule-1 intruder — ACCEPT.** The acceptance "player pushes onto the bot half → a defensive play within 2 s" must hold when the player advances with their hero alone; the Blue hero is an alive `ITeamAgent` combatant. Verified it does NOT break the idle-economy path: an idle hero sits at PlayerStart (`X < 0`), so `IsOnOwnHalf(X>=0)` is false → not counted → rules 2/3 proceed. Correct.
2. **"Defensive play" = Unit OR Building — ACCEPT.** Spec rule 1 literally says "a unit ... OR a tower." `IsDefensiveType = Unit||Building`. Wall (Damage 0) as a defensive building (blocks pathing) is valid; labeling any building "building" in the log is accurate. Cheapest-with-Unit-tiebreak is sound (units have no clearance constraint).
3. **Rule 4 discards most-expensive UNPLAYABLE card — ACCEPT.** The rule-4 gate is "hand holds an unplayable card"; discarding the most-expensive *playable* card would throw away a useful unit/building — clearly not the intent. Confirmed: the M3 core set is all Unit/Building/Economy, so `IsUnplayableByBot` (Spell/HeroUpgrade/Utility) matches nothing → **rule 4 never fires in M3** — pure M4/M5 forward-compat, as claimed.
4. **Rule 2 "gold ≥ 8" as data-driven affordability — ACCEPT.** Verified in `cards.csv`: Miner Cost = **8**. The bot's affordability check (`Miner.Cost <= Gold`) is therefore byte-equivalent to the spec's literal "gold ≥ 8" today, and tracks future CSV rebalances. Rule 3 (`≥12`) and rule 4 (`≥1`) correctly stay as UPROPERTY strategic thresholds (they are not card costs). Correct split.
5. **Randomized spawn Y (±900) with ring-snap — ACCEPT.** Fan-out is desirable for an AI opponent; the ring search re-snaps to the nearest valid navmesh point, and every candidate is own-half/plinth/clearance validated. Non-determinism tick-to-tick is acceptable and intended.

---

## Findings

- **[WARN] SiegeBotController.h:226 / .cpp:593 — `NavProjectionExtent = (200,200,1000)` diverges from the player's `(50,50,50)` (TASK-030).** The task's fidelity mandate calls out "same nav-projection extent," so this is flagged explicitly. **Assessment: justified and safe, no code change required.** The player traces the cursor to a *real* ground point then confirms with a tight extent (the small vertical extent is what rejects castle-roof/plinth-top hits). The bot has no cursor; it synthesizes a `Desired` point at a *guessed* Z (Castle/GoldNode origin) and needs a generous vertical extent to "find the floor," plus a slightly wider horizontal extent to seat a synthetic guess. Critically, `ProjectPointToNavigation` snaps DOWN onto the ground navmesh and the result is re-validated by `IsBotHalfPointClear` (own-half + plinth-box + clearance), so the larger extent cannot yield a placement the player fundamentally couldn't reach, cannot land on a plinth/roof (2D plinth box + navmesh both exclude it), and cannot cross to the Blue half. L_Arena is a single flat navmesh, so there is no higher-elevation island the generous extent could wrongly accept. Non-blocking; noted for design awareness.

- **[WARN] SiegeBotController.cpp:423-425 — Rule 4 spends the discard fee BEFORE `DiscardFromHand` and does not check its return.** This is the exact shape of the player's TASK-022 WARN-1 gold-leak (SpendGold-then-discard); the player controller was hardened to pre-check the slot and to log loudly if `DiscardFromHand` returns false after a spend. Here it is **unreachable in practice** (rule 4 is dormant in M3, and `Chosen.Slot` comes from the same-tick gather of non-empty resolved slots with no intervening hand mutation, so the discard cannot fail), hence WARN not BLOCKER. Recommend, for M4/M5 when unplayable cards exist, mirroring the player's hardened pattern (check the `DiscardFromHand` return / pre-validate) so a future refactor can never silently leak 1 gold.

- **[NIT] SiegeBotController.cpp:325,364,397 — `ConfirmPlayFromHand` return value not checked.** The player controller logs a loud regression tripwire on a false return; the bot ignores it. Cannot fail within one synchronous tick (nothing mutates the bot hand between gather and play), so cosmetic parity only.

- **[NIT] SiegeBotController.cpp:289-343 — rule 1 falls through to rules 2/3 when an intruder is present but no defensive card is affordable.** Rule 2's `!NearestIntruder` gate then blocks it, but rule 3 (`Gold>=12`) has no intruder gate and can fire an attack while the bot half is contested. This matches the spec's literal rule ordering (rule 3 carries no intruder condition) and is reasonable (if it cannot afford to defend, banking/attacking is the only other move) — recording only so it is a conscious behavior, not an oversight.

- **[NIT] SiegeBotController.cpp:729 — capsule-CDO lift uses `ASummonedUnit` CDO half-height with an 88 fallback.** Identical to the player path; fine. Noted only that a BP whose capsule differs from its CDO would use the CDO value — same benign behavior as TASK-030.

## Engine-safety / convention sweep (all clean)

- **Deprecated UE 5.8 APIs:** none. `UNavigationSystemV1::GetCurrent` / `GetDefaultNavDataInstance` / `ProjectPointToNavigation(pt,out,extent)`, `TActorIterator`, `SpawnActorDeferred`, `GetDefaultObject`, `GetTimerManager().SetTimer/ClearTimer`, `GetAuthGameMode`, `GetPlayerState<T>` all current.
- **Symbol resolution (pre-compile):** every called member verified against its header — `AGoldNode::GetTeam()` (note: GoldNode uses `GetTeam`, correctly distinct from the `ITeamAgent::GetTeamId()` used for Castle/Unit/Hero/Building — the bot calls each correctly), `HasMatchEnded()` (SiegeGameMode override), `GetHandSize/GetHandCardID/ConfirmPlayFromHand/DiscardFromHand`, `GetGold/SpendGold/GetAliveMinerCount/CanAddMiner/ResetEconomy`, `IsUnitDead/InitUnit/GetTeamId`, `IsDead`, `IsBuildingDestroyed/InitBuilding`, `FCardRow::{Cost,CardType}`, `ECardType::{Unit,Building,Economy,Spell,HeroUpgrade,Utility}` (no `Instant` enumerator — the code correctly enumerates the three unplayable types). No missing symbol.
- **C4458 (inherited reflected-member shadow):** clean. No local/param/loop named `Owner`, `Instigator`, `Controller`, `Pawn`, or `PlayerState`; the bot PS local is `BotState`/`BotPS`, iterators `It`, indices `Index`/`SlotIndex`/`SampleIndex`. `GetBotPlayerState()` uses `GetPlayerState<T>()` rather than a shadowing local.
- **C4244 (float/double narrowing):** addressed. `IsOnOwnHalf(double)` widened; `ToIntruder2D.Size()` cast to float for standoff math; ring `Angle` is `double`; `ClearanceSq` is `double` via `FMath::Square(static_cast<double>(...))`. No implicit double→float assignment.
- **Null-safety:** `GetWorld`, `Deck`, `BotState`, `CardTable`, per-row `Row` (added only when non-null, so the static selectors never deref null), `NavSys`(+ nav data), `GameMode` cast, `NearestIntruder`, `UnitCDO`/`Capsule`, spawned actors — all guarded. `bWarnedNoNavData` one-shot latch present.
- **Match-active gate:** `EvaluateDecisions` early-returns on `!IsMatchActive()`; `IsMatchActive` reads `ASiegeGameMode::HasMatchEnded()` and is permissive only in a degenerate (no-GameMode) world — the bot-internal half of the TASK-047 belt-and-suspenders.
- **LogSiegeBot:** `DECLARE_LOG_CATEGORY_EXTERN` in .h:24, `DEFINE_LOG_CATEGORY` in .cpp:23 — the bot is its sole user. Exactly one `LogSiegeBot` line per completed play/discard (rules 1-4 each emit one inside the success branch); all non-decision diagnostics route to `LogGitClaudeUnrealTest`.
- **Deck/economy routing:** plays via `Deck->ConfirmPlayFromHand`, discards via `Deck->DiscardFromHand`, spends via `BotState->SpendGold` on the bot's own Red `ASiegePlayerState` (`GetPlayerState<ASiegePlayerState>()`); `CanAddMiner()` enforced in rule 2; Cost/CardType read from `/Game/Data/DT_Cards` every tick, nothing hardcoded.

## Notes for build-master (PASS)

- Compiles in the TASK-051 M3 batch with 042/043/044/045/047. No new module dependency (NavigationSystem already in Build.cs since TASK-030). No Build.cs edit.
- No engine/asset edit in this task — bot-internal C++ only; `SiegePlayerController` untouched.
- Runtime expectation for PIE verification: with the player idle the LogSiegeBot trace should show Rule 2 (Economy) ×3 → Rule 3 (Attack) growing waves; pushing the Blue hero/units past X=0 should show a Rule 1 (Defend) line within one 2 s beat. Rule 4 will NOT appear in M3 (no unplayable cards) — that is expected, not a regression.
- Both WARNs are documented as accepted/justified and require no code change to pass; they are forward-compat / awareness notes.
