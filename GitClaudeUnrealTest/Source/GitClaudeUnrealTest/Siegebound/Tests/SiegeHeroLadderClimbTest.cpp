// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include <type_traits> // TASK-787: the four-method ILadderClimber surface is pinned by member-function-pointer identity — the SiegeLadderClimbTest.cpp:5 idiom, second application

#include "Components/CapsuleComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/ClimbableTower.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/LadderClimber.h"
#include "Siegebound/SiegeLadderClimbStatics.h"
#include "Siegebound/SummonedUnit.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE HERO'S LADDER CLIMB (TASK-778, `CONTACT-§3`) ═══
 *
 *  Jonathan, verbatim (2026-09-01): "lets just make sure the playable character and any units can
 *  climb the ladder by simplying walking up to it and walking against it."
 *
 *  Subject: `AHeroCharacter`'s climb driver, its TEN exits, its watchdog and its contact call site,
 *  plus `ESiegeHeroLadderExit`. QA gate: TASK-779. Compile + suite run: TASK-780.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ WHY HALF OF THIS FILE SCANS SOURCE TEXT — A MEASUREMENT, ⛔ NOT A PREFERENCE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • Every automation test in this project is HEADLESS: there is not one `UWorld::CreateWorld`
 *      and not one `SpawnActor` in `Siegebound/Tests/` (stated at `SiegeLadderClimbTest.cpp:39`).
 *    • ⛔ AND A WORLD-LESS `AHeroCharacter` CANNOT BE DRIVEN THROUGH THIS FEATURE AT ALL: the
 *      driver sets a movement mode, arms an `FTimerManager` entry on the world, reads
 *      `UWorld::GetTimeSeconds` and moves a capsule. `UCharacterMovementComponent::
 *      SetDefaultMovementMode` reaches `UMovementComponent::GetPhysicsVolume`, which dereferences
 *      `GetWorld()` UNCONDITIONALLY when `UpdatedComponent` is null — a `NewObject`'d hero would
 *      CRASH the suite, ⛔ not fail it.
 *    ⇒ So the DECISIONS are asserted against the pure statics the driver consumes, the SHAPE is
 *      asserted by reflection over the shipped class, and THE TEN EXITS — which are the whole
 *      feature and cannot be reached any other way — are asserted by reading the shipped source
 *      between named function boundaries. ⭐ That instrument is the `SiegeRecallTest.cpp` test 12
 *      precedent, second application, and the split is STATED rather than hidden (`SC-§32`: a
 *      mechanism never observed to function is not known to function; the RUNTIME observation is
 *      TASK-780's PIE session).
 *
 *  ⚠️ AND THE LESSON THIS FILE IS WRITTEN AGAINST (`SHIP-§9c`): AN ASSERTION WHOSE TWO SIDES ARE
 *  EQUAL BY CONSTRUCTION PROVES NOTHING. Every probe below either (a) carries a POSITIVE CONTROL
 *  that fails when the probe goes stale, (b) re-derives its expectation from the law's own
 *  arithmetic rather than transcribing it from the subject, or (c) is a truth table where dropping
 *  a term flips a row. ⭐ Four assertions elsewhere in this project stopped discriminating this
 *  week; the controls below exist so these cannot join them quietly.
 *
 *  ⛔⛔ WHAT THIS FILE DOES ⛔ NOT CLAIM, SAID OUT LOUD: it does ⛔ NOT prove a hero climbs. The
 *  tower's entry gate admits `ASummonedUnit` ONLY (`ClimbableTower.cpp:650-656`), so the hero's
 *  poll is REFUSED at runtime today with the verdict `NotAnAdmittedClimber` — the declared blocker in
 *  `handoffs/TASK-778-programmer.md`. These tests prove the hero's half is COMPLETE and CORRECT the
 *  instant that seam widens; the runtime proof is TASK-780's.
 *
 *  🔒 Airlock: no `Capture()`, no `EnsureSnapshot()`, Zone A untouched, ⛔ no token figure
 *  (`AS-§12g`). Nothing here runs a model, opens the editor, or touches Git.
 */

namespace SiegeHeroLadderClimbTestFixture
{
	/** Tolerance for float comparisons that are not exact by nature. Tight enough that any real mistake blows through it. */
	constexpr float Tolerance = 1.e-3f;

	/** Exact-equality tolerance, for the claims whose whole content is the word EXACTLY. */
	constexpr float Exact = 0.f;

	/**
	 *  ⭐ THE PINNED CLIMB LINE AS A ***DELTA***, ⛔ NOT AS TWO ABSOLUTE ENDPOINTS — and that choice
	 *  is deliberate and load-bearing: `TOWER-§8.3`'s `LadderFoot(−450,0,0)` → `LadderTop(−150,0,1200)`
	 *  is being TRANSLATED OUTWARD by TASK-783 (Jonathan's `K-1` option A), and the size of that move
	 *  has ⛔ already changed once during this batch (a ~4.6 uu estimate became a larger build target)
	 *  — which is precisely why ⛔ no figure for it is written anywhere in this suite or in the
	 *  shipped driver. A PURE TRANSLATION leaves `Δ = (300,0,1200)` UNCHANGED, so every expectation
	 *  below survives it, exactly as the law argues when it prices option A. ⛔ Absolute endpoints
	 *  here would have gone stale the day the mesh landed and would have failed for a reason that is
	 *  ⛔ not a defect (`CONTACT-§10.1` cite-rot, in test form).
	 */
	const FVector PinnedClimbDelta(300.f, 0.f, 1200.f);

	/** An arbitrary, non-zero foot position. Nothing may depend on it — that is the point of the delta above. */
	const FVector ArbitraryFoot(-450.f, 0.f, 0.f);

	/** |Δ| = sqrt(300² + 1200²). Re-derived here rather than copied from the law, so the two can disagree. */
	const float PinnedLineLengthUU = FMath::Sqrt(300.f * 300.f + 1200.f * 1200.f); // 1236.93169

	/** The hero's capsule half-height, from `GitClaudeUnrealTestCharacter.cpp:18`'s `InitCapsuleSize(42.f, 96.0f)`. ⛔ NOT read from the subject — test 14 is where these two meet. */
	constexpr float HeroCapsuleHalfHeightFromTheTemplate = 96.f;

	/** The hero's capsule radius — the number Jonathan's whole `K-1` ruling turns on (42 > 37.624 ⇒ the standoff failed by 4.376 uu). */
	constexpr float HeroCapsuleRadiusFromTheTemplate = 42.f;

	/** The UNIT's half-height. Present ONLY so the hero's re-derivations can be shown to DIFFER from it (`CONTACT-§3.3`: ⛔ never inherit a number from the unit). */
	constexpr float UnitCapsuleHalfHeight = 88.f;

	/** The shipped ladder rate from `TOWER-§8.5` (the Archer/Wizard `Speed` cell). Test 16 is the one place this and the shipped property meet. */
	constexpr float ShippedClimbRateFromTheLaw = 350.f;

	/** Loads one of the shipped source files this suite reads. Reports and returns false rather than passing quietly — a probe that cannot read its subject must FAIL, ⛔ never report SAFE. */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(
				TEXT("⛔ Could not find '%s'. This probe reads the SHIPPED source because the ten exits cannot be reached headlessly; a probe that cannot read its subject FAILS."),
				*FullPath));
			return false;
		}

		if (!FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — see above; a stale probe fails."), *FullPath));
			return false;
		}

		return true;
	}

	/** Convenience: the hero's implementation file, which every exit test reads. */
	static bool LoadHeroCpp(FAutomationTestBase& Test, FString& OutText)
	{
		return LoadProjectSource(Test, TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp"), OutText);
	}

	/**
	 *  ⭐ Extracts ONE function's body: from its signature to the first line that closes at column 0
	 *  (`\n}`), which is how every function in this codebase ends. ⛔ Deliberately NOT a
	 *  brace-matcher: a brace-matcher walks into comments and string literals and then fails for a
	 *  reason that has nothing to do with the claim under test.
	 *
	 *  ⭐⭐ IT CARRIES ITS OWN POSITIVE CONTROL: a renamed or deleted function is an ERROR naming the
	 *  exit, and a body too small to contain anything is an ERROR too — so an exit test can never
	 *  pass by finding nothing.
	 */
	static bool ExtractFunctionBody(FAutomationTestBase& Test, const FString& Source, const TCHAR* Signature, FString& OutBody)
	{
		const int32 SignatureIndex = Source.Find(Signature, ESearchCase::CaseSensitive);
		if (SignatureIndex == INDEX_NONE)
		{
			Test.AddError(FString::Printf(
				TEXT("⛔ '%s' is not in the shipped source. The exit this test names either moved or was deleted — ⛔ neither is a pass."),
				Signature));
			return false;
		}

		const int32 BodyEnd = Source.Find(TEXT("\n}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SignatureIndex);
		if (BodyEnd == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find the end of '%s' — the probe is stale, so it fails."), Signature));
			return false;
		}

		OutBody = Source.Mid(SignatureIndex, BodyEnd - SignatureIndex);

		// POSITIVE CONTROL: the extraction actually captured a body, ⛔ not an empty sliver that
		// would make every "contains" assertion below vacuously false and every "does not contain"
		// one vacuously true.
		if (OutBody.Len() < 40)
		{
			Test.AddError(FString::Printf(
				TEXT("⛔ '%s' extracted only %d characters — the probe is broken, ⛔ not the subject."),
				Signature, OutBody.Len()));
			return false;
		}

		return true;
	}

	/** Non-overlapping occurrence count of Needle in Haystack (the `SiegeRecallTest` helper, same shape). */
	static int32 CountOccurrences(const FString& Haystack, const TCHAR* Needle)
	{
		const int32 NeedleLength = FCString::Strlen(Needle);
		if (NeedleLength <= 0)
		{
			return 0;
		}

		int32 Count = 0;
		int32 From = 0;
		for (;;)
		{
			const int32 Found = Haystack.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
			if (Found == INDEX_NONE)
			{
				break;
			}
			++Count;
			From = Found + NeedleLength;
		}
		return Count;
	}

	/**
	 *  ⭐⭐ TASK-787: "IS THIS TOKEN IN THE **CODE**?" — comment-only lines skipped, and it is what
	 *  lets a source-scan probe assert an ABSENCE in a codebase whose comments deliberately NAME the
	 *  shapes they refuse.
	 *
	 *  ⚠️⚠️ THE CONCRETE CASE THAT BOUGHT IT: TASK-787 removed the poll's `NotAnAdmittedClimber`
	 *  branch and left a comment EXPLAINING the removal — which a naive `Contains` would read as the
	 *  branch still being there. ⇒ the test would have forced the source to choose between explaining
	 *  itself and passing, and the explanation would have lost. ⚖️ The prose is the guard; the probe
	 *  has to be the one that gets smarter.
	 *
	 *  ⚠️ DECLARED LIMITATION: a comment TRAILING a line of code is still scanned. Every probe that
	 *  uses this asks about a statement, so none is exposed to that — and each carries a control.
	 */
	static int32 CountOccurrencesInCode(const FString& Source, const TCHAR* Needle)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		int32 Count = 0;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();

			// ⚠️ `*` is qualified rather than bare: a doc-comment continuation is `* text` or `*/`,
			// while `*GetNameSafe(Foo)` starts a CODE line the same way — and this file's UE_LOG
			// argument lists are full of those.
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

			Count += CountOccurrences(Line, Needle);
		}
		return Count;
	}

	/**
	 *  The shared shape of the ten exit tests: the named function's body must reach the ONE
	 *  teardown. ⭐ The MOVEMENT-MODE RESTORE itself is asserted ONCE, in test 13 — because all ten
	 *  exits route through `EndLadderClimb`, so restoring the mode ten times would be ten copies of
	 *  one claim rather than ten claims.
	 */
	static void AssertExitReachesTheTeardown(FAutomationTestBase& Test, const TCHAR* ExitLabel,
		const TCHAR* Signature, const TCHAR* RequiredNeedle)
	{
		FString Source;
		if (!LoadHeroCpp(Test, Source))
		{
			return;
		}

		FString Body;
		if (!ExtractFunctionBody(Test, Source, Signature, Body))
		{
			return;
		}

		Test.TestTrue(FString::Printf(
			TEXT("%s — '%s' must contain '%s'. ⛔ MOVE_Flying ignores gravity: an exit that does not reach the teardown hangs the PLAYER'S OWN BODY in mid-air forever."),
			ExitLabel, Signature, RequiredNeedle),
			Body.Contains(RequiredNeedle, ESearchCase::CaseSensitive));
	}

	/** Arms a climb state on the pinned line at the given capsule half-height and rate. Pure — no world, no actor. */
	static FSiegeLadderClimbState ArmedClimb(float CapsuleHalfHeightUU, float ClimbSpeedUU = ShippedClimbRateFromTheLaw)
	{
		FSiegeLadderClimbState State;
		FSiegeLadderClimbStatics::Begin(State, /*bDead=*/ false, /*bAIFrozen=*/ false, /*bSpellFrozen=*/ false,
			ArbitraryFoot, ArbitraryFoot + PinnedClimbDelta, ClimbSpeedUU, CapsuleHalfHeightUU);
		return State;
	}

	// ── ⛔⛔ TASK-787: THE WIDENED `ILadderClimber` SURFACE, PINNED AT **COMPILE TIME** ────────────
	//
	// ⚠️⚠️ THESE FOUR ARE THE WHOLE REASON A HERO CAN CLIMB AT ALL, AND A RUNTIME ROW CANNOT SEE
	// THEM: `ILadderClimber`'s methods are deliberately plain C++ pure virtuals with ⛔ no
	// reflection, so `FindFunctionByName` will ⛔ never find them on the interface. A
	// member-function-pointer identity check is the strongest instrument available for that
	// contract — it fails at COMPILE TIME, in this module, with a message naming the law.
	//
	// ⭐ EACH ONE CAN FAIL, WHICH IS THE BAR (`SHIP-§9c`): delete `GetOnLadderClimbEnded` and the
	// second line stops compiling; narrow `BeginLadderClimb` back to `ASummonedUnit`-only by
	// removing it from the interface and the first does. ⛔ Neither could pass vacuously.
	static_assert(std::is_same_v<decltype(&ILadderClimber::BeginLadderClimb),
		bool (ILadderClimber::*)(const FVector&, const FVector&)>,
		"CONTACT-§12.2: ILadderClimber::BeginLadderClimb is the START seam and is PINNED at "
		"`bool BeginLadderClimb(const FVector&, const FVector&)` — the signature BOTH implementers "
		"already shipped. Without it AClimbableTower cannot start a climber it cannot cast to a "
		"concrete class, and the hero's complete, tested climb cannot fire at all.");

	static_assert(std::is_same_v<decltype(&ILadderClimber::GetOnLadderClimbEnded),
		FSiegeLadderClimbEnded& (ILadderClimber::*)()>,
		"CONTACT-§12.3: ILadderClimber::GetOnLadderClimbEnded is the COMPLETION seam — the half "
		"that keeps the ladder from BRICKING. A tower that can START a climber it cannot hear from "
		"claims the occupancy slot and NEVER releases it, so the FIRST hero attempt would disable "
		"that tower for everyone, for the rest of the match. A START-ONLY widening is WORSE than "
		"none: if you are here to delete this, you are re-introducing that regression.");

	static_assert(std::is_same_v<decltype(&ILadderClimber::AbortLadderClimb), void (ILadderClimber::*)()>,
		"CONTACT-§4.4 / TOWER-§8.4(B): AbortLadderClimb takes NO reason — the teardown is "
		"REASON-AGNOSTIC, and it is how exit H-10 reaches this hero when the tower dies under it.");

	static_assert(std::is_same_v<decltype(&ILadderClimber::IsClimbing), bool (ILadderClimber::*)() const>,
		"CONTACT-§12.5: IsClimbing is read through the interface by the tower's occupancy BELT, "
		"and it is const because that term only ever READS.");

	// ⭐ AND THE HERO'S OWN ACCESSOR, PINNED FROM THE IMPLEMENTER'S SIDE — a hero that returned a
	// COPY (`FSiegeLadderClimbEnded` by value) would compile at the call site, bind to a temporary
	// and silently never deliver a completion signal to the tower. ⛔ That is the exact brick this
	// task exists to prevent, and a reference-vs-value slip is how it would come back.
	static_assert(std::is_same_v<decltype(&AHeroCharacter::GetOnLadderClimbEnded),
		FSiegeLadderClimbEnded& (AHeroCharacter::*)()>,
		"CONTACT-§12.3: AHeroCharacter::GetOnLadderClimbEnded must return FSiegeLadderClimbEnded& "
		"— a REFERENCE to the hero's own instance. A by-value return would bind the tower to a "
		"temporary and the occupancy slot would never be released.");

	/** Reads a float UPROPERTY off a class's CDO by reflection. Returns false when the property is gone (which is an ERROR, ⛔ not a skip, at every call site below). */
	static bool ReadClassDefaultFloat(const UClass* Class, const TCHAR* PropertyName, float& OutValue)
	{
		const FFloatProperty* const Property = FindFProperty<FFloatProperty>(Class, PropertyName);
		const UObject* const Defaults = Class ? Class->GetDefaultObject() : nullptr;
		if (!Property || !Defaults)
		{
			return false;
		}

		OutValue = Property->GetPropertyValue_InContainer(Defaults);
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. THE CLIMBER SEAM, THE THREE API MEMBERS AND — SINCE TASK-787 — THE COMPLETION
//     DELEGATE THAT RELEASES THE TOWER'S OCCUPANCY SLOT, ALL EXIST ON THE HERO
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbApiTest,
	"Siegebound.HeroLadderClimb.TheHeroImplementsTheClimberSeamAndTheClimbApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbApiTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	UClass* const HeroClass = AHeroCharacter::StaticClass();

	// ── (a) `ILadderClimber` — ⛔ THIS IS EXIT H-10 ITSELF ────────────────────────────────
	// `AClimbableTower::EndPlay` reaches a climber ONLY through this interface (its slot is typed
	// `ACharacter`, which carries no `AbortLadderClimb`). ⇒ without this line the tower falls and
	// the hero hangs in `MOVE_Flying` in a match that keeps running around it.
	TestTrue(TEXT("(a) ⭐ AHeroCharacter implements ILadderClimber — the ONLY route AClimbableTower::EndPlay has to abort a hero climber (exit H-10)"),
		HeroClass->ImplementsInterface(ULadderClimber::StaticClass()));

	// ── (b) The three reflected members ──────────────────────────────────────────────────
	const UFunction* const BeginFunction = HeroClass->FindFunctionByName(TEXT("BeginLadderClimb"));
	const UFunction* const AbortFunction = HeroClass->FindFunctionByName(TEXT("AbortLadderClimb"));
	const UFunction* const IsClimbingFunction = HeroClass->FindFunctionByName(TEXT("IsClimbing"));

	TestNotNull(TEXT("(b) BeginLadderClimb is declared on the hero (the TOWER-§8.4(B) shape, re-declared per CONTACT-§2's per-class-driver ruling)"), BeginFunction);
	TestNotNull(TEXT("(b) AbortLadderClimb is declared on the hero"), AbortFunction);
	TestNotNull(TEXT("(b) IsClimbing is declared on the hero — it is ALSO the K-4 disarm term"), IsClimbingFunction);

	// ── (c) POSITIVE CONTROL: the probe can actually report an absence ────────────────────
	// ⭐ Without this, (b) would pass just as happily against a reflection walk that silently
	// resolved everything — the failure mode `SHIP-§9c` names.
	TestNull(TEXT("(c) ⭐ POSITIVE CONTROL: a name that is NOT on the hero resolves to null, so (b)'s three hits mean something"),
		HeroClass->FindFunctionByName(TEXT("BeginLadderClimbThatDoesNotExist")));

	// ── (d) `BeginLadderClimb` takes the two pinned world points and returns a bool ───────
	// ⛔ NO Z-ORDERING IS IMPLIED: the link is BothWays, so a DESCENT passes the same two points
	// the other way round. The parameter COUNT and TYPES are the contract; the names are not.
	if (BeginFunction)
	{
		int32 VectorParams = 0;
		bool bHasBoolReturn = false;
		// ⭐ The shipped parameter-walk idiom, character-for-character (`SiegeLadderClimbTest.cpp`,
		// `SiegeClimbableTowerTest.cpp:1183`) — ⛔ not a new one.
		for (TFieldIterator<FProperty> It(BeginFunction); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				bHasBoolReturn = It->IsA<FBoolProperty>();
				continue;
			}

			if (const FStructProperty* const StructParam = CastField<FStructProperty>(*It))
			{
				if (StructParam->Struct == TBaseStructure<FVector>::Get())
				{
					++VectorParams;
				}
			}
		}

		TestEqual(TEXT("(d) BeginLadderClimb takes EXACTLY two FVector world points (From -> To)"), VectorParams, 2);
		TestTrue(TEXT("(d) BeginLadderClimb returns a bool — 'returns false and changes NOTHING' is half the pinned contract"), bHasBoolReturn);
	}

	// ── (e) ⭐⭐ THE COMPLETION DELEGATE — **ONE TYPE FOR BOTH PAWNS** (TASK-787, CONTACT-§12.3) ──
	// ⚠️⚠️ THIS IS THE ROW THAT PROVES THE LADDER CANNOT BE BRICKED BY A HERO. The tower binds and
	// unbinds through `ILadderClimber::GetOnLadderClimbEnded()`, which returns the implementer's own
	// `FSiegeLadderClimbEnded`. If the hero grew a SECOND, hero-only delegate type (the shape
	// REFUSED at CONTACT-§12.4), the tower's `AddUniqueDynamic` would be binding a handler of a
	// different signature — and the compile-time pins in the fixture above cover the accessor's
	// return TYPE, but ⛔ not the fact that both pawns broadcast THE SAME delegate.
	const FMulticastDelegateProperty* const HeroClimbEnded =
		CastField<FMulticastDelegateProperty>(HeroClass->FindPropertyByName(FName(TEXT("OnLadderClimbEnded"))));
	const FMulticastDelegateProperty* const UnitClimbEnded =
		CastField<FMulticastDelegateProperty>(ASummonedUnit::StaticClass()->FindPropertyByName(FName(TEXT("OnLadderClimbEnded"))));

	TestNotNull(TEXT("(e) ⭐ AHeroCharacter declares OnLadderClimbEnded as a reflected multicast delegate — the tower's ONLY way to learn a HERO climb ended, and without it the occupancy slot is claimed and NEVER released"),
		HeroClimbEnded);
	TestNotNull(TEXT("(e) SELF-CHECK: ASummonedUnit still declares its own — this row compares two live properties, ⛔ not one live and one missing"),
		UnitClimbEnded);

	if (HeroClimbEnded && UnitClimbEnded)
	{
		// ⭐⭐ IDENTITY, ⛔ NOT SHAPE-EQUIVALENCE: a dynamic delegate declared ONCE has ONE
		// UDelegateFunction, so pointer identity here means literally "the same type" —
		// `FSiegeLadderClimbEnded`, declared in `LadderClimber.h` since TASK-787 moved it out of
		// `SummonedUnit.h`. ⛔ A hero-only clone with an identical signature would FAIL this row,
		// which is exactly what CONTACT-§12.4 asks for and what a parameter-by-parameter
		// comparison would have missed.
		TestTrue(TEXT("(e) ⛔⛔ the hero's and the unit's OnLadderClimbEnded are the SAME delegate type (FSiegeLadderClimbEnded) — ⛔ NOT two lookalikes. One type serves both pawns; a second, hero-only delegate is REFUSED (CONTACT-§12.4)"),
			HeroClimbEnded->SignatureFunction == UnitClimbEnded->SignatureFunction);

		// ⚠️ `FMulticastDelegateProperty::SignatureFunction` is a `TObjectPtr<UFunction>`, and both
		// `TestNotNull` overloads DEDUCE `const ValueType*`. ⛔ Template argument deduction does NOT
		// run TObjectPtr's implicit conversion-to-raw, so the value must already BE a raw pointer at
		// the call site (the `==` row above and the `if`/`->NumParms` rows below are unaffected —
		// none of them deduces). ⭐ Same idiom as `SiegeLadderClimbTest.cpp:1061`, which asserts
		// non-null on this very field. ⛔ The row itself is UNCHANGED: it still fails on a null.
		const UFunction* const HeroSignature = HeroClimbEnded->SignatureFunction;
		TestNotNull(TEXT("(e) SELF-CHECK: the delegate carries a signature function at all — an identity compare between two nulls would pass while proving nothing"),
			HeroSignature);

		if (HeroClimbEnded->SignatureFunction)
		{
			TestEqual(TEXT("(e) …and it still takes EXACTLY two parameters (ACharacter* Climber, bool bReachedTop) — TOWER-§8.4(B)'s pinned signature, which TASK-787's MOVE left untouched"),
				static_cast<int32>(HeroClimbEnded->SignatureFunction->NumParms), 2);
		}

		// ⭐ AND IT IS BlueprintAssignable, mirroring the unit's specifier exactly.
		TestTrue(TEXT("(e) the hero's delegate is BlueprintAssignable, exactly as the unit's is — one shape for both climbers"),
			HeroClimbEnded->HasAnyPropertyFlags(CPF_BlueprintAssignable));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. THE EXIT LIST IS CLOSED — TEN ENUMERATORS, ENUMERATED FRESH
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitListClosedTest,
	"Siegebound.HeroLadderClimb.TheExitReasonListIsClosedAndEnumeratedFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitListClosedTest::RunTest(const FString& Parameters)
{
	const UEnum* const ExitEnum = StaticEnum<ESiegeHeroLadderExit>();
	if (!TestNotNull(TEXT("⛔ ESiegeHeroLadderExit did not resolve — the closed exit list IS the feature's contract"), ExitEnum))
	{
		return false;
	}

	// ⭐ `NumEnums()` counts the hidden `_MAX` sentinel UHT appends.
	TestEqual(TEXT("⭐ EXACTLY TEN exit reasons. CONTACT-§3.1 names ten exits; H-2 and H-10 share `Abort` (ILadderClimber takes ⛔ no reason — TOWER-§8.4(B)) and the watchdog contributes its own. An eleventh enumerator means somebody added an exit without adding a test"),
		ExitEnum->NumEnums() - 1, 10);

	// ⛔ Every enumerator is named, so a rename is a RED test rather than a silent semantic drift.
	const TCHAR* const RequiredNames[] = {
		TEXT("Arrival"), TEXT("Abort"), TEXT("InputReleased"), TEXT("Death"), TEXT("Unpossessed"),
		TEXT("Respawn"), TEXT("RecallArrival"), TEXT("MatchEnd"), TEXT("EndPlay"), TEXT("Watchdog")
	};

	for (const TCHAR* const Name : RequiredNames)
	{
		TestTrue(FString::Printf(TEXT("⭐ ESiegeHeroLadderExit::%s exists"), Name),
			ExitEnum->GetIndexByNameString(FString(Name)) != INDEX_NONE);
	}

	// POSITIVE CONTROL: the lookup can report an absence.
	TestEqual(TEXT("⭐ POSITIVE CONTROL: a reason that does not exist is not found, so the ten hits above mean something"),
		ExitEnum->GetIndexByNameString(TEXT("NotAnExitReason")), INDEX_NONE);

	// ⛔ AND IT IS ⛔ NOT `ESiegeLadderExit`: the unit's enum is DRIVER VOCABULARY (`NewOrder`,
	// `MatchEndFreeze`, `SpellFreeze`) and `CONTACT-§3.1` requires the hero's ten to be enumerated
	// FRESH rather than mapped across. A hero enum that acquired `NewOrder` would be a mapping.
	TestEqual(TEXT("⛔ The hero's list did NOT inherit the unit's `NewOrder` (a hero has no orders) — CONTACT-§3.1's 'enumerate fresh, never map across'"),
		ExitEnum->GetIndexByNameString(TEXT("NewOrder")), INDEX_NONE);
	TestEqual(TEXT("⛔ The hero's list did NOT inherit the unit's `SpellFreeze` — measured: no spell in Siegebound can freeze the hero (SpellLibrary.cpp:304-320)"),
		ExitEnum->GetIndexByNameString(TEXT("SpellFreeze")), INDEX_NONE);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3-12. THE TEN EXITS — ONE TEST EACH
//  ⭐ The whole feature is these ten assertions (CONTACT-§3.1). The movement-mode
//     RESTORE is asserted once, in test 13, because all ten route through ONE teardown.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH1Test,
	"Siegebound.HeroLadderClimb.ExitH1_ArrivalEndsTheClimbAndOnlyArrivalMaySnap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH1Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::TickLadderClimb(float DeltaSeconds)"), Body))
	{
		return false;
	}

	TestTrue(TEXT("H-1 — the driver ends the climb with ESiegeHeroLadderExit::Arrival"),
		Body.Contains(TEXT("ESiegeHeroLadderExit::Arrival"), ESearchCase::CaseSensitive));

	// ⭐⭐ `TOWER-§8.5a` CLAUSE 5, AND IT IS THE CLAIM WORTH ASSERTING: the arrival SNAP is inside
	// the `bReachedTop` branch, so a timed-out climb is ⛔ NEVER handed the deck it failed to reach.
	// ⚖️ A watchdog that teleports its casualty to the destination is not a watchdog.
	const int32 SnapIndex = Body.Find(TEXT("ArrivalTarget("), ESearchCase::CaseSensitive);
	const int32 GuardIndex = Body.Find(TEXT("if (bReachedTop)"), ESearchCase::CaseSensitive);
	TestTrue(TEXT("H-1 — the driver snaps to FSiegeLadderClimbStatics::ArrivalTarget so the hero lands ON the deck rather than wherever the frame's step stopped"),
		SnapIndex != INDEX_NONE);
	TestTrue(TEXT("H-1 — ⛔ the snap is GUARDED by bReachedTop (TOWER-§8.5a clause 5: a timed-out or aborted climb is NEVER handed the deck)"),
		GuardIndex != INDEX_NONE && SnapIndex > GuardIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH2Test,
	"Siegebound.HeroLadderClimb.ExitH2_AbortLadderClimbReachesTheTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH2Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	AssertExitReachesTheTeardown(*this, TEXT("H-2 (explicit abort)"),
		TEXT("void AHeroCharacter::AbortLadderClimb()"), TEXT("EndLadderClimb("));
	AssertExitReachesTheTeardown(*this, TEXT("H-2 (explicit abort) reports the Abort reason"),
		TEXT("void AHeroCharacter::AbortLadderClimb()"), TEXT("ESiegeHeroLadderExit::Abort"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH3Test,
	"Siegebound.HeroLadderClimb.ExitH3_ReleasingTheInputEndsTheClimb",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH3Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::TickLadderClimb(float DeltaSeconds)"), Body))
	{
		return false;
	}

	// ⭐ The hero's MOST FREQUENT exit and the one a unit does ⛔ not have (`K-B` hold-to-climb).
	TestTrue(TEXT("H-3 — the driver asks IsLadderClimbInputHeld() every frame"),
		Body.Contains(TEXT("IsLadderClimbInputHeld()"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("H-3 — releasing ends the climb with ESiegeHeroLadderExit::InputReleased"),
		Body.Contains(TEXT("ESiegeHeroLadderExit::InputReleased"), ESearchCase::CaseSensitive));

	// ⛔ AND THE STEER IS SUPPRESSED, NOT MERELY OBSERVED (the NAV-§3 no-double-driver law): in
	// MOVE_Flying the template's DoMove is unconstrained 3D flight and would pull the capsule off
	// the pinned line, where there is neither a deck-breach window nor an arrival.
	FString MoveBody;
	if (ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::DoMove(float Right, float Forward)"), MoveBody))
	{
		const int32 CaptureIndex = MoveBody.Find(TEXT("LadderClimbSteerFrame = GFrameCounter;"), ESearchCase::CaseSensitive);
		const int32 SuperIndex = MoveBody.Find(TEXT("Super::DoMove("), ESearchCase::CaseSensitive);
		const int32 EarlyReturnIndex = MoveBody.Find(TEXT("return;"), ESearchCase::CaseSensitive);

		TestTrue(TEXT("H-3 — DoMove CAPTURES the steer while a climb runs (it is the only place the player's intent exists before the movement component eats it)"),
			CaptureIndex != INDEX_NONE);
		TestTrue(TEXT("H-3 — ⛔ and RETURNS before Super::DoMove while climbing: ONE steering authority (NAV-§3), or the player's steer fights the climb driver"),
			EarlyReturnIndex != INDEX_NONE && SuperIndex != INDEX_NONE && EarlyReturnIndex < SuperIndex);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH4Test,
	"Siegebound.HeroLadderClimb.ExitH4_DeathAbortsTheClimbBeforeMovementIsDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH4Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::HandleDeath()"), Body))
	{
		return false;
	}

	const int32 AbortIndex = Body.Find(TEXT("AbortLadderClimb();"), ESearchCase::CaseSensitive);
	const int32 DisableIndex = Body.Find(TEXT("DisableMovement();"), ESearchCase::CaseSensitive);

	// ⚠️⚠️ THE TRAP THIS TEST EXISTS FOR: `DisableMovement()` was ALREADY here before TASK-778, so a
	// reviewer can read this function and conclude the hero is covered. It meets the MOVEMENT-MODE
	// half and ⛔ nothing else — the climb state, the driver and the watchdog would all survive the
	// hero's death. ⇒ the abort is a SEPARATE, REQUIRED line.
	TestTrue(TEXT("H-4 — HandleDeath aborts the climb (⛔ the pre-existing DisableMovement() covers the MODE and ⛔ nothing about the state, the driver or the watchdog)"),
		AbortIndex != INDEX_NONE);
	TestTrue(TEXT("H-4 — ⭐ the abort runs BEFORE DisableMovement(), so a corpse's final state is MOVE_None rather than a mode the abort restored afterwards"),
		AbortIndex != INDEX_NONE && DisableIndex != INDEX_NONE && AbortIndex < DisableIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH5Test,
	"Siegebound.HeroLadderClimb.ExitH5_UnPossessedIsAnIndependentBelt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH5Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::UnPossessed()"), Body))
	{
		return false;
	}

	const int32 AbortIndex = Body.Find(TEXT("AbortLadderClimb();"), ESearchCase::CaseSensitive);
	const int32 SuperIndex = Body.Find(TEXT("Super::UnPossessed();"), ESearchCase::CaseSensitive);

	// ⛔⛔ THE NASTIEST EXIT: a hero that dies mid-climb is un-possessed INTO THE GHOST while still
	// in MOVE_Flying. It is a SEPARATE SEAM from H-4 — a possession change for ANY reason (debug
	// possess, seamless travel, a game-mode restart) must end a climb, and enumerating the reasons
	// is how you miss one.
	TestTrue(TEXT("H-5 — UnPossessed aborts the climb (the independent belt; ⛔ NOT a duplicate of H-4)"),
		AbortIndex != INDEX_NONE);
	TestTrue(TEXT("H-5 — ⭐ before Super::UnPossessed(), while the controller and the movement component are still coherent"),
		AbortIndex != INDEX_NONE && SuperIndex != INDEX_NONE && AbortIndex < SuperIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH6Test,
	"Siegebound.HeroLadderClimb.ExitH6_RespawnAbortsBeforeItWritesWalking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH6Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::ResetHero()"), Body))
	{
		return false;
	}

	const int32 AbortIndex = Body.Find(TEXT("AbortLadderClimb();"), ESearchCase::CaseSensitive);
	const int32 WalkingIndex = Body.Find(TEXT("SetMovementMode(MOVE_Walking)"), ESearchCase::CaseSensitive);

	TestTrue(TEXT("H-6 — ResetHero aborts the climb. ⚠️ It is the BACKSTOP, ⛔ not the exit: it is ~180 s downstream of the death (GHOST-§0), so a hero rescued only here was broken for three minutes"),
		AbortIndex != INDEX_NONE);
	TestTrue(TEXT("H-6 — ⭐ the abort runs BEFORE the respawn's own SetMovementMode(MOVE_Walking), so the respawn's deliberate mode wins and ⛔ never the other way round"),
		AbortIndex != INDEX_NONE && WalkingIndex != INDEX_NONE && AbortIndex < WalkingIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH7Test,
	"Siegebound.HeroLadderClimb.ExitH7_TheRecallTeleportEndsTheClimbFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH7Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::EndRecall(ESiegeRecallExit Exit)"), Body))
	{
		return false;
	}

	const int32 AbortIndex = Body.Find(TEXT("AbortLadderClimb();"), ESearchCase::CaseSensitive);
	const int32 TeleportIndex = Body.Find(TEXT("OnHeroRecallArrived.Broadcast(this);"), ESearchCase::CaseSensitive);

	// ⭐ NEAR-UNREACHABLE AND ⛔ KEPT ANYWAY (`CONTACT-§3.4`): the recall's own cancel reads
	// POSITION, so a scripted climb cancels a running channel within ~0.07 s at 350 uu/s. Reaching
	// this line needs a completion and a climb-start on the same frame. ⚖️ An exit you cannot reach
	// is free; an exit you removed is a hang.
	TestTrue(TEXT("H-7 — the recall's granting exit aborts a live climb"), AbortIndex != INDEX_NONE);
	TestTrue(TEXT("H-7 — ⭐ BEFORE the teleport broadcast: the destination owner MOVES this actor inside that call, and a live MOVE_Flying drive would then interpolate the hero back out of its own keep toward a ladder 500 m away"),
		AbortIndex != INDEX_NONE && TeleportIndex != INDEX_NONE && AbortIndex < TeleportIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH8Test,
	"Siegebound.HeroLadderClimb.ExitH8_MatchEndIsTheHerosFrozenTermAndItIsMapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH8Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString TickBody;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::TickLadderClimb(float DeltaSeconds)"), TickBody))
	{
		return false;
	}

	TestTrue(TEXT("H-8 — the driver ends the climb on match end, and it is checked FIRST (nothing should still be driving a body up a tower under the end screen)"),
		TickBody.Contains(TEXT("ESiegeHeroLadderExit::MatchEnd"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("H-8 — it reads IsMatchOver(), the SAME shipped rule the recall channel's own match-end exit reads (⛔ no second match-end rule is invented)"),
		TickBody.Contains(TEXT("IsMatchOver()"), ESearchCase::CaseSensitive));

	// ⛔⛔ `CONTACT-§3.2` — THE FROZEN TERMS ARE ***MAPPED***, ⛔ NEVER PASSED THROUGH. Typing
	// `false` for `bAIFrozen` would assert that no match-end freeze can exist for this pawn, and a
	// pawn entering MOVE_Flying during the end screen is `GHOST-§4`'s input-dead catastrophe with a
	// new cause. ⇒ the ADMISSION predicate must carry the mapping too, ⛔ not just the driver.
	FString BeginBody;
	if (ExtractFunctionBody(*this, Source, TEXT("bool AHeroCharacter::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)"), BeginBody))
	{
		TestTrue(TEXT("H-8 — ⭐ CanBegin's bAIFrozen argument is MAPPED to IsMatchOver() at the admission site (CONTACT-§3.2), ⛔ not typed as a bare false"),
			BeginBody.Contains(TEXT("/*bAIFrozen=*/ IsMatchOver()"), ESearchCase::CaseSensitive));

		// ⚠️ And the recall term, which `CONTACT-§3.4` bullet 1 owes and which nothing else provides:
		// a hero may ⛔ NOT START a climb while a recall channel is running.
		TestTrue(TEXT("H-8/§3.4 — BeginLadderClimb refuses while a recall channel runs (CONTACT-§3.4 bullet 1 — ⛔ provided by nothing else)"),
			BeginBody.Contains(TEXT("IsRecalling()"), ESearchCase::CaseSensitive));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH9Test,
	"Siegebound.HeroLadderClimb.ExitH9_EndPlayAbortsBeforeSuper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH9Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)"), Body))
	{
		return false;
	}

	const int32 AbortIndex = Body.Find(TEXT("AbortLadderClimb();"), ESearchCase::CaseSensitive);
	const int32 SuperIndex = Body.Find(TEXT("Super::EndPlay("), ESearchCase::CaseSensitive);

	TestTrue(TEXT("H-9 — EndPlay aborts a live climb"), AbortIndex != INDEX_NONE);
	TestTrue(TEXT("H-9 — ⭐ before Super::EndPlay, while the movement component and the world timer manager are both still valid (a timer left armed on a torn-down actor is the dangling-handle class this class clears everywhere else)"),
		AbortIndex != INDEX_NONE && SuperIndex != INDEX_NONE && AbortIndex < SuperIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbExitH10Test,
	"Siegebound.HeroLadderClimb.ExitH10_TheTowerDyingReachesTheHeroThroughTheInterface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbExitH10Test::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	// ⭐ THIS EXIT SPANS TWO FILES, SO IT IS ASSERTED ACROSS BOTH — the hero implements the seam,
	// and the tower actually uses it. Either half alone is a hero hanging in the air when the tower
	// falls, which is why neither half is assumed.
	TestTrue(TEXT("H-10 (hero half) — AHeroCharacter implements ILadderClimber, so the tower's ACharacter-typed slot can reach AbortLadderClimb()"),
		AHeroCharacter::StaticClass()->ImplementsInterface(ULadderClimber::StaticClass()));

	FString TowerSource;
	if (!LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp"), TowerSource))
	{
		return false;
	}

	FString TowerEndPlayBody;
	if (!ExtractFunctionBody(*this, TowerSource, TEXT("void AClimbableTower::EndPlay(const EEndPlayReason::Type EndPlayReason)"), TowerEndPlayBody))
	{
		return false;
	}

	TestTrue(TEXT("H-10 (tower half) — AClimbableTower::EndPlay reaches its climber through ILadderClimber"),
		TowerEndPlayBody.Contains(TEXT("ILadderClimber"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("H-10 (tower half) — and calls AbortLadderClimb() on it: a MOVE_Flying climber does ⛔ NOT fall when the floor disappears"),
		TowerEndPlayBody.Contains(TEXT("AbortLadderClimb()"), ESearchCase::CaseSensitive));
	// ⚠️ THE NEEDLE CARRIES ITS OWN `(` ON PURPOSE, AND IT IS ⛔ NOT PEDANTRY: that function's
	// comment NAMES the refused `Cast<AHeroCharacter>` shape in prose, so a bare-identifier search
	// would fail against CORRECT code and would have to be deleted rather than fixed. ⭐ A cast
	// EXPRESSION always carries its parenthesis; a sentence about one does not.
	TestFalse(TEXT("H-10 — ⛔ and it is ⛔ NOT reached through a Cast<AHeroCharacter>(…) branch pair (CONTACT-§4.4 refused exactly that: it hardcodes the class list and is the failed-cast shape the widening removes)"),
		TowerEndPlayBody.Contains(TEXT("Cast<AHeroCharacter>("), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  13. ONE TEARDOWN, ONE RESTORE — the claim the ten exits above all lean on
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbOneTeardownTest,
	"Siegebound.HeroLadderClimb.TheDriverConsumesTheShippedStaticsAndHasExactlyOneTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbOneTeardownTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	// ── (a) THE EXACTLY-ONCE LATCH IS CONSUMED IN EXACTLY ONE PLACE ───────────────────────
	// ⭐ This is what makes "every exit restores the mode exactly once" a property of ONE branch
	// rather than a promise repeated at ten call sites — and it is why the ten tests above assert
	// only that each exit REACHES the teardown.
	TestEqual(TEXT("(a) ⭐ FSiegeLadderClimbStatics::End is called from EXACTLY ONE place — the one teardown. Two call sites is two half-teardowns and a double exit that is no longer inert"),
		CountOccurrences(Source, TEXT("FSiegeLadderClimbStatics::End(")), 1);

	// ── (b) EXACTLY ONE ENTRY INTO MOVE_Flying AND EXACTLY ONE RESTORE ────────────────────
	TestEqual(TEXT("(b) ⭐ MOVE_Flying is entered from EXACTLY ONE place (BeginLadderClimb)"),
		CountOccurrences(Source, TEXT("SetMovementMode(MOVE_Flying)")), 1);
	TestEqual(TEXT("(b) ⛔ AND THE LINE THE WHOLE FEATURE TURNS ON: SetDefaultMovementMode() appears EXACTLY ONCE — in the teardown. MOVE_Flying ignores gravity, so a second, divergent restore is a second thing that can be wrong"),
		CountOccurrences(Source, TEXT("SetDefaultMovementMode()")), 1);

	// ── (c) THE RESTORE IS INSIDE THE TEARDOWN, AFTER THE LATCH ───────────────────────────
	FString TeardownBody;
	if (ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::EndLadderClimb(bool bReachedTop, ESiegeHeroLadderExit Reason)"), TeardownBody))
	{
		const int32 LatchIndex = TeardownBody.Find(TEXT("FSiegeLadderClimbStatics::End("), ESearchCase::CaseSensitive);
		const int32 RestoreIndex = TeardownBody.Find(TEXT("SetDefaultMovementMode()"), ESearchCase::CaseSensitive);
		const int32 WatchdogClearIndex = TeardownBody.Find(TEXT("ClearTimer(LadderClimbWatchdogTimerHandle)"), ESearchCase::CaseSensitive);
		const int32 FlySpeedRestoreIndex = TeardownBody.Find(TEXT("MaxFlySpeed = LadderClimbSavedMaxFlySpeed"), ESearchCase::CaseSensitive);

		TestTrue(TEXT("(c) the teardown consumes the latch FIRST, before any effect — so a double exit (death then EndPlay is the ORDINARY case) is inert by construction"),
			LatchIndex != INDEX_NONE && RestoreIndex != INDEX_NONE && LatchIndex < RestoreIndex);
		TestTrue(TEXT("(c) the teardown clears the watchdog — a looping timer that outlived its climb would eventually abort a climb it did not start"),
			WatchdogClearIndex != INDEX_NONE);
		TestTrue(TEXT("(c) the teardown restores MaxFlySpeed EXACTLY, from the value saved at Begin (zero residual — nothing else in this project writes that field on the hero)"),
			FlySpeedRestoreIndex != INDEX_NONE);
	}

	// ── (d) THE RULES ARE CONSUMED, ⛔ NOT RE-IMPLEMENTED (`CONTACT-§2`) ──────────────────
	const TCHAR* const RequiredStatics[] = {
		TEXT("FSiegeLadderClimbStatics::Begin("), TEXT("FSiegeLadderClimbStatics::Advance("),
		TEXT("FSiegeLadderClimbStatics::ShouldSweep("), TEXT("FSiegeLadderClimbStatics::ArrivalTarget("),
		TEXT("FSiegeLadderClimbStatics::ClimbDirection(")
	};
	for (const TCHAR* const Needle : RequiredStatics)
	{
		TestTrue(FString::Printf(TEXT("(d) the hero's driver CONSUMES '%s' rather than re-implementing the rule (CONTACT-§2: a statics lift plus a per-class DRIVER — ⛔ no hero variant of any rule)"), Needle),
			Source.Contains(Needle, ESearchCase::CaseSensitive));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  14. THE CAPSULE LIFT IS READ FROM THE CAPSULE — AND IT TRACKS A RESIZE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbCapsuleLiftTest,
	"Siegebound.HeroLadderClimb.TheEndpointLiftIsReadFromTheHerosOwnCapsule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbCapsuleLiftTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	// ── (a) THE HERO'S ACTUAL CAPSULE, READ OFF THE CDO ──────────────────────────────────
	// ⭐ This is the number the whole `K-1` ruling turned on, and it is read here rather than
	// trusted: `AGitClaudeUnrealTestCharacter`'s constructor calls `InitCapsuleSize(42.f, 96.0f)`,
	// and `BP_HeroCharacter` overrides nothing.
	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	const UCapsuleComponent* const HeroCapsule = HeroDefaults ? HeroDefaults->GetCapsuleComponent() : nullptr;
	if (!TestNotNull(TEXT("⛔ The hero CDO's capsule did not resolve — it IS the subject of this test"), HeroCapsule))
	{
		return false;
	}

	const float ShippedHalfHeight = HeroCapsule->GetScaledCapsuleHalfHeight();
	const float ShippedRadius = HeroCapsule->GetScaledCapsuleRadius();

	TestEqual(TEXT("(a) ⭐ the hero's capsule half-height is EXACTLY 96 — ⛔ NOT the unit's 88 and ⛔ NOT SiegeSpawn::DefaultCapsuleHalfHeight, which its own comment calls a FALLBACK"),
		ShippedHalfHeight, HeroCapsuleHalfHeightFromTheTemplate, Exact);
	TestEqual(TEXT("(a) ⚠️ the hero's capsule radius is EXACTLY 42. ⛔ IF THIS CHANGES, RE-RUN THE STANDOFF GATE: TOWER-§8.5a's voiding condition is TWO-SIDED and fires on a new capsule as readily as on a re-authored mesh (42 ⇒ 51.624 uu residual, 4.376 SHORT of the required 56 — the deficit Jonathan's K-1 option A closes by translating the ladder)"),
		ShippedRadius, HeroCapsuleRadiusFromTheTemplate, Exact);

	// ── (b) THE LIFT IS THE HALF-HEIGHT, AT BOTH ENDS ────────────────────────────────────
	// `TOWER-§8.5a` clause 6: the sockets are SURFACE points but the thing that travels the line is
	// the capsule CENTRE. ⛔ Without the lift the hero "arrives" with its FEET a half-height below
	// the deck, buried in the slab, and depenetration drops it back down the tower.
	const FSiegeLadderClimbState HeroClimb = ArmedClimb(ShippedHalfHeight);
	TestEqual(TEXT("(b) the START is lifted by the hero's OWN half-height"),
		static_cast<float>(HeroClimb.Start.Z - ArbitraryFoot.Z), ShippedHalfHeight, Tolerance);
	TestEqual(TEXT("(b) the END is lifted by the SAME amount, so the length, the direction and the watchdog budget are unchanged by the lift"),
		static_cast<float>(HeroClimb.End.Z - (ArbitraryFoot.Z + PinnedClimbDelta.Z)), ShippedHalfHeight, Tolerance);
	TestEqual(TEXT("(b) the stored half-height is the one that was handed in"),
		HeroClimb.CapsuleHalfHeightUU, ShippedHalfHeight, Exact);

	// ── (c) ⭐⭐ THE ASSERTION THAT MAKES A FUTURE LITERAL GO RED AND NAME ITS REASON ──────
	// A driver that hardcoded 88 (or 96) would pass (b) forever. This row drives a DIFFERENT
	// capsule through the same code and requires the answer to MOVE with it.
	const FSiegeLadderClimbState UnitSizedClimb = ArmedClimb(UnitCapsuleHalfHeight);
	TestTrue(TEXT("(c) ⭐ a DIFFERENT capsule half-height produces a DIFFERENT lift — so the day somebody replaces GetScaledCapsuleHalfHeight() with a literal, this suite goes RED and says why (CONTACT-§3.3: ⛔ never inherit a number from the unit)"),
		!FMath::IsNearlyEqual(static_cast<float>(UnitSizedClimb.Start.Z), static_cast<float>(HeroClimb.Start.Z), Tolerance));
	TestEqual(TEXT("(c) and the unit-sized lift is exactly the unit's 88, which is what the hero's 96 must NOT be"),
		static_cast<float>(UnitSizedClimb.Start.Z - ArbitraryFoot.Z), UnitCapsuleHalfHeight, Tolerance);

	// ── (d) THE DRIVER READS THE CAPSULE, AND THE FALLBACK IS ONLY A FALLBACK ────────────
	FString Source;
	if (LoadHeroCpp(*this, Source))
	{
		FString BeginBody;
		if (ExtractFunctionBody(*this, Source, TEXT("bool AHeroCharacter::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)"), BeginBody))
		{
			TestTrue(TEXT("(d) the half-height comes from GetScaledCapsuleHalfHeight() on THIS hero's capsule (TOWER-§7: geometry comes FROM the thing — a BP child that resizes its capsule stays correct for free)"),
				BeginBody.Contains(TEXT("GetScaledCapsuleHalfHeight()"), ESearchCase::CaseSensitive));
			TestTrue(TEXT("(d) SiegeSpawn::DefaultCapsuleHalfHeight appears ONLY as the null-capsule fallback, never as the hero's dimension"),
				BeginBody.Contains(TEXT("SiegeSpawn::DefaultCapsuleHalfHeight"), ESearchCase::CaseSensitive));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  15. THE DECK-BREACH WINDOW, RE-DERIVED FOR THE HERO'S CAPSULE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbDeckBreachTest,
	"Siegebound.HeroLadderClimb.TheDeckBreachWindowIsReDerivedFromTheHerosHalfHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbDeckBreachTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	// ⭐ THE EXPECTATION IS RE-DERIVED FROM THE LAW'S OWN FORMULA, ⛔ not transcribed from the
	// subject: `TOWER-§8.5a` clause 2 is "≤ 3 × the capsule half-height, measured in Z, converted
	// to line length by the line's OWN slope" (⛔ never a hardcoded sin 76°).
	const float ExpectedBreachZUU = FSiegeLadderClimbStatics::DeckBreachCapsuleHalfHeights * HeroCapsuleHalfHeightFromTheTemplate; // 288
	const float ExpectedBreachLineUU = ExpectedBreachZUU * (PinnedLineLengthUU / PinnedClimbDelta.Z);                              // 296.863

	const FSiegeLadderClimbState HeroClimb = ArmedClimb(HeroCapsuleHalfHeightFromTheTemplate);

	TestEqual(TEXT("⭐ the hero's window is 3 × 96 = 288 uu of Z ⇒ 296.86 uu of LINE, converted by the line's own slope"),
		HeroClimb.DeckBreachUU, ExpectedBreachLineUU, Tolerance);

	// ⭐ The percentage `CONTACT-§3.3` #2 demands be REPORTED as a number: 288/1200 = 24.00% exactly.
	const float HeroWindowFraction = HeroClimb.DeckBreachUU / HeroClimb.LengthUU;
	TestEqual(TEXT("⭐ that is EXACTLY 24.00% of the 1,236.9 uu ascent (the unit's row is 22.00%) — the number CONTACT-§3.3 #2 requires to be reported rather than asserted to be fine"),
		HeroWindowFraction, 0.24f, Tolerance);

	// ⛔ AND IT MUST DIFFER FROM THE UNIT'S. A window inherited as a number would pass every
	// absolute check above and fail this one.
	const FSiegeLadderClimbState UnitSizedClimb = ArmedClimb(UnitCapsuleHalfHeight);
	TestEqual(TEXT("⛔ the unit-sized window is 22.00% — a DIFFERENT number, from the SAME formula"),
		UnitSizedClimb.DeckBreachUU / UnitSizedClimb.LengthUU, 0.22f, Tolerance);
	TestTrue(TEXT("⭐ so the window is DERIVED from the capsule handed in, ⛔ not inherited from the unit (CONTACT-§3.3 #2)"),
		!FMath::IsNearlyEqual(HeroClimb.DeckBreachUU, UnitSizedClimb.DeckBreachUU, Tolerance));

	// ⛔ THE SWEEP SURVIVES ON THE REST OF THE LINE — `TOWER-§8.5a`'s own QA clause: "DO confirm the
	// self-check survives: the foot and the midpoint of the line ARE swept."
	const FVector Midpoint = (HeroClimb.Start + HeroClimb.End) * 0.5;
	TestTrue(TEXT("⛔ the FOOT of the line is still SWEPT — a whole-line non-swept traversal is refused outright (TOWER-§8.5)"),
		FSiegeLadderClimbStatics::ShouldSweep(HeroClimb, HeroClimb.Start));
	TestTrue(TEXT("⛔ the MIDPOINT is still SWEPT — the window is the elevated END and nothing else"),
		FSiegeLadderClimbStatics::ShouldSweep(HeroClimb, Midpoint));
	TestFalse(TEXT("⭐ and the capsule stops sweeping inside the window at the elevated end, which is the only reason the feature reaches the deck at all"),
		FSiegeLadderClimbStatics::ShouldSweep(HeroClimb, HeroClimb.End));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  16. THE WATCHDOG BUDGET COMES FROM THE ONE SHIPPED RATE THIS CLASS DOES NOT OWN
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbBudgetTest,
	"Siegebound.HeroLadderClimb.TheBudgetComesFromTheOneShippedRateTheHeroReadsRatherThanOwns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbBudgetTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	// ── (a) ⛔⛔ THE GUARD ON THE HERO'S RUNTIME REFLECTION READ ───────────────────────────
	// `AHeroCharacter::TryResolveLadderClimbSpeedUU` looks this property up BY NAME on the unit's
	// CDO, because `CONTACT-§8` forbids a second copy on the hero and the shipped property is
	// `protected` on another task's class. ⇒ a rename would make the hero REFUSE every climb at
	// runtime with one warning. This assertion is what turns that into a RED SUITE instead.
	float ShippedRate = 0.f;
	if (!TestTrue(TEXT("(a) ⛔ ASummonedUnit still has a float UPROPERTY named EXACTLY 'LadderClimbSpeedUU' — the hero resolves the rate by this name at runtime, so a rename must fail HERE and loudly, ⛔ not quietly at a ladder"),
		ReadClassDefaultFloat(ASummonedUnit::StaticClass(), TEXT("LadderClimbSpeedUU"), ShippedRate)))
	{
		return false;
	}

	TestEqual(TEXT("(a) and it is still the 350 uu/s TOWER-§8.5 derived from the Archer/Wizard Speed cell — ⚠️ Jonathan's exposure lever; TOWER-§9.3 prices the whole climb in Longbowman shots against it"),
		ShippedRate, ShippedClimbRateFromTheLaw, Exact);

	// ── (b) THE BUDGET, RE-DERIVED FROM THE LAW'S FORMULA ────────────────────────────────
	const FSiegeLadderClimbState HeroClimb = ArmedClimb(HeroCapsuleHalfHeightFromTheTemplate, ShippedRate);

	const float ExpectedAscentSeconds = PinnedLineLengthUU / ShippedRate;                                        // 3.5341
	const float ExpectedBudget = FSiegeLadderClimbStatics::TimeoutScale * ExpectedAscentSeconds;                 // 14.1364

	TestEqual(TEXT("(b) the line is the pinned 1,236.9 uu — ⭐ built from Δ=(300,0,1200), which a pure translation of the ladder (TASK-783, Jonathan's option A) leaves UNCHANGED"),
		HeroClimb.LengthUU, PinnedLineLengthUU, Tolerance);
	TestEqual(TEXT("(b) ⭐ the watchdog budget is 4 × the climb's own expected duration = 14.14 s against a 3.53 s ascent — generous ON PURPOSE: it must NEVER end a healthy climb early"),
		HeroClimb.TimeoutSeconds, ExpectedBudget, Tolerance);

	// ── (c) THE BUDGET IS FIXED AT ARMING TIME ───────────────────────────────────────────
	// ⭐ A mid-climb retune of the rate cannot extend a climb that is already running — and the
	// hero's cached resolved rate is the same discipline applied to the DRIVE.
	const FSiegeLadderClimbState SlowClimb = ArmedClimb(HeroCapsuleHalfHeightFromTheTemplate, ShippedRate * 0.5f);
	TestTrue(TEXT("(c) halving the rate DOUBLES the budget at arming time (so the budget is derived, ⛔ not a constant)"),
		SlowClimb.TimeoutSeconds > HeroClimb.TimeoutSeconds * 1.9f);

	// ── (d) A ZERO RATE CANNOT PRODUCE AN IMMORTAL CLIMB ─────────────────────────────────
	const FSiegeLadderClimbState ZeroRateClimb = ArmedClimb(HeroCapsuleHalfHeightFromTheTemplate, 0.f);
	TestTrue(TEXT("(d) a mis-tuned rate of 0 still produces a FINITE budget (MinClimbSpeedUU floors the divisor) — ⛔ never a division by zero and ⛔ never a hero that hangs until the match ends"),
		ZeroRateClimb.TimeoutSeconds > 0.f && FMath::IsFinite(ZeroRateClimb.TimeoutSeconds));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  17. THE HERO DUPLICATES NONE OF THE TOWER'S TUNABLES
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbNoDuplicateTunablesTest,
	"Siegebound.HeroLadderClimb.TheHeroOwnsNoneOfTheTowersContactTunables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbNoDuplicateTunablesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	const TCHAR* const TowerOwnedNames[] = {
		TEXT("LadderContactRadiusUU"), TEXT("LadderContactIntentCos"), TEXT("LadderContactDwellSeconds"), TEXT("LadderClimbSpeedUU")
	};

	for (const TCHAR* const Name : TowerOwnedNames)
	{
		TestNull(FString::Printf(
			TEXT("⛔ AHeroCharacter must NOT declare '%s'. `WR-§5`/`CONTACT-§8`: the radius, the cone, the dwell and the rate live on the OWNING ACTOR and the pawn only ASKS — ⚖️ two copies of a number is how they drift, and the drift is invisible until a player reports that one tower feels different"), Name),
			FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), Name));
	}

	// ── POSITIVE CONTROLS, BOTH WAYS ─────────────────────────────────────────────────────
	// ⭐ (i) the probe CAN find a float on the hero, so the four nulls above are findings and not a
	// broken lookup; (ii) the three contact names are REAL and still live on the tower, so a rename
	// there tells you the names moved instead of letting this test pass by aiming at nothing.
	TestNotNull(TEXT("⭐ POSITIVE CONTROL (i): the probe finds RecallChannelSeconds on the hero, so the four absences above mean something"),
		FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), TEXT("RecallChannelSeconds")));

	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestNotNull(FString::Printf(TEXT("⭐ POSITIVE CONTROL (ii): '%s' really does live on AClimbableTower — the name this test guards is a live one"), TowerOwnedNames[Index]),
			FindFProperty<FFloatProperty>(AClimbableTower::StaticClass(), TowerOwnedNames[Index]));
	}

	TestNotNull(TEXT("⭐ POSITIVE CONTROL (ii): 'LadderClimbSpeedUU' really does live on ASummonedUnit — which is where the hero READS it from"),
		FindFProperty<FFloatProperty>(ASummonedUnit::StaticClass(), TEXT("LadderClimbSpeedUU")));

	// ── AND NO COPIED LITERAL IN THE SHIPPED CLIMB REGION ────────────────────────────────
	// ⭐ A reflection check cannot see a number pasted into an expression. This one can.
	FString Source;
	if (LoadHeroCpp(*this, Source))
	{
		const int32 RegionStart = Source.Find(TEXT("LADDER CLIMB REGION BEGIN"), ESearchCase::CaseSensitive);
		const int32 RegionEnd = Source.Find(TEXT("LADDER CLIMB REGION END"), ESearchCase::CaseSensitive);
		if (TestTrue(TEXT("the shipped climb region's sentinels are present and ordered (the probe's own control)"),
			RegionStart != INDEX_NONE && RegionEnd != INDEX_NONE && RegionStart < RegionEnd))
		{
			const FString Region = Source.Mid(RegionStart, RegionEnd - RegionStart);

			TestFalse(TEXT("⛔ no copy of the tower's 150 uu contact radius appears in the hero's climb region"),
				Region.Contains(TEXT("150.f"), ESearchCase::CaseSensitive));
			TestFalse(TEXT("⛔ no copy of the tower's 0.35 s dwell appears in the hero's climb region"),
				Region.Contains(TEXT("0.35f"), ESearchCase::CaseSensitive));
			TestFalse(TEXT("⛔ no copy of the tower's 0.5 intent cosine appears in the hero's climb region — the hold-to-climb sustain is a SIGN test with no number at all, deliberately"),
				Region.Contains(TEXT("0.5f"), ESearchCase::CaseSensitive));
			TestFalse(TEXT("⛔ no copy of the 350 uu/s climb rate appears in the hero's climb region — it is READ from the one shipped property"),
				Region.Contains(TEXT("350.f"), ESearchCase::CaseSensitive));
			TestFalse(TEXT("⛔ and no capsule literal: ⛔ never 88 (the unit's half-height) in the hero's climb region"),
				Region.Contains(TEXT("88.f"), ESearchCase::CaseSensitive));
			TestFalse(TEXT("⛔ and ⛔ never 96 either — the hero's own half-height is READ from the capsule, not typed (CONTACT-§3.3 #1)"),
				Region.Contains(TEXT("96.f"), ESearchCase::CaseSensitive));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  18. THE K-4 DISARM RIDES THE ONE SHIPPED MELEE GUARD
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbDisarmTest,
	"Siegebound.HeroLadderClimb.TheClimbDisarmsMeleeAtTheOneShippedGuardPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbDisarmTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::DoMeleeAttack()"), Body))
	{
		return false;
	}

	// ⭐⭐ ONE TERM ON THE ***EXISTING*** GUARD — the `TOWER-§9.2` / `RECALL-§ R-5` idiom, third
	// application. ⛔ Not a new guard point, ⛔ not a new suppression mechanism, ⛔ not a timer.
	TestTrue(TEXT("⭐ the shipped melee guard now reads IsClimbing() IN THE SAME early-out as bDead / bMeleeSuppressed / the recall disarm — ⛔ a separate `if` further down would be a FOURTH guard point (TOWER-§9.2 refuses exactly that)"),
		Body.Contains(TEXT("bDead || bMeleeSuppressed || FSiegeRecallStatics::IsAttackDisarmed(RecallState) || IsClimbing()"), ESearchCase::CaseSensitive));

	// ⛔ AND THE DISARM IS A TRANSIENT STATE, ⛔ NOT A CLASS-IDENTITY SEAL: the same hero answers
	// "disarmed" during the ~3.5 s climb and "armed" on the frame it ends, because `IsClimbing()`
	// is backed by the same bool the driver runs on. ⇒ ⛔ no decay timer and ⛔ no grace window.
	TestFalse(TEXT("⛔ the climb adds no melee cooldown penalty, no suppression timer and no second disarm flag — the refusal does not even consume the cooldown"),
		Body.Contains(TEXT("SetMeleeSuppressed("), ESearchCase::CaseSensitive));

	// ⛔ AND THE ATTACKABLE HALF GETS NO CODE: nothing here narrows anyone's acquisition of the
	// climbing hero. Jonathan: "they should be attackable while climbing, but they can't attack back."
	const int32 GuardIndex = Body.Find(TEXT("IsClimbing()"), ESearchCase::CaseSensitive);
	TestEqual(TEXT("⭐ IsClimbing() appears EXACTLY ONCE in DoMeleeAttack — one term, one guard point"),
		CountOccurrences(Body, TEXT("IsClimbing()")), 1);
	TestTrue(TEXT("⭐ and it is in the function's opening guard, before any swing bookkeeping"),
		GuardIndex != INDEX_NONE && GuardIndex < Body.Len() / 3);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  19. THE CLIMB ADDS NO KEY, NO BINDING AND NO ESCAPE HANDLER
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbNoKeyTest,
	"Siegebound.HeroLadderClimb.TheClimbAddsNoKeyAndNoEscapeHandler",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbNoKeyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	const int32 RegionStart = Source.Find(TEXT("LADDER CLIMB REGION BEGIN"), ESearchCase::CaseSensitive);
	const int32 RegionEnd = Source.Find(TEXT("LADDER CLIMB REGION END"), ESearchCase::CaseSensitive);
	if (!TestTrue(TEXT("the shipped climb region's sentinels are present and ordered (the probe's own control — a probe that cannot find its region FAILS rather than reporting SAFE)"),
		RegionStart != INDEX_NONE && RegionEnd != INDEX_NONE && RegionStart < RegionEnd))
	{
		return false;
	}

	const FString Region = Source.Mid(RegionStart, RegionEnd - RegionStart);

	// ⭐ `K-A`: AUTO-START. His words were "simply by walking against it" — a confirm key is a
	// second mechanism he explicitly did not ask for.
	TestFalse(TEXT("⛔ the climb region names no engine key constant (K-A: it starts by WALKING, ⛔ never by a press; and KBD-§ would forbid a hardcoded key anyway)"),
		Region.Contains(TEXT("EKeys::"), ESearchCase::CaseSensitive));
	TestFalse(TEXT("⛔ the climb region adds no input binding of its own"),
		Region.Contains(TEXT("BindAction("), ESearchCase::CaseSensitive));
	TestFalse(TEXT("⛔⛔ AND IT ADDS NO `Escape` HANDLER OF ANY KIND — AS-§6 A-2 is untouchable and an automatic QA fail"),
		Region.Contains(TEXT("Escape"), ESearchCase::CaseSensitive));

	// ⭐ POSITIVE CONTROL: the region really is the climb (otherwise all three absences above would
	// be the absences of an empty string).
	TestTrue(TEXT("⭐ POSITIVE CONTROL: the extracted region contains the driver, so the three absences above are claims about real code"),
		Region.Contains(TEXT("SetMovementMode(MOVE_Flying)"), ESearchCase::CaseSensitive));

	// ⛔ AND NO ANIMATION (`CONTACT-§5` — Jonathan's explicit waiver). ⚠️ The waiver is a licence
	// ⛔ NOT TO BUILD; it is ⛔ not an instruction to remove: `A_SiegeBiped_Climb` stays imported,
	// committed and wired to `ABP_Footman`, and the UNITS keep it.
	TestFalse(TEXT("⛔ the climb region plays no montage and references no climb clip (CONTACT-§5: the animation is WAIVED — and ⛔ nothing here removes the unit's shipped clip either)"),
		Region.Contains(TEXT("PlayAnimMontage("), ESearchCase::CaseSensitive) || Region.Contains(TEXT("A_SiegeBiped_Climb"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  20. THE CALL SITE ASKS THE TOWER — AND NEVER SELF-STARTS AROUND ITS GATE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbCallSiteTest,
	"Siegebound.HeroLadderClimb.ThePollAsksTheTowerAndNeverSelfStarts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbCallSiteTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString PollBody;
	if (!ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::PollLadderContact(float DeltaSeconds)"), PollBody))
	{
		return false;
	}

	// ── (a) THE CALL SITE EXISTS ─────────────────────────────────────────────────────────
	// ⚠️⚠️ THE DEBT THIS TEST EXISTS TO PREVENT REPEATING: `TryBeginContactClimb` shipped with ZERO
	// callers, which is why nothing about the contact trigger could be observed at runtime.
	TestTrue(TEXT("(a) ⭐ the hero POLLS AClimbableTower::TryBeginContactClimb — the tower owns the radius, the cone, the dwell and the latch; the pawn only ASKS (WR-§5)"),
		PollBody.Contains(TEXT("TryBeginContactClimb(this, DeltaSeconds)"), ESearchCase::CaseSensitive));

	// ── (b) AND IT IS ACTUALLY REACHED FROM Tick ─────────────────────────────────────────
	// ⛔ A poll nothing calls is the same debt in a new place.
	FString TickBody;
	if (ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::Tick(float DeltaSeconds)"), TickBody))
	{
		TestTrue(TEXT("(b) the poll is reached from Tick"), TickBody.Contains(TEXT("PollLadderContact(DeltaSeconds)"), ESearchCase::CaseSensitive));

		const int32 DriverIndex = TickBody.Find(TEXT("TickLadderClimb(DeltaSeconds)"), ESearchCase::CaseSensitive);
		const int32 PollIndex = TickBody.Find(TEXT("PollLadderContact(DeltaSeconds)"), ESearchCase::CaseSensitive);
		TestTrue(TEXT("(b) ⭐ the DRIVER runs before the POLL, so a climb that ends this frame cannot be re-entered on the same frame"),
			DriverIndex != INDEX_NONE && PollIndex != INDEX_NONE && DriverIndex < PollIndex);
	}

	// ── (c) ⛔⛔ AND IT NEVER STARTS A CLIMB ITSELF ───────────────────────────────────────
	// Self-starting would skip `CanTeamAscend` (a SILENT BACK DOOR around Jonathan's T-3 ruling —
	// and there is ⛔ no hero exemption), skip the single occupancy slot (two capsules on one line,
	// where depenetration shoves one OFF it in mid-air), and leave `ActiveClimber` unset — so
	// `AClimbableTower::EndPlay` could not reach the hero and exit H-10 would not exist for it.
	TestFalse(TEXT("(c) ⛔⛔ the poll NEVER calls BeginLadderClimb itself. It asks and accepts the answer — including the refusal that blocks this feature today (NotAnAdmittedClimber). Bypassing the tower would open a back door around T-3, break TOWER-§10 L-1, and delete exit H-10 for the hero"),
		PollBody.Contains(TEXT("BeginLadderClimb("), ESearchCase::CaseSensitive));

	// ── (d) ⚖️⭐⭐ **INVERTED BY TASK-787 — READ THIS BEFORE FILING IT AS A WEAKENED TEST** ──────
	//
	// ⛔⛔ THE CAUSATION RUNS THE OTHER WAY. These two rows used to REQUIRE the poll to contain
	// `NotAnAdmittedClimber` and `bWarnedLadderStartSeamClosed`: TASK-778 could only ASK the tower
	// and be REFUSED on identity, so it latched a one-shot Warning naming the closed seam, and
	// these rows asserted the refusal was REPORTED rather than swallowed. ⭐ They were correct.
	//
	// ✅ `CONTACT-§12` then CLOSED that blocker by widening the tower's identity term from
	// `ASummonedUnit` to `ILadderClimber` — which `AHeroCharacter` implements as a COMPILE-TIME
	// BASE. ⇒ a hero can no longer PRODUCE that verdict, so a branch handling it is ⛔ unreachable
	// code and its Warning is one that ⛔ CANNOT FIRE. ⚖️ `SC-§36` INVERTED: a warning that cannot
	// fire is indistinguishable from one that works, and this one would read as a live diagnostic
	// forever. ⇒ the branch and the latch were REMOVED, and these rows now assert their ABSENCE.
	//
	// ⛔ THIS IS ⛔ NOT A WEAKENING TO A NULL CHECK: the claim is as specific as it was, it is
	// carried by a POSITIVE CONTROL below, and it goes RED the moment anyone re-introduces a
	// dead identity branch on this call site.
	// ⚠️ READ IN **CODE**, ⛔ NOT IN PROSE: the shipped call site EXPLAINS the removal and names the
	// verdict while doing so (that comment is `CONTACT-§12`'s record at the site it applies to).
	// ⇒ counting comment lines would force the source to choose between explaining itself and
	// passing this test, and the explanation would lose. ⚖️ The prose is the guard; the probe got
	// smarter instead (`CountOccurrencesInCode`).
	TestEqual(TEXT("(d) ⛔ the poll no longer BRANCHES on NotAnAdmittedClimber — TASK-787 widened the tower's identity term to ILadderClimber, so a HERO cannot produce that verdict and a branch for it would be unreachable code carrying a Warning that can never fire (SC-§36 inverted)"),
		CountOccurrencesInCode(PollBody, TEXT("NotAnAdmittedClimber")), 0);
	TestEqual(TEXT("(d) ⛔ …and the one-shot latch that reported it is GONE with it — ⛔ from the whole file, ⛔ not merely from this function"),
		CountOccurrencesInCode(Source, TEXT("bWarnedLadderStartSeamClosed")), 0);

	// ⭐⭐ POSITIVE CONTROL FOR BOTH ROWS ABOVE, AND IT IS ⛔ NOT OPTIONAL: two absence claims over a
	// body that failed to extract — or over a comment filter that swallowed the whole function —
	// would pass vacuously, which is the exact failure this file is written against. The poll DOES
	// still branch on a verdict, in code, so the scanner is proven live on the very shape it just
	// reported missing.
	TestEqual(TEXT("(d) SELF-CHECK: the code-only scanner still finds the verdict this poll DOES act on (ELadderContactVerdict::Climb) — so the two absences above are measurements, ⛔ not a broken probe"),
		CountOccurrencesInCode(PollBody, TEXT("ELadderContactVerdict::Climb")), 1);

	// ── (e) NO SECOND CONTACT STATE ON THE HERO ──────────────────────────────────────────
	// The dwell and `K-C`'s re-arm latch are the tower's bookkeeping, one row per pawn.
	TestFalse(TEXT("(e) ⛔ the hero keeps no FSiegeLadderContactState of its own — the dwell and the K-C re-arm latch are the TOWER's bookkeeping and a second copy would diverge from the gate that actually decides"),
		Source.Contains(TEXT("FSiegeLadderContactState"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  21. THE WATCHDOG IS ARMED ONLY FOR THE CLIMB
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbWatchdogTest,
	"Siegebound.HeroLadderClimb.TheWatchdogIsArmedOnlyForTheClimbAndClearedByEveryExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbWatchdogTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	// ── (a) ARMED IN EXACTLY ONE PLACE ───────────────────────────────────────────────────
	TestEqual(TEXT("(a) ⭐ the watchdog is armed from EXACTLY ONE place. ⚠️ MEASURED: this class had NO unconditional poll before TASK-778 (its only two SetTimer calls are the one-shot Rally cooldown and the conditional War Banner aura) ⇒ a driver copied from the unit's would have shipped with NO WATCHDOG AT ALL and looked identical in review"),
		CountOccurrences(Source, TEXT("SetTimer(LadderClimbWatchdogTimerHandle")), 1);

	FString BeginBody;
	if (ExtractFunctionBody(*this, Source, TEXT("bool AHeroCharacter::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)"), BeginBody))
	{
		TestTrue(TEXT("(a) and it is armed in BeginLadderClimb, beside the mode change it exists to undo"),
			BeginBody.Contains(TEXT("SetTimer(LadderClimbWatchdogTimerHandle"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("(a) ⭐ the deadline is the climb's OWN budget on the WORLD clock, so a Tick that stops cannot postpone it"),
			BeginBody.Contains(TEXT("LadderClimb.TimeoutSeconds"), ESearchCase::CaseSensitive));
	}

	// ── (b) ⛔ AND IT IS NOT A PERMANENT POLL ────────────────────────────────────────────
	// ⭐ One timer armed for ~3.5 s, ⛔ never a standing 0.25 s poll added to the hero for a rare
	// feature. If BeginPlay or the constructor ever arms it, that is what happened.
	FString BeginPlayBody;
	if (ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::BeginPlay()"), BeginPlayBody))
	{
		TestFalse(TEXT("(b) ⛔ BeginPlay does NOT arm the watchdog — it is armed ONLY for a climb's duration (CONTACT-§3.5)"),
			BeginPlayBody.Contains(TEXT("LadderClimbWatchdogTimerHandle"), ESearchCase::CaseSensitive));
	}

	FString ConstructorBody;
	if (ExtractFunctionBody(*this, Source, TEXT("AHeroCharacter::AHeroCharacter()"), ConstructorBody))
	{
		TestFalse(TEXT("(b) ⛔ nor does the constructor"),
			ConstructorBody.Contains(TEXT("LadderClimbWatchdogTimerHandle"), ESearchCase::CaseSensitive));
	}

	// ── (c) CLEARED BY THE ONE TEARDOWN, AND SELF-CLEARING AS A BELT ─────────────────────
	FString WatchdogBody;
	if (ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::OnLadderClimbWatchdog()"), WatchdogBody))
	{
		TestTrue(TEXT("(c) a watchdog that finds no climb clears its own handle rather than looping forever on a walking hero"),
			WatchdogBody.Contains(TEXT("ClearTimer(LadderClimbWatchdogTimerHandle)"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("(c) ⭐ an expired budget DROPS the hero where it is (ESiegeHeroLadderExit::Watchdog) — TOWER-§8.5a clause 5: ⛔ never handed the deck it failed to reach"),
			WatchdogBody.Contains(TEXT("ESiegeHeroLadderExit::Watchdog"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("(c) and a dead hero still holding a climb is ended as a DEATH rather than as a timeout (the H-4 belt)"),
			WatchdogBody.Contains(TEXT("ESiegeHeroLadderExit::Death"), ESearchCase::CaseSensitive));
	}

	// ── (d) THE ONE WARNING THIS FEATURE EVER LOGS ───────────────────────────────────────
	// `TOWER-§8.5a` clause 7: a stall before the window opens must fail LOUDLY, ⛔ never silently.
	FString TeardownBody;
	if (ExtractFunctionBody(*this, Source, TEXT("void AHeroCharacter::EndLadderClimb(bool bReachedTop, ESiegeHeroLadderExit Reason)"), TeardownBody))
	{
		TestTrue(TEXT("(d) the watchdog exit is the ONE exit that logs at Warning — every other reason is ordinary gameplay (TOWER-§8.5a clause 7: a stall must fail LOUDLY)"),
			TeardownBody.Contains(TEXT("Warning"), ESearchCase::CaseSensitive)
			&& TeardownBody.Contains(TEXT("Reason == ESiegeHeroLadderExit::Watchdog"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  22. THE POLL'S HOME — THE HERO'S TICK IS UNCONDITIONAL, AND NOTHING MAY DISABLE IT
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbTickIsUnconditionalTest,
	"Siegebound.HeroLadderClimb.TheHerosTickIsUnconditionalSoAPollInItActuallyRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbTickIsUnconditionalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	// ⚠️⚠️ THE TRAP THIS TEST GUARDS, AND IT IS A ***MEASURED*** ONE FROM THE UNIT SIDE:
	// `ASummonedUnit` ships `bCanEverTick = true` WITH `bStartWithTickEnabled = false`, and its only
	// tick-flag writer turns the tick ON only while lunging or climbing ⇒ a poll in ITS `::Tick`
	// would compile, review clean, pass the suite and ⛔ NEVER RUN in the state a climb starts from.
	// ⚖️ A caller that exists but never runs is indistinguishable from one that works — the same
	// silent class as the zero-caller gap that blocked this batch.
	FString HeroSource;
	if (!LoadHeroCpp(*this, HeroSource))
	{
		return false;
	}

	TestTrue(TEXT("(a) the hero's constructor enables ticking"),
		HeroSource.Contains(TEXT("PrimaryActorTick.bCanEverTick = true"), ESearchCase::CaseSensitive));
	TestFalse(TEXT("(a) ⛔ and it does NOT ship bStartWithTickEnabled = false — the flag that makes the unit's tick OFF in exactly the state a climb starts from"),
		HeroSource.Contains(TEXT("PrimaryActorTick.bStartWithTickEnabled = false"), ESearchCase::CaseSensitive));

	// ⛔⛔ AND NOTHING IN THIS CLASS MAY EVER DISABLE THE TICK. The day somebody optimises a hidden,
	// dead hero by switching its tick off, the climb driver AND this poll die with it — and the
	// watchdog would be the only thing left standing between the player and a permanent hang.
	TestEqual(TEXT("(b) ⛔⛔ the hero writes its actor tick flag ZERO times. A future SetActorTickEnabled(false) here would silently kill the driver and the contact poll — this assertion is what makes that a RED SUITE instead of a player hanging in mid-air"),
		CountOccurrences(HeroSource, TEXT("SetActorTickEnabled(")), 0);

	// ── POSITIVE CONTROL, AND IT IS THE WHOLE REASON THIS TEST IS CREDIBLE ────────────────
	// ⭐ The same probe, aimed at the class that DOES write the flag, must find it. Without this,
	// (b) would pass just as happily against a broken search.
	FString UnitSource;
	if (LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp"), UnitSource))
	{
		TestTrue(TEXT("⭐ POSITIVE CONTROL: the identical probe FINDS SetActorTickEnabled( on ASummonedUnit, which does write its own flag — so the zero above is a finding, ⛔ not a broken search"),
			CountOccurrences(UnitSource, TEXT("SetActorTickEnabled(")) > 0);
		TestTrue(TEXT("⭐ POSITIVE CONTROL: and ASummonedUnit really does ship bStartWithTickEnabled = false — the trap is real, and the hero genuinely does not have it"),
			UnitSource.Contains(TEXT("PrimaryActorTick.bStartWithTickEnabled = false"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  23. CAN THE HERO ACTUALLY SATISFY THE DWELL? (`K-5` was derived at v = 300)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbDwellIsReachableTest,
	"Siegebound.HeroLadderClimb.TheShippedContactRadiusAdmitsTheHerosOwnSpeeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbDwellIsReachableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	// ⭐⭐ THE ARITHMETIC NOBODY DID WHEN `K-5` WAS WRITTEN, AND IT DECIDES WHETHER THE FEATURE CAN
	// FIRE AT ALL: the three contact numbers were derived against **v = 300 uu/s**, the slowest
	// unit. A pawn approaching the endpoint dead-on is inside the disc for `R / v` seconds, and the
	// dwell must fit inside that. ⛔ The HERO is the fastest body in the game.
	float RadiusUU = 0.f;
	float DwellSeconds = 0.f;
	if (!TestTrue(TEXT("the tower still ships LadderContactRadiusUU (the probe's own control)"),
			ReadClassDefaultFloat(AClimbableTower::StaticClass(), TEXT("LadderContactRadiusUU"), RadiusUU))
		|| !TestTrue(TEXT("the tower still ships LadderContactDwellSeconds"),
			ReadClassDefaultFloat(AClimbableTower::StaticClass(), TEXT("LadderContactDwellSeconds"), DwellSeconds)))
	{
		return false;
	}

	float WalkSpeed = 0.f;
	float SprintSpeed = 0.f;
	float MoveSpeedBonus = 0.f;
	if (!TestTrue(TEXT("the hero still ships WalkSpeed"), ReadClassDefaultFloat(AHeroCharacter::StaticClass(), TEXT("WalkSpeed"), WalkSpeed))
		|| !TestTrue(TEXT("the hero still ships SprintSpeed"), ReadClassDefaultFloat(AHeroCharacter::StaticClass(), TEXT("SprintSpeed"), SprintSpeed))
		|| !TestTrue(TEXT("the hero still ships MoveSpeedBonus (Swift Boots)"), ReadClassDefaultFloat(AHeroCharacter::StaticClass(), TEXT("MoveSpeedBonus"), MoveSpeedBonus)))
	{
		return false;
	}

	// ⚠️ THE WORST CASE IS ⛔ NOT THE BASE SPRINT — it is a SWIFT-BOOTS sprint, and that upgrade is
	// one card play away. `GetEffectiveSprintSpeed()` = SprintSpeed × (1 + MoveSpeedBonus × stacks),
	// cap 1 stack.
	const float FastestHeroSpeed = SprintSpeed * (1.f + MoveSpeedBonus);

	// ── (a) THE PREDICATE ITSELF DISCRIMINATES ───────────────────────────────────────────
	// ⭐ Asserted on FABRICATED radii, so this test can never become a tautology about whatever the
	// tower happens to ship: a huge disc admits the fastest hero, a tiny one cannot.
	TestTrue(TEXT("(a) ⭐ CONTROL: a 2,000 uu disc would admit even a Swift-Boots sprint at the shipped dwell"),
		(2000.f / FastestHeroSpeed) >= DwellSeconds);
	TestFalse(TEXT("(a) ⭐ CONTROL: a 10 uu disc could not — so this arithmetic really does discriminate"),
		(10.f / FastestHeroSpeed) >= DwellSeconds);

	// ── (b) THE FOUR SHIPPED HERO SPEEDS, REPORTED AS NUMBERS ────────────────────────────
	// ⚖️ Reported rather than asserted, ⛔ deliberately: `LadderContactRadiusUU` lives on
	// `AClimbableTower` — another task's file and another task's board row (`K-6`) — so a failure
	// here would be this suite failing someone else's un-landed work. ⭐ The WARNING is permanent
	// and appears on every run until the radius admits the hero, which is the honest instrument.
	//
	// ⚠️ AND IT IS `AddWarning`, ⛔ NOT `AddInfo` — the `SiegeKeyboardLayoutTest.cpp:1416` precedent
	// was READ AND DELIBERATELY NOT FOLLOWED, because that case and this one are different: there,
	// a differing resolution is *"an observation about the host, not a defect"*; here, an
	// unsatisfiable dwell means THE FEATURE CANNOT FIRE FOR THIS PAWN AT ALL. ⭐ Colouring the run
	// is exactly what that deserves. ✅ Checked before choosing it: no `Config/*.ini` in this
	// project enables warnings-as-errors, so this ⛔ cannot turn TASK-780's suite red for a defect
	// that lives in another task's file.
	struct FHeroApproach { const TCHAR* Label; float Speed; };
	const FHeroApproach Approaches[] = {
		{ TEXT("walk"), WalkSpeed },
		{ TEXT("walk + Swift Boots"), WalkSpeed * (1.f + MoveSpeedBonus) },
		{ TEXT("sprint"), SprintSpeed },
		{ TEXT("sprint + Swift Boots (WORST CASE)"), FastestHeroSpeed }
	};

	for (const FHeroApproach& Approach : Approaches)
	{
		const float InConeSeconds = RadiusUU / Approach.Speed;
		if (InConeSeconds < DwellSeconds)
		{
			AddWarning(FString::Printf(
				TEXT("⛔ UNREACHABLE DWELL — hero %s: at %.0f uu/s a dead-on approach spends only %.3f s inside the %.0f uu contact disc, against a %.2f s dwell. The hero could walk into that ladder forever and NOTHING would happen. ⇒ board row K-6 (LadderContactRadiusUU on AClimbableTower); the required minimum for this speed is %.1f uu. ⛔ The hero deliberately keeps NO copy of that tunable to work around it."),
				Approach.Label, Approach.Speed, InConeSeconds, RadiusUU, DwellSeconds, Approach.Speed * DwellSeconds));
		}
	}

	// ── (c) THE ONE HARD ASSERTION: THE HERO OWNS NO WORKAROUND ──────────────────────────
	// ⭐ THIS is the claim that belongs to TASK-778 and it is asserted, ⛔ not warned: whatever the
	// radius turns out to be, the fix is ⛔ NEVER a hero-side copy of it (777's own suite fails that
	// too). The reachability above is the tower's row; this line is the hero's.
	TestNull(TEXT("(c) ⛔ whatever K-6 rules, the hero owns NO LadderContactRadiusUU of its own — the dwell is fixed on the TOWER or not at all"),
		FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), TEXT("LadderContactRadiusUU")));
	TestNull(TEXT("(c) ⛔ and no hero-side dwell either"),
		FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), TEXT("LadderContactDwellSeconds")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  24. ⭐⭐ THE COMPLETION SEAM — THE HERO **RELEASES** THE TOWER'S OCCUPANCY SLOT,
//      ONCE, LAST, FROM THE ONE TEARDOWN (TASK-787, `CONTACT-§12.3`)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⛔⛔ THIS IS THE TEST THE WIDENING WAS REALLY FOR, AND THE DEFECT IT GUARDS IS ⛔ WORSE THAN THE
// ONE IT FIXES. TASK-778 shipped a complete hero climb that could not START. If TASK-787 had
// widened only the START, the first hero to walk into a ladder would have been ADMITTED to the
// tower's single occupancy slot and would ⛔ NEVER have released it — because the tower learns a
// climb ended through ⛔ exactly one channel, and this class had ⛔ no completion delegate at all.
// ⇒ every later climber, HERO OR UNIT, would get `LadderBusy` for the rest of the match:
// *"the hero cannot climb"* becomes *"the first hero attempt disables the tower for everyone."*
//
// ⭐ WHAT THIS TEST CAN AND CANNOT SEE, STATED (`SC-§32`): a world-less hero cannot be driven
// through a climb at all (see this file's header), so the RELEASE ITSELF — broadcast → the tower's
// `HandleLadderClimbEnded` → `ReleaseClimber` → the slot clears — is proven in three separable
// pieces: the hero's END of it here, the tower's END of it in
// `SiegeClimbableTowerTest.cpp` test 15, and the runtime observation in TASK-780's PIE session
// (walk the hero up, end the climb, climb AGAIN — a second refusal means the slot leaked).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHeroLadderClimbCompletionSignalTest,
	"Siegebound.HeroLadderClimb.TheTeardownBroadcastsTheOneCompletionSignalLastAndOnlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHeroLadderClimbCompletionSignalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHeroLadderClimbTestFixture;

	FString Source;
	if (!LoadHeroCpp(*this, Source))
	{
		return false;
	}

	FString TeardownBody;
	if (!ExtractFunctionBody(*this, Source,
		TEXT("void AHeroCharacter::EndLadderClimb(bool bReachedTop, ESiegeHeroLadderExit Reason)"), TeardownBody))
	{
		return false;
	}

	// ── (a) ⭐ THE BROADCAST EXISTS, AND IT IS IN THE **TEARDOWN** ───────────────────────
	// ⛔ Not in `AbortLadderClimb`, ⛔ not in `Tick`, ⛔ not at the arrival branch: all TEN exits
	// route through this ONE function, so putting the signal here is what makes "every exit
	// releases the ladder" true by CONSTRUCTION rather than by ten copies of one line.
	const int32 BroadcastIndex = TeardownBody.Find(TEXT("OnLadderClimbEnded.Broadcast("), ESearchCase::CaseSensitive);
	TestTrue(TEXT("(a) ⭐⭐ EndLadderClimb broadcasts OnLadderClimbEnded — the tower's ONLY way to learn a hero climb ended, and without it the occupancy slot is claimed and NEVER released (the ladder is BRICKED for the rest of the match)"),
		BroadcastIndex != INDEX_NONE);

	TestTrue(TEXT("(a) …passing THIS hero and the arrival flag — the pinned (ACharacter*, bool) signature, satisfied by the implicit conversion CONTACT-§4.4 widened the delegate for"),
		TeardownBody.Contains(TEXT("OnLadderClimbEnded.Broadcast(this, bReachedTop)"), ESearchCase::CaseSensitive));

	// ── (b) ⛔ EXACTLY ONCE, AND EXACTLY ONE SITE IN THE WHOLE FILE ──────────────────────
	// ⚠️ A SECOND broadcast site would double-fire the tower's handler. That is survivable today
	// (`ReleaseClimber` is idempotent) and it is ⛔ still refused: two sites is two orderings, and
	// the second one is the one nobody tests.
	TestEqual(TEXT("(b) ⛔ EXACTLY ONE broadcast in the teardown"),
		CountOccurrences(TeardownBody, TEXT("OnLadderClimbEnded.Broadcast(")), 1);
	TestEqual(TEXT("(b) ⛔ …and EXACTLY ONE in the whole file — ten exits, ONE teardown, ONE signal"),
		CountOccurrences(Source, TEXT("OnLadderClimbEnded.Broadcast(")), 1);

	// ── (c) ⭐⭐ THE ORDERING, AND IT IS THE PART THAT BITES ─────────────────────────────
	// ⛔ The broadcast is LAST: after the exactly-once latch is consumed and after the movement
	// mode is restored.
	//   • AFTER THE LATCH — `FSiegeLadderClimbStatics::End` returns false on a second entry and
	//     this function returns immediately, so a listener that re-enters `AbortLadderClimb()`
	//     from inside the delegate is INERT rather than RECURSIVE. ⭐ Exactly-once is INHERITED
	//     from that latch; there is deliberately ⛔ no second flag guarding the broadcast.
	//   • AFTER THE RESTORE — a listener that inspects this hero (or hands it back to path
	//     following) must ⛔ never see it mid-teardown, still in `MOVE_Flying`.
	// ⚠️ Both are ORDER claims, so both are read as INDICES: a "contains" pair would pass with the
	// broadcast at the top of the function, which is precisely the bug.
	const int32 LatchIndex = TeardownBody.Find(TEXT("FSiegeLadderClimbStatics::End(LadderClimb)"), ESearchCase::CaseSensitive);
	const int32 RestoreIndex = TeardownBody.Find(TEXT("SetDefaultMovementMode()"), ESearchCase::CaseSensitive);

	// SELF-CHECK: both anchors are real. An INDEX_NONE anchor is −1, which would make every
	// "comes after" comparison below trivially true — the vacuous pass this file is written against.
	TestTrue(TEXT("(c) SELF-CHECK: the exactly-once latch is where this test thinks it is"), LatchIndex != INDEX_NONE);
	TestTrue(TEXT("(c) SELF-CHECK: the movement-mode restore is where this test thinks it is"), RestoreIndex != INDEX_NONE);

	if (LatchIndex != INDEX_NONE && RestoreIndex != INDEX_NONE && BroadcastIndex != INDEX_NONE)
	{
		TestTrue(TEXT("(c) ⭐ the broadcast comes AFTER the exactly-once latch — so a listener that re-enters AbortLadderClimb() is inert, ⛔ not recursive"),
			BroadcastIndex > LatchIndex);
		TestTrue(TEXT("(c) ⭐ the broadcast comes AFTER SetDefaultMovementMode() — ⛔ no listener ever sees this hero mid-teardown, still in MOVE_Flying"),
			BroadcastIndex > RestoreIndex);
	}

	// ── (d) ⛔ AND THE TEARDOWN IS STILL THE ONLY PLACE THE MODE IS RESTORED ─────────────
	// ⭐ Re-asserted here, ⛔ not borrowed: this test's whole ordering claim is meaningless if the
	// restore it measures against is one of several.
	TestEqual(TEXT("(d) ⛔ SetDefaultMovementMode() appears EXACTLY ONCE in the whole file — ten exits, ONE restore, and the broadcast is ordered against THAT one"),
		CountOccurrences(Source, TEXT("SetDefaultMovementMode()")), 1);

	// ── (e) ⛔ ONE GUARD, AT THE TOP — SO ⛔ NOTHING CAN SKIP THE BROADCAST ───────────────
	// ⚖️ `CONTACT-§12.3`: exactly-once is INHERITED from the ten-exits-one-teardown property that
	// TASK-778 built, ⛔ NOT re-latched around the signal. ⇒ this function has EXACTLY ONE early
	// exit — the latch — and everything after it, the broadcast included, is unconditional.
	//
	// ⭐ WHY COUNT `return` RATHER THAN GUESS A FLAG NAME: a second latch, a "did we already tell
	// the tower?" bool, or a defensive `if (!OnLadderClimbEnded.IsBound()) return;` would all show
	// up here, and ⛔ none of them would have to be spelled the way a test author guessed. ⚠️ A
	// row that can only fail if somebody picks the identifier you predicted is a row that can only
	// fail by accident — this file has four of those to its name already, and ⛔ not a fifth.
	TestEqual(TEXT("(e) ⛔ the teardown has EXACTLY ONE early return — the exactly-once latch. Any second guard could skip the completion signal for some exits, which is the ladder-bricking defect wearing a different hat"),
		CountOccurrences(TeardownBody, TEXT("return;")), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
