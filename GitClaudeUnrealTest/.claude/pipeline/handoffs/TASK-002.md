# TASK-002 Handoff — Castle actor (C++)

- author: gameplay-programmer
- date: 2026-07-02
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)

## Files created

1. `Source/GitClaudeUnrealTest/Siegebound/Castle.h`
2. `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp`

No other files touched. Include as `#include "Siegebound/Castle.h"` (module root is already in PublicIncludePaths — same pattern as TASK-001).

## Class contract for downstream tasks

`UCLASS() class GITCLAUDEUNREALTEST_API ACastle : public AActor, public ITeamAgent`

### Delegate (TASK-006 GameMode subscribes)

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleDestroyed, ACastle*, DestroyedCastle, ETeamId, CastleTeam);

UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
FOnCastleDestroyed OnCastleDestroyed;
```

- Fires **exactly once** per destruction (private `bDestroyed` guard; TakeDamage early-outs while destroyed, so overkill/duplicate hits cannot re-fire it). `ResetCastle()` re-arms it.
- `CastleTeam` is the team of the castle that FELL — the winner is the other team.
- Broadcast happens AFTER the castle is hidden + collision-disabled, so listeners see consistent destroyed state during the callback.
- GameMode handler signature to bind with `AddDynamic`: `UFUNCTION() void OnCastleDestroyedHandler(ACastle* DestroyedCastle, ETeamId CastleTeam);` (handler must be a UFUNCTION).

### Public API

```cpp
virtual ETeamId GetTeamId() const override;                       // ITeamAgent; returns Team
virtual float TakeDamage(float, FDamageEvent const&, AController*, AActor*) override;
UFUNCTION(BlueprintCallable) void ResetCastle();                  // full HP + visible + collision + re-armed event
UFUNCTION(BlueprintPure) float GetCurrentHP() const;              // [0, MaxHP]
UFUNCTION(BlueprintPure) float GetMaxHP() const;                  // 2000 default
UFUNCTION(BlueprintPure) bool IsCastleDestroyed() const;          // true from destruction until ResetCastle()
```

### Properties

- `EditAnywhere ETeamId Team` (default Blue) — integration sets Castle_Red's to Red per instance.
- `EditAnywhere float MaxHP = 2000` (ClampMin 1). `CurrentHP` is private/Transient, seeded to MaxHP at BeginPlay.
- Root component: `UStaticMeshComponent* CastleMesh` (subobject name "CastleMesh", default collision profile BlockAll — blocks movement and nav).
- Soft refs (EditDefaultsOnly, constructor defaults, resolved via `LoadSynchronous()` in OnConstruction, skip-if-null, never crash):
  - `CastleMeshAsset` -> `/Game/Meshes/SM_Castle.SM_Castle`
  - `TeamMaterialBlue` -> `/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue`
  - `TeamMaterialRed` -> `/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red`
  - Material goes to slot 0 only (SM_Castle is single-slot per TASK-013 spec).

## TakeDamage semantics (QA + TASK-003/004 relevance)

1. Returns 0 and does nothing if already destroyed or DamageAmount <= 0.
2. Friendly-fire check resolves the attacker team in this order: `EventInstigator->GetPawn()` as ITeamAgent, then `DamageCauser` as ITeamAgent, then `DamageCauser->GetInstigator()` as ITeamAgent. Same team => ignored entirely (returns 0, no HP change, no Super call).
   - **Attackers (TASK-003 hero, TASK-004 units): pass yourself as DamageCauser and/or your controller as EventInstigator when calling `UGameplayStatics::ApplyDamage` / `AActor::TakeDamage`, or the no-friendly-fire check cannot see your team.** Unresolvable team (world damage) applies at 100%.
3. Melee applies at 100%. Marked `TODO(M2)` in code for projectile 50% / Siege 200% scaling.
4. At 0 HP: hide actor, disable collision, broadcast OnCastleDestroyed(this, Team) — once.

## Notes for TASK-004 (unit castle targeting)

- Find enemy castle: iterate `ACastle` actors, pick `GetTeamId() != own team`; skip any with `IsCastleDestroyed() == true`.
- Destroyed castles have collision disabled (`SetActorEnableCollision(false)`) — overlap/trace-based acquisition will naturally drop them; distance-based acquisition must check `IsCastleDestroyed()`.

## Notes for integration (build-master)

- Place two instances in L_Arena at the TASK-015 anchors: `Castle_Blue` (Team = Blue, default) at CastleAnchor_Blue, `Castle_Red` (set Team = Red) at CastleAnchor_Red.
- Hide/reset uses actor-level `SetActorHiddenInGame` / `SetActorEnableCollision` — no per-component state to fix up.

## For QA to scrutinize

- `Cast<ITeamAgent>` (native cast) is used for team resolution — valid because UTeamAgent is `NotBlueprintable` (TASK-001), so every implementer is native C++.
- `GetTeamId()` override carries no UFUNCTION macro (UHT forbids re-declaring it on overrides) — matches the interface pattern QA approved in TASK-001.
- `OnConstruction` calls `LoadSynchronous()` every construction run; if the asset is missing it returns nullptr and we skip (mesh assignment is also diffed to avoid redundant `SetStaticMesh` calls). Acceptable for two castle actors; flag if you disagree.
- `Super::TakeDamage` is called only AFTER the friendly-fire check passes, so `OnTakeAnyDamage` Blueprint delegates never see friendly hits. Intentional — flag if the team wants Super called unconditionally.
- `FMath::Max` clamp means CurrentHP lands exactly on 0.0 (never negative); the destroyed branch uses `<= 0.0f`.
- `#include "Engine/DamageEvents.h"` added for the FDamageEvent pass-through (UE5 moved it out of Actor.h).
