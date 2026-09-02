// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/Navigation/NavLinkDefinition.h"
#include "Engine/EngineTypes.h"
#include "NavLinkCustomComponent.h"
#include "Siegebound/Building.h"
#include "Siegebound/SiegeLadderClimbStatics.h" // TASK-777 (CONTACT-§4.1): FSiegeLadderContactState is a BY-VALUE member of the contact table below, and ESiegeLadderContactVerdict is a return type — complete types required here, ⛔ not a forward declaration
#include "Siegebound/TeamId.h"
#include "ClimbableTower.generated.h"

class ACharacter;
//~ TASK-787 (CONTACT-§12): `class ASummonedUnit;` is GONE from this list, and its absence is the
//~ headline rather than a tidy — ⛔ this class no longer names a concrete climber type in a
//~ declaration anywhere. The `.cpp`'s include went with it; the capability seam (`ILadderClimber`,
//~ reached from `Siegebound/LadderClimber.h`) is all that is left.
class UPathFollowingComponent;
class UStaticMeshComponent;

/**
 *  ═══ THE LADDER'S SMART LINK (TASK-734, CONVENTIONS TOWER-§8.4/§8.6/§8.7) ═══
 *
 *  A `UNavLinkCustomComponent` with exactly ONE addition: the OPTIONAL second
 *  gate layer of `TOWER-§8.6` — an enemy is refused a PATH through the ladder,
 *  not merely turned away at its foot.
 *
 *  ⭐⭐ WHY A SUBCLASS RATHER THAN THE STOCK COMPONENT, AND WHY THE LAW SAID
 *  "ONLY IF YOU MEASURE IT": `TOWER-§8.6` allows `IsLinkPathfindingAllowed` but
 *  refuses to ASSERT that it works, because nobody had measured what `Querier`
 *  actually is. ⭐ IT IS NOW MEASURED, END TO END, AT FOUR CITED SITES:
 *
 *    1. AIController.cpp:868  — `AAIController::BuildPathfindingQuery` builds
 *       `OutQuery = FPathFindingQuery(*this, …)`, i.e. the query's Owner is the
 *       CONTROLLER. Every unit move in this project goes through
 *       `AAIController::MoveToActor` / `MoveToLocation`
 *       (SummonedUnit.cpp:2652/:2679, MinerUnit.cpp:921/:986), so the owner is
 *       always our own `ASiegeUnitAIController`.
 *    2. RecastNavMesh.cpp:3885 — `ARecastNavMesh::FindPath` hands
 *       `Query.Owner.Get()` straight to `FPImplRecastNavMesh::FindPath(… Owner)`.
 *    3. PImplRecastNavMesh.cpp:1258/442 — that Owner becomes
 *       `FRecastSpeciaLinkFilter::SearchOwner`, and `initialize()` caches it into
 *       `CachedOwnerOb`; `DetourNavMeshQuery.cpp:311-316` proves `initialize()` is
 *       always called by `dtNavMeshQuery::init`, so the cache is never stale-null
 *       by accident.
 *    4. PImplRecastNavMesh.cpp:439 — `isLinkAllowed` then calls
 *       `IsLinkPathfindingAllowed(CachedOwnerOb)`.
 *
 *  ⇒ **`Querier` is the pathing pawn's `AController`.** ⛔ Nothing else is
 *  assumed, and every step above is re-derivable from the cited file and line.
 *
 *  ⚠️⚠️ AND IT FAILS **OPEN**, DELIBERATELY (`AClimbableTower::ShouldLinkAllowPathfinding`):
 *  a Querier that is not a controller, a controller with no pawn, or a pawn that
 *  is not an `ITeamAgent` all return TRUE. ⭐ The worst case of a wrong
 *  measurement is therefore EXACTLY the behaviour of not shipping this layer at
 *  all — the entry predicate in `HandleLadderLinkReached` still refuses the
 *  enemy — and ⛔ never a stuck own-team unit that cannot path to its own tower.
 *
 *  ⚠️ THREADING, STATED RATHER THAN HIDDEN: pathfinding may run off the game
 *  thread, so this override does pointer reads and one enum compare and ⛔ nothing
 *  else — ⛔ no world query, ⛔ no allocation, ⛔ no component lookup. `GateTeam`
 *  is written once, at the owning tower's `BeginPlay`, before any path can
 *  traverse a link that does not exist yet.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UClimbableTowerLadderLink : public UNavLinkCustomComponent
{
	GENERATED_BODY()

public:

	UClimbableTowerLadderLink(const FObjectInitializer& ObjectInitializer);

	/** `TOWER-§8.6`'s optional second layer: refuse an ENEMY a path THROUGH the ladder. Fails OPEN on anything it cannot resolve. */
	virtual bool IsLinkPathfindingAllowed(const UObject* Querier) const override;

	/** Pushed once by the owning tower at BeginPlay, when Team is authoritative (the ConfigureAscentGate ordering this class inherits). */
	void SetGateTeam(ETeamId InTeam) { GateTeam = InTeam; }

	/** The team this link admits — the owning tower's. Read on the pathfinding thread; see the threading note above. */
	ETeamId GetGateTeam() const { return GateTeam; }

private:

	/** ⛔ Not a UPROPERTY: it is derived state pushed from ABuilding::Team, never authored, never serialized, never edited. */
	ETeamId GateTeam = ETeamId::Blue;
};

/**
 *  ═══ THE CLIMBABLE WATCH TOWER (TASK-726 → TASK-734, CONVENTIONS TOWER-§3/§4/§5,
 *      TOWER-§8, TOWER-§9, TOWER-§10) ═══
 *
 *  A 30-gold building (CardID WatchTower) whose entire reason to exist is a
 *  1,200-uu PLATFORM a unit can stand on. It is a PLACE, ⛔ not a weapon.
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ THIS CLASS WAS RE-SCOPED 2026-09-01 BY JONATHAN'S DIRECTIVE — *"instead of
 *  making it a ramp that you walk up it instead has a ladder you climb up"*. THE
 *  FEATURE DID ⛔ NOT CHANGE: the card, the 30 gold, `HIGH-§`, the ×3, the 600×600
 *  deck and the 1,200 uu rise are byte-for-byte what they were (`TOWER-§8.0`).
 *  ⭐ EXACTLY ONE THING CHANGED: **how a unit gets to the top.**
 *
 *  ⛔⛔ AND A LADDER IS ⛔ NOT A GEOMETRY SWAP — IT IS A TRAVERSAL SUBSYSTEM. THE
 *  THREE MEASUREMENTS THAT SIZE THIS FILE (`TOWER-§8.1`, each read at the engine
 *  source, ⛔ none relayed):
 *
 *    M-1 Recast's real walkable ceiling is `atan(20/32)` = **32.005°**
 *        (`rcFilterLedgeSpans`, NOT the 44° `AgentMaxSlope` suggests). The climb
 *        line is **76.0°** — **2.4× over**. ⇒ ⛔ NO amount of mesh authoring makes
 *        a ladder walkable, so without an off-mesh connection the deck is an
 *        UNREACHABLE ISLAND and the card does nothing at all.
 *    M-3 `UPathFollowingComponent::SetMoveSegment` calls `StartUsingCustomLink`
 *        **only** when `PathPt0.CustomNavLinkId != FNavLinkId::Invalid`
 *        (PathFollowingComponent.cpp:959-963), and a SIMPLE `PointLinks` entry
 *        carries no such id. ⇒ ⛔ a simple link is NOT enough; a SMART link
 *        (`UNavLinkCustomComponent`) is REQUIRED.
 *    M-4 `UCharacterMovementComponent::ConstrainInputAcceleration`
 *        (CharacterMovementComponent.cpp:8121-8131) DELETES the vertical component
 *        of steering input for any walking or falling pawn, every frame, by
 *        design. ⇒ ⛔ steering alone cannot lift a unit one centimetre; the
 *        traversal must be DRIVEN. The CLIMBER drives it — `ASummonedUnit`
 *        (TASK-738) and `AHeroCharacter` (TASK-778), each with its own driver and
 *        its own exits (`CONTACT-§2`); this class only decides WHO, WHEN and
 *        BETWEEN WHICH TWO POINTS.
 *
 *  ⭐⭐ AND THE PROPERTY THAT SURVIVED THE REDESIGN INTACT, WHICH IS WHY THE
 *  DELEGATE MATTERS: **THIS CLASS STILL NEVER TICKS AND STILL ARMS NO TIMER.**
 *  Entry arrives as a nav-link callback (or as a pawn's own poll) and completion
 *  arrives as the climber's `OnLadderClimbEnded`, reached through
 *  `ILadderClimber::GetOnLadderClimbEnded()` (TASK-787); both are pushed to us, so
 *  there is nothing to poll (`TOWER-§8` (4)).
 *
 *  ⭐⭐ AND IT SURVIVED THE **CONTACT TRIGGER** TOO (TASK-777, `CONTACT-§4`), WHICH
 *  WAS THE HARDER OF THE TWO — because a DWELL is a continuous measurement and the
 *  obvious way to take one is to poll for it. It is not polled here: `WR-§5`'s
 *  idiom, third application, and it is MEASURED rather than preferred —
 *  `ACommanderNpc` ships `bCanEverTick = false` (`CommanderNpc.cpp:105`) with its
 *  own comment reading *"the proximity gate is POLLED BY THE CALLER through
 *  IsPlayerInRange"*, and the caller is `ASiegePlayerController.cpp:4970`. ⇒ **THE
 *  RADIUS AND THE TEST LIVE HERE; THE PAWN ONLY ASKS**, once per frame, out of the
 *  tick it already pays for. ⛔ A tower tick would ALSO turn test 10(c) of
 *  Tests/SiegeClimbableTowerTest.cpp red by name — the no-poll rule is enforced,
 *  ⛔ not merely written down.
 *  ─────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔⛔ A SIBLING OF ATower, ⛔ NEVER A SUBCLASS OF IT (TOWER-§5). ATower's
 *  OnStatsLoaded override binds Damage/Range/Cadence and arms a fire timer;
 *  inheriting an auto-fire cadence loop that this card explicitly does not want
 *  is how a "harmless" base class becomes a bug. The WatchTower row ships
 *  Damage/Range/Cadence = 0/0/0 (TOWER-§3) so no timer could arm anyway
 *  (ABuilding's qa/TASK-021 WARN-1 law) — ⭐ belt and braces: THIS CLASS SIMPLY
 *  HAS NO FIRE PATH TO ARM. It does not override OnStatsLoaded at all.
 *  ⛔ Tower.{h,cpp} is NOT touched; four shipped tower cards depend on it.
 *
 *  HP binds from the WatchTower row through the shipped ABuilding path (GDD
 *  §3.0), ⛔ never hardcoded. Blocking, nav carving, the health bar, the team
 *  material, hit flash, spawn squash, freeze and destruction are all ABuilding's,
 *  unchanged and un-overridden.
 *
 *  ─── WHAT THIS CLASS DELIBERATELY DOES ⛔ NOT CONTAIN (TOWER-§4/§10, each with
 *  its reason — Tests/SiegeClimbableTowerTest.cpp asserts these ABSENCES) ───
 *
 *  ⛔ NO CAPACITY COUNTER. Physical space is the cap: a 600×600 uu platform holds
 *     4–6 bodies at AgentRadius 34. ⭐ An entire occupancy subsystem — a counter,
 *     a full/refuse state, its UI, its replication and its desync cases — stays
 *     DELETED. ⚠️ `ActiveClimber` below is ⛔ NOT that subsystem and is ⛔ not a
 *     capacity term: it is ONE handle to the ONE unit currently ON THE LADDER,
 *     required by `TOWER-§10` L-1 (one climber at a time) and by L-5 (the tower
 *     must be able to abort a climb it started when it dies). Nothing counts who
 *     is standing on the DECK, and nothing ever will.
 *
 *  ⛔ NO RANGED-ONLY FILTER. Any OWN-TEAM unit may climb. Jonathan's "any ranged
 *     unit CAN climb it" is a PERMISSION, not an exclusion, and a filter admitting
 *     3 CardIDs while rejecting 8 manufactures the stuck-unit class NAV-§ exists
 *     for. A melee unit on top gains nothing (HIGH-§4 gates the bonus on the
 *     already-shipped bRangedAttack member) and self-selects away.
 *
 *  ⛔ NO FALL DAMAGE AND NO CATCH. When the tower dies its mesh unregisters and
 *     occupants LAND and resume walking; this project has ⛔ NO FALL DAMAGE
 *     ANYWHERE (measured in TASK-725 §8). ⚠️⚠️ BUT THAT IS ⛔ NO LONGER FREE FOR A
 *     CLIMBER, AND IT IS THE ONE PLACE THIS REDESIGN GENUINELY ADDS RISK
 *     (`TOWER-§10` L-5): under the ramp the floor vanished and CharacterMovement
 *     dropped to MOVE_Falling BY ITSELF. A `MOVE_Flying` climber will ⛔ NOT fall
 *     — it will HANG IN THE AIR, FOREVER. ⇒ `EndPlay` explicitly aborts the climb
 *     it started. ⚠️ TOWER-§4a's declared residual (landing where the navmesh has
 *     not yet regenerated, with ⛔ no recovery lane) stands UNCHANGED and unsolved
 *     by design — ⛔ no nav-projecting teleport is added here or anywhere.
 *
 *  ⛔ NO PATHING CODE BEYOND THE ONE LINK. ABuilding already roots VisualMesh with
 *     BlockAll + SetCanEverAffectNavigation(true) (Building.h:43-51), so the body
 *     is solid and carves the navmesh like every other building. ⚠️ Descent is
 *     ⛔ NOT free any more: Recast polys are undirected, but a LINK is not — hence
 *     `ENavLinkDirection::BothWays` (`TOWER-§8.7`), which is REQUIRED, ⛔ not
 *     preferred: a one-way ladder makes the deck a dead end with no legal path
 *     off it, i.e. a manufactured stuck unit.
 *
 *  ⛔⛔ ZERO **DAMAGE** COUPLING TO HIGH-§ (HIGH-§3, TOWER-§8 (8)). This class does
 *  ⛔ NOT tell anyone a unit is elevated and does ⛔ NOT call
 *  ASummonedUnit::HeightAdvantageMultiplier. ⚖️ A unit on a HILL and a unit on a
 *  TOWER at the same Z MUST deal identical damage; a tower special case would make
 *  one rule into two and guarantee they eventually disagree. The test file asserts
 *  the parity.
 *  ⚠️ WHAT CHANGED IN 2026-09-01's REDESIGN, STATED PLAINLY BECAUSE IT WAS A REAL
 *  LOSS (`TOWER-§8.4(B)`): the .cpp had to include `SummonedUnit.h` — for
 *  **MOVEMENT**. `TOWER-§`'s founding idea was a tower that never learns a unit
 *  exists, and a ladder ends that, because something must drive a specific body up
 *  a specific line. ⭐ THE ONE COUPLING WAS MOVEMENT AND IT WAS THE ONLY ONE.
 *
 *  ⭐⭐ AND ON 2026-09-02 (TASK-787, `CONTACT-§12`) THAT INCLUDE WAS ⛔ REMOVED AGAIN:
 *  **THIS CLASS NOW NAMES ⛔ ZERO CONCRETE CLIMBER CLASSES.** The movement coupling
 *  is ⛔ not denied — it is NARROWED to one capability interface (`ILadderClimber`:
 *  Begin, Abort, IsClimbing, and the completion delegate's accessor), which admits
 *  `ASummonedUnit` and `AHeroCharacter` on identical terms with ⛔ no branch between
 *  them. ⚖️ *Widening a type until the failure moves is not fixing it; removing the
 *  type from the question is.* The damage seam is, and always was, untouched.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AClimbableTower : public ABuilding
{
	GENERATED_BODY()

public:

	AClimbableTower();

	/**
	 *  The collision channel an ENEMY of TowerTeam carries — always the other
	 *  team's combatant object channel (SiegeNavAreas.h one-home). ⭐ THE SINGLE
	 *  SOURCE OF TRUTH the team gate is expressed in, so the shipped rule and the
	 *  tested claim can never drift apart.
	 *
	 *  ⛔ No new channel is invented (TOWER-§4): ECC_SiegeTeamBlue/ECC_SiegeTeamRed
	 *  are the shipped, proven-in-play channels every combatant capsule is re-typed
	 *  to by team at BeginPlay for the castle's 3× hollow team gating.
	 *
	 *  ⚠️ THE PHYSICAL VOLUME THAT USED TO CONSUME THIS IS GONE (`TOWER-§8.6`) — see
	 *  CanTeamAscend. The channel survives as the VOCABULARY the rule is written
	 *  in, which is the half that was always doing the work.
	 *
	 *  Plain C++ static, ⛔ not a UFUNCTION, no world access, no actor access.
	 */
	static ECollisionChannel AscentBlockedChannel(ETeamId TowerTeam);

	/**
	 *  ⭐⭐ T-3, AS A PURE PREDICATE — AND AS OF TASK-734 THIS IS THE **LIVE RULE**,
	 *  ⛔ no longer a statement about a volume. May a ClimberTeam unit reach this
	 *  tower's platform? ⛔ Enemies NO · ✅ ANY own-team unit YES · ⛔ no capacity
	 *  term and ⛔ no unit-type term in the signature, because there are none in the
	 *  rule. ⛔ THE SIGNATURE AND SEMANTICS ARE FROZEN (`TOWER-§8.6`).
	 *
	 *  ⚖️ THE STORY WORTH KEEPING, BECAUSE IT IS WHY THIS FUNCTION EXISTED WITH NO
	 *  CALLER FOR TWO DAYS: it was written as a STATEMENT of the shipped collision
	 *  matrix so `T-3` was assertable headlessly while the real gate was a physical
	 *  elevation shell. The ladder deleted the shell — the shell's only job was
	 *  stopping an enemy somewhere along a 2,078-uu ramp nobody could locate in
	 *  mesh-local space, and a ladder has ONE discrete entry point. ⭐ So the gate
	 *  collapsed into the predicate that was already here, already QA-passed, and
	 *  already correct. ⛔ The shell's known pivot-vs-mesh tuning item was ⛔ NOT
	 *  fixed — it was DISSOLVED (`TOWER-§8.2`).
	 */
	static bool CanTeamAscend(ETeamId TowerTeam, ETeamId ClimberTeam);

	/** What `HandleLadderLinkReached` decided, in one value — see EvaluateLadderEntry. ⛔ Plain enum, ⛔ not a UENUM: nothing reflects it and nothing edits it. */
	enum class ELadderEntryVerdict : uint8
	{
		/** ✅ Start the traversal. */
		Climb,
		/**
		 *  ⛔ The pawn is not a climber type this gate ADMITS.
		 *
		 *  ⚖️⭐ RENAMED 2026-09-02 (TASK-778, board item (0c) — TASK-777's `D-4`, BOARDED rather
		 *  than left in a handoff). It was `NotASummonedUnit`, which named the ⛔ IMPLEMENTATION of
		 *  the admitted set rather than the ⛔ RULE: the rule is *"this gate admits only climber
		 *  types it can actually drive"*. ⭐⭐ AND THE RENAME PAID FOR ITSELF WITHIN THE DAY: TASK-787
		 *  widened that set from `{ASummonedUnit}` to *"everything implementing `ILadderClimber`"*
		 *  and this enumerator needed ⛔ no edit at all. ⇒ the
		 *  name stays TRUE when the set changes, and a verdict whose name has to be re-read every
		 *  time the set moves is a cite that rots (`CONTACT-§10.1`, applied to an identifier).
		 *  ⛔ The RULE and the PRECEDENCE (identity → team → occupancy) are ⛔ UNCHANGED — this is a
		 *  rename and ⛔ nothing else. ⛔ No second verdict was added beside it.
		 */
		NotAnAdmittedClimber,
		/** ⛔ T-3: an enemy of this tower. */
		WrongTeam,
		/** ⛔ `TOWER-§10` L-1: a climb is already registered on this ladder. */
		LadderBusy
	};

	/**
	 *  ⭐ THE WHOLE ENTRY GATE, AS ONE PURE FUNCTION OF FOUR VALUES — so the rule
	 *  is assertable headlessly with a real truth table, while the shipped
	 *  `HandleLadderLinkReached` is left with nothing but plumbing. (The
	 *  AscentBlockedChannel / CanTeamAscend "one source of truth" discipline, third
	 *  application.)
	 *
	 *  ⛔ THE PRECEDENCE IS PART OF THE CONTRACT, ⛔ not an accident of the ifs:
	 *  IDENTITY → TEAM → OCCUPANCY. An enemy is refused as an ENEMY even when the
	 *  ladder is also busy, because that is the verdict a log line has to be able
	 *  to explain.
	 *
	 *  ⚠️ ClimberTeam is meaningless when bClimberIsAdmittedClimber is false, and the
	 *  identity test short-circuits before it is read.
	 *
	 *  ⚖️⭐ THE IDENTITY PARAMETER WAS RENAMED FROM `bClimberIsSummonedUnit` BY TASK-787
	 *  (`CONTACT-§12`), because it was ⛔ WRONG FOR A HERO the moment the term widened to
	 *  `ILadderClimber`. ⭐ Safe by this file's own precedent — parameter names are ⛔ not
	 *  part of a function's type (`TOWER-§8.4(B)`'s `FromWorld`/`ToWorld` amendment) — and the
	 *  RULE, the PRECEDENCE and the VERDICTS are ⛔ all unchanged. ⛔ The caller now derives it
	 *  from `Cast<ILadderClimber>`; ⛔ the term is WIDENED, ⛔ never removed.
	 *
	 *  ⚠️⚠️ `bLadderOccupied` IS "ANY CLIMB IS REGISTERED", ⛔ NOT "SOMEBODY **ELSE**
	 *  IS CLIMBING", AND THE DIFFERENCE IS A HANGING UNIT. Path following can
	 *  re-enter a link for a unit that is ALREADY on it (`StartUsingCustomLink` is
	 *  called from `SetMoveSegment` on every re-path, and it force-finishes whatever
	 *  link was previous — `PathFollowingComponent.cpp:1454-1463`). Under an
	 *  "…WithAnother" reading that re-entry would be admitted, `BeginLadderClimb`
	 *  would refuse it as *already climbing*, and the refusal handler would then
	 *  UNBIND and CLEAR a climber that is genuinely still in the air — after which
	 *  ⛔ nothing would abort it if the tower died. ⇒ a re-entry is BUSY, the agent is
	 *  handed straight back, and the live climb is left completely untouched.
	 */
	static ELadderEntryVerdict EvaluateLadderEntry(ETeamId TowerTeam, ETeamId ClimberTeam,
		bool bClimberIsAdmittedClimber, bool bLadderOccupied);

	// ═════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ THE CONTACT TRIGGER (TASK-777, CONVENTIONS CONTACT-§4)
	//  "just walking up to it and walking against it" — Jonathan, 2026-09-01
	// ═════════════════════════════════════════════════════════════════════════════
	//
	// ⛔⛔ IT IS ⛔ NOT LITERALLY "CONTACT", AND THAT IS MEASURED RATHER THAN A
	// LIBERTY TAKEN WITH HIS WORDS: **THE LADDER HAS ⛔ ZERO COLLISION** (TASK-737 —
	// no hull anywhere over `LadderFoot`, nearest surface 116 uu). A pawn walking at
	// it passes straight THROUGH and is stopped ~300 uu later by the tower body. ⇒
	// ⛔ an overlap-on-blocking-hit trigger, or anything waiting for the pawn to be
	// STOPPED by the ladder, would ⛔ NEVER fire — ⛔ not by tuning, ⛔ structurally.
	// ⚖️ His sentence describes the player's INTENT exactly and the world's GEOMETRY
	// not at all, and telling those apart is what the law is for.
	//
	// ⭐ AND IT DOES ⛔ NOT REPLACE THE NAV LINK (`K-E`): the LINK is how the AI
	// **plans a route** through the ladder — delete it and the deck is an
	// unreachable island again (`TOWER-§8.1` M-2) — and CONTACT is how a **body
	// starts climbing**. ⛔ They answer different questions and neither substitutes
	// for the other. They also cannot double-fire, and it is asserted rather than
	// promised: both paths run `EvaluateLadderEntry` against the SAME single
	// occupancy slot, and `BeginLadderClimb` refuses a unit that is already
	// climbing.

	/** What `TryBeginContactClimb` decided, in one value. ⛔ Plain enum, ⛔ not a UENUM — the ELadderEntryVerdict precedent: nothing reflects it and nothing edits it. */
	enum class ELadderContactVerdict : uint8
	{
		/** ✅ All three contact terms held AND the entry gate admitted: the traversal is RUNNING and this tower holds the occupancy slot. */
		Climb,
		/** ⛔ Term 1 — outside `LadderContactRadiusUU` of the nearer endpoint (2D). ⭐ Also the state in which this pawn holds ⛔ no contact bookkeeping at all. */
		TooFar,
		/** ⛔ Term 2 — not walking INTO the ladder, or not walking. */
		NotHeadingIn,
		/** ⏳ Terms 1 and 2 hold; `LadderContactDwellSeconds` has not elapsed yet. ⛔ Not a refusal. */
		Dwelling,
		/** ⛔ `K-C`'s re-arm latch — this pawn just finished a climb at this endpoint and has not yet left the radius or the cone. */
		Disarmed,
		/** ⛔ `EvaluateLadderEntry` refused on IDENTITY — the climber does not implement `ILadderClimber`, so this class could neither drive it nor hear when it stopped. ⭐ WIDENED by TASK-787 (`CONTACT-§12`): `AHeroCharacter` is now ADMITTED, and this verdict is what still refuses a future spectator body. ⚖️ Renamed from `NotASummonedUnit` by TASK-778 board item (0c); the rule is ⛔ unchanged. */
		NotAnAdmittedClimber,
		/** ⛔ `EvaluateLadderEntry` refused on TEAM — `T-3`, and there is ⛔ no hero exemption. */
		WrongTeam,
		/** ⛔ `EvaluateLadderEntry` refused on OCCUPANCY — `TOWER-§10` L-1, one climber at a time. */
		LadderBusy,
		/** ⛔ The climber's own `BeginLadderClimb` returned false (dead, frozen, or already climbing) and ⛔ changed nothing. */
		Declined,
		/** ⛔ Self-check: a null climber, or a tower with no ladder link (a CDO). ⛔ Never reachable on a constructed, live tower. */
		NoLadder
	};

	/**
	 *  ⭐⭐ THE WHOLE CONTACT GATE AS ONE **PURE** FUNCTION — the `EvaluateLadderEntry`
	 *  discipline, second application: the three `CONTACT-§4.1` terms and the shipped
	 *  `TOWER-§8.6`/`L-1` entry gate composed in ONE place, so the COMPOSITION is
	 *  assertable headlessly with a real truth table while `TryBeginContactClimb` is
	 *  left with nothing but plumbing.
	 *
	 *  ⛔⛔ THE ORDER IS PART OF THE CONTRACT AND IT IS ⛔ NOT THE OBVIOUS ONE: THE
	 *  THREE CONTACT TERMS RUN **FIRST**, THE ENTRY GATE SECOND. Putting the cheap
	 *  team/occupancy refusals first would look tidier and would introduce a real,
	 *  silent defect — `K-C`'s latch is maintained by `WantsToClimb`, so a pawn
	 *  latched at the foot while SOMEBODY ELSE holds the ladder would be short-
	 *  circuited out on `LadderBusy` every frame, never observed leaving the radius,
	 *  and would carry that latch for the rest of the match. ⇒ ⛔ a pawn permanently
	 *  unable to climb, for a reason nothing logs.
	 *
	 *  ⚠️⚠️ AND THE TEAM GATE IS ⛔ NOT WEAKENED BY RUNNING SECOND (`CONTACT-§4.3`):
	 *  `Climb` is returned from EXACTLY ONE place, and only after `EvaluateLadderEntry`
	 *  returned `Climb` — which is `CanTeamAscend`. ⭐ `CanTeamAscend` gains a SECOND
	 *  CALLER here, ⛔ never an exception. A contact path that skipped it would be a
	 *  ⛔ SILENT BACK DOOR around a Jonathan ruling, opened by a task whose stated
	 *  purpose was something else, and ⛔ no reviewer would have a reason to look —
	 *  which is precisely why the test file asserts an enemy satisfying all three
	 *  contact terms is still refused.
	 */
	static ELadderContactVerdict EvaluateContactEntry(
		FSiegeLadderContactState& ContactState,
		const FVector& ClimberLocation, const FVector& ClimberVelocity,
		const FVector& FootWorld, const FVector& TopWorld,
		float RadiusUU, float IntentCos, float RequiredDwellSeconds, float DeltaSeconds,
		ETeamId TowerTeam, ETeamId ClimberTeam, bool bClimberIsAdmittedClimber, bool bLadderOccupied,
		bool& bOutAscending);

	/**
	 *  ⭐⭐ THE CONTACT ENTRY POINT — **THE PAWN ASKS**, once per frame, out of the tick
	 *  it already pays for. On `Climb` the traversal is ALREADY RUNNING and this tower
	 *  ALREADY holds the occupancy slot; every other verdict changed ⛔ nothing except
	 *  this pawn's own dwell bookkeeping.
	 *
	 *  ⭐ `WR-§5`'s idiom, third application, and it is MEASURED: `ACommanderNpc` owns
	 *  `InteractRadius` and `IsPlayerInRange`, ships `bCanEverTick = false`, and is
	 *  POLLED BY ITS CALLER (`SiegePlayerController.cpp:4970`). ⇒ the radius, the cone,
	 *  the dwell and the latch all live HERE — ⛔ never a second copy on the pawn —
	 *  and this class keeps its no-tick, no-timer property (see the class note).
	 *
	 *  ⛔ NO PER-FRAME LOGGING, DELIBERATELY. This is called from a pawn's tick, so a
	 *  Verbose line per refusal would be one per pawn per frame forever. Only the two
	 *  EVENTS log: an admission, and a `BeginLadderClimb` that declined.
	 *
	 *  ⚖️⭐⭐ THE SEAM THIS FUNCTION USED TO REFUSE IS **OPEN** (TASK-787, `CONTACT-§12`), AND
	 *  THE OLD NOTE IS REPAIRED RATHER THAN DELETED BECAUSE IT IS WHAT BOUGHT THE FIX. It
	 *  read: *"it STARTS the traversal by calling `ASummonedUnit::BeginLadderClimb`, because
	 *  that is the only start API the tower can reach … a climber that is ⛔ not an
	 *  `ASummonedUnit` is refused here with the exact verdict `NotAnAdmittedClimber`."* ⭐ That
	 *  refusal was CORRECT while it stood — a slot claimed for a climber that then failed to
	 *  start would BRICK the ladder — and `CONTACT-§12` closed it the only way it may be
	 *  closed: ⛔ BOTH seams at once. `ILadderClimber` now carries `BeginLadderClimb` AND
	 *  `GetOnLadderClimbEnded()`, so this function can START ⛔ any admitted climber and is
	 *  guaranteed to LEARN when that climb ends.
	 *
	 *  ⛔⛔ AND THE REFUSAL IS ⛔ NOT GONE, ONLY NARROWED: an `ACharacter` that implements
	 *  ⛔ nothing — a future spectator body — still gets `NotAnAdmittedClimber`, and still
	 *  gets it BY IDENTITY, ⛔ before team and ⛔ before occupancy.
	 *
	 *  @param DeltaSeconds  the caller's own frame delta. ⛔ This class reads no clock.
	 */
	ELadderContactVerdict TryBeginContactClimb(ACharacter* Climber, float DeltaSeconds);

	/**
	 *  `TOWER-§8.6`'s OPTIONAL second layer, as a pure static so it is testable
	 *  without a navmesh: may `Querier` PATH through a TowerTeam ladder?
	 *
	 *  ⭐ FAILS **OPEN** BY CONSTRUCTION — a null Querier, a Querier that is not an
	 *  AController, a controller with no pawn and a pawn that is not an ITeamAgent
	 *  ALL return true. ⚠️ That is the entire safety argument for shipping a layer
	 *  the law refused to vouch for: the worst case of a wrong reading of `Querier`
	 *  is the behaviour of not having shipped it, ⛔ never an own-team unit that
	 *  cannot path to its own tower. The measurement itself is cited in full on
	 *  UClimbableTowerLadderLink above.
	 */
	static bool ShouldLinkAllowPathfinding(const UObject* Querier, ETeamId TowerTeam);

	/** Platform height above this tower's origin, in uu — the number the mesh is built to and the number HIGH-§ turns into damage. See PlatformHeightUU. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Tower")
	float GetPlatformHeightUU() const { return PlatformHeightUU; }

	/** The ladder's smart link. ⛔ Never null on a constructed tower (a CDO subobject). */
	const UClimbableTowerLadderLink* GetLadderLink() const { return LadderLink; }

	/**
	 *  ⭐ THE ARTIST↔PROGRAMMER SEAM, BY NAME (CONVENTIONS "Static-mesh SOCKET
	 *  names", `TOWER-§8.4(A)`). TASK-737 authors these two sockets on
	 *  /Game/Meshes/SM_WatchTower at the `TOWER-§8.3` coordinates; this class reads
	 *  them at BeginPlay and ⛔ never hardcodes the geometry (`TOWER-§7`'s "the size
	 *  comes from the MESH, ⛔ never from a literal", applied a second time).
	 */
	static const FName LadderFootSocketName;
	static const FName LadderTopSocketName;

	/**
	 *  ⛔⛔ THE `TOWER-§8.3` PINNED GEOMETRY, IN THE TOWER'S OWN ACTOR SPACE — used
	 *  ONLY when the mesh does not ship the socket. ⭐ THIS FALLBACK IS WHAT MAKES
	 *  THE ART LANE AND THE CODE LANE GENUINELY PARALLEL: this file is correct
	 *  BEFORE SM_WatchTower is re-authored, and self-corrects the moment it lands.
	 *
	 *  ⚠️⚠️ AND NEITHER NUMBER IS A STYLE CHOICE — BOTH ARE NAVMESH ARITHMETIC, AND
	 *  MOVING EITHER "A BIT" SEVERS THE FEATURE SILENTLY:
	 *    • Foot (−460, 0, 0): the body carves its 600×600 footprint and
	 *      `rcErodeWalkableArea` takes 2 more cells (64 uu), so ground navmesh
	 *      starts at X ≤ −364. −460 clears it by **96 uu**.
	 *    • Top (−160, 0, 1200): a 600 uu deck loses 64 uu PER SIDE to ledge-nulling
	 *      plus erosion, so the surviving deck poly is X ∈ [−236, +236]. −160 is
	 *      **76 uu inside it**.
	 *
	 *  ⚠️ THE TWO CLEARANCES WERE `86` / `86` UNTIL TASK-783 TRANSLATED BOTH SOCKETS BY
	 *  `(−10, 0, 0)`, AND THIS BLOCK WENT ON QUOTING THE PRE-TRANSLATION PAIR — repaired
	 *  by TASK-786. ⭐ The numbers above are TASK-783's MEASUREMENTS, ⛔ not a retune, and
	 *  they re-derive from the shipped literals below: |−460 − (−364)| = 96 at the foot,
	 *  |−160 − (−236)| = 76 at the near deck edge. ⭐ The translation is PURE, so `Δ`
	 *  stays (300, 0, 1200) and the line's length (1,236.9 uu) and lean (76.0°) cannot
	 *  notice it — only these two clearances moved, in OPPOSITE directions: the foot's
	 *  IMPROVED 86 → 96, the top's SHRANK 86 → 76.
	 *  ⚠️⚠️ `SC-§36` PROSE-DEBT, NAMED: this comment sat on the very declaration whose
	 *  values it misdescribed, and ⛔ nothing failed — a derivation that has drifted off
	 *  the code it derives is invisible to every gate in the project. `ClimbableTower.cpp`
	 *  (the banner above these two literals) already carried the corrected pair; this
	 *  header was the last stale copy.
	 *  ⚠️ A socket on the deck EDGE is the castle-floor defect class: every property
	 *  readback correct, and nothing can use it.
	 *
	 *  ⛔ NOT named "…Fallback…": the shipped test scans this class's member names
	 *  for the token "Fall" (the refused occupant-catch subsystem), and a member
	 *  called LadderFootFallback would trip it. ⚖️ A naming collision between a
	 *  refusal probe and an unrelated feature is a real cost of scan-based tests
	 *  and is paid here, once, in the name.
	 */
	static const FVector LadderFootDefaultRelative;
	static const FVector LadderTopDefaultRelative;

protected:

	/** ABuilding's BeginPlay (team material, spawn squash, DT_Cards stat bind) then arms the ladder link, once Team is authoritative. ⛔ No timer of any kind is started here. */
	virtual void BeginPlay() override;

	/**
	 *  ⚠️⚠️ `TOWER-§10` L-5 — THE ONE PLACE THIS REDESIGN GENUINELY ADDS RISK, AND
	 *  THE ONLY REASON THIS OVERRIDE EXISTS. Under the ramp, a tower dying beneath
	 *  an occupant was FREE: the floor unregistered and CharacterMovement dropped to
	 *  MOVE_Falling by itself. ⛔ A `MOVE_Flying` climber will NOT fall — it will
	 *  HANG IN THE AIR PERMANENTLY. ⇒ this aborts the climb THIS tower started,
	 *  through the CLIMBER's own idempotent `ILadderClimber::AbortLadderClimb`, which
	 *  restores the movement mode and broadcasts exactly once. ⭐ Since TASK-787 that is
	 *  true of the HERO too — which is exit `H-10`, i.e. the PLAYER'S OWN BODY.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  ⭐ THE HEIGHT OF THE PLATFORM, AND WHAT IT BUYS IN HIGH-§ TERMS — written
	 *  here so nobody retunes a geometry number without seeing its gameplay
	 *  consequence:
	 *
	 *      1,200 uu of height ADVANTAGE over the target
	 *        = 1,200 / 152.4 = 7.874 steps × +10%
	 *        ⇒ ⭐ ×1.787 (+78.7%) firing at a target on the flat ground below
	 *        ⇒ ⭐ ×2.44 from a tower standing on a ~1,000-uu hill, firing into a valley
	 *
	 *  ⚠️ THIS IS ALSO THE MESH'S CONTRACT. SM_WatchTower is built to exactly this
	 *  rise, and `LadderTopDefaultRelative`'s Z IS this number. ⛔ Changing this
	 *  value does NOT move the mesh and does NOT move the socket — it only
	 *  de-synchronises the three. A retune must land WITH a re-authored mesh.
	 *  ⚠️ The 30°/2,078-uu ramp budget this comment used to quote is MOOT for this
	 *  mesh (`TOWER-§8.2`) — ⛔ the rest of `TOWER-§2a` is re-scoped, ⛔ not struck,
	 *  and still binds every other slope in the project.
	 *
	 *  EditDefaultsOnly because 🧑 row T-5 is Jonathan's: his next sentence
	 *  retunes the height without a code change. ⛔ Never a cards.csv column —
	 *  this is structure geometry, not a card stat.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Tower", meta = (ClampMin = "0"))
	float PlatformHeightUU = 1200.f;

	/**
	 *  ⭐⭐ THE OFF-MESH CONNECTION WITHOUT WHICH THE DECK IS AN UNREACHABLE ISLAND
	 *  AND THE 30-GOLD CARD DOES NOTHING (`TOWER-§8.1` M-1/M-2).
	 *
	 *  ⛔ It is a **SMART** link, ⛔ never a simple `PointLinks` entry, and that is
	 *  MEASURED, ⛔ not preferred: `UPathFollowingComponent::SetMoveSegment` calls
	 *  `StartUsingCustomLink` ONLY when `PathPt0.CustomNavLinkId != FNavLinkId::Invalid`
	 *  (PathFollowingComponent.cpp:959-963). A simple link carries no id, so it
	 *  yields ordinary steering — which `ConstrainInputAcceleration` then flattens
	 *  to zero vertical every frame (M-4). A simple link would path-plan
	 *  beautifully and move nobody.
	 *
	 *  ⛔⛔ AND IT IS `ENavLinkDirection::BothWays`, WHICH IS REQUIRED, ⛔ NOT
	 *  PREFERRED (`TOWER-§8.7`): under the ramp descent was free because Recast
	 *  polys are UNDIRECTED, and a link does ⛔ not inherit that. A one-way ladder
	 *  leaves the deck a dead end with ⛔ no legal path off it, so a unit ordered
	 *  down never starts and stands there permanently — a manufactured stuck unit,
	 *  the exact NAV-§ class this namespace exists to avoid.
	 *
	 *  ⚠️ IT IS A `UActorComponent`, ⛔ NOT A `USceneComponent` — `UNavRelevantComponent`
	 *  derives from UActorComponent, so there is ⛔ nothing to attach it to and ⛔ no
	 *  relative transform of its own. Its two endpoints are OWNER-relative and the
	 *  owner's transform places them (`NavLinkCustomComponent.cpp:518-526`).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Tower|Ladder")
	TObjectPtr<UClimbableTowerLadderLink> LadderLink;

	// ═════════════════════════════════════════════════════════════════════════════
	//  THE CONTACT TRIGGER'S THREE FEEL NUMBERS (CONTACT-§4.1, CONTACT-§7 `K-5`)
	// ═════════════════════════════════════════════════════════════════════════════
	//
	// ⚠️⚠️ ALL THREE ARE **DECLARED INVENTED NUMBERS**. They were chosen by the
	// manager, ⛔ not measured off anything in the world, and `K-5` records them as
	// Jonathan's to overrule in one word. ⭐ `EditDefaultsOnly` for exactly that
	// reason: his next sentence retunes the feel with ⛔ no code change.
	//
	// ⭐ AND THE RADIUS AND THE TEST BOTH LIVE HERE, ⛔ never a second copy on the
	// pawn (`WR-§5`, third application — `ACommanderNpc::IsPlayerInRange` reading
	// `InteractRadius`). ⚖️ Two copies of a tuning number is how they drift.

	/**
	 *  How near an endpoint a pawn must be, in **2D (XY)**, before walking into the
	 *  ladder can start a climb.
	 *
	 *  ⭐ THE CONSEQUENCE, WRITTEN BESIDE THE NUMBER (`HIGH-§1`): this is the size of
	 *  the window in which the dwell has to be earned, so it trades directly against
	 *  `LadderContactDwellSeconds`. At 350 uu and a 300 uu/s walk, a pawn aimed at
	 *  the ladder crosses ~105 uu of approach during the 0.35 s dwell and starts
	 *  climbing about **245 uu short of the foot** — i.e. long before it can walk
	 *  through the (collisionless) ladder and reach the tower body.
	 *
	 *  ⚠️⚠️ RAISED 150 → 300 BY `CONTACT-§7` `K-6` (TASK-784), THEN 300 → **350** BY
	 *  TASK-786 — ⛔ NOT a feel tweak either time. At 150 the trigger was UNREACHABLE
	 *  for most of the game; at 300 it was still unreachable for the FASTEST HERO.
	 *
	 *  ⭐⭐ AND THE TWO CLASSES NEED ⛔ DIFFERENT ARITHMETIC — CONFLATING THEM IS WHAT
	 *  MADE 300 LOOK SUFFICIENT:
	 *    • UNITS poll at `StateCheckInterval` = 0.25 s, and each poll credits a WHOLE
	 *      0.25 s of dwell for one sampled instant ⇒ the bar is "TWO samples inside the
	 *      window", i.e. `R/v >= 2 × 0.25` ⇒ R >= 2 × 0.25 × 600 (Cavalry) = **300**.
	 *      ⚠️ It is PROBABILISTIC: a pawn gets two samples on only
	 *      `clamp((R/v − P)/P, 0, 1)` of the sampling phases.
	 *    • THE HERO polls PER FRAME in `Tick`, so dwell accrues `DeltaSeconds` and the
	 *      model is ⭐ EXACT, ⛔ not probabilistic: it needs `R/v >= 0.35` outright
	 *      ⇒ R >= 0.35 × 937.5 = **328.125**.
	 *  ⇒ ⭐⭐ THE **HERO** IS THE BINDING CONSTRAINT, ⛔ NOT THE ROSTER. 350 clears
	 *  328.125 by **6.67%**.
	 *
	 *  ⛔ THE HERO'S FOUR SPEEDS (`WalkSpeed` 500 · `SprintSpeed` 750 · each ×1.25 with
	 *  Swift Boots), dead-on `R/v` against the 0.35 s dwell:
	 *      speed          at R = 300      at R = 350
	 *      500  walk        0.600 ✅        0.700 ✅  (×2.00)
	 *      625  walk+Boots  0.480 ✅        0.560 ✅  (×1.60)
	 *      750  sprint      0.400 ✅        0.467 ✅  (×1.33)
	 *      937.5 sprint+Boots 0.320 ⛔      0.373 ✅  (×1.07)
	 *  ⚠️⚠️ THE LAST ROW IS THE WHOLE REASON FOR 350: a player SPRINTING WITH SWIFT
	 *  BOOTS at a ladder is ⛔ not an edge case — it is the most likely way this feature
	 *  gets tested first, and at 300 it fails at EVERY offset and every frame rate.
	 *  ⚠️ Its margin is real but thin: 0.373 s holds on every phase down to ~20 fps and
	 *  ⛔ fails at 15. Units are unaffected — all six roster speeds are 16/16 at both radii.
	 *
	 *  ⚠️⚠️ AND THE COST IS ⛔ NOT THE ~±125 uu FIRST ESTIMATED, ⛔ NOR A LINEAR SCALE OF
	 *  IT — IT IS MEASURED (Tests/SiegeLadderClimbTest.cpp test 16(c) drives the shipped
	 *  predicate rather than a closed form). The abduction half-window at 300 uu/s:
	 *      R = 150 → ±58    R = 300 → ±202    R = 350 → **±247**
	 *  ⇒ **3.5× for the first doubling, then +22% for a +17% radius rise** — it grows
	 *  FASTER than the radius throughout, because the dwell only ever eats a fixed
	 *  `v × 0.35 s` of approach, which is a smaller fraction of a bigger disc. ⛔ A
	 *  linear scale from ±204 would have guessed ±238 and understated it by ~9 uu.
	 *  ⇒ ⭐ a pawn marching past its own tower is now grabbed from up to ~247 uu to the
	 *  side. 🧑 That trade is Jonathan's (`K-5`/`K-6`).
	 *  ⭐ IF HE WANTS IT NARROWER THERE IS A **FREE** LEVER: the dwell can rise
	 *  0.35 → 0.50 s at ⛔ ZERO cost in radius (both sit in the same 2-poll band for
	 *  units, and 0.50 × 937.5 = 468.75 would need a bigger radius for the hero — so the
	 *  free band is the UNIT side only). ⛔ Raise the radius again and the dwell must rise
	 *  with it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Tower|Ladder", meta = (ClampMin = "0"))
	float LadderContactRadiusUU = 350.f;

	/**
	 *  The cosine of the intent HALF-cone: the pawn's horizontal movement direction
	 *  must agree with the horizontal direction to the endpoint by at least this
	 *  much. **0.5 = a 60° half-cone.**
	 *
	 *  ⭐ THE CONSEQUENCE: this is "walking toward it" versus "brushing past it". It
	 *  reads DIRECTION and ⛔ never a key — the only shape that behaves identically
	 *  for a player-driven pawn and an AI-driven one (and `RECALL-§2` / `KBD-§` would
	 *  forbid a hardcoded key anyway).
	 *  ⚠️ Lowering it toward 0 widens the cone to a full hemisphere and the trigger
	 *  starts catching pawns walking ACROSS the ladder; raising it toward 1 demands a
	 *  dead-straight approach and makes the ladder feel unresponsive at an angle.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Tower|Ladder", meta = (ClampMin = "-1", ClampMax = "1"))
	float LadderContactIntentCos = 0.5f;

	/**
	 *  How long the other two terms must hold CONTINUOUSLY before the climb starts.
	 *
	 *  ⭐⭐ THE CONSEQUENCE, AND IT IS THE LOAD-BEARING ONE — THIS NUMBER IS THE WHOLE
	 *  ANSWER TO ACCIDENTAL ABDUCTION, AND IT MATTERS MOST FOR **UNITS**: without it a
	 *  friendly unit marching past its own tower toward the enemy castle clips the
	 *  intent cone for two frames and is yanked 1,200 uu into the air. ⭐ A pawn
	 *  merely passing through leaves the cone almost immediately (the bearing to the
	 *  endpoint swings as it passes); a pawn deliberately pressing into the ladder
	 *  holds it trivially.
	 *  ⚠️ Lower it and passers-by start getting grabbed; raise it and the ladder feels
	 *  sticky to start. 🧑 `K-5` — the feel is Jonathan's.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Tower|Ladder", meta = (ClampMin = "0"))
	float LadderContactDwellSeconds = 0.35f;

private:

	/**
	 *  Reads the ladder line off the mesh and arms the link, once Team is
	 *  authoritative. ⛔ DEGRADES OPEN in every direction (`TOWER-§8.4(A)`): a
	 *  missing mesh, a missing socket or a degenerate line all fall back to the
	 *  pinned `TOWER-§8.3` literals with ONE warning naming what was missing —
	 *  ⛔ never a broken tower, and ⛔ never a zero-length off-mesh connection.
	 */
	void ConfigureLadderLink();

	/** One socket, in ACTOR space, or DefaultRelative if the mesh cannot supply it. `RTS_Actor` divides the owner transform back out, so a spawn-squash scale wobble can never leak into the stored link data. */
	static FVector ResolveLadderSocketRelative(const UStaticMeshComponent* Mesh, FName SocketName,
		const FVector& DefaultRelative, bool& bOutUsedDefault);

	/**
	 *  ⭐ THE ENTRY POINT — bound to the link's `FOnMoveReachedLink`, so it fires
	 *  exactly when path following hands an agent to this link and ⛔ never on a
	 *  poll. Applies EvaluateLadderEntry and either starts the traversal through
	 *  `ILadderClimber::BeginLadderClimb` (TASK-787 — ⛔ two entry paths, ⛔ ONE rule and
	 *  ⛔ one seam) or hands the agent straight back.
	 *
	 *  ⚠️⚠️ AND THE REFUSAL PATH IS ⛔ NOT OPTIONAL, WHICH IS A MEASURED TRAP:
	 *  `UNavLinkCustomComponent::OnLinkMoveStarted` returns TRUE — i.e. "this link
	 *  is driving the agent now" — for the sole reason that this delegate is BOUND
	 *  (NavLinkCustomComponent.cpp:198-208). It does ⛔ not ask whether we accepted.
	 *  ⇒ **every refusal must immediately resume path following**, or the refused
	 *  unit sits in custom-link limbo forever: a manufactured stuck unit created by
	 *  the very gate meant to protect the tower.
	 */
	void HandleLadderLinkReached(UNavLinkCustomComponent* LinkComp, UObject* PathComp, const FVector& DestPoint);

	/**
	 *  ⭐⭐ THE COMPLETION SIGNAL, AND THE REASON THIS CLASS STILL HAS ⛔ NO TICK AND
	 *  ⛔ NO TIMER: a CLIMBER broadcasts `OnLadderClimbEnded` EXACTLY ONCE per successful
	 *  `BeginLadderClimb`, on every one of its exits — the unit's eight (arrival, abort,
	 *  a new order, death, FreezeAI, ApplyFreeze, EndPlay, and this tower's own EndPlay)
	 *  and the hero's TEN, all of which route through its ONE teardown. Whatever ended
	 *  it, the tower's job is identical: unbind, hand the agent back to path following,
	 *  forget it.
	 *
	 *  ⭐ REACHED THROUGH `ILadderClimber::GetOnLadderClimbEnded()` SINCE TASK-787
	 *  (`CONTACT-§12.3`) — ⛔ never through a member of a concrete climber class, which is
	 *  what let a hero be admitted without ever being able to release the slot.
	 *
	 *  ⛔ `bReachedTop` deliberately drives NOTHING here. An arrival and an abort
	 *  differ for the UNIT (where it ends up) and ⛔ not for the TOWER (the ladder
	 *  is free either way). Branching on it would be the first line of a
	 *  bookkeeping subsystem `TOWER-§4` refuses.
	 *
	 *  ⚠️ WIDENED TO `ACharacter*` BY `CONTACT-§4.4` (`TOWER-§8.4(B)`'s second
	 *  amendment) — ⛔ the parameter TYPE only. The delegate TYPE NAME
	 *  `FSiegeLadderClimbEnded` is UNCHANGED, the arity is UNCHANGED (2), and both
	 *  climbers' `Broadcast(this, bReachedTop)` compiles UNTOUCHED through the implicit
	 *  conversion to `ACharacter*`. ⭐ TASK-787 MOVED the delegate's DECLARATION from
	 *  `SummonedUnit.h` to `LadderClimber.h` — ⛔ a move, ⛔ not an amendment: ⛔ nothing
	 *  about this handler's shape changed, which is why ⛔ ONE type serves both pawns and a
	 *  hero-only second delegate was REFUSED (`CONTACT-§12.4`).
	 *
	 *  ⭐ AND IT IS ALSO WHERE `K-C`'S RE-ARM LATCH IS SET, WHICH IS WHY THE WIDENING
	 *  HAD TO REACH HERE: this is the ONE point every climb ends at, whichever path
	 *  started it, so latching from here means a nav-link ascent and a contact ascent
	 *  are protected from the yo-yo by the SAME line of code.
	 */
	UFUNCTION()
	void HandleLadderClimbEnded(ACharacter* Climber, bool bReachedTop);

	/** Unbinds Climber (through `ILadderClimber::GetOnLadderClimbEnded()`), hands it back to ordinary path following, and clears the ladder if Climber held it. IDEMPOTENT — EndPlay calls it after an abort that already called it, and `RemoveDynamic` on an unbound delegate is a no-op. ⛔ The release is NEVER conditional on the binding: there is deliberately ⛔ no "was it bound?" flag (`CONTACT-§12.6`). */
	void ReleaseClimber(ACharacter* Climber);

	/**
	 *  ⭐ THE OCCUPANCY TERM BOTH ENTRY PATHS FEED TO `EvaluateLadderEntry`, spelled ⛔ ONCE
	 *  (TASK-787, `CONTACT-§12.5`): the slot is held AND the climber holding it is STILL
	 *  climbing, read through `ILadderClimber::IsClimbing()`.
	 *
	 *  ⛔⛔ IT IS A **BELT**, ⛔ NOT THE MECHANISM. The ladder is freed EAGERLY by the completion
	 *  delegate (`HandleLadderClimbEnded` → `ReleaseClimber`); this term exists so that a
	 *  completion signal that is ever MISSED degrades the worst case from *"the tower is dead for
	 *  the rest of the match"* to *"one admission is late by one poll"*. ⚠️ It does ⛔ NOT excuse a
	 *  climber that never broadcasts, and a future climber class that relies on it instead of
	 *  broadcasting would give the two pawn classes DIVERGENT release semantics — which is exactly
	 *  what `CONTACT-§12.5`'s closing clause forbids.
	 *
	 *  ⚠️ THE `bLadderOccupied` CONTRACT IS ⛔ UNCHANGED: it is still "ANY registered climb",
	 *  ⛔ never "somebody ELSE is climbing" — there is ⛔ NO identity comparison in here, and adding
	 *  one would re-open the re-entry defect the `ActiveClimber` note below describes.
	 *  ⚠️ A held climber this class cannot interrogate reads OCCUPIED (the conservative direction,
	 *  which preserves `TOWER-§10` L-1).
	 */
	bool IsLadderSlotOccupied() const;

	/**
	 *  `ANavLinkProxy::ResumePathFollowing`'s idiom, verbatim
	 *  (NavLinkProxy.cpp:344-363): find the agent's UPathFollowingComponent — on the
	 *  pawn or on its controller — and call FinishUsingCustomLink. ⭐ Prefers the
	 *  EXACT component the link handed us (it is the one holding
	 *  `CurrentCustomLinkOb`) and re-derives only if that has gone.
	 *
	 *  ⛔ THIS FUNCTION NEVER TOUCHES A MOVEMENT MODE. `FinishUsingCustomLink` is a
	 *  path-following handshake and nothing else — see the class note on why that
	 *  matters.
	 */
	void ResumeAgentPathFollowing(AActor* Agent, UObject* KnownPathComp) const;

	/**
	 *  ⭐ `TOWER-§10` L-1 — the ONE unit currently on the ladder, and ⛔ NOT a
	 *  capacity counter (see the class note). Two jobs, both required:
	 *    (a) one climber at a time — a second unit waits at the foot in its
	 *        ordinary walking state and ⛔ never latches idle (NAV-§); and
	 *    (b) L-5 — EndPlay needs to know WHO to abort, or a `MOVE_Flying` climber
	 *        hangs in the air when the tower dies.
	 *
	 *  ⚠️⚠️ IT IS DELIBERATELY ⛔ NOT `UNavLinkCustomComponent::HasMovingAgents()`,
	 *  AND THAT IS MEASURED RATHER THAN ASSUMED: `OnLinkMoveStarted` ADDS the agent
	 *  to `MovingAgents` BEFORE it executes this delegate
	 *  (NavLinkCustomComponent.cpp:198-200), so inside `HandleLadderLinkReached`
	 *  `HasMovingAgents()` is ALWAYS true — including for the very unit being
	 *  admitted. Using it as the busy test would refuse every climber, forever, and
	 *  the tower would look like it simply did not work.
	 *
	 *  ⭐ WEAK ON PURPOSE, WHICH IS ALSO THE DEFENSIVE CLEAR `TOWER-§8` (6) ASKS FOR:
	 *  a unit destroyed without reaching any completion path leaves a stale handle
	 *  that evaporates on its own, so the ladder can never be permanently bricked by
	 *  a death this class did not observe.
	 *
	 *  ⛔⛔ WIDENED FROM `ASummonedUnit` TO `ACharacter` BY `CONTACT-§4.4`, AND IT
	 *  REPAIRS A ⛔ MEASURED DEFECT RATHER THAN A SPECULATIVE ONE: as shipped, a HERO
	 *  climber could ⛔ never occupy this slot at all, which broke BOTH of the two
	 *  jobs above — L-1 could not see it (so a unit could be admitted onto the same
	 *  line, and the note at `ClimbableTower.cpp`'s occupancy branch records why that
	 *  is ⛔ not cosmetic: two capsules on one line WILL interpenetrate and
	 *  depenetration shoves one OFF the line, in mid-air), and `EndPlay` could not
	 *  reach it (so the tower falls and the hero HANGS IN `MOVE_Flying` FOREVER).
	 *
	 *  ⭐ `ACharacter` and ⛔ NOT `APawn`, ⛔ not `AActor`, and ⛔ NOT a second
	 *  parallel hero-only slot (⚖️ *two slots is two L-1 rules, and the second one is
	 *  the one nobody tests*). `ACharacter` is `ASummonedUnit`'s and
	 *  `AHeroCharacter`'s nearest common ancestor AND the type that CARRIES the API
	 *  the feature needs — `GetCharacterMovement()` for `MOVE_Flying` and
	 *  `GetCapsuleComponent()` for the half-height lift are ⛔ both `ACharacter`
	 *  members and ⛔ neither exists on `APawn`, so a bare-`APawn` climber is
	 *  IMPOSSIBLE and every consumer would `Cast<ACharacter>` anyway — re-creating
	 *  the exact failed-cast class that CAUSED this defect.
	 *  ⭐ FREE PROPERTY, NAMED SO IT IS NOT LOST: `ASiegeGhostPawn : APawn` is ⛔ not
	 *  an `ACharacter`, so "a ghost can never climb" is now true **BY TYPE**.
	 *
	 *  ⭐⭐ AND SINCE TASK-787 THE SLOT IS ⛔ NOT ONLY OCCUPIABLE BY A HERO BUT **RELEASABLE**
	 *  BY ONE (`CONTACT-§12`), which is the half `CONTACT-§4.4` could not finish: the type was
	 *  widened here, but the tower still learned that a climb ended through a delegate only
	 *  `ASummonedUnit` had. ⇒ a hero could be admitted and ⛔ never released — ⛔ WORSE than
	 *  being refused, because it would have bricked this ladder for EVERY later climber, hero
	 *  and unit alike, for the rest of the match. ⛔ Both halves ship together or neither does.
	 */
	TWeakObjectPtr<ACharacter> ActiveClimber;

	/** The exact UPathFollowingComponent the link handed us for ActiveClimber — the one holding CurrentCustomLinkOb, so the handshake is closed against the right object even if the unit changed controller mid-climb. ⚠️ A CONTACT-started climb has no such handshake and leaves this null, deliberately. */
	TWeakObjectPtr<UObject> ActiveClimberPathComp;

	/**
	 *  One entry per pawn currently AT this ladder — its dwell clock and its `K-C`
	 *  latch (`CONTACT-§4.1`).
	 *
	 *  ⭐⭐ THE ENTRY'S EXISTENCE **IS** "THIS PAWN IS AT THIS LADDER", AND THAT ONE
	 *  decision does three jobs at once: `TryBeginContactClimb` drops the entry the
	 *  moment the pawn leaves the radius, which (a) re-arms `K-C`'s latch — leaving
	 *  the radius is one of its two re-arm conditions — (b) clears the dwell, and
	 *  (c) prunes the table. ⇒ the array is bounded by the pawns actually standing at
	 *  the ladder, which the `TOWER-§4` note above sizes at a handful.
	 *
	 *  ⚠️ WEAK HANDLES, PRUNED ON EVERY LOOKUP, for the same reason `ActiveClimber`
	 *  is weak: a pawn destroyed without ever leaving the radius must ⛔ not leave a
	 *  row behind, and a table that only ever grows is a leak nobody notices in a
	 *  five-minute match.
	 *
	 *  ⛔ NOT A UPROPERTY and ⛔ not reflected — the `ActiveClimber` precedent: it is
	 *  derived, per-frame, local state that is ⛔ never authored, ⛔ never serialized
	 *  and ⛔ never edited. ⛔ M8 (`CONTACT-§9`): nothing here replicates and there is
	 *  ⛔ no RPC — the trigger is evaluated where the pawn is driven, and an unreflected
	 *  struct cannot be replicated by accident even after a later refactor.
	 */
	struct FLadderContactEntry
	{
		TWeakObjectPtr<ACharacter> Climber;
		FSiegeLadderContactState State;
	};
	TArray<FLadderContactEntry> LadderContacts;

	/**
	 *  This pawn's contact row, created on first use. ⚠️ The returned reference is
	 *  invalidated by the NEXT call (the array may reallocate) — every caller uses it
	 *  and drops it inside one statement, which is checked by inspection because the
	 *  alternative is handing out an index nobody remembers to re-validate.
	 *
	 *  ⚠️ Takes a NON-const `ACharacter*` on purpose: it STORES the pointer in a
	 *  `TWeakObjectPtr<ACharacter>`, and a `const ACharacter*` is not convertible to
	 *  the handle's type. `ForgetContact` below keeps its `const` — it only compares.
	 */
	FSiegeLadderContactState& FindOrAddContactState(ACharacter* Climber);

	/** Drops this pawn's contact row if it has one. ⭐ Leaving the radius, re-arming the K-C latch and pruning the table are ONE operation — see LadderContacts. */
	void ForgetContact(const ACharacter* Climber);
};
