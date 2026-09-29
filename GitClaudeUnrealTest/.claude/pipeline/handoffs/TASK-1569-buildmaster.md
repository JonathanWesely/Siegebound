# TASK-1569 — COOKDIR-DANGLING-REFS-DIAGNOSE — build-master handoff

marker: `TASK-1569-COOKDIR-DANGLING-REFS-DIAGNOSE` · 2026-09-28 · build-master · read-only
law: `SC-§39` · `SC-§101` · `SC-§102` · `SC-§118` · `SC-§138` · `PKG-§14` · `VER-§9` · `VER-§12` cl. 7g

## Verdicts first

| | `BP_WobbleTarget` → `MI_Intro_Colorway` | `IK_MeshyBiped` → `SK_Footman_Meshy` |
|---|---|---|
| **Reach from a runtime root** | **NOT REACHED.** 0 referencers (hard and soft), a fixed point at round 1. Source/Config name it 0 times. | **NOT REACHED.** Its closure is 1 node, `RTG_MeshyBiped_to_SiegeBiped`, which has 0 referencers. Source/Config name either one 0 times. |
| **In the 2026-09-09 build** | **The holder ships** (shipped `.utoc` + staged manifest). **The target does not** (0 in both). | **The holder ships** (and so does its referencer, the RTG). **The target does not.** |
| **Did the cook see it** | **Yes, at `Display`.** `LogCook: Display: Package /Game/LevelPrototyping/Interactable/Target/BP_WobbleTarget has a dependency on package /Game/DemoTemplate/_Core/MI_Intro_Colorway which does not exist.` | **Yes, at `Display`.** `LogCook: Display: Package /Game/Characters/IK_MeshyBiped has a dependency on package /Game/Characters/Anims/AB_Test/SK_Footman_Meshy which does not exist.` |
| **Target ever tracked** | **Never.** 0 commits on any branch. The dangling reference has been in the holder since its first commit (`6963c32`, 2026-06-24), and the LFS object is identical at HEAD. | **Yes, for one commit span:** added in `02eda0f` (2026-07-21 23:55, his "made arena changes…"), deleted in `ec7a271` (2026-07-22 00:40, `TASK-246` "strip 02eda0f-swept debris"). Both commits are ancestors of HEAD. |

- **A correction, flagged and not edited (`SC-§101`, `SC-§126` cl. 5).** `handoffs/TASK-1445-programmer.md` §4 says "The shipped cook log does not show the `MI_Intro_Colorway` load error, so either it was present on 2026-09-09 or that log does not carry the line." Its count stands: the `LoadErrors … was not available` form does occur 0 times in that log. But the cooker's own dependency line, in a different wording, is there for both targets. So at the 2026-09-09 cook, the cooker said both targets **did not exist**. That settles the "unknown" that `TASK-1445` left open and the row's (3) expected to stay open.
- **One correction to the dispatch's wording.** It called the 13 soft edges "animation assets to missing preview meshes". The re-enumeration finds 8 edges to meshes (5 to `SKM_Manny`, 2 to `SKM_Quinn`, and 1 from an **IKRetargeter**, not an animation, to `SK_Footman_Meshy`). The other 5 go to `…/EditableAnimations/LS_*` packages. `TASK-1445` §4 had this right ("+ `LS_*` EditableAnimations").

## (0) State at my own instant (`SC-§118` cl. 1, `SC-§138`)

| Reading | Before (20:23 PDT) | After (20:34 PDT) |
|---|---|---|
| `UnrealEditor.exe` / `UnrealEditor-Cmd.exe` by `Get-CimInstance Win32_Process` | exactly 1: PID **12112**, created 2026-09-28 16:58:12, parent 24624. Command line: `"…\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "…\GitClaudeUnrealTest.uproject"`, with no `-game` ⇒ class `editor` | same PID, same command line, `-game` absent |
| `get_headless_status` | `editor_connected` | — |
| `is_pie_active` (a game-thread call, used as the liveness proof per `VER-§12` cl. 7g) | `is_active: false` | `is_active: false` |
| Level | `/Game/Maps/L_MainMenu.L_MainMenu` | same |
| Dirty (`EditorLoadingAndSavingUtils.get_dirty_content_packages` / `get_dirty_map_packages`) | `[]` / `[]` | `[]` / `[]` |
| `AssetRegistry.is_loading_assets()` | False | False |
| **cl. 7g walk** (`unreal.ObjectIterator(unreal.Blueprint)`, `Status == BS_ERROR`) | **0 of 37**, histogram `BS_UNKNOWN` 17 · `BS_UP_TO_DATE` 20 | **0 of 37**, same histogram (read 3 times; last read after my final registry query) |
| Residency (`unreal.find_package`, no load) of `BP_WobbleTarget`, `IK_MeshyBiped`, `RTG_…`, `IK_SiegeBiped` and both targets | all False. Control: `/Game/Maps/L_MainMenu` → True | all False. Control True |
| Editor log `Saved/Logs/GitClaudeUnrealTest.log` (opened 16:58:12, so this process) | baseline 3,307 lines | 3,361 lines. **The delta 3,308→end has 0** lines matching `LoadErrors`, `LogBlueprint: Error` or any of the 5 asset names. **Control:** the same needles over the whole log find 1 line, a pre-existing `LoadErrors: Error: Asset Manager settings do not include an entry for assets of type GameFeatureData…` from startup. |

⇒ **No asset was loaded.** The holders stayed non-resident, and a load of `BP_WobbleTarget` would have written the `LoadErrors … was not available` line that `TASK-1445` recorded. The cl. 7g walk carries a liveness control only (37 resident, with a histogram). No `BS_ERROR` fixture exists, so there is no discriminating control (`VER-§12` cl. 7g "Control strength"). The editor is as I found it. `TASK-1576`'s reads ran against the same editor; its Python lines are in the same log window.

**Instruments I used.** `mcp__unreal_inspector__execute_unreal_python_readonly` only, never the write lane `execute_unreal_python`, plus `get_headless_status` and `is_pie_active`. **`does_asset_exist` is registry-only, from a reading of engine source:** `UEditorAssetSubsystem::DoesAssetExist` → `AssetRegistryModule.Get().GetAssetByObjectPath(...)`. Its `EnsureAssetsLoaded()` helper only waits for the registry scan (`if (… AssetRegistry.IsLoadingAssets()) { … WaitForCompletion() … }`). That branch is a no-op because `is_loading_assets()` = False.

## (1) Registry: hard and soft queried separately

**API:** `unreal.AssetRegistryHelpers.get_asset_registry()` → `get_dependencies` / `get_referencers` with `unreal.AssetRegistryDependencyOptions(include_hard_package_references=…, include_soft_package_references=…, include_searchable_names=False, include_*_management_references=False)`, the `TASK-1439` §2 API. Existence per package: `get_assets_by_package_name(p, False)`.

### `BP_WobbleTarget`
- Path `/Game/LevelPrototyping/Interactable/Target/BP_WobbleTarget` (class `Blueprint`). It sits in COOKDIR `LevelPrototyping`.
- Hard deps (3): `/Game/DemoTemplate/_Core/MI_Intro_Colorway` (**registry assets 0**) · `…/Target/Assets/SM_TargetBaseMesh` (1) · `/Game/LevelPrototyping/Meshes/SM_Cylinder` (1).
- Soft deps (3): `/Engine/EditorResources/S_KBSJoint`, `S_KHinge`, `S_KPrismatic`.
- **Referencers: hard `[]`, soft `[]`.** Upward closure, hard ∪ soft: **0 nodes, a fixed point at round 1.**
- `does_asset_exist("/Game/DemoTemplate/_Core/MI_Intro_Colorway")` = **False**. The target's referencers: hard `[BP_WobbleTarget]`, soft `[]`.
- **Positive controls (fired):** `does_asset_exist(BP_WobbleTarget)` = True. Forward edge to an existing dependency: `SM_TargetBaseMesh` and `SM_Cylinder` both exist. The reverse query is not blind: `get_referencers(SM_TargetBaseMesh, hard)` has 3 entries and `SM_Cylinder`'s has 14, and **both contain `BP_WobbleTarget`**.

### `IK_MeshyBiped`
- Path `/Game/Characters/IK_MeshyBiped` (class `IKRigDefinition`). It sits in COOKDIR `Characters`.
- Hard deps (2): `/Game/Characters/Anims/AB_Test/SK_Footman_Meshy` (**registry assets 0**) · `/Script/IKRig`. Soft deps: `[]`.
- Referencers: **hard `[/Game/Characters/RTG_MeshyBiped_to_SiegeBiped]`**, soft `[]`.
- Upward closure: **1 node** (`RTG_MeshyBiped_to_SiegeBiped`, class `IKRetargeter`), a **fixed point at round 2**. The RTG has 0 referencers, hard and soft.
- `does_asset_exist("/Game/Characters/Anims/AB_Test/SK_Footman_Meshy")` = **False**. The target's referencers: hard `[IK_MeshyBiped]`, **soft `[RTG_MeshyBiped_to_SiegeBiped]`**.
- **Positive controls (fired):** `does_asset_exist(IK_MeshyBiped)` = True. Forward edges of the referencer: `get_dependencies(RTG_MeshyBiped_to_SiegeBiped, hard)` = `[IK_MeshyBiped, /Script/IKRig, IK_SiegeBiped]` (edge seen from both ends), and soft = `[…/AB_Test/SK_Footman_Meshy, /Game/Characters/SK_Footman]`. `IK_SiegeBiped` and `SK_Footman` exist.
- Context for (5): every IK asset under `/Game` (`ARFilter` on `/Script/IKRig` `IKRetargeter` and `IKRigDefinition`) is 3 retargeters and 4 rigs. **`IK_SiegeBiped`'s only referencer is also `RTG_MeshyBiped_to_SiegeBiped`.** The other 4 are `Ice_Magic` / `Fire_Magic` demo mannequin rigs.

### Runtime roots and reach
- **Maps the ship cooks:** `$RECIPE_MAPS` in `Tools/Packaging/ship.ps1` = `'/Game/Maps/L_MainMenu'`, `'/Game/Maps/L_Arena'`.
- **Config game-mode and map keys** (`Config/DefaultEngine.ini`, `[/Script/EngineSettings.GameMapsSettings]`): `GameDefaultMap=/Game/Maps/L_MainMenu.L_MainMenu` · `EditorStartupMap=/Game/Maps/L_Arena.L_Arena` · `GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode`. The only other game-mode keys in `Config/` are the three `+ActiveClassRedirects` for `TP_ThirdPerson*` → `GitClaudeUnrealTest*`. `Config/` has 0 `PrimaryAssetTypesToScan`, `AssetManagerSettings`, `ProjectPackagingSettings`, `DirectoriesToAlwaysCook`, `MapsToCook` or `bCookAll` (grep exit 1; the same corpus answers `GameDefaultMap`).
- **Forward closure from the two maps** (hard ∪ soft, `/Script/` skipped, to a fixed point): 278 packages, 231 of them `/Game`. The maps own 0 `__ExternalActors__` / `__ExternalObjects__` packages. **Neither holder, the RTG nor either target is in it.** The closure has 0 `LevelPrototyping` members and 0 `Meshy`/`RTG_`/`IK_` members. Positive control: `BP_MenuGameMode` = True.
- **The native half, and why it is needed:** `BP_HeroCharacter` and `WBP_HUD` are **not** in the map closure. The game reaches them by string paths in C++. So the registry walk from the maps is only half the reach question.
- **Text instrument for the native side** (`Source/` `*.cpp`/`*.h`, `Config/` `*.ini`, `-F`): `WobbleTarget` 0 · `IK_MeshyBiped` 0 · `MeshyBiped` 0 · `RTG_` 0 · `LevelPrototyping` 0 · `MI_Intro_Colorway` 0 · `DemoTemplate` 0 · `SK_Footman_Meshy` 0 · `AB_Test` 0 · `IKRig` 0 · `IKRetargeter` 0. Control: `BP_HeroCharacter` = 31.
  - `Tools/` (not runtime): `IK_MeshyBiped` has 1 file, `Tools/ArtPipeline/retarget_meshy_to_siegebiped.py`, but only in its docstring and comments, as the documented source of its chain map. It holds no asset path, and the Blender-side tool never loads the asset. `RTG_MeshyBiped`, `WobbleTarget` and `MI_Intro_Colorway` are 0 in `Tools/`. Control: `SK_Footman` = 7 files.
  - Source has 105 distinct `/Game/…` literals.
  - The only constructed `/Game/Characters` paths are `SK_%s`, `ABP_%s`, `Anims/A_%s_Attack` and `A_%s_Death` from a card id (`SummonedUnit.cpp`), plus the fixed `SK_Sorcerer` and `A_Sorcerer_Idle` (`CommanderNpc.cpp`). None of them can form `IK_…` or `RTG_…`.
  - A grep for folder enumeration (`GetAssetsByPath|GetAssetsByClass|ScanPathsSynchronous|GetAssets(`) in `Source/` returned 0. **That zero is uncontrolled.**
- **Binary-as-text scan of `Content/`** (`grep -rl --binary-files=text -F`), covering soft paths the registry may not index:
  - `BP_WobbleTarget`: only its own file.
  - `IK_MeshyBiped`: its own file + `RTG_…`.
  - `RTG_MeshyBiped_to_SiegeBiped`: its own file.
  - `MI_Intro_Colorway` and `DemoTemplate`: `BP_WobbleTarget.uasset` only.
  - `SK_Footman_Meshy`: `IK_MeshyBiped` + `RTG_…`.
  - **This matches the registry exactly.** Each file carrying its own name is the scan's positive control.
- **Reach verdict.** `BP_WobbleTarget` **is not reached**: no asset, map, C++ literal, config key or content string names it. `IK_MeshyBiped` **is not reached**: its only referencer, the RTG, is itself named by nothing.

### The 13 soft edges: the count holds
Sweep: `get_assets_by_path` over the 11 COOKDIRs gives **591** assets (Data 2 · UI 47 · Blueprints 30 · Input 39 · Characters 233 · Meshes 45 · Materials 60 · Textures 86 · VFX 14 · Audio 6 · LevelPrototyping 29), the same as `TASK-1445`. Across them, **1,040** `/Game` dependency edges were checked. **15 are dangling: 2 hard and 13 soft. The count holds.**

| Holder (class) | Soft target(s) |
|---|---|
| `MM_Attack_01` · `MM_Attack_02` · `MM_Attack_03` · `MM_ChargedAttack` · `MM_WallJump` (AnimSequence, `Characters/Mannequins/Anims/Unarmed/…`) | each → `/Game/Characters/Mannequins/Meshes/SKM_Manny`, **and** each → one `…/Mannequins/Animations/EditableAnimations/LS_*` (`LS_Attack_01` ×2, `LS_Attack_02`, `LS_Attack_03`, `LS_ChargedAttack`) |
| `MF_Pistol_Idle_ADS_AO_CD` · `_CU` (AnimSequence) | → `/Game/Characters/Heroes/Mannequin/Meshes/SKM_Quinn` |
| `RTG_MeshyBiped_to_SiegeBiped` (IKRetargeter) | → `/Game/Characters/Anims/AB_Test/SK_Footman_Meshy` |

5 + 5 + 2 + 1 = 13. The detector's control: 1,025 of the 1,040 edges resolved as existing. Its two hard misses agree with `does_asset_exist` = False.
**Scope.** The row asks only that these be re-enumerated. They are outside (4) and (5) here, apart from one measured fact in (2) below.

## (2) The 2026-09-09 build

- **Artifact:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Siegebound-Win64-Shipping-2026-09-09.zip`. Re-measured: **1,334,632,180 B**, sha256 **`1caf8dcd93fbff07294a0f7650d360d8ef2b0163455858ecca971e15833ea7f8`**, 71 entries. This matches `TASK-1441`.
- **Its run:** `.ship\20260910-063540\`. Its `ship-state.json` reads `timestampUtc 2026-09-10T06:35:40Z`, `config Shipping`, `head 31edc239…`, `cook PASS`. The container entries are dated 2026-09-09 23:37 PDT, the same as `cook.out.log`'s mtime.
- **Instruments (read-only, declared under `SC-§71a`).** I extracted 3 entries into the session scratchpad `…\scratchpad\t1569\` (nothing entered the repo): `GitClaudeUnrealTest-Windows.utoc` (687,265 B), `GitClaudeUnrealTest-Windows.pak` (11,466,608 B) and `Manifest_UFSFiles_Win64.txt` (327,069 B).
  - `UnrealPak.exe <utoc> -List`: exit 0, `3448 files`, 1,390 `.uasset` + 3 `.umap`.
  - `UnrealPak.exe <pak> -List`: exit 0, `1745 files`.

| Package (exact `Content/<path>.uasset`) | shipped `.utoc` | staged manifest |
|---|---|---|
| `LevelPrototyping/Interactable/Target/BP_WobbleTarget` | **1** | **1** |
| `Characters/IK_MeshyBiped` | **1** | **1** |
| `Characters/RTG_MeshyBiped_to_SiegeBiped` | 1 | 1 |
| `DemoTemplate/_Core/MI_Intro_Colorway` | **0** | **0** |
| `Characters/Anims/AB_Test/SK_Footman_Meshy` | **0** | **0** |
| soft targets `…/Meshes/SKM_Manny` · `…/Heroes/Mannequin/Meshes/SKM_Quinn` · `EditableAnimations/LS_*` (all 4) | 0 | 0 |
| all 7 soft-edge animation holders | 7 of 7 | — |
| controls: `SM_TargetBaseMesh`, `SM_Cylinder`, `IK_SiegeBiped`, `SK_Footman`, `SKM_Manny_Simple` | 1 each | 1 each |

- **Instrument hazards, caught, with what I did about them (`SC-§137`):**
  - The `.utoc` listing folds some directory case: `…/Anims/Pistol/AIM/MF_Pistol_Idle_ADS_AO_CU.uasset` against the manifest's `…/Aim/…`. So every zero above was re-run case-insensitively (`grep -ci`) and **stayed 0**.
  - A bare `SKM_Manny` needle hit `SKM_Manny_Simple`, a different asset. The table uses exact `…/<name>.uasset"` needles.
  - The `.pak` index names 0 of the 4 needles. Its control is `AssetRegistry.bin` 1 and `DefaultEngine.ini` 1. It holds non-package files only, so its zero carries no weight.
- **Listed chunk sizes in the `.utoc`** (a sighting): `BP_WobbleTarget` 1,627 B, `IK_MeshyBiped` 3,995 B, `RTG_…` 1,145 B.
- **The cook log** `…\.ship\20260910-063540\cook.out.log` (88,190 B, 882 lines; the cooker's own `-abslog` was deleted, per `TASK-1441`). Verbatim, lines 274–275, straight after `Cooked packages 0 Packages Remain 1034 Total 1034`:
  ```
  LogCook: Display: Package /Game/LevelPrototyping/Interactable/Target/BP_WobbleTarget has a dependency on package /Game/DemoTemplate/_Core/MI_Intro_Colorway which does not exist.
  LogCook: Display: Package /Game/Characters/IK_MeshyBiped has a dependency on package /Game/Characters/Anims/AB_Test/SK_Footman_Meshy which does not exist.
  ```
  - **At `Display` only:** the run's summary is `LogInit: Display: Success - 0 error(s), 3 warning(s)`. The 3 warnings are the MCP licence line and 2 `RecastNavMesh` serialize lines.
  - **0 lines** match `LoadErrors`, `was not available`, `Missing import`, `Failed import`, `Failed to load`, `Unable to find package`, `SKM_Manny`, `SKM_Quinn` or `EditableAnimations`.
  - The IoStore staging section (the `LogIoStore:` lines of the `UnrealPak … IoStore` runs) has 0 `missing` lines.
  - **Control:** the same reader finds `Success - `, `IsCookAll=false` and the `Cooked packages … Total 1400` tallies.
  - ⇒ **Measured: the cooker reported the 2 hard edges and did not report the 13 soft ones.**
- **Corroboration:** the neighbouring Shipping run `20260910-050628` (`-clientconfig=Shipping`, 102,293 B) carries the same two lines at its lines 309–310.

## (3) Did the missing targets ever exist? (git root one level up, `SC-§102`)

`git --no-optional-locks log --all -- <path>` from `C:\GitProjects\GitHub\GitClaudeUnrealTesting`, every path anchored `GitClaudeUnrealTest/…`:
- `GitClaudeUnrealTest/Content/DemoTemplate/_Core/MI_Intro_Colorway.uasset` → **0 commits**. The folder `…/Content/DemoTemplate` → 0. **Never tracked.**
- `GitClaudeUnrealTest/Content/Characters/Anims/AB_Test/SK_Footman_Meshy.uasset` → **2 commits.**
  - `02eda0f` (Jonathan Wesely, 2026-07-21 23:55:56 −0700, "made arena changes to make arena much bigger") shows `A` for 11 files under `AB_Test/`, including `SK_Footman_Meshy.uasset` and `SK_Footman_Meshy_Skeleton.uasset`.
  - `ec7a271` (2026-07-22 00:40:31 −0700, "TASK-246: strip 02eda0f-swept debris from m7.6-arena10x (pre-merge hygiene)") shows `D` for the same 11.
  - Both are ancestors of HEAD (`merge-base --is-ancestor`).
- A glob `*MI_Intro_Colorway*`, `*SK_Footman_Meshy*`, `*DemoTemplate*`, `*AB_Test*` over all history finds 2 commits in total: the two above.
- **Controls, same anchoring:** `BP_WobbleTarget.uasset` → `6963c32` (2026-06-24, "testing to see if Git works"). `IK_MeshyBiped.uasset` and `RTG_…` → `f609887` (2026-07-19, `TASK-235` Meshy anim batch) + `02eda0f`. All 3 are tracked at HEAD (`ls-files --error-unmatch`).
- **When each dangling reference entered, from a filesystem read of the local LFS cache** `.git/lfs/objects/<oid>`. `git show <c>:<path>` returns only the LFS pointer; its first try read 0 for every needle, and that zero was discarded as unreadable (`SC-§39`).
  - `IK_MeshyBiped` is the **same LFS object `e6cb3359…` at `f609887`, `02eda0f` and HEAD**. It already names `AB_Test` / `SK_Footman_Meshy` at `f609887`, **two days before that file was first tracked.** So the target sat on disk untracked on 2026-07-19. This is the one place "untracked on disk" is answerable, and here the answer is yes.
  - `RTG_…` is `573d172d…` at both commits and names `SK_Footman_Meshy` in both.
  - `BP_WobbleTarget` is `8bd8784b…` at `6963c32` and HEAD, and names `MI_Intro_Colorway` ×2 and `DemoTemplate` ×1 in both. So the reference arrived with the template content in the repo's first content commit.
  - The holders' own names are the positive controls (count ≥ 2).
- **Reading:** `ec7a271` removed `AB_Test/` and did not fix `IK_MeshyBiped` or the RTG. Git shows neither file in that commit.
- **On disk at the 2026-09-09 cook:** the ship head `31edc23`'s tree has 0 files under either target folder. **Beyond git, the cooker's own line says "which does not exist"** for both targets at that cook (§2). So the row's "unknown" is now answered by the cook log: both were absent at that cook. What stays inferred is the exact mechanism behind the cooker's "no PackageData" (see (4)).
- Today, `Content/DemoTemplate` and `Content/Characters/Anims/AB_Test` do not exist (`ls`).

## (4) What a cooked load would do

- **Does a cooked load ever happen?** By (1), nothing the game runs reaches either holder. No map, no asset, no C++ literal, no config key. ⇒ **In the shipped game, no load of either holder is expected. The question below is hypothetical unless something acquires a reference later.**
- **The runtime Shipping log can say nothing either way.** `VER-§9`: the build is **Shipping**, with no `bUseLoggingInShipping`, so an absence there is `UNOBSERVABLE`. I did not run the packaged game, as the row requires.
- **Engine behaviour, read at source (UE 5.8; these are readings, not runs):**
  1. **Cook, discovery.** `FRequestCluster::FGraphSearch::FExploreEdgesContext::QueueVisitsOfDependencies` (`Editor/UnrealEd/Private/Cooker/CookRequestCluster.cpp`): `if (!DependencyVertex.GetPackageData()) { … UE_LOGFMT(LogCook, Display, "Package {PackageName} has a dependency on package {MissingDependency} which does not exist.", …); } continue;`. The missing edge is **logged at `Display` and skipped**. It is not an error and not a warning, which matches the measured `Success - 0 error(s)`.
  2. **Load with a missing import.** In `FLinkerLoad::VerifyImport` (`Runtime/CoreUObject/Private/UObject/LinkerLoad.cpp`), a failed verify that is not `bCrashOnFail` runs `Import = OriginalImport;` ("put us back the way we were") and posts a `LoadErrors` message. The editor also logs the `AsyncLoading2.cpp` form that `TASK-1445` quoted, `FMessageLog("LoadErrors").Info(… "While trying to load package {MissingPackage}, a dependent package {DependentPackage} … was not available…")`, followed by `ImportedPackageRef->SetIsMissingPackage()`. The importer keeps loading.
  3. **Runtime (Zen loader).** In `AsyncLoading2.cpp`, `static bool GFailLoadOnMissingImport = false;` is the `s.FailLoadOnMissingImport` CVar. The whole-package refusal `"Package %ls (0x%ls) will not be loaded due to Missing imported package (0x%ls)"` fires only when that CVar is true. `Config/` sets it 0 times (grep exit 1). ⇒ **Reading:** a cooked holder that still carried the import would **load**, with the import left null.
- **INFERENCE, labelled (`SC-§101`):** the cooker saves `BP_WobbleTarget` from an in-memory graph in which the material import resolved to null. The save-time harvester (`PackageHarvester.cpp`) collects imports only from objects it is actually handed. So the cooked `BP_WobbleTarget` probably **carries no import of `MI_Intro_Colorway` at all**, and the slot is simply null. If the actor were ever spawned, a null override would most likely fall back to the mesh's own material. The same shape likely holds for `IK_MeshyBiped`'s preview-mesh import.
- **Not measured.** The instrument that would measure it exists and was **not run**: `UnrealPak -Describe=<utoc> -PackageFilter=… -DumpToFile=…`. At source it is the `else if (FParse::Value(FCommandLine::Get(), TEXT("Describe="), …))` branch of `IoStoreUtilities.cpp`, which lists each package's imports and warns `"Missing import: 0x%llX in package …"`. It needs the 1.10 GB `.ucas` pulled out of the zip, and it is outside this row's declared instruments. See (5).

## (5) Options, costed. None chosen (`SC-§101`)

Measured facts that shape the costing:
- Both holders ship on folder placement (`PKG-§14`), not on reach.
- The build that shipped them reported the 2 edges at `Display` and passed.
- `BP_WobbleTarget` has 0 referencers. `IK_MeshyBiped` has exactly 1, the RTG, and the RTG has 0.
- Neither holder can arm the `VER-§12` cl. 7g PIE modal. That needs a `BS_ERROR` Blueprint: `IK_MeshyBiped` is not a Blueprint, and `BP_WobbleTarget` read `BS_UP_TO_DATE` in `TASK-1445`.

| Option | What breaks | What it costs | Still unknown |
|---|---|---|---|
| **(i) Leave both as they are** | Nothing new. It is the 2026-09-09 state, which cooked, passed and shipped both holders unreached. | 0 edits. Every cook keeps writing the 2 `LogCook: Display` lines. Every editor load of `BP_WobbleTarget` (a sweep, a Content Browser open, a census like `TASK-1445`'s) writes one `LoadErrors … was not available` line; `IK_MeshyBiped` presumably does the same (not measured). Three small packages ride in the build (listed `.utoc` sizes 1,627 / 3,995 / 1,145 B). **Gate blind spot, named:** both lines are `LogCook: Display`, so they pass `C2-UAT-LOG` (`'BUILD SUCCESSFUL'`) and the working-tree `C2-COOK-BP-ERRORS` (`$BP_ERROR_NEEDLE = 'LogBlueprint: Error'` + the `Success/Failure - N error(s)` summary). This is the class that `PKG-§14`'s 2026-09-28 rider calls a blind spot, and nothing would flag a third such edge. | What a *future* reference would do. If someone places `BP_WobbleTarget` in `L_Arena`, it reaches runtime with a null slot (§4 inference). Whether the cooked packages carry the import is unmeasured (§4). |
| **(ii) Clear or repoint the dead reference in each** | `BP_WobbleTarget`: nothing references it, so no consumer breaks. The mesh loses the absent `DemoTemplate` look, which it has never had here. `IK_MeshyBiped`: repointing its preview mesh (the hard edge's property is **not measured**; `PreviewSkeletalMesh` is my guess) to an existing mesh with a different skeleton could disturb the rig's retarget-chain bone bindings. The docstring of `Tools/ArtPipeline/retarget_meshy_to_siegebiped.py` cites "IK_MeshyBiped's 9 verified chains" as the source of its `CHAIN_MAP`. | **`BP_WobbleTarget`:** a Blueprint edit of the component template that holds the material, then **🧑 his Compile + Ctrl+S**. A BP compile does not dirty the package, so `save_assets` writes nothing (memory rule; `SC-§68` sha256-CHANGED gate). Finding the slot needs a `load_asset`. That is safe for the modal (`BS_UP_TO_DATE`) but logs the `LoadErrors` line, and this row forbids it. **`IK_MeshyBiped`:** a non-BP asset property edit + save (an editor lane, not a keystroke, if a property set dirties it; unmeasured) + a `load_asset` to read the property first. **The RTG's soft edge to `SK_Footman_Meshy` stays** unless the RTG is edited too. Each re-save of a COOKDIR asset **changes what ships** and is owed a cook to confirm. | Which property carries each edge. Whether a 5.8 re-save of 2026-06 template content changes anything else in the package. Whether the rig survives a preview-mesh swap. |
| **(iii) Delete the asset(s)** (`TASK-1557` shape: its own commit, so `git revert` restores; `PKG-§14` cl. 6 *delete, never move*) | **`BP_WobbleTarget` alone:** breaks nothing registry-visible (0 referencers, 0 text references). **`IK_MeshyBiped` alone: NOT clean.** It would create a new dangling HARD edge, RTG → `IK_MeshyBiped` (the `TASK-1440` (2)(b) rule). The clean unit is **`IK_MeshyBiped` + `RTG_MeshyBiped_to_SiegeBiped` together**. That pair has 0 outside referencers. It leaves `IK_SiegeBiped` with **0 referencers**; it still ships by folder and is a candidate for the same question. It also removes the 13th soft edge. | A tracked-file delete plus a commit. In-editor it is an asset delete; as a file delete it needs no keystroke but an editor-closed window, under the standing close grant, dirty `[]` first. It also drops 1–2 packages from the next build (a cook confirms). | **"Nothing references it" is not "nothing wants it" (`TASK-1439` §5):** the Meshy pipeline cites `IK_MeshyBiped` as the documented source of its chain map. Its sanctioned export path bypasses UE's retargeter ("This tool bypasses UE's exporter entirely"). The RTG/IK pair is therefore probably reference material rather than a live tool, but that reads intent from a docstring. And `LevelPrototyping` is the UE template's folder: whether 🧑 he wants the template interactables kept as a set (`BP_JumpPad`, `BP_DoorFrame`, `BP_WobbleTarget`) is his call. |

**Could be added to any option:** one read-only follow-up would turn the (4) inference into a measurement: extract the `.ucas` to scratch and run `UnrealPak -Describe` filtered to the 2 holders. It costs ~1.1 GB of scratch and a few minutes, and involves no editor. It would show whether the cooked packages import the missing packages at all. I did not run it: it is outside this row's declared instruments.

## Not examined / limitations

- **No asset was loaded,** so the property carrying each dangling edge is not read. It is a guess in (5)(ii), labelled as one.
- **The cooked packages' import tables were not read** (`-Describe` not run). §4's "no import survives in the cooked package" is an inference from source.
- **The runtime was not observed.** A Shipping log absence is `UNOBSERVABLE` (`VER-§9`), and the game was not run.
- **Reach is registry + text.** The Source folder-enumeration grep's 0 is **uncontrolled**. A runtime path built from data outside the card-id patterns, or a UTF-16 or compressed string in content, would evade the text scans. The registry closure is authoritative for asset-to-asset edges.
- **The cooker's `-abslog`** for the 2026-09-09 run is deleted (`TASK-1441`). The UAT-side `cook.out.log` evidently carries `LogCook: Display` lines, but whether it carries every per-package load message is not established. So its 0 `LoadErrors` lines is a sighting, not a controlled zero.
- **The 13 soft edges:** re-enumerated only. I did not establish why the cooker ignored them (probably editor-only preview references; not read at source).
- **The `.utoc` case-folding** (`AIM` vs `Aim`) is uncharacterised. Every zero was re-run case-insensitively.
- **The Development zip** `Siegebound-Win64-Development-2026-08-29.zip` was not examined; the row scoped the 2026-09-09 build.
- **The cl. 7g walk** has a liveness control only (37 resident). No `BS_ERROR` fixture exists.
- `TASK-1576`'s reads were live against the same editor for part of this window. Neither of us wrote.

## Fences honoured

No `load_asset`, save, delete, redirector fix-up, edit or asset-editor open. No PIE, compile, cook, package or `-Describe`. No git write: only `log`, `show`, `ls-files`, `ls-tree`, `merge-base --is-ancestor` and `rev-parse`, all `--no-optional-locks`. Nothing extracted into the repo. `CONVENTIONS.md` and every other row untouched. `Content/` porcelain: 0 lines. HEAD `9b82e8d`. Editor PID 12112 left running, as found.

## Routing

No code and no asset ⇒ no QA. Returns to the **manager**, who puts (5) to 🧑 him as one question. This handoff rides the next commit host (expected `TASK-1580`). Files touched: this handoff, and `TASKBOARD.md` (TASK-1569's `status:` line only). Scratch: `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\35e683bf-936e-4e1a-93dd-6b0127c2c1b8\scratchpad\t1569\` (utoc, pak, manifest, both `-List` outputs).
