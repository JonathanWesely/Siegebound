// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SiegePlayerState.generated.h"

/**
 *  Broadcast whenever the player's gold value actually changes
 *  (passive income tick, spend, reset). NewGold is the post-change,
 *  clamped value — always in [0, MaxGold].
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

/**
 *  Siegebound player state — owns the gold economy (GDD §3.2, M1 subset).
 *
 *  - Gold starts at 50 and gains +2 every 1.0 s on a timer started in BeginPlay.
 *  - Gold is hard-capped at 999 and can never go negative.
 *  - ALL mutations route through the private SetGold() so no code path can
 *    skip the clamp or the OnGoldChanged broadcast.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegePlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	/** Fired on every gold mutation with the new value. The HUD (WBP_HUD, TASK-011) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Gold")
	FOnGoldChanged OnGoldChanged;

	/** Current gold, always in [0, MaxGold]. HUD should call this once on construct to seed its display, then rely on OnGoldChanged. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
	int32 GetGold() const { return Gold; }

	/** True if Cost is non-negative and no greater than the current gold. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
	bool CanAfford(int32 Cost) const;

	/**
	 *  Deducts Cost from Gold. Refuses and returns false — changing nothing and
	 *  broadcasting nothing — if Cost is negative or unaffordable. Gold never goes negative.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	bool SpendGold(int32 Cost);

	/** Play Again (GDD §3.9): resets gold to StartingGold and (re)starts the passive income timer. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
	void ResetGold();

protected:

	/** Seeds gold to StartingGold and starts the passive income timer. */
	virtual void BeginPlay() override;

	/** Stops the passive income timer. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Gold at match start and after a Play Again reset (GDD §3.2). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 StartingGold = 50;

	/** Passive income added on each timer tick (GDD §3.2). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 GoldPerTick = 2;

	/** Seconds between passive income ticks. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0.05"))
	float GoldTickInterval = 1.0f;

	/** Hard cap — gold never exceeds this value (GDD §3.2). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "0"))
	int32 MaxGold = 999;

private:

	/**
	 *  The ONLY place Gold is written. Clamps NewGold to [0, MaxGold] and, if the
	 *  stored value actually changed, broadcasts OnGoldChanged with the new value.
	 *  A write that clamps back to the current value (e.g. an income tick while
	 *  pinned at the cap) is not a mutation and does not broadcast.
	 */
	void SetGold(int32 NewGold);

	/** Passive income timer callback: adds GoldPerTick, clamped at MaxGold. */
	void HandleGoldTick();

	/** (Re)starts the repeating passive income timer. */
	void StartIncomeTimer();

	/** Current gold. Mutate ONLY via SetGold(). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Gold", meta = (AllowPrivateAccess = "true"))
	int32 Gold = 50;

	/** Handle for the repeating passive income timer. */
	FTimerHandle GoldTickTimerHandle;
};
