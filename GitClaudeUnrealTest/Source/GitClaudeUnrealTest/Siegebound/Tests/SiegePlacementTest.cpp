// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Set.h"          // TASK-819 — the "not one binned card came back" measurement
#include "Containers/UnrealString.h"
#include "HAL/UnrealMemory.h"        // FMemory::Memcpy — the SiegeStuckStaticsTest bit-pattern NaN idiom
#include "Internationalization/Text.h"
#include "Math/BoxSphereBounds.h"
#include "Math/Transform.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Misc/FileHelper.h"          // TASK-813 test 17 — the STACK-§2 "no CardID string compare" source probe
#include "Misc/Paths.h"
#include "Siegebound/Building.h"      // TASK-813 — ABuilding: the hover target, its team/CardID/CanScaleFootprint and the TASK-812 series
#include "Siegebound/ClimbableTower.h"// TASK-813 — the ONE class the exclusion must catch, asked through an ABuilding*
#include "Siegebound/DeckComponent.h" // TASK-819 — the hand model DiscardEntireHand loops over
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h" // TASK-819 — the gold SpendGold moves, exactly once
#include "Siegebound/SummonedUnit.h"  // TASK-815 — the class a UNIT card resolves to: the wheel must be inert for it, for FREE
#include "Siegebound/TeamId.h"
#include "Siegebound/Tower.h"         // TASK-813 — the Arrow/Bomb/Ballista/Crystal family, which must NOT be caught by it
#include "UObject/Class.h"
#include "UObject/SoftObjectPtr.h"   // TASK-819 — reading DiscardAllActionAsset's path off the CDO
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for BUILDING PLACEMENT — the FOOTPRINT rules (TASK-735, TOWER-§7) ═══
 *
 *  Subject: `ASiegePlayerController`'s four pinned pure placement seams —
 *  `PlacementFootprintRadiusFromBounds` · `EffectiveBuildingClearance` ·
 *  `IsInsidePlacementFootprint` · `UnitFootprintRefusalText` — plus the shipped
 *  `EditDefaultsOnly` tunables they are fed from, read off the CDO by reflection.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔ "NONE FITS" — WHY THIS IS A NEW FILE AND ⛔ NOT A DUPLICATE FRAME (spec (7))
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Checked first, as required. **No placement or controller test file exists.** The six test
 *  files that `#include` `SiegePlayerController.h` are `SiegeAssistantSelectionTest` ·
 *  `SiegeControlsHelpTest` · `SiegeDeckSlotsTest` · `SiegeRecallTest` ·
 *  `SiegeRespawnLifecycleTest` · `SiegeWarMapTest` — each is the test frame of a DIFFERENT
 *  feature that happens to reach the controller, and hanging the placement rules off any one
 *  of them would file this batch's regressions under someone else's subject.
 *
 *  ⭐⭐ **THIS FILE IS THE PLACEMENT FRAME. `TASK-813` AND `TASK-815` EXTEND IT — ⛔ THEY DO
 *  ⛔ NOT ADD A SECOND ONE.** It is named for the mechanic (`SiegePlacementTest`), not for
 *  this task, precisely so their stacking/wheel claims have an obvious home.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️ **AND ONE LODGER, DECLARED RATHER THAN SMUGGLED: `TASK-819`'s DISCARD-ALL (tests 18–23)**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔ **The discard-all is NOT a placement mechanic and this file's name does not cover it.**
 *  It lives here because `TASK-819`'s dispatch fenced it to `SiegePlayerController.{h,cpp}` +
 *  **this** test file, and because the instrument its sharpest claims need — a comment-skipping
 *  source probe over `SiegePlayerController.{h,cpp}`, with a self-checked scanner — is already
 *  built here (test 17) and would otherwise be cloned into a second frame. ⇒ ⭐ they reuse
 *  `SiegePlacementUpgradeFixture`'s `LoadProjectSource` / `CountOccurrencesInCode` verbatim.
 *
 *  🧑 **A future `SiegeCardDiscardTest.cpp` is the tidier home and re-homing them costs nothing**
 *  — this note exists so that is a decision somebody makes, ⛔ not a surprise somebody finds.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHAT MAKES THIS FILE POSSIBLE — the purity the four seams ship on purpose
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  All four are floats/vectors in, one value out: no `UWorld`, no `AActor`, no allocation, no
 *  clock, no RNG (the `HeightAdvantageMultiplier` / `HeightToBrightness` precedent — a
 *  testability obligation gets a testability seam). ⇒ the whole footprint rule runs in-process
 *  with NO PIE session. ⚠️ If a test here ever starts needing a world, that purity has been
 *  broken, and that is a FINDING rather than a reason to add a fixture.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ EVERY CLAIM BELOW MUST BE ABLE TO **FAIL** (`SHIP-§9c` clause 1), AND THE ONE THAT
 *  IS EASIEST TO FAKE IS NAMED HERE SO NOBODY HAS TO GO LOOKING FOR IT
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⚠️ **A placement-REFUSAL test passes trivially if the footprint never overlapped anything.**
 *  "It refused" proves nothing on its own — a function that returns `true` for everything
 *  passes it. ⇒ ⭐ **EVERY refusal claim in test 6 and test 7 is paired with a CONTROL THAT
 *  MUST BE ADMITTED**, holding the fixture fixed and moving only the term under test:
 *    • same unit, same distance, SMALL footprint  ⇒ must be ADMITTED (test 6b);
 *    • same LARGE footprint, unit moved out       ⇒ must be ADMITTED (test 6c);
 *    • same geometry, body radius 0               ⇒ must be ADMITTED (test 7a).
 *  A "refuses everything" implementation fails all three; a "refuses nothing" implementation
 *  fails the positives. Neither can pass this file.
 *
 *  ⭐ THE SECOND ANTI-FAKE MEASURE: the shipped numbers are read off the CDO by REFLECTION
 *  (the `SiegeHighGroundTest` idiom), and the fixture expresses its footprints as MULTIPLES
 *  OF THE SHIPPED CLEARANCE. ⇒ these tests state a RELATIONSHIP ("smaller than the shipped
 *  number" / "larger than it"), not a transcribed magnitude, so they keep their meaning if
 *  🧑 T-6 retunes anything — and they disagree with a wrong header instead of agreeing with it.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — stated so nobody mistakes green for done
 *  (`SC-§32`: a mechanism never observed to function is not known to function)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔ **THE BOUNDS READ ITSELF.** `TryGetPlacementFootprintRadius` is private, non-const
 *      and reads a live `AStaticMeshActor`; `HasUnitClearance` iterates a `UWorld`. There is
 *      no headless path to either, and ⛔ this file does not pretend otherwise — inventing an
 *      accessor purely so a test could reach them would add shipped surface to make a test
 *      possible. **Their instruments are the QA diff read (TASK-816) and Jonathan's playtest.**
 *      What IS asserted here is the arithmetic they call, and — in test 3 — the scaled-vs-local
 *      distinction reproduced through the SAME `FBoxSphereBounds::TransformBy` that
 *      `UStaticMeshComponent::CalcBounds` performs.
 *    • ⛔ **THE REASON ENUM.** `EPlacementInvalidReason` is private and deliberately STAYS
 *      private (`TASK-813`/`815` are about to edit that switch; a smaller diff is worth more
 *      to them than a refactor). Test 9 asserts the part the PLAYER can tell apart — the
 *      message — which is the observable consequence of the reason being its own value.
 *    • ⛔ **NOTHING HERE RUNS THE MODEL.** No inference, no `Capture()`, no `EnsureSnapshot()`.
 *      🔒 The one-shot latch is untouched and unspent, Zone A is not read, and ⛔ no token
 *      figure appears anywhere in this file (`AS-§12g`).
 */

namespace SiegePlacementTestFixture
{
	/** Exact-equality tolerance, for the claims whose whole content is the words BYTE-FOR-BYTE. */
	constexpr float Exact = 0.f;

	/** Ordinary float tolerance — tight enough that every wrong axis choice blows through it. */
	constexpr float Tolerance = 1.e-3f;

	/**
	 *  Looser tolerance for the ROTATED-bounds claims only: `FBoxSphereBounds::TransformBy`
	 *  builds the rotated AABB from the abs of the rotation matrix, and `cos(-90°)` is
	 *  ~-4.4e-8 rather than 0, so an exact quarter turn leaves a few micro-units behind.
	 *  ⛔ Still far tighter than any real axis mix-up (which moves the answer by ~175 uu here).
	 */
	constexpr float RotationTolerance = 1.e-2f;

	/**
	 *  ⛔ `FAutomationTestBase::TestNotEqual` has NO numeric overload in UE 5.8 (verified at
	 *  `Misc/AutomationTest.h:2004-2012` — only TCHAR*, FStringView, FString, FUtf8StringView,
	 *  FText, FName). ⇒ every "these two numbers must DIFFER" claim goes through this, so the
	 *  intent is explicit and no call silently binds to a string overload.
	 */
	static bool DiffersFrom(float A, float B)
	{
		return !FMath::IsNearlyEqual(A, B, Tolerance);
	}

	/**
	 *  Non-finite floats built from their BIT PATTERNS — the `SiegeStuckStaticsTest` house
	 *  idiom, adopted rather than reinvented. ⛔ Deliberately NOT `0.f/0.f` (undefined
	 *  behaviour) and NOT `FMath::Sqrt(-1.f)` (a toolchain may fold it). Every test that uses
	 *  these asserts `FMath::IsNaN` / `!FMath::IsFinite` on the value FIRST, so a build that
	 *  optimised the value away reports itself instead of passing silently.
	 */
	static float BitsToFloat(uint32 Bits)
	{
		float Result = 0.f;
		FMemory::Memcpy(&Result, &Bits, sizeof(Result));
		return Result;
	}

	static float MakeQuietNaN() { return BitsToFloat(0x7FC00000u); }

	/** Reads a shipped float `UPROPERTY` off the controller CDO, or returns false. */
	static bool TryReadShippedFloat(const TCHAR* PropertyName, float& OutValue)
	{
		OutValue = 0.f;
		const ASiegePlayerController* const Defaults = GetDefault<ASiegePlayerController>();
		if (Defaults == nullptr)
		{
			return false;
		}
		const FFloatProperty* const Property =
			FindFProperty<FFloatProperty>(ASiegePlayerController::StaticClass(), PropertyName);
		if (Property == nullptr)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Defaults);
		return true;
	}

	/**
	 *  ⭐ THE SCALED-BOUNDS PATH, REPRODUCED RATHER THAN DESCRIBED. This is exactly what
	 *  `UStaticMeshComponent::CalcBounds(GetComponentTransform())` does — take the mesh's LOCAL
	 *  bounds and `TransformBy` the component's world transform — so a test written against it
	 *  is testing the real seam and not a paraphrase of it.
	 */
	static FVector ScaledExtent(const FVector& LocalExtent, const FTransform& ComponentTransform)
	{
		const FBoxSphereBounds LocalBounds(FVector::ZeroVector, LocalExtent, LocalExtent.Size());
		return LocalBounds.TransformBy(ComponentTransform).BoxExtent;
	}

	/** The seam under test, spelled once so no test re-types it. */
	static float RadiusOf(const FVector& Extent)
	{
		return ASiegePlayerController::PlacementFootprintRadiusFromBounds(Extent);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. THE FOOTPRINT RADIUS IS THE LARGER HORIZONTAL HALF-EXTENT — and ⛔ Z is
//     IGNORED, because every placement rule in this class is planar
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementFootprintRadiusIsLargerHorizontalHalfExtentTest,
	"Siegebound.Placement.FootprintRadiusIsTheLargerHorizontalHalfExtentAndIgnoresZ",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementFootprintRadiusIsLargerHorizontalHalfExtentTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	// (a) X dominant, then Y dominant — the answer must follow the LARGER axis, not a fixed one.
	TestEqual(TEXT("(a) X-dominant extent yields the X half-extent"),
		RadiusOf(FVector(375.f, 200.f, 900.f)), 375.f, Tolerance);
	TestEqual(TEXT("(a) Y-dominant extent yields the Y half-extent"),
		RadiusOf(FVector(200.f, 375.f, 900.f)), 375.f, Tolerance);

	// (b) ⭐ Z IS IGNORED. A tower is TALL; if height leaked into the radius, a 1,200 uu rise
	//     would refuse placements across a quarter of the spawn box. This claim FAILS for any
	//     implementation that reached for GetMax()/Size()/GetAbsMax() instead of max(|X|,|Y|).
	TestEqual(TEXT("(b) ⛔ A very tall, very narrow extent still yields its HORIZONTAL radius — height must not leak in"),
		RadiusOf(FVector(100.f, 100.f, 5000.f)), 100.f, Tolerance);

	// (c) Sign-blind: a bounds convention that hands back negative half-extents must not
	//     silently produce a negative radius (which would disable every gate downstream).
	TestEqual(TEXT("(c) Negative half-extents are read by magnitude, not by sign"),
		RadiusOf(FVector(-375.f, -200.f, 0.f)), 375.f, Tolerance);

	// (d) Degenerate and non-finite bounds answer 0 — the degrade-OPEN value, which composes
	//     to today's exact behaviour at every consumer.
	TestEqual(TEXT("(d) A zero extent yields 0 (degrade open)"),
		RadiusOf(FVector::ZeroVector), 0.f, Exact);

	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(d) pre — the NaN fixture really is NaN (a folded constant would make the next claim vacuous)"),
		FMath::IsNaN(NaNValue));
	TestEqual(TEXT("(d) A NaN extent yields 0 rather than propagating NaN into a comparison"),
		RadiusOf(FVector(NaNValue, 10.f, 0.f)), 0.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⭐⛔⛔ NO LITERAL SURVIVES — the radius TRACKS THE MESH, which is the whole
//     safety argument for touching a shipped path (TOWER-§7)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementFootprintRadiusTracksTheMeshAndIsNeverAConstantTest,
	"Siegebound.Placement.FootprintRadiusTracksTheMeshBoundsAndIsNeverAHardcodedConstant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementFootprintRadiusTracksTheMeshAndIsNeverAConstantTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	// ⭐ THE INSTRUMENT FOR "A HARDCODED 2700 IS AN AUTOMATIC FAIL": three DIFFERENT meshes,
	// three DIFFERENT answers, each equal to its own input. Any constant — 2700, 750, 375, or
	// the shipped clearance — fails at least two of these three, and a "return the first one
	// you saw" cache fails the second and third.
	const float Wallish = 120.f;
	const float Mineish = 260.f;
	const float Towerish = 375.f;

	TestEqual(TEXT("(a) A wall-scale mesh reports its own size"), RadiusOf(FVector(Wallish, 60.f, 300.f)), Wallish, Tolerance);
	TestEqual(TEXT("(a) A mine-scale mesh reports its own size"), RadiusOf(FVector(Mineish, 180.f, 400.f)), Mineish, Tolerance);
	TestEqual(TEXT("(a) A tower-scale mesh reports its own size"), RadiusOf(FVector(Towerish, 375.f, 1200.f)), Towerish, Tolerance);

	// (b) ⭐ AND THE THREE ARE MUTUALLY DISTINCT — stated separately because "each equals its
	//     input" would still pass if two inputs coincided by accident.
	TestTrue(TEXT("(b) ⛔ wall-scale and mine-scale must not collapse to one answer"),
		DiffersFrom(RadiusOf(FVector(Wallish, 60.f, 300.f)), RadiusOf(FVector(Mineish, 180.f, 400.f))));
	TestTrue(TEXT("(b) ⛔ mine-scale and tower-scale must not collapse to one answer"),
		DiffersFrom(RadiusOf(FVector(Mineish, 180.f, 400.f)), RadiusOf(FVector(Towerish, 375.f, 1200.f))));

	// (c) ⭐ THE DIVIDEND OF THE RULE, MADE A TEST: the Watch Tower's footprint was re-authored
	//     from ~2,700 uu to ~750 uu (half-extent ~1,350 → ~375) one day after this rule was
	//     written, and NOTHING here needed editing. Re-authoring the mesh again must keep
	//     working, so both the OLD and the NEW tower size must report themselves.
	TestEqual(TEXT("(c) ⭐ the PRE-redesign tower half-extent reports itself"), RadiusOf(FVector(1350.f, 1350.f, 2078.f)), 1350.f, Tolerance);
	TestEqual(TEXT("(c) ⭐ the POST-redesign tower half-extent reports itself — same code, no edit"), RadiusOf(FVector(375.f, 375.f, 1200.f)), 375.f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⭐⭐ SCALED BOUNDS, ⛔ NEVER LOCAL BOUNDS (STACK-§6) — the one word that
//     `TASK-815`'s wheel makes load-bearing
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementFootprintUsesScaledBoundsNeverLocalBoundsTest,
	"Siegebound.Placement.FootprintRadiusReadsScaledBoundsSoAWheeledGhostIsValidatedAtTheSizeItIsDrawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementFootprintUsesScaledBoundsNeverLocalBoundsTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	const FVector LocalExtent(375.f, 260.f, 1200.f);
	const float LocalRadius = RadiusOf(LocalExtent);

	// (a) At the SHIPPED scale of 1 the scaled read equals the local read EXACTLY. ⭐ This is
	//     what makes "read scaled bounds" correct TODAY and not speculative work for a feature
	//     that has not landed.
	const FVector UnscaledExtent = ScaledExtent(LocalExtent, FTransform(FVector(1000.f, -2000.f, 90.f)));
	TestEqual(TEXT("(a) ⭐ at scale 1 the scaled bounds equal the local bounds EXACTLY — translation alone changes nothing"),
		RadiusOf(UnscaledExtent), LocalRadius, Tolerance);

	// (b) ⭐⭐ AT THE WHEEL'S MAXIMUM THE ANSWER MUST MOVE. `TASK-815` scales the ghost up to
	//     ×1.5; a LOCAL read would hand the validator the ×1.0 size and silently clear a
	//     building 50% larger than the one on screen — the exact defect class TOWER-§7 exists
	//     to close. ⇒ this claim FAILS the instant anyone swaps CalcBounds for
	//     UStaticMesh::GetBounds().
	const float MaxWheelScale = 1.5f;
	const FVector WheeledExtent = ScaledExtent(LocalExtent, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(MaxWheelScale)));
	const float WheeledRadius = RadiusOf(WheeledExtent);

	TestEqual(TEXT("(b) ⭐⭐ a x1.5 ghost is validated at x1.5 — the scaled read"),
		WheeledRadius, LocalRadius * MaxWheelScale, Tolerance);
	TestTrue(TEXT("(b) ⛔ …and it is emphatically NOT the local radius (a local read would validate a 1.5x building at 1.0x)"),
		DiffersFrom(WheeledRadius, LocalRadius));
	TestTrue(TEXT("(b) ⛔ scaling UP must grow the footprint, never shrink or hold it"),
		WheeledRadius > LocalRadius);

	// (c) …and scaling DOWN must shrink it, so the term is a real multiplier rather than a
	//     one-way clamp that happens to pass (b).
	const FVector ShrunkExtent = ScaledExtent(LocalExtent, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.5f)));
	TestTrue(TEXT("(c) a x0.5 ghost yields a strictly SMALLER footprint"),
		RadiusOf(ShrunkExtent) < LocalRadius);

	// (d) THE ROTATION NOTE, MADE A CLAIM RATHER THAN A PROMISE: the ghost's only rotation is
	//     GhostYawOffset, an exact quarter turn. It swaps X and Y, so max(|X|,|Y|) is
	//     invariant and the shipped footprint is EXACT today — the reason the world-AABB read
	//     costs nothing at the shipped yaw.
	const FVector YawedExtent = ScaledExtent(LocalExtent, FTransform(FRotator(0.f, -90.f, 0.f)));
	TestEqual(TEXT("(d) the shipped -90 yaw leaves the footprint radius invariant (it swaps X and Y)"),
		RadiusOf(YawedExtent), LocalRadius, RotationTolerance);

	// (e) …and an OFF-AXIS yaw grows it, which is the CONSERVATIVE direction: such a ghost
	//     over-refuses and can never under-refuse. Declared as a follow-on, ⛔ not fixed.
	const FVector DiagonalExtent = ScaledExtent(LocalExtent, FTransform(FRotator(0.f, 45.f, 0.f)));
	TestTrue(TEXT("(e) an off-axis yaw makes the radius LARGER — it over-refuses, never under-refuses"),
		RadiusOf(DiagonalExtent) > LocalRadius);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ⭐ THE NON-REGRESSION — every SHIPPED SMALL BUILDING answers BYTE-FOR-BYTE
//     as it did before this task, and the control proves the claim is not vacuous
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementSmallFootprintsReproduceTheShippedClearanceExactlyTest,
	"Siegebound.Placement.EverySmallFootprintReproducesTheShippedBuildingClearanceByteForByte",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementSmallFootprintsReproduceTheShippedClearanceExactlyTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	// ⭐ READ THE SHIPPED NUMBER, ⛔ DO NOT TRANSCRIBE IT. Reflection also proves the property
	// still exists under the name TOWER-§7 and the task spec both pin — a rename or retype
	// would make every claim below vacuous, so it is an ERROR rather than a silent skip.
	float ShippedBuildingClearance = 0.f;
	if (!TryReadShippedFloat(TEXT("BuildingClearance"), ShippedBuildingClearance))
	{
		AddError(TEXT("⛔ BuildingClearance is not a float UPROPERTY on ASiegePlayerController under the name the spec pins."));
		return false;
	}

	TestTrue(TEXT("(pre) the shipped building clearance is a positive number to reason about"),
		ShippedBuildingClearance > 0.f);

	// (a) ⭐⭐ THE CLAIM THAT MAKES THIS SAFE TO SHIP ON A LIVE PATH. Sweep the whole range a
	//     small building can occupy — up to and INCLUDING the shipped clearance itself — and
	//     demand the composition hand back the shipped number with tolerance ZERO. Every
	//     existing small building therefore cannot change behaviour; the argument is
	//     structural, not a promise.
	const float SmallFractions[] = { 0.f, 0.01f, 0.25f, 0.5f, 0.75f, 0.99f, 1.f };
	for (const float Fraction : SmallFractions)
	{
		const float SmallRadius = ShippedBuildingClearance * Fraction;
		TestEqual(*FString::Printf(TEXT("(a) ⭐ a footprint at %.2fx the shipped clearance still answers the shipped clearance EXACTLY"), Fraction),
			ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, SmallRadius),
			ShippedBuildingClearance, Exact);
	}

	// (b) ⛔⛔ THE CONTROL, AND WITHOUT IT (a) IS A FAKE GATE (SHIP-§9c clause 1): a function
	//     that ALWAYS returns the shipped clearance passes every line above. A footprint
	//     LARGER than the clearance must move the answer — that is the entire fix.
	const float LargeRadius = ShippedBuildingClearance * 6.75f; // TOWER-§7's own "6.75x short" figure, as a ratio rather than a magnitude
	TestEqual(TEXT("(b) ⛔ CONTROL — a footprint LARGER than the clearance moves the answer to the footprint"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, LargeRadius),
		LargeRadius, Tolerance);
	TestTrue(TEXT("(b) ⛔ CONTROL — …and that answer is strictly greater than the shipped clearance"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, LargeRadius) > ShippedBuildingClearance);

	// (c) The boundary itself: exactly AT the clearance the two agree, so there is no step.
	TestEqual(TEXT("(c) at exactly the shipped clearance the composition is continuous"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, ShippedBuildingClearance),
		ShippedBuildingClearance, Exact);

	// (d) The degrade path — a missing/degenerate ghost mesh supplies radius 0, which must be
	//     the shipped rule byte-for-byte. This is the arithmetic half of "degrades OPEN".
	TestEqual(TEXT("(d) ⛔ radius 0 (the degrade path) is the shipped clearance, byte-for-byte"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, 0.f),
		ShippedBuildingClearance, Exact);
	TestEqual(TEXT("(d) a negative radius sanitises rather than shrinking the clearance"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, -500.f),
		ShippedBuildingClearance, Exact);
	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(d) pre — the NaN fixture really is NaN"), FMath::IsNaN(NaNValue));
	TestEqual(TEXT("(d) a NaN radius sanitises rather than poisoning the comparison"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, NaNValue),
		ShippedBuildingClearance, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. ⭐ IT IS A **MAX**, ⛔ NOT A SUM — the choice that keeps the shipped 200
//     from changing every existing building's feel
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementBuildingClearanceComposesAsMaxNotSumTest,
	"Siegebound.Placement.BuildingClearanceComposesWithTheFootprintAsAMaxAndNeverAsASum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementBuildingClearanceComposesAsMaxNotSumTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	float ShippedBuildingClearance = 0.f;
	if (!TryReadShippedFloat(TEXT("BuildingClearance"), ShippedBuildingClearance))
	{
		AddError(TEXT("⛔ BuildingClearance is not a float UPROPERTY on ASiegePlayerController under the name the spec pins."));
		return false;
	}

	// (a) A footprint BELOW the clearance: a sum would push every shipped building apart by its
	//     own size and change months-old content's feel. The max must not.
	const float BelowRadius = ShippedBuildingClearance * 0.75f;
	TestEqual(TEXT("(a) ⭐ below the clearance the composition is the clearance, not clearance + footprint"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, BelowRadius),
		ShippedBuildingClearance, Exact);
	TestTrue(TEXT("(a) ⛔ …and it is explicitly NOT the sum"),
		DiffersFrom(ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, BelowRadius),
			ShippedBuildingClearance + BelowRadius));

	// (b) A footprint ABOVE the clearance: the answer is the footprint, still not the sum.
	const float AboveRadius = ShippedBuildingClearance * 3.f;
	TestEqual(TEXT("(b) above the clearance the composition is the footprint"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, AboveRadius),
		AboveRadius, Tolerance);
	TestTrue(TEXT("(b) ⛔ …and it is explicitly NOT the sum"),
		DiffersFrom(ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, AboveRadius),
			ShippedBuildingClearance + AboveRadius));

	// (c) Monotonic and never below the shipped floor — the composition may only ever ADD
	//     safety, never remove it. A "min" implementation fails here loudly.
	TestTrue(TEXT("(c) ⛔ the composition can never fall BELOW the shipped clearance (a min would)"),
		ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, 0.f) >= ShippedBuildingClearance
		&& ASiegePlayerController::EffectiveBuildingClearance(ShippedBuildingClearance, AboveRadius) >= ShippedBuildingClearance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⭐⭐ THE UNIT REFUSAL DISCRIMINATES — and every refusal here is paired with
//     a CONTROL THAT MUST BE ADMITTED (the trivial-pass guard)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUnitFootprintRefusalDiscriminatesTest,
	"Siegebound.Placement.ALargeFootprintRefusesAUnitOccupiedPointWhileASmallOneAdmitsTheSamePoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUnitFootprintRefusalDiscriminatesTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	float ShippedBuildingClearance = 0.f;
	float ShippedUnitClearance = 0.f;
	if (!TryReadShippedFloat(TEXT("BuildingClearance"), ShippedBuildingClearance)
		|| !TryReadShippedFloat(TEXT("UnitPlacementClearance"), ShippedUnitClearance))
	{
		AddError(TEXT("⛔ BuildingClearance and/or UnitPlacementClearance is not a float UPROPERTY on ASiegePlayerController under the names TASK-735 pins."));
		return false;
	}

	// ONE fixture, held fixed across every claim below: the cursor at the origin and one own
	// unit standing 1.5x the shipped clearance away from it. ⭐ Nothing about the unit moves
	// between (a) and (b) — ONLY the footprint does — which is what makes (b) a control rather
	// than a second test.
	const FVector PlacementPoint = FVector::ZeroVector;
	const float UnitDistance = ShippedBuildingClearance * 1.5f;
	const FVector UnitPoint(UnitDistance, 0.f, 0.f);

	const float SmallFootprint = ShippedBuildingClearance * 0.5f;  // a wall: comfortably clear of the unit
	const float LargeFootprint = ShippedBuildingClearance * 4.f;   // a tower: the unit is well inside it
	const float NoBody = 0.f;

	// (a) ⭐ THE FIX ITSELF: a large structure dropped where a unit stands is REFUSED. Before
	//     this task nothing in the validation had a unit term at all, so this was silently
	//     legal and the unit was depenetrated by the movement component after the fact.
	TestTrue(TEXT("(a) ⭐ a LARGE footprint contains the unit — the placement is refused"),
		ASiegePlayerController::IsInsidePlacementFootprint(UnitPoint, PlacementPoint, LargeFootprint, NoBody, ShippedUnitClearance));

	// (b) ⛔⛔ THE MANDATED CONTROL — THIS ONE MUST BE **ADMITTED**. Same unit, same distance,
	//     same tunables: only the footprint shrinks. If this also refused, (a) would prove
	//     nothing — a "refuse everything" implementation passes (a) and fails here.
	TestFalse(TEXT("(b) ⛔ CONTROL — the SAME unit at the SAME distance is ADMITTED under a SMALL footprint"),
		ASiegePlayerController::IsInsidePlacementFootprint(UnitPoint, PlacementPoint, SmallFootprint, NoBody, ShippedUnitClearance));

	// (c) ⛔ THE SECOND CONTROL — same LARGE footprint, unit moved clear. A "refuse whenever
	//     the footprint is big" implementation passes (a) and (b) and fails here.
	const FVector DistantUnitPoint(LargeFootprint * 2.f, 0.f, 0.f);
	TestFalse(TEXT("(c) ⛔ CONTROL — the SAME large footprint ADMITS a unit standing outside it"),
		ASiegePlayerController::IsInsidePlacementFootprint(DistantUnitPoint, PlacementPoint, LargeFootprint, NoBody, ShippedUnitClearance));

	// (d) ⭐ THE SHIPPED TUNABLE IS 0, AND THE GATE IS STILL LIVE. Recorded as a claim because
	//     a 0-valued tunable invites the reading "this gate does nothing" — it is the FOOTPRINT
	//     that carries the refusal, and the tunable is only the retune lever T-6 is owed. This
	//     FAILS if anyone ever drops the footprint term and leaves the gate keyed on the pad.
	TestEqual(TEXT("(d) UnitPlacementClearance ships at 0 — the minimum new refusal"),
		ShippedUnitClearance, 0.f, Exact);
	TestTrue(TEXT("(d) ⭐ …and the gate still refuses at that 0, because the FOOTPRINT is what carries it"),
		ASiegePlayerController::IsInsidePlacementFootprint(UnitPoint, PlacementPoint, LargeFootprint, NoBody, 0.f));

	// (e) The rule is PLANAR, like every clearance beside it: a unit far above or below the
	//     cursor at the same XY is still inside the footprint (the arena floor is the surface,
	//     and a building materialises through the whole column).
	const FVector HighUnitPoint(UnitDistance, 0.f, 5000.f);
	TestTrue(TEXT("(e) the footprint test is 2D — a large Z offset does not escape it"),
		ASiegePlayerController::IsInsidePlacementFootprint(HighUnitPoint, PlacementPoint, LargeFootprint, NoBody, ShippedUnitClearance));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. THE BODY RADIUS AND THE PAD ARE REAL TERMS — each can flip the answer on
//     its own, and the boundary is EXCLUSIVE like the shipped clearances
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementFootprintTermsEachFlipTheAnswerTest,
	"Siegebound.Placement.TheUnitBodyRadiusAndTheClearancePadEachIndependentlyFlipTheRefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementFootprintTermsEachFlipTheAnswerTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	const FVector PlacementPoint = FVector::ZeroVector;
	const float Footprint = 400.f;

	// A unit standing JUST outside the bare footprint — 10 uu clear of it.
	const float Margin = 10.f;
	const FVector UnitPoint(Footprint + Margin, 0.f, 0.f);

	// (a) ⛔ CONTROL THAT MUST BE ADMITTED: with no body and no pad, that unit is outside.
	TestFalse(TEXT("(a) ⛔ CONTROL — bare footprint, no body, no pad: the unit just outside is ADMITTED"),
		ASiegePlayerController::IsInsidePlacementFootprint(UnitPoint, PlacementPoint, Footprint, 0.f, 0.f));

	// (b) ⭐ THE BODY RADIUS ALONE FLIPS IT. A unit is refused when the building would
	//     materialise THROUGH ITS BODY, not merely over its infinitely-thin centre — which is
	//     why the capsule radius is read off the unit and never assumed away.
	TestTrue(TEXT("(b) ⭐ the same geometry REFUSES once the unit's own body radius is counted"),
		ASiegePlayerController::IsInsidePlacementFootprint(UnitPoint, PlacementPoint, Footprint, Margin * 2.f, 0.f));

	// (c) ⭐ THE PAD ALONE FLIPS IT TOO — independently, with the body back at 0. This is what
	//     makes UnitPlacementClearance a live retune lever rather than decoration.
	TestTrue(TEXT("(c) ⭐ the same geometry REFUSES once the clearance pad is raised, with no body at all"),
		ASiegePlayerController::IsInsidePlacementFootprint(UnitPoint, PlacementPoint, Footprint, 0.f, Margin * 2.f));

	// (d) The boundary is EXCLUSIVE (<), matching HasBuildingClearance and HasObstacleClearance
	//     character for character: a unit standing EXACTLY on the edge is admitted.
	const FVector EdgeUnitPoint(Footprint, 0.f, 0.f);
	TestFalse(TEXT("(d) exactly ON the footprint edge is ADMITTED — the shipped strict-less-than boundary"),
		ASiegePlayerController::IsInsidePlacementFootprint(EdgeUnitPoint, PlacementPoint, Footprint, 0.f, 0.f));
	TestTrue(TEXT("(d) …and one unit inside it is refused"),
		ASiegePlayerController::IsInsidePlacementFootprint(FVector(Footprint - 1.f, 0.f, 0.f), PlacementPoint, Footprint, 0.f, 0.f));

	// (e) The terms COMPOSE: neither alone would refuse here, but together they do.
	const FVector FartherUnitPoint(Footprint + (Margin * 3.f), 0.f, 0.f);
	TestFalse(TEXT("(e) body alone is not enough at this distance"),
		ASiegePlayerController::IsInsidePlacementFootprint(FartherUnitPoint, PlacementPoint, Footprint, Margin * 2.f, 0.f));
	TestFalse(TEXT("(e) pad alone is not enough at this distance"),
		ASiegePlayerController::IsInsidePlacementFootprint(FartherUnitPoint, PlacementPoint, Footprint, 0.f, Margin * 2.f));
	TestTrue(TEXT("(e) ⭐ …but the two together refuse — the three terms are summed, not maxed"),
		ASiegePlayerController::IsInsidePlacementFootprint(FartherUnitPoint, PlacementPoint, Footprint, Margin * 2.f, Margin * 2.f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⛔ DEGRADE OPEN — a missing ghost mesh must behave as it did before this
//     task, and must NEVER manufacture a refusal
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementDegradesOpenOnAMissingGhostTest,
	"Siegebound.Placement.AMissingOrDegenerateGhostMeshDegradesOpenAndNeverManufacturesARefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementDegradesOpenOnAMissingGhostTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	const FVector PlacementPoint = FVector::ZeroVector;

	// (a) A zero footprint can contain nothing — not even a unit standing exactly on the
	//     cursor. ⛔ HOUSE NULL-SAFETY LAW: a missing asset must never REFUSE.
	TestFalse(TEXT("(a) ⛔ a zero footprint admits a unit standing exactly on the placement point"),
		ASiegePlayerController::IsInsidePlacementFootprint(PlacementPoint, PlacementPoint, 0.f, 0.f, 0.f));

	// (b) Negative and non-finite inputs sanitise rather than propagating. A NaN radius would
	//     otherwise make every comparison false in one direction and true in another.
	TestFalse(TEXT("(b) a negative footprint sanitises to nothing-contained"),
		ASiegePlayerController::IsInsidePlacementFootprint(FVector(10.f, 0.f, 0.f), PlacementPoint, -900.f, 0.f, 0.f));
	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(b) pre — the NaN fixture really is NaN (a folded constant would make the next two claims vacuous)"),
		FMath::IsNaN(NaNValue));
	TestFalse(TEXT("(b) a NaN footprint sanitises to nothing-contained"),
		ASiegePlayerController::IsInsidePlacementFootprint(FVector(10.f, 0.f, 0.f), PlacementPoint, NaNValue, 0.f, 0.f));
	TestFalse(TEXT("(b) a NaN body radius sanitises to nothing-contained"),
		ASiegePlayerController::IsInsidePlacementFootprint(FVector(10.f, 0.f, 0.f), PlacementPoint, 0.f, NaNValue, 0.f));

	// (c) ⛔ CONTROL THAT MUST REFUSE — without it, (a) and (b) are satisfied by a function
	//     that returns false unconditionally, which would silently delete the whole feature.
	TestTrue(TEXT("(c) ⛔ CONTROL — a real footprint with a real overlap still REFUSES"),
		ASiegePlayerController::IsInsidePlacementFootprint(FVector(10.f, 0.f, 0.f), PlacementPoint, 900.f, 0.f, 0.f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ⭐ THE NEW REFUSAL IS ITS OWN — ⛔ NOT `Clearance` REUSED. The player must
//     be told something they can act on
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUnitRefusalHasItsOwnMessageTest,
	"Siegebound.Placement.TheUnitFootprintRefusalCarriesItsOwnMessageAndNeverReusesAnExistingOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUnitRefusalHasItsOwnMessageTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	const FString UnitRefusal = ASiegePlayerController::UnitFootprintRefusalText().ToString();

	// (a) It says something.
	TestFalse(TEXT("(a) the unit-footprint refusal is not empty"), UnitRefusal.IsEmpty());

	// (b) ⭐⭐ AND IT IS NOT ANY OF THE FOUR SHIPPED REFUSALS. The reason enum is private, so
	//     the MESSAGE is the observable that tells the player which rule stopped them — and
	//     reusing "Too close to another building" for a unit overlap would be a lie the player
	//     cannot act on (they would go looking for a building that is not there).
	//     ⇒ this claim FAILS the instant anyone maps the new reason onto an existing message.
	//
	//     ⚠️ DECLARED RESIDUAL (SHIP-§9c clause 2): the four comparison strings are constructed
	//     here rather than read from the switch, because that switch has no headless seam. The
	//     failure this guards — the new reason reusing an existing message — is caught exactly;
	//     a later REWORDING of a shipped message would slip past it silently. TASK-816's diff
	//     read is the instrument for that, and it is named rather than left implied.
	const TCHAR* const ShippedRefusals[] =
	{
		TEXT("Too steep"),
		TEXT("Too close to obstacles"),
		TEXT("Too close to another building"),
		TEXT("Invalid placement location")
	};

	for (const TCHAR* const Shipped : ShippedRefusals)
	{
		TestNotEqual(*FString::Printf(TEXT("(b) ⭐ the unit refusal is NOT the shipped '%s' message"), Shipped),
			UnitRefusal, FString(Shipped));
	}

	// (c) It NAMES UNITS — the one word that makes the refusal actionable, since the only cure
	//     is to move the army or move the cursor (placement REFUSES and never pushes, NAV-§).
	TestTrue(TEXT("(c) the refusal names the thing in the way, so the player knows what to move"),
		UnitRefusal.Contains(TEXT("unit")));

	// (d) Stable across calls — it is a function-local static, and a fresh FText per call would
	//     mean the message is being rebuilt on a per-frame path.
	TestEqual(TEXT("(d) the message is stable across calls"),
		ASiegePlayerController::UnitFootprintRefusalText().ToString(), UnitRefusal);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  10. THE SHIPPED TUNABLES ARE STILL THE SHIPPED TUNABLES — TASK-735 was
//      forbidden to retune any of them, and this is the instrument for that
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementShippedTunablesAreUnchangedTest,
	"Siegebound.Placement.TheThreeShippedPlacementTunablesAreUnchangedByTheFootprintWork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementShippedTunablesAreUnchangedTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;

	// ⭐ These three are the numbers TOWER-§7 measured the finding against, and TASK-735 spec
	// (4) forbids changing the first of them ("changing that number changes every existing
	// building's feel and is out of fence"). Pinning them here means the footprint work cannot
	// quietly retune the rules it composes with — which is the ONE way this diff could regress
	// content that has been playable for months.
	struct FPinnedTunable
	{
		const TCHAR* Name;
		float Expected;
		const TCHAR* Why;
	};

	const FPinnedTunable Pinned[] =
	{
		{ TEXT("BuildingClearance"),         200.f, TEXT("GDD 3.5 — buildings require 200 units of clearance from any other building") },
		{ TEXT("ObstaclePlacementClearance"), 150.f, TEXT("GDD 5 (M4.5) — buildings need 150 units of clearance from obstacles") },
		{ TEXT("MaxPlacementSlopeDegrees"),   20.f, TEXT("GDD 5 (M4.5) — buildings refused on ground steeper than 20 degrees") }
	};

	for (const FPinnedTunable& Tunable : Pinned)
	{
		float Shipped = 0.f;
		if (!TryReadShippedFloat(Tunable.Name, Shipped))
		{
			AddError(*FString::Printf(TEXT("⛔ '%s' is not a float UPROPERTY on ASiegePlayerController — a rename or retype makes the composition claims vacuous."), Tunable.Name));
			continue;
		}
		TestEqual(*FString::Printf(TEXT("⭐ '%s' is UNCHANGED at its shipped value (%s)"), Tunable.Name, Tunable.Why),
			Shipped, Tunable.Expected, Exact);
	}

	// And the ONE new tunable exists, is a float UPROPERTY, and ships at its documented default.
	// ⛔ The existence check GUARDS the value check rather than sitting beside it: on a failed
	// read TryReadShippedFloat writes 0, which is exactly the value asserted below — so an
	// unguarded pair would report PASS for a property that had been deleted.
	float ShippedUnitClearance = 0.f;
	if (!TryReadShippedFloat(TEXT("UnitPlacementClearance"), ShippedUnitClearance))
	{
		AddError(TEXT("⛔ UnitPlacementClearance is not a float UPROPERTY on ASiegePlayerController — TASK-735's ONE new tunable (the T-6 retune lever) is missing, and the refusal has no EditDefaultsOnly threshold at all."));
		return false;
	}
	TestEqual(TEXT("⭐ UnitPlacementClearance ships at 0 — refuse real overlap and nothing more"),
		ShippedUnitClearance, 0.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════════════════════
//
//   ⭐⭐ TASK-813 — THE BLUE UPGRADE STATE (STACK-§0 / §2 / §3 / §5 / §7)
//
//   Jonathan, verbatim (2026-09-02): "if you hover directly on another tower, the outline
//   instead appears blue, which means you can place it there, and instead of placing a new
//   tower, it will instead double the height of the old one."
//
//   ⭐ EXTENDING THIS FILE, ⛔ NOT ADDING A SECOND PLACEMENT FRAME — TASK-735's handoff §6.7
//   named this file as the placement frame precisely so the stacking claims would land here.
//
//   ───────────────────────────────────────────────────────────────────────────────────────
//   ⛔⛔ THE TRAP THAT MAKES THESE TESTS WORTH WRITING, NAMED SO NOBODY HAS TO GO LOOKING
//   ───────────────────────────────────────────────────────────────────────────────────────
//
//   ⚠️ **A "SHOWS BLUE" TEST PASSES TRIVIALLY IF NOTHING WAS HOVERED.** "The state was not
//   Ready" is also the answer a resolver that returns None for EVERYTHING gives, and "the
//   state was Ready" is the answer one that returns Ready for everything gives. ⇒ ⭐ EVERY
//   claim below moves exactly ONE term of a fixed fixture and pins BOTH sides:
//     • the CONTROL THAT MUST COME OUT **GREEN** (⇒ None, i.e. the shipped green/red path
//       decides and nothing new happened): nothing hovered · a unit card · an ENEMY building ·
//       a DIFFERENT card's building · a destroyed one — tests 11(b)..(g);
//     • the CONTROL THAT MUST COME OUT **RED**: one gold short ⇒ Unaffordable (13a), and a
//       climbable tower ALSO one gold short ⇒ Unaffordable (12e);
//     • and every RED claim is paired with the SAME fixture coming out **BLUE** when the one
//       term under test is put back — 12(a), 13(b).
//   A resolver that answers None for everything fails 11(a), 12(a), 13(b), 14(a).
//   A resolver that answers Ready for everything fails 11(b)..(g), 12(b), 12(f), 13(a).
//   ⛔ Neither can pass this file.
//
//   ⚠️⚠️ TEST 12 CHANGED POLARITY ON 2026-09-03 AND THIS PARAGRAPH CHANGED WITH IT (STACK-§8).
//   It used to read "a climbable tower ⇒ NotStackable" as the red control — and that control
//   was ⛔ GREEN THE WHOLE TIME 🧑 Jonathan could not stack a tower. ⇒ ⚖️ *when a test's
//   polarity flips, its ANTI-FAKE ARGUMENT has to be re-derived, ⛔ not merely re-signed*: the
//   old (a) guarded against "a resolver that says Ready to everything", and the new (a) needs a
//   DIFFERENT partner — 12(b)/(f), the fixtures the resolver must ⛔ still turn away.
//
//   ⭐ AND THE SECOND ANTI-FAKE MEASURE: ⛔ no expectation here is transcribed from the spec.
//   The gold boundaries are expressed as cost ± 1 for THREE different costs (so a hardcoded
//   15 or 30 dies), the cap is WALKED with ApplyStackUpgrade until the series stops moving
//   (so the number 5 appears nowhere), and the colours are read off the CDO by reflection.
//
//   ───────────────────────────────────────────────────────────────────────────────────────
//   ⛔ WHAT IS **NOT** COVERED HERE — stated so nobody mistakes green for done (`SC-§32`)
//   ───────────────────────────────────────────────────────────────────────────────────────
//     • ⛔ THE CURSOR TRACE THAT PRODUCES THE HOVER TARGET. `UpdatePlacementGhost` needs a
//       world, a cursor and a `UMaterialInstanceDynamic`; there is no headless path to it and
//       this file does ⛔ not pretend otherwise. What IS asserted is the DECISION it feeds —
//       the pure resolver — plus the colour it would write. Its instruments are the QA diff
//       read (TASK-814) and Jonathan's playtest.
//     • ⛔ `ConfirmStackUpgrade` ITSELF (it spends gold off a live `ASiegePlayerState` and
//       consumes a hand slot). Test 14 asserts the two things it is built on — that the state
//       stays Ready at the cap, and that the click still buys health there.
//     • ⛔ THE ARITHMETIC. The two series, the cap and the HP delta rule are TASK-812's and
//       are asserted in `SiegeBuildingStackTest.cpp`. ⛔ Nothing here re-derives them; test 14
//       CONSUMES them.
//     • ⛔ No `Capture()`, no `EnsureSnapshot()`, Zone A untouched, ⛔ no token figure.
// ═══════════════════════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════════════════════

namespace SiegePlacementUpgradeFixture
{
	using EUpgradeState = ASiegePlayerController::EPlacementUpgradeState;

	/**
	 *  The card the fixture "holds". ⛔ Never a shipped CardID — only its IDENTITY is used, and
	 *  a name no cards.csv row carries means no claim here can accidentally agree with the
	 *  shipped data. Stored as literals rather than as file-scope `FName`s so nothing in this
	 *  fixture constructs a name at static-init time (the house FText caution, generalised).
	 */
	static const TCHAR* const HeldCard = TEXT("SiegeboundTestTowerA");
	static const TCHAR* const OtherCard = TEXT("SiegeboundTestTowerB");

	/** A cost that is ⛔ not any shipped card's, so no expectation below can accidentally agree with cards.csv. */
	constexpr int32 FixtureCost = 37;

	/**
	 *  ⭐ Readable enum comparisons. `TestEqual` has no enum-class overload, and casting to
	 *  uint8 would print "expected 1, got 0" — useless in a failure report. Naming the states
	 *  makes a red line say what actually happened.
	 */
	static const TCHAR* StateName(EUpgradeState State)
	{
		switch (State)
		{
		case EUpgradeState::None:         return TEXT("None (green/red — nothing new happened)");
		case EUpgradeState::Ready:        return TEXT("Ready (BLUE — the click upgrades)");
		case EUpgradeState::NotStackable: return TEXT("NotStackable (RED — this building refuses scaling)");
		case EUpgradeState::Unaffordable: return TEXT("Unaffordable (RED — not enough gold)");
		default:                          return TEXT("<UNKNOWN STATE — the enum grew and this helper did not>");
		}
	}

	static void CheckState(FAutomationTestBase& Test, const TCHAR* What, EUpgradeState Actual, EUpgradeState Expected)
	{
		Test.TestEqual(What, FString(StateName(Actual)), FString(StateName(Expected)));
	}

	/**
	 *  A transient, world-free building (the `SiegeBuildingStackTest::MakeScratchBuilding`
	 *  precedent, adopted rather than reinvented). ⛔ Never the CDO — test 14 MUTATES.
	 *  `InitBuilding` is safe here: `BeginPlay` never ran, so it sets Team + CardID and
	 *  returns without touching `DT_Cards`.
	 */
	template <typename TBuilding>
	static TStrongObjectPtr<TBuilding> MakeScratchBuilding(ETeamId Team, FName CardID)
	{
		TStrongObjectPtr<TBuilding> Built(
			NewObject<TBuilding>(GetTransientPackageAsObject(), TBuilding::StaticClass(), NAME_None, RF_Transient));
		if (Built.IsValid())
		{
			Built->InitBuilding(Team, CardID);
		}
		return Built;
	}

	/** Seeds a private reflected bool (bDestroyed). ⚠️ Transient instances ONLY — never the CDO. */
	static bool SeedBool(FAutomationTestBase& Test, UObject* Object, const TCHAR* PropertyName, bool Value)
	{
		const FBoolProperty* const Property = Object
			? CastField<FBoolProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)))
			: nullptr;
		if (!Property)
		{
			Test.AddError(FString::Printf(TEXT("SELF-CHECK FAILED: '%s' is not a reflected bool property — the seed could not be planted, so the claim it supports would be measuring nothing."), PropertyName));
			return false;
		}
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	static bool TryReadFloat(UObject* Object, const TCHAR* PropertyName, float& OutValue)
	{
		OutValue = 0.f;
		const FFloatProperty* const Property = Object
			? CastField<FFloatProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)))
			: nullptr;
		if (!Property)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Object);
		return true;
	}

	/** Reads a shipped `FLinearColor` UPROPERTY off the controller CDO, or returns false. */
	static bool TryReadShippedColor(const TCHAR* PropertyName, FLinearColor& OutValue)
	{
		OutValue = FLinearColor::Transparent;
		const ASiegePlayerController* const Defaults = GetDefault<ASiegePlayerController>();
		const FStructProperty* const Property =
			FindFProperty<FStructProperty>(ASiegePlayerController::StaticClass(), PropertyName);
		if (!Defaults || !Property || Property->Struct != TBaseStructure<FLinearColor>::Get())
		{
			return false;
		}
		OutValue = *Property->ContainerPtrToValuePtr<FLinearColor>(Defaults);
		return true;
	}

	/** The seam under test, spelled once so no test re-types its six arguments. */
	static EUpgradeState Resolve(const ABuilding* Hovered, ETeamId OwnTeam, FName Card, bool bIsBuilding, int32 Gold, int32 Cost)
	{
		return ASiegePlayerController::ResolvePlacementUpgradeState(Hovered, OwnTeam, Card, bIsBuilding, Gold, Cost);
	}

	/** The fixture's canonical BLUE call: own team, the held card, a building card, gold to spare. */
	static EUpgradeState ResolveBlueBaseline(const ABuilding* Hovered)
	{
		return Resolve(Hovered, ETeamId::Blue, HeldCard, /*bIsBuilding=*/ true, /*Gold=*/ FixtureCost * 4, FixtureCost);
	}

	/** Reads a shipped project source file. ⛔ A probe that cannot read its subject FAILS. */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath) || !FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — a stale probe FAILS rather than reporting safe."), *FullPath));
			return false;
		}
		return true;
	}

	/**
	 *  Occurrences of Needle on CODE lines only (the `SiegeBuildingStackTest` /
	 *  `SiegeClimbableTowerTest` helper, same shape and same reason): the paragraphs that
	 *  EXPLAIN why the WatchTower is excluded must not read as the name check they forbid.
	 */
	static int32 CountOccurrencesInCode(const FString& Source, const TCHAR* Needle)
	{
		const int32 NeedleLength = FCString::Strlen(Needle);
		if (NeedleLength <= 0)
		{
			return 0;
		}

		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		int32 Count = 0;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();
			const bool bIsCommentLine =
				Trimmed.StartsWith(TEXT("//"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("* "), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("*/"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("/*"), ESearchCase::CaseSensitive)
				|| Trimmed.Equals(TEXT("*"), ESearchCase::CaseSensitive);
			if (bIsCommentLine)
			{
				continue;
			}

			int32 From = 0;
			for (;;)
			{
				const int32 Found = Trimmed.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
				if (Found == INDEX_NONE)
				{
					break;
				}
				++Count;
				From = Found + NeedleLength;
			}
		}

		return Count;
	}

	static const TCHAR* const ControllerHeaderPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h");
	static const TCHAR* const ControllerSourcePath = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp");
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  11. ⭐⭐ THE STATE MACHINE PICKS **BLUE** ONLY FOR own-team + same-CardID + scalable +
//      affordable — and ⛔ EVERY OTHER HOVER FALLS BACK TO TODAY'S GREEN/RED
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUpgradeStateIsBlueOnlyForOwnTeamSameCardTest,
	"Siegebound.Placement.TheUpgradeStateIsBlueOnlyForAnOwnTeamSameCardScalableBuilding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUpgradeStateIsBlueOnlyForOwnTeamSameCardTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	TStrongObjectPtr<ABuilding> Own = MakeScratchBuilding<ABuilding>(ETeamId::Blue, HeldCard);
	TStrongObjectPtr<ABuilding> Enemy = MakeScratchBuilding<ABuilding>(ETeamId::Red, HeldCard);
	TStrongObjectPtr<ABuilding> Different = MakeScratchBuilding<ABuilding>(ETeamId::Blue, OtherCard);
	if (!Own.IsValid() || !Enemy.IsValid() || !Different.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: a scratch ABuilding could not be created — nothing below would mean anything."));
		return false;
	}

	// ── SELF-CHECK ON THE FIXTURE ITSELF ─────────────────────────────────────────────────────
	// ⚠️ Every row below is a statement about TEAM and CARD. If InitBuilding did not actually
	// plant them, the whole test would be comparing a building against itself and passing.
	TestEqual(TEXT("SELF-CHECK: the own-team fixture really is Blue"), static_cast<int32>(Own->GetTeamId()), static_cast<int32>(ETeamId::Blue));
	TestEqual(TEXT("SELF-CHECK: the enemy fixture really is Red"), static_cast<int32>(Enemy->GetTeamId()), static_cast<int32>(ETeamId::Red));
	TestTrue(TEXT("SELF-CHECK: the own-team fixture really carries the held card"), Own->GetCardID() == FName(HeldCard));
	TestTrue(TEXT("SELF-CHECK: the different-card fixture really carries a DIFFERENT card"), Different->GetCardID() != FName(HeldCard));
	TestTrue(TEXT("SELF-CHECK: a plain ABuilding really is scalable (so a None below is about the term under test, not about the predicate)"), Own->CanScaleFootprint());

	// (a) ⭐ THE ONE BLUE ROW. Everything else in this test is this fixture with ONE term moved.
	CheckState(*this, TEXT("(a) ⭐ own team + same card + scalable + affordable ⇒ BLUE"),
		ResolveBlueBaseline(Own.Get()), EUpgradeState::Ready);

	// (b) ⭐⭐ THE CONTROL THAT MUST COME OUT **GREEN** — ⛔ NOTHING HOVERED. This is the row
	//     that stops "shows blue" from passing trivially: with no building under the cursor the
	//     answer must be None, which hands the frame straight back to the five shipped gates.
	CheckState(*this, TEXT("(b) ⭐⭐ CONTROL — nothing hovered ⇒ None, i.e. the shipped green/red decides and ⛔ no blue is invented"),
		ResolveBlueBaseline(nullptr), EUpgradeState::None);

	// (c) ⛔ ENEMY BUILDING ⇒ None (J-7). ⚠️ Deliberately NOT a refusal state: the enemy case is
	//     already refused by the shipped clearance gate, and inventing a new reason for it
	//     would change a message that has been on screen for months.
	CheckState(*this, TEXT("(c) ⛔ an ENEMY building of the SAME card ⇒ None (J-7 — own-team only, and ⛔ no new refusal is invented)"),
		ResolveBlueBaseline(Enemy.Get()), EUpgradeState::None);

	// (d) A DIFFERENT CARD ⇒ None. An ArrowTower in hand does not grow a BombTower.
	CheckState(*this, TEXT("(d) an own-team building of a DIFFERENT card ⇒ None (today's behaviour, unchanged)"),
		ResolveBlueBaseline(Different.Get()), EUpgradeState::None);

	// (e) A UNIT CARD ⇒ None, even hovering a perfectly good target (J-8: ⛔ not units).
	CheckState(*this, TEXT("(e) ⛔ a UNIT card over a valid target ⇒ None (J-8 — only Building cards stack)"),
		Resolve(Own.Get(), ETeamId::Blue, HeldCard, /*bIsBuilding=*/ false, FixtureCost * 4, FixtureCost),
		EUpgradeState::None);

	// (f) NO PENDING CARD ⇒ None. Defensive, and it would fire if the resolver were ever called
	//     outside placement mode.
	CheckState(*this, TEXT("(f) no pending card (NAME_None) ⇒ None"),
		Resolve(Own.Get(), ETeamId::Blue, NAME_None, /*bIsBuilding=*/ true, FixtureCost * 4, FixtureCost),
		EUpgradeState::None);

	// (g) A DESTROYED BUILDING ⇒ None — and ⭐ the SAME instance answered Ready one line ago in
	//     (a), so this row is about bDestroyed and ⛔ nothing else.
	if (SeedBool(*this, Own.Get(), TEXT("bDestroyed"), true))
	{
		TestTrue(TEXT("SELF-CHECK: the seed took — the fixture really reports destroyed"), Own->IsBuildingDestroyed());
		CheckState(*this, TEXT("(g) ⭐ THE SAME building, now DESTROYED ⇒ None (a corpse is not an upgrade target)"),
			ResolveBlueBaseline(Own.Get()), EUpgradeState::None);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  12. ⭐⭐⭐ A CLIMBABLE TOWER **DOES** YIELD BLUE — the hover half of 🧑 JONATHAN'S BUG,
//      asserted on the very resolver that produced his refusal.
//
//  ⚠️⚠️ THIS TEST ASSERTED THE EXACT OPPOSITE UNTIL 2026-09-03, AND IT WAS ⛔ GREEN THE WHOLE
//  TIME HE COULD NOT STACK A TOWER. Its old name was
//  "AClimbableTowerNeverYieldsTheBlueUpgradeStateAndTheTowerFamilyStillDoes" and every row in
//  it passed. ⇒ ⚖️ *a suite measures what the code DOES, ⛔ never whether that is what was
//  wanted* — this file was faithfully protecting the defect, which is why the row is kept and
//  INVERTED rather than deleted.
//
//  ⭐ AND THE TINT FOLLOWS FOR FREE, WITH ⛔ ZERO TINT EDITS (`STACK-§9`): the shipped ternary
//  paints `UpgradeGhostColor` on `Ready` and nothing else changed. ⇒ the row below asserting
//  `Ready` IS the assertion that the ghost is now BLUE on his hover.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementClimbableTowerYieldsBlueTest,
	"Siegebound.Placement.AClimbableTowerYieldsTheBlueUpgradeStateWhileStillRefusingTheFootprintWheel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementClimbableTowerYieldsBlueTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	TStrongObjectPtr<AClimbableTower> Climbable = MakeScratchBuilding<AClimbableTower>(ETeamId::Blue, HeldCard);
	TStrongObjectPtr<ABuilding> Plain = MakeScratchBuilding<ABuilding>(ETeamId::Blue, HeldCard);
	TStrongObjectPtr<ATower> Firing = MakeScratchBuilding<ATower>(ETeamId::Blue, HeldCard);

	// ⭐ THE "STILL REFUSES SOMETHING" CONTROL — a building of a DIFFERENT card, which gate (4)
	// must keep turning away. ⛔ Its own fixture rather than a borrow from test 11: this test's
	// whole polarity now depends on it, and a control that lives in another test's scope is a
	// control nobody maintains.
	TStrongObjectPtr<ABuilding> Different = MakeScratchBuilding<ABuilding>(ETeamId::Blue, OtherCard);
	if (!Climbable.IsValid() || !Plain.IsValid() || !Firing.IsValid() || !Different.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: a scratch building could not be created."));
		return false;
	}

	// (a) ⭐⭐⭐ THE CLAIM, AND IT IS 🧑 THE PLAYTEST BUG IN ONE LINE. This is the exact hover he
	//     performed — his own team, the same card, gold to spare, cursor on his own climbable
	//     tower — and the shipped resolver answered `NotStackable`, which the ternary painted
	//     RED and the click turned into "That building cannot be stacked". It now answers
	//     `Ready`, which the SAME ternary paints ⭐ BLUE with ⛔ zero tint edits (`STACK-§9`).
	CheckState(*this, TEXT("(a) ⭐⭐⭐ an own-team, same-card, AFFORDABLE AClimbableTower ⇒ Ready (BLUE). ⛔ This row read 'NotStackable' until 2026-09-03 and was GREEN throughout the bug"),
		ResolveBlueBaseline(Climbable.Get()), EUpgradeState::Ready);

	// (b) ⭐⭐ THE CONTROL THAT MUST STILL COME OUT **RED**, AND WITHOUT IT (a) IS WORTHLESS.
	//     ⚠️ THE POLARITY OF THIS TEST FLIPPED, SO ITS ANTI-FAKE ARGUMENT HAD TO FLIP WITH IT:
	//     "it was Ready" is also the answer a resolver that has ⛔ stopped refusing ANYTHING
	//     gives — including one where somebody deleted gate (5) outright. ⇒ the discriminating
	//     control is now a fixture that must ⛔ STILL be refused, and gate (3) supplies one that
	//     needs no new machinery: an ENEMY building. (f) below carries the same weight.
	CheckState(*this, TEXT("(b) ⭐⭐ CONTROL — this resolver still REFUSES things: an own-team building of a DIFFERENT card ⇒ None. Without this row, (a) would pass against a resolver that had lost gate (5) entirely"),
		ResolveBlueBaseline(Different.Get()), EUpgradeState::None);

	// (c) ⭐ THE FAMILY HE WAS NEVER POINTING AT IS UNCHANGED. `ATower` is the
	//     ArrowTower/BombTower/BallistaTower/CrystalTower family, and it stacked before this
	//     change and stacks after it — the split moved ⛔ one class's answer, ⛔ not the rule.
	CheckState(*this, TEXT("(c) ⭐ an ATower (the Arrow/Bomb/Ballista/Crystal family) ⇒ BLUE, exactly as before — the split changed ⛔ nothing for them"),
		ResolveBlueBaseline(Firing.Get()), EUpgradeState::Ready);
	CheckState(*this, TEXT("(c) ⭐ …and a plain ABuilding likewise"),
		ResolveBlueBaseline(Plain.Get()), EUpgradeState::Ready);

	// (d) ⭐⭐⭐ THE SPLIT, OBSERVED ON ONE INSTANCE THROUGH THE ⛔ ONE POINTER TYPE THE
	//     PLACEMENT PATH EVER HOLDS. The resolver's parameter is `const ABuilding*`, so asking
	//     both predicates through that pointer is the only way to see virtual dispatch at all —
	//     a SHADOWED non-virtual would pass every other row and fail these.
	//
	//     ⛔⛔ AND THE PAIR IS THE POINT: the tower refuses the WHEEL and accepts the STACK.
	//     ⛔ A WRAPPER — `CanStackHeight() { return CanScaleFootprint(); }` — could ⛔ never
	//     produce this pair, whatever it was named (`STACK-§8` cl. 3).
	const ABuilding* const AsBase = Climbable.Get();
	TestFalse(TEXT("(d) ⛔ CanScaleFootprint() is STILL false through an ABuilding* — the WHEEL exclusion was ⛔ NOT reopened (he reported stacking, ⛔ never resizing)"),
		AsBase->CanScaleFootprint());
	TestTrue(TEXT("(d) ⭐⭐ …while CanStackHeight() is TRUE through the same pointer — two virtuals, two INDEPENDENT answers"),
		AsBase->CanStackHeight());
	CheckState(*this, TEXT("(d) ⭐ …and the resolver, which only ever sees an ABuilding*, now says Ready"),
		ResolveBlueBaseline(AsBase), EUpgradeState::Ready);

	// (e) ⚠️ THE ORDERING RULING SURVIVES THE SPLIT, AND IT IS NOW ASKED THE OTHER WAY ROUND:
	//     with gate (5) no longer refusing this class, a climbable tower the player cannot
	//     afford must fall through to `Unaffordable` — the same answer any other building gives.
	//     ⛔ A resolver still refusing the tower at gate (5) would answer `NotStackable` here
	//     and this row would catch it even if (a) somehow did not.
	CheckState(*this, TEXT("(e) ⚠️ a climbable tower the player cannot afford ⇒ Unaffordable, exactly like every other building — it is no longer a special case at all"),
		Resolve(Climbable.Get(), ETeamId::Blue, HeldCard, /*bIsBuilding=*/ true, /*Gold=*/ 0, FixtureCost),
		EUpgradeState::Unaffordable);

	// (f) ⛔ AND AN ENEMY CLIMBABLE TOWER IS STILL JUST "None" — the team gate is asked BEFORE
	//     the predicate, so hovering the enemy's watch tower does not leak our refusal text.
	TStrongObjectPtr<AClimbableTower> EnemyClimbable = MakeScratchBuilding<AClimbableTower>(ETeamId::Red, HeldCard);
	if (EnemyClimbable.IsValid())
	{
		CheckState(*this, TEXT("(f) ⛔ an ENEMY climbable tower ⇒ None — the team gate is asked first, so no refusal text leaks onto an enemy building"),
			ResolveBlueBaseline(EnemyClimbable.Get()), EUpgradeState::None);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  13. ⛔ INSUFFICIENT GOLD YIELDS **RED**, ⛔ NEVER A BLUE THAT REFUSES (J-5) — and the price
//      is the CARD's, so ⛔ no literal survives
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUpgradeGoldGateTest,
	"Siegebound.Placement.InsufficientGoldYieldsRedNotBlueAndTheBoundaryTracksTheCardsOwnCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUpgradeGoldGateTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	TStrongObjectPtr<ABuilding> Own = MakeScratchBuilding<ABuilding>(ETeamId::Blue, HeldCard);
	if (!Own.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: a scratch ABuilding could not be created."));
		return false;
	}

	// ⭐⭐ THREE DIFFERENT COSTS, EACH FLIPPING AT ITS **OWN** BOUNDARY. This is the instrument
	// for "the cost is the card's own DT_Cards Cost, ⛔ never a literal" (J-2): any hardcoded
	// price — 15 (ArrowTower), 24, 27, 30 (WatchTower), or the fixture's own 37 — puts the flip
	// in the wrong place for at least two of these three rows.
	const int32 Costs[] = { 1, FixtureCost, 250 };
	for (const int32 Cost : Costs)
	{
		// (a) ⭐ THE CONTROL THAT MUST COME OUT **RED**: exactly one gold short.
		CheckState(*this, *FString::Printf(TEXT("(a) ⭐ cost %d, gold %d (ONE short) ⇒ Unaffordable — ⛔ RED, never a blue that refuses (J-5)"), Cost, Cost - 1),
			Resolve(Own.Get(), ETeamId::Blue, HeldCard, true, Cost - 1, Cost), EUpgradeState::Unaffordable);

		// (b) ⭐⭐ THE PAIRED CONTROL THAT MUST COME OUT **BLUE**: the same fixture with exactly
		//     enough. Without it, (a) would also pass for a resolver that never returns Ready.
		//     ⚠️ AND IT PINS THE BOUNDARY AS `<`, NOT `<=`: exact change must BUY the upgrade.
		CheckState(*this, *FString::Printf(TEXT("(b) ⭐⭐ CONTROL — cost %d, gold %d (EXACT change) ⇒ BLUE; the boundary is '<', so exact change buys it"), Cost, Cost),
			Resolve(Own.Get(), ETeamId::Blue, HeldCard, true, Cost, Cost), EUpgradeState::Ready);

		// (c) …and comfortably above.
		CheckState(*this, *FString::Printf(TEXT("(c) cost %d, gold %d ⇒ BLUE"), Cost, Cost + 1),
			Resolve(Own.Get(), ETeamId::Blue, HeldCard, true, Cost + 1, Cost), EUpgradeState::Ready);
	}

	// (d) A FREE CARD IS AFFORDABLE AT ZERO GOLD — the degenerate row, isolated and labelled so
	//     it is not mistaken for a discriminating one.
	CheckState(*this, TEXT("(d) cost 0, gold 0 ⇒ BLUE (a free upgrade is affordable)"),
		Resolve(Own.Get(), ETeamId::Blue, HeldCard, true, /*Gold=*/ 0, /*Cost=*/ 0), EUpgradeState::Ready);

	// (e) ⚠️ THE GOLD GATE IS ASKED **LAST**, so it can never mask a structural refusal: an
	//     ENEMY building the player also cannot afford is still None, ⛔ not Unaffordable.
	TStrongObjectPtr<ABuilding> Enemy = MakeScratchBuilding<ABuilding>(ETeamId::Red, HeldCard);
	if (Enemy.IsValid())
	{
		CheckState(*this, TEXT("(e) ⚠️ an ENEMY building the player cannot afford ⇒ None — gold is asked LAST and never masks a structural answer"),
			Resolve(Enemy.Get(), ETeamId::Blue, HeldCard, true, /*Gold=*/ 0, FixtureCost), EUpgradeState::None);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  14. ⭐ AT THE HEIGHT CAP THE STATE IS **STILL BLUE**, AND THE CLICK STILL BUYS HEALTH (J-6)
//      — ⛔ the number 5 appears nowhere; the cap is WALKED until the series stops moving
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUpgradeStaysBlueAtTheHeightCapTest,
	"Siegebound.Placement.AtTheHeightCapTheStateIsStillBlueAndTheClickStillBuysHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUpgradeStaysBlueAtTheHeightCapTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	TStrongObjectPtr<ABuilding> Own = MakeScratchBuilding<ABuilding>(ETeamId::Blue, HeldCard);
	if (!Own.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: a scratch ABuilding could not be created."));
		return false;
	}

	// Seed a real HP pool so "the click still buys health" is measurable at all.
	const FFloatProperty* const MaxHPProperty = CastField<FFloatProperty>(ABuilding::StaticClass()->FindPropertyByName(TEXT("MaxHP")));
	const FFloatProperty* const CurrentHPProperty = CastField<FFloatProperty>(ABuilding::StaticClass()->FindPropertyByName(TEXT("CurrentHP")));
	if (!MaxHPProperty || !CurrentHPProperty)
	{
		AddError(TEXT("SELF-CHECK FAILED: MaxHP/CurrentHP are not reflected floats on ABuilding — the health half of this claim could not be measured."));
		return false;
	}
	MaxHPProperty->SetPropertyValue_InContainer(Own.Get(), 100.f);
	CurrentHPProperty->SetPropertyValue_InContainer(Own.Get(), 100.f);

	// ── WALK TO THE CAP. ⛔ NO NUMBER FROM THE SPEC IS USED: the loop stops when the HEIGHT
	// SERIES STOPS MOVING, whatever MaxStackHeightMultiplier happens to be. ⇒ ⭐ a retune of the
	// cap moves this test with the code instead of against it.
	// ⭐ THE CEILING COMES OFF **THIS INSTANCE** (STACK-§10 cl. 2 — it is per class now, and
	// `StackHeightMultiplier` is HANDED it rather than reading a CDO). ⛔ Still no number from
	// the spec: the loop stops when this building's own series stops moving.
	const int32 OwnHeightCap = Own->GetMaxStackHeightMultiplier();
	int32 Guard = 0;
	while (ABuilding::StackHeightMultiplier(Own->GetStackUpgradeCount() + 1, OwnHeightCap)
		 > ABuilding::StackHeightMultiplier(Own->GetStackUpgradeCount(), OwnHeightCap)
		&& Guard++ < 64)
	{
		// ⭐ THE STATE MUST BE BLUE ALL THE WAY UP — a "refuse once it is tall" implementation
		// dies here rather than at the cap.
		CheckState(*this, *FString::Printf(TEXT("(a) below the cap (n = %d) ⇒ BLUE"), Own->GetStackUpgradeCount()),
			ResolveBlueBaseline(Own.Get()), EUpgradeState::Ready);

		if (!Own->ApplyStackUpgrade())
		{
			AddError(TEXT("SELF-CHECK FAILED: ApplyStackUpgrade refused on a plain scratch building while walking to the cap — the walk below the cap never happened."));
			return false;
		}
	}

	// ── SELF-CHECKS ON THE WALK, so the claims after it are not vacuous ──────────────────────
	TestTrue(TEXT("SELF-CHECK: the walk actually took at least two upgrades (n <= 1 discriminates nothing about a cap)"),
		Own->GetStackUpgradeCount() >= 2);
	TestTrue(TEXT("SELF-CHECK: the walk terminated on the CAP and not on the guard"), Guard < 64);
	const float CappedHeight = ABuilding::StackHeightMultiplier(Own->GetStackUpgradeCount(), OwnHeightCap);
	TestEqual(TEXT("SELF-CHECK: the height series really has saturated — one more upgrade would move it ⛔ not at all"),
		ABuilding::StackHeightMultiplier(Own->GetStackUpgradeCount() + 1, OwnHeightCap), CappedHeight, 0.f);

	// (b) ⭐⭐ THE CLAIM: AT the cap, the state is STILL BLUE. His own sentence — "at some point
	//     if they keep upgrading it would only upgrade health by 1.5 times and not height" —
	//     and a refusal here would be the silent behaviour change STACK-§5 refused.
	CheckState(*this, TEXT("(b) ⭐⭐ AT the height cap ⇒ STILL BLUE — the click still does something (J-6)"),
		ResolveBlueBaseline(Own.Get()), EUpgradeState::Ready);

	// (c) ⭐ AND THE BLUE IS NOT EMPTY: the click at the cap still buys health. ⚠️ The ARITHMETIC
	//     is TASK-812's and is asserted in SiegeBuildingStackTest — what is asserted HERE is
	//     only that the state's promise is kept, which is what makes the colour honest.
	float MaxBefore = 0.f;
	float MaxAfter = 0.f;
	TryReadFloat(Own.Get(), TEXT("MaxHP"), MaxBefore);
	const int32 CountBefore = Own->GetStackUpgradeCount();
	TestTrue(TEXT("(c) the capped click SUCCEEDS — it is not a refusal"), Own->ApplyStackUpgrade());
	TryReadFloat(Own.Get(), TEXT("MaxHP"), MaxAfter);

	TestTrue(TEXT("(c) ⭐ MaxHP STILL GREW past the cap — blue at the cap is not an empty promise"), MaxAfter > MaxBefore);
	TestEqual(TEXT("(c) ⭐ …while the HEIGHT did ⛔ not move at all (tolerance ZERO)"),
		ABuilding::StackHeightMultiplier(Own->GetStackUpgradeCount(), OwnHeightCap), CappedHeight, 0.f);
	TestTrue(TEXT("(c) …and the count still advanced, so the cap freezes the HEIGHT and ⛔ nothing else"),
		Own->GetStackUpgradeCount() > CountBefore);

	// (d) ⭐ THE CAP CONDITION THE CONFIRM USES, ASSERTED AS THE SHIPPED EXPRESSION. ConfirmStack-
	//     Upgrade decides whether to print the HUD note by comparing the series before and after
	//     — ⛔ never by comparing the count to 5. This row proves that expression is true here and
	//     ⛔ false below the cap, which is the whole content of "the note fires only when it bit".
	TestTrue(TEXT("(d) ⭐ the shipped cap test fires AT the cap"),
		ABuilding::StackHeightMultiplier(Own->GetStackUpgradeCount(), OwnHeightCap) <= ABuilding::StackHeightMultiplier(CountBefore, OwnHeightCap));
	TestFalse(TEXT("(d) ⭐⭐ …and it is FALSE at n = 0 → 1, so the note cannot fire on a first upgrade"),
		ABuilding::StackHeightMultiplier(1, OwnHeightCap) <= ABuilding::StackHeightMultiplier(0, OwnHeightCap));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  15. ⭐ BLUE IS A **THIRD COLOUR**, ⛔ NOT A SHADE OF A SHIPPED ONE — and the two shipped
//      ones are UNCHANGED (STACK-§0: the same MID, the same parameter, one more value)
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUpgradeGhostColourIsAThirdDistinctValueTest,
	"Siegebound.Placement.TheUpgradeGhostColourIsAThirdDistinctValueOnTheShippedGhostParameter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUpgradeGhostColourIsAThirdDistinctValueTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	FLinearColor Valid = FLinearColor::Transparent;
	FLinearColor Invalid = FLinearColor::Transparent;
	FLinearColor Upgrade = FLinearColor::Transparent;

	// ⛔ THE EXISTENCE CHECK GUARDS THE VALUE CHECKS rather than sitting beside them (TASK-735's
	// test 10 ruling): a failed read leaves Transparent in all three, and three Transparents
	// compare EQUAL — so an unguarded file would report "distinct" as a PASS for a deleted
	// property.
	if (!TryReadShippedColor(TEXT("ValidGhostColor"), Valid)
		|| !TryReadShippedColor(TEXT("InvalidGhostColor"), Invalid)
		|| !TryReadShippedColor(TEXT("UpgradeGhostColor"), Upgrade))
	{
		AddError(TEXT("⛔ One of ValidGhostColor / InvalidGhostColor / UpgradeGhostColor is not an FLinearColor UPROPERTY on ASiegePlayerController — the ghost has fewer than three states and the blue one cannot be shown."));
		return false;
	}

	// (a) ⭐⭐ THE CLAIM THAT MATTERS: the blue is not either shipped colour. A third STATE
	//     painted in a shipped colour is invisible to the player — the feature would ship and
	//     read as a bug ("it says I can place here and then it upgrades instead").
	TestFalse(TEXT("(a) ⭐⭐ UpgradeGhostColor is ⛔ NOT the valid (green) colour"), Upgrade.Equals(Valid, 1.e-4f));
	TestFalse(TEXT("(a) ⭐⭐ UpgradeGhostColor is ⛔ NOT the invalid (red) colour"), Upgrade.Equals(Invalid, 1.e-4f));

	// (b) ⭐ AND IT IS A DIFFERENT **HUE**, not a different brightness of red or green: its blue
	//     channel dominates both of its own other channels. A "dark green" third colour passes
	//     (a) and still fails the player at a glance.
	TestTrue(TEXT("(b) ⭐ the upgrade colour is genuinely BLUE-dominant (B > R and B > G) — a third HUE, ⛔ not a third shade"),
		Upgrade.B > Upgrade.R && Upgrade.B > Upgrade.G);

	// (c) ⛔ THE TWO SHIPPED COLOURS ARE UNCHANGED. This is the non-regression pin, in the same
	//     shape as TASK-735's tunable pin: the blue work must not quietly retint the green/red
	//     ghost that has been on screen since TASK-012.
	TestTrue(TEXT("(c) ⛔ ValidGhostColor is still pure green (0,1,0)"), Valid.Equals(FLinearColor(0.f, 1.f, 0.f), 1.e-4f));
	TestTrue(TEXT("(c) ⛔ InvalidGhostColor is still pure red (1,0,0)"), Invalid.Equals(FLinearColor(1.f, 0.f, 0.f), 1.e-4f));

	// (d) SELF-CHECK ON THE INSTRUMENT: a property that does not exist must read FALSE, or every
	//     claim above would be a statement about a reader that always succeeds.
	FLinearColor Nonexistent = FLinearColor::White;
	TestFalse(TEXT("(d) SELF-CHECK: the colour reader returns false for a property that does not exist"),
		TryReadShippedColor(TEXT("ThisGhostColorDoesNotExist"), Nonexistent));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  16. ⭐ THE TWO NEW LINES ARE **THEIR OWN**, ⛔ never a reused refusal — and the cap note
//      ⛔ never reads as one, nor names the excluded card
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUpgradeMessagesAreDistinctTest,
	"Siegebound.Placement.TheUpgradeRefusalAndCapNoticeAreTheirOwnLinesAndNeverNameTheExcludedCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUpgradeMessagesAreDistinctTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	const FString NotStackable = ASiegePlayerController::StackNotStackableRefusalText().ToString();
	const FString CapNotice = ASiegePlayerController::StackHeightCapNoticeText().ToString();
	const FString UnitFootprint = ASiegePlayerController::UnitFootprintRefusalText().ToString();

	// ⛔ The four shipped refusal strings the placement path already prints. Reusing any of them
	// for the upgrade case is the failure this test exists to catch (TASK-735's Clearance
	// precedent: "another building" would be a lie the player cannot act on — and it is exactly
	// the lie a player hovering a building on purpose would be told).
	const FString Shipped[] =
	{
		TEXT("Too steep"),
		TEXT("Too close to obstacles"),
		TEXT("Too close to another building"),
		TEXT("Invalid placement location"),
		TEXT("Not enough gold")
	};

	// (a) Both new lines exist and are non-empty — a blank FText would degrade the refusal into
	//     the silent nothing STACK-§2 forbids.
	TestTrue(TEXT("(a) the not-stackable refusal is non-empty"), !NotStackable.IsEmpty());
	TestTrue(TEXT("(a) the height-cap notice is non-empty"), !CapNotice.IsEmpty());

	// (b) ⭐ EVERY PAIR IS DISTINCT — the player can tell all seven apart.
	for (const FString& One : Shipped)
	{
		TestTrue(*FString::Printf(TEXT("(b) ⭐ the not-stackable refusal differs from the shipped '%s'"), *One), NotStackable != One);
		TestTrue(*FString::Printf(TEXT("(b) ⭐ the cap notice differs from the shipped '%s'"), *One), CapNotice != One);
	}
	TestTrue(TEXT("(b) ⭐ the not-stackable refusal differs from TASK-735's unit-footprint refusal"), NotStackable != UnitFootprint);
	TestTrue(TEXT("(b) ⭐⭐ the cap notice differs from the not-stackable refusal — a SUCCESS and a REFUSAL may never share a line"), CapNotice != NotStackable);

	// (c) ⛔⛔ THE REFUSAL ⛔ NEVER NAMES THE EXCLUDED CARD. The exclusion is structural, so the
	//     next climbable building must inherit this message unchanged — a "WatchTower" in the
	//     player-facing text would be the same name check STACK-§2 makes an automatic fail,
	//     just moved into a string the player reads.
	TestFalse(TEXT("(c) ⛔ the not-stackable refusal does ⛔ NOT name 'WatchTower' — the exclusion is structural and so is its wording"),
		NotStackable.Contains(TEXT("WatchTower"), ESearchCase::IgnoreCase));
	TestFalse(TEXT("(c) ⛔ nor 'Watch Tower'"), NotStackable.Contains(TEXT("Watch Tower"), ESearchCase::IgnoreCase));

	// (d) ⭐ THE CAP NOTICE ⛔ MUST NOT READ AS A REFUSAL. The click SUCCEEDED and bought health;
	//     a line containing "cannot" / "refused" / "not enough" would tell the player the
	//     opposite of what just happened.
	const TCHAR* const RefusalWords[] = { TEXT("cannot"), TEXT("can't"), TEXT("refus"), TEXT("not enough"), TEXT("invalid"), TEXT("too ") };
	for (const TCHAR* const Word : RefusalWords)
	{
		TestFalse(*FString::Printf(TEXT("(d) ⭐ the cap notice does ⛔ not read as a refusal — it contains no '%s'"), Word),
			CapNotice.Contains(Word, ESearchCase::IgnoreCase));
	}

	// (e) ⭐ AND IT SAYS WHAT THE CLICK ACTUALLY BOUGHT. At the cap the feedback may ⛔ not claim
	//     a height gain it will not deliver, so the word that must be present is HEALTH.
	TestTrue(TEXT("(e) ⭐ the cap notice names HEALTH — the thing the capped click actually bought"),
		CapNotice.Contains(TEXT("health"), ESearchCase::IgnoreCase));

	// (f) ⭐ THE REFUSAL NAMES THE THING UNDER THE CURSOR — and this doubles as the SELF-CHECK
	//     for (c): a `Contains` that finds a word which IS there proves the two absences above
	//     are findings rather than statements about a broken search.
	TestTrue(TEXT("(f) ⭐ the not-stackable refusal names the BUILDING — the thing the player must stop hovering (and proves Contains works, so (c)'s absences mean something)"),
		NotStackable.Contains(TEXT("building"), ESearchCase::IgnoreCase));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  17. ⛔⛔ ⛔ NO `CardID` STRING COMPARE EVER ENTERS THE PLACEMENT PATH (STACK-§2's AUTOMATIC
//      FAIL) — and the new reason value is APPENDED, ⛔ not inserted (TASK-735's request)
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementUpgradeNamesNoCardTest,
	"Siegebound.Placement.ThePlacementPathNamesNoCardAndTheNewReasonIsAppendedAfterUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementUpgradeNamesNoCardTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	FString HeaderText;
	FString SourceText;
	if (!LoadProjectSource(*this, ControllerHeaderPath, HeaderText) || !LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	// ── SELF-CHECK ON THE SCANNER, FIRST. ⚠️ Four "zero occurrences" claims follow, and every
	// one of them is also what a scanner that finds NOTHING would report. Proving it CAN find a
	// token that is genuinely on a code line is what makes the zeroes mean something.
	const int32 PredicateHits = CountOccurrencesInCode(SourceText, TEXT("CanScaleFootprint"));
	TestTrue(TEXT("SELF-CHECK: the scanner finds 'CanScaleFootprint' on real CODE lines of the controller — so it can find things, and the zeroes below are findings"),
		PredicateHits > 0);
	// ⭐⭐ AND THE SHARPER HALF OF THE SELF-CHECK: the token 'WatchTower' IS genuinely present in
	// both files — in PROSE, explaining why the exclusion is structural. ⇒ the zeroes below are
	// the comment-skipper doing its job, ⛔ not the token being absent from the file, which is
	// the one way this test could report SAFE while the name check had been added.
	TestTrue(TEXT("SELF-CHECK ⭐⭐: 'WatchTower' IS present in the header's PROSE — so a zero on code lines is a real finding, ⛔ not an absent token"),
		HeaderText.Contains(TEXT("WatchTower"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("SELF-CHECK ⭐⭐: 'WatchTower' IS present in the source's PROSE too"),
		SourceText.Contains(TEXT("WatchTower"), ESearchCase::CaseSensitive));

	// (a) ⛔⛔ THE AUTOMATIC FAIL. STACK-§2: "a CardID == 'WatchTower' string comparison anywhere
	//     in the placement path is an automatic QA fail." Both controller files, code lines only.
	TestEqual(TEXT("(a) ⛔⛔ SiegePlayerController.cpp names 'WatchTower' ZERO times on a code line — the exclusion is STRUCTURAL"),
		CountOccurrencesInCode(SourceText, TEXT("WatchTower")), 0);
	TestEqual(TEXT("(a) ⛔⛔ SiegePlayerController.h names 'WatchTower' ZERO times on a code line"),
		CountOccurrencesInCode(HeaderText, TEXT("WatchTower")), 0);
	TestEqual(TEXT("(a) ⛔ nor 'ClimbableTower' — the exclusion is asked through the base class's virtual, ⛔ never by class name"),
		CountOccurrencesInCode(SourceText, TEXT("ClimbableTower")), 0);

	// (b) ⭐ THE NEW REASON IS APPENDED **AFTER** `Units`, exactly as TASK-735's handoff §6.2
	//     asked — and TASK-815 appends after mine for the same reason: an insertion in the
	//     middle silently renumbers a private enum three tasks are editing in series.
	const int32 EnumStart = HeaderText.Find(TEXT("enum class EPlacementInvalidReason"), ESearchCase::CaseSensitive);
	if (EnumStart == INDEX_NONE)
	{
		AddError(TEXT("⛔ EPlacementInvalidReason was not found in SiegePlayerController.h — it was renamed or moved, and the ordering claim below cannot be made."));
		return false;
	}
	const int32 ClearanceAt = HeaderText.Find(TEXT("Clearance,"), ESearchCase::CaseSensitive, ESearchDir::FromStart, EnumStart);
	const int32 UnitsAt = HeaderText.Find(TEXT("Units,"), ESearchCase::CaseSensitive, ESearchDir::FromStart, EnumStart);
	const int32 UpgradeAt = HeaderText.Find(TEXT("Upgrade"), ESearchCase::CaseSensitive, ESearchDir::FromStart, UnitsAt == INDEX_NONE ? EnumStart : UnitsAt);

	TestTrue(TEXT("(b) the shipped 'Clearance' value is still in the enum"), ClearanceAt != INDEX_NONE);
	TestTrue(TEXT("(b) TASK-735's 'Units' value is still in the enum"), UnitsAt != INDEX_NONE);
	TestTrue(TEXT("(b) ⭐ TASK-813's 'Upgrade' value is in the enum"), UpgradeAt != INDEX_NONE);
	TestTrue(TEXT("(b) ⭐⭐ the order is Clearance → Units → Upgrade — ⛔ APPENDED, never inserted"),
		ClearanceAt != INDEX_NONE && UnitsAt != INDEX_NONE && UpgradeAt != INDEX_NONE
		&& ClearanceAt < UnitsAt && UnitsAt < UpgradeAt);

	// (c) ⭐ AND THE CONFIRM SWITCH HANDLES IT WITH ITS OWN CASE — ⛔ not by falling into
	//     `Clearance` and telling the player about "another building".
	TestTrue(TEXT("(c) ⭐ TryConfirmPlacement's switch has its own 'case EPlacementInvalidReason::Upgrade:'"),
		CountOccurrencesInCode(SourceText, TEXT("case EPlacementInvalidReason::Upgrade:")) == 1);

	return true;
}

// ███████████████████████████████████████████████████████████████████████████████████████████
//  TASK-819 — THE DISCARD-ALL MECHANIC (`CARDBAR-§6`, `CARDBAR-§7`). Tests 18–23.
//  ⛔ A LODGER IN THIS FILE, DECLARED AT THE TOP. Subject: ASiegePlayerController::
//  DiscardEntireHand + DiscardAllCost + the IA_DiscardAll key lane.
// ███████████████████████████████████████████████████████████████████████████████████████████

namespace SiegeDiscardAllFixture
{
	/**
	 *  ⛔⛔ THE FAILURE MODE THIS WHOLE BLOCK IS BUILT AROUND, STATED ONCE:
	 *
	 *      **A DISCARD-ALL TEST PASSES TRIVIALLY ON AN EMPTY HAND.**
	 *
	 *  "Nothing went wrong" is what a correct implementation and a completely broken one both
	 *  report when there was never a card to bin. ⇒ ⭐ every behavioural claim below is paired
	 *  with a **FULL-HAND CONTROL asserted before the act** (the hand really did hold six cards)
	 *  and a **GOLD-MOVED assertion after it** (the balance really did change, by a measured
	 *  amount) — and test 19 additionally builds the LOOP BUG on purpose and proves the gold
	 *  assertion can tell the two apart. A "charges per card" implementation fails test 19; a
	 *  "charges nothing" one fails it too; neither can pass this block.
	 *
	 *  ⭐ NOTHING HERE IS TRANSCRIBED. The fee is read off the controller CDO by reflection, so
	 *  🧑 retuning `DiscardAllCost` (his one word, `CARDBAR-§10`) leaves every claim meaningful.
	 *
	 *  ⛔ WHAT THIS BLOCK CANNOT PROVE (`SC-§32`, stated so nobody mistakes green for done):
	 *    • ⛔ **`DiscardEntireHand` IS NEVER CALLED HERE.** It needs a possessed controller with a
	 *      PlayerState and a live DeckComponent, and its success path reaches
	 *      `UGameplayStatics::PlaySound2D`, which resolves a world. ⇒ tests 19/20 drive the SAME
	 *      TWO SHIPPED APIS IT DRIVES (`ASiegePlayerState::SpendGold`, then
	 *      `UDeckComponent::DiscardFromHand` per occupied slot) on world-free scratch objects, and
	 *      **test 21 proves the shipped body really has that shape** by reading the source. The
	 *      two halves are only worth anything together, and that is why neither is omitted.
	 *    • ⛔ **NO KEY IS PRESSED.** That `H` reaches `OnDiscardAllPressed` at all closes on
	 *      TASK-820's `IMC_Hero` row and on Jonathan's keyboard — ⛔ never on this suite.
	 *    • ⛔ **NO REPLICATION.** `HasAuthority()` is true on a world-free actor by construction
	 *      (`AActor` sets `ROLE_Authority`); the observer-lockout branch is a DIFF READ.
	 */

	/** Hand cards that are ⛔ no shipped CardID, so nothing below can accidentally agree with cards.csv. */
	static const TCHAR* const HandCardPrefix = TEXT("SiegeboundTestHandCard");

	/** Replacements sitting in the draw pile — ⛔ distinct from the hand, so a "refill" that just left the old cards in place goes RED. */
	static const TCHAR* const DrawCardPrefix = TEXT("SiegeboundTestDrawCard");

	/** A world-free ASiegePlayerState. The `MakeScratchBuilding` precedent — ⛔ never the CDO (these MUTATE gold). */
	static TStrongObjectPtr<ASiegePlayerState> MakeScratchPlayerState()
	{
		return TStrongObjectPtr<ASiegePlayerState>(
			NewObject<ASiegePlayerState>(GetTransientPackageAsObject(), ASiegePlayerState::StaticClass(), NAME_None, RF_Transient));
	}

	/** A world-free UDeckComponent, unowned. Its pile logic is pure array work — ⛔ no world, no timer, no RNG beyond the shuffle it never runs here. */
	static TStrongObjectPtr<UDeckComponent> MakeScratchDeck()
	{
		return TStrongObjectPtr<UDeckComponent>(
			NewObject<UDeckComponent>(GetTransientPackageAsObject(), UDeckComponent::StaticClass(), NAME_None, RF_Transient));
	}

	/**
	 *  Reaches a private `TArray<FName>` UPROPERTY (the `SiegeAssistantSelectionTest`
	 *  `FindNameArrayField` idiom, adopted verbatim).
	 *
	 *  ⚠️ IT CHECKS THE INNER PROPERTY TYPE, NOT JUST THE NAME, AND THAT MATTERS HERE: a renamed
	 *  or retyped `Hand` would otherwise leave the fixture holding an EMPTY hand — which is
	 *  precisely the state every claim below passes trivially in.
	 */
	static TArray<FName>* FindNameArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = Object
			? FindFProperty<FArrayProperty>(Object->GetClass(), FieldName)
			: nullptr;
		if (!ArrayProperty || !ArrayProperty->Inner || !ArrayProperty->Inner->IsA<FNameProperty>())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<FName>>(Object);
	}

	/**
	 *  Deals `NumOccupied` distinct fixture cards into the first slots of a scratch deck and
	 *  stocks the draw pile with `NumSpare` DISTINCT replacements. Returns false (LOUDLY) if the
	 *  seed could not be planted — ⛔ a fixture that silently seeded nothing is the trivial pass.
	 */
	static bool SeedHand(FAutomationTestBase& Test, UDeckComponent* Deck, int32 NumOccupied, int32 NumSpare)
	{
		TArray<FName>* const Hand = FindNameArrayField(Deck, TEXT("Hand"));
		TArray<FName>* const DrawPile = FindNameArrayField(Deck, TEXT("DrawPile"));
		if (!Hand || !DrawPile)
		{
			Test.AddError(TEXT("SELF-CHECK FAILED: UDeckComponent's 'Hand' / 'DrawPile' are not reflected TArray<FName> fields — the fixture could seed nothing, so every claim resting on it would be measuring an EMPTY hand."));
			return false;
		}

		const int32 NumSlots = Deck->GetHandSize();
		Hand->Reset();
		for (int32 Slot = 0; Slot < NumSlots; ++Slot)
		{
			Hand->Add(Slot < NumOccupied ? FName(*FString::Printf(TEXT("%s%d"), HandCardPrefix, Slot)) : NAME_None);
		}

		DrawPile->Reset();
		for (int32 Index = 0; Index < NumSpare; ++Index)
		{
			DrawPile->Add(FName(*FString::Printf(TEXT("%s%d"), DrawCardPrefix, Index)));
		}
		return true;
	}

	/** Slots currently holding a card — the exact domain `DiscardEntireHand` collects before it charges. */
	static TArray<int32> OccupiedSlots(const UDeckComponent* Deck)
	{
		TArray<int32> Slots;
		for (int32 Slot = 0; Slot < Deck->GetHandSize(); ++Slot)
		{
			if (!Deck->GetHandCardID(Slot).IsNone())
			{
				Slots.Add(Slot);
			}
		}
		return Slots;
	}

	/** Reads a shipped `int32` UPROPERTY off the controller CDO. ⛔ A tunable that cannot be found FAILS rather than defaulting. */
	static bool TryReadShippedInt(FAutomationTestBase& Test, const TCHAR* PropertyName, int32& OutValue)
	{
		OutValue = 0;
		const ASiegePlayerController* const Defaults = GetDefault<ASiegePlayerController>();
		const FIntProperty* const Property = FindFProperty<FIntProperty>(ASiegePlayerController::StaticClass(), PropertyName);
		if (!Defaults || !Property)
		{
			Test.AddError(FString::Printf(TEXT("SELF-CHECK FAILED: '%s' is not a reflected int32 on ASiegePlayerController — it was renamed or its UPROPERTY was dropped, and the claims resting on it cannot be made."), PropertyName));
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Defaults);
		return true;
	}

	/**
	 *  The text of ONE `void ASiegePlayerController::…` definition, from its signature to the next
	 *  one. ⛔ A probe that cannot find its subject FAILS — a stale extractor returning an empty
	 *  string would report every "zero occurrences" claim below as SAFE.
	 */
	static bool ExtractControllerFunctionBody(FAutomationTestBase& Test, const FString& Source, const TCHAR* Signature, FString& OutBody)
	{
		OutBody.Reset();
		const int32 Start = Source.Find(Signature, ESearchCase::CaseSensitive);
		if (Start == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' was not found in SiegePlayerController.cpp — it was renamed or removed, and every shape claim about it below is unmakeable."), Signature));
			return false;
		}
		const int32 SearchFrom = Start + FCString::Strlen(Signature);
		const int32 NextDefinition = Source.Find(TEXT("\nvoid ASiegePlayerController::"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
		OutBody = Source.Mid(Start, (NextDefinition == INDEX_NONE ? Source.Len() : NextDefinition) - Start);
		return !OutBody.IsEmpty();
	}

	/**
	 *  The same source with every COMMENT line dropped — the identical predicate
	 *  `SiegePlacementUpgradeFixture::CountOccurrencesInCode` uses, so the ORDER claims below and
	 *  the COUNT claims beside them agree on one definition of "code".
	 *
	 *  ⚠️ WITHOUT THIS THE ORDER CLAIMS WOULD MEASURE PROSE: the shipped function explains its own
	 *  ladder in comments that name every symbol the order test looks for.
	 */
	static FString CodeLinesOnly(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		FString Code;
		Code.Reserve(Source.Len());
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();
			const bool bIsCommentLine =
				Trimmed.StartsWith(TEXT("//"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("* "), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("*/"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("/*"), ESearchCase::CaseSensitive)
				|| Trimmed.Equals(TEXT("*"), ESearchCase::CaseSensitive);
			if (!bIsCommentLine)
			{
				Code.Append(Line);
				Code.AppendChar(TEXT('\n'));
			}
		}
		return Code;
	}

	/** Ordinal position of a token on CODE lines, or INDEX_NONE. */
	static int32 CodeIndexOf(const FString& CodeOnly, const TCHAR* Token)
	{
		return CodeOnly.Find(Token, ESearchCase::CaseSensitive);
	}

	/** ⭐ Names the two operands in the failure message, so a red ordering line says WHICH pair inverted. */
	static void CheckPrecedes(FAutomationTestBase& Test, const FString& CodeOnly, const TCHAR* Earlier, const TCHAR* Later)
	{
		const int32 EarlierAt = CodeIndexOf(CodeOnly, Earlier);
		const int32 LaterAt = CodeIndexOf(CodeOnly, Later);
		Test.TestTrue(
			FString::Printf(TEXT("ORDER: '%s' (at %d) must appear BEFORE '%s' (at %d) on code lines"), Earlier, EarlierAt, Later, LaterAt),
			EarlierAt != INDEX_NONE && LaterAt != INDEX_NONE && EarlierAt < LaterAt);
	}

	static const TCHAR* const DiscardAllSignature = TEXT("void ASiegePlayerController::DiscardEntireHand()");

	/** The card bar — read ONLY (TASK-819's write fence excludes it); test 23(d)/(e) measure what is NOT in it. */
	static const TCHAR* const CardHandWidgetHeaderPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h");
	static const TCHAR* const CardHandWidgetSourcePath = TEXT("Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp");
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  18. ⭐ THE FEE IS A **NEW, SEPARATE** TUNABLE — ⛔ NOT `DiscardCost` WEARING A NEW NAME
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDiscardAllFeeIsItsOwnTunableTest,
	"Siegebound.Cards.TheDiscardAllFeeIsItsOwnTunableAndNotThePerCardOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDiscardAllFeeIsItsOwnTunableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDiscardAllFixture;

	int32 DiscardAllCost = 0;
	int32 PerCardCost = 0;
	if (!TryReadShippedInt(*this, TEXT("DiscardAllCost"), DiscardAllCost)
		|| !TryReadShippedInt(*this, TEXT("DiscardCost"), PerCardCost))
	{
		return false;
	}

	// (a) ⛔⛔ THE ALIASING BUG. `CARDBAR-§6`: "NEVER reuse DiscardCost" — the two mean different
	//     things (one card vs the whole hand), so a single shared number would silently retune
	//     both at once. This is the claim that goes red if somebody "tidies up" the duplicate.
	TestTrue(
		FString::Printf(TEXT("(a) ⛔⛔ DiscardAllCost (%d) and DiscardCost (%d) are DIFFERENT numbers — the discard-all fee is its own tunable, ⛔ not the per-card one reused"), DiscardAllCost, PerCardCost),
		DiscardAllCost != PerCardCost);

	// (b) ⭐ BOTH ARE POSITIVE, AND THAT IS A MECHANIC CLAIM RATHER THAN A TRANSCRIPTION: a zero
	//     discard-all fee makes the whole "refused below the fee" ladder inert (SpendGold(0)
	//     always succeeds), and a zero per-card fee would make test 19's discrimination blind.
	TestTrue(FString::Printf(TEXT("(b) DiscardAllCost is > 0 (%d) — a free discard-all would make its refusal branch unreachable"), DiscardAllCost), DiscardAllCost > 0);
	TestTrue(FString::Printf(TEXT("(b) DiscardCost is > 0 (%d) — required for test 19's loop-bug control to discriminate at all"), PerCardCost), PerCardCost > 0);

	// (c) ⭐ THE SOFT PATH POINTS AT THE ASSET TASK-820 AUTHORED. ⚠️ This is a CROSS-ARTIFACT
	//     naming contract (CONVENTIONS), ⛔ not a magic number: a typo here does not fail to
	//     compile, does not crash, and does not warn loudly — it leaves the key silently INERT
	//     forever, which is the one failure that looks exactly like "the feature was never built".
	const ASiegePlayerController* const Defaults = GetDefault<ASiegePlayerController>();
	const FSoftObjectProperty* const AssetProperty =
		FindFProperty<FSoftObjectProperty>(ASiegePlayerController::StaticClass(), TEXT("DiscardAllActionAsset"));
	if (!Defaults || !AssetProperty)
	{
		AddError(TEXT("SELF-CHECK FAILED: 'DiscardAllActionAsset' is not a reflected soft-object property on ASiegePlayerController — the key can never resolve and the contract claim is unmakeable."));
		return false;
	}
	// read through ContainerPtrToValuePtr — the same idiom TryReadShippedColor already uses on
	// this CDO, so the CDO read stays one shape across the file
	const FSoftObjectPtr* const AssetValue = AssetProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Defaults);
	const FString AssetPath = AssetValue ? AssetValue->ToString() : FString();
	TestTrue(
		FString::Printf(TEXT("(c) DiscardAllActionAsset points at TASK-820's IA_DiscardAll (read back: '%s')"), *AssetPath),
		AssetPath.Contains(TEXT("/Game/Input/Actions/IA_DiscardAll"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  19. ⛔⛔ THE FEE MOVES **ONCE** FOR A FULL HAND — the loop bug, measured, with the loop bug
//      ITSELF built beside it as the control that proves the measurement can tell them apart
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDiscardAllChargesTheFlatFeeExactlyOnceTest,
	"Siegebound.Cards.TheDiscardAllChargesItsFlatFeeExactlyOnceForAFullHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDiscardAllChargesTheFlatFeeExactlyOnceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDiscardAllFixture;

	int32 DiscardAllCost = 0;
	int32 PerCardCost = 0;
	if (!TryReadShippedInt(*this, TEXT("DiscardAllCost"), DiscardAllCost)
		|| !TryReadShippedInt(*this, TEXT("DiscardCost"), PerCardCost))
	{
		return false;
	}

	TStrongObjectPtr<ASiegePlayerState> State = MakeScratchPlayerState();
	TStrongObjectPtr<UDeckComponent> Deck = MakeScratchDeck();
	if (!State.IsValid() || !Deck.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: the scratch PlayerState / DeckComponent could not be constructed."));
		return false;
	}

	// ⚠️ THE PRECONDITION THE SHIPPED CODE DEMANDS, ASSERTED RATHER THAN ASSUMED (the
	// SiegeBuildingStackTest precedent): SpendGold refuses outright without authority, so if the
	// engine ever changed the default actor role this test must say WHY it went red instead of
	// reporting a phantom economy bug.
	TestTrue(TEXT("PRECONDITION: a world-free scratch PlayerState HAS authority (AActor sets ROLE_Authority) — SpendGold is reachable"),
		State->HasAuthority());

	// ⭐⭐ THE SPARE COUNT IS `NumSlots + 1` AND THAT IS ⛔ NOT AN ARBITRARY MARGIN — it reproduces
	// `CARDBAR-§6`'s measured fact at fixture scale. A legal 50-card deck leaves 44 in the draw
	// pile after the deal, so no eager reshuffle can fire mid-loop and none of the binned cards
	// can return. With exactly `NumSlots` spares the LAST draw would empty the pile, the §3.4
	// invariant would fold the discard pile straight back in, and both the "discard pile grew"
	// and "no binned card came back" claims below would measure the reshuffle instead of the
	// discard. ⚠️ That reshuffle is CORRECT shipped behaviour (the physical card rule) — the
	// fixture simply must not sit on top of it while measuring something else.
	const int32 NumSlots = Deck->GetHandSize();
	if (!SeedHand(*this, Deck.Get(), /*NumOccupied=*/ NumSlots, /*NumSpare=*/ NumSlots + 1))
	{
		return false;
	}

	// ⭐⭐ THE FULL-HAND CONTROL — ⛔ THE LINE THAT STOPS THIS WHOLE TEST FROM PASSING TRIVIALLY.
	// Everything below is about what happens when six cards are binned; if the hand were empty
	// the "no card left behind" claims would all hold vacuously.
	TArray<int32> Occupied = OccupiedSlots(Deck.Get());
	TestEqual(TEXT("CONTROL ⭐⭐: the hand really is FULL before anything is discarded — every slot holds a card"),
		Occupied.Num(), NumSlots);
	TestTrue(TEXT("CONTROL: the draw pile outlasts the loop — otherwise the §3.4 eager reshuffle fires and this test measures IT, not the discard"),
		Deck->GetDrawPileCount() > NumSlots);
	if (Occupied.Num() != NumSlots)
	{
		return false;
	}

	// remember exactly which cards were held, so "they are gone" is a measurement, not a hope
	TSet<FName> HeldBefore;
	for (const int32 Slot : Occupied)
	{
		HeldBefore.Add(Deck->GetHandCardID(Slot));
	}
	TestEqual(TEXT("CONTROL: the six held cards are DISTINCT — a fixture of duplicates could not detect a partial refill"),
		HeldBefore.Num(), NumSlots);

	// top the balance up well clear of the fee so this test measures the CHARGE, ⛔ not affordability
	State->AddGold(DiscardAllCost * 4);
	const int32 GoldBefore = State->GetGold();
	const int32 DiscardPileBefore = Deck->GetDiscardPileCount();
	TestTrue(TEXT("CONTROL: the balance comfortably covers the fee, so nothing below is an affordability refusal in disguise"),
		GoldBefore > DiscardAllCost);

	// ── THE SHIPPED SEQUENCE: ONE SpendGold, then DiscardFromHand per occupied slot.
	//    ⛔ Test 21 is what proves DiscardEntireHand really is shaped this way.
	TestTrue(TEXT("the single flat charge is accepted"), State->SpendGold(DiscardAllCost));
	int32 Binned = 0;
	for (const int32 Slot : Occupied)
	{
		if (Deck->DiscardFromHand(Slot))
		{
			++Binned;
		}
	}

	// (a) ⛔⛔ THE GOLD-MOVED ASSERTION. Exact, not "less than before".
	TestEqual(TEXT("(a) ⛔⛔ gold moved by EXACTLY the flat fee — ⛔ once for the hand, ⛔ not once per card"),
		GoldBefore - State->GetGold(), DiscardAllCost);

	// (b) every occupied slot really was binned...
	TestEqual(TEXT("(b) every occupied slot was binned"), Binned, NumSlots);
	TestEqual(TEXT("(b) the discard pile grew by exactly the number of cards binned"),
		Deck->GetDiscardPileCount() - DiscardPileBefore, NumSlots);

	// (c) ...and the hand came back FULL and DIFFERENT (§3.4's same-call redraw). ⭐ Asserting
	//     "not empty" alone would pass on an implementation that never moved a card at all.
	int32 SlotsRefilled = 0;
	int32 OldCardsStillHeld = 0;
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		const FName Now = Deck->GetHandCardID(Slot);
		if (!Now.IsNone())
		{
			++SlotsRefilled;
		}
		if (HeldBefore.Contains(Now))
		{
			++OldCardsStillHeld;
		}
	}
	TestEqual(TEXT("(c) every slot is non-empty afterwards — the replacement hand was drawn in the same call"),
		SlotsRefilled, NumSlots);
	TestEqual(TEXT("(c) ⭐ ⛔ NOT ONE of the binned cards is still in hand — the refill is real, ⛔ not a no-op that left the hand alone"),
		OldCardsStillHeld, 0);

	// (d) ⭐⭐ THE NEGATIVE CONTROL — ⛔ THE ONLY THING THAT PROVES CLAIM (a) DISCRIMINATES.
	//     A loop over DiscardHandSlot would charge the flat fee AND the per-card fee six times
	//     over (`CARDBAR-§6` calls it an automatic fail). Built here on an identical fixture: if
	//     the two totals came out equal, claim (a) would be green under the bug too.
	TStrongObjectPtr<ASiegePlayerState> BuggyState = MakeScratchPlayerState();
	if (!BuggyState.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: the loop-bug control PlayerState could not be constructed."));
		return false;
	}
	BuggyState->AddGold(DiscardAllCost * 4);
	const int32 BuggyGoldBefore = BuggyState->GetGold();
	BuggyState->SpendGold(DiscardAllCost);
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		BuggyState->SpendGold(PerCardCost); // ← what looping the per-slot entry point costs
	}
	const int32 BuggySpend = BuggyGoldBefore - BuggyState->GetGold();

	TestEqual(TEXT("(d) the loop-bug control charges the flat fee PLUS one per-card fee per card"),
		BuggySpend, DiscardAllCost + PerCardCost * NumSlots);
	TestTrue(
		FString::Printf(TEXT("(d) ⭐⭐ the correct charge (%d) and the loop-bug charge (%d) are DIFFERENT — so claim (a) can actually catch the bug"), DiscardAllCost, BuggySpend),
		BuggySpend != DiscardAllCost);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  20. ⭐ THE FEE IS **FLAT** — one card costs what six cost, and an EMPTY hand costs NOTHING
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDiscardAllFeeIsFlatAndAnEmptyHandIsFreeTest,
	"Siegebound.Cards.TheDiscardAllFeeIsFlatAcrossHandSizesAndAnEmptyHandSpendsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDiscardAllFeeIsFlatAndAnEmptyHandIsFreeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDiscardAllFixture;

	int32 DiscardAllCost = 0;
	int32 PerCardCost = 0;
	if (!TryReadShippedInt(*this, TEXT("DiscardAllCost"), DiscardAllCost)
		|| !TryReadShippedInt(*this, TEXT("DiscardCost"), PerCardCost))
	{
		return false;
	}

	// ── (a) ONE CARD vs SIX.
	//    ⚠️⚠️ HONEST ABOUT WHAT THIS HALF CAN AND CANNOT PROVE, because the naive version of it
	//    is a TAUTOLOGY: both passes below call `SpendGold(DiscardAllCost)` once, so "the totals
	//    are equal" would be true no matter what the shipped code does. ⇒ ⭐ **the load-bearing
	//    assertion here is that the two passes did DIFFERENT WORK** — a different number of cards
	//    actually left the hand — while the charge stayed put. Without that, an equal total would
	//    only mean the fixture seeded the same hand twice.
	//    ⛔ THE CLAIM ABOUT THE SHIPPED CODE ITSELF IS TEST 21's: exactly one `SpendGold` call,
	//    `DiscardCost` nowhere on a code line, and nothing multiplied by the card count.
	int32 SpendForOne = 0;
	int32 SpendForFull = 0;
	int32 BinnedForOne = 0;
	int32 BinnedForFull = 0;
	int32 FullHandSize = 0;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		TStrongObjectPtr<ASiegePlayerState> State = MakeScratchPlayerState();
		TStrongObjectPtr<UDeckComponent> Deck = MakeScratchDeck();
		if (!State.IsValid() || !Deck.IsValid())
		{
			AddError(TEXT("SELF-CHECK FAILED: a scratch PlayerState / DeckComponent could not be constructed."));
			return false;
		}

		const int32 NumSlots = Deck->GetHandSize();
		const int32 NumOccupied = (Pass == 0) ? 1 : NumSlots;
		FullHandSize = NumSlots;
		if (!SeedHand(*this, Deck.Get(), NumOccupied, /*NumSpare=*/ NumSlots + 1))
		{
			return false;
		}

		const TArray<int32> Occupied = OccupiedSlots(Deck.Get());
		TestEqual(FString::Printf(TEXT("CONTROL: pass %d seeded exactly %d occupied slot(s)"), Pass, NumOccupied),
			Occupied.Num(), NumOccupied);

		State->AddGold(DiscardAllCost * 4);
		const int32 GoldBefore = State->GetGold();
		State->SpendGold(DiscardAllCost);

		int32 Binned = 0;
		for (const int32 Slot : Occupied)
		{
			if (Deck->DiscardFromHand(Slot))
			{
				++Binned;
			}
		}

		const int32 Spent = GoldBefore - State->GetGold();
		if (Pass == 0)
		{
			SpendForOne = Spent;
			BinnedForOne = Binned;
		}
		else
		{
			SpendForFull = Spent;
			BinnedForFull = Binned;
		}
	}

	// ⭐⭐ THE DISCRIMINATOR FIRST: the two passes really did different amounts of work. ⛔ If this
	//    is red, the equal-charge line below means nothing at all.
	TestEqual(TEXT("(a) ⭐⭐ pass 0 binned exactly ONE card"), BinnedForOne, 1);
	TestEqual(TEXT("(a) ⭐⭐ pass 1 binned a FULL hand"), BinnedForFull, FullHandSize);
	TestTrue(FString::Printf(TEXT("(a) ⭐⭐ the two passes moved DIFFERENT numbers of cards (%d vs %d) — so 'the same charge' is a finding about the FEE, ⛔ not about the fixture"), BinnedForOne, BinnedForFull),
		BinnedForOne != BinnedForFull && FullHandSize > 1);

	TestEqual(TEXT("(a) ⭐ …and a hand holding ONE card is charged exactly what a FULL hand is charged — the fee is FLAT (`CARDBAR-§10`, deliberate)"),
		SpendForOne, SpendForFull);
	// ⭐ and what the alternative would have looked like, stated as a number rather than a worry:
	//   under the retired per-card fee the two would have differed by (FullHandSize - 1) cards.
	TestTrue(FString::Printf(TEXT("(a) ⭐ under the retired per-card fee these two would have differed by %d gold — the flat fee is a real design change (`CARDBAR-§10`)"), PerCardCost * (FullHandSize - 1)),
		PerCardCost * (FullHandSize - 1) > 0);

	// ── (b) ⛔ THE EMPTY HAND SPENDS NOTHING, AND THE LADDER REFUSES **BEFORE** THE CHARGE.
	//    `ASiegePlayerState` has NO refund API, so a spend on a no-op is unrecoverable
	//    (qa/TASK-022-report.md WARN-1, generalised from one slot to the whole hand).
	TStrongObjectPtr<ASiegePlayerState> EmptyHandState = MakeScratchPlayerState();
	TStrongObjectPtr<UDeckComponent> EmptyDeck = MakeScratchDeck();
	if (!EmptyHandState.IsValid() || !EmptyDeck.IsValid() || !SeedHand(*this, EmptyDeck.Get(), /*NumOccupied=*/ 0, /*NumSpare=*/ 0))
	{
		return false;
	}
	TestEqual(TEXT("CONTROL: the empty-hand fixture really has zero occupied slots"),
		OccupiedSlots(EmptyDeck.Get()).Num(), 0);

	EmptyHandState->AddGold(DiscardAllCost * 4);
	const int32 EmptyGoldBefore = EmptyHandState->GetGold();

	// the shipped ladder's own condition, spelled out: with no occupied slot it RETURNS and
	// SpendGold is never reached. (⛔ That the shipped body really is ordered this way is test
	// 21's ORDER claim — `DiscardAllRefused_EmptyHand` before `SiegeState->SpendGold(`.)
	const TArray<int32> EmptyOccupied = OccupiedSlots(EmptyDeck.Get());
	if (EmptyOccupied.Num() > 0)
	{
		EmptyHandState->SpendGold(DiscardAllCost);
	}
	TestEqual(TEXT("(b) ⛔ an empty hand moves ZERO gold — the refusal precedes the charge, and there is no refund API to undo one"),
		EmptyHandState->GetGold(), EmptyGoldBefore);

	// ⭐⭐ THE CONTROL THAT STOPS (b) BEING VACUOUS — ⛔ without it, a frozen or zero balance would
	//    pass the line above perfectly. The SAME call on the SAME object, with the guard bypassed,
	//    DOES take the fee ⇒ "nothing moved" is the guard working, ⛔ not the fixture being inert.
	TestTrue(TEXT("(b) CONTROL ⭐⭐: bypassing the empty-hand guard DOES take the fee — so the zero above is the guard, ⛔ not a frozen balance"),
		EmptyHandState->SpendGold(DiscardAllCost));
	TestEqual(TEXT("(b) CONTROL ⭐⭐: …and it took exactly the fee's worth"),
		EmptyGoldBefore - EmptyHandState->GetGold(), DiscardAllCost);

	// ── (c) ⛔ AN UNAFFORDABLE DISCARD-ALL IS NET-ZERO: no gold, no card. `SpendGold` refuses
	//    below the fee without changing or broadcasting anything, so the loop is never entered.
	TStrongObjectPtr<ASiegePlayerState> BrokeState = MakeScratchPlayerState();
	TStrongObjectPtr<UDeckComponent> BrokeDeck = MakeScratchDeck();
	if (!BrokeState.IsValid() || !BrokeDeck.IsValid())
	{
		return false;
	}
	const int32 BrokeSlots = BrokeDeck->GetHandSize();
	if (!SeedHand(*this, BrokeDeck.Get(), /*NumOccupied=*/ BrokeSlots, /*NumSpare=*/ BrokeSlots + 1))
	{
		return false;
	}
	// ⭐ THE FULL-HAND CONTROL AGAIN — a refusal test on an empty hand proves nothing at all.
	TestEqual(TEXT("CONTROL ⭐: the unaffordable case is exercised with a FULL hand, so 'no card moved' is a finding"),
		OccupiedSlots(BrokeDeck.Get()).Num(), BrokeSlots);

	// drain to one gold short of the fee (Gold starts at the shipped StartingGold, so spend down)
	const int32 TargetGold = DiscardAllCost - 1;
	if (BrokeState->GetGold() > TargetGold)
	{
		BrokeState->SpendGold(BrokeState->GetGold() - TargetGold);
	}
	else
	{
		BrokeState->AddGold(TargetGold - BrokeState->GetGold());
	}
	TestEqual(TEXT("CONTROL: the balance is exactly one gold short of the fee — the boundary, ⛔ not a comfortable margin"),
		BrokeState->GetGold(), TargetGold);

	const int32 BrokeGoldBefore = BrokeState->GetGold();
	const bool bCharged = BrokeState->SpendGold(DiscardAllCost);
	TestFalse(TEXT("(c) ⛔ the charge is REFUSED one gold short of the fee"), bCharged);
	TestEqual(TEXT("(c) ⛔ and NO gold moved — the refusal is net-zero"), BrokeState->GetGold(), BrokeGoldBefore);
	TestEqual(TEXT("(c) ⛔ and NO card moved — the hand is untouched"),
		OccupiedSlots(BrokeDeck.Get()).Num(), BrokeSlots);

	// ⭐ and the boundary is exact: ONE more gold and the same call succeeds. Without this, a
	//    "SpendGold always refuses" implementation would pass (c) perfectly.
	BrokeState->AddGold(1);
	TestTrue(TEXT("(c) ⭐ BOUNDARY: at exactly the fee the same charge SUCCEEDS — so the refusal above is about affordability, not about refusing everything"),
		BrokeState->SpendGold(DiscardAllCost));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  21. ⛔⛔ THE SHIPPED BODY'S SHAPE — ONE CHARGE, THE RIGHT FEE, THE RIGHT LOOP, IN ORDER.
//      ⭐ This is the test that reads DiscardEntireHand itself. `CARDBAR-§6`'s automatic fail.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDiscardAllBodyChargesOnceAndLoopsTheDeckTest,
	"Siegebound.Cards.TheDiscardEntireHandBodyChargesOnceAndLoopsTheDeckNotThePerSlotEntryPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDiscardAllBodyChargesOnceAndLoopsTheDeckTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDiscardAllFixture;
	using namespace SiegePlacementUpgradeFixture;

	FString SourceText;
	if (!LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	FString Body;
	if (!ExtractControllerFunctionBody(*this, SourceText, DiscardAllSignature, Body))
	{
		return false;
	}
	const FString BodyCode = CodeLinesOnly(Body);

	// ── SELF-CHECK ON THE SCANNER, FIRST. Five "exactly N" / "zero" claims follow, and a scanner
	//    that found NOTHING would report the same numbers. Proving it finds a token that is
	//    genuinely on a code line of THIS body is what makes the rest mean anything.
	TestTrue(TEXT("SELF-CHECK: the extracted body is substantial (the extractor found a real function, not an empty string)"),
		Body.Len() > 500);
	TestTrue(TEXT("SELF-CHECK: the scanner finds 'DiscardAllCost' on real CODE lines of the body — so it can find things"),
		CountOccurrencesInCode(Body, TEXT("DiscardAllCost")) > 0);

	// ⭐⭐ THE SHARPER HALF (the test-17 device): 'DiscardCost' IS genuinely present in this
	// body — in PROSE, warning against exactly the bug below. ⇒ the zero on code lines is the
	// comment-skipper working, ⛔ not the token being absent, which is the one way this test
	// could report SAFE while the wrong fee had been wired in.
	TestTrue(TEXT("SELF-CHECK ⭐⭐: 'DiscardCost' IS present in the body's PROSE — so a zero on code lines is a real finding"),
		Body.Contains(TEXT("DiscardCost"), ESearchCase::CaseSensitive));

	// (a) ⛔⛔ THE FEE IS CHARGED **ONCE**. Asked with the receiver named, so a format string
	//     mentioning "SpendGold" in a log line cannot be mistaken for a second charge.
	TestEqual(TEXT("(a) ⛔⛔ exactly ONE 'SiegeState->SpendGold(' in the whole body — the flat fee is charged once for the hand"),
		CountOccurrencesInCode(Body, TEXT("SiegeState->SpendGold(")), 1);

	// (b) ⛔⛔ THE AUTOMATIC FAIL. Looping the per-slot entry point would re-run the entire guard
	//     ladder six times AND charge six extra per-card fees on top of the flat one.
	TestEqual(TEXT("(b) ⛔⛔ 'DiscardHandSlot(' is called ZERO times — the loop is over the DECK, ⛔ never over the per-slot entry point"),
		CountOccurrencesInCode(Body, TEXT("DiscardHandSlot(")), 0);
	// ⚠️ ASKED DIRECTLY, ⛔ NOT BY SUBTRACTION: `DiscardAllCost` does NOT contain the substring
	//    `DiscardCost` ("Discard" + "All" + "Cost" — the 'C' never follows the 'd'), so the two
	//    tokens do not overlap and this count is exactly the per-card fee's own occurrences.
	TestEqual(TEXT("(b) ⛔ and the per-card fee 'DiscardCost' appears ZERO times on a code line — the wrong fee is not wired in"),
		CountOccurrencesInCode(Body, TEXT("DiscardCost")), 0);

	// (c) ⭐ THE LOOP IS THE DECK'S PILE MOVEMENT, AND THERE IS EXACTLY ONE OF IT.
	TestEqual(TEXT("(c) exactly ONE 'DeckComponent->DiscardFromHand(' — one loop, ⛔ not a second discard path"),
		CountOccurrencesInCode(Body, TEXT("DeckComponent->DiscardFromHand(")), 1);

	// (d) ⛔⛔ THE LADDER'S ORDER, ON CODE LINES. The empty-hand refusal BEFORE the charge is the
	//     WARN-1 rule; the charge BEFORE the loop is what makes an unaffordable discard net-zero.
	CheckPrecedes(*this, BodyCode, TEXT("!HasAuthority()"), TEXT("bMatchEnded"));
	CheckPrecedes(*this, BodyCode, TEXT("bMatchEnded"), TEXT("bInPlacementMode"));
	CheckPrecedes(*this, BodyCode, TEXT("bInPlacementMode"), TEXT("bInTargetingMode"));
	CheckPrecedes(*this, BodyCode, TEXT("bInTargetingMode"), TEXT("!DeckComponent"));
	CheckPrecedes(*this, BodyCode, TEXT("!DeckComponent"), TEXT("DiscardAllRefused_EmptyHand"));
	CheckPrecedes(*this, BodyCode, TEXT("DiscardAllRefused_EmptyHand"), TEXT("SiegeState->SpendGold("));
	CheckPrecedes(*this, BodyCode, TEXT("SiegeState->SpendGold("), TEXT("DeckComponent->DiscardFromHand("));

	// (e) ⭐ ONE SOUND FOR THE GESTURE, ⛔ not one per card — and it sits AFTER the loop.
	TestEqual(TEXT("(e) exactly ONE discard sound call in the body"),
		CountOccurrencesInCode(Body, TEXT("PlaySound2D(this, CardDiscardSoundPath)")), 1);
	CheckPrecedes(*this, BodyCode, TEXT("DeckComponent->DiscardFromHand("), TEXT("PlaySound2D(this, CardDiscardSoundPath)"));

	// (f) ⭐ THE REFUSAL STRINGS ARE THE SHIPPED ONES, ⛔ REUSED AND NOT REWORDED. A second,
	//     differently-worded copy of a shipped refusal is a UI regression wearing a feature's
	//     clothes — the player would see two different sentences for the same rule.
	TestTrue(TEXT("(f) the placement refusal reuses the shipped 'DiscardRefused_Placing' key"),
		CountOccurrencesInCode(Body, TEXT("DiscardRefused_Placing")) == 1);
	TestTrue(TEXT("(f) the targeting refusal reuses the shipped 'DiscardRefused_Targeting' key"),
		CountOccurrencesInCode(Body, TEXT("DiscardRefused_Targeting")) == 1);
	TestTrue(TEXT("(f) the affordability refusal reuses the shipped 'CardRefused_CantAfford' key"),
		CountOccurrencesInCode(Body, TEXT("CardRefused_CantAfford")) == 1);
	// ⭐ and the SHIPPED per-slot function still carries the same two keys — so "reused" is
	//   measured against the original rather than asserted about a copy.
	FString PerSlotBody;
	if (ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::DiscardHandSlot(int32 Slot)"), PerSlotBody))
	{
		TestTrue(TEXT("(f) ⭐ the shipped DiscardHandSlot still uses 'DiscardRefused_Placing' — the string was REUSED, ⛔ not moved"),
			CountOccurrencesInCode(PerSlotBody, TEXT("DiscardRefused_Placing")) == 1);
		TestTrue(TEXT("(f) ⭐ …and 'DiscardRefused_Targeting'"),
			CountOccurrencesInCode(PerSlotBody, TEXT("DiscardRefused_Targeting")) == 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  22. ⛔⛔ THE INVERSE TRAP (`CARDBAR-§7`) — THE KEY IS A **MAPPED ACTION**, ⛔ NEVER A RAW
//      POLL AND ⛔ NEVER RE-TRANSLATED. ⚠️ THIS DEFECT IS INVISIBLE ON EVERY QWERTY MACHINE.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDiscardAllKeyIsMappedNeverPolledTest,
	"Siegebound.Cards.TheDiscardAllKeyIsAMappedActionAndIsNeverRawPolledOrRetranslated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDiscardAllKeyIsMappedNeverPolledTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;

	FString HeaderText;
	FString SourceText;
	if (!LoadProjectSource(*this, ControllerHeaderPath, HeaderText) || !LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	// ── SELF-CHECK. Four "zero occurrences" claims follow. ⭐ THE SHARPER HALF: BOTH forbidden
	//    tokens ARE genuinely present in these files' PROSE, warning against themselves — so a
	//    zero on code lines is the comment-skipper doing its job, ⛔ not an absent token.
	TestTrue(TEXT("SELF-CHECK: the scanner finds 'EKeys::' on real CODE lines of the controller (the shipped RMB/Esc polls) — so it can find raw key literals"),
		CountOccurrencesInCode(SourceText, TEXT("EKeys::")) > 0);
	TestTrue(TEXT("SELF-CHECK ⭐⭐: 'EKeys::H' IS present in the source's PROSE — so the zero below is a finding"),
		SourceText.Contains(TEXT("EKeys::H"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("SELF-CHECK ⭐⭐: 'GetPositionalKey' IS present in the source's PROSE too"),
		SourceText.Contains(TEXT("GetPositionalKey"), ESearchCase::CaseSensitive));

	// (a) ⛔⛔ NO RAW `EKeys::H` POLL ANYWHERE. Jonathan plays US-Dvorak and his ask is a KEY
	//     POSITION ("on Dvorak it would be 'D'"). A raw poll would fire on his physical `J`.
	TestEqual(TEXT("(a) ⛔⛔ SiegePlayerController.cpp names 'EKeys::H' ZERO times on a code line — ⛔ not as a fallback, ⛔ not as a double cover"),
		CountOccurrencesInCode(SourceText, TEXT("EKeys::H")), 0);
	TestEqual(TEXT("(a) ⛔⛔ SiegePlayerController.h names 'EKeys::H' ZERO times on a code line"),
		CountOccurrencesInCode(HeaderText, TEXT("EKeys::H")), 0);

	// (b) ⛔⛔ AND NO SECOND TRANSLATION. The subsystem already retargets IMC_Hero's keys
	//     wholesale, so calling GetPositionalKey on a MAPPED action double-applies the remap —
	//     `HELP-§1`'s named two-hop defect, which reads perfectly on a QWERTY host.
	TestEqual(TEXT("(b) ⛔⛔ 'GetPositionalKey' is called ZERO times on a code line of the controller source"),
		CountOccurrencesInCode(SourceText, TEXT("GetPositionalKey")), 0);
	TestEqual(TEXT("(b) ⛔⛔ …and ZERO times in the header"),
		CountOccurrencesInCode(HeaderText, TEXT("GetPositionalKey")), 0);

	// (c) ⭐ AND THE POSITIVE HALF — ⛔ WITHOUT IT, DELETING THE FEATURE ENTIRELY WOULD PASS (a)
	//     AND (b) PERFECTLY. The action is resolved null-safe and bound as an Enhanced Input
	//     action on the same trigger event every other command key uses.
	TestEqual(TEXT("(c) ⭐ IA_DiscardAll is soft-resolved exactly once through the shipped null-safe helper"),
		CountOccurrencesInCode(SourceText, TEXT("ResolveInputAction(DiscardAllAction, DiscardAllActionAsset")), 1);
	TestEqual(TEXT("(c) ⭐ …and BOUND exactly once, as a MAPPED action on ETriggerEvent::Started"),
		CountOccurrencesInCode(SourceText, TEXT("BindAction(DiscardAllAction, ETriggerEvent::Started")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  23. ⭐⭐ **EXACTLY ONE ROUTE** — the `H` key, and ⛔ NOTHING ELSE. Jonathan scrapped the
//      right-click alternative mid-implementation; this is the test that keeps it scrapped.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDiscardAllHasExactlyOneRouteTest,
	"Siegebound.Cards.TheDiscardAllHasExactlyOneRouteAndNoDeadPointerSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDiscardAllHasExactlyOneRouteTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDiscardAllFixture;
	using namespace SiegePlacementUpgradeFixture;

	FString HeaderText;
	FString SourceText;
	if (!LoadProjectSource(*this, ControllerHeaderPath, HeaderText) || !LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	// SELF-CHECK: the scanner can find the symbol at all, so the counts below are measurements.
	TestTrue(TEXT("SELF-CHECK: 'DiscardEntireHand' is present on code lines of the controller source"),
		CountOccurrencesInCode(SourceText, TEXT("DiscardEntireHand")) > 0);

	// (a) ⭐⭐ ONE DEFINITION + EXACTLY ONE CALL SITE.
	//     ⚠️ THE TOKENS ARE CHOSEN SO LOG TEXT CANNOT BE COUNTED AS CODE: eight `UE_LOG` format
	//     strings in the function name it as "DiscardEntireHand() refused/ignored/…", and those
	//     sit on CODE lines. A bare "DiscardEntireHand(" would count all ten. ⇒ the CALL is
	//     counted with its trailing semicolon (a definition is followed by a brace, a log string
	//     by a space) and the DEFINITION by its qualified signature.
	TestEqual(TEXT("(a) ⭐ the definition exists exactly once"),
		CountOccurrencesInCode(SourceText, TEXT("void ASiegePlayerController::DiscardEntireHand()")), 1);
	TestEqual(TEXT("(a) ⭐⭐ it is CALLED from EXACTLY ONE place in the controller — ⛔ a second call site means a second route was added, and after his 2026-09-03 ruling that needs HIS word, ⛔ not a reviewer's"),
		CountOccurrencesInCode(SourceText, TEXT("DiscardEntireHand();")), 1);
	TestEqual(TEXT("(a) ⭐ and it is declared exactly once in the header"),
		CountOccurrencesInCode(HeaderText, TEXT("void DiscardEntireHand();")), 1);

	// (b) ⭐ THAT ONE CALLER IS THE KEY HANDLER, AND IT CARRIES NOTHING ELSE. `CARDBAR-§6`: a
	//     second copy of the guard ladder is an automatic fail, so the handler forwards and stops.
	FString HandlerBody;
	if (!ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::OnDiscardAllPressed()"), HandlerBody))
	{
		return false;
	}
	TestEqual(TEXT("(b) ⭐ OnDiscardAllPressed forwards to DiscardEntireHand exactly once"),
		CountOccurrencesInCode(HandlerBody, TEXT("DiscardEntireHand(")), 1);
	// ⛔ ZERO guards of its own — a duplicated ladder would show up as any of these.
	TestEqual(TEXT("(b) ⛔ …and duplicates NO guard: no HasAuthority"), CountOccurrencesInCode(HandlerBody, TEXT("HasAuthority")), 0);
	TestEqual(TEXT("(b) ⛔ …no bInPlacementMode"), CountOccurrencesInCode(HandlerBody, TEXT("bInPlacementMode")), 0);
	TestEqual(TEXT("(b) ⛔ …no SpendGold"), CountOccurrencesInCode(HandlerBody, TEXT("SpendGold")), 0);
	TestEqual(TEXT("(b) ⛔ …and no DiscardFromHand"), CountOccurrencesInCode(HandlerBody, TEXT("DiscardFromHand")), 0);

	// (c) ⛔ THE RETIRED SYMBOLS SURVIVE. `CARDBAR-§6`: retired, ⛔ NOT deleted — deleting a
	//     BlueprintCallable a WBP may still reference is this project's silent-runtime-break
	//     class, and Tests/SiegeControlsHelpTest.cpp still requires the `Cards.Discard` row that
	//     speaks this vocabulary until TASK-821 rewrites it.
	TestTrue(TEXT("(c) ⛔ DiscardHandSlot SURVIVES (retired, ⛔ not deleted)"),
		CountOccurrencesInCode(HeaderText, TEXT("void DiscardHandSlot(int32 Slot)")) == 1
		&& CountOccurrencesInCode(SourceText, TEXT("void ASiegePlayerController::DiscardHandSlot(int32 Slot)")) == 1);
	TestTrue(TEXT("(c) ⛔ DiscardCost SURVIVES as its own tunable (retired, ⛔ not deleted)"),
		CountOccurrencesInCode(HeaderText, TEXT("int32 DiscardCost = ")) == 1);

	// (d) ⭐⭐ THE CARD BAR CARRIES **NO** POINTER SURFACE FOR THIS, AND THAT IS THE POINT OF
	//     JONATHAN'S 2026-09-03 CUT, ⛔ not an omission. His words: *"having that feature will
	//     maybe interfere with a player trying to right click to stop a new spawn"* — right-click
	//     is already the placement-cancel gesture, and a 20-gold accident on a cancel is the
	//     collision worth avoiding. ⚠️ A pointer handler on the BAR would also have fired for
	//     anything that bubbled to it, including the next-card preview — 20 gold for right-
	//     clicking a card that is not even in your hand.
	//     ⚠️ IF THIS EVER GOES RED IT IS NOT NECESSARILY A BUG — it means somebody re-added the
	//     route, and the correct response is to check that HE asked for it and then update THIS
	//     TEST deliberately. ⛔ It is not a line to quietly delete.
	FString WidgetHeaderText;
	FString WidgetSourceText;
	if (!LoadProjectSource(*this, CardHandWidgetHeaderPath, WidgetHeaderText)
		|| !LoadProjectSource(*this, CardHandWidgetSourcePath, WidgetSourceText))
	{
		return false;
	}

	// SELF-CHECK: the probe is reading the right file (a widget with no pass-throughs at all
	// would report every zero below as safe).
	TestTrue(TEXT("SELF-CHECK: the card-bar widget really is the file being read (its shipped RequestPlaySlot pass-through is there)"),
		CountOccurrencesInCode(WidgetSourceText, TEXT("void UCardHandWidget::RequestPlaySlot(int32 SlotIndex)")) == 1);

	TestEqual(TEXT("(d) ⭐⭐ the card bar declares NO mouse handler — the right-click route was scrapped and left nothing behind"),
		CountOccurrencesInCode(WidgetHeaderText, TEXT("NativeOnMouseButtonDown")), 0);
	TestEqual(TEXT("(d) ⭐⭐ …and implements none"),
		CountOccurrencesInCode(WidgetSourceText, TEXT("NativeOnMouseButtonDown")), 0);
	TestEqual(TEXT("(d) ⛔ no dead 'RequestDiscardAll' entry point was left in the header 'in case'"),
		CountOccurrencesInCode(WidgetHeaderText, TEXT("RequestDiscardAll")), 0);
	TestEqual(TEXT("(d) ⛔ nor in the source"),
		CountOccurrencesInCode(WidgetSourceText, TEXT("RequestDiscardAll")), 0);
	TestEqual(TEXT("(d) ⛔ and the widget never reaches DiscardEntireHand by any name"),
		CountOccurrencesInCode(WidgetSourceText, TEXT("DiscardEntireHand")), 0);

	// (e) ⛔ THE THIRD RETIRED SYMBOL SURVIVES. `CARDBAR-§6` retires `RequestDiscardSlot`
	//     alongside `DiscardHandSlot` and `DiscardCost` — TASK-809 removes the six buttons that
	//     bound it, so no shipped UI route reaches it. ⭐ It is asserted HERE because TASK-819's
	//     write fence excludes `CardHandWidget.{h,cpp}`: the survival can be MEASURED from inside
	//     the fence even though the one-line retirement comment must be written by whoever next
	//     edits that file. ⛔ Deleting a BlueprintCallable a WBP may still reference is this
	//     project's silent-runtime-break class.
	TestEqual(TEXT("(e) ⛔ UCardHandWidget::RequestDiscardSlot SURVIVES (retired, ⛔ not deleted)"),
		CountOccurrencesInCode(WidgetSourceText, TEXT("void UCardHandWidget::RequestDiscardSlot(int32 SlotIndex)")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  TASK-815 — THE PLACEMENT FOOTPRINT WHEEL (tests 24–28). `STACK-§4` · `STACK-§6` ·
//  `MARK-§4` (as amended: the wheel's THIRD and FINAL named consumer) · `HIGH-§1`.
//
//  ⭐ IN THE PLACEMENT FRAME BY RIGHT, ⛔ not as a lodger: the wheel sizes the placement
//  ghost and the building the confirm spawns, which is this file's declared subject.
//
//  ⛔⛔ WHAT WOULD MAKE THESE TESTS WORTHLESS, NAMED FIRST SO NOBODY HAS TO GO LOOKING:
//  "the wheel clamped" is also what a wheel that never moves reports, and "the excluded
//  class is inert" is also what a predicate that answers false for EVERYTHING reports.
//  ⇒ ⭐ every inert/clamped claim below is paired with the SAME fixture moving when the one
//  term under test is restored, and every "zero occurrences" source claim is paired with a
//  SELF-CHECK proving the scanner can find that same token somewhere it really is.
// ═══════════════════════════════════════════════════════════════════════════════════════════

namespace SiegePlacementWheelFixture
{
	/**
	 *  ⛔ THE FIXTURE'S TUNABLES ARE ⛔ NOT THE SHIPPED ONES, and that is the whole point:
	 *  a test written against 0.1 / 1.0 / 1.5 would be transcribing the spec back at itself
	 *  and would still pass if the wheel hardcoded them. These are deliberately different in
	 *  every digit — and the minimum is deliberately BELOW 1.0, so a seam that had baked in
	 *  "the identity is the floor" fails here.
	 */
	constexpr float FixtureStep = 0.25f;
	constexpr float FixtureMin  = 0.5f;
	constexpr float FixtureMax  = 2.f;

	/** The seam under test, spelled once so no test re-types its five arguments. */
	static float Notch(float Current, int32 Notches)
	{
		return ASiegePlayerController::StepPlacementFootprintScale(Current, Notches, FixtureStep, FixtureMin, FixtureMax);
	}

	/** The scale-vector seam, spelled once. */
	static FVector ScaleVector(float FootprintScale)
	{
		return ASiegePlayerController::MakePlacementFootprintScale3D(FootprintScale);
	}

	/** The exclusion seam, spelled once. */
	static bool CanScale(const UClass* CardActorClass)
	{
		return ASiegePlayerController::CanCardActorScaleFootprint(CardActorClass);
	}

	/**
	 *  ⭐ Walks the wheel UP one notch at a time until it stops moving, and reports how many
	 *  notches that took. ⛔ The number of notches is never transcribed — it is MEASURED, so
	 *  the claims below survive any retune of the step or the range.
	 *
	 *  The loop guard is generous and is itself asserted by the caller: a run that ends on
	 *  the guard rather than on saturation would be a wheel that never clamps, and that must
	 *  read as a FAILURE rather than as a walk that finished.
	 */
	static int32 WalkToTheTop(float& OutFinalScale, int32& OutNotchesTaken, int32 LoopGuard)
	{
		float Current = FixtureMin;
		int32 Notches = 0;
		while (Notches < LoopGuard)
		{
			const float Next = Notch(Current, 1);
			if (Next == Current)
			{
				break;
			}
			Current = Next;
			++Notches;
		}
		OutFinalScale = Current;
		OutNotchesTaken = Notches;
		return Notches;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  24. ⭐ ONE NOTCH = ONE STEP, AND THE WHEEL CLAMPS AT **BOTH** ENDS — ⛔ including the
//      "⛔ no shrinking" floor (`J-3`), which is a RULING and not a rounding
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementWheelStepsOneNotchAndClampsBothEndsTest,
	"Siegebound.Placement.ThePlacementFootprintWheelStepsOneNotchAndClampsAtBothEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementWheelStepsOneNotchAndClampsBothEndsTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;   // Exact / Tolerance / DiffersFrom / MakeQuietNaN / TryReadShippedFloat
	using namespace SiegePlacementWheelFixture;

	// (a) ⭐ ONE NOTCH = ONE STEP. ⛔ Not two, ⛔ not a fraction, ⛔ not a fixed increment that
	//     ignores the tunable entirely — the last of which is the failure a test written
	//     against the shipped 0.1 could not see.
	TestEqual(TEXT("(a) one notch UP from the minimum moves by EXACTLY one step"),
		Notch(FixtureMin, 1), FixtureMin + FixtureStep, Tolerance);
	TestEqual(TEXT("(a) one notch DOWN from a mid value moves by EXACTLY one step"),
		Notch(FixtureMin + 2.f * FixtureStep, -1), FixtureMin + FixtureStep, Tolerance);
	TestTrue(TEXT("(a) ⛔ and one notch is ⛔ NOT two steps — the claim above would pass a doubled increment if this did not"),
		DiffersFrom(Notch(FixtureMin, 1), FixtureMin + 2.f * FixtureStep));

	// (b) ⛔⛔ THE FLOOR IS A RULING (`J-3`: "⛔ NO SHRINKING — he gave only a max"). Scrolling
	//     DOWN at the minimum must do NOTHING. A wheel that shrank below the authored
	//     footprint would let a player hide a building in a gap its mesh never fitted.
	TestEqual(TEXT("(b) ⛔ one notch DOWN at the minimum stays pinned at the minimum (J-3: no shrinking)"),
		Notch(FixtureMin, -1), FixtureMin, Exact);
	TestEqual(TEXT("(b) ⛔ …and a hundred notches down is still the minimum, ⛔ never a negative scale"),
		Notch(FixtureMin, -100), FixtureMin, Exact);

	// (c) ⭐⭐ THE CEILING IS REACHED **EXACTLY**, AND THE NOTCH COUNT IS ⛔ MEASURED RATHER
	//     THAN TRANSCRIBED. Walking rather than counting is what keeps this claim true after
	//     any retune — and "lands exactly on the maximum" is the property a wheel that
	//     accumulated float error past its clamp would fail.
	float FinalScale = 0.f;
	int32 NotchesTaken = 0;
	constexpr int32 LoopGuard = 1000;
	WalkToTheTop(FinalScale, NotchesTaken, LoopGuard);

	TestTrue(TEXT("(c) SELF-CHECK: the walk ended by SATURATING, ⛔ not by hitting its loop guard — a wheel that never clamps must read as a failure, ⛔ not as a finished walk"),
		NotchesTaken > 0 && NotchesTaken < LoopGuard);
	TestEqual(TEXT("(c) ⭐⭐ walking one notch at a time lands ⛔ EXACTLY on the maximum — ⛔ not near it"),
		FinalScale, FixtureMax, Exact);
	TestEqual(TEXT("(c) ⭐ …and it SATURATES there: further notches change nothing"),
		Notch(FinalScale, 1), FixtureMax, Exact);
	TestEqual(TEXT("(c) ⛔ a single huge notch delta clamps to the maximum rather than overflowing past it"),
		Notch(FixtureMin, 1000000), FixtureMax, Exact);

	// (d) ⭐ THE WHEEL IS ⛔ NOT DEAD AT THE TOP — the property a notch COUNTER would have
	//     broken. One notch down from the ceiling must move IMMEDIATELY, by exactly one step;
	//     a counter that had over-accumulated would need as many notches back as the player
	//     over-scrolled forward, and would feel broken.
	TestEqual(TEXT("(d) ⭐ one notch DOWN from the maximum moves immediately, by exactly one step"),
		Notch(FixtureMax, -1), FixtureMax - FixtureStep, Tolerance);

	// (e) NO NOTCH ⇒ NO CHANGE. This is the frame where the player did not touch the wheel
	//     (and the frame where BOTH directions fired and cancelled), and it must be a no-op
	//     at every point of the range including the two clamped ends.
	TestEqual(TEXT("(e) a zero notch delta is a no-op mid-range"), Notch(FixtureMin + FixtureStep, 0), FixtureMin + FixtureStep, Exact);
	TestEqual(TEXT("(e) …at the floor"), Notch(FixtureMin, 0), FixtureMin, Exact);
	TestEqual(TEXT("(e) …and at the ceiling"), Notch(FixtureMax, 0), FixtureMax, Exact);

	// (f) ⭐ THE INCREMENT IS CONSTANT ACROSS THE RANGE — the SHAPE claim, ⛔ not a magnitude.
	//     A wheel that scaled multiplicatively (×1.1 per notch) reproduces the first notch
	//     and diverges immediately afterwards; this catches it without naming a single number.
	{
		float Walk = FixtureMin;
		bool bConstant = true;
		int32 Samples = 0;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float Next = Notch(Walk, 1);
			if (Next >= FixtureMax) { break; }   // stop before the clamp, which is not an increment
			bConstant = bConstant && FMath::IsNearlyEqual(Next - Walk, FixtureStep, Tolerance);
			Walk = Next;
			++Samples;
		}
		TestTrue(TEXT("(f) SELF-CHECK: at least two un-clamped increments were actually sampled"), Samples >= 2);
		TestTrue(TEXT("(f) ⭐ the increment is CONSTANT across the range — ⛔ additive, ⛔ never multiplicative"), bConstant);
	}

	// (g) ⛔ MISCONFIGURATION DEGRADES OPEN, ⛔ NEVER TO A BUILDING OF UNBOUNDED SIZE.
	//     `ClampMin` guards the editor FIELD only; a hand-edited .uasset or a bad merge can
	//     still deliver any of these, and this seam ends up multiplying a real collision and
	//     navmesh footprint.
	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(g) SELF-CHECK: the NaN really is a NaN (a build that folded it away must report itself)"), FMath::IsNaN(NaNValue));

	TestEqual(TEXT("(g) a ZERO step is INERT — the honest failure, ⛔ never a silently invented default step"),
		ASiegePlayerController::StepPlacementFootprintScale(FixtureMin, 1, 0.f, FixtureMin, FixtureMax), FixtureMin, Exact);
	TestEqual(TEXT("(g) a NEGATIVE step is inert too — ⛔ it must not invert the wheel's direction"),
		ASiegePlayerController::StepPlacementFootprintScale(FixtureMin, 1, -FixtureStep, FixtureMin, FixtureMax), FixtureMin, Exact);
	TestEqual(TEXT("(g) a NaN step is inert"),
		ASiegePlayerController::StepPlacementFootprintScale(FixtureMin, 1, NaNValue, FixtureMin, FixtureMax), FixtureMin, Exact);
	TestTrue(TEXT("(g) ⛔ a NaN CURRENT scale is re-seeded, ⛔ never propagated — a NaN scale reaching an FTransform makes the building's bounds uncomputable"),
		FMath::IsFinite(ASiegePlayerController::StepPlacementFootprintScale(NaNValue, 1, FixtureStep, FixtureMin, FixtureMax)));
	TestEqual(TEXT("(g) a maximum BELOW the minimum collapses the range to the minimum — ⛔ never an inverted interval"),
		ASiegePlayerController::StepPlacementFootprintScale(FixtureMin, 1, FixtureStep, FixtureMax, FixtureMin), FixtureMax, Exact);
	TestEqual(TEXT("(g) a NaN minimum falls back to the unscaled IDENTITY, ⛔ never to a restated ceiling"),
		ASiegePlayerController::StepPlacementFootprintScale(5.f, 1, FixtureStep, NaNValue, NaNValue), 1.f, Exact);

	// (h) ⭐⭐ THE CONTROL WITHOUT WHICH EVERY "INERT" ROW ABOVE IS WORTHLESS: the SAME call
	//     with the one broken term restored must MOVE. A seam that returned its input for
	//     everything passes (b), (e) and (g) perfectly and fails only here.
	TestTrue(TEXT("(h) ⭐⭐ CONTROL — the identical call with a VALID step MOVES, so every 'inert' row above is the guard working and ⛔ not a frozen fixture"),
		DiffersFrom(ASiegePlayerController::StepPlacementFootprintScale(FixtureMin, 1, FixtureStep, FixtureMin, FixtureMax), FixtureMin));

	// (i) 🧑 THE SHIPPED TUNABLES, ASSERTED AS RELATIONSHIPS AND ONE RULING — ⛔ never as
	//     transcribed magnitudes (`STACK-§4`: "step small enough that the max is reachable in
	//     a few notches"; `J-3`: the floor is the unscaled identity).
	float ShippedStep = 0.f, ShippedMin = 0.f, ShippedMax = 0.f;
	const bool bReadStep = TryReadShippedFloat(TEXT("PlacementFootprintWheelStep"), ShippedStep);
	const bool bReadMin  = TryReadShippedFloat(TEXT("PlacementFootprintMin"), ShippedMin);
	const bool bReadMax  = TryReadShippedFloat(TEXT("PlacementFootprintMax"), ShippedMax);
	TestTrue(TEXT("(i) SELF-CHECK: all three wheel tunables are reflected EditDefaultsOnly floats on the controller — a renamed or dropped UPROPERTY must FAIL rather than default to 0"),
		bReadStep && bReadMin && bReadMax);

	if (bReadStep && bReadMin && bReadMax)
	{
		TestEqual(TEXT("(i) ⚖️ J-3 — the shipped MINIMUM is the unscaled IDENTITY: ⛔ the wheel can never shrink a building below its authored footprint"),
			ShippedMin, 1.f, Exact);
		TestTrue(TEXT("(i) the shipped maximum is genuinely ABOVE the minimum — a collapsed range would ship a wheel that does nothing"),
			ShippedMax > ShippedMin);
		TestTrue(TEXT("(i) the shipped step is positive — a 0 would ship the wheel INERT"), ShippedStep > 0.f);

		const int32 NotchesToMax = FMath::CeilToInt((ShippedMax - ShippedMin) / ShippedStep);
		TestTrue(FString::Printf(TEXT("(i) ⭐ STACK-§4 — the maximum is reachable in a FEW notches (measured: %d), and in more than one so the wheel has resolution"), NotchesToMax),
			NotchesToMax >= 2 && NotchesToMax <= 12);

		// ⭐ And the shipped values walk to their own ceiling exactly, not merely near it.
		float ShippedWalk = ShippedMin;
		for (int32 Index = 0; Index < LoopGuard; ++Index)
		{
			const float Next = ASiegePlayerController::StepPlacementFootprintScale(ShippedWalk, 1, ShippedStep, ShippedMin, ShippedMax);
			if (Next == ShippedWalk) { break; }
			ShippedWalk = Next;
		}
		TestEqual(TEXT("(i) ⭐ the SHIPPED tunables also walk to their ceiling ⛔ exactly — float drift may not leave the maximum unreachable"),
			ShippedWalk, ShippedMax, Exact);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  25. ⭐⭐ THE WHEEL SCALES **X AND Y ONLY** — Z belongs to the STACK upgrade, and the
//      footprint the gates validate scales by ⛔ exactly the wheel's factor (`STACK-§6`)
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementWheelScalesXAndYOnlyTest,
	"Siegebound.Placement.ThePlacementFootprintScaleTouchesWidthAndLengthOnlyAndNeverHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementWheelScalesXAndYOnlyTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;
	using namespace SiegePlacementWheelFixture;

	// (a) ⛔⛔ Z IS ALWAYS EXACTLY 1. The Z axis is `ABuilding::ApplyStackUpgrade`'s, which
	//     recomputes it from the AUTHORED baseline every time — a Z smuggled in here would
	//     either be erased by the first upgrade or become a second, silent factor in a series
	//     that is meant to be a pure function of StackUpgradeCount (`J-4`).
	for (const float Sample : { 1.f, 1.25f, 1.5f, 3.f })
	{
		const FVector Vector = ScaleVector(Sample);
		TestEqual(FString::Printf(TEXT("(a) ⛔ Z is exactly 1 at scale %.2f — the height axis belongs to the STACK upgrade, ⛔ never to the wheel"), Sample),
			static_cast<float>(Vector.Z), 1.f, Exact);
		TestEqual(FString::Printf(TEXT("(a) X carries the factor at scale %.2f"), Sample), static_cast<float>(Vector.X), Sample, Exact);
		TestEqual(FString::Printf(TEXT("(a) …and Y carries the SAME factor at scale %.2f (⛔ never an oblong the ghost's facing could change)"), Sample),
			static_cast<float>(Vector.Y), Sample, Exact);
	}

	// (b) ⛔ NON-FINITE AND NON-POSITIVE DEGRADE TO THE UNSCALED IDENTITY. A negative scale
	//     would MIRROR the mesh (inverted normals, inside-out collision); a NaN would make the
	//     spawned actor's bounds uncomputable.
	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(b) SELF-CHECK: the NaN really is a NaN"), FMath::IsNaN(NaNValue));
	TestTrue(TEXT("(b) a NaN scale yields the unscaled identity"), ScaleVector(NaNValue).Equals(FVector::OneVector, Tolerance));
	TestTrue(TEXT("(b) ⛔ a NEGATIVE scale yields the identity — ⛔ never a mirrored building"), ScaleVector(-2.f).Equals(FVector::OneVector, Tolerance));
	TestTrue(TEXT("(b) a ZERO scale yields the identity — ⛔ never a degenerate zero-size actor"), ScaleVector(0.f).Equals(FVector::OneVector, Tolerance));

	// (c) ⭐⭐ THE LINK TO `STACK-§6`, ASSERTED THROUGH THE **REAL** BOUNDS TRANSFORM RATHER
	//     THAN DESCRIBED. `TryGetPlacementFootprintRadius` reads
	//     `GhostMesh->CalcBounds(GetComponentTransform())` and reduces it with
	//     `PlacementFootprintRadiusFromBounds`. Reproducing that exact path here proves the
	//     claim the whole task rests on: ⭐ WHEEL THE GHOST AND THE VALIDATED FOOTPRINT
	//     FOLLOWS, with ⛔ zero further code — so a ×1.5 building can never be validated at ×1.
	//
	//     ⚠️ The ghost's yaw is READ off the CDO, ⛔ not transcribed: the rotation is in the
	//     answer (the world AABB is built from the abs of the rotation matrix), so a test that
	//     assumed an identity rotation would be testing a ghost the game does not spawn.
	float GhostYaw = 0.f;
	const bool bReadYaw = TryReadShippedFloat(TEXT("GhostYawOffset"), GhostYaw);
	TestTrue(TEXT("(c) SELF-CHECK: GhostYawOffset is readable off the controller CDO"), bReadYaw);

	// Y-DOMINANT on purpose: with the shipped quarter-turn yaw the world AABB swaps the axes,
	// so an implementation that scaled only ONE axis is caught by this extent and not by a square one.
	const FVector LocalExtent(200.f, 300.f, 900.f);
	const FRotator GhostRotation(0.f, GhostYaw, 0.f);

	const float BaseRadius = RadiusOf(ScaledExtent(LocalExtent, FTransform(GhostRotation, FVector::ZeroVector, ScaleVector(1.f))));
	TestTrue(TEXT("(c) SELF-CHECK: the unscaled ghost has a real, positive footprint radius to scale FROM"), BaseRadius > 0.f);

	for (const float Sample : { 1.25f, 1.5f, 2.f })
	{
		const float ScaledRadius = RadiusOf(ScaledExtent(LocalExtent, FTransform(GhostRotation, FVector::ZeroVector, ScaleVector(Sample))));
		TestEqual(FString::Printf(TEXT("(c) ⭐⭐ the VALIDATED footprint radius scales by ⛔ exactly the wheel's factor at x%.2f (STACK-§6: a 1.5x building can never be validated at 1.0x)"), Sample),
			ScaledRadius, BaseRadius * Sample, RotationTolerance);
	}

	// (d) ⭐ THE NEGATIVE CONTROL THAT GIVES (c) ITS TEETH: an ASYMMETRIC scale — the exact
	//     mistake of scaling X and leaving Y — does ⛔ NOT scale this footprint at all, because
	//     the ghost's quarter turn puts the untouched axis in the dominant slot. ⇒ (c) is a
	//     statement about `MakePlacementFootprintScale3D` returning X == Y, ⛔ not a tautology.
	const float AsymmetricRadius =
		RadiusOf(ScaledExtent(LocalExtent, FTransform(GhostRotation, FVector::ZeroVector, FVector(1.5f, 1.f, 1.f))));
	TestTrue(TEXT("(d) ⭐ CONTROL — an X-only scale FAILS to grow this footprint, so (c) is measuring X == Y and ⛔ not agreeing with itself"),
		DiffersFrom(AsymmetricRadius, BaseRadius * 1.5f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  26. ⛔⛔ THE WHEEL IS **INERT** FOR ANY CLASS THAT REFUSES FOOTPRINT SCALING — the SAME
//      `CanScaleFootprint()` predicate as the upgrade, ⛔ never a `CardID` name compare
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementWheelIsInertForNonScalableCardsTest,
	"Siegebound.Placement.ThePlacementWheelIsInertForAnyCardClassThatRefusesFootprintScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementWheelIsInertForNonScalableCardsTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;  // MakeScratchBuilding
	using namespace SiegePlacementWheelFixture;

	// (a) ⭐ THE CONTROL FIRST, so every "false" below means something. A plain building card
	//     may be wheeled; if this were false the whole feature would be dead and every
	//     exclusion row would still be green.
	TestTrue(TEXT("(a) ⭐ CONTROL — a plain ABuilding class MAY be footprint-scaled, so the 'false' rows below are exclusions and ⛔ not a dead predicate"),
		CanScale(ABuilding::StaticClass()));

	// (b) ⛔⛔ THE CLAIM. `AClimbableTower` — the WatchTower's class — refuses. A scaled
	//     SM_WatchTower moves the LadderFoot/LadderTop sockets and the rung plane, fires
	//     `TOWER-§8.5a`'s voiding condition, and the climb stops working ENTIRELY while every
	//     readback still reports correct.
	TestFalse(TEXT("(b) ⛔⛔ AClimbableTower refuses footprint scaling — the WatchTower is excluded from the WHEEL by the same rule that excludes it from the UPGRADE"),
		CanScale(AClimbableTower::StaticClass()));

	// (c) ⭐ THE EXCLUSION IS ⛔ NOT OVER-BROAD. `ATower` is the Arrow/Bomb/Ballista/Crystal
	//     family — the towers Jonathan has played with for weeks — and it still scales.
	TestTrue(TEXT("(c) ⭐ ATower (the Arrow/Bomb/Ballista/Crystal family) still scales — the exclusion catches ⛔ only the climbable one"),
		CanScale(ATower::StaticClass()));

	// (d) ⭐ A UNIT CARD'S CLASS IS INERT **FOR FREE**, with ⛔ no separate card-type rule
	//     invented for the wheel: `ASummonedUnit` is not an `ABuilding`, so the same cast that
	//     asks the question also answers it.
	TestFalse(TEXT("(d) ⭐ ASummonedUnit — the class a UNIT card resolves to — is inert, with ⛔ no separate 'is this a building' rule"),
		CanScale(ASummonedUnit::StaticClass()));
	TestFalse(TEXT("(d) …and so is an arbitrary non-building class, so the cast is genuinely the whole rule"),
		CanScale(ASiegePlayerController::StaticClass()));

	// (e) ⛔ A FAILED BP RESOLVE IS INERT, ⛔ never a crash and ⛔ never a default of true.
	TestFalse(TEXT("(e) ⛔ a null class (the missing-BP degrade) is inert"), CanScale(nullptr));

	// (f) ⭐⭐ THE ONE ASSUMPTION THIS SEAM RESTS ON, ASSERTED RATHER THAN ASSUMED: the wheel
	//     runs BEFORE anything is spawned, so it must ask the CLASS. That is only sound while
	//     `CanScaleFootprint()` reads no instance state — so the CDO's answer is compared here
	//     against a REAL instance's, asked through an `ABuilding*` (which also makes this a
	//     virtual-dispatch check: a shadowed non-virtual override passes every row but this).
	TStrongObjectPtr<AClimbableTower> Climbable = MakeScratchBuilding<AClimbableTower>(ETeamId::Blue, FName(TEXT("SiegeboundTestTowerA")));
	TStrongObjectPtr<ABuilding> Plain = MakeScratchBuilding<ABuilding>(ETeamId::Blue, FName(TEXT("SiegeboundTestTowerA")));
	if (Climbable.IsValid() && Plain.IsValid())
	{
		const ABuilding* const ClimbableAsBase = Climbable.Get();
		const ABuilding* const PlainAsBase = Plain.Get();
		// ⛔ TestTrue on an equality, ⛔ not TestEqual on two bools: `FAutomationTestBase` has no
		// bool overload, and "expected 1, got 0" is exactly the failure message this file's
		// charter calls useless. Naming both operands makes a red line say WHICH side moved.
		TestTrue(TEXT("(f) ⭐⭐ the CDO's answer equals a live AClimbableTower's, asked through an ABuilding* — the class is a sound oracle for every instance it will make (both must be FALSE)"),
			CanScale(AClimbableTower::StaticClass()) == ClimbableAsBase->CanScaleFootprint()
			&& !ClimbableAsBase->CanScaleFootprint());
		TestTrue(TEXT("(f) ⭐ …and the same holds for a plain building, so (f) is ⛔ not two falses agreeing by accident (both must be TRUE)"),
			CanScale(ABuilding::StaticClass()) == PlainAsBase->CanScaleFootprint()
			&& PlainAsBase->CanScaleFootprint());
	}
	else
	{
		AddError(TEXT("SELF-CHECK FAILED: a scratch building could not be created, so the CDO-vs-instance claim could not be made."));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  27. ⭐⭐ `MARK-§4`'s AMENDMENT, MEASURED IN THE DIFF RATHER THAN PROMISED IN THE PROSE:
//      the poll is the THIRD consumer, it lives ⛔ only in the placement branch, it adds
//      ⛔ NO `InputAction`, and it reuses ⛔ NONE of the group-pick tunables
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementWheelIsTheThirdMarkConsumerTest,
	"Siegebound.Placement.ThePlacementWheelPollLivesOnlyInThePlacementBranchAndReusesNoGroupPickTunable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementWheelIsTheThirdMarkConsumerTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;  // LoadProjectSource / CountOccurrencesInCode / the two paths
	using namespace SiegeDiscardAllFixture;        // ExtractControllerFunctionBody / CodeLinesOnly / CheckPrecedes

	FString SourceText;
	if (!LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	FString TickBody, WheelBody;
	const bool bGotTick = ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::PlayerTick(float DeltaTime)"), TickBody);
	const bool bGotWheel = ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::ApplyPlacementFootprintWheel()"), WheelBody);
	if (!bGotTick || !bGotWheel)
	{
		return false;
	}

	const FString TickCode = CodeLinesOnly(TickBody);

	// (a) ⭐ EXACTLY ONE CALL SITE IN THE WHOLE FILE. Two occurrences on code lines: this
	//     function's own definition signature, and the ONE call. A poll added to a second
	//     branch — the very thing `MARK-§4` forbids without another amendment — makes this 3.
	TestEqual(TEXT("(a) ⭐ 'ApplyPlacementFootprintWheel()' appears exactly TWICE on code lines: its definition and its ONE call site"),
		CountOccurrencesInCode(SourceText, TEXT("ApplyPlacementFootprintWheel()")), 2);
	TestEqual(TEXT("(a) …and exactly ONE of those two is inside PlayerTick"),
		CountOccurrencesInCode(TickBody, TEXT("ApplyPlacementFootprintWheel();")), 1);

	// (b) SELF-CHECK: the extractor really got PlayerTick and not an empty string — without
	//     this, every count and every ordering claim here would report SAFE on a stale probe.
	TestEqual(TEXT("(b) SELF-CHECK: the extracted body really is PlayerTick (its shipped group-pick poll is there, exactly once)"),
		CountOccurrencesInCode(TickBody, TEXT("ApplyGroupPickWheel();")), 1);

	// (c) ⭐⭐ THE TWO POLLS SIT ON OPPOSITE SIDES OF THE PLACEMENT GATE, WHICH IS `STACK-§4`'s
	//     "refuted at source" claim turned into an assertion. The group-pick branch `return`s
	//     before `if (!bInPlacementMode)` is ever reached, so consumer 1 and consumer 3 can
	//     ⛔ never run in the same frame — and that is why ⛔ no guard was added for a state
	//     that cannot exist.
	CheckPrecedes(*this, TickCode, TEXT("ApplyGroupPickWheel();"), TEXT("if (!bInPlacementMode)"));
	CheckPrecedes(*this, TickCode, TEXT("if (!bInPlacementMode)"), TEXT("ApplyPlacementFootprintWheel();"));

	// (d) ⭐⭐ AND THE ORDER **WITHIN** THE PLACEMENT BRANCH IS LOAD-BEARING, ⛔ not cosmetic:
	//     `UpdatePlacementGhost` reads the ghost's SCALED bounds for the footprint gates
	//     (`STACK-§6`), so a wheel polled AFTER it would validate this frame's click against
	//     LAST frame's size — a green the confirm could refuse. ⚠️ This row is the one that
	//     catches the one-frame lie, and it is invisible to every behavioural test.
	CheckPrecedes(*this, TickCode, TEXT("ApplyPlacementFootprintWheel();"), TEXT("UpdatePlacementGhost();"));

	// (e) ⛔ NO NEW `InputAction` — `MARK-§4`'s wheel law, unamended in this respect. The
	//     wheel stays a POLL, exactly like consumer 1.
	TestEqual(TEXT("(e) ⛔ the wheel body binds NO InputAction"), CountOccurrencesInCode(WheelBody, TEXT("BindAction")), 0);
	TestEqual(TEXT("(e) ⛔ …and resolves none"), CountOccurrencesInCode(WheelBody, TEXT("ResolveInputAction")), 0);
	TestEqual(TEXT("(e) ⛔ …and names no UInputAction at all"), CountOccurrencesInCode(WheelBody, TEXT("UInputAction")), 0);
	TestEqual(TEXT("(e) ⭐ it POLLS the wheel up exactly once"),
		CountOccurrencesInCode(WheelBody, TEXT("WasInputKeyJustPressed(EKeys::MouseScrollUp)")), 1);
	TestEqual(TEXT("(e) ⭐ …and down exactly once"),
		CountOccurrencesInCode(WheelBody, TEXT("WasInputKeyJustPressed(EKeys::MouseScrollDown)")), 1);

	// (f) ⛔⛔ `MARK-§4`'s OWN CLAUSE: the group-pick tunables are WORLD-SPACE RADII in uu
	//     (100 / 200 / 5000) and are ⛔ meaningless as a scale factor — reusing one would grow
	//     a building to x5000. ⭐ The zeroes are paired with a SELF-CHECK proving the scanner
	//     finds all three on code lines elsewhere in this same file, so a zero here is the
	//     separation working and ⛔ not a renamed token the probe can no longer see.
	for (const TCHAR* Forbidden : { TEXT("GroupRadiusWheelStep"), TEXT("GroupRadiusMin"), TEXT("GroupRadiusMax") })
	{
		TestTrue(FString::Printf(TEXT("(f) SELF-CHECK: '%s' IS findable on code lines elsewhere in this file, so the zero below means separation"), Forbidden),
			CountOccurrencesInCode(SourceText, Forbidden) > 0);
		TestEqual(FString::Printf(TEXT("(f) ⛔⛔ the placement wheel reuses '%s' ZERO times — it is a world-space radius and meaningless as a scale factor (MARK-§4)"), Forbidden),
			CountOccurrencesInCode(WheelBody, Forbidden), 0);
	}

	// (g) ⛔⛔ `STACK-§2`'s AUTOMATIC FAIL, RE-ASSERTED FOR THE WHEEL'S OWN CODE: the exclusion
	//     is the ONE cached predicate, and there is ⛔ no name compare beside it.
	TestEqual(TEXT("(g) ⭐ the wheel gates on the structural predicate exactly once"),
		CountOccurrencesInCode(WheelBody, TEXT("bPendingCardCanScaleFootprint")), 1);
	TestEqual(TEXT("(g) ⛔⛔ and names 'WatchTower' ZERO times on code lines — a CardID string compare here is an automatic FAIL"),
		CountOccurrencesInCode(WheelBody, TEXT("WatchTower")), 0);
	TestTrue(TEXT("(g) SELF-CHECK: 'WatchTower' IS present in this file's PROSE, so the zero above is the comment-skipper working and ⛔ not an absent token"),
		SourceText.Contains(TEXT("WatchTower"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  28. ⭐⭐ THE GHOST AND THE SPAWNED BUILDING ARE SIZED FROM **ONE** VALUE BY **ONE**
//      EXPRESSION (spec (4)) — the PROPERTY, ⛔ not a number
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementGhostAndSpawnAgreeOnOneScaleTest,
	"Siegebound.Placement.TheGhostScaleAndTheSpawnedBuildingScaleAreTheSameValueByConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementGhostAndSpawnAgreeOnOneScaleTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;
	using namespace SiegeDiscardAllFixture;

	FString SourceText;
	if (!LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	FString GhostBody, ConfirmBody, WheelBody;
	const bool bGotGhost = ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::UpdatePlacementGhost()"), GhostBody);
	const bool bGotConfirm = ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::TryConfirmPlacement()"), ConfirmBody);
	const bool bGotWheel = ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::ApplyPlacementFootprintWheel()"), WheelBody);
	if (!bGotGhost || !bGotConfirm || !bGotWheel)
	{
		return false;
	}

	// SELF-CHECKS: each extracted body really is the function it claims to be. Without these,
	// every "exactly one" below would report SAFE on an empty string.
	TestTrue(TEXT("SELF-CHECK: the ghost body really is UpdatePlacementGhost (its shipped GhostColor write is there)"),
		CountOccurrencesInCode(GhostBody, TEXT("SetVectorParameterValue(GhostColorParamName")) == 1);
	TestTrue(TEXT("SELF-CHECK: the confirm body really is TryConfirmPlacement (its shipped SpawnActorDeferred is there)"),
		CountOccurrencesInCode(ConfirmBody, TEXT("SpawnActorDeferred<ABuilding>")) == 1);

	// (a) ⭐⭐ ONE EXPRESSION, TWO CONSUMERS. The ghost's scale and the spawned building's
	//     scale are the SAME member run through the SAME function — so there is no second
	//     expression that can drift. ⚖️ A ghost that lies about the thing it previews is the
	//     defect this whole placement path exists to avoid, and here it would be a lie with
	//     teeth: the clearance the click was validated against was measured off the SCALED ghost.
	TestEqual(TEXT("(a) ⭐ the ghost is sized from the wheel's member exactly once"),
		CountOccurrencesInCode(GhostBody, TEXT("MakePlacementFootprintScale3D(PlacementFootprintScale)")), 1);
	TestEqual(TEXT("(a) ⭐⭐ …and the SPAWN transform is sized from the SAME member by the SAME function, exactly once"),
		CountOccurrencesInCode(ConfirmBody, TEXT("MakePlacementFootprintScale3D(PlacementFootprintScale)")), 1);

	// (b) ⭐ AND THERE IS NO THIRD. Three occurrences file-wide = the definition + those two
	//     call sites. A fourth would be a consumer sizing something nobody reviewed.
	TestEqual(TEXT("(b) ⭐ 'MakePlacementFootprintScale3D(' appears exactly THREE times on code lines: its definition and its TWO consumers"),
		CountOccurrencesInCode(SourceText, TEXT("MakePlacementFootprintScale3D(")), 3);

	// (c) ⛔ NOBODY BUILDS THE SCALE VECTOR BY HAND. This is the exact way the two sites would
	//     drift apart later: one of them "inlined" as an FVector literal. ⭐ Paired with a
	//     self-check proving the scanner does find FVector( on code lines in this file.
	TestTrue(TEXT("(c) SELF-CHECK: 'FVector(' IS findable on code lines in this file, so the zeroes below mean something"),
		CountOccurrencesInCode(SourceText, TEXT("FVector(")) > 0);
	TestEqual(TEXT("(c) ⛔ the ghost path never hand-builds a scale vector from the member"),
		CountOccurrencesInCode(GhostBody, TEXT("FVector(PlacementFootprintScale")), 0);
	TestEqual(TEXT("(c) ⛔ nor does the confirm path"),
		CountOccurrencesInCode(ConfirmBody, TEXT("FVector(PlacementFootprintScale")), 0);

	// (d) ⭐⭐ ONE WRITER FOR THE GHOST'S TRANSFORM, AND IT IS ⛔ NOT THE WHEEL. The wheel
	//     writes the NUMBER; `UpdatePlacementGhost` writes the TRANSFORM, unconditionally,
	//     every frame. ⇒ the ghost can never disagree with the member — not on the first frame
	//     of a session, and not if `PlacementFootprintMin` is ever retuned off 1.0, which a
	//     scale applied only when a notch lands would silently get wrong.
	TestEqual(TEXT("(d) ⭐ the ghost's scale is written in exactly ONE place"),
		CountOccurrencesInCode(GhostBody, TEXT("SetActorScale3D(")), 1);
	TestEqual(TEXT("(d) ⭐⭐ …and the WHEEL does not touch the ghost at all — it owns the number, ⛔ not the actor"),
		CountOccurrencesInCode(WheelBody, TEXT("SetActorScale3D(")), 0);
	TestEqual(TEXT("(d) ⛔ the wheel does not reach GhostActor by any route"),
		CountOccurrencesInCode(WheelBody, TEXT("GhostActor")), 0);

	// (e) ⭐⭐ THE ORDERING THAT MAKES `STACK-§6` HOLD, AND IT IS INVISIBLE TO EVERY
	//     BEHAVIOURAL TEST: the ghost is SIZED before its footprint is MEASURED. Written the
	//     other way round the gates would validate a ×1.5 building against a ×1.0 footprint —
	//     silently, and in exactly the class of defect `TOWER-§7` exists to close.
	CheckPrecedes(*this, CodeLinesOnly(GhostBody), TEXT("SetActorScale3D("), TEXT("TryGetPlacementFootprintRadius("));

	// (f) ⭐ `J-4` — "keeping the same width and length" — needs ⛔ NO coordination between the
	//     two features, and this row records why: the wheel writes X/Y through the transform
	//     (VisualMesh is ABuilding's ROOT), and `ApplyStackUpgrade` recomputes Z ALONE from its
	//     authored baseline. Measured in TASK-812's file, from inside this fence, read-only.
	FString BuildingSource;
	if (LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/Building.cpp"), BuildingSource))
	{
		FString UpgradeBody;
		const int32 UpgradeStart = BuildingSource.Find(TEXT("bool ABuilding::ApplyStackUpgrade()"), ESearchCase::CaseSensitive);
		if (UpgradeStart != INDEX_NONE)
		{
			const int32 NextDef = BuildingSource.Find(TEXT("\nfloat ABuilding::"), ESearchCase::CaseSensitive, ESearchDir::FromStart, UpgradeStart);
			UpgradeBody = BuildingSource.Mid(UpgradeStart, (NextDef == INDEX_NONE ? BuildingSource.Len() : NextDef) - UpgradeStart);
		}
		TestTrue(TEXT("(f) SELF-CHECK: ApplyStackUpgrade's body was found and its shipped Z write is there"),
			CountOccurrencesInCode(UpgradeBody, TEXT("Scale.Z =")) == 1);
		TestEqual(TEXT("(f) ⭐ ApplyStackUpgrade writes ⛔ no X — the wheel's width survives every upgrade VERBATIM (J-4)"),
			CountOccurrencesInCode(UpgradeBody, TEXT("Scale.X =")), 0);
		TestEqual(TEXT("(f) ⭐ …and ⛔ no Y — his own words, 'keeping the same width and length'"),
			CountOccurrencesInCode(UpgradeBody, TEXT("Scale.Y =")), 0);
	}

	return true;
}

// ███████████████████████████████████████████████████████████████████████████████████████████
//  TASK-871 — THE FOUR-CORNER FOOTPRINT SLOPE PROBE (tests 29–31).
//  `qa/TASK-816.md` W-4 / ruling `R-3` · `STACK-§6` · `SC-§37` · `SC-§39` · `SC-§41`.
//
//  ⭐ IN THE PLACEMENT FRAME BY RIGHT, ⛔ not as a lodger: the subject is the slope gate of
//  the very validity chain this file was created for, fed by the very footprint radius
//  tests 1–3 already measure.
//
//  ⛔⛔ THE DEFECT, SO NOBODY HAS TO GO AND FIND IT: the slope gate was ONE straight-down
//  trace AT THE CURSOR, while the ghost is wide and — since TASK-815 — player-adjustable up
//  to ×1.5. ⇒ a structure whose CENTRE sat on a flat crown could OVERHANG A STEEP FLANK AND
//  PASS, and the wheel widened that overhang by up to half again. ⭐ The wheel did not create
//  the bug; it widened an exposure that was already there.
//
//  ⛔⛔ WHAT WOULD MAKE THESE TESTS WORTHLESS, NAMED FIRST (the file's charter, applied):
//  ⭐⭐ **"IT REFUSED" IS ALSO WHAT A GATE THAT REFUSES EVERYTHING REPORTS.** A tightened
//  gate is the one change where a test can be green for the worst possible reason. ⇒ every
//  refusal claim below is paired with an ADMISSION on the same fixture with one term moved,
//  and the headline pairing is `SC-§39`'s and `TASK-872` row (c)'s by name:
//    • ⭐ THE NEGATIVE CONTROL — a genuinely FLAT pad, wheeled to the shipped MAXIMUM, with
//      all five samples really taken, must ⛔ still PASS (test 30(b));
//    • ⭐ THE POSITIVE CLAIM — the same probe, one corner moved onto a flank, must REFUSE
//      while the CENTRE ALONE would still admit (test 30(c)). A gate that refuses everything
//      fails (b); a gate that refuses nothing fails (c); ⛔ neither can pass this block.
//
//  ⛔ WHAT THESE TESTS DO **NOT** COVER (`SC-§32`, stated so nobody mistakes green for done):
//    • ⛔ **`IsGroundSlopePlaceable` IS NEVER CALLED HERE.** It is private, needs a `UWorld`
//      and runs live `LineTraceSingleByChannel` sweeps. ⇒ test 30 drives the SAME PURE SEAMS
//      it drives and reproduces its ANY-SAMPLE-FAILS-REFUSES rule, and **test 31 proves the
//      shipped body really has that shape** by reading the source. ⛔ The two halves are only
//      worth anything TOGETHER, and that is why neither is omitted.
//    • ⛔ **NO REAL TERRAIN IS TRACED.** That the arena's hill flanks really read past the
//      limit closes on the shipped `SM_Hill_0N` authoring (≤30° faces, ≤8° crowns) and on
//      🧑 Jonathan's playtest — ⛔ never on this suite.
// ███████████████████████████████████████████████████████████████████████████████████████████

namespace SiegeSlopeProbeFixture
{
	/**
	 *  ⛔ A RADIUS THAT IS NOBODY'S SHIPPED NUMBER, so no claim below can accidentally agree
	 *  with a mesh, a clearance or a tunable. Deliberately not round.
	 */
	constexpr float FixtureRadius = 137.5f;

	/** The four seams under test, spelled once so no test re-types them. */
	static int32 NumSamples(float Radius)
	{
		return ASiegePlayerController::NumPlacementSlopeSamples(Radius);
	}

	static FVector SampleOffset(int32 Index, float Radius)
	{
		return ASiegePlayerController::PlacementSlopeSampleOffset(Index, Radius);
	}

	static float SlopeDegrees(const FVector& Normal)
	{
		return ASiegePlayerController::PlacementSurfaceSlopeDegrees(Normal);
	}

	static bool WithinLimit(const FVector& Normal, float MaxDegrees)
	{
		return ASiegePlayerController::IsSurfaceNormalWithinSlopeLimit(Normal, MaxDegrees);
	}

	/**
	 *  A unit surface normal tilted `Degrees` away from world +Z, in the +X direction.
	 *  ⭐ Built from the ANGLE the caller wants rather than transcribed, so `SlopeDegrees`
	 *  can be round-tripped against it and a radians/degrees or sign error goes RED.
	 */
	static FVector NormalTiltedBy(float Degrees)
	{
		const double Radians = FMath::DegreesToRadians(static_cast<double>(Degrees));
		return FVector(FMath::Sin(Radians), 0.0, FMath::Cos(Radians));
	}

	/**
	 *  ⛔⛔ THE SHIPPED LOOP'S RULE, REPRODUCED RATHER THAN CALLED, AND DECLARED AS SUCH:
	 *  `IsGroundSlopePlaceable` admits a point only when EVERY sample it took is within the
	 *  limit. That rule needs a world to exercise for real — ⇒ **test 31 is what proves the
	 *  shipped body has this shape**, and this helper is what lets the GEOMETRY be measured.
	 *  ⛔ Neither half is a substitute for the other.
	 */
	static bool AllSamplesAdmitted(const TArray<FVector>& SampleNormals, float MaxDegrees)
	{
		for (const FVector& Normal : SampleNormals)
		{
			if (!WithinLimit(Normal, MaxDegrees))
			{
				return false;
			}
		}
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  29. ⭐⭐ EVERY SAMPLE OFFSET IS **DERIVED FROM `FootprintRadius`** — ⛔ NOT ONE OF THEM IS
//      A TRANSCRIBED CORNER (`SC-§37`; `TASK-872` row (c) / `TASK-914` (c))
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementSlopeSampleOffsetsAreDerivedFromTheRadiusTest,
	"Siegebound.Placement.TheSlopeProbeSampleOffsetsAreDerivedFromTheFootprintRadiusAndNeverTranscribed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementSlopeSampleOffsetsAreDerivedFromTheRadiusTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;   // Exact / Tolerance / MakeQuietNaN
	using namespace SiegeSlopeProbeFixture;

	// (a) ⭐ THE CONTROL FIRST, so every "one sample" below means the DEGRADE and ⛔ not a
	//     feature that was never wired: a usable radius really does widen the probe.
	const int32 WideCount = NumSamples(FixtureRadius);
	TestTrue(TEXT("(a) ⭐ CONTROL — a usable footprint radius takes MORE than one sample, so the probe is genuinely live"),
		WideCount > 1);
	TestEqual(TEXT("(a) ⭐ …and it is the centre plus FOUR corners — a rectangle has four, and a probe that lost one would leave a diagonal unmeasured"),
		WideCount, 5);

	// (b) ⛔ THE SHIPPED DEGRADE IS KEPT (spec (4)): an unknown footprint falls back to the
	//     SINGLE straight-down trace, ⛔ never to a hard refusal. A gate that refused on a
	//     missing measurement would make an art-pipeline hiccup unplayable.
	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(b) SELF-CHECK: the NaN fixture really is non-finite, so the degrade row below is measuring what it claims"),
		FMath::IsNaN(NaNValue) && !FMath::IsFinite(NaNValue));
	const float PositiveInfinity = BitsToFloat(0x7F800000u);
	TestTrue(TEXT("(b) SELF-CHECK: the infinity fixture really is non-finite and positive, so the row it feeds is measuring what it claims"),
		!FMath::IsFinite(PositiveInfinity) && !FMath::IsNaN(PositiveInfinity) && PositiveInfinity > 0.f);
	for (const float Unusable : { 0.f, -1.f, -FixtureRadius, NaNValue, PositiveInfinity })
	{
		TestEqual(TEXT("(b) ⛔ an unusable footprint radius takes exactly ONE sample — the shipped single trace at the cursor"),
			NumSamples(Unusable), 1);
	}

	// (c) ⭐⭐ THE ROW THAT A TRANSCRIBED CORNER **CANNOT** SATISFY, AND IT IS THE WHOLE POINT
	//     OF THIS TEST: every offset is LINEAR IN THE RADIUS. Doubling the footprint doubles
	//     every sample offset EXACTLY. A hardcoded `FVector(200, 200, 0)` passes every other
	//     row in this file and fails this one — which is precisely how the wheel's ×1.5 is
	//     carried for free (`STACK-§6`'s payout, a second time).
	for (const float BaseRadius : { FixtureRadius, 1.f, 4321.75f })
	{
		for (int32 Index = 0; Index < NumSamples(BaseRadius); ++Index)
		{
			const FVector AtBase = SampleOffset(Index, BaseRadius);
			const FVector AtDouble = SampleOffset(Index, BaseRadius * 2.f);
			TestTrue(FString::Printf(TEXT("(c) ⭐⭐ sample %d at radius %.2f scales EXACTLY linearly: Offset(2R) == 2 * Offset(R) — a transcribed corner cannot do this"), Index, BaseRadius),
				AtDouble.Equals(AtBase * 2.0, Tolerance));
			TestTrue(FString::Printf(TEXT("(c) ⭐ …and sample %d is the UNIT-radius offset scaled by R, so R is genuinely the only input"), Index),
				AtBase.Equals(SampleOffset(Index, 1.f) * static_cast<double>(BaseRadius), Tolerance));
		}
	}

	// (d) ⭐ SAMPLE 0 IS THE CENTRE, EXACTLY. This is what makes the shipped trace survive the
	//     change unmoved: the FIRST thing the gate still does is the trace it always did, at
	//     the point it always did it.
	TestTrue(TEXT("(d) ⭐ sample 0 is the ZERO vector — the shipped trace, at the shipped point, unmoved"),
		SampleOffset(0, FixtureRadius).Equals(FVector::ZeroVector, Exact));
	TestTrue(TEXT("(d) ⭐ …and it stays the centre for an unusable radius too, so the degrade path traces the same point"),
		SampleOffset(0, NaNValue).Equals(FVector::ZeroVector, Exact));

	// (e) ⭐ EACH CORNER IS THE FOOTPRINT'S OWN HALF-EXTENT ON BOTH AXES, AND ⛔ FLAT IN Z.
	//     A sample carrying a Z would move the trace's ±Z bracket instead of the sample and
	//     would silently shorten the search window.
	int32 CornersSeen = 0;
	TSet<FVector> DistinctCorners;
	bool bSawPlusPlus = false, bSawPlusMinus = false, bSawMinusPlus = false, bSawMinusMinus = false;
	for (int32 Index = 1; Index < NumSamples(FixtureRadius); ++Index)
	{
		const FVector Corner = SampleOffset(Index, FixtureRadius);
		++CornersSeen;
		DistinctCorners.Add(Corner);
		TestEqual(FString::Printf(TEXT("(e) corner %d displaces the footprint's half-extent on X"), Index),
			static_cast<float>(FMath::Abs(Corner.X)), FixtureRadius, Tolerance);
		TestEqual(FString::Printf(TEXT("(e) corner %d displaces the footprint's half-extent on Y"), Index),
			static_cast<float>(FMath::Abs(Corner.Y)), FixtureRadius, Tolerance);
		TestEqual(FString::Printf(TEXT("(e) ⛔ corner %d carries NO Z — a sample is a PLANAR displacement"), Index),
			static_cast<float>(Corner.Z), 0.f, Exact);

		bSawPlusPlus   = bSawPlusPlus   || (Corner.X > 0.0 && Corner.Y > 0.0);
		bSawPlusMinus  = bSawPlusMinus  || (Corner.X > 0.0 && Corner.Y < 0.0);
		bSawMinusPlus  = bSawMinusPlus  || (Corner.X < 0.0 && Corner.Y > 0.0);
		bSawMinusMinus = bSawMinusMinus || (Corner.X < 0.0 && Corner.Y < 0.0);
	}

	// (f) ⭐ THE FOUR CORNERS ARE FOUR **DIFFERENT** CORNERS, asserted as a PROPERTY (all four
	//     quadrants covered, all four distinct) rather than as a transcription of a table. A
	//     copy-paste that repeated one quadrant leaves a whole side of every building
	//     unmeasured and would pass (c), (d) and (e) untouched.
	TestEqual(TEXT("(f) ⭐ four corner samples were offered"), CornersSeen, 4);
	TestEqual(TEXT("(f) ⭐⭐ …and all four are DISTINCT — a duplicated quadrant leaves one side of every building unmeasured"),
		DistinctCorners.Num(), 4);
	TestTrue(TEXT("(f) ⭐⭐ …and they cover ALL FOUR quadrants (+ +, + -, - +, - -), which is what makes them the footprint's corners"),
		bSawPlusPlus && bSawPlusMinus && bSawMinusPlus && bSawMinusMinus);

	// (g) ⛔ TOTAL FUNCTION: an index nobody offers answers the CENTRE rather than a garbage
	//     displacement. The loop can never produce these, so this guards the seam against its
	//     NEXT caller, not against its current one.
	for (const int32 OutOfRange : { -1, 5, 99 })
	{
		TestTrue(FString::Printf(TEXT("(g) ⛔ out-of-range sample index %d answers the centre, ⛔ never a garbage offset"), OutOfRange),
			SampleOffset(OutOfRange, FixtureRadius).Equals(FVector::ZeroVector, Exact));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  30. ⭐⭐ THE SHAPE: A CENTRE-ON-CROWN / CORNER-ON-FLANK PLACEMENT THAT **PASSES TODAY**
//      MUST **FAIL AFTER** — carried by the ⭐ NEGATIVE CONTROL that a FLAT pad at the
//      shipped MAXIMUM wheel scale ⛔ still passes (`SC-§39`; `TASK-914` (c))
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementSlopeProbeRefusesAnOverhangingFootprintAndAdmitsAFlatPadTest,
	"Siegebound.Placement.TheSlopeProbeRefusesACornerOnAFlankWhileAFlatPadAtTheMaximumWheelScaleStillPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementSlopeProbeRefusesAnOverhangingFootprintAndAdmitsAFlatPadTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementTestFixture;   // TryReadShippedFloat / Tolerance / MakeQuietNaN
	using namespace SiegeSlopeProbeFixture;

	// (a) SELF-CHECK: the shipped tunables are read off the CDO BY REFLECTION and ⛔ never
	//     transcribed, so every threshold below survives 🧑 a retune of any of them. A tunable
	//     that cannot be found FAILS rather than defaulting to a number this file invented.
	float SlopeLimit = 0.f, WheelMax = 0.f, WheelMin = 0.f;
	const bool bReadTunables =
		TryReadShippedFloat(TEXT("MaxPlacementSlopeDegrees"), SlopeLimit)
		&& TryReadShippedFloat(TEXT("PlacementFootprintMax"), WheelMax)
		&& TryReadShippedFloat(TEXT("PlacementFootprintMin"), WheelMin);
	if (!bReadTunables)
	{
		AddError(TEXT("SELF-CHECK FAILED: MaxPlacementSlopeDegrees / PlacementFootprintMax / PlacementFootprintMin are not all reflected floats on ASiegePlayerController — every claim in this test rests on them and none can be made."));
		return false;
	}
	TestTrue(TEXT("(a) SELF-CHECK: the shipped slope limit is a usable angle (strictly between flat and vertical), so 'below' and 'above' it both exist"),
		FMath::IsFinite(SlopeLimit) && SlopeLimit > 0.f && SlopeLimit < 90.f);
	TestTrue(TEXT("(a) SELF-CHECK: the wheel really can WIDEN a footprint (max > min), so the ×max control below is not the ×min case wearing a different name"),
		FMath::IsFinite(WheelMax) && FMath::IsFinite(WheelMin) && WheelMax > WheelMin && WheelMin > 0.f);

	// (b) ⭐⭐⭐ **THE NEGATIVE CONTROL** (`TASK-914` (c), verbatim): *"a genuinely flat pad at
	//     the maximum wheel scale must STILL PASS. A gate that refuses everything is ⛔ not a
	//     gate, and a test that only ever sees a refusal ⛔ cannot tell the two apart."*
	const float WheeledRadius = FixtureRadius * WheelMax;
	const int32 WheeledSamples = NumSamples(WheeledRadius);
	TestTrue(TEXT("(b) SELF-CHECK: the wheeled pad really takes the WIDE sample set, so 'the flat pad passed' is ⛔ not one trace passing on its own"),
		WheeledSamples > 1 && WheeledSamples == NumSamples(FixtureRadius));

	TArray<FVector> FlatPadNormals;
	FVector WidestFlatCorner = FVector::ZeroVector;
	for (int32 Index = 0; Index < WheeledSamples; ++Index)
	{
		FlatPadNormals.Add(FVector::UpVector);
		const FVector Offset = SampleOffset(Index, WheeledRadius);
		if (Offset.Size2D() > WidestFlatCorner.Size2D())
		{
			WidestFlatCorner = Offset;
		}
	}
	TestTrue(TEXT("(b) ⭐⭐ **A GENUINELY FLAT PAD AT THE MAXIMUM WHEEL SCALE IS STILL ADMITTED** — the gate did ⛔ not become a refusal machine"),
		AllSamplesAdmitted(FlatPadNormals, SlopeLimit));

	// (b2) ⭐ AND THE CONTROL IS ⛔ NOT VACUOUS: the probe really did reach further at ×max
	//      than at ×min. Without this, "the flat pad passed" would also be what a probe whose
	//      corners never left the centre reports.
	const FVector UnwheeledCorner = SampleOffset(1, FixtureRadius * WheelMin);
	const FVector WheeledCorner = SampleOffset(1, WheeledRadius);
	TestTrue(TEXT("(b2) ⭐ the wheel genuinely widened the probe — the corner sample sits FURTHER out at the maximum scale than at the minimum"),
		WheeledCorner.Size2D() > UnwheeledCorner.Size2D() && WidestFlatCorner.Size2D() > 0.0);

	// (c) ⭐⭐⭐ **THE CLAIM** (spec (5)): a centre-on-crown / corner-on-flank configuration
	//     that PASSES TODAY must FAIL AFTER. Both angles are DERIVED from the shipped limit —
	//     ⛔ neither is transcribed — so a retune of `MaxPlacementSlopeDegrees` keeps this row
	//     meaningful instead of quietly making it vacuous.
	const float CrownDegrees = SlopeLimit * 0.5f;
	const float FlankDegrees = FMath::Min(SlopeLimit * 2.f, (SlopeLimit + 90.f) * 0.5f);
	TestTrue(TEXT("(c) SELF-CHECK: the derived crown is genuinely BELOW the limit and the derived flank genuinely ABOVE it, so the pair discriminates"),
		CrownDegrees < SlopeLimit && FlankDegrees > SlopeLimit && FlankDegrees < 90.f);

	const FVector CrownNormal = NormalTiltedBy(CrownDegrees);
	const FVector FlankNormal = NormalTiltedBy(FlankDegrees);

	// (c1) ⭐⭐ **TODAY'S GATE ADMITS IT.** The single centre trace sees only the crown — which
	//      is exactly why the defect shipped: the building's far side was never measured.
	TArray<FVector> CentreOnly;
	CentreOnly.Add(CrownNormal);
	TestTrue(TEXT("(c1) ⭐⭐ the CENTRE ALONE admits this placement — this is the configuration that PASSES TODAY, and it is the defect"),
		AllSamplesAdmitted(CentreOnly, SlopeLimit));

	// (c2) ⭐⭐ **THE FOOTPRINT PROBE REFUSES IT.** Same centre, same crown, one corner hanging
	//      over the flank. ⇒ the thing that passed above fails here, which is spec (5)'s whole
	//      sentence expressed as an assertion.
	//      ⛔ The fixture is BUILT from the probe's own sample count rather than typed out, so
	//      it cannot silently desync from it the day the count changes.
	const int32 OverhangingCorner = WheeledSamples - 1;
	TArray<FVector> CrownWithOverhang;
	for (int32 Index = 0; Index < WheeledSamples; ++Index)
	{
		CrownWithOverhang.Add(Index == OverhangingCorner ? FlankNormal : CrownNormal);
	}
	TestTrue(TEXT("(c2) SELF-CHECK: the overhang really sits on a CORNER (⛔ not the centre) and the fixture carries one sample per probe point"),
		OverhangingCorner > 0 && CrownWithOverhang.Num() == WheeledSamples);
	TestFalse(TEXT("(c2) ⭐⭐⭐ **A CENTRE ON A CROWN WITH ONE CORNER ON A FLANK IS NOW REFUSED** — the overhang the wheel widened is closed"),
		AllSamplesAdmitted(CrownWithOverhang, SlopeLimit));

	// (c3) ⭐ ANY corner does it, ⛔ not just one privileged index — otherwise three sides of
	//      every building would still be unguarded and (c2) would be green anyway.
	for (int32 FlankIndex = 1; FlankIndex < WheeledSamples; ++FlankIndex)
	{
		TArray<FVector> OneFlank;
		for (int32 Index = 0; Index < WheeledSamples; ++Index)
		{
			OneFlank.Add(Index == FlankIndex ? FlankNormal : CrownNormal);
		}
		TestFalse(FString::Printf(TEXT("(c3) ⭐ a flank under corner %d alone is enough to refuse — every side of the footprint is guarded"), FlankIndex),
			AllSamplesAdmitted(OneFlank, SlopeLimit));
	}

	// (d) ⭐ THE BOUNDARY IS `<=`, ⛔ NOT `<` — the SHIPPED comparison, preserved.
	//     ⚠️ Asserted WITHOUT relying on an `acos(cos(θ)) == θ` round-trip landing on the exact
	//     side of the threshold: that would be a test whose colour depends on the last bit of a
	//     trig call. ⭐ Instead the DISCRIMINATOR is exact by construction — a perfectly FLAT
	//     surface against a limit of exactly zero is admitted under `<=` and refused under `<`,
	//     with ⛔ no trigonometry between the fixture and the answer.
	TestTrue(TEXT("(d) ⭐⭐ a FLAT surface against a limit of exactly zero is ADMITTED — the comparison is `<=`, ⛔ not `<` (exact, ⛔ no trig round-trip)"),
		WithinLimit(FVector::UpVector, 0.f));
	TestFalse(TEXT("(d) ⭐ …and any tilt at all against that same zero limit is refused, so the row above is ⛔ not a predicate that admits everything"),
		WithinLimit(NormalTiltedBy(1.f), 0.f));
	TestFalse(TEXT("(d) ⭐ a hair PAST the shipped limit is refused, so the shipped boundary genuinely bites"),
		WithinLimit(NormalTiltedBy(SlopeLimit + 1.f), SlopeLimit));
	TestTrue(TEXT("(d) ⭐ …and a hair UNDER it is admitted, so (d) is ⛔ not one comparison agreeing with itself"),
		WithinLimit(NormalTiltedBy(SlopeLimit - 1.f), SlopeLimit));

	// (d2) ⭐⭐ AND THE PREDICATE IS ⛔ EXACTLY "measured degrees <= the limit" — the seam and the
	//      gate cannot hold two different opinions about what "flat enough" means. Checked as an
	//      AGREEMENT across the whole probe range rather than at one point, so a predicate that
	//      quietly grew an extra term (a fudge factor, a second threshold) goes RED.
	for (const float Degrees : { 0.f, CrownDegrees, SlopeLimit, FlankDegrees, 89.f, 90.f, 179.f })
	{
		const FVector Probe = NormalTiltedBy(Degrees);
		TestTrue(FString::Printf(TEXT("(d2) ⭐ at %.2f deg the predicate agrees with the measured angle — it is `degrees <= limit` and ⛔ nothing else"), Degrees),
			WithinLimit(Probe, SlopeLimit) == (SlopeDegrees(Probe) <= SlopeLimit));
	}

	// (e) ⭐ THE ANGLE SEAM ROUND-TRIPS. Building the normal FROM an angle and reading the
	//      angle back OUT of it is what catches a radians/degrees slip or a sign error — both
	//      of which would leave every other row in this test green.
	for (const float Degrees : { 0.f, CrownDegrees, SlopeLimit, FlankDegrees, 89.f })
	{
		TestEqual(FString::Printf(TEXT("(e) ⭐ a normal built at %.2f deg reads back as %.2f deg — ⛔ no radians/degrees slip, ⛔ no sign error"), Degrees, Degrees),
			SlopeDegrees(NormalTiltedBy(Degrees)), Degrees, 1.e-2f);
	}

	// (f) ⛔ FAIL-CLOSED, THE SHIPPED DIRECTION, ON EVERY DEGENERATE INPUT. ⭐ Each row is
	//     paired with the FLAT normal being admitted under the same limit, so a "false" here
	//     is the guard firing and ⛔ not a predicate that stopped answering true at all.
	const float NaNValue = MakeQuietNaN();
	TestTrue(TEXT("(f) SELF-CHECK: the NaN fixture really is non-finite"), !FMath::IsFinite(NaNValue));
	TestTrue(TEXT("(f) ⭐ CONTROL — a flat normal IS admitted under this limit, so the refusals below mean something"),
		WithinLimit(FVector::UpVector, SlopeLimit));
	TestFalse(TEXT("(f) ⛔ a NaN surface normal is REFUSED (fail-closed), ⛔ never admitted"),
		WithinLimit(FVector(0.0, 0.0, NaNValue), SlopeLimit));
	TestFalse(TEXT("(f) ⛔ a straight-DOWN normal is refused"), WithinLimit(-FVector::UpVector, SlopeLimit));
	TestFalse(TEXT("(f) ⛔ a sideways (vertical wall) normal is refused"), WithinLimit(FVector(1.0, 0.0, 0.0), SlopeLimit));
	TestFalse(TEXT("(f) ⛔ a degenerate ZERO normal is refused — it reads 90 deg, not 0"),
		WithinLimit(FVector::ZeroVector, SlopeLimit));
	TestFalse(TEXT("(f) ⛔⛔ a NaN LIMIT refuses rather than silently DISABLING a shipped gate — a hand-edited .uasset must not switch this off"),
		WithinLimit(FVector::UpVector, NaNValue));
	TestEqual(TEXT("(f) ⛔ a non-finite normal reads acos's own range ceiling (180 deg — straight down), which every finite limit refuses"),
		SlopeDegrees(FVector(0.0, 0.0, NaNValue)), 180.f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  31. ⭐⭐ THE SHIPPED BODY REALLY HAS THAT SHAPE — it consumes the **ONE** footprint, it
//      re-derives **NOTHING**, the gate chain is **UNREORDERED**, and the trace sits
//      **DOWNSTREAM OF THE SIZING** (`TASK-914` (d); `STACK-§6`; `SC-§41`)
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegePlacementSlopeProbeConsumesTheOneFootprintAndTheChainIsUnreorderedTest,
	"Siegebound.Placement.TheSlopeProbeConsumesTheOneFootprintMeasurementAndTheGateChainIsUnreordered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegePlacementSlopeProbeConsumesTheOneFootprintAndTheChainIsUnreorderedTest::RunTest(const FString& Parameters)
{
	using namespace SiegePlacementUpgradeFixture;  // LoadProjectSource / CountOccurrencesInCode / ControllerSourcePath
	using namespace SiegeDiscardAllFixture;        // ExtractControllerFunctionBody / CodeLinesOnly / CheckPrecedes

	FString SourceText;
	if (!LoadProjectSource(*this, ControllerSourcePath, SourceText))
	{
		return false;
	}

	// ⛔ `ExtractControllerFunctionBody` terminates on the next `void ASiegePlayerController::`
	// and so cannot bound a `bool` definition. Rather than widen a shipped helper that four
	// other tests depend on, the slope gate's body is bounded by BRACE MATCHING here, and the
	// result is SELF-CHECKED below — ⛔ a stale or empty probe must report RED, never SAFE.
	FString SlopeBody;
	{
		const TCHAR* const SlopeSignature = TEXT("bool ASiegePlayerController::IsGroundSlopePlaceable(");
		const int32 Start = SourceText.Find(SlopeSignature, ESearchCase::CaseSensitive);
		const int32 Open = (Start == INDEX_NONE)
			? INDEX_NONE
			: SourceText.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (Open == INDEX_NONE)
		{
			AddError(TEXT("⛔ 'bool ASiegePlayerController::IsGroundSlopePlaceable(' was not found in SiegePlayerController.cpp — it was renamed, removed or given a different return type, and every shape claim below is unmakeable."));
			return false;
		}
		int32 Depth = 0;
		for (int32 At = Open; At < SourceText.Len(); ++At)
		{
			const TCHAR Character = SourceText[At];
			Depth += (Character == TEXT('{')) ? 1 : ((Character == TEXT('}')) ? -1 : 0);
			if (Depth == 0)
			{
				SlopeBody = SourceText.Mid(Start, At - Start + 1);
				break;
			}
		}
	}

	// SELF-CHECKS: the extracted body really IS the slope gate. Without these, every "exactly
	// one" and every "zero" below would report SAFE on an empty string.
	TestTrue(TEXT("SELF-CHECK: the brace matcher bounded a non-empty body"), !SlopeBody.IsEmpty());
	TestEqual(TEXT("SELF-CHECK: the extracted body really is the slope gate (its ONE trace call is there)"),
		CountOccurrencesInCode(SlopeBody, TEXT("LineTraceSingleByChannel(")), 1);
	TestTrue(TEXT("SELF-CHECK: …and it reads a surface normal, which is what a slope gate is for"),
		CountOccurrencesInCode(SlopeBody, TEXT("ImpactNormal")) > 0);
	TestEqual(TEXT("SELF-CHECK: …and the matcher stopped INSIDE this function — it did not swallow the next definition"),
		CountOccurrencesInCode(SlopeBody, TEXT("ASiegePlayerController::HasObstacleClearance")), 0);

	// (a) ⭐ THE GATE CONSUMES THE THREE SEAMS, EACH EXACTLY ONCE. One trace call serving all
	//     five samples is the loop working; a second would be a corner probed by hand.
	TestEqual(TEXT("(a) ⭐ the sample COUNT comes from the seam, exactly once"),
		CountOccurrencesInCode(SlopeBody, TEXT("NumPlacementSlopeSamples(")), 1);
	TestEqual(TEXT("(a) ⭐ the sample OFFSET comes from the seam, exactly once"),
		CountOccurrencesInCode(SlopeBody, TEXT("PlacementSlopeSampleOffset(")), 1);
	TestEqual(TEXT("(a) ⭐ the slope VERDICT comes from the seam, exactly once"),
		CountOccurrencesInCode(SlopeBody, TEXT("IsSurfaceNormalWithinSlopeLimit(")), 1);
	TestEqual(TEXT("(a) ⭐⭐ ONE trace call serves all five samples — a second would be a corner probed by hand, outside the seam"),
		CountOccurrencesInCode(SlopeBody, TEXT("LineTraceSingleByChannel(")), 1);

	// (b) ⛔⛔ THE GATE RE-DERIVES **NOTHING**. It takes the footprint the frame already
	//     measured; it does ⛔ not read the ghost's bounds again. ⭐ A second definition of
	//     "how wide is this building" is exactly how the ghost the player sees and the
	//     footprint the click is validated against drift apart. Each zero is paired with a
	//     SELF-CHECK proving the scanner finds that same token elsewhere in this file
	//     (`SC-§39`: an absent token and an unreadable probe report the same zero).
	for (const TCHAR* Forbidden : { TEXT("CalcBounds("), TEXT("TryGetPlacementFootprintRadius("), TEXT("GetStaticMeshComponent(") })
	{
		TestTrue(FString::Printf(TEXT("(b) SELF-CHECK: '%s' IS findable on code lines elsewhere in this file, so the zero below means separation"), Forbidden),
			CountOccurrencesInCode(SourceText, Forbidden) > 0);
		TestEqual(FString::Printf(TEXT("(b) ⛔⛔ the slope gate never re-derives the footprint via '%s' — it consumes the ONE measurement"), Forbidden),
			CountOccurrencesInCode(SlopeBody, Forbidden), 0);
	}

	// (c) ⛔ THE SLOPE ARITHMETIC LIVES IN THE SEAM AND ⛔ NOWHERE ELSE, so the gate and its
	//     test can never disagree about what "flat enough" means.
	TestTrue(TEXT("(c) SELF-CHECK: 'FMath::Acos(' IS findable on code lines in this file, so the zero below means separation"),
		CountOccurrencesInCode(SourceText, TEXT("FMath::Acos(")) > 0);
	TestEqual(TEXT("(c) ⛔ the gate body holds NO slope arithmetic of its own — the comparison has exactly one home"),
		CountOccurrencesInCode(SlopeBody, TEXT("FMath::Acos(")), 0);
	TestEqual(TEXT("(c) ⭐ and file-wide there is exactly ONE acos — a second would be a second definition of 'slope'"),
		CountOccurrencesInCode(SourceText, TEXT("FMath::Acos(")), 1);

	// (d) ⛔⛔ THE CALL SITE PASSES THE FOOTPRINT, AND THE OLD POINT-ONLY SHAPE IS GONE.
	//     ⚠️⚠️ `SC-§41` IN ITS EXACT SHAPE: `IsGroundSlopePlaceable(PlacementLocation` is a
	//     SUBSTRING of the new call, so a needle without the CLOSING PAREN would count the new
	//     call as if it were the old one and this row would be a LIE that reads green. ⭐ The
	//     `)` is the discriminator, and the pair below is the positive control that proves it:
	//     the same needle reads ZERO with the paren and ONE without it.
	FString GhostBody;
	if (!ExtractControllerFunctionBody(*this, SourceText, TEXT("void ASiegePlayerController::UpdatePlacementGhost()"), GhostBody))
	{
		return false;
	}
	TestTrue(TEXT("(d) SELF-CHECK: the ghost body really is UpdatePlacementGhost (its shipped GhostColor write is there)"),
		CountOccurrencesInCode(GhostBody, TEXT("SetVectorParameterValue(GhostColorParamName")) == 1);
	TestEqual(TEXT("(d) ⭐ the ONE call site passes the frame's footprint radius, exactly once"),
		CountOccurrencesInCode(GhostBody, TEXT("IsGroundSlopePlaceable(PlacementLocation, FootprintRadius)")), 1);
	TestEqual(TEXT("(d) ⛔⛔ the OLD point-only call shape is GONE file-wide — note the CLOSING PAREN, which is the whole discriminator (SC-§41)"),
		CountOccurrencesInCode(SourceText, TEXT("IsGroundSlopePlaceable(PlacementLocation)")), 0);
	TestEqual(TEXT("(d) ⭐⭐ POSITIVE CONTROL for the row above: the SAME needle WITHOUT the closing paren reads ONE — so the zero is separation, ⛔ not a blind probe"),
		CountOccurrencesInCode(SourceText, TEXT("IsGroundSlopePlaceable(PlacementLocation")), 1);

	// (e) ⭐⭐ **THE GATE CHAIN IS UNREORDERED** (`TASK-914` (d)). `first-failing-rule-wins` is
	//     a shipped property: every pre-existing rule is still evaluated in its shipped order
	//     and the `Units` gate is still LAST. ⛔ This task tightened a gate IN PLACE; it did
	//     ⛔ not move one, and a reordering would silently change WHICH message a refusal shows.
	const FString GhostCode = CodeLinesOnly(GhostBody);
	CheckPrecedes(*this, GhostCode, TEXT("EPlacementInvalidReason::Slope;"), TEXT("EPlacementInvalidReason::Obstacle;"));
	CheckPrecedes(*this, GhostCode, TEXT("EPlacementInvalidReason::Obstacle;"), TEXT("EPlacementInvalidReason::Clearance;"));
	CheckPrecedes(*this, GhostCode, TEXT("EPlacementInvalidReason::Clearance;"), TEXT("EPlacementInvalidReason::Units;"));

	// (f) ⭐⭐ **THE TWO ORDERINGS THIS TASK DEPENDS ON**, and the second is NEW. `TASK-815`
	//     bought the first with `CheckPrecedes` because no behavioural test can catch it; the
	//     slope probe now rides the same footprint, so it inherits the same obligation —
	//     written above the sizing, the trace would validate THIS frame's click against LAST
	//     frame's size: an intermittent refusal with a green suite.
	CheckPrecedes(*this, GhostCode, TEXT("SetActorScale3D("), TEXT("TryGetPlacementFootprintRadius("));
	CheckPrecedes(*this, GhostCode, TEXT("TryGetPlacementFootprintRadius("), TEXT("IsGroundSlopePlaceable("));

	// (g) ⛔ THE FENCE, RE-MEASURED AFTER THIS DIFF RATHER THAN PROMISED (`TASK-914` (d)):
	//     `max`-not-sum is untouched — it is `TASK-735`'s entire non-regression argument — and
	//     there is ⛔ NO second clamp on `PlacementFootprintScale` (`R-4c`: a second copy of
	//     the range is the `HIGH-§1` booby trap).
	TestEqual(TEXT("(g) ⛔ EffectiveBuildingClearance still composes as a MAX, exactly once"),
		CountOccurrencesInCode(SourceText, TEXT("FMath::Max(SafeBase, SafeRadius)")), 1);
	TestEqual(TEXT("(g) ⛔ …and never as a SUM"),
		CountOccurrencesInCode(SourceText, TEXT("SafeBase + SafeRadius")), 0);
	TestEqual(TEXT("(g) ⛔ the wheel's range still has exactly ONE clamp site — its definition plus its ONE caller (R-4c)"),
		CountOccurrencesInCode(SourceText, TEXT("StepPlacementFootprintScale(")), 2);
	TestEqual(TEXT("(g) ⛔ …and the ghost/spawn scale still comes from ONE expression with TWO consumers, undisturbed by this diff"),
		CountOccurrencesInCode(SourceText, TEXT("MakePlacementFootprintScale3D(")), 3);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
