// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Math/UnrealMathUtility.h"
#include "Siegebound/SiegeFogStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the FOG CARD's pure rules (TASK-837, FOG-§1 / FOG-§6) ═══
 *
 *  Jonathan, verbatim (2026-09-03): "The fog makes it to where players can really only see up
 *  until about 20 feet in front of you. … I want the player to begin to see a light amount of
 *  fog starting at about 10 feet away and it gets thicker and thicker until they really cannot
 *  see anything beyond 20 feet away."
 *
 *  Subject: `FSiegeFogStatics` + `FSiegeFogTuning`. QA gate: the TASK-837 report.
 *  Compile + suite run: deferred (QUIET-MODULE — three compiles already queued).
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE TRAP THIS FILE IS POINTED AT: **"20 FEET" IS NOT `20` UNREAL UNITS**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Unreal is CENTIMETRES (1 uu = 1 cm) and 1 ft = 30.48 cm exactly, so the ceiling is
 *  20 × 30.48 = 609.6 uu and the onset is 10 × 30.48 = 304.8 uu. A `20` there is wrong by 30×,
 *  it would put the ceiling INSIDE the unit's own 192-uu capsule, and it would read as entirely
 *  plausible in review — which is precisely why it gets a test rather than a promise.
 *
 *  ⛔⛔ AND THAT IS WHY NEITHER `304.8` NOR `609.6` APPEARS ANYWHERE IN THIS FILE AS A CODE
 *  LITERAL (only in prose, where it explains a derivation rather than standing in for it).
 *  Every expectation below is built from `Feet × CentimetresPerFoot`, i.e. RE-DERIVED from the
 *  international definition of the foot and from the numbers in Jonathan's own sentence —
 *  ⛔ never transcribed out of the header it is asserting. ⚖️ A test copied from its subject
 *  agrees with its subject by construction and reports SAFE forever (SHIP-§9c); this one
 *  disagrees with a wrong header the moment the header is wrong. FOG-§1 also forbids a second
 *  shipped literal of either number, and this file honours that from the outside: there is
 *  exactly ONE `609.6` and ONE `304.8` in the codebase, and both are in FSiegeFogTuning.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHY EVERY ASSERTION HERE CAN ACTUALLY FAIL — the standing worry, answered concretely
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⚠️ A falloff test passes TRIVIALLY if every sample it takes happens to sit inside the clear
 *  onset — the function returns 0, the sweep is monotonic, and the suite reports green over a
 *  curve nobody exercised. ⛔ That failure mode is defused explicitly, not hoped away:
 *    • test 2 asserts the two BOUNDARIES and four distances BEYOND the ceiling;
 *    • test 3 samples three points strictly BETWEEN them and asserts values that a LINEAR ramp
 *      (and a smoothstep, and a Beer-Lambert curve) all FAIL — the between-rows discriminate
 *      the shipped shape from its plausible alternatives, they do not merely observe it;
 *    • test 4's sweep COUNTS its samples by category and asserts that all three categories are
 *      non-empty, so a sweep that never left the clear bubble fails the sweep test itself.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — stated so nobody mistakes green for done
 *  (SC-§32: a mechanism never observed to function is not known to function)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔ **NOTHING CONSULTS THIS FILE AT RUNTIME YET.** TASK-838 applies the ceiling inside
 *      `FSiegeCombatStatics::GatherHostileAgents`; until it lands, fog does not exist in the
 *      running game and not one unit's range changes. These tests prove the ARITHMETIC, ⛔ not
 *      the feature.
 *    • ⛔ **THE SYMMETRY (J-F3, "and that includes AI and all units") IS NOT ASSERTED HERE, AND
 *      DELIBERATELY SO.** It is enforced STRUCTURALLY: no function in `FSiegeFogStatics` takes
 *      a team, a viewer, a controller or an actor, so an asymmetric fog is unrepresentable —
 *      you cannot write the favouritism because there is no parameter to branch on. ⭐ A runtime
 *      assertion that "both teams get the same answer" would be trivially true and would report
 *      SAFE forever; the type system is the stronger instrument and it is used instead.
 *    • ⛔ **THE VISUAL.** `FogDensityAt` is the design curve; TASK-841 maps it onto the vendor
 *      pack. "As realistic as possible" is judged on pixels, ⛔ never here.
 *    • ⛔ **THE DURATION, THE REFRESH AND THE CARD** are TASK-839 / TASK-840's.
 *    • ⛔ NOTHING HERE RUNS THE MODEL. No inference, no Capture(), no EnsureSnapshot(). 🔒 The
 *      one-shot latch is untouched and unspent by this file, and ⛔ no token figure appears in
 *      it (AS-§12g).
 */

namespace SiegeFogTestFixture
{
	/**
	 *  ⭐ THE CONVERSION, RE-DERIVED FROM ITS DEFINITION RATHER THAN COPIED FROM THE HEADER.
	 *  `1 ft = 30.48 cm` is the international definition of the foot (exact, since 1959); `10`
	 *  and `20` are the numbers in Jonathan's own sentence. Their products are what the code
	 *  must ship. ⛔ Deliberately NOT written as `304.8f` / `609.6f`: an expectation transcribed
	 *  from its subject cannot disagree with a wrong subject.
	 */
	constexpr float CentimetresPerFoot = 30.48f;
	constexpr float OnsetFeet          = 10.f;
	constexpr float CeilingFeet        = 20.f;
	constexpr float DerivedOnsetUU     = OnsetFeet   * CentimetresPerFoot; // = 304.8 uu
	constexpr float DerivedCeilingUU   = CeilingFeet * CentimetresPerFoot; // = 609.6 uu

	/**
	 *  Tolerance for the decimal expectations. Tight enough that every wrong value in the trap
	 *  list blows straight through it — 600 misses by 9.6 (96,000× this), 610 by 0.4 (4,000×),
	 *  609 by 0.6 (6,000×), and 20 by 589.6 — and loose enough that float32 rounding never does.
	 *
	 *  ⚠️⛔ THE ROUNDING FIGURE, CORRECTED BY TASK-838 (qa/TASK-847.md NIT-3). This comment used
	 *  to say the gap was "~1.5e-5". ⛔ It is understated ~4×. Measured in binary32, one ULP is
	 *  2^(exponent-23), so:
	 *      onset   304.8  (exponent 8) ⇒ 1 ULP = 2^-15 = 3.05e-05
	 *      ceiling 609.6  (exponent 9) ⇒ 1 ULP = 2^-14 = 6.10e-05
	 *  ⇒ the CEILING row clears this tolerance with only ⛔ 1.6× headroom (1e-4 / 6.10e-05).
	 *
	 *  ⛔⛔ DO NOT TIGHTEN THIS BELOW `1e-4`. Anyone who "tightens" it turns a ⛔ CORRECT build
	 *  RED: `20.f * 30.48f` and `609.6f` are different bit patterns, and the derived expectation
	 *  is the whole point of this fixture (an expectation transcribed from its subject cannot
	 *  disagree with a wrong subject). A tighter number would be measuring the FPU, not the fog.
	 */
	constexpr float Tolerance = 1.e-4f;

	/** Exact-equality tolerance, for the claims whose whole content is the word EXACTLY. */
	constexpr float Exact = 0.f;

	/**
	 *  The shipped defaults, untouched — a default-constructed tuning IS the shipped tuning, so
	 *  every test below is asserting the values the game will actually run with.
	 */
	static FSiegeFogTuning ShippedTuning()
	{
		return FSiegeFogTuning();
	}

	/**
	 *  A NaN and an infinity built from their IEEE-754 bit patterns rather than by arithmetic.
	 *  ⛔ `FMath::Sqrt(-1.f)` and `0.f / 0.f` are the obvious spellings and both are the wrong
	 *  choice here: a compiler permitted to assume finite math may fold them away, which would
	 *  quietly turn test 9 into a test of two ordinary floats.
	 */
	static float MakeNaN()
	{
		const uint32 Bits = 0x7FC00000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &Bits, sizeof(Result));
		return Result;
	}

	static float MakeInfinity()
	{
		const uint32 Bits = 0x7F800000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &Bits, sizeof(Result));
		return Result;
	}

	/** The shipped ranges this feature is measured against, read from Docs/Data/cards.csv. */
	constexpr float LongbowmanRange    = 3600.f;
	constexpr float ArcherRange        = 2100.f; // == the Wizard's
	constexpr float BallistaTowerRange = 1400.f;
	constexpr float ArrowTowerRange    =  900.f;
	constexpr float BombTowerRange     =  800.f; // == the Crystal Tower's
	constexpr float ClericRange        =  400.f;
	constexpr float MeleeRange         =  120.f;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. ⭐⛔⛔ THE CONVERSION — 30.48 cm/ft, and NOT feet-as-units (FOG-§1)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogConversionTest,
	"Siegebound.Fog.TheConversionIsThirtyPointFourEightPerFootAndNotFeetAsUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogConversionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// ── (a) The two shipped defaults ARE his feet, converted ─────────────────────────────
	TestEqual(TEXT("(a) ⭐ The ONSET default is 10 ft × 30.48 cm/ft (= 304.8 uu) — his \"about 10 feet\", converted"),
		Tuning.FogVisionOnsetUU, DerivedOnsetUU, Tolerance);

	TestEqual(TEXT("(a) ⭐ The CEILING default is 20 ft × 30.48 cm/ft (= 609.6 uu) — his \"about 20 feet\", converted"),
		Tuning.FogVisionCeilingUU, DerivedCeilingUU, Tolerance);

	// ── (b) ⛔⛔ THE 30× TRAP — feet-as-units, asserted as an explicit REFUSAL ────────────
	// A `20` here would put the ceiling inside the unit's own 192-uu capsule and every ranged
	// unit in the game would be unable to fire at all. This row fails loudly against it.
	TestTrue(TEXT("(b) ⛔⛔ The ceiling is NOT the bare number 20 — feet-as-units is wrong by 30× and would sit INSIDE the 192-uu hero capsule"),
		Tuning.FogVisionCeilingUU > CeilingFeet * 2.f);

	TestTrue(TEXT("(b) ⛔⛔ The onset is NOT the bare number 10 — same 30× trap at the other end"),
		Tuning.FogVisionOnsetUU > OnsetFeet * 2.f);

	// ── (c) ⛔ THE ROUNDING TRAP — 600 and 300 are "close enough" and are NOT the number ──
	// FOG-§1 names these explicitly. Each miss is thousands of times the tolerance, so these
	// rows cannot pass by float drift; they only pass on the exact conversion.
	TestTrue(TEXT("(c) ⛔ The ceiling is NOT the round 600 — a designer's round number is not the conversion"),
		!FMath::IsNearlyEqual(Tuning.FogVisionCeilingUU, 600.f, Tolerance));

	TestTrue(TEXT("(c) ⛔ The ceiling is NOT the truncated 609 — the .6 is load-bearing and must survive"),
		!FMath::IsNearlyEqual(Tuning.FogVisionCeilingUU, 609.f, Tolerance));

	TestTrue(TEXT("(c) ⛔ The onset is NOT the round 300"),
		!FMath::IsNearlyEqual(Tuning.FogVisionOnsetUU, 300.f, Tolerance));

	// ── (d) The band is ordered and non-degenerate — the ramp has somewhere to live ──────
	// Fails against a header whose two defaults were swapped, which is the single edit most
	// likely to survive a careless review (both numbers are still "right", in the wrong slots).
	TestTrue(TEXT("(d) The onset is strictly INSIDE the ceiling — a swapped pair degenerates the ramp to a step"),
		Tuning.FogVisionOnsetUU < Tuning.FogVisionCeilingUU);

	// The ceiling is exactly twice the onset because 20 ft is exactly twice 10 ft. Derived from
	// his two numbers, so it fails if either default is retuned without the other.
	TestEqual(TEXT("(d) The ceiling is exactly twice the onset — 20 ft is twice 10 ft, and the pair must stay coherent"),
		Tuning.FogVisionCeilingUU, Tuning.FogVisionOnsetUU * 2.f, Tolerance);

	// ── (e) The shipped falloff shape is the CURVE that was ruled, not linear ────────────
	TestEqual(TEXT("(e) ⭐ The shipped exponent is 2 — the quadratic ease-in ruled for \"a LIGHT amount … thicker and thicker\""),
		Tuning.FogDensityExponent, 2.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⭐ THE TWO BOUNDARIES — exactly clear at the onset, exactly opaque at the
//     ceiling, and a HARD CUT (not an asymptote) everywhere beyond it
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogBoundariesTest,
	"Siegebound.Fog.DensityIsExactlyZeroAtTheOnsetAndExactlyOneAtAndBeyondTheCeiling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogBoundariesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float Onset   = Tuning.FogVisionOnsetUU;
	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ── (a) THE CLEAR BUBBLE is EXACTLY clear — 0, not near-0 ───────────────────────────
	// Everything closer than 10 ft must render bit-identically to the unfogged game. A curve
	// that leaked 0.001 of density at the camera would haze the entire melee band, which
	// FOG-§2 lists as UNAFFECTED.
	TestEqual(TEXT("(a) At the camera (0 uu) the world is EXACTLY clear"),
		FSiegeFogStatics::FogDensityAt(0.f, Tuning), 0.f, Exact);

	TestEqual(TEXT("(a) At melee range (120 uu — every melee unit in the game) EXACTLY clear"),
		FSiegeFogStatics::FogDensityAt(MeleeRange, Tuning), 0.f, Exact);

	TestEqual(TEXT("(a) ⭐ AT the onset itself the fog has not yet begun — EXACTLY 0, the boundary is closed on the clear side"),
		FSiegeFogStatics::FogDensityAt(Onset, Tuning), 0.f, Exact);

	// One unit PAST the onset is already non-zero: the ramp starts immediately, it does not have
	// a dead band. Fails against an implementation that floored or quantised the ramp.
	TestTrue(TEXT("(a) ⭐ ONE uu past the onset is already non-zero — the fog begins where he said, with no dead band"),
		FSiegeFogStatics::FogDensityAt(Onset + 1.f, Tuning) > 0.f);

	// …and it is still only a WHISPER there — his "LIGHT amount of fog starting at about 10 ft".
	// ⛔ This row FAILS against a linear ramp, which reads 0.0033 here against the quadratic's
	// 0.0000108. It is asserted as an upper bound so it also fails against anything steeper.
	//
	// ⚠️⛔ THE RATIO, CORRECTED BY TASK-838 (qa/TASK-847.md NIT-2). This comment used to say
	// linear was "~3×" the quadratic. ⛔ It is ⛔ ~305× — and the arithmetic is exact rather than
	// approximate, which is why the wrong figure was worth fixing in a project that treats its
	// comments as law: at d = onset + 1, t = 1/(ceiling − onset) = 1/304.8, so linear reads t and
	// quadratic reads t², and the ratio is 1/t = ⛔ the band width itself, 304.8. FOG-§7a states
	// the same fact as "300×". ⛔ The ASSERTION below was always correct and does kill linear
	// (0.0033 is 3.3× the 0.001 bound); ⛔ only the prose was wrong.
	TestTrue(TEXT("(a) ⭐ …and it is a WHISPER, not an edge — density one uu past the onset is under 0.001 (linear reads ~305× more)"),
		FSiegeFogStatics::FogDensityAt(Onset + 1.f, Tuning) < 0.001f);

	// ── (b) ⛔⛔ THE HARD CUT — EXACTLY 1.0 AT the ceiling ───────────────────────────────
	// This is the row an ASYMPTOTE fails. Beer-Lambert, an exponential decay, or any curve
	// approaching 1 without reaching it reads 0.9x here and this assertion is EXACT.
	TestEqual(TEXT("(b) ⭐⛔ AT the ceiling (20 ft) the fog is EXACTLY opaque — 1.0 to the bit. An asymptote reads 0.9x here and FAILS"),
		FSiegeFogStatics::FogDensityAt(Ceiling, Tuning), 1.f, Exact);

	// ── (c) ⛔ BEYOND THE CEILING — and these are the rows a curve sampled only inside the
	//     onset can never reach. Each is a real distance from FOG-§2's table.
	const float BeyondTheCeiling[] = {
		Ceiling + 1.f,        // one unit past
		ArrowTowerRange,      //  900 — the Arrow Tower's reach
		ArcherRange,          // 2100 — the Archer's and the Wizard's
		LongbowmanRange,      // 3600 — the Longbowman's, the biggest cut in the game
		50000.f               // castle to castle: 609.6 is 1.22% of this
	};

	for (const float Distance : BeyondTheCeiling)
	{
		const float Density = FSiegeFogStatics::FogDensityAt(Distance, Tuning);

		TestEqual(*FString::Printf(TEXT("(c) ⛔ At %.0f uu — beyond the ceiling — the fog is EXACTLY opaque, never 'nearly'"), Distance),
			Density, 1.f, Exact);

		// ⛔ And it never OVERSHOOTS. An unclamped linear ramp reads 10.8 at 3600 uu and would
		// hand the renderer a density far outside [0, 1].
		TestTrue(*FString::Printf(TEXT("(c) ⛔ …and never exceeds 1.0 at %.0f uu — an unclamped ramp reads 10.8 at Longbowman range"), Distance),
			Density <= 1.f);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⭐⭐ THE DISCRIMINATING TEST — the falloff ACCELERATES, and these rows fail
//     against a LINEAR ramp, against smoothstep, and against Beer-Lambert
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogFalloffAcceleratesTest,
	"Siegebound.Fog.TheFalloffAcceleratesBetweenTheOnsetAndTheCeilingAndIsNotLinear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogFalloffAcceleratesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float Onset     = Tuning.FogVisionOnsetUU;
	const float Ceiling   = Tuning.FogVisionCeilingUU;
	const float BandWidth = Ceiling - Onset; // = 304.8 uu — the ramp his sentence describes

	// Three points STRICTLY BETWEEN the boundaries. ⛔ These are the samples that make this file
	// a test of the CURVE rather than of two if-statements.
	const float QuarterPoint = Onset + 0.25f * BandWidth; // t = 0.25  (381.0 uu)
	const float MidPoint     = Onset + 0.50f * BandWidth; // t = 0.50  (457.2 uu)
	const float ThreeQuarter = Onset + 0.75f * BandWidth; // t = 0.75  (533.4 uu)

	const float AtQuarter = FSiegeFogStatics::FogDensityAt(QuarterPoint, Tuning);
	const float AtMid     = FSiegeFogStatics::FogDensityAt(MidPoint,     Tuning);
	const float AtThree   = FSiegeFogStatics::FogDensityAt(ThreeQuarter, Tuning);

	// ── (a) ⭐⭐ THE MIDPOINT IS THE WHOLE ARGUMENT ──────────────────────────────────────
	// quadratic  t² at t=0.50  ->  0.2500   <- what must ship
	// LINEAR     t  at t=0.50  ->  0.5000   <- FAILS this row by 0.25, i.e. 2,500× the tolerance
	// smoothstep 3t²-2t³       ->  0.5000   <- FAILS it identically
	// Beer-Lambert (concave)   ->  ~0.63    <- FAILS it in the other direction
	// ⇒ this single assertion separates the shipped shape from every plausible alternative.
	TestEqual(TEXT("(a) ⭐⭐ HALFWAY through the band the fog is only a QUARTER thick (t² at t=0.5 = 0.25). ⛔ A LINEAR ramp reads 0.50 and FAILS here — so does smoothstep"),
		AtMid, 0.25f, Tolerance);

	TestTrue(TEXT("(a) ⛔ …asserted as a bound as well, so any shape at or above linear fails: midpoint density is well UNDER 0.5"),
		AtMid < 0.4f);

	// ── (b) The other two between-points, same discrimination ───────────────────────────
	// quadratic 0.0625 vs linear 0.25 — a 4× separation.
	TestEqual(TEXT("(b) A QUARTER of the way in, density is 0.0625 (t² at t=0.25). ⛔ Linear reads 0.25 — a 4× separation"),
		AtQuarter, 0.0625f, Tolerance);

	// quadratic 0.5625 vs linear 0.75.
	TestEqual(TEXT("(b) THREE QUARTERS of the way in, density is 0.5625 (t² at t=0.75). ⛔ Linear reads 0.75"),
		AtThree, 0.5625f, Tolerance);

	// ── (c) ⭐ "THICKER AND THICKER", AS AN ASSERTION ABOUT THE SECOND DERIVATIVE ────────
	// His doubled comparative is an ACCELERATION claim, and this is what it means numerically:
	// the band's second half must deliver strictly MORE density than its first half.
	// ⛔ A LINEAR ramp delivers exactly 0.5 in each half and fails this row flat.
	const float GainedInFirstHalf  = AtMid - 0.f;
	const float GainedInSecondHalf = 1.f - AtMid;

	TestTrue(TEXT("(c) ⭐⭐ The SECOND half of the band thickens more than the first (0.75 vs 0.25) — that is \"thicker and thicker\". ⛔ A LINEAR ramp splits it 0.5/0.5 and FAILS"),
		GainedInSecondHalf > GainedInFirstHalf);

	// And the gap is decisive, not marginal — it fails against anything close to linear.
	TestTrue(TEXT("(c) ⛔ …and the acceleration is decisive: the second half delivers at least twice the first"),
		GainedInSecondHalf > GainedInFirstHalf * 2.f);

	// The rate itself rises across the band: three equal steps must gain progressively more.
	const float StepGainEarly  = AtQuarter - FSiegeFogStatics::FogDensityAt(Onset, Tuning);
	const float StepGainMiddle = AtMid     - AtQuarter;
	const float StepGainLate   = AtThree   - AtMid;

	TestTrue(TEXT("(c) Equal steps of DISTANCE buy increasing amounts of density — early < middle"),
		StepGainEarly < StepGainMiddle);

	TestTrue(TEXT("(c) …and middle < late. ⛔ A LINEAR ramp makes all three equal; a CONCAVE (Beer-Lambert) curve reverses them"),
		StepGainMiddle < StepGainLate);

	// ── (d) The exponent is a real tunable, and 1.0 really does yield linear ────────────
	// This is the "one-word retune to linear" claim in the header, asserted rather than
	// promised. It also proves tests (a)-(c) above are measuring the EXPONENT and not an
	// accident of the boundary code.
	FSiegeFogTuning LinearTuning = ShippedTuning();
	LinearTuning.FogDensityExponent = 1.f;

	TestEqual(TEXT("(d) ⭐ Retuning the exponent to 1.0 yields a LINEAR ramp — 0.5 at the midpoint, with no code change"),
		FSiegeFogStatics::FogDensityAt(MidPoint, LinearTuning), 0.5f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ⭐ MONOTONICITY — a full sweep that is PROVEN to straddle all three regions
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogMonotonicTest,
	"Siegebound.Fog.DensityIsMonotonicNonDecreasingAndStaysInsideZeroToOneAcrossTheWholeField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogMonotonicTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// 0 -> 1200 uu in 6-uu steps: 201 samples, roughly a quarter of them inside the ramp.
	constexpr int32 SampleCount = 201;
	constexpr float StepUU      = 6.f;

	float PreviousDensity = -1.f;

	// ⛔ THE ANTI-TRIVIALITY COUNTERS. A sweep that never left the clear bubble would be
	// monotonic, in range, and completely meaningless — so the sweep asserts its own coverage.
	int32 ClearSamples   = 0; // density == 0 exactly
	int32 RampSamples    = 0; // strictly between 0 and 1 — the curve under test
	int32 OpaqueSamples  = 0; // density == 1 exactly

	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Distance = static_cast<float>(Index) * StepUU;
		const float Density  = FSiegeFogStatics::FogDensityAt(Distance, Tuning);

		if (Density <= 0.f)      { ++ClearSamples;  }
		else if (Density >= 1.f) { ++OpaqueSamples; }
		else                     { ++RampSamples;   }

		if (!(Density >= 0.f && Density <= 1.f))
		{
			AddError(FString::Printf(TEXT("Density at %.1f uu left [0, 1]: %f"), Distance, Density));
			return false;
		}

		if (Density < PreviousDensity)
		{
			AddError(FString::Printf(TEXT("Density DECREASED at %.1f uu (%f after %f) — walking away must never make the world clearer"),
				Distance, Density, PreviousDensity));
			return false;
		}

		PreviousDensity = Density;
	}

	// ── ⛔⛔ THE SWEEP MUST HAVE ACTUALLY VISITED ALL THREE REGIONS ──────────────────────
	// Without these three rows the loop above is a test that proves nothing, which is exactly
	// the trap TASK-837 named. With them, a curve sampled only inside the onset FAILS.
	TestTrue(TEXT("⛔ ANTI-TRIVIALITY: the sweep visited the CLEAR bubble"),
		ClearSamples > 0);

	TestTrue(TEXT("⛔⛔ ANTI-TRIVIALITY: the sweep actually entered the RAMP — at least 20 samples strictly between 0 and 1. A sweep that never left the onset would pass every other row in this test and prove nothing"),
		RampSamples >= 20);

	TestTrue(TEXT("⛔ ANTI-TRIVIALITY: the sweep reached full OPACITY beyond the ceiling"),
		OpaqueSamples > 0);

	// The sweep ends beyond the ceiling, so the last sample must be fully opaque.
	TestEqual(TEXT("The sweep's far end (1200 uu — twice the ceiling) is EXACTLY opaque"),
		PreviousDensity, 1.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. ⛔⛔ WITH FOG OFF THE GAME IS BYTE-FOR-BYTE THE GAME THAT SHIPPED
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogInertWhenInactiveTest,
	"Siegebound.Fog.EffectiveVisionRadiusIsBitIdenticalToTheRequestWhenFogIsInactive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogInertWhenInactiveTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// Every shipped range in the game, plus the degenerate 0 and a value far past the ceiling.
	const float EveryShippedRange[] = {
		0.f, MeleeRange, ClericRange, BombTowerRange, ArrowTowerRange,
		BallistaTowerRange, ArcherRange, LongbowmanRange, 50000.f
	};

	for (const float Range : EveryShippedRange)
	{
		// EXACT, not near-exact. ⭐ This is the property that keeps a fog regression
		// ATTRIBUTABLE: with no fog in play, TASK-838's funnel must hand every site back the
		// identical float it passed in, so nothing that changes can be blamed on anything else.
		TestEqual(*FString::Printf(TEXT("⛔⛔ With fog INACTIVE a %.0f-uu range comes back EXACTLY unchanged — not one ulp of drift"), Range),
			FSiegeFogStatics::EffectiveVisionRadius(Range, /*bFogActive=*/false, Tuning), Range, Exact);

		// …and the predicate agrees: a Longbowman at 3000 uu still sees its target with no fog.
		TestTrue(*FString::Printf(TEXT("With fog INACTIVE, a target at 99%% of a %.0f-uu range is still visible"), Range),
			Range <= 0.f || FSiegeFogStatics::IsVisibleThroughFog(Range * 0.99f, Range, /*bFogActive=*/false, Tuning));
	}

	// ⛔ The one that would catch a clamp applied unconditionally: 3600 must survive intact.
	TestEqual(TEXT("⛔ The Longbowman's 3600 is untouched with fog off — an unconditional clamp would read 609.6 here"),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/false, Tuning), LongbowmanRange, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⭐⭐ FOG-§2's CONSEQUENCE TABLE, AS ASSERTIONS — the largest combat modifier
//     in the game, measured rather than described
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampsTheShippedRangesTest,
	"Siegebound.Fog.TheCeilingClampsEveryLongRangeCardAndLeavesTheShortOnesUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampsTheShippedRangesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ── (a) EVERYTHING LONGER THAN THE CEILING COLLAPSES ONTO IT ────────────────────────
	const float ClampedRanges[] = {
		LongbowmanRange, ArcherRange, BallistaTowerRange, ArrowTowerRange, BombTowerRange
	};

	for (const float Range : ClampedRanges)
	{
		TestEqual(*FString::Printf(TEXT("(a) A %.0f-uu range clamps to the ceiling under fog"), Range),
			FSiegeFogStatics::EffectiveVisionRadius(Range, /*bFogActive=*/true, Tuning), Ceiling, Exact);
	}

	// ── (b) ⭐ THE CUTS, AS NUMBERS — FOG-§2's disclosure, asserted so it cannot rot ─────
	// ⛔ These rows fail against a ceiling of 600 (which reads -83.3% / -71.4%) as well as
	// against feet-as-units. They are the reason the table in the header stays true.
	TestEqual(TEXT("(b) ⛔⛔ The LONGBOWMAN loses 83.1% of its reach — the single largest combat modifier in the game"),
		1.f - (Ceiling / LongbowmanRange), 0.831f, 1.e-3f);

	TestEqual(TEXT("(b) ⛔ The ARCHER and the WIZARD lose 71.0%"),
		1.f - (Ceiling / ArcherRange), 0.710f, 1.e-3f);

	TestEqual(TEXT("(b) ⛔ The BALLISTA TOWER loses 56.5% — and with its MinRange 300 still applying, its usable band collapses to a 300-609.6 ANNULUS"),
		1.f - (Ceiling / BallistaTowerRange), 0.565f, 1.e-3f);

	TestEqual(TEXT("(b) The ARROW TOWER loses 32.3%"),
		1.f - (Ceiling / ArrowTowerRange), 0.323f, 1.e-3f);

	// The BallistaTower annulus is real: its MinRange survives INSIDE the fogged ceiling, so
	// there is still a band it can shoot in. A ceiling below 300 would close it entirely — this
	// row is the tripwire for that retune.
	TestTrue(TEXT("(b) ⚠️ The Ballista's 300 MinRange is still INSIDE the ceiling, so the annulus is non-empty. A retune below 300 would silently disable the card"),
		Ceiling > 300.f);

	// ── (c) ⛔⛔ THE SHORT RANGES ARE UNTOUCHED — AND THERE IS NO INVENTED FLOOR ─────────
	// FOG-§2 lists the Cleric and every melee unit as UNAFFECTED. `min`, not `clamp`: a
	// hidden floor would RAISE these to 609.6 and would be a buff to every melee unit in the
	// game, delivered under the banner of a visibility nerf.
	TestEqual(TEXT("(c) ✅ The CLERIC's 400 heal range is UNAFFECTED — already inside the ceiling"),
		FSiegeFogStatics::EffectiveVisionRadius(ClericRange, /*bFogActive=*/true, Tuning), ClericRange, Exact);

	TestEqual(TEXT("(c) ✅ Every MELEE unit's 120 is UNAFFECTED"),
		FSiegeFogStatics::EffectiveVisionRadius(MeleeRange, /*bFogActive=*/true, Tuning), MeleeRange, Exact);

	TestEqual(TEXT("(c) ⛔⛔ A 0-uu range stays 0 — there is NO FLOOR, and none may be invented to soften his number"),
		FSiegeFogStatics::EffectiveVisionRadius(0.f, /*bFogActive=*/true, Tuning), 0.f, Exact);

	TestTrue(TEXT("(c) ⛔ A 50-uu range is never RAISED toward the ceiling — min(), not clamp()"),
		FSiegeFogStatics::EffectiveVisionRadius(50.f, /*bFogActive=*/true, Tuning) <= 50.f);

	// ── (d) THE SCALE, so the header's 1.22% claim is asserted and not merely written ────
	TestEqual(TEXT("(d) ⭐ The ceiling is 1.22% of the 50,000-uu castle-to-castle distance — a player sees one-eightieth of the way to the enemy"),
		Ceiling / 50000.f, 0.0122f, 1.e-4f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. ⛔ THERE IS NO INVENTED FLOOR, AND THE CLAMP NEVER LENGTHENS A RANGE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogNoInventedFloorTest,
	"Siegebound.Fog.TheClampOnlyEverShortensARangeAndNeverLengthensOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogNoInventedFloorTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// Sweep the whole plausible range space, including everything below the ceiling where an
	// invented floor would hide. ⛔ FOG-§2 is DISCLOSURE, not a counter-proposal: softening
	// Jonathan's number with a floor would be adjusting his ruling while appearing to honour it,
	// and it would be invisible in a review that only checked the long-range cards.
	// ⛔ ANTI-TRIVIALITY COUNTERS, for the same reason test 4 carries them: a sweep that only
	// ever sampled long ranges would never exercise the pass-through branch where an invented
	// floor would actually live, and it would report green over the bug it was written to catch.
	int32 PassedThroughUntouched = 0; // requests below the ceiling, returned as-is
	int32 ClampedToCeiling       = 0; // requests above the ceiling, cut to it

	for (int32 Index = 0; Index <= 100; ++Index)
	{
		const float Requested = static_cast<float>(Index) * 40.f; // 0 .. 4000 uu
		const float Effective = FSiegeFogStatics::EffectiveVisionRadius(Requested, /*bFogActive=*/true, Tuning);

		if (Effective > Requested)
		{
			AddError(FString::Printf(TEXT("⛔ The clamp LENGTHENED a range: %.0f -> %.1f. Fog is a nerf; a floor here would buff every short-ranged unit in the game"),
				Requested, Effective));
			return false;
		}

		if (Effective > Tuning.FogVisionCeilingUU)
		{
			AddError(FString::Printf(TEXT("⛔ The clamp let %.0f through above the ceiling: %.1f"), Requested, Effective));
			return false;
		}

		if (Effective == Requested) { ++PassedThroughUntouched; }
		else                        { ++ClampedToCeiling;       }
	}

	TestTrue(TEXT("⛔ ANTI-TRIVIALITY: the sweep actually exercised the PASS-THROUGH branch (ranges below the ceiling) — that is where an invented floor would hide"),
		PassedThroughUntouched >= 10);

	TestTrue(TEXT("⛔ ANTI-TRIVIALITY: the sweep actually exercised the CLAMP branch too — so the two rows above are measuring both code paths"),
		ClampedToCeiling >= 10);

	// The two directions stated as single explicit rows, so a reader sees the rule without
	// reconstructing the loop.
	TestTrue(TEXT("⛔ Below the ceiling the request is returned untouched — the fog is a MAXIMUM, never a minimum"),
		FSiegeFogStatics::EffectiveVisionRadius(100.f, /*bFogActive=*/true, Tuning) == 100.f);

	TestTrue(TEXT("⛔ Above the ceiling the request is cut to the ceiling — never to something shorter"),
		FSiegeFogStatics::EffectiveVisionRadius(5000.f, /*bFogActive=*/true, Tuning) == Tuning.FogVisionCeilingUU);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⭐ THE PREDICATE TASK-838 CONSUMES — inclusive, exactly like every shipped
//     range comparison in the project
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisibilityPredicateTest,
	"Siegebound.Fog.TheVisibilityPredicateMatchesTheShippedInclusiveRangeComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisibilityPredicateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ── (a) ⭐⭐ THE FEATURE, IN ONE PAIR OF ROWS ────────────────────────────────────────
	// A Longbowman and a target 3000 uu apart: a routine engagement today, impossible in fog.
	TestTrue(TEXT("(a) With NO fog a Longbowman sees a target at 3000 uu — the game as it ships"),
		FSiegeFogStatics::IsVisibleThroughFog(3000.f, LongbowmanRange, /*bFogActive=*/false, Tuning));

	TestFalse(TEXT("(a) ⭐⭐ UNDER FOG the same Longbowman cannot see the same target — this is the card"),
		FSiegeFogStatics::IsVisibleThroughFog(3000.f, LongbowmanRange, /*bFogActive=*/true, Tuning));

	// …and it can still fight inside the ceiling. ⛔ Fog shortens the Longbowman; it does not
	// delete it. A predicate that returned false everywhere under fog would pass the row above.
	TestTrue(TEXT("(a) ⛔ …but it CAN still shoot at 500 uu under fog — fog shortens the Longbowman, it does not disable it"),
		FSiegeFogStatics::IsVisibleThroughFog(500.f, LongbowmanRange, /*bFogActive=*/true, Tuning));

	// ── (b) ⭐ THE BOUNDARY IS INCLUSIVE, matching SummonedUnit.cpp:1644 et al ───────────
	// The clamp substitutes the OPERAND of the project's `Distance <= Range` comparison and
	// never its shape. ⛔ An exclusive predicate fails this row, and it would make the fog
	// boundary the only exclusive range boundary in the game.
	TestTrue(TEXT("(b) ⭐ At EXACTLY the ceiling the target is still visible — the shipped idiom is `Distance <= Range` and the clamp changes the operand, not the comparison"),
		FSiegeFogStatics::IsVisibleThroughFog(Ceiling, LongbowmanRange, /*bFogActive=*/true, Tuning));

	TestFalse(TEXT("(b) One uu PAST the ceiling the target is gone — the boundary is real and it is where his 20 feet says it is"),
		FSiegeFogStatics::IsVisibleThroughFog(Ceiling + 1.f, LongbowmanRange, /*bFogActive=*/true, Tuning));

	// ── (c) ⛔ THE UNAFFECTED CARDS BEHAVE IDENTICALLY WITH AND WITHOUT FOG ─────────────
	// The Cleric's 400 and melee's 120 are entirely inside the ceiling, so fog cannot change a
	// single one of their engagements. Asserted in BOTH states so an implementation that
	// shortened them anyway fails.
	TestTrue(TEXT("(c) ✅ A CLERIC heals at 350 uu with no fog"),
		FSiegeFogStatics::IsVisibleThroughFog(350.f, ClericRange, /*bFogActive=*/false, Tuning));

	TestTrue(TEXT("(c) ✅ …and heals at 350 uu UNDER fog too — FOG-§2 lists it UNAFFECTED"),
		FSiegeFogStatics::IsVisibleThroughFog(350.f, ClericRange, /*bFogActive=*/true, Tuning));

	TestFalse(TEXT("(c) …and it still cannot heal at 450 uu under fog, because its OWN 400 range still binds — fog did not extend anything"),
		FSiegeFogStatics::IsVisibleThroughFog(450.f, ClericRange, /*bFogActive=*/true, Tuning));

	TestTrue(TEXT("(c) ✅ A MELEE unit engages at 100 uu under fog exactly as it does without"),
		FSiegeFogStatics::IsVisibleThroughFog(100.f, MeleeRange, /*bFogActive=*/true, Tuning));

	// ── (d) The unit's OWN range still binds under fog — the ceiling is a second limit,
	//     never a replacement for the first ────────────────────────────────────────────
	// ⛔ Fails against an implementation that returned "visible" for anything inside 609.6
	// regardless of the caller's range, which would GRANT melee units a 5× reach in fog.
	TestFalse(TEXT("(d) ⛔⛔ A MELEE unit does NOT gain reach in fog — a target at 500 uu is inside the ceiling but far outside its own 120"),
		FSiegeFogStatics::IsVisibleThroughFog(500.f, MeleeRange, /*bFogActive=*/true, Tuning));

	TestFalse(TEXT("(d) ⛔ Nor does the Cleric — 500 uu is inside the ceiling but outside its 400"),
		FSiegeFogStatics::IsVisibleThroughFog(500.f, ClericRange, /*bFogActive=*/true, Tuning));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ⛔ TOTALITY — no input produces a NaN, a divide-by-zero, or a blinded army
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogDegenerateInputsTest,
	"Siegebound.Fog.DegenerateInputsNeverReturnNaNAndNeverBlindTheArmy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogDegenerateInputsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const float NaN      = MakeNaN();
	const float Infinity = MakeInfinity();

	// ── (a) A GARBAGE DISTANCE NEVER FOGS AND NEVER PROPAGATES ──────────────────────────
	const FSiegeFogTuning Tuning = ShippedTuning();

	TestEqual(TEXT("(a) A NaN distance yields EXACTLY clear — garbage must never paint an opaque screen nobody can diagnose"),
		FSiegeFogStatics::FogDensityAt(NaN, Tuning), 0.f, Exact);

	TestEqual(TEXT("(a) An INFINITE distance yields EXACTLY clear from the same guard"),
		FSiegeFogStatics::FogDensityAt(Infinity, Tuning), 0.f, Exact);

	TestEqual(TEXT("(a) A NEGATIVE distance is inside the onset ⇒ clear"),
		FSiegeFogStatics::FogDensityAt(-1000.f, Tuning), 0.f, Exact);

	// The predicate is the ONE place that fails the other way: a garbage DISTANCE is a broken
	// TARGET, and acquiring it would push the NaN into a move order.
	TestFalse(TEXT("(a) ⛔ A NaN distance is NOT visible — a broken target is never acquired, so the NaN cannot reach a move order"),
		FSiegeFogStatics::IsVisibleThroughFog(NaN, LongbowmanRange, /*bFogActive=*/true, Tuning));

	// ── (b) ⛔⛔ A BROKEN TUNING MUST NEVER BLIND THE ARMY ───────────────────────────────
	// This is the failure that would take out every attack in the game at once, because
	// TASK-838 puts this call inside the funnel every acquisition routes through.
	FSiegeFogTuning NaNCeiling = ShippedTuning();
	NaNCeiling.FogVisionCeilingUU = NaN;

	TestEqual(TEXT("(b) ⛔⛔ A NaN ceiling returns the request UNCHANGED — a broken tuning degrades to NO FOG, never to NO VISION"),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/true, NaNCeiling), LongbowmanRange, Exact);

	FSiegeFogTuning NegativeCeiling = ShippedTuning();
	NegativeCeiling.FogVisionCeilingUU = -500.f;

	TestEqual(TEXT("(b) ⛔⛔ A NEGATIVE ceiling returns the request UNCHANGED — a bare min() here would clamp every unit in the game to a negative range and stop all combat"),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/true, NegativeCeiling), LongbowmanRange, Exact);

	// ── (c) THE ZERO-WIDTH AND INVERTED BANDS — no divide by zero, no NaN ───────────────
	FSiegeFogTuning ZeroWidth = ShippedTuning();
	ZeroWidth.FogVisionOnsetUU   = 400.f;
	ZeroWidth.FogVisionCeilingUU = 400.f;

	TestEqual(TEXT("(c) A ZERO-WIDTH band is a hard step: clear one uu before it"),
		FSiegeFogStatics::FogDensityAt(399.f, ZeroWidth), 0.f, Exact);

	TestEqual(TEXT("(c) …and EXACTLY opaque at and past it — the continuous limit of the ramp, not a divide by zero"),
		FSiegeFogStatics::FogDensityAt(400.f, ZeroWidth), 1.f, Exact);

	FSiegeFogTuning Inverted = ShippedTuning();
	Inverted.FogVisionOnsetUU   = DerivedCeilingUU; // the two defaults, swapped
	Inverted.FogVisionCeilingUU = DerivedOnsetUU;

	const float InvertedNear = FSiegeFogStatics::FogDensityAt(100.f, Inverted);
	const float InvertedFar  = FSiegeFogStatics::FogDensityAt(1000.f, Inverted);

	TestTrue(TEXT("(c) An INVERTED band still returns finite values in [0, 1] — it degenerates to a step, it does not produce a negative t"),
		FMath::IsFinite(InvertedNear) && FMath::IsFinite(InvertedFar)
		&& InvertedNear >= 0.f && InvertedNear <= 1.f && InvertedFar >= 0.f && InvertedFar <= 1.f);

	TestTrue(TEXT("(c) …and it is still non-decreasing with distance"),
		InvertedFar >= InvertedNear);

	// ── (d) THE EXPONENT'S DEGENERATE VALUES FALL BACK TO LINEAR ───────────────────────
	// ⛔ Pow(t, 0) is 1 for every t, which would paint a WALL of fog at the onset. The fallback
	// is to the neutral shape instead, and these rows are what stop that regression.
	const float MidPoint = DerivedOnsetUU + 0.5f * (DerivedCeilingUU - DerivedOnsetUU);

	const float DegenerateExponents[] = { 0.f, -2.f, NaN, Infinity };

	for (const float BadExponent : DegenerateExponents)
	{
		FSiegeFogTuning BadTuning = ShippedTuning();
		BadTuning.FogDensityExponent = BadExponent;

		const float Density = FSiegeFogStatics::FogDensityAt(MidPoint, BadTuning);

		TestEqual(TEXT("(d) ⛔ A degenerate exponent falls back to LINEAR (0.5 at the midpoint), NOT to Pow(t, 0) == 1, which would be a wall of fog at the onset"),
			Density, 0.5f, Tolerance);
	}

	// The onset stays clear under every degenerate exponent — the wall, asserted directly.
	FSiegeFogTuning ZeroExponent = ShippedTuning();
	ZeroExponent.FogDensityExponent = 0.f;

	TestEqual(TEXT("(d) ⛔⛔ …and one uu past the onset is still nearly clear under a zero exponent — the wall never appears"),
		FSiegeFogStatics::FogDensityAt(DerivedOnsetUU + 1.f, ZeroExponent), 0.f, 0.01f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
