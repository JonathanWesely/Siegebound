// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  ═══ Siegebound AI-commander region membership (TASK-544, AI-COMMANDER ROBUSTNESS batch) ═══
 *
 *  ⭐ WHY THIS EXISTS. The assistant is gaining a FIFTH `who` shape — `{"in": "<place>"}`
 *  (CONVENTIONS `AS-§21.5`) — so "send all units in the ancient ground to attack the castle"
 *  becomes expressible. The selector has to answer ONE question for each unit it considers:
 *  is this unit standing in that region? That question is this file, and nothing else is.
 *
 *  ⛔ SHIPS WITH ZERO CALL SITES, DELIBERATELY. TASK-548 adds the only caller (the executor).
 *  This is the `FSiegeNavDiagnostics` precedent (`NAV-§` RULING 2, `AS-§21.9`): a diff that is
 *  ONE NEW FILE PAIR proves behaviour-freedom in one look — nothing existing is edited, so
 *  nothing existing can have changed. ⚠️ Adding a call here would destroy that property.
 *
 *  ── ⚠️ DECLARED THIRD INSTANCE (`SC-§15`) ──────────────────────────────────────────────
 *
 *  This body ALREADY EXISTS TWICE in the module, and both instances are byte-identical to
 *  each other (verified by reading both, 2026-08-05):
 *
 *      AAncientGround::IsPointInZone   (AncientGround.cpp:143-151)
 *      ACaptureZone::IsPointInZone     (CaptureZone.cpp:112-118)
 *
 *          const FVector Center = GetActorLocation();
 *          return FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X
 *              && FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y;
 *
 *  ⛔ AND WHY A THIRD IS OWED RATHER THAN A CALL TO EITHER. Both existing instances are
 *  CONST MEMBER FUNCTIONS that read their geometry off a LIVE ACTOR (`GetActorLocation()` +
 *  the actor's own `ZoneHalfExtent`). This one takes a STORED centre and half-extent, because
 *  `AS-§21.4` rules the feature SNAPSHOT-TIME GEOMETRY, EXECUTION-TIME MEMBERSHIP: the
 *  snapshot captures each region's geometry ONCE, and the selector evaluates membership later
 *  from those captured values — WITHOUT holding an actor pointer. There is therefore no actor
 *  to call `IsPointInZone` on at the moment the answer is needed. ⇒ Reuse is not available;
 *  the honest move is a third instance, DECLARED, with the semantics copied rather than
 *  re-decided.
 *
 *  ⛔ THE NUMBER STILL HAS EXACTLY ONE OWNER, AND IT IS NOT THIS FILE. There is not a single
 *  numeric literal in this pair. The half-extent is read from the shipped accessors —
 *  `AAncientGround::GetZoneHalfExtent()` (`AncientGround.h:117`) and
 *  `ACaptureZone::GetZoneHalfExtent()` (`CaptureZone.h:131`) — by the snapshot's `Capture()`
 *  (TASK-547) and handed in as a parameter. ⚠️ Those two are a PAIRED TUNABLE that the actors'
 *  own class docs flag; hard-coding an extent here would silently fork it, which is the exact
 *  failure `AS-§21.4` names — "a unit that is in the ground for one system and not the other".
 *
 *  ⛔ PURE. No `UWorld`, no `AActor`, no `UObject`, no engine subsystem, no allocation, no
 *  state — three plain values in, a bool out. ⭐ That is what lets TASK-549 test it HEADLESSLY
 *  with no world, and it is why this predicate can be tested BETTER than the exclusion filter
 *  it mirrors, which has no automated test at all today.
 *
 *  Not a UObject / not reflected: a plain static library, so no BeginPlay, no GC surface and
 *  no `Build.cs` change (`Core`/`CoreUObject` already cover `FVector`/`FVector2D`). Shape
 *  precedents: `FSiegeCombatStatics` (`SiegeCombatStatics.h:23`), `FSiegeKeyboardLayoutStatics`,
 *  `FSiegeStuckStatics`.
 *
 *  📌 M8: adds no replicated property, no new replicated class, no new relevancy tier.
 */
struct GITCLAUDEUNREALTEST_API FSiegeAssistantRegionStatics
{
	/** 2D XY box, Z IGNORED. Boundary INCLUSIVE (<=), mirroring AAncientGround::IsPointInZone.
	 *
	 *  True iff Point lies within HalfExtent of Centre on BOTH the X and the Y axis.
	 *
	 *  ⛔ Z IS IGNORED ENTIRELY, AND THAT IS COPIED, NOT RE-DECIDED. Units stand on terrain of
	 *  varying height and the hills are climbable, so a 3D test would exclude a unit standing
	 *  on a rise INSIDE the ground — "everyone in the ancient ground" would quietly leave out
	 *  whoever walked uphill. Both shipped instances already made this choice and say so in
	 *  their own doc comments ("a unit on a slight rise inside the footprint still counts",
	 *  `AncientGround.h:108-110`). Centre.Z is read by nothing here; a caller may pass anything.
	 *
	 *  ── THE THREE EDGE CASES (TASK-549 asserts every one of them) ──────────────────────────
	 *
	 *  1. EXACTLY ON THE BOUNDARY  => INSIDE. The comparison is `<=`, not `<`, byte-for-byte
	 *     as both shipped instances. ⚖️ The assistant must give the SAME answer the game
	 *     already gives; flipping this to `<` would make one unit "in the mid" for capture
	 *     scoring and "not in the mid" for selection.
	 *
	 *  2. Z FAR ABOVE OR FAR BELOW  => STILL INSIDE, at any Z, including infinities. See above:
	 *     the Z difference is never computed.
	 *
	 *  3. A ZERO OR NEGATIVE HALF-EXTENT  => the region DEGENERATES; it is NOT special-cased,
	 *     and this is a choice, stated so it is not discovered later. A zero extent admits only
	 *     points exactly on that axis of the centre; a NEGATIVE extent admits NOTHING, because
	 *     `FMath::Abs(...)` is never negative. ⛔ No clamp, no `Abs` on the extent, no early
	 *     out. Two reasons: (a) any guard would be a DIVERGENCE from the two shipped instances,
	 *     which is precisely what this function exists not to be; and (b) the degenerate answer
	 *     is FAIL-CLOSED — an empty region selects nobody, and an empty selection is a LOUD
	 *     refusal with arithmetic in the executor (`AS-§21.11`, designed outcome 4), never a
	 *     silently unfiltered army. ⚠️ A non-finite (NaN) coordinate falls the same way: every
	 *     comparison against NaN is false, so the point is OUTSIDE. Fail-closed again.
	 *
	 *  @param Point      the world-space point under test (a unit's location at EXECUTION time)
	 *  @param Centre     the region's captured world-space centre; Centre.Z is IGNORED
	 *  @param HalfExtent the region's captured XY half-extent, from the shipped
	 *                    GetZoneHalfExtent() accessors — ⛔ never a literal
	 *  @return true iff Point is inside or exactly on the 2D XY box
	 */
	static bool IsPointInRegion(const FVector& Point, const FVector& Centre, const FVector2D& HalfExtent);
};
