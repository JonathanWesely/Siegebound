// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthBarTarget.generated.h"

/**
 *  Read-only surface the overhead health bar (M5.5, TASK-110) POLLS to drive
 *  WBP_UnitHealthBar. Implemented by every combat actor that shows a floating
 *  overhead bar — ASummonedUnit (incl. AMinerUnit), ABuilding (towers, walls,
 *  Barracks, Deep Mine), and AHeroCharacter. The methods bind to the actor's
 *  EXISTING HP getters — this interface adds NO new HP state (CONVENTIONS
 *  "Overhead unit health bars (M5.5)" reuse fact).
 *
 *  Team is read through the SEPARATE ITeamAgent::GetTeamId and is deliberately
 *  NOT duplicated onto this interface. ACastle does NOT implement it (it keeps
 *  its own FOnCastleHPChanged delegate bar, M1) and AGoldNode does NOT implement
 *  it (not damageable — no HP).
 *
 *  Pure-virtual const C++ interface — the ITeamAgent::GetTeamId shape, NOT a
 *  BlueprintNativeEvent (the component polls these from C++; there is no BP
 *  override path). Header-only, mirroring the TeamId.h one-concept-header
 *  precedent.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UHealthBarTarget : public UInterface
{
	GENERATED_BODY()
};

class IHealthBarTarget
{
	GENERATED_BODY()

public:

	/** Current hit points, in [0, GetHealthMax()]. Bound to the actor's existing GetCurrentHP(). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Health")
	virtual float GetHealthCurrent() const = 0;

	/** Maximum hit points (> 0 once the actor's stats are bound). Bound to the actor's existing GetMaxHP(). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Health")
	virtual float GetHealthMax() const = 0;

	/** True while the actor is alive/standing (drives hide-on-death/destruction). Bound to !IsUnitDead / !IsBuildingDestroyed / !IsDead. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Health")
	virtual bool IsHealthBarActorAlive() const = 0;
};
