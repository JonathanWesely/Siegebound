// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes.generated.h"

/**
 *  Siegebound damage-type registry (CONVENTIONS.md "Damage types" / GDD §3.0).
 *
 *  Pure tags — no logic in these classes. The ONLY consumer of the distinction
 *  is ACastle::TakeDamage (TASK-026), which reads DamageEvent.DamageTypeClass
 *  and applies the §3.0 damage-vs-castle scaling: Projectile = 50% (the
 *  anti-sniping rule), melee/default/untyped = 100%. Everything else — units,
 *  hero, buildings — takes listed damage regardless of type (M2 ruling).
 *
 *  Attackers TAG projectile damage: AProjectile::InitProjectile carries the
 *  damage-type class through to ApplyDamage. Melee needs NO tag — untyped or
 *  base-UDamageType damage already reads as 100% (CONVENTIONS), so the existing
 *  M1 hero/unit melee (UDamageType::StaticClass()) is untouched.
 *  USiegeDamageType_Melee exists for registry completeness and explicit
 *  attribution by future attackers, not because any scaling rule requires it.
 *
 *  Reserved for later milestones (declare THEM HERE when they arrive):
 *  USiegeDamageType_Siege (M4, 200% vs castle), USiegeDamageType_Spell (M5,
 *  50% vs castle).
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
