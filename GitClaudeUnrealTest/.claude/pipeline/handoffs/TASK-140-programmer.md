# TASK-140 handoff — Scatter C++ (M6.6 climbable terrain)

**Author:** gameplay-programmer
**Status:** ready-for-qa
**Compile:** NOT compiled (per task — build-master compiles at TASK-143). Files only.

## Files touched (only these three)
- `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h`
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h`
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp`

> NOTE on paths: the task spec listed `Source/GitClaudeUnrealTest/GitClaudeUnrealTest/Siegebound/...` (double dir). The real tree is `Source/GitClaudeUnrealTest/Siegebound/...` — that is what I edited.

## Assets referenced (none new; contract only)
- The `"Terrain"` actor tag string (must match `AProjectile::FindEnvironmentImpact`, `Projectile.cpp:386` — verified `TerrainTagName = FName(TEXT("Terrain"))`).
- Trees' proxy mesh is data (`CollisionProxyMesh` on the DA), set by build-master at TASK-144 (`/Engine/BasicShapes/Cylinder`). No code hardcodes it.

## What each spec item does (implementation summary)

1. **Terrain tag** — `ASiegeBattlefieldScatter()` ctor now `Tags.Add(FName(TEXT("Terrain")))`. Exact string `Terrain` (NOT `Obstacle`). Closes projectile pass-through (decision #3).

2. **Real-geometry blocking channels** — in `ResolveComponentForMesh`, the real-geometry blocker branch (rocks/slabs/hills) now ADDS `SetCollisionResponseToChannel(ECC_Visibility, ECR_Block)` + `(ECC_Camera, ECR_Block)` + `bFillCollisionUnderneathForNavmesh = true`, keeping the existing `ECC_Pawn` block + `SetCanEverAffectNavigation(true)` + `ECC_WorldStatic` object type. WorldStatic RESPONSE stays Ignore (GroundZAt traces it).

3. **Radius-aware keep-clear (headline fix)** — `IsInKeepClear` gained a `float InstanceRadius = 0.f` param. Corridor test inflated to `|Y| <= CorridorHalfWidthCached + R`; each zone inflated to `DistSq <= (sqrt(RadiusSq)+R)^2`. Both call sites (direct `:265` + mirror `:318`) pass the instance's `FootprintR`.

4. **Field-edge clamp (new)** — candidate rejected when `|X|+R > HalfX || |Y|+R > HalfY`. Mirror twin has identical |X|,|Y|, so it inherits the clamp automatically.

5. **Radius derivation + LOOP REORDER** — new `FScatterLayer::FootprintRadius` (0 = auto = `FVector2D(Bounds.BoxExtent.X,.Y).Size() * Scale`; >0 = absolute cm override, NOT scale-multiplied). The mesh+scale rolls MOVED to before the keep-clear/edge/spacing tests (they were after). See the SEED-REORDER warning below.

6. **Radius-aware MinSpacing** — `PlacedPoints` is now `TArray<TPair<FVector2D,float>>` (center + radius); rejection test is `DistSq < (MinSpacing + Ri + Rj)^2`. Applies to all layers (grass included — build-master may re-tune grass MinSpacing/counts if the field reads thin, see below).

7. **Tree collision-proxy** — new `FScatterLayer` fields `CollisionProxyMesh` / `CollisionProxyScale=(1,1,1)` / `CollisionProxyZOffset=0`. When `CollisionProxyMesh` is set: the VISUAL HISM goes to the NoCollision + no-nav branch; a paired PROXY HISM (`ResolveProxyForVisual`) renders invisible (`SetVisibility(false)`, `SetCastShadow(false)`), QueryOnly, blocks **Pawn only** (NOT Visibility/Camera), nav on, fill-underneath on. Exactly ONE proxy per visual HISM (keyed in `VisualToProxy`), so visual/proxy instance indices stay parallel. Proxy instance transform = same X,Y,Yaw; scale = `CollisionProxyScale * instanceScale`; Z = `visualZ + CollisionProxyZOffset * instanceScale` (Z offset IS scaled so a centered-pivot cylinder stays grounded across the scale range).

8. **Channel rule** — real-geometry blockers block Pawn+Visibility+Camera; tree proxies block Pawn only; grass NoCollision+no-nav (unchanged).

**Also:** `MaxPlacementAttemptsPerInstance` bumped 16 → 24 (radius tests raise rejection pressure).

## ⚠️ SEED-REORDER WARNING (for build-master — NOT a regression)
Deriving the footprint radius requires the mesh + scale, so the mesh/scale `FRandomStream` draws MOVED from *after* the keep-clear/spacing tests to *before* them (BattlefieldScatter.cpp ~:234/:240, with a loud in-code comment). This changes the stream draw sequence, so **existing random seeds now produce DIFFERENT (still-valid) layouts.** A fixed `OverrideSeed` remains fully deterministic — it just maps to a new layout than it did pre-M6.6. Do not treat a changed layout at a given seed as a bug.

## What QA should scrutinize (per TASK-142 mandate)
- **Visual/proxy cull-desync (item 7 — the one that bites):** `CullCorridorBlockers` iterates nav-relevant comps. A proxy-layer VISUAL HISM has nav OFF so it is skipped there; the PROXY HISM (nav ON) is processed, and I now call `FindVisualForProxy(Comp)` and `RemoveInstances(ToRemove)` on the paired visual with the SAME index array BEFORE removing from the proxy. Indices are parallel by construction (every proxy instance is added in the same tick as its visual, in both the primary and mirror paths), and removing the same index set from both preserves parallelism. Grass (nav off) and real blockers (no pair) are unaffected.
- **Shadow law:** new param is `InstanceRadius`; new locals `FootprintR`, `MinDist`, `Other`, `ProxyScaleVec`, `ProxyZ`, `bUsesProxy`, `MeshBounds`, `Proxy`, `Visual`, `Existing`, `Pair` — none shadow an inherited reflected member (no `Owner`/`Instigator`/`Controller`/`Slot` etc.). New fields `FootprintRadius`/`CollisionProxyMesh`/`CollisionProxyScale`/`CollisionProxyZOffset` are distinct from any `UStaticMeshComponent`/`UInstancedStaticMeshComponent` field. `VisualToProxy` is a new distinct member.
- **Complete-type include law:** everything dereferenced has its full header already in the .cpp — `Components/HierarchicalInstancedStaticMeshComponent.h` (HISM + inherited `bFillCollisionUnderneathForNavmesh` on UPrimitiveComponent, `SetVisibility`/`SetCastShadow`), `Engine/StaticMesh.h` (`Mesh->GetBounds()` → `FBoxSphereBounds`). No new include needed. `FBoxSphereBounds`/`TPair`/`FVector2D` are CoreMinimal. `ECC_Visibility`/`ECC_Camera` are EngineTypes (already-used `ECC_Pawn`/`ECC_WorldStatic` prove they resolve).
- **`VisualToProxy` GC:** deliberately a NON-UPROPERTY raw-pointer `TMap<UHISM*,UHISM*>` (matches the spec's literal type). GC-safety is via `ScatterComponents` (a `UPROPERTY(Transient) TArray<TObjectPtr>`) which holds BOTH the visual and proxy — the map is only a secondary index and never outlives them (all torn down together on actor destroy). Chosen over a UPROPERTY TMap to sidestep any UHT object-key-map quirk; the objects are already rooted.
- **`IsInKeepClear` caller sweep:** only two callers, both in `ScatterLayer`, both pass `FootprintR`. Declaration default `= 0.f` keeps the signature safe.
- **Projectile object-type nuance (regression #7 relevant):** `FindEnvironmentImpact` uses `LineTraceMultiByObjectType(ECC_WorldStatic, ECC_WorldDynamic)` — an OBJECT-type query that ignores per-channel responses. So the tree PROXY (object type WorldStatic, QueryOnly) IS returned by the projectile trace and arrows die on tree trunks (desired, decision #3), WHILE the building placement ghost (a Visibility/Camera CHANNEL trace) still passes through the proxy because the proxy Ignores Visibility/Camera. No conflict — both behaviors hold. The real-geometry hill/rock hulls are SIMPLE collision, and the projectile trace is `bTraceComplex=false`, so arrows die on hills via the convex hull.

## Known invariant (not a code guard — flagged for the config)
`ResolveComponentForMesh` reuses a HISM by matching `GetStaticMesh() == Mesh`, and proxy HISMs live in `ScatterComponents` too. This is safe ONLY while no layer lists the same mesh as both a visual `Meshes` entry AND another layer's `CollisionProxyMesh`. The shipping config satisfies this (trees' proxy is the engine Cylinder; no layer scatters a Cylinder as a visual). Build-master: do not set a layer's visual mesh to the Cylinder proxy mesh.

## Build-master notes (TASK-144)
- New per-layer fields are unset by default → the Rocks/Hills/Slabs layers keep real-geometry blocking automatically (no proxy). Only the Trees layer sets `CollisionProxyMesh`.
- `CollisionProxyZOffset` IS multiplied by the instance uniform scale in code — your ≈+850 derivation (½ of a 17× 100-unit cylinder) is correct at scale 1 and stays grounded at other scales.
- Grass now gets radius-aware spacing + edge clamp; if the wider field reads thin, lower grass `MinSpacing` (radius adds to it).
