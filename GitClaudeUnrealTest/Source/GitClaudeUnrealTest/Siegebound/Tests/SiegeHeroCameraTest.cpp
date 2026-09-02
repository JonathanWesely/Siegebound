// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "GameFramework/SpringArmComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/HeroCharacter.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE HERO'S THIRD-PERSON CAMERA AT THE LADDER ═══
 *  ═══ TASK-790 · VID-004 row `V2` · law `VIS-§3` (ruling `VIS-R2`) ════════
 *
 *  Jonathan filed it under "some minor visual things to fix". It is the one that most affects
 *  PLAY: at 01:12.0–01:13.5 the camera collapsed into the pawn for ≈2 s — back and hips at
 *  60–70 % of frame — blinding the player at the exact moment they commit to a climb, and costing
 *  the footage analyst the attach instant itself.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ THE MECHANISM, NAMED — AND IT IS ⛔ NOT CLIMB CODE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  `GitClaudeUnrealTestCharacter.cpp:39-42` builds `CameraBoom` with `TargetArmLength = 400` and
 *  configures ⛔ nothing else ⇒ `bDoCollisionTest` / `ProbeSize` / `ProbeChannel` sit at engine
 *  defaults (`SpringArmComponent.cpp:84-86`). `SpringArmComponent.cpp:197` sphere-sweeps the arm
 *  every frame and `:201` (`BlendLocations`) hands the socket STRAIGHT to the hit location. A
 *  400 uu boom against a 1200 uu tower face therefore resolves to the standoff distance — tens of
 *  uu — which IS the camera in the pawn. `AHeroCharacter` had ⛔ zero camera code before TASK-790,
 *  so the behaviour is ⛔ GENERIC: any tall blocker does it. The ladder is only where the player
 *  is REQUIRED to stand flush against one.
 *
 *  ⛔⛔ AND THE ENGINE HAS ⛔ NO IGNORE-ACTOR LIST: `SpringArmComponent.cpp:194` builds its query
 *  as `FCollisionQueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner())` — the owner and
 *  nothing else, with no hook to extend it. So the fix is a FLOOR under the probe's answer,
 *  scoped to climbable geometry, with the probe left ON.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ WHY THE SHAPE OF THIS FILE — A MEASUREMENT, ⛔ NOT A PREFERENCE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  Every automation test in this project is HEADLESS (`SiegeLadderClimbTest.cpp:39`: not one
 *  `UWorld::CreateWorld`, not one `SpawnActor` in `Siegebound/Tests/`), and a world-less
 *  `AHeroCharacter` cannot be ticked. ⇒ the DECISION is asserted against the pure static the
 *  runtime consumes, the CONFIGURATION is asserted against the CDO the constructor produced, and
 *  the two claims that are STRUCTURAL — "the fence held" and "nothing was added to the ten-exit
 *  teardown" — are asserted by reading the shipped source between named function boundaries
 *  (the `SiegeRecallTest.cpp` test 12 / `SiegeHeroLadderClimbTest.cpp` precedent).
 *
 *  ⚠️ AND THE LESSON THIS FILE IS WRITTEN AGAINST (`SHIP-§9c`): AN ASSERTION WHOSE TWO SIDES ARE
 *  EQUAL BY CONSTRUCTION PROVES NOTHING. FIVE assertions in this project stopped discriminating
 *  this week. Every probe below either carries a POSITIVE CONTROL that fails when the probe goes
 *  stale, or re-derives its expectation from the law's own arithmetic instead of transcribing it
 *  from the subject. The `⭐ CONTROL` comments mark them.
 *
 *  ⛔⛔ WHAT THIS FILE DOES ⛔ NOT CLAIM, SAID OUT LOUD: it does ⛔ NOT prove the camera looks
 *  right. That is a PIXEL question and it belongs to `TASK-802`'s PIE row (`AS-§6 A(e)`,
 *  `SC-§35`): walk the hero into the ladder and WATCH. These tests prove the arithmetic is
 *  correct, the configuration reaches the component, the fence held, and the fix left ⛔ nothing
 *  behind for a climb exit to unwind.
 *
 *  🔒 Airlock: no `Capture()`, no `EnsureSnapshot()`, Zone A untouched, ⛔ no token figure
 *  (`AS-§12g`). Nothing here runs a model, opens the editor, or touches Git.
 */

namespace SiegeHeroCameraTestFixture
{
	/** Tolerance for float comparisons that are not exact by nature. Tight enough that any real mistake blows through it. */
	constexpr float Tolerance = 1.e-3f;

	/**
	 *  ⭐ THE BOOM'S SHIPPED NATURAL LENGTH, RE-STATED HERE AS A ***TEST INPUT*** AND ⛔ NOT READ
	 *  FROM THE SUBJECT. `GitClaudeUnrealTestCharacter.cpp:41` is `TargetArmLength = 400.0f`.
	 *  ⛔ Reading it from the CDO and then feeding it back into the same function would make the
	 *  arithmetic rows agree with themselves no matter what either side did.
	 */
	constexpr float NaturalArmUU = 400.f;

	/** Loads a project-relative source file, failing the test (rather than passing vacuously) if it cannot be read. */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' does not exist — the probe is stale, so it FAILS rather than passing on an empty string."), *FullPath));
			return false;
		}

		if (!FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — see above; a stale probe fails."), *FullPath));
			return false;
		}

		return true;
	}

	static bool LoadHeroCpp(FAutomationTestBase& Test, FString& OutText)
	{
		return LoadProjectSource(Test, TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp"), OutText);
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace
	 *  (`\n}`) — how every function in this codebase ends. ⛔ Deliberately NOT a parser: a
	 *  signature that stops matching FAILS the test rather than silently scanning nothing.
	 */
	static bool ExtractFunctionBody(FAutomationTestBase& Test, const FString& Source, const TCHAR* Signature, FString& OutBody)
	{
		const int32 SignatureIndex = Source.Find(Signature, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		if (SignatureIndex == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' not found — the probe is stale, so it FAILS."), Signature));
			return false;
		}

		const int32 BodyEnd = Source.Find(TEXT("\n}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SignatureIndex);
		if (BodyEnd == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find the end of '%s' — the probe is stale, so it FAILS."), Signature));
			return false;
		}

		OutBody = Source.Mid(SignatureIndex, BodyEnd - SignatureIndex);
		return true;
	}

	/**
	 *  Counts occurrences of a needle on CODE lines only — comment lines are skipped.
	 *  ⚠️ Load-bearing: this file's subject is documented in prose that repeatedly NAMES the very
	 *  symbols being counted, so a naive count would report a comment as an implementation.
	 */
	static int32 CountOccurrencesInCode(const FString& Source, const TCHAR* Needle)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		int32 Count = 0;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();
			if (Trimmed.StartsWith(TEXT("//")) || Trimmed.StartsWith(TEXT("*")) || Trimmed.StartsWith(TEXT("/*")))
			{
				continue;
			}

			int32 SearchFrom = 0;
			while (true)
			{
				const int32 Found = Line.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
				if (Found == INDEX_NONE)
				{
					break;
				}
				++Count;
				SearchFrom = Found + 1;
			}
		}

		return Count;
	}

	/** The hero CDO, or null with the test already failed. */
	static const AHeroCharacter* GetHeroDefaults(FAutomationTestBase& Test)
	{
		const UClass* const HeroClass = AHeroCharacter::StaticClass();
		const AHeroCharacter* const Defaults = HeroClass ? Cast<AHeroCharacter>(HeroClass->GetDefaultObject()) : nullptr;
		if (!Defaults)
		{
			Test.AddError(TEXT("⛔ AHeroCharacter has no CDO — the probe cannot run, so it FAILS."));
		}
		return Defaults;
	}

	/** Reads a float UPROPERTY off the CDO by reflected name (the properties are protected; reflection is the supported read). */
	static bool ReadFloatDefault(FAutomationTestBase& Test, const AHeroCharacter& Defaults, const TCHAR* PropertyName, float& OutValue)
	{
		const FFloatProperty* const Property = CastField<FFloatProperty>(AHeroCharacter::StaticClass()->FindPropertyByName(FName(PropertyName)));
		if (!Property)
		{
			Test.AddError(FString::Printf(TEXT("⛔ AHeroCharacter has no reflected float property '%s' — the VIS-§3 pinned name is missing or was renamed."), PropertyName));
			return false;
		}

		OutValue = Property->GetPropertyValue_InContainer(&Defaults);
		return true;
	}
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  1 — THE ARITHMETIC: THE FLOOR RESTORES THE DEFECT FRAME AND LEAVES EVERY OTHER FRAME ALONE
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraPushOutArithmeticTest,
	"Siegebound.HeroCamera.ThePushOutRestoresACollapsedArmAndPassesHealthyArmsThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraPushOutArithmeticTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	constexpr float Floor = 150.f;

	// ⭐ THE UNCOLLIDED FRAME — the one that must be BYTE-IDENTICAL to today. An arm the probe
	// never touched returns EXACTLY zero, so the camera carries no offset at all.
	TestEqual(TEXT("(a) an uncollided 400 uu arm gets EXACTLY zero offset — open ground is untouched"),
		AHeroCharacter::ComputeCameraPushOutLocalX(NaturalArmUU, NaturalArmUU, Floor), 0.f);

	// ⭐⭐ THE MEASURED DEFECT: the probe collapsed the arm to a standoff of a few uu. The
	// expectation is RE-DERIVED from the law ("stand at least Floor off the pawn"), ⛔ not
	// transcribed from the subject: Fixed + |PushOut| must equal Floor.
	{
		constexpr float CollapsedArm = 8.f;
		const float PushOut = AHeroCharacter::ComputeCameraPushOutLocalX(CollapsedArm, NaturalArmUU, Floor);
		TestTrue(TEXT("(b) a collapsed arm is pushed OUT — the sign convention is negative, because the boom socket's +X points AT the pawn"), PushOut < 0.f);
		TestEqual(TEXT("(b) ⭐ the pushed camera lands EXACTLY on the floor — re-derived as Fixed + |PushOut| == Floor, never transcribed"),
			CollapsedArm + FMath::Abs(PushOut), Floor, Tolerance);
	}

	// The worst case the engine can produce: initial penetration puts the socket ON the arm origin.
	TestEqual(TEXT("(c) a totally collapsed arm (0 uu — the camera exactly at the pawn) is pushed the whole floor"),
		AHeroCharacter::ComputeCameraPushOutLocalX(0.f, NaturalArmUU, Floor), -Floor, Tolerance);

	// ⭐ CONTROL — A MILD TRIM MUST BE LEFT ALONE. This row is what stops the fix from becoming a
	// blanket "camera never collides": a wall that trims 400 to 260 is the spring arm working, and
	// if this ever returns non-zero the floor has been turned into a target.
	TestEqual(TEXT("(d) ⭐ CONTROL: an arm the probe trimmed to 260 uu — comfortably past the floor — is left EXACTLY alone"),
		AHeroCharacter::ComputeCameraPushOutLocalX(260.f, NaturalArmUU, Floor), 0.f);

	// The boundary itself belongs to the no-op side: at the floor there is nothing to correct.
	TestEqual(TEXT("(e) an arm sitting EXACTLY on the floor gets EXACTLY zero — the boundary is a no-op, not a nudge"),
		AHeroCharacter::ComputeCameraPushOutLocalX(Floor, NaturalArmUU, Floor), 0.f);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  2 — THE CEILING: A FLOOR MAY NEVER LENGTHEN THE ARM PAST WHAT OPEN GROUND GIVES
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraPushOutNeverExceedsNaturalArmTest,
	"Siegebound.HeroCamera.ThePushOutIsClampedToTheBoomsOwnArmLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraPushOutNeverExceedsNaturalArmTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	// ⭐⭐ THE ROW THAT FAILS THE MOMENT THE CLAMP IS DROPPED. `MinCameraArmLengthUU` is
	// EditDefaultsOnly and FLAGGED for Jonathan's feel pass, so a value ABOVE the boom's 400 uu
	// TargetArmLength is a realistic edit, ⛔ not a hypothetical. Unclamped it would give -480 and
	// the camera would sit FURTHER out under collision than in open ground — a fix that breaks the
	// case it was never supposed to touch.
	constexpr float FloorAboveTheArm = 500.f;
	constexpr float CollapsedArm = 20.f;

	const float PushOut = AHeroCharacter::ComputeCameraPushOutLocalX(CollapsedArm, NaturalArmUU, FloorAboveTheArm);

	TestEqual(TEXT("(a) ⭐⭐ a floor ABOVE the natural arm is clamped TO the natural arm — the camera lands at 400, never at 500"),
		CollapsedArm + FMath::Abs(PushOut), NaturalArmUU, Tolerance);

	// ⭐ CONTROL: the same call with the floor BELOW the arm must give a different, smaller push —
	// otherwise the clamp row above could be satisfied by a function that ignores its floor entirely.
	const float ShallowPushOut = AHeroCharacter::ComputeCameraPushOutLocalX(CollapsedArm, NaturalArmUU, 150.f);
	TestTrue(TEXT("(b) ⭐ CONTROL: a floor BELOW the arm pushes strictly LESS far — proving the floor is read, not ignored"),
		FMath::Abs(ShallowPushOut) < FMath::Abs(PushOut) - Tolerance);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  3 — THE OFF-SWITCH BY NUMBER
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraPushOutOffSwitchTest,
	"Siegebound.HeroCamera.ThePushOutIsInertForANonPositiveFloorOrArm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraPushOutOffSwitchTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	// Setting MinCameraArmLengthUU to 0 must give back today's exact behaviour — the second
	// off-switch, so a playtest can back the fix out by NUMBER as well as by flag.
	TestEqual(TEXT("(a) a zero floor is inert — the numeric off-switch"),
		AHeroCharacter::ComputeCameraPushOutLocalX(5.f, NaturalArmUU, 0.f), 0.f);

	TestEqual(TEXT("(b) a negative floor is inert too — a hand-typed negative cannot invert the fix"),
		AHeroCharacter::ComputeCameraPushOutLocalX(5.f, NaturalArmUU, -150.f), 0.f);

	// A boom with no arm has nothing to reason about; the engine itself skips its trace when
	// TargetArmLength == 0 (SpringArmComponent.cpp:191), so this mirrors the engine's own guard.
	TestEqual(TEXT("(c) a zero-length arm is inert — mirrors the engine skipping its own trace at TargetArmLength == 0"),
		AHeroCharacter::ComputeCameraPushOutLocalX(0.f, 0.f, 150.f), 0.f);

	// ⭐ CONTROL — WITHOUT THIS ROW EVERY ASSERTION ABOVE IS SATISFIED BY A FUNCTION THAT ALWAYS
	// RETURNS ZERO, which is precisely how an assertion stops discriminating.
	TestTrue(TEXT("(d) ⭐ CONTROL: the SAME collapsed arm with a VALID floor is NOT inert — the zeros above are decisions, not a stuck return"),
		AHeroCharacter::ComputeCameraPushOutLocalX(5.f, NaturalArmUU, 150.f) < -Tolerance);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  4 — THE CONFIGURATION REACHES THE INHERITED BOOM AND SURVIVES CONSTRUCTION
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraProbeSizeSurvivesConstructionTest,
	"Siegebound.HeroCamera.TheProbeSizeTunableReachesTheInheritedBoomAndSurvivesConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraProbeSizeSurvivesConstructionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	const AHeroCharacter* const Defaults = GetHeroDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	const USpringArmComponent* const Boom = Defaults->GetCameraBoom();
	if (!TestNotNull(TEXT("(a) the hero CDO carries the INHERITED CameraBoom — the component this whole task configures"), Boom))
	{
		return false;
	}

	float TunableProbeSize = 0.f;
	if (!ReadFloatDefault(*this, *Defaults, TEXT("HeroCameraProbeSize"), TunableProbeSize))
	{
		return false;
	}

	TestEqual(TEXT("(b) the boom's ProbeSize equals HeroCameraProbeSize AFTER construction — the constructor wrote it and nothing clobbered it"),
		Boom->ProbeSize, TunableProbeSize, Tolerance);

	// ⭐⭐ CONTROL — AND IT IS THE POINT OF THIS TEST. (b) alone goes VACUOUS the day the tunable's
	// default happens to equal the engine's: both sides would read 12 whether the constructor line
	// exists or not. The independent probe is the shipped source itself.
	FString HeroCpp;
	if (!LoadHeroCpp(*this, HeroCpp))
	{
		return false;
	}

	FString TuningBody;
	if (!ExtractFunctionBody(*this, HeroCpp, TEXT("void AHeroCharacter::ApplyHeroCameraTuning()"), TuningBody))
	{
		return false;
	}

	TestTrue(TEXT("(c) ⭐⭐ CONTROL: ApplyHeroCameraTuning actually ASSIGNS the boom's ProbeSize from the tunable — (b) cannot go vacuous behind this"),
		CountOccurrencesInCode(TuningBody, TEXT("ProbeSize = HeroCameraProbeSize")) == 1);

	// ⭐ CONTROL: the ctor+BeginPlay re-apply idiom (ApplyMovementSpeed / ApplyTerrainMovementTuning).
	// Definition + constructor call + BeginPlay call = 3. A ctor-only write would bake the C++
	// default into the CDO and silently ignore a BP_HeroCharacter feel-pass edit.
	TestEqual(TEXT("(d) ⭐ CONTROL: ApplyHeroCameraTuning appears 3× in code — its definition PLUS both the constructor and BeginPlay call sites"),
		CountOccurrencesInCode(HeroCpp, TEXT("ApplyHeroCameraTuning")), 3);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  5 — THE REFUSED SHAPE: THE PROBE STAYS ON
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraCollisionTestStaysOnTest,
	"Siegebound.HeroCamera.TheBoomsCollisionProbeIsLeftEnabledOnTheDefaultChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraCollisionTestStaysOnTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	const AHeroCharacter* const Defaults = GetHeroDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	const USpringArmComponent* const Boom = Defaults->GetCameraBoom();
	if (!TestNotNull(TEXT("(a) the hero CDO carries the inherited CameraBoom"), Boom))
	{
		return false;
	}

	// ⛔⛔ A BLANKET `bDoCollisionTest = false` WAS REFUSED OUTRIGHT: it lets the camera sit inside
	// EVERY wall in the map. If this ever reads false, the narrow fix has been replaced by the
	// shape the ruling forbade.
	TestTrue(TEXT("(b) ⛔ bDoCollisionTest is still TRUE — the refused blanket-off has not crept in"), Boom->bDoCollisionTest != 0);

	// ⛔ Retargeting ProbeChannel would need a new channel in DefaultEngine.ini — outside this
	// task's fence, and it would change what every camera in the project collides with.
	TestEqual(TEXT("(c) ⛔ ProbeChannel is still ECC_Camera — no ini-level collision-channel change was smuggled in"),
		static_cast<int32>(Boom->ProbeChannel.GetValue()), static_cast<int32>(ECC_Camera));

	// ⭐ CONTROL: the probe radius is a value this task OWNS, so it must be readable and sane —
	// this row fails if ProbeSize was zeroed as a back-door way of disabling the probe.
	TestTrue(TEXT("(d) ⭐ CONTROL: ProbeSize is strictly positive — a zeroed probe would be a disguised bDoCollisionTest = false"),
		Boom->ProbeSize > 0.f);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  6 — THE THREE PINNED NAMES (`VIS-§3`), CHARACTER FOR CHARACTER
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraPinnedTunablesTest,
	"Siegebound.HeroCamera.TheThreePinnedTunablesExistAreEditDefaultsOnlyAndCarryTheirDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraPinnedTunablesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	const AHeroCharacter* const Defaults = GetHeroDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	UClass* const HeroClass = AHeroCharacter::StaticClass();

	// ⛔ The names are the MANAGER'S (`VIS-§3`) and a rename is a QA blocker on sight. Reflection is
	// the only probe that can tell a rename from a refactor.
	const FFloatProperty* const ProbeSizeProperty = CastField<FFloatProperty>(HeroClass->FindPropertyByName(FName(TEXT("HeroCameraProbeSize"))));
	const FBoolProperty* const IgnoreProperty = CastField<FBoolProperty>(HeroClass->FindPropertyByName(FName(TEXT("bIgnoreClimbableGeometryForCamera"))));
	const FFloatProperty* const MinArmProperty = CastField<FFloatProperty>(HeroClass->FindPropertyByName(FName(TEXT("MinCameraArmLengthUU"))));

	TestNotNull(TEXT("(a) `HeroCameraProbeSize` exists as a reflected float — the VIS-§3 pinned name"), ProbeSizeProperty);
	TestNotNull(TEXT("(b) `bIgnoreClimbableGeometryForCamera` exists as a reflected bool — the VIS-§3 pinned name"), IgnoreProperty);
	TestNotNull(TEXT("(c) `MinCameraArmLengthUU` exists as a reflected float — the VIS-§3 pinned name"), MinArmProperty);

	if (!ProbeSizeProperty || !IgnoreProperty || !MinArmProperty)
	{
		return false;
	}

	// 🧑 ALL THREE ARE FLAGGED FOR JONATHAN'S FEEL PASS, and `EditDefaultsOnly` is what makes that
	// feel pass a defaults edit rather than a recompile. `EditDefaultsOnly` == Edit + DisableEditOnInstance.
	auto CheckEditDefaultsOnly = [this](const FProperty* Property, const TCHAR* Name)
	{
		TestTrue(FString::Printf(TEXT("(d) `%s` is editable — 🧑 the feel pass needs it visible"), Name),
			Property->HasAnyPropertyFlags(CPF_Edit));
		TestTrue(FString::Printf(TEXT("(d) `%s` is EditDefaultsOnly (DisableEditOnInstance) — a per-instance override would make the feel pass unreproducible"), Name),
			Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	};

	CheckEditDefaultsOnly(ProbeSizeProperty, TEXT("HeroCameraProbeSize"));
	CheckEditDefaultsOnly(IgnoreProperty, TEXT("bIgnoreClimbableGeometryForCamera"));
	CheckEditDefaultsOnly(MinArmProperty, TEXT("MinCameraArmLengthUU"));

	// The pinned default: TRUE. Shipping it false would ship the defect with a switch beside it.
	TestTrue(TEXT("(e) ⭐ `bIgnoreClimbableGeometryForCamera` defaults to TRUE — VIS-§3 pins the default, and false would ship the defect"),
		IgnoreProperty->GetPropertyValue_InContainer(Defaults));

	// ⭐ CONTROL: the floor must be a real, positive number. A zero default would satisfy every
	// arithmetic test above while shipping a fix that does NOTHING at runtime.
	float MinArm = 0.f;
	if (ReadFloatDefault(*this, *Defaults, TEXT("MinCameraArmLengthUU"), MinArm))
	{
		TestTrue(TEXT("(f) ⭐ CONTROL: `MinCameraArmLengthUU` ships STRICTLY POSITIVE — a zero default is the numeric off-switch and would ship an inert fix"),
			MinArm > 0.f);
		TestTrue(TEXT("(g) ⭐ CONTROL: the floor is BELOW the boom's 400 uu natural arm — a floor at or above it would be a disguised 'never collide'"),
			MinArm < NaturalArmUU);
	}

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  7 — THE FENCE (`VIS-R2`): THE UE TEMPLATE BASE IS UNTOUCHED
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraTemplateBaseUntouchedTest,
	"Siegebound.HeroCamera.TheSharedTemplateCharacterIsUntouchedByThisCameraFix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraTemplateBaseUntouchedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	// ⚖️ `VIS-R2` IS A FENCE, NOT A PREFERENCE: `AGitClaudeUnrealTestCharacter` is the UE template
	// base, ALSO inherited by `Variant_Combat/CombatCharacter` and
	// `Variant_Platforming/PlatformingCharacter`. A camera tuning for ONE game's hero may never be
	// paid by three unrelated template characters — and TASK-801 calls any diff here an AUTOMATIC
	// FAIL. This test makes that gate executable instead of a reviewer's memory.
	FString TemplateCpp;
	FString TemplateHeader;
	if (!LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/GitClaudeUnrealTestCharacter.cpp"), TemplateCpp) ||
		!LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/GitClaudeUnrealTestCharacter.h"), TemplateHeader))
	{
		return false;
	}

	const FString TemplateBoth = TemplateCpp + TemplateHeader;

	const TArray<FString> ForbiddenTokens = {
		TEXT("ProbeSize"),
		TEXT("ProbeChannel"),
		TEXT("bDoCollisionTest"),
		TEXT("HeroCameraProbeSize"),
		TEXT("bIgnoreClimbableGeometryForCamera"),
		TEXT("MinCameraArmLengthUU"),
	};

	for (const FString& Token : ForbiddenTokens)
	{
		TestEqual(FString::Printf(TEXT("(a) ⛔ '%s' appears ZERO times in the UE template base — VIS-R2's fence held"), *Token),
			CountOccurrencesInCode(TemplateBoth, *Token), 0);
	}

	// ⭐⭐ CONTROL — WITHOUT IT THIS TEST PASSES ON AN EMPTY STRING, A MOVED FILE OR A TYPO IN THE
	// PATH, which is exactly how a fence test quietly stops guarding anything.
	TestTrue(TEXT("(b) ⭐⭐ CONTROL: the template base really was read — 'CameraBoom' is present, so the zeros above are measurements"),
		CountOccurrencesInCode(TemplateBoth, TEXT("CameraBoom")) > 0);

	TestTrue(TEXT("(c) ⭐ CONTROL: the template still builds the boom at TargetArmLength = 400.0f — the natural arm this suite's arithmetic assumes"),
		CountOccurrencesInCode(TemplateCpp, TEXT("TargetArmLength = 400.0f")) == 1);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  8 — THE RULING: THE FIX IS GENERIC, ⛔ NOT SCOPED TO A CLIMB
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraServicedOutsideAnyClimbGuardTest,
	"Siegebound.HeroCamera.TheCameraIsServicedFromTickOutsideAnyClimbGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraServicedOutsideAnyClimbGuardTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	FString HeroCpp;
	if (!LoadHeroCpp(*this, HeroCpp))
	{
		return false;
	}

	FString TickBody;
	if (!ExtractFunctionBody(*this, HeroCpp, TEXT("void AHeroCharacter::Tick(float DeltaSeconds)"), TickBody))
	{
		return false;
	}

	TestEqual(TEXT("(a) Tick calls TickHeroCameraCollision exactly once"),
		CountOccurrencesInCode(TickBody, TEXT("TickHeroCameraCollision()")), 1);

	// ⭐⭐ THE RULING, MADE EXECUTABLE. The collapse is NOT climb-specific — this class had zero
	// camera code before TASK-790, so the defect is the inherited boom meeting tall geometry, and
	// scoping the cure to a climb would leave the identical blindness on the WALK-UP that VID-004
	// actually captured (01:12.0–01:13.5 is the APPROACH, ⛔ not the ascent). If a future edit
	// wraps this call in a climb guard, one of these tokens has to appear in Tick — and this fails.
	TestEqual(TEXT("(b) ⭐⭐ Tick contains NO `LadderClimb.bActive` guard — the camera service is generic, per the ruling"),
		CountOccurrencesInCode(TickBody, TEXT("LadderClimb.bActive")), 0);
	TestEqual(TEXT("(c) ⭐⭐ Tick contains NO `IsClimbing()` guard either — the same ruling, by the other accessor"),
		CountOccurrencesInCode(TickBody, TEXT("IsClimbing()")), 0);

	// ⭐ CONTROL: the extraction really found Tick's body, not an empty span.
	TestTrue(TEXT("(d) ⭐ CONTROL: the extracted Tick body really is Tick — it still drives the climb and the contact poll"),
		CountOccurrencesInCode(TickBody, TEXT("TickLadderClimb(DeltaSeconds)")) == 1 &&
		CountOccurrencesInCode(TickBody, TEXT("PollLadderContact(DeltaSeconds)")) == 1);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  9 — NOTHING TO UNWIND: EVERY PATH THROUGH THE SERVICE WRITES THE OFFSET
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraHasNoStateToUnwindTest,
	"Siegebound.HeroCamera.TheCameraServiceIsStatelessOneWriteOneExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraHasNoStateToUnwindTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	FString HeroCpp;
	if (!LoadHeroCpp(*this, HeroCpp))
	{
		return false;
	}

	FString ServiceBody;
	if (!ExtractFunctionBody(*this, HeroCpp, TEXT("void AHeroCharacter::TickHeroCameraCollision()"), ServiceBody))
	{
		return false;
	}

	// ⭐⭐ THE SHAPE THAT MAKES "NOTHING TO UNWIND" TRUE RATHER THAN CLAIMED: the offset is written
	// on EVERY frame from a local that starts at zero, so the not-pushing state is re-established
	// unconditionally. ⛔ A camera left ignoring the tower forever is exactly the class of bug this
	// wave has been fighting; the cure is to hold no persistent decision at all.
	TestEqual(TEXT("(a) ⭐⭐ the service writes the camera's relative location EXACTLY once — one write, one exit"),
		CountOccurrencesInCode(ServiceBody, TEXT("SetRelativeLocation(")), 1);

	// Exactly one `return` — the null guard. An early return added after it would skip the write
	// and could strand the camera pushed, which is the failure mode this row exists to catch.
	TestEqual(TEXT("(b) ⭐ the service has EXACTLY one early return (the null guard) — no path can skip the write"),
		CountOccurrencesInCode(ServiceBody, TEXT("return;")), 1);

	// ⛔ The refused shapes, as executable prohibitions: nothing here mutates the boom's collision
	// switch, and nothing reaches into another actor's collision responses (cross-actor state that
	// ten climb exits would then owe a revert).
	TestEqual(TEXT("(c) ⛔ the service never writes bDoCollisionTest — the blanket-off shape cannot creep in at runtime"),
		CountOccurrencesInCode(ServiceBody, TEXT("bDoCollisionTest =")), 0);
	TestEqual(TEXT("(d) ⛔ the service never mutates another actor's collision responses — no cross-actor state to unwind"),
		CountOccurrencesInCode(ServiceBody, TEXT("SetCollisionResponseToChannel")), 0);

	// ⭐ CONTROL: the extraction found the real body — it still reads the boom and consults the
	// pure arithmetic. Without this, (a)–(d) would all pass on an empty span.
	TestTrue(TEXT("(e) ⭐ CONTROL: the extracted body really is the service — it reads the boom and calls the pure arithmetic"),
		CountOccurrencesInCode(ServiceBody, TEXT("IsCollisionFixApplied()")) == 1 &&
		CountOccurrencesInCode(ServiceBody, TEXT("ComputeCameraPushOutLocalX(")) == 1);

	return true;
}

//~ ═══════════════════════════════════════════════════════════════════════════════════════════
//~  10 — PRESERVATION: THE TEN-EXIT TEARDOWN GAINED NOTHING
//~ ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroCameraTeardownUnchangedTest,
	"Siegebound.HeroCamera.TheTenExitTeardownGainedNoCameraWorkAndTheCensusHolds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroCameraTeardownUnchangedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroCameraTestFixture;

	FString HeroCpp;
	if (!LoadHeroCpp(*this, HeroCpp))
	{
		return false;
	}

	FString TeardownBody;
	if (!ExtractFunctionBody(*this, HeroCpp, TEXT("void AHeroCharacter::EndLadderClimb(bool bReachedTop, ESiegeHeroLadderExit Reason)"), TeardownBody))
	{
		return false;
	}

	// ⭐⭐ THE WHOLE POINT OF PREFERRING A STATELESS FIX: the ten climb exits funnel through this ONE
	// teardown, and this fix owes it NOTHING. If a later edit makes the camera fix stateful, the
	// revert lands here — and this row is what will notice.
	TestEqual(TEXT("(a) ⭐⭐ the teardown does NO camera work — the camera fix owes the ten exits nothing"),
		CountOccurrencesInCode(TeardownBody, TEXT("Camera")), 0);
	TestEqual(TEXT("(b) ⭐ and it touches no spring arm either"),
		CountOccurrencesInCode(TeardownBody, TEXT("Boom")), 0);

	// The standing one-of-each census the batch preserves (`TOWER-§8`): a second restore site is
	// how a MOVE_Flying hero ends up hanging in mid-air forever.
	TestEqual(TEXT("(c) ⛔ PRESERVED: `SetDefaultMovementMode()` still appears EXACTLY once in the file — one restore, reached from all ten exits"),
		CountOccurrencesInCode(HeroCpp, TEXT("SetDefaultMovementMode()")), 1);
	TestEqual(TEXT("(d) ⛔ PRESERVED: `SetMovementMode(MOVE_Flying)` still appears EXACTLY once — one place the hero leaves gravity"),
		CountOccurrencesInCode(HeroCpp, TEXT("SetMovementMode(MOVE_Flying)")), 1);
	TestEqual(TEXT("(e) ⛔ PRESERVED: `FSiegeLadderClimbStatics::End(` still appears EXACTLY once — the exactly-once latch is not duplicated"),
		CountOccurrencesInCode(HeroCpp, TEXT("FSiegeLadderClimbStatics::End(")), 1);

	// ⭐ CONTROL: the extraction found the real teardown, not an empty span.
	TestTrue(TEXT("(f) ⭐ CONTROL: the extracted body really is the teardown — it still clears the watchdog and restores MaxFlySpeed"),
		CountOccurrencesInCode(TeardownBody, TEXT("ClearTimer(LadderClimbWatchdogTimerHandle)")) == 1 &&
		CountOccurrencesInCode(TeardownBody, TEXT("MaxFlySpeed = LadderClimbSavedMaxFlySpeed")) == 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
