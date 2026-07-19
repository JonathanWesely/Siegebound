// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "Siegebound/TeamId.h"
#include "BattlefieldScatter.generated.h"

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
 *        both castles, both gold nodes, and the PlayerStart. Because no blocking
 *        obstacle ever lands in the corridor, the straight Y≈0 lane between the
 *        castles is ALWAYS walkable — this alone guarantees a path and does not
 *        depend on any async nav state.
 *    (2) CONFIRMATORY — a deferred post-placement reachability validation (once
 *        the async Dynamic nav reports IDLE — polled via UNavigationSystemV1::
 *        IsNavigationBeingBuilt, M7.6) that path-queries Blue-anchor →
 *        Red-anchor; if (defensively) no path is found it culls the blocking
 *        instances nearest the lane in a widening Y band and re-checks, until a
 *        path is confirmed. Never leaves a match unwinnable.
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
	 *  Clears any existing instances, picks a seed (OverrideSeed>0 else a random
	 *  seed, LOGGED on LogSiegeTerrain so the layout is reproducible), scatters
	 *  every layer honoring keep-clear + spacing + the reserved corridor, wires
	 *  blocking/nav on obstacle layers, then schedules the deferred
	 *  reachability validation. Graceful no-op when ScatterConfig is unset.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Terrain")
	void GenerateScatter();

	/** Removes every scattered instance from every HISM (the components persist for reuse). */
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

	/** Scatters a single layer's instances via the shared FRandomStream (asymmetric unless bMirrorSymmetric). */
	void ScatterLayer(const FScatterLayer& Layer, FRandomStream& Stream);

	/**
	 *  True if a 2D point — inflated by InstanceRadius — is inside any keep-clear
	 *  zone (castle/node/PlayerStart radius) or the reserved corridor band. The
	 *  radius makes a WIDE instance's EDGE (not just its center) count, so a broad
	 *  hill centered just off-lane no longer sprawls into the corridor. Built fresh
	 *  each generate from the live level actors.
	 */
	bool IsInKeepClear(const FVector2D& Point2D, float InstanceRadius = 0.f) const;

	/** Rebuilds KeepClearZones + corridor half-width from the live level (castles/nodes/PlayerStart) with CONVENTIONS-coordinate fallbacks. */
	void RebuildKeepClearZones();

	/** Traces down to the arena floor at (X,Y); returns the floor Z (or 0 if no hit). Ignores this actor so already-placed instances never fool the trace. */
	float GroundZAt(float X, float Y) const;

	/** Deferred (async-nav-settled) reachability confirmation — path-queries Blue→Red and culls corridor blockers if (defensively) needed. */
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

	/** Resolves a team's castle world location from the live ACastle actors, falling back to the ±25000 constants (M7.6 10× scale-up). */
	FVector ResolveCastleLocation(ETeamId Team) const;

	/** A single keep-clear disc (center XY + radius²). */
	struct FKeepClearZone
	{
		FVector2D Center = FVector2D::ZeroVector;
		float RadiusSq = 0.f;
	};

	/** Keep-clear discs for THIS generate (castles/nodes/PlayerStart); rebuilt each GenerateScatter. */
	TArray<FKeepClearZone> KeepClearZones;

	/** Cached corridor half-width for this generate (from the config). */
	float CorridorHalfWidthCached = 0.f;

	/** Seed used by the most recent GenerateScatter (re-used when bReRandomizeOnMatchReset is false). */
	int32 LastSeed = 0;

	/** True once a seed has been chosen at least once (so a non-re-randomizing reset re-uses LastSeed). */
	bool bHasSeed = false;

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
