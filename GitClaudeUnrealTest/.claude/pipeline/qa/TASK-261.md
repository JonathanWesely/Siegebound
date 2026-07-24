# QA Report — TASK-261

Verdict: **PASS**
Reviewer: qa-reviewer
Branch: m7.6-arena10x
Date: 2026-07-23
Scope: pre-compile review of `SiegePlayerController.{h,cpp}` — player-placement spawn-box shrink (W1-PREP additions 3). Compile rides TASK-264.

## Verdict summary
0 BLOCKERS, 0 WARN, 3 NIT. Every acceptance criterion is met; the change is surgically confined to the first ground/region gate. Cleared to compile (TASK-264).

## Acceptance verification
- **First gate rewritten (`UpdatePlacementGhost`, :1239)** — `bool bValid = bGroundHit && (IsPointInOwnSpawnBox(Hit.ImpactPoint) || IsPointInCapturedZone(Hit.ImpactPoint));`. The old `Hit.ImpactPoint.X <= PlacementMaxX` half-test is GONE. ✓
- **`IsPointInOwnSpawnBox` (:2396)** — 2D XY abs-delta square centered on the owned castle, half-extent `SpawnBoxHalfExtent`; found via team-filtered `TActorIterator<ACastle>` skipping `!IsValid` and `GetTeamId() != ETeamId::Blue`; null-safe (no Blue castle ⇒ warn-once via `bWarnedMissingSpawnCastle`, return false, no crash). Box math mirrors `IsPointInsideCastlePlinth` exactly. ✓
- **`IsPointInCapturedZone` (:2438)** — single `TActorIterator<ACaptureZone>`, `IsValid` guard, returns `Zone->CanTeamSpawnHere(ETeamId::Blue, Point)`; null-safe if no zone (returns false = pre-capture behavior). ✓
- **`PlacementMaxX` fully retired** — UPROPERTY replaced by `SpawnBoxHalfExtent = FVector2D(840,840)` (h:537-538); the :1236 read replaced; the :1033 refusal-log arg removed and the message rewritten ("outside the Blue spawn box and any Blue-owned capture zone…"). Repo-wide grep of `Source/` returns only 3 hits for `PlacementMaxX`, all in explanatory comments. ✓
- **Downstream composition UNCHANGED** — navmesh projection (`IsPointOnNavmesh`), `IsPointInsideCastlePlinth`, Building-only slope/obstacle/building-clearance, ghost projection, and the `EPlacementInvalidReason` switch are byte-identical (:1240-1290; refusal switch :1006-1039). Only the first gate expression and its rule-(1) comment changed. ✓

## Filter checks
- **UE 5.8 APIs:** `TActorIterator`, `FMath::Abs`, `GetActorLocation`, `GetWorld`, `IsValid`, `GetTeamId`, `UE_LOG`, `GetNameSafe` — all current, none deprecated/removed. ✓
- **2D box test:** `FMath::Abs(Point.X - CastleLocation.X) <= SpawnBoxHalfExtent.X && …Y` — correct square-region test; XY-only (Z ignored) is correct for a spawn region. ✓
- **TActorIterator null safety:** both helpers guard `!World` first, then `IsValid` per actor; return-on-first-match is correct against the single-castle / single-zone contract. ✓
- **Include law:** `Siegebound/CaptureZone.h` fully `#include`d at cpp:26 (complete type required for the `CanTeamSpawnHere` call — satisfied); `Siegebound/Castle.h` (:28) and `EngineUtils.h` (:17) already present; `ETeamId` via `TeamId.h` in the header. ✓
- **Complete-type / const:** `CanTeamSpawnHere` is a `const` method on `ACaptureZone`, called on a `const ACaptureZone*` from the `const IsPointInCapturedZone` — legal. `ACastle::GetTeamId()` is public + const (Castle.h:72) — legal on `const ACastle*`. ✓
- **Shadow law:** new locals `World`, `Castle`, `CastleLocation`, `Zone` (+ param `Point`); none shadow `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`. `Castle`/`CastleLocation` reuse the names `IsPointInsideCastlePlinth` already uses. ✓
- **Const-correctness split:** `IsPointInOwnSpawnBox` is intentionally NON-const — the ONLY member it mutates is the warn-once latch `bWarnedMissingSpawnCastle` (declared h:860, next to `bWarnedNoNavData`). This mirrors the established `IsPointOnNavmesh` (non-const, latched) vs `IsPointInsideCastlePlinth` (const) precedent. No hidden mutation. `IsPointInCapturedZone` is const. ✓
- **Header/cpp consistency:** declarations (h:790 non-const, h:800 const) match definitions (cpp:2396 non-const, cpp:2438 const); `SpawnBoxHalfExtent` UPROPERTY(EditDefaultsOnly, FVector2D) and `bWarnedMissingSpawnCastle` both declared and used consistently. ✓
- **`FindFriendlyCastle` non-reuse — reasoning sound:** `FindFriendlyCastle` returns only a *living* (non-destroyed) castle, so it would refuse the box when the castle is at 0 HP; the spawn box must center on the actor regardless of HP (the plinth keep-out already uses the HP-agnostic all-castle iterator). Using a fresh HP-agnostic team-filtered iterator is the correct choice and consistent with `IsPointInsideCastlePlinth`. In the fully-destroyed edge the match is already over and placement disabled, so both paths converge on "refuse". Confirmed sound. ✓
- **PlacementMaxX regression scan:** grep found no other reader in `Source/` — it was consumed only at the retired gate and the retired log arg. No regression risk to the placement flow. ✓
- **Performance:** both helpers run only in placement mode (per-tick `UpdatePlacementGhost`); N is 1-2 castles / 1 zone, and the `||` short-circuits the zone iterator whenever the point is already in the box. Same per-tick TActorIterator cost profile as the existing `IsPointInsideCastlePlinth`/`HasObstacleClearance`. Acceptable. ✓
- **Naming vs CONVENTIONS "W1-PREP additions 3":** `SpawnBoxHalfExtent` (FVector2D EditDefaultsOnly default (840,840)), `IsPointInOwnSpawnBox`, `IsPointInCapturedZone` — all match the law and the board `names:` block. ✓

## Findings
- [NIT] SiegePlayerController.h:74 — class-doc comment still reads "Valid placement (ALL cards) = ground hit AND X <= 0 (Blue half per CONVENTIONS)"; now stale (box-or-zone gate). Doc drift only — safe to leave, but a one-line refresh would keep the header honest.
- [NIT] SiegePlayerController.h:832-833 — `bPlacementValid` comment "ground hit AND on the Blue half" is stale for the same reason.
- [NIT] SiegePlayerController.h:835-836 — `PlacementLocation` comment "on the Blue half" wording likewise stale. None of these reference `PlacementMaxX` by name and none affect compile/behavior; noted for cleanliness only.

## Notes for build-master (TASK-264)
- The spawn box is centered ON `Castle_Blue`, so the placeable region is the box MINUS the plinth keep-out (`CastlePlinthClearance` 420 half-extent vs the 840 box half-extent) = a ~420-wide ring of ground around the plinth. PIE suite test (a) should confirm that ring is non-empty and navmesh-valid at the shipped `SpawnBoxHalfExtent`; both are FLAGGED tunables if the ring feels too tight in Jonathan's W1 look.
- `IsPointInCapturedZone` depends on a live `ACaptureZone` in `L_Arena` — until TASK-264 step (3) places `CaptureZone_Center`, the mid is correctly unspawnable (null-safe), so run capture tests (c)-(f) only after the instance is placed.
- No compile performed here (correct — code-only handoff). This review clears the batch to compile with TASK-260/262.
