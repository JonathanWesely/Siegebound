# TASK-292c handoff — residual scatter-DF fix (gameplay-programmer)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · Code-only — NO compile, NO PIE, NO Git (build-master lane).
**Consumes:** `handoffs/TASK-292.md` (root-cause diagnosis) §6.3 fallback (b) + `handoffs/TASK-292b.md` (build-master hand-back with the exact residual).
**Outcome:** the two DF/GI flags are now cleared on BOTH scatter HISM paths in code → the scatter no longer feeds the distance-field/Lumen scene → the residual singular-matrix `InverseFast` NaN / `OriginX<=OriginMax` ensure source is removed. `ready-for-qa`.

## The fix — one file, two apply-sites, one flag pair
File: `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp`. On each scatter HISM at creation, set the flag pair:
- `bAffectDistanceFieldLighting = false`
- `bAffectDynamicIndirectLighting = false`

### Site 1 — VISUAL HISM: `ResolveComponentForMesh()` (~cpp:640)
Placed immediately AFTER the TASK-284 `SetCullDistances(...)` / `SetCastShadow(Layer.bCastShadows)` calls and BEFORE `Comp->RegisterComponent()`. This is the HISM that carries every scattered instance (rocks / hills / grass / tree-visuals), including the CastShadow-on hills that were the likely residual DF contributor at 10× coords.

### Site 2 — PROXY HISM: `ResolveProxyForVisual()` (~cpp:719)
Placed immediately AFTER the collision/nav block (`SetCollisionResponseToChannel(ECC_Pawn...)` / `bFillCollisionUnderneathForNavmesh` / `SetCanEverAffectNavigation(true)`) and BEFORE `Proxy->RegisterComponent()`. The proxy is `SetVisibility(false)` + `SetCastShadow(false)`, but it is still a `UPrimitiveComponent` that WOULD be inserted into the DF/Lumen scene and generate a mesh distance field — so it gets the same flag pair (blanket correctness + no needless DF cost).

## Pre-Register ordering — CONFIRMED (the ⚠ from the task)
Both sites set the flags BEFORE `RegisterComponent()`, matching exactly how TASK-284 wired `SetCullDistances` (pre-Register). This is the clean ordering: the render state does not exist until registration, so the flags are read when the component is first inserted into the DF/Lumen scene — no mark-render-dirty needed, no post-Register toggle. (For the record: `bAffectDynamicIndirectLighting` DOES have a `SetAffectDynamicIndirectLighting` setter that would `MarkRenderStateDirty`, but pre-Register there is nothing to dirty, so direct assignment is equivalent and correct; `bAffectDistanceFieldLighting` has NO setter at all, so direct assignment is the only in-code path — I used direct assignment for both to keep the pair symmetric.)

## API verification (so the compile is clean)
Both properties are public `uint8:1` UPROPERTY bitfields on `UPrimitiveComponent`, in the "Lighting flags" block alongside `CastShadow`:
- `UE_5.8/.../Components/PrimitiveComponent.h:556` — `bAffectDynamicIndirectLighting:1`
- `UE_5.8/.../Components/PrimitiveComponent.h:564` — `bAffectDistanceFieldLighting:1`

`UHierarchicalInstancedStaticMeshComponent` inherits from `UPrimitiveComponent`; the include `Components/HierarchicalInstancedStaticMeshComponent.h` was already present (cpp:6) and pulls in the base declaration. NO new include added. Direct assignment of `false` to a `uint8:1` bitfield is standard (implicit → 0). Engine comment note: `bAffectDistanceFieldLighting` "is only used if CastShadow is true" — so clearing it is only *active* on shadow-casting layers (hills), which is precisely where the residual DF participation came from; it is a harmless no-op on non-shadow layers. These are the SAME two properties TASK-292b cleared on the 26 vista/POI/gold dressing components via MCP `set_properties` — this task mirrors that treatment onto the scatter, in code.

## Scope — nothing else changed
Blanket-off in code per the spec (NOT a new `FScatterLayer` DF field — the scatter is decorative environment; a per-layer knob would be needless surface). Untouched: density / `InstanceCount`, cull bands (TASK-284 `CullStartDistance`/`CullEndDistance`), the grid-hash spacing (`FScatterSpacingGrid`), placement / seed-order, mines, traversability, collision/nav profile. The two edits are purely additive (two property sets per HISM path). `BattlefieldScatter.h` NOT touched (no signature/field change needed). No other file touched.

## What QA should scrutinize
- Both apply-sites are pre-`RegisterComponent` (verify site 2 in particular — it is set after the collision block but the `Proxy->RegisterComponent()` is a few lines below, still ahead of it).
- Property names / access: public bitfields on `UPrimitiveComponent`, direct assignment (no setter needed; DistanceField has none).
- Confirm no density/cull/grid/placement drift — diff should be exactly the two comment+flag-pair blocks.

## For build-master (compile + PIE + commit)
Build with the standard Build.bat. Then PIE-verify on L_Arena that BOTH the `OriginX <= OriginMax` ensure (`DoubleFloat.cpp:19`) AND the `InverseFast … non-invertible → NaN` (`Matrix.h:468`) + per-frame NaN spam are GONE — **across MULTIPLE seeds**, because the residual was seed-dependent (TASK-292b: gen-1 seed 1857084801 was already clean, gen-2 seed 1529263745 reproduced it). A single clean seed is NOT sufficient evidence; run several generates (Play Again re-rolls the seed with `bReRandomizeOnMatchReset=true`). Confirm traversability (Blue→Red + mine paths, 0 culls) unregressed. Editor restart recommended for a clean DF/Lumen rebuild (per TASK-292b, ensures self-suppress within a session). Then commit `BattlefieldScatter.cpp` + this handoff + board. No push.

## Files
- Changed: `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` (two flag-pair blocks), `.claude/pipeline/TASKBOARD.md` (TASK-292c entry → ready-for-qa), `.claude/pipeline/handoffs/TASK-292c.md` (this file).
- Evidence (read-only): UE_5.8 `PrimitiveComponent.h:556/564`.
- NO compile, NO PIE, NO Git touched by gameplay-programmer.
