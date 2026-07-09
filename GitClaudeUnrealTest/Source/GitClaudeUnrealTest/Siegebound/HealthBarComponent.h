// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "Engine/TimerHandle.h"
#include "UObject/SoftObjectPtr.h"
#include "HealthBarComponent.generated.h"

class UUserWidget;
class UUnitHealthBarWidget;

/**
 *  Poll-driven overhead health bar (M5.5, TASK-110). A screen-space
 *  UWidgetComponent subclass, added ONCE in-constructor to ASummonedUnit,
 *  ABuilding, and AHeroCharacter (each names its instance HPBarWidget;
 *  subclasses — AMinerUnit, ATower, ABarracks, ADeepMine — inherit it for free).
 *
 *  Unlike the castle bar (which BINDS FOnCastleHPChanged), units/buildings/hero
 *  carry NO HP-changed delegate, so this component POLLS the owner through the
 *  shared IHealthBarTarget interface on a ~PollInterval timer and drives
 *  WBP_UnitHealthBar (UUnitHealthBarWidget) via float-only BIEs.
 *
 *  Behavior law (CONVENTIONS): hide-at-full — the bar is visible ONLY while the
 *  owner is alive, opted-in (bShowHealthBar), and actually damaged
 *  (Current < Max − epsilon); hidden at full HP and on death/destruction. The
 *  fill is team-tinted ONCE at init (blue friendly / red enemy) from
 *  BlueBarColor/RedBarColor via the owner's ITeamAgent::GetTeamId.
 *
 *  Everything is null-safe: a missing widget class means a silent no-bar (logged
 *  once), never a crash; a non-target owner or mis-authored widget simply shows
 *  nothing. Zero behavior change to combat/stats — this only reads HP getters.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API UHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UHealthBarComponent();

protected:

	/** Soft-resolves the widget class, seeds the team tint once, and arms the poll timer (null-safe). */
	virtual void BeginPlay() override;

	/** Clears the poll timer so no poll fires after the owner leaves play. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  Widget class shown by this bar (default /Game/UI/WBP_UnitHealthBar, built
	 *  in TASK-111). Soft — resolved null-safe at BeginPlay; a missing asset means
	 *  no bar (logged once), never a crash. A BP may retarget or clear it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	TSoftClassPtr<UUserWidget> HealthBarWidgetClass;

	/** Seconds between HP polls (spec: 0.15). Never per-tick; clamped > 0 at runtime so a zero cell can't clear the timer. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar", meta = (ClampMin = "0.02"))
	float PollInterval = 0.15f;

	/** Per-actor/per-BP opt-out (default true). When false the bar is always hidden — a zero-code way to suppress clutter (e.g. miners). */
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

	/**
	 *  Timer callback (every PollInterval): reads the owner's IHealthBarTarget.
	 *  Hides the bar unless the owner is alive, opted-in, and damaged
	 *  (Current < Max − epsilon); otherwise shows it and pushes OnHPChanged.
	 */
	void PollHealth();

	/** Cached UUnitHealthBarWidget instance created by the resolved class (null when the asset is missing/mis-authored). */
	UPROPERTY(Transient)
	TObjectPtr<UUnitHealthBarWidget> BarWidget;

	/** Our own shown/hidden latch (starts hidden). Tracked here so the poll never depends on engine IsVisible() semantics for screen-space widgets. */
	bool bBarShown = false;

	/** Repeating HP-poll timer handle; cleared in EndPlay. */
	FTimerHandle PollTimerHandle;
};
