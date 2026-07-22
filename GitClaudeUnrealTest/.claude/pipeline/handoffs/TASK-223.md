# Handoff — TASK-223 — Meshy auto-rig + preset clips, remaining 8 rigged units (art-director)

**Date:** 2026-07-19 (overnight batch) · **Status:** COMPLETE — 8/8 units rigged, 4 clips each, 40 FBXs delivered, zero API failures. HEADLESS only (no editor, no imports — TASK-224 consumes).

## 1) Results table

| Unit | Rig task id | height_m | Attack clip (action_id) | Credits | Verdict |
|---|---|---|---|---|---|
| Knight | 019f7937-3064-7c35-86c8-c90f152b4912 | 1.89 | Right_Hand_Sword_Slash (219) | 17 | OK |
| Cavalry | 019f7939-3598-714e-bc63-e463c43aad1f | 2.08 | Thrust_Slash (240, couched-lance read) | 17 | OK w/ FLAG (§4) |
| Pikeman | 019f793b-3e93-7cf3-bc5a-10acb4f0290f | 1.89 | Thrust_Slash (240) | 17 | OK |
| MilitiaMob | 019f793e-192a-7dcf-9ee8-710754b01892 | 1.49 | Right_Hand_Sword_Slash (219) | 17 | OK (single figure, as manifest says) |
| Sapper | 019f7940-5697-7254-92ba-b006ac98f77f | 1.68 | Standard_Forward_Charge (510, run/charge flavor per spec) | 17 | OK w/ watch (§4) |
| Cleric | 019f7942-9b7e-73ee-999d-4a3bf3ce847e | 1.82 | Charged_Spell_Cast (125, heal-ish) | 17 | OK |
| Longbowman | 019f7944-95cf-7f19-83a1-95a92b959305 | 1.83 | Archery_Shot_1 (224) | 17 | OK |
| Miner | 019f7946-9702-7f6a-aa5e-d2dcf02170be | 1.73 | Charged_Axe_Chop (237, work/pick swing — NOT skipped, see §3) | 17 | OK |

Idle/Walk/Death for every unit: Idle (0) / Casual_Walk (30) / Dead (8) — the TASK-203 Footman picks. Rig 5 + 4×3 credits = **17/unit, 136 total; balance 3273 → 3137** (matches the board estimate). Per-clip animation task ids + shas: each unit's `rig_state.json` (§5).

## 2) Deliverables (exact paths)

- **FBXs (board contract, for TASK-224):** `Content/RawAssets/Characters/Meshy/<Unit>/<Unit>_{Rigged,Idle,Walk,Attack,Death}.fbx` — all 40 present, verified. Untracked, inert until the TASK-224 editor loop.
- **Provenance:** `Tools/ArtPipeline/Cache/<Unit>/meshy_anim/rig_state.json` per unit (TASK-203 schema: all task ids, input/output shas, credit ledger) + animated GLB mirrors alongside.
- **Rig-upload sources (rebuildable):** `Tools/ArtPipeline/Cache/<Unit>/meshy_anim/<Unit>_gameready.glb` (game-ready SM mesh + embedded T_<Unit>_D, same lane as TASK-203 Footman) + `Cache/gameready_batch_report.json` (measured heights, tris).
- **Rig probe evidence:** `Tools/ArtPipeline/Cache/rig_probe_report.json` (per-unit bones/roots/spine/vgroups/hip+foot placement).

## 3) Clip-choice rulings (recorded per spec)

- **Miner Attack = Charged_Axe_Chop (237):** spec said Attack optional, prefer a work/swing clip. The library has NO mining/pickaxe preset (checked); 237 is a downward two-hand chop = the closest pick-drive read (pairs with the S_MinerClink loop). Chosen over skipping.
- **Cavalry gait:** the library has NO horse/canter/gallop preset (checked — none exist). Walk = Casual_Walk (30) like everyone; Attack = Thrust_Slash for the couched-lance read.
- **Cleric:** NO heal-specific preset exists; Charged_Spell_Cast (125) is the heal-ish cast. Support never attacks in-match (rig_manifest flag stands), so this is a visual stand-in slot.
- Action ids verified by name against docs.meshy.ai/en/api/animation-library before spending (0/8/30/240 additionally ground-truthed by TASK-203).

## 4) Flags for TASK-224 (Ruling A conservative rollout)

1. **Cavalry = HOLD-BACK CANDIDATE.** The rig succeeded, but the probe shows Meshy fit the biped to the RIDER: Hips at z-frac **0.688** of mesh height (saddle line), lowest foot bone at **0.433** (stirrup level, inside the horse body) — every other unit sits at hips 0.31–0.53 / feet 0.05–0.09. The horse body is skinned to rider leg/hip bones, so Walk leg-swing will warp the horse. Preview FIRST at 224; expect to hold Cavalry back on procedural anims. (Also: the rigged GLB comes back 1.89 m vs the 2.08 m input — Meshy normalized during rigging; retarget goes through the IK rigs so scale is handled, recorded for completeness.)
2. **Sapper watch item:** Hips at 0.308 — the generated mesh reads hunched/low-slung; clips may exaggerate a crouch. Not a failure; eyeball at the 224 preview.
3. **Skeleton identity across all 8 = the Footman rig:** 24-bone Mixamo-style biped, root `Hips`, reversed-name spine `Spine02→Spine01→Spine`, no fingers, 24 vgroups — byte-for-byte the same naming TASK-203 recorded. **IK_MeshyBiped + RTG_MeshyBiped_to_SiegeBiped are reusable UNCHANGED for all 8 units.** The TASK-203 §5 headless-export FK defect + root-motion note (Attack/Death carry real translation) apply to this whole batch — the 221 spike verdict governs the export lane.

## 5) Method (repeat of the proven TASK-203 lane)

MESHY_TOKEN via HKCU user-env read (never echoed — meshy_generate.py redactor lane); bare TLS (no Norton MITM on api.meshy.ai, reconfirmed via `--check`). Per unit, sequentially (visibility order Knight→Cavalry→Pikeman→MilitiaMob→Sapper→Cleric→Longbowman→Miner): headless-Blender gameready GLB (SM FBX + D texture, measured height) → POST /openapi/v1/rigging (model_url data-URI, height_meters) → poll → POST /openapi/v1/animations (rig_task_id, action_id) ×4 → poll → download FBX+GLB. Runner scripts were transient scratchpad code reusing `meshy_generate.py` as a module (overnight ruling: scripts not committed tonight); the full call recipe is reproducible from rig_state.json + this note.

## 6) Downstream

- **TASK-224:** consume the 40 FBXs; reuse the TASK-203 IK/RTG pair as-is; preview-gate every unit, Cavalry first-to-hold; Miner's Attack is the mining swing (wire same as others — TASK-189 triggers only fire on attack ticks, and the Miner's attack cadence is the mining loop).
- **build-master:** everything new is untracked (`Content/RawAssets/Characters/Meshy/`, Cache is gitignored); nothing to commit until the anim lane ships units at 224/228.
- **Credits:** 3137 remaining — no constraint hit.
