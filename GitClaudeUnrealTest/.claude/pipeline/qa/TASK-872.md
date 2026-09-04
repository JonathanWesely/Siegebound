# QA Report — TASK-872

**Gate over the `POST-GATE` pair.** `SC-§29` coverage ledger below. `SC-§27`: each diff gets its own verdict.

Verdict: **PASS** — **0 BLOCKERS**, 6 WARN, 5 NIT.

## ⛔ `SC-§29` COVERAGE LEDGER — THE GATE NAMES TWO TASKS AND COULD RULE ON **ONE**

| # | task | subject | verdict | blockers |
|---|---|---|---|---|
| 1 | **TASK-870** | the right-click reconciliation on the help screen | ✅ **PASS** | 0 |
| 2 | **TASK-871** | the four-corner footprint slope trace | ⛔ **NOT DELIVERED — UNRULED** | n/a |

⛔ **`TASK-871` produced no diff and no handoff. I make NO claim about it, and board rows (c), (d) and (e) are UNRULED — ⛔ not passed.** See **W-1**; this is the same shape the board already records on `TASK-874` (`TASK-878` was dispatched as a two-subject gate and one subject had never been delivered).

⚠️ **My instruments, declared up front (`SC-§40`): Read / Grep / Glob only — ⛔ no Bash, ⛔ no git, ⛔ no compile, ⛔ no editor, ⛔ no MCP.** The editor is DOWN (`TASK-835`'s commit build) and was not disturbed. Every claim below says how it was measured. The one class I cannot measure is byte-identity of unchanged regions; what I did instead is **line-offset arithmetic against `qa/TASK-816.md`'s independently-taken anchors**, which is real corroborating evidence and is described in full in §2. ⚠️ Grep rendered several `//` as `\` again (e.g. `SiegeControlsHelpWidget.cpp:582`, `SiegePlayerController.cpp:1351`); each was re-read with `Read` and each is a **display artefact, not a defect** — recorded because `qa/TASK-816.md` recorded the same thing and a reviewer trusting that output would file false blockers.

---

## ⛔ ROW (a) — **DID IT REPAIR THE PAGE THE OBSERVATION NAMED, OR THE ONE IT PREFERRED?** ✅ **THE ONE THE OBSERVATION NAMED.**

🧑 **Jonathan, verbatim:** *"opening the war map and right clicking empty ground does not cause it to close, the map seems to function exactly as it should"* ⇒ `Interface.MapMarks` **TRUE**, `Interface.WarMap` **FALSE**.

| what the ruling required | measured |
|---|---|
| the repair lands on **`Interface.WarMap`** | ✅ `SiegeControlsHelpWidget.cpp:1278-1296` — the row's `Detail`. The right-click clause is **GONE**: the sentence now reads *"…that is the mechanic". **Escape closes it too, polled every frame;** that poll exists because a marker click opens the chat box…"* |
| ⛔ **`Interface.MapMarks` is NOT touched** (an edit there is the **automatic fail**) | ✅ Its one-liner (`:1434`), its four reference keys (`:1435-1437`), its whole detail (`:1438-1466`) and its three edges (`:1473-1475`) are intact and still say *"**• RIGHT-CLICK A CIRCLE deletes it.** A right-click that hits no circle does nothing at all, deliberately"* (`:1449-1450`). ⛔ **No repair, no denial, no softening anywhere on that page.** |
| ⛔ it did **not** cite `handoffs/TASK-814-buildmaster.md` as its authority | ✅ **Zero references to `TASK-814` anywhere in the handoff.** Its authority is Jonathan's sentence + `SC-§42`. ⇒ **the row-(a) WARN does not fire.** |
| ⛔ it did **not** re-derive the direction from source | ✅ Handoff §1 states it explicitly and the shipped comment at `:1261-1268` re-states it as the reason. ⭐ **I did not re-derive it either** — `SC-§42` cl. 3 is binding and reading harder cannot terminate. |

⭐ **AND THE REMEDY'S SHAPE IS THE SHIPPED ONE, VERIFIED RATHER THAN ACCEPTED.** `TASK-821` fixed the identical defect class on `Cards.Discard` by **deleting** the sentence, and test 14 keeps it deleted by asserting the page **never names the gesture** (`SiegeControlsHelpTest.cpp:1608`, and the `Cards.CursorHold`-edge assertion at `:1852-1853`). TASK-870 applied that exact shape — **silence + one outbound delegation edge** — rather than inventing a denial sentence. ⇒ **`HELP-§2`'s "one definition, two renderings" holds: right-click-on-the-war-map is now explained in exactly one place in the whole registry.**

---

## ⛔ ROW (b) — THE FIVE FENCES, EACH MEASURED MYSELF

**(b1) ⛔ ZERO DIGITS IN ANY NEW STRING — ✅ PASS, read by eye, ⛔ not trusted to a test.**
I read both changed player-facing strings character by character: the one-liner (`:1240`, *"Walk up to your commander in your castle, then press it to open the war map — press again to close."*) and the whole `Detail` (`:1279-1296`). **Zero digit characters.** Every value is NAMED, not stated (`ACommanderNpc::IsPlayerInRange`, `InteractRadius`), and the only numeric-sounding phrase is *"even one second after paying"*, which is a word. ✅

**(b2) ⛔ NO ROW RENDERS `(undocumented — TODO)` — ✅ PASS.** `ComposeDetailForDisplay` / `ComposeOneLineForDisplay` (`:1648-1667`) substitute the TODO string **only** on an empty field; the war-map row's `OneLine` and `Detail` are both non-empty after the edit. ⛔ No row was emptied.

**(b3) ⛔ NO SHIPPED ROW'S `RelatedActionIds` RE-POINTED — ✅ PASS, and this is the strongest structural check in the review.**
`Row.RelatedActionIds = ` occurs on **exactly 16 blocks** in the registry — `:460 · :570 · :704 · :781 · :874 · :913 · :967 · :1036 · :1064 · :1102 · :1151 · :1233 · :1307 · :1348 · :1372 · :1473`. ⭐ **`qa/TASK-816.md` W-5 independently read "all 16 `RelatedActionIds` blocks" at `TASK-823`'s landing. The count is still 16** ⇒ **no block was added or removed.** The only block whose contents grew is the one TASK-870 owns:

```
Interface.WarMap.RelatedActionIds:
  BEFORE  { Interface.WarMapReveal, Interface.WarMapMarker }
  AFTER   { Interface.WarMapReveal, Interface.WarMapMarker, Interface.MapMarks }   // appended, shipped order kept
```

- ✅ **Outbound from the row being rewritten** — the half `HELP-§7` permits, **declared** in the handoff §3.2 **and** in the shipped comment at `:1298-1306`, with its reason. Both `HELP-§7` obligations met.
- ✅ **Nothing was re-pointed INTO this row.** The only other row naming `Interface.MapMarks` is `Cards.PlacementResize` (`:782`) — which is itself one of `TASK-823`'s three new rows, i.e. **its own** outbound edge, shipped, not TASK-870's. ⛔ No pre-`TASK-823` row names it.
- ✅ **The new id RESOLVES**, and I checked this rather than assumed it: `Interface.MapMarks` is a real row, `AddRow` at **`:1433`**. All three ids on the page resolve (`Interface.WarMapReveal` `:1318`, `Interface.WarMapMarker` `:1353`, `Interface.MapMarks` `:1433`).
  - ⚠️ **The hazard is real and the handoff located it correctly:** a dangling edge renders **nothing** and logs **nothing** (`ComposeDetailContent` `continue`s), and the registry-wide walk that catches it lives inside **test 9, `EveryRowHasAuthoredDetail`** — `SiegeControlsHelpTest.cpp:1085` (macro) with the walk at **`:1131-1134`**, ⛔ *not* in a test named for edges. **That mislocation is exactly what fooled two readers and produced a struck bullet in `HELP-§7`; TASK-870 named the right test.**
- ✅ **No cycle.** `ComposeDetailContent` is one level deep by construction (`SiegeControlsHelpWidget.h:177`; the related entry's body comes from `ComposeDetailForDisplay(*RelatedRow)` at `:1805`, which never reads that row's own edges). `Interface.MapMarks` already pointed back at `Interface.WarMap` **before** this change (`:1474`, a `TASK-823` line) — pre-existing and harmless.
- ✅ **No existing test pins this row's edge count or contents.** I checked every `RelatedActionIds` reference in the test file: test 13's `Page.Related.Num() == Row.RelatedActionIds.Num()` (`:1382-1383`) is scoped to the **Orders** rows; test 15's `Num() > 0` walk (`:2052`) is scoped to the three `TASK-823` rows; test 9 walks all rows for **resolution only**. ⇒ **the append turns nothing red.**

**(b4) ⛔ NO `GetPositionalKey` ON THE LABEL PATH — ✅ PASS.** `GetPositionalKey` occurs **8 times** in `SiegeControlsHelpWidget.cpp`; exactly **2 are calls** — `:1563` (the Lane-C derive arm) and `:1599` (the Lane-A fallback), **both pre-existing**. The other six are comments/prose. ⭐ `qa/TASK-816.md` row (g4) measured the same two calls at `:1524` and `:1560`; **both moved by exactly +39 and neither changed** — see §2. ⛔ **Zero calls added.**

**(b5) ⛔ THE `RawNonLetter` DOCSTRING CHANGE IS ADDITIVE AND THE ENUM IS UNTOUCHED — ✅ PASS.** `SiegeControlsHelpWidget.h:83-98`. The original sentence survives verbatim (*"A RAW-polled key that is NOT a letter — a mouse button, Escape, or the wheel"*); the `qa/TASK-816.md` N-1 clause is **appended** (*"…⭐ or a Slate-delivered mouse gesture: the lane selects the VERBATIM-LABEL algorithm, not the delivery route"*), plus a dated note recording why. ⛔ **The four enum values are unchanged** — `MappedAction` · `RawNonLetter` · `RawLetter` · `PointerOnly` — and no row's `Lane` moved. **Comment-only, as declared.**

**(b6) ⛔ `RequiredIds[]` UNTOUCHED / NO ROW DELETED — ✅ PASS, and I checked the trap the board named.**
`RequiredIds[]` is at `SiegeControlsHelpTest.cpp:248-260` and still lists all **24** original ids including `Interface.WarMap` (`:256`). ⛔ **It was NOT edited to accommodate anything**, and no row was deleted: `AddRow(` occurs **27 times** — 6 Hero + 6 Cards + 5 Orders + 3 PickMode + 7 Interface — reconciling exactly to `qa/TASK-816.md` row (g2)'s independently-measured 27. ⭐ **The war-map row was rewritten IN PLACE, which is precisely why the required-subset assertion cannot see it — and that is the correct way to do it, not an evasion.**

---

## ⭐⭐ §2 — HOW I BOUNDED THE DIFF WITHOUT GIT: THE UNIFORM-OFFSET MEASUREMENT

I cannot run `git diff`. Instead I re-measured **every line anchor `qa/TASK-816.md` recorded independently at `TASK-823`'s landing** and looked at the offsets. The result is not decorative — it **localises the entire diff**.

**`SiegeControlsHelpWidget.cpp`:**

| anchor from `qa/TASK-816.md` | its line then | measured now | offset |
|---|---|---|---|
| `AddRow("Cards.StackUpgrade")` | `:665` | **`:665`** | **0** |
| `AddRow("Cards.PlacementResize")` | `:752` | **`:752`** | **0** |
| `Interface.WarMap` detail's false sentence | `:1268` | *(deleted)* | — |
| `AddRow("Interface.MapMarks")` | `:1394` | **`:1433`** | **+39** |
| `Interface.MapMarks`' *"right-click that hits no circle"* | `:1410` | **`:1449`** | **+39** |
| the `GetPositionalKey` prose occurrence | `:1355` | **`:1394`** | **+39** |
| `GetPositionalKey` call, Lane-C derive arm | `:1524` | **`:1563`** | **+39** |
| `GetPositionalKey` call, Lane-A fallback | `:1560` | **`:1599`** | **+39** |

⇒ ⭐ **ZERO net line change anywhere above `:1239`, and a single uniform `+39` everywhere below it.** The `+39` is fully accounted for by the two new comment blocks (`:1254-1277`, 24 lines) and (`:1298-1306`, 9 lines) plus the reshaped edge initialiser — i.e. **the whole diff to this file lives inside the `Interface.WarMap` row block**. ⛔ The six fenced rows are structurally unreachable by it.

**`SiegeControlsHelpTest.cpp`:** same method, uniform **+5** (test 9's walk `:1126`→`:1131`; the digit control `:1946`→`:1951`; `TestNull("Cards.NoSuchUpgradeRow")` `:2061`→`:2066`; test 16's fixture self-checks `:2183-2186`→**`:2188-2191`**). The `+5` is exactly the new file-header paragraph at `:29-33`. ⇒ **no existing test body changed line count**, and test 17 is a pure append at `:2210-2425`.

**`SiegePlayerController.cpp`:** `qa/TASK-816.md` W-1 cited the war-map poll at **`:746`**. Measured now: **`:746`** — `if (WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Escape))` inside the `if (bWarMapOpen)` branch at `:744-755`. ⛔ **Byte-stable ⇒ the controller was not touched. THE POLL IS INTACT AND NO BEHAVIOUR CHANGED.** ✅ This also confirms the handoff's citation repair: the branch really is at `:744-755`, and the old `:643-671` really had rotted (`SC-§38`).

⚠️ **The honest limit (`SC-§40` cl. 1):** an offset measurement cannot see an **in-place, equal-line-count** edit. It is corroboration, not a diff. **W-5 hands the three byte-identity claims to build-master, which has git.**

---

## ⭐ §3 — THE TEST, AND ITS CONTROL, VERIFIED RATHER THAN ACCEPTED

`Siegebound.ControlsHelp.RightClickIsTaughtOnlyByTheRowsThatOwnIt` (`FSiegeControlsHelpRightClickMeaningsTest`, `SiegeControlsHelpTest.cpp:2251-2425`).

| block | what I measured | will it be green? |
|---|---|---|
| **(a)** count of rows carrying `EKeys::RightMouseButton` **== 3** | `EKeys::RightMouseButton` appears **4 times** in the widget cpp: `:581` (`Cards.Cancel`), `:1073` (`PickMode.Cancel`), `:1436` (`Interface.MapMarks`) — and `:1412`, which is a **citation comment**. ⇒ **the data path carries exactly 3.** | ✅ **3** |
| **(b)** they are `Cards.Cancel` · `PickMode.Cancel` · `Interface.MapMarks`, checked **against the scan** | all three present | ✅ |
| **(c)** pairwise-distinct id / headline / one-liner / detail / **category** | headlines *"Cancel"* · *"Exit the command"* · *"Draw circles on the map"*; categories `CategoryCards` · `CategoryPickMode` · `CategoryInterface` | ✅ all distinct |
| **(d1)** the war-map row carries no right button | `Row.QwertyReferenceKeys = { EKeys::M };` (`:1242`) | ✅ |
| **(d2)** six zeros over `right-click` / `right click` / `right button` in the **composed** one-liner + detail | I read both strings in full: **no occurrence of the substring `right` at all**, in any case | ✅ **0/6** |
| **(d3)** delegation + every edge resolves | verified above | ✅ |
| **(e)** fixture self-checks + negative control | `MakeRow` signature at `:188` matches the call shape exactly (test 16 uses the identical form at `:2189`); `Interface.NoSuchMapRow` is **not** a registry id | ✅ |

⭐⭐ **THE POSITIVE CONTROL — VERIFIED, AND IT IS GENUINELY SAME-ROLE.** `:2370-2378` runs **the identical fragment scan over `ComposeDetailForDisplay(Interface.MapMarks)`**, which I read at `:1449`: *"• RIGHT-CLICK A CIRCLE deletes it."* ⇒ the scan **must** answer true. **Same needle, same function, same code path, a real registry row.** ⛔ Without it the six zeros in (d2) would be indistinguishable from a scanner that matches nothing (`SC-§39`).

⭐⭐ **AND THE ONE THING I WENT LOOKING FOR THAT COULD HAVE MADE THE TEST SELF-CONTRADICTORY — IT DOES NOT.** (d2) scans the war-map page while (d3) attaches a delegation edge to a page that **does** say "right-click". Had the scan used the **composed page** (`ComposeDetailContent`, whose `Content.Body` is the row's prose and whose `Related[].Body` carries each related row's prose, `:1773`/`:1805`), the delegated `Interface.MapMarks` text would have injected `right-click` into the war-map page and **turned its own test red**. Measured at `:1657-1667`: **`ComposeDetailForDisplay` returns `Row.Detail` and nothing else** — row-own prose, with only the empty→TODO substitution. ⇒ **the two blocks are compatible.** ✅ This is the one place where the test could have shipped green-looking and red-running, and it is clear.

⭐ **ITS OWN INSTRUMENT STORY IS TRUE AND WORTH KEEPING (`SC-§39`).** The handoff records that its **first** needle scanned the war-map **source block** and returned `4 / 1 / 1`, every hit inside the C++ comment it had just written quoting Jonathan. **I reproduced the conditions:** the comment at `:1254-1268` does spell `right-click`/`right click` repeatedly, and there is a **4th** textual `EKeys::RightMouseButton` at `:1412` that is a citation comment. ⇒ **a source-grep gate would count 4 and would find the banned fragments; the data-path scan counts 3 and 0.** The shipped test uses the data path. **The trailing-comment edge was real, it was self-caught, and the shipped instrument is immune to it.**

**Compile-safety spot checks (⛔ I did not compile):** `if (!TestNotNull(...))` is shipped precedent in this tree (`SiegeDeckSlotsTest.cpp:237` and 14 more); `MakeRow(Lane, {keys}, bool, bool)` matches `:188` exactly; the class name occurs exactly **twice** (`:2252` macro, `:2256` definition) and the test name string exactly once in `Source/`; ⛔ no duplicate `Siegebound.ControlsHelp.*` name.

---

## ⚖️⚖️ THE TWO ESCALATIONS — **BOTH RULED. NEITHER LEFT AMBIGUOUS.**

### ⭐⭐ **R-1 — THE DECLINED DELIVERABLE (item (3), the spell-targeting-cancel row): ⚖️ THE REFUSAL IS UPHELD. ⛔ DO NOT WRITE THE ROW.**

The handoff declined a **boarded deliverable** on the ground that `qa/TASK-816.md` **W-2**'s premise — *"spell-targeting cancel has no row at all"* — is **false**. **I re-measured both of its verbatim quotes and the code, myself, at source:**

| its claim | measured |
|---|---|
| `Cards.Cancel`'s one-liner names targeting | ✅ `SiegeControlsHelpWidget.cpp:579` — *"Back out of whatever you are placing, **targeting** or circling — it never costs anything."* **Verbatim match.** |
| `Cards.Cancel`'s detail names spell targeting | ✅ `:593-595` — *"One action, two keys, and it is the same gesture everywhere. It exits placement mode, **exits spell targeting**, or aborts a group-order pick at any stage, leaving every existing group and stance untouched."* **Verbatim match.** |
| one handler, three branches | ✅ `ASiegePlayerController::OnCancelPlacePressed`, **`SiegePlayerController.cpp:1343-1368`** — `bInPlacementMode → ExitPlacementMode()` (`:1345-1349`) · `bInTargetingMode → ExitTargetingMode()` (`:1354-1358`), with the shipped comment *"the SAME cancel action leaves targeting mode at no cost"* · `GroupPickStage != None → CancelGroupPick()` (`:1364-1367`). **Exact.** |

⇒ ⚖️ **RULED: `CARDBAR-§8` consumers 2 and 4 are ONE bound action and ONE help row, and that row names spell targeting explicitly. A second row would duplicate a shipped page — the conflation `HELP-§2` calls worse than no help screen — and de-duplicating it would require editing `Cards.Cancel`, which the dispatch fenced.** The refusal was correct, it was measured before it was made, and it was reported rather than silently skipped (`SC-§40`). ⭐ **This is the second time in this family that a QA verdict was carried forward as a fact; `SC-§40` cl. 3 is doing exactly what it was written for, and this time the programmer applied it to a report written by my own role.** See **W-3** — `qa/TASK-816.md` W-2 needs a correction rider so nothing else is boarded on it.

⚠️ **What is genuinely left of W-2 is satisfied, not dropped:** its substantive ask was that the right-click meanings be **told apart**. Test 17 (c) now asserts that as a structural property — three ids, three headlines, three one-liners, three detail pages, **three different categories**.

### ⚠️⚠️ **R-2 — ESCAPE: ⚖️ RULED **OWED**. IT IS A QUESTION FOR JONATHAN, ⛔ NOT A HANDOFF FOOTNOTE.**

**Say it plainly, because that is what this ruling is for:** `Interface.WarMap`'s page **still claims Escape closes the map**, that claim has **never been observed**, and it is **the same evidentiary class** as the right-click half that turned out to be false. The dispatch fenced it, the programmer obeyed the fence and — correctly — **made no claim about it**.

⇒ ⚖️ **RULED: it is OWED, and it goes to Jonathan as a one-click PIE row, not to a reader.** *"Open the war map, press Escape — does it close?"* **One sentence closes the other half of this page.**

⛔ **I am deliberately NOT settling it by reading, and I want the reason on the record rather than the conclusion.** I can see that the poll at `SiegePlayerController.cpp:746` does test `EKeys::Escape`, and that `UWarMapWidget::NativeOnMouseButtonDown` is a **mouse** handler that cannot consume a key. That is **confidence, and `SC-§42` cl. 3 says convergence on an unobservable is exactly the state in which a wrong answer ships fastest** — key routing under a live input mode with a focused widget is an observation, not a deduction. **Three readers already reasoned correctly to an answer they could not confirm; I am not adding a fourth reasoning pass.** ⛔ **Do not let anyone record my paragraph above as evidence.**

### ⚖️ **R-3 — THE `PickMode.Cancel` JUDGMENT CALL (handoff §5.1 / its item 4): ⚖️ LEAVE IT ALONE. The programmer's choice is upheld.**
`:1072` reads *"Right-click or press Escape to abandon the order at any stage — it costs nothing and changes nothing."* The scope sits in the same clause, the page lives under the **Order pick** heading, and test 17 (c) now asserts the property structurally. ⛔ **Editing a correct page to satisfy a prose-ordering preference is churn on a row adjacent to a fence, and it buys nothing.** The row was **not** fenced and it was **still** left alone — that is the right instinct.

---

## Findings

### BLOCKER
**None.**

### WARN

- **[WARN] W-1 — `TASK-871` WAS NEVER DELIVERED, AND THIS GATE COULD RULE ON ONLY ONE OF THE TWO TASKS IT NAMES.**
  **Measured, three ways:** ⛔ `handoffs/TASK-871-programmer.md` **does not exist** (`Glob` over `handoffs/TASK-87*.md` returns 876, 874, 870 only) · ⛔ `Tests/SiegePlacementTest.cpp` carries **28** tests, byte-identical in count to `qa/TASK-816.md`'s measurement, so no negative control and no corner-trace test was added · ⛔ its board status is still **`▶ DISPATCHABLE`**, ⛔ never `ready-for-qa`.
  ⇒ **Board rows (c), (d) and (e) are UNRULED. ⛔ Nothing in this report may be read as passing them, and `qa/TASK-816.md` W-4 / ruling `R-3` remains open.**
  ⭐ **Same shape the board already records on `TASK-874`: a `blocked-by` list is a PLAN, never a MEASUREMENT, and the ten-second detector — does the handoff file EXIST? — was available before dispatch.** **Suggested fix:** re-dispatch `TASK-871` and board a gate over it **by name**; ⛔ do not fold it into a later multi-subject gate on the assumption it landed.

- **[WARN] W-2 — `CARDBAR-§8`'s TABLE ROW 3 IS NOW KNOWN INCOMPLETE ABOUT **REACHABILITY**, AND ITS CITATION HAS ALSO ROTTED. `SC-§38` cl. 4 SAYS ANNOTATE IT IN **THIS** SITTING.**
  Row 3 reads: *"war-map close | `SiegePlayerController.cpp:721` (polled) | `bWarMapOpen` — ⛔ `return`s at `:729`"*. **Two defects, both measured by me:** (i) the branch is at **`:744-755`** with the return at **`:754`** — the line citation is **rotted** by ~23 lines; (ii) more importantly the row is **silent about reachability** — the poll genuinely exists, but Jonathan's observation proves **right-click never arrives there**, so a reader planning a fifth RMB consumer would count a consumer that is not competing for the gesture.
  ⛔ **The row is not FALSE, which is exactly why it will not be noticed.** ⛔ `CONVENTIONS.md` is the manager's file and I did not edit it. **Suggested fix — one annotation on row 3:** *"⚠️ the poll exists at `PlayerTick`'s `if (bWarMapOpen)` branch (BY SYMBOL) but **RMB does not reach it** — `SC-§42`, Jonathan's observation 2026-09-03; consumer 3 competes for **Escape**, not for the right button."* The programmer correctly reported this instead of reaching for the file.

- **[WARN] W-3 — `qa/TASK-816.md` **W-2** CARRIES A PREMISE THAT IS **REFUTED AT SOURCE**, AND A BOARD ITEM WAS BUILT ON IT.**
  See **R-1**. *"Spell-targeting cancel has no row at all"* is false: `Cards.Cancel` names it in **both** its one-liner and its detail, and the code agrees at one handler with three branches. ⛔ **A verdict is a citation, not a fact** (`SC-§40`) — and this one is mine-by-role, which is why it is written here rather than left implied. **Suggested fix:** the manager appends a struck-and-corrected rider to `qa/TASK-816.md` W-2 **and** to board item (3), naming `SiegeControlsHelpWidget.cpp:579`/`:593-595` and `SiegePlayerController.cpp:1343-1368`, so no future row is boarded on the withdrawn half.

- **[WARN] W-4 — THE ESCAPE HALF OF THE SAME SENTENCE IS UNOBSERVED AND STILL SHIPPING.** See **R-2**. **Suggested fix:** board a one-click PIE row for Jonathan — *"open the war map, press Escape: does it close?"* — and route the answer to the manager. ⛔ **Until it is observed, nobody may call the war-map page "reconciled"**; half of it rests on a measurement and half on an assumption, and the last time that was true the assumption was the false one.

- **[WARN] W-5 — THREE BYTE-IDENTITY CLAIMS ARE OUTSIDE MY INSTRUMENTS. HAND THEM TO BUILD-MASTER, WHICH HAS GIT.**
  I cannot run `git diff`. **Claimed and not measured by me:** (a) the six fenced rows are byte-identical; (b) no existing test body was edited; (c) `SiegePlayerController.{h,cpp}` carries no TASK-870 hunk. **What I did measure is in §2 and it is real corroborating evidence** — a uniform `+39` / `+5` / `+0` offset structure that localises both diffs, plus a `RelatedActionIds` block count that still reads 16 and an `AddRow(` count that still reads 27. **Suggested fix:** `TASK-873`'s host build runs `git diff -- Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` and confirms every hunk falls inside the `Interface.WarMap` block, the header paragraph, and the appended test 17.

- **[WARN] W-6 — ⛔ `TASK-873`'s PATHSPEC NAMES FOUR FILES AND ONLY **TWO** CARRY THIS DIFF. THE OTHER TWO ARE `TASK-871`'s, AND `TASK-871` NEVER DELIVERED.**
  The rider's declared pathspec is `SiegeControlsHelpWidget.{h,cpp}` · `SiegePlayerController.{h,cpp}` · `Tests/SiegeControlsHelpTest.cpp` · `Tests/SiegePlacementTest.cpp`. TASK-870 touched **three** files, none of them `SiegePlayerController.*` or `SiegePlacementTest.cpp`. ⇒ ⛔ **any content sitting in those two files right now belongs to some OTHER lane and would be swept in UNGATED.** (`TASK-844` is the board's declared next writer of that header.) **Suggested fix — a stop condition, not a preference:** before adopting the rider, build-master confirms `SiegePlayerController.{h,cpp}` and `Tests/SiegePlacementTest.cpp` are **clean**; if they are not, **drop them from the pathspec** and say why. ⭐ Also see the census note — **`Tests/SiegeAssistantSelectionTest.cpp` carries `TASK-874`'s `+1` in the same working tree and is NOT in this pathspec**, so the executed run will cover a test the commit does not carry (`TL-§5d` cl. 3's shape).

### NIT

- **[NIT] N-1 — `handoffs/TASK-870-programmer.md` §2 cites `RequiredIds[]` at `SiegeControlsHelpTest.cpp:243-255`. Measured: `:248-260`.** ⛔ Nothing rode the bad number and the **symbol is unique and intact**. `SC-§38` cl. 5 exempts handoffs; cl. 6 binds the prompt that relays one — **relay `RequiredIds[]` as a symbol, not as a line range.**
- **[NIT] N-2 — the handoff says "one line in the file-header index". Measured: a **5-line** paragraph at `SiegeControlsHelpTest.cpp:29-33`.** Harmless in itself, but it is the exact figure the `+5` offset reconciliation in §2 depends on, so it is worth being right about.
- **[NIT] N-3 — a pre-existing citation rot on a FENCED row, found in passing and correctly NOT touched.** `SiegeControlsHelpWidget.cpp:582-583` (`Cards.Cancel`'s citation comment) says the three-mode handler is at `SiegePlayerController.cpp:1065-1090`; measured, `OnCancelPlacePressed` is at **`:1343-1368`**. ⛔ Not TASK-870's to fix — the row is on the do-not-touch list. **For whoever next edits `Cards.Cancel`** (and note **R-1** just made that comment's *content* load-bearing for a ruling).
- **[NIT] N-4 — the war-map detail page now renders the WHOLE map-marks page underneath it**, including that page's `{Interface.WarMap}` token, so the page for *opening* the map will carry a delegated block reading *"Open it with [M]"*. One level deep, no recursion, no test affected — **cosmetic, and it closes on Jonathan's eyes** (`HELP-§6`'s Jonathan half, which the programmer correctly declined to claim). Recorded so nobody "fixes" it by copying prose back onto the war-map page, which is the thing the delegation exists to prevent.
- **[NIT] N-5 — the `Interface.WarMap` row's new comment block spells `right-click` several times, inside a file this project greps.** That is **not** a defect — the shipped test scans the **data path** — but it is `SC-§41` cl. 4's shape one file over: **anyone who later writes a source-text gate on this file will get 4 and 6 where the data path gives 3 and 0.** ⛔ Do not "fix" the comment; **fix the needle.**

---

## ⛔ CENSUS — `TL-§5b` / `TL-§5c`

> ⛔ **`427 declared`. ⛔ NOT `427/427`.** I ran ⛔ **no suite**. A pass count belongs to `TASK-873` and only with a `Result={Success}` / `Result={Fail}` pair in hand.

**Fresh, taken by me at review time:** **`427 declared across 31 files`** — needle `^IMPLEMENT_[A-Z_]*AUTOMATION_TEST\(`, scoped to `Source/**/*.cpp`. ✅ **Exactly the figure `TASK-870` declared (`426 → 427`, files `31 → 31`).**

⚠️ **`TL-§5b` cl. 2a's trap — RE-MEASURED, ⛔ not taken on trust, and it is LIVE:** the bare `^IMPLEMENT_` needle returns **`428 across 32 files`**. **The single extra hit is named:** `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.cpp` — `IMPLEMENT_PRIMARY_GAME_MODULE`. ⛔ **The off-by-one lands in the FILE count, which is exactly where it mimics an undeclared test file.** ⛔ An absence is a property of a tree, never a repeal.

**Positive control on the instrument (`SC-§39`):** the scoped needle returns a large non-zero (427) and resolves per-file (`SiegeControlsHelpTest.cpp` = **17**, `SiegeAssistantSelectionTest.cpp` = **39**, `SiegePlacementTest.cpp` = **28**), so it is not blind. ⛔ **Zero `IMPLEMENT_COMPLEX_*` / `IMPLEMENT_CUSTOM_*` macros exist in the tree**, so one macro is one test throughout.

**Delta reconciliation — ⛔ against declared deltas, ⛔ never against an absolute (including the ones in this report):**
`SiegeControlsHelpTest.cpp` **16 → 17**. `qa/TASK-816.md` measured **16** at `TASK-823`'s landing; I measure **17**; declared delta **+1**. ✅ **Balances.**

⭐⭐ **AND A TRAP IN THE RECONCILIATION ITSELF, NAMED SO NOBODY "FIXES" A NUMBER THAT IS RIGHT.** The baseline handed to me is *"the last **executed** run was `426/0` at `e9df584`"*. Naively adding **both** working-tree deltas — `TASK-870`'s `+1` **and** `TASK-874`'s `+1` (its file measures **39**, and `ThirteenKindsInCardRowOrder` survives only inside a comment explaining its deletion, so that diff **is** in the tree) — predicts **428**. The tree measures **427**. ⇒ **The resolution is that `426` is a PASS COUNT, not a census** (`TL-§5c`'s entire point), **and the run that produced it compiled the WORKING TREE, never `HEAD`** (`SC-§43` cl. 4, measured on this project twice in one day) — so `TASK-874`'s `+1` was **already inside** it. ⇒ `426 + 1 (TASK-870) = 427` ✅ **exact.**
⛔ **`TASK-873`: do not reconcile a census against a pass count, and do not go looking for a missing test. Take your own fresh census, reconcile `fresh == prior_census + Σ(declared deltas)`, and expect the executed `N` to be its own number.**

---

## Notes for build-master (`TASK-873`)

1. ⛔ **Parse the log for `Result: Failed` — ⛔ never `$LASTEXITCODE`.** Standing law.
2. ⛔⛔ **W-6 IS A STOP CONDITION, NOT A PREFERENCE.** `TASK-871` **never delivered**. Before adopting the rider's pathspec, confirm `SiegePlayerController.{h,cpp}` and `Tests/SiegePlacementTest.cpp` are **clean**; if they are not, drop them and name what you found. ⛔ **Two of the four files in that pathspec carry no TASK-870 hunk at all.**
3. ⭐ **W-5 — the one thing I could not measure and you can.** `git diff` the two touched files and confirm every hunk falls inside (i) the `Interface.WarMap` row block, (ii) the `RawNonLetter` docstring, (iii) `SiegeControlsHelpTest.cpp:29-33`, (iv) the appended test 17. **A hunk anywhere else is a stop condition** — the six fenced rows and `RequiredIds[]` are the whole point of this gate.
4. ⛔ **Take your own fresh census; ⛔ do not reconcile *to* my 427.** Read the ⭐⭐ paragraph above first — the `426` you have been handed is a **pass count from a working-tree run**, and `TASK-874`'s `+1` is inside it. Reconcile census-to-census on **declared deltas**.
5. ⚠️ **`Tests/SiegeAssistantSelectionTest.cpp` (`TASK-874`, `+1`) is in the same working tree and is NOT in the rider's pathspec.** Your executed run will therefore cover a test your commit does not carry. Say so in the handoff rather than letting the counts look mismatched later (`TL-§5d` cl. 3).
6. ✅ **Nothing here needs the editor or MCP.** ⛔ No behaviour changed anywhere: three files, and two of the three changes are comments. The one production change is player-facing prose plus one appended registry edge.
7. ⚠️ **`TASK-852` shares this file family and is serialised behind `TASK-870`.** Its footprint is one row's prose/comment/edges, one enum docstring, one 5-line header paragraph, one appended test. ⛔ No shared helper, ⛔ no `RequiredIds[]`, ⛔ no existing test body.

## Notes for the manager

- ⛔⛔ **`TASK-871` NEEDS RE-DISPATCHING AND ITS OWN GATE (W-1).** This gate covers one of the two tasks it names, and rows (c)/(d)/(e) are **unruled**, not passed.
- ⚖️ **R-1 UPHELD — ⛔ do NOT board the spell-targeting row.** `qa/TASK-816.md` **W-2**'s premise is refuted at source and needs a struck-and-corrected rider (W-3), on the report **and** on board item (3), before anything else is built on it.
- ⚠️⚠️ **R-2 — ESCAPE IS OWED AND IT IS A QUESTION FOR JONATHAN (W-4).** One click, one sentence: *"open the war map, press Escape — does it close?"* ⛔ Route it as a PIE row; ⛔ do not let anyone close it by reading, including on the strength of my own paragraph in R-2.
- ⚠️ **`CARDBAR-§8` row 3 needs its reachability annotation **in this sitting** (W-2, `SC-§38` cl. 4)** — suggested wording is in the finding. It is your file; the programmer correctly did not reach for it.
- ⭐ **The `+39`/`+5`/`+0` offset method in §2 is reusable by any gate that has no git.** It cannot see an equal-line-count in-place edit, so it is corroboration, never a diff — but it localised two diffs to the block in about five greps.

---

## Board status lines

⚠️ I have no line-editing tool and `TASKBOARD.md` must not be whole-file written — returning the lines rather than editing the file.

```
TASK-872 → status: ✅ qa-passed (2026-09-03) — PASS, 0 BLOCKERS, 6 WARN, 5 NIT. Report: .claude/pipeline/qa/TASK-872.md. ⛔ SC-§29 LEDGER: this gate covers ONE of the two tasks it names — TASK-871 was NEVER DELIVERED (no handoff on disk, SiegePlacementTest.cpp still 28) so rows (c)/(d)/(e) are UNRULED, not passed. ⚖️ TWO ESCALATIONS RULED: (R-1) the declined spell-targeting row — REFUSAL UPHELD, qa/TASK-816.md W-2's premise refuted at source (Cards.Cancel names targeting in BOTH strings; OnCancelPlacePressed has all three branches at :1343-1368); (R-2) ESCAPE IS OWED — one PIE click for Jonathan, same evidentiary class as the right-click half. Census: 427 declared across 31 files (⛔ NOT a pass count, TL-§5c).
TASK-870 → status: ✅ qa-passed (2026-09-03, qa/TASK-872.md — 0 BLOCKER · 6 WARN · 5 NIT) — ready-for-integration, rider TASK-873. ⭐ IT REPAIRED THE PAGE THE OBSERVATION NAMED: Interface.WarMap's right-click claim is GONE, Interface.MapMarks is BYTE-UNTOUCHED, Escape + the controller poll (SiegePlayerController.cpp:744-755) are INTACT. ⭐ The remedy is TASK-821's shipped shape (delete, don't deny) + ONE declared outbound edge that RESOLVES. Test 17 verified INCLUDING its same-role positive control, and the one way it could have contradicted itself (ComposeDetailForDisplay vs ComposeDetailContent) was checked and is clear. ⚠️ ESCAPE STILL UNOBSERVED — owed as a one-click PIE row.
TASK-871 → status: ⛔ STILL NOT DELIVERED as of 2026-09-03 — no handoff on disk, Tests/SiegePlacementTest.cpp unchanged at 28 tests. ⛔ NOT covered by qa/TASK-872.md (SC-§29). Needs re-dispatch + its own gate; qa/TASK-816.md W-4 / R-3 REMAINS OPEN.
TASK-873 → blocked-by: ~~TASK-872 (PASS)~~ ✅ CLEARED 2026-09-03. ⛔ STOP CONDITION FIRST (qa/TASK-872.md W-6): TASK-870 touched only SiegeControlsHelpWidget.{h,cpp} + Tests/SiegeControlsHelpTest.cpp — verify SiegePlayerController.{h,cpp} and Tests/SiegePlacementTest.cpp are CLEAN before adopting the 4-file pathspec, or drop them.
```
