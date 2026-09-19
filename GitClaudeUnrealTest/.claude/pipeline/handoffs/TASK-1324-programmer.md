# Handoff — TASK-1324 (SUITE-RUNNER-CITATION-ROT)

author: gameplay-programmer · 2026-09-19
subject row: `TASK-1324` · marker `TASK-1324-SUITE-RUNNER-CITATION-ROT`
gate: `TASK-1325` · host: `TASK-1326`
source: `qa/TASK-1323-report.md` §6 WARN-1 + §3(c) NIT-1 (both already shipped at `b98b78a` ⇒ defects, `SC-§50`)

**Files written: exactly two.**
`GitClaudeUnrealTest/Tools/run_suite_bounded.ps1` (header comment block only) · this handoff.

⛔ **I did NOT flip my own status line.** `TASKBOARD.md` is fenced on this row's `names` block
(*"NEVER … `TASKBOARD.md`"*), and `TASK-1325` (28) owns the flip (*"then flip `TASK-1324` and this
row"*). The orchestrator has been told; the row is **ready-for-qa in fact**, and the flip is the
gate's.

---

## 0. THE HEADLINE, FIRST, BECAUSE IT IS THE GATE'S CHECK ONE

🚨 **I used the SUBSTRING `THE FILE EXISTS`. I did NOT write a fourth line number — not `:5122`,
not `:5140`, not any other address.**

And the reason stopped being theoretical while I worked:

| reader | instant | where `THE FILE EXISTS` sat |
|---|---|---|
| the board / the manager | 2026-09-19 | `:5122-5123` |
| `TASK-1323` (the gate) | 2026-09-19, later | `:5122` |
| **me, now** | 2026-09-19, later still | **`:5140`** |

⇒ ⚖️ ***The law moved AGAIN, an eighteen-line displacement, between the gate that measured it this
morning and the fix that removed it this afternoon. `:5122` would have been the fourth wrong number
in this one chain, and it would have been wrong the same day it was written.*** That measurement is
recorded **here**, in a dated handoff — it is evidence. It is **not** in the script, where it would
be a citation, and a citation is the thing that rots.

---

## 1. ACCEPTANCE (5) — EVERY ITEM, QUOTED

### 1a. BOTH OLD TEXTS AND BOTH NEW TEXTS

**DEFECT (1) — the citation that does not resolve.** `Tools/run_suite_bounded.ps1`, found by
substring, not by address (`SC-§91`; the row's `:171` had not moved, but I re-grepped rather than
trusting it).

**OLD** (4 lines):

```
    DO NOT "TIDY" (c2) AWAY WHILE FIXING (a). Deleting the true sentence next to a false
    one is how this block got wrong in the first place: "the file exists" was read as "the
    tool runs" (CONVENTIONS.md:5093), and the correction that caught that then quietly
    welded "runs" to "kills".
```

**NEW** (10 lines):

```
    DO NOT "TIDY" (c2) AWAY WHILE FIXING (a). Deleting the true sentence next to a false
    one is how this block got wrong in the first place: "the file exists" was read as "the
    tool runs", and the correction that caught that then quietly welded "runs" to "kills".
    That law sentence is cited here the way everything else in this block is cited -- by
    SUBSTRING, never by address. It earned that twice over: the address this line used to
    carry was wrong on arrival (it pointed at a different section entirely), and the true
    address then moved AGAIN, the same day, between the gate that measured it and the fix
    that deleted it. A fourth number would have rotted too.

        grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md    (one hit)
```

⛔ `(one hit)` is a **hit count**, not an address. There is no `:<digits>` construction anywhere in
the new text — see the negative sweep at §1c.

**DEFECT (2) — NIT-1, the count measured wrong.** One word, the number-free form the row prescribes.

**OLD:**

```
             ...-20260917-211827.log ends two lines earlier inside the same orderly
```

**NEW:**

```
             ...-20260917-211827.log ends a few lines earlier inside the same orderly
```

### 1b. MY OWN `THE FILE EXISTS` GREP = 1, WITH A NEGATIVE CONTROL (`SC-§39`)

```
$ grep -c "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
1

$ grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
5140:       - ⚠️🚨⛔⛔ **⛔ AND THE HALF THAT IS ⛔ STILL TRUE, … ⇒ ⚖️ ***⛔ *"THE FILE EXISTS"* IS
            ⛔ NOT *"THE TOOL RUNS"*. ⛔ CORRECTING THE ⛔ FIRST SENTENCE DOES ⛔ NOT DISCHARGE THE
            ⛔ SECOND, AND A ⛔ TIDY-LOOKING CORRECTION IS ⛔ EXACTLY HOW THE SECOND GETS ⛔ ASSUMED.***

$ grep -c "THE FILE EXISTZ" .claude/pipeline/CONVENTIONS.md      # negative control
0    (exit 1)
```

✅ **1 hit, unique, and it is the sentence the block means** — the gloss *"THE FILE EXISTS is NOT
THE TOOL RUNS"* is verbatim the claim the comment attributes to it. The control returns 0, so the
reader is discriminating and a `1` is not a facade (`SC-§96`).

**And I verified the OLD address was genuinely wrong rather than merely stale** —
`.claude/pipeline/CONVENTIONS.md:5093` reads:

```
> ### ⛔⛔ **THE READER WAS BROKEN AND THE ⛔ REPORT SAID *"EVERY SINGLE BINARY IS CORRUPT"*.
  ⛔ THE CORRECT RE-RUN SAID ⛔ `8/8 MATCH`.**
```

⇒ the **broken-LFS-reader / facade-counter** law (`SC-§96`), a different section entirely. Not a
drifted pointer — a pointer that never pointed there. The board's diagnosis is confirmed at my own
instant.

### 1c. `git diff --stat` = 1 FILE · EVERY CHANGED LINE INSIDE THE HEADER BLOCK

```
$ cd C:/GitProjects/GitHub/GitClaudeUnrealTesting      # SC-§102: git root is ONE LEVEL UP
$ git diff --stat -- GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
 GitClaudeUnrealTest/Tools/run_suite_bounded.ps1 | 12 +++++++++---
 1 file changed, 9 insertions(+), 3 deletions(-)

$ git diff --numstat -- GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
9	3	GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
```

**Changed-line ranges in the NEW file** (`git diff -U0 | grep '^@@'`):

```
@@ -128 +128 @@          -> new line 128
@@ -171,2 +171,8 @@      -> new lines 171-178
```

**Header block bounds, re-grepped after the edit:** `<#` at **`:1`** → `#>` at **`:234`**.
⇒ `128` ∈ [1,234] ✅ · `171-178` ∈ [1,234] ✅. **Every changed line is inside the header comment.**

**First executable line, untouched:** `[CmdletBinding(DefaultParameterSetName = 'Run')]` at `:236`,
`param(` at `:237` — both below `:234`, both byte-identical (absent from the diff).

**File size:** 1,815 → **1,821** lines; net **+6** = `9 − 3`. Three independent routes agree
(§1e).

**Negative sweep — no address survived anywhere in the file, not merely in my new text:**

```
$ grep -n "CONVENTIONS\.md:[0-9]" <header lines 1-240>   -> NONE
$ grep -n "5093" Tools/run_suite_bounded.ps1             -> NONE
$ grep -nE "5122|5140" Tools/run_suite_bounded.ps1       -> NONE
$ grep -n "two lines earlier" Tools/run_suite_bounded.ps1 -> NONE
```

### 1d. `Parser::ParseFile` = 0 ERRORS

```
PS> $errs = $null; $toks = $null
PS> [System.Management.Automation.Language.Parser]::ParseFile($f, [ref]$toks, [ref]$errs) | Out-Null
PS> "Parser::ParseFile errors = " + $errs.Count
Parser::ParseFile errors = 0
PS> "tokens = " + $toks.Count
tokens = 9598
```

⛔ **I ran the parse WITHOUT the positive control and I say so rather than leaving it to be
discovered.** The control is `TASK-1326`'s (1), and it is the **corrected** one: it breaks the
**final** `#>`, never the header's own, because breaking the header's re-pairs against the next of
13 block comments and returns **0 errors** — a silent gate (`SHIP-§9`; measured on `TASK-1318`
CONTROL A). ⇒ **My `0` is a parse, not a validated instrument.** Treat it as declared; `TASK-1326`
earns it.

### 1e. `git status` = ONLY THAT FILE + THIS HANDOFF

Tree was **clean** at `f37bd28` when I started (measured, from the git root). After my two edits and
this file, `git status --porcelain` from `C:/GitProjects/GitHub/GitClaudeUnrealTesting` shows
exactly:

```
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1324-programmer.md
```

⛔ Nothing staged. ⛔ No `TASKBOARD.md`, no `CONVENTIONS.md`, no `Saved/**`, no `.uasset`, no file of
`TASK-1313`'s.

---

## 2. FENCES (4) — EACH ONE MEASURED, NOT ASSERTED

| fence | how I measured it | verdict |
|---|---|---|
| **COMMENT-ONLY, ZERO executable lines** | the whole diff is quoted at §1a and §3; both hunks sit at `:128` and `:171-178`, inside `<# :1 … #> :234`; the first executable token is at `:236` | ✅ **0 executable lines** |
| **`<#`/`#>` census 13/13 BOTH sides** | before: 13/13 · after: 13/13, strictly alternating, no nesting (full pairing at §2a) | ✅ |
| never a parameter default or a bound | `$OverallSeconds = 1500` `:249` · `$BootSeconds    = 420` `:250` · `$StallSeconds   = 180` `:251` — `TL-§6`'s proven triple, absent from the diff | ✅ untouched |
| never the `-ExecCmds` composition | `New-EditorCommandLine` `:432`; the two-lane prose at `:14-32` | ✅ untouched |
| never the comma-vs-semicolon separator | `SUITE lane : -ExecCmds="…;Quit"` `:26` · `COMMAND lane : -ExecCmds="…,QUIT_EDITOR"` `:32` | ✅ untouched |
| never `QUIT_EDITOR` | `$script:Terminator = 'QUIT_EDITOR'` `:294` | ✅ untouched |
| never the `#19` orphan-census logic | `$orphans = @(Get-Process …)` `:1772-1775`; nothing above `:234` mentions it | ✅ untouched |
| never `Saved/**` | I **read** four suite logs for the NIT-1 measurement; I wrote none. `git status` shows no `Saved/` path | ✅ absent from the diff |
| never `TASKBOARD.md` / `CONVENTIONS.md` | read only; both clean in `git status` | ✅ |
| **never any file of `TASK-1313`'s** | `SiegePlayerController.cpp` · `handoffs/TASK-1311-programmer.md` · `qa/TASK-1312-report.md` — **zero** in the diff, zero in `git status`. `TASK-1314` is live in that `.cpp` right now and I never opened it | ✅ **0** |

### 2a. THE DELIMITER PAIRING, BOTH SIDES, AND THE +6 SIGNATURE

```
AFTER:  1→234  309→318  368→378  401→414  516→521  582→590  645→649
        672→685  793→804  887→893  947→960  1028→1031  1053→1061
```

13 opens, 13 closes, strictly alternating, no nesting. Against `TASK-1323`'s verified census
(`1→228, 303→312, 362→372, 395→408, 510→515, 576→584, 639→643, 666→679, 787→798, 881→887,
941→954, 1022→1025, 1047→1055`), **all 25 movable delimiters moved by exactly +6** and the one
immovable one (`<#` at `:1`, above my edits) did not move at all.

⇒ ✅ **The body below the header is displaced by precisely the header's growth — the signature of a
comment-only diff**, and it rules out an equal-sized substitution having ridden along under a
matching line count in the delimiter region.

---

## 3. THE ROW'S (3) — **YES, NIT-1's SITE IS INSIDE A PREVIOUSLY FENCED RANGE. I SAY SO, AND I FIXED IT ANYWAY.**

🚨 **Declared, because the row told me this is the check it expected me to trip on.**

`TASK-1317` fenced `:120-125` as **`(c2)`** and made NIT-1's fix *conditional on that range being
edited anyway*. `TASK-1310` then correctly **declined** the fix on that condition (its §10g:
*"It is not — that range is `(c2)`, which the board and the prompt both fence as untouchable"*),
and `TASK-1323` §3(c) upheld the decline (*"not actionable this loop … Owner: whoever next edits
`(c2)`"*).

**At my instant `(c2)` spans `:123-131`, and the NIT-1 sentence is at `:128` — inside it.**
So the answer to (3) is **yes**, on the semantic reading (the `(c2)` paragraph) as well as on the
numeric one.

**I fixed it anyway, under this row's own authority**, exactly as the row instructs: that fence was
scoped to `TASK-1317`'s loop and to a row whose subject was elsewhere; it was never permanent; and
**`TASK-1324` exists precisely to discharge what the fence deferred**. `TASK-1310` named the owner
as *"whoever next edits `(c2)`"* — that is me, and this row is the edit.

**What I did NOT do inside `(c2)`:** nothing else. One word, `two` → `a few`. The three-way split is
intact (`(c1)` at `:111`, `(c2)` at `:123`), the phrase
`NOT MEASURED — NULL WITH NO POSITIVE CONTROL` still occurs **3 times** (`:123`, `:161`, `:162`) with
its U+2014 em dash verbatim, and the law quotes those two verdict lines verbatim — none of the three
is in the diff.

---

## 4. THE NIT-1 MEASUREMENT — WHY NUMBER-FREE IS NOT A DODGE, IT IS THE ONLY CORRECT ANSWER

The row said *"prefer the number-free form"* because *a count about another file's contents rots*.
**I found a stronger reason: the count is not well-defined at all.** Recording it so the next reader
does not re-litigate it.

**(i) Three readers, three numbers, all honest.**

| reader | wrote | counted |
|---|---|---|
| the shipped comment (`b98b78a`) | **two** | unknown — wrong on any reading |
| `TASK-1310` §10g | **four** | lines strictly between `Sending StopTestSession` and the exit marker |
| **me** | **five** | the displacement `exit_line − stop_line` |

Neither `four` nor `five` is a mistake; they are two defensible readings of *"ends N lines earlier"*.
A comment that has to pick one is a comment that will read wrong to half its readers.

**(ii) The reference is a POPULATION, not a file — and the population disagrees with itself.**
I measured the tail shape of every suite log on disk (`exit_line − stop_line`):

```
delta = 5  on 18 runs
delta = 6  on  3 runs   (…-20260914-143207, …-143418, …-20260917-211945)
no marker  on  2 runs   (…-20260917-211827  = the subject, ends AT StopTestSession)
                        (…-20260909-045537  = (c1)'s OVERALL kill, the corpse)
```

⇒ ⚖️ ***"N lines earlier" than WHICH peer? The gap is 5 against most of them and 6 against three.
Any single integer in that sentence is false against part of the very population the sentence is
generalising over.*** `"a few"` is not vaguer than the truth — **it is exactly as precise as the
truth is.**

**(iii) The load-bearing half was always right and is untouched.** The claim the sentence carries is
*orderly shutdown, not a kill*. Measured: `…-211827.log` is 7,401 lines and its last line is

```
7401  [2026.09.18-04.19.19:965][  5]LogAutomationController: Sending StopTestSession to BBBC42C6…
```

— i.e. it stops **inside** the shutdown handshake, after the last test reported `Success`, not
mid-test and not at a kill. The `(c2)` verdict is unchanged and uncontested.

---

## 5. 🚨 DECLARED RESIDUAL — MY OWN +6 MOVED THE HEADER'S LAUNCH/RECEIPT ASIDE. I LEFT IT. HERE IS WHY.

**QA should scrutinise this paragraph first.** It is the one place my fix has a side effect, and I
am declaring it rather than letting it be found (`SC-§121` cl. 5).

Clause (a) of the header, at `:83-90`, carries `TASK-1310`'s launch/receipt citation. Inserting 6
lines at `:171` pushed both sites down by 6:

| site | aside says | **measured now** |
|---|---|---|
| launch — `Start-Process -FilePath $EditorCmd` | `:1680` | **`:1686`** |
| receipt — `Set-Content … ($LogPath + '.cmdline')` | `:1677` | **`:1683`** |

⛔ **I did NOT renumber them, and that is deliberate on three grounds:**

1. **Writing `:1686`/`:1683` is writing an address.** It is the identical species this row exists to
   remove, and `TASK-1325` (1) makes *"an address anywhere in the new text"* a BLOCKER **even if it
   is currently correct**. Fixing rot with fresh rot is not a fix.
2. **The paragraph already defends itself, and it predicted this exact edit.** It leads with the
   greps and demotes the digits to a dated aside — verbatim:
   `FIND BOTH SITES BY GREP, NOT BY LINE NUMBER -- EDITING THIS BLOCK MOVES THEM, which is how the
   previous revision went stale`. A reader who follows the instruction the sentence gives reaches
   the right line. `TASK-1323` §1 PASSED this shape on exactly that reasoning.
3. **Deleting the aside re-opens a discharged check** (`SC-§59` cl. 5) on text outside this row's
   two named defects.

⚠️ **The one genuine weakness, stated plainly:** the aside is stamped *"As this paragraph was last
written (2026-09-19)"* and **my edit is also 2026-09-19**, so the date stamp cannot tell a reader
today that the numbers have since moved. The stamp discriminates at day resolution and two edits
landed on the same day. ⇒ 🙋 **Homed, not orphaned (`SC-§50`): this is a manager/QA call, not
mine — either the aside loses its digits entirely, or the stamp gains a finer resolution, and
either is a decision about text `TASK-1323` already passed.** If QA rules the aside must change, it
is a one-line deletion and I will take it in a fix loop.

---

## 6. WHAT I DID NOT DO

- ⛔ **No compile, no suite run, no git command that writes.** `TASK-1326` is the host and owns all
  three. Every git call I made was `status`/`diff`/`log`, read-only.
- ⛔ **No status flip.** `TASKBOARD.md` is fenced; the flip is `TASK-1325`'s.
- ⛔ **No positive control on the parse.** `TASK-1326` (1), with the corrected final-`#>` control.
- ⛔ **No `CONVENTIONS.md` edit.** The law side is already corrected and is the manager's.
- ⛔ **I never opened `SiegePlayerController.cpp`.** `TASK-1314` is live in it.

---

## 7. FOR THE GATE (`TASK-1325`) — WHERE TO AIM

1. **Check one is §0 + §1b.** Re-run `grep -c "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md`
   **at your own instant** and take your own negative control — do not take my `1` on trust, and
   expect the *line* it sits on to have moved again. **That it moved is the finding, not a problem.**
2. **The substring-vs-number check is a whole-file sweep, not a diff read.** I ran it that way
   (§1c); `grep -nE "CONVENTIONS\.md:[0-9]"` over the header is the cheap form.
3. **§5 is the paragraph I most want a second pair of eyes on** — I made a judgement call to leave a
   now-stale dated aside standing rather than write a correct number into it. If you disagree, it is
   a BLOCKER I will take.
4. **§3 is the declared fence break.** I edited inside `(c2)`. The row authorised it in (3); please
   confirm I did not do anything else in there — the three `NOT MEASURED — NULL…` occurrences and
   the two verbatim-quoted verdict lines are the things to assert as **state** (`SC-§104`), not to
   count.
5. **Accepted-as-declared, name them:** the parse's positive control and the suite run are
   `TASK-1326`'s. My `Parser::ParseFile = 0` is a real measurement of an unvalidated instrument.
