// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "GoldNode.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 *  Siegebound gold node (GDD §3.3/§5, TASK-025) — the mining destination for
 *  AMinerUnit. Purely a location marker with a visual: it does nothing on its
 *  own; miners walk to the SAME-team node and activate their +1 gold/s there.
 *
 *  Arena contract (CONVENTIONS.md / GDD §5): one per team, 800 units in front
 *  of each castle — placed in TASK-036 as GoldNode_Blue, Team = Blue, at
 *  (-1200, 0, 0) and GoldNode_Red, Team = Red, at (+1200, 0, 0).
 *
 *  - NOT a combatant: deliberately does NOT implement ITeamAgent — unit
 *    acquisition scans ITeamAgent actors (TASK-004), so implementing it would
 *    make enemy units target the node (spec: "no ITeamAgent combat
 *    participation"). Team here is ownership metadata only, read by miners
 *    through GetTeam().
 *  - NOT damageable: SetCanBeDamaged(false) — ApplyDamage routes are refused
 *    engine-side. §3.3's raidable investment is the MINER, never the node.
 *  - Blocks NOTHING: the mesh carries no collision at all (NoCollision
 *    profile, no overlaps, no navmesh relevance). Miners must be able to stand
 *    at/inside the node's footprint (TASK-025 hard rule: the node's own
 *    collision must never block miner arrival), and the node must not carve
 *    the navmesh it is the walk destination of.
 *  - Visual: /Game/Meshes/SM_GoldNode (soft reference — the asset arrives with
 *    TASK-038 and carries M_GoldGlow on slot 0 from import; null-safe: a
 *    missing mesh is logged once at runtime and means an invisible-but-
 *    functional node, never a crash). Resolved in OnConstruction so the node
 *    shows in the editor viewport for TASK-036, and again at BeginPlay for
 *    actors saved before the mesh existed. The component stays Movable: a
 *    Static-mobility component refuses SetStaticMesh once the world has begun
 *    play, which would break the deferred-asset runtime resolve.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AGoldNode : public AActor
{
	GENERATED_BODY()

public:

	AGoldNode();

	/**
	 *  Team whose miners mine at this node (ownership metadata — see the class
	 *  doc for why this is NOT ITeamAgent::GetTeamId). AMinerUnit iterates
	 *  AGoldNode actors and walks to the nearest node whose GetTeam() matches
	 *  its own team (TASK-025).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Team")
	ETeamId GetTeam() const { return Team; }

	/** Resolves the soft mesh in-editor so TASK-036 sees the node while placing it (null-safe, silent — TASK-038 may not have run yet). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Runtime mesh resolve for nodes saved/spawned before SM_GoldNode existed; warns once if the asset is still missing. */
	virtual void BeginPlay() override;

	/**
	 *  Root visual (spec: "UStaticMeshComponent root"). Mesh comes from
	 *  NodeMeshAsset — never hard-referenced (the asset does not exist until
	 *  TASK-038). Carries NO collision by design (see class doc).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|GoldNode")
	TObjectPtr<UStaticMeshComponent> NodeMesh;

	/** Owning team, set per placed instance (TASK-036: GoldNode_Blue = Blue at -1200, GoldNode_Red = Red at +1200). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/**
	 *  Soft reference to the node visual: /Game/Meshes/SM_GoldNode (exact path
	 *  from the TASK-025/TASK-038 names blocks; slot 0 = M_GoldGlow is baked
	 *  into the imported asset, no material logic here). Null-safe on load;
	 *  clearing it in a child/instance is a silent designer opt-out (the
	 *  AttackImpactEffect IsNull pattern, TASK-020).
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|GoldNode")
	TSoftObjectPtr<UStaticMesh> NodeMeshAsset;

private:

	/** Loads NodeMeshAsset onto NodeMesh if resolvable; optionally warns (once) when it is not. */
	void ResolveNodeMesh(bool bWarnIfMissing);

	/** One-shot guard for the missing-mesh warning. */
	bool bWarnedMissingMesh = false;
};
