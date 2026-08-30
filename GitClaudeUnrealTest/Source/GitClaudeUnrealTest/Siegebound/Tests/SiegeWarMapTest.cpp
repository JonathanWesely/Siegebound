// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Layout/Geometry.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Siegebound/CommanderNpc.h"
#include "Siegebound/ScatterConfig.h"
#include "Siegebound/SiegeAssistantConsoleWidget.h"
#include "Siegebound/WarMapWidget.h"
#include "UObject/NameTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the BATTLEFIELD WAR MAP (batch WAR ROOM, TASK-564 + TASK-581) ═══
 *  Subjects: TASK-560's `FSiegeWarMapProjection` + `UWarMapWidget`, TASK-559's
 *  `ACommanderNpc` reveal price, TASK-561's `AppendToInput` refusal contract, and
 *  TASK-581's `USiegeAssistantConsoleWidget::ComposeAppendedInput` whitespace rule.
 *  QA gate: TASK-565. Compile + suite run: TASK-566. Commit: TASK-570.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE AIRLOCK PROOF IS **NOT** IN THIS FILE, AND THAT IS THE POINT OF IT
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `WR-§6` asks for one assertion above all others: that the whole war-map feature spent
 *  ZERO prompt characters and that Zone A did not move from its named, dated baseline of
 *  **5658 chars / 5658 UTF-8 bytes (2026-08-05)**.
 *
 *  ⛔ **THIS FILE DOES NOT ASSERT THAT, ON PURPOSE, AND ⛔ IT DID NOT TOUCH THE FILE THAT
 *  DOES.** `Tests/SiegeAssistantZoneATest.cpp` already carries it — `ShippedZoneAChars = 5658`,
 *  asserted by `Siegebound.Assistant.ZoneA.MeasuredCharCount` (that file, symbol
 *  `FSiegeAssistantZoneAMeasuredCharCountTest`) in BOTH chars and UTF-8 bytes.
 *
 *  ⚖️ **A PRE-EXISTING, UNMODIFIED, STILL-GREEN ASSERTION IS STRICTLY STRONGER EVIDENCE THAN
 *  A NEW ONE THIS BATCH WROTE ABOUT ITSELF.** A second copy here would be a claim the batch
 *  authored in its own defence; the one that already exists is a claim the batch had to
 *  survive. ⇒ ⛔ NO SECOND ASSERTION WAS ADDED and ⛔ that file is byte-untouched. TASK-566
 *  item (3) runs it by name.
 *
 *  ⛔ **AND NO TOKEN FIGURE APPEARS ANYWHERE IN THIS FILE** (`AS-§12g`, `WR-§6`): every size
 *  claim in this batch is in CHARACTERS or BYTES. The shipped `zoneA_tok` has never been
 *  printed and TASK-552 still owes it.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHAT MAKES THIS FILE POSSIBLE — THE PURITY TASK-560 SHIPPED ON PURPOSE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `FSiegeWarMapProjection` is FOUR PURE STATICS: no `UWorld`, no `AActor`, no `UObject`, no
 *  allocation, no clock read, no RNG. Every input is a parameter and every output is a return
 *  value or an out-parameter. ⇒ The entire world→map projection chain runs in-process with NO
 *  PIE session, NO viewport and NO Slate application. ⚠️ If a test here ever starts needing a
 *  world, the purity `WR-§6` calls load-bearing has been broken — and that is a FINDING, not
 *  a reason to add a fixture.
 *
 *  The widget tests (16-20) additionally exploit a property that was verified at the source
 *  rather than assumed: `UWarMapWidget::OpenMap` / `CloseMap` / `ToggleMap` are SAFE WITH NO
 *  WORLD. Every world-dependent leg inside them null-guards `GetWorld()` and early-returns
 *  (`SetAllyRefreshTimerEnabled` WarMapWidget.cpp, `RefreshAllyDots` same file), `SetVisibility`
 *  no-ops without a realized Slate widget (`UWidget::SetVisibilityInternal` guards on
 *  `GetCachedWidget()`, and `BroadcastFieldValueChanged` guards on an empty
 *  `EnabledFieldNotifications`), and the `BlueprintImplementableEvent`s are the same no-op
 *  they are in the shipped "no `WBP_WarMap` yet" path this class was designed to survive.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — STATED SO NOBODY MISTAKES GREEN FOR DONE
 *  (`SC-§32`: a mechanism never observed to function is not known to function)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔ **THE GOLD.** `ASiegePlayerController::ServerRequestEnemyReveal_Implementation`, the
 *      authority branch, `ASiegePlayerState::SpendGold`, the NET-ZERO refusal when the balance
 *      is short, and `ClientReceiveEnemyReveal` — ALL need a world, a player state, a castle
 *      and an own-team `ACommanderNpc`. This file asserts only the PRICE on the CDO (test 21)
 *      and the widget's SINK behaviour (tests 17-20). ⇒ TASK-569's PIE rows (i) and (j).
 *    • ✅⭐ **THE WHITESPACE RULE — ⛔ NO LONGER A GAP. CLOSED BY TASK-581, AND THE HISTORY IS
 *      KEPT BECAUSE IT IS THE LESSON.** This bullet used to read: *"TASK-561's insert
 *      composition is INLINE inside `AppendToInput`, behind a Slate-realized + open + enabled
 *      console, and `InputBox` is `protected` with NO public getter — so the composed string is
 *      unreadable through the shipped public surface."* ⚖️ **All of that was TRUE, and TASK-564
 *      reported it rather than writing a replica test that would have asserted nothing** —
 *      `handoffs/TASK-564-programmer.md` §4. ⇒ **`WR-§6` ruling `W4-R1` then made the rule a
 *      NAMED PURE STATIC**, `USiegeAssistantConsoleWidget::ComposeAppendedInput`, and ⭐ **test 23
 *      below asserts it headlessly — no instance, no Slate, no world, no CDO.** ⛔ **Test 22 still
 *      owns the other half** (the refusals, and that the seam NEVER OPENS THE CONSOLE); the two
 *      are deliberately separate tests because they fail for entirely different reasons.
 *    • The painter. `NativePaint` needs an `FSlateWindowElementList`; every geometry it draws
 *      comes out of `BuildMarkerRects` / `FSiegeWarMapProjection`, which ARE covered — see
 *      test 15's note on why that is the assertion that matters.
 *    • The ally-dot sweep (`RefreshAllyDots`), the refresh timer, `CreateAndAddToViewport`,
 *      `NativeConstruct` / `NativeDestruct`, `NativeOnMouseButtonDown` itself, the proximity
 *      gate, the `IA_WarMap` binding and every cursor/posture concern — all need a world, a
 *      viewport or a pawn.
 *    • ⛔ **NOTHING HERE RUNS THE MODEL.** No inference, no capture, no `ReportFirstCapture`.
 *      🔒 That one-shot latch is untouched and unspent by this file.
 *    • ⛔ **THE WAVE-2 BOUNDS WORK (TASK-573 · 574 · 575 · 576) IS DELIBERATELY ABSENT.** By
 *      manager ruling: a unit test there would be asserting the engine's `GetActorBounds`, not
 *      our logic. TASK-569's PIE rows (n)(o)(p) are its acceptance instrument.
 *
 *  ⚖️ A full green run here means THE MAP'S GEOMETRY AND ITS OUTBOUND SURFACE ARE RIGHT. The
 *  batch's real gate for everything else is TASK-569's PIE matrix and Jonathan's playtest.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔ `SC-§13` / THE `TestEqual`-ON-FString TRAP — HANDLED STRUCTURALLY, NOT BY CARE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `TestEqual` on `FString` is **CASE-INSENSITIVE** in UE 5.8 (`AutomationTest.h`: the
 *  `FString` overload forwards to the `const TCHAR*` one, which compares with `Stricmp`), and
 *  a shipped, QA-passed test in this project once asserted nothing because of exactly that.
 *  ⇒ ⛔ **EVERY STRING CLAIM BELOW USES `TestEqualSensitive`.**
 *
 *  ⚠️ AND THE SAME TRAP HAS A SECOND MOUTH THAT IS EASY TO WALK INTO HERE: **`FName`
 *  comparison and `FName` hashing are ALSO case-insensitive**, so a `TSet<FName>` (or a
 *  `TSet<FString>`, whose `GetTypeHash` is `Strihash`) would silently accept a symbol whose
 *  case had drifted. ⇒ Test 15's reachability sweep therefore collects **`int32` INDICES**,
 *  which are exact, and test 14 pins each index to its symbol with `TestEqualSensitive`. The
 *  two together give a byte-exact claim about the whole reachable set with no case-insensitive
 *  container anywhere in the argument.
 *
 *  ⛔ `SC-§33` (the trailing-default law): **ZERO defaulted parameters are added by this file.**
 *  Every fixture helper below takes all of its parameters explicitly. The discharge grep is
 *  pasted in `handoffs/TASK-564-programmer.md` §6.
 *
 *  📌 M8 DECLARATION (`WR-§8`, verbatim rather than the last three batches' boilerplate): this
 *  file adds NO replicated property, NO new replicated class, NO new relevancy tier and NO RPC.
 *  It is a test file; it adds no shipped surface at all.
 */

namespace SiegeWarMapTestFixture
{
	// ─────────────────────────────────────────────────────────────────────────
	// Tolerances
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  ⚠️ THE FIXTURE NUMBERS BELOW ARE POWERS OF TWO ON PURPOSE, SO THESE TOLERANCES ARE A
	 *  BACKSTOP AND NOT A FUDGE FACTOR. `1024 / 512 / 1920 / 1080 / 48` and the UV fractions
	 *  chosen for the marker layout all land on exactly representable doubles through the
	 *  projection's arithmetic (one add, one multiply, one divide), so every asserted value
	 *  below is EXACT in IEEE-754. A tolerance is carried anyway because a future edit that
	 *  reorders the arithmetic should fail on being WRONG, not on being one ULP different.
	 */
	static constexpr double UvTolerance = 1.e-12;
	static constexpr double PxTolerance = 1.e-9;
	static constexpr double RatioTolerance = 1.e-9;

	// ─────────────────────────────────────────────────────────────────────────
	// Arenas
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  A SYNTHETIC arena with a clean 2:1 aspect. ⛔ It is NOT the shipped one and it is not
	 *  pretending to be: it exists so the expected UVs and pixel values are exact integers and
	 *  halves, which is what lets a failure name the WRONG NUMBER instead of a drift.
	 */
	static FVector2D CleanArenaHalfExtent()
	{
		return FVector2D(1024.0, 512.0);
	}

	/**
	 *  ⭐ THE SHIPPED EXTENT, READ FROM ITS SINGLE OWNER — ⛔ NEVER TRANSCRIBED.
	 *
	 *  `SC-§34`'s structural escape, applied to a TEST: a test that hard-codes `(26000, 12000)`
	 *  is a second copy of a number `USiegeScatterConfig` owns, and the day the arena is
	 *  resized the test fails for a reason that has nothing to do with the map. Reading the CDO
	 *  is also EXACTLY what `UWarMapWidget::ResolveArenaHalfExtent` falls back to, so test 5
	 *  is asserting the real fallback rather than a stand-in for it.
	 *
	 *  ⚠️ IT IS THE **CDO** AND NOT THE ASSET, AND THAT LIMIT IS STATED RATHER THAN GLOSSED:
	 *  `WR-§2` row 4 records that the saved `DA_BattlefieldScatter` OVERRIDES the C++ default
	 *  at runtime. Loading a `/Game/` asset is not something a headless unit test should do,
	 *  so what test 5 proves is that the CODE-DEFAULT lane is non-degenerate. The asset lane is
	 *  TASK-569's readback.
	 */
	static FVector2D ShippedCdoArenaHalfExtent()
	{
		return GetDefault<USiegeScatterConfig>()->ArenaHalfExtent;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Panels and markers
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  A root `FGeometry` of the given local size.
	 *
	 *  ⚠️ `FVector2f`, NOT `FVector2D`, at the Slate boundary — the same discipline
	 *  `WarMapWidget.cpp` records at every `FSlateDrawElement` call: UE 5.8's
	 *  `FDeprecateVector2DParameter` marks its double-precision constructors
	 *  `UE_SLATE_VECTOR_DEPRECATED_DEFAULT`, the reporting macro is off TODAY, and this module
	 *  builds warnings-as-errors — so the day it is switched on is the day a `FVector2D` here
	 *  becomes a build failure.
	 */
	static FGeometry MakePanel(const float WidthPx, const float HeightPx)
	{
		return FGeometry::MakeRoot(FVector2f(WidthPx, HeightPx), FSlateLayoutTransform());
	}

	/** One marker, fully specified. ⛔ No defaulted parameter (`SC-§33`). */
	static FSiegeWarMapMarker MakeMarker(const FName PlaceSymbol, const FVector2D& LocalCentre, const double HitHalfSizePx)
	{
		FSiegeWarMapMarker Marker;
		Marker.PlaceSymbol = PlaceSymbol;
		Marker.LocalCentre = LocalCentre;
		Marker.LocalHitHalfSize = FVector2D(HitHalfSizePx, HitHalfSizePx);
		return Marker;
	}

	/**
	 *  The seven canonical `PlaceVocabulary` symbols, in shipped vocabulary order.
	 *
	 *  ⚠️ **TRANSCRIBED, AND THE REASON IS THE OPPOSITE OF `ShippedCdoArenaHalfExtent`'s —
	 *  BOTH CHOICES ARE DELIBERATE, SO BOTH ARE EXPLAINED.** `PlaceVocabulary` is a
	 *  FILE-LOCAL `static constexpr` table inside `SiegeAssistantSnapshot.cpp` with no public
	 *  publisher: the only runtime route to the list is `GetPlaceNames()` on a snapshot that
	 *  has been `Capture()`d against a live world, which this file may not and will not do.
	 *  ⇒ A literal list is the only reachable option, and it is also the RIGHT one for a
	 *  vocabulary: reading the value under test from the code under test asserts nothing, and
	 *  an eighth place appearing in the shipped table SHOULD break a test that spells seven.
	 *  Established precedent in this suite: `SiegeAssistantGrammarTest.cpp` and
	 *  `SiegeAssistantSelectionTest.cpp` both transcribe place symbols the same way.
	 *
	 *  ⛔ THIS IS NOT THE MAP ADDING A SYMBOL. `UWarMapWidget` contains no place literal at
	 *  all — it READS `GetPlaceNames()`. The list here is the TEST's expectation of what the
	 *  map should be able to hand back, never a source the map consults.
	 */
	static TArray<FName> CanonicalPlaceSymbols()
	{
		return TArray<FName>{
			TEXT("own_castle"),
			TEXT("enemy_castle"),
			TEXT("mid"),
			TEXT("ancient_ground_near"),
			TEXT("ancient_ground_far"),
			TEXT("nearest_mine"),
			TEXT("hero")
		};
	}

	/**
	 *  Seven well-separated UV positions, one per canonical symbol.
	 *
	 *  ⚠️ THE SEPARATION IS A PRECONDITION OF TESTS 14 AND 15, SO THOSE TESTS ASSERT IT
	 *  RATHER THAN ASSUME IT: overlapping rects would silently engage the last-match-wins rule
	 *  (test 12) and turn a reachability failure into a pass.
	 */
	static TArray<FVector2D> MarkerUvLayout()
	{
		return TArray<FVector2D>{
			FVector2D(0.15, 0.20),
			FVector2D(0.85, 0.20),
			FVector2D(0.50, 0.50),
			FVector2D(0.30, 0.75),
			FVector2D(0.70, 0.75),
			FVector2D(0.20, 0.45),
			FVector2D(0.60, 0.35)
		};
	}

	/** The shipped `MarkerHitHalfSizePx` default. ⚠️ A UI affordance in PIXELS — ⛔ never a world radius (`WR-§6`, `AS-§21.4`). */
	static constexpr double ShippedMarkerHitHalfSizePx = 18.0;

	/** The shipped `MapPaddingPx` default. */
	static constexpr double ShippedMapPaddingPx = 48.0;

	/**
	 *  Builds the seven canonical markers projected into a real map rect, exactly the way
	 *  `UWarMapWidget::BuildMarkerRects` composes them: `MapUVToLocal` over a rect from
	 *  `ComputeMapRectLocal`, with a uniform pixel hit box.
	 */
	static TArray<FSiegeWarMapMarker> BuildCanonicalMarkers(const FVector2D& RectOrigin, const FVector2D& RectSize)
	{
		const TArray<FName> Symbols = CanonicalPlaceSymbols();
		const TArray<FVector2D> Uvs = MarkerUvLayout();

		TArray<FSiegeWarMapMarker> Markers;
		Markers.Reserve(Symbols.Num());

		for (int32 Index = 0; Index < Symbols.Num(); ++Index)
		{
			Markers.Add(MakeMarker(
				Symbols[Index],
				FSiegeWarMapProjection::MapUVToLocal(Uvs[Index], RectOrigin, RectSize),
				ShippedMarkerHitHalfSizePx));
		}

		return Markers;
	}

	/** A throwaway world-space dot list of the requested length. Values are irrelevant — only the COUNT is ever asserted. */
	static TArray<FVector2D> MakeDotList(const int32 Count)
	{
		TArray<FVector2D> Dots;
		Dots.Reserve(Count);

		for (int32 Index = 0; Index < Count; ++Index)
		{
			Dots.Emplace(static_cast<double>(Index) * 100.0, static_cast<double>(Index) * -50.0);
		}

		return Dots;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. PROJECTION — the centre and the four corners
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapProjectionCentreAndCornersTest,
	"Siegebound.WarMap.ProjectionCentreAndCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapProjectionCentreAndCornersTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D Arena = CleanArenaHalfExtent();

	// ── The centre of the world is the centre of the map ──────────────────────
	const FVector2D Centre = FSiegeWarMapProjection::WorldToMapUV(FVector2D::ZeroVector, Arena);
	TestEqual(TEXT("World (0,0) maps to the map's horizontal centre"), Centre.X, 0.5, UvTolerance);
	TestEqual(TEXT("World (0,0) maps to the map's vertical centre"), Centre.Y, 0.5, UvTolerance);

	// ── The four ArenaHalfExtent corners map to the four UV corners ───────────
	// ⭐ THE Y ROW IS THE ONE THAT MATTERS, AND IT WAS RE-PINNED BY TASK-692 (`WM-§7`):
	// UE's world frame is LEFT-HANDED (X forward, Y right, Z up), so on a top-down map
	// drawing +X to the RIGHT, world +Y lies toward the map's BOTTOM — the direction
	// Slate's local Y already grows. The pre-692 convention inverted Y here and rendered
	// the whole battlefield MIRRORED about the castle lane — every dot on the wrong side
	// while the map still looked perfectly plausible (Jonathan's first map test caught
	// it). That mirror is the defect this block now kills.
	struct FCornerCase
	{
		const TCHAR* Label;
		FVector2D World;
		double ExpectedU;
		double ExpectedV;
	};

	const FCornerCase Corners[] =
	{
		{ TEXT("(-HalfX, -HalfY) is the TOP-LEFT of the map"),     FVector2D(-Arena.X, -Arena.Y), 0.0, 0.0 },
		{ TEXT("(+HalfX, -HalfY) is the TOP-RIGHT of the map"),    FVector2D( Arena.X, -Arena.Y), 1.0, 0.0 },
		{ TEXT("(-HalfX, +HalfY) is the BOTTOM-LEFT of the map"),  FVector2D(-Arena.X,  Arena.Y), 0.0, 1.0 },
		{ TEXT("(+HalfX, +HalfY) is the BOTTOM-RIGHT of the map"), FVector2D( Arena.X,  Arena.Y), 1.0, 1.0 }
	};

	for (const FCornerCase& Case : Corners)
	{
		const FVector2D Uv = FSiegeWarMapProjection::WorldToMapUV(Case.World, Arena);

		TestEqual(*FString::Printf(TEXT("%s — U"), Case.Label), Uv.X, Case.ExpectedU, UvTolerance);
		TestEqual(*FString::Printf(TEXT("%s — V"), Case.Label), Uv.Y, Case.ExpectedV, UvTolerance);
	}

	// ── The same shape holds against the SHIPPED extent, not just the clean one ─
	// ⚠️ Run separately because the clean arena is 2:1 and the shipped one is not; a
	// projection that accidentally depended on a square-ish aspect would pass above and fail
	// here.
	const FVector2D Shipped = ShippedCdoArenaHalfExtent();
	const FVector2D ShippedCentre = FSiegeWarMapProjection::WorldToMapUV(FVector2D::ZeroVector, Shipped);
	TestEqual(TEXT("Against the SHIPPED CDO extent, world (0,0) is still the map centre — U"), ShippedCentre.X, 0.5, UvTolerance);
	TestEqual(TEXT("Against the SHIPPED CDO extent, world (0,0) is still the map centre — V"), ShippedCentre.Y, 0.5, UvTolerance);

	const FVector2D ShippedTopLeft = FSiegeWarMapProjection::WorldToMapUV(FVector2D(-Shipped.X, -Shipped.Y), Shipped);
	TestEqual(TEXT("Against the SHIPPED CDO extent, (-HalfX, -HalfY) is the top-left — U"), ShippedTopLeft.X, 0.0, UvTolerance);
	TestEqual(TEXT("Against the SHIPPED CDO extent, (-HalfX, -HalfY) is the top-left — V"), ShippedTopLeft.Y, 0.0, UvTolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. PROJECTION — the orientation is PINNED, in both axes and in both directions
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapProjectionOrientationTest,
	"Siegebound.WarMap.ProjectionOrientationIsPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapProjectionOrientationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D Arena = CleanArenaHalfExtent();

	// ⭐ MONOTONICITY, NOT A SPOT CHECK. Test 1 pins five points; a sign error at exactly one
	// of them is conceivable, a sign error across a whole sweep is not. This is the assertion
	// a "helpful" future edit that RE-INTRODUCES the pre-692 mirrored Y has to get past.
	constexpr int32 SampleCount = 33;

	double PreviousU = -1.0;
	double PreviousV = -1.0;
	bool bUStrictlyIncreases = true;
	bool bVStrictlyIncreases = true;

	for (int32 Sample = 0; Sample < SampleCount; ++Sample)
	{
		// Interior fractions of the arena, walking from -Half to +Half on BOTH axes at once.
		const double Fraction = -1.0 + 2.0 * (static_cast<double>(Sample) / static_cast<double>(SampleCount - 1));
		const FVector2D Uv = FSiegeWarMapProjection::WorldToMapUV(
			FVector2D(Arena.X * Fraction, Arena.Y * Fraction), Arena);

		if (Sample > 0)
		{
			bUStrictlyIncreases = bUStrictlyIncreases && (Uv.X > PreviousU);
			bVStrictlyIncreases = bVStrictlyIncreases && (Uv.Y > PreviousV);
		}

		PreviousU = Uv.X;
		PreviousV = Uv.Y;
	}

	TestTrue(TEXT("World +X grows RIGHT: UV.X strictly INCREASES across a 33-sample sweep of the arena"), bUStrictlyIncreases);

	// ⭐⭐ THE TASK-692 CONVENTION (`WM-§7`). If this line ever goes red, someone has put the
	// pre-692 Y inversion back and the map is drawing the battlefield mirrored about the
	// castle lane again — every dot on the wrong side while looking perfectly plausible.
	TestTrue(TEXT("⭐ World +Y grows DOWN: UV.Y strictly INCREASES across the same sweep (UE's left-handed frame puts +Y 90° clockwise from +X seen from above — the same way Slate's local Y grows; a projection that inverts it renders mirrored, TASK-692/WM-§7)"), bVStrictlyIncreases);

	// The two axes are independent: moving on X alone must not move V, and vice versa.
	const FVector2D XOnly = FSiegeWarMapProjection::WorldToMapUV(FVector2D(Arena.X * 0.5, 0.0), Arena);
	TestEqual(TEXT("Moving on world X alone leaves UV.V at the centre — the axes do not bleed"), XOnly.Y, 0.5, UvTolerance);
	TestEqual(TEXT("…and moves UV.U to three quarters"), XOnly.X, 0.75, UvTolerance);

	const FVector2D YOnly = FSiegeWarMapProjection::WorldToMapUV(FVector2D(0.0, Arena.Y * 0.5), Arena);
	TestEqual(TEXT("Moving on world Y alone leaves UV.U at the centre"), YOnly.X, 0.5, UvTolerance);
	TestEqual(TEXT("…and moves UV.V DOWNWARD to three quarters (0.75), ⛔ not up to 0.25"), YOnly.Y, 0.75, UvTolerance);

	// ─────────────────────────────────────────────────────────────────────────
	// ⭐⭐ TASK-692 (`WM-§7`): THE ABSOLUTE-SIDE PIN — the case that would have CAUGHT the
	// mirror. Every assertion above (and the round trip in test 25) stays green under ANY
	// self-consistent sign convention; only tying REAL actors to an ABSOLUTE map side can
	// fail on a coherent mirror. The actors and world XY are the TASK-692 truth table's:
	// castles from source (`BattlefieldScatter.cpp:2632` — Blue −25000, Red +25000, Y 0;
	// ⚠️ transcribed DELIBERATELY, the CanonicalPlaceSymbols argument: an expectation read
	// from the code under test asserts nothing), the mine from the recorded live MinesPass
	// line (seed 704140289, pair[0] primary P=(−17358, 6103)). The frame: the player exits
	// the Blue gate facing battlefield centre = world +X; at identity yaw UE's right vector
	// IS +Y, so his RIGHT = world +Y — and a top-down map drawing +X to the RIGHT must
	// draw +Y toward the map's BOTTOM. Half-plane claims (not exact UVs) so an arena
	// RESIZE cannot break them — only a re-mirrored axis can.
	// ─────────────────────────────────────────────────────────────────────────
	const FVector2D Shipped = ShippedCdoArenaHalfExtent();

	const FVector2D BlueCastleUv = FSiegeWarMapProjection::WorldToMapUV(FVector2D(-25000.0, 0.0), Shipped);
	TestTrue(TEXT("⭐ TASK-692: the BLUE castle (world −X) lands on the map's WEST/LEFT half — U < 0.5"), BlueCastleUv.X < 0.5);

	const FVector2D RedCastleUv = FSiegeWarMapProjection::WorldToMapUV(FVector2D(25000.0, 0.0), Shipped);
	TestTrue(TEXT("⭐ TASK-692: the RED castle (world +X) lands on the map's EAST/RIGHT half — U > 0.5"), RedCastleUv.X > 0.5);

	const FVector2D RecordedMineUv = FSiegeWarMapProjection::WorldToMapUV(FVector2D(-17358.0, 6103.0), Shipped);
	TestTrue(TEXT("⭐⭐ TASK-692: a world +Y mine — on the RIGHT of a +X-facing player at the Blue gate — lands on the map's BOTTOM half, V > 0.5 (the pre-692 transform drew it at V≈0.246, the top — the exact left↔right mirror Jonathan reported)"), RecordedMineUv.Y > 0.5);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. PROJECTION — out of bounds CLAMPS; it does not drop and it does not overflow
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapProjectionClampTest,
	"Siegebound.WarMap.ProjectionClampsOutOfBoundsInsteadOfDropping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapProjectionClampTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D Arena = CleanArenaHalfExtent();

	// ⚠️ CLAMPED, NOT DROPPED, IS A DESIGN DECISION AND THIS TEST IS WHERE IT IS RECORDED AS
	// ONE: a unit that has wandered past the configured arena bound PINS TO THE MAP EDGE. A
	// vanished ally is a lie the player cannot see; an edge-pinned one is visibly at the edge.
	struct FClampCase
	{
		const TCHAR* Label;
		FVector2D World;
		double ExpectedU;
		double ExpectedV;
	};

	const FClampCase Cases[] =
	{
		{ TEXT("far past -X and -Y pins to the top-left corner"),                   FVector2D(-Arena.X * 40.0, -Arena.Y * 40.0), 0.0, 0.0 },
		{ TEXT("far past +X and +Y pins to the bottom-right corner"),               FVector2D( Arena.X * 40.0,  Arena.Y * 40.0), 1.0, 1.0 },
		{ TEXT("far past +X only pins U while leaving V centred"),                  FVector2D( Arena.X * 40.0,  0.0),            1.0, 0.5 },
		{ TEXT("far past -Y only pins V to the TOP edge while leaving U centred"),  FVector2D( 0.0,            -Arena.Y * 40.0), 0.5, 0.0 }
	};

	for (const FClampCase& Case : Cases)
	{
		const FVector2D Uv = FSiegeWarMapProjection::WorldToMapUV(Case.World, Arena);

		TestEqual(*FString::Printf(TEXT("%s — U"), Case.Label), Uv.X, Case.ExpectedU, UvTolerance);
		TestEqual(*FString::Printf(TEXT("%s — V"), Case.Label), Uv.Y, Case.ExpectedV, UvTolerance);
	}

	// Nothing, at any distance, escapes [0,1]. A UV outside the unit square would draw a dot
	// outside the map panel — over the reveal button, or off screen entirely.
	bool bAllInsideUnitSquare = true;

	for (int32 Decade = 0; Decade <= 8; ++Decade)
	{
		const double Magnitude = FMath::Pow(10.0, static_cast<double>(Decade));
		const FVector2D Positive = FSiegeWarMapProjection::WorldToMapUV(FVector2D(Magnitude, Magnitude), Arena);
		const FVector2D Negative = FSiegeWarMapProjection::WorldToMapUV(FVector2D(-Magnitude, -Magnitude), Arena);

		bAllInsideUnitSquare = bAllInsideUnitSquare
			&& Positive.X >= 0.0 && Positive.X <= 1.0 && Positive.Y >= 0.0 && Positive.Y <= 1.0
			&& Negative.X >= 0.0 && Negative.X <= 1.0 && Negative.Y >= 0.0 && Negative.Y <= 1.0;
	}

	TestTrue(TEXT("Every UV stays inside [0,1]² across nine decades of world distance in both directions"), bAllInsideUnitSquare);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. PROJECTION — a degenerate arena does NOT divide by zero
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapProjectionDegenerateArenaTest,
	"Siegebound.WarMap.ProjectionSurvivesADegenerateArenaExtent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapProjectionDegenerateArenaTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⚠️ THIS IS NOT A HYPOTHETICAL INPUT. `ArenaHalfExtent` is an editable field on a
	// DataAsset a designer can clear to (0,0), and an unguarded divide would put NaN into
	// every marker rect — which then propagates into the hit test and makes EVERY CLICK MISS
	// while the map still paints. `WR-§6` requires "never a silent zero-divide"; this is the
	// assertion that the floor is applied BEFORE the division rather than after it.
	const FVector2D Zero = FSiegeWarMapProjection::WorldToMapUV(FVector2D::ZeroVector, FVector2D::ZeroVector);

	TestTrue(TEXT("A (0,0) arena extent yields a FINITE U — ⛔ no NaN, no infinity"), FMath::IsFinite(Zero.X));
	TestTrue(TEXT("A (0,0) arena extent yields a FINITE V — ⛔ no NaN, no infinity"), FMath::IsFinite(Zero.Y));
	TestEqual(TEXT("…and world (0,0) still lands at the map centre — U"), Zero.X, 0.5, UvTolerance);
	TestEqual(TEXT("…and world (0,0) still lands at the map centre — V"), Zero.Y, 0.5, UvTolerance);

	// A NEGATIVE extent is the same defect wearing a different hat, and the shipped floor
	// catches both with one FMath::Max. Asserted as EQUALITY with the zero case rather than
	// re-derived, so the two paths cannot drift apart.
	const FVector2D Negative = FSiegeWarMapProjection::WorldToMapUV(FVector2D::ZeroVector, FVector2D(-500.0, -500.0));
	TestEqual(TEXT("A NEGATIVE arena extent behaves identically to a zero one — U"), Negative.X, Zero.X, UvTolerance);
	TestEqual(TEXT("A NEGATIVE arena extent behaves identically to a zero one — V"), Negative.Y, Zero.Y, UvTolerance);

	// Off-centre against a degenerate arena: everything beyond the 1 uu floor pins to an edge.
	// Still finite, still inside the unit square, still nothing to divide by zero.
	const FVector2D OffCentre = FSiegeWarMapProjection::WorldToMapUV(FVector2D(2.0, -2.0), FVector2D::ZeroVector);
	TestTrue(TEXT("An off-centre point on a degenerate arena is finite in U"), FMath::IsFinite(OffCentre.X));
	TestTrue(TEXT("An off-centre point on a degenerate arena is finite in V"), FMath::IsFinite(OffCentre.Y));
	TestEqual(TEXT("…and pins to the right edge"), OffCentre.X, 1.0, UvTolerance);
	TestEqual(TEXT("…and pins to the top edge (world -Y is the map's top, TASK-692)"), OffCentre.Y, 0.0, UvTolerance);

	// `ComputeMapRectLocal` divides by the same extent and carries the same floor. It must not
	// be the one that got missed.
	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(FVector2D(1920.0, 1080.0), 0.f, FVector2D::ZeroVector, RectOrigin, RectSize);

	TestTrue(TEXT("ComputeMapRectLocal also survives a (0,0) extent — the rect width is finite"), FMath::IsFinite(RectSize.X));
	TestTrue(TEXT("ComputeMapRectLocal also survives a (0,0) extent — the rect height is finite"), FMath::IsFinite(RectSize.Y));
	TestTrue(TEXT("…and the rect is still positive, so the map still draws on a degenerate arena"), RectSize.X > 0.0 && RectSize.Y > 0.0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. THE SHIPPED FALLBACK EXTENT IS NEVER DEGENERATE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapShippedArenaFallbackTest,
	"Siegebound.WarMap.ShippedArenaFallbackIsNeverDegenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapShippedArenaFallbackTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐ THE PROPERTY, ⛔ NOT THE NUMBER. `UWarMapWidget::ResolveArenaHalfExtent` falls back to
	// `GetDefault<USiegeScatterConfig>()->ArenaHalfExtent` when `DA_BattlefieldScatter` does
	// not resolve — a DELIBERATE upgrade over the spec's "named fallback constant" (TASK-560's
	// declared `SC-§15` departure, on `SC-§34` grounds). What must be true of that fallback is
	// that it is USABLE, and that is what is asserted here.
	//
	// ⛔ THIS TEST DELIBERATELY DOES NOT ASSERT `(26000, 12000)`. Pinning the number would
	// re-create the exact stale-derived-constant defect `SC-§34` exists to prevent: the day
	// Jonathan resizes the arena, a test about the WAR MAP would fail for a reason that has
	// nothing to do with the war map. The number's owner is `USiegeScatterConfig`; its value
	// is that class's business and TASK-569 reads the ASSET lane back.
	const FVector2D Fallback = ShippedCdoArenaHalfExtent();

	TestTrue(TEXT("The shipped CDO arena half-extent is strictly positive on X"), Fallback.X > 0.0);
	TestTrue(TEXT("The shipped CDO arena half-extent is strictly positive on Y"), Fallback.Y > 0.0);

	TestTrue(TEXT("⭐ The shipped fallback is strictly ABOVE MinArenaHalfExtentUu on X — so the war map's fallback path can never itself trip the zero-divide floor"),
		Fallback.X > static_cast<double>(FSiegeWarMapProjection::MinArenaHalfExtentUu));
	TestTrue(TEXT("⭐ The shipped fallback is strictly ABOVE MinArenaHalfExtentUu on Y — same reason"),
		Fallback.Y > static_cast<double>(FSiegeWarMapProjection::MinArenaHalfExtentUu));

	// The floor itself is copied from `ABattlefieldScatter` rather than invented, and it must
	// stay a positive number for the FMath::Max to mean anything.
	TestTrue(TEXT("MinArenaHalfExtentUu is strictly positive (it is the divisor's floor)"),
		FSiegeWarMapProjection::MinArenaHalfExtentUu > 0.f);

	// And the fallback actually produces a usable map, end to end.
	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(
		FVector2D(1920.0, 1080.0), static_cast<float>(ShippedMapPaddingPx), Fallback, RectOrigin, RectSize);

	TestTrue(TEXT("A 1920x1080 panel at the shipped padding yields a POSITIVE map rect on the fallback extent — the map draws and hit-tests even with no DataAsset"),
		RectSize.X > 0.0 && RectSize.Y > 0.0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. MAP RECT — the arena aspect is preserved (the map does not lie about distance)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapRectAspectTest,
	"Siegebound.WarMap.MapRectPreservesTheArenaAspect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapRectAspectTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D Arena = CleanArenaHalfExtent();
	const double ArenaAspect = Arena.X / Arena.Y;

	// ⚠️ ASPECT IS A CORRECTNESS PROPERTY, NOT A COSMETIC ONE. Stretching UV across a raw
	// panel would make the map LIE about relative distance: two pairs of dots equally far
	// apart in the world would read as different distances depending on which way they were
	// separated — and a player reads distance off a battlefield map to decide where to send an
	// army.
	struct FPanelCase
	{
		const TCHAR* Label;
		double PanelW;
		double PanelH;
		double Padding;
	};

	const FPanelCase Panels[] =
	{
		{ TEXT("a 1920x1080 panel at the shipped padding (panel narrower than the arena ⇒ WIDTH-bound)"), 1920.0, 1080.0, ShippedMapPaddingPx },
		{ TEXT("a very wide 2000x400 panel (panel wider than the arena ⇒ HEIGHT-bound)"),                 2000.0,  400.0,   0.0 },
		{ TEXT("a tall 800x1200 panel (WIDTH-bound)"),                                                     800.0, 1200.0,   0.0 },
		{ TEXT("a panel whose inner box matches the arena aspect EXACTLY"),                               1000.0,  500.0,   0.0 }
	};

	for (const FPanelCase& Case : Panels)
	{
		FVector2D RectOrigin = FVector2D::ZeroVector;
		FVector2D RectSize = FVector2D::ZeroVector;
		FSiegeWarMapProjection::ComputeMapRectLocal(
			FVector2D(Case.PanelW, Case.PanelH), static_cast<float>(Case.Padding), Arena, RectOrigin, RectSize);

		TestTrue(*FString::Printf(TEXT("[%s] the rect has a positive size"), Case.Label), RectSize.X > 0.0 && RectSize.Y > 0.0);

		if (RectSize.Y > 0.0)
		{
			TestEqual(*FString::Printf(TEXT("[%s] the rect's aspect equals the ARENA's aspect"), Case.Label),
				RectSize.X / RectSize.Y, ArenaAspect, RatioTolerance);
		}

		// Letterboxed INSIDE the padded inner box — never spilling over the WBP's border.
		const double InnerW = Case.PanelW - 2.0 * Case.Padding;
		const double InnerH = Case.PanelH - 2.0 * Case.Padding;

		TestTrue(*FString::Printf(TEXT("[%s] the rect fits inside the padded inner box on X"), Case.Label), RectSize.X <= InnerW + PxTolerance);
		TestTrue(*FString::Printf(TEXT("[%s] the rect fits inside the padded inner box on Y"), Case.Label), RectSize.Y <= InnerH + PxTolerance);

		// It is the LARGEST such rect: at least one axis is fully used.
		const bool bTouchesAnAxis =
			FMath::IsNearlyEqual(RectSize.X, InnerW, PxTolerance) || FMath::IsNearlyEqual(RectSize.Y, InnerH, PxTolerance);
		TestTrue(*FString::Printf(TEXT("[%s] it is the LARGEST fitting rect — one axis is fully used (otherwise the map is needlessly small)"), Case.Label), bTouchesAnAxis);
	}

	// The exact-fit panel is asserted numerically as well, because "fills it exactly" is the
	// one case where an off-by-a-padding error would still satisfy every ratio above.
	FVector2D ExactOrigin = FVector2D::ZeroVector;
	FVector2D ExactSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(FVector2D(1000.0, 500.0), 0.f, Arena, ExactOrigin, ExactSize);

	TestEqual(TEXT("An exact-aspect panel with no padding is filled edge to edge — width"), ExactSize.X, 1000.0, PxTolerance);
	TestEqual(TEXT("An exact-aspect panel with no padding is filled edge to edge — height"), ExactSize.Y, 500.0, PxTolerance);
	TestEqual(TEXT("…with the rect origin at the panel origin — X"), ExactOrigin.X, 0.0, PxTolerance);
	TestEqual(TEXT("…with the rect origin at the panel origin — Y"), ExactOrigin.Y, 0.0, PxTolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. MAP RECT — it is CENTRED inside the padding
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapRectCentringTest,
	"Siegebound.WarMap.MapRectIsCentredInsideThePadding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapRectCentringTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D Arena = CleanArenaHalfExtent();
	const FVector2D Panel(1920.0, 1080.0);

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(Panel, static_cast<float>(ShippedMapPaddingPx), Arena, RectOrigin, RectSize);

	// ⭐ ASSERTED AS "THE MARGINS MATCH", ⛔ NOT BY RE-DERIVING THE FORMULA. A test that
	// recomputes `Padding + (Inner - Size) * 0.5` is a second copy of the implementation and
	// would agree with it even when both are wrong. Equal margins is the OBSERVABLE property.
	const double LeftMargin = RectOrigin.X;
	const double RightMargin = Panel.X - (RectOrigin.X + RectSize.X);
	const double TopMargin = RectOrigin.Y;
	const double BottomMargin = Panel.Y - (RectOrigin.Y + RectSize.Y);

	TestEqual(TEXT("The map rect is horizontally centred — the left and right margins are equal"), LeftMargin, RightMargin, PxTolerance);
	TestEqual(TEXT("The map rect is vertically centred — the top and bottom margins are equal"), TopMargin, BottomMargin, PxTolerance);

	TestTrue(TEXT("Every margin is at least the configured padding, so an edge marker is never clipped by the WBP's border"),
		LeftMargin >= ShippedMapPaddingPx - PxTolerance
		&& RightMargin >= ShippedMapPaddingPx - PxTolerance
		&& TopMargin >= ShippedMapPaddingPx - PxTolerance
		&& BottomMargin >= ShippedMapPaddingPx - PxTolerance);

	// ── A NEGATIVE padding is treated as ZERO, not as a negative inset ────────
	// Asserted as EQUALITY against the zero-padding result rather than against re-derived
	// numbers, so the clamp cannot be "right" in a way that quietly differs.
	FVector2D ZeroPadOrigin = FVector2D::ZeroVector;
	FVector2D ZeroPadSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(Panel, 0.f, Arena, ZeroPadOrigin, ZeroPadSize);

	FVector2D NegPadOrigin = FVector2D::ZeroVector;
	FVector2D NegPadSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(Panel, -250.f, Arena, NegPadOrigin, NegPadSize);

	TestEqual(TEXT("A NEGATIVE padding behaves exactly like zero padding — origin X"), NegPadOrigin.X, ZeroPadOrigin.X, PxTolerance);
	TestEqual(TEXT("A NEGATIVE padding behaves exactly like zero padding — origin Y"), NegPadOrigin.Y, ZeroPadOrigin.Y, PxTolerance);
	TestEqual(TEXT("A NEGATIVE padding behaves exactly like zero padding — size X"), NegPadSize.X, ZeroPadSize.X, PxTolerance);
	TestEqual(TEXT("A NEGATIVE padding behaves exactly like zero padding — size Y"), NegPadSize.Y, ZeroPadSize.Y, PxTolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. MAP RECT — degenerate panels FAIL CLOSED and are ⛔ NEVER negative
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapRectFailsClosedTest,
	"Siegebound.WarMap.MapRectFailsClosedAndIsNeverNegative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapRectFailsClosedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D Arena = CleanArenaHalfExtent();

	// ⛔⛔ THE FAILURE THIS KILLS IS SPECIFIC AND NASTY: a NEGATIVE rect size would INVERT every
	// marker's hit rect, and `FindMarkerIndexAtLocal` would then answer for points nowhere near
	// a marker — a click on empty map handing back a place symbol. Fail-closed means the map
	// draws nothing and hit-tests nothing, which is visibly broken rather than subtly wrong.
	struct FDegenerateCase
	{
		const TCHAR* Label;
		FVector2D Panel;
		double Padding;
	};

	const FDegenerateCase Cases[] =
	{
		{ TEXT("a panel smaller than twice the padding"), FVector2D(50.0, 50.0),   ShippedMapPaddingPx },
		{ TEXT("a zero-size panel"),                      FVector2D(0.0, 0.0),     0.0 },
		{ TEXT("a panel exactly twice the padding"),      FVector2D(96.0, 96.0),   ShippedMapPaddingPx },
		{ TEXT("a zero-height panel"),                    FVector2D(1920.0, 0.0),  0.0 },
		{ TEXT("a zero-width panel"),                     FVector2D(0.0, 1080.0),  0.0 }
	};

	for (const FDegenerateCase& Case : Cases)
	{
		FVector2D RectOrigin = FVector2D(-1.0, -1.0);
		FVector2D RectSize = FVector2D(-1.0, -1.0);
		FSiegeWarMapProjection::ComputeMapRectLocal(
			Case.Panel, static_cast<float>(Case.Padding), Arena, RectOrigin, RectSize);

		TestEqual(*FString::Printf(TEXT("[%s] the rect width is exactly zero"), Case.Label), RectSize.X, 0.0, PxTolerance);
		TestEqual(*FString::Printf(TEXT("[%s] the rect height is exactly zero"), Case.Label), RectSize.Y, 0.0, PxTolerance);

		TestTrue(*FString::Printf(TEXT("[%s] ⛔ the rect size is NEVER negative (a negative size inverts every hit rect)"), Case.Label),
			RectSize.X >= 0.0 && RectSize.Y >= 0.0);

		TestEqual(*FString::Printf(TEXT("[%s] the origin falls back to the panel centre — X"), Case.Label),
			RectOrigin.X, Case.Panel.X * 0.5, PxTolerance);
		TestEqual(*FString::Printf(TEXT("[%s] the origin falls back to the panel centre — Y"), Case.Label),
			RectOrigin.Y, Case.Panel.Y * 0.5, PxTolerance);
	}

	// And a zero rect really does hit-test nothing: a marker built on it has a real hit box,
	// but `BuildMarkerRects` bails before building any marker at all. Asserted here as the
	// arithmetic consequence — the widget-level version is test 16.
	FVector2D ZeroOrigin = FVector2D::ZeroVector;
	FVector2D ZeroSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(FVector2D(50.0, 50.0), static_cast<float>(ShippedMapPaddingPx), Arena, ZeroOrigin, ZeroSize);

	const FVector2D CollapsedTopLeft = FSiegeWarMapProjection::MapUVToLocal(FVector2D(0.0, 0.0), ZeroOrigin, ZeroSize);
	const FVector2D CollapsedBottomRight = FSiegeWarMapProjection::MapUVToLocal(FVector2D(1.0, 1.0), ZeroOrigin, ZeroSize);

	TestEqual(TEXT("On a collapsed rect every UV maps to the same point — X"), CollapsedTopLeft.X, CollapsedBottomRight.X, PxTolerance);
	TestEqual(TEXT("On a collapsed rect every UV maps to the same point — Y"), CollapsedTopLeft.Y, CollapsedBottomRight.Y, PxTolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. UV → LOCAL spans exactly the rect
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapUvToLocalTest,
	"Siegebound.WarMap.MapUvToLocalSpansExactlyTheRect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapUvToLocalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	const FVector2D RectOrigin(48.0, 84.0);
	const FVector2D RectSize(1824.0, 912.0);

	const FVector2D TopLeft = FSiegeWarMapProjection::MapUVToLocal(FVector2D(0.0, 0.0), RectOrigin, RectSize);
	TestEqual(TEXT("UV (0,0) is the rect origin — X"), TopLeft.X, RectOrigin.X, PxTolerance);
	TestEqual(TEXT("UV (0,0) is the rect origin — Y"), TopLeft.Y, RectOrigin.Y, PxTolerance);

	const FVector2D BottomRight = FSiegeWarMapProjection::MapUVToLocal(FVector2D(1.0, 1.0), RectOrigin, RectSize);
	TestEqual(TEXT("UV (1,1) is the rect's far corner — X"), BottomRight.X, RectOrigin.X + RectSize.X, PxTolerance);
	TestEqual(TEXT("UV (1,1) is the rect's far corner — Y"), BottomRight.Y, RectOrigin.Y + RectSize.Y, PxTolerance);

	const FVector2D Middle = FSiegeWarMapProjection::MapUVToLocal(FVector2D(0.5, 0.5), RectOrigin, RectSize);
	TestEqual(TEXT("UV (0.5,0.5) is the rect centre — X"), Middle.X, RectOrigin.X + RectSize.X * 0.5, PxTolerance);
	TestEqual(TEXT("UV (0.5,0.5) is the rect centre — Y"), Middle.Y, RectOrigin.Y + RectSize.Y * 0.5, PxTolerance);

	// The mapping is affine and axis-independent: U drives only X, V drives only Y.
	const FVector2D UOnly = FSiegeWarMapProjection::MapUVToLocal(FVector2D(0.25, 0.0), RectOrigin, RectSize);
	TestEqual(TEXT("U alone moves only the local X"), UOnly.Y, RectOrigin.Y, PxTolerance);
	TestEqual(TEXT("…by exactly a quarter of the rect width"), UOnly.X, RectOrigin.X + RectSize.X * 0.25, PxTolerance);

	const FVector2D VOnly = FSiegeWarMapProjection::MapUVToLocal(FVector2D(0.0, 0.25), RectOrigin, RectSize);
	TestEqual(TEXT("V alone moves only the local Y"), VOnly.X, RectOrigin.X, PxTolerance);
	TestEqual(TEXT("…by exactly a quarter of the rect height"), VOnly.Y, RectOrigin.Y + RectSize.Y * 0.25, PxTolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 10. THE FULL CHAIN — world corner → UV → widget-local rect corner
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapFullChainTest,
	"Siegebound.WarMap.FullChainMapsWorldCornersToRectCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapFullChainTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐ THIS IS THE COMPOSITION THE WIDGET ACTUALLY PERFORMS — `BuildMarkerRects` and the two
	// dot loops in `NativePaint` all run exactly this pair of calls in exactly this order.
	// Tests 1-9 pin each link; this one pins the CHAIN, because a link can be individually
	// correct and still be composed in the wrong order or against the wrong rect.
	const FVector2D Arena = CleanArenaHalfExtent();
	const FVector2D Panel(1920.0, 1080.0);

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(Panel, static_cast<float>(ShippedMapPaddingPx), Arena, RectOrigin, RectSize);

	struct FChainCase
	{
		const TCHAR* Label;
		FVector2D World;
		double ExpectedLocalX;
		double ExpectedLocalY;
	};

	const FChainCase Cases[] =
	{
		{ TEXT("world (-HalfX, -HalfY) lands on the rect's TOP-LEFT pixel"),     FVector2D(-Arena.X, -Arena.Y), RectOrigin.X,               RectOrigin.Y },
		{ TEXT("world (+HalfX, -HalfY) lands on the rect's TOP-RIGHT pixel"),    FVector2D( Arena.X, -Arena.Y), RectOrigin.X + RectSize.X,  RectOrigin.Y },
		{ TEXT("world (-HalfX, +HalfY) lands on the rect's BOTTOM-LEFT pixel"),  FVector2D(-Arena.X,  Arena.Y), RectOrigin.X,               RectOrigin.Y + RectSize.Y },
		{ TEXT("world (+HalfX, +HalfY) lands on the rect's BOTTOM-RIGHT pixel"), FVector2D( Arena.X,  Arena.Y), RectOrigin.X + RectSize.X,  RectOrigin.Y + RectSize.Y },
		{ TEXT("world (0,0) lands on the rect's exact centre"),                  FVector2D::ZeroVector,         RectOrigin.X + RectSize.X * 0.5, RectOrigin.Y + RectSize.Y * 0.5 }
	};

	for (const FChainCase& Case : Cases)
	{
		const FVector2D Local = FSiegeWarMapProjection::MapUVToLocal(
			FSiegeWarMapProjection::WorldToMapUV(Case.World, Arena), RectOrigin, RectSize);

		TestEqual(*FString::Printf(TEXT("%s — local X"), Case.Label), Local.X, Case.ExpectedLocalX, PxTolerance);
		TestEqual(*FString::Printf(TEXT("%s — local Y"), Case.Label), Local.Y, Case.ExpectedLocalY, PxTolerance);
	}

	// A point outside the arena still lands INSIDE the rect (the clamp survives composition).
	const FVector2D FarOut = FSiegeWarMapProjection::MapUVToLocal(
		FSiegeWarMapProjection::WorldToMapUV(FVector2D(Arena.X * 100.0, Arena.Y * 100.0), Arena), RectOrigin, RectSize);

	TestTrue(TEXT("A far-out-of-bounds world point still projects INSIDE the drawable rect — it pins to the edge rather than drawing over the WBP's chrome"),
		FarOut.X >= RectOrigin.X - PxTolerance && FarOut.X <= RectOrigin.X + RectSize.X + PxTolerance
		&& FarOut.Y >= RectOrigin.Y - PxTolerance && FarOut.Y <= RectOrigin.Y + RectSize.Y + PxTolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 11. HIT TEST — the boundary is INCLUSIVE, and only just inside is inside
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapHitTestBoundaryTest,
	"Siegebound.WarMap.MarkerHitTestBoundaryIsInclusive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapHitTestBoundaryTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐ A WIDGET RECT IN PIXELS, ⛔ NEVER A WORLD RADIUS. That distinction is `WR-§6`'s and it
	// is what keeps `AS-§21.4` honoured rather than argued around: a world radius around a
	// place symbol would be a SEMANTIC claim the model reasons over, and that number is
	// Jonathan's to supply. A pixel rect is a UI affordance the player feels with the mouse.
	const double Half = ShippedMarkerHitHalfSizePx;
	const FVector2D Centre(100.0, 100.0);

	TArray<FSiegeWarMapMarker> Markers;
	Markers.Add(MakeMarker(TEXT("mid"), Centre, Half));

	struct FHitCase
	{
		const TCHAR* Label;
		FVector2D Point;
		int32 ExpectedIndex;
	};

	const FHitCase Cases[] =
	{
		{ TEXT("the marker's exact centre"),                     FVector2D(Centre.X,        Centre.Y),        0 },
		{ TEXT("exactly on the RIGHT edge (inclusive)"),         FVector2D(Centre.X + Half, Centre.Y),        0 },
		{ TEXT("exactly on the LEFT edge (inclusive)"),          FVector2D(Centre.X - Half, Centre.Y),        0 },
		{ TEXT("exactly on the BOTTOM edge (inclusive)"),        FVector2D(Centre.X,        Centre.Y + Half), 0 },
		{ TEXT("exactly on the TOP edge (inclusive)"),           FVector2D(Centre.X,        Centre.Y - Half), 0 },
		{ TEXT("exactly on the bottom-right CORNER (inclusive)"),FVector2D(Centre.X + Half, Centre.Y + Half), 0 },
		{ TEXT("one pixel past the right edge"),                 FVector2D(Centre.X + Half + 1.0, Centre.Y),  INDEX_NONE },
		{ TEXT("one pixel past the top edge"),                   FVector2D(Centre.X, Centre.Y - Half - 1.0),  INDEX_NONE },
		{ TEXT("just past the corner on BOTH axes"),             FVector2D(Centre.X + Half + 1.0, Centre.Y + Half + 1.0), INDEX_NONE },
		{ TEXT("inside on X but outside on Y (the axis-pair trap)"), FVector2D(Centre.X, Centre.Y + Half + 5.0), INDEX_NONE },
		{ TEXT("far away on empty map"),                         FVector2D(900.0, 900.0),                     INDEX_NONE }
	};

	for (const FHitCase& Case : Cases)
	{
		TestEqual(*FString::Printf(TEXT("A click %s"), Case.Label),
			FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, Case.Point), Case.ExpectedIndex);
	}

	// A ZERO-size hit box degenerates to a single point rather than to "everything" or
	// "nothing" — the boundary rule holds at the limit.
	TArray<FSiegeWarMapMarker> PointMarker;
	PointMarker.Add(MakeMarker(TEXT("hero"), Centre, 0.0));

	TestEqual(TEXT("A zero-size hit box still matches its exact centre"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(PointMarker, Centre), 0);
	TestEqual(TEXT("…and matches nothing one pixel away"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(PointMarker, FVector2D(Centre.X + 1.0, Centre.Y)), INDEX_NONE);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 12. HIT TEST — LAST match wins, because the last marker is the one drawn ON TOP
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapHitTestOverlapTest,
	"Siegebound.WarMap.MarkerHitTestLastMatchWinsOnOverlap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapHitTestOverlapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⚠️ THIS IS NOT AN EDGE CASE — IT IS THE ORDINARY CASE. Two places genuinely coincide in
	// this game: `hero` standing at `own_castle`, or `nearest_mine` next to `mid`. Markers are
	// PAINTED in vocabulary order, so the LAST one in the array is the one drawn ON TOP.
	// Picking the FIRST would hand the player the symbol of a marker they cannot see — a
	// SILENT WRONG ANSWER, which is the single failure class this whole feature is shaped to
	// avoid.
	const double Half = ShippedMarkerHitHalfSizePx;

	TArray<FSiegeWarMapMarker> Markers;
	Markers.Add(MakeMarker(TEXT("own_castle"), FVector2D(100.0, 100.0), Half));  // painted first  ⇒ underneath
	Markers.Add(MakeMarker(TEXT("hero"),       FVector2D(110.0, 100.0), Half));  // painted second ⇒ on top

	const int32 OverlapIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(105.0, 100.0));
	TestEqual(TEXT("⭐ A click in the OVERLAP returns the LAST marker — the one actually drawn on top"), OverlapIndex, 1);

	if (Markers.IsValidIndex(OverlapIndex))
	{
		TestEqualSensitive(TEXT("…and its symbol is the visible one, byte for byte"),
			Markers[OverlapIndex].PlaceSymbol.ToString(), FString(TEXT("hero")));
	}

	// Outside the overlap, each marker still answers for its own territory.
	TestEqual(TEXT("A click inside the FIRST marker only still returns the first"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(85.0, 100.0)), 0);
	TestEqual(TEXT("A click inside the SECOND marker only still returns the second"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(125.0, 100.0)), 1);

	// Three exactly coincident markers: the topmost still wins, deterministically.
	TArray<FSiegeWarMapMarker> Stacked;
	Stacked.Add(MakeMarker(TEXT("own_castle"),   FVector2D(300.0, 300.0), Half));
	Stacked.Add(MakeMarker(TEXT("nearest_mine"), FVector2D(300.0, 300.0), Half));
	Stacked.Add(MakeMarker(TEXT("hero"),         FVector2D(300.0, 300.0), Half));

	const int32 StackedIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Stacked, FVector2D(300.0, 300.0));
	TestEqual(TEXT("Three exactly coincident markers resolve to the topmost, deterministically"), StackedIndex, 2);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 13. HIT TEST — a click on empty map returns NOTHING, and there is no other answer
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapHitTestEmptyTest,
	"Siegebound.WarMap.MarkerHitTestOnEmptyMapReturnsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapHitTestEmptyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⛔ `WR-§9` OUTCOME 1 IS A DESIGNED OUTCOME, NOT A BUG, AND THIS IS WHERE IT IS PINNED:
	// a click on empty map does NOTHING. ⛔ It does not invent a coordinate, a grid cell, a
	// snap radius or a nearest-marker guess — the shipped vocabulary has NO PRIMITIVE for an
	// arbitrary point, and manufacturing one is exactly what `AS-§21.4` reserves for Jonathan.
	const TArray<FSiegeWarMapMarker> NoMarkers;

	TestEqual(TEXT("An EMPTY marker array returns INDEX_NONE rather than crashing or guessing"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(NoMarkers, FVector2D(500.0, 500.0)), INDEX_NONE);
	TestEqual(TEXT("…including at the origin"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(NoMarkers, FVector2D::ZeroVector), INDEX_NONE);
	TestEqual(TEXT("…and at a negative point (a click above/left of the panel)"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(NoMarkers, FVector2D(-9999.0, -9999.0)), INDEX_NONE);

	// With markers present but far away, an empty-map click is still nothing — it does NOT
	// fall back to "the nearest one".
	TArray<FSiegeWarMapMarker> Markers;
	Markers.Add(MakeMarker(TEXT("mid"), FVector2D(100.0, 100.0), ShippedMarkerHitHalfSizePx));
	Markers.Add(MakeMarker(TEXT("hero"), FVector2D(900.0, 900.0), ShippedMarkerHitHalfSizePx));

	TestEqual(TEXT("⛔ A click between two markers returns NOTHING — there is NO nearest-marker fallback"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(500.0, 500.0)), INDEX_NONE);
	TestEqual(TEXT("⛔ A click barely outside a marker returns NOTHING, not that marker"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(100.0 + ShippedMarkerHitHalfSizePx + 0.5, 100.0)), INDEX_NONE);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 14. PICK — the symbol handed back is byte-identical to the one placed at that rect
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapPickSymbolIdentityTest,
	"Siegebound.WarMap.PickReturnsTheSymbolPlacedAtThatRect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapPickSymbolIdentityTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐⭐ CLICK → SYMBOL, ⛔ NEVER CLICK → COORDINATE. The insert is the LITERAL symbol
	// (`ancient_ground_near`) and not English, and the reason is MEASURED rather than
	// aesthetic: `DEV-01` recorded the natural-English "to the nearest ancient ground"
	// resolving to `nearest_mine`. The symbol is the exact token the model must emit — and it
	// is verifiable by string equality, which is what this test is.
	const FVector2D Arena = CleanArenaHalfExtent();
	const FVector2D Panel(1920.0, 1080.0);

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(Panel, static_cast<float>(ShippedMapPaddingPx), Arena, RectOrigin, RectSize);

	const TArray<FName> Expected = CanonicalPlaceSymbols();
	const TArray<FSiegeWarMapMarker> Markers = BuildCanonicalMarkers(RectOrigin, RectSize);

	TestEqual(TEXT("The fixture built one marker per canonical place symbol"), Markers.Num(), Expected.Num());
	TestEqual(TEXT("⛔ The shipped place vocabulary is SEVEN symbols — an eighth appearing here is a new spatial primitive and needs a number Jonathan supplies (WR-§6)"),
		Expected.Num(), 7);

	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		const int32 HitIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, Markers[Index].LocalCentre);

		TestEqual(*FString::Printf(TEXT("Clicking marker %d's centre returns marker %d"), Index, Index), HitIndex, Index);

		if (Markers.IsValidIndex(HitIndex))
		{
			// ⛔⛔ `TestEqualSensitive`, ⛔ NEVER `TestEqual`. `TestEqual` on `FString` is
			// CASE-INSENSITIVE in UE 5.8 and a shipped, QA-passed test in this project once
			// asserted nothing because of exactly that. `FName` comparison is case-insensitive
			// too, which is why the claim is made on the STRING and not on the FName.
			TestEqualSensitive(*FString::Printf(TEXT("…carrying the LITERAL symbol placed there, byte for byte (marker %d)"), Index),
				Markers[HitIndex].PlaceSymbol.ToString(), Expected[Index].ToString());
		}
	}

	// ⛔ THE SYMBOLS KEEP THEIR UNDERSCORES AND THEIR CASE. `ancient_ground_near` is what the
	// model emits for `where`; `Ancient Ground Near` is not a token in any shipped grammar.
	TestEqualSensitive(TEXT("⭐ The spec's own hazard symbol survives verbatim, underscores and all"),
		Expected[3].ToString(), FString(TEXT("ancient_ground_near")));
	TestEqualSensitive(TEXT("…and so does its sibling, so the two are not silently the same string"),
		Expected[4].ToString(), FString(TEXT("ancient_ground_far")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 15. ⭐ THE AIRLOCK, MEASURED — the reachable pick set is EXACTLY the marker set, or nothing
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapAirlockPickSetTest,
	"Siegebound.WarMap.AirlockPickSetIsExactlyTheMarkerSetOrNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapAirlockPickSetTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ═══════════════════════════════════════════════════════════════════════════
	// ⭐⭐ THIS IS THE MOST IMPORTANT TEST IN THE FILE, AND IT IS A MEASUREMENT
	// RATHER THAN AN ARGUMENT.
	//
	// The map's ENTIRE outbound surface is one delegate carrying one `FName`
	// (`FOnWarMapPlacePicked`), and the only value that can ride it is
	// `Markers[HitIndex].PlaceSymbol` — read out of an array `BuildMarkerRects`
	// filled from `USiegeAssistantSnapshot::GetPlaceNames()`. `UWarMapWidget`
	// contains NO place-symbol literal at all.
	//
	// ⇒ THE CLAIM: however precisely the player clicks, the map can emit ONE OF
	// THE SYMBOLS IT WAS HANDED, OR NOTHING. ⛔ It cannot emit a coordinate, a
	// grid cell, a snap radius, an interpolation between two markers, or an
	// EIGHTH PLACE — because there is no code path that constructs a symbol.
	//
	// ⚖️ AND THAT IS WHY THIS FEATURE SPENDS ZERO PROMPT CHARACTERS: nothing the
	// map computes has a channel into a prompt zone. A test that asserts THAT is
	// worth more than a test that asserts a pixel.
	//
	// ⛔ THE SWEEP COLLECTS `int32` INDICES, ⛔ NOT `FName`s OR `FString`s, AND
	// THAT IS DELIBERATE: `TSet<FName>` hashes case-INSENSITIVELY and
	// `GetTypeHash(FString)` is `Strihash`, so a symbol whose case had drifted
	// would be silently absorbed by either container. Indices are exact; test 14
	// pins index → symbol byte-exactly with `TestEqualSensitive`. The two
	// together give a byte-exact claim with no case-insensitive container
	// anywhere in the argument.
	// ═══════════════════════════════════════════════════════════════════════════

	const FVector2D Arena = CleanArenaHalfExtent();
	const FVector2D Panel(1920.0, 1080.0);

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(Panel, static_cast<float>(ShippedMapPaddingPx), Arena, RectOrigin, RectSize);

	const TArray<FSiegeWarMapMarker> Markers = BuildCanonicalMarkers(RectOrigin, RectSize);

	// ── PRECONDITION, ASSERTED RATHER THAN ASSUMED ───────────────────────────
	// If any two fixture rects overlapped, the last-match-wins rule (test 12) would make one
	// marker unreachable and turn a REACHABILITY FAILURE INTO A PASS. So the fixture's own
	// assumption is checked first — the same "did the guard actually run" discipline `SC-§21`
	// applies to guard placement.
	bool bNoFixtureOverlap = true;
	for (int32 A = 0; A < Markers.Num(); ++A)
	{
		for (int32 B = A + 1; B < Markers.Num(); ++B)
		{
			const bool bOverlapsX = FMath::Abs(Markers[A].LocalCentre.X - Markers[B].LocalCentre.X)
				<= Markers[A].LocalHitHalfSize.X + Markers[B].LocalHitHalfSize.X;
			const bool bOverlapsY = FMath::Abs(Markers[A].LocalCentre.Y - Markers[B].LocalCentre.Y)
				<= Markers[A].LocalHitHalfSize.Y + Markers[B].LocalHitHalfSize.Y;

			bNoFixtureOverlap = bNoFixtureOverlap && !(bOverlapsX && bOverlapsY);
		}
	}
	TestTrue(TEXT("PRECONDITION: no two fixture markers overlap, so every one of them is genuinely reachable and the sweep below means what it says"), bNoFixtureOverlap);

	// ── THE SWEEP ─────────────────────────────────────────────────────────────
	// 400 x 300 samples over the WHOLE panel — cells of 4.8 x 3.6 px against 36 x 36 px hit
	// rects, so every marker is sampled dozens of times and none can be missed by aliasing.
	constexpr int32 SamplesX = 400;
	constexpr int32 SamplesY = 300;

	TSet<int32> ReachedIndices;
	int32 EmptyClicks = 0;
	int32 OutOfRangeAnswers = 0;

	for (int32 Ix = 0; Ix < SamplesX; ++Ix)
	{
		const double LocalX = Panel.X * (static_cast<double>(Ix) + 0.5) / static_cast<double>(SamplesX);

		for (int32 Iy = 0; Iy < SamplesY; ++Iy)
		{
			const double LocalY = Panel.Y * (static_cast<double>(Iy) + 0.5) / static_cast<double>(SamplesY);

			const int32 HitIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(LocalX, LocalY));

			if (HitIndex == INDEX_NONE)
			{
				++EmptyClicks;
			}
			else if (!Markers.IsValidIndex(HitIndex))
			{
				// ⛔ AN ANSWER THAT NAMES NO MARKER. This counter exists because a returned
				// index that is not an index into the array WE HANDED IN would be the map
				// synthesizing an answer — the one thing the airlock forbids.
				++OutOfRangeAnswers;
			}
			else
			{
				ReachedIndices.Add(HitIndex);
			}
		}
	}

	AddInfo(FString::Printf(
		TEXT("Airlock sweep: %d sample clicks over a %.0fx%.0f panel; %d distinct markers reached; %d clicks on empty map; %d answers outside the marker array."),
		SamplesX * SamplesY, Panel.X, Panel.Y, ReachedIndices.Num(), EmptyClicks, OutOfRangeAnswers));

	// ── THE THREE CLAIMS ──────────────────────────────────────────────────────

	TestEqual(TEXT("⭐⭐ ⛔ ZERO clicks produced an answer outside the marker array — the map can only ever name a marker it was HANDED, never one it constructed"),
		OutOfRangeAnswers, 0);

	TestEqual(TEXT("⭐ Every one of the seven markers is reachable by a click — and ⛔ NO EIGHTH ANSWER EXISTS anywhere on the panel"),
		ReachedIndices.Num(), Markers.Num());

	TestTrue(TEXT("⭐ The overwhelming majority of the panel is EMPTY MAP and answers NOTHING — a click there emits no symbol at all (WR-§9 outcome 1, a designed outcome)"),
		EmptyClicks > (SamplesX * SamplesY) / 2);

	// And the reachable set is exactly {0 .. N-1}: no index is skipped and none is invented.
	bool bEveryIndexReached = true;
	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		bEveryIndexReached = bEveryIndexReached && ReachedIndices.Contains(Index);
	}
	TestTrue(TEXT("The reachable index set is exactly {0 .. MarkerCount-1} — nothing skipped, nothing invented"), bEveryIndexReached);

	// ── THE OTHER HALF OF THE AIRLOCK: the pick carries a SYMBOL, not geometry ─
	// `FindMarkerIndexAtLocal` returns an `int32`. The symbol is then READ from the marker.
	// Neither the clicked POINT nor the marker's CENTRE nor its HIT SIZE has any route out of
	// this class — the delegate signature is `(FName)` and nothing else.
	const int32 ProbeIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, Markers[2].LocalCentre);
	TestEqual(TEXT("A pick is an INDEX into the caller's array — the geometry never leaves"), ProbeIndex, 2);

	if (Markers.IsValidIndex(ProbeIndex))
	{
		TestEqualSensitive(TEXT("…and the only thing read out of it is the place symbol"),
			Markers[ProbeIndex].PlaceSymbol.ToString(), FString(TEXT("mid")));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 16. BUILD MARKER RECTS — no snapshot ⇒ no markers, and the array is RESET not appended
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapBuildMarkerRectsWithoutSnapshotTest,
	"Siegebound.WarMap.BuildMarkerRectsYieldsNothingWithoutASnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapBuildMarkerRectsWithoutSnapshotTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⚠️⚠️ THIS TEST ENCODES TASK-560's NAMED FINDING RATHER THAN PAPERING OVER IT.
	// `GetTurnSnapshot()` is NULL until the player's FIRST console sentence, so a map opened
	// before any console use shows dots and NO PLACE MARKERS — and therefore has nothing to
	// click. That is escalated in `handoffs/TASK-560-programmer.md` with three candidate
	// owners; it is NOT fixed in code, because every available fix crosses a boundary that
	// task may not cross.
	//
	// ⭐ WHAT THIS TEST PINS IS THAT THE DEGRADATION IS **CLEAN**: empty, never partial, never
	// stale. A partial marker list would be far worse than an empty one — the player would
	// click a marker and get a symbol resolved against a survey that no longer exists.
	TStrongObjectPtr<UWarMapWidget> Map(NewObject<UWarMapWidget>(GetTransientPackageAsObject()));

	if (!TestTrue(TEXT("A war-map widget object was created"), Map.IsValid()))
	{
		return false;
	}

	// Pre-populated with junk, so "returns empty" and "cleared the caller's array" are
	// distinguishable outcomes. `BuildMarkerRects` resets FIRST, before any early return.
	TArray<FSiegeWarMapMarker> Markers;
	Markers.Add(MakeMarker(TEXT("own_castle"), FVector2D(10.0, 10.0), ShippedMarkerHitHalfSizePx));
	Markers.Add(MakeMarker(TEXT("hero"), FVector2D(20.0, 20.0), ShippedMarkerHitHalfSizePx));
	Markers.Add(MakeMarker(TEXT("mid"), FVector2D(30.0, 30.0), ShippedMarkerHitHalfSizePx));

	Map->BuildMarkerRects(MakePanel(1920.f, 1080.f), Markers);

	TestEqual(TEXT("⛔ With no owning controller and therefore no snapshot, BuildMarkerRects yields ZERO markers — empty, ⛔ never partial"),
		Markers.Num(), 0);

	TestEqual(TEXT("⭐ …and a click then finds nothing, so there is no stale marker to pick"),
		FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, FVector2D(10.0, 10.0)), INDEX_NONE);

	// ⛔ THE DEGENERATE-PANEL EARLY RETURN IS **NOT** COVERED HERE, AND SAYING SO IS THE
	// HONEST THING TO DO RATHER THAN ADDING A LINE THAT WOULD PASS FOR THE WRONG REASON.
	// `BuildMarkerRects` tests the snapshot FIRST; with no snapshot it returns before the
	// rect is ever computed, so a "degenerate panel yields no markers" assertion here would
	// be green because of the NULL SNAPSHOT and would stay green if the panel guard were
	// deleted outright. ⇒ That guard's arithmetic is covered instead by test 8 (the rect
	// really does collapse to zero) and its CONSUMPTION needs a snapshot, i.e. PIE.
	// (`SC-§32`: a guardrail nobody has watched trip is a guardrail nobody has tested — so it
	// is named as untested rather than decorated with a test that cannot see it.)
	TArray<FSiegeWarMapMarker> TinyPanelMarkers;
	TinyPanelMarkers.Add(MakeMarker(TEXT("mid"), FVector2D(1.0, 1.0), ShippedMarkerHitHalfSizePx));

	Map->BuildMarkerRects(MakePanel(4.f, 4.f), TinyPanelMarkers);
	TestEqual(TEXT("A tiny panel also yields ZERO markers — ⚠️ but note this passes on the NO-SNAPSHOT return, ⛔ not on the degenerate-panel guard, which is unreachable headlessly"),
		TinyPanelMarkers.Num(), 0);

	// And a closed, never-opened map holds no dots of either colour.
	TestEqual(TEXT("A freshly constructed map holds no enemy reveal"), Map->GetEnemyRevealDotCount(), 0);
	TestEqual(TEXT("A freshly constructed map holds no ally dots"), Map->GetAllyDotCount(), 0);
	TestFalse(TEXT("…and it is CLOSED (nothing is on screen and nothing is hit-testable until OpenMap)"), Map->IsMapOpen());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 17. REVEAL — starts empty, and a second purchase REPLACES rather than appends
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapRevealReplacesTest,
	"Siegebound.WarMap.RevealReplacesNeverAppendsAndStartsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapRevealReplacesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	TStrongObjectPtr<UWarMapWidget> Map(NewObject<UWarMapWidget>(GetTransientPackageAsObject()));

	if (!TestTrue(TEXT("A war-map widget object was created"), Map.IsValid()))
	{
		return false;
	}

	// ⛔ NO DOTS BEFORE PAYMENT. This is the first leg of `WR-§7`'s state machine.
	TestEqual(TEXT("⛔ No red dots exist before anything is paid for"), Map->GetEnemyRevealDotCount(), 0);

	Map->ReceiveEnemyReveal(MakeDotList(5));
	TestEqual(TEXT("A reveal of five dots is held as five"), Map->GetEnemyRevealDotCount(), 5);

	// ⛔⛔ REPLACE, ⛔ NEVER APPEND. "Pay another 30 gold to reveal the NEW locations" is a
	// REPLACEMENT in Jonathan's own sentence. An accumulating list would slowly turn into a
	// heat map of everywhere the enemy has ever been — strictly more information than was
	// paid for, and a mechanic nobody designed.
	Map->ReceiveEnemyReveal(MakeDotList(2));
	TestEqual(TEXT("⭐ A SECOND reveal REPLACES the first — two dots, ⛔ not seven (an appending list would become a heat map of everywhere the enemy has ever been)"),
		Map->GetEnemyRevealDotCount(), 2);

	// An EMPTY payload is a legitimate answer — "the enemy has nothing alive" — and it must
	// clear the previous survey rather than being ignored as a no-op.
	Map->ReceiveEnemyReveal(TArray<FVector2D>());
	TestEqual(TEXT("An EMPTY reveal payload clears the previous one — an enemy with no units is a real answer, not a no-op"),
		Map->GetEnemyRevealDotCount(), 0);

	// The reveal is entirely independent of the open state: the widget is a SINK, and it does
	// not decide whether a purchase was legitimate.
	Map->ReceiveEnemyReveal(MakeDotList(3));
	TestEqual(TEXT("The widget accepts dots without checking a cost, a balance or an authority — it is the SINK, never the source (WR-§7)"),
		Map->GetEnemyRevealDotCount(), 3);
	TestEqual(TEXT("…and holding a reveal does not manufacture ally dots"), Map->GetAllyDotCount(), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 18. ⭐ REVEAL — CLEARED ON CLOSE, ALWAYS, AND STILL CLEARED ON RE-OPEN
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapRevealClearedOnCloseTest,
	"Siegebound.WarMap.RevealIsClearedOnCloseAndStaysClearedOnReopen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapRevealClearedOnCloseTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐ THIS IS JONATHAN'S MECHANIC IN HIS OWN WORDS: "the enemy locations go away as soon as
	// you close the map and you have to pay another 30 gold." `WR-§7` makes it law and
	// `WR-§9` outcome 4 puts it on the playtest sheet so a report of it is read correctly.
	// ⛔ A "keep it if it was recent" kindness here would delete the mechanic, so the clear is
	// UNCONDITIONAL and behind no flag.
	//
	// ⚠️ THE OPEN/CLOSE PATH RUNS HEADLESSLY BECAUSE EVERY WORLD-DEPENDENT LEG INSIDE IT
	// NULL-GUARDS `GetWorld()` AND EARLY-RETURNS. That was verified at the source, not
	// assumed. What is therefore NOT exercised here is the ally-dot sweep and the refresh
	// timer, which need a world — TASK-569's PIE row (h) is their instrument.
	TStrongObjectPtr<UWarMapWidget> Map(NewObject<UWarMapWidget>(GetTransientPackageAsObject()));

	if (!TestTrue(TEXT("A war-map widget object was created"), Map.IsValid()))
	{
		return false;
	}

	// ── (1) no dots before payment ────────────────────────────────────────────
	Map->OpenMap();
	TestTrue(TEXT("OpenMap opens the map"), Map->IsMapOpen());
	TestEqual(TEXT("(1) ⛔ Nothing is revealed on a fresh open"), Map->GetEnemyRevealDotCount(), 0);

	// ── (2) dots after a successful spend ─────────────────────────────────────
	Map->ReceiveEnemyReveal(MakeDotList(4));
	TestEqual(TEXT("(2) After the authority pushes a paid reveal, the dots are held"), Map->GetEnemyRevealDotCount(), 4);

	// ── (3) ⛔ CLEARED ON CLOSE ───────────────────────────────────────────────
	Map->CloseMap();
	TestFalse(TEXT("CloseMap closes the map"), Map->IsMapOpen());
	TestEqual(TEXT("(3) ⭐⛔ Closing the map DISCARDS the paid reveal, unconditionally"), Map->GetEnemyRevealDotCount(), 0);

	// ── (4) ⛔ STILL CLEARED ON RE-OPEN ───────────────────────────────────────
	Map->OpenMap();
	TestTrue(TEXT("The map re-opens"), Map->IsMapOpen());
	TestEqual(TEXT("(4) ⭐⛔ Re-opening shows NO red dots — the player pays again, even one second after paying (WR-§7 / WR-§9 outcome 4)"),
		Map->GetEnemyRevealDotCount(), 0);

	// ── The mechanic survives repetition: pay, close, re-open, three times ────
	for (int32 Cycle = 1; Cycle <= 3; ++Cycle)
	{
		Map->ReceiveEnemyReveal(MakeDotList(Cycle + 1));
		TestEqual(*FString::Printf(TEXT("Cycle %d: the purchase lands"), Cycle), Map->GetEnemyRevealDotCount(), Cycle + 1);

		Map->CloseMap();
		TestEqual(*FString::Printf(TEXT("Cycle %d: the close clears it — ⛔ no persistence across opens"), Cycle), Map->GetEnemyRevealDotCount(), 0);

		Map->OpenMap();
		TestEqual(*FString::Printf(TEXT("Cycle %d: the re-open still shows nothing"), Cycle), Map->GetEnemyRevealDotCount(), 0);
	}

	// A redundant OpenMap on an already-open map is a no-op and does NOT wipe a live reveal —
	// the guard exists so a double-bind or a repeated key press cannot destroy a purchase.
	Map->ReceiveEnemyReveal(MakeDotList(6));
	Map->OpenMap();
	TestEqual(TEXT("A redundant OpenMap while already open does NOT discard a live purchase"), Map->GetEnemyRevealDotCount(), 6);
	TestTrue(TEXT("…and the map is still open"), Map->IsMapOpen());

	// A redundant CloseMap on an already-closed map is likewise a no-op.
	Map->CloseMap();
	Map->CloseMap();
	TestFalse(TEXT("A redundant CloseMap leaves the map closed"), Map->IsMapOpen());
	TestEqual(TEXT("…and the reveal stays cleared"), Map->GetEnemyRevealDotCount(), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 19. CLEAR — idempotent, and safe on an already-empty reveal
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapClearRevealIdempotentTest,
	"Siegebound.WarMap.ClearEnemyRevealIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapClearRevealIdempotentTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	TStrongObjectPtr<UWarMapWidget> Map(NewObject<UWarMapWidget>(GetTransientPackageAsObject()));

	if (!TestTrue(TEXT("A war-map widget object was created"), Map.IsValid()))
	{
		return false;
	}

	// `ClearEnemyReveal` is reachable from `CloseMap`, from `NativeDestruct` and directly for
	// a match reset, so it is called more than once on the same empty state as a matter of
	// course. It must be a no-op there rather than a re-broadcast — the shipped delegate law
	// (a surface that fires on non-changes trains its consumers to ignore it).
	Map->ClearEnemyReveal();
	TestEqual(TEXT("Clearing an already-empty reveal is safe and leaves it empty"), Map->GetEnemyRevealDotCount(), 0);

	Map->ReceiveEnemyReveal(MakeDotList(7));
	TestEqual(TEXT("A reveal is held"), Map->GetEnemyRevealDotCount(), 7);

	Map->ClearEnemyReveal();
	TestEqual(TEXT("The first clear drops it"), Map->GetEnemyRevealDotCount(), 0);

	Map->ClearEnemyReveal();
	Map->ClearEnemyReveal();
	TestEqual(TEXT("Repeated clears are idempotent — still zero, no crash, no partial state"), Map->GetEnemyRevealDotCount(), 0);

	// After a clear, a new purchase still lands: clearing is not a latch.
	Map->ReceiveEnemyReveal(MakeDotList(2));
	TestEqual(TEXT("A purchase after a clear still lands — ClearEnemyReveal is not a one-way latch"), Map->GetEnemyRevealDotCount(), 2);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 20. TOGGLE — mirrors the open state, and carries the clear-on-close with it
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapToggleTest,
	"Siegebound.WarMap.ToggleMapMirrorsTheOpenState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapToggleTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// `ToggleMap` is the natural binding for the `IA_WarMap` key (TASK-563), so the mechanic
	// has to survive being driven from the toggle and not only from the explicit pair. ⚠️ If
	// `ToggleMap` had been written as its own state machine rather than as a dispatch to
	// `OpenMap`/`CloseMap`, the clear-on-close would be reachable one way and not the other —
	// and the key path is the one the player actually uses.
	TStrongObjectPtr<UWarMapWidget> Map(NewObject<UWarMapWidget>(GetTransientPackageAsObject()));

	if (!TestTrue(TEXT("A war-map widget object was created"), Map.IsValid()))
	{
		return false;
	}

	TestFalse(TEXT("A fresh map is closed"), Map->IsMapOpen());

	Map->ToggleMap();
	TestTrue(TEXT("The first toggle opens it"), Map->IsMapOpen());

	Map->ReceiveEnemyReveal(MakeDotList(3));
	TestEqual(TEXT("A reveal is held while open"), Map->GetEnemyRevealDotCount(), 3);

	Map->ToggleMap();
	TestFalse(TEXT("The second toggle closes it"), Map->IsMapOpen());
	TestEqual(TEXT("⭐ …and the toggle path clears the reveal exactly as the explicit CloseMap does — the key the player presses is not a second, weaker route"),
		Map->GetEnemyRevealDotCount(), 0);

	Map->ToggleMap();
	TestTrue(TEXT("The third toggle re-opens it"), Map->IsMapOpen());
	TestEqual(TEXT("…still with no red dots"), Map->GetEnemyRevealDotCount(), 0);

	// Six more toggles land back where they started — the state machine does not drift.
	for (int32 Toggle = 0; Toggle < 6; ++Toggle)
	{
		Map->ToggleMap();
	}
	TestTrue(TEXT("An even number of further toggles returns the map to open — the state does not drift"), Map->IsMapOpen());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 21. THE REVEAL PRICE — 30 gold, on the ACommanderNpc CDO (WR-§7)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapRevealCostTest,
	"Siegebound.WarMap.EnemyRevealCostIsThirtyOnTheCommanderCdo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapRevealCostTest::RunTest(const FString& Parameters)
{
	// ⭐ JONATHAN'S OWN NUMBER, ASSERTED WHERE IT LIVES: "You can pay 30 gold to reveal all
	// enemy locations." `WR-§7` makes it an `EditDefaultsOnly` `UPROPERTY` default on
	// `ACommanderNpc` — ⛔ a MECHANIC RULE, and therefore ⛔ NEVER a `cards.csv` column (the
	// GDD §3.0 "mechanic rules aren't card stats" law).
	//
	// ⚠️ THIS IS THE **CDO**, i.e. the C++ default, and the limit is stated rather than
	// glossed: a `BP_CommanderNpc` could override it, and that is the designer seam working as
	// intended. What this pins is that the SHIPPED DEFAULT is the number in the directive.
	const ACommanderNpc* const CommanderDefaults = GetDefault<ACommanderNpc>();

	if (!TestNotNull(TEXT("The ACommanderNpc class default object resolves"), CommanderDefaults))
	{
		return false;
	}

	TestEqual(TEXT("⭐ EnemyRevealCost defaults to 30 gold — Jonathan's directive, as a UPROPERTY default (WR-§7)"),
		CommanderDefaults->GetEnemyRevealCost(), 30);

	// ⛔ A FREE REVEAL WOULD DELETE THE MECHANIC SILENTLY. A zero cost still "spends"
	// successfully on every balance, so the button would never refuse and the tactical cost of
	// the map would vanish with nothing in the log to say so.
	TestTrue(TEXT("⛔ The reveal is never free — a zero cost would make the purchase unconditional and delete the mechanic silently"),
		CommanderDefaults->GetEnemyRevealCost() > 0);

	// The proximity gate is a BODY-SCALE number and `WR-§1` / `SC-§34`'s human-scale exemption
	// says it did NOT grow with the 9x castle. Asserted as "positive and plausibly human"
	// rather than pinned to a value, because it is explicitly FLAGGED for Jonathan's feel pass.
	TestTrue(TEXT("InteractRadius is strictly positive, so the war-map proximity gate can actually be satisfied"),
		CommanderDefaults->GetInteractRadius() > 0.f);

	TestTrue(TEXT("⚠️ InteractRadius is still a HUMAN-SCALE number and was NOT multiplied by the 9x castle (WR-§1, SC-§34's human-scale exemption) — a walk-up-to-a-table radius, not a castle-derived one"),
		CommanderDefaults->GetInteractRadius() < 2000.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 22. ⭐ THE CONSOLE SEAM — it refuses loudly and ⛔ NEVER OPENS THE CONSOLE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapAppendToInputRefusalTest,
	"Siegebound.WarMap.AppendToInputRefusesAndNeverOpensTheConsole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapAppendToInputRefusalTest::RunTest(const FString& Parameters)
{
	// ═══════════════════════════════════════════════════════════════════════════
	// ⚠️⚠️ READ THIS BEFORE JUDGING THE COVERAGE HERE — WHAT IS ASSERTED AND WHAT
	// IS **NOT**, AND WHY THE MISSING HALF IS A REPORTED FINDING RATHER THAN AN
	// OMISSION.
	//
	// TASK-564's spec item (4) asks for the whitespace rule (empty box /
	// mid-sentence / trailing space) to be asserted by string equality. ⛔ IT IS
	// NOT ASSERTED HERE, AND IT CANNOT BE, FOR THREE REASONS ALL VERIFIED AT THE
	// SOURCE:
	//   (a) the composition is INLINE inside `AppendToInput`, not extracted into
	//       a pure helper — TASK-561 shipped it that way and its handoff §8.7
	//       explicitly offers the split to this task;
	//   (b) reaching that composition requires `bConsoleOpen` AND `InputBox`
	//       non-null, i.e. a Slate-REALIZED widget tree (`ConstructConsoleTree`
	//       runs from `RebuildWidget`, which only runs from `TakeWidget`) plus
	//       `OpenConsole()`, which takes keyboard focus — a PIE/Slate dependency,
	//       not a headless unit;
	//   (c) even with all of that, `InputBox` is `protected` and the class
	//       exposes NO public getter for the input text, so the composed string
	//       is unreadable through the shipped public surface.
	// ⇒ ⛔ AND I MAY NOT FIX (a): TASK-564's `names:` block lists every non-test
	// source file as NOT TOUCHED. ⇒ REPORTED, ⛔ NOT CODED AROUND
	// (`handoffs/TASK-564-programmer.md` §4). ⛔ A test that transcribed the rule
	// and asserted it against its own replica would assert NOTHING about shipped
	// code, which is precisely the failure this batch keeps warning about.
	//
	// ✅ WHAT **IS** ASSERTED BELOW IS THE HALF THAT IS BOTH REACHABLE AND, ON
	// THE AIRLOCK ARGUMENT, THE MORE IMPORTANT ONE: the seam refuses rather than
	// silently no-opping, and ⛔ IT NEVER OPENS THE CONSOLE. Every path below
	// early-returns before any Slate work.
	// ═══════════════════════════════════════════════════════════════════════════

	TStrongObjectPtr<USiegeAssistantConsoleWidget> Console(
		NewObject<USiegeAssistantConsoleWidget>(GetTransientPackageAsObject()));

	if (!TestTrue(TEXT("An assistant console widget object was created"), Console.IsValid()))
	{
		return false;
	}

	TestFalse(TEXT("A freshly constructed console is CLOSED"), Console->IsConsoleOpen());

	// ── Nothing to insert ⇒ refused, and ⛔ no stray separator appended ────────
	TestFalse(TEXT("An EMPTY insert is refused — ⛔ never a silent no-op, and ⛔ no stray separator is appended"),
		Console->AppendToInput(FString()));

	TestFalse(TEXT("A WHITESPACE-ONLY insert is refused (it trims to empty before anything else runs)"),
		Console->AppendToInput(FString(TEXT("   "))));

	TestFalse(TEXT("A tab-and-newline insert is refused for the same reason"),
		Console->AppendToInput(FString(TEXT("\t\n "))));

	// ── ⭐ A REAL SYMBOL INTO A CLOSED CONSOLE ⇒ REFUSED, AND STILL CLOSED ────
	// ⛔⛔ THIS IS THE CONTRACT THAT MATTERS TO THE AIRLOCK, AND IT IS TASK-561's
	// PINNED PROMISE TO TASK-563: `AppendToInput` NEVER opens the console and NEVER submits.
	// Two independent mechanisms back it — the input POSTURE belongs to
	// `ASiegePlayerController` (this widget never calls `SetInputMode`), and `OpenConsole()`
	// clears the box on EVERY open, so a write into a closed console would be destroyed a
	// moment later, silently. The caller opens first, through the posture-owning path, and
	// checks the bool.
	TestFalse(TEXT("A place symbol into a CLOSED console is refused, loudly, and returns false"),
		Console->AppendToInput(FString(TEXT("ancient_ground_near"))));

	TestFalse(TEXT("⭐⛔ …and the console is STILL CLOSED — the seam did NOT open itself to make the insert succeed (TASK-561's pinned contract; a widget-initiated open would put a visible console on the wrong input posture)"),
		Console->IsConsoleOpen());

	// Repeating the refused call does not accumulate state or flip anything.
	TestFalse(TEXT("A second refused insert behaves identically"),
		Console->AppendToInput(FString(TEXT("mid"))));
	TestFalse(TEXT("…and the console is still closed"), Console->IsConsoleOpen());

	// ⛔ NOTHING WAS SUBMITTED, AND NOTHING COULD HAVE BEEN. `AppendToInput` has no route to
	// `SubmitPressed`, builds no sentence, spends no model call and touches no prompt zone. It
	// moves an OPAQUE string into one text box and knows nothing about the place vocabulary —
	// which symbols exist is `USiegeAssistantVocabulary`'s, and which are clickable is
	// `UWarMapWidget`'s. ⇒ The war map spends ZERO prompt characters, and Zone A stays frozen
	// at the baseline `Tests/SiegeAssistantZoneATest.cpp` already asserts (see this file's
	// header: ⛔ that file is UNTOUCHED and no second assertion was added here).
	TestFalse(TEXT("⛔ After four refused inserts the console has still never opened — the map cannot open, drive or submit through the assistant"),
		Console->IsConsoleOpen());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 23. ⭐⭐ THE WHITESPACE RULE — asserted at last, on the SHIPPED function
//     (TASK-581, CONVENTIONS WR-§6 ruling W4-R1)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⭐ THIS TEST IS THE WHOLE POINT OF TASK-581 AND IT IS WORTH SAYING WHY IN ONE
// LINE: `USiegeAssistantConsoleWidget::ComposeAppendedInput` is a plain `static`,
// so there is NO instance below, NO `NewObject`, NO Slate, NO world, NO CDO and NO
// realized widget tree. Test 22 above needs a widget object; this one needs
// nothing at all. That difference IS the testability seam `WR-§6` now requires.
//
// ⛔ AND IT ASSERTS THE SHIPPED FUNCTION, ⛔ NOT A REPLICA. TASK-564 refused to
// transcribe the rule into this file and assert it against its own copy, because
// that would have asserted NOTHING about shipped code — the exact failure mode a
// shipped, QA-passed test in this project once had. Every call below is a call
// into `SiegeAssistantConsoleWidget.cpp`.
//
// ⛔ EVERY STRING CLAIM USES `TestEqualSensitive`. `TestEqual` on `FString` is
// CASE-INSENSITIVE in UE 5.8, and `operator==` on `FString` is too — which is why
// the prefix claims below are made with `Left()` + `TestEqualSensitive` rather
// than with `StartsWith` or `==`.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapComposeAppendedInputTest,
	"Siegebound.WarMap.ComposeAppendedInputWhitespaceRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapComposeAppendedInputTest::RunTest(const FString& Parameters)
{
	// ── (a) AN EMPTY BOX ⇒ THE BARE SYMBOL PLUS ONE TRAILING SPACE ────────────
	// ⛔ AND ⛔ NO LEADING SPACE. This is the leg that the `!IsEmpty()` test in
	// rule 1 buys: it is FIRST in the `&&` precisely so the index access that
	// follows it is safe, and a reordering would be an out-of-bounds read on
	// exactly this input rather than a wrong answer.
	TestEqualSensitive(TEXT("(a) An EMPTY box yields the bare symbol plus ONE trailing space — ⛔ never a leading space"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(FString(), FString(TEXT("mid"))),
		FString(TEXT("mid ")));

	TestEqualSensitive(TEXT("(a) …and the same holds for the longest shipped place symbol, underscores and case intact"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(FString(), FString(TEXT("ancient_ground_near"))),
		FString(TEXT("ancient_ground_near ")));

	// ── (b) ⭐ A BOX ENDING IN A NON-SPACE ⇒ EXACTLY ONE SPACE IS INSERTED ─────
	// ⭐⭐ THIS IS TASK-561's OWN WORKED EXAMPLE AND IT APPEARS VERBATIM, BECAUSE
	// IT IS THE FAILURE THAT WOULD BREAK THE WHOLE FEATURE QUIETLY: a missing
	// separator gives `toancient_ground_near`, the parse fails, and the war room
	// reads as broken while every individual piece of it looks correct.
	TestEqualSensitive(TEXT("(b) ⭐ A box ending in a NON-space gets exactly ONE separator — the spec's own worked example"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("send 10 footmen to")), FString(TEXT("ancient_ground_near"))),
		FString(TEXT("send 10 footmen to ancient_ground_near ")));

	// The negative form of the same claim, stated separately so a future edit that
	// produced the collision fails on a line that NAMES the collision.
	TestFalse(TEXT("(b) ⛔ …and the two words are NEVER run together — no `toancient_ground_near` collision"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("send 10 footmen to")), FString(TEXT("ancient_ground_near")))
			.Contains(TEXT("toancient_ground_near"), ESearchCase::CaseSensitive));

	// ── (c) A BOX ALREADY ENDING IN A SPACE ⇒ ⛔ NO SECOND SPACE ──────────────
	TestEqualSensitive(TEXT("(c) A box already ending in a SPACE gets ⛔ NO second separator"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("send 10 footmen to ")), FString(TEXT("ancient_ground_near"))),
		FString(TEXT("send 10 footmen to ancient_ground_near ")));

	TestFalse(TEXT("(c) ⛔ …and no double space was introduced anywhere in the result"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("send 10 footmen to ")), FString(TEXT("ancient_ground_near")))
			.Contains(TEXT("  "), ESearchCase::CaseSensitive));

	// ── (d) ⭐ A BOX ENDING IN A **TAB** ⇒ ⛔ NO SPACE ADDED ───────────────────
	// ⭐⭐ THE `FChar::IsWhitespace`-NOT-`EndsWith(" ")` LEG, AND IT IS THE ONE A
	// NAIVE REWRITE BREAKS SILENTLY: `EndsWith(" ")` is false for a tab, so a
	// rewritten rule 1 would insert a separator after one and produce "\t " —
	// a result no other assertion in this file would notice.
	TestEqualSensitive(TEXT("(d) ⭐ A box ending in a TAB gets ⛔ NO separator — the test is FChar::IsWhitespace on the LAST CHARACTER, ⛔ not EndsWith(\" \")"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("send 10 footmen to\t")), FString(TEXT("mid"))),
		FString(TEXT("send 10 footmen to\tmid ")));

	// The same leg for the other whitespace characters `FChar::IsWhitespace`
	// accepts and `EndsWith(" ")` does not, so the claim is about WHITESPACE and
	// not about one lucky character.
	TestEqualSensitive(TEXT("(d) …a NEWLINE terminator is whitespace too, and also gets no separator"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("attack\n")), FString(TEXT("mid"))),
		FString(TEXT("attack\nmid ")));

	TestEqualSensitive(TEXT("(d) …and so is a CARRIAGE RETURN"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("attack\r")), FString(TEXT("mid"))),
		FString(TEXT("attack\rmid ")));

	// ── (e) ⭐ IDEMPOTENCE — COMPOSE TWICE ────────────────────────────────────
	// ⭐ THE PROPERTY RULE 2 EXISTS FOR, ASSERTED AS A COMPOSITION RATHER THAN AS
	// A LITERAL: the SECOND call is fed the FIRST call's OUTPUT, which is exactly
	// what two map clicks in a row do. Feeding it a hand-written `"mid "` would
	// assert the rule against a string this file invented instead of against the
	// one the function produced.
	const FString AfterFirstClick =
		USiegeAssistantConsoleWidget::ComposeAppendedInput(FString(), FString(TEXT("mid")));

	const FString AfterSecondClick =
		USiegeAssistantConsoleWidget::ComposeAppendedInput(AfterFirstClick, FString(TEXT("hero")));

	TestEqualSensitive(TEXT("(e) ⭐ TWO CLICKS IN A ROW give \"mid hero \" — ⛔ NEVER \"mid  hero\". Rule 2's always-a-trailing-space is what makes rule 1 idempotent"),
		AfterSecondClick, FString(TEXT("mid hero ")));

	// A third click, because an off-by-one in the rule could survive two and not
	// three, and three clicks is an ordinary thing for a player to do.
	TestEqualSensitive(TEXT("(e) …and a THIRD click still adds exactly one separator and one trailing space"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(AfterSecondClick, FString(TEXT("own_castle"))),
		FString(TEXT("mid hero own_castle ")));

	// ── (f) ⛔ THE PLAYER'S INTERIOR TEXT IS BYTE-PRESERVED ───────────────────
	// ⛔ NO GLOBAL WHITESPACE NORMALISATION, NO RE-CASING, NO HEAD TRIM
	// (CONVENTIONS §31: a presentation layer may not repair its input). The input
	// below is deliberately ugly — leading whitespace, an interior double space,
	// and mixed case — and ⛔ ALL OF IT MUST SURVIVE UNTOUCHED.
	const FString MessyExisting(TEXT("  Send  10 FOOTMEN   to"));

	TestEqualSensitive(TEXT("(f) ⛔ Leading whitespace, an interior DOUBLE space and MIXED CASE all survive byte-for-byte — the half-typed sentence is the player's"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(MessyExisting, FString(TEXT("ancient_ground_far"))),
		FString(TEXT("  Send  10 FOOTMEN   to ancient_ground_far ")));

	// The structural form of the same claim: the existing text is a byte-exact
	// PREFIX of the result. ⛔ `Left()` + `TestEqualSensitive`, ⛔ never `==` or
	// `StartsWith`, both of which are case-INSENSITIVE on `FString` and would pass
	// on a result whose case had drifted.
	const FString MessyComposed =
		USiegeAssistantConsoleWidget::ComposeAppendedInput(MessyExisting, FString(TEXT("ancient_ground_far")));

	TestEqualSensitive(TEXT("(f) …stated structurally: the existing text is an unmodified, byte-exact PREFIX of the result"),
		MessyComposed.Left(MessyExisting.Len()), MessyExisting);

	// ── ⛔ THE INVARIANT RULE 2 CLAIMS: EXACTLY ONE TRAILING SPACE, ALWAYS ────
	// Asserted over every shape above at once, because "always" is the word rule 2
	// uses and a per-case assertion cannot say it.
	TArray<FString> ExistingShapes;
	ExistingShapes.Add(FString());                                  // empty
	ExistingShapes.Add(FString(TEXT("send 10 footmen to")));        // ends non-space
	ExistingShapes.Add(FString(TEXT("send 10 footmen to ")));       // ends space
	ExistingShapes.Add(FString(TEXT("send 10 footmen to\t")));      // ends tab
	ExistingShapes.Add(FString(TEXT("   ")));                       // whitespace only
	ExistingShapes.Add(MessyExisting);                              // messy

	for (int32 ShapeIndex = 0; ShapeIndex < ExistingShapes.Num(); ++ShapeIndex)
	{
		const FString Result = USiegeAssistantConsoleWidget::ComposeAppendedInput(
			ExistingShapes[ShapeIndex], FString(TEXT("hero")));

		TestTrue(*FString::Printf(TEXT("⛔ ALWAYS exactly one trailing space — the result is non-empty (shape %d)"), ShapeIndex),
			Result.Len() > 0);

		if (Result.Len() > 0)
		{
			TestEqualSensitive(*FString::Printf(TEXT("⛔ …the LAST character is a space (shape %d)"), ShapeIndex),
				Result.Right(1), FString(TEXT(" ")));
		}

		// ⛔ ONE space, not two: the character before the trailing space is part of
		// the symbol, so it is never itself a space. This is the assertion that
		// fails if rule 1 ever double-inserts.
		// ⚠️ WRITTEN AS TestFalse ON AN EXPLICIT bool RATHER THAN AS TestNotEqual ON
		// A TCHAR PAIR, DELIBERATELY: `TCHAR` converts implicitly to `float`, so the
		// non-template `TestNotEqual(const TCHAR*, float, float, float)` overload is
		// VIABLE there. The template still wins on conversion rank, but this file is
		// compiled exactly once (TASK-566) and a bool comparison has no candidate set
		// to reason about at all. The comparison itself is the shipped idiom
		// (SiegeAssistantGrammar.cpp, SiegeAssistantComponent.cpp).
		if (Result.Len() > 1)
		{
			TestFalse(*FString::Printf(TEXT("⛔ …and it is exactly ONE — the character before it belongs to the symbol, ⛔ never a second space (shape %d)"), ShapeIndex),
				Result[Result.Len() - 2] == TEXT(' '));
		}

		// ⛔ AND THE SYMBOL IS ACTUALLY IN THERE, CASE-SENSITIVELY. Without this a
		// function that returned the existing text plus a space would pass every
		// trailing-space claim above.
		TestTrue(*FString::Printf(TEXT("⛔ …and the symbol itself is present, case-sensitively (shape %d)"), ShapeIndex),
			Result.Contains(TEXT("hero"), ESearchCase::CaseSensitive));
	}

	// ── ⛔ THE SYMBOL IS OPAQUE — it is copied, ⛔ never interpreted ───────────
	// The function knows nothing about the place vocabulary and must not learn:
	// which symbols exist is `USiegeAssistantVocabulary`'s and which are clickable
	// is `UWarMapWidget`'s. A symbol it has never heard of composes identically.
	TestEqualSensitive(TEXT("⛔ The symbol is an OPAQUE string — an unknown one composes identically, because this function never consults the vocabulary"),
		USiegeAssistantConsoleWidget::ComposeAppendedInput(
			FString(TEXT("mid")), FString(TEXT("NOT_A_PLACE_SYMBOL"))),
		FString(TEXT("mid NOT_A_PLACE_SYMBOL ")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  24. ELEVATION (TASK-684, WM-§2) — the pinned pure ramp: floor ⇒ dark,
//      ceiling ⇒ light, above-ceiling ⇒ CLAMPED full white, mid ⇒ monotonic
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapHeightToBrightnessTest,
	"Siegebound.WarMap.HeightToBrightnessRampFloorCeilingAndClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapHeightToBrightnessTest::RunTest(const FString& Parameters)
{
	// ⭐ THE SEAM IS PURE (the W4-R1 doctrine WM-§2 cites): no world, no widget instance, no
	// CDO — three floats in, one float out. That purity is exactly what lets this file pin
	// the whole normalization law headlessly while the 7,800-trace capture itself stays a
	// live-editor concern (TASK-689/690's instrument, stated in the not-covered list above).
	constexpr float Tolerance = 1.e-6f;

	// The MEASURED shipped default (SM_Hill_02's 400 uu × the DA hill-scale max 2.5 —
	// derivation at the ElevationReliefCeiling declaration). ⚠️ Used here as an ARBITRARY
	// band value, transcribed the CanonicalPlaceSymbols way: the ramp must hold for ANY
	// ceiling, and the arithmetic below is exact at this one.
	constexpr float Ceiling = 1000.f;

	// ── (a) The FLOOR is dark — brightness exactly 0, at Z=0 and at a lifted ground ──────
	TestEqual(TEXT("(a) A hit AT the ground is brightness 0 (dark) when the ground is Z=0"),
		UWarMapWidget::HeightToBrightness(0.f, 0.f, Ceiling), 0.f, Tolerance);

	TestEqual(TEXT("(a) …and still 0 when the MEASURED ground is non-zero — the ramp is anchored at GroundZ, never at a transcribed Z=0"),
		UWarMapWidget::HeightToBrightness(137.5f, 137.5f, Ceiling), 0.f, Tolerance);

	// ── (b) The CEILING is light — brightness exactly 1 ──────────────────────────────────
	TestEqual(TEXT("(b) A hit AT GroundZ + ReliefCeiling is brightness 1 (light)"),
		UWarMapWidget::HeightToBrightness(Ceiling, 0.f, Ceiling), 1.f, Tolerance);

	TestEqual(TEXT("(b) …and the anchor moves WITH the ground"),
		UWarMapWidget::HeightToBrightness(137.5f + Ceiling, 137.5f, Ceiling), 1.f, Tolerance);

	// ── (c) ⭐ ABOVE the ceiling CLAMPS to full white — THE WM-§2 NORMALIZATION LAW ───────
	// A castle shell is ≈8,000 uu over a ≈1,000-uu hill band; without this clamp a min→max
	// normalization would crush every hill into one gray band and the layer would deliver
	// nothing of "lighter or darker depending on elevation". The clamp is the ruling.
	TestEqual(TEXT("(c) ⭐ A castle-class hit (8,000 uu) clamps to EXACTLY 1 — full white, never a rescale of the whole band"),
		UWarMapWidget::HeightToBrightness(8000.f, 0.f, Ceiling), 1.f, Tolerance);

	TestEqual(TEXT("(c) …and one hair above the ceiling already clamps"),
		UWarMapWidget::HeightToBrightness(Ceiling + 1.f, 0.f, Ceiling), 1.f, Tolerance);

	// ── (d) MID-BAND is linear — the quarter points land exactly ─────────────────────────
	TestEqual(TEXT("(d) Quarter height is brightness 0.25"),
		UWarMapWidget::HeightToBrightness(250.f, 0.f, Ceiling), 0.25f, Tolerance);
	TestEqual(TEXT("(d) Half height is brightness 0.5"),
		UWarMapWidget::HeightToBrightness(500.f, 0.f, Ceiling), 0.5f, Tolerance);
	TestEqual(TEXT("(d) Three-quarter height is brightness 0.75"),
		UWarMapWidget::HeightToBrightness(750.f, 0.f, Ceiling), 0.75f, Tolerance);

	// ── (e) MONOTONIC — lighter NEVER means lower (Jonathan's words, as an invariant) ────
	// Non-decreasing across the whole sweep (the clamped shoulders are flat), STRICTLY
	// increasing inside the open band — a ramp that plateaued mid-band would paint two
	// different heights the same gray and the map would lie by omission.
	{
		float PreviousBrightness = -1.f;

		for (int32 Step = 0; Step <= 60; ++Step)
		{
			// -500 … +2,500 in 50-uu steps: below-ground, the whole band, and past the clamp.
			const float HitZ = -500.f + static_cast<float>(Step) * 50.f;
			const float Brightness = UWarMapWidget::HeightToBrightness(HitZ, 0.f, Ceiling);

			TestTrue(*FString::Printf(TEXT("(e) Brightness is in [0,1] at Z=%.0f"), HitZ),
				Brightness >= 0.f && Brightness <= 1.f);

			TestTrue(*FString::Printf(TEXT("(e) Brightness never DECREASES with height (Z=%.0f)"), HitZ),
				Brightness >= PreviousBrightness);

			if (HitZ > 0.f && HitZ < Ceiling && PreviousBrightness >= 0.f)
			{
				TestTrue(*FString::Printf(TEXT("(e) …and strictly INCREASES inside the open band (Z=%.0f)"), HitZ),
					Brightness > PreviousBrightness);
			}

			PreviousBrightness = Brightness;
		}
	}

	// ── (f) BELOW ground clamps dark — a pit cannot underflow the ramp ───────────────────
	TestEqual(TEXT("(f) A hit BELOW the measured ground clamps to 0, never negative"),
		UWarMapWidget::HeightToBrightness(-400.f, 0.f, Ceiling), 0.f, Tolerance);

	// ── (g) ⛔ THE ZERO-DIVIDE FLOOR — a degenerate ceiling is total, never NaN ───────────
	// ElevationReliefCeiling is an EditDefaultsOnly float a designer can zero; the seam
	// floors it at 1 uu (the MinArenaHalfExtentUu doctrine), so every output stays finite
	// and clamped. 0.5 over a zeroed ceiling ⇒ 0.5 / 1 = 0.5 exactly.
	{
		const float DegenerateZero = UWarMapWidget::HeightToBrightness(0.5f, 0.f, 0.f);
		TestTrue(TEXT("(g) A ZERO ceiling yields a FINITE brightness"), FMath::IsFinite(DegenerateZero));
		TestEqual(TEXT("(g) …floored at 1 uu: 0.5 over a zeroed ceiling is 0.5"), DegenerateZero, 0.5f, Tolerance);

		const float DegenerateNegative = UWarMapWidget::HeightToBrightness(2.f, 0.f, -5.f);
		TestTrue(TEXT("(g) A NEGATIVE ceiling yields a FINITE brightness"), FMath::IsFinite(DegenerateNegative));
		TestEqual(TEXT("(g) …and clamps at 1 like any above-ceiling hit"), DegenerateNegative, 1.f, Tolerance);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  25. ELEVATION (TASK-684) — MapUVToWorld is the EXACT inverse of the shipped
//      projection, so the bake's sample lattice and the painter's texels agree
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapUvToWorldInverseTest,
	"Siegebound.WarMap.MapUvToWorldInvertsTheProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapUvToWorldInverseTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐ WHY THIS TEST EXISTS: the elevation bake generates ONE world sample point per texel
	// through MapUVToWorld. If the inverse drifted from WorldToMapUV by even a sign, every
	// hill would render at a plausible WRONG place — most likely mirrored about the lane,
	// exactly the defect the orientation-pinning tests above exist to kill in the forward
	// direction. The round trip below closes the loop in the backward one.

	const FVector2D CleanArena = CleanArenaHalfExtent();
	const FVector2D ShippedArena = ShippedCdoArenaHalfExtent();

	// ── (a) The centre and the four corners, both arenas ─────────────────────────────────
	{
		const FVector2D Centre = FSiegeWarMapProjection::MapUVToWorld(FVector2D(0.5, 0.5), CleanArena);
		TestEqual(TEXT("(a) UV (0.5, 0.5) is the world origin — X"), Centre.X, 0.0, UvTolerance);
		TestEqual(TEXT("(a) UV (0.5, 0.5) is the world origin — Y"), Centre.Y, 0.0, UvTolerance);

		// ⭐ The Y row carries the TASK-692 convention (`WM-§7`): UV.Y = 0 is the TOP of the
		// map, which is world −HalfY (world +Y grows DOWN on screen, no inversion) — the
		// same contract test 1 pins forward.
		const FVector2D TopLeft = FSiegeWarMapProjection::MapUVToWorld(FVector2D(0.0, 0.0), CleanArena);
		TestEqual(TEXT("(a) UV (0,0) — the map's top-left — is world (-HalfX, -HalfY) — X"), TopLeft.X, -CleanArena.X, UvTolerance);
		TestEqual(TEXT("(a) UV (0,0) — the map's top-left — is world (-HalfX, -HalfY) — Y"), TopLeft.Y, -CleanArena.Y, UvTolerance);

		const FVector2D BottomRight = FSiegeWarMapProjection::MapUVToWorld(FVector2D(1.0, 1.0), CleanArena);
		TestEqual(TEXT("(a) UV (1,1) — the map's bottom-right — is world (+HalfX, +HalfY) — X"), BottomRight.X, CleanArena.X, UvTolerance);
		TestEqual(TEXT("(a) UV (1,1) — the map's bottom-right — is world (+HalfX, +HalfY) — Y"), BottomRight.Y, CleanArena.Y, UvTolerance);

		const FVector2D ShippedTopLeft = FSiegeWarMapProjection::MapUVToWorld(FVector2D(0.0, 0.0), ShippedArena);
		TestEqual(TEXT("(a) Against the SHIPPED CDO extent too — X"), ShippedTopLeft.X, -ShippedArena.X, UvTolerance);
		TestEqual(TEXT("(a) Against the SHIPPED CDO extent too — Y"), ShippedTopLeft.Y, -ShippedArena.Y, UvTolerance);
	}

	// ── (b) ⭐ THE ROUND TRIP, ON THE BAKE'S OWN LATTICE ─────────────────────────────────
	// Texel centres (i + 0.5) / N — the exact expression BakeElevationTexture evaluates —
	// through MapUVToWorld and back through the SHIPPED WorldToMapUV. In-range UVs never
	// engage the forward clamp, so the trip must close to tolerance on BOTH arenas.
	{
		constexpr int32 LatticeX = 13;
		constexpr int32 LatticeY = 6;

		const FVector2D Arenas[] = { CleanArena, ShippedArena };
		const TCHAR* ArenaNames[] = { TEXT("clean"), TEXT("shipped-CDO") };

		for (int32 ArenaIndex = 0; ArenaIndex < 2; ++ArenaIndex)
		{
			for (int32 Iy = 0; Iy < LatticeY; ++Iy)
			{
				for (int32 Ix = 0; Ix < LatticeX; ++Ix)
				{
					const FVector2D Uv(
						(static_cast<double>(Ix) + 0.5) / static_cast<double>(LatticeX),
						(static_cast<double>(Iy) + 0.5) / static_cast<double>(LatticeY));

					const FVector2D RoundTrip = FSiegeWarMapProjection::WorldToMapUV(
						FSiegeWarMapProjection::MapUVToWorld(Uv, Arenas[ArenaIndex]),
						Arenas[ArenaIndex]);

					TestEqual(*FString::Printf(TEXT("(b) Round trip closes at texel (%d,%d) on the %s arena — U"), Ix, Iy, ArenaNames[ArenaIndex]),
						RoundTrip.X, Uv.X, UvTolerance);
					TestEqual(*FString::Printf(TEXT("(b) Round trip closes at texel (%d,%d) on the %s arena — V"), Ix, Iy, ArenaNames[ArenaIndex]),
						RoundTrip.Y, Uv.Y, UvTolerance);
				}
			}
		}
	}

	// ── (c) ⛔ THE DEGENERATE EXTENT — same 1-uu floor as the forward map, no NaN ────────
	{
		const FVector2D Degenerate = FSiegeWarMapProjection::MapUVToWorld(FVector2D(0.25, 0.75), FVector2D::ZeroVector);
		TestTrue(TEXT("(c) A (0,0) extent yields FINITE world coordinates"),
			FMath::IsFinite(Degenerate.X) && FMath::IsFinite(Degenerate.Y));
		TestEqual(TEXT("(c) …floored at 1 uu — X = (2·0.25 − 1)·1"), Degenerate.X, -0.5, UvTolerance);
		TestEqual(TEXT("(c) …floored at 1 uu — Y = (2·0.75 − 1)·1"), Degenerate.Y, 0.5, UvTolerance);

		const FVector2D DegenerateTrip = FSiegeWarMapProjection::WorldToMapUV(Degenerate, FVector2D::ZeroVector);
		TestEqual(TEXT("(c) …and the round trip STILL closes, because both directions floor the SAME axes the SAME way — U"),
			DegenerateTrip.X, 0.25, UvTolerance);
		TestEqual(TEXT("(c) …and the round trip STILL closes — V"),
			DegenerateTrip.Y, 0.75, UvTolerance);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  26. POI ICONS (TASK-685) — every icon centre the layer can produce lands
//      INSIDE the map rect, because the icons ride the SHIPPED projection chain
//      (clamp included) — the bake's lattice idiom, pointed at the icon claim
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapPoiIconProjectionTest,
	"Siegebound.WarMap.PoiIconProjectionStaysInsideTheMapRect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapPoiIconProjectionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeWarMapTestFixture;

	// ⭐ WHY THIS TEST EXISTS: TASK-685's POI icon layer projects LIVE ACTOR positions
	// (gold-node mines, ancient grounds, castles) through the SHIPPED
	// WorldToMapUV → MapUVToLocal chain — the same two calls the dots make, ⛔ never a
	// re-derived transform (WM-§1). The property the layer stands on is that EVERY world
	// position — including a fixture past the configured arena bound, which the forward
	// clamp pins to the edge — produces an icon centre INSIDE the drawn rect, because a
	// centre outside it would paint POI chrome over the WBP's panel border. The lattice
	// below is the elevation bake's (i + 0.5)/N texel idiom (test 25), swept over a world
	// span 1.5× the arena so the OUTER RING of samples exercises the clamp on every edge.
	//
	// ⛔ WHAT THIS TEST DELIBERATELY DOES NOT DO: iterate actors, build a world, or assert
	// the census (the WAVE-2 precedent the spec re-cites: that would test the engine's
	// iterator, not our logic — TASK-690's eye is that instrument). Geometry only.

	const FVector2D PanelSize(1920.0, 1080.0);

	const FVector2D Arenas[] = { CleanArenaHalfExtent(), ShippedCdoArenaHalfExtent() };
	const TCHAR* ArenaNames[] = { TEXT("clean"), TEXT("shipped-CDO") };

	for (int32 ArenaIndex = 0; ArenaIndex < 2; ++ArenaIndex)
	{
		const FVector2D Arena = Arenas[ArenaIndex];

		FVector2D RectOrigin = FVector2D::ZeroVector;
		FVector2D RectSize = FVector2D::ZeroVector;
		FSiegeWarMapProjection::ComputeMapRectLocal(
			PanelSize, static_cast<float>(ShippedMapPaddingPx), Arena, RectOrigin, RectSize);

		TestTrue(*FString::Printf(TEXT("Precondition: the %s arena yields a non-degenerate rect on a 1920x1080 panel"), ArenaNames[ArenaIndex]),
			RectSize.X > 0.0 && RectSize.Y > 0.0);

		// ── (a) The lattice sweep at 1.5× the arena span — in-arena AND out-of-arena ─────
		{
			constexpr int32 LatticeX = 13;
			constexpr int32 LatticeY = 6;
			constexpr double SpanScale = 1.5;

			for (int32 Iy = 0; Iy < LatticeY; ++Iy)
			{
				for (int32 Ix = 0; Ix < LatticeX; ++Ix)
				{
					// Lattice centre in [-SpanScale, +SpanScale] of each half-extent — the
					// bake's texel-centre expression, widened past the bound on purpose.
					const FVector2D WorldXY(
						(2.0 * ((static_cast<double>(Ix) + 0.5) / static_cast<double>(LatticeX)) - 1.0) * Arena.X * SpanScale,
						(2.0 * ((static_cast<double>(Iy) + 0.5) / static_cast<double>(LatticeY)) - 1.0) * Arena.Y * SpanScale);

					const FVector2D IconCentre = FSiegeWarMapProjection::MapUVToLocal(
						FSiegeWarMapProjection::WorldToMapUV(WorldXY, Arena), RectOrigin, RectSize);

					// Boundary INCLUSIVE: an edge-pinned icon centre ON the rect border is
					// the clamp working as specified, not a leak.
					TestTrue(*FString::Printf(TEXT("(a) Lattice (%d,%d) on the %s arena projects INSIDE the rect"), Ix, Iy, ArenaNames[ArenaIndex]),
						IconCentre.X >= RectOrigin.X - PxTolerance
						&& IconCentre.X <= RectOrigin.X + RectSize.X + PxTolerance
						&& IconCentre.Y >= RectOrigin.Y - PxTolerance
						&& IconCentre.Y <= RectOrigin.Y + RectSize.Y + PxTolerance);
				}
			}
		}

		// ── (b) Closed-form spot pins on POI-shaped positions ────────────────────────────
		{
			// A mid-field fixture at the world origin (the two neutral mines' neighbourhood)
			// is the exact rect centre.
			const FVector2D MidField = FSiegeWarMapProjection::MapUVToLocal(
				FSiegeWarMapProjection::WorldToMapUV(FVector2D(0.0, 0.0), Arena), RectOrigin, RectSize);
			TestEqual(*FString::Printf(TEXT("(b) A world-origin POI is the rect centre on the %s arena — X"), ArenaNames[ArenaIndex]),
				MidField.X, RectOrigin.X + RectSize.X * 0.5, PxTolerance);
			TestEqual(*FString::Printf(TEXT("(b) A world-origin POI is the rect centre on the %s arena — Y"), ArenaNames[ArenaIndex]),
				MidField.Y, RectOrigin.Y + RectSize.Y * 0.5, PxTolerance);

			// A castle-shaped fixture at the arena's (-HalfX, -HalfY) corner is the rect's
			// top-left — the Y row carries the TASK-692 convention (world +Y = screen DOWN,
			// `WM-§7`).
			const FVector2D CornerCastle = FSiegeWarMapProjection::MapUVToLocal(
				FSiegeWarMapProjection::WorldToMapUV(FVector2D(-Arena.X, -Arena.Y), Arena), RectOrigin, RectSize);
			TestEqual(*FString::Printf(TEXT("(b) A (-HalfX, -HalfY) POI is the rect top-left on the %s arena — X"), ArenaNames[ArenaIndex]),
				CornerCastle.X, RectOrigin.X, PxTolerance);
			TestEqual(*FString::Printf(TEXT("(b) A (-HalfX, -HalfY) POI is the rect top-left on the %s arena — Y"), ArenaNames[ArenaIndex]),
				CornerCastle.Y, RectOrigin.Y, PxTolerance);

			// A fixture far PAST the opposite bound pins EXACTLY to the bottom-right corner
			// — the clamp is a pin, not an ejection, so a wandered-out POI stays visible at
			// the edge instead of drawing over the WBP chrome (the WorldToMapUV contract the
			// icon layer inherits without writing a line of its own).
			const FVector2D FarOut = FSiegeWarMapProjection::MapUVToLocal(
				FSiegeWarMapProjection::WorldToMapUV(FVector2D(Arena.X * 3.0, Arena.Y * 4.0), Arena), RectOrigin, RectSize);
			TestEqual(*FString::Printf(TEXT("(b) A far-out-of-arena POI pins to the rect bottom-right on the %s arena — X"), ArenaNames[ArenaIndex]),
				FarOut.X, RectOrigin.X + RectSize.X, PxTolerance);
			TestEqual(*FString::Printf(TEXT("(b) A far-out-of-arena POI pins to the rect bottom-right on the %s arena — Y"), ArenaNames[ArenaIndex]),
				FarOut.Y, RectOrigin.Y + RectSize.Y, PxTolerance);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  27. ELEVATION COLOUR (TASK-722, WM-§8a/§8b) — the ramp is GREEN, runs dark to
//      light, is monotonic, and its lerp lives in the DISPLAY space its bytes
//      are written in
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeWarMapBrightnessToRampColorTest,
	"Siegebound.WarMap.BrightnessToRampColorIsGreenDarkToLightAndMonotonic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeWarMapBrightnessToRampColorTest::RunTest(const FString& Parameters)
{
	// ⭐ THE SECOND PURE SEAM OF THE ELEVATION LAYER. `WM-§2` split the ramp's SHAPE
	// (`HeightToBrightness`, section 24 above) from its SCREEN MAPPING; TASK-722 turned the
	// map from gray to green by changing ONLY the mapping, so section 24 is byte-untouched
	// and this section is the whole of the new claim. One float in, one FColor out — no
	// world, no widget instance, no CDO, exactly like its sibling.
	//
	// ⚠️ A WORDING NOTE, so nobody reads a contradiction: section 24 still says brightness 1
	// clamps to "full white". That phrase now describes the ramp's LIGHT GREEN end.
	// `HeightToBrightness` and its documentation were left byte-untouched on purpose
	// (`WM-§8a` item 1 outranks a wording repair); it is flagged in
	// handoffs/TASK-722-programmer.md, not silently edited.
	//
	// ⭐⭐ EVERY CLAIM BELOW IS A RELATIONSHIP, ⛔ NEVER A LITERAL. The two grass constants
	// are file-local to WarMapWidget.cpp and were MEASURED off the shipped ground material
	// (`WM-§8b`, TASK-721) — if this file re-typed them, it would be a test transcribed from
	// its own subject, which is a guardrail that reports SAFE no matter what the subject
	// does (the standing lesson). ⇒ Everything here holds for ANY dark-green/light-green
	// pair, and fails for a gray one.

	const FColor Dark  = UWarMapWidget::BrightnessToRampColor(0.f);
	const FColor Light = UWarMapWidget::BrightnessToRampColor(1.f);

	// ── (a) THE ENDS ARE THE ENDS — "darker green to lighter green" as an invariant ───────
	TestTrue(TEXT("(a) Brightness 0 is STRICTLY DARKER than brightness 1 on EVERY channel — the ramp runs dark green → light green"),
		Dark.R < Light.R && Dark.G < Light.G && Dark.B < Light.B);

	// ⛔ TOTAL IN BOTH DIRECTIONS. The seam clamps, so no input can produce a colour that is
	// not ON the ramp. HeightToBrightness (its only shipped caller) already returns [0,1] and
	// section 24(g) proves it never returns NaN — but this seam is public and does not trust
	// its caller, and this pair is what pins that.
	TestTrue(TEXT("(a) A below-zero brightness clamps to the SAME texel as 0"),
		UWarMapWidget::BrightnessToRampColor(-5.f) == Dark);
	TestTrue(TEXT("(a) An above-one brightness clamps to the SAME texel as 1"),
		UWarMapWidget::BrightnessToRampColor(5.f) == Light);

	// ── (b) OPAQUE at both ends — a transparent texel would punch a hole in the map ───────
	TestEqual(TEXT("(b) The dark end is fully opaque"), static_cast<int32>(Dark.A), 255);
	TestEqual(TEXT("(b) The light end is fully opaque"), static_cast<int32>(Light.A), 255);

	// ── (c) GREEN-DOMINANT EVERYWHERE + (d) MONOTONIC ────────────────────────────────────
	// ⭐ (c) is the claim that catches the two ways this task could have shipped wrong and
	// still compiled: a REVERTED luma ramp (R == G == B) and a TRANSPOSED FColor constructor
	// argument — FColor's ctor takes R,G,B,A while its memory layout is B,G,R,A, and an R/G
	// or G/B swap inverts the dominance outright.
	// ⚠️ STATED HONESTLY: it CANNOT catch an R/B swap. The measured grass chromaticity is
	// only very slightly red-over-blue, so R and B differ by ONE byte at the light end and
	// tie at the dark end. Block (f) is the only guard on that case, and it is weak by
	// nature — the real control there is the comment at the seam.
	{
		FColor PreviousColor = FColor(0, 0, 0, 0);
		float PreviousLuminance = -1.f;

		constexpr int32 StepCount = 20; // 0.00 … 1.00 in 0.05 steps

		for (int32 Step = 0; Step <= StepCount; ++Step)
		{
			const float Brightness = static_cast<float>(Step) / static_cast<float>(StepCount);
			const FColor Color = UWarMapWidget::BrightnessToRampColor(Brightness);

			TestTrue(*FString::Printf(TEXT("(c) GREEN dominates RED at brightness %.2f"), Brightness),
				Color.G > Color.R);
			TestTrue(*FString::Printf(TEXT("(c) GREEN dominates BLUE at brightness %.2f"), Brightness),
				Color.G > Color.B);
			TestTrue(*FString::Printf(TEXT("(c) ⛔ …so the texel is NOT GRAY at brightness %.2f — a reverted luma ramp fails right here"), Brightness),
				!(Color.R == Color.G && Color.G == Color.B));

			// (d) MONOTONIC — "lighter never means lower" (Jonathan's instruction, as an
			// invariant a test can hold forever). Non-decreasing per channel, and STRICTLY
			// increasing in WCAG relative luminance, computed on the bytes.
			const float Luminance =
				0.2126f * static_cast<float>(Color.R)
				+ 0.7152f * static_cast<float>(Color.G)
				+ 0.0722f * static_cast<float>(Color.B);

			if (Step > 0)
			{
				TestTrue(*FString::Printf(TEXT("(d) No channel DECREASES as brightness rises (brightness %.2f)"), Brightness),
					Color.R >= PreviousColor.R && Color.G >= PreviousColor.G && Color.B >= PreviousColor.B);

				TestTrue(*FString::Printf(TEXT("(d) …and LUMINANCE strictly INCREASES (brightness %.2f) — a plateau would paint two heights the same green"), Brightness),
					Luminance > PreviousLuminance);
			}

			PreviousColor = Color;
			PreviousLuminance = Luminance;
		}
	}

	// ── (e) ⭐⭐ THE MAPPING IS AFFINE IN THE SPACE IT WRITES ─────────────────────────────
	// THE CLAIM THAT CATCHES THE COLOUR-SPACE TRAP, and the reason this test exists at all.
	// The ramp's bytes are sRGB-ENCODED (the bake's texture is created with SRGB = true) and
	// the lerp runs in that SAME space, so the MIDPOINT byte must be the average of the two
	// END bytes on every channel. If a future edit "corrects" the seam to lerp in LINEAR
	// space and re-encode — a change that looks like a bug fix, compiles perfectly and
	// reviews as more correct — the midpoint's display value jumps from 0.55 to 0.73 and all
	// three channels fail here at once.
	// ⛔ Nothing is transcribed: the expected midpoint is COMPUTED from the measured ends.
	{
		const FColor Mid = UWarMapWidget::BrightnessToRampColor(0.5f);

		// Half-up rounding, three independent channels, integer-halved endpoints — one byte
		// of slack is arithmetic, not tolerance for a wrong space (a linear-space lerp misses
		// by ~46 bytes on green).
		constexpr int32 RoundingSlackBytes = 1;

		const int32 ExpectedR = (static_cast<int32>(Dark.R) + static_cast<int32>(Light.R)) / 2;
		const int32 ExpectedG = (static_cast<int32>(Dark.G) + static_cast<int32>(Light.G)) / 2;
		const int32 ExpectedB = (static_cast<int32>(Dark.B) + static_cast<int32>(Light.B)) / 2;

		TestTrue(TEXT("(e) ⭐ The MIDPOINT's RED is the average of the two ends — the lerp is in the sRGB space the bytes are written in"),
			FMath::Abs(static_cast<int32>(Mid.R) - ExpectedR) <= RoundingSlackBytes);
		TestTrue(TEXT("(e) ⭐ …and its GREEN is too — a linear-space lerp would land ~46 bytes high here"),
			FMath::Abs(static_cast<int32>(Mid.G) - ExpectedG) <= RoundingSlackBytes);
		TestTrue(TEXT("(e) ⭐ …and its BLUE is too"),
			FMath::Abs(static_cast<int32>(Mid.B) - ExpectedB) <= RoundingSlackBytes);
	}

	// ── (f) ⚠️ THE R/B TRANSPOSITION GUARD — A ONE-BYTE MARGIN, AND IT SAYS SO ────────────
	// The measured grass chromaticity is very slightly RED-over-BLUE, which at the light end
	// is 160 vs 159 and at the dark end is a tie. ⛔ This is a WEAK guard and it is NOT the
	// primary control against a swapped channel — the primary control is the comment at
	// `BrightnessToRampColor`, which says in words that the ramp's own pixels must never be
	// used as the channel-order check. It is here because it is free, and because a swap
	// that survives (c) has nowhere else to be caught.
	TestTrue(TEXT("(f) The LIGHT end is red-over-blue, as the measured grass sample is (R >= B)"),
		Light.R >= Light.B);
	TestTrue(TEXT("(f) …and the DARK end never inverts it either"),
		Dark.R >= Dark.B);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
