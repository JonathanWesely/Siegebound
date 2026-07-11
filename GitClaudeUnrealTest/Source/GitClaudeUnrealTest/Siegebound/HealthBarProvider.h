// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthBarProvider.generated.h"

/**
 *  Per-actor HP-changed delegate — the PUSH/delegate model that MIRRORS
 *  FOnCastleHPChanged (Castle.h). Rebuilt 2026-07-10 (TASK-130) to replace the
 *  retired poll system (5 failed attempts): each combat actor OWNS one member of
 *  this type (named OnHPChanged) and BROADCASTS it on EVERY HP mutation — damage,
 *  heal/regen, spawn-init, reset/respawn — miss NONE, or the bar goes stale
 *  (qa/TASK-005 major-2 seed-then-bind trap). UCombatantHealthBarWidget binds to
 *  it (seed-then-bind), exactly like UCastleHealthBarWidget binds FOnCastleHPChanged.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatantHPChanged, float, CurrentHP, float, MaxHP);

/**
 *  Read/subscribe surface the overhead combatant health bar (TASK-130) uses to
 *  drive WBP_CombatantHealthBar on the PUSH model. Implemented by every combat
 *  actor that shows a floating bar — ASummonedUnit (incl. AMinerUnit), ABuilding
 *  (towers, walls, Barracks, Deep Mine), and AHeroCharacter. The methods forward
 *  to each actor's EXISTING HP getters and its OnHPChanged delegate — this
 *  interface adds NO new HP state (replaces the retired IHealthBarTarget).
 *
 *  Team is read through the SEPARATE ITeamAgent::GetTeamId and is deliberately NOT
 *  duplicated onto this interface. ACastle does NOT implement it (it keeps its own
 *  FOnCastleHPChanged bar) and AGoldNode does NOT implement it (not damageable).
 *
 *  Pure-virtual C++ interface — the ITeamAgent::GetTeamId shape (GetHPChangedDelegate
 *  returns a delegate reference, which is not Blueprint-representable, so none of
 *  these are UFUNCTIONs). Header-only, mirroring the TeamId.h one-concept-header
 *  precedent, and it also declares the shared FOnCombatantHPChanged delegate above.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UHealthBarProvider : public UInterface
{
	GENERATED_BODY()
};

class IHealthBarProvider
{
	GENERATED_BODY()

public:

	/** The actor's HP-changed delegate the bar binds to (seed-then-bind). Returns the owner's OnHPChanged member. */
	virtual FOnCombatantHPChanged& GetHPChangedDelegate() = 0;

	/** Current hit points, in [0, GetHealthMax()]. Forwards to the actor's existing GetCurrentHP(). */
	virtual float GetHealthCurrent() const = 0;

	/** Maximum hit points (> 0 once the actor's stats are bound). Forwards to the actor's existing GetMaxHP() (hero: EFFECTIVE max). */
	virtual float GetHealthMax() const = 0;

	/** True while the actor is alive/standing. Forwards to !IsUnitDead / !IsBuildingDestroyed / !IsDead. */
	virtual bool IsHealthBarActorAlive() const = 0;
};
