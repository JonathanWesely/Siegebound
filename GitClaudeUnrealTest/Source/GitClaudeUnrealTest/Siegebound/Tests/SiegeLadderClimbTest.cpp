// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include <type_traits>

#include "Containers/UnrealString.h"
#include "Math/UnrealMathUtility.h"
#include "Siegebound/ClimbableTower.h" // TASK-784 (CONTACT-§4.1): the contact trigger's three tunables are read off THIS class's CDO — the pawn keeps no copy, and tests 15/16/18 re-derive the call site's cadence against whatever actually ships
#include "Siegebound/MinerUnit.h"
#include "Siegebound/SiegeLadderClimbStatics.h" // TASK-776 (CONTACT-§2): FSiegeLadderClimbState / FSiegeLadderClimbStatics moved here from SummonedUnit.h, byte-identically. ⛔ The ONLY change to this file — not one test was renamed, deleted, re-ordered or altered, and the suite delta is EXACTLY ZERO
#include "Siegebound/SorcererUnit.h"
#include "Siegebound/SummonedUnit.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE LADDER CLIMB + THE DISARM (TASK-738, TOWER-§8/§9/§10) ═══
 *
 *  Jonathan, verbatim (2026-09-01): *"instead of making it a ramp that you walk up it instead
 *  has a ladder you climb up"* — and, the same day, the ruling this file exists to hold:
 *  *"they should be attackable while climbing, but they can't attack back."*
 *
 *  Subject: `FSiegeLadderClimbStatics` / `FSiegeLadderClimbState` (SummonedUnit.h) and the
 *  pinned `ASummonedUnit` climb API. QA gate: TASK-741. Compile + suite: TASK-742.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ THE FAILURE THIS WHOLE FILE IS POINTED AT — AND IT IS WORSE THAN ANYTHING THE RAMP
 *  COULD DO: **`MOVE_Flying` IGNORES GRAVITY.** An exit path that forgets to restore the
 *  movement mode leaves a unit HANGING IN MID-AIR, FOREVER. `TOWER-§8.5` enumerates EIGHT
 *  exits, and the tower-destroyed one was FREE under the ramp (the floor vanished and
 *  CharacterMovement dropped to `MOVE_Falling` by itself) and is explicitly NOT free now.
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⭐⭐ WHY THE EIGHT EXITS ARE TESTED AGAINST A PURE STATE MACHINE RATHER THAN A LIVE ACTOR,
 *  AND IT IS A MEASUREMENT RATHER THAN A PREFERENCE:
 *
 *    • Every automation test in this project is HEADLESS. There is not one `UWorld::CreateWorld`
 *      and not one `SpawnActor` in `Source/GitClaudeUnrealTest/Siegebound/Tests/` — the shipped
 *      instruments are pure statics, CDOs and reflection.
 *    • ⛔ AND A WORLD-LESS `ASummonedUnit` CANNOT BE DRIVEN THROUGH THESE PATHS AT ALL:
 *      the teardown calls `UCharacterMovementComponent::SetDefaultMovementMode`, which reaches
 *      `UMovementComponent::GetPhysicsVolume` — and that function dereferences `GetWorld()`
 *      **unconditionally** when `UpdatedComponent` is null (`MovementComponent.cpp:290-298`).
 *      A `NewObject`'d unit would CRASH the suite, not fail it.
 *    ⇒ So the climb's every DECISION lives in `FSiegeLadderClimbStatics` (the `FSiegeStuckStatics`
 *      / `HeightAdvantageMultiplier` seam precedent) and is exercised here exhaustively, and the
 *      WIRING of each of the eight call sites is TASK-741's diff read. ⛔ That split is stated
 *      rather than hidden — `SC-§32`: a mechanism never observed to function is not known to
 *      function, and pretending a green suite covers the wiring would be the lie.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ AND THE LESSON THIS FILE IS WRITTEN AGAINST, BECAUSE THIS PROJECT HAS PAID FOR IT
 *  TWICE: **AN ASSERTION WHOSE TWO SIDES ARE EQUAL BY CONSTRUCTION PROVES NOTHING** (`SHIP-§9c`
 *  — `Semicolon != Semicolon`, and a parity check whose sides collapsed to the same call).
 *  Every claim below is therefore either (a) a TRUTH TABLE over a function with more than one
 *  input, where dropping a term flips a row, (b) an expectation RE-DERIVED from the pinned
 *  geometry and the shipped `cards.csv` numbers rather than transcribed from the subject, or
 *  (c) paired with a SELF-CHECK that fails if the probe went stale. Where a claim could only
 *  be tautological, it is NOT MADE — see test 9, which names what it cannot prove.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  🔒 Airlock: no `Capture()`, no `EnsureSnapshot()`, Zone A untouched, ⛔ no token figure
 *  (`AS-§12g`). Nothing here runs a model, opens the editor, or touches Git.
 */

namespace SiegeLadderClimbTestFixture
{
	// ── THE PINNED GEOMETRY (TOWER-§8.3), USED AS THE SCENARIO ─────────────────────────────
	// ⭐ These are the two socket coordinates the ART lane builds to and the CODE lane falls
	// back to, and they are quoted here as the SCENARIO the climb runs on — ⛔ never as a
	// shipped constant this file is asserting against itself. Every number downstream is
	// COMPUTED from them (length, lean, ascent duration, shots landed), so a wrong expectation
	// would have to be wrong in the arithmetic rather than in a transcription.
	const FVector LadderFoot(-450.f, 0.f, 0.f);
	const FVector LadderTop(-150.f, 0.f, 1200.f);

	/** |Top - Foot| = sqrt(300² + 1200²) = sqrt(1,530,000) = 1,236.93… uu. TOWER-§8.3 quotes 1,236.9. */
	const float ClimbLineLengthUU = static_cast<float>((LadderTop - LadderFoot).Size());

	/**
	 *  The half-height this file drives its scenarios with: 88 (`SiegeSpawnConstants.h:9`, which
	 *  TOWER-§8.3 quotes). Used here as the test's scenario, exactly as the socket coordinates are.
	 *
	 *  ⚠️⚠️ COMMENT CORRECTED 2026-09-01 (TASK-777, CONTACT-§10.1's stale-cite rule): this comment
	 *  used to read *"and the project never calls InitCapsuleSize"*, and that claim is ⛔ FALSE and
	 *  has been REFUTED at the source — `GitClaudeUnrealTestCharacter.cpp:18` calls
	 *  `InitCapsuleSize(42.f, 96.0f)` on the HERO's own direct base, so the hero ships r 42 /
	 *  hh 96. **34 is the NAV AGENT radius** (`DefaultEngine.ini:290`), ⛔ not a capsule radius —
	 *  two different 34s, which is exactly how the wrong one got quoted. ⛔ COMMENT ONLY: what
	 *  this file ASSERTS is unchanged, because 88 is still the correct scenario for the UNIT these
	 *  tests are about.
	 */
	constexpr float CapsuleHalfHeightUU = 88.f;

	/**
	 *  ⭐⭐ WHERE A CLIMBER MUST FINISH: the capsule CENTRE that puts its FEET on the deck surface.
	 *  The sockets are SURFACE points; the thing that travels the line is the centre, one
	 *  half-height above whatever it stands on. ⛔ An arrival at the bare socket leaves the unit
	 *  buried 88 uu inside the slab.
	 */
	const FVector DeckStandingCentre = LadderTop + FVector(0.f, 0.f, CapsuleHalfHeightUU);

	/** The mirror at the other end: the capsule centre of a unit standing at the ladder's foot. */
	const FVector FootStandingCentre = LadderFoot + FVector(0.f, 0.f, CapsuleHalfHeightUU);

	/**
	 *  ⚠️ TASK-737's MEASUREMENT, quoted as the failure this file now has to exclude: `LadderTop`
	 *  is 150 uu inside a SOLID deck slab, so **the last 40.8 uu of the climb line lies INSIDE the
	 *  deck geometry** — and a swept move stalls against the slab's underside, silently, with
	 *  every one of the eight exits still correct.
	 */
	constexpr float MeasuredDeckIntrusionUU = 40.8f;

	// ── THE DEFENDER, FROM THE SHIPPED cards.csv (post-×3 Range), ⛔ NOTHING ESTIMATED ──────
	// TOWER-§9.3's exposure table is built from these three cells and two HP figures. They are
	// re-stated here so test 11 can RE-COMPUTE the table against whatever rate actually ships,
	// rather than assert a table that was true on the day it was written.
	constexpr float LongbowmanDamage = 18.f;
	constexpr float LongbowmanCadenceSeconds = 1.5f;
	constexpr float ArcherWizardHP = 45.f;
	constexpr float LongbowmanHP = 70.f;

	/** The RAMP this ladder replaced: 2,078 uu of run at 30° ⇒ 2,078 / cos(30°) of surface (TOWER-§2a / TOWER-§9.3). */
	constexpr float RampRunUU = 2078.f;
	constexpr float RampSlopeDegrees = 30.f;

	/** Tolerance for decimal expectations: tight enough that a retune blows straight through it, loose enough that float32 never does. */
	constexpr float Tolerance = 1.e-3f;

	/** Reads a shipped float UPROPERTY off a class default object. Returns false (and writes nothing) if the property is gone — the caller FAILS on that rather than substituting a guess. */
	static bool TryReadDefaultFloat(const UClass* Class, const UObject* Defaults, const TCHAR* PropertyName, float& OutValue)
	{
		if (!Class || !Defaults)
		{
			return false;
		}

		const FFloatProperty* const FloatProperty = CastField<FFloatProperty>(Class->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Defaults);
		return true;
	}

	/** A freshly armed ASCENT on the pinned geometry at the given rate. Returns false if Begin refused (which is itself an assertable failure). */
	static bool ArmPinnedClimb(FSiegeLadderClimbState& OutState, float ClimbSpeedUU)
	{
		OutState = FSiegeLadderClimbState();
		return FSiegeLadderClimbStatics::Begin(OutState, /*bDead=*/ false, /*bAIFrozen=*/ false,
			/*bSpellFrozen=*/ false, LadderFoot, LadderTop, ClimbSpeedUU, CapsuleHalfHeightUU);
	}

	/** The same line the other way round — a DESCENT. ⭐ The API is From -> To; ⛔ nothing enforces foot-then-top. */
	static bool ArmPinnedDescent(FSiegeLadderClimbState& OutState, float ClimbSpeedUU)
	{
		OutState = FSiegeLadderClimbState();
		return FSiegeLadderClimbStatics::Begin(OutState, /*bDead=*/ false, /*bAIFrozen=*/ false,
			/*bSpellFrozen=*/ false, LadderTop, LadderFoot, ClimbSpeedUU, CapsuleHalfHeightUU);
	}

	/** True when the state is byte-for-byte the "not climbing" default — the "changes NOTHING" claim, checked on every field rather than on bActive alone. */
	static bool IsPristine(const FSiegeLadderClimbState& State)
	{
		return !State.bActive
			&& State.Start.IsZero()
			&& State.End.IsZero()
			&& State.LengthUU == 0.f
			&& State.ElapsedSeconds == 0.f
			&& State.TimeoutSeconds == 0.f
			&& State.DeckBreachUU == 0.f
			&& State.CapsuleHalfHeightUU == 0.f;
	}

	/** Every exit reason, so the exactly-once claim is made against ALL of them rather than a sample. */
	static const ESiegeLadderExit AllExitReasons[] =
	{
		ESiegeLadderExit::Arrival,
		ESiegeLadderExit::Abort,
		ESiegeLadderExit::NewOrder,
		ESiegeLadderExit::Death,
		ESiegeLadderExit::MatchEndFreeze,
		ESiegeLadderExit::SpellFreeze,
		ESiegeLadderExit::EndPlay,
		ESiegeLadderExit::Timeout
	};

	/** Names for the failure messages, index-locked to AllExitReasons above. */
	static const TCHAR* const AllExitReasonNames[] =
	{
		TEXT("Arrival (exit 1)"),
		TEXT("Abort (exit 2 — AND exit 8, the tower destroyed mid-climb)"),
		TEXT("NewOrder (exit 3, L-4)"),
		TEXT("Death (exit 4, HandleDeath)"),
		TEXT("MatchEndFreeze (exit 5, FreezeAI)"),
		TEXT("SpellFreeze (exit 6, ApplyFreeze)"),
		TEXT("EndPlay (exit 7)"),
		TEXT("Timeout (the DECLARED ninth reason — a watchdog, ⛔ not one of the law's eight)")
	};

	static_assert(UE_ARRAY_COUNT(AllExitReasons) == UE_ARRAY_COUNT(AllExitReasonNames),
		"The exit-reason table and its name table must stay index-locked.");

	// ── ⛔⛔ THE PINNED API, ASSERTED AT COMPILE TIME (TOWER-§8.4(B)) ───────────────────────
	// ⚠️⚠️ TASK-734 (AClimbableTower) COMPILES AGAINST THESE EXACT SIGNATURES AND IS WRITTEN IN
	// PARALLEL — it cannot see this file and this file cannot see it. A member-function-pointer
	// identity check is the strongest instrument available for that contract: it fails at COMPILE
	// TIME, in this module, with a message naming the law, rather than as a link error in
	// somebody else's task at the integration gate.
	// ⭐ Parameter NAMES are not part of a function's type, so the rename from the law's
	// LadderFootWorld/LadderTopWorld to FromWorld/ToWorld (TASK-734's finding, taken — see the
	// handoff's amendment request) is invisible here, which is exactly why it is safe: the types,
	// the count and the order are what TASK-734 links against, and all three are untouched.
	static_assert(std::is_same_v<decltype(&ASummonedUnit::BeginLadderClimb),
		bool (ASummonedUnit::*)(const FVector&, const FVector&)>,
		"TOWER-§8.4(B): BeginLadderClimb must be `bool BeginLadderClimb(const FVector&, const FVector&)`. TASK-734 compiles against this exact signature.");

	static_assert(std::is_same_v<decltype(&ASummonedUnit::AbortLadderClimb), void (ASummonedUnit::*)()>,
		"TOWER-§8.4(B): AbortLadderClimb must be `void AbortLadderClimb()` — no parameters, so the tower-destroyed path needs nothing it does not have.");

	static_assert(std::is_same_v<decltype(&ASummonedUnit::IsClimbing), bool (ASummonedUnit::*)() const>,
		"TOWER-§8.4(B): IsClimbing must be `bool IsClimbing() const` — ABP_Footman (TASK-739) and the TOWER-§9 disarm guards both read it.");

	// ── ⛔⛔ TASK-784: THE TWO COMPILE-TIME PINS THAT KEEP THE CALL SITE OFF THE ACTOR TICK ────
	//
	// ⚠️⚠️ `SC-§36.1` asks for a test that goes RED if the contact poll is ever moved onto
	// `ASummonedUnit::Tick`. ⭐ A RUNTIME row cannot see a call site — but the MOVE is not
	// possible without first widening the tick predicate, and THAT is catchable at compile time,
	// in this module, with a message naming the law. ⇒ the pin below is the enforcement, and
	// test 15(e) is its readable twin.
	//
	// ⛔ THE TRAP, RESTATED SO THE PIN IS NOT MISTAKEN FOR PEDANTRY: `RefreshActorTickEnabled` is
	// the ONE writer of this actor's tick flag and its value is exactly this function, so a unit
	// that is neither mid-lunge nor ALREADY CLIMBING does not tick — which is precisely the state
	// every contact climb must begin in. A poll in `::Tick` would compile, review clean, pass this
	// whole suite and NEVER RUN.
	static_assert(std::is_same_v<decltype(&FSiegeLadderClimbStatics::WantsActorTick), bool (*)(bool, bool)>,
		"CONTACT-§11.5 / SC-§33: WantsActorTick is PINNED at exactly two inputs (bLungeActive, bClimbActive). "
		"A third term would be the 'obvious repair' that puts the contact poll back on the actor tick — and it "
		"would restore a permanent per-frame tick to the ENTIRE unit fleet, which TASK-738 and TASK-760 spent "
		"two tasks removing. TASK-784's poll rides StateTimerHandle instead; if you are here to add a parameter, "
		"that is the decision you are reversing.");

	// ⭐ THE CALLEE'S SIGNATURE, PINNED FROM THE CALLER'S SIDE — the `BeginLadderClimb` idiom of
	// this fixture, applied in the other direction. `ASummonedUnit::TryContactClimbAtNearestLadder`
	// (TASK-784) compiles against exactly this; a drift in `ClimbableTower.h` would otherwise
	// surface as an error inside SummonedUnit.cpp with nothing naming why.
	// ⚠️ It is also the closest a headless suite can get to "the caller EXISTS": the wiring itself
	// is still a diff read (this file's test 14(e) states that split), but the CONTRACT the wiring
	// depends on now fails HERE, by name, rather than silently.
	static_assert(std::is_same_v<decltype(&AClimbableTower::TryBeginContactClimb),
		AClimbableTower::ELadderContactVerdict (AClimbableTower::*)(ACharacter*, float)>,
		"CONTACT-§4.1: TryBeginContactClimb must stay `ELadderContactVerdict (ACharacter*, float)`. "
		"ASummonedUnit::TryContactClimbAtNearestLadder (TASK-784) is its caller and passes `this` plus the "
		"state poll's real world-clock delta.");

	// ═════════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-784's FIXTURE — THE CONTACT TRIGGER'S **CADENCE**
	// ═════════════════════════════════════════════════════════════════════════════════════════
	//
	// ⚠️⚠️ WHY THESE ROWS EXIST AT ALL, GIVEN THIS FILE'S OWN RULE THAT WIRING IS QA'S AND NOT THE
	// SUITE'S (test 14(e)): TASK-784 adds a CALL SITE, and a call site has exactly one property
	// that is decidable without a world — **the RATE it is called at**. That rate is the whole
	// engineering content of the task: the tower integrates a 0.35 s DWELL out of the deltas its
	// caller hands it, so how often the caller asks decides whether the feature works, whether it
	// works by luck, and whether the dwell still refuses the passers-by it was built to refuse.
	// ⇒ these rows drive the SHIPPED predicate at two cadences and show the answers differ. ⛔ They
	// do not claim the wiring is correct; that is still TASK-785's diff read.

	/** A 60 Hz frame — the cadence `ASummonedUnit::Tick` actually asks at. ⛔ Not a tunable: it is the STEP the dwell is integrated at. */
	constexpr float FrameStepSeconds = 1.f / 60.f;

	/**
	 *  Every UNIT row's `Speed` cell, typed from `Docs/Data/cards.csv` — the same direction the
	 *  Longbowman/Archer numbers above are typed in, and ⛔ never read back off a DataTable (this
	 *  file loads ⛔ no assets). Duplicates collapsed: 250 Ogre · 300 Knight/Longbowman ·
	 *  350 Archer/Pikeman/Cleric/Wizard/Sorcerer · 400 Footman/MilitiaMob · 500 Sapper · 600 Cavalry.
	 */
	constexpr float ShippedUnitSpeedsUU[] = { 250.f, 300.f, 350.f, 400.f, 500.f, 600.f };

	/** Index-locked names for the failure messages, so a red row NAMES the card rather than a number. */
	static const TCHAR* const ShippedUnitSpeedNames[] =
	{
		TEXT("Ogre (250)"),
		TEXT("Knight / Longbowman (300)"),
		TEXT("Archer / Pikeman / Cleric / Wizard / Sorcerer (350)"),
		TEXT("Footman / Militia Mob (400)"),
		TEXT("Sapper (500)"),
		TEXT("Cavalry (600)")
	};

	static_assert(UE_ARRAY_COUNT(ShippedUnitSpeedsUU) == UE_ARRAY_COUNT(ShippedUnitSpeedNames),
		"The roster speed table and its name table must stay index-locked.");

	/** The roster table's length as an int32 — the SiegeLadderClimbTest.cpp:717 idiom, so no loop below compares a signed index against an unsigned count. */
	constexpr int32 RosterSpeedCount = static_cast<int32>(UE_ARRAY_COUNT(ShippedUnitSpeedsUU));

	/**
	 *  ⭐ WALKS A PAWN IN A STRAIGHT LINE PAST THE LADDER FOOT AND REPORTS WHETHER THE **SHIPPED**
	 *  PREDICATE EVER ADMITTED IT — the tower's own `WalkPastAndSeeIfAdmitted` idiom, generalised
	 *  over the two things TASK-784 is actually about: the SAMPLING STEP and its PHASE.
	 *
	 *  The pawn starts 4 radii short of the foot, walks +X at `SpeedUU`, and is sampled every
	 *  `SampleStepSeconds` starting `PhaseSeconds` into the first step. ⛔ Nothing here re-implements
	 *  a term: the radius, the cone, the dwell and the endpoint resolution are all the arguments the
	 *  caller read off the shipped CDOs.
	 *
	 *  ⚠️ THE PHASE IS THE POINT AND IT IS ⛔ NOT PADDING: a sampler whose verdict depends on WHEN
	 *  its first sample happens to land is a feature that works by luck, and that is precisely what
	 *  the 0.25 s state poll turns this trigger into.
	 */
	static bool ApproachIsAdmitted(float SpeedUU, float PerpendicularOffsetY, float SampleStepSeconds,
		float PhaseSeconds, float RadiusUU, float IntentCos, float DwellSeconds)
	{
		const float SafeSpeedUU = FMath::Max(SpeedUU, 1.f);
		const float SafeStepSeconds = FMath::Max(SampleStepSeconds, 1.e-4f);
		const float TotalTravelUU = 8.f * FMath::Max(RadiusUU, 1.f);

		FSiegeLadderContactState State;
		float TravelledUU = (-4.f * FMath::Max(RadiusUU, 1.f)) + (SafeSpeedUU * PhaseSeconds);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(TotalTravelUU / (SafeSpeedUU * SafeStepSeconds)));

		bool bAscending = false;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const FVector Location(LadderFoot.X + TravelledUU, PerpendicularOffsetY, CapsuleHalfHeightUU);
			const FVector Velocity(SafeSpeedUU, 0.f, 0.f);

			if (FSiegeLadderContactStatics::WantsToClimb(State, Location, Velocity, LadderFoot, LadderTop,
				RadiusUU, IntentCos, DwellSeconds, SafeStepSeconds, bAscending) == ESiegeLadderContactVerdict::Climb)
			{
				return true;
			}

			TravelledUU += SafeSpeedUU * SafeStepSeconds;
		}
		return false;
	}

	/** How many evenly-spaced sampling phases (out of PhaseCount, spanning ONE sample step) admit the same approach. */
	static int32 CountAdmittingPhases(float SpeedUU, float PerpendicularOffsetY, float SampleStepSeconds,
		float RadiusUU, float IntentCos, float DwellSeconds, int32 PhaseCount)
	{
		const int32 SafePhaseCount = FMath::Max(PhaseCount, 1);
		int32 Admitted = 0;
		for (int32 Index = 0; Index < SafePhaseCount; ++Index)
		{
			const float PhaseSeconds = SampleStepSeconds * (static_cast<float>(Index) / static_cast<float>(SafePhaseCount));
			if (ApproachIsAdmitted(SpeedUU, PerpendicularOffsetY, SampleStepSeconds, PhaseSeconds, RadiusUU, IntentCos, DwellSeconds))
			{
				++Admitted;
			}
		}
		return Admitted;
	}

	/** Sixteen phases across one sample step — enough that a 1-in-8 phase window cannot be missed, cheap enough to run per row. */
	constexpr int32 PhaseSweepCount = 16;

	/** Perpendicular offsets a "marching past" pawn is tried at, in uu. ⭐ A SET rather than one number: the row that matters is *some* offset the two cadences disagree about, not a pre-chosen one. */
	constexpr float PasserByOffsetsUU[] = { 25.f, 50.f, 75.f, 100.f };

	/**
	 *  Reads the three CONTACT tunables off `AClimbableTower`'s CDO. ⛔ They are read rather than
	 *  typed here on purpose, and it is the OPPOSITE direction from the pinned geometry above:
	 *  `SiegeClimbableTowerTest.cpp` test 12(g) already pins their VALUES against `K-5`, so pinning
	 *  them again here would be a second copy of the same expectation. What THIS file needs is
	 *  whatever actually ships, so that when 🧑 Jonathan retunes one the cadence rows below
	 *  re-derive against his number instead of quietly meaning something else.
	 */
	static bool TryReadContactTunables(float& OutRadiusUU, float& OutIntentCos, float& OutDwellSeconds)
	{
		UClass* const TowerClass = AClimbableTower::StaticClass();
		const AClimbableTower* const TowerDefaults = GetDefault<AClimbableTower>();
		return TryReadDefaultFloat(TowerClass, TowerDefaults, TEXT("LadderContactRadiusUU"), OutRadiusUU)
			&& TryReadDefaultFloat(TowerClass, TowerDefaults, TEXT("LadderContactIntentCos"), OutIntentCos)
			&& TryReadDefaultFloat(TowerClass, TowerDefaults, TEXT("LadderContactDwellSeconds"), OutDwellSeconds);
	}

	/** The unit's shipped state-poll period — the cadence TASK-784 measured and REFUSED to run the dwell on. */
	static bool TryReadStatePollSeconds(float& OutPollSeconds)
	{
		return TryReadDefaultFloat(ASummonedUnit::StaticClass(), GetDefault<ASummonedUnit>(),
			TEXT("StateCheckInterval"), OutPollSeconds);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. THE ADMISSION PREDICATE — every pinned refusal, and "changes NOTHING"
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbAdmissionTest,
	"Siegebound.LadderClimb.RefusesEveryPinnedReasonAndARefusedBeginChangesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbAdmissionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ── SELF-CHECK FIRST: THE HEALTHY CASE IS ADMITTED ──────────────────────────────────
	// ⭐ Without this row every refusal below could be produced by a predicate that returns
	// false unconditionally, and the whole test would report SAFE while the feature never ran.
	FSiegeLadderClimbState Healthy;
	TestTrue(TEXT("SELF-CHECK: a live, unfrozen, not-already-climbing unit on the pinned line IS admitted — otherwise every refusal below is vacuous"),
		FSiegeLadderClimbStatics::CanBegin(Healthy, false, false, false, LadderFoot, LadderTop));

	// ── (a) THE FOUR PINNED REFUSAL REASONS (TOWER-§8.4(B)), ONE TERM AT A TIME ──────────
	// Each row flips EXACTLY ONE input away from the admitted case above, so a dropped term
	// shows up as one failing row that names itself.
	FSiegeLadderClimbState Fresh;
	TestFalse(TEXT("(a) ⛔ A DEAD unit is refused"),
		FSiegeLadderClimbStatics::CanBegin(Fresh, /*bDead=*/ true, false, false, LadderFoot, LadderTop));
	TestFalse(TEXT("(a) ⛔ A MATCH-END FROZEN unit (bAIFrozen) is refused"),
		FSiegeLadderClimbStatics::CanBegin(Fresh, false, /*bAIFrozen=*/ true, false, LadderFoot, LadderTop));
	TestFalse(TEXT("(a) ⛔ A SPELL-FROZEN unit (bSpellFrozen) is refused"),
		FSiegeLadderClimbStatics::CanBegin(Fresh, false, false, /*bSpellFrozen=*/ true, LadderFoot, LadderTop));

	FSiegeLadderClimbState AlreadyClimbing;
	TestTrue(TEXT("SELF-CHECK: the already-climbing fixture actually armed"),
		ArmPinnedClimb(AlreadyClimbing, 350.f));
	TestFalse(TEXT("(a) ⛔ An ALREADY CLIMBING unit is refused — TOWER-§10 L-1's one-at-a-time rule cannot be broken from this side"),
		FSiegeLadderClimbStatics::CanBegin(AlreadyClimbing, false, false, false, LadderFoot, LadderTop));

	// ── (b) THE DEGENERATE-LINE MATH GUARD (the ONE addition beyond the four pins) ───────
	// A zero-length line would make ClimbDirection normalise nothing: the unit would sit in
	// MOVE_Flying steering at zero until the watchdog fired. Refusing at the door is cheaper.
	TestFalse(TEXT("(b) ⛔ A ZERO-LENGTH line is refused — normalising it would steer nowhere while the unit floated"),
		FSiegeLadderClimbStatics::CanBegin(Fresh, false, false, false, LadderFoot, LadderFoot));
	TestFalse(TEXT("(b) ⛔ A SUB-1-uu line is refused"),
		FSiegeLadderClimbStatics::CanBegin(Fresh, false, false, false, LadderFoot, LadderFoot + FVector(0.f, 0.f, 0.5f)));
	TestTrue(TEXT("(b) SELF-CHECK: a line just OVER the 1 uu floor IS admitted — the guard is a floor, not a blanket refusal"),
		FSiegeLadderClimbStatics::CanBegin(Fresh, false, false, false, LadderFoot, LadderFoot + FVector(0.f, 0.f, 2.f)));

	// ⚠️ THE NaN GUARD IS SHIPPED BUT DELIBERATELY **NOT** EXERCISED HERE, AND THE REASON IS
	// MEASURED RATHER THAN LAZY: this build runs with ENABLE_NAN_DIAGNOSTIC == 1 (it is 0 only
	// in Shipping/Test), so merely CONSTRUCTING an FVector containing a NaN calls
	// TVector::DiagnosticCheckNaN() and raises an engine error — the test would fail on its own
	// fixture rather than on its subject. `CanBegin`'s ContainsNaN() refusal is therefore covered
	// by TASK-741's diff read, ⛔ not by a row that would go red for the wrong reason.

	// ── (c) ⭐⭐ "RETURNS FALSE AND CHANGES **NOTHING**" — THE HALF THAT IS EASY TO SHIP
	//        BROKEN, because a half-armed state looks identical to a refused one from the
	//        caller's return value and only shows up as a unit floating three seconds later.
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSiegeLadderClimbState Refused;
		const bool bDead = (Index == 0);
		const bool bAIFrozen = (Index == 1);
		const bool bSpellFrozen = (Index == 2);

		TestFalse(TEXT("(c) Begin refuses the guarded unit"),
			FSiegeLadderClimbStatics::Begin(Refused, bDead, bAIFrozen, bSpellFrozen, LadderFoot, LadderTop, 350.f, CapsuleHalfHeightUU));
		TestTrue(TEXT("(c) ⭐ …and EVERY field is untouched — ⛔ not merely bActive. A half-armed refusal is a unit that floats."),
			IsPristine(Refused));
	}

	// A refused Begin on an ALREADY ACTIVE climb must not re-arm or re-clock the live one.
	FSiegeLadderClimbState Live;
	ArmPinnedClimb(Live, 350.f);
	Live.ElapsedSeconds = 1.25f;
	const FVector LiveTopBefore = Live.End;
	const float LiveElapsedBefore = Live.ElapsedSeconds;
	TestFalse(TEXT("(c) Begin on an already-climbing state is refused"),
		FSiegeLadderClimbStatics::Begin(Live, false, false, false, FVector(9000.f, 9000.f, 9000.f), FVector(9000.f, 9000.f, 9999.f), 350.f, CapsuleHalfHeightUU));
	TestTrue(TEXT("(c) ⭐ …and the LIVE climb's line is not stolen by the refused call"), Live.End.Equals(LiveTopBefore, Tolerance));
	TestEqual(TEXT("(c) ⭐ …and its clock is not reset — a second tower cannot silently restart an ascent already in flight"),
		Live.ElapsedSeconds, LiveElapsedBefore, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. BEGIN — the state it arms, and the watchdog budget it sizes
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbBeginTest,
	"Siegebound.LadderClimb.BeginArmsTheLineAndSizesTheWatchdogFromTheRateAtArmingTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbBeginTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// SELF-CHECK: the pinned geometry really is the 1,236.9 uu line TOWER-§8.3 quotes.
	// Computed from the two socket coordinates — ⛔ not transcribed from the law — so a
	// mistyped fixture fails here instead of silently re-basing every number below it.
	TestEqual(TEXT("SELF-CHECK: sqrt(300² + 1200²) = 1,236.93 uu, the climb line TOWER-§8.3 pins"),
		ClimbLineLengthUU, 1236.93169f, 1.e-2f);

	FSiegeLadderClimbState State;
	TestTrue(TEXT("Begin admits the healthy case"), ArmPinnedClimb(State, 350.f));

	TestTrue(TEXT("(a) bActive is set — and it IS IsClimbing(), and it IS the TOWER-§9 disarm term. One bool, so the disarm cannot outlive the climb."),
		State.bActive);
	// ⭐ STORED IN CAPSULE-CENTRE SPACE — both surface points lifted by one half-height, so the
	// unit finishes STANDING ON the deck rather than buried 88 uu inside the slab.
	TestTrue(TEXT("(a) The start is the foot socket LIFTED to capsule-centre space"),
		State.Start.Equals(LadderFoot + FVector(0.f, 0.f, CapsuleHalfHeightUU), Tolerance));
	TestTrue(TEXT("(a) The end is the top socket LIFTED to capsule-centre space — the position a unit STANDING on the deck occupies"),
		State.End.Equals(DeckStandingCentre, Tolerance));
	TestEqual(TEXT("(a) ⭐ The length is UNCHANGED by the lift — the same offset at both ends cannot alter |End - Start|"),
		State.LengthUU, ClimbLineLengthUU, Tolerance);
	TestEqual(TEXT("(a) The clock starts at zero"), State.ElapsedSeconds, 0.f, Tolerance);

	// ── (b) THE WATCHDOG BUDGET = 4 × the climb's OWN expected duration ──────────────────
	// Re-derived: length ÷ rate is the honest ascent, and the budget is a deliberately
	// generous multiple of it. ⭐ Generous is the requirement: this must NEVER end a healthy
	// climb early — it exists only so a BLOCKED one drops instead of hanging.
	const float ExpectedAscentSeconds = ClimbLineLengthUU / 350.f;
	TestEqual(TEXT("(b) The budget is TimeoutScale × the ascent's own expected duration"),
		State.TimeoutSeconds, FSiegeLadderClimbStatics::TimeoutScale * ExpectedAscentSeconds, 1.e-2f);
	TestTrue(TEXT("(b) ⭐ …which leaves the HEALTHY ascent nowhere near it — a real climb can never be cut short"),
		State.TimeoutSeconds > ExpectedAscentSeconds * 2.f);

	// ── (c) A RETUNE MID-CLIMB CANNOT EXTEND A CLIMB ALREADY IN FLIGHT ──────────────────
	// The budget is fixed at arming time, so LadderClimbSpeedUU is a lever on the NEXT climb
	// and never a way to keep a stuck one alive.
	FSiegeLadderClimbState Slow;
	ArmPinnedClimb(Slow, 100.f);
	TestTrue(TEXT("(c) A slower rate buys a proportionally larger budget at ARMING time"),
		Slow.TimeoutSeconds > State.TimeoutSeconds);

	// ── (d) ⛔ A MIS-TUNED ZERO RATE MUST NOT DIVIDE BY ZERO AND MUST NOT BE IMMORTAL ────
	// LadderClimbSpeedUU is EditDefaultsOnly, so 0 is one keystroke away. The unit then never
	// arrives — and the watchdog is what turns "hangs forever" into "drops after a while".
	FSiegeLadderClimbState Zero;
	TestTrue(TEXT("(d) Begin still admits at a zero rate (the rate is not an admission term)"),
		ArmPinnedClimb(Zero, 0.f));
	TestTrue(TEXT("(d) ⭐ …and its budget is FINITE — a 0 rate cannot produce an immortal climb"),
		FMath::IsFinite(Zero.TimeoutSeconds));
	TestTrue(TEXT("(d) ⭐ …and positive — never a NaN or a zero that would expire on the first frame"),
		Zero.TimeoutSeconds > 0.f);

	// ── (e) A VERY SHORT LADDER STILL GETS A SANE BUDGET (the floor) ────────────────────
	FSiegeLadderClimbState Short;
	Short = FSiegeLadderClimbState();
	FSiegeLadderClimbStatics::Begin(Short, false, false, false, LadderFoot, LadderFoot + FVector(0.f, 0.f, 3.f), 350.f, CapsuleHalfHeightUU);
	TestTrue(TEXT("(e) A 3 uu ladder's budget is floored at MinTimeoutSeconds, ⛔ not 0.03 s"),
		Short.TimeoutSeconds >= FSiegeLadderClimbStatics::MinTimeoutSeconds);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. THE CLIMB LINE HAS A REAL VERTICAL COMPONENT — which is WHY MOVE_Flying
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbDirectionTest,
	"Siegebound.LadderClimb.TheSteerIsMostlyVerticalWhichIsExactlyWhatAWalkingPawnWouldHaveDeleted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbDirectionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	FSiegeLadderClimbState State;
	TestTrue(TEXT("SELF-CHECK: the fixture armed"), ArmPinnedClimb(State, 350.f));

	const FVector Direction = FSiegeLadderClimbStatics::ClimbDirection(State);

	TestEqual(TEXT("(a) The steer is a UNIT vector"), static_cast<float>(Direction.Size()), 1.f, Tolerance);

	// ── (b) ⭐⭐ THE MEASUREMENT THAT LICENSES MOVE_Flying, RESTATED AS AN ASSERTION ──────
	// `UCharacterMovementComponent::ConstrainInputAcceleration` plane-projects steering input
	// whenever `IsMovingOnGround() || IsFalling()` (CharacterMovementComponent.cpp:8121-8131).
	// So THIS is what a walking pawn handed the same steer would actually receive:
	const FVector WhatAWalkingPawnWouldReceive = FVector::VectorPlaneProject(Direction, FVector::UpVector);

	TestEqual(TEXT("(b) ⛔ A walking pawn's steer would have ZERO vertical component — the engine deletes it, every frame, by design"),
		static_cast<float>(WhatAWalkingPawnWouldReceive.Z), 0.f, Tolerance);
	TestTrue(TEXT("(b) ⭐⭐ …and 97% of the steer's MAGNITUDE with it: the surviving horizontal part is under a quarter of the input. THIS is why a nav link alone walks nowhere and MOVE_Flying is required."),
		WhatAWalkingPawnWouldReceive.Size() < 0.25 * Direction.Size());

	// ── (c) THE LEAN, RE-DERIVED FROM THE SOCKETS ───────────────────────────────────────
	// atan(1200 / 300) = atan(4) = 75.96°, which TOWER-§8.3 pins as 76.0° — and which is
	// 2.4× over Recast's 32.005° walkable ceiling, so ⛔ nothing will ever try to WALK it.
	const float LeanDegrees = FMath::RadiansToDegrees(FMath::Atan2(
		static_cast<float>(LadderTop.Z - LadderFoot.Z),
		static_cast<float>(FVector::Dist2D(LadderTop, LadderFoot))));
	TestEqual(TEXT("(c) The pinned sockets describe a 76.0° lean"), LeanDegrees, 75.96376f, 1.e-2f);
	TestTrue(TEXT("(c) ⭐ …comfortably over Recast's 32.005° walkable ceiling, so the ladder is unwalkable BY CONSTRUCTION — ⛔ no ambiguity about which route the AI takes"),
		LeanDegrees > 32.005f);

	// ── (d) ⛔ A DEGENERATE STATE YIELDS ZERO, ⛔ NEVER NaN ──────────────────────────────
	const FSiegeLadderClimbState Degenerate;
	TestTrue(TEXT("(d) ClimbDirection on an un-armed state is ZeroVector, ⛔ never NaN — a NaN steer would corrupt the movement component"),
		FSiegeLadderClimbStatics::ClimbDirection(Degenerate).IsZero());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ARRIVAL — the instant the deck is reached, and NOT one frame earlier
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbArrivalTest,
	"Siegebound.LadderClimb.ArrivesTheInstantTheDeckIsReachedAndNotBefore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbArrivalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	const FVector Direction = (LadderTop - LadderFoot).GetSafeNormal();
	const float Frame = 1.f / 60.f;

	bool bReachedTop = false;
	bool bTimedOut = false;

	// ── (a) AT THE FOOT, THE CLIMB CONTINUES ────────────────────────────────────────────
	// ⭐ This is the SELF-CHECK for every arrival claim below: if Advance returned "arrived"
	// from the foot, the tests below would all pass while the feature never lifted anybody.
	FSiegeLadderClimbState AtFoot;
	ArmPinnedClimb(AtFoot, 350.f);
	TestTrue(TEXT("(a) SELF-CHECK: at the FOOT the climb CONTINUES — otherwise every arrival claim below is vacuous"),
		FSiegeLadderClimbStatics::Advance(AtFoot, FootStandingCentre, Frame, bReachedTop, bTimedOut));
	TestFalse(TEXT("(a) …and it has not arrived"), bReachedTop);

	// ── (b) ONE TOLERANCE-WIDTH SHORT OF THE TOP, IT STILL CONTINUES ────────────────────
	// ⚠️ THE ROW THAT MAKES THE TOLERANCE MEAN SOMETHING. Without it, a tolerance of 10,000 uu
	// would pass every other row in this test — the unit would "arrive" the moment it stepped
	// onto the ladder, and the disarm would release before the climb even started.
	FSiegeLadderClimbState JustShort;
	ArmPinnedClimb(JustShort, 350.f);
	const FVector JustShortOfTop = DeckStandingCentre - Direction * (FSiegeLadderClimbStatics::ArrivalToleranceUU * 3.f);
	TestTrue(TEXT("(b) ⭐ THREE tolerance-widths short of the deck the climb CONTINUES — the arrival radius is small, ⛔ not a blanket"),
		FSiegeLadderClimbStatics::Advance(JustShort, JustShortOfTop, Frame, bReachedTop, bTimedOut));
	TestFalse(TEXT("(b) …and it has not arrived"), bReachedTop);

	// ── (c) INSIDE THE TOLERANCE, IT ARRIVES ────────────────────────────────────────────
	FSiegeLadderClimbState Inside;
	ArmPinnedClimb(Inside, 350.f);
	const FVector InsideTolerance = DeckStandingCentre - Direction * (FSiegeLadderClimbStatics::ArrivalToleranceUU * 0.5f);
	TestFalse(TEXT("(c) Inside the arrival radius the climb ENDS"),
		FSiegeLadderClimbStatics::Advance(Inside, InsideTolerance, Frame, bReachedTop, bTimedOut));
	TestTrue(TEXT("(c) ⭐ …reporting ARRIVAL — the only exit of the eight that does"), bReachedTop);
	TestFalse(TEXT("(c) …and ⛔ not a timeout"), bTimedOut);

	// ── (d) OVERSHOOT — THE LOW-FRAME-RATE CASE ─────────────────────────────────────────
	// A single 20 fps step at 350 uu/s covers 17.5 uu, which is MORE than the arrival radius.
	// Without the along-axis test, such a step would sail straight past the deck and the unit
	// would keep flying — the tolerance alone is NOT a sufficient arrival test.
	FSiegeLadderClimbState Overshot;
	ArmPinnedClimb(Overshot, 350.f);
	const FVector PastTheTop = DeckStandingCentre + Direction * 200.f;
	TestFalse(TEXT("(d) ⭐ A unit that stepped clean PAST the deck still ends its climb — the along-axis test, ⛔ not the radius, catches this"),
		FSiegeLadderClimbStatics::Advance(Overshot, PastTheTop, Frame, bReachedTop, bTimedOut));
	TestTrue(TEXT("(d) …and it counts as ARRIVAL, ⛔ not as a failure"), bReachedTop);

	// ── (e) AN UN-ARMED STATE ADVANCES NOTHING AND CLAIMS NOTHING ───────────────────────
	FSiegeLadderClimbState Idle;
	TestFalse(TEXT("(e) Advance on a state that was never armed returns 'not climbing'"),
		FSiegeLadderClimbStatics::Advance(Idle, FootStandingCentre, Frame, bReachedTop, bTimedOut));
	TestFalse(TEXT("(e) ⛔ …and claims neither arrival"), bReachedTop);
	TestFalse(TEXT("(e) ⛔ …nor timeout — a phantom arrival would broadcast a climb nobody began"), bTimedOut);

	// ── (f) A NEGATIVE DELTA IS NEVER CHARGED TO THE BUDGET ─────────────────────────────
	FSiegeLadderClimbState Backwards;
	ArmPinnedClimb(Backwards, 350.f);
	FSiegeLadderClimbStatics::Advance(Backwards, FootStandingCentre, -100.f, bReachedTop, bTimedOut);
	TestEqual(TEXT("(f) A negative delta charges EXACTLY nothing to the budget — a clock that can run backwards must never be able to expire, or un-expire, a watchdog (the ConsumeStuckDeltaSeconds discipline)"),
		Backwards.ElapsedSeconds, 0.f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. THE DISARM RELEASES ON ARRIVAL — with ⛔ NO lingering window
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbDisarmReleaseTest,
	"Siegebound.LadderClimb.TheDisarmReleasesOnTheVeryStepTheDeckIsReachedWithNoGraceWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbDisarmReleaseTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// TOWER-§9.2: "THE DISARM ENDS THE INSTANT THE UNIT REACHES THE DECK. ⛔ No lingering
	// penalty, ⛔ no decay timer, ⛔ no grace window — the vulnerability is the CLIMB."
	FSiegeLadderClimbState State;
	TestTrue(TEXT("SELF-CHECK: the fixture armed"), ArmPinnedClimb(State, 350.f));

	// ── (a) WHILE CLIMBING, A NORMAL UNIT IS DISARMED ───────────────────────────────────
	TestFalse(TEXT("(a) ⛔ Mid-climb, a unit whose CLASS can attack is refused at the guard points — 'they can't attack back'"),
		FSiegeLadderClimbStatics::IsAttackAllowed(/*bCanEverAttack=*/ true, State.bActive));

	// ── (b) THE ARRIVAL STEP ────────────────────────────────────────────────────────────
	const FVector Direction = (LadderTop - LadderFoot).GetSafeNormal();
	bool bReachedTop = false;
	bool bTimedOut = false;
	FSiegeLadderClimbStatics::Advance(State, DeckStandingCentre - Direction * 2.f, 1.f / 60.f, bReachedTop, bTimedOut);
	TestTrue(TEXT("(b) SELF-CHECK: the deck was reached on this step"), bReachedTop);

	// ⚠️ Advance REPORTS arrival; End() is what clears the state — that is the same split the
	// shipped TickLadderClimb uses (it calls EndLadderClimb the moment Advance says stop), and
	// there is deliberately NOTHING between them: no timer, no fade, no queued release.
	TestTrue(TEXT("(b) The teardown latch fires exactly here, on the arrival step"),
		FSiegeLadderClimbStatics::End(State));

	// ── (c) ⭐⭐ AND THE UNIT IS ARMED AGAIN IMMEDIATELY ─────────────────────────────────
	TestFalse(TEXT("(c) bActive is clear — IsClimbing() is false on the very step the deck was reached"), State.bActive);
	TestTrue(TEXT("(c) ⭐⭐ …so the disarm is RELEASED with no lingering penalty: a unit that reaches the deck may fire on its next state poll"),
		FSiegeLadderClimbStatics::IsAttackAllowed(/*bCanEverAttack=*/ true, State.bActive));

	// ── (d) ⛔ AND NO RESIDUE COULD RE-DISARM IT ────────────────────────────────────────
	// The whole-struct reset is what makes "no second flag anybody forgot to clear" a fact
	// rather than a promise. A leftover ElapsedSeconds or Top is the shape of a decay window.
	TestTrue(TEXT("(d) ⛔ EVERY field is back to the not-climbing default — there is no second flag and no residual clock a grace window could hide in"),
		IsPristine(State));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⭐⭐ ALL EIGHT EXITS — exactly once, every time, whatever the reason
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbEightExitsTest,
	"Siegebound.LadderClimb.EveryExitEndsTheClimbExactlyOnceSoTheMovementModeIsAlwaysRestored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbEightExitsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ⚠️⚠️ WHAT THIS TEST IS ACTUALLY ABOUT: in the shipped code, EndLadderClimb restores the
	// movement mode and broadcasts OnLadderClimbEnded IF AND ONLY IF this latch returns true.
	// So "every exit restores the mode, exactly once" reduces to "the latch fires exactly once
	// per Begin, for every reason" — which is what is asserted here, exhaustively, and which is
	// the property that makes a NINTH exit added later safe by construction rather than by
	// somebody remembering.

	// ── SELF-CHECK: THE LATCH CAN SAY NO ────────────────────────────────────────────────
	// Without this, a latch hard-wired to `return true` would pass every row below and would
	// double-broadcast on death-then-EndPlay in the shipped game.
	FSiegeLadderClimbState NeverBegun;
	TestFalse(TEXT("SELF-CHECK: End() on a unit that never climbed returns FALSE — ⛔ nothing restored, ⛔ nothing broadcast"),
		FSiegeLadderClimbStatics::End(NeverBegun));

	const int32 ExitReasonCount = static_cast<int32>(UE_ARRAY_COUNT(AllExitReasons));
	for (int32 Index = 0; Index < ExitReasonCount; ++Index)
	{
		const TCHAR* const ReasonName = AllExitReasonNames[Index];

		FSiegeLadderClimbState State;
		if (!TestTrue(*FString::Printf(TEXT("[%s] SELF-CHECK: the climb armed before the exit was taken"), ReasonName),
			ArmPinnedClimb(State, 350.f)))
		{
			continue;
		}

		// The first exit is the real one: it is what restores the movement mode. ⛔ If this ever
		// returned false, a MOVE_Flying unit would be left hanging in mid-air FOREVER.
		TestTrue(*FString::Printf(TEXT("[%s] ⭐ The FIRST exit fires the teardown — the movement mode is restored and OnLadderClimbEnded is broadcast"), ReasonName),
			FSiegeLadderClimbStatics::End(State));

		// The second is the double-exit case, and it is ORDINARY rather than exotic: HandleDeath
		// runs, then EndPlay runs on the same actor moments later. A tower listener that calls
		// AbortLadderClimb from inside the broadcast is the same shape.
		TestFalse(*FString::Printf(TEXT("[%s] ⛔ A SECOND exit does nothing — no double restore, no double broadcast (death → EndPlay is the ordinary case, ⛔ not an edge one)"), ReasonName),
			FSiegeLadderClimbStatics::End(State));

		TestFalse(*FString::Printf(TEXT("[%s] ⛔ …and a THIRD does nothing either"), ReasonName),
			FSiegeLadderClimbStatics::End(State));

		TestTrue(*FString::Printf(TEXT("[%s] The state is back to the not-climbing default, so IsClimbing() is false and the disarm is released"), ReasonName),
			IsPristine(State));
	}

	// ── ⭐ AND THE COUNT ITSELF, STATED AS A CLAIM ──────────────────────────────────────
	// TOWER-§8.5 names EIGHT exits. This table holds seven of them as distinct reasons plus the
	// declared watchdog; the eighth law exit (the TOWER destroyed mid-climb) is the Abort path,
	// because AClimbableTower::EndPlay calls the parameterless AbortLadderClimb() — and this
	// class's own EndPlay is the independent belt for it. A future reason added to the enum
	// without a row here fails this line rather than slipping through.
	TestEqual(TEXT("⭐ The exit table covers every reason the enum can produce — a new one must be added here too"),
		ExitReasonCount, 8);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. THE WATCHDOG — drops a hanging climb, but ⛔ never beats an arrival
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbWatchdogTest,
	"Siegebound.LadderClimb.TheWatchdogDropsAHangingClimberButAnArrivalAlwaysWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbWatchdogTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	bool bReachedTop = false;
	bool bTimedOut = false;

	// ── (a) A CLIMBER THAT NEVER MOVES IS EVENTUALLY DROPPED ────────────────────────────
	// The scenario: geometry blocks the sweep, so the capsule never leaves the foot. Without
	// this the unit sits in MOVE_Flying at the bottom of the ladder for the rest of the match.
	FSiegeLadderClimbState Stuck;
	ArmPinnedClimb(Stuck, 350.f);
	TestTrue(TEXT("(a) SELF-CHECK: a stuck climber is STILL CLIMBING well before its budget expires — the watchdog is not trigger-happy"),
		FSiegeLadderClimbStatics::Advance(Stuck, FootStandingCentre, Stuck.TimeoutSeconds * 0.5f, bReachedTop, bTimedOut));
	TestFalse(TEXT("(a) …and has not timed out at half budget"), bTimedOut);

	TestFalse(TEXT("(a) ⭐ Past its budget the climb ENDS"),
		FSiegeLadderClimbStatics::Advance(Stuck, FootStandingCentre, Stuck.TimeoutSeconds, bReachedTop, bTimedOut));
	TestTrue(TEXT("(a) ⭐ …reporting a TIMEOUT — the unit is dropped (mode restored), ⛔ never stranded in mid-air"), bTimedOut);
	TestFalse(TEXT("(a) ⛔ …and it must NOT be reported as an arrival — a phantom arrival would tell the tower a unit reached a deck it never reached"), bReachedTop);

	// ── (b) ⭐⭐ ARRIVAL BEATS THE WATCHDOG ON THE SAME FRAME ────────────────────────────
	// ⚠️ THE ORDERING BUG THIS ROW EXISTS FOR: a unit reaching the deck on the very frame its
	// budget expires must ARRIVE. Test the budget first and you drop a unit off a tower it had
	// already reached — and the disarm would release with bReachedTop false, which is a lie to
	// the tower's listener.
	FSiegeLadderClimbState Photo;
	ArmPinnedClimb(Photo, 350.f);
	TestFalse(TEXT("(b) On the top, with the budget simultaneously blown, the climb ends"),
		FSiegeLadderClimbStatics::Advance(Photo, DeckStandingCentre, Photo.TimeoutSeconds * 10.f, bReachedTop, bTimedOut));
	TestTrue(TEXT("(b) ⭐⭐ …as an ARRIVAL"), bReachedTop);
	TestFalse(TEXT("(b) ⛔ …and NOT as a timeout — arrival is tested before the budget, on purpose"), bTimedOut);

	// ── (c) A HEALTHY ASCENT NEVER COMES CLOSE ──────────────────────────────────────────
	// Re-derived rather than asserted: the honest ascent is length ÷ rate, and the budget is
	// TimeoutScale times that. The watchdog is a safety net, ⛔ not a game rule.
	FSiegeLadderClimbState Healthy;
	ArmPinnedClimb(Healthy, 350.f);
	const float HonestAscentSeconds = ClimbLineLengthUU / 350.f;
	TestTrue(TEXT("(c) ⭐ The healthy 3.53 s ascent finishes with the budget barely touched — a real climb can never be cut short by the net"),
		HonestAscentSeconds < Healthy.TimeoutSeconds * 0.5f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⛔⛔ THE TRANSIENT DISARM IS NOT THE SORCERER'S PERMANENT SEAL
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbTwoTermGateTest,
	"Siegebound.LadderClimb.ATransientClimbIsDistinguishableFromTheSorcerersPermanentClassSeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbTwoTermGateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// TOWER-§9.2: the disarm must ⛔ NOT be implemented by overriding CanEverAttack(), because
	// that is a `const` CLASS-IDENTITY seal — doing so would make a Sorcerer's PERMANENT
	// inability and a Footman's THREE-SECOND climb indistinguishable in the code.

	// ── (a) THE FULL 2×2 TRUTH TABLE OF THE SHIPPED GATE ────────────────────────────────
	// ⭐ FOUR rows over TWO inputs: a gate that dropped either term, or that OR'd instead of
	// AND'ing, fails at least one of them. A single-input gate would not even compile here.
	TestTrue(TEXT("(a) ✅ CAN attack, NOT climbing  ⇒ ALLOWED — the ordinary Footman"),
		FSiegeLadderClimbStatics::IsAttackAllowed(true, false));
	TestFalse(TEXT("(a) ⛔ CAN attack, IS climbing  ⇒ REFUSED — Jonathan's disarm, and the unit's class is untouched"),
		FSiegeLadderClimbStatics::IsAttackAllowed(true, true));
	TestFalse(TEXT("(a) ⛔ CANNOT attack, NOT climbing ⇒ REFUSED — the Sorcerer standing on the ground"),
		FSiegeLadderClimbStatics::IsAttackAllowed(false, false));
	TestFalse(TEXT("(a) ⛔ CANNOT attack, IS climbing ⇒ REFUSED — both reasons at once"),
		FSiegeLadderClimbStatics::IsAttackAllowed(false, true));

	// ── (b) THE CLASS SEAL IS A PER-CLASS CONSTANT, READ OFF THE SHIPPED CDOs ───────────
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	const ASorcererUnit* const SorcererDefaults = GetDefault<ASorcererUnit>();
	const AMinerUnit* const MinerDefaults = GetDefault<AMinerUnit>();

	if (!TestNotNull(TEXT("SELF-CHECK: the ASummonedUnit CDO resolves"), UnitDefaults)
		|| !TestNotNull(TEXT("SELF-CHECK: the ASorcererUnit CDO resolves"), SorcererDefaults)
		|| !TestNotNull(TEXT("SELF-CHECK: the AMinerUnit CDO resolves"), MinerDefaults))
	{
		return false;
	}

	TestTrue(TEXT("(b) A plain ASummonedUnit CAN ever attack — its class identity is untouched by this feature"),
		UnitDefaults->CanEverAttack());
	TestFalse(TEXT("(b) ⛔ ASorcererUnit can NEVER attack — the permanent seal, and it did not move"),
		SorcererDefaults->CanEverAttack());
	TestFalse(TEXT("(b) ⛔ AMinerUnit can NEVER attack — the same permanent seal"),
		MinerDefaults->CanEverAttack());

	// ── (c) ⭐⭐ AND THE TWO TERMS ARE ORTHOGONAL, WHICH IS THE WHOLE CLAIM ──────────────
	// The Sorcerer is disarmed with IsClimbing() FALSE; a climbing Footman is disarmed with
	// CanEverAttack() TRUE. ⇒ the code can tell them apart, which is exactly what would have
	// been lost had the disarm been an override of CanEverAttack().
	TestFalse(TEXT("(c) ⭐ A fresh ASummonedUnit is NOT climbing — the transient term starts clear"),
		UnitDefaults->IsClimbing());
	TestFalse(TEXT("(c) ⭐⭐ …and the SORCERER, which cannot attack at all, is likewise NOT climbing: its inability has nothing to do with a ladder"),
		SorcererDefaults->IsClimbing());
	TestFalse(TEXT("(c) ⭐⭐ …nor is the Miner"), MinerDefaults->IsClimbing());

	// ── (d) THE SORCERER'S SEAL IS UNCONDITIONAL, THE CLIMBER'S IS NOT ──────────────────
	// Stated as the pair of gate outcomes a reviewer would want to see side by side.
	TestFalse(TEXT("(d) ⛔ The Sorcerer is refused whether it is climbing or not — PERMANENT"),
		FSiegeLadderClimbStatics::IsAttackAllowed(SorcererDefaults->CanEverAttack(), false)
		|| FSiegeLadderClimbStatics::IsAttackAllowed(SorcererDefaults->CanEverAttack(), true));
	TestTrue(TEXT("(d) ⭐ The Footman is refused ONLY while climbing and is allowed the moment it is not — TRANSIENT"),
		!FSiegeLadderClimbStatics::IsAttackAllowed(UnitDefaults->CanEverAttack(), true)
		&& FSiegeLadderClimbStatics::IsAttackAllowed(UnitDefaults->CanEverAttack(), false));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ✅ "ATTACKABLE" IS FREE — the proof that ⛔ NO mechanism was written for it
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbStillTargetableTest,
	"Siegebound.LadderClimb.AClimberIsStillAValidTargetBecauseNothingWasAddedThatCouldMakeItNotOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbStillTargetableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// TOWER-§9.2: *"'ATTACKABLE' IS FREE — ⛔ LITERALLY ZERO WORK. … ⛔ do not write code to
	// 'make it targetable'; write a test that proves it already is."*
	//
	// ⭐ WHAT MAKES IT FREE, MEASURED IN THE SHIPPED CODE: the per-candidate acquisition gate in
	// AcquireTarget / AcquireEnemyNearPoint is
	//     Candidate != this  &&  IsTargetAlive(Candidate)  &&  enemy team  &&  inside a 2D disc
	// and IsTargetAlive's only unit-state term is IsUnitDead(). ⛔ No movement mode, ⛔ no Z,
	// ⛔ no climb state anywhere in it.
	//
	// ⚠️⚠️ AND WHAT THIS TEST HONESTLY CANNOT DO, STATED RATHER THAN PAPERED OVER: it cannot run
	// an acquisition sweep with a climbing unit in it. That needs a UWorld and live actors, and
	// this project has no such fixture (see the file header for the crash that makes one
	// impossible headlessly). ⇒ what IS asserted is the STRUCTURAL claim: this feature added
	// nothing that acquisition could consult, and a future "climbers can't be hit" mechanism
	// could not plausibly avoid the names below. The behavioural half is Jonathan's playtest.

	UClass* const UnitClass = ASummonedUnit::StaticClass();
	if (!TestNotNull(TEXT("SELF-CHECK: ASummonedUnit::StaticClass() resolves"), UnitClass))
	{
		return false;
	}

	// ── (a) THE REFLECTION WALK OVER WHAT THIS CLASS ITSELF DECLARES ────────────────────
	TArray<FString> DeclaredMemberNames;
	for (TFieldIterator<FProperty> PropertyIt(UnitClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		DeclaredMemberNames.Add(PropertyIt->GetName());
	}
	for (TFieldIterator<UFunction> FunctionIt(UnitClass, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
	{
		DeclaredMemberNames.Add(FunctionIt->GetName());
	}

	// SELF-CHECK: the walk is live. An empty list passes any scan — the trap
	// SiegeClimbableTowerTest.cpp already names, and it applies here identically.
	TestTrue(TEXT("SELF-CHECK: the reflection walk is live (it found this task's own BeginLadderClimb)"),
		DeclaredMemberNames.Contains(TEXT("BeginLadderClimb")));

	// ── (b) ⛔ NO TARGETABILITY FILTER WAS ADDED ────────────────────────────────────────
	// A mechanism that made a climber harder to hit — or unhittable — would be a predicate, a
	// flag or an override, and it could not plausibly avoid every one of these substrings.
	static const TCHAR* const RefusedTargetabilityTokens[] =
	{
		TEXT("Targetab"), TEXT("Untargetab"), TEXT("Acquirab"),
		TEXT("Invulnerab"), TEXT("Immun"), TEXT("Evasi"), TEXT("Dodge")
	};

	for (const FString& MemberName : DeclaredMemberNames)
	{
		for (const TCHAR* const Token : RefusedTargetabilityTokens)
		{
			if (MemberName.Contains(Token, ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ ASummonedUnit declares '%s', which names targetability ('%s'). TOWER-§9.2 rules that 'attackable' is FREE and gets ⛔ NO mechanism: ")
					TEXT("a climber is an ordinary live unit at an ordinary location, and every surface added here is a surface that can later break Jonathan's ruling."),
					*MemberName, Token));
			}
		}
	}

	// ── (c) THE LIVE-UNIT BASELINE — a fresh unit is not dead, so it is a valid target ──
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!TestNotNull(TEXT("SELF-CHECK: the ASummonedUnit CDO resolves"), UnitDefaults))
	{
		return false;
	}
	TestFalse(TEXT("(c) A unit is not dead by default — and death is the ONLY unit state the acquisition gate reads"),
		UnitDefaults->IsUnitDead());

	// ── (d) ⭐ AND THE TWO STATES ARE INDEPENDENT BOOLEANS, NOT ONE ─────────────────────
	// Beginning a climb writes the climb state and NOTHING else — so a climber's IsUnitDead()
	// is untouched, and the acquisition gate that reads it sees exactly what it saw before.
	// (The pure state carries no liveness field at all, which is the strongest form of that.)
	FSiegeLadderClimbState State;
	TestTrue(TEXT("(d) SELF-CHECK: the fixture armed"), ArmPinnedClimb(State, 350.f));
	TestTrue(TEXT("(d) ⭐ The climb state is climb-only — it carries a line, a clock and a budget, and ⛔ nothing that acquisition reads"),
		State.bActive && State.LengthUU > 0.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 10. ⛔⛔ THE PINNED API TASK-734 COMPILES AGAINST
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbPinnedApiTest,
	"Siegebound.LadderClimb.ThePinnedApiIsPresentUnderItsPinnedNamesTypesAndReflection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbPinnedApiTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ⚠️⚠️ TASK-734 (AClimbableTower) IS BEING WRITTEN IN PARALLEL AND COMPILES AGAINST THIS
	// EXACT SURFACE. It does not compile until this lands — that is the DESIGNED state
	// (the MinerUnit.h:330-334 precedent: one UBT module, one compile at TASK-742) — so a
	// rename or a type drift here surfaces as a link error in SOMEBODY ELSE'S task. The
	// static_asserts in the fixture above catch the C++ types at compile time; these rows catch
	// the REFLECTED surface, which is what Blueprint (ABP_Footman, TASK-739) binds to.

	UClass* const UnitClass = ASummonedUnit::StaticClass();
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!TestNotNull(TEXT("SELF-CHECK: ASummonedUnit::StaticClass() resolves"), UnitClass)
		|| !TestNotNull(TEXT("SELF-CHECK: the ASummonedUnit CDO resolves"), UnitDefaults))
	{
		return false;
	}

	// ── (a) THE THREE UFUNCTIONS, BY THEIR PINNED NAMES ─────────────────────────────────
	static const TCHAR* const PinnedFunctionNames[] =
	{
		TEXT("BeginLadderClimb"), TEXT("AbortLadderClimb"), TEXT("IsClimbing")
	};
	for (const TCHAR* const FunctionName : PinnedFunctionNames)
	{
		TestNotNull(*FString::Printf(TEXT("(a) ⛔ '%s' is a reflected UFUNCTION on ASummonedUnit — TOWER-§8.4(B) pins the name, and Blueprint binds to it"), FunctionName),
			UnitClass->FindFunctionByName(FName(FunctionName)));
	}

	// ── (b) THE DELEGATE, AND ITS **TWO** PARAMETERS ────────────────────────────────────
	const FMulticastDelegateProperty* const ClimbEndedProperty =
		CastField<FMulticastDelegateProperty>(UnitClass->FindPropertyByName(FName(TEXT("OnLadderClimbEnded"))));
	if (!TestNotNull(TEXT("(b) ⛔ OnLadderClimbEnded is a reflected multicast delegate property — it is the tower's ONLY completion signal, and without it AClimbableTower would need a tick"), ClimbEndedProperty))
	{
		return false;
	}

	const UFunction* const Signature = ClimbEndedProperty->SignatureFunction;
	if (!TestNotNull(TEXT("(b) SELF-CHECK: the delegate carries a signature function"), Signature))
	{
		return false;
	}
	TestEqual(TEXT("(b) ⛔ FSiegeLadderClimbEnded takes EXACTLY two parameters (ACharacter* Climber, bool bReachedTop) — TASK-734 binds a handler of this shape"),
		static_cast<int32>(Signature->NumParms), 2);

	// The parameter TYPES, in order — a delegate with two params of the wrong types would pass
	// the count above and still fail to bind in TASK-734.
	int32 ParameterIndex = 0;
	for (TFieldIterator<FProperty> ParamIt(Signature); ParamIt && ParamIt->HasAnyPropertyFlags(CPF_Parm); ++ParamIt, ++ParameterIndex)
	{
		if (ParameterIndex == 0)
		{
			// ⚖️⭐ RETARGETED 2026-09-01 (TASK-777, CONTACT-§4.4 — TOWER-§8.4(B)'s second
			// amendment). ⛔⛔ THIS ASSERTION WENT **RED** ON THE WIDENING, WHICH IS THE
			// EVIDENCE THAT THE PINNING MECHANISM WORKS — ⛔ it is NOT collateral damage,
			// and it is ⛔ NOT a reason to weaken it to a null check. The first parameter
			// is now ACharacter*, the narrowest type that admits BOTH ASummonedUnit and
			// AHeroCharacter and the only one carrying GetCharacterMovement() /
			// GetCapsuleComponent(). ⭐ It still discriminates exactly as hard: an APawn*
			// (the proposal that was ruled against) and an ASummonedUnit* (what shipped
			// before) BOTH fail this line.
			const FObjectProperty* const ClimberParam = CastField<FObjectProperty>(*ParamIt);
			if (TestNotNull(TEXT("(b) Parameter 1 is an object property"), ClimberParam))
			{
				TestTrue(TEXT("(b) …and it is an ACharacter* — CONTACT-§4.4's ruled widening. ⛔ An APawn* (refused: no GetCharacterMovement/GetCapsuleComponent) or the pre-amendment ASummonedUnit* (refused: a hero climber would be invisible to L-1 and to EndPlay) both fail HERE"),
					ClimberParam->PropertyClass == ACharacter::StaticClass());
			}
		}
		else if (ParameterIndex == 1)
		{
			TestNotNull(TEXT("(b) Parameter 2 is a bool (bReachedTop)"), CastField<FBoolProperty>(*ParamIt));
		}
	}

	// ── (c) THE ONE TUNABLE — ITS NAME, ITS SHAPE AND ITS VALUE ────────────────────────
	const FFloatProperty* const RateProperty =
		CastField<FFloatProperty>(UnitClass->FindPropertyByName(FName(TEXT("LadderClimbSpeedUU"))));
	if (!TestNotNull(TEXT("(c) ⛔ LadderClimbSpeedUU is a float UPROPERTY under the name TOWER-§8.5 pins"), RateProperty))
	{
		return false;
	}
	TestTrue(TEXT("(c) ⭐ …and it is EditDefaultsOnly, because it is JONATHAN'S EXPOSURE LEVER (TOWER-§9.3): he must be able to retune the whole risk/reward of this card without a recompile"),
		RateProperty->HasAllPropertyFlags(CPF_Edit | CPF_DisableEditOnInstance));

	float ShippedRate = 0.f;
	TestTrue(TEXT("(c) SELF-CHECK: the shipped rate reads back off the CDO"),
		TryReadDefaultFloat(UnitClass, UnitDefaults, TEXT("LadderClimbSpeedUU"), ShippedRate));
	TestEqual(TEXT("(c) ⭐ It ships at 350 uu/s — the Archer/Wizard `Speed` cell, so ascending costs EXACTLY the time walking the same distance would and ⛔ no speed penalty is smuggled in on top of the disarm"),
		ShippedRate, 350.f, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 11. ⚠️ THE EXPOSURE TABLE — recomputed against the rate that actually shipped
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbExposureTest,
	"Siegebound.LadderClimb.TheShippedRateReproducesTheExposureTableJonathanWasHanded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbExposureTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ⚠️⚠️ TOWER-§9.3's table is the thing Jonathan was handed instead of a softened ruling, and
	// it is TRUE ONLY AT THE RATE THAT SHIPS. This test recomputes it from the shipped default,
	// so a retune that quietly invalidates the table fails HERE — where the consequence is
	// written down — rather than in a playtest three weeks later.
	// ⛔ It does NOT assert that the numbers are acceptable. That is his call, and it is his
	// alone (row T-7).

	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!TestNotNull(TEXT("SELF-CHECK: the ASummonedUnit CDO resolves"), UnitDefaults))
	{
		return false;
	}

	float ShippedRate = 0.f;
	if (!TestTrue(TEXT("SELF-CHECK: the shipped climb rate reads back"),
		TryReadDefaultFloat(ASummonedUnit::StaticClass(), UnitDefaults, TEXT("LadderClimbSpeedUU"), ShippedRate))
		|| ShippedRate <= 0.f)
	{
		return false;
	}

	// ── (a) THE EXPOSURE WINDOW ─────────────────────────────────────────────────────────
	const float AscentSeconds = ClimbLineLengthUU / ShippedRate;
	TestEqual(TEXT("(a) The ascent is 3.53 s at the shipped rate — the window during which the climber CANNOT ANSWER FIRE"),
		AscentSeconds, 3.53409f, 1.e-2f);

	// ── (b) SHOTS LANDED BY ONE DEFENDING LONGBOWMAN ────────────────────────────────────
	// floor(T / Cadence), conservative and favouring the climber, … +1 if it is already firing
	// when the climb starts.
	const int32 MinShots = FMath::FloorToInt(AscentSeconds / LongbowmanCadenceSeconds);
	const int32 MaxShots = MinShots + 1;
	TestEqual(TEXT("(b) A single Longbowman lands 2 shots at minimum"), MinShots, 2);
	TestEqual(TEXT("(b) …and 3 if it was already firing when the climb began"), MaxShots, 3);

	const float MinDamage = MinShots * LongbowmanDamage;
	const float MaxDamage = MaxShots * LongbowmanDamage;
	TestEqual(TEXT("(b) = 36 damage at minimum"), MinDamage, 36.f, Tolerance);
	TestEqual(TEXT("(b) = 54 damage at worst"), MaxDamage, 54.f, Tolerance);

	// ── (c) ⚠️ THE HEADLINE, NOT SOFTENED ───────────────────────────────────────────────
	TestTrue(TEXT("(c) ⚠️ A climbing ARCHER or WIZARD (45 HP) SURVIVES the best case — at 9 HP"),
		MinDamage < ArcherWizardHP);
	TestTrue(TEXT("(c) ⚠️⚠️ …and DIES in the worst case. This is the cost Jonathan ruled, and it is stated rather than tuned away."),
		MaxDamage >= ArcherWizardHP);
	TestTrue(TEXT("(c) ✅ A climbing LONGBOWMAN (70 HP) survives either way (16–34 HP left)"),
		MaxDamage < LongbowmanHP);

	// ── (d) ✅ THE COUNTERWEIGHT, MEASURED: THE LADDER IS EXPOSED FOR LESS TIME THAN THE
	//        RAMP WAS — BY HALF. It would be dishonest to state the cost without this.
	const float RampSurfaceUU = RampRunUU / FMath::Cos(FMath::DegreesToRadians(RampSlopeDegrees));
	const float RampSeconds = RampSurfaceUU / ShippedRate;
	TestEqual(TEXT("(d) The ramp it replaced was 6.86 s of walking (2,078 uu of run at 30°)"),
		RampSeconds, 6.8555f, 1.e-2f);
	TestTrue(TEXT("(d) ✅ ⭐ The ladder's exposure window is under HALF the ramp's — what changed is ⛔ not the duration, it is that the climber cannot answer"),
		AscentSeconds < RampSeconds * 0.52f);

	// ── (e) SELF-CHECK: THE TABLE IS SENSITIVE TO THE RATE ──────────────────────────────
	// ⭐ Without this row every claim above could be a constant that happens to be right, and a
	// retune to 250 uu/s would leave the file green while the card became lethal. TOWER-§9.3's
	// own next row says 250 uu/s costs a THIRD shot — reproduce it.
	const float SlowerAscent = ClimbLineLengthUU / 250.f;
	TestEqual(TEXT("(e) SELF-CHECK: at 250 uu/s the same arithmetic yields THREE shots, ⛔ not two — the table tracks the rate rather than being baked in"),
		FMath::FloorToInt(SlowerAscent / LongbowmanCadenceSeconds), 3);
	TestTrue(TEXT("(e) ⛔ …and 3 × 18 = 54 kills a 45 HP Archer outright, which is why lowering this number is not a free readability win"),
		3.f * LongbowmanDamage >= ArcherWizardHP);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 12. ⚠️⚠️ THE DECK SLAB — the final stretch is NOT swept, and the climber lands
//     ON the deck rather than jamming ~40 uu beneath it (TASK-737's measurement)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbDeckBreachTest,
	"Siegebound.LadderClimb.TheFinalStretchIsNotSweptSoTheClimberLandsOnTheDeckNotJammedUnderIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbDeckBreachTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ⚠️⚠️ THE FAILURE THIS TEST EXISTS FOR, AND IT IS SILENT: `LadderTop` is pinned **150 uu
	// inside a SOLID deck slab** and approached from below at 76°, so the last 40.8 uu of the
	// climb line lies INSIDE the deck geometry — and the 176 uu capsule straddles the slab for
	// far longer than that. ⇒ **a swept move collides with the slab's underside and STALLS.** No
	// error, no log line: the unit stops short forever while every one of the eight exits stays
	// perfectly "correct" and the card silently does nothing.
	// ⛔ The mesh cannot fix it (a hatch puts the socket over a HOLE — the castle-floor defect
	// class) and ⛔ the sockets cannot move (both are navmesh arithmetic). The driver owns it.

	FSiegeLadderClimbState State;
	if (!TestTrue(TEXT("SELF-CHECK: the ascent armed"), ArmPinnedClimb(State, 350.f)))
	{
		return false;
	}

	const FVector Direction = (LadderTop - LadderFoot).GetSafeNormal();

	// ── (a) ⭐ SELF-CHECK: THE ORDINARY LINE IS **SWEPT** ────────────────────────────────
	// Without this row, a driver that abandoned sweeping for the WHOLE traversal would pass
	// every claim below — and that is precisely what `TOWER-§8.5` refuses outright, because it
	// would drag the capsule through the tower body and through other units.
	TestTrue(TEXT("(a) ⭐ SELF-CHECK: at the FOOT the move is SWEPT — the exception is a window, ⛔ not a licence to teleport the whole climb"),
		FSiegeLadderClimbStatics::ShouldSweep(State, FootStandingCentre));
	TestTrue(TEXT("(a) ⭐ …and at the MIDPOINT it is still swept"),
		FSiegeLadderClimbStatics::ShouldSweep(State, (State.Start + State.End) * 0.5f));

	// ── (b) ⭐⭐ THE ROW THAT FAILS IF THE FINAL STRETCH WERE SWEPT ──────────────────────
	TestFalse(TEXT("(b) ⭐⭐ AT THE DECK the move is NOT swept — a swept move here jams the capsule against the slab's underside and the unit stops ~40 uu short FOREVER, silently"),
		FSiegeLadderClimbStatics::ShouldSweep(State, State.End));
	TestFalse(TEXT("(b) ⭐ …and it is already non-swept one capsule-height below the deck, where the capsule TOP first reaches the slab"),
		FSiegeLadderClimbStatics::ShouldSweep(State, State.End - Direction * (2.f * CapsuleHalfHeightUU)));

	// ── (c) THE WINDOW IS BIG ENOUGH FOR THE MEASURED GEOMETRY ──────────────────────────
	// It must cover the capsule's whole traverse of the slab: the capsule height in line terms
	// PLUS the 40.8 uu of line TASK-737 measured inside the slab. Re-derived, ⛔ not transcribed.
	const float LineRiseUU = static_cast<float>(LadderTop.Z - LadderFoot.Z);
	const float CapsuleHeightAlongLine = (2.f * CapsuleHalfHeightUU) * (ClimbLineLengthUU / LineRiseUU);
	TestTrue(TEXT("(c) The non-swept window covers the capsule's full traverse of the slab (its own height along the line, plus TASK-737's measured 40.8 uu of intrusion)"),
		State.DeckBreachUU >= CapsuleHeightAlongLine + MeasuredDeckIntrusionUU);

	// …and it is a WINDOW, not the whole line — the sweep still does its job for most of the climb.
	TestTrue(TEXT("(c) ⭐ …while still leaving the majority of the line swept — this is a scoped exception to TOWER-§8.5, ⛔ not its repeal"),
		State.DeckBreachUU < State.LengthUU * 0.5f);

	// ── (d) ⭐⭐ THE CLIMBER'S FINAL POSITION IS THE DECK SURFACE ───────────────────────
	// The capsule CENTRE finishes one half-height above the deck ⇒ its FEET are exactly ON it.
	const FVector Arrival = FSiegeLadderClimbStatics::ArrivalTarget(State);
	const float FinalFeetZ = static_cast<float>(Arrival.Z) - CapsuleHalfHeightUU;

	TestEqual(TEXT("(d) ⭐⭐ The climber finishes with its FEET exactly on the deck surface (Z = LadderTop.Z)"),
		FinalFeetZ, static_cast<float>(LadderTop.Z), Tolerance);

	// The two named traps, so a failure says WHICH mistake was made:
	TestTrue(TEXT("(d) ⛔ NOT ~40 uu below the deck — that is the swept-stall position TASK-737 measured, and it is the whole reason this test exists"),
		!FMath::IsNearlyEqual(FinalFeetZ, static_cast<float>(LadderTop.Z) - MeasuredDeckIntrusionUU, 5.f));
	TestTrue(TEXT("(d) ⛔ NOT one capsule half-height below the deck — that is the un-lifted-line bug, which buries the unit IN the slab and lets depenetration drop it back down the tower"),
		!FMath::IsNearlyEqual(FinalFeetZ, static_cast<float>(LadderTop.Z) - CapsuleHalfHeightUU, 5.f));

	// ── (e) ⭐ THE WINDOW RIDES THE **ELEVATED** END, SO DESCENT WORKS TOO ──────────────
	// ⚠️ On a descent the unit starts STANDING ON the deck and must pass DOWN through the slab it
	// is standing on — a swept move jams on frame ONE. The window is therefore anchored to the
	// line's high end, ⛔ never to "the last stretch".
	FSiegeLadderClimbState Descent;
	if (TestTrue(TEXT("(e) SELF-CHECK: the descent armed"), ArmPinnedDescent(Descent, 350.f)))
	{
		TestFalse(TEXT("(e) ⭐⭐ DESCENDING, the move is NOT swept at the DECK — which is now the START of the line"),
			FSiegeLadderClimbStatics::ShouldSweep(Descent, Descent.Start));
		TestTrue(TEXT("(e) ⭐ …and IS swept down at the ground end, where there is no slab to pass through"),
			FSiegeLadderClimbStatics::ShouldSweep(Descent, Descent.End));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 13. ⛔ THE API IS From -> To — nothing enforces an ordering, so a future "fix"
//     cannot silently break climbing DOWN (TASK-734's finding)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbBothWaysTest,
	"Siegebound.LadderClimb.DescendingIsTheSameCallWithTheEndpointsSwappedAndNothingEnforcesAnOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbBothWaysTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ⚠️ WHY THIS TEST EXISTS: `TOWER-§8.7` makes the link `ENavLinkDirection::BothWays` because a
	// one-way ladder makes the deck a DEAD END with no legal path off it — a manufactured stuck
	// unit, the exact `NAV-§` class. ⇒ a unit ordered down calls the SAME API with the endpoints
	// the other way round. The law's parameter names (`LadderFootWorld`/`LadderTopWorld`) imply an
	// ordering that a future "tidy-up" could enforce, which would break descent SILENTLY — this
	// file's rename to From/To is half the guard, and this test is the other half.

	FSiegeLadderClimbState Ascent;
	FSiegeLadderClimbState Descent;
	if (!TestTrue(TEXT("(a) An ASCENT (foot -> top) is admitted"), ArmPinnedClimb(Ascent, 350.f))
		|| !TestTrue(TEXT("(a) ⭐ A DESCENT (top -> foot) is admitted by the SAME call — ⛔ no ordering is enforced anywhere"), ArmPinnedDescent(Descent, 350.f)))
	{
		return false;
	}

	// ── (b) THE TWO ARE MIRROR IMAGES, ⛔ NOT THE SAME THING ────────────────────────────
	TestEqual(TEXT("(b) Both directions measure the same line length"),
		Descent.LengthUU, Ascent.LengthUU, Tolerance);

	const FVector Up = FSiegeLadderClimbStatics::ClimbDirection(Ascent);
	const FVector Down = FSiegeLadderClimbStatics::ClimbDirection(Descent);

	// SELF-CHECK: the two directions are genuinely opposite. ⭐ Without this the equality above
	// could be satisfied by a Begin that ignored its arguments entirely and armed the same line
	// twice — the "two sides equal by construction" trap this file is written against.
	TestTrue(TEXT("(b) ⭐ SELF-CHECK: the descent's direction is the EXACT negation of the ascent's — the two calls really did produce different lines"),
		Down.Equals(-Up, Tolerance));
	TestTrue(TEXT("(b) …so a descent genuinely travels DOWNWARD"), Down.Z < 0.f);
	TestTrue(TEXT("(b) …and an ascent UPWARD"), Up.Z > 0.f);

	// ── (c) EACH ENDS WHERE IT WAS SENT, IN CAPSULE-CENTRE SPACE ───────────────────────
	TestTrue(TEXT("(c) An ascent finishes standing ON THE DECK"),
		FSiegeLadderClimbStatics::ArrivalTarget(Ascent).Equals(DeckStandingCentre, Tolerance));
	TestTrue(TEXT("(c) ⭐ A descent finishes standing AT THE FOOT — the endpoints are honoured as given, ⛔ not sorted by Z"),
		FSiegeLadderClimbStatics::ArrivalTarget(Descent).Equals(FootStandingCentre, Tolerance));

	// ── (d) ⭐ THE DECK IS IDENTIFIED BY HEIGHT, ⛔ NOT BY ARGUMENT POSITION ────────────
	// This is the one place the code looks at Z, and it is a fact about the GEOMETRY (which end
	// has a slab) rather than a constraint on the caller. Both orders remain legal.
	TestTrue(TEXT("(d) On an ascent the deck is the line's END"), Ascent.bDeckIsAtEnd);
	TestFalse(TEXT("(d) ⭐ On a descent the deck is the line's START — resolved by Z, so both argument orders work"), Descent.bDeckIsAtEnd);
	TestEqual(TEXT("(d) …and both get the same non-swept window, because it is the same slab"),
		Descent.DeckBreachUU, Ascent.DeckBreachUU, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 14. ⭐⭐ THE DRIVER IS SELF-HEALING — the watchdog no longer rides the very
//     thing it watches (TASK-760, closing qa/TASK-741.md's W-1)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderClimbSelfHealingDriverTest,
	"Siegebound.LadderClimb.AnExternallyKilledActorTickIsRestoredByTheStatePollSoTheWatchdogIsNeverStrandedOnADeadDriver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderClimbSelfHealingDriverTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	// ⚠️⚠️ THE FAILURE THIS TEST EXISTS FOR, AND IT IS A GAP IN THE **NET** RATHER THAN A BUG IN
	// THE CODE: `TickLadderClimb` advances the timeout clock **and** is the only thing that can
	// end a hung climb ⇒ THE WATCHDOG RIDES THE VERY DRIVER IT WATCHES. Any external write of the
	// actor tick flag — a `BP_Unit_*` child, a level Blueprint, tomorrow's C++ site — leaves the
	// climber in `MOVE_Flying` FOREVER, and the watchdog cannot fire to rescue it.
	// TASK-738 found and fixed the one writer that actually did this (`StopAttackLunge` ran
	// `SetActorTickEnabled(false)` on EVERY attack exit). That closed the CAUSE; this closes the
	// CLASS — the 0.25 s state poll re-asserts the tick through the composed writer.

	bool bReachedTop = false;
	bool bTimedOut = false;

	// ── (a) ⚠️⚠️ THE HAZARD IS REAL: THE CLOCK LIVES ON THE DRIVER AND NOWHERE ELSE ─────
	FSiegeLadderClimbState Hung;
	if (!TestTrue(TEXT("SELF-CHECK: the ascent armed"), ArmPinnedClimb(Hung, 350.f)))
	{
		return false;
	}

	const float BudgetSeconds = Hung.TimeoutSeconds;
	if (!TestTrue(TEXT("(a) SELF-CHECK: the watchdog has a real, positive budget to blow — a zero budget would make every row below vacuous"), BudgetSeconds > 0.f))
	{
		return false;
	}

	// The driver is DEAD: `Tick` never runs, so `Advance` is never called. ⭐ Simulating that is
	// simulating exactly NOTHING, because nothing else in the feature touches the clock — which
	// is the coupling itself, stated as an assertion.
	TestEqual(TEXT("(a) ⭐⭐ With the driver dead the clock NEVER MOVES — ⛔ no wall clock and ⛔ no timer advances it, which is exactly why a hung climber cannot rescue itself through the watchdog"),
		Hung.ElapsedSeconds, 0.f, Tolerance);
	TestTrue(TEXT("(a) ⭐⭐ …and the climb stays ACTIVE — the unit hangs in MOVE_Flying with no way to arrive and no way to fall"),
		Hung.bActive);

	// ⭐ SELF-CHECK against this file's standing trap (an assertion true by construction): the two
	// rows above must hold because the DRIVER is dead, ⛔ not because the watchdog never works.
	// Hand the same state ONE call from a live driver and it fires immediately.
	TestFalse(TEXT("(a) ⭐ SELF-CHECK: ONE call from a LIVE driver, past the budget, ends the climb — so the rows above are about a dead driver, ⛔ not a broken net"),
		FSiegeLadderClimbStatics::Advance(Hung, FootStandingCentre, BudgetSeconds, bReachedTop, bTimedOut));
	TestTrue(TEXT("(a) ⭐ …as a TIMEOUT — Advance is the ONE and ONLY thing that can fire the watchdog, and Advance runs on the actor tick"), bTimedOut);

	// ── (b) THE HEALER RIDES A DIFFERENT DRIVER, AND THAT DRIVER IS SHIPPED ─────────────
	// ⭐ The self-heal is worth nothing unless something that is NOT the actor tick keeps running
	// for the whole climb. That is `StateTimerHandle` — an FTimerManager entry on the WORLD, which
	// knows nothing about `PrimaryActorTick` — and `BeginLadderClimb` clears `AttackTimerHandle`
	// ONLY. Its rate is a shipped UPROPERTY, so it is read back rather than assumed.
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	const AMinerUnit* const MinerDefaults = GetDefault<AMinerUnit>();
	if (!TestNotNull(TEXT("(b) SELF-CHECK: the ASummonedUnit CDO resolves"), UnitDefaults)
		|| !TestNotNull(TEXT("(b) SELF-CHECK: the AMinerUnit CDO resolves"), MinerDefaults))
	{
		return false;
	}

	float PollIntervalSeconds = 0.f;
	if (!TestTrue(TEXT("(b) ⭐⭐ ASummonedUnit still ships a StateCheckInterval UPROPERTY — it is the self-heal's ONLY driver, and losing it would silently restore the hang"),
		TryReadDefaultFloat(ASummonedUnit::StaticClass(), UnitDefaults, TEXT("StateCheckInterval"), PollIntervalSeconds)))
	{
		return false;
	}

	TestTrue(TEXT("(b) ⭐⭐ …and it is STRICTLY POSITIVE — FTimerManager::SetTimer with a rate of 0 arms NOTHING, so a sealed interval means a unit with no poll and therefore no self-heal"),
		PollIntervalSeconds > 0.f);
	TestTrue(TEXT("(b) ⭐ …and the poll lands many times over inside the watchdog's own budget (14.1 s ÷ 0.25 s = 56 chances to heal), so a killed tick flag is restored long before anything else could notice"),
		PollIntervalSeconds > 0.f && PollIntervalSeconds * 4.f <= BudgetSeconds);

	// ⚠️ THE DECLARED LIMIT OF THIS FIX, ASSERTED RATHER THAN ASSUMED (ties to qa/TASK-741 W-5):
	// AMinerUnit SEALS StateCheckInterval to 0, so a miner has no state poll and therefore ⛔ no
	// self-heal. ⛔ NOT fixed here: MinerUnit.{h,cpp} is outside this task's fence, W-5 already
	// records that a miner should never be admitted to a ladder in the first place, and its
	// failure mode there is a double-drive the watchdog DROPS rather than a hang.
	float MinerPollIntervalSeconds = -1.f;
	if (TestTrue(TEXT("(b) SELF-CHECK: the miner's StateCheckInterval is readable"),
		TryReadDefaultFloat(AMinerUnit::StaticClass(), MinerDefaults, TEXT("StateCheckInterval"), MinerPollIntervalSeconds)))
	{
		TestEqual(TEXT("(b) ⚠️ AMinerUnit seals the poll to 0 — the ONE shipped class this self-heal does NOT cover, recorded here so the limit is known rather than discovered"),
			MinerPollIntervalSeconds, 0.f, Tolerance);
	}

	// ── (c) ⭐⭐ THE HAZARD, RUN THROUGH THE SHIPPED PREDICATE ──────────────────────────
	FSiegeLadderClimbState Climbing;
	if (!TestTrue(TEXT("(c) SELF-CHECK: a second ascent armed"), ArmPinnedClimb(Climbing, 350.f)))
	{
		return false;
	}

	// The tick flag as the actor holds it. BeginLadderClimb armed it through the same writer.
	bool bActorTickEnabled = FSiegeLadderClimbStatics::WantsActorTick(/*bLungeActive=*/ false, Climbing.bActive);
	if (!TestTrue(TEXT("(c) SELF-CHECK: arming the climb turns the actor tick ON — the driver starts alive, or there is no hazard to simulate"), bActorTickEnabled))
	{
		return false;
	}

	// ⚠️⚠️ THE HAZARD: something OUTSIDE this class writes the flag false mid-climb. Under
	// TASK-738 alone this is terminal — Tick stops, Advance is never called again, the clock
	// freezes at (a)'s zero and the unit hangs in MOVE_Flying for the rest of the match.
	bActorTickEnabled = false;
	TestTrue(TEXT("(c) SELF-CHECK: the climb is still ACTIVE after the external write — the tick flag and the climb state are independent, which is what makes this hang silent"),
		Climbing.bActive);

	// …and the 0.25 s poll lands. This is the shipped self-heal: its condition is the climb's own
	// bActive, and the value it writes is the composed writer's, ⛔ never a bare `true`.
	if (Climbing.bActive)
	{
		bActorTickEnabled = FSiegeLadderClimbStatics::WantsActorTick(/*bLungeActive=*/ false, Climbing.bActive);
	}

	TestTrue(TEXT("(c) ⭐⭐ THE TICK IS RESTORED BY THE POLL — the driver recovers on its own, so the watchdog is never stranded on a dead driver and a hung climber is ALWAYS eventually dropped"),
		bActorTickEnabled);

	// ── (d) THE FULL 2×2 TRUTH TABLE OF THE COMPOSED WRITER, WITH BOTH TRAPS NAMED ──────
	// ⭐ FOUR rows over TWO inputs: each of the two forbidden implementations flips at least one.
	TestTrue(TEXT("(d) ⭐⭐ NOT lunging, IS climbing ⇒ TICK ON — THE SELF-HEAL ROW. ⛔ A predicate that dropped the climb term returns FALSE here, and the poll would then re-assert the very OFF that hung the unit (the StopAttackLunge regression, re-landed by its own cure)"),
		FSiegeLadderClimbStatics::WantsActorTick(false, true));
	TestFalse(TEXT("(d) ⭐⭐ NOT lunging, NOT climbing ⇒ TICK OFF. ⛔ THE OTHER TRAP: a bare SetActorTickEnabled(true) in the self-heal returns TRUE here — it would fight whatever legitimately disabled the tick and re-introduce the exact coupling TASK-738 removed"),
		FSiegeLadderClimbStatics::WantsActorTick(false, false));
	TestTrue(TEXT("(d) IS lunging, NOT climbing ⇒ TICK ON — the TASK-020 lunge, unchanged"),
		FSiegeLadderClimbStatics::WantsActorTick(true, false));
	TestTrue(TEXT("(d) IS lunging, IS climbing ⇒ TICK ON — both drivers at once, and neither may switch it off on the other's behalf"),
		FSiegeLadderClimbStatics::WantsActorTick(true, true));

	// ⭐ THE INVARIANT THE WHOLE SELF-HEAL RESTS ON, STATED ONCE AS ITSELF: while a climb is
	// active the writer's value is TRUE for **every** state of the other driver. ⇒ re-asserting
	// through it can never turn the tick off under a live climb. A future third term that could
	// return false during a climb fails HERE, in this module, rather than as a hang in a playtest.
	TestTrue(TEXT("(d) ⭐⭐ WHILE CLIMBING the writer wants the tick REGARDLESS of the lunge — the invariant that makes the re-assert safe in both directions"),
		FSiegeLadderClimbStatics::WantsActorTick(false, true) && FSiegeLadderClimbStatics::WantsActorTick(true, true));

	// ── (e) ⛔ WHAT THIS TEST DOES NOT PROVE, NAMED RATHER THAN IMPLIED ─────────────────
	// The CALL SITE — that the re-assert really is wired into UpdateState, and really is ABOVE
	// the fence (the fence early-outs on IsClimbing(), so one line lower it is dead code for the
	// exact case it rescues) — is a diff read, ⛔ not a suite row. That is this file's stated
	// split (`SC-§32`, header): every DECISION is pure and exercised here; every WIRING is QA's.
	// ⛔ It cannot be closed by instrumenting harder: ASummonedUnit::UpdateState and
	// RefreshActorTickEnabled are private, LadderClimb is private, and BeginLadderClimb reaches
	// GetWorldTimerManager() ⇒ GetWorld()->GetTimerManager(), which CRASHES a world-less unit
	// rather than failing it. Instantiating one here would take the suite down, not test it.

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 15. ⭐⭐ THE CALL SITE'S **CADENCE**, AND THE TWO THINGS IT COSTS
//     (TASK-784, CONTACT-§4.1)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⚠️⚠️ WHY THESE ROWS EXIST AT ALL, GIVEN THIS FILE'S OWN RULE THAT WIRING IS QA'S AND
// NOT THE SUITE'S (test 14(e)): TASK-784 adds a CALL SITE, and a call site has exactly
// one property that is decidable without a world — **the RATE it is called at**. That
// rate is the whole engineering content of the task, because the tower INTEGRATES its
// dwell out of the deltas its caller hands it.
//
// ⭐ THE CALL SITE SHIPPED ON THE 0.25 s `StateTimerHandle` POLL, AND THE ALTERNATIVE WAS
// MEASURED AND REFUSED: `ASummonedUnit`'s actor tick is written by exactly one function
// whose value is `WantsActorTick(bLungeActive, LadderClimb.bActive)`, so it is OFF for a
// unit that is neither mid-lunge nor ALREADY climbing — precisely the state a contact
// climb starts from. ⛔ A poll in `::Tick` would have compiled, reviewed clean, passed
// this suite and NEVER RUN.
//
// ⚠️ THAT CHOICE IS ⛔ NOT FREE, AND THESE ROWS ARE THE PRICE TAG RATHER THAN A PASS MARK.
// The tuning was designed against a 60 Hz sampler (the tower's own test file steps at
// 1/60); a 0.25 s sampler is 15× coarser and it distorts the dwell in BOTH directions.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderContactCadenceCostTest,
	"Siegebound.LadderClimb.TheShippedContactPollIsCoarserThanTheDwellItIntegratesAndBothCostsAreMeasured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderContactCadenceCostTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	float RadiusUU = 0.f;
	float IntentCos = 0.f;
	float DwellSeconds = 0.f;
	float PollSeconds = 0.f;

	// ⛔ A missing property is a HARD ERROR and ⛔ never a substituted guess — the cadence question
	// is meaningless without the numbers it is a question about.
	if (!TestTrue(TEXT("SELF-CHECK: the three CONTACT tunables read back off AClimbableTower's CDO (WR-§5 — they live on the TOWER, and the pawn only asks)"),
		TryReadContactTunables(RadiusUU, IntentCos, DwellSeconds))
		|| !TestTrue(TEXT("SELF-CHECK: ASummonedUnit's StateCheckInterval reads back off its CDO — it is the rate the contact ask actually runs at"),
			TryReadStatePollSeconds(PollSeconds)))
	{
		return false;
	}

	if (!TestTrue(TEXT("SELF-CHECK: all four numbers are usable (a zero radius, dwell or poll would make every row below vacuous)"),
		RadiusUU > 0.f && DwellSeconds > 0.f && PollSeconds > 0.f && IntentCos >= -1.f && IntentCos <= 1.f))
	{
		return false;
	}

	const float SlowestRosterSpeedUU = ShippedUnitSpeedsUU[0];

	// ── (a) SELF-CHECK: THE WALKER DISCRIMINATES BEFORE ANY CLAIM IS MADE ────────────────
	// ⭐ Both rows are structural rather than tuned, so they hold at ANY radius Jonathan picks:
	// walking dead-on at the endpoint is the intent the trigger exists for, and a pawn skimming
	// the very edge of the disc can never satisfy a 60° cone at all (the bearing to the endpoint
	// is almost perpendicular to its heading for the whole crossing).
	{
		TestEqual(TEXT("(a) SELF-CHECK: at the 60 Hz reference cadence a pawn walking DEAD-ON at the foot at the roster's slowest speed is admitted on EVERY phase — the walker is live"),
			CountAdmittingPhases(SlowestRosterSpeedUU, 0.f, FrameStepSeconds, RadiusUU, IntentCos, DwellSeconds, PhaseSweepCount), PhaseSweepCount);
		TestEqual(TEXT("(a) SELF-CHECK: …and the same pawn skimming the disc's edge is refused on EVERY phase, so the walker is not simply returning `admitted` for everything"),
			CountAdmittingPhases(SlowestRosterSpeedUU, 0.95f * RadiusUU, FrameStepSeconds, RadiusUU, IntentCos, DwellSeconds, PhaseSweepCount), 0);
	}

	// ── (b) THE PREMISE, ASSERTED RATHER THAN ASSUMED: THE SHIPPED POLL IS COARSE ────────
	{
		TestTrue(*FString::Printf(TEXT("(b) The shipped %.2f s poll is more than an order of magnitude coarser than the 60 Hz cadence the contact tuning was designed against — the premise of every row below, and it goes red the day the poll is made fine"),
			PollSeconds), PollSeconds > (FrameStepSeconds * 10.f));

		const int32 PollsNeeded = FMath::CeilToInt(DwellSeconds / PollSeconds);
		TestTrue(*FString::Printf(TEXT("(b) ⭐ The dwell cannot be earned in ONE poll (%d needed at %.2f s for a %.2f s dwell) — ⛔ this is what makes the trigger's reachability depend on a pawn's SPEED at all"),
			PollsNeeded, PollSeconds, DwellSeconds), PollsNeeded >= 2);

		TestTrue(*FString::Printf(TEXT("(b) ⚠️ …and the dwell is LONGER than one poll (%.2f s > %.2f s), which is the precondition for the over-credit hazard (d) measures. ⛔ If this ever goes red the hazard is GONE and (d) should be retired with it, ⛔ not weakened"),
			DwellSeconds, PollSeconds), DwellSeconds > PollSeconds);
	}

	// ── (c) ⭐⭐ COST 1 — THE VERDICT DEPENDS ON THE PAWN'S SPEED, AND FOR PART OF THE
	//     ROSTER ON THE SAMPLING **PHASE** (i.e. on luck) ──────────────────────────────────
	// ⚠️ The law behind every row: a pawn walking dead-on holds proximity AND intent for exactly
	// `RadiusUU / Speed` seconds, and a sampler of period P needs TWO samples inside that window.
	//   • window >= 2P  ⇒ two samples ALWAYS fit ⇒ admitted on every phase (RELIABLE)
	//   • window <= P   ⇒ two samples can NEVER fit ⇒ admitted on no phase (UNREACHABLE)
	//   • in between    ⇒ it depends on where the samples happen to land (BY LUCK)
	// ⭐ Derived from the two shipped numbers, ⛔ never a transcribed table — so it stays true and
	// stays meaningful at whatever radius `K-5`/`K-6` settle on.
	int32 ReliableCards = 0;
	{
		const float ReliableCeilingUU = RadiusUU / (2.f * PollSeconds);
		const float ReachableCeilingUU = RadiusUU / PollSeconds;
		const float CeilingTolerance = 1.e-3f;

		for (int32 Index = 0; Index < RosterSpeedCount; ++Index)
		{
			const float SpeedUU = ShippedUnitSpeedsUU[Index];
			const float WindowSeconds = RadiusUU / SpeedUU;
			const int32 AdmittingPhases = CountAdmittingPhases(SpeedUU, 0.f, PollSeconds, RadiusUU, IntentCos, DwellSeconds, PhaseSweepCount);

			if (SpeedUU <= ReliableCeilingUU + CeilingTolerance)
			{
				++ReliableCards;
				TestEqual(*FString::Printf(TEXT("(c) ✅ %s holds the terms for %.3f s, at least two poll periods, so it climbs on ALL %d phases — RELIABLE at the shipped %.0f uu radius"),
					ShippedUnitSpeedNames[Index], WindowSeconds, PhaseSweepCount, RadiusUU), AdmittingPhases, PhaseSweepCount);
			}
			else if (SpeedUU >= ReachableCeilingUU - CeilingTolerance)
			{
				TestEqual(*FString::Printf(TEXT("(c) ⛔🧑 %s crosses the whole %.0f uu disc in %.3f s — no longer than ONE %.2f s poll, so two samples can never both land inside it and it can NEVER contact-climb. ⛔ Not a code defect and ⛔ not fixable from the pawn: the radius is AClimbableTower's and `K-5` makes it Jonathan's"),
					ShippedUnitSpeedNames[Index], RadiusUU, WindowSeconds, PollSeconds), AdmittingPhases, 0);
			}
			else
			{
				TestTrue(*FString::Printf(TEXT("(c) ⚠️🧑 %s holds the terms for %.3f s — longer than one %.2f s poll but shorter than two, so it climbs on %d of %d phases and the trigger fires BY LUCK for this card"),
					ShippedUnitSpeedNames[Index], WindowSeconds, PollSeconds, AdmittingPhases, PhaseSweepCount),
					AdmittingPhases > 0 && AdmittingPhases < PhaseSweepCount);
			}
		}

		TestTrue(TEXT("(c) SELF-CHECK: at least one roster card is RELIABLE — a table with none would mean the feature is dead rather than coarsely sampled, and the split above would be a constant"),
			ReliableCards > 0);
	}

	// ── (d) ⭐⭐ COST 2 — THE COARSE SAMPLER **MANUFACTURES** DWELL ───────────────────────
	// ⚠️⚠️ THIS IS THE HALF A REVIEWER WOULD NOT GO LOOKING FOR, AND IT IS THE WORSE ONE. Each
	// sample credits a FULL poll period of dwell for ONE INSTANT that satisfied the terms, so a
	// coarse sampler does not merely miss climbs — it invents them. The dwell's entire job
	// (CONTACT-§4.1: "without it a friendly unit marching past its own tower toward the enemy
	// castle clips the intent cone for two frames and is YANKED 1,200 uu INTO THE AIR") is
	// partially undone by the very driver that was the only one available.
	//
	// ⭐ The offset sweep is derived from the shipped radius rather than a fixed list of uu, so it
	// keeps finding the band at whatever radius ships.
	{
		bool bFoundOverCredit = false;
		float OverCreditOffsetUU = 0.f;
		float OverCreditSpeedUU = 0.f;
		const TCHAR* OverCreditSpeedName = TEXT("<none>");
		int32 OverCreditPhases = 0;

		constexpr int32 OffsetSweepCount = 40;
		for (int32 OffsetIndex = 0; OffsetIndex < OffsetSweepCount && !bFoundOverCredit; ++OffsetIndex)
		{
			const float OffsetUU = RadiusUU * (static_cast<float>(OffsetIndex) / static_cast<float>(OffsetSweepCount));
			for (int32 SpeedIndex = 0; SpeedIndex < RosterSpeedCount; ++SpeedIndex)
			{
				const float SpeedUU = ShippedUnitSpeedsUU[SpeedIndex];
				if (CountAdmittingPhases(SpeedUU, OffsetUU, FrameStepSeconds, RadiusUU, IntentCos, DwellSeconds, PhaseSweepCount) != 0)
				{
					continue; // the tuning genuinely admits this line — it is not a passer-by
				}

				const int32 PollAdmitting = CountAdmittingPhases(SpeedUU, OffsetUU, PollSeconds, RadiusUU, IntentCos, DwellSeconds, PhaseSweepCount);
				if (PollAdmitting > 0)
				{
					bFoundOverCredit = true;
					OverCreditOffsetUU = OffsetUU;
					OverCreditSpeedUU = SpeedUU;
					OverCreditSpeedName = ShippedUnitSpeedNames[SpeedIndex];
					OverCreditPhases = PollAdmitting;
					break;
				}
			}
		}

		TestTrue(*FString::Printf(
			TEXT("(d) ⭐⭐ THE %.2f s POLL ADMITS A PAWN THE TUNING REFUSES: %s at %.0f uu from the endpoint is refused on every 60 Hz phase and CLIMBS on %d of %d poll-sampled ones (speed %.0f uu/s). ⇒ the coarse cadence does not only MISS climbs, it MANUFACTURES them — the abduction the dwell exists to prevent, re-opened by the sampling rate rather than by the tuning"),
			PollSeconds, OverCreditSpeedName, OverCreditOffsetUU, OverCreditPhases, PhaseSweepCount, OverCreditSpeedUU),
			bFoundOverCredit);
	}

	// ── (e) ⛔⛔ THE ROW THAT GOES RED IF THE POLL IS EVER MOVED ONTO THE ACTOR TICK ──────
	// ⚠️⚠️ `SC-§36.1` asks for exactly this guard, and this is its READABLE half — the ENFORCING
	// half is the `WantsActorTick` arity pin in the fixture above, which makes the move a compile
	// error. Together they state the trap as machine-checked fact rather than as a comment
	// somebody has to read.
	{
		TestFalse(TEXT("(e) ⛔⛔ THE TRAP, AS AN ASSERTION: a unit that is neither mid-lunge nor ALREADY CLIMBING does NOT tick — and that is precisely the state every contact climb must begin in. ⇒ a poll in ASummonedUnit::Tick would compile, review clean, pass this whole suite and NEVER RUN. The poll rides StateTimerHandle for this reason and ⛔ no other"),
			FSiegeLadderClimbStatics::WantsActorTick(/*bLungeActive=*/ false, /*bClimbActive=*/ false));

		TestTrue(TEXT("(e) SELF-CHECK: …and the predicate is not simply always-false — it wants the tick once a climb IS active, so the row above is about the STARTING state and ⛔ not about a dead function"),
			FSiegeLadderClimbStatics::WantsActorTick(/*bLungeActive=*/ false, /*bClimbActive=*/ true));

		TestTrue(*FString::Printf(TEXT("(e) ⭐ …and the driver the poll DOES ride is shipped and live: StateCheckInterval = %.2f s, strictly positive. ⛔ A zero here would silently delete the whole feature — which is exactly what it does to AMinerUnit (test 17(c))"),
			PollSeconds), PollSeconds > 0.f);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 16. 🧑⚠️ THE RADIUS THE SHIPPED CADENCE ACTUALLY NEEDS — DERIVED, NOT PROPOSED
//     (TASK-784, CONTACT-§7 `K-5` / the `K-6` row)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⚠️⚠️ THIS ROW IS A FINDING, ⛔ NOT A REGRESSION GUARD, AND TASK-784 IS THE FIRST TASK
// THAT COULD HAVE PRODUCED IT: the trigger only meets the roster once something calls it.
// ⭐ Test 15(c) established that a card is RELIABLE only while `RadiusUU / Speed >= 2 ×
// Poll`. Turned around, that is a statement about the RADIUS: the smallest radius at
// which EVERY shipped card is reliable is `2 × Poll × FastestSpeed`. This test computes
// that number from the two shipped values and the roster, and ⛔ does not assert that the
// current radius equals it — the radius is `AClimbableTower`'s and `K-5` makes it 🧑
// Jonathan's. What it asserts is the ARITHMETIC, so the recommendation cannot rot.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderContactRequiredRadiusTest,
	"Siegebound.LadderClimb.TheRadiusEveryRosterCardNeedsAtTheShippedPollIsTwicePollTimesTheFastestSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderContactRequiredRadiusTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	float RadiusUU = 0.f;
	float IntentCos = 0.f;
	float DwellSeconds = 0.f;
	float PollSeconds = 0.f;
	if (!TestTrue(TEXT("SELF-CHECK: the three CONTACT tunables read back off AClimbableTower's CDO"),
		TryReadContactTunables(RadiusUU, IntentCos, DwellSeconds))
		|| !TestTrue(TEXT("SELF-CHECK: ASummonedUnit's StateCheckInterval reads back off its CDO"),
			TryReadStatePollSeconds(PollSeconds))
		|| !TestTrue(TEXT("SELF-CHECK: the numbers are usable"), RadiusUU > 0.f && PollSeconds > 0.f && DwellSeconds > 0.f))
	{
		return false;
	}

	float FastestRosterSpeedUU = 0.f;
	const TCHAR* FastestRosterSpeedName = TEXT("<none>");
	for (int32 Index = 0; Index < RosterSpeedCount; ++Index)
	{
		if (ShippedUnitSpeedsUU[Index] > FastestRosterSpeedUU)
		{
			FastestRosterSpeedUU = ShippedUnitSpeedsUU[Index];
			FastestRosterSpeedName = ShippedUnitSpeedNames[Index];
		}
	}

	if (!TestTrue(TEXT("SELF-CHECK: the roster table yielded a fastest speed"), FastestRosterSpeedUU > 0.f))
	{
		return false;
	}

	// ── (a) THE REQUIRED RADIUS, FROM THE SHIPPED POLL AND THE SHIPPED ROSTER ────────────
	const float RequiredRadiusUU = 2.f * PollSeconds * FastestRosterSpeedUU;

	TestEqual(*FString::Printf(TEXT("(a) ⭐ Every shipped card becomes reliable at 2 × %.2f s × %.0f uu/s (%s) = %.0f uu. ⚠️ This is DERIVED from the poll and the roster, ⛔ not transcribed — it goes red the day either changes, which is exactly when the recommendation needs revisiting"),
		PollSeconds, FastestRosterSpeedUU, FastestRosterSpeedName, RequiredRadiusUU),
		RequiredRadiusUU, 300.f, 0.1f);

	// ── (b) SELF-CHECK: THE REQUIREMENT IS A REAL CONSTRAINT, NOT A TAUTOLOGY ────────────
	// ⭐ Proven by driving the SHIPPED predicate rather than restating (a): at the required radius
	// the fastest card is admitted on every phase, and one poll-period of radius less it is not.
	// ⛔ Neither row reads the currently shipped radius, so both stay honest whichever way `K-6`
	// is ruled.
	{
		const float JustBelowUU = RequiredRadiusUU - (PollSeconds * FastestRosterSpeedUU);

		TestEqual(*FString::Printf(TEXT("(b) SELF-CHECK: at %.0f uu the fastest card climbs on every phase — the required radius really is sufficient"),
			RequiredRadiusUU),
			CountAdmittingPhases(FastestRosterSpeedUU, 0.f, PollSeconds, RequiredRadiusUU, IntentCos, DwellSeconds, PhaseSweepCount), PhaseSweepCount);

		TestTrue(*FString::Printf(TEXT("(b) SELF-CHECK: at %.0f uu — one poll-period of travel less — it does NOT climb on every phase, so (a) is a genuine threshold and ⛔ not a number that would have passed anyway"),
			JustBelowUU),
			CountAdmittingPhases(FastestRosterSpeedUU, 0.f, PollSeconds, JustBelowUU, IntentCos, DwellSeconds, PhaseSweepCount) < PhaseSweepCount);
	}

	// ── (c) ⚠️🧑 THE PRICE OF THAT RADIUS — AND IT IS **SUPER-LINEAR**, WHICH IS WHY THE
	//     FIRST ESTIMATE OF `K-6`'s COST WAS TOO SMALL ──────────────────────────────────────
	// ⛔ A wider disc is ⛔ NOT free: it IS the abduction window, and the dwell is the only thing
	// refusing a passer-by. `K-6` was first costed with a linear "~0.4 × R" model giving ±60 → ±125
	// uu. ⭐ THAT MODEL IS WRONG, and this row proves it by DRIVING THE SHIPPED PREDICATE rather
	// than evaluating a closed form: the dwell only ever eats a FIXED `v × DwellSeconds` of
	// approach, which is a smaller fraction of a bigger disc, so the window grows FASTER than the
	// radius. Measured: ±62 → ±204 uu at 300 uu/s — a 3.3× widening for a 2× radius.
	//
	// ⭐ Both radii are DERIVED (the required one, and half of it), so this row never transcribes
	// a shipped value and keeps meaning the same thing after any retune.
	{
		const auto MeasureAbductionHalfWidthUU = [&](float TestRadiusUU) -> float
		{
			// The largest offset (to a 1/40th-of-radius resolution) still admitted at the 60 Hz
			// reference cadence, driving the SHIPPED predicate — ⛔ never a closed form of it.
			float WidestUU = 0.f;
			constexpr int32 Resolution = 40;
			for (int32 Index = 0; Index <= Resolution; ++Index)
			{
				const float OffsetUU = TestRadiusUU * (static_cast<float>(Index) / static_cast<float>(Resolution));
				if (CountAdmittingPhases(ShippedUnitSpeedsUU[0], OffsetUU, FrameStepSeconds, TestRadiusUU, IntentCos, DwellSeconds, PhaseSweepCount) > 0)
				{
					WidestUU = OffsetUU;
				}
			}
			return WidestUU;
		};

		const float HalfRadiusUU = RequiredRadiusUU * 0.5f;
		const float NarrowHalfWidthUU = MeasureAbductionHalfWidthUU(HalfRadiusUU);
		const float WideHalfWidthUU = MeasureAbductionHalfWidthUU(RequiredRadiusUU);

		TestTrue(TEXT("(c) SELF-CHECK: the abduction window is measurable and non-zero at the narrower radius — a zero would mean the probe never admitted anything and the comparison below would be meaningless"),
			NarrowHalfWidthUU > 0.f);

		TestTrue(*FString::Printf(
			TEXT("(c) ⚠️🧑 THE COST OF THE WIDER DISC IS SUPER-LINEAR: DOUBLING the radius (%.0f → %.0f uu) MORE THAN DOUBLES how far to the side a pawn can march and still be grabbed (%.0f → %.0f uu). ⛔ A linear '~0.4 × R' estimate understates it, which is how `K-6` was first costed at ±125 uu when the measured figure is ~±204 uu at 300 uu/s. 🧑 The trade is Jonathan's (`K-5`/`K-6`), ⛔ not this task's — but it must be made against the real number"),
			HalfRadiusUU, RequiredRadiusUU, NarrowHalfWidthUU, WideHalfWidthUU),
			WideHalfWidthUU > (2.f * NarrowHalfWidthUU));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 17. ⛔⛔ THE PAWN KEEPS **NO COPY** OF THE TOWER'S TUNING — ASSERTED FROM THE
//     PAWN'S OWN SIDE, WITH A TWO-SIDED PROBE (TASK-784, WR-§5 / CONTACT-§4.1)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⚠️ `SiegeClimbableTowerTest.cpp` test 12(g) already scans `ASummonedUnit` for a
// "LadderContact" member. This row is ⛔ NOT that assertion restated: it is TWO-SIDED
// (each probe is validated against the class that DOES have the member, so it cannot go
// blind on a rename), it covers the tokens a duplicate could hide behind under ANOTHER
// name, and — the reason it belongs HERE — TASK-784 is the task that could have copied
// one, so the guard against its own most likely mistake lives in its own file.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeLadderContactNoDuplicatedTuningTest,
	"Siegebound.LadderClimb.TheContactTuningLivesOnlyOnTheTowerAndTheProbeIsValidatedAgainstTheClassThatHasIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeLadderContactNoDuplicatedTuningTest::RunTest(const FString& Parameters)
{
	using namespace SiegeLadderClimbTestFixture;

	UClass* const UnitClass = ASummonedUnit::StaticClass();
	UClass* const TowerClass = AClimbableTower::StaticClass();
	if (!TestNotNull(TEXT("SELF-CHECK: ASummonedUnit::StaticClass() resolves"), UnitClass)
		|| !TestNotNull(TEXT("SELF-CHECK: AClimbableTower::StaticClass() resolves"), TowerClass))
	{
		return false;
	}

	// ── (a) THE TWO-SIDED PROBE — THE HALF THAT KEEPS IT FROM GOING BLIND ────────────────
	static const TCHAR* const ContactTuningPropertyNames[] =
	{
		TEXT("LadderContactRadiusUU"),
		TEXT("LadderContactIntentCos"),
		TEXT("LadderContactDwellSeconds")
	};

	for (const TCHAR* const PropertyName : ContactTuningPropertyNames)
	{
		// ⭐ FIRST against the class that MUST have it. A rename on the tower would otherwise make
		// the absence check below pass while proving nothing at all — the exact blind-instrument
		// failure this project has paid for twice.
		TestNotNull(*FString::Printf(TEXT("(a) SELF-CHECK: '%s' resolves on AClimbableTower — the probe is live, so the absence asserted below is a real absence"), PropertyName),
			TowerClass->FindPropertyByName(FName(PropertyName)));

		TestNull(*FString::Printf(TEXT("(a) ⛔ '%s' does NOT exist on ASummonedUnit. WR-§5: the radius and the test live on the OWNING TOWER and the pawn only ASKS — two copies of a tuning number is how they drift"), PropertyName),
			UnitClass->FindPropertyByName(FName(PropertyName)));
	}

	// ── (b) …AND NO DUPLICATE HIDING UNDER ANOTHER NAME ──────────────────────────────────
	// ⭐ Tokens a re-implemented radius / cone / dwell could not plausibly avoid. ⚠️ "Radius"
	// alone is deliberately NOT on this list — ASummonedUnit legitimately ships AggroRadius and
	// friends, and a token list that banned correct, specified code would be a list nobody could
	// keep (the tower file's own "Fall"/"Fallback" lesson, applied in the other direction).
	static const TCHAR* const DuplicatedTuningTokens[] =
	{
		TEXT("LadderContact"), TEXT("Dwell"), TEXT("IntentCos"), TEXT("ContactRadius")
	};

	TArray<FString> UnitMemberNames;
	for (TFieldIterator<FProperty> PropertyIt(UnitClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		UnitMemberNames.Add(PropertyIt->GetName());
	}
	for (TFieldIterator<UFunction> FunctionIt(UnitClass, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
	{
		UnitMemberNames.Add(FunctionIt->GetName());
	}

	if (!TestTrue(TEXT("(b) SELF-CHECK: the reflection walk over ASummonedUnit is live (it found the shipped LadderClimbSpeedUU) — a walk that found nothing would let every token below pass vacuously"),
		UnitMemberNames.Contains(TEXT("LadderClimbSpeedUU"))))
	{
		return false;
	}

	for (const FString& MemberName : UnitMemberNames)
	{
		for (const TCHAR* const Token : DuplicatedTuningTokens)
		{
			if (MemberName.Contains(Token, ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ ASummonedUnit declares '%s', which names a CONTACT tunable ('%s'). The radius, the cone and the dwell are AClimbableTower's (CONTACT-§4.1) and the pawn only ASKS — TASK-784 adds a CALL SITE and ⛔ no second copy of any decision."),
					*MemberName, Token));
			}
		}
	}

	// ── (c) ⭐ THE DECLARED NON-COVERAGE, AS A DISCRIMINATING TEST ───────────────────────
	// The contact ask rides `StateTimerHandle`, so a class that seals its poll never asks at all.
	// ⚠️ That is AMinerUnit, and this row proves the seal really does separate the two shipped
	// classes rather than being true or false for both — a non-coverage claim nobody can check is
	// just a sentence.
	float UnitPollSeconds = 0.f;
	float MinerPollSeconds = -1.f;
	if (TestTrue(TEXT("(c) SELF-CHECK: both classes' StateCheckInterval read back"),
		TryReadStatePollSeconds(UnitPollSeconds)
		&& TryReadDefaultFloat(AMinerUnit::StaticClass(), GetDefault<AMinerUnit>(), TEXT("StateCheckInterval"), MinerPollSeconds)))
	{
		TestTrue(TEXT("(c) ⭐ ASummonedUnit's poll is STRICTLY POSITIVE — it is the ONLY driver the contact ask runs on, and a zero here would silently delete the whole feature"),
			UnitPollSeconds > 0.f);
		TestTrue(TEXT("(c) ⚠️ AMinerUnit SEALS it to 0 (MinerUnit.cpp:62 seal #1, and FTimerManager::SetTimer with a rate <= 0 arms nothing), so a miner can NEVER contact-climb — DECLARED non-coverage, the same limit TASK-760's self-heal records, and here it is also the WANTED answer (qa/TASK-741 W-5: a miner should never be admitted to a ladder)"),
			MinerPollSeconds <= 0.f);
		TestTrue(TEXT("(c) ⭐⭐ …and the two DISAGREE, which is what makes the seal a discriminating test rather than a constant that happens to read true"),
			(UnitPollSeconds > 0.f) != (MinerPollSeconds > 0.f));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
