# TASK-313 — Knight FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Knight raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26 · **Branch:** main · Ran the PROVEN Footman pipeline (validated commit `d7254da`) with the FLEET-DEFAULT `albedo_delight` brightness profile.

## Reboot-recovery note (read this first)
A PC reboot at ~16:58 on 2026-07-26 killed the in-flight parallel fan-out **after** Stage 1 + Stage 2 completed (16:51 / 16:54) but **before** the rig stage ran. This session re-verified the staged Stage-1/2 output and then ran ONLY the rig stage. **Stage 1 was NOT re-run — no new Meshy credits consumed by this session.**

**Integrity verdict: PASS (no truncation).**
- `Content/RawAssets/Knight.fbx` 597,180 B, `Kaydara FBX Binary` sig + valid binary footer.
- All 3 PNGs decode fully: 1024×1024, 8-bit RGB, terminating `IEND` chunk present (D 1,220,508 B / N 1,049,328 B / ORM 1,249,386 B).
- `Cache/Knight/refine_report.json` complete and well-formed, `elapsed_seconds` present (11.5).
- Concept provenance intact: `Inbox/Knight.png` and `Content/RawAssets/Concepts/Knight.png` byte-identical (md5 `b4a39744897ed1fa7f8aade92dd65944`, 615,046 B).

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). Meshy task `019fa0d4-738e-780b-9043-68fc5ad2fc90`, engine image3d `/openapi/v1/image-to-3d`, `ai_model=latest`, target_polycount 300k, PBR on. Recorded `consumed_credits` **30**. (`credits_before` 2746 → `credits_after` 2596 spans the 4 concurrently-running fan-out agents, so the 150 delta is NOT this unit alone — the per-task recorded figure is 30.) Stage-2 + rig are local (no network).

## Brightness override (PINNED — FLEET-DEFAULT)
`pipeline_manifest.json` → `assets.Knight.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — verified pinned and `"applied": true` in the refine report. Result: **mean linear albedo 0.0246 → 0.1639** (p99 0.8051, only **1.02%** shoulder-clamped — ample highlight headroom).

**This is a bullseye on the validated Footman target (0.164).** Independent re-measure straight off the shipped `T_Knight_D.png` (sRGB→linear, Rec.709 luma): mean **0.1646**, p50 0.1093, p90 0.4236, p99 0.7763.

## Assets staged (same-path overwrite; LOD sidecar written)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Knight.fbx` | Raw STATIC mesh (`SM_Knight`) | 15,000 tris; slots `[TeamRegion, KnightPBR]`; `UVMap`; feet-centre (min_z **-0.006** UE); bounds `[143.7, 70.53, 188.92]` UE |
| `Content/RawAssets/Textures/Knight/T_Knight_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Knight/T_Knight_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Knight/T_Knight_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024²; **metallic mean 0.304, 32.5% of texels metal>0.5** |
| `Content/RawAssets/Characters/Knight.fbx` | Rigged SKELETAL FBX (`SK_Knight` + armature `Footman_Rig`, 21 bones / 20 deform) | 15,000 tris, 7,496 verts; slots `[TeamRegion, KnightPBR]`; `UVMap`; auto_heat skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Knight.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Knight_{Idle,Walk,Attack,Death}.fbx` — all four still Jul-16 06:00, byte-untouched, via `rig_character.py --no-anim-fbx`. In-engine `/Game/Characters/Anims/A_Knight_*` + shared ABP PRESERVED — do NOT reimport.

## How generated (reproducible)
- Stage 1 (PRE-REBOOT, not re-run): `uv run meshy_generate.py --mode image3d Knight` → `Cache/Knight/meshy_raw.glb` (23.5 MB, sha256 `4c2e5c85…`).
- Stage 2 (PRE-REBOOT, verified intact): `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Knight --input Cache/Knight/meshy_raw.glb` (11.5 s).
- Rig (THIS SESSION): `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Knight --no-anim-fbx` (**exit 0**, 39.5 s, Blender 5.1.2, zero warnings in `rig_report.json`).

## Team-region (RECORDED)
`helm_dome + shoulder_caps` (2 selectors; heavily-armoured helmeted-soldier recipe). Result **689 faces / 4.83% of area** (cap 35%). Isolated to the helm crown and shoulder pauldron caps; **visor/face, cuirass, tabard, shield, sword, skirt, greaves all SPARED.** Placeholder blue confirmed isolated in the beauty + 8-angle turntable. Because the helm dome is upward-facing, the team read is strong from the RTS top-down camera — the best team read of the batch.

## Pre-import eyeball gate — PASS (read refine_report + rig_report, viewed previews vs `Concepts/Knight.png`)
- **Silhouette:** conical/visored great helm, sword raised in the right hand, large kite shield on the left, cream tabard over cuirass, wide leather belt with gold buckle, hip pouch, thigh/shin plate, armoured boots — reads instantly as the concept from all 8 turntable angles. Clean bind (0.0% unweighted), no holes, no collapsed limbs.
- **Geometry:** 15,000 tris (budget 15,000 — exactly at cap); feet-centre min_z -0.006; `UVMap` ok; Z height-fit to 190 (188.92).
- **Colour — VIVID, NOT washed out (mean linear 0.1646 ≡ the Footman-validated 0.164):** cream tabard, warm brown leather belt/pouch/scabbard, gold buckle, blue undertunic hints, steel plate — all material-distinct. Emphatically not the muddy-grey failure mode. **Verdict PASS.**
- **The dark shield/plate in the flat-lit preview is CORRECT, not a defect** — see the note below. Checked explicitly, not waved through.

### Metallic note (IMPORTANT for whoever eyeballs this — non-blocking, by design)
In the flat-lit true-albedo previews the kite shield and plate armour read **near-black**, darker than the concept's mid-grey steel. This is expected and correct: the ORM marks **32.5% of texels as metal>0.5** (metallic mean 0.304), and **metals correctly have a dark diffuse base colour** — they get their entire appearance from specular reflection, which the flat-lit albedo preview does not render. Under L_Arena (real lighting + the ORM metallic channel through `M_AssetPBR`) the shield and plate will read as bright reflective steel. Do NOT "fix" this by brightening the albedo — that would produce chalky, plastic-looking fake metal. Compare against the Cycles beauty preview, where the steel already picks up highlights.

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[144.0, 70.8]` deviate >10% from blockout target `[162.0, 62.74]`. BENIGN: the TASK-166 target was a pre-generation guess; the actual model holds the shield tucked closer to the body (narrower X) and the shield adds depth (wider Y). `fit_mode=height` enforces Z only (190), which is exact. Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Knight/previews/preview_front.png` · `preview_threequarter.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Knight/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Knight/rig/previews/bind_threequarter.png`
- 8-angle turntable: `Tools/ArtPipeline/Cache/Knight/rig/previews/turntable_strip.png`
- Concept: `Content/RawAssets/Concepts/Knight.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Knight_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Knight/T_Knight_D.png`
- `T_Knight_N` (**LINEAR**) ← `…/T_Knight_N.png`
- `T_Knight_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Knight_ORM.png`

### 2. Material instance `MI_Knight_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Knight_D, `Normal`=T_Knight_N, `ORM`=T_Knight_ORM.
- **Verify the ORM actually lands** — this unit is 32.5% metal and will look wrong (flat black shield) if the ORM param fails to bind or is imported with sRGB ON.

### 3. Static mesh `SM_Knight`
- Import `Content/RawAssets/Knight.fbx` OVERWRITING `/Game/Meshes/SM_Knight` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe).
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `KnightPBR` → `MI_Knight_PBR`. FBX slots already `[TeamRegion, KnightPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Knight` (runtime visual)
- Import `Content/RawAssets/Characters/Knight.fbx` OVERWRITING `/Game/Characters/SK_Knight` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — do NOT create a new skeleton (FBX carries the exact 21-bone contract → clean bind). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, KnightPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Knight_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/Knight.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_Knight` is the runtime visual, VIVID matching `Concepts/Knight.png`, **with the shield/plate reading as reflective steel (not flat black) — that is the ORM check**; slot-0 team recolour tints the helm dome + shoulder caps Blue/Red; feet meet ground; preserved anims still play; LOD count 3; Message Log clean. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git touched. **TASKBOARD.md deliberately NOT written** (concurrent art-director agents race the board — orchestrator records status from this handoff). Files written by this session: the 2 rig outputs (`Characters/Knight.fbx`, `Characters/Knight.lod.json`), the gitignored `Cache/Knight/rig/**` (report + previews), and this handoff. Stage-1/2 artefacts were pre-existing and left byte-untouched.
