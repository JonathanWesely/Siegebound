# TASK-870 — [HELP-RMB] the right-click reconciliation on the help screen

**Agent:** gameplay-programmer · **Date:** 2026-09-03 · **Status → `ready-for-qa`** (gate `TASK-872`)
**Law:** `HELP-§1` · `HELP-§2` · `HELP-§7` · `CARDBAR-§7` · `CARDBAR-§8` · `CARDBAR-§9` · `STACK-§4` · `SC-§37` · `SC-§38` · `SC-§39` · `SC-§40` · `SC-§41` · `SC-§42` · `TL-§5b` · `TL-§5c`

> ⛔ **No compile · no editor · no MCP · no Git.** The editor was left alone (PID 23636).
> ⛔ **No behaviour changed anywhere.** This is a text repair plus one additive test.

---

## 0. THE ONE-LINE ANSWER

`Interface.WarMap`'s detail page no longer claims the map can be **closed by right-click**. The
**Escape** half is untouched, the **poll in the controller is untouched**, and the page now
**delegates** right-click-on-the-war-map to `Interface.MapMarks` — the row that owns the gesture and
that Jonathan's same sentence ruled **TRUE**.

---

## 1. WHAT THE OBSERVATION SAID, AND WHAT I DID **NOT** DO WITH IT

🧑 **Jonathan, verbatim (the input, item (0)):**

> *"opening the war map and right clicking empty ground does not cause it to close, the map seems to
> function exactly as it should"*

⇒ `Interface.MapMarks` **TRUE** (⛔ not touched) · `Interface.WarMap` **FALSE** (⛔ repaired).

⛔ **I did not re-derive this from source, did not "verify" it by reading, and did not soften it.**
`SC-§42` is now law precisely because three careful readers reached the right answer from
`FReply::Handled()` with an instrument that could not confirm it. **A handled event is not an
actioned event.** The only thing reading could add here was a false sense of closure, so I read the
sentence, not the routing.

---

## 2. FILES TOUCHED — 3

| file | what |
|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeControlsHelpWidget.cpp` | the `Interface.WarMap` row: **one prose sentence**, its citation comment, **one appended `RelatedActionIds` entry** |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeControlsHelpWidget.h` | `ESiegeInputLane::RawNonLetter`'s **docstring only** (item (4) / `qa/TASK-816.md` N-1) |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeControlsHelpTest.cpp` | **test 17** appended + one line in the file-header index |

⛔ **Rows NOT touched, as fenced:** `Cards.Discard` · `Cards.Play` · `Cards.Cancel` ·
`Cards.StackUpgrade` · `Cards.PlacementResize` · `Interface.MapMarks`. ⛔ Also not touched:
`PickMode.Cancel` (**not** on the fence list — see §5, I chose not to).
⛔ **`SiegeControlsHelpTest.cpp:243-255`'s `RequiredIds[]` is byte-untouched.** Nothing was deleted or
renamed; the war-map row was **rewritten in place**, which that required-subset assertion cannot see.

---

## 3. THE PROSE DIFF — THE WHOLE OF IT

**Before** (`Interface.WarMap`, final paragraph):

> …that is the mechanic". **The map can also be closed by right-click or Escape, polled every
> frame;** that poll exists because a marker click opens the chat box…

**After:**

> …that is the mechanic". **Escape closes it too, polled every frame;** that poll exists because a
> marker click opens the chat box…

That is the **entire** player-facing change. ⛔ Nothing else in the page moved — the proximity gate,
the team resolution, the posture rollback and the reveal-discard paragraphs are byte-identical.

### 3.1 Why REMOVAL and not a denial sentence

⭐ **The shape is copied, not invented** (the spec's own instruction). `TASK-821` repaired the
*identical defect class* on `Cards.Discard` — a shipped page teaching a right-click route that did
not exist — by **deleting the sentence**, and test 14 keeps it deleted by asserting the page
**never names the gesture**. I applied that shipped, gate-passed remedy rather than writing
*"right-click does NOT close the map"*, because:

1. `HELP-§2`'s remedy for a control that does not exist is to stop teaching it, not to argue with it;
2. a denial sentence would have to be maintained against a mechanism nobody may re-derive (`SC-§42`);
3. the delegation edge answers the player **better**, from the row that owns the gesture.

### 3.2 The edge change — **DECLARED**, per `HELP-§7`

```
Interface.WarMap.RelatedActionIds:
  BEFORE  { Interface.WarMapReveal, Interface.WarMapMarker }
  AFTER   { Interface.WarMapReveal, Interface.WarMapMarker, Interface.MapMarks }   // ← appended
```

- ✅ It is **outbound from the row I am rewriting** — the half `HELP-§7` permits.
- ⛔ **No other row's `RelatedActionIds` was read-modified.** Nothing was re-pointed *into* my row.
- **Reason:** with the false close-sentence gone, this edge is what puts the **true owner** of
  right-click-on-the-war-map onto the page a player arrives at looking for it. `ComposeDetailContent`
  renders that row's own chip and prose underneath — `HELP-§2`'s "one definition, two renderings".
- ✅ **The id resolves.** ⚠️ A dangling edge renders **nothing** and logs **nothing**
  (`ComposeDetailContent` silently `continue`s); the registry-wide walk that catches it lives inside
  **test 9, `EveryRowHasAuthoredDetail`** — ⛔ *not* in a test named for edges. Test 17 re-asserts it
  for this row specifically, because test 17 is the one a reader opens if the delegation breaks.
- ✅ No cycle risk: `ComposeDetailContent` is **one level deep** by construction and never reads a
  related row's own `RelatedActionIds`. `Interface.MapMarks` already pointed back at
  `Interface.WarMap`; that was true before this change and is harmless.

### 3.3 The citation comment — a rot repair found in passing (`SC-§38`)

The row's citation comment carried **`the right-click/Escape poll and WHY it exists = :643-671`**.
⛔ **Measured: rotted.** The branch is `ASiegePlayerController::PlayerTick`'s `if (bWarMapOpen)` arm
→ `CloseWarMap()`, which now sits near **`:744-755`**. I replaced the coordinate with the **symbol**
and demoted the number to a dated hint. ⛔ Comment only.

---

## 4. ITEM (4) — the `RawNonLetter` docstring (`qa/TASK-816.md` N-1)

Appended exactly the clause the report specified:

> *"…or a Slate-delivered mouse gesture: the lane selects the VERBATIM-LABEL algorithm, not the
> delivery route."*

plus a short note recording **why** (`Interface.MapMarks` sits on this lane and its four gestures
arrive as Slate events on a focused widget; the **lane ruling** was measured and upheld — what was
stale was the **sentence**, which described the lane's *delivery route* as though that were its
membership test).

⛔ **The enum, its values, and every row's `Lane` are unchanged.** Comment-only.

---

## 5. ⛔⛔ ITEM (3) — I AM **NOT** ADDING THE SPELL-TARGETING ROW, AND HERE IS THE MEASUREMENT

> ⚠️ **This is a `SC-§40` finding against a QA verdict: a verdict is a citation, not a fact. I
> re-measured the premise and it does not hold.**

`qa/TASK-816.md` **W-2** states — and the board's item (3) is built on it — that *"**spell-targeting
cancel has no row at all**"*, mapping `Cards.Cancel` to "the placement cancel" only.

⛔ **Measured at source. That mapping under-reads the shipped row by two thirds.**

**`Cards.Cancel`'s shipped one-liner, verbatim:**
> *"Back out of whatever you are placing, **targeting** or circling — it never costs anything."*

**`Cards.Cancel`'s shipped detail, first sentence, verbatim:**
> *"One action, two keys, and it is the same gesture everywhere. It exits placement mode, **exits
> spell targeting**, or aborts a group-order pick at any stage, leaving every existing group and
> stance untouched."*

**And the code agrees, one handler, three modes** — `ASiegePlayerController::OnCancelPlacePressed`
(bound to `IA_CancelPlace` on `ETriggerEvent::Started`) branches
`bInPlacementMode` → `ExitPlacementMode()` · `bInTargetingMode` → `ExitTargetingMode()` ·
`GroupPickStage != None` → `CancelGroupPick()`, with the comment *"the SAME cancel action leaves
targeting mode at no cost"*. The polled RMB/Escape double-cover sits in `PlayerTick`'s
`if (bInTargetingMode)` branch → `ExitTargetingMode()`.

⇒ **`CARDBAR-§8` consumers 2 and 4 are ONE bound action and ONE help row, and that row names spell
targeting explicitly.** Adding a second row for it would:

- **duplicate a shipped page** — the exact conflation `HELP-§2` calls worse than no help screen, and
  the thing test 16 exists to prevent for the wheel;
- **contradict this registry's own declared design rule** (`Cards.Play` is *"ONE row carrying SIX
  actions … six near-identical rows would bury the fifteen commands around them"*, 704's declared
  D-1) — one bound action, one row;
- require touching `Cards.Cancel` to de-duplicate, which is **on the dispatch's do-not-touch list**.

⛔ **Reported, not fixed** — per item (5)'s own instruction and `SC-§40`. **If the manager still wants
the row, it is a one-line ruling and I will write it**, but it should be ruled with this measurement
in hand, not on W-2's premise.

### 5.1 The other half of item (3) — "each right-click row states WHEN before what"

⛔ **Fenced out and declared rather than done.** Measured, the four right-click rows read:

| row | leads with WHEN? | measured |
|---|---|---|
| `Cards.Cancel` | ✅ | *"Back out of whatever you are **placing, targeting or circling**…"* — ⛔ **fenced** |
| `Interface.MapMarks` | ✅ | *"**On the war map:** left-click empty ground to…"* — ⛔ **fenced** |
| `PickMode.Cancel` | ⚠️ partial | *"Right-click or press Escape to abandon **the order at any stage**…"* — gesture first, scope in the same clause; its whole page is inside the pick branch and it sits under the **Order pick** heading. **Not fenced — I chose not to edit a correct page.** |
| `Interface.WarMap` | n/a | ✅ **now teaches no right-click at all** |

⇒ I asserted the requirement **structurally instead of by prose edits**: test 17 proves the three
right-click rows sit in **three different categories** with **pairwise-distinct** ids, headlines,
one-liners and detail pages — i.e. three headings, three modes, no run of look-alike rows. That is
`SC-§37`'s "measure the property", and it is test 16's shipped shape.

---

## 6. THE TEST — 1 added, `Siegebound.ControlsHelp.RightClickIsTaughtOnlyByTheRowsThatOwnIt`

Test **17**, appended; ⛔ no existing test edited except one line in the file-header index.
Named so it says what it checks (`HELP-§7`: *a test's name is its discoverability surface*).

| block | asserts | can it go red? |
|---|---|---|
| (a) | right-click rows found **by scanning** `QwertyReferenceKeys`; count **== 3** | a 4th undocumented consumer, or a missing one |
| (b) | the three are `Cards.Cancel` · `PickMode.Cancel` · `Interface.MapMarks`, checked **against the scan** | a row renamed or dropped |
| (c) | pairwise-distinct id / headline / one-liner / detail / **category** | two meanings collapsed into one row |
| (d1) | `Interface.WarMap` carries **no** right-button reference key | a chip claiming RMB opens the map |
| (d2) | its composed one-liner **and** detail name none of `right-click` / `right click` / `right button` | ⭐ **the false sentence coming back** |
| (d3) | it **delegates** — `Interface.MapMarks` is a related control, and every edge resolves | the delegation being dropped |
| (e) | fixture self-checks + negative control | a scanner that matches everything/nothing |

### 6.1 ⭐ The positive control is a **same-role** one (`SC-§39`)

The six zeros in (d2) are worthless from a scanner that can never say yes. So the control is not a
synthetic literal — it runs **the identical scan over `Interface.MapMarks`'s composed detail**, which
legitimately says *"RIGHT-CLICK A CIRCLE deletes it"*. **Same needle, same role, same code path, real
registry row.** ⛔ Without it, "found nothing" is not evidence.

### 6.2 ⛔ Needle validation, BEFORE shipping (`SC-§41` cl. 8, `SC-§39`)

| # | check | result |
|---|---|---|
| 1 | banned fragments in the **war-map player prose** (one-liner + detail `TEXT(…)` lines only) | **0** ✅ |
| 2 | ⭐ **positive control** — same needle over the **map-marks player prose** | **2** ✅ *(the instrument can see)* |
| 3 | digits in the war-map player prose (`CARDBAR-§9`) | **0** ✅ |
| 4 | rows whose `QwertyReferenceKeys` contain `EKeys::RightMouseButton` | **3** ✅ |

⚠️ **And the blind-instrument moment, recorded because it is the point of the law:** my *first* grep
scanned the whole war-map **source block** and returned `4 / 1 / 1` — **all of them inside the C++
comment I had just written**, which quotes Jonathan's sentence and names the gesture. That is
`SC-§39`'s trailing-comment false-positive edge exactly. **The test itself is immune**: it scans
`ComposeOneLineForDisplay` / `ComposeDetailForDisplay` — the composed **FText**, never the source —
so a comment can never manufacture a hit for it. Re-scanning the `TEXT(…)` prose alone gave **0**.

⚠️ Same class, corroborating: `SiegeControlsHelpWidget.cpp` has a **4th** textual
`EKeys::RightMouseButton` occurrence that is a **citation comment** on the map-marks row. A
source-grep gate would count 4; the data-path scan counts **3**. ⛔ The test uses the data path.

### 6.3 ⚠️ The ceiling of 3 is deliberate — read this before "fixing" a red

`CARDBAR-§8`'s standing rule is that *"the next proposal to consume RMB starts from **this table**,
not from memory"*. A **fourth** right-click row appearing turns (a) red **on purpose**: that is the
law asking for the table to be amended, ⛔ not the test being brittle — the same relationship test 16
has with `MARK-§4`'s ceiling of three. ⛔ **If item (3)'s spell row is ever ruled in, this count moves
to 4 and `CARDBAR-§8` is amended in the same sitting.**

---

## 7. CENSUS — `TL-§5c` / `TL-§5b`

- **DECLARED, ⛔ NOT EXECUTED.** No compile, no suite run, no editor. `N/N` is a pass count and I did
  not earn one.
- **Delta: +1 test, +0 files.** `426 → 427` **declared**; files `31 → 31`.
- Baseline `426 / 0` was the last **executed** figure, at `e9df584`.
- **Needle used:** `^IMPLEMENT_[A-Z_]*AUTOMATION_TEST\(` over `Source/**/*.cpp`.
- ⚠️ **`TL-§5b` cl. 2a's trap is LIVE — I re-measured it rather than assuming yesterday's absence.**
  The bare `^IMPLEMENT_` needle returns **427 / 32** *pre-change* (and 428/32 post-change) against the
  correct **426 / 31**. **The single extra hit is named:**
  `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.cpp:6` — `IMPLEMENT_PRIMARY_GAME_MODULE(...)`.
  ⛔ An absence is a property of a tree, never a repeal.
- **Positive control on the count:** the correct needle returns a large non-zero (426) and the help
  file alone returns **17**, so the instrument is not blind.
- **Uniqueness:** no duplicate `Siegebound.ControlsHelp.*` test name; the new class name occurs
  exactly twice (its `IMPLEMENT_…` macro and its `RunTest` definition).

---

## 8. ⚠️ FOR QA TO SCRUTINISE — including the things I could not close

1. ⛔⛔ **§5 IS THE ONE THING THAT NEEDS A RULING, NOT A REVIEW.** I declined a boarded deliverable
   (item (3)'s new row) on a measurement that contradicts `qa/TASK-816.md` W-2. **Check my two
   verbatim quotes of `Cards.Cancel` and `OnCancelPlacePressed`'s three branches.** If I am wrong,
   this is a one-line fix and I will write the row.
2. ⚠️⚠️ **ESCAPE IS UNVERIFIED AND I AM SAYING SO OUT LOUD.** The dispatch fenced it
   (*"Escape is not in question — only the right-click half"*) and I obeyed. But it is the **same
   evidentiary class**: the page claims Escape closes the map, and only an **observation** can settle
   whether Escape reaches that poll while the map widget holds focus (`SC-§42` cl. 3 — board a
   one-click PIE row, do not read harder). ⛔ **I did not change it and I make no claim about it.**
   ✅ **Recommendation: one more click on Jonathan's next sitting** — *open the war map, press Escape,
   does it close?* One sentence closes the other half of this page.
3. ⚠️ **`CARDBAR-§8`'s table row 3 is now known to be incomplete about REACHABILITY.** The row is not
   false — the poll genuinely exists at `PlayerTick`'s `if (bWarMapOpen)` branch — but Jonathan's
   observation shows **right-click never arrives there**. ⛔ I did not edit `CONVENTIONS.md` (the
   manager's file). ✅ **Manager: `SC-§38` cl. 4 — a law that describes behaviour rots silently and
   instructs the next task. This row should be annotated in the same sitting this diff passes.**
4. **The `PickMode.Cancel` judgment call** (§5.1) — not fenced, and I still left it alone. If QA rules
   that item (3)'s "state WHEN first" binds it, it is a one-line one-liner rewrite.
5. **The prose reads well** is `HELP-§6`'s **Jonathan half** and I do not claim it. What I claim is
   that the page no longer teaches a control that does not exist.
6. ⛔ **`TASK-852` shares this file family and is serialised behind me.** My footprint is: one row's
   prose/comment/edges, one enum docstring, one appended test, one header index line. ⛔ I touched
   **no** shared helper, **no** `RequiredIds[]`, and **no** existing test body.

---

## 9. FENCES — self-audit

| fence | held? |
|---|---|
| ⛔ zero digits in any player-facing string (`CARDBAR-§9`) | ✅ measured **0** |
| ⛔ no `GetPositionalKey` call on any label path (`CARDBAR-§7`) | ✅ none added |
| ⛔ no shipped row's `RelatedActionIds` re-pointed (`HELP-§7`) | ✅ only my own row's, appended, declared |
| ⛔ no production behaviour change | ✅ text + comments + one test |
| ⛔ six fenced rows untouched | ✅ |
| ⛔ `RequiredIds[]` untouched, no row deleted or renamed | ✅ rewritten in place |
| ⛔ no compile / editor / MCP / Git | ✅ |
