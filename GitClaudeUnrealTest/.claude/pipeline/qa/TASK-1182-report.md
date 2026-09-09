# QA Report — TASK-1182 (gate over TASK-1181, `Tools/run_suite_bounded.ps1`)
# **VERDICT: FAIL — 3 BLOCKERS, 11 WARNINGS, 3 NITS**

> ⭐⭐ **LOOP 2 (rev-2, 2026-09-09): VERDICT `PASS` — 0 BLOCKERS, 5 WARNINGS, 4 NITS.**
> The `FAIL` above is **preserved verbatim and remains true of rev-1.** The **current** verdict,
> the re-ruling on mutation adequacy, and the updated `TASK-1183` duty list are in the
> **`# ⭐⭐ LOOP 2`** section at the END of this file. Read that section, not this line.

**Reviewer:** qa-reviewer · 2026-09-09
**Subject:** `Tools/run_suite_bounded.ps1` (836 lines, NEW) · `Tools/SuiteRunnerFixtures/*.log` (8, NEW) · `handoffs/TASK-1181-programmer.md`
**Law:** `TL-§6` · `SC-§116` (cl. 1, cl. 2, **corrected cl. 4(c)**, **new cl. 6**) · `SC-§95` cl. 1-3 · `SC-§87` · `SHIP-§9` · `SC-§104` · `SC-§101` · `SC-§82` · `SC-§106` · `SC-§71b`

---

## 0. ⛔ I HOLD NO `Bash`. THIS SCRIPT WAS **NOT EXECUTED** BY THIS ROW. (`SC-§71b` — above the verdict, not in a footnote)

**Every finding below comes from READING the script, the fixtures and the law. I did not run
`-SelfTest`. I did not run `-DryRun`. I did not run `-VerifyLog`. I launched no engine, opened
no editor, made no MCP call, and touched no `Content/`, `Source/` or `Config/` file.** 🧑 Jonathan
may be playtesting; the editor is his.

⇒ **The `14 / 14`, the `554 / 0` positive control and the 3 mutation kills in the handoff are
CLAIMS I could not replicate.** I checked them the only way I can: by hand-simulating each
fixture through the parser and confirming the arithmetic reaches the asserted exit code. **That
hand-simulation agrees with the author on all 9 fixture cases** (§4 below) — but agreement of
two readings is not an execution.

⇒ **THE EXECUTION DUTY IS TRANSFERRED BY NAME TO ⭐ `TASK-1183`**, with the expected exit codes
written out in §7. ⭐ **`TASK-1183` is this script's FIRST REAL OUTING** — the launch path
(`Start-Process` · the `.cmdline` sidecar · the polling loop · `Stop-Process`) has **never
executed, not once**, and no bound has ever killed a real process.

---

## 1. VERDICT

**FAIL.** Three blockers. **None of them is a logic error in the guards** — the guards are good,
the lane construction is correct, and the `$LASTEXITCODE` discipline is clean. All three blockers
are the **same species as the defects the author found in himself**: *the instrument is green
against what it was built to judge, and untested against what exists.*

> ⚖️ **This is a good tool with a self-test that cannot fail on the three things the tool exists
> for.** The fixes are small: one line of code (B-1), four lines of reporting (B-2), and one blank
> line plus three string assertions (B-3).

**✅ What I confirm outright, because the row asked and because each was verified by reading, not relayed:**

| question | answer | evidence |
|---|---|---|
| Did the suite `;` survive? | ✅ **YES** | `run_suite_bounded.ps1:157` emits `'Automation RunTests {0};Quit'`; `:156` carries `# DO NOT "FIX" THIS SEMICOLON`; **my own census now reads 53 occurrences across 33 files** under `.claude/pipeline/` — the author's `51 / 32` was correct *before his own handoff added 2 in 1 file*. **Reconciled, not contradicted.** No prose site was swept. |
| Can the script be read as licensing a sweep? | ✅ **NO** | `:33` states the rule as *"ASK WHOSE PARSER SEES THE SEPARATOR"*, not *"commas"*; `:19-33` is a header block a "fixer" must read first; and `:165-166` **actively refuses** a caller-supplied embedded separator with exit 64. |
| Right separator per lane? | ✅ **YES** | Suite `;` at `:157`, Command `,` at `:177`, `QUIT_EDITOR` appended by the script at `:176` and **dropped from caller input** at `:169`. |
| Is `$LASTEXITCODE` ever trusted? | ✅ **NEVER — confirmed exhaustively** | 9 `exit` statements (`:685`, `:691`, `:696`, `:715`, `:735`, `:740`, `:745`, `:831`, `:835`): every one is a constant or is derived from `Get-LogVerdict`, i.e. from the log. `$proc.ExitCode` reaches **only a display string** at `:820-822`. **No error branch consults it.** One reachable undocumented code — see **W-11**. |
| Is the corrected cl. 4(c) implemented? | ⚠️ **HALF** | ✅ the **content-match** half is implemented correctly and the count-based form is explicitly rejected (`:180-186`, `:341-362`). ⛔ the **terminator-exemption** half is NOT — see **B-1**. |
| Is the W9-redundancy reading correct? | ✅ **YES, CONFIRMED** | see §3. |

---

## 2. ⛔ BLOCKERS

### **B-1** — `Tools/run_suite_bounded.ps1:206` (+ the all-must-match loop at `:341-362`) — **the Command lane DEMANDS a `Cmd:` echo for the terminator, which the corrected `SC-§116` cl. 4(c) forbids — and `CONVENTIONS.md:4457` already states as measured fact that this runner does not.**

```powershell
# Get-ExpectedEchoes, :198-207
    $expected += 'QUIT_EDITOR'      # <-- :206  goes into the REQUIRED list
    return $expected
```
`Test-DispatchEcho:356` then reds the whole run if **any** element of `$Expected` is unmatched:
```powershell
if ($matched -ne $Expected.Count) { ... return $result }   # -> exit 2 DISPATCH_ECHO_FAILED
```

**The law, `CONVENTIONS.md:4456` (corrected 2026-09-09, by this very row's subject):**
> *"the engine **STRIPS `;Quit` BEFORE ECHOING** ⇒ … so a per-command echo **may NEVER be
> demanded for the TERMINATOR** (`Quit` / `QUIT_EDITOR`). Its evidence is **SELF-TERMINATION +
> EXIT CODE** (duty (b)), not an echo."*

**And `CONVENTIONS.md:4457` — this is why it is a BLOCKER and not a WARN:**
> *"✅ THE CORRECT DUTY, **AS ⭐ `TASK-1181`'s RUNNER ACTUALLY IMPLEMENTS IT**: FOR EACH
> **NON-TERMINATOR** COMMAND YOU DISPATCHED, ASSERT THE LOG CONTAINS A `Cmd:` LINE …"*

⇒ ⛔ **`CONVENTIONS.md` now asserts, as a measurement, a behaviour the tracked reference
implementation does not have.** Every future reader of cl. 4(c) will believe the exemption is
encoded in code. It is not. This is `SC-§95` cl. 2's propagating claim (*"a fact without an owner
and a verb is a rumour with a `file:line` attached"*) pointed at our own law, and `SC-§101` (*a
prescribed remedy is a claim until measured*) pointed at a sentence written **about** the remedy.

**⚖️ THE HONEST MITIGATION, RECORDED SO THE AUTHOR IS NOT ACCUSED OF FICTION:** the
`green-commands.log` fixture is **not** invented. `handoffs/TASK-1175b-buildmaster.md:71-74`
records a **real, measured** command-lane run in which `Cmd: QUIT_EDITOR` **did** echo:
```
Cmd: Siege.Fog.Raise
Cmd: Siege.Fog.Clear
Cmd: Siege.Fog.Status
Cmd: QUIT_EDITOR
```
**No false red has been observed.** But the terminator's echo is the **last line the engine writes
before it exits** — the single line most likely to be lost to a flush race, a hard exit, or this
script's own `Stop-Process` — and the law's own measurement proves the precedent exists in the
*other* lane (`;Quit` is stripped and never echoes at all). **A guard that reds a good run is
`SC-§68` cl. 5's expensive error, and it would land on `TASK-1183` — the row that cannot tell a
tool defect from a feature defect on a first outing.**

**FIX (one line, and it makes the law true):** remove `'QUIT_EDITOR'` from `Get-ExpectedEchoes`'s
**required** list at `:206`. **Keep it at `New-ExecCmdsValue:176`** — it must still be *dispatched*.
Report it as corroboration only, e.g. `terminator echo: seen / NOT SEEN (not a verdict)`, never
contributing to exit 2. The terminator's evidence per cl. 4(b) is self-termination + wall clock,
**both of which this script already measures** (`:814-815`, `:767-812`).
*Alternative, if the author prefers the strictness: leave the code and route `CONVENTIONS.md:4457`
to the manager for correction (`SC-§82`) — but then the law must say the runner demands it.*
**Either the code changes or the law does. They may not disagree.**

---

### **B-2** — `Tools/run_suite_bounded.ps1:661` + `:663-677` (`Show-Verdict`), called at `:827` before `exit $EXIT_BOUND_EXCEEDED` at `:831` — **on the bound path the script prints `exit 0` and `RUNNER_EXIT=0` for a run it just FORCE-KILLED, then exits 6. `RUNNER_EXIT=6` is unreachable.**

```powershell
# :826-832
if ($boundMessage -ne '') {
    Show-Verdict -Verdict $verdict -AsPorcelain:$Porcelain   # prints  exit 0   and  RUNNER_EXIT=0
    Write-Verdict 'BOUND' $boundMessage 'Red'
    Write-Verdict 'exit' ('{0}' -f $EXIT_BOUND_EXCEEDED) 'Red'
    exit $EXIT_BOUND_EXCEEDED                                 # actually exits 6
}
```
`Show-Verdict:661` prints a line **literally labelled `exit`** carrying `$Verdict.Code`, and
`:665` prints `RUNNER_EXIT={0}` from the same variable. `$Verdict.Code` is the **log** verdict and
is **never** `$EXIT_BOUND_EXCEEDED` — nothing in the file ever assigns 6 into a verdict.

**The reachable bad case is realistic and is exactly why the bounds exist.** The suite completes
and writes all 554 results; the editor commandlet then hangs on shutdown (`SC-§87` INSTANCE 2 /
`SC-§116` cl. 1's second defect — *"all three commands ran correctly and the process STILL
HUNG"*); the log stops growing; the **STALL bound trips at 180 s**; `Stop-Process` kills it. The
script then prints:

```
OK                      554 / 0 -- green.
exit                    0
RUNNER_EXIT=0
BOUND                   STALL bound 180s exceeded: the log has not grown for 180s
exit                    6
```

⇒ ⛔ **A human skimming reads `OK … green … exit 0`. A calling agent parsing the documented
machine contract (`:105-106`, *"Emit machine-readable summary lines (KEY=VALUE) for a calling
agent to parse"*) reads `RUNNER_EXIT=0` and reports a green suite for a run that was killed.**
Two instruments, one process, contradictory answers — and the reassuring one is printed first.
This is the tool's own thesis (`SC-§104`: assert STATE, not tallies; `SC-§114`: a null from an
instrument that never finished is not a result) failing inside the tool.

**FIX:** in the bound branch, set `$verdict.Code = $EXIT_BOUND_EXCEEDED` and prepend the bound
message to `$verdict.Reason` **before** calling `Show-Verdict`, and add `RUNNER_BOUND=<message>`
to the porcelain block. The log verdict is still worth printing — keep it as
`RUNNER_LOG_VERDICT=<n>` — but `RUNNER_EXIT` must equal the process exit code on **every** path,
without exception.

---

### **B-3** — `Tools/SuiteRunnerFixtures/*` + `Invoke-SelfTest:517-629` — **`-SelfTest` cannot detect a mutation to ANY of the three things this tool exists for. The mutation set is NOT adequate, and the gap is measurable, not theoretical.**

**(a) MEASURED BY ME: the fixture corpus contains ZERO blank lines.** I searched all 8 fixtures
for `^\s*$` — **0 matches across 0 files.**
⇒ delete `[AllowEmptyString()]` from `:264`, `:287` and `:375` and **`-SelfTest` still reports
14 / 14**, while the script again *cannot read a real log at all*. That is the author's own
defect **5a**, now enshrined as `SC-§116` cl. 6 (i) — ***"a synthetic corpus tests the LOGIC and
not the INPUT; the fixtures were too clean to be evidence of anything but themselves"*** — and
**the corpus that shipped with the fix still has the property that caused it.** The real-log
positive control that caught it (`Saved/Logs/suite-1167rev2.log`) is **not tracked**, so it is not
reproducible by anyone else, ever. ⇒ ⛔ **The tracked fix has no tracked regression.**
**FIX: one blank line inside one suite fixture.** (Ideally two: one mid-log, one at EOF.)

**(b) `New-ExecCmdsValue:148-178` is NEVER INVOKED by `-SelfTest`.** `Invoke-SelfTest` calls only
`Get-LogVerdict`, which calls `Get-ExpectedEchoes` — a **second, independent** implementation of
the same rule. ⇒ mutate `:157` `'Automation RunTests {0};Quit'` → `'Automation,RunTests {0};Quit'`
(the exact `SC-§95` cl. 1 shape), or `:176` `'QUIT_EDITOR'` → `'Quit'` (the exact `SC-§116` cl. 1
second defect), and **`-SelfTest` reports 14 / 14 and green.**
⇒ ⛔ **The one function that encodes the separator-and-terminator law — `TL-§6`'s entire reason
for existing — is the one function no assertion touches.** The next reader who "fixes" the
semicolon gets a green self-test as their reward.
**FIX: three string-equality cases.** `New-ExecCmdsValue -ForMode Suite -ForFilter Siegebound`
`-eq 'Automation RunTests Siegebound;Quit'` · `-ForMode Command -ForCommands A,B`
`-eq 'A,B,QUIT_EDITOR'` · `-ForCommands A,Quit` `-eq 'A,QUIT_EDITOR'` · and one negative:
`-ForCommands 'A;B'` **throws**. That last one is the guard defending the 51 prose sites and it
currently has **no case, no mutant and no exit-64 coverage of any kind**.

**(c) `New-EditorCommandLine:210-234` is never invoked by `-SelfTest` either.** The
single-verbatim-string `-ArgumentList` (`:222-233`) is the fix for the header's **defect #1** —
the one that produced `Cmd: Automation` and a perfectly green-looking zero-started run. A mutant
returning `$parts` (an **array**) instead of `($parts -join ' ')` reproduces that historical
defect **and survives at 14 / 14**. ⭐ **The most load-bearing line in the file has zero automated
coverage.**
**FIX:** one case asserting the resolved string contains `-ExecCmds="Automation RunTests Siegebound;Quit"`
**as a single quoted token**, and that the function returns `[string]`, not `[string[]]`.

> ⚖️ **The claim *"3 mutants, all killed"* is TRUE and is NOT a coverage claim.** All three
> mutants live inside `Get-LogVerdict` — the judging half. The **emitting** half, the **launching**
> half and the **input-tolerance** are unmutated and unfixturable as shipped. Recorded here so the
> figure cannot be quoted as adequacy (`SC-§101`).

---

## 3. ✅ THE AUTHOR'S THREE SELF-FOUND DEFECTS — FIXES JUDGED

**5a — `[AllowEmptyString()]`: FIX IS CORRECT, REGRESSION IS ABSENT.** Present at `:264`
(`Get-CmdEchoes`), `:287` (`Test-DispatchEcho -Echoes`) and `:375` (`Test-SuiteCounts -Lines`),
alongside `[AllowEmptyCollection()]` — both are needed, because `Get-CmdEchoes` returns `@()` on
a log with no echoes and `Mandatory` rejects an empty array too. ✅ The reasoning is right. ⛔ The
regression is missing — **B-3(a)**. **And the same gap exists in one more place: `:288`
`[Parameter(Mandatory)][string[]] $Expected` carries NEITHER attribute** (**N-2**; unreachable
today because both lanes return ≥ 1 non-empty element, but it is the identical trap two lines
below the fix).

**5b — the `-f` / `+` precedence bug: FIX IS COMPLETE, AND I VERIFIED ALL 12 SITES.** Every
multi-fragment `-f` in the file is now parenthesised as `(("…" + "…") -f $x)`: `:166`, `:317-319`,
`:357-360`, `:411-413`, `:419-421`, `:455-456`. The single-fragment sites (`:365`, `:427`, `:431`,
`:502`, `:505`, `:508`) are unaffected. The `:329-332` and `:403-405` messages use plain
concatenation with **no** `-f` — correct, and `:411`'s `'Result={{}}'` escape is right (it renders
`Result={}` and does **not** false-trigger the meta-guard, which requires `\{\d+\}`).
✅ The meta-guard at `:579-582` is real and would fire. ⛔ **It covers only `$verdict.Reason`** —
the five bound messages from `Test-BoundExceeded` are never placeholder-checked and never
text-asserted (**W-5**). *The lesson was applied to the half of the self-test that had the bug.*

**5c — the W9 redundancy: ⭐ THE AUTHOR'S READING IS CORRECT AND I CONFIRM IT.** I traced the W9
fixture by hand. `Echoes = ['Siege.Fog.Raise;Siege.Fog.Status;Quit']`, `Expected = [Raise, Status,
QUIT_EDITOR]`. **Two independent branches red it:** `:305-323` (`hits = 2 ≥ 2` → the named cause)
and, with that branch deleted, `:341-362` (`matched 0 / 3` → the generic k/n). **Both return exit
2.** ⇒ **the W9 branch supplies the NAMED CAUSE, not the detection** — exactly as
`CONVENTIONS.md:4470` records it, and exactly as the author declined to overclaim.
✅ **And his consequence is the right one and is the most valuable line in the handoff:** *a guard
whose only contribution is the diagnosis is untested if the self-test asserts only exit codes.*
The `ReasonLike` assertion at `:583-586` converts a diagnosis-only branch into a tested one, and
that is why M2 now dies. **This is the correct engineering response to an honest negative, and it
should be read as a positive exhibit, not as a defect.**

**THE FOURTH DEFECT — the row asked me to look, and it is B-1** (the terminator echo), with
**B-3(a)** as its structural sibling: the author's headline lesson — *a synthetic corpus tests the
logic, not the input* — is stated in the code comment at `:524-527` and is **not enforced by the
corpus that comment sits next to**. The gap he found in the fixtures is still in the fixtures.

---

## 4. ✅ HAND-SIMULATION OF ALL 9 SELF-TEST CASES (the only verification available to me)

I counted every fixture's markers with a content search and walked `Test-DispatchEcho` →
`Test-SuiteCounts` by hand. **All 9 reach the asserted code and the asserted `ReasonLike`:**

| fixture | Started / Completed / Success / Fail | path taken | code | `ReasonLike` present |
|---|---|---|---|---|
| `green-suite.log` | 20 / 20 / 20 / 0 | echo `1/1` → counts all equal, `fail=0` | **0** | `green` ✓ |
| `red-suite.log` | 20 / 20 / 18 / 2 | equal, `fail>0` `:425` | **5** | `the suite is RED` ✓ |
| `zero-started.log` | 0 / 0 / 0 / 0 | echo `Automation` alone → `:328` | **2** | `bare 'Cmd: Automation'` ✓ |
| `zero-started-filtered.log` | 0 / 0 / 0 / 0 | echo OK → `:401` `started=0` | **3** | `ZERO tests started` ✓ (+ `(the filter matched 0 tests)` — `Found 0` is present) |
| `result-absent.log` | 20 / 0 / 0 / 0 | echo OK → `:409` | **4** | `UNREADABLE INSTRUMENT` ✓ |
| `count-mismatch.log` | 20 / 18 / 18 / 0 | echo OK → `:417` `20≠18` | **8** | `count mismatch` ✓ |
| `w9-cmd-semicolon.log` | n/a (Command) | `:316` `hits=2` | **2** | `SC-116 W9` ✓ |
| `green-commands.log` | n/a (Command) | 4 / 4 matched → `:475` | **0** | `one line per command` ✓ |
| `__does_not_exist__.log` | — | `:453` | **7** | `no readable log` ✓ |

**Bound arithmetic `:597-603`, walked against `Test-BoundExceeded:492-511`:** `1500/0/booted`→
overall ✓ · `420/5/not-booted`→ boot ✓ · `600/180/booted`→ stall ✓ · `40/1/booted`→ silent ✓ ·
`300/5/not-booted`→ silent ✓. **5 / 5 correct.** `$total = 9 + 5 = 14` at `:619` — **the `14 / 14`
figure is arithmetically consistent with the file.**

⚠️ **The three all-zero fixtures with three different causes** (`w9-cmd-semicolon`,
`zero-started`, `zero-started-filtered`) are a genuinely good piece of test design and are the
single strongest argument in the handoff. **Nothing downstream of the counts can tell them
apart.** ✅ Endorsed.

⚖️ **Provenance audit (`SHIP-§9`, and the row asked):** 4 fixtures are **sliced from a real
554-test log** (`green-suite`, `red-suite`, `result-absent`, `count-mismatch` — the last two by
truncation, `red-suite` by flipping 2 results). 3 are **real preambles + a signature recorded
verbatim in law** (`zero-started` ← `SC-§95` cl. 1; `w9-cmd-semicolon` ← `SC-§116` cl. 1;
`zero-started-filtered`). **1 — `green-commands.log` — is in NEITHER category and its provenance
is not declared in the handoff's honesty note (§4a).** I traced it: it faithfully reproduces
`handoffs/TASK-1175b-buildmaster.md:71-74`, a real measured run. ⇒ **the fixture is sound; the
declaration is incomplete** (**W-12**). Name its source in the handoff.

**Guard coverage, stated honestly: 7 of the 9 documented exit codes have a self-test case.
Exit 6 (`BOUND_EXCEEDED`) and exit 64 (`USAGE`) have NEITHER a fixture NOR a mutant.**

---

## 5. ⚠️ WARNINGS

- **[W-1]** `:3-4` / `.NOTES :35-52` — the script's own synopsis calls itself **"THE ONE
  SANCTIONED EXECUTOR … Bounded, self-instrumenting, and it can say NO"** with **no `NOT
  EXECUTED` note anywhere in the file.** `TL-§6` (`CONVENTIONS.md:5966-5969`) and the handoff §6
  both carry the caveat; **the most-copied artefact does not** (`SC-§95` cl. 2: *a correction
  recorded in ONE place does not correct the claim*; `SC-§116` cl. 6). **Fix:** add to `.NOTES`:
  `⛔ LAUNCH PATH NOT EXECUTED as of 2026-09-09. Start-Process / .cmdline / polling / Stop-Process
  are UNRUN; bound arithmetic is proven only against synthetic clocks. First real outing: TASK-1183.`
  **Delete that line only when TASK-1183 reports.**
- **[W-2]** `:386-387` — only `Result={Success}` and `Result={Fail}` are counted. UE's
  `EAutomationState` also emits **`Result={Skipped}`** (and `NotRun`). **One skipped test makes
  `completed ≠ success + fail` → exit 8 with the diagnosis *"the run is incomplete or the log is
  truncated"* — a WRONG CAUSE for a healthy run** (`TL-§5b` 2c: a wrong explanation of a real
  hazard is more dangerous than none). At 554 tests this is a matter of time. **Fix:** count
  `Result=\{(\w+)\}` generically as `resulted`, tally `skipped` separately, compare
  `completed == resulted`, and report skips in the verdict.
- **[W-3]** `:751` `Remove-Item -LiteralPath $LogPath -Force` — **`-LogPath` is caller-supplied
  and completely unvalidated** (`:94`). Nothing confines it to `Saved/Logs`. A mistyped
  `-LogPath` force-deletes whatever it names, including a `.uasset`. **Fix:** reject a `-LogPath`
  that does not resolve under `$RepoRoot\Saved\` or does not end `.log`, with exit 64.
  ⇒ **`TASK-1183` must use the DEFAULT `-LogPath` for every run.**
- **[W-4]** `:155-158` vs `:165-166` — **asymmetric sanitisation.** The Command lane rejects a
  caller-supplied `,`/`;`; the **Suite lane interpolates `$Filter` raw** into the quoted
  `-ExecCmds` value *and* into the whole verbatim command line. A `-Filter` containing `,`, `;`
  or `"` mangles the invocation (a `"` escapes the quoting entirely). The echo guard would catch
  the benign shapes — but this is the same class as 5a: **a parameter never tested against a real
  caller.** **Fix:** apply the `:165-166` rejection to `$Filter` as well.
- **[W-5]** `:606-617` — the 5 bound cases assert **only `tripped == want`**. The message text is
  never asserted and never placeholder-checked. **Defect 5b's exact blind spot, surviving in the
  half of the self-test that did not have the bug.** **Fix:** give each bound case a `ReasonLike`
  and run the `\{\d+\}` guard over `$msg`.
- **[W-6]** `:365` — the **success** message reads `'dispatch echo {0} / {1} -- one line per
  command'`. That is the **superseded** wording of cl. 4(c), the wording `CONVENTIONS.md:4455`
  struck **today, on this row's own evidence** — and `Invoke-SelfTest:553` pins it as an
  assertion. **The guard does the right thing and tells its reader it did the wrong thing.**
  **Fix:** `'dispatch echo {0} / {1} -- matched by CONTENT; extra engine Cmd: lines ignored'`,
  and update the `ReasonLike`.
- **[W-7]** `:806` `Stop-Process -Id $proc.Id -Force` — kills **only the top-level process** and
  **never confirms it died**. UE spawns `ShaderCompileWorker`, `CrashReportClient`,
  `EpicWebHelper`; the script then reads a log the corpse may still hold open. **Fix:**
  `Wait-Process -Id $proc.Id -Timeout 15` after the kill, report survival explicitly, and name
  any surviving child. `SHIP-§9`: **an unkilled process is the failure this bound exists to detect.**
- **[W-8]** `:758` — nothing checks whether an **interactive editor is already running on this
  project** before launching a second instance. The standing operational law is that the editor is
  Jonathan's and may be in use. **Fix:** if `UnrealEditor*.exe` is running, print a loud warning
  (do not auto-kill — that is a human decision).
- **[W-9]** `:475-479` — in the Command lane **exit 0 means DISPATCHED, not SUCCEEDED**. A caller
  reading `RUNNER_EXIT=0` will read it as success (`SC-§104`: assert STATE, not tallies —
  `CONVENTIONS.md:4542` explicitly praises `Siege.Fog.Status` **reading the state back** as the
  standard). **Fix:** say so in the verdict text, emit `RUNNER_MODE=Command`, and consider an
  optional `-ExpectLogPattern` so a caller can assert resolved state, not dispatch.
- **[W-10]** `:782-795` — the boot-peek calls `Read-LogLines`, a **full `Get-Content` of the whole
  growing log, every 2 seconds**, until the echo is seen. On the exact path this exists to catch
  (the echo never appears) that is **~210 full re-reads over 420 s** of an ever-growing file, and
  the re-read cost rises as the file does. **Fix:** track a byte offset, or read only the tail.
- **[W-11]** `:109-110` + the whole MAIN block — `$ErrorActionPreference = 'Stop'` with
  `Set-StrictMode -Version Latest` and **no top-level `try/catch`** means any unhandled
  terminating error (a failed `Start-Process`, a locked sidecar at `:755`, a failed `New-Item` at
  `:749`) exits with **PowerShell's 1** — **a code that appears nowhere in the documented table
  and that a caller will not recognise.** **Fix:** wrap MAIN in `try/catch`, print the exception,
  and exit a documented code.
- **[W-12]** `handoffs/TASK-1181-programmer.md` §4a — `green-commands.log`'s provenance is
  undeclared while the note explicitly declares the other seven. It is sound (it reproduces
  `handoffs/TASK-1175b-buildmaster.md:71-74`, a real run) — **name that source**, because it is
  the *only* evidence behind **B-1**'s current safety.

---

## 6. NITS

- **[N-1]** `:755` `Set-Content -Encoding utf8` writes a **BOM** under Windows PowerShell 5.1. The
  `.cmdline` sidecar is meant to be copy-pasted; a BOM in front of a path is a bad first character.
  Use `-Encoding ascii` or `utf8NoBOM`.
- **[N-2]** `:288` `[Parameter(Mandatory)][string[]] $Expected` lacks the `[AllowEmptyCollection()]`
  / `[AllowEmptyString()]` the author added to its siblings two lines above. Unreachable today;
  it is the identical trap.
- **[N-3]** `:579-587` — a single failing case can append **three** entries to `$failCases`
  (placeholder + reason + code), so the printed `FAILURES:` list can exceed the case count and a
  reader counting lines will over-count failures. Cosmetic; group per case.
- **[N-4]** `:125` `$script:RepoRoot = Split-Path -Parent $PSScriptRoot` resolves the **project**
  root, which is correct for the `.uproject` — but it is **not the git root** (`SC-§102`: the git
  root is one level up, *and a mis-anchored pathspec answers with SILENCE*). Rename to
  `$ProjectRoot` before someone reuses it for a pathspec.

---

## 7. ⭐⭐ EXECUTION DUTY TRANSFERRED BY NAME TO **`TASK-1183`** (`TL-§5c` cl. 5(c) · `SC-§116` cl. 6)

⛔ **I hold no `Bash`. Nothing in this report was executed.** ⭐ **`TASK-1183` is this script's
FIRST REAL OUTING.** Until it reports, **`TL-§6`'s `NOT MEASURED` bullet at
`CONVENTIONS.md:5966-5969` STAYS** — and only the **manager** may strike it (`SC-§82`).

**Run these in order and QUOTE THE EXIT CODE AFTER EACH.** ⚠️ Here `$LASTEXITCODE` **is** the right
thing to read — it is *this script's own* verdict, derived from the log; the never-trust law binds
`Build.bat` and the **editor**, not the runner.

| # | command | expect | what it proves / what to quote |
|---|---|---|---|
| 1 | `-SelfTest` | **0**, `14 / 14` | paste the two tables. If the blockers are fixed first, expect **more than 14** — quote the new total. |
| 2 | `-VerifyLog Tools\SuiteRunnerFixtures\green-suite.log` | **0** | the green path |
| 3 | `-VerifyLog Tools\SuiteRunnerFixtures\red-suite.log` | **5** | **a red suite is reported red** |
| 4 | `-VerifyLog Tools\SuiteRunnerFixtures\zero-started.log` | **2** | `SC-§95` cl. 1 — the historical space split |
| 5 | `-VerifyLog Tools\SuiteRunnerFixtures\count-mismatch.log` | **8** | a truncated run is not green |
| 6 | `-Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Status -VerifyLog Tools\SuiteRunnerFixtures\w9-cmd-semicolon.log` | **2** | `SC-§116` W9, **named by cause** |
| 7 | `-DryRun` **and** `-Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status -DryRun` | **0** each | **paste BOTH command lines verbatim** and confirm them character-for-character against `SC-§116` cl. 2's table (`CONVENTIONS.md:4443-4444`). The suite line MUST still carry `;Quit`. |
| 8 | ⭐ **the first real suite run:** `-Porcelain` | **0** on a green tree | ⛔ the **MEASURED `N / M` THROUGH THE SCRIPT** · `RUNNER_ECHO_MATCHED/EXPECTED` · `discovered` · wall clock · **and whether the number matches the hand-typed lane.** |
| 9 | ⭐ **the command lane's first dispatch through the script:** `-Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status` | **0**, self-terminating ≈ **12 s** | ⛔⛔ **AND ANSWER B-1 BY OBSERVATION: does the log THIS RUN produced contain `Cmd: QUIT_EDITOR`? Quote the line, or state its absence.** One observation settles whether demanding the terminator echo is safe. |
| 10 | ⭐⭐ **make a bound actually kill something:** a real launch with `-BootSeconds 20 -OverallSeconds 45 -StallSeconds 15` | **6** | ⛔ **THE KILL PATH HAS NEVER RUN.** Quote the `BOUND TRIPPED:` line, then **confirm with `Get-Process` that the PID is GONE**, and name any surviving `UnrealEditor-Cmd` / `ShaderCompileWorker` child (**W-7**). `SHIP-§9`: *validate the bound against the hang it exists to detect.* |
| 11 | — | — | **B-2 live check:** for run #10, report **both** `$LASTEXITCODE` **and** the porcelain `RUNNER_EXIT=` line, and **say whether they agree.** They currently will not. |
| 12 | — | — | `cat` the `.cmdline` sidecar and confirm it matches what was launched (`SC-§116` cl. 4(a)). |

⛔ **`-LogPath` stays at the DEFAULT for every run above** (**W-3**).
⛔ **Runs #8-#10 need the editor. 🧑 Jonathan may be playtesting — check before launching, and
never kill an interactive instance to make room** (**W-8**).

---

## 8. ⚖️ THE TWO RULINGS THE ROW ASKED FOR

### **(a) Shipping an unexecuted launch path — RULING: ✅ SHIP IT, once the three blockers are fixed. It is TRACKED, not SANCTIONED, and the law must keep saying so.**

**Reasons, in order of weight:**
1. **The alternative is strictly worse and is the debt itself.** Holding the file out of git until
   it has run leaves the suite executor where it has always been — an unversioned scratchpad copy
   that dies with the session and gets re-typed from memory (`TL-§6`'s measured event: `TASK-1044`
   → `TASK-1056`, *"two different programs, one name"*). **A tracked, reviewed, imperfect executor
   beats an untracked, unreviewed, unknown one on every axis.**
2. **The unexecuted portion is bounded in blast radius.** `Start-Process` / poll / `Stop-Process`
   can hang, kill late, or kill incompletely. It cannot corrupt the repo, the engine, or an
   asset — with the one exception of `:751`'s `Remove-Item`, which is why **W-3** exists and why
   `TASK-1183` is confined to the default `-LogPath`.
3. **The judging half — the half that decides whether we believe a suite result — is the half that
   IS tested**, against a truncated real 554-test log, and I re-derived all 9 verdicts by hand.
4. ⛔ **But `SANCTIONED ≠ EXERCISED`, and the burden of saying so travels WITH THE ARTEFACT**
   (`SC-§116` cl. 6 · `SC-§95` cl. 2). `TL-§6`'s `NOT MEASURED` bullet stays until `TASK-1183`
   reports; the **script's own header must carry the same sentence** (**W-1**); and **no row may
   quote `TL-§6`'s *"the ONE sanctioned executor"* cell without the caveat** until items #8, #9
   and #10 of §7 are on the record.

### **(b) Is the mutation set adequate? — RULING: ⛔ NO. It is adequate for the JUDGING half and absent for the EMITTING, LAUNCHING and INPUT halves.**

The three mutants (M1 verdict-message formatting, M2 the W9 branch, M3 `Started == 0`) are all
inside `Get-LogVerdict`. They are **well chosen** — M2 in particular produced the honest negative
that improved the self-test — and each is genuinely killed. **But they are three probes into one
of four surfaces.**

**Failure modes with NO mutant and NO fixture, named as the row demanded:**

| # | failure mode | site | survives `-SelfTest` at 14/14? |
|---|---|---|---|
| 1 | **the suite `;` swapped for a `,`** — the sweep `SC-§116` cl. 2 exists to prevent | `:157` | ⛔ **YES** |
| 2 | **`QUIT_EDITOR` reverted to `Quit`** — the 10-minute hang of `SC-§116` cl. 1 | `:176` | ⛔ **YES** |
| 3 | **the caller-separator rejection deleted** — the guard defending 53 prose sites; **exit 64 has no case at all** | `:165-166` | ⛔ **YES** |
| 4 | **`-ArgumentList` reverted to an array** — the `SC-§95` cl. 1 space split, the historical false green | `:222-233` | ⛔ **YES** |
| 5 | **`[AllowEmptyString()]` removed** — the script can no longer read any real log | `:264`/`:287`/`:375` | ⛔ **YES** (corpus has **0** blank lines, measured) |
| 6 | **a bound message regressed to the `-f`/`+` bug** | `:502`-`:508` | ⛔ **YES** (bound cases assert only tripped/not) |
| 7 | **the kill fails / the process survives** — exit 6 | `:806` | ⛔ **no fixture, no mutant, never executed** |
| 8 | **a skipped test misdiagnosed as truncation** | `:386-387` | ⛔ **no case** (W-2) |

**Adequacy is restored cheaply — one blank line and roughly four assertions close rows 1-6.** Rows
7 and 8 belong to `TASK-1183` (§7 #10) and to a follow-up row respectively.

---

## 9. 📮 ROUTED TO THE MANAGER — NOT FIXED BY ME (`SC-§82`; I may not edit `CONVENTIONS.md`)

1. ⛔ **`CONVENTIONS.md:4457` asserts *"AS `TASK-1181`'s RUNNER ACTUALLY IMPLEMENTS IT"* for the
   full corrected duty, including the terminator exemption. The runner implements the content-match
   half only (B-1).** Either the code changes (preferred — one line) or the sentence does. **A law
   that mis-describes its own reference implementation is `SC-§95` cl. 2 pointed at us.**
2. ⛔ **`CONVENTIONS.md:5961` (`TL-§6` table row 1) still reads *"MEASURED 2026-09-09: THIS FILE
   DOES NOT EXIST"*.** It exists — I read all 836 lines of it. `SC-§95` cl. 3's sibling site and
   the `TASKBOARD.md` batch header carry the same struck measurement. **Three sites**, as the
   author's §8 correctly routed; I corroborate and re-route rather than relay.
3. ✅ **`CONVENTIONS.md:5966-5969` (`NOT MEASURED`) is CORRECT AS WRITTEN and must NOT be struck by
   `TASK-1183`'s host** — only by the manager, and only after §7 items #8-#10 are on the record.
4. ℹ️ **Census reconciliation, so nobody reads a discrepancy:** the handoff's `51 occurrences /
   32 files` was correct **at the moment it was measured**. My census today reads **53 / 33** — the
   delta is **`handoffs/TASK-1181-programmer.md` itself** (2 occurrences, 1 file). **No prose site
   was swept; the suite `;` is intact everywhere.**

---

## 10. NOTES FOR BUILD-MASTER (`TASK-1183`)

⛔ **This report is a FAIL.** Per routing rule 4 this goes back to `gameplay-programmer` with
`TASK-1181` before `TASK-1183` runs. **Do not stage `Tools/run_suite_bounded.ps1` on this
verdict.**

When a PASS lands, the run duty in **§7** is yours, in full and in order, and **items #8, #9 and
#10 are the acceptance** — the run *is* the acceptance, and a guard you have only seen pass is
`NOT MEASURED` (`SHIP-§9`). ⚠️ `SC-§103`: a batch commit owes **N** status flips —
`TASK-1181`, `TASK-1182` and `TASK-1183` all flip in the same action.

---
---

# ⭐⭐ LOOP 2 — re-gate of `TASK-1181` rev-2 (`836` → `1616` lines)
# **VERDICT: PASS — 0 BLOCKERS, 5 WARNINGS (all new), 4 NITS (all new)**

**Reviewer:** qa-reviewer · 2026-09-09 · **loop 2 of max 3**
**Subject:** `Tools/run_suite_bounded.ps1` (1616 lines) · `Tools/SuiteRunnerFixtures/*.log` (**10**, 2 new + 4 edited) · `handoffs/TASK-1181-programmer.md` **§R2**
**Loop 1 above is preserved verbatim. Nothing in it was edited.** Its `FAIL` was true of rev-1 and stays on the record.

---

## L2.0 ⛔ I STILL HOLD NO `Bash`. THIS SCRIPT WAS **NOT EXECUTED** BY THIS ROW. (`SC-§71b` — above the verdict)

**I ran nothing. No `-SelfTest`, no `-DryRun`, no `-VerifyLog`, no engine, no MCP call, no
`Content/`/`Source/`/`Config/` file touched.** 🧑 Jonathan is asleep and **an art lane is using the
editor — I did not go near it.** Every finding below comes from reading the script, the 10
fixtures, the law and the board, and from **censuses I ran with content search**.

⇒ **EXECUTION DUTY REMAINS TRANSFERRED BY NAME TO ⭐ `TASK-1183`** — updated list at **L2.9**,
which **supersedes loop 1 §7**.

### ⚖️ MY OWN LOOP-1 SENTENCE, AND ITS CONVERSE, APPLIED TO THIS REV

Loop 1 said: *"agreement of two readings is not an execution."* The row is right that the
converse binds me now: **a reported execution is a claim until I can see its artefact.**

⛔ **And I can see none of them.** `R2.10` records that the two stand-in `.cmd` files, all 16
mutants, the mutation harness and every test log under `Saved/Logs/` were **deleted after
measurement**. That is correct scratchpad hygiene and I do not fault it — but it means
**the entire executed-evidence base of rev-2 is DECLARED, not shown.**

⭐ **What rescues this rev from being loop 1 with more prose is that its two most important
claims are corroborated by STRUCTURE I can read, independently of whether the runs happened.**
`B-2`'s fix is not a report of an output; it is a **census result** (one `exit`, two
`Show-Verdict` call sites, `Merge-BoundIntoVerdict` overwriting `Code`) — and that census
proves the rev-1 behaviour is **no longer constructible**, whatever any log said. Same for
`B-1`: I traced `TerminatorEcho` to ground rather than trusting two fixtures.

**The verified/declared split is L2.1 and L2.2. Nothing is blurred between them.**

---

## L2.1 ✅ WHAT I VERIFIED MYSELF, FROM FILES I CAN READ

| # | claim | how I checked it | result |
|---|---|---|---|
| 1 | ⭐ **"exactly one `exit` statement"** | census `^\s*exit\b` over the file | ✅ **2 hits; one is line `786`, INSIDE the `<# … #>` comment block spanning `:782-795`.** The only executable `exit` is **`:1616  exit $exitCode`**. `:248` (`'^(?i)(quit\|exit\|quit_editor)$'`) is a regex literal and `:1098` (`@('Quit','Exit')`) is test data. |
| 2 | no back-door process exit | census `Environment]::Exit` · `SetShouldExit` · `Write-Output` | ✅ **zero hits.** `:1616` is the only way this script can terminate. |
| 3 | ⭐ **one reporting path** | census `Show-Verdict` | ✅ **exactly 2 call sites** — `:1394` (VerifyLog) and `:1597` (run) — **each immediately followed by `return <that same hashtable>.Code`** (`:1395`, `:1598`). ⛔ **There is no `Show-Verdict` in the bound branch any more.** rev-1's `:827`-then-`exit 6` shape is gone. |
| 4 | ⭐ **`RUNNER_EXIT` and the process exit code are one variable** | traced `$code` in `Show-Verdict:1304` ← `$Verdict.Code`; `RUNNER_EXIT` printed from **the same `$code`** at `:1344`; `Merge-BoundIntoVerdict:807` sets `$Verdict.Code = 6` **before** `Show-Verdict` is called at `:1597` | ✅ **They cannot disagree by construction.** This is the whole of B-2 and it is verifiable without running anything. |
| 5 | ⚠️ **`$exitCode` cannot become an array** (which would make `exit $exitCode` throw at `:1616`, **outside** the `try/catch` that ends at `:1615`, and surface as PowerShell's undocumented 1) | walked **every statement** of `Invoke-Main`, `Invoke-SelfTest`, `Show-Verdict`, `Add-SelfTestCase`, `Test-ThrowsOn` for success-stream leaks | ✅ **Clean.** All human output is `Write-Host` (information stream, not captured by assignment); every cmdlet that returns objects is assigned, `\| Out-Null` (`:1471`, `:901`) or `[void]` (`:450`, `:1239`); `Stop-Process`/`Wait-Process`/`Remove-Item`/`Set-Content` are all called without `-PassThru`; `$script:StTotal++` as a bare statement emits nothing. |
| 6 | ⭐⭐ **B-1: the terminator "cannot reach the exit code by any path"** | census `TerminatorEcho` + `$seen`, then traced the required list to its source | ✅ **CONFIRMED BY READING, NOT BY THE FIXTURES.** `TerminatorEcho` is **written** at `:534`, `:539`, `:546`, `:548` and **read** at `:1255`/`:1264` (self-test), `:1310` (display), `:1354` (porcelain) — **never in the matching loop `:596-619`, never in `$result.Ok`**. `$seen` is function-local, written `:541`/`:543`, read once at `:545`, dead thereafter. **And the terminator cannot even enter `$Expected`:** `Get-ExpectedEchoes:320` → `Get-CleanCommandList`, which **drops `quit\|exit\|quit_editor` at `:248`** before returning. Two independent barriers. |
| 7 | ⭐ **the B-1 regression fixture is honest** | read both files in full | ✅ **`no-terminator-echo.log` is `green-commands.log` with exactly one line deleted** — `green-commands.log:14  Cmd: QUIT_EDITOR`. Every other line is identical. It is the flush race loop 1 described. |
| 8 | ⭐⭐ **B-3(a): my loop-1 census was 0 blank lines / 8 fixtures. Does the regression exist now?** | re-ran `^$` over **all 10** fixtures | ✅ **8 empty lines across 4 of 10 fixtures** — `green-suite:10,57` · `skipped-suite:10,57` · `green-commands:10,16` · `no-terminator-echo:10,15`. ⭐ **Mid-log AND at EOF, exactly as claimed.** `Get-Content` on these yields genuine empty-string ELEMENTS, so deleting `[AllowEmptyString()]` from `:497`/`:523`/`:641` **does** now red the run. **The regression is real and tracked.** |
| 9 | ⭐ **"52 cases in 8 sections"** | counted every `Add-SelfTestCase` / `Add-ThrowCase` / loop iteration in the file | ✅ **52, exactly.** §1 = 11 fixtures · §2 = 5 bounds · §3 = 4 + **8** throws = 12 · §4 = 2 + **3** throws = 5 · §5 = 4 · §6 = 2 · §7 = 7 path cases · §8 = 6. **11+5+12+5+4+2+7+6 = 52.** This number is **verified, not accepted.** |
| 10 | ⭐ **all 11 fixture verdicts, re-derived by hand AFTER the blank-line edits** | counted `Test Started.` / `Test Completed.` / `Result={…}` / `Cmd: ` / `Found N` markers myself | ✅ **all 11 reach their asserted code — see the table at L2.3.** ⭐ **The corpus edits did not disturb a single count.** |
| 11 | ⭐ **B-3(b)+(c): case-sensitive equality on both `-ExecCmds` values** | read the operators | ✅ **`-cne` at `:1072`, `:1077`, `:1082`, `:1087`, `:1114`; `-cnotmatch` at `:1155`, `:1163-1166`, `:1176`.** The suite `;Quit` form and the `A,B,QUIT_EDITOR` form are both pinned **case-sensitively**, and `:1148` asserts `-is [string]`. **Loop 1's exact prescription, implemented.** |
| 12 | **W-1 closed?** | read the header | ✅ **`.SYNOPSIS:6-7`** (*"SANCTIONED IS NOT EXERCISED"*) **and `.NOTES:60-69`** (`!!! SANCTIONED IS NOT EXERCISED — LAUNCH PATH NOT EXECUTED AGAINST THE ENGINE`, naming `Start-Process`, the sidecar, the polling loop, boot detection, `Stop-Process` and the survivor check; *"FIRST REAL OUTING: TASK-1183"*; *"DELETE THIS BLOCK ONLY THEN"*). **The burden now travels with the artefact.** |
| 13 | **W-2 closed?** | read `:653-720` + counted `skipped-suite.log` | ✅ **Generic `Result=\{(\w+)\}` at `:656`**, tallied into `success/fail/skipped/notrun/other`, completeness is `Started == Completed == resulted` at `:698`. I counted the fixture myself: **20 Started / 20 Completed / 18 `{Success}` / 2 `{Skipped}` / 0 Fail** ⇒ `20 == 20 == 20`, `fail = 0` ⇒ **exit 0**, reason `18 / 0 -- green. [2 skipped, 0 not-run]`. The wrong-cause diagnosis is gone. |
| 14 | **W-3 closed?** | read `Resolve-SafeLogPath:357-399` and its call site | ✅ **`:1411` runs the guard; the `Remove-Item -Force` is at `:1473` — 62 lines LATER, behind a `return $EXIT_USAGE` at `:1414`.** Traversal is defeated by `GetFullPath` normalisation (`:375`) **plus** the trailing-`\` prefix test at `:382-384`, so `…\SavedEvil\x.log` cannot pass either. 7 self-test cases pin it. |
| 15 | ⭐ **did the author weaken the sweep guard?** | census `Automation RunTests Siegebound;Quit` under `.claude/pipeline/` | ✅ **NO. 58 occurrences / 34 files** — see the reconciliation at **L2.7**. The count only went **UP**, and every delta is a document *about* the semicolon. **No prose site was swept.** `run_suite_bounded.ps1:280` still emits `'Automation RunTests {0};Quit'` under the `# DO NOT "FIX" THIS SEMICOLON` comment at `:279`. |
| 16 | **the three routed law sites** | census `DOES NOT EXIST` | ⛔ **all three still read FALSE** — `CONVENTIONS.md:4895`, `CONVENTIONS.md:5961` (`TL-§6` row 1), `TASKBOARD.md:1735`. Still the manager's. ✅ Note `:5961` **has** acquired the `SANCTIONED ≠ EXERCISED … SEE THE NOT MEASURED NOTE BEFORE QUOTING THIS CELL` caveat. |
| 17 | **is `suite-1167rev2.log` really unreproducible?** | `Saved/Logs/suite-1167rev2.log` exists on disk; `.gitignore:111` = `Saved/` | ⚠️ **Confirmed untracked, and it will stay untracked.** Ruling at **L2.8**. |

---

## L2.2 ⚠️ WHAT I AM ACCEPTING AS **DECLARED** — I could not see one artefact of any of it

Every item below is a **claim**. The scratchpad stand-ins, the 16 mutants, the harness and the
`Saved/Logs/bound-proof-*.log` files were deleted after measurement (`R2.10`).

1. `RUNNER_EXIT=6` · `$LASTEXITCODE=6` · `RUNNER_LOG_VERDICT=0` from `fake_editor_hang_after_green.cmd` (**PROOF A**).
2. `RUNNER_EXIT=6` · `RUNNER_LOG_VERDICT=7` from the BOOT bound (**PROOF B**).
3. *"under rev-1 the identical run printed `OK 20 / 0 -- green`, `exit 0`, `RUNNER_EXIT=0` and then exited 6."*
4. `-SelfTest → 52 / 52, exit 0`. (**The 52 I verified; the 52 PASSES I did not.**)
5. **16 mutants applied and killed**, and each row's `N / 52` tally. ⚠️ **Two of those tallies do not survive arithmetic — L2.6.**
6. The `.cmdline` sidecar's first three bytes `67 58 92`.
7. `powershell -File … -Commands A,B,C` arriving as the single string `'A,B,C'` (**R2.7a**).
8. ⭐ **R2.7b** — `Get-Process -Id` printing `SURVIVED the kill` for a PID `Wait-Process` had confirmed dead.
9. `-VerifyLog Saved\Logs\suite-1167rev2.log → 554 / 554`, exit 0.
10. The two `-DryRun` command lines and the full 11-fixture end-to-end exit table.

⭐ **Item 8 is the one I weigh most, and it is the one I can least verify — so I judged the FIX
instead of the RUN, and the fix is correct.** `:1555-1566`: `Wait-Process -Timeout 15` →
`$proc.Refresh()` → `$proc.HasExited`. **That is the right probe and `Get-Process -Id` is the
wrong one**, for exactly the stated reason: `Start-Process -PassThru` holds an open handle, and a
terminated process's PID remains resolvable by `Get-Process` while any handle to it is open —
so `Get-Process -Id` answers *"a process object exists"*, **not** *"the process is alive"*.
`HasExited` on a refreshed handle answers the actual question. ✅ **The corrected check is sound,
and I would have passed the broken one on a read** — I did not catch it in loop 1 either, and
loop 1 named `:806` as W-7 without noticing that the obvious fix was itself a false red.

⇒ ⭐⭐ **THE GENERALISATION, AND IT IS THE FINDING OF THIS LOOP.** This is the **third** instrument
in this session whose *null / negative* reading was wrong, after `SC-§94` (the log printed the
REQUEST, not the RESULT) and rev-1's `-f`/`+` messages (the code was right, the sentence was
garbage). ⛔ **It is now the FOURTH — because I found one more, in the fix itself: `W-13` below.**
The pattern is not "instruments lie". It is narrower and far more useful:
> ⚖️ **AN INSTRUMENT'S POSITIVE READING GETS EXERCISED CONSTANTLY AND ITS NEGATIVE READING GETS
> EXERCISED ONCE, IN THE INCIDENT — SO THE NEGATIVE PATH IS ALWAYS THE UNTESTED HALF.**
> `Get-Process -Id` was *right* every time the process lived. It was wrong exactly once: the
> first time something died. **`SHIP-§9` says validate a gate against the failure it detects;
> this says the same thing about a PROBE, and the project has now paid for it four times.**
**Routed to the manager as a candidate clause — I do not write `CONVENTIONS.md` (`SC-§82`).**

---

## L2.3 ✅ ALL 11 FIXTURE VERDICTS, RE-DERIVED BY HAND AFTER THE CORPUS EDITS

Markers counted by me, not read off the handoff.

| fixture | Started / Completed / Result-lines | Success / Fail / Skipped | echo path | code | agrees? |
|---|---|---|---|---|---|
| `green-suite.log` | 20 / 20 / 20 | 20 / 0 / 0 | `Cmd: Automation RunTests Siegebound` → 1/1 | **0** | ✅ |
| `red-suite.log` | 20 / 20 / 20 | 18 / **2** / 0 | 1/1 | **5** | ✅ |
| `skipped-suite.log` | 20 / 20 / 20 | 18 / 0 / **2** | 1/1 | **0** + `[2 skipped, 0 not-run]` | ✅ |
| `zero-started-filtered.log` | 0 / 0 / 0 | — | 1/1 ✓, then `Started == 0`, `Found 0` present | **3** | ✅ |
| `result-absent.log` | 20 / **0** / 0 | — | 1/1 | **4** | ✅ |
| `count-mismatch.log` | 20 / **18** / 18 | 18 / 0 / 0 | 1/1 | **8** | ✅ |
| `zero-started.log` | 0 / 0 / 0 | — | `Cmd: Automation` bare → `:566` no-continue, no `;`, then `:585` fires | **2** | ✅ |
| `w9-cmd-semicolon.log` | n/a (Command) | — | `Raise;Status;Quit` → `hits = 2 ≥ 2` at `:573` | **2** | ✅ |
| `green-commands.log` | n/a (Command) | — | 3/3 matched, `QUIT_EDITOR seen` | **0** | ✅ |
| ⭐ `no-terminator-echo.log` | n/a (Command) | — | **3/3 matched, `QUIT_EDITOR NOT SEEN`** | **0** | ✅ **B-1 confirmed independently** |
| `__does_not_exist__.log` | — | — | `Read-LogLines` → `$null` at `:745` | **7** | ✅ |

⭐ **The two Command fixtures differ by exactly one line and produce exactly the same exit code.**
Combined with the `TerminatorEcho` trace at L2.1 #6, **B-1 is closed twice over — once by the
data and once by the control flow.** The row asked me not to trust the two fixtures alone; I did
not, and the code agrees with them.

⚠️ **`zero-started.log` still returns `2`, not `3`.** Loop 1 was asked to rule on this and did
not, so I rule now: ✅ **`2` is CORRECT and I endorse it.** The echo names the **cause** (the
value was split on spaces before the engine saw it); `3` would name the **symptom** (no tests
ran) and would send a reader hunting a filter bug. `zero-started-filtered.log` isolates the `3`
path so the distinction is tested, not merely argued. **Deliberate, defensible, and now ruled.**

---

## L2.4 ⚖️ THE THREE BLOCKERS — RE-JUDGED

### ✅ **B-1 — CLOSED. And I confirm nothing is owed to the manager on it.**

`Get-ExpectedEchoes:306-321` no longer appends the terminator; `:300-304` carries a comment
forbidding its re-addition; the terminator is reported at `:534-550` as `TerminatorEcho` and is
**structurally unable to reach `$result.Ok`** (L2.1 #6). `Test-DispatchEcho:552` handles a
zero-length expected list with a named reason.

⭐ **The author asked me to confirm that `CONVENTIONS.md:4457` is now true of this file. ✅ IT IS.**
That sentence reads *"FOR EACH **NON-TERMINATOR** COMMAND YOU DISPATCHED, ASSERT THE LOG
CONTAINS A `Cmd:` LINE"* — which is now a literally accurate description of `:596-619` operating
on a `$Expected` list from which the terminator has been dropped at `:248`. ⇒ ⛔ **Loop 1 §9
item 1 is DISCHARGED. It must NOT be routed to the manager, and I am striking it here so that a
future reader of loop 1 does not re-route a claim that the code has already made true.**
✅ `QUIT_EDITOR` is still **dispatched** — `New-ExecCmdsValue:288` appends `$script:Terminator`.
The exemption is from the **echo requirement**, never from the dispatch.

### ✅ **B-2 — CLOSED, AND CLOSED STRUCTURALLY. This is the strongest fix in the rev.**

The census at L2.1 #1-#4 is the whole argument and it needs no execution: **one `exit`, two
`Show-Verdict` call sites each followed by `return <same>.Code`, and `Merge-BoundIntoVerdict:807`
overwriting `Code` before either is reached.** ⛔ **rev-1's failure is no longer expressible in
this file** — there is no second place for a code to be computed and no second place for a
verdict to be printed. PROOF A/B are corroboration; the census is the proof.

**On the row's question — is the demotion legible to a machine AND to a skimming human?**

- ⭐ **MACHINE: YES, unambiguously.** `RUNNER_EXIT=6` (`:1344`, from `$code`), plus
  `RUNNER_BOUND=<message>` and `RUNNER_LOG_VERDICT=<n>` emitted **only when a bound tripped**
  (`:1347-1350`). The log's opinion is behind a differently-named key that does not exist on a
  clean run, so a parser cannot confuse the two.
- ✅ **HUMAN: YES — the headline is `NO` and the last line is `exit 6`, both in red** (`:1333-1338`),
  with the log's opinion prefixed `log verdict (info)` in DarkYellow (`:1330`) and the verdict
  sentence itself carrying *"The process was FORCE-KILLED, so nothing it left behind is a result
  (SC-114). For information only…"* (`:808-811`). The word `green` survives on screen **only
  inside an explicitly-labelled info line.** Against rev-1's `OK … exit 0`, this is a different
  document.
- ⚠️ **BUT NOT COMPLETELY — see `W-15`.** The **counts block is not demoted.** `N (started) 20`,
  `completed 20`, `N / M 20 / 0` print in Gray at `:1315-1317`, in the same position and the same
  colour as on a healthy run, with no partial marker; and porcelain emits `RUNNER_STARTED` /
  `COMPLETED` / `SUCCESS` / `FAIL` / `SKIPPED` unqualified at `:1357-1361`. **B-2 demoted the
  verdict; it did not demote the numbers.** Warning, not a blocker: the exit code and the
  headline are both correct and unmissable.

### ✅ **B-3 — CLOSED, and (a) is closed against a corpus I MEASURED.**

**(a)** 0 → **8 blank lines across 4 of 10 fixtures**, mid-log and at EOF (L2.1 #8), plus §8
driving `Get-CmdEchoes` / `Test-SuiteCounts` / `Test-DispatchEcho` directly with
`@('', 'Cmd: X', '', '   ', …)` and `@()` at `:1231-1266`, plus the `try/catch` at `:988-995`
that **reds** a binding failure instead of aborting the run. ⭐ **The regression loop 1 said was
absent now exists, and I confirmed it rather than accepting it.**
**(b)+(c)** `New-ExecCmdsValue` and `New-EditorCommandLine` are now invoked, by **case-sensitive**
equality on both lane values, plus `-is [string]`, plus the quoted-token check (L2.1 #11).
⭐ **`R2.7d` is the receipt: the string-equality cases loop 1 ordered caught a `,$clean`
over-correction that would have emitted `-ExecCmds="System.Object[],QUIT_EDITOR"`.** A prescribed
remedy that catches a defect **during** its own implementation is the best possible evidence that
the prescription was right (`SC-§101`, satisfied rather than merely asserted).

---

## L2.5 ⚖️ RE-RULING (b) — MUTATION ADEQUACY: **⛔ NOT ADEQUATE → ✅ ADEQUATE**

Loop 1 ruled the set inadequate: 3 mutants, all inside `Get-LogVerdict`, three probes into one of
four surfaces. **I reverse that ruling.** 16 mutants now span all four surfaces, and — the part
that matters more than the count — **for 7 of my 8 named rows the covering assertions are in the
file where I can read them, so their coverage does not depend on believing that any run happened.**

| # | my loop-1 failure mode | mutant | covering assertion **I verified exists** | ruling |
|---|---|---|---|---|
| 1 | suite `;` swept to `,` | M4 | `:1072` `-cne 'Automation RunTests Siegebound;Quit'` · `:1077` · `:1155` `-cnotmatch` on the quoted token | ✅ **COVERED — verified** |
| 2 | `QUIT_EDITOR` reverted to `Quit` | M5 | `:1082` `-cne 'A,B,QUIT_EDITOR'` · `:1087` `-cne 'A,QUIT_EDITOR'` · `:1176` | ✅ **COVERED — verified** |
| 3 | caller-separator rejection deleted (**exit 64 had no case at all**) | M6 | `:1090`, `:1093` (emit) + `:1128`, `:1131` (judge) — 4 throw cases | ✅ **COVERED — verified** |
| 4 | `-ArgumentList` back to an ARRAY | M7 | `:1148` `-is [string]` + `:1155` verbatim quoted token + `:1163-1166` | ✅ **COVERED — verified** |
| 5 | `[AllowEmptyString()]` removed | M8 | 4 blank-line fixtures + `:1236`, `:1249`, `:1280` — against a corpus I measured at 8 blank lines | ✅ **COVERED — verified** (tally quibble, L2.6) |
| 6 | bound message `-f`/`+` regression | M9 | `:1049` placeholder guard + `:1053` `Like` on all 3 tripping cases | ✅ **COVERED — verified** |
| 7 | ⭐ **the kill fails / the process survives** | M16 | **none — covered only by execution against a live process** | ⚠️ **ACCEPTED AS DECLARED.** The *probe* at `:1555-1566` I verified by reading and it is correct; the *run* that exposed the broken one I cannot see. ⇒ `TASK-1183` L2.9 #19. |
| 8 | `Result={Skipped}` misdiagnosed | M10 | `skipped-suite.log`, whose 18 + 2 composition I counted | ✅ **COVERED — verified** |

**And the two gaps loop 1 named separately are closed:**
- **exit 64** had *"neither fixture nor mutant"* ⇒ now **16 cases**: 8 throws in §3, 3 in §4, 5 negative path cases in §7.
- **exit 6** had *"neither fixture nor mutant"* ⇒ now **2 pure unit cases** at `:1186-1202` driving `Merge-BoundIntoVerdict` with a **green** log + a tripped bound and asserting `Code == 6`, `LogVerdictCode == 0`, `BOUND EXCEEDED`, `FORCE-KILLED`, no placeholder — **all readable, all real** — plus the declared live proof.

### ⚖️ IS THE RESIDUAL HONESTLY DRAWN, OR CONVENIENTLY DRAWN? — **RULING: HONESTLY DRAWN.**

The four residuals (a real editor resisting `Stop-Process`; real orphan children; a genuinely
incremental boot write; a real `$proc.ExitCode`, plus a spaced `-abslog`) each name a **specific
mechanism** and the **reason the engine is required**, and each points at `TASK-1183` §7 #10.
None of them conceals a failure mode that would flatter the tool.

⭐ **The decisive evidence is `R2.7b` itself.** A conveniently-drawn residual list does not
contain its author's most damaging finding, disclosed unprompted, kept alive as a permanent
mutant, and placed **first** in his own "what to scrutinise" list — a finding whose consequence
he states against himself: *"had this shipped, `TASK-1183`'s very first bound run would have
reported a failed kill on a successful one."* That is the opposite of convenience. `R2.7a`,
`R2.7c` and `R2.7d` are three more of the same, and `R2.7c` is an admission that **his own
headline lesson from rev-1 recurred in the code he wrote to fix it.**

⚠️ **Two omissions, neither of them convenient, both routed to L2.9 rather than to the verdict:**
1. **W-8's already-running-editor branch (`:1459-1467`) has never fired** — it is named in
   R2.5's table (*"no editor was running to trigger it"*) but **not** in R2.4's residual list.
   ⭐ It **will** fire on `TASK-1183` if the art lane's editor is still up, and an operator who
   has not read R2.5 will meet an unexplained `!!` banner on the tool's first outing.
2. **The OVERALL bound has never tripped against a process.** BOOT and STALL each have a declared
   stand-in proof; OVERALL is arithmetic-only. All three share one kill path, so the risk is low —
   but *"no bound has killed a real editor"* is a broader sentence than the evidence, and
   *"OVERALL has never killed anything at all"* is the precise one.

### ⚖️ DOES ONE SHARED VALIDATOR CLOSE THE CLASS, OR JUST THIS INSTANCE?

**It closes the class for every input that exists today, and it installs the regression that
keeps it closed — but it closes it by CONVENTION, not by CONSTRUCTION.**

I enumerated every caller-supplied input and checked both halves:
`$Filter` → `Assert-SafeFilter` from **`:278` (emit)** and **`:316` (judge)** · `$Commands` →
`Get-CleanCommandList` from **`:283` (emit)** and **`:320` (judge)** · `$Mode` → `ValidateSet` on
both. ⭐ **And I found a THIRD consumer the handoff does not mention: the boot detector at
`:1492` calls `Get-ExpectedEchoes`, so it inherits the same validation** — and it runs after
`New-ExecCmdsValue:1419` has already refused bad input with 64. **Nothing is left asymmetric.**
§4's three symmetry throw-cases (`:1128`, `:1131`, `:1134`) are the regression.

⛔ **What it does not do is make a FOURTH entry point structurally unable to skip the validator.**
The guarantee is three assertions, not a type. That is the correct level of claim, it matches the
author's own framing, and I record it so *"one shared validator"* is never quoted as a stronger
guarantee than it is.

**Same verdict on the `5b` placeholder class:** verdict messages (`:1009`), bound messages
(`:1049`) and now throws (`:904`) are all checked — but **a fourth emitter is not**, and that is
`W-14`. ⇒ ⭐ **The author's own rule — *"a guard class is not closed until every site that can
emit that class is checked"* — is correct, and it is still not fully applied. Three of four
organs is not the family.**

---

## L2.6 ⚠️ TWO NUMBERS I COULD NOT RECONCILE (this is L2.0's converse, doing its job)

**The kills are not in doubt** — any tally below 52 is a kill, and every mutant reports one.
**The evidence table is what has the defect**, and the row told me to judge executed evidence.

1. ⚠️ **M8 — claimed `44 / 52`, i.e. 8 red cases.** My derivation finds **7**: the 4 blank-line
   fixtures (`green-suite`, `green-commands`, `skipped-suite`, `no-terminator-echo`) plus §8's
   `Get-CmdEchoes`-blank-lines (`:1236`), `Test-SuiteCounts`-blank-lines (`:1249`) and the W-10
   reader (`:1280`, whose `-split` at `:476` yields a trailing `''`). **I cannot locate an 8th.**
   Either the mutation touched a 4th parameter site or the tally is off by one.
2. ⛔ **M13 — claimed `51 / 52`, i.e. ONE red case, for a mutation described as *"the `-LogPath`
   guard always says Ok"*. §7 contains FIVE negative path cases** (`.uasset`, `.txt`,
   `C:\Windows\Temp`, `Saved\..\Content`, empty) at `:1212-1216`. A guard that always returns
   `Ok = $true` must red **all five** ⇒ `47 / 52`. **`51 / 52` is only consistent with a mutation
   far narrower than its description** — and the narrow reading matters, because if the mutation
   were applied at the **call site** (`:1411`, bypassing the guard) rather than **inside** the
   function, `-SelfTest` would not detect it *at all* and W-3's regression would be weaker than
   the table claims. ✅ **I verified the shipped guard is real, pure and correctly ordered by
   reading it (L2.1 #14), so the CODE is fine.** ⛔ **The DESCRIPTION and the TALLY disagree, and
   one of them is wrong.**

⇒ ⚖️ **Recorded, not escalated.** Neither changes the adequacy ruling, because rows 1-6 and 8 are
carried by assertions I read in the file rather than by mutant tallies. But **13 of 16 tallies
reconcile with my arithmetic and 2 do not** (M4 = 3 red ✓, M5 = 3 ✓, M6 = 4 ✓, M7 = 4 ✓, M11 = 2 ✓,
M15 = 4 ✓, the single-case ones ✓), and a reader quoting *"16 applied, 16 killed"* should know
that two of the supporting figures do not survive re-derivation.

---

## L2.7 ✅ THE `;Quit` CENSUS — RECONCILED, THREE WAYS, AND IT EXONERATES

| measurement | figure | when |
|---|---|---|
| rev-1 handoff §4 | `51 / 32` | before loop 1 |
| **my loop 1** | `53 / 33` | rev-1's handoff had added 2 in 1 file |
| rev-2 handoff `R2.9` #4 | `55 / 34` | |
| ⭐ **my loop 2, measured now** | **`58 / 34`** | |

**The arithmetic closes exactly:** `53` + **3** (rev-2's appendix grew `TASK-1181-programmer.md`
from 2 → **5**) + **2** (`qa/TASK-1182-report.md`, my own loop-1 report, a **new file**) = **58**;
`33` + 1 (my report) = **34**. ⇒ ⭐ **The author's `55` under-counts his own rev-2 appendix by 3**
— he applied *"+2 for QA's report, +2 for my note"* to the **rev-1 baseline of 51** instead of
re-running the census after writing the appendix. **`51 + 2 + 2 = 55`. The census he reports is a
computation, not a measurement.**

⇒ ✅ **Harmless, and it exonerates him on the thing that matters.** Every delta across all four
measurements is a **document about the semicolon**; the count has only ever risen; and the
substantive sites are intact (`CONVENTIONS.md` = 3, `TASKBOARD.md` = 13, and 30 handoff/qa files
at 1-4 each). **No prose site was swept, and `run_suite_bounded.ps1:280` still emits the `;`.**
⚠️ ⭐ **But it is `SC-§95` cl. 2 in miniature, inside a note whose subject is `SC-§95`: a figure
derived from an older figure is a CLAIM, not a measurement, and it should carry the verb that
produced it.** Cheap to fix, worth naming, not worth a warning of its own.

---

## L2.8 ⚖️ THE RULINGS THE ROW ASKED FOR

### **(i) Does `suite-1167rev2.log` being untracked BLOCK? — ⛔ NO.**
`Saved/` is gitignored at **`.gitignore:111`**, by UE convention and correctly — a 554-test log is
build output. ⛔ **The remedy is NOT to track it.** Three reasons this does not block:
1. **The defect it uniquely caught now has a TRACKED regression.** Loop 1's objection was
   *"the tracked fix has no tracked regression"* — that was true at 0 blank lines and is **false
   at 8** (L2.1 #8). The corpus now tests the INPUT, not only the logic.
2. **4 of 10 fixtures are truncated slices of that very log**, so its shape is in the repo even
   though its bytes are not.
3. ⭐ **`TASK-1183` run #16 produces a fresh real log and becomes the successor control** — which
   is why L2.9 #16 requires its path to be named and its header quoted into the handoff.
⇒ **Not a blocker. The duty transfers.**

### **(ii) Loop 1's ruling (a) — shipping an unexecuted launch path — ✅ UNCHANGED: SHIP IT.**
All four reasons hold and two are now stronger: the launch path is proven against a **stand-in**
(strictly more than rev-1 had, strictly less than a real outing, and the header at `:60-69` says
exactly that), and the one repo-damaging line — the `Remove-Item` — is now **guarded** (W-3).
⛔ **`TL-§6`'s `NOT MEASURED` bullet at `CONVENTIONS.md:5966-5969` STAYS until `TASK-1183`
reports, and only the manager may strike it (`SC-§82`).**

### **(iii) The exit-code change at `-VerifyLog` — 2 → 64 on an embedded separator. ✅ I AGREE.**
The author asked me to say if I disagreed. **I do not.** `64` says *"your invocation is wrong"*;
`2` said *"the engine never dispatched your command"* — and R2.7a proves the old behaviour
asserted the second while the first was true, **on a perfectly good log.** A false red that
blames the engine for a CLI mistake is exactly the wrong-cause failure `TL-§5b` 2c calls more
dangerous than no explanation at all. **The change is a correction, not a regression.**

---

## L2.9 ⭐⭐ WHAT `TASK-1183` MUST OBSERVE — **THIS LIST SUPERSEDES LOOP 1 §7**

⛔ **I hold no `Bash` and I ran none of this.** ⭐ **`TASK-1183` remains this script's FIRST REAL
OUTING against the engine.** ⚠️ Here `$LASTEXITCODE` **is** the right thing to read — it is *this
script's own* log-derived verdict; the never-trust law binds `Build.bat` and the **editor**, not
the runner.

### A. No editor needed — do these FIRST, they are cheap and they gate the rest

| # | command | expect | quote / check |
|---|---|---|---|
| 1 | `-SelfTest` | **0**, ⭐ **`52 / 52`** | ⛔ **Quote the total. I derived 52 from the file** (§1=11 §2=5 §3=12 §4=5 §5=4 §6=2 §7=7 §8=6). **A total that is not 52 means the file changed under this gate — stop and say so.** |
| 2 | `-VerifyLog …\green-suite.log` | **0** | |
| 3 | `-VerifyLog …\red-suite.log` | **5** | a red suite reads red |
| 4 | `-VerifyLog …\skipped-suite.log` | **0** | ⛔ the reason must carry **`[2 skipped, 0 not-run]`** (W-2's regression) |
| 5 | `-VerifyLog …\zero-started.log` | **2** | `SC-§95` cl. 1, named by cause |
| 6 | `-VerifyLog …\zero-started-filtered.log` | **3** | the `N == 0` path in isolation |
| 7 | `-VerifyLog …\result-absent.log` | **4** | |
| 8 | `-VerifyLog …\count-mismatch.log` | **8** | |
| 9 | `-Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Status -VerifyLog …\w9-cmd-semicolon.log` | **2** | must name **`SC-116 W9`** |
| 10 | `-Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status -VerifyLog …\green-commands.log -Porcelain` | **0** | `RUNNER_TERMINATOR_ECHO=… seen` |
| 11 | ⭐ **B-1's tracked regression:** same, against `…\no-terminator-echo.log` | **0** | `RUNNER_TERMINATOR_ECHO=… NOT SEEN`. ⛔ **#10 and #11 must BOTH be 0. If #11 is not 0, B-1 has reopened — stop.** |
| 12 | `-VerifyLog` an absent path | **7** | a null is not a result |
| 13 | ⭐ **W-3, and it costs nothing:** `-LogPath Content\Meshes\SM_Rock_01.uasset` (**no** `-VerifyLog`, **no** `-DryRun`) | **64** | ⛔ **then confirm the `.uasset` STILL EXISTS.** This is the one line that can damage the repo. |
| 14 | `-DryRun` **and** `-Mode Command … -DryRun` | **0** each | ⛔ **paste BOTH lines verbatim.** Suite MUST carry `;Quit`; Command MUST end `,QUIT_EDITOR`. Check character-for-character against `SC-§116` cl. 2. |
| 15 | ⭐ **R2.7a's refusal:** `powershell -File .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C -VerifyLog <any fixture>` | **64**, not 2 | a deliberate behaviour change from rev-1; I want it seen firing |

### B. Editor needed — ⛔ **AN ART LANE HAS BEEN USING THE EDITOR AND 🧑 JONATHAN IS ASLEEP**

⛔ **Check `Get-Process UnrealEditor*` before every run below. NEVER kill an interactive
instance to make room.** ⚠️ **W-8's `!! AN EDITOR IS ALREADY RUNNING` banner (`:1459-1467`) has
NEVER FIRED — if you see it, that is the guard working, not a defect.**

| # | run | expect | ⛔ what to quote |
|---|---|---|---|
| 16 | ⭐⭐ **the first real suite run:** `-Porcelain` | **0** on a green tree | the **MEASURED `N / M` THROUGH THE SCRIPT** · `RUNNER_ECHO_MATCHED/EXPECTED` · `RUNNER_STARTED/COMPLETED/SUCCESS/FAIL/SKIPPED` · `discovered` · wall clock · **whether the number matches the hand-typed lane** · ⛔ **and NAME THE LOG'S PATH — it is the successor to the untracked `suite-1167rev2.log` (L2.8 i).** |
| 17 | — | — | ⚠️ **If #16 trips a STALL at 180 s on a HEALTHY suite, that is BOUND TUNING, not a tool defect.** Say which. ⛔ **Do not loop QA on it.** |
| 18 | ⭐ **the command lane's first real dispatch:** `-Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status -Porcelain` | **0**, self-terminating ≈ **12 s** | **does the log carry `Cmd: QUIT_EDITOR`? Quote the line or state its absence.** ⭐ **EITHER ANSWER IS NOW SAFE — B-1 is fixed, so this is data for the law, not a gate.** |
| 19 | ⭐⭐⭐ **MAKE A BOUND KILL A REAL EDITOR:** a real launch with `-BootSeconds 20 -OverallSeconds 45 -StallSeconds 15 -Porcelain` | **6** | ⛔⛔ **THE ONE THING 52 CASES AND 16 MUTANTS CANNOT BUY.** Quote: the `BOUND TRIPPED:` line · ⭐ **the survivor probe's verdict — `PID N confirmed gone (HasExited).` or `!! PID N SURVIVED`** — this is **R2.7b's corrected check on its first outing against a process that can actually resist** · any orphan line · ⛔ **and INDEPENDENTLY confirm with `Get-Process -Id <pid>` that it is gone. Do not take the script's word for its own kill.** |
| 20 | — | — | ⭐ **B-2 live check, on run #19: report BOTH `$LASTEXITCODE` AND the porcelain `RUNNER_EXIT=` and SAY WHETHER THEY AGREE.** ⛔ **They now should — one `exit`, one variable. A disagreement reopens B-2.** |
| 21 | — | — | ⚠️ **On #19 also quote `log verdict (info)`, `RUNNER_BOUND=` and `RUNNER_LOG_VERDICT=`, and say plainly whether a skimming reader could still mistake the output for green** (`W-15`). |
| 22 | — | — | `cat` the `.cmdline` sidecar; confirm it matches what was launched **and that its first character is `C`, not a BOM** (`N-1`). |

### C. Standing constraints on this row

- ⛔ **`-LogPath` stays at the DEFAULT for every run except #13.**
- ⛔ **Do NOT strike `TL-§6`'s `NOT MEASURED` bullet, and do NOT correct the three
  *"THIS FILE DOES NOT EXIST"* sites** (`CONVENTIONS.md:4895`, `CONVENTIONS.md:5961`,
  `TASKBOARD.md:1735`). **They are the manager's (`SC-§82`) — report them, do not edit them.**
  Same for the script's own `!!! SANCTIONED IS NOT EXERCISED` block at `:60-69`: it says
  *"DELETE THIS BLOCK ONLY THEN"*, and *then* means **after your handoff lands**, by the row that
  owns `Tools/`. **Not by you, and not in the same action as the run.**
- ⚠️ ⭐ **`TASKBOARD.md` is being edited concurrently by an art lane. Re-read it immediately
  before you stage it, stage by pathspec, and verify the COMMIT (`git show --stat HEAD`), never
  the index.** ⛔ `SC-§102`: **the git root is ONE LEVEL UP, and a mis-anchored pathspec answers
  with SILENCE.**
- ⚠️ `SC-§103`: this commit owes **N** status flips — **`TASK-1181`, `TASK-1182`, `TASK-1183`**
  all flip in the same action.
- ⭐ **ORDER: run A (1-15) → run B (16-22) → THEN commit.** The handoff must carry the evidence.
  ⛔ **If any run in B exposes a defect, that is routing rule 6 — back to `gameplay-programmer`,
  do NOT commit a script whose first outing failed.**

---

## L2.10 ⚠️ WARNINGS — all five are NEW; loop 1's W-1…W-12 are all closed (L2.11)

- **[W-13]** `run_suite_bounded.ps1:1264` — ⭐⭐ **A VACUOUS SUB-ASSERTION, AND IT IS THE FOURTH
  NULL-READING DEFECT OF THIS SESSION — INSIDE THE CASE THAT TESTS B-1.**
  ```powershell
  if ($de2.TerminatorEcho -notlike '*seen*') { $ok = $false; $why += 'a present terminator was not corroborated' }
  ```
  PowerShell's `-like` is **case-insensitive**, and the negative string is
  `'QUIT_EDITOR NOT SEEN (corroboration only, NOT a verdict …)'` — which **matches `'*seen*'`**,
  because `SEEN` is a substring of `NOT SEEN`. ⇒ ⛔ **This assertion passes whether the terminator
  is reported `seen` or `NOT SEEN`. It cannot fail in the direction it was written to fail.**
  ⚖️ **NOT a blocker:** the case also asserts `$de2.Ok` and `$de2.Expected -eq 2` at `:1262-1263`,
  which are real and which carry B-1's core; and the `:1255` sibling (`-notlike '*NOT SEEN*'`)
  is sound. **Only the corroboration half is dead.**
  **FIX:** `-cnotlike '*QUIT_EDITOR seen*'`, or assert `$de2.TerminatorEcho -notlike '*NOT SEEN*'`.
  ⭐ **The general form, which is the finding: a substring assertion whose needle is a substring
  of its own negation (`SEEN` ⊂ `NOT SEEN`) makes the negative reading unreachable.** Routed to
  the manager as a candidate clause alongside L2.2's probe rule.
- **[W-14]** `run_suite_bounded.ps1:1218-1224` vs `:385-387` / `:391-392` — **the `5b` class is
  closed in three organs and open in a fourth.** §7's `-LogPath` cases assert **only
  `$res.Ok -eq $pc.Want`**; `$res.Reason` is never placeholder-checked and never text-asserted.
  `Resolve-SafeLogPath` builds its refusal messages with **multi-fragment `-f`** — defect 5b's
  exact shape. They are correctly parenthesised **today**. ⇒ **a 5b regression there prints a
  literal `{0}` to the operator being refused, and `-SelfTest` still reports `52 / 52`.**
  **FIX:** run the `\{\d+\}` guard over `$res.Reason` and give each negative case a `ReasonLike`.
- **[W-15]** `run_suite_bounded.ps1:1313-1324` + `:1356-1362` — **B-2 demoted the verdict but not
  the NUMBERS.** On a bound-killed run `N (started)`, `completed`, `N / M 20 / 0` print in Gray at
  the same position and colour as on a healthy run with no partial marker, and porcelain emits
  `RUNNER_STARTED/COMPLETED/SUCCESS/FAIL/SKIPPED` unqualified beside `RUNNER_BOUND`. In a project
  whose entire failure history is people lifting numbers out of logs, **`20 / 0` quoted out of a
  force-killed run into a handoff is the next instance of the defect this tool exists to prevent.**
  **FIX:** when `BoundMessage -ne ''`, label the block *"partial — FORCE-KILLED, not a result"*
  and emit `RUNNER_PARTIAL=1`.
- **[W-16]** `run_suite_bounded.ps1:1340-1363` vs `:1593-1595` — **`$Verdict.Reason` is never
  emitted in porcelain, and neither is `$killSurvived`.** The kill-failure sentence
  (*" !! AND THE KILL FAILED: PID N survived Stop-Process -Force."*) is appended to `Reason` at
  `:1594` and reaches the **console only**. ⇒ **a calling agent gets `RUNNER_EXIT=6` with no
  machine-readable indication that a live engine process is still holding the log it is about to
  read.** Not a false green (6 is correct), but the documented contract at `:71-75` promises a
  parsable summary and omits the one string that carries the cause.
  **FIX:** `RUNNER_KILL_SURVIVED=1` and a one-line `RUNNER_REASON=`.
- **[W-17]** `run_suite_bounded.ps1:71-75` (`.NOTES`) vs `:1381`, `:1414`, `:1422`, `:1444`,
  `:1449`, `:1453`, `:1375` — **porcelain is absent ENTIRELY on the USAGE (64), `-DryRun` (0) and
  `-SelfTest` (0 / 5) paths**, because `Show-Verdict` is the only emitter and none of them call
  it. The contract says *"RUNNER_EXIT is the PROCESS EXIT CODE"* without saying it is **not
  printed at all** on those paths. ⇒ **a caller that greps `RUNNER_EXIT=` and finds nothing must
  treat absence as UNKNOWN, never as 0.** **FIX:** one sentence of doc, or emit `RUNNER_EXIT=` on
  every return.

## L2.11 ✅ LOOP 1's WARNINGS AND NITS — ALL 12 + 4 CLOSED, EACH VERIFIED BY READING

`W-1` header caveat ✅ `:6-7`, `:60-69` · `W-2` generic `Result={}` ✅ `:656`, `:698` + fixture ·
`W-3` `-LogPath` confinement ✅ `:357-399` called at `:1411`, 62 lines before the `Remove-Item` ·
`W-4` `-Filter` guard ✅ `Assert-SafeFilter:210-222`, 4 cases · `W-5` bound messages asserted ✅
`:1049-1056` · `W-6` success wording ✅ `:624` now *"matched by CONTENT; extra engine Cmd: lines
ignored, terminator exempt"* · `W-7` kill confirmation ✅ `:1555-1566`, **and corrected twice** ·
`W-8` running-editor warning ✅ `:1459-1467` (⚠️ never fired — L2.9 B) · `W-9` command-lane
wording ✅ `:771` + `RUNNER_MODE` `:1345` · `W-10` incremental reader ✅ `Read-LogSince:426-478`,
**and it is better than I asked for — it never consumes a partial trailing line (`:468-475`) and
rewinds on truncation (`:443`)** · `W-11` top-level `try/catch` → documented **70** ✅
`:1601-1615`, with exit **1** honestly documented as *"NOT ours"* at `:92-95` · `W-12`
`green-commands.log` provenance ✅ declared in `R2.5` ·
`N-1` ASCII sidecar ✅ `:1478` · `N-2` `$Expected` attributes ✅ `:524` · `N-3` one ledger entry
per case ✅ `Add-SelfTestCase:867-886` · `N-4` `$ProjectRoot` + the `SC-§102` comment ✅ `:178-181`.

## L2.12 NITS (new)

- **[N-5]** `:1478` — nothing ever deletes a stale `.cmdline` sidecar. A failed launch leaves a
  sidecar describing a run that did not happen, beside no log.
- **[N-6]** `:926` — a missing **fixture directory** returns `7` (`LOG_UNREADABLE`), documented as
  *"no log, empty log, or log could not be read"*. A missing corpus is an environment/usage fault.
  It is still a NO, so no false green; `64` would be truer.
- **[N-7]** `:1481` `-WindowStyle Hidden` — if the editor raises a **modal dialog**, the operator
  sees nothing and the run looks like a stall until a bound kills it. That is **correct**
  behaviour, but `TASK-1183`'s operator should expect it rather than diagnose a tool defect.
- **[N-8]** `Read-LogLines:409` uses `Get-Content` with **no `-Encoding`** while
  `Read-LogSince:454` decodes explicitly as UTF-8. Immaterial for ASCII UE logs; an inconsistency
  between the boot peek and the final read.

## L2.13 📮 ROUTED TO THE MANAGER (`SC-§82`; I may not edit `CONVENTIONS.md`)

1. ✅ ⛔ **STRUCK — loop 1 §9 item 1 (`CONVENTIONS.md:4457`) is DISCHARGED BY CODE.** The runner
   now implements the terminator exemption; the sentence is true as written. **Do not action it.**
2. ⛔ **STILL OPEN, third time of asking:** `CONVENTIONS.md:4895`, `CONVENTIONS.md:5961`
   (`TL-§6` row 1) and `TASKBOARD.md:1735` all still read **"DOES NOT EXIST"** of a 1616-line file
   I have now read twice. **Three sites** (`SC-§95` cl. 2's propagation duty).
3. ✅ **`CONVENTIONS.md:5966-5969` (`NOT MEASURED`) is CORRECT AS WRITTEN and must NOT be struck
   by `TASK-1183`'s host** — only by the manager, and only after L2.9 items #16, #18 and #19.
4. ⭐ **Candidate clause, bought four times this session (L2.2 + W-13):** *an instrument's POSITIVE
   reading is exercised constantly and its NEGATIVE reading is exercised once — in the incident —
   so the negative path is always the untested half.* Corollaries measured here: `Get-Process -Id`
   resolves a **dead** PID while `Start-Process -PassThru` holds a handle; and a `-like` needle
   that is a substring of its own negation (`SEEN` ⊂ `NOT SEEN`) can never read false.
5. ⭐ **`SC-§116` cl. 2's recipe should record that `powershell -File` CANNOT pass a multi-command
   `-Commands` list** — it arrives as one comma-joined string (R2.7a). The runner now refuses it
   with instructions; the law's recipe does not warn about it. **Corroborated, re-routed.**
6. ℹ️ **Census: my measurement is `58 / 34`** (L2.7), not the handoff's `55 / 34` — the handoff's
   figure is **computed from an older figure rather than measured**, and under-counts its own
   appendix by 3. **No sweep occurred. Nothing to action beyond the number.**

## L2.14 NOTES FOR BUILD-MASTER (`TASK-1183`)

✅ **This is a PASS. `TASK-1181` is clear to run and clear to commit** — in that order.
**L2.9 supersedes loop 1 §7 and is the acceptance**, and within it **#16, #18 and #19 are the
three that cannot be bought any other way.** ⭐ **#19 is the whole of what this gate could not
give you: 52 self-test cases and 16 mutants do not prove that a bound can kill a real editor, and
the survivor probe you will be reading was WRONG THREE DAYS' WORTH OF REASONING AGO — it is on
its first outing against a process that can resist.** ⛔ Staging list is on the board at
`TASKBOARD.md:1788`; the concurrency, pathspec and `SC-§103` constraints are at L2.9 C.

---
---

# ⭐⭐ LOOP 3 — APPENDED BY **build-master** (`TASK-1183`), 2026-09-09, under **routing rule 6**
# **THE EXECUTION DUTY IS DISCHARGED. ⛔ VERDICT ON `TASK-1181` rev-2: `FAILED ON FIRST OUTING` — 1 BLOCKER, found by RUN #19.**

⛔ **Everything above this line is the qa-reviewer's and is preserved byte-for-byte.** This section is
the build-master's, and it exists because `L2.9` transferred the execution duty by name and `L2.14`
said *"#19 is the whole of what this gate could not give you."* **It gave it. Here is what it said.**

⚖️ **THE GATE'S PASS WAS CORRECT ON THE READING AND I AM NOT OVERTURNING IT.** `L2.1`'s 17 verified
claims all held when executed. The defect below is in the ONE organ the reviewer explicitly said she
could not reach: *"the survivor probe you will be reading was WRONG THREE DAYS' WORTH OF REASONING
AGO — it is on its first outing against a process that can resist."*

## L3.1 ✅ THE RUN TABLE — 15 no-editor + 7 editor, OBSERVED vs EXPECTED

| # | run | expected | **observed** | |
|---|---|---|---|---|
| 1 | `-SelfTest` | 0, `52 / 52` | **0, `52 / 52`** | ✅ QA's hand-derived 52 confirmed ⇒ the file did not move under its own gate |
| 2 | `green-suite.log` | 0 | **0** | ✅ |
| 3 | `red-suite.log` | 5 | **5** | ✅ |
| 4 | `skipped-suite.log` | 0 + `[2 skipped, 0 not-run]` | **0**, reason carried **`[2 skipped, 0 not-run]`** | ✅ W-2's regression is real |
| 5 | `zero-started.log` | 2 | **2**, named `SC-95 cl. 1` | ✅ |
| 6 | `zero-started-filtered.log` | 3 | **3** | ✅ |
| 7 | `result-absent.log` | 4 | **4** | ✅ |
| 8 | `count-mismatch.log` | 8 | **8** | ✅ |
| 9 | `w9-cmd-semicolon.log` | 2, names `SC-116 W9` | **2**, named | ✅ |
| 10 | `green-commands.log` | 0, terminator **seen** | **0**, `RUNNER_TERMINATOR_ECHO=… seen` | ✅ |
| 11 | `no-terminator-echo.log` | 0, terminator **NOT SEEN** | **0**, `… NOT SEEN` | ✅ **B-1 STAYS CLOSED** |
| 12 | absent path | 7 | **7** | ✅ |
| 13 | `-LogPath` a `.uasset` | 64, asset intact | **64** ×2, `SM_Archer.uasset` sha256 **UNMOVED** | ✅ W-3 holds. ⚠️ QA's literal `SM_Rock_01.uasset` **does not exist on disk**; I ran a **real** asset as well, so the control is live rather than vacuous |
| 14 | both `-DryRun` | 0 each | **0 / 0**, both lines **character-for-character** equal to `CONVENTIONS.md:4443-4444` | ✅ suite keeps `;Quit`, command ends `,QUIT_EDITOR` |
| 15 | `-File … -Commands A,B,C` | **64**, not 2 | **64** | ✅ R2.7a's refusal seen firing |
| **16** | first real suite, `-Porcelain` | 0 | **0, `555 / 555`**, 42 s, echo `1/1`, discovered 555 | ✅ **MATCHES the hand-typed lane's `555 / 555` at `4a3da63`** |
| 17 | — | — | no STALL trip on a healthy suite | ✅ bounds correctly tuned for a 42 s suite |
| **18** | command lane, first dispatch | 0, ≈12 s | **0, 12 s exactly**, echo `3/3` | ✅ |
| **19** | bound must kill | **6** | **6** — but see **L3.2** | ⛔ **exit correct, GUARD DEFECTIVE** |
| 20 | B-2 live check | agree | `$LASTEXITCODE=6` **and** `RUNNER_EXIT=6` | ✅ **B-2 CLOSED BY EXECUTION** |
| 21 | W-15 skim risk | — | `RUNNER_BOUND=` + `RUNNER_LOG_VERDICT=8` present and labelled information-only | ✅ a skimmer could **not** read it as green |
| 22 | `.cmdline` sidecar + BOM | first char `C` | **NOT CHECKED** — see L3.4 | ⚠️ |

⭐ **#18 ANSWERS B-1 BY OBSERVATION, AS ORDERED. The log the run produced DOES carry the echo:**

    [2026.09.09-11.53.35:385][  1]Cmd: QUIT_EDITOR

⛔ **And it carries SIX `Cmd:` lines for THREE commands** (`MAP LOAD`, `MAP CHECKDEP`, our three, the
terminator) ⇒ **cl. 4(c)(i)'s claim that a line-counting guard fails a good run is now MEASURED in the
COMMAND lane too, not only in the suite lane.**

⚠️ **#19 as prescribed (`-OverallSeconds 45`) did NOT trip:** the suite finished in **44 s**, one
second inside the bound, returning a genuine `555 / 555` exit 0 — incidentally a second independent
confirmation of the suite number. **That is bound tuning, not a tool defect** (`L2.9` #17, inverted).
I re-ran at `-OverallSeconds 20`, which must trip mid-run, and it did.

## L3.2 ⛔ BLOCKER — `run_suite_bounded.ps1:1567` — **the orphan census uses the reader its own comment eight lines above declares WRONG, and it is wrong in BOTH directions on the SAME run**

    $orphans = @(Get-Process -Name 'UnrealEditor*', 'ShaderCompileWorker', 'CrashReportClient', 'EpicWebHelper' -ErrorAction SilentlyContinue)

`:1548-1555` — nine lines above — states the law this line breaks, and states it correctly:

> *"THE PROBE IS `$proc.HasExited`, NOT `Get-Process -Id`. … `Start-Process -PassThru` holds an OPEN
> HANDLE to the process, and a killed process's PID stays RESOLVABLE by `Get-Process` for as long as
> any handle is open."*

⛔ **`$proc` is still in scope and still holding that handle when `:1567` runs.**

**Verbatim from RUN #19 — three consecutive lines of the script's own output:**

      PID 13816 confirmed gone (HasExited).
      !! 1 engine process(es) still running after the kill: UnrealEditor-Cmd(PID 13816)
      !! Some may be Jonathan's. NOT killing them.

- ⛔ **(a) FALSE POSITIVE, measured.** It names **PID 13816** — the PID the line directly above
  certifies dead. I confirmed that death **three independent ways**: `HasExited=True`;
  `Get-CimInstance Win32_Process -Filter ProcessId=13816` returned **nothing** while the **positive
  control on my own PID returned my process** (so the reader demonstrably worked); and the PID was
  **absent** from a fresh name census taken in a different process.
- ⛔ **(b) FALSE NEGATIVE, measured.** The **one genuine orphan** — `CrashReportClientEditor`
  **PID 13296**, started 04:55:38, still alive at that instant — was **NOT named**, because the
  literal `'CrashReportClient'` **cannot prefix-match a longer process name**. Controlled on a live
  process rather than reasoned: `-Name 'powershell'` → **4**, `-Name 'powershel'` → **0**,
  `-Name 'powershel*'` → **4**.

⇒ ⚖️ ***THE GUARD REPORTS THE PROCESS THAT DIED AND MISSES THE PROCESS THAT LIVED — and then tells its
reader "Some may be Jonathan's. NOT killing them."*** On this machine nothing of Jonathan's was
running at all.

**Why this BLOCKS rather than warns.** The verdict and exit contract are sound and everything else
measured clean — but this is the **W-7 guard `#19` exists to exercise**; `SHIP-§9` says a guard seen
only to pass is `NOT MEASURED`; and it is **W-14's exact shape**: the fix applied to one organ
(`:1560`) and not to its sibling nine lines below. It is also, by my count, the **fifth** null-reading
defect of this session and the **second inside this one function**.

**FIX — two parts, both small:** (1) exclude the killed PID from the census (or filter it on
`HasExited`); (2) **wildcard every name** — `'CrashReportClient*'`, `'ShaderCompileWorker*'`,
`'EpicWebHelper*'`. **And give it the one thing it has never had: a case that FAILS** (`SHIP-§9`) —
assert the killed PID is **ABSENT** from the orphan list, and that a **longer-named** process **IS**
found.

## L3.3 ⚠️ NOT A DEFECT IN THE SCRIPT — a wrapper trap that will bite the next agent

Measured while controlling my own reader, and worth a `.NOTES` line:

- `powershell -Command "& script.ps1"` **mashes every non-zero exit to `1`** (probe: a script that
  exits 42, wrapper reports **1**). ⛔ **`1` is precisely W-11's undocumented code** — so the wrapper
  can manufacture the very false red W-11 warns about. It cost me two apparent `MISMATCH` rows before
  I controlled it, and I nearly reported them as script defects.
- `powershell -File script.ps1` is **faithful** (42 → 42) but **cannot carry an array**: it binds only
  the first element and slides the rest onto positional parameters. That silently gave me
  `Expected = 1` instead of 2 on a Command-lane run **that still returned the expected exit code** —
  a green-looking run with the wrong input, which is this project's signature failure.
- ✅ **The only correct agent invocation is** `-Command "& script … ; exit $LASTEXITCODE"` — faithful
  **and** array-capable.

## L3.4 ⛔ WHAT I DID NOT DO — stated plainly, not omitted

- **#22, the `.cmdline` sidecar and its BOM (`N-1`): NOT CHECKED.** The PC is on a sleep timer and I
  spent the remaining budget on the blocker. **It is UNEXERCISED and I am not implying otherwise.**
- ⛔ **`TL-§6`'s `NOT MEASURED` bullet, `CONVENTIONS.md:4895`, `CONVENTIONS.md:5961`,
  `TASKBOARD.md:1735` and the script's own `SANCTIONED IS NOT EXERCISED` header block are ALL LEFT
  STANDING** — every one of them the manager's under `SC-§82`. ⭐ **And #19 did not earn the strike in
  any case, because it failed.**
- ⛔ **`Tools/run_suite_bounded.ps1`, `Tools/SuiteRunnerFixtures/**`, `handoffs/TASK-1181-programmer.md`
  and THIS REPORT are NOT COMMITTED.** They stay dirty on disk and ship together when rev-3 lands.
  The fog prose lane (`TASK-1177/1178/1179/1180`) shipped alone in `ea7b4d2`.
