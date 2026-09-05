# QA Report — TASK-965 (gate over TASK-964)

**Verdict: PASS — 0 BLOCKERS** · 4 WARN · 5 NIT
**Subject:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp`
**Date:** 2026-09-04 · **Reviewer instrument:** working tree + Read/Grep/Glob only

---

## ⛔⛔ THIS IS A **POST-HOC** GATE OVER **ALREADY-COMMITTED** CODE — `1a457df`

⛔ **Read this before you read the verdict.** `TASK-964` reached `ready-for-qa` on 2026-09-03, this
gate was **never dispatched**, and `TASK-987`'s (correct) `git status`-derived pathspec swept the
then-untracked file into **`1a457df`**. ⇒ **the code was in the repository before it was reviewed.**
This verdict is written **after** the commit and **names it deliberately**, so no future reader
mistakes it for a normal pre-commit gate.

⚖️ **NONE OF THIS IS THE AUTHOR'S FAULT AND THE GRADE DOES NOT REFLECT IT.** `TASK-964` declared both
of its judgement calls *explicitly and in bold, above its own claim*, flipped its row correctly, and
waited. The gate was not sent. **I graded the work, not the process failure.**

⇒ **A FAIL here would have produced a FOLLOW-UP COMMIT — never an amend, never a revert.** It is a
PASS, so no repair commit is owed; the WARNs below are boardable comment-only improvements.

---

## 0. ⛔ MY INSTRUMENT, DECLARED ABOVE THE FINDINGS (`TL-§5c` cl. 5(a), `SC-§54` cl. 4)

**I have `Edit` but no `Bash` (`SC-§78`) ⇒ ⛔ ZERO GIT.** No `show`, no `diff`, no `ls-tree`, no
`log`. **I cannot read `1a457df` directly.** The **working tree is my only instrument**, and I say so
rather than letting a reader assume I read the commit.

⛔ **`SC-§55` APPLIED TO MYSELF FIRST.** My session's `gitStatus` snapshot is **STALE**: its `HEAD`
was **`84eec02`** and it lists the *entire fog batch* (`DT_Cards.uasset`, `cards.csv`,
`T_CardArt_Fog/BrightSun`, `SiegeFogStatics.*`, **and `Tests/SiegeCardRosterTest.cpp` itself**) as
**uncommitted** — that batch **is** `1a457df`. ⇒ **I used not one byte of it as current state.** I
used it only to attribute *which lane* each path belongs to, which is time-invariant.

| claim class | status |
|---|---|
| ⛔ **the working tree == `1a457df`** | ⛔ **ACCEPTED AS DECLARED** (build-master's `git diff HEAD` empty for the atomic three). ⛔ **Declared to me, not measured by me.** |
| ⛔ `1a457df`'s hash, file count, suite figures | ⛔ **ACCEPTED AS DECLARED** — `handoffs/TASK-987-buildmaster.md` §5 |
| ⭐ everything under **§2 MEASURED BY ME** | ⭐ **measured in the working tree at my instant** |

### ⭐ THE ONE INDEPENDENT CORROBORATION OF WORKTREE-vs-COMMIT I *CAN* OFFER

⭐ Anchored census, **mine, at my instant**: `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` across `Source/**` =
**475** across 38 files, and `IMPLEMENT_(COMPLEX|CUSTOM)_AUTOMATION_TEST` = **0** (so one declaration
== one test case; no macro fans out).
The suite **EXECUTED** at `1a457df` reported **`Result={Success}` = 475 · `Result={Fail}` = 0**.

⇒ ⭐ **475 declared == 475 succeeded == 0 failed.** Every declared test **ran** and **passed**, and
the tree has neither gained nor lost a declaration since that run. ⛔ **That is corroboration at
test-declaration granularity, NOT byte equality** — I state the limit rather than overselling it.

---

## 1. ⭐⭐ WHERE I RELY ON THE EXECUTED SUITE — NAMED, AS THE DISPATCH REQUIRED

The pre-compile gate this row was meant to be could not have had this. I use it in **exactly four
places** and nowhere else:

1. **`SiegeCardRosterTest.cpp` contributes exactly 2 of the 475** (my own count of that file). ⇒
   **BOTH tests compiled, ran against the real `DT_Cards.uasset`, and PASSED on the real binary.**
   The entire *compile-risk* class (deprecated UE 5.8 API, signature drift, missing include,
   const-correctness on `ContainerPtrToValuePtr`, `EAutomationTestFlags` enum-class form) is
   **retired by execution, not by my reading.**
2. **`bAbsentResolves == false` and `bPresentResolves == true` were measured BY THE ENGINE.** The new
   test cannot pass otherwise (lines 790, 793). ⇒ the red-producing **value** is compiler-observed.
3. **No asset was left disturbed.** The walk probes all 22 spawnable rows *including `Witch`*; had a
   red been synthesised by moving `BP_Unit_Witch.uasset` and imperfectly restored, the walk would be
   **RED**. It is green. ⇒ **item (1)'s blocker question is answered by measurement, not by trust.**
4. **No residual inversion shipped.** A left-in `TestTrue(bAbsentResolves)` would have produced
   **474/1**, not 475/0.

⛔⛔ **AND THE LIMIT, STATED PLAINLY: A GREEN SUITE PROVES THE TESTS *PASS*, NOT THAT THEY ASSERT THE
RIGHT THING.** Every judgement in §3–§5 below is mine, from reading the source. **W-1, W-3 and W-4
are all findings the green bar actively conceals.**

---

## 2. ⭐ MEASURED BY ME, IN THE WORKING TREE, AT MY INSTANT

| # | measurement | result | positive control |
|---|---|---|---|
| M-1 | `^\s*default\s*:` in `SiegeCardRosterTest.cpp` | ⭐ **0** | **7** in `SiegePlayerController.cpp` (the regex *can* see one) |
| M-2 | `-Wswitch` **symbol** census under `Source/` | ⭐ **2** — `SiegeCardRosterTest.cpp:94` + `:254`, **both in this file, both amended** | — |
| M-3 | `/Game/Blueprints` occurrences in the subject file | ⭐ **9** (author declared 9 at `09b9b50` and 9 after) ⇒ **0 new composer copies** | 46 repo-wide across 19 files |
| M-4 | `DeckCount` in the subject file | ⭐ **0** | 48 hits across 9 production files |
| M-5 | `NoticeRange` in the subject file | ⭐ **0** | 24 hits in `SiegeFogClampTest.cpp` (3) + `SiegeUnitNoticeRangeTest.cpp` (21) |
| M-6 | actor Blueprints on disk | ⭐ **22** = 14 `BP_Unit_*` + 8 `BP_Building_*`; **`BP_Unit_Sorcerer` PRESENT**, **`BP_Unit_Witch` PRESENT**, **no `ZzNoSuchCardZz` asset** | — |
| M-7 | `Docs/Data/cards.csv` roster | ⭐ **34 rows** — **22 spawnable** (13 Unit + 2 Economy + 7 Building), **12 excluded** (7 Spell + 4 HeroUpgrade + 1 Utility) | exact **22 ↔ 22** bijection with M-6 |
| M-8 | `Fog` / `BrightSun` `CardType` | ⭐ **both `Spell`** (`cards.csv:34`, `:35`) ⇒ **NotSpawnable** | — |
| M-9 | `ZzNoSuchCard*` outside the subject file | **2 hits, `SiegeCardArtRosterTest.cpp` comment only** — that file uses its **own** distinct synthetic `QqSyntheticAbsentQq` and explicitly checks non-substring **in both directions** | ⇒ ⭐ **no second copy of the constant, no drift surface** |

---

## 3. ⚖️⭐⭐ RULING ON FLAGGED ITEM (a) — THE UNOBSERVED COMPILED RED AND ITS STRUCTURAL SUBSTITUTE

### ⭐ RULING: THE SUBSTITUTE IS **ADEQUATE**. Board item (2)'s BLOCKER clause is **SATISFIED IN SUBSTANCE**. ⛔ NOT A BLOCKER.

**What was owed** (`TASK-964` item (3)): invert your own new assertion, observe a compiled RED, paste
the verbatim transcript, revert.
**What was delivered** (handoff §3.2): a **verbatim, two-polarity, harness-self-controlled**
transcript from a **shell mirror of the predicate** — RUN 1 GREEN, RUN 2 **RED**, RUN 3 proving the
harness can print `FAIL` — **declared, in bold, as a substitute and never dressed up as the compiled
run.**

**Five reasons I rule it in, in the order they carry weight:**

1. ⛔⛔ **THE SPEC CONTRADICTED ITSELF AND THE AUTHOR RESOLVED IT THE RIGHT WAY.** `TASK-964` item (3)
   demands an observed compiled red while item (5) says *"You are ⛔ NOT compiling"* and item (6)
   fences *"any compile"* **out of scope**. ⛔ **An author cannot be failed for an internally
   inconsistent dispatch**, and of the two available readings it chose the one that **preserved the
   fence** rather than the one that breached it — which is this project's own `SC-§54` cl. 1.
2. ⭐ **THE PURPOSE OF `SHIP-§9` IS DISCHARGED, AND PERMANENTLY.** The one-off inverted-source
   transcript would have proven **once** that the probe can answer `false`. The shipped test proves
   it **on every run, forever** (lines 787–797). ⭐ `SC-§54` cl. 3(c) taken literally: *a disturbed
   asset buys one transcript that decays into a screenshot; a synthetic absent input buys an
   assertion that re-proves the probe can say NO.* **This is the stronger artefact and I say so.**
3. ⭐⭐ **THE OBSERVATION THAT WAS MISSING WHEN THE HANDOFF WAS WRITTEN HAS SINCE BEEN MADE.** The
   compile **has** run and the suite **is** green (475/475, §1). ⇒ `bAbsentResolves == false` and
   `bPresentResolves == true` are now **engine-measured on the real binary**, not shell-mirrored.
   **The mirror's central claim was subsequently confirmed by the instrument it was standing in for.**
4. ⭐ **THE MIRROR'S MAPPING WAS VALIDATED AGAINST THE COMPILED INSTRUMENT** — 22 composed paths on
   disk vs the compiled gate's `22 SPAWNABLE probed / 0 unresolved`, **22 = 22 on both sides**. I
   **independently reproduced that 22** (M-6/M-7, exact bijection). Unlike `TASK-947`'s mirror, this
   one is anchored to the compiled answer at every point.
5. ⭐ **IT WAS DECLARED ABOVE THE CLAIM, TWICE** (§0 and §3.2), and named as *the one judgement call
   in this row*. **A dropped-and-declared item passes; what fails is silence** — and this is the
   opposite of silence.

### ⛔ THE RESIDUAL, STATED PRECISELY — AND IT IS **REAL**, WHICH IS WHY IT IS **W-3**

⛔ **What is still unobserved on any compiled run is not "a red value" — it is the walk's FAILURE
PATH.** These lines have **never executed** and the green bar is exactly why:

- **`:477–484`** — the `!bPackageExists` branch: `++UnresolvedRows`, `UnresolvedCardIDs.Add(...)`,
  and a 3-substitution `AddError` `Printf`.
- **`:580–584`** — the unresolved report, including `FString::Join`.

⛔ **The new test does not close this, because it never enters the walk.** A `%s`/`%d` mismatch in a
never-executed `FString::Printf` is a live UE crash class, and it would surface **only** on the day a
card is missing — i.e. **the exact day the gate is needed.** ⛔ **This is not a defect in the diff and
it is not repairable by its author** (who cannot compile) ⇒ **it is a build-master follow-up row, not
a blocker.** See §7.

---

## 4. ⚖️⭐ RULING ON FLAGGED ITEM (b) — THE ONE AMENDMENT BEYOND THE NAMED RIDER

### ⭐ RULING: **IN SCOPE. IT SHOULD HAVE BEEN FOLDED IN. ⛔ NO REVERT.** It was **correct** to declare it, and **correct** not to leave it.

The header's `THIS TEST IS RED TODAY` paragraph read *"`BP_Unit_Witch` is absent as this file is
written… The row for `Witch` MUST fail."* — **a present-tense claim that `09b9b50` made false.**

**Why it is in scope:**
- The row's `names:` line puts **this exact file** in scope as the **only** file it may touch; the
  paragraph is **inside it**.
- Item (6)'s out-of-scope list does **not** name it.
- The row **cites `SC-§53` cl. 3 among its own laws.** ⛔ **Leaving a source file asserting its own
  gate is expected to be RED — when it is green — is a lie in the tree**, and the very next reader
  would have concluded the walk was *supposed* to fail. ⛔ **Leaving it would have been worse than
  fixing it.**
- **It is comment-only.** ⭐ I verified the mechanism was untouched: the *"do not weaken the
  assertion"* prohibition survives verbatim at `:146–148`, and **M-1 confirms `default:` is still 0.**

**Quality of the amendment:** `:138–148` is **past tense** ✅, **dated `2026-09-03`** ✅, **hash-pinned
`09b9b50`** ✅, and it records the actual `32 / 22 / 10` figures. ⭐ Correct `SC-§53` cl. 3 shape.

⚠️ **BUT THE SAME PARAGRAPH REINTRODUCES THE DEFECT IT WAS WRITTEN TO REMOVE — see W-2.** That is why
this ruling is *"in scope and correct"* rather than *"in scope and clean"*.

⛔ **Should it have been declared as a deviation rather than folded in?** It **was** declared — handoff
§5, under its own heading, with the revert offered. ⭐ **That is precisely the `SC-§29` shape and it is
what let this gate rule on it in one read.** ⛔ **No further declaration was owed.**

---

## 5. ⛔ THE TRANSCRIBED-COUNT QUESTION — CONFIRMED BY **REASON**, NOT INFERRED FROM THE PASS

⭐ **CONFIRMED: this file contains ZERO hard-coded roster counts in ANY assertion.** All five COUNT
assertions are **relational**, and not one contains a numeric literal:

| line | assertion | literal? |
|---|---|---|
| `:534` | `TotalRows > 0` | none |
| `:537` | `SpawnableRows > 0` | none |
| `:540` | `SpawnableRows + ExcludedRows == TotalRows` | none |
| `:548` | `ProbesExecuted == SpawnableRows` | none |
| `:551` | `DistinctComposedPackages.Num() == SpawnableRows` | none |

⛔ **THE REASON, WHICH IS THE THING THE PASS CANNOT TELL YOU:** the file states the design at
`:527–532` — *"a `TestEqual(SpawnableRows, 22)` would be the `TASK-874` trap rebuilt inside the very
gate written to close it: card #23 would turn this file red for the wrong reason, and the obvious
'fix' would be to bump the number."* ⇒ **the 33rd and 34th cards could not have reddened this file BY
CONSTRUCTION.** The green is a **consequence** of that design, not the evidence for it.

**The only two numeric roster literals in the file, and their status:**

- **`:145`** — `32 row(s) read; 22 SPAWNABLE probed; 10 EXCLUDED`, **hash-pinned to `09b9b50`,
  past tense.** ⭐ **Correct as written.** ⛔ **DO NOT "CORRECT" IT** (`SC-§53` cl. 3) — see N-5.
- **`:812`** — an **`AddInfo`**, **present tense**, **undated**, **unhashed**: *"all 22 spawnable
  rows"*. ⛔ **This is W-1**, and it is the single best example in this file of a claim the suite
  **states** without **asserting**.

### `sum(DeckCount) == 50` and `NoticeRange` — NO DUPLICATION, NO CONTRADICTION

⭐ **M-4/M-5: `DeckCount` = 0 occurrences and `NoticeRange` = 0 occurrences in this file.** ⇒ it
**cannot** duplicate or contradict either. **Clean separation of concerns; nothing to fix.**

---

## 6. Findings

### BLOCKERS — ⭐ **NONE (0)**

Every blocker-grade question in this row's spec is cleared, each by my own measurement:

| spec item | question | ruling |
|---|---|---|
| **(1)** | synthetic input, or a **disturbed real asset**? | ⭐ **SYNTHETIC. CLEAR.** `BP_Unit_Witch.uasset` present (M-6); the walk probes it and the suite is green (§1.3); the synthetic is a `TEXT()` constant, never an asset. ⛔ **The permission fence was respected and this gate rewards that.** |
| **(2)** | verbatim RED transcript · no residual inversion | ⭐ **CLEAR.** Verbatim two-polarity transcript with a harness self-control (handoff §3.2), **declared as a mirror**. Polarities in the tree read `TestFalse(bAbsentResolves)` / `TestTrue(bPresentResolves)` / `TestTrue(!=)` at `:790`/`:793`/`:796` — **read by me**, and corroborated by 475/0. **§3 rules the substitute adequate; residual → W-3.** |
| **(3)** | shipped symbols, or a **copy**? | ⭐ **CLEAR — SAME SYMBOLS, NAMED.** `FindSoftObjectField` · `FindNameArrayField` · **`ClassifyRow`** · **`ComposeActorClassPath`** · **`FPackageName::DoesPackageExist`**, in the same order the walk calls them. **M-3: `/Game/Blueprints` = 9, unchanged ⇒ zero hand-built paths, zero new composer copies.** |
| **(4)** | inverts rather than re-runs; suite green in intent | ⭐ **CLEAR — AND GREEN IN FACT.** All 7 `AddError` sites in the new test are **self-check** failures (CDO, table rename/retype, load failure, exception-list unreachable, blind row probe, synthetic not spawnable, control no longer spawnable) — **verified by reading `:672–815`.** ⛔ **Not one fires on the expected absence.** |
| **(5)(c)** | ⛔ was a `default:` label added? | ⭐ **NO — 0, POSITIVE-CONTROLLED AT 7 (M-1).** The overriding blocker is **CLEAR**. The preservation comment at `:245–251` was **strengthened**, citing `qa/TASK-948.md` §(c) by name. |
| **(6)** | `TL-§5b`/`§5c` cl. 5(a) | ⭐ **CLEAR.** Delta declared **`+1` → 434/32 files** and declared **UNEXECUTED** in capitals: *"`434` IS A DECLARATION CENSUS, NOT A PASS COUNT. I DID NOT RUN THE SUITE."* ⛔ **No suite absolute is carried as an expectation** — the last executed figures are correctly attributed to `TASK-949` at `09b9b50`. |

⭐ **AND A CREDIT THAT BELONGS IN THE LEDGER (`TL-§5b`):** the author **caught its own instrument
failing** — a `git ls-tree` / `git show` path-relativity mismatch that printed a confident
`declarations: 0` for all 32 files — **only because it positive-controlled the pipeline against a
file it knew contained a declaration before trusting the total.** ⛔ **That is the exact
silently-confident-zero this project keeps getting burned by, caught by the author, unprompted.**

### WARN

- **[WARN] `SiegeCardRosterTest.cpp:812` — a transcribed roster count in a LIVE output string, present tense, undated, unhashed.**
  `AddInfo(... "The sibling walk asserts that same expression is TRUE for all 22 spawnable rows ...")`.
  It is an `AddInfo`, **not an assertion**, so it **cannot redden** — it will simply print a false
  number into the suite log forever. ⛔ **It is correct today only by coincidence:** the roster went
  **32 → 34** while spawnable stayed **22**, because `Fog` and `BrightSun` are **both `CardType
  Spell`** (M-8, verified in `cards.csv:34`/`:35`). ⛔ **The next Unit / Building / Economy card makes
  this line lie on every green run.** ⭐ This is the **same `SC-§53` cl. 1 shape the author's own §5
  amendment repaired 660 lines above, missed 660 lines below.**
  **Fix (comment-only):** drop the count, or date+hash it — *"…is TRUE for every spawnable row (22 as
  measured at `1a457df`, 2026-09-04)."*

- **[WARN] `SiegeCardRosterTest.cpp:146` — the amended paragraph REINTRODUCES a present-tense claim.**
  *"⇒ ⛔ THE GATE IS GREEN **TODAY** AND MUST STAY THAT WAY."* — undated *"TODAY"*, inside the very
  paragraph rewritten to eliminate present-tense claims, in an edit whose **entire stated rationale**
  was eliminating this shape (`SC-§53` cl. 3). ⛔ It happens to be **true** right now (475/0 at
  `1a457df`) and it **rots identically to the sentence it replaced.**
  **Fix (comment-only):** *"THE GATE WAS GREEN AT `1a457df` (2026-09-04, `475 Success / 0 Fail`) AND MUST STAY THAT WAY."*

- **[WARN] `SiegeCardRosterTest.cpp:150–166` — the DURABLE artefact does not carry the residual its handoff states plainly.**
  The header presents the method change as **the answer** to the never-seen-red debt. ⛔ **The handoff
  §3.3 says it plainly** — *"Nobody has yet seen this file's assertions RED on a compiled run"* — ⛔
  **but a future reader of this test opens the test, not a handoff from 2026-09-03.** Concretely
  still-unexecuted on any compiled run: **`:477–484`** (the `!bPackageExists` branch — `AddError` +
  `UnresolvedRows` + `UnresolvedCardIDs`) and **`:580–584`** (the unresolved report incl.
  `FString::Join`). ⛔ **A `Printf` substitution defect there surfaces only on the day a card is
  missing — the exact day the gate is needed.**
  **Fix:** a two-line `SC-§40` cl. 1 residual note in the header **plus** the `TASK-965-A` follow-up
  row in §7. ⛔ **Not repairable by this row's author, who cannot compile.**

- **[WARN] `SiegeCardRosterTest.cpp:796–797` — the DISCRIMINATION assertion is logically IMPLIED by the two above it and adds zero discriminating power.**
  `!bAbsentResolves && bPresentResolves` ⇒ `bAbsentResolves != bPresentResolves`, necessarily. It
  **cannot fail unless `:790` or `:793` has already failed.** Its comment claims *"a probe stuck at
  EITHER polarity fails HERE"* — ⛔ **it fails one line earlier.** Harmless as a mechanism; the
  **claim** overstates it. ⭐ The same file is **scrupulously honest about exactly this** 250 lines
  above (`:543–547`: *"as the loop is written TODAY these two counters cannot diverge, so this
  assertion discriminates NOTHING on this diff. It is a TRIPWIRE FOR THE NEXT EDIT"*) — **the same
  sentence is owed here.** ⛔ **Do not delete the assertion; it is a fine tripwire.** Label it as one.

### NIT

- **[NIT] `:551–552`** — `DistinctComposedPackages.Num() == SpawnableRows` is, like its declared
  neighbour at `:548`, **structurally always true today**: DataTable row names are unique `FName`s
  (case-insensitive keys) and `ComposeActorClassPath` is injective on `CardID`. A good tripwire;
  **unlike `:543–547` it is not declared as one.**
- **[NIT] `:491–497` vs `:582`** — a row whose **package exists** but whose `_C` **fails to load** is
  pushed into `UnresolvedCardIDs`, and the report then describes it as having *"NO actor Blueprint at
  their composed CONVENTIONS path"* — **which is not what happened.** Two distinct failure modes
  collapse into one message. (Never yet executed — see W-3.)
- **[NIT] `:85–87`** — *"This is the SAME `UDataTable` object `ResolveCardRow` loads"* is measured on
  the **C++ CDO's** default. A **Blueprint subclass** or instance override of `CardTableAsset` would
  make the run-time object differ, and nothing checks that. ⭐ **Low risk** — 8 C++ classes all
  hard-default to `/Game/Data/DT_Cards.DT_Cards` — but the sentence is one notch stronger than what
  is measured (`SC-§49`).
- **[NIT] `:135–136` vs `:660–661`** — the same instrument (`Sorcerer` in the `DT_Cards.uasset`
  binary) is recorded as **3** in the header and **4** in the sibling comment. ⛔ **Both are dated to
  different hashes (`950d8c5` / `09b9b50`) and both are CORRECT AS WRITTEN** under `SC-§53` cl. 3.
  Recorded **only** so a future tidy-up does not "reconcile" two correct measurements into one wrong
  one.
- **[NIT] `:145`** — the dated `32 / 22 / 10` record is now **superseded** (measured today: **34 rows
  / 22 spawnable / 12 excluded**, M-7). ⛔⛔ **IT MUST NOT BE EDITED** — it is hash-pinned to
  `09b9b50` and true as written. ⭐ **A NEW dated line is the only correct way to record the current
  figures.**

---

## 7. ⛔ SEPARATE, NON-BLOCKING LEDGER LINE — THE `-Wswitch` RIDER (`qa/TASK-948.md` `W-2`)

### **RIDER VERDICT: ⭐ COLLECTED AND DISCHARGED — 3 of 3. ⛔ It does NOT change TASK-964's verdict (`SC-§29`).**

| check | result |
|---|---|
| **(a) ALL sites found by SYMBOL census, not by the board's count** | ⭐ **PASS.** My own `-Wswitch` census under `Source/` returns **2** — `:94` and `:254`, **both in this file**. The author's independently-run census also returned **2** and it **agreed with the board** rather than adopting it. **Both amended.** |
| **(b) amended sentence is `SC-§53` cl. 3 shaped** | ⭐ **PASS.** Both sites read **PAST TENSE + DATE (`2026-09-03`) + HASH (`09b9b50`) + TOOLCHAIN NAMED (`MSVC 14.50`, `Build.cs` sets no warning configuration)**, and both then name **the run-time tell as the load-bearing half.** |
| **(c) ⛔ NO `default:` LABEL ADDED** | ⭐⭐ **PASS — 0 labels, positive-controlled at 7** (M-1). ⛔ **The blocker that would have overridden everything is CLEAR.** |

⭐ **AND IT CAUGHT AN INSTRUMENT TRAP WORTH KEEPING:** a bare `grep -c "default:"` on this file reads
**1** — that hit is **the comment at `:245` documenting the absence.** ⛔ **The label count is 0.** The
anchored form `^\s*default\s*:` is the correct instrument, and I re-ran it myself rather than
adopting the number.

⛔ **I did NOT re-litigate the `default:` ruling** — `qa/TASK-948.md` §(c) **RULED** it; I **enforced**
it (spec item (7)).

---

## 8. Notes for build-master / manager

⛔ **NO REPAIR COMMIT IS OWED.** `1a457df` stands: **not amended, not reverted.** The subject compiles,
runs, and passes; every WARN is comment-only.

**Two follow-up rows I recommend the manager board — ⛔ neither blocks anything:**

- **`TASK-965-A` (build-master) — ⭐⭐ BUY THE COMPILED RED THAT `TASK-964` COULD NOT.** ⛔ **The only
  agent who can: the author is fenced from compiling and this needs a suite run.** Temporarily invert
  **`SiegeCardRosterTest.cpp:790`** (`TestFalse` → `TestTrue` on `bAbsentResolves`), run the suite,
  **paste the verbatim RED**, revert, show `git status` clean. ⭐ **Expected `474 Success / 1 Fail`,
  and that exact pair is itself the assertion** (`SHIP-§9`: validate the gate against the failure it
  detects). ⛔ **Second, more valuable half: also exercise the WALK's failure path** by pointing the
  probe at one absent synthetic row, so **`:477–484` and `:580–584` execute at least once** and their
  `Printf` substitutions are proven before the day they are needed (W-3).
- **`TASK-965-B` (gameplay-programmer) — the four comment-only repairs:** **W-1** (`:812` transcribed
  `22`), **W-2** (`:146` undated *"TODAY"*), **W-3** (header residual note), **W-4** (`:796` label the
  implied assertion as a tripwire). ⛔ **Zero mechanism changes. ⛔ `default:` stays absent. ⛔ `:145`
  and `:135–136` must NOT be touched** (N-4, N-5 — dated records, correct as written). ⚠️ **Sequence
  after `TASK-959`**, which rewrites this same file's composer copy.

**One observation OUTSIDE this row's scope, surfaced because it is the class of gap this batch keeps
finding — ⛔ NOT counted against `TASK-964`:**

- ⚠️ **`sum(DeckCount) == 50` APPEARS TO BE ASSERTED NOWHERE IN CODE.** My dispatch told me it is
  *"asserted elsewhere"*; I could not locate it. **`DeckCount` = 0 occurrences under
  `Source/**/Tests/` and 0 in any `*.py`** — it appears **only** in production `.h`/`.cpp` (48 hits,
  9 files). ⭐ I **hand-summed `Docs/Data/cards.csv` and the invariant HOLDS: exactly 50** (Footman 9
  + Archer 8 + Knight 3 + Miner 3 + ArrowTower 3 + Wall 4 + MilitiaMob 3 + Pikeman 3 + Cavalry 3 +
  Longbowman 2 + Cleric 2 + Ogre 2 + Fireball 2 + FrostNova 1 + Sorcerer 2; `Fog` and `BrightSun`
  both carry `DeckCount 0`, so the fog batch was **deck-neutral**). ⛔ **But it holds UNMECHANISED —
  prose, not a gate**, which is `SC-§50` cl. 4's exact shape. ⭐ **`NoticeRange` by contrast IS
  mechanised** (`SiegeUnitNoticeRangeTest.cpp`, 21 hits) **and this file correctly duplicates
  neither.**

---

## 9. ⚖️ CLOSING — WHAT THIS POST-HOC GATE ACTUALLY COST

⭐ **Nothing, in the end — and that is luck, not process.** The code was correct, so running the gate
a day late cost one review cycle instead of an unreviewed artefact living in the repository forever.
⛔ **Had it been wrong, `1a457df` would already have shipped it.** The mechanism that swept it in
(`TASK-987`'s derived pathspec) is **strictly better** than a hand-written list and **must not be
reverted** — it must be **paired with the gate audit** (`TASK-987` cl. 6e), which is the clause that
found this. ⭐ **The audit worked. The dispatch is what failed, and the record says so.**
