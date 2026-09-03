// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// FVector (Math/Vector.h) and FMath (Math/UnrealMathUtility.h) arrive complete through
// CoreMinimal, and they are the ONLY types this pair names: no UWorld, no AActor, no UObject,
// no component, no clock, no allocation. That is not tidiness — it is the property the 14
// headless tests in SiegeLadderClimbTest.cpp are built on. The FSiegeStuckStatics /
// FSiegeCombatStatics idiom, for the third time (complete-type include law, TASK-110).
// ⛔ No .generated.h: nothing in this pair is reflected, deliberately (see FSiegeLadderClimbState).
#include "CoreMinimal.h"

/**
 *  ═══ TASK-776 (CONTACT-§2): THIS FILE IS A ***MOVE***, ⛔ NOT NEW CODE ═══
 *
 *  Everything below the include was lifted VERBATIM out of SummonedUnit.h (this banner,
 *  FSiegeLadderClimbState and FSiegeLadderClimbStatics) and SummonedUnit.cpp (the seven
 *  definitions, at SummonedUnit.cpp:129-302). ⛔ Not one function name, signature, parameter
 *  name, default value, expression or comment changed in transit. CONTACT-§2 ruled the LIFT over
 *  a UActorComponent and over a shared base class precisely because it is a zero-behaviour-change
 *  relocation, and its acceptance criterion is that SiegeLadderClimbTest.cpp's 14 tests pass
 *  byte-identically. ⚖️ A refactor that also fixes something is a refactor nobody can review.
 *
 *  ⭐ WHY IT MOVED: AHeroCharacter becomes a SECOND consumer of these rules (CONTACT-§1 C-1 —
 *  the hero has never been able to climb, and the shipped mechanism was AI-only by construction).
 *  ⛔ Each pawn class still keeps its OWN FSiegeLadderClimbState member, its OWN driver, and
 *  wires its OWN exits: there is ⛔ no registry, ⛔ no subsystem, ⛔ no singleton, ⛔ no shared
 *  base class and ⛔ no component. THE EXITS ARE THE RISK SURFACE AND THEY STAY PER-CLASS — which
 *  is exactly why a component was refused: it cannot intercept HandleDeath, FreezeAI, ApplyFreeze,
 *  UnPossessed, EndRecall or EndPlay, so it would buy nothing while rewriting the hottest file.
 *
 *  ⚠️ THE TWO COMMENT CLAUSES THE MOVE ITSELF SUPERSEDED — RECORDED HERE RATHER THAN EDITED
 *  BELOW, SO THE ORIGINAL TEXT STAYS DIFFABLE CHARACTER-FOR-CHARACTER AGAINST THE SHIPPED VERSION:
 *    (a) FSiegeLadderClimbStatics' doc comment says "no new file (TASK-738 owns
 *        SummonedUnit.{h,cpp} only) — it shares this header with its consumer". TASK-738's fence
 *        was real and its reasoning stands; CONTACT-§2 has since granted this file, and the
 *        statics now share a header with the STATE they consume rather than with one consuming
 *        actor. ⛔ Its one-class-per-header note still applies and still must not be flagged.
 *    (b) The .cpp banner says "everything it DOES lives on the actor below". The SPLIT that
 *        sentence describes is unchanged — the doing lives on ASummonedUnit (SummonedUnit.cpp)
 *        and, from TASK-778, on AHeroCharacter. Only the word "below" moved.
 *
 *  ⛔ ESiegeLadderExit did ⛔ NOT move, and that is a decision rather than an oversight: no
 *  function in this file takes a reason (the teardown is REASON-AGNOSTIC by design), its
 *  enumerators are ASummonedUnit driver vocabulary (AbortLadderClimb, FreezeAI, ApplyFreeze,
 *  ASummonedUnit::EndPlay), and CONTACT-§3.1 requires the hero's TEN exits to be enumerated
 *  FRESH rather than mapped across this eight-value enum. Publishing it here would invite exactly
 *  the mapping the law forbids. It stays in SummonedUnit.h beside the driver that produces it.
 *
 *  ⚠️ CanBegin's bAIFrozen / bSpellFrozen ARE UNIT VOCABULARY (CONTACT-§3.2). A non-unit caller
 *  MAPS them to its own equivalents at file:line — ⛔ it does NOT type `false` to make the call
 *  compile. `false` there is not a neutral value: it is an assertion that no match-end freeze and
 *  no spell freeze can exist for this pawn, and a pawn that starts a MOVE_Flying climb during
 *  either one is the hang this whole feature is shaped around.
 *
 *  ⛔ M8 (CONTACT-§9): ⛔ no replicated property and ⛔ no RPC is authored here. The M8 shape is
 *  already declared in FSiegeLadderClimbState's own comment below — the struct is deliberately
 *  UNREFLECTED, so nothing in it can be replicated by accident even after a later refactor.
 */

//~ ═══════════════════════════════════════════════════════════════════════════════════════
//~  THE LADDER CLIMB — the Watch Tower's ascent (TASK-738; CONVENTIONS TOWER-§8/§9/§10)
//~ ═══════════════════════════════════════════════════════════════════════════════════════
//~
//~ Jonathan, verbatim (2026-09-01): "instead of making it a ramp that you walk up it instead
//~ has a ladder you climb up" — and, the same day, the DECIDED requirement this whole block
//~ is shaped around: "they should be attackable while climbing, but they can't attack back."
//~
//~ ⭐⭐ WHY A LADDER IS A TRAVERSAL SUBSYSTEM RATHER THAN A MESH SWAP, MEASURED AT THE ENGINE
//~ (TOWER-§8.1, re-read at the source rather than relayed): a 76° face is 2.4× over Recast's
//~ 32.005° walkable ceiling, so a nav LINK is the only way a path to the deck exists at all
//~ (TASK-734's half) — and UCharacterMovementComponent::ConstrainInputAcceleration
//~ PLANE-PROJECTS steering input whenever IsMovingOnGround() || IsFalling()
//~ (CharacterMovementComponent.cpp:8121-8131), so the vertical component of any steer handed
//~ to a walking pawn is DELETED by the engine, every frame, by design. A link alone therefore
//~ steers a unit that then walks nowhere. EMovementMode has no climb mode
//~ (EngineTypes.h:1018-1045), MOVE_Custom is refused for this ship (a new physics surface in
//~ a project with zero lines of it), and a raw SetActorLocation/TeleportTo lerp is refused
//~ outright (it bypasses the capsule, the sweep and depenetration — the mechanism NAV-§
//~ refuses on principle). ⇒ MOVE_Flying + swept AddMovementInput along ONE straight line.
//~
//~ ⚠️⚠️ AND THE ONE THING THIS FEATURE CAN SHIP BROKEN, WORSE THAN ANYTHING THE RAMP COULD
//~ DO: MOVE_Flying IGNORES GRAVITY. AN EXIT PATH THAT FORGETS TO RESTORE THE MOVEMENT MODE
//~ LEAVES A UNIT HANGING IN MID-AIR, FOREVER. Under the ramp the tower-destroyed case was
//~ FREE — the floor vanished and CharacterMovement dropped to MOVE_Falling by itself. It is
//~ explicitly NOT free now. That is why the teardown is ONE function reached from EVERY exit,
//~ and why End() below is an exactly-once latch rather than a flag anybody may clear.

/**
 *  Transient per-unit state of an in-flight ladder climb (TASK-738). ⛔ Deliberately
 *  UNREFLECTED (the FSiegeStuckState precedent, NAV-§8/§11): nothing here is a UPROPERTY, so
 *  it cannot be replicated by accident — the property that makes this batch's "no replicated
 *  property" declaration survive a later refactor instead of merely being true today.
 *
 *  ⭐ A DEFAULT-CONSTRUCTED VALUE IS THE "NOT CLIMBING" STATE, and End() restores exactly
 *  that — a whole-struct reset, never a bActive = false with residue left behind (the TASK-020
 *  zero-drift discipline: state that outlives its episode is the next bug).
 */
struct FSiegeLadderClimbState
{
	/** True for the whole ascent. THIS IS ALSO THE TOWER-§9 DISARM TERM — see IsAttackAllowed. */
	bool bActive = false;

	/**
	 *  Where the climb starts, in CAPSULE-CENTRE space.
	 *  ⚠️⚠️ NAMED Start/End AND ⛔ NOT Foot/Top ON PURPOSE (TASK-734's finding, taken): the API
	 *  is From -> To and DESCENT passes them the other way round. A field called "Foot" invites a
	 *  future "fix" to enforce an ordering, which would silently break climbing DOWN.
	 */
	FVector Start = FVector::ZeroVector;

	/** Where the climb ends, in CAPSULE-CENTRE space. On a descent this is the ladder's FOOT. */
	FVector End = FVector::ZeroVector;

	/** |End - Start| at Begin. Cached so the watchdog budget is fixed at the start and cannot drift. */
	float LengthUU = 0.f;

	/** Seconds since Begin. */
	float ElapsedSeconds = 0.f;

	/** Watchdog budget, computed once at Begin from the length and the shipped rate. */
	float TimeoutSeconds = 0.f;

	/**
	 *  ⚠️⚠️ HOW MUCH OF THE LINE, MEASURED FROM ITS **ELEVATED** END, MUST BE DRIVEN WITHOUT A
	 *  COLLISION SWEEP — because that stretch passes THROUGH THE DECK SLAB (TASK-737's measurement;
	 *  see FSiegeLadderClimbStatics::ShouldSweep for the full reasoning and the arithmetic).
	 *  0 means the whole line is swept.
	 */
	float DeckBreachUU = 0.f;

	/**
	 *  Which end of the line is the elevated one (the deck), i.e. where DeckBreachUU applies.
	 *  ⭐⭐ THIS IS A FACT ABOUT THE GEOMETRY, ⛔ NOT AN ORDERING CONSTRAINT ON THE API. Both
	 *  argument orders are legal and both work: on an ASCENT the deck is at End, on a DESCENT it
	 *  is at Start, and Begin resolves it by comparing Z. ⛔ Nothing anywhere requires the caller
	 *  to pass the foot first.
	 */
	bool bDeckIsAtEnd = true;

	/** The capsule half-height the line was offset by at Begin — the same value the teardown's arrival snap uses. */
	float CapsuleHalfHeightUU = 0.f;
};

/**
 *  ═══ THE CLIMB'S DECIDABLE LOGIC, AS PURE FUNCTIONS (TASK-738) ═══
 *
 *  ⛔ EVERYTHING HERE IS PURE. No UWorld, no AActor, no UObject, no component, no clock read,
 *  no allocation, no logging. Every input is a parameter; every output is a return value or a
 *  write to the caller's FSiegeLadderClimbState. The precedents are FSiegeStuckStatics
 *  (SiegeStuckStatics.h) and HIGH-§3's HeightAdvantageMultiplier seam — "a testability
 *  obligation gets a testability seam".
 *
 *  ⭐⭐ AND HERE PURITY IS NOT STYLE, IT IS THE ONLY INSTRUMENT AVAILABLE, MEASURED:
 *  every automation test in this project is headless (there is not one UWorld::CreateWorld or
 *  SpawnActor in Source/GitClaudeUnrealTest/Siegebound/Tests/), and a world-less ASummonedUnit
 *  CANNOT be driven through these paths — UCharacterMovementComponent::SetDefaultMovementMode
 *  reaches UMovementComponent::GetPhysicsVolume, which dereferences GetWorld() unconditionally
 *  when UpdatedComponent is null (MovementComponent.cpp:290-298) and would crash the suite.
 *  ⇒ The eight exits are proven against THIS state machine, and the wiring of each call site
 *  is TASK-741's diff read. That split is stated rather than hidden.
 *
 *  ⛔ Not a UObject, not reflected, no Build.cs change, no new file (TASK-738 owns
 *  SummonedUnit.{h,cpp} only) — it shares this header with its consumer for the same reason
 *  NAV-§7 licenses FSiegeStuckState sharing SiegeStuckStatics.h: pure data types forming ONE
 *  concept with the statics that consume them. ⛔ QA must not flag it as a
 *  one-class-per-header violation.
 */
struct FSiegeLadderClimbStatics
{
	/**
	 *  Shortest line worth climbing. ⛔ A math guard, ⛔ NOT a policy: ClimbDirection would
	 *  have to normalise a zero vector, and every subsequent frame would steer nowhere while
	 *  the unit sat in MOVE_Flying waiting for a watchdog. Refusing at the door is cheaper and
	 *  honest. This is the ONE refusal reason beyond the four TOWER-§8.4(B) pins.
	 */
	static constexpr float MinClimbLineUU = 1.f;

	/**
	 *  How close the CAPSULE CENTRE must come to LadderTop to count as arrived.
	 *  ⭐ DERIVED, ⛔ not felt: 16 uu is HALF a Recast cell (TOWER-§2a's 32 uu cell — its
	 *  erosion tax is quoted as "2 more cells (64 uu)"), i.e. below the navmesh's own spatial
	 *  resolution, so the deck poly the unit lands on cannot tell the difference.
	 *  ⚠️ It is deliberately NOT the only arrival test: Advance also ends the climb the moment
	 *  the unit has travelled the full line length along the climb axis, so a low frame rate
	 *  that steps straight past this radius still arrives on that frame rather than sailing on.
	 */
	static constexpr float ArrivalToleranceUU = 16.f;

	/**
	 *  Watchdog budget = TimeoutScale × the climb's OWN expected duration (length ÷ rate).
	 *  ⭐ 4× is deliberately generous — this must NEVER end a healthy climb early; it exists
	 *  only so a unit whose sweep is blocked by geometry drops instead of hanging in MOVE_Flying
	 *  forever. At the shipped 1,236.9 uu / 350 uu/s that is 14.1 s against a 3.53 s ascent.
	 */
	static constexpr float TimeoutScale = 4.f;

	/** Floor for the watchdog, so a very short ladder still gets a sane budget. */
	static constexpr float MinTimeoutSeconds = 1.f;

	/** Divisor floor: a mis-tuned LadderClimbSpeedUU of 0 must produce a finite watchdog, never a division by zero and never an immortal climb. */
	static constexpr float MinClimbSpeedUU = 1.f;

	/**
	 *  ⚠️⚠️ HOW MANY CAPSULE HALF-HEIGHTS OF **Z** AT THE ELEVATED END MUST BE DRIVEN WITHOUT A
	 *  SWEEP, AND THE ARITHMETIC IS WRITTEN OUT BECAUSE THE ALTERNATIVE IS A SILENTLY DEAD FEATURE.
	 *
	 *  ⭐ THE MEASUREMENT (TASK-737, the ladder mesh): `LadderTop` is pinned **150 uu inside a
	 *  SOLID deck slab** and is approached from below at 76°, so **the last 40.8 uu of the climb
	 *  line lies INSIDE the deck geometry** (40.8 × sin 76° = 39.6 uu of Z ⇒ a ~40 uu slab).
	 *  ⛔ THE MESH CANNOT FIX IT: a hatch would put the socket over a HOLE — the castle-floor
	 *  defect class that cost this project a whole repair wave — and a thinner slab cannot help
	 *  because the 176 uu capsule straddles it regardless. ⛔ AND THE SOCKETS MAY NOT MOVE: both
	 *  are navmesh arithmetic (the top sits 86 uu inside the deck's surviving polygon, the foot
	 *  clears the body's eroded carve by 86 uu), so moving either can sever the traversal link.
	 *
	 *  ⇒ ⛔⛔ A SWEPT MOVE COLLIDES WITH THE UNDERSIDE OF THE DECK AND **STALLS** — with no error,
	 *  no log line, and every one of the eight exits still perfectly "correct". The unit simply
	 *  stops short forever and the card does nothing. **That is the same silent class as the
	 *  StopAttackLunge tick bug this file already caught: a working state machine, driven by a
	 *  mechanism that quietly cannot run.**
	 *
	 *  THE THREE HALF-HEIGHTS, EACH EARNING ITS PLACE (in Z, at the elevated end):
	 *    1 × 88  the capsule's TOP reaches the slab's underside before its centre does
	 *    1 × 88  an allowance for how far the slab hangs BELOW its own surface. ⚠️ This file
	 *            cannot read the mesh, so it is an ASSUMPTION — and it is 2.2× the ~40 uu the
	 *            mesh actually ships. A slab thicker than a capsule half-height would stall the
	 *            sweep before this window opens; that fails LOUDLY through the watchdog's warning
	 *            rather than silently, which is the whole point of having one.
	 *    1 × 88  the capsule's BOTTOM must clear the deck SURFACE before sweeping is safe again
	 *
	 *  ⭐ AND WHY BEING GENEROUS HERE IS **FREE**, WHICH IS THE PART THAT MAKES THIS SAFE RATHER
	 *  THAN JUST NECESSARY: `TOWER-§8.3` pins **≥56 uu of standoff between the capsule surface and
	 *  the tower body face ALONG THE WHOLE LINE**. So the only static geometry a non-swept stretch
	 *  anywhere on this line can pass through is the deck slab itself — the very thing it must.
	 *  The sweep's remaining job is pawn-vs-pawn, and that is covered: `TOWER-§10` L-1 is
	 *  one-climber-at-a-time, and a unit already standing on the deck is resolved by the same
	 *  depenetration that resolves any two of the 4-6 bodies `TOWER-§4` expects up there.
	 */
	static constexpr float DeckBreachCapsuleHalfHeights = 3.f;

	/**
	 *  ⭐⭐ THE TOWER-§9 ATTACK GATE, AND THE WHOLE POINT IS THAT IT TAKES **TWO INDEPENDENT
	 *  TERMS**. bCanEverAttack is the `const` CLASS-IDENTITY seal (ASorcererUnit / AMinerUnit:
	 *  "this class can NEVER attack"); bClimbing is a TRANSIENT PER-INSTANCE state that lasts
	 *  ~3.5 seconds. ⛔ The disarm is deliberately NOT implemented by overriding
	 *  CanEverAttack(): doing so would make a Sorcerer's permanent inability and a Footman's
	 *  three-second climb indistinguishable in the code, and the distinction is exactly what
	 *  TOWER-§9.2 requires to survive.
	 *
	 *  Read at the THREE shipped CanEverAttack() guard points and nowhere else (TOWER-§9.2):
	 *  EnterAttack() stands down to Idle · UpdateStateGrouped() acquires nothing ·
	 *  PerformAttack() refuses. ⛔ NOT a fourth guard point, ⛔ not a new suppression mechanism.
	 */
	static bool IsAttackAllowed(bool bCanEverAttack, bool bClimbing) { return bCanEverAttack && !bClimbing; }

	/**
	 *  ⭐⭐ THE COMPOSED TICK PREDICATE, AS A PURE TWO-TERM GATE (TASK-760) — the DECISION half of
	 *  ASummonedUnit::RefreshActorTickEnabled, lifted here for exactly the reason IsAttackAllowed
	 *  above was lifted here: the actor's writer stays WIRING, and the decision becomes a truth
	 *  table a headless test can drive. ⛔ The expression is character-for-character the one that
	 *  shipped (bLungeActive || bClimbActive) — TASK-760 changed WHERE it can be observed, ⛔ never
	 *  WHAT it decides. This actor cannot be instantiated in a headless test (its teardown
	 *  dereferences GetWorld() unconditionally), so a pure gate is the only instrument there is.
	 *
	 *  ⚠️⚠️ THE ROW THE SELF-HEAL TURNS ON IS (lunge FALSE, climb TRUE) ⇒ TRUE. That row is what
	 *  makes UpdateState's 0.25 s re-assert SAFE IN BOTH DIRECTIONS: re-asserting through this
	 *  predicate can never switch the tick OFF under a live climb (the StopAttackLunge regression
	 *  TASK-738 fixed), and can never switch it ON for a unit that has neither driver — which is
	 *  why the self-heal calls RefreshActorTickEnabled and ⛔ NEVER a bare
	 *  SetActorTickEnabled(true), which would fight whatever legitimately disabled the tick and
	 *  re-introduce the very coupling TASK-738 removed.
	 */
	static bool WantsActorTick(bool bLungeActive, bool bClimbActive) { return bLungeActive || bClimbActive; }

	/**
	 *  The admission predicate — TOWER-§8.4(B)'s pinned contract, character for character:
	 *  false for a DEAD, match-end-frozen (bAIFrozen), spell-frozen (bSpellFrozen) or ALREADY
	 *  CLIMBING unit, plus the degenerate/NaN line guard above. ⛔ Deliberately does NOT read
	 *  bStatsLoaded: the pinned doc comment enumerates four refusal reasons and adding a fifth
	 *  policy reason would be a silent divergence from a contract TASK-734 compiles against.
	 */
	static bool CanBegin(const FSiegeLadderClimbState& State, bool bDead, bool bAIFrozen, bool bSpellFrozen,
		const FVector& FromWorld, const FVector& ToWorld);

	/**
	 *  Arms the state, or ⛔ CHANGES NOTHING and returns false (the pinned "returns false and
	 *  changes NOTHING" guarantee lives HERE, in one place, so it cannot be half-kept).
	 *
	 *  ⭐⭐ THE ENDPOINTS ARRIVE AS **SURFACE** POINTS AND ARE STORED IN **CAPSULE-CENTRE** SPACE:
	 *  both are lifted by CapsuleHalfHeightUU. The sockets sit on generated navmesh (`TOWER-§8.3`)
	 *  — i.e. on the ground and on the deck SURFACE — but the thing that travels the line is the
	 *  capsule's CENTRE, which stands one half-height above whatever it is on. ⛔ Without the lift
	 *  the unit "arrives" with its FEET 88 uu below the deck, i.e. buried in the slab, and
	 *  depenetration drops it back down the tower. The lift is the same at both ends, so
	 *  LengthUU, the direction and the watchdog budget are all unchanged by it.
	 */
	static bool Begin(FSiegeLadderClimbState& State, bool bDead, bool bAIFrozen, bool bSpellFrozen,
		const FVector& FromWorld, const FVector& ToWorld, float ClimbSpeedUU, float CapsuleHalfHeightUU);

	/** Unit vector Start -> End; ZeroVector for a degenerate line (which CanBegin already refuses). */
	static FVector ClimbDirection(const FSiegeLadderClimbState& State);

	//~ ═══════════════════════════════════════════════════════════════════════════════════════
	//~  ⭐⭐ TASK-803 (`CONTACT-§14`) — THE CROSS-TRACK TERM. ⛔ **ADDITIVE**: THE DECLARATION
	//~  ABOVE IS UNTOUCHED, AND SO IS ITS DEFINITION.
	//~ ═══════════════════════════════════════════════════════════════════════════════════════
	//~
	//~ ⛔⛔ THE FENCE, WRITTEN WHERE THE EDIT WOULD HAPPEN (`CONTACT-§14.5`): `ClimbDirection`
	//~ above is ALSO read by ⛔ THREE sites that must keep ⛔ TODAY'S meaning —
	//~   1. `Advance`'s ARRIVAL DOT TEST (`SiegeLadderClimbStatics.cpp`, the `bPassedTheTop` line)
	//~   2. each driver's DECK-BREACH STEP (`HeroCharacter.cpp` / `SummonedUnit.cpp`, the non-swept
	//~      `SetActorLocation(Here + Direction * StepUU)`)
	//~   3. the hero's `IsLadderClimbInputHeld` SUSTAIN SIGN TEST (`HeroCharacter.cpp`)
	//~ ⇒ `SteerDirection` is ⛔ ADDED ALONGSIDE it and is swapped in at the ⛔ TWO SWEPT movement
	//~ calls and ⛔ NOWHERE ELSE.
	//~ ⚖️⚠️ RE-POINTING `ClimbDirection` AT THIS FUNCTION WOULD CHANGE ARRIVAL AND SUSTAIN
	//~ SEMANTICS WITH ⛔ NO COMPILE ERROR AND ⛔ NO TEST FAILURE. That is the whole reason the law
	//~ exists, and it is why `SiegeLadderClimbTest.cpp` test 19 asserts `ClimbDirection` is
	//~ BYTE-IDENTICAL and that `Advance` still reads the LINE rather than the steer.
	//~
	//~ ⚠️ WHAT THE DEFECT ACTUALLY IS, IN ONE SENTENCE (`CONTACT-§14`, which merged two boarded
	//~ symptoms into one): the contact trigger admits a pawn from a **350 uu 2D DISC**, the climb
	//~ driver had ⛔ NO cross-track term, and ⛔ nothing snaps the pawn onto the line ⇒ a pawn
	//~ admitted off-line stayed off-line for the ⛔ ENTIRE climb, and then paid the whole offset in
	//~ ⛔ ONE UNSWEPT FRAME at the arrival snap (`§14.3`). ⛔ It is invisible in footage because the
	//~ ladder carries ⛔ ZERO collision hulls, so nothing depenetrates and no body ever clips.

	/**
	 *  ⭐⭐ HOW FAR ALONG THE LINE THE STEER AIMS — the ONE number that sets the convergence rate,
	 *  and it is ⛔ DERIVED, ⛔ not felt (the `ArrivalToleranceUU` / `MinContactSpeedUU` discipline).
	 *
	 *  THE STEERING LAW, so the arithmetic below is checkable rather than asserted: with the aim
	 *  point `L` uu ahead ON the line and the pawn `e` uu off it, the steer's cross-track component
	 *  is `−e / sqrt(L² + e²)` ⇒ `de/ds = −e / sqrt(L² + e²)`, whose closed form is
	 *  `s = F(e₀) − F(e)` with `F(u) = sqrt(L² + u²) − L·ln((L + sqrt(L² + u²)) / u)`.
	 *  ⭐ It is LINEAR while `e ≫ L` (the error falls ~1 uu per uu travelled) and EXPONENTIAL with
	 *  length constant `L` once `e ≪ L` — i.e. it kills a large offset fast and settles gently.
	 *
	 *  ⛔ THE CEILING IS SET BY THE FENCE, ⛔ NOT BY TASTE: the DECK-BREACH stretch at the top is
	 *  driven along `ClimbDirection` (reader 2 above, which may ⛔ not change), so convergence has
	 *  only the SWEPT stretch to finish in — `1236.93 − 296.86 = 940.07 uu` for the hero
	 *  (`3 × 96` of Z ⇒ 296.86 of line) and `1236.93 − 272.13 = 964.80 uu` for the unit. The hero's
	 *  is shorter, so it is the binding case.
	 *
	 *  THE ARITHMETIC, from the WORST entry error `CONTACT-§14.2` admits (**±277.8 uu**, the
	 *  150 uu/s row) over that 940.07 uu:
	 *    · `L = 150` ⇒ **1.03 uu** left at the window ⇒ **23× inside** the hero's 24.0 uu side
	 *      margin and **15× inside** `ArrivalToleranceUU`. ⇒ `CONTACT-§14.3`'s arrival pop lands
	 *      at ~1 uu instead of ~247 — ⛔ below what one frame of ordinary movement moves anyway.
	 *      ⭐ AND THAT FIGURE IS A **CONSERVATIVE BOUND**, ⛔ not the expected value: it spends
	 *      940.07 uu of *path*, whereas covering 940.07 uu *along the line* takes ~1,041 uu of path
	 *      (converging costs ~8 % extra path, ⛔ well inside the 4× watchdog). The suite measures
	 *      the real figure at **~0.5 uu**. ⚖️ The bound errs toward MORE residual, which is the
	 *      only direction it is safe to be wrong in.
	 *    · `L ≈ 179` is the largest value still meeting a 10× margin (2.4 uu); ⭐ 150 sits 16 %
	 *      under it, which is the headroom, ⛔ not a coincidence.
	 *  THE FLOOR, and it is a REAL bound rather than padding: `L` must stay well above
	 *  `ArrivalToleranceUU` (16). At `L = 16` and `e = 247` the cross component is **0.998** ⇒ the
	 *  pawn CRABS almost purely sideways with no rise for the better part of a second, which reads
	 *  as broken — the same failure the deck-breach window's "⛔ not a teleport" clause guards.
	 *
	 *  ⚠️ THE HONEST CAVEAT, DECLARED: the arithmetic above assumes the driver realises the
	 *  commanded unit vector EXACTLY. It does not — the swept call goes through
	 *  `AddMovementInput`, acceleration and `MaxFlySpeed`, so the real convergence LAGS this bound.
	 *  ⇒ these are the steering law's numbers, ⛔ not a promise about the movement component, and
	 *  the entry / arrival-pop log lines this task also ships are what MEASURE the truth in PIE
	 *  (`TASK-805`). ⭐ The bound is the right side of conservative: it is the best case, and the
	 *  margin it leaves is 23×.
	 */
	static constexpr float SteerLookAheadUU = 150.f;

	/**
	 *  ⭐⭐ THE CROSS-TRACK TERM: the unit vector from `CurrentWorld` toward a look-ahead point
	 *  **ON** the climb line, so a pawn admitted OFF the line CONVERGES onto it as it climbs
	 *  instead of riding its entry offset all the way to the deck.
	 *
	 *  ⭐ IT IS A **CLOSED LOOP**, AND THAT IS THE DESIGN DECISION RATHER THAN AN IMPLEMENTATION
	 *  DETAIL (`CONTACT-§14.5` offered an open-loop alternative — "carry the entry error in the
	 *  state and lerp it to zero"): this reads the pawn's REAL position every frame, so a sweep, a
	 *  depenetration, a `MaxFlySpeed` clamp or a dropped `AddMovementInput` cannot desync it. A
	 *  seeded offset decayed by dead reckoning would be a SECOND source of truth about where the
	 *  pawn is, and the first thing that moved the capsule by any other means would silently make
	 *  it wrong — with, once again, no compile error and no test failure. ⇒ refused, and the
	 *  handoff says so.
	 *
	 *  ⭐ ON THE LINE IT RETURNS `ClimbDirection`. That is not a coincidence to be tidied away
	 *  later — it is the REGRESSION GUARANTEE: a pawn that entered dead centre climbs EXACTLY as
	 *  it does today, with ⛔ no wobble and ⛔ no new steering behaviour to review.
	 *
	 *  ⛔ ZeroVector for a degenerate line — the SAME refusal and the SAME value `ClimbDirection`
	 *  returns, so swapping one for the other at a call site cannot acquire a new failure mode.
	 *
	 *  @param CurrentWorld  the pawn's CAPSULE-CENTRE location this frame, in the same space
	 *                       `Advance` and `ShouldSweep` already take it.
	 */
	static FVector SteerDirection(const FSiegeLadderClimbState& State, const FVector& CurrentWorld);

	/**
	 *  ⭐⭐ WHETHER THIS FRAME'S MOVE MAY BE SWEPT — false only inside the DECK BREACH window at
	 *  the line's elevated end, where the capsule must pass THROUGH the deck slab.
	 *
	 *  ⚠️⚠️ THIS IS A SCOPED, MEASURED EXCEPTION TO `TOWER-§8.5`, WHICH REFUSES A RAW
	 *  `SetActorLocation` LERP **OUTRIGHT** — and it is DECLARED, ⛔ not smuggled. That refusal
	 *  names three harms: driving a unit through the tower BODY (⛔ still refused — the standoff
	 *  contract keeps the whole line clear of it), through OTHER UNITS (⛔ still swept for ~78% of
	 *  the line, and depenetration covers the rest), and **through the DECK** — which TASK-737 has
	 *  since measured to be REQUIRED, because the pinned socket is 150 uu inside a solid slab.
	 *  ⇒ The law was written before that measurement existed. `DeckBreachCapsuleHalfHeights`
	 *  carries the full reasoning; the handoff requests the amendment.
	 *
	 *  ⭐ It is a NON-SWEPT CONTINUOUS DRIVE at the same LadderClimbSpeedUU — ⛔ NOT a teleport
	 *  and ⛔ not a lerp of the whole line. Same rate, same straight line, no visual discontinuity:
	 *  a unit that popped 270 uu up a ladder would read as broken, which is the exact failure the
	 *  climb clip exists to prevent.
	 */
	static bool ShouldSweep(const FSiegeLadderClimbState& State, const FVector& CurrentWorld);

	/**
	 *  The EXACT capsule-centre position the climb ends at — `End`, i.e. the destination surface
	 *  plus one capsule half-height. ⭐ The driver snaps here on arrival so the unit finishes
	 *  standing ON the deck rather than wherever the last frame's step happened to land.
	 */
	static FVector ArrivalTarget(const FSiegeLadderClimbState& State);

	/**
	 *  Advances the clock and asks whether the climb CONTINUES.
	 *  Returns true  — keep steering along ClimbDirection this frame.
	 *  Returns false — the climb is over; exactly one of bOutReachedTop / bOutTimedOut is set
	 *                  (both false only when the state was not active to begin with).
	 *  ⚠️ ARRIVAL IS TESTED BEFORE THE WATCHDOG, ON PURPOSE: a unit that reaches the deck on
	 *  the same frame its budget expires has ARRIVED, and must not be reported as a failure.
	 */
	static bool Advance(FSiegeLadderClimbState& State, const FVector& CurrentWorld, float DeltaSeconds,
		bool& bOutReachedTop, bool& bOutTimedOut);

	/**
	 *  ⭐⭐ THE EXACTLY-ONCE LATCH — the single most load-bearing function in this feature.
	 *  Returns true EXACTLY ONCE per successful Begin, and resets the state to its default.
	 *  Every one of the eight exits routes through it, so "restore the movement mode and
	 *  broadcast OnLadderClimbEnded exactly once" is a property of ONE branch rather than a
	 *  promise repeated at eight call sites — and a double exit (death then EndPlay, or a
	 *  listener that aborts from inside the broadcast) is inert by construction.
	 */
	static bool End(FSiegeLadderClimbState& State);
};

//~ ═══════════════════════════════════════════════════════════════════════════════════════
//~  ⭐⭐ EVERYTHING BELOW THIS BANNER IS **TASK-777** (CONTACT-§4), ⛔ NOT TASK-776'S MOVE.
//~ ═══════════════════════════════════════════════════════════════════════════════════════
//~
//~ ⛔⛔ READ THIS BEFORE DIFFING THIS FILE (TASK-779 item (2) asks for a byte-identical diff of
//~ TASK-776's lift against the OLD SummonedUnit.cpp:136-300): ⭐ EVERYTHING **ABOVE** THIS
//~ BANNER IS UNTOUCHED BY TASK-777 — not one expression, default, parameter name or comment of
//~ the moved region changed, and the lift's acceptance criterion is intact. The contact trigger
//~ is APPENDED beneath it precisely so the moved region stays contiguous and diffable from the
//~ top of the file. ⚖️ Attribution written into the source because the alternative is a gate
//~ that fails the wrong task.
//~
//~ ⭐ WHY IT LIVES IN **THIS** PAIR AND ⛔ NOT ON THE TOWER (CONTACT-§8's file map, and
//~ CONTACT-§4.1's "it is a PURE STATIC"): it names FVector and FMath and ⛔ nothing else — ⛔ no
//~ UWorld, ⛔ no AActor, ⛔ no UObject, ⛔ no component, ⛔ no clock, ⛔ no allocation, ⛔ no
//~ logging. ⭐ That is what lets the whole contact mechanism be asserted headlessly, in a suite
//~ with ⛔ not one SpawnActor. ⛔ It also adds NOTHING REFLECTED, so this pair's deliberate
//~ "no .generated.h" property (see the include note at the top) survives intact.
//~
//~ ⚠️⚠️ AND THE MEASUREMENT THAT SHAPES THE WHOLE MECHANISM, OR YOU WILL BUILD THE WRONG THING
//~ (CONTACT-§4's opening box): **THE LADDER HAS ⛔ ZERO COLLISION.** TASK-737 measured it — no
//~ collision hull anywhere over LadderFoot, nearest surface 116 uu. ⇒ ⛔ A pawn walking at the
//~ ladder passes straight THROUGH it and is stopped ~300 uu later by the solid tower body. ⇒
//~ ⛔⛔ AN OVERLAP-ON-BLOCKING-HIT TRIGGER, OR ANYTHING THAT WAITS FOR THE PAWN TO BE STOPPED BY
//~ THE LADDER, WOULD ⛔ NEVER FIRE — ⛔ not by tuning, ⛔ structurally. Jonathan's words
//~ ("walking against it") describe the player's INTENT exactly and the world's GEOMETRY not at
//~ all, so the mechanism is PROXIMITY + INTENT + DWELL.

/**
 *  Transient per-(pawn, tower) state of the contact trigger. ⛔ Deliberately UNREFLECTED, for
 *  the same reason FSiegeLadderClimbState is: nothing here is a UPROPERTY, so nothing here can
 *  be replicated by accident even after a later refactor (CONTACT-§9's M8 declaration, which is
 *  a STRUCTURAL guarantee here rather than a promise).
 *
 *  ⭐ A DEFAULT-CONSTRUCTED VALUE IS "THIS PAWN IS NOT AT THIS LADDER", which is what lets the
 *  owner DROP the whole entry when the pawn leaves the radius: dropping it re-arms the trigger
 *  and prunes the table in one move (see the latch note on Disarm below).
 */
struct FSiegeLadderContactState
{
	/** Seconds the pawn has held proximity AND intent CONTINUOUSLY, at the endpoint named by bDwellAtTop. */
	float DwellSeconds = 0.f;

	/** Which endpoint the dwell is accumulating against — true = the deck TOP (a descent), false = the FOOT (an ascent). Swapping ends restarts the dwell. */
	bool bDwellAtTop = false;

	/** ⛔ K-C's re-arm latch: while true this pawn may ⛔ not start a climb at bDisarmedAtTop's endpoint, however well it satisfies all three terms. */
	bool bDisarmed = false;

	/** Which endpoint the latch above applies to. Meaningless while bDisarmed is false. */
	bool bDisarmedAtTop = false;
};

/** What WantsToClimb decided, in one value. ⛔ Plain enum, ⛔ not a UENUM: nothing reflects it and nothing edits it (the ELadderEntryVerdict precedent). */
enum class ESiegeLadderContactVerdict : uint8
{
	/** ✅ All three terms held for the full dwell. The CALLER still has its own gate to apply. */
	Climb,
	/** ⛔ TERM 1 — outside LadderContactRadiusUU of the nearer endpoint, measured in 2D. */
	TooFar,
	/** ⛔ TERM 2 — not walking INTO the ladder (or not walking at all). */
	NotHeadingIn,
	/** ⏳ TERMS 1 AND 2 HOLD, term 3 does not yet. ⛔ Not a refusal — the pawn is mid-dwell. */
	Dwelling,
	/** ⛔ K-C's re-arm latch: this pawn just finished a climb at this endpoint and has not yet left the radius or the cone. */
	Disarmed
};

/**
 *  ═══ THE CONTACT TRIGGER'S DECIDABLE LOGIC, AS PURE FUNCTIONS (TASK-777, CONTACT-§4.1) ═══
 *
 *  ⭐⭐ THE ONE THING TO UNDERSTAND ABOUT THE THREE TERMS: **the DWELL is what makes the other
 *  two safe**, and it is load-bearing for the UNIT half specifically. Without it a friendly
 *  unit marching past its own tower toward the enemy castle clips the intent cone for two
 *  frames and is YANKED 1,200 uu INTO THE AIR. ⚖️ Dwell is the cheapest honest translation of
 *  "walking *against* it" into a world where there is nothing to press against.
 *
 *  ⭐ AND THE ARITHMETIC THAT SHOWS THE THREE SHIPPED NUMBERS ACTUALLY COOPERATE — worth
 *  writing down because "it felt about right" is not reviewable. Take a pawn crossing the
 *  trigger disc in a straight line at speed v, at perpendicular offset d from the endpoint.
 *  With the cone measured PAWN → ENDPOINT, the endpoint is within a 60° half-cone of the
 *  heading only while the pawn is still `d / tan 60° = d / 1.732` short of its closest
 *  approach, so the time it can accumulate is `(sqrt(R² − d²) − d/1.732) / v`. At R = 150 and
 *  v = 300 uu/s that is 0.50 s dead-on (d = 0), 0.375 s at d = 50, **0.29 s at d = 75** and
 *  0.18 s at d = 100. ⇒ ⭐ against a 0.35 s dwell the trigger admits a pawn aimed within
 *  ~60 uu of the ladder and refuses everything marching past outside that — which is exactly
 *  the line between "walking into it" and "walking past it".
 *
 *  ⚠️⚠️ AND THAT ARITHMETIC IS ALSO WHY THE CONE IS MEASURED **PAWN → ENDPOINT** AND ⛔ NOT
 *  ALONG THE CLIMB LINE'S OWN HORIZONTAL DIRECTION, WHICH LOOKS TIDIER AND IS **WRONG**: the
 *  line's horizontal direction is CONSTANT, so a pawn merely marching east past the tower would
 *  satisfy it for the WHOLE chord — `2·sqrt(R² − d²)/v` = 0.75 s at d = 100 — and the dwell
 *  would stop protecting anything. ⛔ The bearing MUST swing as the pawn passes; that swing IS
 *  the discrimination.
 *
 *  ⛔ EVERYTHING HERE IS PURE (the FSiegeLadderClimbStatics / FSiegeStuckStatics idiom): every
 *  input is a parameter, every output is a return value or a write to the caller's state.
 */
struct FSiegeLadderContactStatics
{
	/**
	 *  Below this horizontal speed a pawn is STANDING STILL for this feature's purposes.
	 *
	 *  ⭐ DERIVED, ⛔ NOT FELT, and ⛔ NOT a fourth tunable (CONTACT-§7 K-5 names exactly
	 *  THREE): at 1 uu/s a pawn covers less than 0.35 uu across the whole shipped dwell — a
	 *  hundredth of its own capsule radius, i.e. less than the navmesh, the sweep or the eye can
	 *  resolve. It exists so that residual braking velocity, or a pawn nudged by depenetration,
	 *  cannot be read as "walking into the ladder". The MinClimbLineUU idiom: a math guard, ⛔
	 *  not a policy.
	 */
	static constexpr float MinContactSpeedUU = 1.f;

	/**
	 *  ⭐ WHICH END OF THE LADDER THIS PAWN IS AT — **RESOLVED BY Z**, exactly as
	 *  `TOWER-§8.5a` clause 1 resolves which end of the climb line is the deck (K-C: BOTH
	 *  endpoints arm the trigger, so walking into the ladder ON THE DECK descends).
	 *
	 *  ⛔ Z AND ⛔ NOT 2D DISTANCE, AND THE REASON IS ARITHMETIC RATHER THAN TASTE: the pinned
	 *  endpoints are only **300 uu apart in XY** while being 1,200 uu apart in Z, so their two XY
	 *  discs OVERLAP and a pawn standing on the ground can fall inside the TOP's disc. Z separates
	 *  them by 1,200 uu and cannot.
	 *
	 *  ⚠️⚠️ REPAIRED 2026-09-02 (TASK-787 item (6), raised by TASK-786) — THIS PARAGRAPH USED TO
	 *  ASSERT THE DISCS ARE **TANGENT AT A 150 uu RADIUS**, AND IT NAMED THE PRE-TASK-785 SOCKET
	 *  LITERALS (`foot x −450, top x −150`). ⛔ BOTH WERE STALE:
	 *    • the shipped radius is `CONTACT-§7b`'s **K-6.1 = 350** (150 → 300 → 350), at which the
	 *      discs overlap in a **2R − d = 400 uu LENS** and each endpoint's XY centre sits
	 *      **R − d = 50 uu INSIDE** the other's disc ⇒ the Z resolution is load-bearing at ⛔ EVERY
	 *      approach, ⛔ not merely at one tangent point;
	 *    • the sockets moved WEST by 10 uu (TASK-785 / `K-1` option A) to −460 / −160 — ⭐ a PURE
	 *      translation, so the **separation is still exactly 300** and every number above survives.
	 *  ⭐⭐ AND THE PROPERTY WORTH KEEPING IS THAT THIS FUNCTION TAKES ⛔ NO RADIUS TERM AT ALL: no
	 *  retune of `LadderContactRadiusUU` can break it, at any value, ⛔ by construction rather than
	 *  by re-derivation. ⚠️ A change to the 1,200 uu rise (`PlatformHeightUU`) is the one that could.
	 *
	 *  ⚠️ A tie reads as the FOOT (an ascent) — the same tie-break `HandleLadderLinkReached`
	 *  already applies to the link's destination point, kept identical so the two paths can
	 *  never disagree about which way a degenerate line goes.
	 */
	static bool IsAtTopEndpoint(const FVector& PawnLocation, const FVector& FootWorld, const FVector& TopWorld);

	/**
	 *  ⭐⭐ THE THREE-TERM PREDICATE (CONTACT-§4.1). ⛔ ALL THREE ARE REQUIRED.
	 *
	 *    1. PROXIMITY  — within RadiusUU of the nearer endpoint, measured **2D (XY)**. ⛔ 2D
	 *                    because the endpoint is a SURFACE point while the pawn's origin is its
	 *                    capsule CENTRE; a 3D test would silently need `TOWER-§8.5a` clause 6's
	 *                    half-height lift just to be satisfiable, and would then be wrong for
	 *                    any pawn with a different capsule.
	 *    2. INTENT     — the pawn's horizontal MOVEMENT DIRECTION within `acos(IntentCos)` of
	 *                    the horizontal direction from the pawn to that endpoint. ⭐ It reads
	 *                    DIRECTION and ⛔ NEVER A KEY, which is the only shape that works
	 *                    identically for a player-driven pawn and an AI-driven one — and
	 *                    `RECALL-§2` / `KBD-§` would forbid a hardcoded key anyway.
	 *    3. DWELL      — terms 1 and 2 must hold CONTINUOUSLY, at the SAME endpoint, for
	 *                    RequiredDwellSeconds.
	 *
	 *  ⚠️ THE DWELL IS ⛔ NOT CONSUMED BY A `Climb` VERDICT, DELIBERATELY. It stays satisfied
	 *  while the terms hold, so a pawn refused by the CALLER's gate (an occupied ladder — K-D's
	 *  "the second waits in its ordinary walking state") enters the instant the ladder frees,
	 *  rather than serving a fresh 0.35 s sentence it did nothing to earn. ⛔ The thing that
	 *  prevents an immediate RE-entry is the latch below, ⛔ never a consumed dwell.
	 *
	 *  ⚠️ A pawn standing EXACTLY on the endpoint has no defined 2D direction toward it, so
	 *  term 2 fails and it does not climb. ⭐ That is the conservative answer and it costs
	 *  nothing in practice: at the shipped numbers the trigger fires ~55 uu BEFORE the pawn
	 *  reaches the foot (0.35 s × 300 uu/s = 105 uu of a 150 uu approach), so the degenerate
	 *  point is not on the path a walking pawn takes.
	 *
	 *  @param bOutAscending  true when the resolved endpoint is the FOOT (climb UP), false when
	 *                        it is the deck TOP (climb DOWN). ⛔ Written on EVERY path, so a
	 *                        caller that reads it after a refusal reads a defined value.
	 */
	static ESiegeLadderContactVerdict WantsToClimb(
		FSiegeLadderContactState& State,
		const FVector& PawnLocation,
		const FVector& PawnVelocity,
		const FVector& FootWorld,
		const FVector& TopWorld,
		float RadiusUU,
		float IntentCos,
		float RequiredDwellSeconds,
		float DeltaSeconds,
		bool& bOutAscending);

	/**
	 *  ⛔⛔ K-C'S RE-ARM LATCH, AND IT IS MANDATORY RATHER THAN DEFENSIVE PADDING — THE DEFECT
	 *  IT PRE-EMPTS IS CERTAIN, ⛔ NOT HYPOTHETICAL: a pawn that finishes a DESCENT is standing
	 *  at the ladder FOOT, inside the radius, still supplying the very input that brought it
	 *  there ⇒ ⛔ it re-climbs INSTANTLY and the player is stuck in a yo-yo. The mirror case is
	 *  just as certain: a pawn that finishes an ASCENT is standing on the deck at the top.
	 *
	 *  ⇒ The owner calls this when ANY climb ends, passing the endpoint the pawn is nearest
	 *  (IsAtTopEndpoint, so entry and re-arm resolve the same way). That endpoint then stays
	 *  DISARMED for that pawn until it leaves the radius OR drops out of the intent cone.
	 *
	 *  ⭐ Stepping off the deck edge stays legal and free (`TOWER-§8.7`); this latch gates the
	 *  LADDER, ⛔ never the ledge.
	 */
	static void Disarm(FSiegeLadderContactState& State, bool bAtTop);
};
