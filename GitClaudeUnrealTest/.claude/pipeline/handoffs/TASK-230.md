# Handoff — TASK-230 — Footman live wiring via the Blender lane (art-director)

**Date:** 2026-07-19 · **Status: DONE — the 4 Meshy retargeted clips are LIVE at unchanged paths, PIE-verified (walk with legs, full-body attack, believable death, idle sway).** Jonathan's named example is on screen.

## 1) What went live

Same-path overwrite (reimport-over: same package + object name — every hard/soft reference preserved, zero ABP graph edits) of:

| Asset (live path) | Source FBX | len (s) | RateScale | effective | root motion | root lock |
|---|---|---|---|---|---|---|
| `/Game/Characters/Anims/A_Footman_Idle` | `A_Footman_Idle_meshy.fbx` | 4.000 | 1.0 | 4.00 s | OFF | ON |
| `/Game/Characters/Anims/A_Footman_Walk` | `A_Footman_Walk_meshy.fbx` | 4.200 | 1.0 | 4.20 s | OFF | ON |
| `/Game/Characters/Anims/A_Footman_Attack` | `A_Footman_Attack_meshy.fbx` | 3.000 | **1.8** | ~1.67 s | OFF | ON |
| `/Game/Characters/Anims/A_Footman_Death` | `A_Footman_Death_meshy.fbx` | 2.967 | **1.5** | ~1.98 s | OFF | ON |

- All four bound to `/Game/Characters/SK_Footman_Skeleton`; `ABP_Footman` target-skeleton verified intact (a7a77f6 lesson — no rebind needed, no graph edits). All saved; `is_dirty=false` end-state.
- **Attack 1.8** per spec (~1.67 s effective vs the combat window) — verified full swing cycles in PIE.
- **Death 1.5** — code fact: `ASummonedUnit::PlaySkeletalDeathAnim` defers Destroy by `min(GetPlayLength(), DeathAnimMaxHoldSeconds=2.0)` **uncompensated for RateScale** (SummonedUnit.h:465). At rate 1.0 the 2.967 s collapse was cut at 2.0 s; at 1.5 the full collapse (~1.98 s effective) completes and freezes exactly as the hold expires. Reads near-natural (deaths tolerate 1.5×).
- **Root-motion design honored:** the retarget kept horizontal root travel on the `root` bone (TASK-229 §3); `bEnableRootMotion=false` + `bForceRootLock=true` gives clean in-place playback — PIE-verified numerically: attacker's actor location bit-identical across consecutive attack-swing samples (zero slide), walk covers ground only via movement component at 400 uu/s.

## 2) UE-side amplitude gate — PASS (step-2 requirement)

9-sample AnimPose max-axis (uu), sampled on the imported assets in UE — identical digit-for-digit to TASK-229's Blender artifact stage (the numbers survived reimport exactly as QA predicted):

| Clip | len | pelvis | foot_l | foot_r | hand_l | hand_r |
|---|---|---|---|---|---|---|
| Idle | 4.000 | 5.16 | 10.05 | 21.01 | 71.59 | 109.44 |
| Walk | 4.200 | 7.16 | **61.59** | **52.79** | 31.52 | 35.70 |
| Attack | 3.000 | 125.21 | 37.57 | 210.45 | 103.65 | 205.12 |
| Death | 2.967 | 101.88 | 85.75 | 59.79 | 150.90 | 203.12 |

**WALK GATE: PASS — 61.59 / 52.79 ≥ 40 floor** (vs the 7.07 root-only failure signature). Source L/R asymmetry ordering preserved (61.59 > 52.79 mirrors source 56.97 > 50.34) — the QA WARN-1 soft mirror check holds. Same table re-sampled on the LIVE paths post-wire: identical.

## 3) Visual verdicts (the QA-adjudicated eyeball — mine)

Verified live in PIE on real `BP_Unit_Footman` units (SummonTestUnit cheat, Blue-vs-Red staged duels in open field), plus one Persona autoplay frame pre-wire:

- **Handedness: CORRECT, not mirrored.** The SK_Footman mesh carries spear in the RIGHT hand, shield on the LEFT arm (azimuth captures); in every live frame the right/weapon arm does the swinging — matching the amplitude signature (Attack hand_r 205 vs hand_l 104). Rig faces +Y; hand_r at −X as a +Y-facing right hand should be.
- **Walk: PASS** — pie_09/pie_10: genuine stride at 400 uu/s, opposite gait phases across frames, spear-arm swings with the gait, feet planted, no skew, no floating. RateScale 1.0 kept; no foot-slide read worth a change (in-place cycle + capsule speed).
- **Attack: PASS (the Jonathan acceptance)** — pie_11: mid-lunge with raised leg + full torso/spear swing (LEGS + ARMS); pie_41: both Blues with spears high overhead in the 2v1. In-place (no slide, see §1).
- **Death: PASS** — pie_45: the Red Footman slumps forward between the two Blues, head dropped, arm flopping — believable collapse; freeze-then-destroy at the 2.0 s hold with the full clip now inside it (rate 1.5).
- **Idle: PASS with the quirk accepted** — pie_15/pie_20: reads as a restless guard shifting stance. The TASK-229 quirk-2 hand amplification (hand_r 109 uu) lands as natural energy in-game, NOT flail → acceptance-hold, RateScale 1.0.
- **Frame+1 rebase quirk (TASK-229 quirk-1): clipped nothing** — imported lengths 4.000/4.200/3.000/2.967 match the Blender table to the millisecond.

## 4) Captures (durable) — for Jonathan

`Tools/ArtPipeline/Cache/Footman/retarget/ue_previews/`:
- **pie_41.png** — the showcase: 2v1 melee, both spears overhead
- **pie_11.png** — full-body lunge attack ·  **pie_45.png** — death collapse
- **pie_09/pie_10.png** — marching stride ·  **pie_15/pie_20.png** — idle sway
- persona_walk_frame12.png — pre-wire Persona proof (Walk frame 12, mid-stride)
- azim_pos*/neg*.png — mesh handedness reference (spear = right hand); pie_01..48 = full PIE session record

## 5) Backups (created + saved BEFORE overwrite)

`/Game/Characters/Anims/Backup_Procedural/A_Footman_{Idle,Walk,Attack,Death}` — the procedural originals (verified: backup Walk len 1.25 s = procedural). Disk: `Content/Characters/Anims/Backup_Procedural/*.uasset` (4 new files, untracked). Restore = delete live + duplicate back + rename, or reimport-over from a re-export.

## 6) Cleanup performed (per TASK-221-222 note)

- DELETED broken evidence exports `/Game/Characters/Anims/AB_Test/Src_{Idle,Walk,Attack,Death}1` + `Src_Walk_LiveSpike`.
- AB_Test otherwise INTACT (verified inventory): `Src_{Idle,Walk,Attack,Death}`, `SK_Footman_Meshy(+_Skeleton)`, `A_Footman_Meshy_{Idle,Walk,Attack,Death,Walk2}`.
- DELETED my scratch import folder `/Game/Characters/Anims/MeshyLive/` (its 4 assets were re-imported at the live paths; redundant).
- DELETED all transient level preview actors (T230_* SkeletalMeshActor/SceneCapture/lights) — verified zero remain.

## 7) Editor/state notes for build-master + Jonathan

- **Saved by me (only assets I created/changed):** the 4 live `A_Footman_*` (now Meshy), 4 `Backup_Procedural/*`. All `is_dirty=false`. ABP_Footman untouched/not dirty.
- **L_Arena is dirty** (transient preview actors spawned+deleted; also was already `L_Arena*` before I started). I did NOT save the level — Jonathan picks saves at close per standing law. Nothing of mine needs the level saved.
- **Editor toast pending:** "4 changes to source content files detected — Import?" = the auto-reimport watcher seeing the 4 new FBXs under `Content/RawAssets/Characters/MeshyRetargeted/`. Click **Don't Import** (my imports were explicit; an auto-import would use default settings).
- **Log sweep:** clean for the wire. The `GetSocketInfoByName` warnings were my own probe hitting the empty `CharacterMesh0` slot; `MoveToActor/NavMesh` warnings are the known TASK-015 class triggered by my off-lane teleports; one transient `Invalid animation skeleton` warning fired during evidence-asset deletion (cleanup window) — post-cleanup verification shows all four live clips + ABP correctly bound.
- **Commit scope (next build-master window):** modified `Content/Characters/Anims/A_Footman_{Idle,Walk,Attack,Death}.uasset`; new `Content/Characters/Anims/Backup_Procedural/` (4); deleted `Content/Characters/Anims/AB_Test/Src_*1.uasset` ×4 + `Src_Walk_LiveSpike.uasset`; plus the TASK-229 scope already noted there (tool + `MeshyRetargeted/` FBXs).
- Python remote execution was already enabled (in-memory, from TASK-221 §2) and remains so until editor restart.

## 8) Session learnings for TASK-231 (fleet loop)

- **The backgrounded editor ticks NOTHING editor-side** (single-node anim, poseable render flush, Persona autoplay, even Simulate-duplicated preview actors' anim instances) — `EditorPerformanceSettings` throttle props are not python-reachable in 5.8. **PIE is the only lane that animates unattended** — do per-unit visual gates directly in PIE with SummonTestUnit + a SceneCapture2D tracking rig (my `t230_pie_watch.py` pattern in the session scratchpad: teleport combatants to open field ~x=20000, SceneCapture → PNG per exec; AnimPose numeric gate stays editor-side, it needs no ticks).
- EditorAssetLibrary `load_asset`/`save_asset` are blocked during PIE — set asset properties via `unreal.load_object` and re-save after StopPIE.
- Anim-only FBX import recipe that works: `FbxImportUI` with `import_mesh=False, import_animations=True, skeleton=<SK_..._Skeleton>, mesh_type_to_import=FBXIT_ANIMATION, automated_import_should_detect_type=False`, `FBXALIT_EXPORTED_TIME`, `use_default_sample_rate=False`; `AssetImportTask.replace_existing=True` at the live path = clean same-path overwrite.
- Death RateScale law for the fleet: `effective = len / rate ≤ DeathAnimMaxHoldSeconds (2.0 s)` → per-unit `rate ≈ len/2.0` (clamp ≥1.0); Attack per-unit vs its cadence window as TASK-222 spec'd.
