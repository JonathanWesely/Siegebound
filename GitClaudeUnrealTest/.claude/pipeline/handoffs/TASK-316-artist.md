# TASK-316 — Ogre FLEET-REMASTER (M7.9) — art-director handoff

**Status:** GENERATION DONE (Stage-1/2 recovered from disk after the 16:58 reboot → rig re-run, all headless). All assets staged at the EXISTING Ogre raw paths (clean same-path overwrite). **UE import NOT done — deferred (editor-gated). Build-master imports via the remote-exec lane.** NO editor / MCP / Blueprint / Git touched. TASKBOARD NOT written (concurrent-agent race; orchestrator records status).
**Date:** 2026-07-26 · Resumed the FLEET-REMASTER batch after the PC reboot killed the in-flight fan-out. Stage 1 (Meshy) was ALREADY complete on disk — **NO new Meshy credits consumed, Stage 1 NOT re-run.**

---

## ⚠️ RESOLVED FINDING — Ogre binds the SHARED `SK_Footman_Skeleton`, NOT a bespoke `SK_Ogre_Skeleton`

**The board's unit table (TASKBOARD.md line ~904) and CONVENTIONS.md line ~329 both claim Ogre keeps an "EXISTING bespoke `SK_Ogre_Skeleton`". That asset does not exist and never did.** The correct bind target is the SHIPPED shared **`/Game/Characters/SK_Footman_Skeleton`** (root bone `Footman_Rig`). Evidence, strongest first:

1. **`Tools/ArtPipeline/rig_manifest.json` → `assets.Ogre` carries an explicit MANAGER LANE RULING (TASK-242)** that overrides the "bespoke" option by name:
   > `"skeleton": "SiegeBiped"` … *"MANAGER LANE RULING (TASK-242): rigged on the SHARED SiegeBiped (Footman_Rig root) like every humanoid, NOT the _doc's 'bespoke' option — the adaptive height/half-width fit absorbs the bulk; Meshy skeleton is CLIPS ONLY."*
2. **`Tools/ArtPipeline/rig_character.py` line 105:** `SHARED_SKELETON_ROOT = "Footman_Rig"` is a module CONSTANT, not per-unit — *every* unit exports its armature object under that name, so UE's importer always produces the same root bone.
3. **The SHIPPED pre-remaster rig report** (`Cache/Ogre/rig/rig_report.json`, run 2026-07-22T04:08Z, the run that produced the shipped `Content/RawAssets/Characters/Ogre.fbx`): `"skeleton": "SiegeBiped"`, `"armature": {"name": "Footman_Rig", "bone_count": 21}`.
4. **`handoffs/TASK-244.md` line 30 — in-editor structural readback at commit time:** *"SK_Archer/SK_Ogre exist, **bound to shared `SK_Footman_Skeleton`**."*
5. **`handoffs/TASK-243.md` line 44:** SK_Ogre resolves `anim_inst=ABP_Footman_C` — the shared ABP, which can only bind on the shared skeleton.
6. **Build-master TASK-311 verify (commit `d7254da`):** `SK_Footman_Skeleton` is the **sole** skeleton asset in `/Game/Characters`, **no strays** — so no `SK_Ogre_Skeleton` exists to bind to.

**Likely source of the doc error:** `rig_character.py` line 406 writes `report["armature"]["skeleton"] = card_id`, so the Ogre rig report displays a cosmetic `"skeleton": "Ogre"` field *inside* the armature block. That is the **card id**, not a skeleton asset name — the authoritative field is the top-level `"skeleton": "SiegeBiped"`. Reading the nested field as an asset name would produce exactly the "bespoke SK_Ogre_Skeleton" claim.

**Action taken:** rigged to `SiegeBiped` / `Footman_Rig` (the shipped contract). **Build-master: bind the same-path `SK_Ogre` reimport to the EXISTING `/Game/Characters/SK_Footman_Skeleton`. Do NOT create a new skeleton** — that would orphan the four `A_Ogre_*` sequences. Board/CONVENTIONS text needs the correction (manager/orchestrator lane — I did not edit the board).

---

## Integrity check of the reboot-interrupted Stage-1/2 output — **PASS (no re-run needed)**
| Check | Result |
|---|---|
| `Content/RawAssets/Ogre.fbx` | 621,340 B, `Kaydara FBX Binary` magic + valid footer sentinel, 2026-07-26 16:54 |
| `T_Ogre_{D,N,ORM}.png` | 933,394 / 750,273 / 889,066 B — all three: valid PNG signature **and** canonical terminating `IEND` chunk (CRC `ae426082`) ⇒ not truncated |
| `Cache/Ogre/refine_report.json` | parses; complete `result` block + `elapsed_seconds: 14.6` ⇒ Stage 2 ran to completion (report is written last) |
| 5 Stage-2 previews | all present, all 16:54 |
| Concept provenance | `Tools/ArtPipeline/Inbox/Ogre.png` **byte-identical** (sha256 `323d0204…`) to approved `Content/RawAssets/Concepts/Ogre.png` |

**Brightness profile — FLEET-DEFAULT confirmed pinned + applied.** `pipeline_manifest.json` → `assets.Ogre.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — byte-equal to the Footman-validated (`d7254da`) fan-out default. `refine_report.albedo_delight.applied = true` with those exact values. **Mean linear albedo 0.0202 → 0.1416** (p99 0.6345, only **0.22%** shoulder-clamped ⇒ full highlight headroom, no wash-out). Sits just under Footman's validated 0.164 — expected, the Ogre concept is a genuinely desaturated mossy-green/weathered-leather palette.

## Assets staged (same-path overwrite; LOD sidecar refreshed)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Ogre.fbx` | Raw STATIC mesh (`SM_Ogre`) | 15,000 tris; slots `[TeamRegion, OgrePBR]`; `UVMap`; bounds `[251.72, 197.80, 290.01]` UE; min_z `-0.207` |
| `Content/RawAssets/Textures/Ogre/T_Ogre_D.png` | Base colour (sRGB on import) | 1024², brighter de-lit albedo |
| `Content/RawAssets/Textures/Ogre/T_Ogre_N.png` | Normal (LINEAR) | 1024² |
| `Content/RawAssets/Textures/Ogre/T_Ogre_ORM.png` | Occlusion/Roughness/Metallic (LINEAR, sRGB OFF) | 1024² |
| `Content/RawAssets/Characters/Ogre.fbx` | Rigged SKELETAL FBX (`SK_Ogre` + armature `Footman_Rig`, 21 bones) | 14,995 tris / 7,476 verts; slots `[TeamRegion, OgrePBR]`; `UVMap`; **auto_heat** skin, **0.08%** unweighted |
| `Content/RawAssets/Characters/Ogre.lod.json` | SK-LOD recipe (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched):** `Content/RawAssets/Characters/Anims/Ogre_{Idle,Walk,Attack,Death}.fbx` (still Jul-21 21:08, mtime unchanged) via `rig_character.py --no-anim-fbx` (`rig_report.outputs.anim_fbx = None`). In-engine `/Game/Characters/Anims/A_Ogre_*` + shared ABP PRESERVED — do NOT reimport.

**Skin-quality improvement vs the shipped rig:** the pre-remaster Ogre fell back to `envelope` skinning (`method_used: "envelope"`). This rebuild succeeded on **`auto_heat`** — the preferred bone-heat path, same as the rest of the fleet. Unweighted 0.08% (≈6 of 7,476 verts).

## How generated (reproducible)
- Stage 1: **NOT re-run** — reused the cached `Cache/Ogre/meshy_raw.glb` (27.7 MB, 16:52). Zero new credits.
- Stage 2: **NOT re-run** — the 16:54 output passed integrity + profile checks above.
- Rig: `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Ogre --no-anim-fbx` (exit 0, 45.9 s, Blender 5.1.2, zero warnings).

## Team-region (RECORDED)
`shoulder_caps` (single upward-facing band, `min_dot 0.55`, z-band 0.66–0.84; **no `helm_dome`** — the ogre is bare-horned, a dome would paint his face/horns per the TASK-087 Archer lesson). Result **187 faces / 1.33% of area** (cap 35%). Isolated to the spiked pauldron + shoulder/chest-strap line; head, horns, face, maul, feet all SPARED. Confirmed visually — placeholder blue reads as two shoulder patches + a diagonal chest strap in the beauty preview.
**Non-blocking nit:** 1.33% is *down* from the TASK-194 shipped 2.1% / 353 faces (the Meshy silhouette moved geometry out of the selector box). Still legible on the roster's tallest unit, but it is the weakest team read of the units I have handled. If the PIE verify finds the Red/Blue read too subtle, the lever is widening the z-band to ~0.62–0.86 and re-running **Stage 2 only** — no new credits.

## Pre-import eyeball gate — **PASS** (read refine_report + rig_report, viewed previews vs `Concepts/Ogre.png`)
- **Silhouette:** horned/tusked brute, spiked shoulder pauldron, layered hide armour with chained straps, wide belt + pouches, tattered loincloth skirt, bone-and-stone maul in the right fist, big bare feet. Reads instantly as the concept from front and three-quarter. **PASS**
- **Geometry:** 15,000 tris static / 14,995 skeletal (budget 15,000) ✅ · `UVMap` ok ✅ · Z height-fit 290.01 ✅ · origin feet-centre (min_z **-0.207 UE = 2 mm** below ground — negligible, effectively feet-centre; TASK-308's systemic capsule-relative grounding handles the runtime offset) ✅
- **Bind:** **0.08% unweighted**, `auto_heat`; bind previews show an intact silhouette, no exploded verts, no holes, feet planted. **PASS**
- **Colour — VIVID, wash-out GONE (mean linear 0.1416):** green skin with pink/red inner-ear and facial detail, brown weathered leather, grey stone maul head, bone-white maul spikes, dark chain — all material-distinct. Emphatically not the old flat-grey failure mode. **PASS**
- **Verdict: PASS.**

### Warn-only (non-blocking)
`refine_report` warns conformed X/Y `[252.10, 197.60]` deviate >10% from blockout target `[226.72, 98.80]`. **BENIGN and expected** — the Ogre is non-humanoid and far broader than the TASK-166 blockout guess (the Y target of 98.8 was authored for a humanoid footprint; the real ogre is 197.6 deep with the maul overhang and hunched bulk). `fit_mode=height` enforces **Z only (290)**, which landed exactly. Manifest `target_dims_ue` left as-is (blockout provenance).

## Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Ogre/previews/preview_front.png` · `preview_threequarter.png`
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Ogre/previews/preview_beauty_cycles.png`
- Rig bind: `Tools/ArtPipeline/Cache/Ogre/rig/previews/bind_front.png` · `bind_threequarter.png`
- Concept: `Content/RawAssets/Concepts/Ogre.png`

---

## TURNKEY same-path IMPORT recipe (deferred, editor-gated build-master)
Serialized, EXCLUSIVE editor session. **NEVER delete+recreate — same-path OVERWRITE preserves all refs.** Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Ogre_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Ogre/T_Ogre_D.png`
- `T_Ogre_N` (**LINEAR**) ← `…/T_Ogre_N.png`
- `T_Ogre_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Ogre_ORM.png`

### 2. Material instance `MI_Ogre_PBR` (reassign params; do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Ogre_D, `Normal`=T_Ogre_N, `ORM`=T_Ogre_ORM.

### 3. Static mesh `SM_Ogre`
- Import `Content/RawAssets/Ogre.fbx` OVERWRITING `/Game/Meshes/SM_Ogre` at the SAME path. **Nanite OFF.** Import Normals. Collision ≤4 simple hulls (TASK-037 unit recipe).
- Slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_Blue` (default); slot 1 `OgrePBR` → `MI_Ogre_PBR`. FBX slots already `[TeamRegion, OgrePBR]` → map 1:1.

### 4. Skeletal mesh `SK_Ogre` (runtime visual)
- Import `Content/RawAssets/Characters/Ogre.fbx` OVERWRITING `/Game/Characters/SK_Ogre` at the SAME path. **BIND to the EXISTING shared `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`) — **do NOT create a new skeleton, and do NOT look for `SK_Ogre_Skeleton` (it does not exist — see the RESOLVED FINDING at the top).** The FBX carries the exact 21-bone SiegeBiped contract (`Footman_Rig`, `pelvis`, `spine_01..03`, `neck_01`, `head`, `clavicle/upperarm/lowerarm/hand_{l,r}`, `thigh/calf/foot_{l,r}`) ⇒ clean bind. **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, OgrePBR]`.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Ogre_{Idle,Walk,Attack,Death}`; do NOT touch the shared ABP. `--no-anim-fbx` left the raw anim FBXs byte-untouched — nothing new to import. After the SK overwrite, confirm all four still bind (same skeleton ⇒ they must).

### 6. SK-LOD chain — REGENERATE post-import
- Apply `Content/RawAssets/Characters/Ogre.lod.json`: LOD1 50%@0.4 / LOD2 20%@0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction. Readback `lod_count == 3`. URO already set in C++ on `SkeletalVisualMesh`.

### 7. Acceptance
- `SK_Ogre` is the runtime visual, VIVID matching `Concepts/Ogre.png`; slot-0 team recolour tints the pauldron/shoulder band Blue/Red; feet meet ground (TASK-308 live); the four preserved anims still play; LOD count 3; Message Log clean. Commit this ONE unit on main, NO push.

### Known fleet-wide non-blocker (carried from the TASK-311 verify)
`M_AssetPBR.used_with_skeletal_mesh = false` ⇒ a "missing usage flag" warning on every SK unit; auto-heals in editor/PIE but should be baked into the master for cooked builds. Not an Ogre defect — do not chase it here.

## Discipline
NO editor / MCP / Blueprint / Git touched. NO TASKBOARD write (concurrent-agent race). NO Stage-1 re-run (zero Meshy credits). Files written: the 2 rig outputs (`Characters/Ogre.fbx`, `Characters/Ogre.lod.json`), the gitignored `Cache/Ogre/rig/**` (report + previews), and this handoff. The Stage-2 artifacts (`RawAssets/Ogre.fbx`, the 3 PNGs) were left exactly as the pre-reboot run wrote them.
