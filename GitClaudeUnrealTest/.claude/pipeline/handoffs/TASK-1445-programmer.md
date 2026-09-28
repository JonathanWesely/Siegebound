# TASK-1445 — COOKDIR-BROKEN-BP-CENSUS — programmer handoff

- **COUNT, first: 0.** No Blueprint with unresolved compiler errors sits inside the eleven `-COOKDIR` folders right now. **51 of 51** Blueprint-family assets there load and read `BS_UP_TO_DATE`, with 0 load failures.
- **Control: FIRED.** In the same pass, with the same reader, `BP_Basic_Movement` (known broken, loaded by TASK-1439 earlier in this session) reads `BS_ERROR`. The zero is a controlled zero.
- **Agent:** gameplay-programmer, 2026-09-27, the last passenger of the batched read-only session TASK-1439 → TASK-1443 → TASK-1445 on PID 3108. No fix, no edit, no delete, no move, no code, no compile, no PIE, no git, no `ship.ps1` edit.
- **No escalation to 🚨 Blockers owed:** the count is zero. One **adjacent** finding (§4) is outside the row's question, and I flag it rather than escalate it.

## 1. The COOKDIR list, re-read at my own instant (`SC-§138`)

`Tools/Packaging/ship.ps1:281-293`, `$RECIPE_COOKDIRS = @(` … `)`, entries on lines **282–292**: `'Data'` (282) · `'UI'` (283) · `'Blueprints'` (284) · `'Input'` (285) · `'Characters'` (286) · `'Meshes'` (287) · `'Materials'` (288) · `'Textures'` (289) · `'VFX'` (290) · `'Audio'` (291) · `'LevelPrototyping'` (292).
**The file agrees with the remembered list. No disagreement to declare.** The args are built from it at `:1948-1950`, one `-COOKDIR=<Content>\<d>` per entry.

## 2. Instrument, named, and its control

- **Enumerator (no load):** `AssetRegistry.get_assets(unreal.ARFilter(package_paths=["/Game/<each of the 11>"], recursive_paths=True, class_paths=[TopLevelAssetPath("/Script/Engine","Blueprint")], recursive_classes=True))`. `recursive_classes` pulls in every `UBlueprint` subclass: `WidgetBlueprint`, `AnimBlueprint`, `ControlRigBlueprint`. Result: **51** (34 `Blueprint`, 12 `WidgetBlueprint`, 2 `AnimBlueprint`, 3 `ControlRigBlueprint`). 26 were already resident, and the census loaded the other 25.
- **Level-script Blueprints** live inside `.umap`s and escape a class filter, so I checked separately: `World` assets inside the eleven = `[]`. There are none to miss.
- **Reader:** `unreal.EditorAssetLibrary.load_asset(<object path>)` (loading runs compile-on-load), then `Blueprint.get_editor_property("Status")` (`unreal.BlueprintStatus`).
- **Positive control, same call, same reader:** `/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement` → **`<BlueprintStatus.BS_ERROR: 2>`**. The reader can say "broken". (TASK-1439's handoff has that asset's verbatim compiler errors.)
- **Discrimination across the whole editor:** an `unreal.ObjectIterator(unreal.Blueprint)` walk with the same test finds **103** Blueprints in memory after the census, **1** in `BS_ERROR` (the control) and 102 not. The same predicate the engine's pre-PIE scan applies (`PlayLevel.cpp:1299`).
- **Second instrument (log):** `Saved/Logs/GitClaudeUnrealTest.log` from my baseline (line 4807) through the census's end (4814). **0** `LogBlueprint: Error` lines, **0** `[Compiler]` lines. Compare TASK-1439's load of the control, which wrote 4 `LogBlueprint: Error … [Compiler]` lines into the same log. The one new non-routine line is the adjacent `LoadErrors` in §4, a missing dependency, not a compile error.
- **Dead instruments not re-bought (`VER-§12`):** no `function_graphs`/`ubergraph_pages`, and no CDO reflection.

## 3. Per-finding table (clause (3))

**Zero findings, so the table is empty:**

| Asset path | COOKDIR | Verbatim first error line | Referenced? (registry) | Reachable from `L_MainMenu`/`L_Arena`? |
|---|---|---|---|---|
| — | — | — | — | — |

**The full audited set (51), all `BS_UP_TO_DATE`**, so a reader can see what "zero" covered:
- **UI (10):** `WBP_CardHand` · `WBP_CastleHealthBar` · `WBP_CombatantHealthBar` · `WBP_DeckBuilder` · `WBP_DeckCardTile` · `WBP_HUD` · `WBP_MainMenu` · `WBP_SessionMenu` · `WBP_VictoryScreen` · `WBP_WarMap`
- **Blueprints (30):** `BP_BattlefieldScatter` · `BP_CommanderNpc` · `BP_HeroCharacter` · `BP_MenuGameMode` · `BP_Projectile_Fireball` · `BP_SiegeFog` · `BP_SiegeGhostPawn` · `BP_Torch` · `Buildings/` ArrowTower, BallistaTower, Barracks, BombTower, CrystalTower, DeepMine, Wall, WatchTower (8) · `Units/` Archer, Cavalry, Cleric, Footman, Knight, Longbowman, MilitiaMob, Miner, Ogre, Pikeman, Sapper, Sorcerer, Witch, Wizard (14)
- **Input (3):** `Touch/BPI_TouchInterface` · `Touch/UI_Thumbstick` · `Touch/UI_TouchSimple`
- **Characters (5):** `ABP_Footman` · `Mannequins/Anims/Unarmed/ABP_Unarmed` · `Mannequins/Rigs/CR_Mannequin_Body` · `CR_Mannequin_FootIK` · `CR_Mannequin_Procedural`
- **LevelPrototyping (3):** `Interactable/Door/BP_DoorFrame` · `Interactable/JumpPad/BP_JumpPad` · `Interactable/Target/BP_WobbleTarget`
- **Data, Meshes, Materials, Textures, VFX, Audio: 0 Blueprints each** (591 assets across the eleven in total, per the registry: Data 2 · UI 47 · Blueprints 30 · Input 39 · Characters 233 · Meshes 45 · Materials 60 · Textures 86 · VFX 14 · Audio 6 · LevelPrototyping 29).

**Historical corroboration (not controlled, so not counted):** the shipped build's UAT-side cook log `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540\cook.out.log` (88,190 B, the log `TASK-1441` used) carries `LogInit: Display: Success - 0 error(s), 3 warning(s)`, with **0** `LogBlueprint` and **0** `[Compiler]` lines. Its 3 warnings are the MCP licence notice and two `RecastNavMesh` serialize warnings. That fits no broken BP having cooked on 2026-09-09. But no broken BP was in that cook to prove the summary would have counted one, so it is a sighting, not a control.

## 4. ADJACENT finding: outside the row's question, flagged, not counted, not diagnosed

The census load wrote one `LoadErrors` line. It led to a registry-only sweep (no loads) of **dangling `/Game` dependencies** across all 591 assets in the eleven: **15 dangling edges, 2 of them HARD**:

1. **`/Game/LevelPrototyping/Interactable/Target/BP_WobbleTarget` → HARD → `/Game/DemoTemplate/_Core/MI_Intro_Colorway`**, which does not exist (registry: absent; disk: `Content/DemoTemplate` does not exist; the path string is in `BP_WobbleTarget.uasset` ×1). Verbatim, from the census load:
   `LoadErrors: While trying to load package /Game/LevelPrototyping/Interactable/Target/BP_WobbleTarget, a dependent package /Game/DemoTemplate/_Core/MI_Intro_Colorway (749F9C1ADFCD3B73) was not available. Additional explanatory information follows:`
   `FPackageName: Skipped package /Game/DemoTemplate/_Core/MI_Intro_Colorway has a valid, mounted, mount point but does not exist either on disk or in iostore. The uncooked file would be expected on disk at 'C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Content/DemoTemplate/_Core/MI_Intro_Colorway'. Perhaps it has been deleted or was not synced?`
   The BP still compiles (`BS_UP_TO_DATE`), so it is **not** a broken BP by this row's definition. It has **0 registry referencers**, but it is inside a COOKDIR, so by `PKG-§14` it cooks anyway (`TASK-1441`: LevelPrototyping 29→29 shipped).
2. **`/Game/Characters/IK_MeshyBiped` → HARD → `/Game/Characters/Anims/AB_Test/SK_Footman_Meshy`**: absent from the registry and from disk (`Content/Characters/Anims/AB_Test/` does not exist).
3. The other **13 are SOFT**, all animation/retarget assets under `Characters/`: 5 × `MM_*` → `/Game/Characters/Mannequins/Meshes/SKM_Manny` (+ `LS_*` EditableAnimations), 2 × `MF_Pistol_Idle_ADS_AO_*` → `.../SKM_Quinn`, and `RTG_MeshyBiped_to_SiegeBiped` → `SK_Footman_Meshy`. Neither `SKM_Manny` nor the targets are on disk.

Registry control for the sweep: `exists("/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement")` = True and `exists("/Game/DemoTemplate/_Core/MI_Intro_Colorway")` = False. **HYPOTHESIS (labelled):** these are leftovers of template or test content removed without fixing up referencers. Soft edges of this shape are usually editor-only preview references. **What each costs at runtime is NOT measured.** The shipped cook log does not show the `MI_Intro_Colorway` load error, so either it was present on 2026-09-09 or that log does not carry the line. It is unresolved. **The manager decides whether this is worth a row.**

## 5. (6) The ship-time gate: flagged, not designed

Verified at source: `A7-COOKDIRS` (`ship.ps1:1829-1836`) is `Test-Path` per entry and nothing more. It proves the eleven directories **exist** and says nothing about whether their contents compile. The cook gates after it, `C2-UAT-LOG`, match only `'BUILD SUCCESSFUL'` / `'BUILD FAILED'` (`Test-LogPattern` at `:2559-2561`). **Nothing in `ship.ps1` reads a `LogBlueprint: Error`, a `[Compiler]` line or the cooker's `Success - N error(s)` summary.** **Would a check have caught this class at the door?** Yes, for the `BS_ERROR` class. This row's own instrument is such a check: enumerate the Blueprint family under `$RECIPE_COOKDIRS`, load, read `Status`, fire a known-broken control. It ran over 51 Blueprints in about 2 s of editor time (log timestamps `00:22:53.997` → `00:22:55.710`, warm editor). The engine also ships a headless `CompileAllBlueprints` commandlet whose switches include `AllowListFile`, `IgnoreFolder` and `BlueprintBaseClass` (`CompileAllBlueprintsCommandlet.cpp:74-89`). And because the cooker compile-on-loads every Blueprint it cooks, **the cook's own log is a near-free instrument that the lane already produces and does not read.** **What it would cost:** the log route costs a pattern match, but only if the cooker's per-package lines survive into a log `ship.ps1` keeps (`TASK-1441` found the cooker abslog deleted, and the UAT-side log carries only the summary line). A separate pre-cook pass costs one more headless editor launch plus an allow-list derived from `$RECIPE_COOKDIRS`, and it needs its own known-broken control, or its zero is the `SC-§39` false zero this row warns about. **What it could NOT see:** a Blueprint that compiles clean but is wrong at runtime. A missing hard dependency that does not break compilation (§4's `BP_WobbleTarget` is exactly this, and it reads `BS_UP_TO_DATE` today). Broken non-Blueprint assets (materials, Niagara). Native C++ defects. And, for a COOKDIR-scoped pass only, Blueprints pulled into the cook by reference from outside the eleven (the `-map=` roots), which the cook-log route would still see. **The manager boards it if it is worth boarding (`SC-§101`).**

## 6. State at the end of the row = the end of the batched session

| Reading | Value |
|---|---|
| Editor | exactly 1 `UnrealEditor.exe`, PID **3108** (CreationDate 2026-09-26 23:05:31), the same GUI editor. Never closed or relaunched. |
| PIE | `is_active: false`. **Never started in this session.** |
| Level | `/Game/Maps/L_MainMenu.L_MainMenu` |
| `DIRTY_CONTENT` / `DIRTY_MAPS` | `[]` / `[]`. Nothing was saved and nothing is pending. |
| **`BROKEN_BP_LOADED`** | **True** (loaded by TASK-1439, re-touched here as the control) |
| Pack residency | `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']` only |
| In-memory Blueprints / in `BS_ERROR` | 103 / **1** (`BP_Basic_Movement`) |

⛔ **The editor is NOT PIE-safe.** By `PlayLevel.cpp:1270/1299/2659/1577`, the next PIE start in PID 3108 raises the blocking "unresolved compiler errors" modal. **It needs the `TASK-1538` relaunch**, and a fresh `BROKEN_BP_LOADED=False` reading, before any PIE row.

## Not examined / limitations

- **The `Status` field is the compiler's verdict at load.** Data-only Blueprints (none flagged here, but the `IsDataOnly` tag was not tabulated) go through a lighter compile-on-load path. A data-only child of a broken parent would still show the parent in `BS_ERROR`, and none is. Warnings-only (`BS_UP_TO_DATE_WITH_WARNINGS`) counted **0**.
- 26 of the 51 were already resident before the census. Their `Status` is from their compile-on-load earlier in this editor process (PID 3108, launched 2026-09-26 23:05), not from a fresh load. None had been edited in memory (dirty `[]` throughout).
- The **`-map=` roots** (`L_MainMenu`, `L_Arena`, both outside the eleven) and the Blueprints they pull in by reference from outside the eleven are **out of scope** (clause (2)). Their level-script Blueprints were not read. (I did not check whether `L_MainMenu` has one. If it does, it is resident, because the level is open, and it is not the one `BS_ERROR` in memory. `L_Arena`'s is not resident and was not examined.)
- **§4 is adjacent and undiagnosed.** Its runtime cost is not measured, and whether `MI_Intro_Colorway` existed at the 2026-09-09 cook is unknown.
- The historical cook-log sighting in §3 is uncontrolled (no known-broken BP was in that cook).
- Only uncooked editor packages were examined, not the packaged build.

## Routing

No QA row (zero code, zero asset). Returns to the **manager**. Count = 0, so **no 🚨 Blockers escalation** (the row's trigger is a non-zero count). §4 is flagged for the manager's judgment. Files touched: this handoff, and `TASKBOARD.md` (TASK-1445's `status:` line only).
