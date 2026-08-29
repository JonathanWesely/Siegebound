# TASK-676 — THE UNCAP CODE WAVE — programmer handoff (2026-08-28)

Implements CONVENTIONS **UNCAP-§1..§7** exactly. Per-card copy caps abolished; the 50-card
deck total stays **EXACT** (U5 pin — never <=50). ⛔ Not compiled here (QUIET-MODULE — TASK-678
owns the one shared gate). ⛔ No git, no board-graph edits beyond my own status flip, no editor.

## Suite declaration for the TASK-678 gate

**Baseline 136 + 4 new `Siegebound.Deck.Uncap*` cases = EXPECTED 140/140.**
(`SiegeDeckSlotsTest.cpp` now holds 12 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros: 8 pre-existing + 4 new.)

## Per-site diff table (the UNCAP-§7 owned map — and nothing else)

| # | File | Site | Change | Kind |
|---|------|------|--------|------|
| 1 | `Source/GitClaudeUnrealTest/Siegebound/DeckLibrary.cpp` | `IsDeckLegal` | DELETED the per-CardID aggregate MaxCopies check, the `RunningCounts` map (+ its `Reserve`), and the `"the cap is %d (MaxCopies)"` reason. KEPT: null-table refusal, unknown-CardID refusal, negative-Count refusal, `TotalCount() == SiegeLegalDeckSize` EXACT check. The per-entry `FindRow` survives as the unknown-CardID gate. Signature byte-identical. | logic |
| 2 | `Source/GitClaudeUnrealTest/Siegebound/DeckLibrary.h` | class doc + `IsDeckLegal` doc | Doc truth: rules are row-existence/.Cost-driven; copy-cap paragraph replaced with the dated UNCAP-§1/§3 amendment ("strictly WIDENING"). No declaration changed. | comment |
| 3 | `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | `AddCopy` | DELETED the `Current >= Row->MaxCopies` refusal block. KEPT the `!Row` (missing table/row) refusal and the `CardID.IsNone()` guard. ⛔ NO add-time total guard added (U4). Success path (broadcasts + `PersistWorkingDeck`) untouched. | logic |
| 4 | `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | `GetCardMaxCopies` | THE UNCAP-§4 COMPAT SHIM: `return Row ? SiegeLegalDeckSize : 0;` (was `Row->MaxCopies`). Resolved row ⇒ 50; missing table/row ⇒ 0 (unchanged). Signature byte-identical (BlueprintPure). | logic |
| 5 | `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | `AppendIdentityLines` | DELETED the `Row.MaxCopies > 0` block appending `"Max %d per deck"` (UNCAP-§5 truth law). Identity line is now `"<Type> · Cost <n> gold"`. `IdentityLine` became `const`. | logic |
| 6 | `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | `IdentitySeparator` doc (~226) | Comment truth: example string no longer shows the Max-per-deck clause. | comment |
| 7 | `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` | ~20 / ~123 / ~163 / ~176 / ~184 / ~214 / ~328(now 343) / ~398 | Stale-comment sweep, all dated CARD-UNCAP 2026-08-28: class doc, `AddCopy` contract, `IsCurrentDeckLegal` ("cap-respecting" gone), the resolver-block preamble, the `GetCardMaxCopies` shim doc, the `GetCardDescription` composition (identity line clause noted deleted), a `CardTableAsset` rider (MaxCopies = stack cap only), `AppendIdentityLines` one-liner. No declaration/UFUNCTION line changed (diff-proven, see below). | comment |
| 8 | `Source/GitClaudeUnrealTest/Siegebound/DeckTypes.h` | ~11 / ~28 / ~37 | The three `[0..MaxCopies]` claims dropped, replaced with Count >= 0 + exactly-50, dated. Structs/constant untouched. | comment |
| 9 | `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` | `MaxCopies` UPROPERTY (~109) | Comment → hero-upgrade STACK CAP only, dated CARD-UNCAP 2026-08-28, UNCAP-§2; column kept, data untouched (U6). UPROPERTY line itself unchanged. | comment |
| 10 | `Source/GitClaudeUnrealTest/Siegebound/DeckComponent.cpp` | ~46 | Comment-only rider on the draw-pile guarantee ([0..MaxCopies] claim dropped). **ZERO logic diff — verified: every changed diff line is a `//` line.** | comment |
| 11 | `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` | ~218 + ~242 | Comment-only riders: bot-deck legality = sum==50 (old caps recorded as historical sizing); the 2026-08-15 rot-record sentence now dates the cap clause. **ZERO logic diff — verified as above.** | comment |
| 12 | `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp` | header + utils + 4 new macros | See tests section. | tests |

## The 4 new tests (UNCAP-§7 tests law — transient table, ⛔ never the shipped DT_Cards)

New utils in `SiegeDeckSlotsTestUtils`: `MakeScratchCardTable()` (`NewObject<UDataTable>` +
`RowStruct = FCardRow::StaticStruct()` — the in-file EmptySlotIllegal idiom), `AddScratchCard()`
(runtime `UDataTable::AddRow` — verified NOT WITH_EDITOR-guarded in the UE 5.8 engine header,
`DataTable.h:315` — so commandlet-safe), `AddDeckEntry()`.

1. **`Siegebound.Deck.UncapFiftyOfOneCardLegal`** — row Footman with `MaxCopies=12` (the OLD cap,
   deliberately present to prove it is IGNORED): 50×-one-card LEGAL, reason cleared; PLUS the same
   50 split 25+25 across duplicate entries of the same CardID LEGAL (proves the AGGREGATE
   RunningCounts path is gone, not just relaxed).
2. **`Siegebound.Deck.UncapExactFiftyPinned`** — 51 total ILLEGAL with the exact-50 reason
   (`Contains("exactly 50")`), AND 49 total ILLEGAL with the same reason — both directions of the
   U5 EXACT pin.
3. **`Siegebound.Deck.UncapUnknownCardStillIllegal`** — 50 Footman + unknown CardID at Count 0
   (total stays exactly 50, isolating the clause) ⇒ ILLEGAL, `Contains("Unknown card")`.
4. **`Siegebound.Deck.UncapNegativeCountStillIllegal`** — Footman 55 + Archer −5 (sums to exactly
   50, isolating the clause) ⇒ ILLEGAL, `Contains("negative copy count")`.

Zero network, zero disk, zero UWorld/PIE in all four. M8: the wave adds no replicated property,
no new replicated class, no new relevancy tier, no RPC.

## Shim caller census (`GetCardMaxCopies`)

Grep across `Source/`: the ONLY hits are the `.h` declaration and the `.cpp` definition — **zero
C++ call sites**. The caller is the `WBP_DeckCardTile` GRAPH ("+"-grey when `GetCountOf >= this`),
which is exactly what the shim spares: pinned BlueprintPure signature, now receiving 50 for any
resolved row, 0 for missing table/row (unchanged). Accepted consequence per UNCAP-§4: the "+"
greys only at 50-of-one-card. ⛔ Zero WBP/.uasset edits in this wave.

`IsDeckLegal` callers (all unchanged, change is strictly widening): `DeckBuilderWidget.cpp:671`,
`DeckComponent.cpp:42`, `SiegeBotController.cpp:309`, `SiegePlayerController.cpp:277`, tests.

## HeroCharacter zero-diff proof

`git diff --stat -- .../HeroCharacter.h .../HeroCharacter.cpp` ⇒ **empty output** (also absent
from the full `git diff --stat -- Source/`). `AHeroCharacter::GetStackCapForUpgrade`'s
`return Row->MaxCopies;` (HeroCharacter.cpp:958) and the whole stack-cap lane are byte-untouched
(U1). Also zero diffs under `Content/` and `Docs/Data/cards.csv` (U6).
Note: `git status` additionally shows `SessionMenuWidget.{h,cpp}` modified — that is **TASK-680's
parallel lane**, not mine; I touched neither file.

## MaxCopies census (every surviving `Source/` hit, ruled)

| Hits | Ruling |
|------|--------|
| `HeroCharacter.h` ×10, `HeroCharacter.cpp` ×5 | STACK-CAP LANE — fenced, untouched by law (UNCAP-§2/U1). |
| `CardRow.h:117` (the UPROPERTY) | Column survives per U6; its comment now states stack-cap-only. UPDATED. |
| `DeckLibrary.{h,cpp}`, `DeckBuilderWidget.{h,cpp}`, `DeckTypes.h`, `DeckComponent.cpp`, `SiegeBotController.cpp`, tests | All hits are either the stack-cap lane (`UpgradeTailFmt` doc at DeckBuilderWidget.cpp:126, the AppendRuleLines interpolation at :1250-1252 — UNTOUCHED, still true per UNCAP-§5) or dated CARD-UNCAP comments. UPDATED/RULED. |
| `DeckBuilderWidget.cpp:268` (ctor: "card stat (MaxCopies/Cost/DisplayName/DeckCount) is read from rows here") | RULED still-true, untouched: it lists which columns this class reads (MaxCopies IS still read — for the upgrade stack tail); it makes no deck-cap claim. |
| `SiegeBotController.h:203` ("Both defaults are legal (sum 50, each Count <= that card's MaxCopies)") | **RULED, NOT TOUCHED — the file is OUTSIDE the UNCAP-§7 owned map** (only `SiegeBotController.cpp` is owned, and QA criterion (a) makes any extra file a finding). The sentence stays literally true as a composition fact (both shipped decks do satisfy both properties); it does not claim the validator enforces the cap. FLAG for the next wave that owns that header: a one-line dated rider would be cleaner. |

## Deviations declared (SC-§15)

1. **"The shim returns 50" as a TEST** (the dispatch prompt's parenthetical): NOT added as a
   5th case. The board spec + UNCAP-§7 tests law pin exactly the four legality cases, and a shim
   unit test would require instantiating `UDeckBuilderWidget`, whose `CardTableAsset` is a
   protected soft-ptr defaulting to the SHIPPED `/Game/Data/DT_Cards` — resolving it in a test
   violates the ⛔ never-the-shipped-asset law, and the widget has no table-override seam to add
   without a signature deviation. The shim's return is one grep-able line
   (`return Row ? SiegeLegalDeckSize : 0;`) — QA criterion (b) covers it at review, TASK-678
   step (2) covers it live.
2. **Null-table reason STRING kept byte-identical** (`"No card table (DT_Cards) supplied."`):
   the spec's "update the null-table reason wording" was satisfied at the stale part — the
   comment above it (which claimed MaxCopies resolution); the player-facing string itself makes
   no cap claim and other code/tests treat it opaquely (Contains-free), so churn was refused.
3. `IdentitySeparator` doc (~cpp:226) and `IsCurrentDeckLegal`/resolver-block docs (.h ~163/~176)
   were stale-swept beyond the spec's four listed `.h` line numbers — same truth-law grounds,
   same owned files.

## QA scrutiny list (TASK-677)

- Criterion (b): diff `-U0` over both headers shows zero changed declaration/UFUNCTION lines —
  re-verify independently.
- Criterion (e): both rider files' diffs are 100% `//` lines — re-verify independently.
- The `AddCopy` diff: confirm the `!Row` refusal and the broadcast-then-persist success path are
  byte-order-identical to before (only the cap block between them is gone).
- Test 3 and 4 deliberately hold decks whose TOTAL is exactly 50 while violating one clause —
  confirm the isolation logic reads right.
- `UDataTable::AddRow` runtime availability: verified against the engine header at
  `C:\Program Files\Epic Games\UE_5.8\...\DataTable.h:315` (outside every WITH_EDITOR block).
- The suite arithmetic: 136 + 4 = **140** for the 678 gate.
