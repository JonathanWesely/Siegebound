# TASK-116 handoff — Deck-builder widget C++ base: UDeckBuilderWidget

**Status:** ready-for-qa
**Author:** gameplay-programmer
**Laws honored:** files only, NO editor/MCP, NO compile (rides TASK-117), NO git, NO TASKBOARD.md edits. Both NEW files (no existing file touched). No hardcoded card stat — MaxCopies/Cost/DisplayName/DeckCount all read from DT_Cards. Consumes TASK-113 types (qa-passed, on disk) without recompiling them.

## Files created (both under `Source/GitClaudeUnrealTest/Siegebound/`)
- `DeckBuilderWidget.h` — `UDeckBuilderWidget` (UUserWidget subclass) declaration.
- `DeckBuilderWidget.cpp` — implementation.

No `.Build.cs` change: `UDataTable` (Engine/DataTable.h), `USaveGame`/SaveGame flow (GameFramework + Kismet/GameplayStatics.h), `UUserWidget` (UMG) are all already project module deps (UMG is already used by UCardHandWidget / UUnitHealthBarWidget).

## Consumed surface (from TASK-113, character-for-character — nothing redefined)
- `FDeckList` / `FDeckCardEntry` + `FDeckList::TotalCount()` (`Siegebound/DeckTypes.h`).
- `UDeckLibrary::IsDeckLegal(const UDataTable*, const FDeckList&, FString&)` and `::GetDeckAverageCost(const UDataTable*, const FDeckList&)` (`Siegebound/DeckLibrary.h`).
- `USiegeDeckSaveGame` — `SavedDecks`, `ActiveDeckName`, `USiegeDeckSaveGame::SlotName`, `USiegeDeckSaveGame::UserIndex` (`Siegebound/SiegeDeckSaveGame.h`).
- `FCardRow::MaxCopies/.Cost/.DisplayName/.DeckCount/.CardArt` (`Siegebound/CardRow.h`) + `/Game/Data/DT_Cards`.

**QA carry-forwards honored:** slot references use `USiegeDeckSaveGame::SlotName` / `::UserIndex` (never re-literal `"SiegeDecks"`/`0`); every `Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(...))` is null-checked; `DeckBuilderWidget.cpp` `#include "Kismet/GameplayStatics.h"` (complete-type-include law).

## Full surface TASK-118 (WBP_DeckBuilder) will call — character-for-character

**Class:** `UDeckBuilderWidget : public UUserWidget` — reparent `/Game/UI/WBP_DeckBuilder` to it.

### BlueprintCallable (mutating)
```cpp
void AddCopy(FName CardID);      // +1, refused (no-op) at MaxCopies or missing row
void RemoveCopy(FName CardID);   // -1 down to 0 (entry dropped at 0)
void LoadDefaultDeck();          // seed working deck from the curated DeckCount column
void SaveDeckAs(const FString& Name);   // persist working deck under Name (overwrite-on-collision, case-insensitive)
void LoadDeck(const FString& Name);     // load a saved deck into the working deck
void SetActiveDeck(const FString& Name);// mark a SAVED deck active for the next match (STRICT — must already exist)
UTexture2D* GetCardArtTexture(FName CardID); // null-safe card-art resolver (loads; call once per cell)
TArray<FString> GetSavedDeckNames() const;   // BlueprintCallable (reads disk — call once, cache)
```

### BlueprintPure (const reads — safe to re-read on OnDeckModelChanged)
```cpp
int32   GetCountOf(FName CardID) const;
int32   GetTotalCount() const;              // x/50 numerator
float   GetAverageCost() const;             // §8 guide (via UDeckLibrary)
bool    IsCurrentDeckLegal() const;         // Play-with-deck gate (via UDeckLibrary::IsDeckLegal)
TArray<FName> GetCollectionCardIDs() const; // the 28 DT_Cards row names → browser grid
FString GetCardDisplayName(FName CardID) const; // falls back to raw CardID
int32   GetCardCost(FName CardID) const;
int32   GetCardMaxCopies(FName CardID) const;   // WBP greys "+" when GetCountOf >= this
```

### BlueprintImplementableEvents (float/int/bool/FString params ONLY — widget rule)
```cpp
void OnDeckModelChanged();                                  // "re-read the getters" (seed-then-bind)
void OnDeckSlotCountChanged(const FString& CardID, int32 Count); // per-card counter update
```
Broadcast policy (CONVENTIONS delegate law — never on refused/no-op mutations):
- `AddCopy`/`RemoveCopy` (on success): `OnDeckSlotCountChanged(CardID, newCount)` THEN `OnDeckModelChanged()`.
- `LoadDefaultDeck`/`LoadDeck`/`SaveDeckAs`/`SetActiveDeck` (on success): `OnDeckModelChanged()` only (bulk change — the WBP re-reads all 28 cell counts via `GetCountOf`).
- `OnDeckModelChanged()` alone is a sufficient full refresh; `OnDeckSlotCountChanged` is a single-cell optimization.

### Intended WBP flows (so 118 composes the primitives correctly)
- **Event Construct:** call `LoadDefaultDeck()` (or `LoadDeck` of the last active) to SEED, then read the getters. No delegate binding needed — mutations re-fire `OnDeckModelChanged()`.
- **Browser grid:** `GetCollectionCardIDs()` → per cell `GetCardDisplayName/GetCardCost/GetCardMaxCopies/GetCardArtTexture` + `GetCountOf`; "+" calls `AddCopy`, "-" calls `RemoveCopy`; grey "+" when `GetCountOf >= GetCardMaxCopies`.
- **Play with this deck** (enabled only when `IsCurrentDeckLegal()`): `SaveDeckAs(name)` → `SetActiveDeck(name)` → `ASiegeGameMode::StartMatch`. SetActiveDeck is STRICT (activates a SAVED deck), so persist the working deck FIRST.

## DT_Cards reference — how resolved (documented per spec)
Chose the **EditDefaultsOnly UPROPERTY** option (matches `UCardHandWidget` + `UDeckComponent` precedent): `TSoftObjectPtr<UDataTable> CardTableAsset`, defaulted in the constructor to `/Game/Data/DT_Cards.DT_Cards`, resolved null-safe at use time via `ResolveCardTable()` (LoadSynchronous, missing ⇒ nullptr logged once). A BP can retarget it without code. Both the collection getter and the legality/avg-cost checks read through this one table pointer.

## Mandatory scans (both performed independently)
- **(a) Inherited-reflected-member shadow scan (C4457/58/59 — `Slot` on UWidget/UUserWidget):** CLEAN. `UDeckBuilderWidget : UUserWidget : UWidget` carries reflected `Slot` (UPanelSlot*) — grepped both files: NO local/param/loop var named `Slot` (nor `Owner`/`Instigator`/`PlayerState`/`Controller`). Loop/locals renamed to avoid it: `DeckIndex`, `CardIndex`, `Entry`, `Deck`, `Row`, `Table`, `SaveObj`, `ToSave`, `Existing`, `CanonicalName`, `NewCount`, `Current`, `Index`, `Trimmed`, `Names`, `Result`, `ArtTexture`. Params `Name`/`Count`/`CardID` are NOT reflected UPROPERTYs on UUserWidget (the object name is `NamePrivate`, not a `Name` UPROPERTY) — safe, and `Name` is the spec's own signature.
- **(b) Complete-type-include scan (C2027/C2227 — the TASK-110 miss):** CLEAN.
  - `.cpp` calls `UGameplayStatics::{Load,Save}GameToSlot/CreateSaveGameObject/DoesSaveGameExist` ⇒ `#include "Kismet/GameplayStatics.h"` present.
  - derefs `const UDataTable*` (`FindRow`/`ForeachRow`/`GetRowNames`) ⇒ `Engine/DataTable.h` present.
  - derefs `const FCardRow*` (`->MaxCopies/->Cost/->DisplayName/->DeckCount/->CardArt`) AND instantiates `FindRow<FCardRow>`/`ForeachRow<FCardRow>` (needs `FCardRow::StaticStruct`) ⇒ `Siegebound/CardRow.h` present.
  - static-calls `UDeckLibrary::...` ⇒ `Siegebound/DeckLibrary.h` present.
  - `Cast<USiegeDeckSaveGame>`/`::StaticClass`/`::SlotName`/`::UserIndex`/member access ⇒ `Siegebound/SiegeDeckSaveGame.h` present (complete type for the Cast + StaticClass).
  - `Row->CardArt.LoadSynchronous()` (UTexture2D) ⇒ `Engine/Texture2D.h` present (UCardHandWidget precedent).
  - `UE_LOG(LogGitClaudeUnrealTest,...)` ⇒ `GitClaudeUnrealTest.h` present.
  - `FDeckList/FDeckCardEntry/SiegeLegalDeckSize` complete via header's `Siegebound/DeckTypes.h`.
  - Header: `FDeckList WorkingDeck` value member ⇒ `DeckTypes.h` included (complete type). `UDataTable`/`UTexture2D`/`FCardRow`/`USiegeDeckSaveGame` appear only as pointer returns/params ⇒ forward-declared. `.generated.h` is last.

## Flagged decisions for QA / Jonathan
1. **Additive display resolvers beyond the explicit names-block list — `GetCardDisplayName` / `GetCardCost` / `GetCardMaxCopies` / `GetCardArtTexture`.** The names block lists the model API + "a collection-CardIDs getter." A functional browser grid that greys the "+" at the cap and shows card faces needs per-card name/cost/cap/art, and the widget law is emphatic that the WBP must NOT read DT_Cards ("C++ base owns ALL logic", UCardHandWidget "never reads DT_Cards"). I resolved these here, null-safe, rather than push DT_Cards reads into UMG. Purely additive (no listed name renamed/omitted). If QA prefers a minimal surface, `GetCardArtTexture` is the most droppable (text-only cells still work); the other three are effectively required for the grid.
2. **`SetActiveDeck` is STRICT** — it only activates a deck that already exists in the SaveGame and stores that deck's canonical name, so `ActiveDeckName` never dangles. Consequence: the "Play with this deck" flow must `SaveDeckAs` before `SetActiveDeck` (documented above). Rationale: a dangling active name would silently fall back to the DeckCount default at match start (TASK-114), a confusing surprise. Alternative (lenient set-any-name) is a 2-line change if preferred.
3. **Name matching is CASE-INSENSITIVE** (`FString::Equals(..., ESearchCase::IgnoreCase)`) across SaveDeckAs (overwrite key), LoadDeck, and SetActiveDeck — so "MyDeck"/"mydeck" are the same deck (no near-duplicates; mirrors UE's default `FString ==`). Names are stored with their as-entered casing (trimmed). TASK-114's `ActiveDeckName` lookup should also match case-insensitively for full round-trip parity; not required for correctness (I store the canonical saved name into ActiveDeckName, so an exact-match reader still resolves it).
4. **RemoveCopy drops the entry at 0** (canonical one-entry-per-held-card model) rather than leaving a `Count==0` entry. `IsDeckLegal` accepts either, but the canonical form keeps `WorkingDeck.Cards` tidy and `IndexOfCard` unambiguous.
5. **SaveDeckAs has NO legality gate** (§8/save is never blocked — M6 ruling 6); only "Play with this deck" gates on `IsCurrentDeckLegal()`. You can save an in-progress 30-card deck and finish it later.
6. **`AddCopy` refuses silently at the cap / on a missing row (no broadcast).** The WBP greys the "+"; this is the authoritative data-driven backstop. No refusal BIE was specced (the hand UI has one; the deck-builder greys instead), so a refused add is a pure no-op per the delegate law.
7. **`GetSavedDeckNames` is BlueprintCallable, not Pure** — it reads the SaveGame from disk; a Pure node would re-hit disk on every graph evaluation. The WBP should call it once and cache. All other reads are Pure (they touch only the cached DT_Cards + in-memory working deck).
8. **`LoadSaveGame` guards with `DoesSaveGameExist` first** so the normal first-run (no `.sav` yet) is SILENT — no spurious engine warning. LoadDeck/SetActiveDeck on an empty store still log an intentional Warning (explicit user action on nothing saved).
9. **Row order from `GetCollectionCardIDs`** = DT_Cards row-map (import/CSV) order, not sorted. If the browser wants a specific ordering (by cost/type), the WBP sorts or the manager specifies it.

## What QA should scrutinize
- The additive resolvers (flag 1) — accept as necessary, or ask to trim art.
- Complete-type includes in `DeckBuilderWidget.cpp` (the TASK-110 class of miss): `Kismet/GameplayStatics.h` + `Engine/DataTable.h` + `Siegebound/CardRow.h` are all present; every `Cast<USiegeDeckSaveGame>` is null-checked.
- Slot consts: `USiegeDeckSaveGame::SlotName`/`::UserIndex` used everywhere — no `"SiegeDecks"`/`0` re-literal.
- Data-driven cap: `AddCopy` reads `Row->MaxCopies`; nothing hardcoded.
- BIE params are FString/int32 only (`OnDeckModelChanged()` / `OnDeckSlotCountChanged(const FString&, int32)`).
- No gameplay/combat coupling — the class only edits/persists `FDeckList`s and reads DT_Cards.
