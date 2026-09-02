// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/ClimbableTower.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GitClaudeUnrealTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "Siegebound/SiegeNavAreas.h"
#include "Siegebound/SummonedUnit.h"

// ⛔⛔ THE INCLUDE LIST IS STILL PART OF THE CONTRACT, AND EXACTLY ONE LINE OF IT
// CHANGED ON 2026-09-01 — SO IT IS CALLED OUT RATHER THAN SLIPPED IN.
//
// "Siegebound/SummonedUnit.h" IS NOW HERE, AND IT IS HERE FOR **MOVEMENT ONLY**
// (TOWER-§8.4(B) — the one new coupling surface this redesign declares, and the
// ONLY one). A ladder has to drive a SPECIFIC unit along a SPECIFIC line, and
// there is no way to move a unit without touching it.
//
// ⭐ WHAT DID **NOT** CHANGE, WHICH IS THE HALF THAT ACTUALLY MATTERED (HIGH-§3):
// this class still never calls HeightAdvantageMultiplier, never reports that a
// unit is elevated, never reads a unit's damage state, and includes no HIGH-§
// header. A unit on a HILL and a unit on a TOWER at the same Z deal IDENTICAL
// damage, and the damage rule still does not know towers exist.
//
// ⛔ And still NO "Siegebound/Tower.h": this class shares no code with the
// auto-firing tower family and has no fire path to arm.

// ═══════════════════════════════════════════════════════════════════════════════
//  THE PINNED CONSTANTS (TOWER-§8.3 / TOWER-§8.4(A))
// ═══════════════════════════════════════════════════════════════════════════════

// The artist↔programmer seam, spelled once (CONVENTIONS "Static-mesh SOCKET
// names"). TASK-737 authors exactly these two names on /Game/Meshes/SM_WatchTower.
const FName AClimbableTower::LadderFootSocketName(TEXT("LadderFoot"));
const FName AClimbableTower::LadderTopSocketName(TEXT("LadderTop"));

// ⛔ THE SOCKET-ABSENT FALLBACK, AND ⛔ NEVER THE PRIMARY SOURCE. Both numbers are
// navmesh arithmetic, derived in full on the header's declaration:
//   • foot X −450 clears the body's eroded nav carve (X ≤ −364) by 86 uu;
//   • top X −150 sits 86 uu inside the deck's surviving poly (X ∈ [−236, +236]).
// Together they define ONE straight segment: length 1,236.9 uu at 76.0°.
const FVector AClimbableTower::LadderFootDefaultRelative(-450.f, 0.f, 0.f);
const FVector AClimbableTower::LadderTopDefaultRelative(-150.f, 0.f, 1200.f);

namespace SiegeClimbableTowerPrivate
{
	/**
	 *  Below this, a "ladder" is not a traversal and an off-mesh connection is
	 *  nonsense. ⛔ DERIVED, ⛔ not felt: the nav agent's radius is 34 uu
	 *  (TOWER-§2a), so a link shorter than one agent DIAMETER connects two points
	 *  the same body already occupies. The real line is 1,236.9 uu — 18× this — so
	 *  this can only ever fire on a mis-authored mesh, which is exactly when the
	 *  degrade-open fallback has to be there.
	 */
	constexpr float MinimumLadderLineUU = 2.f * 34.f;

	/** The pawn behind a UPathFollowingComponent — ANavLinkProxy::NotifySmartLinkReached's resolution, verbatim (NavLinkProxy.cpp:326-341): the component may hang off the pawn OR off its controller. */
	AActor* ResolvePathFollowingPawn(UObject* PathComp)
	{
		const UPathFollowingComponent* const Follower = Cast<UPathFollowingComponent>(PathComp);
		AActor* Owner = Follower ? Follower->GetOwner() : nullptr;
		if (const AController* const AsController = Cast<AController>(Owner))
		{
			Owner = AsController->GetPawn();
		}
		return Owner;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  UClimbableTowerLadderLink — the smart link, plus TOWER-§8.6's second layer
// ═══════════════════════════════════════════════════════════════════════════════

UClimbableTowerLadderLink::UClimbableTowerLadderLink(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The pinned TOWER-§8.3 line as the CONSTRUCTOR default, so a tower is a
	// correct off-mesh connection from the instant it exists — before BeginPlay,
	// before a mesh is assigned, and in the editor. BeginPlay then replaces these
	// with the mesh's own sockets when SM_WatchTower supplies them.
	//
	// ⭐ Written straight into the inherited protected fields rather than through
	// SetLinkData: SetLinkData refreshes navigation modifiers, and a constructor —
	// which also runs on the class default object — is the wrong place to touch the
	// navigation octree. The base's own constructor seeds these same fields the
	// same way (NavLinkCustomComponent.cpp:40-42).
	LinkRelativeStart = AClimbableTower::LadderFootDefaultRelative;
	LinkRelativeEnd = AClimbableTower::LadderTopDefaultRelative;

	// ⛔⛔ REQUIRED, ⛔ NOT PREFERRED (TOWER-§8.7). Stated explicitly even though it
	// is also the engine default: a one-way ladder leaves the deck a dead end with
	// no legal path off it, and "we happened to inherit the right value" is not a
	// decision anyone can audit. The test asserts this exact value.
	LinkDirection = ENavLinkDirection::BothWays;
}

bool UClimbableTowerLadderLink::IsLinkPathfindingAllowed(const UObject* Querier) const
{
	// The whole rule lives on the tower as a pure static — see the class note for
	// the four cited sites that establish what `Querier` is, and why every
	// unresolved case returns true.
	return AClimbableTower::ShouldLinkAllowPathfinding(Querier, GateTeam);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  AClimbableTower
// ═══════════════════════════════════════════════════════════════════════════════

AClimbableTower::AClimbableTower()
{
	// ⛔ NOTHING is added to the base's behaviour here beyond the ladder link. No
	// tick (ABuilding sets bCanEverTick false and this class keeps it), no timer,
	// no target acquisition, no projectile — TOWER-§3's row ships
	// Damage/Range/Cadence = 0/0/0 and this class has no fire path to arm even if
	// it did not.
	//
	// ⭐⭐ AND THE NO-TICK PROPERTY SURVIVES THE LADDER, WHICH WAS NOT FREE AND IS
	// THE REASON THE API IN TOWER-§8.4(B) HAS A DELEGATE IN IT: a traversal has a
	// beginning and an end, and the obvious way to notice the end is to poll for
	// it. Both edges are PUSHED here instead — entry through the link's
	// FOnMoveReachedLink, completion through ASummonedUnit::OnLadderClimbEnded — so
	// there is nothing left to poll.

	// ⚠️ NO SetupAttachment AND NO RELATIVE TRANSFORM: UNavLinkCustomComponent is a
	// UActorComponent (via UNavRelevantComponent), ⛔ not a USceneComponent. Its two
	// endpoints are OWNER-relative and the owner's transform places them
	// (NavLinkCustomComponent.cpp:518-526). Attaching it is not "tidier" — it does
	// not compile.
	LadderLink = CreateDefaultSubobject<UClimbableTowerLadderLink>(TEXT("LadderLink"));
}

void AClimbableTower::BeginPlay()
{
	// ABuilding: team material, spawn squash, and the DT_Cards stat bind (HP from
	// the WatchTower row — GDD §3.0, never hardcoded). Its LoadStats fires the
	// OnStatsLoaded hook, which this class deliberately does NOT override.
	Super::BeginPlay();

	// After Super, because the gate derives entirely from Team and the deferred
	// spawn path sets Team before BeginPlay (TASK-030/046) — the same ordering the
	// removed ascent gate used, for the same reason.
	ConfigureLadderLink();
}

void AClimbableTower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ⚠️⚠️ TOWER-§10 L-5 — THE ONE PLACE THE LADDER REDESIGN GENUINELY ADDS RISK,
	// AND THE REASON THIS OVERRIDE EXISTS AT ALL.
	//
	// Under the ramp this was FREE: ABuilding::HandleDestroyed destroys the actor,
	// the mesh unregisters, the floor vanishes and CharacterMovement drops an
	// occupant to MOVE_Falling BY ITSELF. ⛔ A MOVE_Flying climber will NOT fall.
	// It will HANG IN THE AIR, permanently, in a match that keeps running around it.
	//
	// AbortLadderClimb is ASummonedUnit's own idempotent exit: it restores the
	// movement mode and broadcasts OnLadderClimbEnded exactly once — which re-enters
	// HandleLadderClimbEnded → ReleaseClimber below, synchronously. The explicit
	// ReleaseClimber after it is belt-and-braces for the one case that broadcast
	// cannot cover (a unit already torn down far enough that its delegate is gone);
	// ReleaseClimber is idempotent precisely so this second call is free.
	//
	// ✅ Then the climber falls and survives — there is ⛔ no fall damage anywhere in
	// Siegebound (TOWER-§4a, measured). ⚠️ TOWER-§4a's declared residual stands
	// UNCHANGED: it may land where the navmesh has not yet regenerated, and ⛔
	// nothing recovers it. ⛔ No nav-projecting teleport is added, deliberately.
	if (ASummonedUnit* const Climber = ActiveClimber.Get())
	{
		Climber->AbortLadderClimb();
		ReleaseClimber(Climber);
	}

	ActiveClimber.Reset();
	ActiveClimberPathComp.Reset();

	Super::EndPlay(EndPlayReason);
}

ECollisionChannel AClimbableTower::AscentBlockedChannel(ETeamId TowerTeam)
{
	// ⭐ THE SINGLE SOURCE OF TRUTH the team rule is written in. It used to author a
	// box's one ECR_Block as well; the box is gone (TOWER-§8.6) and the vocabulary
	// stayed, because the vocabulary was always the part doing the work.
	// SiegeNavAreas.h is the one home for the whole team-gating vocabulary.
	return SiegeEnemyTeamObjectChannel(TowerTeam);
}

bool AClimbableTower::CanTeamAscend(ETeamId TowerTeam, ETeamId ClimberTeam)
{
	// A climber is refused iff it carries the one channel this tower treats as the
	// enemy's. Own-team units match nothing and are admitted.
	//
	// ⛔ There is no capacity term and no unit-type term in this expression because
	// there are none in the rule (TOWER-§4): ANY own-team unit may ascend, however
	// many are already up there. One-at-a-time on the LADDER is a different claim
	// entirely and lives in EvaluateLadderEntry.
	return SiegeTeamObjectChannel(ClimberTeam) != AscentBlockedChannel(TowerTeam);
}

AClimbableTower::ELadderEntryVerdict AClimbableTower::EvaluateLadderEntry(
	ETeamId TowerTeam, ETeamId ClimberTeam, bool bClimberIsSummonedUnit, bool bLadderOccupied)
{
	// IDENTITY first. Anything that is not an ASummonedUnit cannot be driven by the
	// TOWER-§8.4(B) API at all — most obviously the player's own hero, which paths
	// nowhere but is a pawn all the same. ⛔ ClimberTeam is not read on this branch.
	if (!bClimberIsSummonedUnit)
	{
		return ELadderEntryVerdict::NotASummonedUnit;
	}

	// TEAM second — T-3, and the reason the precedence is fixed rather than
	// incidental: an enemy standing at a busy ladder is refused as an ENEMY. A
	// verdict that flipped with traffic could not be explained by a log line, and
	// "it worked in the test because the ladder was free" is how a gate rots.
	if (!CanTeamAscend(TowerTeam, ClimberTeam))
	{
		return ELadderEntryVerdict::WrongTeam;
	}

	// OCCUPANCY last (TOWER-§10 L-1). Two capsules interpolated along one line WILL
	// interpenetrate — bUseRVOAvoidance is false (TOWER-§4b), so units physically
	// jostle and depenetration would shove one OFF the climb line, in mid-air.
	//
	// ⚠️ "OCCUPIED" MEANS **ANY** REGISTERED CLIMB, INCLUDING BY THE VERY UNIT ON THE
	// LINK — see the header's note. Admitting a re-entry by the current climber would
	// end with the tower unbinding and forgetting a unit that is still in the air,
	// and a forgotten climber is one the tower cannot abort when it dies.
	if (bLadderOccupied)
	{
		return ELadderEntryVerdict::LadderBusy;
	}

	return ELadderEntryVerdict::Climb;
}

bool AClimbableTower::ShouldLinkAllowPathfinding(const UObject* Querier, ETeamId TowerTeam)
{
	// ⭐ MEASURED: the pathfinding query's owner is the pathing pawn's AController
	// (AIController.cpp:868 → RecastNavMesh.cpp:3885 → PImplRecastNavMesh.cpp:1258/442
	// → :439). The four citations are spelled out in full on UClimbableTowerLadderLink.
	//
	// ⚠️⚠️ EVERY UNRESOLVED CASE RETURNS TRUE, AND THAT IS THE WHOLE SAFETY
	// ARGUMENT. TOWER-§8.6 allows this layer but explicitly refuses to vouch for it,
	// so it is built such that a wrong reading of Querier degrades to the behaviour
	// of never having shipped it — the entry predicate in HandleLadderLinkReached
	// still refuses the enemy. ⛔ It must never be able to strand an own-team unit
	// by refusing it a path to its own tower.
	const AController* const AsController = Cast<AController>(Querier);
	if (!AsController)
	{
		// Not a controller-owned query: navmesh maintenance, an editor query, a
		// projection, or a null owner. ⛔ Nothing to judge — allow.
		return true;
	}

	// Cast<> copies the source's qualifiers onto the result (Casts.h:88-89), so a
	// const APawn* in yields a const ITeamAgent* — ⛔ never write Cast<const ITeamAgent>.
	const APawn* const Pawn = AsController->GetPawn();
	const ITeamAgent* const TeamAgent = Cast<ITeamAgent>(Pawn);
	if (!TeamAgent)
	{
		// A controller between possessions, or a pawn with no team. ⛔ Allow.
		return true;
	}

	// Same rule, same source of truth as the entry gate — the pathfinding layer is
	// a SECOND application of T-3, ⛔ never a second version of it.
	return CanTeamAscend(TowerTeam, TeamAgent->GetTeamId());
}

FVector AClimbableTower::ResolveLadderSocketRelative(
	const UStaticMeshComponent* Mesh, FName SocketName, const FVector& DefaultRelative, bool& bOutUsedDefault)
{
	bOutUsedDefault = true;

	// Every one of these is a legitimate pre-TASK-737 state, ⛔ not an error: the BP
	// child has not been pointed at SM_WatchTower yet, or the mesh predates the
	// sockets. ⭐ This is precisely the case the degrade-open rule exists for, and
	// it is what lets this file be correct before the art lands.
	if (!Mesh || !Mesh->GetStaticMesh() || !Mesh->GetOwner() || SocketName.IsNone() || !Mesh->DoesSocketExist(SocketName))
	{
		return DefaultRelative;
	}

	bOutUsedDefault = false;

	// ⭐ RTS_Actor, ⛔ NOT RTS_World, AND THE REASON IS A LIVE HAZARD IN THIS VERY
	// ACTOR: UNavLinkCustomComponent stores its endpoints OWNER-RELATIVE and applies
	// the owner transform on read (NavLinkCustomComponent.cpp:518-526), so it needs
	// actor space. ⚠️ AND ABuilding's spawn squash writes SetRelativeScale3D on
	// VisualMesh — which IS the root component — so for ~0.15 s after a tower spawns
	// the ACTOR's scale is wobbling by up to ±30% (SiegeMeshJuiceComponent.cpp:102-106,
	// SquashAmplitude 0.30). RTS_Actor divides that transform straight back out
	// (StaticMeshComponent.cpp:1402-1407), so what is STORED is the authored socket
	// coordinate and the squash can never be baked into the link.
	return Mesh->GetSocketTransform(SocketName, RTS_Actor).GetLocation();
}

void AClimbableTower::ConfigureLadderLink()
{
	if (!LadderLink)
	{
		return;
	}

	// ── (1) GEOMETRY FROM THE MESH, ⛔ NEVER FROM A LITERAL (TOWER-§8.4(A)) ────────
	bool bFootUsedDefault = false;
	bool bTopUsedDefault = false;
	FVector FootRelative = ResolveLadderSocketRelative(VisualMesh, LadderFootSocketName, LadderFootDefaultRelative, bFootUsedDefault);
	FVector TopRelative = ResolveLadderSocketRelative(VisualMesh, LadderTopSocketName, LadderTopDefaultRelative, bTopUsedDefault);

	// ── (2) DEGENERATE LINE ⇒ FALL BACK **BOTH** ENDS, ⛔ NEVER DISABLE THE LINK ───
	// A tower whose two sockets coincide would register a zero-length off-mesh
	// connection: path following would "arrive" at the link the instant it entered
	// it, and nobody would ever reach the deck. ⛔ Disabling the link is NOT the
	// safe answer — that makes the deck an unreachable island, i.e. a broken tower,
	// which is the one outcome TOWER-§8.4(A) forbids. Falling back to the pinned
	// pair is, because the pinned pair is guaranteed non-degenerate by arithmetic.
	const bool bDegenerate = FVector::Dist(FootRelative, TopRelative) < SiegeClimbableTowerPrivate::MinimumLadderLineUU;
	if (bDegenerate)
	{
		FootRelative = LadderFootDefaultRelative;
		TopRelative = LadderTopDefaultRelative;
	}

	// ── (3) ONE warning, naming exactly what was missing (TOWER-§8.4(A)) ──────────
	// ⛔ ONE line, ⛔ not one per socket and ⛔ not one per frame: this runs once per
	// tower, at BeginPlay. A tower that logs twice per spawn trains everyone to stop
	// reading the log.
	if (bFootUsedDefault || bTopUsedDefault || bDegenerate)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AClimbableTower '%s': ladder line taken from the TOWER-§8.3 pinned literals — %s%s%s. ")
			TEXT("The tower is FULLY FUNCTIONAL on the fallback (that is the point); it will self-correct as soon as ")
			TEXT("/Game/Meshes/SM_WatchTower ships the sockets. Foot=%s Top=%s (actor space)."),
			*GetName(),
			bFootUsedDefault ? TEXT("socket 'LadderFoot' missing") : TEXT("socket 'LadderFoot' OK"),
			bTopUsedDefault ? TEXT(", socket 'LadderTop' missing") : TEXT(", socket 'LadderTop' OK"),
			bDegenerate ? TEXT(", and the two sockets were closer than one nav-agent diameter (degenerate link)") : TEXT(""),
			*FootRelative.ToString(), *TopRelative.ToString());
	}

	// ── (4) ARM THE LINK ──────────────────────────────────────────────────────────
	// SetLinkData refreshes the navigation bounds and modifiers, so this is the
	// moment the off-mesh connection becomes real to Recast. START = the FOOT (on
	// ground navmesh), END = the TOP (on deck navmesh); BothWays makes the pair
	// symmetric, and HandleLadderLinkReached reads the travel direction off the
	// destination point rather than assuming one.
	LadderLink->SetLinkData(FootRelative, TopRelative, ENavLinkDirection::BothWays);

	// ── (5) THE TEAM GATE (T-3), IN ITS TWO LAYERS ────────────────────────────────
	// Pathfinding layer: an enemy is never handed a path THROUGH the ladder
	// (TOWER-§8.6's optional second layer, shipped because the Querier is measured;
	// it fails OPEN). Entry layer: HandleLadderLinkReached applies CanTeamAscend at
	// the one discrete entry point, and THAT is the shipped gate — the layer above
	// is defence in depth, ⛔ never the rule.
	LadderLink->SetGateTeam(Team);

	// ── (6) THE ENTRY CALLBACK ────────────────────────────────────────────────────
	// ⛔ Bound HERE and not in the constructor: a constructor binding would live on
	// the class default object.
	LadderLink->SetMoveReachedLink(this, &AClimbableTower::HandleLadderLinkReached);
}

void AClimbableTower::HandleLadderLinkReached(UNavLinkCustomComponent* LinkComp, UObject* PathComp, const FVector& DestPoint)
{
	// Belt-and-braces: this delegate is only ever bound to our own link, so the two
	// can never differ. Using our own member from here on keeps every path in this
	// file talking about the same object.
	if (!LadderLink || LinkComp != LadderLink.Get())
	{
		return;
	}

	AActor* const AgentActor = SiegeClimbableTowerPrivate::ResolvePathFollowingPawn(PathComp);
	ASummonedUnit* const Unit = Cast<ASummonedUnit>(AgentActor);

	// ⚠️ ANY registered climb makes the ladder busy — ⛔ deliberately NOT "…and it is a
	// DIFFERENT unit". The header spells out why: admitting a re-entry by the current
	// climber ends with this tower forgetting a unit that is still in the air.
	const bool bLadderOccupied = ActiveClimber.IsValid();
	const ELadderEntryVerdict Verdict = EvaluateLadderEntry(
		Team,
		Unit ? Unit->GetTeamId() : Team,
		Unit != nullptr,
		bLadderOccupied);

	if (Verdict != ELadderEntryVerdict::Climb)
	{
		// ⚠️⚠️ THE REFUSAL PATH IS LOAD-BEARING AND IT IS MEASURED, ⛔ NOT DEFENSIVE
		// PADDING: UNavLinkCustomComponent::OnLinkMoveStarted returns TRUE — "this
		// link is driving the agent now" — merely because this delegate is BOUND
		// (NavLinkCustomComponent.cpp:198-208). It never asks whether we accepted.
		// ⇒ a refusal that just returns leaves the unit in custom-link limbo
		// forever: a manufactured stuck unit (NAV-§) created by the gate that was
		// supposed to protect the tower. Handing it straight back drops it into its
		// ordinary walking state at the foot, where it re-paths, waits, or walks
		// away — and where the shipped stuck watchdog genuinely does cover it.
		ResumeAgentPathFollowing(AgentActor, PathComp);

		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("AClimbableTower '%s': ladder entry REFUSED for '%s' (verdict %d) — agent handed back to path following."),
			*GetName(), *GetNameSafe(Unit), static_cast<int32>(Verdict));
		return;
	}

	// ── THE TWO WORLD POINTS COME FROM THE LINK ITSELF ────────────────────────────
	// ⛔ Not from the sockets again and ⛔ not from a cached copy: the link is the
	// one object that already holds the armed line, so reading it back is the only
	// way the traversal and the navmesh cannot disagree about where the ladder is.
	const FVector FootWorld = LadderLink->GetStartPoint();
	const FVector TopWorld = LadderLink->GetEndPoint();

	// ── WHICH WAY (TOWER-§8.7's BothWays, cashed in) ──────────────────────────────
	// DestPoint is the far end of the move segment path following is entering
	// (PathFollowingComponent.cpp:963 passes SegmentEnd), so the endpoint it is
	// nearer IS the destination. The two endpoints are 1,236.9 uu apart, so this
	// comparison has an enormous margin. A tie reads as an ascent, which is the
	// only interpretation a degenerate link could support anyway.
	const bool bAscending = FVector::DistSquared(DestPoint, TopWorld) <= FVector::DistSquared(DestPoint, FootWorld);
	const FVector FromWorld = bAscending ? FootWorld : TopWorld;
	const FVector ToWorld = bAscending ? TopWorld : FootWorld;

	// ── BIND BEFORE THE CALL, ⛔ NEVER AFTER ──────────────────────────────────────
	// BeginLadderClimb owns the whole traversal including its exits, and a
	// degenerate or instantly-refused ascent may broadcast OnLadderClimbEnded
	// SYNCHRONOUSLY, inside this very call. Binding afterwards would miss it and
	// leave a stale ActiveClimber that bricks the ladder for the rest of the match.
	// ActiveClimber is set first for the same reason: ReleaseClimber must be able to
	// find and clear it from inside that re-entrant broadcast.
	ActiveClimber = Unit;
	ActiveClimberPathComp = PathComp;
	Unit->OnLadderClimbEnded.AddUniqueDynamic(this, &AClimbableTower::HandleLadderClimbEnded);

	// ⛔ THE TOWER DRIVES NOTHING ITSELF. It does not set a movement mode, does not
	// interpolate, does not tick and does not teleport — TOWER-§8.5 refuses a raw
	// SetActorLocation lerp outright, and the unit owns the MOVE_Flying state
	// machine and all eight of its exits (TOWER-§8.4(B)). This class contributes
	// exactly two facts: WHO climbs, and BETWEEN WHICH TWO WORLD POINTS.
	if (!Unit->BeginLadderClimb(FromWorld, ToWorld))
	{
		// "Returns false and changes NOTHING" (TOWER-§8.4(B)) — dead, match-end
		// frozen, spell-frozen, or already climbing. ⭐ NO climb started, so there is
		// ⛔ no movement mode to restore and ⛔ no ninth exit to invent here: undo our
		// own binding and hand the agent back, exactly as for a refused verdict.
		ReleaseClimber(Unit);

		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("AClimbableTower '%s': '%s' declined the climb (dead, frozen, or already climbing) — agent handed back to path following."),
			*GetName(), *GetNameSafe(Unit));
	}
}

void AClimbableTower::HandleLadderClimbEnded(ASummonedUnit* Unit, bool bReachedTop)
{
	// ⭐⭐ ONE HANDLER FOR ALL EIGHT OF THE UNIT'S EXITS, AND THAT IS DELIBERATE.
	// ASummonedUnit broadcasts this EXACTLY ONCE per successful BeginLadderClimb —
	// on arrival, AbortLadderClimb, a new order (L-4), HandleDeath, FreezeAI,
	// ApplyFreeze, its own EndPlay, and this tower's EndPlay. ⛔ The tower's response
	// is identical in every case: unbind, hand the agent back to path following,
	// free the ladder.
	//
	// ⛔ bReachedTop drives NOTHING but this log line. Arrival and abort differ for
	// the UNIT (where it ends up) and not for the TOWER (the ladder is free either
	// way). Branching on it here would be the first line of the occupancy
	// bookkeeping TOWER-§4 refuses.
	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("AClimbableTower '%s': climb ended for '%s' (reached top: %s)."),
		*GetName(), *GetNameSafe(Unit), bReachedTop ? TEXT("yes") : TEXT("no"));

	ReleaseClimber(Unit);
}

void AClimbableTower::ReleaseClimber(ASummonedUnit* Unit)
{
	// IDEMPOTENT BY CONSTRUCTION, because EndPlay calls it immediately after an
	// AbortLadderClimb that has already re-entered it: RemoveDynamic on an unbound
	// delegate is a no-op, and FinishUsingCustomLink no-ops unless the component is
	// still holding THIS link (PathFollowingComponent.cpp:1481-1494).
	if (Unit)
	{
		Unit->OnLadderClimbEnded.RemoveDynamic(this, &AClimbableTower::HandleLadderClimbEnded);
	}

	const bool bWasActive = (Unit != nullptr) && (ActiveClimber.Get() == Unit);

	// ⛔ THIS IS THE ONLY THING THIS CLASS EVER DOES TO A UNIT ON AN EXIT PATH, AND
	// IT IS A PATH-FOLLOWING HANDSHAKE — ⛔ never a movement mode. TOWER-§8.5's named
	// regression (a MOVE_Flying unit hanging in mid-air forever) can only be caused
	// by an exit that FORGETS to restore the mode; the tower is structurally
	// incapable of creating one, because it never sets a movement mode anywhere.
	// Every end of a climb goes through ASummonedUnit's own idempotent API.
	ResumeAgentPathFollowing(Unit, bWasActive ? ActiveClimberPathComp.Get() : nullptr);

	// Clear the ladder when the unit that held it is done — and also when the handle
	// has gone stale on its own, which is the defensive occupant clear TOWER-§8 (6)
	// asks for: a unit destroyed without reaching any completion path must never
	// leave the ladder permanently occupied.
	if (bWasActive || !ActiveClimber.IsValid())
	{
		ActiveClimber.Reset();
		ActiveClimberPathComp.Reset();
	}
}

void AClimbableTower::ResumeAgentPathFollowing(AActor* Agent, UObject* KnownPathComp) const
{
	if (!LadderLink)
	{
		return;
	}

	// Prefer the exact component the link handed us — it is the one whose
	// CurrentCustomLinkOb points at this link — and re-derive only if it has gone.
	UPathFollowingComponent* PathComp = Cast<UPathFollowingComponent>(KnownPathComp);

	// ANavLinkProxy::ResumePathFollowing, verbatim (NavLinkProxy.cpp:344-363): the
	// component may live on the pawn or on its controller.
	if (!PathComp && Agent)
	{
		PathComp = Agent->FindComponentByClass<UPathFollowingComponent>();
		if (!PathComp)
		{
			const APawn* const AsPawn = Cast<APawn>(Agent);
			if (AController* const Controller = AsPawn ? AsPawn->GetController() : nullptr)
			{
				PathComp = Controller->FindComponentByClass<UPathFollowingComponent>();
			}
		}
	}

	if (PathComp)
	{
		// .Get() first: FinishUsingCustomLink takes an INavLinkCustomInterface*, and
		// handing it the TObjectPtr would lean on a user-defined conversion followed by
		// a derived-to-base one. Spelling the raw pointer keeps it to a single, obvious
		// upcast.
		PathComp->FinishUsingCustomLink(LadderLink.Get());
	}
}
