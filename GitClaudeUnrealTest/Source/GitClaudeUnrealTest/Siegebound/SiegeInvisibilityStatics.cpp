// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeInvisibilityStatics.h"

/**
 *  ═══ TASK-827 (WITCH-§3 + WITCH-§6): THE PURE INVISIBILITY RULES — IMPLEMENTATION ═══
 *
 *  The header carries the law, the measured break-site ledger and the inverse ledger; this file
 *  carries only the four bodies. ⛔ ZERO BEHAVIOUR CHANGE — nothing in the shipped game calls any
 *  of this yet (TASK-829 wires it, TASK-830 casts it).
 *
 *  ⛔ Every function here is TOTAL and PURE-OR-IN-PLACE: no world, no actor, no allocation, no
 *  logging, no timer, ⛔ and no time parameter (the "permanently" law — see the header).
 */

bool FSiegeInvisibilityStatics::IsVisibleTo(ETeamId ViewerTeam, ETeamId TargetTeam, bool bTargetIsInvisible)
{
	// ⛔⛔ WITCH-§2, FOURTH LANE — FIRST, AND ⛔ UNCONDITIONAL.
	// A same-team viewer ALWAYS sees the target, veiled or not. This early return is placed
	// ABOVE the veil test on purpose: it makes "an invisible unit its own player cannot select,
	// order or heal" — which WITCH-§2 calls a BUG, not a feature — structurally unreachable
	// rather than merely unlikely. ⛔ Do NOT fold these two returns into one expression that
	// consults bTargetIsInvisible first; the ordering IS the guarantee.
	// ⭐ Pinned by Siegebound.Invisibility.FriendlyVeilNeverSuppressed, which exists solely to
	// fail on the careless `return !bTargetIsInvisible` implementation.
	if (ViewerTeam == TargetTeam)
	{
		return true;
	}

	// Cross-team: the veil is the whole feature. An UNVEILED enemy is visible, which is what
	// keeps this predicate from degenerating into "enemies are never visible" — the truth-table
	// test asserts both cross-team rows, so neither constant-return survives.
	return !bTargetIsInvisible;
}

bool FSiegeInvisibilityStatics::ApplyVeil(bool& bIsInvisible)
{
	// The ONLY write-true site in the project (WITCH-§6: bIsInvisible is ONE source of truth).
	// Reports whether THIS call did the veiling, so TASK-830's "never target an already-invisible
	// unit" rule has a measurable belt and a double-veil is a visible no-op, not a silent one.
	if (bIsInvisible)
	{
		return false;
	}

	bIsInvisible = true;
	return true;
}

bool FSiegeInvisibilityStatics::ApplyBreak(bool& bIsInvisible, ESiegeVeilBreakReason Reason)
{
	// ⛔⛔ Reason is a LABEL, ⛔ NOT A CONDITION — it is unused by the transition BY DESIGN.
	// WITCH-§3's set is CLOSED: every enumerator that exists breaks the veil, and an action that
	// does not break has no enumerator to pass (see the header's absence comment). Branching on
	// Reason here would re-open the closed set through the back door — the exact "helpfully add
	// Walk" failure the enum comment forbids. It is carried so TASK-829's ONE BreakInvisibility
	// method can LOG which act un-veiled the unit: the difference between a bug report reading
	// "invisibility is broken" and one reading "the Sapper's blast un-veiled it".
	(void)Reason;

	// ⛔⛔ "PERMANENTLY" (his word, WITCH-§3) LIVES IN THESE FOUR LINES AND NOWHERE ELSE.
	// MONOTONE + IDEMPOTENT: once false, every later call is a no-op returning false. There is
	// ⛔ no counter to decay, ⛔ no deadline to compare against, ⛔ no reason that restores, and
	// ⛔ no inverse function anywhere in this file. The flag returns to true ONLY via ApplyVeil
	// from a NEW witch cast — his parenthetical, exactly: "(unless they are later made invisible
	// by a witch again)".
	if (!bIsInvisible)
	{
		return false;
	}

	bIsInvisible = false;

	// True EXACTLY ONCE per veil — on the true→false edge. This is what lets TASK-829 fire its
	// one-shot side effects (the MI_Unit_Invisible material swap, the log line) WITHOUT keeping a
	// "was visible" cache, which WITCH-§6 bans as a second source of truth.
	return true;
}

const TCHAR* FSiegeInvisibilityStatics::UnrecognisedReasonToken()
{
	return TEXT("<unrecognised-veil-break-reason>");
}

const TCHAR* FSiegeInvisibilityStatics::ToString(ESiegeVeilBreakReason Reason)
{
	// ⛔ NO `default:` LABEL — deliberately. A default would swallow a newly added seventh
	// enumerator silently; without one, the fall-through below returns the unrecognised token and
	// Siegebound.Invisibility.BreakReasonCensusIsClosed FAILS. That test asserts index
	// VeilBreakReasonCount is STILL unrecognised, so adding `Walk` here — even WITH a matching
	// case arm — fails the suite and forces the reader back to WITCH-§3.
	switch (Reason)
	{
	case ESiegeVeilBreakReason::Attack:  return TEXT("Attack");
	case ESiegeVeilBreakReason::Heal:    return TEXT("Heal");
	case ESiegeVeilBreakReason::Mine:    return TEXT("Mine");
	case ESiegeVeilBreakReason::Empower: return TEXT("Empower");
	case ESiegeVeilBreakReason::Cast:    return TEXT("Cast");
	case ESiegeVeilBreakReason::Death:   return TEXT("Death");
	}

	return UnrecognisedReasonToken();
}
