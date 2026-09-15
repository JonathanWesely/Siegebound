# TASK-1289 — [DECK-TOWER-WAVE-HOST] build-master handoff

Both rows ride (no lane split): `TASK-1270` at `qa-passed` (`qa/TASK-1287-report.md` line 1: PASS, 0 BLOCKER / 2 WARN / 5 NIT) and `TASK-1271` at `qa-passed` (`qa/TASK-1288-report.md` line 1: PASS, 0 BLOCKER / 1 WARN / 1 NIT). A previous dispatch of this leg died on an API error before doing anything (orchestrator measured at 21:16: editor PID 11576 up, DLL unchanged, index empty, no handoff, board unchanged); this leg started from scratch and re-measured all of it.

## §1 — built leg — 2026-09-14 — ✅ compile PASSED, suite 558/558 ×2, GUI editor relaunched PID 13388 → `TASK-1270` + `TASK-1271` = `built`; ⛔ STOPPED before cl. (4) (🧑 VER-§3 go owed)

### (1) Pre-flight, measured (git root ONE LEVEL UP, `SC-§102`) — HEAD `4a8a1bc`, 21:17 local
`git status --porcelain` — EXACTLY the expected set, no third author:
```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1271-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1288-report.md
```
`git diff --stat -- Source/`:
```
 .../Siegebound/ClimbableTower.h                    |   2 +
 .../Siegebound/DeckBuilderWidget.cpp               |  74 ++++--
 .../Siegebound/DeckBuilderWidget.h                 |  52 ++++-
 .../Siegebound/SiegePlayerController.cpp           |  56 +++++
 .../Siegebound/Tests/SiegeDeckSlotsTest.cpp        | 251 +++++++++++++++++++++
 5 files changed, 417 insertions(+), 18 deletions(-)
```
`ClimbableTower.h` = 1 hunk, 2 insertions (`UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug")` above `ActiveClimber` and above `ActiveClimberPathComp`) — the `TASK-1288` WARN's expected `2 insertions(+)` confirmed. The file set matches the two programmer handoffs' file tables (`TASK-1270-programmer.md` §2: the four deck/controller/test files; `TASK-1271-programmer.md` §7: `ClimbableTower.h` only). Absent, as required: `DeckLibrary.*` · any `.uasset` · `Config/SiegeCloudDev.ini` · anything under `Saved/`. Index EMPTY (`git diff --cached --stat` = nothing). ⛔ Nothing staged at any point in this leg.

**Editor census (`SC-§118`), `Get-CimInstance Win32_Process` by COMMAND LINE:** exactly ONE `UnrealEditor.exe` — PID **11576**, CreationDate 2026-09-14 14:56:15, `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"` ⇒ the GUI editor (project path, no `-game`); owner of the `:8000` listener. **No `-game` instance existed** — nothing of Jonathan's in reach. DLL before the compile: `UnrealEditor-GitClaudeUnrealTest.dll` 2026-09-14 14:53:27, 9,724,928 B (= the `TASK-1274` loop-1 binaries = HEAD's source). Pre-close guards on 11576: unreal-mcp `EditorToolset.EditorAppToolset.IsPIERunning` → `false`; Aura `is_pie_active` → `is_active: false`; in-process `DIRTY_CONTENT []` / `DIRTY_MAPS []` / `DIRTY_COUNT 0`; editor world `L_MainMenu` (left there by the `TASK-1274` verify leg). Nothing dirty ⇒ no save question; nothing saved.

### (2a) Graceful quit — remote-exec lane (scratchpad `quit_editor_remote_1289.py` = the hardened `TASK-1281` §2 script with the census PID set to 11576; ABORTS on `DIRTY_COUNT ≠ 0` or PID mismatch)
Node census 1 (`node_id 596B9CDA4EBB9A223FC17C8F60B86D47`, `project_name GitClaudeUnrealTest`, machine JONATHANWESELY). Identity in-process: `os.getpid()` → **11576**. `unreal.SystemLibrary.quit_editor()` → `success: True` at **21:17:49**. Exit proof at **21:18:00**: `UnrealEditor*` processes 0, `:8000` listeners 0. ⛔ No kill, no save, no Live Coding.

### (2b) Compile — `CLAUDE.md` Build command, verdict FROM THE LOG (scratchpad `build-TASK-1289.log`, 43 lines)
Start 21:18:09, end 21:18:30 (wall 21 s). `Build.bat` exit 0 — recorded, not consulted.
```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: ClimbableTower.cpp, DeckBuilderWidget.cpp, SiegePlayerController.cpp, SiegeDeckSlotsTest.cpp
Using Unreal Build Accelerator local executor to run 16 action(s)
[1/16] Compile [x64] ClimbableTower.cpp
[2/16] Compile [x64] Module.GitClaudeUnrealTest.12.cpp
[3/16] Compile [x64] DeckBuilderWidget.cpp
[4/16] Compile [x64] Module.GitClaudeUnrealTest.14.cpp
[5/16] Compile [x64] Module.GitClaudeUnrealTest.10.cpp
[6/16] Compile [x64] Module.GitClaudeUnrealTest.15.cpp
[7/16] Compile [x64] Module.GitClaudeUnrealTest.11.cpp
[8/16] Compile [x64] SiegeDeckSlotsTest.cpp
[9/16] Compile [x64] Module.GitClaudeUnrealTest.1.cpp
[10/16] Compile [x64] Module.GitClaudeUnrealTest.3.cpp
[11/16] Compile [x64] Module.GitClaudeUnrealTest.7.cpp
[12/16] Compile [x64] SiegePlayerController.cpp
[13/16] Compile [x64] Module.GitClaudeUnrealTest.8.cpp
[14/16] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[15/16] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[16/16] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Total time in Unreal Build Accelerator local executor: 16.71 seconds
Result: Succeeded
Total execution time: 20.08 seconds
```
`Result: Succeeded` at log line 41. 16 actions (vs 7 / 4 in `TASK-1281`) — the header change fanned out to the unity blobs holding `ClimbableTower.h` includers, as predicted. Warnings: **0** (`grep -ci warning` = 0 — none on any changed file). No `0x800711C7` (not Smart App Control). UHT regenerated `Intermediate/Build/Win64/UnrealEditor/Inc/GitClaudeUnrealTest/UHT/ClimbableTower.gen.cpp` at 21:18:12. `UnrealEditor-GitClaudeUnrealTest.dll` mtime **2026-09-14 21:18:29**, 9,756,160 B (was 9,724,928 B).

### (2c) Suite on the NEW binaries — run TWICE (`SC-§114`), `Tools/run_suite_bounded.ps1 -Mode Suite -Filter Siegebound -OverallSeconds 540 -BootSeconds 300 -StallSeconds 180 -Porcelain`
Baseline = **556**: `TASK-1281` §2 (2c) measured 556/556 ×2 on the identical pre-compile DLL (14:53:27, 9,724,928 B — the file this leg found on disk), and no `Source/` change landed between that DLL and HEAD `4a8a1bc` (`158bb42` committed exactly that source; `4a8a1bc` is board-only). Not re-run on the old binaries this leg (would need a revert + rebuild); the identity of the DLL is the evidence.

| run | log (`Saved/Logs/`) | PID | wall | started / completed / discovered | Success / Fail / Skipped | `Result={Fail}` lines | `RUNNER_EXIT` |
|---|---|---|---|---|---|---|---|
| 1 | `run_suite_bounded_suite_20260914-211849.log` | 7824 | 48 s | 558 / 558 / 558 | **558 / 0 / 0** | 0 | 0 |
| 2 | `run_suite_bounded_suite_20260914-211943.log` | 15412 | 48 s | 558 / 558 / 558 | **558 / 0 / 0** | 0 | 0 |

558 = baseline 556 + 2 (`TASK-1270`'s two tests; `TASK-1271` adds none). Dispatch echo 1/1 both runs.

**The two new tests' OWN lines — GREEN both runs** (log timestamps are UTC in this lane):
- run 1: `[2026.09.15-04.19.12:041][781]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalActiveDeckMatchStartNotice} Path={Siegebound.Deck.IllegalActiveDeckMatchStartNotice}` · `[2026.09.15-04.19.12:091][784]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalDeckCannotBecomeActive} Path={Siegebound.Deck.IllegalDeckCannotBecomeActive}`
- run 2: `[2026.09.15-04.20.05:751][791]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalActiveDeckMatchStartNotice} Path={Siegebound.Deck.IllegalActiveDeckMatchStartNotice}` · `[2026.09.15-04.20.05:784][793]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalDeckCannotBecomeActive} Path={Siegebound.Deck.IllegalDeckCannotBecomeActive}`

**Aura `401` race — fired both runs, victim NONE both runs:**
- run 1: `[2026.09.15-04.19.12:425][804]LogAura: Error: Response code: 401` — same ms/frame as `Test Completed. Result={Success} … Deck.UncapNegativeCountStillIllegal` (`:425 [804]`), before `Test Started … Deck.UncapUnknownCardStillIllegal` (`:427 [805]`) ⇒ between tests.
- run 2: `[2026.09.15-04.20.05:868][798]LogAura: Error: Response code: 401` — between the controller's `Test Started … Deck.MigrationFreshSave` (`:835 [796]`) and its `Test Completed. Result={Success}` (`:884 [799]`) ⇒ inside that test's controller-logged window, yet the test stayed green (the same test the race redded in `TASK-1281` §1 run 2). No red, nothing to attribute; the race is still live (~23 s after boot both runs).

### (3) Relaunch record (`SC-§118`, GUI editor only)
Pre-launch census: 0 editor processes (both suite `UnrealEditor-Cmd.exe` instances had exited). `Start-Process`, SAME shape (relative `.uproject`, working dir `Engine/Binaries/Win64`, ⛔ no `-game`) → PID **13388**, CreationDate **2026-09-14 21:21:02**, command line `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"`. `:8000` listening, owner 13388, at 21:21:12 (10 s). Aura `get_headless_status`: `editor_running_bridge_unreachable` on the first poll (still booting), then **`editor_connected`**. DLL mtime 21:18:29 < editor start 21:21:02 ✓. Live log `Saved/Logs/GitClaudeUnrealTest.log` line 1 `Log file open, 09/14/26 21:21:03` (matches the start); `:1695 LogModuleManager: InternalLoadLibrary: 'GitClaudeUnrealTest' ('C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll')` at 04.21.09 UTC — the editor is on the new binaries. ⚠️ The verifier reads THIS instance's log (`VER-§1` cl. 2).

Read-backs on 13388:
- Aura `get_unreal_context` → `current_level_path: /Game/Maps/L_Arena.L_Arena` (the startup map — cl. (3)'s `load_level L_Arena` was already satisfied, no load issued), windows `GitClaudeUnrealTest - Unreal Editor` + `Message Log`.
- unreal-mcp `editor_toolset.toolsets.asset.AssetTools.is_dirty("/Game/Maps/L_Arena")` → **`{"returnValue":false}`** (1271 acceptance (4), row cl. (3)); `is_dirty("/Game/Maps/L_MainMenu")` → **`{"returnValue":false}`**.
- Aura read-only Python in-process: `PID 13388` · `DIRTY_CONTENT []` · `DIRTY_MAPS []` · **`DIRTY_COUNT 0`** · `WORLD L_Arena`.
- **1271 reflection read-back** (same Python call): `unreal.ClimbableTower.static_class()` = `/Script/GitClaudeUnrealTest.ClimbableTower`; on the CDO `get_editor_property("ActiveClimber")` → `READ ok value=None`, `get_editor_property("ActiveClimberPathComp")` → `READ ok value=None` (both names resolve as reflected properties); the Python class doc lists `active_climber (Character): [Read-Only]` and `active_climber_path_comp (Object): [Read-Only]`. Property class from the UHT output this compile generated (`ClimbableTower.gen.cpp`, 21:18:12):
```
372: const UECodeGen_Private::FWeakObjectPropertyParams UHT_STATICS::NewProp_ActiveClimber = { "ActiveClimber", nullptr, (EPropertyFlags)0x0044000000022801, UECodeGen_Private::EPropertyGenFlags::WeakObject, …, STRUCT_OFFSET(AClimbableTower, ActiveClimber), Z_Construct_UClass_ACharacter, … };
373: const UECodeGen_Private::FWeakObjectPropertyParams UHT_STATICS::NewProp_ActiveClimberPathComp = { "ActiveClimberPathComp", nullptr, (EPropertyFlags)0x0044000000022801, UECodeGen_Private::EPropertyGenFlags::WeakObject, …, STRUCT_OFFSET(AClimbableTower, ActiveClimberPathComp), Z_Construct_UClass_UObject, … };
```
⇒ both are **weak-object properties** (`FWeakObjectProperty`, `EPropertyGenFlags::WeakObject`), property classes `ACharacter` / `UObject`; low flag bits `0x22801` = `CPF_Edit 0x1` | `CPF_DisableEditOnTemplate 0x800` | `CPF_Transient 0x2000` | `CPF_EditConst 0x20000` = `VisibleInstanceOnly` + `Transient`, as the row prescribed (high bits not decoded here).
- Fresh-log `Error:` census (at 21:22:18): 6 = the standing GameFeatureData pair (`:2073`, `:2078`, pre-existing) + two `LogModelContextProtocol` session-reinit lines from my first unreal-mcp call after the relaunch (`:2641`, `:2642`) + two `LogToolsetRegistry: Error: Toolset 'AssetTools' not found` (`:2648`, `:2653`) — MINE, the short toolset name; the fully-qualified `editor_toolset.toolsets.asset.AssetTools` answered. None from `LogSiege*` / climb categories. Nothing dirty, nothing saved, no PIE, no Live Coding.

### Status flips (this leg)
- `TASK-1270`: `qa-passed` → **`built`** (compile `Result: Succeeded`, 16 actions, 20.08 s, suite 558/558 ×2, both new tests green, GUI editor relaunched PID 13388; awaiting 🧑 VER-§3 go) — prior text kept after `· was:`.
- `TASK-1271`: `qa-passed` → **`built`** (same compile/suite/relaunch + the reflection read-back + `is_dirty` L_Arena false) — prior text kept after `· was:`.
- `TASK-1289`: `backlog` → **`in-progress — built leg ✅`**. ⛔ Nothing staged, no commit, no push. ⛔ STOPPED before cl. (4): the orchestrator announces the PIE drive (GUI editor PID 13388 by command line; no `-game` instance exists) and waits for Jonathan's go in Claude Code; the verifier leg (cl. 5) is the orchestrator's dispatch, not this host's.

### Follow-ups
1. The Aura `401` race (`TASK-1281` §1 follow-up 1) — still live, fired ~23 s after boot in both runs; run 2 landed inside `Deck.MigrationFreshSave`'s controller window without redding it. Still needs its row.
2. Verifier note for cl. (5) 1271: the reflection is proven on the CDO; the row's binding one-liner (`get_actor_property_in_pie ActiveClimber` on a staged `BP_Building_WatchTower_C`) is still owed and is the verdict — this read-back does not replace it.

## §2 — 5c leg, LANE 1271 ALONE — 2026-09-14 — ✅ committed by pathspec (7 files, 1 PNG via LFS); lane 1270 awaiting loop-1 re-entry at cl. (2)

Authority: the manager's dated ⚖️ LANE-RULE AMENDMENT 2026-09-14 on this row's `blocked-by` line (read in full before any write) — `qa/TASK-1270-verify.md` line 1 `Verdict: VERIFY-FAILED` · `qa/TASK-1271-verify.md` line 1 `Verdict: VERIFIED` (partial 2/6 observable). The verifier's ⚙️ Dev & QA post is visibility, not authorization. 🧑 Jonathan AWAY (unattended). ⛔ No compile, ⛔ no editor touch (GUI editor PID 13388 left up, untouched), ⛔ not pushed.

### (1) Pre-flight, measured (git root ONE LEVEL UP, `SC-§102`) — HEAD `4a8a1bc`, main ahead 19 of origin
First read (start of leg, before the copy-out) = **16 entries**; re-read at 22:10:49 immediately before staging = **17 entries** = the same 16 + this host's own PNG copy-out from (2). No entry appeared or vanished between the two reads. The 22:10:49 read:
```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1271-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1289-buildmaster.md
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1271-t00m27s-staged-watchtower-activeclimber-read.png
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1271-verify.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1288-report.md
```
`git diff --stat -- GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` → `1 file changed, 2 insertions(+)` (1 hunk `@@ -954,9 +954,11 @@` — the two `UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug")` lines above `ActiveClimber` and `ActiveClimberPathComp`, nothing else) ✓. Index EMPTY before staging (`git diff --cached --name-status` = nothing). `Config/SiegeCloudDev.ini` and `.claude/settings.local.json` read `!!` (ignored) and are absent from porcelain; no `Saved/**`, no `.uasset`, no `DeckLibrary.*` in porcelain. **Third author: NONE** — every porcelain entry is on one of the two lists (staged 7 below, or 1270's).

**⛔ MUST NOT STAGE — `TASK-1270`'s (loop 1 in flight), NAMED with their porcelain codes as left on disk, NOT staged:**

| porcelain | path (from the git root) |
|---|---|
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` |
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` |
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` (NEW in loop 1) |
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` |
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h` (NEW in loop 1) |
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp` (NEW in loop 1) |
| ` M` | `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp` |
| `??` | `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md` |
| `??` | `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md` |
| `??` | `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify.md` |
| (absent) | any `VER-TASK-1270-*` PNG — none exists under `playtest-evidence/2026-09-14/` (loop 0's four stills are still only in `Saved/AuraVerify/`; their promotion is the re-entry's cl. (1) per the EVIDENCE RULING) |

### (2) Evidence copy-out (`VER-§4` cl. 1) — mapping confirmed against `qa/TASK-1271-verify.md` `## Evidence (promoted)` line 18
`Saved/AuraVerify/pie_game_c10_three_quarter_t27.75s_f62744.png` → `.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1271-t00m27s-staged-watchtower-activeclimber-read.png` (`cp -p` at 22:09:28; the target did not exist before; single attempt ⇒ no `-a<N>`).
```
d1d2699ed30dc12fad31577553894ee23f7c79a3c745219c51eb6e3bd46ae03f *Saved/AuraVerify/pie_game_c10_three_quarter_t27.75s_f62744.png
d1d2699ed30dc12fad31577553894ee23f7c79a3c745219c51eb6e3bd46ae03f *.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1271-t00m27s-staged-watchtower-activeclimber-read.png
```
`cmp` → equal. `git check-attr filter` → `filter: lfs`; `git check-ignore` → not ignored (rc 1).

**LFS read-back (`SC-§68`, ⛔ never size):** the staged blob `git show :GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1271-t00m27s-staged-watchtower-activeclimber-read.png` =
```
version https://git-lfs.github.com/spec/v1
oid sha256:d1d2699ed30dc12fad31577553894ee23f7c79a3c745219c51eb6e3bd46ae03f
size 1618963
```
oid = sha256 of source = sha256 of target ✓. The local LFS object `.git/lfs/objects/d1/d2/d1d2699ed30dc12fad31577553894ee23f7c79a3c745219c51eb6e3bd46ae03f` exists and re-hashes to the same value. The COMMIT's blob (`git show HEAD:<path>`) is read back after the commit and reported to the orchestrator (`SC-§104` — the commit, not the index, is the claim).

### (3) Staged — by pathspec from the git root, exactly 7
`git diff --cached --name-status`:
```
M	GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
A	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1271-programmer.md
A	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1289-buildmaster.md
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1271-t00m27s-staged-watchtower-activeclimber-read.png
A	GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1271-verify.md
A	GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1288-report.md
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h
```
count = 7. No UE-plugin auto-staged file (the index held exactly these 7 — nothing to unstage). This handoff is re-added after §2 is appended and re-grepped. `git diff --cached --stat -- …/ClimbableTower.h` → `1 file changed, 2 insertions(+)`. `TASKBOARD.md` sweep ACCEPTED per the amendment (it carries 1270's bounce lines and the programmer's loop-1 `ready-for-qa` status line — by design, not a leak).

**Value-grep (`git show :<path>` per staged blob; patterns `eyJ[A-Za-z0-9_-]{20,}` · `sb_secret_[A-Za-z0-9_-]{10,}` · `hf_[A-Za-z0-9]{20,}`):** positive control — three synthetic lines (`eyJ` + 24 chars · `sb_secret_` + 12 · `hf_` + 24) → combined pattern `3`, each pattern alone `1` / `1` / `1` (the grep fires). Staged blobs:

| blob | eyJ | sb_secret_ | hf_ |
|---|---|---|---|
| `TASKBOARD.md` | 0 | 0 | 0 |
| `handoffs/TASK-1271-programmer.md` | 0 | 0 | 0 |
| `handoffs/TASK-1289-buildmaster.md` | 0 | 0 | 0 (re-run on the §2-appended blob before the commit) |
| `VER-TASK-1271-…-read.png` (LFS pointer text) | 0 | 0 | 0 |
| `qa/TASK-1271-verify.md` | 0 | 0 | 0 |
| `qa/TASK-1288-report.md` | 0 | 0 | 0 |
| `Siegebound/ClimbableTower.h` | 0 | 0 | 0 |

### (4) The commit
First line PRESCRIBED by the amendment: *TASK-1271: AClimbableTower::ActiveClimber/ActiveClimberPathComp reflected (VisibleInstanceOnly, Transient) for the verifier; verified VERIFIED partial 2/6 (gate TASK-1288, host TASK-1289 lane-rule split — TASK-1270 bounced VERIFY-FAILED loop 1)* — body = the two house trailer lines. Verified on `git show --stat HEAD` (staged 7 = committed 7), ⛔ never the index. The hash is written onto the three board status lines in a TASKBOARD-only commit 2 (`SC-§103`, the `TASK-1272/1279/1284/1281` precedent) — this file carries no hash because it rides inside the commit it would name.

### (5) Flips (`SC-§103`) — placeholder `<hash — pending, back-referenced in commit 2 (SC-§103)>` in commit 1, the hash in commit 2
- `TASK-1271`: `built` → **`done — committed <hash> · verified VERIFIED partial (2/6 observable) 2026-09-14 (qa/TASK-1271-verify.md …) · host TASK-1289 lane split …`** — prior text kept after `· was:`, including its `· verify: VERIFIED 2026-09-14, qa/TASK-1271-verify.md, partial (2/6 observable)` note and history tail.
- `TASK-1288`: `backlog` (QA did not flip its own row) → **`done — PASS (… line 1: Verdict PASS — 0 BLOCKER · 1 WARN · 1 NIT …) · committed <hash> (host TASK-1289)`** — prior text kept after `· was:`.
- `TASK-1289` (THIS row): → **`in-progress — lane 1271 committed <hash>; lane 1270 awaiting loop-1 re-entry at cl. (2)`** — ⛔ NOT `done`; prior text kept after `· was:`.
- ⛔ `TASK-1270` / `TASK-1287` untouched (1270 carries the programmer's loop-1 `ready-for-qa` line as found; 1287 reads `backlog` as found — the concurrent loop-1 re-gate was told not to write the board).
- Board re-read by sha256 immediately before each write (`0dab3e83…` before the commit-1 flips, `3932b776…` before a one-phrase count correction on this row's line); every replacement asserted an exactly-1 match; line count 36095 unchanged.

### (6) DECLARED, not measured
The committed tree (HEAD `4a8a1bc` + `ClimbableTower.h` alone) was **never compiled in isolation** — the §1 DLL (21:18:29) carried both rows' diffs. Accepted by the amendment: 1271's two lines sit in a file 1270 does not touch, `ClimbableTower.h:5–12` includes none of 1270's changed headers (manager read), and the combined tree is compiled again at 1270's re-entry at cl. (2).

### Follow-ups
1. **1270 re-entry, cl. (1):** the suite baseline is RE-MEASURED on the post-1271 HEAD; the 1270 source set is now 7 files (loop 1 added `SiegePlayerController.h` + `CardHandWidget.{h,cpp}`) — the re-entry pathspec takes every file loop 1's handoff names, not the §1 four.
2. `TASK-1288` NIT (`ClimbableTower.h` ~978/980 "ActiveClimber precedent" wording half-stale) — carried, a later touch.
3. The Aura `401` race (§1 follow-up 1) — still needs its row.

## §3 — 1270 RE-ENTRY (loop 1) at cl. (1)–(3) — 2026-09-14 — under the manager's ⚖️ LANE-RULE AMENDMENT "1270 RE-ENTRY" + "EVIDENCE RULING" clauses

Authority: `TASK-1289` `blocked-by` line (the dated amendment, read in full before any write) + the row's cl. (1)–(3). Gate: `TASK-1270` at `qa-passed — loop 1` (`qa/TASK-1287-report.md` line 1 = the loop-1 PASS, 0 blockers). 🧑 Jonathan AWAY (`VER-§3` cl. 5 unattended, as recorded on this row). ⛔ STOP at `built` — no commit, nothing staged, never pushed; the loop-1 verify is the orchestrator's dispatch.

### (0) Loop-0 evidence preservation — EVIDENCE RULING (i) + (ii) — DONE FIRST, 22:24:30 local, before any pre-flight/quit/compile
**(i) the loop-0 verify report, byte-copied** (`cp -p`; the target did not exist before):
```
054b5c88adc943dceeeba4f3bca37f40efc246fba4e425940e4e7b7abeae734e *.claude/pipeline/qa/TASK-1270-verify.md
054b5c88adc943dceeeba4f3bca37f40efc246fba4e425940e4e7b7abeae734e *.claude/pipeline/qa/TASK-1270-verify-loop0.md
```
sha256 EQUAL · `cmp` equal · `head -1 qa/TASK-1270-verify-loop0.md` = `Verdict: VERIFY-FAILED`. The canonical `qa/TASK-1270-verify.md` is left in place for the loop-1 verifier to overwrite (`VER-§7` cl. 4 — its only legal file; its `head -1` must become the current verdict, `VER-§1` cl. 1). Not ignored (`git check-ignore` rc 1).

**(ii) the four loop-0 stills, COPIED (sources stay in `Saved/AuraVerify/`, re-listed after the copy)** — mapping taken from `qa/TASK-1270-verify-loop0.md` `## Evidence (promoted)` lines 20–23; the four targets did not exist before:

| source (`Saved/AuraVerify/`) | target (`.claude/pipeline/playtest-evidence/2026-09-14/`) | sha256 (source = target) |
|---|---|---|
| `pie_composited_c1_t0.02s_f36751.png` | `VER-TASK-1270-a1-t00m00s-hud-refusal-slot-no-deck-notice.png` | `8b3b26d97da5b57b65ef5ecc66b2596ea8c4737fe598b8a3d70f4a169bf7be23` |
| `pie_composited_c4_t2.08s_f49469.png` | `VER-TASK-1270-a2-t00m02s-hud-refusal-slot-no-deck-notice.png` | `52be55d6a420e88da51e2dcb691baed8344ffb1ead135a55ad8710795a299fb0` |
| `pie_composited_c8_t10.48s_f49952.png` | `VER-TASK-1270-a2-t00m10s-hud-refusal-slot-no-deck-notice.png` | `7cf79619b5c4d7c67010ae3e29d787f54d22e92b2f825472a2e41e177aa72a7d` |
| `pie_composited_c9_t78.30s_f54006.png` | `VER-TASK-1270-a2-t01m18s-hud-refusal-slot-positive-control.png` | `a076eda5c02825d63ede58a9aca39a8fa00267796eaa025b159a649b00209b3b` |

Each pair `sha256sum`-equal AND `cmp`-equal. `git check-attr filter` → `filter: lfs`; not ignored. ⛔ UNSTAGED (index empty, `git diff --cached --name-status` = 0 lines) — they ride the 1270 commit beside `qa/TASK-1270-verify-loop0.md`; the LFS oid-vs-sha256 three-way (`SC-§68`) is owed at that commit, on the COMMIT's blob. Loop 1's attempts continue at `a3` (ruling (iii)) — no `VER-TASK-1270-a3*` name exists yet.

### (1) Pre-flight, measured (git root ONE LEVEL UP, `SC-§102`) — HEAD `6ffc2a2`, main ahead 21 of origin, 22:25 local (after step 0)
`git status --porcelain` — 17 entries = EXACTLY the dispatch's expected set + step 0's five copies + this handoff; **no third author**:
```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1289-buildmaster.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a1-t00m00s-hud-refusal-slot-no-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t00m02s-hud-refusal-slot-no-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t00m10s-hud-refusal-slot-no-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t01m18s-hud-refusal-slot-positive-control.png
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify-loop0.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md
```
(` M handoffs/TASK-1289-buildmaster.md` = this §3; the file was committed in `e1a7f5b`.) Index EMPTY (`git diff --cached --name-status` = nothing). Absent, as required: any `.uasset` · `DeckLibrary.*` · `ClimbableTower.h` (committed in `e1a7f5b`, clean now) · anything under `Saved/`. `Config/SiegeCloudDev.ini` and `.claude/settings.local.json` read `!!` (ignored). `git diff --stat -- GitClaudeUnrealTest/Source/`:
```
 .../Siegebound/CardHandWidget.cpp                  |  17 +
 .../Siegebound/CardHandWidget.h                    |  17 +
 .../Siegebound/DeckBuilderWidget.cpp               |  74 +++-
 .../Siegebound/DeckBuilderWidget.h                 |  52 ++-
 .../Siegebound/SiegePlayerController.cpp           | 111 ++++++
 .../Siegebound/SiegePlayerController.h             |  30 ++
 .../Siegebound/Tests/SiegeDeckSlotsTest.cpp        | 374 +++++++++++++++++++++
 7 files changed, 657 insertions(+), 18 deletions(-)
```
= the 7 source files `handoffs/TASK-1270-programmer.md` names (§2 loop 0 + `L1.3` loop 1). Gate: `qa/TASK-1287-report.md` line 1 = `# QA Report — TASK-1287 — Verdict (loop 1, 2026-09-14): PASS — 0 BLOCKER / 1 WARN / 4 NIT — …`.

**Editor census (`SC-§118`), `Get-CimInstance Win32_Process` by COMMAND LINE, 22:25:15:** exactly ONE `UnrealEditor.exe` — PID **13388**, CreationDate 2026-09-14 21:21:02, `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"` ⇒ the GUI editor; `:8000` owner 13388. **No `-game` instance** — nothing of Jonathan's in reach, no sign he is back. DLL before the compile: 2026-09-14 21:18:29, 9,756,160 B (= §1's loop-0 build). Pre-close guards on 13388: Aura `is_pie_active` → `is_active: false`; Aura read-only Python in-process `PID 13388` · `DIRTY_CONTENT []` · `DIRTY_MAPS []` · `DIRTY_COUNT 0` · `IN_PIE False` · `WORLD L_Arena`. Nothing dirty ⇒ no save question; nothing saved.

### (2a) Graceful quit — remote-exec lane (scratchpad `quit_editor_remote_1289_l1.py` = §1's hardened script with the census PID → 13388 and one added guard: ABORT unless `IN_PIE False`)
Node census 1 (`node_id E874D4594290CCF40DC0AFA617718FFD`, `project_name GitClaudeUnrealTest`, machine JONATHANWESELY). In-process: `PID 13388` · `DIRTY_COUNT 0` · `IN_PIE False`. `unreal.SystemLibrary.quit_editor()` → `success: True` at **22:25:59**. Exit proof at **22:26:09**: `UnrealEditor*` processes 0, `:8000` listeners 0, `Get-Process -Id 13388` → not found. ⛔ No kill, no save, no Live Coding.

### (2b) Compile — `CLAUDE.md` Build command, verdict FROM THE LOG (scratchpad `build-TASK-1289-l1.log`, 46 lines)
Start 22:26:18, end 22:26:45 (wall 27 s). `Build.bat` exit 0 — recorded, not consulted.
```
15: [Adaptive Build] Excluded from GitClaudeUnrealTest unity file: CardHandWidget.cpp, DeckBuilderWidget.cpp, SiegePlayerController.cpp, SiegeDeckSlotsTest.cpp
17: Using Unreal Build Accelerator local executor to run 19 action(s)
22: [2/19] Compile [x64] CardHandWidget.cpp
31: [11/19] Compile [x64] SiegeDeckSlotsTest.cpp
34: [14/19] Compile [x64] SiegePlayerController.cpp
37: [17/19] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
38: [18/19] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
39: [19/19] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
41: Total time in Unreal Build Accelerator local executor: 23.21 seconds
44: Result: Succeeded
45: Total execution time: 26.50 seconds
```
`Result: Succeeded` at log line 44. **19 actions** (vs §1's 16): 3 non-unity files + 13 unity blobs + lib + dll + metadata — the two loop-1 headers (`SiegePlayerController.h`, `CardHandWidget.h`) fanned out to their includers' unity blobs; `DeckBuilderWidget.cpp` is excluded from unity but NOT recompiled (unchanged since §1's build — loop 1 did not touch it). Warnings: **0** (`grep -ci warning` = 0; none on any of the 7 changed files). No `0x800711C7` (not Smart App Control). `UnrealEditor-GitClaudeUnrealTest.dll` mtime **2026-09-14 22:26:44**, 9,765,888 B (was 9,756,160 B).

### (2c) Suite on the NEW binaries — run TWICE (`SC-§114`), `Tools/run_suite_bounded.ps1 -Mode Suite -Filter Siegebound -OverallSeconds 540 -BootSeconds 300 -StallSeconds 180 -Porcelain`

| run | log (`Saved/Logs/`) | PID | wall | started / completed / discovered | Success / Fail / Skipped | `Result={Fail}` lines | `RUNNER_EXIT` |
|---|---|---|---|---|---|---|---|
| 1 | `run_suite_bounded_suite_20260914-222703.log` | 21056 | 64 s | 559 / 559 / 559 | **559 / 0 / 0** | 0 | 0 |
| 2 | `run_suite_bounded_suite_20260914-222813.log` | 28684 | 55 s | 559 / 559 / 559 | **559 / 0 / 0** | 0 | 0 |

Dispatch echo 1/1 both runs. `grep -c "Test Completed. Result={Success}"` = 559 in each log.

**Baseline — DERIVED, not re-measured, and why.** The amendment's re-entry clause says "baseline RE-MEASURED on the post-1271 HEAD"; a true re-measure on HEAD `6ffc2a2`'s source would need binaries compiled WITHOUT 1270's 7-file diff (stash → compile → suite → unstash → compile again): two extra compiles and an editor-less window over a tree the gate has already passed, for a number already measured twice. This dispatch prescribed the derivation instead. Every term is measured elsewhere: **556** = `TASK-1281` §2 (2c) 556/556 ×2 on the DLL built from `158bb42`'s source · `TASK-1271` (`e1a7f5b`) adds no test (2 `UPROPERTY` lines in `ClimbableTower.h`; `handoffs/TASK-1271-programmer.md` §7) and `6ffc2a2` is board-only ⇒ post-1271 HEAD baseline = 556 · §1 above measured **558**/558 ×2 on 556 + loop 0's two tests (`IllegalActiveDeckMatchStartNotice`, `IllegalDeckCannotBecomeActive`) + 1271's header — consistent with +2, +0 · loop 1 adds exactly one (`IllegalDeckNoticeReachesALateListenerExactlyOnce`, `handoffs/TASK-1270-programmer.md` `L1.4`) ⇒ expected **559** = measured **559** ×2. The three new test names appear by their OWN lines below, so the +3 is attributed, not inferred from the total.

**The three `Siegebound.Deck.Illegal*` tests' OWN lines — GREEN both runs** (log clock UTC):
- run 1:
  - `:4566 [2026.09.15-05.27.38:262][604]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalActiveDeckMatchStartNotice} Path={Siegebound.Deck.IllegalActiveDeckMatchStartNotice}`
  - `:4573 [2026.09.15-05.27.38:312][607]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalDeckCannotBecomeActive} Path={Siegebound.Deck.IllegalDeckCannotBecomeActive}`
  - `:4585 [2026.09.15-05.27.38:362][610]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalDeckNoticeReachesALateListenerExactlyOnce} Path={Siegebound.Deck.IllegalDeckNoticeReachesALateListenerExactlyOnce}`
- run 2:
  - `:4561 [2026.09.15-05.28.39:298][816]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalActiveDeckMatchStartNotice} Path={Siegebound.Deck.IllegalActiveDeckMatchStartNotice}`
  - `:4568 [2026.09.15-05.28.39:348][819]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalDeckCannotBecomeActive} Path={Siegebound.Deck.IllegalDeckCannotBecomeActive}`
  - `:4580 [2026.09.15-05.28.39:398][822]LogAutomationController: Display: Test Completed. Result={Success} Name={IllegalDeckNoticeReachesALateListenerExactlyOnce} Path={Siegebound.Deck.IllegalDeckNoticeReachesALateListenerExactlyOnce}`

**Aura `401` race — fired both runs, victim NONE both runs (not counted):**
- run 1: `:5841 [2026.09.15-05.27.45:818][ 35]LogAura: Error: Response code: 401` (+ `:5844`, the content line `{"message":"User not authenticated",…}`) — same ms/frame as `:5838 Test Completed. Result={Success} … Siegebound.HeroLadderClimb.TheEndpointLiftIsReadFromTheHerosOwnCapsule` (`:818 [35]`), before `:5849 Test Started … HeroLadderClimb.TheExitReasonListIsClosedAndEnumeratedFresh` (`:821 [36]`) ⇒ between tests.
- run 2: `:4509 [2026.09.15-05.28.38:982][797]LogAura: Error: Response code: 401` (+ `:4512`) — same ms/frame as `:4508 Test Started … Siegebound.ControlsHelp.RightClickIsTaughtOnlyByTheRowsThatOwnIt` (`:982 [797]`), i.e. inside that test's controller window; it completed `:4518 Result={Success}` (`:016 [799]`) ⇒ no red.
Nothing red, nothing to attribute. The race is still live.

### (3) Relaunch record (`SC-§118`, GUI editor only)
Pre-launch census: 0 `UnrealEditor*` processes (both suite `UnrealEditor-Cmd.exe` instances had exited). `Start-Process`, SAME shape as §1 (relative `.uproject`, working dir `Engine/Binaries/Win64`, ⛔ no `-game`) → PID **26732**, CreationDate **2026-09-14 22:29:32**, command line `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"`. `:8000` listening, owner 26732, at 22:29:51 (19 s). Aura `get_headless_status` → **`editor_connected`**. DLL mtime 22:26:44 < editor start 22:29:32 ✓. **Live log for the verifier (`VER-§1` cl. 2): `Saved/Logs/GitClaudeUnrealTest.log`, line 1 `Log file open, 09/14/26 22:29:33`** (matches the start; §1's 21:21:03 log (PID 13388) rolled to `Saved/Logs/GitClaudeUnrealTest-backup-2026.09.15-05.26.07.log`); `:1695 [2026.09.15-05.29.47:021][  0]LogModuleManager: InternalLoadLibrary: 'GitClaudeUnrealTest' ('C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll')` — the editor is on the new binaries. The log was 2656 lines at read-back time, zero PIE lines.

Read-backs on 26732:
- Aura `get_unreal_context` → `current_level_path: /Game/Maps/L_Arena.L_Arena` (the startup map; no load issued), windows `GitClaudeUnrealTest - Unreal Editor` + `Message Log`.
- unreal-mcp `editor_toolset.toolsets.asset.AssetTools` `is_dirty("/Game/Maps/L_Arena")` → **`{"returnValue":false}`**; `is_dirty("/Game/Maps/L_MainMenu")` → **`{"returnValue":false}`**. (Two earlier calls passing the fully-qualified name as `tool_name` were refused `Unknown tool …` — mine, no engine effect; the short `tool_name` + full `toolset_name` answered.)
- Aura read-only Python in-process: `PID 26732` · `DIRTY_CONTENT []` · `DIRTY_MAPS []` · **`DIRTY_COUNT 0`** · `IN_PIE False` · `WORLD L_Arena`.
- Fresh-log `Error:` census: 4 = the standing GameFeatureData pair (`:2073`, `:2078`, pre-existing on every boot) + `:2641` `LogModelContextProtocol: Error: Unknown session id … client should reinitialize` and `:2642` `Call to unknown method "server/discover"` — MINE, the unreal-mcp session re-init on my first call after the relaunch. None from `LogSiege*`. Nothing dirty, nothing saved, no PIE, no Live Coding.

### Status flips (this leg) — board re-read by sha256 immediately before the write (`914dfde7…07ee`), each anchor asserted exactly-1 and under its own `####` header, line count 36095 unchanged, after `5038b61b…6f3b`
- `TASK-1270`: `qa-passed — loop 1` → **`built — loop 1: compiled Result: Succeeded, 19 actions, 26.50 s 2026-09-14, suite 559/559 ×2, GUI editor relaunched PID 26732; loop-0 report preserved as qa/TASK-1270-verify-loop0.md (sha256 054b5c88…); awaiting BINDING re-verify (unattended)`** + a bracketed pointer to this §3 — prior text kept after `· was:`.
- `TASK-1287`: `backlog` (QA never flipped its own row) → **`done — PASS (loop 0 + loop 1 re-gate), awaiting commit (host TASK-1289)`** + the line-1 verdict counts — prior text kept after `· was:`.
- `TASK-1289` (THIS row): → **`in-progress — lane 1271 committed e1a7f5b; lane 1270 loop-1 built ✅ …`** — ⛔ NOT `done`; prior text kept after `· was:`.
- ⛔ Nothing staged, no commit, no push. ⛔ STOPPED at `built`: the loop-1 BINDING re-verify is the orchestrator's dispatch (`VER-§3` cl. 5 unattended while his away-statement holds; attempts continue at `a3`, ruling (iii)); then cl. (5) the 1270 half, and cl. (6) the 1270 commit with the pathspec the amendment's "1270 RE-ENTRY" clause lists (the 7 source files above + `handoffs/TASK-1270-programmer.md` + `qa/TASK-1287-report.md` + `qa/TASK-1270-verify.md` + `qa/TASK-1270-verify-loop0.md` + the four loop-0 stills + loop 1's stills + the board + this handoff).

### Follow-ups
1. The Aura `401` race — still live in both runs, still unboarded (§1 follow-up 1, §2 follow-up 3).
2. Board text drift, flagged not fixed: the amendment's re-entry clause says "baseline RE-MEASURED on the post-1271 HEAD" while this dispatch prescribed a derivation; derived here with every term cited (above). The manager may want the clause to say "derived" for the record.
3. For the verifier: the PID and the log file changed — the verify reads PID **26732**'s `Saved/Logs/GitClaudeUnrealTest.log` (line 1 `Log file open, 09/14/26 22:29:33`), not loop 0's PID 13388 log (now a `-backup-` file).

## §4 — 1270 RE-ENTRY at cl. (6)–(7) — 2026-09-14 — the 5c commit that closes the wave (19 paths, 6 PNGs via LFS)

Authority: `TASK-1289` `blocked-by` LANE-RULE AMENDMENT — the "1270 RE-ENTRY" clause (pathspec shape + prescribed first line) and the "EVIDENCE RULING" (i)–(iv) — plus cl. (6)–(8). Gate: `qa/TASK-1287-report.md` line 1 = the loop-1 PASS. Verify: `qa/TASK-1270-verify.md` line 1 = **`Verdict: VERIFIED`** (loop 1, attempt `a3`, partial 1/6 observable — acceptance (3) pass on branch (a)). 🧑 Jonathan AWAY (unattended). ⛔ No compile, ⛔ no editor touch, ⛔ never pushed.

### (1) Evidence copy-out, loop 1 (`VER-§4` cl. 1; EVIDENCE RULING (iv)) — mapping confirmed against `qa/TASK-1270-verify.md` `## Evidence (promoted)` lines 20–21
Both targets absent before the copy (`test ! -e`); `cp -p` at 22:45:12; sources left in place in `Saved/AuraVerify/`.

| source (`Saved/AuraVerify/`) | target (`.claude/pipeline/playtest-evidence/2026-09-14/`) | sha256 (source = target) |
|---|---|---|
| `pie_composited_c1_t0.02s_f29117.png` | `VER-TASK-1270-a3-t00m00s-hud-refusal-slot-deck-notice.png` | `38a43f2afb8cb6f1e9d1ccd39c9f469ca9d84a5ff58dba701ab738a303efe1b3` |
| `pie_composited_c4_t67.82s_f32983.png` | `VER-TASK-1270-a3-t01m07s-hud-refusal-slot-positive-control.png` | `1d4f1908f990ba143a77bf9439abe3fcc85a1ccd94702b75efb005add2235820` |

Each pair `sha256sum`-equal AND `cmp`-equal. `git check-attr filter` → `filter: lfs` (both); `git check-ignore` rc 1 (not ignored).

### (2) Loop-0 preservation re-check (EVIDENCE RULING (i)+(ii)) — against §3's recorded hashes, NOT the canonical file (which loop 1 overwrote, as designed)
```
054b5c88adc943dceeeba4f3bca37f40efc246fba4e425940e4e7b7abeae734e *.claude/pipeline/qa/TASK-1270-verify-loop0.md
```
= §3 (0)(i) ⇒ byte-identical since preservation; `head -1` = `Verdict: VERIFY-FAILED`. The four loop-0 stills re-hash to §3 (0)(ii)'s table exactly: `a1-t00m00s` `8b3b26d9…bf7be23` · `a2-t00m02s` `52be55d6…5a299fb0` · `a2-t00m10s` `7cf79619…aa72a7d` · `a2-t01m18s-positive-control` `a076eda5…b00209b3b`.

### (3) Pre-flight, measured (git root ONE LEVEL UP, `SC-§102`) — HEAD `6ffc2a2`, main 0 behind / 21 ahead of origin, 22:45:24 local (after the copy-out)
`git status --porcelain` — **19 entries = EXACTLY the dispatch's expected set; no third author**:
```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1289-buildmaster.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a1-t00m00s-hud-refusal-slot-no-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t00m02s-hud-refusal-slot-no-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t00m10s-hud-refusal-slot-no-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t01m18s-hud-refusal-slot-positive-control.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a3-t00m00s-hud-refusal-slot-deck-notice.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a3-t01m07s-hud-refusal-slot-positive-control.png
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify-loop0.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md
```
Index EMPTY (`git diff --cached --name-status` = 0 lines). `git diff --stat -- …/ClimbableTower.h` = nothing (clean; committed in `e1a7f5b`) ✓. Fence grep of porcelain for `.uasset|DeckLibrary|SiegeCloudDev|/Saved/|settings.local` → rc 1 (none) ✓; `Config/SiegeCloudDev.ini` + `.claude/settings.local.json` read `!!` (ignored). `git diff --stat -- GitClaudeUnrealTest/Source/` = `7 files changed, 657 insertions(+), 18 deletions(-)` — byte-for-byte the §3 stat (the tree the §3 compile + 559/559 ×2 suite + the loop-1 verify ran on). Read-only editor census (`Get-CimInstance`, by command line): exactly ONE `UnrealEditor.exe`, PID **26732**, created 22:29:32, project path, no `-game` ⇒ untouched; no instance of Jonathan's in reach.

### (4) Flips (`SC-§103`) — placeholder `<hash — pending, back-referenced in commit 2 (SC-§103)>` in commit 1, the hash in commit 2
Board re-read by sha256 immediately before the write (`8b4f5003…2f730b24`, asserted in-script, ABORT on mismatch); each `- status:` line located under its own `####` header, prefix-asserted and unique in the file; line count 36095 unchanged; after `6c0562eb…9ba12989`; `git diff --stat` = `3 insertions(+), 3 deletions(-)`; placeholder ×5.
- `TASK-1270`: `built — loop 1 …` → **`done — committed <hash> · verified VERIFIED loop 1 2026-09-14 (qa/TASK-1270-verify.md, partial 1/6 observable; loop 0 VERIFY-FAILED preserved as qa/TASK-1270-verify-loop0.md) · qa-passed loop 1 (TASK-1287) · host TASK-1289 re-entry (…)`** — prior text (incl. the verifier's `verify (loop 1): VERIFIED …` tail) kept after `· was:`.
- `TASK-1287`: `done — PASS …, awaiting commit` → **`done — PASS (loop 0 + loop 1 re-gate) · committed <hash> (host TASK-1289)`** — prior text kept after `· was:`.
- `TASK-1289` (THIS row): `in-progress — …` → **`done — lane 1271 committed e1a7f5b; lane 1270 committed <hash> — acceptance (1)–(6) met across the two commits …`** — prior text kept after `· was:`.
- ⛔ No other row touched.

### (5) Staged — by pathspec from the git root, exactly 19
`git diff --cached --name-status`:
```
M	GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
A	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md
M	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1289-buildmaster.md
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a1-t00m00s-hud-refusal-slot-no-deck-notice.png
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t00m02s-hud-refusal-slot-no-deck-notice.png
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t00m10s-hud-refusal-slot-no-deck-notice.png
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a2-t01m18s-hud-refusal-slot-positive-control.png
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a3-t00m00s-hud-refusal-slot-deck-notice.png
A	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/VER-TASK-1270-a3-t01m07s-hud-refusal-slot-positive-control.png
A	GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify-loop0.md
A	GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify.md
A	GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
M	GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp
```
count = **19**. Porcelain after staging holds nothing outside the index (no UE-plugin auto-staged file; nothing to unstage). This handoff is re-added after (5)–(7) are appended and re-grepped.

### (6) LFS read-back (`SC-§68`, ⛔ never size) — `git show :<path>` per PNG; oid = sha256 of the working file = sha256 of the local object under `.git/lfs/objects/`
| PNG (`playtest-evidence/2026-09-14/`) | staged pointer `oid sha256:` | working sha256 | `.git/lfs/objects` re-hash |
|---|---|---|---|
| `VER-TASK-1270-a1-t00m00s-hud-refusal-slot-no-deck-notice.png` | `8b3b26d97da5b57b65ef5ecc66b2596ea8c4737fe598b8a3d70f4a169bf7be23` | equal | equal |
| `VER-TASK-1270-a2-t00m02s-hud-refusal-slot-no-deck-notice.png` | `52be55d6a420e88da51e2dcb691baed8344ffb1ead135a55ad8710795a299fb0` | equal | equal |
| `VER-TASK-1270-a2-t00m10s-hud-refusal-slot-no-deck-notice.png` | `7cf79619b5c4d7c67010ae3e29d787f54d22e92b2f825472a2e41e177aa72a7d` | equal | equal |
| `VER-TASK-1270-a2-t01m18s-hud-refusal-slot-positive-control.png` | `a076eda5c02825d63ede58a9aca39a8fa00267796eaa025b159a649b00209b3b` | equal | equal |
| `VER-TASK-1270-a3-t00m00s-hud-refusal-slot-deck-notice.png` | `38a43f2afb8cb6f1e9d1ccd39c9f469ca9d84a5ff58dba701ab738a303efe1b3` | equal | equal |
| `VER-TASK-1270-a3-t01m07s-hud-refusal-slot-positive-control.png` | `1d4f1908f990ba143a77bf9439abe3fcc85a1ccd94702b75efb005add2235820` | equal | equal |

Every staged blob is a 3-line pointer (`version https://git-lfs.github.com/spec/v1` / `oid sha256:…` / `size …`). The loop-0 four also equal §3 (0)(ii)'s copy-time hashes and the loop-1 two equal their `Saved/AuraVerify/` sources ((1) above) ⇒ source = target = oid for all six. The COMMIT's blobs (`git show HEAD:<path>`) are read back after the commit and reported to the orchestrator (`SC-§104` — the commit, not the index, is the claim).

### (7) Value-grep (`git show :<path>` per staged blob; patterns `eyJ[A-Za-z0-9_-]{20,}` · `sb_secret_[A-Za-z0-9_-]{10,}` · `hf_[A-Za-z0-9]{20,}`)
Positive control — three synthetic lines (`eyJ` + 24 chars · `sb_secret_` + 12 · `hf_` + 24) in a scratchpad file → combined `3`, each pattern alone `1` / `1` / `1` (the grep fires). Staged blobs, all **0 / 0 / 0**: `TASKBOARD.md` · `handoffs/TASK-1270-programmer.md` · `handoffs/TASK-1289-buildmaster.md` (re-run on the (5)–(7)-appended blob before the commit) · the six PNG pointer texts · `qa/TASK-1270-verify-loop0.md` · `qa/TASK-1270-verify.md` · `qa/TASK-1287-report.md` · `CardHandWidget.cpp` · `CardHandWidget.h` · `DeckBuilderWidget.cpp` · `DeckBuilderWidget.h` · `SiegePlayerController.cpp` · `SiegePlayerController.h` · `Tests/SiegeDeckSlotsTest.cpp`.

### (8) The commit
First line PRESCRIBED by the amendment's "1270 RE-ENTRY" clause, `<VERDICT>` = `VERIFIED`, `<hash>` = `e1a7f5b`: *TASK-1270: illegal-deck activation refused in the builder + a HUD notice at match start when the active deck is not 50 (was: silent default-deck fallback); verified VERIFIED at loop 1 (gate TASK-1287, host TASK-1289 re-entry — TASK-1271 committed e1a7f5b)* — body = the two house trailer lines. Verified on `git show --stat HEAD` (staged 19 = committed 19), ⛔ never the index. The hash is written onto the three board status lines in a TASKBOARD-only commit 2 (`SC-§103`, the §2 / `6ffc2a2` precedent) — this file carries no hash because it rides inside the commit it would name.

### DECLARED, not measured
- The committed tree (`6ffc2a2` + these 7 source files) is the tree §3 compiled (`Result: Succeeded`, DLL 22:26:44) and suited (559/559 ×2): the `Source/` diff stat is byte-for-byte §3's (`7 files changed, 657 insertions(+), 18 deletions(-)`), and HEAD has not moved since §3. Not recompiled in this leg (⛔ no compile, per dispatch).
- Suite baseline for the +3 remains DERIVED (§3 (2c)), not re-measured on binaries without 1270's diff.

### Follow-ups
1. The Aura `401` race — still unboarded (§1 follow-up 1, §2 follow-up 3, §3 follow-up 1).
2. `TASK-1288` NIT (`ClimbableTower.h` ~978/980 "ActiveClimber precedent" wording half-stale) — carried from §2.
3. Board wording drift from §3 follow-up 2 ("baseline RE-MEASURED" vs derived) — still for the manager.
4. `TASK-1287` loop-1 WARN (`IsBound` gates on any listener, not specifically the hand) — later hardening, per the gate; unchanged.
5. Observation carried from the verify (not a defect of this row): Jonathan's `deck1` still reads **51** cards, so his matches play the default deck (the new notice now says so on screen). His trim is his.
