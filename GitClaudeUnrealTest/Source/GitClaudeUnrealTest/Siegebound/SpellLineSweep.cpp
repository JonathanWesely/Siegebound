// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SpellLineSweep.h"

#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/Building.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeGameMode.h"
#include "Siegebound/SummonedUnit.h"

ASpellLineSweep::ASpellLineSweep()
{
	// per-tick by design: the sweep front advances every frame (see class comment)
	PrimaryActorTick.bCanEverTick = true;

	// plain scene root so the actor HAS a transform — it tracks the advancing
	// front as the TASK-238 travel seam. No collision anywhere on this actor:
	// reach is the distance test in ApplyLineEffectUpTo, never physics (the
	// AProjectile no-collision guarantee — it can never block a trace or a pawn).
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// failsafe cap (the AProjectile InitialLifeSpan pattern): a sweep that
	// somehow never completes — a stalled TravelDuration edit mid-PIE — always
	// despawns well before it could straddle a Play Again.
	InitialLifeSpan = 2.f;
}

void ASpellLineSweep::InitLineSweep(FName InCardID, const FCardRow& InRow, ETeamId InCasterTeam, const FVector& InOrigin, const FVector& InAimDirection)
{
	if (bInitialized)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASpellLineSweep '%s': InitLineSweep called twice ('%s' then '%s') — re-initialization ignored; spawn a NEW sweep per cast."),
			*GetNameSafe(this), *CardID.ToString(), *InCardID.ToString());
		return;
	}

	CardID = InCardID;
	Row = InRow;
	CasterTeam = InCasterTeam;
	LineOrigin = InOrigin;
	AimDirection = InAimDirection;
	bInitialized = true;

	// start the transform at the muzzle, facing the aim (the travel seam's rest pose)
	SetActorLocationAndRotation(LineOrigin, AimDirection.Rotation());
}

void ASpellLineSweep::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInitialized)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASpellLineSweep '%s': ticked without InitLineSweep — expiring harmlessly (spawner bug)."),
			*GetNameSafe(this));
		Destroy();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	// match-end law (§3.9): FreezeWorldAtMatchEnd destroys in-flight
	// AProjectiles so nothing lands under the Victory screen, but it predates
	// this class — so the sweep SELF-GATES on the same latch and dies without
	// applying anything further. Null-safe: no ASiegeGameMode (a bare test
	// world) simply skips the gate.
	if (const ASiegeGameMode* GameMode = World->GetAuthGameMode<ASiegeGameMode>())
	{
		if (GameMode->HasMatchEnded())
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASpellLineSweep '%s': match ended mid-sweep ('%s') — destroyed with no further application (match-end freeze law)."),
				*GetNameSafe(this), *CardID.ToString());
			Destroy();
			return;
		}
	}

	ElapsedSeconds += DeltaSeconds;

	// front = how far the line has extended; TravelDuration <= 0 degrades to an
	// instant full-length sweep (documented on the UPROPERTY)
	const float FrontDistance = (TravelDuration > 0.f)
		? LineRange * FMath::Clamp(ElapsedSeconds / TravelDuration, 0.f, 1.f)
		: LineRange;

	ApplyLineEffectUpTo(FrontDistance);

	// travel seam (TASK-238): the actor rides the front so an attached travel
	// VFX moves with the bolt
	SetActorLocation(LineOrigin + AimDirection * FrontDistance);

	if (FrontDistance >= LineRange)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASpellLineSweep: '%s' line sweep complete for %s — %d target(s) affected along %.0f uu from (%s) toward (%.2f, %.2f)."),
			*CardID.ToString(), (CasterTeam == ETeamId::Red) ? TEXT("Red") : TEXT("Blue"),
			AppliedCount, LineRange, *LineOrigin.ToCompactString(), AimDirection.X, AimDirection.Y);
		Destroy();
	}
}

void ASpellLineSweep::ApplyLineEffectUpTo(float FrontDistance)
{
	UWorld* World = GetWorld();
	if (!World || FrontDistance <= 0.f)
	{
		return;
	}

	const FVector SegmentEnd = LineOrigin + AimDirection * FrontDistance;

	// same candidate universe as the M5 resolvers / ApplyRadialDamage: every
	// ITeamAgent in the world (AGoldNode deliberately opts out by not
	// implementing it). Re-gathered per tick — a handful of frames over a
	// ~30-actor roster; fresh gathers make mid-sweep deaths/spawns trivially safe.
	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	for (AActor* Candidate : TeamAgents)
	{
		if (!IsValid(Candidate) || AppliedTargets.Contains(Candidate))
		{
			continue;
		}

		// enemies only (§3.0 — the team filter is the friendly-fire authority,
		// never the instigator chain; native cast valid — UTeamAgent is
		// NotBlueprintable, the SpellLibrary precedent)
		const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
		if (!Agent || Agent->GetTeamId() == CasterTeam)
		{
			continue;
		}

		// reach: nearest point of the swept segment to the candidate, then the
		// house closest-point-on-collision measure from there (qa/TASK-026
		// convention — large fortification footprints are "on the line" at
		// their wall, not only at their origin)
		const FVector NearestOnSegment = FMath::ClosestPointOnSegment(Candidate->GetActorLocation(), LineOrigin, SegmentEnd);
		if (DistanceToTargetCollision(NearestOnSegment, Candidate) > LineHalfWidth)
		{
			continue;
		}

		// processed exactly once — applied AND excluded targets are both
		// marked so nothing is ever re-tested on later ticks
		if (ApplyEffectToTarget(Candidate))
		{
			++AppliedCount;
		}
		AppliedTargets.Add(Candidate);
	}
}

bool ASpellLineSweep::ApplyEffectToTarget(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	switch (Row.SpellEffect)
	{
	case ESpellEffect::AoEDamage:
	{
		// Fireball line: every live enemy ITeamAgent — units, hero, buildings,
		// and the CASTLE, which scales the Spell-typed hit to 50% on its own
		// side (§3.11), exactly the old radial-blast semantics. Liveness gates
		// mirror ResolveTopTargetsDamage (never punch a corpse).
		if (const ASummonedUnit* Unit = Cast<ASummonedUnit>(Target))
		{
			if (Unit->IsUnitDead())
			{
				return false;
			}
		}
		else if (const ABuilding* Building = Cast<ABuilding>(Target))
		{
			if (Building->IsBuildingDestroyed())
			{
				return false;
			}
		}
		else if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(Target))
		{
			if (Hero->IsDead())
			{
				return false;
			}
		}
		else if (const ACastle* Castle = Cast<ACastle>(Target))
		{
			if (Castle->IsCastleDestroyed())
			{
				return false;
			}
		}
		else
		{
			return false; // unknown ITeamAgent type — never a target (defensive)
		}

		// null instigator/causer on purpose: the enemy-only filter above is
		// the friendly-fire authority (the ApplyRadialDamage design); each hit
		// routes through the receiver's own TakeDamage so per-fortification
		// scaling applies (TASK-002 chain).
		UGameplayStatics::ApplyDamage(
			Target, Row.Damage,
			/*EventInstigator=*/ nullptr, /*DamageCauser=*/ nullptr,
			USiegeDamageType_Spell::StaticClass());
		return true;
	}

	case ESpellEffect::Freeze:
	{
		// FrostNova line: enemy units and buildings only — the castle is NEVER
		// freezable and the hero is not freezable (M5 ruling 5 type filter,
		// unchanged by the delivery overhaul). Refresh-not-stack and
		// match-end-freeze precedence live inside ApplyFreeze (TASK-099).
		if (ASummonedUnit* Unit = Cast<ASummonedUnit>(Target))
		{
			if (!Unit->IsUnitDead())
			{
				Unit->ApplyFreeze(Row.EffectDuration);
				return true;
			}
			return false;
		}
		if (ABuilding* Building = Cast<ABuilding>(Target))
		{
			if (!Building->IsBuildingDestroyed())
			{
				Building->ApplyFreeze(Row.EffectDuration);
				return true;
			}
			return false;
		}
		return false; // castle/hero excluded by ruling 5 — no branch on purpose
	}

	default:
		// the resolver only spawns sweeps for the two line effects; anything
		// else here is a caller regression — warn once per sweep via the
		// completion count staying 0 and this Verbose line
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASpellLineSweep '%s': unsupported SpellEffect %d on '%s' — no effect applied."),
			*GetNameSafe(this), static_cast<int32>(Row.SpellEffect), *CardID.ToString());
		return false;
	}
}

float ASpellLineSweep::DistanceToTargetCollision(const FVector& From, const AActor* Target)
{
	if (!Target)
	{
		return TNumericLimits<float>::Max();
	}

	FVector ClosestPoint = FVector::ZeroVector;
	const float Distance = Target->ActorGetDistanceToCollision(From, ECC_Pawn, ClosestPoint);
	if (Distance < 0.f)
	{
		// no ECC_Pawn-blocking collision — actor-origin fallback (shared convention)
		return static_cast<float>(FVector::Dist(From, Target->GetActorLocation()));
	}
	return Distance;
}
