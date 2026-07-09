# QA Report — TASK-116 (Deck-builder widget C++ base: UDeckBuilderWidget)

**Verdict: PASS**  ·  Blockers: 0  ·  Warnings: 0  ·  Nits: 2
Reviewed 2026-07-09 (qa-reviewer, pre-compile). Files-only task; TASK-117 compiles the M6 batch.
Scope: 2 NEW files under `Source/GitClaudeUnrealTest/Siegebound/` — `DeckBuilderWidget.h` / `DeckBuilderWidget.cpp`. No existing file touched. Cross-read to verify the consumed surface (TASK-113, on disk, qa-passed): `DeckLibrary.h`, `SiegeDeckSaveGame.h/.cpp`, `DeckTypes.h`, `CardRow.h`, and `CardHandWidget.h` (the `TSoftObjectPtr<UDataTable>` precedent).

---

## Mandatory scans (both performed independently — grepped, not taken on faith from the handoff)

### (a) Inherited-reflected-member shadow scan (C4457/58/59 — the TASK-025/029 failure class) — CLEAN
- `UDeckBuilderWidget : UUserWidget : UWidget : UVisual : UObject`. The dangerous inherited reflected member here is **`Slot` (UPanelSlot*) on `UWidget`**. Grepped both files for `\b(Slot|Owner|Instigator|PlayerState|Controller)\b` → **zero matches** in either file. No local, parameter, or loop variable shadows a reflected inherited UPROPERTY.
- Loop/locals are all safely renamed: `DeckIndex`, `CardIndex`, `Entry`, `Deck`, `Row`, `Table`, `SaveObj`, `ToSave`, `Existing`, `CanonicalName`, `NewCount`, `Current`, `Index`, `Trimmed`, `Names`, `Result`, `ArtTexture`.
- Params `CardID` / `Name` / `Count`: none is a reflected UPROPERTY on `UUserWidget`/bases (the object name is `NamePrivate`, private, not a `Name` UPROPERTY; there is no `Count`/`CardID` inherited member). `Name` is also the spec's own signature. No shadow.

### (b) Complete-type-include scan (C2027/C2227 — the TASK-110 failure class) — CLEAN
Every type that is dereferenced, upcast, or template-instantiated in `DeckBuilderWidget.cpp` has its full header included:
- `UGameplayStatics::{Load,Save}GameToSlot / CreateSaveGameObject / DoesSaveGameExist / LoadGameFromSlot` → `#include "Kismet/GameplayStatics.h"` present (line 8). ✓ (TASK-113 carry-forward.)
- `const UDataTable*` deref — `ForeachRow<FCardRow>` / `FindRow<FCardRow>` / `GetRowNames` → `#include "Engine/DataTable.h"` present (line 5). ✓
- `const FCardRow*` deref (`->MaxCopies/->Cost/->DisplayName/->DeckCount/->CardArt`) **and** template instantiation `FindRow<FCardRow>`/`ForeachRow<FCardRow>` (needs `FCardRow::StaticStruct`) → `#include "Siegebound/CardRow.h"` present (line 9). ✓ This is the exact TASK-110 mode; it is NOT repeated.
- `UDeckLibrary::IsDeckLegal / GetDeckAverageCost` static calls → `#include "Siegebound/DeckLibrary.h"` present (line 10). ✓
- `Cast<USiegeDeckSaveGame>` + `::StaticClass()` + `::SlotName` + `::UserIndex` + `->SavedDecks/->ActiveDeckName` → `#include "Siegebound/SiegeDeckSaveGame.h"` present (line 11). ✓ (complete type required for the Cast/StaticClass.)
- `Row->CardArt.LoadSynchronous()` returns `UTexture2D*` (LoadSynchronous does an internal `Cast<UTexture2D>` — needs the complete type) → `#include "Engine/Texture2D.h"` present (line 6). ✓
- `UE_LOG(LogGitClaudeUnrealTest, …)` → `#include "GitClaudeUnrealTest.h"` present (line 7). ✓
- Header: value member `FDeckList WorkingDeck` (complete type) → `#include "Siegebound/DeckTypes.h"` present (line 8). `UDataTable`/`UTexture2D`/`USiegeDeckSaveGame`/`FCardRow` appear only as forward-declared pointer returns/params/soft-ptr member ⇒ forward-declared (lines 11–14), which is sufficient. `DeckBuilderWidget.generated.h` is the LAST include (line 9). ✓

---

## Findings

- **[NIT]** DeckBuilderWidget.cpp:288 — `SaveDeckAs` writes `WorkingDeck.DeckName = Trimmed` (and mutates the in-memory `SaveObj->SavedDecks`) *before* the `SaveGameToSlot` write. On a disk-write failure it returns having stamped the working deck with the "saved" name even though nothing persisted (the `SaveObj` copy is local and discarded, so there is no persistent corruption; the failure is logged). Cosmetic only — moving the `WorkingDeck.DeckName` assignment after a successful write would tighten it. No change required.
- **[NIT]** DeckBuilderWidget.cpp:43–53 — `AddCopy` calls `GetCountOf(CardID)` (a linear scan) and then `IndexOfCard(CardID)` (a second linear scan of the same array). Trivial at ≤28 unique entries; could reuse a single `IndexOfCard` result. No change required.

No BLOCKER and no WARN findings.

---

## TASK-113 carry-forwards — VERIFIED IN CODE (all three, not just the handoff's claim)

1. **Shared slot consts, no re-literal** — `USiegeDeckSaveGame::SlotName` / `::UserIndex` are used at every SaveGame call site (cpp lines 290, 294, 300, 378, 381, 445, 453, 462). Grepped: no `"SiegeDecks"` / bare `0` re-literal anywhere. ✓ (`SlotName` is defined once in `SiegeDeckSaveGame.cpp:7` = `TEXT("SiegeDecks")`.)
2. **Null-checked cast** — `LoadSaveGame()` (cpp 441–454) guards with `DoesSaveGameExist` first, then `Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(...))` and returns it; every caller (`SaveDeckAs`, `LoadDeck`, `GetSavedDeckNames`, `SetActiveDeck`, `LoadOrCreateSaveGame`) null-checks the result before dereferencing. A corrupt/foreign slot casts to nullptr rather than crashing. ✓
3. **`#include "Kismet/GameplayStatics.h"`** — present (cpp line 8). ✓

---

## Correctness verification (traced against the TASK-113 surface, not assumed)

- **`AddCopy`** — resolves `Row` first (missing table/row ⇒ silent refuse, cpp 37–41), then refuses at the cap with the **data-driven** `if (Current >= Row->MaxCopies) return;` (cpp 44). Nothing hardcoded. On success sets `NewCount = Current+1`, adds a fresh `FDeckCardEntry` or bumps the existing one, then fires `OnDeckSlotCountChanged(CardID.ToString(), NewCount)` **then** `OnDeckModelChanged()`. Refused/no-op paths broadcast NOTHING (CONVENTIONS delegate law). ✓
- **`RemoveCopy`** — no-op (no broadcast) when absent or already 0 (cpp 78); otherwise `NewCount = Count-1`, and at `<=0` `RemoveAt(Index)` drops the entry (canonical one-entry-per-card). No negative counts possible. Broadcasts `OnDeckSlotCountChanged` then `OnDeckModelChanged`. ✓
- **`LoadDefaultDeck`** — clears `Cards`+`DeckName`, then `Table->ForeachRow<FCardRow>` seeding one entry per row with `DeckCount > 0` (`Entry.Count = Row.DeckCount`). Missing table ⇒ empty deck (logged once by `ResolveCardTable`). Fires `OnDeckModelChanged`. Correctly mirrors the curated DeckCount column (TASK-115), no hardcoded deck. ✓
- **`GetTotalCount` / `GetAverageCost` / `IsCurrentDeckLegal`** — all **delegate**, no reimplemented math: `WorkingDeck.TotalCount()`; `UDeckLibrary::GetDeckAverageCost(ResolveCardTable(), WorkingDeck)`; `UDeckLibrary::IsDeckLegal(ResolveCardTable(), WorkingDeck, Reason)`. Signatures match `DeckLibrary.h` (`const UDataTable*`, `const FDeckList&`, `FString&`) character-for-character. ✓
- **`GetCollectionCardIDs`** — `Table->GetRowNames()` (the 28 DT_Cards rows), data-driven, empty when the table is missing (not hardcoded to 28). ✓
- **`SaveDeckAs` / `LoadDeck` / `GetSavedDeckNames` / `SetActiveDeck`** — round-trip through the one slot, null-safe. `SaveDeckAs` refuses empty/whitespace names, overwrites a case-insensitive same-name entry (no silent duplicates), `LoadOrCreate` for the write path. `LoadDeck`/`SetActiveDeck` use `LoadSaveGame` (never create) so activating/loading from an empty store warns and no-ops. `SetActiveDeck` is STRICT — stores the *canonical* `Deck.DeckName` into `ActiveDeckName`, so it never dangles. ✓
- **DT_Cards resolution** — `EditDefaultsOnly TSoftObjectPtr<UDataTable> CardTableAsset`, defaulted in the constructor to `/Game/Data/DT_Cards.DT_Cards`; resolved null-safe via `ResolveCardTable()` (`LoadSynchronous`, missing ⇒ nullptr logged ONCE via mutable `bWarnedMissingTable`). `ResolveCardRow` logs once per CardID (mutable `WarnedMissingRowIDs`), `GetCardArtTexture` logs once per CardID (`WarnedCardArtIDs`). Every method degrades gracefully on a missing table/row/art — no crash path found. Matches the `UCardHandWidget` precedent (same `CardTableAsset` property, verified). ✓
- **BIE params** — `OnDeckModelChanged()` (no params) and `OnDeckSlotCountChanged(const FString& CardID, int32 Count)`: FString + int32 only. No struct/enum param anywhere — safe for MCP BP authoring at TASK-118. ✓
- **No gameplay/combat coupling** — the class only edits/persists `FDeckList` and reads DT_Cards; no spawn, tick, HP, or gold API touched. ✓
- **API currency (UE 5.8)** — `UUserWidget`, `TSoftObjectPtr`, `UDataTable::ForeachRow/FindRow/GetRowNames`, `UGameplayStatics` SaveGame flow, `LoadSynchronous`, `TSet`, `FString::TrimStartAndEnd`/`Equals(ESearchCase::IgnoreCase)` — all current; nothing deprecated/removed.

---

## Contract confirmation for TASK-118 (names are law — a deviation here would block the WBP)

Every name in the TASK-116 `names:` block is present with the exact signature — **no listed name renamed, omitted, or re-typed**:

| names-block entry | code | ✓ |
|---|---|---|
| `AddCopy(FName)` | `void AddCopy(FName CardID)` — BlueprintCallable | ✓ |
| `RemoveCopy(FName)` | `void RemoveCopy(FName CardID)` — BlueprintCallable | ✓ |
| `GetCountOf(FName) const` | `int32 GetCountOf(FName CardID) const` — BlueprintPure | ✓ |
| `GetTotalCount() const` | `int32 GetTotalCount() const` — BlueprintPure | ✓ |
| `GetAverageCost() const` | `float GetAverageCost() const` — BlueprintPure | ✓ |
| `IsCurrentDeckLegal() const` | `bool IsCurrentDeckLegal() const` — BlueprintPure | ✓ |
| `LoadDefaultDeck()` | `void LoadDefaultDeck()` — BlueprintCallable | ✓ |
| `SaveDeckAs(FString)` | `void SaveDeckAs(const FString& Name)` — BlueprintCallable | ✓ |
| `LoadDeck(FString)` | `void LoadDeck(const FString& Name)` — BlueprintCallable | ✓ |
| `GetSavedDeckNames() const` | `TArray<FString> GetSavedDeckNames() const` — BlueprintCallable | ✓ |
| `SetActiveDeck(FString)` | `void SetActiveDeck(const FString& Name)` — BlueprintCallable | ✓ |
| collection-CardIDs getter | `TArray<FName> GetCollectionCardIDs() const` — BlueprintPure | ✓ |
| BIE `OnDeckModelChanged()` | `void OnDeckModelChanged()` — BlueprintImplementableEvent | ✓ |
| BIE `OnDeckSlotCountChanged(FString CardID, int32 Count)` | `void OnDeckSlotCountChanged(const FString& CardID, int32 Count)` — BlueprintImplementableEvent | ✓ |

**Additive surface TASK-118 also relies on (beyond the names block — see flagged decision 1, ACCEPTED):** `GetCardDisplayName(FName) const → FString`, `GetCardCost(FName) const → int32`, `GetCardMaxCopies(FName) const → int32` (BlueprintPure), and `GetCardArtTexture(FName) → UTexture2D*` (BlueprintCallable). These keep the WBP out of DT_Cards per the widget law.

---

## Rulings on the programmer's 9 flagged decisions

1. **Additive per-card display resolvers (`GetCardDisplayName`/`GetCardCost`/`GetCardMaxCopies`/`GetCardArtTexture`) — ACCEPTED.** The widget law is emphatic that the WBP must NOT read DT_Cards ("C++ base owns ALL logic"; `UCardHandWidget` "never reads DT_Cards"). A functional 28-card browser that shows name/cost, greys the "+" at the cap, and paints card faces genuinely needs these. They are PURELY ADDITIVE — no listed name is renamed or omitted, no signature altered — so this is not a names-block deviation and does not endanger TASK-118. Keep `GetCardArtTexture` too: it is the correct `UCardHandWidget` resolver pattern for the "Card artwork (hand UI)" law (BlueprintCallable UObject return, null-safe, LoadSynchronous per TASK-079 ruling 4). Recorded as an explicit TASK-118 carry-forward above.
2. **`SetActiveDeck` STRICT (only activates an already-saved deck; stores canonical name) — ACCEPTED.** Prevents a dangling `ActiveDeckName` (which would silently fall back to the DeckCount default at match start — a confusing surprise). The documented `SaveDeckAs → SetActiveDeck` order for "Play with this deck" is correct and is carried to TASK-118.
3. **Case-insensitive name matching across SaveDeckAs/LoadDeck/SetActiveDeck (`ESearchCase::IgnoreCase`), storing as-entered trimmed casing — ACCEPTED.** Mirrors UE's default `FString ==`; avoids near-duplicate decks. Round-trip is safe: `SetActiveDeck` stores the canonical `Deck.DeckName`, so a case-sensitive reader still resolves it. See the TASK-114 carry-forward note below.
4. **RemoveCopy drops the entry at 0 (canonical one-entry-per-card) — ACCEPTED.** `IsDeckLegal` accepts either form; the canonical model keeps `IndexOfCard` unambiguous. No negative counts.
5. **SaveDeckAs has NO legality gate — ACCEPTED.** Matches M6 ruling 6 (§8 is a display-only guide; saving is never blocked; only "Play with this deck" gates on the exactly-50 `IsCurrentDeckLegal`). In-progress decks can be saved.
6. **AddCopy refuses silently at the cap / on a missing row, no broadcast — ACCEPTED.** Correct per the CONVENTIONS delegate law (never broadcast on a refused/no-op mutation). The WBP greys the "+"; this C++ cap is the authoritative data-driven backstop.
7. **`GetSavedDeckNames` BlueprintCallable, not Pure — ACCEPTED.** It reads the SaveGame from disk; a Pure node would re-hit disk on every graph evaluation. The other reads are legitimately Pure (in-memory working deck + cached DT_Cards). WBP should call once and cache (carried to TASK-118).
8. **`LoadSaveGame` guards with `DoesSaveGameExist` before `LoadGameFromSlot` — ACCEPTED.** Keeps the normal first-run (no `.sav` yet) SILENT; LoadDeck/SetActiveDeck on an empty store still log an intentional Warning on an explicit user action. Good null-safe pattern.
9. **`GetCollectionCardIDs` returns DT_Cards row-map (import/CSV) order, not sorted — ACCEPTED.** Deterministic and data-driven; if a specific browser ordering (by cost/type) is wanted, TASK-118's WBP sorts or the manager specifies it. Not a correctness issue.

---

## Notes for build-master (on PASS)

- Compiles as part of the TASK-117 batch (with TASK-114). Expect warnings-as-errors CLEAN: both mandatory scans pass; the `Slot`-shadow (M2 class) and the missing-complete-type-include (TASK-110 class) are both absent.
- No `.Build.cs` change needed — `UDataTable` (Engine/DataTable.h), `UTexture2D` (Engine/Texture2D.h), the `UGameplayStatics` SaveGame flow (Kismet/GameplayStatics.h), and `UUserWidget` (UMG) are all already project module deps (UMG is used by `UCardHandWidget`/`UUnitHealthBarWidget`).
- This is a model/logic class only — nothing spawns, ticks, or assembles into a scene. `WBP_DeckBuilder` reparenting is TASK-118 (needs `UDeckBuilderWidget` to exist first — the editor-bounce in TASK-117).

## Carry-forwards

**For TASK-118 (WBP_DeckBuilder authoring):**
- Bind to the surface above **character-for-character**; the additive resolvers (`GetCardDisplayName`/`GetCardCost`/`GetCardMaxCopies`/`GetCardArtTexture`) are the ONLY way the grid gets card data — do NOT read DT_Cards in the WBP.
- Seed-then-bind: Event Construct calls `LoadDefaultDeck()` (or `LoadDeck` of the last active) to SEED, then reads the getters. No delegate binding needed — mutations re-fire `OnDeckModelChanged()`; use `OnDeckSlotCountChanged` only as a single-cell optimization.
- Grey the "+" when `GetCountOf(CardID) >= GetCardMaxCopies(CardID)`. Enable "Play with this deck" only when `IsCurrentDeckLegal()`. Call `GetSavedDeckNames()` ONCE and cache (it hits disk).
- "Play with this deck" flow is ordered: `SaveDeckAs(name)` → `SetActiveDeck(name)` → `ASiegeGameMode::StartMatch` (SetActiveDeck is STRICT — persist first).

**For TASK-114 (in progress — cross-task note, NOT a TASK-116 defect):**
- TASK-116 stores the canonical saved `DeckName` into `ActiveDeckName`, so TASK-114's reader resolves it even with an exact-match lookup. For full parity with the deck-builder's case-insensitive keying, TASK-114's `ActiveDeckName → FDeckList` lookup SHOULD compare case-insensitively (`ESearchCase::IgnoreCase`), matching flag 3. Safe either way given the canonical storage; will be confirmed at TASK-114's review.
