# TASK-042 Handoff — Hero Rally ability + unit move-speed buff API (C++)

- author: gameplay-programmer
- date: 2026-07-04
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)
- milestone: M3 (bot opponent), wave 1, on `main`

## Files touched (2 pairs, 4 files)

1. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — `ApplyMoveSpeedBuff(float, float)` public BlueprintCallable; private `EndMoveSpeedBuff()`; 2 members + 1 timer handle; FreezeAI doc note.
2. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — `ApplyMoveSpeedBuff` / `EndMoveSpeedBuff` bodies; `EndMoveSpeedBuff()` call added to `FreezeAI`; buff-timer clear added to `EndPlay`.
3. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` — `FOnRallyStateChanged` delegate; `OnRallyStateChanged` member; `Rally()` public BlueprintCallable; `RallyAction` input slot; 4 tuning UPROPERTYs; `OnRallyReady()`; `LastRallyTime` + `RallyCooldownTimerHandle`.
4. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` — `Rally()` / `OnRallyReady()` bodies; Rally reset in `ResetHero`; 2 new includes (`Siegebound/SummonedUnit.h`, `TimerManager.h`).

**NOT touched (deliberate):** `MinerUnit.h/.cpp`, `SiegePlayerState`, `SiegeGameState`, `SiegeGameMode` (TASK-043's files — untouched). No `Build.cs` change needed (Niagara/Enhanced-Input deps already present; no new module used). No TASKBOARD edit (orchestrator owns the board). No compile, no Git.

## API added

### ASummonedUnit::ApplyMoveSpeedBuff(float Multiplier, float Duration) — BlueprintCallable
- Applies a temporary `MaxWalkSpeed = Base × Multiplier` and restores `Base` after `Duration` via a one-shot timer (`MoveSpeedBuffTimerHandle` → `EndMoveSpeedBuff`).
- **Drift-free (the TASK-020 lesson):** the resting base speed is captured **once per buff episode** — only when transitioning from inactive→active (`if (!bMoveSpeedBuffActive)`). A refresh while active reuses the stored `MoveSpeedBuffBaseSpeed`; it never recaptures the already-buffed speed. The walk speed is only ever written as `Base × Multiplier` or exactly `Base`, so no number of re-applies can drift the base.
- **Refresh, no stacking:** re-applying reuses the single one-shot handle (SetTimer clears the prior timer first) and recomputes from the stored base — same-magnitude boost, fresh duration.
- No-op on `bDead` / `bAIFrozen`; null-safe with no movement component. Non-positive `Duration` restores immediately (defensive; Rally passes 5 s).

### ASummonedUnit::FreezeAI (extended)
- Added `EndMoveSpeedBuff()` after the existing `StopAttackLunge()`: clears the buff timer and restores base speed **exactly** → zero residual walk speed at match end. Inherited by `AMinerUnit` (its override calls `Super::FreezeAI()`, verified line 162 of MinerUnit.cpp — so miners get the same cleanup without touching that file).

### ASummonedUnit::EndPlay (extended)
- Added `ClearTimer(MoveSpeedBuffTimerHandle)` alongside the existing two clears — no buff-restore fires on a destroyed unit (belt-and-suspenders; UE also auto-clears object-bound timers on destroy).

### AHeroCharacter::Rally() — BlueprintCallable (bound to IA_Rally in TASK-048)
- Cooldown gate via `LastRallyTime` (seeded `-1e9`, first use always allowed). Off cooldown: iterates `GetAllActorsOfClass(ASummonedUnit)`, skips dead / enemy-team units, applies `ApplyMoveSpeedBuff(1 + RallySpeedBonus, RallyDuration)` to friendlies within `RallyRadius` (center-to-center `DistSquared`). Starts `RallyCooldown`, broadcasts `OnRallyStateChanged(false, RallyCooldown)`; `OnRallyReady` broadcasts `(true, 0)` when it expires.
- **Does nothing to the hero's own speed** — the hero is not an `ASummonedUnit`, so `GetAllActorsOfClass(ASummonedUnit)` never returns it. **Enemy units unaffected** — the `GetTeamId() != Team` check drops them.
- A press on cooldown is a no-op that emits the optional refusal broadcast `OnRallyStateChanged(false, remaining)`. No-op while dead.
- `ResetHero` clears the cooldown timer, resets `LastRallyTime`, and broadcasts `(true, 0)` so a respawned hero can Rally immediately (mirrors the existing melee/regen reset in the same function).

### Delegate — FOnRallyStateChanged / OnRallyStateChanged
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRallyStateChanged, bool, bReady, float, CooldownRemaining)`, `UPROPERTY(BlueprintAssignable)` member `OnRallyStateChanged`. Names are spec/`names:`-block exact (this overrides the strict `FOn<Owner><Event>` house pattern per the cross-discipline rule). HUD binds in TASK-050.

## UPROPERTY defaults (all `// GDD §4`, on AHeroCharacter, Category `Siegebound|Combat`)
| Property | Default |
|----------|---------|
| `RallyRadius` | `600.f` |
| `RallySpeedBonus` | `0.25f` |
| `RallyDuration` | `5.f` |
| `RallyCooldown` | `20.f` |

Plus `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> RallyAction` — null, wired in TASK-048.

## Acceptance mapping
- **Speeds every friendly unit within 600 by 25% for 5 s then restores EXACTLY** — `ApplyMoveSpeedBuff(1.25, 5)`; restore writes back the captured base verbatim.
- **Does nothing to the hero** — hero excluded by class filter; Rally touches only unit movement.
- **Unusable for 20 s** — `LastRallyTime` gate; presses inside the window early-out.
- **Delegate reports cooldown state** — `(false, RallyCooldown)` on use, `(true, 0)` on ready, `(false, remaining)` on a refused press.
- **Enemy units unaffected** — team filter.
- **FreezeAI cancels an active buff cleanly, zero residual** — `EndMoveSpeedBuff()` in FreezeAI restores base exactly and clears the timer.

## C4458 shadow scan (self-check, per the CONVENTIONS coding law)
No local/param/loop var shadows an inherited reflected UPROPERTY. Rally locals: `World, Now, SinceLastRally, CooldownRemaining, MyLocation, RallyRadiusSquared, SpeedMultiplier, UnitActors, UnitActor, FriendlyUnit` (loop var deliberately `FriendlyUnit`, not `Owner`/`Instigator`/etc.). Buff params `Multiplier, Duration` and local `Movement` collide with nothing inherited (`Movement` is already used safely elsewhere in this file).

## Flagged decisions for QA (please rule)
1. **Miners are buffed.** `AMinerUnit : ASummonedUnit`, so `GetAllActorsOfClass(ASummonedUnit)` includes friendly miners within range. The spec says "every friendly same-team ASummonedUnit"; miners qualify literally. Effect is a harmless temporary walk-speed boost to a gold-node walk, restored exactly. If Rally should exclude miners, add an `IsA<AMinerUnit>` skip — but that would need a MinerUnit include and is arguably out of scope. Left inclusive per the literal spec.
2. **Range = center-to-center `DistSquared`,** not the melee's closest-point-on-collision. Units are small pawn capsules; the spec's "within 600" reads naturally as center distance. (Closest-point matters only for the castle's ~800×800 footprint, which Rally never targets.)
3. **`ResetHero` now resets Rally** (clear timer, reset `LastRallyTime`, broadcast ready). Additive, mirrors the existing melee/regen reset lines; not in the literal acceptance list but keeps a respawned hero's Rally usable and the HUD consistent. Flagging since it touches M1 `ResetHero`.
4. **Optional refusal broadcast implemented** — a press on cooldown emits `OnRallyStateChanged(false, remaining)` (the spec calls it optional). Harmless with no listener; lets the HUD flash "not ready." Remove the two-line branch if QA prefers a silent no-op.
5. **UPROPERTY flags include `BlueprintReadOnly` + `ClampMin="0"`** (house style, matching the sibling Melee* floats in this class) rather than spec-bare. The `// GDD §4` comment is present per the design ruling.

## What QA should scrutinize
- **Zero-drift argument:** every write to a unit's `MaxWalkSpeed` in the buff path is `Base × Multiplier` or exactly `Base`; base captured only on inactive→active; restore is exact. Verify a refresh cannot recapture a buffed value.
- **FreezeAI path:** `EndMoveSpeedBuff()` runs on freeze; MinerUnit's override calls `Super::FreezeAI()` so the cleanup reaches miners too.
- **No M1/M2 regressions:** melee/lunge, ranged, sprint (hero `StartSprint`/`StopSprint` still own `MaxWalkSpeed` on the hero — Rally never touches the hero's movement, so sprint is untouched), FreezeAI's existing contract, and the Damage/state-machine paths are byte-identical apart from the additive lines above.
- **Include correctness:** `Siegebound/SummonedUnit.h` + `TimerManager.h` added to HeroCharacter.cpp; `SummonedUnit.cpp` already had `CharacterMovementComponent.h` + `TimerManager.h`.

## For integration / downstream
- **TASK-048** wires `RallyAction` to IA_Rally (key Q) at the same bind site as IA_Sprint/IA_Attack in `SetupPlayerInputComponent`, calling `Rally()` on `ETriggerEvent::Started`. `RallyAction` is null-safe until then; add a Warning-log branch mirroring Sprint/Attack if desired.
- **TASK-050** binds `OnRallyStateChanged` on the HUD for the Rally readiness indicator (seed from state, then bind — CONVENTIONS delegate law).
- Defaults are live once compiled; no editor work required for the C++ behavior itself.

## Rally input binding (build-fix) — 2026-07-04

TASK-042 added `RallyAction` + `Rally()` but never bound the input, so Q→Rally() did nothing. Closed the gap in `HeroCharacter.cpp` only.

- **File:** `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`, in `SetupPlayerInputComponent`, inside the existing `Cast<UEnhancedInputComponent>` scope, directly **after** the `AttackAction` block.
- **Exact line added:** `EnhancedInputComponent->BindAction(RallyAction, ETriggerEvent::Started, this, &AHeroCharacter::Rally);`
- Wrapped in the same `if (RallyAction)` null-guard as Attack, with a matching `else` Warning log (RallyAction unassigned until BP_HeroCharacter is wired in TASK-048). `ETriggerEvent::Started` = press, matching AttackAction.
- **Signature check:** `Rally()` is a no-arg `UFUNCTION(BlueprintCallable)` (HeroCharacter.h:135) — compatible with `UEnhancedInputComponent::BindAction`'s member-function overload (no bound params). No shadow locals introduced (no C4458 risk).
- **NOT touched:** Sprint/Attack bindings unchanged; `RallyAction` UPROPERTY unchanged; no `.h` edit (no new include/forward decl needed). No compile, no Git, no TASKBOARD edit.
- Supersedes the "add a Warning-log branch mirroring Sprint/Attack if desired" note in the TASK-048 line above — that branch is now in place; TASK-048 only needs to author the IA_Rally asset + IMC_Hero (key Q) mapping and assign `RallyAction` on BP_HeroCharacter.
