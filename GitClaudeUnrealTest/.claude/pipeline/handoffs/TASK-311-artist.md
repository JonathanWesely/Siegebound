# TASK-311 — Footman FLEET-REMASTER (PILOT) — art-director handoff

**Status:** GENERATION DONE (both `-model` Meshy→Stage-2 AND `-rig` Blender-headless). All assets staged on disk at the EXISTING Footman raw paths (clean same-path overwrite). **UE import NOT done — deliberately deferred (editor-gated, per spec).** NO editor / MCP / Blueprint / deck-builder / Wizard asset touched. Git NOT mutated (a single read-only `git status` used for verification only — no add/commit/branch/checkout).
**Date:** 2026-07-26 · **Branch:** m7.6-arena10x · **PILOT for the 11-unit FLEET-REMASTER (TASK-311..321).**

## Why this unit first
Footman is the PILOT: confirm the existing-concept → Meshy image-to-3D flow produces VIVID colour matching the concept (the OLD non-Meshy meshes render washed-out) before fanning out the other 10. **Colour verdict below — this is the go/no-go signal.**

## MESHY_TOKEN gate
PRESENT (env-only, redacted throughout — never echoed/logged/on-argv). `--check` PASSED: API reachable, 2836 credits before run. Stage-1 consumed **30** credits (2836 → 2806). No quota block. Norton TLS: the Meshy call reused the combined CA bundle `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`. Stage-2 refine + rig are local (no network, no certs).

## Assets staged (on disk) — ALL same-path overwrite except the NEW LOD sidecar
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Footman.fbx` | Raw game-ready STATIC mesh (object `SM_Footman`) | OVERWRITE. 600,268→new; 15,000 tris; slots `[TeamRegion, FootmanPBR]`; `UVMap`; feet-centre (min_z 0.008 UE); bounds `[80.14, 72.32, 179.62]` UE |
| `Content/RawAssets/Textures/Footman/T_Footman_D.png` | Base colour (sRGB on import) | OVERWRITE. 1024², de-lit albedo |
| `Content/RawAssets/Textures/Footman/T_Footman_N.png` | Normal (LINEAR on import) | OVERWRITE. 1024² |
| `Content/RawAssets/Textures/Footman/T_Footman_ORM.png` | Occlusion/Roughness/Metallic packed (LINEAR, sRGB OFF) | OVERWRITE. 1024² |
| `Content/RawAssets/Characters/Footman.fbx` | Rigged SKELETAL FBX (object `SK_Footman` + armature `Footman_Rig`) | OVERWRITE. 788,412 bytes; 14,999 tris / 7,500 verts; 21-bone SiegeBiped; slots `[TeamRegion, FootmanPBR]`; `UVMap`; skin auto_heat **0.0% unweighted** |
| `Content/RawAssets/Characters/Footman.lod.json` | SK-LOD recipe sidecar (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | NEW; schema `siege_sk_lod_recipe_v1` |

**PRESERVED (NOT touched — the remaster law):** `Content/RawAssets/Characters/Anims/Footman_{Idle,Walk,Attack,Death}.fbx` (all still Jul-15, byte-identical) via `rig_character.py --no-anim-fbx`. The in-engine anim uassets `/Game/Characters/Anims/A_Footman_*` and the shared ABP are PRESERVED — build-master must NOT reimport them.

Disposable/gitignored: `Tools/ArtPipeline/Cache/Footman/**` (meshy_raw.glb 23.4MB, refine_report.json, rig/rig_report.json, previews). The two manifests (`pipeline_manifest.json`, `rig_manifest.json`) show git-modified from the WIZARD task (12:05/12:12), NOT this run — my scripts write no manifest.

## How generated (reproducible)
- **Stage 1 (Meshy):** `uv run meshy_generate.py --mode image3d Footman` from `Tools/ArtPipeline/` (input `Inbox/Footman.png`, SHA1 `2580f634…` = byte-identical to the approved `Content/RawAssets/Concepts/Footman.png`). Meshy task `019fa06d-ca57-7b0c-8f7c-127715e1ff1a`, engine `meshy-i23d`, SUCCEEDED → `Cache/Footman/meshy_raw.glb`.
- **Stage 2 (HEADLESS Blender 5.1):** `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Footman --input Cache/Footman/meshy_raw.glb` → exit 0, 10.3 s.
- **Stage B rig (HEADLESS Blender 5.1):** `blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Footman --no-anim-fbx` → exit 0, 43.7 s. Input = the new static `Content/RawAssets/Footman.fbx`.

## Team-region choice (RECORDED)
`helm_dome` + `shoulder_caps` (2 selectors, from the manifest's TASK-086-tuned Footman recipe = the helmeted-soldier recipe, shared with Knight/Pikeman). **NOTE:** the task spec suggested the "Cleric/Wizard robed-caster" recipe — that is the WRONG match for the Footman (he wears an open-face steel HELM, not a hood). The manifest's helmeted recipe is authoritative and correct; verified against the live mesh at the eyeball gate. Result: **900 faces / 7.0% of area** (cap 35%) — the helm crown + shoulder pauldrons tint team-colour; face, gold griffin heraldry, shield, and body all SPARED. Strong classic RTS team read (heavier than the Wizard's 4.1%). Placeholder blue confirmed isolated in the beauty + bind previews.

## Pre-import eyeball gate — PASS (read both reports + viewed all previews vs `Concepts/Footman.png`)
- **Silhouette / faithfulness:** open-face steel helm w/ nasal, chainmail coif, quilted gambeson, gold griffin chest crest, steel pauldrons, brown leather belt, kite shield with gold griffin (character's LEFT / +X), spear (right hand / -X), greaves + boots — reads instantly as the concept's armoured spearman from every angle. Props solidly modelled + (rigged) attached; no holes/islands, clean bind, 0.0% unweighted skin, attack-thrust contact strip deforms cleanly.
- **Geometry:** 15,000 tris (at budget); feet-centre min_z 0.008; `UVMap` present/ok; Z height-fit to 180 (179.62).
- **Colour / material fidelity — VIVID (the acceptance bar):** rich, saturated, material-distinct — GOLD griffin heraldry (chest + shield), true metallic steel (helm/pauldrons/greaves), brown leather, chainmail, deep gambeson. This is emphatically NOT the washed-out/colourless old look — a clear, large step up in material richness. The de-lit D atlas shows saturated colour, not flat grey.

### Colour VERDICT vs concept (the PILOT go/no-go)
**PASS — proceed to the other 10.** The Meshy result carries genuine, saturated PBR colour and material separation that the washed-out old mesh lacks. Two honest caveats (neither blocks):
1. **Hue drift:** the gambeson bakes dark FOREST-GREEN, whereas the concept reads dark slate BLUE-grey. It is vivid and armour-plausible, just a hue shift. If exact concept-blue matters, a re-gen isn't needed — a targeted D-hue nudge could be a later polish; recommend accepting for the pilot.
2. **Overall VALUE is on the moody/dark side** (delight used the in-script conservative defaults — mean linear 0.022→0.050; Footman is NOT in the recorded-dark override set). The flat-lit workbench previews are well-exposed; the Cycles beauty preview is dim (dark gambeson + moody light). **WATCH at `-verify`:** if the unit reads too dark under actual `L_Arena` lighting, the lever is a re-gen with the recorded-dark `albedo_delight` override `{ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` (the Ogre/Knight-proven values) added to the Footman manifest entry — NOT required to proceed, but the known fallback.

### Warn-only (informational, NON-blocking)
`refine_report` warns conformed X/Y `[80.3, 72.8]` deviate >10% from the OLD blockout target `[147, 80]`. Expected + BENIGN: the concept holds the spear near-vertical and the shield close to the body, so the Meshy footprint is NARROWER than the TASK-014 blockout guess. `fit_mode=height` enforces Z only (matched to 180). The narrower footprint is actually BETTER for capsule sizing (no weapon-overhang like Cavalry/Ogre). Manifest `target_dims_ue` left as-is (blockout provenance); could be `_tuned` to the measured dims on a future re-gen.

## Preview render paths (to show Jonathan)
- Beauty (Cycles, team-recolour visible): `Tools/ArtPipeline/Cache/Footman/previews/preview_beauty_cycles.png`
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Footman/previews/preview_front.png` · `preview_back.png` · `preview_threequarter.png` · `preview_top.png`
- Rig bind + anim: `Tools/ArtPipeline/Cache/Footman/rig/previews/bind_threequarter.png` · `turntable_strip.png` · `contact_attack.png` (+ idle/walk/death contact strips)
- Concept reference: `Content/RawAssets/Concepts/Footman.png`

---

## TURNKEY same-path IMPORT recipe (for the deferred editor-gated build-master dispatch)
Serialized, EXCLUSIVE editor session on main. **NEVER delete+recreate — same-path OVERWRITE preserves all refs** (`BP_Unit_Footman`, `DT_Cards`, ghost soft-ref, `ASummonedUnit::ResolveSkeletalVisual`, ABP/anim bindings). Law: CONVENTIONS "Fleet Meshy remaster — same-path overwrite" + "Textured mesh law" (Stage-3) + M7.6 SK-unit LOD/URO law. Gated on TASK-308 (systemic float-fix) being compiled + live (the grounding gate) before `-verify`.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Footman_D` (**sRGB ON**) ← `Content/RawAssets/Textures/Footman/T_Footman_D.png`
- `T_Footman_N` (**LINEAR**) ← `…/T_Footman_N.png`
- `T_Footman_ORM` (**LINEAR, sRGB OFF**) ← `…/T_Footman_ORM.png`

### 2. Material instance `MI_Footman_PBR` (already exists — reassign params, do not recreate)
- Master `/Game/Materials/M_AssetPBR`; params `BaseColor`=T_Footman_D, `Normal`=T_Footman_N, `ORM`=T_Footman_ORM.

### 3. Static mesh `SM_Footman` (STATIC path — ghost + fallback)
- Import `Content/RawAssets/Footman.fbx` OVERWRITING `/Game/Meshes/SM_Footman` at the SAME path. **Nanite OFF.** Import Normals (not compute). Collision = ≤4 simple hulls at import (TASK-037 unit recipe).
- Material slots EXACTLY, in order: slot 0 `TeamRegion` → `MI_TeamColor_<Team>` (import default `MI_TeamColor_Blue`); slot 1 `FootmanPBR` → `MI_Footman_PBR`. FBX slot names are already `[TeamRegion, FootmanPBR]` → map 1:1.

### 4. Skeletal mesh `SK_Footman` (the runtime visual)
- Import `Content/RawAssets/Characters/Footman.fbx` OVERWRITING `/Game/Characters/SK_Footman` at the SAME path. **BIND to the EXISTING `/Game/Characters/SK_Footman_Skeleton`** (root bone `Footman_Rig`) — do NOT create a new skeleton (Footman is the shared-skeleton SOURCE; the FBX carries the exact 21-bone contract → clean bind, no missing-bones warning). **Nanite OFF.** Import Normals. Same two-slot `[TeamRegion, FootmanPBR]` as SM.
- ⚠ Because Footman is the shared-rig source, confirm the in-place SK reimport binds to the existing `SK_Footman_Skeleton` (does not spawn `SK_Footman_Skeleton1`) — this is what keeps every other unit's + all anims' bindings intact.

### 5. Anims + ABP — PRESERVED, do NOT touch
- Do NOT reimport `/Game/Characters/Anims/A_Footman_{Idle,Walk,Attack,Death}` and do NOT touch the shared `ABP_Footman`. They bind to the preserved `SK_Footman_Skeleton` and must keep working after the SK overwrite. (The rig re-authors anims INTERNALLY only for previews; `--no-anim-fbx` left the raw anim FBXs untouched, so there is nothing new to import.)

### 6. SK-LOD chain — REGENERATE post-import (a same-path SK reimport drops to LOD0-only)
- Apply `Content/RawAssets/Characters/Footman.lod.json`: LOD1 50% @ 0.4 / LOD2 20% @ 0.15 via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction (mirror `reimport_meshes.py::_apply_lods`). Readback `lod_count == 3`. URO (`OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations`) is already set in C++ on `SkeletalVisualMesh` for every unit — only the LOD chain needs regenerating.

### 7. Acceptance (feeds TASK-311-verify)
- `SK_Footman` is the runtime visual, VIVID/colourful matching `Concepts/Footman.png` (wash-out gone), slot-0 team recolour tints the helm+shoulders Blue/Red; **feet meet the ground** (TASK-308 must be live); preserved anims still play; LOD count 3; Message Log clean. Capture the "after" PIE screenshot (feeds the TASK-310 gallery). Commit this ONE unit on main with explicit pathspecs, NO push.

## Downstream
- Unblocks **TASK-311-verify** (build-master, gated ALSO on TASK-308). This pilot's colour PASS is the go-ahead to fan out **TASK-312..321** (the other 10 units), same template.
- **No editor / MCP / Blueprint / Git / deck-builder / Wizard asset touched.** Staged and ready for the editor-gated import dispatch.

---

## RE-GEN ADDENDUM — brighter albedo (2026-07-26 15:59) — the RE-PILOT

**Trigger:** the in-engine verify confirmed the first Footman gen reads **too dark/muddy under L_Arena's warm low-angle sun**. Colour/heraldry vivid + wash-out gone — this was purely a VALUE/brightness problem, exactly the WATCH the first handoff flagged. Applied the known fallback lever.

**Fix (no new Meshy credits, mesh/rig untouched):** pinned the Ogre/Knight-proven **recorded-dark** `albedo_delight` override `{ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` into the Footman manifest entry (`Tools/ArtPipeline/pipeline_manifest.json`, `assets.Footman.albedo_delight`, with a TASK-311 re-pilot `_note`), then re-ran Stage-2 refine HEADLESS reusing the **cached** `Cache/Footman/meshy_raw.glb` (Meshy task `019fa06d`, no network, no credits). Blender 5.1 headless, exit 0, ~10 s.

- Command: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Footman --input Cache/Footman/meshy_raw.glb`

**Brightness result — the number:** mean linear albedo **0.0498 → 0.1639** (vs the raw pre-delight 0.0224 source, ~7.3x; vs the first conservative bake, 3.3x). `p99_linear_after 0.6285` (highlights keep headroom — gold/steel not blown), `shoulder_compressed_fraction 0.0017` (**only 0.17% of pixels touch the soft-shoulder clamp — no wash-out**). This is the same tuned profile that landed Ogre at 0.059 / Knight comparably; Footman lands brighter (0.164) because its raw source started brighter than theirs.

**Re-staged (SAME raw paths, clean overwrite):**
- `Content/RawAssets/Textures/Footman/T_Footman_D.png` (brighter de-lit albedo; 1024²; sRGB on import)
- `Content/RawAssets/Textures/Footman/T_Footman_N.png` (re-emitted, byte-identical to prior; LINEAR)
- `Content/RawAssets/Textures/Footman/T_Footman_ORM.png` (re-emitted, byte-identical to prior; LINEAR, sRGB OFF)
- **Mesh/rig FBXs NOT touched** (`Content/RawAssets/Footman.fbx`, `Content/RawAssets/Characters/Footman.fbx`, `Footman.lod.json`) — the delight pass only writes D/N/ORM, so the entire §3–§7 import recipe above is UNCHANGED. Only step §1 (textures) + §2 (MI param reassign) reflect the new D.
- Report: `Cache/Footman/refine_report.json` (`albedo_delight` block above). Team-region unchanged (900 faces / 7.0%), tris 15000, min_z 0.008, slots `[TeamRegion, FootmanPBR]`.

**Preview renders (before = dark first bake, after = brighter):**
- Flat-lit true-albedo (best colour read): AFTER `Cache/Footman/previews/preview_front.png` · `preview_threequarter.png` · `preview_back.png` · `preview_top.png` ; BEFORE `Cache/Footman/previews_dark_before/preview_front.png` · `preview_threequarter.png`
- Sun-lit/darker-context (Cycles beauty, low-angle sun — the L_Arena proxy): AFTER `Cache/Footman/previews/preview_beauty_cycles.png` ; BEFORE `Cache/Footman/previews_dark_before/preview_beauty_cycles.png`
- Dark "before" report kept at `Cache/Footman/refine_report_dark_before.json`.

**Candid read (the go/no-go for the fan-out):** PASS — re-verify in-engine. Flat-lit, the body value is clearly, substantially lifted: gambeson opens up, chainmail reads as metal, gold griffin + steel stay vivid, nothing blows out. The Cycles beauty still looks moody, but that is the preview's OWN rig (single sun energy 3.0 + world background 0.6, no skylight/ambient fill) — it under-represents the improvement; L_Arena's warm ambient will read brighter than this. Given the spec's steer (under-cooking = 10 more dark units), 0.164 is a confident, non-under-cooked value with highlight headroom intact. **If build-master's re-verify confirms it holds under L_Arena, bake this exact `{1.0, 0.25, 0.55, 1.2}` profile into the TASK-312..321 fan-out as the default.**

**Gambeson hue nit (unchanged):** still bakes forest-green vs the concept's slate blue-grey. The delight pass is a value/AO operation, not a hue rotation, so it cannot nudge hue "for free" — deliberately LEFT as a gallery-review nit per spec (a targeted HSV shift on D would be a separate later polish, not worth risking the pilot).

**Discipline:** NO editor / MCP / Blueprint / Git touched. Only files written: the manifest override, the three re-staged PNGs, the refine_report + previews under the gitignored `Cache/`, and this handoff + the board note. The turnkey import recipe (§1–§7 above) stays valid — build-master's only delta is the brighter `T_Footman_D`.
