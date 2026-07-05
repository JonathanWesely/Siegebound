# TASK-067 Handoff — Building blockout meshes: SM_BombTower, SM_BallistaTower, SM_Barracks, SM_DeepMine (art-director)

Status: **COMPLETE — all four modeled, exported, imported, materialed, UCX-collided (authored tight hulls), verified in-editor (dims/tris/collision/material/thumbnails), and saved.** Blender MCP (port 9876) + UE5 MCP both live this session. **Editor was FREE — checked `IsPIERunning` → false before importing (Jonathan not mid-PIE); did NOT wait, did NOT boot/close/bounce the editor, left it UP.** No Git run (build-master owns commit at TASK-069). No TASKBOARD edit (orchestrator/manager own the board).

Workflow = TASK-038 structure-mesh lineage (authored UCX box hulls in the FBX, consumed by the importer as `bIsGenerated:false` convex collision), blockout tier (GDD §6, premium art DEFERRED to M7), beveled (1-seg angle-limited chamfer, 50° limit, 0.02 m width), NO textures (team material carries all color). All ground-center origin (minZ ≈ 0, XY bbox centered) → each sits ON the floor at Z=0.

## Deliverables (all verified in-editor, UE LOD0)

| Mesh | Asset path | FBX source | UE bounds X×Y×Z | Target (footprint × tall) | Tris | UCX (authored, tight) | Nanite |
|------|-----------|-----------|-----------------|---------------------------|------|-----------------------|--------|
| SM_BombTower    | `/Game/Meshes/SM_BombTower`    | `Content/RawAssets/BombTower.fbx`    | 250.0 × 250.0 × 451.0 | ~250×250, ~450 ✓ | 580  | 1 convex box, X±125 Y±125 Z 0..451 (250×250 = base footprint) | Off |
| SM_BallistaTower| `/Game/Meshes/SM_BallistaTower`| `Content/RawAssets/BallistaTower.fbx`| 250.0 × 270.0 × 500.0 | ~250×250, ~500 ✓ | 512  | 1 convex box, X±125 Y±125 Z 0..500 (250×250 = base footprint) | Off |
| SM_Barracks     | `/Game/Meshes/SM_Barracks`     | `Content/RawAssets/Barracks.fbx`     | 400.0 × 419.0 × 348.9 | ~400×400, ~350 ✓ | 312  | 1 convex box, X±200 Y±200 Z 0..348.9 (400×400 = hall footprint) | Off |
| SM_DeepMine     | `/Game/Meshes/SM_DeepMine`     | `Content/RawAssets/DeepMine.fbx`     | 300.0 × 305.0 × 300.0 | ~300×300, ~300 ✓ | 1064 | 1 convex box, X±150 Y±150 Z 0..300 (300×300 = platform footprint) | Off |

Absolute FBX paths:
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\{BombTower,BallistaTower,Barracks,DeepMine}.fbx`

- All four tri counts FAR under the 15k budget (max 1064). UE LOD0 counts match Blender triangulation exactly (FACE smoothing + smart-project UVs → no seam-split inflation).
- **minZ ≈ 0 on all four (measured −2e-5..−3e-5)** → ground-center origin. Object origin (0,0,0) = footprint center at floor level.
- **Dims all within ±10% of target.** The Y bounds exceed the nominal footprint on three meshes — this is intended VISUAL OVERHANG that sits OUTSIDE the tight collision hull, not a footprint change:
  - BallistaTower Y 270 = the long horizontal bolt tip projecting forward (−Y) 20u past the 250 base plinth (weapon overhang, like the Cavalry lance).
  - Barracks Y 419 = the front doorway frame + banner pole protruding 19u past the 400 hall wall.
  - DeepMine Y 305 = the ore cart + rail protruding 5u past the 300 platform.

## Collision (verified via BodySetup_0.AggGeom)

Every mesh has **exactly ONE authored convex element** (`bIsGenerated:false` → my UCX hull, NOT importer auto-generated), `collisionEnabled: QueryAndPhysics`, `CollisionTraceFlag: CTF_UseDefault`. Each hull's `elemBox` equals the **nominal footprint** exactly and no wider:

| Mesh | UCX extents (UE) | Footprint | Tight? |
|------|------------------|-----------|--------|
| SM_BombTower     | X ±125, Y ±125, Z 0..451   | 250×250 | ✓ = base plinth, not wider |
| SM_BallistaTower | X ±125, Y ±125, Z 0..500   | 250×250 | ✓ = base plinth; bolt overhang is OUTSIDE the hull |
| SM_Barracks      | X ±200, Y ±200, Z 0..348.9 | 400×400 | ✓ = hall walls; doorway/banner OUTSIDE the hull |
| SM_DeepMine      | X ±150, Y ±150, Z 0..300   | 300×300 | ✓ = platform; ore-cart overhang OUTSIDE the hull |

- **M1 castle-plinth dead-zone lesson honored:** no hull is wider than its footprint, and all decorative overhangs (bolt/doorway/cart) are deliberately left outside the collision so they never inflate the placement blocker. UE stores UCX as convex elements (not box primitives) — functionally an exact box blocker at the footprint, full building height.

## Materials

- Each mesh: **exactly ONE** material slot named **`MI_TeamColor_Blue`** (from the Blender slot name in the FBX), bound on **slot 0** to **`/Game/Materials/Instances/MI_TeamColor_Blue`**. Verified via `get_material` on all four → `/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue`.
- Imported with `import_materials=False` / `import_textures=False` → the importer created **NO** material/texture assets; each `import_file` returned ONLY the primary `SM_` StaticMesh (no `/Game/RawAssets/` artifacts, no stray UCX asset — UCX was consumed as collision).
- Per the M3 team-color contract (CONVENTIONS §Team-driven visuals): `ABuilding`/BP recolors `VisualMesh` slot 0 to `MI_TeamColor_<Team>` in BeginPlay from `Team`; the Blue instance authored here is the design-time placeholder, so the bot's Red buildings recolor at spawn.

## Silhouette / read (blockout intent — thumbnails confirm)

- **SM_BombTower** — "lobs bombs": stepped square stone tower with a wide open **cauldron/mortar bowl flaring upward** at the top and a bomb resting in it. The funnel/cauldron is the dominant read.
- **SM_BallistaTower** — "long-range sniper": tall slim tower with a turntable-mounted **big horizontal bolt-thrower** on top — wide bow limbs spanning X, a long thin bolt projecting forward (−Y), and a winch drum at the back. The horizontal weapon crossing the top = "reach / sniper."
- **SM_Barracks** — "spawns troops": wide low **hall with a pitched gable roof**, a framed **front doorway** (two posts + lintel) and a **banner** hung over it. Reads as a troop hall/tent with an entrance.
- **SM_DeepMine** — "economy building", **DISTINCT from SM_GoldNode**: a built timber **pit-head headframe/derrick** (4 posts + top platform + overhanging **pulley wheel**) over a shaft-mouth collar, with an **ore cart on a short rail** in front, all on a rock platform. Man-made, mechanical, boxy — nothing like the GoldNode's glowing radial crystal cluster, so players won't confuse the two.

## Orientation, origin, integration notes

- Modeled facing **Blender −Y**, exported `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True` (1 m = 100 UE), `mesh_smooth_type='FACE'` — identical convention to TASK-038 / units. Result: each imports **facing +Y in UE asset space**, front = the **−Y (min-Y) side**.
- **Directional fronts (−Y in asset space):** BallistaTower bolt, Barracks doorway/banner, DeepMine ore-cart+pulley all sit toward −Y. BombTower is near radially symmetric (square tower + round cauldron) — effectively yaw-agnostic.
- Facing is **cosmetic** for these buildings — the Ballista blind spot (`MinRange`, GDD §4) is a radial code rule, not a mesh feature, so orientation does not gate firing. If integration wants a doorway/bolt to face the lane, apply a yaw on `VisualMesh` (buildings do NOT use the units' −90° convention; ArrowTower/Wall were placed yaw-agnostic in TASK-038). No re-export needed either way.
- Ground-center origin means the team vertical gradient (bounds-fit, darkest at floor → brightest at top) needs no tuning.
- These are the `SM_<CardID>` visual meshes resolved by string for placement ghosts and referenced by the per-card building BPs (`BP_Building_BombTower` / `BP_Building_BallistaTower` / `BP_Building_Barracks` / `BP_Building_DeepMine`, VisualMesh component) per CONVENTIONS §Per-card visual assets.

## Warnings / cleanliness

- **ZERO import warnings / errors.** Output-log block for all four reads `Triangulating static mesh SM_<Name>` → `Triangulating mesh UCX_SM_<Name>_00 for collision model` → `Building static mesh` → `Built static mesh [0.0Xs]` with no Warning/Error lines. A dedicated log search for `MikkTSpace | degenerate | bi-normal | binormal | smoothing group | nearly zero | no smoothing` returned **EMPTY** — the authored smart-project UV channel + FACE smoothing groups prevent the tangent/bi-normal warnings that TASK-038's first (UV-less) pass hit. (The "Waiting for static meshes to be ready" lines are just my get_bounds/get_triangle_count queries forcing a build wait — not warnings.)
- No auto-import artifacts: no `/Game/RawAssets/` UE content created (only the raw `.fbx` on disk under `Content/RawAssets/`). Nanite Off on all four (blockout low-poly).

## Git state (NOT committed — read this, build-master @ TASK-069)

- I ran **no git** (no add/reset/commit/push/status). Left entirely for TASK-069.
- New on disk to commit: **4 FBX** (`BombTower.fbx`, `BallistaTower.fbx`, `Barracks.fbx`, `DeepMine.fbx`) in `Content/RawAssets/`, and **4 `.uasset`** (`SM_BombTower`, `SM_BallistaTower`, `SM_Barracks`, `SM_DeepMine`) in `Content/Meshes/`, all saved.
- The UE Revision Control (Git) provider may auto-`git add` the new `.uasset` on save (observed on prior art passes) — left as-is; auto-stage is fine for TASK-069's commit. If build-master wants them unstaged first, a single non-destructive `git restore --staged <paths>` does it.

## Downstream contract (character-exact, per CONVENTIONS §Per-card visual assets)

`SM_BombTower`, `SM_BallistaTower`, `SM_Barracks`, `SM_DeepMine` are the `SM_<CardID>` visual meshes for the four new M4 buildings (CardIDs BombTower / BallistaTower / Barracks / DeepMine). M7 replaces these blockouts with premium art at the same path contract — treat none of this as final art.
