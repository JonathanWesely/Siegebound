# TASK-319 — Cavalry FLEET-REMASTER (M7.9) — art-director handoff

> ## ⚠️ RECONSTRUCTED POST-REBOOT — read this first
> The 2026-07-26 ~16:58 PC reboot killed the Cavalry agent mid-run. This note was rebuilt on 2026-07-26 19:0x by a recovery art-director from the on-disk reports **plus direct measurement of the staged FBXs** (headless Blender 5.1.2 read-only probe). **Nothing was regenerated.**
> **Cavalry was flagged as the risk case — it was investigated hardest, and it came out clean.** Its rig log is truncated mid preview-render (last line = `seq/death/frame_0040`), but that is a **lost log buffer, not lost work**: every death frame 0000–0048 is on disk, all 8 preview stills are on disk, and the rigged FBX + LOD sidecar were written at 16:57:39 — a full ~20 s *before* the truncation point. The ONLY casualty was `rig_report.json`.
> **`Cache/Cavalry/rig/rig_report.json` was lost.** A **stale Jul-16 06:01 report describing a DIFFERENT mesh** was sitting at that path; it has been moved to `rig_report.STALE-PREREBOOT.json` and replaced with a clearly-labelled `_RECONSTRUCTED` report built from measured ground truth. Do not trust the stale file.
> **Integrity verdict: PASS** (no repair needed, nothing re-run). **But read the quadruped-rig finding in §"Skeleton bind target" before scheduling the in-engine verify — it is a real, known limitation that this rebuild does NOT solve.**

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Cavalry raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26 · **Branch:** main · Ran the PROVEN Footman pipeline (validated commit `d7254da`) with the brighter `albedo_delight` override.

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). Stage-1 consumed **30** credits. Meshy task `019fa0d5-3ec1-75dc-b0a1-7fad4ca5e39b`, engine `meshy-i23d`, endpoint `/openapi/v1/image-to-3d`, ran 23:49:17→23:51:54 UTC. Donor `Cache/Cavalry/meshy_raw.glb` (23,009,076 B, sha `604bb95e…`). Concept `Inbox/Cavalry.png` (1024×1024) **verified byte-identical** (sha256 `237c3642706aed06…`) to the approved `Content/RawAssets/Concepts/Cavalry.png`. Stage-2 + rig are local (no network).

## Brightness override (PINNED)
`pipeline_manifest.json` → `assets.Cavalry.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — **values are exactly the FLEET-DEFAULT** validated on the Footman pilot (`d7254da`). Confirmed `applied: true` in the bake.
Result: mean linear albedo **0.0468 → 0.1873** (p99 0.9219, shoulder-clamp 3.64%). **Independently re-measured from the shipped `T_Cavalry_D.png`: 0.1873** — exact match; the bake genuinely reflects the pin. Above the Footman validated-PASS baseline (0.164), but it is the **darkest of the recovered batch** — see the eyeball caveat.

> **Provenance nit (cosmetic, non-blocking):** Cavalry's manifest entry still carries the OLD `_note` from **TASK-225 (2026-07-19)** rather than a TASK-319 FLEET-REMASTER note like Archer/Sapper/Cleric got. The **numeric values are identical to the fleet default** — only the comment string is stale. No re-bake needed; worth a one-line manifest tidy at some later pass.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Cavalry.fbx` | Raw STATIC mesh (`SM_Cavalry`) | **15,000 tris** (budget 15,000 — at the cap); 7,492 verts; slots `[TeamRegion, CavalryPBR]`; `UVMap`; feet-centre (min_z **−0.018** UE); bounds `[94.94, 205.19, 208.05]` UE |
| `Content/RawAssets/Textures/Cavalry/T_Cavalry_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Cavalry/T_Cavalry_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Cavalry/T_Cavalry_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Cavalry.fbx` | Rigged SKELETAL FBX (`SK_Cavalry` + armature `Footman_Rig`, 21 bones) | slots `[TeamRegion, CavalryPBR]`; `UVMap`; auto_heat skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Cavalry.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` — **NEW untracked file, `git add` it** |

All 3 PNGs verified: 1024×1024, 8-bit RGB, **zero CRC errors, IEND present** (no reboot truncation).
**min_z is −0.018 UE** (0.18 mm below the origin plane) — the only unit in the batch that dips negative. Numerically negligible and well inside the feet-centre contract; systemic grounding (TASK-307/308) derives the capsule offset and absorbs it.

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Cavalry_{Idle,Walk,Attack,Death}.fbx` via `rig_character.py --no-anim-fbx`. Verified: the rigged mesh FBX carries **0 embedded actions**. In-engine `/Game/Characters/Anims/A_Cavalry_*` + shared ABP PRESERVED — do NOT reimport.

## How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Cavalry` → `Cache/Cavalry/meshy_raw.glb`.
- Stage 2: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Cavalry --input Cache/Cavalry/meshy_raw.glb` (12.1 s).
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Cavalry --no-anim-fbx` (killed by the reboot during its final report write; all outputs already on disk). Log: `Cache/Cavalry.rig.log` (truncated, but records every rig milestone before the cut).

## Skeleton bind target (RECORDED per the shared-skeleton binding law) — **READ THIS**

**ACTUAL skeleton bound to: `Footman_Rig`** — root bone `root` + the **20-bone SiegeBiped deform set**, byte-for-byte the same humanoid contract as Footman/Archer/Sapper/Cleric/Knight (verified by direct comparison). 20 vertex groups, all matching bone names, no orphan groups. → it will bind to the SHIPPED shared **`/Game/Characters/SK_Footman_Skeleton`**, so the existing `A_Cavalry_*` sequences keep binding and nothing is orphaned.
**This is a BIPED skeleton on a MOUNTED/QUADRUPED mesh.** That is the correct call under the shared-skeleton binding law (a bespoke horse skeleton would orphan the anims), but it is also the honest limitation below.

Measured skin: **0 unweighted verts (0.0000%)**, 0 zero-sum verts, max **10** influences/vert (within UE's 12 default).

### ✅ What is FIXED vs the prior Cavalry rig
The historical defect was that the rider-fit rig **warped the horse**. **That defect is gone:**
- The rigged FBX's bind-pose bounds are `[94.94, 205.19, 208.05]` — **identical to the static mesh's bounds to the centimetre**. The bind introduces **zero** geometric distortion.
- Bind previews (front / side / three-quarter) show the horse fully intact: four legs, barrel, neck, head, tail and the rider all read correctly, no stretching, no collapse, no holes.
- 0.0% unweighted across 7,492 verts.
**The mesh and the bind are sound. It will import and display correctly.**

### ⚠️ What is NOT fixed — the biped-over-quadruped rig (quantified)
The rig is anatomically wrong for a horse, and no amount of re-rigging on `Footman_Rig` can fix that. Measured weight distribution:

| Metric | Cavalry | Archer (healthy ref) | Sapper (healthy ref) |
|---|---|---|---|
| Weighted mean bone-influence drift | **26.6 cm = 12.8% of height** | 15.0 cm = 8.3% | 16.7 cm = 9.9% |
| Top-3 bones' share of total weight | **50.4%** (`thigh_l` 18.3 + `pelvis` 16.7 + `thigh_r` 15.4) | 34.5% | 31.2% |
| `thigh_l` influence Z-span | **0.1 → 173.9 UE** (nearly the whole mesh) | 12.7 → 129.7 | 8.1 → 130.5 |
| `head` bone weight share | **0.69%** | 4.49% | 1.12% |

Read plainly: **the two thigh bones and the pelvis own half the entire mesh** — they are covering the horse's whole barrel, and the horse's four legs are shared between `calf_*`/`foot_*` (≈9% of weight) which sit at *biped* leg positions, not at the horse's four leg roots. `head` owns almost nothing because the rider's head is tiny relative to the horse.

**Consequence:** in bind pose and in the low-amplitude Idle it reads fine (confirmed in the walk-frame renders — the horse holds together and does not shatter). Under a real locomotion clip the horse will read as a **stiff sliding body that flexes at the rider's hip** rather than a horse gait. It will not look broken; it will look wrong-but-stable.

### 🔴 It still needs a quadruped source — and FAB-006 does NOT cover it
I checked `.claude/pipeline/fab/FAB-REQUESTS.md`: **FAB-006 is explicitly a *humanoid* pack** ("Humanoid unit quality… into our SiegeBiped chain"; all four candidate listings are knight/warrior humanoids). There is **no** mention of horse, mount, quadruped or cavalry anywhere in the FAB request file, and FAB-000..006 contain no quadruped entry.
**→ A mounted/quadruped donor is an UNFILLED gap, not a pending FAB-006 deliverable.** A new FAB request (rigged horse / mounted-knight pack with its own quadruped skeleton) is warranted. I did **not** author it myself — three art-director agents are running concurrently and `FAB-REQUESTS.md` would race the same way the board does. **Flagging to the orchestrator/manager to create it.** Note that adopting a bespoke horse skeleton is a *scope decision*, not a pipeline fix: it means Cavalry leaves the shared-skeleton lane and needs its own `A_Cavalry_*` clips (same exception the Ogre already has with `SK_Ogre_Skeleton`).

## Team-region (RECORDED)
1 selector (`shoulder_caps`). **Measured on the exported FBX: 772 faces / 4.36% of surface area** (cap 35%). Spatial extent **Z 135.1–176.6**, **Y −83.4 → +38.0** on a mesh that is 208 tall and 205 deep.
*(Bookkeeping note: `refine_report.json` records 701 faces for the same 4.36% area — selector-time vs post-export count. Area fraction matches; 772 is authoritative.)*

**⚠️ Isolation is imperfect on this unit (cosmetic, non-blocking):** the band lands correctly on the rider's shoulders/cloak, but because the selector is an upward-facing-normal band tuned for a biped, it **also catches the horse's neck/withers and a patch on the horse's browband** — visible as blue on the horse's forehead in `bind_front.png`. At 4.36% of area it is well under the cap and the team read is actually strong and legible for a cavalry unit (team-coloured barding is period-plausible), but the forehead patch reads as slightly odd on close inspection. **Judgement: ship it, flag it for Jonathan.** If he dislikes it the lever is a Cavalry-specific selector that excludes the horse's head region — a Stage-2 re-bake, not a re-rig.

## Pre-import eyeball gate — **PASS** (previews vs `Concepts/Cavalry.png`)
- **Silhouette — PASS:** armoured knight astride a chestnut warhorse, red helm plume, kite shield on the left, cream/gold caparison over the horse's flank, studded peytral and girth straps, red-brown bridle and reins, four hooves, flowing tail. Reads unmistakably as the concept from every angle.
- **Colour — PASS with a caveat (mean linear 0.1873):** the chestnut horse coat, cream caparison, red plume and red bridle are all clearly saturated and material-distinct — the washed-out grey failure mode is **gone**. **Caveat:** the rider's plate armour bakes noticeably **darker** than the bright silver in the concept, reading closer to dark iron/gunmetal. Combined with the batch-lowest mean (0.1873), Cavalry is the unit most likely to need a brightness second look in-engine. It is still above the Footman validated-PASS baseline (0.164), so I am **not** re-baking on spec. If the in-engine verify calls it dark, the lever is a Cavalry-only temper toward `gamma 0.50 / gain 1.3` — a Stage-2 re-bake from the cached GLB (**no Meshy credits**).
- **Bind — PASS:** horse and rider intact, zero warping, 0.0% unweighted. See the quadruped-rig caveat above for animated behaviour.
- **Team band — PASS on numbers** (4.36%, under the 35% cap), with the horse-forehead bleed noted above.
- **Grounding:** min_z −0.018 UE. Systemic grounding via TASK-307/308 (live in `0d717c0`).

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[95.5, 205.5]` deviate >10% from blockout target `[75.5, 254.6]`. BENIGN: the TASK-166 blockout guessed a longer, narrower horse; the Meshy result is a slightly shorter, broader animal. `fit_mode=height` enforces Z only (208.0 → 208.05). **Build-master: this unit's 205-unit depth is more than double a foot unit's — sanity-check its collision hulls and spacing/nav footprint after import.** Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Cavalry/previews/preview_front.png` · `preview_threequarter.png` · `preview_back.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Cavalry/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Cavalry/rig/previews/bind_{front,side,threequarter}.png` · `turntable_strip.png`
- Anim contact strips + full frame seqs (**verified complete despite the truncated log**: idle 61 / walk 31 / attack 41 / death 49): `…/rig/previews/contact_*.png`, `…/rig/previews/seq/<anim>/`
- Concept: `Content/RawAssets/Concepts/Cavalry.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite of the 11 fleet units" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Cavalry_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Cavalry/T_Cavalry_D.png`
- `T_Cavalry_N` (**LINEAR**, compression `TC_Normalmap`) ← `…/T_Cavalry_N.png`
- `T_Cavalry_ORM` (**LINEAR, sRGB OFF**, compression `TC_Masks`) ← `…/T_Cavalry_ORM.png`

### 2. Material instance `MI_Cavalry_PBR` (reassign params; do NOT recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Cavalry_D, `Normal`=T_Cavalry_N, `ORM`=T_Cavalry_ORM.

### 3. Static mesh `SM_Cavalry`
- Import `Content/RawAssets/Cavalry.fbx` OVERWRITING `/Game/Meshes/SM_Cavalry` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe), `ucx: null` — **check the hulls on this one**, the 205-unit depth makes a default hull fit sloppier than on a foot unit.
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (design-time placeholder); slot 1 `CavalryPBR` → `MI_Cavalry_PBR`. FBX slots already `[TeamRegion, CavalryPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Cavalry` (runtime visual)
- Import `Content/RawAssets/Characters/Cavalry.fbx` OVERWRITING `/Game/Characters/SK_Cavalry` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** — do NOT create a new skeleton (the FBX carries the exact `root` + 20-bone contract → clean bind). **Creating a horse-specific skeleton here would ORPHAN every `A_Cavalry_*` clip** — that is a deliberate future scope decision (see the FAB gap above), NOT something to do during this import. **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, CavalryPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Cavalry_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched and embedded no actions — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- A same-path SK reimport drops the mesh to LOD0-only. Apply `Content/RawAssets/Characters/Cavalry.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction (TASK-297 recipe). **Readback `lod_count == 3`.** Component defaults `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations = true`.

### 7. Acceptance
- `SK_Cavalry` is the runtime visual, matching `Concepts/Cavalry.png`; slot-0 team recolour tints the rider's shoulder band Blue/Red; hooves meet ground (systemic fix `0d717c0`); preserved anims still play — **expect a stiff gait, that is the known biped-rig limitation, NOT an import fault; do not loop it back as a defect**; `lod_count == 3`; Message Log clean. Also eyeball armour brightness and the horse-forehead team patch for Jonathan. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git / TASKBOARD touched by the recovery pass. Files written by recovery: this handoff + the gitignored `Cache/Cavalry/rig/rig_report.json` reconstruction (stale file quarantined alongside). No Content/ asset was modified — the 16:53–16:57 staged outputs are untouched originals. **Meshy Stage 1 was NOT re-run — no credits spent by this recovery pass.**
