# TASK-060 — Bot v2: Set II plays + upgrades/instants as discards + rule-4 hardening (handoff)

**Author:** gameplay-programmer · **Status:** ready-for-qa · **Compile:** NOT compiled (TASK-068 batches) · **Git:** untouched · **Board:** NOT edited (orchestrator owns it). M4 wave 6 on `main`; LAST M4 code task. This closes the M3 TASK-046 WARN-2.

## Scope guarantee
Bot-internal. **ONLY two files touched:**
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`

I did NOT edit `SiegePlayerController.*` (only CALL its public static `SpawnUnitSwarm`), `SiegeGameMode.*`, `SiegeGameState.*`, `DeckComponent.*`, or any other file. The TASK-046 decision loop is preserved for the core cards (see "Byte-for-byte" below).

## 1) Set II plays reached by the SAME §4 ordered rules (no new fuzzy logic)
The four ordered rules are unchanged in shape; Set II is reached because the new cards carry the existing `ECardType`/`ECardProfile`:

- **Rule 1 (Defend) — new towers for free.** `FindCheapestDefensiveCard` classifies via `IsDefensiveType(CardType) = Unit || Building`. Bomb Tower / Ballista Tower / Barracks / Wall / Arrow Tower are all **CardType Building** → already selectable as defensive plays; `bOutIsBuilding` is correct for them (they route down the building spawn path, between the intruder and Castle_Red). **No selection change.**
- **Rule 3 (Attack) — new units for free.** `FindMostExpensiveUnitCard` selects **CardType Unit**, which now reaches Ogre / Cavalry / Pikeman / Longbowman / Cleric / MilitiaMob. **No selection change.** Rule 3 still gates on banking to `AttackBankThreshold (12)`, so the bot banks and plays Ogres (cost 12) as income scales → growing waves.
- **Rule 2 (Economy) — Deep Mine added as a second economy play.** Restructured to `if (!NearestIntruder) { 2a Miner; 2b Deep Mine; }`:
  - **2a Miner** is **byte-for-byte with TASK-046** — same `GetAliveMinerCount() < TargetMinerCount && CanAddMiner()` gate, same `FindAffordableMinerCard`, same GoldNode-approach spawn + same log (I only added the explicit `/*SwarmCount=*/ 0` argument). Returns iff a miner was found; otherwise falls through to 2b (unchanged fall-through to rule 3 semantics preserved when 2b also declines).
  - **2b Deep Mine** — `FindAffordableEconomyBuildingCard` matches **CardType Economy AND CardID ∈ `BuildingEconomyCardIDs` (= {DeepMine})**, so **Miner is NOT caught here** (Miner is Economy but not in the set). Deep Mine is **building-routed** (ADeepMine : ABuilding under `/Game/Blueprints/Buildings/BP_Building_DeepMine`) → spawned via `SpawnBotCardActor(bIsBuilding=true)` near `GoldNode_Red`, honoring the §3.5 building clearance. **NO miner-cap interaction** (no `CanAddMiner`, no miner-count gate) — it fires whenever the half is clear and one is affordable (cost 15). Only 2 exist in the M4 deck; it strengthens the bot's economy → bigger waves. `BuildingEconomyCardIDs` mirrors `ASiegePlayerController::BuildingEconomyCardIDs` (TASK-059) and is editable, so a future economy-building needs no code change.

## 2) SwarmCount honored via the shared spawn path (TASK-059)
`SpawnBotCardActor`'s **unit path was rewritten to call `ASiegePlayerController::SpawnUnitSwarm(World, ActorClass, CardID, Team=Red, /*SpawnOwner=*/ this, /*SpawnInstigator=*/ nullptr, GroundPoint, FMath::Max(1, SwarmCount), SwarmSpawnRadius=300)`** — the exact signature from `handoffs/TASK-059.md`. So the bot's Militia Mob (`SwarmCount = 4`) spawns **4 Red copies** on a 300-radius circle for ONE cost, identical to the player. Every other unit passes `SwarmCount = 0` → the helper's `Count <= 1` branch spawns a single unit AT the point with no reprojection (byte-for-byte with the M1/M2 single-unit spawn).

- **SpawnPoint semantics changed:** I now pass the **GROUND** point (the navmesh-projected, own-half/clearance-validated point from `ComputeValidBotSpawnPoint`) — `SpawnUnitSwarm` applies the capsule lift itself. The old bot unit path did the lift locally; that (and the `CapsuleComponent`/`SiegeSpawnConstants.h` includes it needed) is removed. Buildings still spawn flush at the ground point (building path unchanged).
- **Team material (TASK-044):** `SpawnUnitSwarm` sets Team before `FinishSpawning`, so Red MI applies at BeginPlay. `SwarmCount` passed = `Row->SwarmCount` at rules 1 (defensive unit) and 3 (attack), `0` for miners.
- **Gold discipline preserved:** spawn the swarm FIRST, then `SpendGold(Cost)` as the LAST gate; a spend refusal `Destroy()`s **every** spawned copy → exactly one Cost is deducted iff at least one actor appears (the TASK-030 destroy-on-fail pattern generalized to N, mirroring the player's confirm path).

## 3) Upgrades / Instants / Spells classified as DISCARDS
The bot has no hero and no spell system, so `IsUnplayableByBot(ECardType)` = **Spell || HeroUpgrade || Utility** is the rule-4 discard set — already correct for Set II:
- **SharpenedBlade / PlateArmor / SwiftBoots / WarBanner** = `HeroUpgrade` → discard.
- **Masons** = `Utility` (Instant) → discard.
- **Spell** = reserved (M5) → discard.
These are the **only** cards rule 4 cycles; the bot never "plays" them (they are not Unit/Building/Economy so no play rule selects them).

## 4) Rule-4 hardening (folds TASK-046 WARN-2, now LIVE)
- **Fee guarded at gold ≥ 1:** the rule-4 condition is `CardIndex != INDEX_NONE && Gold >= BotDiscardCost (1)` → never charged at 0 gold.
- **DiscardFromHand return-checked BEFORE charging:** reordered to **discard FIRST, then `SpendGold` only if `DiscardFromHand` returned true** → no charge for a no-op discard (out-of-range/empty slot). Since the `Gold >= BotDiscardCost` gate holds the same tick (income only adds), the subsequent `SpendGold` cannot fail; a false return is logged as an unreachable tripwire (the card already left the hand → at worst one free cycle, never a leak).
- **No double-charge:** rule 4 charges at most once then `return`s — no loop, no re-entry within a tick.

### Classification of "unaffordable" cards — flagged decision
The spec asks the classifier to treat "hero-upgrade/Instant/Spell/Utility AND unaffordable cards as unplayable." I split this across the two classifiers so that **both hold without breaking banking**:
- **Play rules (1–3)** already classify an **unaffordable** card as unplayable-THIS-tick: they only ever select an *affordable* card (`Cost <= Gold`), so the bot never plays an unaffordable card (the hard invariant).
- **Rule 4's discard set is type-unplayable ONLY** (HeroUpgrade/Utility/Spell). I deliberately do **NOT** add "unaffordable but type-playable" cards (e.g. an Ogre at gold 11) to the discard set, because the bot **banks toward** them — rule 3 only fires at gold ≥ 12, so discarding an unaffordable Ogre during the bank would directly break the acceptance "attacks with growing Set II waves incl. Ogres." Treating unaffordable Units/Buildings/Economy as discardable is the one reading that contradicts a stated acceptance criterion, so it is excluded and documented in-code (see `IsUnplayableByBot`'s comment). QA: if you want a hard-stall breaker for the (astronomically rare, same as M3) all-reserve hand, it's a follow-up, not this task.

## Invariants (re-confirmed)
- **NEVER plays an unaffordable card:** every play rule selects only `Cost <= Gold` cards; `SpendGold` is the last gate with destroy-on-fail; `ConfirmPlayFromHand` (the draw) runs only after a successful spend+spawn. Unchanged from TASK-046.
- **NEVER spawns on the Blue half (X < 0):** `ComputeValidBotSpawnPoint` still returns X ≥ 0 for the center of every play. For the swarm fan, MilitiaMob (the only `SwarmCount` card) spawns centered at the centerline `BotCenterlineSpawnX = 350`, and `SwarmSpawnRadius = 300 < 350`, so copies span X ∈ [50, 650] — all on the Red half; each ring point is navmesh-reprojected within the walkable Red half. This mirrors the player's identical `SpawnUnitSwarm` (TASK-059). Flagged for QA: same shared-helper property the player accepted; a hard clamp is available if you prefer a guarantee independent of the 350-vs-300 relationship.
- **Exactly ONE `LogSiegeBot` line per fired rule:** every rule logs one `LogSiegeBot` line on a completed play/discard; all diagnostics (missing BP, no valid point, zero-spawn, SpendGold tripwire) go to `LogGitClaudeUnrealTest`. Rule 2b (Deep Mine) adds one new `LogSiegeBot` line format; a MilitiaMob swarm is one play = one line.

## Byte-for-byte for M3 core cards (TASK-046 preserved)
- Rule 1 defensive selection/geometry: unchanged (only the SpawnBotCardActor call gained the `SwarmCount` arg; core cards pass 0/their real 0).
- Rule 2a Miner: gate, selection, spawn geometry, and log **identical** to TASK-046 (added the explicit `/*SwarmCount=*/ 0`).
- Rule 3 attack selection/geometry: unchanged (added the `SwarmCount` arg).
- Single-unit spawns (Footman/Archer/Knight/Miner): `SwarmCount = 0` → `SpawnUnitSwarm` `Count <= 1` path = single unit AT the validated point with the same capsule lift — byte-for-byte.
- Match-active gate, placement validity (plinth 420 / clearance 200 / X ≥ 0), ResetBot, StopDecisionTimer, log category: untouched.

## C4457/58/59 shadow scan (CONVENTIONS coding law)
New param `SwarmCount` and locals `SwarmUnits` / `SwarmUnit` (range-for) / `Card` (file-local static) deliberately avoid the inherited reflected UPROPERTYs (`PlayerState`/`Pawn`/`Instigator`/`Owner`/`Controller`). New members `SwarmSpawnRadius`, `BuildingEconomyCardIDs` shadow nothing. No new double→float narrowing (C4244): `FMath::Max(1, SwarmCount)` is int/int; `SwarmSpawnRadius` is float into a float param. Bot PS local stays `BotState` (TASK-045/046 precedent).

## Includes
Added `#include "Siegebound/SiegePlayerController.h"` (alphabetical, before `SiegePlayerState.h`) for the static `SpawnUnitSwarm`. Removed now-unused `#include "Components/CapsuleComponent.h"` and `#include "Siegebound/SiegeSpawnConstants.h"` (the unit path no longer computes the lift locally — `SpawnUnitSwarm` owns it; `SiegeSpawn::` constants are `inline constexpr`, still single-defined for the whole module in that header, so removing the bot's include is ODR-safe).

## Files
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` — new UPROPERTYs `BuildingEconomyCardIDs`, `SwarmSpawnRadius`; `SpawnBotCardActor` gained an `int32 SwarmCount` param; refreshed `EvaluateDecisions` doc.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` — include swap; `FindAffordableEconomyBuildingCard` static; rule-2 Deep Mine branch; SwarmCount threaded to rules 1/3 spawn; hardened rule 4; unit path routed through `SpawnUnitSwarm`.

## Assets referenced (built by parallel/editor tasks; all null-safe)
`/Game/Data/DT_Cards` · `/Game/Blueprints/Units/BP_Unit_<CardID>` (incl. MilitiaMob/Pikeman/Sapper/Cavalry/Longbowman/Cleric/Ogre) · `/Game/Blueprints/Buildings/BP_Building_<CardID>` (incl. BombTower/BallistaTower/Barracks/**DeepMine**) · `/Game/Materials/Instances/MI_TeamColor_Red` (TASK-044) · level actors `Castle_Red`, `GoldNode_Red`.

## What QA should scrutinize
1. **Rule 4 discard order** (discard-first → charge-on-success) vs the player's §3.6 spend-first order — chosen to satisfy "check DiscardFromHand result BEFORE charging." No leak/double-charge; tripwire on the unreachable SpendGold-false path.
2. **"Unaffordable → unplayable" split** (flagged decision above) — rule-4 discard is type-unplayable only, to protect the Ogre-banking acceptance.
3. **Deep Mine routing** via `BuildingEconomyCardIDs` (mirrors TASK-059) and its no-miner-cap fire condition; Miner is excluded from 2b by the set membership check.
4. **Swarm fan vs the never-Blue-half invariant** — 350 (centerline) > 300 (radius); mirrors the player's shared helper. Clamp available if a hard guarantee is wanted.
5. **Include removal** (`CapsuleComponent.h`, `SiegeSpawnConstants.h`) — confirm no transitive dependency was relied on (the unit path no longer touches either).
