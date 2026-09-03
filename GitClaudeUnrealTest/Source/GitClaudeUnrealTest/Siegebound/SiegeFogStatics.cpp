// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeFogStatics.h"

// ⛔ THERE IS DELIBERATELY NOTHING ELSE TO INCLUDE. FMath arrives complete through the header's
// CoreMinimal, and this translation unit names no engine type beyond it: no UWorld, no AActor,
// no ITeamAgent, no AController, no UGameplayStatics. An engine include appearing in this file
// means the fog rule stopped being pure, and that costs Tests/SiegeFogTest.cpp its headless
// run and costs the header's M8 declaration its proof (FOG-§6, and spec (3) of TASK-837).
// ⛔ In particular: this file must never learn what a TEAM is. Fog is symmetric (J-F3) and the
// enforcement is that the concept is absent, not that a branch is written carefully.

float FSiegeFogStatics::FogDensityAt(float DistanceUU, const FSiegeFogTuning& Tuning)
{
	const float Onset   = Tuning.FogVisionOnsetUU;
	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ── DEGENERATE INPUTS FAIL TOWARD *CLEAR* ────────────────────────────────────────────
	// A NaN distance or a broken tuning yields to the shipped game rather than fogging it.
	// ⛔ The alternative — returning 1.0 on garbage — would render an opaque screen from a
	// single bad float, which is the failure nobody would be able to diagnose.
	if (!FMath::IsFinite(DistanceUU) || !FMath::IsFinite(Onset) || !FMath::IsFinite(Ceiling))
	{
		return 0.f;
	}

	// ── THE ZERO-WIDTH / INVERTED BAND, HANDLED BEFORE ANYTHING CAN DIVIDE BY IT ─────────
	// onset == ceiling is the continuous limit of the ramp as its width goes to zero, and that
	// limit is a HARD STEP at the ceiling. An inverted band (someone swaps the two defaults)
	// lands on the same answer rather than on a negative t. ⛔ This branch is why the division
	// below cannot divide by zero — it is not defence in depth, it is the guard.
	if (Ceiling <= Onset)
	{
		return (DistanceUU >= Ceiling) ? 1.f : 0.f;
	}

	// ── THE CLEAR BUBBLE: at and inside the onset the world is untouched ─────────────────
	// EXACTLY 0, not near-zero: everything closer than 10 ft must render bit-identically to the
	// unfogged game, and the melee band (120 uu) lives entirely in here.
	if (DistanceUU <= Onset)
	{
		return 0.f;
	}

	// ── THE HARD CUT: at and beyond the ceiling it is EXACTLY opaque ─────────────────────
	// "they really cannot see anything beyond 20 feet" — an asymptote would leave a few percent
	// of visibility here forever, and at Longbowman range a few percent is still a lethal shot.
	if (DistanceUU >= Ceiling)
	{
		return 1.f;
	}

	// ── THE RAMP: onset < d < ceiling, so t is strictly inside (0, 1) ────────────────────
	const float T = (DistanceUU - Onset) / (Ceiling - Onset);

	// The quadratic ease-in (see FSiegeFogTuning::FogDensityExponent for the full ruling).
	// ⛔ A non-positive or non-finite exponent falls back to LINEAR rather than to Pow(t, 0),
	// which is 1 everywhere and would paint a wall of fog at the onset. The `!(X > 0.f)` shape
	// is deliberate: it catches NaN, which `X <= 0.f` does not.
	const float RawExponent = Tuning.FogDensityExponent;
	const float Exponent = (FMath::IsFinite(RawExponent) && RawExponent > 0.f) ? RawExponent : 1.f;

	// Clamp is belt-and-braces — t is in (0, 1) and the exponent is positive, so Pow is too.
	return FMath::Clamp(FMath::Pow(T, Exponent), 0.f, 1.f);
}

float FSiegeFogStatics::EffectiveVisionRadius(float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning)
{
	// ⛔⛔ THE INERT PATH, AND IT IS THE IMPORTANT ONE. With fog off this returns the caller's
	// own float back, untouched — no clamp, no min, no sanitising, not one ulp of drift. That
	// is what lets TASK-838 call this UNCONDITIONALLY at the funnel: with no fog in play the
	// game is byte-for-byte the game that shipped, so any regression is attributable to fog.
	if (!bFogActive)
	{
		return RequestedRadiusUU;
	}

	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ⛔ A BROKEN TUNING MUST NEVER BLIND THE ARMY. A non-finite, ZERO or negative ceiling, or a
	// non-finite requested radius, returns the request unchanged — i.e. degrades to "no fog"
	// rather than to "no vision". A negative ceiling under a plain min() would clamp every unit
	// in the game to a negative range and stop all combat everywhere.
	//
	// ⚖️⛔⛔ THE COMPARISON IS `<= 0.f` AND ⛔ NOT `< 0.f` — FOG-§7b half (b), ruled 2026-09-03,
	// shipped by TASK-838 because TASK-838 is the task that gives this function callers. With the
	// strict form, ZERO PASSED the guard and reached the min() below ⇒ min(Range, 0) == 0 for
	// EVERY acquisition in the game ⇒ ⛔ ALL COMBAT STOPS WHILE FOG IS UP. That is the exact
	// outcome the paragraph above says must never happen, and it sat on the tunable's OWN slider
	// stop (the ceiling's `meta = (ClampMin = "0")`, since raised to the onset's 304.8 — half (a)).
	// ⛔ Half (a) alone is NOT the fix: `ClampMin` constrains the editor spinner and nothing else,
	// so an `.ini`, a Blueprint default or a line of C++ can still write 0. THIS line is what
	// makes zero harmless. ⭐ And it is not a new rule — zero was simply the one member of the
	// degenerate class (non-finite, negative, Ceiling <= Onset) that a strict `<` let through.
	if (!FMath::IsFinite(Ceiling) || Ceiling <= 0.f || !FMath::IsFinite(RequestedRadiusUU))
	{
		return RequestedRadiusUU;
	}

	// ⛔ MIN, ⛔ NOT CLAMP. There is no floor and none may be invented: a Cleric's 400 stays 400
	// and a melee unit's 120 stays 120. Raising a short range up to the ceiling would be
	// adjusting Jonathan's ruling (J-F2) while appearing to honour it.
	return FMath::Min(RequestedRadiusUU, Ceiling);
}

bool FSiegeFogStatics::IsVisibleThroughFog(float DistanceUU, float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning)
{
	// ⛔ A garbage DISTANCE is a broken target, not a broken tuning — so this is the one place
	// that does NOT fail toward "no fog". Acquiring a NaN-distance target would push the NaN
	// straight into a move order. Guarded here rather than left to `NaN <= X`, which is false
	// by IEEE but only by accident of the comparison's direction.
	if (!FMath::IsFinite(DistanceUU))
	{
		return false;
	}

	// ⭐ ONE definition, and it is the project's own shipped comparison with a substituted
	// operand: `Distance <= Range` becomes `Distance <= EffectiveVisionRadius(Range, ...)`.
	// ⛔ The `<=` is measured from SummonedUnit.cpp:1644/:1785/:1851/:1956/:2409, not chosen.
	return DistanceUU <= EffectiveVisionRadius(RequestedRadiusUU, bFogActive, Tuning);
}
