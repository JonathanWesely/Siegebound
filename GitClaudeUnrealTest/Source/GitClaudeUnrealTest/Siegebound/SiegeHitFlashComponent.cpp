// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeHitFlashComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "TimerManager.h"

USiegeHitFlashComponent::USiegeHitFlashComponent()
{
	// Timer-driven — no tick (spec: no per-tick cost).
	PrimaryComponentTick.bCanEverTick = false;
}

void USiegeHitFlashComponent::BeginPlay()
{
	Super::BeginPlay();

	// Gather the owner's STATIC + SKELETAL mesh components ONCE. Deliberately NOT
	// generic UMeshComponent: UWidgetComponent (the overhead health bar) IS a
	// UMeshComponent, and overlaying it would white-out the bar during a flash.
	// Component pointers are stable for the actor's life; per-flash we filter on
	// live visibility, so the (M7 TASK-159) skeletal mesh — activated after this
	// runs in the owner's BeginPlay — is flashed once it becomes the visible mesh.
	if (const AActor* OwnerActor = GetOwner())
	{
		TArray<UMeshComponent*> MeshComponents;
		OwnerActor->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* Mesh : MeshComponents)
		{
			if (Mesh && (Mesh->IsA<UStaticMeshComponent>() || Mesh->IsA<USkeletalMeshComponent>()))
			{
				FlashMeshes.Add(Mesh);
			}
		}
	}
}

void USiegeHitFlashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void USiegeHitFlashComponent::TriggerFlash()
{
	if (!bEnableHitFlash)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Null-safe soft resolve (cached, logged once): missing M_HitFlash (art lands
	// in TASK-174) means NO flash, never a crash.
	UMaterialInterface* FlashMaterial = USiegeFeedbackLibrary::ResolveMaterial(HitFlashMaterialPath);
	if (!FlashMaterial)
	{
		return;
	}

	// Overlay pass on every VISIBLE static/skeletal mesh — base slots untouched, so
	// team recolor (slot 0) and the castle crumble swap are never disturbed. A
	// re-trigger mid-flash re-arms the timer below without re-applying (idempotent).
	if (!bFlashing)
	{
		for (UMeshComponent* Mesh : FlashMeshes)
		{
			if (Mesh && Mesh->IsVisible())
			{
				Mesh->SetOverlayMaterial(FlashMaterial);
			}
		}
		bFlashing = true;
	}

	// (Re)arm the single one-shot restore timer — refresh, never stack.
	World->GetTimerManager().SetTimer(FlashTimerHandle, this, &USiegeHitFlashComponent::ClearFlash, HitFlashSeconds, /*bLoop=*/ false);
}

void USiegeHitFlashComponent::ClearFlash()
{
	// Clear the overlay everywhere (idempotent — clearing an absent overlay is a
	// no-op). We clear ALL cached meshes, not only currently-visible ones, so a
	// mesh hidden mid-flash is still restored.
	for (UMeshComponent* Mesh : FlashMeshes)
	{
		if (Mesh)
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
	bFlashing = false;
}
