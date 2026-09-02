// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/LadderClimber.h" // TASK-777 (CONTACT-§4.4): ILadderClimber is a BASE of the UCLASS below — complete type required here, ⛔ not a forward declaration. It is how AClimbableTower::EndPlay aborts a climber it now holds as an ACharacter. ⭐ TASK-787 (CONTACT-§12.3): it ALSO carries FSiegeLadderClimbEnded now, which moved here from this file — so the by-value UPROPERTY below still has its complete type and ⛔ no include was added anywhere
#include "Siegebound/SiegeLadderClimbStatics.h" // TASK-776 (CONTACT-§2): FSiegeLadderClimbState is a BY-VALUE member (LadderClimb) and FSiegeLadderClimbStatics is named by this header's doc comments — complete type required here, not a forward declaration. The types MOVED out of this header unchanged; ⛔ nothing about the driver below changed with them
#include "Siegebound/SiegeStuckStatics.h" // TASK-532: FSiegeStuckState / FSiegeStuckTuning are BY-VALUE members and ESiegeStuckAction is a parameter type — complete types required here, not a forward declaration
#include "Siegebound/TeamId.h"
#include "SummonedUnit.generated.h"

class AAIController;
class ACastle;
class AClimbableTower; // TASK-784 (CONTACT-§4.1): the contact trigger's OWNER. ⭐ A forward declaration is ALL this header needs, and that is the shape of the whole feature — the unit stores ⛔ nothing about a tower and only ASKS the nearest one, per poll, from the .cpp
class AProjectile;
class ASiegePlayerController;
//~ TASK-787: the self-forward-declaration that used to sit here is GONE WITH ITS REASON. It read
//~ *"named by FSiegeLadderClimbEnded below, which UHT parses before the UCLASS"* — `CONTACT-§4.4`
//~ retyped that parameter to `ACharacter*` and `CONTACT-§12.3` moved the delegate to
//~ `LadderClimber.h`, so ⛔ nothing above the UCLASS names this class any more. ⚖️ A declaration
//~ kept after its stated reason expires is exactly the `CONTACT-§10.1` defect, one line long.
class UAnimInstance;
class UAnimSequence;
class UDataTable;
class UCombatantHealthBarComponent;
class UMeshComponent;
class UNiagaraSystem;
class USiegeHitFlashComponent;
class USiegeMeshJuiceComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
struct FSiegeUnitGroup;

/**
 *  Current step of the summoned-unit state machine (GDD §3.8:
 *  Advance → Acquire → Attack → Reacquire). Acquire and Reacquire are the
 *  periodic decisions made on every state check rather than persistent
 *  states, so only the persistent modes appear here.
 */
UENUM(BlueprintType)
enum class ESummonedUnitState : uint8
{
	/** No card stats bound, or no standing enemy castle: standing by. */
	Idle,
	/** Moving toward the acquired target — or toward the enemy castle when none. */
	Advance,
	/** Acquired target within Range: dealing Damage (melee) or firing a homing projectile (bRanged rows) every Cadence seconds. */
	Attack
};

/**
 *  Why an in-flight ladder climb ended (TASK-738). Passed to the ONE teardown so the log can
 *  say which path fired; the teardown itself is deliberately REASON-AGNOSTIC — every reason
 *  restores the movement mode and broadcasts exactly once, which is the property that makes a
 *  future ninth exit safe by construction rather than by review.
 *
 *  ⭐ THE MAPPING ONTO TOWER-§8.5's EIGHT EXITS, STATED SO NOBODY HAS TO COUNT:
 *    Arrival        — exit 1, the deck is reached (the ONLY one with bReachedTop = true)
 *    Abort          — exit 2 (AbortLadderClimb) ⭐ AND exit 8, the TOWER destroyed mid-climb:
 *                     AClimbableTower::EndPlay calls the public abort (TOWER-§10 L-5), which
 *                     is this same reason. ⛔ It is NOT a separate value, because
 *                     AbortLadderClimb()'s signature is PINNED and takes no reason — inventing
 *                     an enumerator nothing can produce would be dead surface pretending to be
 *                     coverage. ASummonedUnit::EndPlay is the independent belt for exit 8.
 *    NewOrder       — exit 3 (TOWER-§10 L-4: a new order aborts the climb and the unit DROPS)
 *    Death          — exit 4 (HandleDeath)
 *    MatchEndFreeze — exit 5 (FreezeAI)
 *    SpellFreeze    — exit 6 (ApplyFreeze)
 *    EndPlay        — exit 7 (the unit itself is torn down)
 *    Timeout        — ⚠️ THE DECLARED NINTH, ⛔ NOT ONE OF THE LAW'S EIGHT. A watchdog: a
 *                     climber that has not arrived after TimeoutScale × its own expected
 *                     duration ends anyway and drops. It exists because "hangs in mid-air
 *                     forever" is this feature's named catastrophic failure, and geometry that
 *                     blocks the sweep would otherwise produce it with every exit correct.
 *
 *  Plain enum, ⛔ NOT a UENUM: it is never a UPROPERTY, and an unreflected type cannot be
 *  replicated by accident — the FSiegeStuckState / ESiegeStuckAction discipline (NAV-§8/§11)
 *  applied to this batch's M8 declaration.
 */
enum class ESiegeLadderExit : uint8
{
	Arrival,
	Abort,
	NewOrder,
	Death,
	MatchEndFreeze,
	SpellFreeze,
	EndPlay,
	Timeout
};

//~ ⚖️⭐ `FSiegeLadderClimbEnded` **MOVED OUT OF THIS FILE** on 2026-09-02 (TASK-787,
//~ `CONTACT-§12.3`) — it now lives with the interface it belongs to, in
//~ `Siegebound/LadderClimber.h`, which this header already includes (see the include note above).
//~ ⛔ A MOVE, ⛔ NOT AN AMENDMENT: the TYPE NAME and the SIGNATURE are UNCHANGED
//~ (`ACharacter*, Climber, bool, bReachedTop`), so `TOWER-§8.4(B)`'s second amendment is INTACT and
//~ the `Broadcast(this, bReachedTop)` in this class's teardown compiles UNTOUCHED. ⭐ WHY: the
//~ delegate is CLIMBER vocabulary, ⛔ not UNIT vocabulary — it only lived here because the unit was
//~ the only climber, and `AClimbableTower` must now reach `AHeroCharacter` through the same type.
//~ ⚠️ The full doc comment moved WITH it rather than being duplicated here; ⛔ this breadcrumb is
//~ deliberately not a second copy of it (a record that can drift is worse than a pointer).

/**
 *  Siegebound summoned unit — Standard, Siege, and Support targeting profiles
 *  (GDD §3.8 + §4). The profile is bound from the card row and UpdateState
 *  dispatches on it: Standard is the M1/M2 body below (unchanged), Siege and
 *  Support are the M4 additions (TASK-054).
 *
 *  ACharacter driven by an ASiegeUnitAIController (AutoPossessAI
 *  PlacedInWorldOrSpawned) walking the navmesh via MoveToActor. The controller
 *  is behaviorally a plain AAIController — its sole addition is the TASK-349
 *  team nav-filter setter (SiegeNavAreas.h one-home), pushed from
 *  ApplyTeamGatingProfile at this unit's team-set sites.
 *
 *  - Stats (HP/Damage/Range/Cadence/Speed) are bound at BeginPlay from the
 *    /Game/Data/DT_Cards row named CardID — NEVER hardcoded (GDD §3.0). A
 *    missing table or row is logged and the unit idles.
 *  - State machine runs on a ~0.25 s timer (not per-tick):
 *      Advance   — MoveToActor toward the nearest standing enemy castle.
 *      Acquire   — nearest alive enemy ITeamAgent within AggroRadius (600);
 *                  if the best unit/hero stands within TieBreakDistance (100)
 *                  of the nearest building/castle, the unit/hero is preferred.
 *      Attack    — within Range: apply Damage every Cadence, with this unit
 *                  as DamageCauser and its controller as EventInstigator so
 *                  receivers can attribute the team (TASK-002 castle contract).
 *      Reacquire — target dead/destroyed or beyond LeashRange (900): resume
 *                  Advance.
 *  - No friendly fire in either direction (GDD §3.0); incoming TakeDamage
 *    mirrors ACastle's instigator-chain team resolution.
 *  - Destructible: at 0 HP the actor is destroyed (units don't respawn).
 *  - VisualMesh (static mesh on the capsule) is intentionally left without a
 *    mesh in C++ — the blueprint child BP_Unit_<CardID> assigns it (TASK-010).
 *  - Attack feedback (TASK-020, blockout tier — no skeletal rig until M7):
 *    each cadence hit runs ONE lunge cycle on VisualMesh — relative location
 *    driven along the capsule's LOCAL +X (actor forward; relative location is
 *    expressed in the parent capsule's axes, so the mesh's own -90° import-fix
 *    yaw per handoffs/TASK-014.md is irrelevant) out AttackLungeDistance and
 *    back, sine-eased, over AttackLungeDuration clamped to 0.8 × Cadence. The
 *    BP-authored rest pose is cached once at BeginPlay and restored EXACTLY at
 *    cycle end, on leaving Attack, and on death — zero drift. Each landed hit
 *    also spawns AttackImpactEffect at the contact point (closest point on the
 *    target's collision; fallback: target location). Purely visual — damage
 *    numbers/timing are untouched.
 *  - Ranged delivery (TASK-028): rows with bRanged true (Archer) run the SAME
 *    state machine (aggro 600, leash 900, Range gate from the row — Archer
 *    700), but each cadence hit SPAWNS a homing AProjectile at the unit
 *    (InitProjectile: own team, current target, row Damage,
 *    USiegeDamageType_Projectile — the castle halves projectile damage on ITS
 *    side, GDD §3.0/TASK-026) instead of applying melee damage. NO lunge and
 *    NO melee impact puff for ranged attacks — the projectile and its own
 *    impact VFX are the telegraph (§3.8). Melee rows (Footman/Knight) run the
 *    M1/TASK-020 path unchanged.
 *  - FreezeAI (TASK-028, the match-end freeze contract consumed by TASK-024):
 *    permanently parks the unit — timers cleared, movement stopped, any
 *    in-flight lunge cancelled to the exact rest pose, Idle until destroyed.
 *    Inherited by AMinerUnit (TASK-025) — the base also stops its walk. Extended
 *    in TASK-054 to also stop the Support heal timer and drop the heal target.
 *  - Targeting profiles (TASK-054, GDD §3.8), dispatched at the top of
 *    UpdateState on the row-bound Profile:
 *      Standard — the M1 melee / M2 ranged body below (unchanged). None (miners,
 *        whose combat machine is sealed by AMinerUnit) also runs this body.
 *      Siege (Ogre, Sapper) — ignores units and the hero entirely; advances on
 *        the nearest enemy ABuilding, else the enemy castle, and tags its melee
 *        with USiegeDamageType_Siege (ACastle/ABuilding scale it to 200%).
 *      Support (Cleric) — NEVER attacks; follows the nearest friendly and heals
 *        the nearest DAMAGED friendly ASummonedUnit within Range at row Damage
 *        HP/s (clamped to MaxHP, friendlies only) on a dedicated heal timer.
 *
 *  - Ladder climb (TASK-738, TOWER-§8/§9/§10): BeginLadderClimb drives a scripted
 *    MOVE_Flying ascent along one straight world line at LadderClimbSpeedUU, called
 *    by AClimbableTower's nav link. While IsClimbing() the unit is ATTACKABLE exactly
 *    as before (nothing narrows acquisition) but CANNOT ATTACK — Jonathan's ruling,
 *    enforced as a second term at the three shipped CanEverAttack() guard points, and
 *    released the instant the deck is reached. Every exit restores the movement mode
 *    and broadcasts OnLadderClimbEnded exactly once, because MOVE_Flying ignores
 *    gravity and a missed restore hangs the unit in mid-air forever.
 *
 *  Spawners (TASK-007): prefer SpawnActorDeferred → InitUnit(Team, CardID) →
 *  FinishSpawning, so BeginPlay binds the right card. InitUnit also works
 *  after a plain SpawnActor (it late-binds the stats if BeginPlay found no CardID).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASummonedUnit : public ACharacter, public ITeamAgent, public IHealthBarProvider, public ILadderClimber
{
	GENERATED_BODY()

public:

	ASummonedUnit();

	//~ Begin ITeamAgent Interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent Interface

	/** Fired on every ACTUAL HP change (spawn-init, damage, heal, death) — drives the overhead bar (UCombatantHealthBarWidget) via the castle-parity PUSH model (TASK-130, mirrors FOnCastleHPChanged). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Unit")
	FOnCombatantHPChanged OnHPChanged;

	/**
	 *  ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — fired on every ACTUAL change of the
	 *  permanent damage boost, driving the boost ROW of the overhead bar on the exact same
	 *  PUSH model as OnHPChanged. Payload is the boost PERCENT (0 .. 400), i.e.
	 *  100 × PermanentDamageBonusPerStack × PermanentDamageStacks — NOT the multiplier, so
	 *  the widget's banding math never has to un-shift a 1.0 base.
	 *
	 *  ⚠️ BROADCAST ON *EVERY* MUTATION — the same discipline as OnHPChanged's four sites.
	 *  Miss one and the bar is stale forever (the qa/TASK-005 seed-then-bind trap). The two
	 *  mutators (AddPermanentDamageStacks / ClearPermanentDamageStacks) are the ONLY writers
	 *  of PermanentDamageStacks, which is what makes "every mutation" auditable.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Unit")
	FOnCombatantDamageBoostChanged OnDamageBoostChanged;

	//~ Begin IHealthBarProvider Interface (TASK-130 push model) — forwards to the EXISTING getters + the OnHPChanged delegate; adds NO HP state.
	virtual FOnCombatantHPChanged& GetHPChangedDelegate() override { return OnHPChanged; }
	virtual float GetHealthCurrent() const override { return GetCurrentHP(); }
	virtual float GetHealthMax() const override { return GetMaxHP(); }
	virtual bool IsHealthBarActorAlive() const override { return !IsUnitDead(); }
	/** ANCIENT GROUNDS (TASK-360): the boost as a PERCENT (0 .. 400) — 100 × PerStack × Stacks. Exactly 0 for an unboosted unit, so the row hides itself. */
	virtual float GetDamageBoostPercent() const override { return 100.f * PermanentDamageBonusPerStack * static_cast<float>(PermanentDamageStacks); }
	/** ANCIENT GROUNDS (TASK-360): units ARE boostable, so this is never null (the interface default returns nullptr = "not boostable" — ABuilding and AHeroCharacter keep it). */
	virtual FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() override { return &OnDamageBoostChanged; }
	//~ End IHealthBarProvider Interface

	/** Applies incoming damage (no friendly fire, GDD §3.0) and destroys the unit at 0 HP. */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 *  Drives the attack-lunge visual (TASK-020) AND the ladder climb (TASK-738). The state
	 *  machine stays timer-driven (TASK-004: never per-tick); tick starts disabled and is
	 *  enabled exactly while one of those two is running — see RefreshActorTickEnabled, which
	 *  is the composed predicate both drivers must go through.
	 *  ⚠️ The climb is per-FRAME rather than on the 0.25 s state poll because it steers the
	 *  movement component: a steering input is consumed per movement update, and a quarter-second
	 *  of unsteered MOVE_Flying is a unit drifting off its own ladder.
	 */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 *  Spawner hook (TASK-007): sets the team and the card row this unit's stats
	 *  come from. Call between SpawnActorDeferred and FinishSpawning (preferred),
	 *  or right after a plain SpawnActor — if BeginPlay already ran without a
	 *  usable CardID, this binds the stats and starts the state machine now.
	 *  The CardID cannot be changed once stats are bound (Team still updates).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	void InitUnit(ETeamId InTeam, FName InCardID);

	/**
	 *  Rally buff hook (TASK-042, GDD §4; called by AHeroCharacter::Rally on
	 *  friendly units within range). Applies a TEMPORARY max-walk-speed multiplier
	 *  for Duration seconds, then restores the base speed EXACTLY via a timer.
	 *  Re-applying REFRESHES the duration and NEVER stacks: the resting base speed
	 *  is captured ONCE per buff episode (a refresh reads the stored base, never the
	 *  already-buffed speed), so the base can never permanently drift (the TASK-020
	 *  cache-once, restore-exactly lesson). No-op on dead or match-end-frozen units;
	 *  null-safe without a movement component. Non-positive Duration restores now;
	 *  FreezeAI cancels an active buff and restores the base with zero residual.
	 *  TASK-099: the walk speed now composes with the Battle Cry combat buff
	 *  through RefreshComposedMoveSpeed (shared episode base) — Rally's OWN
	 *  behavior (magnitude, refresh, exact restore) is unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	void ApplyMoveSpeedBuff(float Multiplier, float Duration);

	/**
	 *  Support-profile heal sink (TASK-054, GDD §3.8): adds Amount HP, clamped to
	 *  MaxHP (no overheal). No-op on a dead, match-end-frozen, or never-bound unit
	 *  and on a non-positive Amount. Friendly-only / no-enemy-heal is enforced by
	 *  the CALLER (the Support state machine only ever heals same-team units) — this
	 *  is the raw apply. Units carry no HP-changed delegate (only ACastle does), so
	 *  there is nothing to broadcast.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	void ApplyHealing(float Amount);

	/**
	 *  War Banner aura hook (TASK-055, consumed by the hero upgrade in TASK-058): applies a
	 *  TEMPORARY additive damage-output multiplier of (1 + Bonus) for Duration seconds, then
	 *  restores it to EXACTLY 1.0 via a timer. Bonus 0.20 = +20% dealt damage (War Banner, GDD §4).
	 *
	 *  Refresh-not-stack (the ApplyMoveSpeedBuff discipline): re-applying OVERWRITES the multiplier
	 *  from the bonus (never compounds off the already-buffed value) and re-arms the single one-shot
	 *  timer, so a friendly hero re-pulsing the aura only refreshes the same-magnitude bonus and its
	 *  window. The multiplier is stored SEPARATELY from the row-bound AttackDamage and composed at
	 *  strike time (ComputeOutputDamage), so — unlike an in-place base mutation — it can never drift:
	 *  expiry always resets to the literal 1.0. No-op on a dead or match-end-frozen unit; a
	 *  non-positive Bonus or Duration clears any active aura immediately. FreezeAI ends it with zero
	 *  residual (same contract as the move-speed buff).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	void SetAuraDamageBonus(float Bonus, float Duration);

	/**
	 *  FrostNova spell freeze (TASK-099, M5 ruling 5; called by USpellLibrary on
	 *  enemy units in the reticle radius — the hero and castle are excluded
	 *  CALLER-side and carry no freeze API). PAUSES the unit for Seconds: state/
	 *  attack timers cleared, any in-flight lunge cancelled to the exact rest
	 *  pose, Support healing stopped, movement halted AND the movement component
	 *  disabled (so a subclass drive — e.g. the miner's arrival poll — cannot
	 *  re-issue a walk mid-freeze), unit parked Idle. UNLIKE FreezeAI this is
	 *  RESUMABLE: expiry restores the default movement mode and re-arms the state
	 *  loop, which reacquires from scratch on its next tick (never a synchronous
	 *  decision — that also keeps AMinerUnit's StateCheckInterval-0 seal intact).
	 *
	 *  Refresh-not-stack (ruling 5): re-applying arms the single expiry timer for
	 *  max(remaining, Seconds) — never additive. MATCH-END PRECEDENCE (ruling 5):
	 *  the match-end FreezeAI WINS — ApplyFreeze no-ops on a match-end-frozen
	 *  unit, FreezeAI wipes the spell-freeze state and its expiry timer, and
	 *  EndSpellFreeze refuses to resume a bAIFrozen unit (triple guard). No-op on
	 *  dead / never-bound units and non-positive Seconds. Virtual so subclasses
	 *  with extra drives can extend it (the FreezeAI precedent).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	virtual void ApplyFreeze(float Seconds);

	/**
	 *  True while a FrostNova spell freeze is active (TASK-099). The match-end
	 *  freeze is the SEPARATE, permanent IsAIFrozen() latch — this reports only
	 *  the resumable spell state (false again once the freeze expires).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	bool IsFrozen() const { return bSpellFrozen; }

	/**
	 *  Battle Cry combat buff (TASK-099, M5 ruling 6; called by USpellLibrary on
	 *  FRIENDLY units in the reticle radius). Applies a temporary walk-speed
	 *  multiplier AND an attack-speed multiplier (effective cadence = row Cadence
	 *  ÷ AttackSpeedMult — attacks per second scale UP) for Seconds, then
	 *  restores both EXACTLY. A live attack loop is re-armed at the new cadence
	 *  immediately (honoring the LastAttackTime cooldown) — the buff never waits
	 *  for the next Attack entry, and expiry never leaves a fast loop running.
	 *
	 *  Self-refresh non-stacking (ruling 6): re-applying OVERWRITES the
	 *  multipliers from the args (never compounds) and re-arms the single expiry
	 *  timer. STACKS WITH Rally and War Banner (independent systems, ruling 6):
	 *  the walk speed is composed as SharedBase × RallyMult × CombatMoveMult —
	 *  the resting base is captured once per speed-buff EPISODE (only while
	 *  NEITHER speed buff is active) and restored exactly when the LAST one ends,
	 *  so no ordering of Rally/BattleCry applies or expiries can drift the base
	 *  (the TASK-020 cache-once/restore-exactly lesson). BattleCry magnitudes
	 *  live in the BattleCry* mechanic UPROPERTYs below (Rally precedent) — the
	 *  resolver composes the call as ApplyCombatBuff(
	 *  GetBattleCryMoveSpeedMultiplier(), GetBattleCryAttackSpeedMultiplier(),
	 *  Row.EffectDuration). Non-positive multipliers sanitize to 1 (defensive);
	 *  non-positive Seconds ends any active buff now. No-op on dead /
	 *  match-end-frozen / never-bound units.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	void ApplyCombatBuff(float MoveSpeedMult, float AttackSpeedMult, float Seconds);

	/** Battle Cry walk-speed multiplier for ApplyCombatBuff: 1 + BattleCryMoveSpeedBonus (GDD §4 +25% ⇒ 1.25). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	float GetBattleCryMoveSpeedMultiplier() const { return 1.f + BattleCryMoveSpeedBonus; }

	/** Battle Cry attack-speed multiplier for ApplyCombatBuff: 1 + BattleCryAttackSpeedBonus (GDD §4 +50% ⇒ 1.5; effective cadence = row Cadence ÷ 1.5). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	float GetBattleCryAttackSpeedMultiplier() const { return 1.f + BattleCryAttackSpeedBonus; }

	/**
	 *  Match-end freeze (TASK-028; called by the game mode at match end per the
	 *  TASK-024 contract, and inherited by AMinerUnit — the base StopMovement
	 *  also halts its gold-node walk, TASK-025). Permanently stops the unit's
	 *  AI: clears the state (acquire) and attack timers, cancels any in-flight
	 *  lunge and restores VisualMesh to EXACTLY the cached rest pose (zero
	 *  residual offset), stops movement, and parks the unit in Idle until it is
	 *  destroyed. Also ends any active move-speed buff (TASK-042), clearing its
	 *  timer and restoring the base speed with zero residual, and stops Support
	 *  healing (TASK-054): clears the heal timer and drops the heal target, so a
	 *  frozen Cleric mends no one (Siege pathing / Support following stop via the
	 *  state-timer clear and StopMovement). Idempotent; safe on dead or never-bound
	 *  units. A frozen unit can never restart: stat binding and the state/attack/
	 *  heal timer callbacks are all gated on the frozen flag. Virtual so subclasses
	 *  with extra drives can extend it. TASK-099: also ends any Battle Cry combat
	 *  buff and WIPES any active spell freeze (state + expiry timer) — the
	 *  match-end freeze has PRECEDENCE over a spell freeze (M5 ruling 5).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	virtual void FreezeAI();

	/** True once FreezeAI ran (PIE verification hook for the TASK-024 match-end freeze). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	bool IsAIFrozen() const { return bAIFrozen; }

	/**
	 *  STUCK WATCHDOG — the engine's blocked verdict, harvested (TASK-532; CONVENTIONS
	 *  NAV-§3, NAV-§8). Called by ASiegeUnitAIController::OnMoveCompleted ONLY (TASK-534),
	 *  when UPathFollowingComponent declares EPathFollowingResult::Blocked — a verdict the
	 *  engine has always computed (<10 uu in 5.0 s) and this project has never asked for.
	 *
	 *  ⛔⛔ THIS RECORDS **EVIDENCE**, IT DOES NOT TAKE AN **ACTION** — THE NO-DOUBLE-DRIVER
	 *  LAW (NAV-§3, CONVENTIONS "FOLLOW command…" §6). It may advance the stall clock so the
	 *  NEXT TickStuckWatchdog fires a rung; it may NEVER issue a move, and it may NEVER call
	 *  HandleStuckEscalation itself. ⚖️ Two code paths that can both re-path one
	 *  UPathFollowingComponent is the TASK-280/282 mill this whole feature exists to remove,
	 *  and it is an automatic QA FAIL — the brakes (EscalationCooldown + the monotonic
	 *  EscalationLevel) live in FSiegeStuckStatics::Evaluate and this must pass through them.
	 *
	 *  Safe on dead / frozen / spell-frozen units (no-op) and on a unit that was never stuck.
	 */
	void NotifyMoveBlocked();

	/** Current hit points, in [0, MaxHP]. PIE verification hook (TASK-010). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	float GetCurrentHP() const { return CurrentHP; }

	/** Maximum hit points, bound from the DT_Cards row (Footman: 80). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	float GetMaxHP() const { return MaxHP; }

	/** True once HP reached 0 (the actor is being destroyed — units don't respawn). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	bool IsUnitDead() const { return bDead; }

	/** Card row this unit's stats were (or will be) bound from. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	FName GetCardID() const { return CardID; }

	/** Current state-machine mode (debug/PIE verification). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	ESummonedUnitState GetUnitState() const { return State; }

	/**
	 *  Group-order assignment (TASK-344; pushed by ASiegePlayerController at the
	 *  stage-3 pick confirm). Stores the group id plus THIS unit's precomputed
	 *  station offset (PositionCenter + offset = the tier-3 station — the
	 *  golden-angle sunflower slot, computed and nav-projected ONCE by the
	 *  controller; per-unit scalars only, no arrays on units). Also DROPS any
	 *  current target so the next state tick (≤0.25 s) re-targets from the NEW
	 *  zones — a fresh order replaces the old behavior (the release law), and an
	 *  AMBUSH group must never inherit a stale far-away chase target. The unit
	 *  resolves the live group each state tick via FindUnitGroup; a null result
	 *  self-heals back to the legacy stance gate.
	 */
	void AssignCommandGroup(int32 GroupId, const FVector& StationOffset);

	/** Leaves the current group order (TASK-344): id back to INDEX_NONE, station offset zeroed. Called by the controller's release paths (T/E, Play Again) and by the null-group self-heal. */
	void ClearCommandGroup();

	/** Group order this unit belongs to, or INDEX_NONE (TASK-344 debug/PIE hook). */
	int32 GetCommandGroupId() const { return CommandGroupId; }

	/**
	 *  This unit's precomputed station offset inside its current group order
	 *  (TASK-344; pushed by AssignCommandGroup, ZeroVector while ungrouped).
	 *
	 *  PUBLIC read added by TASK-396 for the reason GetCommandGroupId() already is:
	 *  GroupStationOffset is private, and the MINER's command bodies (TASK-398) run in
	 *  AMinerUnit's OWN poll rather than in this class's state machine.
	 *
	 *  ⚠️ CURRENTLY UNUSED, ON PURPOSE — it is the pre-placed half of a flagged fix,
	 *  not speculative API. AMinerUnit::ResolveStationOffset RE-DERIVES its own
	 *  sunflower slot because this getter did not exist when TASK-398 was written, and
	 *  MinerUnit.h records the consequence (a miner's slot can differ from the one the
	 *  controller recorded for it — cosmetic crowding, nothing else reads either value)
	 *  together with the fix, quoted character-for-character as this signature. The
	 *  file it had to be added to is this one, and this is the last task that owns it,
	 *  so it lands now; the one-line consumption is a MinerUnit.cpp follow-up.
	 */
	FVector GetGroupStationOffset() const { return GroupStationOffset; }

	//~ ─── COMMAND ELIGIBILITY — TWO PREDICATES, NOT ONE (TASK-396; CONVENTIONS
	//~     "FOLLOW command + the DEFAULT-STANCE law + the MINER command rework
	//~     (2026-08-02)" §3, signatures PINNED character-for-character by §7) ───
	//~
	//~ ⚠️ THIS BLOCK IS `public:` ON PURPOSE AND THE ACCESS LEVEL IS PART OF THE PIN,
	//~ for exactly the reason the ANCIENT-GROUNDS block below is: Profile is PRIVATE,
	//~ so these virtuals are the only sanctioned outside-callable eligibility surface.
	//~ ASiegePlayerController's stage-1 select sweep and EnrollInDefaultFollowGroup
	//~ call them from OUTSIDE this class, and AMinerUnit overrides them as `public`
	//~ (TASK-397) — narrowing the access here would not link.
	//~
	//~ Jonathan widened FOLLOW to the Cleric and gave the Miner all five commands, but
	//~ said nothing about giving the Cleric zone orders, so the ONE shipped predicate
	//~ SPLITS IN TWO (CONVENTIONS §3 table):
	//~
	//~   Unit                                Profile   Follow (C)   Zone orders (R/F)
	//~   Footman/Archer/.../Wizard/Sorcerer   Standard     yes              yes
	//~   Cleric                               Support      yes              NO
	//~   Miner (AMinerUnit overrides both)    None         yes              yes
	//~   Ogre / Sapper                        Siege        NO               NO
	//~   any Red / bot unit                   any          NO               NO

	/**
	 *  FOLLOW eligibility by CLASS IDENTITY (CONVENTIONS §3) — the shipped
	 *  CanEverAttack() idiom: NOT a new cards.csv column and NOT a Profile change.
	 *  Base: Standard (every combat unit) PLUS Support (the Cleric — Jonathan widened
	 *  Follow to it). Siege is excluded here, and that exclusion is the whole reason
	 *  the Ogre and the Sapper keep auto-marching exactly as they do today.
	 *  AMinerUnit overrides to true (TASK-397).
	 */
	virtual bool CanFollowHero() const;

	/**
	 *  ZONE-ORDER eligibility by class identity (CONVENTIONS §3) — the R (Hold) / F
	 *  (Ambush) 3-stage pick. Base: Standard ONLY. The Cleric is deliberately
	 *  excluded (manager ruling: FOLLOW-ONLY this pass — zone orders would mean
	 *  reshaping UpdateStateSupport's heal body, which Jonathan did not ask for).
	 *  AMinerUnit overrides to true (TASK-397).
	 */
	virtual bool CanTakeZoneOrders() const;

	/**
	 *  True when this unit may join the FOLLOW group (CONVENTIONS §3): follow-eligible
	 *  by class + Blue team + alive + not match-end frozen. Read by
	 *  ASiegePlayerController::EnrollInDefaultFollowGroup, which BOTH the C-key confirm
	 *  and the §2 spawn auto-enroll funnel through — so this is the ONE gate that keeps
	 *  Siege units and every Red/bot unit out of Follow. A resumable spell freeze does
	 *  NOT exclude: a frozen-but-thawing unit may be circled and obeys once it wakes.
	 */
	bool IsFollowCommandEligible() const;

	//~ ⚠️ THE MINER SPAWN-DEFAULT CARVE-OUT IS **NOT** HERE, AND MUST NOT BE ADDED
	//~ HERE (manager ruling 7 / CONVENTIONS §5 — a miner SPAWNS MINING). It is owned
	//~ ENTIRELY by AMinerUnit (TASK-398): CanFollowHero() answers the EditDefaultsOnly
	//~ bFollowOnSpawn switch for exactly as long as Super::BeginPlay() — which is where
	//~ the §2 auto-enroll below lives — is on the stack, so the enroll's
	//~ IsFollowCommandEligible() gate refuses and the miner joins no group and no
	//~ Members array. A SECOND, base-side carve-out was drafted here and DELIBERATELY
	//~ REMOVED: it would have silently defeated bFollowOnSpawn = true, which is the one
	//~ line Jonathan flips at his playtest gate. One decision, one owner.
	//~
	//~ The contract TASK-398 depends on, restated so it is not broken by accident:
	//~ TryAutoEnrollInFollowGroup() gates on IsFollowCommandEligible() and NOTHING
	//~ ELSE, and it runs inside ASummonedUnit::BeginPlay's synchronous call stack.
	//~ Moving it off that predicate, or deferring it past that stack, re-opens the
	//~ ruling — AMinerUnit::BeginPlay carries a Warning tripwire for exactly that.

	/**
	 *  True when this unit can join a ZONE order — R (Hold) / F (Ambush) (TASK-344,
	 *  NARROWED by TASK-396): CanTakeZoneOrders() + Blue team + alive + not match-end
	 *  frozen. The controller's stage-1 select sweep calls this.
	 *
	 *  ⚠️ THE NAME NO LONGER COVERS FOLLOW. Name and signature are KEPT deliberately
	 *  (the sweep and the §7 pinned registry both depend on them), but since
	 *  CONVENTIONS §3 split the predicate this means ZONE ORDERS ONLY — the C-key
	 *  surface is IsFollowCommandEligible() above. Siege stays excluded; the Cleric is
	 *  excluded HERE but follow-eligible; the Miner is now eligible for both (its two
	 *  overrides, TASK-397). A resumable spell freeze does NOT exclude, unchanged.
	 */
	bool IsGroupCommandEligible() const;

	//~ ─── ANCIENT GROUNDS (TASK-360; CONVENTIONS §3 "ASorcererUnit" + §4 "the permanent stacking damage boost") ───
	//~
	//~ ⚠️ THIS WHOLE BLOCK IS `public:` ON PURPOSE (manager ruling 2, correcting the plan's
	//~ "same shape as ShouldHoldDeathAnim" line). AAncientGround's boost tick calls
	//~ CanEverAttack(), IsAncientGroundEmpowerer(), CanReceiveDamageBoost() and
	//~ AddPermanentDamageStacks() from OUTSIDE this class; ShouldHoldDeathAnim() (below) is the
	//~ right IDIOM but the WRONG access level (it is protected:) — protected here would not link.
	//~ Every signature below is PINNED in CONVENTIONS §7 and must stay character-for-character:
	//~ the whole ANCIENT-GROUNDS batch compiles as one UBT module against that list.

	/**
	 *  THE ATTACK SEAL (CONVENTIONS §3) — class identity, NOT a CSV flag (the
	 *  mechanic-rules-aren't-card-stats law). Base units attack normally; a subclass that
	 *  returns false can never enter the attack machine, enforced at THREE guard points
	 *  (all required, all QA-verified): EnterAttack() stands the unit DOWN to Idle,
	 *  UpdateStateGrouped() acquires nothing and falls through to station-keeping, and
	 *  PerformAttack() refuses.
	 *
	 *  ⚠️ WHY THREE and not one: LoadStatsAndStart binds
	 *  AttackCadence = FMath::Max(Row->Cadence, MinAttackCadence), and MinAttackCadence is
	 *  0.05 s — so a Cadence-0 row (the Sorcerer's) that EVER reached Attack would fire
	 *  20×/s. That is the exact trap the
	 *  Miner's structural seal exists for (qa/TASK-021 WARN-1). The Miner's seal is
	 *  unavailable here: a COMMANDABLE unit must keep its state timer.
	 */
	virtual bool CanEverAttack() const { return true; }

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §3): true only for ASorcererUnit — the unit whose
	 *  presence inside an AAncientGround grants friendly occupants of that ground one
	 *  permanent damage stack per boost tick. Class identity, not a CSV flag. The ground's
	 *  tick COUNTS an empowerer and then SKIPS it as an occupant: a sorcerer never
	 *  self-boosts, and the grant is FRIENDLY-ONLY (a Blue sorcerer boosts only Blue).
	 *  Public for the same outside-caller reason as CanEverAttack().
	 */
	virtual bool IsAncientGroundEmpowerer() const { return false; }

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §4) — occupant eligibility, read by AAncientGround's
	 *  tick: alive AND able to attack at all AND carrying a positive row Damage AND not a
	 *  Support profile. That predicate excludes the Sorcerer, the Miner and the Cleric —
	 *  units whose damage routes through NEITHER compose point (ComputeOutputDamage /
	 *  ApplyDetonation), so stacks on them would be a number that does nothing — and it
	 *  doubles as the boost row's hidden test (no stacks ⇒ 0% ⇒ the row is hidden).
	 */
	bool CanReceiveDamageBoost() const;

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §4) — grants Stacks permanent damage stacks, clamped to
	 *  MaxPermanentDamageStacks (the +400% cap), and broadcasts OnDamageBoostChanged ONLY on
	 *  an ACTUAL change: a fully-capped unit standing in a ground forever produces no
	 *  spurious per-second bar traffic. Non-positive Stacks is a no-op. The boost is
	 *  PERMANENT — there is no decay path, and it is lost only on death.
	 *
	 *  AUTHORITY IS BY CONSTRUCTION and deliberately NOT re-guarded here: the sole gameplay
	 *  caller is AAncientGround's boost tick, which already gates on its PUSHED
	 *  bAuthoritativeBoost flag (CONVENTIONS §2 — the ground never reads HasAuthority(),
	 *  because it is spawned locally on clients from the replicated seed and keeps
	 *  ROLE_Authority there). A HasAuthority() guard on this function would buy nothing in
	 *  M8 P1, where the unit fleet is server-only.
	 */
	void AddPermanentDamageStacks(int32 Stacks);

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §4) — resets the boost to zero and broadcasts
	 *  UNCONDITIONALLY (the reset-path discipline, mirroring HandleDeath's unconditional
	 *  0-HP broadcast). Required because the rigged-death path defers Destroy by up to
	 *  DeathAnimMaxHoldSeconds (2 s), during which a boosted corpse would otherwise hold a
	 *  full boost bar. Play Again needs ZERO work (its step 2 destroys every unit), and the
	 *  match-end freeze deliberately does NOT reset — a permanent boost survives to the end
	 *  screen.
	 */
	void ClearPermanentDamageStacks();

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §4) — the composed output multiplier
	 *  1 + PermanentDamageBonusPerStack × PermanentDamageStacks. EXACTLY 1.0 at zero stacks,
	 *  so every unboosted unit stays bit-for-bit unchanged, and it is the ONLY route the
	 *  boost takes into damage: AttackDamage is NEVER mutated in place (the house buff law —
	 *  the War Banner / Rally cache-once, restore-exactly precedent). BlueprintPure: the QA
	 *  readback hook.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	float GetPermanentDamageMultiplier() const { return 1.f + PermanentDamageBonusPerStack * static_cast<float>(PermanentDamageStacks); }

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §4 + §8, TASK-379) — the per-stack output-damage
	 *  bonus as an INSTANCE read (0.05 = +5% of base damage per stack at the shipped
	 *  default). PUBLIC deliberately: the property itself is a `protected:` EditAnywhere
	 *  tunable, and that access level cost this batch TWO independent workarounds —
	 *  USiegeCheatManager::SetTestDamageBoost resolved it by REFLECTION, and
	 *  UDeckBuilderWidget's Sorcerer rule line had to state its magnitudes
	 *  QUALITATIVELY because it could not reach the value (qa/TASK-365-report.md
	 *  "THE SIMPLIFICATION VERDICT"). Both are retired by this getter.
	 *
	 *  ⚠️ FLAGGED BALANCE LEVER (CONVENTIONS §4): the day this is retuned, every
	 *  consumer re-derives for free — that is the whole point of the getter, and it is
	 *  why the card text must interpolate it rather than bake a number. Instance read,
	 *  never a CDO read, at the two gameplay call sites: a per-Blueprint override is
	 *  honoured. BlueprintPure, matching GetPermanentDamageMultiplier() one line above.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	float GetPermanentDamageBonusPerStack() const { return PermanentDamageBonusPerStack; }

	/**
	 *  ANCIENT GROUNDS (CONVENTIONS §4 + §8, TASK-379) — the hard stack cap
	 *  (80 ⇒ 80 × 5% = the +400% ceiling Jonathan specified). PUBLIC for the same
	 *  reason as GetPermanentDamageBonusPerStack() above: the player-facing ceiling
	 *  is MaxPermanentDamageStacks × PermanentDamageBonusPerStack, and the deck
	 *  builder must DERIVE it rather than bake "+400%" into a string that goes stale.
	 *
	 *  ⚠️ FLAGGED BALANCE LEVER (CONVENTIONS §4) — the second of the three levers if
	 *  the boost plays too hot (the third is AAncientGround::BoostTickInterval).
	 *  Pure read; the clamp itself stays inside AddPermanentDamageStacks, which
	 *  remains the ONLY writer of the stack count (§6 — never a raw field write).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	int32 GetMaxPermanentDamageStacks() const { return MaxPermanentDamageStacks; }

	//~ ---------------------------------------------------------------------
	//~ ⭐ HIGH GROUND (TASK-724, HIGH-§1/§2/§3) — the elevation damage bonus.
	//~ Jonathan, verbatim: "make their attacks deal more damage the higher
	//~ elevation they are. I would say that for every 5 feet that their
	//~ elevation increases, their damage multiplier increases by 10%."
	//~ 📌 M8: nothing here is replicated, there is no RPC and no new class
	//~ tier — the composition is server-authoritative exactly as every other
	//~ multiplier in ComputeOutputDamage already is.
	//~ ---------------------------------------------------------------------

	/**
	 *  ⭐ THE §-PINNED PURE SEAM (HIGH-§3, cloning the WM-§2 / HeightToBrightness precedent:
	 *  a testability obligation gets a testability seam). The whole elevation rule, as four
	 *  floats in and one multiplier out:
	 *
	 *      Multiplier = 1 + BonusPerStep × max(0, AttackerZ − TargetZ) / StepUU
	 *
	 *  ⭐ HEIGHT IS MEASURED **ABOVE THE TARGET**, ⛔ NEVER AS ABSOLUTE WORLD Z (HIGH-§2, his
	 *  row R-1). Absolute Z would make a unit on a hill hit harder than one standing beside it
	 *  on that same hill, and would tie the bonus to wherever the level designer put Z=0. The
	 *  consequence is the argument FOR it: a unit on a tower shooting a unit on the SAME tower
	 *  gets exactly nothing — the bonus is height ADVANTAGE, which is what he pictured.
	 *
	 *  ⛔ BONUS ONLY WHEN ΔZ > 0. Level ground and shooting UPWARD both return EXACTLY 1.0, and
	 *  there is ⛔ NO low-ground penalty — he asked for a bonus, and inventing a malus is
	 *  inventing a mechanic.
	 *
	 *  ⭐ CONTINUOUS (linear), ⛔ NOT stepped (HIGH-§2, his row R-4): a floored rule puts
	 *  invisible breakpoints on a hillside that a player cannot see, cannot aim for and cannot
	 *  learn, and a 1-uu step flipping damage by 10% is worse feel and harder to test.
	 *
	 *  ⛔ NO CAP (HIGH-§5, his row R-3) — he did not ask for one, and inventing a ceiling is
	 *  silently softening his number. The worst case is COMPUTED and handed to him instead:
	 *  ×2.44 for a unit on a 1,200-uu tower on a ~1,000-uu hill firing into a valley. If he
	 *  ever wants a cap, the sanctioned shape is ONE EditDefaultsOnly HeightBonusMaxMultiplier
	 *  (0 = uncapped), ⛔ never a magic number inside this formula.
	 *
	 *  ⛔⛔ IT READS THE WORLD'S REAL HEIGHT AND NOTHING ELSE — there is no bIsOnATower flag, no
	 *  tower query, no occupancy lookup, and ⛔ NO read of the war map's elevation bake
	 *  (WM-§8d: that bake CLAMPS at GroundZ + 1,000 uu, so a gameplay rule reading it would be
	 *  right on low ground and WRONG on exactly the towers and tall hills this feature exists
	 *  for — SHIP-§9's class of fake instrument). ⭐ That is what makes a hill and a tower at
	 *  the same Z deal IDENTICAL damage, and what lets TOWER-§ work for free: the tower grants
	 *  elevation by BEING TALL, and this rule never learns it exists.
	 *
	 *  A non-positive (or NaN) StepUU returns exactly 1.0 — total, never a divide by zero.
	 *
	 *  ⛔ public, plain C++ static, ⛔ NOT a UFUNCTION, exactly four parameters, none defaulted
	 *  (SC-§33). No world access, no actor access. Tested headlessly in
	 *  Tests/SiegeHighGroundTest.cpp.
	 */
	static float HeightAdvantageMultiplier(float AttackerZ, float TargetZ, float StepUU, float BonusPerStep);

	//~ ─── THE LADDER CLIMB — the ONE new tower↔unit coupling surface (TASK-738, TOWER-§8.4(B)) ───
	//~
	//~ ⚠️⚠️ EVERY SIGNATURE IN THIS BLOCK IS PINNED CHARACTER-FOR-CHARACTER IN TOWER-§8.4(B)
	//~ AND TASK-734 (AClimbableTower) COMPILES AGAINST IT. ⛔ Neither task may change it
	//~ unilaterally — an implementer who MEASURES a better shape says so in the handoff and it
	//~ lands as a law amendment, ⛔ never as a silent divergence (the AS-§21 precedent).
	//~
	//~ ⚠️ STATED HONESTLY BECAUSE IT IS A REAL LOSS (TOWER-§8.4(B)): TOWER-§'s founding idea
	//~ was that the tower is a PLACE that never learns a unit exists, and a ladder ENDS that.
	//~ ⭐ What is preserved is the part that mattered — HIGH-§3's fence is UNTOUCHED. The tower
	//~ still ⛔ never calls HeightAdvantageMultiplier, ⛔ never reports elevation, ⛔ never
	//~ includes a HIGH-§ header. A unit on a hill and a unit on a tower at the same Z still deal
	//~ IDENTICAL damage. The coupling that was refused was GAMEPLAY coupling; this is MOVEMENT
	//~ coupling, and there is no way to move a unit without touching it.

	/**
	 *  Begins a scripted traversal along the straight world-space line From -> To. Returns false
	 *  and changes NOTHING if the unit is dead, match-end frozen (bAIFrozen), spell-frozen
	 *  (bSpellFrozen), or already climbing.
	 *
	 *  ⭐⭐ IT IS From -> To, ⛔ NOT foot-then-top, AND THE DISTINCTION IS LOAD-BEARING: the link
	 *  is `ENavLinkDirection::BothWays` (`TOWER-§8.7`), so a unit ordered DOWN calls this with the
	 *  endpoints the other way round. ⛔ NOTHING here compares the two for Z ordering, and ⛔
	 *  nothing may be added that does. ⚠️ The parameters were renamed from the law's
	 *  `LadderFootWorld`/`LadderTopWorld` for exactly that reason (TASK-734's finding, taken —
	 *  types, count and order are untouched, so it is source- and reflection-compatible; the
	 *  handoff requests the `TOWER-§8.4(B)` text amendment). Descent has its own test.
	 *
	 *  ⭐ MOVE_Flying + swept AddMovementInput along ONE straight segment, at LadderClimbSpeedUU
	 *  (TOWER-§8.5) — ⛔ not MOVE_Custom, ⛔ not a lerp of the line. ⚠️ EXCEPT the deck-breach
	 *  window at the line's elevated end, which is a declared, measured exception; see
	 *  FSiegeLadderClimbStatics::ShouldSweep.
	 *  ⭐ Arming this ALSO arms the TOWER-§9 disarm — see IsClimbing().
	 *  ⚠️ Also refuses a degenerate (sub-1 uu) or NaN line: see FSiegeLadderClimbStatics::
	 *  MinClimbLineUU for why that is a math guard rather than a fifth policy rule.
	 *
	 *  ⭐ The two points are SURFACE positions (the mesh's sockets, which sit on navmesh); the
	 *  driver lifts both by the capsule half-height internally, so the unit finishes standing ON
	 *  the destination rather than buried one half-height inside it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit|Climb")
	bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld);

	/**
	 *  Ends an in-flight climb WHEREVER the unit is: restores the movement mode and lets it
	 *  drop. Idempotent.
	 *
	 *  ⭐ THIS IS ALSO THE TOWER-DESTROYED PATH (TOWER-§10 L-5): AClimbableTower::EndPlay calls
	 *  it on every climber it started, because a MOVE_Flying unit does ⛔ NOT fall when the
	 *  floor disappears — under the ramp it did, and that was free; here it must be explicit.
	 *  ✅ The unit then falls and survives: there is ⛔ no fall damage in Siegebound (TOWER-§4a).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit|Climb")
	void AbortLadderClimb();

	/**
	 *  True for the whole ascent. Read by ABP_Footman's climb state (TASK-739) and by the
	 *  TOWER-§9 disarm guards.
	 *
	 *  ⛔⛔ THIS IS THE DISARM TERM, AND IT IS DELIBERATELY NOT CanEverAttack(): that one is a
	 *  `const` CLASS-IDENTITY seal, and a climb is TRANSIENT. See
	 *  FSiegeLadderClimbStatics::IsAttackAllowed for the two-term gate the three guard points
	 *  actually read.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit|Climb")
	bool IsClimbing() const;

	/**
	 *  Broadcast EXACTLY ONCE per successful BeginLadderClimb — on arrival, abort, a new order,
	 *  death, either freeze, EndPlay, or the declared watchdog. The tower's ONLY completion
	 *  signal, so AClimbableTower needs NO tick (TOWER-§ keeps its no-tick property).
	 *
	 *  ⚠️ BROADCAST LAST, AFTER THE MOVEMENT MODE IS RESTORED — a listener that inspects the
	 *  unit (or hands it back to path following) must never see it mid-teardown. The
	 *  exactly-once latch is cleared BEFORE the broadcast, so a listener that re-enters
	 *  AbortLadderClimb() from inside the delegate is inert rather than recursive.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Unit|Climb")
	FSiegeLadderClimbEnded OnLadderClimbEnded;

	/**
	 *  ⭐ `ILadderClimber`'s completion accessor (TASK-787, `CONTACT-§12.3`) — how `AClimbableTower`
	 *  binds and unbinds WITHOUT naming this class. ⛔ The INSTANCE above is untouched: an interface
	 *  declares no state, so the implementer owns the delegate and the interface owns only the way
	 *  to reach it.
	 *
	 *  ⚠️ INLINE AND ⛔ NOT IN THE `.cpp` DELIBERATELY: it is a member return, there is nothing to
	 *  test in it, and TASK-787's fence keeps `SummonedUnit.cpp` closed. ⛔ It is ⛔ NOT a
	 *  `UFUNCTION` — a reflected function cannot return a delegate reference, and `ILadderClimber`'s
	 *  methods are plain C++ pure virtuals by design (the `IHealthBarProvider` precedent).
	 */
	virtual FSiegeLadderClimbEnded& GetOnLadderClimbEnded() override { return OnLadderClimbEnded; }

protected:

	/** Binds the card stats from DT_Cards and starts the state machine. */
	virtual void BeginPlay() override;

	/** Clears the state/attack timers. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  TASK-349 (CONVENTIONS "Castle 3× HOLLOW" team-gating): AutoPossessAI
	 *  possession can land AFTER BeginPlay (the miner's controller poll exists for
	 *  exactly this reason), so the team nav-filter push in ApplyTeamGatingProfile
	 *  would miss the controller if it only ran at BeginPlay. This override
	 *  re-applies the profile at every possession — idempotent, and Team is always
	 *  authoritative by then (deferred spawns set it via InitUnit before
	 *  FinishSpawning; a post-possession InitUnit team update re-applies again).
	 */
	virtual void PossessedBy(AController* NewController) override;

	/**
	 *  Visual slot for the blueprint child (TASK-010: BP_Unit_Footman assigns
	 *  /Game/Meshes/SM_Footman). No mesh is set in C++. The capsule owns all
	 *  collision; this component never collides or affects navigation.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** Overhead poll-driven health bar (M5.5, TASK-110): hide-at-full, team-tinted. Added once here; AMinerUnit inherits it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
	TObjectPtr<UCombatantHealthBarComponent> HPBarWidget;

	/**
	 *  OPTIONAL skeletal runtime visual (M7 skeletal-animation workstream, TASK-159).
	 *  Empty + hidden by default. At BeginPlay the unit composes /Game/Characters/SK_<CardID>
	 *  from its CardID (mirrors the static /Game/Meshes/SM_<CardID> string law); if it
	 *  RESOLVES this becomes the runtime visual (mesh + AnimClass /Game/Characters/ABP_<CardID>,
	 *  the static VisualMesh hidden, the team recolor routed here) — else it stays empty and
	 *  the static VisualMesh drives exactly as today. Purely ADDITIVE + null-safe; the
	 *  placement GHOST is UNCHANGED (it always resolves the static SM_<CardID> — ghosts don't
	 *  animate), so SM_<CardID> must remain at its path even after a unit gains a rig.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
	TObjectPtr<USkeletalMeshComponent> SkeletalVisualMesh;

	/**
	 *  Relative YAW the skeletal visual carries so the mesh's baked forward faces ACTOR-FORWARD (+X)
	 *  (TASK-326/327 — the yaw half of the per-BP authoring trap TASK-306/307 closed for Z).
	 *  The whole fleet is rigged through ONE pipeline (Tools/ArtPipeline/rig_character.py, character
	 *  front authored on Blender -Y) onto ONE shared skeleton (SK_Footman_Skeleton), so every
	 *  SK_<CardID> bakes its forward on UE-local +Y — measured in-engine across all 12 units
	 *  (12/12 on the shared skeleton; raw SkeletalMeshActors at actor yaw 0 all present their front
	 *  to a +Y camera). Rot(θ)·(0,1,0) = (1,0,0) ⇒ θ = -90, so -90 is correct for EVERY unit — the
	 *  same constant, for the same reason, as ASiegePlayerController::GhostYawOffset, which has
	 *  solved the identical problem for the placement ghost since TASK-014/037/038.
	 *
	 *  This is the ABSOLUTE component yaw, NOT a delta: ResolveSkeletalVisual OVERWRITES the
	 *  component's yaw with it (an additive offset would rotate the already-authored -90 units to
	 *  -180 and is FORBIDDEN). Pitch and roll are preserved exactly, as the grounding fix preserves
	 *  the authored X/Y. A BP_Unit_<Unit> MUST NOT hand-author SkeletalVisualMesh rotation — that is
	 *  what left Archer/Ogre/Wizard at the constructor default 0 and made them walk sideways.
	 *
	 *  EXCEPTION HATCH: a genuinely differently-baked mesh may override this on its own BP. That is
	 *  a NON-DEFAULT requiring an explicit manager ruling (same doctrine as a bespoke skeleton) —
	 *  no current unit triggers it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit")
	float SkeletalVisualYawOffset = -90.f;

	/** §6 white hit-flash on every actual damage event (M7, TASK-154). Driven from TakeDamage; overlay-based, null-safe. AMinerUnit inherits it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeHitFlashComponent> HitFlashComponent;

	/** §6 procedural transform juice (M7, TASK-155): spawn squash-and-stretch on the active visual mesh. AMinerUnit inherits it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeMeshJuiceComponent> MeshJuiceComponent;

	/** DT_Cards row name whose stats drive this unit (BP_Unit_Footman sets Footman, TASK-010). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
	FName CardID = NAME_None;

	/** Team this unit fights for. Spawner sets it per unit (player summons are Blue). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Card stat table (GDD §3.0). May not be imported yet (TASK-008) — resolved null-safe at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Unit")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/**
	 *  Optional per-unit projectile class for FireProjectileAt (TASK-298 — the Wizard's
	 *  BP_Projectile_Fireball fireball visual). Null (the default) falls back to the base
	 *  AProjectile, so every existing ranged unit (Archer/Longbowman/towers, ProjectileClass
	 *  null) is byte-for-byte unchanged. Purely cosmetic — the splash comes from the row
	 *  AoERadius passed to InitProjectile, never from this class.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit")
	TSubclassOf<AProjectile> ProjectileClass;

	/** Acquisition radius in units (GDD §3.8 profile constant: 600 — not a card stat). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0"))
	float AggroRadius = 600.f;

	/** Leash: a target beyond this distance is dropped and Advance resumes (GDD §3.8: 900). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0"))
	float LeashRange = 900.f;

	/** If the best unit/hero stands within this distance of the nearest building/castle, prefer the unit/hero (GDD §3.8: 100). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0"))
	float TieBreakDistance = 100.f;

	/**
	 *  Shield Wall DEFEND engagement BAND (W1 TASK-275; ⚠️ SEMANTIC CHANGE — TASK-574,
	 *  CONVENTIONS WR-§2b row B): under the player's DEFEND stance, a Blue Standard unit
	 *  fights ONLY enemies within this 2D distance PAST ITS OWN CASTLE'S WALL FACE, else
	 *  falls back toward home.
	 *
	 *  ⚠️ IT IS NO LONGER A RADIUS FROM THE CASTLE CENTRE. ⛔ NEVER pass this member
	 *  straight to AcquireEnemyNearPoint — ResolveDefendEngagementRadius() is the ONE
	 *  place the band is converted into a centre radius, and it is the only supported
	 *  read of this value.
	 *
	 *  ⚠️ THE NAME WAS DELIBERATELY NOT CHANGED TO MATCH THE NEW MEANING, and the reason
	 *  is mechanical rather than stylistic: ASorcererUnit's constructor writes this member
	 *  BY NAME to seal itself (SorcererUnit.cpp "DefendRadius 0"), and SorcererUnit.{h,cpp}
	 *  is outside TASK-574's ownership — a rename would have broken a file this task may
	 *  not edit. The MEANING moved; the SYMBOL stayed.
	 *
	 *  WHY THE SHAPE CHANGED, AND WHY BUMPING THE NUMBER WAS REFUSED: the old value was a
	 *  disc about the castle CENTRE, and after the 9× remaster the castle's colliding
	 *  half-width is ≈3,657 uu — so a 2,500 disc lay ENTIRELY INSIDE THE KEEP. Besiegers
	 *  are physically blocked at the gate and therefore stand at ≥3,657 uu from centre, so
	 *  DEFEND acquired nothing, ever: every defender walked home while the castle was
	 *  battered, and no bounds readback could see it. A radius measured from the centre of
	 *  a thing that is SMALLER than the thing cannot mean anything at ANY scale, so the
	 *  repair is structural — the resolver reads the castle's LIVE colliding bounds and
	 *  this value is the band added to them (CONVENTIONS SC-§34's structural escape).
	 *
	 *  DEFAULT 1281 IS BEHAVIOUR-PRESERVING, NOT INVENTED: the 3× castle's measured
	 *  colliding half-extent is 1218.95 uu, and 1218.95 + 1281 = 2500 — EXACTLY the disc
	 *  the shipped game had, so this is a repair and not a balance change. The same 1281
	 *  resolves to ≈4,938 at the 9× castle, and carries itself through the next resize.
	 *
	 *  Q6 default; ⚠️ FLAGGED tunable (playtest item D10) — the MECHANISM above is a
	 *  manager ruling, the NUMBER is Jonathan's. Lives HERE per CONVENTIONS (the unit owns
	 *  it, NOT the controller). // Shield Wall — Defend band past the wall face
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float DefendRadius = 1281.f;

	/** Seconds between state-machine checks (spec: ~0.25 s, never per-tick). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0.05"))
	float StateCheckInterval = 0.25f;

	/**
	 *  TASK-349 loop-2 B2 (march-freeze fix): box half-extent of the filter-aware
	 *  navmesh projection ResolveStructureMarchPoint runs around a structure goal's
	 *  nearest-collision point. Must comfortably cover the gap between a castle
	 *  wall face and the nearest navmesh poly the mover's team filter ALLOWS
	 *  (agent-radius erosion ~34 uu + the interior-area hull-box margin; 800
	 *  horizontal is generous for every wall/gate face of the castle, whose
	 *  colliding bounds are 7313.7 × 7384.5 × 8082.6 uu after the 9× remaster).
	 *  ⚠️ CITATION CORRECTED BY TASK-574 (CONVENTIONS WR-§2b row G): this line used
	 *  to quote "the 2437×2461 castle", which was the PRE-remaster size and had been
	 *  false since the 9× pass. ⛔ THE NUMBER IS DELIBERATELY UNCHANGED (SC-§34 (ii),
	 *  the human/body-scale exemption) and TASK-557 row S10 verified it: what 800
	 *  measures is the GAP AT A FACE — agent-radius erosion plus a hull-box margin,
	 *  both keyed to a UNIT'S BODY — not any castle dimension. Units did not grow, so
	 *  this extent does not scale with the castle. Only the citation was wrong.
	 *  Deliberately NOT castle-half-diagonal-sized: a goal buried DEEP inside the
	 *  enemy interior (e.g. an enemy building at the hall center) is
	 *  unreachable-by-design and should FAIL projection (legacy-fallback, unit
	 *  holds) rather than resolve to a wall point it cannot attack from.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|AI")
	FVector StructureGoalProjectionExtent = FVector(800.f, 800.f, 600.f);

	/**
	 *  Support profile (Cleric) heal-tick cadence (TASK-054). The heal RATE is the
	 *  row Damage (HP/sec); this only sets the granularity — each tick applies
	 *  Damage × SupportHealInterval, so the per-second total is invariant to this
	 *  value. ~0.1 s reads as continuous; never per-tick (TASK-004 law).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0.05"))
	float SupportHealInterval = 0.1f;

	/**
	 *  CHARGE keyword (TASK-055, GDD §3.0 — Cavalry): seconds of uninterrupted movement
	 *  required to prime the charge. The first attack after this much continuous advancing
	 *  deals × ChargeMultiplier, then reverts until momentum is rebuilt. Mechanic RULE, not
	 *  a card stat (CONVENTIONS) — the per-card bCharge flag is the CSV column; these
	 *  magnitudes live here as UPROPERTY // GDD defaults.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	float ChargeMoveSeconds = 2.f; // GDD §3.0

	/** CHARGE damage multiplier for the primed first hit (GDD §3.0: 2×). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "1"))
	float ChargeMultiplier = 2.f; // GDD §3.0

	/**
	 *  Min speed (u/s) counted as "moving" for CHARGE bookkeeping — an implementation detail
	 *  (blocked/stalled detection on the 0.25 s state timer), NOT a GDD stat. A unit slower
	 *  than this while advancing is treated as blocked and loses its momentum.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	float ChargeMoveSpeedThreshold = 50.f;

	/** SLAYER damage multiplier vs high-HP targets (GDD §3.0: 2×). Pikeman. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "1"))
	float SlayerMultiplier = 2.f; // GDD §3.0

	/** SLAYER applies to any target whose MaxHP is >= this (GDD §3.0: 150). Pikeman. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	float SlayerHPThreshold = 150.f; // GDD §3.0

	/**
	 *  BATTLE CRY attack-speed bonus (TASK-099, M5 ruling 6 — mechanic RULE, not
	 *  a card stat; the per-card EffectDuration is the CSV column). 0.5 = +50%
	 *  attack speed. Consumed via GetBattleCryAttackSpeedMultiplier by the spell
	 *  resolver (Rally-precedent placement: magnitudes live with the API owner).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	float BattleCryAttackSpeedBonus = 0.5f; // GDD §4

	/** BATTLE CRY move-speed bonus (TASK-099, M5 ruling 6). 0.25 = +25% move speed; consumed via GetBattleCryMoveSpeedMultiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	float BattleCryMoveSpeedBonus = 0.25f; // GDD §4

	/**
	 *  ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — output damage added per permanent stack.
	 *  0.05 = +5% of BASE damage per stack, i.e. one stack per sorcerer per boost tick
	 *  (Jonathan's ruling iv: two sorcerers in the same ground = two stacks per second).
	 *  A mechanic RULE, so it lives here as a UPROPERTY default — it is NOT a cards.csv
	 *  column (the mechanic-rules-aren't-card-stats law; the Charge/Slayer/BattleCry
	 *  magnitudes above are the precedent). FLAGGED tunable: this and
	 *  MaxPermanentDamageStacks are two of the three balance levers if the boost plays too
	 *  hot (the third is AAncientGround::BoostTickInterval).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	float PermanentDamageBonusPerStack = 0.05f; // GDD §3.12

	/**
	 *  ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — the hard stack cap: 80 × 5% = the
	 *  **+400% ceiling** Jonathan specified ("400% is the max"), and the reason the boost
	 *  bar's four 100%-wide bands cover the whole range exactly. AddPermanentDamageStacks
	 *  clamps to this, so a unit parked in a ground forever tops out instead of growing
	 *  without bound. Mechanic rule, FLAGGED tunable (see above).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Keywords", meta = (ClampMin = "0"))
	int32 MaxPermanentDamageStacks = 80; // GDD §3.12

	/**
	 *  ⭐⛔⛔ HIGH GROUND (TASK-724, HIGH-§1) — THE ELEVATION STEP, IN UNREAL UNITS, AND THE
	 *  ARITHMETIC IS WRITTEN OUT HERE SO ⛔ NOBODY EVER RE-DERIVES IT:
	 *
	 *      Jonathan said "for every 5 FEET". "5 feet" is ⛔ NOT an engine unit.
	 *      Unreal is CENTIMETRES, and 1 uu = 1 cm.
	 *      1 ft = 30.48 cm  (exact, by international definition)
	 *      5 ft = 5 × 30.48 = 152.4 cm  ⇒  ⭐ 152.4 uu
	 *
	 *  ⛔⛔ 152.4 IS THE ONLY NUMBER. ⛔ NOT 150 ("close enough" — wrong by 1.6% and it looks
	 *  like a designer's round number), ⛔ NOT 152, ⛔ NOT 500, and ⛔⛔ ABOVE ALL NOT 5 —
	 *  feet-as-units is wrong by 30×, it would make every unit on the field a god, and it
	 *  would look entirely plausible in review. ⭐ This comment exists so a future "tidy-up"
	 *  cannot round the value away without reading why it is not round.
	 *  Tests/SiegeHighGroundTest.cpp asserts this default against 5 × 30.48 re-derived from
	 *  the foot definition — the regression claim that catches exactly that tidy-up.
	 *
	 *  ⭐ EditDefaultsOnly BECAUSE THE TUNABLE IS THE POINT: he chose 5 ft and 10% in prose,
	 *  so his next sentence retunes both WITHOUT a code change. A mechanic RULE, therefore ⛔
	 *  NEVER a cards.csv column (the mechanic-rules-aren't-card-stats law; the Charge / Slayer
	 *  / BattleCry magnitudes above are the precedent).
	 *
	 *  0 (or negative) disables the bonus cleanly — HeightAdvantageMultiplier returns exactly
	 *  1.0 rather than dividing by zero.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|HighGround", meta = (ClampMin = "0"))
	float HeightBonusStepUU = 152.4f; // HIGH-§1: 5 ft × 30.48 cm/ft

	/**
	 *  ⭐ HIGH GROUND (TASK-724, HIGH-§1) — the damage bonus earned per HeightBonusStepUU of
	 *  height ADVANTAGE over the target. 0.10 = Jonathan's "+10%", ADDITIVE and UNCOMPOUNDED
	 *  (the plain reading of his sentence): two steps is ×1.20, ⛔ not ×1.21.
	 *
	 *  Applied CONTINUOUSLY, ⛔ not in discrete rungs (HIGH-§2, his row R-4), and ⛔ WITHOUT A
	 *  CAP (HIGH-§5, his row R-3 — the worst case is stated to him instead: ×2.44).
	 *  0 disables the bonus while leaving the seam total. Mechanic rule, EditDefaultsOnly for
	 *  the same reason as the step above.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|HighGround", meta = (ClampMin = "0"))
	float HeightBonusPerStep = 0.10f; // HIGH-§1: his "+10%"

	/**
	 *  ⭐⭐ THE LADDER CLIMB RATE (TASK-738, TOWER-§8.5) — AND ITS CONSEQUENCE IS WRITTEN HERE
	 *  BECAUSE HIGH-§1's LAW IS THAT A NUMBER WHOSE CONSEQUENCE IS NOT WRITTEN BESIDE IT GETS
	 *  RETUNED BY SOMEBODY WHO DOES NOT KNOW WHAT THEY ARE CHANGING.
	 *
	 *  ⭐ DERIVED, ⛔ NOT FELT: 350 is the `Speed` cell of the Archer and the Wizard in
	 *  Docs/Data/cards.csv — the fastest units that would ever want this tower. ⇒ ascending
	 *  costs a unit EXACTLY the time walking the same distance would, so the ladder smuggles in
	 *  ⛔ NO speed penalty on top of Jonathan's disarm. The whole cost of the ascent is the
	 *  vulnerability he ruled, and nothing beside it.
	 *
	 *  ⚠️⚠️ THIS IS HIS EXPOSURE LEVER, AND THE CONSEQUENCE IS NOT SOFTENED (TOWER-§9.3): the
	 *  pinned climb line is 1,236.9 uu, so at 350 uu/s the ascent is 3.53 s, during which a
	 *  climber CANNOT ANSWER FIRE. One defending Longbowman (Dmg 18, Cadence 1.5 s, Range
	 *  3,600) lands 2-3 shots = 36-54 damage. ⇒ a climbing ARCHER or WIZARD (45 HP) survives at
	 *  9 HP or DIES; a Longbowman (70 HP) survives. LOWERING THIS NUMBER KILLS THEM OUTRIGHT
	 *  (250 uu/s ⇒ 4.95 s ⇒ 3-4 shots ⇒ dead). ⛔ Do not "slow the climb down so it reads
	 *  better" without re-reading TOWER-§9.3's table.
	 *
	 *  ✅ The counterweights, also measured and also his: the ladder's exposure window is HALF
	 *  the ramp's was (3.53 s vs 6.86 s), the ground shooter gets NO height bonus firing upward
	 *  (HIGH-§2), and the climb can be ABORTED by a new order — the unit drops free, with no
	 *  fall damage anywhere in Siegebound (TOWER-§4a).
	 *
	 *  A 0 (or negative) value cannot hang a unit: the watchdog budget floors the divisor
	 *  (FSiegeLadderClimbStatics::MinClimbSpeedUU) and the climb ends by timeout instead.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit|Climb", meta = (ClampMin = "0"))
	float LadderClimbSpeedUU = 350.f; // TOWER-§8.5: the Archer/Wizard `Speed` cell — walk speed, no penalty

	/**
	 *  Blockout "attack animation" (TASK-020): how far VisualMesh lunges along the
	 *  capsule's local +X (actor forward) on each cadence hit. Purely visual —
	 *  damage math is untouched. ~0 disables the lunge; negative recoils backward.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	float AttackLungeDistance = 40.f;

	/**
	 *  Seconds for one out-and-back lunge cycle, sine-eased. Clamped at cycle
	 *  start to 0.8 × Cadence so the mesh is guaranteed back at rest before the
	 *  next hit (hits are never less than Cadence apart). <= 0 disables the lunge.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	float AttackLungeDuration = 0.3f;

	/**
	 *  Impact puff spawned at the contact point (closest point on the target's
	 *  collision to this unit; fallback: target location) on each damage
	 *  application (TASK-020). Defaults to the READ-ONLY Variant_Combat donor
	 *  NS_Damage — referenced, never edited (CONVENTIONS template-donor rule).
	 *  Resolved and cached ONCE at BeginPlay — no per-attack sync-load hitch; a
	 *  missing asset is logged once and means no VFX, never a crash.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TSoftObjectPtr<UNiagaraSystem> AttackImpactEffect;

	/**
	 *  Skeletal death-anim hold cap (TASK-165). When a rigged unit dies it plays
	 *  A_<CardID>_Death (single-node, non-looping) and HOLDS the final pose; the
	 *  actor's Destroy is deferred by min(clip length, this) so the anim reads.
	 *  Small + capped (spec) so a corpse never lingers indefinitely; non-rigged units
	 *  (no resolved death clip) are destroyed immediately as before (0-length defer).
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback", meta = (ClampMin = "0"))
	float DeathAnimMaxHoldSeconds = 2.f;

	/**
	 *  Whether this unit HOLDS its skeletal death anim before Destroy (TASK-165), deferring the
	 *  actor removal by up to DeathAnimMaxHoldSeconds so the clip reads. Base = true. AMinerUnit
	 *  overrides to FALSE: a miner's removal-from-play runs the §3.3 economy bookkeeping in
	 *  EndPlay (RemoveMinerIncome / UnregisterMinerAlive), which must stay PROMPT — a dead miner
	 *  must not keep accruing income or hold its cap-6 slot for the hold window. Miners show the
	 *  §6 gold-burst on death (HandleDeath) as their death feedback instead of a held pose.
	 */
	virtual bool ShouldHoldDeathAnim() const { return true; }

	/**
	 *  THE FOLLOW BODY (TASK-396; CONVENTIONS §4, pinned `protected` by §7).
	 *  Dispatched from UpdateState's HOISTED follow branch — above the profile
	 *  dispatch, so it runs whatever the profile (that hoist is the reason a
	 *  Support Cleric can follow at all: Support returns out of the profile
	 *  dispatch before the shipped group dispatch is ever reached).
	 *
	 *  - NEVER ATTACKS. CurrentTarget is FORCED null and none of AcquireTarget /
	 *    AcquireEnemyNearPoint / EnterAttack is called on any path. This is a
	 *    per-BODY seal and is deliberately NOT CanEverAttack(): that one is
	 *    per-CLASS and permanent, and a following Footman must become a normal
	 *    attacker the instant its group is released.
	 *  - The hero anchor (ASiegePlayerController::GetFollowAnchor) is resolved
	 *    LIVE every tick and NEVER cached, which is what makes a post-respawn
	 *    replacement pawn work for free.
	 *  - Anchor null (no hero / dead hero) ⇒ HOLD POSITION (EnterIdle), resuming
	 *    the instant a live pawn resolves again — manager ruling 8.
	 *  - Goal = anchor location + this unit's enroll-time GroupStationOffset, via
	 *    EnterAdvanceToLocation, idling inside the shipped 150 uu arrival
	 *    tolerance, and RE-ISSUED ONLY outside FollowRepathTolerance (manager
	 *    ruling 10 — an unconditional re-path is the TASK-280/282 mill).
	 *
	 *  The Group parameter carries no zones (a follow group owns no ground); it is
	 *  read only by the type tripwire that guards the dispatch contract.
	 */
	void UpdateStateFollow(const FSiegeUnitGroup& Group);

	//~ ── STUCK WATCHDOG (TASK-532; CONVENTIONS NAV-§3 behaviour law, NAV-§8 pin) ──────
	//   Jonathan's report: units wedge on scatter rocks and never recover. The engine
	//   ALREADY declares EPathFollowingResult::Blocked after <10 uu in 5.0 s; nothing in
	//   this project ever asked, and EnterAdvance's Idle branch (SummonedUnit.cpp:2534)
	//   then re-issues a BYTE-IDENTICAL request 0.25 s later — a silent livelock. This is
	//   the escape hatch; ⛔ the anti-repath gates themselves are UNTOUCHED.
	//
	//   ⛔ ONE DRIVER, ZERO NEW TIMERS: the whole ladder rides the 0.25 s StateTimerHandle
	//   poll that already exists. A SetTimer in this feature is a finding (NAV-§3).

	/**
	 *  Runs the stall ladder for ONE poll and dispatches at most ONE rung. Called from
	 *  exactly one place — UpdateState, immediately after TrackChargeMovement — which is
	 *  ABOVE the follow hoist and the profile dispatch, so Standard, Siege, Support,
	 *  Follow and Hold/Ambush are all covered by that single line (the TrackChargeMovement
	 *  precedent). UpdateState's freeze/death early-out sits above it, so a dead, frozen or
	 *  spell-frozen unit never reaches it.
	 *
	 *  Also DRAINS SidestepLeaseRemaining by the same DeltaSeconds — the lease and the
	 *  ladder must advance on one clock or the lease could outlive the stall it belongs to.
	 *
	 *  ⚠️ DeltaSeconds MUST come from ConsumeStuckDeltaSeconds() (the world clock),
	 *  ⛔ NEVER from StateCheckInterval — AMinerUnit sets that to 0 by seal
	 *  (MinerUnit.cpp:61) and TASK-533 calls this same watchdog. A 0 delta is inert.
	 */
	void TickStuckWatchdog(float DeltaSeconds);

	/**
	 *  Performs ONE rung of the NAV-§3 ladder. Virtual + protected because AMinerUnit
	 *  OVERRIDES it (TASK-533): the stall DECISION is identical for a miner, but the
	 *  ACTION must differ — the miner's walk is owned by EnsureWalkingToNode/DriveToPoint,
	 *  and running these rungs on it would put a SECOND STEERING AUTHORITY on its movement
	 *  component (the double-drive the miner seal exists to prevent).
	 *
	 *  Sidestep       — ONE move to FSiegeStuckStatics::ComputeSidestepGoal, and take the lease.
	 *  WidenAndRepath — supersede the lease and invalidate the goal latches so the next poll's
	 *                   EnterAdvance* genuinely re-issues toward the ORIGINAL goal.
	 *  Abandon        — drop the move goal and stand down; the standing body re-chooses next poll.
	 *
	 *  ⛔ NEVER cancels the unit's standing ORDER (CommandGroupId is untouched on every
	 *  rung), ⛔ never enters Attack, ⛔ never leaves the unit inert forever.
	 */
	virtual void HandleStuckEscalation(ESiegeStuckAction Action);

	/**
	 *  Seconds of WORLD CLOCK since the previous call, clamped to [0, MaxStuckDeltaSeconds].
	 *  ⛔ THE ONLY SANCTIONED DELTA SOURCE FOR THE WATCHDOG, and it exists as a named
	 *  helper precisely because BOTH call sites (here and AMinerUnit::UpdateMining,
	 *  TASK-533) need it: StateCheckInterval is 0 on the miner by seal, so an inlined
	 *  literal would become the same magic number duplicated across two files.
	 *  The FIRST call on any unit returns 0 (inert) — there is no previous tick to measure.
	 */
	float ConsumeStuckDeltaSeconds();

	/** Transient per-unit stall state (anchor + clocks + rung level). ⛔ Deliberately UNREFLECTED (NAV-§8/§11): an unreflected struct cannot be replicated by accident. */
	FSiegeStuckState StuckState;

	/** The ladder's thresholds. EditDefaultsOnly so Jonathan can tune the feel per unit class WITHOUT a recompile; these are the ONLY tunables this feature adds (NAV-§7). CONFIG, never replicated (NAV-§11). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck")
	FSiegeStuckTuning StuckTuning;

	/** World point the Sidestep rung last steered this unit to. Meaningful only while SidestepLeaseRemaining > 0. */
	FVector SidestepGoal = FVector::ZeroVector;

	/**
	 *  ⚠️ THE SIDESTEP LEASE — LOAD-BEARING, NOT A POLISH ITEM (NAV-§3).
	 *  EnterAdvanceToLocation NULLS CurrentMoveGoal (SummonedUnit.cpp:2675), so the next UpdateState
	 *  would see bGoalChanged == true, re-issue EnterAdvance(real goal), and CANCEL the
	 *  sidestep 0.25 s after it started — the rescue would never happen. While this is
	 *  positive UpdateState skips the profile dispatch entirely and lets the in-flight
	 *  sidestep move run. Drained by TickStuckWatchdog on the same delta as the ladder.
	 *  ⛔ CLEARED ON EVERY EXIT: EnterIdle, EnterAttack, HandleDeath, FreezeAI — and
	 *  ApplyFreeze (the declared fifth; see its comment). A lease that outlives a state
	 *  change is a stuck unit of a new kind.
	 */
	float SidestepLeaseRemaining = 0.f;

	/**
	 *  ⭐ SIDESTEP ATTEMPTS THIS UNIT HAS MADE, EVER — the ONLY term that makes the side actually
	 *  alternate. Incremented once per fired Sidestep rung; its PARITY is the whole payload.
	 *
	 *  ⛔ DELIBERATELY **NOT** A FIELD OF FSiegeStuckState, AND THAT IS THE ENTIRE POINT.
	 *  FSiegeStuckStatics::Reset assigns a default-constructed instance, and Evaluate calls it on
	 *  EVERY Abandon and EVERY re-anchor, so nothing stored there can survive a stall.
	 *  The first shipped version derived Attempt from StuckState.EscalationLevel,
	 *  which Evaluate assigns BEFORE its switch and is therefore ALWAYS exactly 1 whenever
	 *  Sidestep dispatches — a per-unit CONSTANT, so a unit wedged on a rock's left face sidestepped
	 *  into that rock on every stall, forever (qa/TASK-537.md, the BLOCKER). Free-running across
	 *  stalls is the fix; anything Reset can reach is not.
	 *
	 *  ⚠️ IT WRAPS AT 255 -> 0, AND THAT IS HARMLESS AND INTENDED. Only `Attempt & 1` is ever read
	 *  (FSiegeStuckStatics::ComputeSidestepGoal), 256 is even, so the alternation continues
	 *  unbroken across the wrap. ⛔ NOTHING MAY DEPEND ON THIS VALUE MONOTONICALLY — it is not a
	 *  count anyone reports, not a clock, not an index, and it is never compared with >, <, or a
	 *  threshold. A reader wanting a true lifetime tally must add their own counter, not widen this.
	 *
	 *  ⛔ UNREFLECTED, like StuckState and the lease (NAV-§8/§11): an unreflected member cannot be
	 *  replicated by accident. ⛔ Not an FSiegeStuckTuning field either — it is state, not a feel knob.
	 */
	uint8 SidestepAttemptCount = 0;

	/** World-clock timestamp of the previous ConsumeStuckDeltaSeconds call; 0 = never ticked (the first call is inert). */
	float LastStuckTickTimeSeconds = 0.f;

private:

	/**
	 *  Loads DT_Cards and binds the CardID row's stats (HP → max/current HP,
	 *  Speed → MaxWalkSpeed, Damage/Range/Cadence → attack), then starts the
	 *  state timer. Missing table/row/CardID: logs an error and leaves the
	 *  unit Idle — stats are never hardcoded (GDD §3.0).
	 */
	void LoadStatsAndStart();

	/**
	 *  THE SPAWN AUTO-ENROLL (TASK-396; CONVENTIONS §2, the DEFAULT-STANCE law).
	 *  Puts this unit in its owning-team controller's ONE default follow group so
	 *  every follow-eligible Blue unit spawns FOLLOWING and nothing player-side
	 *  auto-engages any more (Jonathan-confirmed: the player personally orders
	 *  every fight; Siege units and the whole bot/Red side are unaffected).
	 *
	 *  Lives on the UNIT, not on a controller call site — that is what covers
	 *  player placement (SpawnUnitSwarm), the ABarracks spawner and SummonTestUnit
	 *  from ONE insertion point — and it is UNCONDITIONAL: a unit spawned after the
	 *  player pressed T still spawns following (reinforcements do NOT inherit the
	 *  last order; flagged, accepted ergonomic consequence).
	 *
	 *  Called from LoadStatsAndStart (see the comment at that call site for why it
	 *  is there and not at the tail of BeginPlay). EVERY refusal is SILENT and
	 *  degrades to today's behavior — a missed enroll must never be a crash and
	 *  never a stall.
	 */
	void TryAutoEnrollInFollowGroup();

	/**
	 *  TASK-044 (CONVENTIONS Team contract): overrides VisualMesh slot 0 with the
	 *  MI_TeamColor matching the unit's ACTUAL Team, so a Red-spawned unit (the M3
	 *  bot's) recolors at runtime without a Red BP duplicate. The BP-authored
	 *  MI_TeamColor_Blue is only the design-time placeholder — a Blue unit re-applies
	 *  the identical Blue instance, so M1/M2 Blue visuals stay byte-for-byte. The two
	 *  MI instances resolve through cached function-local statics (never a per-attack/
	 *  per-frame load) and are null-safe (a missing asset leaves the authored slot,
	 *  never a crash). Inherited by AMinerUnit through Super::BeginPlay — the base
	 *  apply covers miners too. Cosmetic only: slot 0 material, nothing else.
	 */
	void ApplyTeamMaterial();

	/**
	 *  TASK-349 (CONVENTIONS "Castle 3× HOLLOW" team-gating law) — the ONE unit-side
	 *  stamping site, both lanes, driven by the ACTUAL Team like ApplyTeamMaterial:
	 *  (1) PHYSICAL: re-types the capsule's collision OBJECT channel to
	 *      ECC_SiegeTeamBlue/Red (SiegeNavAreas.h) so the ENEMY castle's
	 *      GateBlockerVolume blocks this body at the gate. Object type ONLY — the
	 *      capsule's response matrix is untouched, so every response-based query
	 *      (ECC_Pawn distance math in melee/projectiles/spells/attack range,
	 *      pawn-vs-pawn and pawn-vs-world blocking) is byte-identical.
	 *  (2) PATHING: pushes the team's UNavFilter_Team* onto the possessing
	 *      ASiegeUnitAIController as its default pathfinding filter, so this unit
	 *      never PATHS into the enemy castle's interior area.
	 *  Called from BeginPlay (next to ApplyTeamMaterial), PossessedBy (AutoPossessAI
	 *  can possess after BeginPlay), and InitUnit's post-BeginPlay team update.
	 *  Null-safe/idempotent: no capsule = no stamp; a controller that is not an
	 *  ASiegeUnitAIController (e.g. a BP override pinning plain AAIController) gets
	 *  no filter and keeps pre-feature pathing — the physical lane still gates it.
	 *  AMinerUnit inherits all of it.
	 */
	void ApplyTeamGatingProfile();

	/**
	 *  Skeletal swap (M7, TASK-159): composes /Game/Characters/SK_<CardID> from the
	 *  bound CardID and, if it resolves, makes SkeletalVisualMesh the runtime visual
	 *  (SetSkeletalMeshAsset + SetAnimInstanceClass from /Game/Characters/ABP_<CardID>,
	 *  the static VisualMesh hidden, bUsingSkeletalVisual latched). Missing SK asset =
	 *  keeps the static VisualMesh exactly as today (silent — the normal pre-rig path).
	 *  Missing ABP with a present SK = the skeletal mesh shows its ref pose (null-safe).
	 *  Called once from LoadStatsAndStart, where the CardID is guaranteed bound.
	 */
	void ResolveSkeletalVisual();

	/** The active runtime visual mesh — SkeletalVisualMesh when the skeletal swap took, else the static VisualMesh (team recolor + spawn squash target). */
	UMeshComponent* GetActiveVisualMesh() const;

	/** Periodic state check (every StateCheckInterval): leash/Reacquire, Acquire, then Attack or Advance. */
	void UpdateState();

	/**
	 *  Acquire (GDD §3.8 Standard): nearest alive enemy ITeamAgent within
	 *  AggroRadius. Tie-break: if the nearest candidate is a building/castle
	 *  and the best unit/hero stands within TieBreakDistance of it, the
	 *  unit/hero wins. Returns nullptr when nothing is in range.
	 */
	AActor* AcquireTarget() const;

	/** Nearest standing enemy castle (Team != ours, not IsCastleDestroyed), via actor iteration. */
	AActor* FindNearestEnemyCastle() const;

	/**
	 *  Shield Wall command reshaping (W1 TASK-275): re-targets THIS Standard body per the
	 *  local player's latched stance (PC.GetCurrentCommand). Called from UpdateState ONLY
	 *  when the gate holds — Profile==Standard AND Team==Blue (the player's team) AND
	 *  PC.HasIssuedCommand() — so the LEGACY body below is byte-for-byte unchanged whenever
	 *  the gate is false (bot/Red units, miners, and player units pre-first-command). Never
	 *  bypasses the freeze gating (UpdateState early-returns on frozen before this runs).
	 *    • ATTACK — mirrors the legacy Standard body exactly (TASK-282): self-defense
	 *      AcquireTarget AND the no-in-aggro march goal = the nearest enemy castle.
	 *    • HOLD   — SUPERSEDED (TASK-344): the team-wide Hold stance was replaced by the
	 *      per-unit group orders (UpdateStateGrouped) and nothing latches it any more; a
	 *      stale/out-of-contract Hold value defensively falls through to the ATTACK branch.
	 *    • DEFEND — target = AcquireEnemyNearPoint(own castle centre,
	 *      ResolveDefendEngagementRadius(own castle)); goal = that target ?? fall back to
	 *      the own castle (EnterAdvance). ⚠️ The radius is DERIVED from the castle's live
	 *      colliding bounds (TASK-574) — DefendRadius is the band PAST THE WALL FACE, not
	 *      the disc itself.
	 */
	void UpdateStateStandardCommanded(const ASiegePlayerController& PC);

	/**
	 *  Group-order state body (TASK-344, CONVENTIONS "Group orders — 3-zone HOLD +
	 *  AMBUSH"), dispatched from UpdateState ABOVE the stance gate (grouped behavior
	 *  is per-unit and cannot live in the team-stance body) whenever CommandGroupId
	 *  resolves to a live group. Priority ladder with anti-thrash STICKINESS (the
	 *  TASK-280/282 freeze lesson):
	 *    • the current target is KEPT while alive and zone-valid — acquisition runs
	 *      ONLY when target-less (never a per-tick re-pick);
	 *    • HOLD leash — the tick the target exits BOTH zones it is dropped
	 *      (disengage + return); single MONOTONE exception: a position-tier target
	 *      upgrades to an attack-zone enemy the moment one exists (position→attack
	 *      only — it cannot oscillate);
	 *    • AMBUSH leash-exemption — the zone drop-test is SKIPPED while a live
	 *      target exists (finish the kill), then the ladder resumes;
	 *    • tier 1 acquire = AcquireEnemyNearPoint(attack zone), tier 2 = the
	 *      position zone, tier 3 = advance to the per-unit station (PositionCenter
	 *      + GroupStationOffset) via EnterAdvanceToLocation (which carries the
	 *      TASK-275 kite-fix), idling inside the 150 uu arrival tolerance.
	 */
	void UpdateStateGrouped(const FSiegeUnitGroup& Group);

	/**
	 *  Shield Wall HOLD/DEFEND target search (W1 TASK-275): AcquireTarget's exact
	 *  team-filtered ITeamAgent iteration + pawn/building tie-break, but the eligibility
	 *  gate is a 2D disc — a candidate's LOCATION must lie within Radius of Center —
	 *  instead of AggroRadius-from-self. Nearest-to-self selection + the TieBreakDistance
	 *  rule are kept identical for behavior consistency. Null-safe (no world ⇒ nullptr).
	 */
	AActor* AcquireEnemyNearPoint(const FVector& Center, float Radius) const;

	/**
	 *  Nearest standing OWN-team castle (Team == ours, not destroyed) — the Shield Wall
	 *  DEFEND fallback goal. Mirror of FindNearestEnemyCastle (W1 TASK-275).
	 *
	 *  Deliberately still PRIVATE after TASK-396. It was briefly promoted to public so
	 *  AMinerUnit's DEFEND body could reach it, then reverted: TASK-398 shipped
	 *  ACastle::FindNearestCastleForTeam (the AGoldNode::FindBestMineFor idiom — a
	 *  public static finder on the finder's own type) and no outside caller needs this
	 *  one. FLAGGED, recorded, and NOT taken here: Castle.h notes this function could
	 *  delegate to that finder in a later consolidation pass. It is a ONE-LINE change,
	 *  but the two use a different metric (bounds-aware GetDistanceToTarget here vs
	 *  squared 2D there), and UpdateStateStandardCommanded — this function's only
	 *  caller — is in TASK-396's byte-identical regression set. Not a bugfix to slip
	 *  into a Follow task.
	 */
	ACastle* FindOwnCastle() const;

	/**
	 *  TASK-574 (CONVENTIONS WR-§2b row B) — the ONE conversion from the DEFEND BAND
	 *  (DefendRadius, measured past the wall face) to the CENTRE RADIUS that
	 *  AcquireEnemyNearPoint actually consumes:
	 *
	 *      max(colliding half-extent X, Y) of the own castle  +  DefendRadius
	 *
	 *  read LIVE from the castle actor every time, so the next castle resize carries
	 *  the defence band with it instead of silently deleting the command (the lesson
	 *  the 9× remaster taught, and the same GetActorBounds derivation
	 *  ASiegeGameMode::GetHeroStartTransform branch 3 already ships).
	 *
	 *  ⛔ THREE CONTRACTS THIS FUNCTION IS THE SOLE KEEPER OF — a caller that inlines
	 *  the arithmetic instead breaks all three:
	 *    1. A BAND OF 0 STILL MEANS "NO ACQUISITION", ALWAYS. ASorcererUnit sets
	 *       DefendRadius = 0 as half 1 of its never-attacks seal; a bare
	 *       max(BoxExtent) + 0 would hand it a ~3,657 uu disc and start it picking
	 *       fights. 0 short-circuits to 0 BEFORE any geometry is queried.
	 *    2. bOnlyCollidingComponents = true — the castle's HP-bar widget sits ~9,450 uu
	 *       up at the 9× scale and must never inflate the query. What DEFEND cares
	 *       about is the footprint a besieger is physically stopped by.
	 *    3. NULL / DEGENERATE BOUNDS FALL BACK TO THE AUTHORED VALUE USED AS A PLAIN
	 *       CENTRE RADIUS — i.e. exactly the pre-TASK-574 behaviour, logged once.
	 *       ⛔ Never a zero radius by accident, never a crash.
	 *
	 *  Non-const because it owns the two one-shot log guards below.
	 */
	float ResolveDefendEngagementRadius(const ACastle* OwnCastle);

	/**
	 *  Siege-profile state (TASK-054, GDD §3.8): IGNORE units and the hero;
	 *  target the nearest enemy ABuilding, else the enemy castle, and advance/
	 *  attack it (melee tagged USiegeDamageType_Siege via PerformAttack).
	 */
	void UpdateStateSiege();

	/**
	 *  Support-profile state (TASK-054, GDD §3.8): NEVER attacks; heals the nearest
	 *  DAMAGED friendly within Range and follows the nearest friendly (the damaged
	 *  one if any, else the nearest friendly combat unit; a lone Cleric idles).
	 */
	void UpdateStateSupport();

	/**
	 *  The heal-TARGETING half of UpdateStateSupport, extracted VERBATIM by TASK-396
	 *  so the FOLLOW body can share it: a following Cleric STILL HEALS (manager
	 *  ruling 9 — healing is not attacking, and an escorting medic is the obvious
	 *  intent of a support unit told to follow). Re-picks SupportHealTarget and
	 *  arms/clears the heal timer; returns the target so UpdateStateSupport can keep
	 *  using it as its own follow goal.
	 *
	 *  Statement order is unchanged from the shipped code, so UpdateStateSupport is
	 *  behaviorally byte-identical. The ONE thing that did not move is
	 *  FaceTarget(HealTarget): it stayed at the UpdateStateSupport call site because
	 *  it is wanted there (that Cleric is walking AT its patient) and NOT wanted in
	 *  the follow body (a follower walks to its station, and snapping the yaw at a
	 *  patient behind it every 0.25 s would fight the movement orientation).
	 */
	ASummonedUnit* UpdateSupportHealTargeting();

	/** Nearest standing enemy ABuilding (Team != ours, not destroyed) — the Siege structure search; covers ATower and every ABuilding subclass. */
	AActor* FindNearestEnemyBuilding() const;

	/** Nearest DAMAGED (CurrentHP < MaxHP) friendly ASummonedUnit within AttackRange, excluding self — the Support heal target. */
	ASummonedUnit* FindNearestDamagedFriendly() const;

	/** Nearest friendly COMBAT unit (Profile Standard or Siege — not Support/Miner), excluding self — the Support follow goal when nobody is hurt. */
	ASummonedUnit* FindNearestFriendlyCombatUnit() const;

	/** Arms the Support heal timer if not already running (idempotent); first tick after SupportHealInterval. */
	void StartHealing();

	/** Clears the Support heal timer (idempotent). Called when no heal target is in range, and by FreezeAI / HandleDeath / EndPlay. */
	void StopHealing();

	/** Heal-timer callback (Support): re-validates SupportHealTarget (friendly, alive, damaged, in range) then heals row Damage × SupportHealInterval, clamped to MaxHP. */
	void PerformHeal();

	/** Enters/keeps Attack: stops moving and runs the attack timer at Cadence (first hit respects the elapsed cooldown). */
	void EnterAttack();

	/** Enters/keeps Advance toward Goal: issues the move when the goal changed or path following went idle. Pawn goals chase via MoveToActor; structure goals march to a ResolveStructureMarchPoint location (TASK-349 loop-2 B2). */
	void EnterAdvance(AActor* Goal);

	/**
	 *  TASK-349 loop-2 B2 (march-freeze fix): resolves a STRUCTURE goal (castle /
	 *  building) to a marchable point under the mover's OWN team nav filter. The
	 *  hollow 3× castle put the castle actor's ORIGIN on enemy-interior navmesh,
	 *  which the team filter EXCLUDES — a MoveToActor toward it fails goal-poly
	 *  resolution outright (bAllowPartialPath cannot rescue a goal that never
	 *  resolves to a poly; 123/123 units froze, the TASK-350 re-run's B2). Fix:
	 *  (1) take the nearest point on the goal's ECC_Pawn-blocking collision to
	 *  THIS unit (per-unit — preserves the pre-3× partial-path-to-the-NEAR-wall
	 *  spread; no single-point pile-up), falling back to the actor origin when the
	 *  goal has no blocking collision (shared convention); (2) project it to the
	 *  nearest allowed poly under the possessing controller's
	 *  DefaultNavigationFilterClass (StructureGoalProjectionExtent box) — for an
	 *  enemy castle that lands on the wall-base ring / gate apron OUTSIDE the
	 *  excluded interior, for the OWN castle the interior itself stays allowed.
	 *  Returns false (caller degrades to the legacy MoveToActor — pre-feature
	 *  behavior) when there is no nav system/data or nothing allowed lies within
	 *  the extent. Const, no state; called ONLY from EnterAdvance's existing
	 *  goal-changed/idle re-path gate — never per tick (TASK-280/282 thrash law).
	 */
	bool ResolveStructureMarchPoint(const AActor& Goal, FVector& OutMarchPoint) const;

	/**
	 *  Point variant of EnterAdvance (W1 TASK-275, Shield Wall HOLD): marches toward a
	 *  world LOCATION (not an actor) via AI->MoveToLocation(StructureMoveAcceptanceRadius,
	 *  project-to-nav). (Re)paths only when the point moved meaningfully or path following
	 *  went idle — the EnterAdvance re-path discipline for a point. Clears CurrentMoveGoal
	 *  (so a later actor-advance always re-paths) and tracks the point in
	 *  CurrentMoveGoalLocation. The actor EnterAdvance stays byte-for-byte unchanged.
	 */
	void EnterAdvanceToLocation(const FVector& Point);

	/** Enters Idle: clears the attack timer and stops movement. */
	void EnterIdle();

	/** Attack-timer callback: re-validates the target and range, then applies Damage with team attribution. */
	void PerformAttack();

	/** Yaws the unit toward the target (cosmetic, while attacking). */
	void FaceTarget(const AActor* Target);

	/**
	 *  Ranged delivery (TASK-028): spawns a homing AProjectile at the unit,
	 *  aimed at Target (cosmetic — the projectile re-aims at the target's
	 *  CURRENT location every tick), armed via InitProjectile with our team,
	 *  the row Damage, and USiegeDamageType_Projectile (ACastle applies §3.0's
	 *  50% on ITS side, TASK-026). Spawn sets Instigator = this — the TASK-026
	 *  pawn-shooter rule — so receiver no-friendly-fire checks resolve our
	 *  team through the TASK-002 chain. Null-safe on world/target/spawn.
	 *  DamageAmount is the CENTRALIZED output (TASK-055): row Damage × Charge ×
	 *  Slayer × Aura, computed once in PerformAttack and carried to the projectile
	 *  (Archer/Longbowman with no keywords and no aura pass exactly the row Damage).
	 */
	void FireProjectileAt(AActor* Target, float DamageAmount);

	/**
	 *  Starts ONE lunge cycle from the exact cached rest pose (TASK-020).
	 *  Cycle duration = min(AttackLungeDuration, 0.8 × Cadence). No-op when the
	 *  rest pose is uncached, VisualMesh is missing, or the cycle is degenerate
	 *  (~0 distance or <= 0 duration).
	 */
	void StartAttackLunge();

	/**
	 *  Ends any lunge: restores VisualMesh to EXACTLY the cached rest pose and
	 *  disables tick. Idempotent — called at cycle end, on leaving Attack
	 *  (EnterAdvance/EnterIdle), and on death, guaranteeing zero drift.
	 */
	void StopAttackLunge();

	/** Per-frame lunge driver (runs only while a cycle is active): Base + Distance · sin(π · elapsed / duration) along the capsule's local +X. */
	void UpdateLunge(float DeltaSeconds);

	/**
	 *  ⚠️⚠️ THE COMPOSED TICK PREDICATE (TASK-738) — AND IT IS A REAL TRAP THIS EXISTS TO CLOSE.
	 *  This actor's tick now has TWO independent drivers: the TASK-020 attack lunge and the
	 *  ladder climb. Before this task the lunge owned the tick outright and simply called
	 *  SetActorTickEnabled(false) when it finished — StopAttackLunge does it on EVERY attack
	 *  exit, and UpdateLunge does it again on a stray tick. Either of those would have silently
	 *  KILLED an in-flight climb's driver and left the unit hanging in MOVE_Flying forever with
	 *  every exit path still perfectly correct.
	 *  ⇒ ⛔ NEITHER DRIVER MAY WRITE THE TICK FLAG DIRECTLY ANY MORE. Both go through here, and
	 *  here it is the OR of the two.
	 *
	 *  ⭐ TASK-760 — AND THIS IS ALSO THE **HEALER**: UpdateState re-asserts through this function
	 *  once per 0.25 s state poll while a climb is active, so an external write of the tick flag
	 *  (a BP_Unit_* child, a level Blueprint, tomorrow's C++ site) can no longer hang a climber
	 *  in MOVE_Flying with the watchdog stranded on its dead driver. The decision itself is
	 *  FSiegeLadderClimbStatics::WantsActorTick, which is what makes the re-assert testable and
	 *  what keeps it from EVER writing a value this function would not have written anyway.
	 *
	 *  ⭐ TASK-784 LOOKED HARD AT ADDING A **THIRD** TERM HERE AND DID ⛔ NOT: the contact trigger's
	 *  poll rides StateTimerHandle instead, precisely so this expression stays what TASK-760 pinned.
	 *  ⛔ The expression is UNCHANGED — still exactly `bLungeActive || LadderClimb.bActive`.
	 */
	void RefreshActorTickEnabled();

	// ═════════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ THE CONTACT TRIGGER'S **CALL SITE** (TASK-784, CONTACT-§4.1 / WR-§5)
	//  "just walking up to it and walking against it" — Jonathan, 2026-09-01
	// ═════════════════════════════════════════════════════════════════════════════════════════
	//
	// ⛔⛔ THIS UNIT OWNS ⛔ NONE OF THE RULE AND ⛔ NONE OF THE TUNING. The radius (2D), the intent
	// cone (read as a DIRECTION and ⛔ never a key), the dwell, the endpoint resolution by Z and
	// the K-C re-arm latch ALL live on AClimbableTower and are tested there (TASK-777). ⭐ `WR-§5`'s
	// idiom, fourth application — ACommanderNpc owns InteractRadius + IsPlayerInRange, ships
	// bCanEverTick = false, and is POLLED BY ITS CALLER (SiegePlayerController.cpp:4970). ⇒ **THE
	// PAWN ONLY ASKS.** ⛔ A second copy of any of those numbers here is a hard failure of
	// SiegeClimbableTowerTest.cpp test 12(g), by name — and ⛔ there is not one float in this
	// feature's call site.
	//
	// ⚠️⚠️ AND IT RIDES THE 0.25 s `StateTimerHandle` POLL, ⛔ NOT THE ACTOR TICK — WHICH IS ⛔ NOT
	// A PREFERENCE, IT IS THE ONLY DRIVER THAT RUNS IN THE STATE A CLIMB STARTS FROM. MEASURED:
	// `RefreshActorTickEnabled` (SummonedUnit.cpp) is the ONE writer of this actor's tick flag and
	// its value is `WantsActorTick(bLungeActive, LadderClimb.bActive)` ⇒ a unit that is neither
	// mid-lunge nor ALREADY climbing has its actor tick **OFF** (`bStartWithTickEnabled = false`,
	// SummonedUnit.cpp:139). ⛔ A poll in `::Tick` would compile, review clean, pass the whole
	// suite — AND NEVER RUN, which is the same silent class as the zero-caller gap this task
	// exists to close. ⭐ StateTimerHandle is an FTimerManager entry on the WORLD and knows nothing
	// about PrimaryActorTick, which is exactly why TASK-760's self-heal rides it too.
	//
	// ⚠️ THE DECLARED COST OF THAT CADENCE, MEASURED AND ⛔ NOT PAPERED OVER (full table in
	// Tests/SiegeLadderClimbTest.cpp tests 15 and 16): a pawn walking dead-on at an endpoint holds
	// proximity AND intent for only `LadderContactRadiusUU / Speed` seconds, and a 0.25 s sampler
	// needs TWO samples inside that window. ⇒ at the CURRENTLY shipped 150 uu the trigger is
	// reliable only up to 300 uu/s, is PHASE-DEPENDENT for the Archer (350) and Footman (400), and
	// is UNREACHABLE for Cavalry (600). ⛔ Not fixable from here — the radius is the tower's and
	// `K-5`/`K-6` make it 🧑 Jonathan's. This call site is correct at any radius he picks.
	//
	// ⚠️ A UNIT WITH ⛔ NO STATE POLL NEVER ASKS: AMinerUnit seals `StateCheckInterval = 0`
	// (MinerUnit.cpp:62), and FTimerManager::SetTimer with a rate <= 0 arms NOTHING ⇒ a miner can
	// never contact-climb. ⭐ DECLARED non-coverage — the same limit TASK-760's self-heal records,
	// and here it is also the WANTED answer (qa/TASK-741 W-5: a miner should never be admitted to
	// a ladder in the first place).

	/**
	 *  ⭐⭐ THE WHOLE CALL SITE: find the nearest live ladder and ASK it. Returns true iff a climb
	 *  STARTED, which is the caller's signal to stop running the state machine this poll.
	 *
	 *  Called from UpdateState, one line below TickStuckWatchdog and ABOVE the sidestep-lease
	 *  early-out, so Standard, Siege, Support, Follow and Hold/Ambush are all covered from ONE
	 *  line (the TrackChargeMovement / TickStuckWatchdog placement precedent). ⛔ It is BELOW the
	 *  death/freeze/IsClimbing fence, which is what keeps it from ever asking on behalf of a unit
	 *  that could not climb anyway.
	 *
	 *  ⛔ THE FIND IS ⛔ NOT A SCAN OF ALL ACTORS (the .cpp carries the measurement): a class-hash
	 *  bucket lookup over the live AClimbableTowers, which is ZERO work in every match where
	 *  nobody played the card — ⛔ never the TActorIterator level walk the five other finders in
	 *  this file pay.
	 *
	 *  ⛔ NO TEAM TEST, ⛔ NO ELIGIBILITY TEST AND ⛔ NO PROXIMITY TEST OF ITS OWN. Every one of
	 *  those is `AClimbableTower::EvaluateContactEntry`'s, and a pawn-side copy of the team gate
	 *  would be the silent back door around Jonathan's T-3 ruling that CONTACT-§4.3 exists to
	 *  forbid. This function chooses WHOM to ask and ⛔ nothing else.
	 *
	 *  ⛔⛔ IT CREATES ⛔ NO NINTH EXIT: it never ends a climb, never writes a movement mode, never
	 *  touches LadderClimb and never calls EndLadderClimb / AbortLadderClimb. It also adds ⛔ NO
	 *  SECOND STEERING AUTHORITY — BeginLadderClimb's own step (4) calls
	 *  AAIController::StopMovement() before it takes the capsule, and ⛔ no second stop is issued
	 *  here (NAV-§3's no-double-driver law).
	 *
	 *  @param DeltaSeconds  the poll's own world-clock delta, from ConsumeStuckDeltaSeconds().
	 *                       ⛔ NEVER StateCheckInterval — AMinerUnit seals that to 0, and the dwell
	 *                       the tower integrates out of this value must be real elapsed time.
	 */
	bool TryContactClimbAtNearestLadder(float DeltaSeconds);

	/**
	 *  Per-frame climb driver (TASK-738), runs only while a climb is active. Advances the pure
	 *  state, and either steers one frame of swept movement along the Foot -> Top line
	 *  (AddMovementInput — the capsule, the sweep and depenetration all still apply) or ends the
	 *  climb through the ONE teardown.
	 */
	void TickLadderClimb(float DeltaSeconds);

	/**
	 *  ⭐⭐ THE ONE TEARDOWN — EVERY ONE OF TOWER-§8.5's EIGHT EXITS ARRIVES HERE, AND NOTHING
	 *  ELSE RESTORES THE MOVEMENT MODE OR BROADCASTS OnLadderClimbEnded.
	 *
	 *  ⚠️ THAT SINGULARITY IS THE DESIGN, ⛔ NOT TIDINESS: MOVE_Flying ignores gravity, so an
	 *  exit that forgets the restore leaves a unit hanging in mid-air FOREVER (TOWER-§8.5's
	 *  named regression risk, and the one thing the ramp could not do). Eight call sites each
	 *  remembering to restore is eight chances to be wrong; one branch is one.
	 *
	 *  Idempotent and re-entrancy-safe: the exactly-once latch (FSiegeLadderClimbStatics::End)
	 *  is consumed BEFORE anything else happens, so a double exit (HandleDeath then EndPlay) and
	 *  a listener that aborts from inside the broadcast are both silent no-ops.
	 */
	void EndLadderClimb(bool bReachedTop, ESiegeLadderExit Reason);

	/** Convenience: the possessing AAIController, or nullptr. */
	AAIController* GetAIController() const;

	/** Kills the unit exactly once: clears timers, stops movement, destroys the actor (units don't respawn). */
	void HandleDeath();

	/**
	 *  Resolves the attack/death anim sequences (TASK-165): composes the null-safe soft paths
	 *  /Game/Characters/Anims/A_<CardID>_{Attack,Death} from the bound CardID (mirrors the
	 *  SK_<CardID> / SM_<CardID> string law) and caches whatever resolves. Called from
	 *  ResolveSkeletalVisual only once the skeletal swap took, so un-rigged units never touch it.
	 *  A missing clip caches nullptr and its trigger no-ops — robust across the roster + future units.
	 */
	void CacheActionAnimations();

	/**
	 *  Attack-anim trigger (TASK-165, CODE-ONLY — no ABP/montage asset edit): plays
	 *  A_<CardID>_Attack via USkeletalMeshComponent::PlayAnimation (single-node, non-looping),
	 *  overriding the locomotion ABP for the clip's length, then re-arms RestoreLocomotionAnim so
	 *  idle/walk resume. Null-safe / no-op unless the skeletal runtime, the attack clip, AND a
	 *  locomotion ABP to return to are all present. A faster next attack re-arms the same restore
	 *  timer and simply restarts the clip (continuous swings). Called from PerformAttack for rigged
	 *  units in place of the (now-invisible) procedural lunge.
	 */
	void PlaySkeletalAttackAnim();

	/**
	 *  Swaps the locomotion ABP back onto SkeletalVisualMesh after an attack clip (TASK-165).
	 *  Timer callback for PlaySkeletalAttackAnim AND called on leaving Attack
	 *  (EnterAdvance/EnterIdle/FreezeAI) so a rigged unit returns to walk/idle immediately. No-op
	 *  on a dead unit — death HOLDS its final pose (PlaySkeletalDeathAnim) — or without a skeletal
	 *  runtime / cached locomotion ABP. SetAnimInstanceClass reinitializes out of single-node mode
	 *  and cheaply no-ops when the ABP is already running, so it is safe on every Attack exit.
	 */
	void RestoreLocomotionAnim();

	/**
	 *  Death-anim trigger (TASK-165, CODE-ONLY): plays A_<CardID>_Death (single-node, non-looping)
	 *  which freezes on its final frame — the held death pose — and deliberately does NOT restore
	 *  locomotion. Returns the capped destroy-defer seconds min(clip length, DeathAnimMaxHoldSeconds),
	 *  or 0 when there is no skeletal death clip (caller destroys immediately, unchanged). Called
	 *  from HandleDeath before the (now deferred) Destroy.
	 */
	float PlaySkeletalDeathAnim();

	/** Completes the deferred death removal once the death-anim hold elapses (TASK-165 timer callback). */
	void FinishDeathDestroy();

	/**
	 *  Ends the move-speed buff (TASK-042): clears the buff timer and restores the
	 *  base speed captured at the start of the buff episode EXACTLY. Idempotent —
	 *  a no-op when no buff is active. Timer callback for ApplyMoveSpeedBuff; also
	 *  called by FreezeAI so a match-end freeze leaves zero residual speed.
	 */
	void EndMoveSpeedBuff();

	/**
	 *  Ends the War Banner damage aura (TASK-055): clears the aura timer and restores the
	 *  damage-output multiplier to EXACTLY 1.0. Idempotent — a no-op when no aura is active.
	 *  Timer callback for SetAuraDamageBonus; also called by FreezeAI so a match-end freeze
	 *  leaves zero residual damage buff (mirrors EndMoveSpeedBuff).
	 */
	void EndAuraDamageBuff();

	/**
	 *  Spell-freeze expiry (TASK-099): drops the spell-frozen state, restores the
	 *  default movement mode, and re-arms the state loop (first decision on its
	 *  next tick — never synchronous, preserving AMinerUnit's seal #1) — ONLY
	 *  when the unit is alive, bound, and NOT match-end frozen (M5 ruling 5
	 *  precedence: a spell-freeze expiry must never resume a match-end-frozen
	 *  actor). Timer callback for ApplyFreeze; FreezeAI and EndPlay clear its
	 *  timer.
	 */
	void EndSpellFreeze();

	/**
	 *  Ends the Battle Cry combat buff (TASK-099): clears its timer, resets both
	 *  multipliers to EXACTLY 1, recomposes the walk speed (restoring the shared
	 *  episode base when Rally is also inactive), and re-arms a live attack loop
	 *  at the base cadence. Idempotent — a no-op when no buff is active. Timer
	 *  callback for ApplyCombatBuff; also called by FreezeAI for zero residual.
	 */
	void EndCombatBuff();

	/**
	 *  The ONE walk-speed writer for the buff system (TASK-042/099 composition):
	 *  writes MaxWalkSpeed = SharedBase × RallyMult × CombatMoveMult while any
	 *  speed buff is active, or EXACTLY the shared episode base when none is.
	 *  Only ever called from the Apply/End buff paths, so the base is always a
	 *  captured resting value — zero drift by construction. Null-safe without a
	 *  movement component.
	 */
	void RefreshComposedMoveSpeed();

	/** Attack cadence after the combat buff (TASK-099): AttackCadence ÷ CombatBuffAttackSpeedMult, floored at MinAttackCadence; the plain row cadence when no buff is active. */
	float GetEffectiveAttackCadence() const;

	/**
	 *  Re-arms a LIVE attack loop at the current effective cadence, honoring the
	 *  LastAttackTime cooldown (an already-elapsed cooldown fires on the next
	 *  timer tick, never synchronously from a buff call). No-op when the attack
	 *  timer is not running. Called on combat-buff apply AND expiry so a cadence
	 *  change takes effect mid-Attack instead of waiting for the next re-entry
	 *  (TASK-099).
	 */
	void RearmAttackTimerAtEffectiveCadence();

	/**
	 *  Centralized damage OUTPUT (TASK-055): row Damage × Charge × Slayer × Aura × Ancient
	 *  Grounds × HIGH GROUND, composed in ONE place so every modifier stacks predictably.
	 *  Consumes the primed charge (single hit) and reads Target's MaxHP for the Slayer gate.
	 *  Siege 200% is applied fortification-side by the damage TYPE (TASK-054), NOT here —
	 *  composing it here would double-count. Returns AttackDamage bit-for-bit for a
	 *  non-keyword, un-auraed, MELEE unit (M1/M2 non-regression). Non-const: consumes the
	 *  charge.
	 *  ⭐ HIGH GROUND (TASK-724, HIGH-§3) is composed HERE and ⛔ NOWHERE ELSE, gated on the
	 *  shipped bRangedAttack member — ⛔ never a CardID list and ⛔ never a name check, so a
	 *  future ranged card inherits the behaviour from its own data row with zero code.
	 */
	float ComputeOutputDamage(const AActor* Target);

	/**
	 *  CHARGE bookkeeping (TASK-055), called at the top of every UpdateState: accumulates
	 *  uninterrupted advancing time and primes the charge at ChargeMoveSeconds; a stall (blocked)
	 *  or a full stop (Idle) drops it. No-op unless bCharge (Cavalry) — every other unit is
	 *  byte-unchanged. Deliberately does NOT reset on entering Attack (tracking and the state
	 *  dispatch share one UpdateState — resetting here would race the very hit that should be
	 *  charged); the charge is consumed exactly once in ComputeOutputDamage.
	 */
	void TrackChargeMovement();

	/**
	 *  SUICIDE keyword (TASK-055, Sapper — a Siege + bSuicide unit): the single detonation.
	 *  Applies the AoE blast (ApplyDetonation) then kills the unit (HandleDeath) — no melee, no
	 *  repeat attacks. Triggered by UpdateStateSiege on reaching attack range; HandleDeath also
	 *  detonates if the Sapper is killed en route, and the bDetonated guard keeps it to one blast.
	 */
	void Detonate();

	/**
	 *  Applies the Sapper's one AoE blast at its location: row Damage over row AoERadius,
	 *  USiegeDamageType_Siege (200% vs castle/buildings), enemies only (FSiegeCombatStatics::
	 *  ApplyRadialDamage). Guarded by bDetonated so a contact-trigger and a death-trigger can
	 *  never double-blast. Nothing hardcoded — Damage/AoERadius come from the DT_Cards row.
	 */
	void ApplyDetonation();

	/** Max HP of a damage target across the known combat types (unit/castle/building/hero); 0 for unknown — the Slayer HP gate. */
	static float GetTargetMaxHP(const AActor* Target);

	/**
	 *  True if Target is a live combatant: valid, castle not destroyed
	 *  (IsCastleDestroyed), hero not dead (IsDead), unit not dead. Unknown
	 *  ITeamAgent types (M2 towers/walls) are treated as alive.
	 */
	static bool IsTargetAlive(const AActor* Target);

	/**
	 *  Distance from a point to the CLOSEST POINT on the target's collision
	 *  (mirrors the hero melee, TASK-003) so large-footprint targets — the
	 *  castle's 7313.7 × 7384.5 uu base, origin at center — measure from their
	 *  walls. Falls back to the actor origin when no usable collision exists.
	 *  ⚠️ CITATION CORRECTED BY TASK-574 (uncited instance of WR-§2b row G, found by
	 *  this task's SC-§22 shape sweep): this line quoted "~800x800", the M1 castle,
	 *  and was already two remasters stale. ✅ NO CODE CHANGE IS OWED — measuring to
	 *  the closest collision point is scale-INDEPENDENT by construction, which is
	 *  precisely why this helper never rotted the way DefendRadius did.
	 *  TODO(post-TASK-028, qa/TASK-026-report.md NIT-4): three hand-mirrors of
	 *  this closest-point pattern exist (hero melee inline, this pair,
	 *  AProjectile::GetDistanceToTarget) — consolidate into ONE shared static
	 *  on the next wave that owns all three files; never add a fourth mirror.
	 */
	static float GetDistanceToTarget(const FVector& From, const AActor* Target);

	/**
	 *  As above, additionally returning the closest point itself — the impact-VFX
	 *  contact point (TASK-020). OutClosestPoint falls back to the target's actor
	 *  location when no usable collision exists (ZeroVector only for a null target,
	 *  in which case the returned distance is float-max and callers bail).
	 */
	static float GetDistanceToTarget(const FVector& From, const AActor* Target, FVector& OutClosestPoint);

	/**
	 *  Resolves the attacking team from a damage event — same chain as
	 *  ACastle (TASK-002): instigating controller's pawn, then the damage
	 *  causer, then the causer's instigator pawn. False if no team found
	 *  (world damage), in which case damage applies.
	 */
	static bool TryGetDamageTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam);

	/** Maximum hit points, bound from the card row at BeginPlay (Footman: 80). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	float MaxHP = 0.f;

	/** Current hit points. Mutated only by TakeDamage. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 0.f;

	/** Damage per attack, from the card row (Footman: 12). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	float AttackDamage = 0.f;

	/** Attack range in units, from the card row (Footman: 120). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 0.f;

	/** Seconds between attacks, from the card row (Footman: 1.0). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	float AttackCadence = 1.f;

	/** True when this row's attack is delivered by a homing projectile (row bRanged — Archer). False (melee) leaves the M1 path untouched (TASK-028). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	bool bRangedAttack = false;

	/** CHARGE keyword flag, bound from the row (TASK-055 — Cavalry). Non-charge units never track/prime, so they are byte-unchanged. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	bool bCharge = false;

	/** SLAYER keyword flag, bound from the row (TASK-055 — Pikeman). ×2 vs targets with MaxHP >= SlayerHPThreshold. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	bool bSlayer = false;

	/** SUICIDE keyword flag, bound from the row (TASK-055 — Sapper). Detonates once on contact/death, then dies. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	bool bSuicide = false;

	/** Blast radius for the bSuicide detonation, bound from the row AoERadius (TASK-055 — Sapper 250). 0 for non-AoE units. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	float AoERadius = 0.f;

	/** Targeting/behavior profile bound from the card row (TASK-054): Standard (M1/M2 body), Siege, or Support. None (miners) runs the Standard body — AMinerUnit seals its own machine. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	ECardProfile Profile = ECardProfile::Standard;

	/** Current state-machine mode. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	ESummonedUnitState State = ESummonedUnitState::Idle;

	/** Acquired combat target (nullptr while purely advancing on the castle). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> CurrentTarget;

	/** Goal of the last MoveToActor request — avoids re-pathing every state check. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentMoveGoal;

	/** Group order this unit belongs to (TASK-344), INDEX_NONE = none. Set by AssignCommandGroup (controller, eligibility-gated); cleared by ClearCommandGroup and the null-group self-heal in UpdateState. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	int32 CommandGroupId = INDEX_NONE;

	/** Precomputed per-unit station offset from the group's PositionCenter (TASK-344): the golden-angle sunflower slot, computed + nav-projected ONCE by the controller at the stage-3 confirm — never recomputed per tick. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	FVector GroupStationOffset = FVector::ZeroVector;

	/**
	 *  ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — accumulated permanent damage stacks,
	 *  in [0, MaxPermanentDamageStacks]. Written ONLY by AddPermanentDamageStacks and
	 *  ClearPermanentDamageStacks, each of which broadcasts OnDamageBoostChanged — that
	 *  two-writer discipline is what makes "broadcast on every mutation" auditable.
	 *
	 *  ⚠️ INTEGER STACKS, NEVER A FLOAT (CONVENTIONS §4, and it is load-bearing for the UI):
	 *  the boost bar's whole job is distinguishing EXACTLY 100/200/300% from just-past-them,
	 *  and a float accumulated over 80 additions of 0.05 makes "exactly 100%" epsilon-
	 *  dependent. Integers make the band boundaries exact by construction. Mirrors
	 *  AHeroCharacter::SharpenedBladeStacks.
	 *
	 *  ⚖️ M8 P2 DUTY (recorded, not silently omitted — CONVENTIONS §4 + the NET RELEVANCY
	 *  LAW's "P2 WAVE DUTY: the unit fleet is Tier B"): when the unit fleet replicates this
	 *  becomes UPROPERTY(ReplicatedUsing = OnRep_PermanentDamageStacks), with the OnRep
	 *  re-broadcasting OnDamageBoostChanged so the client's seed-then-bind path is identical
	 *  to the server's. In M8 P1 units are server-only, so no remote client sees a boost bar
	 *  at all — a KNOWN P1 state, not a defect of this task.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	int32 PermanentDamageStacks = 0;

	/** Goal point of the last EnterAdvanceToLocation request (W1 TASK-275, HOLD) — avoids re-pathing to the same point every check. Meaningful only while bHasMoveGoalLocation. */
	FVector CurrentMoveGoalLocation = FVector::ZeroVector;

	/** True once a location move has been issued (W1 TASK-275); gates the CurrentMoveGoalLocation "changed?" comparison so the first point-move always paths. */
	bool bHasMoveGoalLocation = false;

	/**
	 *  THE ANTI-REPATH LATCH (TASK-396, manager ruling 10): the station
	 *  UpdateStateFollow last ISSUED a move to. The follow body re-issues only once
	 *  the recomputed station has drifted further than
	 *  ASiegePlayerController::FollowRepathTolerance from THIS point.
	 *
	 *  ⚠️ A SEPARATE LATCH FROM CurrentMoveGoalLocation ABOVE, on purpose.
	 *  EnterAdvanceToLocation's own guard is a 1 uu Equals test, which a station
	 *  recomputed from a WALKING hero clears on every 0.25 s tick — that is precisely
	 *  the mill behind TASK-280/282 — and CurrentMoveGoalLocation is also written by
	 *  the HOLD/AMBUSH tier-3 body, so reusing it would compare against another
	 *  order's point. Per-unit scalars only (no arrays on units); reset by
	 *  AssignCommandGroup / ClearCommandGroup and whenever the body idles.
	 */
	FVector LastFollowGoalLocation = FVector::ZeroVector;

	/** True once UpdateStateFollow has issued a move this order; gates the LastFollowGoalLocation drift test so the first follow move (and every resume) always paths. */
	bool bHasFollowGoalLocation = false;

	/**
	 *  Hard cache of AttackImpactEffect, resolved ONCE at BeginPlay (TASK-020) —
	 *  keeps the Niagara system alive against GC and avoids per-attack sync loads.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CachedAttackImpactEffect;

	/**
	 *  Locomotion ABP resolved by ResolveSkeletalVisual (TASK-165 / -159 fallback): the class
	 *  SkeletalVisualMesh runs for idle/walk. Cached so RestoreLocomotionAnim can swap it back
	 *  after a single-node attack clip. Null when no ABP resolved (the ref-pose case) — the
	 *  attack trigger is gated on this being set so it can always cleanly return.
	 */
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> LocomotionAnimClass;

	/**
	 *  Attack/death anim sequences resolved ONCE by CacheActionAnimations from the CardID
	 *  (TASK-165). UPROPERTY(Transient) keeps them alive against GC between attacks (mirrors
	 *  CachedAttackImpactEffect); nullptr = no clip at that path, the trigger no-ops (null-safe).
	 */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedAttackAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedDeathAnim;

	/** BP-authored VisualMesh rest pose (RELATIVE location), cached once at BeginPlay — the lunge only ever writes Base + f(elapsed) or exactly Base. */
	FVector VisualMeshBaseRelativeLocation = FVector::ZeroVector;

	/** True once the rest pose was cached at BeginPlay; the lunge never runs without it. */
	bool bVisualMeshBaseCached = false;

	/** True while a lunge cycle is animating — one of the TWO inputs to RefreshActorTickEnabled (TASK-738; it is no longer the only tick driver). */
	bool bLungeActive = false;

	/**
	 *  Transient state of an in-flight ladder climb (TASK-738). ⛔ Deliberately UNREFLECTED
	 *  (the StuckState precedent, NAV-§8/§11): an unreflected struct cannot be replicated by
	 *  accident, which is what makes this batch's M8 "no replicated property" declaration
	 *  survive a later refactor rather than merely be true today.
	 *  ⭐ Its bActive field IS IsClimbing() and IS the TOWER-§9 disarm term — one bool, so the
	 *  disarm cannot outlive the climb by construction (TOWER-§9.2: "the disarm ends the
	 *  INSTANT the unit reaches the deck" — ⛔ no decay timer, ⛔ no grace window, and there is
	 *  no second flag that could be left set).
	 */
	FSiegeLadderClimbState LadderClimb;

	/**
	 *  MaxFlySpeed as it was before the climb, restored EXACTLY at the teardown (the TASK-020 /
	 *  TASK-042 zero-residual discipline). Meaningful only between Begin and End.
	 *  ⭐ MaxFlySpeed is the one speed field NOTHING else in this project writes (grepped:
	 *  Rally, Battle Cry and the row bind all compose MaxWalkSpeed), so borrowing it cannot
	 *  collide with RefreshComposedMoveSpeed's buff episode — which is precisely the drift trap
	 *  those two buffs already exist to avoid.
	 */
	float LadderClimbSavedMaxFlySpeed = 0.f;

	/** Seconds into the current lunge cycle. */
	float LungeElapsed = 0.f;

	/** Duration of the current cycle: min(AttackLungeDuration, 0.8 × Cadence), fixed at cycle start. */
	float LungeCycleDuration = 0.f;

	/** True once the card stats were bound from DT_Cards; the state machine only runs afterwards. */
	bool bStatsLoaded = false;

	/** True once the M7 skeletal swap took (TASK-159): SkeletalVisualMesh is the runtime visual and the team recolor / spawn squash target it, not the static VisualMesh. */
	bool bUsingSkeletalVisual = false;

	/** True from HP hitting 0; death side effects run exactly once. */
	bool bDead = false;

	/** True once FreezeAI ran (TASK-028 match-end freeze): the unit idles until destroyed — stat binding and both timer callbacks are gated on this. */
	bool bAIFrozen = false;

	/** One-shot guard for the missing-AIController warning. */
	bool bWarnedNoAIController = false;

	/**
	 *  TASK-574 one-shot Log guard, once per UNIT LIFETIME and never re-armed — the
	 *  AMinerUnit::bLoggedInteriorAnchor idiom verbatim (Play Again destroys units, so a
	 *  second match is a second unit and logs again). Prints the castle's measured
	 *  colliding half-width, the authored band and the resolved engagement radius the
	 *  first time this unit resolves a DEFEND disc. ⚠️ THIS IS THE ACCEPTANCE INSTRUMENT:
	 *  it is the only place the derivation is readable back from a PIE log with no editor
	 *  probe, and one line per defending unit is the honest cost of that (CONVENTIONS §5
	 *  "MEASURE, DO NOT ASSUME").
	 */
	bool bLoggedDefendBandDerived = false;

	/**
	 *  TASK-574 one-shot Warning guard for the DEFEND-band FALLBACK path (no own castle,
	 *  or degenerate colliding bounds ⇒ the authored DefendRadius is used as a plain
	 *  centre radius, i.e. pre-TASK-574 behaviour). ONE flag covers BOTH causes on
	 *  purpose: they are both static over a unit's lifetime and they produce the IDENTICAL
	 *  behaviour, so a second flag could only ever add a duplicate line; the message names
	 *  which cause fired.
	 */
	bool bLoggedDefendBandFallback = false;

	/** World time of the last landed attack — keeps the cadence honest across target swaps and range flapping. */
	double LastAttackTime = -1.0e9;

	/** Drives UpdateState every StateCheckInterval seconds. */
	FTimerHandle StateTimerHandle;

	/** Drives PerformAttack every AttackCadence seconds while in Attack. */
	FTimerHandle AttackTimerHandle;

	/** Support profile (Cleric): the friendly ASummonedUnit currently being healed (weak — the healer never owns its lifetime; re-validated each heal tick). */
	TWeakObjectPtr<ASummonedUnit> SupportHealTarget;

	/** Drives PerformHeal every SupportHealInterval while a damaged friendly is in range (Support profile). Cleared by StopHealing / FreezeAI / HandleDeath / EndPlay. */
	FTimerHandle HealTimerHandle;

	/** True while a temporary move-speed buff is active (TASK-042 Rally). */
	bool bMoveSpeedBuffActive = false;

	/**
	 *  Resting MaxWalkSpeed captured ONCE when the current buff episode began. The
	 *  buffed speed is only ever written as Base × Multiplier, and the buff ends by
	 *  restoring EXACTLY this value — so no number of refreshes can drift the base
	 *  (the TASK-020 cache-once/restore-exactly contract, applied to walk speed).
	 */
	float MoveSpeedBuffBaseSpeed = 0.f;

	/** Drives EndMoveSpeedBuff once the buff Duration elapses; re-armed (refreshed) on re-apply, never stacked. */
	FTimerHandle MoveSpeedBuffTimerHandle;

	/**
	 *  Rally's active walk-speed multiplier (TASK-042; exactly 1 while inactive).
	 *  Composed with the combat buff by RefreshComposedMoveSpeed (TASK-099) —
	 *  never written to MaxWalkSpeed directly anymore, so Rally and Battle Cry
	 *  stack without either restore clobbering the other.
	 */
	float MoveSpeedBuffMultiplier = 1.f;

	/** True while a Battle Cry combat buff is active (TASK-099). */
	bool bCombatBuffActive = false;

	/** Battle Cry walk-speed multiplier (exactly 1 while inactive); composed with Rally's by RefreshComposedMoveSpeed (TASK-099). */
	float CombatBuffMoveSpeedMult = 1.f;

	/** Battle Cry attack-speed multiplier (exactly 1 while inactive); effective cadence = AttackCadence ÷ this (TASK-099). */
	float CombatBuffAttackSpeedMult = 1.f;

	/** Drives EndCombatBuff once the buff Seconds elapse; re-armed (refreshed) on re-apply, never stacked (TASK-099). */
	FTimerHandle CombatBuffTimerHandle;

	/**
	 *  True while a FrostNova spell freeze is active (TASK-099) — RESUMABLE,
	 *  unlike the permanent match-end bAIFrozen latch. Gates UpdateState /
	 *  PerformAttack / PerformHeal as defense-in-depth (the bAIFrozen pattern).
	 */
	bool bSpellFrozen = false;

	/**
	 *  Drives EndSpellFreeze once the freeze elapses; re-armed at
	 *  max(remaining, new) on re-apply (refresh-not-stack, M5 ruling 5). Cleared
	 *  by FreezeAI (match-end precedence — no expiry may fire post-match) and
	 *  EndPlay.
	 */
	FTimerHandle SpellFreezeTimerHandle;

	/** Restores the locomotion ABP after a single-node attack clip (TASK-165); re-armed on each attack, cleared on death/leaving-Attack. */
	FTimerHandle AttackAnimRestoreTimerHandle;

	/** Defers Destroy while a rigged unit's death anim holds its final pose (TASK-165); a hard cap so a corpse never lingers. */
	FTimerHandle DeathDestroyTimerHandle;

	/** Accumulated uninterrupted-advance time for CHARGE (TASK-055); reset on stall / stop / consume. */
	float ChargeMoveElapsed = 0.f;

	/** True once >= ChargeMoveSeconds of continuous movement has been logged; consumed by the next attack (CHARGE). */
	bool bChargePrimed = false;

	/**
	 *  War Banner damage-output multiplier (TASK-055): 1.0 = no aura. Set to (1 + Bonus) by
	 *  SetAuraDamageBonus and reset to EXACTLY 1.0 on expiry. Stored SEPARATELY from AttackDamage
	 *  and composed at strike time (ComputeOutputDamage), so it can never drift the row-bound base.
	 */
	float AuraDamageMultiplier = 1.f;

	/** True while a War Banner damage aura is active (TASK-055). */
	bool bAuraDamageBuffActive = false;

	/** Drives EndAuraDamageBuff once the aura Duration elapses; re-armed (refreshed) on re-apply, never stacked. */
	FTimerHandle AuraDamageBuffTimerHandle;

	/** True once the bSuicide detonation has fired (TASK-055) — guarantees exactly ONE blast (contact OR death, never both). */
	bool bDetonated = false;
};
