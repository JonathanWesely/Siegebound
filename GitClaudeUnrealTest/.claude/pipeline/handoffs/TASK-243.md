# Handoff — TASK-243 — SK_Archer + SK_Ogre FIRST skeletal import + anim wire + runtime light-up (art, editor)

**Date:** 2026-07-22 (late evening session of 2026-07-21) · **Status: DONE — both units fully animated in PIE. Archer walks (Jonathan's finding CLOSED), bow-shot reads correct-handed, Ogre marches castle-ward ignoring units, heavy smash on structures, both deaths collapse + prompt destroy. No holds.**

## 0) TASK-242 reconcile (pre-step, orchestrator-directed)

TASK-242's agent died at wrap-up but its work was complete on disk — evidence-verified this session: both rigged FBXs at `Content/RawAssets/Characters/{Archer,Ogre}.fbx`, all 8 `A_*_meshy.fbx` at `MeshyRetargeted/{Archer,Ogre}/`, handoffs/TASK-242.md present, gates PASS. Board line already carried the accurate terminal status (`headless-done-pending-import`); flipped to **done** now that this task consumed its outputs. UE-side re-gate numbers match the 242 handoff digit-for-digit (62.07/54.18 and 84.17/81.17 — the dispatch's 62.34/54.37 / 84.58/81.42 variant readings were superseded by the artifact+UE agreement).

## 1) SK first-imports (NEW assets — the first-import lane, TASK-160/162 precedent)

| | SK_Archer | SK_Ogre |
|---|---|---|
| Path | `/Game/Characters/SK_Archer` | `/Game/Characters/SK_Ogre` |
| Bound skeleton | **SK_Footman_Skeleton (shared)** — skeleton NOT dirtied by bind = clean 21-bone merge, no missing-bones | same, clean |
| Slots | `[0] TeamRegion → MI_TeamColor_Blue, [1] ArcherPBR → MI_Archer_PBR` | `[0] TeamRegion → MI_TeamColor_Blue, [1] OgrePBR → MI_Ogre_PBR` |
| Height | 179.9 uu (measured 179.89) | 288.2 uu (measured 288.18) |
| Nanite / LODs / PhysAsset | OFF / LOD0-only / none — mirrors the fleet (verified: 8 of 9 existing SKs have no physics asset, all LOD0-only, Nanite off) | same |
| Verts LOD0 | 17,055 | 21,899 |
| Saved | yes, not dirty | yes, not dirty |

Import note: one benign `LogInterchangeEngine` warning per rigged FBX ("BindPose Matrix ... 2 different Matrices from FbxCluster vs FbxPose") — bind resolved correctly (thumbnails verified upright/textured/correct team regions; heights exact).

## 2) Anim first-wire (FIRST authoring — no backups by design; these units never had anims)

8 new sequences at `/Game/Characters/Anims/A_{Archer,Ogre}_{Idle,Walk,Attack,Death}`, all bound to SK_Footman_Skeleton, `EnableRootMotion=false` + `ForceRootLock=true`, saved not-dirty.

**UE-side amplitude re-gate (height-normalized floors per CONVENTIONS, parameterized — NOT the default 40):**
- **Archer WALK GATE: PASS 62.07 / 54.18 vs floor 41.1** (margins +21.0/+13.1)
- **Ogre WALK GATE: PASS 84.17 / 81.17 vs floor 65.8** (margins +18.4/+15.4)
- Both digit-identical to the TASK-242 Blender artifacts (tracks survived import exactly).

**RateScale adjudication vs Docs/Data/cards.csv:**
| Clip | len | rate | effective | adjudication |
|---|---|---|---|---|
| A_Archer_Attack | 5.000 | **2.5** | 2.00 s | Archery_Shot_1; numeric release-beat probe: max hand-snap at raw t=2.17 s → release lands at **0.87 s effective, inside the 1.2 s cadence** — the shot + follow-through show every cycle on continuous fire (Longbowman ships the same clip at 2.5). |
| A_Ogre_Attack | 1.833 | **1.2** | 1.53 s | smash lands on the 1.5 s cadence tick (TASK-242 §7.4 recommendation confirmed). |
| A_*_Death (both) | 2.967 | **1.5** | 1.98 s | ≤ 2.0 s destroy hold (SummonedUnit.h:465 law, fleet standard). |
| Idle/Walk (both) | 4.0/4.2 | 1.0 | — | fleet standard. |

Anim-FBX import warnings ("invalid bind poses → rebind using time zero pose", 4× per unit) = the known anim-only-FBX class; no mesh is imported from those files and the amplitude gate proves the tracks intact. Non-blocking.

## 3) Runtime light-up — PIE visual verdicts (all PASS, no holds)

`ASummonedUnit` auto-resolution picked both up with ZERO code/BP changes (composed-soft-path law): `mesh=SK_Archer/SK_Ogre`, `anim_inst=ABP_Footman_C` (shared-locomotion fallback), attack/death via single-node (`AnimSingleNodeInstance` observed during swings, locomotion ABP restored after combat — verified live on the winner of a duel).

- **Archer WALK (the Jonathan finding): PASS** — genuine leg strides at 350 uu/s, no glide (live_Archer_t243_00).
- **Archer ATTACK + handedness: PASS** — bow in LEFT hand extended, draw hand at head (Longbowman convention; the mesh carries the bow left — verified thumbnail + live), planted stance, team-tinted projectiles in flight both directions (t243_04/07).
- **Archer DEATH: PASS** — full collapse + prompt destroy ≈2 s (t243_22; destroy timing verified numerically twice).
- **Archer red recolor: PASS** — slot-0 TeamRegion reads red on a Red unit (t243_02).
- **Ogre MARCH + Siege profile: PASS** — walked straight past a red Archer parked in its path (x 19337→21036 at 250 uu/s, never stopped) then attacked the tower → castle. The red bot's own Ogres did the same organically all match (t243_01).
- **Ogre SMASH: PASS** — maul follow-through against the red tower (t243_21), gather at the castle wall (t243_25), single-node confirmed.
- **Ogre DEATH: PASS** — heavy stagger-back → crumple (t243_41/43, open-field kill via apply_damage at 0.25 dilation).
- **Ogre Idle long-arm sway (TASK-242 §7.5 watch):** at game camera reads as restless bulk-shifting, within the fleet-accepted quirk — no flag.
- Static SM_ fallback intact at `/Game/Meshes/SM_{Archer,Ogre}` (untouched, load OK); ghost lane untouched (uses SM_ per code, no changes anywhere near it); ABP_Footman target skeleton intact; **Message Log clean for both new units** (only the pre-existing Knight/Miner/Cleric missing-usage-flag trio warned, unchanged since TASK-163).

**Captures (durable, for Jonathan):** `Tools/ArtPipeline/Cache/Archer/retarget/ue_previews/live_Archer_t243_00..29.png` (00 march pair, 02 red recolor, 04/07 bow-draw duel + projectiles, 22 death collapse) and `Tools/ArtPipeline/Cache/Ogre/retarget/ue_previews/live_Ogre_t243_00..43.png` (01 march, 21/25 castle smash, 41/43 death collapse). SK thumbnails in the session scratchpad (t243_SK_{Archer,Ogre}_thumb.png).

## 4) Editor/state notes (build-master + Jonathan)

- **Saved by me (scoped, all not-dirty):** `SK_Archer`, `SK_Ogre`, the 8 `A_{Archer,Ogre}_*`. Skeleton/ABP untouched-not-dirty. L_Arena NOT dirtied (PIE-only transients).
- **Left dirty, NOT mine to save:** `MI_Archer_PBR` + `MI_Ogre_PBR` were dirty in-session (pre-dating my import per the import-time dirty-delta; likely the retexture-wave session or an engine-side usage-flag set). I only referenced them. Build-master: if git shows them changed at the TASK-244 window, fold them; else Jonathan's save toast decides.
- One PIE match ran to a natural red victory mid-verification (the bot demolished the blue castle while I staged captures) — restarted PIE for the Ogre castle-smash staging; nothing abnormal observed.
- Two full PIE sessions run + stopped; slomo reset to 1; no leftover actors (SceneCapture rigs were PIE-transient).

## 5) Commit manifest (rides TASK-244 — NO Git in my lane)

- NEW: `Content/Characters/SK_Archer.uasset`, `Content/Characters/SK_Ogre.uasset`, `Content/Characters/Anims/A_{Archer,Ogre}_{Idle,Walk,Attack,Death}.uasset` (10 files).
- Plus the TASK-242 scope already noted there (rigged FBXs `Characters/{Archer,Ogre}.fbx`, `Characters/Anims/{Archer,Ogre}_*.fbx` procedural exports, `Characters/Meshy/{Archer,Ogre}/`, `Characters/MeshyRetargeted/{Archer,Ogre}/`, `rig_manifest.json` edit).
- `Cache/**` stays gitignored (captures are review artifacts).

**TASK-244 readiness:** the commit window now owes — this SK batch (10 uassets + 242's raw FBXs), the Sapper anim wire output (TASK-234 §7c, done), MI_Castle_PBR, and the board/CONVENTIONS churn. All verified states are recorded in the respective handoffs.
