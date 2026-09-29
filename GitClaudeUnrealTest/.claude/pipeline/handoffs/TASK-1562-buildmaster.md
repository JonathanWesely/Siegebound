# TASK-1562: help-prose follow-through, 5a and host (build-master handoff)

**Editor for `TASK-1545`: PID 12112.** It runs the GUI editor on the plain `.uproject` (no `-game`) and owns MCP `:8000`. It has `UnrealEditor-GitClaudeUnrealTest.dll` sha256 `72dac5debed7d83ef79a918eb4216929f64bb38d402483b4a47579eccf17f475` loaded, the restored build. It is PIE-clean per `VER-§12` cl. 7g: `BS_ERROR` is 0 of 19 resident. It opened on `L_Arena`. PID 10720 is gone.

**Commit:** `9b82e8d99e8b09447f2bcdb13a31db5c0f2e88e4` (`9b82e8d`), parent `f1091b8`, 2026-09-28. A CODE commit: no `.claude/agents/*`, no `CLAUDE.md`. ⛔ Not pushed. Against `origin/main` (`f1091b8`) the count was 0 ahead / 0 behind before and **1 ahead / 0 behind after**.

marker `TASK-1562-HELP-PROSE-FOLLOWTHROUGH-5A-HOST` · law: `CLAUDE.md` rule 5a / 5c · `SC-§87` · `SC-§118` · `SHIP-§9` · `VER-§2` cl. 2 · `VER-§12` cl. 7g · `TL-§5e` cl. 1/7 · `SC-§102` · `SC-§103` · row amendment `TASK-1562-AMENDED-2026-09-28`

## 1. Gates read at my instant

| gate | read |
|---|---|
| `TASK-1560` QA | `qa/TASK-1561.md` line 1 `PASS` (0 BLOCKER / 2 WARN / 1 NIT) |
| `TASK-1564` QA | `qa/TASK-1565.md` line 1 `PASS` (0 / 1 / 2), so the recipes ride |
| `TASK-1553` | `done`: two `CONVENTIONS.md` hunks, host this row |
| `TASK-1544` | no `CONVENTIONS.md` hunk has landed |
| HEAD | `f1091b8` == `origin/main`. H's lag is confirmed in it: `git log -1 -- …/handoffs/TASK-1550-buildmaster.md` → `f1091b8`. No lag owed. |
| index | empty before staging |
| live agents | none (dispatch) |

## 2. Byte anchors: 6 / 6 equal before the build, after the arm, at staging, and on the HEAD blobs

| file | sha256 | bytes | HEAD blob id |
|---|---|---|---|
| `Source/…/SiegeControlsHelpWidget.cpp` | `6f511582cee9929893daf85d1b51ff98389ac70e62b8f3ab84308db400ddc990` | 279507 | `9a5d7ff6…` |
| `Source/…/SiegeControlsHelpWidget.h` | `249277ed46601d16bb8f717ead2d76e7486efb76f60983662e5f6968917d2720` | 81005 | `488df9df…` |
| `Source/…/Tests/SiegeControlsHelpTest.cpp` | `33fd21e74d9378a4b8b28a39d79fd2b72b7e37bf1b8d031988dc3138f2a58c5f` | 150550 | `ac3d292c…` |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | `0a63186421c3daf48af0d64b32e6ad17db6b45a239ca4864d041ee439c3ee2ea` | — | `f2c11a5e…` = `qa/TASK-1565.md` §4 computed |
| `Tools/Verify/recipes/README.md` | `4f0eaaf634553b05e36536f9d1de858ef292abd3cdc9823a322162c4052d465b` | — | `1b143719…` = §4 computed |
| `handoffs/TASK-1564-programmer.md` | `c8b7c1cfb882843f5b076eca2d39fe2f5eb586806e37e8c6702aca90601845a8` | — | `0374aadc…` = §4 computed |

The first column is the sha256 of `git cat-file -p HEAD:<path>`, and it equals the worktree file for all six (all LF).

## 3. Editor census and graceful quit (`SC-§118`)

- **Before:** exactly one `UnrealEditor.exe`, PID **10720**, created 2026-09-28 15:57:00. Command line: `"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "C:\GitProjects\…\GitClaudeUnrealTest.uproject"` (no `-game`). It owned `:8000`. The other processes were `CrashReportClientEditor` 18796 (`-MONITOR=10720`) and `UnrealTraceServer` 20456.
- **Read over MCP in PID 10720:** `IN_PIE=False` (`is_pie_active` → `false`), `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`, level `L_Arena`, `BS_ERROR` 0 of 19. The loaded DLL was `c0b7f72f…` (`TASK-1538`'s).
- **Quit:** re-identified inside the same call by name, exact command line and no `-game`. `CloseMainWindow()` returned `True` at 16:53:49 and the process had exited by 16:53:57. No force, and no save prompt.
- **Log tail:** `LogExit: Exiting.` · `Log file closed, 09/28/26 16:53:55`. The newest `Saved/Crashes` folder is still 2026-09-04. `Unreal*` process count before the compile: 0.

## 4. Compile: `Result: Succeeded`

`Build.bat` per `CLAUDE.md`, with the editor closed. ⛔ Never Live Coding. Ran 16:54:15 → 16:54:30.

```
[1/9] Compile [x64] SiegeControlsHelpTest.cpp
[2/9] Compile [x64] Module.GitClaudeUnrealTest.11.cpp
[3/9] Compile [x64] SiegeControlsHelpWidget.cpp
[4/9] Compile [x64] SiegePlayerController.cpp
[5/9] Compile [x64] Module.GitClaudeUnrealTest.1.cpp
[6/9] Compile [x64] Module.GitClaudeUnrealTest.2.cpp
[7/9] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[8/9] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[9/9] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Result: Succeeded
Total execution time: 14.82 seconds
```

The exit code (0) is recorded only. Diagnostics (`): warning|error <CODE>`): **0**. The `.h` edit rebuilt its includers, which QA §4 expected.

## 5. UHT: exactly the shape QA predicted

The whole `UHT/` directory (234 files) was copied aside before the quit and diffed after. UBT `Log.txt` reads: `UHT processed GitClaudeUnrealTestEditor in 1.8016749 seconds (3 generated files written)`. Those 3 files are the only ones that differ. None were added or removed.

- `SiegeControlsHelpWidget.generated.h`: **line-number macros only**, each shifted by +3. `_h_438/_h_621/_h_921` became `_h_441/_h_624/_h_924`, and PROLOG `_h_435/_h_618/_h_918` became `_h_438/_h_621/_h_921`. The struct's `_h_127` did not move. Size is unchanged at 11748 B.
- `SiegeControlsHelpWidget.gen.cpp`: the `Detail` property's `Comment` and `ToolTip` metadata (2 lines), plus the struct's CRC and the file-registration CRC.
- `GitClaudeUnrealTest.init.gen.cpp`: the package CRC only (`0x5DDB9D99` → `0x9094798B`).
- **Exec-symbol SETS, compared as sets in both directions:**

| set | pre | post | pre-only | post-only |
|---|---|---|---|---|
| `DECLARE_FUNCTION(exec…)` in `.generated.h` | 9 | 9 | ∅ | ∅ |
| `DEFINE_FUNCTION(…::exec…)` in `.gen.cpp` | 9 | 9 | ∅ | ∅ |

The set is `execCloseHelp, execGetSelectedActionId, execHandleBackButtonClicked, execHandleCloseButtonClicked, execHandleRowButtonClicked, execIsDetailViewActive, execIsHelpOpen, execOpenHelp, execRefreshRows`, which equals QA's 9.

## 6. The mutation arm (`qa/TASK-1561.md` §3): RED, then restored and green

1. **Copied aside** with `cp -p`; the copy has sha256 `6f511582…c990`.
2. **Mutated** with a byte-exact Python `bytes.replace`, asserting `count == 1` (no regex). The literal `TEXT("default handling (which would delete your hero outright) and goes down the same ")` became `TEXT("AHeroCharacter::FellOutOfWorld() default handling (which would delete your hero outright) and goes down the same ")`. `diff` shows one line changed (:469). Mutant: sha256 `c733aca79a3f2981281dc4fb82710b2512f350314652a9c50b2ecd6408e22fa3`, 279540 B, CR 0.
3. **Built the mutant:** `[1/4] Compile [x64] SiegeControlsHelpWidget.cpp` … `Result: Succeeded`, 6.69 s, 0 diagnostics.
4. **Ran the FULL suite** (`Tools/run_suite_bounded.ps1 -Porcelain`): `RUNNER_EXIT=5 · STARTED=566 · COMPLETED=566 · SUCCESS=565 · FAIL=1`. Log `Saved/Logs/run_suite_bounded_suite_20260928-165537.log`. The only `Result={Fail}` is:
   ```
   4263 LogAutomationController: Error: Test Completed. Result={Fail} Name={EveryRowHasAuthoredDetail} Path={Siegebound.ControlsHelp.EveryRowHasAuthoredDetail}
   4265 LogAutomationController: Error: Expected 'Row 'Hero.Jump' detail carries no developer-only fragment '::' (T1/T2: citations and markup live in comments)' to be false. [...\Tests\SiegeControlsHelpTest.cpp(1138)]
   4266 LogAutomationController: Error: Expected 'Row 'Hero.Jump' detail carries no developer-only fragment '()' (T1/T2: citations and markup live in comments)' to be false. [...\Tests\SiegeControlsHelpTest.cpp(1138)]
   ```
   These are the log's only 2 `Expected '` lines. **Both list entries are live.** The "(T1/T2 …)" tail is QA W1's stale label; the match was made on the message start.
5. **Restored** with `cp` from the copy. sha256 is back to `6f511582cee9929893daf85d1b51ff98389ac70e62b8f3ab84308db400ddc990` (279507 B), `cmp` is identical, and the fragment count is 0.
6. **Rebuilt the restored file:** `[1/4] Compile [x64] SiegeControlsHelpWidget.cpp` … `Result: Succeeded`, 6.73 s, 0 diagnostics. DLL 16:56:51, 10,066,944 B, sha256 `72dac5de…f475`. This is the DLL PID 12112 loaded.

## 7. Bounded suite (`SC-§87`): **566 / 566 green**

`Tools/run_suite_bounded.ps1 -Porcelain`, editor closed, on the restored build, 16:56:56–16:57:47. `-nullrhi -unattended -NoLiveCoding -DisablePlugins=Aura`.

```
RUNNER_EXIT=0
RUNNER_ECHO_MATCHED=1 / RUNNER_ECHO_EXPECTED=1
RUNNER_STARTED=566  RUNNER_COMPLETED=566  RUNNER_SUCCESS=566  RUNNER_FAIL=0  RUNNER_SKIPPED=0
RUNNER_LOG=...\Saved\Logs\run_suite_bounded_suite_20260928-165656.log
```

- Distinct results: `Result={Success}` ×566 only. `Expected '` lines: 0.
- The arm's test executed and is green: `EveryRowHasAuthoredDetail` Started at :4260, `Result={Success}` at :4262.
- **Reconciled by name** against `TASK-1538`'s run (`…20260927-234513.log`, 566 paths): **+0 added, −0 removed**. `N` = 566, as QA predicted. `Siegebound.ControlsHelp.*` has 18 tests.
- `LogAura` 0 · `Response code: 401` 0.
- **Save hygiene:** all 5 `.sav` files have the same (sha256, size, mtime) before the quit and after the suite, zero change.

## 8. Relaunch: PID 12112, PIE-clean (`VER-§12` cl. 7g)

- Launched at 16:58:12 with the same command line (the plain `.uproject`; no `-game`, no extra flags). `:8000` was owned by 12112 at 16:58:24. Editor log: `InternalLoadLibrary: 'GitClaudeUnrealTest'` :1693, `Starting MCP server on port 8000` :2233, `Total Editor Startup Time, took 11.562` :2412.
- Census: `UnrealEditor.exe` 12112 only, plus `CrashReportClientEditor` 26428 (`-MONITOR=12112`) and `UnrealTraceServer` 2768 (`--sponsor 12112`).
- **Binary is live:** `(Get-Process 12112).Modules` maps `UnrealEditor-GitClaudeUnrealTest.dll`, 10,066,944 B, sha256 `72DAC5DE…F475`, which is §6.6's restored build.
- `get_headless_status` → `editor_connected`. `is_pie_active` → `is_active: false`, a game-thread-bound liveness proof.

| reading (read-only tool, served PID 12112) | value |
|---|---|
| `is_loading_assets()` | `False` |
| level | `/Game/Maps/L_Arena.L_Arena` |
| PIE / dirty | `IN_PIE=False` · `DIRTY_CONTENT=[]` · `DIRTY_MAPS=[]` |
| **in-memory `BS_ERROR` walk** (`unreal.ObjectIterator(unreal.Blueprint)`) | **0 of 19 resident**. Liveness: total 19 > 0; histogram `BS_UNKNOWN` 17 · `BS_UP_TO_DATE` 2 |
| live-reader control `find_object(None, <level>)` | found |
| `BROKEN_BP_LOADED` (vacuous since `ab57522`, recorded only) | `False` |

- New log `Error:` lines: only the pre-existing `GameFeatureData` pair (:2071, :2076).
- **No PIE has been run on 12112 and nothing was loaded.** The only calls were read-only.

## 9. Commit `9b82e8d`: what shipped (`git show --stat HEAD`, 11 paths under `GitClaudeUnrealTest/`)

```
 .claude/pipeline/CONVENTIONS.md                          |  14 +-
 .claude/pipeline/TASKBOARD.md                            | 281 ++++++++++++++++++++-
 .claude/pipeline/handoffs/TASK-1560-programmer.md        | 214 ++++++++++++++++
 .claude/pipeline/handoffs/TASK-1564-programmer.md        |  92 +++++++
 .claude/pipeline/qa/TASK-1561.md                         | 188 ++++++++++++++
 .claude/pipeline/qa/TASK-1565.md                         | 129 ++++++++++
 Source/…/Siegebound/SiegeControlsHelpWidget.cpp          |  14 +-
 Source/…/Siegebound/SiegeControlsHelpWidget.h            |   7 +-
 Source/…/Siegebound/Tests/SiegeControlsHelpTest.cpp      |  13 +-
 Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md | 22 +-
 Tools/Verify/recipes/README.md                           |   2 +-
 11 files changed, 941 insertions(+), 35 deletions(-)
```

Committed with `git commit -F <msg> -- <11 paths>`. The index held exactly these 11 before the commit and is empty after it. The tree was clean immediately after the commit.

### Read-backs

- **`CONVENTIONS.md`: 2 hunks, both named on `TASK-1553`'s status line.** No foreign hunk.
  - `@@ -12429 +12429`: the `TASK-1444` preamble deferral, struck in place (`TASK-1444-B-DEFERRAL-STRUCK-2026-09-28`).
  - `@@ -12527,0 +12528,12`: `VER-§12` cl. 7g (`VER-12-7G-PIE-START-MODAL-RESIDENCY`).
- **`TASKBOARD.md`: 15 hunks at derive time, each inside a named row or block.**
  - The K5 block's "SUPERSEDED FOR ORDER" line, which carries the K6 marker.
  - `1545` (`TASK-1545-AFTER-BATCH-ACCOUNT-2026-09-28`).
  - `1552` status and blocked-by · `1553` status · `1563` status and blocked-by (the `TASK-1566` unpark).
  - `1560`: my `built` flip, plus the `TASK-1560-GUARD-KEPT-ACCOUNT` bullet.
  - `1561`, `1564`, `1565`: the gate flips.
  - `1562`: my status line, plus the `TASK-1562-AMENDED` bullet.
  - The 248-line K6 insertion: the `K6-POST-BATCH-2026-09-28` block and rows `1566`–`1580`.
  - No assignee-line hunk.

## 10. Board flips (`SC-§103`), written after the commit: 9 lines, 9 hunks

- `1560` `built` → `done` — COMMITTED `9b82e8d`
- `1561` → report COMMITTED
- `1562` → `done` — COMMITTED `9b82e8d`
- `1564` `qa-passed` → `done` — COMMITTED
- `1565` → report COMMITTED
- `1552` / `1553` / `1563` / `1566` → `— COMMITTED 9b82e8d (host TASK-1562)` appended

## 11. One-cycle lag: the next host must carry these

- `TASKBOARD.md`: the 9 flips in §10 (`git diff --stat` 9+/9−).
- `handoffs/TASK-1562-buildmaster.md` (this file, untracked).

## 12. Named and left; held

- **HELD:** nothing. No dirty path went unclaimed, and no `.uasset` / `.umap` was dirty.
- Not staged, by law: `CLAUDE.md` (clean anyway) and `.claude/agents/*` (clean).

## 13. Findings for the manager (reported here, not boarded)

1. **QA W1 is now observed live, not only simulated.** Both guard failures print "(T1/T2: citations and markup live in comments)", which is the wrong rule class for `::` / `()` (T5). This is the follow-up row QA §5 W1 asked the manager to board. If that row changes the label, any later arm must match the new text.
2. **W2 / N1** (the `.h` `Detail` / `OneLine` doc provenance) ship unchanged in this commit, as the fence required. They now also sit in the editor ToolTip metadata that this build regenerated (§5).

## Not examined / limitations

- **No runtime path was exercised.** The row has no runtime criterion (no 5b). The help pages' on-screen text did not change in this commit (comment and test bytes only; the arm's literal was restored before the final build).
- **One suite run per arm state.** There was one red run and one green run, not replicates.
- **The `BS_ERROR` walk carries a liveness control only.** No discriminating fixture exists since the delete (`VER-§12` cl. 7g, control strength).
- **Two same-source DLLs differ.** The first build's DLL (`eedaa655…`) and the restored build's (`72dac5de…`) came from the same source but hash differently (MSVC link non-determinism). Which bytes differ was not examined. Source identity is carried by the file sha256.
- **The exec-symbol "pre" set** is the generated files on disk from the previous compile (`TASK-1538`'s tree, which the `.gen.cpp` mtime of 2026-09-26 shows had not been regenerated since). It is not a `HEAD` build.
- **The adaptive-build list** names `DeckBuilderWidget.cpp` and `SiegeMenuInputSubsystem.cpp`, which were not modified at my instant. UBT's working-set cache was not examined, and only the 6 actions listed were compiled.
