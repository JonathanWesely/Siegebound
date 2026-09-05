# TASK-1041 — [PIN-HARDEN] handoff · gameplay-programmer · 2026-09-05

**Status → `ready-for-qa`** · Gate: **`TASK-1042`** · Input: `qa/TASK-1039.md` **W-1** + the substring finding
**Instant reviewed:** `HEAD` = `12b8707`, index **EMPTY**, and it is **still** empty and still `12b8707` at the end of this row.

> ⭐ Nothing shipped was at risk. The **code** in `12b8707` is correct (QA verified it by reading). This row
> repairs **two guards**, both of which were green against regressions they claimed to catch.

---

## ⛔ WHAT CHANGED — THREE FILES, ALL UNDER `Tests/`

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp` | **Defect 1** — the ordering pin in test 4 (c) gains the **ownership** term |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogReachSeamTest.cpp` | **Defect 2** — new fixture helper + **1 of 4** bare pins swept |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBrightSunTest.cpp` | **Defect 2** — new fixture helper + **3 of 4** bare pins swept |

⛔ **ZERO lines changed outside `Tests/`.** ⛔ **ZERO behaviour change** — no shipped runtime file is touched by
this diff, and the five files the mutations *temporarily* wrote are byte-identical to `12b8707` (§4).

⛔ **NOT an amend.** `Tests/SiegeFogRetentionWiringTest.cpp` shipped inside `12b8707`; this is a **follow-up diff
against tracked content**. `12b8707` was neither amended nor reverted, and **nothing was staged** — a later
commit host takes it after `TASK-1042`.

---

## ⛔⛔ §1 — DEFECT 1: THE PIN THAT COULD NOT FAIL

### The defect, re-derived here rather than taken on relay

`FSiegeFogRetentionOneDoorAndTheAmbushExemptionIsStructuralTest`, part (c) — located **by symbol**, not by line:

```cpp
const int32 ReturnIndex = UpdateStateBody.Find(TEXT("return;"), ESearchCase::CaseSensitive, ESearchDir::FromStart, DispatchIndex);
… && ReturnIndex < RetentionIndex
```

`Find` from `DispatchIndex` returns the **first `return;` anywhere after the dispatch**, and `ASummonedUnit::UpdateState()`
holds **three more** unconditional ones before the retention line (Siege, Support, Witch). ⇒ delete the FOLLOW
dispatch's own `return;` and the assertion finds the **Siege** one, which is *also* before the retention line, and
the row stays **green through the exact regression its own message names**.

### The fix — QA's own one-line form, and it is **OWNERSHIP**, not ordering

```cpp
const FString Between = (DispatchIndex != INDEX_NONE && ReturnIndex != INDEX_NONE && ReturnIndex > DispatchIndex)
    ? UpdateStateBody.Mid(DispatchIndex, ReturnIndex - DispatchIndex)
    : FString();
…
    && ReturnIndex < RetentionIndex
    && CountOccurrencesInCode(Between, TEXT(";")) == 1);
```

Exactly **one `;`** — the dispatch call's own — may sit between the dispatch and the `return;` claimed for it.

⛔ **VERIFIED TO CLOSE THE NAMED DEFECT, NOT ASSUMED** (this was the explicit instruction): see **M1/M2** below.
Both mutations read **GREEN on the shipped pin** and **RED on the hardened one**, and the red lands **only on the
dispatch actually mutated** — the sibling row stays green, so the pin discriminates rather than merely failing.

⛔ **A stale index cannot buy a comfortable pass:** `Between` is the **empty string** when either index is
`INDEX_NONE`, which counts **0**, not 1. (The pre-existing `!= INDEX_NONE` conjuncts already covered that; the new
term does not re-open it.)

### ⚠️ ONE RESIDUAL LIMITATION, DECLARED RATHER THAN DISCOVERED

`Find(TEXT("return;"), …)` is **not** comment-aware, while `CountOccurrencesInCode` **is** (`SC-§80`). So a
**comment** containing the literal text `return;`, placed between a dispatch and its real return, would still
anchor `ReturnIndex` early. ⛔ **I did not invent a third form for it** — the dispatch fenced that explicitly, the
prescribed form closes the defect that was measured, and this residual is a strictly more contrived shape. Naming
it here so it is on the record rather than found later.

---

## ⛔⛔ §2 — DEFECT 2: THE SUBSTRING SWEEP · **MY OWN CENSUS = FOUR**

### The census, re-derived (`grep -rn 'TEXT("FogVisionCeilingUU")' Source/GitClaudeUnrealTest/`)

| # | file | subject scanned | form |
|---|---|---|---|
| 1 | `Tests/SiegeBrightSunTest.cpp` | `FogVolume.cpp` (whole file) | `CountOccurrencesInCode(FogCpp, …) == 0` |
| 2 | `Tests/SiegeBrightSunTest.cpp` | `FogVolume.h` (whole file) | `CountOccurrencesInCode(FogH, …) == 0` |
| 3 | `Tests/SiegeBrightSunTest.cpp` | `SiegeGameMode.cpp`, via the `PolicyNeedles[]` loop | table entry, expected `0` |
| 4 | `Tests/SiegeFogReachSeamTest.cpp` | **`ResolveFogClampedReachUU`'s body** | `CountOccurrencesInCode(SeamBody, …) == 0` |

⭐ **FOUR — I confirm the corrected count and confirm the relayed count of three was short by row 4.**
⛔ **All four swept. Zero survivors.** (`TASK-1042` item 2 needs no exception list.)

### The mechanism

`ApplyFogVisionCeilingUU` **contains** `FogVisionCeilingUU`. New fixture helper, added to **both** files (they are
separate translation units with their own fixture namespaces; the house rule is copy-verbatim, never share):

```cpp
static int32 CountBareCeilingMemberReferences(const FString& Source)
{
    return CountOccurrencesInCode(Source, TEXT("FogVisionCeilingUU"))
        - CountOccurrencesInCode(Source, TEXT("ApplyFogVisionCeilingUU"));
}
```

⭐ **The subtraction is EXACT, not an approximation, and both halves are load-bearing:** the door's spelling
contains the member's spelling **exactly once**, and `CountOccurrencesInCode` counts **non-overlapping** matches
(`From = Found + NeedleLength`) ⇒ every door call contributes exactly one false hit and exactly one is removed.
⇒ the result can never go negative, and **adding door calls can never mask a bare reference** — proven by **M5**.

Row 3 sits in a loop, so the loop became a **paired table** rather than a bare needle list:

```cpp
struct FPolicyNeedle { const TCHAR* Needle; const TCHAR* PermittedSuperstring; };
{ TEXT("FogVisionCeilingUU"), TEXT("ApplyFogVisionCeilingUU") },   // the only entry that needs one
```

⛔⛔ **The discount is PER-ENTRY and never blanket, and that is the interesting part of this half.**
`FogPrevent` in the same array is a **prefix needle on purpose** — it exists to catch
`FogPreventedUntilTimeSeconds`. A generic "strip superstrings" version of this fix would have **deleted a live
guard while looking identical to the correct one**. ⇒ **M10** exists solely to prove that needle still reds.

### ⛔ Direction of failure, stated plainly

All four failed **RED, not green**: the unhardened needle **over-counts**, so nothing was hidden and nothing
shipped wrong. The hazard is a **false red landing on a future row that did nothing wrong** — and the cheapest
way out of a false red is to weaken the pin. That is what this sweep prevents.

### ⛔ THE `Clamp` HALF — DELIBERATELY NOT TOUCHED, WITH THE REASON

`Tests/SiegeUnitNoticeRangeTest.cpp` pins `CountOccurrencesInCode(ResolverBody, TEXT("Clamp")) == 0` over
`ASummonedUnit::ResolveNoticeRadiusUU`. `ResolveFogClampedReachUU` contains `Clamp` — but **`ApplyFogVisionCeilingUU`
does not**, so **routing that resolver through the door cannot trip it**. Only a **direct seam call** inside
`SummonedUnit.cpp` can, and such a call is already RED at two independent rows (funnel test 9's authorised-chokepoint
table, and this batch's test 4 (a): *one* seam call in the file, *inside* the door). ⇒ hardening it would add a
term that no reachable mutation can exercise. **Left alone on purpose; it is not one of the four.**

---

## ⛔⛔⛔ §3 — `SC-§83`: THE MUTATION LOG · **EVERY AUTHORED/MODIFIED PIN WAS SEEN RED**

**Instrument.** No compile and no engine are in scope, so the pins were exercised by a **faithful port of the two
house scanners** (`CountOccurrencesInCode` — comment-skipping, non-overlapping — and `ExtractFunctionBody` —
signature → first `\n}`), driven over the **real files on disk**. The pins under test are pure functions of source
**text**, so this reproduces the assertion exactly; what it does **not** reproduce is compilation, which is
`TASK-1042`/build-master's. Harness: `…/scratchpad/task1041_mutations.py` (scratchpad, **not** in the repo).

**Protocol, applied to every mutation without exception:** read original **bytes** → `sha256` → mutate → write →
**re-read off disk** and assert the on-disk text equals the intended mutation → evaluate **both** the `12b8707` pin
and the `TASK-1041` pin → **restore the original bytes in a `finally`** → **`sha256` again** → stop on mismatch.

### C0 — THE CONTROL FIRST (`SC-§39`)

On the **clean** tree the hardened pins are **GREEN**: ordering pin green for both dispatches, and
`;`-between = **1** for `UpdateStateFollow(*FollowGroup);` and **1** for `UpdateStateGrouped(*Group);`; bare-ceiling
count **0** for `FogVolume.cpp`, `FogVolume.h`, `SiegeGameMode.cpp` and the seam body (seam body extracted,
2522 chars — not an empty scan). ⛔ Without this, every red below would be unreadable.

### Baseline `sha256` of the five subject files (recorded **before** the run, `sha256sum`)

| file | sha256 |
|---|---|
| `Siegebound/SummonedUnit.cpp` | `f7534183b2c89ca504b07eb217518a2a46f79c9e7706f8a432afa9f55e943c6c` |
| `Siegebound/FogVolume.cpp` | `3907fedd5c24d1b56df406e5d4ffed63a82715f5d851655767fa43d47fa011db` |
| `Siegebound/FogVolume.h` | `0032914fe9442d24239aa547998ae283134ae08e69d6e0bc108e7e388fa54c2f` |
| `Siegebound/SiegeGameMode.cpp` | `6a310a6942ad67b5525a94a5b9adf28d1540f48096d707dc98959ae211f5edf6` |
| `Siegebound/SiegeCombatStatics.cpp` | `70d235ed4e43aa73a27cca853802a31907934f124e3a44071b0a33ed7f27b34f` |

⭐ `SummonedUnit.cpp`'s `f7534183…` is the **same value `qa/TASK-1039.md` cites** for the `TASK-1008` mutation
round — an independent corroboration that this tree is the reviewed one.

### The 12 mutations

| # | subject | mutation | `12b8707` pin | **`TASK-1041` pin** | restored byte-exact (verifying `sha256`) |
|---|---|---|---|---|---|
| **M1** | `SummonedUnit.cpp` | delete the **FOLLOW** dispatch's own `return;` | 🟢 **GREEN** (the defect) | 🔴 **RED** — FOLLOW only; GROUPED stays green | ✅ `f7534183…43c6c` |
| **M2** | `SummonedUnit.cpp` | delete the **GROUPED** dispatch's own `return;` | 🟢 **GREEN** (the defect) | 🔴 **RED** — GROUPED only; FOLLOW stays green | ✅ `f7534183…43c6c` |
| **M3** | `FogVolume.cpp` | `+= Tuning.FogVisionCeilingUU;` (**bare**) | 🔴 RED (1) | 🔴 **RED (1)** — pin **1/4 seen red** | ✅ `3907fedd…011db` |
| **M4** | `FogVolume.cpp` | `+= ApplyFogVisionCeilingUU(1.f);` (**legal**) | 🔴 RED (1) — **the false red** | 🟢 GREEN (0) — closed | ✅ `3907fedd…011db` |
| **M5** | `FogVolume.cpp` | `+=` **both** | 🔴 RED (2) | 🔴 **RED (1)** — the discount **cannot mask** a bare ref | ✅ `3907fedd…011db` |
| **M6** | `FogVolume.h` | `+=` **bare** | 🔴 RED (1) | 🔴 **RED (1)** — pin **2/4 seen red** | ✅ `0032914f…4c2f` |
| **M7** | `FogVolume.h` | `+=` **legal door call** | 🔴 RED (1) — false red | 🟢 GREEN (0) — closed | ✅ `0032914f…4c2f` |
| **M8** | `SiegeGameMode.cpp` | `+=` **bare** | 🔴 RED (1) | 🔴 **RED (1)** — pin **3/4 seen red** | ✅ `6a310a69…5edf6` |
| **M9** | `SiegeGameMode.cpp` | `+=` **legal door call** | 🔴 RED (1) — false red | 🟢 GREEN (0) — closed | ✅ `6a310a69…5edf6` |
| **M10** | `SiegeGameMode.cpp` | `+= FogPreventedUntilTimeSeconds;` | — | 🔴 **`FogPrevent` STILL RED** (the per-entry discount weakened **nothing**); ceiling pin correctly green | ✅ `6a310a69…5edf6` |
| **M11** | `SiegeCombatStatics.cpp` | **inside the seam body** `+=` bare `FogTuning.FogVisionCeilingUU` | 🔴 RED (1) | 🔴 **RED (1)** — pin **4/4 seen red** | ✅ `70d235ed…7b34f` |
| **M12** | `SiegeCombatStatics.cpp` | **inside the seam body** `+=` legal door call | 🔴 RED (1) — false red | 🟢 GREEN (0) — closed | ✅ `70d235ed…7b34f` |

⇒ **5 pins authored/modified, 5 seen RED** (the ordering pin at M1 **and** M2, plus one red each for the four bare
pins). ⛔ **`SC-§83` satisfied by measurement, not by assertion** — and the law's own warning was live here: the
ordering pin is the one *this batch authored*, and it is the one that had never been mutated.

⚠️ **DECLARED:** M11/M12 insert text that would not compile in that context. That is intentional and harmless —
these four pins are **source-text probes**, so the mutation only has to be textual to exercise them; nothing was
compiled, and every insert was reverted inside the same process.

⚠️ **One harness bug, disclosed because it touched the tree:** the first run aborted on a **console-encoding**
error (`cp1252` on a box-drawing character) *after* M1 had already been written and restored. The `finally`
restore had already run; `sha256sum -c` confirmed all five subjects **OK** before I re-ran. A second abort (M11's
anchor: `SiegeCombatStatics.cpp` is **CRLF**, the rest of the touched set is LF) happened **before any write** —
`mutate()` runs ahead of the `try` block. **No mutation was ever left on disk.**

---

## ⛔ §4 — THE TREE IS CLEAN, AND BY WHAT INSTRUMENT

Three **independent** instruments, all after the full 12-mutation run:

1. **`sha256sum -c`** against the manifest recorded *before* the run — **5/5 `OK`**, the strongest of the three
   (content identity, not metadata).
2. **`git status --porcelain`** — the only modified `Source/` paths are the **three test files** of this diff.
   ⛔ **None of the five subject files appears at all.**
3. **`git diff --stat`** — `SiegeBrightSunTest.cpp` (+70/−…), `SiegeFogReachSeamTest.cpp`, `SiegeFogRetentionWiringTest.cpp`,
   plus three `.claude/` files that were **already dirty at my start and are not mine** (`agents/qa-reviewer.md`,
   `pipeline/CONVENTIONS.md`, `pipeline/TASKBOARD.md` — I edited **only TASK-1041's own `status:` line** in the last).

Also verified: **index EMPTY** (`git diff --cached --stat` returns nothing) and **`HEAD` = `12b8707`**.
⛔ No `checkout` / `restore` / `stash` / `reset` / `clean` was run — the only git commands used were
`show` / `status` / `diff` / `rev-parse` / `log`.

**Line endings:** all three edited test files were **pure LF** at `12b8707` and are **pure LF** now
(CRLF = 0). No mixed-ending damage.

---

## ⛔ §5 — SUITE DELTA (`TL-§5c`)

**0 tests added · 0 removed · 0 renamed.** Every change is *inside* an existing assertion or is a fixture helper.
⇒ **489 / 0 → 489 / 0, unchanged.** ⛔ **DECLARED, NOT EXECUTED** — no compile is in this row's scope, so the
count is carried from `12b8707` and the *runtime* result of these five assertions is unobserved by me. The
`TASK-1042` reviewer should mark it **accepted-as-declared**; the first real execution is build-master's.

---

## ⛔ §6 — WHAT `TASK-1042` SHOULD SCRUTINISE HARDEST

1. **The `;`-count claim.** Read `UpdateState` and satisfy yourself there is **exactly one `;`** between each
   dispatch and its own `return;` **today** — otherwise this diff false-reds the clean tree. (Measured: 1 and 1.)
2. **Is folding into the existing conjunction right?** I used **QA's own one-line form** rather than a separate
   `TestTrue` row. A separate row would give a distinct failure message; folding keeps the diff minimal and matches
   the prescription exactly. **Flagged as a judgement call, not hidden.**
3. **The subtraction's exactness.** It relies on `ApplyFogVisionCeilingUU` containing `FogVisionCeilingUU` **once**
   and on the scanner being **non-overlapping**. Both re-checked at source; if either changed, the helper's doc
   comment is the thing that would go stale.
4. **The local `struct FPolicyNeedle`** in `SiegeBrightSunTest.cpp` — function-local aggregate, `nullptr` for the
   four entries that need no discount. Confirm the `FogPrevent` prefix semantics survived (**M10** says they did).
5. **`ExtractFunctionBody` on a CRLF file.** `SiegeCombatStatics.cpp` is CRLF; the helper searches `"\n}"`, which
   `"\r\n}"` contains, so extraction is unaffected. Worth a second pair of eyes.
6. **Scope:** confirm for yourself that **no non-test line moved**. My three instruments say so; yours should be
   independent of mine.

---
---

# ⛔⛔ LOOP 2 — THE HANG, ITS MECHANISM, AND THE FIX

**Agent:** gameplay-programmer · **Date:** 2026-09-05 · **Loop:** 2 of max 3
**Return route:** routing rule 6, via `handoffs/TASK-1043-buildmaster.md` §4
**Board HEAD at time of writing:** `ef2c901` · scope: the same three unstaged `Tests/` files
⚠️ **Everything above this line is loop 1's record and is unchanged.** `qa/TASK-1042.md`'s PASS
covered the loop-1 diff and is **SPENT**: the row it gated no longer exists in this form.

---

## L2-§1 — THE MECHANISM. It is an ENGINE DEFECT, and the diff walked into it.

**`FString::Find(…, ESearchDir::FromStart, StartPosition)` clamps `StartPosition` to `Len() - 1`,
not to `Len()`.** Read at source in the installed engine:

`C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Containers/String.cpp.inl:464-482`

```cpp
const ElementType* Start = **this;
int32 RemainingLength = Len();
if (StartPosition != INDEX_NONE && RemainingLength > 0)
{
    const ElementType* End = Start + RemainingLength;
    Start += FMath::Clamp(StartPosition, 0, RemainingLength - 1);   // (Len - 1), NOT Len
    RemainingLength = UE_PTRDIFF_TO_INT32(End - Start);
}
const ElementType* Tmp = TCString<ElementType>::Strnstr(Start, RemainingLength, SubStr, SubStrLen);
return Tmp ? UE_PTRDIFF_TO_INT32(Tmp - **this) : INDEX_NONE;
```

⇒ when a scan-forward cursor reaches **exactly `Len()`**, the call does **not** return `INDEX_NONE`.
It clamps back onto the **final character**, leaves `RemainingLength == 1`, and `Strnstr`
(`Public/Misc/CString.h:919`, which accepts `InStrLen == FindLen`) **re-finds that character**.

Feed that into the house helper's inner loop:

```cpp
int32 From = 0;
for (;;)
{
    const int32 Found = Trimmed.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
    if (Found == INDEX_NONE) { break; }
    ++Count;
    From = Found + NeedleLength;      // recomputes the SAME value, forever
}
```

For `Trimmed = "UpdateStateFollow(*FollowGroup);"` (Len 32) and `Needle = ";"`:
`Found = 31 → From = 32 → Found = 31 → From = 32 → …` **The `for (;;)` never terminates.**

### L2-§1a — Why this is a HANG and not a crash or a red

The loop allocates nothing and asserts nothing. It just increments an `int32` on the **game
thread**. That is exactly what build-master measured: `Test Started` logged, frame counter frozen
at `[899]` (suite) / `[303]` (isolated probe), **no** `Test Completed`, no error, no crash, ten
hours consumed. ⭐ Build-master's refusal to name a mechanism was correct — neither helper *reads*
as non-terminating, because **the non-termination is not in the helper. It is in `Find`.**

### L2-§1b — ⭐⭐ WHY 566 PRIOR CALLS SURVIVED, AND WHY MINE DIED ON CONTACT

The loop can only pin when a match **ends at the end of a trimmed line**, and re-matching a
one-character window then requires the needle to be **exactly one character** equal to that last
character. A needle of ≥ 2 characters **cannot** match a 1-char window, so it always terminates.

**Repo audit, `Source/` (`scratchpad/ue_find_proof.py` §C):**

| measure | value |
|---|---|
| literal needles passed to `CountOccurrencesInCode` in the tree | **566** |
| of those, needles of length **1** — the hang condition | ⛔ **1** |
| the one | `SiegeFogRetentionWiringTest.cpp:740`, `TEXT(";")` — **loop 1's own line** |

The fixture doc had already stated the invariant without knowing why it mattered: *"Every needle
below is a call shape carrying an open paren, never a bare token."* A `;` is, by construction, the
**last character of a C++ statement line**. It pinned on the very first line it scanned.

### L2-§1c — A wrong cause I killed with a measurement, so QA need not re-kill it

`FString::Printf`'s buffer-doubling loop (`String.cpp.inl:1742`) was an attractive suspect. It is
**refuted**: measured in UTF-16 units, the message was **550 / 545** at `HEAD` and **792 / 787**
after loop 1 — **both already exceed the 512 stack buffer**, so the realloc path was exercised and
passing before this diff ever landed. (`scratchpad/fmtlen.py`.) I also cleared unity-build namespace
collision: the three fixtures are `SiegeFogRetentionWiringFixture` / `SiegeBrightSunFixture` /
`SiegeFogReachSeamFixture` — distinct.

---

## L2-§2 — THE FIX. ⛔ The pin is not weakened; it is strictly stronger.

Two new fixture helpers in `SiegeFogRetentionWiringTest.cpp`. **`CountOccurrencesInCode` is
untouched** — it is copied verbatim into ≥ 4 test files by the law in its own doc comment, and this
row may repair only three files. A 3-of-N edit *is* the divergence that law forbids. ⇒ the
single-character case is taken **out of** the string-needle helper instead.

| helper | what it does | why it cannot hang |
|---|---|---|
| `CodeLinesOnly(const FString&)` | returns the source with every **comment line dropped**, using the house predicate character-for-character, newline-normalised | bounded `for` over `ParseIntoArrayLines`; no `Find` |
| `CountCharacter(const FString&, TCHAR)` | counts one character | counted `for` over `Len()`; **never consults `Find`** |

The `(c)` block now takes **every** index — retention, dispatch, `return;` anchor, `Mid` — on
`UpdateStateCode = CodeLinesOnly(UpdateStateBody)`, and counts with `CountCharacter(Between, TEXT(';'))`.

### L2-§2a — ⭐⭐ The projection closes a SECOND defect, and it was a live FALSE GREEN

The orchestrator flagged the untested interaction: the `Find(TEXT("return;"), …)` anchor is
**comment-blind** while the count was **comment-aware**. Run both over one span and a fall-through
that leaves a commented-out `// return;` behind **anchors the pin on a line that cannot execute**,
while the count skips that line and still reads a comfortable **1**. **Measured, not argued:**

| source | where the anchor lands | comment-aware `;` count | verdict |
|---|---|---|---|
| shipped | `return;` | 1 | GREEN ✅ correct |
| `return;` → `// return;` | ⛔ `// return;` | 1 | ⛔ **GREEN — FALSE** |

Projecting **first** and measuring **only** on the projection makes the two passes agree **by
construction**. Under the fix that same mutation reads **RED (3 semicolons)**. ⇒ loop 1 shipped a
pin that a commenting-out could have walked straight through; it cannot now.

---

## L2-§3 — ⭐ SC-§83: THE PIN SEEN RED, ON THE REAL BYTES

The row's entire input is the bytes of `SummonedUnit.cpp`, so the scanner was run against
**disk-mutated** source, read back exactly as the test reads it (`scratchpad/disk_mutation.py`),
with the UE 5.8 `Find` semantics modelled exactly — **including** the clamp defect, instrumented, so
"the fixed path never reaches it" is a **count**, not a claim.

| # | source under test | FOLLOW row | GROUPED row | clamp-defect hits |
|---|---|---|---|---|
| 1 | shipped, **loop-1 code** | ⛔ **DID NOT TERMINATE** | ⛔ **DID NOT TERMINATE** | 199,998 |
| 2 | shipped, **fixed code** | ✅ GREEN (`;`=1) | ✅ GREEN (`;`=1) | **0** |
| 3 | ⭐ **FOLLOW's own `return;` DELETED** (on disk) | 🔴 **RED** (`;`=3) | ✅ **GREEN** | **0** |
| 4 | `return;` → `// return;` (fall-through in camouflage) | 🔴 **RED** (`;`=3) | ✅ GREEN | **0** |

**Row 3 is the SC-§83 obligation and it is satisfied exactly as specified: the hardened pin goes RED
against the regression it names, and the sibling row stays GREEN.** Row 1 reproduces the hang
analytically, which is the counterfactual build-master could not take.

### L2-§3a — Byte-exact restore, hash-verified

⛔ **No `git checkout --` / `restore` / `stash` / `reset` / `clean` at any point.** The disk mutation
restored from an in-process byte copy inside a `finally`.

```
sha256 BEFORE  : f7534183b2c89ca504b07eb217518a2a46f79c9e7706f8a432afa9f55e943c6c   (310850 bytes)
sha256 MUTATED : edd7e7ae2328da3c3bb9e0db5ed220ab30cbb7d981989f9ba5366954a3bfcff9   (310837 bytes)
sha256 AFTER   : f7534183b2c89ca504b07eb217518a2a46f79c9e7706f8a432afa9f55e943c6c   (310850 bytes)
RESTORED BYTE-EXACT: True
```

Re-verified after all work, against the baseline taken before any edit:

| file | baseline sha256 | now | moved? |
|---|---|---|---|
| `Siegebound/SummonedUnit.cpp` | `f7534183…3c6c` | `f7534183…3c6c` | **no** |
| `Tests/SiegeBrightSunTest.cpp` | `44b70587…8691f` | `44b70587…8691f` | **no** (loop-1 content intact) |
| `Tests/SiegeFogReachSeamTest.cpp` | `25e27b7c…2209` | `25e27b7c…2209` | **no** (loop-1 content intact) |
| `Tests/SiegeFogRetentionWiringTest.cpp` | `ddfe31f7…fff8` | *changed — this is the fix* | **yes, intended** |

`git status --porcelain -- Source/` = the same three `M` rows, nothing added, nothing staged.

---

## L2-§4 — WHAT CHANGED SINCE THE `TASK-1042` VERDICT (for the re-gate)

**Only `SiegeFogRetentionWiringTest.cpp` moved.** `SiegeBrightSunTest.cpp` and
`SiegeFogReachSeamTest.cpp` are **byte-identical to the versions `TASK-1042` passed** (hashes above)
— the re-gate can carry those two findings forward rather than re-deriving them.

| # | change | line |
|---|---|---|
| 1 | **new** `CodeLinesOnly` + the engine-defect note | ~146-217 |
| 2 | **new** `CountCharacter` | ~227-240 |
| 3 | `(c)` block derives every index from `UpdateStateCode`, not `UpdateStateBody` | 786-830 |
| 4 | `CountOccurrencesInCode(Between, TEXT(";")) == 1` → `CountCharacter(Between, TEXT(';')) == 1` | 851 |
| 5 | two assertion messages extended (precondition + the ownership row) | 790-794, 848-850 |

⛔ **Not changed:** `CountOccurrencesInCode` (any copy), any non-`Tests/` file, `CONVENTIONS.md`,
`.claude/agents/qa-reviewer.md`, any board row but my own.

---

## L2-§5 — ⛔ SCRUTINISE HARDEST (loop 2)

1. **`CodeLinesOnly` duplicates the five-clause comment predicate.** Deliberate — the alternative was
   editing the shared helper in 3 of N copies. The duplication is called out in a comment that says
   the two must move together. **Judgement call, declared, not hidden.**
2. **Dropping comment lines JOINS previously separated code.** Confirm that is harmless for these
   four rows: no needle spans a line, and a comment is not a statement, so the `;` count is
   unaffected. (Measured: shipped source still reads `;`=1 on both rows.)
3. **`CountCharacter` is comment-blind by design.** It is only ever fed a slice of the projection.
   Confirm no future caller can feed it raw source.
4. **The `return;` anchor still uses `Find(…, StartPosition)`.** Safe here: `DispatchIndex < Len()`
   always (the dispatch was found and has positive length), so the clamp cannot bite. Verify that
   reasoning — it is the one surviving use of the defective overload in this row.
5. ⚠️ **I could not compile or execute** (build-master's lane). Everything above is source reading
   plus a byte-level simulation. The first real execution is build-master's.

---

## L2-§6 — 🚨 FINDING FOR THE MANAGER (⛔ NOT fixed here — out of scope)

`CountOccurrencesInCode` is copied verbatim into **at least four** test files and **every copy
carries this hang** for any future single-character needle. Today the tree has **zero** such needles
(566 audited; the only one was mine and it is gone), so nothing is currently broken — but the next
author who writes `TEXT(";")`, `TEXT("{")` or `TEXT(",")` gets a ten-hour silent stall with no
diagnostic. A one-line guard (`if (From >= Trimmed.Len()) { break; }`) fixes every copy, but it must
land in **all** copies in **one** commit or it breaks the character-for-character law. **That is a
manager's row, not something to smuggle into this one.** Audit script: `scratchpad/ue_find_proof.py`.
