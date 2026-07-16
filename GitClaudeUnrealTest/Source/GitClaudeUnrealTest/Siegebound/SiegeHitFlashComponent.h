// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SiegeHitFlashComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;

/**
 *  Shared §6 hit-flash (M7, TASK-154). Added ONCE in-constructor to every
 *  combatant base — ASummonedUnit, ABuilding, AHeroCharacter, ACastle (subclasses
 *  AMinerUnit / ATower inherit it) — each names its instance HitFlashComponent.
 *  The owner calls TriggerFlash() from its EXISTING TakeDamage choke point on an
 *  ACTUAL damage event (never on a heal/regen or an ignored friendly-fire hit).
 *
 *  MECHANISM — OVERLAY, not slot-swap (deliberate improvement over the literal
 *  spec): the flash sets UMeshComponent::SetOverlayMaterial(M_HitFlash) on each
 *  visible static/skeletal mesh for HitFlashSeconds, then clears it back to
 *  nullptr. The base material slots are NEVER touched, which is why this returns
 *  the actor to its EXACT prior look including the team-recolored slot 0 (the
 *  acceptance), and — critically — composes cleanly with the TASK-157 castle
 *  crumble MI swap on the SAME castle mesh (a slot-swap flash would fight the
 *  crumble swap on those slots and could leak the wrong material). It also works
 *  identically on the static VisualMesh and the (M7 TASK-159) SkeletalVisualMesh:
 *  overlay is a UMeshComponent API shared by both.
 *
 *  Null-safe: a missing /Game/Materials/M_HitFlash (art arrives in TASK-174) is
 *  logged once and means NO flash, never a crash. Timer-driven, zero per-tick cost.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API USiegeHitFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USiegeHitFlashComponent();

	/**
	 *  Flashes every visible static/skeletal mesh on the owner white for
	 *  HitFlashSeconds via an overlay material, then clears it. Re-triggering
	 *  while already flashing just RE-ARMS the timer (never stacks / never
	 *  double-caches). No-op when disabled or with M_HitFlash absent.
	 */
	void TriggerFlash();

protected:

	/** Caches the owner's static/skeletal mesh components (excludes the UWidgetComponent health bar). */
	virtual void BeginPlay() override;

	/** Clears the flash timer (belt-and-braces; the overlay is transient anyway). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Master opt-out (per-BP). When false, TriggerFlash is a no-op. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback")
	bool bEnableHitFlash = true;

	/** White-flash duration in seconds. // GDD §6 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0.01"))
	float HitFlashSeconds = 0.10f; // GDD §6

	/** Overlay material shown during the flash (default /Game/Materials/M_HitFlash). Soft — null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback")
	FString HitFlashMaterialPath = TEXT("/Game/Materials/M_HitFlash");

private:

	/** Clears the overlay material on every cached mesh (flash end). */
	void ClearFlash();

	/** The owner's static/skeletal mesh components, gathered once at BeginPlay (never the widget bar). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> FlashMeshes;

	/** True while a flash is showing — a re-trigger only re-arms the timer. */
	bool bFlashing = false;

	/** Drives ClearFlash after HitFlashSeconds; re-armed on re-trigger, never stacked. */
	FTimerHandle FlashTimerHandle;
};
