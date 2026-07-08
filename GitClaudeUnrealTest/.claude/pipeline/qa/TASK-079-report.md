# QA Report — TASK-079 (Card-art data path: FCardRow.CardArt + cards.csv + UCardHandWidget resolvers)

**Verdict: PASS**
**Counts: 0 BLOCKER / 1 WARN / 1 NIT**
Reviewed pre-compile, 2026-07-07. Files: `Source/GitClaudeUnrealTest/Siegebound/CardRow.h`, `CardHandWidget.h`, `CardHandWidget.cpp`, `Docs/Data/cards.csv`. Ruleset: TASKBOARD card-art manager rulings 1–9, CONVENTIONS "Card artwork (hand UI)" + FCardRow column registry + shadow law, UE 5.8 API validity.

## Findings

- **[WARN] handoffs/TASK-079.md ("Note for TASK-081") — column-count arithmetic is off by one.** The note says "23 columns (row-name col + 22 data cols)". Actual post-change shape: **23 NAMED data columns** (DisplayName … Lifetime, CardArt = 23rd, matching FCardRow's 23 UPROPERTYs 1:1) **plus the unnamed row-name column = 24 comma-separated fields per line** (verified by field count on header + spot rows). "Row-name + 22 data" describes the PRE-079 file. No code impact; corrected here so TASK-081 verifies against the right numbers. The load-bearing claims (CardArt is the last header token; header ↔ UPROPERTY 1:1) are correct.
- **[NIT] CardHandWidget.cpp:269 — `FName CardName(*CardID)` interns arbitrary BP-supplied strings into the global name table.** Input domain is closed in practice (TASK-080 wires the BIE's own CardID param, which round-trips from FName), so growth is bounded; noting only so the assumption is on record. No change requested.

No BLOCKERs.

## Verification detail

**CardRow.h**
- `CardArt` UPROPERTY spelling matches the CSV header token character-for-character (reimport maps by name). Specifiers `EditAnywhere, BlueprintReadOnly, Category="Card"` consistent with sibling rows; `TSoftObjectPtr<UTexture2D>` is UHT-legal in a BlueprintType USTRUCT and mirrors the existing `CardTableAsset` style.
- `#include "UObject/SoftObjectPtr.h"` + `class UTexture2D;` forward decl — correct; no heavy `Engine/Texture2D.h` in the widely-included row header. Doc comment carries the fallback + ruling-4 markers as spec'd.

**cards.csv**
- Header ↔ UPROPERTY 1:1 (23 named data columns ↔ 23 FCardRow UPROPERTYs; `CardArt` last).
- All 22 rows carry the CardArt cell, path form exactly `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>`, `<CardID>` casing verified character-for-character against every row name (incl. ArrowTower, MilitiaMob, BombTower, BallistaTower, DeepMine, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner). Zero blank cells; plain path form (no `Texture2D'...'` wrapper) — correct for soft-ptr CSV import.
- No other cell disturbed — spot-checked against known spec values: Footman 3g/80HP/12dmg/DeckCount 6; Archer 700 range/bRanged true; ArrowTower 900/1.5s; Barracks Footman/8/60; BallistaTower MinRange 300; Sapper bSuicide/AoE 250; MilitiaMob SwarmCount 4. DeckCount still sums to exactly 50 (deck law). No commas inside Notes cells (no CSV quoting hazard); every line has 24 fields.

**CardHandWidget.h/.cpp**
- Null-safety chain complete on EVERY path: empty CardID → silent nullptr (cpp:261); literal "None" → silent nullptr (cpp:270); missing table → nullptr, once-per-widget log via `bWarnedMissingTable` (pre-existing, cpp:344); missing row → nullptr, once-per-CardID via `WarnedMissingRowIDs` (pre-existing, cpp:359); unset `CardArt.IsNull()` → nullptr, once-per-CardID log (cpp:304–317); failed `LoadSynchronous` → nullptr, once-per-CardID log (cpp:322–336). Never dereferences a null; never crashes (ruling 2 satisfied).
- Once-per-CardID logic verified: `Contains` check → `Add` → log, in both fault branches, sharing `WarnedCardArtIDs` — exactly one art warning per CardID for the widget's lifetime.
- **`LastNextCardID` cache ordering VERIFIED:** set as the first statement of `PushNextCardPreview` (cpp:214), before BOTH `OnNextCardUpdated` broadcast sites (cpp:220 empty branch, cpp:233 normal branch) — a BP handler calling `GetNextCardArtTexture()` always reads the card being pushed; NAME_None is cached in the empty-deck window so preview art hides with the preview text.
- **BIE byte-identity VERIFIED** against the TASK-029 handoff's verbatim declarations and the TASK-033/041 wiring records: `OnHandSlotUpdated(int32 SlotIndex, const FString& CardID, const FString& DisplayName, int32 Cost, bool bAffordable)` (h:123), `OnNextCardUpdated(const FString& DisplayName, int32 Cost)` (h:132), `OnCardRefusedMessage(const FString& Reason)` (h:142) — zero delta. `RequestPlaySlot` / `RequestDiscardSlot` / `InitForController` untouched. Ruling 3 satisfied; no version bump attempted.
- `Engine/Texture2D.h` included in the .cpp only — correct (complete type for LoadSynchronous); header uses the forward decl for the `UTexture2D*` UFUNCTION returns (UHT-fine).
- GC-safety: new members are `TSet<FName>` + `FName` (no UPROPERTY needed, no UObject refs); returned raw texture pointer is consumed same-frame by the BP brush — standard, safe.
- Shadow-law scan (C4457/58/59), independent: new identifiers `CardID` (params), `CardName`, `Row`, `CardTable`, `ArtTexture`, `LastNextCardID`, `WarnedCardArtIDs` — no collision with inherited reflected UPROPERTYs (`Slot`, `ColorAndOpacity`, `Padding`, `bIsFocusable`, `Visibility`, …) nor with own members (`CardTable` local ≠ `CardTableAsset` member). CLEAN.
- Deprecated-API scan (UE 5.8): `LoadSynchronous`, `FindRow`, `AddUniqueDynamic`/`RemoveDynamic`, `GetPlayerState<T>`, `FindComponentByClass` — all current. CLEAN.
- Performance: resolver work only on hand-refresh/handler invocation, no per-tick work; repeat `LoadSynchronous` on an already-loaded asset is a fast resolve. Within ruling 4.
- Naming/conventions: `GetCardArtTexture` / `GetNextCardArtTexture` match the board's names section; category `Siegebound|UI` consistent; `CardArt` referenced ONLY in the three touched source files (grep-verified); no hardcoded stat that belongs in DT_Cards (the only literal path is the pre-existing DT_Cards CDO default).

## Rulings on the 8 flagged decisions (handoffs/TASK-079.md)

1. **`const FString& CardID` param, not FName — ACCEPTED.** The board spec (authoritative) names `const FString&`; CONVENTIONS "Resolution seam" agrees; the BIE delivers CardID as FString so TASK-080 wires the pin with zero conversion nodes.
2. **Empty CardID → silent nullptr — ACCEPTED.** "Log once" (ruling 2) governs FAULTS (missing row / unset cell / failed load — all log). An empty slot is the documented NORMAL state (header contract, WARN-2 window) re-pushed on every gold tick; logging it would be spam, not signal.
3. **Literal "None" treated as empty, silent — ACCEPTED.** Consistent with the TASK-029 contract ("the empty-slot signal is the EMPTY string — never compare against the literal None"); a real CardID can never be None; prevents a bogus once-per-CardID missing-row warning.
4. **Single `WarnedCardArtIDs` set for both art-fault flavors — ACCEPTED.** One art warning per CardID satisfies "log once"; the two message texts are distinct so the log names the fault that fired; row-level warnings remain separately guarded in `WarnedMissingRowIDs`.
5. **`LastNextCardID` cached first, including NAME_None — ACCEPTED** (and code-verified, see above). This is exactly what makes the pull-seam sound without a BIE param.
6. **BlueprintCallable, NOT BlueprintPure — ACCEPTED.** Pure nodes re-evaluate per connected pin (would repeat LoadSynchronous + set checks); the board spec itself says BlueprintCallable. Correct call.
7. **`CardArt` appended at the END of FCardRow — ACCEPTED.** The DataTable CSV importer maps by header NAME, not position; struct-end + CSV-end alignment is cosmetic and tidy. The CONVENTIONS registry lists CardArt after `bRanged` as documentation order only — nobody should "fix" this. On record.
8. **LoadSynchronous — ACCEPTED** per manager ruling 4 (512² UI textures, ≤7 visible, loaded on hand refresh). Comment markers present in CardRow.h (doc comment), CardHandWidget.h:156–157, CardHandWidget.cpp:319–321 as required.

## Carry-forward — TASK-080 (WBP_CardHand UMG)

- Seam is exactly as the handoff documents: inside the `OnHandSlotUpdated` handler call `GetCardArtTexture(CardID)` with the event's own CardID pin (straight wire); non-null → `SetBrushFromTexture` + show `Img_CardArt`, null → hide (text-only fallback). Inside the `OnNextCardUpdated` handler call `GetNextCardArtTexture()` (no params — C++ cache guaranteed current); null → hide `Img_NextCardArt`. Both calls on `self`.
- Do NOT add BP-side caching or empty/"None" pre-checks — the resolvers already handle both silently; null is the single "hide art" signal.
- Resolvers are Callable (exec pins), not Pure — place them in the handler execution chain once per event.

## Notes for build-master — TASK-081

- **Phase-1 reimport expectations (corrected arithmetic):** cards.csv = 23 NAMED data columns (↔ 23 FCardRow UPROPERTYs 1:1, `CardArt` last) + the unnamed row-name column = 24 fields per line. Expect **zero NEW warnings**; the known DeepMine CardType-2 warning is pre-existing (TASK-035 watch) — record if it fires, do not treat as new.
- The 22 `T_CardArt_*` assets need NOT exist at reimport time — soft-ptr cells import as bare paths. HOWEVER: if any PIE runs after the reimport but BEFORE TASK-078's import lands, up to 22 once-per-CardID "CardArt ... failed to load" warnings are EXPECTED and benign (they are the designed fallback logs, not reimport faults). Board sequencing (phase-2 PIE after 078+080) should make this moot — noting in case of an interim smoke PIE.
- No new module dependencies; no Build.cs change needed (UMG/Engine already referenced).
