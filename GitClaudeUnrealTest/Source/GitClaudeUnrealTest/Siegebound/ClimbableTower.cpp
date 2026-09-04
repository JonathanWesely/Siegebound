// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/ClimbableTower.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GitClaudeUnrealTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "Siegebound/LadderClimber.h"
#include "Siegebound/SiegeNavAreas.h"

// ⛔⛔ THE INCLUDE LIST IS STILL PART OF THE CONTRACT, AND ONE LINE **LEFT** IT ON
// 2026-09-02 — SO IT IS CALLED OUT RATHER THAN SLIPPED OUT.
//
// ⭐⭐ "Siegebound/SummonedUnit.h" IS **GONE**, AND ITS DEPARTURE IS THE HEADLINE OF
// TASK-787 (CONTACT-§12): **THIS CLASS NOW NAMES ⛔ ZERO CONCRETE CLIMBER CLASSES.**
// It was here for MOVEMENT — "a ladder has to drive a SPECIFIC unit along a SPECIFIC
// line" (TOWER-§8.4(B)'s one declared coupling surface) — and that is still true of
// the FEATURE and no longer true of the TYPE: the tower drives an `ILadderClimber`
// through the four methods that seam now carries (Begin, Abort, IsClimbing, and the
// completion delegate's accessor). ⚠️ The coupling is ⛔ not denied, it is ⛔ NARROWED:
// what remains is one capability interface, and it admits `ASummonedUnit` and
// `AHeroCharacter` on identical terms with ⛔ no branch between them.
// ⛔ Removing it is ⛔ NOT tidying: an include whose stated reason has expired is the
// CONTACT-§10.1 defect, and leaving it would keep a `Cast<ASummonedUnit>` one edit away.
//
// ⭐ WHAT DID **NOT** CHANGE, WHICH IS THE HALF THAT ACTUALLY MATTERED (HIGH-§3):
// this class still never calls HeightAdvantageMultiplier, never reports that a
// unit is elevated, never reads a unit's damage state, and includes no HIGH-§
// header. A unit on a HILL and a unit on a TOWER at the same Z deal IDENTICAL
// damage, and the damage rule still does not know towers exist.
//
// ⛔ And still NO "Siegebound/Tower.h": this class shares no code with the
// auto-firing tower family and has no fire path to arm.
//
// ⭐⭐ TWO MORE INCLUDES ARRIVED ON 2026-09-01 (TASK-777, CONTACT-§4), AND BOTH ARE
// CALLED OUT FOR THE SAME REASON THE LAST ONE WAS:
//
//   • "GameFramework/Character.h" — CONTACT-§4.4 widened the occupancy slot from
//     ASummonedUnit to ACharacter, which is the NARROWEST type that admits both
//     climber classes AND the only one that carries the API a climb needs
//     (GetCharacterMovement / GetCapsuleComponent are ACharacter members and neither
//     exists on APawn). ⭐ It also makes "a ghost can never climb" true BY TYPE:
//     ASiegeGhostPawn is an APawn, not an ACharacter.
//
//   • "Siegebound/LadderClimber.h" — the capability seam that widening REQUIRED, and
//     since TASK-787 the ONLY climber header this file has. It carries the four-method
//     interface AND `FSiegeLadderClimbEnded` itself (CONTACT-§12.3 moved the delegate
//     here from SummonedUnit.h; the TYPE NAME and SIGNATURE are UNCHANGED). ⛔ A
//     Cast<ASummonedUnit>/Cast<AHeroCharacter> branch pair was REFUSED (it hardcodes the
//     class list and is the same failed-cast shape the widening exists to remove), and
//     the interface may ⛔ not live in SiegeLadderClimbStatics.h (that pair is
//     deliberately unreflected, and a dynamic delegate would force a .generated.h into it).
//
// ⚠️⚠️ THE PREVIOUS VERSION OF THIS NOTE READ: *"ONE Cast<ASummonedUnit> REMAINS,
// DELIBERATELY AND IN EXACTLY TWO PLACES — the AddUniqueDynamic/RemoveDynamic pair … it
// is ⛔ not a class-list branch."* ⚖️ IT **WAS** ONE — CONTACT-§12.4 row 1 names it
// exactly: *"the class list with ONE entry hidden"*. It worked only while the unit was
// the only climber that could reach the slot, and it is the reason a COMPLETE, TESTED
// hero climb could not fire. ⇒ the completion lane is now reached through
// `ILadderClimber::GetOnLadderClimbEnded()`, the DRIVER is still per-class exactly as
// CONTACT-§2 ruled, and ⛔ zero concrete climber classes appear anywhere below.

// ═══════════════════════════════════════════════════════════════════════════════
//  THE PINNED CONSTANTS (TOWER-§8.3 / TOWER-§8.4(A))
// ═══════════════════════════════════════════════════════════════════════════════

// The artist↔programmer seam, spelled once (CONVENTIONS "Static-mesh SOCKET
// names"). TASK-737 authors exactly these two names on /Game/Meshes/SM_WatchTower.
const FName AClimbableTower::LadderFootSocketName(TEXT("LadderFoot"));
const FName AClimbableTower::LadderTopSocketName(TEXT("LadderTop"));

// ⛔ THE SOCKET-ABSENT FALLBACK, AND ⛔ NEVER THE PRIMARY SOURCE. Both numbers are
// navmesh arithmetic, derived in full on the header's declaration:
//   • foot X −460 clears the body's eroded nav carve (X ≤ −364) by 96 uu;
//   • top X −160 sits 76 uu inside the deck's surviving poly (X ∈ [−236, +236]).
// Together they define ONE straight segment: length 1,236.9 uu at 76.0°.
//
// ⚠️⚠️ MOVED 2026-09-02 (TASK-785) FROM −450 / −150, AND ⛔ NOT AS TIDYING — THIS PAIR
// IS THE ONE THING IN THIS FILE THAT CAN VOID A ⛔ SAFETY LICENCE WHILE EVERYTHING
// ELSE STILL RUNS. Jonathan's CONTACT-§7 K-1 = option A translated the ladder WEST by
// δ = (−10, 0, 0), and TASK-783 measured what that buys: the hero's standoff
// dist(spine, geometry) 93.619 → ⭐ 103.320 uu against the ≥ 98.0 gate, i.e. hero
// clearance 51.619 → 61.320 against a REQUIRED 56.
// ⇒ ⛔⛔ THE OLD LITERALS RECONSTRUCT THE ***UNLICENSED*** LINE. A socket that failed to
// import would have degraded open onto the exact geometry TOWER-§8.5a is VOID on — with
// one Warning, a fully functional tower, and every readback correct. ⭐ That is the
// failure class this project keeps meeting: a correct-looking fallback that quietly
// restores the broken state. The literals are ⛔ never "just the defaults" any more; they
// carry the deck-breach window's LICENCE, and they move WITH the mesh or not at all.
//
// ⭐ WHAT DID ⛔ NOT MOVE, WHICH IS WHY ⛔ NOTHING ELSE HAD TO BE RE-DERIVED: the move is a
// ⛔ PURE translation (248 of 558 verts moved by exactly δ, 310 unmoved — body, plinth,
// deck and all 8 hulls byte-identical), so Δ = Top − Foot is STILL (300, 0, 1200) ⇒
// length 1,236.9317 uu, lean 75.9638°, the watchdog budget, and the deck-breach window's
// Z-ceiling and its % of the line are ALL unchanged. ⭐ The rung plane held at −22.0, so
// A_SiegeBiped_Climb needed ⛔ no re-export either. ⚠️ The ONE margin the move SPENDS is
// the top's: 86 → 76 uu inside the deck poly. The foot's IMPROVED, 86 → 96.
const FVector AClimbableTower::LadderFootDefaultRelative(-460.f, 0.f, 0.f);
const FVector AClimbableTower::LadderTopDefaultRelative(-160.f, 0.f, 1200.f);

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
	// FOnMoveReachedLink, completion through ILadderClimber::GetOnLadderClimbEnded()
	// (TASK-787: the climber's own FSiegeLadderClimbEnded, whichever class it is) — so
	// there is nothing left to poll.

	// ⚠️ NO SetupAttachment AND NO RELATIVE TRANSFORM: UNavLinkCustomComponent is a
	// UActorComponent (via UNavRelevantComponent), ⛔ not a USceneComponent. Its two
	// endpoints are OWNER-relative and the owner's transform places them
	// (NavLinkCustomComponent.cpp:518-526). Attaching it is not "tidier" — it does
	// not compile.
	LadderLink = CreateDefaultSubobject<UClimbableTowerLadderLink>(TEXT("LadderLink"));

	// ⭐⭐⭐ THIS CLASS'S OWN STACK CEILING (STACK-§10, ruling J-13). ⛔ The base class and
	// every other building are UNCHANGED — this line moves ⛔ only the climbable tower.
	//
	// ⛔⛔ IT IS A **MEASURED SHORTFALL AGAINST WHAT JONATHAN ASKED FOR, ⛔ NOT A DESIGN
	// CHOICE, AND ⛔ NOT A BALANCE DECISION.** His spec was ×2 → ×3 → ×4 → ×5. This tower
	// stops at ×2 because of a term the whole feature was designed without noticing:
	//
	//     the DECK SLAB scales with the mesh (UCX_SM_WatchTower_07 is Z[1160, 1200] ⇒ 40·n uu
	//     thick), while FSiegeLadderClimbStatics::Begin sizes the non-swept deck-breach window
	//     as DeckBreachCapsuleHalfHeights × HalfHeight — a property of the ⛔ PAWN, invariant
	//     in n. ⇒
	//         window OPENS at capsule-centre Z = 1200n − 2·HH
	//         sweep  JAMS  at capsule-centre Z = 1160n − HH
	//         need OPEN <= JAM   <=>   40n <= HH   <=>   n <= HH / 40
	//     unit HH = 88 -> n <= 2.2 (⛔ BINDING)      hero HH = 96 -> n <= 2.4
	//
	// ⇒ ⛔ at ×3/×4/×5 the ascending capsule's TOP meets the slab underside 32/72/112 uu of Z
	// BEFORE the window opens. The climber cannot advance, hangs in MOVE_Flying, and
	// TOWER-§8.5a cl. 7's watchdog eventually DROPS it — a tower whose deck cannot be reached,
	// which is exactly the outcome the original ban existed to prevent.
	//
	// ⛔ The AI UNIT binds, ⛔ not the hero — which inverts TOWER-§8.5a's standing intuition
	// that the hero's fatter capsule is the hazard; here the hero's TALLER capsule buys it
	// MORE window. ⭐ And the headroom was declared in the source all along:
	// DeckBreachCapsuleHalfHeights' own doc comment says it is "2.2× the ~40 uu the mesh
	// actually ships". A Z stack spends exactly that headroom, linearly in n.
	//
	// ⛔⛔ DO ⛔ NOT RAISE THIS NUMBER BY HAND, and do not raise it in a .uasset either — it is
	// EditDefaultsOnly, so a Blueprint child CAN now raise it and WOULD be obeyed. If the
	// deck-breach window is ever re-engineered to be SLAB-derived rather than PAWN-derived,
	// this ceiling is RE-DERIVED from the arithmetic above (STACK-§10 cl. 2, last bullet).
	// ⛔ It is not a knob.
	MaxStackHeightMultiplier = 2;
}

void AClimbableTower::OnStackUpgradeApplied()
{
	// ⭐⭐ THE RE-ARM. ABuilding::ApplyStackUpgrade has just written the new Z onto VisualMesh
	// — which is the ROOT — so the mesh's own navigation octree entry has been refreshed by
	// USceneComponent::PropagateTransformUpdate. ⛔ THE LINK'S HAS NOT: LadderLink is a
	// UActorComponent, ⛔ not a USceneComponent, so no transform propagation reaches it and its
	// registered off-mesh connection would keep describing the PRE-UPGRADE tower.
	//
	// ⭐ Re-running the whole configure is deliberate rather than reaching in and poking
	// SetLinkData: ConfigureLadderLink re-reads the sockets in RTS_Actor space (⇒ the SAME
	// scale-free relatives, because that space divides the owner transform straight back out),
	// re-applies the team gate and re-binds the entry callback, and its SetLinkData call is
	// what drives UpdateNavigationBounds() + RefreshNavigationModifiers(). ⇒ ⛔ one call, ⛔ no
	// duplicated knowledge, and ⛔ nothing here needs to know the new height.
	//
	// ⚠️ Idempotent by construction — it is the same function BeginPlay runs, and running it
	// twice on a tower produces the same link data twice. ⛔ Its degrade-open fallback and its
	// one-warning discipline are unchanged; a tower already on the pinned literals simply logs
	// the same line again, which is the honest outcome for a tower whose sockets are missing.
	Super::OnStackUpgradeApplied();
	ConfigureLadderLink();
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
	// AbortLadderClimb is the CLIMBER's own idempotent exit — the unit's, the hero's, or
	// a future one's: it restores the movement mode and broadcasts OnLadderClimbEnded
	// exactly once, which re-enters HandleLadderClimbEnded → ReleaseClimber below,
	// synchronously. ⭐ Both climber classes now have that broadcast (TASK-787), so exit
	// H-10 releases the slot on the hero exactly as L-5 always did on the unit. The
	// explicit ReleaseClimber after it is belt-and-braces for the one case that broadcast
	// cannot cover (a climber already torn down far enough that its delegate is gone);
	// ReleaseClimber is idempotent precisely so this second call is free.
	//
	// ✅ Then the climber falls and survives — there is ⛔ no fall damage anywhere in
	// Siegebound (TOWER-§4a, measured). ⚠️ TOWER-§4a's declared residual stands
	// UNCHANGED: it may land where the navmesh has not yet regenerated, and ⛔
	// nothing recovers it. ⛔ No nav-projecting teleport is added, deliberately.
	//
	// ⭐⭐ REACHED THROUGH `ILadderClimber`, ⛔ NOT THROUGH A CONCRETE CLASS (CONTACT-§4.4's
	// ruled capability seam). The slot is now `ACharacter`, and `AbortLadderClimb()` is ⛔ not
	// an `ACharacter` member — so without the interface this line would have to be a
	// `Cast<ASummonedUnit>` / `Cast<AHeroCharacter>` branch pair, which hardcodes the class
	// list and is the SAME failed-cast shape that caused the defect the widening repairs.
	// ⚠️ A climber that somehow implements nothing is left alone rather than half-handled: the
	// `ReleaseClimber` below still runs, so the ladder is freed either way.
	if (ACharacter* const Climber = ActiveClimber.Get())
	{
		if (ILadderClimber* const ClimberApi = Cast<ILadderClimber>(Climber))
		{
			ClimberApi->AbortLadderClimb();
		}
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
	ETeamId TowerTeam, ETeamId ClimberTeam, bool bClimberIsAdmittedClimber, bool bLadderOccupied)
{
	// IDENTITY first. Anything that does not implement ILadderClimber cannot be STARTED,
	// ABORTED or HEARD FROM by this class at all — so admitting it would claim the
	// occupancy slot for a body the tower can neither drive nor release.
	//
	// ⚠️⚠️ WIDENED 2026-09-02 (TASK-787, CONTACT-§12), ⛔ NEVER REMOVED, AND THE DIFFERENCE
	// MATTERS: the caller now derives this from `Cast<ILadderClimber>`, so BOTH climber
	// classes are admitted — but an `ACharacter` that implements nothing (a future
	// spectator body) is still refused, and refused BY IDENTITY. ⭐ `ASiegeGhostPawn` never
	// reaches this function at all: it is an `APawn`, and the slot is an `ACharacter`.
	// ⛔ ClimberTeam is not read on this branch.
	if (!bClimberIsAdmittedClimber)
	{
		return ELadderEntryVerdict::NotAnAdmittedClimber;
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

	// ⭐⭐ THE TWO SEAMS, ⛔ NOT A CLASS (TASK-787, CONTACT-§12). The slot is an ACharacter and the
	// capability is ILadderClimber, so BOTH are resolved here and ⛔ neither is a concrete climber
	// class. ⛔ The interface is cast from the ACharacter, ⛔ not from the AActor, so a non-character
	// implementer could never reach the ACharacter-typed slot.
	ACharacter* const Climber = Cast<ACharacter>(AgentActor);
	ILadderClimber* const ClimberApi = Cast<ILadderClimber>(Climber);

	// ⚠️ The team is read through ITeamAgent, exactly as the CONTACT path already reads it — one
	// seam, ⛔ not one per entry path. A climber with no team falls back to this tower's own, which
	// the identity term above refuses anyway when it is not a climber at all.
	const ITeamAgent* const TeamAgent = Cast<ITeamAgent>(Climber);

	// ⚠️ ANY registered climb makes the ladder busy — ⛔ deliberately NOT "…and it is a
	// DIFFERENT unit". The header spells out why: admitting a re-entry by the current
	// climber ends with this tower forgetting a unit that is still in the air.
	// ⭐ AND THE INDEPENDENT BELT (CONTACT-§12.5): the slot also has to be holding a climber that is
	// STILL CLIMBING. See TryBeginContactClimb for the full argument — it is a BELT, ⛔ not the
	// mechanism, and it is spelled the same way on both paths so they cannot diverge.
	const bool bLadderOccupied = IsLadderSlotOccupied();
	const ELadderEntryVerdict Verdict = EvaluateLadderEntry(
		Team,
		TeamAgent ? TeamAgent->GetTeamId() : Team,
		ClimberApi != nullptr,
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
			*GetName(), *GetNameSafe(AgentActor), static_cast<int32>(Verdict));
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
	//
	// ⭐ THE BINDING GOES THROUGH `ILadderClimber::GetOnLadderClimbEnded()` (TASK-787,
	// CONTACT-§12.3) — ⛔ never through a member of a concrete climber class. That is the
	// whole completion seam: the delegate TYPE is one (FSiegeLadderClimbEnded, now in
	// LadderClimber.h) and each implementer owns the INSTANCE.
	ActiveClimber = Climber;
	ActiveClimberPathComp = PathComp;
	ClimberApi->GetOnLadderClimbEnded().AddUniqueDynamic(this, &AClimbableTower::HandleLadderClimbEnded);

	// ⛔ THE TOWER DRIVES NOTHING ITSELF. It does not set a movement mode, does not
	// interpolate, does not tick and does not teleport — TOWER-§8.5 refuses a raw
	// SetActorLocation lerp outright, and the CLIMBER owns the MOVE_Flying state
	// machine and every one of its exits (TOWER-§8.4(B); CONTACT-§2 keeps the DRIVER
	// per-class). This class contributes exactly two facts: WHO climbs, and BETWEEN
	// WHICH TWO WORLD POINTS.
	if (!ClimberApi->BeginLadderClimb(FromWorld, ToWorld))
	{
		// "Returns false and changes NOTHING" (TOWER-§8.4(B)) — dead, match-end
		// frozen, spell-frozen, or already climbing. ⭐ NO climb started, so there is
		// ⛔ no movement mode to restore and ⛔ no ninth exit to invent here: undo our
		// own binding and hand the agent back, exactly as for a refused verdict.
		ReleaseClimber(Climber);

		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("AClimbableTower '%s': '%s' declined the climb (dead, frozen, or already climbing) — agent handed back to path following."),
			*GetName(), *GetNameSafe(Climber));
	}
}

void AClimbableTower::HandleLadderClimbEnded(ACharacter* Climber, bool bReachedTop)
{
	// ⭐⭐ ONE HANDLER FOR EVERY EXIT OF EVERY CLIMBER, AND THAT IS DELIBERATE.
	// A climber broadcasts this EXACTLY ONCE per successful BeginLadderClimb — the unit
	// on its eight (arrival, AbortLadderClimb, a new order (L-4), HandleDeath, FreezeAI,
	// ApplyFreeze, its own EndPlay, and this tower's EndPlay), the hero on its TEN, all of
	// which route through its ONE teardown (CONTACT-§3.1). ⛔ The tower's response is
	// identical in every case, and identical for both classes: unbind, hand the agent back
	// to path following, free the ladder.
	// ⚠️⚠️ THIS IS THE ONE CHANNEL BY WHICH THE LADDER IS EVER FREED EAGERLY — which is why
	// TASK-787 could not widen the START without widening THIS (CONTACT-§12.1): a climber
	// admitted and never heard from holds the slot for the rest of the match.
	//
	// ⛔ bReachedTop drives NOTHING but this log line. Arrival and abort differ for
	// the UNIT (where it ends up) and not for the TOWER (the ladder is free either
	// way). Branching on it here would be the first line of the occupancy
	// bookkeeping TOWER-§4 refuses.
	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("AClimbableTower '%s': climb ended for '%s' (reached top: %s)."),
		*GetName(), *GetNameSafe(Climber), bReachedTop ? TEXT("yes") : TEXT("no"));

	// ── ⛔⛔ K-C'S RE-ARM LATCH, ARMED HERE AND ⛔ NOWHERE ELSE (CONTACT-§4.2) ─────────────
	// ⚠️⚠️ THE DEFECT THIS PRE-EMPTS IS CERTAIN, ⛔ NOT HYPOTHETICAL: a pawn that finishes a
	// DESCENT is standing at the ladder FOOT, inside the contact radius, still supplying the
	// very input that brought it there ⇒ ⛔ it re-climbs INSTANTLY and the player is stuck in a
	// yo-yo. The ASCENT mirror is just as certain — it finishes standing on the deck, at the
	// top endpoint, still pressing forward.
	//
	// ⭐ ARMED FROM **HERE** RATHER THAN FROM THE CONTACT PATH, AND THAT IS THE POINT: this is
	// the ONE place every climb ends, whichever path started it, so a nav-link ascent and a
	// contact ascent are protected by the same line. ⛔ It also does NOT read bReachedTop —
	// the endpoint is resolved from WHERE THE PAWN IS, by the same Z rule that admitted it, so
	// an aborted climb latches correctly too and the "bReachedTop drives nothing" property
	// above survives intact.
	if (Climber && LadderLink)
	{
		FSiegeLadderContactStatics::Disarm(
			FindOrAddContactState(Climber),
			FSiegeLadderContactStatics::IsAtTopEndpoint(
				Climber->GetActorLocation(), LadderLink->GetStartPoint(), LadderLink->GetEndPoint()));
	}

	ReleaseClimber(Climber);
}

void AClimbableTower::ReleaseClimber(ACharacter* Climber)
{
	// IDEMPOTENT BY CONSTRUCTION, because EndPlay calls it immediately after an
	// AbortLadderClimb that has already re-entered it: RemoveDynamic on an unbound
	// delegate is a no-op, and FinishUsingCustomLink no-ops unless the component is
	// still holding THIS link (PathFollowingComponent.cpp:1481-1494).
	//
	// ⭐⭐ THE UNBIND GOES THROUGH `ILadderClimber::GetOnLadderClimbEnded()` (TASK-787,
	// CONTACT-§12.3) — the exact mirror of the two bind sites, and ⛔ NO concrete climber
	// class is named. ⚠️ WHAT USED TO BE HERE, AND WHY IT HAD TO GO: a `Cast<ASummonedUnit>`
	// reaching a delegate that existed on that class and nowhere else. It read as "not a
	// class-list branch" and it WAS one — CONTACT-§12.4 row 1's *"the class list with ONE
	// entry hidden"* — because it worked only while the unit was the only climber that
	// could hold the slot. A hero would have bound nothing, broadcast nothing, and held
	// the ladder for the rest of the match.
	// ⚠️ A climber that implements nothing simply had nothing bound to unbind, and every
	// other line below still runs for it: the release is ⛔ NEVER conditional on the
	// binding, which is exactly why there is ⛔ no "was it bound?" flag (CONTACT-§12.6).
	if (ILadderClimber* const ClimberApi = Cast<ILadderClimber>(Climber))
	{
		ClimberApi->GetOnLadderClimbEnded().RemoveDynamic(this, &AClimbableTower::HandleLadderClimbEnded);
	}

	const bool bWasActive = (Climber != nullptr) && (ActiveClimber.Get() == Climber);

	// ⛔ THIS IS THE ONLY THING THIS CLASS EVER DOES TO A UNIT ON AN EXIT PATH, AND
	// IT IS A PATH-FOLLOWING HANDSHAKE — ⛔ never a movement mode. TOWER-§8.5's named
	// regression (a MOVE_Flying unit hanging in mid-air forever) can only be caused
	// by an exit that FORGETS to restore the mode; the tower is structurally
	// incapable of creating one, because it never sets a movement mode anywhere.
	// Every end of a climb goes through the CLIMBER's own idempotent API
	// (ILadderClimber::AbortLadderClimb, implemented per class — CONTACT-§2).
	ResumeAgentPathFollowing(Climber, bWasActive ? ActiveClimberPathComp.Get() : nullptr);

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

bool AClimbableTower::IsLadderSlotOccupied() const
{
	// ⭐⭐ THE OCCUPANCY TERM, SPELLED **ONCE** FOR BOTH ENTRY PATHS (TASK-787, CONTACT-§12.5).
	//
	// ⛔⛔ IT IS A **BELT**, ⛔ NOT THE MECHANISM, AND IT DOES ⛔ NOT REPLACE THE COMPLETION
	// SIGNAL. The eager release — the climber broadcasts, HandleLadderClimbEnded fires,
	// ReleaseClimber clears the handle — is what actually frees the ladder. This second term
	// exists so that a completion signal that is ever MISSED (a future climber class, a
	// mis-ordered broadcast, a binding that failed) degrades the worst case from ⛔ "the tower
	// is dead for the rest of the match" to "one admission is late by one poll".
	//
	// ⭐ AND IT COSTS ⛔ NOTHING IN LATENCY: the term is evaluated at every admission attempt,
	// so a slot released a moment ago reads FREE at that instant.
	//
	// ⚠️ A HELD CLIMBER THIS CLASS CANNOT INTERROGATE READS **OCCUPIED**, deliberately: the
	// conservative direction is the one that preserves TOWER-§10 L-1 (one ladder, one climber),
	// and after the CONTACT-§12 widening the identity term means only an ILadderClimber can be
	// in the slot at all — so this branch is unreachable by construction rather than by hope.
	//
	// ⛔ ONE FUNCTION, TWO CALLERS — ⛔ NEVER TWO COPIES OF THE EXPRESSION: divergent release
	// semantics between the link path and the contact path is precisely what CONTACT-§12.5's
	// closing clause forbids.
	// ⚠️ `Held` is deliberately NON-const even inside this const method (TWeakObjectPtr::Get()
	// hands back a mutable pointer): `Cast<>` to an INTERFACE is spelled on the non-const
	// pointer here — the same shape the ITeamAgent read in TryBeginContactClimb already uses —
	// and the result is stored const, which is what actually enforces "this term only READS".
	ACharacter* const Held = ActiveClimber.Get();
	if (!Held)
	{
		return false;
	}

	const ILadderClimber* const HeldApi = Cast<ILadderClimber>(Held);
	return HeldApi ? HeldApi->IsClimbing() : true;
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

// ═══════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ THE CONTACT TRIGGER (TASK-777, CONTACT-§4) — "walking against it"
// ═══════════════════════════════════════════════════════════════════════════════

AClimbableTower::ELadderContactVerdict AClimbableTower::EvaluateContactEntry(
	FSiegeLadderContactState& ContactState,
	const FVector& ClimberLocation, const FVector& ClimberVelocity,
	const FVector& FootWorld, const FVector& TopWorld,
	float RadiusUU, float IntentCos, float RequiredDwellSeconds, float DeltaSeconds,
	ETeamId TowerTeam, ETeamId ClimberTeam, bool bClimberIsAdmittedClimber, bool bLadderOccupied,
	bool& bOutAscending)
{
	// ── (1) THE THREE CONTACT TERMS, **FIRST** ────────────────────────────────────────────
	// ⛔⛔ THE ORDER IS PART OF THE CONTRACT AND IT IS ⛔ NOT THE OBVIOUS ONE. Putting the
	// cheap team/occupancy refusals first would look tidier and would introduce a real,
	// silent defect: K-C's latch is maintained by WantsToClimb, so a pawn latched at the
	// foot while SOMEBODY ELSE holds the ladder would be short-circuited out on LadderBusy
	// every frame, never observed leaving the radius, and would carry that latch for the rest
	// of the match — ⛔ a pawn permanently unable to climb, for a reason nothing logs.
	const ESiegeLadderContactVerdict ContactVerdict = FSiegeLadderContactStatics::WantsToClimb(
		ContactState, ClimberLocation, ClimberVelocity, FootWorld, TopWorld,
		RadiusUU, IntentCos, RequiredDwellSeconds, DeltaSeconds, bOutAscending);

	switch (ContactVerdict)
	{
	case ESiegeLadderContactVerdict::TooFar:       return ELadderContactVerdict::TooFar;
	case ESiegeLadderContactVerdict::NotHeadingIn: return ELadderContactVerdict::NotHeadingIn;
	case ESiegeLadderContactVerdict::Dwelling:     return ELadderContactVerdict::Dwelling;
	case ESiegeLadderContactVerdict::Disarmed:     return ELadderContactVerdict::Disarmed;
	case ESiegeLadderContactVerdict::Climb:        break;
	}

	// ── (2) THE SHIPPED ENTRY GATE, UNCHANGED AND UN-RE-IMPLEMENTED ───────────────────────
	// ⛔⛔ THE SAME FUNCTION THE NAV-LINK PATH CALLS, WITH THE SAME PRECEDENCE (identity →
	// team → occupancy) AND THE SAME VERDICTS. ⚠️⚠️ CanTeamAscend therefore gains a SECOND
	// CALLER and ⛔ never an exception (CONTACT-§4.3): a contact path that re-implemented the
	// team question — or skipped it — would be a ⛔ SILENT BACK DOOR around Jonathan's T-3
	// ruling, opened by a task whose stated purpose was something else entirely, and ⛔ no
	// reviewer would have a reason to look for it. ⭐ There is ⛔ no hero exemption: an ENEMY
	// hero may ⛔ not climb your tower either, and it is refused by this same line.
	//
	// ⭐ AND THIS IS ALSO WHY THE TWO PATHS CANNOT DOUBLE-FIRE: ONE gate, reading ONE
	// occupancy slot. The second entrant — from either path — is LadderBusy.
	switch (EvaluateLadderEntry(TowerTeam, ClimberTeam, bClimberIsAdmittedClimber, bLadderOccupied))
	{
	case ELadderEntryVerdict::NotAnAdmittedClimber: return ELadderContactVerdict::NotAnAdmittedClimber;
	case ELadderEntryVerdict::WrongTeam:        return ELadderContactVerdict::WrongTeam;
	case ELadderEntryVerdict::LadderBusy:       return ELadderContactVerdict::LadderBusy;
	case ELadderEntryVerdict::Climb:            break;
	}

	return ELadderContactVerdict::Climb;
}

AClimbableTower::ELadderContactVerdict AClimbableTower::TryBeginContactClimb(ACharacter* Climber, float DeltaSeconds)
{
	if (!Climber || !LadderLink)
	{
		return ELadderContactVerdict::NoLadder;
	}

	// ── THE TWO WORLD POINTS COME FROM THE LINK ITSELF ────────────────────────────────────
	// ⛔ Not from the sockets again and ⛔ not from a cached copy — the identical reasoning
	// HandleLadderLinkReached states: the link is the one object holding the armed line, so
	// reading it back is the only way the traversal, the navmesh and the CONTACT TRIGGER
	// cannot disagree about where the ladder is. ⭐ Three consumers, one source.
	const FVector FootWorld = LadderLink->GetStartPoint();
	const FVector TopWorld = LadderLink->GetEndPoint();
	const FVector ClimberLocation = Climber->GetActorLocation();

	// ── THE CHEAP TERM RUNS BEFORE ANY BOOKKEEPING, AND IT DOES THREE JOBS ────────────────
	// ⭐ Proximity is the only term that needs no state, so it gates the table: a pawn outside
	// the radius has its row DROPPED, which (a) re-arms K-C's latch — leaving the radius is
	// one of its two re-arm conditions — (b) clears the dwell and (c) prunes the table. ⇒ the
	// table is bounded by the pawns actually standing at this ladder, and a passer-by costs
	// one squared-distance compare and nothing else.
	// ⚠️ The same test is re-run inside WantsToClimb, deliberately: that function must be
	// COMPLETE on its own or it is not the pure, headlessly-testable predicate CONTACT-§4.1
	// requires. The duplicate is two multiplies.
	const bool bAtTop = FSiegeLadderContactStatics::IsAtTopEndpoint(ClimberLocation, FootWorld, TopWorld);
	const FVector& Endpoint = bAtTop ? TopWorld : FootWorld;
	const float SafeRadiusUU = FMath::Max(LadderContactRadiusUU, 0.f);

	if (FVector::DistSquared2D(ClimberLocation, Endpoint) > (SafeRadiusUU * SafeRadiusUU))
	{
		ForgetContact(Climber);
		return ELadderContactVerdict::TooFar;
	}

	// ⚠️ The team is read through ITeamAgent rather than off a concrete pawn class — the same seam
	// ShouldLinkAllowPathfinding already uses in this file. A pawn with no team falls back to
	// this tower's own, which the identity term then refuses anyway.
	const ITeamAgent* const TeamAgent = Cast<ITeamAgent>(Climber);

	// ⭐⭐ THE IDENTITY TERM IS A **CAPABILITY**, ⛔ NOT A CLASS (TASK-787, CONTACT-§12.2). Resolved
	// ONCE, here, and reused below as the start seam — so the cast that admits the climber and the
	// cast that drives it are the SAME cast and cannot disagree.
	ILadderClimber* const ClimberApi = Cast<ILadderClimber>(Climber);

	bool bAscending = true;
	const ELadderContactVerdict Verdict = EvaluateContactEntry(
		FindOrAddContactState(Climber),
		ClimberLocation, Climber->GetVelocity(), FootWorld, TopWorld,
		SafeRadiusUU, LadderContactIntentCos, LadderContactDwellSeconds, DeltaSeconds,
		Team,
		TeamAgent ? TeamAgent->GetTeamId() : Team,
		ClimberApi != nullptr,
		IsLadderSlotOccupied(),
		bAscending);

	if (Verdict != ELadderContactVerdict::Climb)
	{
		// ⛔ NO LOG LINE HERE, DELIBERATELY. This runs out of a pawn's tick, so a Verbose line
		// per refusal is one per pawn per frame, forever — the "a tower that logs twice per
		// spawn trains everyone to stop reading the log" rule at ConfigureLadderLink, applied
		// where it matters far more. Only the two EVENTS below log.
		return Verdict;
	}

	// ── THE CLIMBER IS ADMITTED. START IT, EXACTLY AS THE LINK PATH DOES ──────────────────
	// ⭐⭐ THROUGH `ILadderClimber`, AND THROUGH THE **SAME** POINTER THE IDENTITY TERM WAS
	// DERIVED FROM (TASK-787, CONTACT-§12.2). ⚠️ WHAT USED TO BE HERE WAS A
	// `CastChecked<ASummonedUnit>` — safe only because the identity term admitted that ONE
	// class, and the exact line that made a complete, tested hero climb unable to fire.
	// ⛔ There is deliberately ⛔ NO re-cast and ⛔ no second check: a second cast is a second
	// chance for the two to disagree.

	// ── BIND BEFORE THE CALL, ⛔ NEVER AFTER ─────────────────────────────────────────────
	// BeginLadderClimb owns the whole traversal including its exits, and a degenerate or
	// instantly-refused ascent may broadcast OnLadderClimbEnded SYNCHRONOUSLY, inside this
	// very call. Binding afterwards would miss it and leave a stale ActiveClimber that bricks
	// the ladder for the rest of the match. ActiveClimber is set first for the same reason:
	// ReleaseClimber must be able to find and clear it from inside that re-entrant broadcast.
	//
	// ⚠️ ActiveClimberPathComp is CLEARED rather than set: a contact entry is ⛔ not a
	// path-following handshake — nothing handed this pawn to the link, so there is nothing to
	// hand back. ReleaseClimber's ResumeAgentPathFollowing then re-derives a component and
	// calls FinishUsingCustomLink, which no-ops unless that component is actually holding THIS
	// link (PathFollowingComponent.cpp:1481-1494). ⇒ harmless for a contact climber, and
	// correct for the one that walked in off a path.
	ActiveClimber = Climber;
	ActiveClimberPathComp.Reset();
	ClimberApi->GetOnLadderClimbEnded().AddUniqueDynamic(this, &AClimbableTower::HandleLadderClimbEnded);

	// ⛔ THE TOWER STILL DRIVES NOTHING ITSELF — it does not set a movement mode, does not
	// interpolate, does not tick and does not teleport. It contributes exactly two facts, the
	// same two the link path contributes: WHO climbs, and BETWEEN WHICH TWO WORLD POINTS.
	// ⭐ EACH CLIMBER'S OWN `BeginLadderClimb` TAKES SOLE STEERING AUTHORITY BEFORE IT TAKES THE
	// CAPSULE — the unit by calling `AIController::StopMovement()`, the hero by capturing the
	// player's steer instead of forwarding it (`CONTACT-§3`'s `DoMove` override) — so a contact
	// climb started mid-move has ONE driver (the NAV-§3 no-double-driver law). ⛔ That is the
	// CLIMBER's half of the contract, ⛔ not this class's, and it is per-class by CONTACT-§2.
	const FVector FromWorld = bAscending ? FootWorld : TopWorld;
	const FVector ToWorld = bAscending ? TopWorld : FootWorld;

	if (!ClimberApi->BeginLadderClimb(FromWorld, ToWorld))
	{
		// "Returns false and changes NOTHING" (TOWER-§8.4(B)) — dead, match-end frozen,
		// spell-frozen, or ALREADY CLIMBING (the hero adds recall-channelling, CONTACT-§3.4).
		// ⭐ NO climb started, so there is ⛔ no movement mode to restore and ⛔ no extra exit to
		// invent here: undo our own binding, exactly as the link path does on the same refusal.
		ReleaseClimber(Climber);

		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("AClimbableTower '%s': '%s' walked into the ladder but declined the climb (dead, frozen, or already climbing)."),
			*GetName(), *GetNameSafe(Climber));
		return ELadderContactVerdict::Declined;
	}

	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("AClimbableTower '%s': CONTACT climb started for '%s' (%s)."),
		*GetName(), *GetNameSafe(Climber), bAscending ? TEXT("ascending") : TEXT("descending"));

	return ELadderContactVerdict::Climb;
}

FSiegeLadderContactState& AClimbableTower::FindOrAddContactState(ACharacter* Climber)
{
	// Prune every dead handle on the way through. ⭐ Weak on purpose, exactly as ActiveClimber
	// is: a pawn destroyed without ever leaving the radius must ⛔ not leave a row behind, and
	// a table that only ever grows is a leak nobody notices inside a five-minute match.
	for (int32 Index = LadderContacts.Num() - 1; Index >= 0; --Index)
	{
		if (!LadderContacts[Index].Climber.IsValid())
		{
			LadderContacts.RemoveAtSwap(Index);
		}
	}

	for (FLadderContactEntry& Entry : LadderContacts)
	{
		if (Entry.Climber.Get() == Climber)
		{
			return Entry.State;
		}
	}

	FLadderContactEntry& NewEntry = LadderContacts.AddDefaulted_GetRef();
	NewEntry.Climber = Climber;
	return NewEntry.State;
}

void AClimbableTower::ForgetContact(const ACharacter* Climber)
{
	// ⭐ ONE OPERATION, THREE JOBS (see the LadderContacts note): leaving the radius is one of
	// K-C's two re-arm conditions, so dropping the row IS the re-arm — and it clears the dwell
	// and prunes the table at the same time. ⚖️ A latch that had to be cleared by a separate
	// line is a latch somebody eventually forgets to clear.
	for (int32 Index = LadderContacts.Num() - 1; Index >= 0; --Index)
	{
		const ACharacter* const Existing = LadderContacts[Index].Climber.Get();
		if (!Existing || Existing == Climber)
		{
			LadderContacts.RemoveAtSwap(Index);
		}
	}
}
