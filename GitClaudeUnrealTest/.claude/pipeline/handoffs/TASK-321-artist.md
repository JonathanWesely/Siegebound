# TASK-321 — Longbowman FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Longbowman raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26/27 · Scope this session: **RIG STAGE ONLY** — Stage 1 + Stage 2 were already staged at 16:51/16:53 before the PC reboot; they were integrity-checked and ACCEPTED (see below), so **NO new Meshy credits were spent on this unit.**

## Reboot recovery — integrity verdict: **INTACT, Stage 1+2 ACCEPTED AS-IS**
A PC reboot killed the parallel fan-out at ~16:58. Longbowman's Stage-1/2 output survived it:
| Artifact | Timestamp | Verdict |
|---|---|---|
| `Cache/Longbowman/meshy_raw.glb` | Jul 26 16:51, 22,465,440 B | present, Stage-2 consumed it successfully |
| `Cache/Longbowman/refine_report.json` | Jul 26 16:53 | complete + well-formed, `elapsed_seconds 10.1`, no truncation |
| `Content/RawAssets/Longbowman.fbx` | Jul 26 16:53, 601,068 B | valid `Kaydara FBX Binary` header; **PROVEN not truncated — `rig_character.py` re-imported it headlessly and read 14,999 tris / 7,496 verts / both slots / `UVMap`** |
| `T_Longbowman_{D,N,ORM}.png` | Jul 26 16:53 | all three decode clean at **1024² RGB** (PIL full `load()`, not just header) |
| 5× `Cache/Longbowman/previews/*.png` | Jul 26 16:53 | all present and render-complete |

**Stage 2 was NOT re-run** (it did not need to be, and re-running spends nothing but would churn the staged bytes). Stage 1 was never at risk.

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). `--check` PASSED this session: **2536 credits remaining**. **Longbowman consumed 0 credits this session** — Stage 1 was already complete in cache. Norton TLS: reused `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`. The rig stage is local (no network).

## Brightness override (PINNED — VERIFIED IN THE BAKE)
`pipeline_manifest.json` → `assets.Longbowman.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — the FLEET-DEFAULT profile validated on the Footman re-pilot (commit `d7254da`). Pin confirmed present, and the shipped `refine_report.json` confirms it was **actually applied to this bake**: `"applied": true` with exactly those four values.

**Mean linear albedo 0.1005 → 0.309** (p99 0.9446, headroom intact; shoulder-clamp 11.4%). Wash-out fixed — the green tunic reads emphatically GREEN, not grey.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Longbowman.fbx` | Raw STATIC mesh (`SM_Longbowman`) | 15,000 tris; slots `[TeamRegion, LongbowmanPBR]`; `UVMap`; feet-centre (min_z **-0.004** UE); bounds `[98.56, 117.95, 183.96]` UE |
| `Content/RawAssets/Textures/Longbowman/T_Longbowman_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Longbowman/T_Longbowman_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Longbowman/T_Longbowman_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Longbowman.fbx` | Rigged SKELETAL FBX (`SK_Longbowman` + armature `Footman_Rig`, 21 bones) | 14,999 tris; slots `[TeamRegion, LongbowmanPBR]`; `UVMap`; auto_heat skin **0.04% unweighted** |
| `Content/RawAssets/Characters/Longbowman.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Longbowman_{Idle,Walk,Attack,Death}.fbx` — all still **Jul 16 06:04**, byte-untouched, via `rig_character.py --no-anim-fbx`. In-engine `/Game/Characters/Anims/A_Longbowman_*` + shared ABP PRESERVED — do NOT reimport. (The rig report lists `animations: Idle/Walk/Attack/Death` — those are Blender-side preview actions rendered to contact strips only; **no anim FBX was written**.)

## How generated (reproducible)
- Stage 1 (pre-reboot, 16:51): `uv run meshy_generate.py --mode image3d Longbowman` → `Cache/Longbowman/meshy_raw.glb` (22.5 MB).
- Stage 2 (pre-reboot, 16:53): `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Longbowman --input Cache/Longbowman/meshy_raw.glb` (10.1 s).
- Rig (this session): `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Longbowman --no-anim-fbx` (exit 0, 46.8 s).

## Team-region (RECORDED)
`shoulder_caps` (single upward-facing band, z 0.70–0.86; the archer recipe — NO helm_dome, correct for a soft-hooded unit). Result **865 faces / 6.53% of area** (cap 35%).
**Landing shifted vs the M7 shipped mesh (558 faces / 3.8%)** — expected, it's a brand-new Meshy mesh. Selector left as-is per the TASK-194 `_verified` note.
**Watch-item (non-blocking):** on this mesh the band also catches the **nocked arrow shaft and the bow grip**, so those tint with team colour (visible in the beauty/bind previews as a blue line along the arrow). This is the same geometric family as the TASK-086 "shaft striping" misfire, but here it reads as *team-coloured equipment* rather than a defect, it is 6.5% (well under cap), and the shoulder/hood band — the actual team read — is present and correct. **Recorded, not re-tuned:** re-tuning the box without measuring against the new mesh risks losing the shoulder band entirely (a worse failure). If Jonathan dislikes the blue arrow, the lever is an X-narrowing of the `shoulder_caps` box, and it costs one ~10 s re-bake + ~47 s re-rig, **no Meshy credits**.

## Pre-import eyeball gate — **PASS** (read refine_report + rig_report, viewed previews vs `Concepts/Longbowman.png`)
- **Silhouette:** hooded archer at full draw — tall wooden longbow, nocked arrow, back quiver with white fletching, cross-body strap, belted green tunic with pointed skirt, tan trousers, tall brown boots, bracers, side dagger/tool pouches. Reads instantly as the concept from front, back, three-quarter and top.
- **Colour — VIVID, wash-out GONE (mean linear 0.1005 → 0.309):** saturated forest-green tunic, tan/cream hood, warm-brown leather bow/boots/straps, white fletching, skin tone all material-distinct. Emphatically not the washed-out grey the batch exists to fix. The pale patches on the shoulders/chest in the *flat* previews are the **TeamRegion placeholder slot**, not blown-out texture — confirmed by the Cycles beauty preview, where exactly those faces render placeholder-blue.
- **Geometry:** 15,000 tris static / 14,999 skeletal (budget 15,000); feet-centre `min_z -0.004`; `UVMap` present and correct; Z height-fit to 184 (183.96).
- **Bind:** `Footman_Rig`, 21 bones, deform set = the exact shared 20-bone contract; **0.04% unweighted** (auto_heat); no holes, no exploded verts, no collapsed limbs across bind_front/side/threequarter + the turntable strip.
- **M7 watch-item "bow-draw overshoot":** I looked for it specifically. On THIS new mesh the draw pose is clean — arrow nocked at the string, tip projecting forward past the riser exactly as the concept shows, no limb interpenetration or hand-through-bow. **Not reproduced; nothing to flag.** (Real judgement is still in-editor playback at integration, since the pose comes from the preserved anims, not this mesh.)
- **Verdict: PASS.**

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[98.80, 118.20]` deviate >10% from blockout target `[81.78, 57.38]`. BENIGN and expected: the drawn longbow (+X) and the forward-projecting arrow (Y depth) widen the footprint far past the TASK-166 blockout guess, which was measured off a 1,984-tri stand-in. `fit_mode=height` enforces Z only (184) and Z matched exactly. Manifest `target_dims_ue` left as-is (blockout provenance). **Build-master/integration note: Y depth 118 is ~2× the old blockout — sanity-check the capsule/selection bounds if anything looks off, same caution the Cavalry entry carries.**

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Longbowman/previews/preview_front.png` · `preview_threequarter.png` · `preview_back.png` · `preview_top.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Longbowman/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Longbowman/rig/previews/bind_{front,side,threequarter}.png` · `turntable_strip.png`
- Anim contact strips (preview only): `.../rig/previews/contact_{idle,walk,attack,death}.png`
- Concept: `Content/RawAssets/Concepts/Longbowman.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Longbowman_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Longbowman/T_Longbowman_D.png`
- `T_Longbowman_N` (**LINEAR**) ← `…/T_Longbowman_N.png`
- `T_Longbowman_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Longbowman_ORM.png`

### 2. Material instance `MI_Longbowman_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Longbowman_D, `Normal`=T_Longbowman_N, `ORM`=T_Longbowman_ORM.

### 3. Static mesh `SM_Longbowman`
- Import `Content/RawAssets/Longbowman.fbx` OVERWRITING `/Game/Meshes/SM_Longbowman` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe).
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `LongbowmanPBR` → `MI_Longbowman_PBR`. FBX slots already `[TeamRegion, LongbowmanPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Longbowman` (runtime visual)
- Import `Content/RawAssets/Characters/Longbowman.fbx` OVERWRITING `/Game/Characters/SK_Longbowman` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — do NOT create a new skeleton (FBX carries the exact 21-bone contract → clean bind). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, LongbowmanPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Longbowman_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched (still Jul 16) — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/Longbowman.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_Longbowman` is the runtime visual, VIVID matching `Concepts/Longbowman.png`; slot-0 team recolour tints the shoulder band (and the arrow shaft — see watch-item) Blue/Red; feet meet ground (TASK-307 systemic capsule-relative fix must be live); preserved anims still play; LOD count 3; Message Log clean. Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git touched. **TASKBOARD.md NOT written** (concurrent art-director agents — orchestrator records status). `pipeline_manifest.json` NOT written either: the Longbowman `albedo_delight` pin was already present and correct, so I verified rather than re-wrote it (avoids a write race with the other in-flight units). Files written this session: `Content/RawAssets/Characters/Longbowman.fbx`, `Content/RawAssets/Characters/Longbowman.lod.json`, the gitignored `Cache/Longbowman/rig/**`, and this handoff.
