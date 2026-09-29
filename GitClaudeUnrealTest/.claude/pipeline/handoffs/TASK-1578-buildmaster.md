# TASK-1578: 5a for TASK-1574 + TASK-1576 (build-master handoff)

**Editor for `TASK-1579`: PID 15044.** It is the GUI editor on the plain `.uproject` (no `-game`) and owns MCP `:8000`. It has `UnrealEditor-GitClaudeUnrealTest.dll` sha256 `2596fb4b53da07d695884359042d26d17a7917e836ed4bcd177bd53a0551ac38` (10,091,008 B) loaded, which is the restored build. PIE is off. It is PIE-clean per `VER-§12` cl. 7g: `BS_ERROR` 0 of 19 resident. It opened on `L_Arena`. PID 12112 is gone.

**No commit.** `TASK-1580` is the host. The index is empty, and no git write was made.

marker `TASK-1578-HELP-FOLLOWUPS-5A` · row amendments up to `TASK-1578-AMENDED-4-2026-09-29` · law: `CLAUDE.md` rule 5a · `SC-§87` · `SC-§118` · `SHIP-§9` · `VER-§2` cl. 2 · `VER-§12` cl. 7g · source of truth `qa/TASK-1577.md` §5 / §6

## 1. Gates read at my instant (2026-09-29 00:38 PDT)

| gate | read |
|---|---|
| `qa/TASK-1575.md` / `qa/TASK-1586.md` / `qa/TASK-1577.md` | line 1 `PASS` / `PASS` / `PASS` |
| `TASK-1574` / `TASK-1576` / `TASK-1585` | `qa-passed` / `qa-passed` / `qa-passed` |
| `TASK-1569` | `done`, so not running |
| `TASK-1591` | `done`, text only (AMENDED-4 fence note: not a live verifier) |
| `TASK-1545` sitting | `verified` (MEASURED), no sitting in progress |
| index | empty; HEAD `9b82e8d`, `origin/main` `f1091b8` |

## 2. (1) Byte anchors: 4 of 4 equal to `qa/TASK-1577.md` §5, before the build, after every restore, and before the final build

| file | sha256 | bytes · lines | CR · BOM |
|---|---|---|---|
| `Source/…/SiegeControlsHelpWidget.cpp` | `6a9ba04491fea8b58b4538fe8e9496a674db0c23fa4478bdf48fa3679a29fa6b` | 317440 · 4977 | 0 · none |
| `Source/…/SiegeControlsHelpWidget.h` | `7ed5a066b6c5f608a40b23a95ce858db786bdad0e5565eb60cf2583f8a4d1527` | 81822 · 1412 | 0 · none |
| `Source/…/Tests/SiegeControlsHelpTest.cpp` | `588a4180553851f7c52da9e4907aac07d522b2c67db1a9e6121c1c6790de3fa4` | 172078 · 2981 | 0 · none |
| `Source/…/SiegePlayerController.cpp` | `d2dfbf5021b1594162d8e0a50d2b6cced2e7327c7833b912dd324d121995b1fd` | 388165 · 7775 | 0 · none |

## 3. (2) Editor census and graceful quit (`SC-§118`)

- **Before:** exactly one `UnrealEditor.exe`, PID **12112**, created 2026-09-28 16:58:12. Its command line was `"…\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\…\GitClaudeUnrealTest.uproject"` (no `-game`), and it owned `:8000`. The companions were `CrashReportClientEditor` 26428 (`-MONITOR=12112`) and `UnrealTraceServer` 2768.
- **Read over MCP in PID 12112 (read-only):** `is_pie_active` → `false`, `IN_PIE=False`, `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`, level `L_MainMenu`. `BS_ERROR` 0 of 37 (`BS_UNKNOWN` 17 / `BS_UP_TO_DATE` 20). DLL on disk `72dac5de…f475` (`TASK-1562`'s).
- **Quit:** re-identified inside the same call by PID, name and exact command line. `CloseMainWindow()` returned `True` at 00:40:34, and the process had exited by 00:40:43. No force, and no save prompt.
- **Log tail:** `LogExit: Exiting.` · `Log file closed, 09/29/26 00:40:42`. The newest `Saved/Crashes` entry is still 2026-09-04. `Unreal*` process count after the quit: 0.

## 4. (2) Compile of the gated tree: `Result: Succeeded`

`Build.bat` per `CLAUDE.md`, editor closed, never Live Coding. Ran 00:40:59 → 00:41:12.

```
[1/9] Compile [x64] SiegeControlsHelpTest.cpp
[2/9] Compile [x64] Module.GitClaudeUnrealTest.11.cpp
[3/9] Compile [x64] SiegePlayerController.cpp
[4/9] Compile [x64] SiegeControlsHelpWidget.cpp
[5/9] Compile [x64] Module.GitClaudeUnrealTest.1.cpp
[6/9] Compile [x64] Module.GitClaudeUnrealTest.2.cpp
[7/9] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[8/9] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[9/9] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Result: Succeeded
Total execution time: 13.20 seconds
```

The exit code (0) is recorded only. Diagnostics (`): warning|error <CODE>`): **0**. DLL `eda510d9…e92b` (superseded by the final build, §7).

## 5. (3) UHT shape: as `qa/TASK-1577.md` §5 predicted on the post side

I copied the whole `UHT/` directory (234 files) aside before the quit and diffed it after the build. UBT `Log.txt` reads `UHT processed GitClaudeUnrealTestEditor in 1.4512413 seconds (3 generated files written)`. Exactly those 3 files differ, with none added or removed.

- **`SiegeControlsHelpWidget.generated.h`: only the line-number macros changed.**
  - The **post** values are QA's exactly: class macros `_h_453/_h_636/_h_936`, and PROLOG `_h_450/_h_633/_h_933`. The struct's `_h_127` did not move. Size is unchanged at 11748 B.
  - **The "from" side differs from QA's text, and this is explained, not a mismatch.** The files on disk before this build were `TASK-1562`'s compile of HEAD's `.h`, which has 1400 lines (`_h_441/_h_624/_h_924`). QA's `_h_450/…` is `TASK-1576`'s start `.h` at 1409 lines. It includes `TASK-1574`'s +9 lines and was never compiled. The observed shift is therefore +12 (= +9 + +3).
- **`SiegeControlsHelpWidget.gen.cpp`:** the `OneLine` and `Detail` properties' `Comment` and `ToolTip` metadata (4 lines; `TASK-1574` edited both docs and `TASK-1576` edited `Detail`), the struct CRC and the file-registration CRC.
- **`GitClaudeUnrealTest.init.gen.cpp`:** the package CRC only (`0x9094798B` → `0x76528A44`).
- **Exec-symbol SETS, compared as sets in both directions:**

| set | pre | post | pre-only | post-only |
|---|---|---|---|---|
| `DECLARE_FUNCTION(exec…)` in `.generated.h` | 9 | 9 | ∅ | ∅ |
| `DEFINE_FUNCTION(…::exec…)` in `.gen.cpp` | 9 | 9 | ∅ | ∅ |

The set is `execCloseHelp, execGetSelectedActionId, execHandleBackButtonClicked, execHandleCloseButtonClicked, execHandleRowButtonClicked, execIsDetailViewActive, execIsHelpOpen, execOpenHelp, execRefreshRows`. The declare and define sets are equal to each other, and to `handoffs/TASK-1562-buildmaster.md` §5's set. The arms touched only the widget `.cpp`, so UHT did not re-run for them. The final state's declare set equals the post set above (`cmp` identical, files' mtime 00:41:01).

## 6. (4) The 6 mutation arms (`qa/TASK-1577.md` §6): each seen red on its own

**Procedure.** The tool was `scratchpad/t1578/arm.py`, run through `run_arm.ps1`.
- Before any arm: the widget `.cpp` was copied aside by byte copy (`aside/W.cpp`, sha `6a9ba044…fa6b`). Each anchor was counted over all **296** files under `Source/`, and every anchor counted **1**, in the widget `.cpp`. Each replacement counted 0 in `Source/`. Every byte delta and mutant size equalled §6. The guard's ASCII form `TEXT("Falling out of the world is a death, not a despawn ` also counted 1.
- **Per arm:**
  1. Assert the widget file and the aside copy are at the gated sha.
  2. Assert the anchor count in `Source/` is 1.
  3. Python `bytes.replace(anchor, repl, 1)` on the raw UTF-8 bytes. The em dash is the literal `E2 80 94`. No regex, no PowerShell text I/O.
  4. Assert the delta and the mutant size. Assert CR 0 and no BOM.
  5. `Build.bat`, then read `Result: Succeeded` from its log.
  6. Run the FULL bounded suite (`Tools/run_suite_bounded.ps1 -Porcelain`, default filter `Siegebound`).
  7. Read the log for every `Result={Fail}` and every `Expected '` line.
  8. Restore by `shutil.copyfile` from the aside copy, then assert the sha, the size and byte-equality.
- **Batching, said plainly:** one mutant at a time, each with its own build and its own full-suite run. A restore was **not** built between arms. The next arm's build compiled the file from its own mutant bytes. The restored bytes were built once, at the end (§7).

| arm | line | mutant sha256 · size | build | suite | red test (the ONLY `Result={Fail}`) | errors | first error line | restore sha = gated |
|---|---|---|---|---|---|---|---|---|
| **1576-T** | 2101 | `37a77f1e…534c` · 317434 | `Result: Succeeded` 7.15 s, 0 diag | 568 / 567 / **1** · `…20260929-004211.log` | `Siegebound.ControlsHelp.TowerAndMapMarkRowsAreAuthoredAndRawLaned` | **1** | `Expected '⛔ Row 'Interface.MapMarks' detail TEMPLATE types NO number either - a number the page shows is read from its owner when the page is composed' to be false.` (`Tests/…(2090)`) | ✅ `6a9ba044…fa6b` |
| **1576-D1** | 367 | `02b96af8…4687` · 317464 | `Result: Succeeded` 7.03 s, 0 diag | 568 / 567 / **1** · `…20260929-004344.log` | `Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners` | **2** | `Expected 'Row 'Interface.MapMarks' shows the map-circle cap (the map-mark store's MaxMapMarks, class default) as its owner holds it: 'hold up to 9 circle'' to be true.` (`(2917)`) | ✅ |
| **1576-D2** | 391 | `1fea8169…a165` · 317440 | `Result: Succeeded` 6.83 s, 0 diag | 568 / 567 / **1** · `…20260929-004450.log` | `Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners` | **2** | `Expected 'Row 'Cards.StackUpgrade' shows the stack health factor (StackHealthMultiplier at one upgrade) as its owner holds it: 'maximum health by 1.5, compounding'' to be true.` (`(2917)`) | ✅ |
| **1585-A** | 1110 | `2b77421d…3470` · 317517 | `Result: Succeeded` 7.18 s, 0 diag | 568 / 567 / **1** · `…20260929-004554.log` | `Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule` | **2** | `Expected 'Row 'Cards.StackUpgrade' detail teaches the refuted rule 'refuses to be stacked' (TASK-1585 (A): no shipped building refuses the stack, STACK-§10)' to be false.` (`(2772)`) | ✅ |
| **1585-B** | 1582 | `a614d7d2…4ae0` · 317331 | `Result: Succeeded` 7.28 s, 0 diag | 568 / 567 / **1** · `…20260929-004700.log` | `Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule` | **3** | `Expected 'Row 'PickMode.Resize' detail teaches the refuted rule 'inert everywhere' (TASK-1585 (B): the wheel has three jobs, MARK-§4)' to be false.` (`(2772)`) | ✅ |
| **1574-guard** | 650 | `f1ee3714…1562` · 317473 | `Result: Succeeded` 6.62 s, 0 diag | 568 / 567 / **1** · `…20260929-004803.log` | `Siegebound.ControlsHelp.EveryRowHasAuthoredDetail` | **2** | `Expected 'Row 'Hero.Jump' detail carries no developer-only fragment '::' (T1/T2/T5: citations, markup and C++ fragments live in comments)' to be false.` (`(1158)`) | ✅ |

The suite column reads started / success / fail. Logs are under `Saved/Logs/run_suite_bounded_suite_*`.

**The rest of each arm's error lines** (a log's `Expected '` count equals its error count; there are no other `LogAutomationController: Error:` lines apart from the `Result={Fail}` line):
- 1576-D1 #2: `Expected 'Every rendering of row 'Interface.MapMarks' shows the map-circle cap (the map-mark store's MaxMapMarks, class default) (its own page and each related block that renders it)' to be 3, but it was 0.` (`(2953)`)
- 1576-D2 #2: `Expected 'Every rendering of row 'Cards.StackUpgrade' shows the stack health factor (StackHealthMultiplier at one upgrade) (its own page and each related block that renders it)' to be 2, but it was 0.` (`(2953)`)
- 1585-A #2: `… the refuted rule 'cannot be stacked' (TASK-1585 (A): …)' to be false.` (`(2772)`)
- 1585-B #2 and #3: `Expected 'PickMode.Resize names the wheel's other job 'placing a building' (TASK-1585 (B): MARK-§4's three consumers)' to be true.` and `… 'war map' …' to be true.` (`(2799)`)
- 1574-guard #2: `… fragment '()' (T1/T2/T5: citations, markup and C++ fragments live in comments)' to be false.` (`(1158)`)

**Exact-text check.** All 12 observed `Expected '…'` messages, with the ` [file(line)]` suffix stripped, were compared by exact string equality against the 12 backticked lines in `qa/TASK-1577.md` §6. The result is **12 / 12 match both ways**.

**Reading rule applied:**
- D1 and D2 each have 2 errors, so (a) and (a2) are both live.
- In the 1576-T run, `ShownNumbersAreReadFromTheirOwners` is `Result={Success}`, so the arm did not leak into test 20.
- The guard arm carries the new "(T1/T2/T5 …)" label, not the stale "(T1/T2: …)" tail, so `TASK-1574`'s label edit is in the binary.
- No other test went red in any arm.

**cap-i / cap-ii: not owed** (AMENDED-4, `qa/TASK-1577.md` Ruling 1). Not run.

## 7. (4) Restored build and bounded suite (`SC-§87`): **568 / 568 green**

- Re-hash before the final build: 4 of 4 equal to §2.
- **Build:** `[1/4] Compile [x64] SiegeControlsHelpWidget.cpp` · `[2/4] Link … .lib` · `[3/4] Link … .dll` · `[4/4] WriteMetadata` · **`Result: Succeeded`** · `Total execution time: 6.60 seconds` · 0 diagnostics. DLL 00:49:25, 10,091,008 B, sha256 `2596fb4b…ac38`. That is the DLL PID 15044 loaded.
- **Suite** (`Tools/run_suite_bounded.ps1 -Porcelain`, editor closed):

```
RUNNER_EXIT=0
RUNNER_ECHO_MATCHED=1 / RUNNER_ECHO_EXPECTED=1
RUNNER_STARTED=568  RUNNER_COMPLETED=568  RUNNER_SUCCESS=568  RUNNER_FAIL=0  RUNNER_SKIPPED=0
RUNNER_LOG=...\Saved\Logs\run_suite_bounded_suite_20260929-004926.log
```

- Distinct results: `Result={Success}` ×568 only. `Expected '` lines: 0. `LogAura` 0 · `Response code: 401` 0.
- All four arms' tests executed and are green: `EveryRowHasAuthoredDetail` :4264, `NoPageTeachesARefutedStackOrWheelRule` :4289, `ShownNumbersAreReadFromTheirOwners` :4331, `TowerAndMapMarkRowsAreAuthoredAndRawLaned` :4347. `Siegebound.ControlsHelp.*` = 20 tests.
- **Reconciled by name** against `TASK-1562`'s 566 run (`…20260928-165656.log`, 566 unique paths): **+2 added, −0 removed**.
  - `+ Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule`
  - `+ Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners`
  - `N` = 568, as QA predicted.
- **Save hygiene:** all 5 `Saved/SaveGames/*.sav` files have the same (sha256, size, mtime) before the quit and after the suite.
- `git status --porcelain` shows the same path set as at session start; nothing new was created in the worktree apart from this handoff and the Build/Test logs under ignored `Saved/`.

## 8. (5) Relaunch: PID 15044, PIE-clean (`VER-§12` cl. 7g)

- Launched at 00:50:42 with the same command line (plain `.uproject`, no `-game`, no extra flags). `:8000` was owned by 15044 at 00:50:55.
- Editor log markers: `InternalLoadLibrary: 'GitClaudeUnrealTest'` :1694, `Starting MCP server on port 8000` :2231, `Total Editor Startup Time, took 12.115` :2412.
- Census: `UnrealEditor.exe` 15044 only, plus `CrashReportClientEditor` 26652 (`-MONITOR=15044`) and `UnrealTraceServer` 488 (`--sponsor 15044`).
- **Binary is live:** `(Get-Process 15044).Modules` maps `…\Binaries\Win64\UnrealEditor-GitClaudeUnrealTest.dll`, 10,091,008 B, sha256 `2596FB4B…AC38`, which is §7's restored build.
- `get_headless_status` → `editor_connected`, which is not a liveness proof on its own. `is_pie_active` → `is_active: false`, which is the game-thread-bound liveness proof.

| reading (read-only `execute_unreal_python_readonly`, served PID 15044) | value |
|---|---|
| `is_loading_assets()` (asset registry) | `False` |
| level | `/Game/Maps/L_Arena.L_Arena` |
| PIE / dirty | `IN_PIE=False` · `DIRTY_CONTENT=[]` · `DIRTY_MAPS=[]` |
| **in-memory `BS_ERROR` walk** (`unreal.ObjectIterator(unreal.Blueprint)`) | **0 of 19 resident**. Liveness: total 19 > 0; histogram `BS_UNKNOWN` 17 · `BS_UP_TO_DATE` 2 |
| live-reader control `find_object(None, <level>)` | found (`True`) |
| log frame index | `[114]` on the last line (07:51:44 UTC). No new line by 00:53:10 local, because the idle editor was not logging. So this is **not** a frame-counter reading; see the limitations. |
| repeat liveness | `is_pie_active` → `is_active: false` again at about 00:53 local, after the log went quiet |

- New log `Error:` lines: only the pre-existing `GameFeatureData` pair (:2072, :2077).
- **No PIE has been run on 15044, and nothing was loaded.** Every call was read-only.

## 9. Board

- `TASKBOARD.md`: this row's `status:` line only → `built` (with `Edit`).
- The row's `names:` also lists "plus `built` on 1574 / 1576". The dispatch restricted me to this row's status line, so **`TASK-1574` / `TASK-1576` were NOT flipped**. That is for the orchestrator or manager to route.

## Not examined / limitations

- **No runtime path was exercised.** 5b (PIE, the rendered sentences, the wording legs) is `TASK-1579`'s.
- **One suite run per arm state**, not replicates. The restored bytes were built and suite-run once, at the end, not between arms.
- **The `BS_ERROR` walk carries a liveness control only.** No discriminating fixture exists since `ab57522` (`VER-§12` cl. 7g, control strength).
- **The frame counter:** I did not take the three-samples-minutes-apart discriminator. On an idle editor the log does not grow, so the bracketed index cannot move. The game-thread liveness proof is `is_pie_active` answering with a real object, twice.
- **Two same-source DLLs differ:** the gated-tree build (`eda510d9…`) and the restored build (`2596fb4b…`), from byte-identical sources. I did not examine which bytes differ (MSVC link non-determinism, as in `TASK-1562` §12). Source identity is carried by the file sha256.
- **The exec-symbol "pre" set** is the generated files on disk from `TASK-1562`'s compile of HEAD's `.h`. It is not a build of `TASK-1576`'s start `.h`, which was never compiled (§5).
- **The digits in the D1 / D2 lines** come from the `-nullrhi` suite process's culture (`en`), per QA's note.
