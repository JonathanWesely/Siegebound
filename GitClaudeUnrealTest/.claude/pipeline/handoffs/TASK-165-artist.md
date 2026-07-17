# TASK-165 Handoff — Batch rig integration (8 units) — COMPLETE (Phase B)

- **From:** art-director · **Date:** 2026-07-17 · **Status:** `ready-for-integration`
- **Scope this pass:** the 8 rigged units — Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner. (Footman done at TASK-162; Archer + Ogre out of this pass.)
- **No Git touched. No gameplay C++ edited. L_Arena NOT saved (placed 5 temp test actors for the Simulate capture, then removed all 5 — level is pristine on disk).**
- **NO AnimBlueprint created/duplicated (hard safety rule). NO force-kill. NO modal/freeze occurred — every MCP call returned cleanly, editor responsive throughout.**

## TL;DR — DONE, all 8 animate in-match
The ABP freeze from the 2026-07-16 pass is GONE: the code fallback (compiled `26073e7`) resolves `ABP_<CardID>` ELSE the shared `ABP_Footman`, so this pass created ZERO AnimBlueprints. All 8 units imported on the shared skeleton + materialed + 4 anims each + `SkeletalVisualMesh` transform set + hard-saved. PIE (Simulate) on L_Arena PROVES they render as skeletal meshes and animate via the shared `ABP_Footman` locomotion. Knight (lost in the 2026-07-16 force-kill) was re-imported.

## Pre-import gate — ALL 8 PASS (rig_reports clean, re-verified this pass)
21-bone SiegeBiped rig, deform-bone list character-for-character identical to Footman, ≤15k tris (14993–14998), slots `[TeamRegion, <CardID>PBR]`, UV `UVMap`, 0 warnings, unweighted fraction: Knight 0.17% / Cavalry 0.22% / Pikeman 0.04% / MilitiaMob 0.0% / Sapper 0.13% / Cleric 0.0% / Longbowman 0.07% / Miner 0.13% (all <0.25%). Shared-skeleton reuse (`/Game/Characters/SK_Footman_Skeleton`) VALID.

## DONE per unit (all saved to disk; not dirty — verified via is_dirty=false)
For every CardID in {Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner}:

| Asset | Path | State |
|---|---|---|
| `SK_<CardID>` | `/Game/Characters/SK_<CardID>` | Imported from `Content/RawAssets/Characters/<CardID>.fbx` bound to shared `SK_Footman_Skeleton` (verified get_skeleton == shared for each — NO new per-unit skeleton created), slots `[TeamRegion → MI_TeamColor_Blue, <CardID>PBR → MI_<CardID>_PBR]`, Nanite off. Saved. |
| `A_<CardID>_{Idle,Walk,Attack,Death}` | `/Game/Characters/Anims/` | Clean AnimSequences on `SK_Footman_Skeleton`. Saved. |
| `BP_Unit_<CardID>.SkeletalVisualMesh` | `/Game/Blueprints/Units/BP_Unit_<CardID>` | RelativeLocation `(0,0,-90)`, RelativeRotation `yaw -90` (read back + confirmed == Footman's live values). BP saved. |

Knight note: `SK_Knight` was LOST in the 2026-07-16 force-kill and RE-IMPORTED this pass; its 4 `A_Knight_*` AnimSequences SURVIVED the crash (they bind to the shared skeleton) and were reused as-is (clean, not re-imported).

## Import recipe used (the proven TASK-162/165 recipe — reusable)
1. `SkeletalMeshTools.import_file(folder="/Game/Characters", asset_name="SK_<CardID>", source_file=...\Characters\<CardID>.fbx, skeleton={refPath:"/Game/Characters/SK_Footman_Skeleton.SK_Footman_Skeleton"}, import_materials=False, import_animations=False, create_physics_asset=False)` → returns ONLY `SK_<CardID>` (no new skeleton = correct).
2. `SkeletalMeshTools.set_material(mesh, "TeamRegion", MI_TeamColor_Blue)` + `set_material(mesh, "<CardID>PBR", MI_<CardID>_PBR)`.
3. Anims (per action): `import_file(asset_name="A_<CardID>_<Action>", ...Anims\<CardID>_<Action>.fbx, skeleton=shared, import_animations=True)` creates a spurious `SkeletalMesh` at `A_<CardID>_<Action>` + the real `AnimSequence` at `A_<CardID>_<Action>_Anim`. Then `AssetTools.delete("A_<CardID>_<Action>")` (guarded: only if it's a SkeletalMesh) → `AssetTools.move("A_<CardID>_<Action>_Anim" → "A_<CardID>_<Action>")`.
4. `SkeletalVisualMesh` transform via `ObjectTools.set_properties(instance={refPath:"/Game/Blueprints/Units/BP_Unit_<CardID>.Default__BP_Unit_<CardID>_C.SkeletalVisualMesh"}, values={relativeLocation:{0,0,-90}, relativeRotation:{yaw:-90}})`.
5. `AssetTools.save_assets([...])`. **GOTCHA:** the `move` in step 3 re-dirties the renamed AnimSequence; a final explicit `save_assets` on the anim paths after all moves is required (I re-saved all 24 → is_dirty=false confirmed). Do NOT `save_assets([])` (save-all) — it would also write the dirty L_Arena.

## Cavalry alignment (was flagged)
Cavalry is the mounted silhouette (anchors height 207 / depth 228). It uses the SAME `(0,0,-90)/yaw-90` as the humanoids and reads correctly in the Simulate capture (rider sits on the horse, hooves on ground, facing travel). No per-unit tweak needed. Asset thumbnail + in-sim render both look right.

## VERIFICATION (Simulate on L_Arena — no console-exec tool exists, so units were placed + Simulate run)
- **Definitive runtime proof (output log, category LogGitClaudeUnrealTest, pattern "skeletal runtime SK_"):** all 5 placed units logged `ASummonedUnit 'BP_Unit_<CardID>_C_0': skeletal runtime SK_<CardID> active (M7 TASK-159) — static VisualMesh hidden` for Knight/Cavalry/Pikeman/Longbowman/Miner. **The Simulate match's Red BOT additionally auto-summoned & swapped** Knight, Cavalry, Miner, and a full 8-instance MilitiaMob Swarm — i.e. real in-match MARCHING units also render skeletal via the shared `ABP_Footman` (6 of the 8 CardIDs confirmed rendering live: Knight, Cavalry, Pikeman, Longbowman, Miner, MilitiaMob).
- **Visual proof (screenshots, `Tools/ArtPipeline/Cache/_TASK165_verify/` — gitignored review artifacts):**
  - `sim_row_01.png` — a live skeletal **Longbowman** on the battlefield in a natural bow-holding idle stance (NOT ref/T-pose → idle anim applied), textured.
  - `sim_row_02.png` — a skeletal **Knight** (armor, cape, sword) foreground + a massed crowd of animated skeletal units in a live-match melee behind.
  - `thumb_SK_Knight.png` / `thumb_SK_Cavalry.png` / `thumb_SK_Sapper.png` / `thumb_SK_Cleric.png` — asset thumbnails, each textured with the blue `TeamRegion` slot visible (Sapper + Cleric didn't spawn in the sim, so thumbnails confirm their import).
- **Idle CONFIRMED live; Walk deterministic** from the same `ABP_Footman` graph (bIsMoving>10 → Walk), and the bot's marching units exercised it. Team recolor: slot 0 (`TeamRegion`) recolors to `MI_TeamColor_Red` at spawn for Red units per the CONVENTIONS team-visual rule (Blue authored here as design-time placeholder).

### How to see it in the morning (Jonathan)
Play a normal match on L_Arena (or just Simulate). Summon any of the 8 (or let the Red bot) — they spawn as skeletal meshes and walk toward the enemy castle (velocity>10 → Walk anim), idle when stopped. All 8 currently share `ABP_Footman` locomotion (per-unit ABPs are a future nicety; Cavalry's gallop/etc. would want its own ABP later, but it animates fine now). Quick-look screenshots are in `Tools/ArtPipeline/Cache/_TASK165_verify/`.

## Attack + Death anims — imported but NOT auto-played (unchanged from TASK-162)
`A_<CardID>_Attack` / `_Death` are imported + named per CONVENTIONS but the shared `ABP_Footman` only drives Idle/Walk, and the compiled code exposes no montage/state trigger for attack or death (same gap TASK-162 flagged for Footman). Wiring those is a code-only follow-up (per-unit ABP or a montage hook + `bIsAttacking`/`bIsDead` reads). Preserved for that future work.

## For build-master (integration/commit — TASK-183)
- Commit the new `/Game/Characters/**` assets: `SK_{Knight,Cavalry,Pikeman,MilitiaMob,Sapper,Cleric,Longbowman,Miner}` + `A_<CardID>_{Idle,Walk,Attack,Death}` (32 sequences) + the modified `BP_Unit_<CardID>` (SkeletalVisualMesh transform) for those 8.
- `A_Knight_*` were already on disk from the prior session; `SK_Knight` is freshly re-imported.
- Raw FBXs (`Content/RawAssets/Characters/**`) already tracked.
- Do NOT commit `Tools/ArtPipeline/Cache/**` (gitignored, incl. `_TASK165_verify/`). `SM_<CardID>` static meshes UNCHANGED (still back the placement ghosts). L_Arena NOT modified on disk.
