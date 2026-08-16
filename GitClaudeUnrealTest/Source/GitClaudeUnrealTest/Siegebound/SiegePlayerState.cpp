// Copyright Epic Games, Inc. All Rights Reserved.


#include "Siegebound/SiegePlayerState.h"
#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Net/UnrealNetwork.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "TimerManager.h"

namespace
{
	/** One shared shape for the M8 authority guards (doc §3.2): a client copy reaching a server-only economy mutator is a design violation worth a loud line, never a crash. */
	void LogNonAuthorityEconomyCall(const ASiegePlayerState* PS, const TCHAR* FunctionName)
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("[%s] %s refused on a non-authority PlayerState copy — gold/economy is server-authoritative (M8 doc §3.2)."),
			*GetNameSafe(PS), FunctionName);
	}
}

void ASiegePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Doc §3.2: Team PLAIN (deliberately not InitialOnly — snapshot hazard vs the
	// bot's spawn-then-SetTeam and login-edge timing; the value never changes
	// after assignment so steady-state cost is zero). Gold OWNER-ONLY: each
	// machine sees its own economy only (TASK-357 gate d).
	DOREPLIFETIME(ASiegePlayerState, Team);
	DOREPLIFETIME_CONDITION(ASiegePlayerState, Gold, COND_OwnerOnly);
}

void ASiegePlayerState::OnRep_Gold()
{
	// CLIENT display path (doc §3.2): the value already changed via replication —
	// broadcast the EXISTING delegate directly so the HUD updates with zero
	// widget changes. Deliberately NOT SetGold (the authority choke would be a
	// second writer/refuser here; documented in-code per the doc).
	OnGoldChanged.Broadcast(Gold);
}

void ASiegePlayerState::OnRep_Team()
{
	// P1 log-only seam (doc §3.2): no team delegate exists to fire; P2 hangs
	// recolor/team-HUD hooks here. The log proves the seat assignment reached
	// this machine (TASK-357 gate b evidence).
	UE_LOG(LogSiegeNet, Log, TEXT("[%s] Team replicated: %s (M8 seat assignment, doc §2)."),
		*GetNameSafe(this), Team == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
}

void ASiegePlayerState::BeginPlay()
{
	Super::BeginPlay();

	// ── M8 accrual-fork kill (TASK-356; the audit's WORST timer hazard, §3.1):
	// PlayerStates replicate to every client and BeginPlay runs there too — only
	// the AUTHORITY copy may seed gold and arm the income timer. A client copy
	// takes no timer and no seed-write (the Gold initializer already equals
	// StartingGold, so the pre-first-rep display is truthful); its gold arrives
	// owner-only via OnRep_Gold. The overtime BIND below stays on ALL copies:
	// it is read-only display logic (rate re-derive → HUD "+N/s"), and the
	// client's 7:00 latch arrives via ASiegeGameState::OnRep_OvertimeActive
	// firing the same delegate. Standalone: authority ⇒ byte-identical.
	if (HasAuthority())
	{
		// Seed gold and start passive income. ResetGold is the single entry point
		// for "fresh match" economy state, shared with Play Again (GDD §3.9).
		ResetGold();
	}

	// Overtime (GDD §3.2): the shared ASiegeGameState clock latches at 7:00 —
	// bind so rate listeners hear about the doubling the moment it happens.
	// The doubling itself is read LIVE on each base grant in HandleGoldTick
	// (and in GetGoldRate for display, TASK-089), so ACCRUAL stays correct
	// even without this bind; only the HUD notification depends on it.
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
	// M8 authority guard (doc §3.2): a client copy can never mutate gold.
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("SpendGold"));
		return false;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("ResetGold"));
		return;
	}

	// Play Again (GDD §3.9): back to starting gold...
	SetGold(StartingGold);

	// ...and make sure passive income is running for the new match, even if
	// a full reset cleared world timers.
	StartIncomeTimer();
}

void ASiegePlayerState::AddGold(int32 Amount)
{
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("AddGold"));
		return;
	}

	if (Amount <= 0)
	{
		// Grants must be positive — use SpendGold to deduct. A 0/negative amount
		// is a caller bug (TASK-071 Sandbox grant passes a large positive pile).
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] AddGold(%d) refused — grants must be positive (use SpendGold to deduct)."),
			*GetNameSafe(this), Amount);
		return;
	}

	// Route through SetGold so the [0, MaxGold] clamp and the OnGoldChanged
	// broadcast always apply — never a raw Gold write. MaxGold (999) caps the
	// result. The composed gold rate is untouched (no AddIncome), so a lump grant
	// never perturbs +N/s accrual (TASK-071: keep the normal rate).
	SetGold(Gold + Amount);
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

	// Decomposed accrual (TASK-089 2026-07-08 balance directive): miner + flat
	// income land EVERY 1.0 s tick — their per-second values are untouched by
	// this change (GDD §3.3 / §8) — while the BASE income lands only on every
	// BaseIncomeTickPeriod-th tick (default 1 ⇒ 1 gold per 1 s, §3.2 amended —
	// TASK-278 2026-07-24 reverted TASK-089's period 2 / 1-per-2-s rate; the
	// header's BaseIncomeTickPeriod default is the record of truth, not this
	// comment). This function deliberately does NOT call GetGoldRate() anymore:
	// that is now the HUD's rounded-up DISPLAY average, not the exact accrual.
	int32 TickGrant = (MinerIncomeCount * MinerGoldPerTick) + FlatIncomePerTick;

	// Base-income cadence: transient tick-parity counter (non-reflected,
	// CachedGoldRate pattern), zeroed by ResetEconomy so the first post-reset
	// base grant lands exactly on the BaseIncomeTickPeriod-th tick. >= (not ==)
	// so an editor-tuned period shrink can never strand the counter above the
	// threshold and stall base income forever.
	++BaseIncomeTickCounter;
	if (BaseIncomeTickCounter >= BaseIncomeTickPeriod)
	{
		BaseIncomeTickCounter = 0;

		// Overtime doubling (GDD §3.2): the shared latch is read LIVE on the
		// grant tick — multiply-per-grant, never cached — so base accrual can
		// never desync from the match clock.
		const ASiegeGameState* SiegeGameState = GetSiegeGameState();
		const bool bOvertime = SiegeGameState && SiegeGameState->IsOvertimeActive();
		TickGrant += bOvertime ? (GoldPerTick * OvertimeIncomeMultiplier) : GoldPerTick;
	}

	// Exactly ONE SetGold per tick (SetGold stays the single Gold writer, so at
	// most one OnGoldChanged per tick). A zero-grant tick — no miners, no flat
	// income, off-cadence — is a harmless SetGold no-op (no change, no broadcast).
	SetGold(Gold + TickGrant);
}

void ASiegePlayerState::StartIncomeTimer()
{
	// M8 authority belt (doc §3.2): every caller is already guarded; the private
	// arm-site refuses too so no future path can start a client-side accrual.
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("StartIncomeTimer"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// SetTimer on an existing handle replaces the previous timer, so calling
		// ResetGold repeatedly never stacks multiple income ticks.
		World->GetTimerManager().SetTimer(GoldTickTimerHandle, this, &ASiegePlayerState::HandleGoldTick, GoldTickInterval, true);
	}
}

int32 ASiegePlayerState::GetGoldRate() const
{
	// DISPLAY rate (TASK-089 2026-07-08): the per-second AVERAGE behind the
	// HUD's "+N/s" text — NO LONGER the exact per-tick accrual, and
	// HandleGoldTick no longer calls this. GDD §3.2 (amended): base
	// GoldPerTick (1) per BaseIncomeTickPeriod (1 since TASK-278 2026-07-24;
	// was 2) ticks, doubled at 7:00 — the shared overtime latch is read LIVE
	// so the display can never desync from the match clock.
	const ASiegeGameState* SiegeGameState = GetSiegeGameState();
	const bool bOvertime = SiegeGameState && SiegeGameState->IsOvertimeActive();
	const int32 EffectiveBase = bOvertime ? (GoldPerTick * OvertimeIncomeMultiplier) : GoldPerTick;

	// Base averaged over the grant period and rounded UP for display: with the
	// TASK-278 2026-07-24 defaults (period 1) the average is EXACT and the
	// round-up is a NO-OP — a truthful +1/s pre-overtime, +2/s in overtime. It
	// only bites again if the period is editor-tuned above 1. (HISTORY: at
	// TASK-089's period 2 the true base was 0.5/s and the round-up displayed
	// +1/s over it — ruled acceptable because "+0/s" over a visibly rising
	// counter reads as broken. Since TASK-278 the accrual matches the display.)
	// Round-up is STABLE (never alternates), so RefreshGoldRate change
	// detection is unaffected. BaseIncomeTickPeriod is ClampMin 1 — no /0.
	const int32 BaseRate = FMath::DivideAndRoundUp(EffectiveBase, BaseIncomeTickPeriod);

	// GDD §3.3: +1 per miner that ARRIVED at its node; en-route miners add nothing.
	// GDD §8: plus the flat non-miner income (Deep Mines, TASK-057) — additive
	// and separate from the miner count/cap; not doubled by overtime (only the
	// base doubled above), mirroring miner income. Both land EVERY 1.0 s tick,
	// so their display contribution is exact.
	return BaseRate + (MinerIncomeCount * MinerGoldPerTick) + FlatIncomePerTick;
}

void ASiegePlayerState::AddIncome(int32 GoldPerTickDelta)
{
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("AddIncome"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("RemoveIncome"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("AddMinerIncome"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("RemoveMinerIncome"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("RegisterMinerAlive"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("UnregisterMinerAlive"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("PauseIncome"));
		return;
	}

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
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("ResumeIncome"));
		return;
	}

	bIncomePaused = false;
	StartIncomeTimer();
}

void ASiegePlayerState::ResetEconomy()
{
	// M8 authority guard (doc §3.2).
	if (!HasAuthority())
	{
		LogNonAuthorityEconomyCall(this, TEXT("ResetEconomy"));
		return;
	}

	AliveMinerCount = 0;
	MinerIncomeCount = 0;

	// §8 Deep Mine income cleared too (TASK-057): Play Again destroys every
	// ABuilding first (game mode step 2b → ADeepMine::EndPlay → RemoveIncome),
	// so this is normally already 0 — zeroing it unconditionally here is the
	// same belt-and-braces as the miner counts above, and covers a mid-match
	// reset where a mine slipped the destroy sweep.
	FlatIncomePerTick = 0;

	// Base-income tick-parity counter back to 0 (TASK-089): the first
	// post-reset base grant lands exactly on the BaseIncomeTickPeriod-th tick,
	// so Play Again's cadence is deterministic — never inherited from the prior
	// match's parity. Lives HERE (not ResetGold) because the counter is economy
	// STATE exactly like the miner/flat counts above, and Play Again always
	// runs ResetEconomy (game mode order: ResetClock → ResetEconomy →
	// ResetGold → ResumeIncome).
	BaseIncomeTickCounter = 0;

	// Reset-path broadcasts (CONVENTIONS delegate law): unconditional, so HUD
	// listeners re-seed even when the values were already at base. The game
	// mode ran ASiegeGameState::ResetClock() before this, so the rate below
	// re-derives against a cleared overtime latch and lands on the pre-overtime
	// base display value (+1/s with the TASK-278 defaults, and it is EXACT —
	// the round-up TASK-089 relied on here is a no-op at period 1).
	OnMinerCountChanged.Broadcast(AliveMinerCount);
	RefreshGoldRate(/*bForceBroadcast*/ true);
}

void ASiegePlayerState::HandleOvertimeStarted()
{
	// The base accrual just doubled (GDD §3.2). Accrual reads the latch live in
	// HandleGoldTick — this handler exists purely to notify rate listeners of a
	// DISPLAY change. With the TASK-278 2026-07-24 defaults (period 1) the
	// rounded display base moves 1 -> 2 across the flip, so change detection
	// DOES fire and the HUD's "+N/s" text updates at 7:00. (HISTORY: at
	// TASK-089's period 2 the display base was 1 on BOTH sides, so this
	// broadcast was correctly suppressed and the 7:00 signal was the overtime
	// HUD indicator alone — ruling 4. TASK-278 reverted the TUNING, not this
	// handler; the bForceBroadcast=false call is correct under either.)
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
