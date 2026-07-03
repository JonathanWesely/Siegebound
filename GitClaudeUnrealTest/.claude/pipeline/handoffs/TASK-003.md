# TASK-003 Handoff — Hero character (C++)

- author: gameplay-programmer
- date: 2026-07-02
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)

## Files created

1. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h`
2. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`

No template files modified. No other Siegebound files touched (Castle.h/.cpp is being written concurrently by another task).

## Class contract

`AHeroCharacter : public AGitClaudeUnrealTestCharacter, public ITeamAgent` — `UCLASS()` (NOT abstract, so TASK-006's null-safe fallback `DefaultPawnClass = AHeroCharacter` is spawnable). Inherits camera boom + Move/Look/Jump plumbing from the abstract template. `GetTeamId()` returns UPROPERTY `Team` (EditAnywhere, default `ETeamId::Blue`).

## For TASK-009 — exact UPROPERTY slot names on BP_HeroCharacter

All in Category `Input`, all EditAnywhere, all null-safe at runtime:

| Slot | Type | Assign |
|------|------|--------|
| `HeroMappingContext` | `TObjectPtr<UInputMappingContext>` | `/Game/Input/IMC_Hero` |
| `SprintAction` | `TObjectPtr<UInputAction>` | `/Game/Input/Actions/IA_Sprint` |
| `AttackAction` | `TObjectPtr<UInputAction>` | `/Game/Input/Actions/IA_Attack` |

ALSO assign the INHERITED template slots (protected, Category `Input`) or WASD/mouse/jump will not work: `JumpAction` = IA_Jump, `MoveAction` = IA_Move, `LookAction` = IA_Look, `MouseLookAction` = IA_Look (or the template's mouse-look action if one exists — the template binds `MouseLookAction` to its Look handler).

`HeroMappingContext` is added by the character itself in `NotifyControllerChanged()` at priority 1 via the EnhancedInput local-player subsystem (null-safe at every step). Neither the template character nor `ASiegePlayerController` (TASK-007) adds it, so no double-add.

## For TASK-006 — death / reset API

- `FOnHeroDied` — `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroDied, AHeroCharacter*, DeadHero)`
- `UPROPERTY(BlueprintAssignable) FOnHeroDied OnHeroDied;` — broadcast EXACTLY once per death (guarded by `bDead`), after the hero is hidden, collision disabled, movement disabled, and input disabled. Hero knows nothing about respawn timing.
- `UFUNCTION(BlueprintCallable) void ResetHero();` — restores full HP, visibility, collision, `MOVE_Walking` at 500, re-enables input, clears death/cooldown/regen state. Game mode moves/teleports the pawn, then calls this. Call it while the hero is (or is about to be) possessed — `EnableInput` mirrors death's `DisableInput`. If TASK-006 instead destroys + respawns a fresh pawn, that also works (fresh BeginPlay state).
- Queries: `bool IsDead()`, `float GetCurrentHP()`, `float GetMaxHP()` (all BlueprintPure).

## For TASK-007 — melee suppression API

- `UFUNCTION(BlueprintCallable) void SetMeleeSuppressed(bool bSuppressed);`
- `UFUNCTION(BlueprintPure) bool IsMeleeSuppressed() const;`

While suppressed, `DoMeleeAttack()` (bound to IA_Attack Started) is a complete no-op — it does not consume the cooldown, so the swing after leaving placement mode is never rate-limit-blocked by the suppressed click. Set `true` on `EnterPlacementMode`, `false` on exit/cancel.

## Spec numbers (all EditAnywhere UPROPERTYs, GDD §3.1 defaults)

WalkSpeed 500, SprintSpeed 750 (hold IA_Sprint; Started/Completed/Canceled handlers), MeleeDamage 20, MeleeRange 150, MeleeHalfAngleDegrees 30 (60° cone), MeleeCooldown 0.5, MaxHP 200, RegenDelay 8, RegenRate 5 (per second, applied in Tick, stops at max). Regen delay re-arms on taking damage and on a swing that actually damaged >= 1 enemy (a whiff does not re-arm it; a whiff DOES consume the 0.5 s cooldown).

## Implementation notes / QA scrutiny points

1. **Melee target detection**: iterates `UGameplayStatics::GetAllActorsWithInterface(UTeamAgent)`, skips self and same-team (`GetTeamId() == Team`, GDD §3.0 no friendly fire). Range is measured to the CLOSEST POINT on the target's collision via `AActor::ActorGetDistanceToCollision(MyLocation, ECC_Pawn, ClosestPoint)` — necessary because the castle's origin is at the center of an ~800x800 footprint and would never be within 150 units of the hero. ECC_Pawn is blocked by both pawn capsules and default static-mesh collision. If the target has no usable collision (returns < 0, e.g. SM_Castle not yet imported — TASK-002 resolves the mesh null-safe), falls back to actor-origin distance. Cone test is horizontal (yaw plane) against the closest point; a target the hero is overlapping (zero direction) counts as in-cone.
2. **Damage attribution**: `UGameplayStatics::ApplyDamage(Target, 20, GetController(), this, UDamageType::StaticClass())` — hero is both DamageCauser and instigator's pawn, so the castle's team check works via either.
3. **Incoming friendly fire**: `TakeDamage` override ignores damage whose DamageCauser (first) or EventInstigator's pawn (fallback) is a same-team ITeamAgent; unattributable damage (no team on either) APPLIES. Calls `Super::TakeDamage` so `OnTakeAnyDamage` listeners still fire.
4. **Destroyed-castle edge**: a destroyed castle has collision disabled, so it falls into the origin-distance fallback; TASK-002's spec guards its own post-destruction TakeDamage, so this is benign — flagging for awareness.
5. **Template null-action bindings**: `Super::SetupPlayerInputComponent` (template, unmodified per constraints) binds Jump/Move/Look without null checks; EnhancedInput tolerates null-action bindings (they never fire). My Sprint/Attack bindings are explicitly null-checked and log a warning naming the missing asset/task.
6. **Mapping context is not removed on unpossess** — single local player in M1, and re-adding on repossession is idempotent.
7. Cooldown/regen use world-time comparisons (`GetTimeSeconds`), seeded to -1e9 so the first swing is never blocked; no timers to leak; regen runs in `Tick` and is exact (5 * DeltaSeconds, clamped).

## Acceptance mapping (§3.1)

- moves at 500 / 750 sprinting → WalkSpeed/SprintSpeed + StartSprint/StopSprint
- one LMB swing hits every enemy within 150 in the cone for 20 → DoMeleeAttack loop (ALL qualifying targets per swing)
- target at 200 units unaffected → `Distance > MeleeRange` rejects
- one swing per 0.5 s → LastMeleeTime gate
- 150/200 HP untouched 8 s → +5 HP/s in Tick until 200
