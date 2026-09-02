// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LadderClimber.generated.h"

// TASK-787 (CONTACT-§12.3): FSiegeLadderClimbEnded's first parameter is an `ACharacter*` — a
// POINTER parameter, so a forward declaration is everything the delegate and its UHT wrapper need
// (the shipped `Castle.h` → `class ACastle;` precedent, and the generated header emits its own
// declaration as well). ⛔ Deliberately NOT `#include "GameFramework/Character.h"`: this header is
// reached by every climber class and stays as light as `TeamId.h` / `HealthBarProvider.h`.
class ACharacter;

/**
 *  ⭐⭐ THE CLIMB'S ONE COMPLETION SIGNAL — **MOVED HERE FROM `SummonedUnit.h` BY TASK-787**
 *  (`CONTACT-§12.3`; `CONTACT-§8`'s "the delegate's HOME" row). ⚠️ Its old home was that file's
 *  line 115; the number is written as HISTORY and is ⛔ not a live cite — the move itself
 *  invalidated it, which is `CONTACT-§10.1`'s whole point.
 *
 *  ⛔⛔ IT IS A **MOVE**, ⛔ NOT AN AMENDMENT: the TYPE NAME and the SIGNATURE are UNCHANGED, so
 *  `TOWER-§8.4(B)`'s second amendment is INTACT and the rename to `FOnLadderClimbEnded` that was
 *  proposed and REFUSED stays refused. ⭐ WHY IT BELONGS HERE: the delegate is CLIMBER vocabulary,
 *  ⛔ not UNIT vocabulary — the same argument this header already makes about `AbortLadderClimb()`.
 *  It only ever lived on `ASummonedUnit` because the unit was the only climber; `AHeroCharacter`
 *  is the second, and `AClimbableTower` must reach BOTH through one type.
 *
 *  Fired EXACTLY ONCE per successful `BeginLadderClimb` — on arrival, abort, a new order, death,
 *  either freeze, EndPlay, or the watchdog (`TOWER-§8.5`'s eight exits for the unit; the hero's
 *  TEN, all of which route through its ONE teardown). `bReachedTop` is true ONLY for arrival.
 *
 *  ⭐ This is `AClimbableTower`'s ONLY completion signal, which is what lets that class keep its
 *  no-tick property: ⛔ no tick and ⛔ no timer is added there to watch a climber.
 *  ⚠️⚠️ AND IT IS WHAT KEEPS THE LADDER FROM BRICKING (`CONTACT-§12.1`): a climber admitted to the
 *  occupancy slot that never broadcasts is a climber the tower never releases — after which every
 *  later climber, hero or unit, gets `LadderBusy` for the rest of the match.
 *
 *  ⛔ SIGNATURE PINNED CHARACTER-FOR-CHARACTER IN `TOWER-§8.4(B)`. TASK-734 compiles against it.
 *
 *  ⚖️⭐ AMENDED 2026-09-01 (TASK-777, `CONTACT-§4.4` — `TOWER-§8.4(B)`'s SECOND amendment, landed
 *  as a LAW AMENDMENT and ⛔ never silently). EXACTLY ONE THING CHANGED: the first parameter's
 *  TYPE, `ASummonedUnit*` → `ACharacter*`.
 *    ⛔ The delegate TYPE NAME is UNCHANGED — it is still `FSiegeLadderClimbEnded`.
 *    ✅ The parameter NAME Unit → Climber, safe by the `FromWorld`/`ToWorld` precedent: parameter
 *      names are ⛔ not part of a function's type.
 *    ⭐ `ASummonedUnit`'s own `Broadcast(this, bReachedTop)` compiles UNTOUCHED through the
 *      implicit `ASummonedUnit*` → `ACharacter*` conversion, and so does the hero's.
 *  ⚠️⚠️ WHY IT HAD TO CHANGE, AND IT IS A ⛔ MEASURED DEFECT: `AClimbableTower`'s occupancy slot was
 *  typed to this signature, so a HERO climber could never occupy it — invisible to `TOWER-§10`
 *  L-1's one-at-a-time rule AND to the tower's `EndPlay` abort. ⇒ the tower falls and the hero
 *  HANGS in `MOVE_Flying` forever, because `MOVE_Flying` ignores gravity.
 *
 *  ⛔ IT MAY ⛔ NOT LIVE IN `SiegeLadderClimbStatics.{h,cpp}` (`CONTACT-§12.3`): that pair states at
 *  its top *"⛔ No .generated.h: nothing in this pair is reflected, deliberately"*, and a
 *  `DECLARE_DYNAMIC_MULTICAST_DELEGATE` FORCES one. This header already carries a `.generated.h`
 *  because it is a `UINTERFACE`, so hosting it here costs ⛔ nothing.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop);

/**
 *  ═══ THE CLIMBER CAPABILITY SEAM (TASK-777; CONVENTIONS CONTACT-§4.4 / CONTACT-§8) ═══
 *
 *  ⭐ WHAT IT EXISTS FOR, IN ONE SENTENCE: `AClimbableTower::EndPlay` must be able to abort a
 *  climb it started, and after `CONTACT-§4.4` widened the occupancy slot to `ACharacter` the
 *  thing it holds no longer carries `AbortLadderClimb()` — that is `ASummonedUnit` vocabulary,
 *  ⛔ not an `ACharacter` member.
 *
 *  ⚠️⚠️ AND THE CONSEQUENCE OF NOT HAVING THIS IS THE WORST FAILURE IN THE FEATURE, ⛔ NOT A
 *  TIDINESS COMPLAINT: `MOVE_Flying` IGNORES GRAVITY. A tower that dies under a climber it
 *  cannot reach leaves that climber HANGING IN THE AIR FOREVER — and for `AHeroCharacter` that
 *  is the PLAYER'S OWN BODY, in a match that keeps running around it (`CONTACT-§3.1` exit H-10).
 *
 *  ⛔⛔ THE TWO SHAPES THAT WERE REFUSED, RECORDED SO NEITHER IS RE-PROPOSED AS "SIMPLER":
 *    • ⛔ A `Cast<ASummonedUnit>` / `Cast<AHeroCharacter>` BRANCH PAIR. It hardcodes the class
 *      list, and it is the SAME failed-cast shape that CAUSED the defect `CONTACT-§4.4` fixes
 *      (`ClimbableTower.cpp`'s old `Cast<ASummonedUnit>` on the occupancy slot). ⚖️ Widening a
 *      type until the failure moves is not fixing it.
 *    • ⛔ PUTTING THIS INTERFACE IN `SiegeLadderClimbStatics.h`. That header states at its top
 *      *"⛔ No .generated.h: nothing in this pair is reflected, deliberately"*, and a
 *      `UINTERFACE` would FORCE one into it — spending the exact property TASK-776 was told to
 *      preserve byte-for-byte. ⭐ MEASURED, ⛔ not style.
 *
 *  ⭐ THE SHIPPED PROJECT PATTERN, THIRD APPLICATION — one concept, one header, `MinimalAPI`
 *  and `NotBlueprintable`, pure-virtual C++ methods and ⛔ no `UFUNCTION`s: `TeamId.h` →
 *  `ITeamAgent`, `HealthBarProvider.h` → `IHealthBarProvider`. ⛔ Deliberately NOT `UFUNCTION`s
 *  (the `IHealthBarProvider` half of the precedent rather than the `ITeamAgent` half): the two
 *  implementers already declare these names as their own `UFUNCTION`s, and a reflected
 *  interface method would have to agree with them specifier-for-specifier or fail in UHT for a
 *  reason no reader would connect to this file. Nothing here needs to be callable from
 *  Blueprint.
 *
 *  ⛔ THE SURFACE IS **FOUR** METHODS AND IT IS CLOSED AGAIN (`CONTACT-§8`'s amended row pins it):
 *  `AbortLadderClimb()` · `IsClimbing() const` · `BeginLadderClimb(const FVector&, const FVector&)`
 *  · `GetOnLadderClimbEnded()` — ⛔ and nothing else.
 *
 *  ⚠️⚠️ WIDENED 2026-09-02 FROM TWO TO FOUR BY TASK-787 (`CONTACT-§12`), AND THE PARAGRAPH THAT
 *  USED TO STAND HERE IS ⛔ REPAIRED RATHER THAN DELETED (`CONTACT-§10.1`'s discipline). It read:
 *  *"`BeginLadderClimb` is ⛔ NOT here … `CONTACT-§2` ruled that each pawn class keeps its OWN
 *  driver, so STARTING a climb is the pawn's business"*. ⚖️⭐ THAT REASONING WAS WRONG ABOUT WHAT
 *  `CONTACT-§2` RULED: `§2` refused a shared ***DRIVER*** — the per-frame interpolation and the
 *  exits — and it ⛔ never said the tower may not ASK a pawn to start. ✅ PROOF FROM THE SHIPPED
 *  CODE: the tower has ⛔ ALWAYS been the caller (`ClimbableTower.cpp`'s two `BeginLadderClimb`
 *  call sites, one per entry path). ⇒ the old pin removed ⛔ nothing but the tower's ability to
 *  reach a SECOND class, and the DRIVER stays per-class exactly as `§2` ruled.
 *
 *  ⭐⭐ AND THE TWO ADDITIONS ARE ⛔ TWO HALVES OF ONE SEAM, ⛔ NEVER ONE WITHOUT THE OTHER
 *  (`CONTACT-§12.1`): a tower that can START a climber it cannot learn the END from claims the
 *  occupancy slot and ⛔ never releases it. ⚠️ A START-ONLY widening is ⛔ WORSE than no widening —
 *  it converts *"the hero cannot climb"* into *"the FIRST hero attempt disables the tower for
 *  everyone, for the rest of the match."*
 *
 *  ⭐ THE SIGNATURES ARE ⛔ NOT INVENTED: both implementers already shipped
 *  `bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)` character-for-
 *  character before this widening, so ⛔ neither declaration had to change.
 *
 *  ⛔ M8 (`CONTACT-§9`): an interface declares ⛔ no state, so there is ⛔ nothing here to
 *  replicate and ⛔ no RPC. ⭐ THAT CLAUSE IS ALSO WHY THE COMPLETION SEAM IS AN **ACCESSOR** AND
 *  ⛔ NOT THE DELEGATE ITSELF: each implementer owns the INSTANCE (`ASummonedUnit`'s and
 *  `AHeroCharacter`'s own `UPROPERTY(BlueprintAssignable)`); the interface owns only the way to
 *  reach it. ⚠️ Stated rather than omitted — `WR-§8` warns that "nothing to
 *  declare" is a sentence copied from other batches; here it is true for a structural reason
 *  and the reason is the declaration. The climb STATE's tier is declared by its owners.
 *
 *  IMPLEMENTED BY: `ASummonedUnit` (TASK-777, the one base-list entry) and `AHeroCharacter`
 *  (TASK-778). ⚠️ An implementer's methods do ⛔ NOT need the `virtual` keyword repeated — a
 *  matching signature overrides regardless — which is why neither implementer's shipped
 *  `BeginLadderClimb` declaration had to change when TASK-787 pulled it up here.
 *  ⭐ `GetOnLadderClimbEnded()` is the ONE thing both implementers had to ADD: a one-line inline
 *  returning the delegate instance each of them already owns (`ASummonedUnit`) or now owns
 *  (`AHeroCharacter`, TASK-787). ⛔ Neither gained a second delegate TYPE — that shape is refused
 *  at `CONTACT-§12.4`.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class ULadderClimber : public UInterface
{
	GENERATED_BODY()
};

class ILadderClimber
{
	GENERATED_BODY()

public:

	/**
	 *  Ends an in-flight climb WHEREVER the climber is: restores the movement mode, clears the
	 *  climb state, and lets the body drop. ⛔ MUST BE IDEMPOTENT — `AClimbableTower::EndPlay`
	 *  calls it and then calls `ReleaseClimber`, which the implementer's own completion
	 *  broadcast may already have re-entered.
	 *
	 *  ⛔ TAKES NO REASON, AND THAT IS PINNED (`TOWER-§8.4(B)`): the teardown is
	 *  REASON-AGNOSTIC by design, and a reason parameter is the first line of the exit
	 *  bookkeeping `TOWER-§4` refuses.
	 *
	 *  ✅ Dropping is FREE: there is ⛔ no fall damage anywhere in Siegebound (`TOWER-§4a`,
	 *  measured).
	 */
	virtual void AbortLadderClimb() = 0;

	/** True for the whole traversal, false at every one of the implementer's exits. Read by the tower to ask whether a climb it is tracking is still live. */
	virtual bool IsClimbing() const = 0;

	/**
	 *  ⭐ SEAM 1 OF 2 (TASK-787, `CONTACT-§12.2`) — **THE TOWER ASKS THE PAWN TO START.** Begins a
	 *  scripted traversal along the straight world-space line From -> To. Returns false and changes
	 *  ⛔ NOTHING when the implementer refuses (dead, frozen, already climbing — the hero adds
	 *  recall-channelling, `CONTACT-§3.4`).
	 *
	 *  ⛔⛔ THE PAWN ASKS; THE TOWER DECIDES (`CONTACT-§12`'s ruling). This method exists so that
	 *  `AClimbableTower` can start ⛔ ANY admitted climber without naming a concrete class — it is
	 *  ⛔ NOT an invitation for a pawn to drive itself onto a ladder. A self-start skips
	 *  `CanTeamAscend` (a silent back door around `T-3`), skips the occupancy slot (`TOWER-§10`
	 *  L-1) and leaves `ActiveClimber` unset, which DELETES exit `H-10`.
	 *
	 *  ⛔ It is From -> To, ⛔ NOT foot-then-top: the link is `ENavLinkDirection::BothWays`, so a
	 *  DESCENT passes the same two points the other way round. ⛔ NO Z-ordering is implied here and
	 *  ⛔ nothing may be added that implies one.
	 *
	 *  ⭐ The two points are SURFACE positions; each implementer lifts them by ITS OWN capsule
	 *  half-height (`CONTACT-§3.3`: the hero's is 96, the unit's is 88 — ⛔ never inherited).
	 */
	virtual bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld) = 0;

	/**
	 *  ⭐⭐ SEAM 2 OF 2 (TASK-787, `CONTACT-§12.3`) — **THE ONE THAT KEEPS THE LADDER FROM
	 *  BRICKING.** The implementer's own `FSiegeLadderClimbEnded` instance, which the tower binds
	 *  BEFORE it calls `BeginLadderClimb` and unbinds in `ReleaseClimber`.
	 *
	 *  ⛔⛔ WITHOUT THIS THE WIDENING IS WORSE THAN NOTHING: the tower learns a climb ended through
	 *  ⛔ exactly one channel, so a climber it can START but not HEAR FROM holds the occupancy slot
	 *  FOREVER and every later climber — hero or unit — gets `LadderBusy` for the rest of the match.
	 *
	 *  ⭐ AN ACCESSOR RATHER THAN A MEMBER because an interface declares ⛔ no state (the M8 clause
	 *  above). ⛔ Non-const on purpose: the caller BINDS to it.
	 *
	 *  ✅⭐⭐ AND THIS WHOLE SHAPE IS THE ⛔ SHIPPED PROJECT IDIOM, ⛔ NOT A NEW ONE — ⭐ MEASURED, and
	 *  it is the strongest evidence available that it compiles: `HealthBarProvider.h` declares
	 *  `FOnCombatantHPChanged` beside `IHealthBarProvider`, the interface exposes
	 *  `virtual FOnCombatantHPChanged& GetHPChangedDelegate() = 0;` (`:66`), and every implementer
	 *  owns a `UPROPERTY` instance returned by a one-line inline override — `ABuilding.h:75/:78`,
	 *  and ⭐ `AHeroCharacter` ITSELF at `HeroCharacter.h:437/:440`. ⇒ delegate-in-the-interface's-
	 *  header + accessor-in-the-interface + instance-on-the-implementer is a pattern this module
	 *  already builds, ⛔ on this very class.
	 *
	 *  ⚠️ THE BROADCAST'S ORDERING IS THE IMPLEMENTER'S CONTRACT, ⛔ not this header's to enforce:
	 *  broadcast LAST, after the movement mode is restored and after the exactly-once latch is
	 *  consumed, so a listener that re-enters `AbortLadderClimb()` is inert rather than recursive.
	 */
	virtual FSiegeLadderClimbEnded& GetOnLadderClimbEnded() = 0;
};
