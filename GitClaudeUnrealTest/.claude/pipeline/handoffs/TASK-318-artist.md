# TASK-318 — Pikeman FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Meshy image3d → Stage-2 refine → rig, all headless). All assets staged on disk at the EXISTING Pikeman raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26/27 · **FULL pipeline from scratch** — the 2026-07-26 ~16:58 reboot killed the fan-out before Pikeman started (no `meshy_raw.glb` existed; its raw FBX was still the old 2026-07-21 one). Ran the PROVEN Footman pipeline (validated commit `d7254da`) end to end.

## MESHY gate
Token env-only (redacted; never echoed/logged/on-argv). `--check` PASSED (**2536** credits before run). Stage-1 consumed **30** credits (**2536 → 2506**). Norton TLS: reused `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`. Stage-2 + rig are local (no network).
**Meshy task `019fa14a-cdf8-70c4-96c2-0f3f1781032d`**, engine `meshy-i23d`, concept `Inbox/Pikeman.png` (sha256 `75e4d742e417…`, verified **byte-identical** to the approved `Content/RawAssets/Concepts/Pikeman.png`). Donor `Cache/Pikeman/meshy_raw.glb`, 22,504,832 B.

## Brightness override (PINNED — VERIFIED IN THE BAKE)
`pipeline_manifest.json` → `assets.Pikeman.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}`. Pikeman was ALREADY in the recorded-dark override set (TASK-225 pin) at **exactly** the FLEET-DEFAULT values validated on the Footman re-pilot — so the required profile was already in place before refining; I verified rather than re-wrote it (avoids a write race with the other in-flight units). `refine_report.json` confirms it was applied to this bake: `"applied": true` with those four values.

**Mean linear albedo 0.0567 → 0.2107** (p99 0.927; shoulder-clamp only **4.56%** — the healthiest headroom of the batch so far vs Cleric 13.4% / Longbowman 11.4%). Wash-out fixed with room to spare.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Pikeman.fbx` | Raw STATIC mesh (`SM_Pikeman`) | 15,000 tris; slots `[TeamRegion, PikemanPBR]`; `UVMap`; feet-centre (min_z **-0.061** UE); bounds `[191.28, 130.72, 189.72]` UE — **see the WIDE-FOOTPRINT flag below** |
| `Content/RawAssets/Textures/Pikeman/T_Pikeman_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Pikeman/T_Pikeman_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Pikeman/T_Pikeman_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Pikeman.fbx` | Rigged SKELETAL FBX (`SK_Pikeman` + armature `Footman_Rig`, 21 bones) | 14,999 tris; slots `[TeamRegion, PikemanPBR]`; `UVMap`; auto_heat skin **0.04% unweighted** |
| `Content/RawAssets/Characters/Pikeman.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Pikeman_{Idle,Walk,Attack,Death}.fbx` — all still **Jul 16 06:01**, byte-untouched, via `rig_character.py --no-anim-fbx`. In-engine `/Game/Characters/Anims/A_Pikeman_*` + shared ABP PRESERVED — do NOT reimport. (The rig report lists `animations: Idle/Walk/Attack/Death` — Blender-side preview actions rendered to contact strips only; **no anim FBX was written**.)

## How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Pikeman` → `Cache/Pikeman/meshy_raw.glb` (22.5 MB).
- Stage 2: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Pikeman --input Cache/Pikeman/meshy_raw.glb` (exit 0, 9.2 s).
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Pikeman --no-anim-fbx` (exit 0, 40.3 s).

## Team-region (RECORDED)
Two selectors — `helm_dome` (x-constrained 0.18–0.82, z 0.86–1.0) + `shoulder_caps` (upward-facing, z 0.70–0.85). Result **1,549 faces / 10.31% of area** (cap 35%) — within a whisker of the M7 shipped landing (1,318 faces / 9.1%), so the `_verified` selectors transferred cleanly to the new mesh.
**IMPROVEMENT vs the shipped mesh:** the manifest `_verified` note recorded that the z-bands sat slightly LOW because a *raised* pike defined mesh height. On this generation both pikes are held **horizontally**, so height is defined by the helm and the bands land where intended — helm crown + shoulders + upper chest, face/visor spared. Confirmed in the Cycles beauty + bind previews.
**Watch-item (non-blocking):** a short segment of the upper pike shaft crosses the shoulder z-band and tints team colour (same geometric family as the Longbowman arrow this batch). Small, reads as a team-coloured haft, well under cap. Recorded, not re-tuned.

## Pre-import eyeball gate — **PASS** (read refine_report + rig_report, viewed all 5 Stage-2 previews + bind previews vs `Concepts/Pikeman.png`)
- **Silhouette:** helmeted pikeman in a braced two-handed guard — conical steel helm with dark visor slit, mail/plate pauldrons and sleeves, long tabard over gambeson, belt with pouches + scroll/quiver cases, knee cops, tall brown boots, and **two crossed pikes with steel leaf tips**. Reads instantly as the concept.
- **Colour — VIVID, wash-out GONE (mean linear 0.0567 → 0.2107):** the **back view is the clearest read** and matches the concept's palette directly — olive/khaki tabard, blue-grey trousers, warm-brown boots and pike shafts, bright steel helm and spear tips, teal knee cops. All material-distinct, no muddy grey.
  - *Read-the-preview caution for whoever reviews these:* the **front/three-quarter flat previews look pale at first glance** because the `TeamRegion` slot (helm + shoulders + upper chest, 10.3% of area) renders as a light PLACEHOLDER there. That is not blown-out texture — the Cycles beauty preview shows exactly those faces as placeholder-blue, and the back view (almost no team region) shows the true saturated palette. Verified, not a defect.
- **Geometry:** 15,000 tris static / 14,999 skeletal (budget 15,000); feet-centre `min_z -0.061`; `UVMap` present and correct; Z height-fit to 190 (189.72).
- **Bind:** `Footman_Rig`, 21 bones, exact shared deform contract; **0.04% unweighted** (auto_heat); no holes, no exploded verts, clean limbs across bind_front/side/threequarter.
- **Generative addition (non-blocking):** the mesh gives him a **cloak/mantle hanging from one shoulder** that the concept does not show. It is well-formed, olive, and consistent with the costume — it reads as a period-correct campaign cloak, not an artifact. Recorded for Jonathan; not a reason to burn 30 more credits.
- **Verdict: PASS.**

### ⚠️ WIDE FOOTPRINT — the one thing to actually watch (warn-only in the report, but flagged UP)
`refine_report` warns conformed dims `[192.20, 131.20, 190.0]` vs blockout target `[76.84, 60.34, 190.0]`. `fit_mode=height` enforces Z only and **Z matched exactly (190)** — so this is warn-only by law, BUT the magnitude is worth a human look:
- **X is 191 UE vs a 77 target — ~2.5×.** Cause: both pikes are held **horizontally and crossed**, spanning left–right, where the M7 shipped mesh held one pike **raised** (vertical). Y depth 131 vs 60 for the same reason (one pike angles forward).
- **Consequence to check at integration:** unit selection/collision bounds and formation spacing. A ~1.9 m-wide silhouette will visually overlap neighbouring units in a packed line, and the ≤4 simple hulls generated at import will be much wider than the body. The **capsule** is authored in C++ (not from these bounds), so movement/collision should be unaffected — but **eyeball a packed Pikeman line in PIE** before signing off.
- Manifest `target_dims_ue` left as-is (blockout provenance). If Jonathan wants the tighter shipped silhouette, the fix is a **re-gen with a raised-pike concept framing — that costs 30 credits**, so it is his call, not mine.

## Preview paths (to show Jonathan)
- Flat-lit true-albedo: `Tools/ArtPipeline/Cache/Pikeman/previews/preview_back.png` (**best colour read — least team-region placeholder**) · `preview_front.png` · `preview_threequarter.png` · `preview_top.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Pikeman/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Pikeman/rig/previews/bind_{front,side,threequarter}.png` · `turntable_strip.png`
- Anim contact strips (preview only): `.../rig/previews/contact_{idle,walk,attack,death}.png`
- Concept: `Content/RawAssets/Concepts/Pikeman.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Pikeman_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Pikeman/T_Pikeman_D.png`
- `T_Pikeman_N` (**LINEAR**) ← `…/T_Pikeman_N.png`
- `T_Pikeman_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Pikeman_ORM.png`

### 2. Material instance `MI_Pikeman_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Pikeman_D, `Normal`=T_Pikeman_N, `ORM`=T_Pikeman_ORM.

### 3. Static mesh `SM_Pikeman`
- Import `Content/RawAssets/Pikeman.fbx` OVERWRITING `/Game/Meshes/SM_Pikeman` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe) — note the wide pike span above.
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `PikemanPBR` → `MI_Pikeman_PBR`. FBX slots already `[TeamRegion, PikemanPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Pikeman` (runtime visual)
- Import `Content/RawAssets/Characters/Pikeman.fbx` OVERWRITING `/Game/Characters/SK_Pikeman` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — do NOT create a new skeleton (FBX carries the exact 21-bone contract → clean bind). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, PikemanPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Pikeman_*`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs untouched (still Jul 16) — nothing new to import.

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/Pikeman.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_Pikeman` is the runtime visual, VIVID matching `Concepts/Pikeman.png`; slot-0 team recolour tints helm + shoulder band Blue/Red; feet meet ground (TASK-307 systemic capsule-relative fix must be live); preserved anims still play; LOD count 3; Message Log clean. **Additionally: eyeball a packed line of Pikemen for pike-span overlap (see WIDE FOOTPRINT).** Commit this ONE unit on main, NO push.

## Discipline
NO editor / MCP / Blueprint / Git touched. **TASKBOARD.md NOT written** (concurrent art-director agents — orchestrator records status). `pipeline_manifest.json` NOT written (pin already correct — verified, see above). Files written this session: `Content/RawAssets/Pikeman.fbx`, the 3 staged PNGs, `Content/RawAssets/Characters/Pikeman.fbx`, `Content/RawAssets/Characters/Pikeman.lod.json`, the gitignored `Cache/Pikeman/**` (donor GLB, report, previews, rig), and this handoff.
