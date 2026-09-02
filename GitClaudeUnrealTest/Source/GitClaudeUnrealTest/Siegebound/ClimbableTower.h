// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/Navigation/NavLinkDefinition.h"
#include "Engine/EngineTypes.h"
#include "NavLinkCustomComponent.h"
#include "Siegebound/Building.h"
#include "Siegebound/TeamId.h"
#include "ClimbableTower.generated.h"

class ASummonedUnit;
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
 *        traversal must be DRIVEN. `ASummonedUnit` drives it (TASK-738); this
 *        class only decides WHO, WHEN and BETWEEN WHICH TWO POINTS.
 *
 *  ⭐⭐ AND THE PROPERTY THAT SURVIVED THE REDESIGN INTACT, WHICH IS WHY THE
 *  DELEGATE MATTERS: **THIS CLASS STILL NEVER TICKS AND STILL ARMS NO TIMER.**
 *  Entry arrives as a nav-link callback and completion arrives as
 *  `ASummonedUnit::OnLadderClimbEnded`; both are pushed to us, so there is nothing
 *  to poll (`TOWER-§8` (4)).
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
 *  ⚠️ WHAT DID CHANGE, STATED PLAINLY BECAUSE IT IS A REAL LOSS (`TOWER-§8.4(B)`):
 *  the .cpp now includes SummonedUnit.h — for **MOVEMENT**. `TOWER-§`'s founding
 *  idea was a tower that never learns a unit exists, and a ladder ends that,
 *  because something must drive a specific unit up a specific line. ⭐ THE ONE
 *  COUPLING IS MOVEMENT AND IT IS THE ONLY ONE. The damage seam is untouched.
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
		/** ⛔ The pawn on the link is not an ASummonedUnit (the hero, or anything else that can path). */
		NotASummonedUnit,
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
	 *  ⚠️ ClimberTeam is meaningless when bClimberIsSummonedUnit is false, and the
	 *  identity test short-circuits before it is read.
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
		bool bClimberIsSummonedUnit, bool bLadderOccupied);

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
	 *    • Foot (−450, 0, 0): the body carves its 600×600 footprint and
	 *      `rcErodeWalkableArea` takes 2 more cells (64 uu), so ground navmesh
	 *      starts at X ≤ −364. −450 clears it by **86 uu**.
	 *    • Top (−150, 0, 1200): a 600 uu deck loses 64 uu PER SIDE to ledge-nulling
	 *      plus erosion, so the surviving deck poly is X ∈ [−236, +236]. −150 is
	 *      **86 uu inside it**.
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
	 *  through ASummonedUnit's own idempotent AbortLadderClimb, which restores the
	 *  movement mode and broadcasts exactly once.
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
	 *  `ASummonedUnit::BeginLadderClimb` or hands the agent straight back.
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
	 *  ⛔ NO TIMER: `ASummonedUnit` broadcasts `OnLadderClimbEnded` EXACTLY ONCE per
	 *  successful BeginLadderClimb, on every one of its exits (arrival, abort, a new
	 *  order, death, FreezeAI, ApplyFreeze, EndPlay, and this tower's own EndPlay).
	 *  Whatever ended it, the tower's job is identical: unbind, hand the agent back
	 *  to path following, forget it.
	 *
	 *  ⛔ `bReachedTop` deliberately drives NOTHING here. An arrival and an abort
	 *  differ for the UNIT (where it ends up) and ⛔ not for the TOWER (the ladder
	 *  is free either way). Branching on it would be the first line of a
	 *  bookkeeping subsystem `TOWER-§4` refuses.
	 */
	UFUNCTION()
	void HandleLadderClimbEnded(ASummonedUnit* Unit, bool bReachedTop);

	/** Unbinds Unit, hands it back to ordinary path following, and clears the ladder if Unit held it. IDEMPOTENT — EndPlay calls it after an abort that already called it. */
	void ReleaseClimber(ASummonedUnit* Unit);

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
	 */
	TWeakObjectPtr<ASummonedUnit> ActiveClimber;

	/** The exact UPathFollowingComponent the link handed us for ActiveClimber — the one holding CurrentCustomLinkOb, so the handshake is closed against the right object even if the unit changed controller mid-climb. */
	TWeakObjectPtr<UObject> ActiveClimberPathComp;
};
