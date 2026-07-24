# TASK-262 handoff — Bot placement: shrink to castle box + capture-zone spawn (C++, branch)

Status: **ready-for-qa**
Agent: gameplay-programmer
Branch: m7.6-arena10x
Date: 2026-07-23
Lane: branch (frozen on main until the Phase-6 merge)
Mirror of: TASK-261 (player placement, `SiegePlayerController.{h,cpp}` — disjoint file set, ran in parallel)

## What I changed
Replaced ONLY the whole-own-half spawn gate in the bot's placement validator with the W1-PREP-3 rule: a spawn point is eligible iff it is inside the **Red spawn box around Castle_Red** OR inside a **Red-owned capture zone**. Everything else (plinth keep-out, building clearance, the navmesh ring-search) is byte-unchanged. Code-only — not compiled (build-master owns compile at TASK-264).

### Files edited (2, both already in the branch set from the mines feature)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`

## The edit, surgically

### 1. New UPROPERTY (SiegeBotController.h, after `CastleRedFallbackLocation`)
```cpp
UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
FVector2D SpawnBoxHalfExtent = FVector2D(840.f, 840.f);
```
Default (840,840) per CONVENTIONS "W1-PREP additions 3" (= 2× `CastlePlinthClearance`; FLAGGED tunable). `FVector2D` needs no extra include (CoreMinimal).

### 2. Two new private helpers (declared next to `IsBotHalfPointClear` in .h; defined right after it in .cpp)
```cpp
bool IsPointInBotSpawnBox(const FVector& Point) const;   // 2D square, half-extent SpawnBoxHalfExtent, centered on Castle_Red
bool IsPointInCapturedZone(const FVector& Point) const;  // single TActorIterator<ACaptureZone>, null-safe
```

`IsPointInBotSpawnBox`:
```cpp
const FVector CastleRed = GetCastleRedLocation();
return FMath::Abs(Point.X - CastleRed.X) <= SpawnBoxHalfExtent.X &&
    FMath::Abs(Point.Y - CastleRed.Y) <= SpawnBoxHalfExtent.Y;
```

`IsPointInCapturedZone` (null-safe world + single-instance iterator; `CanTeamSpawnHere` folds box test AND Red-ownership match, so Neutral/Blue owner ⇒ false):
```cpp
for (TActorIterator<ACaptureZone> It(World); It; ++It)
{
    const ACaptureZone* Zone = *It;
    return Zone && Zone->CanTeamSpawnHere(ETeamId::Red, Point);
}
return false;
```

### 3. The gate replacement (SiegeBotController.cpp, `IsBotHalfPointClear`)
BEFORE:
```cpp
if (!IsOnOwnHalf(Point.X))
{
    return false; // NEVER the enemy (Blue) half
}
```
AFTER:
```cpp
if (!IsPointInBotSpawnBox(Point) && !IsPointInCapturedZone(Point))
{
    return false; // outside both the Red castle box and any Red-owned mid zone
}
```
`!(box || zone)` == `!box && !zone` — the exact net of the spec's "allowed iff box OR zone", kept as the early-out so the plinth keep-out + building-clearance loops below run UNCHANGED. `ComputeValidBotSpawnPoint`'s widening ring-search still calls this gate (untouched) — the ring walk to a clear spot is intact.

### 4. New include (SiegeBotController.cpp, alphabetical between `Building.h` and `CardRow.h`)
```cpp
#include "Siegebound/CaptureZone.h"
```

## How Castle_Red is found + the +25000 fallback is preserved
`IsPointInBotSpawnBox` calls the pre-existing `GetCastleRedLocation()` (SiegeBotController.cpp:~1069), which is the SAME live team-filtered lookup the controller already uses:
```cpp
for (TActorIterator<ACastle> It(World); It; ++It)
    if (IsValid(Castle) && Castle->GetTeamId() == BotTeam) return Castle->GetActorLocation();
return CastleRedFallbackLocation;   // FVector(25000.f, 0.f, 0.f)
```
So the +25000 X fallback is preserved automatically — no duplicated iterator, no changed fallback behavior. If no Red `ACastle` exists, the box centers on (25000,0), exactly as before.

## Confirmation: non-spawn `IsOnOwnHalf` callers untouched
`IsOnOwnHalf` and its four TARGET/APPROACH callers are byte-unchanged (verified by grep after the edit):
- definition at :848
- miner-approach clamp at :551 and :602
- own-half unit iteration at :877, own-half hero iteration at :900

Inside `IsBotHalfPointClear` the only remaining `IsOnOwnHalf` mentions are in the explanatory comment (:1148–1149); the live gate is `IsPointInBotSpawnBox && IsPointInCapturedZone`. The M3 ordered bot-rules decision trace / `LogSiegeBot` output was not touched (this task only edits placement helpers, not `EvaluateDecisions`).

## What QA should scrutinize
- **Include/complete-type law:** `ACaptureZone` methods are dereferenced (`CanTeamSpawnHere`) ⇒ `Siegebound/CaptureZone.h` included; `ACastle` already included; `TActorIterator` covered by `EngineUtils.h` (already included); `ETeamId` from `Siegebound/TeamId.h` (already included via the .h). No forward-decl-only dereference.
- **Shadow law:** new locals are `CastleRed`, `Zone`, `World` — none shadow `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`. Member is `SpawnBoxHalfExtent`, no `Owner` member introduced.
- **Null-safety:** `IsPointInCapturedZone` returns false on null world and when no zone exists (pre-capture behavior = mid unspawnable). `GetCastleRedLocation` is already null-safe (fallback).
- **Semantics of the gate rewrite:** `!(box || zone)` early-out preserves the composed keep-out/clearance checks that follow. The box is entirely on the Red half (centered at X=25000, half-extent 840), so the bot still never spawns on the Blue half; the mid zone straddles X=0 but only passes when Red-owned.
- The `return Zone && Zone->CanTeamSpawnHere(...)` inside the loop returns on the single instance (matches the "single `TActorIterator`" spec form; null-guarded).

## NOT in scope (per the task)
- `SiegePlayerController.{h,cpp}` (TASK-261, parallel) — untouched.
- `CaptureZone.{h,cpp}` (TASK-260, consumed as-is) — untouched.
- Level placement / centerline deletion / PIE capture-suite — build-master's TASK-264.
- No compile, no Git (build-master, TASK-264).
