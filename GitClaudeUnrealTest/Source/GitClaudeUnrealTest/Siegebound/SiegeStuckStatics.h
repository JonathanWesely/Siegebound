// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// FVector (Math/Vector.h) and FMath (Math/UnrealMathUtility.h) arrive complete through
// CoreMinimal, and they are the ONLY types this header names. That is deliberate: there is
// no UWorld, no AActor, no AAIController, no UPathFollowingComponent anywhere in this pair,
// which is what lets TASK-536 exercise the whole ladder in a headless automation test with
// no PIE session and no navmesh (complete-type include law, TASK-110).
#include "CoreMinimal.h"

#include "SiegeStuckStatics.generated.h"

/**
 *  ═══ Siegebound stuck-unit ladder statics (TASK-531, UNIT-PATHING batch) ═══
 *
 *  Jonathan: *"There are many times when a unit recieves a command, and on the way to it, it
 *  gets stuck behind a rock."* CONVENTIONS `NAV-§1` Cause 2 names the mechanism, and it is
 *  NOT a missing path calculator — every destination in this project already goes through
 *  MoveToActor / MoveToLocation with bUsePathfinding = true.
 *
 *  ⭐ THE ENGINE ALREADY KNOWS THE UNIT IS STUCK AND NOBODY HAS EVER ASKED.
 *  UPathFollowingComponent's shipped defaults (PathFollowingComponent.cpp:136-144 —
 *  BlockDetectionDistance = 10, BlockDetectionInterval = 0.5, BlockDetectionSampleCount = 10)
 *  declare EPathFollowingResult::Blocked after a unit moves < 10 uu in 5.0 s. But there is no
 *  OnMoveCompleted override anywhere in this project and zero references to
 *  EPathFollowingResult, so the verdict is DISCARDED — and 0.25 s later SummonedUnit.cpp:2440
 *  re-issues the BYTE-IDENTICAL request: same start poly, same goal, same filter, same path,
 *  same rock. A silent five-second livelock with no telemetry. This library is the decision
 *  that breaks it.
 *
 *  ⛔ EVERYTHING HERE IS PURE. No UWorld, no AActor, no UObject, no engine singleton, no
 *  allocation, no logging, no RNG, no clock read. Every input is a parameter and every output
 *  is a return value or a write to the caller's FSiegeStuckState. Purity is not style here:
 *  it is what makes the cost argument in `NAV-§3` CHECKABLE rather than asserted, and it is
 *  what lets TASK-536 test the ladder headlessly.
 *
 *  Not a UObject / not reflected: a plain static library, so there is no BeginPlay, no GC
 *  surface, and NO Build.cs change — this pair adds no dependency at all. Precedents:
 *  FSiegeCombatStatics (SiegeCombatStatics.h:23), FSiegeKeyboardLayoutStatics
 *  (SiegeKeyboardLayoutStatics.h:117).
 *
 *  ⚠️ ESiegeStuckAction, FSiegeStuckState and FSiegeStuckTuning SHARE THIS HEADER ON PURPOSE.
 *  They are pure data types forming ONE concept with the statics that consume them — the
 *  standing TeamId.h exception to one-class-per-header, restated for this batch in `NAV-§7`
 *  ("⛔ QA must not flag it as a one-class-per-header violation").
 *
 *  ⛔ EVERY SIGNATURE, FIELD, TYPE, ORDER AND DEFAULT BELOW IS PINNED CHARACTER-FOR-CHARACTER
 *  IN CONVENTIONS `NAV-§8`. UBT compiles the whole module and TASK-532 (ASummonedUnit),
 *  TASK-533 (AMinerUnit) and TASK-536 (the tests) link against this list — "improving" a
 *  shape here breaks another agent's build and is an automatic QA FAIL. Change the pin first,
 *  or not at all.
 *
 *  M8 DECLARATION (verbatim, `NAV-§11`): adds no replicated property, no new replicated
 *  class, no new relevancy tier. FSiegeStuckState is deliberately UNREFLECTED, so it cannot
 *  be replicated by accident — which is the property that makes this declaration survive a
 *  later refactor instead of merely being true today. FSiegeStuckTuning IS reflected, but it
 *  is CONFIG on the CDO (EditDefaultsOnly), identical on every machine by construction.
 *
 *  QA gate: TASK-537 (`.claude/pipeline/qa/TASK-537.md`).
 */

/**
 *  Stuck-lane log category: the harvested engine verdict (`blocked:`) and each fired rung
 *  (`escalate:` with `level=` / `action=` / `stalled=`), per the pinned tokens in `NAV-§7`.
 *  Defined in SiegeStuckStatics.cpp.
 *
 *  ⛔ DECLARED HERE, USED BY THE WIRING TASKS — NOT BY THIS LIBRARY. Nothing in
 *  FSiegeStuckStatics logs: a UE_LOG on a per-unit 0.25 s poll at 120 units is exactly the
 *  cost this feature claims not to have, and Evaluate is required to be side-effect-free
 *  apart from the State it is handed. TASK-532/533/534 own every call.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeStuck, Log, All);

/**
 *  What the ladder decided this poll. Plain enum — NOT a UENUM; it is never a UPROPERTY.
 *
 *  The rung table is law (`NAV-§3`); the MECHANISM each rung uses belongs to the call site:
 *    None           nothing to do. The overwhelmingly common answer.
 *    Sidestep       (1.5 s) one move to ComputeSidestepGoal(...), and take the lease.
 *    WidenAndRepath (3.0 s) re-issue the ORIGINAL goal ONCE with a genuine re-path.
 *    Abandon        (6.0 s) drop the move goal, log once, let the standing body re-choose.
 *
 *  ⛔ No rung may ever cancel the unit's standing ORDER, enter Attack, or leave the unit
 *  inert forever (`NAV-§3` rung table).
 */
enum class ESiegeStuckAction : uint8 { None, Sidestep, WidenAndRepath, Abandon };

/**
 *  Per-unit transient stall state. POD, ~32 bytes. ⛔ NOT a USTRUCT — it is runtime
 *  state, and reflecting it invites somebody to replicate it (NAV-§11).
 *
 *  Owned BY VALUE by the unit (ASummonedUnit::StuckState, `NAV-§8`) and passed by reference
 *  to Evaluate / Reset. The defaults below ARE the cleared state: Reset assigns a
 *  default-constructed instance, so this declaration is the single source of truth for what
 *  "cleared" means and the two can never drift apart.
 *
 *  ⚠️ bHasAnchor is the reason ProgressAnchor's ZeroVector default is harmless. A freshly
 *  constructed (or freshly Reset) state has NO anchor, so the first Evaluate always takes the
 *  re-anchor branch and the world origin is never once used as a comparison point.
 */
struct FSiegeStuckState
{
	FVector ProgressAnchor         = FVector::ZeroVector;
	float   StalledSeconds         = 0.f;
	float   SecondsSinceEscalation = 0.f;
	uint8   EscalationLevel        = 0;
	bool    bHasAnchor             = false;
};

/**
 *  ⭐ MANAGER ADDITION OVER THE PLAN, DECLARED NOT SILENT (SC-§15): the plan wrote a plain
 *  struct; this is a USTRUCT so the rungs are EditDefaultsOnly per-unit-class and Jonathan
 *  can tune the feel WITHOUT a recompile. The pinned FUNCTION signatures are unchanged.
 *
 *  ⚠️ EVERY FIELD HERE IS A FEEL VALUE AND JONATHAN'S PLAYTEST IS THE REAL GATE (`NAV-§7`
 *  tunables row) — these are the ONLY tunables this feature adds. ⛔ No bare literal for any
 *  of them at a call site.
 *
 *  ⛔ AND THE LADDER CANNOT MILL UNDER ARBITRARY VALUES, WHICH IS WHY IT CAN BE
 *  EditDefaultsOnly AT ALL. Evaluate's two brakes are independent but they bound DIFFERENT
 *  THINGS: EscalationCooldown caps fires in TIME (the rate brake), EscalationLevel caps them
 *  in COUNT PER STALL (three rungs, of which only two issue a request). Thresholds out of
 *  order, equal, zero, negative or NaN cannot produce a re-path mill.
 *
 *  ⚠️ ⛔ BUT BRAKE 2 IS NOT A RATE BRAKE, AND AN EARLIER REVISION OF THIS COMMENT WRONGLY
 *  SAID "EITHER ALONE bounds the request rate" (corrected per qa/TASK-537.md WARN-1). A
 *  stall's DURATION is itself tunable, so the count-per-stall bound alone permits those two
 *  requests to recur as fast as the thresholds allow. ⭐ `NAV-§3`'s <= 1 extra path request
 *  per unit per second rests on EscalationCooldown >= 1.0 ALONE. Worst case over legal
 *  tuning (EscalationCooldown = 0, thresholds 0.25 / 0.50 / 0.75) is 2.0 requests/unit/s;
 *  at the shipped defaults it is 0.32. ⇒ ⛔ TREAT EscalationCooldown AS THE ANTI-MILL KNOB
 *  AND DO NOT LOWER IT BELOW 1.0 WITHOUT RE-DERIVING THE CEILING. See Evaluate's comment.
 */
USTRUCT(BlueprintType)
struct FSiegeStuckTuning
{
	GENERATED_BODY()

	/** uu the unit must escape from its anchor to count as making progress. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float ProgressRadius      = 150.f;

	/** SQUARED speed (uu/s)^2 at or above which the unit is moving; 2500 == 50 uu/s. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float MinSpeedSq          = 2500.f;

	/** Stalled seconds before rung 1, Sidestep. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepSeconds     = 1.5f;

	/** Stalled seconds before rung 2, WidenAndRepath. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float WidenSeconds        = 3.0f;

	/** Stalled seconds before rung 3, Abandon (which then clears the stall). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float AbandonSeconds      = 6.0f;

	/** ⛔ BRAKE 1. Minimum seconds between two fired rungs. This is the anti-mill ceiling. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float EscalationCooldown  = 1.0f;

	/** uu the Sidestep waypoint sits laterally off the goal direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepDistance    = 350.f;

	/** Seconds the sidestep move is held against bGoalChanged (TASK-532 owns the lease). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepLeaseSeconds = 2.0f;
};

class GITCLAUDEUNREALTEST_API FSiegeStuckStatics
{
public:

	/**
	 *  PURE. Advances one unit's stall ladder by DeltaSeconds and returns the rung to fire
	 *  this poll, or None. State is the ONLY thing written; nothing else is touched.
	 *
	 *  ── THE LADDER (`NAV-§3`) ──────────────────────────────────────────────────────────
	 *
	 *  NOT advancing  OR  fast enough  OR  escaped ProgressRadius from the anchor
	 *      => Reset(State), re-anchor at Location, return None.       <- the common answer
	 *
	 *  advancing AND slow AND still inside ProgressRadius of the anchor
	 *      => StalledSeconds += DeltaSeconds, SecondsSinceEscalation += DeltaSeconds
	 *      => BRAKE 1: SecondsSinceEscalation < EscalationCooldown            -> None
	 *      => BRAKE 2: the entitled rung is not ABOVE EscalationLevel         -> None
	 *      => >= SidestepSeconds -> Sidestep (level 1)
	 *      => >= WidenSeconds    -> WidenAndRepath (level 2)
	 *      => >= AbandonSeconds  -> Abandon (level 3), then Reset
	 *
	 *  ⛔ THE TWO BRAKES ARE THE ACCEPTANCE CRITERION, NOT A DETAIL. A follow implementation
	 *  that re-paths unconditionally is a QA FAIL (the standing law, CONVENTIONS "FOLLOW
	 *  command…" §4), and the watchdog inherits it in full: a stuck ladder that can re-issue a
	 *  move every poll is the TASK-280 / TASK-282 re-path mill wearing a rescue's clothes.
	 *
	 *  ⚠️ bAdvancing MEANS "THIS UNIT HAS AN ACTIVE PATH-FOLLOWING REQUEST THIS POLL" — the
	 *  shipped idiom is GetMoveStatus() (already read at SummonedUnit.cpp:2440). A unit that
	 *  is idle BY DESIGN (holding, stationed, no request) passes bAdvancing = false and
	 *  therefore re-anchors and returns None no matter how long it stands there. ⛔ Without
	 *  that, the ladder would "rescue" units that were told to stand still.
	 *
	 *  ⚠️ DeltaSeconds IS A PARAMETER AND MUST NEVER BE READ FROM StateCheckInterval.
	 *  AMinerUnit sets that to 0 BY SEAL (MinerUnit.cpp:61) and TASK-533 calls this same
	 *  ladder. This function is therefore total at DeltaSeconds == 0: it accumulates zero, so
	 *  no rung ever advances, it never divides by the delta, and it never asserts. Negative
	 *  and non-finite deltas are clamped to 0 by the same expression. ⭐ It is the same latent
	 *  shape TrackChargeMovement already carries (SummonedUnit.cpp:2733 —
	 *  `ChargeMoveElapsed += StateCheckInterval`), harmless there only because bCharge gates
	 *  it out. It is not reproduced here.
	 *
	 *  @param bAdvancing     the unit has an active path-following request this poll
	 *  @param Location       the unit's world location this poll
	 *  @param VelocitySizeSq GetVelocity().SizeSquared(); compared against Tuning.MinSpeedSq
	 *  @param DeltaSeconds   WORLD-CLOCK seconds since the previous call. 0 is legal and inert
	 *  @param Tuning         the unit class's EditDefaultsOnly feel values
	 *  @param State          READ AND WRITTEN. The unit's transient stall state
	 *  @return the rung to fire this poll, or None (the overwhelmingly common answer)
	 */
	static ESiegeStuckAction Evaluate(bool bAdvancing, const FVector& Location, float VelocitySizeSq,
	                                  float DeltaSeconds, const FSiegeStuckTuning& Tuning,
	                                  FSiegeStuckState& State);

	/**
	 *  PURE and ⛔ FULLY DETERMINISTIC: same inputs => same output, every call, forever. No
	 *  FMath::Rand, no time read, no world query, no nav projection, no state.
	 *
	 *  Returns a lateral waypoint PERPENDICULAR to the Location -> Goal direction, exactly
	 *  SidestepDistance away from Location, with the side ALTERNATING ON Attempt PARITY
	 *  (even => one side, odd => the other).
	 *
	 *  ⚠️ THE PERPENDICULAR IS TAKEN IN THE XY PLANE and Z is preserved from Location.
	 *  "Perpendicular" to a 3D vector names a whole plane rather than a direction; the
	 *  sidestep we want on a battlefield is always lateral on the ground.
	 *
	 *  ⛔ THE DEGENERATE CASES ARE HANDLED EXPLICITLY AND MUST NOT PRODUCE NaN:
	 *    - Goal == Location (or a goal directly overhead) leaves a zero-length lateral =>
	 *      THE STABLE FALLBACK AXIS IS WORLD +Y, chosen because it is exactly what the
	 *      formula yields for a unit facing world +X, so the degenerate answer is continuous
	 *      with the normal one rather than an unrelated special case;
	 *    - a non-positive or non-finite SidestepDistance returns Location unchanged.
	 *
	 *  ⚠️ THE RESULT IS A RAW GEOMETRIC POINT — it is NOT nav-projected and is NOT guaranteed
	 *  reachable, by design (a nav query here would break both the purity and the cost
	 *  claim). A sidestep onto unwalkable ground simply fails to move the unit, the stall
	 *  clock keeps running, and the ladder climbs to WidenAndRepath and then Abandon. That
	 *  escalation IS the recovery path for a bad sidestep.
	 *
	 *  @param Location         the unit's world location
	 *  @param Goal             the world location the unit is trying to reach
	 *  @param SidestepDistance lateral offset in uu (Tuning.SidestepDistance)
	 *  @param Attempt          a caller-owned counter; only its PARITY is read, and it is
	 *                          total over the whole int32 range including negatives
	 *  @return the lateral waypoint, at Location's Z
	 */
	static FVector ComputeSidestepGoal(const FVector& Location, const FVector& Goal,
	                                   float SidestepDistance, int32 Attempt);

	/**
	 *  Clears EVERY field of State to the declared defaults — anchor, both clocks, the
	 *  escalation level and bHasAnchor.
	 *
	 *  ⛔ THIS IS THE ONLY WAY A STALL ENDS BESIDES RE-ANCHORING, and Evaluate calls it
	 *  itself on Abandon. TASK-532 calls it on arrival, on a goal change, on state exit, on
	 *  death and in FreezeAI — a stall clock that outlives the order it was measuring is a
	 *  stuck unit of a new kind.
	 *
	 *  Implemented as an assignment from a default-constructed FSiegeStuckState, so it cannot
	 *  drift from the field defaults in the declaration and cannot miss a field added later.
	 */
	static void    Reset(FSiegeStuckState& State);
};
