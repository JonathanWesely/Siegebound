# TASK-079 handoff — CardArt data path: FCardRow.CardArt + cards.csv column + UCardHandWidget resolvers

- **Author:** gameplay-programmer
- **Date:** 2026-07-07
- **Status requested:** ready-for-qa (orchestrator flips the board)
- **Scope law honored:** file-only — no editor, no compile, no Git, no TASKBOARD edit.

## What changed (files touched)

1. `Source/GitClaudeUnrealTest/Siegebound/CardRow.h`
   - New `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card") TSoftObjectPtr<UTexture2D> CardArt;` appended at the END of `FCardRow` (after `Lifetime`), doc-commented per the CONVENTIONS "Card artwork (hand UI)" law (unset = text-only face, never a crash).
   - Added `#include "UObject/SoftObjectPtr.h"` + `class UTexture2D;` forward declaration (no heavy `Engine/Texture2D.h` include in the widely-included row header).
2. `Docs/Data/cards.csv`
   - Header: appended 23rd column `CardArt` (exact casing — matches the UPROPERTY name 1:1, reimport law).
   - ALL 22 rows: appended the full object path `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>`, `<CardID>` character-for-character from each row name. Zero blank cells. No other cell touched.
3. `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h` / `.cpp`
   - New `UFUNCTION(BlueprintCallable, Category="Siegebound|UI") UTexture2D* GetCardArtTexture(const FString& CardID)`.
   - New `UFUNCTION(BlueprintCallable, Category="Siegebound|UI") UTexture2D* GetNextCardArtTexture()`.
   - New private shared resolver `UTexture2D* ResolveCardArtTexture(FName CardID)` (both callables funnel here).
   - New private members: `FName LastNextCardID` (preview cache) + `TSet<FName> WarnedCardArtIDs` (once-per-CardID log guard).
   - `PushNextCardPreview` now caches `LastNextCardID = NextCardID` as its FIRST statement — before `OnNextCardUpdated` fires.
   - `#include "Engine/Texture2D.h"` added to the .cpp only (complete type needed for `LoadSynchronous`'s internal cast).
   - **The three BIEs are byte-identical** (ruling 3): `OnHandSlotUpdated(int32, const FString&, const FString&, int32, bool)`, `OnNextCardUpdated(const FString&, int32)`, `OnCardRefusedMessage(const FString&)` — untouched, zero param/name/order changes. `RequestPlaySlot` / `RequestDiscardSlot` / `InitForController` also untouched.

## Assets referenced (produced in parallel by TASK-077/078 — expected missing today)

- `/Game/UI/CardArt/T_CardArt_<CardID>` × 22 (soft paths in cards.csv only; a missing texture soft-loads to nullptr → text-only fallback + one warning, never a crash).
- `/Game/Data/DT_Cards` (existing; soft ref unchanged in the widget CDO).

## Resolver semantics (null-safe chain, by inspection)

`GetCardArtTexture(CardID)`:
1. Empty string → `nullptr`, **silent** (empty slot is the normal WARN-2 state; logging would fire 6×/refresh).
2. Literal `"None"` → `nullptr`, silent (defensive: the empty-slot signal is the EMPTY string per the TASK-029 contract; a real card can never be named None).
3. Missing table / missing row → `nullptr`; logging already owned by the pre-existing `ResolveCardRow` (once per widget / once per CardID).
4. Row found, `CardArt.IsNull()` (unset cell) → `nullptr`, log once per CardID.
5. `CardArt.LoadSynchronous()` returns null (unresolvable path, e.g. pre-TASK-078) → `nullptr`, log once per CardID.
6. Otherwise → the loaded `UTexture2D*`.

`GetNextCardArtTexture()`: `LastNextCardID.IsNone()` → `nullptr` silent; else same chain via the shared resolver.

## Flagged decisions for QA (explicit list — 8)

1. **`GetCardArtTexture` takes `const FString& CardID`, not `FName`.** The board spec's preferred seam names `const FString&`; the dispatch summary said `FName`. Board is authoritative, and FString is the right ergonomics: `OnHandSlotUpdated` delivers CardID to the BP as FString, so TASK-080 wires the pin straight through with no conversion node.
2. **Empty CardID returns nullptr with NO log.** Deliberate deviation from "log once" for this one input class: an empty slot is documented normal state and re-pushes on every gold tick; it is not a fault. Faults (missing row, unset cell, failed load) all log once.
3. **Literal "None" string treated as empty (silent nullptr).** Defensive guard consistent with the header's "never compare against the literal None" law; prevents a bogus once-per-CardID missing-row warning if a BP ever passes it.
4. **One `WarnedCardArtIDs` set covers both fault flavors** (unset cell AND failed load) — one art warning per CardID total. The two messages are distinct so the log still names which fault fired first. Missing-row warnings stay in the pre-existing `WarnedMissingRowIDs`.
5. **`LastNextCardID` is cached as the FIRST statement of `PushNextCardPreview`, including `NAME_None`.** Guarantees `GetNextCardArtTexture()` called inside the BP's `OnNextCardUpdated` handler always sees the card being pushed, and goes null in the empty-deck window so the preview art hides with the preview text.
6. **Resolvers are BlueprintCallable, NOT BlueprintPure.** Pure nodes re-evaluate per connected pin — each re-run would repeat `LoadSynchronous` + the warn-set check. Callable = exactly one resolution per handler invocation.
7. **`CardArt` appended at the END of `FCardRow`** (after the M4 spawner block), matching the appended CSV column position. The DataTable CSV importer maps by header NAME, not order, so this is cosmetic alignment — noted so nobody "fixes" a mismatch against the CONVENTIONS registry listing order (which slots CardArt after `bRanged`).
8. **`LoadSynchronous` used, per accepted ruling 4** — comment markers in both CardRow.h and CardHandWidget.cpp say so. No async streaming machinery.

Shadow-law self-scan (C4457/58/59): new identifiers `CardID` (params), `CardName`, `Row`, `CardTable` (pre-existing), `ArtTexture`, `LastNextCardID`, `WarnedCardArtIDs` — none shadow an inherited reflected UPROPERTY (`Slot`, `Owner`, `PlayerState`, `Controller` etc. unused). QA please re-scan independently.

## Note for TASK-081 (build-master) — DT_Cards reimport

- cards.csv now has **23 columns** (row-name col + 22 data cols → `CardArt` is the last header token). Header token `CardArt` matches the UPROPERTY character-for-character; `TSoftObjectPtr<UTexture2D>` imports cleanly from the plain full-object-path string form used in every cell (`/Game/UI/CardArt/T_CardArt_<X>.T_CardArt_<X>` — no `Texture2D'...'` wrapper needed).
- Reimport `/Game/Data/DT_Cards` from `Docs/Data/cards.csv` IMMEDIATELY after the phase-1 compile, BEFORE any PIE (TASK-031 WARN-2 law). Expected: **zero NEW warnings**. The known DeepMine CardType-2 warning is pre-existing (TASK-035 watch) — record if it fires, not new.
- The 22 texture assets need NOT exist at reimport time — soft-ptr cells import as paths without touching the assets; art lands via TASK-078 independently.

## Note for TASK-080 (WBP_CardHand UMG wiring) — the exact seam

- **Hand slots:** inside the `OnHandSlotUpdated` handler, call `GetCardArtTexture(CardID)` with the event's own `CardID` param (FString, straight wire). Non-null → `SetBrushFromTexture(Img_CardArt, Texture)` + show; **null → hide `Img_CardArt`** (text-only fallback). Because the event fires per slot per refresh and the resolver is idempotent, no extra caching is needed BP-side.
- **Preview:** inside the `OnNextCardUpdated` handler, call `GetNextCardArtTexture()` (no params — the C++ cache is guaranteed current at that point). Null (including the empty-DisplayName/no-next-card case) → hide `Img_NextCardArt`.
- Both calls are on the widget itself (`self`) — no controller/deck references needed in the graph.
- Repeated calls are cheap after first load (LoadSynchronous returns the already-loaded texture) and log-safe (once-per-CardID guards).

## What QA should scrutinize

- CSV: exact `<CardID>` casing in all 22 paths (character-for-character vs the row-name column); no cell count drift on any row (every row must now have 23 fields).
- Header/UPROPERTY 1:1 (`CardArt`).
- Null-safety chain order in `ResolveCardArtTexture` (row → IsNull → LoadSynchronous).
- BIE byte-identity: diff the three declarations vs main @ 218b4c9 — must be zero-delta.
- Shadow scan per CONVENTIONS.
