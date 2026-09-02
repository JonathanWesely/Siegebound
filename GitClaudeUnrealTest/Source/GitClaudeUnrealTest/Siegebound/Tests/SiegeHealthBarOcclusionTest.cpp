// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WidgetComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/EngineTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE HEALTH-BAR OCCLUSION CULL (TASK-791; law `VIS-§2`,
 *      ruling `VIS-R1`; `SHIP-§9c`) ═══
 *
 *  Subject: `UCombatantHealthBarComponent`'s three pure seams — `ComputeDesiredBarVisibility`
 *  (the visibility rule), `ShouldPollOcclusion` (the poll gate) and `ComputeOcclusionFromTraceResult`
 *  (the buried-camera rule) — plus the three pinned `EditDefaultsOnly` tunables and four
 *  structural guards.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ WHY TESTS 9-12 EXIST: `qa/TASK-801` FAILED THIS FILE'S SUBJECT WITH TWO BLOCKERS, AND
 *      ⛔ NEITHER OF THEM WAS REACHABLE FROM TESTS 1-8.
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  QA's own summary of this file was that "anti-vacuity was applied rigorously to what the suites
 *  COVER, and both blockers live in what they DON'T." Both lived in the ⛔ CALL GRAPH:
 *
 *    • `B-1` — `ASummonedUnit`'s death path wrote `HPBarWidget->SetVisibility(false)` RAW instead of
 *      `HideBar()`, so the owner-intent latch never went false and the next poll put a 0-HP bar back
 *      over every corpse on the field. ⭐ Test 2's proof that the cull "can only subtract" was
 *      SOUND — it was simply never fed. → **test 10** (owners) and **test 11** (the component
 *      itself) are the two halves of that guard.
 *    • `B-2` — the buried-camera fail-open was delegated to `bFindInitialOverlaps`, which is INERT on
 *      a line trace; and `TASK-790` parks the camera INSIDE the watchtower by design, so the whole
 *      roster's bars would have blinked off together. → **test 9** is the rule, **test 12** is the
 *      guard that the trace keeps feeding it the truth.
 *
 *  ⚠️ Tests 10-12 are SOURCE PROBES, which is a weak instrument, and they are weak on purpose rather
 *  than by accident: what they guard has ⛔ no headless witness at all (see the un-covered list
 *  below). Each carries a POSITIVE CONTROL so it cannot pass on a moved file or a renamed symbol —
 *  and every count they assert was measured on disk before shipping, not predicted.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE TRAP THIS ENTIRE FILE IS POINTED AT: **A VISIBILITY CULL IS THE EASIEST THING IN
 *      THE ENGINE TO TEST VACUOUSLY.**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  A cull's tests pass when the feature works AND when the feature is completely inert,
 *  because "the bar was not hidden" is the correct answer in the overwhelming majority of
 *  states. ⚠️ Five assertions elsewhere in this project stopped discriminating this week
 *  without going red once. So every test below carries an explicit **anti-vacuous** clause,
 *  and they are the assertions to read first:
 *
 *    • Test 1 does not merely check the occluded and the clear case separately — it asserts
 *      that the two answers **DIFFER**. An implementation that quietly stopped reading its
 *      `bOccluded` argument satisfies one of those checks and fails this one.
 *    • Test 3 ("disabling restores the old behaviour") is worthless on its own: it passes
 *      perfectly for a cull that does nothing at all, in EITHER state. So it also asserts
 *      that ENABLING the cull changes at least one outcome — i.e. that the thing it just
 *      proved can be switched off is a thing that was ever switched on.
 *    • Test 4 asserts the poll fires **more than zero** times, not just fewer than every
 *      frame. "Never traces" trivially satisfies "does not trace per frame" while making the
 *      whole feature inert — and it would leave tests 1-3 green forever.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ WHY THIS RUNS HEADLESS AT ALL — AND WHAT THAT COSTS
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Both seams are `static` and pure: bools and floats in, one value out — no `UWorld`, no
 *  `AActor`, no camera, no clock, no RNG (the `ASummonedUnit::HeightAdvantageMultiplier` /
 *  `HIGH-§3` precedent). ⇒ the visibility rule and the poll arithmetic run in-process with
 *  no PIE session. ⚠️ If a test here ever starts needing a world, that purity has been
 *  broken and it is a FINDING, not a reason to add a fixture.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — STATED SO NOBODY MISTAKES GREEN FOR DONE
 *      (`SC-§32`: a mechanism never observed to function is not known to function)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • ⛔ **THE TRACE ITSELF — STILL, AND THIS IS THE HONEST LIMIT OF TESTS 9 AND 12.**
 *      `UpdateHealthBarOcclusion()` reads `GetOwnerPlayer()`, a `PlayerCameraManager` and
 *      `UWorld::LineTraceSingleByChannel`. There is no headless path to any of them, and ⛔ this
 *      file does not pretend otherwise — inventing an injectable camera purely so a test could
 *      reach it would add shipped surface to make a test possible, which is the tail wagging the
 *      dog. ⭐ What CAN now be asserted headlessly is the RULE the trace feeds (test 9) and the
 *      fact that it is still the query form capable of feeding it (test 12). ⛔ What still cannot
 *      be asserted is that a real camera inside a real tower produces a real zero-distance hit —
 *      that chain is established by reading UE 5.8 source (`Chaos::FConvex::RaycastFast`,
 *      `ChaosInterfaceWrapperCore.h:117`, `CollisionConversions.cpp:366`) and it is owed a PIE
 *      row. Its instruments remain QA's diff read and **Jonathan's pixels**: (1) stand on the
 *      watchtower deck with units on the ground below and confirm the four floating bars from
 *      VID-004 @ 02:08.0 are gone; (2) walk to the ladder until the camera pushes in and confirm
 *      the other units' bars ⛔ do NOT all vanish; (3) kill a rigged unit in view and confirm no
 *      0-HP bar reappears over the corpse.
 *    • ⛔ **THAT `SetVisibility` ACTUALLY HIDES A SCREEN-SPACE BAR.** That is an ENGINE
 *      guarantee, verified by reading UE 5.8 source rather than asserted here:
 *      `UWidgetComponent::UpdateWidgetOnScreen()` (WidgetComponent.cpp:1317) removes the
 *      widget from `FWorldWidgetScreenLayer` unless `IsVisible()`. A test cannot add
 *      confidence to that and would only restate it.
 *    • ⛔ **THE ≤1-FRAME AND ≤1-PERIOD LATENCIES.** Both are consequences of polling that the
 *      design accepted on purpose; they are documented on the members, not asserted.
 */

namespace SiegeHealthBarOcclusionTestFixture
{
	/** A 60 fps frame, as a float — the tick rate every timing expectation below is expressed in. */
	constexpr float FrameSeconds = 1.f / 60.f;

	/** Float comparison tolerance (the SiegeHighGroundTest house value). */
	constexpr float Tolerance = 1.e-4f;

	/**
	 *  Drives ShouldPollOcclusion over a run of fixed-length frames and reports how many times it
	 *  fired, plus the frame index of the first two fires (so spacing can be asserted rather than
	 *  merely counted — a gate that fired 66 times in the first 66 frames and never again would
	 *  otherwise pass a pure count check).
	 */
	struct FPollRun
	{
		int32 FireCount = 0;
		int32 FirstFireFrame = INDEX_NONE;
		int32 SecondFireFrame = INDEX_NONE;
	};

	FPollRun RunPoll(int32 FrameCount, float DeltaSeconds, float ConfiguredIntervalSeconds)
	{
		FPollRun Run;
		float Accumulator = 0.f;

		for (int32 FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
		{
			if (UCombatantHealthBarComponent::ShouldPollOcclusion(Accumulator, DeltaSeconds, ConfiguredIntervalSeconds))
			{
				++Run.FireCount;

				if (Run.FirstFireFrame == INDEX_NONE)
				{
					Run.FirstFireFrame = FrameIndex;
				}
				else if (Run.SecondFireFrame == INDEX_NONE)
				{
					Run.SecondFireFrame = FrameIndex;
				}
			}
		}

		return Run;
	}

	/**
	 *  Reflection readers for the three PINNED tunables. They are `protected` on the component and
	 *  they stay that way — a cull's knobs are not public API, and widening them so a test could see
	 *  them would be exactly the shipped-surface-for-a-test mistake this file refuses elsewhere.
	 *
	 *  ⚠️ EACH RETURNS A POINTER AND EVERY CALLER NULL-CHECKS WITH `AddError`, because a renamed or
	 *  RETYPED field makes `FindFProperty` return null — and a test that silently skipped its own
	 *  assertions on a null would report SAFE for ever, which is the precise failure mode
	 *  `SiegeAssistantSelectionTest` documents.
	 */
	/**
	 *  ⚠️ VALUES ARE READ THROUGH `GetPropertyValue_InContainer`, ⛔ NOT through a
	 *  `ContainerPtrToValuePtr<bool>` cast. An `FBoolProperty` may be backed by a BITFIELD, and a
	 *  raw cast would then read a neighbouring byte and report a confidently wrong answer — a
	 *  reflection test that lies is worse than no reflection test.
	 */
	const FBoolProperty* FindBoolProperty(const UObject* Object, const TCHAR* FieldName)
	{
		return FindFProperty<FBoolProperty>(Object->GetClass(), FieldName);
	}

	const FFloatProperty* FindFloatProperty(const UObject* Object, const TCHAR* FieldName)
	{
		return FindFProperty<FFloatProperty>(Object->GetClass(), FieldName);
	}

	/** TEnumAsByte<ECollisionChannel> reflects as an FByteProperty carrying the enum. */
	const FByteProperty* FindByteProperty(const UObject* Object, const TCHAR* FieldName)
	{
		return FindFProperty<FByteProperty>(Object->GetClass(), FieldName);
	}

	//~ ── SOURCE PROBES (added at the qa/TASK-801 repair) ──────────────────────────────────────────
	//  ⚠️ A SOURCE-STRING PROBE IS A WEAK INSTRUMENT AND IS USED HERE ONLY BECAUSE THE THING IT
	//  GUARDS HAS NO OTHER HEADLESS WITNESS. Both TASK-801 blockers lived in the CALL GRAPH — one
	//  owner writing this component's visibility raw, and one query form that cannot tell a wall from
	//  a camera inside one. Neither is reachable from a pure function, and neither can be reached
	//  without a UWorld, a camera and a physics scene. What a probe CAN do is fail loudly when the
	//  shape returns. ⛔ Every probe below therefore carries a POSITIVE CONTROL, so it can never pass
	//  on a moved file, a renamed member or an empty string (the SiegeHeroCameraTest test-7 idiom).

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
	 *  Counts occurrences of a needle on CODE lines only — comment lines are skipped.
	 *  ⚠️ Load-bearing here above all: the very symbols these probes forbid (`SetVisibility`,
	 *  `bFindInitialOverlaps`, `LineTraceTestByChannel`) are NAMED REPEATEDLY in the shipped comments
	 *  that explain why they are forbidden. A naive count would read those warnings as violations and
	 *  report a permanent, unfixable red.
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
	 *  Extracts one function body by signature, ending at the first column-0 closing brace (`\n}`) —
	 *  how every function in this codebase ends. ⛔ Deliberately NOT a parser: a signature that stops
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

	/** This component's own translation unit — the subject of the latch and instrument probes. */
	const TCHAR* const ComponentCppPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp");

	/**
	 *  ⭐ THE COMPLETE CENSUS OF C++ OWNERS OF A UCombatantHealthBarComponent, and it is complete by
	 *  measurement rather than by memory: `CreateDefaultSubobject<UCombatantHealthBarComponent>`
	 *  occurs in exactly these three files (ASummonedUnit — and through it AMinerUnit; ABuilding —
	 *  and through it ATower/ABarracks/ADeepMine; AHeroCharacter).
	 *  ⚠️ ACastle is deliberately ABSENT: its HPBarWidget is a BASE UWidgetComponent and writes
	 *  visibility raw at Castle.cpp:1251/:1447 — legal today, and the exact trap a future class swap
	 *  would inherit. That is boarded separately; this probe would start failing the moment the swap
	 *  lands, which is precisely when someone should be reading it.
	 */
	const TCHAR* const ComponentOwnerCppPaths[] =
	{
		TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp"),
		TEXT("Source/GitClaudeUnrealTest/Siegebound/Building.cpp"),
		TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp"),
	};
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  1. THE HEADLINE — AN OCCLUDED BAR HIDES, A CLEAR ONE SHOWS, AND THE TWO ANSWERS DIFFER
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionHidesBlockedBarsTest,
	"Siegebound.HealthBarOcclusion.ABlockedBarHidesAndAClearBarShowsAndTheAnswersDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionHidesBlockedBarsTest::RunTest(const FString& Parameters)
{
	// ── (a) THE VID-004 DEFECT, AS ONE BOOL: a live unit whose bar is behind stonework HIDES ──
	// This is the four-bars-over-an-empty-deck case at 02:08.0 reduced to its decision.
	const bool bOccludedVisibility = UCombatantHealthBarComponent::ComputeDesiredBarVisibility(
		/*bOwnerWantsBarShown=*/true, /*bCullEnabled=*/true, /*bOccluded=*/true);

	TestFalse(TEXT("(a) ⭐ A live, opted-in unit whose bar is BLOCKED by world geometry is HIDDEN — the VID-004 defect"),
		bOccludedVisibility);

	// ── (b) …and the ordinary case is untouched: nothing in the way ⇒ the bar is still there ──
	// The failure this catches is the over-correction — a cull so eager it deletes the HUD.
	const bool bClearVisibility = UCombatantHealthBarComponent::ComputeDesiredBarVisibility(
		/*bOwnerWantsBarShown=*/true, /*bCullEnabled=*/true, /*bOccluded=*/false);

	TestTrue(TEXT("(b) A live, opted-in unit with a CLEAR line to the camera is VISIBLE — the cull is not a blanket hide"),
		bClearVisibility);

	// ── (c) ⭐⭐ THE ANTI-VACUOUS CLAUSE, AND THE MOST IMPORTANT LINE IN THIS FILE ────────────
	// (a) and (b) are each individually satisfiable by a function that ignores bOccluded entirely
	// and returns a constant — one of them would pass and the other fail, but a reviewer scanning
	// green ticks would not know which. Asserting that the two answers DISAGREE is the assertion
	// that goes red the moment the occlusion input stops being consulted, which is how a cull dies
	// quietly: still called every poll, still costing a trace, no longer changing anything.
	TestTrue(TEXT("(c) ⭐⭐ Flipping ONLY bOccluded flips the answer — the cull is actually reading its input"),
		bOccludedVisibility != bClearVisibility);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  2. THE CULL MAY ONLY SUBTRACT — A DEAD UNIT'S BAR IS NEVER RESURRECTED BY A CLEAR TRACE
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionNeverResurrectsAHiddenBarTest,
	"Siegebound.HealthBarOcclusion.TheCullOnlySubtractsSoAnOwnerHiddenBarStaysHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionNeverResurrectsAHiddenBarTest::RunTest(const FString& Parameters)
{
	// ⭐ THE REGRESSION THIS FILE EXISTS TO PREVENT IS NOT THE ONE IN THE FOOTAGE.
	// Before TASK-791, HideBar() wrote SetVisibility(false) and nothing else ever wrote it back
	// except the owner. The cull introduced a SECOND writer — and the obvious wrong shape for it
	// (poll, then SetVisibility(!bOccluded)) would make every dead unit's bar pop back on at the
	// next poll, ~150 ms after death, all over the field. That is a far louder defect than bars
	// drawing through stonework, and these four assertions are what stand between the two.

	TestFalse(TEXT("(a) ⭐ Owner hid the bar (death) + trace CLEAR ⇒ still hidden — the cull cannot resurrect it"),
		UCombatantHealthBarComponent::ComputeDesiredBarVisibility(/*bOwnerWantsBarShown=*/false, /*bCullEnabled=*/true, /*bOccluded=*/false));

	TestFalse(TEXT("(b) Owner hid the bar + trace BLOCKED ⇒ hidden (the two reasons compose, they do not cancel)"),
		UCombatantHealthBarComponent::ComputeDesiredBarVisibility(/*bOwnerWantsBarShown=*/false, /*bCullEnabled=*/true, /*bOccluded=*/true));

	// An opted-out owner (miners set bShowHealthBar = false) must stay hidden with the cull OFF too:
	// disabling the cull restores the old behaviour, and the old behaviour also kept them hidden.
	TestFalse(TEXT("(c) Owner hid the bar + cull DISABLED ⇒ hidden — an opted-out miner never shows a bar"),
		UCombatantHealthBarComponent::ComputeDesiredBarVisibility(/*bOwnerWantsBarShown=*/false, /*bCullEnabled=*/false, /*bOccluded=*/false));

	// ⭐ The structural statement of the same law, over the WHOLE input space: for every possible
	// (bCullEnabled, bOccluded) pair, no input whatsoever makes an owner-hidden bar visible. This
	// fails against an OR, against an argument-order swap, and against any future third term that
	// forgets to keep the owner's intent outermost.
	const bool bFlags[] = { false, true };
	for (const bool bCullEnabled : bFlags)
	{
		for (const bool bOccluded : bFlags)
		{
			TestFalse(*FString::Printf(
					TEXT("(d) ⭐ EXHAUSTIVE: owner-hidden stays hidden for cull=%s occluded=%s — no input can override the owner"),
					bCullEnabled ? TEXT("on") : TEXT("off"), bOccluded ? TEXT("yes") : TEXT("no")),
				UCombatantHealthBarComponent::ComputeDesiredBarVisibility(/*bOwnerWantsBarShown=*/false, bCullEnabled, bOccluded));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  3. THE PINNED REGRESSION GUARD — bOccludeHealthBarWhenBlocked = false IS THE OLD BEHAVIOUR
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionDisabledRestoresOldBehaviourTest,
	"Siegebound.HealthBarOcclusion.DisablingTheCullReproducesThePreTask791BehaviourExactly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionDisabledRestoresOldBehaviourTest::RunTest(const FString& Parameters)
{
	const bool bFlags[] = { false, true };

	// ── (a) With the cull off, visibility is the OWNER'S INTENT AND NOTHING ELSE ─────────────
	// Pre-TASK-791, ShowBarIfEnabled() called SetVisibility(bShowHealthBar) and HideBar() called
	// SetVisibility(false); nothing else participated. "Exactly" in the spec means exactly, so this
	// is asserted over the entire input space rather than at a representative point — including the
	// occluded column, which is where a leaking cull would show up.
	for (const bool bOwnerWantsBarShown : bFlags)
	{
		for (const bool bOccluded : bFlags)
		{
			TestTrue(*FString::Printf(
					TEXT("(a) Cull DISABLED: visibility == owner intent (%s) regardless of occlusion (%s) — the old behaviour, unchanged"),
					bOwnerWantsBarShown ? TEXT("show") : TEXT("hide"), bOccluded ? TEXT("blocked") : TEXT("clear")),
				UCombatantHealthBarComponent::ComputeDesiredBarVisibility(bOwnerWantsBarShown, /*bCullEnabled=*/false, bOccluded)
					== bOwnerWantsBarShown);
		}
	}

	// ── (b) ⭐⭐ THE ANTI-VACUOUS CLAUSE — WITHOUT THIS, (a) IS WORTHLESS ─────────────────────
	// (a) passes perfectly for a cull that does NOTHING in either state: if the feature were inert,
	// "disabled behaves like before" would be trivially true and would read as a green tick on a
	// dead mechanism. So the guard is only meaningful next to proof that the switch has two sides.
	// This asserts there EXISTS an input on which enabling the cull changes the answer — i.e. that
	// the thing we just proved can be turned off was ever on.
	const bool bWithCullOff = UCombatantHealthBarComponent::ComputeDesiredBarVisibility(
		/*bOwnerWantsBarShown=*/true, /*bCullEnabled=*/false, /*bOccluded=*/true);
	const bool bWithCullOn = UCombatantHealthBarComponent::ComputeDesiredBarVisibility(
		/*bOwnerWantsBarShown=*/true, /*bCullEnabled=*/true, /*bOccluded=*/true);

	TestTrue(TEXT("(b) ⭐⭐ The toggle has two SIDES: on a blocked bar, enabling the cull changes the answer"),
		bWithCullOff != bWithCullOn);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  4. THE PERF CLAUSE — THE POLL FIRES ON A FRACTION OF FRAMES, AND ON MORE THAN NONE
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionDoesNotTraceEveryFrameTest,
	"Siegebound.HealthBarOcclusion.ThePollFiresOnAFractionOfFramesAndOnMoreThanZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionDoesNotTraceEveryFrameTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	// Ten seconds of 60 fps at the SHIPPED default period. Every expectation below is re-derived
	// from Duration/Interval — never transcribed from the component, which is the only way this
	// test can disagree with a wrong default rather than agreeing with it by construction.
	const int32 FrameCount = 600;
	const float IntervalSeconds = 0.15f;
	const float DurationSeconds = FrameCount * FrameSeconds;
	const float ExpectedFires = DurationSeconds / IntervalSeconds; // ≈ 66.7

	const FPollRun Run = RunPoll(FrameCount, FrameSeconds, IntervalSeconds);

	// ── (a) ⛔ NOT PER FRAME — the spec term, stated as the crudest possible assertion ────────
	// VIS-§2 forbids a trace per pawn per frame by name. At 60 fps × 20 units a per-frame design is
	// 1200 traces/second; this design is ≈133. If the gate is ever removed or inverted, this line
	// reports 600 and names the number.
	TestTrue(*FString::Printf(TEXT("(a) ⛔ The poll fired %d times in %d frames — a trace PER FRAME is refused"),
			Run.FireCount, FrameCount),
		Run.FireCount < FrameCount);

	// ── (b) ⭐⭐ THE ANTI-VACUOUS CLAUSE: "never traces" ALSO satisfies (a) ───────────────────
	// A gate that returns false forever passes (a) with a perfect score and leaves the entire
	// feature inert — bars would never hide, tests 1-3 would stay green, and the only symptom would
	// be the original footage defect still on screen. This is the assertion that catches that.
	TestTrue(TEXT("(b) ⭐⭐ …and it fired MORE THAN ZERO times — an inert gate satisfies (a) perfectly"),
		Run.FireCount > 0);

	// ── (c) The rate is the CONFIGURED rate, within float tolerance ──────────────────────────
	// Bounds are ±15% / +5% of Duration/Interval rather than an exact count: accumulating 1/60 in
	// float can land a fire one frame either side of the ideal boundary, and asserting an exact 66
	// would be a flaky test dressed as a precise one. The band is still narrow enough to fail any
	// wrong period — half the rate or double it both land far outside.
	TestTrue(*FString::Printf(
			TEXT("(c) Fired %d times in %.2f s at a %.2f s period — expected ≈%.1f (Duration/Interval, re-derived)"),
			Run.FireCount, DurationSeconds, IntervalSeconds, ExpectedFires),
		Run.FireCount >= FMath::FloorToInt(ExpectedFires * 0.85f) && Run.FireCount <= FMath::CeilToInt(ExpectedFires * 1.05f));

	// ── (d) The fires are SPACED, not front-loaded ───────────────────────────────────────────
	// A count alone cannot tell "one trace every 9 frames" from "67 traces in the first 67 frames
	// and silence thereafter" — and the second would be a burst, i.e. the cost this design refuses,
	// hiding behind a healthy-looking total. The expected gap is Interval/FrameSeconds = 9 frames;
	// asserting ≥ 8 leaves one frame of float slack.
	const int32 ExpectedGapFrames = FMath::FloorToInt(IntervalSeconds / FrameSeconds);

	if (Run.FirstFireFrame == INDEX_NONE || Run.SecondFireFrame == INDEX_NONE)
	{
		AddError(TEXT("(d) SELF-CHECK FAILED: fewer than two fires in 10 seconds — the spacing assertion could not run."));
	}
	else
	{
		TestTrue(*FString::Printf(
				TEXT("(d) Consecutive fires are %d frames apart (first at %d, second at %d) — expected ≈%d, never back-to-back"),
				Run.SecondFireFrame - Run.FirstFireFrame, Run.FirstFireFrame, Run.SecondFireFrame, ExpectedGapFrames),
			(Run.SecondFireFrame - Run.FirstFireFrame) >= (ExpectedGapFrames - 1));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  5. A ZERO OR NEGATIVE INTERVAL IS FLOORED — IT CANNOT BECOME THE PER-FRAME TRACE
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionZeroIntervalIsFlooredTest,
	"Siegebound.HealthBarOcclusion.AZeroOrNegativeIntervalCannotDegradeIntoAPerFrameTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionZeroIntervalIsFlooredTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	// HealthBarOcclusionIntervalSeconds is EditDefaultsOnly, so a Blueprint CAN set it to 0 — and
	// "0 means as fast as possible" is such a natural reading that someone will eventually try it.
	// Without a floor that single field would reinstate, per pawn, the exact cost VIS-§2 forbids by
	// name. The floor is a spec guarantee rather than a tuning value, so it is asserted here.
	const int32 FrameCount = 600;
	const float PathologicalIntervals[] = { 0.f, -1.f, -0.0001f };

	for (const float IntervalSeconds : PathologicalIntervals)
	{
		const FPollRun Run = RunPoll(FrameCount, FrameSeconds, IntervalSeconds);

		// (a) The crude form: it is not per-frame.
		TestTrue(*FString::Printf(TEXT("(a) Interval %.4f: fired %d times in %d frames — NOT once per frame"),
				IntervalSeconds, Run.FireCount, FrameCount),
			Run.FireCount < FrameCount);

		// (b) ⭐ THE CONTRACT FORM, and it is deliberately expressed as the DOCUMENTED guarantee
		// ("at most 30 polls per second") rather than by transcribing the floor constant out of the
		// .cpp — a test copied from its subject agrees with its subject by construction. At 60 fps,
		// 30/s is one fire every other frame, so the bound is FrameCount/2 (+1 for the boundary).
		TestTrue(*FString::Printf(
				TEXT("(b) ⭐ Interval %.4f: %d fires ≤ one per TWO frames — the documented ≤30 polls/second floor holds"),
				IntervalSeconds, Run.FireCount),
			Run.FireCount <= (FrameCount / 2) + 1);

		// (c) Anti-vacuous: a floor that clamped to infinity would also pass (a) and (b) while
		// switching the cull off for anyone who typed 0. It must still poll.
		TestTrue(*FString::Printf(TEXT("(c) Interval %.4f: the cull still polls (%d fires) — the floor throttles, it does not disable"),
				IntervalSeconds, Run.FireCount),
			Run.FireCount > 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  6. A LONG HITCH DOES NOT BANK A BURST OF CONSECUTIVE-FRAME TRACES
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionNoCatchUpBurstTest,
	"Siegebound.HealthBarOcclusion.ASecondLongHitchDoesNotBankACatchUpBurstOfTraces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionNoCatchUpBurstTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	// ⭐ THE BUG THIS CATCHES IS THE ONE A CAREFUL PROGRAMMER WRITES ON PURPOSE.
	// The textbook accumulator subtracts the period on a fire (`Accum -= Period`) to preserve
	// phase. Here that is wrong: after a 1.0 s hitch the accumulator would still hold ~0.85 s —
	// SIX MORE periods — and would fire on six CONSECUTIVE frames afterwards. Per pawn. Across the
	// roster. On the frames immediately following a hitch, which are the frames the game can least
	// afford it. Resetting to zero drops the arrears, which is the right trade for a cull whose
	// skipped answers are stale anyway. This test is the only thing that tells the two apart.
	const float IntervalSeconds = 0.15f;
	float Accumulator = 0.f;

	// (a) The hitch frame itself fires exactly once.
	const bool bHitchFired = UCombatantHealthBarComponent::ShouldPollOcclusion(Accumulator, /*DeltaSeconds=*/1.f, IntervalSeconds);

	TestTrue(TEXT("(a) A 1.0 s hitch frame DOES fire the poll — a long stall must not be silently swallowed"),
		bHitchFired);

	// (b) ⭐ …and the frames that follow it are quiet. 8 frames at 60 fps is 0.133 s, i.e. still
	// short of one 0.15 s period, so a correct gate fires ZERO times here. A catch-up
	// implementation fires on all eight.
	const int32 QuietFrameCount = FMath::FloorToInt(IntervalSeconds / FrameSeconds) - 1; // 8
	int32 BurstFires = 0;

	for (int32 FrameIndex = 0; FrameIndex < QuietFrameCount; ++FrameIndex)
	{
		if (UCombatantHealthBarComponent::ShouldPollOcclusion(Accumulator, FrameSeconds, IntervalSeconds))
		{
			++BurstFires;
		}
	}

	TestEqual(*FString::Printf(
			TEXT("(b) ⭐ The %d frames after the hitch fire %d times — expected 0; a `-= Period` catch-up would fire on every one"),
			QuietFrameCount, BurstFires),
		BurstFires, 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  7. THE SHIPPED DEFAULTS ARE THE ONES VIS-§2 PINNED
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionShippedDefaultsTest,
	"Siegebound.HealthBarOcclusion.TheShippedDefaultsAreTheOnesTheLawPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionShippedDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	const UCombatantHealthBarComponent* const Defaults = GetDefault<UCombatantHealthBarComponent>();
	if (!Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<UCombatantHealthBarComponent>() returned null."));
		return false;
	}

	// ── (a) The cull ships ON ─────────────────────────────────────────────────────────────────
	// A cull that defaults to off is a cull that does not exist: nothing in the shipping data sets
	// this, so `false` here would mean VID-004's bars are still on screen with a green suite.
	if (const FBoolProperty* const OccludeProperty = FindBoolProperty(Defaults, TEXT("bOccludeHealthBarWhenBlocked")))
	{
		TestTrue(TEXT("(a) ⭐ bOccludeHealthBarWhenBlocked defaults TRUE — the cull ships ON, not opt-in"),
			OccludeProperty->GetPropertyValue_InContainer(Defaults));
	}
	else
	{
		AddError(TEXT("(a) SELF-CHECK FAILED: no bool property named 'bOccludeHealthBarWhenBlocked' — VIS-§2 PINS this name and it has been renamed or retyped."));
	}

	// ── (b) The channel is ECC_Visibility, and the reason is not cosmetic ────────────────────
	// The engine's stock Pawn and CharacterMesh profiles set Visibility to ECR_Ignore, so a
	// Visibility trace passes THROUGH units and stops on world geometry — it asks "is the WORLD in
	// the way?". Retargeting this to ECC_Camera or ECC_Pawn silently changes the QUESTION: on
	// ECC_Pawn every bar would flicker whenever a friendly crossed the camera line.
	if (const FByteProperty* const ChannelProperty = FindByteProperty(Defaults, TEXT("HealthBarOcclusionChannel")))
	{
		const uint8 OcclusionChannel = ChannelProperty->GetPropertyValue_InContainer(Defaults);

		TestTrue(*FString::Printf(TEXT("(b) HealthBarOcclusionChannel defaults to ECC_Visibility (raw %u) — pawns ignore it, world geometry blocks it"),
				static_cast<uint32>(OcclusionChannel)),
			static_cast<ECollisionChannel>(OcclusionChannel) == ECC_Visibility);
	}
	else
	{
		AddError(TEXT("(b) SELF-CHECK FAILED: no byte/enum property named 'HealthBarOcclusionChannel' — VIS-§2 PINS this name and it has been renamed or retyped."));
	}

	// ── (c) The period is 0.15 s, and it must be POSITIVE above all ─────────────────────────
	// The pinned value is asserted because VIS-§2 pins it; the positivity is asserted because it is
	// the property that actually protects the frame budget, and it would survive a future re-tune
	// of the number.
	if (const FFloatProperty* const IntervalProperty = FindFloatProperty(Defaults, TEXT("HealthBarOcclusionIntervalSeconds")))
	{
		const float IntervalSeconds = IntervalProperty->GetPropertyValue_InContainer(Defaults);

		TestEqual(TEXT("(c) HealthBarOcclusionIntervalSeconds defaults to 0.15 s — the VIS-§2 pinned period"),
			IntervalSeconds, 0.15f, Tolerance);

		TestTrue(TEXT("(c) …and it is strictly POSITIVE — a non-positive shipped period is the per-frame trace by another name"),
			IntervalSeconds > 0.f);
	}
	else
	{
		AddError(TEXT("(c) SELF-CHECK FAILED: no float property named 'HealthBarOcclusionIntervalSeconds' — VIS-§2 PINS this name and it has been renamed or retyped."));
	}

	// ── (d) The readback hook rests FAIL-OPEN ───────────────────────────────────────────────
	// Before any poll has run — and on every path where the camera cannot be resolved —
	// IsHealthBarOccluded() must read false, because "we could not tell" resolving to HIDDEN would
	// blank every health bar on the field. The CDO is the never-polled state.
	TestFalse(TEXT("(d) ⭐ IsHealthBarOccluded() is FALSE before any poll — the cull fails OPEN, never blanking the HUD"),
		Defaults->IsHealthBarOccluded());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  8. THE STRUCTURAL GUARD — THE WIDGET SPACE IS STILL Screen, BECAUSE World WAS REFUSED
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionWidgetSpaceIsStillScreenTest,
	"Siegebound.HealthBarOcclusion.TheWidgetSpaceIsStillScreenBecauseWorldSpaceWasRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionWidgetSpaceIsStillScreenTest::RunTest(const FString& Parameters)
{
	// ⭐⭐ THIS TEST DEFENDS A RULING, NOT A BEHAVIOUR — and it is the reason the cull can stay cheap.
	//
	// "Health bars draw through walls" has one obvious fix and it is the wrong one: switch to
	// EWidgetSpace::World and let the depth buffer sort it out. VIS-R1 REFUSED that, because World
	// space costs a RENDER TARGET PER BAR across a 20+ pawn roster, changes every bar's
	// scale/legibility law, and would be owed at three call sites. That refusal lives in a document;
	// documents do not go red. This does.
	//
	// ⚠️ It is also the single most likely well-intentioned regression on this component: the next
	// person to see a bar through a wall (the cull is a POLL — a bar can persist up to one period)
	// has every reason to reach for the widget space. This assertion meets them there and names the
	// ruling in its failure text.
	const UCombatantHealthBarComponent* const Defaults = GetDefault<UCombatantHealthBarComponent>();
	if (!Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<UCombatantHealthBarComponent>() returned null."));
		return false;
	}

	TestTrue(TEXT("⭐⭐ The bar is still EWidgetSpace::Screen — VIS-R1 refused the World-space switch (a render target PER BAR, at three call sites). The fix for occlusion is the visibility cull, not the widget space."),
		Defaults->GetWidgetSpace() == EWidgetSpace::Screen);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  9. ⛔ THE BURIED CAMERA FAILS OPEN — ONE BAD RAY MUST NOT BLANK THE WHOLE ROSTER (B-2)
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionBuriedCameraFailsOpenTest,
	"Siegebound.HealthBarOcclusion.ACameraInsideGeometryFailsOpenInsteadOfBlankingEveryBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionBuriedCameraFailsOpenTest::RunTest(const FString& Parameters)
{
	// ⭐⭐ THE DEFECT THIS ROW EXISTS FOR IS ⛔ NOT IN THE FOOTAGE — IT IS THE ONE THE FIX WOULD HAVE
	//     CREATED, AND IT IS LOUDER THAN THE ONE IT FIXES.
	//
	// TASK-790's near-clip floor deliberately parks the follow camera up to 150 uu INSIDE the
	// watchtower at the ladder approach — for ≈2 s, EVERY climb, by design. Chaos answers a ray that
	// starts inside a hull with a blocking hit at ZERO distance (FConvex::RaycastFast: every plane
	// distance negative ⇒ EntryTime never leaves 0 ⇒ OutTime = 0, return true). So for that whole
	// window EVERY unit's camera→bar ray is "blocked" — and the naive cull hides the ENTIRE ROSTER'S
	// HEALTH BARS AT ONCE, at exactly the moment and place VID-004 filmed.
	//
	// ⚠️ The original guard for this (`FCollisionQueryParams::bFindInitialOverlaps = false`) is INERT
	// on a line trace: its only consumer sits behind `if (!bIsSweep) return Block;`
	// (CollisionQueryFilterCallback.cpp:211-217), and a raycast is not a sweep (SceneQuery.cpp:522).
	// The burial is therefore detected on the HIT, and this is the row that proves the rule.

	// ── (a) ⛔ THE HEADLINE: buried camera ⇒ NOT occluded ─────────────────────────────────────
	TestFalse(TEXT("(a) ⛔ A trace that STARTED INSIDE geometry (zero distance) is NOT an occlusion — it is a non-answer, and non-answers fail OPEN"),
		UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(/*bTraceBlocked=*/true, /*bTraceStartedInsideGeometry=*/true, /*HitDistanceUU=*/0.f));

	// ── (b) ⭐⭐ THE ANTI-VACUOUS CLAUSE — "always fail open" satisfies (a) PERFECTLY ──────────
	// A rule that returned false for everything would pass (a) with full marks while switching the
	// entire cull off and putting VID-004's bars straight back on screen with a green suite. A real
	// wall, hit at a real distance, must still occlude.
	TestTrue(TEXT("(b) ⭐⭐ A genuine wall 900 uu down the ray DOES occlude — the fail-open is a special case, not a blanket off-switch"),
		UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(/*bTraceBlocked=*/true, /*bTraceStartedInsideGeometry=*/false, /*HitDistanceUU=*/900.f));

	// ── (c) An unblocked ray is not an occlusion, whatever the other fields say ───────────────
	const bool bFlags[] = { false, true };
	const float Distances[] = { 0.f, 900.f };
	for (const bool bStartedInside : bFlags)
	{
		for (const float DistanceUU : Distances)
		{
			TestFalse(*FString::Printf(TEXT("(c) EXHAUSTIVE: an UNBLOCKED trace is never an occlusion (startedInside=%s, distance=%.0f)"),
					bStartedInside ? TEXT("yes") : TEXT("no"), DistanceUU),
				UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(/*bTraceBlocked=*/false, bStartedInside, DistanceUU));
		}
	}

	// ── (d) ⭐ EACH VETO CLAUSE IS INDEPENDENTLY LOAD-BEARING ─────────────────────────────────
	// The rule has TWO witnesses to the same fact: the engine's own bStartPenetrating flag
	// (FHitResult::IsValidBlockingHit, HitResult.h:236-239) and the raw hit distance. On today's
	// engine path they cannot disagree — ChaosInterfaceWrapperCore.h:117 DEFINES the initial-overlap
	// flag as `Distance <= 0.f` and CollisionConversions.cpp:366 copies it into bStartPenetrating —
	// so these two rows are what keep BOTH clauses honest inside the function's own domain. Delete
	// either clause and one of them goes red.
	TestFalse(TEXT("(d) The DISTANCE clause alone vetoes: blocked, flag clear, but zero distance ⇒ still fails open"),
		UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(/*bTraceBlocked=*/true, /*bTraceStartedInsideGeometry=*/false, /*HitDistanceUU=*/0.f));

	TestFalse(TEXT("(d) The FLAG clause alone vetoes: blocked at a real distance, but flagged start-penetrating ⇒ still fails open"),
		UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(/*bTraceBlocked=*/true, /*bTraceStartedInsideGeometry=*/true, /*HitDistanceUU=*/900.f));

	// ── (e) ⭐⭐ THE END-TO-END STATEMENT, ACROSS BOTH PURE SEAMS: THE ROSTER SURVIVES ─────────
	// This is the assertion that actually names the symptom. Take the buried-camera trace answer from
	// (a) and push it through the visibility rule for a LIVE, OPTED-IN unit with the cull ON. The bar
	// must still be VISIBLE. If a future edit re-delegates the burial to a sweep-only flag, or drops
	// the zero-distance clause, this row reports the roster-wide HUD blackout by name.
	const bool bOcclusionFromBuriedCamera = UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(
		/*bTraceBlocked=*/true, /*bTraceStartedInsideGeometry=*/true, /*HitDistanceUU=*/0.f);

	TestTrue(TEXT("(e) ⭐⭐ A live unit's bar is STILL VISIBLE while the camera is buried in the tower — the whole roster's bars do NOT blink off together"),
		UCombatantHealthBarComponent::ComputeDesiredBarVisibility(
			/*bOwnerWantsBarShown=*/true, /*bCullEnabled=*/true, bOcclusionFromBuriedCamera));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  10. ⛔ NO OWNER WRITES THIS COMPONENT'S VISIBILITY DIRECTLY — THE B-1 REGRESSION, ASSERTED
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionOwnersUseTheLatchTest,
	"Siegebound.HealthBarOcclusion.EveryOwnerHidesTheBarThroughHideBarAndNeverThroughSetVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionOwnersUseTheLatchTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	// ⭐⭐ THIS IS THE ROW THAT WOULD HAVE CAUGHT THE SHIPPED BUG (qa/TASK-801 B-1), AND IT IS WORTH
	//     READING WHY THE OTHER EIGHT TESTS COULD NOT.
	//
	// Test 2 proves ComputeDesiredBarVisibility can only ever SUBTRACT — and that proof is sound. The
	// defect was a CALLER that never set the input: ASummonedUnit's death path wrote
	// `HPBarWidget->SetVisibility(false)` raw instead of `HideBar()`, so the owner-intent latch stayed
	// TRUE, the tick never early-outed, and the next poll ≈150 ms later recomputed the bar VISIBLE and
	// put a 0-HP bar back over the corpse — for up to the full 2 s death-anim hold, on every rigged
	// unit on the field. ⭐ Before TASK-791 that raw call was TERMINAL and correct; the cull is what
	// turned it into a bug, which is exactly the class of defect a leaf-class test cannot see.
	//
	// ⚠️ A source probe is a WEAK instrument and this one is honest about its reach: it catches the
	// SHAPE (`HPBarWidget->SetVisibility` / `->SetHiddenInGame`), not every possible alias — a raw
	// write through a local variable would slip past. It is here because the alternative is nothing.
	int32 TotalLatchCalls = 0;

	for (const TCHAR* const OwnerPath : ComponentOwnerCppPaths)
	{
		FString OwnerSource;
		if (!LoadProjectSource(*this, OwnerPath, OwnerSource))
		{
			continue; // LoadProjectSource has already failed the test — a stale probe never passes quietly.
		}

		// ── THE POSITIVE CONTROL, FIRST: this file really is an owner and the member still has its
		// pinned name. Without it, a moved file or a renamed member would make every count below zero
		// and the test would go green on a probe that scanned nothing.
		const int32 MemberMentions = CountOccurrencesInCode(OwnerSource, TEXT("HPBarWidget"));
		if (MemberMentions == 0)
		{
			AddError(FString::Printf(TEXT("⛔ POSITIVE CONTROL FAILED for '%s': 'HPBarWidget' appears 0× on code lines. The member was renamed or this file is no longer an owner — the probe below would have passed VACUOUSLY."), OwnerPath));
			continue;
		}

		TestEqual(*FString::Printf(TEXT("⛔ '%s' writes 'HPBarWidget->SetVisibility' 0 times — owners latch intent through HideBar()/ShowBarIfEnabled(), they never write visibility"), OwnerPath),
			CountOccurrencesInCode(OwnerSource, TEXT("HPBarWidget->SetVisibility")), 0);

		// The same law's other obvious spelling. A screen-space widget component does not follow actor
		// hidden-in-game state anyway, so this would be a silent no-op AND a latch bypass.
		TestEqual(*FString::Printf(TEXT("⛔ '%s' writes 'HPBarWidget->SetHiddenInGame' 0 times — same law, the other spelling"), OwnerPath),
			CountOccurrencesInCode(OwnerSource, TEXT("HPBarWidget->SetHiddenInGame")), 0);

		TotalLatchCalls += CountOccurrencesInCode(OwnerSource, TEXT("HPBarWidget->HideBar()"));
		TotalLatchCalls += CountOccurrencesInCode(OwnerSource, TEXT("HPBarWidget->ShowBarIfEnabled()"));
	}

	// ── ⭐⭐ THE ANTI-VACUOUS CLAUSE, AND IT IS THE WHOLE REASON THIS TEST MEANS ANYTHING ──────
	// "Nobody writes SetVisibility" is satisfied PERFECTLY by a codebase in which nobody touches the
	// bar at all — which is also what a mass find-and-delete would produce. So the census must also
	// show the LATCH ROUTE IS LIVE: somebody, somewhere, still hides or shows this bar the legal way.
	TestTrue(*FString::Printf(TEXT("⭐⭐ ANTI-VACUOUS: the latch route is actually USED — %d HideBar()/ShowBarIfEnabled() calls across the owners (0 would mean the probe above proved nothing)"),
			TotalLatchCalls),
		TotalLatchCalls > 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  11. ⛔ THE OWNER-INTENT LATCH HAS EXACTLY TWO WRITERS, AND THEY ARE THE TWO ENTRY POINTS
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionLatchWritersTest,
	"Siegebound.HealthBarOcclusion.TheOwnerIntentLatchIsWrittenOnlyByShowBarIfEnabledAndHideBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionLatchWritersTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	// ⭐ THE OTHER HALF OF B-1's GUARD. Test 10 stops an OWNER from bypassing the latch; this stops
	// the COMPONENT from doing it to itself. `bOwnerWantsBarShown` is the outer AND of the whole
	// visibility rule — the single thing that makes "the cull may only subtract" true. The moment the
	// poll, the tick, or a future convenience helper starts writing that field, the cull can override
	// the owner's hide-on-death and test 2's proof becomes a statement about an unreachable function.
	FString ComponentSource;
	if (!LoadProjectSource(*this, ComponentCppPath, ComponentSource))
	{
		return false; // Already failed — a stale probe never passes quietly.
	}

	// ── (a) EXACTLY TWO WRITES IN THE WHOLE TRANSLATION UNIT ─────────────────────────────────
	const int32 LatchWrites = CountOccurrencesInCode(ComponentSource, TEXT("bBarShownByOwner ="));

	TestEqual(*FString::Printf(TEXT("(a) ⛔ 'bBarShownByOwner =' appears %d times on code lines — expected EXACTLY 2 (a third writer is the cull learning to overrule the owner)"),
			LatchWrites),
		LatchWrites, 2);

	// ── (b) …AND THEY ARE THE TWO OWNER ENTRY POINTS, ⛔ NOT SOMEWHERE ELSE ───────────────────
	// (a) alone would stay green if the poll wrote the latch twice and the entry points stopped
	// writing it at all. These two extractions say WHERE the writes live, and each extraction FAILS
	// (rather than scanning an empty string) if its signature ever stops matching.
	FString ShowBody;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::ShowBarIfEnabled()"), ShowBody))
	{
		TestEqual(TEXT("(b) ShowBarIfEnabled() writes the latch exactly once — it records the owner's intent (bShowHealthBar) and delegates the write to ApplyBarVisibility()"),
			CountOccurrencesInCode(ShowBody, TEXT("bBarShownByOwner =")), 1);

		// ⛔ And it does NOT write visibility itself — ApplyBarVisibility() is the single writer.
		TestEqual(TEXT("(b) ⛔ ShowBarIfEnabled() calls SetVisibility 0 times — the single-writer rule holds inside the component too"),
			CountOccurrencesInCode(ShowBody, TEXT("SetVisibility(")), 0);
	}

	FString HideBody;
	if (ExtractFunctionBody(*this, ComponentSource, TEXT("void UCombatantHealthBarComponent::HideBar()"), HideBody))
	{
		TestEqual(TEXT("(b) HideBar() writes the latch exactly once — the death path outranks the cull by LATCHING, not by writing visibility"),
			CountOccurrencesInCode(HideBody, TEXT("bBarShownByOwner =")), 1);

		TestEqual(TEXT("(b) ⛔ HideBar() calls SetVisibility 0 times — same single-writer rule"),
			CountOccurrencesInCode(HideBody, TEXT("SetVisibility(")), 0);
	}

	// ── (c) ⭐ THE POSITIVE CONTROL: the single writer really does exist and really is alone ───
	// Without this, (a) and (b) would both pass on a component that had stopped writing visibility
	// altogether — bars frozen at their construction state, every other test in this file still green.
	TestEqual(TEXT("(c) ⭐ 'SetVisibility(' appears exactly once in the whole component — ApplyBarVisibility() is THE single writer, and it exists"),
		CountOccurrencesInCode(ComponentSource, TEXT("SetVisibility(")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  12. ⛔ THE OCCLUSION QUERY IS STILL THE SINGLE-HIT FORM, BECAUSE THE FAIL-OPEN NEEDS THE HIT
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHealthBarOcclusionQueryFormTest,
	"Siegebound.HealthBarOcclusion.TheTraceIsStillTheSingleHitFormBecauseABooleanCannotSeeABuriedCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHealthBarOcclusionQueryFormTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHealthBarOcclusionTestFixture;

	// ⭐⭐ THIS DEFENDS A RULING, NOT A BEHAVIOUR — the test-8 idiom, pointed at qa/TASK-801 B-2.
	//
	// The cull only ever asks "blocked?", so LineTraceTestByChannel is the obviously correct query and
	// the obviously correct optimisation, and someone WILL reach for it: it skips the hit conversion
	// entirely and carries EQueryFlags::AnyHit. ⛔ It is refused here, and the reason is invisible from
	// the call site: a single bool CANNOT distinguish "a wall is between the camera and the bar" from
	// "the camera is standing inside that wall" — and TASK-790 puts the camera inside the watchtower
	// for ≈2 s at every ladder approach, by design. Swapping this back would restore a roster-wide HUD
	// blackout silently, with tests 1-9 all green, because the rule in test 9 would still be perfect
	// and would simply stop being fed the truth.
	//
	// Documents do not go red. This does, and it names the ruling in its failure text.
	FString ComponentSource;
	if (!LoadProjectSource(*this, ComponentCppPath, ComponentSource))
	{
		return false;
	}

	// ── THE POSITIVE CONTROL FIRST — the probe is pointed at a live trace function ────────────
	const int32 TraceFunctionMentions = CountOccurrencesInCode(ComponentSource, TEXT("UpdateHealthBarOcclusion"));
	if (TraceFunctionMentions == 0)
	{
		AddError(TEXT("⛔ POSITIVE CONTROL FAILED: 'UpdateHealthBarOcclusion' appears 0× on code lines. The trace was renamed or removed — every count below would have passed VACUOUSLY."));
		return false;
	}

	// ── (a) The query is the SINGLE form, exactly once ───────────────────────────────────────
	TestEqual(TEXT("(a) ⭐ 'LineTraceSingleByChannel' appears exactly once — the cull runs ONE trace, and it is the form that returns a HIT"),
		CountOccurrencesInCode(ComponentSource, TEXT("LineTraceSingleByChannel")), 1);

	// ── (b) ⛔ …and the cheaper TEST form has NOT come back ──────────────────────────────────
	TestEqual(TEXT("(b) ⛔ 'LineTraceTestByChannel' appears 0 times — VIS-R1/B-2: the boolean form cannot tell a wall from a camera buried inside one, so the fail-open would silently die with it"),
		CountOccurrencesInCode(ComponentSource, TEXT("LineTraceTestByChannel")), 0);

	// ── (c) ⛔ …and neither has the INERT flag that made B-2 a shipped defect ────────────────
	// bFindInitialOverlaps reads like the right guard and is a no-op on a line trace: its only engine
	// consumer sits behind `if (!bIsSweep) return Block;` (CollisionQueryFilterCallback.cpp:211-217),
	// and SceneQuery.cpp:522 sets bIsSweep false for a raycast. It is named in the shipped comments
	// (which this probe skips) precisely so nobody re-adds it as protection.
	TestEqual(TEXT("(c) ⛔ 'bFindInitialOverlaps' is set 0 times on code lines — it is INERT on a raycast and must not return as a false guard"),
		CountOccurrencesInCode(ComponentSource, TEXT("bFindInitialOverlaps")), 0);

	// ── (d) ⭐ The hit is actually CONSULTED for the burial ──────────────────────────────────
	// (a)-(c) are all satisfied by code that runs the single-hit query and then throws the hit away,
	// using only the boolean return — which is the exact defect in a more expensive costume.
	TestTrue(TEXT("(d) ⭐ 'bStartPenetrating' is read on a code line — the single-hit form is paid for AND used; discarding the hit would reinstate B-2 at a higher price"),
		CountOccurrencesInCode(ComponentSource, TEXT("bStartPenetrating")) > 0);

	TestTrue(TEXT("(d) ⭐ …and the answer is routed through ComputeOcclusionFromTraceResult — the rule test 9 proves is the rule the trace actually uses"),
		CountOccurrencesInCode(ComponentSource, TEXT("ComputeOcclusionFromTraceResult")) > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
