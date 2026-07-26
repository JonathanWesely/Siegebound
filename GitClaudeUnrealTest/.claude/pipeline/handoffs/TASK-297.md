# TASK-297 handoff — Unit SK-LOD apply on MAIN (11 fleet meshes)

**Assignee:** build-master · **Status:** done · **Date:** 2026-07-25 · **Lane:** main (editor-python remote-exec + Git; NO compile, NO editor-close)

## What was done
Regenerated the LOD chains TASK-289 deferred for the **11 fleet skeletal meshes** (all LOD0-only before) — `SK_Footman, SK_Archer, SK_Knight, SK_Miner, SK_Cleric, SK_Ogre, SK_Sapper, SK_Pikeman, SK_Cavalry, SK_MilitiaMob, SK_Longbowman` at `/Game/Characters/SK_<CardID>`. Applied the CONVENTIONS SK-unit LOD law: **LOD1 50% @ screenSize 0.4 / LOD2 20% @ 0.15**. All 11 now report `lod_count = 3` and are saved.

## Mechanism (the wrinkle, resolved)
`regenerate_lod` is editor-python (`unreal.` API) and is NOT in any MCP toolset (`SkeletalMeshTools` has readback only; the ProgrammaticToolset sandbox blocks `import unreal`). Used the **UE Python Remote Execution lane** per the TASK-221 spike:
1. Flipped `bRemoteExecution=true` on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings` via MCP `ObjectTools.set_properties` (in-memory only; reverts on editor restart).
2. Drove `unreal`-API python through the engine's bundled `remote_execution.py` client (multicast 239.0.0.1:6766, bind 127.0.0.1) — runner reconstructed at `.claude/tmp/ue_exec.py` (transient scratch).
3. Per mesh: build a **transient** `SkeletalMeshLODSettings` with 3 `SkeletalMeshLODGroupSettings` (reduction + screen size) → assign to `sk.lod_settings` → `SkeletalMeshEditorSubsystem.regenerate_lod(sk, 3)` → **restore `lod_settings` to its original (None)**. The reduction bakes into the mesh's LODInfo and PERSISTS after clearing the settings ref (verified) — so NO companion LODSettings asset is created/committed. Saved via `EditorAssetLibrary.save_asset`.

## NIT correction (carried from TASK-288 QA)
The TASK-288 recipe/NIT said map `percent_triangles` → `number_of_triangles_percentage`. The actual UE 5.8 python property on `SkeletalMeshOptimizationSettings` is **`num_of_triangles_percentage`** (not `number_of_triangles_percentage`, which does not exist and errors). Set with `reduction_method = SMOT_NUM_OF_TRIANGLES` + `termination_criterion = SMTC_NUM_OF_TRIANGLES`, keep-fractions 0.5 / 0.2. (Recommend the recipe/CONVENTIONS note the exact property name.)

## URO flags — where they actually live
`VisibilityBasedAnimTickOption` + `bEnableUpdateRateOptimizations` are `USkeletalMeshComponent` properties — they have **NO home on the `USkeletalMesh` asset** (probed: both absent on the asset). They are already set at RUNTIME on every unit's `SkeletalVisualMesh` component by **merged TASK-285 C++** (`SummonedUnit.cpp:155-156`: `OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations=true`). So the URO outcome is satisfied by code, not by the 11 assets — nothing to set on them, and it keeps the commit scoped to the SK meshes. Verified active via PIE (units tick-optimized).

## Verification
- **Per-mesh readback (all 11): `lod_count = 3` (was 1).** Per-LOD vertex reduction confirmed — e.g. Footman 14142 → 9240 → 5283; Ogre 21899 → 12257 → 6638; Knight 18456 → 10859 → 5724. Vert fractions ~0.56–0.65 (LOD1) / ~0.29–0.37 (LOD2), consistent with the 0.5 / 0.2 **triangle** target (verts reduce slower than tris).
- **PIE on L_Arena:** Traversability CONFIRMED (0 culls); bot spawned + marched **8 distinct fleet unit types** (Archer, Cavalry, Footman, Knight, MilitiaMob, Ogre, Pikeman, Sapper) — LOD'd units render/animate/behave, incl. the largest (Ogre). Message Log clean: ensure / Accessed None / Fatal / InverseFast / non-invertible / real-NaN (`ContainsNaN`) all **0** (the `NaN` substring buffer-hits were "Nanite"/"resonance"/"maintenance" false-positives).

## Git scope
Commit (explicit pathspecs, NO push): the **11 `SK_*.uasset`** (`Content/Characters/`, LFS pointers) + this handoff + `TASKBOARD.md` (carries the manager's TASK-297 decomposition + M7.7 resume-on-main notes + the done status). **Deck-builder trio (`DeckBuilderWidget.{cpp,h}`, `WBP_DeckBuilder.uasset`) left UNCOMMITTED/parked; `.claude/tmp` scratch not staged.** Editor left running (remote-exec still enabled, reverts on restart). One remaining action across the local work: Jonathan's `git push`.
