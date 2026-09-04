# TASK-821 — the controls menu, the card half. ⭐ ONE row, REWRITTEN IN PLACE, and ⛔ `H` alone.

**Agent:** gameplay-programmer · **Date:** 2026-09-02 · **Status:** `ready-for-qa` → gate **TASK-822**
**Law:** `CARDBAR-§9` (the card half) · `CARDBAR-§7` (the inverse trap) · `HELP-§1` · `HELP-§2` · `HELP-§6` · `SC-§37`
**Compile / editor / MCP / WBP / Git:** ⛔ none run, ⛔ none touched. Not mine.

---

## 0. FILES TOUCHED — two, and the diff is small enough to read in full

| file | change | numstat |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | the `Cards.Discard` row (R-09), rewritten in place | **+85 / −16**, ⭐ **2 hunks, both inside that one block** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `+1 test` (14), `+1` fixture entry, `+1` include, file-charter note | **+276 / −1** |

⛔ **NOT touched, and each for a stated reason:**

- **`SiegeControlsHelpWidget.h`** — ⭐ I read it looking for something false to fix and **there is nothing**: it holds no discard prose, `ESiegeInputLane::PointerOnly`'s doc is still true (two war-map rows still use that lane), and *"The 24 rows"* at `:285` is still 24 because the row was **rewritten, not added beside**. ⇒ an edit there would have been noise.
- **`SiegePlayerController.{h,cpp}`** — ⚠️ **TASK-815 is live in that file.** I **read** it (extensively — §2 below is nothing but reads) and wrote **not one character**.
- **`CardHandWidget.{h,cpp}`** · any `.uasset` · `IMC_Hero` · `IA_*` · the tower/stack/wheel rows (**TASK-823**, a different commit).
- ⛔ **`Cards.Play` and `Cards.Cancel`** — item (4) is withdrawn. **Proof, not assurance:** the whole widget diff contains **exactly one** changed `AddRow(TEXT(...))` line and it is `Cards.Discard`.
- ⛔⛔ **`Tests/SiegeControlsHelpTest.cpp`'s `RequiredIds[]`** — see §5, which is the pre-declared automatic fail.

**Encoding/EOL:** both files were already **LF-only and BOM-less at `HEAD`**; my edits changed neither (numstat above is a localised diff, ⛔ not a whole-file rewrite). Both still decode as clean UTF-8.

---

## 1. WHAT THE ROW WAS, AND WHAT IT IS NOW

| | **shipped (was)** | **after this edit** |
|---|---|---|
| id | `Cards.Discard` | ⭐ **`Cards.Discard` — unchanged, deliberately** |
| DisplayName | "Discard a card" | "Discard your whole hand" |
| Lane | `PointerOnly` | ⭐ **`MappedAction`** |
| `bPointerOnly` | `= true` | ⛔ **the line is DELETED** (struct default is `false`, `SiegeControlsHelpWidget.h:210-211`; every other MappedAction row leaves it alone) |
| `Actions` | *(none)* | `{ IA_DiscardAll }` |
| `QwertyReferenceKeys` | *(none)* | `{ EKeys::H }` — ⭐ **fallback + test fixture ONLY**, the chip is DERIVED |
| `RelatedActionIds` | `{ Cards.CursorHold }` | `{ Cards.Cancel }` |
| chip the player sees | "Mouse click" | the **derived key** — `H` on QWERTY, **`D` on his Dvorak**, ⛔ never typed |

### The three shipped falsehoods, all dead

1. ⛔ the one-liner *"Click a hand card's discard button…"* — **gone**. TASK-809 removes the button it named.
2. ⛔ the **`PointerOnly` chip** — **gone** with the lane and the flag.
3. ⛔ the code comment *"⛔ no key binding exists"* — **gone**. One does: `IA_DiscardAll`, bound on `ETriggerEvent::Started` at `SiegePlayerController.cpp:650-652`.

⚠️ **All three survive in the file as QUOTATIONS inside my replacement comment**, so a reader meets what was killed rather than only its absence. ⛔ **That is prose in a `//` comment, ⛔ not shipped data** — and it is exactly the trap TASK-819 finding 8 flagged. **It bit my out-of-engine probe** (a whole-file scan counted my own quotation as the falsehood surviving) and ⛔ **it cannot bite test 14, which reads the COMPOSED player-facing strings, never the file text.** Named here because a reviewer grepping the file will hit the same thing.

---

## 2. `HELP-§2` MECHANISM 3 — EVERY SENTENCE, TO A `file:line` I OPENED

⛔ **Nothing below came from the task board.** The citation table also rides in a C++ comment above the string (`HELP-§2`'s route for citations: ⛔ never in player prose).

| the sentence on Jonathan's screen | read at |
|---|---|
| the whole hand goes **at once** | the loop over `DeckComponent->DiscardFromHand(Slot)`, `SiegePlayerController.cpp:1287-1302`, over the occupied-slot scan at `:1237-1246` |
| the fee is charged **once**, flat | the single `SpendGold(DiscardAllCost)` at `:1269`; the flat-fee rule at `SiegePlayerController.h:581-582` |
| a **one-card** hand costs the same | same two, plus `SiegePlayerController.h:1486-1489` |
| the refusal is **net-zero** | `:1269-1276` — `SpendGold` refuses below the fee with no change and no broadcast |
| the wording is the **shipped** *"Not enough gold"* | `:1274` (`CardRefused_CantAfford`, **reused, ⛔ not reworded**) |
| an **empty hand** is refused **before** any charge | `:1248-1255` (`DiscardAllRefused_EmptyHand`, TASK-819's one new string) |
| a replacement hand is drawn **immediately** | `DeckComponent.cpp:206-240` — `:231-232` push to the discard pile and redraw the slot **in the same call** |
| **refused while placing or targeting** | `:1202-1220`, with `DiscardRefused_Placing` / `DiscardRefused_Targeting` |
| back out with the cancel gesture, then discard | `ASiegePlayerController::OnCancelPlacePressed`, `:1328` |
| the fee is **`DiscardAllCost`**, never a number | `SiegePlayerController.h:1479-1496` — the property's **own** comment pins this rule *for this row*: *"if it is ever shown to the player it is READ from here, ⛔ never typed"* |
| the key is a **mapped action** | `:235` (soft ref) → `:515` (resolve) → `:650-652` (bind) |

⭐ **The 26-letter table was read too, ⛔ not assumed** — `SiegeKeyboardLayoutStatics.cpp:55-64`: `EKeys::H` is there at `:58`, and there is **no digit anywhere in it**. Test 14(b) rests on that read.

---

## 3. ⛔⛔ THE FEE IS NAMED AND THERE IS ⛔ NOT ONE DIGIT IN THE ROW

The pre-declared automatic fail was *"typing the fee as a literal 20"*. What shipped is stronger than "I didn't":

- the prose **names `DiscardAllCost`** and the suite asserts it is named;
- the suite asserts **there is no digit character at all** in this row's one-liner **or** detail — so a future `20`, `1`, or any other retyped tunable turns it red;
- ⚠️ **scoped to this row on purpose**, and the handoff says why: `Cards.Play` legitimately types *"Key 1"* because a digit provably cannot move. A file-wide no-digits rule would be wrong;
- ⭐ **and the wrong fee is asserted against directly.** `"DiscardAllCost"` does **not** contain the substring `"DiscardCost"` (the `C` never follows the `d`), so *"`DiscardCost` is absent"* is a real, independent claim — the retired per-card fee cannot be named on this page. Same for `DiscardHandSlot` / `RequestDiscardSlot`, the vocabulary the old prose used.

---

## 4. ⭐⭐ TEST 14 — ITEM (5), AND WHY IT IS NOT A TRANSCRIPTION

`Siegebound.ControlsHelp.DiscardAllLetterMovesWhileCardDigitsHold` — **one** test, `HELP-§6`'s sentence with **both operands shipped for the first time**.

| block | the claim | ⛔ what makes it able to FAIL |
|---|---|---|
| (a) | the lane really flipped; **`Cards.Play` is on the SAME lane** | on `PointerOnly`, `ComposeKeyChipLabel` answers the pointer chip **regardless of the keys handed to it** (`:1205-1208`) ⇒ (d) would be *unreachable*, not merely red. And "same lane" is the **one-code-path** claim |
| (b) | the **digits' immunity is STRUCTURAL** | ⭐ `SC-§37`'s sharp edge: measuring "the digit held" against *this file's* hand-built fixture would transcribe **my own omission**. So it asks the **shipped** table (`GetQwertyLetterScanCodes()`): the discard key **is** in it, **no card digit is** |
| (c) | fixture self-check, **both hops** | `H → D → E` is three distinct keys (the `F → U → G` idiom), so a **double** translation is detectable rather than invisible |
| (d) | ⭐ **the letter CHANGES / the digits HOLD**, asserted **as a pair in one expression** | one lambda, both rows, opposite outcomes. The Dvorak chip is asserted `==` **the accessor's own one-hop answer** and `!=` the two-hop one — ⛔ never `== "D"` |
| (d) | the **detail page** moves too | its prose names the key as a `{Cards.Discard}` **token** |
| (e) | the fee is named, never typed | §3 |
| (f) | ⛔ **no right-click, no "discard button"**, and the Alt-cursor caveat went with it | the last asserted **as data** (`Cards.CursorHold` is gone from `RelatedActionIds`), ⛔ not as a substring gamble on the word "Alt" |

⭐ **Two vacuity traps I set and then removed, declared because `SC-§37` is the whole point of the task:**

1. My first draft asserted the Dvorak **page body** `Contains("D")`. ⛔ **Vacuous** — the prose also names `DiscardAllCost`, so that letter is present under *any* implementation. Replaced with the claim that can actually fail: the **raw** prose carries the `{Cards.Discard}` token, i.e. **no letter was typed into the sentence at all**.
2. The digit-scanner self-check originally read `Cards.Play`'s prose (which does contain a digit). That would go red if somebody rewrote **their** row. Replaced with a claim about the **scanner** on a synthetic string — the file's own fixture-self-check idiom.

⛔ **What it cannot prove (`SC-§32`):** nothing here presses a key or opens PIE. That `H` reaches `OnDiscardAllPressed` is **TASK-811's pixel row**; that the page *reads well* is **Jonathan's** part of `HELP-§6` and no agent may claim it.

---

## 5. ⚠️ FOR QA — the things worth your scrutiny, named rather than buried

1. ⭐ **THE ROW-ID REASON IN THE LAW IS HALF WRONG, AND I MEASURED IT RATHER THAN REPEATING IT.** `CARDBAR-§9` and the board both say a new id would *"orphan"* rows carrying `RelatedActionIds` into `Cards.Discard`. **I read all 13 `RelatedActionIds` assignments in the file: ⛔ NOT ONE names `Cards.Discard`** — including `Cards.CursorHold`, which the law cites by name and which has **no `RelatedActionIds` at all**. ⇒ the **live** reason the id survives is `Tests/SiegeControlsHelpTest.cpp:227` alone, which is a **stronger** reason, not a weaker one. Recorded in the code comment so the next reader does not re-derive it. **Correct `CARDBAR-§9` or leave the bullet as a hypothetical — your call, ⛔ not mine to edit.**
2. ⛔⛔ **`RequiredIds[]` WAS NOT EDITED — verify it in the diff, ⛔ not on my word.** `git diff -U0` on the test file shows **4 hunks**: `@@ -7,0 +8,6 @@` (the include), `@@ -21 +27,8 @@` (the file charter), `@@ -97,0 +111,7 @@` (the fixture entry), `@@ -1581,0 +1602,255 @@` (test 14). ⭐ **The array lives around `:223-235` and appears in NO hunk.** The three `RequiredIds`-matching lines in the diff are all `+` lines of my own comments.
3. ⚠️ **I CALL `GetPositionalKey` — IN THE TEST, AND IT IS THE ACCESSOR BEING ASKED FOR THE EXPECTED VALUE.** `CARDBAR-§7`'s ban is on **production** code. **Measured:** `GetPositionalKey` occurrences in `SiegeControlsHelpWidget.cpp`/`.h` are **7 and 8 — identical to `HEAD`**. In test 14 it is the same idiom tests 2/3/10 already use ("assert against what the accessor answers"), which is what `HELP-§6` *requires*. ⛔ Zero conditional layout logic anywhere: no `if (bIsDigit)`, no per-key case.
4. ⚠️ **I ADDED ONE ENTRY TO A SHARED FIXTURE.** `MakeDvorakTranslation()` gains `H → D`, **taken from the shipped table at `Tests/SiegeKeyboardLayoutTest.cpp:211`**, ⛔ not re-derived. **I checked every other consumer**: tests 2/3/4/8/10 use `F/T/C/M/A/Tab/Z/SpaceBar` and none uses `H`. ⭐ **And the entry inherits the double-translate trap for free**, because `D → E` was already two lines above it. (The identically-named helper in `SiegeCardHandKeyLabelTest.cpp:84` is a **separate copy in a file I do not own** — ⛔ untouched.)
5. ⚠️ **I CHANGED THE ROW'S RELATED CONTROL, AND IT IS A JUDGMENT CALL.** `Cards.CursorHold` was there **only** because the discard used to be a HUD button you had to raise the cursor to click. There is no button and no cursor in this gesture now, so that block would teach an irrelevance — ⭐ it **is** the Alt-cursor caveat, and item (3) says the caveat goes. `Cards.Cancel` replaces it because my last paragraph sends the player there. ⛔ **This is data on MY row; `Cards.Cancel`'s own page is untouched.** Rule on it.
6. ⭐ **THE OTHER TWO POINTER ROWS SURVIVE, SO THE LANE IS STILL EXERCISED.** `Interface.WarMapReveal` and `Interface.WarMapMarker` are still `PointerOnly` (`:1021`, `:1056`) ⇒ test 8's `HeldChips > 0` and test 13(c)'s pointer-chip claim keep a **shipped** row to stand on. If `Cards.Discard` had been the only one, this flip would have quietly hollowed out two other tests.
7. ⛔ **THE ONE-LINER DELIBERATELY NAMES NO KEY.** The chip renders beside it and is derived; a *"press H"* one-liner would be `HELP-§1`'s defect in the one place the chip cannot cover it.
8. ⭐ **HOW I KNOW THE STRING CLAIMS ARE GREEN WITHOUT COMPILING** (819 finding 9's device, reused). I re-implemented the string-level assertions of tests 1/5/9/10/14 outside the engine, ran them against the **real** registry source: **47/47 green** (row id survived · lane · no `bPointerOnly` · both tokens resolve to real rows · every `{` belongs to a token · no forbidden `HELP-§2` fragment · detail longer than the one-liner · zero digits · no retired vocabulary · related id resolves and is not self · 24 rows, no duplicate id). Then I modelled `GetPositionalKey` as a dict and re-ran test 14's layout half: **12/12 green** (`H → D` moved, `1..6` held, one-hop asserted, two-hop excluded). ⛔ **NEITHER IS A SUBSTITUTE FOR THE SUITE RUN** — they cannot compile, and they say nothing about `TestEqual` overload resolution. They are what caught the two vacuity traps in §4 and the probe's own comment-scan bug.

---

## 6. SUITE — ⛔ A DELTA, AND THE ABSOLUTE IS CHURNING

> ### **MY DELTA IS `+1`.** `Tests/SiegeControlsHelpTest.cpp` **13 → 14** `IMPLEMENT_SIMPLE_AUTOMATION_TEST`. ⛔ **No new test file.** ⭐ **+1 is the only number I own.**

| reading | when | value |
|---|---|---|
| before my edit | **2026-09-02 20:46:10 PDT** | **370** across **29** files |
| after my edit | **2026-09-02 20:57:28 PDT** | **371** across **29** files |

⭐ **In that eleven-minute window my `+1` accounts for the entire movement** — so `371` genuinely reconciles *at 20:57:28*. ⛔ **Do not read that as "the absolute is stable."** TASK-819 watched it go **338 → 344 → 361 → 370** in one sitting, the last two **a minute apart**, as parallel lanes landed `SiegeFogTest` (+9) and `SiegeInvisibilityTest` (+8).

⚠️ **⇒ `TASK-811` / `TASK-824`: take a FRESH `^IMPLEMENT_` census at compile time and RECONCILE it against each task's declared DELTA. ⛔ Do not add. ⛔ Trust no task's absolute, mine included.**

⚠️ **AND A CENSUS TRAP WORTH KEEPING:** a bare `grep -c "^IMPLEMENT_"` counts **`IMPLEMENT_PRIMARY_GAME_MODULE`** in `GitClaudeUnrealTest.cpp` and reports **371/30** where the honest test census is **370/29**. Filter on `^IMPLEMENT_.*AUTOMATION_TEST`. ⭐ That off-by-one is almost certainly why one earlier reading disagreed with another by exactly 1.

---

## 7. WHAT THE DOWNSTREAM TASKS NEED FROM ME

**🔍 TASK-822 (the gate) —** rows (d) and (e) are covered above with the diff evidence, ⛔ not with assurances. Row (c)'s Dvorak half is **test 14**, and it asserts **both directions in one test** as the gate demands. ⛔ Rows (a) and (b) are **TASK-819's**, not mine — I wrote no controller and no widget code.

**⚙️ TASK-823 (the tower half) —** ⭐ **We rebase clean: I touched exactly ONE `AddRow` block.** The registry is still **24 rows**; your three (stack upgrade · footprint wheel · the **missing map-marks row**) are appends. ⚠️ **My `MakeDvorakTranslation()` entry and my test 14 sit at the END of the test file** — append after test 14 and renumber nothing.

**🔧 TASK-811 (the ship) —** ⭐ **The `TAB` screen now teaches `H` and only `H`.** Your pixel row: open `TAB` → *"Discard your whole hand"* under **Cards and the HUD**, with a **key chip**, ⛔ never *"Mouse click"* → click it → the page must say **`DiscardAllCost`** and ⛔ **must not show a number**, and must mention **no right-click**. ⚠️ **If `IA_DiscardAll` did not land, the chip degrades to the reference key — the row still renders and the screen is still honest; ⛔ a `(not bound)` chip is a REPORTABLE state, not a silent pass.**

**⚙️ TASK-844 (bot parity) —** ⭐ **Nothing of mine blocks you and nothing of mine is in your way**: I touched no controller file and typed **no `20` anywhere**. When `SiegeCardEconomy::DiscardAllGold` becomes the single definition, ⭐ **this help row needs ⛔ ZERO edits** — it already names the symbol rather than the value, which is the property that made naming it worth doing.

---

## 8. M8 DECLARATION (verbatim)

⛔ **No replicated property. ⛔ No new replicated class. ⛔ No new relevancy tier. ⛔ No RPC.** This is registry **data** in a client-local help overlay plus one automation test — it adds no shipped surface beyond the strings on one row, and the overlay was already declared client-local display only (`HELP-§5`).
