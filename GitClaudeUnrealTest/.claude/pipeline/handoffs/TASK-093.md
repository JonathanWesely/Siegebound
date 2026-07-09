# TASK-093 handoff — Placement v3: slope limit + obstacle clearance + ghost-on-slope (C++)

- **Author:** gameplay-programmer
- **Date:** 2026-07-08
- **Status requested:** ready-for-qa (orchestrator flips the board)
- **Scope:** files only — no compile, no editor, no Git, exactly the assigned file set.

## Files touched

- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

Nothing else. (TASK-100 edits these same files next — serialization per M5 ruling 14.)

## What was built (spec items 1–7)

### (1) New UPROPERTYs — mechanic rules, NOT CSV

Both `EditDefaultsOnly, Category = "Siegebound|Placement"` with `// GDD §5 (M4.5)` comments, declared
after `CastlePlinthClearance` in the header:

- `float MaxPlacementSlopeDegrees = 20.f;` (meta `ClampMin="0", ClampMax="90"`)
- `float ObstaclePlacementClearance = 150.f;` (meta `ClampMin="0"`)

Names character-for-character per the Fab-amended tag contract (`ObstaclePlacementClearance`, NOT the
original `TreePlacementClearance` wording).

### (2) Slope check — BUILDINGS only

`IsGroundSlopePlaceable(const FVector& Point) const`:

- Straight-down line trace `Point + (0,0,500)` → `Point − (0,0,500)` (the spec's ±Z500 window; max
  terrain height 250 sits well inside it).
- Channel: **ECC_Visibility** (flagged decision 1 below), `bTraceComplex = false`, ghost actor added to
  the ignore list (belt-and-braces — its collision is fully disabled).
- Slope = angle between `ImpactNormal` and +Z: `RadiansToDegrees(Acos(Clamp(ImpactNormal.Z, -1, 1)))`.
  Refuse when `slope > MaxPlacementSlopeDegrees` (exactly 20.0° still passes — "steeper than" refuses).
- Trace miss / no blocking hit / null world ⇒ **false = refuse (fail-closed)** with a `Verbose` log per
  the task block.
- Refusal surfaces as HUD string **"Too steep"** (NSLOCTEXT key `CardRefused_TooSteep`) through the
  existing `RefuseCardPlay` → `OnCardPlayRefused` + `OnCardRefused` path — no delegate signature change.

### (3) Obstacle clearance — BUILDINGS only

`HasObstacleClearance(const FVector& Point) const`:

- Iterates `TActorIterator<AActor>` and skips anything not carrying actor tag **`Obstacle`** (exact
  FName, static-const; covers trees AND rocks per the Fab amendment — tag-driven, so new obstacle types
  never require code changes).
- Refuses when `FVector::DistSquared2D(actor location, candidate) < ObstaclePlacementClearance²` —
  planar 2D per spec, same math shape as the shipped `HasBuildingClearance`.
- Refusal surfaces as HUD string **"Too close to obstacles"** (NSLOCTEXT key
  `CardRefused_ObstacleClearance`), same refusal path.

### (4) Ghost projection

Already-correct behavior, now encoded as law in comments and verified:

- `PlacementLocation` **is** the cursor trace's `ImpactPoint` (Visibility channel), i.e. the traced
  SURFACE height under the cursor — on the flat floor, a 250-high hill crown, or a flank alike
  (SM_ArenaTerrain's Use-Complex-As-Simple collision answers simple traces with real per-triangle
  geometry). The ghost's `SetActorLocation(PlacementLocation)` therefore takes its Z from the surface.
- Rotation: the ghost keeps its spawn-time yaw-only rotation (`GhostYawOffset`) and is **never** rotated
  afterward — upright, NO normal alignment (comment added at the update site so a future edit can't
  silently add tilt).
- New refusals drive the red ghost exactly like existing invalid placements: they set
  `bPlacementValid = false` in the same per-frame validity chain that feeds the `GhostColor` MID switch.

### (5)+(6) Unchanged — verified by inspection

- Unit/miner placement: both new gates are behind `bPendingIsBuilding` — units/miners still need only
  the navmesh-valid point (+ half/plinth rules). Untouched.
- Castle-roof refusal (`IsPointOnNavmesh` + `NavProjectionExtent`), enemy-half (`PlacementMaxX`),
  200-unit `BuildingClearance`, miner cap (entry + confirm re-gate), card-leaves-hand-at-CONFIRM: all
  byte-identical logic, untouched.
- No cards.csv / DT_Cards change; no BIE/delegate signature change (`FOnCardPlayRefused` /
  `FOnCardRefused` exactly as shipped — the new reasons ride the existing FString path).

### (7) Shadow law

New locals: `TraceStart`, `TraceEnd`, `SlopeQueryParams`, `SlopeHit`, `SlopeDegrees`,
`ObstacleTagName`, `ObstacleClearanceSq`, `Candidate`. None collide with any member/UPROPERTY name;
the two new UPROPERTY names collide with nothing existing (repo-wide grep came back clean before the
edit). Param name `Point` follows the existing sibling helpers (`IsPointOnNavmesh` etc.) and shadows no
member.

## Validation order (spec asked this documented)

Per-frame in `UpdatePlacementGhost` (PlayerTick runs it BEFORE the LMB confirm poll, so confirm always
consumes same-frame state — both new gates are therefore pre-checked BEFORE any gold moves, §3.0
net-zero law):

1. Ground hit + own half (`X <= PlacementMaxX`) — M1, all cards
2. Navmesh projection + castle-plinth keep-out — TASK-030, all cards
3. **Slope** (`IsGroundSlopePlaceable`) — buildings only, NEW
4. **Obstacle clearance** (`HasObstacleClearance`) — buildings only, NEW
5. Building-vs-building clearance (`HasBuildingClearance`) — §3.5, unchanged

The FIRST failing rule is recorded in `EPlacementInvalidReason` (two new enumerators: `Slope`,
`Obstacle` — private, non-UENUM, no serialization impact) and `TryConfirmPlacement`'s refusal branch is
now a switch mapping each reason to its exact HUD string.

## Flagged decisions (QA: please rule on each)

1. **Slope-trace channel = ECC_Visibility** (spec offered WorldStatic/Visibility, programmer picks).
   Rationale: it is the SAME channel as the cursor trace (`TraceCursorToGround`), so the surface that
   positioned the ghost is the surface whose slope is measured — no channel-mismatch drift; anything
   cursor-placeable already blocks Visibility. `bTraceComplex = false`, matching the cursor trace
   (terrain is Use-Complex-As-Simple, so normals are real per-triangle either way).
2. **Slope trace runs at `PlacementLocation` (the cursor-trace impact point), not at the navmesh-projected
   point.** The spec phrase "at the navmesh-projected candidate point" is read as "the candidate point
   that has passed navmesh projection": `IsPointOnNavmesh` discards its projected FNavLocation (shipped
   behavior), the building SPAWNS at `PlacementLocation`, and the navmesh is a tessellated approximation
   that can float off the true surface — measuring where the actor will actually stand is the physically
   correct reading. Max XY divergence is bounded by `NavProjectionExtent` (50).
3. **No obstacle caching** (spec: optional, document). Plain `TActorIterator<AActor>` per candidate
   evaluation, only while placement mode is live — same pattern as `HasBuildingClearance` /
   `IsPointInsideCastlePlinth`; N ≈ 20 obstacles among a few hundred world actors. A cache adds
   staleness risk for no measurable win.
4. **Boundary semantics:** slope exactly 20.0° passes ("steeper than" refuses — spec wording); obstacle
   distance exactly 150.0 passes (`<` refuses — mirrors the shipped `BuildingClearance` comparison
   character-for-character). Ruling requested only if QA reads "within" as inclusive.
5. **ClampMax="90" on MaxPlacementSlopeDegrees** — not in the spec; added so an editor tweak can't set a
   nonsensical >90° threshold. Zero behavior change at the default.
6. **Refusal-priority order slope → obstacle → building-clearance** (a point failing several rules
   reports the first). Slope leads because it is a property of the ground itself; the two clearance
   rules follow in age order. Spec didn't pin a priority.
7. **Names-block note:** the board's names block says "refusal path OnCardRefusedMessage" — the actual
   shipped members are `FOnCardPlayRefused OnCardPlayRefused` + `FOnCardRefused OnCardRefused` (via
   `RefuseCardPlay`). Signatures untouched per spec; flagging so the name mismatch isn't scored against
   this task.
8. **Ghost-on-slope required no code change** (spec item 4): the shipped ghost already takes its Z from
   the cursor-trace impact point and never tilts. Delivered as verification + law-encoding comments at
   the update site rather than new logic.

## What QA should scrutinize

- HUD strings character-for-character: `"Too steep"`, `"Too close to obstacles"`; tag FName
  `"Obstacle"`; UPROPERTY names `MaxPlacementSlopeDegrees` / `ObstaclePlacementClearance`.
- Net-zero law: both new refusals fire from the `!bPlacementValid` branch at the top of
  `TryConfirmPlacement`, before `SpendGold`/spawn/hand-consume can run.
- Both new gates gated on `bPendingIsBuilding` (covers Building cards AND Deep-Mine-style Economy
  buildings via `IsBuildingCard` — deliberate: it is the single source of truth for "spawns an
  ABuilding", and an obstacle-adjacent/sloped Deep Mine should refuse like any building).
- Shadow scan (C4457/58/59) over the new locals listed above.
- New include `CollisionQueryParams.h` (alphabetical, mirrors Projectile.cpp's IWYU precedent for
  `SCENE_QUERY_STAT`).

## Assets/tags referenced (built by parallel tasks)

- Actor tag `Obstacle` — set on every tree/rock instance by TASK-095 (until then the obstacle gate
  finds zero tagged actors and passes — correct on the current flat arena).
- Actor tag `Terrain` — NOT consumed by this task (projectile contract, TASK-094).
- No new content paths; ships via TASK-103's batch compile.
