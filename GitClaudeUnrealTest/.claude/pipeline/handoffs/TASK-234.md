# Handoff — TASK-234 — Sapper anim re-rig, HEADLESS HALF (art-director)

**Date:** 2026-07-21 · **Status: headless-done-pending-wire — re-rig attempt 1 PASS, WALK GATE PASS at the height-normalized floor (38.4 uu), all 4 clips retargeted. 17 credits (single attempt). NO editor/MCP touched; wire step queued behind the integration window.**

## 1) Root cause found (differs from the working hypothesis)

The TASK-231/223 hypothesis was "hunched mesh, hips low". The numeric + visual diagnostics (this task) show the real defect:

- The Sapper mesh is a squat dwarf **hugging a large spherical black bomb against his belly**. The bomb + wrapped gloves formed one frontal mass that defeated Meshy's limb detection.
- The TASK-223 rig's leg chain was fit INTO the bomb: thigh root at z 0.57 m sitting forward at the bomb (Y −0.35), knee at 0.77 m — **above the hip** — then foot at 0.16 m. Non-monotonic zigzag = "legs frozen" in Walk.
- Hips z-frac 0.308 itself was NOT the defect: gate-PASSING units also run low on these stocky bodies (Miner Hips 0.392, Knight 0.459; both with monotonic legs UpLeg > knee > foot, knee ≈ 0.20–0.25, foot ≈ 0.06–0.08 — the "good fit" signature used as this task's acceptance test).
- Also corrected for the record: TASK-223 did NOT use a 1.8 default height — rig_state.json shows `height_meters: 1.68` (the true manifest height). Height was never the lever; the API has **no pose hints at all** (rigging params are only `model_url`/`input_task_id`, `height_meters`, `texture_image_url` — verified against docs.meshy.ai). A spine derotation would not have fixed the leg tangle either.

## 2) Corrective preprocessing (the one cheap Blender pass)

**Bomb carve:** deleted the bomb + gloves from a COPY of the game-ready GLB used only as the rig upload (the rigged mesh is purely a motion source — nothing visual travels downstream):

- Mesh is TRELLIS triangle soup (2,780 loose shells) → connectivity can't isolate the bomb; used **(near-black texture color ∧ frontal window y < 0.10, 0.22 ≤ z ≤ 1.02) ∪ bomb-sphere r 0.36 @ (−0.06, −0.29, 0.62)**. 5,533 of 14,994 faces removed; height preserved 1.6837 m. Boots (below) and black hat/goggles (above) spared by the z-window.
- Result: silhouette reads head / torso / two forward arms ending at sleeve cuffs / two separated legs.
- Upload copy: `Tools/ArtPipeline/Cache/Sapper/rerig_t234/Sapper_gameready_nobomb.glb` (+ carve previews `carve2_*.png`). The shipping SM/SK and all Content meshes are untouched — the carve exists only in Cache.

## 3) Rig attempt 1 (only attempt needed; cap was 2)

- POST /openapi/v1/rigging, `height_meters` 1.68, carved GLB as data URI. Task `019f8699-c947-7423-a77b-897434869d9f`, 5 cr.
- **Fit verdict: PASS.** Hips 0.417 (was 0.308); legs strictly monotonic L 0.374→0.201→0.068, R 0.376→0.216→0.069 (matches the passing signature exactly); spine ascends 0.470→0.524→0.578; shoulders 0.69–0.70 (was 0.53). Evidence: `Cache/Sapper/rerig_t234/attempt1/` (rig FBX/GLB, `rig_state_t234.json`, renders) + scratch inspect JSON mirrored in the attempt dir's state.
- hip_ratio in the retarget normalized from the 1.68 outlier to **1.2386** (tgt pelvis 0.8669 m / src Hips 0.6999 m).

## 4) Clip choices (all on the new rig; 3 cr each)

| Clip | Preset (action id) | Task id | Note |
|---|---|---|---|
| Idle | Idle (0) | 019f869c-8a72-769e-abf9-e5592d0a01a6 | fleet standard |
| Walk | Casual_Walk (30) | 019f869b-c162-7d81-a99d-427bcfd100ac | fleet standard |
| Attack | **Charged_Ground_Slam (127)** | 019f869d-1c56-7dd5-a8a4-b671f6df2d92 | replaces Standard_Forward_Charge (510, root-only). Two-hand overhead slam: wind-up leap → airborne apex → downward slam → crouch recovery (stills `clip_Attack_f*.png`). The suicide-bomber read the spec asked for. No bomb/explode preset exists in the 594-preset library (checked). |
| Death | Dead (8) | 019f869c-ba08-74f9-b00a-5eac8dc194c1 | fleet standard |

## 5) Retarget + gate (SANCTIONED Blender path, exit 0)

`retarget_meshy_to_siegebiped.py --card-id Sapper --min-walk-foot-uu 38.4` (floor = 40 × 1.68/1.75 per the CONVENTIONS height-normalized amendment). Artifact amplitudes (uu, 9-sample max-axis):

| Clip | len (s) | pelvis | foot_l | foot_r | hand_l | hand_r |
|---|---|---|---|---|---|---|
| Idle | 4.000 | 62.49 | 39.00 | 63.64 | 83.89 | 99.32 |
| **Walk** | 4.200 | 8.88 | **42.94** | **39.81** | 25.06 | 33.10 |
| Attack | 3.000 | 68.78 | 48.08 | 59.60 | 77.03 | 153.13 |
| Death | 2.967 | 126.31 | 49.95 | 64.66 | 179.60 | 211.90 |

- **WALK GATE: PASS** — baked 43.04/39.93 and artifact 42.94/39.81 vs floor 38.4. Old failure was 5.36/11.81. Source-side feet went 5.4/11.8 → 37.8/36.9, proving the rig (not the retarget) was the defect.
- **foot_r margin is +1.4 uu = borderline class → the PIE visual verdict at wire is MANDATORY** (conservative-rollout law), not a formality.
- Attack is decisively full-body (every limb ≠ pelvis; 3.000 s) vs the old 0.633 s all-bones≡335.9 root travel.
- Machine report: `Tools/ArtPipeline/Cache/Sapper/retarget/retarget_report.json` (overwritten by this run).

## 6) Deliverables on disk (Git untouched — build-master commits later)

- **OVERWRITTEN** (same contract paths, tracked): `Content/RawAssets/Characters/Meshy/Sapper/Sapper_{Rigged,Idle,Walk,Attack,Death}.fbx` and `Content/RawAssets/Characters/MeshyRetargeted/Sapper/A_Sapper_{Idle,Walk,Attack,Death}_meshy.fbx`
- Provenance/evidence (Cache, gitignored): `Tools/ArtPipeline/Cache/Sapper/rerig_t234/` — carved GLB, carve previews, misfit diagnostics renders (`diag_t223rig_*.png`), clip stills, `attempt1/{rig_state_t234,anim_state_t234}.json` (task ids, shas, credit ledger). TASK-223's `meshy_anim/rig_state.json` left intact as the old rig's provenance.

## 7) WIRE STEP NEEDS (editor-serial, dispatch after the integration window)

1. Backups first: copy live `/Game/Characters/Anims/A_Sapper_{Idle,Walk,Attack,Death}` → `Backup_Procedural/` (procedural is live for Sapper; lengths Idle 2.5 / Walk 1.25 / Attack 1.667 / Death 2.0 s).
2. Same-path reimport-over of the 4 `MeshyRetargeted/Sapper/A_Sapper_*_meshy.fbx` onto **SK_Footman_Skeleton** at `/Game/Characters/Anims/A_Sapper_*` — t231_wire_fleet.py pattern: `enable_root_motion=False`, `force_root_lock=True`, embedded UE-side amplitude re-gate **with the 38.4 floor, NOT the default 40** (t231's script assumed 40 — must be parameterized or Sapper auto-fails).
3. RateScale: Death **1.5** (2.967→1.98 s ≤ the 2.0 s destroy hold, SummonedUnit.h:465 law). Attack: read Sapper cadence from DT_Cards in-editor (GDD: contact-exploder, likely fires once then dies); clip is 3.000 s — RateScale 2.0 (→1.50 s effective) is the recommended start, adjudicate vs cadence like TASK-231 §2. Idle/Walk 1.0.
4. **PIE visual verdict mandatory** (borderline Walk foot_r; also confirm the slam reads at game camera and the death collapse + prompt destroy).
5. Editor toast for `MeshyRetargeted/` source-watcher → **Don't Import** (TASK-230 §7 note).
6. Expect Idle to carry big sway (hand_r 99 uu — the TASK-229 hand-amplification quirk, accepted fleet-wide) and locked horizontal root travel (Idle 65.5 / Attack 48.5 / Death 113.8 uu max on the root bone, neutralized by force_root_lock).

## 7b) Wire attempt 2026-07-21 PM (away-window) — NOT STARTED, scripts staged

The away-window wire slot was consumed entirely by TASK-239's in-flight `recompile_material` (game thread blocked ~15:13→16:09+, see that handoff) — the wire lane needs the same game thread, so no import/gate/PIE was possible. Prepared and verified ready in the `764973cf` session scratchpad:

- `t234_backup.py` — duplicates live `/Game/Characters/Anims/A_Sapper_*` → `Backup_Procedural/` (skip-if-exists), reports live clip classes/lengths, and dumps the DT_Cards Sapper row's attack/cadence fields for the §7.3 RateScale adjudication. Run FIRST.
- `t234_wire.py` — t231_wire_fleet pattern parameterized for Sapper: **WALK_FLOOR = 38.4** (height-normalized, NOT 40), root_motion off + force_root_lock, RateScale Idle/Walk 1.0 / Attack 2.0 / Death 1.5, same-path reimport of the 4 `MeshyRetargeted/Sapper` FBXes onto SK_Footman_Skeleton, amplitude re-gate + ABP target-skeleton check, saves ONLY if all checks pass.
- Then: mandatory PIE visual verdict (borderline foot_r +1.4) via the `SummonTestUnit Sapper 0/1` console lane + SceneCapture (t231_live_summon/pie_rig2 recipes), captures to Cache, board flip only on visual pass.
- FBX artifacts verified on disk 14:38 today: `Content/RawAssets/Characters/MeshyRetargeted/Sapper/A_Sapper_{Idle,Walk,Attack,Death}_meshy.fbx`.

## 8) Credits

5 (rig) + 3×4 (clips) = **17 spent, single attempt** (cap was 2 rigs). Balance 2917 → 2900.


## 7c) WIRE EXECUTED 2026-07-21 evening -- DONE, PIE VISUAL PASS

Context: ran post-relaunch (editor PID 41012) after the wedge-kill of PID 24104 (Jonathan's explicit instruction; see TASK-239 handoff session-3 note).

1. t234_backup: 4 backups created at /Game/Characters/Anims/Backup_Procedural/A_Sapper_{Idle,Walk,Attack,Death}, saved. Live pre-wire clips confirmed procedural (2.5/1.25/1.667/2.0 s). DT_Cards JSON export API absent in 5.8 python -- cadence read from Docs/Data/cards.csv instead: Sapper Cadence=1.0, bSuicide=true, Damage 80, AoERadius 250, HP 60, Speed 500.
2. t234_wire: all 4 same-path reimports onto SK_Footman_Skeleton OK; root_motion off + force_root_lock on; RateScale Idle/Walk 1.0, Attack 2.0 (3.000 -> 1.50s eff), Death 1.5 (2.967 -> 1.98s eff <= 2.0s hold). UE-side WALK GATE: PASS 42.94/39.81 vs floor 38.4 (identical to the Blender artifact numbers). ABP_Footman target skeleton intact. UNIT_OK true -> all 4 SAVED.
3. RateScale adjudication (sec 7.3): cadence 1.0 = the explosion tick; at rate 2.0 the raw slam-impact frames (~2.0-2.4s) land at 1.0-1.2s effective, coinciding with the explosion. Kept 2.0.
4. PIE visual verdict (MANDATORY, borderline foot_r): PASS.
   - Walk: shots 00/01/02 -- upright torso, alternating stride poses, no skew/skid (marching at 500).
   - Attack+Death (suicide sequence, 0.05-dilation duel): wind-up crouch (shots 13, 20) -> lunge/slam-down (24) -> explosion at the 1.0s tick -> collapse falling (30) -> prone (38) -> destroyed by shot 41 = ~2.0s post-contact (numeric: contact t=158.7, gone at t=160.72). Prompt destroy confirmed.
   - Captures: Tools/ArtPipeline/Cache/Sapper/retarget/ue_previews/live_Sapper_00..44.png (shots 45-112 are extra walk frames from a missed-intercept take).
5. Gameplay observations for the programmer lane (NOT anim defects): (a) Sapper-vs-Sapper spawn-overlap contact resolves within one cadence tick -- summoning a duel cluster annihilates instantly; (b) a Sapper that kills a UNIT survives and marches on; the exploder death was only observed when its own suicide tick fired (C_5). If the GDD intends the exploder to always die on any attack, that is a code check, not art.
6. Editor toast: none appeared for MeshyRetargeted (fresh editor session; source FBXs unchanged since the 14:38 scan).

Board: TASK-234 -> done. Nothing committed (build-master lane).
