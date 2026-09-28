# TASK-1557 — SHOWROOM-TRIO-DELETE — art-director handoff

- **Agent:** art-director, 2026-09-27 (log clock 2026-09-28 06:2x UTC), PID 3108.
- **Status: `blocked`. The STOP rule fired on the FIRST delete.** `delete_asset(LVL_Showroom)` returned **`False`**, so I deleted nothing more. `GM_TestGamemode` and `BP_Basic_Movement` were **NOT deleted**.
- **What IS gone (partial):** `LVL_Showroom.umap` and its `LVL_Showroom_BuiltData.uasset`. Registry, disk and the log all agree they are gone. The `Content/` manifest diff is exactly those 2 removals, with 0 additions and 0 modifications anywhere.
- **`BS_ERROR` before → after: 1 → 1.** `BP_Basic_Movement` is still resident in `BS_ERROR`. **The PIE blocker is NOT cleared.**
- **Why it returned `False` (MEASURED from the log; the reason for the re-run is a HYPOTHESIS):** the MCP write tool (`mcp__unreal_editor__execute_unreal_python`) **ran my one script twice in the same frame `[824]`**. Run 1 did the delete (`Force Deleting 2 Package(s)`). Run 2 found the asset already gone (`DeleteAsset failed: Could not find the source asset`) and returned `False`. The only output handed back to me was run 2's. I did not reason my way past the rule. That is the orchestrator's or manager's call (§7).
- No dialog or modal appeared at any point (§3). No save, no PIE, no compile, no git write, no raw file operation.

## 0. State at my own instant, before any act (`SC-§118`, `SC-§138`)

| Reading | Value | Instrument |
|---|---|---|
| Editor processes | exactly 1 `UnrealEditor.exe`, **PID 3108**, CreationDate 2026-09-26 23:05:31, cmdline `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`. This is the GUI editor. The only other `Unreal*` process is `UnrealTraceServer` (29504). | `Get-CimInstance Win32_Process` |
| MCP `:8000` | listening on 127.0.0.1:8000, owned by **PID 3108** | `Get-NetTCPConnection` |
| `get_headless_status` | `editor_connected` | inspector |
| PIE | `is_active: false` | `is_pie_active` |
| Level | `/Game/Maps/L_MainMenu.L_MainMenu`, so not `LVL_Showroom` | `UnrealEditorSubsystem.get_editor_world()` |
| Asset editors on the trio | `GM_TestGamemode` and `LVL_Showroom` **not resident**, so no editor could be open on them. `BP_Basic_Movement` resident (TASK-1439), with **0** `Opening Asset editor` lines in the log. Visible top-level windows of 3108: the main window and `Message Log` (below). No BP-editor window. | `find_object`/`find_package`, `get_unreal_output_logs`, `EnumWindows` |
| Dirty | `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]` | `EditorLoadingAndSavingUtils.get_dirty_content_packages/get_dirty_map_packages` |
| `is_loading_assets()` | `False` | registry |
| Pre-existing window | **`Message Log`** (0x150268): non-modal, owned by the main window. The main window `IsWindowEnabled=True`, so it is not a modal. Probably the Load Errors page from TASK-1445's census (`BP_WobbleTarget`). I never touched it. | `EnumWindows` / `IsWindowEnabled` |
| Source-control provider | `Saved/Config/WindowsEditor/SourceControlSettings.ini`: `Provider=None` | file read |
| `git status` baseline | `git --no-optional-locks status --porcelain=v1 --untracked-files=all -- GitClaudeUnrealTest/Content/ GitClaudeUnrealTest/Config/ GitClaudeUnrealTest/GitClaudeUnrealTest.uproject`, run from the git root `C:/GitProjects/GitHub/GitClaudeUnrealTesting` (`SC-§102`) → **0 lines**. **Control:** the same pathspec matches 4,773 tracked `Content/` files and 6 `Config/` files (`git ls-files`), and the whole-repo status does list the working-tree `M` files. So the silence is a real zero. HEAD `9a67a27`. | git (read-only) |
| `Content/` manifest (before) | **4,783 files, 10,622,423,101 bytes**. The manifest file's own sha256 is `e45ae69ccff889a71d0d2a0d026157e35ac09de6349d7f009bf8a8bbb973556f` (format `relpath\tbytes\tsha256`, sorted, LF). | `hashlib.sha256` walk |

**The deletion set as measured before the delete** (the conditional member **exists**):

| Object path | File | Bytes | sha256 | Tracked? |
|---|---|---|---|---|
| `/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom.LVL_Showroom` | `Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap` | 179,816 | `51833d6e0f7b392925e07268b66e923815c422dcc1d7e852467be1fefa8dfec4` | yes (blob `44a391ad…`) |
| `/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom_BuiltData` (conditional, **exists**) | `…/Levels/LVL_Showroom_BuiltData.uasset` | 1,049,821 | `994799e14518c30139e91f6b63f69ecaf82876e8ed8e029c2346f2df30aa487f` | **no**: ignored by `GitClaudeUnrealTest/.gitignore:120` `*_BuiltData.uasset` |
| `/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.GM_TestGamemode` | `…/Blueprints/GM_TestGamemode.uasset` | 17,212 | `ad6a8eef4b87046982f13d3c4e5bcd6e9e5011f54c1e567b02ff493558b49d65` | yes (blob `6e3d9b9d…`) |
| `/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement` | `…/Blueprints/BP_Basic_Movement.uasset` | 75,510 | `609dad8e8660483ae59c4a2d63893228145d3b6a2cad0f9eba249accc65b2170` | yes (blob `4c4d2bd6…`) |

The pack on disk before the delete: 77 files = 76 registry assets (including the BuiltData) + the loose `Noise10.png`. 76 are tracked: the 75 `.uasset`/`.umap` files other than the BuiltData, plus `Noise10.png`.

## 1. Closure re-proof, at my own instant: **matches TASK-1439 exactly**

API: `AssetRegistryHelpers.get_asset_registry()` → `get_referencers` / `get_dependencies`. Hard and soft were queried separately via `AssetRegistryDependencyOptions(include_hard_package_references / include_soft_package_references, include_searchable_names=False, management off)`. `is_loading_assets()` = False.

| Check | Reading |
|---|---|
| Referencers `BP_Basic_Movement` | hard `['/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode']`, soft `[]` |
| Referencers `GM_TestGamemode` | hard `['/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom']`, soft `[]` |
| Referencers `LVL_Showroom` | hard `[]`, soft `[]` |
| Referencers `LVL_Showroom_BuiltData` | hard `['/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom']`, soft `[]` |
| Upward closure of BP (hard+soft, walked to a fixed point) | `['…/Blueprints/GM_TestGamemode', '…/Levels/LVL_Showroom']`, **= {GM, LVL}** ✔ |
| Trio referencers outside the trio | `{'BP_Basic_Movement': [], 'GM_TestGamemode': [], 'LVL_Showroom': []}` ✔ |
| Pack assets in the registry / edges from outside the pack | **76** / **0** (159 internal edges, the same as TASK-1439) ✔ |
| `ObjectRedirector`s in the pack | **0** ✔ |
| `Content/__ExternalActors__/sA_ArcheryVfxPack`, `__ExternalObjects__/sA_ArcheryVfxPack` | both absent (`ls`: No such file or directory) ✔ |
| **Positive control:** `get_dependencies(GM_TestGamemode, hard)` | `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']`, **edge present, FIRED** ✔ |
| Pack class histogram | Blueprint 9 · MapBuildDataRegistry 1 · Material 10 · MaterialInstanceConstant 13 · NiagaraEmitter 8 · NiagaraSystem 15 · StaticMesh 12 · Texture2D 7 · World 1 |

**The trio's own `/Game` dependencies (11), all KEPT.** All 11 come from `LVL_Showroom`; `GM` → BP only; BP → `/Script` only.
- **Would be left with no referencer ("newly unreferenced, kept"), 8:** `BP_ArrowShower`, `BP_Projectile_7A`, `BP_Projectile_Electricty`, `BP_Projectile_Spawner`, `BP_Projectile_Spawner1`, `BP_Projectile_Spawner_Electricty`, `BP_Projectile_Toxic`, `Materials/MI_Ground_Showscene`.
- **Still referenced after the delete, 3:** `FX/NS_ArrowShower` and `FX/NS_ArrowShower_Shoot` (by `BP_ArrowShower`), `FX/NS_ToxicArrowShoot_Shoot` (by `BP_Projectile_Spawner1`).
- `LVL_Showroom`'s `/Engine` dependencies (BasicShapes, MapTemplates, EngineMaterials) are engine content, untouched.

**Second instrument:** `grep -rl --binary-files=text -e BP_Basic_Movement -e GM_TestGamemode -e LVL_Showroom Content/` → exactly the 4 deletion-set files (`BP_Basic_Movement.uasset`, `GM_TestGamemode.uasset`, `LVL_Showroom.umap`, `LVL_Showroom_BuiltData.uasset`). Over `Config/ Source/ Plugins/ GitClaudeUnrealTest.uproject` → 0 (rc=1). Supplementary UTF-16LE scan of `Content/` for the three names → 0 files.

**No difference from TASK-1439, no outside referencer, no grep hit outside the set ⇒ the STOP gate for (1) did not fire. I proceeded to (2).**

## 2. Pack Blueprint status before the delete (TASK-1445's instrument)

`ARFilter(package_paths=["/Game/sA_ArcheryVfxPack"], recursive_paths=True, class_paths=[TopLevelAssetPath("/Script/Engine","Blueprint")], recursive_classes=True)` → **9**. Then `EditorAssetLibrary.load_asset` → `get_editor_property("Status")`.

| Blueprint | Resident before? | Status |
|---|---|---|
| `Blueprints/BP_ArrowShower` | no | `BS_UP_TO_DATE` |
| `Blueprints/BP_Basic_Movement` **(control)** | yes | **`BS_ERROR`** ✔ fired |
| `Blueprints/BP_Projectile_7A` | no | `BS_UP_TO_DATE` |
| `Blueprints/BP_Projectile_Electricty` | no | `BS_UP_TO_DATE` |
| `Blueprints/BP_Projectile_Spawner` | no | `BS_UP_TO_DATE` |
| `Blueprints/BP_Projectile_Spawner1` | no | `BS_UP_TO_DATE` |
| `Blueprints/BP_Projectile_Spawner_Electricty` | no | `BS_UP_TO_DATE` |
| `Blueprints/BP_Projectile_Toxic` | no | `BS_UP_TO_DATE` |
| `Blueprints/GM_TestGamemode` | no | `BS_UP_TO_DATE` |

**No other pack Blueprint is in `BS_ERROR`.** Memory walk (`unreal.ObjectIterator(unreal.Blueprint)`): **total 111, `BS_ERROR` = 1** (`BP_Basic_Movement`), the expected 1. Dirty afterwards: `[]`/`[]`. Log slice 5143–5154: no `LogBlueprint: Error` and no `[Compiler]` lines. There are six `LogNiagara: TemplateParameterOverrides … Updating in PostLoad to User key` lines (BP_Projectile_7A / _Electricty / _Toxic). These are PostLoad fix-ups, and they did not dirty anything.

⚠️ **Board discrepancy (not a STOP):** the row's kept list names **five** other Blueprints. The registry has **seven** besides the trio's two. The two extras are `BP_Projectile_Spawner1` and `BP_Projectile_Spawner_Electricty`. Both fall under the row's by-rule kept set anyway.

## 3. The delete: STOPPED after the first call

Before the call, from inside the same script: referencers of `LVL_Showroom` hard `[]` / soft `[]`, dirty `([], [])`, `is_loading_assets` False. Editor windows: main + `Message Log`, both `enabled=True`.

**Call 1:** `unreal.EditorAssetLibrary.delete_asset("/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom.LVL_Showroom")` via `mcp__unreal_editor__execute_unreal_python`. **Returned output: `RETURN … = False`.** Post-readings in the same script: `does_asset_exist(LVL_Showroom)=False`, `does_asset_exist(BuiltData)=False`, `.umap` on disk False, BuiltData on disk False, dirty `([], [])`, and `GM_TestGamemode` referencers hard `[]` / soft `[]`.

**The log, verbatim (`Saved/Logs/GitClaudeUnrealTest.log` 5155–5190; everything is in frame `[824]`):**
```
5155 [06.23.26:409][824]LogAuraPython: Warning: PythonExec crash-diagnostics stack has 2 entries at scope start (expected <= 1) — an earlier push was orphaned; clearing when this call completes.
     [06.23.26:412][824]Cmd: log LogPython off                      <- run 1 starts
5164 [06.23.26:619][824]LogUObjectGlobals: Force Deleting 2 Package(s):
        Asset Name: /Game/sA_ArcheryVfxPack/Levels/LVL_Showroom.LVL_Showroom            Asset Type: World
        Asset Name: /Game/sA_ArcheryVfxPack/Levels/LVL_Showroom_BuiltData.LVL_Showroom_BuiltData   Asset Type: MapBuildDataRegistry
     [06.23.27:055][824]LogWorld: UWorld::CleanupWorld for LVL_Showroom, bSessionEnded=true, bCleanupResources=true
5172 [06.23.27:394][824]LogObjectTools: Warning: Detected inconsistencies between reference gathering algorithms. Switching 'Editor.UseLegacyGetReferencersForDeletion' on for the remainder of this editor session.
5173 [06.23.27:394][824]LogEditorTransaction: Non zero active count in UTransBuffer::Reset  ActiveCount : 1  SessionName : Aura  Reason : Delete Selected Item  Purging the undo buffer...
     [06.23.28:173][824]Cmd: log LogPython Log                      <- run 1 ends
     [06.23.28:189][824]Cmd: log LogPython off                      <- run 2 starts (same frame)
5186 [06.23.28:193][824]LogEditorAssetSubsystem: Error: DeleteAsset failed: Could not find the source asset. The AssetData '/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom.LVL_Showroom' could not be found in the Asset Registry.
     [06.23.28:242][824]Cmd: log LogPython Log                      <- run 2 ends
```
(Some intervening load lines are elided: landscape texture builds, `LogChaosDD`, `LogDatasmithContent`, `LogUObjectHash`.)

**Reading it, MEASURED:** one tool call produced **two** executions of the script, both in frame `[824]`. Run 1 performed the delete, and the engine logged it: 2 packages, the World and its BuiltData. Run 2's pre-checks passed trivially (a deleted package has no referencers), so it called `delete_asset` again. That call failed with "could not be found in the Asset Registry" and returned `False`. The output handed back to me is run 2's alone. Run 1's own return value is not observable, because `LogPython` is switched off during the exec, so its prints never reach the log.
**HYPOTHESIS (labelled, `SC-§101`), not tested:** the Aura wrapper re-ran the script because `ForceDeleteObjects` → `ResetTransaction` purged the undo buffer while Aura's own transaction was open (`ActiveCount: 1`, `SessionName: Aura`). The orphaned crash-diagnostics push at 5155 also points at the wrapper.
**Why I stopped anyway:** the row and the dispatch both say *"A `False` return … ⇒ stop at once: delete nothing more."* The return I held was `False`. Explaining it is not a licence to go past it, and a second destructive call through the same wrapper would re-run just the same (§7).

**Dialog / modal: NONE.** After the call, the windows were: main + the pre-existing `Message Log`, both `enabled=True`. `UnrealEditor` `Responding=True`. The log frame index keeps advancing (`[824]` → `[118]` → `[670]` and on). MCP answered every later call.

**Calls 2 and 3 (`GM_TestGamemode`, `BP_Basic_Movement`): NOT MADE.**

## 4. After, in the same editor (read-only readings)

| Reading | Value |
|---|---|
| `does_asset_exist` | `LVL_Showroom` **False** · `LVL_Showroom_BuiltData` **False** · `GM_TestGamemode` **True** · `BP_Basic_Movement` **True** |
| `find_object` / `find_package` | LVL + BuiltData: False / False. GM: True / True. **BP: True / True (still resident).** |
| Referencers now | `GM_TestGamemode` hard `[]` / soft `[]` (its only referencer is gone) · `BP_Basic_Movement` hard `['…/GM_TestGamemode']` / soft `[]` |
| Control `get_dependencies(GM, hard)` | `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']`, still fires |
| Pack registry count | **74** (= 76 − 2) |
| Sweep of every `/Game` package's hard + soft dependencies (**3,957** packages) | **0** edges to either deleted package |
| **Memory walk** | **total 111, `BS_ERROR` = 1** (`/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement`). **1 → 1.** I did not run `collect_garbage`: the BP was not deleted, so GC would not change it. |
| Grep over `Content/` | `BP_Basic_Movement.uasset`, `GM_TestGamemode.uasset` (the map's two files no longer hit) |
| Level / PIE / dirty | `L_MainMenu` · not active · `[]`/`[]` |
| Pack residency | 69 of 74 pack packages resident. `delete_asset` loads its target first, so loading `LVL_Showroom` pulled in its placed classes and effects. They were not dirtied. |

## 5. On disk

**Manifest after:** 4,781 files, 10,621,193,464 bytes. The manifest file's own sha256 is `93ead7eb9c98d592f985c844d72ea6440cea7e356b13abea6c94be7cc08db6cf`.
**Diff (after vs. before):**
```
REMOVED=2
  - sA_ArcheryVfxPack/Levels/LVL_Showroom.umap            179816   51833d6e0f7b392925e07268b66e923815c422dcc1d7e852467be1fefa8dfec4
  - sA_ArcheryVfxPack/Levels/LVL_Showroom_BuiltData.uasset 1049821 994799e14518c30139e91f6b63f69ecaf82876e8ed8e029c2346f2df30aa487f
ADDED=0
MODIFIED=0
PACK before=77 after=75 kept_identical=75
```
The byte delta is 1,229,637, exactly the two files' sizes. **0 changes anywhere else under `Content/`**, which covers the eleven cook folders (`PKG-§14`) and every kept pack file, `Noise10.png` included.

**`git status`** (same top-anchored pathspec as the baseline): ` D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap`, and nothing else. It is **unstaged** (` D`), which fits `Provider=None`. `git diff --cached -- …/Content/` is empty. I did not stage, reset or restore anything. The BuiltData removal is **invisible to git** because the file is ignored (`.gitignore:120`). `Config/` and the `.uproject`: no change.

**Kept pack list: 75 files** (all byte-identical to the before manifest):
- `Blueprints` (9): BP_ArrowShower, **BP_Basic_Movement**, BP_Projectile_7A, BP_Projectile_Electricty, BP_Projectile_Spawner, BP_Projectile_Spawner1, BP_Projectile_Spawner_Electricty, BP_Projectile_Toxic, **GM_TestGamemode** (the two in bold stay only because of the STOP)
- `FX` (15): NS_ArrowShower, NS_ArrowShower_Shoot, NS_LightninSoot_Shoot, NS_LightningShoot, NS_LightningShoot_Arrow, NS_LightningShoot_Hit, NS_PowerSoot_Explosion_1, NS_Powershoot_1, NS_Powershoot_1_Shoot, NS_Powersoot_1_Arrow, NS_ToxicArrow1_Arrow, NS_ToxicArrowShoot_1, NS_ToxicArrowShoot_2, NS_ToxicArrowShoot_Shoot, NS_Toxic_Hit
- `FX/NiagaraEmitters` (8): NE_Basic_Mesh_2, NE_Flash, NE_SimpleBurst, NE_SimpleMesh, NE_Smoke3, NE_Smoke_0, NE_Smoke_01, Ne_Basic_Burst
- `Materials` (23): MI_Aoe, MI_Electricty, MI_Fres_Trans, MI_Fres_Trans_Toxic, MI_Ground_Showscene, MI_Line_Vert, MI_PowerShoot, MI_PowerShoot_Electricty, MI_PowerShoot_Sphere, MI_PowerShoot_Sphere2, MI_PowerShoot_Sub, MI_Radial_2, MI_ToxicShoot_Sphere, M_Basic_Particle, M_Electricty, M_Fres_Trans, M_Fres_Trans_2, M_Ground, M_Lightning, M_Masked_Pan_Addittive, M_Masked_Pan_Sub, M_Master_RadialBeam, M_Smoke
- `Materials/Textures` (8): Noise10.png, T_Flame_Aura_1, T_Ground_Crack_3, T_Lightning_Subuv, T_Line_Vert, T_Noise10, T_Radial_1, T_Smoke_SubUV
- `Models` (12): SM_Arrow_1, SM_Arrow_1_Back, SM_Aura_Line, SM_Electric_Arrow, SM_Electric_Arrow_Elec, SM_Electric_Shoot, SM_Plane, SM_PowerShoot_1, SM_PowerShoot_2, SM_PowerShoot_Sphere, SM_PowerShoot_Sphere_2, Sm_Arrow_Shower

## 6. Side effects of the one delete, for the next reader

1. **The editor's undo buffer was purged** (`UTransBuffer::Reset … Purging the undo buffer`). Any undo history in PID 3108 is gone. That follows from `ForceDeleteObjects`, not from any act of mine beyond the delete.
2. **`Editor.UseLegacyGetReferencersForDeletion` is ON for the rest of PID 3108's session** (engine warning 5172). It clears on the `TASK-1538` relaunch.
3. The `Content/sA_ArcheryVfxPack/Levels/` **directory remains, empty**. Git does not track directories, and the manifest has no entries for them.
4. The `Content/` directory's own mtime reads 2026-09-27 23:23:27.048 (local), which falls inside the delete. The top-level entry count is 36 now and was 36 in my earlier `ls`, and the manifest shows 0 files added. I did not record the directory mtime before, so what touched it is **not examined**.
5. Aura re-indexing started after the content change (`LogAura: StartIndexing`). `Saved/SourceControl/UncontrolledChangelists.json` was re-saved; that is under `Saved/`, not `Content/`.

## 7. The state I left, and what the next dispatch needs (the orchestrator's / manager's call, not mine)

- **Editor:** PID 3108, same GUI editor, never closed or relaunched. PIE off. `L_MainMenu`. Dirty `[]`/`[]`. `Message Log` window still open, as I found it. **`BROKEN_BP_LOADED` = True, `BS_ERROR` = 1: the PIE modal is still armed.**
- **Content:** the showroom map + BuiltData are deleted. `GM_TestGamemode` (now with 0 referencers) and `BP_Basic_Movement` (referenced only by GM) remain on disk and in the registry. No dangling edge exists anywhere (0 of 3,957 packages).
- **Remaining work under his answer:** delete `GM_TestGamemode`, then `BP_Basic_Movement`, same method. The closure for those two is re-proven above: GM has no referencer, and BP's only referencer is GM.
- **Recommended lane fix before any re-dispatch (a design suggestion, not a ruling):** the write wrapper can run a script more than once per call, so a destructive script must be **run-aware**. (a) Guard every destructive call with `does_asset_exist` first and record "already absent" as a distinct outcome from `False`. (b) Append each run's result to a file with a run counter, so a second execution is visible and run 1's return value is not lost. (c) Keep reading the log slice after every call. Whether a `False` produced this way counts as the row's `False` is a ruling for the row's author.
- **`TASK-1538`** must not treat the demo trio as gone: it would relaunch into an editor where `BP_Basic_Movement` still exists, though it will not be resident until something loads it.
- **Host `TASK-1559`:** so far only ` D …/Levels/LVL_Showroom.umap` is in the tree. The BuiltData removal never shows in git.

## Not examined / limitations

- **Why the wrapper executed twice** is a hypothesis (§3). I did not probe it by re-running anything destructive.
- **Run 1's own return value** is unobserved. That the delete succeeded is shown by the engine's `Force Deleting 2 Package(s)` line, the registry, and the disk. It is not shown by a returned `True`.
- No API lists open asset editors (`AssetEditorSubsystem` has no `get_all_edited_assets` in Python). "No editor open on the trio" rests on non-residency (GM, LVL), 0 `Opening Asset editor` log lines, and the visible-window list.
- The contents of the `Message Log` window were not read. It predates my first act and is non-modal.
- `Content/`'s directory mtime (§6.4) was not baselined.
- The grep and the UTF-16 scan cover ASCII and UTF-16LE. Compressed payloads would evade both. The registry census is the primary instrument.
- The manifests live in my session scratchpad (`manifest_before.tsv`, `manifest_after.tsv`). Their sha256 values and the full diff are quoted above.

## Routing

**blocked** → orchestrator / manager: rule on the double-run `False`, then either re-dispatch the remaining two deletes (with the run-aware lane) or re-rule. Gate `TASK-1558` can already read §1, §2 and §5 for the part that happened. Files touched: this handoff and `TASKBOARD.md` (TASK-1557's `status:` line only).

## Resume (2026-09-27 local / 2026-09-28 06:3x UTC log clock, art-director, same PID 3108): **the delete is complete, and the status is now `ready-for-integration`**

**Ruling I resumed on (the orchestrator's, quoted from the dispatch):** the §3 `False` "does NOT count as a failed delete". The log proves the tool ran the one script twice in frame `[824]`: run 1 deleted the map, and run 2 found it absent. Jonathan's authority is unchanged: he chose "Delete the pack's demo trio" first-hand. **Result:** `GM_TestGamemode` and then `BP_Basic_Movement` were deleted, one asset per call, through `EditorAssetLibrary.delete_asset`. **Both calls executed their script TWICE again.** The run-aware lane made that harmless and visible: run 1 deleted and returned `True`, and run 2 recorded `ALREADY_ABSENT`. No STOP rule fired, and no dialog appeared.

### R0. State at my own instant, before any act (`SC-§118`, `SC-§138`)

| Reading | Value |
|---|---|
| Editor | exactly 1 `UnrealEditor.exe`, **PID 3108**, CreationDate 2026-09-26 23:05:31, cmdline `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`, the GUI editor. The only other `Unreal*` process is `UnrealTraceServer` 29504 (`--sponsor 3108`). `:8000` is listening, owned by 3108. `get_headless_status` = `editor_connected`. |
| PIE / level | `is_active: false` · `/Game/Maps/L_MainMenu.L_MainMenu` |
| Dirty | `[]` / `[]` · `is_loading_assets()` False |
| Windows (`EnumWindows`) | main `GitClaudeUnrealTest - Unreal Editor` + the pre-existing non-modal `Message Log` (0x150268), both `enabled=True`, `Responding=True`. The same two as in §0. |
| Trio | `LVL_Showroom` / `_BuiltData`: `does_asset_exist` False, `find_object`/`find_package` False. `GM_TestGamemode`: exists, resident, referencers hard `[]` / soft `[]`. `BP_Basic_Movement`: exists, resident, referencers hard `['/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode']` / soft `[]`. |
| Positive control | `get_dependencies(GM_TestGamemode, hard)` = `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']`, **fired** |
| `BP_Basic_Movement`'s own deps | hard: `/Script/InputCore`, `/Script/HeadMountedDisplay`, `/Script/NavigationSystem`, `/Script/ClothingSystemRuntimeNv`. Soft: `[]`. No `/Game` dependency, so nothing becomes newly unreferenced by its removal. |
| Memory walk (`unreal.ObjectIterator(unreal.Blueprint)`) | **total 111, `BS_ERROR` = 1** (`/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement`) |
| Pack registry | **74** assets. Blueprint filter (recursive classes) = 9: the 7 kept + GM + BP. |
| `Content/` manifest at resume start | 4,781 files, 10,621,193,464 bytes, sha256 `93ead7eb9c98d592f985c844d72ea6440cea7e356b13abea6c94be7cc08db6cf`. **`cmp` byte-identical to §5's `manifest_after.tsv`**: nothing moved between the first delete and this resume. |
| git (read-only, anchored at the git root `SC-§102`) | ` D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap` only. `diff --cached` is empty. HEAD `9a67a27`. |

### R1. The lane: one asset per call, idempotent and self-evidencing

Tool: `mcp__unreal_editor__execute_unreal_python`. The script was identical for both calls apart from `CALL`/`OBJ`. It did the following:
1. Counted the prior `START` lines for its own call token in the run log. Run number = that count + 1.
2. Read the pre-state: `GFrameCounter` (`SystemLibrary.get_frame_count`), the editor log's byte size, `does_asset_exist`, referencers hard and soft (separately, `AssetRegistryDependencyOptions`, searchable-names/management off), dirty, `is_loading_assets`.
3. Appended a `START` line with `fsync`.
4. Took its branch. If absent → `ALREADY_ABSENT`, no call. Else any referencer → `REFUSED_REFERENCERS`. Else dirty → `REFUSED_DIRTY`. Else loading → `REFUSED_LOADING`. Else `delete_asset(OBJ)`.
5. Read `does_asset_exist`, `find_object` and dirty again, and appended an `END` line with `fsync`.

The run log is `<scratchpad>/t1557_runlog.tsv`. It sits beside `manifest_*.tsv`, in the session scratchpad and outside the repo.

Between the two calls, I ran an independent read-only re-query (`execute_unreal_python_readonly`). It read `GM_TestGamemode` absent, `find_object` False. `BP_Basic_Movement` referencers were hard `[]` / soft `[]`. Dirty was `[]`/`[]`. The walk read total 110, `BS_ERROR` 1, and the pack registry count was 73.

### R2. The run log, verbatim (all 8 lines; local clock = UTC-7)

```
START	CALL-A-GM	run=1	2026-09-27T23:31:43.486	frame=5259020	edlog_off=723303	path=/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.GM_TestGamemode	exists_before=True	ref_hard=[]	ref_soft=[]	dirty_before=([], [])	loading=False
END	CALL-A-GM	run=1	2026-09-27T23:31:44.841	path=/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.GM_TestGamemode	outcome=DELETE_CALLED	delete_return=True	exists_after=False	find_object_after=False	dirty_after=([], [])
START	CALL-A-GM	run=2	2026-09-27T23:31:44.895	frame=5259022	edlog_off=724429	path=/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.GM_TestGamemode	exists_before=False	ref_hard=[]	ref_soft=[]	dirty_before=([], [])	loading=False
END	CALL-A-GM	run=2	2026-09-27T23:31:44.898	path=/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.GM_TestGamemode	outcome=ALREADY_ABSENT	delete_return=n/a	exists_after=False	find_object_after=False	dirty_after=([], [])
START	CALL-B-BP	run=1	2026-09-27T23:32:42.970	frame=5262449	edlog_off=725195	path=/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement	exists_before=True	ref_hard=[]	ref_soft=[]	dirty_before=([], [])	loading=False
END	CALL-B-BP	run=1	2026-09-27T23:32:45.725	path=/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement	outcome=DELETE_CALLED	delete_return=True	exists_after=False	find_object_after=False	dirty_after=([], [])
START	CALL-B-BP	run=2	2026-09-27T23:32:45.758	frame=5262450	edlog_off=727242	path=/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement	exists_before=False	ref_hard=[]	ref_soft=[]	dirty_before=([], [])	loading=False
END	CALL-B-BP	run=2	2026-09-27T23:32:45.761	path=/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement	outcome=ALREADY_ABSENT	delete_return=n/a	exists_after=False	find_object_after=False	dirty_after=([], [])
```
**Output the tool handed back to me** (run 2's only, both times): `CALL-A-GM run=2 outcome=ALREADY_ABSENT …` and `CALL-B-BP run=2 outcome=ALREADY_ABSENT …`. Without the run log I would again have seen only the second execution. **Run 1's `True` is now OBSERVED** for both deletes, which closes the §3 limitation for these two.

### R3. Each delete's evidence (`Saved/Logs/GitClaudeUnrealTest.log`, verbatim excerpts)

**`GM_TestGamemode`, call A** (run 1 = log frame `[ 20]`, run 2 = `[ 22]`; the log's bracket = `GFrameCounter` mod 1000):
```
5239 [06.31.43:485][ 20]Cmd: log list LogPython
5240 [06.31.43:485][ 20]Cmd: log LogPython off                       <- run 1
5243 [06.31.44:003][ 20]LogUObjectGlobals: Force Deleting 1 Package(s):
5244 	Asset Name: /Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.GM_TestGamemode
5245 	Asset Type: Blueprint
5247 [06.31.44:547][ 20]LogEditorTransaction: Non zero active count in UTransBuffer::Reset  ActiveCount : 1  SessionName : Aura  Reason : Delete Selected Item  Purging the undo buffer...
5257 [06.31.44:857][ 20]Cmd: log LogPython Log                       <- run 1 ends
5258 [06.31.44:869][ 20]LogSourceControl: Display: Uncontrolled Changelist persistency file saved …/Saved/SourceControl/UncontrolledChangelists.json
5259 [06.31.44:876][ 21]LogAura: StartIndexing called at: 23:31:44:876
5260 [06.31.44:893][ 22]Cmd: log list LogPython
5261 [06.31.44:894][ 22]Cmd: log LogPython off                       <- run 2 (guard held: no delete call, so no "DeleteAsset failed" line)
5262 [06.31.44:906][ 22]Cmd: log LogPython Log
```
**`BP_Basic_Movement`, call B** (run 1 = `[449]`, run 2 = `[450]`):
```
5267 [06.32.42:967][449]LogAuraPython: Warning: PythonExec crash-diagnostics stack has 2 entries at scope start (expected <= 1) — an earlier push was orphaned; clearing when this call completes.
5269 [06.32.42:969][449]Cmd: log LogPython off                       <- run 1
5276 [06.32.44:915][449]LogUObjectGlobals: Force Deleting 1 Package(s):
5277 	Asset Name: /Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement
5278 	Asset Type: Blueprint
5280 [06.32.45:455][449]LogEditorTransaction: Non zero active count in UTransBuffer::Reset  ActiveCount : 1  SessionName : Aura  Reason : Delete Selected Item  Purging the undo buffer...
5290 [06.32.45:732][449]Cmd: log LogPython Log                       <- run 1 ends
5292 [06.32.45:751][450]LogAura: StartIndexing called at: 23:32:45:751
5294 [06.32.45:757][450]Cmd: log LogPython off                       <- run 2 (guard held)
5295 [06.32.45:771][450]Cmd: log LogPython Log
```
The elided lines are `LogDatasmithContent` deprecation notices (the load that `delete_asset` does first) and `LogUObjectHash` compaction. **From line 5229 to the end (5302), the only `Warning` line is 5267, and there is no `Error` line.** No `LogBlueprint` error, no `[Compiler]` line, and no second `Detected inconsistencies …` warning (legacy referencer gathering was already on, §6.2). No `DeleteAsset failed` line: my guard never let run 2 call it.

**Dialogs / modal: NONE.** The window census after each call and at the end read the same two windows: main + the pre-existing `Message Log`, both `enabled=True`, `Responding=True`. The log's frame index kept advancing (`[20]`→`[449]`→`[54]`→`[70]`). MCP answered every call.

**The double run, what it now shows (MEASURED):** 3 of 3 force-deletes through this tool (§3's map, GM, BP) were each followed by a second execution of the same script, each after the same `UTransBuffer::Reset … SessionName : Aura … Purging the undo buffer` line. The second execution came in the same frame (`[824]`) once, and 1–2 frames later (`[20]`→`[22]`, `[449]`→`[450]`) twice. So "same frame" is not an invariant. In both of these calls, run 2 did NOT purge the undo buffer (it made no delete call), and no third execution followed. **HYPOTHESIS, still labelled (`SC-§101`), strengthened but not tested:** the Aura wrapper retries a script when the undo buffer is purged under its open transaction. I made no control call: I did not send a non-deleting write through the same tool to check that it runs only once.

### R4. After, in the same editor (read-only, `execute_unreal_python_readonly`)

| Reading | Value |
|---|---|
| **Memory walk** | **total 109, `BS_ERROR` = 0 (`[]`)** on the FIRST walk after the delete. **`BS_ERROR` 1 → 0 on the same reader** (111 → 110 → 109 across the two deletes). `collect_garbage` was NOT needed and NOT run. |
| `find_object` / `find_package` | `None` for all four deleted paths. `find_object(…/BP_Basic_Movement.BP_Basic_Movement_C)` = `None` |
| `does_asset_exist` | False for `LVL_Showroom`, `LVL_Showroom_BuiltData`, `GM_TestGamemode`, `BP_Basic_Movement` |
| Referencers of the 4 deleted packages | hard `[]` / soft `[]`, each |
| **Sweep, every `/Game` package's hard + soft dependencies** | **3,955 packages, 0 edges to any deleted package** (3,957 in §4 − GM − BP) |
| Pack registry | **72 = 76 − 4** ✔. Class histogram: Blueprint 7 · Material 10 · MaterialInstanceConstant 13 · NiagaraEmitter 8 · NiagaraSystem 15 · StaticMesh 12 · Texture2D 7. The World and MapBuildDataRegistry are gone, and Blueprint went 9 → 7. |
| Pack Blueprints (registry) | `BP_ArrowShower`, `BP_Projectile_7A`, `BP_Projectile_Electricty`, `BP_Projectile_Spawner`, `BP_Projectile_Spawner1`, `BP_Projectile_Spawner_Electricty`, `BP_Projectile_Toxic`: **all 7 kept** |
| `ObjectRedirector`s in the pack | 0 (no redirector was left behind) |
| Level / PIE / dirty | `L_MainMenu` · `is_active: false` · `[]` / `[]` |

### R5. On disk

**Grep** (`grep -rl --binary-files=text -e BP_Basic_Movement -e GM_TestGamemode -e LVL_Showroom`): over `Content/` → **0 files (rc=1)**. It found exactly the 4 deletion-set files before (§1), so it can see a hit. Over `Config/ Source/ Plugins/ GitClaudeUnrealTest.uproject` → 0 (rc=1). A same-instrument positive control, `-e BP_ArrowShower` over the pack, → `BP_ArrowShower.uasset` (it still sees a live name).

**Final manifest:** 4,779 files, 10,621,100,742 bytes, sha256 `8a7512bbff7c1bc9d8fa30ecb4982c2e7f841a509c831e9e2c639113788a7dd2` (`<scratchpad>/manifest_final.tsv`, same `manifest.py`). **Full diff against the ORIGINAL before-manifest** (`manifest_before.tsv`, sha `e45ae69ccff889a71d0d2a0d026157e35ac09de6349d7f009bf8a8bbb973556f`, 4,783 files; its sha was re-verified at resume):
```
REMOVED=4
  - sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.uasset 75510 609dad8e8660483ae59c4a2d63893228145d3b6a2cad0f9eba249accc65b2170
  - sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.uasset 17212 ad6a8eef4b87046982f13d3c4e5bcd6e9e5011f54c1e567b02ff493558b49d65
  - sA_ArcheryVfxPack/Levels/LVL_Showroom.umap 179816 51833d6e0f7b392925e07268b66e923815c422dcc1d7e852467be1fefa8dfec4
  - sA_ArcheryVfxPack/Levels/LVL_Showroom_BuiltData.uasset 1049821 994799e14518c30139e91f6b63f69ecaf82876e8ed8e029c2346f2df30aa487f
ADDED=0
MODIFIED=0
PACK before=77 after=73 kept_identical=73
BYTES before=10622423101 after=10621100742 delta=1322359 removed_sum=1322359
DIFF_EQUALS_TRIO_EXACTLY=True
```
Every removed file's bytes and sha256 match the pre-delete table in §0. **0 changes anywhere else under `Content/`.** That covers the eleven cook folders (`PKG-§14`) and all 73 kept pack files (byte-identical, `Noise10.png` included). The kept pack list is §5's 75 files minus `GM_TestGamemode` and `BP_Basic_Movement` = **73**: Blueprints 7 · FX 15 · FX/NiagaraEmitters 8 · Materials 23 · Materials/Textures 8 · Models 12. `Content/sA_ArcheryVfxPack/Levels/` remains as an **empty directory** (git does not track directories). `Blueprints/` holds the 7 kept files, all with their original mtimes (Jul 10).

**`git status`** (same anchored pathspec as R0; the control `git ls-files` matches 4,773 `Content/` and 6 `Config/` files):
```
 D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.uasset
 D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.uasset
 D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap
```
All three are **unstaged** (` D`, `Provider=None`). `git diff --cached` over `Content/ Config/ .uproject` is **empty**. The BuiltData removal is invisible to git (`.gitignore:120`). `Config/` and the `.uproject`: no change. HEAD is still `9a67a27`. I staged, reset and restored nothing.

### R6. Side effects of this resume

1. The undo buffer was purged twice more (R3, one purge per delete). Nothing new was lost beyond §6.1.
2. Aura re-indexed after each delete (`LogAura: StartIndexing`), and `Saved/SourceControl/UncontrolledChangelists.json` was re-saved each time. Both are under `Saved/`, not `Content/`.
3. The `Blueprints/` directory mtime moved to 2026-09-27 23:32 (the deletes). No file in it changed.

### R7. End state (the editor I leave for `TASK-1538`)

PID **3108**, the same GUI editor (cmdline re-read at the end), **never closed or relaunched**, `Responding=True`. PIE off. `L_MainMenu`. Dirty `[]`/`[]`. The `Message Log` window is still open, as I found it. **`BS_ERROR` = 0: `BP_Basic_Movement` no longer exists in memory or on disk, so the PIE-blocker Blueprint is gone.** `Editor.UseLegacyGetReferencersForDeletion` is still ON for this session (§6.2), and it clears on the relaunch.

### For the manager (not blockers)

- **Kept-list discrepancy:** the row's kept list names **5** other Blueprints, and the registry has **7**. The extras are `BP_Projectile_Spawner1` and `BP_Projectile_Spawner_Electricty`. Both were kept under the row's by-rule kept set. The row text (sourced from `handoffs/TASK-1441-buildmaster.md`) under-counts.
- **"Newly unreferenced, kept"** stays as in §1: 8 assets (`BP_ArrowShower`, `BP_Projectile_7A`, `BP_Projectile_Electricty`, `BP_Projectile_Spawner`, `BP_Projectile_Spawner1`, `BP_Projectile_Spawner_Electricty`, `BP_Projectile_Toxic`, `Materials/MI_Ground_Showscene`). GM and BP had no `/Game` dependency outside the trio, so their removal added none.
- **Lane law candidate:** the write tool re-executes a script after a force-delete (3/3). Any destructive script through `execute_unreal_python` should carry R1's guard and its run log. Otherwise the returned output is the second, no-op execution, and it can read as a failure (§3) or, in a non-idempotent script, act twice.

### Not examined / limitations (resume)

- **Why** the wrapper re-executes is still a hypothesis (R3). I sent no non-deleting write call as a control.
- I did not re-run the per-Blueprint `Status` table for the 7 kept Blueprints after the delete: that would load them. The memory walk (0 `BS_ERROR` across all 109 resident Blueprints) covers the resident ones. The kept files are byte-identical on disk.
- I did not re-run §1's full closure census at resume. R0 re-read the parts the remaining two deletes depended on: GM's and BP's referencers, the forward-edge control, dirty and loading. The resume-start manifest proved nothing on disk had moved since §5.
- No API lists open asset editors (as in §0). Neither GM nor BP had an editor window in the census.
- The run log, manifests and window-census script are in my session scratchpad, not in the repo. Their contents and hashes are quoted above.

### Routing (resume)

**`ready-for-integration`** → gate **`TASK-1558`** (text) → integration check **`TASK-1538`** (5c) on the relaunched editor → host **`TASK-1559`**. The host's delta is the 3 ` D` paths above (BuiltData is gitignored). Files touched this resume: this handoff (appended) and `TASKBOARD.md` (TASK-1557's `status:` line only).
