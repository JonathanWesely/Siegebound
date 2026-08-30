// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Siegebound/Building.h"
#include "Siegebound/TeamId.h"
#include "ClimbableTower.generated.h"

class UBoxComponent;

/**
 *  ═══ THE CLIMBABLE WATCH TOWER (TASK-726, CONVENTIONS TOWER-§1/§3/§4/§5) ═══
 *
 *  A 30-gold building (CardID WatchTower) whose entire reason to exist is a
 *  1,200-uu PLATFORM a unit can stand on. It is a PLACE, ⛔ not a weapon.
 *
 *  ⭐⭐ THE ONE IDEA THAT MAKES THIS CLASS ALMOST EMPTY, AND IT IS DELIBERATE:
 *  the tower grants elevation by BEING TALL. Units WALK UP A REAL RAMP on a
 *  solid wedge (SM_WatchTower, TASK-727) using the walk animation they already
 *  have; Recast generates the walkable surface; HIGH-§ reads the unit's genuinely
 *  higher GetActorLocation().Z and hands out the damage bonus with ⛔ ZERO
 *  tower-awareness. There is therefore ⛔ NO teleport, ⛔ NO garrison, ⛔ NO
 *  special-case movement, ⛔ NO bespoke pathing and ⛔ NO ascent mechanic in this
 *  file — and none is missing. That is what lets a PLAYABLE tower ship with ⛔
 *  zero new animation (TOWER-§1); the climb clip (TASK-733) is a visual upgrade,
 *  ⛔ never a dependency.
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
 *  ─── WHAT THIS CLASS DELIBERATELY DOES ⛔ NOT CONTAIN (TOWER-§4, each with its
 *  reason — Tests/SiegeClimbableTowerTest.cpp asserts these ABSENCES) ───
 *
 *  ⛔ NO CAPACITY COUNTER. Physical space is the cap: a 600×600 uu platform holds
 *     4–6 bodies at AgentRadius 34. ⭐ An entire occupancy subsystem — a counter,
 *     a full/refuse state, its UI, its replication and its desync cases — is
 *     DELETED by choosing the ramp. The cheapest correct feature is the one whose
 *     bookkeeping does not exist.
 *
 *  ⛔ NO RANGED-ONLY FILTER. Any OWN-TEAM unit may walk up. Jonathan's "any ranged
 *     unit CAN climb it" is a PERMISSION, not an exclusion, and a filter admitting
 *     3 CardIDs while rejecting 8 manufactures the stuck-unit class NAV-§ exists
 *     for. A melee unit on top gains nothing (HIGH-§4 gates the bonus on the
 *     already-shipped bRangedAttack member) and self-selects away.
 *
 *  ⛔ NO OCCUPANT BOOKKEEPING AND NO DEATH SPECIAL CASE. When the tower dies its
 *     mesh unregisters, the floor vanishes, CharacterMovement drops to
 *     MOVE_Falling, occupants LAND and resume walking. This project has ⛔ NO FALL
 *     DAMAGE ANYWHERE (measured in TASK-725 §8: Landed / OnLanded / FallDamage /
 *     LandingVelocity / MOVE_Falling = zero hits across Siegebound), so
 *     fall-and-survive is FREE. ⛔ Do not add fall damage and ⛔ do not add a catch.
 *     ⚠️ CORRECTION OF RECORD (TASK-725 §8 vs this task's original spec): the
 *     NAV-§ stuck watchdog does ⛔ NOT cover a bad landing. Its terminal rung is
 *     EnterIdle() and it never teleports and never nav-projects
 *     (SiegeStuckStatics.h:237-241), so it covers a unit stuck ON the navmesh —
 *     ⛔ not one stranded OFF it. Nothing here cites it as a guarantee. See the
 *     handoff's declared residual.
 *
 *  ⛔ NO PATHING CODE. ABuilding already roots VisualMesh with BlockAll +
 *     SetCanEverAffectNavigation(true) (Building.h:43-51), so the body is solid
 *     and carves the navmesh like every other building; the ramp is the walkable
 *     route and Recast finds it with no help. ⭐ Pathing back DOWN is free —
 *     Recast polys are undirected (TASK-725 §7.4).
 *
 *  ⛔⛔ ZERO COUPLING TO HIGH-§ (HIGH-§3, TOWER-§ (6)). This class does ⛔ NOT tell
 *  anyone a unit is elevated, does ⛔ NOT call ASummonedUnit::HeightAdvantageMultiplier,
 *  and does ⛔ NOT include SummonedUnit.h — and SummonedUnit.{h,cpp} does not
 *  include this header. ⚖️ A unit on a HILL and a unit on a TOWER at the same Z
 *  MUST deal identical damage; a tower special case would make one rule into two
 *  and guarantee they eventually disagree. The test file asserts the parity.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AClimbableTower : public ABuilding
{
	GENERATED_BODY()

public:

	AClimbableTower();

	/**
	 *  The collision channel this tower's ascent gate BLOCKS — always the ENEMY
	 *  team's combatant object channel (SiegeNavAreas.h one-home). ⭐ THE SINGLE
	 *  SOURCE OF TRUTH: ConfigureAscentGate authors its one ECR_Block from this
	 *  function and CanTeamAscend below predicts from the same function, so the
	 *  shipped gate and the tested claim can never drift apart.
	 *
	 *  ⛔ No new channel is invented (TOWER-§4): ECC_SiegeTeamBlue/ECC_SiegeTeamRed
	 *  are the shipped, proven-in-play channels every combatant capsule is re-typed
	 *  to by team at BeginPlay for the castle's 3× hollow team gating.
	 *
	 *  Plain C++ static, ⛔ not a UFUNCTION, no world access, no actor access.
	 */
	static ECollisionChannel AscentBlockedChannel(ETeamId TowerTeam);

	/**
	 *  T-3, as a pure predicate: may a ClimberTeam unit reach this tower's
	 *  platform? ⛔ Enemies NO · ✅ ANY own-team unit YES · ⛔ no capacity term and
	 *  ⛔ no unit-type term in the signature, because there are none in the rule.
	 *
	 *  This is a STATEMENT OF THE SHIPPED COLLISION MATRIX, derived from
	 *  AscentBlockedChannel — ⛔ not a second gate. Nothing in the game calls it;
	 *  it exists so the ruling is assertable headlessly
	 *  (Tests/SiegeClimbableTowerTest.cpp), the HeightToBrightness /
	 *  HeightAdvantageMultiplier pure-seam precedent.
	 */
	static bool CanTeamAscend(ETeamId TowerTeam, ETeamId ClimberTeam);

	/** Platform height above this tower's origin, in uu — the number the mesh is built to and the number HIGH-§ turns into damage. See PlatformHeightUU. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Tower")
	float GetPlatformHeightUU() const { return PlatformHeightUU; }

protected:

	/** ABuilding's BeginPlay (team material, spawn squash, DT_Cards stat bind) then arms the ascent gate, once Team is authoritative. ⛔ No timer of any kind is started here. */
	virtual void BeginPlay() override;

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
	 *  ⚠️ THIS IS ALSO THE MESH'S CONTRACT. SM_WatchTower (TASK-727) is built to
	 *  exactly this rise, and TASK-725's ramp budget is arithmetic FROM it: at the
	 *  ruled 30° slope the horizontal run is 1200/tan(30°) = 2,078 uu. Changing
	 *  this value ⛔ does NOT move the mesh — it only de-synchronises the two. A
	 *  retune must land WITH a re-authored mesh.
	 *
	 *  EditDefaultsOnly because 🧑 row T-5 is Jonathan's: his next sentence
	 *  retunes the height without a code change. ⛔ Never a cards.csv column —
	 *  this is structure geometry, not a card stat.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Tower", meta = (ClampMin = "0"))
	float PlatformHeightUU = 1200.f;

	/**
	 *  TEAM GATE, PHYSICAL LANE (TOWER-§4 "can enemies climb it?" ⇒ ⛔ NO). An
	 *  invisible box, inert in the constructor and configured at BeginPlay by
	 *  ConfigureAscentGate from THIS tower's Team: object type = the OWN team
	 *  channel, responses = Ignore EVERYTHING, one ECR_Block on
	 *  AscentBlockedChannel(Team). ⭐ The mechanism is ACastle::GateBlockerVolume
	 *  verbatim (Castle.cpp:300-306, :592-615) — ⛔ no new channel, ⛔ no new area
	 *  class, ⛔ no bespoke filter, ⛔ nothing invented.
	 *
	 *  ⭐⭐ THE SHAPE IS AN ELEVATION SHELL, ⛔ NOT A DOORWAY — a DECLARED
	 *  departure from the castle's gate shape (SC-§15), and the reason is that a
	 *  doorway box would be a GUESS: SM_WatchTower does not exist yet, so nobody
	 *  can say where the ramp mouth sits in mesh-local space. An elevation shell
	 *  needs no such knowledge. It spans the tower's footprint horizontally and
	 *  runs from AscentGateFloorUU up past the platform, so:
	 *    • a ground-walking enemy passes UNDER it untouched (its capsule tops out
	 *      at 176 uu — see AscentGateFloorUU), and
	 *    • an enemy that starts up the ramp is stopped the moment it gains real
	 *      height, wherever on the mesh that ramp happens to be.
	 *  ⭐ It fails SAFE in both directions: sized too small an enemy climbs (no
	 *  stuck units, only a missed rule); sized too large it only forbids ENEMY
	 *  bodies from an air column above ground the tower's own solid body occupies.
	 *
	 *  Ignore-all base ⇒ invisible to every other query, exactly as the castle
	 *  gate is: the cursor placement trace (ECC_Visibility), the projectile
	 *  terrain-impact OBJECT query (WorldStatic/WorldDynamic), and every ECC_Pawn
	 *  reach test (hero melee, unit attacks, projectiles) all pass straight
	 *  through. It NEVER affects navigation — navmesh generation is untouched, so
	 *  ⛔ no enemy is ever stranded by this gate; it is blocked, on the navmesh,
	 *  which is the case the NAV-§ watchdog genuinely does cover.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Tower|Gating")
	TObjectPtr<UBoxComponent> AscentGateVolume;

	/**
	 *  Bottom of the ascent gate, relative to the tower origin (⇒ to the ground
	 *  the tower stands on). ⛔ NOT a style number — it is the clearance that keeps
	 *  the gate off ground traffic, and the arithmetic is:
	 *
	 *      ASummonedUnit is an ACharacter on the engine-default capsule
	 *      (radius 34, half-height 88 — the project never calls InitCapsuleSize;
	 *      SiegeSpawnConstants.h:9 names 88). A unit standing on the ground has
	 *      its capsule centre at Z 88 and its TOP at Z 176.
	 *      300 − 176 = ⭐ 124 uu of headroom.
	 *
	 *  So an enemy walking past on flat ground never touches this box, and the
	 *  124 uu also absorbs ground unevenness across the footprint. At the ruled
	 *  30° ramp an enemy climbs 300/tan(30°) ≈ 520 uu of run (a quarter of the
	 *  way) before it is stopped — visibly "turned back on the ramp", which reads
	 *  correctly.
	 *
	 *  ⚠️ RAISING this leaves enemies more ramp to walk; LOWERING it below ~200
	 *  starts catching ground-standing enemy capsules and IS the way to
	 *  manufacture stuck units. EditAnywhere so integration can correct it against
	 *  the real mesh, ⛔ not so it can be nudged by feel.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Tower|Gating", meta = (ClampMin = "0"))
	float AscentGateFloorUU = 300.f;

	/** Gate height ABOVE the platform: 400 uu clears a 176-uu body standing on the deck plus margin, so an enemy cannot be shoved onto the platform through the top of the shell. Top of the gate = PlatformHeightUU + this. */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Tower|Gating", meta = (ClampMin = "0"))
	float AscentGateHeadroomUU = 400.f;

	/**
	 *  Gate half-extents in the tower's LOCAL X/Y — sized to cover the whole
	 *  structure's footprint from TASK-725's budget: a 2,078-uu ramp run plus a
	 *  600-uu platform is a ~2,700-uu span (half 1,350 ⇒ 1,500 with margin) at a
	 *  300-uu ramp deck width plus the tower body (⇒ 500).
	 *
	 *  ⚠️⚠️ THE ONE NUMBER HERE THAT IS UNVERIFIED, AND IT IS FLAGGED RATHER THAN
	 *  HIDDEN: this assumes the ramp runs along the mesh's LOCAL X. SM_WatchTower
	 *  does not exist yet (TASK-727), so no one can confirm the axis. If the ramp
	 *  runs along local Y, ⭐ SWAP THESE TWO NUMBERS. TASK-728 wires the BP against
	 *  the real mesh and TASK-731 integrates — the axis must be VERIFIED there, ⛔
	 *  not assumed. Getting it wrong fails OPEN (an enemy climbs the far end of
	 *  the ramp), ⛔ never into a stuck unit. EditAnywhere for exactly that fix.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Tower|Gating")
	FVector2D AscentGateHalfExtentXY = FVector2D(1500.f, 500.f);

private:

	/**
	 *  Arms the physical team gate from this tower's Team — symmetric by
	 *  construction (a Blue tower blocks Red, a Red tower blocks Blue, with ⛔ no
	 *  hardcoded team branch), mirroring ACastle::ConfigureTeamGating's physical
	 *  lane. Degenerate tuning (an inverted or zero-volume box) leaves the gate
	 *  INERT and logs a warning rather than arming a nonsense volume.
	 *
	 *  ⛔⛔ THE CASTLE'S SECOND LANE — the UNavModifierComponent stamping
	 *  UNavArea_{Blue,Red}CastleInterior so the enemy's UNavFilter_Team* never
	 *  PATHS in — IS DELIBERATELY NOT REPRODUCED HERE. A DECLARED deviation
	 *  (SC-§15), refused on a checkable mechanism, not on taste:
	 *
	 *    (a) ⚠️ IT WOULD MAKE THE TOWER UNATTACKABLE. The excluded area would
	 *        cover the whole ~2,700-uu footprint, so an enemy melee unit's path
	 *        would END AT THE FOOTPRINT BOUNDARY — up to ~1,300 uu from the tower
	 *        body, and melee reach is ~150 uu. A destructible 250-HP building that
	 *        enemy melee can never reach is a worse bug than the one the lane
	 *        prevents.
	 *    (b) ⭐ THE PILE-UP IT EXISTS TO PREVENT CANNOT HAPPEN HERE. The castle
	 *        needs the nav lane because the enemy castle IS a path GOAL, so paths
	 *        genuinely route inside it. This tower's platform is a DEAD END — its
	 *        only nav connection is back down the ramp — so no route through it is
	 *        ever shorter and Recast never chooses it. The asymmetry is structural.
	 *    (c) ⚠️ It would punch a team-excluded hole in the battlefield around a
	 *        building placed at runtime, stranding any enemy already standing
	 *        there — inventing the NAV-§ failure this whole design avoids.
	 *
	 *  ⇒ The physical lane alone fully delivers T-3 ("enemies cannot climb"), and
	 *  it leaves the navmesh — and therefore enemy pathing to the tower — exactly
	 *  as ABuilding already ships it.
	 */
	void ConfigureAscentGate();
};
