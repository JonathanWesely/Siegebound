// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Siegebound/SummonedUnit.h"
#include "MinerUnit.generated.h"

class AGoldNode;
class ASiegePlayerState;
class UAudioComponent;

/**
 *  What the command layer tells one poll of AMinerUnit::UpdateMining to do
 *  (TASK-397 seam, §5's table wired onto it by TASK-398; law: CONVENTIONS
 *  "FOLLOW command … (2026-08-02)" §5/§6).
 *  Deliberately PLAIN C++ — not a UENUM: this is an internal decision token
 *  between two private members of one class, it is never serialized, never
 *  edited, never seen by Blueprint, and adding a reflected type would put a
 *  DT_Cards-adjacent enum on the schema for nothing.
 *
 *  THE FIVE COMMANDS COLLAPSE ONTO FOUR MODES (TASK-398), which is the whole
 *  reason Jonathan's table is small enough to be safe:
 *    Attack (T) / no command yet -> Mine
 *    Hold (R) and Ambush (F)     -> MineInDisc   (identical for miners — his words)
 *    Defend (E) and Follow (C)   -> GoToPoint    (different POINT, same body)
 *    hero dead / castle gone     -> Stand
 */
enum class EMinerOrderMode : uint8
{
	/**
	 *  THE LEGACY ORDER — today's shipped loop, unchanged: seek the best mine
	 *  ANYWHERE (AGoldNode::FindBestMineFor), walk, register at the ring, earn.
	 *  This is ALSO §5's Attack (T) semantics ("find the nearest mine and start
	 *  mining") and, per the flagged MINER SPAWN-DEFAULT ruling, the miner's
	 *  behavior before the player has commanded anything. That three-way identity
	 *  is deliberate: it is the cheapest regression proof in the batch — the T
	 *  branch and the un-commanded branch return the SAME mode, so they run the
	 *  SAME code, so they cannot drift.
	 */
	Mine,

	/**
	 *  Hold (R) and Ambush (F), which COLLAPSE for a miner (Jonathan: Ambush "is
	 *  the same thing as 'hold'"): the group's POSITION circle only, with the
	 *  ATTACKING PORTION SKIPPED ENTIRELY — no tier-1 attack-zone acquisition and
	 *  no tier-2 position-zone acquisition, because a miner has no target ladder
	 *  at all. The two-rung ladder that replaces it:
	 *    1. a mine INSIDE the circle (AGoldNode::FindBestMineInDisc over
	 *       Point/Radius) — walk to it and mine it exactly as under Mine;
	 *    2. else Station — stand at this miner's own slot inside the circle.
	 *  A mine outside the circle is not a legal destination and is dropped by the
	 *  retarget gate exactly like a depleted one.
	 */
	MineInDisc,

	/**
	 *  Defend (E: the own castle's interior anchor,
	 *  ACastle::GetInteriorAnchorLocation on ACastle::FindNearestCastleForTeam) and
	 *  Follow (C: the hero's live station, anti-repath-banded because that anchor
	 *  MOVES). Walk to Point and stand there. No mining — the tenure is ENDED on
	 *  the way out — and no attacking.
	 */
	GoToPoint,

	/**
	 *  STAND WHERE YOU ARE (TASK-398 addition to TASK-397's three). The two ruled
	 *  cases that have no destination at all:
	 *   - Follow with a DEAD or unresolvable hero — CONVENTIONS §4's hero-death
	 *     ruling is HOLD POSITION, resuming the instant a live pawn resolves (the
	 *     anchor is re-read every poll and never cached, so a post-respawn
	 *     REPLACEMENT pawn works for free);
	 *   - Defend with no standing own castle — §5's "own castle destroyed ⇒ idle
	 *     in place".
	 *  Never a fallback for "I could not read my orders" — that answers Mine (the
	 *  DEGRADE RULE): a miner that cannot hear its orders must keep EARNING.
	 */
	Stand,
};

/**
 *  One poll's destination decision, produced by AMinerUnit::ResolveMinerOrder
 *  and consumed by AMinerUnit::UpdateMining (TASK-397 seam). By-value POD —
 *  no allocation, no caching, re-derived every poll, exactly like the base's
 *  live FindUnitGroup read (a group pointer aliases into a mutating array and
 *  must never outlive the call stack, so the seam copies out what it needs).
 */
struct FMinerOrder
{
	/** Which body runs this poll. */
	EMinerOrderMode Mode = EMinerOrderMode::Mine;

	/** GoToPoint: the destination. MineInDisc: the position circle's CENTRE. Unused for Mine/Stand. */
	FVector Point = FVector::ZeroVector;

	/** MineInDisc: the position circle's radius — the mine-legality disc. Unused otherwise. */
	float Radius = 0.f;

	/** MineInDisc: this miner's own slot inside the circle — where it stands when no mine lies in the circle (ladder rung 2). Unused otherwise. */
	FVector Station = FVector::ZeroVector;

	/**
	 *  GoToPoint: the ANTI-REPATH BAND (CONVENTIONS §4, manager ruling 10) — the
	 *  walk is re-issued only once the recomputed destination has drifted further
	 *  than this from the goal last issued. Follow fills it from
	 *  ASiegePlayerController::GetFollowRepathTolerance() because the hero MOVES;
	 *  Defend leaves it 0 because the castle does not, and at 0 a static goal still
	 *  never re-paths while a move is in flight (drift 0 <= 0).
	 */
	float RepathTolerance = 0.f;
};

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
 *   4. CanEverAttack() -> false (TASK-397, the ASorcererUnit idiom): the three
 *      SHIPPED guard points in ASummonedUnit.cpp — EnterAttack() stands the
 *      unit DOWN to Idle (SummonedUnit.cpp:2050), UpdateStateGrouped() acquires
 *      NOTHING (:1583), PerformAttack() refuses (:2307) — now cover this class
 *      permanently and BY CLASS IDENTITY, not by data. Seals 1-3 keep the
 *      decision loop from ever reaching them; seal 4 is what makes reaching
 *      them harmless. Belt AND braces: TASK-397 chose to KEEP 1-3 (see the
 *      COMMAND SEAM block below) rather than trade them for 4.
 *  No looping timer is ever started from row Cadence on this class; the
 *  miner's own poll runs from ArrivalCheckInterval, clamped >= 0.05 s at arm
 *  time (the WARN-1 guard rule).
 *
 *  COMMAND SEAM (TASK-397; law: CONVENTIONS "FOLLOW command + the
 *  DEFAULT-STANCE law + the MINER command rework (2026-08-02)" §5/§6/§7).
 *  Jonathan's rework gives the miner all five commands with miner-specific
 *  bodies (§5's table). Commands need a DECISION LOOP, and the miner already
 *  has one: UpdateMining, running at the SAME 0.25 s cadence the base state
 *  timer would. TASK-397 therefore took CONVENTIONS §6 approach (B) — KEEP THE
 *  STRUCTURAL SEAL AND READ COMMANDS IN THE MINER'S OWN POLL:
 *   - ResolveMinerOrder() is the ONE place this class reads the owning-team
 *     controller's live command state (group via FindUnitGroup, stance via
 *     HasIssuedCommand/GetCurrentCommand) and answers "where am I going this
 *     poll" as an FMinerOrder.
 *   - Approach (A) — restoring StateCheckInterval and driving the miner from
 *     the base state machine — was REJECTED on measured evidence: (i) the base
 *     dispatch lets Profile None fall through to the LEGACY Standard body
 *     (SummonedUnit.cpp:1197-1211 routes only Siege and Support away), whose
 *     no-target path is Goal = FindNearestEnemyCastle() + EnterAdvance — an
 *     unsealed miner MARCHES ON THE ENEMY CASTLE, and AggroRadius 0 does not
 *     stop it (it only kills acquisition); (ii) every base body and mover the
 *     miner would reuse (UpdateStateGrouped, EnterAdvanceToLocation, EnterIdle,
 *     GetAIController, State, CommandGroupId, GroupStationOffset) is PRIVATE on
 *     ASummonedUnit, so "maximum reuse" would cost a promote-to-protected edit
 *     to a file this batch assigns exclusively to TASK-396; (iii) it would put
 *     TWO 0.25 s drivers on ONE UPathFollowingComponent — the base's
 *     EnterAdvance re-path gate (CurrentMoveGoal != Goal) and this class's
 *     EnsureWalkingToNode gate (GetMoveGoal() == Node) each read the other's
 *     request as a hijack and re-issue it, which is exactly the re-path mill
 *     behind TASK-280/TASK-282.
 *   (B) costs a ~15-line duplicate of the base's owning-team controller +
 *   group resolve, and nothing else. It needs ZERO lines of ASummonedUnit.
 *  TASK-397 shipped the seam INERT (every path returned Mine). TASK-398 wires
 *  §5's table onto it — see the COMMAND SEMANTICS block below.
 *
 *  COMMAND SEMANTICS (TASK-398 — Jonathan's rework, verbatim: "they now do
 *  receive all the commands, but they function differently for the miners").
 *  ResolveMinerOrder answers ONE FMinerOrder per poll; UpdateMining runs the
 *  matching body. The whole five-command table, as built:
 *
 *   | Command       | Order            | Body                                    |
 *   |---------------|------------------|-----------------------------------------|
 *   | Attack (T)    | Mine             | today's loop, byte-identical            |
 *   | none yet      | Mine             | ditto (the §5 spawn default)            |
 *   | Hold (R)      | MineInDisc       | mine in the circle, else station in it  |
 *   | Ambush (F)    | MineInDisc       | IDENTICAL to Hold — implemented ONCE    |
 *   | Defend (E)    | GoToPoint        | the own castle's interior anchor        |
 *   | Follow (C)    | GoToPoint        | the hero's live station, repath-banded  |
 *   | hero dead     | Stand            | HOLD POSITION (CONVENTIONS §4 ruling)   |
 *   | castle gone   | Stand            | idle in place (§5)                      |
 *
 *  THREE INVARIANTS HOLD ACROSS ALL OF THEM, and they are what make the rework
 *  safe on the project's most economically load-bearing unit:
 *   - THE BOOKKEEPING IS NEVER RE-IMPLEMENTED. Every mining branch reaches the
 *     SAME TryRegisterArrivedMiner -> AddMinerIncome -> per-tenure-latch block;
 *     the command layer only chooses WHERE the miner is steered and WHETHER a
 *     given mine is a legal destination. Leaving a mine for the castle/hero runs
 *     LeaveMining() — mine-side release FIRST, then the income side, the
 *     EndPlay(Destroyed) ordering — so a miner can never earn from inside its
 *     own castle, and can never pin a mine's exclusive claim while standing
 *     somewhere else.
 *   - ONE DRIVER ON THE MOVEMENT COMPONENT, ALWAYS. Exactly one of
 *     EnsureWalkingToNode / DriveToPoint / StandInPlace runs per poll. This is
 *     approach (B)'s core promise and the reason the TASK-280/282 re-path mill
 *     cannot reappear here.
 *   - THE MINER STILL CANNOT ATTACK UNDER ANY OF THE FIVE. No body below calls
 *     an acquisition or attack surface (they are all private on the base and
 *     unreachable from this file anyway), seals #1-#3 keep the base state
 *     machine unarmed, and seal #4 covers the three shipped guard points.
 *
 *  🚩 THE SPAWN DEFAULT IS THE FLAGGED RULING, AND ITS LEVER IS bFollowOnSpawn
 *  (see that property — it is the ONE line Jonathan flips). Shipped default:
 *  a miner spawns MINING.
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

	//~ ─── COMMAND SURFACE (TASK-397; signatures PINNED character-for-character by
	//~     CONVENTIONS "FOLLOW command … (2026-08-02)" §7, access level included) ───
	//~
	//~ ⚠️ These are `public:` because the BASE declares them public: Profile is private
	//~ on ASummonedUnit, so these virtuals are the sanctioned outside-callable surface
	//~ (the same reason IsGroupCommandEligible() and CanEverAttack() already are), and
	//~ narrowing access in an override would break the callers.
	//~
	//~ ⚠️ BUILD-ORDER DEPENDENCY: CanFollowHero/CanTakeZoneOrders are declared on
	//~ ASummonedUnit by TASK-396. Until that lands, these two overrides do not compile
	//~ (C3668) — the designed state for this batch, which compiles as ONE UBT module
	//~ against §7's pinned registry (the ANCIENT-GROUNDS precedent). CanEverAttack is
	//~ already shipped (TASK-360) and carries no such dependency.

	/**
	 *  THE ATTACK SEAL, seal #4 of the class doc's four (TASK-397; MANDATORY per
	 *  CONVENTIONS §6 — it was never part of the approach decision). A miner can never
	 *  attack, now BY CLASS IDENTITY as well as structurally: the three shipped guard
	 *  points in ASummonedUnit.cpp (EnterAttack :2050, UpdateStateGrouped :1583,
	 *  PerformAttack :2307) all honour it, so every future command body — none of which
	 *  exists yet — is sealed in advance.
	 *
	 *  ⚠️ Why it matters even though seals 1-3 already make Attack unreachable: the
	 *  Miner row carries Cadence 0 and LoadStatsAndStart clamps to MinAttackCadence
	 *  0.05 s, so a miner that EVER reached Attack would swing 20×/s. Seal 4 is the
	 *  layer that survives someone later re-legalizing the state timer.
	 *
	 *  ZERO BEHAVIOR CHANGE: the only OTHER reader of this predicate,
	 *  CanReceiveDamageBoost(), already returned false for a miner via row Damage 0
	 *  (SummonedUnit.cpp:779-796 names the miner explicitly), and the three guard
	 *  points are unreachable on this class today.
	 */
	virtual bool CanEverAttack() const override { return false; }

	/**
	 *  FOLLOW eligibility (TASK-397; CONVENTIONS §3's table — Jonathan widened Follow
	 *  to the Miner). Base is Profile == Standard || Support, which the miner's
	 *  Profile None would fail. Feeds IsFollowCommandEligible() (the C-key select
	 *  sweep and the §2 spawn auto-enroll). TRUE for a miner in every state the
	 *  PLAYER can ever observe — pressing C over a miner enrolls it and it follows,
	 *  exactly as Jonathan specified.
	 *
	 *  ⚠️ THE ONE EXCEPTION IS A SINGLE SYNCHRONOUS CALL STACK WIDE, AND IT IS HOW
	 *  TASK-398 DELIVERS THE §5 MINER SPAWN-DEFAULT RULING (manager ruling 7 — the
	 *  biggest silent consequence in the batch). Read literally, "Follow is the spawn
	 *  default for every commandable unit" + "the miner is now commandable" means
	 *  every 24-gold miner walks to the hero and earns ZERO gold until personally
	 *  micro'd. The ruling is that a miner SPAWNS MINING. So during — and ONLY
	 *  during — ASummonedUnit::BeginPlay's auto-enroll (CONVENTIONS §2), this
	 *  answers bFollowOnSpawn, which is false by default: the enroll's
	 *  IsFollowCommandEligible() gate refuses, and the miner is never added to the
	 *  default follow group. AMinerUnit::BeginPlay closes the window on the very next
	 *  statement after Super::BeginPlay() returns, so nothing outside that stack can
	 *  observe false.
	 *
	 *  ⚠️ WHY REFUSING THE ENROLL, RATHER THAN THE OBVIOUS ClearCommandGroup() AFTER
	 *  IT (the lever TASK-397's handoff flagged) — this is the load-bearing detail:
	 *  ClearCommandGroup() clears the UNIT's id but CANNOT remove the unit from the
	 *  GROUP's Members array (that array lives on the controller, and the batch's
	 *  file-ownership law forbids reaching into it). A miner left in Members with a
	 *  cleared id hits ASiegePlayerController::EnrollInDefaultFollowGroup's
	 *  `if (Members.Contains(Unit)) return;` idempotence early-out FOREVER — so
	 *  pressing C over that miner would silently do nothing and FOLLOW WOULD BE
	 *  UNREACHABLE FOR MINERS, breaking the one miner behavior Jonathan spelled out
	 *  in full. Refusing eligibility for the duration of the enroll leaves the
	 *  controller's state perfectly consistent instead: the miner is in no group, in
	 *  no Members array, and a later C press enrolls it fresh with a real station.
	 *
	 *  Signature unchanged and still §7-pinned character-for-character; only the
	 *  returned VALUE is now data-driven, which is the semantic this task owns.
	 */
	virtual bool CanFollowHero() const override { return bFollowOnSpawn || bSpawnFollowEnrollWindowClosed; }

	/**
	 *  ZONE-ORDER eligibility (TASK-397; CONVENTIONS §3's table — Jonathan gave the
	 *  miner all five commands). Base is Profile == Standard. Feeds the SHIPPED
	 *  IsGroupCommandEligible(), i.e. the R/F stage-1 select sweep.
	 *
	 *  ⚠️ UNLIKE CanFollowHero() ABOVE, THIS IS UNCONDITIONALLY TRUE — and the
	 *  asymmetry is the §5 ruling, not an oversight. Follow is a SPAWN DEFAULT (it is
	 *  pushed onto every eligible unit at BeginPlay, which is what would silently
	 *  delete the miner economy), whereas a zone order is only ever DELIBERATE: R/F
	 *  reach a miner solely through the stage-1 select sweep, i.e. because the player
	 *  drew a circle around it. There is nothing to opt out of at spawn.
	 *
	 *  Live consequence, stated so QA can check it: a Blue miner inside an R/F select
	 *  circle is COUNTED, assigned a group, and — per §5's table (TASK-398) — goes to
	 *  the position circle and mines a mine inside it if there is one. It can still
	 *  never attack: this class's state timer is sealed (seals #1-#3) so
	 *  UpdateStateGrouped never runs for it, ResolveMinerOrder deliberately never
	 *  reads AttackCenter/AttackRadius, and CanEverAttack() is false.
	 */
	virtual bool CanTakeZoneOrders() const override { return true; }

	//~ ─── End command surface ───

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

	/**
	 *  🚩🚩 THE MINER SPAWN-DEFAULT LEVER — **THIS IS THE ONE LINE JONATHAN FLIPS**
	 *  (TASK-398; CONVENTIONS §5's flagged MINER SPAWN-DEFAULT ruling, manager
	 *  ruling 7, and item 1 on his playtest gate TASK-402).
	 *
	 *   false  = SHIPPED DEFAULT — **a miner SPAWNS MINING.** It is not
	 *            follow-eligible for the duration of ASummonedUnit::BeginPlay's
	 *            auto-enroll (see CanFollowHero()), so it never joins the default
	 *            follow group and walks straight to the best mine, earning +1 gold/s
	 *            on arrival exactly as it does today. Follow still applies to it the
	 *            moment the player explicitly circles it with **C**.
	 *   true   = THE LITERAL READING of "Follow is the spawn default for every
	 *            commandable unit": every miner played walks to the hero and earns
	 *            **zero gold** until personally ordered to mine (T, or R/F over a
	 *            circle containing a mine).
	 *
	 *  Flip it either by changing the `= false` initializer here (one line, needs a
	 *  compile) or — no compile at all — by setting it on `BP_Unit_Miner`, which is
	 *  why this is EditDefaultsOnly rather than a bare C++ constant.
	 *
	 *  ⚠️ It is a SPAWN-TIME switch and is read exactly once per miner, inside
	 *  Super::BeginPlay. Toggling it mid-match changes nothing for miners already on
	 *  the field — deliberately: their orders are the player's, not this flag's.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Miner")
	bool bFollowOnSpawn = false;

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
	 *
	 *  TASK-397: opens with the ONE command-seam read (ResolveMinerOrder).
	 *  TASK-398: dispatches on the answer — Stand and GoToPoint leave mining and
	 *  return; Mine and MineInDisc BOTH fall into the shared mining body below,
	 *  differing only in WHICH finder the retarget gate consults and in what
	 *  happens when that finder comes back empty. Everything from the arrival test
	 *  down is one shared block, which is what makes income identical in both.
	 */
	void UpdateMining();

	/**
	 *  THE COMMAND SEAM (TASK-397 — the whole deliverable of that task; law:
	 *  CONVENTIONS "FOLLOW command … (2026-08-02)" §5/§6). The ONE place this
	 *  class reads the owning-team player controller's LIVE command state and
	 *  turns it into this poll's destination decision:
	 *   - the per-unit GROUP order first (GetCommandGroupId → FindUnitGroup),
	 *     mirroring the base's precedence — ASummonedUnit::UpdateState
	 *     dispatches groups ABOVE the team stance gate;
	 *   - then the team-wide STANCE (HasIssuedCommand / GetCurrentCommand).
	 *  The controller is resolved through ASiegePlayerController::
	 *  FindControllerForTeam(World, Team) — GetFirstPlayerController() is BANNED
	 *  (M8 TEAM LAW). Nothing is cached: the group pointer aliases into a
	 *  mutating array and the stance is live, so both are re-read every poll.
	 *
	 *  Non-const because a DEAD group id SELF-HEALS here (ClearCommandGroup),
	 *  exactly as ASummonedUnit::UpdateState does at its group dispatch.
	 *
	 *  TASK-398 fills in §5's table here (see the class doc's COMMAND SEMANTICS
	 *  block for the full five-row mapping) and changed RETURNS ONLY — the branch
	 *  structure TASK-397 shaped is untouched.
	 *
	 *  DEGRADE RULE (binding, and TASK-398 keeps it at every new exit): anything
	 *  unresolvable — no world, no controller, a dead group id, an unrecognised
	 *  group type — answers Mine. A miner that cannot read its orders must fall
	 *  back to EARNING, never to stalling. Stand is reserved for the two RULED
	 *  no-destination cases (dead hero, destroyed castle), never for a read failure.
	 */
	FMinerOrder ResolveMinerOrder();

	/** (Re)issues MoveToActor toward Node unless a move toward it is already in flight — the base EnterAdvance re-path gate, plus a goal check that heals hijacked moves. */
	void EnsureWalkingToNode(AGoldNode* Node);

	/**
	 *  Walks to a plain world POINT (TASK-398) — the mover behind Defend, Follow and
	 *  the MineInDisc "no mine in the circle" rung. The point-goal sibling of
	 *  EnsureWalkingToNode, and like it the ONLY movement driver that runs on the
	 *  poll it is called from (approach (B)'s one-driver promise).
	 *
	 *  ⚠️ ANTI-REPATH IS A REQUIREMENT, NOT POLISH (CONVENTIONS §4, manager ruling
	 *  10). The follow anchor MOVES, so an unconditional per-poll re-issue is
	 *  exactly the mill that produced TASK-280 ("units freeze just past midfield")
	 *  and TASK-282 ("halt just short of the castle") — each request restarts path
	 *  following before the previous one produced motion. Law, implemented here:
	 *  while a move is IN FLIGHT, re-issue only once the new destination has drifted
	 *  more than RepathTolerance from the goal last issued. Inside ArrivalRadius the
	 *  miner simply STOPS (the shipped 150 uu idle tolerance).
	 *
	 *  bProjectDestinationToNavigation is ON: the castle interior anchor is a raw
	 *  ground point and the hero's station is an offset from a walking pawn, so
	 *  neither is guaranteed to sit exactly on the navmesh. FilterClass stays null
	 *  (an UNFILTERED query, which is what lets the OWN team's interior nav area
	 *  path normally — TASK-350's gating excludes only the enemy filter).
	 */
	void DriveToPoint(const FVector& Point, float RepathTolerance);

	/** Stops any in-flight walk and stands (TASK-398) — the Stand body: hero dead under Follow, or no standing own castle under Defend. Idempotent, null-safe, and it drops the move-goal latch so the next order re-paths from scratch. */
	void StandInPlace();

	/**
	 *  LEAVES MINING for a non-mining order (TASK-398), in the ONE order that keeps
	 *  the books straight — and it is the same order EndPlay(Destroyed) uses:
	 *   1. MINE SIDE FIRST — AGoldNode::UnregisterArrivedMiner releases this miner's
	 *      share of the exclusive claim and stops the drain when it was the last
	 *      occupant. Skipping it would PIN a mine against every other miner on the
	 *      map while this one stands in its castle.
	 *   2. THEN the income/latch side — EndMineTenure (RemoveMinerIncome iff active,
	 *      per-tenure latches cleared, clink stopped). Skipping it would leave a
	 *      miner earning +1 gold/s while hiding inside the castle.
	 *   3. THEN drop the target, so the return to mining is a FRESH tenure that the
	 *      retarget gate re-seeks — the target is nulled on the way OUT, never on
	 *      the way back in.
	 *  Idempotent and cheap: after the first call the target is null and every step
	 *  is a no-op, so calling it on every poll of a non-mining order is free.
	 */
	void LeaveMining();

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
	 *  SeekBestMine's in-circle twin (TASK-398 — Hold/Ambush): re-runs
	 *  AGoldNode::FindBestMineInDisc over the order's position circle and retargets
	 *  TargetGoldNode to the result, with the same tier-1/tier-2 semantics.
	 *
	 *  Kept SEPARATE from SeekBestMine rather than parameterising it, deliberately:
	 *  SeekBestMine stays byte-identical for the Attack/spawn-default path (the
	 *  cheapest regression proof in the batch — CONVENTIONS §5), its BeginPlay
	 *  caller is untouched, and the two nulls MEAN DIFFERENT THINGS. SeekBestMine's
	 *  null is the all-depleted ENDGAME (idle out the match); this one's null is the
	 *  ordinary "the circle you drew has no mine in it", after which the miner
	 *  stations inside the circle and keeps re-seeking — so each owns its own
	 *  one-shot Log latch and its own message.
	 */
	AGoldNode* SeekMineInDisc(const FMinerOrder& Order);

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

	//~ ─── TASK-398 command-semantics state (all per-unit SCALARS — the no-arrays-on-units law) ───

	/**
	 *  Closes the §5 spawn-default window (TASK-398). False for exactly as long as
	 *  Super::BeginPlay() is on the stack — which is where CONVENTIONS §2's follow
	 *  auto-enroll lives — and set true on the very next statement after it returns.
	 *  Read ONLY by CanFollowHero(); see that function for why refusing the enroll is
	 *  the correct lever and ClearCommandGroup() after the fact is not.
	 *  Not a UPROPERTY: pure runtime latch, never serialized, never designer-facing
	 *  (the designer-facing switch is bFollowOnSpawn).
	 */
	bool bSpawnFollowEnrollWindowClosed = false;

	/** Goal point of the last DriveToPoint request — the anti-repath band measures drift against THIS, never against the miner's own position (CONVENTIONS §4). Meaningful only while bHasIssuedPointGoal. */
	FVector LastIssuedPointGoal = FVector::ZeroVector;

	/** True once DriveToPoint has issued at least one move whose goal is still the standing intent. Cleared by StandInPlace and by LeaveMining's mode changes, so a new order always re-paths once. */
	bool bHasIssuedPointGoal = false;

	/** One-shot Log guard per Hold/Ambush episode: the position circle contains no minable or waitable mine, so the miner stations inside it. Re-armed the moment a mine IS found in the circle (a new circle, or an enemy claim releasing). */
	bool bLoggedNoMineInDisc = false;

	/** One-shot Log guard, once per MINER LIFETIME (never re-armed — Play Again destroys units, so a second episode is a second miner): the resolved Defend interior anchor. This is how the CONVENTIONS §5 "MEASURE, DO NOT ASSUME" duty is discharged at runtime — the world point becomes readable back from a PIE log with no editor probe, and one line per miner is the honest cost of that. */
	bool bLoggedInteriorAnchor = false;

	/** One-shot guard per Defend episode: no standing own castle to hide in (destroyed) — the miner idles in place (§5). Re-armed when a castle resolves again (Play Again). */
	bool bLoggedNoOwnCastle = false;

	/** One-shot guard: MoveToLocation toward an order point reported Failed (nav coverage — e.g. an interior anchor the navmesh does not reach). The poll keeps retrying; §5 rules a gate-stranded miner ACCEPTABLE, never a crash and never a fallback to attacking. */
	bool bWarnedPointMoveFailed = false;

	/** Drives UpdateMining every ArrivalCheckInterval seconds (cleared by FreezeAI and EndPlay). */
	FTimerHandle MiningPollTimerHandle;
};
