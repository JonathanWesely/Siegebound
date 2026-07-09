# TASK-113 handoff — Deck data model + SaveGame + legality/avg-cost library

**Status:** ready-for-qa
**Author:** gameplay-programmer
**Laws honored:** files only, no editor/MCP, no compile, no git, no TASKBOARD edits. All files NEW (no existing file touched). No hardcoded card stat — MaxCopies/Cost read from DT_Cards via `FindRow<FCardRow>`.

## Files created (all under `Source/GitClaudeUnrealTest/Siegebound/`)
- `DeckTypes.h` — header-only (`FDeckCardEntry`, `FDeckList`, `SiegeLegalDeckSize`).
- `DeckLibrary.h` / `DeckLibrary.cpp` — `UDeckLibrary` (UBlueprintFunctionLibrary).
- `SiegeDeckSaveGame.h` / `SiegeDeckSaveGame.cpp` — `USiegeDeckSaveGame` (USaveGame).

No `.Build.cs` change: `UDataTable` (Engine/DataTable.h) and `USaveGame` (GameFramework/SaveGame.h) are both in the Engine module, already a project dependency.

## Public surface TASK-114 / TASK-116 consume (character-for-character)

### DeckTypes.h
```cpp
USTRUCT(BlueprintType)
struct GITCLAUDEUNREALTEST_API FDeckCardEntry
{
    FName  CardID = NAME_None;   // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Siegebound|Deck")
    int32  Count  = 0;           // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Siegebound|Deck")
};

USTRUCT(BlueprintType)
struct GITCLAUDEUNREALTEST_API FDeckList
{
    FString                  DeckName;   // UPROPERTY(EditAnywhere, BlueprintReadWrite, ...)
    TArray<FDeckCardEntry>   Cards;      // UPROPERTY(EditAnywhere, BlueprintReadWrite, ...)
    int32 TotalCount() const;            // sum of every entry's Count
};

static constexpr int32 SiegeLegalDeckSize = 50;   // GDD §3.4
```

### DeckLibrary.h  (`#include "Siegebound/DeckLibrary.h"`)
```cpp
UCLASS()
class GITCLAUDEUNREALTEST_API UDeckLibrary : public UBlueprintFunctionLibrary
{
    static bool  IsDeckLegal(const UDataTable* CardTable, const FDeckList& Deck, FString& OutReason);
    static float GetDeckAverageCost(const UDataTable* CardTable, const FDeckList& Deck);
};
```
- `IsDeckLegal` — true iff every entry CardID resolves to a DT_Cards row, per-CardID copy total ≤ that row's `MaxCopies`, and `Deck.TotalCount() == SiegeLegalDeckSize`. `OutReason` = first violation (cleared on success). Null table ⇒ false + reason. Never logs (pure validator — callers surface `OutReason`). Never crashes.
- `GetDeckAverageCost` — `sum(FCardRow.Cost × entry Count) / TotalCount()`; `0.0f` for empty deck or null table. Unresolvable CardIDs contribute 0 cost; denominator stays `TotalCount()`.

### SiegeDeckSaveGame.h  (`#include "Siegebound/SiegeDeckSaveGame.h"`)
```cpp
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeDeckSaveGame : public USaveGame
{
    static const FString      SlotName;        // == TEXT("SiegeDecks")  (defined in .cpp)
    static constexpr int32    UserIndex = 0;
    TArray<FDeckList>  SavedDecks;             // UPROPERTY(BlueprintReadWrite, ...)
    FString            ActiveDeckName;         // UPROPERTY(BlueprintReadWrite, ...)
};
```
Reader/writer pattern for 114/116:
```cpp
// write
UGameplayStatics::SaveGameToSlot(SaveObj, USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex);
// read (null-safe: nullptr ⇒ no file ⇒ treat as empty SavedDecks + empty ActiveDeckName ⇒ DeckCount fallback)
USiegeDeckSaveGame* SaveObj = Cast<USiegeDeckSaveGame>(
    UGameplayStatics::LoadGameFromSlot(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex));
```
`UGameplayStatics::CreateSaveGameObject(USiegeDeckSaveGame::StaticClass())` for a fresh object.

## Mandatory scans (both performed)
- **(a) Inherited-reflected-member shadow scan (C4457/58/59):** clean. `UDeckLibrary`/`USiegeDeckSaveGame` derive from UObject (no `Owner`/`Instigator`/`PlayerState`/`Controller`/`Slot`/`Name`-class reflected members to shadow); `FDeckCardEntry`/`FDeckList` are plain USTRUCTs (no inherited reflected members). No local/param/loop var named after any inherited reflected UPROPERTY (locals: `CardTable`, `Deck`, `OutReason`, `RunningCounts`, `Running`, `Entry`, `Row`, `Total`, `CostSum`).
- **(b) Complete-type-include scan (C2027/C2227):** clean.
  - `DeckLibrary.cpp` dereferences `const UDataTable*` (`FindRow<>`) ⇒ includes `Engine/DataTable.h`; dereferences `const FCardRow*` (`->MaxCopies`, `->Cost`) ⇒ includes `Siegebound/CardRow.h`; uses `FDeckList`/`FDeckCardEntry`/`SiegeLegalDeckSize` ⇒ complete via `DeckLibrary.h` → `Siegebound/DeckTypes.h`. No log-category use, so `GitClaudeUnrealTest.h` intentionally not included.
  - `SiegeDeckSaveGame.cpp` only defines a `static const FString` — no upcast/deref beyond the class's own header.
  - Headers: `DeckLibrary.h` uses `UDataTable` only as a pointer in signatures ⇒ forward-declared (`class UDataTable;`), `FDeckList` by-ref but `DeckTypes.h` included (matches SpellLibrary.h precedent). `SiegeDeckSaveGame.h` has a `TArray<FDeckList>` UPROPERTY member ⇒ `DeckTypes.h` included (complete type required for the container member). Every `.generated.h` is the last include.

## Flagged decisions for QA / Jonathan
1. **Copy-cap enforced on the per-CardID AGGREGATE, not literally per-entry.** The spec/CONVENTIONS wording is "every Count in [0..MaxCopies]." I enforce the running per-CardID sum ≤ MaxCopies (a `TMap<FName,int32>` accumulator). Rationale: `MaxCopies` = "maximum copies of this card allowed in a deck" (FCardRow comment) — the real intent is a per-card cap. For the widget's normal one-entry-per-card deck this is IDENTICAL to a per-entry check; it only differs for a hand-authored/save-loaded/bot deck that splits one card across duplicate entries, which the aggregate correctly rejects. This is a superset-safe strengthening of the literal wording. If QA/Jonathan want strict per-entry, it's a 2-line change.
2. **`IsDeckLegal` "first violation" order is deterministic in entry order:** unknown-CardID → negative-Count → over-cap (checked as the running sum crosses MaxCopies) → wrong total (50). Reason strings are HUD/log ready (e.g. `Card 'Footman' has 5 copies — the cap is 4 (MaxCopies).`). No `§` glyph in the reason strings (kept ASCII "GDD 3.4") to avoid any source-encoding risk in a user-facing string; comments elsewhere keep `§`.
3. **Library deliberately does NOT log** (pure/stateless, may be called every model change by the widget). Callers own user-facing messaging via `OutReason`. Consistent with USpellLibrary being C++-only static math.
4. **`GetDeckAverageCost` denominator = `TotalCount()`** (literal §8 formula), even if some entries have unresolvable rows (those add 0 to the numerator). Only diverges from a "valid-rows-only" denominator for illegal decks, and the value is a display-only guide. Returns `0.0f` (not NaN) for empty/degenerate (`Total <= 0`).
5. **Struct members are `EditAnywhere`** (not EditDefaultsOnly/ReadOnly) specifically so TASK-114's `UPROPERTY(EditDefaultsOnly) TArray<FDeckList> BotDecks` is editable in the BP details panel, and `BlueprintReadWrite` so BP can inspect/set. **Members are UPROPERTY by necessity** — SaveGame tagged-property serialization only round-trips UPROPERTY fields, so nested `FDeckList`/`FDeckCardEntry` fields MUST stay UPROPERTY for save/load to work. Do not demote them.
6. **`static const FString SlotName` defined out-of-line in the .cpp** (one definition, no ODR risk); `UserIndex` is `static constexpr int32` (implicitly inline, C++17+/UE5.8 C++20). `SiegeLegalDeckSize` is `static constexpr` at namespace scope (internal linkage per TU — exactly as specced).

## What QA should scrutinize
- The aggregate-vs-per-entry cap decision (flag 1) — confirm it's an acceptable strengthening.
- Complete-type includes in `DeckLibrary.cpp` (the class of miss that broke TASK-110): `Engine/DataTable.h` + `Siegebound/CardRow.h` are both present.
- That `USiegeDeckSaveGame::SlotName`/`UserIndex` names match what 114/116 will reference (contract: `SlotName`, `UserIndex`).
