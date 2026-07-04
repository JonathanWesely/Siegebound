// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Castle.generated.h"

class ACastle;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 *  Broadcast exactly once when a castle's HP reaches 0 (GDD §3.9).
 *  DestroyedCastle is the castle that fell; CastleTeam is its team —
 *  the OTHER team is the match winner. ASiegeGameMode (TASK-006)
 *  subscribes to this on every ACastle at BeginPlay.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleDestroyed, ACastle*, DestroyedCastle, ETeamId, CastleTeam);

/**
 *  Broadcast on every ACTUAL CurrentHP change (playtest R1 finding 2, TASK-018):
 *  after damage is applied in TakeDamage — NEVER for ignored friendly fire,
 *  which changes nothing — plus in ResetCastle and once at BeginPlay (seed).
 *  UI consumers must still seed from GetCurrentHP()/GetMaxHP() FIRST and bind
 *  second (qa/TASK-005-report.md major 2); UCastleHealthBarWidget::InitForCastle
 *  does exactly that.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleHPChanged, float, CurrentHP, float, MaxHP);

/**
 *  Siegebound castle (GDD §3.9). One per team, placed at the CastleAnchor
 *  TargetPoints in L_Arena at integration (Castle_Blue / Castle_Red).
 *
 *  - Does not attack; it is a 2000 HP objective.
 *  - No friendly fire: damage whose instigator is on the same team is ignored (§3.0).
 *  - At 0 HP it broadcasts OnCastleDestroyed exactly once, hides, and stops colliding.
 *  - ResetCastle() (Play Again, §3.9) restores full HP, visibility, and collision.
 *  - Mesh and per-team material are soft references resolved null-safe in
 *    OnConstruction — the art assets are produced in parallel and may not exist yet.
 *  - Overhead HP bar (playtest R1 finding 2, TASK-018): screen-space HPBarWidget
 *    component; its widget class is soft-resolved null-safe at BeginPlay
 *    (WBP_CastleHealthBar, built in TASK-019 — a missing asset is a silent
 *    no-op). Hidden on destruction, shown again by ResetCastle().
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ACastle : public AActor, public ITeamAgent
{
	GENERATED_BODY()

public:

	ACastle();

	/** Fired exactly once when this castle is destroyed. Win-condition hook for ASiegeGameMode (TASK-006). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleDestroyed OnCastleDestroyed;

	/** Fired on every actual HP change, in ResetCastle, and once at BeginPlay (seed). Drives WBP_CastleHealthBar (TASK-018/019). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleHPChanged OnCastleHPChanged;

	//~ Begin ITeamAgent interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent interface

	/**
	 *  Applies incoming damage. Same-team instigators are ignored entirely (no friendly
	 *  fire, §3.0). Damage-vs-castle scaling (§3.0, TASK-026) is read from
	 *  DamageEvent.DamageTypeClass and applied ONLY here (M2 ruling — units/hero/
	 *  buildings take listed damage everywhere else): USiegeDamageType_Projectile
	 *  (and subclasses) = 50%, melee/default/untyped = 100%. Returns the SCALED
	 *  amount the castle actually took.
	 *  TODO(M4): USiegeDamageType_Siege = 200% vs castle (GDD §3.0/§3.9).
	 *  TODO(M5): spell damage types = 50% vs castle (GDD §3.0).
	 */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Play Again (§3.9): restores MaxHP, visibility, and collision, and re-arms the destroyed event. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Castle")
	void ResetCastle();

	/** Current hit points, in [0, MaxHP]. HUD/QA hook (TASK-011). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	float GetCurrentHP() const { return CurrentHP; }

	/** Maximum hit points (2000 per GDD §3.9). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	float GetMaxHP() const { return MaxHP; }

	/** True once the castle has been destroyed and until ResetCastle(). Unit targeting (TASK-004) should skip destroyed castles. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	bool IsCastleDestroyed() const { return bDestroyed; }

	/** Resolves the soft-referenced mesh and per-team material, null-safe. */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Seeds CurrentHP from MaxHP, fires the OnCastleHPChanged seed broadcast, and initializes the HP bar widget (null-safe). */
	virtual void BeginPlay() override;

	/** Static mesh root. Mesh asset assigned null-safe in OnConstruction from CastleMeshAsset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UStaticMeshComponent> CastleMesh;

	/** Screen-space overhead HP bar (DrawSize 256x32 at Z+1050 — the castle mesh is 900 tall). Widget class resolved null-safe at BeginPlay from HPBarWidgetClass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UWidgetComponent> HPBarWidget;

	/** Which team owns this castle. Set per level instance (Castle_Blue = Blue, Castle_Red = Red). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	ETeamId Team = ETeamId::Blue;

	/** Maximum hit points (GDD §3.9). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle", meta = (ClampMin = "1"))
	float MaxHP = 2000.0f;

	/** Castle blockout mesh (TASK-013). May not exist yet — resolved null-safe in OnConstruction. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UStaticMesh> CastleMeshAsset;

	/** Team material applied when Team == Blue (TASK-012). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialBlue;

	/** Team material applied when Team == Red (TASK-012). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialRed;

	/** HP bar widget class, /Game/UI/WBP_CastleHealthBar (TASK-019). Missing asset = silent no-op, never a crash. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftClassPtr<UUserWidget> HPBarWidgetClass;

private:

	/** Loads (if available) and applies the castle mesh and the Team-appropriate material. Never crashes on missing assets. */
	void ApplyTeamVisuals();

	/** Resolves HPBarWidgetClass null-safe (missing = silent no-op), assigns it to HPBarWidget, and calls InitForCastle on the created UCastleHealthBarWidget. */
	void InitHPBarWidget();

	/** Single-fire destruction: guards on bDestroyed, hides the actor, disables collision, broadcasts OnCastleDestroyed. */
	void HandleDestroyed();

	/**
	 *  Resolves the attacking team from a damage event's instigator chain:
	 *  the instigating controller's pawn, then the damage causer itself, then
	 *  the causer's instigator pawn (projectiles, M2). Returns false if no
	 *  ITeamAgent is found (e.g. world damage), in which case damage applies.
	 */
	static bool TryGetInstigatorTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam);

	/** Current hit points. Mutated only by TakeDamage and ResetCastle. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 2000.0f;

	/** True after OnCastleDestroyed has fired; re-armed only by ResetCastle(). Guarantees the event fires exactly once. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	bool bDestroyed = false;
};
