// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "HAL/UnrealMemory.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Siegebound/SiegeStuckStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the stuck-unit ladder (UNIT-PATHING batch, TASK-536) ═══
 *  Subject: TASK-531's FSiegeStuckStatics. QA gate: TASK-537. Compile + run: TASK-538.
 *
 *  ⭐ THE PROPERTY THAT MAKES THIS FILE POSSIBLE: FSiegeStuckStatics IS TOTALLY PURE.
 *  No UWorld, no AActor, no UObject, no allocation, no clock read, no RNG. Every input is a
 *  parameter and every output is a return value or a write to the caller's FSiegeStuckState.
 *  ⇒ The whole ladder runs in-process with NO PIE session and NO navmesh. If a test here ever
 *  starts needing a world, the purity that `NAV-§3` calls load-bearing has been broken, and
 *  that is a FINDING, not a reason to add a fixture.
 *
 *  ⛔⛔ WHAT THESE TESTS DO **NOT** COVER — STATED HERE SO NOBODY MISTAKES GREEN FOR DONE
 *  (`SC-§32`: a mechanism never observed to function is not known to function). Every item
 *  below needs a world, a pawn or a navmesh and is therefore UNREACHABLE from this file:
 *    • ASummonedUnit::TickStuckWatchdog's call site, its bAdvancing read (GetMoveStatus()),
 *      ConsumeStuckDeltaSeconds' world-clock delta and its MaxStuckDeltaSeconds clamp;
 *    • the SIDESTEP LEASE in full — arming, draining, and all five clear sites
 *      (EnterIdle / EnterAttack / HandleDeath / FreezeAI / ApplyFreeze);
 *    • every rung's ACTION (EnterAdvanceToLocation, the three latch invalidations, EnterIdle,
 *      DriveToPoint / EnsureWalkingToNode / StandInPlace / SeekBestMine on the miner);
 *    • ASiegeUnitAIController::OnMoveCompleted, its Blocked filter, its 10 s log throttle and
 *      ASummonedUnit::NotifyMoveBlocked itself (test 20 models its two WRITES; it cannot call it);
 *    • every BattlefieldScatter / FSiegeNavDiagnostics claim — the telemetry tokens, the
 *      settled-only cull, bCullOnProvisionalFailure, the generation-finished delegate;
 *    • the whole of TASK-530: whether the ini flip reaches L_Arena's serialized nav actor.
 *  ⚖️ A full green run here means THE LADDER'S ARITHMETIC IS RIGHT. The batch's real gate is
 *  TASK-538's PIE log and TASK-539 — Jonathan's own playtest.
 *
 *  ⚠️ TWO KNOWN HOLES REMAIN **PINNED AS SHIPPED**, NOT PAPERED OVER. Tests 18 and 20 assert
 *  the shipped behaviour and the handoff (`handoffs/TASK-536-programmer.md`) states the risk;
 *  ⛔ this file may not "fix" a pinned hole on its own authority — TASK-537 rules.
 *
 *  ⭐ THE THIRD (hole 2) WAS RULED A **BLOCKER** BY TASK-537 AND IS FIXED IN SHIPPED SOURCE AS OF
 *  LOOP 1: ASummonedUnit::SidestepAttemptCount (SummonedUnit.h) is a free-running per-unit
 *  counter, deliberately OUTSIDE FSiegeStuckState because Reset clears that struct between
 *  stalls, so the sidestep side now genuinely alternates across successive stalls. ⛔ TEST 19 WAS
 *  **INVERTED** IN THE SAME LOOP AND RENAMED — it was `Siegebound.Nav.Stuck.
 *  SidestepSideIsConstantForOneUnit` (which asserted the hole) and is now
 *  `Siegebound.Nav.Stuck.SidestepSideAlternatesAcrossStallsForOneUnit`.
 *
 *  `SC-§13` NOTE: this file makes NO FString claim of any kind — the library under test has no
 *  string surface. Every string below is message text passed to a `What` parameter, never an
 *  asserted value, so there is no TestEqual-on-FString case-sensitivity hazard here.
 *
 *  M8 DECLARATION (verbatim, `NAV-§11`): adds no replicated property, no new replicated class,
 *  no new relevancy tier. This is a test file; it adds no shipped surface at all.
 */

namespace SiegeStuckTestUtils
{
	/**
	 *  The shipped poll period. ⚠️ NOT read from StateCheckInterval — AMinerUnit seals that to 0
	 *  (MinerUnit.cpp:61). This is the real cadence of both drivers (ASummonedUnit::UpdateState's
	 *  StateTimerHandle and AMinerUnit's MiningPollTimerHandle / ArrivalCheckInterval).
	 *
	 *  ⭐ 0.25 IS EXACTLY REPRESENTABLE IN BINARY (2^-2) AND SO ARE 1.5 / 3.0 / 6.0, so every
	 *  accumulation below lands EXACTLY on a threshold. That is why the rung timings can be
	 *  asserted at a poll index instead of with a fudge factor.
	 */
	static constexpr float PollSeconds = 0.25f;

	/** Float tolerance for every clock assertion. Generous relative to the exact-binary steps. */
	static constexpr float ClockTolerance = 1.e-4f;

	/** Geometry tolerance: 1/100 uu against SidestepDistance 350. */
	static constexpr float GeometryTolerance = 0.01f;

	/** A velocity comfortably above the shipped MinSpeedSq (2500 == 50 uu/s). 100 uu/s. */
	static constexpr float FastVelocitySizeSq = 10000.f;

	/**
	 *  Builds a float from its IEEE-754 bits. Used for NaN and +INF.
	 *  ⛔ Deliberately NOT 0.f/0.f (undefined behaviour) and NOT std::numeric_limits (a standard
	 *  library include this module does not otherwise carry). Every test that uses this ALSO
	 *  asserts FMath::IsNaN on the result first, so a toolchain that folded the value away
	 *  reports itself instead of passing silently.
	 */
	static float BitsToFloat(uint32 Bits)
	{
		float Result = 0.f;
		FMemory::Memcpy(&Result, &Bits, sizeof(Result));
		return Result;
	}

	static float MakeQuietNaN() { return BitsToFloat(0x7FC00000u); }
	static float MakePositiveInfinity() { return BitsToFloat(0x7F800000u); }

	/**
	 *  One simulated unit: the tuning, the state, and the three per-poll observations the call
	 *  sites feed Evaluate. Poll() is one watchdog tick.
	 *
	 *  ⚠️ bAdvancing defaults TRUE because that is the interesting case; a unit that is idle by
	 *  design (test 4) sets it false.
	 */
	struct FStallHarness
	{
		FSiegeStuckTuning Tuning;
		FSiegeStuckState  State;

		FVector Location       = FVector::ZeroVector;
		float   VelocitySizeSq = 0.f;
		bool    bAdvancing     = true;

		int32 PollCount     = 0;
		float ElapsedSeconds = 0.f;

		ESiegeStuckAction Poll(float DeltaSeconds)
		{
			++PollCount;
			ElapsedSeconds += DeltaSeconds;
			return FSiegeStuckStatics::Evaluate(bAdvancing, Location, VelocitySizeSq, DeltaSeconds, Tuning, State);
		}
	};

	/** Running tally of what a long run returned, plus the order the rungs fired in. */
	struct FActionTally
	{
		int32 NoneCount     = 0;
		int32 SidestepCount = 0;
		int32 WidenCount    = 0;
		int32 AbandonCount  = 0;

		TArray<ESiegeStuckAction> FireSequence;

		void Record(ESiegeStuckAction Action)
		{
			switch (Action)
			{
			case ESiegeStuckAction::Sidestep:       ++SidestepCount; break;
			case ESiegeStuckAction::WidenAndRepath: ++WidenCount;    break;
			case ESiegeStuckAction::Abandon:        ++AbandonCount;  break;
			case ESiegeStuckAction::None:
			default:                                ++NoneCount;     return;
			}

			FireSequence.Add(Action);
		}

		/** Every non-None return. Bounded in TIME by brake 1 (EscalationCooldown). */
		int32 Fires() const { return SidestepCount + WidenCount + AbandonCount; }

		/**
		 *  The rungs that actually ISSUE a path request. ⛔ Abandon issues NONE — it drops the
		 *  goal and stands down (`NAV-§3` rung table) — so it is excluded here on purpose.
		 */
		int32 PathRequests() const { return SidestepCount + WidenCount; }
	};

	/** Runs NumPolls ticks at a fixed delta and tallies every answer. */
	static FActionTally RunPolls(FStallHarness& Harness, int32 NumPolls, float DeltaSeconds)
	{
		FActionTally Tally;
		for (int32 Index = 0; Index < NumPolls; ++Index)
		{
			Tally.Record(Harness.Poll(DeltaSeconds));
		}
		return Tally;
	}

	/** Polls until a rung fires (or MaxPolls elapse). OutPollsTaken counts the polls consumed. */
	static ESiegeStuckAction PollUntilFire(FStallHarness& Harness, float DeltaSeconds, int32 MaxPolls, int32& OutPollsTaken)
	{
		OutPollsTaken = 0;
		for (int32 Index = 0; Index < MaxPolls; ++Index)
		{
			++OutPollsTaken;
			const ESiegeStuckAction Action = Harness.Poll(DeltaSeconds);
			if (Action != ESiegeStuckAction::None)
			{
				return Action;
			}
		}
		return ESiegeStuckAction::None;
	}

	/** Human-readable rung name — MESSAGE TEXT ONLY. ⛔ Never an asserted value (`SC-§13`). */
	static const TCHAR* Describe(ESiegeStuckAction Action)
	{
		switch (Action)
		{
		case ESiegeStuckAction::Sidestep:       return TEXT("Sidestep");
		case ESiegeStuckAction::WidenAndRepath: return TEXT("WidenAndRepath");
		case ESiegeStuckAction::Abandon:        return TEXT("Abandon");
		case ESiegeStuckAction::None:
		default:                                return TEXT("None");
		}
	}

	/**
	 *  ⭐ MODELS THE SHIPPED CALL SITE'S `Attempt` EXPRESSION, character-for-character — as
	 *  CORRECTED at loop 1 of the TASK-537 gate:
	 *      ASummonedUnit::HandleStuckEscalation — `SidestepAttemptCount++ + (GetUniqueID() % 2)`
	 *      AMinerUnit::HandleStuckEscalation uses the identical idiom against the SAME
	 *      inherited member (it declares no counter of its own).
	 *
	 *  ⭐ THE COUNTER IS THE CALLER'S, BY REFERENCE, AND IT IS POST-INCREMENTED HERE EXACTLY AS
	 *  THE SHIPPED LINE DOES — modelling that is the whole point of this helper. It stands in for
	 *  ASummonedUnit::SidestepAttemptCount, a free-running per-unit `uint8`.
	 *
	 *  ⛔ IT IS NOT A FIELD OF FSiegeStuckState AND MUST NEVER BECOME ONE. Reset assigns a
	 *  default-constructed instance on every Abandon and every re-anchor, which is exactly what
	 *  made the FIRST shipped revision's `EscalationLevel + (uid % 2)` a per-unit CONSTANT:
	 *  EscalationLevel is ALWAYS 1 when Sidestep is returned (Evaluate assigns DesiredLevel
	 *  before its switch) and brake 2 lets Sidestep fire once per stall.
	 *  ⇒ qa/TASK-537.md, THE BLOCKER. Test 19 now proves the corrected behaviour.
	 *
	 *  ⚠️ `uint8` BY DESIGN: it wraps 255 -> 0 and that is harmless — only `Attempt & 1` is ever
	 *  read (FSiegeStuckStatics::ComputeSidestepGoal) and 256 is even, so the alternation
	 *  survives the wrap. Test 19(c) asserts that rather than assuming it.
	 */
	static int32 ShippedAttemptForSidestep(uint8& SidestepAttemptCount, uint32 UnitUniqueID)
	{
		return static_cast<int32>(SidestepAttemptCount++) + static_cast<int32>(UnitUniqueID % 2);
	}

	/**
	 *  MODELS THE TWO WRITES ASummonedUnit::NotifyMoveBlocked makes (SummonedUnit.cpp:3068-3077).
	 *  ⛔ IT DOES NOT CALL THE SHIPPED FUNCTION — that needs an actor, a controller and a world.
	 *  This models the state mutation only, so test 20's claim is about the LADDER's response to
	 *  that state, never about the shipped method having been exercised.
	 */
	static void ModelNotifyMoveBlockedWrites(FSiegeStuckState& State, const FVector& Location, const FSiegeStuckTuning& Tuning)
	{
		if (!State.bHasAnchor)
		{
			State.ProgressAnchor = Location;
			State.bHasAnchor     = true;
		}

		State.StalledSeconds = FMath::Max(State.StalledSeconds, Tuning.SidestepSeconds);
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  1. THE SHIPPED TUNING DEFAULTS + THE CLEARED-STATE DEFAULTS
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  `NAV-§7`'s tunables row pins all eight feel values, and every test below reasons from them.
 *  They are EditDefaultsOnly, so a retune is a Blueprint-side act — but a change to the C++
 *  DEFAULT silently re-tunes every unit class that never overrode it. Asserted here so that is
 *  a deliberate act with a red test attached, not a diff nobody notices.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckTuningDefaultsTest,
	"Siegebound.Nav.Stuck.ShippedTuningDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckTuningDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	const FSiegeStuckTuning Tuning;

	TestEqual(TEXT("ProgressRadius default is 150 uu"),       Tuning.ProgressRadius,       150.f,  ClockTolerance);
	TestEqual(TEXT("MinSpeedSq default is 2500 (50 uu/s)"),   Tuning.MinSpeedSq,           2500.f, ClockTolerance);
	TestEqual(TEXT("SidestepSeconds default is 1.5"),         Tuning.SidestepSeconds,      1.5f,   ClockTolerance);
	TestEqual(TEXT("WidenSeconds default is 3.0"),            Tuning.WidenSeconds,         3.0f,   ClockTolerance);
	TestEqual(TEXT("AbandonSeconds default is 6.0"),          Tuning.AbandonSeconds,       6.0f,   ClockTolerance);
	TestEqual(TEXT("EscalationCooldown default is 1.0"),      Tuning.EscalationCooldown,   1.0f,   ClockTolerance);
	TestEqual(TEXT("SidestepDistance default is 350 uu"),     Tuning.SidestepDistance,     350.f,  ClockTolerance);
	TestEqual(TEXT("SidestepLeaseSeconds default is 2.0"),    Tuning.SidestepLeaseSeconds, 2.0f,   ClockTolerance);

	// ⭐ THE ORDERING THE WHOLE LADDER ASSUMES. Evaluate is SAFE under mis-ordered tunables (it
	// tests high-to-low, so exactly one level comes out) — but every rung would not be
	// REACHABLE, and the shipped feel would be a different feature.
	TestTrue(TEXT("SidestepSeconds < WidenSeconds < AbandonSeconds at the shipped defaults"),
		Tuning.SidestepSeconds < Tuning.WidenSeconds && Tuning.WidenSeconds < Tuning.AbandonSeconds);

	// ⚠️ THE FACT TASK-531's HANDOFF FLAGGED FOR THIS SUITE, ASSERTED RATHER THAN RECITED:
	// every rung GAP exceeds the cooldown, so at the shipped defaults BRAKE 1 NEVER DECIDES
	// ANYTHING. Tests 10 and 11 exist because of this line.
	TestTrue(TEXT("Every shipped rung gap EXCEEDS EscalationCooldown, so brake 1 is never load-bearing at the defaults"),
		Tuning.SidestepSeconds > Tuning.EscalationCooldown
		&& (Tuning.WidenSeconds - Tuning.SidestepSeconds) > Tuning.EscalationCooldown
		&& (Tuning.AbandonSeconds - Tuning.WidenSeconds) > Tuning.EscalationCooldown);

	// The cleared state. FSiegeStuckState's member initialisers ARE the definition of "cleared"
	// (Reset assigns a default-constructed instance), so this pins both at once.
	const FSiegeStuckState State;
	TestFalse(TEXT("A fresh state has no anchor"), State.bHasAnchor);
	TestEqual(TEXT("A fresh state has StalledSeconds 0"), State.StalledSeconds, 0.f, ClockTolerance);
	TestEqual(TEXT("A fresh state has SecondsSinceEscalation 0"), State.SecondsSinceEscalation, 0.f, ClockTolerance);
	TestEqual(TEXT("A fresh state has EscalationLevel 0"), static_cast<int32>(State.EscalationLevel), 0);
	TestEqual(TEXT("A fresh state anchors at the origin (harmless: bHasAnchor is false)"),
		State.ProgressAnchor, FVector::ZeroVector, GeometryTolerance);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  2. THE RUNG ORDINALS ARE A CROSS-TASK CONTRACT
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ TASK-532 LOGS THE PINNED `level=` TOKEN AS THE **ACTION'S ORDINAL**
 *  (SummonedUnit.cpp:2947-2948, `static_cast<int32>(Action)`), precisely because
 *  StuckState.EscalationLevel has already been zeroed by Evaluate's internal Reset by the time
 *  Abandon is logged. ⇒ REORDERING THIS ENUM SILENTLY CORRUPTS THE GATE'S OWN EVIDENCE: the log
 *  would report a different rung than the one that fired, and nothing else would complain.
 *  `NAV-§8` pins the enumerator order; this asserts it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckRungOrdinalsTest,
	"Siegebound.Nav.Stuck.RungOrdinalsMatchTheLogContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckRungOrdinalsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ESiegeStuckAction::None is ordinal 0 (the level= token's 'no rung')"),
		static_cast<int32>(ESiegeStuckAction::None), 0);
	TestEqual(TEXT("ESiegeStuckAction::Sidestep is ordinal 1 (level=1)"),
		static_cast<int32>(ESiegeStuckAction::Sidestep), 1);
	TestEqual(TEXT("ESiegeStuckAction::WidenAndRepath is ordinal 2 (level=2)"),
		static_cast<int32>(ESiegeStuckAction::WidenAndRepath), 2);
	TestEqual(TEXT("ESiegeStuckAction::Abandon is ordinal 3 (level=3)"),
		static_cast<int32>(ESiegeStuckAction::Abandon), 3);

	TestEqual(TEXT("ESiegeStuckAction is a uint8 enum, as pinned"), static_cast<int32>(sizeof(ESiegeStuckAction)), 1);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  3. CASE 1 — A MOVING UNIT RETURNS None AND RE-ANCHORS
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  The overwhelmingly common answer, and the one that makes the cost claim true. WOULD CATCH: an
 *  Evaluate that accumulates before testing the cheap path (every moving unit would eventually
 *  be "rescued"), or one that re-anchors only on the FIRST poll and then lets a unit drift 150 uu
 *  at a time without ever resetting the anchor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckMovingReAnchorsTest,
	"Siegebound.Nav.Stuck.MovingUnitReAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckMovingReAnchorsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.bAdvancing     = true;
	Harness.VelocitySizeSq = FastVelocitySizeSq;
	Harness.Location       = FVector(100.f, 200.f, 50.f);

	const ESiegeStuckAction First = Harness.Poll(PollSeconds);
	TestTrue(FString::Printf(TEXT("A moving unit's first poll returns None (got %s)"), Describe(First)),
		First == ESiegeStuckAction::None);
	TestEqual(TEXT("The anchor is seeded at the unit's location"), Harness.State.ProgressAnchor, Harness.Location, GeometryTolerance);
	TestTrue(TEXT("bHasAnchor is set"), Harness.State.bHasAnchor);

	// ⭐ THE ANCHOR TRACKS THE UNIT. Move it far and keep it fast: every poll must RE-seed, so
	// the anchor can never lag behind and manufacture a false "no progress" reading later.
	for (int32 Step = 1; Step <= 40; ++Step)
	{
		Harness.Location = FVector(100.f + Step * 500.f, 200.f, 50.f);

		const ESiegeStuckAction Action = Harness.Poll(PollSeconds);
		TestTrue(FString::Printf(TEXT("Poll %d of a moving unit returns None (got %s)"), Step, Describe(Action)),
			Action == ESiegeStuckAction::None);
		TestEqual(FString::Printf(TEXT("Poll %d re-anchors at the CURRENT location"), Step),
			Harness.State.ProgressAnchor, Harness.Location, GeometryTolerance);
	}

	TestEqual(TEXT("A moving unit never accumulates stall time"), Harness.State.StalledSeconds, 0.f, ClockTolerance);
	TestEqual(TEXT("A moving unit never accumulates escalation time"), Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);
	TestEqual(TEXT("A moving unit never leaves level 0"), static_cast<int32>(Harness.State.EscalationLevel), 0);

	// THE BOUNDARY, BOTH SIDES. The clause is `VelocitySizeSq >= MinSpeedSq`, so EXACTLY at the
	// threshold the unit counts as moving.
	{
		FStallHarness AtThreshold;
		AtThreshold.VelocitySizeSq = AtThreshold.Tuning.MinSpeedSq;
		RunPolls(AtThreshold, 40, PollSeconds);
		TestEqual(TEXT("VelocitySizeSq EXACTLY == MinSpeedSq counts as moving (>=), so no stall accrues"),
			AtThreshold.State.StalledSeconds, 0.f, ClockTolerance);
	}
	{
		FStallHarness JustBelow;
		JustBelow.VelocitySizeSq = JustBelow.Tuning.MinSpeedSq - 1.f;
		RunPolls(JustBelow, 3, PollSeconds);
		TestTrue(TEXT("VelocitySizeSq one unit BELOW MinSpeedSq does accrue stall time"),
			JustBelow.State.StalledSeconds > 0.f);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  4. CASE 2 — A UNIT WITH NO MOVE REQUEST IS NEVER "RESCUED", HOWEVER LONG IT STANDS THERE
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ `NAV-§3`: "A unit that is idle BY DESIGN (holding, stationed, no request) must evaluate to
 *  None regardless of elapsed time, or the ladder will 'rescue' units that were told to stand
 *  still." WOULD CATCH: an Evaluate that keyed the ladder on velocity alone — every Hold /
 *  Ambush / stationed unit and every mid-attack unit in the game would start sidestepping.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckIdleByDesignTest,
	"Siegebound.Nav.Stuck.IdleByDesignUnitIsNeverRescued",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckIdleByDesignTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.bAdvancing     = false;  // no active path-following request this poll
	Harness.VelocitySizeSq = 0.f;    // and it is not moving either
	Harness.Location       = FVector(4000.f, -2500.f, 90.f);

	// 1200 polls at 0.25 s == FIVE MINUTES of standing perfectly still on the spot.
	const FActionTally Tally = RunPolls(Harness, 1200, PollSeconds);

	TestEqual(TEXT("Five minutes of idle-by-design produces ZERO rungs"), Tally.Fires(), 0);
	TestEqual(TEXT("...and 1200 None answers"), Tally.NoneCount, 1200);
	TestEqual(TEXT("...and never accumulates a stall clock"), Harness.State.StalledSeconds, 0.f, ClockTolerance);
	TestEqual(TEXT("...and never leaves level 0"), static_cast<int32>(Harness.State.EscalationLevel), 0);

	// ⚠️ AND IT MUST NOT DEPEND ON THE UNIT BEING SLOW: !bAdvancing is an OR-clause of its own.
	Harness.VelocitySizeSq = 0.f;
	Harness.bAdvancing     = false;
	Harness.Location       = FVector::ZeroVector;
	const FActionTally Stationary = RunPolls(Harness, 400, PollSeconds);
	TestEqual(TEXT("A motionless unit with no request still never escalates"), Stationary.Fires(), 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  5. CASE 3 — Sidestep FIRES AT 1.5 s, AND EXACTLY ONCE
// ════════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSidestepFiresOnceTest,
	"Siegebound.Nav.Stuck.SidestepFiresExactlyOnceAtThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSidestepFiresOnceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.bAdvancing     = true;   // a live path-following request
	Harness.VelocitySizeSq = 0.f;    // pinned against a rock
	Harness.Location       = FVector(1000.f, 1000.f, 0.f);  // and it never moves

	// POLL 1 IS THE ANCHOR POLL: a fresh state has bHasAnchor == false, so the first tick takes
	// the re-anchor branch and its delta is NOT charged to the stall. That is by design, and it
	// is why Sidestep lands on poll 7 and not poll 6.
	TestTrue(TEXT("The first poll of a fresh state re-anchors and returns None"),
		Harness.Poll(PollSeconds) == ESiegeStuckAction::None);
	TestTrue(TEXT("...and the anchor is now set"), Harness.State.bHasAnchor);

	for (int32 Poll = 2; Poll <= 6; ++Poll)
	{
		const ESiegeStuckAction Action = Harness.Poll(PollSeconds);
		TestTrue(FString::Printf(TEXT("Poll %d (stalled %.2f s, below SidestepSeconds) returns None (got %s)"),
			Poll, Harness.State.StalledSeconds, Describe(Action)), Action == ESiegeStuckAction::None);
	}

	TestEqual(TEXT("After 6 polls the stall clock reads 1.25 s — still below the rung"),
		Harness.State.StalledSeconds, 1.25f, ClockTolerance);

	const ESiegeStuckAction AtThreshold = Harness.Poll(PollSeconds);
	TestTrue(FString::Printf(TEXT("Poll 7 (stalled EXACTLY 1.5 s) fires Sidestep (got %s)"), Describe(AtThreshold)),
		AtThreshold == ESiegeStuckAction::Sidestep);

	TestEqual(TEXT("The stall clock reads exactly SidestepSeconds when the rung fires"),
		Harness.State.StalledSeconds, 1.5f, ClockTolerance);
	TestEqual(TEXT("Firing raises EscalationLevel to 1"), static_cast<int32>(Harness.State.EscalationLevel), 1);
	TestEqual(TEXT("Firing zeroes SecondsSinceEscalation — this is brake 1's reload"),
		Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);
	TestTrue(TEXT("Sidestep does NOT clear the anchor — the stall is still in progress"), Harness.State.bHasAnchor);

	// ⛔ AND IT NEVER FIRES A SECOND TIME. Poll on until the ladder's NEXT answer: it must be
	// WidenAndRepath, never a second Sidestep. (Exact timing is asserted in test 7.)
	int32 PollsToNextFire = 0;
	const ESiegeStuckAction NextFire = PollUntilFire(Harness, PollSeconds, 40, PollsToNextFire);
	TestTrue(FString::Printf(TEXT("The rung after Sidestep is WidenAndRepath, never a second Sidestep (got %s)"),
		Describe(NextFire)), NextFire == ESiegeStuckAction::WidenAndRepath);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  6. ⭐⛔ CASE 4 — THE ANTI-MILL TEST. RE-EVALUATING IMMEDIATELY RETURNS None
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE MOST IMPORTANT ASSERTION IN THIS FILE (`NAV-§3`): "a follow implementation that
 *  re-paths unconditionally is a QA FAIL … a stuck ladder that can re-issue a move every poll is
 *  the TASK-280 / TASK-282 mill wearing a rescue's clothes."
 *
 *  WOULD CATCH THE MILL DIRECTLY: an Evaluate that returned Sidestep on every poll once the
 *  threshold was passed would issue 4 path requests per unit per second — ~480/s at 120 units.
 *
 *  ⚠️ HONESTY NOTE, AND IT IS THE REASON TESTS 10 AND 11 EXIST: at the SHIPPED tuning BOTH brakes
 *  refuse this poll, so this test alone cannot tell you WHICH one did the work. It proves the
 *  ladder is not a mill; it does not prove the cooldown is wired up at all.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckImmediateReEvaluationTest,
	"Siegebound.Nav.Stuck.ImmediateReEvaluationIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckImmediateReEvaluationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.Location = FVector(-800.f, 640.f, 12.f);

	int32 PollsToSidestep = 0;
	const ESiegeStuckAction Fired = PollUntilFire(Harness, PollSeconds, 40, PollsToSidestep);
	TestTrue(FString::Printf(TEXT("The ladder reached Sidestep in %d polls (got %s)"), PollsToSidestep, Describe(Fired)),
		Fired == ESiegeStuckAction::Sidestep);

	// (a) THE SAME POLL AGAIN, ZERO ELAPSED TIME — the shape a double-drive would produce
	//     (two code paths evaluating the same unit on one tick).
	const ESiegeStuckAction Immediate = Harness.Poll(0.f);
	TestTrue(FString::Printf(TEXT("Re-evaluating with DeltaSeconds 0 immediately after a rung returns None (got %s)"),
		Describe(Immediate)), Immediate == ESiegeStuckAction::None);

	// (b) THE VERY NEXT REAL POLL, 0.25 s later.
	const ESiegeStuckAction NextPoll = Harness.Poll(PollSeconds);
	TestTrue(FString::Printf(TEXT("The very next 0.25 s poll returns None (got %s)"), Describe(NextPoll)),
		NextPoll == ESiegeStuckAction::None);

	// (c) AND FOR THE WHOLE REST OF THE COOLDOWN WINDOW. With the shipped 1.0 s cooldown and a
	//     1.5 s gap to the next rung, the next three polls must all be None too.
	for (int32 Extra = 1; Extra <= 3; ++Extra)
	{
		const ESiegeStuckAction Action = Harness.Poll(PollSeconds);
		TestTrue(FString::Printf(TEXT("Poll %d after Sidestep still returns None (got %s)"), Extra + 2, Describe(Action)),
			Action == ESiegeStuckAction::None);
	}

	TestEqual(TEXT("EscalationLevel is still 1 — the rung did not re-fire"),
		static_cast<int32>(Harness.State.EscalationLevel), 1);

	// ⛔ THE MILL'S SIGNATURE, MEASURED: hammer 200 polls at the SHIPPED cadence and count.
	//    A mill would return ~200 fires. The two brakes cap it far below that.
	FStallHarness Sustained;
	Sustained.Location = FVector(300.f, -300.f, 0.f);
	const FActionTally Tally = RunPolls(Sustained, 200, PollSeconds);  // 50 s

	TestTrue(FString::Printf(TEXT("50 s of unbroken stall produced %d rungs, not 200 (a per-poll mill would produce ~199)"),
		Tally.Fires()), Tally.Fires() < 40);
	AddInfo(FString::Printf(TEXT("ANTI-MILL MEASUREMENT: 50 s / 200 polls of unbroken stall => %d rungs total (%d Sidestep, %d Widen, %d Abandon), %d path-issuing rungs."),
		Tally.Fires(), Tally.SidestepCount, Tally.WidenCount, Tally.AbandonCount, Tally.PathRequests()));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  7. CASES 5 + 6 — THE FULL CLIMB: Sidestep 1.5 -> Widen 3.0 -> Abandon 6.0 -> AUTO-RESET
// ════════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckLadderClimbTest,
	"Siegebound.Nav.Stuck.LadderClimbsToWidenThenAbandon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckLadderClimbTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.Location = FVector(2500.f, 2500.f, 64.f);

	int32 Polls = 0;

	// ── RUNG 1 ──────────────────────────────────────────────────────────────────────────────
	TestTrue(TEXT("Rung 1 is Sidestep"), PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
	TestEqual(TEXT("Sidestep fires on the 7th poll (1 anchor poll + 6 x 0.25 s == 1.5 s)"), Polls, 7);
	TestEqual(TEXT("...at StalledSeconds == SidestepSeconds"), Harness.State.StalledSeconds, 1.5f, ClockTolerance);

	// ── RUNG 2 ──────────────────────────────────────────────────────────────────────────────
	TestTrue(TEXT("Rung 2 is WidenAndRepath"), PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::WidenAndRepath);
	TestEqual(TEXT("WidenAndRepath fires 6 polls later (1.5 s -> 3.0 s)"), Polls, 6);
	TestEqual(TEXT("...at StalledSeconds == WidenSeconds"), Harness.State.StalledSeconds, 3.0f, ClockTolerance);
	TestEqual(TEXT("...raising EscalationLevel to 2"), static_cast<int32>(Harness.State.EscalationLevel), 2);
	TestEqual(TEXT("...and reloading brake 1"), Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);

	// ── RUNG 3 ──────────────────────────────────────────────────────────────────────────────
	TestTrue(TEXT("Rung 3 is Abandon"), PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Abandon);
	TestEqual(TEXT("Abandon fires 12 polls later (3.0 s -> 6.0 s)"), Polls, 12);

	// ⭐ THE AUTO-RESET. Abandon clears the state INSIDE Evaluate (SiegeStuckStatics.cpp:169), so
	// the call site does not have to remember to — and the very next poll must therefore take
	// the re-anchor branch. WOULD CATCH: an Abandon that left EscalationLevel at 3, which would
	// wedge the ladder at "nothing above level 3 is reachable" and silently disable the watchdog
	// on that unit for the rest of the match.
	TestEqual(TEXT("⭐ Abandon auto-Resets: StalledSeconds is 0"), Harness.State.StalledSeconds, 0.f, ClockTolerance);
	TestEqual(TEXT("⭐ Abandon auto-Resets: SecondsSinceEscalation is 0"), Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);
	TestEqual(TEXT("⭐ Abandon auto-Resets: EscalationLevel is back to 0"), static_cast<int32>(Harness.State.EscalationLevel), 0);
	TestTrue(TEXT("⭐ Abandon auto-Resets: bHasAnchor is dropped, so the next poll re-anchors"), !Harness.State.bHasAnchor);
	TestEqual(TEXT("⭐ Abandon auto-Resets: the anchor itself returns to the declared default"),
		Harness.State.ProgressAnchor, FVector::ZeroVector, GeometryTolerance);

	TestTrue(TEXT("The poll straight after Abandon returns None (the re-anchor branch)"),
		Harness.Poll(PollSeconds) == ESiegeStuckAction::None);
	TestEqual(TEXT("...and re-seeds the anchor at the unit's location"),
		Harness.State.ProgressAnchor, Harness.Location, GeometryTolerance);

	// AND THE LADDER RESTARTS FROM RUNG 1 — never from where it left off.
	TestTrue(TEXT("The next stall starts again at Sidestep, not at Widen"),
		PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
	TestEqual(TEXT("...6 polls after the re-anchor"), Polls, 6);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  8. CASE 7 — ESCAPING ProgressRadius MID-LADDER IS A FULL RESET
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  WOULD CATCH: a reset that cleared the clocks but left EscalationLevel behind. Such a unit
 *  would be at level 2 with a fresh clock, so its NEXT stall would silently SKIP Sidestep — the
 *  cheapest rung and the only one that actually steers around the rock.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckProgressResetsLadderTest,
	"Siegebound.Nav.Stuck.EscapingProgressRadiusResetsTheLadder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckProgressResetsLadderTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.Location = FVector::ZeroVector;

	int32 Polls = 0;
	TestTrue(TEXT("Climb to Sidestep first"), PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
	TestEqual(TEXT("Mid-ladder: level 1"), static_cast<int32>(Harness.State.EscalationLevel), 1);

	// Two more polls so both clocks hold non-zero values when the escape happens.
	Harness.Poll(PollSeconds);
	Harness.Poll(PollSeconds);
	TestTrue(TEXT("Mid-ladder: the stall clock is running"), Harness.State.StalledSeconds > 1.9f);

	// ── THE BOUNDARY, FIRST. The clause is `DistSquared > Square(ProgressRadius)`, so a unit
	//    sitting EXACTLY ProgressRadius from its anchor has NOT escaped.
	Harness.Location = FVector(Harness.Tuning.ProgressRadius, 0.f, 0.f);  // exactly 150 uu
	const float StalledBefore = Harness.State.StalledSeconds;
	TestTrue(TEXT("A displacement of EXACTLY ProgressRadius does not count as escaping"),
		Harness.Poll(PollSeconds) == ESiegeStuckAction::None);
	TestTrue(TEXT("...and the stall clock keeps running through it"), Harness.State.StalledSeconds > StalledBefore);
	TestEqual(TEXT("...and the level is untouched"), static_cast<int32>(Harness.State.EscalationLevel), 1);

	// ── NOW ESCAPE FOR REAL.
	const FVector EscapedTo(400.f, 0.f, 0.f);
	Harness.Location = EscapedTo;

	const ESiegeStuckAction Action = Harness.Poll(PollSeconds);
	TestTrue(FString::Printf(TEXT("Escaping ProgressRadius returns None (got %s)"), Describe(Action)),
		Action == ESiegeStuckAction::None);

	TestEqual(TEXT("FULL reset: StalledSeconds"), Harness.State.StalledSeconds, 0.f, ClockTolerance);
	TestEqual(TEXT("FULL reset: SecondsSinceEscalation"), Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);
	TestEqual(TEXT("⭐ FULL reset: EscalationLevel drops back to 0"), static_cast<int32>(Harness.State.EscalationLevel), 0);
	TestTrue(TEXT("FULL reset: bHasAnchor stays true (it is re-seeded, not dropped)"), Harness.State.bHasAnchor);
	TestEqual(TEXT("FULL reset: the anchor moves to where the unit now is"),
		Harness.State.ProgressAnchor, EscapedTo, GeometryTolerance);

	// A FRESH STALL RESTARTS AT RUNG 1, AND ON THE FULL CLOCK.
	TestTrue(TEXT("The fresh stall's first rung is Sidestep"),
		PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
	TestEqual(TEXT("...and it takes the full 6 polls (1.5 s), with no credit carried over"), Polls, 6);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  9. ⭐ CASE 8 — DeltaSeconds == 0 (THE MINER'S SEAL VALUE), NEGATIVE, NaN AND +INF
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⚠️ AMinerUnit SEALS StateCheckInterval TO 0 (MinerUnit.cpp:61) AND TASK-533 DRIVES THIS SAME
 *  LADDER. A delta accidentally read from it must never advance a rung, never divide and never
 *  assert — the failure mode is SILENT (`NAV-§10` criterion 4).
 *
 *  ⭐ TASK-531 CLAMPS WITH `(DeltaSeconds > 0.f) ? DeltaSeconds : 0.f` RATHER THAN FMath::Max
 *  SPECIFICALLY SO NaN ALSO LANDS ON 0 (every comparison against NaN is false). That is asserted
 *  here rather than trusted, because FMath::Max would pass the zero and negative cases and fail
 *  ONLY on NaN — the exact difference this test exists to detect.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckZeroDeltaTest,
	"Siegebound.Nav.Stuck.ZeroDeltaNeverAdvancesTheLadder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckZeroDeltaTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	// ── (a) THE SEAL VALUE ITSELF: 2000 polls at delta 0 ────────────────────────────────────
	{
		FStallHarness Harness;
		Harness.Location = FVector(120.f, 340.f, 8.f);

		const FActionTally Tally = RunPolls(Harness, 2000, 0.f);

		TestEqual(TEXT("⭐ 2000 polls at DeltaSeconds == 0 fire ZERO rungs"), Tally.Fires(), 0);
		TestEqual(TEXT("...and the stall clock never leaves 0"), Harness.State.StalledSeconds, 0.f, ClockTolerance);
		TestEqual(TEXT("...and the escalation clock never leaves 0"), Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);
		TestEqual(TEXT("...and the level never leaves 0"), static_cast<int32>(Harness.State.EscalationLevel), 0);
		TestTrue(TEXT("...and both clocks stay finite (nothing divided by the delta)"),
			FMath::IsFinite(Harness.State.StalledSeconds) && FMath::IsFinite(Harness.State.SecondsSinceEscalation));
		TestTrue(TEXT("...and the anchor is still a real point"), !Harness.State.ProgressAnchor.ContainsNaN());
	}

	// ── (b) A ZERO DELTA MUST NOT CORRUPT AN ALREADY-RUNNING LADDER ─────────────────────────
	{
		FStallHarness Harness;
		int32 Polls = 0;
		TestTrue(TEXT("Climb to Sidestep on real deltas"),
			PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);

		const float StalledAtRung = Harness.State.StalledSeconds;
		const FActionTally Zeroes = RunPolls(Harness, 500, 0.f);

		TestEqual(TEXT("500 zero-delta polls mid-ladder fire nothing"), Zeroes.Fires(), 0);
		TestEqual(TEXT("...and freeze the stall clock exactly where it was"),
			Harness.State.StalledSeconds, StalledAtRung, ClockTolerance);
		TestEqual(TEXT("...and leave the level where it was"), static_cast<int32>(Harness.State.EscalationLevel), 1);

		// And the ladder resumes correctly the moment real deltas return.
		TestTrue(TEXT("Real deltas afterwards still reach WidenAndRepath"),
			PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::WidenAndRepath);
	}

	// ── (c) NEGATIVE DELTAS: the clocks may never run backwards ─────────────────────────────
	{
		FStallHarness Harness;
		int32 Polls = 0;
		PollUntilFire(Harness, PollSeconds, 40, Polls);  // reach Sidestep
		const float StalledAtRung = Harness.State.StalledSeconds;

		RunPolls(Harness, 100, -5.f);
		TestEqual(TEXT("A negative delta is clamped to 0 — the stall clock never runs backwards"),
			Harness.State.StalledSeconds, StalledAtRung, ClockTolerance);
		TestTrue(TEXT("...and never goes negative"), Harness.State.StalledSeconds >= 0.f);
	}

	// ── (d) ⭐ NaN — THE CASE THAT DISTINGUISHES THE TERNARY FROM FMath::Max ─────────────────
	{
		const float NaNDelta = MakeQuietNaN();

		// ⛔ ASSERTED FIRST: if the toolchain folded this into a non-NaN, the rest of this block
		// would pass VACUOUSLY. This line is what stops that.
		TestTrue(TEXT("The test's NaN constant really is NaN (a vacuous pass guard)"), FMath::IsNaN(NaNDelta));

		FStallHarness Harness;
		Harness.Location = FVector(-60.f, 15.f, 3.f);
		Harness.Poll(PollSeconds);   // anchor poll
		Harness.Poll(PollSeconds);   // one real accumulation, so there is something to corrupt

		const float StalledBefore = Harness.State.StalledSeconds;
		const FActionTally Tally  = RunPolls(Harness, 50, NaNDelta);

		TestEqual(TEXT("⭐ 50 NaN deltas fire zero rungs"), Tally.Fires(), 0);
		TestTrue(TEXT("⭐ NaN never reaches the stall clock (this is the ternary, not FMath::Max)"),
			FMath::IsFinite(Harness.State.StalledSeconds));
		TestTrue(TEXT("⭐ NaN never reaches the escalation clock"),
			FMath::IsFinite(Harness.State.SecondsSinceEscalation));
		TestEqual(TEXT("⭐ A NaN delta accumulates exactly nothing"),
			Harness.State.StalledSeconds, StalledBefore, ClockTolerance);
		TestEqual(TEXT("...and never asserts, crashes or escalates"), static_cast<int32>(Harness.State.EscalationLevel), 0);
	}

	// ── (e) +INFINITY — declared, pinned, and unreachable from the shipped call sites ────────
	//    ⚠️ REPORTED, NOT "FIXED": +INF is NOT clamped (only <= 0 and NaN are), so it satisfies
	//    every threshold at once and the ladder jumps straight to Abandon. That is inside brake 2
	//    (Abandon issues no path request) and Abandon's own Reset scrubs the infinity out of the
	//    state, so the unit is left CLEAN rather than poisoned. Both call sites clamp the delta to
	//    MaxStuckDeltaSeconds == 1 (SummonedUnit.cpp:2889) so no shipped caller can produce it.
	{
		const float InfDelta = MakePositiveInfinity();
		TestTrue(TEXT("The test's +INF constant really is infinite (a vacuous pass guard)"),
			!FMath::IsFinite(InfDelta) && !FMath::IsNaN(InfDelta));

		FStallHarness Harness;
		Harness.Poll(PollSeconds);  // anchor poll

		const ESiegeStuckAction Action = Harness.Poll(InfDelta);
		TestTrue(FString::Printf(TEXT("An infinite delta lands on Abandon, the rung that issues NO path request (got %s)"),
			Describe(Action)), Action == ESiegeStuckAction::Abandon);
		TestTrue(TEXT("...and Abandon's own Reset scrubs the infinity out of the state"),
			FMath::IsFinite(Harness.State.StalledSeconds) && FMath::IsFinite(Harness.State.SecondsSinceEscalation));
		TestEqual(TEXT("...leaving a clean, fully-cleared state"), static_cast<int32>(Harness.State.EscalationLevel), 0);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  10. ⭐⭐ BRAKE 1 IN ISOLATION — THE COOLDOWN, MADE LOAD-BEARING BY RETUNING
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ THE GAP TASK-531 FOUND IN ITS OWN DEFAULTS, CLOSED HERE.
 *
 *  At the shipped tuning the rung gaps (1.5 -> 3.0 -> 6.0) all EXCEED EscalationCooldown (1.0),
 *  so brake 1 never DECIDES anything: every poll it refuses, brake 2 would have refused too.
 *  ⇒ A SUITE THAT ONLY EVER USES DEFAULT TUNING LEAVES THE ENTIRE ANTI-MILL GATE UNTESTED, and
 *  that gate is the law (`CONVENTIONS.md:535` / `NAV-§3`).
 *
 *  FSiegeStuckTuning is EditDefaultsOnly, so SidestepSeconds 1.5 with WidenSeconds 1.6 is a
 *  value Jonathan can type at the playtest without a recompile — this is not a synthetic case,
 *  it is the case brake 1 exists for.
 *
 *  ⛔ WOULD CATCH: deleting the cooldown check entirely. Without it Widen fires at 1.75 s, one
 *  poll after Sidestep. With it, Widen is held until EXACTLY EscalationCooldown has passed.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckCooldownGatesTest,
	"Siegebound.Nav.Stuck.EscalationCooldownGatesTheLadder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckCooldownGatesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.Tuning.SidestepSeconds = 1.5f;
	Harness.Tuning.WidenSeconds    = 1.6f;   // ⭐ only 0.1 s above rung 1 — well inside the cooldown
	Harness.Tuning.AbandonSeconds  = 6.0f;
	// EscalationCooldown stays at the shipped 1.0.

	int32 Polls = 0;
	TestTrue(TEXT("Sidestep still fires on time at 1.5 s"),
		PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
	TestEqual(TEXT("...on the 7th poll"), Polls, 7);
	TestEqual(TEXT("...and brake 1 is reloaded to 0"), Harness.State.SecondsSinceEscalation, 0.f, ClockTolerance);

	// ⭐ THE LOAD-BEARING POLLS. The stall clock passes WidenSeconds (1.6) on the very next poll
	// (1.75 s) and brake 2 is satisfied (DesiredLevel 2 > EscalationLevel 1) — so the ONLY thing
	// that can be returning None here is the cooldown.
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		const ESiegeStuckAction Action = Harness.Poll(PollSeconds);

		TestTrue(FString::Printf(TEXT("⭐ Poll %d after Sidestep: stalled %.2f s is PAST WidenSeconds 1.60 and brake 2 would allow it, but the cooldown (%.2f/%.2f s) refuses (got %s)"),
			Index, Harness.State.StalledSeconds, Harness.State.SecondsSinceEscalation, Harness.Tuning.EscalationCooldown, Describe(Action)),
			Action == ESiegeStuckAction::None);

		TestTrue(TEXT("⭐ ...and the ONLY reason it can be None is brake 1: the entitled rung is genuinely above the current level"),
			Harness.State.StalledSeconds >= Harness.Tuning.WidenSeconds
			&& static_cast<int32>(Harness.State.EscalationLevel) == 1);

		TestTrue(TEXT("⭐ ...because the escalation clock has not reached the cooldown yet"),
			Harness.State.SecondsSinceEscalation < Harness.Tuning.EscalationCooldown);
	}

	// THE 4th POLL PUTS SecondsSinceEscalation AT EXACTLY 1.0 — the gate is `<`, so it opens.
	const ESiegeStuckAction Released = Harness.Poll(PollSeconds);
	TestTrue(FString::Printf(TEXT("⭐ WidenAndRepath is released the instant SecondsSinceEscalation reaches EscalationCooldown (got %s)"),
		Describe(Released)), Released == ESiegeStuckAction::WidenAndRepath);
	TestEqual(TEXT("⭐ ...i.e. exactly EscalationCooldown after Sidestep, not one poll after the threshold"),
		Harness.State.StalledSeconds, 2.5f, ClockTolerance);

	AddInfo(TEXT("BRAKE 1 CONFIRMED LOAD-BEARING: with SidestepSeconds 1.5 / WidenSeconds 1.6, the cooldown delayed rung 2 from 1.75 s to 2.50 s. At the SHIPPED defaults this gate never decides anything, which is why this retuned case exists (handoffs/TASK-531-programmer.md, note to TASK-536)."));

	// ── AND THE EXTREME: EVERY THRESHOLD IDENTICAL. Only the cooldown can space these out. ──
	{
		FStallHarness Degenerate;
		Degenerate.Tuning.SidestepSeconds = 0.25f;
		Degenerate.Tuning.WidenSeconds    = 0.25f;
		Degenerate.Tuning.AbandonSeconds  = 0.25f;
		// Cooldown 1.0.

		// With all three equal, the high-to-low test picks level 3 (Abandon) the first time the
		// clock passes 0.25 — so Sidestep and Widen are UNREACHABLE and the ladder emits one
		// Abandon per cycle. Bounded, never a mill, and never a NaN.
		const FActionTally Tally = RunPolls(Degenerate, 200, PollSeconds);  // 50 s

		TestEqual(TEXT("Equal thresholds: only Abandon is reachable (high-to-low selection)"), Tally.SidestepCount, 0);
		TestEqual(TEXT("Equal thresholds: Widen is unreachable too"), Tally.WidenCount, 0);
		TestTrue(TEXT("⛔ Equal thresholds STILL cannot mill: Abandon issues no path request at all"),
			Tally.PathRequests() == 0);
		TestTrue(FString::Printf(TEXT("⛔ Equal thresholds fired %d rungs in 50 s — bounded well under one per second by brake 1"),
			Tally.Fires()), Tally.Fires() <= 51);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  11. ⭐⭐ BRAKE 2 IN ISOLATION — MONOTONICITY HOLDS WITH THE COOLDOWN SET TO ZERO
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ TASK-531 DOCUMENTS THE TWO BRAKES AS INDEPENDENT. This proves the second half of that: with
 *  EscalationCooldown == 0, brake 1 is DISABLED (`SecondsSinceEscalation < 0` is never true), so
 *  every refusal below is brake 2's monotonic EscalationLevel doing the work alone.
 *
 *  ⚠️⚠️ AND IT MEASURES WHAT BRAKE 2 ALONE ACTUALLY BUYS, WHICH IS **NOT** WHAT THE HANDOFF SAYS.
 *  Brake 2 bounds the COUNT per stall (<= 3 rungs, of which <= 2 issue a request). It does NOT
 *  reproduce `NAV-§3`'s "<= 1 extra path request per unit per second" CEILING, because a stall's
 *  duration is itself a tunable: shrink AbandonSeconds and the same 2 requests are spent over a
 *  shorter window. The second block below measures that. ⛔ Reported, not encoded as a failure —
 *  the SHIPPED defaults honour the ceiling and TASK-537 rules on the wording.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckMonotonicLevelTest,
	"Siegebound.Nav.Stuck.MonotonicLevelHoldsWithoutTheCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckMonotonicLevelTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	// ── (a) THE ISOLATED PROOF: cooldown 0, shipped thresholds ──────────────────────────────
	{
		FStallHarness Harness;
		Harness.Tuning.EscalationCooldown = 0.f;  // ⛔ brake 1 disabled

		int32 Polls = 0;
		TestTrue(TEXT("Sidestep fires at 1.5 s with the cooldown disabled"),
			PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
		TestEqual(TEXT("...level is now 1"), static_cast<int32>(Harness.State.EscalationLevel), 1);

		// ⭐ THE ASSERTION THAT ISOLATES BRAKE 2. Brake 1 cannot refuse anything (cooldown 0), yet
		// the next five polls must all be None, because DesiredLevel is still 1 and the gate is
		// STRICTLY greater. WOULD CATCH `DesiredLevel < EscalationLevel` or `<=` written as `<`.
		for (int32 Index = 1; Index <= 5; ++Index)
		{
			const ESiegeStuckAction Action = Harness.Poll(PollSeconds);
			TestTrue(FString::Printf(TEXT("⭐ Poll %d with EscalationCooldown == 0 still returns None — brake 2 alone (got %s)"),
				Index, Describe(Action)), Action == ESiegeStuckAction::None);
			TestEqual(TEXT("⭐ ...and brake 1 is provably not the reason (cooldown is 0)"),
				Harness.Tuning.EscalationCooldown, 0.f, ClockTolerance);
		}

		TestTrue(TEXT("The ladder still climbs to Widen when the level genuinely rises"),
			PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::WidenAndRepath);
		TestEqual(TEXT("...at the honest threshold, 3.0 s"), Harness.State.StalledSeconds, 3.0f, ClockTolerance);
	}

	// ── (b) THE PER-STALL COUNT BOUND, SUSTAINED, WITH BRAKE 1 DISABLED ─────────────────────
	{
		FStallHarness Harness;
		Harness.Tuning.EscalationCooldown = 0.f;

		const FActionTally Tally = RunPolls(Harness, 480, PollSeconds);  // 120 s

		// EVERY rung fires at most ONCE per stall: each cycle contributes exactly one of each,
		// so the counts can never differ by more than the one truncated cycle at the end.
		TestTrue(FString::Printf(TEXT("⭐ <= 1 Sidestep per stall (Sidesteps %d vs Abandons %d)"),
			Tally.SidestepCount, Tally.AbandonCount), Tally.SidestepCount <= Tally.AbandonCount + 1);
		TestTrue(FString::Printf(TEXT("⭐ <= 1 WidenAndRepath per stall (Widens %d vs Abandons %d)"),
			Tally.WidenCount, Tally.AbandonCount), Tally.WidenCount <= Tally.AbandonCount + 1);
		TestTrue(FString::Printf(TEXT("⭐ <= 3 rungs per stall, EVER (fires %d, stalls %d)"),
			Tally.Fires(), Tally.AbandonCount + 1), Tally.Fires() <= 3 * (Tally.AbandonCount + 1));
		TestTrue(FString::Printf(TEXT("⭐ <= 2 PATH REQUESTS per stall — Abandon issues none (requests %d, stalls %d)"),
			Tally.PathRequests(), Tally.AbandonCount + 1), Tally.PathRequests() <= 2 * (Tally.AbandonCount + 1));

		// THE ORDER NEVER INVERTS AND NO RUNG EVER REPEATS INSIDE A STALL.
		const ESiegeStuckAction ExpectedCycle[3] =
		{
			ESiegeStuckAction::Sidestep,
			ESiegeStuckAction::WidenAndRepath,
			ESiegeStuckAction::Abandon
		};
		for (int32 Index = 0; Index < Tally.FireSequence.Num(); ++Index)
		{
			TestTrue(FString::Printf(TEXT("Fire %d is %s, the cyclic ladder order (got %s)"),
				Index, Describe(ExpectedCycle[Index % 3]), Describe(Tally.FireSequence[Index])),
				Tally.FireSequence[Index] == ExpectedCycle[Index % 3]);
		}

		AddInfo(FString::Printf(TEXT("BRAKE 2 ALONE, 120 s sustained stall, EscalationCooldown = 0: %d rungs (%d Sidestep, %d Widen, %d Abandon), %d path requests => %.2f path requests/unit/s."),
			Tally.Fires(), Tally.SidestepCount, Tally.WidenCount, Tally.AbandonCount,
			Tally.PathRequests(), Tally.PathRequests() / 120.f));
	}

	// ── (c) ⚠️ THE MEASUREMENT THAT CONTRADICTS "EITHER BRAKE ALONE BOUNDS THE **RATE**" ─────
	//    Cooldown 0 AND a fast ladder. Brake 2's per-stall count still holds perfectly — but the
	//    resulting RATE exceeds NAV-§3's "<= 1 extra path request per unit per second" ceiling,
	//    because nothing but brake 1 bounds a stall's TURNOVER. ⛔ Recorded via AddInfo, not a
	//    red test: no shipped tuning does this, and TASK-537 owns the ruling.
	{
		FStallHarness Harness;
		Harness.Tuning.EscalationCooldown = 0.f;
		Harness.Tuning.SidestepSeconds    = 0.25f;
		Harness.Tuning.WidenSeconds       = 0.50f;
		Harness.Tuning.AbandonSeconds     = 0.75f;

		const FActionTally Tally = RunPolls(Harness, 400, PollSeconds);  // 100 s
		const float RequestsPerSecond = Tally.PathRequests() / 100.f;

		// The per-stall bound — brake 2's ACTUAL guarantee — still holds exactly.
		TestTrue(FString::Printf(TEXT("Brake 2's per-stall bound survives even the fastest tuning (%d requests over %d stalls)"),
			Tally.PathRequests(), Tally.AbandonCount + 1),
			Tally.PathRequests() <= 2 * (Tally.AbandonCount + 1));

		AddInfo(FString::Printf(TEXT("⚠️ FINDING FOR TASK-537 (measured, not argued): with EscalationCooldown = 0 and thresholds 0.25/0.50/0.75 — all legal EditDefaultsOnly values — the ladder issues %.2f path requests/unit/s, ABOVE NAV-§3's <= 1/unit/s ceiling. Brake 2 bounds the COUNT PER STALL (<= 2 requests), not the RATE; the rate ceiling rests on EscalationCooldown >= 1.0 alone. handoffs/TASK-531-programmer.md §2 states the two brakes are independently rate-bounding — that half is not true. The SHIPPED defaults are unaffected (see the ShippedTuningDefaults test)."),
			RequestsPerSecond));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  12. THE WORST-CASE REQUEST RATE AT THE SHIPPED DEFAULTS
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  `NAV-§10` criterion 1 is a NUMBER, so it gets a measurement rather than an argument:
 *  "<= 1 extra path request per unit per second, and ONLY for units that are demonstrably not
 *  moving." TASK-531's arithmetic predicts ~0.33 requests/unit/s sustained at the defaults.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSustainedRateTest,
	"Siegebound.Nav.Stuck.SustainedStallRespectsTheRequestCeiling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSustainedRateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	constexpr int32 NumPolls = 240;                       // 60 s at the shipped 0.25 s cadence
	constexpr float DurationSeconds = NumPolls * PollSeconds;

	FStallHarness Harness;
	Harness.Location = FVector(-14000.f, 6200.f, 40.f);   // the far field, where tiles settle last
	const FActionTally Tally = RunPolls(Harness, NumPolls, PollSeconds);

	// ── BRAKE 1's TIME BOUND: at most one fire per EscalationCooldown, plus the first. ───────
	const int32 CooldownCeiling = FMath::FloorToInt(DurationSeconds / Harness.Tuning.EscalationCooldown) + 1;
	TestTrue(FString::Printf(TEXT("⭐ %d rungs in %.0f s is within brake 1's ceiling of %d (<= 1 per EscalationCooldown)"),
		Tally.Fires(), DurationSeconds, CooldownCeiling), Tally.Fires() <= CooldownCeiling);

	// ── BRAKE 2's COUNT BOUND: <= 2 PATH-ISSUING rungs per stall; Abandon issues none. ───────
	const int32 Stalls = Tally.AbandonCount + 1;
	TestTrue(FString::Printf(TEXT("⭐ %d path requests over %d stalls is within brake 2's bound of %d (<= 2 per stall)"),
		Tally.PathRequests(), Stalls, 2 * Stalls), Tally.PathRequests() <= 2 * Stalls);

	// ── AND THE SUSTAINED RATE TASK-531 PREDICTS: ~2 requests per AbandonSeconds == ~0.33/s. ─
	const float RequestsPerSecond = Tally.PathRequests() / DurationSeconds;
	const float PredictedCeiling  = 2.f / Harness.Tuning.AbandonSeconds;   // 0.333...
	// The margin is 0.01 rather than an epsilon because the run is TRUNCATED mid-stall: the last
	// cycle contributes its Sidestep and Widen without its Abandon, so the measured rate sits
	// exactly ON the predicted ceiling rather than under it.
	TestTrue(FString::Printf(TEXT("⭐ Sustained rate %.3f requests/unit/s is at or under the predicted %.3f"),
		RequestsPerSecond, PredictedCeiling), RequestsPerSecond <= PredictedCeiling + 0.01f);

	// Sanity: the ladder actually RAN. A bound satisfied by doing nothing proves nothing.
	TestTrue(FString::Printf(TEXT("The ladder actually cycled during the run (%d Abandons in %.0f s)"),
		Tally.AbandonCount, DurationSeconds), Tally.AbandonCount >= 8);
	TestTrue(TEXT("Every stall issued its Sidestep"), Tally.SidestepCount >= Tally.AbandonCount);
	TestTrue(TEXT("Every stall issued its Widen"), Tally.WidenCount >= Tally.AbandonCount);

	AddInfo(FString::Printf(TEXT("SHIPPED-DEFAULT WORST CASE, %.0f s of unbroken stall: %d rungs (%d Sidestep, %d Widen, %d Abandon) => %.3f path requests/unit/s. At 120 wedged units that is ~%.0f extra requests/s; the forbidden per-poll mill would be ~%.0f/s."),
		DurationSeconds, Tally.Fires(), Tally.SidestepCount, Tally.WidenCount, Tally.AbandonCount,
		RequestsPerSecond, RequestsPerSecond * 120.f, (1.f / PollSeconds) * 120.f));

	// ⛔ THE OTHER HALF OF CRITERION 1: "ONLY FOR UNITS THAT ARE DEMONSTRABLY NOT MOVING."
	// A unit that is moving pays ZERO requests over the identical run.
	{
		FStallHarness Moving;
		Moving.VelocitySizeSq = FastVelocitySizeSq;
		const FActionTally MovingTally = RunPolls(Moving, NumPolls, PollSeconds);
		TestEqual(TEXT("⛔ A MOVING unit costs zero rungs over the same 60 s"), MovingTally.Fires(), 0);
	}
	{
		FStallHarness NoRequest;
		NoRequest.bAdvancing = false;
		const FActionTally IdleTally = RunPolls(NoRequest, NumPolls, PollSeconds);
		TestEqual(TEXT("⛔ A unit with no move request costs zero rungs over the same 60 s"), IdleTally.Fires(), 0);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  13. CASE 9a — ComputeSidestepGoal IS PERPENDICULAR, AT THE RIGHT DISTANCE, AT THE RIGHT Z
// ════════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSidestepGeometryTest,
	"Siegebound.Nav.Stuck.SidestepGoalIsPerpendicularAtDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSidestepGeometryTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	const FSiegeStuckTuning Tuning;

	struct FGeometryCase
	{
		FVector Location;
		FVector Goal;
		const TCHAR* Description;
	};

	const FGeometryCase Cases[] =
	{
		{ FVector::ZeroVector,             FVector(1000.f, 0.f, 0.f),         TEXT("due +X")                       },
		{ FVector::ZeroVector,             FVector(0.f, 1000.f, 0.f),         TEXT("due +Y")                       },
		{ FVector(500.f, -250.f, 90.f),    FVector(-3000.f, 4000.f, 90.f),    TEXT("diagonal, same Z")             },
		{ FVector(-120.f, 640.f, 12.5f),   FVector(-120.f, 640.f, 900.f),     TEXT("goal overhead (degenerate XY)") },
		{ FVector(10.f, 10.f, 0.f),        FVector(11.f, 10.f, 0.f),          TEXT("goal 1 uu away")               },
		{ FVector(-9000.f, 8000.f, -30.f), FVector(9000.f, -8000.f, 300.f),   TEXT("far diagonal, Z differs")      },
	};

	for (const FGeometryCase& Case : Cases)
	{
		for (int32 Attempt = 0; Attempt <= 1; ++Attempt)
		{
			const FVector Result = FSiegeStuckStatics::ComputeSidestepGoal(
				Case.Location, Case.Goal, Tuning.SidestepDistance, Attempt);

			TestTrue(FString::Printf(TEXT("[%s, attempt %d] the result is never NaN"), Case.Description, Attempt),
				!Result.ContainsNaN());

			// DISTANCE — EXACTLY SidestepDistance from the unit, never a scaled or squared value.
			TestEqual(FString::Printf(TEXT("[%s, attempt %d] the waypoint is exactly SidestepDistance from the unit"),
				Case.Description, Attempt),
				static_cast<float>(FVector::Dist(Result, Case.Location)), Tuning.SidestepDistance, GeometryTolerance);

			// Z — TAKEN FROM THE UNIT, NEVER FROM THE GOAL. A sidestep that inherited the goal's
			// Z would ask a ground unit to walk to a point in the air.
			TestEqual(FString::Printf(TEXT("[%s, attempt %d] Z is preserved from the unit's own location"),
				Case.Description, Attempt),
				static_cast<float>(Result.Z), static_cast<float>(Case.Location.Z), GeometryTolerance);

			// PERPENDICULAR — the offset has zero projection on the travel direction. Asserted in
			// the XY plane, which is what the pinned contract promises ("the perpendicular is
			// taken in the XY plane"); the 3D dot is zero too because the offset's Z is zero.
			const FVector Offset = Result - Case.Location;
			TestEqual(FString::Printf(TEXT("[%s, attempt %d] the sidestep offset has no Z component"),
				Case.Description, Attempt), static_cast<float>(Offset.Z), 0.f, GeometryTolerance);

			// ⚠️ The guard is a genuine degeneracy test, not a convenience: a goal directly
			// overhead has NO travel direction in the XY plane, so "perpendicular to it" names
			// nothing. Those cases are covered by the fallback-axis assertions below instead.
			const FVector TravelXY(Case.Goal.X - Case.Location.X, Case.Goal.Y - Case.Location.Y, 0.f);
			if (TravelXY.SizeSquared() > 1.e-4)
			{
				const FVector TravelDir = TravelXY.GetSafeNormal();
				const float Projection = static_cast<float>(FVector::DotProduct(Offset, TravelDir));

				TestEqual(FString::Printf(TEXT("[%s, attempt %d] the offset is PERPENDICULAR to the travel direction (projection %.4f uu)"),
					Case.Description, Attempt, Projection), Projection, 0.f, GeometryTolerance);
			}
		}
	}

	// ⭐ THE DOCUMENTED CONTINUITY CLAIM, ASSERTED RATHER THAN TAKEN ON TRUST: the degenerate
	// fallback axis (+Y) is chosen because it is EXACTLY what the formula yields for a unit
	// facing world +X. If somebody "simplifies" the fallback to +X or a random axis, the
	// degenerate answer stops being continuous with the normal one and this goes red.
	{
		const FVector Origin = FVector(70.f, -40.f, 5.f);
		const FVector FacingX = FSiegeStuckStatics::ComputeSidestepGoal(Origin, Origin + FVector(500.f, 0.f, 0.f), 350.f, 0);
		const FVector Degenerate = FSiegeStuckStatics::ComputeSidestepGoal(Origin, Origin, 350.f, 0);

		TestEqual(TEXT("⭐ The degenerate fallback equals the +X-facing answer — the axis is continuous, not arbitrary"),
			Degenerate, FacingX, GeometryTolerance);
		TestEqual(TEXT("⭐ ...and that answer is the unit's location offset along world +Y"),
			Degenerate, Origin + FVector(0.f, 350.f, 0.f), GeometryTolerance);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  14. CASE 9b — THE SIDE ALTERNATES ON Attempt PARITY, ACROSS THE WHOLE int32 RANGE
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ WOULD CATCH THE `% 2 == 0` BUG THE IMPLEMENTATION EXPLICITLY AVOIDS: `%` on a negative
 *  int32 yields -1 for odd values, so `% 2 == 0` would send every negative-odd Attempt to the
 *  SAME side as the evens. `& 1` is total over the whole range.
 *
 *  ⚠️ SEE TEST 19 BEFORE READING THIS AS COVERAGE OF THE FEATURE: the shipped call sites cannot
 *  actually vary Attempt for one unit, so this contract is real but currently unreachable.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSidestepParityTest,
	"Siegebound.Nav.Stuck.SidestepGoalAlternatesOnAttemptParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSidestepParityTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	const FVector Location(1200.f, -800.f, 24.f);
	const FVector Goal(1200.f + 2000.f, -800.f, 24.f);   // travelling due +X
	constexpr float Distance = 350.f;

	// Travelling +X, the perpendicular is +Y; even => +Y, odd => -Y.
	const FVector Even = Location + FVector(0.f, Distance, 0.f);
	const FVector Odd  = Location + FVector(0.f, -Distance, 0.f);

	struct FParityCase { int32 Attempt; bool bEven; const TCHAR* Description; };

	const FParityCase Cases[] =
	{
		{ 0,           true,  TEXT("0")           },
		{ 1,           false, TEXT("1")           },
		{ 2,           true,  TEXT("2")           },
		{ 3,           false, TEXT("3")           },
		{ 100,         true,  TEXT("100")         },
		{ 101,         false, TEXT("101")         },
		{ -1,          false, TEXT("-1")          },   // ⛔ the `% 2 == 0` trap
		{ -2,          true,  TEXT("-2")          },
		{ -3,          false, TEXT("-3")          },   // ⛔ the `% 2 == 0` trap
		{ -100,        true,  TEXT("-100")        },
		{ MAX_int32,   false, TEXT("MAX_int32")   },
		{ MIN_int32,   true,  TEXT("MIN_int32")   },   // wrapped past INT32_MAX
	};

	for (const FParityCase& Case : Cases)
	{
		const FVector Result = FSiegeStuckStatics::ComputeSidestepGoal(Location, Goal, Distance, Case.Attempt);

		TestEqual(FString::Printf(TEXT("Attempt %s steps to the %s side"), Case.Description, Case.bEven ? TEXT("+Y") : TEXT("-Y")),
			Result, Case.bEven ? Even : Odd, GeometryTolerance);

		TestTrue(FString::Printf(TEXT("Attempt %s produces no NaN"), Case.Description), !Result.ContainsNaN());
	}

	// AND THE TWO SIDES ARE GENUINELY OPPOSITE — mirrored through the unit, not merely different.
	{
		const FVector A = FSiegeStuckStatics::ComputeSidestepGoal(Location, Goal, Distance, 0);
		const FVector B = FSiegeStuckStatics::ComputeSidestepGoal(Location, Goal, Distance, 1);

		TestEqual(TEXT("The even and odd waypoints are exact mirror images through the unit"),
			(A - Location) + (B - Location), FVector::ZeroVector, GeometryTolerance);
		TestEqual(TEXT("...and are 2 x SidestepDistance apart"),
			static_cast<float>(FVector::Dist(A, B)), 2.f * Distance, GeometryTolerance);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  15. CASE 9c — FULLY DETERMINISTIC
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ "same inputs => same output, every call, forever. No FMath::Rand, no time read, no world
 *  query, no nav projection, no state." A sidestep that varied between calls would be
 *  unreproducible in a bug report and would put an RNG draw on a per-unit hot path.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSidestepDeterminismTest,
	"Siegebound.Nav.Stuck.SidestepGoalIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSidestepDeterminismTest::RunTest(const FString& Parameters)
{
	const FVector Location(-3333.f, 777.f, 41.5f);
	const FVector Goal(6000.f, -2200.f, 300.f);
	constexpr float Distance = 350.f;

	const FVector Reference = FSiegeStuckStatics::ComputeSidestepGoal(Location, Goal, Distance, 0);

	// 1000 identical calls, BIT-FOR-BIT identical answers. Exact equality on purpose — a
	// tolerance here would hide exactly the drift this test exists to catch.
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		const FVector Again = FSiegeStuckStatics::ComputeSidestepGoal(Location, Goal, Distance, 0);
		if (!(Again == Reference))
		{
			AddError(FString::Printf(TEXT("Call %d returned %s, not the reference %s — ComputeSidestepGoal is NOT deterministic."),
				Index, *Again.ToString(), *Reference.ToString()));
			return false;
		}
	}
	TestTrue(TEXT("1000 identical calls returned bit-identical waypoints"), true);

	// AND INTERLEAVING OTHER INPUTS CANNOT DISTURB IT — proof there is no hidden static state.
	for (int32 Index = 0; Index < 200; ++Index)
	{
		FSiegeStuckStatics::ComputeSidestepGoal(FVector(Index * 13.f, Index * -7.f, Index * 2.f),
			Goal, Distance + Index, Index);
	}

	const FVector AfterInterleaving = FSiegeStuckStatics::ComputeSidestepGoal(Location, Goal, Distance, 0);
	TestTrue(TEXT("⛔ 200 interleaved calls with other inputs left the answer unchanged (no hidden static state)"),
		AfterInterleaving == Reference);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  16. CASE 9d — THE DEGENERATE INPUTS PRODUCE NO NaN
// ════════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSidestepDegenerateTest,
	"Siegebound.Nav.Stuck.SidestepGoalDegenerateInputsAreSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSidestepDegenerateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	const FVector Location(250.f, -125.f, 33.f);

	// ── (a) Goal == Location: a zero-length lateral. The stable +Y fallback, NOT a NaN. ──────
	{
		const FVector EvenSide = FSiegeStuckStatics::ComputeSidestepGoal(Location, Location, 350.f, 0);
		const FVector OddSide  = FSiegeStuckStatics::ComputeSidestepGoal(Location, Location, 350.f, 1);

		TestTrue(TEXT("Goal == Location produces no NaN (even)"), !EvenSide.ContainsNaN());
		TestTrue(TEXT("Goal == Location produces no NaN (odd)"), !OddSide.ContainsNaN());
		TestEqual(TEXT("Goal == Location falls back to world +Y at the full distance"),
			EvenSide, Location + FVector(0.f, 350.f, 0.f), GeometryTolerance);
		TestEqual(TEXT("...and parity still flips the fallback side"),
			OddSide, Location + FVector(0.f, -350.f, 0.f), GeometryTolerance);
		TestEqual(TEXT("...at exactly SidestepDistance"),
			static_cast<float>(FVector::Dist(EvenSide, Location)), 350.f, GeometryTolerance);
	}

	// ── (b) A GOAL DIRECTLY OVERHEAD: the same zero-length lateral, same fallback. ───────────
	{
		const FVector Overhead = FSiegeStuckStatics::ComputeSidestepGoal(Location, Location + FVector(0.f, 0.f, 5000.f), 350.f, 0);
		TestTrue(TEXT("A goal directly overhead produces no NaN"), !Overhead.ContainsNaN());
		TestEqual(TEXT("A goal directly overhead uses the same +Y fallback"),
			Overhead, Location + FVector(0.f, 350.f, 0.f), GeometryTolerance);
	}

	// ── (c) NON-POSITIVE AND NON-FINITE DISTANCES RETURN THE LOCATION UNCHANGED ─────────────
	{
		const float NaNDistance = MakeQuietNaN();
		TestTrue(TEXT("The test's NaN distance really is NaN (a vacuous pass guard)"), FMath::IsNaN(NaNDistance));

		const float BadDistances[] = { 0.f, -1.f, -350.f, NaNDistance };
		for (const float Distance : BadDistances)
		{
			const FVector Result = FSiegeStuckStatics::ComputeSidestepGoal(Location, Location + FVector(900.f, 0.f, 0.f), Distance, 0);

			TestTrue(TEXT("A non-positive or NaN SidestepDistance produces no NaN"), !Result.ContainsNaN());
			TestEqual(TEXT("⛔ A non-positive or NaN SidestepDistance returns the unit's own location — the caller issues an already-satisfied move rather than acting on garbage"),
				Result, Location, GeometryTolerance);
		}
	}

	// ── (d) AN EXTREME BUT VALID DISTANCE STILL LANDS ON THE PERPENDICULAR ──────────────────
	{
		const FVector Huge = FSiegeStuckStatics::ComputeSidestepGoal(Location, Location + FVector(1.f, 0.f, 0.f), 1.e6f, 0);
		TestTrue(TEXT("A very large SidestepDistance produces no NaN"), !Huge.ContainsNaN());
		TestEqual(TEXT("...and still lands exactly that far away"),
			static_cast<float>(FVector::Dist(Huge, Location)), 1.e6f, 1.f);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  17. Reset CLEARS EVERY FIELD
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  Reset is what TASK-532 calls at all five lease-clear sites and what Evaluate calls on
 *  Abandon. It is implemented as an assignment from a default-constructed instance so it cannot
 *  drift from the field defaults or miss a field somebody adds later — asserted field by field
 *  AND as a whole-struct byte comparison, so a NEW field added without a default is caught too.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckResetTest,
	"Siegebound.Nav.Stuck.ResetClearsEveryField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckResetTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FSiegeStuckState State;
	State.ProgressAnchor         = FVector(9999.f, -9999.f, 500.f);
	State.StalledSeconds         = 5.75f;
	State.SecondsSinceEscalation = 0.9f;
	State.EscalationLevel        = static_cast<uint8>(2);
	State.bHasAnchor             = true;

	FSiegeStuckStatics::Reset(State);

	TestEqual(TEXT("Reset clears ProgressAnchor"), State.ProgressAnchor, FVector::ZeroVector, GeometryTolerance);
	TestEqual(TEXT("Reset clears StalledSeconds"), State.StalledSeconds, 0.f, ClockTolerance);
	TestEqual(TEXT("Reset clears SecondsSinceEscalation"), State.SecondsSinceEscalation, 0.f, ClockTolerance);
	TestEqual(TEXT("Reset clears EscalationLevel"), static_cast<int32>(State.EscalationLevel), 0);
	TestTrue(TEXT("Reset drops bHasAnchor, so the next Evaluate re-anchors"), !State.bHasAnchor);

	// ⚠️ A BYTE COMPARISON AGAINST A DEFAULT-CONSTRUCTED INSTANCE WAS CONSIDERED AND DELIBERATELY
	// REJECTED, and this is worth stating because it looks like the obvious drift guard:
	// FSiegeStuckState is 24 bytes of FVector + 2 floats + 2 single-byte fields, so it carries
	// TRAILING PADDING. Padding bytes are indeterminate in both instances and the implicit
	// copy-assignment Reset performs is memberwise, so a Memcmp here would be an INTERMITTENT
	// test rather than a strict one. ⇒ The five named field assertions above are the guard, and
	// a field added to the struct without a matching assertion here is caught by review
	// (`NAV-§8` pins the field list) rather than by a flaky byte compare.

	// AND RESET IS IDEMPOTENT — the five lease-clear sites may fire in any combination.
	FSiegeStuckStatics::Reset(State);
	FSiegeStuckStatics::Reset(State);
	TestTrue(TEXT("Reset is idempotent"), !State.bHasAnchor && State.StalledSeconds == 0.f);

	// A RESET MID-LADDER GENUINELY RESTARTS THE CLIMB AT RUNG 1.
	{
		FStallHarness Harness;
		int32 Polls = 0;
		PollUntilFire(Harness, PollSeconds, 40, Polls);                       // Sidestep
		PollUntilFire(Harness, PollSeconds, 40, Polls);                       // WidenAndRepath
		TestEqual(TEXT("Mid-ladder the unit is at level 2"), static_cast<int32>(Harness.State.EscalationLevel), 2);

		FSiegeStuckStatics::Reset(Harness.State);

		TestTrue(TEXT("After an external Reset the next rung is Sidestep again, not Abandon"),
			PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
		TestEqual(TEXT("...and it takes 7 polls again (1 re-anchor + 6 accumulating)"), Polls, 7);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  18. ⚠️ KNOWN HOLE 1 (PINNED, NOT FIXED) — THE rung-0 SPEED CLAUSE IS AN `OR`
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⚠️⚠️ THIS TEST ASSERTS WHAT SHIPS, AND WHAT SHIPS MAY NOT BE WHAT JONATHAN WANTS.
 *  ⛔ TASK-536 MAY NOT CHANGE IT — TASK-537 RULES.
 *
 *  The rung-0 condition is `!bAdvancing OR speed >= MinSpeedSq OR escaped ProgressRadius`.
 *  Because the speed clause is an OR, a unit SLIDING ALONG A ROCK'S COLLIDER (or oscillating in
 *  place) at >= 50 uu/s re-anchors on EVERY poll and is NEVER rescued — even though its NET
 *  displacement is zero, which is the thing that actually matters. The displacement clause alone
 *  would catch it; the speed clause is a cheap early-out that strictly WEAKENS it.
 *
 *  ⚠️ THIS IS NOT A CLAIM ABOUT THE OBSERVED WEDGE SHAPE. `NAV-§0` ruling 1 is explicit that
 *  Jonathan has not tracked when the wedging happens, and no test, handoff or finding may be
 *  written as if either hypothesis were established. It is a PLAYTEST RISK TO WATCH: if TASK-539
 *  reports "units still wedge on rocks", this is the first thing to check, and both fixes (drop
 *  the speed clause, or lower MinSpeedSq) are inside the pinned signature.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckSlidingUnitTest,
	"Siegebound.Nav.Stuck.SlidingUnitNeverEscalates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckSlidingUnitTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	FStallHarness Harness;
	Harness.bAdvancing     = true;     // a live move request
	Harness.VelocitySizeSq = 2601.f;   // 51 uu/s — just over the 50 uu/s threshold
	Harness.Location       = FVector(700.f, 700.f, 0.f);

	// 240 polls == 60 s of scraping along a collider with ZERO net progress. The unit oscillates
	// a few uu about one point — well inside ProgressRadius — so displacement never rescues it
	// either; only the velocity clause is keeping it out of the ladder.
	FActionTally Tally;
	for (int32 Index = 0; Index < 240; ++Index)
	{
		Harness.Location = FVector(700.f + ((Index % 2) ? 4.f : -4.f), 700.f, 0.f);
		Tally.Record(Harness.Poll(PollSeconds));
	}

	TestEqual(TEXT("⚠️ SHIPPED BEHAVIOUR, PINNED: 60 s of sliding at 51 uu/s with zero net progress fires ZERO rungs"),
		Tally.Fires(), 0);
	TestEqual(TEXT("⚠️ ...because the speed clause re-anchors every poll, so no stall time ever accrues"),
		Harness.State.StalledSeconds, 0.f, ClockTolerance);
	TestTrue(TEXT("⚠️ ...and the net displacement over the whole 60 s stayed well inside ProgressRadius"),
		FVector::Dist(Harness.Location, FVector(700.f, 700.f, 0.f)) < Harness.Tuning.ProgressRadius);

	// THE COUNTERFACTUAL, MEASURED IN THE SAME TEST: the identical unit one uu/s slower IS
	// rescued. That is what makes this a THRESHOLD hole rather than a broken ladder.
	{
		FStallHarness Slower;
		Slower.VelocitySizeSq = 2499.f;   // 49.99 uu/s
		Slower.Location       = FVector(700.f, 700.f, 0.f);

		FActionTally SlowTally;
		for (int32 Index = 0; Index < 240; ++Index)
		{
			Slower.Location = FVector(700.f + ((Index % 2) ? 4.f : -4.f), 700.f, 0.f);
			SlowTally.Record(Slower.Poll(PollSeconds));
		}

		TestTrue(FString::Printf(TEXT("The SAME unit at 49.99 uu/s instead of 51 uu/s IS rescued (%d rungs)"),
			SlowTally.Fires()), SlowTally.Fires() > 0);

		AddInfo(FString::Printf(TEXT("⚠️ HOLE 1 FOR TASK-537 (`NAV-§3` rung 0 is an OR): at 51 uu/s with zero net progress the ladder fires 0 rungs over 60 s; at 49.99 uu/s the same unit fires %d. A unit scraping along a collider above MinSpeedSq is never rescued. Shipped behaviour asserted as-is; flagged as a TASK-539 playtest watch, NOT fixed here."),
			SlowTally.Fires()));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  19. ⭐ THE SIDESTEP SIDE ALTERNATES ACROSS SUCCESSIVE STALLS (was HOLE 2 — RULED A BLOCKER,
//      FIXED IN SHIPPED SOURCE AT LOOP 1, AND THIS TEST WAS INVERTED IN THE SAME LOOP)
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ THIS TEST USED TO ASSERT THE HOLE. It was `Siegebound.Nav.Stuck.SidestepSideIsConstantForOneUnit`
 *  and it proved, three stall cycles deep, that a unit sidesteps to the IDENTICAL waypoint every
 *  time. TASK-537 ruled that a **BLOCKER** (*"the ladder detects the stall and cannot escape it —
 *  which is the entire point of the feature"*), the fix landed in shipped source, and the test now
 *  proves the OPPOSITE. ⛔ IF THIS TEST IS EVER REVERTED TO ASSERTING CONSTANCY, THE FIX IS GONE.
 *
 *  THE DEFECT IT GUARDS, EACH LINK VERIFIED AT THE ARTIFACT:
 *    • Sidestep fires EXACTLY ONCE per stall (brake 2 is monotonic) — so Attempt can only vary
 *      ACROSS stalls;
 *    • FSiegeStuckState (pinned, `NAV-§8`) HAS NO ATTEMPT COUNTER, and Reset clears the whole
 *      struct on every Abandon and every re-anchor — so ⛔ NOTHING STORED THERE SURVIVES A STALL;
 *    • the ORIGINAL shipped expression was `EscalationLevel + (GetUniqueID() % 2)`, and
 *      EscalationLevel is ALWAYS 1 when Sidestep is returned (Evaluate assigns DesiredLevel
 *      before its switch)  ⇒  Attempt was a per-unit CONSTANT, 1 or 2, forever
 *      ⇒  ⛔ A UNIT WEDGED ON A ROCK'S LEFT FACE SIDESTEPPED INTO THE ROCK EVERY SINGLE TIME.
 *
 *  ⭐ THE FIX, AND WHY IT IS SHAPED THIS WAY: `ASummonedUnit::SidestepAttemptCount` — a free-running
 *  per-unit `uint8`, deliberately declared OUTSIDE FSiegeStuckState, because Reset clearing that
 *  struct between stalls is precisely what caused the bug. ⛔ No pinned signature moved and no
 *  field was added to a pinned struct; `NAV-§8` is untouched.
 *
 *  ⛔ WHAT THIS TEST CAN AND CANNOT CLAIM. It MODELS the corrected call-site expression
 *  (ShippedAttemptForSidestep) against a REAL ladder run, so the stall cycles, the Resets and the
 *  EscalationLevel readings are genuine. It does NOT execute GetUniqueID() and does NOT construct a
 *  unit — that needs a world (§5 of handoffs/TASK-536-programmer.md). The claim here is about the
 *  ARITHMETIC of the corrected expression across real stalls; TASK-538's compile and TASK-539's
 *  playtest settle the rest.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckAttemptAlternatesTest,
	"Siegebound.Nav.Stuck.SidestepSideAlternatesAcrossStallsForOneUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckAttemptAlternatesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	constexpr uint32 PretendUniqueID = 12345678u;   // EVEN, so the unit-id term contributes 0
	const FVector UnitLocation(0.f, 0.f, 0.f);
	const FVector RockwardGoal(2000.f, 0.f, 0.f);   // the goal beyond the rock, due +X

	// ⭐ THE UNIT'S OWN COUNTER, LIVING OUTSIDE THE LADDER STATE — exactly as
	// ASummonedUnit::SidestepAttemptCount does. It is declared HERE, outside the stall loop and
	// ⛔ deliberately NOT inside FStallHarness (which owns the FSiegeStuckState the ladder Resets),
	// because SURVIVING THOSE RESETS IS THE WHOLE PROPERTY UNDER TEST.
	uint8 SidestepAttemptCount = 0;

	FStallHarness Harness;
	Harness.Location = UnitLocation;

	TArray<FVector> SidestepGoals;
	TArray<int32>   Attempts;

	// THREE CONSECUTIVE FULL STALL CYCLES: Sidestep -> Widen -> Abandon -> re-anchor -> repeat.
	// Each time, compute the sidestep waypoint exactly as the shipped call site would.
	for (int32 Cycle = 0; Cycle < 3; ++Cycle)
	{
		int32 Polls = 0;
		const ESiegeStuckAction Fired = PollUntilFire(Harness, PollSeconds, 60, Polls);

		TestTrue(FString::Printf(TEXT("Cycle %d begins with Sidestep (got %s)"), Cycle, Describe(Fired)),
			Fired == ESiegeStuckAction::Sidestep);

		// ⛔ STILL TRUE, AND IT IS THE REASON THE COUNTER MAY NOT LIVE IN THE LADDER STATE:
		// EscalationLevel reads 1 at EVERY Sidestep, on every stall, forever. Any Attempt term
		// derived from it is a constant — which is the defect this test now guards against.
		TestEqual(FString::Printf(TEXT("⛔ Cycle %d: EscalationLevel is 1 at the moment Sidestep is returned — so it can NEVER be the varying term"), Cycle),
			static_cast<int32>(Harness.State.EscalationLevel), 1);

		const int32 Attempt = ShippedAttemptForSidestep(SidestepAttemptCount, PretendUniqueID);
		Attempts.Add(Attempt);
		SidestepGoals.Add(FSiegeStuckStatics::ComputeSidestepGoal(
			Harness.Location, RockwardGoal, Harness.Tuning.SidestepDistance, Attempt));

		// Run the rest of the ladder out so the next loop starts a genuinely fresh stall.
		PollUntilFire(Harness, PollSeconds, 60, Polls);   // WidenAndRepath
		PollUntilFire(Harness, PollSeconds, 60, Polls);   // Abandon (auto-Resets)
		Harness.Poll(PollSeconds);                        // the re-anchor poll
	}

	TestEqual(TEXT("Three full stall cycles were observed"), SidestepGoals.Num(), 3);

	// ⭐ THE COUNTER ADVANCED ONCE PER FIRED RUNG AND SURVIVED EVERY Reset IN BETWEEN — three
	// stalls, three Abandons (each an internal Reset), three re-anchors, and it still reads 3.
	TestEqual(TEXT("⭐ The unit's free-running counter advanced once per fired Sidestep and SURVIVED all three Resets"),
		static_cast<int32>(SidestepAttemptCount), 3);

	// (a) ⭐ THE FIX ITSELF: CONSECUTIVE STALLS GO TO OPPOSITE SIDES.
	for (int32 Index = 1; Index < Attempts.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("⭐ stall %d computes a DIFFERENT Attempt PARITY from stall %d (%d vs %d) — the counter carried across the stall"),
			Index, Index - 1, Attempts[Index], Attempts[Index - 1]),
			(Attempts[Index] & 1) != (Attempts[Index - 1] & 1));

		TestFalse(FString::Printf(TEXT("⭐ stall %d therefore sidesteps to a DIFFERENT waypoint than stall %d — a unit wedged on a rock's left face tries the RIGHT next time"),
			Index, Index - 1),
			SidestepGoals[Index].Equals(SidestepGoals[Index - 1], GeometryTolerance));

		// Stronger than "different": the two offsets must SUM TO ZERO about the unit, i.e. they
		// are exact mirrors. A merely-different waypoint could still be on the same side.
		TestEqual(FString::Printf(TEXT("⭐ …and stalls %d and %d are EXACT MIRRORS about the unit, not merely different"), Index - 1, Index),
			(SidestepGoals[Index] - UnitLocation) + (SidestepGoals[Index - 1] - UnitLocation),
			FVector::ZeroVector, GeometryTolerance);
	}

	// It ALTERNATES rather than drifting: the third stall comes back to the first stall's side.
	TestEqual(TEXT("⭐ The third stall returns to the FIRST stall's waypoint — left / right / left, an alternation and not a drift"),
		SidestepGoals[2], SidestepGoals[0], GeometryTolerance);

	// (b) ⚠️ THE WRAP, ASSERTED RATHER THAN ASSUMED. SidestepAttemptCount is a `uint8` and it is
	// FREE-RUNNING, so a long-lived, frequently-wedged unit WILL wrap it. That is intended and
	// harmless: only `Attempt & 1` is ever read and 256 is even, so the parity sequence continues
	// unbroken across the boundary. ⛔ Nothing anywhere may depend on this value monotonically.
	{
		uint8 NearWrap = 254;
		constexpr uint32 EvenNeighbourId = 1000u;   // even ⇒ the id term contributes 0

		const int32 A254 = ShippedAttemptForSidestep(NearWrap, EvenNeighbourId);
		const int32 A255 = ShippedAttemptForSidestep(NearWrap, EvenNeighbourId);

		TestEqual(TEXT("⚠️ The uint8 counter WRAPPED 255 -> 0 rather than saturating"), static_cast<int32>(NearWrap), 0);

		const int32 A000 = ShippedAttemptForSidestep(NearWrap, EvenNeighbourId);
		const int32 A001 = ShippedAttemptForSidestep(NearWrap, EvenNeighbourId);

		TestEqual(TEXT("Attempt 254 is read before the increment"), A254, 254);
		TestEqual(TEXT("Attempt 255 is read before the wrap"),      A255, 255);
		TestEqual(TEXT("Attempt 0 is read after the wrap"),         A000, 0);
		TestEqual(TEXT("Attempt 1 follows it"),                     A001, 1);

		const FVector G254 = FSiegeStuckStatics::ComputeSidestepGoal(UnitLocation, RockwardGoal, 350.f, A254);
		const FVector G255 = FSiegeStuckStatics::ComputeSidestepGoal(UnitLocation, RockwardGoal, 350.f, A255);
		const FVector G000 = FSiegeStuckStatics::ComputeSidestepGoal(UnitLocation, RockwardGoal, 350.f, A000);
		const FVector G001 = FSiegeStuckStatics::ComputeSidestepGoal(UnitLocation, RockwardGoal, 350.f, A001);

		TestEqual(TEXT("⭐ The side still alternates ACROSS THE WRAP: the first post-wrap goal mirrors the last pre-wrap one"),
			(G000 - UnitLocation) + (G255 - UnitLocation), FVector::ZeroVector, GeometryTolerance);
		TestEqual(TEXT("…254 and 0 are both even, so both sidestep to the SAME side"), G000, G254, GeometryTolerance);
		TestEqual(TEXT("…255 and 1 are both odd, so both sidestep to the OTHER side"),  G001, G255, GeometryTolerance);
	}

	// (c) ✅ THE HALF THAT ALREADY WORKED, PRESERVED UNCHANGED IN SUBSTANCE: two neighbours with
	// different unit-id parity go OPPOSITE ways, so a clump wedged on one rock does not fan into
	// each other. ⛔ TASK-537 required this assertion be KEPT while the per-unit half was inverted.
	{
		uint8 NeighbourACount = 0;
		uint8 NeighbourBCount = 0;

		const int32 EvenIdAttempt = ShippedAttemptForSidestep(NeighbourACount, 1000u);   // 0 + 0 == 0 (even)
		const int32 OddIdAttempt  = ShippedAttemptForSidestep(NeighbourBCount, 1001u);   // 0 + 1 == 1 (odd)

		TestEqual(TEXT("An even unit id yields an EVEN Attempt on its first sidestep"), EvenIdAttempt, 0);
		TestEqual(TEXT("An odd unit id yields an ODD Attempt on its first sidestep"),   OddIdAttempt,  1);

		const FVector NeighbourA = FSiegeStuckStatics::ComputeSidestepGoal(UnitLocation, RockwardGoal, 350.f, EvenIdAttempt);
		const FVector NeighbourB = FSiegeStuckStatics::ComputeSidestepGoal(UnitLocation, RockwardGoal, 350.f, OddIdAttempt);

		TestEqual(TEXT("✅ Two neighbours of opposite unit-id parity DO sidestep to opposite sides (the de-correlation TASK-532 claims)"),
			(NeighbourA - UnitLocation) + (NeighbourB - UnitLocation), FVector::ZeroVector, GeometryTolerance);

		// ⭐ AND THE DE-CORRELATION SURVIVES THE ALTERNATION, which is the interaction the fix
		// could plausibly have broken: the id term is a FIXED PHASE OFFSET, so two neighbours
		// stepping in lockstep stay on opposite sides at EVERY attempt index, wrap included.
		for (int32 Step = 1; Step < 8; ++Step)
		{
			const int32 NextA = ShippedAttemptForSidestep(NeighbourACount, 1000u);
			const int32 NextB = ShippedAttemptForSidestep(NeighbourBCount, 1001u);

			TestTrue(FString::Printf(TEXT("✅ At lockstep attempt %d the two neighbours are STILL on opposite sides (%d vs %d)"),
				Step, NextA, NextB), (NextA & 1) != (NextB & 1));
		}
	}

	AddInfo(TEXT("⭐ HOLE 2 IS CLOSED (qa/TASK-537.md BLOCKER, loop 1). The shipped Attempt is now SidestepAttemptCount++ + (GetUniqueID() % 2), where SidestepAttemptCount is a free-running uint8 on ASummonedUnit — deliberately OUTSIDE FSiegeStuckState, because Reset clears that struct on every Abandon and every re-anchor and that is exactly what made the old EscalationLevel term a per-unit constant. Three consecutive real stall cycles now produce mirrored waypoints (left/right/left); the uint8 wrap at 255->0 preserves the parity because 256 is even; and the neighbour de-correlation is unchanged. ⛔ No pinned NAV-§8 signature or struct was touched."));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  20. ⚠️ KNOWN HOLE 3 (PINNED, NOT FIXED) — NotifyMoveBlocked's CLOCK BUMP IS DISCARDED
// ════════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⚠️ TASK-532's FLAG C SAYS THIS EVIDENCE IS "usually discarded on the very next poll". THE PURE
 *  HALF OF THAT IS TESTABLE AND IS ASSERTED HERE, AND IT IS STRONGER THAN "usually":
 *
 *    • `EPathFollowingResult::Blocked` means the path request FINISHED. The controller's status is
 *      therefore Idle until something re-issues, and the re-issue happens LATER in the same poll
 *      than the watchdog call (SummonedUnit.cpp:1348 sits above the profile dispatch at :2440).
 *    • ⇒ The next Evaluate sees bAdvancing == false, takes the re-anchor branch, and calls Reset —
 *      WIPING the StalledSeconds bump NotifyMoveBlocked just wrote.
 *  ⇒ IN THE SCENARIO IT WAS WRITTEN FOR, THE BUMP CONTRIBUTES EXACTLY NOTHING.
 *
 *  ⛔ WHAT THIS TEST CAN AND CANNOT CLAIM. It MODELS NotifyMoveBlocked's two writes on a bare
 *  state; it does NOT call the shipped function, which needs an actor, a controller and a world.
 *  The "a Blocked verdict implies Idle on the next poll" step is engine-contract REASONING, not
 *  something asserted here. What IS asserted is the ladder's response to the state in both cases.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeStuckBlockedBumpTest,
	"Siegebound.Nav.Stuck.BlockedClockBumpIsDiscardedWhenNotAdvancing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeStuckBlockedBumpTest::RunTest(const FString& Parameters)
{
	using namespace SiegeStuckTestUtils;

	// ── (a) THE DISCARD. bAdvancing == false on the poll after the verdict. ─────────────────
	{
		FStallHarness Harness;
		Harness.Location = FVector(1500.f, -400.f, 20.f);

		ModelNotifyMoveBlockedWrites(Harness.State, Harness.Location, Harness.Tuning);
		TestEqual(TEXT("The modelled bump raised StalledSeconds to SidestepSeconds"),
			Harness.State.StalledSeconds, Harness.Tuning.SidestepSeconds, ClockTolerance);
		TestTrue(TEXT("...and set the anchor, so Evaluate's no-anchor branch is not the wiper"),
			Harness.State.bHasAnchor);

		Harness.bAdvancing = false;   // what a finished (Blocked) request leaves behind

		const ESiegeStuckAction Action = Harness.Poll(PollSeconds);
		TestTrue(FString::Printf(TEXT("The poll after a Blocked verdict returns None (got %s)"), Describe(Action)),
			Action == ESiegeStuckAction::None);
		TestEqual(TEXT("⚠️ PINNED: the bump is WIPED — StalledSeconds is back to 0 and the evidence is gone"),
			Harness.State.StalledSeconds, 0.f, ClockTolerance);
		TestEqual(TEXT("⚠️ ...and the level is back to 0 too"), static_cast<int32>(Harness.State.EscalationLevel), 0);
	}

	// ── (b) THE CORROBORATING CASE: if the next poll DOES have a live request, the bump ──────
	//    survives — and ⛔ IT STILL DOES NOT BYPASS THE BRAKES (`NAV-§3`). Brake 1 holds the rung
	//    for a full EscalationCooldown, so the bump can only PULL the first rung forward, never
	//    fire one itself and never skip Sidestep to reach Abandon.
	{
		FStallHarness Harness;
		Harness.Location   = FVector(-2200.f, 900.f, 0.f);
		Harness.bAdvancing = true;

		ModelNotifyMoveBlockedWrites(Harness.State, Harness.Location, Harness.Tuning);

		// The very next poll must NOT fire, even though StalledSeconds already exceeds
		// SidestepSeconds: SecondsSinceEscalation is only 0.25.
		const ESiegeStuckAction Immediate = Harness.Poll(PollSeconds);
		TestTrue(FString::Printf(TEXT("⛔ The bump does NOT fire a rung on the next poll — brake 1 still applies (got %s)"),
			Describe(Immediate)), Immediate == ESiegeStuckAction::None);

		int32 Polls = 0;
		const ESiegeStuckAction Fired = PollUntilFire(Harness, PollSeconds, 40, Polls);
		TestTrue(FString::Printf(TEXT("⛔ When it does fire, the rung is Sidestep — never a jump to Abandon (got %s)"),
			Describe(Fired)), Fired == ESiegeStuckAction::Sidestep);
		TestEqual(TEXT("⛔ ...released exactly when SecondsSinceEscalation reached EscalationCooldown (3 further polls)"),
			Polls, 3);
		TestEqual(TEXT("⛔ ...i.e. 1.0 s after the bump, not instantly"),
			Harness.ElapsedSeconds, 1.0f, ClockTolerance);
	}

	// ── (c) THE BASELINE IT IS COMPARED AGAINST: the same unit with NO bump at all. ──────────
	{
		FStallHarness Harness;
		Harness.Location = FVector(-2200.f, 900.f, 0.f);

		int32 Polls = 0;
		TestTrue(TEXT("Without the bump, Sidestep still arrives"),
			PollUntilFire(Harness, PollSeconds, 40, Polls) == ESiegeStuckAction::Sidestep);
		TestEqual(TEXT("...but at 1.75 s (7 polls), not 1.0 s — so the bump IS worth 0.75 s when it survives"),
			Harness.ElapsedSeconds, 1.75f, ClockTolerance);
	}

	AddInfo(TEXT("⚠️ HOLE 3 FOR TASK-537: modelled headlessly, ASummonedUnit::NotifyMoveBlocked's clock bump is worth 0.75 s when the next poll still has a live request, and worth exactly nothing when it does not. A Blocked verdict means the request finished, so GetMoveStatus() is Idle on the next poll and Evaluate's re-anchor branch wipes the bump — the case it was written for is the case in which it does nothing. TASK-532 flagged this as 'largely inert' and could not fix it inside its own file; the shipped behaviour is asserted here, not changed. ⛔ The world half (that a Blocked verdict implies an Idle status next poll) is engine-contract reasoning and is NOT tested here."));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
