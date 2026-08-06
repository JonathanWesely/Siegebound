// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeAssistantRegionStatics.h"

// ⛔ THERE IS DELIBERATELY NOTHING ELSE TO INCLUDE (the FSiegeStuckStatics precedent).
// FVector, FVector2D and FMath all arrive COMPLETE through the header's CoreMinimal, and this
// translation unit names no engine type beyond them: no UWorld, no AActor, no AAncientGround,
// no ACaptureZone, no snapshot, no subsystem. An engine include appearing in this file means
// the predicate stopped being pure, and that costs TASK-549 its headless test (`AS-§21.9`).

bool FSiegeAssistantRegionStatics::IsPointInRegion(const FVector& Point, const FVector& Centre, const FVector2D& HalfExtent)
{
	// ⛔ BYTE-COPIED SEMANTICS, NOT INVENTED ONES (`SC-§15` third instance — see the header for
	// why neither shipped instance can be called). The two donors are identical to each other:
	//
	//     AAncientGround::IsPointInZone (AncientGround.cpp:143-151)
	//     ACaptureZone::IsPointInZone   (CaptureZone.cpp:112-118)
	//         const FVector Center = GetActorLocation();
	//         return FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X
	//             && FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y;
	//
	// The ONLY change is where the two operands come from: a CAPTURED centre/extent instead of
	// a live actor's origin and member (`AS-§21.4`, snapshot-time geometry). The expression
	// itself is unchanged, and it must stay unchanged — the assistant has to give the SAME
	// answer the game already gives.
	//
	// 2D (XY) box about Centre; Z IGNORED (a region test — units stand on climbable rises
	// inside the footprint). Boundary INCLUSIVE: `<=`, so a point exactly on the edge is IN.
	// A non-positive HalfExtent degenerates to an empty (or centre-line) region and is NOT
	// special-cased — see the header's edge case 3 for why that is fail-closed on purpose.
	return FMath::Abs(Point.X - Centre.X) <= HalfExtent.X
		&& FMath::Abs(Point.Y - Centre.Y) <= HalfExtent.Y;
}
