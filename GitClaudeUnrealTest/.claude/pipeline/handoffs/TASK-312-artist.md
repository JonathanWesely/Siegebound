# TASK-312 — Archer FLEET-REMASTER (M7.9) — art-director handoff

> ## ⚠️ RECONSTRUCTED POST-REBOOT — read this first
> The 2026-07-26 ~16:58 PC reboot killed the Archer agent **after** every artifact was written but **before** it wrote its handoff. This note was rebuilt on 2026-07-26 19:0x by a recovery art-director from the on-disk reports **plus direct measurement of the staged FBXs** (headless Blender 5.1.2 read-only probe). **Nothing was regenerated** — the staged assets are the original 16:54–16:57 outputs, validated intact.
> **`Cache/Archer/rig/rig_report.json` was ALSO lost.** A **stale Jul-21 21:07 report describing a DIFFERENT mesh** was sitting at that path; it has been moved to `rig_report.STALE-PREREBOOT.json` and replaced with a clearly-labelled `_RECONSTRUCTED` report built from measured ground truth. Do not trust the stale file.
> **Integrity verdict: PASS** (no repair needed, nothing re-run).

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Archer raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26 · **Branch:** main · Ran the PROVEN Footman pipeline (validated commit `d7254da`) with the brighter `albedo_delight` override.

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). Stage-1 consumed **30** credits. Meshy task `019fa0d1-4bc3-752f-9504-b49d551859c6`, engine `meshy-i23d`, endpoint `/openapi/v1/image-to-3d`, ran 23:44:58→23:47:51 UTC. Donor `Cache/Archer/meshy_raw.glb` (22,986,948 B, sha `1b9b9075…`). Concept `Inbox/Archer.png` (1408×768) **verified byte-identical** (sha256 `bbc0db006fccc690…`) to the approved `Content/RawAssets/Concepts/Archer.png`. Stage-2 + rig are local (no network).

## Brightness override (PINNED)
`pipeline_manifest.json` → `assets.Archer.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — the FLEET-DEFAULT profile validated on the Footman pilot (`d7254da`). Confirmed `applied: true` in the bake.
Result: mean linear albedo **0.0615 → 0.2919** (p99 0.909, shoulder-clamp 4.52%). **Independently re-measured from the shipped `T_Archer_D.png`: 0.2918** — the bake genuinely reflects the pin. Well clear of the Footman validated-PASS baseline (0.164); highest of the three recovered units.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Archer.fbx` | Raw STATIC mesh (`SM_Archer`) | **14,998 tris** (budget 15,000); 7,505 verts; slots `[TeamRegion, ArcherPBR]`; `UVMap`; feet-centre (min_z **+0.009** UE); bounds `[87.22, 101.27, 179.91]` UE |
| `Content/RawAssets/Textures/Archer/T_Archer_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Archer/T_Archer_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Archer/T_Archer_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Archer.fbx` | Rigged SKELETAL FBX (`SK_Archer` + armature `Footman_Rig`, 21 bones) | slots `[TeamRegion, ArcherPBR]`; `UVMap`; auto_heat skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Archer.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` — **NEW untracked file, `git add` it** |

All 3 PNGs verified: 1024×1024, 8-bit RGB, **zero CRC errors, IEND present** (no reboot truncation).

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Archer_{Idle,Walk,Attack,Death}.fbx` via `rig_character.py --no-anim-fbx`. Verified: the rigged mesh FBX carries **0 embedded actions**. In-engine `/Game/Characters/Anims/A_Archer_*` + shared ABP PRESERVED — do NOT reimport.

## How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Archer` → `Cache/Archer/meshy_raw.glb`.
- Stage 2: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Archer --input Cache/Archer/meshy_raw.glb` (10.4 s).
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Archer --no-anim-fbx` (killed by the reboot during its final report write; all outputs already on disk).

## Skeleton bind target (RECORDED per the shared-skeleton binding law)
Armature **`Footman_Rig`** — root bone `root` + the **20-bone SiegeBiped deform set**, byte-for-byte the same contract as the shipped Footman/Sapper/Cleric/Knight rigs (verified by direct comparison). 20 vertex groups, **all** matching bone names, no orphan groups. → binds cleanly to the SHIPPED shared **`/Game/Characters/SK_Footman_Skeleton`**. Existing `A_Archer_*` sequences keep binding.
Measured skin: **0 unweighted verts (0.0000%)**, 0 zero-sum verts, max **8** influences/vert (within UE's 12 default — no clamping).
Rig fit: weighted mean bone-influence drift **15.0 cm = 8.3% of height** — healthy humanoid distribution (top-3 bones own 34.5% of weight).

## Team-region (RECORDED)
2 selectors (shoulder band recipe). **Measured on the exported FBX: 496 faces / 3.29% of surface area** (cap 35%). Spatially isolated to the shoulder/upper-chest mantle, **Z 128.2–155.9 of a 179.9-tall mesh**. Hair, face, tunic body, quiver, bow, boots all SPARED. Placeholder blue confirmed isolated in the bind previews.
*(Bookkeeping note: `refine_report.json` records 375 faces for the same 3.29% area — the report counts the selector-time face set before the final export topology pass. The **area fraction is identical**; the measured 496 is the authoritative post-export count.)*

## Pre-import eyeball gate — **PASS** (previews vs `Concepts/Archer.png`)
- **Silhouette — PASS:** ginger bob, green quilted tunic with gold griffon blazon, steel pauldrons, arrow-filled quiver over the left shoulder, brown bracers/gloves, tall cuffed boots, bow in the right hand. Reads instantly as the concept from front, three-quarter and back.
- **Colour — VIVID, PASS (mean linear 0.2919):** green tunic, ginger hair, warm brown leather and steel pauldrons are all material-distinct and saturated. The washed-out grey failure mode is **gone**.
- **Bind — PASS:** clean standing bind, 0.0% unweighted, correct limb separation, no stretching/collapse, no holes.
- **Team band — PASS:** clean isolated blue shoulder mantle, 3.29% of area.
- **Grounding:** min_z **+0.009 UE** (feet-centre). TASK-259 float-fix history is superseded — grounding is now systemic via TASK-307/308 (live in `0d717c0`).
- **Caveat (non-blocking, cosmetic):** the bow limb + string remesh to a very thin spindly sliver at 1.5-unit voxel size — the known voxel-remesh limitation on sub-voxel props (same as the previously shipped Archer). Reads fine at gameplay camera distance.

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[87.5, 101.5]` deviate >10% from blockout target `[125.0, 67.3]`. BENIGN: the TASK-166 blockout guessed a wide-stance archer; the Meshy result is a slim standing figure that is deeper (quiver + bow) than wide. `fit_mode=height` enforces Z only (180.0 → 179.91). Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Archer/previews/preview_front.png` · `preview_threequarter.png` · `preview_back.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Archer/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Archer/rig/previews/bind_{front,side,threequarter}.png` · `turntable_strip.png`
- Anim contact strips + full frame seqs (verified complete: idle 61 / walk 31 / attack 41 / death 49): `…/rig/previews/contact_*.png`, `…/rig/previews/seq/<anim>/`
- Concept: `Content/RawAssets/Concepts/Archer.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite of the 11 fleet units" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Archer_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Archer/T_Archer_D.png`
- `T_Archer_N` (**LINEAR**, compression `TC_Normalmap`) ← `…/T_Archer_N.png`
- `T_Archer_ORM` (**LINEAR, sRGB OFF**, compression `TC_Masks`) ← `…/T_Archer_ORM.png`

### 2. Material instance `MI_Archer_PBR` (reassign params; do NOT recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Archer_D, `Normal`=T_Archer_N, `ORM`=T_Archer_ORM.

### 3. Static mesh `SM_Archer`
- Import `Content/RawAssets/Archer.fbx` OVERWRITING `/Game/Meshes/SM_Archer` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe), `ucx: null`.
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (design-time placeholder); slot 1 `ArcherPBR` → `MI_Archer_PBR`. FBX slots already `[TeamRegion, ArcherPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Archer` (runtime visual)
- Import `Content/RawAssets/Characters/Archer.fbx` OVERWRITING `/Game/Characters/SK_Archer` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** — do NOT create a new skeleton (the FBX carries the exact `root` + 20-bone contract → clean bind). Creating a new skeleton would ORPHAN the anims. **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, ArcherPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Archer_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched and embedded no actions — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- A same-path SK reimport drops the mesh to LOD0-only. Apply `Content/RawAssets/Characters/Archer.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction (TASK-297 recipe). **Readback `lod_count == 3`.** Component defaults `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations = true` (URO already set in C++ on `SkeletalVisualMesh`).

### 7. Acceptance
- `SK_Archer` is the runtime visual, VIVID matching `Concepts/Archer.png`; slot-0 team recolour tints the shoulder mantle Blue/Red; feet meet ground (systemic fix `0d717c0`); preserved anims still play; `lod_count == 3`; Message Log clean. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git / TASKBOARD touched by the recovery pass. Files written by recovery: this handoff + the gitignored `Cache/Archer/rig/rig_report.json` reconstruction (stale file quarantined alongside). No Content/ asset was modified — the 16:54–16:57 staged outputs are untouched originals.
