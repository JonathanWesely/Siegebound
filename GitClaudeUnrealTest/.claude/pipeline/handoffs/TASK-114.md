# TASK-114 handoff — Wire decks into matches (DeckComponent override path + player active deck + bot 2 decks)

**Status:** ready-for-qa
**Author:** gameplay-programmer
**Laws honored:** files only, NO editor/MCP, NO compile (rides TASK-117), NO git, NO TASKBOARD.md edits. All changes ADDITIVE / backward-compatible — the unset/illegal path is byte-identical to today.

## Files touched (all under `Source/GitClaudeUnrealTest/Siegebound/`)
1. `DeckComponent.h` — `#include "Siegebound/DeckTypes.h"`; new public `void SetPendingDeckList(const FDeckList&)` (BlueprintCallable); new private `bool bHasPendingDeckList = false;` + `FDeckList PendingDeckList;`.
2. `DeckComponent.cpp` — `#include "Siegebound/DeckLibrary.h"`; `SetPendingDeckList` impl; `BuildAndShuffle` restructured to prefer a legal pending list, else the UNCHANGED DeckCount build.
3. `SiegePlayerController.cpp` — 3 includes (`Kismet/GameplayStatics.h`, `Siegebound/DeckLibrary.h`, `Siegebound/SiegeDeckSaveGame.h`); BeginPlay loads the active saved deck and pushes it as the pending override before the existing `BuildAndShuffle`. **TASK-121's two lines PRESERVED** (see below).
4. `SiegeBotController.h` — `#include "Siegebound/DeckTypes.h"`; new `UPROPERTY(EditDefaultsOnly, Category="Siegebound|Bot") TArray<FDeckList> BotDecks`.
5. `SiegeBotController.cpp` — `#include "Siegebound/DeckLibrary.h"`; constructor authors the 2 curated decks; BeginPlay randomly picks one, validates, `SetPendingDeckList`, logs on `LogSiegeBot` — before the existing `BuildAndShuffle`.

## Override / fallback logic (the core contract)
`UDeckComponent::BuildAndShuffle()`:
- Loads `CardTable` ONCE (needed for both the legality check and the DeckCount build).
- `if (bHasPendingDeckList && UDeckLibrary::IsDeckLegal(CardTable, PendingDeckList, Reason))` → build the draw pile from the pending list: `Count` copies of each `Entry.CardID`. Logs the OVERRIDE source.
- `else` → (a) if a pending list WAS set but is illegal, log a Warning with the validator's reason; (b) run the **existing DeckCount build UNCHANGED** (the only diff from the pre-M6 code is `if (const UDataTable* CardTable = ...Load())` became `if (CardTable)` reusing the pre-loaded local — same behavior; the missing-table log-and-empty branch is verbatim).
- **Backward-compat proof:** when `bHasPendingDeckList == false`, the `&&` short-circuits (IsDeckLegal never called), we go straight to `else`, skip the warning, and run the identical DeckCount build. Empty/unset/illegal/missing-table = today's exact path.
- **Persistence:** `bHasPendingDeckList` / `PendingDeckList` are NEVER cleared in `BuildAndShuffle` or `ResetDeck`, so an override survives `ResetDeck()`/Play Again (same match keeps the same deck). `SetPendingDeckList` is the only writer.

`ASiegePlayerController::BeginPlay` (before the existing BuildAndShuffle, inside the same `if (DeckComponent)` block):
- `Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex))` — **null-checked** (TASK-113 carry-forward). Uses the shared `::SlotName` / `::UserIndex` consts (no re-literals).
- No save file ⇒ skip (fallback). Empty `ActiveDeckName` ⇒ skip (fallback). Name not found in `SavedDecks` ⇒ Warning + fallback. Found but `!IsDeckLegal` ⇒ Warning + fallback. Found + legal ⇒ `SetPendingDeckList`.
- The FDeckList is COPIED into the component by `SetPendingDeckList`, so no reference into the transient SaveGame object is retained.

## Bot deck compositions (both legal: sum 50, each Count ≤ that card's MaxCopies)
Authored as `ASiegeBotController` constructor defaults (EditDefaultsOnly, per-BP tunable). Independently verified against `cards.csv` MaxCopies.

**BotDecks[0] "Bot Aggro Rush"** — cheap unit pressure, avg cost 4.72:
| CardID | Count | MaxCopies | Cost×Count |
|--------|-------|-----------|-----------|
| Footman | 12 | 12 | 36 |
| MilitiaMob | 6 | 6 | 30 |
| Pikeman | 6 | 6 | 30 |
| Knight | 6 | 6 | 36 |
| Archer | 6 | 10 | 24 |
| Cavalry | 4 | 4 | 28 |
| Sapper | 4 | 4 | 20 |
| Wall | 4 | 10 | 16 |
| Miner | 2 | 4 | 16 |
| **Sum** | **50** | | **236 → 4.72 avg** |

**BotDecks[1] "Bot Defensive Economy"** — towers/walls/economy/heavy finishers, avg cost 6.86:
| CardID | Count | MaxCopies | Cost×Count |
|--------|-------|-----------|-----------|
| Wall | 10 | 10 | 40 |
| ArrowTower | 8 | 8 | 40 |
| Knight | 6 | 6 | 36 |
| BombTower | 4 | 4 | 32 |
| BallistaTower | 4 | 4 | 28 |
| Miner | 4 | 4 | 32 |
| Barracks | 3 | 3 | 30 |
| CrystalTower | 3 | 3 | 27 |
| Cleric | 3 | 3 | 18 |
| Ogre | 2 | 2 | 24 |
| DeepMine | 2 | 2 | 30 |
| Longbowman | 1 | 4 | 6 |
| **Sum** | **50** | | **343 → 6.86 avg** |

Both are genuinely distinct from each other (aggro-rush vs defensive-economy) AND from the player's TASK-115 curated DeckCount default (Footman 12 / Archer 8 / Knight 3 / Miner 3 / ArrowTower 3 / Wall 4 / MilitiaMob 3 / Pikeman 3 / Cavalry 3 / Longbowman 2 / Cleric 2 / Ogre 2 / Fireball 2).

Bot pick: `FMath::RandRange(0, BotDecks.Num()-1)` → validate via `IsDeckLegal` → `SetPendingDeckList` → one grep-able `LogSiegeBot` line: `[Bot <name>] Deck select: chose curated deck <i> of <n> '<name>' (<50> cards, avg cost <x.xx>) — pushed as pending override.` Missing/illegal ⇒ a `LogSiegeBot` fallback line, no override.

## TASK-121 preservation (SHARED SiegePlayerController.cpp)
Read the file FRESH before editing. Both functional lines are INTACT and untouched:
- `#include "Siegebound/SiegeCheatManager.h"` (line 31) — I inserted my new Siegebound includes as separate lines around it, never modifying it.
- `CheatClass = USiegeCheatManager::StaticClass();` (line 50, in the constructor) — untouched; I edited only BeginPlay, not the constructor.

## Mandatory scans (both performed — CLEAN)
- **(a) Inherited-reflected-member shadow scan (C4457/58/59):** CLEAN. New locals/params/loop vars: DeckComponent.cpp — `CardTable`, `PendingLegalityReason`, `Entry`, `Copy`, `Deck`; SiegePlayerController.cpp — `DeckSave`, `ActiveName`, `ActiveDeck`, `CardTable`, `LegalityReason`, `Candidate`; SiegeBotController.cpp — `Entry`, `Result`, `InCardID`, `InCount`, `AggroDeck`, `FortressDeck`, `DeckIndex`, `ChosenDeck`, `CardTable`, `LegalityReason`. NONE shadow an inherited reflected UPROPERTY (Owner/PlayerState/Pawn/Player/Instigator/Controller/Slot/…). Note `CardTable` ≠ the member `CardTableAsset` (different identifier — no shadow of the member either).
- **(b) Complete-type-include scan (C2027/C2227):** CLEAN.
  - `DeckComponent.h`: `FDeckList PendingDeckList` by-value member ⇒ `DeckTypes.h` included.
  - `DeckComponent.cpp`: `UDeckLibrary::IsDeckLegal` ⇒ `DeckLibrary.h`; FDeckList/FDeckCardEntry complete via header→DeckTypes.h; `UDataTable`/`FCardRow` deref via already-present `Engine/DataTable.h`+`CardRow.h`.
  - `SiegePlayerController.cpp`: `UGameplayStatics::LoadGameFromSlot` ⇒ `Kismet/GameplayStatics.h`; `USiegeDeckSaveGame` deref/`Cast`/statics ⇒ `SiegeDeckSaveGame.h`; `UDeckLibrary` ⇒ `DeckLibrary.h`; FDeckList complete via those.
  - `SiegeBotController.h`: `TArray<FDeckList> BotDecks` ⇒ `DeckTypes.h`.
  - `SiegeBotController.cpp`: `UDeckLibrary` ⇒ `DeckLibrary.h`; FDeckList/FDeckCardEntry complete via header→DeckTypes.h; `UDataTable*` passed by pointer only (Engine/DataTable.h already present).

## Flagged decisions for QA
1. **`FMath::RandRange(0, BotDecks.Num()-1)` instead of the literal `RandRange(0,1)`.** With the 2 default decks this is identical (0 or 1, inclusive). Hardened to `Num()-1` + a `BotDecks.Num() > 0` guard so a BP that edits the array to any size (1, 3, …) can never index out of range — the null-safe house rule. Distribution across the 2 defaults is unchanged.
2. **Both bot decks deliberately contain ONLY bot-playable card types (Unit/Building/Economy).** They omit HeroUpgrade/Utility and all spells — including the bot's castable Fireball/Lightning. Rationale: the bot's rule 5 cycles unplayable cards; a deck with none means the bot never wastes a decision cycling. Trade-off: with these decks the bot's rule-3 spell-cast branch (Fireball/Lightning) never fires. This is a design choice, not a requirement — if QA/Jonathan want the bot to exercise its spell rules, adding e.g. Lightning ×2 to the Fortress deck (dropping Longbowman ×1 + Wall ×1) keeps it legal. Left conservative for clean, efficient bot play.
3. **Two sequential null-checks avoided.** In both controllers the new load/pick logic sits INSIDE the existing `if (DeckComponent)` block, before the existing `BuildAndShuffle()` call — the `BuildAndShuffle()` call and its `else` branch are byte-identical to before (minimal, review-friendly diff; no double guard).
4. **`PendingDeckList` is a plain (non-UPROPERTY) member.** It holds only FName/FString/int32 (no UObject refs), so it needs no GC tracking; it is transient runtime state set at match start. Consistent with keeping override state out of reflection/serialization.
5. **Player-side "not found" / "illegal" are Warnings, not silent.** No-save-file and empty-ActiveDeckName are SILENT (the common/default fallback), but a set-but-broken active deck logs a Warning so a playtest can see why the curated default was used. Not player-facing.

## What QA should scrutinize
- Backward-compat: confirm the `bHasPendingDeckList == false` path in `BuildAndShuffle` is byte-identical behavior to pre-M6 (short-circuit → else → DeckCount build; the only source change is reusing the pre-loaded `CardTable` local).
- TASK-121's include + `CheatClass` line are preserved (constructor untouched; only BeginPlay edited).
- Both bot decks: independently re-sum to 50 and re-check each Count ≤ MaxCopies from `cards.csv`.
- The shared `USiegeDeckSaveGame::SlotName` / `::UserIndex` consts are used (no re-literals); the `LoadGameFromSlot` cast is null-checked.
- Complete-type includes above (the class of miss that broke TASK-110).
