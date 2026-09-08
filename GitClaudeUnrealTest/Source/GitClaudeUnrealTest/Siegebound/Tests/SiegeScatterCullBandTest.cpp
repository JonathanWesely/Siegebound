// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Math/RandomStream.h"
#include "Math/UnrealMathUtility.h"
#include "Siegebound/BattlefieldScatter.h"

#include <limits> // the NaN / infinity the IsFinite fail-safe is asserted against

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE FOLIAGE QUALITY LEVER — the scatter's CULL BAND ═══
 *  (TASK-1122 · GFX-§9 · QA gate TASK-1123 · compile + suite host TASK-1124)
 *
 *  Subject: the two PURE statics on `ASiegeBattlefieldScatter` —
 *  `FoliageQualityScaleToCullDistanceFactor()` and `ComputeFoliageScaledCullBand()`.
 *  Four ints and a float in, two ints out: no `UWorld`, no `AActor`, no `UObject`,
 *  no allocation, no clock, no RNG (the `HIGH-§3` / `HeightAdvantageMultiplier`
 *  testability idiom). ⚠️ If a test here ever needs a world, the purity that makes
 *  this lever reviewable has been broken — that is a FINDING, not a reason to add a
 *  fixture.
 *
 *  ───────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE TRAP THIS FILE IS POINTED AT: **`0.25` IS A COUNT, NOT A DISTANCE**
 *  ───────────────────────────────────────────────────────────────────────────────
 *
 *  `USiegeGraphicsSettingsSubsystem`'s ladder (0.25 / 0.50 / 0.75 / 1.00 / 1.00)
 *  was authored for the DENSITY lever `GFX-§9` **struck**, where `0.25` means "a
 *  quarter of the instances". Drawn instances under a cull band scale with the
 *  AREA the band covers (≈ end²), so reusing `0.25` as a DISTANCE multiplier would
 *  draw `0.25² = 6.25 %` of them — a **16× overshoot** — and would pull the grass
 *  band from **90 m to 22.5 m**: grass materialising in the player's lap, which
 *  reads as a broken game at Low rather than a scaled one.
 *
 *  ⇒ the ruling is **`factor = sqrt(quality)`**, and this file asserts the ruling,
 *  not the code. ⛔ **Every expected number below is transcribed from the RULED
 *  METRE TABLE in `handoffs/TASK-1122-programmer.md`, never re-derived by calling
 *  the function under test.** A test computed from its subject agrees with its
 *  subject by construction and reports SAFE for ever (`SHIP-§9c`); this one
 *  disagrees the moment the mapping stops being the one that was ruled.
 *
 *  The authored bands the table is stated against are MEASURED — engine read-backs
 *  off the live DataAsset, recorded in `handoffs/TASK-1083-buildmaster.md`
 *  (Trees 24,000 / 32,000 uu) and `handoffs/TASK-1084-buildmaster.md` §0b
 *  (Grass 6,000 / 9,000 · Plants 8,000 / 12,000). ⛔ `DA_BattlefieldScatter` is
 *  never edited by this row; these are the values it already holds.
 *
 *  ───────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS **CANNOT** SEE (`SC-§79` — stated so green is not read as done)
 *  ───────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔ **A RENDERED PIXEL.** Nothing here draws anything. Whether grass actually
 *      disappears at 45 m is the `GFX-§9` / `SC-§94` pixels rung and it is UNSPENT
 *      by this row — no PIE session, no capture, no frame. The engine-log half is
 *      answered by the `FoliageCullBand` line `ApplyFoliageCullBands()` emits on
 *      every generate (it prints values READ BACK off the component, never the
 *      values requested), and that line has NOT been observed either.
 *    • ⛔ **THE ACTOR'S CALL GRAPH.** `RunScatterPasses` / `ApplyFoliageCullBands` /
 *      `ResolveComponentForMesh` need a world and a DataAsset. So these tests
 *      cannot see: that the read is at the funnel, that the re-apply pass is
 *      actually called, that a Play-Again re-scatter re-applies, or that
 *      `SetCullDistances` reached the render proxy. ⚠️ Both `TASK-1114` blockers in
 *      this lane's history lived in a CALL GRAPH, not in a diff — the gate should
 *      read those three functions rather than trust this file.
 *    • ⛔ **THE INSTANCE COUNTS.** The delivery criterion's second half ("placed
 *      counts IDENTICAL between Low and Epic") is proved here only in miniature —
 *      `TheLeverConsumesNoRandomStreamDraws` shows the functions move no stream.
 *      That the SCATTER moves no stream is proved STRUCTURALLY (the diff contains
 *      no `InstanceCount` / `OuterTarget` arithmetic and the lever is applied after
 *      both placement passes), and structurally is not the same as observed.
 *
 *  ───────────────────────────────────────────────────────────────────────────────
 *  🚨 `SC-§83` / `SC-§101` — THE NAMED MUTATIONS, AND **NO WITNESSED RED**
 *  ───────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔ NOTHING BELOW HAS BEEN COMPILED OR EXECUTED. Every row is a DERIVED
 *  PREDICTION, exactly the class of claim `SC-§101` says inherits no authority from
 *  the reasoning that produced it. Acceptance is a red observed under the mutation,
 *  and this row has observed none.
 *
 *    M1  `FoliageQualityScaleToCullDistanceFactor`: `return ClampedQuality;`
 *        (identity — the naive mapping the ruling refuses)
 *        ⇒ **≥ 3 rows**, including `TheQualityScalarIsTranslatedBySquareRootNotIdentity`,
 *          `TheRuledMetreMappingForTheMeasuredLayers`, `LowAndEpicCullDistancesDiffer…`.
 *
 *    M2  ⛔ **DECLARED NO-RED, ON PURPOSE.** Delete ONLY the `BaseEndUU <= 0` early
 *        return in `ComputeFoliageScaledCullBand`.
 *        ⇒ ~~**ZERO rows red.** Today's arithmetic independently produces `0/0` for a
 *          never-culled layer (`Min(floor, 0) = 0` ⇒ `Clamp(0, 0, 0)`), so the guard
 *          is invisible to every assertion here.~~
 *        🚨⛔ **FALSIFIED BY EXECUTION — TASK-1124, 2026-09-08. M2 REDDENS.**
 *          Measured: **1 test, 5 assertions** — `NeverCulledLayersStayNeverCulled…`,
 *          row *"A zero end keeps its non-zero start untouched"* (expected `4000`,
 *          got `0`), once per quality level. ⛔ **qa/TASK-1123.md's re-derivation
 *          was RIGHT and the author's self-declared no-red was WRONG**: the `0/0`
 *          reasoning covers the END only, and the guard also protects a non-zero
 *          **START** on a never-culled layer — the `(4000, 0)` case the gate named.
 *          ⇒ ⭐ the guard is **genuinely covered**, not defence-in-depth alone.
 *          ⚠️ Keep the honesty of the original note: declaring a suspected no-red
 *          is still the right conduct — it is exactly what got it checked.
 *
 *    M3  Delete the `BaseEndUU <= 0` early return AND replace `EffectiveFloorUU`
 *        with the bare `FoliageCullNearFieldFloorUU` (i.e. drop the `Min` cap) —
 *        the realistic shape of the future edit M2 is guarding against.
 *        ⇒ **≥ 1 row**, `NeverCulledLayersStayNeverCulledAtEveryLevel` (a Hills
 *          layer authored `0` would start culling at 35 m).
 *
 *    M4  Replace `EffectiveFloorUU` with the bare `FoliageCullNearFieldFloorUU`,
 *        keeping the early return (the lever gains the power to LENGTHEN a band).
 *        ⇒ **≥ 1 row**, `TheNearFieldFloorHoldsAndNeverLengthensAnAuthoredBand`
 *          (a layer authored 1,000 uu would be stretched to 3,500).
 *
 *    M5  Delete the `DistanceFactor >= 1.0f` early return.
 *        ⇒ **≥ 1 row**, `EpicAndCinematicReturnTheAuthoredBandsByteForByte` — and
 *          ⚠️ ONLY its authored-HARD-POP case: for an ordinary band the arithmetic
 *          reproduces the authored pair exactly, so the three ordinary layers stay
 *          GREEN under M5. The inverted pair is the whole discriminator, which is
 *          why that assertion is in the file at all.
 *
 *    M6  `FoliageCullNearFieldFloorUU` 3500 → 1800.
 *        ⇒ **≥ 1 row**, `TheNearFieldFloorHoldsAndNeverLengthensAnAuthoredBand`.
 *
 *    M7  Drop the `FMath::Clamp(…, 0.0f, 1.0f)` in the factor.
 *        ⇒ **≥ 1 row**, `TheQualityScalarIsTranslatedBySquareRootNotIdentity`
 *          (its `factor(1.5) == 1.0` and `factor(-0.5) == 0.0` assertions).
 *
 *    M8  Delete the `FMath::IsFinite` fail-safe in the factor.
 *        ⇒ ~~**≥ 1 row**, `TheQualityScalarIsTranslatedBySquareRootNotIdentity`
 *          (its NaN assertion).~~
 *        🚨⛔ **FALSIFIED BY EXECUTION — TASK-1124, 2026-09-08. M8 IS A ZERO-RED:
 *          the suite stayed 552/0.** ⛔ **qa/TASK-1123.md's WARN-1 was RIGHT.**
 *          `FMath::Clamp` is `Max(Min(…))`, and every comparison against a NaN is
 *          false, so the mutated value is squeezed back onto the SAME observable —
 *          `SC-§104` cl. 3, the many-to-one funnel. The assertion cannot see it.
 *          ⛔ **DO NOT "FIX" THIS BY DELETING THE `IsFinite` CALL.** The fail-safe
 *          is real and the undefined behaviour it prevents is real; what is absent
 *          is a TEST that can observe its removal, and one cannot be written
 *          through `Clamp`. Recorded as a declared no-red guard, per the gate.
 *
 *  ⚠️ ONE GUARD IS **UNREACHABLE BY PROOF** AND NO MUTATION IS CLAIMED FOR IT: the
 *  `FMath::Min(…, OutEndUU)` on the scaled start in the non-hard-pop branch. Given
 *  `BaseStart < BaseEnd` and a factor in `[0,1)`, the scaled start is always ≤ the
 *  scaled end and the floor only RAISES the end, so no input can drive it. It is
 *  kept as a guard against a future reordering, and it is declared here rather than
 *  covered by a test that could not fail.
 */

namespace
{
	/** The MEASURED authored bands (uu) — engine read-backs, not header defaults. */
	constexpr int32 TreesAuthoredStartUU = 24000;   // 240 m
	constexpr int32 TreesAuthoredEndUU = 32000;     // 320 m
	constexpr int32 GrassAuthoredStartUU = 6000;    //  60 m
	constexpr int32 GrassAuthoredEndUU = 9000;      //  90 m
	constexpr int32 PlantsAuthoredStartUU = 8000;   //  80 m
	constexpr int32 PlantsAuthoredEndUU = 12000;    // 120 m

	/** The shipped Foliage ladder, Low → Cinematic (USiegeGraphicsSettingsSubsystem). */
	constexpr float LadderLow = 0.25f;
	constexpr float LadderMedium = 0.50f;
	constexpr float LadderHigh = 0.75f;
	constexpr float LadderEpic = 1.00f;
	constexpr float LadderCinematic = 1.00f;

	/** uu → metres. Unreal is centimetres (1 uu = 1 cm). */
	FORCEINLINE float ToMetres(int32 UnrealUnits)
	{
		return UnrealUnits / 100.0f;
	}
}

// ═════════════════════════════════════════════════════════════════════════════
// 1 — THE NO-REGRESSION ASSERTION. Epic is the shipped battlefield, unchanged.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandEpicUnchangedTest,
	"Siegebound.ScatterCullBand.EpicAndCinematicReturnTheAuthoredBandsByteForByte",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandEpicUnchangedTest::RunTest(const FString& /*Parameters*/)
{
	const float EpicLevels[] = { LadderEpic, LadderCinematic, 1.5f /* a scalar ABOVE the baseline must not lengthen anything */ };

	for (const float Quality : EpicLevels)
	{
		int32 OutStart = 0;
		int32 OutEnd = 0;

		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(TreesAuthoredStartUU, TreesAuthoredEndUU, Quality, OutStart, OutEnd);
		TestEqual(TEXT("Trees start is the authored value at Epic/Cinematic"), OutStart, TreesAuthoredStartUU);
		TestEqual(TEXT("Trees end is the authored value at Epic/Cinematic"), OutEnd, TreesAuthoredEndUU);

		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(GrassAuthoredStartUU, GrassAuthoredEndUU, Quality, OutStart, OutEnd);
		TestEqual(TEXT("Grass start is the authored value at Epic/Cinematic"), OutStart, GrassAuthoredStartUU);
		TestEqual(TEXT("Grass end is the authored value at Epic/Cinematic"), OutEnd, GrassAuthoredEndUU);

		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(PlantsAuthoredStartUU, PlantsAuthoredEndUU, Quality, OutStart, OutEnd);
		TestEqual(TEXT("Plants start is the authored value at Epic/Cinematic"), OutStart, PlantsAuthoredStartUU);
		TestEqual(TEXT("Plants end is the authored value at Epic/Cinematic"), OutEnd, PlantsAuthoredEndUU);

		// ⭐ THE DISCRIMINATOR FOR M5, and the reason this case is here at all: an
		// AUTHORED HARD POP (start >= end, ScatterConfig.h:252-255). The Epic early
		// return passes the inverted pair through untouched; the arithmetic path
		// would flatten the start onto the end. For an ORDINARY band the two paths
		// agree exactly, so without this case M5 would ship green.
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(12000, 9000, Quality, OutStart, OutEnd);
		TestEqual(TEXT("An authored hard pop keeps its authored start at Epic"), OutStart, 12000);
		TestEqual(TEXT("An authored hard pop keeps its authored end at Epic"), OutEnd, 9000);
	}

	// Today's pre-lever call was SetCullDistances(Max(Start,0), Max(End,0)); a
	// negative authored value must still clamp to zero and nothing else.
	int32 NegStart = 0;
	int32 NegEnd = 0;
	ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(-500, GrassAuthoredEndUU, LadderEpic, NegStart, NegEnd);
	TestEqual(TEXT("A negative authored start clamps to zero, exactly as the pre-lever call did"), NegStart, 0);
	TestEqual(TEXT("A negative authored start does not disturb the end"), NegEnd, GrassAuthoredEndUU);

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 2 — NEVER-CULLED STAYS NEVER-CULLED. The Hills layer's silhouette contract.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandNeverCulledTest,
	"Siegebound.ScatterCullBand.NeverCulledLayersStayNeverCulledAtEveryLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandNeverCulledTest::RunTest(const FString& /*Parameters*/)
{
	// End == 0 is the engine's NEVER-CULL sentinel, and the HILLS layer rides it so
	// its silhouette reads across the 10x field (ScatterConfig.h:261-269). A
	// multiplier would turn "never culled" into "culled at ZERO distance" — a layer
	// that renders NOWHERE at every level below Epic. That is not a quality
	// setting; it is a layer being switched off.
	const float EveryLevel[] = { LadderLow, LadderMedium, LadderHigh, LadderEpic, LadderCinematic, 0.0f, -1.0f, 7.0f };

	for (const float Quality : EveryLevel)
	{
		int32 OutStart = 0;
		int32 OutEnd = 0;

		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(0, 0, Quality, OutStart, OutEnd);
		TestEqual(TEXT("A never-culled layer keeps a zero start"), OutStart, 0);
		TestEqual(TEXT("A never-culled layer keeps a zero end — the lever may not switch a layer off"), OutEnd, 0);

		// The sentinel is the END. A layer with a non-zero start and a zero end is
		// still never culled, and both values must survive untouched.
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(4000, 0, Quality, OutStart, OutEnd);
		TestEqual(TEXT("A zero end keeps its non-zero start untouched"), OutStart, 4000);
		TestEqual(TEXT("A zero end stays zero regardless of the start"), OutEnd, 0);

		// A negative end is the same sentinel after the Max(…, 0) the pre-lever call
		// already applied.
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(0, -9000, Quality, OutStart, OutEnd);
		TestEqual(TEXT("A negative authored end clamps to the never-cull sentinel"), OutEnd, 0);
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 3 — THE MAPPING RULING ITSELF: sqrt, not identity.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandSqrtMappingTest,
	"Siegebound.ScatterCullBand.TheQualityScalarIsTranslatedBySquareRootNotIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandSqrtMappingTest::RunTest(const FString& /*Parameters*/)
{
	// The ruled factors, transcribed from the handoff's metre table — NOT computed
	// by calling the subject.
	TestEqual(TEXT("Low  quality 0.25 maps to a HALF-distance band"), ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(LadderLow), 0.500000f, 0.0005f);
	TestEqual(TEXT("Medium quality 0.50 maps to 0.707"), ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(LadderMedium), 0.707107f, 0.0005f);
	TestEqual(TEXT("High quality 0.75 maps to 0.866"), ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(LadderHigh), 0.866025f, 0.0005f);
	TestEqual(TEXT("Epic quality 1.00 maps to EXACTLY 1.0"), ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(LadderEpic), 1.0f, 0.0f);

	// ⛔ THE REFUSAL, ASSERTED AS A REFUSAL. The naive mapping is what the value
	// ladder invites, so the test says out loud that we do not do it.
	TestTrue(TEXT("Low does NOT reuse 0.25 as a distance multiplier (that would be a 16x overshoot)"),
		!FMath::IsNearlyEqual(ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(LadderLow), LadderLow, 0.01f));

	// The identity that makes sqrt the RIGHT answer rather than merely a gentler
	// one: drawn instances scale with the area, so factor² must reproduce the
	// count scalar the ladder was authored to deliver.
	for (const float Quality : { LadderLow, LadderMedium, LadderHigh, LadderEpic })
	{
		const float Factor = ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(Quality);
		TestEqual(TEXT("factor squared reproduces the quality scalar (drawn instances scale with area)"), Factor * Factor, Quality, 0.001f);
	}

	// Fail-safes. A scalar above the baseline may never LENGTHEN the band (GFX-§9:
	// Cinematic never exceeds Epic), a negative may never produce a nonsense
	// distance, and a non-finite value may never reach RoundToInt.
	TestEqual(TEXT("A quality above 1.0 is clamped to the authored baseline"), ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(1.5f), 1.0f, 0.0f);
	TestEqual(TEXT("A negative quality clamps to zero rather than producing a negative distance"), ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(-0.5f), 0.0f, 0.0f);
	// ⛔ NaN AND INFINITY BOTH, because FMath::Clamp cannot catch either: Clamp is a
	// pair of comparisons and every comparison against a NaN is false, so a NaN
	// would sail through to FMath::RoundToInt — where a float-to-int conversion of
	// a NaN is undefined behaviour, on its way into SetCullDistances.
	TestEqual(TEXT("A NaN quality falls back to FULL distance, never NaN"),
		ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(std::numeric_limits<float>::quiet_NaN()), 1.0f, 0.0f);
	TestEqual(TEXT("An infinite quality falls back to FULL distance"),
		ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(std::numeric_limits<float>::infinity()), 1.0f, 0.0f);

	// And the whole band, not just the factor: a NaN must never reach the ints.
	int32 NanStart = 0;
	int32 NanEnd = 0;
	ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(GrassAuthoredStartUU, GrassAuthoredEndUU,
		std::numeric_limits<float>::quiet_NaN(), NanStart, NanEnd);
	TestEqual(TEXT("A NaN quality yields the authored start, not a garbage int"), NanStart, GrassAuthoredStartUU);
	TestEqual(TEXT("A NaN quality yields the authored end, not a garbage int"), NanEnd, GrassAuthoredEndUU);

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 4 — THE RULED METRE TABLE, layer by layer, level by level.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandMetreTableTest,
	"Siegebound.ScatterCullBand.TheRuledMetreMappingForTheMeasuredLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandMetreTableTest::RunTest(const FString& /*Parameters*/)
{
	struct FExpectedRow
	{
		const TCHAR* LayerName;
		int32 AuthoredStartUU;
		int32 AuthoredEndUU;
		float Quality;
		float ExpectedStartMetres;
		float ExpectedEndMetres;
	};

	// ⛔ TRANSCRIBED FROM THE HANDOFF'S RULED TABLE, IN METRES. Tolerance is 0.2 m
	// (20 uu) because the ruling is a DISTANCE POLICY, not a bit pattern — pinning
	// the exact int would make a harmless float-rounding change look like a defect,
	// while 0.2 m is far tighter than any mapping error could hide in.
	const FExpectedRow Rows[] =
	{
		{ TEXT("Trees"),  TreesAuthoredStartUU,  TreesAuthoredEndUU,  LadderLow,       120.0f, 160.0f },
		{ TEXT("Trees"),  TreesAuthoredStartUU,  TreesAuthoredEndUU,  LadderMedium,    169.7f, 226.3f },
		{ TEXT("Trees"),  TreesAuthoredStartUU,  TreesAuthoredEndUU,  LadderHigh,      207.8f, 277.1f },
		{ TEXT("Trees"),  TreesAuthoredStartUU,  TreesAuthoredEndUU,  LadderEpic,      240.0f, 320.0f },

		{ TEXT("Grass"),  GrassAuthoredStartUU,  GrassAuthoredEndUU,  LadderLow,        30.0f,  45.0f },
		{ TEXT("Grass"),  GrassAuthoredStartUU,  GrassAuthoredEndUU,  LadderMedium,     42.4f,  63.6f },
		{ TEXT("Grass"),  GrassAuthoredStartUU,  GrassAuthoredEndUU,  LadderHigh,       52.0f,  77.9f },
		{ TEXT("Grass"),  GrassAuthoredStartUU,  GrassAuthoredEndUU,  LadderEpic,       60.0f,  90.0f },

		{ TEXT("Plants"), PlantsAuthoredStartUU, PlantsAuthoredEndUU, LadderLow,        40.0f,  60.0f },
		{ TEXT("Plants"), PlantsAuthoredStartUU, PlantsAuthoredEndUU, LadderMedium,     56.6f,  84.9f },
		{ TEXT("Plants"), PlantsAuthoredStartUU, PlantsAuthoredEndUU, LadderHigh,       69.3f, 103.9f },
		{ TEXT("Plants"), PlantsAuthoredStartUU, PlantsAuthoredEndUU, LadderEpic,       80.0f, 120.0f },
	};

	for (const FExpectedRow& Row : Rows)
	{
		int32 OutStartUU = 0;
		int32 OutEndUU = 0;
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(Row.AuthoredStartUU, Row.AuthoredEndUU, Row.Quality, OutStartUU, OutEndUU);

		TestEqual(*FString::Printf(TEXT("%s fade START at quality %.2f (metres)"), Row.LayerName, Row.Quality),
			ToMetres(OutStartUU), Row.ExpectedStartMetres, 0.2f);
		TestEqual(*FString::Printf(TEXT("%s cull END at quality %.2f (metres)"), Row.LayerName, Row.Quality),
			ToMetres(OutEndUU), Row.ExpectedEndMetres, 0.2f);
	}

	// ⛔ THE ONE NUMBER THE RESHAPE EXISTS TO PREVENT, ASSERTED AS AN ABSENCE:
	// grass at Low must NOT land on 22.5 m (the refused `9000 x 0.25`).
	int32 LowGrassStartUU = 0;
	int32 LowGrassEndUU = 0;
	ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(GrassAuthoredStartUU, GrassAuthoredEndUU, LadderLow, LowGrassStartUU, LowGrassEndUU);
	TestTrue(TEXT("Grass at Low does NOT end at the refused 22.5 m (grass in the player's lap)"),
		ToMetres(LowGrassEndUU) > 30.0f);
	TestTrue(TEXT("Grass at Low still reaches at least the 35 m keep-clear distance from the hero's spawn"),
		LowGrassEndUU >= ASiegeBattlefieldScatter::FoliageCullNearFieldFloorUU);

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 5 — THE DELIVERY CRITERION, first half: Low and Epic must DIFFER.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandLowDiffersFromEpicTest,
	"Siegebound.ScatterCullBand.LowAndEpicCullDistancesDifferAndLowIsAlwaysShorter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandLowDiffersFromEpicTest::RunTest(const FString& /*Parameters*/)
{
	// TASK-1116 measured that the Foliage row is one of three on the panel that
	// CANNOT CHANGE A PIXEL today: `[FoliageQuality@N]` sets only
	// foliage.DensityScale / grass.DensityScale / pcg.Quality, and this project has
	// no foliage system, no landscape and no PCG. This assertion is the whole
	// reason the row exists — if Low and Epic agree, the slider still lies.
	const int32 AuthoredBands[][2] =
	{
		{ TreesAuthoredStartUU,  TreesAuthoredEndUU  },
		{ GrassAuthoredStartUU,  GrassAuthoredEndUU  },
		{ PlantsAuthoredStartUU, PlantsAuthoredEndUU },
	};

	for (const int32(&Band)[2] : AuthoredBands)
	{
		int32 LowStart = 0;
		int32 LowEnd = 0;
		int32 EpicStart = 0;
		int32 EpicEnd = 0;
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(Band[0], Band[1], LadderLow, LowStart, LowEnd);
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(Band[0], Band[1], LadderEpic, EpicStart, EpicEnd);

		TestTrue(TEXT("Low's cull END differs from Epic's — the slider changes something"), LowEnd != EpicEnd);
		TestTrue(TEXT("Low's cull START differs from Epic's"), LowStart != EpicStart);
		TestTrue(TEXT("Low is SHORTER than Epic, never longer"), LowEnd < EpicEnd);

		// And the difference is worth having: at Low the band draws a quarter of the
		// area Epic does, which is the reduction the ladder was authored to deliver.
		const float AreaRatio = (static_cast<float>(LowEnd) * LowEnd) / (static_cast<float>(EpicEnd) * EpicEnd);
		TestEqual(TEXT("Low draws about a QUARTER of Epic's area (the ladder's intended 0.25)"), AreaRatio, 0.25f, 0.02f);
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 6 — MONOTONE, BOUNDED, NEVER INVERTED.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandMonotoneTest,
	"Siegebound.ScatterCullBand.TheLadderIsMonotoneNeverExceedsTheAuthoredBandAndNeverInverts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandMonotoneTest::RunTest(const FString& /*Parameters*/)
{
	const float Ladder[] = { LadderLow, LadderMedium, LadderHigh, LadderEpic, LadderCinematic };

	const int32 AuthoredBands[][2] =
	{
		{ TreesAuthoredStartUU,  TreesAuthoredEndUU  },
		{ GrassAuthoredStartUU,  GrassAuthoredEndUU  },
		{ PlantsAuthoredStartUU, PlantsAuthoredEndUU },
		{ 0,                     0                   }, // a never-culled layer (Hills)
		{ 1500,                  2000                }, // a synthetic short band, below the floor
	};

	for (const int32(&Band)[2] : AuthoredBands)
	{
		int32 PreviousEnd = -1;

		for (const float Quality : Ladder)
		{
			int32 OutStart = 0;
			int32 OutEnd = 0;
			ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(Band[0], Band[1], Quality, OutStart, OutEnd);

			TestTrue(TEXT("The applied end never exceeds the authored end — the lever only ever SHORTENS"), OutEnd <= FMath::Max(Band[1], 0));
			TestTrue(TEXT("The applied start never exceeds the authored start"), OutStart <= FMath::Max(Band[0], 0));
			TestTrue(TEXT("Neither distance is ever negative"), OutStart >= 0 && OutEnd >= 0);

			// An authored fade band (start < end) stays a fade band; the pair is
			// never inverted by the scaling or by the floor.
			if (FMath::Max(Band[0], 0) < FMath::Max(Band[1], 0))
			{
				TestTrue(TEXT("An authored fade band is never inverted into a start beyond its end"), OutStart <= OutEnd);
			}

			TestTrue(TEXT("A higher quality level never yields a SHORTER band"), OutEnd >= PreviousEnd);
			PreviousEnd = OutEnd;
		}
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 7 — THE NEAR-FIELD FLOOR: 35 m, measured, and it may never lengthen a band.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandFloorTest,
	"Siegebound.ScatterCullBand.TheNearFieldFloorHoldsAndNeverLengthensAnAuthoredBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandFloorTest::RunTest(const FString& /*Parameters*/)
{
	TestEqual(TEXT("The near-field floor is 35 m — the measured castle keep-clear distance from the hero's spawn"),
		ToMetres(ASiegeBattlefieldScatter::FoliageCullNearFieldFloorUU), 35.0f, 0.01f);

	// A synthetic layer whose authored band is long enough to be scaled but short
	// enough that Low would push it under the floor: 4,000 uu (40 m) x 0.5 = 20 m.
	// ⚠️ NO SHIPPED LAYER IS IN THIS SITUATION TODAY — the shortest applied end is
	// grass at Low, 45 m. This is a bound on FUTURE DA_BattlefieldScatter edits,
	// and it is asserted with a synthetic layer precisely because no real one
	// exercises it. Stated so nobody reads a green here as evidence the floor fired
	// in a real match.
	int32 OutStart = 0;
	int32 OutEnd = 0;
	ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(3000, 4000, LadderLow, OutStart, OutEnd);
	TestEqual(TEXT("A short band is floored at 35 m rather than pulled to 20 m"), OutEnd, ASiegeBattlefieldScatter::FoliageCullNearFieldFloorUU);
	TestTrue(TEXT("The floored end still sits inside the authored band"), OutEnd <= 4000);
	TestTrue(TEXT("The fade start is still below the floored end"), OutStart < OutEnd);

	// ⛔ AND THE OTHER DIRECTION, WHICH IS THE ONE A BARE CONSTANT WOULD BREAK: a
	// layer authored TIGHTER than the floor is exempt, never stretched out to meet
	// it. The lever may shorten a band; it may not lengthen one.
	ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(800, 1000, LadderLow, OutStart, OutEnd);
	TestEqual(TEXT("A layer authored tighter than the floor keeps its authored end"), OutEnd, 1000);
	TestTrue(TEXT("The floor never lengthens a band toward 35 m"), OutEnd < ASiegeBattlefieldScatter::FoliageCullNearFieldFloorUU);

	// The band never collapses to nothing at any level, for any positive authored
	// end — a scatter culled to NOTHING is a bug, not a quality level.
	const float EveryLevel[] = { LadderLow, LadderMedium, LadderHigh, LadderEpic, 0.0f, -3.0f };
	const int32 PositiveEnds[] = { 1, 500, 4000, 9000, 32000 };
	for (const float Quality : EveryLevel)
	{
		for (const int32 AuthoredEnd : PositiveEnds)
		{
			int32 Start = 0;
			int32 End = 0;
			ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(AuthoredEnd / 2, AuthoredEnd, Quality, Start, End);
			TestTrue(TEXT("A culled layer never collapses to a zero-distance band"), End > 0);
			TestTrue(TEXT("A culled layer never gains a negative band"), Start >= 0);
		}
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 8 — AN AUTHORED HARD POP STAYS A HARD POP.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandHardPopTest,
	"Siegebound.ScatterCullBand.AnAuthoredHardPopStaysAHardPop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandHardPopTest::RunTest(const FString& /*Parameters*/)
{
	// ScatterConfig.h:252-255: a start >= end is treated by the engine as a HARD POP
	// at the end distance. Scaling the two independently could quietly convert that
	// authored decision into a fade — the semantics have to survive, not just the
	// arithmetic.
	const float BelowEpic[] = { LadderLow, LadderMedium, LadderHigh };

	for (const float Quality : BelowEpic)
	{
		int32 OutStart = 0;
		int32 OutEnd = 0;

		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(9000, 9000, Quality, OutStart, OutEnd);
		TestEqual(TEXT("An equal start/end pair stays a hard pop after scaling"), OutStart, OutEnd);

		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(12000, 9000, Quality, OutStart, OutEnd);
		TestEqual(TEXT("An inverted start/end pair stays a hard pop after scaling"), OutStart, OutEnd);
		TestTrue(TEXT("The hard pop still moved closer at a lower quality level"), OutEnd < 9000);
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// 9 — THE DETERMINISM HALF, IN MINIATURE: the lever draws no RNG.
// ═════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeScatterCullBandNoRngTest,
	"Siegebound.ScatterCullBand.TheLeverConsumesNoRandomStreamDraws",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeScatterCullBandNoRngTest::RunTest(const FString& /*Parameters*/)
{
	// ⭐ THE WHOLE RESHAPE IN ONE ASSERTION. `InstanceCount` / `OuterTarget` drive
	// the iteration count of a loop drawing from a SHARED FRandomStream
	// (BattlefieldScatter.cpp:647, :648, :660), so a per-machine multiplier there
	// would shift the stream and desync EVERY SUBSEQUENT LAYER. The cull band is
	// safe because it touches the stream not at all — and "not at all" is a claim
	// that can be measured rather than promised.
	//
	// ⚠️ SCOPE, PLAINLY: this proves the two PURE FUNCTIONS move no stream. It does
	// NOT prove the ACTOR moves no stream — that is a property of the call graph
	// (the read sits beside `FRandomStream Stream(Seed)` and the apply pass runs
	// after both placement passes, with `Stream` never passed to either), and it is
	// the gate's to verify by reading, not this file's to assert.
	FRandomStream Control(20260907);
	FRandomStream Subject(20260907);

	for (int32 Warmup = 0; Warmup < 8; ++Warmup)
	{
		Control.FRand();
		Subject.FRand();
	}

	const float EveryLevel[] = { LadderLow, LadderMedium, LadderHigh, LadderEpic, LadderCinematic, 0.0f, -1.0f, 4.0f };
	for (int32 Repeat = 0; Repeat < 64; ++Repeat)
	{
		for (const float Quality : EveryLevel)
		{
			int32 OutStart = 0;
			int32 OutEnd = 0;
			ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(GrassAuthoredStartUU, GrassAuthoredEndUU, Quality, OutStart, OutEnd);
			ASiegeBattlefieldScatter::FoliageQualityScaleToCullDistanceFactor(Quality);
		}
	}

	TestEqual(TEXT("The subject stream's position is unmoved by 512 lever evaluations"), Subject.GetCurrentSeed(), Control.GetCurrentSeed());
	for (int32 Draw = 0; Draw < 16; ++Draw)
	{
		TestEqual(TEXT("Every subsequent draw is identical — the lever did not displace the sequence"), Subject.FRand(), Control.FRand(), 0.0f);
	}

	// And the functions are stateless: the same inputs give the same answer no
	// matter how many times, or in what order, they were called before. A hidden
	// latch here would make two machines disagree on the SECOND match.
	int32 FirstStart = 0;
	int32 FirstEnd = 0;
	ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(TreesAuthoredStartUU, TreesAuthoredEndUU, LadderLow, FirstStart, FirstEnd);
	for (int32 Repeat = 0; Repeat < 32; ++Repeat)
	{
		int32 AgainStart = 0;
		int32 AgainEnd = 0;
		ASiegeBattlefieldScatter::ComputeFoliageScaledCullBand(TreesAuthoredStartUU, TreesAuthoredEndUU, LadderLow, AgainStart, AgainEnd);
		TestEqual(TEXT("Repeated evaluations return an identical start"), AgainStart, FirstStart);
		TestEqual(TEXT("Repeated evaluations return an identical end"), AgainEnd, FirstEnd);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
