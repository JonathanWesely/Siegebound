# TASK-102 Handoff — Bot v4: M5 spell rules (Fireball at clusters, Lightning at defended towers)

**From:** gameplay-programmer
**Date:** 2026-07-08
**Status:** ready-for-qa (file work only — NOT compiled; compiles at the TASK-103 batch)

## Files touched (complete list)

- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`

No other files were modified. TASKBOARD.md untouched (orchestrator owns status flips).

## What changed

### 1. Rule 3 (SPELLS) inserted; chain renumbered per M5 ruling 10

New `EvaluateDecisions` order: **1 Defend, 2 Economy, 3 SPELLS (new), 4 Attack (was 3), 5 Cycle (was 4)**. Every trace label, code comment, and the header doc were renumbered — grep `LogSiegeBot` output now shows `Rule 1 (Defend)`, `Rule 2 (Economy)`, `Rule 3a (Spell-Fireball)`, `Rule 3b (Spell-Lightning)`, `Rule 4 (Attack)`, `Rule 5 (Cycle)`.

- **3a Fireball:** hand + affordability check FIRST (`FindAffordableCardByID`), THEN the world scan (`FindFireballClusterTarget`). Fires when >= `FireballClusterMinUnits` (3) player-team units cluster within the Fireball row's `AoERadius`; resolves at the cluster **centroid** via `USpellLibrary::ResolveSpell`.
- **3b Lightning:** evaluated only if 3a found no cluster. Fires when a live player **ATower** has >= `LightningTowerMinUnits` (2) player-team units within the Lightning row's `AoERadius`; resolves at the **tower's location**.
- If a target IS found, rule 3 owns the tick (returns) whether the resolver accepts or refuses — a refusal (returns false) spends no gold, keeps the card, logs a Verbose diagnostic on `LogGitClaudeUnrealTest` (NOT on `LogSiegeBot`), and retries next tick. Mirrors rule 1's refused-spawn-point precedent.
- If no target is found, the rule does not fire and the chain falls through to rule 4.

### 2. Cluster algorithm (documented per spec)

Snapshot alive player-team `ASummonedUnit` locations once per decision. Every unit anchors a candidate cluster = all player units (itself included) within `ClusterRadius`, **2D distance** (M4.5 hills must not break grouping). Qualifies at member count >= 3 (anchor + 2 others = the spec's "any unit having >=2 other player units within 300"). Winner = anchor with the MOST members; ties broken by anchor distance to Castle_Red (nearest = biggest threat) — deterministic for a given world state. Cast point = the winning cluster's member centroid. O(N^2) pairwise on the snapshot, run only after the hand/affordability gates and only inside the 2 s cadence.

### 3. Tower-adjacency algorithm (documented)

Same unit snapshot; iterate live enemy `ATower` actors (`!IsBuildingDestroyed()`, enemy `GetTeamId()`), count player units within radius (2D). Winner = most nearby units; tie = nearest Castle_Red. Lightning resolves AT the tower, so the resolver's ruling-4 top-current-HP selection includes the tower itself (distance 0) — the "tower-killer" behavior.

### 4. Play accounting (spec point 2)

Mirrors unit plays exactly: resolve first (the spell's "spawn"), `SpendGold(Row->Cost)` as the LAST gate, then `ConfirmPlayFromHand(Slot)` moves the card to the discard pile. `SpendGold` cannot fail (affordability held this tick, income only adds between checks — the rule-5 fee invariant); a false return is a logged tripwire, never a double-charge.

### 5. Rule-5 discard-set exemption (spec point 3)

`FindMostExpensiveUnplayableCard` now takes the two castable CardIDs and skips them: **Fireball/Lightning are never cycled** — the bot holds them awaiting a target (the bank-toward-an-Ogre precedent). **FrostNova/BattleCry/Pickpocket keep NO cast rule** (GDD silent) and still fall through to rule-5 discard economics, documented at `IsUnplayableByBot`.

### 6. New members / helpers (shadow-scanned)

- UPROPERTYs (mechanic rules, GDD § comments): `FireballCardID` ("Fireball"), `LightningCardID` ("Lightning"), `FireballClusterMinUnits` (3), `LightningTowerMinUnits` (2).
- Private helpers: `FindFireballClusterTarget(float ClusterRadius, int32 MinUnits, FVector& OutCentroid, int32& OutClusterSize) const`, `FindLightningTowerTarget(float SearchRadius, int32 MinUnits, int32& OutNearbyUnitCount) const`.
- Renamed anonymous-namespace helper `FindAffordableMinerCard` → `FindAffordableCardByID` (byte-for-byte behavior for rule 2; now shared by 2/3a/3b).
- C4458 shadow-scan performed: no local/param collides with any member name (new or existing).

## Trace format (M3 acceptance law — exactly one LogSiegeBot line per fired rule)

```
[Bot <name>] Rule 3a (Spell-Fireball): cast 'Fireball' (cost 7) at cluster centroid (X, Y, Z) — N player units within 300, gold G1->G2.
[Bot <name>] Rule 3b (Spell-Lightning): cast 'Lightning' (cost 8) at player tower '<name>' (X, Y, Z) — N player units within 400, gold G1->G2.
```

## Assets / externals referenced

- `USpellLibrary::ResolveSpell(UWorld*, FName, const FCardRow&, ETeamId, const FVector&)` — pinned CONVENTIONS signature; **`SpellLibrary.h` does not exist yet** (TASK-098, same parallel file wave). This file will not compile until 098 lands — by design, per the board's file-wave/TASK-103 batching law.
- `FCardRow::AoERadius / Cost` (TASK-097, already landed), `ATower`/`ABuilding::GetTeamId/IsBuildingDestroyed`, `ASummonedUnit::GetTeamId/IsUnitDead` — all verified present.

## Flagged decisions (QA scrutiny list)

1. **Cluster/adjacency radii read from `Row->AoERadius`, not hardcoded 300/400.** Ruling 10's numbers equal the Fireball/Lightning AoERadius CSV cells; per the data-driven law the bot reads the table, so a balance edit re-tunes the bot automatically. The min-unit counts (3/2) are mechanic-rule UPROPERTYs.
2. **Hero excluded from both unit counts.** GDD says "player units"; the hero is counted for rule-1 DEFEND (TASK-046 precedent) but not as a cluster/adjacency member here. The resolver may still damage a hero inside the blast — that is resolver-side.
3. **Miners count as cluster members** (they ARE `ASummonedUnit`s) — a 3-miner mining cluster is a legitimate Fireball target. GDD does not exclude them.
4. **Fireball/Lightning are never discarded by rule 5**, even if a target never appears (potential hand-slot clog if the player never clusters). Chosen to mirror the rule-4 bank-toward-it precedent; the spec exempts only FrostNova/BattleCry/Pickpocket to discard.
5. **A found-but-refused resolve still owns the tick** (return without a LogSiegeBot line) — mirrors rule 1's refused-spawn-point behavior; retries next tick. LogSiegeBot stays strictly one-line-per-ACTUAL-play.
6. **Deterministic tie-breaks** (most members/units, then nearest Castle_Red) are manager-undefined details, chosen for grep-able reproducibility.
7. **Centroid Z is the mean of unit actor (capsule-center) locations**, not a ground projection. Whether the TASK-098 radial helper measures 2D or 3D from TargetPoint is resolver-side; at ClusterRadius 300 a ~90-unit capsule offset cannot exclude legitimate members. QA may cross-check against 098's helper semantics at the batch.
8. **`#include "Siegebound/SpellLibrary.h"` references a not-yet-existing TASK-098 file** — intentional (parallel file wave; compile at TASK-103). If 098's landed signature deviates from the pinned one, TASK-103 will surface it and 098 owns the fix (CONVENTIONS pin).
9. **Rule 3 runs regardless of intruders** (per ruling-10 ordering): if rule 1 lacks an affordable defensive card while intruders are present, a Fireball at an intruding cluster can act as de-facto defense. This is a straight consequence of the mandated rule order.
