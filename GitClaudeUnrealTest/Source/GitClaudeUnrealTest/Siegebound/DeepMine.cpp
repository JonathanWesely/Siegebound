// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeepMine.h"

#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegePlayerState.h"
#include "TimerManager.h"

ADeepMine::ADeepMine()
{
	// Per-card class (TASK-063 names block: BP_Building_DeepMine with row
	// DeepMine): the row IDENTITY defaults here so LoadStats binds the 200 HP;
	// the +2/s income is the DeepMineIncome UPROPERTY, not a table stat (§8).
	// TASK-063's BP sets the same CardID (no-op) and the deferred-spawn
	// InitBuilding(Team, "DeepMine") agrees.
	CardID = FName(TEXT("DeepMine"));
}

void ADeepMine::BeginPlay()
{
	// Base: ApplyTeamMaterial (Team is already set — deferred spawn) + LoadStats
	// (200 HP from the row). Team is finalized before BeginPlay, so the economy
	// resolution below binds to the correct side.
	Super::BeginPlay();

	// §8: the +2/s starts the INSTANT the mine is placed — no walk. Register now;
	// if the owning player state is not resolvable yet (an early-spawn ordering
	// edge), TryRegisterIncome arms a light retry poll that keeps trying.
	TryRegisterIncome();
}

void ADeepMine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(IncomeRetryTimerHandle);

	// §8 raidable: removing the mine (combat death, the PlayAgain building sweep,
	// a KillZ fall) drops the owner's rate by exactly what we registered. World
	// teardown (PIE end / quit / travel) deliberately does NOT unregister — the
	// income dies with the player state, mirroring AMinerUnit's death bookkeeping.
	if (EndPlayReason == EEndPlayReason::Destroyed && bIncomeRegistered)
	{
		if (ASiegePlayerState* OwnerState = CachedOwnerState.Get())
		{
			OwnerState->RemoveIncome(DeepMineIncome);
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ADeepMine '%s': owner player state gone before death bookkeeping — its +%d/s could not be removed (it dies with the player state anyway)."),
				*GetNameSafe(this), DeepMineIncome);
		}
		bIncomeRegistered = false;
	}

	Super::EndPlay(EndPlayReason);
}

void ADeepMine::TryRegisterIncome()
{
	if (bIncomeRegistered)
	{
		// already active — nothing to do (and the retry poll, if it was ever
		// armed, cleared itself the moment it registered).
		GetWorldTimerManager().ClearTimer(IncomeRetryTimerHandle);
		return;
	}

	ASiegePlayerState* OwnerState = ResolveOwningPlayerState();
	if (!OwnerState)
	{
		// unresolvable (warned once in the resolver): arm/keep the retry poll so a
		// deferred player-state creation can still complete the registration. In
		// every designed flow the owning economy exists before a 15-gold Deep Mine
		// can be placed, so this normally never arms.
		if (!GetWorldTimerManager().IsTimerActive(IncomeRetryTimerHandle))
		{
			GetWorldTimerManager().SetTimer(IncomeRetryTimerHandle, this, &ADeepMine::TryRegisterIncome,
				FMath::Max(IncomeRegisterRetryInterval, 0.05f), /*bLoop=*/ true);
		}
		return;
	}

	// Latch BEFORE the mutation so nothing can double-register, then add exactly
	// DeepMineIncome to the flat NON-MINER income path (§8): this never touches
	// the alive-miner count, the MaxActiveMiners cap, or the miner delegates —
	// the §3.3 miner economy is untouched, and the cap does NOT apply to mines.
	CachedOwnerState = OwnerState;
	bIncomeRegistered = true;
	OwnerState->AddIncome(DeepMineIncome);

	// resolved — the retry poll (if armed) has done its job.
	GetWorldTimerManager().ClearTimer(IncomeRetryTimerHandle);
}

ASiegePlayerState* ADeepMine::ResolveOwningPlayerState()
{
	// Multi-team economy (TASK-043): resolve the OWNING team's player state via
	// ASiegeGameState::GetPlayerStateForTeam(Team) — the same accessor AMinerUnit
	// uses — so a Blue mine raises only Blue's rate and a Red bot mine only Red's,
	// rather than grabbing the first player state.
	const UWorld* World = GetWorld();
	ASiegeGameState* SiegeGameState = World ? World->GetGameState<ASiegeGameState>() : nullptr;
	if (!SiegeGameState)
	{
		if (!bWarnedNoGameState)
		{
			bWarnedNoGameState = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ADeepMine '%s': no ASiegeGameState yet — income registration retries (GameStateClass = ASiegeGameState, TASK-024)."),
				*GetNameSafe(this));
		}
		return nullptr;
	}

	// One-shot the team lookup once it fails with a LIVE GameState: a genuinely
	// mis-teamed mine would otherwise re-query — and GetPlayerStateForTeam logs
	// unconditionally on its not-found path — every retry tick forever. This is
	// the AMinerUnit::ResolveOwningPlayerState precedent (closes qa/TASK-043 WARN).
	// It never triggers in the designed flows, where the owning economy exists
	// before any mine can be placed.
	if (bWarnedNoTeamPlayerState)
	{
		return nullptr;
	}

	ASiegePlayerState* OwnerState = SiegeGameState->GetPlayerStateForTeam(Team);
	if (!OwnerState)
	{
		bWarnedNoTeamPlayerState = true;
	}
	return OwnerState;
}
