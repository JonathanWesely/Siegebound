# TASK-1281 — [MENU-INPUT-WAVE-HOST] build-master handoff

Lane A (`TASK-1274` alone — no word from 🧑 Jonathan on `1270`/`1271`, so neither rides; neither's diff is in the tree). Wave anchored on `TASK-1274` at `qa-passed` (`qa/TASK-1280-report.md` line 1: PASS, 0 blockers, warn 4, nit 2).

## §1 — built leg — 2026-09-14 — ⛔ STOPPED at cl. (2): compile PASSED, suite RED on the new test → `TASK-1274` = `build-failed` (QA loop 1)

### (1) Pre-flight, measured (git root ONE LEVEL UP, `SC-§102`)
`git status --porcelain` at HEAD `1a3200c`, 14:29 local — EXACTLY the expected set, no third author:
```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1274-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1280-report.md
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuAccept.uasset
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuDown.uasset
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuUp.uasset
?? GitClaudeUnrealTest/Content/Input/IMC_MainMenu.uasset
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp
```
- WARN-4 accepted: the three source files are `??` (new), not ` M`. WARN-2 re-measured: `git diff --stat -- CONVENTIONS.md` = EMPTY — the R16 hunks are in `8560b0d`; CONVENTIONS is clean. `L_MainMenu.umap` absent (acceptance (5) "not dirtied" branch). `Config/SiegeCloudDev.ini` absent. No `IMC_Hero`/`IMC_Default`/`IMC_MouseLook` entry. ⛔ Nothing staged at any point in this leg.

**Editor census (`SC-§118`), `Get-CimInstance Win32_Process` by COMMAND LINE:** exactly ONE `UnrealEditor.exe` — PID **6136**, CreationDate 2026-09-14 12:22:56, `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"` ⇒ the GUI editor (project path, no `-game`); owner of the `:8000` listener. **No `-game` instance existed** — nothing of Jonathan's in reach. Pre-close guards on 6136: unreal-mcp `EditorAppToolset.IsPIERunning` → `false`; Aura `is_pie_active` → `false`; in-process `get_dirty_content_packages()` → `[]`, `get_dirty_map_packages()` → `[]`, `DIRTY_COUNT 0`; editor world `L_Arena`. Nothing dirty ⇒ no save question arose; nothing saved.

### (2a) Graceful quit — the remote-exec lane (`TASK-1230` §7 recipe, script `quit_editor_remote.py` in this session's scratchpad)
Node census 1 (`node_id 3A50D43F4793CCA8F0A258BD8BA14FA0`, `project_name GitClaudeUnrealTest`, machine JONATHANWESELY). Identity proof in-process: `os.getpid()` → **6136**. `unreal.SystemLibrary.quit_editor()` → `success: True` at **14:30:15**. Exit proof: `UnrealEditor*` processes 0 and `:8000` listeners 0 at **14:30:26**. ⛔ No kill, no `Stop-Process`, no save, no Live Coding.

### (2b) Baseline suite — RE-MEASURED on the HEAD `1a3200c` binaries (DLL mtime 2026-09-09 13:08:08; last `Source/` commit `ea7b4d2` 09-09 05:02 ⇒ those binaries ARE HEAD's)
`Tools/run_suite_bounded.ps1 -Mode Suite -Filter Siegebound -OverallSeconds 540 -BootSeconds 300 -StallSeconds 180 -Porcelain` (the `TL-§6` executor; it exists, in the tree): PID 5168, boot at 10 s, wall **44 s**, dispatch echo 1/1, **started 555 / completed 555 / Success 555 / Fail 0 / Skipped 0 — green**, `RUNNER_EXIT=0`. Log `Saved/Logs/run_suite_bounded_suite_20260914-143038.log`. ⇒ baseline = **555**, the `TASK-1177` figure re-confirmed by measurement.

### (2c) Compile — the `CLAUDE.md` Build command, verdict FROM THE LOG (scratchpad `build-TASK-1281.log`, 34 lines)
Start 14:31:34, end 14:31:53. `Build.bat` exit 0 — recorded, NOT consulted.
```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: SiegeMenuInputSubsystem.cpp, SiegeMenuInputTest.cpp
Using Unreal Build Accelerator local executor to run 7 action(s)
[1/7] Compile [x64] SiegeMenuInputTest.cpp
[2/7] Compile [x64] SiegeMenuInputSubsystem.cpp
[3/7] Compile [x64] Module.GitClaudeUnrealTest.1.cpp
[4/7] Compile [x64] Module.GitClaudeUnrealTest.2.cpp
[5/7] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[6/7] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[7/7] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Total time in Unreal Build Accelerator local executor: 13.48 seconds
Result: Succeeded
Total execution time: 18.98 seconds
```
Warnings in the build log: **0** (`grep -ci warning` = 0; none on either new file). One project module rebuilt (`GitClaudeUnrealTest`; the target's other modules were up to date). `UnrealEditor-GitClaudeUnrealTest.dll` mtime **2026-09-14 14:31:53**, 9,724,416 B (was 9,661,952 B). Not Smart App Control (19 s, no `0x800711C7`).

### (2d) Suite on the NEW binaries — run TWICE (`SC-§114`: one unreplicated red is not a conclusion)
| run | log | PID | wall | started / completed | Success / Fail | reds |
|---|---|---|---|---|---|---|
| 1 | `run_suite_bounded_suite_20260914-143207.log` | 20584 | 48 s | 556 / 556 | **554 / 2** | `Siegebound.ControlsHelp.TheThreeWheelMeaningsAreThreeDistinctRows` · `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` |
| 2 | `run_suite_bounded_suite_20260914-143418.log` | 9776 | 48 s | 556 / 556 | **554 / 2** | `Siegebound.Deck.MigrationFreshSave` · `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` |

Discovered = 556 = baseline + 1 (the new test IS registered and ran). `RUNNER_EXIT=5` both runs. ⛔ NOT "+1 green".

**Red B — DETERMINISTIC, the diff's own test, the reason for `build-failed`.** `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` fails in BOTH runs on exactly ONE captured `Error:` in its event block, and it is the ENGINE's, not a test assertion:
```
[2026.09.14-21.32.48:635][653]LogAutomationController: Error: LogPlayerController: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget [Widget.cpp(976)]! [log]
```
(run 2: identical text at `[2026.09.14-21.34.58:949][668]`). Source: `BP_MenuGameMode` BeginPlay → `SetInputMode_UIOnlyEx(WidgetToFocus = WBP_MainMenu)`, and `WBP_MainMenu`'s `SObjectWidget` is not focusable — PRE-EXISTING on every `L_MainMenu` boot (`Saved/Logs/GitClaudeUnrealTest-backup-2026.09.14-03.55.44.log:1918` and four other older logs, all before this diff). The automation framework reds a test on any captured `Error` it did not declare; `Tests/SiegeMenuInputTest.cpp` has no `AddExpectedError` / `SetSuppressLogErrors`. Everything the test itself asserts PASSED: `cold state: focused index 0 ("Play (vs Bot)") of 7 buttons`; `LogSiegeMenuInput: IMC_MainMenu applied at priority 0 on 'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started)`; `IA_MenuAccept -> OnClicked.Broadcast() on 'Button_2' ("Deck Builder")`.
- ⭐ **WARN-3 did NOT fire the way QA predicted — and that is a finding worth keeping:** `LogEditor: Starting PIE for the automation tests for world, L_MainMenu` · `PIE: Play in editor total start time 0.841 seconds` under `-nullrhi -unattended`. **The suite lane CAN host PIE.** The failure is code-scope (the test does not expect an error the map has always printed), so per routing rule 6 it is a QA loop, not a lane note. The fix is the programmer's; the smallest shape is named in the QA-report append as a CLAIM (`SC-§101`), not a recipe.

**Red A — a RACE with the Aura plugin, NOT the diff, recorded so nobody bounces it as one.** `LogAura: Error: Response code: 401` / `{"message":"User not authenticated","status":"error"}` (the plugin's indexing call, unauthenticated in the `-nullrhi` lane — `Indexing failed: Authentication required`) fires ~19–24 s after boot and reds WHICHEVER test's event window is open at that instant. Baseline: landed at 21:31:02.990 between `Deck.UncapExactFiftyPinned` and `Deck.UncapFiftyOfOneCardLegal` — no red (that is why the baseline was 555/0). Run 1: inside `ControlsHelp.TheThreeWheelMeaningsAreThreeDistinctRows` (red; green again in run 2). Run 2: inside `Deck.MigrationFreshSave` (red; green in run 1 and baseline). Three runs, three landing spots, same 401, same 19–24 s. ⇒ **the suite's "all green" is a coin flip on every run until the plugin's 401 is kept out of test windows** — follow-up for the manager (not on any row; outside `TASK-1274`'s names). Not counted against `TASK-1274`.

Errors appended verbatim to `qa/TASK-1280-report.md` under `## Build errors (TASK-1281)`.

### (3) Relaunch record (`SC-§118`, GUI editor only) — done anyway, so the machine is back in its standing editor-up state
`Start-Process` of the same exe with the SAME argument shape (relative `.uproject`, working dir `Engine/Binaries/Win64`, ⛔ no `-game`) → PID **10128**, CreationDate **2026-09-14 14:36:11**, command line `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"`. `:8000` listening, owner 10128, at 14:36:21 (10 s). DLL mtime 14:31:53 < editor start 14:36:11 ✓; fresh log line 1695 `LogModuleManager: InternalLoadLibrary: 'GitClaudeUnrealTest' ('…/Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll')` at 14:36:18. ⚠️ The editor is on the NEW binaries; the programmer's fix will need another compile ⇒ another graceful close (same lane, ~15 s).

Read-backs on PID 10128: Aura `get_headless_status` → **`editor_connected`**; `get_unreal_context` → `current_level_path: /Game/Maps/L_Arena.L_Arena`, open windows `GitClaudeUnrealTest - Unreal Editor` + `Message Log` (the standing `LogGameFeatures: Error: Asset manager settings do not include a rule for assets of type GameFeatureData` — pre-existing, not ours); `AssetTools.is_dirty(/Game/Maps/L_MainMenu)` → **`false`**; `AssetTools.exists` → `/Game/Input/IMC_MainMenu` **true**, `/Game/Input/Actions/IA_MenuUp` **true**, `IA_MenuDown` **true**, `IA_MenuAccept` **true**; `get_input_mapping_context_keys(/Game/Input/IMC_MainMenu.IMC_MainMenu)` → **`mapping_count: 6`**: `IA_MenuUp`←`Up`, `IA_MenuUp`←`Gamepad_DPad_Up`, `IA_MenuDown`←`Down`, `IA_MenuDown`←`Gamepad_DPad_Down`, `IA_MenuAccept`←`Enter`, `IA_MenuAccept`←`Gamepad_FaceButton_Bottom` (all Boolean). Fresh-log `Error:` census: 4 — the GameFeatureData pair + two `LogModelContextProtocol` session-reinit lines from my own first call after the relaunch; none from `LogSiegeMenuInput`. Nothing dirty, nothing saved, no PIE.

### Status flips (this leg)
- `TASK-1274`: `qa-passed` → **`build-failed`** (suite leg; QA loop 1 of 3; previous status text retained on the line).
- `TASK-1281`: `backlog` → **`in-progress — built leg ⛔ STOPPED at cl. (2)`**. ⛔ No `built`, no verify leg, nothing staged, no commit, no push. The wave re-enters at cl. (2) after the programmer's fix and a `TASK-1280` re-read.

Slack: 🔧 Build & Git thread, `p1789421946253369` (🚧 TASK-1281).

### Follow-ups for the orchestrator → manager
1. **The Aura 401 race in the suite lane** (Red A above): a pre-existing, environment-side cause of random single-test reds on every `-nullrhi` suite run; needs a row (either quiet the plugin's indexing in the suite lane, or a suite-wide expected-error for `LogAura` 401 — both outside any current row's names).
2. **WARN-3 is measurably wrong in the good direction:** PIE hosts under `-nullrhi`; QA's "cannot host PIE" prediction should not be carried forward as a lane limitation.
3. **`BP_MenuGameMode` focuses a non-focusable widget** at every boot (an engine `Error:` on every `L_MainMenu` start, in every log since at least 09-13). Cosmetic today; it is the thing that reds the new test. Whether the durable fix is in the Blueprint (asset edit) or the test's expectation is the manager's/programmer's call.


## §2 — built leg, loop 1 — 2026-09-14 — ✅ compile PASSED, suite 556/556 ×2, menu test GREEN → `TASK-1274` = `built — loop 1`; ⛔ STOPPED before the verifier leg (🧑 VER-§3 go owed)

Re-entry at cl. (2) on the programmer's loop-1 fix (`qa/TASK-1280-report.md` line 1 = loop-1 PASS; `## Loop 1 re-gate` read whole). The diff is ONE file: `Tests/SiegeMenuInputTest.cpp` 203 → 217 lines, `AddExpectedError(TEXT("InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget"), EAutomationExpectedErrorFlags::Contains, /*Occurrences*/ 1)` at `:129-130`. No asset touched.

### (1) Pre-flight, measured (`SC-§102`, git root one level up) — HEAD `1a3200c`, 14:52 local
Porcelain = §1's set + my own `?? handoffs/TASK-1281-buildmaster.md` — no third author, nothing staged (`git diff --cached --stat` empty), `git diff --stat -- CONVENTIONS.md` EMPTY (WARN-2 stays closed), `L_MainMenu.umap` / `Config/SiegeCloudDev.ini` / `IMC_Hero` / `IMC_Default` / `IMC_MouseLook` absent. Test file mtime 14:43:27 > DLL mtime 14:31:53 ⇒ the recompile was genuinely owed (the §1 binaries carried the un-pinned test).

**Editor census (`SC-§118`), by COMMAND LINE:** exactly ONE `UnrealEditor.exe` — PID **10128**, CreationDate 2026-09-14 14:36:11, the §3 relaunch (GUI shape: relative `.uproject`, ⛔ no `-game`), owner of the `:8000` listener. **No `-game` instance existed.** Guards on 10128: unreal-mcp `EditorToolset.EditorAppToolset.IsPIERunning` → `false`; Aura `is_pie_active` → `false`; in-process `DIRTY_CONTENT []` / `DIRTY_MAPS []` / `DIRTY_COUNT 0`; editor world `L_Arena`. Nothing dirty ⇒ no save question; nothing saved.

### (2a) Graceful quit — remote-exec lane (`quit_editor_remote.py`, now hardened: it ABORTS instead of quitting if `DIRTY_COUNT ≠ 0` or the in-process PID ≠ the census PID)
Node census 1 (`node_id 061DD40D4E3753D018F110B5594817A3`, `project_name GitClaudeUnrealTest`). Identity in-process: `os.getpid()` → **10128**. `unreal.SystemLibrary.quit_editor()` → `success: True` at **14:52:57**. Exit proof at **14:53:10**: `UnrealEditor*` processes 0, `:8000` listeners 0. ⛔ No kill, no save, no Live Coding.

### (2b) Compile — `CLAUDE.md` Build command, verdict FROM THE LOG (scratchpad `build-TASK-1281-loop1.log`, 29 lines)
Start 14:53:21, end 14:53:28. `Build.bat` exit 0 — recorded, not consulted.
```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: SiegeMenuInputSubsystem.cpp, SiegeMenuInputTest.cpp
Using Unreal Build Accelerator local executor to run 4 action(s)
[1/4] Compile [x64] SiegeMenuInputTest.cpp
[2/4] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[3/4] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[4/4] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Total time in Unreal Build Accelerator local executor: 5.44 seconds
Result: Succeeded
Total execution time: 6.39 seconds
```
Only the test file recompiled (the subsystem's object was current). Warnings: **0** (`grep -ci warning` = 0). No `0x800711C7` (not Smart App Control). `UnrealEditor-GitClaudeUnrealTest.dll` mtime **2026-09-14 14:53:27**, 9,724,928 B (was 9,724,416 B).

### (2c) Suite on the new binaries — run TWICE (`SC-§114`), `Tools/run_suite_bounded.ps1 -Mode Suite -Filter Siegebound -OverallSeconds 540 -BootSeconds 300 -StallSeconds 180 -Porcelain`
| run | log (`Saved/Logs/`) | PID | wall | started / completed | Success / Fail | `Result={Fail}` lines | `RUNNER_EXIT` |
|---|---|---|---|---|---|---|---|
| 1 | `run_suite_bounded_suite_20260914-145342.log` | 30300 | 46 s | 556 / 556 | **556 / 0** | 0 | 0 |
| 2 | `run_suite_bounded_suite_20260914-145446.log` | 11380 | 46 s | 556 / 556 | **556 / 0** | 0 | 0 |

Discovered 556 = baseline 555 (§1 (2b)) + 1. Dispatch echo 1/1 both runs.

**The new test's OWN line — GREEN both runs:**
- run 1: `[2026.09.14-21.54.22:167][646]LogAutomationController: Display: Test Completed. Result={Success} Name={DownTwiceThenAcceptOpensDeckBuilder} Path={Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder}`
- run 2: `[2026.09.14-21.55.26:677][662] … Result={Success} … Path={Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder}`
Its assertions in both runs: `cold state: focused index 0 ("Play (vs Bot)") of 7 buttons` · `LogSiegeMenuInput: … IMC_MainMenu applied at priority 0 on 'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started)` · `IA_MenuAccept -> OnClicked.Broadcast() on 'Button_2' ("Deck Builder")`. PIE hosted under `-nullrhi` again (`Play in editor total start time 0.762 s` / `0.751 s`), ended by `Cmd: Exit` (PIE only).

**Expected-error occurrence count = 1 per run, measured two ways.** (a) The pattern `Attempting to focus Non-Focusable widget` occurs EXACTLY ONCE in each whole log — run 1 `:6447`, run 2 `:6448` — and the framework re-emitted it as `LogPlayerController: Verbose: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget [Widget.cpp(976)]!` (the `Verbose` downgrade is the `GLog`-path mark of a matched expectation, exactly the trace in the loop-1 re-gate check 2); no `Error:`-level copy exists in either test block. (b) The pin is `Occurrences 1`, which reds the test with `Expected ('Warning') level log message or higher matching '…' to occur 1 times … but it was found N time(s)` on ANY N ≠ 1 — that sentence is ABSENT from both logs and the test is green ⇒ N = 1. ⚠️ Note for the next reader: the `LogAutomationController: Suppressed expected … 1 times.` report line that the `GWarn`-side filter prints for other tests' expectations (e.g. `'Snapshot roster TRUNCATED'`, `:3352`/`:3590`) does NOT print for this one — the `GLog` output-device path does not emit it. Its absence is not a missing count; (a)+(b) are the count. ⛔ The pin was not touched.

**Red A (the Aura `401` race) — fired both runs, HIT NOTHING this time:**
- run 1: `[21.54.04:522][803] LogAura: Error: Response code: 401` — same millisecond AND frame as `Test Completed. Result={Success} … Deck.UncapNegativeCountStillIllegal` (`:522 [803]`), before `Test Started … Deck.UncapUnknownCardStillIllegal` (`:524 [804]`) ⇒ between tests, no victim (the §1 baseline shape).
- run 2: `[21.55.08:730][804] LogAura: Error: Response code: 401` — same ms/frame as the CONTROLLER's `Test Started … Deck.UncapExactFiftyPinned` (`:730 [804]`), yet that test completed `Result={Success}` at `:779` ⇒ the line landed before the worker's capture window opened. No victim.
So this leg's 556/0 ×2 is cleaner than acceptance (4) needs (baseline + 1 with the only permitted reds being the race) — but the race is STILL LIVE (fired ~22 s after boot in both runs) and the §1 follow-up (1) stands unchanged: it will red a random test on some future run.

### (3) Relaunch record (`SC-§118`, GUI editor only)
Pre-launch census at 14:56:15: 0 editor processes (both suite `UnrealEditor-Cmd.exe` instances had exited). `Start-Process`, SAME shape (relative `.uproject`, working dir `Engine/Binaries/Win64`, ⛔ no `-game`) → PID **11576**, CreationDate **2026-09-14 14:56:15**, command line `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"`. `:8000` listening, owner 11576, at 14:56:25 (10 s). DLL mtime 14:53:27 < editor start 14:56:15 ✓. Fresh `Saved/Logs/GitClaudeUnrealTest.log` line 1 `Log file open, 09/14/26 14:56:16`; `:1694 LogModuleManager: InternalLoadLibrary: 'GitClaudeUnrealTest' ('…/Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll')` at 14:56:22 — the editor is on the loop-1 binaries.

Read-backs on 11576: Aura `get_headless_status` → **`editor_connected`**; `get_unreal_context` → `current_level_path: /Game/Maps/L_Arena.L_Arena`, windows `GitClaudeUnrealTest - Unreal Editor` + `Message Log` (the standing GameFeatureData pair, pre-existing); remote-exec in-process `PID 11576`, `DIRTY_CONTENT []`, `DIRTY_MAPS []`, **`DIRTY_COUNT 0`** (⇒ `L_MainMenu` not dirty), world `L_Arena`; `get_input_mapping_context_keys(/Game/Input/IMC_MainMenu.IMC_MainMenu)` → **`mapping_count: 6`** — `IA_MenuUp`←`Up`/`Gamepad_DPad_Up`, `IA_MenuDown`←`Down`/`Gamepad_DPad_Down`, `IA_MenuAccept`←`Enter`/`Gamepad_FaceButton_Bottom` (all Boolean). Fresh-log `Error:` census 5 = the GameFeatureData pair + THREE `LogPython: Error` lines that are MINE (my read-back's `Package.is_dirty` attribute miss at 14:57:16 — the Python `Package` has no `is_dirty`; the `DIRTY_MAPS` census is the authoritative answer and was taken in the same call before the miss). None from `LogSiegeMenuInput`. Nothing dirty, nothing saved, no PIE, no Live Coding.

### Status flips (this leg)
- `TASK-1274`: `qa-passed — loop 1` → **`built — loop 1`** (compile `Result: Succeeded`, suite 556/556 ×2, menu test GREEN, GUI editor relaunched PID 11576; awaiting 🧑 VER-§3 go).
- `TASK-1281`: → **`in-progress — built leg ✅ (loop 1)`**. ⛔ Nothing staged, no commit, no push. ⛔ STOPPED before the verifier leg — the orchestrator announces PIE and waits for Jonathan's go (Aura verification drives PIE on the GUI editor PID 11576).

Slack: 🔧 Build & Git thread (`1783116286.945249`), `p1789423121483739` (🔧 TASK-1281 built, loop 1).

### Follow-ups (unchanged from §1 unless noted)
1. The Aura `401` race — still live (fired ~22 s after boot in both runs; missed every window by luck this time). Needs its row.
2. WARN-3 stays refuted: PIE hosted under `-nullrhi` in all four suite runs to date.
3. `BP_MenuGameMode` focuses a non-focusable widget — now WARN-5 on the QA report; its row must delete `Tests/SiegeMenuInputTest.cpp:117-130` in the same diff (the pin at 1 reds the test when the error stops).
4. NEW, small: the `Suppressed expected … N times` line is not printed for `GLog`-captured expectations — a future host reading a suite log for an occurrence count should count the `Verbose`-downgraded line, as above, not wait for a report line that never comes.

## §3 — 5c commit leg — 2026-09-14 — ✅ `VERIFIED` (bare, the first BINDING verdict) ⇒ cl. (6) executed; commit `<hash — pending, back-referenced in commit 2 (SC-§103)>`

Entry condition read: `qa/TASK-1274-verify.md` line 1 `Verdict: VERIFIED`, 3/3 `pass` (deck builder `DeckBar` 10 children · sandbox Accept → `L_Arena` · Settings Accept → `SettingsMenuWidget_0`), 1 attempt, GUI editor PID 11576, `DIRTY_COUNT=0`, nothing saved. Lane A (no `1270`/`1271` diff in the tree, no word from 🧑). ⛔ No engine work in this leg — `tasklist` census: exactly one `UnrealEditor.exe`, PID **11576** (the §2 relaunch), untouched; no `-game` instance.

### (a) Evidence promotion by COPY (`VER-§4` cl. 1) — `cp -n`, sources left in place
| source (`Saved/AuraVerify/`) | target (`.claude/pipeline/playtest-evidence/2026-09-14/`) | bytes | sha256 (source = target) |
|---|---|---|---|
| `pie_composited_c1_t18.72s_f54287.png` | `VER-TASK-1274-t00m18s-deck-builder-deckbar-10-children.png` | 392,808 | `8cacfa0131952a9dd794cb63b8e10009d6c12f427ee6b8727b3287cc9b62c58f` |
| `pie_composited_c2_t3.04s_f65307.png` | `VER-TASK-1274-t00m03s-sandbox-accept-l-arena-loaded.png` | 1,148,483 | `85abe8324b188416d78cac0fd597c3f21ed607ce0039ce08ca4159523a039c87` |
| `pie_composited_c3_t21.05s_f68344.png` | `VER-TASK-1274-t00m21s-settings-menu-widget-open.png` | 215,660 | `f7d3acf704600837d8b2de91cd475246ab764bd95bde814b0b2db618ae341c5c` |
`sha256sum` run on both sides after the copy: three equal pairs. The `Saved/AuraVerify/` sources and the three `rec_*` films stay under `Saved/` (never staged).

### (b) Pre-flight porcelain, measured at HEAD `1a3200c` (git root ONE LEVEL UP, `SC-§102`) — EXACT, 12 entries, no third author
```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1274-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1281-buildmaster.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1274-verify.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1280-report.md
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuAccept.uasset
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuDown.uasset
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuUp.uasset
?? GitClaudeUnrealTest/Content/Input/IMC_MainMenu.uasset
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Tests/… (see below)
```
(the twelfth entry is `?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp`.) Index EMPTY at pre-flight (`git diff --cached --stat` = nothing; the UE Git plugin auto-stage did not fire). Absent, as required: `Config/SiegeCloudDev.ini` · anything under `Saved/` · `IMC_Hero` / `IMC_Default` / `IMC_MouseLook` · `Content/Maps/L_MainMenu.umap` · `settings.local.json` · any `1270`/`1271` file. `.gitattributes:1` `*.uasset filter=lfs` and `:4` `*.png filter=lfs` — both classes go through LFS. The board's working diff vs HEAD = exactly the 3 status lines of `TASK-1274` / `TASK-1280` / `TASK-1281` (3 insertions / 3 deletions) — nothing else on the board rides.

**Value-grep (secret VALUE shapes: `hf_…` · JWT `eyJ…` · `sb_secret_…` / `sb_publishable_…` · `DbPassword=<value>` · `sk-…` · `xox[bp]-…`):** 0 hits in each of the 7 new text files and 0 in the board's added lines; positive control (`echo 'DbPassword=abc123'`) fires 1 ⇒ the pattern is live. No census script exists under `Tools/` (grep `census|secret` = none) — the inline grep is the census.

### (c) LFS read-back BEFORE the commit — `git show :<path>` line 1 + `oid sha256:` vs `sha256sum` of the working file (⛔ never size, `SC-§68`)
| path | pointer line 1 | oid = sha256 |
|---|---|---|
| `Content/Input/IMC_MainMenu.uasset` | `version https://git-lfs.github.com/spec/v1` | **MATCH** `c0da00d3a5b5de4731369cc8a506f8f3b39c3439f52faf22ee68dc5d218cd1d9` |
| `Content/Input/Actions/IA_MenuUp.uasset` | same | **MATCH** `d0efdfa8861034731c4f729c5a1e9dbda0823ade77507b7b73e2e4db58d2ebaa` |
| `Content/Input/Actions/IA_MenuDown.uasset` | same | **MATCH** `8760b138199fa321c462642774d2e068732d84b87098e7e8d5794eb9421b9391` |
| `Content/Input/Actions/IA_MenuAccept.uasset` | same | **MATCH** `9c8918974782cd188d2ca086956ec6b66e14731a866a32c159db8cc2699396ee` |
| `playtest-evidence/2026-09-14/VER-TASK-1274-t00m18s-deck-builder-deckbar-10-children.png` | same | **MATCH** `8cacfa01…c58f` |
| `playtest-evidence/2026-09-14/VER-TASK-1274-t00m03s-sandbox-accept-l-arena-loaded.png` | same | **MATCH** `85abe832…9c87` |
| `playtest-evidence/2026-09-14/VER-TASK-1274-t00m21s-settings-menu-widget-open.png` | same | **MATCH** `f7d3acf7…41c5c` |
`git lfs ls-files -a` lists all seven with the matching oid prefixes. 7/7 MATCH ⇒ no re-add needed.

### (d) Board flips (step 1 of `SC-§103`, BEFORE staging) — three rows, no other
- `TASK-1274`: `built — loop 1` → `done — committed <pending> · verified 2026-09-14 (first BINDING verdict) · qa-passed loop 1 · built loop 1 · 3 stills promoted` (history tail kept under `· was:`).
- `TASK-1280`: `backlog` → `done — PASS (loop 0 + loop 1 re-gate) · committed <pending> (host TASK-1281)` (QA had not flipped its own row).
- `TASK-1281`: `in-progress — built leg ✅ (loop 1)` → `done — committed <pending> — 5c leg …`.
Commit 2 (the hash back-reference) replaces `<pending>` on the three lines and in this section's heading.

### (e) The commit — 15 paths by pathspec from the git root (14 above + this handoff), message first line PRESCRIBED by the row (lane A, `<VERDICT>` = `VERIFIED`), body = the two house trailer lines. `git show --stat HEAD` quoted in the report to the orchestrator; ⛔ NOT pushed (`main` was 17 ahead of `origin/main` before this commit).

### Follow-ups (new in this leg)
5. Acceptance (2) of `TASK-1274` ("mouse clicks unchanged, one click per button") was NOT measured by the verifier (`ui_perform` click shapes are the traced dead ends, `VER-§5` cl. 5) — a real mouse click and the Down/Down/Enter keyboard check are owed to 🧑 Jonathan's hand (handoff `TASK-1274-programmer.md` §7). Not a fail; recorded on the `TASK-1274` row.
6. The verifier's H3: `has_mapping_context(IMC_MainMenu)` read `0` through the Python wrapper while `query_keys_mapped_to_action` on the same live subsystem returned all six keys — a wrapper/return-type quirk worth one line in the next recipe, not a row.
