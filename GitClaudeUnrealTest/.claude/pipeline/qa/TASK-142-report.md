# QA Report — TASK-142 (pre-compile review of TASK-140 + TASK-141, M6.6 climbable terrain)

**Reviewer:** qa-reviewer
**Date:** 2026-07-14
**Scope:** files-only review (no compile, no engine, no Git — build-master compiles at TASK-143/145)

## Verdicts
- **TASK-140 (scatter C++ — ScatterConfig.h / BattlefieldScatter.h / BattlefieldScatter.cpp): PASS**
- **TASK-141 (hero movement — HeroCharacter.h / HeroCharacter.cpp): PASS**

BLOCKERS: 0 · WARN: 0 · NIT: 2

---

## Mandated scans (each reported pass/fail with file:line)

### 1. Shadow law (C4457/58/59) — PASS
- **TASK-141:** `HeroMaxStepHeight` (HeroCharacter.h:374), `HeroWalkableFloorAngle` (:378), `HeroJumpZVelocity` (:382) are `Hero`-prefixed as mandated. None collides with any inherited reflected member of `AGitClaudeUnrealTestCharacter`/`ACharacter`/`APawn`/`AActor`; the identically-named fields (`MaxStepHeight`/`WalkableFloorAngle`/`JumpZVelocity`) live on `UCharacterMovementComponent` (a component, not a base), so no shadow. Grep across `Source/` confirms the three names + `ApplyTerrainMovementTuning` appear ONLY in the two HeroCharacter files.
- **TASK-140:** new param `InstanceRadius` (BattlefieldScatter.h:161 / .cpp:585) and new locals (`FootprintR`, `MinDist`, `Other`, `Candidate`, `MirrorPoint`, `ProxyScaleVec`, `ProxyZ`, `bUsesProxy`, `MeshBounds`, `Proxy`, `Visual`, `Existing`, `Pair`) shadow no inherited reflected member (no `Owner`/`Instigator`/`Controller`/`Slot`/`Team` etc. on `AActor`). New struct fields `FootprintRadius`/`CollisionProxyMesh`/`CollisionProxyScale`/`CollisionProxyZOffset` (ScatterConfig.h) and member `VisualToProxy` (BattlefieldScatter.h:134) are distinct from any base/component field. Note: `ResolveCastleLocation(ETeamId Team)` uses a param named `Team`, but `ASiegeBattlefieldScatter` (an AActor) has no `Team` member — no shadow (contrast `AHeroCharacter`, which does have `Team`, but that method is on the scatter class).

### 2. Complete-type include law — PASS
- **BattlefieldScatter.cpp:** `Components/HierarchicalInstancedStaticMeshComponent.h` (:6) covers every HISM/UPrimitiveComponent method used (AddInstance, RemoveInstances, GetInstanceCount, GetInstanceTransform, ClearInstances, Set/GetStaticMesh, SetCollision*, SetVisibility, SetCastShadow, SetCanEverAffectNavigation, CanEverAffectNavigation, `bFillCollisionUnderneathForNavmesh`). `Engine/StaticMesh.h` (:9) covers `Mesh->GetBounds()` (:251, returns `FBoxSphereBounds`). `NavigationPath.h` (:13) covers `Path->IsValid()/IsPartial()`; `NavigationSystem.h` (:14) covers `UNavigationSystemV1`. `ScatterConfig.h` (:18) gives the complete `FScatterLayer`/`USiegeScatterConfig` (header only forward-declared `FScatterLayer`). `Castle.h`/`GoldNode.h` give the dereferenced actors. `ECC_Visibility`/`ECC_Camera`/`ECC_Pawn`/`ECC_WorldStatic`/`ECR_*` resolve via EngineTypes (already-used `ECC_Pawn`/`ECC_WorldStatic` prove the transitive include). No forward-declared-only dereference found.
- **HeroCharacter.cpp:** `GameFramework/CharacterMovementComponent.h` present at :13 (pre-existing) — the `MaxStepHeight`/`JumpZVelocity` field writes and `SetWalkableFloorAngle()` call in `ApplyTerrainMovementTuning` (:685-690) dereference the complete `UCharacterMovementComponent`. No new include needed.

### 3. Every `IsInKeepClear` caller updated for the new signature — PASS
Grep found exactly two callers, both pass the intended real radius `FootprintR` (not defaulting to 0):
- `BattlefieldScatter.cpp:265` — main path: `IsInKeepClear(Candidate, FootprintR)`
- `BattlefieldScatter.cpp:318` — mirror path: `IsInKeepClear(MirrorPoint, FootprintR)`
Declaration default `= 0.f` (BattlefieldScatter.h:161) keeps the signature safe for any future call. No other call sites exist.

### 4. Visual/proxy cull-desync (THE ONE THAT BITES) — PASS (traced explicitly)
The pairing and lockstep culling are **correct**; no orphan is produced.
- **One proxy per visual HISM:** `ResolveProxyForVisual` (:419) keys on `VisualComp` in `VisualToProxy` and reuses an existing proxy (:428), else creates exactly one (:442) and stores it (:471). 1:1 pairing per unique tree mesh's visual HISM.
- **Parallel indices at build time:** in `ScatterLayer`, the visual instance is added (:291) and, when `bUsesProxy`, the paired proxy instance is added to the SAME proxy in lockstep (:303-309); the mirror path mirrors both (:323 visual, :335 proxy). Every proxy instance is added alongside its visual, so visual[N] ↔ proxy[N] for each pair. (Graceful-degradation path: if `CollisionProxyMesh` fails to resolve, `ResolveProxyForVisual` returns nullptr consistently → no proxy HISM exists at all, the visual HISM carries NoCollision+no-nav and is simply skipped in the cull; degraded to "no tree collider," never a desync/crash — matches the spec.)
- **Lockstep removal:** `CullCorridorBlockers` (:697) iterates only nav-relevant comps (`CanEverAffectNavigation()`, :706), so proxy-layer VISUAL HISMs (nav OFF) and grass are skipped as primaries. For a tree PROXY it computes `ToRemove` from the proxy's own transforms (proxy X/Y == visual X/Y by construction, :307), then removes the SAME index array from the paired visual via `FindVisualForProxy` (:732-735) BEFORE removing from the proxy (:736). Because the identical index array is applied to both HISMs, their re-indexing stays parallel across repeated widening-band culls. The visual is never processed as a primary (nav OFF), so there is no double-remove and no order dependence.
- **ClearScatter reaches both:** both visual and proxy HISMs are registered in `ScatterComponents` (:415 visual, :470 proxy); `ClearScatter` (:150) iterates that array and calls `ClearInstances` on each. Components persist for reuse; `VisualToProxy` stays valid across re-scatter. GC-safe: `ScatterComponents` is `UPROPERTY(Transient)` rooting both; `VisualToProxy` is a secondary raw-pointer index whose entries all outlive it.

### 5. Template law — PASS
TASK-141 touched only `HeroCharacter.h`/`.cpp`. Grep confirms the three tunables + helper exist nowhere else. `GitClaudeUnrealTestCharacter.cpp:31` still reads `GetCharacterMovement()->JumpZVelocity = 500.f;` (the template's own pre-existing base value — unmodified). `AHeroCharacter` correctly overrides all three onto the subclass; base ctor runs first (500), then the derived ctor's `ApplyTerrainMovementTuning()` raises to 600 — derived wins.

### 6. Loop reorder / seed-change determinism — CONFIRMED INTENTIONAL (not flagged)
Seed selection is unchanged (BattlefieldScatter.cpp:109-125): `OverrideSeed>0` still forces a fixed seed; the reorder only moves the mesh+scale draws (:234, :240) earlier in the same `FRandomStream`, which alters the draw sequence but keeps a fixed `OverrideSeed` fully deterministic. Radius derivation (:244-253) happens BEFORE the field-edge clamp (:257), keep-clear test (:265), and spacing test (:272-281), exactly as intended. Loudly documented in-code (:225-233) and in the handoff. Not a regression.

### 7. Standard pass — PASS
- **Deprecated/removed UE 5.8 APIs:** none. `SetWalkableFloorAngle`, `UNavigationSystemV1::GetCurrent`, `FindPathToLocationSynchronously`, `UNavigationPath::IsValid/IsPartial`, `AddInstance(FTransform,bWorldSpace)`, `RemoveInstances(TArray<int32>)`, `GetInstanceTransform`, `SetCanEverAffectNavigation`, `SetCollisionResponseToChannel(ECC_Camera/…)`, `bFillCollisionUnderneathForNavmesh` are all current.
- **Null/validity checks:** `Mesh` always non-null (drawn from `Resolved[]`, which only holds successfully `LoadSynchronous`'d meshes); `Comp` null-checked before `GetBounds` (:236); proxy mesh load null-checked with graceful log (:433-440); `GetWorld()` guarded throughout; `GetCharacterMovement()` null-checked in `ApplyTerrainMovementTuning` (:685); castle/node/PlayerStart iterators use `IsValid`. `Mesh->GetBounds()` is on an already-resolved hard `UStaticMesh*`, not an unloaded soft ptr.
- **Radius math:** keep-clear inflation `(sqrt(RadiusSq)+R)^2` correct (:599-600); spacing `(MinSpacing+Ri+Rj)^2` correct (:275-276); field-edge clamp `|X|+R>HalfX || |Y|+R>HalfY` correct (:257); corridor `|Y| <= CorridorHalfWidthCached + R` correct (:591). Mirror twin reuses the same `FootprintR` and inherits the edge clamp via equal `|X|,|Y|` — correct.

---

## Findings
- [NIT] BattlefieldScatter.cpp:599 — `IsInKeepClear` recomputes `FMath::Sqrt(Zone.RadiusSq)` per zone per candidate inside the rejection-sampling loop. Bounded (only `bBlocking` layers call it, and those have low counts; grass with `bBlocking=false` never enters), so not a real perf smell — could pre-store the un-squared radius on `FKeepClearZone` if ever hot. No action required.
- [NIT] BattlefieldScatter.cpp:349-363 / handoff "Known invariant" — `ResolveComponentForMesh` matches a HISM by `GetStaticMesh() == Mesh`, and proxy HISMs share `ScatterComponents`. Safe only while no layer lists a mesh that is also another layer's `CollisionProxyMesh`. This is a CONFIG constraint (build-master TASK-144: do not scatter the engine `Cylinder` as a visual mesh), not a code defect. Carried forward as a note, not a blocker.

## Notes for build-master (TASK-143/144/145)
- Both tasks are cleared to compile. Real compile/link + UHT still owed at TASK-143/145 (this is a source review; include-completeness and shadow were verified by inspection + grep, but the warnings-as-errors build is the final gate).
- TASK-144 config constraint (from the NIT above): the Trees layer's `CollisionProxyMesh` = `/Engine/BasicShapes/Cylinder`; ensure NO layer lists that Cylinder in its visual `Meshes` array, or the proxy/visual HISM reuse-by-mesh would collide.
- `CollisionProxyZOffset` IS multiplied by the instance's uniform scale in code (:306, :333) — the ≈+850 derivation (½ of a 100-unit cylinder × 17) is correct at scale 1 and stays grounded across the scale range.
- SEED-REORDER is expected: a given `OverrideSeed` now maps to a NEW (still-valid) layout vs pre-M6.6. Deterministic — do not treat a changed layout as a bug.
- Grass now gets radius-aware spacing + edge clamp; if the wider field reads thin, lower grass `MinSpacing` (the footprint radius adds to it).
