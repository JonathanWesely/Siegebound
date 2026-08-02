// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/GoldNode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Siegebound/MinerUnit.h"
#include "TimerManager.h"

AGoldNode::AGoldNode()
{
	// no per-frame work: drain runs on a 1 s timer only while occupied
	// (TASK-004 never-per-tick law)
	PrimaryActorTick.bCanEverTick = false;

	// not damageable (standing law, TASK-025): the mine is scenery, not a
	// combatant — ApplyDamage routes are refused engine-side before any
	// TakeDamage runs. The raidable investment is the miner itself.
	SetCanBeDamaged(false);

	// root visual. Collision profile NoCollision + no overlaps + no navmesh
	// relevance: the mine blocks NOTHING (standing law — its collision must
	// never block miner arrival, it must not carve the navmesh it is the
	// destination of, and the corridor-mines-allowed ruling leans on exactly
	// this). Mobility stays Movable (the component default): a Static-mobility
	// component refuses SetStaticMesh once the world has begun play, which
	// would break the deferred-asset resolve in BeginPlay below.
	NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	SetRootComponent(NodeMesh);
	NodeMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	NodeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NodeMesh->SetGenerateOverlapEvents(false);
	NodeMesh->SetCanEverAffectNavigation(false);

	// visual contract: soft path, never a hard reference (the asset may not
	// exist in every content state; null-safe resolve below).
	NodeMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_GoldNode.SM_GoldNode")));
}

void AGoldNode::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// editor-time resolve so the mine is visible while placed/previewed.
	// Silent: OnConstruction re-runs on every editor property tweak — warning
	// here would spam.
	ResolveNodeMesh(/*bWarnIfMissing=*/ false);
}

void AGoldNode::BeginPlay()
{
	Super::BeginPlay();

	// runtime fallback for mines created before SM_GoldNode was imported
	// (level-loaded actors do not re-run construction scripts).
	ResolveNodeMesh(/*bWarnIfMissing=*/ true);

	// Gauge-denominator latch for mines that never receive InitMine (a
	// hand-placed/debug instance). Scatter-spawned mines get InitMine right
	// after spawn (TASK-255), which re-latches — both orders are safe because
	// InitMine fully resets the mine.
	if (InitialGoldReserve <= 0)
	{
		InitialGoldReserve = GoldReserve;
	}
	if (GoldReserve <= 0)
	{
		// an authored-empty mine starts life depleted (latch only — no miners
		// exist to evict, nothing binds the delegates yet)
		bDepleted = true;
	}

	// drive the gauge once so a non-full mine reads correctly from frame one
	// (full reserve writes GlowIntensityFull = 1.0 = the authored look)
	UpdateGlowGauge();
}

void AGoldNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// actor-destroy safety (ClearScatter's Play-Again mine teardown): the
	// looping drain timer must never outlive the mine. Registry/claim are
	// cleared WITHOUT notifying miners — at teardown time the Play-Again
	// sequence has already destroyed them (plan-of-record), and any straggler
	// sees its weak target go stale and re-seeks on its own poll.
	StopDrainTimer();
	ArrivedMiners.Reset();
	OccupyingTeam.Reset();

	Super::EndPlay(EndPlayReason);
}

void AGoldNode::InitMine(int32 InReserve)
{
	GoldReserve = FMath::Max(0, InReserve);
	InitialGoldReserve = GoldReserve;
	bDepleted = (GoldReserve == 0);

	// fresh mine: no tenants, no claim, no drain clock (a re-init while
	// occupied is not a designed path, but leaving a stale claim/timer behind
	// would violate the occupancy invariant, so reset unconditionally)
	StopDrainTimer();
	ArrivedMiners.Reset();
	OccupyingTeam.Reset();

	UpdateGlowGauge();
	OnMineReserveChanged.Broadcast(GoldReserve, InitialGoldReserve);
}

bool AGoldNode::CanTeamMine(ETeamId MinerTeam) const
{
	// depleted/empty mines refuse everyone; a claimed mine admits only the
	// claiming team (same-team miners stack); an unclaimed live mine admits
	// either team.
	return !bDepleted
		&& GoldReserve > 0
		&& (!OccupyingTeam.IsSet() || OccupyingTeam.GetValue() == MinerTeam);
}

bool AGoldNode::TryRegisterArrivedMiner(AMinerUnit* Miner)
{
	if (!IsValid(Miner))
	{
		return false;
	}

	// sweep stale entries FIRST: if every occupant died un-unregistered, the
	// claim releases here and the arriving team can take the mine over
	CompactArrivedMiners();

	const ETeamId MinerTeam = Miner->GetTeamId();
	if (!CanTeamMine(MinerTeam))
	{
		return false;
	}

	// defensive double-arrival: already registered is an idempotent success,
	// never a duplicate registry entry (a duplicate would double this miner's
	// drain contribution)
	if (ArrivedMiners.Contains(Miner))
	{
		return true;
	}

	const bool bWasEmpty = (ArrivedMiners.Num() == 0);
	ArrivedMiners.Add(Miner);

	if (bWasEmpty)
	{
		// the atomic 0 -> 1 claim: first arrival locks the mine to its team
		// and starts the drain clock
		OccupyingTeam = MinerTeam;
		StartDrainTimer();
	}

	return true;
}

void AGoldNode::UnregisterArrivedMiner(AMinerUnit* Miner)
{
	if (ArrivedMiners.Num() == 0)
	{
		return;
	}

	// remove this miner AND sweep stale entries in the same pass (a null
	// Miner still compacts — harmless)
	ArrivedMiners.RemoveAll([Miner](const TWeakObjectPtr<AMinerUnit>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Miner;
	});

	if (ArrivedMiners.Num() == 0)
	{
		// last occupant left/died: release the claim and stop the drain —
		// the mine is claimable by either team again (TASK-254's wait-mode
		// miners auto-claim on their next poll)
		OccupyingTeam.Reset();
		StopDrainTimer();
	}
}

AGoldNode* AGoldNode::FindBestMineFor(UWorld* World, ETeamId Team, const FVector& From)
{
	if (!World)
	{
		return nullptr;
	}

	AGoldNode* BestMinable = nullptr;
	float BestMinableDistSq = TNumericLimits<float>::Max();
	AGoldNode* BestWait = nullptr;
	float BestWaitDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AGoldNode> It(World); It; ++It)
	{
		AGoldNode* Mine = *It;
		if (!IsValid(Mine) || Mine->bDepleted || Mine->GoldReserve <= 0)
		{
			continue;
		}

		// 2D distance — the miner arrival metric (mines may sit on hills;
		// height must not skew "nearest"). Strict < keeps the first-found
		// mine on exact ties: iteration order is stable for a fixed world,
		// so the finder stays deterministic (no churn between equal options).
		const float DistSq = static_cast<float>(FVector::DistSquared2D(Mine->GetActorLocation(), From));

		if (Mine->CanTeamMine(Team))
		{
			// tier-1: minable NOW (unclaimed, or already ours)
			if (DistSq < BestMinableDistSq)
			{
				BestMinable = Mine;
				BestMinableDistSq = DistSq;
			}
		}
		else
		{
			// non-depleted but refused => enemy-occupied: tier-2 wait target
			if (DistSq < BestWaitDistSq)
			{
				BestWait = Mine;
				BestWaitDistSq = DistSq;
			}
		}
	}

	// tier-1 beats tier-2 at ANY distance; null = all depleted (or no mines):
	// the intended all-depleted income death — callers idle/skip, never crash
	return BestMinable ? BestMinable : BestWait;
}

AGoldNode* AGoldNode::FindBestMineInDisc(UWorld* World, ETeamId Team, const FVector& Center, float Radius, const FVector& From)
{
	// ⚠️ FindBestMineFor above is UNMODIFIED BY LAW (the bot shares it — CONVENTIONS
	// §5 + the TASK-400 QA criterion), so this is a deliberate write-out of the same
	// two-tier loop with ONE extra gate, not a refactor of it. Every selection rule
	// below is IDENTICAL to that function's on purpose: change one, change both.
	if (!World)
	{
		return nullptr;
	}

	// An empty disc contains nothing. This is also the structural reason a FOLLOW
	// group (PositionRadius == 0 by CONVENTIONS §1) can never be mistaken for a
	// position circle if one ever reached this function.
	if (Radius <= 0.f)
	{
		return nullptr;
	}

	const float RadiusSq = Radius * Radius;

	AGoldNode* BestMinable = nullptr;
	float BestMinableDistSq = TNumericLimits<float>::Max();
	AGoldNode* BestWait = nullptr;
	float BestWaitDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AGoldNode> It(World); It; ++It)
	{
		AGoldNode* Mine = *It;
		if (!IsValid(Mine) || Mine->bDepleted || Mine->GoldReserve <= 0)
		{
			continue;
		}

		// THE ONE EXTRA GATE (§5): the MINE's own location must lie inside the
		// position circle. 2D, matching every other zone disc in the project (the
		// group orders' PositionRadius/AttackRadius tests) and the miner's arrival
		// metric — the arena is flat and mines may sit on hills, so height must
		// never decide membership. Boundary INCLUSIVE (<=), the house convention
		// for a radius test.
		const float DiscDistSq = static_cast<float>(FVector::DistSquared2D(Mine->GetActorLocation(), Center));
		if (DiscDistSq > RadiusSq)
		{
			continue;
		}

		// From here down: byte-for-byte the FindBestMineFor selection. 2D distance
		// from the CALLER (not from the circle centre) so the miner still walks to
		// the nearest legal mine; strict < keeps the first-found mine on exact ties,
		// and iteration order is stable for a fixed world — that determinism is what
		// the caller's no-churn retarget rule rests on.
		const float DistSq = static_cast<float>(FVector::DistSquared2D(Mine->GetActorLocation(), From));

		if (Mine->CanTeamMine(Team))
		{
			// tier-1: minable NOW (unclaimed, or already ours)
			if (DistSq < BestMinableDistSq)
			{
				BestMinable = Mine;
				BestMinableDistSq = DistSq;
			}
		}
		else
		{
			// non-depleted but refused => enemy-occupied: tier-2 wait target
			if (DistSq < BestWaitDistSq)
			{
				BestWait = Mine;
				BestWaitDistSq = DistSq;
			}
		}
	}

	// tier-1 beats tier-2 at ANY distance INSIDE the circle. null = no mine in the
	// circle at all: a NORMAL answer here (unlike FindBestMineFor's null, which is
	// the all-depleted endgame) — the caller stations inside the circle instead.
	return BestMinable ? BestMinable : BestWait;
}

void AGoldNode::HandleDrainTick()
{
	// stale sweep: occupants that died without unregistering drop here; if
	// that empties the registry the claim releases inside the compaction
	CompactArrivedMiners();

	const int32 MinerCount = ArrivedMiners.Num();
	if (MinerCount == 0)
	{
		return;
	}

	const int32 NewReserve = FMath::Max(0, GoldReserve - MinerCount * DrainPerMinerPerSecond);
	if (NewReserve != GoldReserve)
	{
		GoldReserve = NewReserve;
		UpdateGlowGauge();
		OnMineReserveChanged.Broadcast(GoldReserve, InitialGoldReserve);
	}

	if (GoldReserve == 0)
	{
		Deplete();
	}
}

void AGoldNode::Deplete()
{
	// idempotence guard: a DrainPerMinerPerSecond tuned to 0 can tick at
	// reserve 0 without re-running the latch body
	if (bDepleted)
	{
		return;
	}

	// 1) latch + stop the clock
	bDepleted = true;
	GoldReserve = 0;
	StopDrainTimer();

	// 2) snapshot, then clear registry + claim BEFORE notifying: a notified
	//    miner may re-enter this mine (UnregisterArrivedMiner from its
	//    eviction path, FindBestMineFor from its re-seek) and must observe it
	//    already empty + depleted.
	TArray<TWeakObjectPtr<AMinerUnit>> EvictedMiners = MoveTemp(ArrivedMiners);
	ArrivedMiners.Reset();
	OccupyingTeam.Reset();

	// 3) per-miner eviction. AMinerUnit::NotifyMineDepleted is declared and
	//    implemented by TASK-254 (un-arrive: RemoveMinerIncome + clear the
	//    per-tenure arrival latch, stop the clink, re-seek) — this call site
	//    is the pinned contract; the batch links at TASK-258.
	int32 EvictedCount = 0;
	for (const TWeakObjectPtr<AMinerUnit>& WeakMiner : EvictedMiners)
	{
		if (AMinerUnit* EvictedMiner = WeakMiner.Get())
		{
			++EvictedCount;
			EvictedMiner->NotifyMineDepleted(this);
		}
	}

	// 4) broadcast, then gauge to the depleted ember
	OnMineDepleted.Broadcast(this);
	UpdateGlowGauge();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("AGoldNode '%s': depleted (initial reserve %d) — evicted %d miner(s)."),
		*GetNameSafe(this), InitialGoldReserve, EvictedCount);
}

void AGoldNode::CompactArrivedMiners()
{
	if (ArrivedMiners.Num() == 0)
	{
		return;
	}

	ArrivedMiners.RemoveAll([](const TWeakObjectPtr<AMinerUnit>& Entry)
	{
		return !Entry.IsValid();
	});

	if (ArrivedMiners.Num() == 0)
	{
		// every occupant died un-unregistered: restore the invariant
		// (OccupyingTeam set <=> registry non-empty)
		OccupyingTeam.Reset();
		StopDrainTimer();
	}
}

void AGoldNode::StartDrainTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	if (TimerManager.IsTimerActive(DrainTimerHandle))
	{
		// already draining: never re-arm mid-cycle (a reset would stretch the
		// current 1 s window and desync from the income clock)
		return;
	}

	TimerManager.SetTimer(DrainTimerHandle, this, &AGoldNode::HandleDrainTick, DrainTickIntervalSeconds, /*bLoop=*/ true);
}

void AGoldNode::StopDrainTimer()
{
	// null-safe on world: InitMine may run at odd lifecycle points, and
	// EndPlay must never assume a live world during teardown
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DrainTimerHandle);
	}
}

void AGoldNode::UpdateGlowGauge()
{
	// LAZY MID over slot 0 (M_GoldGlow) — created on the first call that
	// finds a resolved mesh with a slot-0 material, retried on later calls
	// otherwise (the deferred-asset resolve can land the mesh after BeginPlay
	// started the gauge). Every guard is a silent no-op: the gauge is
	// cosmetic, never load-bearing (the GlowIntensity param itself arrives
	// with TASK-257 — setting a nonexistent param on a MID is visually inert).
	if (!GlowMID)
	{
		if (!NodeMesh || !NodeMesh->GetStaticMesh())
		{
			return;
		}

		GlowMID = NodeMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (!GlowMID)
		{
			// no slot-0 material on the mesh — retry next update
			return;
		}
	}

	const float ReserveFraction = (InitialGoldReserve > 0)
		? FMath::Clamp(static_cast<float>(GoldReserve) / static_cast<float>(InitialGoldReserve), 0.0f, 1.0f)
		: 0.0f;

	// intensity MODULATION of the authored emissive — never a material
	// replacement (the glows-regardless-of-team law, CONVENTIONS Team contract)
	GlowMID->SetScalarParameterValue(GlowIntensityParamName,
		FMath::Lerp(GlowIntensityDepleted, GlowIntensityFull, ReserveFraction));
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
			TEXT("AGoldNode '%s': mesh '%s' failed to load — mine is invisible but fully functional."),
			*GetNameSafe(this), *NodeMeshAsset.ToString());
	}
}
