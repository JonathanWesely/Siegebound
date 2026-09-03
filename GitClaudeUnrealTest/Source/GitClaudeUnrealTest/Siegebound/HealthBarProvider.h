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

	//~ Begin CAST PROGRESS (TASK-830 item (8); law WITCH-§9.1 / §9.3 / §9.6) — DEFAULTED, NOT pure
	//  virtual, for the SAME reason the two boost methods above are: the defaults ARE the contract
	//  for "nothing is being channelled on me". ⛔ ABuilding, AHeroCharacter and every non-witch
	//  unit therefore need ZERO changes and render PIXEL-IDENTICALLY to today — WITCH-§9.6 states
	//  that is a REQUIREMENT, not an expectation. ⛔ Do not add overrides there. This is the boost
	//  bar's own plumbing pattern REUSED, deliberately not reinvented.
	//
	//  ⭐⭐ WHY THIS SURFACE EXISTS AT ALL, IN ONE SENTENCE: SK_Witch does not exist, so the witch
	//  spawns STATIC, and on a 3-second INTERRUPTIBLE channel "starting", "running" and "broken"
	//  are currently IDENTICAL PIXELS (WITCH-§9). An interruption the player cannot perceive is
	//  not counterplay, and the interruption IS the card's counterplay.
	//
	//  ⭐⭐ TWO ACTORS ANSWER TRUE FOR ONE CAST, AND THAT IS THE DESIGN RATHER THAN A DEFECT.
	//  WITCH-§9.2 rules the tell is a TWO-ENDED bar — the SAME bar, the SAME fill, on the WITCH
	//  AND on her TARGET — because Jonathan's sentence names both actors ("...interupted if the
	//  witch OR unit that is turning invisible are attacked") and the bar must therefore appear on
	//  EXACTLY the two actors you can attack to break the cast. The cheap default (one bar on the
	//  caster, which is what every game ships) fails that requirement.
	//
	//  ⛔⛔ AND THE RULE THAT KEEPS IT HONEST: EACH END ANSWERS ABOUT ITSELF. ⛔ A caster must
	//  NEVER push a percent onto another actor's widget (WITCH-§9.3 forbids it by name) — that is
	//  a second source of truth, and it STRANDS A BAR on the target the moment the caster dies
	//  mid-cast, which the interrupt rule makes the COMMON case rather than an edge one.

	/**
	 *  Progress of the cast being channelled BY or ON this actor, in PERCENT.
	 *
	 *  ⛔⛔ THE RANGE IS 0..100, ⛔ NEVER 0..1 — the shipped GetDamageBoostPercent convention
	 *  directly above, reused so one widget never carries two scales (WITCH-§9.3 pins it). The
	 *  consuming widget divides by 100 on its side, exactly as the HP row already does.
	 *  ⚠️ THE CONSEQUENCE OF GETTING IT BACKWARDS, in arithmetic (HIGH-§1): a 0..1 producer read
	 *  by a /100 consumer paints 0.03 of a bar for a cast that is 3% done and 0.01 of one for a
	 *  cast that is finished — i.e. a tell that never visibly moves, on a 3-second window whose
	 *  whole job is answering "how much LONGER".
	 *
	 *  ⛔ DERIVED PER CALL, ⛔ never a stored field, in every implementation: an interrupted cast
	 *  must leave NO partial state (WITCH-§4), and a remembered percent is partial state that
	 *  survives the cancel and freezes the bar mid-flight.
	 *
	 *  Default 0 ⇒ a non-casting owner's cast row seeds empty and stays collapsed.
	 */
	virtual float GetCastProgressPercent() const { return 0.f; }

	/**
	 *  True while a cast is running BY this actor (it is the caster) or ON it (it is the subject).
	 *
	 *  ⛔ A SEPARATE QUESTION FROM THE PERCENT, and it must NOT be derived from one: a cast that
	 *  has just begun reads 0%, so "percent > 0" would hide the bar for the first poll of every
	 *  cast — the exact moment WITCH-§9.1 requirement 1 ("a cast is RUNNING, on THIS one") exists
	 *  to serve. This is the flag the cast row's collapse is driven from.
	 *
	 *  ⭐ AND IT IS WHAT MAKES *BROKEN* DISTINGUISHABLE FROM *COMPLETED* (WITCH-§9.1 row 4, the
	 *  requirement with no tell at all today): an INTERRUPT drives this false while the percent is
	 *  still mid-flight, whereas a COMPLETION drives it false only after the percent has run the
	 *  full window. The pair is the signal; neither half carries it alone.
	 *
	 *  Default false ⇒ an owner with no cast concept never shows the row.
	 */
	virtual bool IsCastInProgress() const { return false; }

	//~ End cast progress
};
