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

	for (const FScatterLayer& Layer : ScatterConfig->Layers)
	{
		ScatterLayer(Layer, Stream);
	}

	// Defer the reachability confirmation so the async Dynamic navmesh update has
	// time to carve the newly-added obstacle instances before we path-test.
	ReachabilityAttempt = 0;
	World->GetTimerManager().ClearTimer(TraversabilityTimerHandle);
	World->GetTimerManager().SetTimer(TraversabilityTimerHandle, this,
		&ASiegeBattlefieldScatter::ValidateTraversability, FMath::Max(NavSettleDelay, 0.01f), false);
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
	for (UStaticMesh* Mesh : Resolved)
	{
		ResolveComponentForMesh(Mesh, Layer);
	}

	const float HalfX = FMath::Max(ScatterConfig->ArenaHalfExtent.X, 1.f);
	const float HalfY = FMath::Max(ScatterConfig->ArenaHalfExtent.Y, 1.f);
	const bool bMirror = ScatterConfig->bMirrorSymmetric;
	const float MinSpacingSq = Layer.MinSpacing * Layer.MinSpacing;
	const float ScaleLo = FMath::Min(Layer.ScaleRange.X, Layer.ScaleRange.Y);
	const float ScaleHi = FMath::Max(Layer.ScaleRange.X, Layer.ScaleRange.Y);

	// MinSpacing is per-layer, so only this layer's placed points matter.
	TArray<FVector2D> PlacedPoints;
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

			// Blocking obstacles honor keep-clear + the reserved corridor; grass ignores it.
			if (Layer.bBlocking && IsInKeepClear(Candidate))
			{
				continue;
			}

			bool bTooClose = false;
			if (MinSpacingSq > 0.f)
			{
				for (const FVector2D& P : PlacedPoints)
				{
					if (FVector2D::DistSquared(P, Candidate) < MinSpacingSq)
					{
						bTooClose = true;
						break;
					}
				}
			}
			if (bTooClose)
			{
				continue;
			}

			UStaticMesh* Mesh = Resolved[Stream.RandRange(0, Resolved.Num() - 1)];
			UHierarchicalInstancedStaticMeshComponent* Comp = ResolveComponentForMesh(Mesh, Layer);
			if (!Comp)
			{
				continue;
			}

			const float Scale = Stream.FRandRange(ScaleLo, ScaleHi);
			const float Yaw = Layer.bRandomYaw ? Stream.FRandRange(0.f, 360.f) : 0.f;
			const float Z = GroundZAt(X, Y) + Layer.ZOffset;

			const FTransform InstanceXf(FRotator(0.f, Yaw, 0.f), FVector(X, Y, Z), FVector(Scale));
			Comp->AddInstance(InstanceXf, /*bWorldSpace=*/true);
			PlacedPoints.Add(Candidate);
			++Placed;
			bPlacedThis = true;

			// Mirror-symmetric fallback mode: place the twin across X=0 (still
			// keep-clear-checked for blocking layers).
			if (bMirror)
			{
				const FVector2D MirrorPoint(-X, Y);
				if (!(Layer.bBlocking && IsInKeepClear(MirrorPoint)))
				{
					const float MirrorZ = GroundZAt(-X, Y) + Layer.ZOffset;
					const float MirrorYaw = Layer.bRandomYaw ? FMath::Fmod(Yaw + 180.f, 360.f) : 0.f;
					const FTransform MirrorXf(FRotator(0.f, MirrorYaw, 0.f), FVector(-X, Y, MirrorZ), FVector(Scale));
					Comp->AddInstance(MirrorXf, /*bWorldSpace=*/true);
					PlacedPoints.Add(MirrorPoint);
					++Placed;
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

	if (Layer.bBlocking)
	{
		// BLOCKING obstacle: block ONLY the Pawn channel so units/hero physically
		// block and route around; ignore every other channel so projectiles pass
		// through cosmetically (accepted M6.5 gap — projectile self-destruct reads
		// Obstacle-TAGGED actors, and HISM instances are not tagged actors).
		// bCanEverAffectNavigation carves the (Dynamic) navmesh. QueryOnly (not
		// QueryAndPhysics) — character movement is query/sweep-based and nav
		// generation reads the collision geometry, so no physics state is needed;
		// leaner for the many static instances (§6 perf budget).
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Comp->SetCollisionObjectType(ECC_WorldStatic);
		Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
		Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Comp->SetCanEverAffectNavigation(true);
	}
	else
	{
		// Pure decoration (grass): no collision, no navmesh effect.
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
	}

	Comp->RegisterComponent();
	ScatterComponents.Add(Comp);
	return Comp;
}

void ASiegeBattlefieldScatter::RebuildKeepClearZones()
{
	KeepClearZones.Reset();
	CorridorHalfWidthCached = ScatterConfig ? FMath::Max(ScatterConfig->CorridorHalfWidth, 0.f) : 400.f;

	UWorld* World = GetWorld();
	if (!World || !ScatterConfig)
	{
		return;
	}

	const float CastleR = ScatterConfig->CastleKeepClearRadius;
	const float NodeR = ScatterConfig->GoldNodeKeepClearRadius;
	const float StartR = ScatterConfig->PlayerStartKeepClearRadius;

	// Castles (live) — else CONVENTIONS ±8000 fallbacks.
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
		KeepClearZones.Add({ FVector2D(-8000.f, 0.f), CastleR * CastleR });
	}
	if (!bFoundRedCastle)
	{
		KeepClearZones.Add({ FVector2D(8000.f, 0.f), CastleR * CastleR });
	}

	// Gold nodes (live) — else CONVENTIONS ±7200 fallbacks.
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
		KeepClearZones.Add({ FVector2D(-7200.f, 0.f), NodeR * NodeR });
	}
	if (!bFoundRedNode)
	{
		KeepClearZones.Add({ FVector2D(7200.f, 0.f), NodeR * NodeR });
	}

	// PlayerStart(s) (live) — else CONVENTIONS ≈(-6800,0) fallback.
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
		KeepClearZones.Add({ FVector2D(-6800.f, 0.f), StartR * StartR });
	}
}

bool ASiegeBattlefieldScatter::IsInKeepClear(const FVector2D& Point2D) const
{
	// The reserved central corridor: NO blocking obstacle inside |Y| <= half-width
	// across the whole X span — the deterministically-always-walkable Blue→Red lane.
	if (FMath::Abs(Point2D.Y) <= CorridorHalfWidthCached)
	{
		return true;
	}
	for (const FKeepClearZone& Zone : KeepClearZones)
	{
		if (FVector2D::DistSquared(Point2D, Zone.Center) <= Zone.RadiusSq)
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
		World->GetTimerManager().SetTimer(TraversabilityTimerHandle, this,
			&ASiegeBattlefieldScatter::ValidateTraversability, FMath::Max(NavSettleDelay, 0.01f), false);
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
		// Only blocking obstacles carve nav; grass (no-nav) is irrelevant to pathing.
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
	// CONVENTIONS world axes (M6.5 4× widening): Blue -8000, Red +8000.
	return FVector((Team == ETeamId::Blue) ? -8000.f : 8000.f, 0.f, 0.f);
}
