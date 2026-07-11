# TASK-134 handoff — Procedural battlefield scatter (C++ files)

**Status:** ready-for-qa · files only, NO compile / NO editor / NO Git (per spec). CLASS + LOGIC only; the DataAsset instance + level actor are TASK-137 (build-master).

## Files created
- `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` — `USiegeScatterConfig` (UDataAsset) + `FScatterLayer` (USTRUCT) + `EScatterRegionBias` (UENUM).
- `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.cpp` — trivial (pure data container; .cpp exists so the module compiles the reflected types).
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` — `ASiegeBattlefieldScatter` (AActor) + `LogSiegeTerrain` category (extern).
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` — the scatter logic + traversability guarantee. `DEFINE_LOG_CATEGORY(LogSiegeTerrain)` here.

## Files modified
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`
  - Added `#include "Siegebound/BattlefieldScatter.h"`.
  - `PlayAgain()` step 7 (new, at the END of the reset sequence): finds the `ASiegeBattlefieldScatter` via `TActorIterator` (null-safe — no actor = no-op) and calls `ClearScatter()` + `GenerateScatter()`. The actor owns the re-randomize/seed decision (`bReRandomizeOnMatchReset`).
  - `GetHeroStartTransform()` comment: stale PlayerStart `(-1700,0,100)` → `≈(-6800,0,100)` (M6.5 4× widening, TASK-136).
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h`
  - `GetHeroStartTransform` doc comment: same `(-1700,0,100)` → `≈(-6800,0,100)` fix.

**No other file touched.** ZERO combat/stat change. cards.csv / DT_Cards / card actors untouched.

## Config schema — `USiegeScatterConfig` (DA_BattlefieldScatter, TASK-137 populates)
`FScatterLayer` (one per TREES / ROCKS / HILLS / GRASS):
| field | type | default | meaning |
|---|---|---|---|
| `LayerName` | FName | None | debug/log name |
| `Meshes` | `TArray<TSoftObjectPtr<UStaticMesh>>` | empty | candidate meshes; ONE HISM per unique mesh; each instance picks a random one (variety) |
| `InstanceCount` | int32 | 0 | instances to ATTEMPT (rejection sampling may place fewer) |
| `ScaleRange` | FVector2D | (1,1) | uniform per-instance scale [min,max] |
| `bRandomYaw` | bool | true | random 0-360° yaw |
| `bBlocking` | bool | **true** | TRUE = obstacle (Pawn-block + carve nav + honor keep-clear); FALSE = grass (NoCollision, no-nav, ignores keep-clear) |
| `RegionBias` | EScatterRegionBias | WholeField | WholeField / EdgeBias / CenterBias density weighting (never overrides keep-clear) |
| `MinSpacing` | float | 300 | min 2D spacing between same-layer instances |
| `ZOffset` | float | 0 | per-mesh pivot correction (art flags in TASK-135) |

Config-level: `Layers[]`, `bMirrorSymmetric` (**default FALSE** = asymmetric organic random; TRUE mirrors across X=0), `ArenaHalfExtent` (default (8600,3200)), and keep-clear radii `CastleKeepClearRadius`=900 / `GoldNodeKeepClearRadius`=500 / `PlayerStartKeepClearRadius`=700 / `CorridorHalfWidth`=400.

## Seed / re-randomize
`GenerateScatter()`: seed = `OverrideSeed` if >0, else re-use `LastSeed` when `bReRandomizeOnMatchReset==false` & already seeded, else a fresh `FMath::RandRange(1, MAX_int32-1)`. The chosen seed is logged on **`LogSiegeTerrain`** as one grep-able line (`GenerateScatter seed=… mirror=… layers=… corridorHalfY=…`) so any layout is reproducible. BeginPlay does the initial scatter; SiegeGameMode reset re-scatters.

## TRAVERSABILITY GUARANTEE — the implementation (spell-out for QA)
Two independent layers; the first alone is sufficient, the second confirms:

1. **DETERMINISTIC — reserved corridor + keep-clear (the real guarantee).** `IsInKeepClear(Point2D)` returns true (⇒ blocking layer rejects the candidate) when EITHER:
   - `|Y| <= CorridorHalfWidth` (default 400) **across the whole X span** — so the straight `Y≈0` lane between the two castles is NEVER obstructed by any blocking obstacle. This lane is on the flat slab and is always walkable, independent of any async nav state. This alone guarantees a Blue→Red path every match.
   - the point is inside any keep-clear disc: both castles (r 900 @ ±8000), both gold nodes (r 500 @ ±7200), the PlayerStart (r 700 @ ≈-6800). Discs are rebuilt each generate from the LIVE `ACastle`/`AGoldNode`/`APlayerStart` actors (`RebuildKeepClearZones`), with CONVENTIONS-coordinate fallbacks if an actor is missing.
   - GRASS (`bBlocking==false`) skips keep-clear entirely (lush everywhere).

2. **CONFIRMATORY — deferred nav reachability check (`ValidateTraversability`).** Scheduled on a timer `NavSettleDelay` (0.75 s) after `GenerateScatter` so the async **Dynamic** navmesh has time to carve the new obstacles. The two castle endpoints are inset toward the centerline by `CastleQueryInset` (1200) onto open pad ground first, so the query never starts/ends inside the castle's own nav-carved hole (a false negative). It path-queries `UNavigationSystemV1::FindPathToLocationSynchronously(Blue-castle, Red-castle)`; pass = `Path && Path->IsValid() && !Path->IsPartial()` → logs CONFIRMED and stops. If (defensively) NOT reachable, it culls blocking HISM instances within a widening `|Y| <= band` (band = corridor + attempt×`CorridorWidenStep`) via `CullCorridorBlockers` — which iterates only nav-affecting HISMs and `RemoveInstances` those in the band — then re-schedules the check (up to `MaxReachabilityAttempts`=5). RemoveInstance re-dirties the Dynamic nav. Because clearing the band restores an obstacle-free straight lane, this converges. No nav system ⇒ logged warning + rely on the corridor (still guaranteed).

**Why this is not a hard-failure risk:** the corridor exclusion means obstacles are geometrically incapable of blocking the Y≈0 castle-to-castle lane; the nav query is a belt-and-suspenders confirmation, not the primary mechanism, so async-nav timing can only affect the (essentially never-taken) cull path, never the guarantee itself.

## Collision / nav wiring (per unique mesh, `ResolveComponentForMesh`)
- **Blocking layers:** `QueryOnly` collision, ObjectType `WorldStatic`, response Ignore-all + **Block `ECC_Pawn`** only (units/hero route around; projectiles pass through cosmetically — accepted M6.5 gap), `SetCanEverAffectNavigation(true)`. Nav flag + mobility set BEFORE `RegisterComponent`.
- **Grass layer:** `NoCollision`, `SetCanEverAffectNavigation(false)`.
- One `UHierarchicalInstancedStaticMeshComponent` per unique mesh (perf law); components reused across re-scatter, only instances cleared.

## QA scan notes
- **Shadow scan:** no local/param shadows an inherited reflected member. `ResolveCastleLocation(ETeamId Team)` — `Team` is a param on `ASiegeBattlefieldScatter`, which has NO inherited reflected `Team` (that member lives on ACastle/AGoldNode, different classes). Loop vars `It`/`L`/`Comp` are block-scoped.
- **Complete-type-include scan (the flagged trap):** all dereferenced types are fully included in the .cpp — HISM (`Components/HierarchicalInstancedStaticMeshComponent.h`), `USceneComponent`, `UStaticMesh`, `UWorld`, `FHitResult` (`Engine/HitResult.h`), `FCollisionQueryParams` (`CollisionQueryParams.h`), `TActorIterator` (`EngineUtils.h`), `APlayerStart`, `ACastle`, `AGoldNode`, `USiegeScatterConfig`/`FScatterLayer`/`EScatterRegionBias` (`ScatterConfig.h`), `UNavigationSystemV1` (`NavigationSystem.h`), `UNavigationPath` (`NavigationPath.h` — for `IsValid()`/`IsPartial()`).
- `NavigationSystem` + `AIModule` are already in `GitClaudeUnrealTest.Build.cs` (no Build.cs change needed).

## Downstream dependencies
- **build-master (TASK-136) — HARD DEPENDENCY:** `L_Arena`'s `RecastNavMesh` MUST be `RuntimeGeneration = Dynamic`. Without it the runtime-scattered obstacles do NOT carve the navmesh and units walk straight through them — the whole blocking/routing behavior (and the nav-confirmation half of the guarantee) is inert. (The geometric corridor still holds regardless.) Also: compile these files warnings-as-errors; move castles/nodes/PlayerStart to ±8000/±7200/≈-6800 (the keep-clear discs read those live actors).
- **build-master (TASK-137):** create `/Game/Data/DA_BattlefieldScatter` (a `USiegeScatterConfig`) and place ONE `ASiegeBattlefieldScatter` named `BattlefieldScatter` in `L_Arena` with `ScatterConfig` pointing at the DataAsset.
- **art-director (TASK-135) → feeds TASK-137's DataAsset population:** the per-layer `Meshes` lists (soft refs to the curated Fab meshes — PREFER mobile/low-poly variants), per-layer `InstanceCount`, `ScaleRange`, `MinSpacing`, `ZOffset` (pivot corrections), `bBlocking` (TRUE trees/rocks/hills, FALSE grass), and any keep-clear radius tuning. Obstacle meshes MUST carry footprint collision or they won't carve nav.
