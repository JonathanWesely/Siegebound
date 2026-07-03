# QA Report — TASK-003
Verdict: PASS

Reviewed: `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` / `.cpp` against TASKBOARD TASK-003 spec + names block, CONVENTIONS.md, GDD §3.1/§3.0, handoffs/TASK-003.md, template parent `GitClaudeUnrealTestCharacter.h/.cpp` (unmodified — verified), `Siegebound/TeamId.h`, and `Siegebound/Castle.h/.cpp` (contract cross-check).

Blockers: 0. Warnings: 2. Nits: 3.

## Findings

- [WARN] HeroCharacter.cpp:209-216 — `bDealtDamage` is set true whenever `ApplyDamage` is *called*, ignoring its return value. A swing whose only in-cone target refuses the damage (e.g. a destroyed castle's `TakeDamage` returns 0 via its `bDestroyed` guard, reachable through the origin-distance fallback once its collision is off) re-arms the 8 s regen delay even though no damage was dealt — contradicting §3.1 "without ... dealing damage" and the handoff's own whiff rule. Fix: `bDealtDamage |= UGameplayStatics::ApplyDamage(...) > 0.f;`. Not a blocker: unreachable in normal M1 play (only manifests swinging inside a destroyed castle post-victory), no crash, self-corrects 8 s later.
- [WARN] HeroCharacter.cpp:303-323 — `ResetHero()` does not clear `bMeleeSuppressed`. Defensible (suppression is owned by TASK-007's placement mode; force-clearing it while placement is still active would let LMB melee and confirm placement simultaneously), but it creates a contract obligation: TASK-007 MUST call `SetMeleeSuppressed(false)` on every placement-mode exit path, including `HandleMatchEnd` and hero death, or a Play Again from an unexited placement mode leaves the hero permanently unable to melee. Note the asymmetry: if TASK-006 uses destroy+respawn instead of ResetHero, a fresh pawn starts unsuppressed — so TASK-007 exiting placement mode on death/match-end is required for consistency in either respawn strategy. Recorded below for downstream.
- [NIT] HeroCharacter.cpp:318 — `EnableInput(Cast<APlayerController>(GetController()))`: unlike `DisableInput(nullptr)` (pops from all PCs), `EnableInput(nullptr)` is a no-op that logs a `LogActor` Error. Harmless (repossession recreates and pushes a fresh InputComponent in `PawnClientRestart`), but guard with `if (APlayerController* PC = ...)` to avoid a spurious error log when TASK-006 calls ResetHero before possession.
- [NIT] HeroCharacter.cpp:59-79 — `HeroMappingContext` is never removed from the subsystem on unpossess/death. Acceptable for M1 (single local player; `AddMappingContext` re-add is idempotent) and documented in the handoff; revisit if M3+ ever swaps pawns on one controller.
- [NIT] HeroCharacter.cpp:112-123 — sprint held across death: `HandleDeath` resets speed to walk (correct), but after `ResetHero` a still-held Shift stays at 500 until re-pressed (`ETriggerEvent::Started` only fires on a fresh press). Trivial; players re-press naturally.

## Rulings on handoff-flagged interpretations

(a) **Whiff consumes cooldown but does not re-arm regen delay — CORRECT.** §3.1 says regen starts "after 8 s without taking or dealing damage"; a whiff deals no damage, so it must not delay regen. The acceptance line "swings are rate-limited to one per 0.5 s" makes no hit/whiff distinction, so a whiff consuming the cooldown is right. (See WARN 1 for the one refinement: judge "dealt" by ApplyDamage's return, not by the attempt.)

(b) **Closest-point-on-collision range measurement — APPROVED.** Required for the castle (~800×800 footprint, origin at center — a literal 150-unit center check would make the castle unhittable, breaking M1 exit criteria "hero melee also damages it"). The §3.1 acceptance "a target at 200 units is unaffected" cannot be violated for pawns: a pawn 200 units away center-to-center with the template's 42-unit capsule radius has its closest point at ~158 > 150; every M1 pawn is at that scale. ECC_Pawn is the right channel (blocked by pawn capsules and default static-mesh collision); the -1 no-collision sentinel falls back to origin distance, which only triggers for missing-mesh or destroyed castles and is benign (destroyed castle TakeDamage returns 0). Cone test uses the same closest point as the range test — consistent, and the cos(30°) dot gate on the normalized horizontal vector mathematically cannot pass a target behind the hero (dot < 0 < 0.866); the zero-length overlap case correctly counts as in-cone.

(c) **Melee-suppression API for TASK-007 — sound.** `SetMeleeSuppressed(bool)` BlueprintCallable / `IsMeleeSuppressed()` BlueprintPure; suppressed clicks are complete no-ops that do not consume the cooldown, so the first real swing after leaving placement mode is never rate-limit-blocked. Contract gap recorded in WARN 2: suppression survives `ResetHero` but not destroy+respawn — TASK-007 must clear it on every exit path (incl. match end, hero death).

## Verified clean

- **UE 5.8 API:** `NotifyControllerChanged` override with Super call matches the engine-shipped pattern (cf. Variant_Combat/CombatCharacter.cpp:558); EnhancedInput binding mirrors the template's `SetupPlayerInputComponent` after Super (which binds Jump/Move/Look), with null-checked actions + actionable warnings (better than template); `ApplyDamage(Target, Dmg, Controller, Causer, TypeClass)` and `ActorGetDistanceToCollision(Point, Channel, OutClosestPoint)` signatures correct; no deprecated APIs; `UE_KINDA_SMALL_NUMBER` current; Build.cs already lists EnhancedInput.
- **Reflection/GC:** `generated.h` last include; `GITCLAUDEUNREALTEST_API` correct; `TObjectPtr` on all UObject UPROPERTYs; dynamic delegate `FOnHeroDied(AHeroCharacter*)` declared with forward-declared param; no timers to dangle (world-time comparisons only, seeded -1e9 so the first swing/regen is never blocked).
- **Spec numbers:** 500/750 (constructor + BeginPlay seed, Started/Completed/Canceled handlers), 20 dmg, 150 range, ±30° (=60°) cone, 0.5 s cooldown, 200 HP, regen 5 HP/s after 8 s from last damage taken OR dealt, clamped at max, dead heroes don't regen.
- **Death/reset:** single-fire `OnHeroDied` (bDead guard), broadcast after hide + collision/input/movement disable; `ResetHero` restores HP, visibility, collision, MOVE_Walking @ WalkSpeed, input, cooldown/regen state.
- **Friendly fire, both directions:** outgoing skips self + same-team before any damage; incoming ignores same-team causer-first-then-instigator-pawn; unattributable damage applies — behaviorally consistent with ACastle::TryGetInstigatorTeam. Cross-check with TASK-002: hero passes `GetController()` as EventInstigator and `this` as DamageCauser, so the castle resolves the attacker team via either of its first two probes; Blue-on-Blue castle damage is ignored, Red castle takes 20 at 100% melee. Contracts agree.
- **Conventions:** class/file names, Siegebound/ location, and the exact `/Game/Input/IMC_Hero`, `/Game/Input/Actions/IA_Sprint`, `IA_Attack` paths match the names block; template files untouched; UCLASS not abstract (spawnable as TASK-006's fallback pawn).

## Notes for build-master (if PASS)

- Compile-only for this task: no assets exist yet for the input slots; the class must boot raw (it does — every input ref is null-safe and logs which asset/task is missing).
- Nothing to place in-level for TASK-003; the hero enters the scene via TASK-006 (DefaultPawnClass) + TASK-009 (BP_HeroCharacter).

## Notes for downstream tasks

- **TASK-007 (binding):** call `SetMeleeSuppressed(false)` on EVERY placement-mode exit path — confirm, cancel, `HandleMatchEnd`, and hero death — regardless of whether TASK-006 respawns via ResetHero or destroy+respawn (see WARN 2).
- **TASK-006:** prefer calling `ResetHero()` while the hero is possessed (or immediately before repossession); calling it unpossessed works but logs a spurious `EnableInput` error (NIT 3). Existing handoff note for TASK-009 (assign inherited JumpAction/MoveAction/LookAction/MouseLookAction slots) stands.
