// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/BattlefieldScatter.h"

#include "CollisionQueryParams.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInterface.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Siegebound/Castle.h"
#include "Siegebound/GoldNode.h"
#include "Siegebound/ScatterConfig.h"

DEFINE_LOG_CATEGORY(LogSiegeTerrain);

namespace
{
	/**
	 *  Samples a Y offset in [-HalfY, HalfY] with the layer's density bias. The
	 *  magnitude curve weights the placement toward the field edges / center /
	 *  neither; a fair coin picks the sign. This is aesthetic density only —
	 *  keep-clear (incl. the reserved corridor) is enforced separately and always
	 *  wins, so a CenterBias obstacle still never lands in the lane.
	 */
	float SampleBiasedY(FRandomStream& Stream, float HalfY, EScatterRegionBias Bias)
	{
		const float U = Stream.FRand(); // [0,1)
		float Mag;
		switch (Bias)
		{
		case EScatterRegionBias::EdgeBias:   Mag = FMath::Sqrt(U); break; // bias toward |Y| = HalfY
		case EScatterRegionBias::CenterBias: Mag = U * U;          break; // bias toward Y = 0
		case EScatterRegionBias::WholeField:
		default:                             Mag = U;              break; // uniform magnitude
		}
		const float Sign = (Stream.FRand() < 0.5f) ? -1.f : 1.f;
		return Sign * Mag * HalfY;
	}
}

ASiegeBattlefieldScatter::ASiegeBattlefieldScatter()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Projectile-terrain contract (CONVENTIONS "Climbable terrain (M6.6)", decision
	// #3): AProjectile::FindEnvironmentImpact already object-traces the scatter HISMs
	// (WorldStatic/WorldDynamic) and keeps only impacts whose owner actor is tagged,
	// so this SINGLE tag makes arrows die on the scattered rocks/hills/tree-trunks.
	// Exact string "Terrain" — NOT "Obstacle": the Obstacle tag is read as an
	// actor-location clearance test, and this actor is a single point at the origin,
	// so Obstacle would place the whole clearance zone at (0,0,0). "Terrain" is the
	// walkable/cover tag the projectile pass wants.
	Tags.Add(FName(TEXT("Terrain")));
}

void ASiegeBattlefieldScatter::BeginPlay()
{
	Super::BeginPlay();

	// Match-start scatter (Jonathan: "randomly generated at the start of each match").
	GenerateScatter();
}

void ASiegeBattlefieldScatter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraversabilityTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ASiegeBattlefieldScatter::GenerateScatter()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!ScatterConfig)
	{
		if (!bWarnedNoConfig)
		{
			bWarnedNoConfig = true;
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] No ScatterConfig assigned — scatter is a no-op (DA_BattlefieldScatter unassigned; TASK-137 populates it)."),
				*GetNameSafe(this));
		}
		return;
	}

	// A generate always starts from a clean field (idempotent: BeginPlay's first
	// call clears nothing, Play Again's re-scatter clears the prior layout).
	ClearScatter();

	// Seed selection: a fixed OverrideSeed wins; else a fresh random seed every
	// match (Jonathan's "each match"); else — only when re-randomize is OFF —
	// re-use the last seed so Play Again reproduces the same layout.
	int32 Seed;
	if (OverrideSeed > 0)
	{
		Seed = OverrideSeed;
	}
	else if (!bReRandomizeOnMatchReset && bHasSeed)
	{
		Seed = LastSeed;
	}
	else
	{
		Seed = FMath::RandRange(1, MAX_int32 - 1);
	}
	LastSeed = Seed;
	bHasSeed = true;

	FRandomStream Stream(Seed);

	// Build the keep-clear discs (castles/nodes/PlayerStart) + cache the corridor
	// half-width for this generate.
	RebuildKeepClearZones();

	// The one grep-able reproducibility line (CONVENTIONS "Logging": LogSiegeTerrain).
	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter '%s'] GenerateScatter seed=%d mirror=%s layers=%d corridorHalfY=%.0f"),
		*GetNameSafe(this), Seed, ScatterConfig->bMirrorSymmetric ? TEXT("true") : TEXT("false"),
		ScatterConfig->Layers.Num(), CorridorHalfWidthCached);

	// W1-PREP hill-surface placement (CONVENTIONS "W1-PREP additions", TASK-250):
	// TWO passes so hill surfaces EXIST before anything traces onto them. Pass 1
	// places every bAllowOnHills=false layer (the hills themselves included — a
	// hill never places on a hill) and registers the real-geometry blocker HISMs
	// as hill surfaces; pass 2 places the bAllowOnHills=true layers, whose ground
	// resolve then accepts the elevated hill-surface Z (ResolveHillAwareGroundZ).
	// ⚠️ Seed-order note (the TASK-140 precedent): with NO layer opted in, the
	// pass split preserves the config order exactly, so existing seeds reproduce
	// unchanged; opting a layer in moves it to pass 2 and (intentionally) changes
	// the FRandomStream draw sequence — a fixed OverrideSeed stays deterministic.
	HillSurfaceComponents.Reset();
	for (const FScatterLayer& Layer : ScatterConfig->Layers)
	{
		if (!Layer.bAllowOnHills)
		{
			ScatterLayer(Layer, Stream);
		}
	}
	for (const FScatterLayer& Layer : ScatterConfig->Layers)
	{
		if (Layer.bAllowOnHills)
		{
			ScatterLayer(Layer, Stream);
		}
	}

	// Defer the reachability confirmation until the async Dynamic navmesh has
	// actually finished carving the newly-added obstacle instances: poll
	// IsNavigationBeingBuilt until the nav system reports idle (M7.6 — a fixed
	// delay guessed wrong at the ~9.8× field), capped by MaxNavSettleWait.
	// StartNavSettlePoll clears any pending timer itself.
	ReachabilityAttempt = 0;
	StartNavSettlePoll();
}

void ASiegeBattlefieldScatter::ClearScatter()
{
	for (UHierarchicalInstancedStaticMeshComponent* Comp : ScatterComponents)
	{
		if (Comp)
		{
			Comp->ClearInstances();
		}
	}
}

void ASiegeBattlefieldScatter::ScatterLayer(const FScatterLayer& Layer, FRandomStream& Stream)
{
	if (Layer.InstanceCount <= 0 || Layer.Meshes.Num() == 0 || !ScatterConfig)
	{
		return;
	}

	// Resolve (load) the soft donor meshes once.
	TArray<UStaticMesh*> Resolved;
	Resolved.Reserve(Layer.Meshes.Num());
	for (const TSoftObjectPtr<UStaticMesh>& SoftMesh : Layer.Meshes)
	{
		if (SoftMesh.IsNull())
		{
			continue;
		}
		if (UStaticMesh* Mesh = SoftMesh.LoadSynchronous())
		{
			Resolved.AddUnique(Mesh);
		}
		else
		{
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter] Layer '%s' mesh '%s' failed to resolve — skipped."),
				*Layer.LayerName.ToString(), *SoftMesh.ToString());
		}
	}
	if (Resolved.Num() == 0)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter] Layer '%s' has no resolvable meshes — layer skipped."),
			*Layer.LayerName.ToString());
		return;
	}

	// One HISM per unique mesh, wired with this layer's collision/nav profile.
	// W1-PREP (TASK-250): a pass-1 REAL-GEOMETRY blocker layer (bBlocking, no
	// collision proxy, not itself hill-allowed) registers its HISMs as HILL
	// SURFACES — what pass-2 bAllowOnHills layers ground-trace against. In the
	// shipped DA that is the HILLS layer; tree layers are excluded by their proxy
	// (a canopy top is not ground), grass by bBlocking=false, and any opted-in
	// blocker (e.g. rocks on hills) by its own bAllowOnHills=true.
	const bool bHillSurfaceProvider = Layer.bBlocking && Layer.CollisionProxyMesh.IsNull() && !Layer.bAllowOnHills;
	for (UStaticMesh* Mesh : Resolved)
	{
		UHierarchicalInstancedStaticMeshComponent* Comp = ResolveComponentForMesh(Mesh, Layer);
		if (Comp && bHillSurfaceProvider)
		{
			HillSurfaceComponents.AddUnique(Comp);
		}
	}

	const float HalfX = FMath::Max(ScatterConfig->ArenaHalfExtent.X, 1.f);
	const float HalfY = FMath::Max(ScatterConfig->ArenaHalfExtent.Y, 1.f);
	const bool bMirror = ScatterConfig->bMirrorSymmetric;
	const bool bUsesProxy = !Layer.CollisionProxyMesh.IsNull();
	const float ScaleLo = FMath::Min(Layer.ScaleRange.X, Layer.ScaleRange.Y);
	const float ScaleHi = FMath::Max(Layer.ScaleRange.X, Layer.ScaleRange.Y);

	// Placed points carry (2D center, footprint radius) so MinSpacing is radius-aware:
	// two instances are rejected when their centers are closer than MinSpacing + both
	// radii, so wide hills stop interpenetrating (was center-only TArray<FVector2D>).
	TArray<TPair<FVector2D, float>> PlacedPoints;
	PlacedPoints.Reserve(Layer.InstanceCount * (bMirror ? 2 : 1));

	int32 Placed = 0;
	for (int32 InstanceIndex = 0; InstanceIndex < Layer.InstanceCount; ++InstanceIndex)
	{
		bool bPlacedThis = false;
		for (int32 Attempt = 0; Attempt < MaxPlacementAttemptsPerInstance && !bPlacedThis; ++Attempt)
		{
			const float X = Stream.FRandRange(-HalfX, HalfX);
			const float Y = SampleBiasedY(Stream, HalfY, Layer.RegionBias);
			const FVector2D Candidate(X, Y);

			// ⚠️ SEED-ORDER CHANGE (M6.6 TASK-140, INTENTIONAL — NOT a regression):
			// the mesh + scale rolls were previously drawn AFTER the keep-clear/spacing
			// tests (only on a candidate that passed). They now roll HERE, BEFORE those
			// tests, because the radius-aware guards (keep-clear inflation, field-edge
			// clamp, radius-aware MinSpacing) need the instance's footprint radius,
			// which derives from the chosen mesh's bounds × the rolled scale. Drawing
			// them earlier changes the FRandomStream draw sequence, so EXISTING seeds
			// now produce DIFFERENT (still-valid) layouts. A fixed OverrideSeed stays
			// fully deterministic. (Documented in the TASK-140 handoff for build-master.)
			UStaticMesh* Mesh = Resolved[Stream.RandRange(0, Resolved.Num() - 1)];
			UHierarchicalInstancedStaticMeshComponent* Comp = ResolveComponentForMesh(Mesh, Layer);
			if (!Comp)
			{
				continue;
			}
			const float Scale = Stream.FRandRange(ScaleLo, ScaleHi);

			// Footprint radius: explicit override (absolute cm) else auto-derive from
			// the mesh's XY half-diagonal × the rolled scale.
			float FootprintR;
			if (Layer.FootprintRadius > 0.f)
			{
				FootprintR = Layer.FootprintRadius;
			}
			else
			{
				const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
				FootprintR = FVector2D(MeshBounds.BoxExtent.X, MeshBounds.BoxExtent.Y).Size() * Scale;
			}

			// FIELD-EDGE clamp: reject a candidate whose footprint would sprawl past
			// the arena half-extents into the boundary walls.
			if (FMath::Abs(X) + FootprintR > HalfX || FMath::Abs(Y) + FootprintR > HalfY)
			{
				continue;
			}

			// Blocking obstacles honor keep-clear + the reserved corridor, now inflated
			// by the footprint radius so a wide instance centered off-lane no longer
			// sprawls across the corridor; grass (non-blocking) ignores keep-clear.
			if (Layer.bBlocking && IsInKeepClear(Candidate, FootprintR))
			{
				continue;
			}

			// Radius-aware MinSpacing: reject if the centers are closer than
			// MinSpacing + this instance's radius + the other's radius.
			bool bTooClose = false;
			for (const TPair<FVector2D, float>& Other : PlacedPoints)
			{
				const float MinDist = Layer.MinSpacing + FootprintR + Other.Value;
				if (MinDist > 0.f && FVector2D::DistSquared(Other.Key, Candidate) < MinDist * MinDist)
				{
					bTooClose = true;
					break;
				}
			}
			if (bTooClose)
			{
				continue;
			}

			const float Yaw = Layer.bRandomYaw ? Stream.FRandRange(0.f, 360.f) : 0.f;

			// W1-PREP ground resolve (TASK-250): flat-floor trace first, then — for
			// a bAllowOnHills layer — the hill-surface upgrade: when a pass-1 hill
			// stands at this XY the instance grounds on the HILL surface (elevated
			// Z, slope within the layer's MaxPlacementSlopeDeg), and the candidate
			// is REJECTED outright over a steeper face (grounding at floor Z would
			// bury it inside the hill — the pre-W1-PREP bare-hills defect).
			float GroundZ = GroundZAt(X, Y);
			if (Layer.bAllowOnHills && !ResolveHillAwareGroundZ(X, Y, GroundZ, Layer.MaxPlacementSlopeDeg, GroundZ))
			{
				continue;
			}
			const float Z = GroundZ + Layer.ZOffset;

			const FTransform InstanceXf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, Z), FVector(Scale));
			Comp->AddInstance(InstanceXf, /*bWorldSpace=*/true);
			PlacedPoints.Add(TPair<FVector2D, float>(Candidate, FootprintR));
			++Placed;
			bPlacedThis = true;

			// Tree collision-proxy: add the paired proxy instance in LOCKSTEP with the
			// visual (same index) so the visual/proxy pair stays parallel for the
			// corridor cull. The proxy is scaled non-uniformly by CollisionProxyScale ×
			// the instance scale, and lifted by CollisionProxyZOffset × scale so a
			// centered-pivot cylinder grounds at the tree base.
			if (bUsesProxy)
			{
				if (UHierarchicalInstancedStaticMeshComponent* Proxy = ResolveProxyForVisual(Comp, Layer))
				{
					const FVector ProxyScaleVec = Layer.CollisionProxyScale * Scale;
					const float ProxyZ = Z + Layer.CollisionProxyZOffset * Scale;
					const FTransform ProxyXf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, ProxyZ), ProxyScaleVec);
					Proxy->AddInstance(ProxyXf, /*bWorldSpace=*/true);
				}
			}

			// Mirror-symmetric fallback mode: place the twin across X=0 (still
			// radius-aware keep-clear-checked for blocking layers). The twin's |X|,|Y|
			// equal the primary's, so it is already inside the field-edge clamp.
			if (bMirror)
			{
				const FVector2D MirrorPoint(-X, Y);
				bool bPlaceMirror = !(Layer.bBlocking && IsInKeepClear(MirrorPoint, FootprintR));
				float MirrorGroundZ = 0.f;
				if (bPlaceMirror)
				{
					// The twin grounds INDEPENDENTLY (hills are placed asymmetrically
					// even in mirror mode, so the twin's XY may sit on a different — or
					// no — hill): same floor trace + hill-surface upgrade as the
					// primary; an over-slope face at the twin's XY skips JUST the twin,
					// exactly like the keep-clear guard above.
					MirrorGroundZ = GroundZAt(-X, Y);
					if (Layer.bAllowOnHills && !ResolveHillAwareGroundZ(-X, Y, MirrorGroundZ, Layer.MaxPlacementSlopeDeg, MirrorGroundZ))
					{
						bPlaceMirror = false;
					}
				}
				if (bPlaceMirror)
				{
					const float MirrorZ = MirrorGroundZ + Layer.ZOffset;
					const float MirrorYaw = Layer.bRandomYaw ? FMath::Fmod(Yaw + 180.f, 360.f) : 0.f;
					const FTransform MirrorXf(FRotator(0.f, MirrorYaw, 0.f), FVector(-X, Y, MirrorZ), FVector(Scale));
					Comp->AddInstance(MirrorXf, /*bWorldSpace=*/true);
					PlacedPoints.Add(TPair<FVector2D, float>(MirrorPoint, FootprintR));
					++Placed;

					// Mirror proxy in lockstep (keeps the visual/proxy indices parallel).
					if (bUsesProxy)
					{
						if (UHierarchicalInstancedStaticMeshComponent* Proxy = ResolveProxyForVisual(Comp, Layer))
						{
							const FVector ProxyScaleVec = Layer.CollisionProxyScale * Scale;
							const float ProxyZ = MirrorZ + Layer.CollisionProxyZOffset * Scale;
							const FTransform ProxyXf(FRotator(0.f, MirrorYaw, 0.f), FVector(-X, Y, ProxyZ), ProxyScaleVec);
							Proxy->AddInstance(ProxyXf, /*bWorldSpace=*/true);
						}
					}
				}
			}
		}
	}

	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter] Layer '%s': placed %d instances (target %d, blocking=%s, meshVariants=%d)."),
		*Layer.LayerName.ToString(), Placed, Layer.InstanceCount,
		Layer.bBlocking ? TEXT("true") : TEXT("false"), Resolved.Num());
}

UHierarchicalInstancedStaticMeshComponent* ASiegeBattlefieldScatter::ResolveComponentForMesh(UStaticMesh* Mesh, const FScatterLayer& Layer)
{
	if (!Mesh)
	{
		return nullptr;
	}

	// Reuse the existing HISM for this mesh (one per unique mesh — the perf law).
	for (UHierarchicalInstancedStaticMeshComponent* Existing : ScatterComponents)
	{
		if (Existing && Existing->GetStaticMesh() == Mesh)
		{
			return Existing;
		}
	}

	UHierarchicalInstancedStaticMeshComponent* Comp = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	if (!Comp)
	{
		return nullptr;
	}

	// Mobility + attach + nav flag must be set BEFORE RegisterComponent so the
	// nav-relevance is computed correctly at registration.
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetupAttachment(RootComponent);
	Comp->SetStaticMesh(Mesh);

	// W1-PREP OverrideMaterial (CONVENTIONS "W1-PREP additions", TASK-250): a SET
	// override replaces the donor materials on EVERY slot via SetMaterial — the
	// SM_ asset itself is never touched (lane-clean: main-lane donors stay
	// pristine). ALL slots, not slot 0 only: the hill donors SM_Hill_01–03 are
	// single-slot ("HillGround"), where all-slots ≡ slot 0, and multi-slot donors
	// on other layers never strand a stray extra slot. Null (default) = donor
	// materials; a failed resolve degrades to the donor look with a warning —
	// never a crash. Applied ONCE at HISM creation (the reuse path above returns
	// early): a mesh SHARED by two layers keeps the FIRST layer's override
	// (one-HISM-per-mesh perf law — conflicting overrides on a shared mesh are a
	// DA config smell). TASK-249's M_HillGrass rides this on the HILLS layer.
	if (!Layer.OverrideMaterial.IsNull())
	{
		if (UMaterialInterface* OverrideMat = Layer.OverrideMaterial.LoadSynchronous())
		{
			const int32 NumSlots = FMath::Max(Comp->GetNumMaterials(), 1);
			for (int32 SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx)
			{
				Comp->SetMaterial(SlotIdx, OverrideMat);
			}
		}
		else
		{
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter] Layer '%s' OverrideMaterial '%s' failed to resolve — donor materials kept (degraded, never a crash)."),
				*Layer.LayerName.ToString(), *Layer.OverrideMaterial.ToString());
		}
	}

	// A layer with a CollisionProxyMesh (trees) delegates ALL blocking + nav to a
	// separate invisible proxy HISM (ResolveProxyForVisual): the VISIBLE mesh here
	// gets NO collision + no nav, so the canopy never carves an 8 m nav-blob.
	const bool bUsesProxy = !Layer.CollisionProxyMesh.IsNull();

	if (Layer.bBlocking && !bUsesProxy)
	{
		// REAL-GEOMETRY blocker (rocks / slabs / hills): the HISM's own geometry is
		// both the collider and the nav obstacle (CONVENTIONS "Climbable terrain
		// (M6.6)" scatter-channel law). Block Pawn (units/hero route AND climb),
		// Visibility + Camera (arrows die on the mound per decision #3, and the camera
		// never clips inside a mound when the hero stands on a crown), and set
		// bFillCollisionUnderneathForNavmesh so Recast fills the volume UNDER the hill
		// — units climb OVER the hill rather than the navmesh tunnelling through it.
		// ECC_WorldStatic RESPONSE stays IGNORE (from SetCollisionResponseToAllChannels
		// below): GroundZAt line-traces ECC_WorldStatic for the placement-ghost floor
		// height, so blocking WorldStatic would break placement. QueryOnly (not
		// QueryAndPhysics) — movement is query/sweep-based and nav reads collision
		// geometry, so no physics state is needed; leaner for the many statics (§6).
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Comp->SetCollisionObjectType(ECC_WorldStatic);
		Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
		Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Comp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Comp->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		Comp->bFillCollisionUnderneathForNavmesh = true;
		Comp->SetCanEverAffectNavigation(true);
	}
	else
	{
		// Either the GRASS layer (pure decoration) OR the VISUAL mesh of a proxy
		// layer (trees). No collision, no navmesh effect: for a proxy layer the
		// blocking + nav are carried by the paired invisible proxy HISM.
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
	}

	Comp->RegisterComponent();
	ScatterComponents.Add(Comp);
	return Comp;
}

UHierarchicalInstancedStaticMeshComponent* ASiegeBattlefieldScatter::ResolveProxyForVisual(UHierarchicalInstancedStaticMeshComponent* VisualComp, const FScatterLayer& Layer)
{
	if (!VisualComp)
	{
		return nullptr;
	}

	// Exactly one proxy per visual HISM — reuse it so the visual/proxy instance
	// indices stay parallel (the corridor cull removes matching indices from both).
	if (UHierarchicalInstancedStaticMeshComponent** Existing = VisualToProxy.Find(VisualComp))
	{
		return *Existing;
	}

	UStaticMesh* ProxyMesh = Layer.CollisionProxyMesh.LoadSynchronous();
	if (!ProxyMesh)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter] Layer '%s' CollisionProxyMesh '%s' failed to resolve — this layer's visuals have NO collider (degraded, never a crash)."),
			*Layer.LayerName.ToString(), *Layer.CollisionProxyMesh.ToString());
		return nullptr;
	}

	UHierarchicalInstancedStaticMeshComponent* Proxy = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	if (!Proxy)
	{
		return nullptr;
	}

	// Mobility + attach + nav flags must be set BEFORE RegisterComponent so nav
	// relevance is computed correctly at registration.
	Proxy->SetMobility(EComponentMobility::Movable);
	Proxy->SetupAttachment(RootComponent);
	Proxy->SetStaticMesh(ProxyMesh);

	// W1-PREP (TASK-250): the layer's OverrideMaterial applies to the PROXY too
	// (CONVENTIONS law: applied to "the layer's HISM + proxy components").
	// Cosmetically moot — the proxy never renders (SetVisibility(false) below) —
	// but it keeps the pair uniform if a proxy is ever un-hidden for debugging.
	// Null/failed resolve = silent no-op here (the visual path already warned).
	if (!Layer.OverrideMaterial.IsNull())
	{
		if (UMaterialInterface* OverrideMat = Layer.OverrideMaterial.LoadSynchronous())
		{
			const int32 NumSlots = FMath::Max(Proxy->GetNumMaterials(), 1);
			for (int32 SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx)
			{
				Proxy->SetMaterial(SlotIdx, OverrideMat);
			}
		}
	}

	// Invisible Pawn-block-only proxy (tree collision-proxy contract): it blocks ONLY
	// the Pawn channel — deliberately NOT Visibility/Camera, or the building placement
	// ghost (a Visibility/Camera trace) would snap to the invisible cylinder. Nav +
	// fill-underneath so units route around the slim trunk footprint (not the canopy).
	// Hidden + no shadow so the proxy never renders. WorldStatic RESPONSE stays Ignore,
	// so GroundZAt's WorldStatic trace passes through the proxy (correct floor height).
	Proxy->SetVisibility(false);
	Proxy->SetCastShadow(false);
	Proxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Proxy->SetCollisionObjectType(ECC_WorldStatic);
	Proxy->SetCollisionResponseToAllChannels(ECR_Ignore);
	Proxy->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Proxy->bFillCollisionUnderneathForNavmesh = true;
	Proxy->SetCanEverAffectNavigation(true);

	Proxy->RegisterComponent();
	ScatterComponents.Add(Proxy);        // rooted for GC + reached by ClearScatter
	VisualToProxy.Add(VisualComp, Proxy);
	return Proxy;
}

UHierarchicalInstancedStaticMeshComponent* ASiegeBattlefieldScatter::FindVisualForProxy(UHierarchicalInstancedStaticMeshComponent* ProxyComp) const
{
	if (!ProxyComp)
	{
		return nullptr;
	}
	for (const TPair<UHierarchicalInstancedStaticMeshComponent*, UHierarchicalInstancedStaticMeshComponent*>& Pair : VisualToProxy)
	{
		if (Pair.Value == ProxyComp)
		{
			return Pair.Key;
		}
	}
	return nullptr;
}

void ASiegeBattlefieldScatter::RebuildKeepClearZones()
{
	KeepClearZones.Reset();
	// No-config fallback mirrors the USiegeScatterConfig default (M7.6 ruling #2: corridor half-width 1000).
	CorridorHalfWidthCached = ScatterConfig ? FMath::Max(ScatterConfig->CorridorHalfWidth, 0.f) : 1000.f;

	UWorld* World = GetWorld();
	if (!World || !ScatterConfig)
	{
		return;
	}

	const float CastleR = ScatterConfig->CastleKeepClearRadius;
	const float NodeR = ScatterConfig->GoldNodeKeepClearRadius;
	const float StartR = ScatterConfig->PlayerStartKeepClearRadius;

	// Castles (live) — else the ±25000 fallbacks (M7.6 10× scale-up).
	bool bFoundBlueCastle = false;
	bool bFoundRedCastle = false;
	for (TActorIterator<ACastle> It(World); It; ++It)
	{
		ACastle* Castle = *It;
		if (!IsValid(Castle))
		{
			continue;
		}
		const FVector L = Castle->GetActorLocation();
		KeepClearZones.Add({ FVector2D(L.X, L.Y), CastleR * CastleR });
		if (Castle->GetTeamId() == ETeamId::Blue)
		{
			bFoundBlueCastle = true;
		}
		else
		{
			bFoundRedCastle = true;
		}
	}
	if (!bFoundBlueCastle)
	{
		KeepClearZones.Add({ FVector2D(-25000.f, 0.f), CastleR * CastleR });
	}
	if (!bFoundRedCastle)
	{
		KeepClearZones.Add({ FVector2D(25000.f, 0.f), CastleR * CastleR });
	}

	// Gold nodes (live) — else the ±24200 fallbacks (M7.6: nodes stay 800 in front of the ±25000 castles).
	bool bFoundBlueNode = false;
	bool bFoundRedNode = false;
	for (TActorIterator<AGoldNode> It(World); It; ++It)
	{
		AGoldNode* Node = *It;
		if (!IsValid(Node))
		{
			continue;
		}
		const FVector L = Node->GetActorLocation();
		KeepClearZones.Add({ FVector2D(L.X, L.Y), NodeR * NodeR });
		if (Node->GetTeam() == ETeamId::Blue)
		{
			bFoundBlueNode = true;
		}
		else
		{
			bFoundRedNode = true;
		}
	}
	if (!bFoundBlueNode)
	{
		KeepClearZones.Add({ FVector2D(-24200.f, 0.f), NodeR * NodeR });
	}
	if (!bFoundRedNode)
	{
		KeepClearZones.Add({ FVector2D(24200.f, 0.f), NodeR * NodeR });
	}

	// PlayerStart(s) (live) — else the ≈(-23800,0) fallback (M7.6: 1200 in front of Castle_Blue).
	bool bFoundStart = false;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* Start = *It;
		if (!IsValid(Start))
		{
			continue;
		}
		const FVector L = Start->GetActorLocation();
		KeepClearZones.Add({ FVector2D(L.X, L.Y), StartR * StartR });
		bFoundStart = true;
	}
	if (!bFoundStart)
	{
		KeepClearZones.Add({ FVector2D(-23800.f, 0.f), StartR * StartR });
	}
}

bool ASiegeBattlefieldScatter::IsInKeepClear(const FVector2D& Point2D, float InstanceRadius) const
{
	// The reserved central corridor: NO blocking obstacle inside |Y| <= half-width
	// across the whole X span — the deterministically-always-walkable Blue→Red lane.
	// Inflated by InstanceRadius so a WIDE instance's EDGE (not just its center) is
	// what must clear the corridor — the headline M6.6 fix for hills sprawling in.
	if (FMath::Abs(Point2D.Y) <= CorridorHalfWidthCached + InstanceRadius)
	{
		return true;
	}
	for (const FKeepClearZone& Zone : KeepClearZones)
	{
		// Inflate each keep-clear disc by the footprint radius: reject if the
		// instance's edge would enter the zone, i.e. DistSq <= (sqrt(RadiusSq)+R)^2.
		const float InflatedRadius = FMath::Sqrt(Zone.RadiusSq) + InstanceRadius;
		if (FVector2D::DistSquared(Point2D, Zone.Center) <= InflatedRadius * InflatedRadius)
		{
			return true;
		}
	}
	return false;
}

float ASiegeBattlefieldScatter::GroundZAt(float X, float Y) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	// Trace down to the arena floor; ignore this actor so already-placed scatter
	// instances never fool the trace. Flat slab ⇒ Z=0 is the correct fallback.
	const FVector TraceStart(X, Y, 50000.f);
	const FVector TraceEnd(X, Y, -50000.f);
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("BattlefieldScatterGroundTrace"), /*bTraceComplex=*/false, this);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		return Hit.ImpactPoint.Z;
	}
	return 0.f;
}

bool ASiegeBattlefieldScatter::ResolveHillAwareGroundZ(float X, float Y, float FloorZ, float MaxSlopeDeg, float& OutZ) const
{
	OutZ = FloorZ;

	// No hill surfaces registered this generate ⇒ the flat floor stands.
	if (HillSurfaceComponents.Num() == 0)
	{
		return true;
	}

	// COMPONENT-scoped down-traces, NOT a world channel trace (the bare-hills root
	// cause, TASK-250 diagnosis): no world trace can see a hill surface, because
	// (a) the hill HISMs IGNORE the ECC_WorldStatic trace channel (scatter-channel
	// law — GroundZAt must pass through them for the floor height), and (b) they
	// belong to THIS actor, which GroundZAt's query params ignore wholesale.
	// LineTraceComponent tests the instance bodies of exactly the registered
	// hill-surface HISMs — nothing else (rocks, invisible tree proxies, castles)
	// can fool the ground resolve, and the placement-ghost / projectile channel
	// contracts stay untouched.
	const FVector TraceStart(X, Y, 50000.f);
	const FVector TraceEnd(X, Y, -50000.f);
	FCollisionQueryParams Params(TEXT("BattlefieldScatterHillTrace"), /*bTraceComplex=*/false);

	bool bOnHill = false;
	FVector BestNormal = FVector::UpVector;
	float BestZ = FloorZ;
	for (UHierarchicalInstancedStaticMeshComponent* SurfaceComp : HillSurfaceComponents)
	{
		if (!SurfaceComp)
		{
			continue;
		}
		FHitResult Hit;
		// Highest hit wins: a hit at-or-below the floor means the trace clipped a
		// buried skirt, not a standable surface — the floor stands for those.
		if (SurfaceComp->LineTraceComponent(Hit, TraceStart, TraceEnd, Params) && Hit.ImpactPoint.Z > BestZ)
		{
			bOnHill = true;
			BestZ = static_cast<float>(Hit.ImpactPoint.Z); // explicit LWC double → float (placement grid precision is ample)
			BestNormal = Hit.ImpactNormal;
		}
	}

	if (!bOnHill)
	{
		return true; // not over a hill — the flat floor Z stands
	}

	// Slope gate: the face's angle from horizontal via the up-ness of its normal.
	// Steeper than the layer's MaxPlacementSlopeDeg ⇒ REJECT the candidate — the
	// caller must not fall back to FloorZ (that grounds the prop UNDER the hill,
	// buried inside the mound: the exact defect this resolve exists to fix).
	// Double all the way down: FVector is double-precision (LWC), and mixing a
	// double .Z with float literals inside the Clamp template would fail
	// deduction; the final compare promotes the float limit safely.
	const double SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(BestNormal.Z, -1.0, 1.0)));
	if (SlopeDeg > FMath::Max(MaxSlopeDeg, 0.f))
	{
		return false;
	}

	OutZ = BestZ;
	return true;
}

void ASiegeBattlefieldScatter::StartNavSettlePoll()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	NavSettleElapsed = 0.f;
	World->GetTimerManager().ClearTimer(TraversabilityTimerHandle);

	// No nav system ⇒ nothing to poll: a single fixed fallback wait, then run the
	// validation (which itself skips the nav confirmation gracefully — the
	// reserved corridor stays the deterministic traversability guarantee).
	if (!UNavigationSystemV1::GetCurrent(World))
	{
		World->GetTimerManager().SetTimer(TraversabilityTimerHandle, this,
			&ASiegeBattlefieldScatter::ValidateTraversability, FMath::Max(NavSettleFallbackDelay, 0.01f), false);
		return;
	}

	// First poll fires one interval from now (never same-frame), by which time the
	// dirty-area rebuild for the just-added/culled instances has registered — so an
	// early "idle" reading is a real idle, not a not-yet-started rebuild.
	World->GetTimerManager().SetTimer(TraversabilityTimerHandle, this,
		&ASiegeBattlefieldScatter::PollNavSettle, FMath::Max(NavPollInterval, 0.05f), false);
}

void ASiegeBattlefieldScatter::PollNavSettle()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Interval = FMath::Max(NavPollInterval, 0.05f);
	NavSettleElapsed += Interval;

	// Still building? Re-arm until idle or the cap. (The static helper is null-safe:
	// a nav system torn down mid-poll reads as "not building" and falls through to
	// the validation, whose own no-nav path degrades gracefully.)
	if (UNavigationSystemV1::IsNavigationBeingBuilt(World))
	{
		if (NavSettleElapsed < MaxNavSettleWait)
		{
			World->GetTimerManager().SetTimer(TraversabilityTimerHandle, this,
				&ASiegeBattlefieldScatter::PollNavSettle, Interval, false);
			return;
		}

		// Cap hit: proceed-with-warning — a pathologically slow rebuild must never
		// strand the reachability confirmation (worst case the check fails and the
		// widening-cull retry path polls again).
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] Navigation still building after %.1f s (MaxNavSettleWait cap) — proceeding with the reachability validation anyway."),
			*GetNameSafe(this), NavSettleElapsed);
	}

	ValidateTraversability();
}

void ASiegeBattlefieldScatter::ValidateTraversability()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Inset the endpoints toward the centerline onto open pad ground — the raw
	// castle center can sit inside the castle's own nav-carved hole (a false
	// negative). FindPathToLocationSynchronously still projects each endpoint to
	// the navmesh within its default query extent.
	FVector BlueLoc = ResolveCastleLocation(ETeamId::Blue);
	FVector RedLoc = ResolveCastleLocation(ETeamId::Red);
	BlueLoc.X -= FMath::Sign(BlueLoc.X) * CastleQueryInset; // Blue (X<0) moves toward 0
	RedLoc.X -= FMath::Sign(RedLoc.X) * CastleQueryInset;   // Red  (X>0) moves toward 0

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		// No nav system — the reserved corridor is still the deterministic
		// guarantee, so this is not a failure; just skip the confirmation.
		if (!bWarnedNoNav)
		{
			bWarnedNoNav = true;
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] No navigation system — nav reachability confirmation skipped; the reserved central corridor still guarantees a Blue→Red lane."),
				*GetNameSafe(this));
		}
		return;
	}

	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, BlueLoc, RedLoc);
	const bool bReachable = Path && Path->IsValid() && !Path->IsPartial();
	if (bReachable)
	{
		UE_LOG(LogSiegeTerrain, Log,
			TEXT("[BattlefieldScatter '%s'] Traversability CONFIRMED — Blue→Red castle path exists (after %d cull(s))."),
			*GetNameSafe(this), ReachabilityAttempt);
		return;
	}

	// Defensive re-roll: the reserved corridor should make this impossible, but if
	// a config edit shrank the corridor/radii, cull blocking instances in a
	// widening Y band around the lane, then re-check after the nav settles again.
	++ReachabilityAttempt;
	const float Band = CorridorHalfWidthCached + ReachabilityAttempt * CorridorWidenStep;
	const int32 Removed = CullCorridorBlockers(Band);
	UE_LOG(LogSiegeTerrain, Warning,
		TEXT("[BattlefieldScatter '%s'] Blue→Red path NOT found (attempt %d) — culled %d blocking instance(s) within |Y|<=%.0f; re-checking after nav settles."),
		*GetNameSafe(this), ReachabilityAttempt, Removed, Band);

	if (ReachabilityAttempt < MaxReachabilityAttempts)
	{
		// Wait for the post-cull nav re-carve to settle (poll to idle again), then re-check.
		StartNavSettlePoll();
	}
	else
	{
		// Final guarantee: the corridor band has been cleared of blockers, so the
		// straight Y≈0 lane is now obstacle-free even if the async nav has not yet
		// reported a path. Never leave a match unwinnable.
		UE_LOG(LogSiegeTerrain, Error,
			TEXT("[BattlefieldScatter '%s'] Reachability unconfirmed after %d culls; corridor force-cleared to |Y|<=%.0f as the final traversability guarantee (straight lane is obstacle-free)."),
			*GetNameSafe(this), ReachabilityAttempt, Band);
	}
}

int32 ASiegeBattlefieldScatter::CullCorridorBlockers(float Band)
{
	int32 TotalRemoved = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Comp : ScatterComponents)
	{
		// Only nav-relevant comps carve nav: real-geometry blockers (rocks/hills/
		// slabs) AND tree PROXY HISMs. A proxy-layer's VISUAL HISM has nav OFF, so it
		// is skipped HERE and culled in lockstep via its proxy below; grass (no-nav)
		// is irrelevant to pathing and also skipped by this same guard.
		if (!Comp || !Comp->CanEverAffectNavigation())
		{
			continue;
		}

		TArray<int32> ToRemove;
		const int32 Count = Comp->GetInstanceCount();
		for (int32 Idx = 0; Idx < Count; ++Idx)
		{
			FTransform InstanceXf;
			if (Comp->GetInstanceTransform(Idx, InstanceXf, /*bWorldSpace=*/true))
			{
				if (FMath::Abs(InstanceXf.GetLocation().Y) <= Band)
				{
					ToRemove.Add(Idx);
				}
			}
		}
		if (ToRemove.Num() > 0)
		{
			// If Comp is a tree PROXY, remove the SAME instance indices from its paired
			// VISUAL HISM in lockstep — otherwise this cull deletes the trunk collider
			// (nav-relevant) but leaves the visible tree standing with no blocker (the
			// visual/proxy desync flagged in the TASK-140 spec). Indices are parallel
			// by construction (every proxy instance is added alongside its visual), and
			// removing the same index set from both preserves that parallelism.
			if (UHierarchicalInstancedStaticMeshComponent* Visual = FindVisualForProxy(Comp))
			{
				Visual->RemoveInstances(ToRemove);
			}
			Comp->RemoveInstances(ToRemove);
			TotalRemoved += ToRemove.Num();
		}
	}
	return TotalRemoved;
}

FVector ASiegeBattlefieldScatter::ResolveCastleLocation(ETeamId Team) const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACastle> It(World); It; ++It)
		{
			ACastle* Castle = *It;
			if (IsValid(Castle) && Castle->GetTeamId() == Team)
			{
				return Castle->GetActorLocation();
			}
		}
	}
	// World axes, M7.6 10× scale-up (branch supersedes main's ±8000 law at merge): Blue -25000, Red +25000.
	return FVector((Team == ETeamId::Blue) ? -25000.f : 25000.f, 0.f, 0.f);
}
