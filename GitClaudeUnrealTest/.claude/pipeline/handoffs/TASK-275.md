# TASK-275 handoff — Standard-body reshaping + enemy-box helper (`ASummonedUnit` + `ACastle`)

**Status:** ready-for-qa
**Branch:** m7.6-arena10x
**Assignee:** gameplay-programmer
**Law:** CONVENTIONS "Unit commands (Shield Wall stances) — ATTACK / HOLD / DEFEND (W1, 2026-07-23)"
**Consumes:** TASK-274 (qa-passed) `ASiegePlayerController` API — `GetCurrentCommand`/`HasIssuedCommand`/`GetHoldLocation`/`GetHoldRadius`.

This is the COMMAND-CONSUMING (unit) half. **CODE-ONLY — NO compile, NO Git** (build-master compiles at TASK-277).

## Files edited (ONLY these four — per the guardrail)
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Castle.h`
- `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp`

`SiegePlayerController.{h,cpp}`, the bot, the capture-zone code, and the Siege/Support/None dispatch are all UNTOUCHED.

## The gate (byte-for-byte legacy protection)
Inserted in `ASummonedUnit::UpdateState` **between the Support dispatch and `const FVector MyLocation = GetActorLocation();`** (SummonedUnit.cpp ~1037-1062), so the legacy Standard body (now lines 1064-1102) is **byte-for-byte identical** to before — verified by re-reading it against the original.

```cpp
if (Profile == ECardProfile::Standard && Team == ETeamId::Blue)
{
    if (const UWorld* CmdWorld = GetWorld())
    {
        if (const ASiegePlayerController* PC = Cast<ASiegePlayerController>(CmdWorld->GetFirstPlayerController()))
        {
            if (PC->HasIssuedCommand())
            {
                UpdateStateStandardCommanded(*PC);
                return;
            }
        }
    }
}
```

- **How the PC is read:** `GetWorld()->GetFirstPlayerController()` cast to `ASiegePlayerController` (single-player + bot local PC). Fully null-safe — no world / no PC / not-a-SiegePC / not-yet-issued all fall through to the legacy body, never a crash.
- **How the team is read:** the local human player is Blue through M7 (CONVENTIONS team contract — the codebase hardcodes "player is always ETeamId::Blue", e.g. SiegePlayerController.cpp:2757). So `Team == ETeamId::Blue` == "this is one of the player's own units". Bot units are Red → excluded → **never read the player command**.
- **Miners excluded:** the M1/M2 body is shared by `Standard` AND `None` (miners). Adding `Profile == ECardProfile::Standard` keeps miners (Profile None) on the legacy path, satisfying "None (miners) bodies are UNTOUCHED".
- **Freeze-safe:** `UpdateState` early-returns on `bDead/!bStatsLoaded/bAIFrozen/bSpellFrozen` **above** the gate, so a frozen unit never reaches the command body. FreezeAI/ApplyFreeze are not bypassed.
- **Live stance:** re-evaluated every `UpdateState` tick (timer-driven, no new Tick work) — a mid-flight command change re-targets on the next state check.

## The three branches (`UpdateStateStandardCommanded`)
- **ATTACK** — self-defense `AcquireTarget()` is UNCHANGED (leash/reacquire + acquire + in-aggro attack/advance mirror the legacy body exactly). The ONLY change: when there is no in-aggro `CurrentTarget`, the march **goal = `FindNearestEnemyInSpawnBox(EnemyCastle)` ?? EnemyCastle**, where `EnemyCastle = Cast<ACastle>(FindNearestEnemyCastle())`. Net: units clear enemy unit/building defenders sitting in the enemy spawn box before hitting the castle; on arrival normal aggro picks those up. `!Goal ⇒ EnterIdle` (match over) preserved.
- **HOLD** — `CurrentTarget = AcquireEnemyNearPoint(GetHoldLocation(), GetHoldRadius())` (only enemies inside the disc are eligible; re-picking each tick inherently drops a target that left the disc and ignores enemies outside it even if in normal aggro range). Goal = that target (EnterAttack in range / EnterAdvance otherwise) **?? march to `GetHoldLocation()` via `EnterAdvanceToLocation`**; on arrival (2D dist ≤ `HoldArrivalTolerance` 150, file-local const) with no in-disc enemy → `EnterIdle` (holds position).
- **DEFEND** — `OwnCastle = FindOwnCastle()` (null ⇒ EnterIdle, match over). `CurrentTarget = AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), DefendRadius)`. Goal = that target (attack/advance) **?? fall back to the own castle (`EnterAdvance(OwnCastle)` — actor path)**. We never attack our own castle (EnterAttack only fires on an enemy `CurrentTarget`).

## New helper signatures
### `ASummonedUnit` (SummonedUnit.h/.cpp)
- `void UpdateStateStandardCommanded(const ASiegePlayerController& PC);` — private; the command dispatcher (switch on `PC.GetCurrentCommand()`).
- `AActor* AcquireEnemyNearPoint(const FVector& Center, float Radius) const;` — private; **exact clone of `AcquireTarget`** (same team-filter, alive-filter, pawn/building bucketing, TieBreakDistance tie-break, nearest-to-SELF selection) with the ONLY change being the eligibility gate: `FVector::DistSquared2D(Candidate->GetActorLocation(), Center) <= Radius²` replacing `GetDistanceToTarget(self) <= AggroRadius`.
- `AActor* FindNearestEnemyInSpawnBox(const ACastle* EnemyCastle) const;` — private; nearest alive enemy ITeamAgent whose `GetActorLocation()` passes `EnemyCastle->IsPointInSpawnBox`. Excludes the `EnemyCastle` itself (its own center trivially passes; it is the fallback goal anyway). `null EnemyCastle ⇒ nullptr`.
- `ACastle* FindOwnCastle() const;` — private; mirror of `FindNearestEnemyCastle` but `Castle->GetTeamId() == Team`, skip destroyed. Returns `ACastle*` (upcasts for `EnterAdvance`).
- `void EnterAdvanceToLocation(const FVector& Point);` — private; POINT variant of `EnterAdvance`. Same attack-exit bookkeeping; issues `AI->MoveToLocation(Point, StructureMoveAcceptanceRadius, /*bStopOnOverlap*/false, /*bUsePathfinding*/true, /*bProjectDestinationToNavigation*/true, /*bCanStrafe*/true, nullptr, /*bAllowPartialPath*/true)`. Re-paths only when the point moved (>1 uu) or path status is Idle. **`EnterAdvance(AActor*)` is left byte-for-byte unchanged.**
- New members: `FVector CurrentMoveGoalLocation` + `bool bHasMoveGoalLocation` (point re-path tracking, mirrors `CurrentMoveGoal`); `float DefendRadius = 2500.f` (EditDefaultsOnly, ClampMin 0, Category `Siegebound|Commands`) — lives HERE per CONVENTIONS (unit-owned, NOT the controller).
- File-local `constexpr float HoldArrivalTolerance = 150.f` (anonymous namespace, alongside `StructureMoveAcceptanceRadius`).
- Forward-declares added: `class ACastle;`, `class ASiegePlayerController;`.

### `ACastle` (Castle.h/.cpp)
- `bool IsPointInSpawnBox(const FVector& Point) const;` — public BlueprintPure; `FMath::Abs(Point.X-ActorX) <= SpawnBoxHalfExtent.X && FMath::Abs(Point.Y-ActorY) <= SpawnBoxHalfExtent.Y` (Z ignored).
- `FVector2D SpawnBoxHalfExtent = FVector2D(840,840)` — EditDefaultsOnly, ClampMin 0.
- **Disjoint from the bot:** this is an ADDITIVE third reader. `ASiegeBotController::IsPointInBotSpawnBox` (TASK-262) is NOT touched — different class, different method name, its own `SpawnBoxHalfExtent`. Paired-tunable 3-way law is documented in the header comment (Castle ≡ PlayerController ≡ BotController, all (840,840)); the flagged future delegation is explicitly noted OUT of scope.

## Include / type completeness
- SummonedUnit.cpp gains `#include "Siegebound/SiegePlayerController.h"` (complete type for the const getters + `ESiegeUnitCommand` via its `UnitCommand.h` include). No dependency cycle — SiegePlayerController.h forward-declares ASummonedUnit, never includes SummonedUnit.h.
- `ACastle` complete type was already `#include "Siegebound/Castle.h"` in SummonedUnit.cpp (used for `Cast<ACastle>` + `IsPointInSpawnBox` + `TActorIterator<ACastle>`).
- Castle.cpp: `FMath`/`FVector2D` via CoreMinimal (already present); no new include needed.

## What QA should scrutinize
- **Legacy byte-identity when the gate is false** — the inserted gate sits ABOVE `const FVector MyLocation`; legacy body lines 1064-1102 are unchanged (I diffed against the pre-edit read). Confirm miners/bot/Red/pre-command all take it.
- **Shadow scan:** new locals — `CmdWorld`, `PC`, `HoldLoc`, `HoldRad`, `OwnCastle`, `EnemyCastleActor`, `EnemyCastle`, `BoxDefender`, `Goal`, `Acquired`, `Candidate`, `Agent`, `Distance`, `Best`, `BestDistance`, `BestPawn/BestOther*`, `Castle`, `AI`, `bPointChanged`, `Result`, param `PC`/`Point`/`Center`/`Radius`. None shadow an inherited reflected member (Owner/PlayerState/Instigator/Controller). `Castle`/`Goal`/`Candidate` reuse names already local in this class.
- **`MoveToLocation` signature** — 8 args, `EPathFollowingRequestResult::Type` return (mirrors the existing `MoveToActor` usage; `bProjectDestinationToNavigation=true` so a traced hold surface point projects to the navmesh).
- **Null-safety** — every new lookup guarded (`GetWorld`, PC cast, `FindOwnCastle` null, `FindNearestEnemyInSpawnBox` null-castle/null-world, `AcquireEnemyNearPoint` null-world). Empty world / no castle / no PC all degrade to EnterIdle or the legacy body.
- **Disc semantics** — HOLD/DEFEND eligibility uses the candidate's `GetActorLocation()` vs `Center` (2D), per spec wording "whose location is within Radius"; nearest-to-self selection + tie-break kept identical to AcquireTarget.

## QA delta 1 (2026-07-23 — HOLD kite-out-of-position fix, non-blocking WARN)
QA passed (0 blockers) with one WARN folded in before W1: `EnterAdvanceToLocation` was not symmetric with `EnterAdvance`. `EnterAdvance(AActor*)` does NOT invalidate `bHasMoveGoalLocation`/`CurrentMoveGoalLocation`, so a HOLD unit that pathed to point P → chased an in-disc enemy (actor move) → had that enemy LEAVE the disc while still Moving would, on the return `EnterAdvanceToLocation(P)`, see `bPointChanged==false` + non-Idle status and SKIP re-issuing, leaving the stale `MoveToActor(enemy)` live and kiting the unit out of position until that path went Idle.

**Fix (local to `EnterAdvanceToLocation`, ~SummonedUnit.cpp:1869-1874; `EnterAdvance(AActor*)` left byte-for-byte untouched):** capture `const bool bWasActorMove = (CurrentMoveGoal != nullptr);` BEFORE the existing `CurrentMoveGoal = nullptr;`, and OR it into the re-path condition.

Before:
```cpp
CurrentMoveGoal = nullptr;
const bool bPointChanged = !bHasMoveGoalLocation || !CurrentMoveGoalLocation.Equals(Point, 1.f);
if (bPointChanged || AI->GetMoveStatus() == EPathFollowingStatus::Idle)
```
After:
```cpp
const bool bWasActorMove = (CurrentMoveGoal != nullptr);
CurrentMoveGoal = nullptr;
const bool bPointChanged = !bHasMoveGoalLocation || !CurrentMoveGoalLocation.Equals(Point, 1.f);
if (bWasActorMove || bPointChanged || AI->GetMoveStatus() == EPathFollowingStatus::Idle)
```
A return-to-point immediately after an actor-move now ALWAYS re-issues the location move even when the point is unchanged and status is non-Idle, closing the kite gap.

**DEFEND NIT (per-tick `MoveToActor` re-issue idling at own castle):** left as-is by direction — it is identical to the pre-existing legacy castle-advance behavior in `EnterAdvance(AActor*)` (any unit advancing on a non-pawn structure re-issues when path-following goes Idle), not a regression introduced here; not expanded.

## Out of scope (untouched)
`SiegePlayerController.{h,cpp}`, `SiegeBotController.*`, `CaptureZone.*`, the Siege/Support/None dispatch, input assets, the HUD. No compile, no Git.

## Slack
Posting (or proxy text) in ⚙️ Dev & QA (C0BF0QZP3CN, thread_ts 1783116269.740549):
> ⚙️ GAMEPLAY-PROGRAMMER: ✅ TASK-275 ready-for-qa — Shield Wall unit-side (ASummonedUnit + ACastle), code-only. Gate `Profile==Standard && Team==Blue && PC->HasIssuedCommand()` inserted above the legacy body (byte-for-byte unchanged; miners/bot/Red/pre-command all excluded). ATTACK clears the enemy spawn-box before the castle (new ACastle::IsPointInSpawnBox + SpawnBoxHalfExtent(840,840), additive — bot's IsPointInBotSpawnBox untouched); HOLD → AcquireEnemyNearPoint(HoldLocation,HoldRadius) + EnterAdvanceToLocation; DEFEND → AcquireEnemyNearPoint(OwnCastle,DefendRadius=2500) + fall-back to FindOwnCastle. Null-safe, freeze-safe, no new Tick work. Files: SummonedUnit.{h,cpp}, Castle.{h,cpp}. handoffs/TASK-275.md.
