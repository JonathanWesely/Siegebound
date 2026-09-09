# TASK-1181 — `Tools/run_suite_bounded.ps1`, the tracked executor

**Author:** gameplay-programmer · 2026-09-09
**Law:** `TL-§6` · `SC-§116` · `SC-§95` cl. 1 & cl. 3 · `SC-§87` · `SHIP-§9` · `SC-§39`
**Status:** `ready-for-qa` → gate `TASK-1182`, host `TASK-1183`

> ⛔ **NO ENGINE WAS LAUNCHED. NO PROCESS WAS STARTED AT ALL.** Every result below comes
> from file parsing and arithmetic. The editor was never opened, no MCP call was made,
> nothing was compiled, nothing was committed, `L_Arena.umap` was never touched.

---

## 0. FILES

| file | state |
|---|---|
| `Tools/run_suite_bounded.ps1` | **NEW** — the executor |
| `Tools/SuiteRunnerFixtures/*.log` | **NEW** — 8 fixtures, the failure-side corpus |
| `.claude/pipeline/handoffs/TASK-1181-programmer.md` | this note |
| `.claude/pipeline/TASKBOARD.md` | my `- status:` line only |

⛔ **Zero `Source/**`, zero `Content/**`, zero `Config/**`, zero `.uasset`,
`Tools/Packaging/ship.ps1` untouched, `CONVENTIONS.md` untouched.** `TASK-1178`'s lane
(`Source/GitClaudeUnrealTest/Siegebound/**`) was never entered — `git status` confirms its
two modified files are its own and not mine.

---

## 1. WHAT THE RUNNER DOES

One script, two lanes, five verdict guards, three bounds, and a `-SelfTest` that drives
every guard against the failure it exists to catch.

```
-SelfTest              run the fixture corpus + bound arithmetic. Launches nothing.
-DryRun                print the exact resolved command line. Launches nothing.
-VerifyLog <path>      judge an existing log. Launches nothing.
(default)              launch the editor bounded, then judge the log it produced.
-Porcelain             also emit RUNNER_*=<value> lines for a calling agent.
```

Bounds are parameters with `TL-§6`'s proven defaults, not re-derived:
`-OverallSeconds 1500` · `-BootSeconds 420` · `-StallSeconds 180`.

### Exit codes — every one of them exists to say NO

| code | meaning |
|---|---|
| `0` | OK — suite green / all commands dispatched |
| `2` | **DISPATCH_ECHO_FAILED** — the invocation was mangled; nothing ran |
| `3` | **ZERO_STARTED** — echo fine, `N == 0` (`SC-§95` cl. 1) |
| `4` | **RESULT_ABSENT** — tests started, no `Result={}` readable. Unreadable instrument, *not* a green suite |
| `5` | **TESTS_FAILED** — a genuinely red suite |
| `6` | **BOUND_EXCEEDED** — overall / boot / stall (`SC-§87`) |
| `7` | **LOG_UNREADABLE** — absent or empty log (`SC-§114`: a null is not a result) |
| `8` | **COUNT_MISMATCH** — `Started != Completed != Success + Fail` |
| `64` | USAGE |

⛔ **`$LASTEXITCODE` is recorded and never consulted for the verdict.** The script prints
it as `process exit code (RECORDED, NOT TRUSTED)`. `Build.bat` returns 0 on a failed build
and the editor returns 0 on a run that executed nothing; the log is the only witness.

---

## 2. THE TWO LANES, AND WHY THEIR SEPARATORS DIFFER

**This is the whole reason the tool exists.** Both forms are now emitted by code, so no row
re-types either one again.

### SUITE lane (default) — the `;` is CORRECT

```
-ExecCmds="Automation RunTests Siegebound;Quit"
```

The entire string is **ONE console command**, whose name is `Automation`. The Automation
handler splits its **own** argument list on `;`. That `;` is therefore **never seen by the
`-ExecCmds` parser at all**.

### COMMAND lane — commas, and `QUIT_EDITOR`

```
-ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR"
```

Here the `-ExecCmds` parser itself does the splitting, and it splits **only on comma**
(`ParseExecCommands.cpp:29`, `else if (CurrentChar == ',' && !bInQuotes)`). `QUIT_EDITOR`
is **appended by the script, never by the caller** — `Quit` does not quit an editor
commandlet (`EditorServer.cpp:5993`), and that defect hung a process for ten minutes.

> ⭐ **The rule is NOT "commas". The rule is "ASK WHOSE PARSER SEES THE SEPARATOR".**
> That sentence is in the script's header block, not only in this note.

### ⛔ The ~20 existing `;Quit` sites were LEFT ALONE — deliberately

Measured: **51 occurrences across 32 files** under `.claude/pipeline/` carry
`Automation RunTests Siegebound;Quit`. **I changed none of them.** They are correct.
`SC-§116` cl. 2 warns that a reader who learns "commas, not semicolons" will sweep them and
break the one lane that works; the script actively defends against that by refusing a
caller-supplied embedded separator:

```
> .\Tools\run_suite_bounded.ps1 -Mode Command -Commands "Siege.Fog.Raise;Siege.Fog.Status"
USAGE: Command 'Siege.Fog.Raise;Siege.Fog.Status' contains a separator. Pass commands as
separate array elements; this script joins them. (SC-116)      EXIT=64
```

A caller who passes `Quit` or `Exit` in the command list has it **dropped** and
`QUIT_EDITOR` appended instead — verified by `-DryRun`.

### The third silent-null: ONE PowerShell argument

`-ExecCmds` is built into a **single verbatim command-line string** handed to
`Start-Process -ArgumentList` as one string. An *array* `-ArgumentList` lets PowerShell
re-quote elements, which is the exact mechanism that produced `Cmd: Automation` and a
zero-started, zero-failed, perfectly green-looking run (`SC-§95` cl. 1).

---

## 3. THE BUILT-IN NO-OP DETECTOR (guard b), AND A MEASUREMENT THAT CHANGES THE RECIPE

The dispatch-echo check runs **before a single result is read** (`SC-§116` cl. 4c).

### 🚨 MEASURED, AND IT CORRECTS A NATURAL READING OF THE LAW

I read the real green log `Saved/Logs/suite-1167rev2.log` (554 tests) rather than assuming
what the echo looks like. Two findings that a "count one `Cmd:` line per command"
implementation would have got wrong:

1. **The working suite echo does NOT contain the `;`.** It reads:
   ```
   [2026.09.09-05.48.32:329][  0]Cmd: Automation RunTests Siegebound
   ```
   The `;Quit` is already consumed by the Automation handler. **`grep -i quit` over that
   entire 554-test log returns ZERO hits** — the word never appears. The suite self-exits
   via `LogAutomationCommandLine: ...Automation Test Queue Empty 554 tests performed.` →
   `TestExit` → `RequestExit`.
   ⇒ ⭐ **This is why "a `Cmd:` echo still carrying the separators" is a safe W9 detector in
   BOTH lanes: the correct suite form does not carry one.**

2. ⛔ **The engine emits its own `Cmd:` lines.** That same log has **THREE** `Cmd:` lines for
   **ONE** command of ours (`MAP LOAD`, `MAP CHECKDEP`, then ours). Other logs add
   `OBJ SAVEPACKAGE` and `HighResShot`.
   ⇒ ⛔ **A guard that counts total `Cmd:` lines is wrong on a good run.** This one matches
   echoes **by content** against an expected list and reports `k / n`.

### What it reports

`DISPATCH ECHO  1 / 1` on the real log; `0 / 3` on the W9 fixture. The `N / M` discipline
applied to dispatch, before any result count is believed.

---

## 4. HOW I DEMONSTRATED IT CAN RETURN NO (`SHIP-§9`)

### 4a. The fixture corpus — 8 logs, real or truncated-real

| fixture | provenance | drives | exit |
|---|---|---|---|
| `green-suite.log` | **truncated-real** — sliced from `Saved/Logs/suite-1167rev2.log`, 20 matched test pairs; the `Found N` and `N tests performed` numbers edited 554→20 for coherence | green path | `0` |
| `red-suite.log` | the above with 2 `Result={Success}` flipped to `Result={Fail}` (`Error:` severity, as the engine writes them) | real failures | `5` |
| `zero-started.log` | real preamble + the `SC-§95` cl. 1 signature recorded verbatim in law (`Cmd: Automation`, `Ready to start automation`, then idle) | PowerShell space split | `2` |
| `zero-started-filtered.log` | real preamble, **correct** echo, `Found 0 automation tests` | `N == 0` in isolation | `3` |
| `result-absent.log` | real Started lines, Completed lines truncated away | unreadable instrument | `4` |
| `count-mismatch.log` | Started 20 / Completed 18 | truncated run | `8` |
| `w9-cmd-semicolon.log` | real preamble + the `SC-§116` cl. 1 echo recorded verbatim (`Cmd: Siege.Fog.Raise;Siege.Fog.Status;Quit`) | the W9 signature | `2` |
| `green-commands.log` | comma lane, 4 echoes incl. `QUIT_EDITOR`, self-exit | command lane green | `0` |

⚠️ **Honest provenance:** `green-suite`/`red-suite`/`result-absent`/`count-mismatch` are
sliced from a **real** 554-test log. The two failure-signature fixtures
(`w9-cmd-semicolon`, `zero-started`) are **real preambles carrying the echo line recorded
verbatim in `SC-§116` cl. 1 and `SC-§95` cl. 1** — the original logs lived in a session
scratchpad that is gone, which is `TL-§6`'s own complaint. I did not invent the signatures;
I copied them from the law that measured them.

⭐ **The corpus has a property worth naming:** `w9-cmd-semicolon`, `zero-started` and
`zero-started-filtered` have **identical, all-zero result counts** — `Started=0 Completed=0
Success=0 Fail=0` — and **three different root causes**. Nothing downstream of the counts
can tell them apart. That is the argument for the echo guard in one line.

### 4b. `-SelfTest` result: **14 / 14**

9 fixture cases + 5 bound-arithmetic cases, each asserted against its **expected exit code
AND its expected diagnosis text**. Launches no engine and no process.

### 4c. The strongest positive control: the REAL, FULL, UNTRUNCATED log

```
> .\Tools\run_suite_bounded.ps1 -VerifyLog Saved\Logs\suite-1167rev2.log
DISPATCH ECHO  1 / 1     N (started) 554     completed 554     N / M 554 / 0
discovered 554           OK  554 / 0 -- green.                 exit 0
```
Same log, a filter that was never run → **exit 2**, `dispatch echo 0 / 1 -- never echoed:
Automation RunTests SiegeboundNotARealGroup`. The guard is sensitive to content, not a
rubber stamp.

### 4d. MUTATION PROOF — 3 mutants, all killed

Run against **scratchpad copies**, never the tracked file.

| mutant | change | self-test |
|---|---|---|
| **M1** | re-introduce the `-f` precedence bug in one verdict message | **13/14, exit 5** |
| **M2** | disable the W9 semicolon detector | **13/14, exit 5** |
| **M3** | stop treating `Started == 0` as a finding | **13/14, exit 5** |

---

## 5. 🚨 THREE DEFECTS MY OWN TESTING FOUND — recorded because each is a lesson

### 5a. The fixtures passed while the real log crashed the script

`Get-Content` on the real log yields **blank lines**, and a `Mandatory [string[]]`
parameter rejects empty-string *elements*. First run against the real log:
`Cannot bind argument to parameter 'Lines' because it is an empty string.` My 8 fixtures
had no blank lines, so **the corpus was 8/8 green while the instrument could not read a
real log at all.** Fixed with `[AllowEmptyString()]`.
⇒ ⭐ **A synthetic corpus tests the logic and not the input.** The real-log positive control
is not optional garnish; it caught what 8 purpose-built fixtures could not.

### 5b. The verdict was right and the sentence was garbage

In PowerShell the format operator **`-f` binds TIGHTER than `+`**. A message written as
```powershell
("dispatch echo {0} / {1} -- never echoed: {2}. Grep the log ... " +
 "and read it yourself ..." -f $matched, $Expected.Count, $missing)
```
formats **only the last fragment**, and printed a literal `{0} / {1} ... {2}` to the reader.
**5 of the 9 verdict messages were affected.** Every exit code was correct throughout — the
self-test passed — and the explanation a human reads was broken.
⇒ ✅ Fixed at all 5 sites, **and the self-test now asserts no verdict message contains an
unsubstituted `{n}`.** Mutant **M1** proves that meta-guard fires.
⇒ ⚖️ **The self-test asserted the CODE and not the MESSAGE, so a broken instrument passed
its own gate.** Same family as `SC-§94`: the value was never wrong, it was never read.

### 5c. ⚠️ A GUARD I BUILT IS REDUNDANT FOR DETECTION — stated plainly, do not let it be read as more than it is

**Mutant M2 originally SURVIVED at 14/14.** Disabling the W9 semicolon detector did **not**
change the W9 fixture's verdict: it still exited `2`, because the generic `k / n` echo
check already fails (`0 / 3` — the mangled echo matches none of the three expected
commands).

Measured side by side on the same fixture:

| | verdict | message |
|---|---|---|
| W9 branch live | exit `2` | `SC-116 W9: a single 'Cmd:' echo carries the separators -- 'Siege.Fog.Raise;Siege.Fog.Status;Quit'. -ExecCmds splits on COMMA, never ';' (ParseExecCommands.cpp:29). Nothing was dispatched.` |
| W9 branch disabled | exit `2` | `dispatch echo 0 / 3 -- never echoed: ...` |

⇒ ⭐ **What the W9 branch uniquely supplies is the NAMED CAUSE, not the detection.** Two
independent guards catch the signature — defence in depth, and I am glad of it — but I will
not claim the W9 branch is what catches W9.
⇒ ✅ **Consequence, and it is the real fix:** a guard whose only contribution is the
diagnosis is **untested** if the self-test asserts only exit codes. Every case now carries a
`ReasonLike` assertion, and **M2 now dies (13/14).**

---

## 6. ⛔ WHAT I COULD NOT VERIFY WITHOUT A LIVE EDITOR — `NOT MEASURED`

🧑 Jonathan may be playtesting; the editor is his. Per spec item (4) I did **not** run the
suite. These are honestly open and `TASK-1183`'s run is the first real outing:

1. ⛔ **The launch path has never executed.** `Start-Process`, the `.cmdline` sidecar, the
   polling loop and `Stop-Process` are **NOT MEASURED**. The *bound arithmetic* is proven
   (5/5 synthetic cases); **the process-kill path is not.**
2. ⛔ **No bound has ever tripped against a real process.** I have never watched this script
   kill a hung editor. `SC-§87` says a hang must announce itself — that claim is currently
   supported by arithmetic only.
3. ⛔ **The COMMAND lane has never been dispatched by this script.** Its emitted string is
   character-identical to `SC-§116` cl. 2's measured-working form (`-DryRun` output in §2),
   but *this script* has not produced that log.
4. ⚠️ **`-abslog` with a quoted path containing spaces is untested by execution.** The repo
   path has no spaces, so the default is safe; a caller passing a spaced `-LogPath` is
   unproven.
5. ⚠️ **Boot detection on a real log has not been observed.** It polls a growing file for
   our `Cmd:` echo; the parsing is proven on completed logs, not on partial ones mid-write.
6. ⚠️ **The suite `Filter` default `Siegebound` yields `N = 554`** at the shipped tree per
   `suite-1167rev2.log` — that is the figure **that log's execution** produced, and I am
   citing it as a log I read, **not as a run I performed** (`SC-§95` cl. 2: the verb matters).

---

## 7. FOR QA (`TASK-1182`) — WHAT TO SCRUTINISE

1. **§5c is the honest answer to your row's question.** "What failure has it actually been
   shown to catch?" — the table in §4a, the mutation proof in §4d, and the admission that
   one guard is redundant for detection.
2. **Check my exit-code ordering choice.** `zero-started.log` (the historical `SC-§95` cl. 1
   event) returns `2` (echo), **not** `3` (count). Deliberate: the echo names the *cause*,
   and it is checked first per `SC-§116` cl. 4c. `zero-started-filtered.log` isolates the
   `3` path. **If you think the historical event should return `3`, say so** — it is a
   defensible call either way and I made it explicitly rather than by accident.
3. **Check the fixture provenance claims in §4a** against `SC-§116` cl. 1 and `SC-§95` cl. 1.
   Two fixtures are reconstructed-from-verbatim-law, and I have labelled them as such.
4. **Check that I did not "fix" a single `;Quit` site.** 51 occurrences / 32 files, untouched.
5. ⚠️ **The `Automation RunTests X` echo-match is anchored on the engine stripping `;Quit`**
   (measured on UE 5.8). I allow the `;Quit` suffix defensively, but if a future engine
   echoes differently, that regex is where it breaks.

---

## 8. ROUTED, NOT FIXED BY ME (`SC-§82` — `CONVENTIONS.md` is the manager's)

1. ⭐ **`TL-§6`'s table says the file "DOES NOT EXIST".** It now does. The manager owns that
   correction; I am routing it, not editing it.
2. ⭐ **`SC-§95` cl. 3 and the `TL-§6` row both carry the "does not exist" measurement**, and
   `SC-§95` cl. 2's propagation duty says a correction in one place does not correct the
   claim. **Three sites** name it: `CONVENTIONS.md` `TL-§6` table row 1, `SC-§95` cl. 3, and
   the `TASKBOARD.md` batch header.
3. ⭐ **A finding for the law, from §3:** `SC-§116` cl. 4(c) says *"grep for `Cmd: ` and
   confirm ONE LINE PER COMMAND."* Measured, that instruction is **wrong as written for the
   suite lane** — the engine emits its own `Cmd:` lines (3 lines, 1 command of ours) and the
   correct suite echo has already had its `;Quit` stripped. The *intent* is right; the
   literal count is not. Suggested wording: *"confirm one echo per command **matched by
   content**; the engine emits its own."* **The manager's call, in the manager's file.**

---
---

# ⭐ REV-2 — QA loop 1/3, answering `qa/TASK-1182-report.md`

**Author:** gameplay-programmer · 2026-09-09 · **rev-1 above is preserved verbatim; nothing in it was edited.**
**Status:** `qa-failed` → **`ready-for-qa`**
**Law:** `TL-§6` · `SC-§116` cl. 4(c) + cl. 6 · `SC-§95` cl. 1-3 · `SC-§87` · `SC-§104` · `SC-§114` · `SHIP-§9` · `SC-§101`

> ⛔ **NO COMPILE. NO GIT. NO EDITOR. NO MCP CALL. `Source/**` NEVER ENTERED.**
> 🧑 Jonathan may be playtesting; the editor is his and was never launched.
> ⭐ **BUT THIS REV WAS *EXECUTED*.** Unlike rev-1's gate, everything below carries a
> measured exit code. Nothing here is arithmetic I did in my head.
> The only processes started were **synthetic stand-in `.cmd` files in the scratchpad**
> that ignore every argument — never `UnrealEditor-Cmd.exe`.

## R2.0 QA's framing, accepted

QA re-derived all 9 fixture verdicts by hand and they agreed with my claimed codes — and
then said the thing that mattered: **agreement of two readings is not an execution.** It
was right, and it was right in a way that cost me: running this rev found **four defects
that neither of us saw by reading**, three of them in code I wrote *for this fix* (R2.7).

---

## R2.1 ⛔ BLOCKER-1 — the terminator exemption. FIXED, and the law is now true.

**`Get-ExpectedEchoes` no longer appends `QUIT_EDITOR` to the REQUIRED list.** The
terminator is reported as `TerminatorEcho`, a corroboration string that **cannot reach
the exit code by any path**: it is set before the guard runs and is never read by the
matching loop.

| | before | now |
|---|---|---|
| required echoes, `-Commands A,B` | `A, B, QUIT_EDITOR` (3) | **`A, B` (2)** |
| terminator missing from the log | **exit 2 — false red** | exit 0, `RUNNER_TERMINATOR_ECHO=... NOT SEEN (corroboration only, NOT a verdict)` |
| suite lane | `QUIT_EDITOR` demanded, never echoes | `n/a (suite lane: ';Quit' is consumed by the Automation handler and never echoes)` |

**Regression fixture, MEASURED:** new `Tools/SuiteRunnerFixtures/no-terminator-echo.log`
— `green-commands.log` with the `Cmd: QUIT_EDITOR` line **deleted**. It is the exact
flush-race QA described.

```
> .\Tools\run_suite_bounded.ps1 -Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status -VerifyLog Tools\SuiteRunnerFixtures\no-terminator-echo.log -Porcelain
RUNNER_EXIT=0
RUNNER_ECHO_MATCHED=3   RUNNER_ECHO_EXPECTED=3
RUNNER_TERMINATOR_ECHO=QUIT_EDITOR NOT SEEN (corroboration only, NOT a verdict -- the terminator's
                       evidence is self-termination + wall clock, SC-116 cl. 4b)
```

The same run against `green-commands.log` (terminator present) is also **exit 0**, with
`QUIT_EDITOR seen`. **The presence or absence of the terminator echo does not move the
verdict — which is the whole of cl. 4(c).**

⇒ ⭐ **`CONVENTIONS.md:4457` is now a true statement about this file.** I took QA's
preferred option (change the code, not the law). Nothing is owed to the manager on B-1.
`QUIT_EDITOR` is still **dispatched** — `-DryRun` proof in R2.6.

**Mutant M11** re-adds the terminator to the required list → **killed, 50/52.**

---

## R2.2 ⚠️⚠️ BLOCKER-2 — a force-killed run reading GREEN. FIXED, AND EXIT 6 IS PROVEN REACHABLE BY EXECUTION.

### The fix is structural, not cosmetic

There is now **exactly ONE `exit` statement in the file**, at the bottom, of the value
`Show-Verdict` printed. `Invoke-Main` is a function that **returns** a code; the bound
path no longer has an `exit` of its own. A new pure function `Merge-BoundIntoVerdict`
moves the bound **into** `Verdict.Code` and demotes the log's opinion to
`LogVerdictCode`. **`RUNNER_EXIT` and the process exit code are the same variable — they
cannot disagree, because there is no second place for them to be computed.**

### ⭐⭐ THE PROOF QA DEMANDED — `RUNNER_EXIT=6` OBSERVED, NOT ASSERTED

I could not launch the editor, so I built what the bound actually needs: **a process that
hangs.** Two scratchpad `.cmd` stand-ins, each ignoring every argument handed to it:

| stand-in | behaviour | reproduces |
|---|---|---|
| `fake_editor_hang_after_green.cmd` | copies the **complete green 20-test log** to the log path, then hangs forever | **`SC-§87` INSTANCE 2 exactly** — every test ran and reported, and the process still will not exit |
| `fake_editor_silent.cmd` | writes **no log at all**, hangs | the command never reaches the engine |

**PROOF A — QA's exact scenario: the suite completed, the shutdown hung, the stall bound killed it.**

```
> .\Tools\run_suite_bounded.ps1 -EditorCmd <fake_editor_hang_after_green.cmd> `
      -LogPath Saved\Logs\bound-proof-green.log -OverallSeconds 300 -BootSeconds 60 -StallSeconds 12 -Porcelain

PID 18620 started at 03:47:11
boot observed at 2s -- the command reached the engine.
BOUND TRIPPED: STALL bound 12s exceeded: the log has not grown for 12s
  PID 18620 confirmed gone (HasExited).
=== VERDICT ===
DISPATCH ECHO           1 / 1
N (started) 20   completed 20   N / M 20 / 0   discovered 20
BOUND TRIPPED           STALL bound 12s exceeded: the log has not grown for 12s
log verdict (info)      exit 0 -- 20 / 0 -- green.
NO                      BOUND EXCEEDED -- STALL bound 12s exceeded ... The process was FORCE-KILLED,
                        so nothing it left behind is a result (SC-114). For information only, the
                        partial log would have judged as exit 0: 20 / 0 -- green.
exit                    6
RUNNER_EXIT=6
RUNNER_BOUND=STALL bound 12s exceeded: the log has not grown for 12s
RUNNER_LOG_VERDICT=0
>>> $LASTEXITCODE = 6
```

⭐ **`RUNNER_EXIT=6`. `$LASTEXITCODE=6`. THEY AGREE.** The green log verdict survives as
`RUNNER_LOG_VERDICT=0`, explicitly labelled *"for information only"*, and the headline word
is `NO`. **Under the rev-1 code this identical run printed `OK 20 / 0 -- green`, `exit 0`,
`RUNNER_EXIT=0` and then exited 6.** That is B-2, reproduced and then killed.

**PROOF B — the boot bound, nothing ever written:**

```
BOUND TRIPPED: BOOT bound 10s exceeded: the command was never echoed (elapsed 10s)
  PID 6204 confirmed gone (HasExited).
exit 6   RUNNER_EXIT=6   RUNNER_LOG_VERDICT=7   >>> $LASTEXITCODE = 6
```

⇒ ⛔ **`RUNNER_EXIT=6` is reachable. Twice, by two different bounds, with two different
log verdicts (0 and 7) underneath it.** The kill path, the polling loop, `Start-Process`,
the `.cmdline` sidecar, boot detection on a growing file and `Stop-Process` **all
executed** — against a stand-in, never against the engine.

**Mutant M12** (the merge keeps the log verdict — the shipped defect) → **killed, 51/52.**

---

## R2.3 ⛔ BLOCKER-3 — a self-test that could not fail on the tool's core behaviour. FIXED.

**`-SelfTest` grew 14 → 52 cases, in 8 sections**, and now drives the **emitting** half,
the **launching** half, the **bound merge**, the **`-LogPath` guard** and **input
tolerance** — not just the judging half.

**(a) The blank-line gap — CLOSED with a real corpus change.** QA measured 0 blank lines
across 8 fixtures. Now: `green-suite.log`, `green-commands.log`, `skipped-suite.log` and
`no-terminator-echo.log` each carry **a mid-log blank line AND a blank line at EOF**, and
§8 additionally calls `Get-CmdEchoes` / `Test-SuiteCounts` / `Test-DispatchEcho`
**directly** with `@('', 'Cmd: X', '', '   ', ...)` and with `@()`. Every fixture case is
also wrapped in `try/catch`, so a parameter-binding failure **reds the case** instead of
aborting the run. **Mutant M8** (delete `[AllowEmptyString()]`) → **killed, 44/52 — 8
cases red.** The regression QA said was absent now exists and is tracked.

**(b) `New-ExecCmdsValue` — now invoked, by string equality.** §3 asserts
`'Automation RunTests Siegebound;Quit'` and `'A,B,QUIT_EDITOR'` **with `-cne`
(case-sensitive)**, plus `'A,Quit'` → `'A,QUIT_EDITOR'`, plus **8 negative cases** that
must throw.

**(c) `New-EditorCommandLine` — now invoked.** §5 asserts the return **`-is [string]`**,
that `-ExecCmds="Automation RunTests Siegebound;Quit"` survives **verbatim as one quoted
token**, that both paths are quoted, and that the command lane emits the comma form.

---

## R2.4 ⚖️ RULING (b) — the mutation set. GROWN FROM 3 TO 16, ALL EXECUTED.

QA's ruling was right and I do not contest it: *"3 mutants, all killed" is true and is not
a coverage claim.* All three lived in `Get-LogVerdict`. Here is the grown set. **Every row
was applied to a scratchpad copy and RUN — the tracked file was never mutated.**

| mutant | QA row | what it breaks | site | observed result |
|---|---|---|---|---|
| **M1** | — | `-f`/`+` precedence in a **verdict** message | `Test-SuiteCounts` | **KILLED** 51/52, exit 5 |
| **M2** | — | W9 semicolon detector disabled | `Test-DispatchEcho` | **KILLED** 51/52, exit 5 |
| **M3** | — | `Started == 0` no longer a finding | `Test-SuiteCounts` | **KILLED** 51/52, exit 5 |
| **M4** | **1** | ⭐ **THE SWEEP: suite `;` to `,`** | `New-ExecCmdsValue` | **KILLED** 49/52, exit 5 |
| **M5** | **2** | ⭐ **terminator `QUIT_EDITOR` to `Quit`** (the 10-min hang) | `$script:Terminator` | **KILLED** 49/52, exit 5 |
| **M6** | **3** | caller-separator rejection deleted | `Get-CleanCommandList` | **KILLED** 48/52, exit 5 |
| **M7** | **4** | ⭐ **`-ArgumentList` back to an ARRAY** (the historical false green) | `New-EditorCommandLine` | **KILLED** 48/52, exit 5 |
| **M8** | **5** | `[AllowEmptyString()]` removed | 3 param sites | **KILLED** 44/52, exit 5 |
| **M9** | **6** | `-f`/`+` precedence in a **bound** message | `Test-BoundExceeded` | **KILLED** 51/52, exit 5 |
| **M10** | **8** | `Result={Skipped}` misdiagnosed as truncation | `Test-SuiteCounts` | **KILLED** 51/52, exit 5 |
| **M11** | **B-1** | terminator put back in the REQUIRED list | `Get-ExpectedEchoes` | **KILLED** 50/52, exit 5 |
| **M12** | **B-2** | bound merge keeps the LOG verdict | `Merge-BoundIntoVerdict` | **KILLED** 51/52, exit 5 |
| **M13** | **W-3** | `-LogPath` guard always says Ok | `Resolve-SafeLogPath` | **KILLED** 51/52, exit 5 |
| **M14** | new | the **judging** half stops validating its input | `Get-ExpectedEchoes` | **KILLED** 51/52, exit 5 |
| **M15** | new | `-f`/`+` precedence in a **throw** message | `Get-CleanCommandList` | **KILLED** 48/52, exit 5 |
| **M16** | **7** | ⭐ **kill-confirmation probe reverted** | the kill path | **KILLED by EXECUTION** — see below |

**16 applied, 16 killed.** QA's rows 1-6 and 8 are covered by `-SelfTest`; **row 7 is
covered by a live process**, because it cannot be covered any other way:

```
M16 MUTANT  (probe = Get-Process -Id)    !! PID 23604 SURVIVED Stop-Process -Force and is STILL RUNNING.
                                         !! AND THE KILL FAILED: PID 23604 survived Stop-Process -Force.
TRACKED     (probe = $proc.HasExited)      PID 13348 confirmed gone (HasExited).
```

Same stand-in, same bound, same 8-second boot limit. **The mutant false-reds on a kill
that succeeded; the tracked file reports the truth.**

### Failure modes that remain uncoverable by `-SelfTest`, and why

1. **The kill actually failing against a *real* engine** — a stand-in `.cmd` dies on
   command. To watch `Stop-Process -Force` genuinely lose, you need a process that
   resists, and UE's shutdown is the only one we have. `TASK-1183` §7 #10.
2. **Real orphan children** (`ShaderCompileWorker`, `CrashReportClient`, `EpicWebHelper`).
   The scan is written and executed; it has never had a real orphan to find. My stand-in
   *did* orphan a `ping` child, which the scan correctly ignores as not-an-engine-process.
3. **Boot detection against a genuinely incremental write.** My stand-in copies its log in
   one `copy` call. The incremental reader is unit-tested for offset advance, no-re-read
   and past-EOF rewind (§8), but a real engine writes in thousands of small appends.
4. **`$proc.ExitCode` from a real editor**, and **`-abslog` with a spaced path**
   (this repo's path has no spaces).

---

## R2.5 THE OTHER FINDINGS

| id | fix | proof |
|---|---|---|
| **W-1** | `.NOTES` now carries a block headed **`!!! SANCTIONED IS NOT EXERCISED`**, naming every unrun element and `TASK-1183` as the first real outing, with *"DELETE THIS BLOCK ONLY THEN"*. `.SYNOPSIS` carries the same caveat. | read the header |
| **W-2** | `Result={...}` tallied **generically**: `resulted`, `skipped`, `notrun`, `other`. Completeness is `Started == Completed == resulted`. New fixture `skipped-suite.log` (18 Success + 2 Skipped) → **exit 0**, reason `20 / 0 -- green. [2 skipped, 0 not-run]`. | M10 |
| **W-3** | New pure `Resolve-SafeLogPath`: the log must resolve **under `<project>\Saved\`** and end **`.log`**, else exit 64 **before** `Remove-Item` is reached. `-LogPath Content\Meshes\SM_Rock_01.uasset` → **exit 64, measured**. | M13 + 7 self-test cases |
| **W-4** | `-Filter` gets the Command lane's guard: `,` `;` `"` and blank → exit 64. `-Filter "Siegebound;Quit,Evil"` → **exit 64, measured**. | 4 self-test cases |
| **W-5** | The 5 bound cases now assert **message text + the `\{\d+\}` placeholder guard**. | M9 |
| **W-6** | Success wording is now `matched by CONTENT; extra engine Cmd: lines ignored, terminator exempt`. | fixture `ReasonLike` |
| **W-7** | `Wait-Process -Timeout 15`, then **`$proc.Refresh(); $proc.HasExited`**, then an orphan scan. A survivor is named in the verdict text. | M16, executed |
| **W-8** | A running `UnrealEditor*` is detected and warned about **loudly, by PID, without killing it**. | code; no editor was running to trigger it |
| **W-9** | Command-lane verdict now ends *"COMMAND LANE: this means DISPATCHED, not SUCCEEDED. Nothing here asserts what the commands did."*, plus `RUNNER_MODE=`. | fixture `ReasonLike` |
| **W-10** | New `Read-LogSince`: byte-offset incremental read, **never consumes a partial trailing line**, rewinds if the file shrinks. Replaces ~210 full re-reads. | 1 self-test case (advance / no-re-read / past-EOF rewind) |
| **W-11** | `Invoke-Main` returns a code; a top-level `try/catch` maps an unhandled throw to **documented exit 70**. | see the honest limit in R2.8 |
| **W-12** | `green-commands.log`'s provenance is now declared: it reproduces `handoffs/TASK-1175b-buildmaster.md:71-74`, a real measured command-lane run. | this note |
| **N-1** | `.cmdline` sidecar is `-Encoding ascii`. **Measured first 3 bytes `67 58 92` (`C:\`), not `239 187 191`.** | measured |
| **N-2** | `$Expected` now carries `[AllowEmptyCollection()][AllowEmptyString()]`, and `Test-DispatchEcho` handles a zero-length expected list with a named reason. | 1 self-test case |
| **N-3** | One ledger entry per case; the `FAILURES (n cases)` count is now the case count. | visible in every mutant run |
| **N-4** | `$script:RepoRoot` → **`$script:ProjectRoot`**, with a comment citing `SC-§102` and *"never hand this to a git pathspec"*. | read the constant block |

**✅ Left exactly as QA confirmed:** the suite `;` at `New-ExecCmdsValue` (comment intact),
exit 64 on a caller-supplied separator, `$LASTEXITCODE` never consulted for any verdict,
and the `ReasonLike` assertions.

---

## R2.6 THE TWO LANES, RE-EMITTED BY THE FIXED CODE (`-DryRun`, exit 0 each)

```
SUITE  : ...UnrealEditor-Cmd.exe "...\GitClaudeUnrealTest.uproject"
         -ExecCmds="Automation RunTests Siegebound;Quit"
         -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog="...\Saved\Logs\...log"

COMMAND: ...UnrealEditor-Cmd.exe "...\GitClaudeUnrealTest.uproject"
         -ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR"
         -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog="...\Saved\Logs\...log"
```

**The suite `;Quit` is intact. The command lane still appends `QUIT_EDITOR`.** Both match
`SC-§116` cl. 2's table character-for-character.

**Real-log positive control, re-run through the rewritten code:**

```
> .\Tools\run_suite_bounded.ps1 -VerifyLog Saved\Logs\suite-1167rev2.log -Porcelain
exit=0   DISPATCH ECHO 1 / 1   OK 554 / 0 -- green.
RUNNER_STARTED=554 RUNNER_COMPLETED=554 RUNNER_SUCCESS=554 RUNNER_FAIL=0 RUNNER_SKIPPED=0
```

⚠️ That log is still **untracked**, so this control is still not reproducible by anyone
else — QA's point stands and I have not solved it.

**All fixtures, measured end-to-end:** `green-suite`→0 · `red-suite`→5 ·
`skipped-suite`→0 · `zero-started`→2 · `zero-started-filtered`→3 · `result-absent`→4 ·
`count-mismatch`→8 · `w9-cmd-semicolon`→2 · `green-commands`→0 · `no-terminator-echo`→0 ·
absent log→7. **`-SelfTest` → 52/52, exit 0.**

---

## R2.7 🚨 FOUR DEFECTS THIS REV'S OWN EXECUTION FOUND — three of them in the fix itself

⭐ **This is the section QA should read first.** Every one was invisible to reading and
appeared the moment the thing ran. `SC-§116` cl. 6 is not a slogan.

### R2.7a ⭐ A FALSE RED, REACHABLE FROM THE MOST NATURAL CLI FORM

**MEASURED:** `powershell.exe -File run_suite_bounded.ps1 -Mode Command -Commands A,B,C`
delivers `$Commands` as **ONE element, the string `'A,B,C'`** — `-File` does not split on
commas (only in-shell invocation does). The **emitting** half refused that (exit 64); the
**judging** half had no such guard, so `-VerifyLog` treated `'A,B,C'` as ONE expected
command and returned **`dispatch echo 0 / 1` → exit 2 on a perfectly good log.**

⇒ Fixed by extracting `Assert-SafeFilter` + `Get-CleanCommandList` and calling them from
**both** halves; `-VerifyLog` maps the throw to **exit 64** with a message naming the
working invocation forms. **Mutant M14** guards it.
⇒ ⭐ **Asymmetric sanitisation is how one half of a tool contradicts the other** — the
same shape as B-2, found in a different organ.

### R2.7b ⭐⭐ MY W-7 FIX WAS ITSELF A FALSE RED

The first survivor probe used `Get-Process -Id $proc.Id`. It printed
**`!! PID 30672 SURVIVED the kill`** for a process `Wait-Process` had already confirmed
dead (`HasExited=True`, `ExitCode=-1`). Cause: **`Start-Process -PassThru` holds an open
handle, and a killed process's PID stays resolvable by `Get-Process` while any handle is
open.** ⇒ The probe is now `$proc.Refresh(); $proc.HasExited`.

⇒ ⛔ **Had this shipped, `TASK-1183`'s very first bound run would have reported a failed
kill on a successful one** — in the one line a bound run most needs to trust, to the one
row QA says *"cannot tell a tool defect from a feature defect on a first outing."*
**M16 is this defect, kept as a mutant.**

### R2.7c DEFECT 5b RECURRED — IN A MESSAGE I WROTE FOR THIS FIX

The `-f`-binds-tighter-than-`+` bug reappeared in a **new `throw` message**, because the
self-test placeholder-checked *verdict* messages and *bound* messages but **not throws**.
The refusal printed a literal `Command '{0}' contains a separator`.

⇒ `Test-ThrowsOn` now asserts the throw **threw**, has a **non-empty** message, and
**carries no `{n}`**. **Mutant M15** proves it fires.
⇒ ⭐ **A guard class is not closed until every site that can emit that class is checked.**
Fixing 5 of 5 verdict messages did not fix the family.

### R2.7d POWERSHELL UNROLLED A ONE-ELEMENT ARRAY

`Get-CleanCommandList` returning a 1-element array unrolled to a scalar, so
`$clean.Count` threw under `StrictMode` — caught by the *"a caller-supplied `Quit` is
DROPPED"* case. My first fix (`return ,$clean`) over-corrected and produced
**`-ExecCmds="System.Object[],QUIT_EDITOR"`** — caught by the string-equality cases from
B-3(b), **the very cases QA ordered me to add.**

⇒ Plain return, `@()` at every call site.
⇒ ⭐ **B-3(b) paid for itself before it ever reached QA.**

---

## R2.8 ⛔ WHAT STILL CANNOT BE VERIFIED WITHOUT THE ENGINE — `NOT MEASURED`

1. ⛔ **`UnrealEditor-Cmd.exe` has never been launched by this file.** The launch path is
   now proven against a **stand-in**, which is strictly more than rev-1 had and strictly
   less than a real outing. **The header says exactly this.**
2. ⛔ **No bound has killed a real editor.** A `.cmd` dies on command; UE's shutdown may not.
3. ⛔ **The COMMAND lane has never been dispatched to the engine by this script.**
   ⭐ `TASK-1183` §7 #9 still answers B-1 by observation — quote the `Cmd: QUIT_EDITOR`
   line or its absence. **Either answer is now safe**: the code no longer depends on it.
4. ⚠️ Real orphan children, real incremental boot detection, a real `$proc.ExitCode`,
   `-abslog` with a spaced path (R2.4's list).
5. ⚠️ **Exit 1 is still reachable and is NOT ours.** A PowerShell **parameter-binding**
   failure happens **before any line of the script runs**, so no internal `try/catch` can
   reach it. **Measured.** Documented in the exit table as `(1) NOT ours`.
6. ⚠️ The `Automation RunTests X` echo match is still anchored on UE 5.8 stripping
   `;Quit`. Unchanged from rev-1, and still the place a future engine breaks it.

---

## R2.9 ROUTED TO THE MANAGER — NOT FIXED BY ME (`SC-§82`)

1. ✅ **B-1 is closed in code, so `CONVENTIONS.md:4457` needs NO correction** — it now
   describes this runner accurately. QA's §9 item 1 is discharged by the code path.
2. ⭐ **STILL OPEN, unchanged from rev-1 §8:** `TL-§6` table row 1, `SC-§95` cl. 3 and the
   `TASKBOARD.md` batch header all still read *"THIS FILE DOES NOT EXIST"*. **Three sites.**
   QA corroborated. Manager's file, manager's call.
3. ✅ **`TL-§6`'s `NOT MEASURED` bullet must STAY** until `TASK-1183` reports. I did not
   touch it, and the script's own header now carries the same sentence (W-1).
4. ℹ️ **Census, reconciled:** rev-1 measured `51 / 32`; QA measured `53 / 33`; I now
   measure **`55 / 34`**. The deltas are **QA's own report file (+2/+1) and this rev-2
   note (+2/+1)** — documents *about* the semicolon, not sweeps *of* it. **No prose site
   was changed. `git status` shows my only tracked edits are under `Tools/`.**
5. ⭐ **A finding for the law, new and measured (R2.7a):** `SC-§116` cl. 2's recipe should
   record that **`powershell -File` cannot pass a multi-command `-Commands` list** — it
   arrives as one comma-joined string. The runner now refuses it with instructions, but
   the law's recipe does not warn about it.

---

## R2.10 FILES

| file | state |
|---|---|
| `Tools/run_suite_bounded.ps1` | **REWRITTEN** — 836 → 1616 lines |
| `Tools/SuiteRunnerFixtures/no-terminator-echo.log` | **NEW** — B-1 regression |
| `Tools/SuiteRunnerFixtures/skipped-suite.log` | **NEW** — W-2 regression |
| `Tools/SuiteRunnerFixtures/green-suite.log` · `green-commands.log` | **EDITED** — blank lines added (B-3a) |
| `.claude/pipeline/handoffs/TASK-1181-programmer.md` | this rev-2 appendix; **rev-1 preserved** |
| `.claude/pipeline/TASKBOARD.md` | my `status:` line only |

⛔ **Zero `Source/**`, zero `Content/**`, zero `Config/**`, zero `.uasset`,
`CONVENTIONS.md` untouched.** `TASK-1178`'s two modified `Source/` files are its own lane's.
Scratchpad-only: the two stand-in `.cmd` files, the 16 mutants, the mutation harness.
Test logs under `Saved/Logs/` were removed after measurement.

## R2.11 FOR QA (`TASK-1182` rev-2) — WHAT TO SCRUTINISE

1. ⭐ **R2.7b is the finding I would gate on if I were you.** My own W-7 fix false-red'd,
   and only running it showed that. Ask whether any *other* new check is asserted rather
   than observed.
2. **R2.7a changes behaviour for a caller**: `-VerifyLog` with an embedded separator now
   returns **64, not 2**. That is a deliberate exit-code change. Say if you disagree.
3. **Check I did not weaken the sweep guard.** The `;` refusal is untouched; I only made
   the *judging* half refuse the same input. Census `55 / 34`, all documents.
4. **Check the terminator can never reach the exit code** — grep `TerminatorEcho`; it is
   written, printed and never compared.
5. **52 self-test cases and 16 mutants is still not a live outing.** The header says so.
   `TASK-1183` §7 items #8-#10 remain the acceptance.
