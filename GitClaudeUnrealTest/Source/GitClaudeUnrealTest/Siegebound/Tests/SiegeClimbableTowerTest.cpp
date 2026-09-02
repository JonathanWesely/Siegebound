// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "AI/Navigation/NavLinkDefinition.h"
#include "Containers/UnrealString.h"
#include "NavLinkCustomComponent.h"
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
 *  ═══ AUTOMATION TESTS for THE CLIMBABLE WATCH TOWER (TASK-726 → TASK-734,
 *      TOWER-§ / TOWER-§8 / TOWER-§10) ═══
 *
 *  ⭐⭐ MOST OF THIS FILE IS ABOUT SOMETHING THAT IS ⛔ NOT THERE. That is unusual
 *  and it is the point: `AClimbableTower`'s value is largely the code it does NOT
 *  contain — no fire loop, no capacity counter, no ranged-only filter, no
 *  occupancy bookkeeping, no coupling to the damage rule. An absence has no
 *  behaviour to call, so those claims are asserted through REFLECTION and through
 *  the pure predicates the class exposes.
 *
 *  ⚠️⚠️ THE LESSON THIS FILE WAS WRITTEN AGAINST, AND IT HAS NOW COST QA LOOPS
 *  TWICE: AN ASSERTION WHOSE TWO SIDES ARE EQUAL BY CONSTRUCTION PROVES NOTHING.
 *  Every claim below therefore ships with a SELF-CHECK that fails if the
 *  instrument has gone blind — probe names are validated against a class that DOES
 *  have them, property iterators are validated by counting members they DO find,
 *  the height parity is validated by first proving the multiplier is not a
 *  constant, and `GetLinkData` is validated by proving it overwrites a sentinel.
 *  If any self-check ever fires, the test around it was about to pass vacuously.
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ WHAT CHANGED ON 2026-09-01 (TASK-734), AND WHICH ASSERTIONS **DIED** —
 *  STATED HERE RATHER THAN DELETED IN SILENCE (`SC-§27` R-6, and `TOWER-§8` (9)
 *  asked for exactly this list):
 *
 *   ⛔ DIED — test 4's self-check `DeclaredMemberNames.Contains("AscentGateVolume")`.
 *      The physical elevation shell and its three tuning properties were REMOVED
 *      by `TOWER-§8.6`; a self-check that requires a deleted member to exist is a
 *      guaranteed red. ✅ REPLACED, ⛔ not dropped: the same self-check now names
 *      `LadderLink` (the member that took its place), and the *stronger* half of
 *      the old claim is promoted into its own test — **test 7 asserts all four
 *      removed members are GONE by name**, which the old file could not say at all.
 *
 *   ⛔ NARROWED — test 5(d)'s `TowerAwarenessTokens` no longer contains "Climb".
 *      ⚠️ This is ⛔ NOT a weakening of `HIGH-§3`; it is the token list catching up
 *      with `TOWER-§8.4(B)`, which DECLARES exactly one new coupling between a
 *      tower and a unit — **movement** — while leaving the **damage** seam sealed.
 *      `ASummonedUnit` now legitimately declares `BeginLadderClimb`,
 *      `AbortLadderClimb`, `IsClimbing` and `OnLadderClimbEnded` (TASK-738), and a
 *      scan that banned "Climb" would have failed on correct, specified code.
 *      ✅ The four tokens that still matter — Tower / Platform / Occupan / Ascen —
 *      are UNTOUCHED, and the removal is made honest by a new self-check that
 *      requires `IsClimbing` to be FOUND: the walk is proven live on the very
 *      token that left the list.
 *
 *   ⭐ NOTHING ELSE CHANGED. Tests 1, 2, 3 and 5(a)–(c) are byte-identical claims;
 *      only their comments were re-pointed at the ladder.
 *  ─────────────────────────────────────────────────────────────────────────────
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero asset loads, ⛔ zero writes.
 *  Class-default objects and the reflection tables only, exactly like
 *  SiegeHighGroundTest.cpp.
 *
 *  ⛔ WHAT THIS FILE DELIBERATELY DOES NOT COVER (SC-§32 — green here is ⛔ NOT
 *  "the ladder works"):
 *    • ⛔ THAT RECAST ACTUALLY CONNECTS THE LINK. Whether the foot lands on ground
 *      navmesh and the top on deck navmesh is Recast's verdict on SM_WatchTower
 *      (TASK-737) plus the arithmetic in `TOWER-§8.3` — and it is settled by
 *      TASK-742's integration and Jonathan's playtest, ⛔ never here. ⚠️ This is
 *      the single highest-risk unverified claim in the whole batch.
 *    • ⛔ THAT A UNIT ACTUALLY CLIMBS. The traversal is `ASummonedUnit`'s
 *      (TASK-738) and its own test file covers it.
 *    • ⛔ THAT `EndPlay` ABORTS AN IN-FLIGHT CLIMB (`TOWER-§10` L-5). Proving it
 *      needs a world, a spawned tower, a spawned unit and a live climb — ⛔ none of
 *      which a headless reflection test may create, and a test that fabricates
 *      world-less actors to fake it would be more likely to crash the suite than to
 *      catch the bug. ⭐ WHAT **IS** ASSERTED IN ITS PLACE (test 10): the
 *      completion lane that `EndPlay` drives — `HandleLadderClimbEnded` as a
 *      2-parameter UFUNCTION — EXISTS, because without it `AddUniqueDynamic` cannot
 *      bind and every climb would leave the ladder occupied forever.
 *    • ⛔ THE PATHFINDING LAYER'S REFUSAL. `IsLinkPathfindingAllowed` needs a real
 *      controller possessing a real pawn. Test 9 covers the half that carries the
 *      safety argument (it fails OPEN) and test 3 covers the rule it delegates to.
 *    • ⛔ THE CARD DATA. The WatchTower row lives in Docs/Data/cards.csv and is
 *      INERT until /Game/Data/DT_Cards is reimported (an editor step).
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
	 *
	 *  ⚠️ AND THE LIST HAS ALREADY EXTRACTED A PRICE, WHICH IS WORTH RECORDING
	 *  BECAUSE IT IS THE COST OF SCAN-BASED TESTS: the ladder's socket-absent
	 *  constants are named `Ladder*DefaultRelative` and ⛔ NOT `Ladder*Fallback`,
	 *  because "Fallback" contains "Fall". ⚖️ The alternative — carving an exception
	 *  into the scan — is how a scan stops meaning anything, so the NAME moved.
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
	 *
	 *  ⚠️⚠️ "Climb" WAS REMOVED FROM THIS LIST ON 2026-09-01 AND THAT REMOVAL IS
	 *  SPECIFIED, ⛔ NOT A CONCESSION. `TOWER-§8.4(B)` declares exactly ONE new
	 *  coupling — MOVEMENT — and names the four members that carry it
	 *  (`BeginLadderClimb`, `AbortLadderClimb`, `IsClimbing`, `OnLadderClimbEnded`).
	 *  ⭐ THE HALF THAT MATTERED IS UNTOUCHED: the tokens below still forbid a
	 *  `bIsOnATower`, a platform-Z cache, an occupancy lookup or an ascent flag, so
	 *  a unit on a HILL and a unit on a TOWER at the same Z still cannot be told
	 *  apart by the damage rule. ⚖️ A movement API is not a damage special case, and
	 *  a list that could not tell them apart would have banned the specification.
	 */
	static const TCHAR* const TowerAwarenessTokens[] =
	{
		TEXT("Tower"), TEXT("Platform"), TEXT("Occupan"), TEXT("Ascen")
	};

	/**
	 *  ⛔ THE FOUR MEMBERS `TOWER-§8.6` REMOVED. The elevation shell existed to stop
	 *  an enemy partway up a 2,078-uu ramp whose mouth nobody could locate in
	 *  mesh-local space; a ladder has ONE discrete entry point, so the gate became
	 *  one predicate at that point and the shell — along with its known-open
	 *  pivot-vs-mesh tuning item — DISSOLVED rather than being fixed.
	 *  ⚠️ Re-adding any of these is a regression, ⛔ not a restoration.
	 */
	static const TCHAR* const RemovedAscentShellMemberNames[] =
	{
		TEXT("AscentGateVolume"),
		TEXT("AscentGateFloorUU"),
		TEXT("AscentGateHeadroomUU"),
		TEXT("AscentGateHalfExtentXY")
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

	/** Every reflected member AClimbableTower DECLARES ITSELF (ExcludeSuper) — properties then functions. */
	static void CollectDeclaredMemberNames(const UClass* Class, TArray<FString>& OutNames)
	{
		if (!Class)
		{
			return;
		}
		for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			OutNames.Add(PropertyIt->GetName());
		}
		for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
		{
			OutNames.Add(FunctionIt->GetName());
		}
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

	/**
	 *  ⛔ THE `TOWER-§8.3` PINNED GEOMETRY, TYPED FROM THE **LAW** AND ⛔ NEVER READ
	 *  BACK OFF THE CLASS UNDER TEST. That direction is the whole point: the
	 *  expectation is what the manager pinned and the artist is building to, so a
	 *  typo in `AClimbableTower::LadderFootDefaultRelative` fails HERE instead of
	 *  shipping a link whose foot sits inside the tower's own eroded nav carve.
	 */
	const FVector PinnedLadderFootRelative(-450.f, 0.f, 0.f);
	const FVector PinnedLadderTopRelative(-150.f, 0.f, 1200.f);

	/** `TOWER-§8.3`'s two DERIVED figures for the same segment. ⭐ Recomputed from the shipped pair, so they catch a typo the raw-coordinate pin could not explain. */
	constexpr float PinnedClimbLineLengthUU = 1236.9f;
	constexpr float PinnedClimbLeanDegrees = 76.0f;

	/** The law quotes both figures rounded to one decimal, so the band has to admit the rounding (~0.04 on each) and nothing else. */
	constexpr float GeometryTolerance = 0.1f;
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
	//
	// ⭐⭐ AND THIS SURVIVED THE LADDER, WHICH WAS NOT FREE (TOWER-§8 (4)): a traversal has a
	// beginning and an END, and the obvious way to notice an end is to poll for it. Both
	// edges are PUSHED to the tower instead — entry through the link's FOnMoveReachedLink,
	// completion through ASummonedUnit::OnLadderClimbEnded — so there is still nothing to
	// poll. Test 10 asserts the completion lane that makes this possible actually exists.
	const AClimbableTower* const Defaults = GetDefault<AClimbableTower>();
	if (!Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<AClimbableTower>() returned null."));
		return false;
	}

	TestFalse(TEXT("⛔ The class default object never ticks — no per-frame scan, no acquisition loop, and no climb poll"),
		Defaults->PrimaryActorTick.bCanEverTick);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. T-3 — ⛔ ENEMIES REFUSED · ✅ ANY OWN-TEAM UNIT ADMITTED (TOWER-§4/§8.6)
//     ⭐ AS OF TASK-734 THIS PREDICATE IS THE **LIVE RULE**, not a description
//        of a volume: the ladder's entry point calls it (TOWER-§8.6).
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

	// ── SELF-CHECK: the rule names the ENEMY channel, and mirrors by team ───────────────
	// Pinning what AscentBlockedChannel RETURNS is what stops the pair below from being
	// "consistent but backwards": a rule keyed on its OWN channel would keep every assertion
	// in this test mutually agreeable while locking the owner out and letting the enemy up.
	TestTrue(TEXT("SELF-CHECK: a BLUE tower treats the RED combatant channel as the enemy's"),
		AClimbableTower::AscentBlockedChannel(ETeamId::Blue) == ECC_SiegeTeamRed);
	TestTrue(TEXT("SELF-CHECK: a RED tower treats the BLUE combatant channel as the enemy's"),
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
	//        up there, because occupancy is not an input to THIS predicate. Physical space
	//        is the cap: a 600×600 platform at AgentRadius 34.
	//        ⚠️ ⛔ Do NOT confuse this with `TOWER-§10` L-1: one climber at a time on the
	//        LADDER is a different claim about a different resource, it lives in
	//        EvaluateLadderEntry, and test 8 asserts it there. Deck occupancy is still
	//        uncounted and always will be.
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
	CollectDeclaredMemberNames(ClimbableTowerClass, DeclaredMemberNames);

	// ── SELF-CHECK: THE ITERATOR ACTUALLY WALKED SOMETHING ──────────────────────────────
	// ⚠️⚠️ THIS IS THE WHOLE REASON THIS TEST IS NOT A LIE. A scan over an empty list finds
	// no banned token and passes — brilliantly, permanently and vacuously. Requiring the
	// members this class is KNOWN to declare proves the reflection walk is live before a
	// single absence is claimed.
	// ⚠️ The third line USED to require "AscentGateVolume". That member was REMOVED by
	// TOWER-§8.6 and the requirement moved to its successor; the removal itself is now
	// asserted, far more strongly, by test 7.
	TestTrue(TEXT("SELF-CHECK: the reflection walk found the members this class is known to declare"),
		DeclaredMemberNames.Num() >= 4);
	TestTrue(TEXT("SELF-CHECK: …including PlatformHeightUU by name"),
		DeclaredMemberNames.Contains(TEXT("PlatformHeightUU")));
	TestTrue(TEXT("SELF-CHECK: …and the ladder's smart link (the member that replaced the removed ascent-gate shell)"),
		DeclaredMemberNames.Contains(TEXT("LadderLink")));

	// ── The absence claims ──────────────────────────────────────────────────────────────
	// ⭐ An occupancy subsystem (a counter, a full/refuse state, its UI and its replication),
	// a ranged-only climber filter, or an occupant death/catch special case cannot be built
	// without a member named after itself.
	//
	// ⚠️ AND THE LADDER DID **NOT** SMUGGLE ONE IN, WHICH IS EXACTLY WHAT THIS SCAN IS FOR:
	// `ActiveClimber` is ONE handle to the ONE unit on the ladder, required by TOWER-§10 L-1
	// and L-5 — it counts nobody, refuses nobody on the DECK, and names none of these
	// tokens. A real capacity counter could not have avoided them.
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

	// ── (d) ⛔ AND THE TOWER IS INVISIBLE TO THE **DAMAGE** RULE, FROM THE RULE'S OWN SIDE ─
	// The parity above is a property of the arithmetic; THIS is the structural guarantee
	// that keeps it true. A bIsOnATower flag, a cached platform Z or an occupancy lookup
	// could not plausibly avoid every one of these tokens. ExcludeSuper: only what
	// ASummonedUnit itself declares.
	TArray<FString> UnitDeclaredMemberNames;
	CollectDeclaredMemberNames(ASummonedUnit::StaticClass(), UnitDeclaredMemberNames);

	// SELF-CHECK: the walk is live (same trap as test 4 — an empty list passes any scan).
	TestTrue(TEXT("SELF-CHECK: the ASummonedUnit reflection walk found HeightBonusStepUU"),
		UnitDeclaredMemberNames.Contains(TEXT("HeightBonusStepUU")));

	// ⭐⭐ SELF-CHECK ON THE TOKEN THAT **LEFT** THE LIST, WHICH IS WHAT MAKES ITS REMOVAL
	// HONEST RATHER THAN CONVENIENT. "Climb" is no longer banned because TOWER-§8.4(B)
	// SPECIFIES a movement API on ASummonedUnit — so this requires that API to actually be
	// there. If TASK-738's `IsClimbing` were renamed or dropped, this fires and says so,
	// instead of the scan quietly narrowing for no reason anybody can reconstruct.
	TestTrue(TEXT("SELF-CHECK: ASummonedUnit really does declare the TOWER-§8.4(B) climb API ('IsClimbing') — which is WHY 'Climb' left the banned-token list"),
		UnitDeclaredMemberNames.Contains(TEXT("IsClimbing")));

	for (const FString& MemberName : UnitDeclaredMemberNames)
	{
		for (const TCHAR* const Token : TowerAwarenessTokens)
		{
			if (MemberName.Contains(Token, ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ ASummonedUnit declares '%s', which names the tower ('%s'). HIGH-§3 forbids it: the DAMAGE rule reads the world's real height and NOTHING else. ")
					TEXT("A tower special case makes one rule into two, and guarantees a hill and a tower eventually disagree. ")
					TEXT("(TOWER-§8.4(B) permits a MOVEMENT API — Begin/Abort/IsClimbing/OnLadderClimbEnded — and nothing else.)"),
					*MemberName, Token));
			}
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⭐⭐ THE LADDER LINK — A **SMART**, **BothWays** OFF-MESH CONNECTION ON THE
//     `TOWER-§8.3` PINNED LINE (TOWER-§8.1 M-1/M-3, §8.3, §8.7)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerLadderLinkTest,
	"Siegebound.ClimbableTower.LadderLinkIsABothWaysSmartLinkOnThePinnedClimbLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerLadderLinkTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
	const AClimbableTower* const Defaults = GetDefault<AClimbableTower>();
	if (!ClimbableTowerClass || !Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: AClimbableTower's class or CDO returned null."));
		return false;
	}

	// ── (a) ⛔ A **SMART** LINK, ⛔ NEVER A SIMPLE ONE — AND THIS IS THE MEASURED HALF ───
	// TOWER-§8.1 M-3: UPathFollowingComponent::SetMoveSegment calls StartUsingCustomLink
	// ONLY when PathPt0.CustomNavLinkId != FNavLinkId::Invalid, and a simple PointLinks entry
	// carries no such id. A simple link would therefore path-plan beautifully, hand the unit
	// ordinary steering, and ConstrainInputAcceleration would delete the vertical component
	// of that steering every frame (M-4) — a tower that looks wired up and moves nobody.
	// Read through the PROPERTY's declared class, so swapping the member's type is what
	// fails, not merely reassigning it.
	const FObjectPropertyBase* const LinkProperty =
		CastField<FObjectPropertyBase>(ClimbableTowerClass->FindPropertyByName(TEXT("LadderLink")));

	if (!LinkProperty || !LinkProperty->PropertyClass)
	{
		AddError(TEXT("⛔ AClimbableTower declares no object property named 'LadderLink' — the deck is an unreachable island and the 30-gold card does nothing (TOWER-§8.1 M-2)."));
		return false;
	}

	TestTrue(TEXT("(a) ⭐ 'LadderLink' is a UNavLinkCustomComponent — a SMART link, which is the only kind path following will drive (M-3)"),
		LinkProperty->PropertyClass->IsChildOf(UNavLinkCustomComponent::StaticClass()));

	const UClimbableTowerLadderLink* const Link = Defaults->GetLadderLink();
	if (!Link)
	{
		AddError(TEXT("⛔ The class default object carries no LadderLink subobject — CreateDefaultSubobject was removed or renamed."));
		return false;
	}

	// ── SELF-CHECK: GetLinkData ACTUALLY WRITES ITS OUT-PARAMETERS ──────────────────────
	// ⚠️⚠️ WITHOUT THIS, EVERY ASSERTION BELOW COULD BE READING THE SENTINELS THIS TEST
	// ITSELF WROTE. Seed all three out-params with values the shipped link cannot hold — a
	// direction that is not BothWays, and two points far outside any tower — then require
	// all three to have been overwritten. A GetLinkData that stopped writing (a refactor, a
	// wrong override, a null impl) would otherwise let this whole test pass on its own
	// scratch memory.
	const FVector LeftSentinel(9999.f, 9999.f, 9999.f);
	const FVector RightSentinel(-9999.f, -9999.f, -9999.f);

	FVector LeftPoint = LeftSentinel;
	FVector RightPoint = RightSentinel;
	ENavLinkDirection::Type Direction = ENavLinkDirection::LeftToRight;
	Link->GetLinkData(LeftPoint, RightPoint, Direction);

	// ⛔ TestTrue on a negated Equals, ⛔ not TestNotEqual: the automation API ships a
	// TestNotEqual for strings, FText and FName only — there is no FVector overload and no
	// generic template for it (AutomationTest.h:2004-2012).
	TestTrue(TEXT("SELF-CHECK: GetLinkData overwrote the LEFT sentinel — the instrument is live"),
		!LeftPoint.Equals(LeftSentinel));
	TestTrue(TEXT("SELF-CHECK: GetLinkData overwrote the RIGHT sentinel — the instrument is live"),
		!RightPoint.Equals(RightSentinel));

	// ── SELF-CHECK: the direction enumerators are DISTINCT ──────────────────────────────
	// "Direction == BothWays" would be trivially true if the enum had collapsed, and the
	// sentinel above is only meaningful because LeftToRight is a different value.
	TestTrue(TEXT("SELF-CHECK: BothWays / LeftToRight / RightToLeft are three distinct enumerators"),
		ENavLinkDirection::BothWays != ENavLinkDirection::LeftToRight &&
		ENavLinkDirection::BothWays != ENavLinkDirection::RightToLeft);

	// ── (b) ⛔⛔ BothWays IS **REQUIRED**, ⛔ NOT PREFERRED (TOWER-§8.7) ──────────────────
	// Under the ramp, descent was free because Recast polys are UNDIRECTED. A link does ⛔
	// not inherit that property and it has to be re-established explicitly. A one-way ladder
	// leaves the deck a DEAD END with no legal path off it: a unit ordered down has no path,
	// path following never starts, and it stands there permanently — a manufactured stuck
	// unit, the exact NAV-§ failure class this whole namespace exists to avoid.
	TestTrue(TEXT("(b) ⛔ The ladder link is ENavLinkDirection::BothWays — a one-way ladder manufactures a stranded unit on the deck"),
		Direction == ENavLinkDirection::BothWays);

	// ── (c) THE PINNED GEOMETRY (TOWER-§8.3), EXPECTED FROM THE **LAW** ─────────────────
	// ⚠️ NEITHER COORDINATE IS A STYLE CHOICE AND MOVING EITHER "A BIT" SEVERS THE FEATURE
	// SILENTLY: the foot at X −450 clears the body's eroded nav carve (which starts at
	// X ≤ −364) by 86 uu, and the top at X −150 sits 86 uu inside the deck's surviving poly
	// (X ∈ [−236, +236]) after ledge-nulling and erosion take 64 uu per side. A socket on the
	// deck EDGE is the castle-floor defect class: every readback correct, nothing can use it.
	TestEqual(TEXT("(c) The link's START is the pinned LadderFoot (−450, 0, 0) — 86 uu clear of the body's eroded nav carve"),
		LeftPoint, PinnedLadderFootRelative);
	TestEqual(TEXT("(c) The link's END is the pinned LadderTop (−150, 0, 1200) — 86 uu inside the deck's surviving navmesh poly"),
		RightPoint, PinnedLadderTopRelative);

	// ── (d) THE TWO DERIVED FIGURES, RECOMPUTED FROM THE SHIPPED PAIR ───────────────────
	// ⭐ This is what a raw-coordinate pin cannot do: it re-derives the law's OTHER two
	// numbers from the code's own points, so a plausible-looking typo (a sign, a transposed
	// axis, a 120 for a 1200) shows up as a wrong LENGTH or a wrong LEAN rather than as a
	// coordinate nobody can eyeball.
	const FVector ClimbLine = RightPoint - LeftPoint;
	const float LineLengthUU = static_cast<float>(ClimbLine.Size());
	const float LeanDegrees = FMath::RadiansToDegrees(
		FMath::Atan2(static_cast<float>(FMath::Abs(ClimbLine.Z)), static_cast<float>(ClimbLine.Size2D())));

	TestEqual(TEXT("(d) ⭐ ONE straight segment of 1,236.9 uu (TOWER-§8.3)"),
		LineLengthUU, PinnedClimbLineLengthUU, GeometryTolerance);

	// 76° is 2.4× over Recast's real 32.005° walkable ceiling, which is a FEATURE: Recast
	// will never try to walk the ladder, so there is no ambiguity about which route the AI
	// takes. It is also a LEANING siege ladder rather than a vertical one, which is what
	// keeps the traversal to ONE segment instead of three (step in · rise · step out).
	TestEqual(TEXT("(d) ⭐ …leaning 76.0° from horizontal — 2.4× over Recast's 32.005° ceiling, so nothing ever tries to walk it"),
		LeanDegrees, PinnedClimbLeanDegrees, GeometryTolerance);

	// ── (e) THE LINK'S TOP AND THE PLATFORM HEIGHT ARE THE SAME NUMBER ─────────────────
	// Two INDEPENDENTLY DECLARED values that must agree: PlatformHeightUU is the tunable
	// Jonathan owns (🧑 row T-5) and the number HIGH-§ turns into damage; the socket default's
	// Z is the geometry the mesh is built to. ⛔ Neither is computed from the other, so this
	// is the assertion that catches a height retune landing WITHOUT the mesh — the exact
	// de-synchronisation PlatformHeightUU's comment warns about.
	TestEqual(TEXT("(e) The ladder's TOP sits at exactly PlatformHeightUU — a height retune that forgets the mesh fails here"),
		static_cast<float>(RightPoint.Z), Defaults->GetPlatformHeightUU(), Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. ⛔ THE ELEVATION SHELL IS **GONE** (TOWER-§8.6) AND THE SOCKET CONTRACT IS
//     PINNED (TOWER-§8.4(A) + CONVENTIONS "Static-mesh SOCKET names")
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerShellRemovalTest,
	"Siegebound.ClimbableTower.TheAscentGateShellIsGoneAndTheSocketContractIsPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerShellRemovalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
	if (!ClimbableTowerClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: AClimbableTower::StaticClass() returned null."));
		return false;
	}

	// ── SELF-CHECK: FindPropertyByName RESOLVES SOMETHING ON THIS CLASS ────────────────
	// ⚠️⚠️ EVERY ASSERTION BELOW IS A NULL CHECK, AND A LOOKUP THAT HAD STOPPED WORKING
	// WOULD RETURN NULL FOR EVERYTHING AND PASS ALL FOUR — permanently, and for the wrong
	// reason. Two positive lookups first: one member that predates the ladder and one the
	// ladder added.
	TestNotNull(TEXT("SELF-CHECK: 'PlatformHeightUU' still resolves — the property lookup is live"),
		ClimbableTowerClass->FindPropertyByName(TEXT("PlatformHeightUU")));
	TestNotNull(TEXT("SELF-CHECK: 'LadderLink' resolves — the ladder's own member is there to be found"),
		ClimbableTowerClass->FindPropertyByName(TEXT("LadderLink")));

	// ── (a) THE FOUR REMOVED MEMBERS ───────────────────────────────────────────────────
	// ⚖️ THE SHELL HAD EXACTLY ONE JOB — stop an enemy partway up a 2,078-uu ramp whose
	// mouth nobody could locate in mesh-local space — AND THAT JOB NO LONGER EXISTS. A
	// ladder has ONE discrete entry point, so the gate became ONE predicate at that point
	// (CanTeamAscend, test 3). ⭐ That DISSOLVED the shell's known-open pivot-vs-mesh tuning
	// item rather than fixing it, and TOWER-§8.6 also records that the shell could not have
	// done the new job anyway: a scripted MOVE_Flying traversal is not reliably stopped by a
	// blocking volume.
	// ⚠️ Re-adding any of these is a REGRESSION, ⛔ not a restoration — TOWER-§8.2 says so in
	// as many words, and QA is instructed not to read their absence as a defect.
	for (const TCHAR* const RemovedName : RemovedAscentShellMemberNames)
	{
		TestNull(*FString::Printf(
			TEXT("(a) ⛔ '%s' is GONE — the physical elevation shell was removed by TOWER-§8.6, not repaired"), RemovedName),
			ClimbableTowerClass->FindPropertyByName(FName(RemovedName)));
	}

	// ── (b) THE ARTIST↔PROGRAMMER SEAM, BY NAME ────────────────────────────────────────
	// ⭐ These are the two names TASK-737 authors on /Game/Meshes/SM_WatchTower and the two
	// names this class asks for at BeginPlay. They are the whole contract between the two
	// lanes, and the two lanes were built in PARALLEL and could not see each other — so the
	// only thing keeping them in step is that both transcribed the same law. A rename on
	// either side is silent at runtime (the code degrades to the pinned literals and the
	// mesh's sockets are simply never read), which is exactly why it is pinned here.
	TestEqual(TEXT("(b) The foot socket is named exactly 'LadderFoot'"),
		AClimbableTower::LadderFootSocketName, FName(TEXT("LadderFoot")));
	TestEqual(TEXT("(b) The top socket is named exactly 'LadderTop'"),
		AClimbableTower::LadderTopSocketName, FName(TEXT("LadderTop")));

	// SELF-CHECK: two DISTINCT names. If both constants ever collapsed onto one FName the
	// two assertions above could not both hold — but a future refactor that derived one from
	// the other could make them equal while still matching one literal, and the link would
	// then be zero-length.
	TestNotEqual(TEXT("SELF-CHECK: the two socket names are distinct — a collapsed pair would define a zero-length ladder"),
		AClimbableTower::LadderFootSocketName, AClimbableTower::LadderTopSocketName);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⭐ THE ENTRY GATE'S FULL TRUTH TABLE, INCLUDING ITS **PRECEDENCE**
//     (TOWER-§8.6 + TOWER-§10 L-1)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerLadderEntryTest,
	"Siegebound.ClimbableTower.LadderEntryRefusesEnemiesThenASecondClimberInThatFixedOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerLadderEntryTest::RunTest(const FString& Parameters)
{
	using EVerdict = AClimbableTower::ELadderEntryVerdict;

	// ── SELF-CHECK: THE FUNCTION IS NOT A CONSTANT ─────────────────────────────────────
	// ⚠️⚠️ A truth table over a function that returns one value for everything would be a
	// long list of agreeable assertions and no information at all. Prove it can produce at
	// least two DIFFERENT verdicts before believing any single row.
	const EVerdict Admitted = AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, /*bIsUnit=*/ true, /*bBusy=*/ false);
	const EVerdict Refused = AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Red, /*bIsUnit=*/ true, /*bBusy=*/ false);
	TestTrue(TEXT("SELF-CHECK: the verdict is NOT a constant — an own-team climber and an enemy get different answers"),
		Admitted != Refused);

	// ── (a) ✅ THE ONLY ACCEPTING ROW ──────────────────────────────────────────────────
	// Own team, a real ASummonedUnit, ladder free. Both team directions, so a hardcoded
	// team branch cannot hide behind a single-team test.
	TestTrue(TEXT("(a) ✅ A BLUE unit climbs a free BLUE ladder"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, true, false) == EVerdict::Climb);
	TestTrue(TEXT("(a) ✅ A RED unit climbs a free RED ladder"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Red, ETeamId::Red, true, false) == EVerdict::Climb);

	// ── (b) ⛔ T-3 — THE ENEMY IS REFUSED, AND REFUSED AS AN **ENEMY** ─────────────────
	TestTrue(TEXT("(b) ⛔ A RED unit is refused a free BLUE ladder, as WrongTeam"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Red, true, false) == EVerdict::WrongTeam);
	TestTrue(TEXT("(b) ⛔ A BLUE unit is refused a free RED ladder, as WrongTeam"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Red, ETeamId::Blue, true, false) == EVerdict::WrongTeam);

	// ── (c) ⛔ TOWER-§10 L-1 — ONE CLIMBER AT A TIME ───────────────────────────────────
	// Two capsules interpolated along one line WILL interpenetrate: bUseRVOAvoidance is
	// false (TOWER-§4b), so units physically jostle and depenetration would shove one OFF
	// the climb line in mid-air. The second unit waits at the FOOT in its ordinary walking
	// state — ⛔ no queue object, ⛔ no counter, ⛔ no UI, ⛔ no replication.
	//
	// ⚠️⚠️ THE OCCUPANCY TERM IS "ANY CLIMB IS REGISTERED", ⛔ NOT "SOMEBODY **ELSE** IS
	// CLIMBING", AND THE DIFFERENCE IS A UNIT HANGING IN MID-AIR FOREVER. Path following
	// re-enters a link on every re-path (SetMoveSegment → StartUsingCustomLink), including
	// for the unit already on it. Under an "…WithAnother" reading that re-entry would be
	// ADMITTED, BeginLadderClimb would refuse it as *already climbing*, and the refusal
	// handler would unbind and clear a climber still in the air — leaving nothing to abort
	// it when the tower dies (TOWER-§10 L-5). The caller therefore passes
	// `ActiveClimber.IsValid()` with ⛔ no identity comparison, and this row is the
	// function's half of that contract.
	TestTrue(TEXT("(c) ⛔ An own-team unit is refused an OCCUPIED ladder, as LadderBusy — it waits at the foot"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, true, true) == EVerdict::LadderBusy);
	TestTrue(TEXT("(c) ⛔ …and the RED mirror, so no hardcoded team branch hides in the occupancy path"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Red, ETeamId::Red, true, true) == EVerdict::LadderBusy);

	// ── (d) ⭐ THE PRECEDENCE IS PART OF THE CONTRACT ──────────────────────────────────
	// An enemy at a BUSY ladder is refused as an ENEMY, not as traffic. ⚖️ A verdict that
	// flipped with ladder traffic could not be explained by a log line, and "it worked in
	// the test because the ladder happened to be free" is how a gate rots. This row is the
	// one that fails if somebody reorders the ifs for tidiness.
	TestTrue(TEXT("(d) ⭐ An enemy at an OCCUPIED ladder is still WrongTeam — team outranks occupancy"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Red, true, true) == EVerdict::WrongTeam);

	// ── (e) ⛔ IDENTITY OUTRANKS EVERYTHING ────────────────────────────────────────────
	// Anything that is not an ASummonedUnit cannot be driven by the TOWER-§8.4(B) API at
	// all — most obviously the player's own hero. Both rows pass a team value that would
	// otherwise change the answer, which is what proves the short-circuit is real.
	TestTrue(TEXT("(e) ⛔ A non-unit on a free own-team ladder is NotASummonedUnit"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, false, false) == EVerdict::NotASummonedUnit);
	TestTrue(TEXT("(e) ⛔ …and an enemy non-unit at a busy ladder is STILL NotASummonedUnit — identity outranks both other terms"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Red, false, true) == EVerdict::NotASummonedUnit);

	// ── (f) THE TWO GATES AGREE, BUT ARE NOT THE SAME FUNCTION ────────────────────────
	// EvaluateLadderEntry must DELEGATE the team question to CanTeamAscend rather than
	// re-implement it. Asserting the agreement across all four team pairings is what would
	// catch a second, divergent copy of T-3 growing inside the entry gate.
	const ETeamId Teams[] = { ETeamId::Blue, ETeamId::Red };
	for (const ETeamId TowerTeam : Teams)
	{
		for (const ETeamId ClimberTeam : Teams)
		{
			const bool bPredicateAdmits = AClimbableTower::CanTeamAscend(TowerTeam, ClimberTeam);
			const bool bEntryAdmits =
				AClimbableTower::EvaluateLadderEntry(TowerTeam, ClimberTeam, true, false) == EVerdict::Climb;
			// ⛔ TestTrue on the equality, ⛔ not TestEqual(bool, bool): bool would drag in the
			// int32/int64/SIZE_T overload set for no benefit.
			TestTrue(TEXT("(f) The entry gate's team verdict IS CanTeamAscend's — one rule, ⛔ never two copies"),
				bEntryAdmits == bPredicateAdmits);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ⭐ THE OPTIONAL PATHFINDING LAYER FAILS **OPEN** (TOWER-§8.6)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerLinkPathfindingTest,
	"Siegebound.ClimbableTower.TheLinkPathfindingLayerFailsOpenOnEveryUnresolvedQuerier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerLinkPathfindingTest::RunTest(const FString& Parameters)
{
	// ⭐⭐ WHY THIS TEST IS ABOUT THE **PERMISSIVE** CASES, WHICH LOOKS BACKWARDS UNTIL YOU
	// READ THE LAW: TOWER-§8.6 ALLOWS `IsLinkPathfindingAllowed` but explicitly REFUSES to
	// vouch for it — "nobody has measured what Querier actually is." TASK-734 measured it
	// (four cited engine sites, spelled out on UClimbableTowerLadderLink) and shipped the
	// layer anyway — and the entire safety argument for doing so is that EVERY case it
	// cannot resolve returns TRUE. ⇒ if the measurement is wrong, the worst outcome is the
	// behaviour of never having shipped the layer, ⛔ and never an own-team unit that cannot
	// path to its own tower. These rows ARE that argument.
	//
	// ⚠️ They are also the rows most likely to catch a real regression: the naive version of
	// this function — `return CanTeamAscend(Team, Cast<AController>(Querier)->GetPawn()...)` —
	// dereferences null on the very first line, on the pathfinding thread.

	// ── SELF-CHECK: THE RULE IT DELEGATES TO IS LIVE AND DISCRIMINATING ────────────────
	// ⚠️ "It returns true" is only interesting if returning true is an EXCEPTION rather than
	// the function's only behaviour. CanTeamAscend is the rule this layer defers to, and it
	// demonstrably answers both ways (test 3 proves it in full).
	TestFalse(TEXT("SELF-CHECK: the underlying rule refuses an enemy — 'fails open' is an exception, not the only outcome"),
		AClimbableTower::CanTeamAscend(ETeamId::Blue, ETeamId::Red));
	TestTrue(TEXT("SELF-CHECK: …and admits an own-team climber"),
		AClimbableTower::CanTeamAscend(ETeamId::Blue, ETeamId::Blue));

	// ── (a) A NULL QUERIER ─────────────────────────────────────────────────────────────
	// Real and routine: navmesh maintenance queries, editor queries and projections carry
	// no owner at all (FRecastSpeciaLinkFilter caches whatever SearchOwner resolves to,
	// which may be nothing). ⛔ Nothing to judge ⇒ allow.
	TestTrue(TEXT("(a) A NULL querier is allowed — and, crucially, does not dereference null on the pathfinding thread"),
		AClimbableTower::ShouldLinkAllowPathfinding(nullptr, ETeamId::Blue));
	TestTrue(TEXT("(a) …in both team directions"),
		AClimbableTower::ShouldLinkAllowPathfinding(nullptr, ETeamId::Red));

	// ── (b) A QUERIER THAT IS NOT A CONTROLLER ────────────────────────────────────────
	// Any UObject will do; a tower CDO is a convenient one that is definitely not an
	// AController and definitely not null.
	TestTrue(TEXT("(b) A non-controller UObject querier is allowed"),
		AClimbableTower::ShouldLinkAllowPathfinding(GetDefault<AClimbableTower>(), ETeamId::Blue));

	// ── (c) ⭐ A **PAWN** IS NOT A CONTROLLER, AND THIS ROW IS NOT REDUNDANT ───────────
	// The measurement says the Querier is the CONTROLLER (AAIController::BuildPathfindingQuery
	// passes `*this`). The tempting shortcut — treating the querier as the pawn and reading
	// its team directly — would make this row REFUSE for an enemy-team unit CDO and would
	// silently work most of the time. Requiring "allowed" here pins which of the two objects
	// the code believes it is holding.
	TestTrue(TEXT("(c) An ASummonedUnit querier (a pawn, ⛔ not a controller) is allowed — the Querier is the CONTROLLER"),
		AClimbableTower::ShouldLinkAllowPathfinding(GetDefault<ASummonedUnit>(), ETeamId::Blue));
	TestTrue(TEXT("(c) …and for a RED tower too, so the row cannot pass by team coincidence"),
		AClimbableTower::ShouldLinkAllowPathfinding(GetDefault<ASummonedUnit>(), ETeamId::Red));

	// ⛔ THE REFUSING CASE — a live AController possessing a live ITeamAgent pawn — is
	// DECLARED UNCOVERED HERE, deliberately: constructing possessed actors outside a world
	// to fake it is far more likely to crash this suite than to catch a bug. The RULE is
	// covered by test 3; the WIRING is covered by TASK-742's integration and by Jonathan's
	// playtest (an enemy that never paths onto the deck).

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 10. ⭐⭐ THE COMPLETION LANE EXISTS — WHICH IS WHAT LETS THE TOWER STAY TICKLESS
//     AND WHAT `EndPlay` DRIVES (TOWER-§8.4(B), TOWER-§8.5, TOWER-§10 L-5)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerCompletionLaneTest,
	"Siegebound.ClimbableTower.DeclaresTheClimbCompletionUFunctionAndNoPollingSubstitute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerCompletionLaneTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
	if (!ClimbableTowerClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: AClimbableTower::StaticClass() returned null."));
		return false;
	}

	// ── SELF-CHECK: THE FUNCTION TABLE RESOLVES SOMETHING ─────────────────────────────
	// A UFunction lookup that had stopped working would return null for everything, and
	// the claim below would read as "the completion lane is missing" — or, if it were
	// phrased as an absence, would pass vacuously. Anchor it on a UFUNCTION that has been
	// on this class since TASK-726.
	TestNotNull(TEXT("SELF-CHECK: 'GetPlatformHeightUU' resolves as a UFunction — the function table is live"),
		ClimbableTowerClass->FindFunctionByName(TEXT("GetPlatformHeightUU")));

	// ── (a) ⭐⭐ THE COMPLETION HANDLER MUST BE A **UFUNCTION** ───────────────────────
	// ⛔ Not a style preference — a mechanical requirement. `OnLadderClimbEnded` is a DYNAMIC
	// multicast delegate (TOWER-§8.4(B)), and AddUniqueDynamic can only bind a reflected
	// function. If this stopped being a UFUNCTION the binding would fail and the tower would
	// never learn that a climb ended: `ActiveClimber` would never clear, the ladder would be
	// permanently occupied after the FIRST climb of the match, and — worse — TOWER-§10 L-5's
	// abort-on-destroy would be pointing at a handle that no longer means anything.
	const UFunction* const CompletionHandler = ClimbableTowerClass->FindFunctionByName(TEXT("HandleLadderClimbEnded"));
	TestNotNull(TEXT("(a) ⭐ 'HandleLadderClimbEnded' is a reflected UFUNCTION — without it AddUniqueDynamic cannot bind and no climb ever ends, as far as the tower knows"),
		CompletionHandler);

	// ── (b) ITS SHAPE MATCHES THE PINNED DELEGATE, FROM THIS SIDE OF THE SEAM ─────────
	// FSiegeLadderClimbEnded is pinned as TWO parameters (ASummonedUnit*, bool) in
	// TOWER-§8.4(B). ⚠️ TASK-734 and TASK-738 were written IN PARALLEL and could not see
	// each other, so the only thing keeping the handler and the delegate in step is that
	// both transcribed the same law. This is the cheapest possible check that they did.
	if (CompletionHandler)
	{
		TestEqual(TEXT("(b) …taking exactly the 2 parameters FSiegeLadderClimbEnded declares (ASummonedUnit*, bool)"),
			static_cast<int32>(CompletionHandler->NumParms), 2);
	}

	// ── (c) ⛔ AND NOTHING POLLS ───────────────────────────────────────────────────────
	// ⭐ The delegate above is what BUYS the no-tick property, and that was not free: a
	// traversal has an END, and the obvious way to notice an end is to poll for it. Test 2
	// asserts bCanEverTick is false; this asserts the other half — that no timer handle, no
	// poll interval and no tick-shaped member crept in as a substitute for the callback.
	TArray<FString> DeclaredMemberNames;
	CollectDeclaredMemberNames(ClimbableTowerClass, DeclaredMemberNames);

	TestTrue(TEXT("SELF-CHECK: the reflection walk is live (it found the completion handler it just resolved)"),
		DeclaredMemberNames.Contains(TEXT("HandleLadderClimbEnded")));

	static const TCHAR* const PollingTokens[] = { TEXT("Timer"), TEXT("Tick"), TEXT("Poll"), TEXT("Interval") };
	for (const FString& MemberName : DeclaredMemberNames)
	{
		for (const TCHAR* const Token : PollingTokens)
		{
			if (MemberName.Contains(Token, ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ AClimbableTower declares '%s', which names a POLL ('%s'). Both edges of a climb are PUSHED to this class — ")
					TEXT("entry through the link's FOnMoveReachedLink, completion through ASummonedUnit::OnLadderClimbEnded — so there is nothing to poll (TOWER-§8 (4))."),
					*MemberName, Token));
			}
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
