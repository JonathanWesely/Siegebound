# TASK-1193 — SHIP STOPPED BEFORE THE COOK. THE COOK DID **NOT** RUN.

**Verdict: STOP at `UNEXPECTED-ERROR` — a `Tools/Packaging/ship.ps1` defect at gate B2-SUITE, NOT a compile failure, NOT a cook failure, NOT a red suite.**
Written by build-master, 2026-09-09, invocation 1. Filed at the row's prescribed failure path; the title is the honest one.

⚠️ **Read this first:** the Shipping target was **never compiled and never cooked** by this run. Every gate this week wrote `"COOKED TARGET NOT COMPILED"` ~~and that sentence is **still true**. `SC-§111` still has no gate.~~ ✅ **STRUCK 2026-09-09 by the manager (`SC-§53` cl. 3): true at invocation 1 (20:07Z); FALSE since invocation 3 attempt 2 (22:08 PDT), which compiled the Shipping target clean — see the INVOCATION 2 heading's strike below for the evidence, and `SC-§111` cl. 6 entry 1.**

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

---

# INVOCATION 2 (2026-09-09 14:05–14:07 PDT, run log dir `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-210513\`) — B2 PASSED WHERE IT CRASHED BEFORE; THE COOK WAS INVOKED FOR THE FIRST TIME AND UAT REFUSED IN 9 SECONDS. ~~**THE SHIPPING TARGET HAS STILL NEVER COMPILED.**~~ ✅ **STRUCK 2026-09-09 by the manager (`SC-§53` cl. 3 — struck, not deleted): true when written (14:07 PDT); FALSE since invocation 3 attempt 2 (22:08 PDT, run dir `…\.ship\20260910-050628\`), which compiled the Shipping target for the first time in this project's history — UBT 18 unity modules + link `GitClaudeUnrealTest-Win64-Shipping.exe`, `Result: Succeeded`, 25.74 s, 0 errors; cook 1400/1400, 32.73 s; UAT `BUILD SUCCESSFUL`, 1 m 14 s. No INVOCATION 3 section exists in this file because there was no failure to record; that run's record is `handoffs/TASK-1193-buildmaster.md` PART 3 (attempt 2) §5 and `SC-§111` cl. 6 entry 1. Its STOP was the desktop lock at `C3-CAPTURE`, not code.**

**Verdict: `SHIP RESULT: STOP at C2-UAT-LOG - UAT's own verdict lines: BUILD SUCCESSFUL=False BUILD FAILED=True (exit code deliberately ignored)` · `POWERSHELL-EXIT=2` (14:07:10 PDT).**
**Cause, measured (not inferred): UBT's Live Coding mutex check.** A live `UnrealEditor.exe` — the `-game -windowed -ResX=3200 -ResY=1800` playtest session PID 12500, started 14:05:49 from Jonathan's own interactive PowerShell (PID 2000, started 08:38:14, parent `explorer.exe` PID 14392) — held `Global\LiveCoding_C++Program Files+Epic Games+UE_5.8+Engine+Binaries+Win64+UnrealEditor.exe` when UAT's `-build` step invoked UBT at ~14:07:00. **NOT a code failure. NOT a Shipping compile error — UBT stopped at "Creating makefile" before compiling a single file.** ~~`SC-§111` still has no gate.~~ ✅ **STRUCK 2026-09-09 (`SC-§53` cl. 3): gated by invocation 3 attempt 2 — see the heading's strike above.**
Written by build-master (`TASK-1197` host), 2026-09-09, appended — invocation 1's record above stays.

## 1. The gates, in order

| Gate | Verdict | Evidence |
|---|---|---|
| A1–A8 (13 gates) | **PASS** | route Pixel (requested explicitly); fence route (ii) `.gitignore:19:packagedZIPofGame/` proven this run; 1290.9 GB free; 11 COOKDIRs / both maps / boot map present; desktop drivable (root window under (768,480): `CASCADIA_HOSTING_WINDOW_CLASS` / `WindowsTerminal`); no prior state ⇒ B and C in full |
| **B1-COMPILE** | **PASS** | `log-parsed verdict: Succeeded=True Failed=False timedOut=False (exit code deliberately ignored)` |
| **B2-SUITE** | **PASS — the fourth validation side (`TASK-1195`/`1196`/`1197`)** | `[PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log); timedOut=False` — the exact shape QA predicted; invocation 1 died on this line with `UNEXPECTED-ERROR` |
| C1-COOK | PASS | "UAT finished within 90 min" — it returned in 9 s |
| **C2-UAT-LOG** | **STOP** | `UAT's own verdict lines: BUILD SUCCESSFUL=False BUILD FAILED=True (exit code deliberately ignored)` · `REQUIRED TO CLEAR: Read …\.ship\20260909-210513\cook.out.log.` |
| C2-STAGE-*, C3-*, C4, D, F | **NOT REACHED** | no stage write, no `ship-state.json`, no capture, no zip, no commit |

## 2. The errors, verbatim (`cook.out.log`, 3,831 B; `cook.err.log` 0 B)

The UAT command line UAT parsed (its own echo): `BuildCookRun -project=C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject -nop4 -utf8output -platform=Win64 -clientconfig=Shipping -build -cook -map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena -pak -stage -prereqs -archive -archivedirectory=C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame -AdditionalCookerOptions="-COOKDIR=…\Content\Data -COOKDIR=…\Content\UI -COOKDIR=…\Content\Blueprints -COOKDIR=…\Content\Input -COOKDIR=…\Content\Characters -COOKDIR=…\Content\Meshes -COOKDIR=…\Content\Materials -COOKDIR=…\Content\Textures -COOKDIR=…\Content\VFX -COOKDIR=…\Content\Audio -COOKDIR=…\Content\LevelPrototyping"` (11 COOKDIRs, each the absolute `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\…` path — the `PKG-§5a` recipe verbatim).

```
********** BUILD COMMAND STARTED **********
Running: C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" -Target="GitClaudeUnrealTestEditor Win64 Development -Project=C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject" -Target="GitClaudeUnrealTest Win64 Shipping -Project=C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject  -remoteini=\"C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\"  " -log="C:\Users\wesel\AppData\Roaming\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.8\UBA-GitClaudeUnrealTestEditor-Win64-Development.txt"
Log file: C:\Users\wesel\AppData\Roaming\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.8\UBA-GitClaudeUnrealTestEditor-Win64-Development.txt
Determining max actions to execute in parallel (16 physical cores, 16 logical cores)
  Executing up to 16 processes, one per physical core
Using 'git status' to determine working set for adaptive non-unity build (C:\GitProjects\GitHub\GitClaudeUnrealTesting).
UbaServer - Listening on 0.0.0.0:1345
Creating makefile for GitClaudeUnrealTest (command line arguments changed)
UHT compiled-in object format Default
Unable to build while Live Coding is active. Exit the editor and game, or press Ctrl+Alt+F11 if iterating on code in the editor or game

Result: Failed (OtherCompilationError)
Total execution time: 5.89 seconds
Trace written to file C:\Users\wesel\AppData\Local\UnrealBuildTool\Trace.uba with size 3.4kb
Took 6.38s to run dotnet.exe, ExitCode=6
UnrealBuildTool failed. See log for more details. (C:\Users\wesel\AppData\Roaming\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.8\UBA-GitClaudeUnrealTestEditor-Win64-Development.txt)
AutomationTool executed for 0h 0m 9s
AutomationTool exiting with ExitCode=6 (6)
BUILD FAILED
```

UBT's own log `…\UBA-GitClaudeUnrealTestEditor-Win64-Development.txt` `:100-103` (19,596 B, mtime 14:07:10):
```
Checking for live coding mutex: Global\LiveCoding_C++Program Files+Epic Games+UE_5.8+Engine+Binaries+Win64+UnrealEditor.exe
Unable to build while Live Coding is active. Exit the editor and game, or press Ctrl+Alt+F11 if iterating on code in the editor or game
BuildException: Unable to build while Live Coding is active. Exit the editor and game, or press Ctrl+Alt+F11 if iterating on code in the editor or game
```

## 3. The cause, measured in order

1. **Who held the mutex.** `Get-CimInstance Win32_Process`: `UnrealEditor.exe` PID 12500, started **14:05:49**, command line `"…\UnrealEditor.exe" C:\…\GitClaudeUnrealTest.uproject -game -windowed -ResX=3200 -ResY=1800`, parent PID 2000 = argument-less `powershell.exe` started **08:38:14**, parent `explorer.exe`. An explorer-parented, argument-less PowerShell from this morning is a person's console window: **Jonathan's playtest session, launched by hand.** Its window title is `Siegebound (64-bit Development PCD3D_SM6)` — identical to a GUI editor's — which is how the build-master mistook the earlier ones for the editor (item 4).
2. **Why one `-game` process stops a Shipping cook.** UAT's `-build` step is ONE UBT invocation with TWO targets (`GitClaudeUnrealTestEditor Win64 Development` for the cooker + `GitClaudeUnrealTest Win64 Shipping`), and UBT's first act is the Live Coding mutex check, global per engine binary path. **Any live `UnrealEditor.exe` with Live Coding — GUI editor or `-game` — refuses the whole cook.** ⇒ `.claude/commands/ship.md` §0.3 (*"A running editor is fine and is not a stop — the Shipping monolithic target links different binaries than the editor's DLLs"*) is **FALSE for the cook on this evidence**: the DLL argument is right and irrelevant, UBT never reaches linking. `A3-QUIET-MODULE` looks only for UBT/UAT/UnrealPak/UnrealEditor-Cmd and passed at 14:05:13 because no `UnrealEditor.exe` was alive at that instant.
3. **Timeline.** 14:05:07 the launch pre-check found `-game` PID 8760 (started 14:03:10) and force-stopped it; 14:05:13 script start; B1 compiled ~14:05:13–14:05:30 (`Result: Succeeded`, nothing to rebuild); **14:05:49 Jonathan relaunched (PID 12500)**; the suite ran to ~14:06:45 (`-nullrhi`, `UnrealEditor-Cmd`, unaffected — it builds nothing); ~14:07:00 UAT → UBT → mutex held → `BUILD FAILED` 14:07:10. **B1 passed by 19 seconds of timing, not by design.**
4. **The build-master's own part, stated plainly.** Believing per the dispatch brief that the process was the reopened GUI editor, the build-master force-stopped Jonathan's `-game` sessions twice (PID 7256 at ~14:02, PID 8760 at 14:05:07); an earlier one (PID 29712, started 13:56:32) disappeared on its own before any action. He relaunched within ~90 s each time. **PID 12500 was NOT touched and will not be** — the next run is scheduled around his session, not over it. Apology posted in 🚨 Blockers.

## 4. What it is NOT

- ⛔ Not a code failure — no source file was compiled; `Source/**` untouched since `8c444ca`.
- ⛔ Not a Shipping compile failure — ~~the Shipping target has **still never been compiled** in this project's history.~~ ✅ **STRUCK 2026-09-09 (`SC-§53` cl. 3): true at 14:07 PDT; it compiled clean at 22:08 PDT, invocation 3 attempt 2 — see the heading's strike.** At THIS invocation UBT compiled zero files of it.
- ⛔ Not a recipe failure — the UAT line is the `PKG-§5a` recipe verbatim (11 `-COOKDIR`s, both maps, `-clientconfig=Shipping`, `-pak -stage -prereqs -archive`).
- ⛔ Not a red suite (555/555, and this time the gate itself read it: B2 PASS). Not Smart App Control (`0x800711C7` absent), not the desktop (A8 PASS), not disk.

## 5. Required to clear (`SHIP-§1`: reported, not worked around)

- **No code change. No programmer cycle. No flag.** The remedy is machine state: **no `UnrealEditor.exe` of any kind alive** (editor or `-game` — same mutex) for the run's duration (≈15 s compile + 2 min suite + the first-ever Shipping compile + cook, unmeasured — budget 30–60 min) plus the screen for the C3 click rig. ⛔ Never `Ctrl+Alt+F11` on his session, never a kill of his playtest: **Jonathan says when**, then invocation 1 is re-run verbatim (Phases A–C, STOP at C3).
- Recipe/doc debt for the manager: (a) `ship.md` §0.3's "a running editor is fine" sentence; (b) `A3-QUIET-MODULE` should count a live `UnrealEditor.exe` (any mode) as a STOP for a run that will cook, or at least print it — `Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'"` + `CommandLine` (`-game` vs editor) is the cheap check; (c) a `-game` window's title equals the editor's — identify by command line, never by title.

## 6. Census check (`TASK-1190`): `Config/*.ini` in the stage

- This cook produced **no stage**. The only stage that exists is the Aug-30 Development one; `UnrealPak.exe GitClaudeUnrealTest-Windows.pak -List` (11,464,556 B, 2026-08-30 00:07) holds 82 `.ini` entries: **78 engine-side + exactly four project inis — `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Config/DefaultInput.ini`, `Config/SiegeCloudDev.ini`.** `DefaultEditor.ini`, `DefaultEditorPerProjectUserSettings.ini` and `SiegeCloudDev.ini.example` were not staged. `DefaultGame.ini` has no `[Staging]` section (grep: none). ⇒ the census prediction holds: **`SiegeCloudDev.ini` ships.**
- `Config/SiegeCloudDev.ini` (1,705 B, 2026-08-23, **untracked**): under `[SiegeCloud]`, exactly two keys and nothing else besides comment lines — `ProjectUrl` (42 chars, present) and `AnonKey` (208 chars, present). No service-role key, no password, no other section. ⛔ Values not reproduced. For `{{SHIP:CLOUD_SYNC}}`: the shipped build carries the cloud project URL and the publishable/anon key and nothing beyond them — the same two values that shipped in the Aug-29 Development zip. Whether a `[Staging]` deny is wanted is the manager's/Jonathan's call.

## 7. State left behind

- Previous zip `Siegebound-Win64-Development-2026-08-29.zip` (1,409,955,049 B, 2026-08-29 17:16) **untouched**. Stage `Windows\` **untouched** (Aug-30 Shipping binaries `GitClaudeUnrealTest-Win64-Shipping.exe` 177,716,736 B + `.pdb` 249,204,736 B, mtime 2026-08-30 00:04; no Development orphan). No `ship-state.json`, no adjudication record, no capture. Nothing staged, nothing committed, nothing pushed by this invocation (the B2 fix commit `31edc23` preceded it; `main` 1 ahead).
- `Content/Maps/L_Arena.umap` sha256 `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` before the first kill, after the kills, and after the run; `Content/` porcelain empty throughout. Jonathan's `-game` session PID 12500 **left running**.
- Artefacts: `…\.ship\20260909-210513\{compile.out.log, suite.out.log, suite.err.log, automation.log, automation-report\, cook.out.log, cook.err.log}`; `C:\Users\wesel\AppData\Roaming\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.8\{UBA-GitClaudeUnrealTestEditor-Win64-Development.txt, Log.txt}`; session scratch `live2.log`, `live2.precheck.log`.
