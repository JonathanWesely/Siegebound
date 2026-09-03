// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WidgetComponent.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "HAL/UnrealMemory.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE TWO-ENDED CAST BAR — THE C++ HALF (TASK-860; law `WITCH-§9`,
 *      gate `TASK-862`; `SHIP-§9`, `HIGH-§1`) ═══
 *
 *  Subject: `UCombatantHealthBarComponent`'s five pure cast seams — `ShouldPollCastProgress`,
 *  `ShouldPushCastRow`, `SanitizeCastPercent`, `ComputeCastBarHeightPixels` and `ComputeCastPivotY` —
 *  plus the shipped `DrawSize`/`Pivot` defaults read off the CDO, the two `EditDefaultsOnly` cast
 *  tunables read by reflection, and six structural guards.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE QUESTION THIS FILE EXISTS TO ANSWER, AND IT IS NOT "DOES THE ARITHMETIC WORK"
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `WITCH-§9` exists because "starting", "running" and "broken" are ⛔ IDENTICAL PIXELS on a witch
 *  who spawns static. Jonathan did not ask for a 3-second cast; he asked for one ⛔ THAT CAN BE
 *  INTERRUPTED — and counterplay the player cannot perceive is not counterplay.
 *
 *  ⇒ `SHIP-§9` (validate a gate against the FAILURE it detects) makes the deliverable a suite that
 *  can tell a ⛔ BROKEN cast from a ⛔ COMPLETED one. That is a question about a ⛔ SEQUENCE OF EVENTS
 *  OVER TIME, not about one value, so **test 7 replays whole casts** and **test 8 replays the same
 *  two casts through the ⛔ TEMPTING BUG** to prove the suite goes red on it. ⚖️ A suite that cannot
 *  tell those two apart ⛔ IN CODE has reproduced, one layer down, the exact defect this feature
 *  exists to remove.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ THE FLEET-WIDE HAZARD THIS TASK CARRIES, AND WHY TEST 5 IS THE FIRST ONE TO READ
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `BoostOutline` and `Bar` are ⛔ `Fill` slots — they absorb every spare pixel. So a `DrawSize` bump
 *  in the ⛔ CONSTRUCTOR (the obvious reading of "DrawSize grows to fit the new segment") renders
 *  `Bar` at ⛔ 19.33 px instead of 14.00 on ⛔ EVERY BUILDING, THE HERO AND ALL 20+ UNITS, casting or
 *  not — a permanent, silent, game-wide health-bar resize shipped by a witch task. `TASK-861`
 *  measured that before either half was written.
 *
 *  ⇒ the bar grows ⛔ AT CAST TIME, and **test 5 reads the shipped defaults off the CDO** so the
 *  regression is caught by a ⛔ RUNTIME READ rather than by a source string. ⭐ Test 5 is the row that
 *  goes red the moment somebody "simplifies" the geometry back into the constructor.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHY THIS RUNS HEADLESS AT ALL
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  All five seams are `static` and pure — bools and floats in, one value out; no `UWorld`, no
 *  `AActor`, no widget, no Slate, no clock (the `ComputeDesiredBarVisibility` / `HIGH-§3`
 *  precedent). ⚠️ If a test here ever starts needing a world, that purity has been broken and it is
 *  a FINDING, not a reason to add a fixture. (The house rule bans `SpawnActor`/`CreateWorld` under
 *  `Siegebound/Tests/`, and nothing here needs them.)
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — STATED SO NOBODY MISTAKES GREEN FOR DONE
 *      (`SC-§32`: a mechanism never observed to function is not known to function)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔⛔ **THAT ANY AMBER PIXEL IS EVER DRAWN. NOTHING IN THIS FILE IS EVIDENCE THE BAR APPEARS.**
 *      `CastBarRoot` / `CastBarFill` ⛔ DO NOT EXIST YET: `TASK-861` is blocked on an irreducible
 *      ~90-second 🧑 JONATHAN step (MCP cannot instantiate a widget into a `WidgetTree`). Until that
 *      lands, `OnCastProgressChanged` is a `BlueprintImplementableEvent` with ⛔ NO implementation,
 *      which in UE is a ⛔ SILENT NO-OP. ⇒ this half is ⛔ INERT AND HARMLESS by construction, and
 *      every test below is a test of the ⛔ DATA the widget will be given, ⛔ never of what is shown.
 *    • ⛔ **WHETHER 4.00 px OF AMBER READS AT GAMEPLAY DISTANCE**, whether centre-out reads as
 *      different in motion, and whether the TWO-ENDED pair reads as ONE event. `TASK-861` §9 asked
 *      those questions and correctly refused to answer them; they are ⛔ Jonathan's eye, not a green
 *      suite. `TASK-131` is this project's record of structural readback ⛔ PASSING on a visually
 *      broken widget.
 *    • ⛔ **THAT `SetDrawSize`/`SetPivot` REALLY MOVE A SCREEN-SPACE WIDGET.** That is an ENGINE
 *      guarantee (`SWorldWidgetScreenLayer.cpp` feeds `Pivot` to the canvas slot as its ALIGNMENT,
 *      re-read every frame beside `GetDrawSize`). A test cannot add confidence to it.
 *    • ⛔ **THE PROVIDER'S OWN CORRECTNESS.** `ASummonedUnit::GetCastProgressPercent` /
 *      `IsCastInProgress` are `TASK-830`'s, asserted in `SiegeInvisibilityTest` tests 31-32. ⛔ This
 *      file deliberately does not re-pin them — a second owner of one pin is how counts drift.
 *    • ⚠️ **THE ≤1-POLL STALENESS.** An interrupt landing inside the FINAL poll window (≤50 ms of a
 *      3 s cast, i.e. 1.7% of it) still paints as "nearly full". Declared on the tunable, and
 *      disambiguated by the other half of the signal — a COMPLETED cast also turns the target
 *      translucent (`MI_Unit_Invisible`); a broken one changes nothing.
 */

namespace SiegeCastBarTestFixture
{
	/** Float comparison tolerance (the SiegeHighGroundTest / SiegeHealthBarOcclusionTest house value). */
	constexpr float Tolerance = 1.e-4f;

	/** A 60 fps frame, as a float — the tick the cast poll actually rides. */
	constexpr float FrameSeconds = 1.f / 60.f;

	/** The shipped Witch channel, `WitchCastSeconds` (TASK-830). Used as the window every replay below runs. */
	constexpr float ShippedCastSeconds = 3.f;

	/**
	 *  The shipped cast poll period. ⛔ RESTATED HERE AS A LITERAL ON PURPOSE, and test 9 asserts the
	 *  component's own default EQUALS it by reflection — so this constant cannot drift away from the
	 *  shipped value without a red row. (A test that read the value it is checking would prove nothing.)
	 */
	constexpr float ShippedPollSeconds = 0.05f;

	/** The occlusion cull's period — the WRONG one for this row, and test 9 is the arithmetic proving it. */
	constexpr float OcclusionPollSeconds = 0.15f;

	/**
	 *  Usable fill width of the bar in screen pixels (`DrawSize.X` 90 less the row's padding), quoted
	 *  from `CastBarRowHeightPixels`'s own budget comment. Used ONLY to turn a percentage shortfall
	 *  into the pixels a player would (or would not) see.
	 */
	constexpr float BarFillWidthPixels = 88.f;

	/**
	 *  A quiet NaN and a positive infinity, built from their IEEE-754 bit patterns.
	 *  ⛔ NOT `0.f / 0.f` or `FMath::Sqrt(-1.f)`: both are constant-foldable and a fast-math build may
	 *  evaluate them at compile time into something that is no longer non-finite — which would make
	 *  test 2's NaN clause pass while testing nothing.
	 */
	float MakeQuietNaN()
	{
		const uint32 NaNBits = 0x7FC00000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &NaNBits, sizeof(Result));
		return Result;
	}

	float MakePositiveInfinity()
	{
		const uint32 InfBits = 0x7F800000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &InfBits, sizeof(Result));
		return Result;
	}

	//~ ── THE CAST REPLAY HARNESS (tests 7-9) ──────────────────────────────────────────────────────

	/** One poll's worth of provider answer — exactly the pair `UpdateCastProgress` reads together. */
	struct FCastSample
	{
		bool bCasting = false;
		float ProviderPercent = 0.f;
	};

	/** One `OnCastProgressChanged` the widget would receive. */
	struct FCastEvent
	{
		float CastPercent = 0.f;
		bool bCasting = false;
	};

	/**
	 *  Builds the sequence of provider answers a whole cast produces, from the arithmetic
	 *  `ASummonedUnit::GetCastProgressPercent` actually runs: `100 * Elapsed / Rate` off the LIVE
	 *  one-shot timer, and `(false, 0)` once the timer is gone.
	 *
	 *  ⭐ THE LAST LIVE SAMPLE IS THE WHOLE POINT AND IT IS NEVER 100: the timer's callback CLEARS the
	 *  handle, so the last value any poll can observe on a COMPLETED cast is the one taken ONE POLL
	 *  before the end — `100 * (Duration - Interval) / Duration`. Test 9 is the consequence.
	 *
	 *  `InterruptAtSeconds` >= `CastDurationSeconds` ⇒ a completed cast; below it ⇒ a broken one.
	 */
	TArray<FCastSample> MakeCastRun(float CastDurationSeconds, float PollIntervalSeconds, float InterruptAtSeconds)
	{
		TArray<FCastSample> Samples;

		if (CastDurationSeconds <= 0.f || PollIntervalSeconds <= 0.f)
		{
			// Never reached from the rows below; a guard so a mistyped parameter cannot spin forever.
			return Samples;
		}

		const float EndSeconds = FMath::Min(CastDurationSeconds, InterruptAtSeconds);

		for (int32 PollIndex = 1; PollIndex < 100000; ++PollIndex)
		{
			const float PollTimeSeconds = PollIndex * PollIntervalSeconds;
			if (PollTimeSeconds >= EndSeconds)
			{
				break;
			}

			Samples.Add(FCastSample{ true, 100.f * PollTimeSeconds / CastDurationSeconds });
		}

		// The FIRST poll after the cast ended — completed or interrupted, the provider answers
		// identically here because the teardown is the same one line (the timer is cleared).
		Samples.Add(FCastSample{ false, 0.f });

		return Samples;
	}

	/**
	 *  Replays a run through the component's SHIPPED decision functions, composed exactly as
	 *  `UpdateCastProgress` -> `PushCastProgress` compose them, and returns the events the widget
	 *  would receive.
	 *
	 *  ⛔ `bUseShippedGate == false` swaps in THE TEMPTING BUG — `if (bCasting)` instead of
	 *  `ShouldPushCastRow` — so test 8 can validate the gate against the failure it detects rather
	 *  than merely against success (`SHIP-§9`). ⚠️ That branch is a MODEL of a wrong implementation,
	 *  ⛔ not a switch that exists in shipping code.
	 */
	TArray<FCastEvent> ReplayCastRun(const TArray<FCastSample>& Samples, bool bUseShippedGate)
	{
		TArray<FCastEvent> Events;

		// The component's `bCastRowDriven` latch: what the row was last DRIVEN to, never "is a cast
		// running". The answer always comes from the provider on the poll that asks.
		bool bRowDriven = false;

		for (const FCastSample& Sample : Samples)
		{
			const bool bShouldPush = bUseShippedGate
				? UCombatantHealthBarComponent::ShouldPushCastRow(Sample.bCasting, bRowDriven)
				: Sample.bCasting;

			if (!bShouldPush)
			{
				continue;
			}

			// PushCastProgress latches unconditionally and FIRST, before anything can early out.
			bRowDriven = Sample.bCasting;

			Events.Add(FCastEvent{
				UCombatantHealthBarComponent::SanitizeCastPercent(Sample.bCasting, Sample.ProviderPercent),
				Sample.bCasting });
		}

		return Events;
	}

	/** Highest percent the widget was ever told during a run — the number that separates COMPLETED from BROKEN. */
	float PeakCastPercent(const TArray<FCastEvent>& Events)
	{
		float Peak = 0.f;
		for (const FCastEvent& Event : Events)
		{
			Peak = FMath::Max(Peak, Event.CastPercent);
		}
		return Peak;
	}

	/** How many events carried `bCasting == false` — i.e. how many times the row was told to come DOWN. */
	int32 CountRowDownEvents(const TArray<FCastEvent>& Events)
	{
		int32 Count = 0;
		for (const FCastEvent& Event : Events)
		{
			if (!Event.bCasting)
			{
				++Count;
			}
		}
		return Count;
	}

	//~ ── SOURCE PROBES ────────────────────────────────────────────────────────────────────────────
	//  ⚠️ A SOURCE-STRING PROBE IS A WEAK INSTRUMENT and is used below only where the thing it guards
	//  has no other headless witness — call-graph placement, symbol absence, and the C++↔widget
	//  contract `TASK-861` binds by string. ⛔ Every probe carries a POSITIVE CONTROL so it can never
	//  pass on a moved file, a renamed symbol or an empty string (the SiegeHeroCameraTest test-7
	//  idiom), and every count it asserts was measured on disk before shipping, not predicted.

	/** Loads a project-relative source file, failing the test (rather than passing vacuously) if it cannot be read. */
	bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' does not exist — the probe is STALE, so it FAILS rather than passing on an empty string."), *FullPath));
			return false;
		}

		if (!FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — a stale probe fails."), *FullPath));
			return false;
		}

		return true;
	}

	/**
	 *  Counts occurrences of a needle on CODE lines only — comment lines are skipped. A faithful
	 *  replica of `SiegeHealthBarOcclusionTest`'s helper, same skip rule.
	 *
	 *  ⚠️ LOAD-BEARING HERE FOR THE SAME REASON IT IS THERE: the symbols these probes forbid
	 *  (`SetVisibility`, `CastBarRoot`, `CastBarFill`) are NAMED REPEATEDLY in the shipped comments
	 *  that explain why the C++ half must not touch them. A naive count would read those warnings as
	 *  violations and report a permanent, unfixable red.
	 *  ⚠️ AND THE OTHER DIRECTION IS REAL TOO (`SC-§39`): this skips a whole line that STARTS with a
	 *  comment marker, so a needle sitting after code on a trailing-`//` line still counts. Every
	 *  needle below was chosen after measuring both readings on disk.
	 */
	int32 CountOccurrencesInCode(const FString& Source, const TCHAR* Needle)
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

	/**
	 *  Rebuilds the source with every comment LINE removed, so ORDER can be asserted between two
	 *  symbols. ⛔ Required rather than convenient: `TickComponent`'s comment block names
	 *  `bOccludeHealthBarWhenBlocked` 1 200 characters ABOVE the code line that tests it, so an index
	 *  comparison on the raw text would compare a warning against a statement and read backwards.
	 */
	FString StripCommentLines(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		FString Out;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();
			if (Trimmed.StartsWith(TEXT("//")) || Trimmed.StartsWith(TEXT("*")) || Trimmed.StartsWith(TEXT("/*")))
			{
				continue;
			}
			Out += Line;
			Out += TEXT("\n");
		}
		return Out;
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace — the
	 *  `SiegeHealthBarOcclusionTest` helper. ⛔ Deliberately NOT a parser: a signature that stops
	 *  matching FAILS the test rather than silently scanning an empty string.
	 */
	bool ExtractFunctionBody(FAutomationTestBase& Test, const FString& Source, const TCHAR* Signature, FString& OutBody)
	{
		const int32 SignatureIndex = Source.Find(Signature, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		if (SignatureIndex == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' not found — the probe is STALE, so it FAILS."), Signature));
			return false;
		}

		const int32 BodyEnd = Source.Find(TEXT("\n}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SignatureIndex);
		if (BodyEnd == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find the end of '%s' — the probe is STALE, so it FAILS."), Signature));
			return false;
		}

		OutBody = Source.Mid(SignatureIndex, BodyEnd - SignatureIndex);
		return true;
	}

	/** This component's translation unit and header — the subjects of the structural probes. */
	const TCHAR* const ComponentCppPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp");
	const TCHAR* const ComponentHeaderPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h");

	/** The widget base carrying the ONE C++→widget event `TASK-861` binds by string. */
	const TCHAR* const WidgetHeaderPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarWidget.h");

	/**
	 *  Reflection reader for the two PINNED cast tunables. They are `protected` on the component and
	 *  they stay that way — a cast row's knobs are not public API, and widening them so a test could
	 *  see them would be the shipped-surface-for-a-test mistake this project refuses elsewhere.
	 *  ⚠️ Returns a pointer and every caller null-checks with `AddError`: a renamed or RETYPED field
	 *  makes `FindFProperty` return null, and a test that silently skipped its own assertions on a
	 *  null would report SAFE for ever.
	 */
	const FFloatProperty* FindCastFloatProperty(const TCHAR* FieldName)
	{
		return FindFProperty<FFloatProperty>(UCombatantHealthBarComponent::StaticClass(), FieldName);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  1. ⛔ THE PUSH GATE IS AN `OR`, AND THE SECOND TERM IS THE FALLING EDGE
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarPushGateTest,
	"Siegebound.CastBar.ThePushGateFiresOnTheFallingEdgeAndTheFleetPathIsTheOnlySilentOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarPushGateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ⭐ THE COMPLETE TRUTH TABLE — four inputs, and exactly ONE of them is silent.
	TestTrue(TEXT("(a) casting + row already open ⇒ PUSH (the ordinary in-flight poll)"),
		UCombatantHealthBarComponent::ShouldPushCastRow(/*bCasting=*/ true, /*bRowAlreadyDriven=*/ true));

	TestTrue(TEXT("(b) casting + row down ⇒ PUSH (the RISING edge — a cast just began)"),
		UCombatantHealthBarComponent::ShouldPushCastRow(/*bCasting=*/ true, /*bRowAlreadyDriven=*/ false));

	// ⛔⛔ THE HIGHEST-CONSEQUENCE ROW IN THE FILE. An `if (bCasting)` gate — which is what everyone
	// writes — answers FALSE here, and this is the poll on which a cast ENDS. It is therefore the ONLY
	// poll that can ever report an INTERRUPT, i.e. the counterplay this whole feature exists to make
	// visible. Test 8 replays what dropping it costs.
	TestTrue(TEXT("(c) ⛔⛔ NOT casting + row STILL OPEN ⇒ PUSH — the FALLING EDGE, the only event that ever reports an INTERRUPT"),
		UCombatantHealthBarComponent::ShouldPushCastRow(/*bCasting=*/ false, /*bRowAlreadyDriven=*/ true));

	// ⭐ THE FLEET'S WHOLE-LIFE PATH: every building, the hero and every non-witch unit sits here for
	// the entire match ⇒ ZERO Blueprint calls, ever. This is what makes the feature free.
	TestFalse(TEXT("(d) ⭐ NOT casting + row already down ⇒ SILENT — every building, the hero and all 20+ units, all match long"),
		UCombatantHealthBarComponent::ShouldPushCastRow(/*bCasting=*/ false, /*bRowAlreadyDriven=*/ false));

	// ── ANTI-VACUITY: the gate READS BOTH ARGUMENTS ──────────────────────────────────────────
	// A gate that returned a constant, or that ignored either argument, satisfies some rows above.
	// These two assert that each argument ALONE can flip the answer.
	TestTrue(TEXT("(e) ⭐ ANTI-VACUOUS: holding bRowAlreadyDriven at false, bCasting alone flips the answer — the first argument is read"),
		UCombatantHealthBarComponent::ShouldPushCastRow(true, false) != UCombatantHealthBarComponent::ShouldPushCastRow(false, false));

	TestTrue(TEXT("(f) ⭐ ANTI-VACUOUS: holding bCasting at false, bRowAlreadyDriven alone flips the answer — the falling-edge term is real"),
		UCombatantHealthBarComponent::ShouldPushCastRow(false, true) != UCombatantHealthBarComponent::ShouldPushCastRow(false, false));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  2. ⛔ THE PERCENT THE WIDGET IS ALLOWED TO SEE — 0..100, NEVER 0..1, AND NEVER A STALE FILL
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarSanitizeTest,
	"Siegebound.CastBar.NotCastingIsExactlyZeroAndACastingPercentIsClampedToTheShippedZeroToHundredScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarSanitizeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ── (a) NOT CASTING ⇒ EXACTLY 0, WHATEVER THE PROVIDER SAID ──────────────────────────────
	// ⛔ The gate and the fill travel as ONE atomic event, so "a collapsed row carrying a stale 87%"
	// is a state this makes UNREACHABLE rather than merely unlikely. The 87 is the provider LYING on
	// purpose — a real provider answers 0 here, and this row does not depend on it doing so.
	TestEqual(TEXT("(a) ⛔ not casting ⇒ 0.f even when the provider answers 87 — the atomic event can never carry a stale fill"),
		UCombatantHealthBarComponent::SanitizeCastPercent(/*bCasting=*/ false, 87.f), 0.f, Tolerance);

	TestEqual(TEXT("(a) ⛔ …and 0.f even from a provider answering 100 — 'not casting' outranks every percent"),
		UCombatantHealthBarComponent::SanitizeCastPercent(false, 100.f), 0.f, Tolerance);

	// ── (b) CASTING ⇒ THE VALUE PASSES THROUGH ON THE 0..100 SCALE ───────────────────────────
	TestEqual(TEXT("(b) casting + 41.7 ⇒ 41.7 — a mid-flight percent is passed straight through"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, 41.7f), 41.7f, Tolerance);

	// ⭐⭐ HIGH-§1, THE COST OF GETTING THE SCALE BACKWARDS, IN ARITHMETIC: a 0..1 producer would hand
	// this 0.5 for a HALF-FINISHED cast, and 0.5 survives unchanged — the widget then divides by 100
	// and paints 0.5% of a bar. ⇒ a tell that never visibly moves, on a 3-second window whose entire
	// job is answering "how much LONGER". The surface is NOT rescaled, deliberately: rescaling here
	// would silently accept a wrong producer and make the two conventions indistinguishable.
	TestEqual(TEXT("(b) ⭐⭐ HIGH-§1: 0.5 is NOT rescaled to 50 — the scale is 0..100 and a 0..1 producer would paint 0.5% of a bar for a half-done cast"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, 0.5f), 0.5f, Tolerance);

	// ── (c) ⛔ THE CEILING IS THE GUARANTEE THE TELL CANNOT LIE ──────────────────────────────
	// "The fill reached the ends" must be producible ONLY by a cast that ran its whole window,
	// because that is the SOLE difference between COMPLETED and BROKEN (WITCH-§9.1 row 4).
	TestEqual(TEXT("(c) ⛔ 140 clamps to 100 — 'the fill reached the ends' can never be manufactured by a bad provider"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, 140.f), 100.f, Tolerance);

	TestEqual(TEXT("(c) ⛔ -20 clamps to 0 — a negative fill is not a fill"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, -20.f), 0.f, Tolerance);

	// ── (d) ⚠️ NON-FINITE ⇒ 0, AND `FMath::Clamp` CANNOT DO THIS JOB ────────────────────────
	// Clamp is a pair of `<` / `>` tests and EVERY comparison against NaN is false, so a NaN passes
	// straight through a clamp and into Slate's SetPercent. This is the row that proves the explicit
	// IsFinite branch exists and is reached — delete it and this goes red while (c) stays green.
	const float NaNPercent = MakeQuietNaN();
	TestFalse(TEXT("(d) SELF-CHECK: the NaN this row feeds really is non-finite (a folded constant would test nothing)"),
		FMath::IsFinite(NaNPercent));

	TestEqual(TEXT("(d) ⚠️ NaN ⇒ 0.f — a clamp alone would pass it through, because every comparison against NaN is false"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, NaNPercent), 0.f, Tolerance);

	TestEqual(TEXT("(d) ⚠️ +Inf ⇒ 0.f — the same branch, the other non-finite"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, MakePositiveInfinity()), 0.f, Tolerance);

	// ── (e) ⭐ ANTI-VACUITY: the function is not simply returning 0 ──────────────────────────
	TestTrue(TEXT("(e) ⭐ ANTI-VACUOUS: a casting mid-flight percent and a not-casting one DIFFER — the gate argument is genuinely read"),
		UCombatantHealthBarComponent::SanitizeCastPercent(true, 41.7f) != UCombatantHealthBarComponent::SanitizeCastPercent(false, 41.7f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  3. THE GROWN HEIGHT — WHOLE PIXELS, BECAUSE THE PIVOT IS COMPUTED FROM THIS NUMBER
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarGrownHeightTest,
	"Siegebound.CastBar.TheGrownHeightIsWholePixelsAndANegativeRowCannotShrinkTheBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarGrownHeightTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ── (a) THE SHIPPED BUDGET: 22 + 8 = 30 ─────────────────────────────────────────────────
	//   · NOT casting: 22 px, CastBarRoot Collapsed ⇒ its slot is SKIPPED ⇒ (22 - 1) split Fill 1:2
	//     ⇒ BoostOutline 7.00 · Bar 14.00 — byte-identical to today.
	//   · CASTING:     30 px ⇒ (30 - 1 - 1) split Fill 1:1:2 ⇒ CastBarRoot 7.00 · BoostOutline 7.00 ·
	//     Bar 14.00. ⭐ BoostOutline and Bar keep their EXACT current heights in BOTH states.
	TestEqual(TEXT("(a) 22 px base + 8 px row = 30 px while a cast runs — the TASK-861 budget"),
		UCombatantHealthBarComponent::ComputeCastBarHeightPixels(22.f, 8.f), 30.f, Tolerance);

	// ── (b) ⛔ A NEGATIVE ROW HEIGHT IS FLOORED, NOT OBEYED ──────────────────────────────────
	// CastBarRowHeightPixels is EditDefaultsOnly, so a Blueprint can type -8. Obeying it would make
	// the health bar SHRINK when a witch starts channelling, which is worse than no tell at all.
	TestEqual(TEXT("(b) ⛔ a -8 px row floors to 0 ⇒ the bar keeps its shipped height rather than SHRINKING at cast time"),
		UCombatantHealthBarComponent::ComputeCastBarHeightPixels(22.f, -8.f), 22.f, Tolerance);

	// ── (c) ⛔ WHOLE PIXELS, AND IT MATTERS BECAUSE THE PIVOT IS DERIVED FROM THIS ───────────
	// `SetDrawSize` TRUNCATES into an FIntPoint (`FIntPoint((int32)Size.X, (int32)Size.Y)`), so an
	// un-rounded 30.4 would be LAID OUT at 30 while ComputeCastPivotY compensated for 30.4 — a
	// half-pixel disagreement between the two halves of ONE geometry change, i.e. a bar that creeps.
	TestEqual(TEXT("(c) ⛔ 22 + 8.4 rounds to 30 — SetDrawSize truncates, so an un-rounded height and its pivot would disagree"),
		UCombatantHealthBarComponent::ComputeCastBarHeightPixels(22.f, 8.4f), 30.f, Tolerance);

	TestEqual(TEXT("(c) ⛔ 22 + 8.6 rounds to 31 — rounding, not truncation: the height is the honest one and the pivot follows it"),
		UCombatantHealthBarComponent::ComputeCastBarHeightPixels(22.f, 8.6f), 31.f, Tolerance);

	// ── (d) ⭐ ANTI-VACUITY: the row height is actually added ────────────────────────────────
	TestTrue(TEXT("(d) ⭐ ANTI-VACUOUS: a 0 row and an 8 px row give DIFFERENT heights — the second argument is read, not ignored"),
		UCombatantHealthBarComponent::ComputeCastBarHeightPixels(22.f, 0.f) != UCombatantHealthBarComponent::ComputeCastBarHeightPixels(22.f, 8.f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  4. ⭐⭐ THE PIXEL-IDENTICAL GUARANTEE AS ARITHMETIC — THE BAR GROWS **UPWARD**, INTO THE SKY
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarPivotTest,
	"Siegebound.CastBar.TheGrownBarHoldsItsBottomEdgeStillSoTheHealthRowNeverMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarPivotTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ⛔ THE PROBLEM THIS SOLVES IS INVISIBLE FROM THE CALL SITE. The component never set Pivot, so it
	// is the engine default (0.5, 0.5) — the widget is CENTRED on its projected anchor. Growing
	// DrawSize by 8 px about a centred pivot grows 4 px UP **and 4 px DOWN**, dropping the health bar
	// 4 px INTO THE UNIT'S HEAD for the whole cast. `TASK-861` measured that the component never sets
	// Pivot; this is the arithmetic that answers it.
	const float BaseHeight = 22.f;
	const float BasePivotY = 0.5f;
	const float GrownHeight = 30.f;

	const float GrownPivotY = UCombatantHealthBarComponent::ComputeCastPivotY(BaseHeight, BasePivotY, GrownHeight);

	// ── (a) ⭐⭐ THE WHOLE CONTRACT, IN ONE LINE: THE BOTTOM EDGE DOES NOT MOVE ───────────────
	// A canvas alignment A offsets a box of height H by -A*H, so the bottom edge sits (1 - A) * H
	// BELOW the anchor. Holding that product constant is the entire mechanism.
	const float BaseBottomOffset = (1.f - BasePivotY) * BaseHeight;
	const float GrownBottomOffset = (1.f - GrownPivotY) * GrownHeight;

	TestEqual(*FString::Printf(TEXT("(a) ⭐⭐ the bottom edge is UNMOVED: base (1-%.4f)*%.1f = %.4f px vs grown (1-%.4f)*%.1f = %.4f px"),
			BasePivotY, BaseHeight, BaseBottomOffset, GrownPivotY, GrownHeight, GrownBottomOffset),
		GrownBottomOffset, BaseBottomOffset, Tolerance);

	// ── (b) ⇒ THE +8 px IS SPENT ENTIRELY UPWARD ────────────────────────────────────────────
	// Top edge = anchor - A*H. The grown box's top must sit exactly 8 px higher than the base box's,
	// which is the only direction with room for it.
	const float BaseTopOffset = BasePivotY * BaseHeight;
	const float GrownTopOffset = GrownPivotY * GrownHeight;

	TestEqual(*FString::Printf(TEXT("(b) ⭐ the growth is ENTIRELY UPWARD: the top edge rises by %.4f px, which is the whole 8 px (0 px into the unit's head)"),
			GrownTopOffset - BaseTopOffset),
		GrownTopOffset - BaseTopOffset, GrownHeight - BaseHeight, Tolerance);

	// ── (c) THE NUMBER ITSELF, SO A CHANGE IS VISIBLE IN THE DIFF ───────────────────────────
	// 1 - ((1 - 0.5) * 22) / 30 = 1 - 11/30 = 0.633333…
	TestEqual(TEXT("(c) the shipped pivot for a 22 → 30 px grow is 0.63333 (1 - 11/30)"),
		GrownPivotY, 1.f - (11.f / 30.f), Tolerance);

	// ── (d) ⛔ THE FLEET-WIDE ALTERNATIVE THIS REFUSES, STATED AS ARITHMETIC ─────────────────
	// The obvious fix is Pivot=(0.5,1.0) in the CONSTRUCTOR. That would move EVERY bar in the game up
	// by half its height, permanently — 11 px on a 22 px bar, on every building, the hero and all 20+
	// units, casting or not. This row records the size of the regression that was refused.
	const float ConstructorPivotShiftPixels = (1.f - BasePivotY) * BaseHeight;
	TestEqual(TEXT("(d) ⛔ REFUSED ALTERNATIVE: a constructor Pivot=(0.5,1.0) would shift every bar in the game up 11 px, permanently"),
		ConstructorPivotShiftPixels, 11.f, Tolerance);

	// ── (e) DEGENERATE INPUT RETURNS THE BASE RATHER THAN DIVIDING BY ZERO ──────────────────
	TestEqual(TEXT("(e) a non-positive grown height returns the base pivot unchanged — no division by zero"),
		UCombatantHealthBarComponent::ComputeCastPivotY(22.f, 0.5f, 0.f), 0.5f, Tolerance);

	// ── (f) ⭐ ANTI-VACUITY: the pivot really did move ───────────────────────────────────────
	// A function that returned BasePivotY unchanged would satisfy (e) and would fail (a) only because
	// (a) is doing real work. Assert the change explicitly so nobody has to infer it.
	TestTrue(*FString::Printf(TEXT("(f) ⭐ ANTI-VACUOUS: the pivot MOVED (%.5f → %.5f); an implementation returning the base pivot would leave the bar centred and drop it into the head"),
			BasePivotY, GrownPivotY),
		!FMath::IsNearlyEqual(GrownPivotY, BasePivotY, Tolerance));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  5. ⛔⛔ THE FLEET-WIDE REGRESSION GUARD — THE SHIPPED DEFAULTS, READ OFF THE **CDO**
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarShippedDefaultsTest,
	"Siegebound.CastBar.TheConstructorDefaultDrawSizeAndPivotAreUntouchedSoEveryNonCastingActorIsPixelIdentical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarShippedDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ⭐⭐ THE FIRST TEST TO READ IN THIS FILE, AND THE ONLY ONE THAT IS A **RUNTIME** READ OF THE
	// SHIPPED STATE. `TASK-860`'s spec item (4) says "DrawSize grows to fit the new segment", and
	// `TASK-861` measured what the LITERAL reading costs: `BoostOutline` and `Bar` are `Fill` slots
	// and absorb every spare pixel, so a CONSTRUCTOR bump to (90, 30) renders `Bar` at 19.33 px
	// instead of 14.00 on EVERY BUILDING, THE HERO AND ALL 20+ UNITS — casting or not. A permanent,
	// silent, game-wide health-bar resize shipped by a witch task.
	//
	// ⇒ this row goes red the moment anyone "simplifies" the geometry back into the constructor, and
	// it does so by CALLING the shipped accessors on the class default object rather than by reading
	// the source text. The CDO has run the constructor; what it reports IS what every actor gets.
	const UCombatantHealthBarComponent* const DefaultComponent = GetDefault<UCombatantHealthBarComponent>();
	if (DefaultComponent == nullptr)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: GetDefault<UCombatantHealthBarComponent>() returned null — every assertion below would have been vacuous."));
		return false;
	}

	// ⚠️ NARROWED TO `float` EXPLICITLY, AND IT IS ⛔ NOT COSMETIC: under Large World Coordinates
	// `FVector2D` is `UE::Math::TVector2<double>`, so `.X`/`.Y` are DOUBLES. Handing a double actual
	// to `TestEqual(const TCHAR*, float, float, float)` alongside float literals makes the float and
	// double overloads AMBIGUOUS — each is a better match for a different argument — which is a hard
	// compile error rather than a warning. The shipped seams all take floats, so this is also the
	// exact conversion `ApplyCastRowGeometry` performs at the call site.
	const FVector2D DefaultDrawSize = DefaultComponent->GetDrawSize();
	const FVector2D DefaultPivot = DefaultComponent->GetPivot();

	const float DefaultDrawWidth = static_cast<float>(DefaultDrawSize.X);
	const float DefaultDrawHeight = static_cast<float>(DefaultDrawSize.Y);
	const float DefaultPivotX = static_cast<float>(DefaultPivot.X);
	const float DefaultPivotY = static_cast<float>(DefaultPivot.Y);

	// ── (a) ⛔ THE SHIPPED DrawSize IS STILL (90, 22) ────────────────────────────────────────
	TestEqual(*FString::Printf(TEXT("(a) ⛔⛔ the default DrawSize.Y is %.2f px — it MUST be 22: a constructor bump to 30 renders Bar at 19.33 px instead of 14.00 on every actor in the game"),
			DefaultDrawHeight),
		DefaultDrawHeight, 22.f, Tolerance);

	TestEqual(*FString::Printf(TEXT("(a) the default DrawSize.X is %.2f px — the cast row changes the HEIGHT only; the width is not this feature's business"),
			DefaultDrawWidth),
		DefaultDrawWidth, 90.f, Tolerance);

	// ── (b) ⛔ AND THE PIVOT IS STILL THE ENGINE DEFAULT ─────────────────────────────────────
	// ⭐ THIS IS THE OTHER HALF OF THE SAME GUARD, and it is the one nobody expects: moving Pivot to
	// (0.5, 1.0) in the constructor is the OBVIOUS fix for the 4-px head-drop, and it would shift
	// every bar in the game up 11 px permanently (test 4d). The pivot must be recomputed WITH the
	// size, at cast time — never installed once.
	TestEqual(*FString::Printf(TEXT("(b) ⛔ the default Pivot.Y is %.4f — it MUST stay the engine default 0.5: a constructor Pivot=(0.5,1.0) moves every bar in the game up 11 px, permanently"),
			DefaultPivotY),
		DefaultPivotY, 0.5f, Tolerance);

	TestEqual(TEXT("(b) the default Pivot.X is 0.5 — horizontal centring is untouched by the cast row"),
		DefaultPivotX, 0.5f, Tolerance);

	// ── (c) ⭐ AND THE BUDGET IS DERIVED FROM WHAT WAS JUST MEASURED, NOT FROM A LITERAL ──────
	// Feeding the CDO's own numbers into the shipped seams is what makes test 3's arithmetic a
	// statement about THE SHIPPED BAR rather than about two constants typed into a test.
	const FFloatProperty* const RowHeightProperty = FindCastFloatProperty(TEXT("CastBarRowHeightPixels"));
	if (RowHeightProperty == nullptr)
	{
		AddError(TEXT("⛔ 'CastBarRowHeightPixels' was not found as an FFloatProperty — renamed or retyped. The budget below cannot be derived, so this FAILS rather than skipping."));
		return false;
	}

	const float ShippedRowHeight = RowHeightProperty->GetPropertyValue_InContainer(DefaultComponent);
	TestEqual(TEXT("(c) the shipped cast row is 8 px — one BoostOutline-shaped row (7.00 px) plus its 1 px bottom pad, so both states land on whole pixels"),
		ShippedRowHeight, 8.f, Tolerance);

	const float GrownHeight = UCombatantHealthBarComponent::ComputeCastBarHeightPixels(DefaultDrawHeight, ShippedRowHeight);
	TestEqual(*FString::Printf(TEXT("(c) ⭐ the SHIPPED bar grows %.0f → %.0f px at cast time, derived from the CDO's own defaults rather than from literals"),
			DefaultDrawHeight, GrownHeight),
		GrownHeight, 30.f, Tolerance);

	// ── (d) ⭐⭐ AND THE HEALTH ROW DOES NOT MOVE, ON THE SHIPPED NUMBERS ─────────────────────
	const float GrownPivotY = UCombatantHealthBarComponent::ComputeCastPivotY(DefaultDrawHeight, DefaultPivotY, GrownHeight);
	TestEqual(TEXT("(d) ⭐⭐ WITCH-§9.6 on the shipped numbers: the bottom edge of the GROWN bar sits exactly where the shipped bar's does"),
		(1.f - GrownPivotY) * GrownHeight, (1.f - DefaultPivotY) * DefaultDrawHeight, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  6. ⛔ NOT CASTING IS BYTE-IDENTICAL — THE GEOMETRY IS RESTORED TO THE **CAPTURED** BASE
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarGeometryRestoreTest,
	"Siegebound.CastBar.TheGeometryIsDerivedFromTheCapturedBaseSoTheBarCannotCreepAndIsRestoredExactly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarGeometryRestoreTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	FString ComponentSource;
	if (!LoadProjectSource(*this, ComponentCppPath, ComponentSource))
	{
		return false;
	}

	FString GeometryBody;
	if (!ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::ApplyCastRowGeometry(bool bCasting)"), GeometryBody))
	{
		return false;
	}

	// ── POSITIVE CONTROL: the extraction is substantial ──────────────────────────────────────
	TestTrue(*FString::Printf(TEXT("SELF-CHECK: the extracted ApplyCastRowGeometry body is %d chars — an empty extraction would make every count below a vacuous zero"),
			GeometryBody.Len()),
		GeometryBody.Len() > 200);

	// ── (a) ⛔⛔ THE GROWN HEIGHT IS DERIVED FROM THE **CAPTURED BASE**, NEVER FROM THE LIVE SIZE ──
	// ⭐ THIS IS THE DEFECT NOBODY WOULD SEE IN REVIEW: reading `GetDrawSize().Y` here instead of
	// `CastBarBaseDrawSize.Y` compiles, reads perfectly sensibly, and passes tests 3, 4 and 5 — and
	// then adds 8 px to the CURRENT height on every edge, so a unit that casts twice ends the match
	// with a bar 16 px too tall. The base is captured ONCE at BeginPlay precisely so this cannot happen.
	TestEqual(TEXT("(a) ⛔⛔ the grown height is computed from CastBarBaseDrawSize.Y — from the CAPTURED base, so repeated casts cannot make the bar CREEP upward"),
		CountOccurrencesInCode(GeometryBody, TEXT("ComputeCastBarHeightPixels(CastBarBaseDrawSize.Y")), 1);

	TestEqual(TEXT("(a) ⛔ …and NOT from the live GetDrawSize() — that would add 8 px to the CURRENT height on every single cast"),
		CountOccurrencesInCode(GeometryBody, TEXT("ComputeCastBarHeightPixels(GetDrawSize")), 0);

	// ── (b) ⛔ THE NOT-CASTING BRANCH RESTORES THE CAPTURED PAIR EXACTLY ─────────────────────
	// Restoring to the C++ CONSTANT rather than to the captured values would silently delete a
	// Blueprint's legitimate DrawSize/Pivot override the first time that actor finished a cast.
	TestEqual(TEXT("(b) ⛔ not casting ⇒ DrawSize is restored to CastBarBaseDrawSize — this actor's OWN size, not the C++ default (a BP override survives a cast)"),
		CountOccurrencesInCode(GeometryBody, TEXT(": CastBarBaseDrawSize;")), 1);

	TestEqual(TEXT("(b) ⛔ not casting ⇒ Pivot is restored to CastBarBasePivot — the same rule for the other half of the one geometry change"),
		CountOccurrencesInCode(GeometryBody, TEXT(": CastBarBasePivot);")), 1);

	// ── (c) ⛔⛔ THE TWO WRITES ARE ONE CHANGE AND BOTH LIVE HERE ────────────────────────────
	// Applying either alone moves the health bar: size alone grows 4 px into the unit's head, pivot
	// alone shifts the bar without resizing it. They are written together, on the cast's two edges.
	TestEqual(TEXT("(c) ⛔ ApplyCastRowGeometry writes DrawSize exactly once"),
		CountOccurrencesInCode(GeometryBody, TEXT("SetDrawSize(")), 1);

	TestEqual(TEXT("(c) ⛔ …and Pivot exactly once, in the SAME function — applying either alone moves the health bar"),
		CountOccurrencesInCode(GeometryBody, TEXT("SetPivot(")), 1);

	// ── (d) ⛔ SELF-GATING: "twice per cast, never per poll" ─────────────────────────────────
	// The ~59 polls between the two edges all land here and leave immediately. Without the early-out
	// the pair would be written 20×/second on a casting unit for no benefit.
	TestTrue(TEXT("(d) ⛔ the geometry self-gates on the size already being right — the two writes land on the cast's EDGES, never on the ~59 polls between them"),
		CountOccurrencesInCode(GeometryBody, TEXT("GetDrawSize().Equals(TargetDrawSize)")) == 1);

	// ── (e) ⛔ THE CONSTRUCTOR IS CLEAN — the fleet-wide guard, from the source side ──────────
	// Test 5 proves the shipped defaults by CALLING the CDO. This proves the same thing structurally,
	// which is the reading that names the failure: no cast symbol may appear in the constructor at all.
	FString ConstructorBody;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("UCombatantHealthBarComponent::UCombatantHealthBarComponent()"), ConstructorBody))
	{
		TestEqual(TEXT("(e) ⛔ the constructor names 'Cast' ZERO times — the geometry may never migrate there (a constructor bump is the fleet-wide regression)"),
			CountOccurrencesInCode(ConstructorBody, TEXT("CastBar")), 0);

		TestEqual(TEXT("(e) ⛔ …and it calls SetPivot ZERO times — the pivot is recomputed WITH the size at cast time, never installed once"),
			CountOccurrencesInCode(ConstructorBody, TEXT("SetPivot(")), 0);

		// POSITIVE CONTROL: the constructor probe is pointed at a real constructor that really does
		// set the size — otherwise the two zeros above would be a statement about an empty string.
		TestEqual(TEXT("(e) SELF-CHECK: the constructor DOES set the shipped DrawSize once — the two zeros above are about a live function"),
			CountOccurrencesInCode(ConstructorBody, TEXT("SetDrawSize(CombatantHealthBarDrawSize)")), 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  7. ⭐⭐ THE ONE THAT MATTERS — A **BROKEN** CAST AND A **COMPLETED** CAST PRODUCE DIFFERENT
//     EVENT SEQUENCES
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarBrokenVsCompletedTest,
	"Siegebound.CastBar.ABrokenCastAndACompletedCastProduceDifferentEventSequences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarBrokenVsCompletedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ⭐⭐ THE DELIVERABLE. `WITCH-§9.1` row 4 is the perception with NO tell at all in the shipped
	// game: COMPLETION already has one (the target turns translucent, for the OWNER only) and BREAK
	// has none, for anyone — so "nothing happened" is exactly indistinguishable from "still casting"
	// and the player cannot tell whether their own interrupt LANDED.
	//
	// ⇒ the question this row asks is the one `SHIP-§9` makes the feature answer: can the suite tell
	// the two apart? It is a question about a SEQUENCE OVER TIME, so both are replayed in full.

	const TArray<FCastSample> CompletedSamples = MakeCastRun(ShippedCastSeconds, ShippedPollSeconds, /*InterruptAtSeconds=*/ ShippedCastSeconds);
	const TArray<FCastSample> BrokenSamples = MakeCastRun(ShippedCastSeconds, ShippedPollSeconds, /*InterruptAtSeconds=*/ 1.25f);

	const TArray<FCastEvent> Completed = ReplayCastRun(CompletedSamples, /*bUseShippedGate=*/ true);
	const TArray<FCastEvent> Broken = ReplayCastRun(BrokenSamples, /*bUseShippedGate=*/ true);

	// ── POSITIVE CONTROL: both runs actually happened ────────────────────────────────────────
	if (Completed.Num() < 2 || Broken.Num() < 2)
	{
		AddError(*FString::Printf(TEXT("⛔ SELF-CHECK FAILED: the replay produced %d completed and %d broken events. A run that never started would make every comparison below vacuously equal."),
			Completed.Num(), Broken.Num()));
		return false;
	}

	const float CompletedPeak = PeakCastPercent(Completed);
	const float BrokenPeak = PeakCastPercent(Broken);

	// ── (a) ⭐⭐ THE DISCRIMINATOR: THE TWO SEQUENCES DIFFER, AND BY A LOT ────────────────────
	// This is the assertion the whole task is gated on. If it ever passes trivially — e.g. because
	// both runs collapsed to one event — the self-check above has already failed the test.
	TestTrue(*FString::Printf(TEXT("(a) ⭐⭐ COMPLETED peaks at %.2f%% and BROKEN at %.2f%% — a %.2f-point gap. If these were equal the suite would have reproduced, in code, the exact defect this feature removes"),
			CompletedPeak, BrokenPeak, CompletedPeak - BrokenPeak),
		(CompletedPeak - BrokenPeak) > 50.f);

	// ── (b) A COMPLETED CAST RUNS ITS WHOLE WINDOW ───────────────────────────────────────────
	// 100 * (3.00 - 0.05) / 3.00 = 98.333% — the last value ANY poll can observe, because the timer's
	// callback clears the handle (test 9 is the arithmetic and the pixel consequence).
	TestEqual(*FString::Printf(TEXT("(b) COMPLETED reaches %.3f%% — 100 * (Duration - Interval) / Duration, the last observable value on a cast that ran its window"),
			CompletedPeak),
		CompletedPeak, 100.f * (ShippedCastSeconds - ShippedPollSeconds) / ShippedCastSeconds, 1.e-2f);

	// ── (c) A BROKEN CAST STOPS WHERE THE INTERRUPT LANDED ──────────────────────────────────
	// Interrupted at 1.25 s of a 3 s cast; the last poll before that is 1.20 s ⇒ 40.0%.
	TestEqual(*FString::Printf(TEXT("(c) BROKEN stops at %.3f%% — the interrupt landed at 1.25 s of a 3 s cast and the bar never got past 40%%"),
			BrokenPeak),
		BrokenPeak, 40.f, 1.e-2f);

	// ── (d) ⛔ BOTH END WITH EXACTLY ONE TERMINAL EVENT, AND IT IS THE LAST ──────────────────
	// ⭐ The row must come DOWN both ways. The terminal event is byte-identical for the two endings —
	// which is correct and is the point: what distinguishes them is WHERE the fill had got to, not
	// how it ended. A distinct "interrupted" signal would be a second source of truth.
	TestEqual(TEXT("(d) ⛔ COMPLETED tells the row to come down EXACTLY once — never zero (frozen bar) and never twice (a flicker)"),
		CountRowDownEvents(Completed), 1);

	TestEqual(TEXT("(d) ⛔ BROKEN tells the row to come down EXACTLY once — the interrupt is reported, and reported once"),
		CountRowDownEvents(Broken), 1);

	TestFalse(TEXT("(d) COMPLETED's LAST event carries bCasting == false — the row is left CLOSED, and DrawSize restored with it"),
		Completed.Last().bCasting);

	TestFalse(TEXT("(d) BROKEN's LAST event carries bCasting == false — same teardown, one line, both endings"),
		Broken.Last().bCasting);

	TestEqual(TEXT("(d) ⛔ COMPLETED's terminal event carries EXACTLY 0.f — a collapsed row can never keep a stale 98%"),
		Completed.Last().CastPercent, 0.f, Tolerance);

	TestEqual(TEXT("(d) ⛔ BROKEN's terminal event carries EXACTLY 0.f — a collapsed row can never keep a stale 40%"),
		Broken.Last().CastPercent, 0.f, Tolerance);

	// ── (e) THE FILL IS MONOTONIC WHILE THE CAST RUNS ───────────────────────────────────────
	// A bar that jumps backwards mid-cast reads as a second cast starting. Derived per call from one
	// live timer, so it cannot — asserted rather than assumed.
	float PreviousPercent = -1.f;
	bool bMonotonic = true;
	for (int32 EventIndex = 0; EventIndex < Completed.Num() - 1; ++EventIndex)
	{
		if (Completed[EventIndex].CastPercent < PreviousPercent)
		{
			bMonotonic = false;
			break;
		}
		PreviousPercent = Completed[EventIndex].CastPercent;
	}
	TestTrue(TEXT("(e) the fill only ever advances while the cast runs — a bar that jumped backwards would read as a SECOND cast beginning"),
		bMonotonic);

	// ── (f) ⭐ AND THE LENGTHS DIFFER TOO — a second, independent witness ────────────────────
	// 59 live polls + 1 terminal vs 24 + 1. A player watching sees the shorter bar; the suite sees
	// the shorter sequence. Two different facts, one event stream.
	TestEqual(*FString::Printf(TEXT("(f) ⭐ COMPLETED emits %d events and BROKEN %d — the interrupt is visible in the LENGTH as well as in the peak"),
			Completed.Num(), Broken.Num()),
		Completed.Num(), 60);

	TestEqual(TEXT("(f) ⭐ …and BROKEN emits 25 — 24 live polls over 1.20 s plus the one that reports the interrupt"),
		Broken.Num(), 25);

	// ── (g) ⛔ NO EVENT EVER CARRIES A PERCENT WITH THE ROW CLOSED ───────────────────────────
	// The atomicity guarantee, asserted over the whole stream rather than at one sample.
	bool bAtomic = true;
	for (const FCastEvent& Event : Completed)
	{
		if (!Event.bCasting && Event.CastPercent != 0.f)
		{
			bAtomic = false;
			break;
		}
	}
	for (const FCastEvent& Event : Broken)
	{
		if (!Event.bCasting && Event.CastPercent != 0.f)
		{
			bAtomic = false;
			break;
		}
	}
	TestTrue(TEXT("(g) ⛔ across BOTH whole runs, no event pairs 'row closed' with a non-zero fill — the gate and the fill travel as ONE atomic event"),
		bAtomic);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  8. ⭐⭐ `SHIP-§9` — THE GATE IS VALIDATED AGAINST THE **FAILURE** IT DETECTS
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarFallingEdgeFailureTest,
	"Siegebound.CastBar.DroppingTheFallingEdgeStrandsTheRowOpenForeverAndTheSuiteCatchesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarFallingEdgeFailureTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ⭐⭐ `SHIP-§9`: validate a gate against the FAILURE it detects, never merely against success.
	// Test 7 proves the shipped composition tells the two endings apart. ⛔ It would ALSO pass on an
	// implementation gated `if (bCasting)` — the version everyone writes — because that version still
	// produces a high peak for a completed cast and a low one for a broken one. What it stops doing
	// is EVER TELLING THE ROW TO COME DOWN.
	//
	// ⇒ this row replays the SAME two casts through that wrong gate and asserts the suite would go
	// red. ⚠️ The `false` argument below is a MODEL of a wrong implementation, ⛔ not a switch that
	// exists in shipping code.

	const TArray<FCastSample> CompletedSamples = MakeCastRun(ShippedCastSeconds, ShippedPollSeconds, ShippedCastSeconds);
	const TArray<FCastSample> BrokenSamples = MakeCastRun(ShippedCastSeconds, ShippedPollSeconds, 1.25f);

	const TArray<FCastEvent> ShippedCompleted = ReplayCastRun(CompletedSamples, /*bUseShippedGate=*/ true);
	const TArray<FCastEvent> ShippedBroken = ReplayCastRun(BrokenSamples, /*bUseShippedGate=*/ true);

	const TArray<FCastEvent> BuggyCompleted = ReplayCastRun(CompletedSamples, /*bUseShippedGate=*/ false);
	const TArray<FCastEvent> BuggyBroken = ReplayCastRun(BrokenSamples, /*bUseShippedGate=*/ false);

	if (BuggyCompleted.Num() == 0 || BuggyBroken.Num() == 0)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: the modelled-bug replay produced no events at all, so 'the terminal event is missing' would be true for the wrong reason."));
		return false;
	}

	// ── (a) ⛔⛔ THE ROW IS NEVER TOLD TO COME DOWN ──────────────────────────────────────────
	TestEqual(TEXT("(a) ⛔⛔ under `if (bCasting)` a COMPLETED cast tells the row to come down ZERO times — the bar is frozen at its last fill for the rest of the match"),
		CountRowDownEvents(BuggyCompleted), 0);

	TestEqual(TEXT("(a) ⛔⛔ …and a BROKEN cast likewise — the interrupt the player just landed is NEVER reported"),
		CountRowDownEvents(BuggyBroken), 0);

	// ── (b) ⇒ THE LAST THING THE WIDGET EVER HEARS IS A MID-FLIGHT FILL ─────────────────────
	TestTrue(*FString::Printf(TEXT("(b) ⛔ the buggy COMPLETED run ends on (%.2f%%, casting=true) — a cast bar frozen mid-flight, on a unit doing nothing, with DrawSize left 8 px too tall"),
			BuggyCompleted.Last().CastPercent),
		BuggyCompleted.Last().bCasting);

	TestTrue(*FString::Printf(TEXT("(b) ⛔ the buggy BROKEN run ends on (%.2f%%, casting=true) — the player interrupted the cast and the bar still says it is running"),
			BuggyBroken.Last().CastPercent),
		BuggyBroken.Last().bCasting);

	// ── (c) ⭐ THE SHIPPED GATE EMITS EXACTLY ONE MORE EVENT — AND IT IS THE ONE THAT MATTERS ─
	TestEqual(*FString::Printf(TEXT("(c) ⭐ shipped COMPLETED emits %d events, the buggy one %d — the whole difference is the single terminal event"),
			ShippedCompleted.Num(), BuggyCompleted.Num()),
		ShippedCompleted.Num() - BuggyCompleted.Num(), 1);

	TestEqual(TEXT("(c) ⭐ …and the same single event separates shipped BROKEN from buggy BROKEN"),
		ShippedBroken.Num() - BuggyBroken.Num(), 1);

	// ── (d) ⭐⭐ THE SHIPPED VERSION DOES THE THING THE BUGGY ONE CANNOT ─────────────────────
	// The positive half of the same claim, so this row cannot pass by both implementations being
	// broken in the same direction.
	TestEqual(TEXT("(d) ⭐⭐ the SHIPPED gate reports the ending exactly once in both runs — which is the entire behavioural difference this test exists to defend"),
		CountRowDownEvents(ShippedCompleted) + CountRowDownEvents(ShippedBroken), 2);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  9. ⛔ THE POLL PERIOD IS LOAD-BEARING FOR "COMPLETED vs BROKEN", NOT A SMOOTHNESS KNOB
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarPollPeriodTest,
	"Siegebound.CastBar.TheCastRowHasItsOwnFasterPeriodBecauseTheOcclusionOneWouldMakeCompletedLookBroken",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarPollPeriodTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ⭐ THE ARITHMETIC THAT MAKES 0.05 A REQUIREMENT RATHER THAN A PREFERENCE. The unit derives the
	// percent from the LIVE timer and the timer's callback CLEARS the handle, so the last value the
	// bar can observe on a COMPLETED cast is the one sampled ONE POLL before the end. At the occlusion
	// cull's 0.15 s that is 95.0% — which is directly confusable with an interrupt landing at 95%.
	const UCombatantHealthBarComponent* const DefaultComponent = GetDefault<UCombatantHealthBarComponent>();
	if (DefaultComponent == nullptr)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: GetDefault<UCombatantHealthBarComponent>() returned null."));
		return false;
	}

	// ── (a) THE SHIPPED DEFAULT IS 0.05, READ BY REFLECTION ─────────────────────────────────
	const FFloatProperty* const IntervalProperty = FindCastFloatProperty(TEXT("CastProgressPollIntervalSeconds"));
	if (IntervalProperty == nullptr)
	{
		AddError(TEXT("⛔ 'CastProgressPollIntervalSeconds' was not found as an FFloatProperty — renamed or retyped. The probe FAILS rather than skipping its own assertions."));
		return false;
	}

	const float ShippedInterval = IntervalProperty->GetPropertyValue_InContainer(DefaultComponent);
	TestEqual(*FString::Printf(TEXT("(a) the shipped cast poll period is %.4f s (20 reads/second/actor) — and it is NOT the occlusion cull's 0.15 s"),
			ShippedInterval),
		ShippedInterval, ShippedPollSeconds, Tolerance);

	// ── (b) ⭐ THE CONSEQUENCE, MEASURED BY REPLAYING THE SAME COMPLETED CAST AT BOTH PERIODS ─
	const TArray<FCastEvent> FastCompleted = ReplayCastRun(
		MakeCastRun(ShippedCastSeconds, ShippedInterval, ShippedCastSeconds), /*bUseShippedGate=*/ true);
	const TArray<FCastEvent> SlowCompleted = ReplayCastRun(
		MakeCastRun(ShippedCastSeconds, OcclusionPollSeconds, ShippedCastSeconds), /*bUseShippedGate=*/ true);

	const float FastPeak = PeakCastPercent(FastCompleted);
	const float SlowPeak = PeakCastPercent(SlowCompleted);

	TestEqual(*FString::Printf(TEXT("(b) at the shipped 0.05 s a COMPLETED cast peaks at %.3f%%"), FastPeak),
		FastPeak, 98.333f, 1.e-2f);

	TestEqual(*FString::Printf(TEXT("(b) at the occlusion cull's 0.15 s the SAME cast peaks at only %.3f%%"), SlowPeak),
		SlowPeak, 95.f, 1.e-2f);

	// ── (c) ⛔ AND IN PIXELS, WHICH IS THE UNIT THE PLAYER ACTUALLY PERCEIVES ────────────────
	const float FastShortfallPixels = (100.f - FastPeak) * 0.01f * BarFillWidthPixels;
	const float SlowShortfallPixels = (100.f - SlowPeak) * 0.01f * BarFillWidthPixels;

	TestTrue(*FString::Printf(TEXT("(c) at 0.05 s the completed bar falls %.2f px short of full on an %.0f px fill — invisible"),
			FastShortfallPixels, BarFillWidthPixels),
		FastShortfallPixels < 2.f);

	TestTrue(*FString::Printf(TEXT("(c) ⛔ at 0.15 s it falls %.2f px short — VISIBLE, and directly confusable with an interrupt landing at 95%%"),
			SlowShortfallPixels),
		SlowShortfallPixels > 4.f);

	TestTrue(*FString::Printf(TEXT("(c) ⭐ the slower period is %.1f× worse — which is why the cast row does NOT inherit the cull's accumulator or its period"),
			SlowShortfallPixels / FastShortfallPixels),
		(SlowShortfallPixels / FastShortfallPixels) > 2.5f);

	// ── (d) ⭐ ANTI-VACUITY: both replays actually ran, and they DIFFER ──────────────────────
	TestTrue(TEXT("(d) ⭐ ANTI-VACUOUS: the two periods produce different peaks — a replay that ignored its interval would give one number twice"),
		!FMath::IsNearlyEqual(FastPeak, SlowPeak, 1.f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  10. THE CAST POLL GATE — ITS OWN ACCUMULATOR, THE SHARED 30 Hz FLOOR, AND NO CATCH-UP
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarPollGateTest,
	"Siegebound.CastBar.ThePollGateFiresOnPeriodFloorsAtThirtyHertzAndNeverBurstsAfterAHitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarPollGateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	// ── (a) IT FIRES, AND IT FIRES ON PERIOD ────────────────────────────────────────────────
	// One second of 60 fps frames at a 0.05 s period ⇒ ~20 fires. ⭐ "More than zero" is asserted
	// explicitly: "never polls" trivially satisfies "does not poll every frame" while making the
	// whole feature inert, and would leave every other row in this file green forever.
	{
		float Accumulator = 0.f;
		int32 FireCount = 0;
		int32 FirstFireFrame = INDEX_NONE;
		int32 SecondFireFrame = INDEX_NONE;

		for (int32 FrameIndex = 0; FrameIndex < 60; ++FrameIndex)
		{
			if (UCombatantHealthBarComponent::ShouldPollCastProgress(Accumulator, FrameSeconds, ShippedPollSeconds))
			{
				++FireCount;
				if (FirstFireFrame == INDEX_NONE)
				{
					FirstFireFrame = FrameIndex;
				}
				else if (SecondFireFrame == INDEX_NONE)
				{
					SecondFireFrame = FrameIndex;
				}
			}
		}

		TestTrue(*FString::Printf(TEXT("(a) ⭐ ANTI-VACUOUS: the gate fired %d times over one second — MORE THAN ZERO ('never polls' would make the whole feature inert)"),
				FireCount),
			FireCount > 0);

		TestTrue(*FString::Printf(TEXT("(a) …and %d times is ~20/second, not 60 — it is a POLL, not a per-frame read"),
				FireCount),
			FireCount >= 18 && FireCount <= 21);

		TestTrue(*FString::Printf(TEXT("(a) the fires are SPACED (frames %d and %d), not a burst at the start"),
				FirstFireFrame, SecondFireFrame),
			SecondFireFrame - FirstFireFrame >= 2);
	}

	// ── (b) ⛔ A BLUEPRINT CANNOT ASK FOR A PER-FRAME READ ───────────────────────────────────
	// CastProgressPollIntervalSeconds is EditDefaultsOnly, so a BP can type 0. The gate is shared with
	// the occlusion cull precisely so that its 30 Hz floor — a guarantee, not a tuning value — is
	// implemented ONCE. Two copies of a guarantee drift.
	{
		float Accumulator = 0.f;
		int32 FireCount = 0;
		for (int32 FrameIndex = 0; FrameIndex < 60; ++FrameIndex)
		{
			if (UCombatantHealthBarComponent::ShouldPollCastProgress(Accumulator, FrameSeconds, /*ConfiguredIntervalSeconds=*/ 0.f))
			{
				++FireCount;
			}
		}

		TestTrue(*FString::Printf(TEXT("(b) ⛔ an interval of 0 fires %d times/second, NOT 60 — the 30 Hz floor is a guarantee a Blueprint cannot lower"),
				FireCount),
			FireCount <= 31);

		TestTrue(TEXT("(b) …and it still fires — flooring must not switch the poll off"),
			FireCount > 0);
	}

	// ── (c) ⛔ NO CATCH-UP AFTER A HITCH ─────────────────────────────────────────────────────
	// On a fire the accumulator is RESET rather than having the period subtracted. Subtracting would
	// leave a one-second hitch holding ~20 periods of banked time and fire on ~20 CONSECUTIVE frames
	// afterwards — a burst arriving on the frames the game can least afford it.
	{
		float Accumulator = 0.f;

		TestTrue(TEXT("(c) a 1.0 s hitch fires the gate once"),
			UCombatantHealthBarComponent::ShouldPollCastProgress(Accumulator, /*DeltaSeconds=*/ 1.f, ShippedPollSeconds));

		TestEqual(TEXT("(c) ⛔ …and the accumulator is RESET TO ZERO, not decremented — the arrears are dropped"),
			Accumulator, 0.f, Tolerance);

		// ⚠️ TWO frames, ⛔ not three, and the number is MEASURED rather than chosen: three frames at
		// 1/60 s is 0.0500000 s, which REACHES the 0.05 s period and fires legitimately. An assertion
		// of "three quiet frames" would have been red on arrival — for the crime of the gate working.
		int32 ImmediateRefires = 0;
		for (int32 FrameIndex = 0; FrameIndex < 2; ++FrameIndex)
		{
			if (UCombatantHealthBarComponent::ShouldPollCastProgress(Accumulator, FrameSeconds, ShippedPollSeconds))
			{
				++ImmediateRefires;
			}
		}

		// ⭐ THIS IS THE ROW THAT DISCRIMINATES: under `-= Period` the accumulator would still hold
		// 0.95 s of banked time here, so the very NEXT frame fires — and the ~19 after it.
		TestEqual(TEXT("(c) ⛔ the two frames immediately after the hitch fire ZERO times — under `-= Period` the next frame would fire on 0.95 s of banked arrears"),
			ImmediateRefires, 0);

		int32 BurstFires = 0;
		for (int32 FrameIndex = 0; FrameIndex < 20; ++FrameIndex)
		{
			if (UCombatantHealthBarComponent::ShouldPollCastProgress(Accumulator, FrameSeconds, ShippedPollSeconds))
			{
				++BurstFires;
			}
		}

		TestTrue(*FString::Printf(TEXT("(c) ⛔ over the 20 frames after a 1 s hitch the gate fires %d times — its ORDINARY rate. A catch-up implementation would fire on ~19 of those 20 CONSECUTIVE frames, i.e. a trace/poll burst on the frames the game can least afford it"),
				BurstFires),
			BurstFires <= 8);
	}

	// ── (d) ⭐ ITS OWN ACCUMULATOR — the cast row and the cull do not share banked time ───────
	// Passing separate accumulators must give separate schedules; sharing one would make the cast row
	// inherit the cull's 150 ms period through the back door.
	{
		float CastAccumulator = 0.f;
		float OcclusionAccumulator = 0.f;

		int32 CastFires = 0;
		int32 OcclusionFires = 0;
		for (int32 FrameIndex = 0; FrameIndex < 60; ++FrameIndex)
		{
			if (UCombatantHealthBarComponent::ShouldPollCastProgress(CastAccumulator, FrameSeconds, ShippedPollSeconds))
			{
				++CastFires;
			}
			if (UCombatantHealthBarComponent::ShouldPollOcclusion(OcclusionAccumulator, FrameSeconds, OcclusionPollSeconds))
			{
				++OcclusionFires;
			}
		}

		TestTrue(*FString::Printf(TEXT("(d) ⭐ over one second the cast row polled %d times and the cull %d — separate accumulators, separate periods, one tick"),
				CastFires, OcclusionFires),
			CastFires > OcclusionFires);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  11. ⛔ THE OWNER ANSWERS ABOUT **ITSELF** — THE WITCH NEVER PUSHES ONTO ANOTHER ACTOR'S BAR
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarOwnerAnswersAboutItselfTest,
	"Siegebound.CastBar.TheComponentReadsItsOwnOwnersProviderAndKnowsNothingAboutWitches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarOwnerAnswersAboutItselfTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	FString ComponentSource;
	if (!LoadProjectSource(*this, ComponentCppPath, ComponentSource))
	{
		return false;
	}

	FString UpdateBody;
	if (!ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::UpdateCastProgress()"), UpdateBody))
	{
		return false;
	}

	// ── POSITIVE CONTROL FIRST ───────────────────────────────────────────────────────────────
	TestEqual(TEXT("SELF-CHECK: UpdateCastProgress reads the cast GATE — the probe is pointed at a live poll, so the absences below mean something"),
		CountOccurrencesInCode(UpdateBody, TEXT("IsCastInProgress()")), 1);

	// ── (a) ⭐ BOTH GETTERS, FROM THE **OWNER'S** PROVIDER ───────────────────────────────────
	// They share ONE resolver on the owner, so asking them together is what makes a bar's GATE and
	// its FILL incapable of disagreeing. Reading one this poll and the other next poll would
	// reintroduce that disagreement by hand.
	TestEqual(TEXT("(a) ⭐ the fill is read on the SAME poll as the gate — one resolver, so the two can never disagree"),
		CountOccurrencesInCode(UpdateBody, TEXT("GetCastProgressPercent()")), 1);

	TestEqual(TEXT("(a) ⛔ the provider is resolved from GetOwner() — the target answers about ITSELF (WITCH-§9.3)"),
		CountOccurrencesInCode(UpdateBody, TEXT("Cast<IHealthBarProvider>(GetOwner())")), 1);

	// ── (b) ⛔⛔ THE COMPONENT KNOWS NOTHING ABOUT WITCHES, ANYWHERE IN THE FILE ─────────────
	// ⭐ THE STRUCTURAL EXPRESSION OF THE TWO-ENDED DESIGN: the subject PULLS the witch's live timer
	// through its own back-pointer, on the unit side. If the bar layer ever learns what a witch is,
	// somebody has started PUSHING — which strands a bar on the target the moment the witch dies
	// mid-cast, and the interrupt rule makes that the COMMON case rather than an edge one.
	TestEqual(TEXT("(b) ⛔⛔ 'ASummonedUnit' appears ZERO times in the whole component — a bar that knew what a witch was would be a bar somebody is PUSHING to"),
		CountOccurrencesInCode(ComponentSource, TEXT("ASummonedUnit")), 0);

	TestEqual(TEXT("(b) ⛔ 'IncomingWitchCaster' appears ZERO times — the back-pointer is the UNIT's business; the bar only ever asks its own owner"),
		CountOccurrencesInCode(ComponentSource, TEXT("IncomingWitchCaster")), 0);

	TestEqual(TEXT("(b) ⛔ 'WitchCastTarget' appears ZERO times — same rule, the other end of the same link"),
		CountOccurrencesInCode(ComponentSource, TEXT("WitchCastTarget")), 0);

	// ── (c) ⛔ AND THERE IS NO CLOCK HERE ────────────────────────────────────────────────────
	// The percent is derived per call from the ONE timer the unit already owns. A second clock in the
	// bar layer would be a second source of truth that survives a cancel.
	TestEqual(TEXT("(c) ⛔ the component owns NO timer — the cast has exactly one clock and it lives on the unit"),
		CountOccurrencesInCode(ComponentSource, TEXT("FTimerHandle")), 0);

	TestEqual(TEXT("(c) ⛔ …and sets none"),
		CountOccurrencesInCode(ComponentSource, TEXT("SetTimer")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  12. ⛔ ONE ATOMIC EVENT, ONE WRITER — AND THE C++ HALF NEVER NAMES A WIDGET ELEMENT
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarSinglePushSiteTest,
	"Siegebound.CastBar.ExactlyOnePlaceDecidesWhatTheWidgetIsToldAndTheCollapseIsNotTheComponentsJob",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarSinglePushSiteTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	FString ComponentSource;
	if (!LoadProjectSource(*this, ComponentCppPath, ComponentSource))
	{
		return false;
	}

	// ── (a) ⛔ THE EVENT IS PUSHED FROM EXACTLY ONE PLACE ────────────────────────────────────
	// The seed at BeginPlay and every poll both route through PushCastProgress, so there is exactly
	// ONE function that decides what the widget is told. A second push site is how a seed and a poll
	// start disagreeing about a row that is meant to carry one atomic value.
	TestEqual(TEXT("(a) ⛔ 'OnCastProgressChanged' is called exactly once in the whole component — one place decides what the widget is told"),
		CountOccurrencesInCode(ComponentSource, TEXT("OnCastProgressChanged")), 1);

	FString PushBody;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::PushCastProgress(bool bCasting, float RawCastPercent)"), PushBody))
	{
		TestEqual(TEXT("(a) ⛔ …and that one call lives in PushCastProgress — the seed and every poll both arrive here"),
			CountOccurrencesInCode(PushBody, TEXT("OnCastProgressChanged(")), 1);

		// ⛔ The latch is written BEFORE anything can early out, so a missing widget cannot leave the
		// falling edge undetectable when a widget appears later.
		TestEqual(TEXT("(a) the row latch is written exactly once, unconditionally, at the top of the push"),
			CountOccurrencesInCode(PushBody, TEXT("bCastRowDriven = bCasting;")), 1);

		// ⛔ SANITISED AT THE PUSH SITE, so no caller can hand the widget a raw provider value.
		TestEqual(TEXT("(a) ⛔ the percent is sanitised AT the push site — no caller can route a raw provider value to the widget"),
			CountOccurrencesInCode(PushBody, TEXT("SanitizeCastPercent(")), 1);

		// ⛔ Geometry BEFORE the value: the widget should be laid out into the box it is about to
		// fill, so a cast's first frame is never a full-height fill inside a 22 px box.
		const FString PushCode = StripCommentLines(PushBody);
		const int32 GeometryIndex = PushCode.Find(TEXT("ApplyCastRowGeometry("), ESearchCase::CaseSensitive);
		const int32 EventIndex = PushCode.Find(TEXT("OnCastProgressChanged("), ESearchCase::CaseSensitive);

		if (GeometryIndex == INDEX_NONE || EventIndex == INDEX_NONE)
		{
			AddError(TEXT("⛔ Could not locate both the geometry call and the event push on CODE lines of PushCastProgress — the ordering probe is STALE."));
		}
		else
		{
			TestTrue(TEXT("(a) ⛔ the geometry is applied BEFORE the value is pushed — a cast's first frame must not paint a full-height fill inside a 22 px box"),
				GeometryIndex < EventIndex);
		}
	}

	// ── (b) ⛔⛔ THE COLLAPSE IS `TASK-861`'s JOB, AND THIS FILE PROVES THE C++ STAYED OUT ────
	// The component supplies the DATA; the widget decides what is SHOWN. That is `WITCH-§9.3`'s
	// contract, pinned in law precisely so the two halves never negotiate it.
	// ⚠️ It is also a live count-pin: `SetVisibility(` is pinned at EXACTLY 1 across this file by
	// SiegeHealthBarOcclusionTest's owner-intent-latch row. A second call added for the cast row would
	// have turned that test red, under a name mentioning neither this task nor the cast bar — and the
	// tempting "fix" would have deleted a single-writer guarantee two other tests rest on.
	TestEqual(TEXT("(b) ⛔⛔ 'CastBarRoot' is named ZERO times in C++ — the widget owns the collapse, and this file must never learn that name"),
		CountOccurrencesInCode(ComponentSource, TEXT("CastBarRoot")), 0);

	TestEqual(TEXT("(b) ⛔ 'CastBarFill' is named ZERO times in C++ — same rule; the component pushes a float and a bool and nothing else"),
		CountOccurrencesInCode(ComponentSource, TEXT("CastBarFill")), 0);

	FString GeometryBody;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::ApplyCastRowGeometry(bool bCasting)"), GeometryBody))
	{
		TestEqual(TEXT("(b) ⛔ the cast geometry calls SetVisibility ZERO times — the row's collapse is the WIDGET's job, and the file's single-writer pin is untouched"),
			CountOccurrencesInCode(GeometryBody, TEXT("SetVisibility(")), 0);
	}

	FString UpdateBody;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::UpdateCastProgress()"), UpdateBody))
	{
		TestEqual(TEXT("(b) ⛔ …and neither does the cast poll"),
			CountOccurrencesInCode(UpdateBody, TEXT("SetVisibility(")), 0);
	}

	FString PushBodyAgain;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::PushCastProgress(bool bCasting, float RawCastPercent)"), PushBodyAgain))
	{
		TestEqual(TEXT("(b) ⛔ …nor the push"),
			CountOccurrencesInCode(PushBodyAgain, TEXT("SetVisibility(")), 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  13. ⛔ NO CACHED PERCENT LIVES IN THIS COMPONENT — THE STORED-TELL BAN, SCOPED TO ITS HEADER
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarNoStoredPercentTest,
	"Siegebound.CastBar.TheComponentRemembersWhetherTheRowIsOpenAndNeverWhatItSaid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarNoStoredPercentTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	FString HeaderSource;
	if (!LoadProjectSource(*this, ComponentHeaderPath, HeaderSource))
	{
		return false;
	}

	// ⛔⛔ WITCH-§4 requires an interrupt to leave NO partial state, and the unit already honours that
	// by DERIVING the percent per call from the live timer. Caching it HERE would reintroduce, one
	// layer up, exactly the remembered value that survives a cancel and freezes the bar mid-flight —
	// a bar that says a cast is still running after the player already interrupted it.
	//
	// ⚠️ THE NEEDLE IS A MEMBER SHAPE, NOT THE WORD "CastPercent", AND THAT IS DELIBERATE (`SC-§39`):
	// `CastPercent` occurs 3× on code lines in this header as a SUBSTRING of the parameter names
	// `ProviderCastPercent` and `RawCastPercent`, so banning the bare word would be RED ON ARRIVAL
	// for the crime of the shipped signatures being well named. A member declaration ends in `;` and
	// a member initialiser contains ` = `; a parameter does neither.
	TestEqual(TEXT("(a) ⛔ no member declaration ends in 'Percent;' — nothing in this component remembers what the fill said"),
		CountOccurrencesInCode(HeaderSource, TEXT("Percent;")), 0);

	TestEqual(TEXT("(a) ⛔ …and no member initialiser assigns one either"),
		CountOccurrencesInCode(HeaderSource, TEXT("Percent =")), 0);

	// ── POSITIVE CONTROL: the probe CAN see member shapes in this file ───────────────────────
	// Without this, the two zeros above are indistinguishable from a needle that never matches
	// anything (`SC-§40`: a claim of ABSENCE must be paired with proof the scanner was alive).
	TestTrue(TEXT("SELF-CHECK: the same needle SHAPE matches real members here ('Accumulator = 0.f;' ×2) — the zeros above are a fact, not a dead probe"),
		CountOccurrencesInCode(HeaderSource, TEXT("Accumulator = 0.f;")) == 2);

	// ── (b) ⭐ WHAT IT **IS** ALLOWED TO REMEMBER, DECLARED EXACTLY ONCE ─────────────────────
	// ⛔ NOT "is a cast running" — that question is only ever answered by the owner, on the poll that
	// asks it. This records whether the ROW IS CURRENTLY DRIVEN OPEN, which is the single fact the
	// falling edge cannot be detected without.
	TestEqual(TEXT("(b) ⭐ the one piece of state the row keeps is 'bCastRowDriven', declared exactly once — what the row was TOLD, never what is TRUE"),
		CountOccurrencesInCode(HeaderSource, TEXT("bool bCastRowDriven")), 1);

	// ⚠️ SCOPE, STATED SO NOBODY WIDENS IT: the `bIsCasting` / `bCastInProgress` ban that `TASK-830`
	// wrote belongs to `SummonedUnit.h` and MUST NOT be widened tree-wide (ruled by `TASK-849`) — this
	// component's ordinary LOCALS legitimately carry those names. A header can only declare MEMBERS,
	// which is why the ban is a header rule in both files and a source rule in neither.
	TestEqual(TEXT("(b) ⛔ no cast-state BOOL member beyond the row latch — a 'bIsCasting' member here would be a second source of truth (a LOCAL of that name is fine and is not what this counts)"),
		CountOccurrencesInCode(HeaderSource, TEXT("bool bIsCasting")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  14. ⛔ THE CAST POLL SITS **ABOVE** ALL THREE OCCLUSION EARLY-OUTS
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarTickPlacementTest,
	"Siegebound.CastBar.TheCastPollIsNotGatedByTheOcclusionCullsThreeEarlyOuts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarTickPlacementTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	FString ComponentSource;
	if (!LoadProjectSource(*this, ComponentCppPath, ComponentSource))
	{
		return false;
	}

	FString TickBody;
	if (!ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::TickComponent("), TickBody))
	{
		return false;
	}

	// ⛔ COMMENT LINES ARE STRIPPED BEFORE ANY INDEX IS TAKEN, AND IT IS LOAD-BEARING: TickComponent's
	// comment block NAMES `bOccludeHealthBarWhenBlocked` some 1 200 characters ABOVE the code line
	// that tests it, so an index comparison on the raw text would compare a warning against a
	// statement and read backwards — a probe that passes or fails for a reason unrelated to the code.
	const FString TickCode = StripCommentLines(TickBody);

	const int32 CastPollIndex = TickCode.Find(TEXT("ShouldPollCastProgress("), ESearchCase::CaseSensitive);
	const int32 CullFlagIndex = TickCode.Find(TEXT("bOccludeHealthBarWhenBlocked"), ESearchCase::CaseSensitive);
	const int32 OwnerLatchIndex = TickCode.Find(TEXT("bBarShownByOwner"), ESearchCase::CaseSensitive);
	const int32 OcclusionPollIndex = TickCode.Find(TEXT("ShouldPollOcclusion("), ESearchCase::CaseSensitive);

	// ── POSITIVE CONTROL: all four symbols are present on code lines ─────────────────────────
	if (CastPollIndex == INDEX_NONE || CullFlagIndex == INDEX_NONE || OwnerLatchIndex == INDEX_NONE || OcclusionPollIndex == INDEX_NONE)
	{
		AddError(*FString::Printf(TEXT("⛔ POSITIVE CONTROL FAILED: cast poll %d · cull flag %d · owner latch %d · occlusion poll %d (INDEX_NONE = -1). A missing symbol would make every ordering below vacuously true."),
			CastPollIndex, CullFlagIndex, OwnerLatchIndex, OcclusionPollIndex));
		return false;
	}

	// ── (a) ⛔ NOT UNDER `bOccludeHealthBarWhenBlocked` ──────────────────────────────────────
	// That flag is about STONEWORK and has nothing to say about casts. 🧑 It is also Jonathan's feel
	// switch — turning the occlusion cull off must not silently delete the Witch's only tell.
	TestTrue(*FString::Printf(TEXT("(a) ⛔ the cast poll (code index %d) precedes the cull's master switch (%d) — a flag about STONEWORK must never switch off the cast tell"),
			CastPollIndex, CullFlagIndex),
		CastPollIndex < CullFlagIndex);

	// ── (b) ⛔⛔ NOT UNDER `!bBarShownByOwner` — THE HIGHEST-CONSEQUENCE ONE ─────────────────
	// A unit that DIES MID-CAST would otherwise stop polling with its row latched OPEN, and its bar
	// would come back from a respawn still painting the cast that killed it.
	TestTrue(*FString::Printf(TEXT("(b) ⛔⛔ the cast poll (%d) precedes the owner-intent early-out (%d) — a unit that dies MID-CAST must not respawn still painting it"),
			CastPollIndex, OwnerLatchIndex),
		CastPollIndex < OwnerLatchIndex);

	// ── (c) ⛔ AND NOT UNDER THE CULL'S OWN 0.15 s GATE ──────────────────────────────────────
	// Inheriting that period is measurably wrong here — test 9 is the arithmetic.
	TestTrue(*FString::Printf(TEXT("(c) ⛔ the cast poll (%d) precedes the occlusion poll gate (%d) — it must not inherit the cull's 0.15 s period"),
			CastPollIndex, OcclusionPollIndex),
		CastPollIndex < OcclusionPollIndex);

	// ── (d) ⭐ AND IT STILL RUNS AFTER Super::TickComponent ──────────────────────────────────
	// Super FIRST and unconditionally: UWidgetComponent::TickComponent → UpdateWidget() →
	// UpdateWidgetOnScreen(), the engine's add/remove of this bar on FWorldWidgetScreenLayer.
	// Skipping it for ANY reason would strand the bar's screen projection.
	const int32 SuperIndex = TickCode.Find(TEXT("Super::TickComponent("), ESearchCase::CaseSensitive);
	TestTrue(*FString::Printf(TEXT("(d) ⭐ Super::TickComponent (%d) still runs FIRST, before the cast poll (%d) — the engine's screen-layer add/remove is not skipped"),
			SuperIndex, CastPollIndex),
		SuperIndex != INDEX_NONE && SuperIndex < CastPollIndex);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  15. ⛔⛔ THE PINNED C++↔WIDGET CONTRACT — CHARACTER-FOR-CHARACTER, BECAUSE `TASK-861` BINDS
//      IT BY STRING
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCastBarWidgetContractTest,
	"Siegebound.CastBar.TheOneEventIsDeclaredExactlyAsTheLawPinsItAndCarriesFloatAndBoolOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCastBarWidgetContractTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCastBarTestFixture;

	FString WidgetHeader;
	if (!LoadProjectSource(*this, WidgetHeaderPath, WidgetHeader))
	{
		return false;
	}

	// ⛔⛔ `WITCH-§9.3` pins this signature CHARACTER-FOR-CHARACTER as the cross-task guarantee between
	// `TASK-860` and `TASK-861`, and the pin is not ceremony: `TASK-861` authors the widget-side
	// override by NAME, through MCP, against an asset this file cannot see. A rename here does not
	// break a compile — it silently produces an event nothing implements, which in UE is a SILENT
	// NO-OP. ⇒ the whole tell would vanish with a green build and a green suite.
	TestEqual(TEXT("(a) ⛔⛔ the pinned signature is present character-for-character: 'void OnCastProgressChanged(float CastPercent, bool bCasting);'"),
		CountOccurrencesInCode(WidgetHeader, TEXT("void OnCastProgressChanged(float CastPercent, bool bCasting);")), 1);

	// ── (b) ⛔ IT IS A BlueprintImplementableEvent, NOT A BlueprintNativeEvent ───────────────
	// The three shipped events on this widget (OnHPChanged, SetTeamColor, SetDamageBoost) are all
	// BIEs, and the cast row is the fourth. ⚠️ The widget MUST author it as a TRUE override
	// (bOverrideFunction = true): a K2Node_CustomEvent of the same name is DSL-indistinguishable,
	// reports bIsImplemented = true, and NEVER fires from C++ — the exact defect that hid the
	// health-bar bug five times (TASK-131). ⛔ That is asserted on the ASSET by TASK-861, not here.
	TestEqual(TEXT("(b) ⛔ the widget declares FOUR BlueprintImplementableEvents — the three shipped rows plus the cast row, all the same shape"),
		CountOccurrencesInCode(WidgetHeader, TEXT("UFUNCTION(BlueprintImplementableEvent, Category = \"Siegebound|UI\")")), 4);

	// ── (c) ⛔ FLOAT AND BOOL ONLY — NEVER AN ENUM ──────────────────────────────────────────
	// The MCP BP-param rule: an enum pin is not authorable through the tooling that builds this asset.
	// ⭐ And it is also why the cast bar's amber is a DESIGN-TIME decision — the pinned contract
	// carries no colour channel, so there is nothing to push a tint through even if someone wanted to.
	FString EventDeclaration;
	const int32 DeclarationIndex = WidgetHeader.Find(TEXT("void OnCastProgressChanged("), ESearchCase::CaseSensitive);
	if (DeclarationIndex == INDEX_NONE)
	{
		AddError(TEXT("⛔ POSITIVE CONTROL FAILED: 'void OnCastProgressChanged(' not found at all — (a) above could not have passed, and (c) would be vacuous."));
		return false;
	}

	const int32 DeclarationEnd = WidgetHeader.Find(TEXT(";"), ESearchCase::CaseSensitive, ESearchDir::FromStart, DeclarationIndex);
	if (DeclarationEnd == INDEX_NONE)
	{
		AddError(TEXT("⛔ The declaration of 'OnCastProgressChanged' has no terminating ';' — the probe is STALE, so it FAILS rather than slicing a garbage substring."));
		return false;
	}

	EventDeclaration = WidgetHeader.Mid(DeclarationIndex, DeclarationEnd - DeclarationIndex);

	TestFalse(*FString::Printf(TEXT("(c) ⛔ the declaration '%s' carries no enum parameter — float/bool ONLY (the MCP BP-param rule)"), *EventDeclaration),
		EventDeclaration.Contains(TEXT("enum"), ESearchCase::IgnoreCase));

	TestFalse(TEXT("(c) ⛔ …and no struct/colour parameter — there is deliberately NO colour channel, which is why the amber is design-time"),
		EventDeclaration.Contains(TEXT("FLinearColor"), ESearchCase::CaseSensitive));

	TestTrue(TEXT("(c) the percent parameter is a float and the gate a bool — the two the law names, in that order"),
		EventDeclaration.Contains(TEXT("float CastPercent"), ESearchCase::CaseSensitive)
			&& EventDeclaration.Contains(TEXT("bool bCasting"), ESearchCase::CaseSensitive));

	// ── (d) ⛔ THE WIDGET HOLDS NO CAST STATE AND BINDS NO CAST DELEGATE ─────────────────────
	// There is no cast delegate to bind — the surface is poll-shaped by design (TASK-830 §17.1), and
	// the component owns the whole path exactly as it owns the boost row's.
	TestEqual(TEXT("(d) ⛔ the widget declares no cast delegate — the surface is poll-shaped by design, and one push path means no second source of truth"),
		CountOccurrencesInCode(WidgetHeader, TEXT("FOnCastProgressChanged")), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
