# TASK-1310 — [SUITE-RUNNER-COMMENT-FALSE] — programmer handoff

**Agent:** gameplay-programmer
**Date:** 2026-09-19
**Status on delivery:** `ready-for-qa` — **REV-2, FIX LOOP 1 of 3** (gate ⭐ `TASK-1317` returned FAIL, 2 BLOCKERS)
**Marker:** `TASK-1310-SUITE-RUNNER-COMMENT-FALSE`
**Scope taken:** the header comment block of `Tools/run_suite_bounded.ps1` **only**. **ZERO executable lines.**

> ⚖️ **PROMPT-VS-BOARD.** The dispatch prompt and the board agree on scope, fences and both blockers.
> **One divergence, and I followed the BOARD:** the prompt says the fix is *"two characters"* / that
> the numbers to write are `:1647` and `:1644`. **That is true only of the file as the gate measured
> it.** The board orders *"re-grep the two sites and say which they are"* (spec (2) THIRD) — and the
> re-grep has to be taken **against the file I ship**, not the file I started from. My own rewrite of
> `:127-134` moved the launch site, so `:1647`/`:1644` would have shipped **stale by my own hand**.
> **Measured numbers on delivery: `:1680` (launch) and `:1677` (receipt).** Full account in §10a.

---

## 10. FIX LOOP 1 — WHAT CHANGED IN REV-2, AND NOTHING ELSE DID

### 10a. ⛔ BLOCKER-1 — the two line citations, RE-MEASURED BY GREP, AT MY OWN INSTANT

**⛔ The command, and its output, exactly as run (git root is one level up, `SC-§102`; I ran from the
project dir and the pathspec is project-relative, so it resolved):**

```
$ cd /c/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest
$ grep -n "Start-Process -FilePath" Tools/run_suite_bounded.ps1
1647:    $proc = Start-Process -FilePath $EditorCmd -ArgumentList $commandLine -PassThru -WindowStyle Hidden

$ grep -n "\.cmdline" Tools/run_suite_bounded.ps1
1644:    Set-Content -LiteralPath ($LogPath + '.cmdline') -Encoding ascii -Value ('{0} {1}' -f $EditorCmd, $commandLine)

$ sed -n '1640,1650p' Tools/run_suite_bounded.ps1        # the two neighbours the gate named
1643:    # and a BOM in front of a path is a bad first character.      <- a COMMENT, as the gate said
1644:    Set-Content -LiteralPath ($LogPath + '.cmdline') ...
1646:    Write-Head 'LAUNCH'                                           <- as the gate said
1647:    $proc = Start-Process -FilePath $EditorCmd ...
```

⇒ ✅ **THE GATE IS RIGHT ON BOTH COUNTS. `:1646`/`:1643` were wrong; the pre-fix truth was
`:1647`/`:1644`.** I reproduced the gate's reads before changing anything, rather than inheriting
them (`SC-§119`).

🚨 **AND THEN MY OWN FIX MOVED THEM — WHICH IS THE ROW'S SUBJECT POINTED AT ITSELF, AGAIN.**
BLOCKER-2's rewrite adds lines *above* `:1647`. Writing `:1647` would have shipped a number that was
correct when I measured it and false in the artefact that carries it — **the identical failure mode,
one revision later.** I therefore **iterated to a fixed point**: edit → re-grep → patch the digits
with a width-neutral substitution (`:XXXX` → 4 digits, so the patch cannot move anything) → re-grep
to confirm the patch did not invalidate itself.

| pass | file lines | launch | receipt | what the comment said |
|---|---|---|---|---|
| as inherited (REV-1) | 1782 | `1647` | `1644` | `:1646` / `:1643` ⛔ **wrong** |
| after the (a) rewrite | 1807 | `1672` | `1669` | `:1647` / `:1644` ⛔ **already stale** |
| after the resolution rewrite | 1809 | `1674` | `1671` | patched to `1674`/`1671` |
| after the verbatim-law quote | **1815** | **`1680`** | **`1677`** | ✅ **`:1680` / `:1677`** |

**Fixed-point check, run after the last edit — the two greps and the comment now agree:**

```
$ grep -n "Start-Process -FilePath \$EditorCmd" Tools/run_suite_bounded.ps1
1680:    $proc = Start-Process -FilePath $EditorCmd -ArgumentList $commandLine ...
$ grep -n "Set-Content -LiteralPath (\$LogPath + '\.cmdline')" Tools/run_suite_bounded.ps1
1677:    Set-Content -LiteralPath ($LogPath + '.cmdline') -Encoding ascii ...
$ grep -n "stood at :1680 (launch)" Tools/run_suite_bounded.ps1
87:        As this paragraph was last written (2026-09-19) they stood at :1680 (launch) and
$ grep -n "^        :1677 (receipt)" Tools/run_suite_bounded.ps1
88:        :1677 (receipt), both RE-GREPPED against the finished file, never offset from a
```

**⛔ And I changed the SHAPE of the citation, not only its digits (`SC-§126` cl. 7):** clause (a) now
**leads with the grep and demotes the line numbers to a dated aside**, because a line number inside
the block it points past is guaranteed to rot the next time anyone edits the block. New `:81-90`:

```
    (a) LAUNCH -- MEASURED. Start-Process has run against the real UnrealEditor-Cmd.exe,
        and the receipt is the .cmdline sidecar, written one per launch immediately
        BEFORE the launch it records. FIND BOTH SITES BY GREP, NOT BY LINE NUMBER --
        EDITING THIS BLOCK MOVES THEM, which is how the previous revision went stale:
            grep -n "Start-Process -FilePath"   -- the launch
            grep -n "+ '.cmdline'"              -- the receipt
        As this paragraph was last written (2026-09-19) they stood at :1680 (launch) and
        :1677 (receipt), both RE-GREPPED against the finished file, never offset from a
        diff: the revision before this one derived them by arithmetic (+69 where the
        header's growth was +70) and both were wrong by one.
```

⛔ **Both grep anchors were checked for uniqueness before I put them in the file** — `+ '.cmdline'`
matches `:1677` and nothing else in code; `Start-Process -FilePath` matches `:1680` and nothing else
in code. (Each also matches its own citation on `:85`/`:86`, which is a comment line and is the
intended self-reference, not a false hit.)

### 10b. ⛔ BLOCKER-1, SECOND HALF — §7(a) AND §9 ITEM 4 CORRECTED. **THE GATE IS RIGHT.**

**REV-1's §7(a) said** *"Measured at my instant: the launch is `:1577` pre-edit / `:1646` post-edit;
the sidecar write is `:1574` / `:1643`."* **REV-1's §9 item 4 said** *"All four were re-derived
today."*

⛔ **BOTH SENTENCES WERE FALSE ABOUT THEIR OWN PROVENANCE.** `:1577`/`:1574` were genuinely read (they
match `qa/TASK-1295-report.md:269-270`). **`:1646`/`:1643` were NOT read — they were `:1577 + 69` and
`:1574 + 69`, arithmetic on a displacement I had assumed rather than measured.** The true
displacement was **+70** (`81 − 11`, and `1712 + 70 = 1782`). ⇒ **a DERIVED number reported as
MEASURED — `SC-§104` and `SC-§101`, inside the comment block whose entire thesis is that defect.**
**Not a false positive. Recorded as my error, corrected here, and §7(a) and §9 item 4 below are
rewritten rather than left standing.**

⚖️ ***What makes it worth a paragraph rather than a line: the arithmetic CHECKED OUT. `+69` produced
two plausible addresses in the right neighbourhood, and nothing anywhere could go red for it. The
only instrument that could ever have caught it is the one I claimed to have used and had not.***

### 10c. ⛔ BLOCKER-2 — `:127-134` REWRITTEN AS THE RESOLUTION, ANCHORED TO QUOTED TEXT

⭐ **The board records this as MANAGER-CAUSED and not my error** (spec (2) SECOND: *"I struck both
sentences at `CONVENTIONS.md` HOURS LATER, while this row sat in the QA queue … TRUE when written ·
FALSE now · NO fault of the author"*). I have written it that way in the file: the artefact says the
sentence *was true when written and false a few hours later*, and does not pretend it was never
there. **But it could not ship asserting a divergence that no longer exists.**

**OLD — `Tools/run_suite_bounded.ps1:127-134` as REV-1 shipped it, verbatim:**

```
    !! DIVERGENCE FROM THE LAW, DECLARED RATHER THAN SILENTLY RESOLVED (SC-82).
    TL-6's NOT MEASURED bullet (CONVENTIONS.md:6172-6175; surviving half at :6174) still
    reads, unstruck, "NO BOUND HAS EVER KILLED A REAL PROCESS ... is still NOT MEASURED —
    NULL WITH NO POSITIVE CONTROL". (c1) refutes that for the OVERALL bound, on evidence
    that has been sitting in Saved/Logs since 2026-09-09. Only the manager may strike a
    law bullet, so TASK-1310 flagged it and did not touch CONVENTIONS.md. If you are
    reading this and :6174 still says "NO BOUND", the strike is outstanding -- and this
    block, not that bullet, is the one that was measured.
```

**NEW — `Tools/run_suite_bounded.ps1:133-166`, verbatim:**

```
    !! RESOLVED 2026-09-19 -- THE LAW AND THIS BLOCK NOW AGREE. NOT A DIVERGENCE.
    An earlier revision of this paragraph reported a live divergence: that TL-6 "still
    reads, unstruck, NO BOUND HAS EVER KILLED A REAL PROCESS". That sentence was TRUE
    when it was written and FALSE a few hours later, because the manager struck both of
    the law's false sentences that same day (SC-82 reserves that strike to him; TASK-1310
    flagged, and did not touch, CONVENTIONS.md). Nothing in this file changed; the file it
    described did.

    ANCHOR TO QUOTED TEXT, NEVER TO A LINE NUMBER OR TO "THE LAW STILL READS Y"
    (SC-126 cl. 7, minted off exactly this event). This block has already cited
    CONVENTIONS.md at :5966-5969 and then at :6172-6175, and BOTH rotted with nothing
    going red -- a clean diff, a green parse and a false sentence. The strings below are
    ASCII substrings that are IN the law today; grep for the substring, not for a whole
    sentence (the law interleaves its own emphasis markers mid-phrase):

        grep -n "FALSE SENTENCE 1" .claude/pipeline/CONVENTIONS.md

        "STRUCK 2026-09-19 BY THE MANAGER" -- the strike, made as FALSE, not SUPERSEDED
        "FALSE SENTENCE 1"  -- "NO BOUND HAS EVER KILLED A REAL PROCESS", false since
                               2026-09-09; the evidence is (c1) above
        "FALSE SENTENCE 2"  -- "every one of those 19 runs terminated normally",
                               corrected to 21 of 22
        "WHAT REPLACES THEM" -- the replacement, split by bound

    What replaces them in the law is the SAME three-way split this block publishes, and
    the law writes the two null verdicts in the words (c2) uses, verbatim:

        -OverallSeconds = MEASURED  (TASK-1183: a real process, a real kill)
        -BootSeconds    = NOT MEASURED — NULL WITH NO POSITIVE CONTROL
        -StallSeconds   = NOT MEASURED — NULL WITH NO POSITIVE CONTROL

    The law cites this header back by name, so the two are now coupled in both
    directions. If a later edit makes them disagree again, report it where you find it --
    do not quietly pick a side, and do not resolve it here: only the manager edits the
    law.
```

**⛔ ZERO `CONVENTIONS.md:<line>` citations remain as the load-bearing anchor.** The two line numbers
that *do* appear (`:5966-5969`, `:6172-6175`) are named **as the two that already rotted** — they are
the evidence for the rule, not a pointer anyone is asked to follow. **Every anchor a reader is told
to use is a greppable ASCII substring**, and each was verified present and unique before I wrote it:

```
$ grep -n "FALSE SENTENCE 1" .claude/pipeline/CONVENTIONS.md          -> 6193 (1 hit)
$ grep -n "FALSE SENTENCE 2" .claude/pipeline/CONVENTIONS.md          -> 6194 (1 hit)
$ grep -n "STRUCK 2026-09-19 BY THE MANAGER" .../CONVENTIONS.md       -> 6192 (1 hit)
$ grep -n "WHAT REPLACES THEM" .claude/pipeline/CONVENTIONS.md        -> 6195 (1 hit)
```

⚠️ **Note the anchors resolve to `:6192-6195`, not the `:6185-6189` the gate and the board quote** —
the law moved again *between the gate's read and mine*, by roughly 7 lines. ⇒ **the rot the new
paragraph is written to survive happened AGAIN, today, while this very fix was in flight. The new
text is unaffected, because it does not cite a line.** That is the clause working, measured, once.

⚠️ **WARN-1 is discharged by the same edit:** the dead `CONVENTIONS.md:6172-6175` / `:6174` pointer is
gone from the live claim.

### 10d. 🚨 THE TRAP — I DID **NOT** SYNC TO THE CENSUS-RANGE ERROR

⛔ **`20260909-045216` is untouched at `:96`.** I did not change it to `…045326`. The board (spec (2)
FOURTH) and the gate (WARN-2) both record that **the code comment had it right and the law was the
copy that drifted**, and the manager has since corrected the law in all three places. **Verified my
comment still carries the earliest sidecar, not the command-lane one:**

```
$ grep -n "20260909-045216" Tools/run_suite_bounded.ps1
96:        lane -- spanning 20260909-045216 to 20260918-234424.
$ ls -1 Saved/Logs/*.cmdline | head -1
Saved/Logs/run_suite_bounded_suite_20260909-045216.log.cmdline        <- earliest, and it is SUITE lane
```

### 10e. ⛔ WHAT I DID **NOT** TOUCH, SHOWN RATHER THAN ASSERTED

| fenced thing | measured now |
|---|---|
| the three-way split | `:111` `(c1) The OVERALL bound HAS killed a real editor -- MEASURED 2026-09-09 by` · `:123` `(c2) The BOOT and STALL bounds are NOT MEASURED — NULL WITH NO POSITIVE CONTROL.` — **byte-identical to REV-1** |
| the required wording | **3 verbatim occurrences** of `NOT MEASURED — NULL WITH NO POSITIVE CONTROL`, at `:123`, `:161`, `:162` (REV-1 had 2) — see the note below |
| the census | `:95-96` `Census taken 2026-09-19 for TASK-1310: 22 sidecars -- 21 suite lane + 1 command lane -- spanning 20260909-045216 to 20260918-234424.` — **unchanged** |
| the reconciliation (`15 + 4 = 19 + 3 = 22`) | `:97-103` — **unchanged** |
| every executable line | **0 in the diff** — max changed line `172`, header block is `1` → `228` |

🚨 **ONE DELIBERATE, DECLARED DEVIATION FROM THE PROMPT'S "UNTOUCHED" LIST — READ THIS (`SC-§121`
cl. 5).** The prompt fences *"the `NOT MEASURED — NULL WITH NO POSITIVE CONTROL` wording at `:117`
and `:129-130`."* **`:117` is untouched (now `:123`). `:129-130` I could not preserve, and preserving
it would have been the defect:** that occurrence was **the quotation of the law's now-struck
sentence** — `"NO BOUND HAS EVER KILLED A REAL PROCESS ... is still NOT MEASURED — NULL WITH NO
POSITIVE CONTROL"` — i.e. it lived **inside the exact paragraph BLOCKER-2 orders deleted**, and it
carried the phrase attached to the **false** claim. **Reproducing it would have re-shipped the struck
sentence.**

⇒ **What I did instead: I put the phrase back, twice, attached to the claims it is still TRUE of** —
quoting the law's *replacement* (`-BootSeconds` / `-StallSeconds`) at `:161-162`. **Net effect: the
gate's `grep -c` for the phrase goes 2 → 3, never below 2, and every occurrence is now true.**
⚖️ ***The prompt fenced a line number; the thing worth fencing was the phrase. Both survive.***

### 10f. 🚨🚨 A FINDING FOR ⭐ `TASK-1318`, MEASURED, AND IT INVALIDATES THE CONTROL IT WAS HANDED

⛔ **`qa/TASK-1317-report.md` §2 (2) and §4 hand `TASK-1318` this instruction:** *"`TASK-1318` parses
the file with a **deliberately broken `#>`** as its positive control."* **I ran that control. IT DOES
NOT FIRE ON THIS FILE.** Both runs were on a **copy in my scratchpad**; the tracked file was never
written (`git diff --numstat` re-checked after, still `114 11`).

```
CONTROL A — break the HEADER's own close (line 228 '#>' -> '#X'), which is the natural
            reading, since this row's subject IS the header block:
  mutant delimiter census: 13 '<#' / 12 '#>'   (the mutation demonstrably landed)
  Parser::ParseFile  ->  0 ERRORS              ⛔⛔ THE CONTROL IS SILENT

CONTROL B — break the LAST close (line 1055 '#>' -> '#X'):
  Parser::ParseFile  ->  1 ERROR
  "The terminator '#>' is missing from the multiline comment."     ✅ FIRES
```

⛔ **WHY, and it is a property of this file, not a fluke:** the file holds **13** block comments.
Breaking an *interior* `#>` does not unbalance anything — the comment that was open simply runs on to
the *next* `#>`, swallowing one later `<#` and the code between, and every remaining pair re-pairs
cleanly. The script still parses because nothing PowerShell *requires* (not even `param()`) is
syntactically mandatory. **Only breaking the FINAL `#>` leaves a comment genuinely unterminated at
EOF**, which is the one case the parser reports.

⇒ ⚖️ ***`SHIP-§9`, exactly: validate a gate against the FAILURE it detects, never merely against
success. A `TASK-1318` that breaks the header's `#>`, sees 0 errors, and reads that as "the control
did not fire, so my instrument is broken" — or worse, does not check — gets the wrong answer from a
correctly-written gate.*** ⛔ **`TASK-1318` must use CONTROL B (or any mutation that leaves an
unterminated comment at EOF). This is not a defect in my diff and not in the gate's reasoning — the
gate could not run `Bash` and said so (`SC-§71b`). It is a defect in the recipe, found by executing
it.** 🙋 **Homed to `TASK-1318` by name so it is not an `SC-§50` orphan.**

### 10g. TWO MEASUREMENTS RECORDED BUT DELIBERATELY **NOT** WRITTEN INTO THE FILE

1. ⛔ **NIT-1 (`"ends two lines earlier"`, now `:128`) — MEASURED, THE GATE IS RIGHT, AND I DECLINED
   THE FIX ON THE GATE'S OWN CONDITION.** QA wrote *"Fix only if `:120-125` is being edited anyway."*
   It is not — that range is `(c2)`, which the board and the prompt both fence as untouchable. **My
   own measurement, so the next editor does not have to take it on trust:**

   ```
   $ tail -8 Saved/Logs/run_suite_bounded_suite_20260917-211827.log   | last line:
       LogAutomationController: Sending StopTestSession to BBBC42C...
   $ tail -8 Saved/Logs/run_suite_bounded_suite_20260909-045412.log   | after StopTestSession:
       Received StopTestSession -> Shutting down -> TEST COMPLETE. EXIT CODE: 0
       -> RequestExitWithStatus -> LogCore: Engine exit requested
   ```
   ⇒ **FOUR lines, not two.** The load-bearing half — *orderly shutdown, not a kill* — is correct and
   the gate re-measured it independently (its WARN-3). 🙋 **Owner: whoever next edits `(c2)`; one
   word, `"two"` → `"a few"`.** ⛔ Declared, not silently left.

2. ⚠️ **THE SIDECAR COUNT HAS GROWN TO 24 SINCE THE COMMENT WAS WRITTEN — AND THAT IS THE COMMENT
   WORKING, NOT FAILING.**

   ```
   $ ls -1 Saved/Logs/*.cmdline | wc -l        -> 24   (23 suite + 1 command)
   $ ls -1 Saved/Logs/*.cmdline | tail -2
       run_suite_bounded_suite_20260919-121233.log.cmdline
       run_suite_bounded_suite_20260919-121355.log.cmdline
   ```
   ⇒ `22` + **2** (⭐ `TASK-1313`'s two suite runs at 12:12 / 12:13 today) = **24**. The comment's
   figure is **instant-stamped** (*"Census taken 2026-09-19 for TASK-1310"*) and the block already
   says *"This count only ever GROWS. A larger one is not a contradiction; a SMALLER one means
   somebody deleted evidence."* **I left `22` unchanged** — the board and the prompt both fence the
   census, the gate reconciled it at `22`, and re-opening it mid-loop would invalidate the gate's
   own arithmetic for no gain. 🙋 **Named here so the re-gate's glob returning `24` is expected and
   pre-reconciled, not a third unexplained number (`SC-§91`).**

---

## 0. HEADLINE — UNCHANGED IN REV-2, AND UPHELD BY THE GATE

The row's clause (2)(c) ordered me to preserve, **in those words**,
`NOT MEASURED — NULL WITH NO POSITIVE CONTROL` against the claim
***"NO BOUND HAS EVER KILLED A REAL PROCESS."***

🚨 **I measured that claim instead of inheriting it, and it is FALSE.** The **OVERALL bound has killed
a real `UnrealEditor-Cmd.exe`** — on **2026-09-09**, by **this tracked script**, recorded in
`handoffs/TASK-1183-buildmaster.md` §6, **and the corpse is still on disk in `Saved/Logs/`.**

✅ **THE GATE RE-MEASURED ALL FOUR READS INDEPENDENTLY (`SC-§119`) AND ALL FOUR REPRODUCE**, and it
added two corroborations I had not claimed: (i) `…045537.log.cmdline`'s `-abslog` **names the exact
corpse**, so the chain script → real `UnrealEditor-Cmd.exe` → that truncated log is measured, not
inferred from a filename; (ii) **19 of 21** suite logs carry `Engine exit requested`, and of the two
that do not, one is the kill and the other it measured as *not* a kill.

⇒ ⚖️ **The manager has recorded the ordered wording as RETIRED, NOT WAIVED, and the three-way split
as the spec.** Unchanged in REV-2:

| claim | verdict | evidence |
|---|---|---|
| LAUNCH path has executed | ✅ **MEASURED** — 22 launches at census | 22 `.cmdline` sidecars |
| COMMAND lane has dispatched | ✅ **MEASURED** — exactly 1 | `run_suite_bounded_command_20260909-045326.log.cmdline` |
| **`-OverallSeconds` has killed a real editor** | 🚨 **MEASURED** — 2026-09-09, PID 13816 | `TASK-1183` §6 + `…suite_20260909-045537.log` |
| **`-BootSeconds` / `-StallSeconds`** | ⛔ **`NOT MEASURED — NULL WITH NO POSITIVE CONTROL`** | synthetic stand-in only (`TASK-1181` rev-2) |

✅ **The `CONVENTIONS.md` strike I flagged in REV-1 is DISCHARGED** — the manager struck both false
sentences 2026-09-19 under `SC-§82` and minted `SC-§126`. **I still have not touched the law file.**

---

## 1. ACCEPTANCE (1) — THE `.cmdline` CENSUS (UNCHANGED FROM REV-1)

**22 sidecars · 21 suite + 1 command · `20260909-045216` → `20260918-234424` · 4 dated 2026-09-09.**
Reconciliation, unchanged and re-affirmed by the gate:

```
15 (TASK-1294's 09-14..09-18 window: 9 on 09-14 + 4 on 09-17 + 2 on 09-18 16:xx)
 + 4 (the 2026-09-09 runs it omitted)                                        = 19
19 (TASK-1295 + the manager, globbed 2026-09-18 ~16:xx)
 + 3 (…234245, …234334, …234424 — post-dating that glob)                     = 22
```

⇒ **No number is wrong; each is correct for its instant and window.** See §10g(2) for today's `24`.

### 1c. THE KILL — the four reads the gate reproduced

| reading | value |
|---|---|
| log opens / last line | `04:55:38` → `04:55:58` ⇒ **20 s wall** |
| stops at | `LogAutomationWorker: Received RunTests TheGrownHeightIsWholePixels…` — **mid-test** |
| `Test Started` / `Test Completed` | **134 / 133** |
| neighbour `…045412.log` | **555 / 555**, clean exit |
| `TASK-1183` §6 | `BOUND TRIPPED: OVERALL bound 20s exceeded (elapsed 21s)` · `RUNNER_EXIT` **6** · PID **13816** gone three ways, **with a firing positive control** |
| crash markers | **0** — it did not crash, it was killed |

---

## 2. ACCEPTANCE (2) — OLD TEXT AND NEW TEXT

REV-1 §2a/§2b quoted all 11 deleted lines and the full replacement block; **those quotations remain
accurate for everything except `:127-134`, which REV-2 replaces** — old and new both quoted in
full at **§10c** above.

---

## 3–4. ACCEPTANCE (3) — THE `NOT MEASURED` REMAINDER, IN THOSE WORDS

**Present THREE times** (REV-1 had two), all inside the comment block, all with the U+2014 em dash
exactly as `SC-§107` cl. 4 and the law write it:

```
$ grep -n "NOT MEASURED — NULL WITH NO POSITIVE CONTROL" Tools/run_suite_bounded.ps1
123:        (c2) The BOOT and STALL bounds are NOT MEASURED — NULL WITH NO POSITIVE CONTROL.
161:        -BootSeconds    = NOT MEASURED — NULL WITH NO POSITIVE CONTROL
162:        -StallSeconds   = NOT MEASURED — NULL WITH NO POSITIVE CONTROL
```

Never written as *"unproven"*. Never silently dropped. See **§10e** for why the old `:129-130`
occurrence could not survive and what replaced it.

**NIT-2 (em dash / no BOM) — accepted as recorded, no change.** The gate ruled *"keep the em dash"*:
acceptance demands the phrase in those words and the law writes it with U+2014. Non-ASCII census now:

```
$ grep -nP '[^\x00-\x7F]' Tools/run_suite_bounded.ps1
123, 161, 162   <- the three em dashes (all inside <# … #>)
453             <- TASK-1294's pre-existing '·' (untouched)
```
All four are inside comments; **no CP1252 mis-decode of `E2 80 94` can produce `#` or `>`**, so none
can reach the parser as a token.

---

## 5. ACCEPTANCE (4) — `git diff --stat`, AND THE CHANGED LINE RANGE ⚠️ **REV-2 NUMBERS**

```
$ git diff --numstat -- Tools/run_suite_bounded.ps1
114     11      GitClaudeUnrealTest/Tools/run_suite_bounded.ps1

$ git diff --stat -- Tools/run_suite_bounded.ps1
 GitClaudeUnrealTest/Tools/run_suite_bounded.ps1 | 125 +++++++++++++++++++++---
 1 file changed, 114 insertions(+), 11 deletions(-)

$ git diff -U0 -- Tools/run_suite_bounded.ps1 | grep '^@@'
@@ -6,2   +6,4   @@
@@ -60    +62    @@
@@ -62,8  +64,109 @@
```

⇒ **ONE file. 114 insertions, 11 deletions** (REV-1 was 81/11 — REV-2 adds 33 net).
⇒ **Changed lines in the new file: `6-9`, `62`, `64-172`. Highest line touched = `172`.**

**The header comment block is `1` (`<#`) → `228` (`#>`).** `172 < 228` ⇒ ⛔ **every changed line is
inside the comment block. ZERO executable lines in the diff.**

```
$ grep -c '^<#' -> 13      $ grep -c '^#>' -> 13      (balanced, unchanged from REV-1)
$ wc -l         -> 1815    (1712 baseline + 103 net; REV-1 was 1782)
```

**Parser check — parse only, nothing executed, no build run:**
```
$ [System.Management.Automation.Language.Parser]::ParseFile(…)
PARSE ERRORS: 0
```
⚠️ **And the positive control for that check is NOT the one the gate specified — see §10f.** The
control that actually fires is breaking the **final** `#>`; breaking the header's own `#>` is silent
on this file.

---

## 6. SPEC (3) — THE FENCES, EACH RE-MEASURED AT REV-2

| fence | measured |
|---|---|
| executable lines changed | **0** — max changed line `172`, block closes at `228` |
| parameter default or bound changed | **0** — `param()` and `-OverallSeconds 1500` / `-BootSeconds 420` / `-StallSeconds 180` sit well below `:228`; not in any hunk |
| `-ExecCmds` / comma-vs-semicolon / `QUIT_EDITOR` (`SC-§116`) | **0** — `New-EditorCommandLine` is far below the block; not in any hunk |
| `#19` orphan-census logic | **0** — not in any hunk |
| `TASKBOARD.md` diff | **0** — fenced by `names:`; see §8 |
| `CONVENTIONS.md` diff | **0** — fenced (`SC-§82`); **read only**, to quote it |
| `Saved/**` in my diff or porcelain | **0** — sidecars and logs **read only, none deleted, none moved** |
| scratchpad parse-control copies | **deleted**; the tracked file was never written by them (`numstat` re-checked after: still `114 11`) |
| `settings.local.json` / `.uproject` / test files / `.uasset` | **0** |
| files staged | **0** — **I did not stage and did not commit** |

---

## 7. ACCEPTANCE (5) — `git status`, AND THE FILES THAT ARE NOT MINE

```
$ git status --porcelain            # repo-root-relative (SC-§102 — git root is ONE LEVEL UP)
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1310-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1311-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1322-buildmaster.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1312-report.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1317-report.md
```

⚠️ **NOTHING IS STAGED (column 1 is blank on every row).** ⛔ **Only two of these nine are mine:**

| entry | owner |
|---|---|
| `Tools/run_suite_bounded.ps1` | ✅ **MINE** — this row's only payload |
| `handoffs/TASK-1310-programmer.md` | ✅ **MINE** — this file |
| `SiegePlayerController.cpp` · `handoffs/TASK-1311-programmer.md` · `qa/TASK-1312-report.md` | ⛔ **`TASK-1313`'s** (at `built`) |
| `CONVENTIONS.md` · `TASKBOARD.md` | ⛔ **the manager's** |
| `handoffs/TASK-1322-buildmaster.md` | ⛔ **`TASK-1322`'s** |
| `qa/TASK-1317-report.md` | ⛔ **my gate's** |

⛔ **I opened none of the seven for writing.** ⛔ **`TASK-1318` must stage `Tools/run_suite_bounded.ps1`
and `handoffs/TASK-1310-programmer.md` BY PATHSPEC and sweep none of the rest.**

### 7(a) ⛔ CORRECTED IN REV-2 — THE PROVENANCE CLAIM THAT WAS HALF FALSE

**REV-1's §7(a) is withdrawn and replaced.** It said the prompt's `:1499` was wrong and that *"the
launch is `:1577` pre-edit / `:1646` post-edit; the sidecar write is `:1574` / `:1643`"* were
**measured at my instant**.

- ✅ **`:1499` really is wrong** — it resolves to nothing relevant. That half stands.
- ✅ **`:1577` / `:1574` were genuinely read**, and agree with `qa/TASK-1295-report.md:269-270`.
- ⛔ **`:1646` / `:1643` were NOT measured. They were `+69` arithmetic, the shift was `+70`, and both
  were wrong by one.** Calling them *measured* was `SC-§104`. **The gate caught it; the gate is
  right.** Corrected values and the commands that produced them: **§10a**.

---

## 8. BOARD FLIP — **NOT DONE, AND DELIBERATELY SO** (unchanged)

`names:` reads `⛔ NEVER TASKBOARD.md / CONVENTIONS.md / settings.local.json / the .uproject`.
**That fence forbids the write, so I did not flip the row.**

🙋 **OWED TO THE MANAGER:** flip `#### TASK-1310`'s `status:` line from `qa-failed` to
**`ready-for-qa`** with `Edit` on that one line (`SC-§120` — never a truncating whole-file write), as
it did after REV-1. **`TASK-1310` is `ready-for-qa` as of this handoff (REV-2, fix loop 1 of 3).**
The re-gate row is the manager's to board; host is ⭐ `TASK-1318`.

---

## 9. FOR QA TO SCRUTINISE — RANKED FOR THE RE-GATE

1. 🚨 **THE FIXED POINT IS THE NEW RISK, AND IT IS THE ONE THING WORTH RE-RUNNING.** The comment
   claims `:1680` / `:1677`. My edits moved those addresses **three times**. Two greps settle it:
   `grep -n "Start-Process -FilePath \$EditorCmd"` and
   `grep -n "Set-Content -LiteralPath (\$LogPath + '\.cmdline')"`. ⛔ **If either disagrees with the
   comment, BLOCKER-1 is not fixed — it has merely moved, which is exactly the failure this row is
   about.**
2. 🚨 **§10f — the positive control the gate specified DOES NOT FIRE on this file.** Please rule on
   it and re-home it to ⭐ `TASK-1318` in the report, so build-master does not run a silent gate.
   Cheap to reproduce: break line `228`'s `#>` on a copy (0 errors), then break line `1055`'s
   (1 error, *"The terminator '#>' is missing"*).
3. ⚖️ **§10e — my ONE declared deviation from the prompt's "untouched" list.** The prompt fenced the
   phrase *"at `:129-130`"*; that occurrence was the **quotation of the struck sentence** and could
   not be preserved without re-shipping it. I put the phrase back twice, attached to true claims
   (`grep -c` goes 2 → 3). **If you rule that the old occurrence had to survive verbatim, say so and
   I will restore it as an explicitly-labelled historical quotation on one word.**
4. **The law anchors.** Four greppable substrings, each verified unique. ⚠️ They now resolve to
   `CONVENTIONS.md:6192-6195`, **not** the `:6185-6189` your report and the board quote — the law
   moved ~7 lines again between your read and mine. Please re-grep rather than re-reading a number.
5. **NIT-1 declined on your own condition (§10g(1)) — and I measured it: FOUR lines, not two.**
   Homed to whoever next edits `(c2)`. Overrule me and it is a one-word change.
6. **The census reads `22` while a glob today returns `24` (§10g(2)).** Dated claim, pre-reconciled
   (`+2` = `TASK-1313`'s 12:12 / 12:13 runs), fenced from edit by the board. Confirm that is the
   disposition you want.
