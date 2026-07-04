// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Siegebound/TeamId.h"

class AController;
class UDamageType;
class UWorld;

/**
 *  Shared Siegebound combat statics (TASK-055). Free-standing helpers used by more
 *  than one gameplay actor — authored ONCE here so callers never hand-mirror the
 *  logic (the qa/TASK-026 NIT-4 "never add a fourth mirror" discipline).
 *
 *  Not a UObject / not reflected: a plain static library, so there is no BeginPlay,
 *  no GC surface, and no Build.cs change (same module — only Core/Engine deps that
 *  are already linked). TASK-056's Bomb Tower AoE projectile links against
 *  ApplyRadialDamage exactly as the Sapper suicide does here.
 */
class GITCLAUDEUNREALTEST_API FSiegeCombatStatics
{
public:

	/**
	 *  Applies Damage (of DamageTypeClass) to EVERY actor implementing ITeamAgent whose
	 *  team differs from Team and whose collision lies within Radius of Center — i.e. all
	 *  ENEMY combat actors caught in the blast, never a friendly (GDD §3.0 no friendly
	 *  fire). The same-team exclusion is enforced HERE (by the Team filter), not left to
	 *  each receiver — so a caller with no resolvable instigator chain (a tower-fired
	 *  projectile) still cannot friendly-fire.
	 *
	 *  Distance is measured to the CLOSEST POINT on each candidate's collision (the
	 *  ASummonedUnit::GetDistanceToTarget contract), so large-footprint fortifications —
	 *  the castle's ~800×800 base, buildings — are hit when the blast center sits at their
	 *  wall, not only when their ORIGIN happens to fall inside Radius. A candidate with no
	 *  ECC_Pawn-blocking collision falls back to its actor origin.
	 *
	 *  Each hit is routed through the target's own TakeDamage, so per-fortification scaling
	 *  still applies (a Siege-typed blast is 200% vs castle/buildings, TASK-054; units/hero
	 *  take the listed amount). DamageCauser is null on purpose — the Team parameter, not the
	 *  instigator chain, is the friendly-fire authority; InstigatorController is passed through
	 *  for attribution when the caller has one (units do via GetController(); towers may not).
	 *  AGoldNode deliberately does not implement ITeamAgent, so mining nodes are never caught.
	 *
	 *  No-op on a null World, Radius <= 0, or Damage <= 0. A null DamageTypeClass defaults to
	 *  base UDamageType (100% everywhere).
	 *
	 *  @param World                world to search (GetAllActorsWithInterface)
	 *  @param InstigatorController attacker's controller for damage attribution, or null
	 *  @param Team                 the ATTACKER's team; actors on this team are never damaged
	 *  @param Center               blast origin in world space
	 *  @param Radius               blast radius in units (closest-point)
	 *  @param Damage               damage dealt to each enemy BEFORE receiver-side scaling
	 *  @param DamageTypeClass      damage type carried to each ApplyDamage (Siege / Projectile / ...)
	 */
	static void ApplyRadialDamage(
		UWorld* World,
		AController* InstigatorController,
		ETeamId Team,
		const FVector& Center,
		float Radius,
		float Damage,
		TSubclassOf<UDamageType> DamageTypeClass);
};
