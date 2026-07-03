# TASK-013 Handoff — Castle blockout mesh (art-director)

Status: **Blender phase complete — FBX exported and verified. Editor import NOT done** (editor was reserved by TASK-015 this run). A follow-up editor task must perform the import below.

## Deliverable

| Item | Value |
|------|-------|
| FBX (import source) | `Content/RawAssets/Castle.fbx` (repo-relative; absolute: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Castle.fbx`) |
| Render mesh node | `SM_Castle` (single mesh, identity transform) |
| Tri count | **2,414** triangles (spec limit 15k) |
| Dimensions | **814.5 x 820.0 x 900.0 UE units** (~8.1 x 8.2 x 9.0 m) — spec ~800x800x900 |
| Origin | Ground-center: pivot at (0,0,0), mesh min Z = 0 exactly |
| Material slots | Exactly **ONE**, slot/material name in FBX: `MI_TeamColor_Blue` |
| UVs | Channel 0 present (smart-projected) — no missing-UV import warnings; team material is bounds-based and does not depend on them |
| Collision | 9 convex UCX nodes in the FBX: `UCX_SM_Castle_00` … `UCX_SM_Castle_08` (plinth box, solid wall-ring box, 4 tower cylinders, keep box, 2 gate-turret cylinders; 204 tris total) |

Verified by headless re-import of the exported FBX: node names, one material slot, dimensions, and identity transforms all confirmed. Silhouette (keep + pyramid roof + spire, 4 cone-roofed corner towers, gatehouse turrets, crenellated walls, chamfered edges) reads clearly at 15 m in a test render.

Source: mesh is 100% procedural — build script and `SM_Castle.blend` live in the session scratchpad only (per task constraints, no .blend in the repo). The FBX is the durable source for import; the geometry can be regenerated from the script if ever needed.

## Import instructions (follow-up editor task)

1. Import `Content/RawAssets/Castle.fbx` to **`/Game/Meshes/SM_Castle`** (Content/Meshes/). The FBX render node is already named `SM_Castle` so the asset name matches automatically.
2. **Import scale/units:** units are baked to centimeters in the FBX (Blender "All Local" export). Import with uniform scale **1.0**, no "Convert Scene" tweaks. Expected result: bounds ~814 x 820 x 900 units, pivot at ground-center — exactly 5x the height of the 180-unit mannequin.
3. **Collision:** turn OFF "Auto Generate Collision" and let the importer consume the `UCX_` nodes (default "One Convex Hull Per UCX"). Expected: 9 convex elements that block hero + units. Fallback if UCX nodes are ignored by the importer in use: add simple box collision — spec only requires that the castle blocks movement.
4. **Material:** single slot — assign **`/Game/Materials/Instances/MI_TeamColor_Blue`** as the default (exists since TASK-012; the FBX material name matches character-for-character, so material search/match may bind it automatically — verify slot 0 afterwards). Do NOT let the importer create a new material asset; delete any auto-created `/Game/Meshes/MI_TeamColor_Blue` duplicate and point the slot at the TASK-012 instance. Per TASK-002, `ACastle` swaps Blue/Red instances at runtime — the mesh default just needs to be the Blue instance.
5. Nanite: not needed at 2.4k tris; leave off (blockout tier, replaced in M7 at the same path).
6. Do not place the mesh in any level — integration (build-master) places the two `ACastle` actors at the anchors.

## Orientation & placement notes

- The **gate faces +X** (in UE, after standard FBX axis conversion). For `Castle_Blue` at `CastleAnchor_Blue` (-2000, 0, 0) the default yaw 0 already faces the gate toward the Red side (+X). For `Castle_Red`, yaw 180 makes gates face each other — cosmetic choice; the mesh is otherwise symmetric.
- Gate opening is decorative only: the wall-ring UCX is a solid box, so nothing can path into the courtyard.
- Vertical team gradient (TASK-012 `M_TeamColor`) auto-fits the mesh bounds: darkest at the plinth, brightest at the spire — no tuning needed.

## Caution for whoever runs the editor next

The FBX sits inside `Content/`, so the editor's auto-import monitor may pop an import prompt for `Content/RawAssets/`. **Dismiss it** (or if already auto-imported to `/Game/RawAssets/Castle`, delete that asset) — the deliberate import target is `/Game/Meshes/SM_Castle` per step 1. Same applies to TASK-014's `Footman.fbx` in the same folder.

## Import complete (editor phase, 2026-07-02)

Imported and verified via Unreal MCP; asset saved (`Content/Meshes/SM_Castle.uasset`).

| Check | Result |
|-------|--------|
| Asset path | `/Game/Meshes/SM_Castle` (StaticMesh, explicit import — no auto-import artifact existed in `/Game/RawAssets/`, which is empty) |
| Bounds | 814.48 x 820.0 x 900.0 UE units; min Z = 0 (pivot at ground-center) — matches spec ~800x800x900 |
| Triangles (LOD0) | 2,414 (matches export exactly) |
| Material slots | Exactly ONE, slot name `MI_TeamColor_Blue`, assigned to `/Game/Materials/Instances/MI_TeamColor_Blue` (the TASK-012 instance — importer created NO material assets; `/Game/Meshes/` contains only the two SM_ meshes) |
| Collision | 9 convex elements consumed from the UCX_ nodes, all `bIsGenerated: false` (plinth box, wall-ring box, 4 tower cylinders, keep box, 2 gate-turret cylinders); CollisionTraceFlag = CTF_UseDefault (simple collision blocks) |
| Nanite | Off (per step 5) |

Not placed in any level (integration places the `ACastle` actors). Source FBX unchanged at `Content/RawAssets/Castle.fbx`.
