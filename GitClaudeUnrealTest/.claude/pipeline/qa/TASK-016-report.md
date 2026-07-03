# QA Report — TASK-016 — Hero attack feedback hooks (C++)

**Verdict: PASS** (0 blockers, 0 majors, 1 warning, 2 nits)

- reviewer: qa-reviewer
- date: 2026-07-03
- reviewed: `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h`, `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`, `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` against the TASK-016 spec, handoffs/TASK-016.md, and the TASK-003 spec/handoff (M1 byte-identity baseline)

## Findings

### Blockers
None.

### Majors
None.

### Warnings
- [WARN-1] HeroCharacter.cpp:229-243 — VFX/shake keying on `ApplyDamage`'s return value (flagged decision 1) rests on an IMPLICIT receiver contract: every damage receiver's `TakeDamage` must return 0 for ignored hits. **Verified it holds for both current receivers** — `ACastle::TakeDamage` returns 0.0f for destroyed / friendly / non-positive damage (Castle.cpp:126-145) and `ASummonedUnit::TakeDamage` returns 0.f for dead / friendly (SummonedUnit.cpp:479-498) — so behavior is correct today. Risk is forward-looking: any M2+ ITeamAgent receiver that returns nonzero for an ignored hit will puff/shake incorrectly. No code change now; manager should record "TakeDamage returns 0 for every ignored/refused hit" as a receiver contract in CONVENTIONS.md during M2 decomposition.

### Nits
- [NIT-1] HeroCharacter.cpp:157-160 — montage call sits before the damage loop (flagged decision 2). `Montage_Play` does not synchronously evaluate pose, apply root motion, or fire notifies, so this frame's `GetActorLocation()`/facing capture is unaffected — but moving it after the loop would erase even the theoretical interaction. Optional; do NOT respin for this.
- [NIT-2] GitClaudeUnrealTest.uproject — `Modules[0].AdditionalDependencies` lists Engine/AIModule/UMG but not Niagara. That list is an editor hint only, not required for linkage; the Build.cs entry is what matters. No action required.

## Flagged-decision rulings (handoffs/TASK-016.md — all seven ruled)

1. **"Actually damaged" = `ApplyDamage(...) > 0.f` in new local `bAnyEnemyDamaged`, separate from M1's `bDealtDamage` — PASS.** The return-value reading is the correct interpretation of "each enemy actually damaged": I verified both receivers return 0 for every ignored hit (see WARN-1 for citations), so no puff/shake on a destroyed castle or dead unit, while the unconditional `bDealtDamage = true` keeps the M1 regen re-arm byte-identical, including the M1 quirk where a swing on a destroyed castle re-arms regen. The alternative (reuse `bDealtDamage` for feedback) would puff invisible destroyed castles — rejected.
2. **Montage before the damage loop — ACCEPTED.** Behavior-equivalent (see NIT-1). No change required.
3. **Section jump via `PlayAnimMontage`'s `StartSectionName` parameter — PASS.** `ACharacter::PlayAnimMontage` calls `Montage_JumpToSection(StartSectionName, AnimMontage)` internally when the name is not `NAME_None`, and is null-safe on a missing mesh/anim instance. This satisfies the spec text exactly; a separate explicit call would be redundant.
4. **`BlueprintReadOnly` omitted on the four new UPROPERTYs — ACCEPTED.** The spec's flag list is exactly `(EditAnywhere, Category "Combat|Feedback")` and the cross-discipline rule makes the spec character-for-character binding. Nothing reads these from BP in M1; TASK-017 only needs EditAnywhere for the Details panel. Do not add the flag without a spec change.
5. **No `IsLocalController()` check on the shake — ACCEPTED.** `ClientStartCameraShake` executes locally in standalone, M1 is local-only, and the guarded `Cast<APlayerController>` already no-ops for AI/unpossessed heroes. The client-RPC form is also the correct one for M8. Revisit at M8, not before.
6. **`HitImpactEffect` as hard `TObjectPtr`, not soft — ACCEPTED.** Spec-exact for TASK-016. The contrast with TASK-020's `TSoftObjectPtr<UNiagaraSystem>` is each task following its own spec; recorded here so nobody "harmonizes" them later. Hard ref means NS_Damage loads with BP_HeroCharacter once TASK-017 wires it — acceptable at blockout scale.
7. **`"Niagara"` in Public (not Private) dependency modules — ACCEPTED.** Matches how every other module is listed in this Build.cs, and TASK-020 will use `UNiagaraSystem` from SummonedUnit.h as well, which needs public visibility anyway.

## Byte-identity audit — damage path vs M1 (highest-risk surface)

Verified against the TASK-003 spec (20 dmg, 150 units, ±30° cone, 0.5 s cooldown, suppression, regen re-arm) and handoffs/TASK-003.md:

- Early returns `bDead || bMeleeSuppressed` BEFORE the cooldown gate — suppressed swing consumes no cooldown and (new) never reaches the montage. Unchanged.
- Cooldown: `(Now - LastMeleeTime) < MeleeCooldown` reject, then stamp — unchanged.
- Facing flatten + `IsNearlyZero` fallback, `MinCosAngle` from `MeleeHalfAngleDegrees` (30) — unchanged.
- Target set (`GetAllActorsWithInterface(UTeamAgent)`), self-skip, `IsValid`, same-team skip — unchanged.
- Range via `ActorGetDistanceToCollision(MyLocation, ECC_Pawn, ClosestPoint)` with actor-origin fallback on < 0; `Distance > MeleeRange` reject — unchanged. The spec's VFX fallback location comes free from the existing M1 fallback, as claimed.
- Cone test on horizontal `ToTarget` with overlap-counts-as-in-cone — unchanged.
- `ApplyDamage(Target, MeleeDamage, GetController(), this, UDamageType::StaticClass())` — identical arguments; only the return value is now captured into a local (no behavioral effect).
- `bDealtDamage = true` UNCONDITIONAL after ApplyDamage; `if (bDealtDamage) LastCombatTime = Now;` — regen re-arm byte-identical, M1 destroyed-castle quirk preserved.
- No feedback state (montage/VFX/shake/`bAnyEnemyDamaged`) feeds any damage condition — feedback is purely additive. Confirmed.
- No other function in HeroCharacter.h/.cpp was altered beyond the 4 UPROPERTYs, forward decls, includes, and doc comments.

## Null-safety with all four properties unset (first-compile state, pre-TASK-017)

- `AttackMontage` null → montage block skipped (and `PlayAnimMontage` is itself null-safe for the raw-C++ fallback pawn with no anim instance).
- `HitImpactEffect` null → no spawn; `SpawnSystemAtLocation` never called with null system.
- `HitCameraShake` null → shake block skipped; `Cast<APlayerController>` guard additionally covers unpossessed/AI heroes.
- `AttackMontageSection` = NAME_None only ever used inside the `AttackMontage` guard.
- `World` validated by the existing early return before any new use.
- With all four unset, `DoMeleeAttack` is behaviorally identical to M1 except a discarded local. Crash-free. PASS.

## Compile-risk review (UE 5.8, nothing compiled yet)

- Includes correct: `Animation/AnimMontage.h`, `Camera/CameraShakeBase.h`, `NiagaraFunctionLibrary.h`, `NiagaraSystem.h` (plugin-standard flat paths, valid once the Niagara module is a dependency); header adds `Templates/SubclassOf.h` + forward decls `UAnimMontage`/`UCameraShakeBase`/`UNiagaraSystem` — all sufficient for the TObjectPtr/TSubclassOf UPROPERTYs.
- Build.cs: `"Niagara"` appended to `PublicDependencyModuleNames` — correct module name; Niagara plugin is enabled by default in UE 5.x, so no .uproject edit is needed.
- No deprecated APIs: `PlayAnimMontage`, `ClientStartCameraShake` (the post-4.26 API), and `UNiagaraFunctionLibrary::SpawnSystemAtLocation` are all current in 5.8. `TObjectPtr` used per conventions.
- UPROPERTY specifiers and names (`AttackMontage`, `AttackMontageSection`, `HitImpactEffect`, `HitCameraShake`, Category "Combat|Feedback") are character-for-character the spec's `names:` block.

## Notes for build-master

1. TASK-016 owns the Build.cs Niagara edit and it has landed — TASK-020 must NOT touch Build.cs (it is serialized behind this task for exactly that file).
2. No .uproject change needed for Niagara (default-enabled plugin). NIT-2 is informational only.
3. **Working-tree caution:** Castle.cpp/SummonedUnit.cpp already contain TASK-018/TASK-020 in-progress changes (e.g., `OnCastleHPChanged`, `HPBarWidget`). A module compile for TASK-016 will compile those too — sequence integration so unreviewed in-progress work is not swept into the TASK-016 commit.
4. For TASK-017 (downstream): choose an `AttackMontageSection` that reads as ONE swing inside the 0.5 s cooldown; a wrong/missing section name is safe (engine warning, montage plays from start). Assignment surface is documented at the bottom of handoffs/TASK-016.md.
