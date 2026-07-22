# TASK-250 — Scatter-on-hills + OverrideMaterial field (C++, branch) — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-07-22 · **Branch:** `m7.6-arena10x` (verified checkout; HEAD `ec7a271` at start)
**Status:** ready-for-qa. NO compile run (per spec — rides the pre-W1 build-master bounce). No Git, no editor.

## Files touched (complete list)
- `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` — 3 new `FScatterLayer` fields + `UMaterialInterface` fwd-decl
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` — `HillSurfaceComponents` index, `ResolveHillAwareGroundZ` decl, GenerateScatter doc
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` — two-pass generation, hill-aware ground resolve (primary + mirror), OverrideMaterial application (visual + proxy), new helper, `Materials/MaterialInterface.h` include

## 1. DIAGNOSIS — why nothing places on hills (actual mechanism, with evidence)

**The board's expected mechanism ("hill footprint/MinSpacing acts as an exclusion zone") is NOT what happens.** `PlacedPoints` is a LOCAL array inside `ScatterLayer()` (cpp, placement loop) — MinSpacing/footprint rejection is strictly *same-layer*. Hills exert **zero cross-layer exclusion**; grass/rock/tree candidates land on hill XYs freely.

**The real mechanism: the ground trace can never see a hill surface, so anything landing on a hill's XY is grounded at the FLAT FLOOR Z underneath it — i.e., placed *inside* the hill mesh, entombed and invisible.** Two independent causes, either alone sufficient:

1. **`GroundZAt` (BattlefieldScatter.cpp) traces the `ECC_WorldStatic` channel, and hill HISMs IGNORE that channel.** The real-geometry blocker profile in `ResolveComponentForMesh` sets `SetCollisionResponseToAllChannels(ECR_Ignore)` then blocks only Pawn/Visibility/Camera — deliberately, per the CONVENTIONS scatter-channel law ("Leave ECC_WorldStatic on Ignore — GroundZAt traces WorldStatic for the placement-ghost ground height; blocking it would break placement"). A WorldStatic-channel down-trace passes straight through every hill.
2. **`GroundZAt`'s query params ignore the scatter actor wholesale:** `FCollisionQueryParams(TEXT("BattlefieldScatterGroundTrace"), false, this)` — the `this` is the ignored actor, and every hill HISM is a component of `this` ("ignore this actor so already-placed instances never fool the trace").

So Jonathan's screenshot (bare gray hill amid grassy field) = the hill surface itself hosting nothing, while the grass instances that rolled hill XYs sit buried at floor Z inside the mound. Layer ORDER was irrelevant pre-fix — no order could make the trace see a hill.

**Bonus finding (feeds TASK-249):** the "hills kept the stone donor material" note on the board is stale — binary string-scan of `Content/Meshes/SM_Hill_01/02/03.uasset` shows all three donors have **one material slot, named `HillGround`, already assigned `/Game/Materials/Instances/MI_BattlefieldGround`**. The bare/wrong look is that flat-ground material planar-projected over hill slopes (exactly the smear the tri-planar law forbids), not raw stone. M_HillGrass via OverrideMaterial remains the right fix.

## 2. DESIGN — what changed

### New `FScatterLayer` fields (ScatterConfig.h)
| Field | Type / default | Behavior |
|---|---|---|
| `bAllowOnHills` | bool = **false** | Per-layer opt-in. False = pre-task behavior byte-for-byte. True = layer placed in pass 2, ground resolve accepts elevated hill Z. |
| `MaxPlacementSlopeDeg` | float = **35** (Clamp 0–89, EditCondition bAllowOnHills) | Max hill-face slope; steeper candidates are REJECTED (re-rolled), never floor-grounded (that would re-bury them). 35° = the ≤30° climbable-face law + margin. |
| `OverrideMaterial` | TSoftObjectPtr\<UMaterialInterface\> = null | Null = donor materials (no-op). Set = SetMaterial on **ALL slots** of the layer's visual HISMs + paired proxy HISMs at component creation. SM_ assets never touched (lane-clean). |

Field name is `OverrideMaterial` per the CONVENTIONS "W1-PREP additions" law (the dispatch prompt's `UOverrideMaterial` read as a U-prefix slip — CONVENTIONS + board both say `OverrideMaterial`).

### Placement flow (BattlefieldScatter.cpp)
- **Two-pass generation** in `GenerateScatter`: pass 1 = all `bAllowOnHills=false` layers (hills themselves included — a hill never places on a hill), pass 2 = the opted-in layers. Guarantees hills exist before anything traces onto them.
- **Hill-surface registry:** pass-1 layers that are real-geometry blockers (`bBlocking && CollisionProxyMesh null && !bAllowOnHills`) register their HISMs in `HillSurfaceComponents` (raw-pointer secondary index over `ScatterComponents`-rooted comps — the `VisualToProxy` precedent; reset each generate).
- **`ResolveHillAwareGroundZ` (new):** for opted-in layers only. Floor trace stays `GroundZAt` (untouched); then **component-scoped** `LineTraceComponent` down-traces against exactly the registered hill HISMs (verified present + per-instance in UE 5.8: `InstancedStaticMeshComponent.h:564`, impl traces `InstancePhysicsBodies->LineTrace`). Highest above-floor hit wins; slope from `ImpactNormal` (`acos(Normal.Z)`, double-precision LWC-safe); within limit ⇒ elevated Z, over limit ⇒ candidate rejected. No world channel trace is involved, so the placement-ghost / projectile / camera channel contracts are untouched.
- **Mirror path:** the twin grounds independently at (−X, Y) with the same resolve; an over-slope face skips just the twin (same shape as the existing keep-clear twin-skip).
- **Seed order:** with no layer opted in, the pass split preserves DA order exactly — existing seeds reproduce unchanged. Opting a layer in moves it to pass 2 and intentionally changes the draw sequence (TASK-140 precedent; fixed OverrideSeed stays deterministic). Per-attempt draw order (X, Y, mesh, scale, yaw) is unchanged; the hill resolve draws no randoms.

### Laws explicitly UNCHANGED (traversability audit)
- **Keep-clear + corridor:** the footprint-inflated `IsInKeepClear` + field-edge clamp + radius-aware MinSpacing all run BEFORE the ground resolve, on the 2D candidate — identical for hill-placed instances.
- **Nav sanity for blockers on hills:** a hill-allowed blocking layer keeps `bCanEverAffectNavigation=true`, so it still participates in `ValidateTraversability` + `CullCorridorBlockers` (the cull is |Y|-based and Z-agnostic — it reaches elevated instances). The corridor cannot regress: the hill itself already passed its own footprint-inflated corridor test, and every blocker on it independently passes its own. Tree proxies on hills ride the elevated Z (ProxyZ derives from the same Z).
- **Safe default opt-ins:** grass/plants are non-blocking (zero nav interaction) — recommended first opt-ins; rocks/trees legal too (above). Documented in the `bAllowOnHills` field comment; **DA wiring is TASK-251/249's side — no DA edits here.**

### OverrideMaterial application points
- `ResolveComponentForMesh`: after `SetStaticMesh`, before registration. All slots (`GetNumMaterials`); **recorded choice:** all-slots ≡ slot 0 for the single-slot hill donors (evidence above) and never strands an extra slot on multi-slot donors used by other layers. Failed soft-resolve ⇒ warn + donor look (never a crash). Applied once at creation; the mesh-reuse path returns early, so a mesh SHARED by two layers keeps the first layer's override (one-HISM-per-mesh perf law — flagged as a DA config smell, not code-resolved).
- `ResolveProxyForVisual`: same application (CONVENTIONS law: "HISM + proxy components"); cosmetically moot (proxy is invisible) but keeps the pair uniform; silent no-op on failed resolve (visual path already warned).

## 3. QA scrutiny points
1. `LineTraceComponent` usage — engine API verified against the installed 5.8 source (per-instance bodies, QueryOnly OK); confirm you agree the signature/args match `InstancedStaticMeshComponent.h:564`.
2. The rocks-not-opted-in corner: a ROCKS layer left `bAllowOnHills=false` is pass-1 real geometry ⇒ becomes a hill SURFACE, so opted-in grass could sit on large flat rock tops (≤35°). Judged acceptable/organic; flag if you disagree.
3. Mirror restructure in `ScatterLayer` (`bPlaceMirror` bool replaced the single `if`) — check brace/logic equivalence for the non-hill path.
4. Aliasing: `ResolveHillAwareGroundZ(X, Y, GroundZ, …, GroundZ)` passes the same var as `FloorZ` (by value) and `OutZ` (by ref) — safe because FloorZ is a value param; confirm.
5. Double/float discipline: slope math in double, one explicit `static_cast<float>` on the LWC Z; shadow-law scan came back clean (no C4456/57/58/59 candidates).
6. No behavior change with an all-default DA (self-audited: pass split degenerates to original order; Z path identical when `bAllowOnHills` false; `OverrideMaterial` null short-circuits).

## For downstream
- **TASK-249 (art):** wire `OverrideMaterial` = `M_HillGrass` on the HILLS layer in DA_BattlefieldScatter after the pre-W1 bounce compiles this. Donor slot layout: single slot `HillGround` (all three SM_Hill donors).
- **TASK-251 (art):** hill scale range widening is orthogonal; elevated placement automatically follows any hill scale since the resolve traces live instance bodies.
- **build-master:** compile rides the pre-W1 bounce; no new modules, one new engine include (`Materials/MaterialInterface.h`).
