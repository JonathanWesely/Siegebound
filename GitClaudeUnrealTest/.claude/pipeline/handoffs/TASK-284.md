# TASK-284 handoff — Scatter cull bands + grid-hash MinSpacing accelerator

**Assignee:** gameplay-programmer · **Branch:** `m7.6-arena10x` · **Status:** ready-for-qa
**Scope:** code-only, file-only (no compile, no Git). Owns `BattlefieldScatter.{h,cpp}` + `ScatterConfig.h`.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` — added the 3 cull fields to `FScatterLayer`.
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` — grid-hash accelerator + cull/shadow apply at both HISM-creation sites.
- `BattlefieldScatter.h` — **NOT touched.** The grid-hash helper is a `.cpp`-internal anonymous-namespace struct (no new actor member/method needed), and the cull fields live on the data struct, so no header change was required. In scope (owned file, left clean).

No other files changed. `DA_BattlefieldScatter` and `L_Arena.umap` (branch-owned, Phase 3 territory) are UNTOUCHED — this task only wires the code path; densities/bands/flags are Phase-3 DATA.

---

## (1) Cull bands + shadow flags

### Fields that EXISTED vs ADDED
Confirmed against the current `FScatterLayer`: the W1-PREP `OverrideMaterial` pass (TASK-249/250) added `OverrideMaterial`, `bAllowOnHills`, `MaxPlacementSlopeDeg` — but **NONE** of the three cull fields. So **all three were ADDED** by this task:

| Field | Type | Default | Rationale |
|---|---|---|---|
| `CullStartDistance` | `int32` (ClampMin 0) | `0` | Near edge of the fade band → `InstanceStartCullDistance`. |
| `CullEndDistance` | `int32` (ClampMin 0) | `0` | `0` = **never culled** (`InstanceEndCullDistance`); the safe no-change fallback. |
| `bCastShadows` | `bool` | `true` | `true` = hills/obstacles ON (silhouette), mirrors `bBlocking`'s obstacle-default pattern; DA sets FALSE on grass/plants at Phase 3. |

`int32` (not float) matches the `UInstancedStaticMeshComponent::SetCullDistances(int32, int32)` signature 1:1 — no narrowing. CONVENTIONS says "uu"; integer uu is exact for cull distances.

**`bCastShadows` single-default reconciliation:** a USTRUCT field has one literal default, but the spec's "true for hills / false for grass/plants" is a per-layer intent. Resolved exactly like the existing `bBlocking` field (default `true` = obstacle case; DA sets `false` for grass) — so the struct default is `true`, and Phase 3 sets grass/plants `false` in `DA_BattlefieldScatter`. An **unpopulated DA is byte-for-byte unchanged** vs today: `SetCullDistances(0,0)` = never culled (same as no call), and `SetCastShadow(true)` = the implicit component default (same as no call).

### The TWO apply-sites (both HISM/component creation points)
1. **`ResolveComponentForMesh()`** (visual HISM) — added, before `RegisterComponent()`, with the rest of the render profile:
   ```cpp
   Comp->SetCullDistances(FMath::Max(Layer.CullStartDistance, 0), FMath::Max(Layer.CullEndDistance, 0));
   Comp->SetCastShadow(Layer.bCastShadows);
   ```
2. **`ResolveProxyForVisual()`** (tree collision-proxy HISM) — added `SetCullDistances` (same band, for a uniform visual/proxy pair state; functionally moot since the proxy is `SetVisibility(false)`, but harmless and consistent). The proxy's **`SetCastShadow(false)` STAYS as the proxy-contract invariant** — it is NOT driven by `Layer.bCastShadows`.

**⚠ QA please scrutinize this one decision:** the spec says "apply `SetCastShadow` in … the tree collision-proxy path." The proxy path *does* call `SetCastShadow` — hardcoded `false`, which is correct and must stay: an invisible trunk-cylinder proxy casting a shadow would be a floating-shadow bug. `Layer.bCastShadows` drives the **VISIBLE** tree/rock/hill HISM via `ResolveComponentForMesh` (that's where a tree's shadow is actually controlled). I judged applying the layer flag to the invisible proxy to be wrong; flagging it explicitly rather than silently. If QA/Jonathan wants the proxy to literally read the flag, it's a one-line change — but it would only ever make an invisible cylinder cast (or not cast) a shadow it can't render.

**Include check:** `SetCullDistances` is on `UInstancedStaticMeshComponent` (base of HISM) — the file includes `Components/HierarchicalInstancedStaticMeshComponent.h` (complete type). `SetCastShadow` is on `UPrimitiveComponent` and was already called in this file (`Proxy->SetCastShadow(false)` pre-existed). No new includes needed.

---

## (2) Grid-hash spacing accelerator

### What was O(n²) — and what was NOT
The genuine quadratic is the **per-layer radius-aware MinSpacing** scan in `ScatterLayer`: each candidate scanned **every** already-placed instance (`PlacedPoints`) → O(n²) as `InstanceCount` grows (the Phase-3 ≈4.9× fill is exactly this blowup).

The **keep-clear proximity check is NOT O(n²)** and was left untouched: `IsInKeepClear` / `IsInKeepClearDiscs` test against `KeepClearZones` = a **fixed ≤4-element set** (≤2 castles + ≥1 PlayerStart), rebuilt from live actors, independent of instance count → O(1) per candidate already. Grid-hashing a 4-element set would add complexity for zero gain and risk changing a keep-clear RULE (forbidden by the spec). So the grid targets exactly the instance-vs-instance term. (The mines pass's `PrimaryPoints` spacing loop is O(mines²), mines ≤ 3/side, and carries its own strict draw-order contract — deliberately untouched.)

### Design
Internal anonymous-namespace struct `FScatterSpacingGrid` in `BattlefieldScatter.cpp` (non-UPROPERTY, name chosen at implementation per the spec). Uniform 2D spatial hash:
- **Cell size** = `MinSpacing + 2 × MaxLayerR`, floored at 1 uu (degenerate zero-radius/zero-spacing guard against div-by-zero — in that case nothing is ever rejected anyway, so the result matches trivially).
- `MaxLayerR` = the layer's **maximum possible footprint radius**: explicit `FootprintRadius` override ⇒ that exact value (all instances share it); else `maxMeshHalfDiag × ScaleHi` using the **same** `GetBounds()` XY-half-diagonal formula the per-instance `FootprintR` uses.
- Key = two `int32` cell coords packed into a `uint64` via their `uint32` bit patterns (collision-free across the full int32×int32 range, negatives included). `TMap<uint64, TArray<TPair<FVector2D,float>>>`.
- `Add(center, radius)` on every placement (primary + mirror twin) — 1:1 replacement of the old `PlacedPoints.Add`. `PlacedPoints` is fully removed (it was read only by the spacing scan).
- `AnyTooClose(candidate, Rc)` scans only the **3×3 cell block** around the candidate and applies the **identical** pairwise test.

### Why placement is UNCHANGED (equivalence / determinism argument)
The rejection distance between candidate (radius `Rc`) and placed point (radius `Rp`) is `MinSpacing + Rc + Rp`. Both `Rc, Rp ≤ MaxLayerR`, so any conflicting point is strictly closer than `MinSpacing + 2·MaxLayerR = CellSize`. A point within Euclidean distance `< CellSize` differs by at most **one cell per axis** (|a−b| < s ⇒ |⌊a/s⌋−⌊b/s⌋| ≤ 1) — so it is guaranteed to lie in the searched 3×3 block. `AnyTooClose` then applies the **byte-identical inequality** (`MinDist = MinSpacing + Rc + Rp`; `MinDist > 0 && DistSquared < MinDist²`), so it returns `true` **iff** the old full scan would have. The check is a boolean "any within distance," so iteration order is irrelevant to the outcome.

Critically for the seed-order law: **the grid draws nothing from any `FRandomStream`.** The candidate generation sequence (X, Y, mesh index, scale, yaw) is 100% unchanged and in the same order; the mirror path still spacing-checks nothing (only keep-clear), exactly as before; the grid `Add` calls mirror the old `PlacedPoints.Add` calls 1:1. Therefore **a fixed seed produces byte-identical placement** vs the pre-refactor build. This is a pure query accelerator, not a rule change.

**Empirical proof note (honest):** this task is file-only (no compile/run per the dispatch). The byte-identical-for-one-seed *runtime* proof is the equivalence argument above plus the deterministic-log comparison in the integration step — TASK-286's PIE sanity explicitly checks "a fixed seed produces the SAME scatter layout as before the refactor." Set a fixed `OverrideSeed` and diff the `LogSiegeTerrain` per-layer "placed N instances" lines before/after; they must be identical.

---

## QA scan checklist (self-reviewed, all clear)
- **Shadow-of-inherited-reflected-member law:** no new local/param/loop var shadows an AActor/UObject inherited reflected UPROPERTY. `FScatterSpacingGrid::MinSpacing` is a member of a standalone struct (no inheritance) — not a shadow. `MaxLayerR`, `MaxHalfDiag`, `SpacingGrid`, `Bucket`, `CX/CY/DX/DY`, `CandRadius` are plain locals. `bCastShadows` is a new data field, not a shadow.
- **Complete-type include law:** `SetCullDistances`/`SetCastShadow` both resolve on already-included complete types (HISM header + pre-existing `SetCastShadow` usage). No forward-declared-pointer deref added.
- **Null-safety:** unchanged paths; grid handles empty layers (no placements → empty grid → `AnyTooClose` returns false). Degenerate cell size floored at 1.
- **No density / DA / rule change:** densities, keep-clear/corridor rules, mine pass, `L_Arena`, `SummonedUnit` all untouched.
