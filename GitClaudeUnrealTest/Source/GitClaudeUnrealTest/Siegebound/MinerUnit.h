// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Siegebound/SummonedUnit.h"
#include "MinerUnit.generated.h"

class AGoldNode;
class ASiegePlayerState;
class UAudioComponent;

/**
 *  Siegebound miner — the §3.3 economy unit (card row Miner, TASK-025;
 *  retarget/wait/evict rework for the W1-PREP mirrored depleting mines,
 *  TASK-254 — law: CONVENTIONS "Mirrored depleting mines").
 *
 *  ASummonedUnit subclass that NEVER fights: it walks to the best NEUTRAL
 *  mine (AGoldNode::FindBestMineFor — THE single finder since TASK-253; the
 *  old same-team filter is gone, exclusive occupancy replaces it), registers
 *  at the ring (TryRegisterArrivedMiner, the atomic claim), stands there, and
 *  activates +1 gold/s on the owning player state only once REGISTERED.
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
 *  Economy bookkeeping (TASK-024 / handoffs/TASK-024.md contract, amended to
 *  PER-TENURE semantics by TASK-254 — a tenure = one registered stay at one
 *  mine, ended by eviction or death):
 *   - RegisterMinerAlive()   — BeginPlay, exactly once per LIFETIME (retried
 *                              by the poll if the state was not resolvable).
 *   - AddMinerIncome()       — exactly once per TENURE, on registered arrival
 *                              (never on spawn); latched by bArrivedAtNode/
 *                              bIncomeActive, both cleared by eviction so the
 *                              next mine's arrival re-adds income.
 *   - RemoveMinerIncome()    — eviction (NotifyMineDepleted) OR death, iff
 *                              income was active; whichever fires first clears
 *                              bIncomeActive, so the pair can never double-
 *                              Remove. Always called before Unregister so
 *                              income ⊆ alive holds at every step.
 *   - UnregisterMinerAlive() — death, ALWAYS (arrived or not).
 *  Mine-side bookkeeping (TASK-253 registry): TryRegisterArrivedMiner at the
 *  ring; UnregisterArrivedMiner at death — EndPlay(Destroyed) runs it BEFORE
 *  the player-state bookkeeping above, so the mine never drains for a miner
 *  whose income is being removed. Deplete() empties the mine's registry
 *  BEFORE NotifyMineDepleted fires, so death-after-evict never touches the
 *  mine twice.
 *  "Death" is EndPlay with reason Destroyed — the single choke point for every
 *  removal-from-play path (combat death via the base's HandleDeath → Destroy,
 *  the PlayAgain unit sweep, a KillZ fall) — never at world teardown.
 *  CanAddMiner() is NOT called here: the §3.3 cap is enforced at play time by
 *  TASK-030, before any gold moves.
 *
 *  Movement/arrival (TASK-254 retarget/wait/evict): MoveToActor toward the
 *  finder's mine (acceptance 0.8 × ArrivalRadius, the house fraction), then
 *  the ~0.25 s poll (never per-tick, TASK-004 law) drives the loop:
 *   - RETARGET GATE: a null/stale/depleted target is re-found via
 *     FindBestMineFor; while UN-arrived and pointed at an enemy-claimed mine
 *     the finder is re-consulted and the target switches ONLY to a
 *     minable-now (tier-1) mine — never between wait targets and never
 *     between equal options (the no-churn rule; the finder's strict-<
 *     tiebreak pins exact ties). An ARRIVED miner never retargets.
 *   - AT THE RING (2D distance <= ArrivalRadius): TryRegisterArrivedMiner.
 *     Success runs the arrival block (stand + clink + income, once per
 *     tenure); refusal is WAIT MODE — stand at the ring, retry every poll,
 *     auto-claim the instant the last enemy occupant leaves or dies.
 *   - OUTSIDE THE RING: heal the walk (failed/finished-short/hijacked moves
 *     re-issued; a displaced ARRIVED miner walks back — income unaffected).
 *   - FINDER NULL (every mine depleted, or none exist): idle in place and
 *     keep polling — the intended all-depleted income death (a NORMAL state:
 *     Log, demoted from the M2 Error per the plan-of-record).
 *  Eviction (NotifyMineDepleted, called by the depleting mine): un-arrive —
 *  income off, clink off, per-tenure latches cleared, target nulled; the
 *  next poll re-seeks.
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

	/** True while this miner holds a registered tenure at its mine (PER-TENURE since TASK-254 — cleared by eviction; PIE verification hook, the IsAIFrozen/IsUnitDead house pattern). Income active iff this AND an owner state was registered at arrival. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Miner")
	bool HasArrivedAtNode() const { return bArrivedAtNode; }

	/**
	 *  Eviction seam (TASK-254 — the PINNED TASK-253 contract: called by
	 *  AGoldNode::Deplete() on every still-registered miner AFTER the mine
	 *  latched bDepleted, emptied its registry and released its claim). Ends
	 *  this miner's tenure: RemoveMinerIncome iff income was active, clears
	 *  the per-tenure arrival latch, stops the clink, nulls the target — the
	 *  0.25 s poll then re-seeks (walk / wait / idle per the retarget gate).
	 *  Never calls back into the mine (its registry is already empty). Safe
	 *  on a FROZEN miner (the accepted post-match drain quirk): books still
	 *  balance, and no movement follows because FreezeAI killed the poll.
	 */
	void NotifyMineDepleted(AGoldNode* DepletedMine);

protected:

	/**
	 *  Super binds the card stats (30 HP / 350 speed from DT_Cards row Miner)
	 *  — with the state machine structurally sealed (see class doc). Then:
	 *  timer sweep (seal #3), RegisterMinerAlive on the owning team's player
	 *  state (TASK-024 contract), seek the best mine (SeekBestMine →
	 *  AGoldNode::FindBestMineFor) and start the walk, and arm the poll.
	 */
	virtual void BeginPlay() override;

	/** Clears the arrival poll and, for reason == Destroyed, unregisters from the mine FIRST (UnregisterArrivedMiner — releases claim + drain when this was the last occupant; idempotent no-op after an eviction) and THEN runs the §3.3 death bookkeeping (RemoveMinerIncome iff income active; UnregisterMinerAlive always). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  TASK-165: a miner NEVER holds a skeletal death anim (returns false, so the base HandleDeath
	 *  destroys it immediately). The §3.3 economy bookkeeping runs in EndPlay on Destroy — deferring
	 *  the destroy for a death-anim hold would keep a dead miner accruing income and holding its
	 *  cap-6 slot for the hold window. The §6 gold-burst on death is the miner's death feedback.
	 */
	virtual bool ShouldHoldDeathAnim() const override { return false; }

	/**
	 *  Standing this close to the mine (2D) counts as at-the-ring: registration
	 *  is attempted there (success = arrival + income once per tenure; refusal
	 *  = wait mode standing at this ring). The walk's acceptance radius is
	 *  0.8 × this, so the natural stop always lands inside the ring.
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
	 *  registration if needed, then the TASK-254 loop — retarget gate
	 *  (re-seek a null/stale/depleted target; tier-1 upgrade while un-arrived,
	 *  the no-churn rule), at-ring registration (TryRegisterArrivedMiner:
	 *  success = arrival block once per tenure, refusal = WAIT MODE standing
	 *  at the ring with per-poll retries), or walk healing (re-MoveToActor
	 *  when the move failed, finished short, or was pointed at a different
	 *  goal). Post-arrival it only walks a displaced miner back.
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

	/**
	 *  Re-runs THE finder (AGoldNode::FindBestMineFor, TASK-253) from the
	 *  miner's position and retargets TargetGoldNode to the result (tier-1
	 *  minable-now, else tier-2 enemy-occupied wait target). Null — every
	 *  mine depleted, or none exist — leaves the target null and logs ONCE at
	 *  Log level (the intended all-depleted endgame, demoted from the M2
	 *  Error): the caller idles and the poll keeps re-seeking.
	 */
	AGoldNode* SeekBestMine();

	/**
	 *  Ends the current mine tenure (shared by the NotifyMineDepleted eviction
	 *  seam and the defensive stale-mine un-arrive): RemoveMinerIncome iff
	 *  bIncomeActive (on the SAME cached state), clear the per-tenure
	 *  bArrivedAtNode/bIncomeActive latches, stop the clink. Idempotent; NEVER
	 *  touches the mine registry — callers own that side.
	 */
	void EndMineTenure();

	/** Player state this miner registered with — death bookkeeping goes to the SAME state (weak: the world owns its lifetime). */
	TWeakObjectPtr<ASiegePlayerState> CachedOwnerState;

	/** The mine this miner currently walks to / waits at / stands on (weak: never retained). Re-found by the poll whenever null/stale/depleted (SeekBestMine); nulled by eviction so the next poll re-seeks. */
	TWeakObjectPtr<AGoldNode> TargetGoldNode;

	/** True once RegisterMinerAlive ran (TASK-024: exactly once per miner) — Unregister fires at death iff this. */
	bool bRegisteredAlive = false;

	/**
	 *  PER-TENURE arrival latch (TASK-254 rewrite — the M2 one-way-per-
	 *  LIFETIME latch is gone): set when TryRegisterArrivedMiner accepts this
	 *  miner at the ring, cleared ONLY by eviction (NotifyMineDepleted) or the
	 *  defensive stale-mine un-arrive — so arrival (and AddMinerIncome) fires
	 *  exactly once per tenure and again at the NEXT mine. Displacement never
	 *  clears it: a displaced arrived miner walks back, income latched. While
	 *  true, this miner sits in exactly one mine's ArrivedMiners registry.
	 */
	bool bArrivedAtNode = false;

	/** True while this tenure's AddMinerIncome is outstanding (registered arrival happened) — RemoveMinerIncome fires iff this, at eviction OR death, whichever comes first (each clears it: never a double-Remove). The §3.3 killed-en-route rule holds: never true before a registered arrival. Invariant: bIncomeActive ⇒ bArrivedAtNode. */
	bool bIncomeActive = false;

	/** One-shot guard: the ASiegeGameState is not available yet at resolve time (early-spawn edge). The "no player state for this team" case is logged by GetPlayerStateForTeam (TASK-043), not here. */
	bool bWarnedNoOwnerState = false;

	/** One-shot guard (TASK-044, closes qa/TASK-043 WARN): with a LIVE GameState, the team's ASiegePlayerState was not found — a genuinely mis-teamed miner. Latched so the 0.25 s upkeep poll stops re-querying GetPlayerStateForTeam (whose not-found path logs unconditionally), turning ~4 Warning lines/sec into exactly one. Distinct from bWarnedNoOwnerState (the no-GameState-yet retry, which is left untouched). */
	bool bWarnedNoTeamPlayerState = false;

	/** One-shot guard: no AAIController possessing the miner (poll keeps retrying — possession can land a tick after spawn). Named distinctly from the base's private bWarnedNoAIController — no shadowing. */
	bool bWarnedNoWalkController = false;

	/** One-shot Log guard: the finder returned null — every mine depleted (or none exist). A NORMAL endgame state (demoted from the M2 Error per the plan-of-record): the miner idles in place and the poll keeps re-seeking. Re-armed if a mine is ever found again (defensive — depletion is one-way, so null is normally terminal). */
	bool bLoggedNoMineAvailable = false;

	/** One-shot Log guard per WAIT episode: standing at the ring of an enemy-claimed mine (TASK-254 wait mode). Re-armed by a successful registration, a retarget, or an eviction, so each new queue logs exactly once (PIE occupancy-suite visibility). */
	bool bLoggedWaitingAtMine = false;

	/** One-shot guard: arrived with no registered owner state — mining activates no income. */
	bool bWarnedIncomeSkipped = false;

	/** One-shot guard: MoveToActor toward the node reported Failed (navmesh coverage tier — the poll keeps retrying). */
	bool bWarnedMoveFailed = false;

	/** Drives UpdateMining every ArrivalCheckInterval seconds (cleared by FreezeAI and EndPlay). */
	FTimerHandle MiningPollTimerHandle;
};
