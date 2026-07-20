# Handoff — TASK-211 — HOTFIX: Jonathan's 3 recurring editor Load Errors (art-director)

**Date:** 2026-07-18 · **Status:** ERROR 1 RESOLVED (verified). ERRORS 2+3 ROOT-CAUSED as STRUCTURAL — the spec'd fix (load meshes + save skeleton) is a proven no-op; needs manager re-scope. Zero mutations made; nothing dirtied, nothing saved, Jonathan's session untouched.

## Error 1 — `/Game/A_Footman_Meshy_Idle` stale ref: RESOLVED, no Jonathan click required

- **Registry/referencer check:** the package does not exist and has NO asset-registry record at all (`get_referencers` errors "Asset does not exist" — nothing on disk can reference it). Content root has ZERO loose files; `AB_Test/` holds exactly the 11 intended scratch assets.
- **Open-tab check:** live editor's open asset editors = `AB_Test/A_Footman_Meshy_Walk` (the wedge tab) + `RTG_MeshyBiped_to_SiegeBiped` only. **No tab holds the Idle stray** — nothing to close for this error.
- **Recurrence chain, reconstructed from the session log:** prior sessions had the zombie root-level file on disk (editor re-save after the TASK-203 spike delete); every load attempt failed on a GUID-mismatched dependency (old `SK_Footman_Meshy_Skeleton` GUID `89F045F5EFA596DD`). At **20:10:00 today** the file was deleted in-editor (`SourceControl: rm 'Content/A_Footman_Meshy_Idle.uasset'`); the single LoadErrors entry in today's Message Log is the residue of that deletion's own load pass — the LAST one possible. No LoadErrors entry for it has fired since, including through all of this task's activity.
- **Jonathan:** nothing to click. The Message Log line is historical; a fresh editor session comes up clean. (Standing TASK-203 note still applies: click **Don't Import** on any "source content files detected" toast.)

## Errors 2+3 — SK_Cavalry / SK_Knight missing-bones: STRUCTURAL, spec'd fix cannot work

**What the spec assumed:** transient bone merge succeeds in memory, was just never saved → load meshes, save `SK_Footman_Skeleton`.

**What the evidence shows (each step verified live):**
1. Skeleton BoneTree = **22 bones**, root hierarchy `Footman_Rig → root → pelvis → …` (`Footman_Rig` IS the skeleton's root bone).
2. SK_Cavalry + SK_Knight loaded at 20:38 and fired "will be added now" — yet BoneTree is STILL 22 with the meshes resident. Explicit `load_asset` of both changed nothing and `is_dirty` = false on skeleton and both meshes. **The merge announces itself and then silently fails, every load.**
3. Disk truth (uasset name-table scan): `Cavalry_Rig` exists only in SK_Cavalry, `Knight_Rig` only in SK_Knight, the skeleton has neither.
4. Blender inspection of `Content/RawAssets/Characters/Cavalry.fbx`: bone hierarchy is `root → pelvis → …` (21 bones, no `Cavalry_Rig` BONE) but the **armature OBJECT is named `Cavalry_Rig`** — UE's FBX importer converts the armature node into an extra ROOT bone above `root`. The Footman import (which CREATED the skeleton) rooted it at `Footman_Rig` the same way.
5. Therefore every unit mesh's root bone (`<Unit>_Rig`) differs from the skeleton root (`Footman_Rig`) → `USkeleton::MergeAllBonesToBoneTree` fails on root mismatch → **saving the skeleton can never contain these bones; the warning re-fires on every mesh load, forever.**

**Fleet-wide scope:** ALL 8 non-Footman units carry their own `<Unit>_Rig` (uasset scans: Pikeman/Cleric/Longbowman/MilitiaMob/Miner/Sapper/Cavalry/Knight, all written 7/17 00:34–00:40, all AFTER the skeleton's last save 7/16 02:59). Cavalry+Knight are merely the two that happened to load in Jonathan's session — any other unit will fire the identical error when it loads (e.g. in PIE).

**Why nothing is visibly broken:** the `<Unit>_Rig` wrapper bone never animates (identity transform, no skinning); by-name retargeting drives the 21 real bones. M7 anims QA-passed with this state. The errors are cosmetic load warnings — annoying, not damaging.

**Fix lanes for manager to rule on:**
- **(1) RECOMMENDED — proper fix, new task:** re-export the 8 rigged unit FBXs with the Blender armature OBJECT renamed to `Footman_Rig` (one-line change in the rig lane; bone hierarchy untouched), then in-editor same-path reimport of the 8 `SK_<Unit>` meshes against the existing shared skeleton. Mesh roots then match the skeleton root — no merge needed, warnings gone permanently, anims/ABP unaffected (bone names below the root are identical). Editor-serial; sensible to fold into or sequence with TASK-210.
- **(2) Accept-as-benign:** document the warning as known-cosmetic. Zero work, errors stay.
- **NOT viable:** load+save skeleton (proven no-op, above); hand-adding bones to a USkeleton (no API); renaming bones inside existing SK_ assets (no editor surface).

## Mandatory post-verify (all PASS, spec (c))

- **ABP_Footman:** TargetSkeleton = `/Game/Characters/SK_Footman_Skeleton` (asset-registry = saved state); generated class `ABP_Footman_C` alive with an active anim instance in a preview world; **zero** Blueprint/Compiler errors in the full session log; `is_dirty` false. No a7a77f6-style rebind needed.
- **Animation spot-check:** `A_Footman_Walk` renders the textured Footman correctly posed mid-stride (capture: scratchpad `A_Footman_Walk_thumb.png`) — skeleton/mesh/anim all resolve.
- **No-mutation sweep:** `is_dirty` = false on all 14 checked assets (SK_Footman, SK_Footman_Skeleton, ABP_Footman, SK_Cavalry, SK_Knight, live A_Footman_Idle/Walk/Attack/Death, AB_Test scratch incl. Walk2 zombie-sibling, RTG). No save issued (nothing was dirtied — "save ONLY what this dirties" = nothing). No tabs opened/closed, no PIE, no deletes, no Git.
- **Fresh LoadErrors check:** the session log contains exactly the 3 known entries (20:10 residue + 2× 20:38); this task added none.

## Acceptance reconciliation

- Error 1: **gone** (root cause deleted 20:10; verified unreferenced + registry-clean; residual Message Log line clears on next session; no click needed).
- Errors 2+3: **NOT clearable by the ticketed method** — diagnosis-changing finding, escalated. They will stay out of the Message Log only until the next SK_Cavalry/SK_Knight (or any unit) load. Manager decision required between fix lanes (1)/(2).

## Standing notes carried forward

- `AB_Test/A_Footman_Meshy_Walk` wedge tab + RTG tab remain open by design (TASK-204 wants the RTG tab; zombie Walk delete waits on the tab close, TASK-205).
- Nothing here to commit; rides TASK-210 or the follow-up fix task.
