# Handoff — TASK-242 — Archer + Ogre: rig_character.py skinning + Meshy clips + retarget (art, HEADLESS)

**Date:** 2026-07-21 evening · **Status: headless-done-pending-import — BOTH units complete. All gates PASS on attempt 1 each (rig budget was 2). 34 credits total (17/unit, low end of the 17–23 estimate). NO editor/MCP touched, NO Git touched, NO SK created (TASK-243 owns the first-import).**

## 0) Lane ruling honored

Both units skinned onto the shared **21-bone SiegeBiped** via `rig_character.py` with the `Footman_Rig` armature-root law (TASK-212) — NOT a Meshy-skeleton SK. Meshy was used for CLIPS ONLY (its rig is purely a motion source for the retarget). The rig_manifest `_doc` note that "Ogre may go bespoke" is overridden by the manager's lane ruling, recorded in the Ogre manifest entry.

## 1) rig_manifest.json — two entries AUTHORED (they were missing)

Per the file's per-unit pattern, heights from pipeline_manifest dims:
- **Archer**: `input_fbx` Content/RawAssets/Archer.fbx, SiegeBiped, weapon_side r, attack_style `thrust` (Longbowman precedent — the real bow-shot ships via the Meshy clip). 180 UE.
- **Ogre**: `input_fbx` Content/RawAssets/Ogre.fbx, SiegeBiped (lane ruling recorded), weapon_side r, attack_style `overhead` (else-branch overhead swing = maul read). 290 UE, roster's tallest.

## 2) SiegeBiped rigs (rig_character.py, headless, attempt 1 each)

| | Archer | Ogre |
|---|---|---|
| Armature root | `Footman_Rig` ✓ | `Footman_Rig` ✓ |
| Bones | 21 (SiegeBiped contract) | 21 |
| Skinning | **auto_heat**, 0.17% unweighted | **envelope fallback** (bone-heat failed on the shaggy-hide triangle soup — expected path, deterministic), 0.0% unweighted |
| Slots carried | `[TeamRegion, ArcherPBR]` + UVMap ✓ | `[TeamRegion, OgrePBR]` + UVMap ✓ |
| Height (measured) | 179.89 UE / 1.7989 m | 288.18 UE / 2.8818 m |
| Tris | 15,000 | 14,982 |
| Rigged FBX | `Content/RawAssets/Characters/Archer.fbx` | `Content/RawAssets/Characters/Ogre.fbx` |

Procedural anim FBXs also exported per the standard lane: `Content/RawAssets/Characters/Anims/{Archer,Ogre}_{Idle,Walk,Attack,Death}.fbx`. Walk contact sheets eyeballed: Archer strides cleanly with the bow deforming on-arm; Ogre stride reads (envelope skinning is blockier — the Meshy-retargeted clips are the real motion, PIE verdict at 243 judges the final read). Reports: `Tools/ArtPipeline/Cache/{Archer,Ogre}/rig/rig_report.json` + previews.

## 3) Meshy rigs — Sapper-lesson pre-checks applied, attempt 1 PASS both (cap was 2)

Pre-rig mesh eyeball: Archer = slim silhouette, thin bow at the side (Longbowman precedent) — no carve. Ogre = no Sapper-style frontal prop mass (the maul reads low/forward, limbs separated) — no carve. Upload GLBs: `Cache/<Unit>/meshy_anim/<Unit>_gameready.glb` (D-textured, heights 1.8 / 2.88 m; merged into `Cache/gameready_batch_report.json` without clobbering the fleet's 8 entries).

Post-rig fit inspection (the TASK-234 acceptance signature — legs strictly monotonic UpLeg>knee>foot + healthy Hips z-frac):

- **Archer** (task `019f8804-b247-7064-ad49-7a8541b7e82a`): Hips 0.531; legs L 0.477→0.262→0.060, R 0.477→0.261→0.059 — textbook PASS.
- **Ogre** (task `019f8804-ba5d-7af2-9652-04fa028ac113`): Hips 0.536; legs L 0.498→0.209→0.080, R 0.498→0.209→0.070 — PASS. Arm-chain rest positions read ambiguously in the numerics (hand bones toward the back half vs the forward maul grip), so before spending clip credits the **FREE `basic_animations` walking preview** was downloaded and rendered: legs stride with knee bends, the maul swings attached to its fist, the off-hand swings naturally — no pauldron/cape smearing. Motion evidence accepted the rig; no carve, no attempt 2. (New trick for the fleet playbook: the rig task's free walking preview is a zero-credit fit verdict.)

Evidence: `Cache/{Archer,Ogre}/meshy_anim/t242_evidence/` (inspect JSONs, bone overlays, free-walk stills, attack stills, the transient t242_* scripts) + `Cache/<Unit>/meshy_anim/rig_state.json` (full task ids, shas, credit ledger).

## 4) Clip picks (3 cr each; task ids in rig_state.json)

| Clip | Archer | Ogre |
|---|---|---|
| Idle | Idle (0) | Idle (0) |
| Walk | Casual_Walk (30) | Casual_Walk (30) |
| Attack | **Archery_Shot_1 (224)** — the proven Longbowman pick, per spec | **Heavy_Hammer_Swing (128)** — browsed from the preset library as the meatier grounded two-hand smash (vs the leaping Charged_Ground_Slam 127, kept as fallback). Stills verify: crouch gather → maul cocked overhead two-handed → full forward smash. |
| Death | Dead (8) | Dead (8) |

FBXs at the contract paths: `Content/RawAssets/Characters/Meshy/{Archer,Ogre}/<Unit>_{Rigged,Idle,Walk,Attack,Death}.fbx`.

## 5) Retarget + HEIGHT-NORMALIZED gate (sanctioned Blender path, exit 0 both)

Floors per CONVENTIONS (40 × height/1.75 from measured heights): **Archer 41.1 uu**, **Ogre 65.8 uu** (tall unit ⇒ floor well above 40, as ruled).

**Archer** — `--card-id Archer --min-walk-foot-uu 41.1` — **WALK GATE PASS** (artifact 62.07 / 54.18; margins +21.0/+13.1, NOT borderline). hip_ratio 0.9702.

| Clip | len (s) | root_travel | pelvis | foot_l | foot_r | hand_l | hand_r |
|---|---|---|---|---|---|---|---|
| Idle | 4.000 | 20.0 | 20.73 | 10.39 | 32.06 | 74.39 | 87.53 |
| **Walk** | 4.200 | 9.9 | 8.08 | **62.07** | **54.18** | 33.04 | 43.89 |
| Attack | 5.000 | 6.1 | 8.34 | 4.17 | 6.30 | 97.53 | 92.38 |
| Death | 2.967 | 84.4 | 105.31 | 83.36 | 62.70 | 158.98 | 224.50 |

Attack is a PLANTED bow shot (feet ~4–6, hands ~95 uu — the correct archery pattern, decisively not root-only).

**Ogre** — `--card-id Ogre --min-walk-foot-uu 65.8` — **WALK GATE PASS** (artifact 84.17 / 81.17; margins +18.4/+15.4, NOT borderline). hip_ratio 0.9604.

| Clip | len (s) | root_travel | pelvis | foot_l | foot_r | hand_l | hand_r |
|---|---|---|---|---|---|---|---|
| Idle | 4.000 | 47.5 | 36.90 | 56.98 | 55.57 | 268.93 | 258.20 |
| **Walk** | 4.200 | 14.8 | 14.20 | **84.17** | **81.17** | 62.84 | 68.95 |
| Attack | 1.833 | 70.0 | 43.01 | 96.75 | 86.41 | 281.10 | 243.90 |
| Death | 2.967 | 150.1 | 191.05 | 158.39 | 116.93 | 307.90 | 405.83 |

Artifacts: `Content/RawAssets/Characters/MeshyRetargeted/{Archer,Ogre}/A_<Unit>_{Idle,Walk,Attack,Death}_meshy.fbx` (armature `Footman_Rig`, 21 bones, re-import-verified). Machine reports: `Cache/{Archer,Ogre}/retarget/retarget_report.json`. Zero warnings both.

## 6) Credits

2900 → 2866 = **34 spent** (per unit: 5 rig + 4×3 clips = 17; single rig attempt each, cap was 2; the Ogre fit verdict used the FREE walking preview). Balance **2866**.

## 7) TASK-243 wire notes (first-import lane)

1. NEW `/Game/Characters/SK_{Archer,Ogre}` bound to the SHARED `SK_Footman_Skeleton` — the Footman_Rig root law makes the bind clean (no missing-bones warning expected; both rigged FBXs carry the exact 21-bone contract). Slots [0] TeamRegion → MI_TeamColor_Blue, [1] <Unit>PBR → MI_<Unit>_PBR. Nanite off.
2. Clips are FIRST authoring at `/Game/Characters/Anims/A_<Unit>_*` — no backups exist. root-motion OFF + force_root_lock (root_travel is carried on the root bone as designed — Ogre Death travels 150 uu, Attack 70 uu; the lock neutralizes it).
3. In-editor amplitude re-gate floors: **41.1 (Archer) / 65.8 (Ogre)** — parameterize, do NOT use the default 40 (the t234_wire.py precedent).
4. RateScale adjudication vs cards.csv cadence: **Archer Attack is 5.000 s vs cadence 1.2** — the bow-shot needs a hefty rate (start ~2.5–3.0 and adjudicate the draw-release beat vs the damage tick, TASK-231 §2 style). Ogre Attack 1.833 s vs cadence 1.5 — rate ~1.2 lands the smash on the tick. Death 2.967 s → rate 1.5 → 1.98 s fits the 2.0 s destroy hold (fleet standard) for both.
5. Expect the fleet-accepted Idle hand-sway quirk, AMPLIFIED on the Ogre's long arms (Idle hands ~260–270 uu) — PIE eyeball judges it; if it reads wild at game camera, flag rather than patch tools.
6. Jonathan's original finding was "Archer doesn't walk" — the PIE visual verdict on Archer Walk closes it.
7. Pre-existing dirty files note for build-master: `SM_{Archer,Ogre}.uasset`, `T_*.uasset`, RawAssets textures + `{Archer,Ogre}.fbx` modifications in git status belong to the TASK-200/202 retexture wave (TASK-241's commit), NOT this task. This task's NEW files: `Characters/{Archer,Ogre}.fbx`, `Characters/Anims/{Archer,Ogre}_*.fbx`, `Characters/Meshy/{Archer,Ogre}/`, `Characters/MeshyRetargeted/{Archer,Ogre}/`, + the `rig_manifest.json` edit.

Board: TASK-242 → headless-done-pending-import. TASK-243 is unblocked (queue behind the editor-priority ladder).
