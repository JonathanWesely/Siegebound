# TASK-037 Handoff — Unit blockout meshes: SM_Miner, SM_Archer, SM_Knight (art-director)

Status: **COMPLETE — all three modeled, exported, imported, verified, and saved.** Blender MCP + UE5 MCP both live this session. Board status flip is the orchestrator's (I did not edit TASKBOARD.md). No Git run (build-master owns commit, TASK-040).

Family style: SM_Footman lineage (GDD §6) — chunky ~3 heads tall, oversized hands/weapons, beveled (1-seg angle-limited chamfer), blockout tier, NO textures (team material carries color). Distinct silhouettes verified at distance via Workbench ortho renders (front + 3/4), scratchpad-temporary.

## Deliverables (all verified in-editor)

| Mesh | Asset path | FBX source | Bounds X×Y×Z (UE units) | Height | minZ | Tris (LOD0, UE) | Nanite |
|------|-----------|-----------|-------------------------|--------|------|-----------------|--------|
| SM_Miner  | `/Game/Meshes/SM_Miner`  | `Content/RawAssets/Miner.fbx`  | 129.5 × 62.0 × 173.0 | 173 (~170 ✓) | ~0 | 1124 | Off |
| SM_Archer | `/Game/Meshes/SM_Archer` | `Content/RawAssets/Archer.fbx` | 125.0 × 67.3 × 180.0 | 180 (✓)      | ~0 | 1558 | Off |
| SM_Knight | `/Game/Meshes/SM_Knight` | `Content/RawAssets/Knight.fbx` | 162.0 × 62.7 × 190.0 | 190 (✓)      | ~0 | 1244 | Off |

- All tri counts well under the 8k budget. minZ = 0 (measured ~−2e-6, i.e. exactly on the floor) → **feet-center origin**; each mesh sits ON the floor when placed at Z=0.
- Height ordering reads correctly at a glance: Knight (190, widest 162) > Archer (180) > Miner (173). Widths also rank Knight > Miner ≈ Archer, reinforcing the "heavy / normal / worker" read.
- Absolute FBX paths (repo-relative shown above):
  `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\{Miner,Archer,Knight}.fbx`

## Material (each mesh)
- Exactly **ONE** material slot, slot name **`MI_TeamColor_Blue`**, assigned to **`/Game/Materials/Instances/MI_TeamColor_Blue`** (the TASK-012 instance).
- Imported with `import_materials=False` / `import_textures=False` → the importer created **NO** new material or texture assets; `/Game/Meshes/` gained only the three `SM_` meshes. Slot then bound explicitly to the existing instance. Verified via `get_material` on each.

## Silhouette / read (blockout intent)
- **Miner** — civilian worker: chunky un-armored body, rounded **hard-hat dome + front brim**, **pickaxe shouldered on the +X side** (diagonal handle up to a horizontal double-point head that clears the head outline top-right). Reads "worker with tool," clearly not a soldier.
- **Archer** — ranged: leaner/taller body, **pointed ranger cap**, large **segmented bow arc with string held on an extended left (−X) arm** (the dominant front-silhouette element), **quiver + 3 arrow shafts/fletching on the back (+Y)** over the right shoulder.
- **Knight** — heavy tank: broadest/bulkiest body, **oversized pauldrons**, **great-helm with visor brow + crest fin**, big **heater shield (rounded top, pointed bottom, center boss) on the left (−X)**, **sword held point-up on the right (+X)**.

## Orientation, origin, integration notes
- Modeled facing **Blender −Y**, exported with default FBX axes (`axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True`, meters) — identical convention to SM_Footman (see `handoffs/TASK-014.md`). Result: each imports **facing +Y in UE asset space** (UE-mannequin convention).
- **Front of each figure = the −Y (min-Y) side** in UE asset space: Miner hard-hat brim, Archer bow+cap, Knight visor+shield all sit on −Y.
- Per TASK-014, the per-unit BP should give **VisualMesh a relative yaw of −90°** so the figure faces the actor's forward +X (same fix already used for `BP_Unit_Footman`). Weapon sides above are stated in **asset space before** that −90 yaw.
- Feet-center origin means the TASK-012 team vertical gradient auto-fits bounds (darkest at feet, brightest at head/helm) with no tuning.

## Collision
- Each mesh: importer auto-collision removed, replaced with **4-hull simple convex collision** (`generate_convex_collisions hull_count=4, max_hull_verts=12`) so placement-ghost / level use is sane (matches the "any simple collision" intent of TASK-014 step 3). Convex hulls DO include the weapon/shield overhang.
- **Gameplay collision is the pawn capsule** sized by TASK-010, not this mesh collision — the mesh is the `VisualMesh` component. If a BP enables mesh collision and the weapon-inclusive hull is undesirable, swap to a body-only capsule at integration; no re-export needed.

## Warnings / cleanliness
- **Zero import warnings.** Smoothing groups written on export (`mesh_smooth_type='FACE'` — avoids the "no smoothing group" warning); UV channel 0 `UVMap` present on every mesh (smart-project) — no missing-UV warning.
- No auto-import artifacts: `/Game/RawAssets/` in the content browser is empty (checked before and after). The FBXs live under `Content/RawAssets/`; if the editor's auto-reimport monitor ever prompts, dismiss it — the deliberate targets are `/Game/Meshes/SM_*`.
- All three `.uasset` saved to disk (`Content/Meshes/SM_{Miner,Archer,Knight}.uasset`) alongside their FBX in `Content/RawAssets/` — ready for build-master to stage/commit (TASK-040). Not placed in any level.

## Tri-count note
UE LOD0 triangle counts (authoritative) differ slightly from Blender face-triangulation counts (e.g. Archer Blender 1586 vs UE 1558) due to UV-seam splitting/triangulation on import. All UE counts are far under 8k.

## Downstream contract (character-exact, per CONVENTIONS §Per-card visual assets)
`SM_Miner`, `SM_Archer`, `SM_Knight` are the `SM_<CardID>` visual meshes resolved by string for placement ghosts and referenced by the per-card unit BPs (`BP_Unit_Miner` / `BP_Unit_Archer` / `BP_Unit_Knight`, VisualMesh component). M7 replaces these blockouts with skeletal meshes at the same path contract — treat none of this as final art.
