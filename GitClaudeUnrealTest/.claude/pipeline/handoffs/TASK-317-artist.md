# TASK-317 — Sapper FLEET-REMASTER (M7.9) — art-director handoff

> ## ⚠️ RECONSTRUCTED POST-REBOOT — read this first
> The 2026-07-26 ~16:58 PC reboot killed the Sapper agent **after** every artifact was written but **before** it wrote its handoff. This note was rebuilt on 2026-07-26 19:0x by a recovery art-director from the on-disk reports **plus direct measurement of the staged FBXs** (headless Blender 5.1.2 read-only probe). **Nothing was regenerated.**
> **Sapper is the cleanest of the three recovered units:** its `rig_report.json` **completed** (written 16:57:56, ~2 s before the kill), so this reconstruction is corroborated by a genuine script report as well as by measurement — the two agree on every field.
> **Integrity verdict: PASS** (no repair needed, nothing re-run).

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Sapper raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26 · **Branch:** main · Ran the PROVEN Footman pipeline (validated commit `d7254da`) with the brighter `albedo_delight` override.

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). Stage-1 consumed **30** credits. Meshy task `019fa0d8-99f0-76e7-afd2-21c2707bc6c9`, engine `meshy-i23d`, endpoint `/openapi/v1/image-to-3d`, ran 23:52:57→23:56:27 UTC. Donor `Cache/Sapper/meshy_raw.glb` (23,464,384 B, sha `f3fefeb1…`). Concept `Inbox/Sapper.png` (1024×1024) **verified byte-identical** (sha256 `61e364b212c5ebf9…`) to the approved `Content/RawAssets/Concepts/Sapper.png`. Stage-2 + rig are local (no network).

## Brightness override (PINNED)
`pipeline_manifest.json` → `assets.Sapper.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — the FLEET-DEFAULT profile validated on the Footman pilot (`d7254da`). Confirmed `applied: true` in the bake.
Result: mean linear albedo **0.0528 → 0.2394** (p99 0.9349, shoulder-clamp 6.01%). **Independently re-measured from the shipped `T_Sapper_D.png`: 0.2394** — exact match; the bake genuinely reflects the pin. Comfortably above the Footman validated-PASS baseline (0.164).

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Sapper.fbx` | Raw STATIC mesh (`SM_Sapper`) | **14,996 tris** (budget 15,000); 7,503 verts; slots `[TeamRegion, SapperPBR]`; `UVMap`; feet-centre (min_z **+0.02** UE); bounds `[100.03, 104.75, 168.59]` UE |
| `Content/RawAssets/Textures/Sapper/T_Sapper_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Sapper/T_Sapper_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Sapper/T_Sapper_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Sapper.fbx` | Rigged SKELETAL FBX (`SK_Sapper` + armature `Footman_Rig`, 21 bones) | slots `[TeamRegion, SapperPBR]`; `UVMap`; auto_heat skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Sapper.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` — **NEW untracked file, `git add` it** |

All 3 PNGs verified: 1024×1024, 8-bit RGB, **zero CRC errors, IEND present** (no reboot truncation).
*(Minor bookkeeping: `refine_report.json` records `result.tris: 14998`; the exported FBX measures **14,996** — the export topology pass shed 2 tris. The rig report's import step independently also reads 14,996. Both are under budget; 14,996 is authoritative.)*

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Sapper_{Idle,Walk,Attack,Death}.fbx` via `rig_character.py --no-anim-fbx`. Verified: the rigged mesh FBX carries **0 embedded actions**. In-engine `/Game/Characters/Anims/A_Sapper_*` + shared ABP PRESERVED — do NOT reimport.

## How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Sapper` → `Cache/Sapper/meshy_raw.glb`.
- Stage 2: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Sapper --input Cache/Sapper/meshy_raw.glb` (11.4 s).
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Sapper --no-anim-fbx` (50.5 s, **completed**).

## Skeleton bind target (RECORDED per the shared-skeleton binding law)
Armature **`Footman_Rig`** — root bone `root` + the **20-bone SiegeBiped deform set**, identical to the shipped Footman/Cleric/Knight contract (verified by direct comparison). 20 vertex groups, **all** matching bone names, no orphan groups. → binds cleanly to the SHIPPED shared **`/Game/Characters/SK_Footman_Skeleton`**. Existing `A_Sapper_*` sequences keep binding.
Measured skin: **0 unweighted verts (0.0000%)**, 0 zero-sum verts, max **13** influences/vert. Rig fit: weighted mean bone-influence drift **16.7 cm = 9.9% of height** — healthy humanoid distribution (top-3 bones own 31.2% of weight).

> **⚠️ Build-master note — 13 influences/vert:** exceeds UE's default 12-bone limit. UE will drop the smallest influence and renormalize on import; expect an informational Message Log line, **not** an error. This is cosmetically harmless (the 13th weight is negligible). If you prefer zero log noise, raise the import option to 16 bones/vertex — do NOT reduce below 12.

## Team-region (RECORDED)
1 selector (`shoulder_caps`). **Measured on the exported FBX: 683 faces / 4.64% of surface area** (cap 35%). Spatially isolated to the collar/scarf/shoulder shelf, **Z 107.5–142.3 of a 168.6-tall mesh**. The bomb, goggles, face, gloves, trousers and boots are all SPARED. Placeholder blue confirmed isolated in the bind previews.
*(Bookkeeping note: `refine_report.json` records 606 faces for the same 4.63% area — selector-time count vs post-export count. Area fraction matches; 683 is authoritative.)*

## Pre-import eyeball gate — **PASS** (previews vs `Concepts/Sapper.png`)
- **Silhouette — PASS:** burly demolitionist hugging a large black bomb, aviator goggles pushed up on the forehead, moustache, tuft of hair, brown leather jerkin with pale grey sleeves, fringed trousers, heavy boots. Reads instantly as the concept; the bomb-hug pose is unmistakable.
- **Colour — PASS (mean linear 0.2394):** brown leathers, cream/grey sleeves, black iron bomb, skin tones all material-distinct. The palette is inherently muted-brown **in the concept itself**, so this unit is the least "colourful" of the three by design — but the browns read as brown, not as washed-out grey. Wash-out failure mode is **gone**.
- **Team band — PASS:** clean isolated blue collar/shoulder band, 4.64% of area, clearly legible against the brown jerkin.
- **Grounding:** min_z **+0.02 UE** (feet-centre). Systemic grounding via TASK-307/308 (live in `0d717c0`).

### ✅ The historical Sapper rig defect is FIXED — checked especially carefully as instructed
The OLD Sapper rig was hunched and broke anim transfer. **The new bind does not reproduce that defect:**
- The **mesh** is hunched over the bomb (correct — that is the concept), but the **skeleton is a normal upright biped**: `thigh` at Z 84.3, `spine_01` at 101.2, `head` at 152.6 on a 168.6-tall mesh — textbook proportional placement, not a hunched skeleton baked into the rest pose.
- Bind previews (front + side) show a **clean bind**: correct limb separation, arms wrapping the bomb without candy-wrapper twisting, boots and legs intact, **no stretching, no collapse, no holes**.
- **0.0% unweighted**, drift 9.9% of height — statistically indistinguishable from the healthy Archer rig (8.3%).
- Because the skeleton is the standard upright `Footman_Rig` contract, the shared `A_Sapper_*` clips retarget onto it the same as any other humanoid. **Verdict: anim transfer risk resolved.**
- *Residual cosmetic note (non-blocking):* the hands sit low (weighted centroids Z ≈ 62–64) because the mesh grips the bomb at waist height. Under a big arm-swing clip the forearms will drag the bomb with them — expected for a static held prop, and this unit is `bSuicide` so its Attack is a detonation, not a swing. Judge in-editor at the verify step.

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[100.4, 104.8]` deviate >10% from blockout target `[73.94, 98.0]`. BENIGN: the bomb widens the footprint well beyond the TASK-166 blockout guess. `fit_mode=height` enforces Z only (169.0 → 168.59). Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Sapper/previews/preview_front.png` · `preview_threequarter.png` · `preview_back.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Sapper/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Sapper/rig/previews/bind_{front,side,threequarter}.png` · `turntable_strip.png`
- Anim contact strips + full frame seqs (verified complete: idle 61 / walk 31 / attack 41 / death 49): `…/rig/previews/contact_*.png`, `…/rig/previews/seq/<anim>/`
- Concept: `Content/RawAssets/Concepts/Sapper.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite of the 11 fleet units" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Sapper_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Sapper/T_Sapper_D.png`
- `T_Sapper_N` (**LINEAR**, compression `TC_Normalmap`) ← `…/T_Sapper_N.png`
- `T_Sapper_ORM` (**LINEAR, sRGB OFF**, compression `TC_Masks`) ← `…/T_Sapper_ORM.png`

### 2. Material instance `MI_Sapper_PBR` (reassign params; do NOT recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Sapper_D, `Normal`=T_Sapper_N, `ORM`=T_Sapper_ORM.

### 3. Static mesh `SM_Sapper`
- Import `Content/RawAssets/Sapper.fbx` OVERWRITING `/Game/Meshes/SM_Sapper` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe), `ucx: null`.
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (design-time placeholder); slot 1 `SapperPBR` → `MI_Sapper_PBR`. FBX slots already `[TeamRegion, SapperPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Sapper` (runtime visual)
- Import `Content/RawAssets/Characters/Sapper.fbx` OVERWRITING `/Game/Characters/SK_Sapper` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** — do NOT create a new skeleton (the FBX carries the exact `root` + 20-bone contract → clean bind). Creating a new skeleton would ORPHAN the anims. **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, SapperPBR]`. See the 13-influences note above.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Sapper_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched and embedded no actions — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- A same-path SK reimport drops the mesh to LOD0-only. Apply `Content/RawAssets/Characters/Sapper.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction (TASK-297 recipe). **Readback `lod_count == 3`.** Component defaults `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations = true`.

### 7. Acceptance
- `SK_Sapper` is the runtime visual, matching `Concepts/Sapper.png`; slot-0 team recolour tints the collar band Blue/Red; feet meet ground (systemic fix `0d717c0`); preserved anims still play — **specifically re-check the Sapper Attack/Death play cleanly given the old hunched-rig history**; `lod_count == 3`; Message Log clean apart from the expected influence-clamp line. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git / TASKBOARD touched by the recovery pass. Files written by recovery: this handoff only — Sapper's own `rig_report.json` was intact and was left exactly as the script wrote it. No Content/ asset was modified.
