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
 *  (±8000 castles) with trees / rocks / hills / grass, re-seeded each match so
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
 *        the async Dynamic nav has settled) that path-queries Blue-anchor →
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

	/** Resolves-or-creates the HISM for a mesh, applying the layer's collision/nav profile. Returns nullptr if the mesh is unresolvable. */
	UHierarchicalInstancedStaticMeshComponent* ResolveComponentForMesh(UStaticMesh* Mesh, const FScatterLayer& Layer);

	/** Scatters a single layer's instances via the shared FRandomStream (asymmetric unless bMirrorSymmetric). */
	void ScatterLayer(const FScatterLayer& Layer, FRandomStream& Stream);

	/** True if a 2D point is inside any keep-clear zone (castle/node/PlayerStart radius) or the reserved corridor band. Built fresh each generate from the live level actors. */
	bool IsInKeepClear(const FVector2D& Point2D) const;

	/** Rebuilds KeepClearZones + corridor half-width from the live level (castles/nodes/PlayerStart) with CONVENTIONS-coordinate fallbacks. */
	void RebuildKeepClearZones();

	/** Traces down to the arena floor at (X,Y); returns the floor Z (or 0 if no hit). Ignores this actor so already-placed instances never fool the trace. */
	float GroundZAt(float X, float Y) const;

	/** Deferred (async-nav-settled) reachability confirmation — path-queries Blue→Red and culls corridor blockers if (defensively) needed. */
	void ValidateTraversability();

	/** Culls blocking instances whose |Y| <= Band across the X span (widening re-roll used only if a path is somehow not found). Returns the number removed. */
	int32 CullCorridorBlockers(float Band);

	/** Resolves a team's castle world location from the live ACastle actors, falling back to the CONVENTIONS ±8000 constants. */
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

	/** Pending deferred-validation timer. */
	FTimerHandle TraversabilityTimerHandle;

	/** Widening-cull attempt counter for the deferred validation. */
	int32 ReachabilityAttempt = 0;

	/** One-shot guards for the graceful-degradation warnings (spam-free). */
	bool bWarnedNoConfig = false;
	bool bWarnedNoNav = false;

	// --- Tunables the traversability validation uses (mechanic rules → UPROPERTY defaults, CONVENTIONS §3.0 law) ---

	/** Delay (s) after GenerateScatter before the reachability check runs, so the async Dynamic navmesh update can settle. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float NavSettleDelay = 0.75f;

	/** Max widening-cull re-checks before the corridor is force-cleared (each re-check widens the culled Y band by CorridorWidenStep). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "1"))
	int32 MaxReachabilityAttempts = 5;

	/** How much (cm) the culled corridor band widens per failed re-check. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "0"))
	float CorridorWidenStep = 250.f;

	/** Max rejection-sampling attempts per instance before it is skipped (keeps GenerateScatter bounded on a crowded field). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability", meta = (ClampMin = "1"))
	int32 MaxPlacementAttemptsPerInstance = 16;

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
