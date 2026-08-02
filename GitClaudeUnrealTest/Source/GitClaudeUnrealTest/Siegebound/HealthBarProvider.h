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
 *  Per-actor PERMANENT-DAMAGE-BOOST delegate (TASK-362, ancient grounds) — the same
 *  PUSH model as FOnCombatantHPChanged above, one bar row up. BoostPercent is the
 *  boost in PERCENT (0 = none, 100 = +100%, 400 = the cap), NOT a 0-1 fraction and
 *  NOT a multiplier: the UI's whole job is telling EXACTLY 100/200/300% apart from
 *  just past them, so the wire carries the human-readable percent and the banding
 *  math happens once, in C++ (UCombatantHealthBarComponent).
 *
 *  The owner BROADCASTS on EVERY mutation of its stack count — grant, clear, death
 *  reset — miss NONE or the row goes stale forever (the same qa/TASK-005 major-2 trap
 *  the HP delegate documents). Owned and broadcast by ASummonedUnit (TASK-360);
 *  actors with no boost concept simply never provide one (see the pointer accessor
 *  on IHealthBarProvider below).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDamageBoostChanged, float, BoostPercent);

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

	//~ Begin permanent damage boost (TASK-362, ancient grounds) — DEFAULTED, NOT pure virtual.
	//  Deliberately the only two non-pure methods on this interface: the defaults ARE the
	//  contract for "this actor cannot be boosted", so ABuilding and AHeroCharacter need
	//  ZERO changes (do not add overrides there). ASummonedUnit overrides both (TASK-360).

	/**
	 *  Current permanent damage boost in PERCENT (0 = none, 100 = +100%, 400 = the cap) —
	 *  NOT a fraction, NOT a multiplier. Read once at BeginPlay to SEED the boost row before
	 *  any broadcast can arrive; the component bands it into a fill fraction + band color.
	 *  Default 0 ⇒ a non-boostable owner seeds its row to opacity 0 and it stays hidden.
	 */
	virtual float GetDamageBoostPercent() const { return 0.f; }

	/**
	 *  The actor's boost-changed delegate, or nullptr when the actor has no boost concept.
	 *  A POINTER on purpose: "not boostable" must be expressible without every building and
	 *  the hero carrying a dead delegate member just to return a reference. The bar SEEDS
	 *  UNCONDITIONALLY from GetDamageBoostPercent() and only then binds, iff this is non-null.
	 */
	virtual FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() { return nullptr; }

	//~ End permanent damage boost
};
