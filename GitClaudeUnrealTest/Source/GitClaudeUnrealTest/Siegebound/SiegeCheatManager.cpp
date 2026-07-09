// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeCheatManager.h"

#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/Castle.h"
#include "Siegebound/HealthBarTarget.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/TeamId.h"

namespace
{
	/** How far the "crosshair" (camera forward) trace reaches for both cheats. */
	constexpr float SiegeCheatTraceDistance = 100000.f;

	/**
	 *  Alive across the combat types: units/buildings/hero answer IHealthBarTarget;
	 *  ACastle keeps its own destroyed latch (it does NOT implement IHealthBarTarget,
	 *  CONVENTIONS M5.5 note). Unknown ITeamAgent types are treated as alive.
	 */
	bool IsCombatActorAlive(const AActor* Actor)
	{
		if (const IHealthBarTarget* Bar = Cast<IHealthBarTarget>(Actor))
		{
			return Bar->IsHealthBarActorAlive();
		}
		if (const ACastle* Castle = Cast<ACastle>(Actor))
		{
			return !Castle->IsCastleDestroyed();
		}
		return true;
	}

	/**
	 *  Camera-forward "crosshair" line trace on ECC_Visibility (the same channel the
	 *  controller's cursor trace uses), ignoring the controlled pawn. True on a
	 *  blocking hit.
	 */
	bool TraceFromCrosshair(APlayerController* PC, UWorld* World, FHitResult& OutHit)
	{
		if (!PC || !World)
		{
			return false;
		}
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * SiegeCheatTraceDistance;

		FCollisionQueryParams Params(FName(TEXT("SiegeCheatCrosshair")), /*bTraceComplex=*/false);
		if (const APawn* ViewPawn = PC->GetPawn())
		{
			Params.AddIgnoredActor(ViewPawn);
		}
		return World->LineTraceSingleByChannel(OutHit, ViewLocation, TraceEnd, ECC_Visibility, Params);
	}

	/** Nearest alive ITeamAgent whose team differs from MyTeam, measured from RefLocation. */
	AActor* FindNearestEnemy(UWorld* World, ETeamId MyTeam, const FVector& RefLocation)
	{
		if (!World)
		{
			return nullptr;
		}
		TArray<AActor*> TeamAgents;
		UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

		AActor* Best = nullptr;
		float BestDistSq = TNumericLimits<float>::Max();
		for (AActor* Candidate : TeamAgents)
		{
			if (!IsValid(Candidate))
			{
				continue;
			}
			const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
			if (!Agent || Agent->GetTeamId() == MyTeam)
			{
				continue;
			}
			if (!IsCombatActorAlive(Candidate))
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(RefLocation, Candidate->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Candidate;
			}
		}
		return Best;
	}

	/** The local player's team, read from the controlled pawn if it is an ITeamAgent; Blue otherwise (CONVENTIONS team contract). */
	ETeamId ResolveLocalTeam(const APlayerController* PC)
	{
		if (PC)
		{
			if (const ITeamAgent* PawnAgent = Cast<ITeamAgent>(PC->GetPawn()))
			{
				return PawnAgent->GetTeamId();
			}
		}
		return ETeamId::Blue;
	}
}

void USiegeCheatManager::SummonTestUnit(FString CardID, bool bRed)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SummonTestUnit — no owning PlayerController; nothing spawned."));
		return;
	}
	UWorld* World = PC->GetWorld();
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SummonTestUnit — no World; nothing spawned."));
		return;
	}

	const FName CardName(*CardID);
	if (CardName.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SummonTestUnit — empty CardID; nothing spawned."));
		return;
	}

	// Compose the SAME unit BP path ASiegePlayerController::ResolveCardActorClass /
	// ASiegeBotController::ResolveBotCardActorClass compose for a unit card
	// (CONVENTIONS composed soft-class law). A building/spell CardID has no
	// BP_Unit_ class, so this refuses cleanly — this cheat summons UNIT cards.
	const FString ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardID, *CardID);
	UClass* UnitClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath)).LoadSynchronous();
	if (!UnitClass || !UnitClass->IsChildOf(ASummonedUnit::StaticClass()))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("USiegeCheatManager::SummonTestUnit — '%s' missing or not an ASummonedUnit (composed '%s'); this cheat summons UNIT cards only."),
			*CardID, *ClassPath);
		return;
	}

	const ETeamId Team = bRed ? ETeamId::Red : ETeamId::Blue;

	// Ground point = the surface under the crosshair (camera forward trace); fall
	// back to the controlled pawn's location. SpawnUnitSwarm applies the capsule
	// lift, so this must be a GROUND point (the trace ImpactPoint is exactly that).
	FVector GroundPoint;
	FHitResult Hit;
	if (TraceFromCrosshair(PC, World, Hit))
	{
		GroundPoint = Hit.ImpactPoint;
	}
	else if (const APawn* ViewPawn = PC->GetPawn())
	{
		GroundPoint = ViewPawn->GetActorLocation();
	}
	else
	{
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		GroundPoint = ViewLocation + ViewRotation.Vector() * 800.f;
	}

	// Spawn via the SHARED shipping unit-spawn entry (the player confirm path AND
	// the bot both call this). Instigator = nullptr (team attribution resolves via
	// each unit's own ITeamAgent — the bot's team-neutral choice); Count 1 = a
	// single test unit (call again for a swarm).
	const TArray<ASummonedUnit*> Spawned = ASiegePlayerController::SpawnUnitSwarm(
		World, UnitClass, CardName, Team,
		/*SpawnOwner=*/ PC, /*SpawnInstigator=*/ nullptr,
		GroundPoint, /*Count=*/ 1, /*Radius=*/ 0.f);

	if (Spawned.Num() == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("USiegeCheatManager::SummonTestUnit — SpawnUnitSwarm produced no unit for '%s'."), *CardID);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeCheatManager::SummonTestUnit — spawned '%s' for team %s at %s."),
		*CardID, (Team == ETeamId::Red) ? TEXT("Red") : TEXT("Blue"), *GroundPoint.ToString());
}

void USiegeCheatManager::ApplyTestDamage(float Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — no owning PlayerController; no damage applied."));
		return;
	}
	UWorld* World = PC->GetWorld();
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — no World; no damage applied."));
		return;
	}
	if (Amount <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — Amount %.1f must be positive; no damage applied."), Amount);
		return;
	}

	const ETeamId MyTeam = ResolveLocalTeam(PC);

	// Prefer the combat actor under the crosshair; else the nearest enemy — the
	// deterministic locked-desktop fallback (no aiming needed to hit the enemy
	// castle / units and drive their HP + overhead bars).
	AActor* Target = nullptr;
	FHitResult Hit;
	if (TraceFromCrosshair(PC, World, Hit))
	{
		AActor* HitActor = Hit.GetActor();
		if (IsValid(HitActor) && Cast<ITeamAgent>(HitActor))
		{
			Target = HitActor;
		}
	}
	if (!Target)
	{
		FVector Ref;
		if (const APawn* RefPawn = PC->GetPawn())
		{
			Ref = RefPawn->GetActorLocation();
		}
		else
		{
			FVector ViewLocation = FVector::ZeroVector;
			FRotator ViewRotation = FRotator::ZeroRotator;
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
			Ref = ViewLocation;
		}
		Target = FindNearestEnemy(World, MyTeam, Ref);
	}
	if (!Target)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — no combat actor under the crosshair and no enemy found; no damage applied."));
		return;
	}

	// Normal engine TakeDamage path. Instigator + causer are BOTH null, so the
	// receiver's team resolver reads WORLD damage and applies regardless of the
	// target's team (a debug tool damages friend or foe); null damage-type = base
	// UDamageType (100%, no fortification scaling).
	const float Applied = UGameplayStatics::ApplyDamage(Target, Amount, /*EventInstigator=*/nullptr, /*DamageCauser=*/nullptr, /*DamageTypeClass=*/nullptr);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeCheatManager::ApplyTestDamage — requested %.1f on '%s'; %.1f applied (after receiver-side scaling)."),
		Amount, *GetNameSafe(Target), Applied);
}

void USiegeCheatManager::AddTestGold(int32 Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::AddTestGold — no owning PlayerController; no gold granted."));
		return;
	}

	ASiegePlayerState* SiegeState = PC->GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::AddTestGold — no ASiegePlayerState on the local player; no gold granted."));
		return;
	}

	// Gold choke-point API — clamp + OnGoldChanged broadcast preserved; AddGold
	// itself refuses+logs a non-positive amount.
	SiegeState->AddGold(Amount);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeCheatManager::AddTestGold — requested +%d gold on '%s' (now %d)."),
		Amount, *GetNameSafe(SiegeState), SiegeState->GetGold());
}
