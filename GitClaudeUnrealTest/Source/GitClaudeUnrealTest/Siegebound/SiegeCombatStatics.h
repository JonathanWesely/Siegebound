// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/NumericLimits.h" // TASK-838: TNumericLimits<float>::Max() is FSiegeVisionQuery's "no reach of its own" sentinel — named explicitly per IWYU rather than leaned on transitively, because it is used in a HEADER
#include "Templates/SubclassOf.h"
#include "Siegebound/TeamId.h"

class AActor;
class AController;
class UDamageType;
class UWorld;

// TASK-838 (FOG-§6): the fog band this file hands to FSiegeFogStatics. Forward-declared rather
// than included so SiegeCombatStatics.h stays free of the fog header — the fog rule is consumed
// in SiegeCombatStatics.cpp and nowhere else, which is what keeps the "applied at ONE place"
// claim checkable by reading a single translation unit.
struct FSiegeFogTuning;

/**
 *  ⭐⭐ HOW ONE GATHER TREATS A ***VEILED*** CANDIDATE (TASK-829, law `WITCH-§2`).
 *
 *  ⛔⛔ THIS TYPE EXISTS FOR EXACTLY ONE REASON, AND IT IS ⛔ NOT CONFIGURABILITY. `WITCH-§2`
 *  rules the nine acquisition sites and the AoE lane ⛔ DIFFERENTLY, and both of them come out
 *  of the SAME gather:
 *    ⭐ ACQUISITION ("what can this thing SEE and shoot at") ⇒ a veiled ENEMY is ⛔ NOT returned.
 *      ⭐ That ⛔ IS the card.
 *    ⛔ BLAST (`ApplyRadialDamage`) ⇒ a veiled enemy ⛔ IS returned and ⛔ IS damaged.
 *      ⚖️ ***A blast is ⛔ not an act of seeing*** (`WITCH-§2`, ruling `J-W2`) — and it is the
 *      50-gold card's ⛔ ONLY counter. A Fireball / Sapper / Bomb Tower must still flush a
 *      veiled push, or the card is an auto-win.
 *
 *  ⭐ THE DEFAULT IS THE ⛔ SAFE DIRECTION, AND THAT IS DELIBERATE. `GatherHostileAgents`
 *  defaults to `SuppressVeiled`, so a ⛔ TENTH acquisition site written next month — by
 *  somebody who has never read `WITCH-§` — honours the veil ⛔ by omission. ⛔ Only the ONE
 *  lane that must see through it says so, ⛔ out loud, at its call site.
 *  ⚠️ A default argument is normally the shape `GHOST-§1` distrusts ("a flag can be forgotten
 *  at a guard point"). ⭐ It is right HERE because forgetting it yields the ⛔ CONSERVATIVE
 *  answer: the veil holds. The failure mode of the opposite default is a veiled unit that is
 *  visible to a site nobody remembered to update, which is the bug report "invisibility is broken".
 *
 *  📌 TASK-838 NOTE, so a reader does not mistake it for a change of mind: the five VISION sites
 *  now spell `SuppressVeiled` out loud too. That is ⛔ not a new opinion about the veil — it is
 *  C++: they must name this argument to reach the `Vision` parameter that follows it. ⭐ The
 *  default is untouched and still does its whole job — a TENTH acquisition site written next
 *  month, by somebody who has never read `WITCH-§`, still honours the veil ⛔ by omission.
 *
 *  Plain `enum class`, ⛔ NOT a `UENUM` — the `ESiegeVeilBreakReason` / `ESiegeLadderExit`
 *  discipline (`WITCH-§6`'s M8 declaration): it is never a `UPROPERTY`, and an unreflected type
 *  ⛔ cannot be replicated by accident. `WITCH-§6` records that a naively replicated veil ⛔ leaks
 *  the veiled unit's position to the enemy client's renderer.
 *
 *  📌 NOT a one-class-per-header violation — the shipped "pure data types may share a header when
 *  they form one concept" exception (`SiegeInvisibilityStatics.h` holds `ESiegeVeilBreakReason`
 *  beside `FSiegeInvisibilityStatics`; `SiegeStuckStatics.h` holds three). This enum is the
 *  vocabulary of the two gathers declared below and has no meaning away from them.
 */
enum class ESiegeVeilPolicy : uint8
{
	/** ⭐ ACQUISITION: a veiled ENEMY is dropped from the result. ⛔ A veiled ALLY never is (`WITCH-§2` lane 4 — the predicate answers "visible" for every same-team query). */
	SuppressVeiled,

	/** ⛔ BLAST ONLY: veiled enemies are returned. ⚖️ `WITCH-§2` / `J-W2` — a blast is not an act of seeing, and it is the card's counter. ⛔ Do not use this to "fix" an acquisition site. */
	IncludeVeiled
};

/**
 *  ⭐⭐ ONE ACT OF SEEING: WHERE A QUERY LOOKS FROM, AND HOW FAR IT IS ASKING TO SEE
 *  (TASK-838, law `FOG-§7`).
 *
 *  ⛔⛔ THIS TYPE IS THE EXEMPTION, AND THAT IS ITS ENTIRE JOB. `WITCH-§1` made ONE chokepoint,
 *  which was right — but it means `GatherHostileAgents` is now the shared road for consumers
 *  that are ⛔ NOT acts of seeing, and a BLIND clamp there would answer three different
 *  questions with one answer. `FOG-§7`'s taxonomy, measured:
 *
 *    row 1  ⭐ VISION / ACQUISITION — "what can this thing SEE and shoot at"
 *           `ASummonedUnit` ×2 · `ATower` ×2 · `AHeroCharacter`   ⇒ ✅ CLAMPED. This IS the card.
 *    row 2  ⛔ BLAST — "what is INSIDE this explosion" (`ApplyRadialDamage`)      ⇒ NOT clamped
 *    row 3  ⛔ DIRECTED SPELL GEOMETRY — "what lies ALONG the line the caster AIMED"
 *           (`USpellLibrary`'s shape gathers · `ASpellLineSweep`)                 ⇒ NOT clamped
 *    row 4  ⛔ CHEAT LANE (`USiegeCheatManager`, ROUTED not re-enumerated)         ⇒ NOT clamped
 *    row 5  ⛔ FRIENDLY BUFF (`ResolveAllyBuff`) — a different gather entirely     ⇒ NOT clamped
 *
 *  ⭐ THE EXEMPTION IS EXPRESSED BY WHAT THE CALL SITE HANDS OVER — ⛔ never by a branch inside
 *  the funnel that inspects its caller, ⛔ never by a magic list of exempt sites, ⛔ never by an
 *  `if` on a call-site enum invented for this. A `switch (CallerKind)` inside
 *  `GatherHostileAgents` is an AUTOMATIC QA FAIL (`TASK-838(1b)`). The four unclamped lanes pass
 *  ⛔ NOTHING; they cannot be accidentally given a vision radius, because they never construct
 *  one. And the constructors below are NAMED for the act: `SeeingFrom(Center, BlastRadius)` at a
 *  blast would ⛔ READ WRONG on the page, which is the guard `FOG-§7` asked for.
 *
 *  ⛔⛔ WHY THIS IS NOT A CLAMP AT THE CALL SITE — the distinction is the whole design.
 *  The site supplies only the two facts ⛔ only it knows: where it is looking FROM, and the reach
 *  it is looking WITH. It performs no min(), consults no ceiling, names no fog symbol and does
 *  not know whether fog exists. `Tests/SiegeAcquisitionFunnelTest.cpp` test 9 asserts exactly
 *  that: `FSiegeFogStatics` and `EffectiveVisionRadius` appear ZERO times in all five call-site
 *  files, "including for TASK-829 and TASK-838, which must NOT relax this row."
 *
 *  ⛔⛔ AND IT IS NOT A TEAM PARAMETER. Nothing here names a team, a viewer, a controller or a
 *  player. Fog is SYMMETRIC (`J-F3`, his own words: "And that includes AI and all units") and
 *  `FOG-§7a` enforces that STRUCTURALLY — an asymmetric fog is unrepresentable because there is
 *  no parameter to branch on. ⚠️ Note the deliberate contrast with the veil above, which IS
 *  per-viewer. The two cards ask different questions of one funnel: invisibility filters WHICH
 *  ACTORS come back (per-actor, per-viewer); fog clamps HOW FAR THE QUERY REACHES (per-query,
 *  team-blind). ⛔ A task that "harmonises" them has misread both (`TASK-838(1c)`).
 *
 *  Plain `struct`, ⛔ not a `USTRUCT` — the `ESiegeVeilPolicy` discipline directly above: it is
 *  never a `UPROPERTY`, and an unreflected type cannot be replicated by accident.
 */
struct FSiegeVisionQuery
{
	/** World-space point the query is looking FROM — the viewer's own location, never a target's. */
	FVector ViewOrigin = FVector::ZeroVector;

	/**
	 *  The reach this site is asking for, ⛔ BEFORE fog. `TNumericLimits<float>::Max()` means
	 *  "this site has no reach of its own" — see `SeeingFromUnbounded` below.
	 *  ⛔ A BLAST RADIUS IS NOT A VISION RADIUS AND MUST NEVER BE PUT HERE (`SiegeFogStatics.h`).
	 */
	float RequestedRadiusUU = TNumericLimits<float>::Max();

	/**
	 *  A site that HAS a reach of its own: a unit's `AggroRadius`, a tower's `AttackRange`, the
	 *  hero's `MeleeRange`. ⭐ Fog can only ever SHORTEN it — `EffectiveVisionRadius` is `min`,
	 *  never `clamp`, so a Cleric's 400 stays 400 and a melee 120 stays 120 (`FOG-§2`).
	 */
	static FSiegeVisionQuery SeeingFrom(const FVector& InViewOrigin, float InRequestedRadiusUU)
	{
		FSiegeVisionQuery Query;
		Query.ViewOrigin = InViewOrigin;
		Query.RequestedRadiusUU = InRequestedRadiusUU;
		return Query;
	}

	/**
	 *  ⭐ A site whose eligibility is NOT a range from the viewer — a commanded unit's zone disc
	 *  (`AcquireEnemyNearPoint`, gated on a disc around a COMMANDED POINT) and a chain zap's
	 *  fire-time snapshot (`FireChainZapAt`, whose bounces are measured from the PREVIOUS target,
	 *  deliberately not from the tower). Both are still acts of seeing, and both must be blinded
	 *  by fog — but neither has a number to hand over.
	 *
	 *  ⛔⛔ UNBOUNDED IS THE ONLY BEHAVIOUR-NEUTRAL ANSWER, AND THAT IS WHY IT IS THE ANSWER AND
	 *  NOT A SHORTCUT. Handing over some OTHER radius (a unit's `AggroRadius`, a tower's
	 *  `AttackRange`) would narrow these two sites ⛔ WITH FOG OFF — a shipped behaviour change
	 *  wearing a fog card's commit message. With fog OFF, `EffectiveVisionRadius` returns `Max`
	 *  bit-identically ⇒ nothing is cut; under fog it returns the ceiling ⇒ the site sees exactly
	 *  as far as everything else does.
	 */
	static FSiegeVisionQuery SeeingFromUnbounded(const FVector& InViewOrigin)
	{
		FSiegeVisionQuery Query;
		Query.ViewOrigin = InViewOrigin;
		return Query;
	}
};

/**
 *  Shared Siegebound combat statics (TASK-055). Free-standing helpers used by more
 *  than one gameplay actor — authored ONCE here so callers never hand-mirror the
 *  logic (the qa/TASK-026 NIT-4 "never add a fourth mirror" discipline).
 *
 *  Not a UObject / not reflected: a plain static library, so there is no BeginPlay,
 *  no GC surface, and no Build.cs change (same module — only Core/Engine deps that
 *  are already linked). TASK-056's Bomb Tower AoE projectile links against
 *  ApplyRadialDamage exactly as the Sapper suicide does here.
 */
class GITCLAUDEUNREALTEST_API FSiegeCombatStatics
{
public:

	// ═══════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ THE ACQUISITION FUNNEL (TASK-828, law WITCH-§1)
	// ═══════════════════════════════════════════════════════════════════════════
	/**
	 *  ⭐⭐ THE ONE PLACE IN THIS PROJECT THAT MAY CALL
	 *  `UGameplayStatics::GetAllActorsWithInterface(UTeamAgent…)`. A grep of `Source/`
	 *  returning a SECOND hit is an automatic QA BLOCKER (WITCH-§1). The comment-skipping
	 *  form of that grep ships as an automation test — `Siegebound.Acquisition.
	 *  GetAllActorsWithInterfaceAppearsExactlyOnceInSource` — because this file's own prose
	 *  names the token it forbids, and a naive text scan would force the source to stop
	 *  explaining itself.
	 *
	 *  ⚖️ WHY THIS EXISTS, stated once so nobody re-flattens it back into nine loops.
	 *  Before TASK-828 there was NO shared chokepoint: nine sites each enumerated the world,
	 *  each re-did `Cast<ITeamAgent>`, and each re-did its own `GetTeamId() != Team` compare.
	 *  Invisibility (WITCH-§0) cannot use the ghost's mechanism — `ASiegeGhostPawn` is
	 *  untargetable because it does NOT implement `ITeamAgent`, which is compile-time,
	 *  class-level and permanent, whereas a veil is per-instance, runtime, reversible and
	 *  MUST PRESERVE ATTACKABILITY (an invisible unit that could not be attacked would make
	 *  the 3-second cast's own interruption rule unreachable). A veil is therefore a FLAG AT
	 *  A GUARD POINT — exactly the design GHOST-§1 refused in writing, because "a flag can be
	 *  forgotten at a guard point". ⇒ the mitigation is that there is now only ONE guard
	 *  point to forget. The same funnel carries FOG-§'s vision-range clamp; both cards ride
	 *  one piece of engineering, which is why they are one batch and not two.
	 *
	 *  ⛔ WHAT THIS FUNCTION IS, AND ONLY IS: enumerate → `IsValid` → `Cast<ITeamAgent>` →
	 *  keep the ones whose team differs from ViewerTeam. ⛔ It does NOT test distance,
	 *  liveness, class, profile, aggro or line of sight — every call site keeps its OWN
	 *  filtering exactly where it already lived (TASK-828 lifted the enumerate-and-team-filter
	 *  step and nothing else, so the refactor is behaviour-neutral by construction).
	 *
	 *  ⭐ ORDER IS PRESERVED. Out is filled in the enumeration's own order, so every caller's
	 *  strict-improvement tie-break (`Distance < Best`, `DistSq >= BestDistSq`) still keeps
	 *  the SAME winner it kept before the refactor. Rewriting this to sort, or to fill Out in
	 *  any other order, silently changes tie outcomes at all nine sites.
	 *
	 *  Out is Reset() first — callers pass a fresh local and never pre-seed it. A null World
	 *  yields an empty Out (the shipped `GetAllActorsWithInterface` null behaviour, kept).
	 *
	 *  ⭐⭐ LANDED BY TASK-829 (`WITCH-§2`): `FSiegeInvisibilityStatics::IsVisibleTo(...)` is
	 *     consulted HERE and ⛔ NOWHERE ELSE in the acquisition surface — reached through
	 *     `IsAgentVisibleTo` below, which is the ⛔ ONE veil consult in the project. ⇒ all eight
	 *     acquisition sites honour the veil for ⛔ free, and ⛔ none of them contains a veil check.
	 *     ⛔ Note the FRIENDLY answer is already correct — the predicate returns visible for every
	 *     same-team query — so `GatherFriendlyAgents` below needed no change and takes ⛔ no policy.
	 *  ⚠️⚠️ AND THE ⛔ MEASURED GAP TASK-829 IS ⛔ NOT ALLOWED TO CLOSE, SAID HERE SO IT IS ⛔ NOT
	 *     MISTAKEN FOR COVERAGE (`WITCH-§8`, measured by TASK-848): `ASiegeBotController` does ⛔ NOT
	 *     use this funnel. It enumerates enemy units with `TActorIterator` in ⛔ THREE scans
	 *     (`FindNearestEnemyIntruderOnBotHalf`'s unit arm + hero arm, `FindFireballClusterTarget`)
	 *     that ⛔ never touch a gather ⇒ ⛔ a veiled unit is invisible to every unit, tower and hero
	 *     and ⛔ STILL FULLY VISIBLE TO THE BOT'S BRAIN. ⭐ That is ⛔ TASK-851, and `IsAgentVisibleTo`
	 *     below is the seam it calls — ⛔ one function, ⛔ not a re-expressed rule.
	 *  ⭐⭐ LANDED BY TASK-838 (`FOG-§4(a)`, `FOG-§6`, `FOG-§7`): `FSiegeFogStatics::
	 *     EffectiveVisionRadius(...)` is applied HERE and ⛔ NOWHERE ELSE, exactly as the veil is.
	 *     It needs a RADIUS, which this signature did not take — so the SECOND optional parameter
	 *     below carries it, and it carries the ONE other thing only the site knows: where it is
	 *     looking FROM. ⛔ The two optional parameters are INDEPENDENT and neither may be folded
	 *     into the other: fog clamps by DISTANCE (per-query, team-blind), the veil by a
	 *     per-instance FLAG (per-actor, per-viewer). ⛔ Do NOT add a per-site clamp instead —
	 *     that is the failure this whole file exists to prevent, and it is an automatic FAIL
	 *     (`SiegeAcquisitionFunnelTest.cpp` test 9 pins it at ZERO in all five call-site files).
	 *  ⛔ THE ONE LANE THAT MUST SEE THROUGH THE VEIL: `ApplyRadialDamage` passes
	 *     `ESiegeVeilPolicy::IncludeVeiled` (WITCH-§2 / J-W2 — a blast is not an act of seeing,
	 *     and it is the card's counter). ⛔ That is the ONLY `IncludeVeiled` call site in shipping
	 *     code, and it is asserted as exactly one by the automation suite.
	 *  ⛔ THE LANES THAT MUST SEE THROUGH THE FOG pass ⛔ NO Vision at all — a blast, the two
	 *     directed spell gathers, the friendly buff and the cheat lane (`FOG-§7` rows 2/3/4/5).
	 *     ⚖️ 🧑 `J-F9`: `ASpellLineSweep::LineRange = 900.f` ALREADY EXCEEDS the 609.6 ceiling, so
	 *     a blind clamp here would cut the hero's line spell by 32.3% whenever fog is up — a live,
	 *     unrequested nerf to a shipped card, smuggled in under a fog card. Jonathan's words were
	 *     "ranged UNITS will not be able to fire beyond this range"; a HERO SPELL IS NOT A UNIT,
	 *     so his sentence does not answer it. ⭐ The proceeding default is NOT CLAMPED, and it is
	 *     reversible in ⛔ ONE line: the sweep's call site constructs a Vision query. Nothing else
	 *     changes, anywhere.
	 *
	 *  @param World       world to enumerate; null yields an empty Out
	 *  @param ViewerTeam  the ASKING actor's team — agents on this team are never returned
	 *  @param Out         reset, then filled with every valid hostile ITeamAgent actor
	 *  @param VeilPolicy  ⭐ defaults to SuppressVeiled — a veiled enemy is DROPPED. ⛔ Pass
	 *                     IncludeVeiled ONLY for a blast (see ESiegeVeilPolicy above)
	 *  @param Vision      ⭐ defaults to null — "this query is NOT an act of seeing", so no fog
	 *                     ceiling is applied. ⛔ Pass one ONLY from a `FOG-§7` row 1 vision /
	 *                     acquisition site (see FSiegeVisionQuery above)
	 */
	static void GatherHostileAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out,
		ESiegeVeilPolicy VeilPolicy = ESiegeVeilPolicy::SuppressVeiled,
		const FSiegeVisionQuery* Vision = nullptr);

	/**
	 *  ⭐⭐ THE ⛔ ONE VEIL CONSULT IN THE PROJECT (TASK-829, `WITCH-§1` + `WITCH-§2`). True when
	 *  ViewerTeam may SEE / ACQUIRE Candidate.
	 *
	 *  ⛔⛔ WHY THIS IS A FUNCTION AND ⛔ NOT AN INLINE `Candidate->IsInvisible()` INSIDE THE GATHER
	 *  LOOP — and it is the whole reason `TASK-851` can be a small task instead of a second
	 *  implementation of the rule: `ASiegeBotController`'s three `TActorIterator` scans are ⛔ OUTSIDE
	 *  the funnel (`WITCH-§8`). They cannot route through a gather, so the ⛔ only way they can honour
	 *  the SAME rule is to call the SAME function. ⭐ One rule, two kinds of caller.
	 *
	 *  ⭐⭐ THE SIGNATURE TAKES ⛔ ONE TEAM AND ⛔ ONE ACTOR, ON PURPOSE, AND IT CLOSES A ⛔ REAL
	 *  MEASURED HOLE (`qa/TASK-847.md` WARN-2). `FSiegeInvisibilityStatics::IsVisibleTo(ViewerTeam,
	 *  TargetTeam, bInvisible)` is ⛔ EXACTLY SYMMETRIC under exchange of its two `ETeamId`
	 *  arguments for ⛔ ALL EIGHT inputs ⇒ a ⛔ SWAPPED call site is ⛔ UNDETECTABLE BY ANY TEST
	 *  WRITABLE TODAY, and would become a ⛔ silent defect the day an asymmetric term arrives (a
	 *  see-through-veils detector, a per-team reveal). ⇒ ⭐ **THIS signature makes the swap
	 *  ⛔ UNTYPEABLE — the two arguments have ⛔ different types.** The one surviving call to the
	 *  symmetric predicate lives in this function's body and carries the named-argument comments
	 *  `TASK-829(3b)` requires.
	 *
	 *  ⛔ A NON-UNIT CANDIDATE IS ALWAYS VISIBLE, and that is a RULING, not a fallback: `WITCH-§6`
	 *  forbids a mirrored veil bool on any other class, and `J-W10` rules the ⛔ HERO not veilable.
	 *  ⇒ castles, buildings, towers and the hero ⛔ cannot be veiled, so `true` is the ⛔ complete
	 *  answer for them, ⛔ not a permissive default. ⚠️ If a second veilable class is ever added,
	 *  the fix is an interface method — ⛔ never a second `Cast` bolted on here.
	 *
	 *  Null / pending-kill Candidate ⇒ false (nothing invalid is visible to anyone).
	 *
	 *  @param ViewerTeam  the team DOING the looking
	 *  @param Candidate   the actor being looked at
	 */
	static bool IsAgentVisibleTo(ETeamId ViewerTeam, const AActor* Candidate);

	/**
	 *  The SAME-TEAM half of the funnel, and it is named rather than left as an unexplained
	 *  second enumeration (WITCH-§2, fourth lane).
	 *
	 *  ⛔ FRIENDLY ACQUISITION IS **NEVER** VEIL-SUPPRESSED: an invisible unit that its own
	 *  player cannot select, order, heal or buff is a BUG, not a feature. That is why this is
	 *  a separate entry point instead of a `bool bHostile` on the one above — the two lanes
	 *  have DIFFERENT futures (TASK-829 changes one of them and must not be able to change
	 *  the other by editing a shared branch).
	 *
	 *  It exists because the audit that produced the "nine sites" figure found a TENTH
	 *  consumer hiding behind one of them: `SpellLibrary.cpp`'s single enumeration fed THREE
	 *  filter loops, and one of those three — `ResolveAllyBuff` (Battle Cry) — wants
	 *  FRIENDLIES. Routing it through GatherHostileAgents would have silently made Battle Cry
	 *  buff nobody. Same enumeration, same order guarantee, inverted team term.
	 *
	 *  @param World       world to enumerate; null yields an empty Out
	 *  @param ViewerTeam  the ASKING actor's team — only agents on THIS team are returned
	 *  @param Out         reset, then filled with every valid same-team ITeamAgent actor
	 */
	static void GatherFriendlyAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out);

	/**
	 *  The team relation the funnel filters on, spelled as a pure function so it can be
	 *  asserted exhaustively without a world (the SiegeLadderClimbStatics precedent).
	 *  True when CandidateTeam is an ENEMY of ViewerTeam.
	 *
	 *  ⚠️ Deliberately NOT `!=` inlined at the two gather bodies: a test that only asserted
	 *  "the gatherer returns something" would pass just as happily with the team term deleted.
	 *  This is the term such a test can pin, and `IsHostileTeam(Blue, Blue) == false` is the
	 *  control that goes red the moment the friendly-fire filter is dropped.
	 */
	static bool IsHostileTeam(ETeamId ViewerTeam, ETeamId CandidateTeam);

	/**
	 *  Applies Damage (of DamageTypeClass) to EVERY actor implementing ITeamAgent whose
	 *  team differs from Team and whose collision lies within Radius of Center — i.e. all
	 *  ENEMY combat actors caught in the blast, never a friendly (GDD §3.0 no friendly
	 *  fire). The same-team exclusion is enforced HERE (by the Team filter), not left to
	 *  each receiver — so a caller with no resolvable instigator chain (a tower-fired
	 *  projectile) still cannot friendly-fire.
	 *
	 *  Distance is measured to the CLOSEST POINT on each candidate's collision (the
	 *  ASummonedUnit::GetDistanceToTarget contract), so large-footprint fortifications —
	 *  the castle's ~800×800 base, buildings — are hit when the blast center sits at their
	 *  wall, not only when their ORIGIN happens to fall inside Radius. A candidate with no
	 *  ECC_Pawn-blocking collision falls back to its actor origin.
	 *
	 *  Each hit is routed through the target's own TakeDamage, so per-fortification scaling
	 *  still applies (a Siege-typed blast is 200% vs castle/buildings, TASK-054; units/hero
	 *  take the listed amount). DamageCauser is null on purpose — the Team parameter, not the
	 *  instigator chain, is the friendly-fire authority; InstigatorController is passed through
	 *  for attribution when the caller has one (units do via GetController(); towers may not).
	 *  AGoldNode deliberately does not implement ITeamAgent, so mining nodes are never caught.
	 *
	 *  No-op on a null World, Radius <= 0, or Damage <= 0. A null DamageTypeClass defaults to
	 *  base UDamageType (100% everywhere).
	 *
	 *  ⭐ TASK-828: the candidate universe now comes from GatherHostileAgents above — the
	 *  enemies-only guarantee is the SAME team filter it always was, just authored once
	 *  instead of nine times.
	 *
	 *  ⛔⛔ TASK-829 — THE VEIL EXEMPTION, SHIPPED ⛔ EXPLICITLY AND ⛔ COMMENTED SO A FUTURE
	 *  READER DOES NOT "FIX" IT. This lane passes `ESiegeVeilPolicy::IncludeVeiled`, so an
	 *  INVISIBLE unit ⛔ IS caught by a blast. ⚖️ `WITCH-§2` / `J-W2`: ***a blast is not an act
	 *  of seeing***, and it is the 50-gold card's ⛔ ONLY counter — a Fireball, a Sapper or a
	 *  Bomb Tower must be able to flush a veiled push, or the card is an auto-win. ⚠️ Deleting
	 *  that argument would make every AoE in the game silently miss veiled units, and ⛔ no
	 *  friendly-fire or damage test would go red.
	 *
	 *  @param World                world to search (via GatherHostileAgents)
	 *  @param InstigatorController attacker's controller for damage attribution, or null
	 *  @param Team                 the ATTACKER's team; actors on this team are never damaged
	 *  @param Center               blast origin in world space
	 *  @param Radius               blast radius in units (closest-point)
	 *  @param Damage               damage dealt to each enemy BEFORE receiver-side scaling
	 *  @param DamageTypeClass      damage type carried to each ApplyDamage (Siege / Projectile / ...)
	 */
	static void ApplyRadialDamage(
		UWorld* World,
		AController* InstigatorController,
		ETeamId Team,
		const FVector& Center,
		float Radius,
		float Damage,
		TSubclassOf<UDamageType> DamageTypeClass);

private:

	/**
	 *  ⛔ THE SOLE `GetAllActorsWithInterface(UTeamAgent…)` IN THE PROJECT, and it is PRIVATE
	 *  so it cannot become a tenth back door. The two public gathers are its only callers and
	 *  differ by one bool; anything that wants team agents goes through one of them and is
	 *  therefore automatically subject to whatever WITCH-§/FOG-§ later add there.
	 *
	 *  @param bWantHostile  true keeps enemies of ViewerTeam; false keeps ViewerTeam's own
	 */
	static void GatherTeamAgentsFiltered(const UWorld* World, ETeamId ViewerTeam, bool bWantHostile, TArray<AActor*>& Out);

	/**
	 *  ⭐⭐ THE ⛔ ONE FOG-STATE READ IN THE ENTIRE ACQUISITION SURFACE (TASK-838, `FOG-§6`'s M8
	 *  clause: *"the ceiling is derived from ONE replicated 'fog is active until T' scalar, ⛔ never
	 *  from per-actor visibility"*). PRIVATE, so it cannot become a second door.
	 *
	 *  ⛔⛔ THIS IS A SEAM, ⛔ NOT A STUB, AND THE DIFFERENCE IS WHO IS SUPPOSED TO FILL IT.
	 *  `AFogVolume` — the authoritative "fog is active until T" actor — is ⛔ TASK-839's, and
	 *  TASK-839 is BLOCKED BY THIS TASK. So the wiring lands first and the source lands second,
	 *  which is the same order `TASK-837` shipped in: ⛔ a landed file is not a landed feature.
	 *  ⇒ ⚠️ TODAY THIS RETURNS FALSE, so the ceiling never fires and the acquisition surface is
	 *  BYTE-FOR-BYTE the game that shipped. ⛔ Say that out loud rather than let a green suite
	 *  imply otherwise.
	 *
	 *  ⭐ WHAT TASK-839 DOES WITH IT: replaces the body's final `return false` with the
	 *  `AFogVolume` read (its ONE scalar + its tuning) and changes ⛔ nothing else — ⛔ not this
	 *  signature, ⛔ not its single call site, ⛔ not one line in any of the five vision sites.
	 *  ⛔ It does NOT add a second read somewhere else; a second read is a second guard point,
	 *  which is the failure `WITCH-§1` and this whole file exist to prevent.
	 *
	 *  @param World      world whose fog state is being asked about; null is never under fog
	 *  @param OutTuning  ⭐ ALWAYS assigned, on every path, so no caller can read an
	 *                    uninitialised band even when the answer is "no fog"
	 *  @return           true when fog is active in World right now
	 */
	static bool ReadFogState(const UWorld* World, FSiegeFogTuning& OutTuning);
};
