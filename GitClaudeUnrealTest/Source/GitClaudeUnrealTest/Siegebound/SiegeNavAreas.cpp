// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeNavAreas.h"

UNavArea_BlueCastleInterior::UNavArea_BlueCastleInterior(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Normal-cost, fully walkable area: navmesh generation and the OWN team's
	// pathing are byte-identical to NavArea_Default — exclusion is entirely on the
	// ENEMY's query filter (UNavFilter_TeamRed). DrawColor only aids the editor's
	// navmesh view at TASK-350 verification (Blue-ish per the §6 team palette).
	DefaultCost = 1.f;
	DrawColor = FColor(13, 77, 255);
}

UNavArea_RedCastleInterior::UNavArea_RedCastleInterior(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Mirror of the Blue area (see above) — excluded only by UNavFilter_TeamBlue.
	DefaultCost = 1.f;
	DrawColor = FColor(255, 26, 13);
}

UNavFilter_TeamBlue::UNavFilter_TeamBlue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Blue AI must never PATH into the RED castle's interior (CONVENTIONS
	// team-gating law, pathing lane). AddExcludedArea marks the area excluded for
	// every query built with this filter; paths toward goals inside it become
	// partial paths ending at the area boundary (no door pile-up).
	AddExcludedArea(UNavArea_RedCastleInterior::StaticClass());
}

UNavFilter_TeamRed::UNavFilter_TeamRed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Red AI must never PATH into the BLUE castle's interior — mirror of the Blue filter.
	AddExcludedArea(UNavArea_BlueCastleInterior::StaticClass());
}

TSubclassOf<UNavArea> SiegeTeamInteriorAreaClass(ETeamId Team)
{
	return (Team == ETeamId::Red)
		? TSubclassOf<UNavArea>(UNavArea_RedCastleInterior::StaticClass())
		: TSubclassOf<UNavArea>(UNavArea_BlueCastleInterior::StaticClass());
}

TSubclassOf<UNavigationQueryFilter> SiegeTeamNavFilterClass(ETeamId Team)
{
	return (Team == ETeamId::Red)
		? TSubclassOf<UNavigationQueryFilter>(UNavFilter_TeamRed::StaticClass())
		: TSubclassOf<UNavigationQueryFilter>(UNavFilter_TeamBlue::StaticClass());
}

void ASiegeUnitAIController::ApplyTeamNavigationFilter(ETeamId Team)
{
	// DefaultNavigationFilterClass is protected on AAIController — this public
	// wrapper is the whole reason the subclass exists. Every MoveTo issued with a
	// null FilterClass (all of ASummonedUnit's) resolves through this default.
	DefaultNavigationFilterClass = SiegeTeamNavFilterClass(Team);
}
