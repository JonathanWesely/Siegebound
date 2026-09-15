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
