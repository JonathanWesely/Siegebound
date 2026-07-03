// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GitClaudeUnrealTestCharacter.h"
#include "Siegebound/TeamId.h"
#include "HeroCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class AHeroCharacter;

/**
 *  Broadcast exactly once each time the hero dies (HP reaches 0).
 *  The game mode (TASK-006) binds here to drive respawn timing — the hero
 *  itself knows nothing about respawn schedules (loose coupling, GDD §3.1).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroDied, AHeroCharacter*, DeadHero);

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
 *  - 200 max HP; regenerates RegenRate HP/s starting RegenDelay seconds after
 *    last taking OR dealing damage, stopping at max.
 *  - At 0 HP: hidden, input + collision disabled, OnHeroDied broadcast once.
 *    ResetHero() restores the hero (called by the game mode on respawn/Play Again).
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

	//~ Begin ITeamAgent Interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent Interface

	/** Applies incoming damage (no friendly fire), tracks combat time for regen, and triggers death at 0 HP. */
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

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
};
