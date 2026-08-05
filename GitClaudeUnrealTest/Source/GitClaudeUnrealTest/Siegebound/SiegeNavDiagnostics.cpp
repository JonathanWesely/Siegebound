// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeNavDiagnostics.h"

// IsInGameThread() — the shared static latch in LogNavConfigOnce is not synchronised.
#include "CoreGlobals.h"

// ENavigationDataResolution. Required as a COMPLETE type: GetCellSize takes it by value and
// we name ::Default. RecastNavMesh.h only forward-declares it in places, so it is included
// explicitly rather than inherited transitively (complete-type include law).
#include "AI/Navigation/NavigationDataResolution.h"

// FNavigationSystem::GetCurrent<T>(const UWorld*) — the CONST overload
// (NavigationSystemBase.h:121). ⚠️ UNavigationSystemV1::GetCurrent takes a NON-const UWorld*
// (NavigationSystem.h:1177-1178), so it cannot be used from these const-World signatures and
// ⛔ must not be reached by const_cast — this library never needs a mutable world.
#include "AI/NavigationSystemBase.h"

// TActorIterator — the fallback route to the nav actor when the nav system holds no
// registered MainNavData (see ResolveRecastNavMesh below).
#include "EngineUtils.h"

// UWorld, complete type: read through by FNavigationSystem::GetCurrent and TActorIterator.
#include "Engine/World.h"

// ANavigationData + ERuntimeGenerationType (NavigationData.h:523) — GetRuntimeGenerationMode's
// return type, switched on below, so the complete enum is required here.
#include "NavigationData.h"

// UNavigationSystemV1 — GetNumRemainingBuildTasks / GetNumRunningBuildTasks /
// GetNumDirtyAreas / HasDirtyAreasQueued / GetDefaultNavDataInstance.
#include "NavigationSystem.h"

// ARecastNavMesh — ShouldGatherDataOnGameThread, GetNumActiveTiles, GetNavMeshTilesCount and
// the live config members. ⭐ The whole point of this file.
#include "NavMesh/RecastNavMesh.h"

// FObjectKey — the per-world "already logged" latch key.
#include "UObject/ObjectKey.h"

DEFINE_LOG_CATEGORY(LogSiegeNavDiag);

namespace
{
	/**
	 *  Printed for `activeTiles=` / `poolCap=` when no ARecastNavMesh resolved. ⛔ Never a
	 *  real count — a real pool is >= 0 — so the gate can tell "unavailable" from "zero",
	 *  which for the NAV-§6 tile-pool hazard are opposite readings.
	 */
	constexpr int32 SiegeNavDiag_Unavailable = -1;

	/**
	 *  Prune the world latch below once it exceeds this many entries. One entry is added per
	 *  PIE session; each is ~16 bytes and can never false-positive (FObjectKey carries the
	 *  serial number, so a recycled UObject index resolves as a DIFFERENT key). This exists
	 *  only so a long editor session cannot grow the set without bound.
	 */
	constexpr int32 SiegeNavDiag_LatchPruneThreshold = 8;

	/**
	 *  Worlds whose config line has already been emitted.
	 *
	 *  ⚠️ THIS IS THE ONE PIECE OF STATE IN THE FILE AND IT IS THE ONE THE SPEC ASKED FOR:
	 *  TASK-529 (4) requires "once per world" and explicitly REFUSES a plain static bool,
	 *  which would fire on the first PIE session of an editor run and stay silent for every
	 *  session after — i.e. the evidence would be missing exactly when somebody re-ran PIE to
	 *  check a fix. Keying on the world's FObjectKey gives a fresh latch per PIE world.
	 *
	 *  ⛔ It is NOT gameplay state, it is NOT a member, and it is NOT a write to any engine
	 *  object — the Stage 0 "changes nothing" guarantee (NAV-§9 clause 1) is intact.
	 *  FObjectKey deliberately holds no GC reference, so this cannot keep a dead world alive.
	 */
	TSet<FObjectKey>& GetConfigLoggedWorlds()
	{
		static TSet<FObjectKey> LoggedWorlds;
		return LoggedWorlds;
	}

	/**
	 *  ⛔ LOWERCASE ON PURPOSE. NAV-§9 clause 3's revert clause is worded against the literal
	 *  string `gatherOnGameThread=false`, so the casing is part of the evidence contract.
	 */
	const TCHAR* SiegeNavDiag_BoolToken(const bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	/** ERuntimeGenerationType -> the `runtimeGen=` token. Names match NavigationData.h:523. */
	const TCHAR* SiegeNavDiag_RuntimeGenToken(const ERuntimeGenerationType Mode)
	{
		switch (Mode)
		{
		case ERuntimeGenerationType::Static:               return TEXT("Static");
		case ERuntimeGenerationType::DynamicModifiersOnly: return TEXT("DynamicModifiersOnly");
		case ERuntimeGenerationType::Dynamic:              return TEXT("Dynamic");
		case ERuntimeGenerationType::LegacyGeneration:     return TEXT("LegacyGeneration");
		default:                                           return TEXT("Unknown");
		}
	}

	/**
	 *  The RUNNING ARecastNavMesh, or null. ⛔ Never the CDO: both routes below yield a live
	 *  world actor.
	 *
	 *  Route 1 is the nav system's registered default nav data — correct, because it is the
	 *  instance the DEFAULT AGENT actually paths on, which is the one whose config governs
	 *  the units that wedge.
	 *
	 *  Route 2 (actor iteration) exists because route 1 returns null when registration FAILED
	 *  — and `RegistrationFailed_AgentNotValid` is a named risk of this very batch
	 *  (CONVENTIONS NAV-§2(a)). A serialized nav actor that is present in the level but
	 *  UNREGISTERED is a diagnosis worth printing, not a reason to print nothing.
	 */
	const ARecastNavMesh* SiegeNavDiag_ResolveRecastNavMesh(const UWorld* World, const UNavigationSystemV1* NavSys)
	{
		if (NavSys)
		{
			if (const ARecastNavMesh* MainNavMesh = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance()))
			{
				return MainNavMesh;
			}
		}

		if (World)
		{
			for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
			{
				if (const ARecastNavMesh* NavMesh = *It)
				{
					return NavMesh;
				}
			}
		}

		return nullptr;
	}
}

void FSiegeNavDiagnostics::LogNavConfigOnce(const UWorld* World)
{
	// The latch below is an unsynchronised shared static. Every wired call site (TASK-535)
	// is game-thread gameplay code; anything else returns without touching it.
	if (!IsInGameThread())
	{
		UE_LOG(LogSiegeNavDiag, Verbose, TEXT("nav-config: skipped — not on the game thread."));
		return;
	}

	if (!World)
	{
		UE_LOG(LogSiegeNavDiag, Verbose, TEXT("nav-config: skipped — no world."));
		return;
	}

	TSet<FObjectKey>& LoggedWorlds = GetConfigLoggedWorlds();
	const FObjectKey WorldKey(World);

	if (LoggedWorlds.Contains(WorldKey))
	{
		return;
	}

	// ⛔ Read the RUNNING actor, never the CDO and never the ini. Reading the ini would answer
	// the wrong question: DefaultEngine.ini:281-284 warns in its own comment that the
	// serialized L_Arena nav actor carries its OWN copies of these params, and whether the ini
	// reaches it is the entire thing this line exists to settle (NAV-§9).
	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	const ARecastNavMesh* NavMesh = SiegeNavDiag_ResolveRecastNavMesh(World, NavSys);

	if (!NavMesh)
	{
		// ⚠️ DELIBERATELY DOES NOT LATCH. The nav system/nav actor may simply not be up yet;
		// leaving the world unlatched lets a later call still produce the evidence.
		UE_LOG(LogSiegeNavDiag, Verbose,
			TEXT("nav-config: skipped — no ARecastNavMesh in world '%s' yet (navSystem=%s)."),
			*World->GetName(), SiegeNavDiag_BoolToken(NavSys != nullptr));
		return;
	}

	LoggedWorlds.Add(WorldKey);

	// Drop keys whose world is gone. Rebuilt rather than removed in place: in UE 5.8 TSet is a
	// define-injected alias over TCompactSet or TSparseSet (Containers/Set.h) depending on
	// UE_USE_COMPACT_SET_AS_DEFAULT, and a plain range-for depends on nothing that differs
	// between them. Runs at most once per SiegeNavDiag_LatchPruneThreshold PIE sessions.
	if (LoggedWorlds.Num() >= SiegeNavDiag_LatchPruneThreshold)
	{
		TSet<FObjectKey> SurvivingWorlds;
		SurvivingWorlds.Reserve(LoggedWorlds.Num());

		for (const FObjectKey& Key : LoggedWorlds)
		{
			if (Key.ResolveObjectPtr() != nullptr)
			{
				SurvivingWorlds.Add(Key);
			}
		}

		LoggedWorlds = MoveTemp(SurvivingWorlds);
	}

	const FString NavMeshName = NavMesh->GetName();

	// ⚠️ cellSize comes from GetCellSize(Default), NOT from ARecastNavMesh::CellSize. That
	// member is UE_DEPRECATED(all) in UE 5.8 (RecastNavMesh.h:710) and reading it would emit
	// C4996 in a build where warnings are errors; it is also WITH_EDITORONLY_DATA-migrated
	// (RecastNavMesh.cpp:680-686) so it is not the live value. The project's own ini comment
	// already says as much: "the loose CellSize/CellHeight keys are deprecated"
	// (DefaultEngine.ini:271-272) — the real value lives in NavMeshResolutionParams[Default].
	//
	// ⚠️ maxTileJobs is the CAP, not the effective concurrency: with gatherOnGameThread=true
	// the generator uses min(max(NumWorkerThreads*2, 1), maxTileJobs)
	// (RecastNavMeshGenerator.cpp:5465). With gatherOnGameThread=false it is bypassed entirely
	// and the engine submits exactly 1 (:5892-5896) — which is the whole bug.
	UE_LOG(LogSiegeNavDiag, Log,
		TEXT("nav-config: actor='%s' gatherOnGameThread=%s maxTileJobs=%d cellSize=%.2f tileSizeUU=%.2f agentRadius=%.2f poolCap=%d fixedPool=%s runtimeGen=%s"),
		*NavMeshName,
		SiegeNavDiag_BoolToken(NavMesh->ShouldGatherDataOnGameThread()),
		NavMesh->GetMaxSimultaneousTileGenerationJobsCount(),
		NavMesh->GetCellSize(ENavigationDataResolution::Default),
		NavMesh->TileSizeUU,
		NavMesh->AgentRadius,
		NavMesh->TilePoolSize,
		SiegeNavDiag_BoolToken(NavMesh->bFixedTilePoolSize != 0),
		SiegeNavDiag_RuntimeGenToken(NavMesh->GetRuntimeGenerationMode()));
}

void FSiegeNavDiagnostics::LogNavBuildSnapshot(const UWorld* World, const TCHAR* Tag)
{
	// A null/empty tag is reported, never dereferenced — a telemetry call may not crash a match.
	const TCHAR* const SafeTag = (Tag && *Tag) ? Tag : TEXT("<none>");

	if (!World)
	{
		UE_LOG(LogSiegeNavDiag, Verbose, TEXT("nav-build [%s]: skipped — no world."), SafeTag);
		return;
	}

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	if (!NavSys)
	{
		UE_LOG(LogSiegeNavDiag, Verbose, TEXT("nav-build [%s]: skipped — no navigation system."), SafeTag);
		return;
	}

	// ⭐ These four are what decompose BattlefieldScatter.cpp:1953's IsNavigationBeingBuilt
	// into its parts. That composite reads IDLE ~211 s before the queue drains, and NAV-§12
	// records which of its terms went false as UNKNOWN — this line is what closes that.
	const int32 Remaining  = NavSys->GetNumRemainingBuildTasks();
	const int32 Running    = NavSys->GetNumRunningBuildTasks();
	const int32 DirtyAreas = NavSys->GetNumDirtyAreas();
	const bool  bHasDirty  = NavSys->HasDirtyAreasQueued();

	// ⚠️ NAV-§6, the tile-pool hazard: activeTiles counts tiles that actually hold data,
	// poolCap is the allocated pool (RecastNavMesh.h:1168-1170). ⛔ INSTRUMENT ONLY — this
	// batch does NOT raise TilePoolSize. Overflow drops tiles with a PERMANENT hole and logs
	// "tile limit reached!" (RecastNavMeshGenerator.cpp:6366-6369), which would look exactly
	// like "stuck behind a rock, forever" — so the numbers are printed even though the hazard
	// has never once been observed to fire.
	int32 ActiveTiles = SiegeNavDiag_Unavailable;
	int32 PoolCap     = SiegeNavDiag_Unavailable;

	if (const ARecastNavMesh* NavMesh = SiegeNavDiag_ResolveRecastNavMesh(World, NavSys))
	{
		ActiveTiles = NavMesh->GetNumActiveTiles();
		PoolCap     = NavMesh->GetNavMeshTilesCount();
	}

	// ⚠️ The line is emitted even with no nav actor (activeTiles=-1 poolCap=-1): dropping it
	// would throw away the queue numbers, which are the half the gate needs most.
	UE_LOG(LogSiegeNavDiag, Log,
		TEXT("nav-build [%s]: remaining=%d running=%d dirtyAreas=%d hasDirty=%s activeTiles=%d poolCap=%d"),
		SafeTag,
		Remaining,
		Running,
		DirtyAreas,
		SiegeNavDiag_BoolToken(bHasDirty),
		ActiveTiles,
		PoolCap);
}
