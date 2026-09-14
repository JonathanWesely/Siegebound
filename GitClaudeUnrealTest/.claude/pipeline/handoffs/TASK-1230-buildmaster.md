# TASK-1230 — AURA-PILOT — build-master handoff

Tracking row for the orchestrator-run chain. Legs: N1 = `TASK-787` (host `TASK-780`, `1231bc4`) · N2 = `TASK-1068` (host `TASK-1070`, `c6bb376`) · N3 = `TASK-671` (host `TASK-674`, `d8bfd23`). 🧑 confirmed "N1, N2, N3 as drafted" in Claude Code (2026-09-13). All three are COMMITTED `done` rows ⇒ the row's clean-tree clause applies: each `built` leg = confirm the running GUI editor is on HEAD's binaries. ⛔ No compile, no editor launch/close/restart, no Live Coding, no DT_Cards edit, nothing staged, nothing committed in this dispatch.

## §1 — built leg — 2026-09-14 (covers N1, N2, N3 at once: one editor, one HEAD)

All times local (`Pacific Standard Time`, UTC−07:00 in effect on 2026-09-09/13; the git `-0700` stamps and the file mtimes are on the same clock). Measured 2026-09-14 00:02 local.

**HEAD:** `e404c5c`. Git root is ONE LEVEL UP (`SC-§102`): `git status --porcelain` from the project dir prints `GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md` + `TASKBOARD.md` (the unrelated R15 lane — untouched, unstaged).

### Proof (a) — last commit touching C++ vs the module DLL's mtime

```
git log -1 --format='%h %ci' -- Source/ GitClaudeUnrealTest.Target.cs GitClaudeUnrealTestEditor.Target.cs
ea7b4d2 2026-09-09 05:02:13 -0700
```
Cross-checked from the git root with `-- GitClaudeUnrealTest/Source GitClaudeUnrealTest/Plugins '*.Build.cs' '*.Target.cs'` → the same `ea7b4d2 2026-09-09 05:02:13 -0700` (TASK-1180, the fog prose lane). `Plugins/` holds only `SiegeLlama`. Last five C++-touching commits: `ea7b4d2` 09-09 05:02 · `4a3da63` 09-09 01:57 · `60dca54` 09-08 19:10 · `61702e1` 09-08 01:35 · `42734b7` 09-07 21:03.

```
Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll   9,661,952 B   LastWriteTime 2026-09-09 13:08:08  (= 2026-09-09 20:08:08 UTC)
Binaries/Win64/UnrealEditor.modules                                LastWriteTime 2026-09-09 13:08:09
```
DLL mtime `2026-09-09 13:08:08` ≥ commit time `2026-09-09 05:02:13` — by 8 h 05 m 55 s. ✅

### Proof (b) — no uncommitted C++

```
git status --porcelain -- Source/
(empty)
```
✅

### Proof (c) — editor start time > DLL mtime

| PID | Start (local) | Command line | Class (`SC-§118`, by command line) |
|---|---|---|---|
| **8228** | **2026-09-13 20:45:04** | `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"` | **GUI editor — the drive target** |
| 13304 | 2026-09-13 23:53:52 | `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject -game -windowed -ResX=3200 -ResY=1800` | 🧑 **Jonathan's `-game` play session — NEVER driven, NEVER closed** |

Source: `Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'"` (ProcessId, CreationDate, CommandLine). Editor PID 8228 started `2026-09-13 20:45:04` > DLL mtime `2026-09-09 13:08:08` (by 4 d 07 h 36 m 56 s) ⇒ the module the GUI editor loaded IS the on-disk DLL. ✅

### Proof (d) — every DLL under `Binaries/Win64/`, flagged against the editor start

| DLL | Bytes | mtime (local) | newer than editor start 2026-09-13 20:45:04? |
|---|---|---|---|
| `UnrealEditor-GitClaudeUnrealTest.dll` | 9,661,952 | 2026-09-09 13:08:08 | no |
| `ggml-base.dll` | 772,096 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-alderlake.dll` | 1,180,672 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-cannonlake.dll` | 1,395,200 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-cascadelake.dll` | 1,381,376 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-cooperlake.dll` | 1,382,400 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-haswell.dll` | 1,184,768 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-icelake.dll` | 1,388,032 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-ivybridge.dll` | 1,073,664 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-piledriver.dll` | 1,077,248 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-sandybridge.dll` | 1,053,696 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-sapphirerapids.dll` | 1,659,904 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-skylakex.dll` | 1,389,056 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-sse42.dll` | 876,544 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-x64.dll` | 869,376 | 2026-08-02 14:18:25 | no |
| `ggml-cpu-zen4.dll` | 1,389,056 | 2026-08-02 14:18:25 | no |
| `ggml-vulkan.dll` | 52,332,032 | 2026-08-02 14:18:25 | no |
| `ggml.dll` | 86,016 | 2026-08-02 14:18:25 | no |
| `libomp140.x86_64.dll` | 661,856 | 2026-08-02 14:18:25 | no |
| `llama.dll` | 2,795,008 | 2026-08-02 14:18:25 | no |
| `tbb12.dll` | 342,456 | 2026-06-24 18:19:45 | no |
| `tbbmalloc.dll` | 117,176 | 2026-06-24 18:19:45 | no |

Recursive filter for `*.dll,*.modules,*.target` with `LastWriteTime > 2026-09-13 20:45:04` under `Binaries/Win64/`: **zero hits**. ✅ (The `ggml-*`/`llama`/`tbb*` DLLs are the SiegeLlama plugin's runtime, not project modules; listed because the ask was "every module DLL".)

**Built-leg conclusion (facts only):** GUI editor PID 8228 is running the on-disk `UnrealEditor-GitClaudeUnrealTest.dll` built after the last C++ commit, with no uncommitted C++ ⇒ HEAD `e404c5c`'s binaries. ⛔ Not compiled, not relaunched, in this dispatch. The three nominee hashes (`1231bc4`, `c6bb376`, `d8bfd23`) are all ancestors of `e404c5c` on `main`.

### Aura reachability (pipeline side, `unreal_inspector`, read-only)

- `get_headless_status` → `status: editor_connected`
- `get_unreal_context` → `open_windows: ["GitClaudeUnrealTest - Unreal Editor"]`, `level: "PersistentLevel"`, `current_level_path: "/Game/Maps/L_Arena.L_Arena"`

The context reports ONE window, titled as the editor, with `L_Arena` loaded as the level-editor map. ⚠️ Aura's context does NOT expose the PID or command line — a `-game` instance carries the same window title (`SC-§118`). Corroboration that it is the GUI editor: the unreal-mcp (Epic EditorToolset) bridge on the same machine answered `IsPIERunning → false` and `GetOpenAssets → []`, both editor-only surfaces, and the `-game` instance (PID 13304) is a running match, not a level-editor map. The verifier must still identify by command line before its first PIE call (`VER-§2` cl. 4) — this handoff's PID table is that identification as of 2026-09-14 00:02.

### PIE / dirty state (`VER-§3` cl. 4 — report only)

- `EditorAppToolset.IsPIERunning` → `false` (GUI editor not in PIE)
- `EditorAppToolset.GetOpenAssets` → `[]` (no asset editors open)
- `AssetTools.is_dirty("/Game/Data/DT_Cards")` → `false`
- `AssetTools.is_dirty("/Game/Maps/L_Arena")` → `false`

No PIE, no dirty `DT_Cards`, no dirty `L_Arena`. (No tool enumerates ALL dirty packages; these two are the assets the pilot touches.)

### Status flip

`TASK-1230` `status:` `backlog` → `in-progress — built leg ✅ 2026-09-14 (…) awaiting 🧑 VER-§3 go`. Rows `787`/`1068`/`671` untouched (eligibility doubt (c): the built leg flips nothing there).

## §2 — Probe 5 pre-read (report only — ⛔ nothing edited)

**Revert baseline** (oid-vs-sha256, never size):
```
git show HEAD:GitClaudeUnrealTest/Content/Data/DT_Cards.uasset
version https://git-lfs.github.com/spec/v1
oid sha256:aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b
size 46510
sha256sum Content/Data/DT_Cards.uasset
aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b
git status --porcelain -- Content/Data/DT_Cards.uasset   → (empty)
```
Working file == HEAD oid today. (`git show HEAD:Content/Data/DT_Cards.uasset` from the project dir FAILS — `fatal: path 'GitClaudeUnrealTest/Content/Data/DT_Cards.uasset' exists, but not 'Content/Data/DT_Cards.uasset'` — the root-anchored spelling above is the one that answers, `SC-§102`.)

**Live in-editor value (read-only `get_rows`, the same lane TASK-031/061 wrote through):** `DataTableTools.get_rows(/Game/Data/DT_Cards.DT_Cards, ["WatchTower"])` → `cost: 30`, `maxCopies: 3`, `hP: 250`, `deckCount: 0`, `cardType: "Building"`. `Docs/Data/cards.csv` line 32: `WatchTower,Watch Tower,Building,30,3,250,…,DeckCount 0`. Editor and CSV agree on `Cost 30`.

**The edit tool:** unreal-mcp `editor_toolset.toolsets.data_table.DataTableTools.set_rows` — args `data_table: {refPath: "/Game/Data/DT_Cards.DT_Cards"}`, `values: '{"WatchTower": {"cost": 5000}}'` (camelCase property names; only named properties change). Read-back: `DataTableTools.get_rows` (same args shape, `row_names: ["WatchTower"]`). Save: `AssetTools.save_assets(["/Game/Data/DT_Cards"])` (targeted, NOT save-all — TASK-061's lane). Dirty check: `AssetTools.is_dirty("/Game/Data/DT_Cards")`. Precedent: `handoffs/TASK-031.md` (add_rows + set_rows in-place, GUID + CSV import-source linkage preserved) and `handoffs/TASK-061.md` (set_rows in 4 batches, targeted save, `is_dirty` false after). Aura-side equivalents exist (`mcp__unreal_editor__add_or_replace_rows_in_data_table`, census line 113) but are NOT in the verifier's grant and are not the lane the row names.

**"TASK-731 read-back" citation:** `handoffs/TASK-731-buildmaster.md` does not contain `PlatformHeightUU`; the `1200` CDO read-back lives in `handoffs/TASK-728-integration-buildmaster.md:134` (`PlatformHeightUU (CDO) = 1200`) and `handoffs/TASK-728-artist.md:233`. The number is right; the citation points one handoff off.

**Two Probe 5 facts to weigh (report only):**
1. `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` `[/Script/UnrealEd.EditorLoadingSavingSettings]` (line 81) carries `AutoReimportDirectorySettings=(SourceDirectory="/Game/",…)` and NO `bMonitorContentDirectories` override (engine default). The two `EditorLoadingSavingSettings.AutoReimport=True/False` lines (2556, 4169) are `[DetailCategories]`/`[DetailCategoriesAdvanced]` UI-expansion keys, not the setting. `TASK-061` observed `LogCSVImportFactory: Imported DataTable 'DT_Cards'` auto-reimports firing from `cards.csv` during an edit window. A `set_rows` edit does not touch `cards.csv`, so nothing should overwrite `Cost 5000` — but if `cards.csv` is touched during the probe, the auto-reimport would restore `30` silently.
2. The row's revert plan (`git checkout -- Content/Data/DT_Cards.uasset` + GUI-editor graceful relaunch before the known-good rerun) is an editor-lifecycle action on PID 8228 only; PID 13304 (`-game`) is out of scope for every step (`SC-§118`).

## §3 — leg N1 — 5c commit — 2026-09-14

**Scope:** the 5c leg for a COMMITTED `done` row (`TASK-787`, host `1231bc4`; clean-tree clause on the `TASK-1230` row). ⛔ No engine work, no compile, no editor touch (GUI editor PID 8228 left as found; PID 13304 `-game` never touched, `SC-§118`). ⛔ TASKBOARD.md not edited this dispatch (manager writing concurrently) — status flip and the pilot row are the manager's to write.

**Commit:** `51ac7d5` — `TASK-1230 leg N1 — TASK-787 runtime verification (advisory, VER-§6 pilot): VERIFIED partial 1/3 — qa/TASK-787-verify.md + 4 promoted VER-TASK-787 stills (host 1231bc4; evidence host = this leg)`. Not pushed (main 6 ahead of `origin/main`, 0 behind).

`git show --stat HEAD` (the COMMIT, never the index):
```
51ac7d5
 .../VER-TASK-787-a2-t01m25s-ground-contact-at-ladder-foot.png |  3 ++
 .../VER-TASK-787-a2-t01m27s-climb-started-z252.png            |  3 ++
 .../VER-TASK-787-a2-t01m29s-climb-ended-on-ground-z98.png     |  3 ++
 .../VER-TASK-787-a3-t03m29s-ghost-pawn-at-approach-point.png  |  3 ++
 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-787-verify.md    | 35 ++++++
 5 files changed, 47 insertions(+)
```
(all four PNGs under `GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/`; each is a 3-line LFS pointer — `*.png` is an LFS pattern in the root `.gitattributes`, line 4; this leg is the named evidence host per `VER-§4` cl. 4 / `FR-§6`.)

**Staging discipline (`SC-§102`, pathspecs from the git root one level up):** `git status --porcelain` before = `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md`, `?? qa/TASK-787-verify.md`. Staged by pathspec: the report + the four PNGs only. After staging the porcelain showed exactly those 5 as `A` with the two `M` files and this handoff still unstaged; after the commit the tree holds only `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md` (other lanes' dirt, left out). The UE Git plugin auto-staged nothing during the window.

**Copy proofs (`VER-§4` cl. 1 copy-out — COPY, never move; sources still present under `Saved/AuraVerify/`, machine-local, gitignored). sha256 measured on source and destination, equal in all four; the LFS pointer oid in the index/commit equals the same sha256 (oid-vs-sha256, never size):**
| source (`Saved/AuraVerify/`) | promoted as (`playtest-evidence/2026-09-14/`) | sha256 = LFS oid | bytes |
|---|---|---|---|
| `pie_game_c10_side_t85.64s_f731330.png` | `VER-TASK-787-a2-t01m25s-ground-contact-at-ladder-foot.png` | `beb1ceff56d51eba6522ad65dbe048d41ccb33c4f61964969f3d005d6368a319` | 1152195 |
| `pie_game_c11_side_t87.38s_f731352.png` | `VER-TASK-787-a2-t01m27s-climb-started-z252.png` | `46de27d29afe724c340c29db02cc68a8161b42b02a47d6bde0a23ebf2c3c8338` | 1068366 |
| `pie_game_c12_side_t89.17s_f731378.png` | `VER-TASK-787-a2-t01m29s-climb-ended-on-ground-z98.png` | `92c9ae480890465878c2db3f94435b82ac89fc3f43cac6ed638b419614a25009` | 983915 |
| `pie_game_c14_t209.33s_f734725.png` | `VER-TASK-787-a3-t03m29s-ghost-pawn-at-approach-point.png` | `8b0ac66612111339fd2561ef89f8478f2d399b87d561906926d9e281ed557095` | 1343101 |
No source frame was missing; nothing substituted. The mapping is the one the report's `## Evidence (promoted)` section lists, taken from the report.

**Draft row for `AURA-PHASE0.md` §Pilot (NOT written there yet — the closing post writes all three rows at once):**
| case | Aura verdict | 🧑 his playtest answer | match Y/N | credit | wall time |
|---|---|---|---|---|---|
| N1 / `TASK-787` | `VERIFIED (advisory) partial 1/3` (row 1 pass — Z 98.15→252.69, speed 350.0 = `LadderClimbSpeedUU`; rows 2+3 `unobs` — hold released at 1.5 s ⇒ designed `H-3` abort, then the hero died and attempt 3 drove the ghost) | *"Walking my hero into the tower's ladder foot climbs it onto the deck about 1200 uu up, and I can climb it again straight after."* | pending manager adjudication | not visible (no credit field in any tool reply) | ≈35 min |

**Follow-ups observed in the report (report only, for the manager):** the sandbox menu button is not actuable by `ui_perform` (`OnClicked` never fired, 3 shapes); `ActiveClimber` is unreflected so the slot-leak line (row 3) has no Aura observable; hero-side climb lines are `Verbose` and the verifier has no console lane; the recipe's capsule half-height `88` is wrong for this hero (reads 96, deck Z 1296 not 1288).

## §4 — leg N3 — 5c commit — 2026-09-14

**Scope:** the 5c leg for a COMMITTED `done` row (`TASK-671`, host `d8bfd23` — TASK-674, the deck bar; clean-tree clause on the `TASK-1230` row). Same shape as §3. ⛔ No engine work, no compile, no editor touch (GUI editor PID 8228 left as found; PID 13304 `-game` never touched, `SC-§118`). ⛔ TASKBOARD.md not edited this dispatch — status flip and the pilot row are the manager's to write.

**Commit:** `e398c13` — `TASK-1230 leg N3 — TASK-671 runtime verification (advisory, VER-§6 pilot): UNOBSERVABLE — no verifier input reaches the main-menu buttons (ui_perform clicks never fire OnClicked; 8× Tab moves no focus, "NOTHING BINDS IT") — qa/TASK-671-verify.md + 1 promoted still (host d8bfd23; evidence host = this leg)`. Not pushed (main 7 ahead of `origin/main`, 0 behind).

`git show --stat HEAD` (the COMMIT, never the index):
```
e398c13
 .../VER-TASK-671-t00m50s-main-menu-after-8-tabs-no-focus.png |  3 +++
 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-671-verify.md   | 28 ++++++++++++++++++++++
 2 files changed, 31 insertions(+)
```
(the PNG under `GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/`, a 3-line LFS pointer — `filter: lfs` per `git check-attr`; this leg is the named evidence host per `VER-§4` cl. 4 / `FR-§6`.)

**Staging discipline (`SC-§102`, pathspecs from the git root one level up):** `git status --porcelain` before = `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md`, `?? qa/TASK-671-verify.md`. Staged by pathspec: the report + the one PNG only. After staging, `git diff --cached --name-status` listed exactly those 2 as `A`, the two `M` files and this handoff still unstaged; after the commit the tree holds only `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md` (other lanes' dirt, left out). The UE Git plugin auto-staged nothing during the window; nothing needed unstaging.

**Copy proof (`VER-§4` cl. 1 copy-out — COPY, never move; the source is still present under `Saved/AuraVerify/`, machine-local, gitignored). sha256 measured on source and destination, equal; the LFS pointer oid in the COMMIT (`git show HEAD:…png`) equals the same sha256 (oid-vs-sha256, never size):**
| source (`Saved/AuraVerify/`) | promoted as (`playtest-evidence/2026-09-14/`) | sha256 = LFS oid | bytes |
|---|---|---|---|
| `pie_composited_c21_t50.84s_f814789.png` | `VER-TASK-671-t00m50s-main-menu-after-8-tabs-no-focus.png` | `032659536b9e51b03c86e6cd6428f15608943c4df2a6fb46720b661f66c45c6a` | 375756 |
Single attempt ⇒ no `-a<N>` suffix (`VER-§4` cl. 2). The mapping is the one the report's `## Evidence (promoted)` section lists, taken from the report. The video (`Saved/AuraVerify/rec_1789371561640696100_3/recording.h264`) was NOT promoted — never is.

**Draft row for `AURA-PHASE0.md` §Pilot (NOT written there yet — the closing post writes all three rows at once):**
| case | Aura verdict | 🧑 his playtest answer | match Y/N | credit | wall time |
|---|---|---|---|---|---|
| N3 / `TASK-671` | `UNOBSERVABLE (advisory)` (both rows `unobs` for ONE cause — the deck builder opens only from the main menu's "Deck Builder" button and no verifier input reached it: 8× `simulate_key_press Tab` → `binding_found: false`, every `ui_snapshot` `focused: false`; Enter never sent; no `UDeckBuilderWidget` constructed) | *"Opening the deck builder shows ten slots deck1–deck10 with the orange rim on deck1 only."* (manager's redraft; his confirmation pending) | `n/a — blind spot: the menu needs a human click` | not visible (no credit field in any tool reply) | ≈12 min (PIE ≈56 s) |

**Two lane facts from the report for §Pilot cl. 4:**
- (a) ENHANCED INPUT / UMG: on `L_MainMenu` the player has NO input mapping context (`applied_mapping_contexts: []` on every press; menu pawn `DefaultPawn_0`), and Aura's `ui_perform` (three click shapes, traced in `qa/TASK-787-verify.md`, `OnClicked` never fired) / `simulate_key_press` (delivered to the player controller, not as a Slate `FKeyEvent`) reached neither our `KBD-§` mappings nor Slate focus navigation. ⇒ every UMG-button-gated flow (deck builder, sandbox, settings, login…) is currently verifier-blind; a human click is the only known route.
- (b) VRAM co-run hazard: the PIE co-ran with Jonathan's `-game` session (PID 13304, 3200×1800). During PIE `is_pie_active` read `gpu.current_usage_mb: 3830` vs `budget_mb: 3674` (`available_mb: -1`), and the promoted frame carries the engine's red "Video memory has been exhausted (… MB over budget)" banner across the menu. Before PIE: 2930 / 4418; after stop: 2867 / 4404. Every co-run leg is exposed to this; whether it would have affected a rendered observable is unknown (the builder never opened).

**Follow-ups observed in the report (report only, for the manager):** the Tab-count → button mapping is unmeasured (0 of 8 presses moved focus), so N2's Route A (keyboard to "Sandbox (No Bot)") is NOT proven ⇒ Route B (direct `load_level L_Arena`) per R-SANDBOX; `simulate_button_press` gamepad Accept, `ui_perform type`, and `inject_input_action` were not tried (no focus to fire at / outside the recipe / no mapping context to inject into); attempts 2 and 3 of the budget unspent by the recipe's step c. Editor left on `L_MainMenu` (level editor, PIE stopped), `discarded_unsaved: false`, nothing saved.

## §5 — leg N2 — 5c commit — 2026-09-14

**Scope:** the 5c leg for a COMMITTED `done` row (`TASK-1068`, host `c6bb376` — TASK-1070, the fog-visual caller; clean-tree clause on the `TASK-1230` row). Same shape as §3/§4. ⛔ No compile, no editor lifecycle action (GUI editor PID 8228 the only `UnrealEditor.exe` running at 2026-09-14 12:05 local — `Get-CimInstance Win32_Process`: PID 8228, CreationDate 2026-09-13 20:45:04, the `.uproject`-only command line; Jonathan's `-game` PID 13304 had exited). ⛔ TASKBOARD.md not edited this dispatch — status flip and the pilot row are the manager's to write.

**Commit:** `72292a3` — `TASK-1230 leg N2 — TASK-1068 runtime verification (advisory, VER-§6 pilot): VERIFIED 2/2 — Fog visual spawned Z=7000 scale (640,360,260), timer 299.06 s, destroyed 300.1 s later — qa/TASK-1068-verify.md + 4 promoted VER-TASK-1068 stills (host c6bb376; evidence host = this leg)`. Not pushed (main 8 ahead of `origin/main`, 0 behind).

`git show --stat HEAD` (the COMMIT, never the index):
```
72292a3
 ...TASK-1068-t00m14s-before-cast-no-fog-visual.png |  3 ++
 ...1068-t01m04s-fog-visual-spawned-player-view.png |  3 ++
 ...8-t05m33s-fog-visual-still-up-before-expiry.png |  3 ++
 ...8-t06m22s-fog-visual-destroyed-after-expiry.png |  3 ++
 .../.claude/pipeline/qa/TASK-1068-verify.md        | 35 ++++++++++++++++++++++
 5 files changed, 47 insertions(+)
```
(all four PNGs under `GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/`; each a 3-line LFS pointer — `git check-attr filter` → `lfs`; this leg is the named evidence host per `VER-§4` cl. 4 / `FR-§6`.)

**Staging discipline (`SC-§102`, pathspecs from the git root one level up):** `git status --porcelain` before = `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md`, `?? qa/TASK-1068-verify.md` (+ the four `??` PNGs once copied). Staged by pathspec: the report + the four PNGs only. After staging, `git diff --cached --name-status` listed exactly those 5 as `A`, the two `M` files and this handoff still unstaged; after the commit the tree held only `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md` (other lanes' dirt, left out). The UE Git plugin auto-staged nothing during the window; nothing needed unstaging.

**Copy proofs (`VER-§4` cl. 1 copy-out — COPY, never move; all four sources still present under `Saved/AuraVerify/`, machine-local, gitignored). sha256 measured on source and destination, equal in all four; the LFS pointer oid in the COMMIT (`git show HEAD:…png`) equals the same sha256 (oid-vs-sha256, never size):**
| source (`Saved/AuraVerify/`) | promoted as (`playtest-evidence/2026-09-14/`) | sha256 = LFS oid | bytes |
|---|---|---|---|
| `pie_composited_c22_t14.97s_f2184903.png` | `VER-TASK-1068-t00m14s-before-cast-no-fog-visual.png` | `cc5df4f20d920612de4277e1c5de484df89b8c79d64bcf71df8d272d4b8bc8a4` | 933919 |
| `pie_composited_c23_t64.21s_f2187850.png` | `VER-TASK-1068-t01m04s-fog-visual-spawned-player-view.png` | `699223ae1e865f057e47b8d035bf3936cdba58f6181e8d572e9da03c398c66d0` | 833148 |
| `pie_composited_c25_t333.61s_f2203991.png` | `VER-TASK-1068-t05m33s-fog-visual-still-up-before-expiry.png` | `453c6e53779f713da7c3865e747bc62639ede7bcd9167080270e4506d450f80e` | 649442 |
| `pie_composited_c26_t382.00s_f2206890.png` | `VER-TASK-1068-t06m22s-fog-visual-destroyed-after-expiry.png` | `799685afe51264db65081ff2e140caf7c71fcadd2437e2c7273cd84e62f7a17f` | 1083408 |
Single attempt ⇒ no `-a<N>` suffix (`VER-§4` cl. 2). The mapping is the one the report's `## Evidence (promoted)` section lists, taken from the report. Not promoted, per the report: the outside scene-capture `pie_game_c24_three_quarter_t64.29s_f2187851.png` (quoted only) and the video `Saved/AuraVerify/rec_1789412224892939300_4/recording.h264` — never is.

**Draft row for `AURA-PHASE0.md` §Pilot (NOT written there yet — the closing post writes all rows at once):**
| case | Aura verdict | 🧑 his playtest answer | match Y/N | credit | wall time |
|---|---|---|---|---|---|
| N2 / `TASK-1068` (host 1070, `c6bb376`) | `VERIFIED (advisory) 2/2` (row 1 — `BP_SiegeFog_C` 0→1 within 0.93 s of `IA_Card1`, `RelativeLocation.Z` 7000, `RelativeScale3D` (640, 360, 260), `FogActiveUntilTimeSeconds` 363.196615 at world 64.139 s ⇒ 299.06 s ∈ (295, 300], gold 73→24; row 2 — `Fog VISUAL destroyed` 300.106 s after the spawn line, census 0, `FogVisualActor` = `None`) | *"Playing the Fog card raises one battlefield-wide fog visual immediately and the fog timer reads five minutes."* | pending manager adjudication (observed: immediate spawn, 299.06 s timer, destroyed at +300.1 s) | not visible (no credit field in any tool reply) | ≈30 min (PIE 7 min 03 s) |

**Lane facts from the report for §Pilot cl. 4:**
- (a) ENHANCED INPUT: `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` reached OUR `IMC_Hero` card mapping (`value_type: Boolean`; the slot-1 card was played, no targeting mode) — our own action, not a default binding. `IA_DiscardAll` untested (Fog was in the opening hand, slots 1/5/6). Route B (direct `load_level L_Arena`, vs-bot) — no menu, no keyboard/pointer route needed.
- (b) The ~60 s per-`run_verification_sequence` wait cap made the 300 s hold FIVE calls (checkpoints t=03m14s, 04m22s, 05m33s, then the post-expiry read at 06m22s).
- (c) A red "Video memory has been exhausted (N MB over budget)" banner sat on EVERY frame — including the BEFORE frame at 168.906 MB over — with NO `-game` co-run this leg (PID 13304 exited before dispatch) and `is_pie_active` reading DXGI UNDER budget (2846/7123 before PIE, 3946/7123 after). The in-game banner and DXGI's budget disagree, or the PIE window crossed it transiently — unmeasured (report H4). No quoted number was read off pixels, so it affected no verdict.
- (d) The match ended in Defeat (winner Red) at t≈04m24s with the hero alive at spawn, HP 200 — cause unobserved (report H2: presumably the Blue castle fell to the bot's units while the hero was never driven). The fog visual OUTLIVED the match end by 99.4 s — the destroy fired on the pinned expiry (timer path), NOT on the match-end path. Both hypotheses are the manager's: whether a match end SHOULD clear the fog is a design question, not a finding against the row.

**Follow-ups observed in the report (report only, for the manager):** `bInTargetingMode` is a bare `bool` with no `UPROPERTY` (`SiegePlayerController.h:3292`) ⇒ unreadable by any Aura tool (the instant-cast claim rests on the 0.93 s spawn latency, the slot-1 card leaving the hand and the −50 gold); the outside scene-capture on `BP_SiegeFog0` from 70,816 uu shows NO fog (the visual does not render from outside its box in a scene capture — H1; listed so nobody re-takes it); the Play Again exit (the third destroy path) was not exercised; the visual's LOOK is not judged (his eye's call, `VER-§0` cl. 3). Editor left on `L_Arena` (level editor, PIE stopped), nothing saved, `discarded_unsaved: false`.

## §6 — Probe 5 — broken state ARMED — 2026-09-14

**Scope (`R-PROBE5`, ruling 5):** the DELIBERATELY BROKEN working-tree state for the verifier's failing rerun — a DATA-only break (⛔ no C++, no compile, no Live Coding): `DT_Cards` row `Fog` `Cost` **50 → 5000** (> `MaxGold` 999 ⇒ unpayable in every mode). Executed on the GUI editor PID 8228 (the only `UnrealEditor.exe`, identified by command line, `SC-§118`), level editor on `L_Arena`, `EditorAppToolset.IsPIERunning` → `false` before the edit. ⛔ NOT reverted in this dispatch — the revert (`git checkout -- Content/Data/DT_Cards.uasset` + graceful GUI-editor relaunch + `VER-§3` re-announce) is the NEXT build-master dispatch, after the verifier's broken rerun (report `qa/TASK-1230-verify.md`, evidence `VER-TASK-1230-…`, budget = TASK-1230's own 3).

**Baseline BEFORE the break (12:05 local):**
```
git status --porcelain -- GitClaudeUnrealTest/Content/Data/DT_Cards.uasset   -> (empty)
git show HEAD:GitClaudeUnrealTest/Content/Data/DT_Cards.uasset
  oid sha256:aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b   size 46510
sha256sum Content/Data/DT_Cards.uasset
  aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b   (== HEAD oid)
DataTableTools.get_rows(/Game/Data/DT_Cards.DT_Cards, ["Fog"])   -> cost: 50 (cardType Spell, maxCopies 2, spellEffect FogCover, spellDelivery Auto, effectDuration 300, deckCount 0)
AssetTools.is_dirty(/Game/Data/DT_Cards)   -> false
```
HEAD at the time of the break = `72292a3` (§5); the DT's blob is unchanged since `aa2c5bc1…dedf5b` (§2's baseline, same oid).

**The break (12:09 local):**
```
DataTableTools.set_rows(/Game/Data/DT_Cards.DT_Cards, values = {"Fog": {"cost": 5000}})   -> null (no error)
DataTableTools.get_rows(["Fog","WatchTower"])   -> Fog.cost: 5000 · WatchTower.cost: 30 (untouched — the N1-shaped break is RETIRED, R-PROBE5)
AssetTools.is_dirty(/Game/Data/DT_Cards)   -> true
AssetTools.save_assets(["/Game/Data/DT_Cards"])   -> true   (TARGETED — this one asset, never save-all; the never-save law covers L_Arena)
AssetTools.is_dirty(/Game/Data/DT_Cards)   -> false      AssetTools.is_dirty(/Game/Maps/L_Arena) -> false
DataTableTools.get_rows(["Fog"]) after the save   -> cost: 5000
```

**On-disk / git state AFTER (12:09:23 local, the file's mtime):**
```
sha256sum Content/Data/DT_Cards.uasset
  1bd502db27c87f00a981385b4b06b40e45cc78bad1cd79ba8f3a8a35352b36a8   != HEAD oid aa2c5bc1…dedf5b
ls -l   -> 46510 bytes   ⚠️ IDENTICAL byte count to the committed blob — size would have reported "unchanged"; oid-vs-sha256 is the only proof that works here.
git status --porcelain (git root)
   M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
   M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
   M GitClaudeUnrealTest/Content/Data/DT_Cards.uasset        <- leading SPACE: worktree-modified, NOT staged
  ?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1230-buildmaster.md
git diff --cached --name-status   -> (empty — the index is clean of it; the UE Git plugin auto-staged nothing, no `git restore --staged` was needed)
git status --porcelain -- GitClaudeUnrealTest/Docs/Data/cards.csv   -> (empty); mtime 2026-09-04 19:13:59 — ⛔ NOT touched, so no auto-reimport can silently restore 50 (§2 fact 1).
```

**Revert recipe for the next dispatch (unchanged from §2 / R-PROBE5):** `git checkout -- GitClaudeUnrealTest/Content/Data/DT_Cards.uasset` from the git root (`SC-§102`), proven by `git status --porcelain -- …DT_Cards.uasset` empty AND `sha256(working) == aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b` (⛔ never size — see the 46510 coincidence above); the editor still holds `cost 5000` in memory ⇒ graceful GUI-editor relaunch of PID 8228 (identify by command line, never a `-game` instance) BEFORE the known-good rerun; then the orchestrator re-announces per `VER-§3`. ⛔ Never stage, never commit the mutated DT.

## §7 — Probe 5 — revert + relaunch — 2026-09-14

**Scope (`R-PROBE5`, the dispatch after the verifier's broken rerun — `qa/TASK-1230-verify.md` reads `Verdict: VERIFY-FAILED (advisory — VER-§6 pilot)`):** (A) revert the deliberate `DT_Cards` break, (B) graceful GUI-editor relaunch so the editor no longer holds `Fog Cost 5000` in memory. ⛔ No C++, no compile, no Live Coding. ⛔ TASKBOARD.md not edited — the status flip and the pilot row are the manager's to write.

**(A) Revert — proofs (git root one level up, `SC-§102`; oid-vs-sha256, ⛔ never size):**
```
BEFORE (12:21 local)
git status --porcelain -- GitClaudeUnrealTest/Content/Data/DT_Cards.uasset   ->  " M"   (worktree-modified, NOT staged)
sha256(working)  1bd502db27c87f00a981385b4b06b40e45cc78bad1cd79ba8f3a8a35352b36a8   size 46510
git show HEAD:GitClaudeUnrealTest/Content/Data/DT_Cards.uasset
  oid sha256:aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b   size 46510
git diff --cached --name-status   -> (empty — index clean of it; the UE Git plugin auto-staged nothing)

git checkout -- GitClaudeUnrealTest/Content/Data/DT_Cards.uasset     rc=0  (LFS smudge ran: the first bytes are the .uasset magic C1 83 2A 9E, NOT a 3-line pointer; no `git lfs checkout` needed)

AFTER (12:21:24 local, the file's new mtime)
git status --porcelain -- GitClaudeUnrealTest/Content/Data/DT_Cards.uasset   -> (empty)
sha256(working)  aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b   size 46510   == HEAD oid
git status --porcelain -- GitClaudeUnrealTest/Docs/Data/cards.csv   -> (empty)   ⛔ untouched
git status --porcelain (git root) -> M CONVENTIONS.md · M TASKBOARD.md · ?? handoffs/TASK-1230-buildmaster.md · ?? qa/TASK-1230-verify.md   (DT_Cards gone from the list)
```
⚠️ The mutated blob (`1bd502db…`) and the committed blob (`aa2c5bc1…`) were BOTH 46510 bytes. A size check would have reported the break "unchanged" in both directions; only the sha256/oid comparison proved the break existed and then proved it was gone. (`SHIP-§` LFS law: oid-vs-sha256, never size — confirmed again.)

**(B) Close + relaunch record (`SC-§118`, GUI editor only, identified by COMMAND LINE):**
- Census before: `Get-CimInstance Win32_Process` → exactly ONE `UnrealEditor.exe`: PID **8228**, CreationDate 2026-09-13 20:45:04, command line `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "../../../../../../GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"`. **No `-game` instance existed** (nothing of Jonathan's to keep clear of).
- Guards before the close (unreal-mcp): `EditorAppToolset.IsPIERunning` → `false`; `AssetTools.is_dirty(/Game/Data/DT_Cards)` → `false`; `AssetTools.is_dirty(/Game/Maps/L_Arena)` → `false`; `DataTableTools.get_rows(["Fog","WatchTower"])` → **`Fog.cost: 5000`** in memory (disk already `50`) — the stale-memory state that makes the relaunch mandatory.
- Lane used: the **remote-exec graceful quit lane** (Epic's reference client `Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py`, multicast 239.0.0.1:6766, script in the session scratchpad `quit_editor_remote.py`). Node census = 1 (`node_id 4E9671924B8D628393AF27A2D44FAF59`, `project_name GitClaudeUnrealTest`, machine JONATHANWESELY); identity proven in-process before the quit: `os.getpid()` → **8228**, `DIRTY_COUNT 0` (dirty content + map packages). Then `unreal.SystemLibrary.quit_editor()` → `success: True` at 12:22:31. ⛔ No kill, no `Stop-Process`, no save.
- Exit proof: process census polled every 2 s → **no `UnrealEditor*` process at 12:22:40**; TCP :8000 listener count 0.
- Relaunch: `Start-Process` of the same exe with the SAME argument (the relative `.uproject` path, working directory `Engine/Binaries/Win64`, ⛔ no `-game`) → **PID 6136**, CreationDate **2026-09-14 12:22:56**, command line identical in shape to the old one (quoted above). unreal-mcp :8000 listening at 12:23:08.
- Read-backs on PID 6136: Aura `unreal_inspector.get_headless_status` → **`editor_connected`**; `get_unreal_context` → `current_level_path: /Game/Maps/L_Arena.L_Arena` (the editor reopened on `L_Arena` — fine); unreal-mcp `DataTableTools.get_rows(["Fog","WatchTower"])` → **`Fog.cost: 50`**, `WatchTower.cost: 30` (Fog: Spell, maxCopies 2, spellEffect FogCover, effectDuration 300 — the committed row); `AssetTools.is_dirty(/Game/Data/DT_Cards)` → `false`. ⇒ the reload took the reverted data; the in-memory 5000 is gone.
- Editor left UP (PID 6136), level editor on `L_Arena`, no PIE, nothing dirty, nothing saved. The orchestrator re-announces per `VER-§3` before any known-good drive.

## §8 — Probe 5 — 5c commit — 2026-09-14

**Commit:** `d3ade81` — `TASK-1230 Probe 5 — the deliberately broken rerun of TASK-1068 (advisory, VER-§6 pilot): VERIFY-FAILED exhibited — DT_Cards Fog Cost 50→5000 in-editor ⇒ press refused ("hand slot 0 ('Fog') refused — cost 5000, gold 88", HUD "Not enough gold"), no BP_SiegeFog_C spawned, gold unspent; break reverted by oid-vs-sha256 (file size was IDENTICAL) + GUI editor relaunched — qa/TASK-1230-verify.md + 4 promoted VER-TASK-1230-p5 stills (evidence host = this leg)`. Not pushed (main 9 ahead of `origin/main`, 0 behind).

`git show --stat HEAD` (the COMMIT, never the index):
```
d3ade813ad791002237780f55eeea9bfed7a6a93
 ...-1230-p5-t00m13s-before-press-no-fog-visual.png |  3 ++
 ...-p5-t01m39s-fog-press-refused-no-fog-visual.png |  3 ++
 ...230-p5-t01m49s-plus-10s-still-no-fog-visual.png |  3 ++
 ...-TASK-1230-p5-t02m29s-not-enough-gold-toast.png |  3 ++
 .../.claude/pipeline/qa/TASK-1230-verify.md        | 33 ++++++++++++++++++++++
 5 files changed, 45 insertions(+)
```
`git show --name-only HEAD | grep -c DT_Cards` → **0** — ⛔ `DT_Cards` is NOT in the commit (it was reverted first; its porcelain was empty at staging time). All four PNGs under `GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/`, each a 3-line LFS pointer (`git check-attr filter` → `lfs`); this leg is the named evidence host per `VER-§4` cl. 4 / `FR-§6`.

**Staging discipline (`SC-§102`, pathspecs from the git root one level up):** porcelain before = `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md`, `?? qa/TASK-1230-verify.md` (+ the four `??` PNGs once copied). Staged by pathspec: the report + the four PNGs only. After staging, `git diff --cached --name-status` listed exactly those 5 as `A`; the two `M` files and this handoff stayed unstaged; after the commit the tree holds only `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md`. The UE Git plugin auto-staged nothing across the editor boot (index checked empty after PID 6136 came up and again before `git add`).

**Copy proofs (`VER-§4` cl. 1 copy-out — COPY, never move; all four sources still present under `Saved/AuraVerify/`, machine-local, gitignored). sha256 measured on source and destination, equal in all four; the LFS pointer oid in the COMMIT (`git show HEAD:…png`) equals the same sha256 (oid-vs-sha256, never size):**
| source (`Saved/AuraVerify/`) | promoted as (`playtest-evidence/2026-09-14/`) | sha256 = LFS oid | bytes |
|---|---|---|---|
| `pie_composited_c27_t13.31s_f2245940.png` | `VER-TASK-1230-p5-t00m13s-before-press-no-fog-visual.png` | `7df9fcd1d954c507181d33c8d0ec9b0928c282802f7778190cce7349f8f3446a` | 1149659 |
| `pie_composited_c28_t99.86s_f2251115.png` | `VER-TASK-1230-p5-t01m39s-fog-press-refused-no-fog-visual.png` | `75c7ef22e6e9c131293bce51b16b2a4df5c0a10333ad3b3f3d4a83aa388492a0` | 1150410 |
| `pie_composited_c29_t109.02s_f2251658.png` | `VER-TASK-1230-p5-t01m49s-plus-10s-still-no-fog-visual.png` | `719e47869ee4ecad1de2500592ae888dd8b1bbb3c372f957c3b2e1b91a7bc4da` | 1148905 |
| `pie_composited_c30_t149.80s_f2254093.png` | `VER-TASK-1230-p5-t02m29s-not-enough-gold-toast.png` | `124320839c58f388d3e53a291389f17da415f79ae0873f6dcb027fa524e5411f` | 1440547 |
The mapping is the one the report's `## Evidence (promoted)` section lists, taken from the report. The video was NOT promoted — never is.

**Draft exhibit line for `AURA-PHASE0.md` §Pilot (NOT written there yet — the manager's closing post writes the rows):**
| case | Aura verdict | failure class | detection latency | credit | wall time |
|---|---|---|---|---|---|
| Probe 5 / deliberately broken rerun of `TASK-1068` (`DT_Cards` Fog Cost 50→5000, data-only, `R-PROBE5`) | `VERIFY-FAILED (advisory)` — press refused (`hand slot 0 ('Fog') refused — cost 5000, gold 88`; HUD "Not enough gold"), no `BP_SiegeFog_C` spawned, gold unspent | `SC-§79` entry-point-unreachable — a data cell invisible to compile / suite / text QA (the `SC-§36.1` shape) | 0.81 s after the press | not visible (no credit field in any tool reply) | ≈20 min wall, PIE 2 min 49 s |

**Lane fact for §Pilot cl. 4:** the mutated and the committed `DT_Cards` were the SAME SIZE (46510 B) — a size check would have called the break unchanged (and, after the revert, could not have distinguished reverted from still-broken); only oid-vs-sha256 proved it both ways. The `SHIP-§` LFS law (oid-vs-sha256, never size) confirmed again.

## §9 — leg N4 — 5c commit — 2026-09-14

**Scope:** the 5c leg for a COMMITTED `done` row (`TASK-780`, host `1231bc4` — the contact-climb integration; clean-tree clause on the `TASK-1230` row). Same shape as §3/§4/§5/§8. ⛔ No engine work, no compile, no editor touch (GUI editor PID 6136 left as found, level editor on `L_Arena`, no PIE). ⛔ TASKBOARD.md not edited this dispatch — the manager is closing the pilot on the board concurrently; the status flip and the pilot row are the manager's to write.

**Commit:** `fa97de8` (`fa97de8777f76be4de98eb93b46cb368ee2843aa`) — `TASK-1230 leg N4 — TASK-780 runtime verification (advisory, VER-§6 pilot): VERIFIED 2/2 — hero climbs the staged Watch Tower to Z 1298.40 (deck 1296 +2.4) at 350 uu/s and climbs again 2.8 s after arrival; four admissions, zero refusals (attempt 2 void: hero killed by the bot) — qa/TASK-780-verify.md + 6 promoted VER-TASK-780 stills (host 1231bc4; evidence host = this leg)`. Not pushed (main 10 ahead of `origin/main`, 0 behind).

`git show --stat HEAD` (the COMMIT, never the index):
```
fa97de8777f76be4de98eb93b46cb368ee2843aa
 ...80-a1-t00m44s-ground-contact-at-ladder-foot.png |  3 ++
 ...VER-TASK-780-a1-t00m46s-mid-climb-on-ladder.png |  3 ++
 ...K-780-a1-t00m55s-mid-second-climb-on-ladder.png |  3 ++
 ...VER-TASK-780-a3-t00m39s-mid-climb-on-ladder.png |  3 ++
 .../VER-TASK-780-a3-t00m43s-hero-on-deck.png       |  3 ++
 ...SK-780-a3-t00m49s-hero-on-deck-second-climb.png |  3 ++
 .../.claude/pipeline/qa/TASK-780-verify.md         | 39 ++++++++++++++++++++++
 7 files changed, 57 insertions(+)
```
All six PNGs under `GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-14/`, each a 3-line LFS pointer (`git check-attr filter` → `lfs`); this leg is the named evidence host per `VER-§4` cl. 4 / `FR-§6`.

**Staging discipline (`SC-§102`, pathspecs from the git root one level up):** porcelain before = ` M CONVENTIONS.md`, ` M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md`, `?? qa/TASK-780-verify.md`; `git diff --cached` empty (the UE Git plugin auto-staged nothing; nothing needed unstaging). Staged by pathspec: the report + the six PNGs only. After staging, `git diff --cached --name-status` listed exactly those 7 as `A`; the two ` M` files and this handoff stayed unstaged. After the commit the tree holds only ` M CONVENTIONS.md`, ` M TASKBOARD.md`, `?? handoffs/TASK-1230-buildmaster.md` (other lanes' dirt, left out); `git diff --cached` empty.

**Copy proofs (`VER-§4` cl. 1 copy-out — COPY, never move; all six sources still present under `Saved/AuraVerify/`, machine-local, gitignored). sha256 measured on source and destination, equal in all six; the LFS pointer oid in the COMMIT (`git show HEAD:…png`) equals the same sha256 (oid-vs-sha256, never size):**
| source (`Saved/AuraVerify/`) | promoted as (`playtest-evidence/2026-09-14/`) | sha256 = LFS oid | bytes |
|---|---|---|---|
| `n4_a1_01_ground_contact_t44.36s_f30206.png` | `VER-TASK-780-a1-t00m44s-ground-contact-at-ladder-foot.png` | `b73e0707224fcb01773ad4e7ed9eca6eff3264a28cc47d4de809268aac4a66fc` | 776019 |
| `n4_a1_02_mid_climb_t46.83s_f30258.png` | `VER-TASK-780-a1-t00m46s-mid-climb-on-ladder.png` | `c56a3dc367a515d745020f1bbb9c33d03d0c8226b65b7e8ac03a29f75204a0ee` | 775071 |
| `n4_a1_04_mid_second_climb_t55.93s_f30664.png` | `VER-TASK-780-a1-t00m55s-mid-second-climb-on-ladder.png` | `dd913e84e8e7d00cc4b106b710af4e43f56a94172a98a9c9f1f9c222b4f176ef` | 775418 |
| `n4_a3_04_on_deck_closeup_t43.35s_f43907.png` | `VER-TASK-780-a3-t00m43s-hero-on-deck.png` | `255b5712813cc853f920db79b63809a73949d67fc14aaa606ba32d5d271d65f7` | 869808 |
| `n4_a3_06_on_deck_second_t49.97s_f44296.png` | `VER-TASK-780-a3-t00m49s-hero-on-deck-second-climb.png` | `a969da5cea2bc3f1fdb1a7fb401f6146f90f4b52134f88fbac09d57781b4cae2` | 868891 |
| `n4_a3_02_mid_climb_t39.85s_f43705.png` | `VER-TASK-780-a3-t00m39s-mid-climb-on-ladder.png` | `2f032fd0c05abfed2797f2aa7cbff1994eb9825f1ce273544330043d79bec6e4` | 780032 |
The mapping is the one the report's `## Evidence (promoted)` section lists, taken from the report. Multi-attempt ⇒ `-a1` / `-a3` suffixes (`VER-§4` cl. 2); attempt 2 is VOID (ghost pawn) and its stills were not promoted, per the report. The two videos (`Saved/AuraVerify/rec_1789414256091262300_10/recording.h264`, `rec_1789414498176396800_13/recording.h264`) were NOT promoted — never are.

**Draft row for `AURA-PHASE0.md` §Pilot (written there by §10 below, unstaged):**
| case | Aura verdict | 🧑 his playtest answer | match Y/N | credit | wall time |
|---|---|---|---|---|---|
| N4 / `TASK-780` (host `1231bc4`) | `VERIFIED (advisory) 2/2` (row 1 — Z 98.15 → 1298.40 = deck 1296 +2.4 uu, inside ±25, `speed 350.0` = `LadderClimbSpeedUU`, `MOVE_Walking` on the deck, `CurrentHP 200`; row 2 — a second climb admitted 2.8 s after the first's deck arrival, four admissions across two sessions, zero refusals) | *"Walking my hero into the tower's ladder foot climbs it onto the deck about 1200 uu up, and I can climb it again straight after."* | pending manager adjudication (observed: deck Z 1298.40, second climb admitted at +2.8 s) | not visible (no credit field in any tool reply) | ≈16 min (PIE drives 12:30:58–12:36:20 local; two sessions) |

**Lane facts from the report for §Pilot cl. 4:**
- (a) INPUT HOLD SUSTAINED: `inject_input_action IA_Move` with `hold_seconds` 8 and 5 was each sustained in ONE call (`action_hold_started`, the hero moved/climbed continuously to the stated length) — no back-to-back chaining, no gap, exit `H-3` never tripped. N1's 1.5 s release was the RECIPE, not the tool.
- (b) A held W walks the hero off the 472-uu deck in under 1 s at 500 u/s after the climb hands back `MOVE_Walking` — so "Z at +7 s" assumes a stop that does not happen; a 5 s hold (which ends on the deck) shows the held deck. A recipe timing, not a climb failure.
- (c) STALE LOG AFTER A RELAUNCH: after §7's relaunch, Aura's `get_unreal_output_logs` served the OLD instance's `Saved/Logs/GitClaudeUnrealTest_2.log` (`log_source=editor-cached`, PID 8228's file); the live PID 6136 writes `Saved/Logs/GitClaudeUnrealTest.log`. A verifier hazard: any log read after a relaunch must name the file whose line-1 `Log file open` matches the instance's creation time.
- (d) Route B (vs-bot) kills the hero: the bot killed the hero at PIE t≈02m51.7s in session 1 (`'BP_HeroCharacter_C_0' died — ghost … spawned`), voiding attempt 2; a ghost cannot climb by type. Drives on Route B must finish inside ≈2m50s or restart PIE.
- (e) Hero-side climb lines are `Verbose` and `ConfigureLadderLink`/abandonment lines were ABSENT for a runtime-spawned tower — zero `ClimbableTower`/`Ladder`/`ABANDONED` matches in PID 6136's log for either session; the verifier has no console lane to raise verbosity.

**Follow-ups observed in the report (report only, for the manager):** `ActiveClimber` unreflected (`TASK-1271` boards the `UPROPERTY`) — "released" is inferred from the admission, never read; the last ≈250 uu of each climb reports `Velocity.Z = 0` while Z still rises (H2, mechanism unread); fall from the deck left HP at 200 (fall damage apparently unmodelled, unmeasured elsewhere); the tower was STAGED by `pie_scene_edit spawn_actor`, never summoned from the hand; descent / `K-C` re-arm / unit-at-ladder / no-abduction / sprint lines not exercised. Editor left on `L_Arena` (level editor, PIE stopped), nothing saved.

## §10 — the consolidated §Pilot table — written to `qa/AURA-PHASE0.md` — 2026-09-14

Appended as `## Pilot (TASK-1230) — 2026-09-14` at the file's tail (after `## Tier`). Rows N1 / N3 / N2 / Probe 5 / N4 from §3/§4/§5/§8/§9; every `match` cell reads `manager: see TASK-1230 row` (adjudication is the manager's, on the board, concurrently — no Y/N invented here); numbers copied from the five verify reports with their PIE times, nothing re-derived. ⛔ Left UNSTAGED — the manager's closing host row commits it, not this leg.
