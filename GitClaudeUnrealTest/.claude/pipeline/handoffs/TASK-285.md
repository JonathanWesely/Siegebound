# TASK-285 handoff — Unit URO / OnlyTickPoseWhenRendered

**Assignee:** gameplay-programmer
**Branch:** `m7.6-arena10x`
**Status:** ready-for-qa
**Scope:** file-only (no compile, no Git — build-master's TASK-286 owns those)

## What changed

Single file touched: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (constructor,
immediately after the `SkeletalVisualMesh` setup block, was ~line 135).

Two properties set on the cosmetic `SkeletalVisualMesh` (`USkeletalMeshComponent`):

```cpp
SkeletalVisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
SkeletalVisualMesh->bEnableUpdateRateOptimizations = true;
```

- `SummonedUnit.h` — **NOT touched** (listed in the task file-set as the allowed scope, but no
  header change is required for two constructor property sets). Byte-identical.
- **No new include** — `Components/SkeletalMeshComponent.h` is already included (SummonedUnit.cpp:10);
  it defines both `USkeletalMeshComponent::VisibilityBasedAnimTickOption` and the
  `EVisibilityBasedAnimTickOption` enum. `bEnableUpdateRateOptimizations` is inherited from
  `USkinnedMeshComponent` via the same include. Both are public UPROPERTYs, directly settable.
- **`SetVisibleInRayTracing(false)` NOT applied** — that emergency perf lever is explicitly
  reserved for W2/W3 per the spec + CONVENTIONS SK-unit URO law; out of scope here.

Matches CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" SK-unit URO law (CONVENTIONS.md:173):
`VisibilityBasedAnimTickOption=OnlyTickPoseWhenRendered` and `bEnableUpdateRateOptimizations=true`
on `SkeletalVisualMesh`.

## CRITICAL CONFIRMATION — no gameplay-critical logic depends on the per-tick pose off-screen

Read the FULL source (`SummonedUnit.cpp` 1–2451 and `SummonedUnit.h`). Every gameplay-critical path
is driven by the AI/state machine + cadence TIMERS + CharacterMovementComponent — never by the anim
pose. `OnlyTickPoseWhenRendered`/URO throttle ONLY this component's pose evaluation and touch nothing
below. **No critical path reads the pose off-screen → the flags are safe UNCONDITIONALLY; no scoping
/ no regression flag needed.** Evidence (file:line):

1. **Structural decoupling — separate cosmetic component.** `SkeletalVisualMesh` is its OWN
   `CreateDefaultSubobject<USkeletalMeshComponent>` attached to the capsule
   (SummonedUnit.cpp:130-135; declared SummonedUnit.h:341-342), NOT the ACharacter animation `Mesh`.
   It carries `NoCollision`, no overlaps, no nav (`:132-134`). The URO flags therefore affect a pure
   visual attachment.

2. **Movement — CMC + nav MoveTo, never root motion.** Advance issues
   `AAIController::MoveToActor` / `MoveToLocation` (SummonedUnit.cpp:1778, :1784-1785, :1838-1840);
   speed is `UCharacterMovementComponent::MaxWalkSpeed` from the card row (`:960-963`). Grep for
   `RootMotion|AnimNotify|bEnableRootMotion` across the TU → **0 matches**. The locomotion ABP is
   velocity-DRIVEN (reads CMC velocity), not the reverse — CMC ticks independently of the pose, so an
   off-screen unit keeps marching.

3. **Aggro / target acquisition — distance math on the state timer.** `AcquireTarget`
   (`:1104-1175`), `AcquireEnemyNearPoint` (`:1333-1406`), `FindNearestEnemyCastle/Building/…`
   (`:1177`, `:1518`) are pure `GetActorLocation`/distance loops. All invoked from `UpdateState`
   (`:1005`) on `StateTimerHandle` (looping `StateCheckInterval`, armed `:1002`). No pose read.

4. **Attack timing — cadence timer, wall-clock stamped.** `EnterAttack` (`:1707`) arms
   `AttackTimerHandle` → `PerformAttack` looping at the effective cadence (`:1741`); `LastAttackTime`
   is `World->GetTimeSeconds()` (`:1958-1960`). Fully timer-driven.

5. **Attack DAMAGE delivery — applied DIRECTLY, never via AnimNotify (the key check).**
   `PerformAttack` (`:1871`) applies melee via `UGameplayStatics::ApplyDamage` (`:1933`) or ranged via
   `FireProjectileAt` (`:1914`) at the moment the cadence timer fires. `PlaySkeletalAttackAnim`
   (`:1919`, `:1944`) is played AFTER damage lands and is purely cosmetic feedback. There are **no
   AnimNotify hooks** anywhere (grep = 0). So off-screen units deal full damage on schedule even
   with the pose frozen — the exact failure mode the confirmation guards against does NOT exist here.

6. **Support heal delivery — heal timer, not pose.** `PerformHeal` on `HealTimerHandle`
   (`:1655`, `:1665`) → `ApplyHealing` (`:1690`). Timer-driven.

7. **Death / destroy — timer-deferred, not pose-gated.** `PlaySkeletalDeathAnim` (`:396`) is
   cosmetic; the removal is `DeathDestroyTimerHandle` → `FinishDeathDestroy` (`:2381`, `:414`). An
   off-screen death still destroys on schedule; only the death pose may pop.

8. **The TASK-020 attack lunge is unaffected.** It is a procedural offset on the STATIC `VisualMesh`
   via the actor `PrimaryActorTick` (`UpdateLunge`, `:2163`), not the skeletal pose tick — and it
   only runs for non-rigged/static units (`:1942-1949`). `VisibilityBasedAnimTickOption`/URO govern
   the `USkeletalMeshComponent` pose tick only, not `PrimaryActorTick`, so the lunge is untouched.

**Net:** the only observable effect is the intended one — an unrendered unit's visible pose lags /
pops (accepted at the gameplay cam per the spec). March, aggro, target acquisition, attack cadence,
damage, healing, and death timing are unchanged.

## What QA should scrutinize
- Enum spelling `EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered` and the bool
  `bEnableUpdateRateOptimizations` (UE 5.8 names — both public members reachable via the existing
  `Components/SkeletalMeshComponent.h` include; no shadow, no new include).
- Confirm the two lines are the ONLY change and land on `SkeletalVisualMesh` (not `VisualMesh`,
  not the ACharacter `Mesh`).
- Confirm `SetVisibleInRayTracing` is absent (reserved lever).
- Include/shadow scan + the off-screen-pose-dependency confirmation above.

## Out of scope (untouched)
ATTACK/DEFEND/HOLD command logic (TASK-282, committed @ `5fb8058`), LODs (Phase 4), scatter,
`DA_BattlefieldScatter`, `L_Arena.umap`. No compile / no Git (TASK-286).
