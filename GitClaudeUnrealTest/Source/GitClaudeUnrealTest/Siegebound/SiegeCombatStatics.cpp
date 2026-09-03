// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeCombatStatics.h"

#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/SiegeInvisibilityStatics.h" // TASK-829 (WITCH-§2): FSiegeInvisibilityStatics::IsVisibleTo — the ONE suppression predicate, called from IsAgentVisibleTo below and NOWHERE else
// ⚠️ The trailing note below deliberately does NOT spell the veil flag's name: a comment TRAILING
// a line of CODE is still scanned by the house source-text probes, and the one-source-of-truth gate
// (Siegebound.Invisibility.TheVeilHasOneSourceOfTruthAndOneWriteDoor) would read this file as a
// second copy of it. See the veil flag's declaration in SummonedUnit.h.
#include "Siegebound/SiegeFogStatics.h"          // TASK-838 (FOG-§6): the fog rule — consumed HERE and in no other translation unit
#include "Siegebound/SummonedUnit.h"             // TASK-829 (WITCH-§6): ASummonedUnit is the ONLY veilable class — complete type required for the Cast in IsAgentVisibleTo
#include "Siegebound/TeamId.h"

// ═════════════════════════════════════════════════════════════════════════════════
//  THE ACQUISITION FUNNEL (TASK-828, law WITCH-§1) — the rationale lives in the
//  header beside the declarations; this is the mechanism only.
// ═════════════════════════════════════════════════════════════════════════════════

bool FSiegeCombatStatics::IsHostileTeam(ETeamId ViewerTeam, ETeamId CandidateTeam)
{
	// The whole friendly-fire authority of this project, in one expression (GDD §3.0).
	// ⛔ Do not "simplify" this away by inlining `!=` at the two call sites: it is the term
	// the exhaustive 2x2 automation test pins, and an inlined operator cannot be asserted.
	return CandidateTeam != ViewerTeam;
}

void FSiegeCombatStatics::GatherTeamAgentsFiltered(const UWorld* World, ETeamId ViewerTeam, bool bWantHostile, TArray<AActor*>& Out)
{
	// callers pass a fresh local; Reset rather than Empty keeps the allocation on the
	// per-tick acquisition paths (ASummonedUnit polls this every 0.25 s per unit).
	Out.Reset();

	if (!World)
	{
		return;
	}

	// ⛔⛔ THE ONE ENUMERATION. Every combat actor implements ITeamAgent (hero, units,
	// castles, buildings); AGoldNode deliberately does not, so mining nodes are never
	// returned to anyone (TASK-025 contract), and ASiegeGhostPawn deliberately does not,
	// which is the ENTIRETY of its "cannot be attacked" guarantee (GHOST-§1).
	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	Out.Reserve(TeamAgents.Num());

	// ⭐ IN ENUMERATION ORDER — never sorted. Every caller's tie-break is a strict
	// improvement test, so the FIRST candidate wins a tie; re-ordering here would silently
	// change which target nine acquisition sites pick.
	for (AActor* Candidate : TeamAgents)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		// native cast is valid: UTeamAgent is NotBlueprintable (TASK-001 ruling)
		const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
		if (!Agent)
		{
			continue;
		}

		if (IsHostileTeam(ViewerTeam, Agent->GetTeamId()) != bWantHostile)
		{
			continue;
		}

		Out.Add(Candidate);
	}
}

bool FSiegeCombatStatics::IsAgentVisibleTo(ETeamId ViewerTeam, const AActor* Candidate)
{
	// Nothing invalid is visible to anyone. Placed first so the cast below never runs on a
	// pending-kill actor; the gathers already dropped these, but TASK-851's TActorIterator
	// scans will call this directly and must get the same answer.
	if (!IsValid(Candidate))
	{
		return false;
	}

	// ⛔⛔ ONE CAST, AND IT IS THE COMPLETE ANSWER — ⛔ not a permissive fallback.
	// WITCH-§6: `bIsInvisible` is ONE source of truth and there is ⛔ no mirrored bool on any
	// other class. J-W10 rules the HERO ⛔ not veilable, and WITCH-§7 records towers as ⛔ not
	// veilable today. ⇒ a castle, a building, a tower or the hero ⛔ CANNOT be veiled, so
	// "visible" is the whole truth for them. The witch, the miner and the sorcerer all derive
	// from ASummonedUnit, so every veilable actor in the game is caught by this one cast.
	// ⚠️ If a SECOND veilable class is ever added, the fix is a method on ITeamAgent — ⛔ never a
	// second Cast bolted on here, because the second one is the one somebody forgets.
	const ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate);
	if (!Unit)
	{
		return true;
	}

	// ⭐⭐ THE ⛔ ONE CALL TO THE SYMMETRIC PREDICATE IN THE WHOLE PROJECT, and the named-argument
	// comments are ⛔ NOT style (TASK-829(3b), qa/TASK-847.md WARN-2): `IsVisibleTo`'s body is
	// `(Viewer == Target) || !bTargetIsInvisible`, which is ⛔ EXACTLY SYMMETRIC under exchange of
	// its two ETeamId arguments for ⛔ all eight inputs ⇒ a SWAPPED call is ⛔ undetectable by any
	// test writable today, and becomes a ⛔ silent defect the day an asymmetric term arrives.
	// ⭐ THIS function's own signature makes the swap ⛔ untypeable for every CALLER (one team, one
	// actor); the comments guard the one place the symmetric form still has to be written.
	//
	// ⚠️⚠️ THE CALL IS ON ⛔ ONE LINE ON PURPOSE, AND IT IS ⛔ NOT A FORMATTING PREFERENCE — this
	// is a measured property of the house source-text probes, found while writing this task's gate.
	// `CountOccurrencesInCode` SKIPS any line whose trimmed form STARTS WITH `/*`, so the natural
	// one-argument-per-line layout would put each `/*ViewerTeam=*/` at the head of its own line and
	// make the guard ⛔ INVISIBLE TO THE VERY TEST THAT ENFORCES IT — an assertion that reads zero
	// and a "fix" that bumps the expectation to zero. ⇒ ⛔ do not re-wrap this call.
	return FSiegeInvisibilityStatics::IsVisibleTo(/*ViewerTeam=*/ ViewerTeam, /*TargetTeam=*/ Unit->GetTeamId(), /*bTargetIsInvisible=*/ Unit->IsInvisible());
}

bool FSiegeCombatStatics::ReadFogState(const UWorld* World, FSiegeFogTuning& OutTuning)
{
	// Assigned on EVERY path, before any early-out, so "no fog" never hands back an
	// uninitialised band. A default-constructed tuning IS the shipped tuning (609.6 / 304.8 /
	// exponent 2), which is also what AFogVolume's instance will default to.
	OutTuning = FSiegeFogTuning();

	// A gather with no world can never be under fog. Kept live rather than folded into the
	// return below so the shape TASK-839 inherits is already the correct one.
	if (!World)
	{
		return false;
	}

	// ⛔⛔ THIS LINE IS THE SEAM, AND IT IS THE ONLY LINE TASK-839 REPLACES.
	// `AFogVolume` — the authoritative "fog is active until T" scalar (FOG-§6) — does not exist
	// in the tree yet: it is TASK-839's, and TASK-839 is blocked by THIS task. ⇒ the clamp is
	// wired now, at the one place FOG-§6 says it must live, and the state arrives next.
	// ⚠️⚠️ CONSEQUENCE, STATED SO A GREEN SUITE DOES NOT IMPLY MORE THAN IT PROVES: while this
	// returns false, EffectiveVisionRadius returns every request bit-identically, the vision cut
	// below can never shorten anything, and the acquisition surface is byte-for-byte the game
	// that shipped. ⛔ Fog does not exist at runtime until TASK-839 lands.
	// ⭐ TASK-839: iterate AFogVolume, take its active flag and its FSiegeFogTuning, return here.
	// ⛔ Do NOT add a second read anywhere else — a second read is a second guard point, which is
	// the exact failure WITCH-§1 and this whole file exist to prevent.
	return false;
}

void FSiegeCombatStatics::GatherHostileAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out,
	ESiegeVeilPolicy VeilPolicy, const FSiegeVisionQuery* Vision)
{
	GatherTeamAgentsFiltered(World, ViewerTeam, /*bWantHostile=*/ true, Out);

	// ═══════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ THE FOG VISION CEILING — ⛔ ONE clamp, for the FIVE vision sites (TASK-838; law
	//  FOG-§4(a) / FOG-§6 / FOG-§7). "The fog makes it to where players can really only see up
	//  until about 20 feet in front of you. And that includes AI and all units."
	// ═══════════════════════════════════════════════════════════════════════════════════════
	//
	// ⛔⛔ THE `if (Vision)` IS THE EXEMPTION, AND IT IS THE *ONLY* THING THAT DECIDES IT.
	// It does NOT inspect the caller, it does not read a call-site enum, and there is no list of
	// exempt sites anywhere in this file (FOG-§7's structural law; TASK-838(1b) makes a
	// `switch (CallerKind)` here an automatic QA FAIL). A lane is clamped exactly when it hands
	// over an act of seeing. The blast (WITCH-§2), the two directed spell gathers, the friendly
	// buff and the cheat lane hand over nothing, so they cannot be clamped — not "are not
	// clamped today", CANNOT BE, because there is no radius here to clamp them with.
	//
	// ⚖️ 🧑 J-F9 — THE ONE THING THIS `if` DECIDES THAT IS JONATHAN'S TO OVERRULE, NOT MINE.
	// ASpellLineSweep::LineRange = 900.f ALREADY EXCEEDS the 609.6 ceiling, so a blind clamp here
	// would cut the hero's line spell by 32.3% whenever fog is up — live code, measured, not
	// hypothetical. His sentence says "ranged UNITS will not be able to fire beyond this range",
	// and a HERO SPELL IS NOT A UNIT, so it does not answer this. ⇒ the proceeding default is
	// NOT CLAMPED, following this batch's own sibling ruling (WITCH-§2: "a blast is not an act of
	// seeing" — the hero AIMS a line sweep, he does not acquire along it).
	// ⚠️⚠️ THE RESIDUAL, UNSOFTENED: under fog the hero can hit something at 900 uu that he
	// literally cannot see. ⭐ Flipping it is ONE line at SpellLineSweep.cpp's gather — construct
	// a Vision query there. Nothing in this file changes either way.
	// ⛔⛔ AND J-F9 COVERS A CLASS, NOT ONE SPELL — MEASURED BY TASK-838 AND CORRECTING THE LAW IT
	// WAS BOARDED FROM: `Lightning` ships `AoERadius = 700` (its reticle radius in
	// ResolveTopTargetsDamage), which ALSO already exceeds the ceiling. FOG-§7's claim that "every
	// AoE radius in the game is under the ceiling" is FALSE — its six-card enumeration missed this
	// one. ⇒ a blind clamp would have been TWO live nerfs, not one: the line sweep −32.3% and
	// Lightning's target selection −12.9%. Both are row 3 (the caster AIMS), both hand over
	// nothing, both are exempt for the same structural reason.
	if (Vision)
	{
		// ⛔ ONE fog-state read per gather, from the ONE seam (FOG-§6's "one replicated scalar").
		FSiegeFogTuning FogTuning;
		const bool bFogActive = ReadFogState(World, FogTuning);

		// ⛔⛔ CALLED UNCONDITIONALLY. ⛔ NOT wrapped in a hand-written `if (bFogActive)` — that
		// would re-introduce the branch TASK-837 built this function to delete. The seam returns
		// RequestedRadiusUU BIT-IDENTICALLY when fog is off (no clamp, no sanitising, not one ulp
		// of drift), which is precisely the property that keeps any fog regression ATTRIBUTABLE
		// TO FOG. It is also TOTAL: a NaN, a negative or a ZERO ceiling (FOG-§7b) all return the
		// request unchanged, i.e. they fail toward NO FOG and never toward no vision.
		const float EffectiveRadiusUU = FSiegeFogStatics::EffectiveVisionRadius(
			Vision->RequestedRadiusUU, bFogActive, FogTuning);

		// ⭐⭐ THE NO-OP PROOF, AND IT IS A STRICTLY STRONGER STATEMENT THAN `if (bFogActive)`.
		// The cut runs only when the fog ACTUALLY SHORTENED the request — so every path on which
		// the ceiling did nothing (fog off, a ceiling above this site's own reach, and every
		// degenerate tuning in FOG-§7a's totality list) skips the loop entirely, costs zero
		// distance queries, and provably cannot change one element of Out. ⛔ It is a test of this
		// function's OUTPUT, never of the fog's state, which is why it holds for the broken-tuning
		// cases a state check would sail straight past. A NaN request compares false and is
		// therefore also a no-op, which is the correct direction.
		// ⛔ Do not "simplify" this to `<=`: at equality there is nothing to remove.
		if (EffectiveRadiusUU < Vision->RequestedRadiusUU)
		{
			const FVector ViewOrigin = Vision->ViewOrigin;
			const float RequestedRadiusUU = Vision->RequestedRadiusUU;

			// ⭐ RemoveAll is ORDER-PRESERVING — the same load-bearing property the veil filter
			// below relies on. Every call site breaks ties by STRICT improvement, so the FIRST
			// surviving candidate wins; a filter that reordered survivors would silently re-pick
			// targets at five sites without changing a single result SET.
			Out.RemoveAll([&ViewOrigin, RequestedRadiusUU, bFogActive, &FogTuning](const AActor* Candidate)
			{
				// ⭐⭐ CLOSEST POINT ON THE CANDIDATE'S COLLISION, and the metric is a CORRECTNESS
				// requirement rather than a preference. It is exactly what the two sites that own a
				// reach already measure with — ASummonedUnit::GetDistanceToTarget and the hero's
				// melee sweep are both ActorGetDistanceToCollision(…, ECC_Pawn, …) with the same
				// origin fallback — so with fog off (where this loop cannot run anyway) it would
				// agree with them term for term. ⛔ It is also never STRICTER than any site's own
				// metric: closest-point <= origin-to-origin, so this cut can only ever remove what
				// the fog removed and can never drop a candidate a site would have kept. A tower
				// measuring origin-to-origin therefore keeps its own gate untouched.
				// ⚖️ And it is the honest answer to "can this thing SEE it": you see a castle whose
				// WALL is five metres away, not one whose pivot happens to sit forty metres back.
				FVector ClosestPoint = FVector::ZeroVector;
				float Distance = Candidate->ActorGetDistanceToCollision(ViewOrigin, ECC_Pawn, ClosestPoint);
				if (Distance < 0.f)
				{
					// no ECC_Pawn-blocking collision: fall back to the actor origin (the shipped
					// ApplyRadialDamage / GetDistanceToTarget fallback, kept identical)
					Distance = static_cast<float>(FVector::Dist(ViewOrigin, Candidate->GetActorLocation()));
				}

				// ⛔ THE COMPARISON'S SHAPE IS NOT RE-AUTHORED HERE. IsVisibleThroughFog owns the
				// INCLUSIVE `<=` — measured from the project's own acquisition idiom, not chosen —
				// so the clamp substitutes the OPERAND of the shipped comparison and never its
				// shape (FOG-§7a). Writing `Distance > EffectiveRadiusUU` here instead would put a
				// second, silently divergent copy of that boundary rule in the codebase.
				return !FSiegeFogStatics::IsVisibleThroughFog(Distance, RequestedRadiusUU, bFogActive, FogTuning);
			});
		}
	}

	// ⛔ THE BLAST LANE STOPS HERE. WITCH-§2 / J-W2: a blast is not an act of seeing, so
	// ApplyRadialDamage asks for IncludeVeiled and a veiled unit IS caught by it.
	// ⭐ TASK-838 left this block byte-identical and put the fog cut ABOVE it deliberately. The
	// two filters are independent RemoveAll passes, so their order cannot change the result, and
	// keeping this one last means the blast lane's early-out still reads as the last word on the
	// veil. ⛔ The blast is exempt from the FOG cut for a completely different reason — it hands
	// over no Vision — and the two exemptions must stay separately expressed: one card must never
	// be able to change the other by editing a shared branch.
	if (VeilPolicy != ESiegeVeilPolicy::SuppressVeiled)
	{
		return;
	}

	// ⭐⭐ THE VEIL CONSULT — ⛔ ONE guard point for EIGHT acquisition sites (WITCH-§1). This is
	// the whole mitigation: WITCH-§0 recorded that invisibility must be "a flag consulted at a
	// guard point", which is exactly the design GHOST-§1 refused because a flag can be FORGOTTEN
	// at a guard point. ⇒ the answer was to leave only ONE guard point to forget, and this is it.
	//
	// ⭐ RemoveAll is ⛔ ORDER-PRESERVING, and that is load-bearing rather than incidental: every
	// call site breaks ties by STRICT improvement (`Distance < Best`, `DistSq >= BestDistSq`), so
	// the FIRST surviving candidate wins a tie. A filter that reordered survivors would silently
	// re-pick targets at eight sites without changing a single result SET — the exact regression
	// class the funnel's no-sort guarantee exists for. ⛔ Do not replace this with a sort, a
	// partition, a reverse pass or an index-descending erase loop.
	//
	// ⛔ WHY THE FILTER IS HERE AND ⛔ NOT INSIDE GatherTeamAgentsFiltered: that helper is the raw
	// enumerate-and-team-filter step and its behaviour-neutrality is asserted term by term by the
	// TASK-828 suite. The veil is a FEATURE, not part of the enumeration, and the FRIENDLY lane
	// must never acquire it (WITCH-§2 lane 4) — keeping the two apart means TASK-829 cannot change
	// the friendly lane by editing a shared branch, which is why GatherFriendlyAgents was made a
	// separate entry point in the first place.
	Out.RemoveAll([ViewerTeam](const AActor* Candidate)
	{
		return !IsAgentVisibleTo(ViewerTeam, Candidate);
	});
}

void FSiegeCombatStatics::GatherFriendlyAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out)
{
	// ⛔⛔ NEVER veil-suppressed (WITCH-§2, fourth lane) — see the header. There is deliberately
	// ⛔ NO ESiegeVeilPolicy parameter here: an invisible unit its own player cannot select, order,
	// heal or buff is a ⛔ BUG, not a feature, and a lane with no policy ⛔ cannot be given one by
	// accident. ⭐ BELT AND BRACES: even if this DID suppress, FSiegeInvisibilityStatics::IsVisibleTo
	// checks `ViewerTeam == TargetTeam` FIRST and unconditionally, so a same-team query answers
	// "visible" for a veiled unit anyway. Two independent reasons, ⛔ neither relied upon alone.
	GatherTeamAgentsFiltered(World, ViewerTeam, /*bWantHostile=*/ false, Out);
}

void FSiegeCombatStatics::ApplyRadialDamage(
	UWorld* World,
	AController* InstigatorController,
	ETeamId Team,
	const FVector& Center,
	float Radius,
	float Damage,
	TSubclassOf<UDamageType> DamageTypeClass)
{
	// nothing to do without a world or a positive blast (the SetTimer-style non-positive guard)
	if (!World || Radius <= 0.f || Damage <= 0.f)
	{
		return;
	}

	// a null type reads as 100% everywhere (base UDamageType); callers normally pass Siege/Projectile
	if (!DamageTypeClass)
	{
		DamageTypeClass = UDamageType::StaticClass();
	}

	// ⭐ SITE 5 of 9 (TASK-828) — and the one a naive audit misses, because EVERY AoE in the
	// game routes through it: Sapper suicide, Bomb Tower, Fireball, every blast projectile.
	//
	// ⛔ FRIENDLY FIRE IS STILL IMPOSSIBLE, AND THE AUTHORITY DID NOT MOVE. It was, and still
	// is, the TEAM FILTER ON THE CANDIDATE SET — never the receiver's instigator chain, so a
	// tower-fired projectile with no resolvable instigator still cannot hit its own side.
	// GatherHostileAgents(World, Team, …) applies exactly the `IsValid` + `Cast<ITeamAgent>` +
	// `GetTeamId() != Team` triple that used to be written out below, in enumeration order,
	// so the set this loop walks is element-for-element what it was before the refactor.
	// AGoldNode still opts out by not implementing ITeamAgent, so mining nodes are never
	// damaged by a blast (TASK-025 contract).
	//
	// ⛔⛔⛔ TASK-829 — THE VEIL EXEMPTION, AND IT IS THE ⛔ ONE `IncludeVeiled` IN SHIPPING CODE.
	// ⚖️ WITCH-§2 / ruling J-W2: ***A BLAST IS NOT AN ACT OF SEEING.*** An INVISIBLE unit ⛔ IS
	// caught by this blast, and that is ⛔ not an oversight to tidy up — it is the ⛔ ONLY counter
	// the 50-gold card has. A Fireball, a Sapper's suicide or a Bomb Tower must still flush a
	// veiled push, or a veiled army is an auto-win.
	// ⚠️⚠️ IF YOU ARE HERE TO "FIX" THIS: deleting the argument makes EVERY AoE in the game
	// silently miss veiled units, and ⛔ not one friendly-fire, damage or radius test would go red.
	// The suite asserts this argument by name (Siegebound.Invisibility.BlastLaneIsExemptFromTheVeil).
	// ⭐ THE SYMMETRY IT BUYS, so the design is visible: the bot cannot AIM a Fireball at a veiled
	// cluster (TASK-851 suppresses the aiming scan) but a Fireball it aimed at something else
	// still catches them — the same asymmetry the player gets, from the same two laws.
	TArray<AActor*> HostileAgents;
	GatherHostileAgents(World, Team, HostileAgents, ESiegeVeilPolicy::IncludeVeiled);

	for (AActor* Candidate : HostileAgents)
	{
		// closest-point distance so a blast at a fortification's wall still reaches it — the
		// castle/building ORIGIN can be hundreds of units past the wall. Mirrors ASummonedUnit::
		// GetDistanceToTarget; folds into that consolidation on the wave that owns hero/projectile/
		// unit together (qa/TASK-026 NIT-4) — a documented single call site, not a new mirror.
		FVector ClosestPoint = FVector::ZeroVector;
		float Distance = Candidate->ActorGetDistanceToCollision(Center, ECC_Pawn, ClosestPoint);
		if (Distance < 0.f)
		{
			// no ECC_Pawn-blocking collision: fall back to the actor origin
			Distance = static_cast<float>(FVector::Dist(Center, Candidate->GetActorLocation()));
		}
		if (Distance > Radius)
		{
			continue;
		}

		// route through the target's TakeDamage so per-fortification scaling still applies
		// (Siege → 200% vs castle/buildings, TASK-054). DamageCauser null (see the header).
		UGameplayStatics::ApplyDamage(Candidate, Damage, InstigatorController, nullptr, DamageTypeClass);
	}
}
