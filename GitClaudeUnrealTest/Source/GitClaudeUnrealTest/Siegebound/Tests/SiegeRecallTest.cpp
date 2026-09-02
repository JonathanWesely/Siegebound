// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/HeroCharacter.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for RECALL — the 10-second channel home (TASK-748, `RECALL-§`) ═══
 *
 *  Jonathan, verbatim (2026-09-01): "you can press 'b', and that will allow the player to
 *  starting a recall animation similar to league of legends where after 10 seconds they teleport
 *  back to their castle and they completely refill their health. During this recall animation the
 *  player cannot attack and if they get hit with an attack it interupts the channel and they
 *  would have to press 'b' again to start it from the beginning."
 *
 *  Subject: `FSiegeRecallStatics` / `FSiegeRecallState` / `FSiegeRecallArrival` / `ESiegeRecallExit`
 *  (HeroCharacter.h) plus the shipped tunables and the recall region of HeroCharacter.cpp.
 *  QA gate: TASK-753. Compile + suite run: TASK-754.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE TWO TRAPS THIS WHOLE FILE IS POINTED AT — BOTH WOULD REVIEW AS CORRECT
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔ TRAP ONE — the death-path restore function. It re-applies the hero's CUMULATIVE upgrade
 *  mods onto a freshly-restored base, re-arms the War Banner aura and restores input that death
 *  disabled. Reused for a recall it would DOUBLE-APPLY every upgrade stack and re-arm a running
 *  aura, every single time the player recalls, and the diff would look like sensible reuse.
 *
 *  ⛔ TRAP TWO — the heal must read the EFFECTIVE maximum, not the raw base field. A base read
 *  under-heals a two-stack Plate-Armor hero by 200 HP, so "they completely refill their health"
 *  quietly becomes false while the line reads perfectly.
 *
 *  ⚖️ Neither trap is provable by an absence, so test 12 SCANS THE SHIPPED SOURCE between two
 *  sentinel comments and fails on either one appearing. That is a new instrument for this
 *  project and it is DECLARED as such in `handoffs/TASK-748-programmer.md` — it exists because
 *  TASK-753's gate requires both traps "asserted by a test, ⛔ not merely absent", and a
 *  headless suite has no other way to watch a call that must never happen.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ WHY THE CHANNEL IS TESTED AGAINST PURE STATICS RATHER THAN A LIVE HERO —
 *  A MEASUREMENT, NOT A PREFERENCE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *    • Every automation test in this project is HEADLESS: there is not one `UWorld::CreateWorld`
 *      and not one `SpawnActor` in `Siegebound/Tests/` (stated at `SiegeLadderClimbTest.cpp:39`).
 *    • ⛔ AND A WORLD-LESS `AHeroCharacter` CANNOT BE DRIVEN THROUGH THESE PATHS AT ALL: the
 *      channel reads `UWorld::GetTimeSeconds`, broadcasts dynamic multicast delegates, spawns a
 *      Niagara component and — on the death exit — reaches `UCharacterMovementComponent`, whose
 *      `GetPhysicsVolume` dereferences `GetWorld()` unconditionally when `UpdatedComponent` is
 *      null. A `NewObject`'d hero would CRASH the suite, not fail it.
 *    ⇒ So every DECISION the channel makes lives in `FSiegeRecallStatics` (the
 *      `FSiegeLadderClimbStatics` / `FSiegeStuckStatics` / `HeightAdvantageMultiplier` seam
 *      precedent) and is exercised here exhaustively; the WIRING of each of the seven call
 *      sites is TASK-753's diff read. ⛔ That split is stated rather than hidden — `SC-§32`: a
 *      mechanism never observed to function is not known to function, and pretending a green
 *      suite covers the wiring would be the lie.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️ AND THE LESSON THIS FILE IS WRITTEN AGAINST: AN ASSERTION WHOSE TWO SIDES ARE EQUAL BY
 *  CONSTRUCTION PROVES NOTHING (`SHIP-§9c`). Every claim below is either (a) a TRUTH TABLE over
 *  a function with more than one input, where dropping a term flips a row, (b) an expectation
 *  RE-DERIVED from Jonathan's own sentence or from a shipped tunable read by reflection rather
 *  than transcribed from the subject, or (c) paired with a POSITIVE CONTROL that fails if the
 *  probe went stale. Where a claim could only be tautological it is NOT MADE, and test 10 names
 *  what it cannot prove.
 *
 *  🔒 Airlock: no `Capture()`, no `EnsureSnapshot()`, Zone A untouched, ⛔ no token figure
 *  (`AS-§12g`). Nothing here runs a model, opens the editor, or touches Git.
 */

namespace SiegeRecallTestFixture
{
	/**
	 *  ⭐ HIS NUMBER, FROM HIS SENTENCE — "after 10 seconds". ⛔ Deliberately not read out of
	 *  `RecallChannelSeconds`: an expectation transcribed from its subject cannot disagree with a
	 *  wrong subject. Test 1 is the one place these two meet, and that is the whole point of it.
	 */
	constexpr float ChannelSecondsFromHisSentence = 10.f;

	/** The Plate-Armor stack count the law's "under-heals by up to 200 HP" figure is stated at. */
	constexpr int32 PlateArmorStacksInTheLawsExample = 2;

	/** Tolerance for float comparisons that are not exact-by-nature. Tight enough that any real mistake blows through it. */
	constexpr float Tolerance = 1.e-4f;

	/** Exact-equality tolerance, for the claims whose whole content is the word EXACTLY. */
	constexpr float Exact = 0.f;

	/** A channel that started at this clock value — an arbitrary non-zero epoch, so nothing can pass by accidentally comparing against 0. */
	constexpr double Epoch = 1234.5;

	/** Builds a running channel anchored at the given spot, started at Epoch. */
	static FSiegeRecallState RunningChannel(const FVector& Anchor = FVector::ZeroVector)
	{
		return FSiegeRecallStatics::Begin(Epoch, Anchor);
	}

	/** Reads a shipped float tunable off the hero CDO by reflection; returns false (and reports) when the property is gone. */
	static bool ReadHeroFloat(FAutomationTestBase& Test, const TCHAR* PropertyName, float& OutValue)
	{
		const FFloatProperty* const Property = FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), PropertyName);
		if (Property == nullptr)
		{
			Test.AddError(FString::Printf(
				TEXT("⛔ AHeroCharacter has no float UPROPERTY '%s'. That member IS the thing under test, so its disappearance is an ERROR, not a skip."),
				PropertyName));
			return false;
		}

		const AHeroCharacter* const Defaults = GetDefault<AHeroCharacter>();
		if (Defaults == nullptr)
		{
			Test.AddError(TEXT("⛔ The AHeroCharacter class default object did not resolve."));
			return false;
		}

		OutValue = Property->GetPropertyValue_InContainer(Defaults);
		return true;
	}

	/** Loads one of the hero's two shipped source files; reports and returns false rather than passing quietly. */
	static bool LoadHeroSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(
				TEXT("⛔ Could not find '%s'. This probe scans the SHIPPED source for the two RECALL-§1 traps; a probe that cannot read its subject must FAIL, never report SAFE."),
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

	/** Non-overlapping occurrence count of Needle in Haystack. */
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
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. HIS TEN SECONDS, AND THE TUNABLE IS EDIT-DEFAULTS-ONLY
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallChannelDurationTest,
	"Siegebound.Recall.ChannelRunsForHisTenSecondsAndIsAuthoredOnDefaultsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallChannelDurationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	// ── (a) The shipped duration IS the number in his sentence ───────────────────────────
	// ⭐ This is the ONE place the expectation and the subject meet, and the expectation came
	// from his words rather than from the header — so it goes red the day somebody retunes the
	// channel without a ruling from him.
	float ShippedChannelSeconds = 0.f;
	if (!ReadHeroFloat(*this, TEXT("RecallChannelSeconds"), ShippedChannelSeconds))
	{
		return false;
	}

	TestEqual(TEXT("(a) ⭐ RecallChannelSeconds is EXACTLY the 10 seconds Jonathan asked for"),
		ShippedChannelSeconds, ChannelSecondsFromHisSentence, Exact);

	// ── (b) It is EditDefaultsOnly, not EditAnywhere ─────────────────────────────────────
	// A per-instance-editable channel length lets two heroes in one level disagree about how
	// long a recall takes, which is a balance divergence nobody would ever look for.
	{
		const FFloatProperty* const ChannelProperty =
			FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), TEXT("RecallChannelSeconds"));

		TestTrue(TEXT("(b) RecallChannelSeconds is exposed to the editor at all (positive control)"),
			ChannelProperty != nullptr && ChannelProperty->HasAnyPropertyFlags(CPF_Edit));

		TestTrue(TEXT("(b) ⛔ …and it is EditDefaultsOnly — instance-level editing is disabled"),
			ChannelProperty != nullptr && ChannelProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}

	// ── (c) The movement-cancel tolerance is a real, sane band ───────────────────────────
	// ⚠️ Both ends matter and both can fail: 0 would cancel the channel on animation settle and
	// capsule penetration-resolve (it would look broken and random), and a large value would
	// hand the player a free repositioning budget — the exact thing the cancel exists to stop.
	float ShippedTolerance = 0.f;
	if (!ReadHeroFloat(*this, TEXT("RecallMoveCancelToleranceUU"), ShippedTolerance))
	{
		return false;
	}

	TestTrue(TEXT("(c) The movement-cancel tolerance is strictly positive — a 0 would cancel on idle jitter"),
		ShippedTolerance > 0.f);

	TestTrue(TEXT("(c) ⛔ …and small enough that no walk input can stay inside it (under 100 uu — a fifth of a second of walking at the shipped 500 u/s)"),
		ShippedTolerance < 100.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⭐ THE COMPLETION BOUNDARY — 9.9 s IS NOT ENOUGH, AND THAT IS THE
//     INTERRUPT TEST'S WHOLE FOUNDATION
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallCompletionBoundaryTest,
	"Siegebound.Recall.CompletionNeedsTheFullChannelAndNinePointNineSecondsIsNotEnough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallCompletionBoundaryTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	const FSiegeRecallState Channel = RunningChannel();
	const float Duration = ChannelSecondsFromHisSentence;

	// ── (a) Under the full duration ⇒ NOT complete ───────────────────────────────────────
	// ⭐ 9.9 is the interrupt scenario the task names by name: a channel that is one tenth of a
	// second from home has earned NOTHING. This row fails against an implementation that
	// rounded, floored to whole seconds, or compared against a stale duration.
	const double IncompleteOffsets[] = { 0.0, 0.5, 5.0, 9.9, 9.999 };
	for (const double Offset : IncompleteOffsets)
	{
		TestFalse(*FString::Printf(TEXT("(a) At +%.3f s of a %.0f s channel the recall is NOT complete"), Offset, Duration),
			FSiegeRecallStatics::IsComplete(Channel, Epoch + Offset, Duration));
	}

	// ── (b) At and past the full duration ⇒ complete ─────────────────────────────────────
	// The positive half, without which (a) would pass vacuously against a function that always
	// returned false.
	const double CompleteOffsets[] = { 10.0, 10.001, 60.0 };
	for (const double Offset : CompleteOffsets)
	{
		TestTrue(*FString::Printf(TEXT("(b) At +%.3f s the recall IS complete"), Offset),
			FSiegeRecallStatics::IsComplete(Channel, Epoch + Offset, Duration));
	}

	// ── (c) ⛔ A channel that is not running is NEVER complete ────────────────────────────
	// Otherwise a hero standing still with no channel would "arrive" on its first tick.
	TestFalse(TEXT("(c) ⛔ The cleared state is never complete, no matter how much time has passed"),
		FSiegeRecallStatics::IsComplete(FSiegeRecallStatics::Cleared(), Epoch + 9999.0, Duration));

	// ── (d) Progress tracks the same clock and is clamped ────────────────────────────────
	TestEqual(TEXT("(d) Progress at the start is 0"),
		FSiegeRecallStatics::Progress01(Channel, Epoch, Duration), 0.f, Exact);

	TestEqual(TEXT("(d) Progress at the half-way point is 0.5"),
		FSiegeRecallStatics::Progress01(Channel, Epoch + 5.0, Duration), 0.5f, Tolerance);

	TestEqual(TEXT("(d) Progress at 9.9 s is 0.99 — visibly nearly-there, and still worth nothing"),
		FSiegeRecallStatics::Progress01(Channel, Epoch + 9.9, Duration), 0.99f, Tolerance);

	TestEqual(TEXT("(d) Progress is clamped at 1 well past the end"),
		FSiegeRecallStatics::Progress01(Channel, Epoch + 500.0, Duration), 1.f, Exact);

	TestEqual(TEXT("(d) ⛔ Progress with no channel running is 0, never a stale fraction"),
		FSiegeRecallStatics::Progress01(FSiegeRecallStatics::Cleared(), Epoch + 5.0, Duration), 0.f, Exact);

	// ── (e) A clock handed back in time yields 0, never a negative that reads as complete ──
	TestEqual(TEXT("(e) A clock earlier than the channel's start reports 0 elapsed, not a negative"),
		FSiegeRecallStatics::ElapsedSeconds(Channel, Epoch - 100.0), 0.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⛔ R-4 — A RE-PRESS RESTARTS FROM ZERO. "start it from the beginning."
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallRestartsFromZeroTest,
	"Siegebound.Recall.RepressRestartsFromZeroAndNeverResumesPartialProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallRestartsFromZeroTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	const float Duration = ChannelSecondsFromHisSentence;

	// A first channel gets to 9.9 s — as close to home as it is possible to be.
	const FSiegeRecallState First = RunningChannel();
	const double RestartClock = Epoch + 9.9;

	TestEqual(TEXT("(setup) The first channel really did reach 0.99 progress — otherwise the restart claim below would be vacuous"),
		FSiegeRecallStatics::Progress01(First, RestartClock, Duration), 0.99f, Tolerance);

	// The player presses the key again. It cancels, and the NEXT press begins a fresh channel.
	const FSiegeRecallState Second = FSiegeRecallStatics::Begin(RestartClock, FVector::ZeroVector);

	// ── (a) ⭐ ZERO. Not 0.99, not "carried over", not "a bit quicker this time" ──────────
	// ⛔ THIS ROW IS THE RULING AND IT FAILS AGAINST EVERY RESUME IMPLEMENTATION: a Begin that
	// preserved the earlier StartTimeSeconds would report 0.99 here.
	TestEqual(TEXT("(a) ⭐ A re-pressed channel starts at EXACTLY 0 progress — 9.9 s of prior channelling is worth nothing"),
		FSiegeRecallStatics::Progress01(Second, RestartClock, Duration), 0.f, Exact);

	TestEqual(TEXT("(a) …and 0 elapsed"),
		FSiegeRecallStatics::ElapsedSeconds(Second, RestartClock), 0.f, Exact);

	// ── (b) It needs the FULL duration again, measured from the re-press ──────────────────
	// The strongest form of the claim: at the moment the FIRST channel would have completed,
	// the restarted one is still 9.9 s from home.
	TestFalse(TEXT("(b) ⛔ At the instant the FIRST channel would have finished, the restarted one is NOT complete"),
		FSiegeRecallStatics::IsComplete(Second, Epoch + 10.0, Duration));

	TestFalse(TEXT("(b) …and still not complete a tenth of a second before its own full duration"),
		FSiegeRecallStatics::IsComplete(Second, RestartClock + 9.9, Duration));

	TestTrue(TEXT("(b) It completes exactly one full channel after the RE-PRESS, and not before"),
		FSiegeRecallStatics::IsComplete(Second, RestartClock + 10.0, Duration));

	// ── (c) The restart also re-anchors, so the cancel-by-movement datum is not stale ─────
	// ⚠️ Without this a player who walked 900 uu between the two presses would be cancelled on
	// the restarted channel's very first frame, for a move they made before it existed.
	const FVector NewSpot(900.f, -400.f, 30.f);
	const FSiegeRecallState Relocated = FSiegeRecallStatics::Begin(RestartClock, NewSpot);

	TestFalse(TEXT("(c) A channel restarted somewhere else anchors THERE — it is not instantly cancelled by the walk that preceded it"),
		FSiegeRecallStatics::HasLeftAnchor(Relocated, NewSpot, 25.f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ⛔ R-1 — ONLY DAMAGE THAT LANDS INTERRUPTS
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallOnlyLandedDamageInterruptsTest,
	"Siegebound.Recall.OnlyDamageThatActuallyLandedInterruptsTheChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallOnlyLandedDamageInterruptsTest::RunTest(const FString& Parameters)
{
	// ── (a) Nothing landed ⇒ the channel survives ────────────────────────────────────────
	// ⚠️ These are the REAL shipped shapes, not hypotheticals: friendly fire returns 0, a hit on
	// an already-dead hero returns 0, and a fully-mitigated hit returns 0. A rule keyed on
	// "an attack happened" rather than "damage landed" would cancel the player's recall every
	// time an ally brushed him.
	const float NonLandingAmounts[] = { 0.f, -0.0001f, -1.f, -500.f };
	for (const float Amount : NonLandingAmounts)
	{
		TestFalse(*FString::Printf(TEXT("(a) ⛔ An applied amount of %.4f does NOT interrupt — nothing landed"), Amount),
			FSiegeRecallStatics::DamageInterrupts(Amount));
	}

	// ── (b) Anything at all landed ⇒ interrupted ─────────────────────────────────────────
	// ⭐ The boundary is open on the low side: one tenth of one hit point is being hit. This row
	// fails against a threshold implementation ("only interrupt on >= 5 damage"), which is a
	// mechanic nobody asked for and which would let chip damage be ignored.
	const float LandingAmounts[] = { 0.0001f, 0.1f, 1.f, 20.f, 999.f };
	for (const float Amount : LandingAmounts)
	{
		TestTrue(*FString::Printf(TEXT("(b) An applied amount of %.4f DOES interrupt — that is being hit"), Amount),
			FSiegeRecallStatics::DamageInterrupts(Amount));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. ⭐⭐ THE DISARM IS TRANSIENT STATE, ⛔ NOT A PERMANENT IDENTITY SEAL
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallDisarmIsTransientTest,
	"Siegebound.Recall.TheAttackDisarmIsTransientStateAndNotAPermanentIdentitySeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallDisarmIsTransientTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	// ── (a) ⭐⭐ THE SAME PREDICATE ANSWERS BOTH WAYS, DECIDED PURELY BY STATE ─────────────
	// ⛔ THIS IS THE DISTINCTION THE TOWER WAVE PAID FOR: a `const` class-identity query ("can
	// this kind of thing ever attack") is a CONSTANT for a given class and cannot produce these
	// two answers. A disarm expressed that way makes a 10-second channel indistinguishable from
	// a unit that can never attack for as long as it exists.
	TestTrue(TEXT("(a) ⭐ A hero with a channel running IS disarmed"),
		FSiegeRecallStatics::IsAttackDisarmed(RunningChannel()));

	TestFalse(TEXT("(a) ⭐ …and the SAME predicate says the SAME hero is armed the moment the channel is cleared"),
		FSiegeRecallStatics::IsAttackDisarmed(FSiegeRecallStatics::Cleared()));

	// ── (b) A freshly-constructed hero is ARMED ──────────────────────────────────────────
	// A hero that shipped disarmed would be a permanent seal wearing the state's clothes. The
	// CDO is the cheapest possible witness that the default is "can attack".
	{
		const AHeroCharacter* const Defaults = GetDefault<AHeroCharacter>();
		if (!TestNotNull(TEXT("The AHeroCharacter class default object resolves"), Defaults))
		{
			return false;
		}

		TestFalse(TEXT("(b) ⛔ A hero is NOT channelling by default — the disarm is something that happens to it, not something it is"),
			Defaults->IsRecalling());
	}

	// ── (c) ⛔ NO PERMANENT SEAL WAS ADDED TO THE CLASS ───────────────────────────────────
	// ⚠️ The positive control comes first, otherwise all three null checks would pass vacuously
	// against a typo'd class pointer.
	TestNotNull(TEXT("(c) DoMeleeAttack is still a UFUNCTION on the hero (positive control — the checks below are not vacuous)"),
		AHeroCharacter::StaticClass()->FindFunctionByName(TEXT("DoMeleeAttack")));

	TestNull(TEXT("(c) ⛔ The hero gained NO CanEverAttack identity query"),
		AHeroCharacter::StaticClass()->FindFunctionByName(TEXT("CanEverAttack")));

	TestNull(TEXT("(c) ⛔ …and no bCanEverAttack flag"),
		FindFProperty<FBoolProperty>(AHeroCharacter::StaticClass(), TEXT("bCanEverAttack")));

	TestNull(TEXT("(c) ⛔ …and no bCanAttack flag"),
		FindFProperty<FBoolProperty>(AHeroCharacter::StaticClass(), TEXT("bCanAttack")));

	// ── (d) ⛔ AND NO NEW SUPPRESSION MECHANISM WAS INVENTED ──────────────────────────────
	// The shipped placement-mode suppression must still be the ONLY suppression API on the
	// hero; the recall disarm rides the existing melee guard instead of adding a second lever.
	TestNotNull(TEXT("(d) The shipped SetMeleeSuppressed is untouched (positive control)"),
		AHeroCharacter::StaticClass()->FindFunctionByName(TEXT("SetMeleeSuppressed")));

	TestNull(TEXT("(d) ⛔ No parallel SetRecallSuppressed was added — the disarm is a state term at the EXISTING guard"),
		AHeroCharacter::StaticClass()->FindFunctionByName(TEXT("SetRecallSuppressed")));

	TestNull(TEXT("(d) ⛔ …and no bRecallSuppressed flag either"),
		FindFProperty<FBoolProperty>(AHeroCharacter::StaticClass(), TEXT("bRecallSuppressed")));

	// ⛔ WHAT THIS TEST DELIBERATELY DOES NOT CLAIM: that `DoMeleeAttack` actually consults the
	// predicate. That call needs a live hero in a world and is TASK-753's diff read (gate item:
	// the disarm rides the SHIPPED attack entry point). ⚖️ Asserting it here would require a
	// mock of the guard, and a mock of the guard proves only that the mock works.

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⛔ R-2 — MOVING CANCELS, AND IDLE JITTER DOES NOT
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallMovementCancelTest,
	"Siegebound.Recall.LeavingTheAnchorCancelsAndStandingStillDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallMovementCancelTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	const FVector Anchor(1000.f, -2000.f, 300.f);
	const FSiegeRecallState Channel = RunningChannel(Anchor);
	const float ToleranceUU = 25.f;

	// ── (a) Standing exactly still, and drifting inside the tolerance, do NOT cancel ──────
	TestFalse(TEXT("(a) A hero that has not moved at all keeps channelling"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor, ToleranceUU));

	TestFalse(TEXT("(a) ⭐ A 24 uu settle inside a 25 uu tolerance keeps channelling — capsule jitter is not a walk"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor + FVector(24.f, 0.f, 0.f), ToleranceUU));

	// ── (b) Leaving the tolerance cancels, on any axis ────────────────────────────────────
	// ⚠️ The Z row is load-bearing and is a deliberate design choice, not an accident of the
	// maths: falling off the ledge you were standing on IS leaving the spot, and a channel that
	// survived the fall would arrive from somewhere the player never chose. It fails against a
	// horizontal-only distance, which is the shape this would most plausibly be written as.
	TestTrue(TEXT("(b) A 26 uu step along X cancels"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor + FVector(26.f, 0.f, 0.f), ToleranceUU));

	TestTrue(TEXT("(b) A 26 uu step along Y cancels"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor + FVector(0.f, 26.f, 0.f), ToleranceUU));

	TestTrue(TEXT("(b) ⭐ A 26 uu drop along Z cancels too — the rule is 3D, and falling is moving"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor + FVector(0.f, 0.f, -26.f), ToleranceUU));

	TestTrue(TEXT("(b) A full walking step cancels"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor + FVector(500.f, 0.f, 0.f), ToleranceUU));

	// ── (c) ⛔ A cleared channel is never cancelled by movement ───────────────────────────
	// Without this the predicate would report "cancel me" for every walking hero in the level.
	TestFalse(TEXT("(c) ⛔ With no channel running, moving 10,000 uu cancels nothing"),
		FSiegeRecallStatics::HasLeftAnchor(FSiegeRecallStatics::Cleared(), Anchor + FVector(10000.f, 0.f, 0.f), ToleranceUU));

	// ── (d) A negative tolerance is clamped, not trusted ─────────────────────────────────
	// It would otherwise make the comparison true for every position including the anchor, and
	// cancel every channel on its first frame.
	TestFalse(TEXT("(d) A negative tolerance still does not cancel a hero standing exactly on its anchor"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor, -50.f));

	TestTrue(TEXT("(d) …while a real 1 uu move under a clamped-to-zero tolerance does cancel"),
		FSiegeRecallStatics::HasLeftAnchor(Channel, Anchor + FVector(1.f, 0.f, 0.f), -50.f));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. ⭐ THE SEVEN EXITS ARE A CLOSED LIST, AND EXACTLY ONE OF THEM GRANTS
//     THE ARRIVAL
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallExitsAreClosedAndOnlyOneGrantsTest,
	"Siegebound.Recall.SevenExitsExistAndExactlyOneOfThemGrantsTheArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallExitsAreClosedAndOnlyOneGrantsTest::RunTest(const FString& Parameters)
{
	// ── (a) The enum still has exactly the seven exits the law enumerates ────────────────
	// ⭐ THIS ROW EXISTS TO GO RED WHEN AN EIGHTH EXIT IS ADDED. `TOWER-§8`'s hanging-unit
	// lesson generalises: a timed state whose exits are not enumerated will strand the player in
	// one of them, so a new exit must force a decision about whether it grants an arrival rather
	// than inheriting one silently.
	const UEnum* const ExitEnum = StaticEnum<ESiegeRecallExit>();
	if (!TestNotNull(TEXT("ESiegeRecallExit is a reflected UENUM"), ExitEnum))
	{
		return false;
	}

	const int32 ExitCount = ExitEnum->NumEnums() - 1; // NumEnums() includes the hidden _MAX entry

	TestEqual(TEXT("(a) ⭐ There are EXACTLY seven enumerated recall exits — add one and this row fails until the law and the handoff are updated"),
		ExitCount, 7);

	// Collected by index rather than looked up by name: GetNameStringByIndex returns the SHORT
	// name for a scoped enum, so this comparison is against the same text the header declares.
	TArray<FString> ShippedExitNames;
	for (int32 Index = 0; Index < ExitCount; ++Index)
	{
		ShippedExitNames.Add(ExitEnum->GetNameStringByIndex(Index));
	}

	const TCHAR* const ExpectedExitNames[] = {
		TEXT("Completed"), TEXT("CancelledByInput"), TEXT("CancelledByMovement"),
		TEXT("InterruptedByDamage"), TEXT("InterruptedByDeath"),
		TEXT("CancelledByMatchEnd"), TEXT("CancelledByEndPlay")
	};

	for (const TCHAR* const ExpectedName : ExpectedExitNames)
	{
		TestTrue(*FString::Printf(TEXT("(a) The '%s' exit is still named on the enum"), ExpectedName),
			ShippedExitNames.Contains(FString(ExpectedName)));
	}

	// ── (b) ⭐ Sweep EVERY exit against BOTH binding states: exactly ONE combination grants ──
	// A 14-row sweep rather than seven hand-written lines, so an eighth exit is swept
	// automatically and a change that made a cancel "helpfully" heal is caught by the count.
	int32 GrantingCombinations = 0;
	for (int32 Index = 0; Index < ExitCount; ++Index)
	{
		const ESiegeRecallExit Exit = static_cast<ESiegeRecallExit>(ExitEnum->GetValueByIndex(Index));

		for (const bool bBound : { true, false })
		{
			if (FSiegeRecallStatics::ExitGrantsArrival(Exit, bBound))
			{
				++GrantingCombinations;

				TestEqual(TEXT("(b) ⛔ The ONLY granting exit is Completed"),
					static_cast<int32>(Exit), static_cast<int32>(ESiegeRecallExit::Completed));

				TestTrue(TEXT("(b) ⛔ …and it only grants with a destination owner bound"), bBound);
			}
		}
	}

	TestEqual(TEXT("(b) ⭐ EXACTLY ONE of the fourteen (exit × binding) combinations grants an arrival"),
		GrantingCombinations, 1);

	// ── (c) The six non-completion exits leave the hero untouched ────────────────────────
	// ⭐ THIS IS THE "interrupted at 9.9 s leaves the hero in place at unchanged HP" claim, in
	// the only form a headless suite can make it: the payload that would have moved and healed
	// the hero carries nothing.
	for (int32 Index = 0; Index < ExitCount; ++Index)
	{
		const ESiegeRecallExit Exit = static_cast<ESiegeRecallExit>(ExitEnum->GetValueByIndex(Index));
		if (Exit == ESiegeRecallExit::Completed)
		{
			continue;
		}

		const FSiegeRecallArrival Arrival = FSiegeRecallStatics::BuildArrival(Exit, /*bDestinationOwnerBound=*/ true, 400.f);

		TestEqual(*FString::Printf(TEXT("(c) Exit '%s' heals NOTHING — the hero keeps the hit points it had"),
			*ExitEnum->GetNameStringByIndex(Index)), Arrival.HealTargetHP, 0.f, SiegeRecallTestFixture::Exact);

		TestFalse(*FString::Printf(TEXT("(c) Exit '%s' teleports NOWHERE — the hero stays exactly where it stands"),
			*ExitEnum->GetNameStringByIndex(Index)), Arrival.bTeleportHome);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⛔⛔ TRAP TWO — THE HEAL IS THE EFFECTIVE MAXIMUM, AND A PLATE-ARMOR HERO
//     IS 200 HP WORSE OFF IF IT IS NOT
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallHealIsTheEffectiveMaximumTest,
	"Siegebound.Recall.ArrivalHealsToTheEffectiveMaximumWithPlateArmorStacksApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallHealIsTheEffectiveMaximumTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	// ⭐ RE-DERIVED FROM THE SHIPPED TUNABLES, ⛔ NOT TRANSCRIBED FROM THE LAW'S PROSE. The law
	// states the damage as "up to 200 HP" and derives it as "base 200 + bonus 100 × 2 stacks";
	// this test reads BOTH numbers off the hero CDO and multiplies them itself, so a retune of
	// either one moves the expectation with it instead of leaving a stale literal behind.
	float BaseMaximum = 0.f;
	float BonusPerStack = 0.f;
	if (!ReadHeroFloat(*this, TEXT("MaxHP"), BaseMaximum) ||
		!ReadHeroFloat(*this, TEXT("MaxHPBonus"), BonusPerStack))
	{
		return false;
	}

	const float EffectiveMaximumAtTwoStacks = BaseMaximum + (BonusPerStack * PlateArmorStacksInTheLawsExample);
	const float UnderHealIfTheBaseWereRead = EffectiveMaximumAtTwoStacks - BaseMaximum;

	// ── (a) The trap is real: the two readings genuinely diverge ─────────────────────────
	// ⚠️ Without this row the assertions below could pass on a hero whose bonus was 0, where a
	// base read and an effective read agree and the whole trap is invisible.
	TestTrue(TEXT("(a) The Plate-Armor bonus is non-zero, so a base read and an effective read really do diverge"),
		BonusPerStack > 0.f);

	TestTrue(TEXT("(a) ⭐ …and the divergence at two stacks is a substantial share of the hero's whole health bar, not a rounding difference"),
		UnderHealIfTheBaseWereRead >= BaseMaximum * 0.5f);

	// ── (b) A completed arrival carries the FULL effective maximum ───────────────────────
	const FSiegeRecallArrival PlatedArrival =
		FSiegeRecallStatics::BuildArrival(ESiegeRecallExit::Completed, /*bDestinationOwnerBound=*/ true, EffectiveMaximumAtTwoStacks);

	TestEqual(TEXT("(b) ⭐ A two-stack Plate-Armor hero arrives at its FULL effective maximum"),
		PlatedArrival.HealTargetHP, EffectiveMaximumAtTwoStacks, Exact);

	TestTrue(TEXT("(b) ⛔ …which is strictly MORE than the raw base maximum — a base read would silently under-heal him by exactly that gap"),
		PlatedArrival.HealTargetHP > BaseMaximum);

	TestEqual(TEXT("(b) ⛔ …and the gap is the law's figure, re-derived from the shipped bonus rather than copied from its prose"),
		PlatedArrival.HealTargetHP - BaseMaximum, UnderHealIfTheBaseWereRead, Exact);

	// ── (c) An un-upgraded hero arrives at exactly the base maximum ──────────────────────
	// The other end of the same claim: the arrival does not over-heal a hero who bought nothing.
	const FSiegeRecallArrival PlainArrival =
		FSiegeRecallStatics::BuildArrival(ESiegeRecallExit::Completed, /*bDestinationOwnerBound=*/ true, BaseMaximum);

	TestEqual(TEXT("(c) A hero with no upgrades arrives at exactly the base maximum — no over-heal either"),
		PlainArrival.HealTargetHP, BaseMaximum, Exact);

	// ── (d) The arrival really does ask for the teleport ─────────────────────────────────
	TestTrue(TEXT("(d) A completed arrival asks the destination owner for the teleport"),
		PlatedArrival.bTeleportHome);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ⭐ THE ARRIVAL IS ATOMIC — NO DESTINATION OWNER MEANS NO TELEPORT
//     AND THEREFORE NO HEAL
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallArrivalIsAtomicTest,
	"Siegebound.Recall.WithNoDestinationOwnerBoundThereIsNoTeleportAndNoHeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallArrivalIsAtomicTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	// ⚖️ THE DECLARED RULING THIS PINS: a heal without a teleport would be a free full refill
	// from anywhere on the map, handed out by a WIRING GAP rather than by a decision. A feature
	// that is inert until it is integrated is a visible bug; one that half-fires into an exploit
	// is an invisible one, so the two effects stand or fall together.
	const FSiegeRecallArrival Unbound =
		FSiegeRecallStatics::BuildArrival(ESiegeRecallExit::Completed, /*bDestinationOwnerBound=*/ false, 400.f);

	TestFalse(TEXT("(a) ⛔ With nothing bound, a completed channel grants no arrival"),
		FSiegeRecallStatics::ExitGrantsArrival(ESiegeRecallExit::Completed, /*bDestinationOwnerBound=*/ false));

	TestEqual(TEXT("(a) ⛔ …so the hero is NOT healed — a full refill would otherwise be free"),
		Unbound.HealTargetHP, 0.f, Exact);

	TestFalse(TEXT("(a) ⛔ …and is not moved"),
		Unbound.bTeleportHome);

	// The positive control: the SAME exit with an owner bound does grant, so the row above is
	// about the binding and not about a function that never grants anything.
	TestTrue(TEXT("(b) The same exit WITH an owner bound does grant (positive control)"),
		FSiegeRecallStatics::ExitGrantsArrival(ESiegeRecallExit::Completed, /*bDestinationOwnerBound=*/ true));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  10. ⛔ CanBegin — DEAD, ALREADY CHANNELLING, OR MATCH OVER
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallCanBeginTest,
	"Siegebound.Recall.AChannelStartsOnlyForALivingHeroInARunningMatchWithNoChannelUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallCanBeginTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	const FSiegeRecallState Idle = FSiegeRecallStatics::Cleared();
	const FSiegeRecallState Running = RunningChannel();

	// ── The full 2×2×2 truth table. Each of the three terms is dropped by a different row, so
	//    deleting any one of them from the implementation turns this test red.
	TestTrue(TEXT("(a) Alive, idle, match running ⇒ a channel may start — the ONLY true row"),
		FSiegeRecallStatics::CanBegin(Idle, /*bDead=*/ false, /*bMatchEnded=*/ false));

	TestFalse(TEXT("(b) ⛔ A DEAD hero may not start a channel — the dead have no abilities, and the respawn path owns the corpse"),
		FSiegeRecallStatics::CanBegin(Idle, /*bDead=*/ true, /*bMatchEnded=*/ false));

	TestFalse(TEXT("(c) ⛔ A hero ALREADY channelling may not stack a second one — a re-press is a CANCEL, never a restack"),
		FSiegeRecallStatics::CanBegin(Running, /*bDead=*/ false, /*bMatchEnded=*/ false));

	TestFalse(TEXT("(d) ⛔ A DECIDED match may not start a channel — nothing teleports or heals under the end screen"),
		FSiegeRecallStatics::CanBegin(Idle, /*bDead=*/ false, /*bMatchEnded=*/ true));

	TestFalse(TEXT("(e) ⛔ …and every combination of two or three blockers stays refused"),
		FSiegeRecallStatics::CanBegin(Running, /*bDead=*/ true, /*bMatchEnded=*/ false) ||
		FSiegeRecallStatics::CanBegin(Running, /*bDead=*/ false, /*bMatchEnded=*/ true) ||
		FSiegeRecallStatics::CanBegin(Idle, /*bDead=*/ true, /*bMatchEnded=*/ true) ||
		FSiegeRecallStatics::CanBegin(Running, /*bDead=*/ true, /*bMatchEnded=*/ true));

	// ⛔ WHAT THIS DOES NOT CLAIM, NAMED RATHER THAN FAKED: that `BeginRecall` consults this
	// predicate with the hero's real death latch and the real game mode's match latch. That
	// needs a world and is TASK-753's diff read.

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  11. ⭐ THE ARRIVAL PAYLOAD CARRIES EXACTLY TWO EFFECTS AND CANNOT CARRY
//      THE DEATH PATH'S FIVE
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallArrivalPayloadIsClosedTest,
	"Siegebound.Recall.TheArrivalPayloadExpressesExactlyTwoEffectsAndNothingElse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallArrivalPayloadIsClosedTest::RunTest(const FString& Parameters)
{
	// ⭐ `RECALL-§1`: "Recall performs exactly two effects: the teleport, and the heal." The
	// death-path restore function does FIVE further things — re-applies the cumulative upgrade
	// mods, re-arms the aura, resets the Rally cooldown, restores input and re-broadcasts the
	// loadout. ⛔ NONE of them is expressible in this payload, and this test goes red the day a
	// third field appears, which is the day somebody starts widening the arrival.
	const UScriptStruct* const ArrivalStruct = FSiegeRecallArrival::StaticStruct();
	if (!TestNotNull(TEXT("FSiegeRecallArrival is a reflected USTRUCT"), ArrivalStruct))
	{
		return false;
	}

	int32 PropertyCount = 0;
	bool bFoundHealTarget = false;
	bool bFoundTeleportFlag = false;

	for (TFieldIterator<FProperty> It(ArrivalStruct); It; ++It)
	{
		++PropertyCount;
		const FName PropertyName = It->GetFName();
		bFoundHealTarget = bFoundHealTarget || PropertyName == FName(TEXT("HealTargetHP"));
		bFoundTeleportFlag = bFoundTeleportFlag || PropertyName == FName(TEXT("bTeleportHome"));
	}

	TestEqual(TEXT("(a) ⭐ The arrival payload has EXACTLY two properties — a heal target and a teleport flag"),
		PropertyCount, 2);

	TestTrue(TEXT("(a) …the heal target is one of them"), bFoundHealTarget);
	TestTrue(TEXT("(a) …and the teleport flag is the other"), bFoundTeleportFlag);

	// ── (b) ⛔ The hero did not learn the destination ─────────────────────────────────────
	// ⭐ The destination belongs to the owner of the shipped teleport-home rule (the team-keyed
	// PlayerStart, else beside the hero's own castle). "hero spawns OUTSIDE the keep" is the
	// recorded cost of a second, hand-typed destination rule, so the hero must carry no
	// location, no offset and no castle reference of its own for this feature.
	TestNotNull(TEXT("(b) The hero DOES expose the arrival delegate — the seam the destination owner binds (positive control)"),
		FindFProperty<FProperty>(AHeroCharacter::StaticClass(), TEXT("OnHeroRecallArrived")));

	const TCHAR* const ForbiddenDestinationMembers[] = {
		TEXT("RecallDestination"), TEXT("RecallTargetLocation"), TEXT("RecallHomeLocation"),
		TEXT("RecallCastleOffset"), TEXT("RecallTeleportOffset")
	};

	for (const TCHAR* const MemberName : ForbiddenDestinationMembers)
	{
		TestNull(*FString::Printf(TEXT("(b) ⛔ The hero carries no '%s' — the destination is resolved by its owner, never hand-typed here"), MemberName),
			FindFProperty<FProperty>(AHeroCharacter::StaticClass(), MemberName));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  12. ⛔⛔ THE SOURCE SCAN — THE TWO TRAPS, ASSERTED RATHER THAN HOPED FOR,
//      PLUS THE `Escape` LAW
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRecallSourceScanTest,
	"Siegebound.Recall.TheShippedRecallPathCallsNoDeathPathRestoreAndReadsTheEffectiveMaximum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRecallSourceScanTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRecallTestFixture;

	// ⚖️ WHY THIS TEST IS A SOURCE SCAN AND NOT A BEHAVIOURAL ASSERTION, SAID PLAINLY: both of
	// `RECALL-§1`'s traps are about a call that must NEVER be made and a field that must NEVER
	// be read. Neither is observable from a payload, and neither is reachable headlessly — the
	// paths that would expose them need a live hero in a world. TASK-753's gate requires both
	// "asserted by a test, ⛔ not merely absent", so the shipped text is the subject.
	FString Implementation;
	FString Header;
	if (!LoadHeroSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp"), Implementation) ||
		!LoadHeroSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h"), Header))
	{
		return false;
	}

	// ── (a) THE SELF-CHECK. A probe that has gone stale must FAIL, never report SAFE ──────
	const FString BeginSentinel(TEXT("RECALL REGION BEGIN"));
	const FString EndSentinel(TEXT("RECALL REGION END"));

	const int32 BeginCount = CountOccurrences(Implementation, *BeginSentinel);
	const int32 EndCount = CountOccurrences(Implementation, *EndSentinel);

	if (BeginCount != 1 || EndCount != 1)
	{
		AddError(FString::Printf(
			TEXT("⛔ HeroCharacter.cpp must carry EXACTLY ONE '%s' and ONE '%s' sentinel (found %d and %d). This probe cannot locate the recall path without them, and a probe that cannot see its subject is an ERROR, not a pass."),
			*BeginSentinel, *EndSentinel, BeginCount, EndCount));
		return false;
	}

	const int32 RegionStart = Implementation.Find(*BeginSentinel, ESearchCase::CaseSensitive) + BeginSentinel.Len();
	const int32 RegionEnd = Implementation.Find(*EndSentinel, ESearchCase::CaseSensitive);

	if (RegionEnd <= RegionStart)
	{
		AddError(TEXT("⛔ The recall region's END sentinel precedes its BEGIN sentinel — the region cannot be read."));
		return false;
	}

	const FString Region = Implementation.Mid(RegionStart, RegionEnd - RegionStart);

	TestTrue(TEXT("(a) The recall region is substantial — a region that had shrunk to nothing would make every claim below vacuous"),
		Region.Len() > 2000);

	// ── (b) ⛔⛔ TRAP ONE — the death-path restore function is NOWHERE on the recall path ──
	// ⚠️ It re-applies the hero's CUMULATIVE upgrade mods onto a freshly-restored base and
	// re-arms the War Banner aura. On a LIVE hero that double-applies every upgrade stack and
	// re-arms a running aura — silently, on every recall, and it reviews as sensible reuse.
	const TCHAR* const DeathPathRestoreName = TEXT("ResetHero");

	const int32 RestoreInWholeFile = CountOccurrences(Implementation, DeathPathRestoreName);
	TestTrue(TEXT("(b) POSITIVE CONTROL: the death-path restore function is still present in this file under the name this probe searches for — otherwise the claim below would pass against a typo"),
		RestoreInWholeFile >= 2);

	TestEqual(TEXT("(b) ⛔⛔ The death-path restore function appears ZERO times inside the recall region — recall does the teleport and the heal, and nothing else"),
		CountOccurrences(Region, DeathPathRestoreName), 0);

	// ── (c) ⛔⛔ TRAP TWO — the heal reads the EFFECTIVE maximum, never the raw base ───────
	// The identity asserted: every occurrence of the base field's name inside the region is part
	// of `GetEffectiveMaxHP` or of `GetMaxHP` (which itself returns the effective value). A bare
	// read of the base field — or of the Plate-Armor bonus — breaks the equality and fails here.
	const int32 BaseFieldMentions = CountOccurrences(Region, TEXT("MaxHP"));
	const int32 EffectiveMentions = CountOccurrences(Region, TEXT("EffectiveMaxHP"));
	const int32 PublicGetterMentions = CountOccurrences(Region, TEXT("GetMaxHP"));

	TestTrue(TEXT("(c) POSITIVE CONTROL: the recall region really does call GetEffectiveMaxHP — the heal has a source"),
		CountOccurrences(Region, TEXT("GetEffectiveMaxHP()")) >= 1);

	TestEqual(TEXT("(c) ⛔⛔ EVERY mention of the base hit-point field inside the recall region is part of an EFFECTIVE read — there is no bare base read to under-heal a Plate-Armor hero by 200"),
		BaseFieldMentions, EffectiveMentions + PublicGetterMentions);

	// ── (d) ⛔⛔ THE `Escape` LAW — the channel adds NO key handler at all ────────────────
	// `AS-§6` A-2 is a closed Jonathan ruling and consuming Escape is an automatic fail. The
	// shipped cancel routes (placement, spell targeting, group pick) must keep firing
	// byte-identically while a channel runs, which they can only do if nothing here intercepts.
	const TCHAR* const ForbiddenInputInterceptors[] = {
		TEXT("NativeOnKeyDown"), TEXT("NativeOnPreviewKeyDown"), TEXT("FReply"),
		TEXT("SetInputMode"), TEXT("bShowMouseCursor")
	};

	for (const TCHAR* const Forbidden : ForbiddenInputInterceptors)
	{
		TestEqual(*FString::Printf(TEXT("(d) ⛔ '%s' appears nowhere in HeroCharacter.cpp — the channel intercepts no raw key and owns no cursor state"), Forbidden),
			CountOccurrences(Implementation, Forbidden), 0);

		TestEqual(*FString::Printf(TEXT("(d) ⛔ …nor does '%s' appear in HeroCharacter.h"), Forbidden),
			CountOccurrences(Header, Forbidden), 0);
	}

	// ── (e) ⛔ NO HARD-CODED KEY — `B` follows the layout system or it follows nothing ────
	// ⚠️ `KBD-§4` tables all 26 letters, so the recall key inherits Dvorak support through
	// IMC_Hero's positional remap with zero extra code. A literal key constant here would be
	// wrong ONLY on a non-QWERTY layout — invisible to every reviewer and immediately visible to
	// Jonathan, who uses one.
	TestEqual(TEXT("(e) ⛔ No engine key constant is referenced in HeroCharacter.cpp — the key is never named in code"),
		CountOccurrences(Implementation, TEXT("EKeys::")), 0);

	TestEqual(TEXT("(e) ⛔ …nor in HeroCharacter.h"),
		CountOccurrences(Header, TEXT("EKeys::")), 0);

	// ── (f) ⛔ The double-translate trap is not walked into ───────────────────────────────
	// The positional-key API is for RAW POLLED keys. IA_Recall runs through IMC_Hero, whose
	// `.Key` fields are ALREADY remapped by the subsystem, so calling it again would translate
	// twice (`SiegePlayerController.h:1223-1225`).
	TestEqual(TEXT("(f) ⛔ The single-key positional lookup is never called from HeroCharacter.cpp — its IMC is already remapped, and a second translation would double-shift the key"),
		CountOccurrences(Implementation, TEXT("GetPositionalKey")), 0);

	TestEqual(TEXT("(f) ⛔ …nor from HeroCharacter.h"),
		CountOccurrences(Header, TEXT("GetPositionalKey")), 0);

	// The positive control for (f): the hero DOES go through the whole-CONTEXT remap, which is
	// the correct lane. Without this row, (f) would also pass on a hero that had no layout
	// support at all — which is the other way to get Dvorak wrong.
	TestTrue(TEXT("(f) POSITIVE CONTROL: the hero still routes IMC_Hero through the layout subsystem's whole-context remap"),
		CountOccurrences(Implementation, TEXT("GetPositionalContext")) >= 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
