# TASK-320 — MilitiaMob FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Stage-1/2 recovered from disk after the 16:58 reboot → rig re-run, all headless). All assets staged at the EXISTING MilitiaMob raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched. TASKBOARD NOT written (concurrent-agent race; orchestrator records status).
**Date:** 2026-07-26 · Resumed the FLEET-REMASTER batch after the PC reboot killed the in-flight fan-out. Stage 1 (Meshy) was ALREADY complete on disk — **NO new Meshy credits consumed, Stage 1 NOT re-run.**

## Integrity check of the reboot-interrupted Stage-1/2 output — **PASS (no re-run needed)**
| Check | Result |
|---|---|
| `Content/RawAssets/MilitiaMob.fbx` | 599,132 B, `Kaydara FBX Binary` magic + valid footer sentinel, 2026-07-26 16:53 |
| `T_MilitiaMob_{D,N,ORM}.png` | 1,241,408 / 1,073,006 / 1,187,171 B — all three: valid PNG signature **and** canonical terminating `IEND` chunk (CRC `ae426082`) ⇒ not truncated |
| `Cache/MilitiaMob/refine_report.json` | parses; complete `result` block + `elapsed_seconds: 11.2` ⇒ Stage 2 ran to completion (report is written last) |
| 5 Stage-2 previews | all present, all 16:53 |
| Concept provenance | `Tools/ArtPipeline/Inbox/MilitiaMob.png` **byte-identical** (sha256 `c7d4b675…`) to approved `Content/RawAssets/Concepts/MilitiaMob.png` |

**Brightness profile — FLEET-DEFAULT confirmed pinned + applied.** `pipeline_manifest.json` → `assets.MilitiaMob.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — byte-equal to the Footman-validated (`d7254da`) fan-out default. `refine_report.albedo_delight.applied = true` with those exact values. **Mean linear albedo 0.0376 → 0.1906** (p99 0.8659, only **1.72%** shoulder-clamped ⇒ highlight headroom intact). Lands slightly *above* Footman's validated 0.164 — the brightest and best-reading of the two units in this batch. The manifest `_note` still carries the older TASK-225 wording ("temper gamma toward 0.65 if the rebake over-lifts") — **not needed: it did not over-lift** (1.72% clamp). Values left as-is; no temper applied.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/MilitiaMob.fbx` | Raw STATIC mesh (`SM_MilitiaMob`) | 15,000 tris; slots `[TeamRegion, MilitiaMobPBR]`; `UVMap`; bounds `[110.16, 55.36, 148.70]` UE; feet-centre min_z `0.012` |
| `Content/RawAssets/Textures/MilitiaMob/T_MilitiaMob_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/MilitiaMob/T_MilitiaMob_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/MilitiaMob/T_MilitiaMob_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/MilitiaMob.fbx` | Rigged SKELETAL FBX (`SK_MilitiaMob` + armature `Footman_Rig`, 21 bones) | 15,000 tris / 7,500 verts; slots `[TeamRegion, MilitiaMobPBR]`; `UVMap`; **auto_heat** skin, **0.0%** unweighted |
| `Content/RawAssets/Characters/MilitiaMob.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/MilitiaMob_{Idle,Walk,Attack,Death}.fbx` (still Jul-16 06:02, mtime unchanged) via `rig_character.py --no-anim-fbx` (`rig_report.outputs.anim_fbx = None`). In-engine `/Game/Characters/Anims/A_MilitiaMob_*` + shared ABP PRESERVED — do NOT reimport.

## Skeleton (RECORDED)
Shared **`/Game/Characters/SK_Footman_Skeleton`** (root bone `Footman_Rig`, 21 bones). Rig spec `SiegeBiped` per `rig_manifest.json` → `assets.MilitiaMob.skeleton`. Verified present in the exported FBX: `Footman_Rig`, `pelvis`, `spine_01..03`, `neck_01`, `head`, `clavicle/upperarm/lowerarm/hand_{l,r}`, `thigh/calf/foot_{l,r}`.

## Swarm note (gameplay-side: NOTHING changes)
MilitiaMob is the **Swarm** card — this is the **SINGLE** mesh; the runtime spawns **4 copies** (`SwarmCount 4`). The remaster swaps the one mesh only; no gameplay, no spawn count, no card data touched. On-field the 4-up spawn multiplies the team-colour read, which is why the shoulder-band recipe was accepted for this unit at the TASK-194 wave gate.

**Height-normalized amplitude-floor law — N/A this task, recorded for completeness.** CONVENTIONS' `40 uu × (unit height / 1.75 m)` floor (MilitiaMob @ 1.49 m ⇒ **≈34 uu**) governs the **retarget** path (`retarget_meshy_to_siegebiped.py`) and Walk-clip acceptance. This task ran `rig_character.py --no-anim-fbx`: **no anim FBX was exported and no shipped clip was altered** — the four existing `A_MilitiaMob_*` sequences are preserved untouched, so their already-accepted amplitude is unchanged. Nothing in the rig path touched amplitude.

## How generated (reproducible)
- Stage 1: **NOT re-run** — reused the cached `Cache/MilitiaMob/meshy_raw.glb` (22.9 MB, 16:52). Zero new credits.
- Stage 2: **NOT re-run** — the 16:53 output passed integrity + profile checks above.
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id MilitiaMob --no-anim-fbx` (exit 0, 46.0 s, Blender 5.1.2, zero warnings).

## Team-region (RECORDED)
`shoulder_caps` (single upward-facing band, `min_dot 0.55`, z-band 0.66–0.84; **no `helm_dome`** — keeps the green riveted helm and the bearded face unpainted). Result **571 faces / 4.61% of area** (cap 35%). Isolated to the shoulder/mantle/scarf shelf and the shield rim; helmet, face, tunic body, pitchfork, boots all SPARED. **Improved** on the TASK-194 shipped 2.8% / 413 faces — the strongest team read of the two units in this batch. Placeholder blue confirmed isolated in the beauty preview.

## Pre-import eyeball gate — **PASS** (read refine_report + rig_report, viewed previews vs `Concepts/MilitiaMob.png`)
- **Silhouette:** green riveted kettle-helm, dark full beard, cream/tan wrapped scarf, olive peasant tunic with torn hem, brown leather cross-belts + buckled waist belt + hip pouch, brown round shield with teal-green rim and central boss, **wooden pitchfork in the right hand**, tan cuffed trousers, grey-blue gaiters, brown boots. Reads instantly as the concept from front and three-quarter — one of the closest concept matches in the batch. **PASS**
- **Geometry:** 15,000 tris static and skeletal (budget 15,000) ✅ · `UVMap` ok ✅ · Z height-fit 148.70 (target 149) ✅ · feet-centre origin min_z **0.012 UE** ✅
- **Bind:** **0.0% unweighted**, `auto_heat`; bind previews show an intact silhouette, no exploded verts, no holes, feet planted, pitchfork and shield held correctly. **PASS**
- **Colour — VIVID, wash-out GONE (mean linear 0.1906):** green helm, cream scarf, olive tunic, brown leather, brown+teal shield, warm wood fork, skin/beard — all clearly material-distinct and saturated. Emphatically not the old flat-grey failure mode. **PASS**
- **Verdict: PASS.**

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[110.20, 55.70]` deviate >10% from blockout target `[74.52, 34.32]`. **BENIGN** — the pitchfork held out at arm's length (+X) and the shield on the off arm widen the footprint well past the TASK-166 bare-peasant blockout guess. `fit_mode=height` enforces **Z only (149)**, which landed exactly (148.70). Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/MilitiaMob/previews/preview_front.png` · `preview_threequarter.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/MilitiaMob/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/MilitiaMob/rig/previews/bind_front.png` · `bind_threequarter.png`
- Concept: `Content/RawAssets/Concepts/MilitiaMob.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_MilitiaMob_D` (**sRGB ON**) ← `Content/RawAssets/Textures/MilitiaMob/T_MilitiaMob_D.png`
- `T_MilitiaMob_N` (**LINEAR**) ← `…/T_MilitiaMob_N.png`
- `T_MilitiaMob_ORM` (**LINEAR, sRGB OFF**) ← `…/T_MilitiaMob_ORM.png`

### 2. Material instance `MI_MilitiaMob_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_MilitiaMob_D, `Normal`=T_MilitiaMob_N, `ORM`=T_MilitiaMob_ORM.

### 3. Static mesh `SM_MilitiaMob`
- Import `Content/RawAssets/MilitiaMob.fbx` OVERWRITING `/Game/Meshes/SM_MilitiaMob` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe).
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `MilitiaMobPBR` → `MI_MilitiaMob_PBR`. FBX slots already `[TeamRegion, MilitiaMobPBR]` → map 1:1.

### 4. Skeletal mesh `SK_MilitiaMob` (runtime visual)
- Import `Content/RawAssets/Characters/MilitiaMob.fbx` OVERWRITING `/Game/Characters/SK_MilitiaMob` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — do NOT create a new skeleton (the FBX carries the exact 21-bone contract ⇒ clean bind). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, MilitiaMobPBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_MilitiaMob_{Idle,Walk,Attack,Death}`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs byte-untouched — nothing new to import. After the SK overwrite, confirm all four still bind (same skeleton ⇒ they must).

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/MilitiaMob.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_MilitiaMob` is the runtime visual, VIVID matching `Concepts/MilitiaMob.png`; slot-0 team recolour tints the shoulder band Blue/Red; feet meet ground (TASK-308 live); the four preserved anims still play; **all 4 swarm copies spawn and read as a unit**; LOD count 3; Message Log clean. Commit this ONE unit on main, NO push.

### Known fleet-wide non-blocker (carried from the TASK-311 verify)
`M_AssetPBR.used_with_skeletal_mesh = false` ⇒ a "missing usage flag" warning on every SK unit; auto-heals in editor/PIE but should be baked into the master for cooked builds. Not a MilitiaMob defect — do not chase it here.

## Discipline
NO editor / MCP / Blueprint / Git touched. NO TASKBOARD write (concurrent-agent race). NO Stage-1 re-run (zero Meshy credits). Files written: the 2 rig outputs (`Characters/MilitiaMob.fbx`, `Characters/MilitiaMob.lod.json`), the gitignored `Cache/MilitiaMob/rig/**` (report + previews), and this handoff. The Stage-2 artifacts (`RawAssets/MilitiaMob.fbx`, the 3 PNGs) were left exactly as the pre-reboot run wrote them.
