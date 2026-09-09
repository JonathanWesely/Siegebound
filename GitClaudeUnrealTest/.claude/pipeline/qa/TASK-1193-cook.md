# TASK-1193 — SHIP STOPPED BEFORE THE COOK. THE COOK DID **NOT** RUN.

**Verdict: STOP at `UNEXPECTED-ERROR` — a `Tools/Packaging/ship.ps1` defect at gate B2-SUITE, NOT a compile failure, NOT a cook failure, NOT a red suite.**
Written by build-master, 2026-09-09, invocation 1. Filed at the row's prescribed failure path; the title is the honest one.

⚠️ **Read this first:** the Shipping target was **never compiled and never cooked** by this run. Every gate this week wrote `"COOKED TARGET NOT COMPILED"` and that sentence is **still true**. `SC-§111` still has no gate.

---

## 1. What the live run did, in order (run log dir `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755\`)

| Gate | Verdict | Evidence |
|---|---|---|
| A1–A8 (13 gates) | **PASS** | route Pixel (explicit), fence route (ii) `.gitignore:19`, 1294.5 GB free, desktop drivable |
| **B1-COMPILE** | **PASS** | `compile.out.log`: `Result: Succeeded` · `Total execution time: 12.95 seconds` (log-parsed; exit code ignored) |
| **B2-SUITE** | **CRASHED — not judged** | `SHIP RESULT: STOP at UNEXPECTED-ERROR - The property 'Count' cannot be found on this object. Verify that the property exists.` · stack: `at <ScriptBlock>, C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1: line 2236` · `powershell exit=1` |
| C1…C4, D, F | **NOT REACHED** | no UAT invocation, no stage write, no state file (`ship-state.json` absent), no zip, no commit |

## 2. The suite itself is GREEN — measured by hand from the script's own logs (`PKG-§9c`, Development editor target)

```
automation.log:6982  LogAutomationCommandLine: Display: ...Automation Test Queue Empty 555 tests performed.
automation.log:6984  LogExit: Display: **** TestExit: Automation Test Queue Empty ****
```
| Phrase | `automation.log` | `suite.out.log` |
|---|---|---|
| `Result={Success}` | **555** | **555** |
| `Result={Fail}` | **0** | **0** |
| `Test Completed` | 555 | 555 |

Editor self-terminated on `-testexit` (`FPlatformMisc::RequestExit(1, FEngineLoop::Tick.GScopedTestExit)`); `automation-report/index.json` written (311,660 B). Compile started 20:07:55Z; suite ended 20:09:44Z. **555 / 555, matching TASK-1183's last measurement at `4a3da63`.**

## 3. The defect, measured in the same PowerShell the script ran under (`5.1.26100.9168`) — ⛔ not read off the source

`ship.ps1:2236`
```powershell
$successCount = (Get-LogMatches -Paths $slogs -Pattern 'Result=\{Success\}' -Simple).Count
```
1. **`-Simple` ⇒ `Select-String -SimpleMatch` ⇒ the pattern is LITERAL, backslashes included.** Measured on this run's `automation.log`:
   - `-SimpleMatch 'Result=\{Success\}'` → **0** hits
   - `-SimpleMatch 'Result={Success}'` → 555
   - regex `'Result=\{Success\}'` → 555 · regex `'Result=\{Fail\}'` → 0
   ⇒ the pattern **can never match** any UE log; the regex escaping and `-Simple` contradict each other.
2. **`Get-LogMatches` (`:755-771`) returns `$res` where `$res = @()` on zero matches; PowerShell unrolls an empty array to `$null`, and under the script's `Set-StrictMode -Version Latest` (`:214`), `.Count` on it THROWS.** Reproduced verbatim in PS 5.1:
   ```
   function f { $res = @(); return $res }; (f).Count
   THROWS: The property 'Count' cannot be found on this object. Verify that the property exists.
   ```
3. **The same shape sits on `:2237`–`:2238`** — `$failLines = Get-LogMatches … 'Result=\{Fail\}' -Simple` then `$failLines.Count` — and on a GREEN suite the `Result={Fail}` match count is **by definition 0**, i.e. the zero-match path is the *normal* path for that line. (Stated as a measurement of the file, not as a fix.)
4. **Provenance:** `git blame -L 2234,2238` → all five lines `2cc8213` (TASK-703, 2026-08-30, the ONLY commit that has ever touched `ship.ps1`). ⇒ **B2 has been a guaranteed `UNEXPECTED-ERROR` on every live invocation since the file was born.** No live `/ship` ever passed B2; TASK-716 was blocked at the desktop before PHASE B.
5. **Law class: `SHIP-§9c` clause 1 — a gate never validated against the log it parses — and `SHIP-§7`'s named blind spot: `-DryRun` records B2 as `PLAN` and cannot reach it, so `DRYRUN-OK` (2026-08-30 acceptance, and again today at 20:05Z) was reported over this.** It is the fifth forever-stop in this lane, and the first that fails in the wrong voice (exit 1, `UNEXPECTED-ERROR`) rather than as a named gate.

## 4. What it is NOT

- ⛔ Not a compile failure (B1 PASS, `Result: Succeeded`).
- ⛔ Not a red suite (555/555, 0 fail, measured twice over from two logs).
- ⛔ Not a cook failure — **UAT was never invoked.** No line of the Shipping target compiled.
- ⛔ Not an environment/SAC/sandbox event (the script parsed its own logs and then threw in its own code).

## 5. What is required to clear it (`SHIP-§1`: reported, not worked around)

- **A fix row for `gameplay-programmer` over `Tools/Packaging/ship.ps1` B2 (`:2236`–`:2238`) with a QA gate** — `Tools/**/*.ps1` is CODE (`TL-§6`). The build-master does not write code and `ship.md` forbids working around a gate by hand ("If a gate is wrong, fix the gate — under a task, with QA"). ⛔ No flag, no hand-run cook, no Development fallback was attempted.
- **`SHIP-§9c` validation duty for the fix:** B2 must be shown `PASS` on this run's real logs (`.ship/20260909-200755/automation.log`, `suite.out.log` — 555 Success / 0 Fail) **and** `STOP` on a log carrying `Result={Fail}` lines (`Tools/SuiteRunnerFixtures/red-suite.log` exists in `8c444ca`), and must not throw on the zero-match path.
- Then re-run `/ship` (invocation 1). PHASE A + B are cheap (13 s compile, ~110 s suite); the cook is the 30-minute part and is still unproven.

## 6. State left behind

- Previous zip `Siegebound-Win64-Development-2026-08-29.zip` **untouched**. Stage `Windows/` **untouched** (still the Aug-30 Shipping binaries, no orphan). No `ship-state.json`, no adjudication record. Nothing staged, nothing committed, nothing pushed.
- Editor **closed** (PID 6732 stopped under the standing grant for the compile); `L_Arena.umap` sha256 `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` before AND after. Left closed pending the re-run.
