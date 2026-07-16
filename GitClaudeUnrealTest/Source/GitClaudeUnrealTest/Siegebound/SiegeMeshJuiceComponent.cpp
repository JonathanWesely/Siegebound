// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeMeshJuiceComponent.h"

#include "Components/SceneComponent.h"

USiegeMeshJuiceComponent::USiegeMeshJuiceComponent()
{
	// Ticks ONLY while an animation is active (enabled on Play*, disabled when idle) —
	// no per-frame cost otherwise (the ASummonedUnit lunge-tick discipline).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USiegeMeshJuiceComponent::SetTargetMesh(USceneComponent* InMesh)
{
	TargetMesh = InMesh;
	bBaseCaptured = false;

	if (InMesh)
	{
		// Capture the authored transform ONCE — every animation writes Base + f(t) or
		// exactly Base, so no number of squashes/recoils can ever drift the rest pose.
		BaseScale = InMesh->GetRelativeScale3D();
		BaseRelativeLocation = InMesh->GetRelativeLocation();
		bBaseCaptured = true;
	}
}

void USiegeMeshJuiceComponent::PlaySpawnSquash()
{
	if (!TargetMesh || !bBaseCaptured || SpawnSquashSeconds <= 0.f)
	{
		return;
	}

	bSquashActive = true;
	SquashElapsed = 0.f;
	UpdateTickEnabled();
}

void USiegeMeshJuiceComponent::PlayRecoil(const FVector& WorldFireDirection)
{
	if (!TargetMesh || !bBaseCaptured || RecoilDistance <= 0.f || RecoilSeconds <= 0.f)
	{
		return;
	}

	// Kick opposite the (horizontal) fire direction. Convert the world back-vector
	// into the mesh's PARENT space so the relative-location offset reads correctly
	// regardless of the mesh's own yaw / attachment (towers: mesh is the root, so
	// the parent is world and this is an identity transform).
	FVector WorldBack = -WorldFireDirection;
	WorldBack.Z = 0.f;
	WorldBack = WorldBack.GetSafeNormal();
	if (WorldBack.IsNearlyZero())
	{
		return; // degenerate fire direction — skip this recoil, no drift
	}

	if (const USceneComponent* Parent = TargetMesh->GetAttachParent())
	{
		RecoilLocalDir = Parent->GetComponentTransform().InverseTransformVectorNoScale(WorldBack).GetSafeNormal();
	}
	else
	{
		RecoilLocalDir = WorldBack;
	}

	bRecoilActive = true;
	RecoilElapsed = 0.f;
	UpdateTickEnabled();
}

void USiegeMeshJuiceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!TargetMesh || !bBaseCaptured)
	{
		bSquashActive = false;
		bRecoilActive = false;
		UpdateTickEnabled();
		return;
	}

	// SCALE channel — spawn squash-and-stretch: a decaying sine wobble that
	// stretches Z while squashing XY (and vice-versa), settling to EXACTLY BaseScale.
	if (bSquashActive)
	{
		SquashElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(SquashElapsed / SpawnSquashSeconds, 0.f, 1.f);
		if (Alpha >= 1.f)
		{
			TargetMesh->SetRelativeScale3D(BaseScale);
			bSquashActive = false;
		}
		else
		{
			const float Decay = 1.f - Alpha;
			const float Wobble = SquashAmplitude * FMath::Sin(2.f * UE_PI * SquashOscillations * Alpha) * Decay;
			const FVector Squashed(
				BaseScale.X * (1.f - Wobble),
				BaseScale.Y * (1.f - Wobble),
				BaseScale.Z * (1.f + Wobble));
			TargetMesh->SetRelativeScale3D(Squashed);
		}
	}

	// LOCATION channel — tower recoil: instant kick back, easing return to EXACTLY
	// BaseRelativeLocation (ease-out so it snaps back and settles before the next shot).
	if (bRecoilActive)
	{
		RecoilElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(RecoilElapsed / RecoilSeconds, 0.f, 1.f);
		if (Alpha >= 1.f)
		{
			TargetMesh->SetRelativeLocation(BaseRelativeLocation);
			bRecoilActive = false;
		}
		else
		{
			// magnitude 1 → 0 with an ease-out return (square of the remaining fraction)
			const float Remaining = 1.f - Alpha;
			const float Magnitude = RecoilDistance * Remaining * Remaining;
			TargetMesh->SetRelativeLocation(BaseRelativeLocation + RecoilLocalDir * Magnitude);
		}
	}

	UpdateTickEnabled();
}

void USiegeMeshJuiceComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(bSquashActive || bRecoilActive);
}
