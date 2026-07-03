// Copyright Epic Games, Inc. All Rights Reserved.


#include "Siegebound/SiegePlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"

void ASiegePlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Seed gold and start passive income. ResetGold is the single entry point
	// for "fresh match" economy state, shared with Play Again (GDD §3.9).
	ResetGold();
}

void ASiegePlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GoldTickTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool ASiegePlayerState::CanAfford(int32 Cost) const
{
	// Negative costs are invalid requests, never "affordable" — keeps
	// CanAfford consistent with SpendGold's refusal of negative amounts.
	return Cost >= 0 && Cost <= Gold;
}

bool ASiegePlayerState::SpendGold(int32 Cost)
{
	if (!CanAfford(Cost))
	{
		// Refused: nothing changes, nothing broadcasts, gold never goes negative.
		return false;
	}

	SetGold(Gold - Cost);

	return true;
}

void ASiegePlayerState::ResetGold()
{
	// Play Again (GDD §3.9): back to starting gold...
	SetGold(StartingGold);

	// ...and make sure passive income is running for the new match, even if
	// a full reset cleared world timers.
	StartIncomeTimer();
}

void ASiegePlayerState::SetGold(int32 NewGold)
{
	// Single choke point for ALL gold writes: clamp first so no caller can
	// exceed the cap or go negative, then broadcast only on a real change.
	const int32 ClampedGold = FMath::Clamp(NewGold, 0, MaxGold);

	if (ClampedGold == Gold)
	{
		return;
	}

	Gold = ClampedGold;

	OnGoldChanged.Broadcast(Gold);
}

void ASiegePlayerState::HandleGoldTick()
{
	// TODO(M2): Overtime — at 7:00 match time the passive income rate doubles
	// (GDD §3.2). M1 uses the flat GoldPerTick rate only.
	SetGold(Gold + GoldPerTick);
}

void ASiegePlayerState::StartIncomeTimer()
{
	if (UWorld* World = GetWorld())
	{
		// SetTimer on an existing handle replaces the previous timer, so calling
		// ResetGold repeatedly never stacks multiple income ticks.
		World->GetTimerManager().SetTimer(GoldTickTimerHandle, this, &ASiegePlayerState::HandleGoldTick, GoldTickInterval, true);
	}
}
