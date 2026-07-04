// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeCombatStatics.h"

#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/TeamId.h"

void FSiegeCombatStatics::ApplyRadialDamage(
	UWorld* World,
	AController* InstigatorController,
	ETeamId Team,
	const FVector& Center,
	float Radius,
	float Damage,
	TSubclassOf<UDamageType> DamageTypeClass)
{
	// nothing to do without a world or a positive blast (the SetTimer-style non-positive guard)
	if (!World || Radius <= 0.f || Damage <= 0.f)
	{
		return;
	}

	// a null type reads as 100% everywhere (base UDamageType); callers normally pass Siege/Projectile
	if (!DamageTypeClass)
	{
		DamageTypeClass = UDamageType::StaticClass();
	}

	// every combat actor implements ITeamAgent (hero, units, castles, buildings); AGoldNode
	// deliberately does not, so mining nodes are never damaged by a blast (TASK-025 contract).
	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	for (AActor* Candidate : TeamAgents)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		// enemies only — the friendly-fire authority is this Team filter, not the receiver's
		// instigator chain (native cast is valid: UTeamAgent is NotBlueprintable, TASK-001).
		const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
		if (!Agent || Agent->GetTeamId() == Team)
		{
			continue;
		}

		// closest-point distance so a blast at a fortification's wall still reaches it — the
		// castle/building ORIGIN can be hundreds of units past the wall. Mirrors ASummonedUnit::
		// GetDistanceToTarget; folds into that consolidation on the wave that owns hero/projectile/
		// unit together (qa/TASK-026 NIT-4) — a documented single call site, not a new mirror.
		FVector ClosestPoint = FVector::ZeroVector;
		float Distance = Candidate->ActorGetDistanceToCollision(Center, ECC_Pawn, ClosestPoint);
		if (Distance < 0.f)
		{
			// no ECC_Pawn-blocking collision: fall back to the actor origin
			Distance = static_cast<float>(FVector::Dist(Center, Candidate->GetActorLocation()));
		}
		if (Distance > Radius)
		{
			continue;
		}

		// route through the target's TakeDamage so per-fortification scaling still applies
		// (Siege → 200% vs castle/buildings, TASK-054). DamageCauser null (see the header).
		UGameplayStatics::ApplyDamage(Candidate, Damage, InstigatorController, nullptr, DamageTypeClass);
	}
}
