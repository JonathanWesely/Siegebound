// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Siegebound/Building.h"
#include "Siegebound/ClimbableTower.h"
#include "Siegebound/SiegeNavAreas.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/TeamId.h"
#include "Siegebound/Tower.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE CLIMBABLE WATCH TOWER (TASK-726, TOWER-§) ═══
 *
 *  ⭐⭐ EVERY TEST IN THIS FILE IS ABOUT SOMETHING THAT IS ⛔ NOT THERE. That is
 *  unusual and it is the point: `AClimbableTower`'s whole value is the code it
 *  does NOT contain — no fire loop, no capacity counter, no ranged-only filter,
 *  no occupant bookkeeping, no coupling to the damage rule. An absence has no
 *  behaviour to call, so these tests assert it through REFLECTION and through the
 *  one pure predicate the class exposes.
 *
 *  ⚠️⚠️ THE LESSON THIS FILE WAS WRITTEN AGAINST (and it cost a QA loop the day
 *  before): AN ASSERTION WHOSE TWO SIDES ARE EQUAL BY CONSTRUCTION PROVES
 *  NOTHING. Every claim below therefore ships with a SELF-CHECK that fails if the
 *  instrument has gone blind — the probe names are validated against the class
 *  that DOES have them, the property iterators are validated by counting the
 *  members they DO find, and the height parity is validated by first proving the
 *  multiplier is not a constant. If any of those self-checks ever fires, the test
 *  around it was about to pass vacuously.
 *
 *  ⭐ WHY A NEW FILE (the "extend, never a parallel new frame unless none fits"
 *  law — this is the declared "none fits" case): of the 14 shipped test files
 *  none covers ABuilding, ATower or any structure — they are scoped to
 *  assistant / settings / keyboard / stuck / account / cloud / castle-transform /
 *  deck / warmap / controls-help / high-ground. Same frame as the rest of the
 *  suite: the same simple-automation-test macro, EditorContext | EngineFilter,
 *  "Siegebound.*" names.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero asset loads, ⛔ zero writes.
 *  Class-default objects and the reflection tables only, exactly like
 *  SiegeHighGroundTest.cpp.
 *
 *  ⛔ WHAT THIS FILE DELIBERATELY DOES NOT COVER (SC-§32 — green here is ⛔ NOT
 *  "the tower works"):
 *    • ⛔ THAT THE RAMP IS ACTUALLY WALKABLE. That is Recast's verdict on
 *      SM_WatchTower's geometry (TASK-727) and it is answered by TASK-725's nav
 *      arithmetic plus Jonathan's playtest (TASK-732) — ⛔ never here.
 *    • ⛔ THAT THE ASCENT GATE ACTUALLY STOPS AN ENEMY. Arming the box needs a
 *      world, a spawned tower with an authoritative Team, and two capsules. What
 *      IS asserted here is the RULE the gate is authored from, through the same
 *      single source of truth the shipped code uses.
 *    • ⛔ THE GATE'S LOCAL EXTENTS. AscentGateHalfExtentXY assumes the ramp runs
 *      along the mesh's local X, and SM_WatchTower does not exist yet — that is a
 *      DECLARED unverified input for TASK-728/731, ⛔ not something a headless
 *      test can settle.
 *    • ⛔ THE CARD DATA. The WatchTower row lives in Docs/Data/cards.csv
 *      (TASK-723) and is INERT until /Game/Data/DT_Cards is reimported
 *      (TASK-731's editor step).
 *    • ⛔ NOTHING HERE RUNS THE MODEL. No inference, no Capture(), no
 *      EnsureSnapshot(); 🔒 the one-shot latch is untouched, Zone A is not read,
 *      and ⛔ no token figure appears anywhere in this file (AS-§12g).
 */

namespace SiegeClimbableTowerTestFixture
{
	/**
	 *  ⭐ ATower's fire-loop properties, BY NAME. These are the probes for "this
	 *  class has no fire path": if AClimbableTower ever became an ATower subclass
	 *  (or grew its own turret), these names would resolve on it.
	 *
	 *  ⚠️ A NAME LIST IS A BLIND INSTRUMENT THE MOMENT SOMEBODY RENAMES A FIELD —
	 *  the probes would find nothing on EITHER class and the test would pass while
	 *  proving nothing. Test 2 therefore asserts each name against ATower FIRST.
	 */
	static const TCHAR* const TowerFireLoopPropertyNames[] =
	{
		TEXT("AttackDamage"),
		TEXT("AttackRange"),
		TEXT("AttackCadence"),
		TEXT("AttackAoERadius"),
		TEXT("AttackMinRange"),
		TEXT("AttackChainTargets"),
		TEXT("AttackChainFalloff")
	};

	/**
	 *  Substrings that must ⛔ NEVER appear in a member AClimbableTower declares —
	 *  one token per refused subsystem (TOWER-§4):
	 *    Occupan/Garrison/Capacit/Full → the capacity counter and its full/refuse state
	 *    Ranged                        → the ranged-only climber filter
	 *    Fall/Land                     → an occupant death or catch special case
	 *  ⭐ These are the names such a feature could not plausibly avoid.
	 */
	static const TCHAR* const RefusedSubsystemTokens[] =
	{
		TEXT("Occupan"), TEXT("Garrison"), TEXT("Capacit"), TEXT("Full"),
		TEXT("Ranged"), TEXT("Fall"), TEXT("Land")
	};

	/**
	 *  Substrings that must ⛔ NEVER appear in a member ASummonedUnit declares —
	 *  the HIGH-§3 decoupling, read from the OTHER side of the seam. A
	 *  bIsOnATower flag, an occupancy lookup or a cached platform Z could not
	 *  plausibly avoid all of these.
	 */
	static const TCHAR* const TowerAwarenessTokens[] =
	{
		TEXT("Tower"), TEXT("Climb"), TEXT("Platform"), TEXT("Occupan"), TEXT("Ascen")
	};

	/** Reads a shipped float UPROPERTY off a class default object. Returns false (and writes nothing) if the property is gone — the caller FAILS on that rather than substituting a guess. */
	static bool TryReadDefaultFloat(const UClass* Class, const UObject* Defaults, const TCHAR* PropertyName, float& OutValue)
	{
		if (!Class || !Defaults)
		{
			return false;
		}

		const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Class->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Defaults);
		return true;
	}

	/**
	 *  ⭐ THE ELEVATION STEP, RE-DERIVED FROM ITS DEFINITION RATHER THAN COPIED FROM THE
	 *  HEADER — the same discipline SiegeHighGroundTest.cpp:80-88 already ships, for the same
	 *  reason. `1 ft = 30.48 cm` is the international definition of the foot (exact, since
	 *  1959) and `5` is the number in Jonathan's sentence; their product is the step the code
	 *  must ship. ⛔ Deliberately NOT written as `152.4f`: an expectation transcribed from the
	 *  subject cannot disagree with a wrong subject.
	 *
	 *  ⚠️ AND THE MAINTENANCE REASON — what a SECOND literal actually costs (TASK-730 W-1):
	 *  HeightBonusStepUU is EditDefaultsOnly PRECISELY so Jonathan can retune it. A magic
	 *  152.4 buried in a TOWER test — a file with nothing to do with that tuning — is a booby
	 *  trap for the day he does. HIGH-§1's "⛔ no second literal of it exists anywhere in the
	 *  codebase" law exists for exactly that, and this file was the one place breaking it.
	 */
	constexpr float CentimetresPerFoot = 30.48f;
	constexpr float FeetPerStep = 5.f;
	constexpr float ExpectedStepUU = FeetPerStep * CentimetresPerFoot; // = 152.4 uu

	/** Tolerance for the decimal expectations quoted out of HIGH-§5's table — tight enough that a retuned step blows straight through it, loose enough that float32 rounding never does. */
	constexpr float Tolerance = 1.e-4f;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. ⛔⛔ A SIBLING OF ATower, ⛔ NEVER A SUBCLASS OF IT (TOWER-§5)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerSiblingTest,
	"Siegebound.ClimbableTower.IsADirectABuildingChildAndNeverAnATowerSubclass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerSiblingTest::RunTest(const FString& Parameters)
{
	UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
	UClass* const TowerClass = ATower::StaticClass();
	UClass* const BuildingClass = ABuilding::StaticClass();

	if (!ClimbableTowerClass || !TowerClass || !BuildingClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a StaticClass() returned null — nothing below could mean anything."));
		return false;
	}

	// ── SELF-CHECK: the two classes really are RELATED, under one base ───────────────────
	// Without this, "AClimbableTower is not a child of ATower" would also be true if ATower
	// were an unrelated class in another hierarchy, or if this file were probing a typo'd
	// class that happens to compile. The sibling claim only carries weight once BOTH sides
	// are shown to hang off ABuilding.
	TestTrue(TEXT("SELF-CHECK: ATower is itself an ABuilding — so the two classes share the base this claim is about"),
		TowerClass->IsChildOf(BuildingClass));

	// ── (a) It IS a building — HP, BlockAll collision, nav relevance, the health bar,
	//        the team material, the freeze API and destruction all come from ABuilding,
	//        unchanged (TOWER-§4 "does it block pathing?" ⇒ yes, with zero work).
	TestTrue(TEXT("(a) AClimbableTower IS an ABuilding"),
		ClimbableTowerClass->IsChildOf(BuildingClass));

	// ── (b) ⛔ AND IT IS NOT A TOWER. ATower::OnStatsLoaded binds Damage/Range/Cadence and
	//        arms a repeating fire timer. Inheriting that loop for a card whose row is
	//        0/0/0 is how a "harmless" base class becomes a bug — and the row could be
	//        retuned by anyone at any time, at which point the tower would silently start
	//        shooting. This is THE load-bearing assertion of TOWER-§5.
	TestFalse(TEXT("(b) ⛔ AClimbableTower is NOT an ATower — it must never inherit the auto-fire cadence loop"),
		ClimbableTowerClass->IsChildOf(TowerClass));

	// ── (c) A DIRECT child, with nothing wedged in between. IsChildOf(ABuilding) alone
	//        would still pass if someone inserted an intermediate combat class between the
	//        two; this catches that.
	TestTrue(TEXT("(c) ABuilding is AClimbableTower's IMMEDIATE super — no intermediate class was inserted"),
		ClimbableTowerClass->GetSuperClass() == BuildingClass);

	// ── (d) ⛔ And the reverse direction, so a future refactor cannot satisfy (b) by
	//        making ATower a child of AClimbableTower instead. Four shipped tower cards
	//        depend on ATower and none of them may change.
	TestFalse(TEXT("(d) ⛔ ATower is NOT an AClimbableTower either — the four shipped tower cards are untouched"),
		TowerClass->IsChildOf(ClimbableTowerClass));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⛔ NO AUTO-FIRE. The tower is a PLATFORM, ⛔ not a weapon (TOWER-§3/§4)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerNoAutoFireTest,
	"Siegebound.ClimbableTower.CarriesNoneOfATowersFireLoopStateAndNeverTicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerNoAutoFireTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
	UClass* const TowerClass = ATower::StaticClass();

	if (!ClimbableTowerClass || !TowerClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a StaticClass() returned null."));
		return false;
	}

	for (const TCHAR* const PropertyName : TowerFireLoopPropertyNames)
	{
		// ── SELF-CHECK FIRST, ALWAYS ────────────────────────────────────────────────────
		// ⭐ SHIP-§9: the instrument is validated against the failure it exists to detect.
		// If ATower renames AttackCadence tomorrow, this probe would find nothing on either
		// class and the absence assertion below would pass while proving NOTHING. This line
		// converts that silent rot into a loud failure that names the stale probe.
		TestTrue(*FString::Printf(TEXT("SELF-CHECK: '%s' is still a real ATower property — the probe is not stale"), PropertyName),
			TowerClass->FindPropertyByName(FName(PropertyName)) != nullptr);

		// FindPropertyByName walks supers too, so a null here proves the name is absent from
		// AClimbableTower AND from ABuilding AND from every engine base — i.e. there is no
		// fire-loop state anywhere in this actor's layout.
		TestNull(*FString::Printf(TEXT("⛔ AClimbableTower carries NO '%s' — it has no fire path to arm"), PropertyName),
			ClimbableTowerClass->FindPropertyByName(FName(PropertyName)));
	}

	// ── The family's tickless discipline, read off the shipped CDO ──────────────────────
	// ABuilding sets bCanEverTick false so the whole building family stays tickless (ATower
	// fires on a timer, never on tick). A per-frame scan sneaking into this class would be
	// exactly the auto-fire the card refuses, and it would show up here first.
	const AClimbableTower* const Defaults = GetDefault<AClimbableTower>();
	if (!Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<AClimbableTower>() returned null."));
		return false;
	}

	TestFalse(TEXT("⛔ The class default object never ticks — no per-frame scan, no acquisition loop"),
		Defaults->PrimaryActorTick.bCanEverTick);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. T-3 — ⛔ ENEMIES REFUSED · ✅ ANY OWN-TEAM UNIT ADMITTED (TOWER-§4)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerTeamGateTest,
	"Siegebound.ClimbableTower.RefusesEnemyClimbersAndAdmitsEveryOwnTeamUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerTeamGateTest::RunTest(const FString& Parameters)
{
	// ── SELF-CHECK: the two team channels are DISTINCT ──────────────────────────────────
	// Every claim below is a statement about one channel differing from another. If the two
	// ini-backed aliases ever collapsed onto the same GameTraceChannel, CanTeamAscend would
	// return false for EVERYBODY (the tower would admit nobody, including its owner) and the
	// mirror-image assertions would still look self-consistent. This catches that first.
	TestTrue(TEXT("SELF-CHECK: ECC_SiegeTeamBlue and ECC_SiegeTeamRed are DISTINCT channels"),
		ECC_SiegeTeamBlue != ECC_SiegeTeamRed);

	// ── SELF-CHECK: the gate blocks the ENEMY channel, and mirrors by team ──────────────
	// The predicate below is derived from AscentBlockedChannel, and so is the one ECR_Block
	// the shipped ConfigureAscentGate authors — one source of truth. Pinning what that
	// source RETURNS is what stops the pair from being "consistent but backwards": a gate
	// that blocked its OWN channel would keep every assertion in this test mutually
	// agreeable while locking the owner out and letting the enemy walk up.
	TestTrue(TEXT("SELF-CHECK: a BLUE tower blocks the RED combatant channel"),
		AClimbableTower::AscentBlockedChannel(ETeamId::Blue) == ECC_SiegeTeamRed);
	TestTrue(TEXT("SELF-CHECK: a RED tower blocks the BLUE combatant channel"),
		AClimbableTower::AscentBlockedChannel(ETeamId::Red) == ECC_SiegeTeamBlue);

	// ── (a) ⛔ ENEMIES MAY NOT CLIMB — T-3, both directions so no hardcoded team branch
	//        can hide behind a single-team test.
	TestFalse(TEXT("(a) ⛔ A RED unit may NOT ascend a BLUE tower"),
		AClimbableTower::CanTeamAscend(ETeamId::Blue, ETeamId::Red));
	TestFalse(TEXT("(a) ⛔ A BLUE unit may NOT ascend a RED tower"),
		AClimbableTower::CanTeamAscend(ETeamId::Red, ETeamId::Blue));

	// ── (b) ✅ ANY OWN-TEAM UNIT MAY — a PERMISSION, not an exclusion. The signature has
	//        no CardID, no unit-type and no bRanged term, so there is no ranged-only filter
	//        to get wrong; a melee unit is admitted and simply gains nothing on top
	//        (HIGH-§4), which is how it self-selects away without ever becoming a stuck
	//        unit pathing toward a tower it may not enter.
	TestTrue(TEXT("(b) ✅ A BLUE unit — of ANY type, ranged or melee — may ascend a BLUE tower"),
		AClimbableTower::CanTeamAscend(ETeamId::Blue, ETeamId::Blue));
	TestTrue(TEXT("(b) ✅ A RED unit — of ANY type — may ascend a RED tower"),
		AClimbableTower::CanTeamAscend(ETeamId::Red, ETeamId::Red));

	// ── (c) ⛔ NO CAPACITY LIMIT — the verdict is stable no matter how many are already
	//        up there, because occupancy is not an input. Physical space is the cap: a
	//        600×600 platform at AgentRadius 34. Repeating the call is the closest a pure
	//        predicate can come to "the hundredth climber is treated like the first", and
	//        the structural half of this claim is test 4.
	for (int32 ClimberIndex = 0; ClimberIndex < 100; ++ClimberIndex)
	{
		if (!AClimbableTower::CanTeamAscend(ETeamId::Blue, ETeamId::Blue))
		{
			AddError(*FString::Printf(TEXT("(c) ⛔ Own-team climber #%d was REFUSED — a capacity limit has appeared where the law forbids one."), ClimberIndex));
			break;
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ⛔ THE REFUSED SUBSYSTEMS ARE ABSENT FROM THE CLASS ITSELF (TOWER-§4)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerNoRefusedSubsystemsTest,
	"Siegebound.ClimbableTower.DeclaresNoCapacityCounterNoRangedFilterAndNoOccupantBookkeeping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerNoRefusedSubsystemsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
	if (!ClimbableTowerClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: AClimbableTower::StaticClass() returned null."));
		return false;
	}

	// Collect every member AClimbableTower DECLARES — ExcludeSuper on purpose: ABuilding's
	// and the engine's members are not this class's claim to make, and including them would
	// make the scan noisy enough to need exceptions, which is how a scan stops meaning
	// anything.
	TArray<FString> DeclaredMemberNames;
	for (TFieldIterator<FProperty> PropertyIt(ClimbableTowerClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		DeclaredMemberNames.Add(PropertyIt->GetName());
	}
	for (TFieldIterator<UFunction> FunctionIt(ClimbableTowerClass, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
	{
		DeclaredMemberNames.Add(FunctionIt->GetName());
	}

	// ── SELF-CHECK: THE ITERATOR ACTUALLY WALKED SOMETHING ──────────────────────────────
	// ⚠️⚠️ THIS LINE IS THE WHOLE REASON THIS TEST IS NOT A LIE. A scan over an empty list
	// finds no banned token and passes — brilliantly, permanently and vacuously. Requiring
	// the members this class is KNOWN to declare (the platform height plus the three gate
	// members) proves the reflection walk is live before a single absence is claimed.
	TestTrue(TEXT("SELF-CHECK: the reflection walk found the members this class is known to declare"),
		DeclaredMemberNames.Num() >= 4);
	TestTrue(TEXT("SELF-CHECK: …including PlatformHeightUU by name"),
		DeclaredMemberNames.Contains(TEXT("PlatformHeightUU")));
	TestTrue(TEXT("SELF-CHECK: …and the ascent gate volume"),
		DeclaredMemberNames.Contains(TEXT("AscentGateVolume")));

	// ── The absence claims ──────────────────────────────────────────────────────────────
	// ⭐ An occupancy subsystem (a counter, a full/refuse state, its UI and its replication),
	// a ranged-only climber filter, or an occupant death/catch special case cannot be built
	// without a member named after itself. Each is DELETED by choosing the ramp, and this is
	// what keeps them deleted.
	for (const FString& MemberName : DeclaredMemberNames)
	{
		for (const TCHAR* const Token : RefusedSubsystemTokens)
		{
			if (MemberName.Contains(Token, ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ AClimbableTower declares '%s', which names the REFUSED '%s' subsystem (TOWER-§4). ")
					TEXT("Capacity, a ranged-only filter and occupant bookkeeping were deleted deliberately — occupants FALL and survive, and there is no fall damage in this project."),
					*MemberName, Token));
			}
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. ⭐⭐ HIGH-§3 PARITY — A HILL AND A TOWER AT THE SAME HEIGHT DEAL THE SAME
//     DAMAGE, AND THE DAMAGE RULE NEVER LEARNS THE TOWER EXISTS
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerHeightParityTest,
	"Siegebound.ClimbableTower.PlatformAndAnEqualHillYieldTheIdenticalDamageMultiplier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerHeightParityTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	const AClimbableTower* const TowerDefaults = GetDefault<AClimbableTower>();
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!TowerDefaults || !UnitDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: a class default object returned null."));
		return false;
	}

	// ── PRECONDITION: read the SHIPPED HIGH-§ tuning, never a transcribed copy ──────────
	// The expectations below are only meaningful at 152.4 uu / +10%. Reading them off the
	// CDO (rather than re-typing them) means a retune fails HERE, loudly, instead of quietly
	// changing what this file claims to prove. A missing property is a hard error, ⛔ never a
	// substituted guess.
	float StepUU = 0.f;
	float BonusPerStep = 0.f;
	if (!TryReadDefaultFloat(ASummonedUnit::StaticClass(), UnitDefaults, TEXT("HeightBonusStepUU"), StepUU) ||
		!TryReadDefaultFloat(ASummonedUnit::StaticClass(), UnitDefaults, TEXT("HeightBonusPerStep"), BonusPerStep))
	{
		AddError(TEXT("SELF-CHECK FAILED: HeightBonusStepUU / HeightBonusPerStep did not resolve on ASummonedUnit — the HIGH-§ tuning this test rides has been renamed or removed."));
		return false;
	}

	// ⭐ The expectation is DERIVED (5 ft × 30.48 cm/ft), ⛔ never re-typed as 152.4f — see the
	// fixture's note. Tolerance, not Exact: 5.f * 30.48f and 152.4f differ by ~4e-6 in float32,
	// which is four orders of magnitude inside the band, while every wrong step in the trap
	// list (150 / 152 / 5 / 500) blows straight through it.
	TestEqual(TEXT("PRECONDITION: the shipped elevation step is 5 ft × 30.48 cm/ft (= 152.4 uu)"), StepUU, ExpectedStepUU, Tolerance);
	TestEqual(TEXT("PRECONDITION: the shipped bonus is +10% per step"), BonusPerStep, 0.10f, Tolerance);

	// ── THE TWO HEIGHTS, FROM TWO GENUINELY DIFFERENT SOURCES ───────────────────────────
	// Left:  the TOWER's own shipped tunable, read through the class under test.
	// Right: a HILL crest, typed here as the plain 1,200-uu row of HIGH-§5's table.
	// ⛔ The right-hand value is deliberately NOT derived from the left: if PlatformHeightUU
	// is ever retuned, these two stop matching and this test says so — which is the point,
	// because 1,200 uu is the number PlatformHeightUU's comment and 🧑 row T-5 promise
	// Jonathan buys +78.7%.
	const float TargetZ = 0.f;
	const float TowerPlatformRiseUU = TowerDefaults->GetPlatformHeightUU();
	constexpr float HillCrestRiseUU = 1200.f;

	// ⚠️⚠️ AND THE SUNKEN BASIN THE HILL SITS IN IS LOAD-BEARING, ⛔ NOT DECORATION
	// (TASK-730 W-2 — this is the repair). The tower pair is (1200, 0). If the hill pair were
	// ALSO (1200, 0) the two sides would be the SAME CALL, the parity assertion below would be
	// equal BY CONSTRUCTION, and it could only fail in the case the height equality one line
	// above already catches — a restatement of "PlatformHeightUU is still 1,200" wearing the
	// word "parity". ⭐ Siting the hill 500 uu BELOW the tower's world instead makes the two
	// calls genuinely different — (1200, 0) against (700, −500) — so the assertion now carries
	// the claim it always advertised: the rule is a function of the DIFFERENCE and of nothing
	// else. It fails against any implementation that read absolute world Z (which would score
	// these ×1.787 vs ×1.459), and against any tower special case keyed on a world height.
	// ⛔ This is the same defect class as an assertion whose two sides are literally identical:
	// a claim that cannot fail occupies the space where a real one would have gone.
	constexpr float BasinFloorZ = -500.f;

	// ── SELF-CHECK: THE MULTIPLIER IS NOT A CONSTANT ────────────────────────────────────
	// ⚠️⚠️ WITHOUT THIS LINE THE PARITY CLAIM BELOW IS WORTHLESS. A function that returned
	// 1.0 for every input — or that had been accidentally short-circuited — would satisfy
	// "tower equals hill" perfectly. Prove the instrument responds to height before using it
	// to compare two heights.
	const float HalfHeightMultiplier = ASummonedUnit::HeightAdvantageMultiplier(TargetZ + 600.f, TargetZ, StepUU, BonusPerStep);
	const float TowerMultiplier = ASummonedUnit::HeightAdvantageMultiplier(TargetZ + TowerPlatformRiseUU, TargetZ, StepUU, BonusPerStep);
	TestTrue(TEXT("SELF-CHECK: the multiplier RISES with height — the instrument is live, not a constant"),
		TowerMultiplier > HalfHeightMultiplier + Tolerance);

	// ── (a) THE PARITY ITSELF (HIGH-§3) ─────────────────────────────────────────────────
	// The rule reads height ABOVE THE TARGET and nothing else. There is no bIsOnATower flag,
	// no tower query and no occupancy lookup, so a unit on the tower's 1,200-uu platform and a
	// unit on a 1,200-uu hill crest are — to the damage path — the same unit, EVEN THOUGH the
	// two stand 500 uu apart in world Z and shoot targets 500 uu apart in world Z.
	//   TOWER: target on the flat at 0,          attacker on the platform at  1,200.
	//   HILL:  target on a basin floor at −500,  attacker on the crest    at    700.
	const float HillMultiplier = ASummonedUnit::HeightAdvantageMultiplier(BasinFloorZ + HillCrestRiseUU, BasinFloorZ, StepUU, BonusPerStep);

	TestEqual(TEXT("(a) The tower's platform rise and the hill's crest rise are the same height"),
		TowerPlatformRiseUU, HillCrestRiseUU, Tolerance);
	TestEqual(TEXT("(a) ⭐ …and therefore the IDENTICAL damage multiplier, though the hill sits 500 uu lower in the world — a hill and a tower can never disagree"),
		TowerMultiplier, HillMultiplier, Tolerance);

	// ── (b) THE PINNED CONSEQUENCE — 🧑 row T-5's +78.7% ─────────────────────────────────
	// 1 + 0.10 × (1200 / 152.4) = 1 + 0.10 × 7.874015… = 1.787401…
	// This is the number written into PlatformHeightUU's comment and quoted to Jonathan. It
	// is pinned as a LITERAL, not recomputed from the tuning above, so re-deriving the
	// formula in this file could never make the claim agree with itself.
	TestEqual(TEXT("(b) ⭐ A 1,200-uu platform buys ×1.7874 (+78.7%) against a target on the flat below"),
		TowerMultiplier, 1.78740157f, Tolerance);

	// ── (c) THE DIRECTIONAL COMPANION — parity is not an artifact of a flat function ─────
	// Drop the crest by EXACTLY one step and the multiplier must drop by EXACTLY one bonus.
	// If (a)'s equality came from a degenerate rule, this fails.
	// ⚠️ This crest is sited in the TOWER's world (target on the flat at 0), ⛔ not in (a)'s
	// basin — it is compared against TowerMultiplier, so it must share TowerMultiplier's
	// target. (a) has just proven the two rises are equal, which is what licenses that.
	const float OneStepLowerMultiplier = ASummonedUnit::HeightAdvantageMultiplier(TargetZ + HillCrestRiseUU - StepUU, TargetZ, StepUU, BonusPerStep);
	TestEqual(TEXT("(c) A hill exactly one 152.4-uu step lower yields exactly 0.10 less — the equality above is real, not degenerate"),
		TowerMultiplier - OneStepLowerMultiplier, BonusPerStep, Tolerance);

	// ── (d) ⛔ AND THE TOWER IS INVISIBLE TO THE RULE, FROM THE RULE'S OWN SIDE ──────────
	// The parity above is a property of the arithmetic; THIS is the structural guarantee
	// that keeps it true. A bIsOnATower flag, a cached platform Z or an occupancy lookup
	// could not plausibly avoid every one of these tokens. ExcludeSuper: only what
	// ASummonedUnit itself declares — which is exactly the surface TASK-724 touched.
	TArray<FString> UnitDeclaredMemberNames;
	for (TFieldIterator<FProperty> PropertyIt(ASummonedUnit::StaticClass(), EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		UnitDeclaredMemberNames.Add(PropertyIt->GetName());
	}
	for (TFieldIterator<UFunction> FunctionIt(ASummonedUnit::StaticClass(), EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
	{
		UnitDeclaredMemberNames.Add(FunctionIt->GetName());
	}

	// SELF-CHECK: the walk is live (same trap as test 4 — an empty list passes any scan).
	TestTrue(TEXT("SELF-CHECK: the ASummonedUnit reflection walk found HeightBonusStepUU"),
		UnitDeclaredMemberNames.Contains(TEXT("HeightBonusStepUU")));

	for (const FString& MemberName : UnitDeclaredMemberNames)
	{
		for (const TCHAR* const Token : TowerAwarenessTokens)
		{
			if (MemberName.Contains(Token, ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ ASummonedUnit declares '%s', which names the tower ('%s'). HIGH-§3 forbids it: the damage rule reads the world's real height and NOTHING else. ")
					TEXT("A tower special case makes one rule into two, and guarantees a hill and a tower eventually disagree."),
					*MemberName, Token));
			}
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
