# TASK-105 Handoff — SM_CrystalTower blockout mesh (art-director)

Status: **COMPLETE — modeled, exported, imported NEW, materialed (2 slots), UCX-collided (authored footprint-exact hull), verified in-editor (dims/tris/collision/materials/render), and saved.** Blender MCP (port 9876) + UE5 MCP both live this session. **Editor was FREE — `IsPIERunning` → false before any editor mutation; editor left UP.** Editor mutations kept to the minimum for this one asset (M_CrystalGlow create + one import + slot binds + save) per the parallel-C++-agents constraint. No Git run. No TASKBOARD edit.

Workflow = TASK-067 building-blockout lineage (authored UCX hull in the FBX consumed as `bIsGenerated:false` convex collision), blockout tier (GDD §6, premium art deferred to M7), beveled (1-seg angle-limited chamfer, 50° limit, 0.02 m), FACE smoothing + smart-project UVs (layer `UVMap`), ground-center origin (minZ ≈ 0, XY bbox centered) → sits ON the floor at Z=0.

## Deliverables (all verified in-editor, UE LOD0)

| Asset | Path | Notes |
|-------|------|-------|
| **SM_CrystalTower** | `/Game/Meshes/SM_CrystalTower` | NEW StaticMesh (fresh import — nothing overwritten/deleted). Saved, not dirty. |
| **M_CrystalGlow** | `/Game/Materials/M_CrystalGlow` | NEW emissive crystal material (see below). Saved, not dirty. |
| FBX source | `Content/RawAssets/CrystalTower.fbx` | Blender export, axis contract `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale` (1 m = 100 UE), `mesh_smooth_type='FACE'`. |
| Silhouette captures | `.claude/pipeline/handoffs/TASK-105-silhouette.png` (3/4 view), `TASK-105-silhouette-side.png` (side), `TASK-105-editor-thumbnail.png` (UE render — confirms in-engine render + emissive bloom) | Acceptance captures. |

| Metric | Value | Budget / target | OK |
|--------|-------|-----------------|----|
| Tris (UE LOD0) | **1,212** (= Blender count exactly, no seam-split inflation) | ≤ 8k (spec) / ≤ 15k (§6 buildings) | ✓ |
| Bounds X×Y×Z | **250.0 × 250.0 × 487.8** | ~250×250 tower footprint, ~500 tall (tower family: ArrowTower 497, Ballista 500) | ✓ |
| minZ | −2.0e-05 | ground-center origin | ✓ |
| Nanite | **Off** | blockout law | ✓ |
| UV layer | `UVMap` (smart-project 66°, margin 0.02) | spec | ✓ |
| LODs | 1 (LOD0 only) | — | ✓ |

## Collision (verified via ObjectTools → BodySetup_0.AggGeom readback)

- **Exactly ONE authored convex element**, `bIsGenerated: false` (= my `UCX_SM_CrystalTower_00` from the FBX, NOT importer auto-generated; the log shows `Triangulating mesh UCX_SM_CrystalTower_00 for collision model`).
- elemBox: **X ±125, Y ±125, Z 0..500** — footprint-exact 250×250 (no wider than the plinth — M1 castle-plinth dead-zone lesson honored), full building height.
- `collisionEnabled: QueryAndPhysics`, `CollisionTraceFlag: CTF_UseDefault`, DefaultInstance profile `BlockAll`.
- Crystal-shard visual overhang: the crown shards splay to ~±129 radially at their tips (off-axis, so axis-aligned bounds stay exactly 250×250) — deliberately OUTSIDE the hull, decorative only (Ballista-bolt precedent).

## Materials (2 slots — readback-verified post-save)

| Slot | Slot name (from FBX) | Assigned material | Covers |
|------|----------------------|-------------------|--------|
| 0 | `MI_TeamColor_Blue` | `/Game/Materials/Instances/MI_TeamColor_Blue` | Stone body: plinths, corner piers, drum shaft, crown collar. **Design-time placeholder** — ABuilding recolors slot 0 to `MI_TeamColor_<Team>` at BeginPlay (team recolor law), so Red's tower recolors at spawn. |
| 1 | `M_CrystalGlow` | `/Game/Materials/M_CrystalGlow` | All crystal shards (central needle + 5 crown shards + 3 base crystals). **Team-agnostic emissive** — NOT touched by the recolor (slot 0 only), mirrors the M_GoldGlow gold-node exception: crystals glow the same cyan for both teams. |

- **M_CrystalGlow** (new, Opaque Lit): `Constant3Vector` cyan linear **(R 0.10, G 0.80, B 1.00)** → **BaseColor** and → `Multiply.A`; `Constant` **6.0** → `Multiply.B`; `Multiply` (≈ 0.6, 4.8, 6.0 HDR > 1) → **EmissiveColor**. Exact M_GoldGlow recipe (TASK-038), cyan instead of gold. Wiring re-verified via `get_property_input`; recompiled with no shader errors; blooms in the editor thumbnail.
- Imported with `import_materials=False` / `import_textures=False` → importer created NO material/texture assets; `import_file` returned ONLY the primary SM (no /Game/RawAssets/ artifacts, UCX consumed as collision).
- Color note for TASK-108: the cyan (0.10, 0.80, 1.00) was chosen to match the "cyan zap" language of `NS_ChainZap` — keep that VFX in the same cyan family for coherence.

## Silhouette / read (blockout intent — captures confirm)

"Chain-lightning crystal tower": two-step square stone **plinth** (250×250) with four 45°-rotated **corner piers** → tapered octagonal **drum shaft** → flared octagonal **crown collar** → **crystal cluster**: one tall central hexagonal **needle** (to ~z 488) ringed by **5 outward-fanned crown shards** (varied heights/tilts/spins, clear dark gaps between shards — reads as a cluster, not a blob), plus **3 small base crystals** growing outward from the plinth steps ("crystal infests the stonework" motif). Emissive cyan crystals vs team-colored stone = unmistakable at 15 m and clearly distinct from every other tower (ArrowTower roof spike / Ballista horizontal bolt / BombTower cauldron) AND from SM_GoldNode (gold radial cluster, no stone tower).

## Orientation, origin, integration notes (READ THIS, TASK-107)

- **Radially symmetric — yaw-agnostic.** No facing contract; place at any yaw (BombTower precedent). Chain attack is a radial code rule (Range 800), not a mesh feature.
- Ground-center origin: placed at terrain surface Z, it sits flush; the team vertical gradient (bounds-fit) needs no tuning.
- This is the `SM_<CardID>` visual mesh for CardID **`CrystalTower`** — resolved by string as `/Game/Meshes/SM_CrystalTower` by the placement ghost, and referenced by `BP_Building_CrystalTower`'s `VisualMesh` (TASK-107). Slot 0 recolor works unchanged (slot 0 is the team slot). **Do NOT override slot 1** — the crystal glow must stay authored for both teams.
- The placement ghost tints ALL slots (existing behavior) — crystals included while ghosting; normal.

## Warnings / cleanliness

- **ZERO import warnings/errors.** Log block: `Triangulating static mesh SM_CrystalTower` → `Triangulating mesh UCX_SM_CrystalTower_00 for collision model` → `Building static mesh` → `Built static mesh [0.01s]`. Dedicated search for `MikkTSpace | degenerate | bi-normal | binormal | smoothing group | nearly zero` returned **EMPTY**.
- No auto-import artifacts (no /Game/RawAssets/ UE content). Both assets saved, `is_dirty` = false.

## Git state (NOT committed — build-master)

- I ran **no git**. New on disk to commit: `Content/RawAssets/CrystalTower.fbx`, `Content/Meshes/SM_CrystalTower.uasset`, `Content/Materials/M_CrystalGlow.uasset`, plus the three capture PNGs in `.claude/pipeline/handoffs/` and this handoff. The UE Revision Control (Git) provider may auto-stage the new .uasset on save (observed on prior art passes) — left as-is.

## Downstream contract (character-exact)

`SM_CrystalTower` (/Game/Meshes/) + slot 0 `MI_TeamColor_Blue` placeholder per the names block. Card art `T_CardArt_CrystalTower` is TASK-106 (separate lane). M7 replaces this blockout with premium art at the same path contract — treat none of this as final art.
