# TASK-1026 — [MAXCOPIES-DIAG] — gameplay-programmer handoff

**Date:** 2026-09-04 · **Mode:** ⛔ DIAGNOSE-ONLY · **Source edits: 0** · **Suite delta: 0 / 0**
**Sole file written:** this handoff (+ TASK-1026's own `status:` line on the board).
**Inputs read:** `.claude/pipeline/footage/VID-006-fog-brightsun-paid-and-resolved-but-silent.md` · `.claude/pipeline/qa/TASK-850.md` · `CONVENTIONS.md` `UNCAP-§1..§7` · `Docs/Data/cards.csv` · the source files named below.

---

## 0. THE ANSWER IN ONE LINE

> ⛔ **`MaxCopies` is (2) PARTIALLY LIVE.** It binds in **exactly one lane — the hero-upgrade stack cap — and on exactly four of the thirty-four card rows.** On the other **30 rows it is inert data: no code path anywhere reads its value into a comparison.** ⛔ **`Witch` is one of the 30.**
>
> ⇒ ⛔ **NOT A DEFECT.** The three Witches are `UNCAP-§3` behaving exactly as Jonathan ruled.

I started from the ruling and tried to refute it. **The refutation failed, and it failed in the strongest possible direction:** the three Witches are not merely *permitted* by `CARD-UNCAP` — they are **only reachable because of it** (§3 below).

---

## 1. THE CLASSIFICATION — and why it is (2), not (1) or (3)

The row offered three boxes. Two are wrong, and it matters which way:

| | Verdict | Why not |
|---|---|---|
| (1) **VESTIGIAL** | ⛔ **REJECTED** | A vestigial column would be read nowhere that matters. `AHeroCharacter::ApplyUpgrade` reads it and **refuses a play with it** (`EHeroUpgradeResult::RefusedAtMaxStacks`). Deleting the column would change shipped behaviour. |
| (2) **PARTIALLY LIVE** | ✅ **THIS ONE** | Enforced in the hero-upgrade lane; unenforced in deck-building, draw and hand-refill. Every lane named in §2. |
| (3) **LIVE BUT UNENFORCED** | ⛔ **REJECTED** | This would need a read that *cannot* bind. It **can**: the four `HeroUpgrade` rows carry `MaxCopies` `2/2/1/1` — all `> 0`, so `StackCap > 0` passes and the `GetUpgradeStackCount() >= StackCap` guard **genuinely fails** on the 3rd (or 2nd) stack. The guard is reachable and the value decides it. |

⚠️ **The evidence separates all three cleanly. I am not hedging between them.**

**But (2) understates it in a way Jonathan should hear.** The usual reading of "partially live" is *by lane*. Here the split is also **by row type**, and that is the bigger finding:

> ⛔ **`MaxCopies` is inert on 30 of 34 rows** — every `Unit`, `Building`, `Economy`, `Utility` and `Spell` row. It is live on 4: `SharpenedBlade` · `PlateArmor` · `SwiftBoots` · `WarBanner`.

The mechanism is a **name gate that runs before the cap is ever fetched**. `ApplyUpgrade` (`HeroCharacter.cpp`, `bKnownUpgrade`) compares the incoming `CardID` against the four hard `FName` constants and returns `RefusedInvalidCard` **before** calling `GetStackCapForUpgrade`. A `Witch`, `Fog` or `Footman` CardID can never reach the only line in the codebase that reads the column.

⇒ **The `TASK-850`/`UNCAP-§2` lead was correct and is now generalised.** It found `MaxCopies` inert on **Spell** rows. It is inert on **Unit** rows too — and on every other non-upgrade type. ⛔ **The deck copy cap is unenforced game-wide.** What is *not* true is the stronger phrasing "the cap is unenforced game-wide" without qualification: the **hero-upgrade stack cap** — the column's one surviving meaning under `UNCAP-§2` — is alive and working. Both halves of that sentence are load-bearing.

---

## 2. THE CONSUMER TABLE — every site, located by SYMBOL

### 2a. Sites that READ THE VALUE (`Row->MaxCopies` / `Row.MaxCopies`) — there are **three**

| # | Symbol | File | Lane | Binds? |
|---|---|---|---|---|
| **C1** | `AHeroCharacter::GetStackCapForUpgrade` → `return Row->MaxCopies;` | `Siegebound/HeroCharacter.cpp` | hero-upgrade **STACK CAP** | ✅ **YES — the only binding read in the project** |
| **C2** | `AHeroCharacter::GetUpgradeStackCap` → `return GetStackCapForUpgrade(...)` | `Siegebound/HeroCharacter.cpp` | HUD **pip cap** (TASK-064) | display; truthful, non-binding |
| **C3** | `UDeckBuilderWidget::AppendRuleLines` → `Printf(UpgradeTailFmt, Row.MaxCopies)` | `Siegebound/DeckBuilderWidget.cpp` | card-details **RULES tail** | display only, and **gated to `Row.CardType == ECardType::HeroUpgrade`** + a known upgrade CardID |

**C1's one enforcement site:** `AHeroCharacter::ApplyUpgrade` —
`const int32 StackCap = GetStackCapForUpgrade(UpgradeCardID);` → `if (StackCap <= 0) return RefusedInvalidCard;` → `if (GetUpgradeStackCount(UpgradeCardID) >= StackCap) return RefusedAtMaxStacks;`
Guarded upstream by `bKnownUpgrade` (the four-name gate). **This is the whole of `MaxCopies`' authority in the shipped game.**

### 2b. Sites that carry the NAME but NOT the value

| # | Symbol | File | Status |
|---|---|---|---|
| **C4** | `FCardRow::MaxCopies` | `Siegebound/CardRow.h` | the declaration itself; its doc comment already states the `UNCAP-§2` severance |
| **C5** | `UDeckBuilderWidget::GetCardMaxCopies(FName)` | `Siegebound/DeckBuilderWidget.cpp` | ⛔ **THE TRAP: the function is named for the column and does NOT read it.** It resolves the row *only as an existence check* and returns `SiegeLegalDeckSize` (50). The `UNCAP-§4` compat shim. |
| **C6** | `WBP_DeckCardTile.uasset` calls `GetCardMaxCopies` | `Content/UI/` | live **caller of the shim** ⇒ the "+" greys at 50-of-one-card, never at the column's value |
| **C7** | `Tests/SiegeDeckSlotsTest.cpp` — `AddScratchCard(Table, TEXT("Footman"), /*MaxCopies*/ 12)` + the `Siegebound.Deck.Uncap*` cases | tests | ⭐ the column is set to `12` **deliberately, to prove it is ignored**; 50-of-one-card is asserted **LEGAL**. The non-enforcement is **pinned by a test.** |

### 2c. Comment-only sites — **no read, no branch** (13 files, ~30 lines)

`DeckLibrary.h` (41-42) · `DeckLibrary.cpp` (17-19) · `SiegeBotController.h` (203) · `SiegeBotController.cpp` (247) · `DeckBuilderWidget.h` (125/129/176/195/343) · `DeckBuilderWidget.cpp` (126/521/787/1346) · `HeroCharacter.h` (×10) · `HeroCharacter.cpp` (141/171/1003/1009) · `CardRow.h` (205-210).

⭐ **These are in good order** — this is the `SC-§65b` failure class (*"a sentence describing a future that has since arrived"*) and the `CARD-UNCAP` batch handled it. Every one of them either states the abolition or carries a dated rider. **Two examples worth quoting because they are the ones that would otherwise mislead a future reader:**

- `SiegeBotController.h:203` — *"Both defaults are legal (sum 50, each Count <= that card's MaxCopies)"* — **stale on its face**, but the paired `SiegeBotController.cpp:247` carries the correction (*"until CARD-UNCAP 2026-08-28 abolished it"*). The `.h` sentence is historically true and no longer binding. **Not a defect; flagged as the one comment a reader could still take at face value.**
- `DeckBuilderWidget.cpp:126` — *"the CSV column IS the cap"* — **true**, but only for the hero-upgrade tail it annotates.

⚠️ **Census method note (`SC-§80`):** I used `grep`, which is **comment-inclusive by construction** — it does not have `CountOccurrencesInCode`'s blind spot. I wrote no scanner and ran no test. The comment sites above are enumerated from the same sweep as the code sites, not a second pass.

---

## 3. THE THREE LANES, CHECKED SEPARATELY — they do **not** disagree

The row required deck-building, draw and hand-refill to be checked apart, because they *may* disagree. **Measured: they agree, and all three fall on the same side.**

| Lane | Call sites (by symbol) | `MaxCopies` reads | Enforced? |
|---|---|---|---|
| **Deck-building (authoring)** | `UDeckBuilderWidget::AddCopy` · `UDeckLibrary::IsDeckLegal` · `UDeckBuilderWidget::GetCardMaxCopies` · `WBP_DeckCardTile` "+" grey | **0** value reads | ⛔ **NO.** `AddCopy`'s `Current >= Row->MaxCopies` refusal is **deleted** (`UNCAP-§4`). `IsDeckLegal` tests only: row resolves · `Count >= 0` · `TotalCount() == SiegeLegalDeckSize` (50, exact). The "+" greys at 50 via the shim. |
| **Draw** | `UDeckComponent::BuildAndShuffle` · `ShuffleDrawPile` · `DrawNextCard` | **0** | ⛔ **NO.** Builds `Entry.Count` copies of each CardID (override deck) or `Row.DeckCount` copies (fallback). Neither consults `MaxCopies`. |
| **Hand-refill** | `UDeckComponent::ConfirmPlayFromHand` · `DiscardFromHand` · `MoveHandCardToDiscardAndRedraw` · `ReshuffleDiscardIntoDrawIfNeeded` · `PeekNextCardID` | **0** | ⛔ **NO.** `Siegebound/SiegePlayerController.cpp` contains **zero occurrences of the symbol at all.** |
| **Hero-upgrade stack** | `AHeroCharacter::ApplyUpgrade` · `GetStackCapForUpgrade` | **1** | ✅ **YES** — the only one |
| **Display** | `AppendRuleLines` upgrade tail · `GetUpgradeStackCap` pips | 2 | display; truthful; `UNCAP-§5`'s *"Max N per deck"* identity clause is confirmed **deleted** (no `"per deck"` string survives outside comments) |

**The only legality gate in the project is `UDeckLibrary::IsDeckLegal`** — I enumerated its four production callers: `DeckBuilderWidget.cpp:920` (`IsWorkingDeckLegal`), `DeckComponent.cpp:42` (the override-deck gate), `SiegeBotController.cpp:314` (bot deck pick), `SiegePlayerController.cpp:299` (the **active saved deck**, before `SetPendingDeckList`). ⛔ **All four run the same amended body. There is no second, stricter validator anywhere.**

---

## 4. ⛔ DOES `UNCAP-§3` EXPLAIN THE THREE WITCHES? — **YES, and it is the *only* thing that can**

This is the refutation attempt, and it inverted into a proof. Reasoning from the source, not the video:

1. Card copies are **physical instances**: `BuildAndShuffle` pushes one `FName` into `DrawPile` per copy. Nothing creates or destroys entries afterwards — `MoveHandCardToDiscardAndRedraw` moves one to `DiscardPile`, `ReshuffleDiscardIntoDrawIfNeeded` moves the pile back wholesale. **Total Witch instances is conserved for the match.**
2. Three simultaneous Witches (hand slot 4 + slot 5 + `Next`) therefore requires **≥ 3 Witch instances in that deck**. A recycled discard cannot help — the same instance cannot occupy two places.
3. ⭐ **`Witch` has `DeckCount = 0` in `cards.csv`.** The curated-default fallback path would deal **zero** Witches. ⇒ that session ran an **override deck** (`SetPendingDeckList`) authored with `Witch` `Count >= 3`.
4. ⛔ **Under the pre-`CARD-UNCAP` law that deck would have been REFUSED.** `IsDeckLegal`'s deleted per-CardID check compared aggregate count against `Row->MaxCopies` = **2**; `Count 3 > 2` ⇒ illegal ⇒ `BuildAndShuffle` logs the fallback warning and builds from `DeckCount` — **which contains no Witch at all.**

⇒ ⛔ **The three Witches are not a leak past a cap. They are the direct, intended output of the check `UNCAP-§3` deleted.** Before 2026-08-28 that hand was unreachable; after it, it is exactly what the ruling promises.

*(Stronger reading, offered but not asserted: VID-006's timeline also shows a Witch **consumed** at ~00:13 and slot 5 refilled with another Witch. With ~44 cards left in the draw pile no reshuffle can have occurred, so the deck plausibly holds **≥ 4** Witches. I did not read his save file — the row fences the engine — so I assert only **≥ 3**, which is sufficient.)*

### ⛔ THE PLAIN SENTENCE FOR JONATHAN

> **Nothing is broken. You removed the per-card copy limit yourself on 2026-08-28 — your words were *"uncap the maximum number of cards you can hold for any and every card… but you can have as many as you would like for any specific card."* We deleted the check that day. Your deck holds three or more Witches because you built it that way and the game now allows it. The `MaxCopies` column still says `2` for the Witch, but nothing reads that number for units, buildings or spells any more — it only still means something for the four hero upgrades (Sharpened Blade, Plate Armor, Swift Boots, War Banner), where it caps how many times you can stack the upgrade. The hand in the video is behaving exactly as ruled.**

---

## 5. ⛔ NO DEFECT IS MANUFACTURED — but two honest observations, neither a fix row

Recorded as observations, **not** proposed work. Neither is a bug; both are Jonathan-or-manager calls:

- **O1 — a stale `MaxCopies` cell is now silent.** Since 30 of 34 rows never read the column, a wrong value there produces **no error, no log line, no failing test**. `TASK-850` already had to reason about `Fog`'s `MaxCopies = 2` and could only rule it *"inert-and-declared"*. That reasoning cost real review time on `TASK-850` and again here. **A future card author will pay it again.** ⚖️ The options (leave as-is / rename the column to `StackCap` / blank it on non-upgrade rows) are **his call, not mine** — and note any rename touches `DT_Cards.uasset` and three WBPs, which is not free.
- **O2 — `SiegeBotController.h:203` is the one comment that still reads as current law** (*"each Count <= that card's MaxCopies"*) with its correction living in the `.cpp`, not beside it. Cosmetic; recorded for completeness because the row asked for the prose lane to be checked.

⛔ **I am not proposing a fix for the three Witches, because there is nothing to fix.**

---

## 6. WHAT QA SHOULD SCRUTINISE

1. ⭐ **The Blueprint half of the census — I claim it is complete, and here is the control.** A `Source/`-only sweep cannot see a Blueprint that breaks an `FCardRow` and pulls the `MaxCopies` pin. String-probing `Content/` found the name in **four** packages: `DT_Cards.uasset` (the data asset itself) and `WBP_CardHand` · `WBP_HUD` · `WBP_DeckCardTile`. **Positive control, to tell a real read from struct-layout serialization:** I probed each for other `FCardRow` members that are certainly unused there.
   - `WBP_CardHand` / `WBP_HUD`: `bSuicide`, `ChainFalloff`, `SpawnInterval`, `AoERadius` **and `CardRow` itself are ALL present** ⇒ the whole struct layout is serialized; `MaxCopies` is present as a **member name, not a read**.
   - `WBP_DeckCardTile`: those four members are **ABSENT** and `CardRow` is **ABSENT**, while `GetCardMaxCopies` is **PRESENT** ⇒ the `MaxCopies` string is a **substring of the function name** — the already-enumerated C5/C6 shim, not a struct break.
   ⚠️ **Declared limit:** this is a string probe into a binary package, not an engine graph read (the row fences engine/MCP). It proves **absence** strongly; for the two present-by-layout cases it is an **inference from the control**, not a graph inspection. ⛔ **If QA wants it closed absolutely, it needs one editor-side look at those two graphs — I could not and did not do that.**
2. **The `bKnownUpgrade` claim** (that a `Witch` CardID can never reach `GetStackCapForUpgrade`) — verify the four `FName` constants at `HeroCharacter.cpp:90…` match the CSV `CardID`s. I checked `SharpenedBlade` explicitly (`FName(TEXT("SharpenedBlade"))` vs CSV `SharpenedBlade`) and the CSV's four `HeroUpgrade` rows are exactly those four names.
3. **The conservation argument in §4** is the load-bearing step. If entries can be created or duplicated anywhere I did not look, the ≥3 conclusion weakens. I traced every mutator of `DrawPile`/`DiscardPile`/`Hand` in `DeckComponent.cpp` (299 lines, all 12 member functions).
4. **That I picked (2) rather than saying "cannot separate."** The row licensed an honest "cannot separate". I did **not** use it, and §1 gives the falsifier for each rejected box.

---

## 7. FENCES + BOOKKEEPING

- ⛔ **0 source edits** — `git status` on `Source/` is unchanged by me. No test, no compile, no engine, no MCP, no mutating Git (read-only `grep`/`sed`/`awk` over the working tree only; no `checkout`/`restore`/`stash`/`reset`/`clean`).
- ⛔ **Suite delta: `0` tests / `0` files.** Last executed baseline **475 / 0** at `1a457df` — **unchanged and not re-run.**
- ⛔ **Board:** only `TASK-1026`'s own `status:` line was edited, re-located by its `#### TASK-1026 — [MAXCOPIES-DIAG]` heading immediately before the edit.
- 🚨 **GATE ROW: ⛔ ABSENT.** I checked as instructed. `TASK-1026`'s `names:` line carries **no `GATE:` entry**, and a board-wide sweep for `TASK-1026` returns only this row's own lines plus one passing mention inside `TASK-1027`'s status. ⛔ **No qa-reviewer row names TASK-1026 as its subject.** ⇒ **this row is UNGATED**, the same class the `GATE: OWED` abolition was written to stop. **Reporting it, not fixing it** — boarding a gate is the manager's action.
