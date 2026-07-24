# QA Report — TASK-284
Verdict: PASS

Scope reviewed (file-only, pre-compile): `ScatterConfig.h` (`FScatterLayer` cull fields) + `BattlefieldScatter.cpp` (grid-hash accelerator + cull/shadow apply at both HISM sites). `BattlefieldScatter.h` untouched (correct — grid is a `.cpp`-internal anon-namespace struct; cull fields live on the data struct). Branch `m7.6-arena10x`, M7.6 Phase 1.

## THE LOAD-BEARING CHECK — grid-hash placement EQUIVALENCE: SOUND

The refactor is a pure query accelerator over the per-layer instance-vs-instance MinSpacing scan. Every leg of the equivalence proof verified at the source:

1. **Cell size covers the search radius (the crux).** `SpacingGrid.Init(Layer.MinSpacing + 2·MaxLayerR, ...)` (cpp:376). Rejection distance between candidate (Rc) and placed point (Rp) is `MinSpacing + Rc + Rp`. Both radii are bounded by `MaxLayerR`:
   - override branch (cpp:360-363): every instance's `FootprintR` == `Layer.FootprintRadius` == `MaxLayerR` exactly (cpp:408-411 mirrors cpp:360-363), so Rc = Rp = MaxLayerR.
   - auto-derive branch (cpp:366-374): `MaxLayerR = maxHalfDiag × ScaleHi`, using the SAME `FVector2D(Bounds.BoxExtent.X,.Y).Size()` formula as per-instance `FootprintR` (cpp:414-415). Per-instance halfDiag ≤ maxHalfDiag (max over `Resolved`) and rolled `Scale ≤ ScaleHi`, so `FootprintR ≤ MaxLayerR`.
   Thus `MinDist = MinSpacing + Rc + Rp ≤ MinSpacing + 2·MaxLayerR = CellSize`. Rejection is strict (`DistSquared < MinDist²`), so any real conflict has `dist < MinDist ≤ CellSize` ⇒ `|Δx|,|Δy| < CellSize` ⇒ `|⌊a/s⌋−⌊b/s⌋| ≤ 1` per axis ⇒ the conflict is GUARANTEED inside the queried 3×3 block. No conflict can sit outside the block and be missed. The degenerate floor (`CellSize = max(…,1)`, cpp:77) is safe: when `MinSpacing+2·MaxLayerR < 1`, `MinDist ≤ that < 1 = CellSize`, so coverage still holds (and `MinDist>0` guard trivially matches the old scan when everything is zero).
2. **Same pairwise inequality.** `AnyTooClose` applies `MinDist = MinSpacing + CandRadius + Other.Value; MinDist > 0 && FVector2D::DistSquared(Other.Key, Candidate) < MinDist*MinDist` (cpp:114-116) — the identical radius-aware test the linear scan used, just restricted to the 3×3 block. It is a boolean "any within distance," so iteration order over buckets is irrelevant to the outcome.
3. **No FRandomStream draw added / removed / reordered.** The grid (`Init/CellCoord/CellKey/Add/AnyTooClose`) draws NOTHING from any stream — all pure. The candidate sequence is untouched: `X` (cpp:384) → `Y` via `SampleBiasedY` = FRand+FRand (cpp:385) → mesh index (cpp:397) → `Scale` (cpp:403) → `Yaw` (cpp:444). (The mesh/scale-before-tests ordering is the PRE-EXISTING TASK-140 change, not introduced here.) `PlacedPoints.Add ↔ SpacingGrid.Add` is 1:1: primary (cpp:460 AddInstance ↔ cpp:461 Add) and mirror twin (cpp:507 AddInstance ↔ cpp:508 Add). The mirror path spacing-checks nothing (keep-clear only, cpp:487) exactly as before, and every stored point has radius = `FootprintR ≤ MaxLayerR`, so the coverage invariant holds for the twins too. Fixed seed ⇒ identical accept/reject sequence ⇒ byte-identical layout.
4. **Keep-clear left untouched.** `RebuildKeepClearZones` / `IsInKeepClear` / `IsInKeepClearDiscs` (the ≤4-zone O(1) test) are unchanged; the grid targets ONLY the instance-vs-instance term. `PlacedPoints` is fully removed — grep confirms ZERO live references (only cited in explanatory comments).

Cell-key packing (`(uint32(CX)<<32)|uint32(CY)`, cpp:89-92) is a collision-free bijection over int32×int32 incl. negatives; `FloorToInt` gives a true floor for negative coords; field coords (±26000) are nowhere near int32 overflow. `TMap<uint64,…>` lookups are exact.

## CULL BANDS — correct

- 3 fields ADDED to `FScatterLayer` (confirmed none pre-existed from the TASK-249 OverrideMaterial pass): `CullStartDistance` int32=0 (h:222), `CullEndDistance` int32=0 (h:233), `bCastShadows` bool=true (h:247). `int32` matches `UInstancedStaticMeshComponent::SetCullDistances(int32,int32)` 1:1 — no narrowing.
- **Unpopulated DA is byte-identical to today.** `SetCullDistances(0,0)` = never culled (End==0 = no cull, == no call); `SetCastShadow(true)` = the implicit primitive default (== no call). Confirmed.
- Applied at BOTH HISM-creation sites, before `RegisterComponent`: visual `ResolveComponentForMesh` (cpp:636 `SetCullDistances` + cpp:637 `SetCastShadow(Layer.bCastShadows)`) and proxy `ResolveProxyForVisual` (cpp:711 `SetCullDistances`). Setting these pre-registration matches the file's established pattern (collision/material set the same way).
- `SetCullDistances` / `SetCastShadow` resolve on complete included types (`Components/HierarchicalInstancedStaticMeshComponent.h`, cpp:6; `SetCastShadow` already used pre-existingly at cpp:703). No new include needed. Complete-type include law satisfied.
- UE 5.8: both APIs current, non-deprecated. No removed/deprecated API anywhere in the diff.

## PROXY-SHADOW FLAGGED DECISION — ACCEPT

The invisible tree collision proxy keeps hardcoded `SetCastShadow(false)` (cpp:703), NOT driven by `Layer.bCastShadows`; the cull band IS mirrored to the proxy (cpp:711, harmless — proxy is `SetVisibility(false)`, cull is moot but keeps the pair uniform for debug un-hide). The programmer's reading is correct: a shadow-casting invisible trunk cylinder would be a floating-shadow artifact, and a tree's actual shadow is controlled on the VISIBLE HISM via `ResolveComponentForMesh` (cpp:637). The spec's "apply `SetCastShadow` in the proxy path" is satisfied to the letter — `SetCastShadow` IS called there — while correctly NOT wiring the layer flag to it. Wiring the flag would only ever toggle a shadow on a mesh that renders nothing. Ruling: ACCEPT as-is.

## Findings
- [NIT] BattlefieldScatter.cpp:344-373 — the cell-coverage invariant (`FootprintR ≤ MaxLayerR`) assumes non-negative scale. A negative `ScaleRange` (no `ClampMin` on the struct field) would flip the bound and could let the grid miss a conflict the old exhaustive scan caught — but negative scatter scale is nonsensical/inverts meshes, never appears in `DA_BattlefieldScatter` (Phase-3 data, out of this task's scope), and defaults are (1,1). Equivalence HOLDS for all valid/in-scope data. Optional belt-and-suspenders: clamp `ScaleLo`/`ScaleHi ≥ 0` or add `ClampMin=0` to `ScaleRange`. Not blocking.

## Notes for build-master (if PASS)
- Static equivalence is proven sound; the RUNTIME byte-identical-seed proof is TASK-286's job (set a fixed `OverrideSeed`, diff the `LogSiegeTerrain` per-layer "placed N instances" lines pre/post-refactor — they must match exactly). The per-layer log line (cpp:527-530) is the diff surface.
- No density / DA / rule change in this task: `DA_BattlefieldScatter` (density is Phase 3 / TASK-287) and `L_Arena` MUST show UNTOUCHED in `git diff --stat` — only `BattlefieldScatter.cpp` + `ScatterConfig.h` moved here. `SummonedUnit` (TASK-285) is disjoint.
- Watch at compile: none expected — includes complete, no shadow of inherited reflected members (new locals `MaxLayerR/MaxHalfDiag/SpacingGrid/Bucket/CX/CY/DX/DY/CandRadius/Other`, and the standalone-struct member `MinSpacing`, shadow nothing on AActor/UObject).
```
```
