// Copyright Epic Games, Inc. All Rights Reserved.


#include "Siegebound/SiegePlayerState.h"
#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/SiegeGameState.h"
#include "TimerManager.h"

void ASiegePlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Seed gold and start passive income. ResetGold is the single entry point
	// for "fresh match" economy state, shared with Play Again (GDD §3.9).
	ResetGold();

	// Overtime (GDD §3.2): the shared ASiegeGameState clock latches at 7:00 —
	// bind so rate listeners hear about the doubling the moment it happens.
	// The doubling itself is read LIVE in GetGoldRate(), so ACCRUAL stays
	// correct even without this bind; only the HUD notification depends on it.
	if (ASiegeGameState* SiegeGameState = GetSiegeGameState())
	{
		SiegeGameState->OnOvertimeStarted.AddUniqueDynamic(this, &ASiegePlayerState::HandleOvertimeStarted);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] No ASiegeGameState found at BeginPlay — the §3.2 overtime doubling will never activate (ASiegeGameMode sets GameStateClass, TASK-024)."),
			*GetNameSafe(this));
	}

	// Change-detection baseline for OnGoldRateChanged: consumers seed from
	// GetGoldRate() (seed-then-bind law) and only need broadcasts for CHANGES.
	CachedGoldRate = GetGoldRate();
}

void ASiegePlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GoldTickTimerHandle);
	}

	// Unbind the overtime handler (hygiene — a dying object's dynamic
	// bindings are dropped lazily by the delegate anyway).
	if (ASiegeGameState* SiegeGameState = GetSiegeGameState())
	{
		SiegeGameState->OnOvertimeStarted.RemoveDynamic(this, &ASiegePlayerState::HandleOvertimeStarted);
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
	// Match-end freeze gate (§3.9, TASK-024): PauseIncome() clears the timer,
	// but ResetGold() legitimately restarts it (M1 law, qa/TASK-005-report.md
	// major 1) — if that ever happens while paused, this gate keeps gold frozen.
	if (bIncomePaused)
	{
		return;
	}

	// Rate-composed accrual (GDD §3.2/§3.3, TASK-024): base (doubled in
	// overtime) + 1 per arrived miner — the same GetGoldRate() the HUD reads,
	// so the displayed rate always equals the observed accrual.
	SetGold(Gold + GetGoldRate());
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

int32 ASiegePlayerState::GetGoldRate() const
{
	// GDD §3.2: base 2/s, doubling to 4/s at 7:00 — read the shared overtime
	// latch LIVE so the rate can never desync from the match clock.
	const ASiegeGameState* SiegeGameState = GetSiegeGameState();
	const bool bOvertime = SiegeGameState && SiegeGameState->IsOvertimeActive();
	const int32 BaseRate = bOvertime ? (GoldPerTick * OvertimeIncomeMultiplier) : GoldPerTick;

	// GDD §3.3: +1 per miner that ARRIVED at its node; en-route miners add nothing.
	// GDD §8: plus the flat non-miner income (Deep Mines, TASK-057) — additive
	// and separate from the miner count/cap; not doubled by overtime (only the
	// base doubled above), mirroring miner income.
	return BaseRate + (MinerIncomeCount * MinerGoldPerTick) + FlatIncomePerTick;
}

void ASiegePlayerState::AddIncome(int32 GoldPerTickDelta)
{
	if (GoldPerTickDelta <= 0)
	{
		// register/unregister must stay symmetric and positive (ADeepMine passes
		// its DeepMineIncome UPROPERTY, GDD §8 = 2). A 0/negative amount is a
		// caller bug, never a legitimate flat-income registration.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] AddIncome(%d) refused — flat income deltas must be positive (TASK-057 §8 Deep Mine)."),
			*GetNameSafe(this), GoldPerTickDelta);
		return;
	}

	FlatIncomePerTick += GoldPerTickDelta;

	// A positive delta always raises the composed rate — broadcast on the actual
	// change (CONVENTIONS delegate law). Separate from the miner path entirely:
	// GetAliveMinerCount / CanAddMiner / the miner delegates are never touched.
	RefreshGoldRate(/*bForceBroadcast*/ false);
}

void ASiegePlayerState::RemoveIncome(int32 GoldPerTickDelta)
{
	if (GoldPerTickDelta <= 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] RemoveIncome(%d) refused — flat income deltas must be positive (TASK-057 §8 Deep Mine)."),
			*GetNameSafe(this), GoldPerTickDelta);
		return;
	}

	if (GoldPerTickDelta > FlatIncomePerTick)
	{
		// A caller removed more than was ever added (symmetry broken). Clamp to 0
		// so the rate can never go below base, and log — the ADeepMine contract is
		// AddIncome once at BeginPlay, RemoveIncome once on death.
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] RemoveIncome(%d) exceeds the flat income accumulator (%d) — clamped to 0 (a Deep Mine unregistered more than it registered; TASK-057)."),
			*GetNameSafe(this), GoldPerTickDelta, FlatIncomePerTick);
		FlatIncomePerTick = 0;
	}
	else
	{
		FlatIncomePerTick -= GoldPerTickDelta;
	}

	RefreshGoldRate(/*bForceBroadcast*/ false);
}

void ASiegePlayerState::AddMinerIncome()
{
	++MinerIncomeCount;

	if (MinerIncomeCount > AliveMinerCount)
	{
		// Diagnostic only: arrived miners are a subset of alive miners, so this
		// means an AMinerUnit double-reported arrival or skipped registration
		// (TASK-025 contract: RegisterMinerAlive at BeginPlay, AddMinerIncome
		// exactly once on arrival).
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] AddMinerIncome: income count (%d) exceeds alive miners (%d) — a miner is double-reporting arrival (TASK-025 contract)."),
			*GetNameSafe(this), MinerIncomeCount, AliveMinerCount);
	}

	RefreshGoldRate(/*bForceBroadcast*/ false);
}

void ASiegePlayerState::RemoveMinerIncome()
{
	if (MinerIncomeCount <= 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] RemoveMinerIncome with no active miner income — refused (TASK-025 calls this only for a miner that had ARRIVED)."),
			*GetNameSafe(this));
		return;
	}

	--MinerIncomeCount;
	RefreshGoldRate(/*bForceBroadcast*/ false);
}

void ASiegePlayerState::RegisterMinerAlive()
{
	++AliveMinerCount;

	if (AliveMinerCount > MaxActiveMiners)
	{
		// Diagnostic only: the cap is enforced BEFORE any spawn, at play time,
		// via CanAddMiner (GDD §3.3 / TASK-030) — a miner that got here anyway
		// bypassed that gate. Counting stays truthful either way.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] RegisterMinerAlive: %d alive miners exceeds the cap (%d, GDD §3.3) — a spawner skipped the CanAddMiner play-time gate (TASK-030)."),
			*GetNameSafe(this), AliveMinerCount, MaxActiveMiners);
	}

	// ++ always changes the count — broadcast unconditionally is still
	// broadcast-on-actual-change (CONVENTIONS delegate law).
	OnMinerCountChanged.Broadcast(AliveMinerCount);
}

void ASiegePlayerState::UnregisterMinerAlive()
{
	if (AliveMinerCount <= 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] UnregisterMinerAlive with no alive miners — refused (TASK-025 calls this exactly once per miner death)."),
			*GetNameSafe(this));
		return;
	}

	--AliveMinerCount;
	OnMinerCountChanged.Broadcast(AliveMinerCount);
}

void ASiegePlayerState::PauseIncome()
{
	// Flag first, then clear: even if a paused-window tick were somehow already
	// queued this frame, HandleGoldTick's gate refuses it (belt-and-braces).
	bIncomePaused = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GoldTickTimerHandle);
	}
}

void ASiegePlayerState::ResumeIncome()
{
	bIncomePaused = false;
	StartIncomeTimer();
}

void ASiegePlayerState::ResetEconomy()
{
	AliveMinerCount = 0;
	MinerIncomeCount = 0;

	// §8 Deep Mine income cleared too (TASK-057): Play Again destroys every
	// ABuilding first (game mode step 2b → ADeepMine::EndPlay → RemoveIncome),
	// so this is normally already 0 — zeroing it unconditionally here is the
	// same belt-and-braces as the miner counts above, and covers a mid-match
	// reset where a mine slipped the destroy sweep.
	FlatIncomePerTick = 0;

	// Reset-path broadcasts (CONVENTIONS delegate law): unconditional, so HUD
	// listeners re-seed even when the values were already at base. The game
	// mode ran ASiegeGameState::ResetClock() before this, so the rate below
	// re-derives against a cleared overtime latch and lands on the base 2/s.
	OnMinerCountChanged.Broadcast(AliveMinerCount);
	RefreshGoldRate(/*bForceBroadcast*/ true);
}

void ASiegePlayerState::HandleOvertimeStarted()
{
	// The base rate just doubled (GDD §3.2). GetGoldRate() reads the latch
	// live — this handler exists purely to notify rate listeners of the change.
	RefreshGoldRate(/*bForceBroadcast*/ false);
}

void ASiegePlayerState::RefreshGoldRate(bool bForceBroadcast)
{
	const int32 NewRate = GetGoldRate();

	if (!bForceBroadcast && NewRate == CachedGoldRate)
	{
		return;
	}

	CachedGoldRate = NewRate;
	OnGoldRateChanged.Broadcast(NewRate);
}

ASiegeGameState* ASiegePlayerState::GetSiegeGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<ASiegeGameState>() : nullptr;
}
