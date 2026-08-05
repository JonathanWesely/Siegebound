// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeStuckStatics.h"

// ⛔ THERE IS DELIBERATELY NOTHING ELSE TO INCLUDE. FVector and FMath arrive complete through
// the header's CoreMinimal, and this translation unit names no engine type beyond them: no
// UWorld, no AActor, no AAIController, no UNavigationSystemV1, no UPathFollowingComponent.
// An engine include appearing in this file means the ladder stopped being pure, and that
// costs TASK-536 its headless test (CONVENTIONS `NAV-§3`, and spec (7) of TASK-531).

DEFINE_LOG_CATEGORY(LogSiegeStuck);

ESiegeStuckAction FSiegeStuckStatics::Evaluate(bool bAdvancing, const FVector& Location, float VelocitySizeSq,
                                               float DeltaSeconds, const FSiegeStuckTuning& Tuning,
                                               FSiegeStuckState& State)
{
	// ══ THE OVERWHELMINGLY COMMON PATH, FIRST AND CHEAPEST ═════════════════════════════════
	//
	// Any ONE of these means the unit is not stalling, so the stall state is thrown away and
	// re-anchored where the unit stands right now:
	//
	//   !bAdvancing                    no active path-following request this poll. ⛔ A unit
	//                                  idle BY DESIGN (holding, stationed, mid-attack) MUST
	//                                  land here regardless of elapsed time, or the ladder
	//                                  "rescues" units that were told to stand still.
	//   VelocitySizeSq >= MinSpeedSq   it is moving. Squared on both sides — no sqrt.
	//   !State.bHasAnchor              first poll, or the poll after a Reset: there is no
	//                                  anchor to measure against yet, so make one.
	//   escaped ProgressRadius         it has covered ground since the anchor. Real progress.
	//
	// ⭐ THE || CHAIN SHORT-CIRCUITS IN THAT ORDER ON PURPOSE: the two cheapest and most
	// frequently-true tests are first, so the single DistSquared below is reached only by a
	// unit that already has a live move request AND is measurably slow. Cost on this path is
	// at most one DistSquared, one multiply, three compares and a ~32-byte POD store — zero
	// allocations, zero world queries. FOR SCALE: AcquireTarget already runs a full-world
	// GetAllActorsWithInterface WITH a TArray allocation, per unit, on this very same 0.25 s
	// poll (SummonedUnit.cpp:1453-1454). ⛔ No fps or ms claim is made here or anywhere in
	// this batch — no FPS baseline exists in this project (`NAV-§2c`), so only the structural
	// claim is defensible.
	if (!bAdvancing
		|| VelocitySizeSq >= Tuning.MinSpeedSq
		|| !State.bHasAnchor
		|| FVector::DistSquared(Location, State.ProgressAnchor) > FMath::Square(Tuning.ProgressRadius))
	{
		Reset(State);
		State.ProgressAnchor = Location;
		State.bHasAnchor     = true;
		return ESiegeStuckAction::None;
	}

	// ══ THE UNIT IS ADVANCING, SLOW, AND HAS NOT LEFT ITS ANCHOR: IT IS STALLING ═══════════

	// ⚠️ DeltaSeconds IS A PARAMETER AND IS NEVER StateCheckInterval — AMinerUnit sets that to
	// 0 by seal (MinerUnit.cpp:61) and TASK-533 drives this same ladder. This one expression
	// is the whole DeltaSeconds == 0 contract: zero, negative and NaN all become 0, so the
	// clocks below cannot advance, cannot go backwards and cannot become non-finite.
	// ⛔ Written as `> 0.f ? … : 0.f` rather than FMath::Max so that NaN lands on 0 too (any
	// comparison against NaN is false). Nothing in this function ever DIVIDES by the delta,
	// so a zero delta has no second failure mode to guard.
	const float SafeDelta = (DeltaSeconds > 0.f) ? DeltaSeconds : 0.f;

	State.StalledSeconds         += SafeDelta;
	State.SecondsSinceEscalation += SafeDelta;

	// ══ BRAKE 1 — THE COOLDOWN. ⛔ THIS IS THE ANTI-MILL GATE ══════════════════════════════
	//
	// A rung fires at most once per EscalationCooldown, because firing zeroes
	// SecondsSinceEscalation and no fire is reachable below the cooldown. It is checked
	// BEFORE any rung is even considered, so a stalled unit's steady-state per-poll cost is
	// two adds and one compare.
	//
	// ⭐ THIS ALSO BOUNDS THE FIRST RUNG. SecondsSinceEscalation accumulates from the start of
	// the stall alongside StalledSeconds, so with the shipped defaults it reads 1.5 when
	// StalledSeconds first reaches SidestepSeconds — the gate is already open and Sidestep
	// fires exactly on time. The cooldown only ever BITES when the thresholds are tuned
	// closer together than it is, which is precisely the case it exists for: FSiegeStuckTuning
	// is EditDefaultsOnly and Jonathan can set SidestepSeconds = 1.5 and WidenSeconds = 1.6.
	if (State.SecondsSinceEscalation < Tuning.EscalationCooldown)
	{
		return ESiegeStuckAction::None;
	}

	// Which rung has the elapsed stall earned? Tested HIGH-TO-LOW so the answer is the
	// HIGHEST satisfied threshold. That ordering is also what makes mis-ordered, equal or
	// zero tunables safe: whatever the values, exactly one level comes out and it never
	// oscillates. ⛔ A NaN threshold compares false, so a corrupt tunable DISABLES the ladder
	// (DesiredLevel stays 0) rather than firing it every poll.
	uint8 DesiredLevel = 0;
	if (State.StalledSeconds >= Tuning.AbandonSeconds)        { DesiredLevel = 3; }
	else if (State.StalledSeconds >= Tuning.WidenSeconds)     { DesiredLevel = 2; }
	else if (State.StalledSeconds >= Tuning.SidestepSeconds)  { DesiredLevel = 1; }

	// ══ BRAKE 2 — MONOTONICITY. A RUNG NEVER RE-FIRES WITHIN A STALL ═══════════════════════
	//
	// Strictly greater, so level N is fired at most ONCE: the ladder climbs, then abandons.
	// This also covers "no rung earned yet" (DesiredLevel == 0 == EscalationLevel).
	//
	// A single large delta (a frame hitch, or a miner polled after a long gap) may jump
	// straight from level 0 to level 3. That is the honest reading — the unit really has been
	// stalled that long — and it issues STRICTLY FEWER requests than climbing rung by rung
	// would, so it cannot weaken the bound below.
	if (DesiredLevel <= State.EscalationLevel)
	{
		return ESiegeStuckAction::None;
	}

	// ══ ⭐ THE WORST CASE, COMPUTED FROM THE TWO BRAKES ABOVE (`NAV-§3`, gate criterion 1) ══
	//
	// ⛔ THE TWO BRAKES BOUND DIFFERENT THINGS, AND ONLY ONE OF THEM BOUNDS THE *RATE*. An
	// earlier revision of this block claimed "either brake alone bounds the request rate";
	// that is FALSE and was corrected at qa/TASK-537.md (WARN-1). The numbers below are QA's
	// derivation, not a re-estimate.
	//
	//   BRAKE 1 (TIME — this is the rate brake) : a fire zeroes SecondsSinceEscalation and
	//                     none is reachable below EscalationCooldown
	//                     =>  <= 1 fire per unit per EscalationCooldown, whatever else is tuned.
	//   BRAKE 2 (COUNT PER STALL — ⛔ NOT a rate brake) : DesiredLevel must EXCEED
	//                     EscalationLevel and only rises, and there are exactly three levels
	//                     =>  <= 3 fires per stall, EVER. Level 3 is Abandon, which issues NO
	//                     path request (it drops the goal), so <= 2 PATH REQUESTS PER STALL.
	//                     ⚠️ THAT IS A COUNT, NOT A RATE: a stall's DURATION is itself tunable
	//                     (AbandonSeconds is EditDefaultsOnly), so brake 2 alone permits those
	//                     two requests to recur as fast as the thresholds allow.
	//
	// ⇒ THE RATE, DERIVED. Let P = the driver's poll period (0.25 s on both). A cycle is
	//   Sidestep -> Widen -> Abandon -> one dead re-anchor poll (Abandon's Reset drops
	//   bHasAnchor, so the next poll MUST take the re-anchor branch and return None), i.e. at
	//   least 4 polls, carrying 2 path requests.
	//
	//   ⇒ WORST CASE OVER LEGAL EditDefaultsOnly TUNING: EscalationCooldown = 0 with
	//     thresholds 0.25 / 0.50 / 0.75 gives 2 requests / (4 x 0.25 s) =
	//     ⭐ 2.0 EXTRA PATH REQUESTS / UNIT / SECOND.
	//   ⇒ SHIPPED DEFAULTS (1.5 / 3.0 / 6.0, cooldown 1.0): 2 requests / 6.25 s =
	//     ⭐ 0.32 / UNIT / SECOND.
	//   ⇒ `NAV-§3`'s <= 1 / unit / s CEILING THEREFORE RESTS ON EscalationCooldown >= 1.0
	//     ALONE. ⛔ It is a property of the TUNING, not a structural property of the ladder,
	//     and any retune that lowers EscalationCooldown below 1.0 lowers the ceiling with it.
	//   ⇒ AND ONLY FOR UNITS THAT ARE DEMONSTRABLY NOT MOVING — a unit that is moving, or has
	//     no move request at all, never reaches this line: it re-anchored above and costs
	//     zero requests forever. That half IS structural.
	//
	// A WHOLE-ARMY WEDGE AT ~120 UNITS IS THEREFORE ~38 EXTRA REQUESTS/S at the shipped
	// defaults, ~120/s at the <= 1/unit/s ceiling, and ~240/s at the pathological tuning
	// above. Bounded, and reached only in the case where the alternative is an unwinnable
	// match.
	//
	// ⛔ For contrast, this is what the law forbids: the same ladder WITHOUT these two lines
	// re-fires its rung on every 0.25 s poll = 4 requests/unit/s = ~480/s at 120 units, which
	// is the TASK-280 / TASK-282 re-path mill wearing a rescue's clothes. ⚖️ So the worst
	// legal tuning is HALF the mill and TWICE the stated ceiling — which is the exact reason
	// the false "either brake alone" claim mattered enough to correct in shipped source.
	State.EscalationLevel        = DesiredLevel;
	State.SecondsSinceEscalation = 0.f;

	switch (DesiredLevel)
	{
	case 1:
		return ESiegeStuckAction::Sidestep;

	case 2:
		return ESiegeStuckAction::WidenAndRepath;

	default:
		// Level 3, Abandon — the stall is over either way, so the state is cleared HERE rather
		// than left to the call site. Reset drops bHasAnchor, so the very next poll takes the
		// re-anchor branch and returns None; the earliest a second Abandon can occur is a full
		// AbandonSeconds later. ⛔ Abandon issues no path request of its own (`NAV-§3`): the
		// call site drops the move goal and lets the standing body re-choose next poll.
		Reset(State);
		return ESiegeStuckAction::Abandon;
	}
}

FVector FSiegeStuckStatics::ComputeSidestepGoal(const FVector& Location, const FVector& Goal,
                                               float SidestepDistance, int32 Attempt)
{
	// A non-positive distance names no lateral point. Returning the unit's OWN location is the
	// only answer that cannot be wrong: the caller issues a move that is already satisfied,
	// the rung is spent, and the ladder climbs on the next poll rather than acting on garbage.
	// ⛔ Written as !(x > 0) rather than (x <= 0) so a NaN distance lands here too.
	if (!(SidestepDistance > 0.f))
	{
		return Location;
	}

	// PERPENDICULAR, TAKEN IN THE XY PLANE. Rotating (dx, dy) by 90 degrees gives (-dy, dx);
	// Z is left at 0 so the returned point inherits Location's Z exactly. Flattening is
	// deliberate — "perpendicular" to a 3D vector names a whole plane, and the sidestep we
	// want on a battlefield is always lateral on the ground, at the unit's own height.
	FVector Lateral(-(Goal.Y - Location.Y), (Goal.X - Location.X), 0.f);

	// ⛔ THE DEGENERATE CASE, HANDLED EXPLICITLY: Goal == Location, or a goal directly
	// overhead, leaves a zero-length lateral. FVector::Normalize returns false and writes
	// NOTHING when the squared length is below tolerance, so there is no division by zero and
	// no NaN — the fallback simply replaces the vector.
	//
	// THE STABLE FALLBACK AXIS IS WORLD +Y. It is chosen, not arbitrary: it is exactly what
	// the rotation above yields for a unit facing world +X, so the degenerate answer is
	// CONTINUOUS with the normal one instead of being an unrelated special case. It is also a
	// constant, which keeps the whole function deterministic.
	if (!Lateral.Normalize())
	{
		Lateral = FVector::YAxisVector;
	}

	// SIDE ALTERNATES ON Attempt PARITY: even -> +Lateral, odd -> -Lateral.
	// ⛔ Bitwise AND rather than `% 2 == 0`, because `%` on a NEGATIVE int yields -1 for odd
	// values, so `% 2 == 0` would silently send every negative-odd Attempt to the SAME side as
	// the evens. `& 1` is total over the whole int32 range (two's-complement representation is
	// guaranteed since C++20) and keeps the alternation correct for any caller-supplied
	// counter, including one that has wrapped past INT32_MAX.
	const float Side = ((Attempt & 1) == 0) ? 1.f : -1.f;

	// ⛔ DETERMINISTIC BY CONSTRUCTION: every term above is a parameter or a compile-time
	// constant. No RNG, no clock, no world, no nav projection, no static state. Same inputs
	// produce the same FVector on every call, on every machine, which is what TASK-536 tests
	// and what keeps a sidestep reproducible in a bug report.
	return Location + Lateral * (Side * SidestepDistance);
}

void FSiegeStuckStatics::Reset(FSiegeStuckState& State)
{
	// Assigned from a default-constructed instance rather than field-by-field ON PURPOSE: the
	// member initialisers in FSiegeStuckState ARE the definition of "cleared", so this cannot
	// drift from them and cannot silently miss a field somebody adds later.
	State = FSiegeStuckState();
}
