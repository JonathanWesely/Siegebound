# TASK-261 handoff — Player placement: shrink to castle box + capture-zone spawn

Status: **ready-for-qa**
Agent: gameplay-programmer
Branch: m7.6-arena10x
Date: 2026-07-23
Lane: branch (SiegePlayerController.{h,cpp} JOINS the branch touched-files set; frozen on main until the Phase-6 merge)

## What I changed
Replaced the first ground/region gate of player card placement. The old rule
("valid iff ground hit AND X ≤ PlacementMaxX" = the entire Blue half X≤0) is gone;
placement is now valid iff the traced ground point is **inside the Blue spawn box
OR inside a Blue-owned capture zone**. Everything downstream of that first gate
(navmesh projection, plinth keep-out, slope, obstacle & building clearance, the
ghost projection, the invalid-reason enum) is UNCHANGED.

Code-only, not compiled (build-master owns the compile at TASK-264).

## Files edited (2)
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

No other files touched. `SiegeBotController.{h,cpp}` (TASK-262, parallel) NOT touched — disjoint file set. `ACaptureZone` NOT touched — consumed read-only.

## The edit, precisely

### SiegePlayerController.cpp
- **`#include "Siegebound/CaptureZone.h"`** added (alphabetical, before `CardRow.h`). Complete type is required — I call `Zone->CanTeamSpawnHere(...)`, not just hold a pointer.
- **`UpdatePlacementGhost()` (~:1236)** — the first gate changed from:
  `bool bValid = bGroundHit && Hit.ImpactPoint.X <= PlacementMaxX;`
  to:
  `bool bValid = bGroundHit && (IsPointInOwnSpawnBox(Hit.ImpactPoint) || IsPointInCapturedZone(Hit.ImpactPoint));`
  The rule-(1) comment block just above it was updated to describe the box-or-zone gate. Rules (2)-(6) and the rest of the function are byte-unchanged.
- **Refusal log (~:1032, `default:` case)** — rewrote the message; it no longer references `PlacementMaxX` (removed the `X > %.0f` fragment + its arg), now reads "outside the Blue spawn box and any Blue-owned capture zone, off the navmesh, or on a castle plinth".
- **Two new helper implementations** added right after `IsPointInsideCastlePlinth`.

### SiegePlayerController.h
- **`PlacementMaxX` UPROPERTY (was :527-529) RETIRED** — replaced by:
  `UPROPERTY(EditDefaultsOnly, Category="Siegebound|Placement") FVector2D SpawnBoxHalfExtent = FVector2D(840.f, 840.f);`
- Two new helper declarations (after `IsPointInsideCastlePlinth`).
- One new warn-once latch member `bool bWarnedMissingSpawnCastle = false;` (next to `bWarnedNoNavData`).

## New helper signatures
```cpp
// 2D square around Castle_Blue, half-extent SpawnBoxHalfExtent. Non-const ONLY
// for the warn-once latch (mirrors IsPointOnNavmesh precedent).
bool IsPointInOwnSpawnBox(const FVector& Point);

// Single ACaptureZone via TActorIterator; returns Zone->CanTeamSpawnHere(Blue, Point).
bool IsPointInCapturedZone(const FVector& Point) const;
```

## How I found Castle_Blue
Fresh team-filtered `TActorIterator<ACastle>` inside `IsPointInOwnSpawnBox`, exactly
the pattern `IsPointInsideCastlePlinth` (~:2383) uses: iterate, `IsValid` guard,
`Castle->GetTeamId() != ETeamId::Blue` → skip, then read `Castle->GetActorLocation()`
and test `FMath::Abs(Point.{X,Y} - CastleLocation.{X,Y}) <= SpawnBoxHalfExtent.{X,Y}`.
The local player is always Blue (team contract), so the team is the literal
`ETeamId::Blue` (matches `IsPointInCapturedZone`'s `CanTeamSpawnHere(ETeamId::Blue, …)`).
I deliberately did NOT reuse `FindFriendlyCastle` (it filters out destroyed castles;
the box should center on the castle regardless of HP — and if Blue's castle is
destroyed the match has ended and placement is already disabled). The box is
centered on the castle actor, so it straddles Castle_Blue at (−8000,0) → covers
roughly X∈[−8840,−7160], Y∈[−840,+840] with the default half-extent.

## PlacementMaxX retirement
Fully retired. A repo-wide grep of `Source/` for `PlacementMaxX` returns only THREE
hits, all in comments documenting "the retired X<=PlacementMaxX half-test". The
UPROPERTY, the `.cpp:1236` read, and the `.cpp:1033` log arg are all removed. (Its
default was 0.f = the neutral centerline, so no BP override carried a meaningful value.)

## Null-safety / behavior
- No Blue castle in world ⇒ `IsPointInOwnSpawnBox` refuses (returns false) and warns
  ONCE (latched — the function is polled every tick in placement mode). Never crashes.
- No `ACaptureZone` in world ⇒ `IsPointInCapturedZone` returns false silently
  (pre-capture-feature behavior: mid is unspawnable). Never crashes.
- Neutral or Red-owned mid zone ⇒ `CanTeamSpawnHere(Blue, …)` returns false ⇒ not
  placeable there (correct — nobody spawns in an unheld mid; Blue can't spawn in a
  Red-held mid).
- Short-circuit `||`: a point in the spawn box never queries the zone (cheap common case).

## What QA should scrutinize
- **Shadow law:** new locals are `World`, `Castle`, `CastleLocation`, `Zone` (+ param
  `Point`). None shadow `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`.
  `Castle`/`CastleLocation` are the same names `IsPointInsideCastlePlinth` already uses.
- **Complete-type include law (SiegePlayerController.cpp):** `Zone->CanTeamSpawnHere`
  needs a complete `ACaptureZone` — `Siegebound/CaptureZone.h` is now `#include`d (not
  just forward-declared). `Castle->GetTeamId()`/`GetActorLocation()` — `Siegebound/Castle.h`
  already included (:27). `TActorIterator` — `EngineUtils.h` already included (:17).
- **Const correctness:** `IsPointInOwnSpawnBox` is intentionally NON-const (it flips the
  warn-once latch), called from the non-const `UpdatePlacementGhost` — fine.
  `IsPointInCapturedZone` is const. This mirrors the existing `IsPointOnNavmesh`
  (non-const for its latch) vs `IsPointInsideCastlePlinth` (const) split.
- **Scope discipline:** only the first gate changed. Rules 2-6, the ghost projection,
  and the `EPlacementInvalidReason` switch are untouched; a point inside the box but on
  the plinth / off-navmesh / too steep / too close still refuses with its existing reason.

## NOT in scope (per the task)
- `SiegeBotController.{h,cpp}` (TASK-262, parallel) — untouched.
- `ACaptureZone` (TASK-260) — consumed read-only via its public API, not modified.
- Level placement of `CaptureZone_Center`, centerline (`M_CenterlineStripe`) actor
  deletion, and the compile + PIE capture-suite — all build-master's TASK-264.
- No compile, no Git.
