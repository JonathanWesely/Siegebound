# TASK-165 Handoff — Batch rig integration (8 units) — PARKED (editor modal-blocked)

- **From:** art-director · **Date:** 2026-07-16 · **Status:** `in-progress` / PARKED — needs Jonathan to dismiss an editor modal.
- **Scope this pass:** the 8 rigged units — Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner. (Footman done at TASK-162; Archer + Ogre are out of this pass.)
- **No Git touched. No gameplay C++ edited. L_Arena not modified.**

## TL;DR
Pre-import gate PASSED for all 8. Mesh + animation import is PROVEN and clean on the SHARED skeleton (Knight fully done: `SK_Knight` + 4 AnimSequences). The batch then hit a hard tooling wall on the **AnimBlueprint** step, and a `create` call left the **editor stuck on a modal dialog** — all MCP calls now time out. Jonathan must dismiss the dialog to unblock; and the per-unit-ABP step needs a decision (see "ABP problem" + "Resume options").

## Pre-import gate — ALL 8 PASS (rig_reports clean)
Every unit: 21-bone SiegeBiped rig, deform-bone list character-for-character identical to Footman, ≤15k tris, slots `[TeamRegion, <CardID>PBR]`, UV `UVMap`, 0 warnings, unweighted <0.25%. Shared-skeleton reuse (`/Game/Characters/SK_Footman_Skeleton`) is therefore VALID — confirmed in-editor that SK_Knight bound to it with deform bones matching Footman and NO skeleton pollution (both report the same 22-bone tree; the differing bone-0 label is the identity armature node and does not drive deformation). Cavalry note: it is a mounted silhouette (anchors height 207 / depth 228) — watch its `SkeletalVisualMesh` alignment when it gets wired; the Footman `(0,0,-90)/yaw-90` may need a tweak.

## DONE (saved to disk; build-master commits at integration)
| Asset | Path | State |
|---|---|---|
| `SK_Knight` | `/Game/Characters/SK_Knight` | Imported, bound to shared `SK_Footman_Skeleton`, slots `[TeamRegion→MI_TeamColor_Blue, KnightPBR→MI_Knight_PBR]`, feet-center (z 0→188.5), Nanite off. ✅ |
| `A_Knight_Idle/Walk/Attack/Death` | `/Game/Characters/Anims/` | Clean AnimSequences, all on `SK_Footman_Skeleton`. ✅ |

## PROVEN RECIPE (reuse for the remaining 7 + the anims are the same for Knight — mesh+anims are the easy, working part)
Per unit `<CardID>` (source FBX under `Content/RawAssets/Characters/`):
1. **Mesh:** `SkeletalMeshTools.import_file(folder="/Game/Characters", asset_name="SK_<CardID>", source_file="...\\Characters\\<CardID>.fbx", skeleton={refPath:"/Game/Characters/SK_Footman_Skeleton.SK_Footman_Skeleton"}, import_materials=False, import_animations=False, create_physics_asset=False)`. Slot names survive import (`TeamRegion`, `<CardID>PBR`).
2. **Materials:** `SkeletalMeshTools.set_material(mesh, "TeamRegion", MI_TeamColor_Blue)` + `set_material(mesh, "<CardID>PBR", MI_<CardID>_PBR)`.
3. **Anims (per action Idle/Walk/Attack/Death):** the anim FBX contains a skinned mesh + action, so `import_file(asset_name="A_<CardID>_<Action>", ...Anims\\<CardID>_<Action>.fbx, skeleton=shared, import_animations=True)` produces TWO assets — a spurious `SkeletalMesh` named `A_<CardID>_<Action>` AND the real `AnimSequence` named `A_<CardID>_<Action>_Anim`. Then: `AssetTools.delete("A_<CardID>_<Action>")` (the mesh) → `AssetTools.move("A_<CardID>_<Action>_Anim" → "A_<CardID>_<Action>")`. Verified end result = clean `AnimSequence` on the shared skeleton. (This is scriptable in one `ProgrammaticToolset.execute_tool_script` per unit — did exactly this for Knight.)

## THE ABP PROBLEM (the real blocker — needs a decision before the batch can finish)
The code resolves `/Game/Characters/ABP_<CardID>` per unit, so each unit needs its own AnimBlueprint on the shared skeleton, driving velocity→Idle/Walk (duplicate of the proven `ABP_Footman` graph, with the 2 sequence-player anim refs repointed to `A_<CardID>_Idle/Walk`).
- **What works:** `AssetTools.duplicate(ABP_Footman → ABP_<CardID>)` copies the full graph; and I CAN repoint the sequence players — `ObjectTools.set_properties` on `AnimGraphNode_SequencePlayer_1/_2` with `{"node":{"sequence":{refPath:...}}}` cleanly swaps the anim and preserves playRate 1.25 + loop (verified on ABP_Knight: Idle→A_Knight_Idle, Walk→A_Knight_Walk).
- **What's broken:** `AssetTools.duplicate` on an AnimBlueprint **STRIPS the TargetSkeleton** (verified on an untouched control `ABP_ZTest`: compiler error *"The skeleton asset for this animation Blueprint is missing"*). And **no exposed MCP tool can set an AnimBP's TargetSkeleton** — `ObjectTools` always redirects a blueprint ref to its CDO (`Default__ABP_..._C`), which has no `targetSkeleton`; the generated-class ref exposes only AnimInstance props. `BlueprintTools` has no skeleton setter.
- **What froze the editor:** `BlueprintTools.create(asset_type=AnimBlueprint)` (with `SK_Footman_Skeleton` pre-selected) opened a MODAL "Create Anim Blueprint" skeleton-picker and never returned (300s) — the editor game thread is now blocked, so ALL MCP calls time out. **Jonathan: dismiss/Cancel that dialog to recover the editor.**
- Note: `compile_blueprint` on an AnimBlueprint ALWAYS returns "failed to compile. Compile Errors: []" even for the known-good `ABP_Footman` — that specific empty-error result is a benign wrapper quirk; the REAL errors show in the output log (`LogsToolset.GetLogEntries category="" pattern=...`).

## RESUME OPTIONS (pick one; all need the editor unblocked first)
1. **Jonathan-assisted create (matches how TASK-162 likely authored ABP_Footman):** for each unit, `SelectAssets([SK_Footman_Skeleton])` → `create` AnimBlueprint (Jonathan clicks through the modal each time, or it may auto-accept when a fresh session isn't mid-block) → then build the Idle/Walk graph. Downside: modal per ABP + AnimGraph authoring headless is unproven.
2. **Child-AnimBlueprint / reparent (best headless candidate — TEST FIRST):** duplicate `ABP_Footman → ABP_<CardID>` (full graph, skeleton stripped) then `BlueprintTools.set_parent(ABP_<CardID>, parent_class=ABP_Footman_C)`. A child AnimBlueprint INHERITS the parent's TargetSkeleton — this may restore a valid skeleton without any setter tool. Then repoint the 2 sequence players (proven) + `compile` + verify via log. If a child ABP compiles clean, the whole batch becomes headless-scriptable.
3. **Programmer fallback (route to gameplay-programmer):** make the SkeletalVisualMesh swap fall back to a shared `ABP_SiegeBiped` (= today's `ABP_Footman`) when `ABP_<CardID>` is absent. All 8 animate on the shared skeleton, but with Footman's locomotion (Cavalry would look wrong) — acceptable stopgap only.

## REMAINING per-unit work (7 more meshes + all ABPs + all BP wiring)
- Meshes+anims (recipe above): Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner. (Knight mesh+anims already done.)
- ABPs `ABP_<CardID>` for all 8 (pending the ABP decision above).
- `BP_Unit_<CardID>.SkeletalVisualMesh` relative transform = `(0,0,-90)` loc / `yaw -90` (reuse Footman's; re-check Cavalry). Set via ObjectTools on the BP's inherited `SkeletalVisualMesh` component — NOT yet attempted this pass.
- PIE verify a spread (a humanoid + Cavalry + Miner): skeletal render, Idle/Walk, team recolor slot 0. Screenshots.

## JUNK to delete on resume
`/Game/Characters/ABP_ZTest`, `/Game/Characters/ABP_ZTest2` (if the modal create actually produced it), and `/Game/Characters/ABP_Knight` (skeleton-less duplicate — recreate it via the chosen ABP path). `SK_Knight` + the 4 `A_Knight_*` AnimSequences are GOOD — keep.

## For build-master
Nothing to commit yet from this pass beyond the Knight mesh+anims (and even those only after the ABP path is settled and the batch completes). Do not commit `ABP_Knight`/`ABP_ZTest*` (broken/junk). No level changes.
