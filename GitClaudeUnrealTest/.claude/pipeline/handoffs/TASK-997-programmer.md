# TASK-997 — [ARCHER-PROVENANCE], AS RESTATED — programmer handoff

**Status:** `ready-for-qa`
**Date:** 2026-09-04
**Row read:** `.claude/pipeline/TASKBOARD.md` → `TASK-997`, **the RESTATED text**, in full, before any edit.
**Law cited:** ⭐⭐⭐ `FOG-§9.11` · ⭐ `SC-§73` · ⭐ `SC-§38a` · ⭐ `SC-§60` · `SC-§38` · `SC-§39` · `SC-§62`

---

## ⛔ THE HEADLINE, FIRST: **NO NUMERIC CELL WAS CHANGED. ANYWHERE.**

Not in the fog-cut table, not in the tests, not in the data. The diff is **one code token** and
**one block of prose**. Every figure this row was *originally* written to move is **still on disk
unchanged**, because ⭐ `TASK-1004` made those figures **true again** this morning.

- ⛔ Items **(1) (2) (3) (4) (5)** of the original row were **NOT executed, not partially executed,
  not "verified then re-applied".** They are cancelled and I treated them as **non-instructions**.
- ⛔ I re-checked the cancellation **against disk**, not against the board's word (below).

---

## What actually shipped — exactly two edits

### ➊ (5a) — the one-token fix. `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogTest.cpp`

Origin: **`qa/TASK-981.md` WARN-1** — ⛔ *not* `J-F29`, which is why it outlived the revision.

```cpp
// before
NegativeCeilingCurve.FogVisionCeilingUU = -609.6f;
// after
NegativeCeilingCurve.FogVisionCeilingUU = -DerivedCeilingUU;
```

- Located **by symbol** (`NegativeCeilingCurve.FogVisionCeilingUU`), not by the row's `:956` hint.
  The hint happened to be accurate, but the file is dirty from `TASK-981` and I did not rely on it.
- `DerivedCeilingUU` is the fixture constant at `SiegeFogTestFixture` (`CeilingFeet *
  CentimetresPerFoot`). It is **already in scope** at this site — the same block reads it at
  `ProbeDistances[]` a few lines up, so this is not a new dependency.
- **Why it was a defect:** this file's own header states *"NEITHER `304.8` NOR `609.6` APPEARS
  ANYWHERE IN THIS FILE AS A CODE LITERAL (only in prose…)"* and *"there is exactly ONE `609.6`…
  in FSiegeFogTuning"*. `SiegeFogStatics.h` repeats *"There is exactly ONE `609.6` in the codebase
  and it is this line."* The literal falsified both.
- **Zero behaviour change, and the reason is stronger than "the values match":** the assertion is
  *"a NEGATIVE ceiling yields EXACTLY CLEAR"*. It depends on the **sign only** — QA-981 says so
  outright (*"the value is arbitrary — any negative works"*). ⚠️ **QA should note this explicitly:**
  `20.f * 30.48f` and `609.6f` are **different bit patterns** (this file's own `:129` says so), so
  the magnitude did move by up to 1 ULP. It cannot matter here, and it would matter at a site that
  compared magnitudes. This one does not.

### ➋ (5c) — the disambiguation. `Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.h`

**Prose only, in the fog-cut table's header block, above the table.** The table itself — the column
header row and all eight data rows — is **byte-identical**.

Added to the `FOG-§2` table banner (between *"nothing here is estimated:"* and the closing rule):

- ⛔ **every percentage below is a FIRING-RANGE cut** — not a notice cut, not a chase cut;
- ⛔ the rows reading ✅ UNAFFECTED are unaffected **in firing range only**;
- ⛔ **nothing in the table was corrected — it was AMBIGUOUS, not wrong** (stated in the comment
  itself, so a future reader does not read the banner as an erratum);
- ⭐ **why** the ambiguity is new: notice **==** firing for a ranged unit until today, so one number
  served both gates and the block was unambiguous **by accident**; ⭐⭐⭐ `FOG-§9.11` retired that
  identity, and notice is now its own gate (`ASummonedUnit::UnitEngagementRadiusUU`), one value for
  every shipped card — a **DEFAULT, never a cap** — while `shipped Range` stays **per-card**;
- ⇒ ⛔ **the notice cut is a single figure, not a table, and it is none of the numbers below**:
  **87.8%** (`1 − 609.6/5000`);
- ⇒ ⛔ **cite a row below ONLY for firing range**; for acquisition/chase/leash cite `FOG-§9.11`.

---

## ⚠️ THE ONE JUDGEMENT CALL, DECLARED RATHER THAN BURIED

The dispatch said *"do not change, **add**, or recompute a single number."* The board row's own
`spec:` (5c) said *"state … that NOTICE is a SEPARATE gate at `5000` cutting `87.8%`"*. Those two
sentences pull in opposite directions on **new prose**, and I read the prohibition as scoped to the
**table's cells** — which is how the row's `names:` line words it (*"ZERO NUMERIC CELLS"*,
*"`:177`/`:178` are OFF LIMITS"*) and how the spec words its FAIL condition (*"a diff to any
**numeric cell** ⇒ FAIL"*).

So the new prose contains exactly **two** figures, and ⛔ **neither is derived here**:

| figure | where I took it from, verbatim |
|---|---|
| `5000` | `FOG-§9.11`'s own table (`UnitEngagementRadiusUU` `2000` → **`5000.f`**) and the shipped `SummonedUnit.h` `static constexpr float UnitEngagementRadiusUU = 5000.f;` |
| `87.8%` (`1 − 609.6/5000`) | `CONVENTIONS.md` `FOG-§9.9` corrected row, **character-for-character**; corroborated at `SummonedUnit.h` (*"fog costs a unit 87.8% of its notice RADIUS"*) and `SummonedUnit.cpp` (*"87.8% of the radius (609.6 / 5000)"*) |

⛔ **If QA reads the prohibition as absolute, the finding is one line to fix** — strike the
parenthesised figures and leave the *qualitative* claim (firing ≠ notice, notice is steeper, cite
`FOG-§9.11`). I did **not** take that reading, because a header that says *"the notice cut is a
different number"* **without saying which number** sends the reader back to the same lane that
produced `J-F31` three times today. Flagged here so the call is reviewable rather than assumed.

One derived **comparison** also appears — *"steeper than every firing cut in this table, the
Longbowman's included"* (`87.8 > 83.1`). It introduces **no number**, and it is the reason the
figure matters: the notice cut is **not bounded** by anything in the table, and it **bites the melee
rows**, whose firing cut is genuinely zero. That row (`120 → 120  ✅ UNAFFECTED`) was the single most
misreadable cell in the file after `FOG-§9.11`, and it is now covered.

---

## Suite delta

| | delta |
|---|---|
| new tests | **0** |
| assertions re-derived | **0** |
| assertions removed / renamed | **0** |
| behaviour change | **0** |

⛔ **Stated as a DELTA on purpose — nothing in this batch has been executed, so I assert no absolute
count.** No compile, no editor, no Git was run (out of scope, and the build-master's job).

**The delta is 0 by mechanism, not by hope** — I checked, rather than assumed:

- ⭐ Every prose-census over `SiegeFogStatics.h` goes through `SiegeFogClampTest.cpp`'s
  `CountOccurrencesInCode`, which **skips comment lines** (`//`, `* `, `*/`, `/*`, bare `*`).
  **Every line I inserted is a `*` doc-comment continuation** ⇒ invisible to all of them.
- The three sites that load that header by path were each checked:
  the `AsymmetryTokens` loop (code-only counts) · the `ClampMin = "304.8"` count (untouched, still 1
  in code) · the raw `FogHeader.Contains(TEXT("THE VISUAL'S CURVE ONLY"))` probe (that string is
  untouched and still present).
- Nothing pins `609.6` **tree-wide**, and `CountAcrossShippingSource` **excludes `/Tests/`**, so the
  (5a) token is invisible to the census machinery in both directions.

---

## ⛔ What QA should scrutinise

1. ⭐⭐ **THE (5a) DIFF SHOWS NO REMOVED LINE — AND THAT IS CORRECT, NOT A MISSING EDIT.**
   `git diff` vs `HEAD` renders my fix as a lone **`+ … = -DerivedCeilingUU;`** with **no matching
   `-609.6f` deletion**, because the whole `(c2)` block is `TASK-981`'s **still-uncommitted** work —
   the literal never existed in `HEAD`. ⛔ **A reviewer diffing against `HEAD` and looking for the
   removal will not find it and may call the item unshipped.** Verify by **census** instead:
   `grep -n "609\.6f\|304\.8f" Tests/SiegeFogTest.cpp` must return **only prose hits** (the header's
   own two self-referential mentions), and **zero code lines**. That is the file's stated law.
2. ⛔ **The numeric fence — please verify it as an ABSENCE, with a positive control.** These were
   confirmed present and unmodified after my edits, and all are **correct today**:
   `SiegeFogStatics.h` — `Archer 2100 / -71.0%` · `Wizard 2100 / -71.0%` · `Longbowman 3600 /
   -83.1%` · every tower row · Cleric · melee, and the `under a 609.6 ceiling` column header;
   `Tests/SiegeFogTest.cpp` — `ArcherRange = 2100.f; // == the Wizard's` · `0.710f` **and its test
   name** *"The ARCHER and the WIZARD lose 71.0%"* (⛔ the name still matches its number) ·
   the trailing `// 2100` at `ArcherRange,` in the ceiling/range/clamp probe lists.
3. ⚠️ **The judgement call in the section above** — the two quoted figures in new prose. Adjudicate
   it explicitly rather than letting it pass silently; it is a one-line fix either way.
4. ⛔ **Scope:** I touched **no** `cards.csv`, `DT_Cards`, `CardRow.h`, `SummonedUnit.*`. ⛔ Item
   **(5b)** was **not** attempted here — it is ⭐ `TASK-1000`'s, and `SummonedUnit.h` is not in this
   row's fence (`SC-§62`).

---

## ⭐ Confirmed against disk, not against the board's word

The row warned that executing its retired items would be **active harm**. I verified the
cancellation is real **before** trusting it, so the "don't do it" is evidence-backed rather than
obeyed:

- `Tests/SiegeFogTest.cpp` still reads `ArcherRange = 2100.f` ⇒ ⭐ `TASK-1004`'s revert **did** land
  on the test side.
- The table still reads `Archer 2100 / -71.0%`, `Wizard 2100 / -71.0%`.
- `1 − 609.6/2100 = 0.7097` ⇒ **`-71.0%` is the true figure for `2100`.** The retired item (2) would
  have written `-69.5%` (the `2000` figure) beside a `2100` that ⭐ `TASK-1004` restored, i.e. it
  would have made a table **false** under a header promising *"nothing here is estimated"* —
  ⛔ **exactly the defect this row exists to repair.** Not executed.

⚖️ **Neither surviving item was overtaken by another row.** Both were still live on disk when I
opened the files, so there was nothing to report as already-fixed and nothing was manufactured to
justify the dispatch.

---

## Files touched

- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogTest.cpp` — **1 token** (5a)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.h` — **prose insertion only**, in the
  `FOG-§2` table's header banner (5c)
- `.claude/pipeline/handoffs/TASK-997-programmer.md` — this file
- `.claude/pipeline/TASKBOARD.md` — **`TASK-997`'s own `status:` line only**

**Assets referenced:** none. **Engine / MCP touched:** none. **Git:** none.
