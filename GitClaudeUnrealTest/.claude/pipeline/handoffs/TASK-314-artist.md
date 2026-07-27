# TASK-314 — Miner FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Miner raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26 · **Branch:** main · Ran the PROVEN Footman pipeline (validated commit `d7254da`) with the FLEET-DEFAULT `albedo_delight` brightness profile.

## Reboot-recovery note (read this first)
A PC reboot at ~16:58 on 2026-07-26 killed the in-flight parallel fan-out **after** Stage 1 + Stage 2 completed (16:54 / 16:55) but **before** the rig stage ran. This session re-verified the staged Stage-1/2 output and then ran ONLY the rig stage. **Stage 1 was NOT re-run — no new Meshy credits consumed by this session.**

**Integrity verdict: PASS (no truncation).**
- `Content/RawAssets/Miner.fbx` 605,532 B, `Kaydara FBX Binary` sig + valid binary footer.
- All 3 PNGs decode fully: 1024×1024, 8-bit RGB, terminating `IEND` chunk present (D 1,206,852 B / N 1,024,506 B / ORM 1,137,080 B).
- `Cache/Miner/refine_report.json` complete and well-formed, `elapsed_seconds` present (11.5).
- Concept provenance intact: `Inbox/Miner.png` and `Content/RawAssets/Concepts/Miner.png` byte-identical (md5 `51ac18bb6e5a7dd1a3d0940e01269bcb`, 860,216 B).

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). Meshy task `019fa0d6-dfee-764e-8e80-d9d090471cb1`, engine image3d `/openapi/v1/image-to-3d`, `ai_model=latest`, target_polycount 300k, PBR on. Recorded `consumed_credits` **30** (`credits_before` 2596 → `credits_after` 2536; the delta spans the concurrently-running fan-out agents). Stage-2 + rig are local (no network).

## Brightness override (PINNED — FLEET-DEFAULT)
`pipeline_manifest.json` → `assets.Miner.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — verified pinned (explicitly noted as the TASK-314 M7.9 fan-out default) and `"applied": true` in the refine report. Result: **mean linear albedo 0.0496 → 0.2417** (p99 0.9252, **4.33%** shoulder-clamped — headroom intact).

Independent re-measure straight off the shipped `T_Miner_D.png` (sRGB→linear, Rec.709 luma): mean **0.2489**, p50 0.2089, p90 0.6064, p99 0.8615. Lands **between** Footman (0.164) and Cleric (0.373) — comfortably in the validated band, and it is the **most colourful unit of the batch** (HSV saturation mean 0.199; 61.3% of texels sat>0.15, vs Knight's 24.8%).

## Assets staged (same-path overwrite; LOD sidecar written)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Miner.fbx` | Raw STATIC mesh (`SM_Miner`) | 14,998 tris; slots `[TeamRegion, MinerPBR]`; `UVMap`; feet-centre (min_z **0.041** UE); bounds `[121.96, 75.23, 172.87]` UE |
| `Content/RawAssets/Textures/Miner/T_Miner_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Miner/T_Miner_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Miner/T_Miner_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024²; metallic mean 0.023 (**non-metal unit** — cloth/leather, only the pick head + buckle) |
| `Content/RawAssets/Characters/Miner.fbx` | Rigged SKELETAL FBX (`SK_Miner` + armature `Footman_Rig`, 21 bones / 20 deform) | 14,997 tris, 7,495 verts; slots `[TeamRegion, MinerPBR]`; `UVMap`; auto_heat skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Miner.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Miner_{Idle,Walk,Attack,Death}.fbx` — all four still Jul-16 06:05, byte-untouched, via `rig_character.py --no-anim-fbx`. In-engine `/Game/Characters/Anims/A_Miner_*` + shared ABP PRESERVED — do NOT reimport.

## How generated (reproducible)
- Stage 1 (PRE-REBOOT, not re-run): `uv run meshy_generate.py --mode image3d Miner` → `Cache/Miner/meshy_raw.glb` (23.3 MB, sha256 `042861a6…`).
- Stage 2 (PRE-REBOOT, verified intact): `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Miner --input Cache/Miner/meshy_raw.glb` (11.5 s).
- Rig (THIS SESSION): `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Miner --no-anim-fbx` (**exit 0**, 47.4 s, Blender 5.1.2, zero warnings in `rig_report.json`).

## Team-region (RECORDED)
Single selector (1) — collar/upper-chest + shoulder band. Manifest recipe deliberately has **NO helm_dome** (a soft brimmed miner's cap is not a helmet — the Archer TASK-087 lesson about painting the face). Result **776 faces / 5.52% of area** (cap 35%). Isolated to the collar/shoulder shelf; **brimmed hat + lamp, face, beard, gloves, belt, satchel, pickaxe, trousers, boots all SPARED.** Placeholder blue confirmed isolated in the beauty + 8-angle turntable, and it wraps far enough around to read from every angle.

## Pre-import eyeball gate — PASS (read refine_report + rig_report, viewed previews vs `Concepts/Miner.png`)
- **Silhouette:** wide-brim miner's hat with front lamp, pointed ears, big forked beard, pickaxe carried over the shoulder with the free fist raised, tan/olive layered tunic, wide belt with a big square buckle, hip pouch, slung satchel bag, heavy boots — reads instantly as the concept from all 8 turntable angles. Clean bind (0.0% unweighted), no holes, no collapsed limbs.
- **Geometry:** 14,998 tris SM / 14,997 SK (budget 15,000); feet-centre min_z 0.041; `UVMap` ok; Z height-fit to 173 (172.87).
- **Colour — VIVID, the standout of the batch (mean linear 0.2489, saturation 0.199):** warm tan/olive tunic, saturated brown leathers, ruddy skin tone, pale wood pick haft, dark steel pick head, brass lamp accent — all material-distinct with real hue separation. Emphatically not the muddy-grey failure mode. **Verdict PASS.**

### Caveat (non-blocking, cosmetic)
The team-region band sits directly under the chin, so its jagged upper edge **grazes the bottom fringe of the beard** — in the beauty render a few beard tips catch the team blue. Face, hat, and the beard mass are all clean; at RTS camera distance this is invisible. Flagged only so it is not mistaken for a bake error later. If a future pass wants it tightened, the lever is a Z-constrained selector in the manifest `team_region` recipe (drop the band ~2 cm) — not worth a re-gen now.

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[122.1, 75.4]` deviate >10% from blockout target `[129.45, 62.0]`. BENIGN: the TASK-166 target was a pre-generation guess; the shoulder-carried pickaxe and the slung satchel add real depth (Y 75.4 vs 62.0 guess) while the stocky dwarf stands narrower than guessed. `fit_mode=height` enforces Z only (173), which is exact. Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Miner/previews/preview_front.png` · `preview_threequarter.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Miner/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Miner/rig/previews/bind_threequarter.png`
- 8-angle turntable: `Tools/ArtPipeline/Cache/Miner/rig/previews/turntable_strip.png`
- Concept: `Content/RawAssets/Concepts/Miner.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Miner_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Miner/T_Miner_D.png`
- `T_Miner_N` (**LINEAR**) ← `…/T_Miner_N.png`
- `T_Miner_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Miner_ORM.png`

### 2. Material instance `MI_Miner_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Miner_D, `Normal`=T_Miner_N, `ORM`=T_Miner_ORM.

### 3. Static mesh `SM_Miner`
- Import `Content/RawAssets/Miner.fbx` OVERWRITING `/Game/Meshes/SM_Miner` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe).
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `MinerPBR` → `MI_Miner_PBR`. FBX slots already `[TeamRegion, MinerPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Miner` (runtime visual)
- Import `Content/RawAssets/Characters/Miner.fbx` OVERWRITING `/Game/Characters/SK_Miner` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — do NOT create a new skeleton (FBX carries the exact 21-bone contract → clean bind). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, MinerPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Miner_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/Miner.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_Miner` is the runtime visual, VIVID matching `Concepts/Miner.png`; slot-0 team recolour tints the collar/shoulder band Blue/Red; feet meet ground; preserved anims still play; LOD count 3; Message Log clean. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git touched. **TASKBOARD.md deliberately NOT written** (concurrent art-director agents race the board — orchestrator records status from this handoff). Files written by this session: the 2 rig outputs (`Characters/Miner.fbx`, `Characters/Miner.lod.json`), the gitignored `Cache/Miner/rig/**` (report + previews), and this handoff. Stage-1/2 artefacts were pre-existing and left byte-untouched.
