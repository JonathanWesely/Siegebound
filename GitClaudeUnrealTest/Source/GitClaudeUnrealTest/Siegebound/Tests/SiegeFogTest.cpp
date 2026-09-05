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
 *  ⛔⛔⭐⭐ AMENDED 2026-09-04 (TASK-981, law FOG-§9.2) — THE CURVE UNDER TEST **CHANGED SIDES**,
 *  AND THIS BLOCK IS RE-DERIVED RATHER THAN RE-SIGNED (`SC-§60`)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⚠️⚠️ READ THIS BEFORE TRUSTING ANY "…AND THIS KILLS X" COMMENT BELOW. Until 2026-09-04 the
 *  shipped curve was a QUADRATIC EASE-IN and **BEER-LAMBERT WAS ONE OF THE RED CONTROLS** — the
 *  midpoint row was written precisely because linear, smoothstep AND Beer-Lambert all fail it.
 *  ⛔ Jonathan has now ruled Beer-Lambert IN, so that assertion is not merely stale: **the old
 *  red control is the very fixture that just changed sides.** ⇒ every discriminator in this file
 *  was RE-DERIVED against the new curve, and the list of shapes each row kills is restated from
 *  scratch below. ⛔ A row inherited unchanged would now be asserting the losing side of a
 *  ruling while looking like coverage.
 *
 *  ⛔⛔ AND THE OLD CURVE IS NOW ITSELF A RED CONTROL, BY NAME. Test 3 asserts the shipped values
 *  are far from what `t²` returns at the same distances — including at the onset, where the two
 *  disagree by **0.859 out of 1.0**, which is the entire substance of his ruling.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHY EVERY ASSERTION HERE CAN ACTUALLY FAIL — the standing worry, answered concretely
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⚠️ A falloff test passes TRIVIALLY if every sample it takes happens to sit somewhere the
 *  curve is flat — the suite reports green over a shape nobody exercised. ⛔ That failure mode is
 *  defused explicitly, not hoped away:
 *    • test 2 asserts the camera, HIS TWO QUOTED NUMBERS (86% at 10 ft, 98% at 20 ft), and the
 *      asymptotic tail beyond the ceiling;
 *    • ⭐ test 3 asserts three ABSOLUTE values derived from his ratio by a DIFFERENT arithmetic
 *      route than the implementation uses (`Sqrt` here vs `Exp`/`Loge` there), so the two can
 *      only agree if σ really is derived from the ceiling. It then names the shapes each row
 *      kills — the retired `t²`, linear-in-distance, smoothstep and `sqrt(d/ceiling)`;
 *    • ⭐⭐ test 4's sweep counts a NEW category re-derived for this curve: samples INSIDE the
 *      old onset that are strictly positive. Under the retired curve every one of those was
 *      exactly 0, so that counter is the sweep-level embodiment of his ruling and it goes RED
 *      against any curve that still carries a clear bubble;
 *    • ⭐ test 10 RETUNES both tunables and asserts the curve moves with them, so a hardcoded σ
 *      — the single most plausible wrong implementation — cannot pass.
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

	// ═══════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ THE BEER-LAMBERT EXPECTATIONS (TASK-981, FOG-§9.2) — DERIVED FROM **HIS RATIO**
	//  BY A ⛔ DIFFERENT ARITHMETIC ROUTE THAN THE IMPLEMENTATION USES
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 *  ⭐ HIS NUMBER, from his own sentence: *"Tuned to 98% obscured at 20 feet"* ⇒ 2% of the
	 *  light still arrives. ⛔ Written here as the RATIO he named, not transcribed from the
	 *  header's `FogTransmittanceAtCeiling` — an expectation copied from its subject agrees with
	 *  a wrong subject by construction and reports SAFE forever (`SHIP-§9c`).
	 */
	constexpr float HisTransmittanceAtCeiling = 0.02f;

	/**
	 *  ⭐⭐⭐ THE LOAD-BEARING TRICK IN THIS FILE, AND IT IS WHY THESE ROWS ARE EVIDENCE RATHER
	 *  THAN A RESTATEMENT: the three expectations below are built with **`FMath::Sqrt`**, while
	 *  `FogDensityAt` computes its answer with **`FMath::Loge` + `FMath::Exp`**. Two different
	 *  functions, two different code paths, one agreed answer.
	 *
	 *  ⛔⛔ THIS IS ⛔ NOT THE BANNED √ TEST. `FOG-§9.2` and `TASK-981(3)` forbid asserting the
	 *  RELATION `f(d/2) == √f(d)` — a THEOREM of the model, true for every σ and every d, which
	 *  could therefore never fail and would report SAFE forever. ⭐ What is asserted here is
	 *  something entirely different: three **ABSOLUTE NUMBERS**. They depend on σ being derived
	 *  from `FogVisionCeilingUU` specifically, so they go RED against a hardcoded σ, against a σ
	 *  derived from the ONSET instead (which reads 0.9996 at the ceiling, not 0.98), against a
	 *  base-10 log, and against a sign error. ⛔ The √ is the DERIVATION PATH, never the CLAIM.
	 *
	 *      T(ceiling)     = 0.02            ⇒ obscuration 0.9800   ⭐ his "98% at 20 feet"
	 *      T(ceiling/2)   = √0.02           ⇒ obscuration 0.8586   ⭐ his "86% at 10 feet"
	 *      T(ceiling/4)   = √√0.02          ⇒ obscuration 0.6239
	 */
	static float ExpectedObscurationAtCeiling()
	{
		return 1.f - HisTransmittanceAtCeiling;
	}

	static float ExpectedObscurationAtHalfTheCeiling()
	{
		return 1.f - FMath::Sqrt(HisTransmittanceAtCeiling);
	}

	static float ExpectedObscurationAtAQuarterOfTheCeiling()
	{
		return 1.f - FMath::Sqrt(FMath::Sqrt(HisTransmittanceAtCeiling));
	}

	/**
	 *  ⛔⛔ THE RETIRED CURVE, KEPT ALIVE **ONLY AS A RED CONTROL** (`SC-§60`). This is the `t²`
	 *  ease-in TASK-981 deleted from the shipping header, reimplemented here so test 3 can assert
	 *  the shipped function is NOT it. ⭐ Keeping the loser computable is what turns "we changed
	 *  the curve" into a measurement instead of a claim.
	 *  ⛔ It is deliberately NOT reachable from shipping code and must never be promoted out of
	 *  this fixture.
	 */
	static float RetiredQuadraticDensityAt(float DistanceUU)
	{
		if (DistanceUU <= DerivedOnsetUU)   { return 0.f; }
		if (DistanceUU >= DerivedCeilingUU) { return 1.f; }
		const float T = (DistanceUU - DerivedOnsetUU) / (DerivedCeilingUU - DerivedOnsetUU);
		return T * T;
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

	// ── (e) ⭐⭐ THE SHAPE KNOB IS HIS RATIO — 2% transmittance = "98% obscured at 20 feet" ──
	// ⛔ REPLACED 2026-09-04 (TASK-981). This row used to assert `FogDensityExponent == 2` for the
	// quadratic ease-in; that tunable is RETIRED and the ruling it encoded is overturned.
	// ⭐ The expectation is HIS PERCENTAGE re-derived (1 − 0.98), never transcribed from the
	// header, so it disagrees with a wrong header the moment the header is wrong.
	TestEqual(TEXT("(e) ⭐⭐ The shipped transmittance at the ceiling is 0.02 — his \"98% obscured at 20 feet\", written as the 2% that still gets through"),
		Tuning.FogTransmittanceAtCeiling, 1.f - 0.98f, Tolerance);

	// ⛔ And it is a RATIO, so it must live strictly inside (0, 1). At 0 the extinction
	// coefficient is INFINITE (an opaque screen at the camera); at 1 it is ZERO (a 50-gold card
	// that does nothing). Both are game-breaking values and this row is the tripwire for a retune
	// that lands on either.
	TestTrue(TEXT("(e) ⛔ The transmittance is strictly inside (0, 1) — 0 makes σ infinite (whiteout), 1 makes σ zero (no fog at all)"),
		Tuning.FogTransmittanceAtCeiling > 0.f && Tuning.FogTransmittanceAtCeiling < 1.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⭐⭐ HIS TWO QUOTED NUMBERS, AS ASSERTIONS — 86% obscured at 10 ft and 98% at
//     20 ft — plus the asymptotic tail that REPLACED the hard cut on the picture
//
//  ⛔⛔ REWRITTEN 2026-09-04 (TASK-981, FOG-§9.2). This test used to assert "EXACTLY
//  0 at the onset, EXACTLY 1 at the ceiling". ⛔ BOTH HALVES ARE NOW FALSE, and not
//  because they were wrong — because Jonathan overruled the curve they described.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogBoundariesTest,
	"Siegebound.Fog.DensityIsExactlyZeroAtTheCameraAndIsHisEightySixPercentAtTenFeetAndNinetyEightAtTwenty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogBoundariesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float Onset   = Tuning.FogVisionOnsetUU;
	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ── (a) THE CAMERA IS EXACTLY CLEAR — the ONE exact value the new curve still has ────
	// `1 − exp(0)` is 0 identically, so this is a real bit-level claim rather than a rounding
	// hope. ⛔ It is also the guard that stops a negative distance producing a NEGATIVE density.
	TestEqual(TEXT("(a) At the camera (0 uu) the world is EXACTLY clear — 1 − exp(0) = 0, to the bit"),
		FSiegeFogStatics::FogDensityAt(0.f, Tuning), 0.f, Exact);

	TestEqual(TEXT("(a) ⛔ A NEGATIVE distance is EXACTLY clear too — `1 − exp(+x)` is unbounded BELOW and must never leak a negative density to the renderer"),
		FSiegeFogStatics::FogDensityAt(-500.f, Tuning), 0.f, Exact);

	// ── (b) ⭐⭐⭐ HIS TWO NUMBERS. THIS IS THE WHOLE RULING, AS TWO ROWS ─────────────────
	// ⛔ Both expectations are derived from his RATIO by `Sqrt`, while the subject computes with
	// `Loge`/`Exp`. They can only agree if σ is genuinely −ln(T)/ceiling.
	TestEqual(TEXT("(b) ⭐⭐ AT the ceiling (20 ft / 609.6 uu) the fog is 98.00% obscured — HIS \"98% obscured at 20 feet\", asserted as 1 − 0.02"),
		FSiegeFogStatics::FogDensityAt(Ceiling, Tuning), ExpectedObscurationAtCeiling(), Tolerance);

	TestEqual(TEXT("(b) ⭐⭐ AT the onset (10 ft / 304.8 uu) the fog is 85.86% obscured — HIS \"it forces 86% at 10\", asserted as 1 − √0.02"),
		FSiegeFogStatics::FogDensityAt(Onset, Tuning), ExpectedObscurationAtHalfTheCeiling(), Tolerance);

	// ── (c) ⛔⛔ THE INVERSION, ASSERTED BY NAME — the row that fails against what shipped ─
	// Until 2026-09-04 this exact distance returned EXACTLY 0. His ruling moved it to 0.859. ⇒ a
	// tree that still carried the retired curve fails HERE, by 0.859 out of 1.0, which is the
	// largest single disagreement anywhere in this suite.
	TestTrue(TEXT("(c) ⛔⛔ At the onset the fog is THICK, not absent — the retired t² curve returned EXACTLY 0 here and this row is what kills it"),
		FSiegeFogStatics::FogDensityAt(Onset, Tuning) > 0.5f);

	// ── (d) ⛔⛔ THE PICTURE IS NOT A HARD CUT ANY MORE — asymptotic, by his choice ───────
	// ⛔ THE MECHANIC STILL IS ONE, and that divergence is deliberate (FOG-§9.2): at the ceiling
	// the picture says "you can just barely make something out" while acquisition says ZERO.
	// Nobody harmonises them. This row is the one a hard-cut density fails.
	TestTrue(TEXT("(d) ⛔⛔ AT the ceiling the picture is strictly LESS than fully obscured — Beer-Lambert never arrives, and a curve that reads 1.0 here is the retired hard cut"),
		FSiegeFogStatics::FogDensityAt(Ceiling, Tuning) < 1.f);

	// ── (e) ⚠️ THE DECLARED CONSEQUENCE: THE MELEE BAND IS NOW HAZED ────────────────────
	// The old curve rendered 120 uu perfectly clear and FOG-§2 listed melee as UNAFFECTED. Under
	// his ruling it is ~53.7% obscured. ⭐ Asserted rather than left to be discovered — and the
	// row immediately after proves the MECHANIC is untouched, which is the half that matters.
	TestTrue(TEXT("(e) ⚠️ At melee range (120 uu) the picture is now over HALF obscured — the retired curve rendered it perfectly clear. Declared consequence of his ruling, not a regression"),
		FSiegeFogStatics::FogDensityAt(MeleeRange, Tuning) > 0.5f);

	TestEqual(TEXT("(e) ⭐⭐ …and the MECHANIC is untouched: a melee unit's 120-uu reach is returned bit-identically under fog. THE LOOK CHANGED; THE GAME DID NOT"),
		FSiegeFogStatics::EffectiveVisionRadius(MeleeRange, /*bFogActive=*/true, Tuning), MeleeRange, Exact);

	// ── (f) BEYOND THE CEILING — monotone, bounded, and approaching 1 without a step ─────
	// Each is a real distance from FOG-§2's table. ⛔ These are the samples a curve exercised only
	// near the camera can never reach.
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

		TestTrue(*FString::Printf(TEXT("(f) ⛔ At %.0f uu — beyond the ceiling — the fog is at least as thick as his 98%%"), Distance),
			Density >= ExpectedObscurationAtCeiling());

		// ⛔ And it never OVERSHOOTS. An unclamped linear ramp reads 10.8 at 3600 uu and would
		// hand the renderer a density far outside [0, 1].
		TestTrue(*FString::Printf(TEXT("(f) ⛔ …and never exceeds 1.0 at %.0f uu — an unclamped ramp reads 10.8 at Longbowman range"), Distance),
			Density <= 1.f);
	}

	// ⭐ ONE uu past the ceiling the picture is still essentially the ceiling's picture — the
	// curve is smooth THERE, which is exactly where the mechanic is a cliff. ⛔ This row is the
	// evidence for the "picture and mechanic disagree on purpose" claim in the header.
	TestTrue(TEXT("(f) ⭐⭐ One uu past the ceiling the PICTURE barely changes (still ~0.98) while ACQUISITION has already gone to zero — the divergence, measured"),
		FSiegeFogStatics::FogDensityAt(Ceiling + 1.f, Tuning) - FSiegeFogStatics::FogDensityAt(Ceiling, Tuning) < 0.01f);

	TestFalse(TEXT("(f) ⭐⭐ …and the MECHANIC at that same one uu is already absolute — a Longbowman cannot see 609.6 + 1"),
		FSiegeFogStatics::IsVisibleThroughFog(Ceiling + 1.f, LongbowmanRange, /*bFogActive=*/true, Tuning));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⭐⭐⭐ THE DISCRIMINATING TEST — **INVERTED 2026-09-04 (TASK-981)**, AND ITS
//     ANTI-FAKE ARGUMENT IS RE-DERIVED FROM SCRATCH RATHER THAN RE-SIGNED
//
//  ⛔⛔ THE RULING THIS ROW'S POLARITY NOW SERVES: `FOG-§9.2`, from Jonathan's
//  *"Tuned to 98% obscured at 20 feet, it forces 86% at 10 … so lets do that."*
//
//  ⛔⛔⭐⭐ WHY THIS BANNER IS LONG (`SC-§60` cl. 2/cl. 3 — the row is KEPT AND
//  INVERTED, never deleted, WITH ITS HISTORY):
//
//  This test used to be titled "…AcceleratesAndIsNotLinear" and it asserted the
//  midpoint reads **0.25**. Its stated red controls were linear (0.50), smoothstep
//  (0.50) and ⛔ **BEER-LAMBERT (~0.63)**. ⇒ **THE CURVE THIS ROW EXISTED TO KILL IS
//  THE CURVE THAT NOW SHIPS.** That is the exact hazard `SC-§60` cl. 3 names: *the
//  old red control is frequently the very fixture that just changed sides.*
//
//  ⛔ SO NOTHING BELOW IS INHERITED. Re-derived, and each row says what it kills:
//    • the THREE ANCHORS kill every alternative SHAPE, including `sqrt(d/ceiling)`,
//      which passes the concavity rows but misses 0.8586 by 0.15;
//    • the RETIRED `t²` CURVE is now itself a red control, computed in the fixture;
//    • CONCAVITY replaces acceleration — and it is a genuine flip, not a rewording:
//      the old row demanded the second half gain MORE, this one demands it gain LESS.
//      ⛔ A tree still carrying the old curve fails it in the opposite direction.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogFalloffAcceleratesTest,
	"Siegebound.Fog.TheFalloffIsBeerLambertExtinctionAndDeceleratesAndIsNotTheRetiredQuadratic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogFalloffAcceleratesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float Ceiling = Tuning.FogVisionCeilingUU;

	// ⛔ THE ANCHORS ARE FRACTIONS OF THE CEILING, NOT POINTS IN A BAND. The old test sampled
	// `onset + t × (ceiling − onset)` because the curve lived in that band. Beer-Lambert has no
	// band — it is defined from the CAMERA — so the natural anchors are ceiling/4, ceiling/2
	// (which is the onset) and the ceiling itself.
	const float QuarterOfCeiling = Ceiling * 0.25f; // 152.4 uu —  5 ft
	const float HalfOfCeiling    = Ceiling * 0.50f; // 304.8 uu — 10 ft, == FogVisionOnsetUU
	const float ThreeQuarters    = Ceiling * 0.75f; // 457.2 uu — 15 ft

	const float AtQuarter = FSiegeFogStatics::FogDensityAt(QuarterOfCeiling, Tuning);
	const float AtHalf    = FSiegeFogStatics::FogDensityAt(HalfOfCeiling,    Tuning);
	const float AtCeiling = FSiegeFogStatics::FogDensityAt(Ceiling,          Tuning);

	// ── (a) ⭐⭐⭐ THE THREE ANCHORS. THIS IS THE WHOLE MODEL, AS THREE NUMBERS ────────────
	// ⛔ Each expectation is built with `Sqrt` from HIS ratio; the subject computes with
	// `Loge`/`Exp`. ⇒ agreement is a falsifiable coincidence, not a transcription.
	// ⛔⛔ AND THIS IS NOT THE BANNED √ TEST: the claim is three ABSOLUTE values, not the
	// relation `f(d/2) == √f(d)` (which is a theorem and could never fail).
	TestEqual(TEXT("(a) ⭐⭐ At the CEILING (20 ft): 0.9800 — his \"98% obscured at 20 feet\", derived as 1 − 0.02"),
		AtCeiling, ExpectedObscurationAtCeiling(), Tolerance);

	TestEqual(TEXT("(a) ⭐⭐ At HALF the ceiling (10 ft): 0.8586 — his \"it forces 86% at 10\", derived as 1 − √0.02"),
		AtHalf, ExpectedObscurationAtHalfTheCeiling(), Tolerance);

	TestEqual(TEXT("(a) ⭐ At a QUARTER of the ceiling (5 ft): 0.6239 — derived as 1 − √√0.02"),
		AtQuarter, ExpectedObscurationAtAQuarterOfTheCeiling(), Tolerance);

	// ⛔ σ derived from the ONSET instead of the ceiling — the single most plausible wrong
	// derivation — reads 0.9996 at the ceiling. This bound kills it without naming it.
	TestTrue(TEXT("(a) ⛔ The ceiling reads 0.98 and NOT ~0.9996 — a σ derived from the ONSET instead of the CEILING fails here"),
		AtCeiling < 0.99f);

	// ── (b) ⛔⛔ THE RETIRED `t²` CURVE, AS AN EXPLICIT RED CONTROL ──────────────────────
	// ⭐ Keeping the loser computable is what turns "we changed the curve" into a measurement.
	// Measured separations: 0.624 at 5 ft · 0.859 at 10 ft · 0.020 at 20 ft.
	TestTrue(TEXT("(b) ⛔⛔ At the ONSET the shipped curve and the RETIRED t² curve disagree by more than 0.5 — t² returned EXACTLY 0 here and this is the substance of his ruling"),
		FMath::Abs(AtHalf - RetiredQuadraticDensityAt(HalfOfCeiling)) > 0.5f);

	TestTrue(TEXT("(b) ⛔ …and at a quarter of the ceiling they disagree by more than 0.5 too — t² is still inside its clear bubble there"),
		FMath::Abs(AtQuarter - RetiredQuadraticDensityAt(QuarterOfCeiling)) > 0.5f);

	// ⛔ At the ceiling the two are only 0.02 apart — small, but 200× the tolerance, and it is
	// the row that kills the retired HARD CUT on the picture specifically.
	TestTrue(TEXT("(b) ⛔ At the ceiling the retired curve read EXACTLY 1.0 and the shipped one reads 0.98 — a difference of 0.02, which is 200× the tolerance"),
		FMath::Abs(AtCeiling - RetiredQuadraticDensityAt(Ceiling)) > Tolerance * 100.f);

	// ── (c) ⛔ LINEAR-IN-DISTANCE IS KILLED TOO, and it is a DIFFERENT curve from the old
	//     band-normalised linear the retired test spoke about ─────────────────────────────
	// `d / ceiling` reads 0.25 / 0.50 / 1.00 at these three anchors against 0.62 / 0.86 / 0.98.
	TestTrue(TEXT("(c) ⛔ NOT linear-in-distance: at a quarter of the ceiling `d/ceiling` reads 0.25 and the shipped curve reads 0.62"),
		AtQuarter > 0.5f);

	TestTrue(TEXT("(c) ⛔ …and NOT sqrt(d/ceiling) either, which reads 0.707 at half the ceiling against the shipped 0.8586"),
		AtHalf > 0.80f);

	// ── (d) ⭐⭐ CONCAVITY — THE POLARITY FLIP, STATED AS THE OPPOSITE OF WHAT SHIPPED ───
	// ⛔ THE OLD ROW: "the SECOND half of the band thickens MORE than the first (0.75 vs 0.25)".
	// ⛔ THE NEW ROW: the FIRST half thickens more. Measured 0.8586 vs 0.1214 — a 7.07× ratio.
	// ⇒ ⛔ a tree still carrying `t²` fails this by asserting the reverse, and LINEAR (which
	// splits any interval 50/50) and SMOOTHSTEP (symmetric about the midpoint) both fail it flat.
	const float GainedInFirstHalf  = AtHalf - 0.f;
	const float GainedInSecondHalf = AtCeiling - AtHalf;

	TestTrue(TEXT("(d) ⭐⭐ The FIRST half of the distance thickens MORE than the second — Beer-Lambert is CONCAVE. ⛔ The retired t² curve asserted the exact opposite, and LINEAR splits it 50/50 and FAILS"),
		GainedInFirstHalf > GainedInSecondHalf);

	TestTrue(TEXT("(d) ⛔ …and the deceleration is decisive, not marginal: the first half delivers at least twice the second (measured 7.07×)"),
		GainedInFirstHalf > GainedInSecondHalf * 2.f);

	// Four EQUAL steps of distance buy strictly DECREASING amounts of density. ⛔ The retired
	// test asserted strictly INCREASING; this is the same instrument with its sign reversed.
	const float StepOne   = AtQuarter - FSiegeFogStatics::FogDensityAt(0.f, Tuning);
	const float StepTwo   = AtHalf    - AtQuarter;
	const float StepThree = FSiegeFogStatics::FogDensityAt(ThreeQuarters, Tuning) - AtHalf;

	TestTrue(TEXT("(d) Equal steps of DISTANCE buy DECREASING amounts of density — first > second. ⛔ The retired curve made this ascending"),
		StepOne > StepTwo);

	TestTrue(TEXT("(d) …and second > third. ⛔ A LINEAR ramp makes all three equal and fails both rows"),
		StepTwo > StepThree);

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

	// ⛔⛔ THE ANTI-TRIVIALITY COUNTERS, **RE-DERIVED 2026-09-04 (TASK-981)** — the old three
	// categories were `== 0` / strictly-between / `== 1`, and TWO of them are now wrong for this
	// curve: Beer-Lambert is exactly 0 at ONE sample (the camera) and is NEVER exactly 1 inside
	// this sweep (it needs d > 16,201 uu before `exp` underflows). ⇒ the old
	// `OpaqueSamples > 0` row would go RED against a CORRECT implementation.
	//
	// ⭐⭐ THE REPLACEMENT IS STRONGER THAN WHAT IT REPLACES, and this is the point: the first
	// counter is the sweep-level embodiment of HIS RULING. Under the retired `t²` curve every
	// sample inside the onset was EXACTLY 0; under his ruling every one of them is positive.
	// ⇒ a tree that still carries a CLEAR BUBBLE fails the sweep test itself.
	int32 PositiveInsideTheOldOnset = 0; // 0 < d <= onset AND density > 0  — ⭐ the anti-bubble counter
	int32 LiveCurveSamples          = 0; // strictly between 0 and 1        — the curve under test
	int32 BeyondHisNinetyEight      = 0; // density >= 0.98                 — the far field

	const float Onset = Tuning.FogVisionOnsetUU;

	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Distance = static_cast<float>(Index) * StepUU;
		const float Density  = FSiegeFogStatics::FogDensityAt(Distance, Tuning);

		if (Distance > 0.f && Distance <= Onset && Density > 0.f) { ++PositiveInsideTheOldOnset; }
		if (Density > 0.f && Density < 1.f)                       { ++LiveCurveSamples;          }
		if (Density >= ExpectedObscurationAtCeiling())            { ++BeyondHisNinetyEight;      }

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

	// ── ⛔⛔ THE SWEEP MUST HAVE ACTUALLY EXERCISED THE CURVE ────────────────────────────
	// Without these rows the loop above is a test that proves nothing. Measured on the shipped
	// tuning: 50 · 200 · 99.
	TestTrue(TEXT("⛔⛔⭐ ANTI-TRIVIALITY (the row that encodes his ruling): at least 20 samples INSIDE the old onset carry STRICTLY POSITIVE density. ⛔ The retired t² curve returned EXACTLY 0 at every one of them, so a clear bubble FAILS here"),
		PositiveInsideTheOldOnset >= 20);

	TestTrue(TEXT("⛔⛔ ANTI-TRIVIALITY: at least 20 samples land strictly between 0 and 1 — a sweep that only ever saw a flat region would pass every other row and prove nothing"),
		LiveCurveSamples >= 20);

	TestTrue(TEXT("⛔ ANTI-TRIVIALITY: the sweep reached the far field — at least one sample at or beyond his 98%"),
		BeyondHisNinetyEight > 0);

	// ⛔ The sweep ends at twice the ceiling. ⭐ REPLACED: this used to assert EXACTLY 1.0, which
	// only a hard cut can satisfy. Beer-Lambert reads 0.99955 there — thicker than his ceiling
	// figure, and still strictly short of opaque, which is the asymptote asserted directly.
	TestTrue(TEXT("The sweep's far end (1200 uu — twice the ceiling) is thicker than his 98%…"),
		PreviousDensity > ExpectedObscurationAtCeiling());

	TestTrue(TEXT("⛔ …and STILL not fully opaque — Beer-Lambert approaches 1 and never arrives. A hard cut reads exactly 1.0 here and FAILS"),
		PreviousDensity < 1.f);

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

	// ── (c) ⭐⭐ THE ONSET IS **IGNORED** BY THE CURVE NOW — asserted, not assumed ────────
	// ⛔ REPLACED 2026-09-04 (TASK-981). These rows used to exercise the ZERO-WIDTH and INVERTED
	// BANDS, because the retired `t²` ramp normalised distance over `(ceiling − onset)` and could
	// divide by zero there. ⛔ Beer-Lambert has NO BAND: `FogDensityAt` does not read the onset at
	// all. ⇒ the old rows are not merely stale, they test a parameter the function stopped having.
	//
	// ⭐ THE REPLACEMENT IS THE STRONGER CLAIM, and it is `FOG-§9.2` item (6) as an assertion:
	// two tunings differing ONLY in their onset must produce a BIT-IDENTICAL curve.
	FSiegeFogTuning AbsurdOnset = ShippedTuning();
	AbsurdOnset.FogVisionOnsetUU = 5.f; // as far from 304.8 as anyone could plausibly type

	FSiegeFogTuning HugeOnset = ShippedTuning();
	HugeOnset.FogVisionOnsetUU = 5000.f; // inverted against the ceiling, which used to be a branch

	const float ProbeDistances[] = { 1.f, 120.f, DerivedOnsetUU, 457.2f, DerivedCeilingUU, 3000.f };

	for (const float Distance : ProbeDistances)
	{
		const float Shipped = FSiegeFogStatics::FogDensityAt(Distance, ShippedTuning());

		TestEqual(*FString::Printf(TEXT("(c) ⭐⭐ At %.1f uu a TINY onset (5) gives a BIT-IDENTICAL density — the onset is not read by the curve any more"), Distance),
			FSiegeFogStatics::FogDensityAt(Distance, AbsurdOnset), Shipped, Exact);

		TestEqual(*FString::Printf(TEXT("(c) ⭐⭐ …and so does a HUGE, INVERTED onset (5000 > the ceiling) at %.1f uu. ⛔ The retired ramp branched on exactly this and produced a hard step"), Distance),
			FSiegeFogStatics::FogDensityAt(Distance, HugeOnset), Shipped, Exact);
	}

	// ⛔⛔ …AND THE ONSET'S SURVIVING ROLE IS STILL LIVE, so nobody reads the rows above as
	// licence to delete the constant: it is the CEILING's `ClampMin` floor (FOG-§7b half (a)).
	TestEqual(TEXT("(c) ⛔ The onset KEEPS ITS VALUE — it lost a role, not its existence. FOG-§7b still uses it as the ceiling's slider floor"),
		ShippedTuning().FogVisionOnsetUU, DerivedOnsetUU, Tolerance);

	// ── (c2) ⛔⛔ THE NEW DIVIDE-BY-ZERO: THE CEILING IS σ's DENOMINATOR ─────────────────
	// ⛔ THIS HAZARD DID NOT EXIST UNDER THE OLD CURVE, which divided by `(ceiling − onset)` and
	// was protected by the `Ceiling <= Onset` branch. Beer-Lambert divides by the ceiling itself.
	// ⭐ Degenerate ⇒ CLEAR, never a whiteout — the module's own totality law.
	FSiegeFogTuning ZeroCeilingTuning = ShippedTuning();
	ZeroCeilingTuning.FogVisionCeilingUU = 0.f;

	TestEqual(TEXT("(c2) ⛔⛔ A ZERO ceiling yields EXACTLY CLEAR — it is σ's DENOMINATOR now, and an unguarded divide would hand the renderer a NaN or an infinite σ (a total whiteout)"),
		FSiegeFogStatics::FogDensityAt(400.f, ZeroCeilingTuning), 0.f, Exact);

	FSiegeFogTuning NegativeCeilingCurve = ShippedTuning();
	NegativeCeilingCurve.FogVisionCeilingUU = -DerivedCeilingUU;

	TestEqual(TEXT("(c2) ⛔ A NEGATIVE ceiling yields EXACTLY CLEAR — a negative σ would make the world get CLEARER with distance"),
		FSiegeFogStatics::FogDensityAt(400.f, NegativeCeilingCurve), 0.f, Exact);

	FSiegeFogTuning NaNCeilingCurve = ShippedTuning();
	NaNCeilingCurve.FogVisionCeilingUU = NaN;

	TestEqual(TEXT("(c2) ⛔ A NaN ceiling yields EXACTLY CLEAR"),
		FSiegeFogStatics::FogDensityAt(400.f, NaNCeilingCurve), 0.f, Exact);

	// ── (d) ⛔⛔ THE TRANSMITTANCE'S DEGENERATE VALUES FAIL TOWARD **CLEAR** ─────────────
	// ⛔ REPLACED 2026-09-04: these rows used to guard `FogDensityExponent`, which is RETIRED.
	// ⭐ The failure they now guard is WORSE than the one they replaced. `Pow(t, 0) == 1` painted
	// a wall of fog inside the BAND; here a transmittance of 0 makes σ INFINITE, which paints an
	// opaque screen AT THE CAMERA — the failure nobody could diagnose from a screenshot.
	const float DegenerateTransmittances[] = {
		0.f,        // σ = +infinity  ⇒ total whiteout if unguarded
		-0.5f,      // log of a negative ⇒ NaN
		1.f,        // σ = 0          ⇒ no fog (degenerate but harmless)
		2.f,        // σ < 0          ⇒ the world would get CLEARER with distance
		NaN,
		Infinity
	};

	for (const float BadTransmittance : DegenerateTransmittances)
	{
		FSiegeFogTuning BadTuning = ShippedTuning();
		BadTuning.FogTransmittanceAtCeiling = BadTransmittance;

		// ⭐ Probed AT THE CEILING, i.e. where a broken σ does the most damage.
		TestEqual(*FString::Printf(TEXT("(d) ⛔⛔ A degenerate transmittance (%f) yields EXACTLY CLEAR at the ceiling — degenerate inputs fail toward NO FOG, ⛔ never toward a whiteout"), BadTransmittance),
			FSiegeFogStatics::FogDensityAt(DerivedCeilingUU, BadTuning), 0.f, Exact);

		// …and at the camera too, which is the pixel a whiteout would ruin first.
		TestEqual(*FString::Printf(TEXT("(d) ⛔ …and EXACTLY CLEAR one uu from the camera under the same tuning (%f)"), BadTransmittance),
			FSiegeFogStatics::FogDensityAt(1.f, BadTuning), 0.f, Exact);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  10. ⭐⭐⭐ σ IS **DERIVED FROM THE TUNABLES**, NOT HARDCODED — the row that kills
//      the single most plausible wrong implementation
//
//  ⛔ NEW 2026-09-04 (TASK-981; ruling `FOG-§9.2`). It is the structural replacement
//  for the retired test 3(d) ("retuning the exponent to 1.0 really does yield
//  linear"), which proved the same thing about the tunable that just died.
//
//  ⛔⛔ WHY IT IS NEEDED AND IS NOT CEREMONY: every absolute anchor in tests 2 and 3
//  is satisfied *equally well* by `Sigma = 0.0064174f` typed as a literal. ⛔ Such an
//  implementation is correct today, passes the whole suite, and silently DECOUPLES the
//  picture from both `EditDefaultsOnly` knobs — so Jonathan's "one-word retune" (the
//  entire argument for `J-F2`) would stop working with nothing going red.
//  ⇒ ⭐ this test moves the tunables and demands the curve move with them.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogSigmaIsDerivedTest,
	"Siegebound.Fog.TheExtinctionCoefficientIsDerivedFromTheTunablesAndIsNotHardcoded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogSigmaIsDerivedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogTestFixture;

	// ── (a) RETUNE THE TRANSMITTANCE — the ceiling's obscuration must follow it exactly ──
	// ⛔ `0.5` is a PROBE VALUE, not a design number: the shipped tuning is `0.02` and this
	// tunable has exactly one shipped value, so there is nothing for it to collide with
	// (`SC-§40` cl. 10). ⭐ It is chosen because 1 − 0.5 is exact in binary32, so the expectation
	// carries no rounding of its own.
	FSiegeFogTuning HalfTransmittance = ShippedTuning();
	HalfTransmittance.FogTransmittanceAtCeiling = 0.5f;

	TestEqual(TEXT("(a) ⭐⭐ With the transmittance retuned to 0.5, the CEILING reads exactly 0.50 obscured. ⛔ A HARDCODED σ still reads 0.98 here and FAILS"),
		FSiegeFogStatics::FogDensityAt(DerivedCeilingUU, HalfTransmittance), 0.5f, Tolerance);

	// …and the √ structure follows the new σ too: at half the ceiling, 1 − √0.5 = 0.2929.
	// ⛔ NOT a test of the √ theorem — it is a second ABSOLUTE value under a DIFFERENT σ, which
	// is what proves the derivation rather than the algebra.
	TestEqual(TEXT("(a) ⭐ …and half the ceiling reads 1 − √0.5 = 0.2929 under that same retune — a SECOND absolute value under a DIFFERENT σ"),
		FSiegeFogStatics::FogDensityAt(DerivedCeilingUU * 0.5f, HalfTransmittance), 1.f - FMath::Sqrt(0.5f), Tolerance);

	// ── (b) RETUNE THE CEILING — σ's anchor moves, so the whole curve stretches ──────────
	// ⛔ Doubling the ceiling must halve σ. ⇒ the OLD ceiling distance now reads what HALF the
	// ceiling used to read (1 − √0.02 = 0.8586), and the NEW ceiling reads his 0.98.
	// ⭐ Derived from the fixture (`× 2`), never typed — a ceiling retune cannot stale it.
	FSiegeFogTuning DoubledCeiling = ShippedTuning();
	DoubledCeiling.FogVisionCeilingUU = DerivedCeilingUU * 2.f;

	TestEqual(TEXT("(b) ⭐⭐ With the ceiling DOUBLED, the NEW ceiling reads his 0.98 — the anchor travelled with the tunable"),
		FSiegeFogStatics::FogDensityAt(DerivedCeilingUU * 2.f, DoubledCeiling), ExpectedObscurationAtCeiling(), Tolerance);

	TestEqual(TEXT("(b) ⭐⭐ …and the OLD ceiling distance now reads 0.8586, because it is exactly HALF the new ceiling. ⛔ A HARDCODED σ reads 0.98 here and FAILS"),
		FSiegeFogStatics::FogDensityAt(DerivedCeilingUU, DoubledCeiling), ExpectedObscurationAtHalfTheCeiling(), Tolerance);

	// ── (c) THE TIGHTEST LEGAL CEILING — FOG-§7b's own ClampMin floor ───────────────────
	// ⭐ Derived from the ONSET rather than typed, which is exactly what `FOG-§7b` half (a) made
	// the slider's floor. ⇒ this row also proves the floor is a usable fog rather than a
	// degenerate one: at the floor, the ceiling still reads his 98%.
	FSiegeFogTuning TightestCeiling = ShippedTuning();
	TightestCeiling.FogVisionCeilingUU = DerivedOnsetUU;

	TestEqual(TEXT("(c) ⭐ At FOG-§7b's tightest legal ceiling (the onset's own 304.8) that distance reads his 0.98 — the slider floor is still a real fog, not a degenerate one"),
		FSiegeFogStatics::FogDensityAt(DerivedOnsetUU, TightestCeiling), ExpectedObscurationAtCeiling(), Tolerance);

	// ── (d) ⛔⛔ THE MECHANIC IS UNMOVED BY ANY OF IT — the picture and the clamp are
	//     SEPARATE, and this row is what stops a future "harmonisation" ──────────────────
	// ⛔ Retuning the LOOK must not touch acquisition. `EffectiveVisionRadius` reads the ceiling
	// and NEVER the transmittance, so the 0.5-transmittance tuning above must clamp identically.
	TestEqual(TEXT("(d) ⛔⛔ Retuning the TRANSMITTANCE does not move the CLAMP by one ulp — the look and the mechanic are separate surfaces and nobody harmonises them"),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/true, HalfTransmittance),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/true, ShippedTuning()), Exact);

	// ⛔ …whereas retuning the CEILING moves the clamp, because the ceiling IS the mechanic's
	// own number. This is the positive control for the row above: it proves that comparison
	// could have detected a difference at all.
	TestEqual(TEXT("(d) ⭐ POSITIVE CONTROL: retuning the CEILING *does* move the clamp — so the row above is a real measurement, not a comparison that could never differ"),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/true, DoubledCeiling), DerivedCeilingUU * 2.f, Tolerance);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
