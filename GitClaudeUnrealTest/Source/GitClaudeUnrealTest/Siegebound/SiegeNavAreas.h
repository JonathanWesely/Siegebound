// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/EngineTypes.h"
#include "NavAreas/NavArea.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Siegebound/TeamId.h"
#include "SiegeNavAreas.generated.h"

/**
 *  TASK-349 — team-gated castle interior, ONE-HOME (CONVENTIONS "Castle 3× HOLLOW
 *  (2026-07-28)" team-gating law). This header is the single home for the whole
 *  team-gating vocabulary, BOTH lanes:
 *
 *  PHYSICAL truth (collision):
 *  - ECC_SiegeTeamBlue / ECC_SiegeTeamRed — the two custom OBJECT channels every
 *    combatant capsule (ASummonedUnit + subclasses, AHeroCharacter) is re-typed to
 *    by team at BeginPlay. Defined in Config/DefaultEngine.ini as
 *    GameTraceChannel1/2 with DefaultResponse=ECR_Block; keep the ini and these
 *    aliases in lockstep. Re-typing changes ONLY the capsule's object type, never
 *    its response matrix — so every response-based query in the codebase
 *    (ActorGetDistanceToCollision on ECC_Pawn: hero melee, projectiles, spells,
 *    unit attack range) is byte-identical. ACastle::GateBlockerVolume blocks the
 *    ENEMY channel and ignores the own one, so each castle's gate admits its own
 *    team and physically stops the other — INCLUDING the player-driven hero
 *    (hero ruling: blocked at the enemy gate, default FLAGGED to Jonathan).
 *
 *  PATHING truth (navigation):
 *  - UNavArea_BlueCastleInterior / UNavArea_RedCastleInterior — normal-cost nav
 *    areas ACastle::InteriorNavModifier stamps over the castle's bounds (team-
 *    selected at BeginPlay). Traversal cost 1 — navmesh GENERATION is untouched;
 *    exclusion is entirely filter-side.
 *  - UNavFilter_TeamBlue / UNavFilter_TeamRed — per-team query filters; each
 *    EXCLUDES the enemy castle's interior area, so enemy AI never PATHS inside
 *    (no door pile-up: their paths end at the area boundary, not at the gate).
 *  - ASiegeUnitAIController — the unit fleet's AI controller. AAIController with
 *    exactly one addition: a public setter for the protected
 *    DefaultNavigationFilterClass, pushed by the pawn at its team-set sites.
 *
 *  The gating lives on ACTOR components + controllers — a crumble mesh swap on
 *  SM_Castle can never strip it. Everything here is null-safe by construction:
 *  a body that is never re-typed keeps ECC_Pawn behavior, a controller that is
 *  not an ASiegeUnitAIController simply gets no filter (pre-feature pathing),
 *  and the areas/filters degrade to the default filter when unresolved.
 */

/** Combatant object channel, Blue team (ini: GameTraceChannel1 / "SiegeTeamBlue"). */
constexpr ECollisionChannel ECC_SiegeTeamBlue = ECC_GameTraceChannel1;

/** Combatant object channel, Red team (ini: GameTraceChannel2 / "SiegeTeamRed"). */
constexpr ECollisionChannel ECC_SiegeTeamRed = ECC_GameTraceChannel2;

/** Object channel a Team's combatant bodies are stamped with (capsule re-type at BeginPlay). */
constexpr ECollisionChannel SiegeTeamObjectChannel(ETeamId Team)
{
	return (Team == ETeamId::Red) ? ECC_SiegeTeamRed : ECC_SiegeTeamBlue;
}

/** Object channel of Team's ENEMY — the channel a Team-owned GateBlockerVolume blocks. */
constexpr ECollisionChannel SiegeEnemyTeamObjectChannel(ETeamId Team)
{
	return (Team == ETeamId::Red) ? ECC_SiegeTeamBlue : ECC_SiegeTeamRed;
}

/**
 *  Nav area marking the BLUE castle's interior/footprint (CONVENTIONS naming law).
 *  Normal traversal cost — Blue (and neutral queries) walk it freely; only
 *  UNavFilter_TeamRed excludes it.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UNavArea_BlueCastleInterior : public UNavArea
{
	GENERATED_UCLASS_BODY()
};

/** Nav area marking the RED castle's interior/footprint — mirror of the Blue area; excluded only by UNavFilter_TeamBlue. */
UCLASS()
class GITCLAUDEUNREALTEST_API UNavArea_RedCastleInterior : public UNavArea
{
	GENERATED_UCLASS_BODY()
};

/** Blue team's pathfinding filter: excludes the RED castle's interior area (Blue AI never paths inside the enemy keep). */
UCLASS()
class GITCLAUDEUNREALTEST_API UNavFilter_TeamBlue : public UNavigationQueryFilter
{
	GENERATED_UCLASS_BODY()
};

/** Red team's pathfinding filter: excludes the BLUE castle's interior area — mirror of the Blue filter. */
UCLASS()
class GITCLAUDEUNREALTEST_API UNavFilter_TeamRed : public UNavigationQueryFilter
{
	GENERATED_UCLASS_BODY()
};

/** The team's interior area class (what the castle's InteriorNavModifier stamps). Never null. */
GITCLAUDEUNREALTEST_API TSubclassOf<UNavArea> SiegeTeamInteriorAreaClass(ETeamId Team);

/** The team's pathfinding filter class (what unit AI controllers adopt as DefaultNavigationFilterClass). Never null. */
GITCLAUDEUNREALTEST_API TSubclassOf<UNavigationQueryFilter> SiegeTeamNavFilterClass(ETeamId Team);

/**
 *  Unit-fleet AI controller (TASK-349). Behaviorally a plain AAIController — the
 *  ONLY addition is the public team-filter setter below, because
 *  AAIController::DefaultNavigationFilterClass is protected and the CONVENTIONS
 *  team-gating law wires the filter per POSSESSED PAWN's team, which only the
 *  pawn knows (ASummonedUnit pushes it from its team-set sites: BeginPlay,
 *  PossessedBy, and a post-BeginPlay InitUnit team update). Every MoveToActor /
 *  MoveToLocation issued with a null FilterClass — i.e. every existing call in
 *  ASummonedUnit, untouched — then picks the team filter up automatically.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeUnitAIController : public AAIController
{
	GENERATED_BODY()

public:

	/** Adopts the team's query filter as this controller's default pathfinding filter (idempotent; safe to re-push on a late team change). */
	void ApplyTeamNavigationFilter(ETeamId Team);

	/**
	 *  TASK-534 (CONVENTIONS NAV-§1 Cause 2 / NAV-§3) — harvests the stuck verdict
	 *  the engine is ALREADY computing and this project has never asked for.
	 *
	 *  UPathFollowingComponent runs block detection by default
	 *  (PathFollowingComponent.cpp:137-139 + :1560-1603 — BlockDetectionDistance=10,
	 *  BlockDetectionInterval=0.5, BlockDetectionSampleCount=10, bUseBlockDetection
	 *  true) and finishes the request with EPathFollowingResult::Blocked
	 *  (:1111) once the agent has moved under 10 uu across the 5.0 s sample window.
	 *  Before this override there was NO OnMoveCompleted anywhere in the project and
	 *  zero EPathFollowingResult references, so that verdict was DISCARDED — and
	 *  0.25 s later ASummonedUnit::EnterAdvance's Idle branch (SummonedUnit.cpp:2440)
	 *  re-issued the byte-identical request: same start poly, same goal, same filter,
	 *  same path, same rock. A silent 5-second livelock with no telemetry.
	 *
	 *  ⛔ THIS FUNCTION IS EVIDENCE, NOT ACTION (NAV-§3). It records and returns. It
	 *  issues NO move — no MoveTo*, no StopMovement, no steering of any kind — because
	 *  only ASummonedUnit::TickStuckWatchdog may steer. Two code paths that can both
	 *  re-path on one UPathFollowingComponent is the TASK-280/282 mill this feature
	 *  exists to remove, and re-pathing here would be an automatic QA FAIL.
	 *
	 *  ⚠️ THE OVERLOAD IS DELIBERATE. AAIController declares OnMoveCompleted TWICE:
	 *  the FPathFollowingResult form (AIController.h:230) and a
	 *  UE_DEPRECATED_FORGAME(4.13) EPathFollowingResult::Type form (:233). This
	 *  overrides the FORMER. In a non-engine module UE_DEPRECATED_FORGAME expands to
	 *  UE_DEPRECATED (UEBuildModuleCPP.cs:1693), so overriding the latter from this
	 *  module would emit C4996. Zero polling cost: this is purely event-driven.
	 */
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:

	/**
	 *  World-time stamp (seconds) of the last "blocked:" line this controller emitted;
	 *  NEGATIVE means "never logged" — world time is never negative — so a unit's
	 *  FIRST block always prints, which is what the TASK-537 gate greps for.
	 *
	 *  This IS the per-unit rate limit: every unit auto-possesses its own controller
	 *  instance (SummonedUnit.cpp:105-106), so per-controller state is per-unit state.
	 *  ⛔ It throttles the LOG ONLY and never the NotifyMoveBlocked evidence feed.
	 */
	double LastBlockedLogTimeSeconds = -1.0;
};
