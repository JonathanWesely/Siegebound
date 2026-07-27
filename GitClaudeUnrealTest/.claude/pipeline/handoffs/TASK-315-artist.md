# TASK-315 — Cleric FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Cleric raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26 · **Branch:** m7.6-arena10x · Ran the PROVEN Footman pipeline (validated commit `d7254da`) with the brighter `albedo_delight` override.

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). `--check` PASSED (2776 credits before run). Stage-1 consumed **30** credits (2776 → 2716). Norton TLS: reused `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`. Stage-2 + rig are local (no network). Meshy task `019fa0d2-1ecc-77ae-a323-3c00e5f2a3e8`, engine `meshy-i23d`, concept `Inbox/Cleric.png` (byte-identical to approved `Content/RawAssets/Concepts/Cleric.png`).

## Brightness override (PINNED)
`pipeline_manifest.json` → `assets.Cleric.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` (the validated Footman re-pilot profile). Result: mean linear albedo **0.116 → 0.361** (p99 0.937, headroom intact). NOTE: Cleric's cream robe starts bright, so it lands on the HIGH side (0.361 vs Footman 0.164); 13.4% of pixels touch the soft shoulder-clamp (≤0.98). NOT too dark — the opposite; faithful to the concept's pale/cream robe.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Cleric.fbx` | Raw STATIC mesh (`SM_Cleric`) | 14,998 tris; slots `[TeamRegion, ClericPBR]`; `UVMap`; feet-centre (min_z 0.005 UE); bounds `[97.5, 79.22, 181.88]` UE |
| `Content/RawAssets/Textures/Cleric/T_Cleric_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Cleric/T_Cleric_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Cleric/T_Cleric_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Cleric.fbx` | Rigged SKELETAL FBX (`SK_Cleric` + armature `Footman_Rig`, 21 bones) | slots `[TeamRegion, ClericPBR]`; `UVMap`; auto_heat skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Cleric.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Cleric_{Idle,Walk,Attack,Death}.fbx` (Jul-16, byte-identical) via `rig_character.py --no-anim-fbx`. In-engine `/Game/Characters/Anims/A_Cleric_*` + shared ABP PRESERVED — do NOT reimport.

## How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Cleric` → `Cache/Cleric/meshy_raw.glb` (21.8 MB).
- Stage 2: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Cleric --input Cache/Cleric/meshy_raw.glb` (exit 0, 10.5 s).
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Cleric --no-anim-fbx` (exit 0, 40.1 s).

## Team-region (RECORDED)
`shoulder_caps` (single upward-facing band; robed-caster recipe, NO helm_dome — a hood is not a helmet). Result **425 faces / 2.5% of area** (cap 35%). Isolated to the shoulder/mantle/collar shelf below the hood; hood crown, face, robe body, staff, book all SPARED. Low-but-clean team read (matches TASK-194 _verified). Placeholder blue confirmed isolated in the beauty preview.

## Pre-import eyeball gate — PASS (read refine_report + rig_report, viewed previews vs `Concepts/Cleric.png`)
- **Silhouette:** hooded robe + brown cloak, ornate bronze cross/wheel staff, brown book, rope belt/knot, floating light glyph, sandals, bearded face under hood — reads instantly as the concept from every angle. Clean bind (0.0% unweighted), no holes.
- **Geometry:** 14,998 tris (budget 15,000); feet-centre min_z 0.005; `UVMap` ok; Z height-fit to 182 (181.88).
- **Colour — VIVID, NOT dark (mean linear 0.361):** cream robe, bronze/gold staff head, brown book + rope, red sandal accents, skin/beard all material-distinct. Emphatically not washed-out-grey. **Verdict PASS.**
- **Caveat (non-blocking):** cream robe reads slightly bright/pale under flat light (13.4% shoulder-clamp on a naturally-white robe = the OPPOSITE of the dark failure mode). Faithful to the concept's pale robe; under L_Arena warm ambient it will read fine. If a re-verify finds it too washed, the lever is a temper (gamma 0.65 / gain 1.0) — not expected.

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[98.2, 79.4]` deviate >10% from blockout target `[83.49, 78.96]`. BENIGN: the staff (+X) and book widen the footprint vs the TASK-166 robed-blockout guess. `fit_mode=height` enforces Z only (182). Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Cleric/previews/preview_front.png` · `preview_threequarter.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Cleric/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Cleric/rig/previews/bind_threequarter.png`
- Concept: `Content/RawAssets/Concepts/Cleric.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Cleric_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Cleric/T_Cleric_D.png`
- `T_Cleric_N` (**LINEAR**) ← `…/T_Cleric_N.png`
- `T_Cleric_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Cleric_ORM.png`

### 2. Material instance `MI_Cleric_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Cleric_D, `Normal`=T_Cleric_N, `ORM`=T_Cleric_ORM.

### 3. Static mesh `SM_Cleric`
- Import `Content/RawAssets/Cleric.fbx` OVERWRITING `/Game/Meshes/SM_Cleric` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe).
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `ClericPBR` → `MI_Cleric_PBR`. FBX slots already `[TeamRegion, ClericPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Cleric` (runtime visual)
- Import `Content/RawAssets/Characters/Cleric.fbx` OVERWRITING `/Game/Characters/SK_Cleric` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — do NOT create a new skeleton (FBX carries the exact 21-bone contract → clean bind). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, ClericPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Cleric_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/Cleric.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_Cleric` is the runtime visual, VIVID matching `Concepts/Cleric.png`; slot-0 team recolour tints the shoulder band Blue/Red; feet meet ground; preserved anims still play; LOD count 3; Message Log clean. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git touched. Files written: the manifest override (Cleric), the 3 staged PNGs, the 2 FBXs + LOD sidecar, the gitignored `Cache/Cleric/**` (report + previews), this handoff, the board note.
