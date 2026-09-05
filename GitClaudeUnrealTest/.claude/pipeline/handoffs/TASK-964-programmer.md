# TASK-964 — GAMEPLAY-PROGRAMMER HANDOFF — the owed compiled RED, built as a PERMANENT negative control

**Gate:** `TASK-965` · **Host:** `TASK-961` (amended, not a new host) · 2026-09-03
**Baseline:** `09b9b50` · **Files touched: 1** — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp`

---

## 0. ⛔ WHAT I EXECUTED AND WHAT I DID NOT — ABOVE THE CLAIM (`TL-§5c` cl. 5(a), `SC-§54` cl. 4)

**I EXECUTED:** the four-instrument absence census (both polarities) · the `-Wswitch` symbol census · the `default:`-label census (positive-controlled) · the path-composer copy census at both endpoints · the declaration-delta measurement at both endpoints · the `git status` scope sweeps over `Source/` and `Content/` · the `BP_Unit_Witch.uasset` digest re-read · a **two-polarity mirror of the predicate**, whose inverted run went RED.

⛔ **I DID NOT COMPILE, AND I DID NOT OBSERVE A COMPILED RED BAR.** My dispatch fences compiling, the editor, MCP and Git, and the board row item (5) says so in as many words. ⇒ **the item (3) inverted-assertion experiment was NOT run against the compiled binary.** I say that before I report anything else. What I did instead, and why I believe it is a better discharge than the one-off transcript, is §3 — **but the substitution is mine to declare, not to grade.**

⛔ **I TOUCHED NO ASSET.** Not `BP_Unit_Witch.uasset`, not `DT_Cards.uasset`, not one byte of `Content/**`. Measured, not asserted — §1.

⛔ **I opened no editor and used no MCP** (the editor is wedged on a modal awaiting Jonathan). ⛔ **No Git operation beyond read-only `show` / `ls-tree` / `status`.**

---

## 1. ⛔⛔ THE METHOD — THE HALF THAT WAS THE SPEC (`SC-§54` cl. 3)

**`Content/**` IS ABSENT FROM MY DIFF.** The row that gates me (`TASK-965` item (1)) makes this blocker-grade, so here is the measurement rather than my word:

| sweep | result |
|---|---|
| `git status --porcelain -uall -- Source/` | **1 path** — `…/Tests/SiegeCardRosterTest.cpp` (`M`) |
| `git status --porcelain -uall -- Content/` | 27 paths, **all `Content/FogArea/**`** (`TASK-927`, pre-existing, not mine) |
| `Content/` paths **outside** the FogArea lane | ⭐ **0** |
| `git status -- Content/Blueprints/Units/BP_Unit_Witch.uasset` | ⭐ **empty — unchanged vs `HEAD`** |
| `sha256sum BP_Unit_Witch.uasset` | `b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab` |

⭐ **That digest is character-for-character the one `TASK-949` recorded at four points before the refusal.** The refused move did not happen, under this task ID or any other.

⚠️ **Declared drift, not mine:** `TASK-949` recorded **24** untracked `Content/FogArea/**`; I measure **27**. Different instant, different lane (`TL-§5b`). I report my own number rather than reconciling to a relayed one.

### The synthetic input, and why I did NOT take the board's suggested name

⛔ **The board suggested `ZzNoSuchCard`. I rejected it and the reason is a measurement:** `ZzNoSuchCard` is a **SUBSTRING** of the fixture's existing `ZzNoSuchCardZz`, so a grep for its absence returns the *existing constant's* hits — its "measured absence" would have been **unmeasurable by the very instrument used to establish it.** That is the dispatch's own "a banned word can be a substring of a shipped identifier" trap, sitting in the suggestion.

⭐ **I reused the fixture's single `NegativeControlCardID` (`ZzNoSuchCardZz`) instead** — one constant, no second copy free to drift, `SC-§38`-shaped. Re-measured by me at my instant, **every instrument positive-controlled** (`SC-§39`):

| instrument | `ZzNoSuchCardZz` | `Sorcerer` (positive control) |
|---|---|---|
| `Docs/Data/cards.csv` | **0** | 1 |
| `Content/Data/DT_Cards.uasset` (binary) | **0** | 4 |
| `Content/Blueprints/**` filenames | **0** | 1 |
| files under `Source/` | **1** ⚠️ | 28 |

⚠️ **The `Source/` reading is 1, not 0, and that is CORRECT and expected:** the one file is `SiegeCardRosterTest.cpp` itself, which *declares* the constant. The file header's `0` was measured at `950d8c5`, **before this test file was tracked** — it is dated and hash-pinned, it is true as written, and per `SC-§53` cl. 3 I did **NOT** "correct" it. I recorded the distinction in the new test's doc comment instead.

---

## 2. THE DELIVERABLE — `AnAbsentCardIDFailsToResolveItsComposedActorClassPath`

`IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeCardRosterAbsentCardIDNegativeControlTest, "Siegebound.CardRoster.AnAbsentCardIDFailsToResolveItsComposedActorClassPath", …)` — a **second** test in the same file, ~150 lines including its doc comment.

### ⛔ It reaches the SHIPPED symbols, not a copy (`TASK-965` item (3))

It calls, **in the same order the walk calls them**, these five fixture symbols — every one of them the *same* symbol the green test calls:

`FindSoftObjectField` → `FindNameArrayField` → **`ClassifyRow`** → **`ComposeActorClassPath`** → **`FPackageName::DoesPackageExist`**

⛔ **It hand-builds NO path string and contains NO `/Game/...` literal.** Measured: `/Game/Blueprints` occurrences are **9 at `09b9b50` and 9 in my worktree** ⇒ ⭐ **I added ZERO path-composer copies.** (`TASK-959` item (1)'s *"a sixth copy is a FINDING"* clause: my additions do not trip it — the composer still lives in exactly one function and my test *consumes* it.)

### ⭐ It INVERTS rather than re-runs the walk (`TASK-965` item (4))

The load-bearing line is `TestFalse(…, bAbsentResolves)` where
`bAbsentResolves = FPackageName::DoesPackageExist(SyntheticComposed.PackageName)` — **the identical expression the sibling walk asserts TRUE for every spawnable row.**

⛔ **No `AddError` fires on the expected absence.** All **7** `AddError` sites in the new test are SELF-CHECK failures (CDO unreachable, `CardTableAsset` renamed/retyped, table failed to load, `BuildingEconomyCardIDs` unreachable, row probe blind, synthetic not routed spawnable, positive control no longer spawnable). Swept and listed — QA can re-run `awk 'NR>580 && /AddError/'`.

⇒ **the suite stays GREEN, and it is green *because* the probe answered ABSENT.**

### ⭐ The two ledgers, kept apart (`SC-§51` cl. 4)

**(a) REACHABILITY — the vacuity tell that is easy to omit and expensive to omit.** The synthetic is asserted to `ClassifyRow` → `UnitActor`. ⛔ **A CardID the walk would route `NotSpawnable` is never probed at all, so a "negative control" built on one stands for a red that could never happen.** This is the premise, not decoration, and it is a hard `return false` if it fails.

**(b) INSTRUMENT CONTROLS (`SC-§39`) — and there are TWO probes here, so there are two positive controls.**

| probe | negative half | ⭐ positive control | what the control catches |
|---|---|---|---|
| `FindRow<FCardRow>` | synthetic is **not** a row | `Sorcerer` **is** found | a lookup answering null for *everything* |
| `DoesPackageExist` | synthetic path **absent** | `Sorcerer` path **present** | a probe gone blind in *either* direction |

plus a **DISCRIMINATION** assertion requiring the two `DoesPackageExist` answers to **differ** in the same run.

**(c) LIVENESS** — the synthetic's absence is re-measured against the CDO-derived table **at run time**, so if `ZzNoSuchCardZz` ever becomes a real card the control dies **by failing** rather than passing quietly.

---

## 3. ⭐⭐ `SHIP-§9` — PROVING THE CONTROL CAN FAIL, AND WHAT I OWE

### 3.1 The structural half — permanent, compiled, and I believe it is the stronger half

⛔ **The positive control IS the inverted assertion.** `TestTrue(…, bPresentResolves)` asserts the *opposite polarity* of `TestFalse(…, bAbsentResolves)`, through the **same symbol**, in the **same run**. ⇒ **a probe stuck at ABSENT (the only mode in which my `TestFalse` could pass vacuously) turns this test RED at that line.** The discrimination assertion closes the other direction.

⭐ **This is `SC-§54` cl. 3(c) taken literally: a disturbed asset buys ONE transcript that decays into a screenshot; a synthetic absent input buys an assertion that re-proves the probe can say NO on every run, forever.** And the test `AddInfo`s both booleans, so **`TASK-961`'s compiled suite log will carry the evidence verbatim**:

```
NEGATIVE CONTROL — DoesPackageExist('/Game/Blueprints/Units/BP_Unit_ZzNoSuchCardZz') = ABSENT   [synthetic, measured absent]
NEGATIVE CONTROL — DoesPackageExist('/Game/Blueprints/Units/BP_Unit_Sorcerer') = PRESENT   [positive control]
```

### 3.2 The executed half — a MIRROR of the predicate, in both polarities

⛔ **DECLARED AS A SUBSTITUTE, NOT AS THE COMPILED RUN** (`SC-§54` cl. 4). I could not compile, so I transcribed the two symbols under test and executed both polarities.

⭐ **And unlike `TASK-947`'s mirror, this one's mapping is VALIDATED AGAINST THE COMPILED INSTRUMENT:** the compiled gate at `09b9b50` published `22 SPAWNABLE probed` with **0 unresolved**; my filesystem mapping (`/Game/X` → `Content/X.uasset`) finds **exactly 22** composed-path actor Blueprints on disk (14 `BP_Unit_*` + 8 `BP_Building_*`). **22 = 22, 0 unresolved on both sides** — the mirror reproduces the compiled probe's answer on all 22 points.

**VERBATIM TRANSCRIPT** (`Tools`-free; script kept in scratchpad, not committed):

```
INPUTS
  absent  CardID = ZzNoSuchCardZz   composed = /Game/Blueprints/Units/BP_Unit_ZzNoSuchCardZz    DoesPackageExist = 0
  present CardID = Sorcerer  composed = /Game/Blueprints/Units/BP_Unit_Sorcerer   DoesPackageExist = 1

RUN 1 — THE SHIPPED POLARITY (what the new test actually asserts)
  PASS  NEGATIVE CONTROL - the absent card's composed path does NOT resolve
  PASS  POSITIVE CONTROL - the present card's composed path DOES resolve
  PASS  DISCRIMINATION - the one probe symbol answered DIFFERENTLY for the two
  => RUN 1 VERDICT: GREEN

RUN 2 — THE INVERTED POLARITY (assert the ABSENT card DOES resolve)
         ⛔ this is the deliberate inversion; it MUST go RED or the
            control is a tautology that passes on any input.
  FAIL  INVERTED - the absent card's composed path DOES resolve   (expected 1, got 0)
  => RUN 2 VERDICT: RED

RUN 3 — SELF-CONTROL ON THE HARNESS (SC-39: prove 'report' can print FAIL)
  FAIL  SYNTHETIC - a check wired to fail on purpose

SUMMARY  run1=GREEN  run2=RED
```

⭐ **Note RUN 3:** the harness itself is positive-controlled, so RUN 2's `FAIL` is a measured failure and not a printer that cannot print `PASS`.

### 3.3 ⛔ WHAT REMAINS OWED, NAMED PLAINLY

⛔ **Nobody has yet seen this file's assertions RED on a compiled run, and I have not changed that.** What I changed is that **the red-producing condition is now observed and printed on every compiled green pass**, rather than being reachable only by breaking something.

⚠️ **NO RESIDUAL INVERSION IS IN THE TREE.** The inversion lived only in the shell mirror, never in `Source/`. `git status -- Source/` shows exactly one modified file, and the three assertion polarities in it are `TestFalse(bAbsentResolves)` / `TestTrue(bPresentResolves)` / `TestTrue(bAbsentResolves != bPresentResolves)` — swept by name at lines 787–797.

---

## 4. ⭐ THE NAMED RIDER — the `-Wswitch` claim (`qa/TASK-948.md` `W-2`)

**SITE COUNT BY SYMBOL CENSUS (`-Wswitch`), NOT BY THE BOARD'S COUNT: ⭐ 2 — which AGREES with the board.** Lines **94** and **254** post-edit (were 92 and 215). Repo-wide the symbol also appears in 5 `.claude/pipeline/**` documents; those are records of past measurements, out of my fence, untouched.

Both sites amended to `SC-§53` cl. 3 shape — **PAST TENSE + DATE + HASH + TOOLCHAIN NAMED**:

> *"under Clang `-Wswitch` makes a 7th enumerator a COMPILE error, but **AS MEASURED 2026-09-03 AT `09b9b50`** this project's `Build.cs` sets **NO** warning configuration … and `TASK-949` built it on **MSVC 14.50** ⇒ **ON THIS TOOLCHAIN THE COMPILE-TIME HALF MAY NOT FIRE AT ALL.** ⭐ **THE RUN-TIME TELL IS THE LOAD-BEARING HALF HERE**, and it is UNAFFECTED …"*

⛔⛔ **NO `default:` LABEL WAS ADDED. MEASURED, POSITIVE-CONTROLLED:**

| file | `^\s*default\s*:` labels |
|---|---|
| `SiegeCardRosterTest.cpp` | ⭐ **0** |
| `SiegePlayerController.cpp` (control — proves the regex can SEE one) | **7** |

⚠️ **AND AN INSTRUMENT TRAP WORTH RECORDING:** a bare `grep -c "default:"` on the file reads **1**. That hit is the *comment at line 213 documenting the absence* — the trailing-token false-positive direction of `SC-§39`'s amendment. **The label count is 0.** I also *strengthened* the preservation comment at that site to cite `qa/TASK-948.md` §(c)'s ruling by name, so the next tidy-up meets the ruling instead of the temptation.

---

## 5. ⚠️ ONE AMENDMENT BEYOND THE NAMED RIDER — DECLARED, NOT SLIPPED IN

⛔ **I also rewrote the file header's `THIS TEST IS RED TODAY` paragraph, and QA should rule on whether that was mine to do.**

**Why:** it was a **present-tense claim that is now measurably FALSE.** It read *"`BP_Unit_Witch` is absent as this file is written… The row for `Witch` MUST fail."* `BP_Unit_Witch.uasset` is on disk and committed at `09b9b50`, and the gate went GREEN there. That is `SC-§53` cl. 1 exactly — the repair landed in one commit and the paragraph describing the pre-repair state did not — **in the very file my row edits, with `SC-§53` among my cited laws.**

**What I did:** past tense + date + hash (`SC-§53` cl. 3), recorded the actual `09b9b50` green with its `32/22/10` figures, kept the *"do not weaken the assertion"* prohibition, and added the `SC-§54` record of the refused move plus a pointer to the new sibling test.

⚠️ **If QA rules this out of scope, it is a comment-only revert and blocks nothing.** I flag it rather than let it be discovered (`SC-§29`).

---

## 6. THE DELTA — `+1`, BOTH ENDPOINTS MEASURED BY ME, ⛔ UNEXECUTED (`TL-§5b`/`§5c` cl. 5(a))

Scope: `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, needle `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`.

| | declarations | files |
|---|---|---|
| `09b9b50` (last EXECUTED suite), via `git show` per file — **32 joined, 0 join failures** | 433 | 32 |
| worktree now | **434** | 32 |
| ⭐ **DELTA** | **+1** | **0** |

⛔⛔ **`434` IS A DECLARATION CENSUS, NOT A PASS COUNT. I DID NOT RUN THE SUITE.** The last **EXECUTED** figures remain `TASK-949`'s **`433 Result={Success}` / `0 Result={Fail}` at `09b9b50`**.

⚠️ **INSTRUMENT FAILURE I CAUGHT WITH A POSITIVE CONTROL, RECORDED BECAUSE THE DISPATCH PREDICTED IT:** my first delta run printed **`declarations: 0`** for all 32 files. Cause: `git ls-tree` emits **cwd-relative** paths while `git show` requires **repo-root-relative** ones (this project's git root is one level above the `.uproject`). ⇒ **32 silently failed `git show` calls summing to a confident `0`.** Caught only because I ran the pipeline against a file I knew contained a declaration *before* trusting the total. Fixed with `--full-name`, and the corrected run reports its **cardinality (32 joined / 0 failed) beside its result**, per `SC-§39.1`.

---

## 7. FOR QA (`TASK-965`) AND FOR `TASK-959`

**Scrutinise these, in this order:**

1. ⛔ **The substitution in §3.** I did not observe a compiled RED and I claim a structural discharge instead. **That is the one judgement call in this row.** Item (3) as literally written was unexecutable under my fence; I did not quietly redefine it.
2. ⛔ **§5's beyond-rider amendment** — rule it in or out.
3. **The `TestNotNull` / `TestTrue` / `TestFalse` / `FindRow` signatures** — I verified all four against `UE_5.8/Engine/…/AutomationTest.h` and `DataTable.h` (`bool TestTrue(const FString&, bool)`, `template<typename ValueType> bool TestNotNull(const FString&, const ValueType*)`, `FindRow(FName, const TCHAR*, bool)`), plus 86 in-house `if (!TestTrue(` usages. **No new `#include` was needed** — the include block is byte-identical to `09b9b50`.
4. **Vacuity:** the test can only pass with a live table, a found positive row, a spawnable-routed synthetic, two *different* composed paths and two *different* probe answers. I do not claim more tells than that; the reachability assertion and the two positive controls are the ones that do work.

**⭐ FOR `TASK-959` (the path-composer extraction), the thing you need in one line:** I added **one call site each** to `ClassifyRow` ×3, `ComposeActorClassPath` ×2, `FindSoftObjectField` ×1, `FindNameArrayField` ×1 — **and ZERO new copies of the composed-path literal** (`/Game/Blueprints` is 9 before and 9 after). Re-run your own census at your instant; the file is now **817 lines** with **2** test declarations.
