// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "DamageNumberActor.generated.h"

class UUserWidget;
class UWidgetComponent;

/**
 *  Self-managing floating damage number (M7 §6, TASK-156). Spawned by
 *  USiegeFeedbackLibrary::ShowDamageNumber at a hit location; owns a screen-space
 *  UWidgetComponent showing /Game/UI/WBP_DamageNumber (a UDamageNumberWidget),
 *  rises + fades over Lifetime (C++-owned animation), then destroys itself.
 *
 *  - NULL-SAFE: the widget asset is authored later (TASK-176-adjacent). Until it
 *    exists, Spawn() no-ops (logged once) — never a crash, never an empty actor.
 *  - CAPPED: a process-wide live count (LiveCount) caps concurrent numbers
 *    (MaxConcurrentNumbers) so 60+ units taking damage can't leak/uncap widgets
 *    (§6 perf budget). Over the cap, Spawn() simply drops the number.
 *  - Cosmetic only: never collides, never affects gameplay/nav.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ADamageNumberActor : public AActor
{
	GENERATED_BODY()

public:

	ADamageNumberActor();

	/**
	 *  Factory (the single entry — USiegeFeedbackLibrary::ShowDamageNumber forwards
	 *  here): resolves the widget class once, honors the concurrency cap, spawns at
	 *  WorldLocation, and pushes Amount + Tint. All no-op-safe.
	 */
	static void Spawn(const UObject* WorldContextObject, float Amount, const FVector& WorldLocation, const FLinearColor& Tint);

	/** Rise + fade + self-destruct. */
	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Resolves the widget class null-safe, creates the widget, and increments the live count. */
	virtual void BeginPlay() override;

	/** Decrements the live count. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Screen-space number face. Widget class soft-resolved at BeginPlay (WBP_DamageNumber). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<UWidgetComponent> NumberWidget;

	/** WBP_DamageNumber class (UDamageNumberWidget). Soft — resolved null-safe at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback")
	TSoftClassPtr<UUserWidget> NumberWidgetClass;

	/** Seconds the number lives before self-destruct (rise+fade window). // GDD §6 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0.05"))
	float Lifetime = 0.9f;

	/** Upward drift speed in units/second over the lifetime. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Feedback", meta = (ClampMin = "0"))
	float RiseSpeed = 70.f;

private:

	/** Pushes Amount + Tint to the created UDamageNumberWidget (null-safe). Called right after spawn. */
	void Init(float Amount, const FLinearColor& Tint);

	/**
	 *  Resolves /Game/UI/WBP_DamageNumber ONCE (cached tri-state), logging a single
	 *  miss. Returns whether damage numbers are available this session — Spawn()
	 *  early-outs when false so no empty actor churns before the art lands.
	 */
	static bool AreDamageNumbersAvailable();

	/** Seconds elapsed since spawn. */
	float Elapsed = 0.f;

	/** Amount pushed to the widget (cached for the deferred push after BeginPlay creates the widget). */
	float PendingAmount = 0.f;

	/** Tint pushed to the widget. */
	FLinearColor PendingTint = FLinearColor::White;

	/** Process-wide live count for the concurrency cap. */
	static int32 LiveCount;

	/** Max concurrent floating numbers (§6 perf budget). Over this, Spawn() drops the number. */
	static constexpr int32 MaxConcurrentNumbers = 48;
};
