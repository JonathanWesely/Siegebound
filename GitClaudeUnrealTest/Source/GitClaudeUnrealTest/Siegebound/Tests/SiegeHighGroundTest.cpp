// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Math/UnrealMathUtility.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SummonedUnit.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for HIGH GROUND — the elevation damage bonus (TASK-724, HIGH-§) ═══
 *
 *  Jonathan, verbatim (2026-08-30): "make their attacks deal more damage the higher elevation
 *  they are. I would say that for every 5 feet that their elevation increases, their damage
 *  multiplier increases by 10%."
 *
 *  Subject: `ASummonedUnit::HeightAdvantageMultiplier` — the `HIGH-§3` pinned pure seam — plus
 *  the two shipped `EditDefaultsOnly` tunables it is fed from. QA gate: TASK-730. Compile +
 *  suite run + `DT_Cards` reimport: TASK-731.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE TRAP THIS WHOLE FILE IS POINTED AT: **`5 FEET` IS NOT `5` UNREAL UNITS**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Unreal is CENTIMETRES (1 uu = 1 cm) and 1 ft = 30.48 cm exactly, so his step is
 *  **5 × 30.48 = 152.4 uu**. A `5` there would be wrong by **30×** — every unit a god — and it
 *  would look entirely plausible in review, which is precisely why it gets a test rather than
 *  a promise.
 *
 *  ⛔⛔ **AND THAT IS WHY `152.4` APPEARS NOWHERE IN THIS FILE AS A CODE LITERAL** (only in
 *  prose, where it is explaining the derivation rather than standing in for it). Every expectation
 *  below is built from `FeetPerStep × CentimetresPerFoot`, i.e. RE-DERIVED from the
 *  international definition of the foot and from the number in Jonathan's own sentence —
 *  ⛔ never transcribed out of the header it is asserting. ⚖️ **A test copied from its subject
 *  agrees with its subject by construction and reports SAFE for ever** (`SHIP-§9c`); this one
 *  disagrees with a wrong header the moment the header is wrong. `HIGH-§1` also forbids a
 *  second shipped literal, and this file honours that from the outside: there is exactly ONE
 *  `152.4` in the codebase and it is `ASummonedUnit::HeightBonusStepUU`.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHAT MAKES THIS FILE POSSIBLE — THE PURITY `HIGH-§3` SHIPPED ON PURPOSE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `HeightAdvantageMultiplier` is FOUR FLOATS IN, ONE FLOAT OUT: no `UWorld`, no `AActor`, no
 *  `UObject`, no allocation, no clock read, no RNG (the `WM-§2` / `HeightToBrightness`
 *  precedent — a testability obligation gets a testability seam). ⇒ the entire elevation rule
 *  runs in-process with NO PIE session. ⚠️ If a test here ever starts needing a world, the
 *  purity `HIGH-§3` calls load-bearing has been broken, and that is a FINDING rather than a
 *  reason to add a fixture.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — STATED SO NOBODY MISTAKES GREEN FOR DONE
 *  (`SC-§32`: a mechanism never observed to function is not known to function)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔ **THE COMPOSE SITE ITSELF.** `ASummonedUnit::ComputeOutputDamage` is `private`, is
 *      non-`const`, consumes the primed charge, and reads `GetActorLocation()` on two live
 *      actors. There is NO headless path to it and ⛔ this file does not pretend otherwise —
 *      ⚠️ inventing an accessor purely so a test could reach it would add shipped surface to
 *      make a test possible, which is the tail wagging the dog. **Its instruments are
 *      TASK-730's diff read (gate question 3: one compose point, gated on `bRangedAttack`,
 *      melee still bit-for-bit) and Jonathan's playtest.** What IS asserted here is the
 *      arithmetic that site multiplies by, and the STRUCTURAL facts that keep the hero and
 *      every melee unit out of it (test 9).
 *    • ⛔ **THE DATA.** Which cards carry `bRanged = true` lives in `Docs/Data/cards.csv` and
 *      is TASK-723's; it is INERT until `/Game/Data/DT_Cards` is reimported (TASK-731's editor
 *      step). This file asserts the C++ default of the gate flag, ⛔ not the shipped roster.
 *    • ⛔ **NOTHING HERE RUNS THE MODEL.** No inference, no `Capture()`, no `EnsureSnapshot()`.
 *      🔒 The one-shot latch is untouched and unspent by this file, Zone A is not read, and
 *      ⛔ no token figure appears anywhere in it (`AS-§12g`).
 */

namespace SiegeHighGroundTestFixture
{
	/**
	 *  ⭐ THE CONVERSION, RE-DERIVED FROM ITS DEFINITION RATHER THAN COPIED FROM THE HEADER.
	 *  `1 ft = 30.48 cm` is the international definition of the foot (exact, since 1959), and
	 *  `5` is the number in Jonathan's sentence. Their product is the step the code must ship.
	 *  ⛔ Deliberately NOT written as `152.4f`: an expectation transcribed from the subject
	 *  cannot disagree with a wrong subject.
	 */
	constexpr float CentimetresPerFoot = 30.48f;
	constexpr float FeetPerStep = 5.f;
	constexpr float StepUU = FeetPerStep * CentimetresPerFoot; // = 152.4 uu

	/** His "+10%", as the plain additive fraction the law pins. */
	constexpr float BonusPerStep = 0.10f;

	/**
	 *  Tolerance for the decimal expectations quoted out of `HIGH-§5`'s table. Tight enough
	 *  that every wrong step in the trap list (150 / 152 / 5 / 500) blows straight through it,
	 *  loose enough that float32 rounding never does.
	 */
	constexpr float Tolerance = 1.e-4f;

	/** Exact-equality tolerance, for the claims whose whole content is the word EXACTLY. */
	constexpr float Exact = 0.f;

	/** The seam, at the shipped tunables — spelled once so no test re-types the arguments. */
	static float MultiplierAt(float AttackerZ, float TargetZ)
	{
		return ASummonedUnit::HeightAdvantageMultiplier(AttackerZ, TargetZ, StepUU, BonusPerStep);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. ⛔ THE POSITIVE-ONLY RULE — level ground and shooting UPWARD are EXACTLY
//     1.0, and there is NO low-ground malus (HIGH-§2, his row R-1)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundPositiveOnlyTest,
	"Siegebound.HighGround.LevelGroundAndUphillShotsAreExactlyOneWithNoMalus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundPositiveOnlyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ── (a) LEVEL GROUND is exactly 1.0 — the overwhelmingly common case ─────────────────
	// EXACT, not near-exact: FMath::Max(0, 0) is exactly 0, so the whole bonus term vanishes
	// and an ordinary flat-field archer's damage must be untouched to the last bit. A rule
	// that quietly returned 1.0000001 on flat ground would be a balance change to every
	// existing engagement in the game.
	TestEqual(TEXT("(a) Attacker and target at the SAME Z ⇒ EXACTLY 1.0"),
		MultiplierAt(0.f, 0.f), 1.f, Exact);

	TestEqual(TEXT("(a) …and still exactly 1.0 on a lifted shared floor — the rule is anchored on the DIFFERENCE, never on Z=0"),
		MultiplierAt(1200.f, 1200.f), 1.f, Exact);

	// ── (b) ⛔ SHOOTING UPWARD IS EXACTLY 1.0 — NEVER A PENALTY ──────────────────────────
	// He asked for a bonus. Inventing a malus is inventing a mechanic he never requested, and
	// it would silently nerf every unit that ever stands in a dip. This assertion FAILS
	// against a symmetric formula (one that dropped the max(0, ·) and let ΔZ go negative),
	// which is the single most likely way to get this wrong.
	const float UphillDepths[] = { -1.f, -StepUU, -1000.f, -2200.f, -8000.f };

	for (const float Depth : UphillDepths)
	{
		const float Multiplier = MultiplierAt(0.f, -Depth); // target ABOVE the attacker

		TestEqual(*FString::Printf(TEXT("(b) Shooting UP at a target %.0f uu above ⇒ EXACTLY 1.0, never a malus"), -Depth),
			Multiplier, 1.f, Exact);

		TestTrue(*FString::Printf(TEXT("(b) …and it is never BELOW 1.0 (a low-ground penalty is a mechanic nobody asked for) at depth %.0f"), -Depth),
			Multiplier >= 1.f);
	}

	// ── (c) The very first unit above the target already earns something ─────────────────
	// The boundary is at ΔZ = 0 and it is open on the high side: one centimetre of advantage
	// is a positive bonus, not a rounded-away zero. This is what "continuous" means at the
	// bottom end, and it fails against any implementation that floors the step count.
	TestTrue(TEXT("(c) ⭐ ONE uu of advantage is already a bonus — the rule has no dead band above zero"),
		MultiplierAt(1.f, 0.f) > 1.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⭐⛔⛔ THE CONVERSION AT THE SEAM — 152.4 uu buys +10%, and 5 uu does NOT
//     (the feet-as-units trap, HIGH-§1)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundStepIsTenPercentTest,
	"Siegebound.HighGround.OneFiveFootStepInUnrealUnitsIsExactlyTenPercent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundStepIsTenPercentTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ── (a) His sentence, as arithmetic: one step of advantage ⇒ ×1.10 ───────────────────
	TestEqual(TEXT("(a) ⭐ ONE step (5 ft = 152.4 uu) of advantage ⇒ ×1.10 — Jonathan's sentence, verbatim"),
		MultiplierAt(StepUU, 0.f), 1.10f, Tolerance);

	TestEqual(TEXT("(a) TWO steps ⇒ ×1.20"),
		MultiplierAt(2.f * StepUU, 0.f), 1.20f, Tolerance);

	TestEqual(TEXT("(a) TEN steps (50 ft) ⇒ ×2.00 — his rule doubles damage at fifty feet"),
		MultiplierAt(10.f * StepUU, 0.f), 2.00f, Tolerance);

	// ── (b) ⭐⛔⛔ THE 30× TRAP, AT THE FORMULA'S END OF IT ────────────────────────────────
	// ⚠️ WHICH HALF OF THE TRAP THIS CATCHES, SAID PRECISELY: the shipped CONSTANT is test 8's
	// business; what this pins is that the FORMULA really divides the rise by a step, so five
	// UNITS is 3.3% of one step and buys 0.33%. ⇒ it fails against a formula that multiplied
	// the raw height by the bonus and skipped the division (which would read ×1.50 here) —
	// i.e. against the arithmetic shape a feet-as-units mistake actually produces.
	const float FiveUnitRise = MultiplierAt(5.f, 0.f);

	TestEqual(TEXT("(b) ⭐ A 5-UU rise (5 cm — a kerb) is worth ×1.0033, ⛔ NOT ×1.10 — feet are not units"),
		FiveUnitRise, 1.0032808f, Tolerance);

	TestTrue(TEXT("(b) ⛔⛔ A 5-uu rise is nowhere near +10% — a formula that skipped the divide-by-step would read ×1.50 here and every unit on the field would be a god"),
		FiveUnitRise < 1.01f);

	// ⛔ THE OBVIOUS THIRD CLAUSE — "and StepUU is between 100 and 200 uu" — IS DELIBERATELY
	// NOT WRITTEN HERE. `StepUU` is this file's OWN constant, so a band check on it asserts
	// the compiler's multiplication rather than the game's behaviour and can never fail. The
	// same band, aimed at the SHIPPED value where it can fail, lives in test 8.

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⭐ THE CURVE IS CONTINUOUS, ⛔ NOT STEPPED (HIGH-§2, his row R-4)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundContinuousNotSteppedTest,
	"Siegebound.HighGround.TheCurveIsContinuousAndNeverStepped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundContinuousNotSteppedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ⚖️ BOTH READINGS OF HIS ENGLISH ARE DEFENSIBLE AND THE LAW CHOSE CONTINUOUS: a stepped
	// rule puts invisible breakpoints on a hillside that a player cannot see, cannot aim for
	// and cannot learn, and a 1-uu step flipping damage by 10% is worse feel and harder to
	// test. ⭐ EVERY ASSERTION BELOW FAILS AGAINST A FLOORED IMPLEMENTATION — that is the
	// whole point of the test, and it is why fractional-step points are used rather than the
	// whole-step points test 2 already owns (those two agree under BOTH readings and would
	// therefore prove nothing here).

	// ── (a) The fractional points a stepped rule would flatten to 1.00 / 1.10 ────────────
	TestEqual(TEXT("(a) HALF a step ⇒ ×1.05 — a stepped rule would return exactly 1.00 here"),
		MultiplierAt(0.5f * StepUU, 0.f), 1.05f, Tolerance);

	TestEqual(TEXT("(a) A TENTH of a step ⇒ ×1.01 — a stepped rule would return exactly 1.00 here"),
		MultiplierAt(0.1f * StepUU, 0.f), 1.01f, Tolerance);

	TestEqual(TEXT("(a) NINE TENTHS of a step ⇒ ×1.09, ⛔ not 1.00 — the last inch before a rung still counts"),
		MultiplierAt(0.9f * StepUU, 0.f), 1.09f, Tolerance);

	TestEqual(TEXT("(a) ONE AND A HALF steps ⇒ ×1.15 — a stepped rule would return exactly 1.10 here"),
		MultiplierAt(1.5f * StepUU, 0.f), 1.15f, Tolerance);

	// ── (b) STRICTLY increasing across a fine sweep — no plateaus anywhere ───────────────
	// A floored implementation is FLAT between rungs, so a strict-increase claim sampled at
	// sub-step spacing fails on the very first pair inside a rung. The sweep runs from level
	// ground to three steps at 1/40th-of-a-step spacing, i.e. ~3.8 uu apart — far finer than
	// any rung a stepped reading could have used.
	{
		float PreviousMultiplier = MultiplierAt(0.f, 0.f);

		for (int32 Sample = 1; Sample <= 120; ++Sample)
		{
			const float Advantage = (static_cast<float>(Sample) / 40.f) * StepUU;
			const float Multiplier = MultiplierAt(Advantage, 0.f);

			TestTrue(*FString::Printf(TEXT("(b) STRICTLY increases at ΔZ=%.1f uu — a stepped rule plateaus here and this claim catches it"), Advantage),
				Multiplier > PreviousMultiplier);

			PreviousMultiplier = Multiplier;
		}
	}

	// ── (c) Equal height differences buy equal bonus ANYWHERE on the curve ───────────────
	// The increment from 0.25 to 0.75 of a step must equal the increment from 5.25 to 5.75 —
	// true of a straight line, false of a staircase (the first pair straddles no rung, the
	// second straddles none either, but a staircase makes both zero while the line makes both
	// +5%). Asserted as a positive, non-zero, equal pair so a staircase cannot satisfy it.
	{
		const float LowIncrement = MultiplierAt(0.75f * StepUU, 0.f) - MultiplierAt(0.25f * StepUU, 0.f);
		const float HighIncrement = MultiplierAt(5.75f * StepUU, 0.f) - MultiplierAt(5.25f * StepUU, 0.f);

		TestEqual(TEXT("(c) A half-step of extra height buys +5% low on the curve"), LowIncrement, 0.05f, Tolerance);
		TestEqual(TEXT("(c) …and the SAME +5% high on the curve — equal heights buy equal bonus everywhere"), HighIncrement, 0.05f, Tolerance);
		TestTrue(TEXT("(c) ⛔ …and neither increment is ZERO, so a flat staircase cannot pass this pair by returning 0 twice"),
			LowIncrement > 0.f && HighIncrement > 0.f);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ⭐ THE BONUS IS LINEAR IN HEIGHT AND ⛔ NEVER COMPOUNDS
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundLinearNotCompoundingTest,
	"Siegebound.HighGround.TheBonusIsLinearInHeightAndNeverCompounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundLinearNotCompoundingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ⚠️ THE CLAIM IS ABOUT THE **BONUS**, ⛔ NOT ABOUT THE MULTIPLIER. Double the height
	// doubles the BONUS (0.10 → 0.20); it does NOT double the multiplier (1.10 → 2.20). Stated
	// this way because the sloppy version of this sentence is how a linearity test ends up
	// asserting something false and getting "fixed" into asserting nothing.

	// ── (a) bonus(2h) == 2 × bonus(h), at several heights ────────────────────────────────
	const float Heights[] = { 0.3f * StepUU, StepUU, 3.7f * StepUU, 1000.f };

	for (const float Height : Heights)
	{
		const float SingleBonus = MultiplierAt(Height, 0.f) - 1.f;
		const float DoubleBonus = MultiplierAt(2.f * Height, 0.f) - 1.f;

		TestEqual(*FString::Printf(TEXT("(a) Doubling ΔZ=%.1f uu exactly DOUBLES the bonus"), Height),
			DoubleBonus, 2.f * SingleBonus, Tolerance);

		TestTrue(*FString::Printf(TEXT("(a) …and the bonus at ΔZ=%.1f is strictly positive, so the doubling claim is not satisfied by two zeroes"), Height),
			SingleBonus > 0.f);
	}

	// ── (b) ⛔ COMPOUNDING IS REFUTED EXPLICITLY, WHERE THE TWO READINGS DIVERGE ──────────
	// A multiplicative reading (×1.10 PER step, i.e. pow(1.1, n)) agrees with the additive one
	// at exactly one step and diverges after: 1.21 vs 1.20 at two steps, 2.594 vs 2.000 at ten.
	// ⭐ These assertions are the reason test 2's whole-step points are not sufficient on their
	// own — they are the ones a compounding implementation cannot pass.
	TestEqual(TEXT("(b) TWO steps is ×1.20 — ⛔ NOT the ×1.21 a compounding reading would give"),
		MultiplierAt(2.f * StepUU, 0.f), 1.20f, Tolerance);

	TestTrue(TEXT("(b) ⛔ …and it is measurably BELOW 1.21, so pow(1.1, n) fails here"),
		MultiplierAt(2.f * StepUU, 0.f) < 1.205f);

	TestEqual(TEXT("(b) TEN steps is ×2.00 — ⛔ NOT the ×2.594 a compounding reading would give"),
		MultiplierAt(10.f * StepUU, 0.f), 2.00f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. ⚠️ THE HIGH-§5 WORST-CASE TABLE — the figures handed to Jonathan, asserted
//     where they live, and ⛔ NO CAP
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundWorstCaseTableTest,
	"Siegebound.HighGround.WorstCaseMultipliersMatchTheLawTableAndAreUncapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundWorstCaseTableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ⭐ THESE DECIMALS COME OUT OF `HIGH-§5`'s TABLE, ⛔ NOT out of a re-run of the formula.
	// That is what makes them a cross-check: the law's arithmetic and the shipped code are two
	// independent derivations, and this test is where they are made to agree. If a future edit
	// changes the curve, the numbers Jonathan was given stop matching the game he plays, and
	// this test is what says so.

	// ── (a) ~1,000 uu — the tallest shipped hill crown at max scatter scale ──────────────
	TestEqual(TEXT("(a) A 1,000-uu hill crown over a valley target ⇒ ×1.6562 (+65.6%)"),
		MultiplierAt(1000.f, 0.f), 1.656168f, Tolerance);

	// ── (b) ~1,200 uu — TOWER-§'s proposed platform, on flat ground ──────────────────────
	TestEqual(TEXT("(b) A 1,200-uu tower platform on flat ground ⇒ ×1.7874 (+78.7%)"),
		MultiplierAt(1200.f, 0.f), 1.787402f, Tolerance);

	// ── (c) ⚠️ ~2,200 uu — THE ONE TO ACTUALLY WATCH: the tower ON a tall hill ───────────
	// A Longbowman there deals 18 × 2.44 = 43.9 per shot at 3,600 range, against a target that
	// — if it is another Longbowman on flat ground — cannot reach it at all. ⭐ That is the
	// stacking case, it was stated to Jonathan BEFORE he played it, and it is his to cap or
	// keep. This assertion is what keeps the figure he was quoted honest.
	TestEqual(TEXT("(c) ⚠️ A 1,200-uu tower on a 1,000-uu hill, firing into a valley ⇒ ×2.4436 (+144%) — the stacking case"),
		MultiplierAt(2200.f, 0.f), 2.443570f, Tolerance);

	// ── (d) ~8,000 uu — a castle shell; theoretical, no unit can stand there today ───────
	TestEqual(TEXT("(d) A castle-shell height (8,000 uu) ⇒ ×6.2493 — theoretical today, and it is NOT clamped away"),
		MultiplierAt(8000.f, 0.f), 6.249344f, Tolerance);

	// ── (e) ⛔⛔ NO CAP — HE DID NOT ASK FOR ONE (his row R-3) ────────────────────────────
	// Inventing a ceiling he never requested is silently softening his number, which is the
	// same sin as not tripling the range. ⭐ These claims FAIL against any invented clamp: a
	// cap at 2.0 breaks the first, and a cap at any finite value breaks the second at some
	// height, so the growth claim is checked far past every plausible clamp.
	TestTrue(TEXT("(e) ⛔ The 8,000-uu case is NOT clamped to some invented ceiling — it really is above ×6"),
		MultiplierAt(8000.f, 0.f) > 6.f);

	{
		const float FarAbove = MultiplierAt(100000.f, 0.f);

		TestTrue(TEXT("(e) ⛔ …and the curve is still climbing at 100,000 uu — an invented cap of ANY finite value fails here"),
			FarAbove > MultiplierAt(50000.f, 0.f));

		TestTrue(TEXT("(e) …while staying a finite number, never an overflow"),
			FMath::IsFinite(FarAbove));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⛔ A DEGENERATE STEP IS TOTAL — never a divide by zero, never a NaN in a
//     damage number
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundDegenerateStepTest,
	"Siegebound.HighGround.ADegenerateStepIsTotalAndNeverDividesByZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundDegenerateStepTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// `HeightBonusStepUU` is an EditDefaultsOnly float a designer can zero from a Blueprint
	// default, and an unguarded divide would put inf or NaN straight into a damage number —
	// which then propagates into HP and never comes back out. The seam guards BEFORE it
	// divides, and a disabled step means NO bonus rather than an explosion.

	// ── (a) A ZERO step ⇒ exactly 1.0, finite ────────────────────────────────────────────
	{
		const float ZeroStep = ASummonedUnit::HeightAdvantageMultiplier(2200.f, 0.f, 0.f, BonusPerStep);

		TestTrue(TEXT("(a) A ZERO step yields a FINITE multiplier — never inf, never NaN"), FMath::IsFinite(ZeroStep));
		TestEqual(TEXT("(a) …and it is EXACTLY 1.0 — a disabled step disables the bonus, it does not explode"), ZeroStep, 1.f, Exact);
	}

	// ── (b) A NEGATIVE step ⇒ exactly 1.0 (⛔ never a negative bonus) ─────────────────────
	// Without the guard this would return 1 − 1.44 = −0.44, i.e. a NEGATIVE damage multiplier,
	// which downstream reads as healing the target you are shooting at.
	{
		const float NegativeStep = ASummonedUnit::HeightAdvantageMultiplier(2200.f, 0.f, -StepUU, BonusPerStep);

		TestTrue(TEXT("(b) A NEGATIVE step yields a FINITE multiplier"), FMath::IsFinite(NegativeStep));
		TestEqual(TEXT("(b) …and EXACTLY 1.0 — ⛔ never the negative multiplier an unguarded divide would produce"), NegativeStep, 1.f, Exact);
	}

	// ── (c) A ZERO bonus-per-step disables the feature cleanly ───────────────────────────
	TestEqual(TEXT("(c) A ZERO bonus-per-step ⇒ exactly 1.0 at any height — the feature switches off without a code change"),
		ASummonedUnit::HeightAdvantageMultiplier(2200.f, 0.f, StepUU, 0.f), 1.f, Exact);

	// ── (d) The guard does not eat the ordinary case ─────────────────────────────────────
	// ⚠️ A guard written as `StepUU < 0` (or one that swallowed everything) would make every
	// assertion above pass while deleting the entire feature. This is the claim that stops the
	// degenerate test from being satisfiable by a function that just returns 1.0.
	TestTrue(TEXT("(d) ⛔ …and a HEALTHY step still produces a real bonus — the guard cannot be a blanket 'return 1.0'"),
		MultiplierAt(2200.f, 0.f) > 2.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. ⭐⛔ HEIGHT ADVANTAGE, ⛔ NOT ABSOLUTE WORLD Z — so a HILL and a TOWER at the
//     same height deal IDENTICAL damage (HIGH-§2 row R-1, HIGH-§3)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundHillAndTowerAgreeTest,
	"Siegebound.HighGround.OnlyHeightAdvantageMattersSoAHillAndATowerAgree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundHillAndTowerAgreeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ⭐⭐ WHY THIS TEST IS BUILT THE WAY IT IS. "A hill and a tower agree" is trivially true
	// of any function of ΔZ, so asserting it with two IDENTICAL calls would prove nothing —
	// the two sides would be equal by construction and the test would report SAFE for ever.
	// ⇒ EVERY PAIR BELOW IS CONSTRUCTED AT A **DIFFERENT ABSOLUTE Z**, so the claim being made
	// is the one with real content: THE REFUSED READING (b) — absolute world Z — PRODUCES
	// DIFFERENT ANSWERS FOR THESE PAIRS AND FAILS EVERY ONE OF THEM.

	// ── (a) A hill in a valley vs a tower on a highland shelf — same ΔZ, different Z ──────
	// HILL:  ground at Z=0, crown 1,200 up ⇒ attacker 1,200, target 0.
	// TOWER: a 900-uu highland shelf, platform 1,200 up ⇒ attacker 2,100, target 900.
	// Same advantage, absolute heights 900 uu apart. Under absolute Z these read ×1.787 and
	// ×2.378 — the tower would out-damage the hill for standing on higher ground while having
	// no more advantage over its target, and `TOWER-§` would stop working for free.
	{
		constexpr float ValleyGroundZ = 0.f;
		constexpr float HillCrownHeight = 1200.f;
		constexpr float HighlandGroundZ = 900.f;
		constexpr float TowerPlatformHeight = 1200.f;

		const float HillMultiplier = MultiplierAt(ValleyGroundZ + HillCrownHeight, ValleyGroundZ);
		const float TowerMultiplier = MultiplierAt(HighlandGroundZ + TowerPlatformHeight, HighlandGroundZ);

		TestEqual(TEXT("(a) ⭐ A 1,200-uu HILL and a 1,200-uu TOWER at DIFFERENT world heights deal IDENTICAL damage"),
			TowerMultiplier, HillMultiplier, Tolerance);

		TestTrue(TEXT("(a) …and the shared value is a real bonus, so the equality is not two ×1.0s agreeing about nothing"),
			HillMultiplier > 1.5f);
	}

	// ── (b) The tower ON a hill vs a pure hill of the same ADVANTAGE, sited elsewhere ────
	// TOWER-ON-HILL: a 1,000-uu hill crown carrying a 1,200-uu platform ⇒ attacker at 2,200,
	//                target on the valley floor at 0.
	// PURE HILL:     a single 2,200-uu crown rising out of a SUNKEN basin at −500 ⇒ attacker
	//                at 1,700, target at −500.
	// ⚠️ THE SUNKEN BASIN IS THE POINT, NOT DECORATION: without it both sides would reduce to
	// the identical call `MultiplierAt(2200, 0)`, the comparison would be equal by
	// construction, and it would pass against ANY implementation — including the absolute-Z
	// one it is supposed to catch. Sited 500 uu apart, the absolute-Z reading gives ×2.44 vs
	// ×2.12 and this claim fails, which is the only reason it is worth writing down.
	{
		constexpr float HillCrownZ = 1000.f;
		constexpr float PlatformHeight = 1200.f;
		constexpr float BasinFloorZ = -500.f;

		const float TowerOnHill = MultiplierAt(HillCrownZ + PlatformHeight, 0.f);
		const float PureHillInABasin = MultiplierAt(BasinFloorZ + (HillCrownZ + PlatformHeight), BasinFloorZ);

		TestEqual(TEXT("(b) ⭐ A tower stacked on a hill is indistinguishable from a pure hill of the same ADVANTAGE sited 500 uu lower — no tower special case exists, and no altitude term either"),
			TowerOnHill, PureHillInABasin, Tolerance);

		TestTrue(TEXT("(b) …and the shared value is the ×2.44 stacking case, so this is not two ×1.0s agreeing about nothing"),
			TowerOnHill > 2.f);
	}

	// ── (c) TRANSLATION INVARIANCE — where the designer put Z=0 cannot matter ────────────
	// ⭐ THIS IS THE ASSERTION THAT REFUTES THE ABSOLUTE-Z READING OUTRIGHT. Sliding the whole
	// engagement up or down the world axis must not change one bit of the outcome; an
	// absolute-Z implementation changes with every offset.
	{
		const float Reference = MultiplierAt(1200.f, 0.f);
		const float WorldOffsets[] = { -25000.f, -1234.5f, 0.f, 777.25f, 25000.f, 100000.f };

		for (const float Offset : WorldOffsets)
		{
			TestEqual(*FString::Printf(TEXT("(c) Sliding the whole engagement to Z%+.1f changes NOTHING — the rule reads the difference, never the altitude"), Offset),
				MultiplierAt(1200.f + Offset, Offset), Reference, Tolerance);
		}
	}

	// ── (d) ⭐ SELF-LIMITING BY CONSTRUCTION — the whole argument for reading (a) ─────────
	// A unit on a tower shooting a unit on the SAME tower gets nothing. Under absolute Z it
	// would get +78.7% for the crime of both of them being high up, which is not what anybody
	// means by "high ground".
	TestEqual(TEXT("(d) ⭐ Two units on the SAME 1,200-uu platform ⇒ EXACTLY 1.0 — the bonus is height ADVANTAGE, not altitude"),
		MultiplierAt(1200.f, 1200.f), 1.f, Exact);

	TestEqual(TEXT("(d) …and two units on the same 2,200-uu tower-on-a-hill likewise ⇒ EXACTLY 1.0"),
		MultiplierAt(2200.f, 2200.f), 1.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⭐⛔⛔ THE REGRESSION CLAIM — the SHIPPED step is 152.4 uu, and this is the
//     test that catches a future "tidy-up" rounding it to 150 (HIGH-§1)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundShippedTunablesTest,
	"Siegebound.HighGround.TheShippedStepIsFiveFeetConvertedToUnrealUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundShippedTunablesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHighGroundTestFixture;

	// ⚠️ READ BY REFLECTION, ⛔ NOT THROUGH A GETTER ADDED FOR THE TEST'S CONVENIENCE. Both
	// tunables are `protected` EditDefaultsOnly UPROPERTYs, which is the right shape for a
	// designer knob; reaching them reflectively adds ZERO shipped surface. ⭐ AND IT BUYS A
	// SECOND CLAIM FOR FREE: the property NAMES `HIGH-§1` pins are asserted too, because a
	// rename makes the lookup return null and this test fails loudly instead of silently
	// asserting nothing about a field that moved.
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();

	if (!TestNotNull(TEXT("The ASummonedUnit class default object resolves"), UnitDefaults))
	{
		return false;
	}

	const FFloatProperty* const StepProperty =
		FindFProperty<FFloatProperty>(ASummonedUnit::StaticClass(), TEXT("HeightBonusStepUU"));
	const FFloatProperty* const BonusProperty =
		FindFProperty<FFloatProperty>(ASummonedUnit::StaticClass(), TEXT("HeightBonusPerStep"));

	if (StepProperty == nullptr || BonusProperty == nullptr)
	{
		AddError(TEXT("⛔ HeightBonusStepUU and/or HeightBonusPerStep is not a float UPROPERTY on ASummonedUnit under the names HIGH-§1 pins. A rename or retype makes every claim below vacuous, so this is an ERROR rather than a silent skip."));
		return false;
	}

	const float ShippedStepUU = StepProperty->GetPropertyValue_InContainer(UnitDefaults);
	const float ShippedBonusPerStep = BonusProperty->GetPropertyValue_InContainer(UnitDefaults);

	// ── (a) ⭐ THE PINNED NUMBER, CHECKED AGAINST ITS DERIVATION ─────────────────────────
	// Expected = 5 ft × 30.48 cm/ft, computed here from the international definition of the
	// foot — ⛔ NOT copied out of the header. That is what makes this a regression claim and
	// not a transcription: it disagrees with a wrong header instead of agreeing with it.
	TestEqual(TEXT("(a) ⭐ HeightBonusStepUU is EXACTLY 5 ft × 30.48 cm/ft = 152.4 uu"),
		ShippedStepUU, StepUU, Tolerance);

	// ── (b) ⛔ EACH TRAP VALUE, NAMED, so a failure says WHICH mistake was made ──────────
	TestTrue(TEXT("(b) ⛔ It is NOT 150 — 'close enough' is wrong by 1.6% and looks like a designer's round number"),
		!FMath::IsNearlyEqual(ShippedStepUU, 150.f, 1.f));

	TestTrue(TEXT("(b) ⛔ It is NOT 152 — a truncated conversion"),
		!FMath::IsNearlyEqual(ShippedStepUU, 152.f, 0.2f));

	TestTrue(TEXT("(b) ⛔⛔ It is NOT 5 — feet-as-units, wrong by 30×, and it would make every unit on the field a god"),
		!FMath::IsNearlyEqual(ShippedStepUU, 5.f, 1.f));

	TestTrue(TEXT("(b) ⛔ It is NOT 500"),
		!FMath::IsNearlyEqual(ShippedStepUU, 500.f, 1.f));

	// The same trap list stated once more as a BAND, so a wrong value nobody thought to
	// enumerate is caught too: one step is roughly chest-to-head at this project's scale.
	TestTrue(TEXT("(b) The shipped step is > 100 uu — anything smaller means the conversion was skipped"),
		ShippedStepUU > 100.f);

	TestTrue(TEXT("(b) …and < 200 uu — anything larger means it was over-applied"),
		ShippedStepUU < 200.f);

	// ── (c) His "+10%", as shipped ───────────────────────────────────────────────────────
	TestEqual(TEXT("(c) HeightBonusPerStep is 0.10 — Jonathan's '+10%', as a plain additive fraction"),
		ShippedBonusPerStep, 0.10f, Tolerance);

	TestTrue(TEXT("(c) ⛔ …and it is not a PERCENTAGE typed as 10 — that would be +1000% per step"),
		ShippedBonusPerStep < 1.f);

	// ── (d) ⭐ THE TUNABLES AND THE BEHAVIOUR ARE TIED TOGETHER, NOT ASSERTED APART ──────
	// Feeding the SHIPPED defaults through the SHIPPED seam must reproduce his sentence. This
	// is what stops the constant and the formula from drifting past each other while both
	// halves keep their own tests green.
	TestEqual(TEXT("(d) ⭐ The shipped tunables, through the shipped seam: 5 ft of advantage ⇒ ×1.10, exactly as he said"),
		ASummonedUnit::HeightAdvantageMultiplier(ShippedStepUU, 0.f, ShippedStepUU, ShippedBonusPerStep), 1.10f, Tolerance);

	TestEqual(TEXT("(d) …and 10 ft ⇒ ×1.20"),
		ASummonedUnit::HeightAdvantageMultiplier(2.f * ShippedStepUU, 0.f, ShippedStepUU, ShippedBonusPerStep), 1.20f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ⛔ THE HERO AND EVERY MELEE UNIT ARE EXCLUDED — asserted STRUCTURALLY,
//     because the compose site itself has no headless path (HIGH-§4, rows R-5/R-6)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHighGroundHeroAndMeleeExcludedTest,
	"Siegebound.HighGround.TheHeroAndMeleeUnitsAreStructurallyExcluded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHighGroundHeroAndMeleeExcludedTest::RunTest(const FString& Parameters)
{
	// ⚠️ WHAT THIS TEST HONESTLY IS, STATED BEFORE ITS ASSERTIONS: `ComputeOutputDamage` is
	// private and needs two live actors, so ⛔ NO test can watch the hero or a melee unit be
	// turned away at the gate. What CAN be asserted — and what actually protects the ruling —
	// are the two STRUCTURAL facts the exclusion rests on: the hero cannot reach that function
	// at all, and the gate flag is closed until a data row opens it. ⭐ Both fail loudly under
	// the edits that would break the exclusion, which is more than a mock of the gate would do.

	// ── (a) ⛔ THE HERO IS NOT AN ASummonedUnit, SO IT NEVER REACHES COMPOSE POINT 3 ─────
	// `AHeroCharacter` is melee and runs a separate damage path (his row R-5). ⭐ THIS CLAIM
	// FAILS THE DAY SOMEBODY RE-PARENTS THE HERO UNDER ASummonedUnit — which would silently
	// hand the player's own character an uncapped elevation bonus nobody decided to give it.
	TestFalse(TEXT("(a) ⛔ AHeroCharacter is NOT a subclass of ASummonedUnit — the hero's damage never passes through compose point 3"),
		AHeroCharacter::StaticClass()->IsChildOf(ASummonedUnit::StaticClass()));

	// ── (b) The tunables live on the unit and ⛔ NOWHERE ELSE ────────────────────────────
	// The positive half is what keeps the negative half honest: if the names were wrong, BOTH
	// lookups would return null and the "hero has none" claim would pass vacuously.
	TestNotNull(TEXT("(b) ASummonedUnit carries HeightBonusStepUU — the positive control that stops the claim below being vacuous"),
		FindFProperty<FFloatProperty>(ASummonedUnit::StaticClass(), TEXT("HeightBonusStepUU")));

	TestNull(TEXT("(b) ⛔ AHeroCharacter carries NO HeightBonusStepUU — the elevation tunables were not copied onto the hero"),
		FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), TEXT("HeightBonusStepUU")));

	TestNull(TEXT("(b) ⛔ …and no HeightBonusPerStep either"),
		FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), TEXT("HeightBonusPerStep")));

	// ── (c) ⛔ THE GATE IS CLOSED BY DEFAULT — melee is excluded by the SHIPPED flag ──────
	// The compose site is gated on `bRangedAttack`, which is bound from the row's `bRanged`
	// column at LoadStatsAndStart (⛔ never a CardID list, ⛔ never a name check). Its C++
	// default is FALSE, so a unit is melee — and gets exactly ×1.0 — until its own data row
	// says otherwise. ⭐ THIS CLAIM FAILS IF SOMEBODY FLIPS THAT DEFAULT, which would hand the
	// bonus to every unit whose row failed to bind.
	{
		const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();

		if (!TestNotNull(TEXT("The ASummonedUnit class default object resolves"), UnitDefaults))
		{
			return false;
		}

		const FBoolProperty* const RangedFlag =
			FindFProperty<FBoolProperty>(ASummonedUnit::StaticClass(), TEXT("bRangedAttack"));

		if (RangedFlag == nullptr)
		{
			AddError(TEXT("⛔ bRangedAttack is not a bool UPROPERTY on ASummonedUnit. That member IS the elevation gate (HIGH-§4), so its disappearance is an ERROR, not a skip."));
			return false;
		}

		TestFalse(TEXT("(c) ⛔ bRangedAttack defaults to FALSE — the elevation gate is shut until a data row opens it, so every melee unit gets exactly ×1.0"),
			RangedFlag->GetPropertyValue_InContainer(UnitDefaults));
	}

	// ⛔ AND NOTHING ELSE IS ASSERTED HERE ON PURPOSE. The obvious next line — "a skipped
	// multiplier is 1.0, therefore melee damage is unchanged" — would have both sides equal by
	// construction and would report SAFE against every possible implementation. ⚖️ **A claim
	// that cannot fail is worse than no claim, because it occupies the space where a real one
	// would have gone.** The melee bit-for-bit property is TASK-730's gate question 3, read off
	// the diff, and it is named here rather than faked.

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
