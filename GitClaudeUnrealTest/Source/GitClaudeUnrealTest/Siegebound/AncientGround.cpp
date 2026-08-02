// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/AncientGround.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/TeamId.h"
#include "TimerManager.h"

namespace
{
	/** Team buckets for the per-team sorcerer counters: exactly Blue + Red (ETeamId has no third value). */
	constexpr int32 NumTeamBuckets = 2;

	/** Bucket index for a team — [0] = Blue, [1] = Red. The ONE mapping both boost passes share. */
	FORCEINLINE int32 TeamBucketIndex(ETeamId Team)
	{
		return (Team == ETeamId::Red) ? 1 : 0;
	}
}

AAncientGround::AAncientGround()
{
	// No per-frame work: the boost evaluation runs on a repeating timer, NEVER
	// per-tick (TASK-004 never-per-tick law). There are also zero overlap events
	// in this module — the occupancy test is the same 2D box sweep ACaptureZone
	// uses, so no collision primitive is created at all.
	PrimaryActorTick.bCanEverTick = false;

	// ⚖️ NET RELEVANCY TIER C — NOT REPLICATED. bReplicates is deliberately left
	// at the AActor default (false) and is never touched: both machines build an
	// identical pair locally from the scatter's Tier-A replicated seed. See the
	// class doc for the full rationale + the authority-push law.

	// Scene root; the decal attaches beneath it and projects down.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Rune decal: relative pitch -90 => the decal faces -Z (projects straight
	// down onto the terrain). Deliberate 2-mirror of ACaptureZone's recipe (see
	// the class-doc debt note). DecalSize / FadeScreenSize / SortOrder are all
	// applied from the tunables in ApplyDecalFootprint (constructor seed +
	// OnConstruction + BeginPlay) so instance edits take.
	GroundDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("GroundDecal"));
	GroundDecal->SetupAttachment(SceneRoot);
	GroundDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	// Visual contract: soft path, never a hard reference (the material is
	// authored in parallel, TASK-374, and is null-safe if absent).
	GroundDecalMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_AncientGround.M_AncientGround")));

	ApplyDecalFootprint();
}

void AAncientGround::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep the editor decal footprint matched to ZoneHalfExtent while placed /
	// previewed. Silent: OnConstruction re-runs on every property tweak.
	ApplyDecalFootprint();
}

void AAncientGround::BeginPlay()
{
	Super::BeginPlay();

	ApplyDecalFootprint();

	// Soft-load the rune material. NULL-SAFE: a missing material means no
	// visual, THE BOOST MECHANIC STILL RUNS, and the miss is logged exactly
	// once. No MID is created — the ground is team-neutral, so nothing drives a
	// colour param at runtime (M_AncientGround's authored default is the look).
	// Log category mirrors ACaptureZone, this class's acknowledged 2-mirror
	// donor, so the two zone actors report material misses on one channel.
	if (GroundDecal)
	{
		if (UMaterialInterface* DecalMaterial = GroundDecalMaterialAsset.LoadSynchronous())
		{
			GroundDecal->SetDecalMaterial(DecalMaterial);
		}
		else if (!bWarnedMissingMaterial)
		{
			bWarnedMissingMaterial = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] AncientGround: rune material '%s' did not resolve — the boost mechanic runs without a visual (TASK-374 authors it)."),
				*GetNameSafe(this), *GroundDecalMaterialAsset.ToString());
		}
	}

	// ⚠️ THE TIMER IS ARMED UNCONDITIONALLY, AND THAT IS DELIBERATE. Arming it
	// from the authority flag would couple correctness to spawn ORDER: a plain
	// World->SpawnActor<AAncientGround>() runs BeginPlay INSIDE the spawn call,
	// i.e. BEFORE the scatter gets the pointer back to call InitAncientGround,
	// so the flag is still false here on the SERVER too. The law is that the
	// TICK BODY gates on the stored flag (see ApplyBoostTick) — that is
	// order-independent and fail-closed. The cost on a non-authoritative machine
	// is one predicted branch per second across exactly two actors.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BoostTickTimerHandle, this, &AAncientGround::ApplyBoostTick,
			BoostTickInterval, /*bLoop=*/ true, /*FirstDelay=*/ BoostTickInterval);
	}
}

void AAncientGround::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Timer hygiene: the boost tick never outlives the actor (a ClearScatter /
	// Play Again destroys this pair and re-places a fresh one).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BoostTickTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AAncientGround::InitAncientGround(bool bAuthoritative)
{
	// THE AUTHORITY PUSH. The scatter threads its OWN bAuthoritativeGenerate
	// here; this actor never derives the answer itself (class doc: it is
	// spawned locally on the client and keeps ROLE_Authority there, so deriving
	// it would run a rogue client-side boost sim instead of suppressing one).
	bAuthoritativeBoost = bAuthoritative;

	// Grep-able, two lines per match. This is the diagnostic for the most
	// dangerous spot in the feature: on a CLIENT both grounds MUST report
	// authoritativeBoost=false. If a client ever prints true, the scatter's
	// threading regressed — not this actor's gate.
	const FVector Location = GetActorLocation();
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] AncientGroundInit authoritativeBoost=%s P=(%.0f, %.0f) halfExtent=(%.0f, %.0f)"),
		*GetNameSafe(this), bAuthoritative ? TEXT("true") : TEXT("false"),
		Location.X, Location.Y, ZoneHalfExtent.X, ZoneHalfExtent.Y);
}

bool AAncientGround::IsPointInZone(const FVector& Point) const
{
	// 2D (XY) box about the actor origin; Z ignored (a region test) — byte-copy
	// of ACaptureZone::IsPointInZone so "standing in the zone" reads identically
	// for both zone actors.
	const FVector Center = GetActorLocation();
	return FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X
		&& FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y;
}

void AAncientGround::ApplyBoostTick()
{
	// ⚠️ THE ONLY AUTHORITY GATE IN THIS CLASS, and it reads the PUSHED flag —
	// never the engine's per-actor authority query, which is TRUE on the client
	// for this locally-spawned, non-replicating actor and would therefore turn
	// the intended guard into a rogue duplicate simulation. Mirrors
	// ASiegeBattlefieldScatter::RunScatterPasses(Seed, bAuthoritativeGenerate).
	if (!bAuthoritativeBoost)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// ONE sweep of the unit fleet, two team buckets. Buildings / towers /
	// castles / gold nodes and the hero are NOT ASummonedUnit, so they are
	// excluded by type — the boost is a unit-fleet mechanic.
	int32 SorcererCount[NumTeamBuckets] = { 0, 0 };
	TArray<ASummonedUnit*> Occupants;

	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		ASummonedUnit* Unit = *It;
		if (!IsValid(Unit) || Unit->IsUnitDead())
		{
			continue;
		}
		if (!IsPointInZone(Unit->GetActorLocation()))
		{
			continue;
		}

		if (Unit->IsAncientGroundEmpowerer())
		{
			// A sorcerer is a SOURCE, never a sink — it NEVER self-boosts and
			// never boosts a fellow sorcerer (class identity, not a CSV flag).
			++SorcererCount[TeamBucketIndex(Unit->GetTeamId())];
			continue;
		}

		if (!Unit->CanReceiveDamageBoost())
		{
			// Dead / never-attacks / zero-damage / Support-profile units carry no
			// boost (their damage routes through neither compose point, which
			// also keeps their boost bar row hidden).
			continue;
		}

		Occupants.Add(Unit);
	}

	// Common case: a ground with no sorcerer on it. Nothing to grant, and no
	// call means no spurious FOnCombatantDamageBoostChanged broadcast.
	if (SorcererCount[0] == 0 && SorcererCount[1] == 0)
	{
		return;
	}

	// FRIENDLY-ONLY (Jonathan ruling): every occupant is granted the count of
	// sorcerers on ITS OWN team, so a CONTESTED ground empowers both sides
	// simultaneously through their own sorcerers — it is a shared resource, not
	// a captured one. PER-SORCERER STACKING: 2 friendly sorcerers => 2 stacks
	// this tick. Authority for the mutation is by construction — this gated tick
	// is AddPermanentDamageStacks' sole caller.
	for (ASummonedUnit* Unit : Occupants)
	{
		const int32 Grant = SorcererCount[TeamBucketIndex(Unit->GetTeamId())];
		if (Grant > 0)
		{
			Unit->AddPermanentDamageStacks(Grant);
		}
	}
}

void AAncientGround::ApplyDecalFootprint()
{
	if (!GroundDecal)
	{
		return;
	}

	// After the -90 pitch: DecalSize.X = projection half-depth along -Z; .Y/.Z =
	// the square footprint half-extents in the ground plane. ⚠️ .Y/.Z are the
	// ZoneHalfExtent VERBATIM — the decal MATCHES the mechanic box exactly and
	// must never be shrunk for looks, or the player is shown a lie about where
	// the boost applies.
	GroundDecal->DecalSize = FVector(DecalProjectionDepth, ZoneHalfExtent.X, ZoneHalfExtent.Y);

	// See the header: the engine default 0.01 culls this decal at the arena's
	// zoomed-out framing.
	GroundDecal->SetFadeScreenSize(DecalFadeScreenSize);

	// ⚠️ SORT ORDER LIVES ON THE COMPONENT, NOT THE MATERIAL — confirmed by
	// TASK-374, which dumped UMaterial's full property list rather than assume:
	// UE 5.8 UMaterial exposes NO SortOrder/SortPriority field, so the artist
	// CANNOT author this and it can only be set here (these grounds are spawned
	// procedurally, so there is no level actor to hand-tune either).
	// Without it the ancient-ground and capture-zone decals Z-FIGHT wherever
	// they overlap near the centerline — precisely where a sorcerer is most
	// likely to be played. ACaptureZone leaves SortOrder at the default 0 (a
	// repo-wide grep for SortOrder returns no other hit), so any positive value
	// wins; 10 leaves headroom for a future decal to slot between them.
	GroundDecal->SetSortOrder(DecalSortOrder);

	GroundDecal->MarkRenderStateDirty();
}
