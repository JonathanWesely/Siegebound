# Handoff — TASK-232 — MilitiaMob wire under the height-normalized floor (art-director)

**Date:** 2026-07-19 · **Status: DONE — MilitiaMob is LIVE on Meshy full-body clips at unchanged paths, saved not-dirty. Biped fleet closes at 7/9** (Footman, Knight, Pikeman, Cleric, Longbowman, Miner, MilitiaMob; holds: Cavalry TASK-233, Sapper TASK-234).

## 1) What went live (same-path reimport-over, refs preserved, zero ABP edits)

| Asset (live path) | Source FBX | len (s) | RateScale | effective | root motion | root lock |
|---|---|---|---|---|---|---|
| `/Game/Characters/Anims/A_MilitiaMob_Idle` | `A_MilitiaMob_Idle_meshy.fbx` | 4.000 | 1.0 | 4.00 s | OFF | ON |
| `/Game/Characters/Anims/A_MilitiaMob_Walk` | `A_MilitiaMob_Walk_meshy.fbx` | 4.200 | 1.0 | 4.20 s | OFF | ON |
| `/Game/Characters/Anims/A_MilitiaMob_Attack` | `A_MilitiaMob_Attack_meshy.fbx` | 1.500 | **1.5** | **1.00 s** | OFF | ON |
| `/Game/Characters/Anims/A_MilitiaMob_Death` | `A_MilitiaMob_Death_meshy.fbx` | 2.967 | **1.5** | **1.98 s** | OFF | ON |

- All four bound to `/Game/Characters/SK_Footman_Skeleton`; imported with the proven TASK-230/231 recipe (FbxImportUI anim-only, FBXALIT_EXPORTED_TIME, replace_existing at the live path). `is_dirty=false` end-state; only these 4 + the 4 backups were saved (scoped saves, no save-all).
- **Attack 1.5 → 1.00 s effective = cards.csv cadence 1.0 EXACT** (Docs/Data/cards.csv MilitiaMob Cadence=1.0).
- **Death 1.5 → 1.98 s effective ≤ the 2.0 s destroy hold** (SummonedUnit.h:465 law, fleet norm).
- ABP_Footman target-skeleton verified intact before and after (a7a67f8/a7a77f6 lesson — no rebind needed).

## 2) Amplitude gate — PASS under the HEIGHT-NORMALIZED floor (the TASK-232 ruling, now CONVENTIONS law)

Floor = 40 uu × (1.49 m / 1.75 m) = **34.06 uu**. Height basis verified in-engine: SK_MilitiaMob bounds height readback = **148.8 uu ≈ 1.49 m**.

UE 9-sample AnimPose max-axis (uu), identical on scratch (T232_Stage) AND re-sampled on the LIVE paths post-wire:

| Clip | len | pelvis | foot_l | foot_r | hand_l | hand_r |
|---|---|---|---|---|---|---|
| Idle | 4.000 | 18.45 | 23.89 | 22.55 | 98.86 | 111.53 |
| Walk | 4.200 | 4.53 | **39.43** | **34.25** | 34.60 | 28.94 |
| Attack | 1.500 | 14.59 | 39.28 | 55.51 | 142.92 | 131.96 |
| Death | 2.967 | 71.49 | 52.30 | 46.35 | 147.52 | 177.70 |

- **WALK GATE: PASS 39.43/34.25 ≥ 34.06** — digit-for-digit the numbers the ruling adjudicated. Decisively not the ~7 uu root-only failure class (pelvis 4.53 with live feet).
- **L/R ordering preserved** (QA WARN-1 assert): baked foot_l > foot_r (39.43 > 34.25) matches source LeftFoot > RightFoot (42.16 > 34.93) — no mirror signature. PIE handedness confirms: pitchfork RIGHT hand, shield LEFT arm, consistent with the fleet.

## 3) PIE visual verdict — PASS (the borderline gate's conservative-rollout eyeball, mine)

Stage pass (transient SkeletalMeshActor in the PIE world, SK_MilitiaMob + looped scratch clips; all captures durable):

- **Walk: PASS** — genuine alternating short-unit stride, upright torso, feet planting, arms swinging with gait; no skew, no nose-down pitch, no lump-shuffle (the Sapper defect class), no mirror. Reads exactly as the ruling's "plausible short-unit stride". (t232_stage_Walk_02/03/04)
- **Attack: PASS** — full-body lunging thrust: legs stagger and drive, torso rotates into the swing. (t232_stage_Attack_05/06)
- **Death: PASS** — buckles to knees ~0.66 s, fully collapsed flat on its back by ~1.98 s; believable, fits the destroy hold at rate 1.5. (t232_stage_Death_02/04)
- **Idle: PASS** — restless-guard energy, same accepted amplification class as Knight/Cleric/Miner/Footman. (t232_stage_Idle_02)

Live pass (real `BP_Unit_MilitiaMob` units post-wire, PIE match running):

- March at 400 uu/s in swarm diamonds of 4 (the red AI organically played MilitiaMob during the session — wired clips animating match units). (rec_live_MilitiaMob_04, _08)
- Full-body attack cycling at the red castle wall with hit sparks. (rec_live_MilitiaMob_06/07)
- Death→destroy behavior observed 3× (Knight kills): corpse cleanly removed within the 2.0 s hold, no stuck bodies. Collapse frames are from the stage series (live kills completed between capture ticks).

## 4) Captures (durable, for Jonathan) — `Tools/ArtPipeline/Cache/MilitiaMob/retarget/ue_previews/`

t232_stage_{Walk_01..04, Attack_01..06, Death_01..04, Idle_01..02}.png + rec_live_MilitiaMob_{03..12, 18..26}.png (plus TASK-231's rec_stage_Walk_MilitiaMob_01..04 hold-era evidence).

## 5) Backups (created + saved BEFORE overwrite; TASK-231's wave did NOT cover MilitiaMob — verified absent first, no doubles)

`/Game/Characters/Anims/Backup_Procedural/A_MilitiaMob_{Idle,Walk,Attack,Death}` — procedural originals (verified: backup Walk len 1.25 s = procedural; live pre-wire was Idle 2.5 / Walk 1.25 / Attack 1.667 / Death 2.0). Backup_Procedural now holds 28 assets = 7 units × 4.

## 6) Editor/state end-audit

- `/Game/Characters/Anims/T232_Stage/` DELETED (in-memory scratch, never saved). **Dirty content packages: ZERO.**
- PIE ended; zero T232-tagged actors in the editor world; level NOT saved by me (nothing of mine needs it).
- Log sweep: clean for the wire — no anim/skeleton/import errors. One transient probe AttributeError (my script, fixed mid-run, no asset impact). Pre-existing WATCH (not this task, no material was touched): `LogSkeletalMesh: Material with missing usage flag` on SK_MilitiaMob/SK_Knight during PIE — the materials lack bUsedWithSkeletalMesh; cosmetic, editor auto-compensates; worth folding into an M7.5 polish line.
- Editor may show the source-content watcher toast for `MeshyRetargeted/` FBXs → **Don't Import** (same standing note as TASK-230 §7 / TASK-231 §4).

## 7) EXACT commit scope for TASK-235 (fold-in per its (e) clause)

- Modified (4): `Content/Characters/Anims/A_MilitiaMob_{Idle,Walk,Attack,Death}.uasset`
- New (4): `Content/Characters/Anims/Backup_Procedural/A_MilitiaMob_{Idle,Walk,Attack,Death}.uasset`
- NO new raw files: the 4 `Content/RawAssets/Characters/MeshyRetargeted/MilitiaMob/A_MilitiaMob_*_meshy.fbx` are already inside TASK-231 §7's 32-FBX scope item. `Cache/**` stays gitignored. L_Arena untouched by this task.
- Git observation for build-master (read-only look, I touched nothing): the 4 Backup_Procedural/A_MilitiaMob_* show as staged (`A `) in `git status` — verify index state before composing the commit, per the TASK-235 verify-first law.
