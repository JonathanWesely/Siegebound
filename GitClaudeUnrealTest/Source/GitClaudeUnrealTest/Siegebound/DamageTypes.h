// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes.generated.h"

/**
 *  Siegebound damage-type registry (CONVENTIONS.md "Damage types" / GDD §3.0).
 *
 *  Pure tags — no logic in these classes. The consumers of the distinction are
 *  ACastle::TakeDamage (TASK-026) and ABuilding::TakeDamage (TASK-054), which
 *  read DamageEvent.DamageTypeClass and apply the §3.0 damage-vs-fortification
 *  scaling: USiegeDamageType_Siege = 200% vs BOTH the castle and buildings (M4
 *  ruling — Siege units batter fortifications), USiegeDamageType_Projectile =
 *  50% vs the castle only (the anti-sniping rule), melee/default/untyped = 100%.
 *  Units and the hero always take listed damage regardless of type (M2/M4 ruling).
 *
 *  Attackers TAG projectile damage: AProjectile::InitProjectile carries the
 *  damage-type class through to ApplyDamage. Melee needs NO tag — untyped or
 *  base-UDamageType damage already reads as 100% (CONVENTIONS), so the existing
 *  M1 hero/unit melee (UDamageType::StaticClass()) is untouched.
 *  USiegeDamageType_Melee exists for registry completeness and explicit
 *  attribution by future attackers, not because any scaling rule requires it.
 *
 *  Reserved for later milestones (declare THEM HERE when they arrive):
 *  USiegeDamageType_Spell (M5, 50% vs castle).
 */

/** Melee-contact damage (GDD §3.0). 100% vs castle — identical to untyped/default damage; a tag, never a requirement. */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeDamageType_Melee : public UDamageType
{
	GENERATED_BODY()
};

/** Homing-projectile damage (GDD §3.0: Archer, towers). ACastle::TakeDamage scales this — and any subclass — to 50%. */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeDamageType_Projectile : public UDamageType
{
	GENERATED_BODY()
};

/**
 *  Siege / battering damage (GDD §3.0: Profile=Siege units — Ogre, Sapper — tag
 *  their attacks with this). Both ACastle::TakeDamage AND ABuilding::TakeDamage
 *  scale this — and any subclass — to 200% (M4 ruling, TASK-054): Siege units
 *  smash fortifications for double. Units and the hero take the listed amount.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeDamageType_Siege : public UDamageType
{
	GENERATED_BODY()
};
