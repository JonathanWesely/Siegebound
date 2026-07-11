// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "UObject/SoftObjectPtr.h"
#include "CombatantHealthBarComponent.generated.h"

class UUserWidget;
class UCombatantHealthBarWidget;

/**
 *  Push-model overhead health bar (TASK-130, castle-parity REBUILD of the retired
 *  poll system). A screen-space UWidgetComponent subclass, added ONCE in-constructor
 *  to ASummonedUnit, ABuilding, and AHeroCharacter (each names its instance
 *  HPBarWidget; subclasses — AMinerUnit, ATower, ABarracks, ADeepMine — inherit it).
 *
 *  MIRRORS ACastle::HPBarWidget: at BeginPlay it soft-resolves the widget class,
 *  reads the owner as IHealthBarProvider + ITeamAgent, and calls
 *  UCombatantHealthBarWidget::InitForCombatant (SEED-THEN-BIND) + SetTeamColor once.
 *  There is NO poll timer (the failed poll system is retired); the bar updates only
 *  when the owner BROADCASTS its OnHPChanged delegate.
 *
 *  Visibility (reversed hide-at-full law): the bar is ALWAYS VISIBLE while the owner
 *  is alive and opted-in (bShowHealthBar). Hide/show on death/respawn is the OWNER's
 *  job (HideBar / ShowBarIfEnabled) — the ACastle::HandleDestroyed / ResetCastle
 *  parity, since screen-space widget components do not follow actor hidden-in-game
 *  state and there is no poll to observe death here.
 *
 *  Everything null-safe: a missing widget class is a silent no-bar (logged once),
 *  never a crash. Zero combat/stat behavior change — it only reads HP getters + binds.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API UCombatantHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UCombatantHealthBarComponent();

	/** Shows the bar iff opted-in (bShowHealthBar). Called by the owner on respawn/reset (ACastle::ResetCastle parity). */
	void ShowBarIfEnabled();

	/** Hides the bar. Called by the owner on death/destruction (ACastle::HandleDestroyed parity) — screen-space widgets do not follow SetActorHiddenInGame. */
	void HideBar();

protected:

	/** Soft-resolves the widget class, seeds+binds via the owner's OnHPChanged delegate, and pushes the team tint once (null-safe). */
	virtual void BeginPlay() override;

	/**
	 *  Component-side delegate handler (TASK-130 render-fix). Bound to the owner's OnHPChanged in
	 *  BeginPlay, ADDITIVE to the widget's own seed-then-bind. On each HP change it drives the
	 *  CURRENT on-screen `GetWidget()` directly (identity-proof: if the widget's own binding is
	 *  updating a stale/offscreen instance from an earlier frame, this always targets the live one)
	 *  and calls `RequestRedraw()` (World-space stale-render safety; a no-op for Screen-space live Slate).
	 */
	UFUNCTION()
	void HandleOwnerHPChanged(float CurrentHP, float MaxHP);

	/**
	 *  Widget class shown by this bar (default /Game/UI/WBP_CombatantHealthBar, built
	 *  in TASK-131). Soft — resolved null-safe at BeginPlay; a missing asset means no
	 *  bar (logged once), never a crash. A BP may retarget or clear it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	TSoftClassPtr<UUserWidget> HealthBarWidgetClass;

	/** Per-actor/per-BP opt-out (default true). When false the bar stays hidden — a zero-code way to suppress clutter (e.g. miners). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	bool bShowHealthBar = true;

	/** Relative Z (units) of the bar above the actor root. Default 120; buildings/hero BPs may raise it. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	float BarHeightZ = 120.f;

	/** Fill tint for a friendly (Blue) owner — §6 palette / MI_TeamColor linear values. Pushed to SetTeamColor once at init. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	FLinearColor BlueBarColor = FLinearColor(0.05f, 0.30f, 1.00f);

	/** Fill tint for an enemy (Red) owner — §6 palette / MI_TeamColor linear values. Pushed to SetTeamColor once at init. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	FLinearColor RedBarColor = FLinearColor(1.00f, 0.10f, 0.05f);

private:

	/** The widget instance cached from GetWidget() after SetWidgetClass; used for the seed-then-bind + team-tint setup in BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<UCombatantHealthBarWidget> BarWidget;
};
