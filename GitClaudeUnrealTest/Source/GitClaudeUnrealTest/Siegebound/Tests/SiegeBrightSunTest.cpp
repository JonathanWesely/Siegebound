// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "HAL/UnrealMemory.h" // FMemory::Memcpy — the sanctioned bit-pattern NaN (SiegeCastBarTest.cpp:129)
#include "Math/UnrealMathUtility.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/FogVolume.h"   // AFogVolume — the CDO these rows read the four BrightSun tunables off
#include "Siegebound/SummonedUnit.h" // ASummonedUnit::HeightAdvantageMultiplier — the DATUM TIE (test 3), read-only
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for `BrightSun` — THE PREVENTION WINDOW + THE HEIGHT-SCALED TIMER
 *      (TASK-982; law `FOG-§10`, `FOG-§10.3`, `FOG-§10.6`, `FOG-§10.7`, `FOG-§9.4`, `FOG-§9.5`,
 *      `HIGH-§1`, `SC-§37`; rulings ✅ `J-F13` `J-F14` `J-F15` `J-F16` `J-F17` `J-F18` `J-F19`) ═══
 *
 *  ⛔ Jonathan, verbatim (2026-09-04): *"If bright sun is played during the bright sun window, then
 *  the timer gets RESET to whatever the new time would be under the new cast, UNLESS that new time
 *  would be LESS than the current time … and the player is basically prevented from playing the
 *  card. … For calculating 'bright sun' fog prevention time, the height is sampled at the time
 *  that the card is cast. The ground is simply the elevation of the flat grass terrain, NOT
 *  INCLUDING THE HILLS, therefore that ground height number should be the same anywhere on the
 *  map, and it should be the SAME HEIGHT IN WHICH RANGED UNITS' DAMAGE IS AT 1 TIMES THEIR
 *  DAMAGE."* And: *"Even when the 'bright sun' fog prevention timer ends, the fog that was cleared
 *  STILL REMAINS CLEAR."*
 *
 *  ⭐⭐ WHY A NEW FILE RATHER THAN ROWS IN `Tests/SiegeFogVolumeTest.cpp`, stated because the
 *  natural home was considered: that file HAS been edited by this row, but only where its own
 *  comments instructed ("⭐ WHEN `FogClear` IS APPENDED BELOW, THIS ROW MOVES TO IT") and where a
 *  changed signature forced it. Keeping the ~20 NEW assertions here makes this row's diff legible
 *  to its gate and keeps `TASK-998`'s reviewed file as close to byte-unchanged as the change
 *  allowed. ⛔ `Tests/SiegeFogClampTest.cpp` and `Tests/SiegeFogTest.cpp` are other rows' and are
 *  untouched; `Tests/SiegeHighGroundTest.cpp` is the DAMAGE lane's and is untouched — test 3 below
 *  reads that lane's symbols but writes nothing in its file.
 *
 *  MECHANISM — ⛔ zero PIE, ⛔ zero `SpawnActor`, ⛔ zero `UWorld::CreateWorld`, ⛔ zero asset
 *  loads, ⛔ zero writes. The three house lanes, unchanged:
 *    (a) DIRECT calls on the PURE static `AFogVolume::BrightSunWindowSeconds` — the whole reason
 *        that function takes its tunables as parameters (the `HeightAdvantageMultiplier`
 *        testability-seam precedent, followed on purpose);
 *    (b) CDO reads through the property system (`GetDefault<T>()` + `FindPropertyByName`), the
 *        `SiegeBuildingStackTest.cpp:205` idiom — so the expectations are built from the SHIPPED
 *        numbers rather than from transcriptions of them, and a RENAME goes red here, which is
 *        exactly the `CoreRedirects` hazard `FogVolume.h` documents;
 *    (c) SOURCE-TEXT structural probes with comment lines skipped, for the claims that are about
 *        WHICH FUNCTION CALLS WHICH and about what is ABSENT — the only lane that can see a call
 *        graph, or the absence of a trace, without a world.
 *
 *  ⚠️⚠️ THE DECLARED GAP, stated so it is honest rather than discovered: there is not one
 *  `SpawnActor` anywhere in `Siegebound/Tests/`, so the behaviours that need a live actor — that
 *  `ApplyBrightSun` really moves the stored expiry, that `RaiseFog` really refuses during a
 *  window, that `GetFogPreventionSecondsRemaining` really counts down — are asserted STRUCTURALLY
 *  here and are ⛔ NOT proven at runtime by this suite. ⭐ Each structural form is chosen to be the
 *  one that can still go red on the ACTUAL hazard: the one-way door dies as a restored fog
 *  deadline, the sun-on-sun branch dies as an `FMath::Max`, the datum dies as a trace, and the
 *  formula dies at a step boundary — and every one of those is visible without a world.
 */

namespace SiegeBrightSunFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these claims are about. */
	const TCHAR* FogVolumeH = TEXT("Source/GitClaudeUnrealTest/Siegebound/FogVolume.h");
	const TCHAR* FogVolumeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp");
	const TCHAR* SpellLibraryCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp");
	const TCHAR* GameModeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp");
	const TCHAR* PlayerControllerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp");

	/**
	 *  A quiet NaN built from its IEEE-754 bit pattern — copied verbatim from
	 *  `SiegeCastBarTest.cpp:129`, which already paid for this lesson.
	 *  ⛔ NOT `0.f / 0.f` and ⛔ NOT `FMath::Sqrt(-1.f)`: both are constant-foldable, and a fast-math
	 *  build may evaluate them at compile time into something that is no longer non-finite — which
	 *  would make test 1's NaN clauses pass while testing NOTHING. ⭐ Each use is paired with a
	 *  `FMath::IsFinite` self-check for exactly that reason.
	 */
	static float MakeQuietNaN()
	{
		const uint32 NaNBits = 0x7FC00000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &NaNBits, sizeof(Result));
		return Result;
	}

	/** Reads a shipped project file. ⛔ A probe that cannot read its subject FAILS. */
	static bool LoadProjectFile(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
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
	 *  Occurrences of Needle on CODE lines only — the house helper, copied verbatim from
	 *  `SiegeFogVolumeTest.cpp` / `SiegeFogClampTest.cpp` / `SiegeAcquisitionFunnelTest.cpp` so all
	 *  four agree character for character. ⛔ Do not "improve" it here; a divergent counter would
	 *  make two files disagree about the same source.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL: `FogVolume.h`'s tunable docs NAME `FogVisionCeilingUU`
	 *  and the retired `2000`/`69.5%` figures in the paragraphs that explain why they are NOT used.
	 *  A scanner that counted comments would force that header to choose between explaining the law
	 *  and passing it — and test 2(b) below asserts a ZERO that those very comments would break.
	 *  ⚠️ DECLARED LIMITATION, inherited and restated: a comment TRAILING a line of code IS still
	 *  scanned. Every probe below is a whole-line construct or a statement.
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

			// `*` is qualified rather than bare on purpose: a doc-comment continuation is `* text`
			// or `*/`, while `*GetNameSafe(Foo)` starts a CODE line with the same character.
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

	/**
	 *  ⛔⛔ BARE references to the ceiling MEMBER, with the DOOR's spelling discounted (TASK-1041).
	 *
	 *  ⭐⭐ THE COLLISION, AND IT IS A SUBSTRING ONE: `ApplyFogVisionCeilingUU` ⛔ CONTAINS
	 *  `FogVisionCeilingUU`. ⇒ a plain census of the member's name cannot tell *"this file READS the
	 *  vision ceiling"* — the coupling `FOG-§9.5` calls an AUTOMATIC FAIL — from *"this file routes
	 *  a reach through the ONE unit-side door"*, which is legal everywhere. ⛔ TASK-1008 shipped
	 *  that door AFTER these pins were written, so the collision arrived under a green suite.
	 *
	 *  ⚠️ The failure mode is a ⛔ FALSE RED, not a false green — the pin over-counts, so nothing is
	 *  hidden today. ⛔ It is hardened anyway because the RED lands on a FUTURE row that did nothing
	 *  wrong, and the cheapest way out of a false red is to weaken the pin.
	 *
	 *  ⭐ THE SUBTRACTION IS ⛔ EXACT, NOT AN APPROXIMATION, and both halves are needed to say so:
	 *  the door's spelling contains the member's spelling ⛔ EXACTLY ONCE, and
	 *  `CountOccurrencesInCode` counts ⛔ NON-OVERLAPPING matches ⇒ every door call contributes
	 *  exactly one false hit and exactly one is removed. ⛔ The result therefore can never go
	 *  negative, and ⛔ adding door calls can never mask a bare reference.
	 */
	static int32 CountBareCeilingMemberReferences(const FString& Source)
	{
		return CountOccurrencesInCode(Source, TEXT("FogVisionCeilingUU"))
			- CountOccurrencesInCode(Source, TEXT("ApplyFogVisionCeilingUU"));
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace — the
	 *  house helper. ⛔ Deliberately NOT a parser: a signature that stops matching FAILS rather than
	 *  silently scanning nothing (`SC-§38` — a probe pinned to a stale coordinate must go RED,
	 *  never quietly green).
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
	 *  Reads a float `UPROPERTY` off an object by name — the `SiegeBuildingStackTest.cpp:205`
	 *  helper. Reflection rather than a C++ member read on purpose: it works regardless of the
	 *  property's access specifier (`HeightBonusStepUU` and `HeightBonusPerStep` are `protected` on
	 *  `ASummonedUnit`, and this row must NOT widen their access to read them), and a RENAMED
	 *  property returns null and FAILS here instead of silently reading a different member.
	 */
	static bool ReadFloatProperty(FAutomationTestBase& Test, const UObject* Object, const TCHAR* PropertyName, float& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FFloatProperty* const FloatProperty = CastField<FFloatProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			Test.AddError(FString::Printf(
				TEXT("SELF-CHECK FAILED: '%s' is not a reflected float property on '%s' — the probe is stale. ")
				TEXT("⚠️ If it was RENAMED rather than removed, this red is also the CoreRedirects warning FogVolume.h documents."),
				PropertyName, *Object->GetClass()->GetName()));
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Object);
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐⭐ — THE FORMULA, AT THE **STEP BOUNDARIES**, WHERE `floor` BREAKS.
//  ⛔ `Window = Base + BonusPerStep × floor(max(0, HeroZ − Ground) / Step)`.
//  Every expectation is built from the SHIPPED CDO values, so a retune moves the
//  test and the code together and neither can drift alone. The BOUNDARY cases are
//  the point: 1523 vs 1524 and 3047 vs 3048 are the only inputs that can tell
//  `floor` from `round`, and a rounded rule would hand out the next whole minute
//  HALF a step early on every perch in the game.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunWindowFormulaTest,
	"Siegebound.BrightSun.TheWindowFormulaFloorsAtEveryStepBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunWindowFormulaTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	const AFogVolume* const Defaults = GetDefault<AFogVolume>();
	if (!Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<AFogVolume>() returned null — every expectation below is read off this object."));
		return false;
	}

	float BaseSeconds = 0.f;
	float BonusPerStep = 0.f;
	float StepUU = 0.f;
	float GroundZ = -1.f; // ⛔ seeded NON-zero so a failed read cannot masquerade as the shipped 0
	if (!ReadFloatProperty(*this, Defaults, TEXT("BrightSunBaseDurationSeconds"), BaseSeconds)
		|| !ReadFloatProperty(*this, Defaults, TEXT("BrightSunBonusSecondsPerStep"), BonusPerStep)
		|| !ReadFloatProperty(*this, Defaults, TEXT("BrightSunHeightStepUU"), StepUU)
		|| !ReadFloatProperty(*this, Defaults, TEXT("ArenaGroundReferenceZUU"), GroundZ))
	{
		return false;
	}

	// ⭐ HIS SENTENCES, ARITHMETIC SHOWN — the numbers are RE-DERIVED FROM THE WORDS, never
	// re-typed, so this row and the shipped constants cannot agree by both being wrong the same
	// way (the `FogDurationSeconds` = 5 × 60 precedent in Tests/SiegeFogVolumeTest.cpp test 3).
	//   "base … is 2 minutes"        ⇒ 2 × 60  = 120 s
	//   "increases by 1 minute"      ⇒ 1 × 60  =  60 s per step
	//   "50 feet"                    ⇒ 50 × 30.48 cm/ft = 1524 uu   ⛔ 1524, NOT 609.6
	TestEqual(
		TEXT("⭐ `BrightSunBaseDurationSeconds` is his TWO MINUTES — 2 × 60 = 120 s (FOG-§10.1 also pins the card ")
		TEXT("row's EffectDuration at 120, the same number from a second sentence of his)."),
		BaseSeconds, 2.f * 60.f);

	TestEqual(
		TEXT("⭐ `BrightSunBonusSecondsPerStep` is his ONE MINUTE per step — 1 × 60 = 60 s."),
		BonusPerStep, 1.f * 60.f);

	TestEqual(
		TEXT("⭐⭐ `BrightSunHeightStepUU` is his FIFTY FEET, RE-DERIVED: 50 × 30.48 cm/ft = 1524 uu. ⛔ NOT 609.6 — ")
		TEXT("he AMENDED 20 ft ⇒ 50 ft on 2026-09-04, and 609.6 is the retired 20-ft figure (which is ALSO the fog ")
		TEXT("vision ceiling, the coincidence FOG-§9.5 exists to keep decoupled — see test 2)."),
		StepUU, 50.f * 30.48f);

	TestEqual(
		TEXT("⭐⭐ `ArenaGroundReferenceZUU` is the FLAT-GRASS datum at Z = 0 (✅ J-F13, CONVENTIONS:131 — ")
		TEXT("SM_ArenaTerrain \"placed at (0,0,0) reproduces the old ArenaGround slab's WALK SURFACE at Z=0\"). ")
		TEXT("⛔ It is a constant, not a trace: a hero on a HILL EARNS the height."),
		GroundZ, 0.f);

	// ── ⛔⛔ THE BOUNDARY TABLE. Heights are expressed RELATIVE TO THE SHIPPED DATUM, so this row
	//    stays correct if he ever moves the datum — which is the whole reason it is one constant.
	struct FBoundaryCase
	{
		float HeightAboveGroundUU;
		float ExpectedSteps;
		const TCHAR* Why;
	};

	const FBoundaryCase Cases[] =
	{
		{ 0.f,    0.f, TEXT("standing ON the datum — flat grass, no bonus, exactly the base") },
		{ 1523.f, 0.f, TEXT("ONE uu BELOW the first step — ⛔ still 0 steps; a `round` would already say 1") },
		{ 1524.f, 1.f, TEXT("EXACTLY the first step — the boundary is INCLUSIVE") },
		{ 3047.f, 1.f, TEXT("ONE uu below the second step — ⛔ still 1; a `round` would say 2") },
		{ 3048.f, 2.f, TEXT("EXACTLY the second step") },
		{ 1600.f, 1.f, TEXT("⭐⭐ A HERO ON A HILL (Z = +1600) EARNS ONE STEP — the row that would have gone RED against the STRUCK downward-trace default, which read a hill as ~0 (J-F13, FOG-§10.7 (D))") },
		{ -500.f, 0.f, TEXT("⛔ BELOW the datum clamps to 0 steps — no malus, only no bonus; a negative window is unreachable") },
	};

	for (const FBoundaryCase& Case : Cases)
	{
		const float ExpectedSeconds = BaseSeconds + BonusPerStep * Case.ExpectedSteps;
		const float ActualSeconds = AFogVolume::BrightSunWindowSeconds(
			GroundZ + Case.HeightAboveGroundUU, GroundZ, BaseSeconds, BonusPerStep, StepUU);

		TestEqual(
			*FString::Printf(TEXT("⭐ %.0f uu above the datum ⇒ %.0f step(s) ⇒ %.0f s — %s"),
				Case.HeightAboveGroundUU, Case.ExpectedSteps, ExpectedSeconds, Case.Why),
			ActualSeconds, ExpectedSeconds);
	}

	// ── ⛔ UNCAPPED (✅ `J-F14`, ruled — he accepted the proceeding default explicitly). Asserted at
	//    an absurd height precisely because a cap would be invisible at every realistic one: the
	//    tallest reachable perch is a stacked Watch Tower at ~2,400 uu, i.e. ONE step, so a cap set
	//    anywhere above 1 step would pass every other row in this file.
	TestEqual(
		TEXT("⛔⛔ THE HEIGHT BONUS IS UNCAPPED (J-F14). 100 steps up earns 100 bonuses. ⛔ A cap would be INVISIBLE ")
		TEXT("to every other row here — the tallest reachable perch (a ×2 Watch Tower, ~2,400 uu) is ONE step — which ")
		TEXT("is exactly why this row is absurd on purpose."),
		AFogVolume::BrightSunWindowSeconds(GroundZ + 100.f * StepUU, GroundZ, BaseSeconds, BonusPerStep, StepUU),
		BaseSeconds + BonusPerStep * 100.f);

	// ── ⛔ TOTALITY: a designer-zeroed (or NaN) step degrades to EXACTLY the base, never to a
	//    divide by zero — the ASummonedUnit::HeightAdvantageMultiplier doctrine, and the reason the
	//    guard is spelled `!(X > 0.f)` rather than `(X <= 0.f)`.
	TestEqual(
		TEXT("⛔ A ZEROED step yields exactly the base window (no divide by zero, no infinite timer)."),
		AFogVolume::BrightSunWindowSeconds(GroundZ + 5000.f, GroundZ, BaseSeconds, BonusPerStep, 0.f), BaseSeconds);
	TestEqual(
		TEXT("⛔ A NEGATIVE step yields exactly the base window."),
		AFogVolume::BrightSunWindowSeconds(GroundZ + 5000.f, GroundZ, BaseSeconds, BonusPerStep, -1.f), BaseSeconds);
	// ⛔ THE NaN IS BUILT FROM ITS BIT PATTERN, never from `0.f/0.f` or `FMath::Sqrt(-1.f)` — both
	// are constant-foldable and a fast-math build may hand back something FINITE, which would make
	// the two rows below pass while testing nothing (the SiegeCastBarTest.cpp:125 lesson, reused).
	const float QuietNaN = MakeQuietNaN();
	TestFalse(
		TEXT("SELF-CHECK: the NaN these two rows feed really is non-finite. ⛔ A folded constant here would make both ")
		TEXT("of them vacuous, which is precisely how a totality guard gets certified without ever being exercised."),
		FMath::IsFinite(QuietNaN));

	TestEqual(
		TEXT("⛔ A NaN step yields exactly the base window — `!(X > 0.f)` catches it, `(X <= 0.f)` would NOT."),
		AFogVolume::BrightSunWindowSeconds(GroundZ + 5000.f, GroundZ, BaseSeconds, BonusPerStep, QuietNaN), BaseSeconds);
	TestEqual(
		TEXT("⛔ A NaN hero Z yields exactly the base window — a broken transform can never buy a longer window."),
		AFogVolume::BrightSunWindowSeconds(QuietNaN, GroundZ, BaseSeconds, BonusPerStep, StepUU), BaseSeconds);

	// ── ⛔⛔ ONE FORMULA, ONE COPY. `TASK-991` will call the DURATION accessor to build its
	//    "would reduce from X to Y" message; a SECOND copy of this arithmetic anywhere is the
	//    defect that makes the message start lying as the two drift.
	FString FogCpp;
	if (LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		TestEqual(
			TEXT("⛔⛔ THE FORMULA EXISTS IN EXACTLY ONE PLACE: `FMath::FloorToFloat(` appears ONCE in FogVolume.cpp. ")
			TEXT("A second `floor` is a second copy of the window arithmetic, and two copies drift until TASK-991's ")
			TEXT("refusal message starts quoting a Y the cast would not actually produce."),
			CountOccurrencesInCode(FogCpp, TEXT("FMath::FloorToFloat(")), 1);

		TestEqual(
			TEXT("SELF-CHECK: the pure static is defined exactly once."),
			CountOccurrencesInCode(FogCpp, TEXT("float AFogVolume::BrightSunWindowSeconds(")), 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐ — THE STEP IS ITS **OWN** CONSTANT, AND THE DATUM IS **NOT TRACED**.
//  ⛔ `FOG-§9.5`: never couple a card to a constant somebody else may retune for
//  an unrelated reason. `FogVisionCeilingUU` is 609.6 — which was ALSO this
//  card's step under the retired 20-ft reading — and `J-F12`'s own named remedy
//  for over-strong fog is "lower the ceiling". Referencing it here would make a
//  VISION decision silently retune a CARD.
//  ⛔ And ✅ `J-F13` closed the datum as a FLAT CONSTANT: there is no trace.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunDatumIsFlatAndDecoupledTest,
	"Siegebound.BrightSun.TheHeightStepIsItsOwnConstantAndTheGroundDatumIsNeverTraced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunDatumIsFlatAndDecoupledTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	FString FogCpp;
	FString FogH;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp) || !LoadProjectFile(*this, FogVolumeH, FogH))
	{
		return false;
	}

	// ── (a) ⛔⛔ NO TRACE, ANYWHERE IN THE CLASS (✅ J-F13). 📌 His words: *"The ground is simply the
	//    elevation of the flat grass terrain, NOT including the hills, therefore that ground height
	//    number should be the SAME ANYWHERE ON THE MAP."* ⛔ The pre-ruling default WAS a downward
	//    trace with buildings ignored; it is REFUTED, and this row is what stops it coming back.
	//    ⭐ It is asserted as an ABSENCE because that is the only shape the defect has: a trace here
	//    would read a HILL as ~0 and silently delete the height a hero on a hill has EARNED, while
	//    every duration test in test 1 stayed green.
	const TCHAR* const TraceNeedles[] =
	{
		TEXT("LineTraceSingleByChannel"),
		TEXT("LineTraceSingleByObjectType"),
		TEXT("SweepSingleByChannel"),
		TEXT("FHitResult"),
		TEXT("FCollisionQueryParams"),
		TEXT("ECC_"),
	};
	for (const TCHAR* const Needle : TraceNeedles)
	{
		TestEqual(
			*FString::Printf(
				TEXT("⛔⛔ ZERO `%s` in FogVolume.cpp — ✅ J-F13 ruled the datum a FLAT CONSTANT. There is no trace, no ")
				TEXT("trace channel and no missed-trace degrade path in this class at all. A trace would read a HILL as ")
				TEXT("~0 and delete the height he has earned there, with every duration assertion still green."),
				Needle),
			CountOccurrencesInCode(FogCpp, Needle), 0);
	}

	// ── (b) ⛔⛔ THE STEP NEVER REFERENCES THE FOG CEILING (FOG-§9.5 — an AUTOMATIC FAIL per this
	//    row's own spec). The two numbers were EQUAL (609.6) under the retired 20-ft reading, so
	//    "reuse the ceiling" was the tempting edit; his 50-ft amendment separated the values but
	//    NOT the hazard, because J-F12's remedy for over-strong fog is still "lower the ceiling".
	//    ⚠️ Counted on CODE lines only — `FogVolume.h`'s own doc EXPLAINS this rule by naming the
	//    symbol, which is precisely the case CountOccurrencesInCode exists to survive.
	//    ⛔ TASK-1041: the needle is now the BARE member, with `ApplyFogVisionCeilingUU` — which
	//    CONTAINS it — discounted. See `CountBareCeilingMemberReferences`. A volume that one day
	//    routes a reach through the unit-side door is not the coupling this row forbids, and the
	//    unhardened needle would have RED-lit it while naming a rule it had not broken.
	TestEqual(
		TEXT("⛔⛔ ZERO code references to `FogVisionCeilingUU` in FogVolume.cpp (FOG-§9.5). Coupling the card's height ")
		TEXT("step to the VISION ceiling would let J-F12's own named remedy — \"lower the ceiling\" — silently retune ")
		TEXT("BrightSun as a side effect of a vision decision."),
		CountBareCeilingMemberReferences(FogCpp), 0);
	TestEqual(
		TEXT("⛔ …and zero in FogVolume.h's declarations too (the header explains the rule in COMMENTS, which are skipped)."),
		CountBareCeilingMemberReferences(FogH), 0);

	// ── (c) ⛔ AND NOT VIA THE TUNING STRUCT EITHER — the indirect spelling of the same coupling.
	TestEqual(
		TEXT("⛔ ZERO `FSiegeFogTuning` in FogVolume.cpp: the state object does not carry, read or reach the vision ")
		TEXT("band. The seam (FSiegeCombatStatics::ReadFogState) owns that, and it asks this object exactly ONE ")
		TEXT("question — IsFogActive()."),
		CountOccurrencesInCode(FogCpp, TEXT("FSiegeFogTuning")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐⭐ — **THE DATUM TIE**: HIS SENTENCE MADE EXECUTABLE.
//  📌 "…it should be the SAME HEIGHT IN WHICH RANGED UNITS' DAMAGE IS AT 1 TIMES
//  THEIR DAMAGE AND INCREASES BY 10% EVERY 5 FEET ABOVE THAT."
//  ⛔⛔ THIS IS AN **ASSERTION, NEVER A CALL** (`FOG-§10.7` (D) rules 3 + 4). The
//  damage lane's zero is the TARGET'S OWN Z, per attack, per pair — a RELATIVE
//  condition true at any absolute elevation. Converting `ComputeOutputDamage` to
//  read `ArenaGroundReferenceZUU` would be a game-wide rebalance and would
//  contradict `HIGH-§2` row `R-1`, his own earlier ruling. ⛔ NOT ONE LINE of the
//  damage formula is changed by this row; this test is the entire tie.
//  ⭐ Without it the two lanes agree by COINCIDENCE and drift silently — the
//  defect class this batch has already hit three times.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunDatumTiesToTheDamageLaneTest,
	"Siegebound.BrightSun.TheGroundDatumIsTheSameHeightWhereElevationDamageIsExactlyOneTimes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunDatumTiesToTheDamageLaneTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	const AFogVolume* const FogDefaults = GetDefault<AFogVolume>();
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!FogDefaults || !UnitDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: a CDO returned null — the tie below is read off BOTH objects, so nothing after this would mean anything."));
		return false;
	}

	float GroundZ = -1.f;      // ⛔ seeded NON-zero: a failed read must not masquerade as the shipped 0
	float HeightStepUU = 0.f;
	float BonusPerStep = 0.f;
	if (!ReadFloatProperty(*this, FogDefaults, TEXT("ArenaGroundReferenceZUU"), GroundZ)
		|| !ReadFloatProperty(*this, UnitDefaults, TEXT("HeightBonusStepUU"), HeightStepUU)
		|| !ReadFloatProperty(*this, UnitDefaults, TEXT("HeightBonusPerStep"), BonusPerStep))
	{
		return false;
	}

	// ⛔ READ-ONLY, NEVER WRITTEN. `HeightBonusStepUU = 152.4f` is CORRECT and is NOT this row's:
	// 🧑 he corrected himself — *"keep it as a 10% increase in damage every 5 feet above the ground,
	// I mispoke when I said 10 feet"* (FOG-§10.7 (E)). ⛔ Taking the 10-ft sentence at face value
	// would have DOUBLED the step and HALVED every elevation damage bonus in the game, silently and
	// green. This row asserts the shipped values; it does not move them.
	TestEqual(
		TEXT("⛔ `ASummonedUnit::HeightBonusStepUU` is his FIVE FEET, re-derived: 5 × 30.48 = 152.4 uu. ⛔ NOT 10 ft — ")
		TEXT("he confirmed the misspeak himself (FOG-§10.7 (E)). ⛔ READ-ONLY here: BrightSun's step is a SEPARATE ")
		TEXT("1524 (50 ft), and the two must never be collapsed."),
		HeightStepUU, 5.f * 30.48f);

	TestEqual(
		TEXT("⛔ `ASummonedUnit::HeightBonusPerStep` is his \"+10%\" = 0.10, ADDITIVE and UNCOMPOUNDED."),
		BonusPerStep, 0.10f);

	// ⭐⭐⭐ THE TIE ITSELF — the SAME SYMBOL feeds both sides. If EITHER lane's datum moves, this
	// goes RED, which is the entire reason `ArenaGroundReferenceZUU` is ONE constant rather than
	// two that happen to agree today.
	TestEqual(
		TEXT("⭐⭐⭐ AT THE DATUM, ELEVATION DAMAGE IS EXACTLY ×1.0 — his sentence, executable: ")
		TEXT("HeightAdvantageMultiplier(ArenaGroundReferenceZUU, ArenaGroundReferenceZUU, …) == 1.0. ")
		TEXT("⛔ This is an ASSERTION, never a call: the damage formula is UNCHANGED and still measures against the ")
		TEXT("TARGET'S own Z (HIGH-§2 R-1). Red here means the two elevation lanes have started disagreeing about ")
		TEXT("where \"the ground\" is."),
		ASummonedUnit::HeightAdvantageMultiplier(GroundZ, GroundZ, HeightStepUU, BonusPerStep), 1.0f);

	TestEqual(
		TEXT("⭐⭐⭐ …AND ONE DAMAGE STEP (5 ft) ABOVE IT IS EXACTLY ×1.10 — \"increases by 10% every 5 feet above ")
		TEXT("that\", measured FROM THE SAME SYMBOL BrightSun measures its window from."),
		ASummonedUnit::HeightAdvantageMultiplier(GroundZ + HeightStepUU, GroundZ, HeightStepUU, BonusPerStep), 1.10f);

	// ⛔ AND THE FENCE, EXECUTED: the damage lane must NOT start reading this constant at runtime.
	// A `ArenaGroundReferenceZUU` appearing in SummonedUnit.cpp would be the game-wide rebalance
	// FOG-§10.7 (D) rule 4 calls an automatic fail — every attacker on a hill would gain a bonus
	// against a target standing on that same hill.
	FString UnitCpp;
	if (LoadProjectFile(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp"), UnitCpp))
	{
		TestEqual(
			TEXT("⛔⛔ ZERO `ArenaGroundReferenceZUU` in SummonedUnit.cpp — THE TIE IS AN ASSERTION, NEVER A CALL ")
			TEXT("(FOG-§10.7 (D) rule 4). Wiring the damage formula to an absolute world datum would give every ")
			TEXT("attacker on a hill a bonus against a target on that SAME hill, and would contradict HIGH-§2 R-1."),
			CountOccurrencesInCode(UnitCpp, TEXT("ArenaGroundReferenceZUU")), 0);
	}

	// ⚠️ REPORTED, ⛔ NOT SWEPT (FOG-§10.7 (D) rule 5): `FSiegeAssistantSnapshot::MarkPlaceGroundZ`
	// is a THIRD site holding this same 0.f today. It is deliberately NOT referenced from the fog
	// lane and NOT asserted equal here — its own comment licenses it to be retuned for hill-accurate
	// marks, i.e. it may legitimately STOP being the flat-grass datum, and pinning it would convert
	// a documented divergence into a false coupling. The handoff names it.

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐⭐ — **THE ONE-WAY DOOR**: `SHIELDED` ⇒ `CLEAR`, ⛔ NEVER ⇒ `FOGGED`.
//  📌 HIS WORDS: "Even when the 'bright sun' fog prevention timer ends, the fog
//  that was cleared STILL REMAINS CLEAR."
//  ⛔ Asserted at the ONE line that could break it: `ApplyBrightSun` must ZERO the
//  fog deadline rather than stash it. A "resume the fog" implementation is a FAIL
//  against his sentence, and it is spelled as a SAVED remainder — so this row
//  counts the zeroing and bans the stash.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunIsAOneWayDoorTest,
	"Siegebound.BrightSun.TheShieldExpiresToClearAndTheFogCanNeverComeBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunIsAOneWayDoorTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	FString ApplyBody;
	if (!ExtractFunctionBody(*this, FogCpp, TEXT("bool AFogVolume::ApplyBrightSun("), ApplyBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted ApplyBrightSun body is substantial — an empty extraction would make every ")
		TEXT("count below read as SAFE when it is really just blind."),
		ApplyBody.Len() > 200);

	// ── ⭐⭐⭐ THE DOOR ITSELF. The fog deadline is ZEROED, not suspended — so when the shield
	//    lapses there is nothing left to resume and `IsFogActive()` compares the clock against 0.0
	//    forever. ⛔ That is what makes the one-way door a PROPERTY rather than an expiry handler
	//    somebody could forget to write.
	TestEqual(
		TEXT("⭐⭐⭐ `ApplyBrightSun` ZEROES the fog deadline exactly once — HIS ONE-WAY DOOR: \"the fog that was ")
		TEXT("cleared STILL REMAINS CLEAR\". ⛔ A zero here means the card shields WITHOUT clearing (the fourth state ")
		TEXT("FOG-§10.3 forbids); a stashed remainder anywhere means the fog RESUMES, which is a FAIL against his ")
		TEXT("own sentence rather than a missing feature."),
		CountOccurrencesInCode(ApplyBody, TEXT("FogActiveUntilTimeSeconds = 0.0;")), 1);

	// ── ⛔ AND IT NEVER WRITES A NON-ZERO FOG DEADLINE. This is the "restore the fog" defect in its
	//    only possible spelling: any other assignment to that scalar from in here is a resume.
	TestEqual(
		TEXT("⛔⛔ `ApplyBrightSun` makes EXACTLY ONE assignment to the fog deadline, and test above proves it is the ")
		TEXT("ZERO. ⛔ A second assignment is a remembered fog — i.e. a resume — and there is no legitimate reason ")
		TEXT("for this function to ever write a non-zero fog deadline."),
		CountOccurrencesInCode(ApplyBody, TEXT("FogActiveUntilTimeSeconds =")), 1);

	// ── ⛔ NO STASH ANYWHERE IN THE CLASS. A "suspended fog" needs somewhere to live, and the only
	//    two deadlines in this class are pinned by the writer census in test 6.
	TestEqual(
		TEXT("⛔ ZERO `Suspended` / paused-fog storage in FogVolume.cpp — there is NO suspended fog, NO paused timer ")
		TEXT("and NO remembered remainder anywhere in this class (FOG-§10.3)."),
		CountOccurrencesInCode(FogCpp, TEXT("Suspended")), 0);

	// ── ⭐ THE SHIELD IS A RESET, NEVER AN ACCUMULATE — the same refresh-never-stack shape as
	//    RaiseFog. `+=` would let a player bank hours by re-casting from the same perch, which is
	//    exactly the ADDITIVE option J-F18's reasoning rejected.
	TestEqual(
		TEXT("⛔ `ApplyBrightSun` never ACCUMULATES the prevention deadline: ZERO `FogPreventedUntilTimeSeconds +=`. ")
		TEXT("J-F18's LONGER case is a RESET to the new window; an accumulate would let a player bank hours from one ")
		TEXT("perch, which is the ADDITIVE option his ruling rejected."),
		CountOccurrencesInCode(ApplyBody, TEXT("FogPreventedUntilTimeSeconds +=")), 0);

	TestEqual(
		TEXT("SELF-CHECK / LIVE CONTROL: it DOES stamp the prevention deadline, exactly once. A zero here would mean ")
		TEXT("the card clears the fog and shields NOBODY, and every ban above would be a meaningless zero."),
		CountOccurrencesInCode(ApplyBody, TEXT("FogPreventedUntilTimeSeconds =")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐⭐⭐ — THE FOUR INTERACTIONS, AND `J-F18` IS A **BRANCH**.
//  ✅ `J-F16` fog-on-fog ⇒ RESET to 300 · ✅ `J-F17` BrightSun with no fog ⇒ LEGAL
//  · ✅ `J-F18` sun-on-sun ⇒ CONDITIONAL (longer RESETS, shorter REFUSES) · ✅
//  `J-F19` fog-during-window ⇒ REFUSED, no gold, card kept.
//  ⛔ The `max(remaining, new)` default is STRUCK, and the delta is invisible to
//  any DURATION test: `max` and "reset if longer" give the IDENTICAL remaining
//  time in the longer case. They diverge ONLY in the shorter case — where `max`
//  silently kept the timer while BILLING 60 gold and EATING the card. ⇒ the only
//  assertion that can see the difference is a STRUCTURAL one about the branch.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunInteractionsTest,
	"Siegebound.BrightSun.SunOnSunIsAConditionalBranchAndFogDuringTheWindowIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunInteractionsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	FString ApplyBody;
	FString RaiseBody;
	if (!ExtractFunctionBody(*this, FogCpp, TEXT("bool AFogVolume::ApplyBrightSun("), ApplyBody)
		|| !ExtractFunctionBody(*this, FogCpp, TEXT("bool AFogVolume::RaiseFog("), RaiseBody))
	{
		return false;
	}

	// ── (a) ✅ `J-F18` IS A BRANCH, ⛔ NOT `max`. The struck default's arithmetic was RIGHT and its
	//    ECONOMICS were wrong — so the ban is on the SPELLING, because no duration assertion can
	//    tell the two apart. ⛔ `FMath::Max` here would restore the version that keeps the timer
	//    while billing 60 gold and eating the card.
	TestEqual(
		TEXT("⛔⛔ ✅ J-F18 — `ApplyBrightSun` does NOT use the struck `max(remaining, new)` refresh: ZERO `FMath::Max`. ")
		TEXT("⭐ THIS IS THE ONLY ASSERTION THAT CAN SEE THE RULING: `max` and \"reset if longer\" produce the IDENTICAL ")
		TEXT("remaining time in the LONGER case, so every duration test passes either way. They diverge only in the ")
		TEXT("SHORTER case — where `max` silently keeps the timer while BILLING 60 gold and EATING the card."),
		CountOccurrencesInCode(ApplyBody, TEXT("FMath::Max")), 0);

	TestEqual(
		TEXT("⭐ …and the branch reads the LIVE remainder to decide, exactly once (his \"the current time left\"). ")
		TEXT("⛔ A cached remainder would make the decision on a stale number."),
		CountOccurrencesInCode(ApplyBody, TEXT("GetFogPreventionSecondsRemaining()")), 1);

	TestEqual(
		TEXT("⭐⭐ …and it computes the NEW window through the DURATION ACCESSOR rather than inline, exactly once — ")
		TEXT("which is what makes the SAME number reachable by TASK-991 WITHOUT casting the card (FOG-§10.7 (A))."),
		CountOccurrencesInCode(ApplyBody, TEXT("GetBrightSunWindowSeconds(")), 1);

	TestEqual(
		TEXT("⛔ …and the cast path does NOT re-implement the formula: ZERO `FMath::FloorToFloat` inside ")
		TEXT("ApplyBrightSun. One formula, one copy (test 1 pins the file-wide count at 1)."),
		CountOccurrencesInCode(ApplyBody, TEXT("FMath::FloorToFloat")), 0);

	// ── (b) ✅ `J-F19` — FOG DURING A PREVENTION WINDOW IS REFUSED, AND THE GUARD IS IN THE ONE
	//    WRITER. ⛔ Its absence is not a missing feature: it is the FOURTH STATE (fogged AND
	//    shielded), reachable in one cast, with nothing red anywhere.
	TestEqual(
		TEXT("⛔⛔ ✅ J-F19 — `RaiseFog` consults the prevention window exactly once and refuses while it is up ")
		TEXT("(\"prevention will not allow any new fog to come in\"). ⛔ Without this guard a Fog cast during a shield ")
		TEXT("produces FOGGED-AND-SHIELDED — FOG-§10.3's forbidden fourth state — in one click, with no red anywhere."),
		CountOccurrencesInCode(RaiseBody, TEXT("IsFogPrevented()")), 1);

	// ── (c) ✅ `J-F16` — FOG ON FOG IS STILL AN UNCONDITIONAL RESET. ⛔ The guard added in (b) must
	//    not have grown an "already fogged" companion: his ruling is that a re-cast RESETS the full
	//    5 minutes, and an `if (IsFogActive())` branch here is how that quietly becomes a no-op.
	TestEqual(
		TEXT("⛔ ✅ J-F16 — `RaiseFog` has NO \"already fogged\" branch: ZERO `IsFogActive()`. A second Fog cast RESETS ")
		TEXT("the full 5 minutes (his words); the only thing that can refuse it is BrightSun's window."),
		CountOccurrencesInCode(RaiseBody, TEXT("IsFogActive()")), 0);

	// ── (d) ✅ `J-F17` — BrightSun with NO fog up is LEGAL, and the way that dies is the READ door.
	//    The very first cast of a match may be this card (a pre-emptive play is a good play), and no
	//    AFogVolume exists until something spawns one. ⛔ A `Find` in the FogClear arm would make a
	//    60-gold pre-emptive cast a SILENT no-op — banned in every branch of this mechanic.
	FString SpellCpp;
	if (!LoadProjectFile(*this, SpellLibraryCpp, SpellCpp))
	{
		return false;
	}

	FString ResolveBody;
	if (!ExtractFunctionBody(*this, SpellCpp, TEXT("bool USpellLibrary::ResolveSpell("), ResolveBody))
	{
		return false;
	}

	TestEqual(
		TEXT("SELF-CHECK / LIVE CONTROL: the BrightSun arm exists in ResolveSpell exactly once."),
		CountOccurrencesInCode(ResolveBody, TEXT("case ESpellEffect::FogClear:")), 1);

	TestEqual(
		TEXT("⭐⭐ ✅ J-F17 — the BrightSun arm reaches the state object exactly once, through ApplyBrightSun. ⛔ A zero ")
		TEXT("here is a 60-gold card that spends the gold, spawns its VFX, logs \"resolved\" and shields NOBODY."),
		CountOccurrencesInCode(ResolveBody, TEXT("->ApplyBrightSun(")), 1);

	TestEqual(
		TEXT("⛔⛔ ✅ J-F17 — ResolveSpell NEVER uses the READ door (`AFogVolume::Find(`). BrightSun is LEGAL with no ")
		TEXT("fog up — the prevention window ALONE is a good pre-emptive play — so the FIRST cast of a match may be ")
		TEXT("this card, with no state actor in the world yet. A `Find` would make that cast a SILENT no-op, and a ")
		TEXT("silent no-op is banned in every branch of this mechanic."),
		CountOccurrencesInCode(ResolveBody, TEXT("AFogVolume::Find(")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 ⭐⭐⭐ — **NO GOLD MOVES AND THE CARD IS NOT CONSUMED.** ⛔ TWO
//  ASSERTIONS, ⛔ NEVER ONE (`FOG-§10.6`): a build that refunds the gold but EATS
//  THE CARD satisfies exactly half his ruling, and a test written against either
//  half alone PASSES the broken build.
//  ⭐ Neither property is invented here — both are the SHIPPED `RefuseCardPlay`
//  net-zero doctrine, reached by returning false. This row proves the fog arms
//  really return false and that the shipped doctrine really delivers both halves.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunRefusalIsNetZeroTest,
	"Siegebound.BrightSun.ARefusedFogOrSunSpendsZeroGoldAndKeepsTheCardInHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunRefusalIsNetZeroTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	FString FogCpp;
	FString SpellCpp;
	FString ControllerCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp)
		|| !LoadProjectFile(*this, SpellLibraryCpp, SpellCpp)
		|| !LoadProjectFile(*this, PlayerControllerCpp, ControllerCpp))
	{
		return false;
	}

	// ── (a) ⛔ THE WRITER CENSUS. Both deadlines have exactly THREE and TWO assignment sites, and
	//    naming them is what makes "nothing was written on the refusal path" checkable at all:
	//    fog = RaiseFog(1) + ApplyBrightSun's zero(1) + ResetFog(1); prevention = ApplyBrightSun(1)
	//    + ResetFog(1). ⛔ A FOURTH writer is a second place the state machine can be moved from,
	//    and it is how a refusal starts leaving a partial stamp behind.
	TestEqual(
		TEXT("⛔⛔ THE FOG DEADLINE HAS EXACTLY THREE WRITE SITES IN THE PROJECT: RaiseFog, ApplyBrightSun's zeroing, ")
		TEXT("and ResetFog. ⛔ A fourth is a second place the machine can be moved from — and the refusal paths ")
		TEXT("asserted below are only meaningful because this census bounds them."),
		CountOccurrencesInCode(FogCpp, TEXT("FogActiveUntilTimeSeconds =")), 3);

	TestEqual(
		TEXT("⛔⛔ THE PREVENTION DEADLINE HAS EXACTLY TWO WRITE SITES: ApplyBrightSun and ResetFog."),
		CountOccurrencesInCode(FogCpp, TEXT("FogPreventedUntilTimeSeconds =")), 2);

	// ── (b) ⛔ BOTH FOG ARMS REFUSE BY RETURNING FALSE, so the caller's shipped net-zero doctrine
	//    runs. ⛔ MEASURED CENSUS of `return false;` inside ResolveSpell = 8: three pre-existing
	//    guards (null world, None CardID, unresolvable effect), the shared `!bResolved` tail, and
	//    TWO PER FOG ARM (no state actor / the ruling refusal). ⛔ PIN MOVED 4 ⇒ 8 by this row's own
	//    four additions — declared rather than discovered. A DROP means a refusal started falling
	//    through to `bResolved = true`, i.e. spending gold and spawning VFX for a spell that did
	//    nothing; a RISE means a new silent refusal path nobody has ruled on.
	FString ResolveBody;
	if (!ExtractFunctionBody(*this, SpellCpp, TEXT("bool USpellLibrary::ResolveSpell("), ResolveBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔⛔ `ResolveSpell` refuses in exactly EIGHT places: null world, None CardID, unresolvable effect, the ")
		TEXT("shared `!bResolved` tail, and TWO PER FOG ARM (no state actor / the ruling refusal). ⛔ A DROP means a ")
		TEXT("refusal now falls through to `bResolved = true` — spending the gold, spawning NS_Spell_<CardID>, ")
		TEXT("logging \"resolved\", and changing NOTHING. ⛔ A RISE means a new silent refusal path nobody has ruled on."),
		CountOccurrencesInCode(ResolveBody, TEXT("return false;")), 8);

	// ── (c) ⭐⭐ PROPERTY 1 — ZERO GOLD MOVES. Both shipped resolver call sites refund in full on a
	//    false, through the choke-pointed gold API. ⛔ Asserted at the call sites rather than
	//    described, because "the caller refunds" is precisely the sentence that rots.
	FString InstantBody;
	if (ExtractFunctionBody(*this, ControllerCpp, TEXT("void ASiegePlayerController::ResolveSpellInstant("), InstantBody))
	{
		TestEqual(
			TEXT("⭐ PROPERTY 1 (instant path): a resolver refusal refunds the FULL cost — `AddGold(Row.Cost)`, once. ")
			TEXT("⛔ Without it, fog would be the only card in the game that bills you for a refusal."),
			CountOccurrencesInCode(InstantBody, TEXT("AddGold(Row.Cost)")), 1);

		// ⭐⭐ PROPERTY 2 — THE CARD IS NOT CONSUMED, and it is a SEPARATE assertion because a build
		// can pass property 1 and still eat the card. Proven by ORDER: the hand step sits AFTER the
		// refusal's early `return`, so a refused play can never reach it.
		const int32 RefundIndex = InstantBody.Find(TEXT("AddGold(Row.Cost)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		const int32 DrawIndex = InstantBody.Find(TEXT("ConfirmInstantDraw("), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		TestTrue(
			TEXT("⭐⭐ PROPERTY 2 (instant path): THE CARD IS NOT CONSUMED. `ConfirmInstantDraw` sits AFTER the refusal's ")
			TEXT("refund-and-return, so a refused play can never reach the hand step. ⛔ TWO assertions, never one: a ")
			TEXT("build that refunded the gold but EATS THE CARD satisfies half his ruling, and a test written against ")
			TEXT("either half alone PASSES the broken build."),
			RefundIndex != INDEX_NONE && DrawIndex != INDEX_NONE && RefundIndex < DrawIndex);
	}

	FString ConfirmBody;
	if (ExtractFunctionBody(*this, ControllerCpp, TEXT("void ASiegePlayerController::TryConfirmSpellTarget("), ConfirmBody))
	{
		TestEqual(
			// ⛔ PROSE RE-POINTED BY TASK-1018, ⛔ ASSERTION UNTOUCHED (the count is still 1 and it is still
			// the only refund site in this function). This used to read "— where `Fog` and `BrightSun`
			// resolve today"; that clause went FALSE the moment the routing stopped naming `GoldSteal`, and
			// both fog cards now take the INSTANT path above. ⛔ The row is kept because this path still
			// carries every AIMED spell (`Fireball`, `FrostNova`, `Lightning`, `BattleCry`) and losing the
			// refund would bill a player for a refusal on any of them.
			TEXT("⭐ PROPERTY 1 (targeting path — every AIMED spell; ⛔ `Fog`/`BrightSun` moved to the ")
			TEXT("INSTANT path in TASK-1018 and are covered by PROPERTY 1 above): a resolver refusal ")
			TEXT("refunds the FULL cost, `AddGold(TargetingCost)`, exactly once. ⛔ MEASURED at 1, not 2: the ")
			TEXT("confirm-time affordability guard above it never SPENT, so it has nothing to give back — the only ")
			TEXT("refund site is the resolver refusal, and losing it bills a player for a refusal."),
			CountOccurrencesInCode(ConfirmBody, TEXT("AddGold(TargetingCost)")), 1);

		const int32 RefusalIndex = ConfirmBody.Find(TEXT("USpellLibrary::ResolveSpell("), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		const int32 ConsumeIndex = ConfirmBody.Find(TEXT("ConfirmPlayFromHand("), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		TestTrue(
			// ⛔ Same TASK-1018 re-point: "a refused Fog/BrightSun" named the wrong path after the routing
			// fix. ⛔ Assertion and ordering claim UNCHANGED — this is still the aimed spells' only
			// card-consumption guard, and `J-F19`'s own guarantee is now proven on the INSTANT path by
			// PROPERTY 2 above (`ConfirmInstantDraw` after the refund-and-return).
			TEXT("⭐⭐ PROPERTY 2 (targeting path): THE CARD IS NOT CONSUMED. `ConfirmPlayFromHand` sits AFTER the ")
			TEXT("`ResolveSpell` refusal's refund-and-exit, so a refused AIMED spell leaves the slot untouched — ")
			TEXT("the M2 \"the card leaves the hand at CONFIRM\" law doing exactly what J-F19 asks for."),
			RefusalIndex != INDEX_NONE && ConsumeIndex != INDEX_NONE && RefusalIndex < ConsumeIndex);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 ⭐⭐⭐ — `Play Again` ⇒ CLEAR, **BOTH** TIMERS ZEROED — AND THE GAME
//  MODE STILL LEARNS **NO FOG POLICY** (`FOG-§10.3`; the `SC-§62` exception
//  granted to `TASK-998`, which `TASK-982` had to PRESERVE rather than merely
//  not-break).
//  ⭐ The exception's CONDITION is executable, which is why it is asserted rather
//  than promised: the game mode may tell the volume to clear; it may not know a
//  duration, a ceiling, a density or a window. This row would go red the moment a
//  second timer leaked a second policy value into `PlayAgain`.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunPlayAgainClearsBothTimersTest,
	"Siegebound.BrightSun.PlayAgainZeroesBothTimersWithoutTeachingTheGameModeAnyFogPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunPlayAgainClearsBothTimersTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	FString FogCpp;
	FString GameMode;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp) || !LoadProjectFile(*this, GameModeCpp, GameMode))
	{
		return false;
	}

	FString ResetBody;
	if (!ExtractFunctionBody(*this, FogCpp, TEXT("void AFogVolume::ResetFog("), ResetBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔ ResetFog zeroes the FOG deadline (FOG-§10.3: \"CLEAR, both timers zeroed\")."),
		CountOccurrencesInCode(ResetBody, TEXT("FogActiveUntilTimeSeconds = 0.0;")), 1);

	TestEqual(
		TEXT("⭐⭐ …AND THE PREVENTION DEADLINE — the half TASK-982 added, and the half that would rot silently. ")
		TEXT("⛔ Without it a match-2 player inherits match-1 IMMUNITY to fog: the Fog card refuses for up to several ")
		TEXT("minutes of a match nobody cast BrightSun in, and every other row in this file stays green."),
		CountOccurrencesInCode(ResetBody, TEXT("FogPreventedUntilTimeSeconds = 0.0;")), 1);

	// ── ⭐⭐⭐ THE `SC-§62` EXCEPTION'S CONDITION, EXECUTED. `TASK-998` was granted its game-mode
	//    edit on the condition that the game mode learns NO fog policy. `TASK-982` doubled the
	//    state and had to keep that true — so the call site stays `It->ResetFog();` and the SECOND
	//    zero happens inside the volume. ⛔ If a future row "helpfully" adds a
	//    `ClearFogPrevention()` loop, or a duration, or a ceiling, to PlayAgain, these go red.
	FString PlayAgainBody;
	if (!ExtractFunctionBody(*this, GameMode, TEXT("void ASiegeGameMode::PlayAgain("), PlayAgainBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted PlayAgain body is substantial — an empty extraction would make every claim ")
		TEXT("below vacuous."),
		PlayAgainBody.Len() > 200);

	TestEqual(
		TEXT("SELF-CHECK / LIVE CONTROL: PlayAgain still calls ResetFog exactly once, and the call site is ")
		TEXT("BYTE-UNCHANGED by TASK-982 — one entry point for a state object that now holds two timers."),
		CountOccurrencesInCode(PlayAgainBody, TEXT("It->ResetFog();")), 1);

	// ⛔ TASK-1041 — THE SUBSTRING SWEEP, AND THE REASON THE DISCOUNT IS ⛔ PER-ENTRY.
	//    Each needle now carries an optional PERMITTED SUPERSTRING: a LONGER spelling that CONTAINS
	//    the needle and is ⛔ NOT the violation. ⛔ Exactly one entry needs it, and it is the one
	//    TASK-1008's door created — `ApplyFogVisionCeilingUU` CONTAINS `FogVisionCeilingUU`.
	//    ⛔⛔ NEVER a blanket "strip superstrings" rule: `FogPrevent` is a PREFIX needle ⛔ ON PURPOSE
	//    (it exists to catch `FogPreventedUntilTimeSeconds`, and TEST 7 above depends on that), so a
	//    generic version of this fix would have DELETED a live guard while looking identical.
	struct FPolicyNeedle
	{
		/** The forbidden spelling. */
		const TCHAR* Needle;
		/** A longer spelling that CONTAINS `Needle` and is legal — discounted. `nullptr` = none. */
		const TCHAR* PermittedSuperstring;
	};

	const FPolicyNeedle PolicyNeedles[] =
	{
		{ TEXT("BrightSun"),          nullptr },
		{ TEXT("FogPrevent"),         nullptr },
		{ TEXT("FogDurationSeconds"), nullptr },
		{ TEXT("FogVisionCeilingUU"), TEXT("ApplyFogVisionCeilingUU") },
		{ TEXT("FSiegeFogTuning"),    nullptr },
	};
	for (const FPolicyNeedle& Policy : PolicyNeedles)
	{
		const int32 PermittedCount = (Policy.PermittedSuperstring != nullptr)
			? CountOccurrencesInCode(GameMode, Policy.PermittedSuperstring)
			: 0;

		TestEqual(
			*FString::Printf(
				TEXT("⭐⭐⭐ SC-§62 CONDITION HELD: ZERO `%s` anywhere in SiegeGameMode.cpp. The game mode may TELL the ")
				TEXT("volume to clear; it may NOT know a duration, a ceiling, a density or a window. ⛔ TASK-982 doubled ")
				TEXT("the state and leaked none of it here — which is the whole reason the second zero lives inside ")
				TEXT("ResetFog rather than in a second loop up there."),
				Policy.Needle),
			CountOccurrencesInCode(GameMode, Policy.Needle) - PermittedCount, 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 ⭐⭐⭐ — **THE TWO ACCESSORS `TASK-989` AND `TASK-991` ARE BLOCKED ON.**
//  ⛔ `SC-§40` cl. 2 is satisfied by BOARDED callers, not by callers in this diff:
//  this row ships both with no caller of its own, on purpose, because the seam is
//  the ONE thing a later row cannot add for itself.
//  ⛔ Both must be `const` and side-effect-free: `TASK-991` must compute a full
//  window WITHOUT CASTING THE CARD, and `TASK-989` must read the remainder at
//  CLICK time. A duration computed inside the cast branch would force one of them
//  to duplicate the formula or to cast the card to find out whether to cast it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBrightSunSeamIsCallableOutsideTheCastPathTest,
	"Siegebound.BrightSun.BothAccessorsAreConstAndCallableWithoutCastingTheCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBrightSunSeamIsCallableOutsideTheCastPathTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBrightSunFixture;

	FString FogH;
	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeH, FogH) || !LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	// ⛔ THE PINNED SIGNATURES, character-for-character. TASK-989 and TASK-991 compile against
	// these; a silent change here breaks two boarded rows that cannot see this file's diff.
	TestEqual(
		TEXT("⭐⭐ `float GetFogPreventionSecondsRemaining() const;` is declared — TASK-989's ONLY dependency on this ")
		TEXT("row, and TASK-991's `X`. ⛔ `const` is the load-bearing word: it is read at CLICK time, in a refusal ")
		TEXT("path, and must change nothing."),
		CountOccurrencesInCode(FogH, TEXT("float GetFogPreventionSecondsRemaining() const;")), 1);

	TestEqual(
		TEXT("⭐⭐ `float GetBrightSunWindowSeconds(ETeamId CasterTeam) const;` is declared — the DURATION accessor ")
		TEXT("(item 5a), TASK-991's `Y`. ⛔⛔ CALLABLE OUTSIDE THE CAST PATH: without it TASK-991 must either ")
		TEXT("DUPLICATE the formula or CAST THE CARD TO FIND OUT WHETHER TO CAST IT, and both are defects."),
		CountOccurrencesInCode(FogH, TEXT("float GetBrightSunWindowSeconds(ETeamId CasterTeam) const;")), 1);

	TestEqual(
		TEXT("⭐ …and the pure static behind it is public and takes every tunable as a parameter (⛔ no defaults — ")
		TEXT("SC-§33), which is what lets test 1 assert the step boundaries with no world at all."),
		CountOccurrencesInCode(FogH, TEXT("static float BrightSunWindowSeconds(float HeroZUU, float GroundReferenceZUU, float BaseSeconds, float BonusSecondsPerStep, float HeightStepUU);")), 1);

	// ⛔⛔ THE REMAINDER IS LIVE, NEVER CACHED (FOG-§10.6). It is computed from the clock on every
	// call — his ruling is that the message carries "the ACTUAL amount of time left", and a value
	// captured at cast time would be stale by exactly the elapsed duration.
	// ⚠️ SC-§37, stated as a limitation rather than hidden: a SINGLE-CLICK test cannot tell a live
	// read from a cached one. This structural row can, because a cache is a MEMBER — so the census
	// in test 6 (two deadlines, five write sites) is what actually forbids one.
	FString RemainderBody;
	if (ExtractFunctionBody(*this, FogCpp, TEXT("float AFogVolume::GetFogPreventionSecondsRemaining("), RemainderBody))
	{
		TestEqual(
			TEXT("⭐⭐ THE REMAINDER IS RECOMPUTED FROM THE CLOCK ON EVERY CALL: `GetTimeSeconds()`, once. ⛔ A cached ")
			TEXT("remainder would make TASK-989's message count down from the wrong number, or never change at all."),
			CountOccurrencesInCode(RemainderBody, TEXT("GetTimeSeconds()")), 1);
	}

	// ⛔ AND THE DURATION ACCESSOR SAMPLES HEIGHT LIVE — TASK-991's gate calls a Y that does NOT
	// re-sample height a BLOCKER. The sample is a hero lookup; asserting it is present is what
	// distinguishes "computes a window" from "returns the base forever".
	FString WindowBody;
	if (ExtractFunctionBody(*this, FogCpp, TEXT("float AFogVolume::GetBrightSunWindowSeconds("), WindowBody))
	{
		TestEqual(
			TEXT("⭐⭐ THE DURATION ACCESSOR SAMPLES THE HERO LIVE on every call — `TActorIterator<AHeroCharacter>`, ")
			TEXT("once. ⛔ Its answer CHANGES as he climbs, which is exactly what makes it usable for TASK-991's ")
			TEXT("refusal message. ⛔ ApplyBrightSun is the ONLY place the answer is ever frozen (J-F15: sampled once, ")
			TEXT("at the cast)."),
			CountOccurrencesInCode(WindowBody, TEXT("TActorIterator<AHeroCharacter>")), 1);

		TestEqual(
			TEXT("⛔ …and it writes NOTHING: zero assignments to either deadline inside a `const` accessor (belt and ")
			TEXT("braces — `const` already forbids it; this row states the intent for a future editor who is tempted ")
			TEXT("to cache the answer in a mutable member)."),
			CountOccurrencesInCode(WindowBody, TEXT("UntilTimeSeconds =")), 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
