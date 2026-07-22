# Handoff — TASK-203 — Meshy animation SPIKE: auto-rig + preset clips on Footman → IK Retargeter → scratch A/B (art-director)

**Date:** 2026-07-18 · **Status:** spike evidence COMPLETE, one defect FLAGGED (see §5 — the acceptance clause "retargeted clips play on SK_Footman" is met only partially: clips play, but carry pelvis-only motion until the flagged headless-export defect is resolved). Live assets untouched (proven, §7).

## 1) Meshy half (headless API — done clean)

- `--check` passed bare (no Norton MITM on `api.meshy.ai`; key resolved from HKCU `MESHY_TOKEN`, never echoed).
- **Auto-rig:** game-ready mesh (`Content/RawAssets/Footman.fbx` → GLB w/ D-texture at `Tools/ArtPipeline/Cache/Footman/meshy_anim/Footman_gameready.glb`, 15k tris, 1.7965 m) → Meshy rigging task `019f76d1-c028-76c5-88ba-1e6c560c8130` (`height_meters` 1.8). Result: clean 24-bone Mixamo-style biped (Hips root; Spine02→Spine01→Spine reversed-name spine; no fingers), same char mesh re-skinned (7488 verts, 22 vgroups).
- **Preset clips** (library picks matched to the Footman's `attack_style: thrust`):
  | Action | action_id | Preset | Frames @30fps |
  |---|---|---|---|
  | Idle | 0 | Idle | 120 |
  | Walk | 30 | Casual_Walk | 126 |
  | Attack | 240 | Thrust_Slash | 90 |
  | Death | 8 | Dead | 89 |
- **CREDITS FLAG (board premise wrong):** the clip library is NOT credit-free — rig 5 + 4×3 = **17 credits consumed** (balance 3290 → 3273). Fleet math at TASK-204/205 must include ~3 credits/clip/unit.
- Raw FBX checked in (untracked, for build-master): `Content/RawAssets/Characters/Meshy/Footman/Footman_{Rigged,Idle,Walk,Attack,Death}.fbx`. Animated GLBs + provenance (`rig_state.json`: task ids, shas, credit ledger): `Tools/ArtPipeline/Cache/Footman/meshy_anim/`.

## 2) Editor half — assets created (all NEW packages; live editor never mutated)

Method: `-run=pythonscript` commandlet as a SECOND headless process beside Jonathan's live editor — probe-verified safe for NEW-package writes only (the reimport tooling's "editor must be closed" law is about same-path OVERWRITES; nothing here overwrites). Live editor discovered everything via its directory watcher. `-SCCProvider=None -nosourcecontrol` used after run3 showed the commandlet's SCC integration auto-staging files (see §6).

| Asset | Path | Note |
|---|---|---|
| SK_Footman_Meshy (+ SK_Footman_Meshy_Skeleton) | `/Game/Characters/Anims/AB_Test/` | Meshy rig imported with its OWN skeleton — shared-skeleton law held (asserted in-script) |
| Src_Idle/Walk/Attack/Death | `/Game/Characters/Anims/AB_Test/` | source clips on the Meshy skeleton |
| IK_MeshyBiped | `/Game/Characters/` | root `Hips`; 9 chains (Spine=Spine02..Spine, Neck, Head, L/R Clavicle, L/R Arm=Arm..Hand, L/R Leg=UpLeg..Foot) |
| IK_SiegeBiped | `/Game/Characters/` | root `pelvis`; matching 9 chains on the SiegeBiped names (spine_01..spine_03 etc.) — REUSABLE for the whole humanoid roster |
| RTG_MeshyBiped_to_SiegeBiped | `/Game/Characters/` | 5 default ops (Pelvis Motion, FK Chains, Run IK Rig, Root Motion, Remap Curves); all 9 chains verified mapped per-op; pelvis op Hips→pelvis |
| A_Footman_Meshy_Idle / **Walk2** / Attack / Death | `/Game/Characters/Anims/AB_Test/` | retargeted onto SK_Footman (skeleton readback = SK_Footman_Skeleton on all 4) |

**Naming deviation:** the Walk clip is `A_Footman_Meshy_Walk2` — the canonical `..._Walk` filename is Windows delete-pending-wedged because the LIVE editor has that (older, static) package open in an anim tab I opened for playback verification. The stale `AB_Test/A_Footman_Meshy_Walk.uasset` on disk is a zombie: **delete it (and the tab) at TASK-205 scratch cleanup / after Jonathan closes the tab.**

## 3) UE 5.8 API learnings (retarget automation — feeds TASK-205)

1. A factory-fresh `IKRetargeter` has an EMPTY op stack in 5.8 (`get_num_retarget_ops()==0`); `auto_map_chains` is then a silent no-op. Must call `add_default_ops()` + `run_op_initial_setup(i)` + `assign_ik_rig_to_all_ops(SOURCE/TARGET, rig)`.
2. `duplicate_and_retarget` is DEPRECATED; `run_batch_retarget(FIKRetargetBatchOperationInputs)` is current, honors `target_path` (deprecated one dumps to /Game/ root), and does NOT auto-save outputs (deprecated one does) — save explicitly, save-first-verify-second.
3. `AnimPose` sampling (`AnimPoseExtensions.get_anim_pose_at_time/get_bone_pose`) is the reliable headless proof of clip motion — asset thumbnails/anim tabs are not.

## 4) Comparison captures (recorded paths — the TASK-204 eyeball material)

All under `Tools/ArtPipeline/Cache/Footman/meshy_anim/`:
- `AB_Footman_{Idle,Walk,Attack,Death}_procedural_vs_meshy.png` — row A shipped procedural contact sheet, row B 6-frame Meshy motion on the same Footman. Motion verdict is stark: e.g. Attack A = stiff spear raise vs B = full lunge-crouch-thrust arc. (Row B renders dark = the KNOWN recorded-dark albedo issue, not an anim matter.)
- `editor_walk_frameA/B.png` — retargeted clip open on SK_Footman in the live editor's anim preview.
- `editor_rtg_view.png` — RTG_MeshyBiped_to_SiegeBiped open in the live retarget editor: source+target aligned/grounded, chain rows visible.
- Amplitude tables (quantitative A/B): `rig_state.json` sibling logs + this file §5.

## 5) THE FLAG (verbatim, load-bearing for TASK-204/205)

**Headless IK-batch FK transfer defect:** every batch export (both APIs) produces clips whose per-bone world amplitude ≈ pelvis amplitude — pelvis/root motion transfers, FK limb rotation does NOT (retargeted Walk foot-minus-pelvis = −0.09 uu vs source-foot 57 uu). Everything inspectable is verified correct: rigs/chains/retarget-roots, per-op chain mapping (`LeftLeg<-LeftLeg` … by op name), FK op `chains_to_retarget` = 9 entries `enable_fk: True, rotation_alpha: 1.0`, no LogIKRig errors. Amplitudes (uu, max-axis over 9 samples):

| Clip | src Hips | src Foot | retgt pelvis | retgt foot |
|---|---|---|---|---|
| Walk | 7.1 | 57.0 | 7.16 | 7.07 |
| Attack | 124.1 | 183.7 (RFoot) | 125.2 | 126.0 |

Suspicion: 5.8 defect/init-gap in the commandlet (non-Slate) batch-export path. **Recommended next step (one human click):** open `RTG_MeshyBiped_to_SiegeBiped` (already open in a live tab), pick `Src_Walk` as preview — if the target follows live, the RTG asset is good and the UI's own Export Selected Animations button produces correct clips; the fleet lane (TASK-205) then needs the UI export or a repaired script path. Also note: Meshy Attack/Death contain real root-motion translation (~1.25 m thrust travel) — TASK-205 must decide in-place vs root-motion handling before overwriting live clips.

## 6) Incidents & cleanup ledger (all resolved except noted)

- Runs 3/4 auto-saved 8 broken stray packages to `/Game/` root (deprecated API + unsaved-source bug) AND the commandlet's source-control integration git-staged them. Cleaned: files deleted, `git reset` of exactly those 8 index entries (disclosed here; no other git action taken). Root cause eliminated via `-SCCProvider=None -nosourcecontrol` + `target_path`.
- Live editor toast "10 changes to source content files detected — Import?" is sitting in Jonathan's editor: **please click Don't Import** (they are the Meshy FBXs/new assets, already handled).
- Two asset tabs left open in the live editor (A_Footman_Meshy_Walk anim preview, RTG retarget editor) — intentionally not closed (Jonathan's editor; the RTG tab is exactly what TASK-204 wants to look at).
- `Tools/ArtPipeline/__pycache__` removed.

## 7) Live-asset verification (acceptance)

- MCP `is_dirty` = false for all 7 live assets (SK_Footman, SK_Footman_Skeleton, ABP_Footman, A_Footman_Idle/Walk/Attack/Death) — checked before AND after all work.
- `git status Content/Characters Content/RawAssets` → zero modified tracked files; only new untracked spike assets (listed in §2) + `Content/RawAssets/Characters/Meshy/`.
- Live `A_Footman_*` never overwritten; ghosts/ABP untouched; NO save-all ever issued; editor never closed.

## 8) Downstream

- **TASK-204 (Jonathan):** judge motion quality from §4 boards; resolve §5 flag with the one-click UI check; rule fleet GO/NO-GO + scope + credit budget (§1 flag).
- **TASK-205:** reuse IK_SiegeBiped + RTG as-is; fix export lane per §5; delete AB_Test + zombie Walk + Src_* at cleanup; remember clips cost 3 credits each.
- **build-master:** nothing to commit until the TASK-204 gate rules; all new files are untracked and inert.
