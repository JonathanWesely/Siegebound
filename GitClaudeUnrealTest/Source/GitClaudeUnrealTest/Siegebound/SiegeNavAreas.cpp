// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeNavAreas.h"

// TASK-534, complete-type include law. SiegeNavAreas.h only ever sees the FORWARD
// declaration AIController.h:40 provides (enough to DECLARE a const-ref parameter);
// reading Result.Code and naming EPathFollowingResult::Blocked needs the real
// definitions (PathFollowingComponent.h:53 and :121), and Cast<ASummonedUnit> plus
// the NotifyMoveBlocked call need the real unit class. All three live here, in the
// .cpp, so no translation unit that merely includes SiegeNavAreas.h pays for them.
#include "Navigation/PathFollowingComponent.h"
#include "Siegebound/SiegeStuckStatics.h"
#include "Siegebound/SummonedUnit.h"

/**
 *  Per-unit throttle window for the harvested "blocked:" line, in seconds
 *  (TASK-534 clause 5 — throttling is part of the deliverable, and the window is a
 *  named constant rather than a bare literal at the call site).
 *
 *  WHY 10 AND NOT 5: the engine cannot re-declare Blocked for the same unit faster
 *  than its own sample window allows — BlockDetectionInterval 0.5 s x
 *  BlockDetectionSampleCount 10 = 5.0 s, plus the 0.25 s UpdateState poll that
 *  re-issues the move and refills the sample buffer, so ~5.25 s is the floor on the
 *  natural event period. A window at or under that floor would throttle NOTHING. At
 *  10 s a permanently wedged unit prints roughly half as often as it wedges, and a
 *  fully wedged 120-unit army costs at most ~12 lines/s instead of ~23 — while a
 *  unit's FIRST block still prints immediately, so the evidence is never lost.
 *
 *  ⛔ Deliberately NOT a UPROPERTY: NAV-§7 pins FSiegeStuckTuning's fields as the
 *  ONLY tunables this feature adds. This is a log-hygiene constant, not a feel knob.
 */
static constexpr double SiegeBlockedLogThrottleSeconds = 10.0;

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

void ASiegeUnitAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	// ⛔ ALWAYS FIRST, ALWAYS UNCONDITIONAL. The base broadcasts ReceiveMoveCompleted
	// (the Blueprint "MoveCompleted" delegate) and forwards to the deprecated
	// overload — AIController.cpp:985-989, read directly. Skipping it would silently
	// break brain-component and Blueprint notifications that nobody would ever
	// connect back to this change.
	Super::OnMoveCompleted(RequestID, Result);

	// ⛔ Blocked ONLY (TASK-534 clause 3). Aborted fires every time a unit re-targets
	// (a new request aborts the previous one), OffPath / Invalid / Success all have
	// their own legitimate causes, and this batch has NO runtime evidence about any
	// of them. Acting on them would manufacture stalls that are not stalls.
	if (Result.Code != EPathFollowingResult::Blocked)
	{
		return;
	}

	// Null-safe by construction: GetPawn() is null across unpossess and actor
	// teardown, and the cast legitimately fails for pawns this controller could be
	// possessing that are not fleet units. AMinerUnit derives from ASummonedUnit, so
	// this one cast covers the whole fleet. A pawn we do not own is not an error —
	// return quietly, log nothing.
	ASummonedUnit* Unit = Cast<ASummonedUnit>(GetPawn());
	if (!Unit)
	{
		return;
	}

	// The throttle governs the LOG and nothing else. A wedged army must not spam at
	// default verbosity, but the stamp is monotonic and needs no reset: "time since
	// the last line" is self-correcting, and resetting it would require reacting to
	// Success, which clause 3 forbids. World-null (teardown) simply skips the line.
	if (const UWorld* World = GetWorld())
	{
		const double NowSeconds = World->GetTimeSeconds();
		if (LastBlockedLogTimeSeconds < 0.0
			|| (NowSeconds - LastBlockedLogTimeSeconds) >= SiegeBlockedLogThrottleSeconds)
		{
			LastBlockedLogTimeSeconds = NowSeconds;

			// FPathFollowingResult::ToString() prints "Blocked[Blocked]" for this code
			// (PathFollowingComponent.cpp:76-80) — it allocates an FString, which is why
			// it is built INSIDE the throttled branch and never on the hot path.
			//
			// ⚠️ The format literal is deliberately PURE ASCII (no em-dash). This file
			// is UTF-8 with no BOM, and this string is the TASK-537 gate's grep target
			// and Jonathan's playtest evidence — it must survive any codepage.
			UE_LOG(LogSiegeStuck, Log,
				TEXT("[Stuck] blocked: unit='%s' team=%s result=%s reqId=%u loc=%s - engine path-following ")
				TEXT("block detection fired (agent moved under BlockDetectionDistance across its sample ")
				TEXT("window). Evidence only: no move is issued from OnMoveCompleted."),
				*GetNameSafe(Unit),
				(Unit->GetTeamId() == ETeamId::Red) ? TEXT("Red") : TEXT("Blue"),
				*Result.ToString(),
				RequestID.GetID(),
				*Unit->GetActorLocation().ToCompactString());
		}
	}

	// ⛔ NOT THROTTLED, AND THAT IS DELIBERATE — the window above rate-limits the LOG,
	// never the evidence. Dropping a NotifyMoveBlocked would silently starve the
	// watchdog of the very signal this task exists to harvest.
	//
	// ⛔⛔ AND THIS IS THE LAST STATEMENT IN THE FUNCTION, BY LAW (NAV-§3):
	// NotifyMoveBlocked may advance the stall clock so the NEXT watchdog tick fires a
	// rung; it may never fire one itself, and nothing here issues a move. The
	// tempting "we already know it's blocked, just re-path right here" is precisely
	// the double-driver defect this feature removes.
	Unit->NotifyMoveBlocked();
}
