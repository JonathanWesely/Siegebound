# TASK-1338 — COMMENT-QUANTIFIER-DISCHARGE — programmer handoff

- **row:** `TASK-1338` · marker `TASK-1338-COMMENT-QUANTIFIER-DISCHARGE`
- **agent:** gameplay-programmer, 2026-09-20
- **subject:** `Tools/run_suite_bounded.ps1` — the helper doc comment below the header block, + NIT-3's one conditional
- **gate:** `TASK-1339` · **host:** `TASK-1340` · **no 5a** (PowerShell in a tool) · **no 5b** (no runtime criterion)
- **HEAD at my instant:** `c316929`, `origin/main` == local, tree was clean of my file when I started
- **scratchpad:** `…/scratchpad/t1338-lab/` (+ two loose control copies) — **all deleted**, verified
- **⚖️ FIX LOOP 1** (2026-09-20, after `qa/TASK-1339-report.md` **FAIL**): **ONE prose clause changed, zero
  code changed.** HEAD is now `4e388d6`. See **§0b**, which also **corrects §3's residue-1 misclassification**
  — that misclassification is the *cause* of the blocker, not a detail, so it is retracted in place below.

---

## 0b. 🚨 FIX LOOP 1 — BLOCKER-1 DISCHARGED. The sentence had FOUR conjuncts; I had enumerated THREE.

### 0b.1 ⚖️ THE RETRACTION FIRST, BECAUSE IT IS THE CAUSE

⛔ **§3 residue 1 below is WRONG and I retract it.** It classified the sentence's trailing material as
*"pre-existing, retained verbatim … not in my diff."* **That is false.** I **re-authored** that clause in my
own diff: *"the derived value is printed"* → *"**the boundary itself** is printed"*. Having filed it as
inherited text, my enumeration never asked whether it was **true**, and it stopped at three claims while the
sentence it quoted verbatim four lines above made **four**. ⇒ **The misclassification is not a bookkeeping
slip — it is the whole mechanism of the miss.** The gate found it, and it is right.

🚨 **And the counter-example was already in my own handoff.** §5 case B publishes the new detail as
`FAIL-CLOSED: the parser reported 1 error(s)…` with the boundary **absent** — one screen below §3 asserting
the boundary prints on *every* run. **I measured the refutation and did not connect it to my own sentence.**

### 0b.2 THE METHOD CORRECTION — why a text census could never have reached conjunct D

My colon-digit census was **correct** and the gate confirmed it token-for-token with a different instrument.
It still could not decide conjunct D, because **conjunct D's truth value is owned by code ~490 lines away**,
not by the comment's text. ⇒ **Enumerating a sentence against the comment it lives in is not enough when a
conjunct makes a claim about runtime behaviour.** This time I split the shipped string into its conjuncts
**first**, then chose an instrument **per conjunct**, and a behavioural conjunct got a **code** instrument.

### 0b.3 THE CLAUSE — OLD and NEW, verbatim

**OLD** (the false universal — my own re-authored clause):

```
    CITATION: the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED
    THEM, and the boundary itself is printed in the ledger line on EVERY run, pass or fail,
    which is the one place it cannot go stale.
```

**NEW** (the gate's prescribed wording, adopted verbatim):

```
    CITATION: the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED
    THEM, and the boundary itself is printed in the ledger line on EVERY RUN THAT DERIVES
    ONE, pass or fail, which is the one place it cannot go stale.
```

⭐ **I adopted the prescription verbatim this time, and the contrast with my earlier refusal is deliberate.**
Last loop I declined a prescribed clause **because I enumerated it and measured it false**. This one I
enumerated and measured **true** (§0b.4 conjunct D) — so it is adopted. ⛔ The refusal was never a licence to
prefer my own wording; the instrument decides, not the authorship.

⚠️ **DECLARED: the paragraph tail reflowed 7 lines → 8 lines** (+1). The replacement clause is 17 characters
longer and the paragraph wraps at ≤92 (its own widest line is `:1130` at 92); 7 lines could not hold it
without exceeding that norm. **No wording was added or dropped in the reflow** — proven in §0b.6.
⇒ **the final `^#>` moved a SIXTH time: `1061`→`1065`→`1145`→`1168`→`1175`→`:1176`.**

### 0b.4 🚨 THE FOUR-CONJUNCT ENUMERATION — ONE NAMED INSTRUMENT PER CONJUNCT, ALL FOUR QUOTED

Shipped sentence, split on its own `,`/`:`/`and` seams:

| # | conjunct (verbatim) | kind | **INSTRUMENT** | check, quoted | verdict |
|---|---|---|---|---|---|
| **A** | *"THE DERIVED BOUNDARY IS NOWHERE WRITTEN DOWN HERE"* | **text** — absence of a live value | `grep` over the comment | `awk 'NR>=1089 && NR<=1176' \| grep -nE '238\|1089\|1176\|[0-9]\.\.[0-9]'` ⇒ **no output** | ✅ **TRUE** |
| **B** | *"NO ADDRESS INTO THIS FILE SURVIVES IN THIS COMMENT AS A LIVE CITATION"* | **text** — absence of a live address | complete colon-digit census **+** the non-colon escape route | census of `1089..1176` ⇒ **7 lines / 13 tokens, exactly 2 into this file**; `grep -nEi '\b(lines?\|at\|near\|around)[[:space:]]+[0-9]+'` **file-wide** ⇒ **2 hits, `:121` (a clock, in the header) and `:1649` (the code comment), NEITHER in `1089..1176`** | ✅ **TRUE** |
| **C** | *"the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED THEM"* | **text** — provenance of those 2 | read the 2 tokens' own lines | `:1108` `:1680` and `:1109` `:1677`, **both prefixed `9d80505 (TASK-1327) deleted`** | ✅ **TRUE** by exhaustion |
| **D** | *"the boundary itself is printed in the ledger line on **EVERY RUN THAT DERIVES ONE**, pass or fail"* | 🚨 **BEHAVIOURAL — owned by code, NOT by this comment. A text census cannot decide it.** | **the code path**: all **four** `return` shapes driven, each compared to an **independent oracle** | see §0b.5 — **printed ⟺ boundary derived, on all four returns** | ✅ **TRUE** |

⚖️ **Rider, enumerated rather than filed as residue** (this is the exact habit that caused the blocker):
*"which is the one place it cannot go stale"* — its antecedent is **the ledger line**. Instrument: conjunct A
(the boundary is written **nowhere** in this comment) **+** the fact that the ledger value is **computed at
run time** from the parser, never recorded. ⇒ the ledger line is indeed the only place it appears, and a
computed value cannot go stale. ✅ **TRUE.** ⛔ I am no longer classifying any part of this sentence as
"inherited, therefore not my problem."

### 0b.5 🚨 CONJUNCT D, MEASURED — all four returns vs an INDEPENDENT oracle

⛔ I did **not** test the predicate against itself. The **oracle** is a separate parse I run myself: *did this
file yield a first block-comment token?* The **subject** is the shipped emit block, extracted **by text**
(`$hdr = Get-HeaderRotProneAddress -Path $PSCommandPath`, only `$PSCommandPath` → `$TestPath`) and run against
the shipped helper, also extracted by text. **4 `return @{` shapes confirmed present in the extracted helper.**

| case | return | oracle: boundary derived? | boundary clause PRINTED? | agree | emitted detail |
|---|---|---|---|---|---|
| **A. happy** | `:1239` | ✅ yes — `first block comment at 1..238` | ✅ yes | **OK** | `header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address` |
| **B. parse error** | `:1184-1186` | ❌ no — `parse errors` | ❌ no | **OK** | `FAIL-CLOSED: the parser reported 1 error(s) on this file…` |
| **B′. no leading block comment** | `:1196-1198` | ❌ no — `no leading block comment` | ❌ no | **OK** | `FAIL-CLOSED: no leading block comment found by the parser…` |
| **C. anchor missing** | `:1216-1218` | ✅ yes — `first block comment at 1..238` | ✅ **yes** | **OK** | `header derived as lines 1..238; `**`0 exhibit line(s) excluded`**`…; FAIL-CLOSED: exhibit anchor not found…` |

⇒ **`printed ⟺ derived` on all four returns. Conjunct D is TRUE.**
⭐ **Case C is the witness that earns the words "pass or fail":** it is a **FAIL** that derives a boundary and
**still prints it**. Without C, "pass or fail" would be decoration; with C it is a measured claim.
⭐ **And the two suppressions are exactly the two returns that never reached `$first`/`$last`** — which is
what *"that derives one"* denotes. The old *"EVERY run"* asserted B and B′ too, and they falsify it.
⛔ **`Last -gt 0` is a faithful test for "derived":** the two non-deriving returns set `Last = 0` literally,
and a real token's `EndLineNumber` is 1-based, so it can never be 0. The predicate and the oracle cannot
disagree.
⛔ **The `0 exhibit line(s) excluded` tripwire still prints on path C** — re-confirmed, unchanged.

### 0b.6 THE RE-PARSE, THE CONTROL, AND THE TRAP (`SHIP-§9i`)

| # | run | result |
|---|---|---|
| 1 | `[Parser]::ParseFile` on the **tracked file**, post-fix | ⭐ **0 errors** |
| 2 | **POSITIVE CONTROL** — break the **re-derived** final `^#>` at **`:1176`** | ⭐ **1 error ⇒ THE CONTROL FIRES** |
| 3 | **THE TRAP, re-executed** — break the **header's own** `^#>` at `:238` | ⛔ **0 errors — SILENT CLEAN, reproduced again** |

⛔ The closer was **re-derived** (last `^#>` hit), never carried over from `:1175`. Delimiters at my instant:
`^<#` = **14**, `^#>` = **14**, raw `<#` = **15** (the 15th is the string literal in the helper). File **1996**
lines; doc comment **`1089..1176`**.

### 0b.7 `-SelfTest` SANITY PASS — 55 / 55, and I did NOT read it off the summary

`RUNNER_EXIT = 0`. **`55 yes / 0 no`**, counted by my own reader over the child process's output.
⚠️ **Instrument note, recorded because it nearly produced a false tally:** the output has **several section
layouts**, so a **fixed column** is the wrong instrument — my first two reads returned 35 and then 50 of 55.
Five case names are long enough that the padding collapses to a **single space** before the verdict. The
correct read is a **delimited, case-SENSITIVE** token (`-cmatch`, never `-match`, which hits the lowercase
`no` inside case names). Corrected read: **55 / 55**, matching the runner's own `55 / 55` summary.
⭐ **`SC-126: header grows no rot-prone line address` is PRESENT, LAST and `yes`**, detail **unchanged**:
`header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address`.
✅ **No staging trap:** I ran the **tracked file in place**, where `Tools/SuiteRunnerFixtures` (10 logs; the
11th case is the deliberately absent `__does_not_exist__.log`) already sits beside it ⇒ the `exit 7` fixture
trap cannot arise. ⛔ **ZERO ledger delta: 55 → 55**, no case added, renamed, reordered or dropped.

### 0b.8 FENCES FOR THIS LOOP

| fence | verdict | basis |
|---|---|---|
| 🚨 **header block byte-identical** | ✅ | `sha256` = **`072F55CEB8BC4AD189FCACDBABF5E97C6E912028150C3891E19EF55F08593A2E`**, **14741** bytes through the 238th LF — **equal to the baseline**, measured before *and* after the edit. Boundary **derived**, never the literal 238 |
| 🚨 **this loop changed EXACTLY one paragraph** | ✅ | ⭐ **Reverse-substitution proof:** replacing my 8 new lines with the 7 old ones returns the whole file to `4C668F97A480DF7449AC9C2271BA1FB443CBCC672B3DE0403442E117194D0E8B` — **the exact bytes the gate reviewed**. ⇒ the conditional, the header, the shapes, the exempt walk and all 55 cases are untouched **by construction**, not by assertion |
| ⛔ **NIT-3's conditional NOT touched** | ✅ | `if ($hdr.Last -gt 0)` still at `:1655`; `$hdr.Exempt` still absent from the predicate. Covered by the reverse-substitution proof |
| **zero hunks at or above `:238`** | ✅ | `git diff -U0` hunks: `@@ -1145`, `-1148`, `-1153`, `-1638`, `-1642` ⇒ **lowest is `1145`** |
| no digit / no address added | ✅ | my 8 edited lines (`:1161-1168`) ⇒ `grep -c '[0-9]'` = **0** |
| no line endings changed | ✅ | **0 CR bytes** before and after; file is LF-only in the working tree |
| CLAIMS A/B/C undisturbed | ✅ | §0b.4 — re-measured post-fix, not carried over |
| header / controls / ledger / fences NOT re-run beyond the gate's scope | ✅ | the nine controls, the three (four) NIT-3 paths and the 55-case ledger **stand as the gate measured them** |
| scratchpad deleted | ✅ | `t1338-fix` removed (verified). ⚠️ `t1331-controls.ps1` and other rows' files remain — **pre-existing, not mine** |

**`git diff --numstat`** (from the real git root one level up, `SC-§102`), cumulative vs `4e388d6`:

```
32      16      GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
```

**`git status --porcelain`** at my instant:

```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1343-buildmaster.md
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1338-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1339-report.md
```

⚠️ **`SiegePlayerController.cpp` is NO LONGER dirty — another lane committed it as `4e388d6` while I was out.**
⛔ `TASKBOARD.md` and `handoffs/TASK-1343-buildmaster.md` are **NOT mine** (I am fenced from the board).
⇒ **MY delta is exactly `Tools/run_suite_bounded.ps1` + this handoff.** 🚨 **HOST: commit BY PATHSPEC, never `-a`.**

### 0b.9 ⛔ THE BOARD FLIP IS STILL OWED BY SOMEONE ELSE

My `names:` still fences me from `TASKBOARD.md`, so I **did not** flip `TASK-1338` to `ready-for-qa`.
`SC-§134` cl. 5 charges that to the board, not to me. ⇒ **the gate or the orchestrator owes the flip**, and I
said so in my reply. ⛔ Left stale **loudly and on purpose**, again.

---

## 0. 🚨 FIRST, THE ONE THING THE DISPATCH ASKED ME TO SAY IMMEDIATELY

⛔ **My `names:` block fences me from `TASKBOARD.md`** — it reads *"NEVER a `.cpp` / `.uasset` / `Saved/**` /
`TASKBOARD.md` / `CONVENTIONS.md`"*. The row's `status:` line invokes `SC-§134` (*first observable action
flips the row*), but the `names:` fence is the narrower instrument and **board + law outrank the dispatch**,
so **I did not edit the board.** ⇒ **The flip to `ready-for-qa` is owed by the orchestrator or by the gate**,
exactly as `TASK-1331`/`TASK-1335` were flipped by `TASK-1336`. I reported this in my first line back.
⛔ The row is **NOT** left stale silently — it is left stale **loudly and on purpose**.

## 1. WHAT I EXECUTED vs WHAT I DERIVED (`SC-§91` · `SC-§71b`)

**I have a shell and I used it. Everything below marked MEASURED was executed at my own instant**:
`-SelfTest` twice (before and after), `[Parser]::ParseFile` with a **firing** positive control **and** the
trap control, the **nine** controls of the previous row re-run end to end, three NIT-3 behavioural cases,
`sha256` four times, and `git diff --numstat` / `-U0` / `status` from the **real git root one level up**
(`SC-§102`). ⛔ **Nothing in this handoff is inherited from `TASK-1335`, `TASK-1336` or `TASK-1337`.**

## 2. 🚨 EVERY SUBJECT LOCATED BY QUOTED TEXT — found vs predicted (`SC-§126` cl. 10/11 · `SHIP-§9i`)

⭐ **The expected divergence did NOT occur, and that is itself the measurement.** `qa/TASK-1336-report.md`
cites addresses taken against `8cbd5d1`; `b55f622` committed that same content and **nothing has touched the
file since**, so every predicted address was still live when I arrived. I located each **by text anyway** —
had I trusted the numbers I would have had no way to know that.

| subject | quoted text I searched for | predicted | **FOUND (pre-edit)** | agrees |
|---|---|---|---|---|
| WARN-1 | `NO BOUNDARY, COUNT OR OFFSET IS` | `:1153-1154` | **`:1153-1154`** | ✅ |
| WARN-2 | `UNDER ANY INPUT` | `:1143-1146` | **`:1145`** (claim spans `:1143-1146`) | ✅ |
| NIT-2 (a) | `the next edit that adds or removes` | `:1148-1149` | **`:1148-1149`** | ✅ |
| NIT-2 (b) | `It moves with any edit` | `:1155-1156` | **`:1155-1156`** | ✅ |
| NIT-3 (code) | `FAIL-CLOSED: the parser reported` | `:1177` | **`:1177`** | ✅ |
| NIT-3 (emit) | `header derived as lines {0}..{1}` | `:1642` | **`:1642`** | ✅ |
| NIT-4 | `those two delimiters` | `:1158` | **`:1158`** | ✅ |
| NIT-1 (untouched) | `the block moved +3 under TASK-1327` | `:1147-1148` | **`:1147-1148`** | ✅ |
| NIT-5 (untouched) | `ALL THREE OF ITS ENTRY CONDITIONS` | `:1162` | **`:1162`** | ✅ |

**Geometry I derived, never inherited** (pre-edit → post-edit):

| quantity | pre-edit | **post-edit** |
|---|---|---|
| `^<#` / `^#>` pairs | 14 / 14 (raw `<#` = 15, the 15th is the **string literal** in the helper) | **14 / 14** (raw `<#` = 15) |
| header block | `1..238` | **`1..238` (unchanged)** |
| helper doc comment | `1089..1168` | **`1089..1175`** |
| **final `^#>`** | `:1168` | ⭐ **`:1175` — IT MOVED A FIFTH TIME** (`1061`→`1065`→`1145`→`1168`→**`1175`**) |
| `Get-HeaderRotProneAddress` | `:1169..1232` | **`:1176..1239`** |
| file length | 1980 | **1995** |

## 3. 🚨🚨 THE CLAUSE THAT WAS SET TO CATCH ME — MY OWN SENTENCE, ENUMERATED

### OLD (the false universal), verbatim

```
    language's problem and not this function's guess. NO BOUNDARY, COUNT OR OFFSET IS
    WRITTEN DOWN IN THIS COMMENT: the derived value is printed in the ledger line on EVERY
    run, pass or fail, which is the one place it cannot go stale. It moves with any edit
    that adds or removes a line ABOVE it -- not with every edit to this file, and a number
    recorded here would rot exactly as the hand-deleted citations above it did. A count in
    a comment is an address wearing different clothes. (This paragraph may NOT quote those
    two delimiters literally: a doc comment that spells its own closer CLOSES ITSELF there,
    and the rest of the prose is then parsed as code. Measured while writing this function.)
```

### NEW, verbatim

⛔ **SUPERSEDED BY FIX LOOP 1 — the clause `on EVERY run, pass or fail` below was FALSE and is now
`on EVERY RUN THAT DERIVES ONE, pass or fail`. See §0b.3 for the shipped text. Kept unedited as the record
of what the gate actually blocked.**

```
    language's problem and not this function's guess. THE DERIVED BOUNDARY IS NOWHERE
    WRITTEN DOWN HERE, AND NO ADDRESS INTO THIS FILE SURVIVES IN THIS COMMENT AS A LIVE
    CITATION: the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED
    THEM, and the boundary itself is printed in the ledger line on EVERY run, pass or fail,
    which is the one place it cannot go stale. It moves with any edit that CHANGES THE LINE
    COUNT above it -- not with every edit to this file, and a live address recorded here
    would rot exactly as the hand-deleted citations above it did. A count in a comment is
    an address wearing different clothes. (This paragraph may NOT quote a block comment's
    own opener and closer literally: a doc comment that spells its own closer CLOSES ITSELF
    there, and the rest is then parsed as code. Measured while writing this function.)
```

### ⛔ WHY I DID NOT USE THE BOARD'S OWN SUGGESTED WORDING — and this is a finding

The board (and WARN-1) offered: *"…the only numbers here are **DELTAS ABOUT CLOSED COMMITS**; the derived
value is printed in the ledger line on EVERY RUN."*

🚨 **I enumerated that sentence before adopting it, and its second clause is ITSELF FALSE — it would have
been the third consecutive quantifier to outrun its measurement, written by the row boarded to stop exactly
that.** *"Six consecutive rows"*, *"two dead addresses"*, *"NOTE THE TWO DIGITS"*, *"ALL THREE OF ITS ENTRY
CONDITIONS"* and the regex literal `{3,4}` are **numbers in this comment that are not deltas about closed
commits**. ⇒ **I declined the prescribed wording and report it rather than implementing it (`SC-§100` ·
`SC-§101`).** ⭐ The trap is not the *category* of number — it is **quantifying over numbers at all**. My
replacement therefore quantifies over **addresses into this file**, which is a class I can enumerate to
exhaustion with one grep, and says nothing whatever about "numbers".

### 🚨 THE ENUMERATION ITSELF — every line of the comment `1089..1175`, MY ADDITIONS INCLUDED

My sentence makes exactly **three** checkable claims. Each is enumerated against the **post-edit** comment.

> 🚨 **CORRECTED IN FIX LOOP 1 — THIS LINE IS THE DEFECT.** The sentence quoted four lines above makes
> **FOUR** conjuncts, not three. I enumerated a **model** of my sentence rather than the sentence. The
> missing conjunct D — *"the boundary itself is printed … on EVERY run, pass or fail"* — is a claim about
> **runtime behaviour**, so no census of this comment's text could ever have decided it. **The four-conjunct
> enumeration, with one named instrument per conjunct, is in §0b.4.** The three claims below are still
> correct and the gate confirmed them token-for-token; they were simply not all of them.

**CLAIM A — *"THE DERIVED BOUNDARY IS NOWHERE WRITTEN DOWN HERE."***
Instrument: `awk 'NR>=1089 && NR<=1175' | grep -nE '238|1089|1175|1\.\.'` ⇒ **ABSENT.** The derived values
are `First=1`, `Last=238`, and the comment's own extent `1089..1175`. **None appears.** ✅

**CLAIM B — *"NO ADDRESS INTO THIS FILE SURVIVES IN THIS COMMENT AS A LIVE CITATION."***
**CLAIM C — *"the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED THEM."***

⛔ Instrument: a **complete** colon-digit census of all 87 lines, every token on every line, post-edit —
not a sample:

| # | line | token | points into | live? | verdict |
|---|---|---|---|---|---|
| 1 | `:1105` | `:5093` | `CONVENTIONS.md` | quoted from `f5697f3`'s **deletion** | not this file |
| 2 | `:1106` | `:5966` | `CONVENTIONS.md` | the header exhibit's record | not this file |
| 3 | `:1106` | `:6172` | `CONVENTIONS.md` | the header exhibit's record | not this file |
| 4 | **`:1108`** | **`:1680`** | ⭐ **THIS FILE** | **`9d80505 (TASK-1327) deleted … they stood at`** | **dead — quoted from the diff that deleted it** |
| 5 | **`:1109`** | **`:1677`** | ⭐ **THIS FILE** | **`9d80505 (TASK-1327) deleted`** | **dead — same diff** |
| 6 | `:1126` | `:29` | `ParseExecCommands.cpp` | engine source, declared out of scope | not this file |
| 7 | `:1126` | `:5993` | `EditorServer.cpp` | engine source, declared out of scope | not this file |
| 8 | `:1129` | `:29` | `ParseExecCommands.cpp` | re-quote of the same engine address | not this file |
| 9–13 | `:1131` | `:42` `:55` `:38` `:55` `:58` | nothing | fragments of `'23:42'`, `'04:55:38'`, `'04:55:58'` | clock times, not addresses |

⇒ **13 colon-digit tokens. Exactly 2 point into this file. BOTH are quoted from the diff that deleted them.**
⇒ **CLAIM B and CLAIM C are TRUE by exhaustion, not by assertion.** ✅

⛔ **I also closed the non-colon escape route**, because a census of one spelling is a sample, not an
enumeration: `grep -nEi '\b(lines?|at|near|around)\s+[0-9]+'` over the same 87 lines ⇒ **ABSENT.** There is
no `line 1680` form hiding from the colon census. ✅

⛔ **And my own additions were enumerated separately**, since that is the specific failure this row exists to
end: `awk 'NR>=1143 && NR<=1150' | grep -E '[0-9]'` ⇒ **ZERO digits in the WARN-2 addition**, and the WARN-1
replacement contains **no digit, no ordinal and no address**. **My diff adds nothing the census must forgive.** ✅

### ⚖️ Three residues I declare rather than hide

1. ⛔⛔ **RETRACTED IN FIX LOOP 1 — THIS CLASSIFICATION IS WRONG, AND IT IS THE CAUSE OF BLOCKER-1.**
   ~~*"which is the one place it cannot go stale"* — **pre-existing**, retained verbatim. `one` is idiomatic
   for *only*, and it is a claim about **the ledger line**, not about this file's geometry. Not falsified by
   my sentence, and **not in my diff**.~~
   🚨 **"Not in my diff" is FALSE.** I **re-authored** the clause immediately before it — *"the derived value
   is printed"* → *"**the boundary itself** is printed"*. By filing the whole trailing portion as inherited
   text, I exempted it from enumeration, and that is exactly why my census stopped at three claims and never
   asked whether the boundary really printed on **every** run. It does not: my own NIT-3 conditional
   suppresses it on two of the four returns, and **§5 case B below publishes that counter-example**.
   ⇒ **A clause I re-worded is my clause.** Provenance is not a defence, and "retained verbatim" must be
   *measured against the diff*, not asserted. Corrected text and the full four-conjunct enumeration: **§0b**.
2. *"those two addresses"* (`:1145`) and *"two dead addresses"* (`:1140`) — **counts, and they survive on
   purpose.** My quantifier is about **addresses**, not about counts, so they cannot falsify it. ⭐ **This is
   precisely why the narrowing works where the old sentence did not.**
3. `+3` (`:1152`) — the offset that falsified the old sentence. **It is untouched (NIT-1 is out of scope) and
   it does not falsify the new one**, because an offset is not an address into this file.

## 4. THE OTHER THREE SUBJECTS — OLD and NEW verbatim

### WARN-2, prose half only (spec (2)) — code change DECLINED by the manager and **NOT made**

OLD → `…naming those two addresses UNDER ANY INPUT, / not merely under the happy one.`
NEW:
```
    failure this case can emit is capable of naming those two addresses UNDER ANY INPUT
    THAT DELETES OR REWORDS THE ANCHOR, not merely under the happy one. A REFLOW THAT
    SPLITS THAT PARAGRAPH IS A DIFFERENT INPUT CLASS AND IS NOT COVERED: the downward walk
    stops at the inserted blank, the address line falls outside the exemption and IS
    scanned -- visibly, because the excluded-line count in the same ledger line drops.
    Narrowing the exempt block to the marker line would make that case UNCONDITIONAL, so
    it is ruled out, not overlooked.
```
⚠️ **DECLARED ADDITION beyond the literal prescription, flagged for the gate.** The board prescribed only the
four-word narrowing. I added the two sentences that say **which** input class is uncovered and **why the
obvious cure is wrong**, because a bare narrowing reads as arbitrary and the **next** reader's first instinct
is the narrowing that has now been ruled wrong **three times from two opposite derivations**. ⛔ It is prose,
it adds no number, and it is one contiguous block the gate can strike without touching anything else.
⛔ **I did NOT narrow the block, did NOT touch the exempt walk, did NOT "fix" the fail-open.**

### NIT-2 — both sites, exact form from spec (3)

- site (a) `…and the next edit that adds or removes / a line ABOVE it moves it again.`
  → `…and any edit that CHANGES THE LINE / COUNT above it moves it again.`
- site (b) `It moves with any edit / that adds or removes a line ABOVE it`
  → `It moves with any edit that CHANGES THE LINE / COUNT above it`

### NIT-4 — the dangling antecedent

`(This paragraph may NOT quote those two delimiters literally…)`
→ `(This paragraph may NOT quote a block comment's own opener and closer literally…)`
⭐ Removes the dangling referent **and** a count in the same stroke.

## 5. 🚨 NIT-3 — THE ONE BEHAVIOURAL BYTE, KEPT SEPARABLE AND PROVEN IN THREE DIRECTIONS

```powershell
    $hdr = Get-HeaderRotProneAddress -Path $PSCommandPath
    $hdrLead = @()
    if ($hdr.Last -gt 0) {
        $hdrLead = @(('header derived as lines {0}..{1}; {2} exhibit line(s) excluded by substring, never by address' `
                      -f $hdr.First, $hdr.Last, $hdr.Exempt))
    }
    $hdrDetail = ($hdrLead + $hdr.Why) -join '; '
```

⛔ **The `FAIL-CLOSED` strings are untouched — not one byte** (`SC-§132`). I removed a **false leading
clause**, not a failure. **MEASURED** by extracting the shipped helper by text and driving all three entry
conditions against scratchpad copies:

| entry condition | `Ok/First/Last/Exempt` | OLD detail | **NEW detail** |
|---|---|---|---|
| **A. happy path** | `True / 1 / 238 / 6` | `header derived as lines 1..238; 6 exhibit line(s) excluded…` | **byte-identical — ZERO change** ✅ |
| **B. parse error** (final `^#>` broken) | `False / 0 / 0 / 0` | ⛔ `header derived as lines 0..0; 0 exhibit line(s) excluded…; FAIL-CLOSED: the parser reported 1 error(s)…` | ✅ **`FAIL-CLOSED: the parser reported 1 error(s)…`** — the false clause gone, the failure verbatim |
| **C. anchor missing** (marker reworded in place) | `False / 1 / 238 / 0` | `header derived as lines 1..238; **0 exhibit line(s) excluded**…; FAIL-CLOSED: exhibit anchor not found…` | ✅ **byte-identical — THE TRIPWIRE STILL PRINTS** |

🚨 **Case C is the non-regression that matters and it is why the conditional keys off `$hdr.Last`, NEVER off
`$hdr.Exempt`.** `TASK-1332` ruled the published `0 exhibit line(s) excluded` meter **load-bearing** — it is
the only visible signal that the exemption was defeated. A conditional written against `Exempt -gt 0` would
have read naturally, passed every test in the suite, and **silently deleted that tripwire on the exact path
it exists to serve.** ⛔ Gate: check this keying specifically.

## 6. `SC-§129` — NAMED, AND IT BINDS ME (`SC-§126` cl. 5)

⭐ **`SC-§129` applies because my diff TOUCHES `Tools/run_suite_bounded.ps1` — the trigger is *touches*, never
*changes code*.** This row is **almost entirely prose**, which is exactly the shape a code-keyed trigger
would have missed; all three historic rot events on this file were prose-only.

**PURITY CONFIRMED AT MY OWN INSTANT** (`SC-§91`), not inherited: `Invoke-Main`'s **first statement** is
`if ($SelfTest) { return (Invoke-SelfTest) }` (`:1743`), and a census of `Invoke-SelfTest`'s whole body for
`Start-Process` / `Invoke-Expression` / `[Diagnostics.Process]` / `Start-Job` / `cmd.exe` / `UnrealEditor` /
`.exe` returns **ZERO**. ⛔ No editor process is spawned. *(It does read the fixture corpus from disk, as
declared.)*

### THE FULL LEDGER, BY NAME, POST-EDIT (`SC-§104` — never by total)

`RUNNER_EXIT = 0`. **Re-derived, not inherited from the 55/55 at `b55f622`.** Every case `yes`:

**§1 fixture corpus (11):** `green-suite.log` 0/0 · `red-suite.log` 5/5 · `skipped-suite.log` 0/0 ·
`zero-started.log` 2/2 · `zero-started-filtered.log` 3/3 · `result-absent.log` 4/4 · `count-mismatch.log` 8/8 ·
`w9-cmd-semicolon.log` 2/2 · `green-commands.log` 0/0 · `no-terminator-echo.log` 0/0 · `__does_not_exist__.log` 7/7
**§2 bound arithmetic (5):** overall bound tripped · boot bound: command never echoed · stall bound: log stopped
growing · healthy run is NOT killed · slow boot inside the bound survives
**§3 lane construction (12):** suite value is `'Automation RunTests Siegebound;Quit'` · suite value honours a
sub-group filter · command value is `'A,B,QUIT_EDITOR'` (COMMAS + terminator) · a caller-supplied `'Quit'` is
DROPPED, QUIT_EDITOR appended · caller separator `';'` inside a command is REFUSED (exit 64) · caller separator
`','` inside a command is REFUSED (exit 64) · an empty command list is REFUSED (exit 64) · a command list of
ONLY terminators is REFUSED (exit 64) · W-4: a `-Filter` carrying `';'` is REFUSED (exit 64) · W-4: a `-Filter`
carrying `','` is REFUSED (exit 64) · W-4: a `-Filter` carrying a quote is REFUSED (exit 64) · W-4: an empty
`-Filter` is REFUSED (exit 64)
**§4 expected echoes (5):** suite expects exactly 1 echo, without `";Quit"` · B-1: command lane requires 2 echoes
and NOT the terminator · symmetry: `Get-ExpectedEchoes` refuses `';'` like `New-ExecCmdsValue` · symmetry:
`Get-ExpectedEchoes` refuses `','` like `New-ExecCmdsValue` · symmetry: `Get-ExpectedEchoes` refuses a mangled `-Filter`
**§5 command line (4):** `New-EditorCommandLine` returns `[string]`, not `[string[]]` ·
`-ExecCmds="Automation RunTests Siegebound;Quit"` survives verbatim · paths quoted; `-nullrhi`/`-unattended`
present · command lane emits the COMMA form with QUIT_EDITOR
**§5b Aura exclusion (2):** `-DisablePlugins=Aura` present verbatim in BOTH lanes · the flag guard REFUSES 4
degenerate lines, still accepts the real one
**§6 bound merge (2):** B-2: green log + tripped bound -> exit 6, NOT 0 · no bound tripped -> the log verdict is untouched
**§7 `-LogPath` safety (7):** `'Saved\Logs\run.log'` · `'C:\proj\Saved\Logs\run.log'` ·
`'Content\Meshes\SM_Rock_01.uasset'` · `'Saved\Logs\run.txt'` · `'C:\Windows\Temp\run.log'` ·
`'Saved\..\Content\run.log'` · `''`
**§8 input tolerance (7):** `Get-CmdEchoes` survives BLANK LINES (`[AllowEmptyString]`) · `Get-CmdEchoes` survives
an EMPTY array (`[AllowEmptyCollection]`) · `Test-SuiteCounts` survives BLANK LINES and still counts · zero echoes
is NOT a pass; missing terminator is reported · B-1: terminator echo is corroboration, never a required match ·
W-10: incremental log reader advances and never re-reads · ⭐ **`SC-126: header grows no rot-prone line address`**

⇒ `11+5+12+5+4+2+2+7+7` = **55 / 55**, exit **0**. ⛔ **ZERO ledger delta (55 → 55): no case added, renamed,
reordered or dropped** — I ran `-SelfTest` **before** my first edit and **after** my last, and the two dumps
are name-for-name and order-for-order identical.

🚨 **`SC-126: header grows no rot-prone line address` is PRESENT, LAST, and GREEN**, detail:
`header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address` — **unchanged by
my diff**, which is the required outcome for the happy path.

⛔ **NO suite-count change is claimed.** I added no automation test; the `-SelfTest` ledger (55) and the UE
suite (561) are different instruments and are **not** reconciled against each other.

## 7. THE PARSE CHECK, WITH A CONTROL THAT CAN ACTUALLY FIRE (`SHIP-§9i`)

| # | run | result |
|---|---|---|
| 1 | `[Parser]::ParseFile` on the **tracked file**, post-edit | ⭐ **0 errors** |
| 2 | **POSITIVE CONTROL** — break the **DERIVED** final `^#>` at **`:1175`** | ⭐ **1 error ⇒ THE CONTROL FIRES** |
| 3 | **THE TRAP, executed not argued** — break the **header's own** `^#>` at `:238` | ⛔ **0 errors — SILENT CLEAN** |

⭐ **Run 3 is the one I want the gate to read.** The spec warned that a control built on the header's own
closer is *structurally incapable of failing*; I did not take that on trust, I **reproduced it**. The block
re-pairs against the next `<#` and the parser reports nothing. ⇒ **a control anchored there would have
returned a green that means nothing.** My derived pair count is **`^<#` = 14 / `^#> ` = 14** (raw `<#` = 15 —
the 15th is the **string literal** inside `Get-HeaderRotProneAddress`, not a delimiter).

## 8. 🚨 ALL NINE OF THE PREVIOUS ROW'S CONTROLS RE-RUN — none inherited (inheriting is a gate blocker)

Method, re-derived: a scratchpad copy of **my edited file** plus the fixture corpus (`SuiteRunnerFixtures`
must sit beside the script or the run dies `exit 7` before reaching the case — I hit that and staged them);
**one** shape injected at a time as a new line at **line 24**, inside `.DESCRIPTION` and deliberately
**outside** the exhibit block; `-SelfTest` run **in a child process**; the verdict read **positionally** at
column 63, ⛔ **never by regex** — PowerShell's `-match` is case-insensitive and hits the `no` inside the
case's own name, the instrument failure `TASK-1331` recorded.

| # | control injected | expect | **got** | exit | exempt | verdict |
|---|---|---|---|---|---|---|
| (i) | `CONVENTIONS.md:9999` | NO | **NO** | 5 | 6 | ✅ fired, shape (a) |
| (ii) | `CONVENTIONS.md at :9999` | NO | **NO** | 5 | 6 | ✅ fired — the sweep-defeating spelling |
| (iii) | bare `(:9999)` | NO | **NO** | 5 | 6 | ✅ fired, shape (b) |
| (iv) | `CONVENTIONS.md:29` — **TWO digits** | NO | **NO** | 5 | 6 | ✅ fired — `{3,4}` refuted again |
| (v) | `EngineThing.cpp:1234` | yes | **yes** | 0 | 6 | ✅ did not fire |
| (vi) | clock `07:15:09` | yes | **yes** | 0 | 6 | ✅ did not fire |
| (vii) | ⭐ **NEGATIVE CONTROL — my edited prose, unmodified** | yes | **yes** | 0 | 6 | ✅ **exit 0, `55 / 55`** |
| (viii) | lowercase `conventions.md:9999` | NO | **NO** | 5 | 6 | ✅ `(?i)` on shape (a) holds |
| (ix) | English *"see CONVENTIONS.md: 3 rows apply"* | yes | **yes** | 0 | 6 | ✅ no false red |

⇒ **all nine agree.** ⭐ **`Exempt` reads `6` in every one of the nine** — the injected line grows the header
to `1..239` and shifts the exhibit, and the substring exclusion tracks it without an address. That column is
the evidence the exclusion is still doing its job under mutation, so I published it on every row.
⇒ **(vii) is the half that proves the check is shippable** (`SC-§39`): a check that cannot pass is as broken
as one that cannot fail, and it was re-run **after** my prose landed, not before.

## 9. FENCES — each one measured (spec (7))

| fence | verdict | basis |
|---|---|---|
| 🚨 **header block byte-identical** | ✅ | **`sha256` = `072F55CEB8BC4AD189FCACDBABF5E97C6E912028150C3891E19EF55F08593A2E`, 14741 bytes through the 238th LF — equal to the `b55f622` baseline.** Boundary **derived** (`<#` → matching `#>`), never taken as the literal 238 |
| 🚨 corroborated independently of the hash | ✅ | `git diff -U0` ⇒ **lowest hunk `@@ -1145`** ⇒ **ZERO hunks at or above `:238`** — the one instrument that catches an equal-line-count rewrite |
| ZERO executable change except NIT-3's conditional | ✅ | only 2 of 5 hunks are in code, both in the `$hdrDetail` composition; §5 proves the happy path is byte-identical |
| no parameter default / bound changed | ✅ | `:253-255` and `:298` are above `:1089`; zero hunks there |
| no `-ExecCmds` composition / separator / `QUIT_EDITOR` (`SC-§116`) | ✅ | §3 and §5 of the ledger present **by name and in order**; zero hunks in that region |
| **no existing self-test case edited, renamed or reordered** | ✅ | §6's before/after ledger dumps are identical name-for-name **and** order-for-order |
| **no line at or above the header's final `#>`** | ✅ | lowest hunk `@@ -1145` |
| the exhibit untouched | ✅ | anchor still derived at `:144`, `Exempt = 6` on the happy path and on all nine controls |
| `Saved/**` · `TASKBOARD.md` · `CONVENTIONS.md` absent | ✅ | `git status` below |
| **ZERO `Source/` bytes, ZERO `.uasset`** | ✅ | see the ⚠️ below — the dirt is **`TASK-1341`'s** |
| no compile, no UE suite, no git write, no commit, no 5b | ✅ | `TASK-1340` is the host |
| scratchpad deleted | ✅ | `t1338-lab` removed (verified `True`); the two loose control copies deleted. ⚠️ A **pre-existing** `t1331-controls.ps1` from an earlier row remains — **not mine, not deleted by me** |
| tracked `sha256` identical before/after all control work | ✅ | `4C668F97A480DF7449AC9C2271BA1FB443CBCC672B3DE0403442E117194D0E8B` before and after **both** batteries |

**`git diff --numstat`** (⛔ **not** `--stat`, whose number is a changed-line total — `SC-§128`), run from the
real git root one level up (`SC-§102`):

```
31      16      GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
```

**`git status --porcelain`** (whole repo, at my instant):

```
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1341-programmer.md
```

⚠️ **The tree was CLEAN when I started and two of those three rows are NOT mine.**
`SiegePlayerController.cpp` and `handoffs/TASK-1341-programmer.md` are **`TASK-1341`'s work, landed in
parallel while I was running** — exactly as my dispatch and `parallel-safe` predicted (disjoint files:
`Tools/**` vs `Source/**`). ⛔ **I touched neither and I staged nothing.** 🚨 **HOST: commit BY PATHSPEC,
never `-a`** — a `-a` here sweeps a live lane into a gated diff (`SC-§102` · `SC-§106`).
⇒ **MY delta is exactly `Tools/run_suite_bounded.ps1` + this handoff.**

The `LF will be replaced by CRLF` warning on this file is **pre-existing** (`TASK-1324`/`1327`/`1331`/`1335`
all recorded it), not from my diff.

## 10. THE THREE OUT-OF-SCOPE NITs — NAMED AS DELIBERATELY UNTOUCHED (spec (6))

⛔ A skipped item that is not declared reads as a forgotten one.

- **NIT-1** — `the block moved +3 under TASK-1327 (measured across that commit, not quoted from a ledger)` at
  **`:1152-1153`**. **UNTOUCHED.** It is a **delta about a closed commit**, the one number in this comment
  that *should* be there, and it cannot rot the way a position can.
- **NIT-5** — `IT FAILS CLOSED AT ALL THREE OF ITS ENTRY CONDITIONS` at **`:1169`**. **UNTOUCHED.** It ships
  with its own denominator (the same sentence enumerates them: *the parse, the header boundary and the
  exhibit anchor*) ⇒ self-checking, not an address. I re-verified all three guards exist, located by their
  `FAIL-CLOSED:` text: **`:1185`** (parse), **`:1197`** (no leading block comment), **`:1217`** (exhibit
  anchor) post-edit.
- **NIT-6** — the coverage trade documented at **`:1118-1122`** and the shape-(a) `Rx`. **UNTOUCHED**, and
  zero hunks land in that region.

⛔ **WARN-2's code change: NOT MADE.** The block was not narrowed, the exempt walk was not touched, the
fail-open was not "fixed". Three rulings say no; my prose now records *why*, so the next reader does not
rediscover the temptation.

## 11. WHAT THE GATE (`TASK-1339`) SHOULD SCRUTINISE HARDEST

0. 🚨 **FIX LOOP 1 SUPERSEDES THE ORDERING BELOW. Scrutinise §0b.4 and §0b.5 FIRST** — the four-conjunct
   enumeration and the measurement of conjunct D against all four `return` shapes. **Conjunct D is the one
   that failed, and it is the only one a text census cannot decide.** Drive the four returns yourself; my
   oracle-vs-predicate table is hearsay to you. Items 1–6 below are the original loop's and still stand.
1. ⭐ **My own sentence's enumeration (§3) — that is this row's entire reason for existing.** Re-run the
   colon-digit census yourself; **do not accept my table**. The claim stands or falls on there being exactly
   **two** addresses into this file and both being quoted-as-deleted.
2. 🚨 **My refusal of the board's prescribed wording (§3).** I believe the offered clause *"the only numbers
   here are deltas about closed commits"* is **false** and would have been the third consecutive miss. If you
   disagree, that is a finding against me, not a nit — but check the five counter-instances first.
3. ⚠️ **My declared addition to the WARN-2 paragraph (§4)** — two sentences beyond the literal prescription.
   Rule on it; it is strikeable as one contiguous block.
4. 🚨 **NIT-3's keying (§5)** — `$hdr.Last -gt 0`, **never** `$hdr.Exempt`. Case C is the regression that the
   natural spelling would have caused.
5. ⚠️ **`:1176-1239`, `:1175`, `:1144-1167` are MY addresses, measured at MY instant, and they are hearsay to
   you** (`SC-§126` cl. 11). The final `^#>` has now moved **five** times. **Re-derive everything.**
6. ⛔ **The board flip is owed by someone else** (§0) — my `names:` fences me from `TASKBOARD.md`.

## 12. NOTES FOR THE HOST (`TASK-1340`)

- ⛔ **Commit BY PATHSPEC. `TASK-1341`'s `Source/` work is live in the tree right now** (§9). Never `-a`.
- ⛔ **Git root is ONE LEVEL UP** — a mis-anchored pathspec answers with **SILENCE**, not an error (`SC-§102`).
- ⭐ **Your parse control: the final `^#>` is at `:1176` AT MY INSTANT and it has now moved SIX times**
  (`1061`→`1065`→`1145`→`1168`→`1175`→**`1176`**) — the sixth move is fix loop 1's +1-line reflow. **Re-derive
  it; it is hearsay to you.** ⛔ Never anchor on the header's own closer — I **executed** that trap twice and
  it returns a **silent 0** (§7 run 3 · §0b.6 run 3).
- ✅ Expect `-SelfTest` **55 / 55**, exit 0, `SC-126: header grows no rot-prone line address` **last and
  green**, detail `header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address`.
  ⛔ If any name is missing or any detail differs, that is an equal-line-count rewrite showing itself — **stop
  and report.**
- ✅ Header `sha256` to re-assert: **`072F55CEB8BC4AD189FCACDBABF5E97C6E912028150C3891E19EF55F08593A2E`**
  (14741 bytes through the 238th LF). Derive the boundary, don't trust the 238.
- ✅ Expect **no** UE suite-count delta. ⛔ Do not reconcile the `-SelfTest` ledger (55) against the suite (561).
- ⛔ **Three flips are owed** (`SC-§103`): `TASK-1338`, `TASK-1339`, `TASK-1340` — and `TASK-1338`'s is owed
  because my `names:` fenced me out of the board, not because I forgot it.
