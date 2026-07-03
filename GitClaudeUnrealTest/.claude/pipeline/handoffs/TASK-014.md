# TASK-014 Handoff — Footman blockout mesh (art-director)

Status: **Blender phase complete — FBX exported and verified.** Editor import NOT done (editor was reserved by TASK-015 this run). This note is the import brief for the follow-up editor step.

## Source file

| What | Where |
|------|-------|
| FBX (import source) | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Footman.fbx` |
| Authored with | Blender 5.1.2 headless (FBX 7400, default exporter axes, meters) |

## Mesh stats (verified by re-importing the FBX into a clean Blender scene)

- Single mesh object named `SM_Footman` (nothing else in the file — no rig, no anim, no lights/cameras)
- **Triangles: 2,152** (budget was <= 8k) · 1,122 verts
- **Height: exactly 1.80 m -> 180 UE units** at default import (min Z 0.0, max Z 1.80)
- Bounding box: 1.47 m (X) x 0.80 m (Y) x 1.80 m (Z) — width includes shield (-X side) and sword (+X side)
- **Origin at feet-center**: object origin (0,0,0) sits between the boot soles at ground level
- **ONE material slot, named exactly `MI_TeamColor_Blue`** (importer-matching name per TASK-012 contract)
- UVs: channel 0 `UVMap` (smart-projected, non-overlapping) — fine for the blockout tier
- Facing: modeled facing Blender **-Y**. With default Blender FBX export + default UE import this is expected to land facing **+Y in UE asset space** (same convention as the UE mannequin). Verify in the static mesh viewer after import.

## Import instructions (editor step)

1. Import `Content\RawAssets\Footman.fbx` to **`/Game/Meshes/SM_Footman`** (Content/Meshes/).
   - Import as **Static Mesh** (no skeleton in the file; leave Skeletal Mesh off).
   - Import Uniform Scale 1.0, Convert Scene on (defaults) — mesh is authored in meters, lands at ~180 units. Verify height ~180 next to the mannequin.
   - Generate Lightmap UVs: on (default) — source UVs are channel 0.
2. Material: the FBX's single slot is named `MI_TeamColor_Blue`. Assign **`/Game/Materials/Instances/MI_TeamColor_Blue`** (exists, TASK-012) to slot 0 — do NOT let the importer create a new material/texture (untick Import Materials/Textures, or delete any auto-created material and remap). One slot only.
   - TASK-012 note: the team gradient auto-fits the mesh bounds; feet-origin gives darkest-at-feet, brightest-at-helm as intended.
3. Collision: **simple capsule (or box) collision** — add via the static mesh editor (Add Capsule Simplified Collision) roughly full-height. Note TASK-010 sizes the unit's gameplay capsule itself (~90 half-height); the mesh just needs any simple collision so the placement ghost/level use is sane. No complex-as-simple.
4. Sanity checks after import: height ~180 units, exactly 1 material slot with MI_TeamColor_Blue, origin at feet-center (mesh sits ON the floor when placed at Z=0), reads blue at 15 m.

## Notes for integration / downstream

- `/Game/Meshes/SM_Footman` is referenced by TASK-007 (placement ghost) and TASK-010 (`BP_Unit_Footman` VisualMesh) — path and name are contractual, character-exact.
- If the asset faces +Y as expected, TASK-010 should give VisualMesh a relative yaw of **-90°** so the footman faces the actor's forward +X (identical to the mannequin convention). The sword arm is the character's right.
- The FBX lives inside Content/ (`RawAssets`), so if the editor's auto-reimport monitor prompts to import it, dismiss the prompt and import explicitly to /Game/Meshes/ instead.
- Silhouette renders used for sign-off (scratchpad, session-temporary): front + 3/4 workbench renders confirmed sword-and-shield read at distance.
- M7 will replace this blockout with a skeletal mesh at the same visual-slot contract; nothing here should be treated as final art.

## Import complete (editor phase, 2026-07-02)

Imported and verified via Unreal MCP; asset saved (`Content/Meshes/SM_Footman.uasset`).

| Check | Result |
|-------|--------|
| Asset path | `/Game/Meshes/SM_Footman` (StaticMesh, explicit import — no auto-import artifact; `/Game/RawAssets/` is empty) |
| Bounds | 147.1 (X) x 80.0 (Y) x 180.0 (Z) UE units; min Z = 0 — height exactly 180, origin at feet-center. X spans -86 (shield side) to +61 (sword side), matching the authored 1.47 x 0.80 x 1.80 m box |
| Triangles (LOD0) | 2,152 (matches export exactly) |
| Material slots | Exactly ONE, slot name `MI_TeamColor_Blue`, assigned to `/Game/Materials/Instances/MI_TeamColor_Blue` (TASK-012 instance — importer created NO material/texture assets) |
| Collision | Importer's auto-generated convex hull was removed and replaced with a **simple capsule** per step 3: center (0, 0, 90), radius 40, length 100 → spans Z 0–180 (full height). No complex-as-simple (CTF_UseDefault) |
| Nanite | Off |

Integration reminders (unchanged from above): TASK-010 gives VisualMesh relative yaw -90 deg if the asset faces +Y; the unit's gameplay capsule (~90 half-height) is sized by TASK-010 itself — the mesh capsule here is only for placement-ghost/level sanity. Source FBX unchanged at `Content/RawAssets/Footman.fbx`.
