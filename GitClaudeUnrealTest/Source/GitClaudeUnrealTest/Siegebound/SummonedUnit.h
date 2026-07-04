// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "SummonedUnit.generated.h"

class AAIController;
class UDataTable;
class UNiagaraSystem;
class UStaticMeshComponent;

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
 *  Siegebound summoned unit, Standard targeting profile (GDD §3.8 + §4).
 *
 *  ACharacter driven by a plain AAIController (AutoPossessAI
 *  PlacedInWorldOrSpawned) walking the navmesh via MoveToActor.
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
 *    Inherited by AMinerUnit (TASK-025) — the base also stops its walk.
 *
 *  Spawners (TASK-007): prefer SpawnActorDeferred → InitUnit(Team, CardID) →
 *  FinishSpawning, so BeginPlay binds the right card. InitUnit also works
 *  after a plain SpawnActor (it late-binds the stats if BeginPlay found no CardID).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASummonedUnit : public ACharacter, public ITeamAgent
{
	GENERATED_BODY()

public:

	ASummonedUnit();

	//~ Begin ITeamAgent Interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent Interface

	/** Applies incoming damage (no friendly fire, GDD §3.0) and destroys the unit at 0 HP. */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 *  Drives ONLY the attack-lunge visual (TASK-020). The state machine stays
	 *  timer-driven (TASK-004: never per-tick); tick starts disabled and is
	 *  enabled solely while a lunge cycle is animating.
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
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	void ApplyMoveSpeedBuff(float Multiplier, float Duration);

	/**
	 *  Match-end freeze (TASK-028; called by the game mode at match end per the
	 *  TASK-024 contract, and inherited by AMinerUnit — the base StopMovement
	 *  also halts its gold-node walk, TASK-025). Permanently stops the unit's
	 *  AI: clears the state (acquire) and attack timers, cancels any in-flight
	 *  lunge and restores VisualMesh to EXACTLY the cached rest pose (zero
	 *  residual offset), stops movement, and parks the unit in Idle until it is
	 *  destroyed. Also ends any active move-speed buff (TASK-042), clearing its
	 *  timer and restoring the base speed with zero residual. Idempotent; safe on
	 *  dead or never-bound units. A frozen unit can never restart: stat binding and
	 *  both timer callbacks are gated on the frozen flag. Virtual so subclasses with
	 *  extra drives can extend it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
	virtual void FreezeAI();

	/** True once FreezeAI ran (PIE verification hook for the TASK-024 match-end freeze). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
	bool IsAIFrozen() const { return bAIFrozen; }

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

protected:

	/** Binds the card stats from DT_Cards and starts the state machine. */
	virtual void BeginPlay() override;

	/** Clears the state/attack timers. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  Visual slot for the blueprint child (TASK-010: BP_Unit_Footman assigns
	 *  /Game/Meshes/SM_Footman). No mesh is set in C++. The capsule owns all
	 *  collision; this component never collides or affects navigation.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** DT_Cards row name whose stats drive this unit (BP_Unit_Footman sets Footman, TASK-010). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
	FName CardID = NAME_None;

	/** Team this unit fights for. Spawner sets it per unit (player summons are Blue). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Card stat table (GDD §3.0). May not be imported yet (TASK-008) — resolved null-safe at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Unit")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/** Acquisition radius in units (GDD §3.8 profile constant: 600 — not a card stat). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0"))
	float AggroRadius = 600.f;

	/** Leash: a target beyond this distance is dropped and Advance resumes (GDD §3.8: 900). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0"))
	float LeashRange = 900.f;

	/** If the best unit/hero stands within this distance of the nearest building/castle, prefer the unit/hero (GDD §3.8: 100). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0"))
	float TieBreakDistance = 100.f;

	/** Seconds between state-machine checks (spec: ~0.25 s, never per-tick). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|AI", meta = (ClampMin = "0.05"))
	float StateCheckInterval = 0.25f;

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

private:

	/**
	 *  Loads DT_Cards and binds the CardID row's stats (HP → max/current HP,
	 *  Speed → MaxWalkSpeed, Damage/Range/Cadence → attack), then starts the
	 *  state timer. Missing table/row/CardID: logs an error and leaves the
	 *  unit Idle — stats are never hardcoded (GDD §3.0).
	 */
	void LoadStatsAndStart();

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

	/** Enters/keeps Attack: stops moving and runs the attack timer at Cadence (first hit respects the elapsed cooldown). */
	void EnterAttack();

	/** Enters/keeps Advance toward Goal: issues MoveToActor when the goal changed or path following went idle. */
	void EnterAdvance(AActor* Goal);

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
	 */
	void FireProjectileAt(AActor* Target);

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

	/** Convenience: the possessing AAIController, or nullptr. */
	AAIController* GetAIController() const;

	/** Kills the unit exactly once: clears timers, stops movement, destroys the actor (units don't respawn). */
	void HandleDeath();

	/**
	 *  Ends the move-speed buff (TASK-042): clears the buff timer and restores the
	 *  base speed captured at the start of the buff episode EXACTLY. Idempotent —
	 *  a no-op when no buff is active. Timer callback for ApplyMoveSpeedBuff; also
	 *  called by FreezeAI so a match-end freeze leaves zero residual speed.
	 */
	void EndMoveSpeedBuff();

	/**
	 *  True if Target is a live combatant: valid, castle not destroyed
	 *  (IsCastleDestroyed), hero not dead (IsDead), unit not dead. Unknown
	 *  ITeamAgent types (M2 towers/walls) are treated as alive.
	 */
	static bool IsTargetAlive(const AActor* Target);

	/**
	 *  Distance from a point to the CLOSEST POINT on the target's collision
	 *  (mirrors the hero melee, TASK-003) so large-footprint targets — the
	 *  castle's ~800x800 base, origin at center — measure from their walls.
	 *  Falls back to the actor origin when no usable collision exists.
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

	/** Current state-machine mode. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	ESummonedUnitState State = ESummonedUnitState::Idle;

	/** Acquired combat target (nullptr while purely advancing on the castle). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Unit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> CurrentTarget;

	/** Goal of the last MoveToActor request — avoids re-pathing every state check. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentMoveGoal;

	/**
	 *  Hard cache of AttackImpactEffect, resolved ONCE at BeginPlay (TASK-020) —
	 *  keeps the Niagara system alive against GC and avoids per-attack sync loads.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CachedAttackImpactEffect;

	/** BP-authored VisualMesh rest pose (RELATIVE location), cached once at BeginPlay — the lunge only ever writes Base + f(elapsed) or exactly Base. */
	FVector VisualMeshBaseRelativeLocation = FVector::ZeroVector;

	/** True once the rest pose was cached at BeginPlay; the lunge never runs without it. */
	bool bVisualMeshBaseCached = false;

	/** True while a lunge cycle is animating — actor tick is enabled exactly then. */
	bool bLungeActive = false;

	/** Seconds into the current lunge cycle. */
	float LungeElapsed = 0.f;

	/** Duration of the current cycle: min(AttackLungeDuration, 0.8 × Cadence), fixed at cycle start. */
	float LungeCycleDuration = 0.f;

	/** True once the card stats were bound from DT_Cards; the state machine only runs afterwards. */
	bool bStatsLoaded = false;

	/** True from HP hitting 0; death side effects run exactly once. */
	bool bDead = false;

	/** True once FreezeAI ran (TASK-028 match-end freeze): the unit idles until destroyed — stat binding and both timer callbacks are gated on this. */
	bool bAIFrozen = false;

	/** One-shot guard for the missing-AIController warning. */
	bool bWarnedNoAIController = false;

	/** World time of the last landed attack — keeps the cadence honest across target swaps and range flapping. */
	double LastAttackTime = -1.0e9;

	/** Drives UpdateState every StateCheckInterval seconds. */
	FTimerHandle StateTimerHandle;

	/** Drives PerformAttack every AttackCadence seconds while in Attack. */
	FTimerHandle AttackTimerHandle;

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
};
