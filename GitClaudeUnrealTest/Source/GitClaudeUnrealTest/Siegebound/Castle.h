// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Castle.generated.h"

class ACastle;
class ACommanderNpc;
class ATorch;
class UBoxComponent;
class UCameraShakeBase;
class UMaterialInterface;
class UNavModifierComponent;
class USiegeHitFlashComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 *  Broadcast exactly once when a castle's HP reaches 0 (GDD §3.9).
 *  DestroyedCastle is the castle that fell; CastleTeam is its team —
 *  the OTHER team is the match winner. ASiegeGameMode (TASK-006)
 *  subscribes to this on every ACastle at BeginPlay.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleDestroyed, ACastle*, DestroyedCastle, ETeamId, CastleTeam);

/**
 *  Broadcast on every ACTUAL CurrentHP change (playtest R1 finding 2, TASK-018):
 *  after damage is applied in TakeDamage — NEVER for ignored friendly fire,
 *  which changes nothing — plus in ResetCastle and once at BeginPlay (seed).
 *  UI consumers must still seed from GetCurrentHP()/GetMaxHP() FIRST and bind
 *  second (qa/TASK-005-report.md major 2); UCastleHealthBarWidget::InitForCastle
 *  does exactly that.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleHPChanged, float, CurrentHP, float, MaxHP);

/**
 *  Siegebound castle (GDD §3.9). One per team, placed at the CastleAnchor
 *  TargetPoints in L_Arena at integration (Castle_Blue / Castle_Red).
 *
 *  - Does not attack; it is a 2000 HP objective.
 *  - No friendly fire: damage whose instigator is on the same team is ignored (§3.0).
 *  - At 0 HP it broadcasts OnCastleDestroyed exactly once, hides, and stops colliding.
 *  - ResetCastle() (Play Again, §3.9) restores full HP, visibility, and collision.
 *  - Mesh and per-team material are soft references resolved null-safe in
 *    OnConstruction — the art assets are produced in parallel and may not exist yet.
 *  - Overhead HP bar (playtest R1 finding 2, TASK-018): screen-space HPBarWidget
 *    component; its widget class is soft-resolved null-safe at BeginPlay
 *    (WBP_CastleHealthBar, built in TASK-019 — a missing asset is a silent
 *    no-op). Hidden on destruction, shown again by ResetCastle().
 *
 *  ⛔⛔ THE CASTLE FURNISHES ITSELF — NOTHING IS PLACED IN L_Arena (batch WAR
 *  ROOM, TASK-562; law CONVENTIONS WR-§4 placement clause + WR-§5 placement
 *  clause). At BeginPlay this actor SPAWNS and ATTACHES its own interior
 *  torches (ATorch, from TorchAnchors) and its own commander NPC + war table
 *  (ACommanderNpc, from CommanderNpcAnchor), then pushes its own Team into the
 *  NPC. ⚖️ THREE LOAD-BEARING REASONS, AND ALL THREE ARE WHY THIS IS CODE
 *  RATHER THAN LEVEL CONTENT:
 *    (1) L_Arena is the asset this project is under STANDING ORDERS never to
 *        save (WR-§3; the one-time nav-data-only exception was granted for
 *        TASK-350 and EXPIRED at that commit). Furniture placed by hand would
 *        require exactly the save that is forbidden.
 *    (2) BOTH castles get IDENTICAL furnishing BY CONSTRUCTION — there is no
 *        mirror step for anyone to get wrong, and any future re-pose is
 *        handled for free because every anchor is CASTLE-MESH-RELATIVE.
 *        (TASK-637 comment rider, GH-R13: the old claim here that "Castle_Red's
 *        yaw 180" is what the relative anchors absorb was STALE — BOTH castle
 *        actors sit at yaw 0 and both gates face world −Y, measured live at
 *        TASK-617 C1. The mesh-relative reasoning stands on its own: if a
 *        level edit ever DOES yaw a castle, its furniture follows with no
 *        second edit.)
 *    (3) The anchors travel with the castle actor FOREVER: move, rotate or
 *        re-scale the castle and its furniture follows, with no second edit.
 *
 *  ⚠️ THE FURNISHING IS DELIBERATELY **NOT** AUTHORITY-GATED, AND THAT IS THE
 *  OPPOSITE OF THE USUAL M8 RULE — SO IT IS STATED HERE RATHER THAN LEFT TO
 *  LOOK LIKE AN OVERSIGHT. ATorch and ACommanderNpc are NET RELEVANCY TIER C
 *  (not replicated, no gameplay truth — declared in their own headers). This
 *  BeginPlay runs on the server AND on every client, so each machine builds
 *  its own identical local set from the same EditDefaultsOnly anchors: a pure
 *  local projection of already-replicated castle state, exactly the
 *  AAncientGround / AGoldNode precedent. ⛔ A HasAuthority() guard here would
 *  be the DEFECT, not the safeguard — it would leave every client with an
 *  unlit castle and no commander. Nothing in this lane mutates gameplay truth,
 *  so there is no mutation to guard.
 *
 *  ⛔ LIFECYCLE — see ApplyDestroyedState(), which is where the furnishing is
 *  torn down and rebuilt. A leak here is 12 orphan point lights per replay.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ACastle : public AActor, public ITeamAgent
{
	GENERATED_BODY()

public:

	ACastle();

	/** Fired exactly once when this castle is destroyed. Win-condition hook for ASiegeGameMode (TASK-006). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleDestroyed OnCastleDestroyed;

	/** Fired on every actual HP change, in ResetCastle, and once at BeginPlay (seed). Drives WBP_CastleHealthBar (TASK-018/019). M8: OnRep_CurrentHP fires this SAME delegate on clients — zero widget changes (doc §3.1). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleHPChanged OnCastleHPChanged;

	/**
	 *  ⚖️ NET RELEVANCY TIER: **A — `bAlwaysRelevant = true`** (declared per the
	 *  CONVENTIONS NET RELEVANCY LAW declaration duty; set in the constructor).
	 *  Rationale: match-critical near-singleton (two per match) carrying the WIN
	 *  CONDITION's state — its HP/crumble/destroyed truth may never depend on
	 *  camera distance. TASK-357 measured the far castle (488 m) failing under
	 *  the engine's default 150 m relevancy; this is that blocker's fix.
	 */

	/** Registers the M8 P1 castle set — CurrentHP / bDestroyed / CrumbleStage (OnReps) + Team (InitialOnly belt) — doc §3.1 (TASK-356). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin ITeamAgent interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent interface

	/**
	 *  Applies incoming damage. Same-team instigators are ignored entirely (no friendly
	 *  fire, §3.0). Damage-vs-fortification scaling (§3.0) is read from
	 *  DamageEvent.DamageTypeClass: USiegeDamageType_Siege = 200% (Siege units batter
	 *  the castle, TASK-054), USiegeDamageType_Projectile (and subclasses) = 50% (the
	 *  anti-sniping rule), melee/default/untyped = 100%. Units and the hero always take
	 *  listed damage (no scaling). Returns the SCALED amount the castle actually took.
	 *  USiegeDamageType_Spell (and subclasses) = 50% (spells vs the castle, GDD §3.0/§3.11 — TASK-098).
	 */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Play Again (§3.9): restores MaxHP, visibility, and collision, and re-arms the destroyed event. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Castle")
	void ResetCastle();

	/**
	 *  Heals this castle by Total HP spread evenly over Duration seconds (Masons
	 *  Instant — GDD §4, TASK-059). A repeating timer delivers the heal in fixed
	 *  HealTickInterval ticks; each tick is CLAMPED to MaxHP and broadcasts
	 *  OnCastleHPChanged on any actual change, so the HP bar (TASK-018) tracks the
	 *  repair live. A second call while a heal runs ADDS Total to the remaining pool
	 *  and re-derives the per-tick rate over the new Duration (Masons restack). The
	 *  stream self-terminates when the pool empties OR HP reaches MaxHP (never over
	 *  MaxHP), and is cancelled by HandleDestroyed()/ResetCastle(). No-op on a
	 *  destroyed castle or a non-positive Total/Duration. The heal MAGNITUDES belong
	 *  to the caller (ASiegePlayerController's Masons UPROPERTYs, CONVENTIONS
	 *  mechanic-rule) — this stays a generic heal-over-time receiver.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Castle")
	void HealOverTime(float Total, float Duration);

	/** Current hit points, in [0, MaxHP]. HUD/QA hook (TASK-011). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	float GetCurrentHP() const { return CurrentHP; }

	/** Maximum hit points (2000 per GDD §3.9). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	float GetMaxHP() const { return MaxHP; }

	/** True once the castle has been destroyed and until ResetCastle(). Unit targeting (TASK-004) should skip destroyed castles. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	bool IsCastleDestroyed() const { return bDestroyed; }

	/**
	 *  True when Point lies within this castle's 2D spawn box — a castle-centered square
	 *  test: |Point.X - ActorX| <= SpawnBoxHalfExtent.X && |Point.Y - ActorY| <=
	 *  SpawnBoxHalfExtent.Y (Z ignored). Used by the Shield Wall ATTACK command
	 *  (ASummonedUnit::FindNearestEnemyInSpawnBox, W1 TASK-275) so player units clear
	 *  enemies massing inside the enemy castle's spawn region before hitting the castle.
	 *  ADDITIVE — this does NOT touch the bot's ASiegeBotController::IsPointInBotSpawnBox
	 *  (TASK-262 logic UNDISTURBED); it is an independent third reader of the same box.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	bool IsPointInSpawnBox(const FVector& Point) const;

	/**
	 *  THE INTERIOR ANCHOR (TASK-398; signature PINNED character-for-character by
	 *  CONVENTIONS "FOLLOW command + the DEFAULT-STANCE law + the MINER command
	 *  rework (2026-08-02)" §7). The world point inside this castle's shell that a
	 *  unit told to "hide inside the castle" walks to — Jonathan's Defend (E)
	 *  semantics for the Miner (§5's table).
	 *
	 *  ⚠️ THIS INVENTS NO MECHANIC — IT REUSES TASK-350. The 3× castle is HOLLOW
	 *  and walk-in, and own-team units already enter through the shipped team
	 *  gating: UNavArea_{Blue,Red}CastleInterior on InteriorNavModifier (the enemy's
	 *  UNavFilter_Team* excludes it; an UNFILTERED query — which is what a miner's
	 *  MoveToLocation issues — treats it as plain navmesh) plus the GateBlockerVolume
	 *  that ignores the OWN team's channel. This function only names the destination.
	 *
	 *  = the ACTOR TRANSFORM applied to InteriorAnchorRelativeLocation, never
	 *  ActorLocation + offset. (TASK-623 comment rider, CR-R6: the claim here that
	 *  "Castle_Red is placed at yaw 180" was STALE — BOTH castle actors sit at yaw
	 *  0 and both gates face world −Y, measured live at TASK-617 C1. The transform
	 *  form is kept regardless: a non-zero relative anchor must rotate with the
	 *  castle if a level edit ever yaws one, or it lands outside the wrong wall.)
	 *  At the shipped ZeroVector default the two are identical BY CONSTRUCTION, and
	 *  this returns the actor's own location.
	 *
	 *  Callers own the "no castle" case: a DESTROYED castle is not a hiding place
	 *  (FindNearestCastleForTeam already skips them, so the caller gets nullptr and
	 *  idles in place — CONVENTIONS §5, "own castle destroyed ⇒ idle in place").
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	FVector GetInteriorAnchorLocation() const;

	/**
	 *  Nearest STANDING castle belonging to Team (TASK-398). Additive helper, the
	 *  AGoldNode::FindBestMineFor idiom: one public static finder living on the
	 *  finder's own type, so callers outside ASummonedUnit can resolve "my castle".
	 *
	 *  ⚠️ WHY THIS EXISTS RATHER THAN A CALL TO ASummonedUnit::FindOwnCastle():
	 *  that function is PRIVATE (SummonedUnit.h, inside the private block) and
	 *  SummonedUnit.{h,cpp} is another task's exclusive file this batch — so a
	 *  promote-to-protected edit was not available. This is a faithful mirror of it:
	 *  same-team, IsValid, skip destroyed, nearest wins, first-found on an exact tie.
	 *  The ONE deliberate difference is the metric — squared 2D distance (the house
	 *  arena metric, matching FindBestMineFor and the miner's own arrival test)
	 *  instead of the base's bounds-aware GetDistanceToTarget. With exactly one own
	 *  castle per match the two can never disagree about the winner.
	 *
	 *  Null-safe on World; returns nullptr when the team has no standing castle.
	 *  FLAGGED for a later consolidation pass: ASummonedUnit::FindOwnCastle could
	 *  delegate here once that file is free (one line, out of scope for TASK-398).
	 */
	static ACastle* FindNearestCastleForTeam(UWorld* World, ETeamId Team, const FVector& From);

	/** Resolves the soft-referenced mesh and per-team material, null-safe. */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/**
	 *  TASK-349 loop-2 B3 Leg 2: selects the TEAM's interior nav area on
	 *  InteriorNavModifier here — after the serialized Team is authoritative,
	 *  BEFORE the first navmesh generation pass — so FRESHLY GENERATED interior
	 *  tiles build correct-first-time (never ctor-Blue-then-flip). This is the
	 *  GENERATION-TIME half only: pre-built/saved tiles see no area CHANGE here
	 *  and are re-marked by the loop-4 unconditional BeginPlay refresh in
	 *  ConfigureTeamGating (B4 — the two hooks cover different lanes and both
	 *  must stay; see the InteriorNavModifier doc). BeginPlay's SetAreaClass
	 *  re-assert early-outs on the unchanged value (free no-op, kept for
	 *  spawned-castle paths).
	 */
	virtual void PostInitializeComponents() override;

	/** Seeds CurrentHP from MaxHP, fires the OnCastleHPChanged seed broadcast, initializes the HP bar widget (null-safe), arms team gating, and spawns this castle's own furnishing (TASK-562). */
	virtual void BeginPlay() override;

	/**
	 *  Tears the spawned furnishing down (TASK-562). ⚠️ A BELT, DECLARED AS ONE:
	 *  the ordinary teardown paths are ApplyDestroyedState(true) and world
	 *  shutdown (which ends every actor anyway). This override closes the one
	 *  remaining lane — an explicit ACastle::Destroy() — because AActor::Destroy
	 *  DETACHES its attached actors rather than destroying them, so without this
	 *  a destroyed castle would leave its torches and its commander standing in
	 *  mid-air with no owner. Nothing calls ACastle::Destroy() today; this exists
	 *  so that nothing has to remember not to.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Static mesh root. Mesh asset assigned null-safe in OnConstruction from CastleMeshAsset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UStaticMeshComponent> CastleMesh;

	/** Screen-space overhead HP bar (DrawSize 256x32 at Z+9450 — the 9× castle is ~8083 tall; the Z is C++-AUTHORED here, not BP-authored, so TASK-557 re-derived it ×3 a second time: 1050-over-900 → 3150-over-2694 (TASK-349) → 9450-over-8083, holding the same ≈1.17× headroom over the mesh top at every scale). Widget class resolved null-safe at BeginPlay from HPBarWidgetClass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UWidgetComponent> HPBarWidget;

	/** §6 white hit-flash on every actual damage event (M7, TASK-154). Driven from TakeDamage; overlay-based (composes cleanly with the crumble MI swap), null-safe. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeHitFlashComponent> HitFlashComponent;

	/**
	 *  Team gate blocker (TASK-349, CONVENTIONS "Castle 3× HOLLOW" team-gating law,
	 *  PHYSICAL lane): an invisible box spanning the gate opening, configured at
	 *  BeginPlay (ConfigureTeamGating) from THIS castle's Team — object type = the
	 *  OWN team channel, responses = Ignore everything, Block ONLY the enemy team's
	 *  channel. So enemy combatants (units AND the player-driven hero — hero
	 *  ruling) are physically stopped at the gate while the own team walks through
	 *  untouched. An ACTOR component: the crumble mesh swap (ApplyCrumbleStage
	 *  SetStaticMesh) can never strip it. Never affects navigation (the nav lane is
	 *  InteriorNavModifier's job) and is invisible to every trace/overlap path
	 *  (ignore-all base: cursor ECC_Visibility, projectile WorldStatic terrain
	 *  query by OBJECT type, ECC_Pawn distance math all pass through). Sized and
	 *  positioned by the two Gating tunables below, whose defaults are the
	 *  TASK-350 PIE-VERIFIED gate values for the 3× hollow SM_Castle (loop-2
	 *  ride-along — see GateBlockerRelativeLocation for the co-commit reasoning
	 *  and the honest mesh-only-revert residue).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Gating")
	TObjectPtr<UBoxComponent> GateBlockerVolume;

	/**
	 *  Team interior nav marker (TASK-349, PATHING lane): stamps this castle's
	 *  collision bounds with UNavArea_BlueCastleInterior or
	 *  UNavArea_RedCastleInterior (ctor default is the Blue area, NEVER
	 *  UNavArea_Null, so navmesh GENERATION is untouched in every state). The
	 *  areas cost 1 (normal walk) — the OWN team and un-filtered queries
	 *  (placement projection) treat the interior floor as plain navmesh, while the
	 *  ENEMY team's UNavFilter_Team* EXCLUDES it, so enemy AI never paths inside
	 *  (no door pile-up). Actor component — crumble-swap-proof, like the blocker.
	 *
	 *  Nav-area determinism hardening (loop-2 B3 legs 1–3 + loop-4 B4, all four
	 *  deliberately coexisting):
	 *  (1) the ctor calls ForceNavigationRelevancy(true) so this component owns
	 *  its OWN nav-octree element instead of riding the root CastleMesh's GEOMETRY
	 *  element (UNavRelevantComponent attaches to the owner's root by default —
	 *  which routed our per-hull area list through the engine's raw-geometry
	 *  GetCollisionAreaClass, the `Areas.Num() <= 1` ensure site, and coupled the
	 *  area marking to every mesh-swap/collision-toggle rebuild of the geometry
	 *  element; decoupled, the areas apply through the dynamic-area marking path —
	 *  the NavModifierVolume shape — which supports them properly and survives
	 *  geometry churn). FINAL-RUN-confirmed (ensure absent).
	 *  (2) the TEAM area is selected in PostInitializeComponents, BEFORE the first
	 *  navmesh generation pass — GENERATION-TIME correctness: freshly generated
	 *  tiles build team-correct-first-time (no ctor-Blue → Red flip window).
	 *  (3) ResetCastle and ApplyCrumbleStage re-assert the modifier's octree entry
	 *  in the SAME frame as their mesh swap (RefreshNavigationModifiers), so every
	 *  tile rebuilt by the swap gathers the team area — no Play-Again enemy-open
	 *  window. FINAL-RUN-confirmed (the crumble fence re-marked the red hall).
	 *  (4) loop-4 B4 (Jonathan-authorized): ConfigureTeamGating at BeginPlay runs
	 *  an UNCONDITIONAL RefreshNavigationModifiers — the PRE-BUILT-TILE re-mark.
	 *  Leg 2 removed the only post-registration area CHANGE, so editor-built and
	 *  SAVED tiles (every real boot lane; the editor world never runs the runtime
	 *  hooks, its tiles are always ctor-Blue) were never dirtied and stayed stale
	 *  UNBOUNDED (the 626-sample/211 s FINAL-RUN probe). The startup refresh
	 *  forces those tiles to rebuild once against the already-correct team area;
	 *  on the fresh-build lane it is at most one redundant re-mark. Legs 2 and 4
	 *  answer DIFFERENT lanes — do not fold either into the other.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Gating")
	TObjectPtr<UNavModifierComponent> InteriorNavModifier;

	/** Which team owns this castle. Set per level instance (Castle_Blue = Blue, Castle_Red = Red). M8 (TASK-356, doc §3.1): replicated COND_InitialOnly as a belt — the value is level-authored identically on both machines already (the CASTLE-3X gating reads it client-side, addendum §2). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	ETeamId Team = ETeamId::Blue;

	/** Maximum hit points (GDD §3.9). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle", meta = (ClampMin = "1"))
	float MaxHP = 2000.0f;

	/**
	 *  Half-extent (XY) of this castle's spawn box, read by IsPointInSpawnBox (W1
	 *  TASK-275, Shield Wall ATTACK command). PAIRED-TUNABLE (3-way law, CONVENTIONS):
	 *  ACastle::SpawnBoxHalfExtent ≡ ASiegePlayerController::SpawnBoxHalfExtent ≡
	 *  ASiegeBotController::SpawnBoxHalfExtent — all default (7380,7380); keep the three
	 *  in lockstep. Re-derived 840 → 2460 by TASK-349 (Castle 3× HOLLOW), then
	 *  2460 → 7380 by TASK-557 (CONVENTIONS WR-§2 row 1, the 9× castle) — the SAME
	 *  paired-tunable law both times: half-extent ≈ the castle's full width
	 *  (9× bounds 7313.7 × 7384.5 × 8082.6 uu), preserving the original intent, which
	 *  keeps the box spanning the castle's walkable INTERIOR — spawn-inside works by
	 *  construction. The value appears in 3 places (flagged in CONVENTIONS); a future
	 *  pass MAY delegate both controllers to this castle helper — OUT of scope here.
	 *  The mid ACaptureZone::ZoneHalfExtent deliberately STAYS (840,840) (WR-§2 row 7).
	 *
	 *  ⚠️ TASK-557 LEDGER NOTE — TWO CONSEQUENCES OF THE 7380 BOX, BOTH REPORTED, NEITHER
	 *  A DEFECT HERE. (a) The box now reaches |X| = 32,380 against a ±26,000
	 *  USiegeScatterConfig::ArenaHalfExtent.X — HARMLESS, because the box is only the
	 *  FIRST gate: navmesh projection + collision + clearances still run and there is no
	 *  navmesh past the arena, so the overhang can never yield a placement. (b) It moves
	 *  the spawn-box EDGE from |X| = 22,540 to |X| = 17,620, which is the figure
	 *  CONVENTIONS "Ancient Grounds …" names as THE binding constraint on
	 *  USiegeScatterConfig::AncientGroundMaxAbsX. ✅ THAT FLAG WAS ANSWERED THE SAME
	 *  DAY AND THIS NOTE IS THE RECORD OF IT: manager ruling W2-R1 (CONVENTIONS
	 *  WR-§2b row E) RE-DERIVED the ceiling 21,000 → 16,080, and TASK-576 landed it
	 *  in ScatterConfig.h. ⛔ IT IS NO LONGER "FLAGGED", IT IS RULED — do not re-open
	 *  a closed ruling. 16,080 is not a new margin: it re-solves the SAME relationship
	 *  against the 17,620 edge this note reports, preserving the original 1,540 centre
	 *  margin and the original 700 uu footprint-edge clearance exactly. ⚠️ Two
	 *  artifacts, and NEITHER is this header's: the C++ default lives in
	 *  ScatterConfig.h (TASK-576) and the saved DA_BattlefieldScatter is TASK-569's
	 *  editor step.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Castle", meta = (ClampMin = "0"))
	FVector2D SpawnBoxHalfExtent = FVector2D(7380.f, 7380.f);

	/**
	 *  GateBlockerVolume center, relative to the castle root (TASK-349 Gating
	 *  tunable — component data, so neither a mesh import nor a crumble swap can
	 *  strip it). The TASK-350 PIE-VERIFIED position for the AUTHORED 3× hollow
	 *  SM_Castle was (6, −525, 284) — the gate corridor mouth on the local −Y side;
	 *  hero blocked at the enemy gate / passing his own, proven in-engine on both
	 *  instances.
	 *
	 *  RE-DERIVED ×3 → (18, −1575, 852) by TASK-557 for the 9× castle (CONVENTIONS
	 *  WR-§2 row 3). It is a MESH-LOCAL offset and the mesh scaled uniformly under
	 *  it, so the ×3 is exact, not an estimate: the point tracks the same feature of
	 *  the same geometry. SIGN CONVENTION VERIFIED BEFORE TYPING, not assumed — Y is
	 *  NEGATIVE because the gate corridor mouth is on the local −Y side and +Y is
	 *  "deeper into the keep" (InteriorAnchorRelativeLocation's doc states this, and
	 *  the shipped box span Y [−660, −390] confirms it); ×3 preserves the sign and
	 *  the direction.
	 *
	 *  WHY baking a mesh-derived value here is SAFE (co-commit reasoning, QA
	 *  re-check point, unchanged in substance from TASK-350): the code default and
	 *  the mesh it is measured against land in the SAME batch, so no committed world
	 *  ever pairs these values with a mesh of a different scale.
	 *
	 *  HONEST RESIDUE (the failure path's known, accepted cost — RE-STATED for 9×): a
	 *  future MESH-ONLY revert to the 3× castle (or further back to the solid
	 *  814.5×820.6×894.9 one) re-creates a mis-placed blocker, now proportionally
	 *  larger. Any such revert must retune these two tunables WITH the mesh.
	 *  EditAnywhere stays — per-instance facing/offset corrections remain the
	 *  integration task's lever.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Castle|Gating")
	FVector GateBlockerRelativeLocation = FVector(18.f, -1575.f, 852.f);

	/**
	 *  GateBlockerVolume half-extents (TASK-349 Gating tunable). The TASK-350
	 *  PIE-VERIFIED cover for the 3× castle was (260, 135, 226): X 520 span across
	 *  the corridor mouth (a 600-wide clear opening ⇒ ≈40 uu of jamb gap per side),
	 *  Y 270 through the wall (no capsule tunneling), Z covering floor ≈58 up to
	 *  ≈510, past the ≥450 clear height.
	 *
	 *  ⛔⛔ RE-DERIVED FOR THE 9× CASTLE BY TASK-557 — AND **X IS NOT A ×3**
	 *  (CONVENTIONS WR-§2 row 2 says ×3; this is a DECLARED DEPARTURE under SC-§15,
	 *  refused on a CHECKABLE MECHANISM, and the arithmetic is the whole argument):
	 *    • Y 135 → 405 and Z 226 → 678 ARE ×3. Both span SHELL features that scaled —
	 *      wall thickness and floor-to-lintel. The new Z band is [174, 1530], and its
	 *      floor 174 is EXACTLY the 9× interior floor height (3 × 58, WR-§1), so the
	 *      blocker still sits ON the threshold rather than above or below it; 1530
	 *      still clears the ≈1356 gate clear height, as 510 cleared ≈452.
	 *    • X 260 → **900, NOT 780.** ⚠️ THE VALUE IS RATIFIED AND THE ARGUMENT THAT
	 *      STOOD HERE IS RETIRED — they are not the same act (CONVENTIONS WR-§2b
	 *      ruling **W4-R4**, 2026-08-15, raised as qa/TASK-565.md WARN-1). ⛔ That
	 *      ruling keeps the retired wording VERBATIM, which is exactly why it is not
	 *      re-printed here: two copies of a dead claim is how it comes back to life.
	 *      Read it there; what follows is the corrected reasoning.
	 *      ⛔ THE APERTURE IS MEASURED, NEVER PROJECTED. The delivered 9× gate is a
	 *      **1560 uu COLLISION gap spanning x −762 … +798**, and a **1470 uu VISUAL**
	 *      opening — handoffs/TASK-555-artist.md, the as-built readback. ⛔ **1800 is
	 *      the CARVE-CUTTER RECIPE width** (that handoff's row H, "recipe only; no
	 *      carve ran"): the width of the TOOL, not the width of the HOLE, because the
	 *      cutter meets wall geometry.
	 *      ⇒ AGAINST THAT APERTURE THE ×3 IS AN EXACT, ZERO-GAP FIT — not a leaky one.
	 *      With GateBlockerRelativeLocation.X = 18, X = 780 spans −762 … +798,
	 *      character for character the measured collision gap. ⇒ **the jamb gap at 780
	 *      is 0 per side, and the mesh-local X offset leaves NO residual** to argue
	 *      about.
	 *      ⇒ SO 900 STANDS AS A HARMLESS SUPERSET, and that is the whole of its
	 *      justification — it closes no gap, because there is none to close. It spans
	 *      −882 … +918, i.e. it embeds 120 uu per side INTO THE SOLID UCX JAMB HULLS
	 *      (a 1560 collision gap is precisely the claim that everything outside
	 *      −762 … +798 is hull). Inert twice over: nothing can occupy that space to be
	 *      blocked by it, and the volume Ignores every channel except the ENEMY team's
	 *      (ConfigureTeamGating).
	 *      ⛔ WHAT THE TOLERANCE IS STILL PINNED TO — this half was never in doubt:
	 *      **BODIES DID NOT GROW (WR-§1 / SC-§34's human-scale exemption), so the GAP
	 *      TOLERANCE MAY NOT GROW EITHER.** The bar is the narrowest agent DIAMETER on
	 *      the field — hero capsule r≈42 ⇒ 84 uu (SiegeGameMode.h), Cavalry r45 ⇒ 90 uu
	 *      (CONVENTIONS "Castle 3× HOLLOW", the sizing agents). Any per-side gap that
	 *      reaches 84 uu lets those capsules walk into the enemy keep AROUND the
	 *      blocker, silently deleting the team-gated interior while every bounds
	 *      readback still passes.
	 *  ⚠️ The integration task PIE-VERIFIES the cover and may re-tune this EditAnywhere
	 *  value against the delivered geometry. It may NOT reduce it below "opening span
	 *  minus one agent diameter" — and that span is the MEASURED **1560**, ⛔ NOT 1800
	 *  — without re-opening the reasoning above.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Castle|Gating", meta = (ClampMin = "0"))
	FVector GateBlockerExtent = FVector(900.f, 405.f, 678.f);

	/**
	 *  🚩 FLAGGED TUNABLE (TASK-398; CONVENTIONS "FOLLOW command … (2026-08-02)"
	 *  §5 + §8). Where "inside the castle" IS, expressed in the castle's OWN
	 *  local frame — read only through GetInteriorAnchorLocation(), which applies
	 *  the actor transform (so any castle pose is handled for free — TASK-637
	 *  comment rider, GH-R13: the old parenthesis here credited "Castle_Red's
	 *  yaw 180", which was STALE: BOTH castle actors sit at yaw 0 and both gates
	 *  face world −Y, measured live at TASK-617 C1. The transform form is what
	 *  keeps this value pose-proof if a level edit ever yaws one — the
	 *  GetInteriorAnchorLocation() doc's TASK-623 rider reasoning).
	 *
	 *  Default ZeroVector, and it is not an arbitrary zero: SM_Castle's origin is
	 *  GROUND-CENTRE by law (CONVENTIONS WR-§0 — 9× bounds within ±10 % of
	 *  7326 × 7380 × 8082 uu, ground-centre origin), so local (0, 0) is the interior
	 *  floor's CENTRE in XY.
	 *
	 *  ⚖️ TASK-557 LEDGER ROW — (ii) DELIBERATELY UNCHANGED, AND THE REASON IS
	 *  STRUCTURAL, NOT A JUDGEMENT CALL: a ZERO VECTOR IS SCALE-INVARIANT. The centre
	 *  of a 3× castle and the centre of a 9× castle are the same local point, so
	 *  there is nothing here to multiply. ⚠️ What DID rot is the old comment's claim
	 *  that the interior floor is "FLAT AT GROUND LEVEL with a ≤40 uu threshold step":
	 *  the shipped floor is z ≈ 58 and the 9× floor is z ≈ 174 (WR-§1), reached by a
	 *  RE-DERIVED stair/ramp approach — the ≤40 uu STEP LIMIT survives (bodies did not
	 *  grow), the single step does not. Z 0 remains correct anyway BECAUSE this value
	 *  is only ever a navmesh DESTINATION: GetInteriorAnchorLocation feeds a
	 *  MoveToLocation, and the mover projects onto the interior floor poly whatever
	 *  its height.
	 *
	 *  ⚠️ WHAT TO CHANGE IT TO, AND WHEN: nudge it (never the code) if the PIE
	 *  measurement shows local (0,0) sitting inside a keep/tower hull rather than
	 *  the open hall — the symptom is a miner that stalls at the gate instead of
	 *  walking in. The gate corridor mouth is on the local −Y side
	 *  (GateBlockerRelativeLocation Y −1575), so +Y is "deeper into the keep".
	 *  EditDefaultsOnly, never replicated: it is design-time data, identical on
	 *  both machines by construction — exactly like the two Gating tunables above.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Gating")
	FVector InteriorAnchorRelativeLocation = FVector::ZeroVector;

	//~ ---------------- WAR ROOM FURNISHING (TASK-562; WR-§4 + WR-§5) ----------------
	//~ Every anchor below is CASTLE-MESH-RELATIVE and is authored in the NEW 9×
	//~ interior space. ⛔ NO pre-scale number was copied. The defaults are filled in
	//~ the constructor from named interior constants (Castle.cpp anonymous namespace),
	//~ so the derivation lives next to the arithmetic instead of behind a literal.

	/**
	 *  Where this castle's torches hang, in CASTLE-MESH-LOCAL space (WR-§4's
	 *  placement clause). ⚠️ AN ANCHOR IS A POINT **ON A WALL**, NOT ON THE
	 *  FLOOR: SM_Torch's origin is its WALL-MOUNT FACE (TASK-556 / WR-§4) and
	 *  the mesh extends along its own +X into the room, so each transform's
	 *  ROTATION is what aims the torch off the wall — yaw 0 = "the wall is to my
	 *  −X", yaw 180 = "to my +X", yaw +90 = "to my −Y", yaw −90 = "to my +Y". A
	 *  floor-origin torch would bury itself in the masonry, which is why the
	 *  origin convention is repeated here.
	 *
	 *  The six shipped defaults and how they were derived are documented in
	 *  Castle.cpp (ACastle::ACastle, the furnishing block). Scale is left at 1 on
	 *  every anchor — the torch's SIZE belongs to TASK-556's mesh, not to this
	 *  array.
	 *
	 *  ⚠️ FLAGGED FOR JONATHAN (TASK-571): both the count and the placement are
	 *  feel tunables. EditDefaultsOnly ⇒ BP_Torch's owner edits this array on the
	 *  castle's defaults with no recompile.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TArray<FTransform> TorchAnchors;

	/**
	 *  Hard cap on spawned torches per castle (WR-§4: "recommended default 6 ⇒ 12
	 *  shadowless point lights in the whole level"). The spawn takes
	 *  min(TorchAnchors.Num(), MaxTorchesPerCastle), so ADDING anchors in a
	 *  Blueprint can never quietly multiply the level's light count past this
	 *  number — raising the cap is the deliberate second edit.
	 *  0 disables torches entirely (a clean designer/perf kill switch, and the
	 *  answer if decision D3 ever comes back against runtime lights).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing", meta = (ClampMin = "0"))
	int32 MaxTorchesPerCastle = 6;

	/**
	 *  Where the commander NPC stands, in CASTLE-MESH-LOCAL space (WR-§5's
	 *  placement clause) — a FLOOR point in the grand hall, rotated so the
	 *  commander faces the room (his war table is placed by ACommanderNpc a
	 *  fixed distance along the actor's own +X). Derivation in Castle.cpp.
	 *
	 *  ⛔ HE MAY NOT STAND IN THE GATE CORRIDOR OR ON THE APPROACH: the corridor
	 *  is the only way in and out for both teams, and the approach is the
	 *  re-derived stair/ramp (WR-§1) — furniture on either is an obstacle in the
	 *  one route the whole feature depends on. The shipped default is checked
	 *  against both volumes in Castle.cpp's comment, and a re-tune must re-check.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	FTransform CommanderNpcAnchor;

	/**
	 *  Torch actor class, soft — /Game/Blueprints/BP_Torch (the ASiegeGameMode::
	 *  HeroPawnClassAsset precedent, verbatim). TWO DISTINCT NULL PATHS, and the
	 *  difference is deliberate:
	 *    • CLEARED (IsNull) ⇒ SILENT OPT-OUT — this castle spawns no torches at
	 *      all. The AttackImpactEffect designer-opt-out pattern (TASK-020).
	 *    • SET BUT UNRESOLVABLE ⇒ fall back to the raw C++ ATorch and say so ONCE
	 *      in the log. ⚠️ THIS IS THE PATH THAT RUNS TODAY: no task in the WAR
	 *      ROOM batch authors BP_Torch (TASK-566 imports meshes, 567 the crumble
	 *      trio, 568 the input/UI assets, 569 the DataAsset + PIE), so the raw
	 *      class is the shipped behaviour until somebody creates the Blueprint.
	 *      That is a complete, working torch — see ATorch's own defaults.
	 *  ⚠️ NAMED FORWARD: TASK-558 parks TASK-556's MEASURED flame-centre offset on
	 *  BP_Torch's TorchLightRelativeOffset. That landing site only exists once the
	 *  Blueprint does; until then the torch derives the offset from its own mesh
	 *  bounds (a ≈2.6 % polish item, never a defect). ⛔ Do NOT "fix" that by
	 *  transcribing the number into C++.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TSoftClassPtr<ATorch> TorchClassAsset;

	/**
	 *  Commander NPC actor class, soft — /Game/Blueprints/BP_CommanderNpc. Same
	 *  two null paths as TorchClassAsset: cleared = this castle gets no commander
	 *  (silent), set-but-unresolvable = the raw C++ ACommanderNpc plus one log
	 *  line. ⚠️ As with the torch, no task in this batch authors the Blueprint, so
	 *  the C++ fallback is what runs — and it is fully functional (soft meshes,
	 *  null-safe, its own EditDefaultsOnly tunables at their law defaults).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TSoftClassPtr<ACommanderNpc> CommanderNpcClassAsset;

	//~ ------ F1 DISCOVERABILITY FURNISHING (TASK-661; VID-001 branch (i)) ------
	//~ The castle-entry fix chain's placement lane (the F1-R2 activation ruling,
	//~ 2026-08-27). VID-001 was the FIRST real-input entry attempt on record and
	//~ it died at the EAST design seal — working exactly as GH-R9 intends — while
	//~ the SOUTH route re-proved hero-walkable live (handoffs/TASK-656-
	//~ buildmaster.md: 16/16 capsule stations EMPTY, risers manifest-true). The
	//~ defect is DISCOVERABILITY: nothing tells the player the door is south. The
	//~ three families below are pure visual signage — banner poles marking the
	//~ channel mouth, a trampled-path ribbon sweeping spawn line → around the toe
	//~ ring → the mouth, and rock dressing on the east seal line so the visible
	//~ cause matches the invisible wall. Every anchor is CASTLE-MESH-LOCAL like
	//~ every furnishing anchor above (symmetric on both castles by construction;
	//~ Red's mouth banners signpost the hero's ATTACK approach for free). The
	//~ derivations from 656's measured geometry live beside the arithmetic in
	//~ Castle.cpp (ACastle::ACastle, the F1 block).
	//~ ⛔ F1-R3: nothing in this lane touches a seal, a hull, or collision of any
	//~ kind. TASK-657's assets are collisionless by design (ucx: null declared)
	//~ AND every spawned component is forced NoCollision code-side regardless of
	//~ what the asset ships — GH-R9 belt and braces; the entry-chain collision
	//~ record stays byte-identical.

	/**
	 *  Gate banner-pole anchors, CASTLE-MESH-LOCAL (TASK-661): one marker either
	 *  side of the south channel mouth, faced SOUTH (+X forward = the banner's
	 *  read direction, the TASK-657 pivot contract) so the pair reads from the
	 *  open field and frames the one legitimate way in. Derivation in Castle.cpp;
	 *  EditDefaultsOnly ⇒ re-posing is a defaults edit, no recompile.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TArray<FTransform> GateBannerAnchors;

	/**
	 *  Trample-path ribbon anchors, CASTLE-MESH-LOCAL (TASK-661), in CHAIN ORDER:
	 *  the hero's own east-face stop lane heading south, a corner, the wrap west
	 *  along the measured-clear line south of the toe ring, and a final turn-in
	 *  segment pointing due north into the channel mouth. Each anchor's yaw aims
	 *  +X along the direction of travel (the TASK-657 tileable-along-X contract);
	 *  each sits a couple of uu over measured support with a small monotonic z
	 *  stagger so no two segments are ever coplanar whatever length the delivered
	 *  ribbon has. Derivation and the leg tables in Castle.cpp.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TArray<FTransform> TramplePathAnchors;

	/**
	 *  Toe-rock anchors, CASTLE-MESH-LOCAL (TASK-661): a picket of rocks along
	 *  the d1 seal line's visual rim crest — the exact line VID-001's hero jumped
	 *  at — so the friendly 95-uu green lip reads as the hard stop it already is
	 *  (656: riser +506.5 vs step 50, unmountable BY DESIGN and STAYING so —
	 *  F1-R3). Spacing ≈ 200 uu with deterministic yaw jitter, alternating the
	 *  01/02 meshes for variety. Derivation in Castle.cpp.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TArray<FTransform> ToeRockAnchors;

	/**
	 *  Gate banner mesh, soft — /Game/Meshes/SM_Castle_GateBanner (TASK-657
	 *  authors it IN PARALLEL; this code deliberately lands first). The ATorch
	 *  two-null-paths law: CLEARED (IsNull) = the silent designer opt-out — no
	 *  banners, no log; SET BUT UNRESOLVABLE = skip the family with ONE log line
	 *  (one-shot guard), never a crash. No C++ fallback exists or should: there
	 *  is no such thing as a placeholder banner worth shipping.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TSoftObjectPtr<UStaticMesh> GateBannerMeshAsset;

	/** Trample-path ribbon segment mesh, soft — /Game/Meshes/SM_Castle_TramplePath (TASK-657, tileable along local X, pivot at ground contact). Same two null paths as GateBannerMeshAsset: cleared = silent opt-out, unresolvable = one-line skip. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TSoftObjectPtr<UStaticMesh> TramplePathMeshAsset;

	/** Toe rock mesh 1 of 2, soft — /Game/Meshes/SM_Castle_ToeRock01 (TASK-657). The family spawns if EITHER rock mesh resolves (each anchor falls back to the one that did); both cleared = silent opt-out; set-but-neither-resolvable = one-line skip. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TSoftObjectPtr<UStaticMesh> ToeRockMeshAsset01;

	/** Toe rock mesh 2 of 2, soft — /Game/Meshes/SM_Castle_ToeRock02 (TASK-657 names it OPTIONAL, so its absence alone is never logged: odd anchors quietly fall back to 01). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Furnishing")
	TSoftObjectPtr<UStaticMesh> ToeRockMeshAsset02;

	/** Seconds between heal-over-time ticks (Masons repair, TASK-059) — impl detail, not a GDD stat. Smaller = smoother bar; the total/duration are the caller's. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle", meta = (ClampMin = "0.05"))
	float HealTickInterval = 0.2f;

	/** Castle blockout mesh (TASK-013). May not exist yet — resolved null-safe in OnConstruction. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UStaticMesh> CastleMeshAsset;

	/** Team material applied when Team == Blue (TASK-012). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialBlue;

	/** Team material applied when Team == Red (TASK-012). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialRed;

	/** HP bar widget class, /Game/UI/WBP_CastleHealthBar (TASK-019). Missing asset = silent no-op, never a crash. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftClassPtr<UUserWidget> HPBarWidgetClass;

	/**
	 *  §6 screen shake on castle hits (TASK-158): a brief camera shake on the LOCAL
	 *  player controller each time the castle takes ACTUAL damage. Default is the
	 *  READ-ONLY Variant_Combat donor BP_CameraShake_Hit_Enemy (CONVENTIONS
	 *  template-donor rule) — a BP may retarget it to a dedicated BP_CameraShake_CastleHit.
	 *  Soft, null-safe: a missing class = no shake, never a crash.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftClassPtr<UCameraShakeBase> CastleHitCameraShake;

	//~ §3.9 castle crumble stage thresholds (TASK-157) — fractions of MaxHP, fired ONCE each
	//~ on the way DOWN, in order. Play Again re-arms them (ResetCastle). // GDD §3.9 (75/50/25%)

	/** Crumble stage 1 threshold (fraction of MaxHP). // GDD §3.9 — 75% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Crumble", meta = (ClampMin = "0", ClampMax = "1"))
	float CrumbleFraction1 = 0.75f; // GDD §3.9

	/** Crumble stage 2 threshold (fraction of MaxHP). // GDD §3.9 — 50% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Crumble", meta = (ClampMin = "0", ClampMax = "1"))
	float CrumbleFraction2 = 0.50f; // GDD §3.9

	/** Crumble stage 3 threshold (fraction of MaxHP). // GDD §3.9 — 25% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Crumble", meta = (ClampMin = "0", ClampMax = "1"))
	float CrumbleFraction3 = 0.25f; // GDD §3.9

private:

	/** Loads (if available) and applies the castle mesh and the Team-appropriate material. Never crashes on missing assets. */
	void ApplyTeamVisuals();

	/** Resolves HPBarWidgetClass null-safe (missing = silent no-op), assigns it to HPBarWidget, and calls InitForCastle on the created UCastleHealthBarWidget. */
	void InitHPBarWidget();

	/**
	 *  TASK-349 team gating, both lanes, configured from THIS castle's Team at
	 *  BeginPlay (never hardcoded Blue/Red — symmetric for both castles):
	 *  GateBlockerVolume gets its tunable size/position plus the team response
	 *  matrix (object type = own channel; Ignore all, Block ONLY the enemy
	 *  channel), and InteriorNavModifier adopts the team's interior nav area.
	 *  Null-safe on both components — a missing subobject degrades to pre-feature
	 *  behavior, never a crash.
	 */
	void ConfigureTeamGating();

	/**
	 *  Spawns this castle's torches + commander NPC from the anchors and attaches
	 *  them to CastleMesh (TASK-562, WR-§4/WR-§5). Pushes this castle's Team into
	 *  the NPC via ACommanderNpc::InitCommanderNpc — ⛔ PUSHED, never derived by
	 *  the NPC (TASK-559's contract).
	 *
	 *  IDEMPOTENT BY CONSTRUCTION: it clears any existing set first, so it can be
	 *  called from BeginPlay and again from every Play-Again restore without ever
	 *  producing two sets or an orphan. Null-safe throughout — a cleared class, an
	 *  unresolvable class, an empty anchor array or a refused SpawnActor each
	 *  degrade to "that piece of furniture is absent" and the castle plays exactly
	 *  as it does today.
	 *
	 *  ⛔ NO AUTHORITY GUARD, DELIBERATELY — see the class doc. Both machines
	 *  build their own Tier-C local set.
	 */
	void SpawnCastleFurnishings();

	/** Destroys the spawned torches + commander NPC and empties the tracking arrays (the ASiegeBattlefieldScatter::ClearScatter lifecycle: destroyed, never pooled). Idempotent and null-safe. TASK-661: also destroys the F1 discoverability mesh components. */
	void DestroyCastleFurnishings();

	/**
	 *  Spawns the F1 discoverability set (TASK-661) — gate banners, trample-path
	 *  ribbon, toe rocks — as UStaticMeshComponents attached to CastleMesh at the
	 *  three anchor arrays' CASTLE-MESH-LOCAL transforms. A SIBLING of the torch/
	 *  commander spawn, called from inside SpawnCastleFurnishings so it rides the
	 *  IDENTICAL lifecycle with zero extra call sites: BeginPlay, both Play-Again
	 *  edges (ApplyDestroyedState), teardown via DestroyCastleFurnishings.
	 *  Null-safe throughout — every family soft-resolves per its property doc and
	 *  a missing asset degrades to "that dressing is absent" (TASK-657 builds the
	 *  assets in parallel and may land after this code; that ordering is the
	 *  ruling's design, not an accident). ⛔ No authority guard, deliberately —
	 *  the same Tier-C local-projection posture as the torches (see the class
	 *  doc): both machines build identical sets from the same CDO defaults.
	 */
	void SpawnDiscoverabilityFurnishings();

	/**
	 *  Creates, configures, attaches and registers ONE discoverability mesh
	 *  component at a castle-local anchor (TASK-661). ⛔ THE GH-R9 ENFORCEMENT
	 *  SITE: SetCollisionEnabled(NoCollision) + no overlap events + no nav
	 *  relevancy are forced HERE, code-side, on every component regardless of
	 *  what the soft-resolved asset ships — the entry chain's collision record
	 *  stays byte-identical even against a mis-authored asset. bCastShadow is
	 *  EXPLICIT at every call site (the trailing-defaulted-parameter law: no
	 *  default, so no caller can forget it means anything). Returns nullptr only
	 *  on a null mesh/CastleMesh; the component is tracked in
	 *  SpawnedDiscoverabilityMeshes.
	 */
	UStaticMeshComponent* SpawnDiscoverabilityMesh(UStaticMesh* Mesh, const FTransform& Anchor, bool bCastShadow);

	/** Resolves TorchClassAsset: nullptr = the deliberate cleared opt-out; unresolvable = ATorch::StaticClass() plus one log line. */
	UClass* ResolveTorchClass();

	/** Resolves CommanderNpcClassAsset: nullptr = the deliberate cleared opt-out; unresolvable = ACommanderNpc::StaticClass() plus one log line. */
	UClass* ResolveCommanderNpcClass();

	/** Single-fire destruction: guards on bDestroyed, hides the actor, disables collision, broadcasts OnCastleDestroyed. */
	void HandleDestroyed();

	/**
	 *  §3.9 castle crumble (TASK-157): advances CrumbleStage while CurrentHP has crossed
	 *  the next 75/50/25% threshold ON THE WAY DOWN — firing each stage EXACTLY once, in
	 *  order (a single big hit that crosses two thresholds fires both). Never retreats, so
	 *  a heal-back-up never un-crumbles or re-arms a passed stage (only ResetCastle re-arms).
	 *  Called from TakeDamage after CurrentHP is lowered.
	 */
	void UpdateCrumbleStages();

	/**
	 *  Applies one crumble stage (1..3): swaps the castle mesh AND/OR material to the
	 *  damaged variant (soft /Game/Meshes/SM_Castle_Crumble0N + /Game/Materials/MI_Castle_Crumble0N,
	 *  whichever resolves — null-safe, missing keeps the current look) and bursts NS_CastleDebris
	 *  at the castle. VISUAL ONLY — the crumble mesh variants must preserve the UCX footprint
	 *  (art contract) so placement/pathing are untouched.
	 *
	 *  ⭐ SLOT CONTRACT (TASK-634 slot-audit addendum, per TASK-630 §3): the material
	 *  write is SLOT 0 ONLY — slots >= 1 ([1 CastlePBR, 2 CastleInteriorPBR from
	 *  TASK-633's import]) always come from the SAVED stage asset, never from
	 *  runtime. Full record beside the write in Castle.cpp.
	 */
	void ApplyCrumbleStage(int32 Stage);

	/** Masons heal-over-time tick (TASK-059): delivers HealPerTick clamped to MaxHP, broadcasts OnCastleHPChanged on change, self-stops at MaxHP or when the pool empties. */
	void HandleHealTick();

	/** Cancels any running heal-over-time and zeroes its pool (called by HandleDestroyed and ResetCastle). Null-safe/idempotent. */
	void StopHealOverTime();

	/**
	 *  Resolves the attacking team from a damage event's instigator chain:
	 *  the instigating controller's pawn, then the damage causer itself, then
	 *  the causer's instigator pawn (projectiles, M2). Returns false if no
	 *  ITeamAgent is found (e.g. world damage), in which case damage applies.
	 */
	static bool TryGetInstigatorTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam);

	/** Current hit points. Mutated only by TakeDamage and ResetCastle (authority — M8 guards, doc §3.1). Replicated; OnRep_CurrentHP drives the client bar via the existing delegate. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 2000.0f;

	/** True after OnCastleDestroyed has fired; re-armed only by ResetCastle(). Guarantees the event fires exactly once (server). M8: replicated; OnRep_Destroyed applies the client's visual/collision state ONLY — never the win-condition broadcast (doc §3.1). */
	UPROPERTY(ReplicatedUsing = OnRep_Destroyed, VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	bool bDestroyed = false;

	/** Highest crumble stage fired so far (0 = pristine, 1/2/3 = 75/50/25% crossed). Monotonic; reset to 0 by ResetCastle. (TASK-157) M8: replicated; ApplyCrumbleStage is ABSOLUTE (addendum §3), so a join-in-progress client lands the final stage in one OnRep. */
	UPROPERTY(ReplicatedUsing = OnRep_CrumbleStage, VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	int32 CrumbleStage = 0;

	/**
	 *  CLIENT HP arrival (M8, doc §3.1): fires the EXISTING OnCastleHPChanged with
	 *  the replicated value — the same broadcast every server-side mutation site
	 *  makes, so the bar/HUD path is byte-identical (zero widget changes).
	 */
	UFUNCTION()
	void OnRep_CurrentHP();

	/**
	 *  CLIENT destroyed-state arrival (M8, doc §3.1; name per the ratified bool
	 *  law — `bDestroyed` → `OnRep_Destroyed`): applies the shared visual/
	 *  collision half (ApplyDestroyedState) for BOTH edges (destroy + Play-Again
	 *  restore). NEVER broadcasts OnCastleDestroyed — that is the server
	 *  win-condition hook; match end reaches this machine via the GameState rep
	 *  (doc §3.4).
	 */
	UFUNCTION()
	void OnRep_Destroyed();

	/**
	 *  CLIENT crumble arrival (M8, doc §3.1 + addendum §3): stage > 0 →
	 *  ApplyCrumbleStage(stage) — absolute, so intermediate stages may be skipped
	 *  safely; stage == 0 (Play-Again reset) → ApplyTeamVisuals() restores the
	 *  pristine mesh + team material (ApplyCrumbleStage guards 1..3 and can never
	 *  un-crumble — the addendum-pinned branch).
	 */
	UFUNCTION()
	void OnRep_CrumbleStage();

	/**
	 *  The shared visual/collision half of destruction (M8 refactor, doc §3.1):
	 *  hide/show the actor, disable/enable collision (the gate blocker rides the
	 *  actor state on both machines — addendum §2), hide/show the HP bar (screen-
	 *  space widgets do not follow actor hidden state — TASK-018). Called by the
	 *  server paths (HandleDestroyed / ResetCastle) AND by OnRep_Destroyed, so
	 *  both machines run the identical state change; the server-only halves
	 *  (heal-stop, sting, win broadcast, HP/crumble resets) stay in their owners.
	 *
	 *  ⛔⛔ IT ALSO OWNS THE WAR-ROOM FURNISHING LIFECYCLE (TASK-562) — destroy on
	 *  the true edge, re-spawn on the false edge — AND THAT HOOK CHOICE IS
	 *  LOAD-BEARING, NOT A CONVENIENCE. The TASK-562 spec named HandleDestroyed
	 *  and ResetCastle as the donors; both are AUTHORITY-ONLY (each opens with a
	 *  HasAuthority() refusal), while the furnishing is TIER C and exists
	 *  SEPARATELY ON EVERY MACHINE. Hooking them would therefore have leaked one
	 *  full set of torches + one commander per castle per replay ON EVERY CLIENT
	 *  — the exact "12 orphan lights per replay" hazard WR-§4 warns about,
	 *  arriving on the machine nobody was watching. This function is the ONE the
	 *  server paths and the client's OnRep_Destroyed both run, which is precisely
	 *  why the CASTLE-3X gate blocker already "rides the actor state on both
	 *  machines" from here (addendum §2) — mirroring that lifecycle, which the
	 *  spec asked for, means hooking HERE.
	 */
	void ApplyDestroyedState(bool bNowDestroyed);

	/** HP still to be delivered by the running Masons heal-over-time (0 = none). Mutated only by HealOverTime / HandleHealTick / StopHealOverTime. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float HealRemaining = 0.0f;

	/** HP delivered per heal tick, derived from Total/Duration on HealOverTime (TASK-059). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float HealPerTick = 0.0f;

	/** Repeating heal-over-time timer handle (Masons repair). Cleared by StopHealOverTime. */
	FTimerHandle HealTimerHandle;

	/**
	 *  The torch actors this castle spawned (TASK-562), in TorchAnchors order.
	 *  GC-rooted via UPROPERTY; DestroyCastleFurnishings DESTROYS them — the
	 *  ASiegeBattlefieldScatter::SpawnedMines lifecycle verbatim (destroyed,
	 *  never pooled, so a Play Again gets an exactly-fresh set). Transient: it is
	 *  runtime bookkeeping, never saved with a level or a Blueprint.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ATorch>> SpawnedTorches;

	/** The commander NPC this castle spawned (TASK-562). Same lifecycle as SpawnedTorches — exactly one per standing castle, destroyed and re-spawned rather than re-used, so the Team push re-runs on the fresh actor. */
	UPROPERTY(Transient)
	TObjectPtr<ACommanderNpc> SpawnedCommanderNpc;

	/**
	 *  The F1 discoverability mesh components this castle spawned (TASK-661), in
	 *  spawn order (banners, then the path chain, then the rocks). COMPONENTS,
	 *  not actors, on purpose: they live and die WITH this actor, so the WR-§4
	 *  orphan hazard the torch belt exists for cannot arise here at all —
	 *  DestroyCastleFurnishings tears them down on the same edges anyway so a
	 *  fallen castle sheds its signage with its torches. GC-rooted via UPROPERTY;
	 *  Transient — runtime bookkeeping, never saved.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedDiscoverabilityMeshes;

	/** One-shot guard for the "BP_Torch unresolvable, using the C++ ATorch" line (once per castle, never per spawn and never per Play Again). */
	bool bLoggedTorchClassFallback = false;

	/** One-shot guard for the "BP_CommanderNpc unresolvable, using the C++ ACommanderNpc" line. */
	bool bLoggedCommanderNpcClassFallback = false;

	/** One-shot guard for the "SM_Castle_GateBanner unavailable — family skipped" line (TASK-661; once per castle, never per Play Again — the expected state until TASK-657 lands). */
	bool bLoggedGateBannerMeshMissing = false;

	/** One-shot guard for the "SM_Castle_TramplePath unavailable — family skipped" line (TASK-661). */
	bool bLoggedTramplePathMeshMissing = false;

	/** One-shot guard for the "no toe rock mesh resolves — family skipped" line (TASK-661; fires only when at least one of the two is SET and NEITHER resolves — a cleared pair is the silent opt-out). */
	bool bLoggedToeRockMeshMissing = false;
};
