// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeFogStatics.h"

// ⛔ THERE IS DELIBERATELY NOTHING ELSE TO INCLUDE. FMath arrives complete through the header's
// CoreMinimal, and this translation unit names no engine type beyond it: no UWorld, no AActor,
// no ITeamAgent, no AController, no UGameplayStatics. An engine include appearing in this file
// means the fog rule stopped being pure, and that costs Tests/SiegeFogTest.cpp its headless
// run and costs the header's M8 declaration its proof (FOG-§6, and spec (3) of TASK-837).
// ⛔ In particular: this file must never learn what a TEAM is. Fog is symmetric (J-F3) and the
// enforcement is that the concept is absent, not that a branch is written carefully.

// ⛔⛔⛔ THE VISUAL'S CURVE ONLY. ⛔ NOTHING MECHANICAL MAY CALL THIS — see the header's banner
// and FOG-§9.2. It is ASYMPTOTIC, so "is it opaque here?" has no true answer; the mechanical
// question belongs to EffectiveVisionRadius / IsVisibleThroughFog, which are a HARD CUT.
//
// ⭐ REWRITTEN 2026-09-04 (TASK-981) from a quadratic ease-in to BEER-LAMBERT EXTINCTION. ⛔ The
// old curve was SUPERSEDED, not bugged: it returned EXACTLY 0 at the onset, and Jonathan then
// ruled ~86% obscured there, on purpose, after being shown the arithmetic.
float FSiegeFogStatics::FogDensityAt(float DistanceUU, const FSiegeFogTuning& Tuning)
{
	const float Ceiling       = Tuning.FogVisionCeilingUU;
	const float Transmittance = Tuning.FogTransmittanceAtCeiling;

	// ⛔ NOTE WHAT IS ABSENT: `FogVisionOnsetUU` IS NOT READ. Under Beer-Lambert there is no knee
	// and no clear bubble, so the onset has no part in the picture any more. It keeps its value
	// and its two other jobs (the ceiling's ClampMin floor, and the reporting point) — FOG-§9.2.

	// ── DEGENERATE INPUTS FAIL TOWARD *CLEAR* ────────────────────────────────────────────
	// A NaN distance or a broken tuning yields to the shipped game rather than fogging it.
	// ⛔ The alternative — returning 1.0 on garbage — would render an opaque screen from a
	// single bad float, which is the failure nobody would be able to diagnose.
	if (!FMath::IsFinite(DistanceUU) || !FMath::IsFinite(Ceiling) || !FMath::IsFinite(Transmittance))
	{
		return 0.f;
	}

	// ── ⛔⛔ THE CEILING IS σ's DENOMINATOR NOW, SO ZERO IS A DIVIDE BY ZERO ──────────────
	// ⚠️ THIS GUARD IS NEW AND IT IS NOT DEFENCE IN DEPTH — IT IS THE GUARD. The retired ramp
	// divided by `(ceiling − onset)` and was protected by the `Ceiling <= Onset` branch, which
	// no longer exists because the band no longer exists. `!(X > 0.f)` rather than `X <= 0.f`
	// is deliberate: it also catches NaN, though the finite check above already has.
	// ⭐ Same shape and same reasoning as EffectiveVisionRadius' `Ceiling <= 0.f` (FOG-§7b half
	// (b)): a broken tuning degrades to NO FOG, ⛔ never to a whiteout the player cannot see past.
	if (!(Ceiling > 0.f))
	{
		return 0.f;
	}

	// ── THE TRANSMITTANCE MUST BE A REAL RATIO, STRICTLY INSIDE (0, 1) ───────────────────
	// ⛔ At `<= 0` the log diverges and σ is INFINITE — an opaque screen everywhere, including at
	// the camera. ⛔ At `>= 1` the log is zero or positive, i.e. no fog (or a NEGATIVE σ, which
	// would make the world get CLEARER with distance). Both degenerate to CLEAR, which is this
	// module's totality law: fail toward no fog, never toward no vision.
	if (!(Transmittance > 0.f) || !(Transmittance < 1.f))
	{
		return 0.f;
	}

	// ── AT AND BEHIND THE CAMERA THE WORLD IS EXACTLY CLEAR ─────────────────────────────
	// EXACTLY 0, and it is the only exact value the new curve has. ⛔ It is also the guard that
	// stops a NEGATIVE distance producing a NEGATIVE density: `1 − exp(+x)` is unbounded below,
	// and for a large negative distance `exp` overflows to +inf.
	if (!(DistanceUU > 0.f))
	{
		return 0.f;
	}

	// ── BEER-LAMBERT: σ IS ⛔ DERIVED FROM THE TUNABLES, ⛔ NEVER TYPED ───────────────────
	//
	//     σ = −ln(FogTransmittanceAtCeiling) / FogVisionCeilingUU
	//       = −ln(0.02) / 609.6 = 3.9120230 / 609.6 = 0.0064174 per uu
	//
	// ⭐ Deriving it is what makes BOTH tunables live: retuning either moves the whole curve, and
	// the suite asserts exactly that, so a hardcoded σ goes RED. Transmittance is strictly inside
	// (0, 1) by the guard above ⇒ Loge is strictly negative ⇒ σ is strictly positive.
	const float Sigma = -FMath::Loge(Transmittance) / Ceiling;

	// The obscuration — what the player cannot see through. ⛔ Asymptotic: it approaches 1 and
	// never arrives, which is precisely why nothing mechanical may consult it. Clamp is
	// belt-and-braces (Sigma > 0 and DistanceUU > 0 already put Exp in (0, 1)); it also absorbs
	// the underflow-to-zero case at very large distances, where the result is exactly 1.
	return FMath::Clamp(1.f - FMath::Exp(-Sigma * DistanceUU), 0.f, 1.f);
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
