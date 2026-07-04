# TASK-038 Handoff — Structure blockout meshes + M_GoldGlow (art-director)

Status: **COMPLETE — all 3 meshes + M_GoldGlow modeled, exported, imported, verified, saved.** Blender MCP + UE5 MCP both live this session. Editor left UP for the tasks after me (TASK-033..036, TASK-040). No board edit (orchestrator flips status). No git mutation (commit is TASK-040) — see "Git state" below.

Family style: SM_Castle/SM_Footman lineage (GDD §6) — blockout tier, beveled (1-seg angle-limited chamfer), team material carries color, no textures. All ground-center origin (minZ = 0, XY bbox centered) so each sits ON the floor at Z=0.

## Deliverables (all verified in-editor, UE LOD0)

| Mesh | Asset path | FBX source | Bounds X×Y×Z (UE) | Tris | Origin | Nanite |
|------|-----------|-----------|-------------------|------|--------|--------|
| SM_GoldNode   | `/Game/Meshes/SM_GoldNode`   | `Content/RawAssets/GoldNode.fbx`   | 190.2 × 200.0 × 249.4 | 222 | ground-center (minZ 0, XY 0,0) | Off |
| SM_Wall       | `/Game/Meshes/SM_Wall`       | `Content/RawAssets/Wall.fbx`       | 400.0 × 100.0 × 250.0 (EXACT) | 264 | ground-center | Off |
| SM_ArrowTower | `/Game/Meshes/SM_ArrowTower` | `Content/RawAssets/ArrowTower.fbx` | 250.0 × 250.0 × 497.4 | 512 | ground-center | Off |

Absolute FBX paths:
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\{GoldNode,Wall,ArrowTower}.fbx`

All tris far under the 15k budget. Wall is dimensionally EXACT per the M2 ruling (400 long × 100 thick × 250 tall; placement clearance 200 is a code rule, not geometry). ArrowTower height 497.4 vs target ~500 — the 0.02 m chamfer shaved ~2.6 UE off the roof apex point; within ±10% tolerance.

## Collision (verified via BodySetup.AggGeom)

| Mesh | Collision | Detail |
|------|-----------|--------|
| SM_GoldNode   | 4 convex hulls (generated, `generate_convex_collisions` 4/12) | wraps the crystal cluster, tallest hull ~z242; QueryAndPhysics; CTF_UseDefault |
| SM_Wall       | 1 UCX box (authored `UCX_SM_Wall_00`, `bIsGenerated:false`) | X±200 Y±50 Z 0..250 = full visual box → blocks pawns wall-to-wall; QueryAndPhysics; CTF_UseDefault |
| SM_ArrowTower | 1 UCX box (authored `UCX_SM_ArrowTower_00`, `bIsGenerated:false`) | X±125 Y±125 Z 0..500, footprint 250×250 = EXACTLY the base plinth, NOT wider than base (M1 castle-plinth dead-zone lesson honored); QueryAndPhysics; CTF_UseDefault |

- Wall/Tower UCX boxes were authored in the FBX and consumed by the importer (`bIsGenerated:false` confirms they are my authored hulls, not auto-generated). UE stores UCX as convex elements (not box primitives) — functionally an exact box blocker.
- **`bCanEverAffectNavigation` (walls carve the dynamic navmesh, §3.7):** this is a `UStaticMeshComponent` property set at INTEGRATION on the mesh component of `BP_Building_Wall` (default is true). The asset carries solid simple collision (QueryAndPhysics, CTF_UseDefault), which is the prerequisite — with component nav left enabled the wall WILL carve the navmesh. **Build-master: confirm `bCanEverAffectNavigation=true` on the Wall BP's VisualMesh component during assembly.**

## Materials

| Mesh | Slot 0 name | Assigned material |
|------|-------------|-------------------|
| SM_GoldNode   | `M_GoldGlow`        | `/Game/Materials/M_GoldGlow` (new, this task) |
| SM_Wall       | `MI_TeamColor_Blue`| `/Game/Materials/Instances/MI_TeamColor_Blue` (TASK-012) |
| SM_ArrowTower | `MI_TeamColor_Blue`| `/Game/Materials/Instances/MI_TeamColor_Blue` (TASK-012) |

- Exactly ONE material slot each. Imported with `import_materials=False`/`import_textures=False` → the importer created NO material/texture assets (`/Game/RawAssets/` content path is empty; no auto-import artifacts). Slots then bound explicitly and re-verified via `get_material`.
- Wall/Tower are team-tintable exactly like SM_Castle: `ABuilding`/BP can swap Blue↔Red MI at runtime; the mesh default is the Blue instance.

## M_GoldGlow (new material, `/Game/Materials/M_GoldGlow`)

- Opaque Lit `Material`. Graph:
  - `Constant3Vector` = warm yellow linear **(R 1.0, G 0.66, B 0.12)** → **BaseColor**, and → `Multiply.A`.
  - `Constant` = **6.0** → `Multiply.B`.
  - `Multiply` (≈ 6.0, 3.96, 0.72, HDR >1) → **EmissiveColor**.
- Emissive/BaseColor wiring re-verified via `get_property_input`; recompiled with no shader errors. Net effect = §6 "gold glows warm yellow" — emits above 1.0 so it blooms in the viewport. Saved.
- Only ONE `M_GoldGlow` exists (checked `find_assets` — no dupes).

## Warnings — resolved (zero on final import)

- FIRST import of the boxy Wall/Tower produced 2 warnings each: *"degenerate tangent bases … MikkTSpace"* and *"nearly zero bi-normals."* Root cause: no UV channel authored, so MikkTSpace could not compute tangents on the axis-aligned faces (GoldNode's angled crystal facets escaped it).
- Fix: added a smart-projected UV channel (`UVMap`, angle 66°, margin 0.02) to all three meshes in Blender, re-exported, DELETED the three meshes (no referencers — checked), and re-imported fresh from the UV-bearing FBX.
- Re-import log block (timestamps 08.56.xx) shows clean `Built static mesh` for all three with **NO tangent/bi-normal warnings**. Final state = ZERO import warnings. All re-verified (dims/tris/collision/material identical) and re-saved.
- Team material is bounds/world-position based and does not sample the UVs; UVs are present purely for clean tangents + future lightmap support.

## Git state (NOT committed — read this, build-master)

- I ran **no mutating git** (no add/reset/commit/push). I ran only read-only `git status`/`git diff --cached`/`git ls-files` to verify state.
- **3 FBX** (`GoldNode.fbx`, `Wall.fbx`, `ArrowTower.fbx`) are **UNTRACKED** as intended.
- **4 .uasset** (`SM_GoldNode`, `SM_Wall`, `SM_ArrowTower`, `M_GoldGlow`) currently show **STAGED (`A`)** in the index. I did NOT stage them — this is the **UE editor's Revision Control (Git) provider auto-`git add`-ing new assets on save**. (Note: TASK-037's unit .uasset are currently untracked, so the state is inconsistent across the two art passes.)
- These 4 files are exactly TASK-040's commit targets, so a commit works either way. If you want them untracked first, a single `git restore --staged <paths>` unstages them (non-destructive). I left them as-is rather than run a git mutation against the "do not touch Git" constraint.

## Downstream contract (character-exact, CONVENTIONS §Per-card visual assets)

- `SM_ArrowTower` and `SM_Wall` are the `SM_<CardID>` visual meshes resolved by string for placement ghosts and referenced by `BP_Building_ArrowTower` / `BP_Building_Wall` (VisualMesh component). `SM_GoldNode` is the visual for `AGoldNode` level instances (GoldNode_Blue/-Red).
- Orientation: Wall and Tower are radially/bilaterally symmetric — no facing contract; place at any yaw. GoldNode is a radial cluster, also yaw-agnostic.
- M7 replaces these blockouts with final art at the same path contract — treat none as final.
