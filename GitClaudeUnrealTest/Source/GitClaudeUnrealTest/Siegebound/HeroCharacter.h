// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GitClaudeUnrealTestCharacter.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/TeamId.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "HeroCharacter.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UDataTable;
class UCombatantHealthBarComponent;
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
 *  Broadcast whenever the hero's Instant-upgrade loadout changes (TASK-058, GDD §3.10/§7):
 *  on every ApplyUpgrade that adds a stack, on ResetUpgrades, and on respawn re-apply
 *  (ResetHero) so a freshly rebuilt HUD reflects the persisted upgrades. Carries the current
 *  stack count of each of the four Instant upgrades (0 = not owned). The §7 HUD icon row
 *  (TASK-064) seeds from the Get*Stacks getters FIRST, THEN binds here (seed-then-bind law,
 *  CONVENTIONS). Four plain int params keep it MCP-authorable — no enums/structs (the
 *  CONVENTIONS MCP-param rule); the per-upgrade cap for the pips comes from GetUpgradeStackCap.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHeroUpgradesChanged, int32, SharpenedBladeStacks, int32, PlateArmorStacks, int32, SwiftBootsStacks, int32, WarBannerStacks);

/**
 *  Result of AHeroCharacter::ApplyUpgrade (TASK-058, GDD §3.0 refund contract). The Instant
 *  play path (TASK-059) spends the card's Cost ONLY on Applied; any Refused* result means the
 *  play is refused with NO spend (full-refund rule, §3.10) — RefusedAtMaxStacks drives the
 *  "… at max stacks" OnCardRefused message, RefusedInvalidCard covers an unknown CardID or an
 *  unavailable DT_Cards row (the stack cap is never guessed).
 */
UENUM(BlueprintType)
enum class EHeroUpgradeResult : uint8
{
	/** A stack was added and the mods applied — the caller SPENDS the cost and draws a replacement. */
	Applied,
	/** Already at the card's MaxCopies cap — the caller REFUSES the play with no spend and the "at max stacks" message. */
	RefusedAtMaxStacks,
	/** Unknown upgrade CardID, or DT_Cards/its row unavailable (cap unresolved) — the caller REFUSES with no spend. */
	RefusedInvalidCard
};

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
class GITCLAUDEUNREALTEST_API AHeroCharacter : public AGitClaudeUnrealTestCharacter, public ITeamAgent, public IHealthBarProvider
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

	/** Fired whenever the Instant-upgrade loadout changes (TASK-058). The §7 HUD row (TASK-064) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnHeroUpgradesChanged OnHeroUpgradesChanged;

	//~ Begin ITeamAgent Interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent Interface

	/** Fired on every ACTUAL HP change (spawn-init, regen, damage, death, respawn, Plate-Armor upgrade/reset) — drives the overhead bar (UCombatantHealthBarWidget) via the castle-parity PUSH model (TASK-130, mirrors FOnCastleHPChanged). Additive to the M1 WBP_HUD HP. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnCombatantHPChanged OnHPChanged;

	//~ Begin IHealthBarProvider Interface (TASK-130 push model) — forwards to the EXISTING getters + the OnHPChanged delegate; adds NO HP state. GetMaxHP() already returns the EFFECTIVE max (Plate Armor composed).
	virtual FOnCombatantHPChanged& GetHPChangedDelegate() override { return OnHPChanged; }
	virtual float GetHealthCurrent() const override { return GetCurrentHP(); }
	virtual float GetHealthMax() const override { return GetMaxHP(); }
	virtual bool IsHealthBarActorAlive() const override { return !IsDead(); }
	//~ End IHealthBarProvider Interface

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

	/**
	 *  Applies one stack of an Instant hero upgrade (GDD §3.10/§4, TASK-058) — called by the
	 *  Instant play path (TASK-059) with the card's CardID. The stack cap is the card's MaxCopies
	 *  read from DT_Cards (never guessed). On success the per-stack mod is applied on top of the
	 *  IMMUTABLE base stats (every bonus derives live from the stack count — drift-free):
	 *    - SharpenedBlade → +MeleeDamageBonus melee damage per stack (cap 2): cone melee 20→30→40.
	 *    - PlateArmor     → +MaxHPBonus max HP per stack AND an immediate heal of MaxHPBonus (cap 2): 200→300→400.
	 *    - SwiftBoots     → +MoveSpeedBonus fractional move speed to BOTH walk and sprint (cap 1).
	 *    - WarBanner      → enables the friendly-unit damage aura (cap 1): a pulse timer that calls
	 *                       ASummonedUnit::SetAuraDamageBonus(WarBannerDamageBonus, …) on same-team
	 *                       units within WarBannerAuraRadius (TASK-055 / mirrors Rally's TASK-042 loop).
	 *  Returns EHeroUpgradeResult so the caller can REFUND an over-cap or invalid play with NO spend
	 *  (§3.0/§3.10). Works whether the hero is alive or dead (the persistent stack is always added);
	 *  alive-only side effects — the Plate Armor heal, the active aura pulsing — are deferred to
	 *  respawn (ResetHero re-applies them). Broadcasts OnHeroUpgradesChanged on Applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero")
	EHeroUpgradeResult ApplyUpgrade(FName UpgradeCardID);

	/**
	 *  Clears ALL upgrade stacks and mods back to the base hero, ends the War Banner aura, and
	 *  broadcasts OnHeroUpgradesChanged (TASK-058, GDD §3.9). Upgrades PERSIST through hero death
	 *  (ResetHero re-applies the cumulative mods on respawn); they RESET only here, on match end /
	 *  Play Again. INTEGRATION: the match-reset owner (ASiegeGameMode::PlayAgain) must call this on
	 *  the hero — see handoffs/TASK-058.md (kept out of the game mode per this task's files-only scope).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero")
	void ResetUpgrades();

	/** Current hit points, in [0, effective MaxHP]. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	float GetCurrentHP() const { return CurrentHP; }

	/** EFFECTIVE maximum hit points: base MaxHP (§3.1: 200) + Plate Armor bonus (§3.10). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	float GetMaxHP() const { return GetEffectiveMaxHP(); }

	/** EFFECTIVE melee damage per swing: base MeleeDamage (§3.1: 20) + Sharpened Blade bonus (§3.10). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
	float GetEffectiveMeleeDamage() const { return MeleeDamage + (MeleeDamageBonus * SharpenedBladeStacks); }

	/** Current stacks of Sharpened Blade (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetSharpenedBladeStacks() const { return SharpenedBladeStacks; }

	/** Current stacks of Plate Armor (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetPlateArmorStacks() const { return PlateArmorStacks; }

	/** Current stacks of Swift Boots (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetSwiftBootsStacks() const { return SwiftBootsStacks; }

	/** Current stacks of War Banner (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetWarBannerStacks() const { return WarBannerStacks; }

	/** Current stacks of the given upgrade CardID, or 0 for a non-upgrade CardID (TASK-064 generic seed). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetUpgradeStackCount(FName UpgradeCardID) const;

	/** Stack cap (MaxCopies from DT_Cards) for the given upgrade CardID; 0 if unknown/unavailable (TASK-064 pip cap). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetUpgradeStackCap(FName UpgradeCardID) const;

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

	//~ Hero upgrades (TASK-058) — base stats are IMMUTABLE; every bonus derives live from a stack count.

	/** EFFECTIVE max HP: base MaxHP + PlateArmor bonus. The single source used by every HP clamp / regen cap / full-heal. */
	float GetEffectiveMaxHP() const { return MaxHP + (MaxHPBonus * PlateArmorStacks); }

	/** Fractional Swift Boots move-speed bonus (0 = none). Applied to BOTH walk and sprint. */
	float GetMoveSpeedBonusFraction() const { return MoveSpeedBonus * SwiftBootsStacks; }

	/** EFFECTIVE base walk speed (Swift Boots composed). */
	float GetEffectiveWalkSpeed() const { return WalkSpeed * (1.f + GetMoveSpeedBonusFraction()); }

	/** EFFECTIVE sprint speed (Swift Boots composed). */
	float GetEffectiveSprintSpeed() const { return SprintSpeed * (1.f + GetMoveSpeedBonusFraction()); }

	/** Pushes the correct effective speed (sprint vs walk per bSprinting) into the movement component. Null-safe. */
	void ApplyMovementSpeed();

	/**
	 *  Applies the M6.6 climbable-terrain tunables (HeroMaxStepHeight / HeroWalkableFloorAngle /
	 *  HeroJumpZVelocity) onto the CharacterMovementComponent (WalkableFloorAngle via the
	 *  SetWalkableFloorAngle setter so the cached WalkableFloorZ recomputes). Null-safe. Called from
	 *  BOTH the constructor and BeginPlay (mirrors ApplyMovementSpeed) so a BP_HeroCharacter tweak survives.
	 */
	void ApplyTerrainMovementTuning();

	/** Stack cap (MaxCopies) for an upgrade CardID read from DT_Cards; 0 when the table/row is unavailable (caller refuses — never guesses). */
	int32 GetStackCapForUpgrade(FName UpgradeCardID) const;

	/** Begins the War Banner aura pulse (immediate pulse + looping timer). No-op while dead or when WarBanner is not owned; resumes on respawn via ResetHero. */
	void StartWarBannerAura();

	/** Clears the War Banner aura pulse timer (does NOT clear the stack). Called on death, ResetUpgrades, and re-arm. Null-safe/idempotent. */
	void StopWarBannerAura();

	/** War Banner pulse callback: SetAuraDamageBonus(WarBannerDamageBonus, …) on every friendly ASummonedUnit within WarBannerAuraRadius (mirrors Rally's iterate-friendlies loop). */
	void PulseWarBannerAura();

	/** Broadcasts OnHeroUpgradesChanged with the four current stack counts. */
	void BroadcastUpgradesChanged();

protected:

	/** Team this hero fights for. The local player is always Blue (CONVENTIONS team contract). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Overhead poll-driven health bar (M5.5, TASK-110): hide-at-full, team-tinted. ADDITIVE to the hero's own WBP_HUD HP readout (M1) — that stays. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero")
	TObjectPtr<UCombatantHealthBarComponent> HPBarWidget;

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

	//~ Climbable-terrain movement tuning (M6.6, TASK-141) — pushed onto the CharacterMovementComponent
	//~ by ApplyTerrainMovementTuning() from BOTH the ctor and BeginPlay (mirrors ApplyMovementSpeed).
	//~ The `Hero` prefix is MANDATORY and load-bearing: an un-prefixed name would SHADOW the identically
	//~ named UCharacterMovementComponent field it drives (MaxStepHeight / WalkableFloorAngle /
	//~ JumpZVelocity), which UHT compiles as the C4457/58/59 shadow HARD ERROR (CONVENTIONS shadow law).
	//~ Retune is comfort/margin only — the ≤30° hill faces are already climbable without it.

	/** Max vertical step the hero walks up without jumping, in units. Drives CharacterMovement MaxStepHeight (M6.6: 50, was 45). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float HeroMaxStepHeight = 50.f;

	/** Steepest floor the hero can stand/walk on, in degrees. Drives CharacterMovement WalkableFloorAngle via SetWalkableFloorAngle (M6.6: 50, was 44.76). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0", ClampMax = "90"))
	float HeroWalkableFloorAngle = 50.f;

	/** Upward launch speed of a jump, in u/s. Drives CharacterMovement JumpZVelocity (M6.6: 600 ⇒ ~184 cm apex, was 500). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float HeroJumpZVelocity = 600.f;

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

	//~ Hero-upgrade magnitudes (TASK-058) — UPROPERTY defaults per GDD §3.10/§4, NEVER from CSV
	//~ (CONVENTIONS: mechanic magnitudes are class UPROPERTYs; only the stack CAP is a DT_Cards column).

	/** Sharpened Blade: melee damage added PER stack (GDD §3.10: 10). Cap 2 ⇒ cone melee 20→30→40. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float MeleeDamageBonus = 10.f; // GDD §3.10

	/** Plate Armor: max HP added PER stack AND healed immediately on apply (GDD §3.10: 100). Cap 2 ⇒ 200→300→400. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float MaxHPBonus = 100.f; // GDD §3.10

	/** Swift Boots: fractional move-speed bonus PER stack applied to BOTH walk and sprint (GDD §3.10: 0.25 = +25%). Cap 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float MoveSpeedBonus = 0.25f; // GDD §3.10

	/** War Banner: aura radius in units within which friendly units are buffed (GDD §4: 600). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float WarBannerAuraRadius = 600.f; // GDD §4

	/** War Banner: fractional damage bonus applied to each friendly unit in the aura (GDD §4: 0.20 = +20%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float WarBannerDamageBonus = 0.20f; // GDD §4

	/** War Banner: seconds between aura pulses. The bonus is (re)applied to in-range units each pulse; the SetAuraDamageBonus window is 2× this so a unit that stays in range never flickers and a unit that leaves loses the bonus shortly after. Impl detail — not a GDD stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0.05"))
	float WarBannerPulseInterval = 0.5f;

	/** DT_Cards asset the stack caps (MaxCopies) are read from (GDD §3.0). Resolved null-safe at BeginPlay; a missing table refuses upgrade plays. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Upgrades")
	TSoftObjectPtr<UDataTable> CardTableAsset;

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

	//~ Hero-upgrade state (TASK-058). Stack counts are the ONLY mutated state — every effective
	//~ stat derives live from them, so they persist through respawn (ResetHero does not clear them)
	//~ and reset only in ResetUpgrades. VisibleInstanceOnly for PIE inspection; not saved.

	/** Sharpened Blade stacks (0..MaxCopies). Drives GetEffectiveMeleeDamage. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 SharpenedBladeStacks = 0;

	/** Plate Armor stacks (0..MaxCopies). Drives GetEffectiveMaxHP. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 PlateArmorStacks = 0;

	/** Swift Boots stacks (0..MaxCopies). Drives GetMoveSpeedBonusFraction. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 SwiftBootsStacks = 0;

	/** War Banner stacks (0..MaxCopies). >0 ⇒ the aura pulse timer runs while alive. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 WarBannerStacks = 0;

	/** True while IA_Sprint is held (drives ApplyMovementSpeed's walk-vs-sprint choice so upgrades recompute the right speed). */
	bool bSprinting = false;

	/** Hard-resolved DT_Cards (cached at BeginPlay) that ApplyUpgrade reads MaxCopies from. */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedCardTable;

	/** Drives PulseWarBannerAura every WarBannerPulseInterval while War Banner is owned and the hero is alive. */
	FTimerHandle WarBannerAuraTimerHandle;
};
