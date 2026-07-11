# QA Report — TASK-134 (+ TASK-133)
Verdict: **PASS** (both tasks) — 0 BLOCKER, 2 WARN, 3 NIT

Combined pre-compile review of the M6.5 battlefield-scatter code wave.
- TASK-134 (primary): `ScatterConfig.h/.cpp`, `BattlefieldScatter.h/.cpp`, `SiegeGameMode.cpp/.h` edits.
- TASK-133 (quick): `SiegeBotController.h` constants, `GoldNode.h` comments.

**Scope limit of this review (STATED, per spec):** this verdict certifies the CODE logic is correct for UE 5.8. It does NOT and CANNOT certify runtime behavior — that obstacles actually carve the navmesh, that units visibly route around them, and that no live match strands a unit. That requires (a) `L_Arena`'s `RecastNavMesh RuntimeGeneration = Dynamic` (build-master TASK-136 — HARD DEP; without it the blocking/nav half is inert) and (b) the PIE + two-match traversability verification (TASK-136/137). A PASS here means the guarantee LOGIC is sound, not that the battlefield is proven navigable on screen.

---

## Traversability guarantee — rigorous assessment: DOES IT HOLD?

**Yes, the deterministic layer genuinely holds, with one bounded caveat (WARN-1).**

**Layer 1 — reserved corridor (the real guarantee).** `IsInKeepClear()` returns true for `FMath::Abs(Point2D.Y) <= CorridorHalfWidthCached` (400) at ALL X, and every blocking candidate is rejected when `Layer.bBlocking && IsInKeepClear(Candidate)` (BattlefieldScatter.cpp:214). This is applied uniformly to EVERY blocking layer (trees/rocks/hills — `bBlocking` default TRUE) at EVERY placement attempt, with no X-gap. Grass is correctly exempt (`bBlocking==false` skips the check). So no blocking instance ORIGIN ever lands within |Y|<=400. The castles (±8000,0), gold nodes (±7200,0), and PlayerStart (-6800,0) all sit ON the centerline, so the corridor itself carries the castle↔castle lane, the castle↔node miner lanes, and the spawn exit — all provably clear of blocker origins end-to-end. The "ring around a spawn / node just outside its keep-clear radius" scenario CANNOT seal the actor in: any blocker on the corridor near it would have |Y|<=400 and is excluded, so the corridor is always an open exit through the ring. This layer does not depend on any async nav state. Sound.

**Keep-clear discs.** `RebuildKeepClearZones()` builds discs from the LIVE `ACastle`/`AGoldNode`/`APlayerStart` actors (IsValid-guarded `TActorIterator`), with correct CONVENTIONS fallbacks per team only when the live actor is missing (Blue -8000 / Red +8000; nodes ∓7200; start -6800). Radii (castle 900, node 500, start 700) are additive on top of the corridor. Note the discs are stored as RadiusSq and compared with `DistSquared` — correct. At the new ±8000 scale the castle r900 covers the ~810-unit footprint the handoff cites; final footprint-vs-radius sizing is a DATA tuning call at TASK-137. Correct with correct fallbacks.

**Layer 2 — confirmatory nav check.** Deferred by timer `NavSettleDelay` (0.75s) so the async Dynamic nav can carve. Endpoints are inset toward center by `CastleQueryInset` (1200): Blue -8000→-6800, Red +8000→+6800 (sign math verified: `X -= Sign(X)*Inset` moves toward 0). Both inset points land on clear corridor ground off the castle's own nav hole — valid, projectable endpoints. `bReachable = Path && Path->IsValid() && !Path->IsPartial()` is the correct pass predicate. Null-nav path: warns ONCE and returns, relying on the corridor — no crash, no false failure. **Convergence is provable:** on failure it culls blocking instances with origin |Y|<=Band where Band = corridor + attempt×250 (650/900/1150/1400/1650), monotonically widening, capped at `MaxReachabilityAttempts`=5, then stops with an Error log. Finite, widening, ≤5. Because the final cull clears blockers to |Y|<=1650, the straight lane is genuinely obstacle-free even if nav never reports — it never leaves a match unwinnable. The cull filters to nav-affecting comps via `CanEverAffectNavigation()`, correctly ignoring grass. Sound.

**The one real gap (WARN-1, bounded, non-blocking):** the corridor test is measured to instance ORIGINS, not mesh footprints. A large-footprint blocking mesh (e.g. a scaled-up hill) whose origin sits just outside |Y|=400 could lean its collision toward Y=0. To actually block the exact centerline it would need a footprint half-width > 400 (an 800+ diameter obstacle) — implausible for curated mobile/low-poly scatter, and even then a single intrusion cannot wall off a 6400-wide-per-side field; a genuine wall-off is exactly what Layer 2's cull removes. So this is not a hard-failure risk, but it should be closed by DATA at TASK-137 (keep max-scale × mesh bounds < CorridorHalfWidth, or widen the corridor). Flagged for build-master/art-director, not a code blocker.

---

## Findings

### TASK-134
- [WARN] BattlefieldScatter.cpp:431 (`IsInKeepClear`) / ScatterConfig.h:173 — Corridor exclusion is ORIGIN-based, not footprint-based. A blocking mesh with footprint half-width > CorridorHalfWidth (400) placed just outside the band could lean into the Y≈0 lane. Bounded (needs an extreme footprint; wide field; caught by the nav cull) — close via DataAsset tuning at TASK-137 (obstacle max footprint < corridor) OR subtract a footprint margin. Not a code blocker.
- [WARN] BattlefieldScatter.cpp:277 (`ResolveComponentForMesh`) — HISM reuse is keyed on mesh ONLY, ignoring `Layer.bBlocking`. If the SAME UStaticMesh is ever listed in both a blocking (tree/rock/hill) and non-blocking (grass) layer, both share ONE HISM with whichever collision/nav profile was created first. Realistically won't happen (grass ≠ obstacle meshes), but TASK-137 must avoid cross-layer mesh reuse in DA_BattlefieldScatter, or the collision/nav on one set will be wrong.
- [NIT] BattlefieldScatter.cpp:498 (`ValidateTraversability`) — the confirmatory query uses `FindPathToLocationSynchronously` with the DEFAULT nav agent, which may differ from the units' actual agent radius. Fine as a belt-and-suspenders check (corridor is the real guarantee), but the confirmation is only as representative as the default agent. No change required.
- [NIT] BattlefieldScatter.cpp:534 (`CullCorridorBlockers`) — after the final (5th) attempt the code force-clears and logs Error but does NOT run a last confirming path query, so a truly-clear lane after the last cull is logged as an Error rather than a CONFIRMED. Cosmetic/log-only; the lane is clear regardless.
- [NIT] ScatterConfig.h:8 — `ScatterConfig.generated.h` is included at line 8 with a forward-decl (`class UStaticMesh;`) after it. Valid (generated.h only needs to be the last #include, decls after are fine), but conventional style puts the forward decl above the generated include. No functional issue.

### TASK-133
- No findings. Constants, comments, and the "no Blue fallback" invariant all correct (see confirmation below).

---

## Mandatory scans (both tasks)

**Deprecated UE 5.8 APIs — NONE.** `UNavigationSystemV1::FindPathToLocationSynchronously`/`GetCurrent`, `UNavigationPath::IsValid`/`IsPartial`, HISM `AddInstance(FTransform,bWorldSpace)`/`RemoveInstances(TArray<int32>)`/`GetInstanceTransform`/`GetInstanceCount`/`ClearInstances`/`GetStaticMesh`, `SetCanEverAffectNavigation`/`CanEverAffectNavigation`, `LineTraceSingleByChannel`, `TActorIterator`, `GetTimerManager().SetTimer/ClearTimer`, `FMath::Sign/RandRange/Fmod` — all current, non-deprecated. `RemoveInstances(const TArray<int32>&)` sorts descending internally, so passing ascending indices (the code builds `ToRemove` 0..Count) is SAFE — no index-shift bug.

**Inherited-reflected-member shadow scan — CLEAN.** `ResolveCastleLocation(ETeamId Team)` — `Team` is a param on `ASiegeBattlefieldScatter` (an AActor); AActor has NO inherited reflected `Team` member (that lives on ACastle/AGoldNode, unrelated classes) → no C4458. All loop vars (`It`, `L`, `Comp`, `Zone`, `Idx`, `Existing`) are block-scoped; the three `TActorIterator It` loops in `RebuildKeepClearZones` are in disjoint scopes. New step-7 `if (UWorld* World = GetWorld())` in `SiegeGameMode::PlayAgain` (cpp:673) — verified PlayAgain (cpp:494–684) declares NO other `World` local → no C4456 local-shadow.

**Complete-type-include scan — CLEAN.** BattlefieldScatter.cpp includes every dereferenced type: HISM (`Components/HierarchicalInstancedStaticMeshComponent.h`), `SceneComponent.h`, `Engine/StaticMesh.h`, `Engine/World.h`, `Engine/HitResult.h`, `CollisionQueryParams.h`, `EngineUtils.h` (TActorIterator), `GameFramework/PlayerStart.h`, `NavigationPath.h` (IsValid/IsPartial), `NavigationSystem.h` (UNavigationSystemV1), `TimerManager.h`, `Siegebound/Castle.h` (GetTeamId), `Siegebound/GoldNode.h` (GetTeam), `Siegebound/ScatterConfig.h`. `SiegeGameMode.cpp` adds `#include "Siegebound/BattlefieldScatter.h"` (cpp:12) and already includes `EngineUtils.h` (pre-existing TActorIterator usage). Verified `ACastle::GetTeamId()` and `AGoldNode::GetTeam()` exist and are public. `NavigationSystem` + `AIModule` confirmed already in Build.cs by the handoff. `.generated.h` is the last include in both new headers.

**Null-safety on config/soft refs — CLEAN.** No config ⇒ warn-once + return no-op (cpp:80). Empty/zero-count/no-mesh layer ⇒ early return (cpp:153). Unresolvable soft mesh ⇒ skipped + logged, never crash (cpp:167–184). No world / no nav ⇒ guarded returns. `ScatterComponents` is `UPROPERTY(Transient) TArray<TObjectPtr<...>>` — GC-safe for the runtime-created HISMs; `ScatterConfig` is `TObjectPtr` UPROPERTY. Actor tick disabled; no per-tick work. Runtime component creation pattern (NewObject(this) → SetMobility/SetupAttachment/nav BEFORE RegisterComponent) is correct.

---

## HISM collision / nav wiring — CORRECT
- Blocking layers (cpp:305–320): `QueryOnly`, ObjectType `ECC_WorldStatic`, `ResponseToAllChannels(ECR_Ignore)` then `Block ECC_Pawn` only, `SetCanEverAffectNavigation(true)` — all set BEFORE `RegisterComponent`. Matches spec (Pawn-block only; carve nav; projectiles pass cosmetically = accepted M6.5 gap).
- Grass (cpp:323–326): `NoCollision`, `SetCanEverAffectNavigation(false)`.
- One HISM per unique mesh (cpp:284–291 dedup on `GetStaticMesh()==Mesh`); components reused across re-scatter, only instances cleared. Perf law satisfied.

## Determinism, seed, mirror, reset — CORRECT
- Layout driven by `FRandomStream(Seed)`; all draws (X, biased-Y ×2, mesh index, scale, yaw) route through the stream → repeatable-per-seed, varied-per-match. Seed logged one grep-able line on `LogSiegeTerrain` (cpp:122).
- Seed policy (cpp:99–113): OverrideSeed>0 fixed; else re-use LastSeed only when `bReRandomizeOnMatchReset==false && bHasSeed`; else fresh `RandRange(1, MAX_int32-1)`. BeginPlay → fresh; Play Again (default re-randomize=true) → fresh; re-randomize=false → repeats. Correct.
- `bMirrorSymmetric` default FALSE (ScatterConfig.h:142); `bReRandomizeOnMatchReset` default TRUE (BattlefieldScatter.h:83). Mirror twin at (-X, Y) is itself keep-clear-checked for blocking layers (cpp:258).
- PlayAgain step-7 hook (cpp:668–683) is a NEW step at the END, null-safe `TActorIterator<ASiegeBattlefieldScatter>` (no actor = clean no-op); existing reset steps 1–6 untouched. Instances placed/culled with `bWorldSpace=true` consistently → robust to the actor being placed off-origin in L_Arena.

## Perf — CORRECT
HISM (never individual actors); `MaxPlacementAttemptsPerInstance`=16 bounds rejection sampling; per-layer `InstanceCount` cap is DATA; tick disabled; single deferred synchronous path query (not per-tick). Within §6 intent (final FPS is TASK-137's human watch).

## Zero regression — CONFIRMED
Scatter actor + config are wholly additive. SiegeGameMode edits = one include, one END-of-sequence step-7, two comment fixes. No cards.csv / DT_Cards / card-actor / combat / economy / KillZ change.

---

## TASK-133 constant-value confirmation
- `ASiegeBotController::CastleRedFallbackLocation = FVector(8000.f, 0.f, 0.f)` — CONFIRMED (SiegeBotController.h:319, comment cites +8000).
- `ASiegeBotController::GoldNodeRedFallbackLocation = FVector(7200.f, 0.f, 0.f)` — CONFIRMED (h:323; node stayed 800 in front of the castle, moved WITH it).
- `BotCenterlineSpawnX = 350.f` — UNCHANGED, with the centerline-relative ruling comment (h:278–281).
- NO Blue fallback constant exists or was added — CONFIRMED (only the two Red fallbacks present; player side uses live `TActorIterator`).
- KillZ / other values — untouched (not in the edited files).
- `GoldNode.h` comments — updated ∓1200 → ±7200 with M6.5 rationale (h:20–23, 80); code unchanged.
- Values match CONVENTIONS "World axes (arena contract)" (M6.5-updated) lines 85–91 exactly.

---

## Notes for build-master (on PASS)
1. **HARD DEP (TASK-136):** set `L_Arena` `RecastNavMesh RuntimeGeneration = Dynamic` — without it the blocking/nav half AND the nav-confirmation layer are inert (units walk through obstacles). The geometric corridor still holds, but the milestone's route-around behavior does not. Verify with a test blocker → nav carves + path reroutes.
2. Move castles/nodes/PlayerStart to ±8000 / ±7200 / ≈-6800 so `RebuildKeepClearZones` reads them live (fallbacks only fire when the live actor is missing).
3. Compile TASK-133 + TASK-134 warnings-as-errors via the standard Build.bat editor-bounce.
4. **TASK-137 DataAsset tuning (closes WARN-1 & WARN-2):** keep each obstacle layer's max footprint (mesh bounds × max scale) < `CorridorHalfWidth` (400) — or widen the corridor — so no blocker can lean into the Y≈0 lane; and do NOT list the same UStaticMesh in both a blocking and the grass layer (shared-HISM collision profile). Obstacle meshes must carry footprint collision or they won't carve nav. PIE two-match traversability + FPS verification is TASK-137.

Board: set TASK-133 and TASK-134 to `qa-passed`.
