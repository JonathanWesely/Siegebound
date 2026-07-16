// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SiegeMeshJuiceComponent.generated.h"

class USceneComponent;

/**
 *  Shared procedural transform juice (M7 §6, TASK-155) — NO new assets, pure C++.
 *  Added in-constructor to ASummonedUnit and ABuilding (subclasses AMinerUnit /
 *  ATower inherit it); the owner points it at the ACTIVE visual mesh via
 *  SetTargetMesh (the static VisualMesh, or the M7 SkeletalVisualMesh when that
 *  is the runtime visual — TASK-159) and calls:
 *   - PlaySpawnSquash() on spawn: a 0.15 s squash→stretch→settle scale wobble
 *     on the mesh, restored EXACTLY to the authored RelativeScale3D (units AND
 *     buildings).
 *   - PlayRecoil(WorldFireDirection) on each tower shot: kicks the mesh back
 *     opposite the fire direction, easing back before the next shot (ATower).
 *
 *  Frame-rate-independent (elapsed/duration lerps) and drift-free (the authored
 *  scale + relative location are captured ONCE at SetTargetMesh and every
 *  animation returns to them EXACTLY — the TASK-020 cache-once / restore-exact
 *  discipline). Ticks ONLY while an animation is active; otherwise idle.
 *
 *  SCALE (squash) and LOCATION (recoil) are independent transform channels, so
 *  they never fight — and on a unit they never collide with the base-class
 *  attack lunge (which writes only the mesh's RelativeLocation, while a unit's
 *  spawn squash writes only its RelativeScale3D and units never recoil).
 *
 *  NOTE for buildings/towers: their VisualMesh is the collision ROOT, so this
 *  juice momentarily moves/scales that root (the spec names VisualMesh as the
 *  target). The offsets are tiny + brief and always restore exactly; a BP can
 *  zero SpawnSquashSeconds / RecoilDistance to disable if a playtest flags it.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API USiegeMeshJuiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USiegeMeshJuiceComponent();

	/** Points the juice at the mesh to animate and captures its authored scale + relative location ONCE (drift-free base). Null clears the target. */
	void SetTargetMesh(USceneComponent* InMesh);

	/** Starts the spawn squash-and-stretch (no-op with no target or a non-positive duration). */
	void PlaySpawnSquash();

	/** Starts a recoil kick opposite WorldFireDirection (no-op with no target, a zero direction, or a non-positive distance/duration). */
	void PlayRecoil(const FVector& WorldFireDirection);

	/** Per-frame driver (runs only while squash and/or recoil is active). */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	/** Seconds for the spawn squash-and-stretch. // GDD §6 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0"))
	float SpawnSquashSeconds = 0.15f; // GDD §6

	/** Peak squash/stretch fraction (0.30 = ±30% wobble at the start, decaying to 0). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0", ClampMax = "0.9"))
	float SquashAmplitude = 0.30f;

	/** Squash oscillations across the window (1.25 ≈ one squash + a settling stretch). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0.25"))
	float SquashOscillations = 1.25f;

	/** Tower recoil kick-back distance in units. // GDD §6 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0"))
	float RecoilDistance = 14.f; // GDD §6

	/** Tower recoil kick-and-return time in seconds. // GDD §6 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0"))
	float RecoilSeconds = 0.12f; // GDD §6

private:

	/** Enables/disables this component's tick to match whether any animation is active. */
	void UpdateTickEnabled();

	/** The mesh being animated (static VisualMesh or M7 SkeletalVisualMesh). */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> TargetMesh;

	/** Authored RelativeScale3D captured once (squash restores to EXACTLY this). */
	FVector BaseScale = FVector::OneVector;

	/** Authored RelativeLocation captured once (recoil restores to EXACTLY this). */
	FVector BaseRelativeLocation = FVector::ZeroVector;

	/** True once SetTargetMesh captured a valid base. */
	bool bBaseCaptured = false;

	//~ Spawn squash state
	bool bSquashActive = false;
	float SquashElapsed = 0.f;

	//~ Recoil state
	bool bRecoilActive = false;
	float RecoilElapsed = 0.f;

	/** Recoil kick direction in the mesh's PARENT space (world back-direction transformed once per kick). */
	FVector RecoilLocalDir = FVector::ZeroVector;
};
