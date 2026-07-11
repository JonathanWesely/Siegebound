// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"
#include "ScatterConfig.generated.h"

class UStaticMesh;

/**
 *  Region bias for a scatter layer's placement (CONVENTIONS "Battlefield &
 *  procedural terrain (M6.5)" — the `region-bias (whole-field / edge-bias)`
 *  knob). Used by ASiegeBattlefieldScatter to weight where a layer's instances
 *  land within the arena Y band. Purely aesthetic density weighting — it NEVER
 *  overrides the keep-clear / corridor exclusions (those are the traversability
 *  guarantee and always win).
 *   - WholeField : uniform across the field width (default for grass/hills).
 *   - EdgeBias   : denser toward the field's Y edges, thinner near the lane
 *                  (keeps the mid-field readable for the combat corridor).
 *   - CenterBias : denser toward the centerline Y band (still outside the
 *                  reserved corridor — obstacles hug the flanks of the lane).
 */
UENUM(BlueprintType)
enum class EScatterRegionBias : uint8
{
	WholeField,
	EdgeBias,
	CenterBias
};

/**
 *  One layer of the procedural battlefield scatter (CONVENTIONS "Battlefield &
 *  procedural terrain (M6.5)"). A layer is one logical group — TREES / ROCKS /
 *  HILLS / GRASS — that shares placement rules and a candidate mesh set. The
 *  scatter actor owns ONE HISM per unique mesh (perf law), and each instance of
 *  a layer picks a random mesh from `Meshes`, so a single layer of 8 mesh
 *  variants yields high visual variety from 8 HISMs.
 *
 *  All fields are content, editor-populated on the DataAsset instance
 *  (/Game/Data/DA_BattlefieldScatter, TASK-137) — none is hardcoded in C++.
 */
USTRUCT(BlueprintType)
struct FScatterLayer
{
	GENERATED_BODY()

	/** Human/debug name for the layer (e.g. "Trees", "Rocks", "Hills", "Grass"). Only used in log lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	FName LayerName = NAME_None;

	/**
	 *  Candidate meshes for this layer (SOFT so the config never force-loads the
	 *  heavy Fab donors until GenerateScatter resolves them). One HISM is created
	 *  per unique resolved mesh; each placed instance picks a random entry, so
	 *  more meshes = more variety (Jonathan: "use as many assets as possible").
	 *  Null/unresolvable entries are skipped (logged once), never a crash.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	TArray<TSoftObjectPtr<UStaticMesh>> Meshes;

	/** How many instances to ATTEMPT to place for this layer (rejection sampling may place fewer if the field is crowded). Cap per the §6 perf budget — blocking + dynamic-nav is costly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	int32 InstanceCount = 0;

	/** Uniform per-instance scale range [Min, Max]; each instance rolls a uniform scale in this range. Min==Max==1 = no scale variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	FVector2D ScaleRange = FVector2D(1.f, 1.f);

	/** Random full 0-360° yaw per instance (natural rotation variety). Pitch/roll stay upright. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	bool bRandomYaw = true;

	/**
	 *  BLOCKING obstacle layer (Jonathan decision #1, 2026-07-10): instances block
	 *  the Pawn channel and set bCanEverAffectNavigation=true so units physically
	 *  block on them AND the (Dynamic) navmesh carves around them. Defaults TRUE
	 *  (the obstacle layers TREES/ROCKS/HILLS) — set FALSE for the GRASS layer
	 *  (pure decoration: NoCollision, no nav effect). Blocking layers ALSO honor
	 *  the keep-clear zones + reserved corridor; the non-blocking GRASS layer
	 *  ignores keep-clear (lush everywhere).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	bool bBlocking = true;

	/** Density weighting across the arena Y band (see EScatterRegionBias). Never overrides keep-clear/corridor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	EScatterRegionBias RegionBias = EScatterRegionBias::WholeField;

	/** Minimum 2D spacing (cm) between two instances of THIS layer — rejection sampling drops candidates closer than this to an already-placed instance of the same layer. 0 = no spacing rule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	float MinSpacing = 300.f;

	/**
	 *  Vertical offset (cm) applied to placed instances after the ground trace —
	 *  a per-mesh origin correction for donor meshes whose pivot is not at the
	 *  base (art-director flags these in handoffs/TASK-135.md; build-master sets
	 *  the value on the DataAsset at TASK-137). 0 for base-pivot meshes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	float ZOffset = 0.f;
};

/**
 *  Designer config for the runtime procedural battlefield scatter (CONVENTIONS
 *  "Battlefield & procedural terrain (M6.5)", TASK-134). A UDataAsset so the
 *  layer set + density + placement knobs are CONTENT, not code — the instance
 *  /Game/Data/DA_BattlefieldScatter is populated by build-master (TASK-137) from
 *  the art-director's curated Fab mesh list (TASK-135). ASiegeBattlefieldScatter
 *  reads it at match start; an unset/empty config is a graceful no-op.
 *
 *  Placement is ASYMMETRIC organic random by default (Jonathan ruling — this is
 *  PvE so organic variety beats strict fairness); bMirrorSymmetric is the
 *  playtest fallback toggle that mirrors placement across the X=0 centerline.
 *
 *  The keep-clear radii + corridor half-width below are the DATA half of the
 *  NON-NEGOTIABLE traversability guarantee: blocking obstacles are excluded from
 *  the castle pads, gold-node pads, the PlayerStart, and the reserved central
 *  lane, so a navigable Blue→Red path always exists. Jonathan can make the field
 *  denser/riskier at playtest by shrinking these.
 */
UCLASS(BlueprintType)
class GITCLAUDEUNREALTEST_API USiegeScatterConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	/** The scatter layers — TREES / ROCKS / HILLS / GRASS (order does not matter; each is placed independently). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Layers")
	TArray<FScatterLayer> Layers;

	/**
	 *  FALSE (default) = fully ASYMMETRIC organic random placement (Jonathan's
	 *  M6.5 ruling — organic variety for PvE). TRUE = mirror every placement
	 *  across the X=0 centerline for a symmetric, provably-fair field (the
	 *  playtest fallback if asymmetric matches read as unfair). A FLAGGED
	 *  decision — never silently imposed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Placement")
	bool bMirrorSymmetric = false;

	/**
	 *  Half-extent (cm) of the rectangular scatter region on X (across the
	 *  castles) and Y (field width). Default X=8600 places a little past the
	 *  ±8000 castles; Y=3200 matches the arena floor half-width. The actor clamps
	 *  to this if the level bounds cannot be resolved.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Bounds")
	FVector2D ArenaHalfExtent = FVector2D(8600.f, 3200.f);

	/** Keep-clear radius (cm) around EACH castle (±8000) — no blocking obstacle lands inside, so a castle's mouth is never walled. Part of the traversability guarantee. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float CastleKeepClearRadius = 900.f;

	/** Keep-clear radius (cm) around EACH gold node (±7200) — miners must always reach their node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float GoldNodeKeepClearRadius = 500.f;

	/** Keep-clear radius (cm) around the PlayerStart / hero spawn (≈-6800,0) — the hero never spawns inside an obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float PlayerStartKeepClearRadius = 700.f;

	/**
	 *  Half-width (cm) of the reserved central combat corridor: NO blocking
	 *  obstacle is placed within |Y| <= this across the WHOLE X span. This keeps
	 *  the straight Y≈0 lane between the two castles permanently walkable — the
	 *  deterministic core of the traversability guarantee (the nav reachability
	 *  check is the belt-and-suspenders confirmation on top). Grass ignores it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float CorridorHalfWidth = 400.f;
};
