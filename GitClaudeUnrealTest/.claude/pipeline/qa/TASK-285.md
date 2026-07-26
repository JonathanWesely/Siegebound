# QA Report — TASK-285

**Task:** [P2] Unit URO / OnlyTickPoseWhenRendered block (C++, branch `m7.6-arena10x`)
**File reviewed:** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (constructor, lines 137–156)
**Handoff:** `.claude/pipeline/handoffs/TASK-285.md`
**Verdict: PASS** — 0 BLOCKER, 0 WARN, 0 NIT

## What changed
Two property sets on the cosmetic `SkeletalVisualMesh` (`USkeletalMeshComponent`), plus a documenting comment block, in the constructor:

```cpp
SkeletalVisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered; // :155
SkeletalVisualMesh->bEnableUpdateRateOptimizations = true;                                                    // :156
```

## Findings
None. All acceptance criteria met.

## Verification detail

### 1. Correct component + UE 5.8 API validity — CONFIRMED
- Both flags land on `SkeletalVisualMesh` (the cosmetic `USkeletalMeshComponent` subobject created at :130, attached to the capsule, NoCollision/no-overlap/no-nav, hidden by default), NOT the inherited ACharacter animation `Mesh`. The decoupling is structural — a separate subobject.
- `EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered` is a valid, non-deprecated enumerator in UE 5.8 (defined alongside `USkinnedMeshComponent`). It is the most aggressive option: pose evaluation is fully skipped while the mesh is unrendered.
- `VisibilityBasedAnimTickOption` (public, on `USkinnedMeshComponent`) and `bEnableUpdateRateOptimizations` (public `uint8:1`, `BlueprintReadWrite`, on `USkinnedMeshComponent`) are both directly assignable. Type of `SkeletalVisualMesh` (`USkeletalMeshComponent` : `USkinnedMeshComponent`) reaches both.
- Include present: `Components/SkeletalMeshComponent.h` at :10 pulls in the component type and the enum. No new include required; no shadowing.

### 2. Load-bearing safety claim — no gameplay-critical path reads the pose off-screen — INDEPENDENTLY CONFIRMED
Grep across the TU (`RootMotion|bEnableRootMotion|AnimNotify|SetVisibleInRayTracing`): **the ONLY hits are inside the new comment block (:149–:154). Zero real code matches for `RootMotion`, `bEnableRootMotion`, or `AnimNotify`.** There are no AnimNotify hooks and no root motion anywhere in `SummonedUnit`.
- **Movement:** CMC `MaxWalkSpeed` + `AAIController` `MoveToActor`/`MoveToLocation` (nav). CMC ticks independently of the pose → off-screen units keep marching. No root motion.
- **Aggro/target:** `AcquireTarget` (:1125) is pure `GetActorLocation`/distance math, driven from `UpdateState` (:1026) on `StateTimerHandle`. No pose read.
- **Attack timing:** `AttackTimerHandle`→`PerformAttack` (:1892), wall-clock stamped via `LastAttackTime = World->GetTimeSeconds()` (:1981). Cadence-driven.
- **Damage delivery — applied DIRECTLY, never via AnimNotify (the key check):** in `PerformAttack`, melee is `UGameplayStatics::ApplyDamage(...)` (:1954) and ranged is `FireProjectileAt(Target, OutputDamage)` (:1935), both executed the instant the cadence timer fires. `PlaySkeletalAttackAnim`/`StartAttackLunge` run AFTER damage lands and are purely cosmetic feedback. An off-screen, pose-frozen unit still deals full damage on schedule.
- **Projectile origin:** `FireProjectileAt` (:2098) spawns at `GetActorLocation()` (:2119) — CMC-driven actor transform, always current off-screen — and passes `DamageAmount` explicitly onto the projectile (:2128). No `GetSocketLocation`/`GetSocketTransform`/`GetBoneLocation` anywhere in the TU, so nothing reads a stale bone/socket. Even the aim re-computes from the target's current location.
- **Heal/death:** heal is `HealTimerHandle`-driven; death removal is `DeathDestroyTimerHandle`→`FinishDeathDestroy` (:435). Timer-deferred, not pose-gated.

Conclusion: the URO/OnlyTickPoseWhenRendered flags throttle ONLY this cosmetic component's pose tick. No off-screen pose dependency exists → the flags are safe unconditionally; no scoping/regression flag needed. The only observable effect is the intended off-screen anim pop, accepted at the gameplay cam per spec.

### 3. `SetVisibleInRayTracing(false)` NOT applied — CONFIRMED
Absent from code — the only occurrence is the comment (:153) documenting that it is the reserved W2/W3 emergency lever, deliberately out of scope. Correct.

### 4. Shadow/include, header, regression, UE 5.8 — CLEAN
- Header `SummonedUnit.h` untouched (grep for the new symbols returns no matches; constructor property sets need no declaration). Byte-identical, as claimed.
- No regression to TASK-282 (the ATTACK/DEFEND/HOLD command path `UpdateStateStandardCommanded`, :1070–:1083) — disjoint (constructor-only change).
- No regression to the TASK-020 lunge: `UpdateLunge` runs off `PrimaryActorTick` on the STATIC `VisualMesh`, a separate mechanism from the skeletal pose tick that `VisibilityBasedAnimTickOption`/URO govern. Untouched.
- UE 5.8 clean: valid symbols, present include, no deprecated APIs.

## Notes for build-master (TASK-286)
- Change is constructor-only in `SummonedUnit.cpp`; `SummonedUnit.h` must show NO diff. Expect `git diff --stat` on this file to be the two property lines + the comment block only.
- Serialization with TASK-282 is satisfied — this change is disjoint from the command path and does not touch it.
- Compile is expected GREEN (no new symbols, no include changes). Editor bounce per the branch grant.
- PIE spot-check: an off-screen unit should still march/acquire/fight on schedule (damage is timer-direct); only its visible pose may pop when it re-enters view. That is the accepted, intended behavior.
