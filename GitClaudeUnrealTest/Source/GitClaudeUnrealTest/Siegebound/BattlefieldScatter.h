// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "Siegebound/TeamId.h"
#include "BattlefieldScatter.generated.h"

class AAncientGround;
class ACastle; // TASK-576 (WR-§2b row D): the castle whose LIVE colliding bounds the traversability query inset is derived from
class AGoldNode;
class ANavigationData;
class USceneComponent;
class USiegeScatterConfig;
class UHierarchicalInstancedStaticMeshComponent;
class UNavigationSystemV1;
class UStaticMesh;
struct FScatterLayer;

/** Terrain/scatter log category (CONVENTIONS "Logging"): one grep-able line per generate with the chosen seed so any layout is reproducible. */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeTerrain, Log, All);

/**
 *  Runtime procedural battlefield scatter (CONVENTIONS "Battlefield &
 *  procedural terrain (M6.5)", TASK-134). ONE instance is placed in L_Arena
 *  (named `BattlefieldScatter`, TASK-137). At match start it reads
 *  ScatterConfig (a USiegeScatterConfig DataAsset) and fills the widened arena
 *  (±25000 castles — M7.6 10× scale-up) with trees / rocks / hills / grass, re-seeded each match so
 *  every match differs (Jonathan: "randomly generated at the start of each
 *  match … use as many assets as possible for variety").
 *
 *  PERF LAW: obstacles are HISM instances (one UHierarchicalInstancedStatic
 *  MeshComponent per unique mesh) — NEVER individual actors. Blocking layers
 *  (trees/rocks/hills) block the Pawn channel and set bCanEverAffectNavigation
 *  = true so units physically block AND the navmesh carves; the grass layer is
 *  non-blocking decoration. Blocking obstacles carving the navmesh at runtime
 *  REQUIRES L_Arena's RecastNavMesh to be RuntimeGeneration = Dynamic — that is
 *  set at the level/project level by TASK-136 (build-master); without it,
 *  runtime obstacles do not affect pathing and units walk through them.
 *
 *  TRAVERSABILITY GUARANTEE (NON-NEGOTIABLE — a match where units cannot reach
 *  the enemy castle is a HARD FAILURE): GenerateScatter guarantees a navigable
 *  Blue→Red castle path EVERY match via two layers of defense:
 *    (1) DETERMINISTIC — a reserved clear central corridor (|Y| <=
 *        CorridorHalfWidth across the whole X span) plus keep-clear radii around
 *        both castles and the PlayerStart (the per-team gold-node discs died
 *        with the W1-PREP mirrored-mines redesign — mines are spawned BY this
 *        actor, are NoCollision, and clear their own aprons). Because no
 *        blocking obstacle ever lands in the corridor, the straight Y≈0 lane
 *        between the castles is ALWAYS walkable — this alone guarantees a path
 *        and does not depend on any async nav state.
 *    (2) CONFIRMATORY — a deferred post-placement reachability validation (once
 *        the async Dynamic nav reports IDLE — polled via UNavigationSystemV1::
 *        IsNavigationBeingBuilt, M7.6) that path-queries Blue-anchor →
 *        Red-anchor PLUS Blue-anchor → every spawned mine (TASK-255: a
 *        walled-in mine = economy failure); a missing castle path culls the
 *        blocking instances nearest the lane in a widening Y band, a missing
 *        mine path culls a widening clearance disc around that mine, and every
 *        defensive cull is followed by RegroundMines (a cull can delete a
 *        mine's supporting hill). Re-checks until confirmed. Never leaves a
 *        match unwinnable.
 *
 *  ⛔ SETTLED-ONLY CULL LAW (TASK-535 — CONVENTIONS `NAV-§4`; the determinism law,
 *  TASKBOARD.md:11044). Layer (2) is a query against an ASYNCHRONOUSLY BUILDING
 *  navmesh, and until TASK-535 it could both (a) print "Traversability CONFIRMED"
 *  while tile tasks were still queued — measured: the MaxNavSettleWait cap fires,
 *  logs "proceeding anyway", and the very next line claimed CONFIRMED — and (b)
 *  DELETE INSTANCES on the strength of that unsettled answer. (b) is the real
 *  defect: a cull decided by a partially-built navmesh is a cull decided by WALL-
 *  CLOCK TIMING, so the shipped instance set stopped being a pure function of the
 *  seed. Three rules now bind this actor:
 *    (i)   HONEST LABELLING. ValidateTraversability reads
 *          UNavigationSystemV1::GetNumRemainingBuildTasks() at the query and
 *          prints EITHER "CONFIRMED (nav settled: 0 pending)" OR "PROVISIONAL
 *          (N tile task(s) pending — PRE-SETTLE query)". ⛔ A pre-settle query may
 *          NEVER print the word CONFIRMED — that string is a claim.
 *    (ii)  ⛔ A CULL MAY ONLY BE DRIVEN BY A SETTLED QUERY. A PROVISIONAL failure
 *          culls NOTHING (bCullOnProvisionalFailure, EditDefaultsOnly, default
 *          false — the escape hatch exists only so the old behaviour is one
 *          checkbox away, never as the shipped default).
 *    (iii) THE DEFINITIVE CHECK IS EVENT-DRIVEN. This actor binds
 *          UNavigationSystemV1::OnNavigationGenerationFinishedDelegate
 *          (NavigationSystem.h:444) and re-runs the reachability check ONCE, when
 *          generation ACTUALLY finishes — one bind, one deferred re-check per
 *          match. ⛔ No polling, ⛔ no 216 s stall, unbound in EndPlay.
 *  ⭐ AND WHY THIS *KEEPS* THE NON-NEGOTIABLE GUARANTEE RATHER THAN WEAKENING IT:
 *  post-settle, the reachability answer is STABLE, so a cull driven by it is a pure
 *  function of the geometry — hence of the seed. The cull is NOT removed; it is
 *  MOVED to the only moment at which it is both TRUE and DETERMINISTIC. Layer (1),
 *  the reserved corridor, is unchanged and still guarantees the lane geometrically
 *  with no dependence on any async nav state.
 *  🚩 ACCEPTED, FLAGGED CONSEQUENCE (Jonathan's call at the playtest, TASK-539):
 *  a post-settle cull deletes instances LATER — potentially VISIBLY (a rock popping
 *  out ~27 s in at 8× tile concurrency, or ~216 s if that flip rolls back) where it
 *  used to happen invisibly at +5 s. It only ever fires when the field is genuinely
 *  walled off — the HARD-FAILURE case — and a visible pop is strictly better than an
 *  unwinnable match. ⛔ Do NOT "fix" it by restoring the provisional cull.
 *
 *  ⚖️ M8 DECLARATION (TASK-535): adds no replicated property, no new replicated
 *  class, no new relevancy tier. Everything TASK-535 adds is server-side validation
 *  state on an actor that already replicates only its seed pair; the client path
 *  (OnRep_GenerationIndex) never validates, never culls and never binds the delegate.
 *
 *  ⚠️ 180°-ROTATIONAL SYMMETRY LAW (TASK-358 — CONVENTIONS "Ancient Grounds +
 *  Sorcerer + 180° terrain symmetry" §1; SUPERSEDES the M6.5 asymmetric-organic
 *  ruling AND the retired bMirrorSymmetric X-mirror). EVERY terrain pass in this
 *  actor generates on the BLUE half (X <= 0) and emits each placement's twin
 *  under a proper rigid rotation about the map center:
 *        loc' = (−X, −Y, Z)   ·   yaw' = Fmod(yaw + 180, 360)   ·   scale' = scale
 *  On yaw-only / uniform-scale transforms that is EXACT — no negative scale, no
 *  HISM winding flip, no approximation. THREE rules bind every pass here and any
 *  pass added later:
 *    (1) BLUE, NEVER RED. PlayerStart exists ONLY at (−23800, 0, 100); there is
 *        no Red-side PlayerStart, so generating Red would rotate a legally-placed
 *        prop straight ONTO the hero spawn.
 *    (2) ZERO RNG DRAWS IN THE ROTATION STEP. The twin is COMPUTED, never
 *        sampled — this is what preserves intra-build determinism and host ==
 *        client. Same binary + same seed ⇒ identical layout and identical logs.
 *    (3) EMIT THE TWIN INLINE, never as a bulk post-pass (it would break
 *        VisualToProxy index parallelism, SpacingGrid registration and
 *        HillSurfaceComponents registration).
 *  Cross-BUILD layout stability is explicitly NOT a contract: existing seeds
 *  produce new layouts under this law, exactly as the TASK-140 draw-order change
 *  did. Deliberate residual asymmetries (each LOGGED when it fires): the
 *  ValidateTraversability destructive culls, and the one-real/one-rotated-over-
 *  clear PlayerStart keep-clear disc.
 *
 *  ROTATED DEPLETING MINES (W1-PREP, TASK-255 — CONVENTIONS "Mirrored
 *  depleting mines", AMENDED by the law above): after the two layer passes,
 *  PlaceMines spawns MineCountPerSide AGoldNode PAIRS — each drawn once on the
 *  Blue half then rotated 180° to (−X, −Y, yaw 180) (TASK-358; the twin yaw was
 *  ALREADY 180, so only Y changed), so castle-distance sums are equal by
 *  construction. The pass draws from a DEDICATED FRandomStream(Seed XOR
 *  0x4D494E45), exactly two draws per attempt in fixed X-then-Y order. Mines are
 *  corridor-legal by Jonathan ruling (AGoldNode is NoCollision — the
 *  traversability guarantee is untouched) and delete the nav-relevant blockers in
 *  their clearance discs at BOTH ends (hills EXEMPT — hills are never deleted).
 *  The old hill-parity clone/rollback machinery is DELETED (TASK-358): under a
 *  true rotation the twin lands on the geometrically identical point of the
 *  rotated hill — same Z, same slope, always — so parity holds by construction.
 *
 *  ANCIENT GROUNDS (batch ANCIENT-GROUNDS, TASK-361 — CONVENTIONS §2
 *  "Placement"): immediately after the mines, PlaceAncientGrounds spawns ONE
 *  AAncientGround on the Blue half plus its 180° rotational twin — the objective
 *  Jonathan asked for ("one per side placed symmetrically"), delivered as a
 *  SCATTER PASS rather than a level actor so it is random per match with NO
 *  L_Arena save, agrees on host and client via the replicated seed, re-places
 *  itself for free on Play Again, and reuses the clearance-cull + hill-trace
 *  machinery already here. It too draws from its OWN FRandomStream (Seed XOR
 *  0x41474E44, "AGND"), exactly two draws per attempt in fixed X-then-Y order.
 *  ⚠️ The pass THREADS bAuthoritativeGenerate into AAncientGround::
 *  InitAncientGround — the ground is spawned locally on the CLIENT too and so
 *  keeps ROLE_Authority there; it must never read authority for itself.
 *
 *  Everything is null-safe: no config, no meshes, or no nav system each degrade
 *  to a logged no-op / geometric-only guarantee — never a crash, never a
 *  combat/stat change.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeBattlefieldScatter : public AActor
{
	GENERATED_BODY()

public:

	ASiegeBattlefieldScatter();

	/**
	 *  ⚖️ NET RELEVANCY TIER: **A — `bAlwaysRelevant = true`** (declared per the
	 *  CONVENTIONS NET RELEVANCY LAW declaration duty; set in the constructor).
	 *  Rationale: a SINGLETON whose replicated seed DRIVES WORLD GENERATION on
	 *  every machine, and a point actor at the world origin — under the engine's
	 *  default 150 m relevancy it was permanently irrelevant to players ~250 m
	 *  away, so the client generated NOTHING (TASK-357: 0 gold nodes vs 6).
	 *  Tier A is also the PRECONDITION of the signed doc's D9 superset argument.
	 */

	/** Registers the M8 P1 seed set — ChosenSeed (plain) + GenerationIndex (OnRep) — doc D9/§3.5 (TASK-356). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 *  AUTHORITY entry (M8, TASK-356 doc §3.5): clears any existing instances,
	 *  picks a seed (OverrideSeed>0 else a random
	 *  seed, LOGGED on LogSiegeTerrain so the layout is reproducible), scatters
	 *  every layer honoring keep-clear + spacing + the reserved corridor, wires
	 *  blocking/nav on obstacle layers, then schedules the deferred
	 *  reachability validation. Graceful no-op when ScatterConfig is unset.
	 *  A CLIENT copy refuses (its per-machine random seed IS the audit's
	 *  different-battlefields bug) and instead regenerates deterministically via
	 *  OnRep_GenerationIndex with the replicated ChosenSeed. Standalone:
	 *  authority ⇒ byte-identical.
	 *  W1-PREP (TASK-250): layers place in TWO passes — bAllowOnHills=false
	 *  first (hills included, registering the hill-surface HISMs), then the
	 *  hill-allowed layers, whose ground resolve accepts elevated hill Z within
	 *  each layer's MaxPlacementSlopeDeg. W1-PREP (TASK-255): PlaceMines runs
	 *  after pass 2 and before the nav-settle poll (injected twin hills +
	 *  clearance culls must carve nav ahead of the reachability validation).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Terrain")
	void GenerateScatter();

	/** Removes every scattered instance from every HISM (the components persist for reuse) and DESTROYS the spawned mine pair actors (Play-Again lifecycle, TASK-255 — fresh mines every re-scatter) AND the spawned ancient-ground pair (TASK-361 — the identical lifecycle: destroyed, never pooled, so every re-scatter gets a fresh pair with a fresh authority push). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Terrain")
	void ClearScatter();

	/** Re-randomize on Play Again (SiegeGameMode's reset path drives this). When false, ClearScatter+GenerateScatter re-uses the last seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siegebound|Terrain")
	bool bReRandomizeOnMatchReset = true;

	// ═══════════════════════════════════════════════════════════════════════════
	// THE FOLIAGE QUALITY LEVER — CULL BAND ONLY (TASK-1122, GFX-§9)
	// ═══════════════════════════════════════════════════════════════════════════
	//
	// ⛔⛔ THE ONE SENTENCE THAT GOVERNS EVERYTHING BELOW: this lever may move what
	// is DRAWN and may NEVER move what is PLACED. A quality level is PER-MACHINE
	// (GFX-§3); the layout is a REPLICATED CONTRACT (M8/D9). The client never
	// self-generates (BeginPlay:230) — it mirrors the authority's seed through
	// OnRep_GenerationIndex and must reproduce the SAME field. `InstanceCount` /
	// `OuterTarget` drive the iteration count of a loop that draws from a SHARED
	// FRandomStream, so thinning instances would shift the stream and desync EVERY
	// SUBSEQUENT LAYER, not merely the thinned one — and it would make a low-spec
	// client's obstacle set a SUBSET where the authority-only traversability
	// residual (BattlefieldScatter.cpp:339-340) was signed off on it being a
	// SUPERSET. ⛔ DENSITY IS STRUCK. Cull distances are render-side state applied
	// AFTER placement and consume ZERO RNG ⇒ per-client-safe BY CONSTRUCTION.
	//
	// ⛔⛔ AND THE SECOND TRAP, WHICH IS WHY A BARE `Band × Scale` IS REFUSED:
	// USiegeGraphicsSettingsSubsystem's ladder (0.25 / 0.50 / 0.75 / 1.00 / 1.00)
	// was authored for the STRUCK density lever, where it is a COUNT multiplier.
	// A count multiplier applied to a DISTANCE is wrong by a square: drawn
	// instances scale with the AREA of the annulus, ≈ end², so a naive
	// `0.25 × band` would draw 0.0625 of them — SIXTEEN TIMES the intended cut —
	// and would pull the grass band from 90 m to 22.5 m, i.e. grass materialising
	// in the player's lap. That reads as a BROKEN GAME at Low, not a scaled one.
	// ⇒ the translation is `factor = sqrt(quality)`, which reproduces EXACTLY the
	// drawn-instance reduction the ladder was designed to deliver, through the one
	// mechanism that is determinism-safe. See FoliageQualityScaleToCullDistanceFactor.

	/**
	 *  ⛔ THE NEAR-FIELD FLOOR, IN UNREAL UNITS (1 uu = 1 cm) — a cull end may
	 *  never be pulled below this by the quality lever. 3,500 uu = 35 m, and that
	 *  number is MEASURED rather than picked: the castle keep-clear disc
	 *  (CastleKeepClearRadius 4,500 uu, DA-serialised) puts the nearest possible
	 *  scattered tuft ≈35 m from the hero's spawn (`handoffs/TASK-1084-buildmaster.md`
	 *  §0b item 3). A cull end shorter than that renders the layer INVISIBLE from
	 *  spawn — the lever would be switching the layer OFF, and a quality level that
	 *  turns a layer off is the "control that lies" GFX-§9 exists to forbid.
	 *
	 *  ⚠️ HONEST SCOPE: with today's authored bands this floor NEVER BINDS (the
	 *  shortest applied end is grass at Low, 45 m). It is a bound on FUTURE
	 *  DA_BattlefieldScatter edits — the DA is content and can change without a
	 *  code review — and it is exercised by a synthetic layer in the tests, not by
	 *  any shipped layer. Stated so nobody reads its presence as evidence it fired.
	 */
	static constexpr int32 FoliageCullNearFieldFloorUU = 3500;

	/**
	 *  Translates the graphics facade's Foliage QUALITY scalar (a normalized
	 *  0..1 value; the shipped ladder is 0.25 / 0.50 / 0.75 / 1.00 / 1.00 for
	 *  Low..Cinematic) into a CULL-DISTANCE factor.
	 *
	 *  ⛔ `sqrt`, NOT identity. The scalar is a COUNT scalar by origin and cull
	 *  cost scales with the AREA the band covers (≈ end²), so `sqrt(q)` is the
	 *  distance that draws exactly `q` of the instances. Ruled mapping, in metres,
	 *  against the MEASURED authored bands (engine read-backs recorded in
	 *  `handoffs/TASK-1083-buildmaster.md` §Trees and `TASK-1084-buildmaster.md` §0b):
	 *
	 *      level        q      factor   Trees 240/320 m   Grass 60/90 m   Plants 80/120 m   drawn
	 *      Low       0.25      0.500     120 / 160 m       30 / 45 m       40 /  60 m        25 %
	 *      Medium    0.50      0.707     170 / 226 m       42 / 64 m       57 /  85 m        50 %
	 *      High      0.75      0.866     208 / 277 m       52 / 78 m       69 / 104 m        75 %
	 *      Epic      1.00      1.000     240 / 320 m       60 / 90 m       80 / 120 m       100 %
	 *      Cinematic 1.00      1.000     ⛔ IDENTICAL TO EPIC — never above the authored baseline
	 *
	 *  ⚠️ Those are the AUTHORED metres handed to SetCullDistances. The engine's
	 *  own ViewDistance group multiplies `r.ViewDistanceScale` (0.4 @Low → 1.0
	 *  @Epic) on top at draw time, and GFX-§9 STRUCK any compensation for it — so
	 *  Foliage=Low + ViewDistance=Low compounds to ≈18 m of grass. That is two
	 *  honestly-labelled Low sliders doing what they say; it is flagged for the
	 *  gate rather than silently corrected.
	 *
	 *  Pure: no world, no UObject, no allocation, no RNG, no clock (the
	 *  HIGH-§3 / `HeightAdvantageMultiplier` testability idiom).
	 */
	static float FoliageQualityScaleToCullDistanceFactor(float FoliageQualityScale);

	/**
	 *  Applies the Foliage quality factor to ONE layer's authored cull band and
	 *  returns the pair to hand to UInstancedStaticMeshComponent::SetCullDistances.
	 *
	 *  Guarantees, each of them a named test in `Tests/SiegeScatterCullBandTest.cpp`:
	 *   (1) ⛔ `AuthoredEnd == 0` (the engine's NEVER-CULLED sentinel, which the
	 *       HILLS layer rides so its silhouette reads across the 10× field) comes
	 *       back UNTOUCHED. A multiply would turn "never culled" into "culled at
	 *       zero distance", i.e. a layer that renders nowhere.
	 *   (2) ⛔ Scale >= 1.0 (Epic, Cinematic, AND the facade's null fallback)
	 *       returns today's EXACT pair — `Max(Start,0)`, `Max(End,0)` — with no
	 *       arithmetic performed at all, so the default battlefield cannot regress
	 *       by so much as a rounding step. A regression at Epic is a FAIL.
	 *   (3) The band never goes negative and never collapses to zero (the floor
	 *       above, itself capped by the authored end so the lever can only ever
	 *       SHORTEN a band, never lengthen one).
	 *   (4) An authored HARD POP (`Start >= End`, ScatterConfig.h:252-255) stays a
	 *       hard pop — the semantics are preserved, not just the numbers.
	 *
	 *  Pure, exactly like the factor above.
	 */
	static void ComputeFoliageScaledCullBand(
		int32 AuthoredStartUU,
		int32 AuthoredEndUU,
		float FoliageQualityScale,
		int32& OutStartUU,
		int32& OutEndUU);

protected:

	/** Scatters once at match start. */
	virtual void BeginPlay() override;

	/** Stops the pending traversability timers AND unbinds the nav-generation-finished delegate (TASK-535 — a bind that outlives the actor is a dangling callback into a destroyed world). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  The designer config (USiegeScatterConfig DataAsset — instance
	 *  /Game/Data/DA_BattlefieldScatter, TASK-137). Unset ⇒ GenerateScatter is a
	 *  logged no-op. EditDefaultsOnly so the level instance points at the asset.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain")
	TObjectPtr<USiegeScatterConfig> ScatterConfig;

	/**
	 *  Explicit RNG seed: >0 forces a fixed, reproducible layout; 0 (default)
	 *  picks a fresh random seed every GenerateScatter so each match differs.
	 *  The chosen seed is always logged on LogSiegeTerrain.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Terrain")
	int32 OverrideSeed = 0;

private:

	/** Root so the runtime HISMs have a stable attach parent. */
	UPROPERTY(VisibleAnywhere, Category = "Siegebound|Terrain")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 *  One HISM per unique scattered mesh (perf law). GC-tracked; reused across
	 *  re-scatter (ClearScatter clears instances but keeps the components). Keyed
	 *  by mesh in a transient map rebuilt each generate.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> ScatterComponents;

	/**
	 *  Visual-HISM → paired proxy-HISM index (tree collision-proxy contract). For a
	 *  layer with CollisionProxyMesh set, the visual HISM renders with NO collision
	 *  and the invisible proxy HISM (Pawn-block-only) carries the blocking + nav.
	 *  BOTH are also stored in ScatterComponents (a UPROPERTY), which is what roots
	 *  them for GC and what ClearScatter iterates — this map is only a secondary
	 *  index (raw pointers are safe: every key/value outlives the map, all torn down
	 *  together on actor destroy). Its job: keep the pair matched so
	 *  CullCorridorBlockers removes the SAME instance indices from BOTH in lockstep
	 *  (else culling the nav-relevant proxy orphans a visible tree with no collider).
	 */
	TMap<UHierarchicalInstancedStaticMeshComponent*, UHierarchicalInstancedStaticMeshComponent*> VisualToProxy;

	/**
	 *  HISMs that count as HILL SURFACE for bAllowOnHills layers (W1-PREP,
	 *  TASK-250): the real-geometry blocker HISMs (bBlocking, no collision proxy,
	 *  not themselves hill-allowed) registered by pass 1 of GenerateScatter —
	 *  in the shipped DA, the HILLS layer. Reset + rebuilt every generate. Like
	 *  VisualToProxy, this is a secondary index over comps already rooted via
	 *  ScatterComponents (a UPROPERTY), so raw pointers are safe: every entry
	 *  outlives the array and all are torn down together on actor destroy.
	 */
	TArray<UHierarchicalInstancedStaticMeshComponent*> HillSurfaceComponents;

	/**
	 *  The mine pair actors spawned by PlaceMines this generate (W1-PREP,
	 *  TASK-255) — primaries and twins interleaved [P0, M0, P1, M1, ...].
	 *  GC-rooted via UPROPERTY; ClearScatter DESTROYS them (Play-Again
	 *  lifecycle: exactly-fresh mines every re-scatter; PlayAgain kills miners
	 *  in its step 2 BEFORE the step-7 re-scatter per the plan-of-record, so no
	 *  dangling miner registries — AGoldNode::EndPlay clears its own drain
	 *  timer and deliberately skips miner notification, TASK-253).
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGoldNode>> SpawnedMines;

	/**
	 *  The ancient-ground pair spawned by PlaceAncientGrounds this generate
	 *  (TASK-361) — [P, P′], primary first. GC-rooted via UPROPERTY; ClearScatter
	 *  DESTROYS them, EXACTLY like SpawnedMines (Play-Again lifecycle: a fresh
	 *  pair at a fresh location every re-scatter). Destroying rather than pooling
	 *  also re-runs the authority push on the new pair, so a re-scatter can never
	 *  leave a stale bAuthoritativeBoost behind. AAncientGround latches no state
	 *  and clears its own boost timer in EndPlay, so nothing else needs resetting.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AAncientGround>> SpawnedAncientGrounds;

	/** Resolves-or-creates the VISUAL HISM for a mesh, applying the layer's collision/nav profile. Returns nullptr if the mesh is unresolvable. */
	UHierarchicalInstancedStaticMeshComponent* ResolveComponentForMesh(UStaticMesh* Mesh, const FScatterLayer& Layer);

	/**
	 *  Resolves-or-creates the invisible Pawn-block-only PROXY HISM paired to a
	 *  visual HISM (tree collision-proxy contract, CONVENTIONS "Climbable terrain
	 *  (M6.6)"). Exactly ONE proxy per visual HISM so the visual/proxy instance
	 *  indices stay parallel (CullCorridorBlockers removes both in lockstep).
	 *  Returns nullptr if the layer's CollisionProxyMesh is unset/unresolvable.
	 */
	UHierarchicalInstancedStaticMeshComponent* ResolveProxyForVisual(UHierarchicalInstancedStaticMeshComponent* VisualComp, const FScatterLayer& Layer);

	/** Reverse lookup: the visual HISM paired to a proxy HISM (or nullptr for a real-geometry blocker). Lets CullCorridorBlockers cull the visual in lockstep with its proxy. */
	UHierarchicalInstancedStaticMeshComponent* FindVisualForProxy(UHierarchicalInstancedStaticMeshComponent* ProxyComp) const;

	/**
	 *  Scatters a single layer's instances via the shared FRandomStream, honoring
	 *  the config's EScatterSymmetryMode (TASK-358). Under the default
	 *  Rotational180 the primary X draw is narrowed to the BLUE half [−HalfX, 0]
	 *  and every accepted instance emits its (−X, −Y, yaw+180) twin INLINE — same
	 *  HISM, same scale, in lockstep with its collision proxy and registered in the
	 *  SpacingGrid. The per-layer InstanceCount therefore counts primary + twin
	 *  (target 340 ⇒ 170 pairs), so the AddInstance budget is unchanged. ZERO extra
	 *  FRandomStream draws: only the primary X draw's RANGE moved.
	 */
	void ScatterLayer(const FScatterLayer& Layer, FRandomStream& Stream);

	/**
	 *  True if a 2D point — inflated by InstanceRadius — is inside any keep-clear
	 *  zone (castle/PlayerStart radius) or the reserved corridor band. The
	 *  radius makes a WIDE instance's EDGE (not just its center) count, so a broad
	 *  hill centered just off-lane no longer sprawls into the corridor. Built fresh
	 *  each generate from the live level actors.
	 */
	bool IsInKeepClear(const FVector2D& Point2D, float InstanceRadius = 0.f) const;

	/**
	 *  Keep-clear DISC test ONLY — no corridor band (TASK-255): the mines pass
	 *  uses this because corridor mines are ALLOWED by Jonathan ruling
	 *  (high-risk gold; AGoldNode is NoCollision, so a lane mine cannot break
	 *  the traversability guarantee). IsInKeepClear layers the corridor test on
	 *  top of this for the blocking layer passes.
	 */
	bool IsInKeepClearDiscs(const FVector2D& Point2D, float InstanceRadius) const;

	/** Rebuilds KeepClearZones + corridor half-width from the live level (castles/PlayerStart — the gold-node discs died with the W1-PREP mines redesign, TASK-255) with CONVENTIONS-coordinate fallbacks. */
	void RebuildKeepClearZones();

	/** Traces down to the arena floor at (X,Y); returns the floor Z (or 0 if no hit). Ignores this actor so already-placed instances never fool the trace. */
	float GroundZAt(float X, float Y) const;

	/**
	 *  Hill-aware ground resolve for bAllowOnHills layers (W1-PREP, TASK-250).
	 *  Component-scoped down-traces against ONLY the registered hill-surface
	 *  HISMs (world channel traces can never see them: the hill HISMs ignore the
	 *  ECC_WorldStatic trace channel by the scatter-channel law AND belong to this
	 *  actor, which GroundZAt ignores wholesale — the bare-hills root cause).
	 *  Returns true with OutZ = the hill-surface Z when the candidate sits over a
	 *  hill face within MaxSlopeDeg, true with OutZ = FloorZ when it is not over
	 *  a hill at all, and FALSE when the face is steeper than MaxSlopeDeg — the
	 *  caller must REJECT that candidate (grounding at FloorZ would bury it
	 *  inside the hill).
	 */
	bool ResolveHillAwareGroundZ(float X, float Y, float FloorZ, float MaxSlopeDeg, float& OutZ) const;

	/**
	 *  TASK-255 extension of the TASK-250 hill resolve: the SAME component-scoped
	 *  down-trace + slope gate as ResolveHillAwareGroundZ (which now delegates
	 *  here — behavior for the layer passes is unchanged), additionally surfacing
	 *  the hit hill-surface COMPONENT + INSTANCE index. OutSurfaceComp/
	 *  OutInstanceIndex are set only when the point is over a hill face within
	 *  MaxSlopeDeg (else nullptr / INDEX_NONE with OutZ = FloorZ); returns FALSE
	 *  when the face is steeper — the caller must reject that candidate. Draw-free
	 *  (the determinism law: zero FRandomStream draws anywhere in the hill / trace
	 *  / clearance paths).
	 *  TASK-358 note: OutSurfaceComp is still LIVE (the mines pass reads it as the
	 *  is-this-point-on-a-hill predicate, and TASK-361's ancient-ground pass uses
	 *  it to REJECT hills outright). OutInstanceIndex is now INFORMATIONAL only —
	 *  its one consumer was the deleted hill-parity clone — but it is kept in the
	 *  signature deliberately: it costs nothing and is the natural hook for any
	 *  future per-instance hill query.
	 */
	bool FindHillSurfaceAt(float X, float Y, float FloorZ, float MaxSlopeDeg, float& OutZ,
		UHierarchicalInstancedStaticMeshComponent*& OutSurfaceComp, int32& OutInstanceIndex) const;

	/**
	 *  W1-PREP depleting mines pass (TASK-255 — CONVENTIONS "Mirrored depleting
	 *  mines"; Jonathan's locked rulings — AMENDED by the 180°-rotational law,
	 *  TASK-358). Called by GenerateScatter AFTER the two layer passes and BEFORE
	 *  StartNavSettlePoll. Draws EXCLUSIVELY from a dedicated FRandomStream(Seed
	 *  XOR 0x4D494E45) — the seed-order law: the layer stream gains ZERO draws —
	 *  exactly two draws per attempt in fixed X-then-Y order; every trace /
	 *  rotation / clearance step downstream is draw-free. Per mine: half-draw on
	 *  the Blue half (|X| ≥ max(MineClearanceRadius, MineMinSpacing/2)), spacing vs
	 *  prior primaries, keep-clear discs tested at BOTH P and P′ = (−X, −Y) (the
	 *  zone SET is asymmetric — PlayerStart is Blue-side — so testing the twin is
	 *  not redundant), NO corridor test (ruling), slope-gated grounding at both
	 *  points (≤ MineMaxSlopeDeg), clearance-delete at both points, then the
	 *  tracked AGoldNode pair spawn + InitMine(MineGoldReserve) with the twin at
	 *  (−X, −Y) yaw 180. ≤ 2 × MaxPlacementAttemptsPerInstance attempts, then the
	 *  DETERMINISTIC fallback slot with an Error log — the economy never ships
	 *  short. Ends with the one grep-able MinesPass reproducibility line (seed +
	 *  pairs + hill/fb flags): same seed ⇒ identical line, the TASK-258
	 *  determinism criterion.
	 *  ⚠️ TASK-358: the hill-parity INJECTION (clone the supporting hill onto the
	 *  bare side, re-trace, roll back + reject on no-fit) is DELETED as provably
	 *  unreachable — under a true 180° rotation P′ lands on the geometrically
	 *  identical point of the rotated hill (Z and the normal's Z component are both
	 *  invariant under a Z-axis rotation), so parity holds by construction. The
	 *  MinesPass log's `inj=` token went with it.
	 */
	void PlaceMines(int32 Seed);

	/**
	 *  ANCIENT GROUNDS pass (batch ANCIENT-GROUNDS, TASK-361 — CONVENTIONS
	 *  "Ancient Grounds + Sorcerer + 180° terrain symmetry" §2 "Placement", plan
	 *  §2). Called by RunScatterPasses IMMEDIATELY AFTER PlaceMines — after,
	 *  because the mine set must exist to be kept clear of; and inside the shared
	 *  seed-deterministic body, so OnRep_GenerationIndex re-runs it identically on
	 *  the client. Places ONE AAncientGround on the BLUE half plus its 180°
	 *  ROTATIONAL TWIN at (−X, −Y) yaw 180 — the §1 law, conformed to, not a new
	 *  exception.
	 *
	 *  ⚠️ THE bAuthoritativeGenerate PARAMETER IS THE POINT OF THIS SIGNATURE.
	 *  AAncientGround runs a boost simulation and MUST NOT read HasAuthority()
	 *  itself: it is spawned LOCALLY ON THE CLIENT from the replicated seed and
	 *  therefore keeps ROLE_Authority there, so a self-read would silently run a
	 *  rogue client-side sim. This pass PUSHES RunScatterPasses' own flag into
	 *  InitAncientGround(bool) on BOTH spawned grounds — the same threading shape
	 *  as RunScatterPasses(Seed, bAuthoritativeGenerate) itself. (CONVENTIONS §2
	 *  writes the pass as `PlaceAncientGrounds(int32 Seed)`; the second parameter
	 *  is the only honest way to satisfy the authority-is-pushed law that the SAME
	 *  clause states, and this is a private method no other task calls.)
	 *
	 *  Draws EXCLUSIVELY from a dedicated FRandomStream(Seed XOR 0x41474E44 =
	 *  "AGND") — the seed-order law: the layer stream and the mine stream each
	 *  gain ZERO draws — exactly two draws per attempt in fixed X-then-Y order,
	 *  with every downstream step (mine clearance, hill rejection, ground traces,
	 *  the rotation, the clearance culls, the fallback) DRAW-FREE, so no rejection
	 *  path can desync the sequence. Per attempt: |X| in [AncientGroundMinAbsX,
	 *  AncientGroundMaxAbsX] negated onto the Blue half, |Y| <=
	 *  AncientGroundMaxAbsY, AncientGroundMineClear from every spawned mine tested
	 *  at BOTH P and P′, and FLAT GROUND ONLY — a candidate over a hill face is
	 *  REJECTED OUTRIGHT (a 1,680² gathering box needs flat ground; the hill is
	 *  never parity-cloned for it). NO corridor test, by the same ruling as the
	 *  mines: AAncientGround has no collision primitive and no nav geometry, so it
	 *  cannot touch the traversability guarantee. 48 attempts, then a
	 *  DETERMINISTIC fallback at (−12000, +6000) logged at Error — the mines'
	 *  "never ships short" discipline, because a match with no ancient ground has
	 *  no Sorcerer mechanic at all. Ends with the one grep-able AncientGroundsPass
	 *  reproducibility line: same binary + same seed ⇒ identical line on host and
	 *  client.
	 */
	void PlaceAncientGrounds(int32 Seed, bool bAuthoritativeGenerate);

	/**
	 *  Re-seats every spawned mine on the CURRENT surface under it — hill else
	 *  floor, ANY slope (90° gate: a placed mine must re-seat on whatever
	 *  remains, never float mid-air because a face reads steep). Called after
	 *  EVERY defensive cull (TASK-255 law): a widening cull can delete a mine's
	 *  supporting hill — CullCorridorBlockers does not exempt hill comps.
	 *  Draw-free.
	 */
	void RegroundMines();

	/**
	 *  Deferred (async-nav-settled) reachability confirmation — path-queries
	 *  Blue→Red AND Blue→each mine (TASK-255 economy guarantee); defensively culls
	 *  corridor blockers / per-mine clearance discs and regrounds mines after every
	 *  cull.
	 *  TASK-535 (`NAV-§4`): the verdict is now LABELLED by the live tile-task queue
	 *  (GetNumRemainingBuildTasks) — CONFIRMED only when the queue is empty,
	 *  PROVISIONAL otherwise — and ⛔ the defensive cull runs ONLY on a SETTLED
	 *  query (unless bCullOnProvisionalFailure is flipped). A PROVISIONAL failure
	 *  logs, culls nothing, arms nothing, and hands the verdict to the
	 *  event-driven definitive re-check. Idempotent and re-entrancy-safe: callable
	 *  from the settle poll, the retry path and the nav-generation-finished
	 *  callback without any of them stacking.
	 */
	void ValidateTraversability();

	/**
	 *  Binds OnNavGenerationFinished to the live nav system's
	 *  OnNavigationGenerationFinishedDelegate (NavigationSystem.h:444) — ONCE, and
	 *  AUTHORITY-ONLY (the validation it drives is authority-only, M8 doc D9).
	 *  Idempotent: a second call while already bound is a no-op, so the Play-Again
	 *  re-scatter cannot stack a second callback. Null-safe when there is no nav
	 *  system (the reserved corridor is still the deterministic guarantee).
	 */
	void BindNavGenerationFinished();

	/** Removes the binding from the EXACT nav system it was taken on (a weak ptr, so a torn-down nav system is simply forgotten). Idempotent; called by EndPlay and by every terminal verdict. */
	void UnbindNavGenerationFinished();

	/**
	 *  ⭐ THE DEFINITIVE CHECK (`NAV-§4`): the navmesh generator has just drained its
	 *  tile-task queue, which is the ONLY moment at which the reachability answer is
	 *  both TRUE and DETERMINISTIC. Schedules ONE deferred ValidateTraversability
	 *  and latches, so repeated broadcasts (one per ANavigationData, plus any later
	 *  drain) cannot turn this into a poll.
	 *  ⚠️ IT DEFERS BY DESIGN, IT DOES NOT VALIDATE INLINE: this fires from INSIDE
	 *  the Recast generator's tick (RecastNavMeshGenerator.cpp:7631 →
	 *  ARecastNavMesh::OnNavMeshGenerationFinished → NavigationSystem.cpp:4915), and
	 *  a cull re-entering the nav system to dirty areas from inside its own generator
	 *  tick is exactly the re-entrancy the spec forbids. One timer tick of latency
	 *  costs nothing against a check that was previously wrong by ~211 s.
	 */
	UFUNCTION()
	void OnNavGenerationFinished(ANavigationData* NavData);

	/** Timer body for the deferred definitive re-check: clears the pending latch, then runs the ordinary ValidateTraversability (same code path, same rules — the only difference is that the navmesh has actually settled). */
	void RunDefinitiveTraversabilityCheck();

	/**
	 *  Starts the nav-settle wait for the deferred reachability validation (M7.6 —
	 *  replaces the old fixed NavSettleDelay 0.75 s guess): arms PollNavSettle on
	 *  the shared TraversabilityTimerHandle every NavPollInterval until the async
	 *  Dynamic nav reports idle. When NO nav system exists to poll, falls back to
	 *  a single fixed NavSettleFallbackDelay wait (ValidateTraversability itself
	 *  degrades gracefully without nav). Shared by GenerateScatter and the
	 *  widening-cull retry path.
	 */
	void StartNavSettlePoll();

	/** One poll step: nav still building (and under MaxNavSettleWait) ⇒ re-arm; idle or capped ⇒ run ValidateTraversability (a hit cap logs a proceed-with-warning, never strands the guarantee). */
	void PollNavSettle();

	/** Culls blocking instances whose |Y| <= Band across the X span (widening re-roll used only if a path is somehow not found). Returns the number removed. */
	int32 CullCorridorBlockers(float Band);

	/**
	 *  DISC sibling of CullCorridorBlockers (TASK-255): removes every
	 *  NAV-RELEVANT blocking instance whose center lies within Radius of Center
	 *  — real-geometry blockers and tree PROXY comps, with the paired visual
	 *  HISM culled in LOCKSTEP via VisualToProxy (same law as the corridor
	 *  cull). Grass is untouched (not nav-relevant, excluded by the same guard)
	 *  and HILL-SURFACE comps are EXEMPT (hills are never deleted — Jonathan
	 *  conflict rule). Center-in-disc semantics, matching the corridor cull's
	 *  center-in-band. Returns the number removed.
	 *  TASK-358 SYMMETRY: this call is symmetry-neutral BY ITSELF — the verdict is
	 *  the CALLER's. PlaceMines calls it at P AND at its exact rotation −P, so it
	 *  deletes rotational PAIRS (symmetry-preserving, the every-match case);
	 *  ValidateTraversability calls it around ONE unreachable mine, which is a
	 *  genuine asymmetry escape and logs a Warning at that site.
	 */
	int32 RemoveBlockingInstancesInDisc(const FVector2D& Center, float Radius);

	/**
	 *  Resolves a team's live ACastle actor, or null if none exists.
	 *  TASK-576: extracted from ResolveCastleLocation (whose behaviour is unchanged —
	 *  it now delegates) so the LOCATION and the BOUNDS used by the traversability
	 *  query endpoint are read from THE SAME actor by construction, rather than from
	 *  two independent iterations that a duplicate/mis-teamed castle could split.
	 */
	ACastle* ResolveCastleActor(ETeamId Team) const;

	/** Resolves a team's castle world location from the live ACastle actors, falling back to the ±25000 constants (M7.6 10× scale-up). */
	FVector ResolveCastleLocation(ETeamId Team) const;

	/**
	 *  ⚖️ TASK-576 (WR-§2b row D): the traversability query inset, DERIVED FROM THE
	 *  LIVE CASTLE instead of transcribed. Returns the distance to move CastleLocation
	 *  toward the centerline so the path-query endpoint lands on open pad ground.
	 *
	 *  ⛔ NOT const, deliberately: it owns the one-shot fallback warning flag.
	 *  ⚠️ Null castle or degenerate colliding bounds ⇒ the AUTHORED literals (today's
	 *  behaviour), warned once. ⛔ Never zero — a zero inset puts the endpoint at the
	 *  castle CENTRE, inside its own nav-carved hole, which is the guaranteed
	 *  false-negative this whole constant exists to avoid.
	 */
	float ResolveCastleQueryInset(ETeamId Team, const FVector& CastleLocation);

	/** A single keep-clear disc (center XY + radius²). */
	struct FKeepClearZone
	{
		FVector2D Center = FVector2D::ZeroVector;
		float RadiusSq = 0.f;
	};

	/** Keep-clear discs for THIS generate (castles/PlayerStart — gold-node discs removed with the W1-PREP mines redesign, TASK-255); rebuilt each GenerateScatter. */
	TArray<FKeepClearZone> KeepClearZones;

	/** Cached corridor half-width for this generate (from the config). */
	float CorridorHalfWidthCached = 0.f;

	/**
	 *  TASK-1122 (GFX-§9): the Foliage quality scalar read ONCE per generate, at
	 *  the top of RunScatterPasses beside `FRandomStream Stream(Seed)` — the single
	 *  funnel for both the authority path and the client mirror. Follows
	 *  CorridorHalfWidthCached's idiom exactly (cache-per-generate, no tick, no
	 *  per-frame poll, no CVar sink).
	 *
	 *  ⛔ 1.0 IS THE FAIL-SAFE and it is the value on a dedicated server, in a
	 *  test, and whenever the graphics facade cannot be resolved: 1.0 means "the
	 *  authored DA_BattlefieldScatter bands, unchanged".
	 *  ⛔ THIS FIELD IS READ AT EXACTLY ONE PLACE — ApplyFoliageCullBands() — and
	 *  that function calls nothing but SetCullDistances. It must never be read
	 *  inside ScatterLayer, never near an FRandomStream draw, and never by
	 *  anything that decides WHERE or HOW MANY.
	 */
	float FoliageCullScaleCached = 1.0f;

	/**
	 *  TASK-1122: one record per HISM this actor has ever created, holding the
	 *  AUTHORED cull band of the layer that created it.
	 *
	 *  ⛔ WHY THIS EXISTS RATHER THAN JUST READING `Layer` AT THE CALL SITE — the
	 *  defect it closes is real and was found by reading ClearScatter: a re-scatter
	 *  (Play Again) CLEARS INSTANCES BUT KEEPS THE COMPONENTS (ClearScatter:471-479),
	 *  and ResolveComponentForMesh RETURNS EARLY on the reuse path (:902-908) —
	 *  above every render-profile call. So without a re-apply pass, a quality change
	 *  followed by Play Again would move NOTHING, while a quality change followed by
	 *  a fresh match would work: a lever live on one path and silently dead on the
	 *  other, which is exactly the shape TASK-1109 / SC-§94 was bought on, and it
	 *  would make the panel's "applies at the next match start" sentence a lie for
	 *  half the ways a match starts.
	 *
	 *  ⛔ AND WHY THE AUTHORED BAND IS RECORDED RATHER THAN RE-READ: re-applying
	 *  from whatever `Layer` happens to be in scope would hand a mesh SHARED by two
	 *  layers the LAST layer's band, where the one-HISM-per-mesh law (:894-908,
	 *  OverrideMaterial comment) gives it the FIRST layer's. Recording at creation
	 *  preserves first-layer-wins EXACTLY, including at Epic where any drift would
	 *  be a regression on the default battlefield.
	 *
	 *  Weak pointers, and deliberately NOT a UPROPERTY: every component here is
	 *  already rooted by ScatterComponents (a UPROPERTY) — this is a secondary
	 *  index in the VisualToProxy / HillSurfaceComponents idiom — and a weak
	 *  pointer self-invalidates rather than dangling. Appended ONLY on the creation
	 *  path, so it is bounded by the unique-mesh count (~59 today) for the actor's
	 *  whole life, not by re-scatter count.
	 */
	struct FScatterCullBandRecord
	{
		TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> Component;
		FName LayerName = NAME_None;
		int32 AuthoredStartUU = 0;
		int32 AuthoredEndUU = 0;
	};
	TArray<FScatterCullBandRecord> CullBandRecords;

	/**
	 *  TASK-1122 (GFX-§9): re-applies the Foliage-scaled cull band to every
	 *  recorded HISM, then READS IT BACK off the component and logs both numbers
	 *  (SC-§94 cl. B — read the state, never echo the request; FIELD-§7 / the
	 *  GFX-§9 "verify by pixels AND the engine log" obligation).
	 *
	 *  Called from RunScatterPasses AFTER both placement passes, so (a) every
	 *  component exists, (b) it costs one pass over ~59 components rather than a
	 *  lookup inside the per-instance rejection loop, and (c) the reuse path of a
	 *  Play-Again re-scatter is covered. ⛔ It calls SetCullDistances and NOTHING
	 *  else: no AddInstance, no RemoveInstance, no FRandomStream, no transform.
	 */
	void ApplyFoliageCullBands();

	/** Seed used by the most recent GenerateScatter (re-used when bReRandomizeOnMatchReset is false). */
	int32 LastSeed = 0;

	/** True once a seed has been chosen at least once (so a non-re-randomizing reset re-uses LastSeed). */
	bool bHasSeed = false;

	/**
	 *  M8 (TASK-356 doc D9/§3.5): the authority's FINAL chosen seed each generate
	 *  (previously logged only). Plain replication — consumed by
	 *  OnRep_GenerationIndex, which rides the same actor property bunch (applied
	 *  before the OnRep fires: the pair is atomic).
	 */
	UPROPERTY(Replicated)
	int32 ChosenSeed = 0;

	/**
	 *  M8 (doc D9/§3.5): incremented on EVERY authority GenerateScatter — the
	 *  client regen TRIGGER. Fires even when bReRandomizeOnMatchReset=false
	 *  repeats a seed (the index still changes), and a join-in-progress client
	 *  sees index >= 1 vs its CDO 0 ⇒ regenerates off the initial rep.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_GenerationIndex)
	int32 GenerationIndex = 0;

	/**
	 *  CLIENT regen (doc §3.5): ClearScatter + the seed-deterministic passes with
	 *  the replicated ChosenSeed. The nav-reachability validation and its
	 *  defensive culls stay AUTHORITY-ONLY (live navmesh queries + an attempt
	 *  counter — not client-reproducible); accepted residual D9: client obstacles
	 *  are a SUPERSET of the server's in the rare defensively-culled match —
	 *  provably never rubber-bands (movement corrections fire only when a client
	 *  claims passage the server refuses, and a superset makes that impossible).
	 */
	UFUNCTION()
	void OnRep_GenerationIndex();

	/**
	 *  The shared seed-deterministic generate body (M8 refactor, TASK-356): keep-
	 *  clear rebuild + the reproducibility log + the two layer passes + PlaceMines
	 *  — byte-identical sequence to the pre-M8 GenerateScatter tail. The AUTHORITY
	 *  path (bAuthoritativeGenerate) then arms the deferred reachability
	 *  validation; the CLIENT path skips it (authority-only, D9) and logs the
	 *  attempt-count line for the TASK-357 comparison instead.
	 */
	void RunScatterPasses(int32 Seed, bool bAuthoritativeGenerate);

	/** Pending deferred-validation timer — shared by the nav-settle poll (PollNavSettle) and the validation itself (one timer at a time; EndPlay clears it). */
	FTimerHandle TraversabilityTimerHandle;

	/** Widening-cull attempt counter for the deferred validation. */
	int32 ReachabilityAttempt = 0;

	/**
	 *  One-shot timer for the DEFINITIVE (post-settle) re-check — deliberately its
	 *  OWN handle, never TraversabilityTimerHandle: the two can legitimately be in
	 *  flight at the same time (a settle poll re-arming while generation finishes),
	 *  and sharing one handle would silently cancel whichever armed first. Cleared
	 *  by EndPlay and by every re-scatter.
	 *  ⛔ NOT A POLL: it is armed once per generation-finished latch, never re-arms
	 *  itself, and fires at the next timer tick.
	 */
	FTimerHandle DefinitiveCheckTimerHandle;

	/** The nav system this actor's delegate binding was taken on (weak: a torn-down nav system is forgotten rather than unbound through a stale pointer). */
	TWeakObjectPtr<UNavigationSystemV1> BoundNavSystem;

	/** True while OnNavGenerationFinished is bound — the idempotence latch for the bind/unbind pair. */
	bool bNavGenerationFinishedBound = false;

	/** True between the generation-finished broadcast and the deferred re-check running: collapses the N broadcasts of one drain (one per ANavigationData) into ONE check, and makes the callback non-re-entrant. */
	bool bDefinitiveCheckPending = false;

	/** True once the event-driven definitive re-check has produced a verdict — the "run it ONCE" latch (`NAV-§4`). Reset by every authority re-scatter. */
	bool bDefinitiveCheckDone = false;

	/** True while the definitive (post-settle) check is the one executing, so ValidateTraversability can label its line as the definitive verdict rather than the poll's. */
	bool bInDefinitiveCheck = false;

	/** Seconds accumulated by the CURRENT nav-settle poll (reset by StartNavSettlePoll, compared against MaxNavSettleWait). Runtime state, not a tunable. */
	float NavSettleElapsed = 0.f;

	/** One-shot guards for the graceful-degradation warnings (spam-free). */
	bool bWarnedNoConfig = false;
	bool bWarnedNoNav = false;

	/** TASK-576: one-shot guard for the castle-query-inset fallback warning (no castle / degenerate bounds ⇒ the authored literals). */
	bool bWarnedCastleInsetFallback = false;

	/** TASK-576: one-shot guard for the resolved-inset Log line, so the derivation is observable at PIE exactly once per actor lifetime instead of once per re-check. */
	bool bLoggedCastleInsetDerivation = false;

	// --- Tunables the traversability validation uses (mechanic rules → UPROPERTY defaults, CONVENTIONS §3.0 law) ---

	/**
	 *  Interval (s) between UNavigationSystemV1::IsNavigationBeingBuilt polls after
	 *  a generate/cull (M7.6 — replaces the fixed NavSettleDelay 0.75 s: a guessed
	 *  delay under-waits the ~9.8× field's async rebuild and over-waits a small
	 *  one, so the reachability validation now waits for NAV IDLE instead). The
	 *  first poll fires one interval AFTER the scatter, by which time the dirty-
	 *  area rebuild has registered — never a same-frame false-idle.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0.05"))
	float NavPollInterval = 0.25f;

	/** Cap (s) on the nav-idle wait: if the async rebuild is STILL running after this, ValidateTraversability proceeds anyway with a warning — a pathologically slow build must never strand the traversability confirmation. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float MaxNavSettleWait = 10.f;

	/** Fixed fallback wait (s) used only when NO nav system exists to poll — ValidateTraversability then skips the nav confirmation gracefully (the reserved corridor stays the deterministic guarantee). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float NavSettleFallbackDelay = 2.f;

	/** Max widening-cull re-checks before the corridor is force-cleared (each re-check widens the culled Y band by CorridorWidenStep). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "1"))
	int32 MaxReachabilityAttempts = 5;

	/**
	 *  ⛔ THE DETERMINISM SWITCH (TASK-535 — CONVENTIONS `NAV-§4`). DEFAULT false, and
	 *  the default is the law: a PROVISIONAL (pre-settle) reachability FAILURE never
	 *  runs the widening cull, because that cull DELETES INSTANCES and its answer, on
	 *  a partially-built navmesh, depends on wall-clock timing — which would make the
	 *  shipped instance set stop being a pure function of the seed (the determinism
	 *  law, TASKBOARD.md:11044). With it false, every cull that ever runs was decided
	 *  by a SETTLED query, so it is reproducible from the seed alone.
	 *  ⚠️ FLIPPING IT TO true RESTORES THE PRE-TASK-535 BEHAVIOUR AND RE-OPENS THAT
	 *  HOLE. It exists only so the old path is one checkbox away for a diagnostic
	 *  A/B — ⛔ never as a shipping default, and ⛔ never as the answer to "the rock
	 *  popped out late" (that visible late cull is the accepted, flagged consequence:
	 *  it fires ONLY when the field is genuinely walled off, and a visible pop is
	 *  strictly better than an unwinnable match).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability")
	bool bCullOnProvisionalFailure = false;

	/** How much (cm) the culled corridor band widens per failed re-check. M7.6: 250 → 400, scaled with the corridor (half-width now 1,000) so each widening attempt still clears a meaningful band of the 10× field. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float CorridorWidenStep = 400.f;

	/** Max rejection-sampling attempts per instance before it is skipped (keeps GenerateScatter bounded on a crowded field). Raised 16→24 for M6.6: the radius-aware keep-clear / edge-clamp / spacing tests reject more candidates, so more attempts are needed to hit the target count. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "1"))
	int32 MaxPlacementAttemptsPerInstance = 24;

	/**
	 *  ⛔ AUTHORED FLOOR / DEGENERATE-BOUNDS FALLBACK ONLY — ⛔ NO LONGER THE INSET
	 *  THAT SHIPS. Inset (cm) applied to each castle location toward the centerline
	 *  before the reachability path query, so the query endpoints land on OPEN pad
	 *  ground rather than inside the castle's own nav-carved footprint (which would be
	 *  a false-negative path result).
	 *
	 *  ⚖️ RE-DERIVED STRUCTURALLY 2026-08-15 — WR-§2b row D (TASK-576). ⛔ THE VALUE IS
	 *  UNCHANGED AND THAT IS THE POINT: what changed is that ResolveCastleQueryInset
	 *  now MEASURES the castle (GetActorBounds(bOnlyCollidingComponents=true)) and adds
	 *  CastleQueryFacePad, with this literal demoted to a floor. WHY a bump was refused:
	 *    • its own retired doc said "≈1200 clears a ~810-unit castle footprint", so it
	 *      is castle-derived by construction — and it has been STALE SINCE CASTLE-3X:
	 *      at half-depth 1,218.95 the endpoint already landed ≈19 uu INSIDE the
	 *      footprint, the exact false-negative it exists to prevent;
	 *    • at the 9× castle (half-depth 3,656.85) it is 2,457 uu inside;
	 *    • ⛔ ×3 = 3,600 DOES NOT FIX IT — still 57 uu inside. A multiplier buys one
	 *      resize before it rots again, which is why WR-§2b bans it for this row.
	 *  ⚠️ A NON-ZERO value here is load-bearing: it is what the null/degenerate path
	 *  falls back to (with CastleQueryFacePad), and a zero inset would put the endpoint
	 *  at the castle CENTRE — inside the nav hole, a guaranteed false negative.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float CastleQueryInset = 1200.f;

	/**
	 *  ⭐ THE LIVE TUNABLE (TASK-576, WR-§2b row D — the same BAND-PAST-THE-WALL-FACE
	 *  shape rulings W2-R2 and row B use): how far (cm) PAST the castle's measured
	 *  colliding wall face the path-query endpoint is placed. The shipped inset is
	 *      measured face distance  +  this pad,
	 *  floored by CastleQueryInset, so a castle resize carries the endpoint with it and
	 *  this constant can never rot the way the raw inset did.
	 *
	 *  📐 795 IS NOT A NEW NUMBER — IT IS THE ORIGINAL AUTHOR'S PAD, RECOVERED: the
	 *  retired doc sized 1,200 against a ~810-uu castle footprint (half-extent ≈405),
	 *  so the pad they actually chose was 1,200 − 405 = 795. ⇒ at that castle this
	 *  derivation REPRODUCES 1,200 EXACTLY (405 + 795); at CASTLE-3X it yields 2,014
	 *  (the value that row should always have had); at 9× it yields 4,451.85.
	 *
	 *  ⚖️ THE HONEST BAND, AND BOTH ENDS ARE ENFORCED IN CODE: the endpoint must be
	 *  OUTSIDE the castle footprint (> 3,656.85 at 9×) and, where possible, INSIDE
	 *  USiegeScatterConfig::CastleKeepClearRadius (4,500 after WR-§2 row 4), inside
	 *  which the pad is guaranteed obstacle-free. 4,451.85 satisfies both, with 48 uu
	 *  of headroom under the disc — so the pad is CAPPED at the room the disc leaves
	 *  whenever that room is positive. ⚠️ If the disc is NARROWER than the castle
	 *  (e.g. before TASK-569 lands 4,500 in DA_BattlefieldScatter, where it reads
	 *  1,500), the cap is skipped ON PURPOSE: clearing the wall face outranks sitting
	 *  in the disc, because outside-the-disc risks a stray blocker while
	 *  inside-the-footprint is a CERTAIN false negative.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float CastleQueryFacePad = 795.f;
};
