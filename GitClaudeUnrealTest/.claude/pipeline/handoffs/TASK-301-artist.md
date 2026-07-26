# TASK-301 — `SM_Wizard` game-ready textured static mesh (W-UNIT-1) — art-director handoff

**Status:** GENERATION + STAGE-2 DONE, mesh staged on disk. **UE import NOT done — deliberately deferred (editor-gated).** No editor / MCP / Blueprint / Git touched.
**Date:** 2026-07-26

## Scope executed vs deferred
- DONE here (no-editor): Meshy image-to-3D (Stage 1) + headless Stage-2 refine → `SM_Wizard` FBX + D/N/ORM textures staged in `Content/RawAssets/`, passed the pre-import eyeball gate.
- DEFERRED (coordinator to dispatch separately): the Stage-3 **Unreal import** into `/Game/`. The editor is occupied by the in-flight deck-builder fix (Track A); the import serializes. All import parameters are specified in "Stage-3 import recipe" below so build-master/the import dispatch can run it turnkey.

## `MESHY_TOKEN` gate
PRESENT (resolved from process env, value redacted throughout). `--check` PASSED: API reachable, 2866 credits before run. This run consumed **30** credits (balance 2866 → 2836). No quota block.

## Assets produced (staged, on disk)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Wizard.fbx` | Raw game-ready static mesh (object `SM_Wizard`) | 610,908 bytes; 15,000 tris; slots `[TeamRegion, WizardPBR]`; `UVMap`; feet-center (min_z 0.065 UE) |
| `Content/RawAssets/Textures/Wizard/T_Wizard_D.png` | Base color (sRGB on import) | 1024², de-lit (albedo delight applied) |
| `Content/RawAssets/Textures/Wizard/T_Wizard_N.png` | Normal (LINEAR on import) | 1024² |
| `Content/RawAssets/Textures/Wizard/T_Wizard_ORM.png` | Occlusion/Roughness/Metallic packed (LINEAR, sRGB OFF) | 1024² |
| `Tools/ArtPipeline/Cache/Wizard/meshy_raw.glb` | Meshy dense donor (Stage-1 output) | 23.8 MB, 227,469 verts; gitignored/disposable Cache |
| `Tools/ArtPipeline/Cache/Wizard/previews/*.png` | Eyeball-gate previews (front/back/threequarter/top/beauty_cycles) | preview thumbnails |
| `Tools/ArtPipeline/Cache/Wizard/refine_report.json` | Stage-2 machine report | tris/bounds/UV/slots/team-region |

Also updated (committed pipeline file, NOT editor/Git): `Tools/ArtPipeline/pipeline_manifest.json` — added the `Wizard` asset entry (modeled on the Cleric robed-caster sibling; target_dims now mesh-measured `_tuned`, team_region `_verified` at the gate).

## How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Wizard` (from `Tools/ArtPipeline/`) — input `Inbox/Wizard.png`. Meshy task `019f9fcc-44c6-753e-843e-85617c077604`, engine `meshy-i23d`, SUCCEEDED (progress 100). → `Cache/Wizard/meshy_raw.glb`.
- Stage 2 (HEADLESS): `"…/Blender 5.1/blender.exe" --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Wizard --input Cache/Wizard/meshy_raw.glb` → exit 0, 11.0 s.
- **Norton TLS:** reused the concept step's combined CA bundle `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE` (Meshy's `api.meshy.ai` is NOT in Norton's HF exclusions). `MESHY_TOKEN`/`HF_TOKEN` env-only throughout — never echoed, logged, or on argv.

## Pre-import eyeball gate — PASS
Read `refine_report.json` AND viewed all preview renders against `Content/RawAssets/Concepts/Wizard.png`.
- **Silhouette:** robed, hooded, bearded caster — reads instantly as a wizard from every angle. Faithful to the concept.
- **Staff:** gnarled wooden staff with a bronze crown holding a red ember-crystal orb — captured cleanly (right hand).
- **Fireball-hand:** glowing yellow-orange fireball cradled in the open left palm — captured.
- **TeamRegion:** the cream shoulder-cape / hood-mantle is isolated as slot-0 (555 faces / **4.1%** area, under the 35% cap). Front AND back previews confirm the cape is the sole recolor region; hood + bare bearded face SPARED (Archer bare-HEAD lesson holds). The Cleric `shoulder_caps` recipe transferred perfectly.
- **Color fidelity (acceptance bar):** matches `Concepts/Wizard.png` — muted charcoal-grey robe, cream mantle, red singed flame-cut hems (beautifully caught, esp. back view), warm fireball, red staff crystal, brown leather crossbelt + pouch + boots. Delight pass opened shadows (mean linear 0.060 → 0.097).
- **Geometry:** 15,000 tris (at budget), `UVMap` present/ok, feet-center origin (min_z 0.065), bounds `[129.88, 99.25, 181.94]` UE (Z height-fit to 182).

### Flagged artifacts (minor, NON-blocking)
1. The **held fireball's flame** has thin spiky protrusions — Meshy's 3D interpretation of the 2D flame. Reads as fire but is jagged. Anticipated in the TASK-300 concept handoff as the "harmless emissive prop" class. The in-flight fireball is the **Fire_Magic Niagara** VFX on `BP_Projectile_Fireball` (TASK-304), so this held ball is a cosmetic caster prop, not the gameplay projectile.
2. Slight roughness where the fireball meets the fingers.
Neither blocks. This is the placement-ghost / null-safe fallback static mesh; if a later polish pass wants a cleaner held flame, that is a rig-time or targeted-regen tweak, not required now.

### Warn-only (informational)
`refine_report` warns conformed X/Y `[130.3, 99.4]` deviate >10% from the original target guess `[120, 82]`. Expected: fit_mode=height enforces **Z only** (matched to 182); the caster's outstretched fireball arm + held staff make the footprint deeper than the Cleric. Manifest target_dims updated to the measured values (`_tuned`) so a re-gen conforms clean. **WATCH at integration:** the capsule/collision footprint is wider than a stock humanoid (the staff + arm) — same class as the Cavalry/Ogre weapon-overhang note.

## Stage-3 import recipe (for the deferred editor-gated dispatch)
Per CONVENTIONS "Wizard unit" static-mesh clause + "Textured mesh law" + the TRELLIS.2 Stage-3 import law. Serialized, exclusive editor session:
- **Textures → `/Game/Textures/`:** `T_Wizard_D` (sRGB ON), `T_Wizard_N` (LINEAR), `T_Wizard_ORM` (LINEAR, sRGB OFF). Sources = the three PNGs above.
- **Material instance:** `MI_Wizard_PBR` at `/Game/Materials/…` from master `/Game/Materials/M_AssetPBR` — params `BaseColor`=T_Wizard_D, `Normal`=T_Wizard_N, `ORM`=T_Wizard_ORM.
- **Mesh:** import `Content/RawAssets/Wizard.fbx` OVERWRITING `/Game/Meshes/SM_Wizard` at the same path (NEVER delete+recreate — preserves refs). **Nanite OFF.** Collision per the unit law (≤4 simple hulls generated at import, TASK-037 recipe).
- **Material slots — assign EXACTLY, in order:** slot 0 `TeamRegion` → `MI_TeamColor_<Team>` (BeginPlay recolor drives the cream mantle); slot 1 `WizardPBR` → `MI_Wizard_PBR`. FBX slot names are already `[TeamRegion, WizardPBR]` (verified), so they map 1:1.
- Acceptance: baked result reads as the `Concepts/Wizard.png` palette; team-region tints cleanly Blue/Red.

## Downstream
- Unblocks **TASK-302** (rig `SK_Wizard` from this game-ready mesh — the rigged FBX lands at `Content/RawAssets/Characters/Wizard.fbx` per the roster's static-vs-rigged two-tier convention), **TASK-303** (card art, independent), **TASK-304** (integration/assemble).
- **Path reconciliation note:** the naming block/TASK-301 spec mention `Content/RawAssets/Characters/Wizard.fbx` for the raw FBX; the LIVE pipeline (matching every other roster unit — `Content/RawAssets/Cleric.fbx` etc.) writes the STATIC-mesh FBX to `Content/RawAssets/Wizard.fbx`, and the `Characters/` subfolder holds the RIGGED variant (TASK-302 output, e.g. `Content/RawAssets/Characters/Cleric.fbx`). I followed the live two-tier convention. `SM_Wizard` still imports to `/Game/Meshes/SM_Wizard` exactly as specced.
