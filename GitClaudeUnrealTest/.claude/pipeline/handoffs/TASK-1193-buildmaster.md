# TASK-1193 — build-master handoff, PART 1 (invocation 1: Phases A–C)

**Outcome of invocation 1 (2026-09-09): `SHIP RESULT: STOP at UNEXPECTED-ERROR` at gate B2-SUITE, `ship.ps1:2236`. B1 compile PASS; suite measured 555/555 green by hand; THE COOK DID NOT RUN. Evidence: `qa/TASK-1193-cook.md`.** No C3 capture exists. Phase D was never reachable. This file will gain a part 2 when a re-run reaches `ADJUDICATE`.

## 1. Phase A — every check, with its evidence

| Check | Instrument | Reading |
|---|---|---|
| Tree state | `git rev-parse HEAD` / `git status --porcelain` / `git rev-list --left-right --count origin/main...main` | **HEAD `8c444ca2d54cdfffccb8737c6519609d13ac6fa2` — "fog visual updates", Jonathan's own commit (2026-09-09 12:44 -0700)**, ⛔ not the `ea7b4d2` the brief named · **`main` 0 / 0 vs `origin/main` — already pushed by him** · dirty = 3: ` M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md`, ` M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md`, ` M GitClaudeUnrealTest/CLAUDE.md` · build-relevant dirt 0 (tree hash `e3b0c44298fc`) |
| ⚠️ What `8c444ca` swept in (`git show --stat HEAD`, reported not tidied — `SHIP-§5` cl.3) | | `Tools/run_suite_bounded.ps1` (+1616), `Tools/SuiteRunnerFixtures/*.log` (11 fixtures), `handoffs/TASK-1181-programmer.md`, `handoffs/TASK-1183-buildmaster.md`, `qa/TASK-1182-report.md`, +30/−0 CONVENTIONS, +139 TASKBOARD. ⇒ the withheld runner lane is now in history under his commit; the TASK-1091 `Content/RawAssets/MainCharacter*` + evidence PNGs are no longer untracked (not in this commit's stat either — they were absent from porcelain at my start). |
| Mid-operation | `.git/{MERGE_HEAD,CHERRY_PICK_HEAD,REVERT_HEAD,BISECT_LOG,rebase-*}` | none |
| QUIET-MODULE | `tasklist` + script A3 | no UBT/UAT/UnrealPak/UnrealEditor-Cmd; the GUI editor PID 6732 (~1.9 GB) was up and was closed by me (below) |
| `PKG-§7a` fence, **proven this run** | `git rev-parse --show-toplevel` → `C:/GitProjects/GitHub/GitClaudeUnrealTesting` · `git check-ignore -v -- <staging>/README.md` → `.gitignore:19:packagedZIPofGame/` (the **work-tree-root** `.gitignore`; line 19 read: `packagedZIPofGame/`) | route (ii); script A4-FENCE PASS with the same reading |
| Disk | script A5 | 1294.5 GB free on C: (need 4) |
| Evidence route | script A6 | installed engine (`Engine\Build\InstalledBuild.txt` exists) ⇒ Shipping log-silent ⇒ **Pixel**; instrument probe OK, primary screen 1536×960 |
| Recipe sanity | script A7 | 11 COOKDIRs present; both maps present; `GameDefaultMap=/Game/Maps/L_MainMenu.L_MainMenu` |
| Desktop | script A8 | drivable — root window under (768,480) `CASCADIA_HOSTING_WINDOW_CLASS` / `WindowsTerminal` (dry run: `ApplicationFrameWindow`); positive lock detector found no lock owner |
| Prior state | `packagedZIPofGame/.ship/` | did not exist — no state, no adjudication record |
| Dry run | `ship.ps1 -DryRun` at 20:05:35Z | `DRYRUN-OK`; plan printed (exact Build.bat, suite and UAT lines with all 11 `-COOKDIR`s; click point client-relative (0.5000, 0.2929), entry 1 of 7, pitch 0.069028; stage authority = manifests for both binaries) |

## 2. The editor kill and `L_Arena`

- `Stop-Process -Name UnrealEditor -Force` (PowerShell; the first `taskkill /F` attempt was mangled by Git Bash into `F:/` and did nothing) + `CrashReportClientEditor`. No leftovers. Standing grant; every save declined by construction (force-stop).
- `Saved/Autosaves/PackageRestoreData.json` after the kill: `RestoreEnabled: false, Packages: []` — nothing offered, nothing parked (`SHIP-§9e(i)` procedure consulted, inert).
- `Content/Maps/L_Arena.umap` sha256 **before** `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` · **after** `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` · mtime `2026-09-05T00:42:58.649` both times · porcelain unchanged.
- Editor left **closed**.

## 3. Phase B

- **B1-COMPILE: PASS** — `compile.out.log`: `Result: Succeeded`, `Total execution time: 12.95 seconds`, UBA local executor 11.02 s (log-parsed; exit code ignored). Command = the CLAUDE.md line, printed by the script.
- **B2-SUITE: script crashed before judging it.** Suite (Development editor target, `-nullrhi`, `Automation RunTests Siegebound`): `Automation Test Queue Empty 555 tests performed.` · `Result={Success}` = **555** · `Result={Fail}` = **0** (both `automation.log` and `suite.out.log`) · `TestExit` self-termination · report written. **555 / 555 measured at 20:09:44Z.**
- Crash: `ship.ps1:2236` — `Get-LogMatches … -Pattern 'Result=\{Success\}' -Simple` can never match (`-SimpleMatch` treats the backslashes literally: measured 0 hits vs 555 for the unescaped literal), and the empty return's `.Count` throws under `Set-StrictMode -Version Latest` (reproduced). Same shape at `:2237–2238`. Both lines from `2cc8213` (TASK-703), the file's only commit. Full measurement in `qa/TASK-1193-cook.md` §3.

## 4. Phase C — NOT REACHED

- UAT verdict lines: none (UAT never invoked). Cook duration: n/a. Stage contents: unchanged from the Aug-30 hand cook (`Windows/GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe` 177,716,736 B + `.pdb` 249,204,736 B; no Development orphan; manifests dated Aug 30). `PKG-§10`/`§4` re-checks: not run this invocation.
- C3 capture paths: **none exist.**

## 5. What this invocation did NOT prove (`SHIP-§6`)

- Whether the Shipping target compiles (⛔ first-ever Shipping compile still pending) · whether the cook succeeds · the arena/deck/HUD/hero pixels · anything about gameplay feel (no input lane, ever).

## 6. Route

`ship.md`: *"If a gate is wrong, fix the gate — under a task, with QA."* `Tools/**/*.ps1` is code (`TL-§6`). ⇒ fix row for `gameplay-programmer` → QA (`SHIP-§9c` both-sides validation on this run's real logs + `Tools/SuiteRunnerFixtures/red-suite.log`) → re-run `/ship` invocation 1. Nothing staged, nothing committed, nothing pushed by this invocation.

## 7. Artefacts (absolute)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755\{compile.out.log, suite.out.log, automation.log, automation-report\index.json}`
- `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\9ea39ad5-585f-4ec7-9dc8-126791d0a5a4\scratchpad\{dryrun.log, live.log}` (session scratch; the `.ship` copies are the durable ones)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\qa\TASK-1193-cook.md`

---

# PART 2 — invocation 2 (the re-run under `TASK-1197`, 2026-09-09 14:05–14:07 PDT), run log dir `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-210513\`

**Outcome: `SHIP RESULT: STOP at C2-UAT-LOG - UAT's own verdict lines: BUILD SUCCESSFUL=False BUILD FAILED=True (exit code deliberately ignored)`, `POWERSHELL-EXIT=2`. B2 PASSED — the fourth validation side printed. The cook was invoked for the first time in this project's history and UAT refused in 9 s on UBT's Live Coding mutex, held by Jonathan's live `-game` playtest session (PID 12500, launched 14:05:49 from his own PowerShell). NOT a code failure; the Shipping target has STILL never compiled. Full record + verbatim errors: `qa/TASK-1193-cook.md` INVOCATION 2. No C3 capture exists; Phase D never reachable.**

## 1. `SC-§91` — the tree at this invocation's instant

| Instrument | Reading |
|---|---|
| `git rev-parse HEAD` | **`31edc23`** — the B2 fix commit (`TASK-1197`), 14:03:57, immediately before the launch |
| `git rev-list --left-right --count origin/main...main` | **`0  1`** — `main` 1 ahead (his `8c444ca` is origin's tip) |
| `git status --porcelain` at launch | ` M CLAUDE.md` (his) · `?? handoffs/TASK-1190-census.md` · `?? handoffs/TASK-1190-programmer.md` · `?? handoffs/TASK-1193-buildmaster.md` · `?? Docs/Packaging/` (the README lane's) — **`Tools/Packaging/ship.ps1` CLEAN** (the row's precondition) |
| Mismatch vs the brief | none — brief said `8c444ca` / 0-0 before the fix; the tree said exactly that |
| Build-relevant dirt | 0 (no prior state; nothing under `Source/ Content/ Config/ Plugins/`) |

## 2. Phase A — every gate with its evidence (`PKG-§7a` fence proven this run)

| Gate | Evidence (the script's own lines) |
|---|---|
| A1-UPROJECT / A1-REPO / A1-ENGINE / A1-STAGING | 1 `.uproject`; repo root `C:\GitProjects\GitHub\GitClaudeUnrealTesting`; `UE_5.8`; staging `…\packagedZIPofGame` (first existing of 3 probes) |
| A2-NO-MID-OPERATION | `unresolved git state markers: none` |
| A3-QUIET-MODULE | `build/cook processes live: none` (⚠️ does not look for a GUI/`-game` `UnrealEditor.exe`; none alive at 14:05:13 — §5) |
| **A4-FENCE** | `route (ii): ignored by .gitignore:19:packagedZIPofGame/ "C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\packagedZIPofGame"` — measured this run |
| A5-DISK | `1290.9 GB free on C: (need 4)` |
| A6-EVIDENCE-ROUTE | `route PIXEL (requested explicitly); instrument: screen capture OK, primary screen 1536x960; PKG-6a arena bar UNCHANGED - only the instrument changed; ends in CALLER ADJUDICATION at C3-BOOT-ARENA, it never self-passes` |
| A7-COOKDIRS / A7-MAPS / A7-BOOTMAP | `11 COOKDIR entries; missing: none` · `maps: /Game/Maps/L_MainMenu+/Game/Maps/L_Arena; missing: none` · `GameDefaultMap must start with /Game/Maps/L_MainMenu` |
| A8-DESKTOP | `no lock-screen owner found at the primary-screen centre (root window under (768,480): class 'CASCADIA_HOSTING_WINDOW_CLASS', owner 'WindowsTerminal')` |
| Pixel verdict record / Resume decision | absent at `…\.ship\ship-adjudication.json` · `no prior state - PHASE B and C will run in full` |

## 3. Phase B

- **B1-COMPILE: PASS** — `log-parsed verdict: Succeeded=True Failed=False timedOut=False (exit code deliberately ignored)`; `compile.out.log` 1,072 B (up to date since invocation 1).
- **B2-SUITE: PASS — pasted verbatim, the fourth validation side (`TASK-1195` fix / `TASK-1196` gate / `TASK-1197` host):**
  `[PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log); timedOut=False`
  Exactly the shape QA predicted. Invocation 1 died on this line with `UNEXPECTED-ERROR … 'Count' cannot be found`. Suite 555 / 555; `automation.log` 1,183,744 B, `suite.out.log` 363,343 B, `suite.err.log` 0 B, report written.

## 4. Phase C — UAT invoked (first time ever), refused in 9 s

- Command: the script's printed `RunUAT.bat BuildCookRun … -clientconfig=Shipping -build -cook -map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena -pak -stage -prereqs -archive -archivedirectory="…\packagedZIPofGame" -AdditionalCookerOptions="-COOKDIR=… ×11"` — the `PKG-§5a` recipe verbatim (UAT's own echo in the cook record).
- **C1-COOK: PASS** (returned within 90 min — in 9 s). **C2-UAT-LOG: STOP** — `BUILD SUCCESSFUL=False BUILD FAILED=True`. Cook duration **0 h 0 m 9 s**, `ExitCode=6`.
- UAT's own verdict lines, verbatim: `Unable to build while Live Coding is active. Exit the editor and game, or press Ctrl+Alt+F11 if iterating on code in the editor or game` · `Result: Failed (OtherCompilationError)` · `UnrealBuildTool failed. See log for more details.` · `AutomationTool exiting with ExitCode=6 (6)` · `BUILD FAILED`. UBT log `:100`: `Checking for live coding mutex: Global\LiveCoding_C++Program Files+Epic Games+UE_5.8+Engine+Binaries+Win64+UnrealEditor.exe`.
- Cause: UAT's `-build` = ONE UBT invocation with BOTH `GitClaudeUnrealTestEditor Win64 Development` and `GitClaudeUnrealTest Win64 Shipping`; the mutex check refused because `UnrealEditor.exe` PID 12500 (`-game -windowed -ResX=3200 -ResY=1800`, Jonathan's, started 14:05:49) was alive. **The Shipping target compiled zero files.** Routing rule 6 mechanics applied (record, `build-failed`, posts) — but this is **machine state, not a code error**: no programmer cycle owed; remedy = re-run with no `UnrealEditor.exe` alive, on Jonathan's word (cook record §5).
- C2-STAGE-* / C3 / C4: NOT REACHED. **C3 capture paths: none.** Stage unchanged from the Aug-30 hand cook (`GitClaudeUnrealTest-Win64-Shipping.exe` 177,716,736 B + `.pdb` 249,204,736 B, mtime 2026-08-30 00:04; no Development orphan).

## 5. The editor — found, done, learned

- 14:00:12 PDT: `UnrealEditor.exe` PID 29712 (started 13:56:32, 5.6 GB, title `Siegebound (64-bit Development PCD3D_SM6)`). Channel read first: no post from Jonathan, nothing about a playtest. By ~14:02 it was gone and PID 7256 (started 14:01:40) was up → force-stopped under the standing grant. 14:03:10 PID 8760 appeared → the launch pre-check force-stopped it at 14:05:07. 14:05:49 PID 12500 appeared → **identified by `Win32_Process` as `-game -windowed -ResX=3200 -ResY=1800`, parent = Jonathan's interactive PowerShell (PID 2000, 08:38:14, parent `explorer.exe`) — his playtest, not the editor.** Left running; apology posted in 🚨. Every kill declined every save by construction; `PackageRestoreData.json` = `RestoreEnabled: false, Packages: []`.
- `L_Arena.umap` sha256 **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** before the first kill, after the kills, and after the run; mtime `2026-09-05 00:42:58`; `Content/` porcelain empty throughout.
- ⚠️ Learned: (1) a `-game` session's title equals the editor's — tell them apart by command line; (2) UBT's Live Coding mutex is global per engine binary, so any live `UnrealEditor.exe` blocks UAT's `-build` — `ship.md` §0.3 "a running editor is fine" is false for the cook and `A3` does not look for it; (3) B1 passed only because the compile finished 19 s before his relaunch.

## 6. Census check (`TASK-1190`) — `Config/*.ini` in the stage; `SiegeCloudDev.ini`

- **This cook produced no stage.** The Aug-30 Development pak (`UnrealPak -List`, 11,464,556 B) holds 82 `.ini` entries: 78 engine-side + **exactly four project inis — `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Config/DefaultInput.ini`, `Config/SiegeCloudDev.ini`** (editor inis and `.example` not staged). `DefaultGame.ini` has no `[Staging]` section. The census prediction holds: **`SiegeCloudDev.ini` ships.**
- `Config/SiegeCloudDev.ini` (1,705 B, 2026-08-23, **untracked**): `[SiegeCloud]` with exactly two keys, nothing else besides comments — **`ProjectUrl` (42 chars, present) and `AnonKey` (208 chars, present)**. No service-role key, no password. ⛔ Values not reproduced anywhere. ⇒ `{{SHIP:CLOUD_SYNC}}`: the build carries the cloud project URL and the publishable/anon key (RLS-gated by design, `ACC-§`) and nothing beyond them — the same two values the Aug-29 Development zip shipped. A `[Staging]` deny is the manager's/Jonathan's call.

## 7. Not proven by this invocation (`SHIP-§6`)

Whether the Shipping target compiles (⛔ still first-ever pending) · whether the cook succeeds · arena/deck/HUD/hero pixels · gameplay feel (no input lane, ever). Proven: the B2 parser reads a real green suite live; any live `UnrealEditor.exe` blocks the cook.

## 8. Route

Not a code failure ⇒ **no programmer dispatch recommended**. Required: Jonathan's word that his `-game` session is closed (or a window), then **re-run invocation 1 verbatim** (A–C, STOP at C3; ~35 min + the Shipping compile; screen needed). Manager: the three recipe/doc findings (cook record §5). Nothing staged/committed/pushed by this invocation; `31edc23` preceded it; `main` 1 ahead.

## 9. Artefacts (absolute)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-210513\{compile.out.log, compile.err.log, suite.out.log, suite.err.log, automation.log, automation-report\index.json, cook.out.log, cook.err.log}`
- `C:\Users\wesel\AppData\Roaming\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.8\{UBA-GitClaudeUnrealTestEditor-Win64-Development.txt, Log.txt}`
- `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\9ea39ad5-585f-4ec7-9dc8-126791d0a5a4\scratchpad\{live2.log, live2.precheck.log}` (session scratch)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\qa\TASK-1193-cook.md` (INVOCATION 2 appended) · `…\handoffs\TASK-1197-buildmaster.md`

---

# PART 3 — invocation 3 (2026-09-09, on Jonathan's word in Claude Code: *"ok, I am done playtesting, you are clear to start the cook"* — `SC-§118` cl. 6), run log dir `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-222353\` (empty — Phase A only)

**Outcome: `SHIP RESULT: STOP at A8-DESKTOP - THE DESKTOP IS LOCKED: root window under (768,480): class 'LockScreenBackstopFrame', owner 'explorer'`, `POWERSHELL-EXIT=2`, 2 s after launch. Machine state, NOT a code failure, NOT routing rule 6. No compile, no suite, no cook, no stage write, no C3 capture. The Shipping target has STILL never compiled (`SC-§111` still ungated). Nothing of any class was closed. Previous zip untouched.**

## 1. `SC-§91` — the tree at this invocation's instant

| Instrument | Reading |
|---|---|
| `git rev-parse HEAD` | **`31edc23968f8ee9766b4155c5b13a6cc354427de`** (the B2 fix, `TASK-1197`) — matches the brief |
| `git rev-list --left-right --count origin/main...main` | **`0  1`** — `main` 1 ahead, not pushed |
| `git status --porcelain` (git root one level up, `SC-§102`) | 12 paths: ` M .claude/commands/ship.md` · ` M .claude/pipeline/CONVENTIONS.md` · ` M .claude/pipeline/TASKBOARD.md` · ` M .claude/pipeline/qa/TASK-1193-cook.md` · ` M CLAUDE.md` · `?? handoffs/TASK-1190-census.md` · `?? handoffs/TASK-1190-programmer.md` · `?? handoffs/TASK-1191-programmer.md` · `?? handoffs/TASK-1193-buildmaster.md` · `?? handoffs/TASK-1197-buildmaster.md` · `?? qa/TASK-1192-report.md` · `?? Docs/Packaging/` (all `GitClaudeUnrealTest/`-prefixed). **`Tools/Packaging/ship.ps1` CLEAN** (the row's precondition). Build-relevant dirt: none under `Source/ Content/ Config/ Plugins/`. |
| Mid-operation markers | none |
| Prior ship state | `.ship/` holds only the two earlier run dirs; no `ship-state.json`, no `ship-adjudication.json` |

## 2. The hand classification (`SHIP-§10` cl. 3 / `SC-§118` cl. 1), verbatim — by COMMAND LINE, never by name or title

First pass, before anything else (PowerShell `Get-CimInstance Win32_Process`, filter `UnrealEditor.exe | UnrealEditor-Cmd.exe | UnrealBuildTool.exe | AutomationTool.exe | UnrealPak.exe | dotnet.exe | GitClaudeUnrealTest.exe | GitClaudeUnrealTest-Win64-Shipping.exe | CrashReportClientEditor.exe | UnrealBuildAccelerator.exe | UbaAgent.exe`; each hit to be classed `-game` ⇒ GAME-SESSION (Jonathan) / no `-game` ⇒ GUI-EDITOR / `-Cmd` ⇒ COMMANDLET):

```
CENSUS AT 2026-09-09 15:21:14.252 -07:00
RESULT: ZERO instances of UnrealEditor.exe / UnrealEditor-Cmd.exe / UBT / UAT / UnrealPak / dotnet / game exes / CRC
---
Get-Process cross-check (name only, informational):
(empty)
```

Second pass, in-process immediately before the launch (`launch3.ps1`, session scratch; it REFUSES and exits 3 on any hit — it contains no `Stop-Process`):

```
PRECHECK (hand classification, SHIP-10 cl. 3) at 2026-09-09 15:23:52.610 -07:00
RESULT: ZERO instances of UnrealEditor.exe / UnrealEditor-Cmd.exe / UnrealBuildTool / AutomationTool / UnrealPak / CrashReportClientEditor
L_Arena before: 1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622
HEAD: 31edc23968f8ee9766b4155c5b13a6cc354427de
LAUNCH at 2026-09-09 15:23:52.849 -07:00
CMD: powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1 -Configuration Shipping -BootEvidence Pixel
```

Nothing was closed. The standing grant was not exercised (suspended for this run by `SC-§118` cl. 6).

## 3. Phase A — every gate with the script's own evidence (13 gates: 12 PASS, 1 STOP)

| Gate | Evidence |
|---|---|
| A1-UPROJECT | `1 .uproject file(s) under C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest` |
| A1-REPO | `git rev-parse --show-toplevel` → `C:\GitProjects\GitHub\GitClaudeUnrealTesting` |
| A1-ENGINE | `C:\Program Files\Epic Games\UE_5.8` |
| A1-STAGING | staging `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame` (first existing of 3 probes); run log dir `…\.ship\20260909-222353`; target zip `Siegebound-Win64-Shipping-2026-09-09.zip`; HEAD `31edc23…`; dirty paths 12 (listed) |
| A2-NO-MID-OPERATION | `unresolved git state markers: none`; `Editor processes: 0` |
| A3-QUIET-MODULE | `build/cook processes live: none`; `Staged-game processes: none running from the stage` (A3 still does not classify a GUI/`-game` `UnrealEditor.exe` — `TASK-1199`; the hand classification in §2 covers it this run) |
| **A4-FENCE (`PKG-§7a`, proven this run)** | `route (ii): ignored by .gitignore:19:packagedZIPofGame/  "C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\packagedZIPofGame"` |
| A5-DISK | `1289.8 GB free on C: (need 4)` |
| A6-EVIDENCE-ROUTE | `bUseLoggingInShipping: absent - correct`; engine class INSTALLED (Launcher) ⇒ Shipping log-silent; `route PIXEL (requested explicitly); instrument: screen capture OK, primary screen 1536x960; PKG-6a arena bar UNCHANGED` |
| A7-COOKDIRS / A7-MAPS / A7-BOOTMAP | `11 COOKDIR entries; missing: none` · `maps: /Game/Maps/L_MainMenu+/Game/Maps/L_Arena; missing: none` · `GameDefaultMap: /Game/Maps/L_MainMenu.L_MainMenu` |
| **A8-DESKTOP — STOP** | `Desktop input: LOCKED - root window under (768,480): class 'LockScreenBackstopFrame', owner 'explorer'` → `[STOP] A8-DESKTOP THE DESKTOP IS LOCKED … Route Pixel reaches the arena by CLICKING Play in the shipped menu (PKG-9f) and simulated input cannot land on a locked session (TASK-076). Checked here so it costs seconds, not a 30-minute cook.` · `REQUIRED TO CLEAR: Unlock the desktop and re-run, or schedule the ship for a time it is unlocked … Do NOT fall back to the map argument to dodge this.` |

Hand re-probe of the same predicate at **15:24:54 PDT** (read-only P/Invoke `GetAncestor(WindowFromPoint(768,480), GA_ROOT)`): `class='LockScreenBackstopFrame' owner='explorer' (pid 14392)`; foreground window owner `Idle`; **`LogonUI` alive = True**; `query session` → console session 2 `wesel` Active. ⇒ the lock is standing, not a transient — Jonathan locked the machine after clearing the cook. Note the shape (`SHIP-§9b`): `query session` reports the console `Active` on a locked machine; the root-window class is the probe that told the truth, exactly as TASK-716 measured.

## 4. Phases B and C — NOT REACHED

- B1-COMPILE: not run. B2-SUITE: not run (the last live reading remains invocation 2's `[PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log); timedOut=False`).
- UAT verdict lines: none (UAT not invoked). Cook duration: n/a. Stage: unchanged from the Aug-30 hand cook. `Config/` listing: not re-measured this invocation (the `PKG-§12` standing answer from part 2 §6 stands; re-measured on the run that produces a stage).
- **C3 capture paths: none exist.** No adjudication is pending; no record written.

## 5. `L_Arena` and the previous artifact

- `Content/Maps/L_Arena.umap` sha256 **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** before (15:23:52) and after (15:23:55) — unmoved; mtime `2026-09-05 00:42:58`.
- `Siegebound-Win64-Development-2026-08-29.zip` 1,409,955,049 B, mtime Aug 29 17:16 — untouched (`PKG-§7b`).

## 6. Route

Not a code failure ⇒ no programmer dispatch. Remedy = the gate's own text: Jonathan unlocks the desktop (and keeps every `UnrealEditor.exe` down for ~1 h, `SHIP-§10`), then invocation 3 is re-launched **verbatim** — same command, the hand classification re-run in-process first, nothing closed. Asked in 🚨 Blockers (thread `1783116296.221319`); the host polls the lock predicate (session scratch `deskprobe.ps1`, read-only) and relaunches on unlock within its window. Nothing staged, committed or pushed by this invocation; `main` 1 ahead.

## 7. Artefacts (absolute)

- `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\9ea39ad5-585f-4ec7-9dc8-126791d0a5a4\scratchpad\{launch3.ps1, live3.log, deskprobe.ps1}` (session scratch; `live3.log` is the full stdout incl. the SHIP SUMMARY block)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-222353\` (created by A1-STAGING; empty)

---

# PART 3 (continued) — invocation 3, ATTEMPT 2 (2026-09-09 22:06:28–22:09:50 PDT, same word from Jonathan relayed by the orchestrator: *"I am not doing any playtesting… you are clear to start the cook"* — `SC-§118` cl. 6), run log dir `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-050628\`

**Outcome: `SHIP RESULT: STOP at C3-CAPTURE - MECHANICAL PRE-FILTER ONLY (SHIP-8c) - THIS IS NOT A PASS OF C3-BOOT-ARENA AND CAN NEVER BECOME ONE. the captured frame is UNIFORMLY BLANK - one colour across the whole sample grid | the menu click was NOT delivered: ABORTED - the window under the click point is NOT the game (root class 'LockScreenBackstopFrame'). NO INPUT WAS INJECTED. - the capture cannot show an arena`, `POWERSHELL-EXIT=2` at 22:09:50.228. THE DESKTOP LOCKED MID-RUN: unlocked at A8 (22:06:28), locked when the rig reached the Play button (~22:09:45). Machine state, NOT a code failure, NOT routing rule 6. Everything upstream PASSED for the first time in this project's history: the Shipping target COMPILED, the cook COMPLETED, UAT said `BUILD SUCCESSFUL`, the stage was verified. Nothing adjudicable exists (both captures are black lock-screen frames). Previous zip untouched. Nothing of any class was closed by me. The earlier `.ship\20260909-222353\` (attempt 1, the A8 stop) was not resumed and stays empty.**

## 1. `SC-§91` — the tree at this attempt's instant

| Instrument | Reading |
|---|---|
| `git rev-parse HEAD` | **`31edc23968f8ee9766b4155c5b13a6cc354427de`** — matches the brief |
| `git rev-list --left-right --count origin/main...main` | **`0  1`** — `main` 1 ahead, 0 behind, not pushed |
| `git status --porcelain` (git root one level up, `SC-§102`), by hand at 22:03 and by the script at 22:06:28 | 12 paths, identical to attempt 1's list (` M .claude/commands/ship.md` · ` M .claude/pipeline/CONVENTIONS.md` · ` M .claude/pipeline/TASKBOARD.md` · ` M .claude/pipeline/qa/TASK-1193-cook.md` · ` M CLAUDE.md` · `?? handoffs/TASK-1190-census.md` · `?? handoffs/TASK-1190-programmer.md` · `?? handoffs/TASK-1191-programmer.md` · `?? handoffs/TASK-1193-buildmaster.md` · `?? handoffs/TASK-1197-buildmaster.md` · `?? qa/TASK-1192-report.md` · `?? Docs/Packaging/`, all `GitClaudeUnrealTest/`-prefixed). **`Tools/Packaging/ship.ps1` CLEAN.** Build-relevant dirt: `0 path(s), hash e3b0c44298fc`. |
| Mid-operation markers | none |
| Prior ship state | none at the stable path (`ship-state.json` / `ship-adjudication.json` absent) ⇒ `Resume decision: no prior state - PHASE B and C will run in full` |

## 2. The hand classification (`SHIP-§10` cl. 3 / `SC-§118` cl. 1), verbatim — by COMMAND LINE, never by name or title

First pass, before anything else (PowerShell `Get-CimInstance Win32_Process`, name filter `UnrealEditor.exe | UnrealEditor-Cmd.exe | UnrealBuildTool | AutomationTool | UnrealPak | ShaderCompileWorker | GitClaudeUnrealTest*` plus any `dotnet.exe` whose command line names UAT/UBT; each hit to be classed `-game` ⇒ game (Jonathan) / no `-game` ⇒ editor / `-Cmd` ⇒ commandlet):

```
CENSUS AT 2026-09-09 22:03:02 -07:00
ZERO matching processes (UnrealEditor.exe / UnrealEditor-Cmd.exe / UBT / UAT / UnrealPak / SCW / GitClaudeUnrealTest*)

LOGONUI: 0
SESSION:  USERNAME              SESSIONNAME        ID  STATE   IDLE TIME  LOGON TIME
>wesel                 console             2  Active    3+13:42  9/6/2026 8:22 AM
```

Desktop pre-probe (read-only P/Invoke, the A8 predicate itself) at 22:05:07.106: `root window class='CASCADIA_HOSTING_WINDOW_CLASS' owner='WindowsTerminal' (pid 13980)`, `LogonUI alive: False`, `LockApp alive: True` (a background process, not diagnostic — the predicate is the root window), verdict `not locked`.

Second pass, in-process immediately before the launch (`launch3b.ps1`, session scratch; it REFUSES and exits 3 on any hit; it contains no `Stop-Process`, `taskkill` or `Kill()`):

```
PRECHECK (hand classification, SHIP-10 cl. 3 / SC-118 cl. 1) at 2026-09-09 22:06:27.748 -07:00
RESULT: ZERO instances of UnrealEditor.exe / UnrealEditor-Cmd.exe / UnrealBuildTool / AutomationTool / UnrealPak / CrashReportClientEditor / ShaderCompileWorker / dotnet(UAT|UBT)
L_Arena before: 1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622
HEAD: 31edc23968f8ee9766b4155c5b13a6cc354427de
AHEAD-BEHIND origin/main...main: 0	1
LAUNCH at 2026-09-09 22:06:28.107 -07:00
CMD: powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1 -Configuration Shipping -BootEvidence Pixel
```

Nothing was closed by me at any point. The standing grant was not exercised (suspended for this run, `SC-§118` cl. 6). The only kills in the run are `ship.ps1`'s own, of the two game processes it launched itself (§7) — resolved by path, pre-existing PIDs excluded, never an `UnrealEditor.exe`.

## 3. Phase A — every gate with the script's own evidence (13/13 PASS)

| Gate | Evidence (verbatim) |
|---|---|
| A1-UPROJECT | `1 .uproject file(s) under C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest` |
| A1-REPO | `git rev-parse --show-toplevel` → `C:\GitProjects\GitHub\GitClaudeUnrealTesting` |
| A1-ENGINE | `C:\Program Files\Epic Games\UE_5.8` |
| A1-STAGING | staging `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame` (first existing of 3 probes); run log dir `…\.ship\20260910-050628`; target zip `Siegebound-Win64-Shipping-2026-09-09.zip`; HEAD `31edc23…`; dirty paths 12 (listed) |
| A2-NO-MID-OPERATION | `unresolved git state markers: none`; `Editor processes: 0` |
| A3-QUIET-MODULE | `build/cook processes live: none`; `Staged-game processes: none running from the stage` (A3 still does not classify a GUI/`-game` `UnrealEditor.exe` — `TASK-1199`; §2 covers it by hand) |
| **A4-FENCE (`PKG-§7a`, proven this run)** | `route (ii): ignored by .gitignore:19:packagedZIPofGame/  "C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\packagedZIPofGame"` |
| A5-DISK | `1287.5 GB free on C: (need 4)` |
| A6-EVIDENCE-ROUTE | `bUseLoggingInShipping: absent - correct`; `Engine class: INSTALLED (Launcher) … Shipping is LOG-SILENT`; `route PIXEL (requested explicitly); instrument: screen capture OK, primary screen 1536x960; PKG-6a arena bar UNCHANGED - only the instrument changed; ends in CALLER ADJUDICATION at C3-BOOT-ARENA, it never self-passes` |
| A7-COOKDIRS / A7-MAPS / A7-BOOTMAP | `11 COOKDIR entries; missing: none` · `maps: /Game/Maps/L_MainMenu+/Game/Maps/L_Arena; missing: none` · `GameDefaultMap: /Game/Maps/L_MainMenu.L_MainMenu` |
| A8-DESKTOP | `route Pixel: no lock-screen owner found at the primary-screen centre (root window under (768,480): class 'CASCADIA_HOSTING_WINDOW_CLASS', owner 'WindowsTerminal'). POSITIVE LOCK DETECTOR ONLY - this is not a proof that the click will land; the rig re-tests the exact point before it injects anything.` |

## 4. Phase B

- **B1-COMPILE: PASS** — `log-parsed verdict: Succeeded=True Failed=False timedOut=False (exit code deliberately ignored)`; `compile.out.log` 1,072 B (Development editor target up to date), 22:06:29–22:06:31.
- **B2-SUITE: PASS — verbatim, the fifth live reading of the repaired gate:**
  `[PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log); timedOut=False`
  22:06:31–22:08:04; `automation.log` 1,183,748 B, `suite.out.log` 363,345 B, `suite.err.log` 0 B, `automation-report\` written.

## 5. Phase C — the cook: THE FIRST SHIPPING COMPILE IN THIS PROJECT'S HISTORY, AND IT PASSED (`SC-§111` gated at last)

- Command: the script's printed `RunUAT.bat BuildCookRun -project="…\GitClaudeUnrealTest.uproject" -nop4 -utf8output -platform=Win64 -clientconfig=Shipping -build -cook -map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena -pak -stage -prereqs -archive -archivedirectory="…\packagedZIPofGame" -AdditionalCookerOptions="-COOKDIR=… ×11 (Data, UI, Blueprints, Input, Characters, Meshes, Materials, Textures, VFX, Audio, LevelPrototyping)"` — the `PKG-§5a` recipe verbatim from `ship.ps1`, never retyped.
- **UBT (`cook.out.log:10-64`): one invocation, two targets** — `GitClaudeUnrealTestEditor Win64 Development` (up to date) + `GitClaudeUnrealTest Win64 Shipping`. `** For GitClaudeUnrealTest-Win64-Shipping **`: `[1/20]…[18/20] Compile [x64] Module.GitClaudeUnrealTest.{1..18}.cpp` (all 18 unity modules), `[19/20] Link [x64] GitClaudeUnrealTest-Win64-Shipping.exe`, `[20/20] WriteMetadata GitClaudeUnrealTest-Win64-Shipping.target`. `Total time in Unreal Build Accelerator local executor: 24.86 seconds` · **`Result: Succeeded`** · `Total execution time: 25.74 seconds` · `Took 26.00s to run dotnet.exe, ExitCode=0` · `Build command time: 26.02 s`. Toolchain: VS 14.50.35735, Windows 10.0.22621.0 SDK. **Zero compile errors, zero link errors** — the fog floor, `Siege.Fog.*`, the Graphics menu and the knight all met a cooked target and compiled.
- **Cook:** `UnrealEditor-Cmd.exe … -run=Cook -Map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena -TargetPlatform=Windows -unversioned -COOKDIR=… ×11`; `Cooked packages 1400 Packages Remain 0 Total 1400`; `Took 32.64s to run UnrealEditor-Cmd.exe, ExitCode=0`; `Cook command time: 32.73 s` (warm DDC from the Aug-30 hand cook; shadermaps recompiled for the fog/hero materials).
- **Stage/archive:** `Stage command time: 13.32 s` (UFS 3540 + NonUFS 40 + Debug 3 manifest items) · `Archive command time: 0.56 s` · **`BuildCookRun time: 73.51 s`**.
- **UAT's own verdict lines, verbatim (`cook.out.log:993-995`):** `BUILD SUCCESSFUL` · `AutomationTool executed for 0h 1m 14s` · `AutomationTool exiting with ExitCode=0 (Success)`. Census of the 102,293 B log: `Error:` lines **0**, `Warning:` lines 6. `cook.err.log` 0 B.
- **Cook duration: 1 m 14 s** (22:08:05 → 22:09:19 PDT). Whole run launch→STOP: 3 m 22 s.
- Gates: **C1-COOK PASS** (`UAT finished within 90 min`) · **C2-UAT-LOG PASS** (`UAT's own verdict lines: BUILD SUCCESSFUL=True BUILD FAILED=False (exit code deliberately ignored)`) · **C2-STAGE-PRESENT PASS** (`staged click target …\packagedZIPofGame\Windows\GitClaudeUnrealTest.exe; game binary GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe`) · **C2-STAGE-MANIFESTS PASS** (`3 manifest file(s) … listing 3583 path(s)`) · **C2-STAGE-PRUNE PASS** (`no orphans - every staged .exe/.pdb is manifest-listed`) · **C2-STAGE-ONE-EXE PASS** (`1 runnable game exe(s) [GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe] + 1 root shim(s) [GitClaudeUnrealTest.exe]` — `PKG-§10` measured). C4-NO-MODELS not reached by the script (it follows C3) — measured by hand in §6.

## 6. The stage (`packagedZIPofGame\Windows\`) — summary, `Config/` listing, `PKG-§4`

- Executables: `GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe` **178,076,160 B, 2026-09-09 22:08:31** (the Aug-30 hand cook's 177,716,736 B was replaced) · `.pdb` 252,080,128 B · root shim `GitClaudeUnrealTest.exe` 172,032 B (22:09:05) · `Engine\Extras\Redist\en-us\vc_redist.{x64,arm64}.exe` (prereqs). Exactly one runnable game exe (`PKG-§10`).
- Paks: `GitClaudeUnrealTest-Windows.pak` 11,466,608 B · `.ucas` 1,103,375,200 B · `.utoc` 687,265 B · `global.ucas` 3,318,560 B · `global.utoc` 806 B (all 22:09:12–22:09:18).
- **`PKG-§4` by hand: `.gguf` = 0, `Models` directories = 0.**
- **`Config/` in the stage — `UnrealPak -List` of the `.pak` (1,745 entries): 82 `.ini` entries, of which exactly FOUR are project inis: `GitClaudeUnrealTest/Config/DefaultEngine.ini` (3,471 B) · `DefaultGame.ini` (673 B) · `DefaultInput.ini` (1,011 B) · `SiegeCloudDev.ini` (290 B, sha1 `E6BF5EFE…`)**; the other 78 are engine/plugin inis. Loose (non-pak) inis: `Engine\Config\StagedBuild_GitClaudeUnrealTest.ini` and `Engine\Saved\Config\Windows\Manifest.ini` only. No editor inis, no `.example`.
- **`SiegeCloudDev.ini` — measured on the STAGED BYTES, not the source:** `UnrealPak -Extract -Filter=*SiegeCloudDev.ini` into the scratchpad → `GitClaudeUnrealTest/Config/SiegeCloudDev.ini`, **290 B, 3 lines: a BOM-prefixed `[SiegeCloud]` header · `ProjectUrl` (42-char value) · `AnonKey` (208-char value) — and nothing else** (the cook strips the source's comments; source `Config/SiegeCloudDev.ini` is 1,705 B, 2026-08-23, untracked/ignored in git). The extracted copy was deleted immediately after the key-name read. **The `PKG-§12` prediction holds: the build carries `ProjectUrl` + `AnonKey` and nothing beyond them. Values reproduced nowhere.**

## 7. C3 — the drive, the STOP, and the measured cause

- `launching (route Pixel - PKG-9f: NO ARGUMENTS, exactly a player's double-click): …\packagedZIPofGame\Windows\GitClaudeUnrealTest.exe` → `game window: found; title 'Siegebound  '` → `clicking menu entry 1 of 7 (Play vs Bot) at client-relative (0.5000, 0.2929)` → **`menu click: NOT DELIVERED - ABORTED - the window under the click point is NOT the game (root class 'LockScreenBackstopFrame'). NO INPUT WAS INJECTED.`**
- `Game processes (by path): 2 live under the stage: pid 25228 GitClaudeUnrealTest, pid 11744 GitClaudeUnrealTest-Win64-Shipping` (both launched by the rig; both killed by the rig afterwards, by PID; none alive at 22:11:26) · `Window title: Siegebound [read from pid 11744 GitClaudeUnrealTest-Win64-Shipping]` · **C3-BOOT-TITLE PASS**.
- **STOP C3-CAPTURE** (verbatim in the outcome line above). Captures: `…\.ship\20260910-050628\bootverify-menu-preclick-NOT-ADJUDICABLE.png` (7,575 B, 22:09:45) and `…\bootverify-arena.png` (7,575 B, 22:09:47) — **both read by eye: uniformly black 1536×960 frames (a locked session captures black).** Nothing adjudicable; no `ship-state.json` written (the STOP precedes the state write); no adjudication pending.
- **Why the desktop was locked 3 minutes after A8 said it was not — measured, not guessed (22:11:26 probe):** root window `LockScreenBackstopFrame`/`explorer`, `LogonUI` alive ⇒ locked; **`GetLastInputInfo`: last keyboard/mouse input 523.5 s earlier = 22:02:43 PDT** (Jonathan's go-ahead keystrokes). Policy: `HKCU\Control Panel\Desktop` `ScreenSaveActive=1` but no `SCRNSAVE.EXE`/`ScreenSaveTimeOut` (no screensaver); `InactivityTimeoutSecs` unset; dynamic lock unset; **`powercfg` VIDEOIDLE: AC `0x12c` = 300 s, DC `0xb4` = 180 s** — the display turns off at 5 min idle and this machine locks the session with it. Timeline: A8 at 3 m 45 s idle → unlocked; the rig at ~7 m idle → locked. Attempt 1 (15:23) fits the same shape. `Security` 4800/4801 events not readable from this account. ⇒ **On this machine a Pixel-route ship must reach C3 within ~5 min of the last human input, or someone must be at the keyboard.** This run reached C3 at +3 m 17 s from launch — a launch inside ~1.5 min of his last keystroke would have made it; that is timing, not a remedy (`SC-§118` cl. 3's doctrine) — the remedy is his presence or his own change to the timeout, both his call.
- `SHIP-§9b` note for the manager: `A8-DESKTOP` is a positive lock detector at PHASE A and worked; it cannot see a lock that arrives later. The rig's abort predicate is what caught it, exactly as designed — **the gate family is intact; the cost `PKG-§9f` names ("schedulable, not unattended") now has a measured number on this machine: 300 s.**

## 8. `L_Arena` and the previous artifact

- `Content/Maps/L_Arena.umap` sha256 **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** before (22:06:27) and after (22:09:50) — unmoved. `Content/` porcelain empty.
- `Siegebound-Win64-Development-2026-08-29.zip` 1,409,955,049 B — untouched (`PKG-§7b`). No Shipping zip exists yet (Phase D never reached, by design).

## 9. Route

Not a code failure ⇒ **no programmer dispatch**; `qa/TASK-1193-cook.md` not appended (there is no cook failure to record — the cook record's INVOCATION 2 sentence "the Shipping target has still never compiled" is superseded by §5 here). Remedy = the gate's own text: the desktop unlocked **and kept unlocked through C3** — asked in 🚨 Blockers 22:13 PDT (unlock and stay ~4 min, hands off during the ~30 s the game owns the screen; opt-outs offered: reply *wait* / name a time / lengthen the timeout himself / discharge by eye per `SHIP-§8b(7)` on the staged shim). The host watches the lock predicate and relaunches the identical command on unlock (in-process hand classification first, refuse on any instance, nothing closed). Nothing staged, committed or pushed by this attempt; `main` 1 ahead. Slack: 🔧 launch `p1789016803552049` · 🔧 verdict `p1789017198861939` · 🚨 ask `p1789017207314289`.

## 10. Artefacts (absolute)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-050628\{compile.out.log, compile.err.log, suite.out.log, suite.err.log, automation.log, automation-report\, cook.out.log (102,293 B), cook.err.log (0 B), bootverify-menu-preclick-NOT-ADJUDICABLE.png, bootverify-arena.png}`
- `C:\Users\wesel\AppData\Roaming\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.8\{UBA-GitClaudeUnrealTestEditor-Win64-Development.txt, Log.txt, FinalCopyWin64_*.txt}` · cook commandlet log `C:\Program Files\Epic Games\UE_5.8\Engine\Programs\AutomationTool\Saved\Cook-2026.09.09-22.08.32.txt`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\` — the verified Shipping stage at `31edc23`
- `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\9ea39ad5-585f-4ec7-9dc8-126791d0a5a4\scratchpad\{launch3b.ps1, live3b.log}` (session scratch; `live3b.log` is the full stdout incl. the SHIP SUMMARY block)

## 11. Stand-down (23:08 PDT)

The lock predicate was polled every 10 s from 22:13 to 23:07 PDT (read-only P/Invoke; the same root-window test A8 and the rig use): `LOCKED|LockScreenBackstopFrame|explorer|LogonUI=True` at every reading, idle counter 523 s → 3,866 s (last input still 22:02:43). No reply in 🚨 Blockers. Zero `UnrealEditor*`/game processes at 23:07. The watcher was stopped and a stand-down note posted in 🚨 (fresh dispatch on his word/unlock; identical command; hand classification in-process first; nothing closed; ~3.5 min; he must stay at the keyboard through C3, hands off during the ~30 s the game owns the screen — or discharge by eye on the staged shim, `SHIP-§8b(7)`). Board `- status:` line updated accordingly. `L_Arena` `1f78419d…0af15622` unchanged at stand-down. Nothing staged, committed or pushed; `main` 1 ahead.

---

# PART 4 — THE RESUME (Phases D → E → F → G), 2026-09-09 late, on Jonathan's `SHIP-§8b(7)` adjudication by eye

**His sentence, verbatim (relayed by the orchestrator from Claude Code):** *"yes, the deck, HUD, and hero all are working, go ahead and update the Zip with the updated game and update the README with the specifications I gave earlier."* Followed, while this part was being written: *"I closed the game."*

**What he adjudicated:** the staged build at `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\GitClaudeUnrealTest.exe` as left by invocation 3 attempt 2 (cook of 22:08–22:09 PDT, run dir `.ship\20260910-050628`), run by him by double-click after the 23:08 stand-down and before 23:31 PDT (the census below read idle 117 s at 23:31:06).

## 0. `SC-§118` cl. 6 — hand classification BEFORE anything else (nothing closed, nothing to close)

```
CENSUS AT 2026-09-09 23:31:06  (Get-CimInstance Win32_Process; names UnrealEditor* / GitClaudeUnrealTest* / *Win64-Shipping* / UnrealBuildTool* / AutomationTool* / UnrealPak* / ShaderCompileWorker*)
ZERO instances of UnrealEditor*/GitClaudeUnrealTest*/*Win64-Shipping*/UBT/UAT/UnrealPak/SCW
idle seconds (GetLastInputInfo): 117
```

The staged game he ran to adjudicate was NOT running ⇒ the stage is not locked by his session ⇒ no STOP, no question owed in 🚨. The in-process precheck of the launcher (§2) re-classifies immediately before the script starts and refuses on any hit; it contains no kill of any kind.

## 1. `SHIP-§8d` same-build rule — the ADJUDICATED stage, hashed BEFORE the resume launched (`Get-FileHash -Algorithm SHA256`, 23:31 PDT)

| File (under `packagedZIPofGame\Windows\`) | sha256 (BEFORE) | bytes | mtime |
|---|---|---|---|
| `GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe` | `9965A35AE1F349F604837B642D6A4FEBC2E8EBE624802DBFEC559835B4FEFF2C` | 178,076,160 | 2026-09-09 22:08:31 |
| `GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.pak` | `E0CCFFDECCCD1EB6F5E837CB02ACEB8706DCCFD6D8C6DB864003C277CAEE6464` | 11,466,608 | 2026-09-09 22:09:12 |
| `GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.ucas` | `7DB7A5DDA0160AEB40512CEB994B61464409E9BB44F42608B36F2FBF529B0EFA` | 1,103,375,200 | 2026-09-09 22:09:18 |
| `GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.utoc` | `2DE83D7EBD77774210BF63297AC2E457D036A89EDC95D5B9931AF0E33CABDD7A` | 687,265 | 2026-09-09 22:09:18 |

Also recorded (not part of the four-row rule, for reconciliation): root shim `GitClaudeUnrealTest.exe` `7F2C8FD657C18E6031F0C1EEF8DACB86A4134630C99A45BEE61BD99B3300F4B5` 172,032 B 22:09:05 · `global.ucas` `3722B03E3D1FFBFAA1D2762E640A8FD103519B17325FDB56F338413083EBF266` 3,318,560 B · `global.utoc` `129E060757099CEB3A41CF75B6421362A471388EB09337C1DCB3251568D6BA9B` 806 B.

Beside them, `SC-§91` at this instant (git root one level up, `SC-§102`): `HEAD` = `31edc23968f8ee9766b4155c5b13a6cc354427de` · `origin/main...main` left/right = `0 1` (main 1 ahead, 0 behind, not pushed) · build-relevant porcelain (`Source/ Content/ Config/ Plugins/`) = EMPTY — zero build-relevant dirt (the script's tree hash should again read `e3b0c44298fc`).

No `ship-state.json` and no `ship-adjudication.json` exist at `packagedZIPofGame\.ship\` (the C3-CAPTURE STOP preceded the state write). `ship.ps1:97-100` reuses B+C only when *"a state file proves they were run for a BYTE-IDENTICAL build input"*, and there is no documented way to resume against an existing stage without one ⇒ the resume below re-runs B+C in full and produces a NEW stage; the AFTER hashes and the four-row `MATCH`/`DIFFER` table follow in §3. ⛔ No state file or record was hand-written (`SHIP-§8d` cl. 4).

## 2. The resume launch (invocation 4, 2026-09-09 23:35:40 PDT) — `launch4.ps1` (session scratch; precheck refuses on any hit, contains no kill), run log dir `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540\`

```
PRECHECK (hand classification, SHIP-10 cl. 3 / SC-118 cl. 1) at 2026-09-09 23:35:40.218 -07:00
RESULT: ZERO instances of UnrealEditor.exe / UnrealEditor-Cmd.exe / UnrealBuildTool / AutomationTool / UnrealPak / CrashReportClientEditor / ShaderCompileWorker / GitClaudeUnrealTest*.exe / dotnet(UAT|UBT)
L_Arena before: 1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622
HEAD: 31edc23968f8ee9766b4155c5b13a6cc354427de
AHEAD-BEHIND origin/main...main: 0 1
LAUNCH at 2026-09-09 23:35:40.660 -07:00
CMD: powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1 -Configuration Shipping -BootEvidence Pixel
```

Nothing was closed by me at any point (`SC-§118` cl. 6). The only kills in the run are `ship.ps1`'s own, of the two game processes it launched itself (pid 12364 shim, pid 26944 `-Win64-Shipping`), by PID; none alive at 23:39:52.

| Gate | Evidence (the script's own lines, `live4.log`) |
|---|---|
| A1–A8 | **13/13 PASS.** A4-FENCE `route (ii): ignored by .gitignore:19:packagedZIPofGame/` (proven this run) · A5 `1286.9 GB free` · A6 `route PIXEL … primary screen 1536x960` · A7 11 COOKDIRs / both maps / boot map · A8 `no lock-screen owner … root window under (768,480): class 'Chrome_WidgetWin_1', owner 'claude'` (unlocked — his Claude window was foreground) |
| Build-relevant dirt / Resume decision | `0 path(s), hash e3b0c44298fc` · `Pixel verdict record: absent` · **`Resume decision: no prior state - PHASE B and C will run in full`** (`ship.ps1:97-100` — no documented no-cook resume exists without a state file) |
| B1-COMPILE | PASS — `Target is up to date`, `Result: Succeeded`, 1.17 s (`compile.out.log`) |
| B2-SUITE | **PASS — `555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log); timedOut=False`** (23:35:41–23:37:0x) |
| C1-COOK / C2-UAT-LOG | PASS / PASS — `BUILD SUCCESSFUL=True BUILD FAILED=False`. `cook.out.log` (88,190 B): UBT `Target is up to date` → `Result: Succeeded` 0.92 s (**the Shipping exe was NOT relinked** — mtime stays 22:08:31); `Cooked packages 1400 Packages Remain 0 Total 1400`; `Cook command time: 24.07 s`; `BuildCookRun time: 38.63 s`; `AutomationTool exiting with ExitCode=0 (Success)`; census `Error:` lines **0**, `Warning:` lines 6; `cook.err.log` 0 B |
| C2-STAGE-PRESENT / MANIFESTS / PRUNE / ONE-EXE | PASS ×4 — click target `Windows\GitClaudeUnrealTest.exe` (manifest-resolved), game binary `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe` (manifest-resolved), 3 manifests / 3583 paths, `no orphans`, `1 runnable game exe(s) + 1 root shim(s)` (`PKG-§10`) |
| C3 drive (`PKG-§9f`) | `launching (route Pixel - PKG-9f: NO ARGUMENTS, exactly a player's double-click)` → `game window: found; title 'Siegebound  '` → `clicking menu entry 1 of 7 (Play vs Bot) at client-relative (0.5000, 0.2929)` → **`menu click: DELIVERED - delivered 2 press(es) at client (768,281) of 1536x960 -> screen (768,281)`** → `waiting 90s for level travel and the HUD to settle` |
| C3-BOOT-TITLE | PASS — `Window title: Siegebound [read from pid 26944 GitClaudeUnrealTest-Win64-Shipping]` (`PKG-§8`) |
| C3-CAPTURE | **CHECK** (pre-filter, may only fail) — `bootverify-arena.png, 751,684 B, 494 distinct sampled colours, process live, title read` |
| C3-BOOT-ARENA | **ADJUD — `SHIP RESULT: ADJUDICATE C3 - …\.ship\20260910-063540\bootverify-arena.png`, `POWERSHELL-EXIT=4` at 23:39:37.706.** Bindings printed: `captureSha256 f9ecd3370e83798385aab4224b81a1f1e4f95fbcf7c5806b372e3a02bf147cb8` · `head 31edc23…` · `config Shipping` · `stageExeBytes 178076160` · `stageExeUtc 2026-09-10T05:08:31.6006548Z`. **`ship-state.json` WRITTEN BY THE SCRIPT (936 B, 23:39:37): `compile PASS · suiteTotal 555 · cook PASS · boot ADJUDICATE`, `treeHash e3b0c442…`, `recipeHash 44f607c9…`.** |
| `L_Arena` | `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` before (23:35:40) and after (23:39:37) — unmoved |

Whole run launch→ADJUDICATE: **3 m 57 s**. No zip written by the script (C3 precedes D — by design); the Development zip untouched.

## 3. `SHIP-§8d` same-build rule — the four-row table (AFTER hashes at 23:39:52 PDT, `Get-FileHash -Algorithm SHA256`)

| File | BEFORE (adjudicated, §1) | AFTER (re-cooked) | bytes | mtime AFTER | Verdict |
|---|---|---|---|---|---|
| `…\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe` | `9965A35A…4FEFF2C` | `9965A35AE1F349F604837B642D6A4FEBC2E8EBE624802DBFEC559835B4FEFF2C` | 178,076,160 | 22:08:31 (not relinked — UBT up to date) | **MATCH** |
| `…\Content\Paks\GitClaudeUnrealTest-Windows.pak` | `E0CCFFDE…CAEE6464` | `E0CCFFDECCCD1EB6F5E837CB02ACEB8706DCCFD6D8C6DB864003C277CAEE6464` | 11,466,608 | 23:37:49 (rewritten, byte-identical) | **MATCH** |
| `…\Content\Paks\GitClaudeUnrealTest-Windows.ucas` | `7DB7A5DD…529B0EFA` | `7DB7A5DDA0160AEB40512CEB994B61464409E9BB44F42608B36F2FBF529B0EFA` | 1,103,375,200 | 23:37:54 (rewritten, byte-identical) | **MATCH** |
| `…\Content\Paks\GitClaudeUnrealTest-Windows.utoc` | `2DE83D7E…33CABDD7A` | `2DE83D7EBD77774210BF63297AC2E457D036A89EDC95D5B9931AF0E33CABDD7A` | 687,265 | 23:37:54 (rewritten, byte-identical) | **MATCH** |

Reconciliation rows (not part of the rule): root shim `7F2C8FD6…300F4B5` 172,032 B (23:37:43) MATCH · `global.ucas` `3722B03E…BEBF266` MATCH · `global.utoc` `129E0607…D6BA9B` MATCH · `.pdb` `53EAF924AFBBE34B46B0F2D0E84E443C9CC89BB3D98C1C34C9716C44B856032E` 252,080,128 B (22:08:31, not relinked). Same `HEAD` (`31edc23`), zero build-relevant dirt (`e3b0c442…`) both runs — necessary and, with the bytes, sufficient.

**⇒ ALL FOUR MATCH. The stage Jonathan adjudicated by eye and the stage in this zip are the same build, byte for byte. His adjudication carries (`SHIP-§8d` new bullet, item 3); the resume proceeds D → E → F → G by hand.** The `PKG-§12` measurement of part 3 §6 (four project inis in the pak; `SiegeCloudDev.ini` = `ProjectUrl` + `AnonKey` only, 290 B) carries with the byte-identical pak.

## 4. The rig's capture — adjudicated FAIL on its own terms; an instrument lead, not a build finding

- **What the two frames show (read by eye, `Read` tool, both 1536×960):** `bootverify-menu-preclick-NOT-ADJUDICABLE.png` — open blue sky with white clouds, **no menu, no text, no UI**. `bootverify-arena.png` (90 s after the delivered click) — open blue sky with white clouds in a different arrangement (time passed; the same sky), **no arena ground, no castle, no card bar, no readouts, no hero — no UI element of any kind**. Against the printed `PKG-§6a` bar, nothing is adjudicable; ambiguity is FAIL (`SHIP-§8b` rule 2/4).
- **The record I wrote (06:45:06Z), verdict `FAIL`, bound to `captureSha256 f9ecd337…`, `HEAD 31edc23…`, `Shipping`, `178076160`, `2026-09-10T05:08:31.6006548Z`, four affirmative "what I saw" observations + `reason` naming it an INSTRUMENT FINDING.** Re-invoked the script at 23:45:24 (precheck: zero instances) so it would read and retire the record: **`SHIP RESULT: STOP at A8-DESKTOP - THE DESKTOP IS LOCKED` in 2 s (23:45:26)** — the session had locked again ~5 min after his last keystroke (`PKG-§9f`'s 300 s), before the resume decision could read the record. The record therefore stayed armed at the stable path, which would STOP the next `/ship` at `C3-VERDICT-BINDING` once HEAD moves — so I archived it BY HAND, exactly where and as the script's own FAIL path does (`ship.ps1:2185-2199`): `Move-Item` → `…\.ship\20260910-063540\ship-adjudication.20260910-063540.consumed-FAIL.json` (sha256 `529E3982…54F027` identical before/after the move; archived, never deleted). `ship-state.json` NOT touched: it still reads `boot=ADJUDICATE` with no record, which is the script's own post-FAIL state (the next run re-cooks and re-captures). ⛔ No PASS record was written on his behalf: his eye was not on this capture, and a record binds to its capture.
- **The lead (`SC-§101` — stated as a lead, not a finding):** the primary display is **3840×2400 physical at 250 % scaling** (`Win32_VideoController` Intel Arc 140T `3840×2400`; `Screen.PrimaryScreen.Bounds` = `1536×960` in the DPI-unaware rig process; two further screens `DISPLAY2` `1280×720 @ X=-1280` and `DISPLAY3` `1280×720 @ X=2560`). A DPI-unaware `CopyFromScreen` of "1536×960" takes the **top-left 1536×960 PHYSICAL pixels = the top-left 40 % of the screen** — where the centred 7-entry menu and the arena's horizon/HUD both lie outside the frame, and where the L_MainMenu backdrop and the arena's upper field are both sky. Both captures fit that exactly; the click math (client-relative fractions, `SetCursorPos` in logical coordinates that Windows scales) does not suffer from it, which is why the click was DELIVERED and the game travelled (the process stayed alive 90 s, title read) while the capture saw sky. The rig launched under his saved `%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\Config\Windows\GameUserSettings.ini` (written 23:27:44 by his own sitting): `ResolutionSizeX=1568, ResolutionSizeY=862, FullscreenMode=1, sg.ResolutionQuality=0`. **For the manager:** `A6`'s instrument probe and `C3-CAPTURE` pass a capture that is a crop; the honest repair is a DPI-aware capture (physical bounds) or `SetProcessDPIAware` in the rig — a `ship.ps1` row behind `TASK-1201`, same family as `SHIP-§9b` (an instrument that answers correctly in the ordinary case and lies in exactly the case that matters). ⛔ Not re-adjudicated, not "close enough", not a bar change.
- **What carries:** Jonathan's `SHIP-§8b(7)` adjudication on the byte-identical build (§3). Both verdicts are on record; the README's *What was verified* states both, in that order, and says the rig capture was not used as evidence.

## 5. Measured this ship, outside the placeholders — the `Saved\` location the README's prose asserts is FALSE for this build

`Docs/Packaging/README-source.md` *Known notes* (uncited prose): *"The game writes its save data (accounts, decks, settings) into `Windows\GitClaudeUnrealTest\Saved\` next to the executable"*. **Measured after two runs of the staged Shipping build (his 23:2x sitting and the rig's 23:37 launch): `packagedZIPofGame\Windows\GitClaudeUnrealTest\Saved\` DOES NOT EXIST; the game wrote to `C:\Users\wesel\AppData\Local\GitClaudeUnrealTest\Saved\`** — `SaveGames\SiegeDecks.sav` 4,391 B (23:25:48) · `Config\Windows\GameUserSettings.ini` 1,378 B (23:27:44) · `GitClaudeUnrealTest_PCD3D_SM6.upipelinecache` (23:27:44) · `Config\CrashReportClient\…\CrashReportClient.ini` (23:37:57, the rig's launch). The staged `Engine\Config\StagedBuild_GitClaudeUnrealTest.ini` marks the build as staged/installed, which routes `ProjectSavedDir` to the user directory. ⛔ I do not edit the source's prose (routed to the manager, `TASK-1204` lane's note: a wording change is the manager's). What I own is `{{SHIP:VERIFIED}}` — a measurement made this ship — so the rendered README states the measured location there, beside the note it corrects, and the source gets a one-line fix + QA re-read on the next ship. The only stray non-manifest file in the stage, `Engine\Saved\Config\Windows\Manifest.ini` (25 B, `[Manifest] Version=2`, mtime 2026-08-29 17:12 — a UAT staging artefact, not boot-verify output; it was in the Aug-29 zip too), stays; the script's D1 purge targets `GitClaudeUnrealTest\Saved` only, which is absent.

## 6. Phase E — the README rendered BY HAND (`ship.md` §3 / `SHIP-§3a`), `render_readme.py` (session scratch) from `Docs/Packaging/README-source.md` (81,034 B, 946 lines — the file `qa/TASK-1192-report.md` loop 2 gated) → `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\README.md` (86,729 B, 968 lines, UTF-8 no BOM, LF)

| Placeholder | × | Value (from THIS ship's measurements) |
|---|---|---|
| `{{SHIP:ZIP_NAME}}` | 1 | `Siegebound-Win64-Shipping-2026-09-09.zip` (the name the script's A1-STAGING printed for this run) |
| `{{SHIP:DATE}}` | 1 | `2026-09-09` |
| `{{SHIP:CONFIG}}` | 3 | `Shipping` |
| `{{SHIP:SIZE}}` | 2 | `About 1.76 GB extracted (1,760,342,236 bytes, 70 files): game executable … 178,076,160 B · debug-symbol file … .pdb 252,080,128 B, shipped beside it · cooked content .pak 11,466,608 B + .ucas 1,103,375,200 B + .utoc 687,265 B (plus global.ucas/global.utoc, 3.3 MB) · the zip itself about 1.33 GB` (zip measured by a stage-only pass at 1,334,601,319 B before the render; the final archive is 1,334,631,291 B = 1.3346 GB — the figure holds) |
| `{{SHIP:HEAD}}` | 2 | `31edc23` (`git rev-parse HEAD` = `31edc23968f8ee9766b4155c5b13a6cc354427de` at this instant) |
| `{{SHIP:DIFF_BASE}}` | 2 | `22728c8` — `PKG-§11`'s measured value, USED; cross-check by the 2026-08-29 date bracket: `22728c8` ("final ship touches", 2026-08-29 17:28:54 -0700) is the last commit dated 2026-08-29 and an ancestor of `31edc23`; the Aug-29 zip (mtime 17:16) predates the evening's three commits (`24e0730` 17:19, `c5047cf` 17:20, `22728c8` 17:28), of which only `22728c8` touches a build-relevant path (`Config/DefaultEngine.ini` +3 — the ini change `PKG-§11` attributes to that package). The old README does NOT record its commit (grep: none). |
| `{{SHIP:VERIFIED}}` | 1 | Three instruments in order, player-facing, no law citations: (i) by machine — both runs' gate list (compile, 555/555 ×2, first-ever Shipping compile 18 modules/0 errors/25.7 s, cook 1400/1400 + `BUILD SUCCESSFUL` ×2, stage hygiene one exe + shim, no-args launch + title *Siegebound*, four project inis by name, four-file sha256 MATCH, zip read-back 71 entries); (ii) **by eye — Jonathan Wesely, 2026-09-09 23:08–23:31 PDT, his sentence VERBATIM** (§0), named as this package's boot verification on the same bytes; (iii) the rig's capture — "nothing usable, recorded rather than hidden": click delivered, both frames sky, judged FAIL on its own terms, not used as evidence, the 250 % DPI lead named; plus the measured `Saved\` location (§5) beside the note it corrects. |
| `{{SHIP:CHANGED_SINCE}}` | 1 | The check line (`git log 22728c8..31edc23`, 57 commits, read one by one, nothing removed) + five player-facing additions the visible 13-item draft lacked: notice range / leash + Fog shortening both · the Witch's 3-s interruptible cast + the bot honouring the veil + hidden-not-invulnerable · four-corner footprint slope · health-bar occlusion + camera-at-ladder floor (built/tested, not watched live) · the ladder re-aim (built/tested, not watched live). No task IDs, no law citations. The draft list beneath it is untouched. |
| `{{SHIP:CLOUD_SYNC}}` | 2 | `PKG-§12`'s pinned text verbatim (*"Available. The build carries the cloud project's address and its publishable key … Use Login → Link to Cloud / Sync Now."*) — the void conditions re-checked on the byte-identical pak (part 3 §6: four project inis; `SiegeCloudDev.ini` = `ProjectUrl` + `AnonKey` only). Values reproduced nowhere. |

- **`grep -c "{{SHIP:" packagedZIPofGame/README.md` → `0`** (and the renderer's own survivor count → 0; 15 occurrences / 9 distinct resolved). ⛔ Nothing else stripped: the 308 `<!-- src: -->` comment lines STAY (invisible when rendered; they are the README's citations).
- `PKG-§8` / `PKG-§10`: title `# Siegebound — Win64 Shipping build`; the click target `Windows\GitClaudeUnrealTest.exe` named **exactly once** (line 50, fixed-string grep = 1 — my first VERIFIED draft named it a second time and was corrected before the zip); the zip name named exactly once; UTF-8 integrity: 182 em dashes, 7 arrows, 0 replacement characters.
- The Aug-29 README (8,421 B) is backed up at the scratchpad (`README.2026-08-29.superseded.md`); the rendered README is out of tree and `git check-ignore -v` still answers `.gitignore:19:packagedZIPofGame/` for it, the zip and the shim (`PKG-§3`).

## 7. Phase D — the zip, by hand (`zip_stage.ps1`, session scratch; .NET `ZipArchive` — ZIP64-capable — `README.md` at the root + every staged file as `Windows/<rel>`, `Optimal`, written to `.partial` first, `SHIP-§4` read-back BEFORE the rename; ⛔ `Compress-Archive` not used)

- Purge: `Windows\GitClaudeUnrealTest\Saved\` — absent (the Shipping build saves to `%LOCALAPPDATA%`, §5); nothing to purge. Staged: **70 files, 1,760,342,236 B**.
- **`C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Siegebound-Win64-Shipping-2026-09-09.zip` — 1,334,631,291 B (1.33 GB decimal / 1.24 GiB), written in 40 s, mtime 23:50:35 PDT, sha256 `9CA197CB43581459436B18C4382AE2B889795DFFBD03E20E102779B450D70F19`.**
- **`SHIP-§4` read-back (on the `.partial`, before it took the name):** `entries=71 clickTarget=True binary=True README=True pak=1 ucas=2 utoc=2 models/gguf=0 exes=4 pdbs=1` — exes = `Windows/GitClaudeUnrealTest.exe` (shim) · the Shipping binary · `vc_redist.x64.exe` + `vc_redist.arm64.exe` (prereq installers, not game exes — `PKG-§10`'s one runnable GAME exe holds) · pdb = the Shipping `.pdb` only · `README.md` entry 86,729 B / 29,878 B compressed · **sum of entry lengths 1,760,428,965 = stage 1,760,342,236 + README 86,729 exactly** · entry count = 70 + 1. Then `Move-Item` → the final name. The README's own "71 entries" line was written before the zip and re-verified against it.
- `D2-SIZE-SANITY`: no previous Shipping zip to compare against (first of its configuration); against `PKG-§9d`'s Aug-29 Development baseline (1,409,955,049 B for a 1.91 GB stage) the Shipping archive is 94.7 % of it for a 1.76 GB stage — reconciles with the smaller Shipping exe (178 MB vs 348 MB) and the larger `.pdb` (252 MB vs the Development 383 MB → smaller). Nothing unexplained.
- **`PKG-§7b` / `D3-PRUNE`: Shipping zips present = 1 (keep 2) ⇒ pruned NONE. `Siegebound-Win64-Development-2026-08-29.zip` 1,409,955,049 B, mtime 2026-08-29 17:16 — PROTECTED, untouched.** Two zips on disk.
- `PKG-§4` in the archive: `.gguf` = 0, `Models/` = 0. The model on disk that is NOT in it: `GitClaudeUnrealTest/Models/Qwen3-4B-Q4_K_M.gguf` 2,497,280,256 B (`Get-Item`, 2026-08-02) = 2.50 GB decimal / 2.33 GiB — the README's "about 2.5 GB" stands.

## 8. Phase F — commit by explicit pathspec (`SHIP-§5`, cl. 5 as corrected by the manager), anchored at the git root one level up (`SC-§102`)

- `SC-§91` at the instant: `HEAD 31edc23`, `origin/main...main` = `0 1` (1 ahead, 0 behind). Porcelain: 5 modified (`ship.md` +25/−?, `CONVENTIONS.md` +52, `TASKBOARD.md` +128, `qa/TASK-1193-cook.md` +84, `CLAUDE.md` 1 line — his approved SLACK-grant clause correction) · 7 untracked (`handoffs/TASK-1190-census.md`, `TASK-1190-programmer.md`, `TASK-1191-programmer.md`, `TASK-1193-buildmaster.md`, `TASK-1197-buildmaster.md`, `qa/TASK-1192-report.md`, `Docs/Packaging/` = `README-source.md` only). **Index empty** (the UE Git plugin auto-stage trap did not fire — the editor has been closed all day).
- **`SC-§106` existence check, every path resolved against disk, with a control:** all 12 `EXISTS` (sizes in the log); the control `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1192.md` (the row's old wrong name) → `MISSING` — the check can go red.
- **Never-commit files proven clean:** `git status --porcelain -- Tools/Packaging/ship.ps1 Tools/run_suite_bounded.ps1 Tools Source Content Config` → empty. The staging dir appears neither staged nor untracked (`git status --porcelain -- packagedZIPofGame` empty; `check-ignore` answers). cl. 7a sweep: no other untracked pipeline record exists (`git status` lists exactly the 7 above, all on the pathspec).
- **The 12 paths:** `GitClaudeUnrealTest/Docs/Packaging/README-source.md` · `…/handoffs/TASK-1190-census.md` · `…/handoffs/TASK-1190-programmer.md` · `…/handoffs/TASK-1191-programmer.md` · `…/handoffs/TASK-1193-buildmaster.md` (this file) · `…/handoffs/TASK-1197-buildmaster.md` · `…/qa/TASK-1192-report.md` · `…/qa/TASK-1193-cook.md` · `…/TASKBOARD.md` · `…/CONVENTIONS.md` · `GitClaudeUnrealTest/.claude/commands/ship.md` · `GitClaudeUnrealTest/CLAUDE.md`. Untracked ones `git add -- <one path>` each, then `git commit -F <msg> -- <the 12>`; verified by `git show --stat HEAD` (the COMMIT, never the index). **The commit hash cannot appear in this file (the file rides the commit); it is on the four board rows, in 🔧 Build & Git, and in the ship report.** ⛔ NOT pushed; `main` becomes 2 ahead.
- `SC-§103`: the four flips (`TASK-1190` · `1191` · `1192` · `1193` → `done + <hash>`) are made by `Edit`, one `- status:` line each, in the same action as the commit — which leaves `TASKBOARD.md` dirty by exactly those four lines afterwards (a board line cannot carry the hash of the commit that carries it); reported as such.

## 9. Not proven by this ship (`SHIP-§6`) — always stated

- **No input-injection lane ⇒ no gameplay-feel claim.** Human acceptance is Jonathan's extract-and-click of THIS zip (his sitting was on the staged folder, not the archive; the archive's bytes are the same files, read back entry by entry, but the extract step itself is his).
- **The rig's capture proved nothing about pixels** (§4) — the pixel evidence for this package is his eye, on the byte-identical stage, not a retained capture of an arena.
- ⛔ Still unobserved, and named in the README's *What is NOT in this build*: the Graphics menu's **10-s keep-or-revert has never fired against a real `FTimerManager`** · the **fog's ~181 s breathing cycle** (diagnosis open) · the veil shimmer visible to the enemy (ruling open) · no console / no cheats in Shipping (by construction, not a defect).
- 🙋 **The list he is owed for his extract-and-click (`PKG-§6`, row cl. 7):** change the resolution, touch nothing for 10 s, watch it come back · does the FPS number scroll off while dragging a slider · WASD + click the knight · watch the fog for ≥ 3 minutes · (from `CHANGED_SINCE`) walk a unit past a ladder and watch whether it is pulled on; bury the camera in a tower and watch the bars.
- The `Saved\` note in the README source is false for this build (§5) — the rendered README says so in *What was verified*; the source line itself is the manager's to fix.

## 10. Findings for the manager (report, not act — `SC-§101`: leads are labelled)

1. **Capture instrument (rig finding, lead):** on this 3840×2400 @ 250 % machine the DPI-unaware `CopyFromScreen` in `ship.ps1`'s Pixel route appears to capture the top-left 40 % of the screen (both frames = sky, click delivered, game alive 90 s, title read). Same family as `SHIP-§9b`. A `ship.ps1` row behind `TASK-1201`: DPI-aware capture (physical bounds) + a control that a known on-screen mark is inside the frame. Until then, `C3-CAPTURE`'s "not uniformly blank" pre-filter passes a crop.
2. **Recipe seam (finding):** `A8-DESKTOP` precedes the resume decision, so a re-invoke that only needs to READ a verdict record (no click, no desktop) is refused when the desktop is locked — the FAIL record could not be retired by the script and was archived by hand (§4). The record read belongs before A8, or A8 should apply only to a run that will drive.
3. **README source prose (doc finding, one line):** `README-source.md` *Known notes* claims saves live in `Windows\GitClaudeUnrealTest\Saved\`; measured: `%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\` (§5). One-line fix + QA re-read on the next ship.
4. **`PKG-§9f`'s number, third sighting:** the session locked at ~23:45 PDT, ~5 min after his 23:40 keystroke — the 300 s display timeout again; the rig's own run made it because his "I closed the game" reset the idle counter at ~23:35.
5. **Stage stray:** `Windows\Engine\Saved\Config\Windows\Manifest.ini` (25 B, `[Manifest] Version=2`, 2026-08-29) is in no manifest and in both zips; harmless, named.
6. **The zip holds 4 `.exe` entries**, two of them `vc_redist` prereq installers — `PKG-§10`'s "exactly one runnable game exe" is true; a future read-back gate should count game exes, not `.exe` entries.

## 11. Artefacts (absolute)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Siegebound-Win64-Shipping-2026-09-09.zip` (1,334,631,291 B, sha256 `9CA197CB…70F19`) · `…\packagedZIPofGame\README.md` (rendered, 86,729 B) · `…\packagedZIPofGame\Windows\` (the verified stage at `31edc23`, 70 files)
- `…\packagedZIPofGame\.ship\20260910-063540\{compile.out.log, suite.out.log, automation.log, automation-report\, cook.out.log (88,190 B), cook.err.log (0 B), bootverify-menu-preclick-NOT-ADJUDICABLE.png, bootverify-arena.png (751,684 B, sha256 f9ecd337…), ship-adjudication.20260910-063540.consumed-FAIL.json}` · `…\.ship\ship-state.json` (the script's own, `boot=ADJUDICATE`, untouched) · `…\.ship\20260910-064525\` (empty; the A8 stop)
- Session scratch: `launch4.ps1, live4.log, live4b.log, render_readme.py, values.json, zip_stage.ps1, zip_stageonly.log, zip_final.log, README.2026-08-29.superseded.md`
