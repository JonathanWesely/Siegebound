# TASK-016 Handoff — Hero attack feedback hooks (C++)

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)

## Files touched

1. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` — 4 new UPROPERTYs, forward decls (`UAnimMontage`, `UCameraShakeBase`, `UNiagaraSystem`), `Templates/SubclassOf.h` include, class doc updated.
2. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` — feedback hooks in `DoMeleeAttack`; new includes (`Animation/AnimMontage.h`, `Camera/CameraShakeBase.h`, `NiagaraFunctionLibrary.h`, `NiagaraSystem.h`).
3. `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` — appended `"Niagara"` to `PublicDependencyModuleNames` (this task owns that edit; TASK-020 must NOT touch Build.cs).

No other files changed. No template files touched.

## New UPROPERTYs (all `EditAnywhere, Category = "Combat|Feedback"`, all unset in C++, wired on BP_HeroCharacter in TASK-017)

| Property | Type | TASK-017 wires |
|----------|------|----------------|
| `AttackMontage` | `TObjectPtr<UAnimMontage>` | `/Game/Variant_Combat/Anims/AM_ComboAttack` (or `AM_ChargedAttack`) |
| `AttackMontageSection` | `FName` (default `NAME_None`) | single-swing section name |
| `HitImpactEffect` | `TObjectPtr<UNiagaraSystem>` | `/Game/Variant_Combat/VFX/NS_Damage` |
| `HitCameraShake` | `TSubclassOf<UCameraShakeBase>` | `/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy` |

## Behavior implemented (all inside `DoMeleeAttack`, nothing else altered)

1. **Montage** — plays immediately after the cooldown stamp (`LastMeleeTime = Now`), so it fires on every swing that passes the 0.5 s gate, hit OR whiff. The dead/suppressed early-returns sit above the cooldown gate (unchanged from M1), so a suppressed or dead "swing" never reaches the montage. Uses `PlayAnimMontage(AttackMontage, 1.0f, AttackMontageSection)` — `ACharacter::PlayAnimMontage` itself performs `Montage_JumpToSection` when the section name is not `NAME_None`, and is internally null-safe on a missing anim instance. Guarded by `if (AttackMontage)` anyway (house style).
2. **Impact VFX** — inside the target loop, `ApplyDamage`'s return value is now captured; when `> 0` the effect spawns via `UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitImpactEffect, ClosestPoint)`. `ClosestPoint` is the existing M1 closest-point-on-collision result, which M1 already falls back to `GetActorLocation()` when the target has no usable collision — the spec's fallback comes for free.
3. **Camera shake** — after the loop, if >= 1 target actually took damage and `HitCameraShake` is set: `Cast<APlayerController>(GetController())` then `ClientStartCameraShake(HitCameraShake)`. In M1 the hero's controller IS the local player controller; an unpossessed/AI hero has no PC and silently gets no shake.
4. **Build.cs** — `"Niagara"` added as the last entry of `PublicDependencyModuleNames`.

## Damage-path byte-identity audit

- Early returns (dead, suppressed), cooldown math, `LastMeleeTime` stamp: untouched.
- Target iteration, team filter, closest-point range test, cone test: untouched.
- `ApplyDamage` call: identical arguments; only change is capturing the return value into a local.
- `bDealtDamage = true;` remains UNCONDITIONAL after `ApplyDamage` exactly as M1 — regen re-arm (`LastCombatTime`) behavior is unchanged, including the M1 edge case where a swing on a destroyed castle re-arms regen despite dealing 0.
- Damage is never gated on the montage, notifies, or any feedback state.

## Flagged decisions for QA (please rule on each)

1. **"Actually damaged" = `ApplyDamage(...) > 0.f`, tracked in a NEW local `bAnyEnemyDamaged`, separate from M1's `bDealtDamage`.** Rationale: `ACastle::TakeDamage` and `ASummonedUnit::TakeDamage` both return 0 for ignored hits (destroyed castle, dead unit, friendly fire), so keying VFX/shake on the return value avoids spawning a hit puff on an invisible destroyed castle. Keeping `bDealtDamage` unconditional preserves M1 regen re-arm byte-for-byte. Alternative reading of the spec ("damaged" = passed all filters, i.e. reuse `bDealtDamage` for feedback too) would puff/shake on 0-damage hits. Please confirm the return-value reading.
2. **Montage plays BEFORE the damage loop** (right after the cooldown stamp) rather than after. `Montage_Play` does not synchronously evaluate the pose or move the actor, so this frame's `GetActorLocation()`/facing capture is unaffected; ordering chosen for readability. If QA prefers zero theoretical interaction, moving the montage call after the loop is behavior-equivalent.
3. **Section jump via `PlayAnimMontage`'s `StartSectionName` parameter** instead of a separate explicit `Montage_JumpToSection` call — the engine helper does exactly that internally when the name is set. Spec text says "call PlayAnimMontage(AttackMontage) and jump to AttackMontageSection if set"; I read the built-in parameter as satisfying it.
4. **`BlueprintReadOnly` deliberately OMITTED on the four new UPROPERTYs.** The spec's flags list is exactly `(EditAnywhere, Category "Combat|Feedback")`; the rest of the file uses `EditAnywhere, BlueprintReadOnly`. I followed the spec character-for-character (cross-discipline rule) over house style. Nothing reads these from BP in M1. Say the word and I add the flag.
5. **Shake has no `IsLocalController()` check** — `ClientStartCameraShake` is a client RPC that executes locally in standalone, and M1 is local-only; the null `APlayerController` cast already covers non-player heroes. Multiplayer correctness is M8's problem and the RPC form is already the right one for it.
6. **`HitImpactEffect` is a hard `TObjectPtr`, not a soft ptr** (spec-exact). Note the contrast: TASK-020 specs a `TSoftObjectPtr<UNiagaraSystem>` with a C++ path default on the footman. Both follow their own specs; flagging so nobody "harmonizes" them into a spec violation later.
7. **Niagara added to Public (not Private) dependencies** — matches how every other module is listed in this Build.cs, and TASK-020 will reference `UNiagaraSystem` from another header pair anyway.

## What QA should scrutinize

- The `DoMeleeAttack` diff against the byte-identity requirement (section above) — that is the highest-risk surface.
- Include correctness for the Niagara headers (`NiagaraFunctionLibrary.h`, `NiagaraSystem.h` — plugin-standard include paths; module now in Build.cs).
- Null-safety with NOTHING assigned: montage guard, effect guard, shake guard + PC cast — every new use is behind a check; with all four properties unset the function is behaviorally identical to M1 except for the captured return value.

## For TASK-017 (downstream)

Assign on `/Game/Blueprints/BP_HeroCharacter`, Details category **Combat > Feedback**: `AttackMontage`, `AttackMontageSection` (pick the single-swing section; leave `None` to play from the start), `HitImpactEffect`, `HitCameraShake`. No code hook-up needed beyond the property assignments — behavior activates automatically once non-null.
