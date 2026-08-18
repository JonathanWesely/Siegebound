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
#include "NavigationData.h" // TASK-535: ANavigationData is the OnNavigationGenerationFinishedDelegate payload — complete type, read through GetNameSafe
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h" // M8 (TASK-356): DOREPLIFETIME for the seed pair
#include "TimerManager.h"
#include "Siegebound/AncientGround.h" // TASK-361: the ancient-ground pair this actor spawns
#include "Siegebound/Castle.h"
#include "Siegebound/GoldNode.h"
#include "Siegebound/ScatterConfig.h"
#include "Siegebound/SiegeNavAreas.h" // TASK-349: team object channels — re-typed combatant capsules must keep blocking scatter
#include "Siegebound/SiegeNavDiagnostics.h" // TASK-535/529 (NAV-§9 Stage 0): the three telemetry call sites

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

	/**
	 *  Value of the `mirror=` token in the ONE grep-able GenerateScatter
	 *  reproducibility line (TASK-358). The KEY is deliberately KEPT — QA's
	 *  existing grep for `GenerateScatter seed=… mirror=…` must keep matching —
	 *  and only the VALUE changes, from the retired bMirrorSymmetric true/false
	 *  to the symmetry-MODE name. Same seed + same binary ⇒ byte-identical line
	 *  on host and client.
	 */
	const TCHAR* SymmetryModeToken(EScatterSymmetryMode Mode)
	{
		switch (Mode)
		{
		case EScatterSymmetryMode::Rotational180: return TEXT("rot180");
		case EScatterSymmetryMode::Asymmetric:    return TEXT("asymmetric");
		default:                                  return TEXT("unknown");
		}
	}

	/**
	 *  Uniform spatial-hash grid that accelerates the per-layer radius-aware
	 *  MinSpacing rejection in ScatterLayer (TASK-284) — it replaces the old O(n²)
	 *  scan of EVERY placed instance with an O(1)-amortized scan of the 3×3 cell
	 *  block around a query candidate, so the scatter density can scale (the ≈4.9×
	 *  Phase-3 fill) without quadratic blowup.
	 *
	 *  EQUIVALENCE (the determinism law): the grid changes only HOW conflicting
	 *  placed points are found, never the accept/reject outcome. Init() sizes the
	 *  cell to `MinSpacing + 2 × (the layer's MAX possible footprint radius)`. The
	 *  rejection distance between a candidate (radius Rc) and a placed point (radius
	 *  Rp) is `MinSpacing + Rc + Rp`; since Rc,Rp are each ≤ MaxLayerR, any
	 *  conflicting point is strictly closer than the cell size, which forces it into
	 *  the 3×3 block (|Δcell| ≤ 1 per axis). AnyTooClose() then applies the SAME
	 *  pairwise inequality as the old scan, so it returns true iff the full scan
	 *  would have — byte-identical placement for a fixed seed, with zero change to
	 *  the FRandomStream draw sequence (the grid draws nothing).
	 */
	struct FScatterSpacingGrid
	{
		float CellSize = 1.f;
		float MinSpacing = 0.f; // the layer's base MinSpacing (added to both radii, exactly as the old scan)
		// Cell key (packed CX|CY) → the (2D center, footprint radius) of every placed point whose center falls in that cell.
		TMap<uint64, TArray<TPair<FVector2D, float>>> Cells;

		void Init(float InCellSize, float InMinSpacing)
		{
			// Floor at 1 uu so a degenerate all-zero-radius / zero-spacing layer never
			// divides by zero (in that case MinDist is always 0, so nothing is ever
			// rejected — the grid result trivially matches the old scan either way).
			CellSize = FMath::Max(InCellSize, 1.f);
			MinSpacing = InMinSpacing;
			Cells.Reset();
		}

		FORCEINLINE int32 CellCoord(float V) const
		{
			return FMath::FloorToInt(V / CellSize);
		}

		// Pack two int32 cell coords into one 64-bit key via their uint32 bit patterns
		// (collision-free across the whole int32 × int32 range, negatives included).
		FORCEINLINE static uint64 CellKey(int32 CX, int32 CY)
		{
			return (static_cast<uint64>(static_cast<uint32>(CX)) << 32) | static_cast<uint64>(static_cast<uint32>(CY));
		}

		void Add(const FVector2D& Center, float Radius)
		{
			Cells.FindOrAdd(CellKey(CellCoord(Center.X), CellCoord(Center.Y))).Add(TPair<FVector2D, float>(Center, Radius));
		}

		// Exact replica of the old rejection test, restricted to the 3×3 cell block:
		// true iff some placed point P satisfies dist(P, Candidate) < MinSpacing +
		// CandRadius + P.radius (with that bound > 0). Byte-identical to the full scan.
		bool AnyTooClose(const FVector2D& Candidate, float CandRadius) const
		{
			const int32 CX = CellCoord(Candidate.X);
			const int32 CY = CellCoord(Candidate.Y);
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					if (const TArray<TPair<FVector2D, float>>* Bucket = Cells.Find(CellKey(CX + DX, CY + DY)))
					{
						for (const TPair<FVector2D, float>& Other : *Bucket)
						{
							const float MinDist = MinSpacing + CandRadius + Other.Value;
							if (MinDist > 0.f && FVector2D::DistSquared(Other.Key, Candidate) < MinDist * MinDist)
							{
								return true;
							}
						}
					}
				}
			}
			return false;
		}
	};
}

ASiegeBattlefieldScatter::ASiegeBattlefieldScatter()
{
	PrimaryActorTick.bCanEverTick = false;

	// M8 (TASK-356 doc D9/§3.5): the level-placed scatter actor replicates so the
	// authority's chosen seed + generation index reach clients — host and client
	// generate IDENTICAL battlefields (per-machine random seeds were the audit's
	// mismatched-collision bug). The instances themselves are never replicated;
	// only the two ints are. Standalone: no connections ⇒ zero cost.
	bReplicates = true;

	// ⚖️ NET RELEVANCY — TIER A (CONVENTIONS NET RELEVANCY LAW; TASK-356 loop-1
	// BLOCKER 1 fix). This actor is a SINGLETON whose two replicated ints DRIVE
	// WORLD GENERATION on every machine — the textbook Tier-A case. It is also a
	// POINT actor sitting at the world origin while both players fight ~250 m
	// away, so UE's default 150 m distance relevancy made it permanently
	// irrelevant: at the two-client gate `OnRep_GenerationIndex` never fired and
	// the client ran with ZERO obstacles and ZERO gold nodes (host: 6).
	//
	// ⚠️ D9 CORRECTION (recorded in code so the signed doc is never misread):
	// the "client obstacles are a SUPERSET of the server's ⇒ never rubber-bands"
	// argument in TASK-353 §3.5 is VOID unless the seed actually ARRIVES. Under
	// default relevancy the client got a strict SUBSET (zero) — the exact
	// inversion the design promised to avoid. **Tier-A membership is that
	// argument's precondition**; this line is what makes the doc's reasoning true.
	bAlwaysRelevant = true;

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

void ASiegeBattlefieldScatter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Doc D9/§3.5: the seed pair. Same-bunch atomicity — ChosenSeed is applied
	// before OnRep_GenerationIndex fires, so the regen always reads a fresh seed.
	DOREPLIFETIME(ASiegeBattlefieldScatter, ChosenSeed);
	DOREPLIFETIME(ASiegeBattlefieldScatter, GenerationIndex);
}

void ASiegeBattlefieldScatter::BeginPlay()
{
	Super::BeginPlay();

	// ── STAGE 0 TELEMETRY (TASK-535 wiring TASK-529's library; CONVENTIONS `NAV-§9`) ──
	// Two pure reads + two log lines. ⛔ NO behaviour change: FSiegeNavDiagnostics
	// holds no state, ticks nothing and writes to no engine object.
	// ⭐ THE CONFIG LINE IS THE BATCH'S ACCEPTANCE TEST. Its `gatherOnGameThread=`
	// token is read OFF THE RUNNING ARecastNavMesh, and it is the ONLY way to prove
	// whether the TASK-530 ini flip reached the SERIALIZED L_Arena nav actor — the
	// exact trap Config/DefaultEngine.ini:281-284 documents in its own comment. A PIE
	// log still reading `gatherOnGameThread=false` after the flip is TASK-540's
	// rollback trigger (`NAV-§9` clause 3), ⛔ never an excuse for an L_Arena save.
	// Taken BEFORE GenerateScatter so `pre-scatter` genuinely is the pre-scatter
	// queue depth — it is the baseline the `post-scatter` dirty spike is read against.
	// Deliberately NOT authority-gated: a client's nav actor carries its own
	// serialized config, and a client-side mismatch is exactly the kind of thing
	// this line exists to make visible. Both calls are null-World safe.
	UWorld* const TelemetryWorld = GetWorld();
	FSiegeNavDiagnostics::LogNavConfigOnce(TelemetryWorld);
	FSiegeNavDiagnostics::LogNavBuildSnapshot(TelemetryWorld, TEXT("pre-scatter"));

	// Match-start scatter (Jonathan: "randomly generated at the start of each match").
	// M8 (TASK-356 doc §3.5): AUTHORITY-gated — a client instance NEVER
	// self-generates (its local random seed is the different-battlefields bug);
	// it waits for OnRep_GenerationIndex, which arrives with the authority's
	// ChosenSeed (join-in-progress included: index >= 1 vs the CDO's 0 fires the
	// OnRep off the initial rep). Standalone: authority ⇒ byte-identical.
	if (HasAuthority())
	{
		GenerateScatter();
	}
	else
	{
		UE_LOG(LogSiegeTerrain, Log,
			TEXT("[BattlefieldScatter '%s'] Client copy — waiting for the replicated seed (OnRep_GenerationIndex; M8 doc D9)."),
			*GetNameSafe(this));
	}
}

void ASiegeBattlefieldScatter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraversabilityTimerHandle);
		// TASK-535: the definitive post-settle re-check rides its OWN handle.
		World->GetTimerManager().ClearTimer(DefinitiveCheckTimerHandle);
	}

	// TASK-535 (`NAV-§4`): clean teardown of the nav-generation-finished binding. A
	// dynamic delegate holds the object by name+pointer, so a binding that outlives
	// this actor is a callback into a destroyed actor on the next world's build.
	UnbindNavGenerationFinished();

	Super::EndPlay(EndPlayReason);
}

void ASiegeBattlefieldScatter::GenerateScatter()
{
	// M8 authority guard (TASK-356 doc §3.5): a client's local random seed is the
	// very bug D9 exists to kill — client regeneration ONLY ever happens through
	// OnRep_GenerationIndex with the replicated seed. Standalone: authority ⇒
	// this guard is a provable no-op (doc §10).
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] GenerateScatter refused on a non-authority copy — the client regenerates from the replicated seed (M8 doc D9)."),
			*GetNameSafe(this));
		return;
	}

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

	// M8 (doc D9/§3.5): publish the chosen seed + bump the regen trigger. The
	// index bumps EVERY generate — a repeated seed (bReRandomizeOnMatchReset
	// false) still fires the client OnRep, and a join-in-progress client sees
	// index >= 1 and regenerates off its initial rep. Standalone: two int writes
	// nothing reads.
	ChosenSeed = Seed;
	++GenerationIndex;

	RunScatterPasses(Seed, /*bAuthoritativeGenerate=*/ true);

	// ── STAGE 0 TELEMETRY (TASK-535/529, `NAV-§9`) — THE DIRTY-COUNT SPIKE ──
	// Every blocking instance of this generate (738 in the shipped DA) has just
	// landed, so this snapshot is the whole scatter's nav cost in one line:
	// `dirtyAreas=`/`remaining=` here against `pre-scatter` is what turns "the
	// navmesh rebuilds slowly" from a claim into a measured number, and
	// `activeTiles=` against `poolCap=` is the ONLY instrument on the `NAV-§6`
	// tile-pool hazard. Read-only; one line; the AUTHORITY path only, because the
	// client's spike is its own OnRep-driven generate.
	FSiegeNavDiagnostics::LogNavBuildSnapshot(World, TEXT("post-scatter"));
}

void ASiegeBattlefieldScatter::OnRep_GenerationIndex()
{
	// CLIENT regen (TASK-356 doc §3.5): ChosenSeed rode the same bunch (applied
	// before this fires — atomic pair). Deterministic passes only; the nav
	// validation stays authority-side (D9 residual accepted: client obstacles
	// are a superset in the rare defensively-culled match — never rubber-bands).
	if (!ScatterConfig)
	{
		if (!bWarnedNoConfig)
		{
			bWarnedNoConfig = true;
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] No ScatterConfig assigned on the CLIENT copy — cannot mirror the server battlefield (DA_BattlefieldScatter unassigned)."),
				*GetNameSafe(this));
		}
		return;
	}

	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter '%s'] Client regen: replicated seed=%d generation=%d (M8 doc D9)."),
		*GetNameSafe(this), ChosenSeed, GenerationIndex);

	ClearScatter();
	LastSeed = ChosenSeed;
	bHasSeed = true;
	RunScatterPasses(ChosenSeed, /*bAuthoritativeGenerate=*/ false);
}

void ASiegeBattlefieldScatter::RunScatterPasses(int32 Seed, bool bAuthoritativeGenerate)
{
	FRandomStream Stream(Seed);

	// Build the keep-clear discs (castles/nodes/PlayerStart) + cache the corridor
	// half-width for this generate. Deterministic inputs on both machines: the
	// castles are level-placed at identical transforms and the config is the same
	// asset — so the client's discs match the server's (M8 doc §3.5).
	RebuildKeepClearZones();

	// The one grep-able reproducibility line (CONVENTIONS "Logging": LogSiegeTerrain).
	// M8: identical on both machines for the same seed — the TASK-357 layout
	// comparison greps this line + the MinesPass line on both instances.
	// TASK-358: the `mirror=` KEY is preserved on purpose (QA's grep) — its VALUE
	// is now the symmetry-MODE name (`rot180` / `asymmetric`), not true/false.
	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter '%s'] GenerateScatter seed=%d mirror=%s layers=%d corridorHalfY=%.0f"),
		*GetNameSafe(this), Seed, SymmetryModeToken(ScatterConfig->SymmetryMode),
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

	// W1-PREP rotated depleting mines (TASK-255): AFTER pass 2 — the hill surfaces
	// + hill-riding props exist AND (TASK-358) the hill field is already complete
	// with its rotational twins, which is what makes the mine pair's two grounding
	// traces provably agree — and BEFORE StartNavSettlePoll, so the clearance-disc
	// culls are part of the nav rebuild the reachability validation waits on. Uses
	// its own dedicated stream (seed-order law) — the layer Stream above is
	// untouched by this call. M8: the client
	// spawns its own LOCAL mine pair actors at identical deterministic positions
	// (AGoldNode state replication is P2 — accepted P1 gap, doc §3.5).
	PlaceMines(Seed);

	// ANCIENT GROUNDS (TASK-361 — CONVENTIONS §2 "Placement"): IMMEDIATELY after
	// the mines, and inside this shared seed-deterministic body so the client's
	// OnRep_GenerationIndex path re-runs it identically. The order is load-bearing
	// in one direction only: the pass keeps its distance from the SPAWNED mines,
	// so the mine set must already exist. It uses its own dedicated stream (the
	// seed-order law) — neither the layer Stream above nor the mine stream is
	// touched, so every existing layout is preserved draw-for-draw.
	//
	// ⚠️ bAuthoritativeGenerate IS THREADED THROUGH, not re-derived. The grounds
	// are spawned locally on the CLIENT too (they ride the replicated seed exactly
	// like the mines) and therefore keep ROLE_Authority there — so the boost sim
	// can only be disabled by PUSHING this machine's authority decision into
	// InitAncientGround. See PlaceAncientGrounds' header comment.
	PlaceAncientGrounds(Seed, bAuthoritativeGenerate);

	// AUTHORITY ONLY (M8 doc D9): the deferred reachability confirmation depends
	// on live navmesh queries + the widening-cull attempt counter — not client-
	// reproducible. The client's layout is the server's seed-deterministic
	// superset; it never validates or culls.
	if (!bAuthoritativeGenerate)
	{
		UE_LOG(LogSiegeTerrain, Log,
			TEXT("[BattlefieldScatter '%s'] Client passes complete (seed=%d) — nav validation/culls are authority-only (M8 doc D9)."),
			*GetNameSafe(this), Seed);
		return;
	}

	// Defer the reachability confirmation until the async Dynamic navmesh has
	// actually finished carving the newly-added obstacle instances: poll
	// IsNavigationBeingBuilt until the nav system reports idle (M7.6 — a fixed
	// delay guessed wrong at the ~9.8× field), capped by MaxNavSettleWait.
	// StartNavSettlePoll clears any pending timer itself.
	ReachabilityAttempt = 0;

	// TASK-535 (`NAV-§4`): re-arm the DEFINITIVE post-settle check for this generate.
	// Play Again runs the whole path again, so every latch resets here rather than at
	// construction — a stale `done` latch would silently disarm the guarantee on the
	// second match. The pending timer is cleared for the same reason: a re-check armed
	// against the PREVIOUS layout must never land on this one.
	bDefinitiveCheckPending = false;
	bDefinitiveCheckDone = false;
	bInDefinitiveCheck = false;
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DefinitiveCheckTimerHandle);
	}
	BindNavGenerationFinished();

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

	// Play-Again mine lifecycle (TASK-255): the spawned pair actors are DESTROYED,
	// not pooled — every re-scatter gets exactly-fresh mines (fresh reserve, no
	// depletion latch, no stale claims). PlayAgain kills all miners in its step 2
	// BEFORE this step-7 re-scatter (plan-of-record law), so no live registries
	// dangle; AGoldNode::EndPlay clears its own drain timer and deliberately
	// skips miner notification (TASK-253).
	for (const TObjectPtr<AGoldNode>& Mine : SpawnedMines)
	{
		AGoldNode* MinePtr = Mine.Get();
		if (IsValid(MinePtr))
		{
			MinePtr->Destroy();
		}
	}
	SpawnedMines.Reset();

	// Ancient-ground lifecycle (TASK-361) — EXACTLY the mine lifecycle above:
	// destroyed, never pooled, so every re-scatter places a fresh pair at a fresh
	// location AND re-runs the authority push on the new actors (a pooled ground
	// could carry a stale bAuthoritativeBoost across a Play Again). AAncientGround
	// latches no state and clears its own boost timer in EndPlay, so there is
	// nothing else to unwind — no SiegeGameMode edit is needed anywhere.
	for (const TObjectPtr<AAncientGround>& Ground : SpawnedAncientGrounds)
	{
		AAncientGround* GroundPtr = Ground.Get();
		if (IsValid(GroundPtr))
		{
			GroundPtr->Destroy();
		}
	}
	SpawnedAncientGrounds.Reset();
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

	// 180°-ROTATIONAL SYMMETRY (TASK-358 — CONVENTIONS "Ancient Grounds +
	// Sorcerer + 180° terrain symmetry" §1, the ONLY legal terrain symmetry on
	// this project). Generate the BLUE half (X <= 0) and emit each instance's
	// twin under a proper rigid rotation about the map center:
	//     loc' = (-X, -Y, Z)   yaw' = Fmod(yaw + 180, 360)   scale' = scale
	// On the yaw-only / uniform-scale transforms every scatter instance uses this
	// is EXACT: no negative scale, no HISM winding flip, no approximation (the
	// retired bMirrorSymmetric X-mirror was a FAKE reflection and is dead).
	const bool bRotSym = (ScatterConfig->SymmetryMode == EScatterSymmetryMode::Rotational180);
	const float DrawMaxX = bRotSym ? 0.f : HalfX;

	const bool bUsesProxy = !Layer.CollisionProxyMesh.IsNull();
	const float ScaleLo = FMath::Min(Layer.ScaleRange.X, Layer.ScaleRange.Y);
	const float ScaleHi = FMath::Max(Layer.ScaleRange.X, Layer.ScaleRange.Y);

	// Radius-aware MinSpacing acceleration (TASK-284): a uniform spatial-hash grid
	// replaces the old O(n²) scan of every placed point. Placed points are still
	// (2D center, footprint radius) — a candidate is rejected when its center is
	// closer than MinSpacing + both radii to a placed instance (wide hills stop
	// interpenetrating) — but the grid restricts each query to a 3×3 cell block.
	// The cell size is MinSpacing + 2 × the layer's MAX possible footprint radius,
	// so every point the old full scan could reject on lands in that block; the
	// accept/reject outcome is byte-identical and no FRandomStream draw moves (the
	// grid draws nothing). MaxLayerR bounds every instance's radius:
	//  - explicit FootprintRadius override ⇒ every instance shares that exact radius;
	//  - auto-derive ⇒ meshHalfDiag × scale, bounded by maxHalfDiag × ScaleHi (the
	//    SAME GetBounds()/half-diagonal formula the per-instance FootprintR uses).
	float MaxLayerR;
	if (Layer.FootprintRadius > 0.f)
	{
		MaxLayerR = Layer.FootprintRadius;
	}
	else
	{
		float MaxHalfDiag = 0.f;
		for (UStaticMesh* Mesh : Resolved)
		{
			const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
			MaxHalfDiag = FMath::Max(MaxHalfDiag,
				static_cast<float>(FVector2D(MeshBounds.BoxExtent.X, MeshBounds.BoxExtent.Y).Size()));
		}
		MaxLayerR = MaxHalfDiag * ScaleHi;
	}
	FScatterSpacingGrid SpacingGrid;
	SpacingGrid.Init(Layer.MinSpacing + 2.f * MaxLayerR, Layer.MinSpacing);

	// TARGET-COUNT SEMANTICS (CONVENTIONS §1: "per-layer TargetCount already counts
	// primary + twin, so a target of 340 becomes ~170 pairs"). Under the rotational
	// law each outer iteration emits a PAIR, so the ITERATION budget halves while
	// the layer still ships ~InstanceCount instances — the perf budget (~15,000
	// AddInstance calls across the DA) is IDENTICAL to the old asymmetric field,
	// and Jonathan's "generate one half" algorithm costs half the RNG draws and
	// half the ground traces. DivideAndRoundUp so an ODD target never ships a pair
	// short (341 ⇒ 171 pairs ⇒ 342 instances). Each iteration keeps the FULL
	// MaxPlacementAttemptsPerInstance rejection budget — unchanged.
	const int32 OuterTarget = bRotSym
		? FMath::DivideAndRoundUp<int32>(Layer.InstanceCount, 2)
		: Layer.InstanceCount;

	int32 Placed = 0;
	// Asymmetry-escape + free-assertion counters (reported on the layer log line).
	// Both are PROVABLY 0 under the shipped level geometry — see the twin block —
	// so a non-zero value is a real signal for QA, not noise.
	int32 TwinSkipped = 0;
	int32 TwinZMismatch = 0;
	for (int32 InstanceIndex = 0; InstanceIndex < OuterTarget; ++InstanceIndex)
	{
		bool bPlacedThis = false;
		for (int32 Attempt = 0; Attempt < MaxPlacementAttemptsPerInstance && !bPlacedThis; ++Attempt)
		{
			// ⚠️ BLUE HALF ONLY (CONVENTIONS §1 — NOT NEGOTIABLE, and not arbitrary):
			// PlayerStart exists ONLY at (-23800, 0, 100); there is no Red-side
			// PlayerStart, so generating the RED half would rotate a legally-placed
			// prop straight ONTO the hero spawn. Generating Blue honors the real
			// keep-clear disc and its rotated image over-clears a harmless empty
			// patch at (+23800, 0). The mines pass already draws X < 0 — consistent,
			// not new. Still EXACTLY ONE draw: only the RANGE narrows, so the draw
			// sequence's SHAPE (and therefore the determinism contract) is untouched.
			const float X = Stream.FRandRange(-HalfX, DrawMaxX);
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
			// sprawls across the corridor.
			if (Layer.bBlocking && IsInKeepClear(Candidate, FootprintR))
			{
				continue;
			}

			// R-A1 (TASK-623): NON-blocking layers (Grass/Plants) now honor the
			// keep-clear DISCS ONLY, inflated by FootprintR through the SAME
			// IsInKeepClearDiscs inflation the blocking path uses. ⛔ The corridor
			// band is deliberately NOT applied here — the Blue→Red lane stays lush;
			// only the keep-clear pads lose decoration. Radius authority: the live
			// CastleKeepClearRadius 4,500 is DA-SERIALIZED in DA_BattlefieldScatter
			// (CR-R6's W8-R3 correction), not merely the ScatterConfig.h C++ default —
			// a future header re-derivation alone will NOT move this disc. Kills the
			// measured in-footprint flora at the root (TASK-617 A7: ~170 z<10 grass
			// per castle footprint + ~104 grass / ~17 plants per apron, every seed).
			if (!Layer.bBlocking && IsInKeepClearDiscs(Candidate, FootprintR))
			{
				continue;
			}

			// Radius-aware MinSpacing (TASK-284 grid-hash accelerated): reject if any
			// placed instance's center is closer than MinSpacing + this instance's
			// radius + the other's radius. The spatial hash restricts the scan to the
			// 3×3 cell block; because the cell size bounds the max interaction distance,
			// that block contains EVERY point the old full O(n²) scan would have found —
			// same accept/reject, no draw-sequence change (the grid draws nothing).
			if (SpacingGrid.AnyTooClose(Candidate, FootprintR))
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
			SpacingGrid.Add(Candidate, FootprintR); // TASK-284: register in the spatial hash (same data the old PlacedPoints held)
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

			// ═══ THE 180° ROTATIONAL TWIN (TASK-358) ═══════════════════════════════
			// Emitted INLINE, inside the instance loop — NEVER as a bulk post-pass.
			// A post-pass breaks three SHIPPED invariants:
			//   (1) VisualToProxy index parallelism — the tree visual and its
			//       collision proxy must be added in LOCKSTEP or CullCorridorBlockers /
			//       RemoveBlockingInstancesInDisc orphan visible trees with no blocker;
			//   (2) SpacingGrid registration of the twin, so LATER primaries in this
			//       same layer respect it (a post-pass twin would be invisible to the
			//       spacing law and could interpenetrate);
			//   (3) HillSurfaceComponents registration for pass-1 blockers — the hill
			//       field must be COMPLETE (primaries AND twins) before pass-2 layers
			//       trace against it, which is exactly what makes the twin's ground Z
			//       provably equal to the primary's (see the assertion below).
			// ZERO FRandomStream DRAWS IN HERE — the twin is COMPUTED, never sampled.
			// That is what preserves intra-build determinism and host==client agreement
			// (the same discipline PlaceMines already enforces).
			if (bRotSym)
			{
				const FVector2D TwinPoint(-X, -Y);

				// The twin's |X| and |Y| equal the primary's, so the field-edge clamp
				// above already covers it (a rotation about the center preserves both
				// magnitudes) — no re-test needed.
				//
				// Keep-clear IS re-tested, defensively. Under the shipped level it can
				// never reject: the corridor test reads |−Y| = |Y| (identical answer),
				// the two castle discs are an exact rotational pair at ±25000 sharing
				// one CastleKeepClearRadius (so twin-vs-Red ≡ primary-vs-Blue), and the
				// lone asymmetric disc — PlayerStart at (−23800, 0) — is unreachable by
				// a twin, which always has X >= 0. The guard stays because it costs
				// nothing and a future level edit (a moved castle, a second
				// PlayerStart) would otherwise silently place into a keep-clear zone.
				bool bPlaceTwin = !(Layer.bBlocking && IsInKeepClear(TwinPoint, FootprintR));
				// R-A1 (TASK-623): the non-blocking DISC re-test, mirroring the primary
				// site (discs only — the corridor is never applied to non-blocking
				// layers). Same defensive character as the blocking guard above: the
				// castle discs are an exact rotational pair and the PlayerStart disc is
				// unreachable by a twin (X >= 0), so under the shipped level this can
				// never reject — it guards the same future level edits, for free. A
				// rejection lands in the existing TwinSkipped asymmetry-escape counter.
				if (bPlaceTwin && !Layer.bBlocking && IsInKeepClearDiscs(TwinPoint, FootprintR))
				{
					bPlaceTwin = false;
				}
				float TwinGroundZ = 0.f;
				if (bPlaceTwin)
				{
					// Z IS RE-TRACED, NEVER COPIED (CONVENTIONS §1). It keeps the
					// null-safe code shape AND turns an assumption into a FREE QA
					// ASSERTION: under an exact rotation this MUST return the primary's
					// Z. Why it must — the hill field is itself rotationally symmetric
					// (pass 1 emits every hill's twin before any pass-2 layer traces),
					// the arena floor is a flat slab, and a Z-axis rotation leaves both
					// the surface Z and the normal's Z component INVARIANT, so the
					// slope gate acos(N.Z) returns the identical answer at both points.
					TwinGroundZ = GroundZAt(-X, -Y);
					if (Layer.bAllowOnHills && !ResolveHillAwareGroundZ(-X, -Y, TwinGroundZ, Layer.MaxPlacementSlopeDeg, TwinGroundZ))
					{
						bPlaceTwin = false;
					}
					else if (!FMath::IsNearlyEqual(TwinGroundZ, GroundZ, 1.f))
					{
						// The assertion FIRED: the field is not rotationally symmetric
						// where it must be. Place anyway at the honest traced Z (never
						// float an instance), but make it loud — this is a real defect
						// signal, not cosmetic drift. Only the first few are printed
						// (a systemic break would otherwise emit thousands of lines and
						// bury the rest of the generate log); the TOTAL always reaches
						// the layer's summary line as `zMismatch=`.
						++TwinZMismatch;
						if (TwinZMismatch <= 3)
						{
							UE_LOG(LogSiegeTerrain, Warning,
								TEXT("[BattlefieldScatter] Layer '%s' SymmetryAssert: twin ground Z mismatch at P=(%.0f,%.0f) Z=%.1f vs P'=(%.0f,%.0f) Z=%.1f — the rotated field is NOT symmetric here."),
								*Layer.LayerName.ToString(), X, Y, GroundZ, -X, -Y, TwinGroundZ);
						}
					}
				}

				if (bPlaceTwin)
				{
					const float TwinZ = TwinGroundZ + Layer.ZOffset;
					// yaw + 180 UNCONDITIONALLY — a proper rigid rotation rotates the
					// mesh too, so this is NOT gated on bRandomYaw (the retired mirror
					// block zeroed it for fixed-yaw layers; that was a fake-reflection
					// artifact and would leave every fixed-yaw twin facing the wrong way).
					const float TwinYaw = FMath::Fmod(Yaw + 180.f, 360.f);
					const FTransform TwinXf(FRotator(0.f, TwinYaw, 0.f), FVector(-X, -Y, TwinZ), FVector(Scale));
					Comp->AddInstance(TwinXf, /*bWorldSpace=*/true);
					SpacingGrid.Add(TwinPoint, FootprintR); // invariant (2): later primaries must see the twin
					++Placed;

					// Invariant (1): twin proxy in LOCKSTEP with the twin visual.
					if (bUsesProxy)
					{
						if (UHierarchicalInstancedStaticMeshComponent* Proxy = ResolveProxyForVisual(Comp, Layer))
						{
							const FVector ProxyScaleVec = Layer.CollisionProxyScale * Scale;
							const float ProxyZ = TwinZ + Layer.CollisionProxyZOffset * Scale;
							const FTransform ProxyXf(FRotator(0.f, TwinYaw, 0.f), FVector(-X, -Y, ProxyZ), ProxyScaleVec);
							Proxy->AddInstance(ProxyXf, /*bWorldSpace=*/true);
						}
					}
				}
				else
				{
					// ASYMMETRY ESCAPE — counted here and reported on the layer line
					// (CONVENTIONS §1: every local symmetry break must be logged).
					// Provably 0 under the shipped level; a non-zero count means the
					// keep-clear set or the hill field stopped being rotationally
					// symmetric and the layer ships one instance short of its target.
					++TwinSkipped;
				}
			}
		}
	}

	// TASK-358: `sym`/`pairs`/`twinSkipped`/`zMismatch` are additive tokens — the
	// pre-existing `placed`/`target`/`blocking`/`meshVariants` keys are untouched
	// so any existing QA grep still matches. Under the law `pairs` is the outer
	// iteration budget and `placed` should land at ~2 × the pairs actually filled.
	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter] Layer '%s': placed %d instances (target %d, sym=%s, pairs %d, twinSkipped=%d, zMismatch=%d, blocking=%s, meshVariants=%d)."),
		*Layer.LayerName.ToString(), Placed, Layer.InstanceCount,
		SymmetryModeToken(ScatterConfig->SymmetryMode), OuterTarget, TwinSkipped, TwinZMismatch,
		Layer.bBlocking ? TEXT("true") : TEXT("false"), Resolved.Num());

	if (TwinSkipped > 0)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter] Layer '%s' SymmetryEscape: %d rotational twin(s) SKIPPED (keep-clear or slope rejected the twin point) — the field is locally asymmetric and the layer shipped %d short of its %d target."),
			*Layer.LayerName.ToString(), TwinSkipped, FMath::Max(Layer.InstanceCount - Placed, 0), Layer.InstanceCount);
	}
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
		// TASK-349 (team gating, consequential): combatant capsules are re-typed
		// from ECC_Pawn to ECC_SiegeTeamBlue/Red at BeginPlay — this ignore-all
		// body must Block those channels too, or re-typed units/hero would walk
		// (and fall) THROUGH rocks and hills the shipped game has them climb.
		Comp->SetCollisionResponseToChannel(ECC_SiegeTeamBlue, ECR_Block);
		Comp->SetCollisionResponseToChannel(ECC_SiegeTeamRed, ECR_Block);
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

	// Cull bands + shadow casting (CONVENTIONS "Arena 10× scale-up & LOD/perf
	// (M7.6)", TASK-284) — the per-layer LOD/perf knobs that make the Phase-3
	// density fill affordable, set with the rest of the render profile before
	// RegisterComponent. SetCullDistances(Start, End): End==0 = NEVER culled, and
	// the DA defaults both to 0, so an unpopulated layer renders exactly as before
	// (Phase 3 populates the real bands). SetCastShadow drives the VISIBLE
	// instances' shadow casting from the layer flag (hills ON for the silhouette,
	// grass/plants OFF once the DA sets it) — its default true matches today's
	// implicit component default, so behavior is unchanged until Phase 3.
	Comp->SetCullDistances(FMath::Max(Layer.CullStartDistance, 0), FMath::Max(Layer.CullEndDistance, 0));
	Comp->SetCastShadow(Layer.bCastShadows);

	// LWC render-precision fix (TASK-292c): keep every scatter HISM OUT of the
	// distance-field and Lumen dynamic-GI scenes. The scatter is decorative
	// environment — at ≈15,000 instances on the 10× field, generating per-mesh
	// distance fields is both a needless cost AND the residual singular-matrix
	// source: a scattered instance's DF/Lumen-card transform can go non-invertible
	// → InverseFast NaN → the OriginX<=OriginMax DoubleFloat ensure at PIE
	// first-frame (seed-dependent, fires right after GenerateScatter — TASK-292b).
	// This BLANKET off mirrors the TASK-292/292b vista treatment (the same two
	// flags cleared on the 26 dressing components) onto the scatter. Set BEFORE
	// RegisterComponent — exactly like the cull/shadow calls above — so the flags
	// are live when the component is inserted into the DF/Lumen scene.
	Comp->bAffectDistanceFieldLighting = false;
	Comp->bAffectDynamicIndirectLighting = false;

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
	Proxy->SetCastShadow(false); // proxy-contract INVARIANT — an invisible trunk cylinder must never cast a shadow; NOT driven by Layer.bCastShadows (that drives the VISIBLE tree HISM in ResolveComponentForMesh)
	// Cull bands (CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)", TASK-284):
	// apply the layer's cull band to the proxy too for a uniform pair state. This is
	// functionally moot — the proxy is SetVisibility(false), so it never renders or
	// culls as geometry, and nav/collision are unaffected by cull distance — but it
	// keeps the visual/proxy pair consistent if a proxy is ever un-hidden for debug.
	// SetCastShadow stays false above (the proxy contract), so ONLY the cull band is
	// mirrored from the layer here, never the shadow flag.
	Proxy->SetCullDistances(FMath::Max(Layer.CullStartDistance, 0), FMath::Max(Layer.CullEndDistance, 0));
	Proxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Proxy->SetCollisionObjectType(ECC_WorldStatic);
	Proxy->SetCollisionResponseToAllChannels(ECR_Ignore);
	Proxy->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	// TASK-349 (team gating, consequential): re-typed combatant capsules
	// (ECC_SiegeTeamBlue/Red) must still be stopped by the invisible trunk
	// cylinder — mirror of the real-geometry blocker's addition above.
	Proxy->SetCollisionResponseToChannel(ECC_SiegeTeamBlue, ECR_Block);
	Proxy->SetCollisionResponseToChannel(ECC_SiegeTeamRed, ECR_Block);
	Proxy->bFillCollisionUnderneathForNavmesh = true;
	Proxy->SetCanEverAffectNavigation(true);

	// LWC render-precision fix (TASK-292c): keep the invisible collision proxy out
	// of the distance-field / Lumen-GI scene too. The proxy never renders
	// (SetVisibility(false) + SetCastShadow(false) above), but it is still a
	// UPrimitiveComponent that WOULD be inserted into the DF/Lumen scene and
	// generate a mesh distance field — another candidate singular-matrix / needless
	// cost source at 10× scale. Same flag pair as the visual HISM, set BEFORE
	// RegisterComponent (below), matching the pre-Register cull calls above.
	Proxy->bAffectDistanceFieldLighting = false;
	Proxy->bAffectDynamicIndirectLighting = false;

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

	// NO gold-node zones (W1-PREP mines redesign, TASK-255): the old live
	// AGoldNode sweep AND its hardcoded ±24,200 fallback discs are DELETED —
	// this actor now SPAWNS the mines itself (PlaceMines, after this rebuild
	// runs), so a live sweep would find nothing at generate time and the
	// fallback discs would keep-clear two PHANTOM points no mine occupies (the
	// phantom-disc trap). Mines are NoCollision and clear their own aprons.

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
	return IsInKeepClearDiscs(Point2D, InstanceRadius);
}

bool ASiegeBattlefieldScatter::IsInKeepClearDiscs(const FVector2D& Point2D, float InstanceRadius) const
{
	// Disc test WITHOUT the corridor band (TASK-255 split): the mines pass calls
	// this directly because corridor mines are ALLOWED by ruling; the layer
	// passes keep the corridor via IsInKeepClear above (behavior unchanged).
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
	// Thin wrapper since TASK-255: FindHillSurfaceAt is the SAME TASK-250 trace +
	// slope gate, extended to also surface the hit component + instance index (the
	// mines pass reads the component as its on-a-hill predicate; TASK-361's
	// ancient-ground pass will read it to REJECT hills outright). The layer passes
	// only need the Z, so the identity outs are discarded here — layer placement
	// behavior is unchanged.
	UHierarchicalInstancedStaticMeshComponent* SurfaceComp = nullptr;
	int32 InstanceIndex = INDEX_NONE;
	return FindHillSurfaceAt(X, Y, FloorZ, MaxSlopeDeg, OutZ, SurfaceComp, InstanceIndex);
}

bool ASiegeBattlefieldScatter::FindHillSurfaceAt(float X, float Y, float FloorZ, float MaxSlopeDeg,
	float& OutZ, UHierarchicalInstancedStaticMeshComponent*& OutSurfaceComp, int32& OutInstanceIndex) const
{
	OutZ = FloorZ;
	OutSurfaceComp = nullptr;
	OutInstanceIndex = INDEX_NONE;

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
	// contracts stay untouched. ZERO FRandomStream draws anywhere in here — the
	// TASK-255 determinism law for the hill/trace paths.
	const FVector TraceStart(X, Y, 50000.f);
	const FVector TraceEnd(X, Y, -50000.f);
	FCollisionQueryParams Params(TEXT("BattlefieldScatterHillTrace"), /*bTraceComplex=*/false);

	bool bOnHill = false;
	FVector BestNormal = FVector::UpVector;
	float BestZ = FloorZ;
	UHierarchicalInstancedStaticMeshComponent* BestComp = nullptr;
	int32 BestItem = INDEX_NONE;
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
			BestComp = SurfaceComp;
			BestItem = Hit.Item; // per-instance body index — for an (H)ISM this IS the instance index (GetInstanceTransform-compatible). TASK-358: informational since the parity clone was deleted; the COMPONENT is the live output.
		}
	}

	if (!bOnHill)
	{
		return true; // not over a hill — the flat floor Z stands (identity outs stay null/INDEX_NONE)
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
	OutSurfaceComp = BestComp;
	OutInstanceIndex = BestItem;
	return true;
}

void ASiegeBattlefieldScatter::PlaceMines(int32 Seed)
{
	// ClearScatter already destroyed the previous match's pair actors; this reset
	// only drops stale entries so the pass always starts from an empty ledger.
	SpawnedMines.Reset();

	UWorld* World = GetWorld();
	if (!World || !ScatterConfig)
	{
		return;
	}

	const int32 CountPerSide = ScatterConfig->MineCountPerSide;
	if (CountPerSide <= 0)
	{
		UE_LOG(LogSiegeTerrain, Log,
			TEXT("[BattlefieldScatter '%s'] MinesPass skipped (MineCountPerSide=%d)."),
			*GetNameSafe(this), CountPerSide);
		return;
	}

	// DEDICATED mine stream (seed-order law, CONVENTIONS "Mirrored depleting
	// mines"): XOR tag 0x4D494E45 = ASCII "MINE". The layer stream is a separate
	// FRandomStream that this pass never touches, so every existing scatter seed
	// keeps its exact layer layout, and the mines are reproducible in isolation
	// from the same match seed. ALL mine draws come from THIS stream, in the
	// fixed order documented at the draw site below — the determinism contract
	// the TASK-258 same-seed⇒identical-log PIE test rides on.
	FRandomStream MineStream(Seed ^ 0x4D494E45);

	const float HalfX = FMath::Max(ScatterConfig->ArenaHalfExtent.X, 1.f);
	const float HalfY = FMath::Max(ScatterConfig->ArenaHalfExtent.Y, 1.f);
	const float ClearR = FMath::Max(ScatterConfig->MineClearanceRadius, 0.f);
	const float Spacing = FMath::Max(ScatterConfig->MineMinSpacing, 0.f);
	const float EdgeMargin = FMath::Max(ScatterConfig->MineEdgeMargin, 0.f);
	const float SlopeGateDeg = FMath::Clamp(ScatterConfig->MineMaxSlopeDeg, 0.f, 89.f);
	const int32 Reserve = FMath::Max(ScatterConfig->MineGoldReserve, 0);

	// Half-draw band. The |X| floor max(ClearR, Spacing/2) buys TWO spacing
	// guarantees for free, which is why the explicit test below only needs the
	// prior PRIMARIES. TASK-358 (twin now (−X, −Y) instead of (−X, Y)) only
	// STRENGTHENS both — every distance below gains a non-negative Y term:
	//  - own twin: dist(P, P′) = 2·|P| = 2·√(X² + Y²) ≥ 2|X| ≥
	//    2·max(ClearR, Spacing/2) ≥ Spacing, and the pair's two clearance discs
	//    can never overlap across the center (2|P| ≥ 2·ClearR);
	//  - cross-pair vs another pair's ROTATED twin: both primaries sit on the SAME
	//    (Blue) half, so the negated X's ADD — dist(Pi, Pj′) =
	//    √((Xi + Xj)² + (Yi + Yj)²) ≥ |Xi| + |Xj| ≥ 2·(Spacing/2) = Spacing.
	// (The dispatch's "|X| ≥ max(600, spacing/2)": the 600 is MineClearanceRadius
	// — the floor that keeps a pair's own discs from overlapping the centerline —
	// not MineEdgeMargin, which only insets the OUTER band below.)
	const float MinAbsX = FMath::Max(ClearR, Spacing * 0.5f);
	const float MaxAbsX = HalfX - EdgeMargin;
	const float MaxAbsY = HalfY - EdgeMargin;
	const bool bDrawBandValid = (MaxAbsX > MinAbsX) && (MaxAbsY > 0.f);
	if (!bDrawBandValid)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] MinesPass draw band degenerate (|X| in [%.0f, %.0f], |Y| <= %.0f) — every mine takes its deterministic fallback slot."),
			*GetNameSafe(this), MinAbsX, MaxAbsX, MaxAbsY);
	}

	// MineClass null ⇒ AGoldNode (CONVENTIONS default; TSubclassOf guarantees an
	// AGoldNode-compatible class either way, so InitMine below is always valid).
	UClass* ResolvedMineClass = ScatterConfig->MineClass ? ScatterConfig->MineClass.Get() : AGoldNode::StaticClass();

	// Accepted primary centers (fallback slots included) — the spacing law's memory.
	TArray<FVector2D> PrimaryPoints;
	PrimaryPoints.Reserve(CountPerSide);

	// Mines carry TWO placement points + slope gates per candidate — double the
	// standard rejection budget before the deterministic fallback (spec: ≤ 2×
	// MaxPlacementAttemptsPerInstance). Kept at 2× even though TASK-358 removed
	// the parity-injection reject: the budget is a tunable, not a derived number,
	// and shrinking it would change the fallback rate for no gain.
	const int32 MaxMineAttempts = 2 * FMath::Max(MaxPlacementAttemptsPerInstance, 1);

	// Resolves a candidate PAIR (P on the Blue half, P′ its 180° ROTATION about the
	// map center): floor + hill grounding at both points and the slope gate.
	// Returns false with the FIELD STATE UNTOUCHED when either face is over-slope.
	// ZERO stream draws inside — pure trace/geometry, so a variable number of
	// internal rejections can never desync the draw sequence.
	// TASK-358: the hill-parity clone / re-trace / ROLLBACK machinery this lambda
	// used to carry is DELETED — see the unreachability argument below.
	auto TryResolveMinePair = [&](const FVector2D& Pt, float GateDeg,
		float& OutZP, float& OutZM, bool& bOutOnHill) -> bool
	{
		const FVector2D Pm(-Pt.X, -Pt.Y);
		const float FloorZP = GroundZAt(Pt.X, Pt.Y);
		const float FloorZM = GroundZAt(Pm.X, Pm.Y);

		UHierarchicalInstancedStaticMeshComponent* CompP = nullptr;
		UHierarchicalInstancedStaticMeshComponent* CompM = nullptr;
		int32 ItemP = INDEX_NONE;
		int32 ItemM = INDEX_NONE;
		if (!FindHillSurfaceAt(Pt.X, Pt.Y, FloorZP, GateDeg, OutZP, CompP, ItemP) ||
			!FindHillSurfaceAt(Pm.X, Pm.Y, FloorZM, GateDeg, OutZM, CompM, ItemM))
		{
			return false; // over-slope hill face at P or P′ — reject the candidate (never ground at floor inside a mound)
		}

		const bool bHillP = (CompP != nullptr);
		const bool bHillM = (CompM != nullptr);
		bOutOnHill = bHillP || bHillM;

		// ═══ WHY THE HILL-PARITY CLONE / RE-TRACE / ROLLBACK IS DELETED ═══════════
		// (TASK-358 — CONVENTIONS §1: "deleting it is part of the law, not an
		// optional cleanup". The removed path was: clone the supporting hill
		// instance onto the bare side, re-trace the bare point on the clone,
		// roll the clone back and REJECT the candidate if the point missed it,
		// else clearance-delete the clone's footprint.)
		//
		// THE ARGUMENT — it is now PROVABLY unreachable:
		//  1. P′ = (−P.X, −P.Y) is the exact 180° rotation of P about the map
		//     center, and the HILL FIELD IS ITSELF ROTATIONALLY SYMMETRIC:
		//     ScatterLayer emits every hill instance's (−X, −Y, yaw+180) twin on
		//     the SAME HISM, inline, during pass 1 — i.e. before this pass runs.
		//  2. A 180° yaw rotation about the world Z axis through the origin is a
		//     PROPER RIGID MOTION that maps each hill instance H exactly onto its
		//     twin H′. So the down-trace at P′ strikes H′ at precisely the rotated
		//     image of the point the trace at P strikes on H.
		//  3. That rotation leaves Z INVARIANT, and it leaves the surface normal's
		//     Z COMPONENT invariant — and N.Z is the ONLY quantity the slope gate
		//     reads (acos(N.Z)). So FindHillSurfaceAt returns the SAME hit/miss
		//     verdict and the SAME Z at both points: bHillP == bHillM and
		//     OutZP == OutZM, ALWAYS. "Either-side-has ⇒ both-have" is satisfied
		//     BY CONSTRUCTION; there is never a bare side to clone onto.
		//  4. The deleted path existed ONLY as a workaround for the retired X-MIRROR,
		//     which was a FAKE reflection: it mirrored the mesh's LOCAL Y, so an
		//     edge-of-hill primary could land its twin OFF the cloned hill's
		//     footprint — the old code said exactly that at its rollback site. A
		//     true rotation cannot miss, so the rollback has nothing to roll back.
		//
		// The parity injection was also the ONLY producer of the old OutInjectedSide
		// / OutFootprintCulls out-params, so those are deleted with it (and the
		// MinesPass log's `inj=` token with them).
		//
		// DEFENSIVE RESIDUAL: if the invariant is somehow violated (a level edit
		// that de-symmetrizes the keep-clear set could make ScatterLayer skip a hill
		// twin — it counts and logs that as a SymmetryEscape), the pair is still
		// SAFE and we do NOT reject: both ends already carry their own honestly
		// traced Z (FindHillSurfaceAt yields the floor Z when a point is not over a
		// hill), so nothing floats and nothing is buried — the pair just ships with
		// one end on a mound. Loud, not fatal.
		if (bHillP != bHillM)
		{
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] SymmetryAssert: mine hill parity broken at P=(%.0f,%.0f) hill=%s / P'=(%.0f,%.0f) hill=%s — the rotated hill field is NOT symmetric here (unreachable under the 180° law; pair still ships, each end on its own traced surface)."),
				*GetNameSafe(this), Pt.X, Pt.Y, bHillP ? TEXT("yes") : TEXT("no"),
				Pm.X, Pm.Y, bHillM ? TEXT("yes") : TEXT("no"));
		}
		return true;
	};

	FString PairsLog;

	for (int32 MineIndex = 0; MineIndex < CountPerSide; ++MineIndex)
	{
		FVector2D P = FVector2D::ZeroVector;
		float ZP = 0.f;
		float ZM = 0.f;
		bool bOnHill = false;
		bool bAccepted = false;

		for (int32 Attempt = 0; bDrawBandValid && Attempt < MaxMineAttempts && !bAccepted; ++Attempt)
		{
			// THE ONLY STREAM DRAWS IN THE MINES PASS — exactly two per attempt, in
			// fixed X-then-Y order (the auditable draw sequence: total draws =
			// 2 × attempts-consumed, and everything after this pair — spacing,
			// keep-clear, traces, rotation, clearance — is draw-free by design, so
			// no rejection path can ever desync the sequence). TASK-358 changed
			// WHERE the twin lands, never HOW MANY draws are taken: the rotation is
			// pure arithmetic on the drawn point.
			const float X = -MineStream.FRandRange(MinAbsX, MaxAbsX); // draw 1: |X|, negated → Blue half (X < 0)
			const float Y = MineStream.FRandRange(-MaxAbsY, MaxAbsY); // draw 2: Y across the inset field width
			const FVector2D Candidate(X, Y);

			// Spacing vs prior PRIMARIES only — the twin + cross-pair distances
			// hold by the MinAbsX construction (see the band comment above).
			bool bTooClose = false;
			for (const FVector2D& Prior : PrimaryPoints)
			{
				if (FVector2D::DistSquared(Prior, Candidate) < Spacing * Spacing)
				{
					bTooClose = true;
					break;
				}
			}
			if (bTooClose)
			{
				continue;
			}

			// Keep-clear DISCS at BOTH P and P′ = (−X, −Y), inflated by the clearance
			// radius — the zone SET is NOT symmetric (the PlayerStart sits on the
			// Blue side only, and castles are live-swept), so testing the rotated
			// twin is NOT redundant. NO corridor test BY RULING: corridor mines are
			// ALLOWED (high-risk gold) — AGoldNode is NoCollision/no-nav, so a lane
			// mine cannot break the traversability guarantee.
			if (IsInKeepClearDiscs(Candidate, ClearR) || IsInKeepClearDiscs(FVector2D(-X, -Y), ClearR))
			{
				continue;
			}

			// Grounding + slope gate (≤ MineMaxSlopeDeg at BOTH points — miners
			// must walk onto both ends of the pair). Hill parity is now automatic
			// under the 180° law (TASK-358) — no injection, no rollback.
			if (!TryResolveMinePair(Candidate, SlopeGateDeg, ZP, ZM, bOnHill))
			{
				continue;
			}

			P = Candidate;
			bAccepted = true;
		}

		bool bFallback = false;
		if (!bAccepted)
		{
			// DETERMINISTIC FALLBACK SLOT (Jonathan ruling: the economy NEVER ships
			// short — 6 mines exist every match, whatever the field looks like).
			// Slot i: X = −clamp(HalfX/2 into the draw band), Y fanned around the
			// centerline in max(Spacing, 2·ClearR) steps ⇒ defaults (−13,000,
			// {−3,000, 0, +3,000}) — mid-half, clear of the castle (±25,000,
			// r 1,500) and PlayerStart (−23,800, r 800) discs by construction,
			// corridor-legal by ruling. ZERO draws — same coordinates every time
			// this mine index falls back (the determinism contract). NOT re-tested
			// against spacing/keep-clear (there is nothing left to try); the Error
			// line flags the layout for QA/PIE scrutiny.
			bFallback = true;
			const float FallbackAbsX = FMath::Clamp(HalfX * 0.5f, MinAbsX, FMath::Max(MaxAbsX, MinAbsX));
			const float FallbackMaxY = (MaxAbsY > 0.f) ? MaxAbsY : FMath::Max(HalfY - ClearR, 0.f);
			const float FallbackStep = FMath::Max(Spacing, 2.f * ClearR);
			const float FallbackY = FMath::Clamp((MineIndex - (CountPerSide - 1) * 0.5f) * FallbackStep, -FallbackMaxY, FallbackMaxY);
			P = FVector2D(-FallbackAbsX, FallbackY);

			UE_LOG(LogSiegeTerrain, Error,
				TEXT("[BattlefieldScatter '%s'] Mine %d found no valid draw in %d attempts — deterministic fallback slot (%.0f, %.0f) used (the economy never ships short)."),
				*GetNameSafe(this), MineIndex, MaxMineAttempts, P.X, P.Y);

			// The slope gate opens to 90° at the fallback: the slot MUST seat, so it
			// takes whatever surface stands there (a steep face ⇒ the mine sits on
			// the slope — cosmetic, never buried, because FindHillSurfaceAt still
			// returns the TOP surface Z). With a 90° gate the resolve can no longer
			// fail on slope, so this branch is itself defensive-only; it grounds
			// both ends at the flat floor.
			if (!TryResolveMinePair(P, 90.f, ZP, ZM, bOnHill))
			{
				ZP = GroundZAt(P.X, P.Y);
				ZM = GroundZAt(-P.X, -P.Y); // TASK-358: the twin is the 180° ROTATION, so Y negates too
				bOnHill = false;
			}
		}

		// Clearance-delete at BOTH points (r = MineClearanceRadius): the apron +
		// miner walk-in ring is guaranteed blocker-free at each end of the pair.
		// Hills inside the disc survive (exempt — hills are never deleted); grass
		// is untouched by the nav guard. TASK-358: because the two discs are
		// centered at P and its exact rotation −P, and the scatter field is
		// rotationally symmetric, this cull deletes ROTATIONAL PAIRS — it is
		// symmetry-PRESERVING and is NOT one of the logged asymmetry escapes.
		const int32 CullsP = RemoveBlockingInstancesInDisc(P, ClearR);
		const int32 CullsM = RemoveBlockingInstancesInDisc(FVector2D(-P.X, -P.Y), ClearR);

		// Spawn the tracked pair + InitMine (the TASK-253 API — safe before or
		// after BeginPlay). Primary yaw 0 / twin yaw 180 — ALREADY the 180° law's
		// yaw (this is why the mine pass is the precedent the rotational law
		// generalizes, and why TASK-358 changed no yaw here). NO yaw draw (the
		// specced draw sequence is X,Y only; a cosmetic yaw roll would silently
		// shift every later draw). AlwaysSpawn because the mine is NoCollision by
		// law — collision adjustment must never bend the rotated math (a nudged
		// twin breaks the equal-castle-distance fairness proof).
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;

		AGoldNode* Primary = World->SpawnActor<AGoldNode>(ResolvedMineClass, FVector(P.X, P.Y, ZP), FRotator::ZeroRotator, SpawnParams);
		if (Primary)
		{
			Primary->InitMine(Reserve);
			SpawnedMines.Add(Primary);
		}
		// TASK-358: THE twin transform — (−P.X, −P.Y, ZM) with yaw 180. The Y
		// negation here is the one that actually moves the shipped mine actor.
		AGoldNode* Twin = World->SpawnActor<AGoldNode>(ResolvedMineClass, FVector(-P.X, -P.Y, ZM), FRotator(0.f, 180.f, 0.f), SpawnParams);
		if (Twin)
		{
			Twin->InitMine(Reserve);
			SpawnedMines.Add(Twin);
		}
		if (!Primary || !Twin)
		{
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] Mine %d SpawnActor failed (P=%s M=%s) — pair incomplete (mine class unresolvable/aborted spawn; never a crash)."),
				*GetNameSafe(this), MineIndex, Primary ? TEXT("ok") : TEXT("FAIL"), Twin ? TEXT("ok") : TEXT("FAIL"));
		}

		PrimaryPoints.Add(P);

		// ⚠️ LOG-FORMAT CHANGE (TASK-358, called out for QA): the `inj=` token is
		// GONE — the hill-parity injection it reported no longer exists (see the
		// unreachability argument in TryResolveMinePair), so `culls=` is now
		// exactly CullsP + CullsM. `M=` is still the twin, now the 180° rotation.
		// Every other token and the whole `[i] P=… M=… hill=… fb=… culls=…` shape
		// is unchanged, so the TASK-258 same-seed⇒identical-line criterion holds.
		PairsLog += FString::Printf(TEXT(" [%d] P=(%.0f,%.0f,%.0f) M=(%.0f,%.0f,%.0f) hill=%s fb=%s culls=%d"),
			MineIndex, P.X, P.Y, ZP, -P.X, -P.Y, ZM,
			bOnHill ? TEXT("yes") : TEXT("no"),
			bFallback ? TEXT("yes") : TEXT("no"),
			CullsP + CullsM);
	}

	// The one grep-able MinesPass reproducibility line (CONVENTIONS "Logging" +
	// the TASK-258 determinism criterion: same seed ⇒ this line is IDENTICAL).
	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter '%s'] MinesPass seed=%d mineStream=%d pairsPlanned=%d minesSpawned=%d reserve=%d:%s"),
		*GetNameSafe(this), Seed, Seed ^ 0x4D494E45, CountPerSide, SpawnedMines.Num(), Reserve, *PairsLog);
}

void ASiegeBattlefieldScatter::PlaceAncientGrounds(int32 Seed, bool bAuthoritativeGenerate)
{
	// ClearScatter already destroyed the previous match's pair actors; this reset
	// only drops stale entries so the pass always starts from an empty ledger
	// (the PlaceMines idiom, deliberately identical).
	SpawnedAncientGrounds.Reset();

	UWorld* World = GetWorld();
	if (!World || !ScatterConfig)
	{
		return;
	}

	// DEDICATED ancient-ground stream (seed-order law, CONVENTIONS §2): XOR tag
	// 0x41474E44 = ASCII "AGND" (the mines use 0x4D494E45 = "MINE"). The layer
	// stream and the mine stream are separate FRandomStreams that this pass NEVER
	// touches, so shipping this whole feature moves ZERO existing draws: every
	// seed keeps its exact layer layout AND its exact mine layout, and the ground
	// pair is reproducible in isolation from the same match seed. ALL ground draws
	// come from THIS stream, in the fixed order documented at the draw site below.
	FRandomStream GroundStream(Seed ^ 0x41474E44);

	const float ArenaHalfX = FMath::Max(ScatterConfig->ArenaHalfExtent.X, 1.f);
	const float ArenaHalfY = FMath::Max(ScatterConfig->ArenaHalfExtent.Y, 1.f);
	// Explicit LWC double → float (FVector2D is double-precision in UE5; the
	// placement grid's precision is ample in float, and the file already uses this
	// cast idiom at the hill-trace Z).
	const float HalfExtX = FMath::Max(static_cast<float>(ScatterConfig->AncientGroundHalfExtent.X), 0.f);
	const float HalfExtY = FMath::Max(static_cast<float>(ScatterConfig->AncientGroundHalfExtent.Y), 0.f);
	const float MineClear = FMath::Max(ScatterConfig->AncientGroundMineClear, 0.f);
	const float ClearR = FMath::Max(ScatterConfig->AncientGroundClearRadius, 0.f);

	// The half-draw band (CONVENTIONS §2 — these numbers ARE the law):
	//  - |X| in [4000, 16080] — 2,320 uu clear of the mid capture zone at the near
	//    end, 1,540 uu in FRONT of the spawn-box edge at the far end. ⚖️ THE CEILING
	//    WAS RE-DERIVED 21,000 → 16,080 at the 9× castle (WR-§2b row E / ruling
	//    W2-R1, TASK-576): the spawn-box edge moved 22,540 → 17,620 when
	//    SpawnBoxHalfExtent went (2460,2460) → (7380,7380), and 16,080 re-solves the
	//    SAME relationship — 17,620 − 1,540 centre margin, footprint edge 16,920,
	//    i.e. the identical 700 uu edge clearance. ⛔ Measure against the LIVE
	//    SpawnBoxHalfExtent if you ever retune it; the old doc transcribed 22,540 as
	//    an absolute and that is exactly what rotted (SC-§34);
	//  - |Y| <= 10,800 — ArenaHalfExtent.Y minus a 1,200 margin, so the 840-half
	//    footprint edge lands at 11,640, inside the ±12,500 arena ground.
	// The extra Min() against (arena half-extent − footprint half-extent) is a
	// pure GUARD for a mis-tuned DataAsset — it is a NO-OP at the shipped defaults
	// (26,000−840 = 25,160 > 16,080 and 12,000−840 = 11,160 > 10,800), so it
	// cannot silently alter the specced band.
	// ⚠️ NO CORRIDOR TEST and NO keep-clear DISC test, both deliberate: the ruling
	// for the corridor is the mines' (AAncientGround has no collision primitive
	// and no nav geometry, so it cannot break the traversability guarantee, and a
	// lane objective is good contested design), and the |X| ceiling above is the
	// castle/spawn-box exclusion in closed form — the band was derived FROM those
	// geometries, so re-testing them would tighten the law rather than enforce it.
	// ⚠️ THE DISC TEST STAYS OMITTED AT 9× AND IT IS STILL A PROVABLE NO-OP, on the
	// re-derived numbers: with CastleKeepClearRadius 4,500 the castle disc bites at
	// |X| ≥ 20,500, which is 3,580 uu OUTSIDE the 16,920 footprint edge. (The old
	// rationale quoted |X| ≥ 23,500 off the pre-CASTLE-3X radius; that figure is
	// retired — the CONCLUSION survives, the number did not.)
	const float MinAbsX = FMath::Max(ScatterConfig->AncientGroundMinAbsX, 0.f);
	const float MaxAbsX = FMath::Min(FMath::Max(ScatterConfig->AncientGroundMaxAbsX, 0.f), FMath::Max(ArenaHalfX - HalfExtX, 0.f));
	const float MaxAbsY = FMath::Min(FMath::Max(ScatterConfig->AncientGroundMaxAbsY, 0.f), FMath::Max(ArenaHalfY - HalfExtY, 0.f));
	const bool bDrawBandValid = (MaxAbsX > MinAbsX) && (MaxAbsY > 0.f);
	if (!bDrawBandValid)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] AncientGroundsPass draw band degenerate (|X| in [%.0f, %.0f], |Y| <= %.0f) — the pair takes its deterministic fallback slot."),
			*GetNameSafe(this), MinAbsX, MaxAbsX, MaxAbsY);
	}

	// Draw-free helper: the surface Z under (X,Y), and whether that surface is
	// FLAT. The 90° gate means FindHillSurfaceAt never rejects on slope, so a
	// non-null OutSurfaceComp is EXACTLY the predicate "this point is over a hill
	// face" — which is what FLAT GROUND ONLY needs (CONVENTIONS §2: a 1,680²
	// gathering box needs flat ground; the hill is REJECTED, never parity-cloned
	// for it — deliberately keeping the machinery TASK-358 deleted from coming
	// back). OutZ is honest on EVERY path: FindHillSurfaceAt seeds it with FloorZ
	// and only raises it to a hill top, so the fallback below can reuse this to
	// seat on whatever stands there. ZERO stream draws — as with the mines, a
	// variable number of internal rejections can never desync the draw sequence.
	auto ResolveSurfaceZ = [&](float X, float Y, float& OutZ) -> bool
	{
		const float FloorZ = GroundZAt(X, Y);
		UHierarchicalInstancedStaticMeshComponent* SurfaceComp = nullptr;
		int32 InstanceIndex = INDEX_NONE;
		const bool bResolved = FindHillSurfaceAt(X, Y, FloorZ, 90.f, OutZ, SurfaceComp, InstanceIndex);
		return bResolved && (SurfaceComp == nullptr);
	};

	// Draw-free: keep AncientGroundMineClear from every mine spawned by the pass
	// that ran immediately before this one. Tested at BOTH ends of the pair — the
	// mine field is itself rotationally symmetric, so the two tests AGREE in the
	// healthy case, but a mine whose SpawnActor failed would leave the set
	// asymmetric, and an honest test at both points costs nothing.
	auto ClearsEveryMine = [&](const FVector2D& Q) -> bool
	{
		for (const TObjectPtr<AGoldNode>& Mine : SpawnedMines)
		{
			AGoldNode* MinePtr = Mine.Get();
			if (!IsValid(MinePtr))
			{
				continue;
			}
			const FVector L = MinePtr->GetActorLocation();
			if (FVector2D::DistSquared(FVector2D(L.X, L.Y), Q) < MineClear * MineClear)
			{
				return false;
			}
		}
		return true;
	};

	// 48 attempts (CONVENTIONS §2 — stated as a LITERAL there, so it is a literal
	// here rather than derived; it happens to equal the mines' 2 ×
	// MaxPlacementAttemptsPerInstance at the shipped 24, and a ground carries the
	// same two-point burden a mine pair does).
	const int32 MaxGroundAttempts = 48;

	FVector2D P = FVector2D::ZeroVector;
	float ZP = 0.f;
	float ZM = 0.f;
	bool bAccepted = false;
	// FREE ASSERTION (the TASK-358 idiom): under the 180° law the flat/hill verdict
	// at P and at P′ must AGREE — the hill field is rotationally symmetric and a
	// Z-axis rotation leaves both Z and the normal's Z component invariant. Counted
	// rather than logged per attempt (48 attempts would spam), reported once below.
	// PROVABLY 0 on a healthy field, so any non-zero value is a real signal.
	int32 FlatParityBreaks = 0;

	for (int32 Attempt = 0; bDrawBandValid && Attempt < MaxGroundAttempts && !bAccepted; ++Attempt)
	{
		// ⚠️ THE ONLY STREAM DRAWS IN THE ANCIENT-GROUNDS PASS — exactly two per
		// attempt, in fixed X-then-Y order (the auditable sequence: total draws =
		// 2 × attempts-consumed). EVERYTHING after this pair — mine clearance, the
		// hill rejection, the ground traces, the rotation, the clearance culls, the
		// fallback and the spawns — is DRAW-FREE by design, so no rejection path
		// can desync the sequence and host/client agree bit-for-bit off the
		// replicated seed. There is deliberately NO yaw draw: the primary is yaw 0
		// and the twin is a fixed +180 (the §1 law), so a cosmetic yaw roll would
		// buy nothing and silently shift every later draw.
		const float X = -GroundStream.FRandRange(MinAbsX, MaxAbsX); // draw 1: |X|, negated → BLUE half (X < 0)
		const float Y = GroundStream.FRandRange(-MaxAbsY, MaxAbsY); // draw 2: Y across the inset field width
		const FVector2D Candidate(X, Y);
		// THE TWIN: the exact 180° rotation about the map center (CONVENTIONS §1).
		// BLUE half, never Red — the PlayerStart exists ONLY at (−23800, 0, 100),
		// so generating Red would rotate this objective onto the hero spawn.
		const FVector2D TwinPoint(-X, -Y);

		if (!ClearsEveryMine(Candidate) || !ClearsEveryMine(TwinPoint))
		{
			continue;
		}

		float CandZP = 0.f;
		float CandZM = 0.f;
		const bool bFlatP = ResolveSurfaceZ(Candidate.X, Candidate.Y, CandZP);
		const bool bFlatM = ResolveSurfaceZ(TwinPoint.X, TwinPoint.Y, CandZM);
		if (bFlatP != bFlatM)
		{
			++FlatParityBreaks;
		}
		if (!bFlatP || !bFlatM)
		{
			continue; // over a hill at either end — REJECT OUTRIGHT (flat ground only)
		}

		P = Candidate;
		ZP = CandZP;
		ZM = CandZM;
		bAccepted = true;
	}

	bool bFallback = false;
	if (!bAccepted)
	{
		// DETERMINISTIC FALLBACK SLOT (the mines' "never ships short" discipline,
		// applied to the objective: a match with no ancient ground has no Sorcerer
		// mechanic at all, which is a worse failure than a ground on an awkward
		// patch). CONVENTIONS §2 fixes the coordinates at (−12000, +6000) — a
		// LITERAL, deliberately NOT clamped into the configured band: the one
		// property the fallback must have is that it is the same point every time,
		// and folding config into it would make the "deterministic" slot vary with
		// a designer's tuning. ZERO draws. It is NOT re-tested against mine
		// clearance or slope (there is nothing left to try) — the Error line is
		// what flags the layout for QA/PIE scrutiny.
		bFallback = true;
		P = FVector2D(-12000.f, 6000.f);

		UE_LOG(LogSiegeTerrain, Error,
			TEXT("[BattlefieldScatter '%s'] AncientGrounds found no valid draw in %d attempts — deterministic fallback slot (%.0f, %.0f) used (the objective never ships short)."),
			*GetNameSafe(this), MaxGroundAttempts, P.X, P.Y);

		// Seat both ends on whatever surface stands there: ResolveSurfaceZ's 90°
		// gate can no longer reject, so the returned Z is the TOP surface (hill
		// crest or floor) and the pair is never buried. The flat/hill verdict is
		// discarded here BY DESIGN — the slot must seat.
		// (NO RegroundAncientGrounds counterpart to RegroundMines is needed, on the
		// fallback path or any other: AAncientGround::IsPointInZone is a 2D XY box
		// that IGNORES Z, and the decal projects DecalProjectionDepth both ways, so
		// even if a later defensive cull deleted a hill out from under a fallback
		// ground, neither the mechanic nor the visual would change. Z here is
		// cosmetic-only — deliberately unlike a mine, which miners must walk onto.)
		ResolveSurfaceZ(P.X, P.Y, ZP);
		ResolveSurfaceZ(-P.X, -P.Y, ZM);
	}

	// FREE ASSERTION (CONVENTIONS §1: the twin Z is RE-TRACED, never copied, and
	// under an exact rotation it MUST come back equal). Mismatch ⇒ each end still
	// ships at its own honestly traced Z — nothing floats, nothing is buried — and
	// the break is loud rather than fatal.
	if (FMath::Abs(ZP - ZM) > 1.f)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] SymmetryAssert: ancient-ground twin Z mismatch — P=(%.0f,%.0f,%.1f) vs P'=(%.0f,%.0f,%.1f) (unreachable under the 180° law; both ends ship at their own traced Z)."),
			*GetNameSafe(this), P.X, P.Y, ZP, -P.X, -P.Y, ZM);
	}
	if (FlatParityBreaks > 0)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] SymmetryAssert: ancient-ground hill parity broke on %d of %d candidate(s) — the rotated hill field is NOT symmetric there (unreachable under the 180° law; those candidates were simply rejected, so placement stayed correct)."),
			*GetNameSafe(this), FlatParityBreaks, MaxGroundAttempts);
	}

	// Clearance-delete at BOTH ends (r = AncientGroundClearRadius): units must be
	// able to GATHER on the runes, not fight a thicket for standing room. Hills
	// inside the disc survive (exempt — hills are never deleted), which costs
	// nothing here because a hill candidate was rejected outright; grass is
	// untouched by the nav guard. Because the two discs are centered at P and its
	// EXACT rotation −P over a rotationally symmetric field, this cull deletes
	// rotational PAIRS — it is symmetry-PRESERVING and is NOT one of the logged
	// asymmetry escapes (the same standing as the mines' aprons).
	const int32 CullsP = RemoveBlockingInstancesInDisc(P, ClearR);
	const int32 CullsM = RemoveBlockingInstancesInDisc(FVector2D(-P.X, -P.Y), ClearR);

	// Spawn the tracked pair. Primary yaw 0 / twin yaw 180 — the §1 law's yaw,
	// fixed, never rolled. AlwaysSpawn because AAncientGround has NO collision
	// primitive at all (SceneRoot + UDecalComponent), so collision adjustment must
	// never be allowed to nudge the rotated math: a displaced twin would break the
	// "one per side, symmetric" fairness the objective exists to provide.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AAncientGround* Primary = World->SpawnActor<AAncientGround>(AAncientGround::StaticClass(), FVector(P.X, P.Y, ZP), FRotator::ZeroRotator, SpawnParams);
	if (Primary)
	{
		// ⚠️⚠️ THE AUTHORITY PUSH — THE SINGLE MOST DANGEROUS LINE IN THIS FEATURE
		// (CONVENTIONS §2). The ground MUST NOT read HasAuthority() for itself: it
		// is spawned locally on the CLIENT from the replicated seed and so keeps
		// ROLE_Authority there, which would make a self-read return TRUE and run a
		// ROGUE CLIENT-SIDE BOOST SIM diverging from the server's. We push THIS
		// machine's decision — the very flag RunScatterPasses is running under.
		// The ground logs `AncientGroundInit authoritativeBoost=…` on receipt: on a
		// CLIENT both grounds MUST print false. A client printing true is a
		// threading regression HERE, not in the actor.
		Primary->InitAncientGround(bAuthoritativeGenerate);
		SpawnedAncientGrounds.Add(Primary);
	}
	AAncientGround* Twin = World->SpawnActor<AAncientGround>(AAncientGround::StaticClass(), FVector(-P.X, -P.Y, ZM), FRotator(0.f, 180.f, 0.f), SpawnParams);
	if (Twin)
	{
		// The SAME push on the twin — both grounds or neither. Missing it here
		// would fail CLOSED (that ground silently never boosts) and would hand one
		// team a boost the other cannot get, which is exactly the asymmetry the
		// rotational law exists to prevent.
		Twin->InitAncientGround(bAuthoritativeGenerate);
		SpawnedAncientGrounds.Add(Twin);
	}
	if (!Primary || !Twin)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] AncientGround SpawnActor failed (P=%s M=%s) — pair incomplete (aborted spawn; never a crash, but ONE side would hold an objective the other cannot)."),
			*GetNameSafe(this), Primary ? TEXT("ok") : TEXT("FAIL"), Twin ? TEXT("ok") : TEXT("FAIL"));
	}

	// PAIRED-TUNABLE DIVERGENCE GUARD: AncientGroundHalfExtent (this config, used
	// for the placement margin) and AAncientGround::ZoneHalfExtent (the actor's
	// mechanic box + decal size) are the same number stored twice, which is the
	// price of the ACaptureZone 2-mirror. If they ever drift, the pass would inset
	// the band for one footprint while the ground boosts over another — silently.
	// Cheap, draw-free, once per generate.
	if (Primary)
	{
		const FVector2D ActorHalfExtent = Primary->GetZoneHalfExtent();
		const float ActorHalfX = static_cast<float>(ActorHalfExtent.X);
		const float ActorHalfY = static_cast<float>(ActorHalfExtent.Y);
		if (!FMath::IsNearlyEqual(ActorHalfX, HalfExtX, 1.f) || !FMath::IsNearlyEqual(ActorHalfY, HalfExtY, 1.f))
		{
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] AncientGround half-extent DIVERGENCE: config AncientGroundHalfExtent=(%.0f, %.0f) vs AAncientGround::ZoneHalfExtent=(%.0f, %.0f) — these are a PAIRED TUNABLE (CONVENTIONS §2); the placement margin and the boost box now disagree."),
				*GetNameSafe(this), HalfExtX, HalfExtY, ActorHalfX, ActorHalfY);
		}
	}

	// The one grep-able AncientGroundsPass reproducibility line (CONVENTIONS §2 —
	// the token set is fixed there: seed / P / M / fb / culls). Same binary + same
	// seed ⇒ this line is IDENTICAL, and identical on host and client, which is
	// the determinism criterion QA reads. (The stream seed is Seed ^ 0x41474E44.)
	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter '%s'] AncientGroundsPass seed=%d P=(%.0f,%.0f,%.0f) M=(%.0f,%.0f,%.0f) fb=%s culls=%d"),
		*GetNameSafe(this), Seed, P.X, P.Y, ZP, -P.X, -P.Y, ZM,
		bFallback ? TEXT("yes") : TEXT("no"),
		CullsP + CullsM);
}

void ASiegeBattlefieldScatter::RegroundMines()
{
	// After EVERY defensive cull (TASK-255 law): a widening cull can delete a
	// mine's supporting hill (CullCorridorBlockers does NOT exempt hill comps —
	// pre-existing corridor semantics kept), which would leave the mine floating
	// at its old hill-surface Z. Re-seat each mine on whatever stands there NOW:
	// hill else floor, ANY slope (90° gate — an already-placed mine must never
	// strand mid-air because a face reads steep). Draw-free.
	for (const TObjectPtr<AGoldNode>& Mine : SpawnedMines)
	{
		AGoldNode* MinePtr = Mine.Get();
		if (!IsValid(MinePtr))
		{
			continue;
		}
		const FVector L = MinePtr->GetActorLocation();
		const float MineX = static_cast<float>(L.X);
		const float MineY = static_cast<float>(L.Y);
		const float FloorZ = GroundZAt(MineX, MineY);
		float NewZ = FloorZ;
		UHierarchicalInstancedStaticMeshComponent* SurfaceComp = nullptr;
		int32 InstanceIndex = INDEX_NONE;
		FindHillSurfaceAt(MineX, MineY, FloorZ, 90.f, NewZ, SurfaceComp, InstanceIndex);
		if (!FMath::IsNearlyEqual(NewZ, static_cast<float>(L.Z), 0.5f))
		{
			MinePtr->SetActorLocation(FVector(L.X, L.Y, NewZ));
		}
	}
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

	// ── STAGE 0 TELEMETRY (TASK-535/529, `NAV-§9`) — THE MOMENT OF THE CLAIM ──
	// The snapshot that sits beside the CONFIRMED/PROVISIONAL line and decomposes it:
	// `remaining=`/`running=` are the tile-task queue this verdict is labelled by, and
	// `dirtyAreas=`/`hasDirty=` are the OTHER half of IsNavigationBeingBuilt — together
	// they say exactly which of the two made the settle poll fall through. Read-only.
	FSiegeNavDiagnostics::LogNavBuildSnapshot(World, TEXT("at-confirmation"));

	// Which caller is speaking. The event-driven pass is the DEFINITIVE one
	// (`NAV-§4`): it runs off the engine's own generation-finished signal rather than
	// off a queue-empty lull, so its verdict is the one that discharges the guarantee.
	const TCHAR* const VerdictSource = bInDefinitiveCheck
		? TEXT(" [definitive: OnNavigationGenerationFinished]")
		: TEXT("");

	// Inset the endpoints toward the centerline onto open pad ground — the raw
	// castle center can sit inside the castle's own nav-carved hole (a false
	// negative). FindPathToLocationSynchronously still projects each endpoint to
	// the navmesh within its default query extent.
	// ⚖️ TASK-576 (WR-§2b row D): the inset is now DERIVED PER CASTLE from that
	// castle's live colliding bounds — the flat CastleQueryInset literal had been
	// stale since CASTLE-3X (endpoint ≈19 uu inside the footprint) and at the 9×
	// castle put both endpoints 2,457 uu inside, i.e. inside the nav-carved hole:
	// the precise false negative this inset exists to avoid. Sign convention is
	// unchanged (both teams move toward the centerline); only the MAGNITUDE moved.
	FVector BlueLoc = ResolveCastleLocation(ETeamId::Blue);
	FVector RedLoc = ResolveCastleLocation(ETeamId::Red);
	const float BlueInset = ResolveCastleQueryInset(ETeamId::Blue, BlueLoc);
	const float RedInset = ResolveCastleQueryInset(ETeamId::Red, RedLoc);
	BlueLoc.X -= FMath::Sign(BlueLoc.X) * BlueInset; // Blue (X<0) moves toward 0
	RedLoc.X -= FMath::Sign(RedLoc.X) * RedInset;    // Red  (X>0) moves toward 0

	// One grep-able line per actor lifetime (not per re-check — this function runs
	// from the poll AND from the definitive nav callback). It is the PIE instrument
	// for TASK-569 row (p): the two insets must clear the castle half-depth, and the
	// two endpoints must sit between the wall face and the mid-field.
	if (!bLoggedCastleInsetDerivation)
	{
		bLoggedCastleInsetDerivation = true;
		UE_LOG(LogSiegeTerrain, Log,
			TEXT("[BattlefieldScatter '%s'] CastleQueryInset derived: Blue %.0f -> endpoint X %.0f, Red %.0f -> endpoint X %.0f (authored floor %.0f + face pad %.0f)."),
			*GetNameSafe(this), BlueInset, BlueLoc.X, RedInset, RedLoc.X, CastleQueryInset, CastleQueryFacePad);
	}

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

	// ⛔ IS THE ANSWER WE ARE ABOUT TO GET EVEN TRUE YET? (TASK-535, `NAV-§4`.)
	// GetNumRemainingBuildTasks() is the generator's live queue depth
	// (RunningDirtyTiles + PendingDirtyTiles + the time-sliced generator —
	// RecastNavMeshGenerator.h:794). Zero means the navmesh has nothing left to
	// carve, so a path query against it answers about the FINAL geometry. Non-zero
	// means tiles are still holding an older bake and the query answers about a
	// navmesh that no longer matches the field — which is exactly the state in which
	// this function used to print "CONFIRMED" and, worse, DELETE INSTANCES.
	// ⚠️ The dirty-area queue is folded in on purpose: dirty areas that have not yet
	// become tile tasks read `remaining=0` while carving is still owed, and treating
	// that as settled would re-open the same hole one level up. This can only ever
	// make CONFIRMED HARDER to print, never easier.
	const int32 RemainingTileTasks = FMath::Max(NavSys->GetNumRemainingBuildTasks(), 0);
	const bool bNavStillBuilding = UNavigationSystemV1::IsNavigationBeingBuilt(World);
	const bool bNavSettled = (RemainingTileTasks == 0) && !bNavStillBuilding;

	// Which half of the settle test failed — so a "PROVISIONAL (0 tile task(s)
	// pending)" line can never read as a contradiction. The adjacent `at-confirmation`
	// snapshot carries the raw `dirtyAreas=`/`hasDirty=` numbers behind this word.
	const TCHAR* const UnsettledReason = (RemainingTileTasks > 0)
		? TEXT("tile tasks still queued")
		: TEXT("dirty areas queued but not yet submitted as tile tasks");

	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, BlueLoc, RedLoc);
	const bool bCastleReachable = Path && Path->IsValid() && !Path->IsPartial();

	// TASK-255: the ECONOMY guarantee rides the same confirmation — every spawned
	// mine must be path-reachable from the Blue anchor (a walled-in mine = economy
	// failure). Checked only once the castle lane is confirmed: on an unpathable
	// field the corridor cull below is the prerequisite repair, and every mine
	// query would false-fail against the same break anyway. Blue anchor only:
	// the pairs are exact 180° ROTATIONS of each other (TASK-358 — and the whole
	// scatter field now is too), so Blue's distances equal Red's by construction —
	// what this catches is an ASYMMETRIC blocker wall (one of the logged escapes,
	// or a level edit), and the per-mine disc cull below repairs it on whichever
	// side it stands.
	TArray<AGoldNode*> UnreachableMines;
	if (bCastleReachable)
	{
		for (const TObjectPtr<AGoldNode>& Mine : SpawnedMines)
		{
			AGoldNode* MinePtr = Mine.Get();
			if (!IsValid(MinePtr))
			{
				continue;
			}
			UNavigationPath* MinePath = UNavigationSystemV1::FindPathToLocationSynchronously(World, BlueLoc, MinePtr->GetActorLocation());
			if (!(MinePath && MinePath->IsValid() && !MinePath->IsPartial()))
			{
				UnreachableMines.Add(MinePtr);
			}
		}
	}

	if (bCastleReachable && UnreachableMines.Num() == 0)
	{
		// ⛔ HONEST LABELLING (`NAV-§4`). "CONFIRMED" is a CLAIM about a navmesh, and a
		// query run against a still-building one cannot make it. The word is now spent
		// ONLY on a settled query; a pre-settle pass says PROVISIONAL and says why.
		if (bNavSettled)
		{
			UE_LOG(LogSiegeTerrain, Log,
				TEXT("[BattlefieldScatter '%s'] Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + %d mine path(s) exist (after %d cull(s))%s."),
				*GetNameSafe(this), SpawnedMines.Num(), ReachabilityAttempt, VerdictSource);

			// The event-driven pass is the one that DISCHARGES the guarantee (`NAV-§4`):
			// it ran off the engine's own generation-finished signal, so there is
			// nothing left to wait for. A settle-poll CONFIRMED deliberately does NOT
			// discharge it — a queue that reads empty at +5 s can be a lull between
			// waves of dirty areas, and the whole point of the event bind is to get one
			// verdict against a navmesh the ENGINE calls finished.
			if (bInDefinitiveCheck)
			{
				bDefinitiveCheckDone = true;
				UnbindNavGenerationFinished();
			}
			return;
		}

		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] Traversability PROVISIONAL (%d tile task(s) pending — PRE-SETTLE query; %s) — a Blue→Red castle path + %d mine path(s) were found, but the navmesh is NOT settled, so this is NOT the CONFIRMED guarantee; the definitive re-check runs once on OnNavigationGenerationFinished%s."),
			*GetNameSafe(this), RemainingTileTasks, UnsettledReason, SpawnedMines.Num(), VerdictSource);
		return;
	}

	// ⭐⛔ THE SETTLED-ONLY CULL GATE (TASK-535, `NAV-§4`) — THE DETERMINISM FIX.
	// Below this point the code DELETES INSTANCES. On a partially-built navmesh the
	// failure that would trigger that deletion is an artefact of WALL-CLOCK TIMING —
	// tiles still holding the pre-scatter bake — so culling on it would make the
	// shipped instance set depend on how fast the machine baked, not on the seed. That
	// is a live violation of the determinism law (TASKBOARD.md:11044: "the same seed
	// must produce byte-identical placement"), and it is the reason this task exists.
	// ⭐ THE CULL IS NOT REMOVED — IT IS MOVED to the only moment at which it is both
	// TRUE and DETERMINISTIC: post-settle, the reachability answer is stable, so a cull
	// driven by it is a pure function of the geometry, hence of the seed. The
	// NON-NEGOTIABLE traversability guarantee survives intact (layer (1), the reserved
	// corridor, never depended on nav state at all, and the definitive post-settle
	// check still performs every repair this path ever performed).
	// 🚩 ACCEPTED CONSEQUENCE, FLAGGED FOR JONATHAN (TASK-539), ⛔ NOT TO BE "FIXED":
	// the repair now lands LATER and can be VISIBLE (~27 s at 8× tile concurrency,
	// ~216 s if the concurrency flip rolls back) where it used to happen invisibly at
	// +5 s. It only ever fires when the field is genuinely walled off — a visible pop
	// beats an unwinnable match.
	if (!bNavSettled && !bCullOnProvisionalFailure)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] Traversability PROVISIONAL (%d tile task(s) pending — PRE-SETTLE query; %s) — castle lane %s, %d mine(s) unreachable; defensive cull SUPPRESSED (bCullOnProvisionalFailure=false, the determinism law): a cull decided against a partially-built navmesh is decided by wall-clock timing. Deferring the repair to the definitive post-settle check (OnNavigationGenerationFinished)%s."),
			*GetNameSafe(this), RemainingTileTasks, UnsettledReason,
			bCastleReachable ? TEXT("REACHABLE") : TEXT("NOT reachable"),
			UnreachableMines.Num(), VerdictSource);

		// ⛔ Nothing is culled, ReachabilityAttempt is NOT consumed (it counts CULLS,
		// and no cull happened), and NO re-poll is armed — re-polling would spin the
		// timer against a navmesh whose queue we already know is not empty. The event
		// bind is what brings us back, exactly once, when it really is.
		return;
	}

	// Defensive re-roll — SHARED attempts machinery (one counter, one widen step,
	// one re-poll loop) for both failure kinds, castle lane first. ⚠️ Reached ONLY on
	// a SETTLED query (or with the bCullOnProvisionalFailure escape hatch explicitly
	// flipped), so every cull below is reproducible from the seed alone.
	++ReachabilityAttempt;
	if (!bCastleReachable)
	{
		// The reserved corridor should make this impossible, but if a config edit
		// shrank the corridor/radii, cull blocking instances in a widening Y band
		// around the lane, then re-check after the nav settles again.
		const float Band = CorridorHalfWidthCached + ReachabilityAttempt * CorridorWidenStep;
		// (TASK-358: CullCorridorBlockers logs its own SymmetryEscape line when it
		// actually removes something — this line stays the reachability narrative.)
		const int32 Removed = CullCorridorBlockers(Band);
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] Blue→Red path NOT found (attempt %d) — culled %d blocking instance(s) within |Y|<=%.0f; re-checking after nav settles."),
			*GetNameSafe(this), ReachabilityAttempt, Removed, Band);
	}
	else
	{
		// Mine-approach repair (TASK-255): widen a clearance-cull disc around each
		// unreachable mine — same widen step as the corridor path, base radius =
		// the mine clearance law. RemoveBlockingInstancesInDisc keeps hills exempt
		// even here (hills are never deleted; a hill-ringed mine resolves by the
		// discs eating the non-hill blockers plugging the gaps — and the ≤30°
		// placement gate means the mine's own hill is always climbable).
		const float BaseClearR = ScatterConfig ? FMath::Max(ScatterConfig->MineClearanceRadius, 0.f) : 600.f;
		const float CullRadius = BaseClearR + ReachabilityAttempt * CorridorWidenStep;
		int32 Removed = 0;
		for (AGoldNode* MinePtr : UnreachableMines)
		{
			const FVector L = MinePtr->GetActorLocation();
			Removed += RemoveBlockingInstancesInDisc(FVector2D(L.X, L.Y), CullRadius);
		}
		// TASK-358 asymmetry escape (CONVENTIONS §1). UNLIKE the mines pass — which
		// culls at P AND its exact rotation −P and is therefore symmetry-preserving —
		// this repair culls around ONE unreachable mine only. It stays side-agnostic
		// BY LAW (mirroring the cull would delete more geometry for zero
		// traversability gain), so it locally breaks the 180° symmetry and must say so.
		if (Removed > 0)
		{
			UE_LOG(LogSiegeTerrain, Warning,
				TEXT("[BattlefieldScatter '%s'] SymmetryEscape: mine-approach repair culled %d instance(s) around %d unreachable mine(s) at ONE point each (r<=%.0f) — the rotated twin discs are deliberately NOT culled."),
				*GetNameSafe(this), Removed, UnreachableMines.Num(), CullRadius);
		}
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] %d mine(s) NOT path-reachable from the Blue anchor (attempt %d) — culled %d blocking instance(s) within r<=%.0f of each; re-checking after nav settles."),
			*GetNameSafe(this), UnreachableMines.Num(), ReachabilityAttempt, Removed, CullRadius);
	}

	// TASK-255 law: RegroundMines after EVERY defensive cull — the corridor cull
	// can delete a mine's supporting hill (it does not exempt hill comps), and a
	// mine must never float at a dead hill's Z.
	RegroundMines();

	if (ReachabilityAttempt < MaxReachabilityAttempts)
	{
		// Wait for the post-cull nav re-carve to settle (poll to idle again), then re-check.
		// TASK-535: the event bind stays live alongside this poll on purpose — the poll
		// gives up at MaxNavSettleWait and would then hand back a PROVISIONAL (culls
		// suppressed), so the generation-finished signal is what finishes the repair.
		StartNavSettlePoll();
	}
	else if (!bCastleReachable)
	{
		// Final guarantee: the corridor band has been cleared of blockers, so the
		// straight Y≈0 lane is now obstacle-free even if the async nav has not yet
		// reported a path. Never leave a match unwinnable.
		UE_LOG(LogSiegeTerrain, Error,
			TEXT("[BattlefieldScatter '%s'] Reachability unconfirmed after %d culls; corridor force-cleared to |Y|<=%.0f as the final traversability guarantee (straight lane is obstacle-free)%s."),
			*GetNameSafe(this), ReachabilityAttempt, CorridorHalfWidthCached + ReachabilityAttempt * CorridorWidenStep, VerdictSource);

		// TASK-535: TERMINAL. The attempts budget is spent and the force-clear stance
		// is final, so a later generation-finished callback could only re-log the same
		// verdict — release the bind rather than leave a live callback with no work.
		bDefinitiveCheckDone = true;
		UnbindNavGenerationFinished();
	}
	else
	{
		// Final economy stance: every unreachable mine's approach disc has been
		// force-cleared of non-hill blockers at the widest radius — best-effort;
		// the match is still winnable (the castle lane itself passed this pass's path
		// query) even if a pathological layout leaves a mine contested-by-terrain.
		// TASK-535: the castle lane's standing is reported with the SAME honesty rule
		// as the main verdict — the word CONFIRMED is never spent on a pre-settle query.
		UE_LOG(LogSiegeTerrain, Error,
			TEXT("[BattlefieldScatter '%s'] Mine reachability unconfirmed after %d culls; unreachable-mine approach discs force-cleared (best-effort economy guarantee — castle lane itself is %s)%s."),
			*GetNameSafe(this), ReachabilityAttempt,
			bNavSettled ? TEXT("CONFIRMED (nav settled: 0 pending)") : TEXT("PROVISIONAL (PRE-SETTLE query)"),
			VerdictSource);

		// TASK-535: TERMINAL — same reasoning as the castle-lane branch above.
		bDefinitiveCheckDone = true;
		UnbindNavGenerationFinished();
	}
}

void ASiegeBattlefieldScatter::BindNavGenerationFinished()
{
	// Idempotent: Play Again re-arms through RunScatterPasses, and a second bind would
	// mean two definitive checks racing one latch.
	if (bNavGenerationFinishedBound)
	{
		return;
	}

	// AUTHORITY ONLY (M8 doc D9, the same rule the validation itself follows): the
	// client never path-queries and never culls, so it has nothing to be woken for.
	if (!HasAuthority())
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* const NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		// No nav system to listen to. NOT a failure: layer (1), the reserved corridor,
		// is a geometric guarantee that never needed nav at all — and the settle-poll
		// path already degrades to its fixed fallback wait. One Verbose line, because
		// this is a legitimate configuration, not a fault.
		UE_LOG(LogSiegeTerrain, Verbose,
			TEXT("[BattlefieldScatter '%s'] No navigation system at arm time — the definitive post-settle traversability check is not bound (the reserved corridor remains the deterministic guarantee)."),
			*GetNameSafe(this));
		return;
	}

	// AddUniqueDynamic, not AddDynamic: belt-and-braces against a double bind if this
	// ever gets called from a path the latch above does not cover.
	NavSys->OnNavigationGenerationFinishedDelegate.AddUniqueDynamic(this, &ASiegeBattlefieldScatter::OnNavGenerationFinished);
	BoundNavSystem = NavSys;
	bNavGenerationFinishedBound = true;
}

void ASiegeBattlefieldScatter::UnbindNavGenerationFinished()
{
	// Weak: if the nav system has already been torn down (EndPlay during world
	// teardown is the normal case) there is nothing to unbind from and nothing to
	// dereference — the delegate died with its owner.
	if (UNavigationSystemV1* const NavSys = BoundNavSystem.Get())
	{
		NavSys->OnNavigationGenerationFinishedDelegate.RemoveDynamic(this, &ASiegeBattlefieldScatter::OnNavGenerationFinished);
	}

	BoundNavSystem.Reset();
	bNavGenerationFinishedBound = false;
}

void ASiegeBattlefieldScatter::OnNavGenerationFinished(ANavigationData* NavData)
{
	// Already discharged, or a check is already queued for this drain: the engine
	// broadcasts once PER ANavigationData, and a match can drain more than once, so
	// this pair of latches is what keeps an EVENT from degenerating into a POLL.
	if (bDefinitiveCheckDone || bDefinitiveCheckPending)
	{
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	bDefinitiveCheckPending = true;

	UE_LOG(LogSiegeTerrain, Log,
		TEXT("[BattlefieldScatter '%s'] Nav generation FINISHED ('%s') — running the DEFINITIVE post-settle traversability check (one deferred re-check; NAV-§4)."),
		*GetNameSafe(this), *GetNameSafe(NavData));

	// ⚠️ DEFERRED BY ONE TIMER TICK, DELIBERATELY. This callback runs from INSIDE the
	// Recast generator's own tick (RecastNavMeshGenerator.cpp:7631 →
	// ARecastNavMesh::OnNavMeshGenerationFinished → NavigationSystem.cpp:4915). The
	// check it schedules can DELETE HISM INSTANCES, which immediately dirties nav
	// areas — re-entering the nav system from inside its generator tick. One tick of
	// latency removes that entire class of problem, and it is nothing against a
	// confirmation that was previously wrong by ~211 s.
	// ⛔ NOT A POLL: armed once per latch, never re-arms itself.
	World->GetTimerManager().ClearTimer(DefinitiveCheckTimerHandle);
	World->GetTimerManager().SetTimer(DefinitiveCheckTimerHandle, this,
		&ASiegeBattlefieldScatter::RunDefinitiveTraversabilityCheck, 0.001f, false);
}

void ASiegeBattlefieldScatter::RunDefinitiveTraversabilityCheck()
{
	bDefinitiveCheckPending = false;

	// A verdict landed between the broadcast and this tick (the settle poll can get
	// there first): nothing to do.
	if (bDefinitiveCheckDone)
	{
		return;
	}

	// The ONLY difference from any other pass: the label. Every rule — the settled
	// gate, the attempts budget, RegroundMines — is the same code, so there is no
	// second implementation of the guarantee to keep in sync.
	// ⛔ Non-re-entrant: ValidateTraversability never calls back into this, and the
	// pending latch above is already cleared, so a broadcast fired from inside the
	// check itself would simply queue the next one.
	bInDefinitiveCheck = true;
	ValidateTraversability();
	bInDefinitiveCheck = false;
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

	// TASK-358 asymmetry-escape log (CONVENTIONS §1: the destructive culls stay
	// SIDE-AGNOSTIC by law — mirroring a cull would delete more geometry for zero
	// traversability gain — but each MUST log when it fires). This one is called
	// ONLY from ValidateTraversability's failure path, so any firing is already
	// exceptional; shipped runs report 0 culls across every recorded PIE
	// (TASK-287/291/295). Note the band |Y| <= B is itself a rotation-symmetric
	// REGION, so in practice the rotational pairs fall together and symmetry
	// survives — logged anyway, because "in practice" is not a guarantee.
	if (TotalRemoved > 0)
	{
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] SymmetryEscape: CullCorridorBlockers removed %d nav-relevant instance(s) within |Y|<=%.0f — a destructive cull is side-agnostic by law and can locally break the 180° symmetry."),
			*GetNameSafe(this), TotalRemoved, Band);
	}
	return TotalRemoved;
}

int32 ASiegeBattlefieldScatter::RemoveBlockingInstancesInDisc(const FVector2D& Center, float Radius)
{
	if (Radius <= 0.f)
	{
		return 0;
	}
	const float RadiusSq = Radius * Radius;

	int32 TotalRemoved = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Comp : ScatterComponents)
	{
		// Same nav-relevance guard as CullCorridorBlockers (this is its DISC
		// sibling): only nav-relevant comps matter — real-geometry blockers and
		// tree PROXY HISMs. Grass (no-nav decoration) is untouched by law, and a
		// proxy-layer's VISUAL HISM is skipped here and culled in LOCKSTEP via
		// its proxy below.
		if (!Comp || !Comp->CanEverAffectNavigation())
		{
			continue;
		}
		// HILLS ARE NEVER DELETED (Jonathan conflict rule — the one guard the
		// corridor cull does NOT have): hill-surface comps are nav-relevant
		// real-geometry blockers, but a mine sits ON a hill rather than deleting
		// it. TASK-358 adds a second reason: the hill field is the ROTATIONAL
		// REFERENCE FRAME every later pass traces against (the mine pair's two
		// grounding traces, and TASK-361's ancient-ground slope gate), so eating
		// hills asymmetrically would break the symmetry proof, not just the look.
		if (HillSurfaceComponents.Contains(Comp))
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
				const FVector Loc = InstanceXf.GetLocation();
				// Center-in-disc semantics, matching the corridor cull's
				// center-in-band (the clearance radius is sized to cover the apron).
				if (FVector2D::DistSquared(FVector2D(Loc.X, Loc.Y), Center) <= RadiusSq)
				{
					ToRemove.Add(Idx);
				}
			}
		}
		if (ToRemove.Num() > 0)
		{
			// Visual+proxy lockstep — the same law as the corridor cull: a culled
			// trunk proxy must take its visible tree with it, or the tree stands
			// unblocked (the visual/proxy desync defect). Indices are parallel by
			// construction; removing the same set from both preserves that.
			if (UHierarchicalInstancedStaticMeshComponent* Visual = FindVisualForProxy(Comp))
			{
				Visual->RemoveInstances(ToRemove);
			}
			Comp->RemoveInstances(ToRemove);
			TotalRemoved += ToRemove.Num();
		}
	}

	// TASK-358: this cull fires from TWO kinds of caller and only ONE of them is
	// an asymmetry escape, so the escape verdict is made by the CALLER, not here:
	//  - PlaceMines calls it at P and at its exact rotation −P, so the two calls
	//    delete ROTATIONAL PAIRS — symmetry-PRESERVING, and the expected every-match
	//    case (which is why this line is Log, not Warning);
	//  - ValidateTraversability calls it around ONE unreachable mine — a genuine
	//    escape, and that call site logs its own Warning-level SymmetryEscape line.
	if (TotalRemoved > 0)
	{
		UE_LOG(LogSiegeTerrain, Log,
			TEXT("[BattlefieldScatter '%s'] DiscCull removed %d nav-relevant instance(s) within r<=%.0f of (%.0f, %.0f) (hills exempt)."),
			*GetNameSafe(this), TotalRemoved, Radius, Center.X, Center.Y);
	}
	return TotalRemoved;
}

ACastle* ASiegeBattlefieldScatter::ResolveCastleActor(ETeamId Team) const
{
	// TASK-576: this loop is ResolveCastleLocation's ORIGINAL body, extracted verbatim
	// (same iteration, same IsValid + team test, same first-match semantics) so that
	// the endpoint's LOCATION and its BOUNDS are read from the same actor. Behaviour
	// of ResolveCastleLocation is byte-identical — it delegates and keeps its own
	// ±25000 fallback, which only ever applied when no castle was found.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACastle> It(World); It; ++It)
		{
			ACastle* Castle = *It;
			if (IsValid(Castle) && Castle->GetTeamId() == Team)
			{
				return Castle;
			}
		}
	}
	return nullptr;
}

FVector ASiegeBattlefieldScatter::ResolveCastleLocation(ETeamId Team) const
{
	if (const ACastle* Castle = ResolveCastleActor(Team))
	{
		return Castle->GetActorLocation();
	}
	// World axes, M7.6 10× scale-up (branch supersedes main's ±8000 law at merge): Blue -25000, Red +25000.
	return FVector((Team == ETeamId::Blue) ? -25000.f : 25000.f, 0.f, 0.f);
}

float ASiegeBattlefieldScatter::ResolveCastleQueryInset(ETeamId Team, const FVector& CastleLocation)
{
	// ⚖️ WR-§2b row D / SC-§34's structural escape: MEASURE THE CASTLE, do not
	// transcribe a number. The authored literals are the floor and the fallback.
	//
	// ⛔ NEVER ZERO (spec item 7): the fallback is the larger of the two authored
	// tunables, so a single mis-typed 0 in either field still cannot produce the
	// centre-of-castle endpoint this constant exists to prevent.
	const float AuthoredFloor = FMath::Max(CastleQueryInset, 0.f);
	const float AuthoredPad = FMath::Max(CastleQueryFacePad, 0.f);
	const float AuthoredFallback = FMath::Max(AuthoredFloor, AuthoredPad);

	auto WarnFallbackOnce = [this, Team, AuthoredFallback](const TCHAR* Reason)
	{
		if (bWarnedCastleInsetFallback)
		{
			return;
		}
		bWarnedCastleInsetFallback = true;
		UE_LOG(LogSiegeTerrain, Warning,
			TEXT("[BattlefieldScatter '%s'] CastleQueryInset could not be derived for %s (%s) — falling back to the authored inset %.0f (pre-TASK-576 behaviour). The path-query endpoint may land inside the castle footprint if the castle is larger than that."),
			*GetNameSafe(this), (Team == ETeamId::Blue) ? TEXT("Blue") : TEXT("Red"), Reason, AuthoredFallback);
	};

	const ACastle* Castle = ResolveCastleActor(Team);
	if (!IsValid(Castle))
	{
		// ResolveCastleLocation took its ±25000 fallback too, so there is no live
		// geometry to measure — today's behaviour is the only honest answer.
		WarnFallbackOnce(TEXT("no live ACastle for this team"));
		return AuthoredFallback;
	}

	// bOnlyCollidingComponents = true: what matters is what CARVES THE NAVMESH and
	// blocks a path, not the render/widget bounds (ACastle's HP-bar widget sits far
	// above the keep and must not inflate this) — the same choice, for the same
	// reason, as ASiegeGameMode::GetHeroStartTransform branch 3.
	FVector CastleBoundsOrigin = FVector::ZeroVector;
	FVector CastleBoxExtent = FVector::ZeroVector;
	Castle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, CastleBoundsOrigin, CastleBoxExtent);

	// The direction the endpoint travels: Blue (X<0) moves +X toward the centerline,
	// Red (X>0) moves −X. Matches the FMath::Sign() form at the call site exactly;
	// X == 0 cannot happen for a castle but resolves to +X harmlessly.
	const float TowardCenterline = (CastleLocation.X <= 0.0) ? 1.f : -1.f;

	// Distance from the CASTLE ACTOR'S PIVOT to the colliding wall face on the
	// centerline side. The (Origin − Location) term is not pedantry: GetActorBounds
	// returns the bounds' own centre, which need not sit on the actor pivot, and
	// ignoring it would silently under-inset a mesh whose pivot is off-centre.
	const float FaceDistance = static_cast<float>((CastleBoundsOrigin.X - CastleLocation.X) * TowardCenterline + CastleBoxExtent.X);
	if (!(FaceDistance > 1.f))
	{
		// Degenerate/unresolvable bounds (mesh not yet streamed in, extent ~0, or a
		// pivot offset that swallows the extent) — fall through to the authored value.
		WarnFallbackOnce(TEXT("degenerate colliding bounds"));
		return AuthoredFallback;
	}

	// Stay inside the castle keep-clear disc WHEN THE DISC IS WIDER THAN THE CASTLE:
	// inside it no scatter obstacle is ever placed, so the pad is guaranteed open
	// ground. When the disc is NARROWER than the footprint (a stale/mis-tuned
	// DataAsset — which is exactly the state until TASK-569 lands 4,500 in
	// DA_BattlefieldScatter), the cap is skipped on purpose: clearing the wall face
	// outranks sitting in the disc, because outside-the-disc merely RISKS a blocker
	// while inside-the-footprint is a CERTAIN false negative.
	const float KeepClearRadius = ScatterConfig ? FMath::Max(ScatterConfig->CastleKeepClearRadius, 0.f) : 0.f;
	const float RoomInsideDisc = KeepClearRadius - FaceDistance;
	const float Pad = (RoomInsideDisc > 0.f) ? FMath::Min(AuthoredPad, RoomInsideDisc) : AuthoredPad;

	// The authored literal is a FLOOR (WR-§2b's governing principle), then the disc
	// caps it back down where the disc is usable. Both operands are floats already;
	// FaceDistance/Pad were cast at their source (FVector components are DOUBLE in
	// UE5 and FMath::Max is a single-type template — CONVENTIONS compile trap).
	float Inset = FMath::Max(AuthoredFloor, FaceDistance + Pad);
	if (RoomInsideDisc > 0.f)
	{
		Inset = FMath::Min(Inset, KeepClearRadius);
	}

	// INVARIANT, and it holds for every branch above: Inset > FaceDistance whenever
	// the pad is non-zero (both Inset candidates exceed FaceDistance, and so does the
	// cap, since RoomInsideDisc > 0 ⇒ KeepClearRadius > FaceDistance) ⇒ the endpoint
	// is OUTSIDE the colliding footprint. That is the whole contract of this constant.
	return Inset;
}
