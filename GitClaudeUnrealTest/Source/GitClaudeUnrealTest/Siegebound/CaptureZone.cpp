// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CaptureZone.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"

namespace
{
	// Owner-tint colors (TASK-263 contract; Blue/Red match MI_TeamColor_*).
	const FLinearColor NeutralZoneColor(0.5f, 0.5f, 0.5f);
	const FLinearColor BlueZoneColor(0.05f, 0.30f, 1.00f);
	const FLinearColor RedZoneColor(1.00f, 0.10f, 0.05f);
}

ACaptureZone::ACaptureZone()
{
	// No per-frame work: the capture eval runs on a repeating timer, never
	// per-tick (TASK-004 never-per-tick law).
	PrimaryActorTick.bCanEverTick = false;

	// Scene root; the decal attaches beneath it and projects down.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Owner-tint decal: relative pitch -90 => the decal faces -Z (projects
	// straight down onto the terrain), mirroring the ADecalActor / M_SpellReticle
	// recipe (SiegePlayerController reticle). DecalSize is applied from
	// ZoneHalfExtent in ApplyDecalFootprint (constructor seed + OnConstruction +
	// BeginPlay) so instance edits take.
	ZoneDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("ZoneDecal"));
	ZoneDecal->SetupAttachment(SceneRoot);
	ZoneDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	// Visual contract: soft path, never a hard reference (the material is
	// authored in parallel, TASK-263, and is null-safe if absent).
	ZoneDecalMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_CaptureZone.M_CaptureZone")));

	ApplyDecalFootprint();
}

void ACaptureZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep the editor decal footprint matched to ZoneHalfExtent while placed /
	// previewed. Silent: OnConstruction re-runs on every property tweak.
	ApplyDecalFootprint();
}

void ACaptureZone::BeginPlay()
{
	Super::BeginPlay();

	ApplyDecalFootprint();

	// Soft-load the owner-tint material and wrap it in a MID so EvaluateCapture /
	// ResetCaptureZone can drive `ZoneColor` by owner. Null-safe: a missing
	// material leaves no MID (no visual), the capture mechanic still runs, and
	// the miss is logged exactly once.
	if (ZoneDecal)
	{
		if (UMaterialInterface* DecalMaterial = ZoneDecalMaterialAsset.LoadSynchronous())
		{
			ZoneDecal->SetDecalMaterial(DecalMaterial);
			ZoneDecalMID = ZoneDecal->CreateDynamicMaterialInstance();
		}
		else if (!bWarnedMissingMaterial)
		{
			bWarnedMissingMaterial = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] CaptureZone: owner-tint material '%s' did not resolve — capturing runs without a visual (TASK-263 authors it)."),
				*GetNameSafe(this), *ZoneDecalMaterialAsset.ToString());
		}
	}

	// Seed the decal to the current (Neutral) owner.
	ApplyOwnerColorToDecal();

	// Authoritative capture eval only (local-authority world today; M8 replication
	// revisit — see class doc). A pure client would not run this and, with
	// CaptureOwner unreplicated, would not tint — an accepted M8 gap.
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				CaptureEvalTimerHandle, this, &ACaptureZone::EvaluateCapture,
				CaptureEvalInterval, /*bLoop=*/ true, /*FirstDelay=*/ CaptureEvalInterval);
		}
	}
}

void ACaptureZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureEvalTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool ACaptureZone::IsPointInZone(const FVector& Point) const
{
	// 2D (XY) box about the actor origin; Z ignored (a spawn/region test).
	const FVector Center = GetActorLocation();
	return FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X
		&& FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y;
}

bool ACaptureZone::CanTeamSpawnHere(ETeamId Team, const FVector& Point) const
{
	// Legal capture-zone spawn iff inside the box AND this team currently holds
	// it. A Neutral zone matches neither team => nobody may spawn there.
	return IsPointInZone(Point) && CaptureOwner == TeamToState(Team);
}

void ACaptureZone::ResetCaptureZone()
{
	// §3.9 reset path: force Neutral, re-tint, and broadcast UNCONDITIONALLY
	// (reset-path broadcast law) so listeners snap back even if already Neutral.
	CaptureOwner = ECaptureState::Neutral;
	ApplyOwnerColorToDecal();
	OnCaptureOwnerChanged.Broadcast(this, CaptureOwner);
}

void ACaptureZone::EvaluateCapture()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 BlueCount = 0;
	int32 RedCount = 0;

	// "Unit" = any ASummonedUnit (AMinerUnit is a subclass, so miners are
	// included) OR the AHeroCharacter. Buildings / towers / castles / gold nodes
	// are NOT ASummonedUnit / AHeroCharacter, so they are excluded by type.
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
		if (Unit->GetTeamId() == ETeamId::Blue)
		{
			++BlueCount;
		}
		else
		{
			++RedCount;
		}
	}

	for (TActorIterator<AHeroCharacter> It(World); It; ++It)
	{
		AHeroCharacter* Hero = *It;
		if (!IsValid(Hero) || Hero->IsDead())
		{
			continue;
		}
		if (!IsPointInZone(Hero->GetActorLocation()))
		{
			continue;
		}
		if (Hero->GetTeamId() == ETeamId::Blue)
		{
			++BlueCount;
		}
		else
		{
			++RedCount;
		}
	}

	// Symmetric capture rule. Empty (both 0) latches the last owner => start from
	// the current owner and only move on a decisive/contested state.
	ECaptureState NewOwner = CaptureOwner;
	if (BlueCount > 0 && RedCount == 0)
	{
		NewOwner = ECaptureState::Blue;
	}
	else if (RedCount > 0 && BlueCount == 0)
	{
		NewOwner = ECaptureState::Red;
	}
	else if (BlueCount > 0 && RedCount > 0)
	{
		// Contested — Jonathan RULING 2026-07-23: neutralize (default). The
		// sticky alternative (leave the owner unchanged) survives when the
		// toggle is turned off.
		if (bNeutralizeWhenContested)
		{
			NewOwner = ECaptureState::Neutral;
		}
	}
	// else both 0 => empty => NewOwner stays == CaptureOwner (unchanged).

	SetCaptureOwner(NewOwner);
}

void ACaptureZone::SetCaptureOwner(ECaptureState NewOwner)
{
	if (NewOwner == CaptureOwner)
	{
		return;
	}

	CaptureOwner = NewOwner;
	ApplyOwnerColorToDecal();
	OnCaptureOwnerChanged.Broadcast(this, CaptureOwner);
}

ECaptureState ACaptureZone::TeamToState(ETeamId Team)
{
	return (Team == ETeamId::Blue) ? ECaptureState::Blue : ECaptureState::Red;
}

void ACaptureZone::ApplyOwnerColorToDecal()
{
	if (!ZoneDecalMID)
	{
		return;
	}

	FLinearColor Color = NeutralZoneColor;
	switch (CaptureOwner)
	{
	case ECaptureState::Blue:
		Color = BlueZoneColor;
		break;
	case ECaptureState::Red:
		Color = RedZoneColor;
		break;
	case ECaptureState::Neutral:
	default:
		Color = NeutralZoneColor;
		break;
	}

	ZoneDecalMID->SetVectorParameterValue(ZoneColorParamName, Color);
}

void ACaptureZone::ApplyDecalFootprint()
{
	if (!ZoneDecal)
	{
		return;
	}

	// After the -90 pitch: DecalSize.X = projection half-depth along -Z; .Y/.Z =
	// the square footprint half-extents in the ground plane (the zone box).
	ZoneDecal->DecalSize = FVector(DecalProjectionDepth, ZoneHalfExtent.X, ZoneHalfExtent.Y);
	ZoneDecal->MarkRenderStateDirty();
}
