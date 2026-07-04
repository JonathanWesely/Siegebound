// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Siegebound/Building.h"
#include "Barracks.generated.h"

struct FCardRow;

/**
 *  Siegebound spawner building (GDD §4 Barracks, TASK-057): an ABuilding that
 *  periodically summons friendly units and then self-destructs. BP child:
 *  BP_Building_Barracks (CardID Barracks — row: 250 HP, and the spawner triple
 *  SpawnCardID Footman / SpawnInterval 8 / Lifetime 60; all read from DT_Cards).
 *
 *  - On BeginPlay the base binds HP and fires OnStatsLoaded, where this class
 *    reads the spawner columns and arms a LOOPING spawn timer at SpawnInterval
 *    (8 s) plus a ONE-SHOT self-destruct timer at Lifetime (60 s). First spawn
 *    lands one full interval after the stats bind (the ATower first-shot
 *    precedent — a fresh Barracks is an 8 s commitment, not an instant burst).
 *  - Each spawn tick composes the BP for the row's SpawnCardID
 *    (/Game/Blueprints/Units/BP_Unit_<SpawnCardID> — Footman, CONVENTIONS
 *    composed soft-class path, null-safe) and deferred-spawns it Team = own team
 *    at a navmesh-projected point a short offset toward the enemy (units then
 *    path to the enemy castle via the base state machine). InitUnit sets the
 *    team BEFORE BeginPlay, so the TASK-044 team material lands correctly.
 *  - At Lifetime the Barracks destroys itself; its already-spawned units are
 *    independent actors and PERSIST (the §4 "expiring spawner" rule).
 *  - Match-end freeze (TASK-024 sweep): FreezeAI() stops both timers so nothing
 *    spawns under the Victory screen. The game mode's FreezeWorldAtMatchEnd
 *    iterates ABarracks and calls it, the same way it FreezeAI()s units and
 *    silences towers. PlayAgain destroys every building regardless.
 *  - Spawner guard (the ATower Cadence-guard precedent, qa/TASK-021 WARN-1):
 *    SpawnInterval <= 0 or an empty SpawnCardID never arms the loop (logged;
 *    the Barracks stands but never spawns) — SetTimer with a rate <= 0 would
 *    CLEAR instead of schedule, and a non-spawning row is a data error, never
 *    clamped into validity.
 *
 *  Stationary and destructible via the ABuilding base (250 HP from the row,
 *  BlockAll collision, navigation-relevant). No mesh/material in C++ — the BP
 *  child assigns SM_Barracks + the team material (TASK-063).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ABarracks : public ABuilding
{
	GENERATED_BODY()

public:

	ABarracks();

	/**
	 *  Match-end freeze (TASK-057; called by ASiegeGameMode::FreezeWorldAtMatchEnd
	 *  the same way units are FreezeAI()d and towers silenced): permanently stops
	 *  the spawn loop AND the self-destruct timer so the Barracks summons nothing
	 *  and does nothing under the Victory screen until PlayAgain destroys it.
	 *  Idempotent and safe before stats bind. Already-spawned units are separate
	 *  actors, frozen by the game mode's own ASummonedUnit sweep.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Building")
	void FreezeAI();

	/** True once FreezeAI ran (match-end verification hook). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	bool IsAIFrozen() const { return bFrozen; }

protected:

	/** Clears the spawn + lifetime timers (runs synchronously inside Destroy — a dead Barracks can never spawn again). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  ABuilding stat hook: reads the spawner triple (SpawnCardID / SpawnInterval
	 *  / Lifetime) from the row the base just loaded HP from, then arms the
	 *  looping spawn timer (SpawnInterval > 0 rows only) and the one-shot
	 *  self-destruct timer (Lifetime > 0 rows only). Fired once, post-HP-bind.
	 */
	virtual void OnStatsLoaded(const FCardRow& Row) override;

	/**
	 *  Distance, along the direction toward the enemy half (CONVENTIONS world
	 *  axes: Blue advances +X, Red advances -X), at which spawned units appear —
	 *  clear of the Barracks footprint (SM_Barracks is ~400×400, TASK-067). A
	 *  feel value, not a GDD stat; the point is navmesh-projected and the unit
	 *  paths to the enemy castle from there regardless.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Building", meta = (ClampMin = "0"))
	float SpawnFrontOffset = 300.f;

	/** Half-extent for snapping the spawn point onto the navmesh (generous vertical so a ground-Z guess still finds the floor; the §3.5 ProjectPointToNavigation rule). */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Building")
	FVector NavProjectionExtent = FVector(300.f, 300.f, 1000.f);

private:

	/** Spawn-timer callback: composes + deferred-spawns one SpawnUnitCardID unit of our team at the navmesh-projected front point. No-op once destroyed/frozen. */
	void SpawnUnit();

	/** Lifetime-timer callback: the §4 expiry — destroys the Barracks (its already-spawned units persist as independent actors). */
	void HandleLifetimeExpired();

	/** Resolves /Game/Blueprints/Units/BP_Unit_<SpawnUnitCardID>_C once, caches it, and validates it is an ASummonedUnit (null-safe: missing BP = no spawn + log). */
	UClass* ResolveSpawnUnitClass();

	/** Card row this Barracks spawns (row SpawnCardID: Footman), read from DT_Cards at OnStatsLoaded — never hardcoded. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	FName SpawnUnitCardID = NAME_None;

	/** Seconds between spawns, from the row (Barracks: 8). <= 0 means the loop was never armed. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	float SpawnIntervalSeconds = 0.f;

	/** Seconds until self-destruct, from the row (Barracks: 60). <= 0 means the Barracks never self-destructs. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	float LifetimeSeconds = 0.f;

	/** Composed spawn BP class, resolved + cached on the first spawn — never a per-tick sync load. */
	UPROPERTY(Transient)
	TObjectPtr<UClass> CachedSpawnUnitClass;

	/** True once FreezeAI ran: both timers are cleared and stay cleared until the actor is destroyed. */
	bool bFrozen = false;

	/** Drives SpawnUnit every SpawnIntervalSeconds, armed once in OnStatsLoaded (SpawnInterval > 0 rows only). */
	FTimerHandle SpawnTimerHandle;

	/** Drives HandleLifetimeExpired once, LifetimeSeconds after the stats bind (Lifetime > 0 rows only). */
	FTimerHandle LifetimeTimerHandle;

	/** One-shot guard for the missing-spawn-BP warning (so a missing BP logs once, not every SpawnInterval). */
	bool bWarnedNoSpawnClass = false;
};
