// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Siegebound/SummonedUnit.h"
#include "SorcererUnit.generated.h"

/**
 *  Siegebound Sorcerer — the ANCIENT GROUNDS empowerer (TASK-360; law: CONVENTIONS
 *  "Ancient Grounds + Sorcerer + 180° terrain symmetry" §3; design authority: Jonathan's
 *  approved plan `there-is-one-new-glittery-bentley.md` §3).
 *
 *  A 60-cost ASummonedUnit subclass that is COMMANDABLE LIKE ANY OTHER UNIT but CANNOT
 *  ATTACK. Its entire contribution is presence: while it stands inside an AAncientGround,
 *  that ground's 1 Hz tick grants every eligible FRIENDLY unit standing in the same ground
 *  one permanent damage stack per second per sorcerer (+5% of base damage each, capped at
 *  +400%). The boost is permanent, survives leaving the ground, and is lost only on death.
 *
 *  ── WHY A CLASS AND NOT A CSV FLAG ──────────────────────────────────────────────────────
 *  Both behaviors are CLASS IDENTITY, not card stats (the mechanic-rules-aren't-card-stats
 *  law), so they ride two base virtuals rather than new DT_Cards columns — no enum change,
 *  no CSV column, no DT_Cards schema risk:
 *    • CanEverAttack()          → false  (THE SEAL)
 *    • IsAncientGroundEmpowerer() → true  (what makes a ground grant stacks)
 *  This is the ShouldHoldDeathAnim() IDIOM (same file, same shape, zero surprise for QA) —
 *  but deliberately NOT its access level: both virtuals are declared in ASummonedUnit's
 *  PUBLIC block because AAncientGround calls them from outside the class hierarchy.
 *
 *  ── THE ATTACK SEAL: THREE GUARD POINTS, ALL IN ASummonedUnit.cpp ───────────────────────
 *  ⚠️ The Sorcerer row ships Damage/Range/Cadence 0, and LoadStatsAndStart clamps the row
 *  Cadence to MinAttackCadence (0.05 s) — so a Cadence-0 unit that EVER reached the Attack
 *  state would land 20 hits/s. One guard is not enough; the seal is:
 *    1. EnterAttack()       — the structural chokepoint every attack entry funnels through:
 *                             a sealed unit STANDS DOWN via EnterIdle() (not a silent
 *                             return), so the state machine stays honest.
 *    2. UpdateStateGrouped() — a sealed unit acquires NOTHING (target forced null instead of
 *                             the two AcquireEnemyNearPoint tiers) and falls through to
 *                             tier-3 station-keeping. Without it a grouped sorcerer would
 *                             walk up to an enemy and stand there.
 *    3. PerformAttack()      — !CanEverAttack() joins the bAIFrozen/bSpellFrozen gate
 *                             (defense in depth, and cover for any future caller).
 *  The Miner's structural seal (StateCheckInterval 0) is NOT available here and must never
 *  be copied onto this class: a COMMANDABLE unit needs its state timer to obey group orders.
 *
 *  ── COMMANDABILITY IS LOAD-BEARING ON `Profile = Standard` ──────────────────────────────
 *  IsGroupCommandEligible() gates on Profile == ECardProfile::Standard, so the CSV row keeps
 *  Standard (Support — the Cleric's never-attack route — would make the Sorcerer
 *  UNCOMMANDABLE, which is why that option was rejected). Nothing on the spawn path changes:
 *  the placement check is IsChildOf(ASummonedUnit), which this class passes by inheritance.
 *
 *  ── STATE THIS CLASS DELIBERATELY DOES NOT TOUCH ────────────────────────────────────────
 *  • StateCheckInterval stays at the base 0.25 s (see above — commandable units need it).
 *  • It adds NO replicated state and does not change ASummonedUnit's replication posture;
 *    the M8 P2 duty for the boost counter is recorded on PermanentDamageStacks in
 *    SummonedUnit.h. In M8 P1 units are server-only, so a remote client sees no sorcerers
 *    and no boost bars — a KNOWN P1 state, not a defect.
 *  • It never self-boosts: AAncientGround COUNTS an empowerer and then SKIPS it as an
 *    occupant, and CanReceiveDamageBoost() independently returns false for it
 *    (CanEverAttack() false, row Damage 0).
 *
 *  Blueprint: /Game/Blueprints/Units/BP_Unit_Sorcerer reparents to THIS class (TASK-375).
 *  Stats still bind from the DT_Cards `Sorcerer` row at BeginPlay, never from code (§3.0).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASorcererUnit : public ASummonedUnit
{
	GENERATED_BODY()

public:

	ASorcererUnit();

	//~ Begin ASummonedUnit ancient-grounds surface (CONVENTIONS §3 + the §7 PINNED registry —
	//~ these two overrides are the ENTIRE behavioral difference from a plain ASummonedUnit).

	/** THE SEAL: this unit can never attack. Enforced at three guard points in ASummonedUnit.cpp (see the class comment). */
	virtual bool CanEverAttack() const override { return false; }

	/** This unit empowers friendly occupants of an AAncientGround it stands in (and is itself skipped as an occupant — never self-boosts). */
	virtual bool IsAncientGroundEmpowerer() const override { return true; }

	//~ End ASummonedUnit ancient-grounds surface
};
