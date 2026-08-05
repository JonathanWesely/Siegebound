// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Only ever taken BY POINTER in this header, never dereferenced here — the
// SiegeCombatStatics.h precedent (complete-type include law: the .cpp includes
// "Engine/World.h" for the complete type it actually reads through).
class UWorld;

/**
 *  ⛔ NOT `LogSiegeNav` — CONVENTIONS `NAV-§7`, log-category row (manager ruling 7b).
 *  `LogSiegeNet` already exists (SiegeSessionSubsystem.h:18) and the two would differ by
 *  ONE CHARACTER in a log file somebody is skimming at 2 a.m. A "simplification" back to
 *  `LogSiegeNav` re-introduces that collision and is a QA FINDING, not a cleanup.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeNavDiag, Log, All);

/**
 *  ═══ Siegebound navmesh-build telemetry (TASK-529, UNIT-PATHING batch — STAGE 0) ═══
 *
 *  ⭐ WHY THIS EXISTS. Units wedge on rocks mid-order. The verified mechanism
 *  (CONVENTIONS `NAV-§1`, cause 1) is that `bDoFullyAsyncNavDataGathering=True` forces the
 *  Recast generator to submit EXACTLY ONE tile task at a time
 *  (RecastNavMeshGenerator.cpp:5892-5896), so the runtime scatter's 738 blocking instances
 *  dirty ~326 tiles that then take ~216 s to drain at ~1.5 tiles/s. A unit ordered across
 *  the map paths over tiles that still hold the PRE-SCATTER bake: the navmesh says open
 *  ground, the rock's collider says no.
 *
 *  ⭐ AND WHY IT IS THE MOST IMPORTANT SMALL THING IN THE BATCH. `LogNavConfigOnce` reads
 *  `ARecastNavMesh::ShouldGatherDataOnGameThread()` OFF THE RUNNING ACTOR, which is the
 *  ONLY way to prove whether the TASK-530 ini flip reached the SERIALIZED `L_Arena` nav
 *  actor at all. That is the exact trap `Config/DefaultEngine.ini:281-284` documents in its
 *  own comment, and it is the conditional that TASK-540 branch A rolls back on
 *  (`NAV-§9` clause 3): a PIE log still reading `gatherOnGameThread=false` after the flip
 *  means the ini never landed, and the fix is the REVERT — ⛔ never an `L_Arena` save
 *  (`NAV-§5`).
 *
 *  ⛔ STAGE 0 — THIS LIBRARY CHANGES NOTHING. No members, no ticking, no timers, no writes
 *  to ANY engine object. Every entry point is a pure read of public API plus one `UE_LOG`.
 *  `NAV-§9` clause 1: a telemetry task that alters behaviour has broken the arrangement
 *  Jonathan agreed to. ⛔ It also ships with ZERO CALL SITES — TASK-535 (the exclusive
 *  owner of `BattlefieldScatter`) wires them — precisely so a reviewer can confirm the
 *  behaviour-free claim in ONE look at a diff containing only a new file pair.
 *
 *  Not a UObject / not reflected: a plain static library, so there is no BeginPlay, no GC
 *  surface, and NO Build.cs change — `NavigationSystem` and `AIModule` are already public
 *  dependencies. Precedent: `FSiegeCombatStatics` (SiegeCombatStatics.h:23),
 *  `FSiegeKeyboardLayoutStatics` (SiegeKeyboardLayoutStatics.h:60).
 *
 *  ⛔ PINNED — CONVENTIONS `NAV-§8`. Both signatures and the access level are the batch's
 *  link contract; five other tasks compile against them. "Improving" one breaks the link
 *  and is an automatic QA FAIL.
 *
 *  ⚖️ M8 DECLARATION: adds no replicated property, no new replicated class, no new
 *  relevancy tier. Structural reason (`NAV-§11`): this is a static library with no
 *  instances and no reflected symbol of any kind — there is nothing here a replication
 *  graph could ever see, and no later refactor can accidentally make one.
 *
 *  ═══ THE EMITTED LINES (⛔ the `key=` tokens are PINNED by `NAV-§7`; the gate greps them) ═══
 *
 *  LogNavConfigOnce — ONCE per world, at Log verbosity:
 *      LogSiegeNavDiag: nav-config: actor='RecastNavMesh-Default' gatherOnGameThread=true
 *      maxTileJobs=8 cellSize=32.00 tileSizeUU=2000.00 agentRadius=34.00 poolCap=1024
 *      fixedPool=true runtimeGen=Dynamic
 *
 *  LogNavBuildSnapshot — three times per match, at Log verbosity:
 *      LogSiegeNavDiag: nav-build [pre-scatter]: remaining=0 running=0 dirtyAreas=0
 *      hasDirty=false activeTiles=326 poolCap=1024
 *
 *  ⚠️ `true`/`false` ARE LOWERCASE ON PURPOSE — `NAV-§9` clause 3's revert clause is worded
 *  against the literal string `gatherOnGameThread=false`, so the casing is part of the
 *  evidence contract, not a style choice.
 *
 *  ⚠️ THE TWO `poolCap=` VALUES ARE DELIBERATELY DIFFERENT SOURCES AND THAT IS THE POINT:
 *  the config line prints the CONFIGURED `ARecastNavMesh::TilePoolSize`; the snapshot line
 *  prints the RUNTIME `GetNavMeshTilesCount()` (the pool the navmesh actually allocated,
 *  per the engine's own comment at RecastNavMesh.h:1169). With `bFixedTilePoolSize=True`
 *  they should agree — and if they ever DIVERGE, that divergence is itself the `NAV-§6`
 *  tile-pool signal. `activeTiles=` against `poolCap=` is the ONLY instrument this batch
 *  puts on that hazard (2.81 layers/column against the engine's own `AverageLayersPerTile`
 *  = 3), and overflow drops tiles with a PERMANENT hole that looks exactly like "stuck
 *  behind a rock, forever."
 */
class GITCLAUDEUNREALTEST_API FSiegeNavDiagnostics
{
public:

	/**
	 *  The RUNNING actor's config — this is the line that settles `NAV-§9`'s conditional.
	 *
	 *  Reads, off the live `ARecastNavMesh` and ⛔ never off the CDO and ⛔ never off the
	 *  ini (reading the ini would answer the wrong question — the whole point is what the
	 *  SERIALIZED ACTOR carries): `ShouldGatherDataOnGameThread()`,
	 *  `GetMaxSimultaneousTileGenerationJobsCount()`, `GetCellSize(Default)`, `TileSizeUU`,
	 *  `AgentRadius`, `TilePoolSize`, `bFixedTilePoolSize`, `GetRuntimeGenerationMode()`.
	 *
	 *  "Once" is ONCE PER WORLD, keyed on the world's `FObjectKey` — ⛔ deliberately NOT a
	 *  plain static bool, which would fire on the first PIE session of an editor run and
	 *  stay silent for every session after it (the evidence would be missing exactly when
	 *  somebody re-ran to check a fix).
	 *
	 *  ⚠️ The latch is taken ONLY on a line that was actually emitted. If the nav system or
	 *  the nav actor is not up yet, the call logs one `Verbose` miss line and leaves the
	 *  world UNLATCHED, so a later call still gets its chance.
	 *
	 *  No-op (one `Verbose` line at most) on a null World, no nav system, or no
	 *  `ARecastNavMesh`. ⛔ Never `check()`, never `ensure()` — a telemetry call that
	 *  crashes a match is infinitely worse than one that prints nothing.
	 *
	 *  ⚠️ Game thread only (it touches a shared static latch); a call from any other thread
	 *  logs one `Verbose` line and returns.
	 *
	 *  @param World world whose nav system/nav actor to report on; null is safe
	 */
	static void LogNavConfigOnce(const UWorld* World);

	/**
	 *  Queue depth + pool headroom. Tag ∈ "pre-scatter" | "post-scatter" | "at-confirmation".
	 *
	 *  ⭐ `remaining=` / `running=` / `dirtyAreas=` / `hasDirty=` are what answer WHY the
	 *  `BattlefieldScatter.cpp:1953` traversability poll reads "idle" at +5 s against a
	 *  ~3 %-built navmesh: `IsNavigationBeingBuilt` is a composite, and this line
	 *  decomposes it into the four numbers that actually moved (`NAV-§12`, the
	 *  "which of the two returned false is UNKNOWN" limitation — this is what closes it).
	 *
	 *  Emits ONE line at Log verbosity per call. ⛔ Not for per-tick use: `NAV-§9` ships
	 *  exactly three calls per match (TASK-535 owns them), and default-verbosity spam is
	 *  explicitly out of scope.
	 *
	 *  ⚠️ PARTIAL AVAILABILITY STILL PRINTS. If the nav system is up but no `ARecastNavMesh`
	 *  resolves, the line is still emitted with `activeTiles=-1 poolCap=-1` — `-1` means
	 *  "unavailable", never a real count. Dropping the whole line would throw away the
	 *  queue numbers, which are the half of this line the gate needs most.
	 *
	 *  No-op (one `Verbose` line at most) on a null World or no nav system. A null or empty
	 *  Tag is reported as `<none>` rather than dereferenced.
	 *
	 *  @param World world whose nav system/nav actor to report on; null is safe
	 *  @param Tag   where in the frame this snapshot was taken; null is safe
	 */
	static void LogNavBuildSnapshot(const UWorld* World, const TCHAR* Tag);
};
