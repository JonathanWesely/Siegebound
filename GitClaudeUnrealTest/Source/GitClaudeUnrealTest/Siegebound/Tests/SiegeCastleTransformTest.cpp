// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Siegebound/Castle.h"
#include "Siegebound/TeamId.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ THE CASTLE-ROTATION TRANSFORM PIN (batch CASTLE-ROTATION, TASK-665) ═══
 *
 *  ONE job: make an ACCIDENTAL UN-ROTATION of the castles FAIL LOUDLY. The
 *  CASTLE-ROTATION wave (CONVENTIONS ROT-§0..§4, Jonathan's directive 2026-08-27)
 *  turned both gates toward the battlefield centre: Castle Blue (−25000, 0, 0)
 *  yaw **+90** (gate → world +X), Castle Red (+25000, 0, 0) yaw **−90** (gate →
 *  world −X) — the ROT-§1 expected transforms, measured live before the one
 *  ordered save (TASK-662, ledger ROT-§2) and re-measured by the TASK-663
 *  battery. An enormous amount of downstream truth now keys on those two poses
 *  (the WR-§2b spawn facing, the bot's gate-front wave anchor, the F1 furnishing
 *  world positions, the walkability record). A future editor session that
 *  reverts, re-imports, or hand-nudges either castle back toward yaw 0 would
 *  silently re-falsify all of it — THIS test is the tripwire.
 *
 *  ⭐ WHY A NEW FILE (the "extend, never a parallel new frame unless none fits"
 *  law — this is the declared "none fits" case): every existing Tests/ file is
 *  subject-scoped to pure statics / CDO reads, and SiegeWarMapTest.cpp's charter
 *  states in bold that a test needing a world there is a FINDING. This test's
 *  entire subject IS level data — it must load the L_Arena world asset. Same
 *  frame as the rest of the suite (the same simple-automation-test macro,
 *  EditorContext | EngineFilter, "Siegebound.*" name — the macro token appears
 *  exactly ONCE in this file so a grep census counts one test), one new file
 *  for the one level-data-reading subject.
 *
 *  MECHANISM — read-only, ⛔ zero writes, ⛔ zero network, ⛔ no PIE, no editor
 *  mutation: LoadObject<UWorld> on /Game/Maps/L_Arena (returns the live editor
 *  world when the map is open, or loads the serialized package when it is not —
 *  never opens/saves/marks anything dirty), then read each ACastle's ROOT
 *  COMPONENT RelativeLocation/RelativeRotation. For a level-placed actor whose
 *  root has NO attach parent, relative IS world — and unlike
 *  GetActorLocation()/GetActorRotation() (which read the transient
 *  ComponentToWorld, only valid after registration), the relative fields are
 *  serialized UPROPERTYs, valid in BOTH load states. The no-attach-parent
 *  precondition is asserted, not assumed. Team comes from ACastle::GetTeamId(),
 *  a plain accessor of the serialized, per-instance Team UPROPERTY.
 *
 *  WHAT THIS DOES NOT COVER (SC-§32 — green here is not "the gates work"):
 *  actor poses only. Gate-corridor walkability, nav, GateBlocker arming and the
 *  spawn facing are runtime behaviors — the TASK-663 battery + TASK-667's live
 *  re-verify own those. This file pins the LEVEL DATA those verdicts stand on.
 */

namespace SiegeCastleTransformTestFixture
{
	/** ROT-§1 expected transforms (measured live by TASK-662 pre-save, re-measured by TASK-663; ledger ROT-§2). */
	constexpr double ExpectedBlueX = -25000.0;
	constexpr double ExpectedRedX = 25000.0;
	constexpr double ExpectedY = 0.0;
	constexpr double ExpectedZ = 0.0;
	constexpr double ExpectedBlueYaw = 90.0;  // gate (castle-local −Y, the 617 C1 datum) → world +X
	constexpr double ExpectedRedYaw = -90.0;  // gate → world −X

	/** Location tolerance (uu): the castles sit on exact authored coordinates; 1 uu absorbs float↔double serialization noise only. */
	constexpr double LocationTolerance = 1.0;

	/** Angle tolerance (deg): the yaws were typed, not simulated; 0.1° absorbs serialization noise only. */
	constexpr double AngleTolerance = 0.1;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastleRotatedTransformPinTest,
	"Siegebound.Castle.RotatedTransformPin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastleRotatedTransformPinTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastleTransformTestFixture;

	// ── Load the arena world asset (read-only; the live editor world when open,
	//    the serialized package otherwise — both states are handled below).
	UWorld* ArenaWorld = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/L_Arena.L_Arena"));
	if (!ArenaWorld)
	{
		AddError(TEXT("/Game/Maps/L_Arena did not load as a UWorld — the arena map is missing or renamed, and the castle transform pin cannot run."));
		return false;
	}

	ULevel* Level = ArenaWorld->PersistentLevel;
	if (!Level)
	{
		AddError(TEXT("L_Arena loaded but has no PersistentLevel — cannot enumerate castles."));
		return false;
	}

	// ── Find exactly one castle per team among the persistent level's actors.
	const ACastle* BlueCastle = nullptr;
	const ACastle* RedCastle = nullptr;
	int32 CastleCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		const ACastle* Castle = Cast<ACastle>(Actor);
		if (!Castle)
		{
			continue;
		}
		++CastleCount;
		if (Castle->GetTeamId() == ETeamId::Blue)
		{
			TestNull(TEXT("L_Arena holds at most ONE Blue castle"), BlueCastle);
			BlueCastle = Castle;
		}
		else
		{
			TestNull(TEXT("L_Arena holds at most ONE Red castle"), RedCastle);
			RedCastle = Castle;
		}
	}

	TestEqual(TEXT("L_Arena holds exactly TWO castles"), CastleCount, 2);
	if (!BlueCastle || !RedCastle)
	{
		AddError(FString::Printf(TEXT("Castle census failed (Blue %s, Red %s) — the transform pin cannot proceed."),
			BlueCastle ? TEXT("found") : TEXT("MISSING"), RedCastle ? TEXT("found") : TEXT("MISSING")));
		return false;
	}

	// ── Pin one castle's pose against the ROT-§1 expectation.
	const auto PinCastle = [this](const ACastle* Castle, const TCHAR* Label, double ExpectedX, double ExpectedYaw) -> void
	{
		const USceneComponent* Root = Castle->GetRootComponent();
		if (!Root)
		{
			AddError(FString::Printf(TEXT("%s castle '%s' has no root component — cannot read its pose."), Label, *GetNameSafe(Castle)));
			return;
		}

		// Relative == world only for an unattached root — assert the precondition
		// instead of assuming it (see the file header for why relative, not
		// GetActorLocation: the relative fields are serialized and valid even on a
		// loaded-but-unregistered level).
		TestNull(FString::Printf(TEXT("%s castle root has NO attach parent (relative == world)"), Label), Root->GetAttachParent());

		const FVector Location = Root->GetRelativeLocation();
		const FRotator Rotation = Root->GetRelativeRotation();
		AddInfo(FString::Printf(TEXT("%s castle '%s' measured at (%.2f, %.2f, %.2f) rot (pitch %.2f, yaw %.2f, roll %.2f)."),
			Label, *GetNameSafe(Castle), Location.X, Location.Y, Location.Z, Rotation.Pitch, Rotation.Yaw, Rotation.Roll));

		TestEqual(FString::Printf(TEXT("%s castle X"), Label), Location.X, ExpectedX, LocationTolerance);
		TestEqual(FString::Printf(TEXT("%s castle Y"), Label), Location.Y, ExpectedY, LocationTolerance);
		TestEqual(FString::Printf(TEXT("%s castle Z"), Label), Location.Z, ExpectedZ, LocationTolerance);

		// Yaw compared through NormalizeAxis so an equivalent representation
		// (e.g. 270 for −90) passes and only a REAL pose change fails.
		TestEqual(FString::Printf(TEXT("%s castle yaw (normalized delta from ROT-§1 expectation %.0f)"), Label, ExpectedYaw),
			FRotator::NormalizeAxis(Rotation.Yaw - ExpectedYaw), 0.0, AngleTolerance);
		TestEqual(FString::Printf(TEXT("%s castle pitch"), Label), FRotator::NormalizeAxis(Rotation.Pitch), 0.0, AngleTolerance);
		TestEqual(FString::Printf(TEXT("%s castle roll"), Label), FRotator::NormalizeAxis(Rotation.Roll), 0.0, AngleTolerance);

		// ── The INVARIANT behind the numbers, asserted independently of the pinned
		//    yaw (a sign-error tripwire): the gate (castle-local −Y, the 617 C1
		//    datum) must open TOWARD the centerline. Under yaw θ, local (0,−1,0) →
		//    world (sin θ, −cos θ, 0); its X component must point opposite the
		//    castle's own X side. Derived from the MEASURED yaw, so this fails on
		//    any un-rotation even if someone edits the pinned constants above.
		const double YawRad = FMath::DegreesToRadians(Rotation.Yaw);
		const double GateWorldX = FMath::Sin(YawRad);
		const double TowardCenterline = (Location.X <= 0.0) ? 1.0 : -1.0;
		TestTrue(FString::Printf(TEXT("%s castle gate opens toward the battlefield centre (gate-dir X %.3f, centerline sign %+.0f)"), Label, GateWorldX, TowardCenterline),
			GateWorldX * TowardCenterline > 0.99);
	};

	PinCastle(BlueCastle, TEXT("Blue"), ExpectedBlueX, ExpectedBlueYaw);
	PinCastle(RedCastle, TEXT("Red"), ExpectedRedX, ExpectedRedYaw);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
