// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DamageTypes.h"

// Intentionally empty: the Siegebound damage types are pure tags (GDD §3.0 /
// CONVENTIONS damage-type registry). All scaling logic lives in the receivers'
// TakeDamage — ACastle::TakeDamage (TASK-026, the Projectile 50% branch) and
// both ACastle and ABuilding::TakeDamage (TASK-054, the Siege 200% branch).
