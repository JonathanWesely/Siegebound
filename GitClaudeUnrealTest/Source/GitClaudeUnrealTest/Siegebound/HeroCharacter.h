// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GitClaudeUnrealTestCharacter.h"
#include "Siegebound/TeamId.h"
#include "Templates/SubclassOf.h"
#include "HeroCharacter.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UInputAction;
class UInputMappingContext;
class UNiagaraSystem;
class AHeroCharacter;

/**
 *  Broadcast exactly once each time the hero dies (HP reaches 0).
 *  The game mode (TASK-006) binds here to drive respawn timing — the hero
 *  itself knows nothing about respawn schedules (loose coupling, GDD §3.1).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroDied, AHeroCharacter*, DeadHero);

/**
 *  Broadcast on every Rally cooldown-state change (TASK-042, GDD §4):
 *  - on a successful Rally: (bReady=false, CooldownRemaining=RallyCooldown)
 *  - when the cooldown elapses:  (bReady=true,  CooldownRemaining=0)
 *  - (optional refusal) on a press during cooldown: (bReady=false, remaining).
 *  The HUD (TASK-050) binds here to drive the Rally readiness indicator; the hero
 *  itself owns no UI (loose coupling, mirrors OnHeroDied / the CONVENTIONS delegate law).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRallyStateChanged, bool, bReady, float, CooldownRemaining);

/**
 *  Siegebound player hero (GDD §3.1).
 *
 *  Subclasses the abstract third-person template character to inherit the
 *  camera boom and Move/Look/Jump plumbing (template files untouched).
 *
 *  - Team Blue by default; implements ITeamAgent (shared friendly-fire check).
 *  - Walks at 500 u/s, sprints at 750 u/s while IA_Sprint is held.
 *  - Melee on IA_Attack: MeleeDamage to ALL enemy ITeamAgent actors within
 *    MeleeRange AND inside a ±MeleeHalfAngleDegrees forward cone, rate-limited
 *    to one swing per MeleeCooldown seconds. No friendly fire.
 *  - Attack feedback (TASK-016, playtest R1): AttackMontage on every
 *    non-suppressed swing that passes the cooldown (hit or whiff), HitImpactEffect
 *    per enemy actually damaged, HitCameraShake once per swing that damaged >= 1
 *    enemy. Purely visual — damage timing/numbers never depend on any of it; all
 *    four assets are optional (wired on BP_HeroCharacter in TASK-017).
 *  - 200 max HP; regenerates RegenRate HP/s starting RegenDelay seconds after
 *    last taking OR dealing damage, stopping at max.
 *  - At 0 HP: hidden, input + collision disabled, OnHeroDied broadcast once.
 *    ResetHero() restores the hero (called by the game mode on respawn/Play Again).
 *  - Falling past the world's KillZ (TASK-036 arena boundary) is a DEATH, not
 *    a Destroy: FellOutOfWorld routes into the same path as lethal damage, so
 *    the standard 5 s respawn brings the hero back (TASK-024, M1 carry-over).
 *  - SetMeleeSuppressed(true) disables melee while the placement mode owns
 *    the LMB (TASK-007).
 *
 *  Input assets (IMC_Hero, IA_Sprint, IA_Attack) are assigned on the derived
 *  blueprint BP_HeroCharacter in TASK-009; every input reference is null-safe
 *  so the raw C++ class also runs (game mode fallback pawn, TASK-006).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AHeroCharacter : public AGitClaudeUnrealTestCharacter, public ITeamAgent
{
	GENERATED_BODY()

public:

	AHeroCharacter();

	/** Fired exactly once per death. The game mode (TASK-006) binds here for respawn timing. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnHeroDied OnHeroDied;

	/** Fired on every Rally cooldown-state change (TASK-042). The HUD (TASK-050) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnRallyStateChanged OnRallyStateChanged;

	//~ Begin ITeamAgent Interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent Interface

	/** Applies incoming damage (no friendly fire), tracks combat time for regen, and triggers death at 0 HP. */
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 *  KillZ handler (M1 "sprints off the slab and falls forever" carry-over,
	 *  closed by TASK-024; TASK-036 sets L_Arena's KillZ = -2000 and the
	 *  boundary volumes). A hero falling out of the world dies through the
	 *  EXACT combat-death path — HandleDeath() hides it, stops movement and
	 *  input, and broadcasts OnHeroDied once, so the game mode's standard 5 s
	 *  respawn (§3.1) brings it back at its castle. Deliberately does NOT call
	 *  Super: AActor::FellOutOfWorld() would Destroy() the pawn, and the hero
	 *  must survive falling off the world. The engine re-checks per movement
	 *  tick while an actor sits below KillZ, so repeat calls on the hidden
	 *  corpse early-out on the death latch until the respawn teleports it back
	 *  above ground (or PlayAgain does, if the match has ended).
	 */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/**
	 *  Performs the melee swing if allowed (alive, not suppressed, off cooldown):
	 *  MeleeDamage to every enemy ITeamAgent within MeleeRange and inside the
	 *  ±MeleeHalfAngleDegrees forward cone. Bound to IA_Attack; also callable
	 *  from blueprint/UI for testing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Combat")
	void DoMeleeAttack();

	/**
	 *  While true, DoMeleeAttack is a no-op (does not even consume the cooldown).
	 *  TASK-007's placement mode sets this while the LMB confirms card placement.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Combat")
	void SetMeleeSuppressed(bool bSuppressed) { bMeleeSuppressed = bSuppressed; }

	/** True while placement mode (TASK-007) owns the LMB. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
	bool IsMeleeSuppressed() const { return bMeleeSuppressed; }

	/**
	 *  Hero active ability (GDD §4, bound to IA_Rally in TASK-048; also callable from
	 *  blueprint/UI). If off cooldown: buffs every friendly (same-team) ASummonedUnit
	 *  within RallyRadius by +RallySpeedBonus move speed for RallyDuration seconds
	 *  (via ApplyMoveSpeedBuff), starts the RallyCooldown, and broadcasts
	 *  OnRallyStateChanged(false, RallyCooldown); a second broadcast (true, 0) fires
	 *  when the cooldown elapses. Does NOTHING to the hero's own speed and never
	 *  touches enemy units. A press while on cooldown is a no-op (optional refusal
	 *  broadcast). No-op while dead. Null-safe with no world/units present.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Combat")
	void Rally();

	/**
	 *  Restores the hero to a playable state after death or Play Again (GDD §3.9):
	 *  full HP, visible, collision + movement + input re-enabled.
	 *  Respawn timing and placement belong to the game mode (TASK-006) — it moves
	 *  the hero (or respawns the pawn) and then calls this.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero")
	void ResetHero();

	/** Current hit points, in [0, MaxHP]. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	float GetCurrentHP() const { return CurrentHP; }

	/** Maximum hit points (GDD §3.1: 200). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	float GetMaxHP() const { return MaxHP; }

	/** True from the moment HP hits 0 until ResetHero() is called. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	bool IsDead() const { return bDead; }

protected:

	virtual void BeginPlay() override;

	/** Out-of-combat regen (GDD §3.1): RegenRate HP/s once RegenDelay seconds have passed since last combat. */
	virtual void Tick(float DeltaSeconds) override;

	/** Adds HeroMappingContext to the enhanced input subsystem (null-safe) when possessed by a player. */
	virtual void NotifyControllerChanged() override;

	/** Binds Sprint/Attack on top of the template's Jump/Move/Look bindings. All bindings null-safe. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Sprint input pressed: raise max walk speed to SprintSpeed. */
	void StartSprint();

	/** Sprint input released/canceled: return max walk speed to WalkSpeed. */
	void StopSprint();

	/** Kills the hero exactly once: hide, disable input/collision/movement, broadcast OnHeroDied. */
	void HandleDeath();

	/** Rally cooldown-timer callback (TASK-042): broadcasts OnRallyStateChanged(true, 0) — Rally usable again. */
	void OnRallyReady();

	/** True when the damage is attributable to the hero's own team (DamageCauser first, then EventInstigator's pawn). */
	bool IsFriendlyDamage(AController* EventInstigator, AActor* DamageCauser) const;

protected:

	/** Team this hero fights for. The local player is always Blue (CONVENTIONS team contract). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Mapping context slot for /Game/Input/IMC_Hero — assigned on BP_HeroCharacter in TASK-009. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> HeroMappingContext;

	/** Input action slot for /Game/Input/Actions/IA_Sprint — assigned on BP_HeroCharacter in TASK-009. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	/** Input action slot for /Game/Input/Actions/IA_Attack — assigned on BP_HeroCharacter in TASK-009. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	/** Input action slot for /Game/Input/Actions/IA_Rally (key Q) — assigned + bound on BP_HeroCharacter in TASK-048. Null-safe until then. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RallyAction;

	/** Base movement speed in u/s (GDD §3.1: 500). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 500.f;

	/** Movement speed in u/s while IA_Sprint is held (GDD §3.1: 750). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float SprintSpeed = 750.f;

	/** Damage per melee swing (GDD §3.1: 20). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float MeleeDamage = 20.f;

	/** Melee reach in units (GDD §3.1: 150). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float MeleeRange = 150.f;

	/** Half-angle of the forward melee cone in degrees (GDD §3.1: ±30 = 60° cone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0", ClampMax = "180"))
	float MeleeHalfAngleDegrees = 30.f;

	/** Minimum seconds between melee swings (GDD §3.1: 0.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float MeleeCooldown = 0.5f;

	/** Rally: radius in units within which friendly units are buffed. // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallyRadius = 600.f;

	/** Rally: fractional move-speed bonus applied to each buffed unit (0.25 = +25%). // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallySpeedBonus = 0.25f;

	/** Rally: seconds each friendly unit keeps the move-speed buff. // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallyDuration = 5.f;

	/** Rally: seconds before Rally can be used again. // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallyCooldown = 20.f;

	/**
	 *  Montage played on EVERY swing that passes the cooldown gate — hit or whiff —
	 *  while melee is not suppressed. VISUAL ONLY: damage is applied immediately in
	 *  DoMeleeAttack and never gated on anim notifies (playtest R1 finding 1).
	 *  Wired on BP_HeroCharacter in TASK-017 (/Game/Variant_Combat/Anims/AM_ComboAttack
	 *  or AM_ChargedAttack); null-safe — unset means no montage, damage unchanged.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Optional montage section to start at so exactly one swing plays (chosen in TASK-017); NAME_None plays from the start. */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	FName AttackMontageSection = NAME_None;

	/**
	 *  Impact effect spawned once per enemy actually damaged this swing, at the closest
	 *  point on that enemy's collision to the hero (fallback: its actor location).
	 *  Wired in TASK-017 (/Game/Variant_Combat/VFX/NS_Damage); null-safe.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TObjectPtr<UNiagaraSystem> HitImpactEffect;

	/**
	 *  Camera shake played once per swing on the local player controller when the
	 *  swing damaged >= 1 enemy, via ClientStartCameraShake. Wired in TASK-017
	 *  (/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy); null-safe.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	/** Maximum hit points (GDD §3.1: 200). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero", meta = (ClampMin = "1"))
	float MaxHP = 200.f;

	/** Seconds after last taking OR dealing damage before regen begins (GDD §3.1: 8). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float RegenDelay = 8.f;

	/** Out-of-combat regeneration in HP/s (GDD §3.1: 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float RegenRate = 5.f;

private:

	/** Current hit points. Mutated only by TakeDamage, Tick (regen), and ResetHero. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Hero", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 200.f;

	/** True from HP hitting 0 until ResetHero(). Death side effects run exactly once. */
	bool bDead = false;

	/** True while placement mode (TASK-007) suppresses melee. */
	bool bMeleeSuppressed = false;

	/** World time of the last melee swing (cooldown gate). Seeded far in the past so the first swing is always allowed. */
	double LastMeleeTime = -1.0e9;

	/** World time the hero last took or dealt damage (regen gate). */
	double LastCombatTime = -1.0e9;

	/** World time of the last successful Rally (cooldown gate). Seeded far in the past so the first Rally is always allowed. */
	double LastRallyTime = -1.0e9;

	/** Drives OnRallyReady once RallyCooldown elapses after a successful Rally, to broadcast the ready state. */
	FTimerHandle RallyCooldownTimerHandle;
};
