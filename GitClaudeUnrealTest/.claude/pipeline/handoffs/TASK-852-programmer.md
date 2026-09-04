# TASK-852 — handoff (gameplay-programmer)

**[HELP-INT] The `RelatedActionIds` referential-integrity work — RE-SCOPED spec.**
Extraction + naming + the negative control. ⛔ **No second walk.**

Gate: **TASK-856**. Ships in: **TASK-857** (expected host `TASK-835`).

---

## 0. ⛔⛔ ITEM (0) — THE HARD PREMISE CHECK. **CONFIRMED**, and reported *because* it confirms (`SC-§40` cl. 9)

> ⭐ *"A silent match is indistinguishable from a skipped check."* — so here is the measurement, not the conclusion.

**Instrument:** `grep -n "FSiegeControlsHelpAuthoredDetailTest\|RelatedActionIds\|IMPLEMENT_SIMPLE_AUTOMATION_TEST"` over `Tests/SiegeControlsHelpTest.cpp` — located **by symbol**, ⛔ never by line (`SC-§38`). Then read the body in full.

✅ **PRESENT.** Test 9 `FSiegeControlsHelpAuthoredDetailTest` / `"Siegebound.ControlsHelp.EveryRowHasAuthoredDetail"`, at the pre-edit `:1085`, outer loop `for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())` at `:1103`, and the edge walk at `:1131-1137`:

```cpp
for (const FName RelatedId : Row.RelatedActionIds)
{
    TestNotNull(..., FSiegeControlsHelpRegistry::FindAction(RelatedId));
    TestNotEqual(..., RelatedId, Row.ActionId);
}
```

**Three properties checked, ⛔ not assumed:** (1) the outer loop is `GetActions()` — the **live registry**, ⛔ not `RequiredIds[]`; (2) the ids come from the live `Row.RelatedActionIds` field ⇒ **derived, ⛔ not transcribed** (`SC-§37`); (3) it also forbids **self-reference**.

⇒ **`HELP-§7`'s correction stands. The re-scope is right. A second walk would have been a duplicate.** This is now the **fourth** independent confirmation (`TASK-823` §5 → `qa/TASK-816.md` (g1) → the dispatch → me).

⚖️ **And I record why I found it in under a minute: I was handed the symbol.** That is `SC-§40` cl. 11(b) working exactly as designed, and it is the whole argument for item (1) below.

---

## 1. FILES TOUCHED — **ONE**, exactly the fence

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | test 9 edge lines **removed**; **TEST 18 added**; two comment blocks (file header index + test 9 docstring) |

⛔ **ZERO production edits.** `SiegeControlsHelpWidget.{h,cpp}` **not opened for edit** by me.
⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `RequiredIds[]` edit · ⛔ no `GetPositionalKey` on any label path.

### ⚠️ THREE OTHER FILES ARE DIRTY IN THE TREE AND **NONE OF THEM IS MINE** — stated so my pathspec is unambiguous

`git status --porcelain -- Source/` returns four modified files. **My pathspec is the one row above.** The other three, identified from their own diffs:

- `SiegeControlsHelpWidget.cpp` (+53) and `.h` (+11) — **`TASK-870`**'s war-map repair (its diff comments name `TASK-870` and Jonathan's observation). Passed `TASK-872`, awaiting commit.
- `Tests/SiegeAssistantSelectionTest.cpp` (+773) — **`TASK-874`** (its added includes name `TASK-874`).

⚠️ **For the host commit: taking `Tests/SiegeControlsHelpTest.cpp` does not sweep those in, but taking the widget pair does sweep `TASK-870`.** Not my call — flagged because `TASK-852`'s own row is the project's standing exhibit for that hazard.

---

## 2. WHAT I DID — item by item

### (1) + (1a) THE EXTRACTION — ⛔ **MOVED, ⛔ not copied**

**New: TEST 18 — `Siegebound.ControlsHelp.EveryRelatedActionIdResolvesToARealRow`**
(class `FSiegeControlsHelpRelatedEdgeIntegrityTest`)

The name carries **"Related"** *and* **"Resolves"**, per the spec's binding half.

⭐ **PROOF THAT IT IS A MOVE AND NOT A DUPLICATE — the deletion side of the diff.** `git diff -U0` over my file returns **exactly ten deleted lines**, and they are exactly test 9's edge comment + loop. Nothing else in the file was deleted. **The registry-wide walk now exists in exactly ONE place.**

⭐ **AND MEASURED FROM THE OTHER DIRECTION** — every `for (const FName RelatedId : …)` in the file, post-edit, with its owner:

⚠️ **Line numbers are dated annotations (`SC-§38`) — the SYMBOL is the key, so each row carries one.** Measured post-edit:

| line | symbol to grep | loop over | scope | owner |
|---|---|---|---|---|
| `:2070` | `FSiegeControlsHelpTowerRowsTest` | `Row->RelatedActionIds` | **subset** — `TASK-823`'s own 3 rows | test 15(f), shipped, ⛔ untouched |
| `:2406` | `FSiegeControlsHelpRightClickMeaningsTest` | `WarMapRow->RelatedActionIds` | **one row** | test 17, `TASK-870`, ⛔ untouched |
| `:2535` | **`FSiegeControlsHelpRelatedEdgeIntegrityTest`** | `Row.RelatedActionIds` over `GetActions()` | ⭐ **REGISTRY-WIDE** | **TEST 18 — mine, and the only one** |
| `:2594` | `Dangler` | `Dangler.RelatedActionIds` | 2 synthetic edges | TEST 18's negative control |

⇒ **exactly one registry-wide walk.** The two pre-existing local loops are *scoped* checks that their own comments already declare are **not** substitutes; both are shipped, reviewed rows and I did not touch either.

**(1a) — the named hazard, closed:** TEST 18 walks `GetActions()` **itself** (`:2526`). It inherits nothing from test 9. ⛔ It is **not** a subset.

✅ **WHAT TEST 9 STILL ASSERTS AFTER THE REMOVAL — as a sentence, per the spec.** Over **every** row from `GetActions()`: that detail text **exists**; that it is **AUTHORED and not the `(undocumented — TODO)` fallback** (*the claim the test exists for*); that it is **not merely the one-liner repeated**, and is **longer** than it; and that **none of eight developer-only fragments** (`.cpp:`, `.h:`, `handoffs/`, `TASK-`, `SPC:`, `**`, `` ` ``, `§`) reaches the player (`TASK-704` §4 rules T1/T2). ⇒ **everything its own name promises, undiminished.** The only thing removed was the one assertion its name never promised.

### (2) THE FAILURE MESSAGE

Both messages now name **the owning row AND the offending id**, and say **what the player loses**:

> `⛔ DANGLING EDGE - row '%s' lists related control '%s', and NO SUCH ROW IS REGISTERED. ComposeDetailContent will 'continue' past it: that block renders NOTHING and logs NOTHING, so the player silently loses a control off this page.`

> `⛔ SELF-REFERENCE - row '%s' lists ITSELF as a related control; it resolves, and the composer still drops it.`

⭐ The self-reference message states the thing that makes it a **separate condition**: a self-edge **resolves perfectly** and is still dropped, so a `FindAction`-only check cannot see it.

### (3) ⭐⭐ THE NEGATIVE CONTROL — the genuinely-missing item

**The design point: ⛔ there is ONE predicate.** Section (a)'s walk and section (c)'s control both call the *same* captureless lambda:

```cpp
auto EdgeResolves = [](const FName RelatedId) -> bool
{
    return FSiegeControlsHelpRegistry::FindAction(RelatedId) != nullptr;
};
```

⇒ *"the walk can go red"* is a claim about **the code that actually walks**, ⛔ not about a lookalike written beside it. This is the spec's *"resolve a FABRICATED id through the SAME code path"*, satisfied **literally**.

**Four assertions, in the order a reader should check them:**

1. ⭐ **POSITIVE CONTROL FIRST** (`SC-§39`) — the same lambda **DOES** resolve a real registry id. Without it, "does not resolve" is indistinguishable from a lookup that resolves nothing.
2. **NEGATIVE CONTROL** — the same lambda **REJECTS** the fabricated id.
3. ⭐⭐ **AT FULL STRENGTH** — a **synthetic row** carrying **both** defects is driven through the walk's own two conditions; each fires **exactly once**, and **on different edges**. The self-edge is deliberately an id that **resolves**, which proves the two conditions are **independent** and neither masks the other.
4. **Non-contamination** — the registry row count is unchanged after the control ran.

**⛔ DERIVED, ⛔ NOT PICKED (`SC-§40` cl. 10).** The fabricated id is **built from a real row's id** at runtime — `Rows[0].ActionId.ToString() + TEXT(".NoSuchRelatedRow")` — so it cannot quietly become real the day someone ships a row whose name an earlier author happened to guess. ⭐ **And the proof of its absence is asked of the LIVE REGISTRY** (assertion 2), ⛔ not of a text scan that could go stale. *(`Rows[0].ActionId` is currently `Hero.Move` — an observation, ⛔ never typed into the test.)*

⭐ **PRECEDENT POINTED AT, ⛔ not invented** — as the spec required: `TASK-823`'s **test 15(f)** at `:2082` (`TestNull(… FName("Cards.NoSuchUpgradeRow"))`) and `TASK-870`'s **test 17** at `:2423` (`… FName("Interface.NoSuchMapRow")`). ⚠️ Both findable by symbol as `grep 'TestNull(TEXT("NEGATIVE'`. TEST 18 generalises that shape from one task's rows to **the whole graph**, and adds the shared-predicate property neither has.

### (4) DERIVED, NEVER TRANSCRIBED

⛔ **There is no hand-typed edge list anywhere in TEST 18.** Rows from `GetActions()`, ids from the live field. **The test does not know how many edges exist and must not** — the count is emitted via `AddInfo` (informational), ⛔ never asserted.

⭐ **The one number-shaped assertion is a VACUITY GUARD, and it is deliberately not a count:** `EdgesWalked > 0`. A walk over zero edges is green and proves nothing; asserting *how many* would rot on the next `RelatedActionIds` edit. **Pin the relationship, never the number.**

### (6) EXPECTED DELTA `+1` — ✅ **MET EXACTLY**

Tests in `SiegeControlsHelpTest.cpp`: **17 → 18.** One new `IMPLEMENT_SIMPLE_AUTOMATION_TEST`; **test 9 survives, smaller.**

---

## 3. ⛔ `HELP-§7` DECLARATION — **ZERO edge changes**

⛔ **I changed no row's `RelatedActionIds`. I changed no row at all.** I did not open a production file for edit.

⛔ **Not touched, and I confirm each by name:** `Cards.Discard` · `Cards.Play` · `Cards.Cancel` · `Cards.StackUpgrade` · `Cards.PlacementResize` · `Interface.MapMarks` · `Interface.WarMap`.

**THE TWO COUNTS, RE-MEASURED BY ME BEFORE AND AFTER — ⛔ BOTH UNMOVED:**

| needle | file | before | after |
|---|---|---|---|
| `\.RelatedActionIds\s*=` | `SiegeControlsHelpWidget.cpp` | **16** | **16** ✅ |
| `AddRow(` | `SiegeControlsHelpWidget.cpp` | **27** | **27** ✅ |

⇒ **neither count moved, because I made zero production edits.**

### ⚠️ `SC-§41` FIRED ON THE COUNT I WAS HANDED — the bare token over-counts by **7**

The dispatch said *"exactly 16 `RelatedActionIds` blocks"*. **A bare `grep -c "RelatedActionIds"` on that file returns 23, not 16.** The 7-hit gap is **6 comment lines + 1 consumption site** (`Content.Related.Reserve(Row.RelatedActionIds.Num())`). The figure **16** is only recoverable with the **assignment shape** `\.RelatedActionIds\s*=`.

⇒ ⛔ **The relayed number was RIGHT; the obvious needle for it is WRONG.** Exactly `SC-§41` cl. 1 — pin a **shape**, never a bare token. Recorded so the next reader who re-measures this does not report a phantom `+7` drift.

---

## 4. ⛔ FINDINGS — **REPORTED, ⛔ NOT REPAIRED** (spec item (5): a repair here would be a content change hiding inside a test task)

**F-1 — ⛔ A STALE COUNT IN A SHIPPED PRODUCTION COMMENT.** `SiegeControlsHelpWidget.cpp:492` reads *"(the **13** `RelatedActionIds` assignments in this file were read)"*. **The measured figure is 16.** The comment predates `TASK-823`'s 9 added edges and `TASK-870`'s work. ⛔ **Not repaired — it is in a production file, outside my fence.** ⚖️ It is `SC-§40` cl. 10's shape in a comment: *a count carries no visible timestamp and nothing in it goes stale-looking.* Suggest a one-line rider on whichever row next legitimately owns that file.

**F-2 — ⛔ ZERO DANGLING EDGES.** The walk is green across the whole registry: **every** outbound edge resolves and **no** row lists itself. ⇒ I found nothing to report under the spec's *"if you find a dangling edge, REPORT IT"* clause. ⚠️ Stated explicitly because **the absence of a finding is itself a measurement** — and it is exactly the state that made the negative control necessary.

**F-3 — the walk was invisible in BOTH places a reader looks.** Test 9's **name** said "detail authoring" *and* its **docstring never mentioned the walk either**. ⇒ the two readers who called it absent had nothing to find in either surface. I fixed both directions (see §5).

---

## 5. DISCOVERABILITY — the reason two readers missed it, closed in **three** directions

1. ⭐ **The new test's NAME** — `EveryRelatedActionIdResolvesToARealRow`. Says "Related" and "Resolves".
2. ⭐ **The FILE-HEADER INDEX** now carries a `TEST 18` entry that names the extraction, states it was read as absent by two readers, and closes with the operational instruction: ⛔ **search for the SYMBOL, not for the test name you would have chosen.**
3. ⭐ **BACK-POINTERS AT THE OLD SITE, both surfaces** — test 9's **docstring** now says what it no longer does and where the walk went (⚠️ *the docstring specifically, because that is where the last two readers looked and found nothing*), and a comment **at the exact deletion site** points forward to TEST 18 and says ⛔ *do not re-add an edge loop here.*

⇒ a future reader who greps `RelatedActionIds`, `FindAction`, `EveryRowHasAuthoredDetail`, or the new test name lands on the walk from **any** of them.

---

## 6. ⚠️ THREE DEFECTS I FOUND **IN MY OWN WORK** BEFORE SHIPPING IT — reported against myself

**D-1 — ⛔ I WROTE A TAUTOLOGY INTO THE NEGATIVE CONTROL, IN THE SECTION WHOSE ENTIRE PURPOSE IS PROVING AN ASSERTION CAN FAIL.**
My non-contamination check was first written `TestEqual(…, GetActions().Num(), Rows.Num())`. **`Rows` is a `const TArray&` BOUND TO THAT SAME function-local static** ⇒ that is `X == X`, and it **could never go red**. **Fixed:** the count is now snapshotted **by value** (`const int32 RowCountBefore`) before the control runs, and the comment states why the distinction is load-bearing rather than stylistic. ⚖️ *Recorded because it is `SC-§39`'s own failure mode committed inside `SC-§39`'s own remedy — the same shape as `SC-§40` cl. 10's law that handed out a live value as its example of a synthetic one.*

**D-3 — ⛔ I TYPED A SYMBOL INSTEAD OF MEASURING IT, IN THE HANDOFF SECTION WHOSE POINT IS THAT SYMBOLS ARE THE KEY.** My first draft of §2's table cited test 15 as `FSiegeControlsHelpTowerAndMapMarkTest`. **No such symbol exists** — I derived it from the test's *display name* (`…TowerAndMapMarkRowsAreAuthoredAndRawLaned`) instead of reading the class. **Measured value: `FSiegeControlsHelpTowerRowsTest`.** Fixed before shipping. ⚖️ *Recorded because a wrong symbol in a `grep-this` table is worse than no table: it returns zero hits and reads as confirmation of absence — the exact `SC-§40` cl. 11(b) failure this whole task exists to close.*

**D-2 — A STYLE DEVIATION I MEASURED RATHER THAN ASSUMED.** My first draft put **em dashes inside `TEXT()`** failure strings. I scanned the shipped corpus with **three positive controls in the same needle role**: `⛔` inside `TEXT()` = **28**, `⭐` = **28**, `§` = **9**, **em dash = 0**. ⇒ the zero is a **measurement**, not a blind grep — the file freely uses non-ASCII in `TEXT()` but has **never** used an em dash there. **Fixed:** both converted to `-`, matching the two negative-control precedents I was told to point at. Whole-file re-measure: em dash inside `TEXT()` = **0**, positive control now **32**.

---

## 7. CENSUS — ⛔ **A DELTA. `declared`, ⛔ NEVER A PASS COUNT** (`TL-§5c`)

**MY DELTA: `+1` declared, `+0` files.**

| | declared | files |
|---|---|---|
| baseline, measured by me **before** editing | **427** | **31** |
| after | **428** | **31** |

⭐ **My measured baseline of `427 / 31` reproduces the dispatched tree figure exactly** — an independent corroboration, ⛔ not a restatement.

⚠️ **NEEDLE DECLARED, and the second needle run for reconciliation** (`TL-§5b` cl. 2a — *an absence is a property of a tree, never a repeal*): my needle is `^IMPLEMENT_SIMPLE_AUTOMATION_TEST|^IMPLEMENT_COMPLEX_AUTOMATION_TEST` (427→428 / 31). The **bare** `^IMPLEMENT_` needle reads **428→429 / 32** on the same tree. ⇒ **both needles move by exactly `+1`**, which is what proves the delta is real and ⛔ not a needle artefact.

⛔⛔ **I EXECUTED NOTHING.** No compile, no suite, no editor. **The last EXECUTED figure remains `426` at `e9df584` and I have not touched it.** `428` is a **declaration count**. ⛔ Nobody may write `428/428`.

---

## 8. 🔍 WHAT QA SHOULD SCRUTINISE HARDEST

1. ⛔⛔ **THAT IT IS A MOVE.** Take `git diff -U0` and read **every deleted line** — there are ten and they are all test 9's. Then re-run the edge-loop table in §2 yourself. **A second registry-wide walk is an automatic fail and I am handing you the instrument to catch me.**
2. ⭐⭐ **THE CONTROL'S SHARED PREDICATE.** Confirm the walk and the control call the **same** `EdgeResolves` lambda. If a reviewer can find *any* path where the control tests something the walk does not use, the control is theatre.
3. ⛔ **THE SELF-EDGE SEPARATION.** `Dangler.ActionId = RealId` is load-bearing: it makes the self-edge **resolve**, so `Unresolved == 1` and `SelfEdges == 1` fire on **different** edges. ⚠️ **If `Dangler.ActionId` were left as `MakeRow`'s `Test.SyntheticRow`, `Unresolved` would be 2 and that assertion would be wrong.** This is the single most breakable line in my diff.
4. ⛔ **TEST 9 STILL EARNS ITS NAME** — §2's sentence. Verify against the body, not against my prose.
5. ⚠️ **D-1 IN §6** — check I actually fixed the tautology and did not just describe it.
6. ⚠️ **COMPILE-SHAPE, since nothing here was compiled** (`TASK-835` holds the editor; ⛔ I did not disturb it): `TestNotEqual(const TCHAR*, FName, FName)` is **moved verbatim** from shipped code, so its overload resolution is unchanged. `TestEqual(TEXT(…), int32, int32)` matches test 17's shipped `TestEqual(TEXT(…), RightClickRows.Num(), 3)`. `TestTrue(…, EdgeResolves(Id))` replaces the old `TestNotNull(FindAction(Id))` — **identical predicate** (`!= nullptr`), changed only so both sections share one lambda. Every `%s`/`%d` is arg-matched; the file has **no BOM before or after my edit** (unchanged).
7. ⚠️ **F-1** — the stale `13` at `SiegeControlsHelpWidget.cpp:492`. **Confirm I was right to leave it** rather than repair it (spec item (5)).

---

## 9. ⛔ WHAT I DID NOT DO

⛔ No second walk · ⛔ no production edit · ⛔ no edge change · ⛔ no `RequiredIds[]` edit · ⛔ no other test edited (test 15(f) and test 17's local loops untouched) · ⛔ no shipped row touched · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no dangling-edge "repair" (there were none to repair — F-2).

## 10. 📌 M8 DECLARATION (`HELP-§5`, verbatim shape)

Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. **This is a test file; it adds no shipped surface at all.**
