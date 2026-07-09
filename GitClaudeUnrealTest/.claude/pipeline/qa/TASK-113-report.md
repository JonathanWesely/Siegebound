# QA Report — TASK-113 (Deck data model + SaveGame + legality/avg-cost library)

**Verdict: PASS**  ·  Blockers: 0  ·  Warnings: 0  ·  Nits: 2
Reviewed 2026-07-09 (qa-reviewer, pre-compile). Files-only task; TASK-117 compiles the batch.
Scope: 5 NEW files under `Source/GitClaudeUnrealTest/Siegebound/` — `DeckTypes.h`, `DeckLibrary.h/.cpp`, `SiegeDeckSaveGame.h/.cpp`. Cross-read: `CardRow.h` (FCardRow.MaxCopies/.Cost/.DisplayName reuse). No existing file touched.

---

## Mandatory scans (both performed independently — not taken on faith from the handoff)

### (a) Inherited-reflected-member shadow scan (C4457/58/59) — CLEAN
- `UDeckLibrary : UBlueprintFunctionLibrary` and `USiegeDeckSaveGame : USaveGame` both descend from `UObject`, which carries no reflected `Owner`/`Instigator`/`PlayerState`/`Controller`/`Slot`/`Name`-class UPROPERTY to shadow.
- `FDeckCardEntry`/`FDeckList` are plain USTRUCTs (no inherited reflected members).
- Every local/param/loop var checked: `CardTable`, `Deck`, `OutReason`, `RunningCounts`, `Running`, `Entry`, `Row`, `Total`, `CostSum`, `Total` — none collide with an inherited reflected UPROPERTY. (`DeckName` is a struct member, not a shadow.) No hit.

### (b) Complete-type-include scan (C2027/C2227 — the class of miss that broke TASK-110) — CLEAN
- `DeckLibrary.cpp` dereferences `const UDataTable*` (`CardTable->FindRow<FCardRow>`) → **`#include "Engine/DataTable.h"` present (line 5).** ✓
- `DeckLibrary.cpp` dereferences `const FCardRow*` (`Row->MaxCopies`, `Row->Cost`) AND instantiates the `FindRow<FCardRow>` template (needs `FCardRow::StaticStruct`) → **`#include "Siegebound/CardRow.h"` present (line 6).** ✓  This is exactly the TASK-110 failure mode; it is NOT present here.
- `FDeckList`/`FDeckCardEntry`/`SiegeLegalDeckSize` reach complete type via `DeckLibrary.h` → `Siegebound/DeckTypes.h`. ✓
- `SiegeDeckSaveGame.cpp` only defines a `static const FString` — no upcast/deref beyond its own header. ✓
- Headers: `DeckLibrary.h` uses `UDataTable` only as a signature pointer → forward-declared `class UDataTable;` (line 10), and includes `DeckTypes.h` for the by-ref `FDeckList`. `SiegeDeckSaveGame.h` has a `TArray<FDeckList>` value member → requires the complete type → `DeckTypes.h` included (line 7). Every `*.generated.h` is the LAST include in its header (DeckTypes.h:6, DeckLibrary.h:8, SiegeDeckSaveGame.h:8). ✓

---

## Findings

- [NIT] DeckLibrary.cpp:51 — the "wrong total" reason string reads `"a legal deck is exactly %d (GDD 3.4)."` with an ASCII "3.4" (no `§`). This is a deliberate encoding-safety choice (flag 2) for a user-facing/log string and is fine — noting only so build-master isn't surprised the glyph is absent. No change required.
- [NIT] DeckTypes.h:70 — `static constexpr int32 SiegeLegalDeckSize = 50;` at namespace scope gives internal linkage (a distinct copy per TU that includes the header). This is exactly what the spec asked for and is ODR-safe; it is only ever read by value, never ODR-used by address, so no linker concern for TASK-114's `UDeckComponent` consumption. No change required.

No BLOCKER and no WARN findings.

---

## Correctness verification (traced, not assumed)

**`IsDeckLegal(const UDataTable* CardTable, const FDeckList& Deck, FString& OutReason)`** — CORRECT
- Null table → `OutReason` set, returns `false` before any deref (lines 10–15). ✓ Never crashes on null.
- Per entry, in array order: unknown CardID (`FindRow` null, `bWarnIfRowMissing=false`) → `false`+reason; negative `Count` → `false`+reason; running per-CardID aggregate over `MaxCopies` → `false`+reason. ✓ First-violation order is deterministic (entry order, then a final total check).
- **Cap boundary is correct: `if (Running > Row->MaxCopies)` — `Count == MaxCopies` yields `Running == MaxCopies`, which is NOT `>`, so it PASSES.** No off-by-one. ✓
- Total check runs after the loop: `Deck.TotalCount() != SiegeLegalDeckSize` → `false`+reason. Because negative counts are rejected earlier in the loop, `TotalCount()` is only reached with all counts ≥ 0 (no negative-masking). ✓
- Success path calls `OutReason.Reset()` then `return true` (spec: cleared on success). ✓
- `int32& Running = RunningCounts.FindOrAdd(...)` reference is used within the same iteration only (no further map insert before use) — no dangling-reference/rehash hazard. ✓
- MaxCopies==0 data case (a card not meant for decks) correctly rejects any Count>0 entry; a Count==0 entry is a harmless empty slot. Data-driven, no hardcoded stat. ✓

**`GetDeckAverageCost(const UDataTable* CardTable, const FDeckList& Deck)`** — CORRECT
- **Divide-by-zero guarded:** `Total = TotalCount(); if (!CardTable || Total <= 0) return 0.0f;` (lines 61–66). Empty deck → `Total==0` → `0.0f`, never NaN, never `/0`. ✓
- Numerator `sum(Row->Cost * Entry.Count)` skips `Count<=0` entries and null rows (unresolvable CardID contributes 0); denominator stays `TotalCount()` per the literal §8 formula (flag 4). Int product/sum then `static_cast<float>` division — exact, no overflow at 50-card scale. ✓ Never hardcodes Cost (reads `FCardRow.Cost`). ✓

**`USiegeDeckSaveGame`** — CORRECT + persistence VERIFIED against engine source
- `USaveGame` subclass; `SavedDecks` (`TArray<FDeckList>`) and `ActiveDeckName` (`FString`) are `UPROPERTY`; nested `FDeckList::DeckName`/`::Cards` and `FDeckCardEntry::CardID`/`::Count` are ALL `UPROPERTY`. ✓
- **The "silent data loss" trap is genuinely avoided.** I confirmed against `UE_5.8/.../Private/GameplayStatics.cpp` (`SaveGameToMemory`, line 2381) that the save archive is `FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false)` with `ArIsSaveGame` left FALSE. Therefore the `CPF_SaveGame` property filter does NOT apply and every non-transient UPROPERTY serializes — plain `UPROPERTY(BlueprintReadWrite)` is sufficient and the nested structs round-trip. The `SaveGame` UPROPERTY specifier is NOT required here. (Load path line 2484 is symmetric.) No demotion of these members — they must stay UPROPERTY.
- `SlotName` = `static const FString`, single out-of-line definition in the .cpp (`TEXT("SiegeDecks")`, line 7) — one source of truth, no duplicated literal for TASK-114/116. `UserIndex` = `static constexpr int32 = 0`, shared const. ✓

**USTRUCT correctness** — `FDeckCardEntry`/`FDeckList` are `USTRUCT(BlueprintType)` with `GENERATED_BODY`; `EditAnywhere, BlueprintReadWrite` on data members (justified by flag 5 for TASK-114's EditDefaultsOnly `BotDecks`); `TotalCount()` is a non-reflected const helper (legal in a USTRUCT). `GITCLAUDEUNREALTEST_API` on the structs matches the `FCardRow` precedent. ✓

**API currency (UE 5.8)** — `FindRow`, `TMap::FindOrAdd`, `FString::Printf`, `TArray`, `USaveGame`, `UBlueprintFunctionLibrary` all current; nothing deprecated/removed. Includes `Kismet/BlueprintFunctionLibrary.h` (line 6) and `GameFramework/SaveGame.h` (line 6) correctly present. ✓

---

## Rulings on the programmer's flagged decisions

1. **Copy-cap enforced on the per-CardID running AGGREGATE, not literally per-entry — ACCEPTED.** This matches spec ruling 5 ("copy-cap + exactly-50 enforced data-driven" via the ONE shared `IsDeckLegal`) and the design intent of `MaxCopies` ("Maximum copies of this card allowed in a deck", CardRow.h:79). The aggregate is a strict STRENGTHENING of the literal per-entry wording: it rejects every deck a per-entry check would reject, PLUS a save-loaded/bot/hand-authored deck that splits one card across duplicate entries to evade the cap. For the widget's one-entry-per-card deck (TASK-116) the two are identical. Never wrongly passes. This is the correct interpretation, not merely an acceptable one.
2. **Deterministic first-violation order (unknown → negative → over-cap → wrong-total, in entry order); ASCII reason strings — ACCEPTED.** Satisfies "OutReason = first violation"; strings are HUD/log-ready; the ASCII "GDD 3.4" avoids source-encoding risk in a user-facing string (see NIT above).
3. **Library never logs (pure/stateless validator) — ACCEPTED.** Correct for a function the widget may call every model change; callers own messaging via `OutReason`. Consistent with `USpellLibrary`.
4. **`GetDeckAverageCost` denominator = `TotalCount()`; returns `0.0f` for empty/degenerate — ACCEPTED** per spec ruling 6 (display-only §8 guide, no hard rule). Only diverges from a valid-rows-only denominator on illegal decks, which the guide never gates.
5. **Struct members `EditAnywhere, BlueprintReadWrite`; members remain UPROPERTY by necessity — ACCEPTED.** `EditAnywhere` is required for TASK-114's `EditDefaultsOnly BotDecks` to be tunable in the details panel; UPROPERTY is required for SaveGame round-trip (verified above). Do NOT demote.
6. **`SlotName` out-of-line `static const FString`; `UserIndex`/`SiegeLegalDeckSize` `static constexpr` — ACCEPTED.** ODR-safe idioms; single definition for the slot name.

---

## Contract confirmation for downstream (names are law)

Every public signature matches CONVENTIONS "Deck-builder & saved decks (M6)" and the TASK-113 `names:` block character-for-character. No names-block or signature deviation → **TASK-114 and TASK-116 are cleared to build against these types as written:**
- `FDeckCardEntry { FName CardID; int32 Count }` ✓
- `FDeckList { FString DeckName; TArray<FDeckCardEntry> Cards; int32 TotalCount() const }` ✓
- `static constexpr int32 SiegeLegalDeckSize = 50` ✓
- `UDeckLibrary::IsDeckLegal(const UDataTable*, const FDeckList&, FString&)` ✓
- `UDeckLibrary::GetDeckAverageCost(const UDataTable*, const FDeckList&)` ✓
- `USiegeDeckSaveGame : USaveGame` — `TArray<FDeckList> SavedDecks`, `FString ActiveDeckName`, `static const FString SlotName == "SiegeDecks"`, `static constexpr int32 UserIndex == 0` ✓

---

## Notes for build-master (on PASS)

- Compiles as part of the TASK-117 batch. Expect warnings-as-errors clean: both mandatory scans pass, `Engine/DataTable.h` + `CardRow.h` are present in DeckLibrary.cpp (the TASK-110 include miss is NOT repeated).
- No `.Build.cs` change needed — `UDataTable` (Engine/DataTable.h) and `USaveGame` (GameFramework/SaveGame.h) are Engine-module types already depended on.
- These are library/data types only; nothing spawns or ticks. No scene/asset assembly.

## Carry-forwards for TASK-114 / TASK-116 (QA will enforce at their review)

- **TASK-114/116 must NOT re-literal the slot** — use `USiegeDeckSaveGame::SlotName` and `USiegeDeckSaveGame::UserIndex` (the single-source consts), never `TEXT("SiegeDecks")`/`0` again.
- **TASK-114 load path must null-check the cast:** `Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(...))` returns nullptr when no file exists → fall back to the curated DeckCount default (per its spec). Any consumer calling `UGameplayStatics` must `#include "Kismet/GameplayStatics.h"` (complete-type law).
- **TASK-116 widget** wraps `IsDeckLegal`/`GetDeckAverageCost` in BlueprintPure/Callable; it must supply the `DT_Cards` `UDataTable*` and, if it dereferences `FCardRow`, include `Engine/DataTable.h` + `Siegebound/CardRow.h` in its .cpp.
- **Aggregate-cap consequence:** TASK-115's curated DeckCount and TASK-114's two BotDecks must each keep per-CardID totals ≤ MaxCopies and sum == 50, or `IsDeckLegal` correctly rejects them (falling back to DeckCount). Not a TASK-113 issue — flagged so the downstream data/decks are authored to pass this validator.
