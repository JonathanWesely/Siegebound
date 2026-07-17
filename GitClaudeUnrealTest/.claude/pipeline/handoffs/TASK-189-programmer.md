# TASK-189 handoff — Attack/death anim trigger for rigged units (code-only)

**Assignee:** gameplay-programmer
**Status:** ready-for-qa (2026-07-17)
**Follow-up to:** TASK-165 (batch rig integration) + TASK-162 (spike integration) — the A_<CardID>_{Attack,Death} clips were imported but nothing played them.

## What was wrong
Rigged units (`bUsingSkeletalVisual == true`) run only the shared `ABP_Footman` idle/walk locomotion. Their attack feedback was the TASK-020 procedural lunge — which moves the now-**HIDDEN** static `VisualMesh`, so it is invisible on rigged units. Death just `Destroy()`s the actor with no death anim.

## Approach (obeys the hard constraint — NO AnimBlueprint/montage asset touched)
CODE-ONLY, single-node `USkeletalMeshComponent::PlayAnimation(UAnimSequence*, bLooping=false)` on `SkeletalVisualMesh`. That call puts the component into single-node mode and plays the clip directly, **overriding** the locomotion ABP for its duration. No new/edited ABP, no montage slot added via MCP (both froze the editor previously). No asset created or edited.

The attack/death sequences are resolved by **null-safe soft path composed from the CardID** — `/Game/Characters/Anims/A_<CardID>_Attack` and `_Death` — exactly mirroring the existing `SK_<CardID>` / `SM_<CardID>` string law in `ResolveSkeletalVisual`. A clip that does not resolve caches `nullptr` and its trigger no-ops (unit just doesn't play that anim; never a crash). Robust across the roster + future units.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h` (one-line virtual override — economy safety, see below)

## Assets referenced (composed by string, not created)
- `/Game/Characters/Anims/A_<CardID>_Attack` (UAnimSequence)
- `/Game/Characters/Anims/A_<CardID>_Death` (UAnimSequence)
- Locomotion ABP already resolved by `ResolveSkeletalVisual` (`ABP_<CardID>` else shared `ABP_Footman`), now cached for restore.

## Code hook points

### Attack — `ASummonedUnit::PerformAttack()` (SummonedUnit.cpp, the TASK-020 lunge seam)
- **Melee branch:** the lunge is now gated on `bUsingSkeletalVisual`:
  - `if (bUsingSkeletalVisual) PlaySkeletalAttackAnim(); else StartAttackLunge();`
  - Rigged unit → plays `A_<CardID>_Attack`; the invisible lunge is SKIPPED. Static/non-rigged unit → keeps the TASK-020 lunge (byte-unchanged). The impact puff (on `DamageApplied > 0`) is unchanged for both.
- **Ranged branch:** after `FireProjectileAt(...)`, added `PlaySkeletalAttackAnim();` so a rigged Archer/Longbowman visibly animates its shot (e.g. bow draw). No lunge/puff for ranged either way (TASK-028 unchanged). Non-rigged ranged units: no-op.

### Restore-locomotion mechanism — `RestoreLocomotionAnim()`
- `PlaySkeletalAttackAnim()` schedules `AttackAnimRestoreTimerHandle` at `max(clip GetPlayLength(), MinAttackCadence)`. When it fires, `RestoreLocomotionAnim()` calls `SkeletalVisualMesh->SetAnimInstanceClass(LocomotionAnimClass)` — swapping the ABP back so velocity-driven idle/walk resume.
- `LocomotionAnimClass` (`TSubclassOf<UAnimInstance>`) is cached in `ResolveSkeletalVisual` from the same `AnimClass` it already assigns (per-unit `ABP_<CardID>` else shared `ABP_Footman`).
- `RestoreLocomotionAnim()` is ALSO called on **leaving Attack** — `EnterAdvance` and `EnterIdle` (right after `StopAttackLunge()`) — so a rigged unit that stops attacking returns to walk/idle immediately instead of waiting for the timer. And in `FreezeAI()` so a match-end-frozen unit never holds a mid-swing pose.
- Engine detail relied on: `SetAnimInstanceClass` reinitializes when coming OUT of single-node mode and cheaply early-outs when the same ABP is already in blueprint mode — so it is safe to call on every Attack exit (I dropped a mode-check guard because the engine already no-ops the redundant case).

### Death — `ASummonedUnit::HandleDeath()` + `PlaySkeletalDeathAnim()`
- `PlaySkeletalDeathAnim()` plays `A_<CardID>_Death` single-node non-looping (freezes on the final frame = held death pose) and returns `min(GetPlayLength(), DeathAnimMaxHoldSeconds)`; returns 0 when there is no skeletal death clip.
- `HandleDeath` now: `const float DeathHoldSeconds = ShouldHoldDeathAnim() ? PlaySkeletalDeathAnim() : 0.f;` — if > 0, it **freezes the corpse and defers Destroy**:
  - `GetCharacterMovement()->StopMovementImmediately(); DisableMovement();` — MOVE_None so the body does NOT fall (necessary because the collision-disable below removes the floor the CMC stands on).
  - `SetActorEnableCollision(false)` — the corpse never blocks a living unit's path (a dead unit is already skipped by `IsTargetAlive`).
  - `HPBarWidget->SetVisibility(false)` — hides the now-0-HP overhead bar.
  - `SetTimer(DeathDestroyTimerHandle, ... FinishDeathDestroy, DeathHoldSeconds)`.
  - else → immediate `Destroy()`, byte-for-byte the M1..M6 behavior.
- The §6 gold-burst + final `OnHPChanged` broadcast already fired earlier in `HandleDeath`, so the death still reads even on the immediate-destroy path. The skeletal component keeps animating during the hold via its own component tick (independent of actor tick, which stays disabled).

### `DeathAnimMaxHoldSeconds` (new UPROPERTY, EditAnywhere, default **2.0 s**, ClampMin 0)
The "small, capped" destroy-defer window per the spec — a corpse can never linger longer than this even if a clip is long/mis-authored.

## Miner economy safety (the one cross-file edit)
`AMinerUnit::EndPlay(reason == Destroyed)` runs the §3.3 economy bookkeeping (`RemoveMinerIncome` / `UnregisterMinerAlive`). Deferring `Destroy()` would delay that decrement by up to the hold window — a dead miner would keep accruing income and hold its cap-6 slot for ~2 s. To keep miner economics prompt I added a protected virtual:
- base `ASummonedUnit::ShouldHoldDeathAnim() const { return true; }`
- `AMinerUnit` overrides it to `return false;` (inline in MinerUnit.h) → a miner is destroyed immediately on death (no hold), its §6 gold-burst is its death feedback. No invariant is at risk and bookkeeping stays at the same instant as before.

## New members (SummonedUnit.h)
- `TSubclassOf<UAnimInstance> LocomotionAnimClass` (UPROPERTY Transient) — cached ABP for restore.
- `TObjectPtr<UAnimSequence> CachedAttackAnim` / `CachedDeathAnim` (UPROPERTY Transient — GC-alive, mirrors `CachedAttackImpactEffect`).
- `FTimerHandle AttackAnimRestoreTimerHandle` / `DeathDestroyTimerHandle` — both cleared in `EndPlay`; the restore handle also cleared in `HandleDeath`/`FreezeAI`/`PlaySkeletalDeathAnim`.
- `float DeathAnimMaxHoldSeconds = 2.f`.
- Forward decls `class UAnimInstance;` / `class UAnimSequence;`; cpp adds `#include "Animation/AnimSequence.h"` (complete type for `GetPlayLength()` + the `UAnimSequence*`→`UAnimationAsset*` upcast in `PlayAnimation`).
- New helpers: `CacheActionAnimations()`, `PlaySkeletalAttackAnim()`, `RestoreLocomotionAnim()`, `PlaySkeletalDeathAnim()`, `FinishDeathDestroy()`, `ShouldHoldDeathAnim()` (virtual).

## Hero (TASK-162 gap) — NOT changed, flagged as fast follow-up
`AHeroCharacter::DoMeleeAttack()` ALREADY plays an attack anim: `if (AttackMontage) PlayAnimMontage(AttackMontage, 1.f, AttackMontageSection);`. The hero uses the **template Manny + `ABP_Manny_Combat`**, which HAS a montage slot — so the hero's proper, locomotion-blending, montage-based trigger works and is SUPERIOR to the units' single-node override. The single-node `PlayAnimation` approach does NOT cleanly fit the hero: it would override the combat ABP and REGRESS the existing working montage system + lose blending. Per the task's own guidance ("if the hero's mesh setup differs enough to risk it, do the units now and flag the hero"), the hero's remaining work is **asset wiring** (assign `AttackMontage` on `BP_HeroCharacter`, and optionally a death anim) — an art/editor task, NOT this code task. No code gap on the hero.

## Edge cases / what QA should scrutinize
- **Repeated attacks retriggering:** a faster next attack re-arms the SAME `AttackAnimRestoreTimerHandle` and simply restarts the clip via `PlayAnimation` — reads as continuous swings; the ABP only returns once attacks actually stop (or on Attack-exit). No leak (one-shot handle, always re-set or cleared).
- **Attack-move interplay:** leaving Attack (`EnterAdvance`/`EnterIdle`) restores locomotion immediately, so a chase resumes walk animation without waiting on the timer. Attack↔Advance flapping at range-edge causes ABP↔single-node churn — acceptable blockout-tier, no correctness issue (units in stable melee stay in Attack and don't flap).
- **Death-hold timing:** capped at `DeathAnimMaxHoldSeconds` (2 s). Corpse is frozen (MOVE_None, no gravity fall) + non-colliding + bar hidden during the hold; already `bDead` so nothing targets it (`IsTargetAlive`).
- **No-ABP pathological case:** `PlaySkeletalAttackAnim` is gated on `LocomotionAnimClass` being set, so a rig with NO ABP at all never enters single-node (it can't cleanly return) — it keeps its ref pose rather than freezing on an attack frame.
- **KillZ / match-reset:** if something `Destroy()`s the actor during a death hold, `EndPlay` clears both new timers (no dangling). A KillZ fall that routes through `HandleDeath` would hold the death pose while off-world for ≤2 s — cosmetic, minor, flagged.
- **Null-safety:** every trigger no-ops if the skeletal runtime, the clip, or (for attack) the locomotion ABP is absent. No clip resolves synchronously per-attack — both are cached once at `ResolveSkeletalVisual`/`CacheActionAnimations` (no cadence sync-load hitch).
- **Brace-init vexing-parse guard:** all `TSoftObjectPtr<UAnimSequence>` locals in `CacheActionAnimations` use `{ FSoftObjectPath(...) }`.
- **Coding-law checks:** no shadowing of inherited reflected members introduced; complete-type include added for `UAnimSequence` (`Animation/AnimSequence.h`). `USkeletalMeshComponent` / `UAnimInstance` complete types already included in the TU.

## Not done (per instructions)
- No compile, no build, no Git. build-master compiles next.
- No asset created or edited.
