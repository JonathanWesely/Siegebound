# TASK-1439 — BASIC-MOVEMENT-DIAGNOSE — programmer handoff

- **Agent:** gameplay-programmer, 2026-09-27, the first of the batched read-only session TASK-1439 → TASK-1443 → TASK-1445 (Jonathan approved the bundle first-hand in Claude Code).
- **Referencer count first:** **1 referencer, inside the pack (`GM_TestGamemode`); 0 outside the pack.** Across all 76 pack assets: **0** referencers anywhere outside `/Game/sA_ArcheryVfxPack/`.
- **First error line (verbatim):** `[Compiler] In use pin  Return Value  no longer exists on node  Is Head Mounted Display Enabled . Please refresh node or break links to remove pin.`
- **Diagnosis only.** Nothing was fixed, saved, compiled, deleted or moved. No PIE, no git, no code.

## 0. Editor and session state (`SC-§118`, `SC-§138`, measured at my own instant)

| Reading | Start of session | After this row |
|---|---|---|
| `UnrealEditor.exe` processes | exactly 1: PID **3108**, cmdline `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"` (the GUI editor, not a `-game` session) | same |
| `get_headless_status` | `editor_connected` | — |
| `is_pie_active` | `is_active: false` | not started (PIE forbidden) |
| Level | `/Game/Maps/L_MainMenu.L_MainMenu` | same |
| `DIRTY_CONTENT` / `DIRTY_MAPS` | `[]` / `[]` | `[]` / `[]` |
| `BROKEN_BP_LOADED` (`unreal.find_object(None, ".../BP_Basic_Movement.BP_Basic_Movement")`) | **False** | **True** (loaded on purpose, clause (1)) |
| Pack packages resident (`unreal.find_package` over all 76 registry entries) | `[]` | `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']` only |

⛔ **From this row on, the editor is NOT PIE-clean.** `BP_Basic_Movement` is resident in `BS_ERROR`, and by the mechanism in §4 the next PIE start in PID 3108 raises the blocking modal. It needs the `TASK-1538` relaunch (or any relaunch) and a fresh `BROKEN_BP_LOADED=False` reading before any PIE row.

## 1. The error text, verbatim

- **Instrument:** `unreal.EditorAssetLibrary.load_asset("/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement")` through `execute_unreal_python_readonly`. The load triggers compile-on-load. The compiler output was read from `Saved/Logs/GitClaudeUnrealTest.log` lines 4730–4745, a slice I delimited myself (baseline 4,729 lines before the load, 0 matches for `Basic_Movement`). No asset editor was opened.
- **Status read-back:** `Blueprint.get_editor_property("Status")` = `<BlueprintStatus.BS_ERROR: 2>`. Generated class: `/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.BP_Basic_Movement_C`.
- **Nobody had read it, confirmed:** `Basic_Movement` appears **0 times in every surviving log** under `Saved/Logs/` (all 12 editor logs and backups back to 2026-09-22, the `_2`/`_3` logs, the M8 logs and every suite log). So the error text had never been on disk in any log that still exists.

The full slice as logged, unedited, at frame `[994]`, `2026.09.28-00.12.20` (log clock):

```
LogStreaming: Display: FlushAsyncLoading(556): 1 QueuedPackages, 0 AsyncPackages
LogLinker: Warning: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: VerifyImport: Failed to find script package for import object 'Package /Script/XRBase'
LogBlueprint: Error: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] In use pin  Return Value  no longer exists on node  Is Head Mounted Display Enabled . Please refresh node or break links to remove pin.
LogBlueprint: Error: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] Could not find a function named "IsHeadMountedDisplayEnabled" in 'BP_Basic_Movement'.
Make sure 'BP_Basic_Movement' has been compiled for  Is Head Mounted Display Enabled
LogBlueprint: Error: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] In use pin  Return Value  no longer exists on node  Is Head Mounted Display Enabled . Please refresh node or break links to remove pin.
LogBlueprint: Error: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] Could not find a function named "IsHeadMountedDisplayEnabled" in 'BP_Basic_Movement'.
Make sure 'BP_Basic_Movement' has been compiled for  Is Head Mounted Display Enabled
LogBlueprint: Warning: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] Input Axis Event references unknown Axis 'MoveRight' for  InputAxis MoveRight
LogBlueprint: Warning: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] Input Axis Event references unknown Axis 'MoveForward' for  InputAxis MoveForward
LogBlueprint: Warning: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] Input Axis Event references unknown Axis 'LookUp' for  InputAxis LookUp
LogBlueprint: Warning: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] Input Axis Event references unknown Axis 'Turn' for  InputAxis Turn
LogBlueprint: Warning: [AssetLog] C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\sA_ArcheryVfxPack\Blueprints\BP_Basic_Movement.uasset: [Compiler] InputAction Event references unknown Action 'Jump' for  InputAction Jump
```

(The double spaces around `Return Value` and the node title are in the engine's message, not mine.)

**Counting it:** 4 error lines, but only **2 distinct errors**, each printed twice, plus 1 linker warning and 5 compiler warnings.

**Where the errors sit (graph read with `get_asset_graph`, strand `EventGraph`, on the already-loaded asset):** the EventGraph holds two unresolved call nodes, `K2Node_CallFunction_971` and `K2Node_CallFunction_243`. The inspector renders them as bare `K2Node_CallFunction`, with no function name, because the function did not resolve. Each one's `Return Value` feeds the `Index` of a `Select` node (`K2Node_Select_1` and `K2Node_Select_0`). Those Selects choose between the `FirstPersonCamera` forward/right vector (True) and the actor forward/right vector (False) for the two `AddMovementInput` calls (MoveForward / MoveRight). ⇒ **Two nodes × two messages = the four error lines.** That the duplication is per-node is an inference from the node count. It fits exactly but was not proven one-to-one.

**Asset shape (`get_asset_meta`):** parent `Character`. Variables `GunOffset`, `BaseTurnRate`, `BaseLookUpRate`, `UsingMotionControllers?`. Components `FirstPersonCamera` → `Sphere`. Events: legacy `InputAxis` MoveRight / MoveForward / LookUp / Turn and `InputAction` Jump ×2. Functions: `Construction Script` only. It is the stock first-person template character, shipped inside a VFX pack.

### 1a. What the error text points at: MEASURED chain, one link HYPOTHESIS

Measured (each read-only):
1. The linker line: `VerifyImport: Failed to find script package for import object 'Package /Script/XRBase'`.
2. `Engine/Config/BaseEngine.ini:1211-1212`: `; Refactoring most of HeadMountedDisplay engine module into XRBase plugin.` / `+ClassRedirects=(OldName="HeadMountedDisplayFunctionLibrary", NewName="/Script/XRBase.HeadMountedDisplayFunctionLibrary")`.
3. `Engine/Plugins/Runtime/XRBase/XRBase.uplugin:13`: `"EnabledByDefault": false`. The project's `.uproject` `Plugins` block (ModelingToolsEditorMode, StateTree, GameplayStateTree, ModelContextProtocol, EditorToolset, Terminal, ProceduralVegetationEditor, SiegeLlama, Aura) does not name XRBase, and a grep of the `.uproject` for `XRBase|HeadMounted|OpenXR` returns 0.
4. In PID 3108 right now: `find_package("/Script/XRBase")` = **False**, `find_object("/Script/XRBase.HeadMountedDisplayFunctionLibrary")` = **False**, `hasattr(unreal, "HeadMountedDisplayFunctionLibrary")` = **False**, and `find_package("/Script/HeadMountedDisplay")` = True.
5. The registry's recorded hard script dependencies of the asset: `/Script/InputCore`, `/Script/HeadMountedDisplay`, `/Script/NavigationSystem`, `/Script/ClothingSystemRuntimeNv`. That is the saved-time module; the redirect in (2) turns it into `/Script/XRBase` at load.

**HYPOTHESIS (labelled, `SC-§101`), not tested:** the two `Is Head Mounted Display Enabled` nodes are broken because their owning class `HeadMountedDisplayFunctionLibrary` now lives in the XRBase plugin (engine redirect), and this project does not enable XRBase. Their function never resolves, so the pin and the function lookup both fail. What would test it: enable XRBase and recompile, or remove the two nodes and recompile. Both are remedy acts, not mine to take. The five input warnings are a separate, non-fatal issue: the project defines no legacy Axis/Action mappings for those names. They are warnings, not errors, and on their own would not put the Blueprint in `BS_ERROR`.

## 2. Referencer census, asset registry (not grep)

**API used, named:** `unreal.AssetRegistryHelpers.get_asset_registry()` → `AssetRegistry.get_referencers(package_name, unreal.AssetRegistryDependencyOptions(...))` and `AssetRegistry.get_dependencies(...)`. Hard and soft were queried separately via `include_hard_package_references` / `include_soft_package_references` (`include_searchable_names=False`). `ar.is_loading_assets()` = **False**, so the registry scan was complete. For the whole pack I used `ar.get_assets_by_path("/Game/sA_ArcheryVfxPack", recursive=True)`, which returned **76** assets.

**Positive control (fired):** the forward edge of the one referencer, `get_dependencies("/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode", hard)`, returns `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']`. So the registry holds this edge in both directions, and the reverse query is not blind. (A first "control", `L_MainMenu`'s referencers = `[]`, is **not** a control: maps are normally unreferenced. I report it only so nobody re-reads it as one.)

| | Result |
|---|---|
| **(a) Who references `BP_Basic_Movement`: hard** | `['/Game/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode']` |
| **(a) soft** | `[]` |
| **(b) What it references: hard** | `/Script/InputCore`, `/Script/HeadMountedDisplay`, `/Script/NavigationSystem`, `/Script/ClothingSystemRuntimeNv`. **No `/Game` or `/Engine` content.** |
| **(b) soft** | `[]` |
| Upward transitive closure (hard+soft, walked to a fixed point) | `GM_TestGamemode` ← `/Game/sA_ArcheryVfxPack/Levels/LVL_Showroom`. Nothing references `LVL_Showroom`. |
| **(c) Any referencer outside `Content/sA_ArcheryVfxPack/`?** | **No.** Both nodes of the closure are inside the pack. |
| **(d) Does anything in `/Game` outside the pack reference the pack at all?** | **No. 0 of 76 pack assets have any referencer outside the pack.** The pack has 159 internal referencer edges. |

**Supplementary text instrument** (it does not replace the census, since a `.uasset` reference is not text; recorded because it covers paths the registry does not index):
- `grep -rni "sA_ArcheryVfxPack|GM_TestGamemode|Basic_Movement"` over `Config/` → 0, over `Source/` → 0, over `Plugins/` (binary-as-text) → 0.
- `grep -rl --binary-files=text -i "sA_ArcheryVfxPack" Content/`, excluding the pack → **0 files**. Positive control: the same grep inside the pack → **76** files (all 76 assets carry their own path), so the scan can see an ASCII path in a `.uasset`.
- Pack on disk: **77** files = 76 assets + 1 stray `Content/sA_ArcheryVfxPack/Materials/Textures/Noise10.png` (a loose source image, not an asset). That settles the board's "77 files" vs the registry's 76.

**Registry oddity (not relied on):** a query with both package flags off and only `include_*_management_references=True` returned the same set as the hard query. I did not chase it, and no finding rests on it.

## 3. Is it reachable at runtime?

| Path | Answer | Instrument |
|---|---|---|
| Placed in any level | **No level places it.** No map, and no external-actor package, is a registry referencer. `LVL_Showroom`'s own hard dependencies list `GM_TestGamemode` but **not** `BP_Basic_Movement` directly. `Content/__ExternalActors__/sA_ArcheryVfxPack` and `__ExternalObjects__/sA_ArcheryVfxPack` do not exist. | registry + `ls` |
| Reached by a GameMode | **Only by `GM_TestGamemode` (pack-internal)**, which only `LVL_Showroom` (the pack's demo map) references. HYPOTHESIS: `LVL_Showroom`'s World Settings GameMode override → `GM_TestGamemode` → its `DefaultPawnClass` = `BP_Basic_Movement`. The edges are measured; which property carries each edge is not (reading that would mean loading `GM_TestGamemode`, and I did not). The project's own game mode is `GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode` (`Config/DefaultEngine.ini:12`), with `GameDefaultMap=/Game/Maps/L_MainMenu` (`:10`) and `EditorStartupMap=/Game/Maps/L_Arena` (`:11`). | registry + config read |
| DataTable / Niagara asset | **No.** No DataTable or Niagara asset anywhere is a referencer (the only referencer is `GM_TestGamemode`). | registry |
| Soft class path | **No.** Registry soft referencers `[]`. Config/Source/Plugins text 0. Content binary-as-text outside the pack 0. | registry + grep |
| From `L_MainMenu` / `L_Arena` / the default game mode | **Not reachable.** | above |

⇒ **At runtime it is reachable only by opening `LVL_Showroom`**, a pack demo map nothing else references, so it is dev-only. This agrees with `TASK-1441` (0 of 76 pack files cooked).

## 4. Why does PIE see it? The mechanism is read from source; the tie to the TASK-1399 wedge is a HYPOTHESIS

Read from engine source (read-only):
- `Engine/Source/Editor/UnrealEd/Private/PlayLevel.cpp:1270`: `for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)`. The pre-PIE scan walks **every Blueprint in memory**, not the ones the level references.
- `:1299`: `else if (BS_Error == Blueprint->Status && Blueprint->bDisplayCompilePIEWarning)` → `ErroredBlueprints.Add(Blueprint)`.
- `:2659-2662`: `if (ErroredBlueprints.Num() && !GIsDemoMode)` → `ShowCompilationErrorsDialog(...)` → `:1577` `CustomDialog->ShowModal()`, with the text at `:1509`: *"Are you sure you want to Play in Editor? The following {0} Asset(s) have unresolved compiler errors."*
- `bDisplayCompilePIEWarning` is `UPROPERTY(transient)` (`Blueprint.h:466-469`), set to `true` on every compile, including compile-on-load (`Editor/Kismet/Private/BlueprintCompilationManager.cpp:372`, `:1509`). It is cleared only when the user clicks through the dialog (`PlayLevel.cpp:2682`), and only for that residency.
- **Measured live:** an `unreal.ObjectIterator(unreal.Blueprint)` walk with the same `Status == BS_ERROR` test finds **47** Blueprints in memory and **exactly 1** in `BS_ERROR`: `BP_Basic_Movement`. (`bDisplayCompilePIEWarning` itself is `protected` to Python and could not be read. Its value is inferred from the compile-on-load setter above.)

**HYPOTHESIS H-4 (labelled):** the `TASK-1399`-era wedge was this dialog. The editor puts it up modally at PIE start because `BP_Basic_Movement` was resident in `BS_ERROR`. The modal blocks the game thread, and everything else follows. **Not observed in this session** (PIE forbidden). The dialog's text was never captured during the wedge. It is consistent with every recorded symptom.

**The point the row asked me to carry:** the trigger is **residency**, not the sweep. Any act that loads `BP_Basic_Movement` re-arms it for the rest of that editor process: a Blueprint sweep, a Content Browser open of the asset, opening `LVL_Showroom` or `GM_TestGamemode`, a full-content op or a cook-in-editor, **and this row's own deliberate load**. The `TASK-1399` sweep is only what loaded it first. "Don't sweep" is not the fix: it only lowers the odds of one load path.

**Side notes, not remedies:** (i) the dialog's own "continue" clears the flag for that residency (`:2682`), so a human who clicks through once can PIE until the next load or recompile. That is a workaround for a human, not for the MCP lane, which cannot click it. (ii) `GIsDemoMode` suppresses the dialog. Neither is a remedy of record.

## 5. The three branches, costed. None chosen (`SC-§101`)

The measurements that change the costing: the referencer set is **exactly** `GM_TestGamemode` ← `LVL_Showroom`, all pack-internal. The error is **2 nodes of one call type** tied to a **disabled engine plugin**.

| Branch | What breaks | What it costs | Still unknown after this row |
|---|---|---|---|
| **(a) DELETE `BP_Basic_Movement`** | `GM_TestGamemode` loses its hard dependency (its pawn-class reference, per the §3 hypothesis), and `LVL_Showroom` plays with a null or default pawn. Both are pack-internal and dev-only. **Nothing outside the pack breaks** (0 outside referencers, 0 cooked). By `TASK-1440` (2)(b), a pack-internal referencer exists, so a delete is **off unless `GM_TestGamemode` (and, if the manager wants no dangling demo, `LVL_Showroom`) goes with it or is re-pointed**. Enumerated: those two, no others. | A content + git act: `.uasset` removal from a tracked path (`TASK-1399` row: `git ls-files --error-unmatch` succeeds). A redirector/fix-up pass if done in-editor. No human keystroke if done as a file delete, but the in-editor delete dialog would prompt about `GM_TestGamemode`. | Whether the `NS_Impact` donor work (below) wants the demo map or the character, which is unlikely since it wants FX. Whether `GM_TestGamemode` references anything else that would dangle (it was not loaded). |
| **(b) FIX the compile errors** | Nothing, if done right. Two variants the error text now separates, both the manager's to choose: **(b1)** enable the **XRBase** plugin in the `.uproject` (a config act, zero `.uasset` change, **if** the §1a hypothesis holds). It adds an engine plugin to the editor **and to every packaged build**, and it needs an editor restart. **(b2)** edit the BP: remove or replace the two `Is Head Mounted Display Enabled` nodes (`K2Node_CallFunction_971`, `_243`), e.g. feed the two `Select` indices a constant `false`, then Compile + Save. | (b1): one `.uproject` line + restart + a re-load to prove `BS_UP_TO_DATE`. It changes the plugin set of the shipping build, which is a `PKG-§` question. (b2): a Blueprint-authoring act carrying a 🧑 **human `Compile` + `Ctrl+S` keystroke** (a BP compile does not dirty the package; `save_assets` returns true and writes nothing), owed a **sha256-CHANGED gate** (`SC-§68`). The two error types are small and mechanical. The five input-mapping **warnings** would remain under either variant, since they are warnings, not errors. | Whether (b1) alone fully resolves `BS_ERROR` (the §1a hypothesis is untested). Whether XRBase has side effects on this project's input or rendering (not examined). |
| **(c) EXCLUDE the folder** | Nothing today. | The weakest option. It leaves a broken asset **committed** and **resident-able**: exclusion from cook or Aura does **not** stop an editor load, so the PIE modal (§4) survives any exclusion. By `TASK-1440` (2)(d) it is at most a stopgap. | Which exclusion list is meant: cook (already excluded by placement, `TASK-1441`), Aura index, or the Content Browser. None of them prevents residency. |
| ~~(e) MOVE~~ | — | Refused by `TASK-1440` (2)(e). Moving into a COOKDIR would ship a `BS_ERROR` class. Restated, not re-argued. | — |

⚠️ **Dependency surfaced, not assumed away:** the pack is named as a **read-only VFX donor** by boarded work. `TASK-174` (`NS_Impact`: "projectile/arrow impacts + trails draws on `Content/sA_ArcheryVfxPack/`"), whose status line reads "REVIVED 2026-07-21 … dispatch-ready", and `TASK-239` (`NS_Spell_Lightning`, "Donor `Content/sA_ArcheryVfxPack/` (READ-ONLY)"), which is closed-by-245. **The registry shows neither left a live reference** (0 outside referencers), so the donor use is by copy or re-author, not by reference. ⇒ "nothing references it" is **not** "nothing wants it", but what is wanted is the FX (Niagara, materials, textures), not `BP_Basic_Movement`, `GM_TestGamemode` or `LVL_Showroom`. A delete scoped to the one BP (or to the BP + game mode + demo map) leaves the donor surface intact. A folder delete would not. That is an inference from asset classes, not a reading of the donor rows' intent.

## Not examined / limitations

- **PIE not started** (forbidden). The modal mechanism in §4 is read from source plus a live in-memory census. The claim that this was the TASK-1399 wedge stays **H-4, a hypothesis**.
- **`GM_TestGamemode` and `LVL_Showroom` were not loaded**, on purpose, to keep pack loads to the one the row requires. So the property carrying each edge (World Settings override, `DefaultPawnClass`) is a hypothesis. The edges themselves are registry-measured.
- **The §1a cause (XRBase disabled) is untested.** Testing it is a remedy act.
- `bDisplayCompilePIEWarning` could not be read (`protected` to Python). Its value is inferred from the engine's compile-on-load setter.
- The registry covers what it indexes. Soft paths stored as plain `FName`/`FString` inside non-registry fields are covered only by the ASCII binary scan, and UTF-16 or compressed payloads would evade that scan. Config/Source/Plugins text is covered.
- Only the one error-producing asset was loaded. The other 8 pack Blueprints were not compiled or inspected (TASK-1445 scopes to the eleven COOKDIRs, and the pack is not one of them).
- Remedy chosen: **none** (`SC-§101`). The ruling is `TASK-1440`'s.

## Routing

No QA row (zero code, zero asset). Returns to the **manager → `TASK-1440`**. Files touched: this handoff, and `TASKBOARD.md` (TASK-1439's `status:` line only).
