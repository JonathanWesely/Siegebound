// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Siegebound/SummonedUnit.h"
#include "MinerUnit.generated.h"

class AGoldNode;
class ASiegePlayerState;
class UAudioComponent;

/**
 *  Siegebound miner — the §3.3 economy unit (card row Miner, TASK-025).
 *
 *  ASummonedUnit subclass that NEVER fights: it walks to the SAME-team
 *  AGoldNode, stands there, and activates +1 gold/s on the owning player
 *  state ON ARRIVAL only (~10 s walk from a mid-half placement, GDD §3.3).
 *  It stays attackable by enemies through the base (ITeamAgent + TakeDamage —
 *  §3.3: economy is a raidable investment); killing it removes its income.
 *
 *  NO-ATTACK-PATH RULE (qa/TASK-021-report.md WARN-1, BINDING): the Miner row
 *  carries Cadence 0, and the base clamps Cadence to a 0.05 s minimum — if a
 *  miner ever reached the Attack state it would swing 20×/s. The combat state
 *  machine is therefore sealed STRUCTURALLY, through protected base surfaces
 *  only (the base file is frozen qa-passed):
 *   1. StateCheckInterval = 0 (constructor): FTimerManager::SetTimer with a
 *      rate <= 0 CLEARS the handle instead of scheduling — LoadStatsAndStart's
 *      one arming call (it can only ever run once; bStatsLoaded latches) never
 *      arms the UpdateState timer. The state machine's only driver never runs.
 *   2. AggroRadius = 0 (constructor): AcquireTarget rejects every candidate at
 *      any distance > 0, so CurrentTarget can never be acquired — and
 *      UpdateState only enters Attack for a non-null CurrentTarget. Even the
 *      ONE synchronous UpdateState that LoadStatsAndStart makes inside
 *      Super::BeginPlay is acquisition-dead; at most it issues a castle-bound
 *      Advance that the gold-node walk replaces within the same call stack.
 *   3. ClearAllTimersForObject(this) right after Super::BeginPlay: kills
 *      anything armed during stat binding even if a serialized blueprint value
 *      ever re-legalized StateCheckInterval (TASK-024 precedent for this lever
 *      when the handle itself is private).
 *  No looping timer is ever started from row Cadence on this class; the
 *  miner's own poll runs from ArrivalCheckInterval, clamped >= 0.05 s at arm
 *  time (the WARN-1 guard rule).
 *
 *  Stats still bind "as usual" (GDD §3.0): Super::BeginPlay reads the Miner
 *  row from /Game/Data/DT_Cards — 30 HP, 350 speed — never hardcoded.
 *
 *  Economy bookkeeping (exact TASK-024 / handoffs/TASK-024.md contract):
 *   - RegisterMinerAlive()   — BeginPlay, exactly once (retried by the poll if
 *                              the player state was not resolvable yet).
 *   - AddMinerIncome()       — exactly once, on ARRIVAL at the node (never on
 *                              spawn); latched by bArrivedAtNode/bIncomeActive.
 *   - RemoveMinerIncome()    — death, ONLY if income had activated; called
 *                              before Unregister so income ⊆ alive holds at
 *                              every intermediate step.
 *   - UnregisterMinerAlive() — death, ALWAYS (arrived or not).
 *  "Death" is EndPlay with reason Destroyed — the single choke point for every
 *  removal-from-play path (combat death via the base's HandleDeath → Destroy,
 *  the PlayAgain unit sweep, a KillZ fall) — never at world teardown.
 *  CanAddMiner() is NOT called here: the §3.3 cap is enforced at play time by
 *  TASK-030, before any gold moves.
 *
 *  Movement/arrival: MoveToActor toward the nearest same-team AGoldNode
 *  (acceptance 0.8 × ArrivalRadius, the house fraction), then a ~0.25 s poll
 *  (never per-tick, TASK-004 law) detects arrival by 2D distance <=
 *  ArrivalRadius, stops movement ("stand at the node"; the mining "clink"
 *  audio is M7), and afterwards only heals the walk: a failed/hijacked/
 *  displaced move is re-issued toward the node. No same-team node in the
 *  level: log and idle (nodes are placed in TASK-036).
 *
 *  FreezeAI (TASK-028 contract): the base override stops the walk
 *  (StopMovement) and parks the unit; this class extends it to clear the
 *  arrival poll so nothing can re-issue MoveToActor under the match-end
 *  freeze. The poll additionally gates on IsAIFrozen()/IsUnitDead().
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AMinerUnit : public ASummonedUnit
{
	GENERATED_BODY()

public:

	AMinerUnit();

	/**
	 *  Match-end freeze (TASK-024 caller, TASK-028 base contract). Base:
	 *  clears the state/attack timers, cancels any lunge, StopMovement (this
	 *  is what halts the gold-node walk), parks Idle, latches the frozen flag.
	 *  Miner extension: clears the arrival/upkeep poll so the walk can never
	 *  be re-issued. Plain C++ override — no UFUNCTION re-declaration
	 *  (handoffs/TASK-028.md rule); idempotent like the base.
	 */
	virtual void FreezeAI() override;

	/** True once this miner reached its gold node (PIE verification hook — the IsAIFrozen/IsUnitDead house pattern). Income activated iff this AND an owner state was registered. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Miner")
	bool HasArrivedAtNode() const { return bArrivedAtNode; }

protected:

	/**
	 *  Super binds the card stats (30 HP / 350 speed from DT_Cards row Miner)
	 *  — with the state machine structurally sealed (see class doc). Then:
	 *  timer sweep (seal #3), RegisterMinerAlive on the owning team's player
	 *  state (TASK-024 contract), start the walk to the nearest same-team
	 *  AGoldNode, and arm the arrival poll.
	 */
	virtual void BeginPlay() override;

	/** Clears the arrival poll and, for reason == Destroyed, runs the §3.3 death bookkeeping (RemoveMinerIncome if arrived; UnregisterMinerAlive always). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  TASK-165: a miner NEVER holds a skeletal death anim (returns false, so the base HandleDeath
	 *  destroys it immediately). The §3.3 economy bookkeeping runs in EndPlay on Destroy — deferring
	 *  the destroy for a death-anim hold would keep a dead miner accruing income and holding its
	 *  cap-6 slot for the hold window. The §6 gold-burst on death is the miner's death feedback.
	 */
	virtual bool ShouldHoldDeathAnim() const override { return false; }

	/**
	 *  Standing this close to the gold node (2D) counts as arrived: income
	 *  activates exactly once and the miner stands. The walk's acceptance
	 *  radius is 0.8 × this, so the natural stop always lands inside the ring.
	 *  // GDD §3.3 — ~10 s walk, then +1 gold/s activates only on arrival
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Miner", meta = (ClampMin = "0"))
	float ArrivalRadius = 150.f;

	/**
	 *  Seconds between arrival/upkeep polls (registration retry, arrival
	 *  detection, walk healing) — the miner's replacement for the sealed
	 *  combat state timer. ~0.25 s house cadence, never per-tick (TASK-004
	 *  law); clamped >= 0.05 s at arm time (qa/TASK-021 WARN-1 guard rule).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Miner", meta = (ClampMin = "0.05"))
	float ArrivalCheckInterval = 0.25f;

private:

	/**
	 *  Poll body (every ArrivalCheckInterval): gate on dead/frozen, retry
	 *  registration if needed, then arrival detection (2D distance <=
	 *  ArrivalRadius → stand + AddMinerIncome exactly once) or walk healing
	 *  (re-MoveToActor when the move failed, finished short, or was pointed at
	 *  a different goal). Post-arrival it only walks a displaced miner back.
	 */
	void UpdateMining();

	/** (Re)issues MoveToActor toward Node unless a move toward it is already in flight — the base EnterAdvance re-path gate, plus a goal check that heals hijacked moves. */
	void EnsureWalkingToNode(AGoldNode* Node);

	/** Resolves the owning player state and calls RegisterMinerAlive exactly once (latched); safe to call repeatedly. */
	void TryRegisterWithOwnerState();

	/** Starts the §6 "clink" mining loop on arrival (TASK-179): resolves S_MinerClink null-safe onto ClinkAudio and Play()s it (idempotent). No-op if the sound is absent. */
	void StartMiningClink();

	/** Stops the mining clink loop (death/freeze). Idempotent + null-safe. */
	void StopMiningClink();

	/** Looping "clink" mining SFX (TASK-179), attached at the node height. Sound soft-resolved at arrival (S_MinerClink); the loop flag is authored on the asset (TASK-180). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Miner", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAudioComponent> ClinkAudio;

	/**
	 *  The owning team's ASiegePlayerState, resolved through
	 *  ASiegeGameState::GetPlayerStateForTeam(Team) (TASK-043 multi-team economy)
	 *  — no longer "the first player state" (M2 assumed one). A Blue miner binds
	 *  the player's Blue economy; a Red bot miner binds the bot's Red economy, so
	 *  each miner raises only its own side's rate. Nullptr when the game state or
	 *  the team's player state is not resolvable yet (the arrival poll retries);
	 *  in a single-Blue-PS world this returns the same Blue state M2 resolved.
	 */
	ASiegePlayerState* ResolveOwningPlayerState();

	/** Nearest AGoldNode whose GetTeam() matches ours (actor iteration per spec), or nullptr. */
	AGoldNode* FindNearestSameTeamGoldNode() const;

	/** Player state this miner registered with — death bookkeeping goes to the SAME state (weak: the world owns its lifetime). */
	TWeakObjectPtr<ASiegePlayerState> CachedOwnerState;

	/** The same-team gold node this miner walks to/stands at (weak: never retained; found once at BeginPlay). */
	TWeakObjectPtr<AGoldNode> TargetGoldNode;

	/** True once RegisterMinerAlive ran (TASK-024: exactly once per miner) — Unregister fires at death iff this. */
	bool bRegisteredAlive = false;

	/** True once this miner reached the node (arrival is a one-way latch — displacement never re-triggers arrival). */
	bool bArrivedAtNode = false;

	/** True once AddMinerIncome ran (arrival while registered) — RemoveMinerIncome fires at death iff this, per the §3.3 killed-en-route rule. */
	bool bIncomeActive = false;

	/** One-shot guard: the ASiegeGameState is not available yet at resolve time (early-spawn edge). The "no player state for this team" case is logged by GetPlayerStateForTeam (TASK-043), not here. */
	bool bWarnedNoOwnerState = false;

	/** One-shot guard (TASK-044, closes qa/TASK-043 WARN): with a LIVE GameState, the team's ASiegePlayerState was not found — a genuinely mis-teamed miner. Latched so the 0.25 s upkeep poll stops re-querying GetPlayerStateForTeam (whose not-found path logs unconditionally), turning ~4 Warning lines/sec into exactly one. Distinct from bWarnedNoOwnerState (the no-GameState-yet retry, which is left untouched). */
	bool bWarnedNoTeamPlayerState = false;

	/** One-shot guard: no AAIController possessing the miner (poll keeps retrying — possession can land a tick after spawn). Named distinctly from the base's private bWarnedNoAIController — no shadowing. */
	bool bWarnedNoWalkController = false;

	/** One-shot guard: the gold node found at BeginPlay disappeared mid-match. */
	bool bWarnedNodeLost = false;

	/** One-shot guard: arrived with no registered owner state — mining activates no income. */
	bool bWarnedIncomeSkipped = false;

	/** One-shot guard: MoveToActor toward the node reported Failed (navmesh coverage tier — the poll keeps retrying). */
	bool bWarnedMoveFailed = false;

	/** Drives UpdateMining every ArrivalCheckInterval seconds (cleared by FreezeAI and EndPlay). */
	FTimerHandle MiningPollTimerHandle;
};
