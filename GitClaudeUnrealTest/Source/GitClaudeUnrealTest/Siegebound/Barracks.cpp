// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Barracks.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "NavigationSystem.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/SiegeSpawnConstants.h"
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"

ABarracks::ABarracks()
{
	// Per-card class (TASK-063 names block: BP_Building_Barracks with row
	// Barracks): the row IDENTITY is the class's nature, so it defaults here;
	// every stat on that row (HP + the spawner triple) still binds from DT_Cards
	// at BeginPlay, never from code (§3.0). TASK-063's BP sets the same value
	// (no-op) and the deferred-spawn InitBuilding(Team, "Barracks") agrees.
	CardID = FName(TEXT("Barracks"));
}

void ABarracks::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A destroyed Barracks (combat death, Lifetime expiry, PlayAgain sweep) can
	// never spawn again — clear both timers synchronously inside Destroy (the
	// ATower EndPlay precedent).
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ABarracks::OnStatsLoaded(const FCardRow& Row)
{
	Super::OnStatsLoaded(Row);

	// Bind the spawner triple from the row the base just loaded HP from (GDD §4:
	// never hardcoded — Barracks authors SpawnCardID Footman / SpawnInterval 8 /
	// Lifetime 60 in Docs/Data/cards.csv).
	SpawnUnitCardID = Row.SpawnCardID;
	SpawnIntervalSeconds = Row.SpawnInterval;
	LifetimeSeconds = Row.Lifetime;

	// Spawn loop — the ATower Cadence-guard precedent (qa/TASK-021 WARN-1,
	// BINDING): SetTimer with a rate <= 0 CLEARS the handle instead of scheduling
	// it, and a Barracks row with no SpawnInterval / no SpawnCardID is a data
	// error, not a valid non-spawning building. Refuse to arm (logged) and never
	// clamp a broken row into a firing one.
	if (SpawnIntervalSeconds > 0.f && !SpawnUnitCardID.IsNone())
	{
		// §4 "every 8 s": ONE looping timer, armed once, never stopped while the
		// Barracks lives (until FreezeAI / Lifetime / destruction). First spawn
		// lands one full interval after the stats bind.
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ABarracks::SpawnUnit, SpawnIntervalSeconds, /*bLoop=*/ true);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABarracks '%s': row '%s' has SpawnInterval %.2f / SpawnCardID '%s' — a spawner needs a positive interval and a spawn card (Barracks authors 8 / Footman). Barracks stands but never spawns."),
			*GetNameSafe(this), *CardID.ToString(), SpawnIntervalSeconds, *SpawnUnitCardID.ToString());
	}

	// Self-destruct — one-shot at Lifetime (§4 "self-destructs after 60 s"). A
	// non-positive Lifetime means "never expires" (logged); the spawn loop still
	// runs. SetTimer(<=0) would clear, so this is gated too.
	if (LifetimeSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ABarracks::HandleLifetimeExpired, LifetimeSeconds, /*bLoop=*/ false);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABarracks '%s': row '%s' has Lifetime %.2f — a Barracks should self-destruct after a finite time (row authors 60). It will persist until destroyed."),
			*GetNameSafe(this), *CardID.ToString(), LifetimeSeconds);
	}
}

void ABarracks::SpawnUnit()
{
	// Belt-and-braces: FreezeAI / Destroy clear this timer, so a frozen or dying
	// Barracks should never get here — but spawning from one would be wrong
	// enough to guard (the ATower ScanAndFire precedent).
	if (bFrozen || IsBuildingDestroyed())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* UnitClass = ResolveSpawnUnitClass();
	if (!UnitClass)
	{
		// missing BP already logged once in the resolver — skip this spawn tick.
		return;
	}

	// Front point: a short offset toward the enemy half (CONVENTIONS world axes —
	// Blue advances +X, Red advances -X), clear of the Barracks footprint. The
	// spawned unit paths to the enemy castle from here regardless of the exact
	// offset, so this only needs to clear the building's collision.
	const float ForwardSign = (Team == ETeamId::Blue) ? 1.f : -1.f;
	FVector SpawnPoint = GetActorLocation() + FVector(ForwardSign * SpawnFrontOffset, 0.f, 0.f);

	// Snap onto the navmesh (GDD §3.5 projection rule, the placement/bot pattern):
	// keeps the unit on walkable ground even if the raw front point clipped a
	// wall or edge. No nav system = spawn at the raw point (AdjustIfPossible...
	// below still lifts it out of ground collision).
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation Projected;
		if (NavSys->ProjectPointToNavigation(SpawnPoint, Projected, NavProjectionExtent))
		{
			SpawnPoint = Projected.Location;
		}
	}

	// Lift the spawn so the capsule stands on the projected ground (the shared
	// SiegeSpawn constants, mirroring ASiegePlayerController / ASiegeBotController).
	float CapsuleHalfHeight = SiegeSpawn::DefaultCapsuleHalfHeight;
	if (const ASummonedUnit* UnitCDO = UnitClass->GetDefaultObject<ASummonedUnit>())
	{
		if (const UCapsuleComponent* Capsule = UnitCDO->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnPoint + FVector(0.f, 0.f, CapsuleHalfHeight + SiegeSpawn::SpawnGroundClearance));

	// Deferred spawn (the TASK-004/030 preferred path): InitUnit binds Team +
	// CardID BEFORE BeginPlay reads DT_Cards, so the unit is never mis-teamed on
	// its first state check and the TASK-044 team material lands correctly.
	// Owner = this Barracks; Instigator = nullptr (the Barracks is not a pawn —
	// team attribution flows through the unit's own ITeamAgent, set by InitUnit).
	ASummonedUnit* Unit = World->SpawnActorDeferred<ASummonedUnit>(
		UnitClass, SpawnTransform, /*Owner=*/ this, /*Instigator=*/ nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Unit)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABarracks '%s': SpawnActorDeferred failed for '%s' (%s) — no unit this tick."),
			*GetNameSafe(this), *SpawnUnitCardID.ToString(), *GetNameSafe(UnitClass));
		return;
	}

	Unit->InitUnit(Team, SpawnUnitCardID);
	Unit->FinishSpawning(SpawnTransform);
}

void ABarracks::HandleLifetimeExpired()
{
	// §4 expiry: the Barracks goes away; the units it already spawned are
	// independent actors and are NOT touched — they keep advancing/fighting.
	// Destroy() runs EndPlay (clears the spawn timer) and unregisters VisualMesh
	// from the nav octree (the dynamic navmesh heals — the ABuilding contract).
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ABarracks '%s': Lifetime (%.0f s) elapsed — self-destructing (spawned units persist)."),
		*GetNameSafe(this), LifetimeSeconds);

	Destroy();
}

void ABarracks::FreezeAI()
{
	if (bFrozen)
	{
		return;
	}
	bFrozen = true;

	// Stop both timers so nothing spawns and no self-destruct fires under the
	// Victory screen — the match-end freeze contract (TASK-024 sweep). The
	// Barracks parks until PlayAgain destroys it (which runs EndPlay anyway).
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
}

UClass* ABarracks::ResolveSpawnUnitClass()
{
	if (CachedSpawnUnitClass)
	{
		return CachedSpawnUnitClass;
	}

	if (SpawnUnitCardID.IsNone())
	{
		// OnStatsLoaded would not have armed the spawn timer in this case — a
		// diagnostic guard only.
		return nullptr;
	}

	// CONVENTIONS composed soft-class path (the TASK-030 spawn law): row
	// SpawnCardID -> /Game/Blueprints/Units/BP_Unit_<CardID>_C (Footman). Missing
	// or wrong-base BP = no spawn + one warning (never a crash).
	const FString CardName = SpawnUnitCardID.ToString();
	const FString ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);

	UClass* ResolvedClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath)).LoadSynchronous();
	if (ResolvedClass && ResolvedClass->IsChildOf(ASummonedUnit::StaticClass()))
	{
		CachedSpawnUnitClass = ResolvedClass;
		return CachedSpawnUnitClass;
	}

	if (!bWarnedNoSpawnClass)
	{
		bWarnedNoSpawnClass = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABarracks '%s': spawn BP '%s' not found or not an ASummonedUnit (created in TASK-062) — Barracks spawns nothing until it exists."),
			*GetNameSafe(this), *ClassPath);
	}

	return nullptr;
}
