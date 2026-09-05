# TASK-1045 — [PIN-HARDEN-2] — gameplay-programmer handoff

**Status on exit:** `ready-for-qa` · **Gate:** TASK-1046
**Subject:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp` — test 4 (c),
located **by symbol** (`FSiegeFogRetentionOneDoorAndTheAmbushExemptionIsStructuralTest::RunTest`,
the `ExemptDispatches` loop). The `:698` on TASK-1041's row is dated, as the row warned; it is `:846` today.

---

## 0. THE THREE FENCES, CHECKED BEFORE ANYTHING ELSE

| fence | result |
|---|---|
| **W-1 already closed by loop 2** — do not re-apply | ✅ Confirmed **in the file**, per the ruling that the FILE wins for W-1. `CodeLinesOnly` is at `:189` with its doc rider; the anchor runs on its output. ⛔ **I re-applied nothing.** No second comment-aware conjunct was added. |
| **TASK-1052's `From >= …Len()` break-guard must NOT be present** in `CountOccurrencesInCode` | ✅ **ABSENT — the fence held.** The inner loop at `:137-148` is still the shipped `for (;;)` with `From = Found + NeedleLength` and no bound. ⛔ I did not adopt, remove, or add it. `CountOccurrencesInCode` is **byte-unchanged** in this diff. |
| **`Tests/SiegeUnitNoticeRangeTest.cpp`'s `Clamp` pin is settled** | ✅ **Not opened, not read for edit, unmodified.** `git status` shows it clean. |

**No third disagreement between the report and the file was found**, so the exception was not extended.
The one disagreement I acted on is the one the manager already ruled: W-1 = file wins, W-4 = report wins.

---

## 1. WHAT I CHANGED — the whole behavioural diff is four things

`SiegeFogRetentionWiringTest.cpp`, **sole write**. 79 insertions / 3 deletions, of which the
executable part is:

1. **`+#include "Misc/Char.h"`** — `FChar::IsWhitespace`, named explicitly rather than leaned on
   transitively. `Containers/UnrealString.h` does **not** include `Misc/Char.h` directly (I checked the
   engine header; the path is `Misc/CString.h` → `Misc/Char.h`), and I cannot compile, so I refused to
   rely on transitivity. `FChar::IsWhitespace` is already house-used at
   `SiegeAssistantConsoleWidget.cpp:1197`, whose own doc note gives the reason I want it:
   *"⛔ NOT EndsWith(" "), BECAUSE A TAB MUST COUNT."*

2. **New fixture helper `CodeWithoutTrailingComments`** (`:220`), placed immediately after
   `CodeLinesOnly` so the two projection stages sit in composition order.

3. **The call site** (`:846`):
   ```cpp
   const FString UpdateStateCode = CodeWithoutTrailingComments(CodeLinesOnly(UpdateStateBody));
   ```
   (was `CodeLinesOnly(UpdateStateBody)`), plus a comment naming the stage and the fixed order.

4. **The (c) assertion's message tail**, which was **over-claiming**. It said *"measured on CODE ONLY,
   so a commented-out `// return;` cannot stand in for the real one."* **M-E below proves that was
   FALSE for a trailing `// return;`.** The message is now true.

⛔ **Zero lines outside `Tests/`. Zero production-behaviour change. No test added, no assertion added
or removed** (`TestEqual` 19 / `TestTrue` 9 / `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 4 — identical to HEAD).

### Why a NEW helper, disclosed rather than smuggled
The row says *"no new helper if the file's existing scanner will do."* **None will**, and this is the
sentence `qa/TASK-1047.md`'s asymmetric-disclosure finding says must be written down:
- `CountOccurrencesInCode` **cannot count `;` at all** — a one-character needle hangs it on the UE 5.8
  `Find` clamp. That is the >10 h defect documented at `:143`.
- `CodeLinesOnly` drops **whole comment lines only** — a trailing comment is on a code line.
- `CountCharacter` is comment-blind **by construction**; its own doc says *"Feed it a COMMENT-FREE
  projection."* W-4 is precisely the case where the projection it was fed was not comment-free enough.

⛔ **It is NOT a third copy of the house five-clause predicate** — it answers a different question
(*where does a comment START on a code line*), and its doc says so, so nobody keeps it in step with
`CodeLinesOnly`'s rider by mistake.

### Two deliberate refusals, both to avoid trading a loud failure for a silent one
- A `//` opens a comment **only at column 0 or preceded by whitespace**. That is what leaves
  `TEXT("http://…")` and `TEXT("a//b")` intact — truncating a **string literal** deletes real code and
  can only ever **lower** the `;` count, i.e. produce a **FALSE GREEN**. Residual: a tight `Foo();// x`
  is not stripped (fails **loud**).
- **Block comments are not stripped.** Truncating at an opener would lose the statement after a
  closer on the same line — again a false green. Residual fails **loud**.
- Termination is **structural**: a counted `for` over `Len()` that never consults `Find`, exactly the
  discipline `CountCharacter`'s doc established. ⛔ **I did not write a new `Find`-with-`StartPosition`
  loop into this file** — provably safe or not, that is the shape that cost the last run.

---

## 2. ⚠️ MY INSTRUMENT, DECLARED PRECISELY — AND IT IS A SIMULATION

⛔ **I did not compile and did not run the UE suite.** That is build-master's lane.
I observed every red below with an **out-of-engine re-implementation of the pin's source-text
scanning**, at
`…\scratchpad\pin_instrument.py` (+ `mutate.py`, `validate.py`).

**Exactly what I re-implemented, transcribed from source read this session — not from memory:**

| re-implemented | transcribed from |
|---|---|
| `FString::Find(…, FromStart, StartPosition)` **including the `Len()-1` clamp** | `String.cpp.inl:464-480` (UE 5.8), read this session |
| `FString::ParseIntoArrayLines` → `ParseIntoArray` over `{"\r\n","\r","\n"}` | `String.cpp.inl:1431` and `:1446`, algorithm copied statement for statement |
| `FString::TrimStartInline` | `String.cpp.inl:948` |
| `Mid` / `Left` / `StartsWith(CaseSensitive)` | `UnrealString.h` |
| `CountOccurrencesInCode`, `CodeLinesOnly`, `CountCharacter`, `ExtractFunctionBody`, and clause (c)'s body | the test file itself, at `c79bf5b` |

**Corroboration — five checks the instrument was NOT fitted to, so a silent divergence would show:**

| # | check | result |
|---|---|---|
| **V-1** | ⭐ Feed `CountOccurrencesInCode` the one-character needle `";"` | **HANGS**, cursor fails to advance — my clamp transcription independently reproduces the **measured** >10 h defect. This is the strongest single check I have: it is a *bug* the simulation had to reproduce, not a success. |
| **V-2** | `CountOccurrencesInCode(UnitSource, "ResolveFogClampedReachUU(")` | **1** — matches what test 4 (a) pins |
| **V-3** | `CountOccurrencesInCode(UnitSource, "ApplyFogVisionCeilingUU(")` | **10** = the 9-site table + the door's own definition, matching test 2's stated derivation |
| **V-4** | `ExtractFunctionBody` on `UpdateState()` | lands on the real signature, stops at the column-0 brace, 335 lines |
| **V-5** | projection | 335 body lines → 131 code lines (204 comment lines dropped) |

**Declared divergence, and it turned out to be NIL:** `FString` indexes UTF-16 code units on Windows;
Python indexes code points. The `UpdateState` **body** holds 3 astral characters — but **the projection
holds ZERO** (all 3 live on comment lines, which the projection drops). ⇒ every index the pin computes
is **numerically identical** in both, not merely order-isomorphic. Measured, not assumed.

**What my instrument does NOT establish, and I will not claim it does:** that the file **compiles**.
`#include "Misc/Char.h"`, `FChar::IsWhitespace`, `Line[Index]`, `Line.Left(Cut)` and the new signature
are unverified by any compiler. ⇒ **suite delta DECLARED-NOT-EXECUTED (below).** If QA wants the reds
confirmed in-engine rather than in simulation, that needs a build row — say so and I will stop rather
than approximate further.

---

## 3. THE OBSERVATIONS — every one SEEN, none reasoned about

Subject: `Siegebound/SummonedUnit.cpp`. **PRISTINE sha256 `f7534183b2c89ca504b07eb217518a2a46f79c9e7706f8a432afa9f55e943c6c`** (310,850 bytes).
Each mutation applied by a **byte anchor asserted unique (count == 1)**, restored byte-exact, hash
verified **after every single mutation**, inside a `finally`. Every run repeated after the C++ was
written, with the Python helper re-transcribed into the **exact counted-`for` form that shipped** — so
these numbers describe the shipped text, not an equivalent-looking cousin.

| # | mutation | TASK-1041 pin (CURRENT) | MY pin (PROPOSED) |
|---|---|---|---|
| **(3a) CONTROL** | clean tree, no mutation | **GREEN / GREEN** (`;`=1, 1) | **GREEN / GREEN** (`;`=1, 1) |
| **(a) W-1 decoy** | comment carrying `return;` between dispatch and real return, **real `return;` deleted** | 🔴 **RED, count `3`** | 🔴 **RED, count `3`** |
| **(b) W-4 decoy** | trailing `// dispatch; then return` on the dispatch line, **tree otherwise correct** | 🔴 **RED, count `2` — FALSE RED** | ✅ **GREEN, count `1`** |
| **(c) M1** | FOLLOW's own `return;` deleted, no comments | 🔴 **FOLLOW RED (`3`) · GROUPED GREEN** | 🔴 **FOLLOW RED (`3`) · GROUPED GREEN** |
| **(c) M2** | GROUPED's own `return;` deleted, no comments | 🔴 **GROUPED RED (`3`) · FOLLOW GREEN** | 🔴 **GROUPED RED (`3`) · FOLLOW GREEN** |
| **M-E** *(my own extra probe — see §4)* | **real `return;` deleted** AND a **trailing** `// return;` on the dispatch line | ⛔ **GREEN — FALSE GREEN** | 🔴 **RED, count `3`** |

### (3a) — the control came FIRST and it is clean
Both dispatches GREEN on the untouched tree, under **both** variants, and the two projections are
**index-identical** (D=843 R=880 / D=1430 R=1461, Retention=2049). ⛔ **My change is a literal no-op on
the shipped tree**: no code line in `UpdateState()` carries a trailing `//` comment today, so
`CodeWithoutTrailingComments(CodeLinesOnly(body))` differs from `CodeLinesOnly(body)` by **exactly one
trailing `\n`** at the very end, after every index. Measured, not argued.

### (a) — the witness duty, discharged. **COUNT = 3. It MATCHED QA's derived 3.**
The landed loop-2 fix has now been **seen red by an instrument**, which per the row it never had been.
The mechanism I measured is the one `qa/TASK-1047.md` §6.2 derives: the `// return;` line is dropped
from the projection, the anchor falls through to the Siege dispatch's return at `:1638`, and the code
lines `:1619` + `:1626` + `:1637` put **3** semicolons in the gap.
⛔ I did **not** hunt for a more contrived decoy, and I did **not** revert loop 2's projection to
manufacture a "before". The historical green is **cited from §6.2, never re-staged.**

### (c) — it still DISCRIMINATES, which is the clause that would have caught a loosened fix
M1 reds **FOLLOW only**; M2 reds **GROUPED only**; the sibling stays green in both. **Identical under
CURRENT and PROPOSED** — the ownership term is untouched, so W-4 was not bought by loosening it.

---

## 4. 🔴 FINDING — I closed W-4 at the BASIS, and doing so closed a **live FALSE GREEN** nobody had named

**This is the one thing in this handoff QA should scrutinise hardest, because it is mine and not the row's.**

The row's minimum fix for W-4 is to stop a trailing comment inflating the count. The **cheapest** way to
do that is to strip trailing comments at the **count site** only (`CountCharacter(strip(Between), ';')`).
I did **not** do that. I put the strip on the **projection**, so all four rows share one basis.

**Why — measured, as M-E:** with the count-site form, the anchor `Find(TEXT("return;"), …)` still runs
over text containing trailing comments. So:

```
UpdateStateFollow(*FollowGroup); // return;      <-- real `return;` DELETED
```

anchors `ReturnIndex` **inside the comment**, leaving `Between` = the dispatch call alone ⇒ `;`=1 ⇒
✅ **GREEN, with the regression the row exists to catch actually present.** I measured that on the
current shipped pin: **M-E, CURRENT = GREEN.** It is the *same* false green loop 2 closed for whole
comment lines, **surviving in trailing form** — and a count-site fix would have left it standing.
With the projection form: **M-E, PROPOSED = RED, count 3.**

**Consequences QA should weigh:**
- This is **not** a claim that W-1 is unfixed. Loop 2's fix is present and correct for whole comment
  lines (I witnessed it: observation (a)). M-E is the **trailing** analogue and belongs to **W-4's
  term**, which is mine.
- It is why the assertion's message needed changing: it asserted immunity it did not have.
- ⛔ It is **not** the "hunt for a more contrived decoy until something reads green" the trap clause
  forbids. That clause bans manufacturing a green for the **already-fixed W-1** by contriving decoys or
  reverting the projection. M-E reverts nothing and re-stages nothing; it is a probe of **my own fix's
  form**, run to choose between two candidate forms, and the choice is recorded here rather than made
  silently.

---

## 5. RESTORE LEDGER — 10/10 byte-exact, plus 2 `finally` restores

All ten restores (five mutations × two full runs) returned
**`f7534183b2c89ca504b07eb217518a2a46f79c9e7706f8a432afa9f55e943c6c`**, identical to pristine, verified
by sha256 **immediately after each mutation, before the next one began**. Both `finally` blocks
re-verified. Mutated hashes, recorded so a residue claim is falsifiable:

| mutation | mutated sha256 | restored byte-exact |
|---|---|---|
| M-A (W-1 decoy) | `4a19704d244d2d30fc3e69155d5a7641cca946b39bcf4f63e1fc80cb78d46fcf` | ✅ |
| M-B (W-4 decoy) | `a712abacf3c9c20c3b3d8cbc8b551c123dba8dd013d7efeb213108bfc106a31c` | ✅ |
| M-C (M1) | `edd7e7ae2328da3c3bb9e0db5ed220ab30cbb7d981989f9ba5366954a3bfcff9` | ✅ |
| M-D (M2) | `116c96cf30bd30b7e22629e3c4d5338e82da8a4a07741062ce73372c5d654055` | ✅ |
| M-E (my probe) | `89ad9fcb35829c28072447a8a6a8c1025c6019b4fb694a761f988d8a66f1f2ba` | ✅ |

**Independent corroboration that is not my own hash:** `git status --porcelain` shows
`SummonedUnit.cpp` **clean / unmodified**. A byte of residue would appear there.

---

## 6. SUITE DELTA — `DECLARED-NOT-EXECUTED` (`TL-§5c`)

**489 / 0 → 489 / 0. Delta ZERO.** No test and no assertion added or removed; the census is identical
to HEAD (`TestEqual` 19, `TestTrue` 9, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 4).
⛔ **I did not compile and did not run the suite.** The `489 / 0` figure is **carried from the
TASK-1044 dispatch note, not re-observed by me.**

Static checks I *could* run in place of a compiler, all against HEAD as the baseline:
- brace delta: `-2` in **both** HEAD and working tree ⇒ my helper is brace-balanced (the residual −2 is
  pre-existing, from braces inside string literals).
- `/*` vs `*/` delta: `-1` in **both** ⇒ **parity restored**. My first draft left two literal `/*`
  tokens inside a block comment (a `-Wcomment` diagnostic on Clang); I rewrote that prose to spell the
  opener in words. Recorded because it is the kind of thing that only surfaces at compile time.
- zero invisible/zero-width characters (an early draft of mine introduced two `U+200B`; **removed**,
  verified absent).

---

## 7. FOR QA — what to scrutinise, in priority order

1. **§4, the projection-vs-count-site choice.** It is the one judgement call I made beyond the row's
   letter, and it widens the fix's blast radius from one conjunct to the shared basis of four rows.
   The control (index-identical, no-op on a clean tree) is my defence; check it.
2. **My instrument is a simulation.** Every red in §3 is only as good as the transcription. V-1 (the
   reproduced hang) is the check I would attack first if I wanted to break my own claim.
3. **`FChar::IsWhitespace` + `Misc/Char.h`** — uncompiled. So is `Line.Left(Cut)` with `Cut == Len()`
   (returns the whole line; matches `CodeLinesOnly`'s treatment of empty lines).
4. **The whitespace qualification on `//`** — deliberate, and it means a tight `Foo();// x` still
   false-reds. I judged a loud residual better than a silent one; overrule me if that is wrong.

## 8. NOT MINE — one adjacent modification, disclosed so it is not attributed to me

`git status` also shows `Siegebound/FogVolume.h` modified. **I did not touch it.** Its diff is a
comment-only correction headed *"CORRECTED 2026-09-05 (`TASK-1050`)"*. Flagging it because my row's
scope clause is "zero lines outside `Tests/`", and a reader checking `Source/` will see two paths, not one.
