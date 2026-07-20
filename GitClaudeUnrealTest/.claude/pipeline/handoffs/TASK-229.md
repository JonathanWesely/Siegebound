# Handoff — TASK-229 — Blender-side retarget tool: retarget_meshy_to_siegebiped.py (gameplay-programmer)

**Date:** 2026-07-19 · **Status:** ready-for-qa — tool built, Footman processed end-to-end, **WALK GATE PASS** (exit 0).

## 1) What was built

`Tools/ArtPipeline/retarget_meshy_to_siegebiped.py` — headless-Blender (5.1.2, bpy+stdlib only, factory-startup) retarget tool implementing the SANCTIONED export path (CONVENTIONS M7.5 "UE-5.8 retarget-export defect + SANCTIONED export path"). Bypasses UE's broken `FIKRetargetBatchOperation` entirely. Sibling of `rig_character.py` (same CLI/exit-code/OutputGuard house style: 0 = OK+gate pass, 1 = stage/gate failure, 2 = usage/input error; `--check` / `--verify-only` / `--clips` / `--min-walk-foot-uu` / `--save-blend`; per-CardID operation, fleet-ready for any of the 9 units).

Invocation:
```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup ^
  --python-exit-code 1 --python Tools/ArtPipeline/retarget_meshy_to_siegebiped.py -- --card-id Footman
```

## 2) Method (constraint-based, per dispatch)

1. Import the unit's SiegeBiped rig (`Content/RawAssets/Characters/<Unit>.fbx`, armature `Footman_Rig`, 21 bones — mesh deleted) + the Meshy clip FBX (24-bone Mixamo-style, root `Hips` — mesh deleted). Scene fps pinned to 30 BEFORE the clip import so FBX key seconds land on exact source frames (Meshy presets are 30 fps — probe-verified: Walk = frames 1..127 = 4.200 s, matching the UE-side evidence to the millisecond).
2. Per mapped bone pair: a HELPER bone grown on the source armature (parented to the source bone, rest-oriented to `A @ tgt_rest_world`), then a **COPY_ROTATION constraint (WORLD→WORLD, REPLACE)** on the target pose bone following its helper. Net transfer: `tgt_world(t) = src_delta(t) @ A @ tgt_rest` — the FK rotation transfer the UE preview performs. Semantics per chain: `aim` (A = minimal rotation taking target rest bone direction onto source rest direction — target tracks the source bone's absolute world direction; proportion differences per-unit drop out) for all 19 anatomical bones; `delta` (A = identity) for Hips→pelvis (the Mixamo Hips bone's geometric direction is arbitrary — probe shows it pointing sideways).
3. **Translation ONLY on root/pelvis**, scaled by hip-height ratio (tgt pelvis rest Z / src Hips rest Z = 1.0087 for Footman): Hips world offset from rest split into HORIZONTAL → `root` bone (**root motion preserved in the FBX** — UE decides root-lock; never baked into pelvis) and VERTICAL → `pelvis` (walk bob + death collapse survive a root-lock).
4. BAKE: every frame sampled through the evaluated depsgraph, constraints **stripped**, captured world rotations + computed translations written back as plain FK keyframes (quaternion, hemisphere-continuity enforced) on a fresh action `A_<Unit>_<Clip>_meshy`. `bind_action` slotted-actions law (Blender 4.4+) honored on every action assignment.
5. Export: animation-only FBX (armature + action, NO mesh), settings mirroring `rig_character.export_anim_fbx` VERBATIM (axis_forward=-Z, axis_up=Y, apply_unit_scale, FBX_SCALE_NONE, FACE smoothing, no leaf bones, bake_anim step 1.0 simplify 0.0, primary/secondary bone axis Y/X, path_mode STRIP) with `object_types={'ARMATURE'}` as the single documented deviation (no mesh in the file). Armature object name re-asserted `Footman_Rig` (TASK-212 shared-root law).
6. VERIFY (the codified gate): 9-sample world-position max-axis amplitude in UE units (the TASK-203/221 spike method) on THREE stages: the SOURCE Meshy rig (control), the baked in-scene action, and the **re-imported exported FBX** (artifact). Walk gate: both feet ≥ 40 uu on baked AND artifact, else exit 1.

## 3) Chain map used (IK_MeshyBiped's 9 chains + retarget root; TASK-203/221-222 §3/§7)

| Chain | Meshy → SiegeBiped | Semantics |
|---|---|---|
| RetargetRoot | Hips → pelvis (+ scaled translation split root/pelvis) | delta |
| Spine **[REVERSED naming]** | Spine02→spine_01, Spine01→spine_02, Spine→spine_03 (Meshy Spine02 = LOWEST, Spine = chest) | aim |
| Neck | neck → neck_01 | aim |
| Head | Head → head | aim |
| L/R Clavicle | LeftShoulder→clavicle_l, RightShoulder→clavicle_r | aim |
| L/R Arm | Arm→upperarm, ForeArm→lowerarm, Hand→hand | aim |
| L/R Leg | UpLeg→thigh, Leg→calf, Foot→foot | aim |

Unmapped source: LeftToeBase/RightToeBase/head_end/headfront (no SiegeBiped counterpart). Unmapped target: `root` (carries horizontal root motion only, rotation held at rest).

## 4) Footman run — amplitude table (uu, 9-sample max-axis; gate floor 40 on Walk feet)

| Clip | len (s) | Stage | pelvis/Hips | foot_l | foot_r | hand_l | hand_r |
|---|---|---|---|---|---|---|---|
| Idle | 4.000 | source (Meshy) | 5.11 | 0.78 | 14.87 | 52.83 | 68.91 |
| | | baked | 5.16 | 10.05 | 21.01 | 71.59 | 109.44 |
| | | **artifact** | 5.16 | 10.05 | 21.01 | 71.59 | 109.44 |
| Walk | 4.200 | source (Meshy) | 7.10 | **56.97** | **50.34** | 22.29 | 22.37 |
| | | baked | 7.16 | **61.79** | **52.96** | 31.65 | 35.79 |
| | | **artifact** | 7.16 | **61.59** | **52.79** | 31.52 | 35.70 |
| Attack | 3.000 | source (Meshy) | 124.13 | 59.56 | 183.67 | 120.17 | 176.11 |
| | | baked | 125.19 | 37.60 | 210.41 | 103.68 | 205.10 |
| | | **artifact** | 125.21 | 37.57 | 210.45 | 103.65 | 205.12 |
| Death | 2.967 | source (Meshy) | 101.00 | 35.66 | 54.82 | 140.15 | 184.63 |
| | | baked | 101.88 | 85.73 | 59.75 | 150.95 | 203.07 |
| | | **artifact** | 101.88 | 85.75 | 59.79 | 150.90 | 203.12 |

**WALK GATE: PASS** — artifact feet 61.59 / 52.79 uu vs the 7.07 uu root-only failure signature (≈8× over it, and ≥ the 40 floor with margin). Cross-validation: the source-control numbers reproduce the spike's recorded evidence EXACTLY (Walk LeftFoot 56.97 / RightFoot 50.34 / Hips 7.10 = handoffs/TASK-221-222.md §7 verbatim), so this tool's sampling is provably the same method QA's evidence used. Clip lengths match the UE §7 table to the millisecond (timing preserved). Attack/Death pelvis ~125/~101 = the known ~1.25 m root-motion travel, now carried on the `root` bone (max horizontal root travel: Idle 4.7, Walk 8.5, Attack 132.0, Death 77.4 uu).

## 5) Outputs

- `Content/RawAssets/Characters/MeshyRetargeted/Footman/A_Footman_{Idle,Walk,Attack,Death}_meshy.fbx` (4 files, untracked)
- `Tools/ArtPipeline/Cache/Footman/retarget/retarget_report.json` (full machine report incl. chain map + per-clip amplitudes; Cache/ is gitignored)
- Validated: `py_compile` clean under Blender's bundled Python (`.../5.1/python/bin/python.exe`); `--check` exit 0; full run exit 0; write confinement verified via git status — only the tool file + `MeshyRetargeted/` are new, nothing outside RawAssets/Cache.

## 6) Quirks / QA-scrutiny pointers

1. **Frame re-base on FBX round trip:** baked frames 1..N re-import as 2..N+1 (Blender FBX time mapping). SPAN and duration are preserved exactly; UE imports by time, not frame index. Cosmetic.
2. **Idle hand amplitude amplified vs source** (hand_r 109 vs 69 uu): the Meshy Idle preset genuinely sways the arms; our longer arm chain (upperarm+lowerarm+hand ≈ 0.94 m vs Meshy ≈ 0.71 m) amplifies world-space arc under rotation transfer. Expected behavior of FK retargeting — eyeball-gate at TASK-230 judges the read.
3. **Attack foot_l 37.6 vs source 59.6:** the left (plant) foot moves less than the source's because leg proportions and the scaled root travel differ; the clip is decisively NOT root-only (every limb ≠ pelvis amplitude, foot_r 210). No skew: rotations are pure (LocRotScale with no scale term; hemisphere-continuous quaternions).
4. **.fbm side-folders:** Blender's FBX importer extracts embedded Meshy textures into `<clip>.fbm/` beside the INPUT FBXs. Those folders already existed from prior imports (TASK-223 era); the tool writes nothing there itself. Still inside RawAssets.
5. **Meshy source armature imports at object scale 0.01** — all math is world-space so this drops out; hip ratio computed from world heights (0.9255/0.9175 m — both rigs were fitted to the same mesh, so rest stances nearly coincide and the `aim` correction A is small everywhere it applies).
6. **fps law:** scene fps must be pinned to 30 BEFORE importing a Meshy clip (the SiegeBiped rig FBX carries 24 fps and would otherwise re-time key conversion). The tool re-pins after the rig import — do not reorder those calls.
7. **Cavalry (fleet note for TASK-231):** rider-fit hold-back stands (TASK-223 §4) — the tool will run on it, but hip ratio uses the saddle-line Hips (z-frac 0.688), so expect odd translation scaling; preview-gated regardless.

## 7) Downstream

- **TASK-230 (art-director):** import the 4 artifact FBXs onto SK_Footman_Skeleton (UE half of the gate: foot ≥40 uu after import, bone-name match, no skew), backups first, root-motion OFF + RateScale per the TASK-222 spec, same-path overwrite `A_Footman_*`.
- **TASK-231:** fleet loop — the tool takes any `--card-id`; run `--check` first per unit.
- **build-master:** new untracked: `Tools/ArtPipeline/retarget_meshy_to_siegebiped.py` + `Content/RawAssets/Characters/MeshyRetargeted/` — commit rides the next window after QA.
