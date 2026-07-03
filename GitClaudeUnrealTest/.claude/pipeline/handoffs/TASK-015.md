# TASK-015 — L_Arena blockout level (art-director handoff)

## Level
- **/Game/Maps/L_Arena** (`Content/Maps/L_Arena.umap`) — classic (non-world-partition) level, created by duplicating the engine `Template_Default` map. Saved clean; `Lvl_ThirdPerson` untouched.

## Actors (label — class — exact world transform)

| Label | Class | Location | Rotation | Scale / Size |
|---|---|---|---|---|
| ArenaGround | StaticMeshActor (`/Engine/BasicShapes/Cube`) | (0, 0, -50) | (0, 0, 0) | scale (64, 32, 1) → slab 6400 x 3200 x 100, **top surface exactly at Z=0**, full collision, walkable |
| CastleAnchor_Blue | TargetPoint | **(-2000, 0, 0)** | (0, 0, 0) | 1,1,1 |
| CastleAnchor_Red | TargetPoint | **(+2000, 0, 0)** | (0, 0, 0) | 1,1,1 |
| PlayerStart | PlayerStart | **(-1700, 0, 100)** | yaw 0 → **faces +X** (toward Red castle) | 1,1,1 |
| CenterlineMarker | DecalActor | (0, 0, 0) | pitch -90 (projects down) | DecalSize half-extents (250, 1600, 30) → stripe 60 (X) x 3200 (Y) along the X=0 plane |
| NavMeshBounds_Arena | NavMeshBoundsVolume | (0, 0, 0) | (0, 0, 0) | scale (34, 18, 5) → 6800 x 3600 x 1000 (covers entire ground + margin) |
| RecastNavMesh-Default | RecastNavMesh | auto-generated | — | built tile bounds X -3952..+3952, Y -1976..+1976 → navmesh spans **both halves**, castle-to-castle pathing works |

Template lighting kept as-is (defaults, per spec): DirectionalLight, SkyLight, SkyAtmosphere, ExponentialHeightFog, VolumetricCloud, plus the template SM_SkySphere backdrop.

## New asset created
- **/Game/Materials/M_CenterlineStripe** — Material, domain = DeferredDecal, blend = Translucent, emissive constant (5.0, 4.0, 0.5) warm gold. Used only by CenterlineMarker. Not part of the TASK-012 TeamColor/Ghost contract; the M7 premium pass can restyle it in place.

## Implementation note — centerline is a decal, not a mesh plane
Spec allowed "thin emissive plane or decal". A BasicShapes plane kept its physics collision regardless of `BodyInstance` property writes over MCP, so it was replaced with a **DecalActor**, which has no collision by design. Verified by line trace at the centerline: nothing hittable above the ground surface at X=0.

## Verification performed
- All transforms above queried back from the level after save — exact matches.
- ArenaGround bounds: min (-3200, -1600, -100), max (3200, 1600, 0).
- Trace (0, 500, 50) → (0, 500, 0.5): no hit → CenterlineMarker has zero gameplay collision.
- Viewport capture: stripe visibly divides the two halves; anchors and PlayerStart in place.
- PIE smoke test: session started on L_Arena, world came up for play with no errors, stopped cleanly.
- `is_dirty` false on both saved assets (level + material).

## Notes for integration (build-master)
- Place `Castle_Blue` (Team=Blue) at **CastleAnchor_Blue (-2000, 0, 0)** and `Castle_Red` (Team=Red) at **CastleAnchor_Red (+2000, 0, 0)**. Anchors sit at ground level (Z=0); SM_Castle's origin is ground-center (TASK-013), so castles can take the anchor transform directly.
- Adding castles (colliding actors) will trigger the automatic runtime/editor navmesh rebuild around them — no manual nav setup needed.
- Open space near (±1200, 0) intentionally left empty for M2 gold nodes.

## Notes for TASK-009 / TASK-010 PIE tests
- PlayerStart faces +X, so the possessed hero spawns looking at the Red half.
- Ground is walkable everywhere; navmesh (visualize with P) covers both halves — a `BP_Unit_Footman` dropped on the Blue half can path to the Red castle anchor area.
- Blue placement half = X <= 0, per the CONVENTIONS world-axes contract; the stripe marks the boundary visually only (no collision, no gameplay effect).

## Source files
- None in ArtStaging — the blockout was built entirely in-editor from engine primitives; no Blender assets were required for this task.
