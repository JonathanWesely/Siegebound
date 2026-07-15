# TASK-141 — Hero terrain movement tuning (handoff → QA)

**Status:** ready-for-qa
**Assignee:** gameplay-programmer
**Milestone:** M6.6 Climbable terrain
**Law:** CONVENTIONS "Climbable terrain (M6.6)" → "Hero terrain-movement tuning (`AHeroCharacter` ONLY)"

## Scope note
Files-only, no compile, no editor/MCP, no Git (build-master compiles at TASK-143; QA reviews first).

## Files touched (exactly two — the hard constraint)
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h`
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`

> Path note: the actual files live at `Source/GitClaudeUnrealTest/Siegebound/…` (one fewer `GitClaudeUnrealTest` nesting than the dispatch prompt's `…/GitClaudeUnrealTest/Siegebound/…`). Same files, verified via glob.

The template base `GitClaudeUnrealTestCharacter.h/.cpp` was **NOT** touched (CONVENTIONS §"C++ layout" template law — it is the base for the unrelated `Variant_*` maps; `AHeroCharacter` subclasses it, so all three tunables live on the subclass).

## What changed

### HeroCharacter.h
1. Three new `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement")` members, added right after `SprintSpeed` in the Movement category, with a shadow-law comment block:

   | Property | Type | Default | Drives (on `UCharacterMovementComponent`) |
   |---|---|---|---|
   | `HeroMaxStepHeight` | float | `50.f` | `MaxStepHeight` (was 45) |
   | `HeroWalkableFloorAngle` | float | `50.f` (deg) | `WalkableFloorAngle` via `SetWalkableFloorAngle` (was 44.76) |
   | `HeroJumpZVelocity` | float | `600.f` | `JumpZVelocity` (was 500) |

   Clamp meta: `MaxStepHeight`/`JumpZVelocity` `ClampMin="0"`; `WalkableFloorAngle` `ClampMin="0", ClampMax="90"`.
2. New protected helper declaration `void ApplyTerrainMovementTuning();`, placed immediately after `void ApplyMovementSpeed();`.

### HeroCharacter.cpp
3. **Constructor call site** — `ApplyTerrainMovementTuning();` added right after `GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;` (the existing ctor movement-write, cpp:~45).
4. **BeginPlay call site** — `ApplyTerrainMovementTuning();` added right after the existing `ApplyMovementSpeed();` (cpp:~82). This is the re-apply so a `BP_HeroCharacter` tweak survives — exactly mirroring the `ApplyMovementSpeed()` ctor→BeginPlay pattern.
5. **Helper implementation** — added directly after `ApplyMovementSpeed()`:
   ```cpp
   void AHeroCharacter::ApplyTerrainMovementTuning()
   {
       if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
       {
           MoveComp->MaxStepHeight = HeroMaxStepHeight;
           MoveComp->SetWalkableFloorAngle(HeroWalkableFloorAngle);
           MoveComp->JumpZVelocity = HeroJumpZVelocity;
       }
   }
   ```

`GravityScale` and `AirControl` are untouched (per spec).

## Points for QA to scrutinize
- **Shadow law (the headline scan):** the three members are `Hero`-prefixed on purpose. `MaxStepHeight` / `WalkableFloorAngle` / `JumpZVelocity` are fields of `UCharacterMovementComponent` (a *component*, not a base class of `AHeroCharacter`), and no `ACharacter`/`APawn`/`AActor` base declares any of the three prefixed names — so there is no C4457/58/59 collision. Grep across `Source/` confirms the three names + `ApplyTerrainMovementTuning` appear ONLY in these two files.
- **Complete-type include law:** `#include "GameFramework/CharacterMovementComponent.h"` is already present at `HeroCharacter.cpp:13` (unchanged by me) — the `MaxStepHeight`/`JumpZVelocity` field writes and the `SetWalkableFloorAngle()` call all dereference the complete `UCharacterMovementComponent` type, satisfied by that include. No new include needed.
- **`SetWalkableFloorAngle` vs raw field:** intentionally used the setter, not `MoveComp->WalkableFloorAngle = …`. The runtime walkability test uses the cached `WalkableFloorZ` (cosine); the setter recomputes it, a raw field write would not. This matches the spec bullet.
- **Ctor safety:** `GetCharacterMovement()` is valid in the ctor (ACharacter creates the movement component as a default subobject in its own ctor, before the subclass ctor body). The pre-existing line 45 already dereferences it there; the helper additionally null-checks.
- **Values read 50 / 50 / 600** per the CONVENTIONS M6.6 table (Current 45/44.76/500 → Target 50/50/600).

## Acceptance (deferred to build-master compile at TASK-143)
Compiles warnings-as-errors; three UPROPERTYs = 50/50/600; `ApplyTerrainMovementTuning()` called from ctor + BeginPlay; no template-base file touched; no shadow.
