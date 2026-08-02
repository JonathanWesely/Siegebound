// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "Siegebound/TeamId.h"
#include "BattlefieldScatter.generated.h"

class AAncientGround;
class AGoldNode;
class USceneComponent;
class USiegeScatterConfig;
class UHierarchicalInstancedStaticMeshComponent;
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

protected:

	/** Scatters once at match start. */
	virtual void BeginPlay() override;

	/** Stops the pending traversability timer. */
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

	/** Deferred (async-nav-settled) reachability confirmation — path-queries Blue→Red AND Blue→each mine (TASK-255 economy guarantee); defensively culls corridor blockers / per-mine clearance discs and regrounds mines after every cull. */
	void ValidateTraversability();

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

	/** Resolves a team's castle world location from the live ACastle actors, falling back to the ±25000 constants (M7.6 10× scale-up). */
	FVector ResolveCastleLocation(ETeamId Team) const;

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

	/** Seconds accumulated by the CURRENT nav-settle poll (reset by StartNavSettlePoll, compared against MaxNavSettleWait). Runtime state, not a tunable. */
	float NavSettleElapsed = 0.f;

	/** One-shot guards for the graceful-degradation warnings (spam-free). */
	bool bWarnedNoConfig = false;
	bool bWarnedNoNav = false;

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

	/** How much (cm) the culled corridor band widens per failed re-check. M7.6: 250 → 400, scaled with the corridor (half-width now 1,000) so each widening attempt still clears a meaningful band of the 10× field. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float CorridorWidenStep = 400.f;

	/** Max rejection-sampling attempts per instance before it is skipped (keeps GenerateScatter bounded on a crowded field). Raised 16→24 for M6.6: the radius-aware keep-clear / edge-clamp / spacing tests reject more candidates, so more attempts are needed to hit the target count. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "1"))
	int32 MaxPlacementAttemptsPerInstance = 24;

	/**
	 *  Inset (cm) applied to each castle location toward the centerline before the
	 *  reachability path query, so the query endpoints land on OPEN pad ground
	 *  rather than inside the castle's own nav-carved footprint (which would be a
	 *  false-negative path result). The pad is inside CastleKeepClearRadius so no
	 *  obstacle ever sits there; ≈1200 clears a ~810-unit castle footprint.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float CastleQueryInset = 1200.f;
};
