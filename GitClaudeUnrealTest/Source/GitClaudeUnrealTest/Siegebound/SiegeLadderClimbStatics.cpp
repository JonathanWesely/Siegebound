// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeLadderClimbStatics.h"

// ⛔ THERE IS DELIBERATELY NOTHING ELSE TO INCLUDE — the SiegeStuckStatics.cpp discipline, and
// here it is the feature's own acceptance criterion rather than a style note: FVector and FMath
// arrive complete through the header's CoreMinimal, and this translation unit names no engine
// type beyond them. An engine include appearing in this file means the climb rules stopped being
// pure, and that costs SiegeLadderClimbTest.cpp its 14 headless tests (CONTACT-§2).
//
// ⭐ TASK-776 — EVERY LINE BELOW THIS COMMENT WAS MOVED VERBATIM FROM SummonedUnit.cpp:129-302.
// ⛔ Not one expression, default, parameter name or comment changed. The provenance note and the
// two superseded comment clauses are recorded in the header's file-header block, ⛔ not by
// editing the text below — so this body stays diffable character-for-character against the
// shipped version (the TASK-779 gate reads it exactly that way).

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  THE LADDER CLIMB — the pure half (TASK-738; CONVENTIONS TOWER-§8.5)
//  ⛔ No UWorld, no AActor, no component, no clock, no allocation, no logging. Everything the
//  climb DECIDES lives here so it can be tested headlessly; everything it DOES lives on the
//  actor below. The FSiegeStuckStatics / HeightAdvantageMultiplier precedent.
// ═══════════════════════════════════════════════════════════════════════════════════════════

bool FSiegeLadderClimbStatics::CanBegin(const FSiegeLadderClimbState& State, bool bDead, bool bAIFrozen,
	bool bSpellFrozen, const FVector& FromWorld, const FVector& ToWorld)
{
	// TOWER-§8.4(B)'s four pinned refusal reasons, in the order the contract states them.
	if (bDead || bAIFrozen || bSpellFrozen || State.bActive)
	{
		return false;
	}

	// The math guard (NOT a fifth policy rule): a zero-length or non-finite line would make
	// ClimbDirection normalise nothing, and the unit would sit in MOVE_Flying steering at zero
	// until the watchdog fired. Refusing at the door is cheaper, and the caller learns now.
	if (FromWorld.ContainsNaN() || ToWorld.ContainsNaN())
	{
		return false;
	}

	// ⛔ SYMMETRIC IN THE TWO POINTS — a squared distance cannot express an ordering, which is
	// deliberate: the link is BothWays and a descent passes them the other way round.
	return (ToWorld - FromWorld).SizeSquared() >= FMath::Square(MinClimbLineUU);
}

bool FSiegeLadderClimbStatics::Begin(FSiegeLadderClimbState& State, bool bDead, bool bAIFrozen,
	bool bSpellFrozen, const FVector& FromWorld, const FVector& ToWorld, float ClimbSpeedUU,
	float CapsuleHalfHeightUU)
{
	// ⛔ THE "CHANGES NOTHING" HALF OF THE PINNED CONTRACT LIVES HERE, IN ONE PLACE. Every
	// write below is after the refusal, so a refused Begin cannot half-arm the state — which is
	// exactly the failure a caller could never see and a test could never catch downstream.
	if (!CanBegin(State, bDead, bAIFrozen, bSpellFrozen, FromWorld, ToWorld))
	{
		return false;
	}

	// ⭐⭐ SURFACE SPACE -> CAPSULE-CENTRE SPACE. The sockets sit on generated navmesh (the ground
	// and the deck SURFACE, TOWER-§8.3), but the thing that travels this line is the capsule's
	// CENTRE, which stands one half-height above whatever it is on. ⛔ Without this lift the unit
	// finishes with its FEET a half-height below the deck — buried in the slab — and depenetration
	// drops it back down the tower. The SAME lift at both ends, so the length, the direction and
	// the watchdog budget are all identical to the un-lifted line.
	const float HalfHeight = FMath::Max(CapsuleHalfHeightUU, 0.f);
	const FVector Lift(0.f, 0.f, HalfHeight);

	State.bActive = true;
	State.Start = FromWorld + Lift;
	State.End = ToWorld + Lift;
	State.CapsuleHalfHeightUU = HalfHeight;
	State.LengthUU = static_cast<float>((State.End - State.Start).Size());
	State.ElapsedSeconds = 0.f;

	// The watchdog budget is fixed HERE, from the length and the rate at the moment of arming —
	// so a mid-climb retune of LadderClimbSpeedUU cannot extend a climb that is already running.
	State.TimeoutSeconds = FMath::Max(MinTimeoutSeconds,
		TimeoutScale * State.LengthUU / FMath::Max(ClimbSpeedUU, MinClimbSpeedUU));

	// ⭐ WHICH END IS THE DECK — resolved by Z, ⛔ NEVER by argument order. On an ascent that is
	// End; on a descent it is Start. A tie (a level line) reads as End, which is arbitrary and
	// harmless because a level line has no slab to pass through.
	State.bDeckIsAtEnd = (State.End.Z >= State.Start.Z);

	// ⚠️ THE DECK-BREACH WINDOW (TASK-737's measurement — see DeckBreachCapsuleHalfHeights).
	// Converted from Z into LINE length using this line's OWN slope, so it is exact for whatever
	// geometry ships rather than resting on a hardcoded sin(76°).
	const float RiseUU = FMath::Abs(static_cast<float>(State.End.Z - State.Start.Z));
	if (RiseUU > UE_KINDA_SMALL_NUMBER && HalfHeight > 0.f)
	{
		const float BreachZUU = DeckBreachCapsuleHalfHeights * HalfHeight;
		State.DeckBreachUU = FMath::Min(BreachZUU * (State.LengthUU / RiseUU), State.LengthUU);
	}
	else
	{
		// A level line passes through no slab, and a zero half-height means no capsule to bury.
		State.DeckBreachUU = 0.f;
	}

	return true;
}

FVector FSiegeLadderClimbStatics::ClimbDirection(const FSiegeLadderClimbState& State)
{
	// GetSafeNormal returns ZeroVector rather than NaN for a degenerate line. CanBegin already
	// refuses those, so this is belt — but a direction of NaN would corrupt the movement
	// component, and that is not a failure worth being clever about.
	return (State.End - State.Start).GetSafeNormal();
}

FVector FSiegeLadderClimbStatics::ArrivalTarget(const FSiegeLadderClimbState& State)
{
	// Already in capsule-centre space, so this IS the position the unit finishes at: the
	// destination surface plus one capsule half-height, i.e. standing ON the deck.
	return State.End;
}

bool FSiegeLadderClimbStatics::ShouldSweep(const FSiegeLadderClimbState& State, const FVector& CurrentWorld)
{
	if (!State.bActive || State.DeckBreachUU <= 0.f)
	{
		return true;
	}

	// Distance from the ELEVATED end, measured along the line. ⛔ Deliberately not "distance from
	// End": on a DESCENT the deck is at Start, and the capsule has to leave the slab it is
	// standing on before a sweep can do anything but jam it in place on the first frame.
	const FVector DeckEnd = State.bDeckIsAtEnd ? State.End : State.Start;
	const float DistanceFromDeck = static_cast<float>(FVector::Dist(CurrentWorld, DeckEnd));

	return DistanceFromDeck > State.DeckBreachUU;
}

bool FSiegeLadderClimbStatics::Advance(FSiegeLadderClimbState& State, const FVector& CurrentWorld,
	float DeltaSeconds, bool& bOutReachedTop, bool& bOutTimedOut)
{
	bOutReachedTop = false;
	bOutTimedOut = false;

	if (!State.bActive)
	{
		return false;
	}

	// A negative delta is never charged to the budget (the ConsumeStuckDeltaSeconds discipline:
	// a clock that can run backwards must not be able to expire a watchdog).
	State.ElapsedSeconds += FMath::Max(DeltaSeconds, 0.f);

	const FVector ToTop = State.End - CurrentWorld;
	const FVector Direction = ClimbDirection(State);

	// TWO arrival tests, ORed, and both are needed:
	//   (a) the remaining distance ALONG the climb axis has run out — this is the one that fires
	//       at a low frame rate, where a single step can carry the capsule clean past the top;
	//   (b) the capsule centre is inside ArrivalToleranceUU of the top — the ordinary case.
	// ⚠️ ARRIVAL IS TESTED BEFORE THE WATCHDOG on purpose: a unit that reaches the deck on the
	// very frame its budget expires has ARRIVED, and reporting that as a failed climb would drop
	// it off a tower it had already reached.
	const bool bPassedTheTop = FVector::DotProduct(ToTop, Direction) <= 0.f;
	const bool bWithinTolerance = ToTop.SizeSquared() <= FMath::Square(ArrivalToleranceUU);
	if (bPassedTheTop || bWithinTolerance)
	{
		bOutReachedTop = true;
		return false;
	}

	if (State.ElapsedSeconds >= State.TimeoutSeconds)
	{
		bOutTimedOut = true;
		return false;
	}

	return true;
}

bool FSiegeLadderClimbStatics::End(FSiegeLadderClimbState& State)
{
	// ⭐⭐ THE EXACTLY-ONCE LATCH. Read-and-clear in one place, so "restore the movement mode and
	// broadcast exactly once" is a property of ONE branch rather than a promise repeated at
	// eight call sites. A second call — death then EndPlay, or a delegate listener that calls
	// AbortLadderClimb from inside the broadcast — returns false and does nothing.
	if (!State.bActive)
	{
		return false;
	}

	// WHOLE-STRUCT reset, never a bare bActive = false: a stale Top or a stale elapsed clock
	// left behind is exactly the residue the TASK-020 zero-drift contract exists to forbid.
	State = FSiegeLadderClimbState();
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ THIS ONE FUNCTION IS **TASK-803** (CONTACT-§14), ⛔ NOT TASK-776'S MOVE AND ⛔ NOT
//  TASK-777'S TRIGGER. THE CROSS-TRACK TERM — the fix for the MERGED defect.
// ═══════════════════════════════════════════════════════════════════════════════════════════
//
// ⛔⛔ ATTRIBUTION, WRITTEN INTO THE SOURCE FOR THE THIRD TIME AND FOR THE SAME REASON: everything
// ABOVE this banner is TASK-776's byte-identical lift out of SummonedUnit.cpp:129-302 and is
// ⛔ UNTOUCHED by TASK-803 — ⛔ not one expression, default, parameter name or comment of it
// changed, `ClimbDirection` INCLUDED and `ClimbDirection` MOST OF ALL. The new function is
// APPENDED between the two existing regions precisely so BOTH stay contiguous and diffable.
//
// ⛔ AND THE PURITY BAR IS UNCHANGED BY THIS ADDITION: still ⛔ no include beyond the header,
// still FVector and FMath and nothing else — which is what lets the convergence claim below be
// asserted headlessly, in a suite with ⛔ not one SpawnActor.

FVector FSiegeLadderClimbStatics::SteerDirection(const FSiegeLadderClimbState& State, const FVector& CurrentWorld)
{
	// ⛔⛔ `ClimbDirection` IS **READ** HERE, ⛔ NEVER REDEFINED (CONTACT-§14.5). This function is
	// the only NEW reader of it; its three existing readers keep today's meaning exactly.
	const FVector Along = ClimbDirection(State);

	// ⛔ A degenerate line, or a non-finite position, yields the SAME ZeroVector ClimbDirection
	// yields — the identical refusal and the identical value, so a call site that swapped one for
	// the other cannot acquire a new failure mode at the door. CanBegin already refuses degenerate
	// lines, so the first term is belt; the second is NOT — GetSafeNormal propagates a NaN input
	// straight through (its size test is `NaN < tolerance`, which is false), and a NaN steer handed
	// to the movement component corrupts it.
	if (Along.IsZero() || CurrentWorld.ContainsNaN())
	{
		return Along;
	}

	// WHERE THE PAWN ACTUALLY IS, measured ALONG the line from Start. ⭐ THE CLOSED LOOP: the real
	// position is re-read every frame, so a sweep, a depenetration or a MaxFlySpeed clamp cannot
	// desync this from the world the way a dead-reckoned entry offset would.
	const double AlongUU = FVector::DotProduct(CurrentWorld - State.Start, Along);

	// THE AIM POINT — ⛔ ON the line, and ⛔ ON the segment. Clamped at BOTH ends deliberately:
	//   · at LengthUU, so the aim never runs off the far end (and, at the very top, becomes `End`
	//     itself — which is exactly where the arrival snap is going, so the last stretch of
	//     convergence is aimed at the same point the teardown will use);
	//   · at 0, so a pawn admitted BELOW Start is never aimed at a point on the line's BACKWARD
	//     extension, which would drive it down and away from the ladder it just asked to climb.
	const double AimUU = FMath::Clamp(AlongUU + static_cast<double>(SteerLookAheadUU),
		0.0, static_cast<double>(State.LengthUU));
	const FVector Aim = State.Start + Along * AimUU;

	const FVector Steer = (Aim - CurrentWorld).GetSafeNormal();

	// ⭐ ON the line the aim point sits exactly SteerLookAheadUU ahead ALONG it, so this returns
	// `Along` and a centred climb is unchanged — the regression guarantee, not an accident.
	// ⛔ The fallback covers the one degenerate case left: a pawn standing exactly ON its own aim
	// point, reachable only when the clamp collapsed at an endpoint. There is no direction to take
	// there, and the line's own is the right answer.
	return Steer.IsZero() ? Along : Steer;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ EVERYTHING BELOW THIS BANNER IS **TASK-777** (CONTACT-§4.1), ⛔ NOT TASK-776'S MOVE.
//  THE CONTACT TRIGGER — the pure half: PROXIMITY + INTENT + DWELL
// ═══════════════════════════════════════════════════════════════════════════════════════════
//
// ⛔⛔ ATTRIBUTION, WRITTEN INTO THE SOURCE SO A GATE CANNOT FAIL THE WRONG TASK: everything
// ABOVE this banner is TASK-776's byte-identical lift out of SummonedUnit.cpp:129-302 and is
// ⛔ UNTOUCHED by TASK-777 — the moved region stays contiguous from the top of the file and
// diffs character-for-character against the shipped version, which is exactly what TASK-779
// item (2) reads it for.
//
// ⛔ AND THE PURITY BAR IS UNCHANGED BY THIS ADDITION: still ⛔ no include beyond the header,
// still FVector and FMath and nothing else. An engine include appearing in this file means the
// contact trigger stopped being testable headlessly, which is the whole reason it lives here.

bool FSiegeLadderContactStatics::IsAtTopEndpoint(const FVector& PawnLocation, const FVector& FootWorld, const FVector& TopWorld)
{
	// ⛔ Z, ⛔ NEVER 2D DISTANCE — the endpoints are 1,200 uu apart in Z and only 300 uu apart
	// in XY, so their two XY discs OVERLAP and a pawn on the ground can stand inside the TOP's
	// disc. A STRICT `<` makes a tie read as the FOOT, i.e. an ascent — the same tie-break
	// HandleLadderLinkReached applies to the link's destination point.
	//
	// ⚠️⚠️ REPAIRED 2026-09-02 (TASK-787 item (6), raised by TASK-786): this comment used to say
	// the discs are "TANGENT at a 150 uu radius". That was true only of the radius the feature
	// SHIPPED WITH FOR ONE DAY. At `CONTACT-§7b`'s K-6.1 radius of 350 they overlap in a
	// 2R − d = 400 uu LENS, and each endpoint's XY centre sits R − d = 50 uu INSIDE the other's
	// disc. ⇒ the Z resolution is load-bearing at EVERY approach, ⛔ not just at one tangent
	// point — ⛔ do not weaken it.
	// ⭐ AND IT SURVIVES THE RADIUS BY CONSTRUCTION, WHICH IS THE POINT WORTH KEEPING: this
	// function takes ⛔ NO radius term at all, so ⛔ no retune of `LadderContactRadiusUU` can
	// break it. ⚠️ A change to `PlatformHeightUU` — the 1,200 — is the one that could.
	return FMath::Abs(PawnLocation.Z - TopWorld.Z) < FMath::Abs(PawnLocation.Z - FootWorld.Z);
}

ESiegeLadderContactVerdict FSiegeLadderContactStatics::WantsToClimb(
	FSiegeLadderContactState& State, const FVector& PawnLocation, const FVector& PawnVelocity,
	const FVector& FootWorld, const FVector& TopWorld,
	float RadiusUU, float IntentCos, float RequiredDwellSeconds, float DeltaSeconds, bool& bOutAscending)
{
	// ── WHICH ENDPOINT (K-C: BOTH of them arm the trigger) ──────────────────────────────────
	const bool bAtTop = IsAtTopEndpoint(PawnLocation, FootWorld, TopWorld);
	const FVector& Endpoint = bAtTop ? TopWorld : FootWorld;

	// ⛔ WRITTEN ON EVERY PATH, INCLUDING EVERY REFUSAL. An out-parameter left untouched on a
	// refusal is an uninitialised read waiting for the first caller that logs it.
	bOutAscending = !bAtTop;

	// ── TERM 1: PROXIMITY, IN 2D ────────────────────────────────────────────────────────────
	// A negative radius is clamped rather than rejected: it is a mis-tuned EditDefaultsOnly
	// value, and the honest behaviour of a zero-radius trigger is "never fires", ⛔ not
	// "compares against a negative square and fires everywhere".
	const float SafeRadiusUU = FMath::Max(RadiusUU, 0.f);
	const bool bInRange = FVector::DistSquared2D(PawnLocation, Endpoint) <= (SafeRadiusUU * SafeRadiusUU);

	// ── TERM 2: INTENT — DIRECTION, ⛔ NEVER A KEY ──────────────────────────────────────────
	// GetSafeNormal2D returns the ZERO vector for a degenerate input, so a pawn standing still
	// and a pawn standing exactly ON the endpoint both yield a dot of 0 — below any sane
	// IntentCos — with ⛔ no special case to get wrong. MinContactSpeedUU is the separate,
	// derived floor that keeps braking residue and depenetration nudges out of the reading.
	const FVector ToEndpoint = (Endpoint - PawnLocation).GetSafeNormal2D();
	const FVector Heading = PawnVelocity.GetSafeNormal2D();
	const bool bHeadingIn =
		(PawnVelocity.Size2D() >= MinContactSpeedUU) &&
		(FVector::DotProduct(Heading, ToEndpoint) >= IntentCos);

	// ── K-C'S RE-ARM LATCH, TESTED **BEFORE** THE DWELL ─────────────────────────────────────
	// ⚠️⚠️ AND IT IS MAINTAINED ON **EVERY** SAMPLE, WHICH IS WHY THE THREE TERMS RUN BEFORE THE
	// OWNER'S ENTRY GATE RATHER THAN AFTER IT. If an occupancy or team refusal could short-
	// circuit this function, a pawn latched at the foot while SOMEBODY ELSE climbed would never
	// be observed leaving the radius, and its latch would survive forever — a pawn permanently
	// unable to use a ladder, for a reason nothing logs.
	if (State.bDisarmed)
	{
		if (State.bDisarmedAtTop != bAtTop || !bInRange || !bHeadingIn)
		{
			// ✅ It changed ends, left the radius, or steered out of the cone — exactly K-C's
			// re-arm conditions. The latch drops and this sample is judged on its merits below.
			State.bDisarmed = false;
			State.bDisarmedAtTop = false;
		}
		else
		{
			// ⛔ Still standing in it, still pressing into it, having just finished a climb
			// there. This is the yo-yo, and refusing it is the whole point of the latch.
			State.DwellSeconds = 0.f;
			return ESiegeLadderContactVerdict::Disarmed;
		}
	}

	if (!bInRange)
	{
		State.DwellSeconds = 0.f;
		return ESiegeLadderContactVerdict::TooFar;
	}

	if (!bHeadingIn)
	{
		State.DwellSeconds = 0.f;
		return ESiegeLadderContactVerdict::NotHeadingIn;
	}

	// ── TERM 3: DWELL — CONTINUOUS, AND AT **ONE** ENDPOINT ─────────────────────────────────
	// Swapping ends restarts the clock: 0.2 s of pressing into the foot plus 0.2 s of pressing
	// into the deck is ⛔ not 0.4 s of intent about either of them.
	if (State.bDwellAtTop != bAtTop)
	{
		State.DwellSeconds = 0.f;
		State.bDwellAtTop = bAtTop;
	}

	// A negative DeltaSeconds cannot un-dwell: clamping keeps the accumulator monotonic under a
	// caller that hands us a bad clock.
	State.DwellSeconds += FMath::Max(DeltaSeconds, 0.f);

	if (State.DwellSeconds < RequiredDwellSeconds)
	{
		return ESiegeLadderContactVerdict::Dwelling;
	}

	// ⛔ THE DWELL IS **NOT** CLEARED HERE — see the header. It stays satisfied while the terms
	// hold, so a pawn the caller refuses (a busy ladder) enters the moment the ladder frees.
	// Re-entry after a climb is prevented by the latch, ⛔ never by a consumed dwell.
	return ESiegeLadderContactVerdict::Climb;
}

void FSiegeLadderContactStatics::Disarm(FSiegeLadderContactState& State, bool bAtTop)
{
	// ⛔ The dwell is cleared as well as the latch set: a pawn that lands still pressing into
	// the ladder must start its next dwell from zero even after the latch drops, or the 0.35 s
	// it accumulated on the way UP would pay for the trip back DOWN.
	State.DwellSeconds = 0.f;
	State.bDwellAtTop = bAtTop;
	State.bDisarmed = true;
	State.bDisarmedAtTop = bAtTop;
}
