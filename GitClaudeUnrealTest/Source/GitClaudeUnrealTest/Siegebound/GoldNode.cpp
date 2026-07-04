// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/GoldNode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GitClaudeUnrealTest.h"

AGoldNode::AGoldNode()
{
	// pure location marker + visual: nothing to do per frame
	PrimaryActorTick.bCanEverTick = false;

	// not damageable (TASK-025 spec): the node is scenery, not a combatant —
	// ApplyDamage routes are refused engine-side before any TakeDamage runs.
	// §3.3's raidable investment is the miner itself.
	SetCanBeDamaged(false);

	// root visual (spec: "UStaticMeshComponent root"). Collision profile
	// NoCollision + no overlaps + no navmesh relevance: the node blocks
	// NOTHING (TASK-025 hard rule — its collision must never block miner
	// arrival, and it must not carve the navmesh it is the destination of).
	// Mobility stays Movable (the component default): a Static-mobility
	// component refuses SetStaticMesh once the world has begun play, which
	// would break the deferred-asset resolve in BeginPlay below.
	NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	SetRootComponent(NodeMesh);
	NodeMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	NodeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NodeMesh->SetGenerateOverlapEvents(false);
	NodeMesh->SetCanEverAffectNavigation(false);

	// visual contract (TASK-025/TASK-038 names blocks): soft path, never a hard
	// reference — the asset is modeled/imported by TASK-038 and may not exist yet.
	NodeMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_GoldNode.SM_GoldNode")));
}

void AGoldNode::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// editor-time resolve so TASK-036 sees the node while placing it. Silent:
	// pre-TASK-038 the asset legitimately does not exist, and OnConstruction
	// re-runs on every editor property tweak — warning here would spam.
	ResolveNodeMesh(/*bWarnIfMissing=*/ false);
}

void AGoldNode::BeginPlay()
{
	Super::BeginPlay();

	// runtime fallback for nodes saved into the level before SM_GoldNode was
	// imported (level-loaded actors do not re-run construction scripts).
	ResolveNodeMesh(/*bWarnIfMissing=*/ true);
}

void AGoldNode::ResolveNodeMesh(bool bWarnIfMissing)
{
	// cleared-in-editor (IsNull) is a silent designer opt-out — the
	// AttackImpactEffect pattern (TASK-020)
	if (!NodeMesh || NodeMeshAsset.IsNull())
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = NodeMeshAsset.LoadSynchronous())
	{
		// SetStaticMesh self-no-ops on the same mesh; the explicit check keeps
		// repeated OnConstruction runs from touching the render state at all
		if (NodeMesh->GetStaticMesh() != LoadedMesh)
		{
			NodeMesh->SetStaticMesh(LoadedMesh);
		}
	}
	else if (bWarnIfMissing && !bWarnedMissingMesh)
	{
		bWarnedMissingMesh = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AGoldNode '%s': mesh '%s' failed to load (SM_GoldNode is imported in TASK-038) — node is invisible but fully functional."),
			*GetNameSafe(this), *NodeMeshAsset.ToString());
	}
}
