# Handoff — TASK-231 — Fleet animation loop, RECOVERY RUN (art-director)

**Date:** 2026-07-19 · **Status: DONE — 5 units LIVE on Meshy clips (Knight, Pikeman, Cleric, Longbowman, Miner), 3 HELD (Cavalry preview-gated FAIL, Sapper broken retarget, MilitiaMob borderline gate FAIL).** Recovery of a stalled prior agent; its wire proved COMPLETE and law-compliant — nothing was half-wired, nothing rolled back.

## 1) Recovery findings (Phase 1 — evidence reconstruction)

The prior agent stalled ~13:42 during its post-wire live PIE watch of Longbowman (its scratchpad survived in this session's scratch dir; PIE was still running — I ended it). What it actually completed, all verified from disk + editor:

- **Headless retarget: ALL 8 units** (`Tools/ArtPipeline/Cache/<Unit>/retarget/retarget_report.json` + 32 FBXs in `Content/RawAssets/Characters/MeshyRetargeted/<Unit>/`, 13:05–13:07). Gate: 5 PASS / 3 FAIL (table below).
- **Backups**: `/Game/Characters/Anims/Backup_Procedural/A_<Unit>_*` for all 5 wire candidates, saved 13:15–13:17 (+ Footman from TASK-230). Procedural lengths preserved: Idle 2.5 / Walk 1.25 / Attack 1.667 / Death 2.0 s.
- **Wire: all 5 gate-passers in one batch at 13:37–13:38** (`t231_wire_fleet.py`): same-path reimport-over at `/Game/Characters/Anims/A_<Unit>_{Idle,Walk,Attack,Death}` onto SK_Footman_Skeleton, `enable_root_motion=False` + `force_root_lock=True`, per-unit RateScale, then an embedded LIVE-PATH UE amplitude re-gate — the script's save block only fires when EVERY unit passes; all 20 .uassets are on disk at 13:38, proving the in-editor gate passed for all 5.
- Its Slack claim ("Knight wired PASS, Pikeman wired PASS, starting Longbowman") was never posted to 🎨 — it described the post-wire PIE verification loop, which I completed/redid below. It also left `/Game/Characters/Anims/T231_Stage/` (24 unsaved scratch imports) — cleaned up (see §6).
- **No rollbacks needed**: audit showed every wired unit fully conformant (flags, rates, skeleton, saved, not dirty), held units untouched procedural, ABP_Footman bound to SK_Footman_Skeleton, no leftover actors, L_Arena not dirtied.
- **Stall class**: no UE-side hang reproduced; every call lane was responsive today. Mitigation used anyway: one-shot remote-exec connections per call (fresh connect/run/disconnect via `ue_exec.py`), short chunked execs, PIE captures via SceneCapture2D export per exec.

## 2) Per-unit results table

Amplitudes = UE-side artifact (9-sample AnimPose max-axis, uu); Walk-gate floor 40. Cadence from DT_Cards.

| Unit | Walk gate (foot_l/foot_r) | Visual verdict (PIE unless noted) | Outcome | RateScale I/W/A/D | Attack eff. vs cadence |
|---|---|---|---|---|---|
| Knight | **PASS 52.20/56.61** | full-body sword melee, right-handed, lunging legs (live_Knight_02 + prior agent's watch) | **LIVE** (13:38) | 1.0/1.0/**1.25**/1.5 | 1.20 s vs 1.2 s — exact |
| Pikeman | **PASS 55.84/45.08** | pikes leveled, marching in step, feet planted (live_Pikeman_04) | **LIVE** (13:38) | 1.0/1.0/**2.0**/1.5 | 1.50 s vs 1.5 s — exact |
| Cleric | **PASS 56.95/59.36** | staff idle sway (right-handed) verified live ×2 groups; walk + death-throe from stage series; support never attacks in-match | **LIVE** (13:38) | 1.0/1.0/**1.6**/1.5 | 1.67 s (n/a — no combat attacks) |
| Longbowman | **PASS 59.39/41.34** | double archery draw (bow LEFT hand, draw hand at head — correct), marching stride, death collapse + prompt destroy (rec_live_08/10/21) | **LIVE** (13:38) | 1.0/1.0/**2.5**/1.5 | 2.00 s vs 1.5 s — overshoot, see §4 |
| Miner | **PASS 51.91/47.01** | walk trek + two-handed overhead pick chop cycling at GoldNode (rec_live_03/08/11) | **LIVE** (13:38) | 1.0/1.0/**2.0**/1.5 | 3.83 s chop loop (cadence 0 — mining lane, reads deliberate) |
| Cavalry | **FAIL 37.94/36.19** | stage previews: horse body grotesquely deformed — biped Walk drives rider hip/leg bones the horse is skinned to (saddle-line Hips z-frac 0.688, TASK-223 §4 exactly as predicted). stage_Cavalry_Walk_s10 / Attack_s12 = evidence | **HELD — preview-only, NEVER wire** (explicit-pass-required stands; recommend: needs a quadruped rig/clip source, Meshy library has none) | — (procedural live) | — |
| Sapper | **FAIL 5.36/11.81** | stage preview: body pitches nose-down, legs frozen, shuffles as a lump (rec_stage_Walk_Sapper_04) — the hunched Meshy rig (Hips z-frac 0.308) breaks the aim-transfer rest-pose premise; hip_ratio outlier 1.68; Attack (0.633 s Standard_Forward_Charge) is pure root travel (all bones ≡ 335.9) | **HELD** (procedural stays; retry needs a re-rigged upright Sapper or per-unit chain corrections) | — | — |
| MilitiaMob | **FAIL 39.43/34.25** (borderline) | stage preview: plausible small stride (short 1.49 m unit); decisively not the 7 uu defect class, but under the codified 40-uu floor | **HELD** (gate law). Flag for manager: a height-scaled floor (40 × 1.49/1.75 ≈ 34) would pass it — adjudication + wire is a ~10-min follow-up if ruled | — | — |

Death for all 5 live units: 2.967 s @ 1.5 → **1.98 s effective ≤ the 2.0 s destroy hold** (SummonedUnit.h:465 law, uncompensated for RateScale — TASK-230 §1).

## 3) QA WARN-1 asymmetry assert (applied per qa/TASK-229-qa.md)

Applied across all 32 artifact clips: source foot_l/foot_r ordering preserved in most; ordering FLIPS found only on near-equal pairs (≤ ~10% split: Knight Walk 52.2<56.6 vs src 51.5>46.0; Cleric Walk; several Death clips). Adjudicated NOT mirrors: the chain map is name-to-name (structural), a real mirror would be global (Footman/Pikeman/Longbowman prove the axis convention), and PIE handedness confirms — swords/staves/pick consistently RIGHT-handed, bow LEFT-handed, per unit. Recorded as expected proportion-interaction noise of the FK transfer.

## 4) Notes / polish flags (non-blocking)

- **Longbowman attack overshoot**: 2.00 s effective draw vs 1.5 s cadence → the draw loop restarts ~75% through on continuous fire. Stills read clean; flagged for a polish pass (RateScale 3.35 would sync exactly, at the cost of a hasty-looking draw). Left at the wired 2.5.
- **Idle hand-amplification quirk** (TASK-229 quirk 2) lands as natural energy on Knight/Cleric/Miner, same as Footman — accepted.
- Editor may show the source-content watcher toast for `MeshyRetargeted/` FBXs → **Don't Import** (imports were explicit; same note as TASK-230 §7).

## 5) Captures (durable, for Jonathan) — `Tools/ArtPipeline/Cache/<Unit>/retarget/ue_previews/`

- **Longbowman**: rec_live_08 (double draw), rec_live_10 (stride), rec_live_19/21 (melee + death collapse at the red castle)
- **Cleric**: rec_live_04 + rec_live_20 (staff idle, two groups), stage_Cleric_Walk_s12 / Death_s20
- **Miner**: rec_live_03 (trek), rec_live_08 + rec_live_11 (chop phases at the glowing node)
- **Knight**: live_Knight_02 (2-Blue melee) · **Pikeman**: live_Pikeman_04 (pikes leveled march) — prior agent's, verified by me
- **Cavalry (hold evidence)**: stage_Cavalry_Walk_s10, stage_Cavalry_Attack_s12 · **Sapper (hold evidence)**: rec_stage_Walk_Sapper_04 · **MilitiaMob**: rec_stage_Walk_MilitiaMob_03/04 + foreground of rec_stage_Walk_Sapper_04
- Plus the prior agent's full stage series (24+ frames × 4 clips × 6 units) and live series.

## 6) Editor/state end-audit (all verified post-cleanup)

- `/Game/Characters/Anims/T231_Stage/` DELETED (in-memory scratch, incl. my MilitiaMob/Sapper doc-staging; never saved to disk). **Dirty content packages: ZERO.**
- All 20 live wired assets: correct lengths/rates/flags, bound to SK_Footman_Skeleton, `is_dirty=false` (disk mtime 13:38). ABP_Footman target-skeleton intact — no rebind needed (a7a77f6 lesson).
- PIE ended; zero T23*-tagged actors in the editor world; the gray figure near the blue GoldNode in Miner captures is the PIE default player pawn at PlayerStart, not a leftover.
- Log sweep: only benign AttributeError probes from the prior agent's scripts; no asset/skeleton errors.

## 7) Commit scope (next build-master window)

- Modified: `Content/Characters/Anims/A_{Knight,Pikeman,Cleric,Longbowman,Miner}_{Idle,Walk,Attack,Death}.uasset` (20)
- New: `Content/Characters/Anims/Backup_Procedural/A_{Knight,Pikeman,Cleric,Longbowman,Miner}_*.uasset` (20)
- New: `Content/RawAssets/Characters/MeshyRetargeted/<all 8 units>/*.fbx` (32 — raw-asset rule; the 3 held units' FBXs are provenance for the holds/retries) — Footman's 4 + the tool are already in the TASK-229/230 scope note
- `Cache/**` stays gitignored. L_Arena untouched by this task.

## 8) TASK-205 reconcile

TASK-205's scope (fleet rollout, same-path law, PIE verify, AB_Test cleanup) has been executed by TASK-230 + TASK-231 under Ruling A via the sanctioned Blender lane. Remainder = the three holds (Cavalry quadruped problem, Sapper re-rig, MilitiaMob floor adjudication) — recorded here, owned by future tasks if the manager issues them. TASK-205 annotated closed-by-reconcile on the board.
