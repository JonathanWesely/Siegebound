# TASK-254 — MinerUnit retarget / wait / evict (handoff)

**Author:** gameplay-programmer · **Date:** 2026-07-22 · **Branch:** `m7.6-arena10x` (MinerUnit.{h,cpp} branch-owned per the M7.6 ownership extension)
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h`, `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp` — nothing else.
**NOT compiled** (TASK-258 batch compile, by spec). Consumes the TASK-253 API exactly as pinned in handoffs/TASK-253-programmer.md.

## What changed (plan §1b, faithful)

- **`FindNearestSameTeamGoldNode` DELETED** (the last `GetTeam()` caller in this file) → `SeekBestMine()`, a thin wrapper over `AGoldNode::FindBestMineFor(World, Team, GetActorLocation())` that retargets `TargetGoldNode` and one-shot-Logs the finder-null (all-depleted) state. The M2 no-node **Error is demoted to Log** (BeginPlay routes through the same wrapper).
- **`UpdateMining` rework** (0.25 s cadence untouched):
  1. dead/frozen gate + registration retry — **byte-identical to M2**;
  2. **retarget gate**: ONE shared `bTargetDead` predicate (`!Node || IsDepleted() || GetGoldReserve() <= 0`) drives (a) a defensive tenure break (`bArrivedAtNode && bTargetDead` → `EndMineTenure()`) and (b) the re-seek (`SeekBestMine`; null → idle-in-place + return, poll retries). While **un-arrived** at an enemy-claimed target, the finder is re-consulted and the target switches **only to a minable-now (tier-1) mine** — a tier-2 result, even a nearer queue, keeps the current target (**no-churn rule**; ARRIVED miners never retarget);
  3. **at the ring** (2D ≤ ArrivalRadius, metric unchanged): `TryRegisterArrivedMiner` — **success = the M2 arrival block, code-identical** (latch, StopMovement, clink, AddMinerIncome guarded by `bRegisteredAlive && OwnerState`, warn-once fallback); **failure = WAIT MODE** (stand at the ring, one-shot Log per episode, retry every poll → auto-claim the instant `UnregisterArrivedMiner` frees the claim);
  4. outside the ring: `EnsureWalkingToNode` heal — **untouched** (goal-check heals a retarget's stale in-flight move for free).
- **NEW `public: void NotifyMineDepleted(AGoldNode* DepletedMine);`** — the pinned TASK-253 contract, declared character-for-character. Body: `EndMineTenure()` (RemoveMinerIncome **iff** `bIncomeActive`, clear both per-tenure latches, clink off) + unconditional `TargetGoldNode = nullptr` + one Log line. **Never calls back into the mine** (Deplete() already emptied the registry — re-entrant Unregister would be a no-op; we skip it entirely).
- **`EndPlay(Destroyed)`**: `UnregisterArrivedMiner(TargetGoldNode)` runs **FIRST**, before the existing player-state bookkeeping (which is byte-identical: RemoveMinerIncome-iff-active, then UnregisterMinerAlive-always). World-teardown reasons still skip everything, as before.
- **Header docs rewritten to per-tenure semantics**: `bArrivedAtNode` (the one-way-per-lifetime latch is now PER-TENURE), `bIncomeActive`, class-doc economy contract + movement loop, `HasArrivedAtNode`, `ArrivalRadius`. `bWarnedNodeLost` replaced by `bLoggedNoMineAvailable` + `bLoggedWaitingAtMine`.
- **`FreezeAI` untouched.** Constructor seals untouched. `TryRegisterWithOwnerState`/`ResolveOwningPlayerState`/clink helpers untouched. Post-match drain quirk accepted + commented (in `NotifyMineDepleted`).

## Invariant table (SELF-AUDIT — QA: this is the review spine)

States: **R** = bRegisteredAlive (lifetime) · **A** = bArrivedAtNode (tenure) · **I** = bIncomeActive (tenure) · **M** = membership in a mine's ArrivedMiners registry.

| # | Transition | Where | Guarantees at every intermediate step |
|---|---|---|---|
| 1 | R: F→T | TryRegisterWithOwnerState (BeginPlay + poll retry) | latch set BEFORE RegisterMinerAlive; once per lifetime (unchanged from M2) |
| 2 | M: out→in | UpdateMining at-ring, `TryRegisterArrivedMiner` success only | only reachable when !A; mine-side atomic claim on 0→1 |
| 3 | A: F→T | same success block, immediately after #2 | **A ⇒ M** (set in the same block; arrived miners never retarget, so M is always the CURRENT target) |
| 4 | I: F→T | same block, iff `R && CachedOwnerState` | **I ⇒ A ∧ R**; AddMinerIncome exactly once per tenure (gated by !A entry + A latched first) |
| 5 | Eviction: A,I→F (M already out) | NotifyMineDepleted → EndMineTenure | Deplete() emptied the registry BEFORE calling us — no mine call-back, no double-unregister; RemoveMinerIncome iff I, I cleared in the same block → **death after evict can never double-Remove** |
| 6 | Death: M→out, then I→F, then R→F | EndPlay(Destroyed) | mine unregister FIRST (idempotent no-op for en-route/waiting/evicted miners) so the mine never drains a miner whose income is being removed; then the M2-verbatim order Remove-before-Unregister → **income ⊆ alive holds at every step** |
| 7 | Defensive stale: A,I→F | UpdateMining tenure break (bTargetDead) | unreachable in designed flows (ClearScatter only after the PlayAgain miner sweep; Deplete() is synchronous) — kept so A/I can never outlive their mine; the SAME predicate gates the re-seek, so a re-seek can never run with tenure flags still set |
| 8 | Teardown (non-Destroyed EndPlay) | EndPlay | no bookkeeping (counts die with the state; the mine tears down its own registry) — unchanged |
| 9 | Freeze | FreezeAI (untouched) | no state transitions; poll + clink killed. Eviction CAN land on a frozen miner (post-match drain quirk): books stay balanced (#5), no movement follows (poll dead) |

Cross-checks: no path sets I without A; no path clears A without clearing I (EndMineTenure clears both; EndPlay clears I mid-destruction); every RemoveMinerIncome site tests-and-clears I in one block (sites: EndMineTenure, EndPlay — exactly two); every AddMinerIncome site is the single arrival block. Mine registry ⊆ live miners: #6 pre-destroy unregister + the mine's own stale-weak compaction as backstop.

## Deviations from spec (each justified — QA rulings welcome)

1. **Arrival block "VERBATIM": code lines are identical; two COMMENT lines updated** ("one-way latch … once per miner" → per-tenure; clink "stops on death/freeze" → "+eviction"). Leaving them would have made the comments factually wrong under the new semantics. Zero behavior delta — diff the block to confirm.
2. **Two Log lines added beyond spec**: wait-mode entry (one-shot per episode, `bLoggedWaitingAtMine`) and per-miner eviction. Both are PIE visibility for the T-F occupancy/depletion suites; the eviction line also consumes the `DepletedMine` param (no unused-parameter surface).
3. **`bTargetDead` includes `GetGoldReserve() <= 0`** alongside `IsDepleted()` — mirrors FindBestMineFor's own skip predicate; guards the `DrainPerMinerPerSecond = 0` tuning edge where reserve could in principle sit at 0 unlatched.
4. **No-churn interpretation pinned**: the retarget-while-un-arrived rule upgrades CATEGORY only (wait-target → minable-now). A nearer tier-1 while already walking to a tier-1 does NOT retarget — a moving miner's "nearest" oscillates, which is exactly the churn the rule forbids; the finder's strict-< handles exact ties.
5. **`#include "EngineUtils.h"` removed** — the deleted finder was this file's only TActorIterator user.
6. **EndPlay unregisters via `TargetGoldNode` unconditionally** (not gated on A) — idempotent by TASK-253 contract; also lets a dying waiter trigger the mine's stale-sweep for free.

## QA pointers

- Bookkeeping-invariants review mode (board spec): the table above enumerates every path in/out of R/A/I/M — verify each against the code seams (`UpdateMining` retarget gate + arrival/wait blocks, `EndMineTenure`, `NotifyMineDepleted`, `EndPlay`).
- Frozen-miner eviction removes income POST-match (HUD rate may tick down after the freeze) — accepted-quirk territory (plan §1b), reset by Play-Again's ResetEconomy; flagging so it isn't read as a bug.
- `Team` (protected ASummonedUnit member) is used for `CanTeamMine`/`FindBestMineFor` — same member M2 used for the state resolve; no shadowing introduced; no new includes needed (GoldNode.h already included).
- Known cross-file state: GoldNode.cpp's `Deplete()` call site now links (this task declared the pinned signature). Tree still does not compile until TASK-255/256 land (BattlefieldScatter.cpp:656 + SiegeBotController.cpp:1017 still reference the removed `Team` API) — expected, TASK-258 compiles the batch.
