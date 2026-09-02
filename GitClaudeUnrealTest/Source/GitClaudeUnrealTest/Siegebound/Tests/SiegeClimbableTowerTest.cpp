// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "AI/Navigation/NavLinkDefinition.h"
#include "Containers/UnrealString.h"
#include "GameFramework/Character.h" // TASK-777 (CONTACT-§4.4): ACharacter::StaticClass() is the widened delegate's pinned first parameter type
#include "Misc/FileHelper.h" // TASK-787 (CONTACT-§12): tests 14/15 read the shipped .cpp — a Cast<> leaves NOTHING in the reflection tables, so "the tower names no concrete climber class" is unaskable any other way
#include "Misc/Paths.h"
#include "NavLinkCustomComponent.h"
#include "Siegebound/Building.h"
#include "Siegebound/ClimbableTower.h"
#include "Siegebound/HeroCharacter.h" // TASK-787 (CONTACT-§12.3): test 15 asserts the HERO and the UNIT share ONE completion delegate type — the property the whole widening turns on
#include "Siegebound/LadderClimber.h" // TASK-787: ULadderClimber::StaticClass() for the ImplementsInterface rows, and FSiegeLadderClimbEnded's new home
#include "Siegebound/SiegeLadderClimbStatics.h" // TASK-777 (CONTACT-§4.1): FSiegeLadderContactState / FSiegeLadderContactStatics, and CanBegin for the second no-double-fire belt
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
	 *
	 *  ⭐⭐ AND IT ⛔ HAS NOW FIRED — 2026-09-02, WHICH IS WHY THESE TWO LINES ARE ⛔ NOT
	 *  DECORATION. `TOWER-§8.3` was amended to **−460 / −160** on Jonathan's `CONTACT-§7`
	 *  `K-1` = option `A` (TASK-783 translated the ladder west by `δ = 10 uu` to buy the
	 *  hero's standoff back — `dist(spine, geometry)` 93.619 → 103.320 vs a ≥ 98.0 gate).
	 *  These pins went **RED** against the still-shipped `−450 / −150` fallback literals —
	 *  ⛔ the exact line whose hero clearance is **51.6 against a required 56**, i.e. the
	 *  line `TOWER-§8.5a` is VOID on. ⭐ A degrade-open fallback would have restored it
	 *  SILENTLY, with a fully functional tower and every readback correct; this assertion
	 *  is the only thing that said so out loud. ⛔ Do not weaken it, and ⛔ never "sync" it
	 *  by reading the constants back off the class under test — that is the whole point of
	 *  the direction.
	 */
	const FVector PinnedLadderFootRelative(-460.f, 0.f, 0.f);
	const FVector PinnedLadderTopRelative(-160.f, 0.f, 1200.f);

	/**
	 *  `TOWER-§8.3`'s two DERIVED figures for the same segment. ⭐ Recomputed from the shipped pair, so they catch a typo the raw-coordinate pin could not explain.
	 *  ⭐ ⛔ UNCHANGED by the 2026-09-02 move, and that is the ⛔ PURE translation paying out: both endpoints shifted by the SAME `δ`, so `Δ = (300, 0, 1200)` is invariant and the length and lean cannot notice.
	 */
	constexpr float PinnedClimbLineLengthUU = 1236.9f;
	constexpr float PinnedClimbLeanDegrees = 76.0f;

	/** The law quotes both figures rounded to one decimal, so the band has to admit the rounding (~0.04 on each) and nothing else. */
	constexpr float GeometryTolerance = 0.1f;

	// ═════════════════════════════════════════════════════════════════════════════
	//  THE CONTACT TRIGGER'S FIXTURE (TASK-777, CONTACT-§4)
	// ═════════════════════════════════════════════════════════════════════════════

	/**
	 *  ⛔ THE THREE FEEL NUMBERS, TYPED FROM `CONTACT-§7` AND ⛔ NEVER READ BACK OFF THE
	 *  CLASS UNDER TEST — the same direction the pinned socket coordinates above are
	 *  typed in. ⚠️ The cone and the dwell are DECLARED INVENTED numbers and Jonathan may
	 *  overrule either in one word; when he does, the row that reads them off the CDO
	 *  fails HERE and names the drift, which is the only way a "feel" value and a test
	 *  can be kept honest about each other.
	 *
	 *  ⚠️⚠️ THE RADIUS MOVED `K-5` 150 → `K-6` 300 (TASK-784) → **350** (TASK-786, which
	 *  also moves this pin). ⭐ IT IS THE ONE OF THE THREE THAT IS ⛔ NOT INVENTED — both
	 *  bounds are DERIVED, and they are ⛔ NOT the same arithmetic:
	 *
	 *      UNITS  poll at 0.25 s, each poll crediting a WHOLE 0.25 s of dwell for one
	 *             sampled instant ⇒ the bar is "TWO samples inside the window":
	 *             R >= 2 × 0.25 × 600 (Cavalry)      = 300 uu   [test 16(a) derives this]
	 *      HERO   polls PER FRAME in Tick, so dwell accrues DeltaSeconds and the model is
	 *             ⭐ EXACT rather than probabilistic:
	 *             R >= 0.35 × 937.5 (sprint + Boots) = **328.125 uu**
	 *
	 *  ⇒ ⭐⭐ **THE HERO BINDS, ⛔ NOT THE ROSTER**, and 350 clears 328.125 by **6.67%**.
	 *  ⚠️⚠️ THAT IS EXACTLY WHY 300 LOOKED SUFFICIENT AND WAS ⛔ NOT: `SiegeLadderClimbTest`
	 *  test 16 derives the radius from the UNIT poll and the UNIT roster, so it is
	 *  unit-shaped by construction and ⛔ cannot see the hero's per-frame requirement.
	 *  ⛔ It is ⛔ NOT wrong and ⛔ must NOT be weakened — it is simply answering a
	 *  different question, and this pin is the one that carries the hero's answer.
	 *
	 *  ⛔⛔ AND NEITHER OLD VALUE MERELY FAILED — 150 FAILED **INTERMITTENTLY**, WHICH IS
	 *  WORSE THAN A DEAD FEATURE BECAUSE IT READS TO A PLAYER AS "sometimes it works":
	 *  at 150 the unit phase counts were Ogre 16/16 · Knight 16/16 · Archer **11/16** ·
	 *  Footman **8/16** · Sapper **3/16** · Cavalry **0/16**. From 300 up, every roster
	 *  card is 16/16 — ⚠️ so the units were already fixed at 300 and the 300 → 350 step
	 *  buys ⛔ nothing for them. It is bought ENTIRELY for the hero's fourth speed
	 *  (sprint + Swift Boots, 937.5 uu/s), which at 300 fails at EVERY offset and every
	 *  frame rate, and at 350 holds on every phase down to ~20 fps.
	 *  ⚠️ The price is the abduction window, and it grows SUPER-linearly — see
	 *  `WalkPastAndSeeIfAdmitted` below, which is why its passer-by offset had to move too.
	 */
	constexpr float PinnedContactRadiusUU = 350.f;
	constexpr float PinnedContactIntentCos = 0.5f;   // = a 60° half-cone
	constexpr float PinnedContactDwellSeconds = 0.35f;

	/** The half-height the ladder scenarios stand a pawn on — `SiegeSpawnConstants.h:9`'s 88, used as this file's SCENARIO exactly as the socket coordinates are. */
	constexpr float ScenarioCapsuleHalfHeightUU = 88.f;

	/** A 60 Hz frame. ⛔ Not a tunable — it is the step the dwell is INTEGRATED at, and stepping the predicate rather than handing it one big delta is what proves the accumulation is continuous. */
	constexpr float FrameStepSeconds = 1.f / 60.f;

	/** A walk speed to drive the scenarios at. ⭐ Chosen as the shipped ladder rate (`LadderClimbSpeedUU` = 350) rounded DOWN to 300, so every timing expectation below is a round number that can be checked by hand. */
	constexpr float ScenarioWalkSpeedUU = 300.f;

	/**
	 *  Steps the pure predicate for `TotalSeconds` at `FrameStepSeconds`, holding the pawn
	 *  STILL at one place with one velocity, and returns the LAST verdict.
	 *  ⚠️ A fixed location is a deliberate simplification for the rows that are about the
	 *  CLOCK; the rows that are about the geometry (`WalkPastAndSeeIfAdmitted`) actually
	 *  move the pawn, because a bearing that never swings would let a broken cone pass.
	 */
	static ESiegeLadderContactVerdict HoldContact(FSiegeLadderContactState& State,
		const FVector& Location, const FVector& Velocity, float TotalSeconds, bool& bOutAscending)
	{
		ESiegeLadderContactVerdict Verdict = ESiegeLadderContactVerdict::TooFar;
		const int32 Steps = FMath::Max(1, FMath::RoundToInt(TotalSeconds / FrameStepSeconds));
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Verdict = FSiegeLadderContactStatics::WantsToClimb(State, Location, Velocity,
				PinnedLadderFootRelative, PinnedLadderTopRelative,
				PinnedContactRadiusUU, PinnedContactIntentCos, PinnedContactDwellSeconds,
				FrameStepSeconds, bOutAscending);
		}
		return Verdict;
	}

	/**
	 *  ⭐⭐ THE ABDUCTION TEST, AND IT IS THE ONE THAT ACTUALLY MOVES A PAWN: walks a body
	 *  in a straight line along +X, `PerpendicularOffsetY` uu to the side of the ladder
	 *  foot, from well outside the trigger disc to well past it — and reports whether the
	 *  predicate ever admitted it.
	 *
	 *  ⭐ THE ARITHMETIC THE TWO SHIPPED ROWS ARE DERIVED FROM, so neither expectation is
	 *  a transcription of the code: with the cone measured PAWN → ENDPOINT, a pawn passing
	 *  at offset `d` holds the cone only while it is still `d / tan 60° = d / 1.732` short
	 *  of its closest approach, so it can accumulate `(sqrt(R² − d²) − d/1.732) / v`
	 *  seconds.
	 *
	 *  ⚠️⚠️ RE-DERIVED AT `K-6`'s R = **350** BY TASK-786, AND THE OLD PASSER-BY OFFSET HAD
	 *  TO MOVE — ⛔ this is ⛔ NOT a weakened row, it is the same claim re-sited onto a disc
	 *  that grew. At v = 300 the dwell only ever eats a FIXED `v × 0.35 = 105 uu` of
	 *  approach, and that is a SMALLER FRACTION of a bigger disc, so the grab window grows
	 *  **super-linearly** at every step:
	 *      R = 150 → ±57.8    R = 300 → ±202.1    R = 350 → **±247.2**
	 *  i.e. **3.5× for the first doubling, then +22% for a +17% radius rise.** ⛔ A linear
	 *  model understates it at BOTH steps: "~0.4 · R" first put `K-6`'s cost at ±125 when it
	 *  was ±204, and scaling ±204 by 350/300 would guess ±238 rather than the real ±247.
	 *  ⇒ the old d = 100 row (REFUSED at 150 with 0.180 s) banks **0.895 s** at 350 and would
	 *  be ADMITTED. ⚠️ d = 250 is ⛔ ALSO too tight now — it refuses by only **0.015 s**,
	 *  which is a coin-flip dressed as an assertion. The refusal row is taken at **d = 300**:
	 *    • d = 0   → 350/300             = **1.167 s** (≥ 0.35 ⇒ ADMITTED — walking straight
	 *                                      at the ladder IS the intent)
	 *    • d = 300 → (180.3 − 173.2)/300 = **0.024 s** (< 0.35 ⇒ REFUSED, by a 0.326 s margin)
	 *  ⭐ AND d = 300 IS A **STRONGER** STATEMENT THAN d = 100 EVER WAS: at 300 the pawn is
	 *  inside the radius for **1.202 s — nearly FOUR dwells** — and is still refused, so the
	 *  row now isolates the CONE as the thing doing the refusing. At the old offset the
	 *  proximity term was helping.
	 */
	static bool WalkPastAndSeeIfAdmitted(float PerpendicularOffsetY)
	{
		FSiegeLadderContactState State;
		FVector Location(PinnedLadderFootRelative.X - 700.f, PerpendicularOffsetY, ScenarioCapsuleHalfHeightUU);
		const FVector Velocity(ScenarioWalkSpeedUU, 0.f, 0.f);

		bool bAscending = false;
		// ⚠️ The run was widened with the radius (was 200 frames from −400): at R = 300 a −400
		// start is only 100 uu clear of the disc. ⭐ Extra travel can only ever ADD chances to
		// be admitted, so a longer run makes the REFUSAL row strictly harder to pass and can
		// never flatter the admission row.
		for (int32 Step = 0; Step < 300; ++Step) // 300 frames × 5 uu = 1,500 uu of travel: in at −700, out at +800
		{
			const ESiegeLadderContactVerdict Verdict = FSiegeLadderContactStatics::WantsToClimb(
				State, Location, Velocity, PinnedLadderFootRelative, PinnedLadderTopRelative,
				PinnedContactRadiusUU, PinnedContactIntentCos, PinnedContactDwellSeconds,
				FrameStepSeconds, bAscending);
			if (Verdict == ESiegeLadderContactVerdict::Climb)
			{
				return true;
			}
			Location += Velocity * FrameStepSeconds;
		}
		return false;
	}

	// ═══════════════════════════════════════════════════════════════════════════════════════
	//  ⭐ TASK-787's SOURCE-SCAN INSTRUMENT (CONTACT-§12) — the `SiegeHeroLadderClimbTest.cpp`
	//  helpers, same shape, ⛔ not a new mechanism.
	// ═══════════════════════════════════════════════════════════════════════════════════════
	//
	// ⚠️⚠️ WHY THIS FILE — WHICH IS OTHERWISE PURE REFLECTION AND PURE STATICS — NOW READS SOURCE
	// TEXT, AND IT IS A ⛔ MEASUREMENT RATHER THAN A PREFERENCE: the claim TASK-787 has to prove is
	// *"the tower names ⛔ NO concrete climber class on the climb path"*, and a CAST IS NOT
	// REFLECTED. `Cast<ASummonedUnit>` leaves ⛔ nothing in the UClass, ⛔ nothing in a UFunction and
	// ⛔ nothing in a property — it is invisible to every instrument this file already owns.
	// ⇒ the only headless way to assert it is to read the shipped `.cpp`. ⭐ Each probe below
	// carries a POSITIVE CONTROL that requires the scanner to FIND something, so a scan that went
	// blind fails loudly instead of reporting a clean bill of health.
	//
	// ⛔ AND IT IS ⛔ NOT A SUBSTITUTE FOR THE PURE TRUTH TABLES ABOVE. It answers "which TYPE does
	// this line name", which is exactly the question a truth table over bools cannot ask.

	/** Loads one of the shipped source files this suite reads. Reports and returns false rather than passing quietly — a probe that cannot read its subject must FAIL, ⛔ never report SAFE. */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(
				TEXT("⛔ Could not find '%s'. This probe reads the SHIPPED source because a Cast<> leaves nothing in the reflection tables; a probe that cannot read its subject FAILS."),
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

	/** Convenience: the tower's implementation file — the subject of the "no concrete climber class" claim. */
	static bool LoadClimbableTowerCpp(FAutomationTestBase& Test, FString& OutText)
	{
		return LoadProjectSource(Test, TEXT("Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp"), OutText);
	}

	/** Non-overlapping occurrence count of Needle in Haystack (the `SiegeHeroLadderClimbTest` helper, same shape). */
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
	 *  ⭐⭐ OCCURRENCES IN **CODE**, WITH COMMENT-ONLY LINES SKIPPED — and this is ⛔ NOT
	 *  fastidiousness, it is what makes the "⛔ zero concrete climber casts" claim POSSIBLE.
	 *
	 *  ⚠️⚠️ THIS CODEBASE DELIBERATELY WRITES THE REFUSED SHAPES INTO ITS COMMENTS — *"a
	 *  `Cast<ASummonedUnit>` / `Cast<AHeroCharacter>` branch pair was REFUSED"*, *"what used to be
	 *  here was a `CastChecked<ASummonedUnit>`"* — because `CONTACT-§12.4` exists so ⛔ nobody
	 *  re-proposes them as "simpler". ⇒ a scanner that counted comments would force the file to
	 *  CHOOSE between explaining what it refuses and passing its own test, and it would resolve
	 *  that by deleting the explanation. ⚖️ That is exactly backwards: the prose is the guard.
	 *
	 *  ⚠️ DECLARED LIMITATION, ⛔ not hidden: a comment TRAILING a line of code is still scanned
	 *  (there is no tokenizer here). ⭐ Every probe in tests 14/15 is either a whole-line construct
	 *  or a statement, so none of them is exposed to that — and each carries a positive control.
	 */
	static int32 CountOccurrencesInCode(const FString& Source, const TCHAR* Needle)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		int32 Count = 0;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();

			// ⚠️ `*` is qualified rather than bare on purpose: a doc-comment continuation is `* text`
			// or `*/`, while `*GetNameSafe(Foo)` — a dereferenced FString in a UE_LOG argument list —
			// starts a CODE line with the same character. Skipping those would be a silent blind spot
			// in the middle of the file this test is about.
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
	// SILENTLY: the foot at X −460 clears the body's eroded nav carve (which starts at
	// X ≤ −364) by 96 uu, and the top at X −160 sits 76 uu inside the deck's surviving poly
	// (X ∈ [−236, +236]) after ledge-nulling and erosion take 64 uu per side. A socket on the
	// deck EDGE is the castle-floor defect class: every readback correct, nothing can use it.
	// ⚠️ Both margins were RE-MEASURED by TASK-783, ⛔ not predicted from the old pair: the
	// foot's IMPROVED (86 → 96) and the top's is the ONE thing the 2026-09-02 move SPENDS
	// (86 → 76). ⭐ And this pair is now licence-bearing as well as navmesh arithmetic — it is
	// the geometry TOWER-§8.5a's ≥ 98.0 uu standoff was measured on.
	TestEqual(TEXT("(c) The link's START is the pinned LadderFoot (−460, 0, 0) — 96 uu clear of the body's eroded nav carve"),
		LeftPoint, PinnedLadderFootRelative);
	TestEqual(TEXT("(c) The link's END is the pinned LadderTop (−160, 0, 1200) — 76 uu inside the deck's surviving navmesh poly"),
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
	// Anything that does not implement `ILadderClimber` can be neither driven nor heard from by
	// the tower, so admitting it would claim the occupancy slot for a body that could never
	// release it. Both rows pass a team value that would otherwise change the answer, which is
	// what proves the short-circuit is real.
	//
	// ⚠️⚠️ THE **RULE** IS UNCHANGED HERE AND THE **SET** IS NOT (TASK-787, CONTACT-§12): this
	// argument used to mean *"not an `ASummonedUnit`, most obviously the player's own hero"*, and
	// the hero is now ADMITTED (test 15 row (a)). ⛔ These two rows were ⛔ NOT weakened to follow
	// it — they still assert that identity outranks team AND occupancy, which is what would catch
	// somebody reordering the ifs. ⭐ What they now describe is a body that implements ⛔ nothing:
	// a future spectator, or anything else that could be handed to this gate by mistake.
	TestTrue(TEXT("(e) ⛔ A pawn that is not an admitted climber, on a free own-team ladder, is NotAnAdmittedClimber"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, false, false) == EVerdict::NotAnAdmittedClimber);
	TestTrue(TEXT("(e) ⛔ …and an enemy non-climber at a busy ladder is STILL NotAnAdmittedClimber — identity outranks both other terms"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Red, false, true) == EVerdict::NotAnAdmittedClimber);

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
	// FSiegeLadderClimbEnded is pinned as TWO parameters in TOWER-§8.4(B), and CONTACT-§4.4
	// AMENDED the first one's TYPE from ASummonedUnit* to ACharacter* (TASK-777). ⚠️ TASK-734
	// and TASK-738 were written IN PARALLEL and could not see each other, so the only thing
	// keeping the handler and the delegate in step is that both transcribed the same law. This
	// is the cheapest possible check that they did.
	//
	// ⭐⭐ AND THE ARITY CHECK ALONE WOULD HAVE SLEPT THROUGH THE WHOLE AMENDMENT — the count
	// is 2 before and after — so the parameter TYPE is asserted here too. ⛔ Without this row
	// the handler could drift back to ASummonedUnit* (or to APawn*, the shape that was ruled
	// AGAINST) and this test would stay green while AddUniqueDynamic silently refused to bind,
	// leaving ActiveClimber permanently occupied after the first climb of the match.
	if (CompletionHandler)
	{
		TestEqual(TEXT("(b) …taking exactly the 2 parameters FSiegeLadderClimbEnded declares (ACharacter*, bool)"),
			static_cast<int32>(CompletionHandler->NumParms), 2);

		int32 ParameterIndex = 0;
		bool bSawFirstParameter = false;
		for (TFieldIterator<FProperty> ParamIt(CompletionHandler); ParamIt && ParamIt->HasAnyPropertyFlags(CPF_Parm); ++ParamIt, ++ParameterIndex)
		{
			if (ParameterIndex != 0)
			{
				continue;
			}
			bSawFirstParameter = true;

			const FObjectProperty* const ClimberParam = CastField<FObjectProperty>(*ParamIt);
			if (TestNotNull(TEXT("(b) The handler's first parameter is an object property"), ClimberParam))
			{
				TestTrue(TEXT("(b) ⭐ …and it is an ACharacter* — CONTACT-§4.4's ruled widening, WITHOUT which a HERO climber is invisible to TOWER-§10 L-1 and to EndPlay's abort (the tower falls and the hero hangs in MOVE_Flying forever)"),
					ClimberParam->PropertyClass == ACharacter::StaticClass());
			}
		}

		// ⚠️ SELF-CHECK: a parameter walk that found nothing would let the type claim above
		// pass by never running — the vacuous pass this whole file is written against.
		TestTrue(TEXT("SELF-CHECK: the parameter walk actually reached the handler's first parameter"),
			bSawFirstParameter);
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
					TEXT("entry through the link's FOnMoveReachedLink, completion through ILadderClimber::GetOnLadderClimbEnded() — so there is nothing to poll (TOWER-§8 (4))."),
					*MemberName, Token));
			}
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 11. ⭐⭐ THE CONTACT TRIGGER'S THREE TERMS — AND THE ONE THAT STOPS A UNIT BEING
//     ABDUCTED AS IT MARCHES PAST ITS OWN TOWER (TASK-777, CONTACT-§4.1 / K-C)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⛔⛔ THE MEASUREMENT THAT SHAPES EVERY ROW BELOW: **THE LADDER HAS ZERO COLLISION**
// (TASK-737 — no hull anywhere over LadderFoot). ⇒ there is ⛔ nothing to walk
// *against*, and a trigger that waited for a blocking hit would ⛔ NEVER fire. The
// mechanism is therefore PROXIMITY + INTENT + DWELL, and these rows are what stop
// that from meaning "anything near the tower gets grabbed".

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerContactTermsTest,
	"Siegebound.ClimbableTower.ContactNeedsProximityIntentAndDwellAndRefusesAPasserBy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerContactTermsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;
	using EContact = ESiegeLadderContactVerdict;

	// A pawn standing on the ground, 100 uu WEST of the ladder foot — inside the 350 uu
	// disc — and the two velocities that make it either walk into the ladder or away.
	//
	// ⚠️⚠️ `FarFromFoot` MOVED −300 → −600 WITH `K-6` (TASK-786), AND IT IS ⛔ NOT COSMETIC:
	// the old value is ⛔ no longer outside the disc at all. At R = 300 a pawn 300 uu out sat
	// EXACTLY on the rim and the shipped proximity test is `DistSquared2D <= R²` — an
	// inclusive `<=` — so it read as IN RANGE; at R = 350 it is 50 uu INSIDE. ⇒ every row
	// below that leans on it (the self-check, (e), and (i)'s first re-arm condition) would
	// have silently changed meaning instead of failing. ⭐ 600 uu clears even 350 by 250 uu,
	// so this fixture does not need revisiting on the next retune.
	const FVector AtFoot = PinnedLadderFootRelative + FVector(-100.f, 0.f, ScenarioCapsuleHalfHeightUU);
	const FVector FarFromFoot = PinnedLadderFootRelative + FVector(-600.f, 0.f, ScenarioCapsuleHalfHeightUU);
	const FVector Inward(ScenarioWalkSpeedUU, 0.f, 0.f);
	const FVector Outward(-ScenarioWalkSpeedUU, 0.f, 0.f);

	bool bAscending = false;

	// ── SELF-CHECK: THE PREDICATE IS NOT A CONSTANT ────────────────────────────────────
	// ⚠️⚠️ A predicate that returned one value for everything would make every row below a
	// long list of agreeable assertions carrying no information at all — the trap the top
	// of this file is written against. Prove two DIFFERENT verdicts first.
	{
		FSiegeLadderContactState Admitted;
		FSiegeLadderContactState Refused;
		const EContact A = HoldContact(Admitted, AtFoot, Inward, 1.f, bAscending);
		const EContact B = HoldContact(Refused, FarFromFoot, Inward, 1.f, bAscending);
		TestTrue(TEXT("SELF-CHECK: the contact predicate is NOT a constant — a pawn pressing into the ladder and a pawn 600 uu away get different answers"),
			A != B);
		TestTrue(TEXT("SELF-CHECK: …and the admitting case really is Climb, so every 'refused' row below is refusing something that could otherwise have succeeded"),
			A == EContact::Climb);
	}

	// ── (a) ⛔ INSIDE THE RADIUS, MOVING **AWAY** ⇒ NEVER CLIMBS ───────────────────────
	// The proximity term alone is not the trigger. ⭐ This is the row that fails if somebody
	// "simplifies" the cone away — and its failure mode in the world is a pawn being yanked
	// 1,200 uu into the air while walking in the opposite direction.
	{
		FSiegeLadderContactState State;
		TestTrue(TEXT("(a) ⛔ A pawn INSIDE the radius walking AWAY from the ladder is NotHeadingIn, however long it does it"),
			HoldContact(State, AtFoot, Outward, 5.f, bAscending) == EContact::NotHeadingIn);
		TestEqual(TEXT("(a) …and its dwell clock is held at zero, so it cannot bank credit for a later approach"),
			State.DwellSeconds, 0.f);
	}

	// ── (b) ⛔ TOWARD IT, BUT FOR **LESS** THAN THE DWELL ⇒ DOES NOT CLIMB ─────────────
	// ⭐ The two halves of this row differ ONLY in elapsed time, which is what makes it a
	// test of the DWELL rather than of the other two terms.
	{
		FSiegeLadderContactState State;
		TestTrue(TEXT("(b) ⛔ 0.30 s of pressing into the ladder is not enough — the verdict is Dwelling, ⛔ not Climb"),
			HoldContact(State, AtFoot, Inward, 0.30f, bAscending) == EContact::Dwelling);
		TestTrue(TEXT("(b) ✅ …and continuing past 0.35 s on the SAME state admits it — the only thing that changed is the clock"),
			HoldContact(State, AtFoot, Inward, 0.10f, bAscending) == EContact::Climb);
	}

	// ── (c) ⛔ THE DWELL MUST BE **CONTINUOUS** ────────────────────────────────────────
	// ⭐ Two thirds of a dwell, interrupted by one frame of steering away, is worth nothing —
	// ⛔ not two thirds. A dwell that survived interruption would let a pawn accumulate
	// credit by loitering, which is the abduction defect with extra steps.
	{
		FSiegeLadderContactState State;
		HoldContact(State, AtFoot, Inward, 0.30f, bAscending);
		HoldContact(State, AtFoot, Outward, FrameStepSeconds, bAscending);
		TestTrue(TEXT("(c) ⛔ ONE frame of steering away resets the dwell — 0.30 s banked + 0.30 s more is still Dwelling, ⛔ not Climb"),
			HoldContact(State, AtFoot, Inward, 0.30f, bAscending) == EContact::Dwelling);
	}

	// ── (d) ⛔ STANDING STILL IS NOT WALKING INTO IT ───────────────────────────────────
	// MinContactSpeedUU is a derived degeneracy floor (⛔ not a fourth feel number): below
	// 1 uu/s a pawn covers less than 0.35 uu across the whole dwell. ⚠️ Without it, braking
	// residue or a depenetration nudge pointed at the ladder reads as intent.
	{
		FSiegeLadderContactState Still;
		FSiegeLadderContactState Creeping;
		TestTrue(TEXT("(d) ⛔ A pawn STANDING STILL inside the radius never climbs, however long it stands there"),
			HoldContact(Still, AtFoot, FVector::ZeroVector, 5.f, bAscending) == EContact::NotHeadingIn);
		TestTrue(TEXT("(d) ⛔ …and neither does one creeping at 0.5 uu/s — below the derived MinContactSpeedUU floor"),
			HoldContact(Creeping, AtFoot, FVector(0.5f, 0.f, 0.f), 5.f, bAscending) == EContact::NotHeadingIn);
	}

	// ── (e) ⛔ OUTSIDE THE RADIUS, WALKING STRAIGHT AT IT ⇒ TooFar ─────────────────────
	{
		FSiegeLadderContactState State;
		TestTrue(TEXT("(e) ⛔ A pawn 600 uu out, walking straight at the ladder, is TooFar — intent alone does not reach across the map"),
			HoldContact(State, FarFromFoot, Inward, 5.f, bAscending) == EContact::TooFar);
	}

	// ── (f) ⭐⭐ THE ABDUCTION BOUNDARY, WITH THE PAWN ACTUALLY WALKING ────────────────
	// ⚠️⚠️ THIS IS THE ROW THE DWELL EXISTS FOR, AND IT IS THE ONE `CONTACT-§4.1` NAMES:
	// *"a friendly unit marching past its own tower toward the enemy castle clips the cone
	// for two frames and is yanked 1,200 uu into the air."* Both expectations are DERIVED
	// (see WalkPastAndSeeIfAdmitted): 1.167 s of in-cone time dead-on vs 0.024 s at 300 uu
	// offset, against a 0.35 s dwell.
	//
	// ⚠️⚠️ THE PASSER-BY OFFSET MOVED 100 → 300 WITH `K-6` (TASK-786) BECAUSE THE GEOMETRY
	// MOVED UNDER IT, ⛔ NOT TO MAKE A RED ROW GREEN: at R = 350 a pawn 100 uu to the side
	// banks 0.895 s and IS abducted, so leaving the row at 100 would have shipped a genuine
	// FAILURE. ⚠️ 250 was ⛔ also rejected — it refuses by only 0.015 s, and an assertion
	// that close to its own boundary is a coin flip, ⛔ not a test. 300 refuses by 0.326 s.
	// ⭐ THE COST IS DECLARED RATHER THAN ABSORBED: `K-6` buys reachability by WIDENING this
	// window from ±57.8 (R=150) to ±202.1 (R=300) to ±247.2 uu (R=350), because the dwell
	// only ever eats a fixed 105 uu of a disc that keeps growing.
	// ⭐ The two rows must DISAGREE, which is also this pair's own self-check: if they ever
	// agree, the walk has stopped discriminating and both claims are worthless.
	{
		const bool bAimedAtIt = WalkPastAndSeeIfAdmitted(0.f);
		const bool bMarchingPast = WalkPastAndSeeIfAdmitted(300.f);

		TestTrue(TEXT("(f) ✅ A pawn walking STRAIGHT AT the ladder foot is admitted — 1.167 s of in-cone approach against a 0.35 s dwell"),
			bAimedAtIt);
		TestFalse(TEXT("(f) ⛔⛔ A pawn marching PAST 300 uu to the side is ⛔ NOT abducted — it is INSIDE the 350 uu disc for 1.202 s, nearly FOUR whole dwells, and the 60° cone still refuses it after 0.024 s. ⭐ At K-6's radius this row isolates the CONE: proximity is no longer helping"),
			bMarchingPast);
		TestTrue(TEXT("SELF-CHECK: the two walks DISAGREE — a walk that admitted (or refused) everything would make both rows above vacuous"),
			bAimedAtIt != bMarchingPast);
	}

	// ── (g) ⭐ BOTH ENDPOINTS ARM, AND THE END IS RESOLVED BY **Z** (K-C) ──────────────
	// ⚠️⚠️ THE Z RULE IS ARITHMETIC, ⛔ NOT TASTE: the endpoints are 1,200 uu apart in Z but
	// only 300 uu apart in XY, so a pawn on the GROUND can sit inside the TOP's XY disc.
	//
	// ⚠️⚠️ RE-CHECKED AT `K-6`'s R = **350** BY TASK-786, AND THE PREMISE GOT **STRONGER** AT
	// EVERY STEP, ⛔ not weaker. The XY separation is fixed at 300 uu, so the discs go:
	//     R = 150 → exactly TANGENT      (a ground pawn merely GRAZED the foot's rim)
	//     R = 300 → OVERLAP, 300-uu lens (each endpoint's centre lands ON the other's rim)
	//     R = 350 → OVERLAP, 400-uu lens (each centre is 50 uu INSIDE the other's disc)
	// ⭐ **Each endpoint's XY centre now lies well inside the OTHER endpoint's disc.** ⇒ the
	// Z resolution is no longer merely tidy, it is the ONLY thing keeping a pawn at the foot
	// from being range-eligible for the deck and vice versa.
	// ⭐ AND IT SURVIVES THE WIDENING BY CONSTRUCTION, WHICH IS WHY THIS ROW STILL PASSES:
	// `IsAtTopEndpoint` (SiegeLadderClimbStatics.cpp:213) compares |P.z − Top.z| against
	// |P.z − Foot.z| and takes ⛔ NO radius term at all, so no radius can perturb it.
	// ⛔ WHAT WOULD ACTUALLY BREAK IT, NAMED RATHER THAN LEFT TO BE DISCOVERED: the rule is a
	// midplane test at Z = PlatformHeightUU / 2 = **600 uu**. A ground pawn stands at 88, i.e.
	// 512 uu of margin. It fails only if a pawn can stand ABOVE 600 uu at the foot — a deck
	// lowered under ~176 uu, or a second structure tall enough to stand on beside the ladder.
	// ⚠️ ⛔ A radius change can never cause it; a `PlatformHeightUU` change can.
	//
	// ⚠️ THE THIRD ROW'S EXPECTATION MOVED `TooFar` → `Climb` WITH THE RADIUS, AND ⛔ NOT TO
	// DODGE A RED: under the deck socket the pawn is EXACTLY 300 uu from the foot, which at
	// `K-5` was out of range and from `K-6` on is INSIDE it. ⭐ At 350 it is inside by a
	// clean **50 uu**, so this row ⛔ no longer rests on the inclusive `<=` at 300² == 300²
	// the way it briefly did at R = 300 — the boundary case is gone, ⛔ not merely tolerated.
	// ⭐ Its velocity is turned to `Outward` so the row keeps DISCRIMINATING rather than
	// merely refusing: resolving by
	// **Z** picks the foot, which the pawn is walking straight at ⇒ Climb + ASCENT; resolving
	// by **XY** would pick the top, whose 2D bearing from directly underneath is degenerate
	// ⇒ NotHeadingIn + descent. ⛔ The two hypotheses now differ in BOTH outputs. Had the
	// expectation simply been relaxed to `NotHeadingIn`, BOTH hypotheses would have produced
	// it and the row would have proved exactly nothing.
	{
		const FVector OnDeck = PinnedLadderTopRelative + FVector(100.f, 0.f, ScenarioCapsuleHalfHeightUU);
		FSiegeLadderContactState Descending;
		FSiegeLadderContactState UnderTheDeck;
		bool bDescendAscending = true;

		TestTrue(TEXT("(g) ✅ Walking into the ladder ON THE DECK admits a climb — both endpoints arm the trigger"),
			HoldContact(Descending, OnDeck, Outward, 1.f, bDescendAscending) == EContact::Climb);
		TestFalse(TEXT("(g) ⭐ …and it reports a DESCENT, so the driver is handed the line the right way round"),
			bDescendAscending);

		bool bUnderAscending = false;
		TestTrue(TEXT("(g) ⭐⭐ A pawn on the GROUND under the deck socket resolves to the FOOT by Z — at K-6's radius the two discs OVERLAP and it is 300 uu from the foot, so walking west it CLIMBS the foot. An XY resolution would have picked the top, whose bearing from directly underneath is degenerate, and refused as NotHeadingIn"),
			HoldContact(UnderTheDeck, FVector(PinnedLadderTopRelative.X, 0.f, ScenarioCapsuleHalfHeightUU), Outward, 1.f, bUnderAscending) == EContact::Climb);
		TestTrue(TEXT("(g) …and it is reported as an ASCENT, which is the half that proves the Z resolution picked the FOOT — this flag is written on every path, including refusals, and is a pure function of Z with ⛔ no radius term"),
			bUnderAscending);
	}

	// ── (h) ⛔⛔ K-C's RE-ARM LATCH — THE YO-YO, AND IT IS A CERTAIN DEFECT ────────────
	// A pawn that finishes a DESCENT stands at the foot, inside the radius, still supplying
	// the input that brought it there. ⛔ Without the latch it re-climbs on the very next
	// frame and the player is stuck in a loop.
	{
		FSiegeLadderContactState State;
		TestTrue(TEXT("SELF-CHECK: the pawn is admitted BEFORE the latch is armed — otherwise the refusal below proves nothing"),
			HoldContact(State, AtFoot, Inward, 1.f, bAscending) == EContact::Climb);

		FSiegeLadderContactStatics::Disarm(State, /*bAtTop=*/ false);

		TestTrue(TEXT("(h) ⛔ Standing where the climb ended, still pressing into the ladder, is REFUSED as Disarmed — ⛔ no instant re-ascent"),
			HoldContact(State, AtFoot, Inward, 5.f, bAscending) == EContact::Disarmed);
		TestEqual(TEXT("(h) …and no dwell accumulates while latched, so the re-arm cannot be paid for in advance"),
			State.DwellSeconds, 0.f);
	}

	// ── (i) ⭐ …AND THE LATCH RE-ARMS ON **EITHER** OF ITS TWO CONDITIONS ──────────────
	// ⚖️ A latch that never cleared would be a pawn permanently unable to use a ladder — the
	// opposite defect, and a worse one because nothing logs it.
	{
		FSiegeLadderContactState LeavesRadius;
		FSiegeLadderContactState SteersAway;
		HoldContact(LeavesRadius, AtFoot, Inward, 1.f, bAscending);
		HoldContact(SteersAway, AtFoot, Inward, 1.f, bAscending);
		FSiegeLadderContactStatics::Disarm(LeavesRadius, /*bAtTop=*/ false);
		FSiegeLadderContactStatics::Disarm(SteersAway, /*bAtTop=*/ false);

		// Condition 1 — it walks out of the radius.
		HoldContact(LeavesRadius, FarFromFoot, Inward, FrameStepSeconds, bAscending);
		TestTrue(TEXT("(i) ✅ Leaving the radius re-arms the trigger — the pawn can climb again on its next approach"),
			HoldContact(LeavesRadius, AtFoot, Inward, 1.f, bAscending) == EContact::Climb);

		// Condition 2 — it steers out of the cone without leaving the radius.
		HoldContact(SteersAway, AtFoot, Outward, FrameStepSeconds, bAscending);
		TestTrue(TEXT("(i) ✅ Steering out of the cone re-arms it too — ⛔ the pawn is not trapped at the foot of its own ladder"),
			HoldContact(SteersAway, AtFoot, Inward, 1.f, bAscending) == EContact::Climb);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 12. ⛔⛔ THE CONTACT PATH RUNS THE **SAME** ENTRY GATE — SO IT IS NOT A BACK DOOR
//     AROUND T-3, AND THE TWO PATHS CANNOT DOUBLE-FIRE
//     (TASK-777, CONTACT-§4.3 / K-D / K-E, TOWER-§8.6, TOWER-§10 L-1)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⚠️⚠️ THIS IS THE TEST A REVIEWER WOULD HAVE ⛔ NO REASON TO LOOK FOR. A contact
// trigger that quietly skipped CanTeamAscend would overturn a JONATHAN RULING from
// inside a task whose stated purpose was something else entirely, and it would look
// perfectly correct in review.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerContactEntryGateTest,
	"Siegebound.ClimbableTower.ContactEntryStillRefusesEnemiesAndASecondClimber",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerContactEntryGateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;
	using EContact = AClimbableTower::ELadderContactVerdict;

	const FVector AtFoot = PinnedLadderFootRelative + FVector(-100.f, 0.f, ScenarioCapsuleHalfHeightUU);
	// ⚠️ −600, ⛔ not −300, for the same reason as test 11's copy: at `K-6`'s 350 uu radius a
	// pawn 300 uu out is 50 uu INSIDE the disc, which would have turned row (e)'s
	// "out-of-range enemy" into an in-range one and quietly deleted the ORDERING claim that
	// row exists to make.
	const FVector FarFromFoot = PinnedLadderFootRelative + FVector(-600.f, 0.f, ScenarioCapsuleHalfHeightUU);
	const FVector Inward(ScenarioWalkSpeedUU, 0.f, 0.f);
	const FVector Outward(-ScenarioWalkSpeedUU, 0.f, 0.f);

	// Steps the COMPOSED gate for one second — long enough that the dwell is never what
	// refuses a row below, so every refusal is attributable to the gate it names.
	const auto RunGate = [](FSiegeLadderContactState& State, const FVector& Location, const FVector& Velocity,
		ETeamId TowerTeam, ETeamId ClimberTeam, bool bIsUnit, bool bBusy, float TotalSeconds) -> EContact
	{
		EContact Verdict = EContact::TooFar;
		bool bAscending = false;
		const int32 Steps = FMath::Max(1, FMath::RoundToInt(TotalSeconds / FrameStepSeconds));
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Verdict = AClimbableTower::EvaluateContactEntry(State, Location, Velocity,
				PinnedLadderFootRelative, PinnedLadderTopRelative,
				PinnedContactRadiusUU, PinnedContactIntentCos, PinnedContactDwellSeconds, FrameStepSeconds,
				TowerTeam, ClimberTeam, bIsUnit, bBusy, bAscending);
		}
		return Verdict;
	};

	// ── SELF-CHECK: THE COMPOSED GATE IS NOT A CONSTANT ────────────────────────────────
	{
		FSiegeLadderContactState Friend;
		FSiegeLadderContactState Enemy;
		const EContact A = RunGate(Friend, AtFoot, Inward, ETeamId::Blue, ETeamId::Blue, true, false, 1.f);
		const EContact B = RunGate(Enemy, AtFoot, Inward, ETeamId::Blue, ETeamId::Red, true, false, 1.f);
		TestTrue(TEXT("SELF-CHECK: the composed gate is NOT a constant — a friend and an enemy in identical circumstances get different answers"),
			A != B);
		TestTrue(TEXT("SELF-CHECK: …and the friendly case really is Climb, so every refusal below refuses something that would otherwise have succeeded"),
			A == EContact::Climb);
	}

	// ── (a) ⛔⛔ AN ENEMY SATISFYING **ALL THREE** CONTACT TERMS IS STILL REFUSED ──────
	// ⭐ `CanTeamAscend` gains a SECOND CALLER here, ⛔ never an exception (CONTACT-§4.3).
	// Both team directions, so a hardcoded team branch cannot hide behind one of them.
	{
		FSiegeLadderContactState RedAtBlue;
		FSiegeLadderContactState BlueAtRed;
		TestTrue(TEXT("(a) ⛔ A RED pawn walking into a BLUE tower's ladder is WrongTeam — T-3 is not weakened by the new path"),
			RunGate(RedAtBlue, AtFoot, Inward, ETeamId::Blue, ETeamId::Red, true, false, 1.f) == EContact::WrongTeam);
		TestTrue(TEXT("(a) ⛔ …and the BLUE-at-RED mirror, so the gate is not one team's special case"),
			RunGate(BlueAtRed, AtFoot, Inward, ETeamId::Red, ETeamId::Blue, true, false, 1.f) == EContact::WrongTeam);
	}

	// ── (b) ⛔ TOWER-§10 L-1 — A SECOND PAWN IS REFUSED WHILE ONE IS CLIMBING ──────────
	// ⭐⭐ AND THIS IS ALSO HALF OF `K-E`'s "THE TWO PATHS CANNOT DOUBLE-FIRE", ASSERTED
	// RATHER THAN PROMISED: the contact path reads the SAME single occupancy slot the
	// nav-link path does, so whichever arrives second is LadderBusy.
	{
		FSiegeLadderContactState Second;
		TestTrue(TEXT("(b) ⛔ An own-team pawn pressing into an OCCUPIED ladder is LadderBusy — it waits in its ordinary walking state (K-D)"),
			RunGate(Second, AtFoot, Inward, ETeamId::Blue, ETeamId::Blue, true, true, 1.f) == EContact::LadderBusy);
	}

	// ── (c) ⭐ THE OTHER HALF OF THE NO-DOUBLE-FIRE CLAIM, AS AN INDEPENDENT BELT ──────
	// Even if a gate were bypassed, the climber's own admission predicate refuses a unit
	// that is already climbing. ⛔ Two belts, ⛔ not one restated twice.
	{
		FSiegeLadderClimbState Active;
		Active.bActive = true;
		TestFalse(TEXT("(c) ⭐ FSiegeLadderClimbStatics::CanBegin refuses an ALREADY-CLIMBING pawn — the second, independent belt behind K-E's no-double-fire claim"),
			FSiegeLadderClimbStatics::CanBegin(Active, false, false, false, PinnedLadderFootRelative, PinnedLadderTopRelative));
		TestTrue(TEXT("SELF-CHECK: …and it ADMITS the same line for a pawn that is not climbing, so the refusal above is about bActive and not about the line"),
			FSiegeLadderClimbStatics::CanBegin(FSiegeLadderClimbState(), false, false, false, PinnedLadderFootRelative, PinnedLadderTopRelative));
	}

	// ── (d) ⛔ IDENTITY OUTRANKS TEAM AND OCCUPANCY, EXACTLY AS ON THE LINK PATH ───────
	// ⚖️⭐ THIS ROW USED TO SAY *"the verdict `AHeroCharacter` lands on until the identity term
	// widens"*. ⭐ IT DID WIDEN (TASK-787, `CONTACT-§12`) — and ⛔ BOTH HALVES SHIPPED TOGETHER,
	// which is the only way it may be closed: a slot claimed for a climber that could be STARTED
	// but never HEARD FROM would brick the ladder for the rest of the match, which is ⛔ worse than
	// the refusal it replaced. ⇒ the hero is now ADMITTED (test 15), and this row keeps the claim
	// that ⛔ SURVIVES the widening: a pawn implementing ⛔ nothing is refused, and refused ⛔ BY
	// IDENTITY — before team, before occupancy.
	{
		FSiegeLadderContactState NotAClimber;
		TestTrue(TEXT("(d) ⛔ A pawn that is not an admitted climber is NotAnAdmittedClimber even at a free own-team ladder — identity outranks both other terms"),
			RunGate(NotAClimber, AtFoot, Inward, ETeamId::Blue, ETeamId::Blue, false, true, 1.f) == EContact::NotAnAdmittedClimber);
	}

	// ── (e) ⭐⭐ THE ORDER IS PART OF THE CONTRACT: CONTACT TERMS FIRST, ENTRY GATE SECOND
	// A pawn that is out of range is TooFar, ⛔ NOT WrongTeam — even when it is an enemy at a
	// busy ladder and both of those refusals would also apply.
	{
		FSiegeLadderContactState Distant;
		TestTrue(TEXT("(e) ⭐ An out-of-range enemy at a BUSY ladder is TooFar — the contact terms are evaluated first, and that ordering is load-bearing"),
			RunGate(Distant, FarFromFoot, Inward, ETeamId::Blue, ETeamId::Red, true, true, 1.f) == EContact::TooFar);
	}

	// ── (f) ⛔⛔ …AND HERE IS **WHY** THAT ORDER IS LOAD-BEARING, AS A FAILING-ABLE ROW ─
	// ⚠️⚠️ If the entry gate ran first, a pawn latched by K-C while SOMEBODY ELSE held the
	// ladder would be short-circuited out on LadderBusy every single frame, never observed
	// leaving the cone, and would carry that latch FOR THE REST OF THE MATCH — a pawn
	// permanently unable to climb, for a reason nothing logs. This row walks exactly that
	// sequence and requires the latch to have cleared.
	{
		FSiegeLadderContactState State;
		RunGate(State, AtFoot, Inward, ETeamId::Blue, ETeamId::Blue, true, false, 1.f);
		FSiegeLadderContactStatics::Disarm(State, /*bAtTop=*/ false);

		// The whole re-arm happens while the ladder is BUSY — the case that would be
		// short-circuited away under the tidier ordering.
		TestTrue(TEXT("(f) SELF-CHECK: while latched and pressing in, the verdict is Disarmed — so the clear below is a real state change"),
			RunGate(State, AtFoot, Inward, ETeamId::Blue, ETeamId::Blue, true, true, 0.5f) == EContact::Disarmed);
		RunGate(State, AtFoot, Outward, ETeamId::Blue, ETeamId::Blue, true, true, FrameStepSeconds);

		// ⚠️ WORDED PRECISELY: the re-arm frames returned Disarmed then NotHeadingIn, ⛔ NOT
		// LadderBusy — because the contact terms short-circuit BEFORE the entry gate. That IS
		// the property under test: under the gate-first ordering those same frames WOULD have
		// returned LadderBusy, the latch would never have been touched, and this row goes red.
		TestTrue(TEXT("(f) ⭐⭐ The latch cleared across frames the ladder was OCCUPIED for — under a gate-first ordering those frames would have short-circuited as LadderBusy and the latch would have survived the match"),
			RunGate(State, AtFoot, Inward, ETeamId::Blue, ETeamId::Blue, true, false, 1.f) == EContact::Climb);
	}

	// ── (g) ⛔ THE THREE FEEL NUMBERS SHIP AS EditDefaultsOnly FLOATS AT THEIR LAW VALUES ──
	// ⚠️ Expectations typed from `CONTACT-§7` — the RADIUS from `K-6`, the cone and dwell
	// from `K-5` — ⛔ never read back off the class, the same direction the pinned socket
	// coordinates are typed in. 🧑 The cone and dwell are DECLARED INVENTED and the feel is
	// Jonathan's; when he retunes one, this row fails and NAMES the drift rather than letting
	// a test quietly mean something else.
	//
	// ⭐⭐ THIS PIN WAS DELIBERATELY LEFT **ARMED AND RED** BY TASK-785, WHICH DID NOT OWN THE
	// RADIUS: rather than weaken a test for a change outside its fence, it let the row stand
	// and fail. TASK-784 then shipped `LadderContactRadiusUU = 300.f`, and TASK-786 moves the
	// pin, its label and the citation TOGETHER — ⛔ never one without the others, because a
	// pin whose LABEL still cites the superseded law is worse than no pin at all.
	// ⛔ AND IT IS STILL TYPED FROM THE LAW, ⛔ never read back off the CDO: a row that read
	// the class and compared it to itself would pass forever and mean nothing. That is the
	// whole reason `SiegeLadderClimbTest.cpp` DERIVES the radius instead of re-pinning it —
	// the two files deliberately approach the same number from opposite directions.
	{
		const AClimbableTower* const TowerDefaults = GetDefault<AClimbableTower>();
		UClass* const ClimbableTowerClass = AClimbableTower::StaticClass();
		if (!TowerDefaults || !ClimbableTowerClass)
		{
			AddError(TEXT("SELF-CHECK FAILED: the AClimbableTower class default object returned null."));
			return false;
		}

		float RadiusUU = 0.f;
		float IntentCos = 0.f;
		float DwellSeconds = 0.f;
		const bool bReadRadius = TryReadDefaultFloat(ClimbableTowerClass, TowerDefaults, TEXT("LadderContactRadiusUU"), RadiusUU);
		const bool bReadIntent = TryReadDefaultFloat(ClimbableTowerClass, TowerDefaults, TEXT("LadderContactIntentCos"), IntentCos);
		const bool bReadDwell = TryReadDefaultFloat(ClimbableTowerClass, TowerDefaults, TEXT("LadderContactDwellSeconds"), DwellSeconds);

		// ⛔ A missing property is a HARD ERROR and ⛔ never a substituted guess: the whole
		// mechanism is unreachable if the tower stopped shipping its own tuning.
		if (!bReadRadius || !bReadIntent || !bReadDwell)
		{
			AddError(TEXT("⛔ One of LadderContactRadiusUU / LadderContactIntentCos / LadderContactDwellSeconds did not resolve as a float UPROPERTY on AClimbableTower — WR-§5 puts the radius and the test on the OWNING actor, and the contact trigger cannot be tuned without them."));
			return false;
		}

		TestEqual(TEXT("(g) ⭐⭐ LadderContactRadiusUU ships at K-6's 350 uu — the HERO's per-frame requirement (0.35 s × 937.5 uu/s sprint+Boots = 328.125) plus 6.67% margin, which BINDS ABOVE the unit roster's 2 × 0.25 × 600 = 300. ⛔ At 300 the fastest hero could never climb at any offset or frame rate; at 150 the trigger fired for only part of the roster and only on part of the sampling phases"),
			RadiusUU, PinnedContactRadiusUU, Tolerance);
		TestEqual(TEXT("(g) LadderContactIntentCos ships at K-5's 0.5 (a 60° half-cone)"), IntentCos, PinnedContactIntentCos, Tolerance);
		TestEqual(TEXT("(g) ⭐ LadderContactDwellSeconds ships at K-5's 0.35 s — the term that stops a unit being abducted as it marches past its own tower"),
			DwellSeconds, PinnedContactDwellSeconds, Tolerance);

		// ⭐⭐ AND THE TUNING IS THE **TOWER'S**, WHICH IS THE HALF `WR-§5` IS ABOUT: the
		// radius lives on the owning actor and the pawn only asks. ⛔ A second copy on
		// ASummonedUnit would be two numbers that eventually disagree.
		TArray<FString> UnitMemberNames;
		CollectDeclaredMemberNames(ASummonedUnit::StaticClass(), UnitMemberNames);
		TestTrue(TEXT("SELF-CHECK: the reflection walk over ASummonedUnit is live (it found the shipped LadderClimbSpeedUU)"),
			UnitMemberNames.Contains(TEXT("LadderClimbSpeedUU")));
		for (const FString& MemberName : UnitMemberNames)
		{
			if (MemberName.Contains(TEXT("LadderContact"), ESearchCase::IgnoreCase))
			{
				AddError(*FString::Printf(
					TEXT("⛔ ASummonedUnit declares '%s'. The contact trigger's radius, cone and dwell live on the OWNING TOWER and the pawn only ASKS (WR-§5, third application — ACommanderNpc::IsPlayerInRange reading InteractRadius). A second copy on the pawn is two tuning numbers that eventually disagree."),
					*MemberName));
			}
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 14. ⭐⭐ THE TOWER NAMES ⛔ NO CONCRETE CLIMBER CLASS ON THE CLIMB PATH
//     (TASK-787, `CONTACT-§12.2` / `§12.3` / `§12.4` row 1)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⚖️⭐ THE PROPERTY `CONTACT-§4.4` WAS CREATED TO BUY AND DID ⛔ NOT FINISH BUYING. TASK-777
// widened the occupancy SLOT to `ACharacter` and reached the climber through `ILadderClimber` in
// `EndPlay` — correctly — but the ENTRY GATE still asked `Cast<ASummonedUnit>`, the START still
// went through `CastChecked<ASummonedUnit>`, and the completion lane was bound through a delegate
// that existed on that one class. ⇒ a COMPLETE, TESTED hero climb (TASK-778) could ⛔ never fire.
//
// ⛔⛔ AND THE ONE THAT LOOKED INNOCENT IS THE ONE THAT MATTERED MOST: a SINGLE
// `Cast<ASummonedUnit>` guarding the bind/unbind pair, with a comment explaining it was *"not a
// class-list branch"*. `CONTACT-§12.4` row 1 names that exactly — ***the class list with one entry
// hidden***. ⇒ this test scans for ⛔ ALL of them, ⛔ including that one.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerNoConcreteClimberClassTest,
	"Siegebound.ClimbableTower.TheClimbPathNamesNoConcreteClimberClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerNoConcreteClimberClassTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;

	FString TowerSource;
	if (!LoadClimbableTowerCpp(*this, TowerSource))
	{
		return false;
	}

	// ── SELF-CHECK: THE SCANNER CAN FIND A CAST IN THIS FILE AT ALL ───────────────────
	// ⚠️⚠️ ⛔ NOT OPTIONAL. Every row below is an ABSENCE, and an absence over a file that failed
	// to load, or over a scanner that matches nothing, is the vacuous pass this whole file is
	// written against. The tower is FULL of interface casts now — requiring at least two proves
	// the instrument is live on the exact syntax it is about to report missing.
	const int32 InterfaceCasts = CountOccurrencesInCode(TowerSource, TEXT("Cast<ILadderClimber>"));
	TestTrue(TEXT("SELF-CHECK: the scanner finds Cast<ILadderClimber> in ClimbableTower.cpp — so the absences below are MEASUREMENTS, ⛔ not a broken probe"),
		InterfaceCasts >= 2);

	// ── (a) ⛔ ZERO CONCRETE CLIMBER CASTS, IN EVERY SPELLING ─────────────────────────
	// ⭐ `CastChecked` is listed separately and deliberately: the START used exactly that form,
	// and a scan for `Cast<` alone would ⛔ miss it (it is a different identifier, not a prefix
	// match — `CastChecked<X>` does not contain the substring `Cast<`).
	static const TCHAR* const RefusedCastForms[] =
	{
		TEXT("Cast<ASummonedUnit>"),
		TEXT("CastChecked<ASummonedUnit>"),
		TEXT("Cast<AHeroCharacter>"),
		TEXT("CastChecked<AHeroCharacter>")
	};

	for (const TCHAR* const Form : RefusedCastForms)
	{
		TestEqual(*FString::Printf(
			TEXT("(a) ⛔ ClimbableTower.cpp contains ZERO '%s' — CONTACT-§12.4 row 1: a concrete climber cast is the class list, and ONE hidden entry is what made a finished hero climb unable to fire"),
			Form),
			CountOccurrencesInCode(TowerSource, Form), 0);
	}

	// ── (b) ⛔ AND NO CONCRETE CLIMBER HEADER IS EVEN INCLUDED ────────────────────────
	// ⭐ THE STRUCTURAL VERSION OF (a), AND IT IS STRICTLY STRONGER: without the include, a
	// `Cast<ASummonedUnit>` cannot be re-introduced without ALSO re-introducing the coupling —
	// which is a visible, reviewable line rather than a quiet one buried mid-function.
	// ⚠️ `TOWER-§8.4(B)` declared that include as this redesign's ONE new coupling surface. It is
	// ⛔ not denied here; it is NARROWED to a capability interface (TASK-787).
	TestEqual(TEXT("(b) ⛔ ClimbableTower.cpp no longer includes SummonedUnit.h — the ONE declared coupling surface is now the ILadderClimber seam, and ⛔ nothing else"),
		CountOccurrencesInCode(TowerSource, TEXT("#include \"Siegebound/SummonedUnit.h\"")), 0);
	TestEqual(TEXT("(b) ⛔ …and it never included HeroCharacter.h — the widening admits the hero WITHOUT the tower learning that class exists"),
		CountOccurrencesInCode(TowerSource, TEXT("#include \"Siegebound/HeroCharacter.h\"")), 0);
	TestEqual(TEXT("(b) SELF-CHECK: it DOES include the capability seam — so (b) is about which header, ⛔ not about a scanner that cannot see includes"),
		CountOccurrencesInCode(TowerSource, TEXT("#include \"Siegebound/LadderClimber.h\"")), 1);

	// ── (c) ⭐ BOTH ENTRY PATHS START THE CLIMB THROUGH THE INTERFACE ─────────────────
	// ⛔ TWO paths (the nav-link callback and the contact poll), ⛔ ONE rule and ⛔ ONE seam. A
	// widening that reached only one of them would leave the OTHER admitting units only, which is
	// the same defect with a smaller blast radius.
	TestEqual(TEXT("(c) ⭐ EXACTLY TWO BeginLadderClimb call sites — the link path and the contact path — and BOTH go through the ILadderClimber pointer the identity term was derived from"),
		CountOccurrencesInCode(TowerSource, TEXT("ClimberApi->BeginLadderClimb(")), 2);

	// ── (d) ⭐⭐ THE COMPLETION LANE IS BOUND **AND** UNBOUND THROUGH THE ACCESSOR ─────
	// ⚠️⚠️ THE ASYMMETRIC CASE IS THE DANGEROUS ONE, AND IT IS WHY BOTH SIDES ARE COUNTED: a bind
	// through the interface with an unbind still keyed to a concrete class would leave the tower
	// subscribed to a hero it had already released — and the next climb would fire a handler for
	// the WRONG climber.
	TestEqual(TEXT("(d) ⭐ two binds through GetOnLadderClimbEnded() — one per entry path"),
		CountOccurrencesInCode(TowerSource, TEXT("GetOnLadderClimbEnded().AddUniqueDynamic(")), 2);
	TestEqual(TEXT("(d) ⭐ and ONE unbind through the same accessor, in the ONE release path both entry paths and EndPlay funnel into"),
		CountOccurrencesInCode(TowerSource, TEXT("GetOnLadderClimbEnded().RemoveDynamic(")), 1);
	TestEqual(TEXT("(d) ⛔ and ZERO direct member accesses left — `->OnLadderClimbEnded.` would be a concrete-class reach in interface clothing"),
		CountOccurrencesInCode(TowerSource, TEXT("->OnLadderClimbEnded.")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// 15. ⭐⭐⭐ THE SLOT IS **RELEASED** FOR A NON-UNIT CLIMBER — ⛔ THE LADDER IS NOT
//     BRICKED (TASK-787, `CONTACT-§12.1` / `§12.3` / `§12.5`)
// ═══════════════════════════════════════════════════════════════════════════════
//
// ⛔⛔ THE SENTENCE THIS TEST EXISTS FOR: **A START-ONLY WIDENING IS WORSE THAN NO WIDENING AT
// ALL.** It converts *"the hero cannot climb"* into *"the FIRST hero attempt disables the tower
// for EVERYONE — hero and unit alike — for the rest of the match."* The tower learns a climb ended
// through ⛔ exactly one channel, so a climber it can START but not HEAR FROM holds the single
// occupancy slot forever and every later admission returns `LadderBusy`.
//
// ⭐ WHAT IS PROVEN HERE AND WHAT IS ⛔ NOT (`SC-§32`, stated rather than implied): the RELEASE
// CYCLE is proven as a rule (the pure gate), the MECHANISM that drives it is proven as a shape
// (the delegate both pawns share, and the belt behind it). ⛔ The live sequence — admit → end →
// admit again on a real tower — needs a world, a spawned tower and a spawned pawn, which this
// suite may ⛔ not create (see this file's header on why faking it would crash rather than catch).
// ⇒ that row is TASK-780's PIE session, and it is boarded there in exactly those words.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeClimbableTowerSlotReleaseTest,
	"Siegebound.ClimbableTower.TheSlotIsReleasedForANonUnitClimberSoTheLadderIsNotBricked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeClimbableTowerSlotReleaseTest::RunTest(const FString& Parameters)
{
	using namespace SiegeClimbableTowerTestFixture;
	using EVerdict = AClimbableTower::ELadderEntryVerdict;

	// ── (a) ⭐⭐ THE HEADLINE: AN ADMITTED CLIMBER THAT IS **NOT** AN `ASummonedUnit` CLIMBS ──
	// ⚠️ THE IDENTITY ARGUMENT IS A `bool` AND THAT IS THE POINT OF THE SPLIT: this row asserts
	// the RULE — "an admitted climber on a free own-team ladder gets `Climb`, ⛔ never
	// `NotAnAdmittedClimber`" — while test 14 asserts the DERIVATION, i.e. that the caller now
	// computes that bool from `Cast<ILadderClimber>` rather than from a concrete class. ⛔ Neither
	// half is the claim on its own; together they are.
	TestTrue(TEXT("(a) ⭐⭐ an admitted climber (the widened ILadderClimber term — a HERO qualifies) is admitted to a free own-team ladder: Climb, ⛔ NOT NotAnAdmittedClimber"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, /*bClimberIsAdmittedClimber=*/ true, /*bBusy=*/ false) == EVerdict::Climb);

	// ⭐ AND THE HERO REALLY DOES SATISFY THAT TERM — asserted against the reflection tables, so
	// this is ⛔ not an argument about a bool: `AHeroCharacter` implements the interface the
	// caller casts to, which is what makes row (a) about the player's own body.
	TestTrue(TEXT("(a) ⭐ AHeroCharacter implements ILadderClimber — so the widened identity term admits the PLAYER'S OWN BODY, ⛔ not merely 'some hypothetical climber'"),
		AHeroCharacter::StaticClass()->ImplementsInterface(ULadderClimber::StaticClass()));
	TestTrue(TEXT("(a) SELF-CHECK: …and so does ASummonedUnit, so the widening ADDED a class rather than swapping one for another"),
		ASummonedUnit::StaticClass()->ImplementsInterface(ULadderClimber::StaticClass()));

	// ── (b) ⛔⛔ THE RELEASE CYCLE — **THE BRICK SEQUENCE** ───────────────────────────────
	// Admit → the slot is HELD → the climb ENDS → a SECOND climber is admitted.
	//
	// ⚠️⚠️ SAY WHAT THIS DOES AND DOES ⛔ NOT PROVE, BECAUSE ROW ③ CALLS THE SAME EXPRESSION AS ROW
	// ① AND A READER WHO MISSES THAT WILL READ IT AS A TAUTOLOGY — IT IS ⛔ NOT ONE, BUT ⛔ ONLY
	// BECAUSE OF WHAT IT ASSERTS: ⭐ **THE GATE CARRIES ⛔ NO STATE ACROSS CALLS.** A refusal does
	// ⛔ not latch, and "this pawn was once refused" is ⛔ not remembered. ⇒ ③ goes RED the day
	// somebody gives `EvaluateLadderEntry` a cache, a cooldown, or a "has been refused" flag —
	// which is one of the two ways a ladder gets bricked, the other being a slot that never clears.
	// ⛔ WHAT IT DOES ⛔ NOT PROVE, and it is proven elsewhere rather than left implied: that the
	// tower's `bLadderOccupied` argument actually GOES back to false. That is the completion
	// delegate's job — rows (c)/(d) — and the live sequence is TASK-780's PIE row.
	TestTrue(TEXT("(b) ① the first climber is ADMITTED to a free ladder"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, true, /*bBusy=*/ false) == EVerdict::Climb);
	TestTrue(TEXT("(b) ② while it holds the slot, a SECOND climber is refused — TOWER-§10 L-1, one ladder one climber"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, true, /*bBusy=*/ true) == EVerdict::LadderBusy);
	TestTrue(TEXT("(b) ③ ⭐⭐ AND THE REFUSAL DOES ⛔ NOT LATCH: with the slot free again the very next admission is Climb. The gate is PURE — it remembers ⛔ nothing about ②, so a released ladder is usable on the NEXT attempt, ⛔ not the next match"),
		AClimbableTower::EvaluateLadderEntry(ETeamId::Blue, ETeamId::Blue, true, /*bBusy=*/ false) == EVerdict::Climb);

	// ── (c) ⭐ THE MECHANISM BEHIND ③ — **ONE DELEGATE TYPE, REACHED THROUGH THE INTERFACE** ──
	// ⚠️⚠️ THIS IS THE ROW THAT MAKES ③ MORE THAN ARITHMETIC. `bBusy=false` in ③ is only reachable
	// in the shipped game if the tower is actually TOLD the climb ended — and it is told through
	// `ILadderClimber::GetOnLadderClimbEnded()`, which returns the implementer's OWN instance.
	// ⇒ if the hero broadcast a DIFFERENT delegate type, `AddUniqueDynamic` would be binding to
	// something the hero never fires, and ③ would be unreachable in the world while staying green
	// here. Pointer identity on the signature function is what forbids that.
	const FMulticastDelegateProperty* const HeroClimbEnded =
		CastField<FMulticastDelegateProperty>(AHeroCharacter::StaticClass()->FindPropertyByName(FName(TEXT("OnLadderClimbEnded"))));
	const FMulticastDelegateProperty* const UnitClimbEnded =
		CastField<FMulticastDelegateProperty>(ASummonedUnit::StaticClass()->FindPropertyByName(FName(TEXT("OnLadderClimbEnded"))));

	TestNotNull(TEXT("(c) ⭐ the HERO owns a completion delegate instance — TASK-787 added it; before that this class had NONE and could not release the slot at all"),
		HeroClimbEnded);
	TestNotNull(TEXT("(c) SELF-CHECK: the UNIT still owns its own — this row compares two live properties"),
		UnitClimbEnded);

	if (HeroClimbEnded && UnitClimbEnded)
	{
		TestTrue(TEXT("(c) ⛔⛔ both pawns' delegates are the SAME type (FSiegeLadderClimbEnded, now in LadderClimber.h) — ⛔ NOT two lookalikes. A second, hero-only delegate type is REFUSED (CONTACT-§12.4) precisely because the tower binds ONE handler"),
			HeroClimbEnded->SignatureFunction == UnitClimbEnded->SignatureFunction);
		// ⚠️ `FMulticastDelegateProperty::SignatureFunction` is a `TObjectPtr<UFunction>`, and both
		// `TestNotNull` overloads DEDUCE `const ValueType*`. ⛔ Template argument deduction does NOT
		// run TObjectPtr's implicit conversion-to-raw, so the value must already BE a raw pointer at
		// the call site (the `==` row above and `operator->` below are unaffected — neither deduces).
		// ⭐ Same idiom as `SiegeLadderClimbTest.cpp:1061`, which asserts non-null on this very field.
		// ⛔ The row itself is UNCHANGED: it still fails on a null, which is the whole point of it.
		const UFunction* const SharedSignature = HeroClimbEnded->SignatureFunction;
		TestNotNull(TEXT("(c) SELF-CHECK: the shared signature function exists — comparing two nulls would 'pass' while proving nothing"),
			SharedSignature);
	}

	// ── (d) ⭐ THE HERO'S HALF OF THE RELEASE, READ IN THE SHIPPED SOURCE ────────────────
	// ⛔ Reflection cannot see a Broadcast. The hero's ordering (last, after the restore, exactly
	// once, from the ONE teardown all ten exits route through) is asserted in full by
	// `SiegeHeroLadderClimbTest` test 24; this row is the cheapest possible check that the SITE
	// exists at all, so a reader of THIS file is not left assuming it.
	{
		FString HeroSource;
		if (LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp"), HeroSource))
		{
			TestEqual(TEXT("(d) ⭐ the hero broadcasts the completion signal EXACTLY ONCE in its whole implementation — ten exits, ONE teardown, ONE signal, and that is what frees this tower's slot"),
				CountOccurrencesInCode(HeroSource, TEXT("OnLadderClimbEnded.Broadcast(")), 1);
		}
	}

	// ── (e) ⭐ THE INDEPENDENT BELT (`CONTACT-§12.5`) — ⛔ A BELT, ⛔ NOT THE MECHANISM ────
	// The occupancy term additionally requires the held climber to still be CLIMBING, read through
	// `ILadderClimber::IsClimbing()`. ⇒ a completion signal that is ever MISSED degrades the worst
	// case from "the tower is dead for the match" to "one admission is late by one poll".
	//
	// ⛔⛔ IT IS ⛔ NOT A SUBSTITUTE FOR (c)/(d) AND MUST NEVER BE SHIPPED INSTEAD OF THEM: without
	// the eager release the tower keeps a STALE `ActiveClimber`, and the two pawn classes would get
	// DIVERGENT release semantics — two mechanisms for one traversal, which is what `CONTACT-§2`
	// refused for the driver. ⇒ TWO independent mechanisms, TWO independent assertions.
	{
		FString TowerSource;
		if (LoadClimbableTowerCpp(*this, TowerSource))
		{
			TestEqual(TEXT("(e) ⭐ the occupancy term is spelled ONCE, in IsLadderSlotOccupied() — ⛔ not copied per entry path, because two copies is how the link path and the contact path come to disagree about when a ladder is free"),
				CountOccurrencesInCode(TowerSource, TEXT("bool AClimbableTower::IsLadderSlotOccupied() const")), 1);
			TestEqual(TEXT("(e) ⭐ and BOTH entry paths feed the gate from it"),
				CountOccurrencesInCode(TowerSource, TEXT("IsLadderSlotOccupied()")), 3); // the definition + two call sites
			TestTrue(TEXT("(e) ⭐ the belt asks the HELD climber whether it is still climbing, through the interface — ⛔ the slot alone is not the whole term"),
				TowerSource.Contains(TEXT("HeldApi->IsClimbing()"), ESearchCase::CaseSensitive));
			// ⛔⛔ AND THE EAGER PATH IS STILL THERE — the belt did ⛔ NOT replace it. TWO clears:
			// `ReleaseClimber`'s (every completion, both entry paths) and `EndPlay`'s (the tower
			// dying under a climber, TOWER-§10 L-5). ⚖️ Shipping the belt INSTEAD of the completion
			// seam is ruled out by CONTACT-§12.5's closing clause, and this row is what would go
			// red if a future task "simplified" it that way.
			TestEqual(TEXT("(e) ⛔ the eager release is INTACT: ActiveClimber is cleared in EXACTLY TWO places — ReleaseClimber (every completion) and EndPlay (L-5). The belt is a SECOND mechanism, ⛔ never a replacement for the first"),
				CountOccurrencesInCode(TowerSource, TEXT("ActiveClimber.Reset()")), 2);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
