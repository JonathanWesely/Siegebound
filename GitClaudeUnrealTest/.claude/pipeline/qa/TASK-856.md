# QA Report — TASK-856 (gate over `TASK-852` ALONE)

**Verdict: PASS** — `TASK-852` = **PASS**.
**Blockers: 0** · WARN: 4 · NIT: 2 · Rulings: 1
**Reviewer:** qa-reviewer · **Date:** 2026-09-03 · ⛔ no code edited, ⛔ no compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git.

---

## 0. ⛔ `SC-§29` COVERAGE LEDGER

| task | in this gate? | why |
|---|---|---|
| **TASK-852** (`RelatedActionIds` edge-integrity extraction) | ✅ **COVERED — PASS** | the only task this row names |
| **TASK-870** (`SiegeControlsHelpWidget.{h,cpp}` + test 17) | ⛔ **NOT GATED HERE** | gated by `TASK-872` (PASSED). Its test 17 sits **in the same file** — see **W-4**. I read around it, never through it. |
| **TASK-874** (`Tests/SiegeAssistantSelectionTest.cpp`) | ⛔ **NOT GATED HERE** | still ungated; disjoint file |

⛔ **This gate covers `TASK-852` and nothing else.** Fence measured: one file, `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp`. **Zero production edits** — corroborated below by both widget-file counts being unmoved.

---

## 1. ⛔ ROW (g) — ITEM (0)'s PREMISE CHECK: **RUN, and REPORTED ALTHOUGH IT CONFIRMED**

✅ **Not a WARN.** Handoff §0 carries all three things `SC-§40` cl. 9 demands and cl. 11 makes cheap: the **instrument** (grep over the test file), the **symbol** (`FSiegeControlsHelpAuthoredDetailTest` / `"Siegebound.ControlsHelp.EveryRowHasAuthoredDetail"`), and **what it returned** — present, outer loop `GetActions()`, edges resolved through `FindAction`, self-reference forbidden. It then states the three properties as *checked, not assumed*.

⭐ *"A silent match is indistinguishable from a skipped check."* — this one was not silent. It is now the **fourth** independent confirmation (`TASK-823` §5 → `qa/TASK-816.md` (g1) → the dispatch → the programmer), and I did not make it a fifth by re-deriving it: I gated the **consequence** instead, which is row (a).

---

## 2. ⛔⛔ ROW (a) — **EXACTLY ONE REGISTRY-WIDE WALK.** Censused by me, ⛔ not taken from the handoff

**Needle:** `for (const FName RelatedId :` over the whole file. **Positive control (`SC-§39`):** the needle returns **4** hits, so it is not blind. Cross-checked against the broader `RelatedActionIds` needle (**17** hits in this file), every one of which I read.

| line | owner symbol | loops over | scope | verdict |
|---|---|---|---|---|
| `:2070` | `FSiegeControlsHelpTowerRowsTest` | `Row->RelatedActionIds` | **subset** — test 15(f), driven by `TASK-823`'s own 3-row fixture table | ⛔ untouched; its own comment `:2064-2066` declares it **not** a substitute |
| `:2406` | `FSiegeControlsHelpRightClickMeaningsTest` | `WarMapRow->RelatedActionIds` | **one row** — test 17 | ⛔ untouched |
| `:2535` | **`FSiegeControlsHelpRelatedEdgeIntegrityTest`** | `Row.RelatedActionIds` over `Rows` = `GetActions()` (`:2508`, `:2526`) | ⭐ **REGISTRY-WIDE** | **TEST 18 — the only one** |
| `:2594` | same test | `Dangler.RelatedActionIds` | 2 synthetic edges on a **local** row | TEST 18's own negative control |

⇒ ✅ **ONE registry-wide walk. The automatic-fail condition did NOT occur.** And **test 9 now carries zero edge loops** — I read its body in full (`:1104-1157`), not its docstring.

⚠️ **All four symbols in the handoff's own census table are correct** — I re-derived each from its `IMPLEMENT_SIMPLE_AUTOMATION_TEST` rather than from a display name (`:1906`, `:2268`, `:2491`). **D-3's fix holds**: `FSiegeControlsHelpTowerRowsTest` is real, and its display name really is `…TowerAndMapMarkRowsAreAuthoredAndRawLaned` — the exact trap he described.

### ⭐⭐ AND THE MOVE ITSELF, PROVEN WITHOUT A DIFF — the deletion side reconstructed from **two independent prior anchors**

⛔ I have no Git by design, so I could not read `git diff`'s deletion side. I reconstructed it arithmetically and it **closes exactly**:

- `qa/TASK-872.md` §2 independently measured test 9's edge `for` at **`:1131`** (post-`870`, pre-`852`).
- `handoffs/TASK-852-programmer.md` §0 cites the pre-edit walk at **`:1131-1137`** — **the same anchor, from a different reader, at a different time.**
- Post-edit the file adds: the TEST 18 file-header paragraph `:29-36` (**+8**) and test 9's docstring paragraph `:1092-1097` (**+6**); it removes the walk (**−10**: 1 blank + 2 comment + the 7-line loop, i.e. `:1128-1137`); it adds the back-pointer block `:1142-1153` (**+12**).
- Pre-edit, test 9's `RunTest` would have closed at `:1141`. **1141 + 8 + 6 − 10 + 12 = 1157.** **Measured closing brace: `:1157`.**

⇒ ⭐ **The deletion is exactly ten lines and nothing else left test 9.** Two anchors and the closing brace all reconcile to the line.
⚠️ **The honest limit (`SC-§32`):** an offset reconciliation cannot see an **in-place, equal-line-count** edit. It is corroboration of diff strength, ⛔ not a diff. Build-master has git — **W-4** hands it the one byte-identity question that matters.

---

## 3. ⛔⛔ ROW (b) — **COVERAGE NOT NARROWED**

- TEST 18's outer loop is `for (const FSiegeControlsHelpAction& Row : Rows)` where `Rows = FSiegeControlsHelpRegistry::GetActions()` (`:2508`). ⛔ **Not `RequiredIds[]`, not a fixture list, not a subset.** The comment at `:2520-2522` names that hazard by its history.
- ⭐ **The vacuity guard is real and is deliberately not a count:** `TestTrue(…, EdgesWalked > 0)` (`:2561`). A walk over zero edges is green and proves nothing; this forbids that state without pinning a number that would rot on the next edge edit. The count goes to `AddInfo` (`:2564`), ⛔ never asserted. ✅ `SC-§37` satisfied — **there is no hand-typed edge list anywhere in TEST 18.**
- ✅ **Ids are derived from the live field**, rows from the live registry. It covers edges added after it was written.

---

## 4. ⭐⭐ ROW (c) — **THE NEGATIVE CONTROL. It CAN go red, and NO condition is unreachable.**

**Traced by hand, not accepted.**

**(i) One predicate, and it is the walking one.** `EdgeResolves` (`:2503-2506`) is called at **`:2546` (the walk)**, `:2579` (positive control), `:2582` (negative control), `:2596` (full-strength control). ⛔ **There is no second lookup anywhere in the test** — I checked every `FindAction` in TEST 18's span and the lambda is the only one. ⇒ *"the walk can go red"* is a claim about the code that **actually walks**.

**(ii) Positive control FIRST** (`:2578-2579`) — the same lambda **does** resolve `RealId`. Without it, "does not resolve" is indistinguishable from a lookup that resolves nothing (`SC-§39`). ✅ Ordered correctly.

**(iii) The fabricated id is DERIVED, ⛔ not picked** (`SC-§40` cl. 10): `RealId.ToString() + TEXT(".NoSuchRelatedRow")` (`:2572-2574`). It cannot quietly become real. ⭐ **And its absence is asked of the LIVE REGISTRY** (`:2582`), not of a text scan that could go stale.

**(iv) ⭐⭐ THE SINGLE MOST BREAKABLE LINE, and it is correct.** `Dangler.ActionId = RealId` (`:2589`) with `RelatedActionIds = { RealId, FabricatedId }` (`:2590`). Hand-traced:

| edge | `EdgeResolves` | `== Dangler.ActionId` | effect |
|---|---|---|---|
| `RealId` | **true** (resolves) | **true** | `SelfEdges` → 1 |
| `FabricatedId` | **false** | false | `Unresolved` → 1 |

⇒ `Unresolved == 1` and `SelfEdges == 1`, **firing on DIFFERENT edges**. ✅ **Both conditions are reachable, both are exercised exactly once, and neither masks the other.** The self-edge **resolves on purpose**, which is precisely what proves a `FindAction`-only check cannot see it.

**(v) ⛔ A CRASH GUARD I CHECKED BECAUSE `Rows[0]` DEMANDS ONE.** `:2510-2513` early-returns if `Rows.Num() == 0` **before** `Rows[0]` is indexed at `:2572`. ✅ No out-of-bounds on an empty registry.

**(vi) Non-contamination** (`:2618-2619`) — `Dangler` is a **local**; it never enters the registry. See **D-1** below.

---

## 5. ✅ ROW (d) — **TEST 9 STILL EARNS ITS NAME**, verified against the body

Over **every** row from `GetActions()` (`:1117`), test 9 still asserts: detail **exists** (`:1122`) · detail is **AUTHORED and not the `(undocumented — TODO)` fallback** (`:1125` — *the claim it exists for*) · detail is **not the one-liner repeated** (`:1131`) · detail is **longer** than the one-liner (`:1133`) · **none of eight developer-only fragments** reaches the player (`:1111-1115`, `:1136-1140`, `TASK-704` §4 rules T1/T2).
⇒ ⭐ **Everything its own name promises, undiminished. The only assertion removed is the one its name never promised.**

### ⛔ `RequiredIds[]` — **UNEDITED. Verified twice, by two different instruments.**
1. **Content read** at `:256-268`: 25 ids, including `Cards.Play` / `Cards.CursorHold` / `Cards.Discard` / `Cards.Cancel`, asserted as a **required subset** by `TestNotNull` in a loop (`:270-274`).
2. ⭐ **Span arithmetic against an independent prior measurement:** `qa/TASK-872.md` N-1 measured it at `:248-260`. It is now `:256-268` — **moved +8 with an unchanged 13-line span**, and **+8 is exactly the TEST 18 file-header paragraph**. ⇒ no id added, none removed.
⛔ **The `CARDBAR-§9` automatic-fail condition did not occur.**

### ⭐ BOTH RELAYED PRODUCTION COUNTS RE-MEASURED BY ME — both unmoved, which is the fence held
| needle (⛔ shape, not token) | file | measured |
|---|---|---|
| `AddRow\(` | `SiegeControlsHelpWidget.cpp` | **27** ✅ |
| `\.RelatedActionIds\s*=` | `SiegeControlsHelpWidget.cpp` | **16** ✅ |

---

## 6. ⛔ ROW (e) — **THE FAILURE MESSAGE, READ AS A STRANGER WOULD**

> `⛔ DANGLING EDGE - row '%s' lists related control '%s', and NO SUCH ROW IS REGISTERED. ComposeDetailContent will 'continue' past it: that block renders NOTHING and logs NOTHING, so the player silently loses a control off this page.` (`:2543-2546`)

✅ **Names the owning row AND the offending id**, and says **what the player loses**. A stranger reading one red line knows which page points where and why nobody noticed. The self-reference message (`:2551-2554`) states the property that makes it a **separate** condition. Every `%s`/`%d` is arg-matched (`:2544`, `:2552`, `:2564`, `:2578`, `:2581`). ✅ **Discoverability half landed.**

---

## 7. ⚠️ THE THREE SELF-REPORTED DEFECTS — **weighed, and each verified FIXED rather than described**

**D-1 — THE TAUTOLOGY INSIDE THE NEGATIVE CONTROL. ✅ GENUINELY FIXED, and I swept the file for the species.**
`const int32 RowCountBefore = Rows.Num();` is snapshotted **by value** at `:2517` and compared at `:2619` against `FSiegeControlsHelpRegistry::GetActions().Num()`. ⛔ `Rows.Num()` appears nowhere in that assertion. The comment at `:2614-2617` states *why* the distinction is load-bearing rather than stylistic — the right thing to leave behind.
⭐ **AND NOTHING ELSE IN THE FILE HAS THAT SHAPE.** I read **every** `TestEqual` / `TestNotEqual` in the file (66 call sites). ⛔ **No other assertion has both sides derived from the same live expression.** The nearest neighbours all compare an **output against an input** and can all fail: `:623` (`RawResolved.Num()` vs `RawKeys.Num()`), `:1272` (Qwerty vs Dvorak composition), `:1399` (`Page.Related.Num()` vs the registry field), `:2038` (`Resolved.Num()` vs the reference keys). ⇒ **the species is clean file-wide.**
⚖️ Weighing it: a control that cannot fail, written inside the control, is the exact defect this row exists to remove. **He found it himself, fixed it, and named it as `SC-§39`'s own failure mode committed inside `SC-§39`'s own remedy.** That is the standard, and it is met.

**D-2 — EM DASHES IN `TEXT()`. ✅ FIXED, and I reproduced the corpus measurement independently.**
My needle `TEXT\("[^"]*—` over the whole file = **0**. ⭐ **Positive control in the identical needle role** (`SC-§41` cl. 3): `TEXT\("[^"]*⛔` = **32**. ⇒ the zero is a **measurement**, not a blind grep, and my 32 reproduces his post-fix figure independently. Both new failure strings use `-` (`:2544`, `:2552`), matching the two shipped precedents. Em dashes survive only in `/* */` prose, which is corpus-consistent.

**D-3 — THE GUESSED SYMBOL. ✅ FIXED** — verified at `:1906` (above). ⚖️ Correctly weighed by him: a wrong symbol in a *grep-this* table returns zero hits and **reads as confirmation of absence** — the exact `SC-§40` cl. 11(b) failure this task exists to close.

⇒ **The self-reporting is COMPLETE for the three it names.** ⛔ **One further defect survives, unreported — WARN-1.**

---

## Findings

- **[WARN-1] ⛔ THE FOURTH DEFECT — a back-pointer in the edited file is now FALSE.** `Tests/SiegeControlsHelpTest.cpp:2403-2405` (test 17, `TASK-870`'s) reads *"…`continue`s past a dangling id). **Test 9 walks the whole registry for this**; the claim is repeated here because THIS test is the one that would be read if the delegation broke."* ⛔ **Test 9 no longer walks the registry — TEST 18 does.** ⇒ this is `SC-§40`'s own species: prose describing code in the present tense, shipped inside the artefact it describes, now untrue. **Not a blocker** — it is non-executable, and a reader who follows it to test 9 hits the docstring (`:1093-1097`) and the deletion-site comment (`:1142-1153`), both of which point at TEST 18, so it self-corrects in **one hop**. ✅ **He was RIGHT not to edit it** (spec item (5) fenced him to test 9's edge lines; touching test 17 would have been the fence breach). ⛔ **What is owed is that he did not REPORT it**, and his §5 claim that a reader *"lands on the walk from any of them"* is the one sentence in the handoff that overstates. *Fix:* one-word repair (`Test 9` → `TEST 18`) on whichever row next legitimately edits that file — ⚠️ **naturally the same host as `TASK-857`**, so it costs nothing.
- **[WARN-2] `handoffs/TASK-852-programmer.md` §8 item 3 — the counterfactual arithmetic is WRONG, though the conclusion is right.** It states: *"If `Dangler.ActionId` were left as `MakeRow`'s `Test.SyntheticRow`, `Unresolved` would be 2."* ⛔ **It would be 1.** `EdgeResolves` consults the **registry**, not `Dangler`, so `RealId` resolves regardless of what `Dangler.ActionId` holds; what would break is `SelfEdges`, which would be **0**, failing the `:2608` assertion instead of the `:2606` one. The claim is only true under a *different* counterfactual (writing the self-edge as `{ Dangler.ActionId, FabricatedId }`). ⇒ ⛔ **The code is correct; the prose about the code is not** — flagged because *"accurate about your own diff"* is the standard this row was re-scoped to enforce, and because the line he nominated as **"the single most breakable line in my diff"** is the one he mis-described. *Fix:* correct it in the record; no code change.
- **[WARN-3] ⛔ F-1 ROUTED — `SiegeControlsHelpWidget.cpp:492` says *"the **13** `RelatedActionIds` assignments in this file were read"*; measured **16**.** ✅ **Confirmed at source, and CONFIRMED CORRECT TO LEAVE:** it is a production file, outside his one-file fence, and spec item (5) forbids exactly this repair inside a test task. **See the ruling below for where the rider belongs.**
- **[WARN-4] ⛔⛔ BUILD-MASTER — TWO OPEN COMMIT ROWS NAME THE SAME FILE.** `Tests/SiegeControlsHelpTest.cpp` is the **sole** pathspec of **`TASK-857`** (this work) **and** one of the four pathspecs of **`TASK-873`** (`TASK-870`'s build) — `qa/TASK-872.md` §2 records test 17 as *"a pure append at `:2210-2425`"* in this same file, still uncommitted. ⇒ **whichever commits first sweeps the other's test in.** ✅ **No hard-gate breach either way** — `TASK-870` PASSED `TASK-872` and `TASK-852` passes here, so nothing ungated ships — but **the adopting handoff must NAME BOTH ROWS**, or one will read as uncommitted while its test has shipped. ⛔ **I could not run `git status` (no Git by design): verify before staging** — *a `names:` line is a citation; `git status` is the measurement.*
- **[NIT-5] The shared-predicate property is exact for the RESOLVE half and a re-enactment for the SELF-EDGE half.** The walk's resolve condition and the control both call `EdgeResolves`; the walk's self-edge condition is `TestNotEqual(…, RelatedId, Row.ActionId)` (`:2551-2554`) and the control re-enacts it as `RelatedId == Dangler.ActionId` (`:2600`). **Same operator, same `FName` type, so the risk is very small** and I am not gating on it — but the handoff's blanket *"walk and control call the same lambda"* is precise only for one of the two conditions. ⭐ *The shape that would close it:* a second captureless `IsSelfEdge(RelatedId, OwnerId)` lambda, symmetrical with `EdgeResolves`. Worth doing only if this test is ever extended.
- **[NIT-6] The handoff's "second needle" is a second SCOPE, and its last-executed citation is superseded.** (a) Bare `^IMPLEMENT_` over `Tests/*.cpp` reads **428 / 31 — identical to the full needle**, ⛔ not `429/32`. It reads `429/32` only when the scope widens to `Source/GitClaudeUnrealTest/**/*.cpp`, where the 32nd file is `GitClaudeUnrealTest.cpp` carrying `IMPLEMENT_PRIMARY_GAME_MODULE`. **Both figures are right under their own scope and both move `+1`, so the reconciliation stands** — but the reason is scope, not needle. Recorded so nobody hunts a phantom test. (b) §7 cites the last executed figure as *"426 at `e9df584`"*; the current figure is **`427 Result={Success}` / `0 Result={Fail}` at `1aa0fee`**. Stale-at-writing citation, ⛔ not a wrong measurement.

---

## ⚖️ RULING — WHERE THE F-1 RIDER BELONGS

⛔ **NOT on `TASK-870` / `TASK-873`.** `SiegeControlsHelpWidget.cpp` is theirs by possession, but **`TASK-870` has already passed its gate (`TASK-872`)**. Appending a prose repair to it now would put an **ungated hunk inside a gated diff** — strictly worse than a stale count, and the exact shape the hard gate exists to refuse.

✅ **THE RIDER BELONGS ON THE NEXT ROW THAT LEGITIMATELY OPENS `SiegeControlsHelpWidget.cpp` FOR EDIT AND IS GATED *AFTER* THE REPAIR.** The manager boards it as a one-line rider on the next HELP-lane content row. It is recorded here and on the board so it does not rot further.

⭐ **AND THE REPAIR MUST NOT RE-TYPE `16`.** Follow `qa/TASK-816.md` row (g2)'s shipped pattern: there, the same species was fixed by **replacing the number with its derivation** (`GetActions().Num()`, with `27` surviving only as a dated observation). A hand-typed `16` rots on the next `RelatedActionIds` append exactly as `13` did — ⛔ *a count carries no visible timestamp and nothing in it goes stale-looking.*

⚠️ **And `SC-§41` on the needle, so the repairer does not report a phantom:** a bare `grep "RelatedActionIds"` on that file returns **23** — **6 comment lines + 1 consumption site** (`Content.Related.Reserve(Row.RelatedActionIds.Num())`) on top of the 16. **The figure 16 is recoverable ONLY with the assignment shape `\.RelatedActionIds\s*=`.** I used that shape; the relayed number was right and the obvious needle is wrong.

---

## ⛔ CENSUS — `TL-§5c` / `TL-§5d`

> **`428` declared across `31` files.** Needle `^IMPLEMENT_SIMPLE_AUTOMATION_TEST|^IMPLEMENT_COMPLEX_AUTOMATION_TEST`, scope `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, taken at review time. Per-file: **`SiegeControlsHelpTest.cpp` = 18.**

✅ **The declared `+1` is EXACT and I reproduced it independently** (`427 → 428`, `31 → 31` files, this file `17 → 18`). Both needles move `+1` — see **NIT-6** for the scope correction to the handoff's reconciliation.

⛔⛔ **`428` IS A `declared` FIGURE. I EXECUTED NOTHING** — no compile, no suite, no editor. ⛔ **Nobody may write `428/428`.**
⭐ **The last EXECUTED figure is `427 Result={Success}` / `0 Result={Fail}` at `1aa0fee`**, and the zero carries its positive control (a synthetic row read `1`; the real log read `0`), so the instrument is proven readable rather than absent.

⚠️ **`TL-§5d` — THE HOST, NAMED.** TEST 18 exists **only in the working tree and is invisible to `HEAD`**; nothing will ever go red to say so. Its host is **`TASK-857`**, riding the first commit-gate build dispatched after this PASS (**expected host `TASK-835`**), pathspec `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp`. ⛔ **Until that commit lands, this PASS covers code that no commit carries.**

---

## Notes for build-master (`TASK-857` / its host)

1. ⛔⛔ **W-4 IS THE ONE THAT CAN BITE.** Staging `Tests/SiegeControlsHelpTest.cpp` takes **TEST 18 (`TASK-852`) AND TEST 17 (`TASK-870`)** in one blob. Both are gated; **name both rows in the handoff.** ⛔ Take `git status --porcelain` and `git diff --stat` first — do not derive the pathspec from any prose, including mine.
2. ⛔ **Taking the widget pair sweeps `TASK-870`; taking `Tests/SiegeAssistantSelectionTest.cpp` sweeps `TASK-874`, which is UNGATED.** ⛔ That third file must not enter this commit.
3. ✅ **THE ACCEPTANCE ROW, BY NAME AND BOTH HALVES** (`TASK-857` item (3)): `Siegebound.ControlsHelp.EveryRelatedActionIdResolvesToARealRow` reports `Result={Success}` **AND** `Siegebound.ControlsHelp.EveryRowHasAuthoredDetail` reports `Result={Success}`. ⛔ **A total can hide a loop cut out of a shipped test — report both by name.**
4. ⭐ **Report `Result={Fail}` verbatim, including when it reads `0`.** An **absent** `Result` line is an unreadable instrument, ⛔ not a green suite (`TL-§5c`).
5. ⚠️ **Expect your executed `N` to be its own number.** Take a fresh census; reconcile `fresh == prior + Σ(declared deltas)`; ⛔ reconcile to no published absolute. My `428/31` will be stale.
6. **Compile-shape notes, since nothing here was compiled:** `TestNotEqual(const TCHAR*, FName, FName)` is the **same shape already shipped and compiled** at `:2075-2076` (test 15(f)) ⇒ overload resolution is proven, not assumed. `TestEqual(TEXT(…), int32, 1)` matches ~12 shipped sites in this file. `MakeRow`'s signature at `:196` matches the call at `:2588` (4 args, correct types), and `using namespace SiegeControlsHelpTestUtils;` is present at `:2497`. `AddInfo` is already used at `:1059`. All new code sits inside `#if WITH_DEV_AUTOMATION_TESTS` (`:20` / `:2624`). No deprecated or removed UE 5.8 API in the diff surface.

---

## ⛔ What this gate did NOT and COULD NOT verify (`SC-§32`)

⛔ No compile · ⛔ no test executed · ⛔ no editor, no MCP (the editor is UP, PID 11892, with an art task running — ⛔ I did not disturb it) · ⛔ **no `git diff`** — the ten-line deletion is proven by **arithmetic reconciliation of two independent prior anchors**, which cannot see an in-place equal-line-count edit (**W-4** hands that to the agent with git) · ⛔ I did not re-derive item (0)'s premise a fifth time; I gated its consequence · ⛔ `TASK-870`'s and `TASK-874`'s diffs are **not** cleared by this report · ⛔ that any help page **reads well** is Jonathan's under `HELP-§6` and no agent may claim it.

---

## Board status lines (⛔ apply by line edit — do NOT whole-file write this board)

```
TASK-852 → status: ✅ **qa-passed (2026-09-03, gate `TASK-856`) — ⛔ 0 BLOCKERS.** ⭐ MOVE CONFIRMED: exactly ONE registry-wide walk file-wide (QA's own 4-hit edge-loop census; test 15(f) + test 17 untouched as subset checks), deletion side reconciled to EXACTLY 10 lines against TWO independent prior anchors (`qa/TASK-872.md` `:1131` + the handoff's own cite) with the closing brace landing at `:1157` as predicted. ⭐⭐ NEGATIVE CONTROL VERIFIED CAN-GO-RED: one shared `EdgeResolves` lambda, derived (not picked) fabricated id, positive control first, `Rows[0]` crash-guarded at `:2510`, and BOTH conditions hand-traced firing exactly once on DIFFERENT edges. ✅ Test 9 still asserts all five families over every `GetActions()` row; `RequiredIds[]` UNEDITED (verified twice — content + a `+8`/13-line span reconciliation). ✅ `AddRow(` = 27 and `.RelatedActionIds =` = 16 re-measured by QA, both unmoved ⇒ zero production edits. ✅ All three self-reported defects verified FIXED not described (D-1 tautology fixed by-value AND the species swept clean across all 66 TestEqual sites; D-2 em-dash-in-`TEXT()` = 0 with a 32-hit positive control; D-3 symbol real). ⚠️ 4 WARN / 2 NIT — a FOURTH defect survives (W-1: `:2404` still says "Test 9 walks the whole registry", now FALSE, correctly outside his fence but UNREPORTED) · W-2 the handoff's counterfactual arithmetic for `Dangler.ActionId` is wrong (Unresolved would be 1, SelfEdges 0) though the code is right · W-3 F-1 routed · W-4 ⛔ `TASK-857` and `TASK-873` BOTH name `Tests/SiegeControlsHelpTest.cpp`. Report: `.claude/pipeline/qa/TASK-856.md`.

TASK-856 → status: ✅ **qa-passed — DONE (2026-09-03). Verdict PASS, 0 blockers, 4 WARN, 2 NIT, 1 ruling.** ⛔ `SC-§29` ledger names `TASK-852` ALONE. Census `428 declared / 31 files` (⛔ NOT a pass count; last EXECUTED = `427 Result={Success}` / `0 Result={Fail}` at `1aa0fee`). `TL-§5d` host named: `TASK-857`, expected build `TASK-835`. Report: `.claude/pipeline/qa/TASK-856.md`.

TASK-857 → blocked-by: ~~TASK-856 (PASS)~~ ✅ CLEARED 2026-09-03. ⛔ READ `qa/TASK-856.md` W-4 FIRST: `Tests/SiegeControlsHelpTest.cpp` carries TEST 18 (`TASK-852`) **AND** TEST 17 (`TASK-870`, gated by `TASK-872`, uncommitted) — the commit takes BOTH; NAME BOTH ROWS. ⛔ `Tests/SiegeAssistantSelectionTest.cpp` (`TASK-874`) is UNGATED and must NOT enter this commit.
```

**Manager:** the **F-1 rider** (`SiegeControlsHelpWidget.cpp:492`, `13` → measured `16`) needs a home per the ruling above — ⛔ **not** `TASK-870`/`TASK-873` (already gated), but the next row that opens that file for edit, and the repair must carry the **derivation**, never a re-typed number. **W-1**'s one-word comment repair (`:2404`) should ride the same host as `TASK-857`.
