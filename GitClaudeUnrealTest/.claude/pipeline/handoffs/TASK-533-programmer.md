# TASK-533 — `AMinerUnit::HandleStuckEscalation` — the override, and the miner's watchdog call site

**Status:** ready-for-qa · **QA gate: TASK-537** · **Author:** gameplay-programmer
**Files touched (EXACTLY two):**
- `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h` (+37)
- `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp` (+225)

⛔ Not touched: `SummonedUnit.*` (532, frozen) · `SiegeStuckStatics.*` (531, frozen) · `SiegeNavAreas.*` (534) · `BattlefieldScatter.*` (535) · `Tests/` (536) · `Build.cs` · any `.umap` · any `.ini`.
⛔ No compile, no Git, no editor/MCP/PIE. **Not compiled — TASK-538 owns the only compile.**

## M8 DECLARATION (verbatim, as required)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Reason it holds structurally: this task adds **no member of any kind**. It declares one non-reflected `virtual … override` and reuses `StuckState` / `StuckTuning` / `SidestepLeaseRemaining`, which TASK-532 already declared unreflected (or, for `StuckTuning`, `EditDefaultsOnly` CDO config identical on every machine). Units are server-only in M8 P1.

---

## 1. The pin

Character-for-character against `NAV-§8`'s `MinerUnit.h` block, including the access level:

```cpp
// AMinerUnit — protected override
virtual void HandleStuckEscalation(ESiegeStuckAction Action) override;
```

Nothing else was added to the class. No new member, no new tunable, no new timer, no new log category.

## 2. ⭐ THE DELTA SOURCE — the answer to the question this task exists to get right

**I pass `ConsumeStuckDeltaSeconds()` — TASK-532's world-clock helper — and nothing else.**

```cpp
TickStuckWatchdog(ConsumeStuckDeltaSeconds());
```

- ⛔ `StateCheckInterval` is **never read** in this diff. Grep confirms it appears in my change only inside one explanatory comment block (`MinerUnit.cpp:316`), never as a value. Its seal at `MinerUnit.cpp:62` (`StateCheckInterval = 0.f`) is **byte-for-byte untouched**.
- **Why it would have failed silently, verified at the artifact:** `FSiegeStuckStatics::Evaluate` clamps the delta with `const float SafeDelta = (DeltaSeconds > 0.f) ? DeltaSeconds : 0.f;` (`SiegeStuckStatics.cpp:60`). A 0 delta accumulates nothing into `StalledSeconds`, so `DesiredLevel` stays 0, so brake 2 (`DesiredLevel <= State.EscalationLevel`) returns `None` **forever**. No crash, no assert, no log line — the watchdog would simply never fire on a miner, on exactly the unit class most likely to walk into a rock.
- ✅ **AND THE MINER HAS EXACTLY ONE CONSUMER OF THAT HELPER, which is what makes the delta whole rather than split.** Seal #1 means `ASummonedUnit::UpdateState`'s call site (`SummonedUnit.cpp:1348`) never runs on this class — its timer can never be armed, and seal #3's post-`Super::BeginPlay` `ClearAllTimersForObject(this)` sweeps anything that somehow was. The single **synchronous** `UpdateState` that `LoadStatsAndStart` makes inside `Super::BeginPlay` is the one exception and it is **inert by construction**: `ConsumeStuckDeltaSeconds`'s first call on any unit latches the timestamp and returns 0 (`SummonedUnit.cpp:2879-2883`). Its only effect is that the miner's *first* `UpdateMining` tick gets a real delta instead of an inert one.

## 3. The call site — `UpdateMining`, after the registration retry

Placed **after** the `IsUnitDead() || IsAIFrozen()` early-out (so a dead or match-end-frozen miner never reaches the ladder, mirroring the base's freeze-above-the-call ordering) and **after** the alive-registration retry (so the §3.3 lifetime bookkeeping runs under every order and can never be delayed by anything below). The board pins the call to `UpdateMining`; the statement position inside it is mine, and this is why.

⛔ **Zero new timers.** It rides the existing `MiningPollTimerHandle` poll at `ArrivalCheckInterval` (0.25 s). No `SetTimer` in this diff.

**A spell-frozen miner also cannot climb the ladder**, which is worth stating because `UpdateMining` gates on `IsAIFrozen()` (match-end) and not on the spell freeze: `ApplyFreeze` calls `StopMovement`, so `GetMoveStatus() == Idle`, so `bAdvancing` is false, so `Evaluate` re-anchors and returns `None` however long the freeze lasts.

## 4. What each rung does, in the miner's vocabulary

| rung | miner action | why this and not the base's |
|---|---|---|
| **`Sidestep`** | `DriveToPoint(FSiegeStuckStatics::ComputeSidestepGoal(Loc, StalledGoal, StuckTuning.SidestepDistance, Attempt), 0.f)` + arm the lease | ⛔ `EnterAdvanceToLocation` is the **base's** mover. `DriveToPoint` is the miner's, and it carries the **shipped** anti-repath band at `:841-856` — reused, not re-implemented. Tolerance `0.f`, exactly as the static orders pass it: it suppresses a re-issue only when the destination has not moved at all, which keeps the rung to **one** path request. |
| **`WidenAndRepath`** | drop the lease → `bHasIssuedPointGoal = false` → `StopMovement()` → `EnsureWalkingToNode(TargetGoldNode.Get())` if non-null | A genuine re-path to the **ORIGINAL** goal. The goal is not changed and `CommandGroupId` is untouched. |
| **`Abandon`** | drop the lease → `StandInPlace()` → `SeekBestMine()` **iff `!bArrivedAtNode`** | Issues **no path request of its own**, as `NAV-§3` requires. |

`StalledGoal` source, in order: the live `TargetGoldNode`'s location → else `LastIssuedPointGoal` (when `bHasIssuedPointGoal`) → else own location. The two sources are mutually exclusive by construction: `LeaveMining()` nulls `TargetGoldNode` on the way into the point orders. The degenerate third case is answered by `ComputeSidestepGoal`'s pinned stable +Y fallback, not a NaN.

`Attempt = StuckState.EscalationLevel + (GetUniqueID() % 2)` — the same idiom TASK-532 uses, for the same de-correlation reason (a clump of miners wedged on one rock must not all sidestep the same way into each other). Non-negative, so the callee's parity test is well defined.

⛔ **No rung enters Attack** (this class structurally cannot), ⛔ **no rung cancels the standing ORDER**, ⛔ **no rung leaves the miner inert** — every one of them is followed by the poll's own movement driver within ≤ 0.25 s.

**Request budget: ≤ 2 path requests per stall** (Sidestep 1, Widen 1, Abandon 0), matching `SiegeStuckStatics.cpp:107-130`'s documented bound.

## 5. ⚠️ FLAGGED — decisions I made rather than buried. QA rules on these.

### A. ⛔⛔ THE DECLARED THIRD EDIT: the sidestep lease read. **Without it the `Sidestep` rung is a GUARANTEED silent no-op.**

The dispatch said "two things to add". I added a third — two lines — and this is the finding I most want QA to press on, because the alternative was shipping a vacuous rung.

**The mechanism, traced:** the watchdog call sits at the top of the poll, and `UpdateMining` then runs **exactly one** movement driver below it (approach (B)'s one-driver promise). Every one of them cancels an in-flight sidestep **in the same call stack that just issued it**:
- `EnsureWalkingToNode(Node)` — the sidestep is a **location** move, so `PathFollow->GetMoveGoal() == Node` is false at `:920`, so the gate falls through and `MoveToActor(Node)` is re-issued;
- `DriveToPoint(Order.Point, tol)` — my sidestep set `LastIssuedPointGoal = SidestepPoint`, and the order's point is ~`SidestepDistance` (350 uu) away, so the band's drift test at `:852` fails and it re-issues toward `Order.Point`.

Net effect without the lease: one wasted path query, then the byte-identical request the feature exists to break. **A no-op.**

```cpp
if (SidestepLeaseRemaining > 0.f)
{
    return;
}
```

This is the **identical** idiom TASK-532 shipped at `SummonedUnit.cpp:1364` for the identical reason, and it **adds no state**: `SidestepLeaseRemaining` is the base's existing member, drained by `TickStuckWatchdog` on the same clock as the ladder, and cleared by the base at all five exits (`EnterIdle` `:2710`, `EnterAttack` `:2472`, `HandleDeath` `:3466`, `FreezeAI` `:697`, `ApplyFreeze` `:942`). A miner reaches `FreezeAI` and `HandleDeath` through the base, so the lease can never outlive the stall or the unit.

**What it delays, stated honestly rather than claimed away.** The alive-registration retry runs **above** the line; eviction is push-driven (`NotifyMineDepleted`) and death runs through `EndPlay`, so neither can be delayed at all. What **can** be delayed, by at most `SidestepLeaseSeconds` (2.0 s), is the **at-the-ring test** — arrival/registration and the wait-mode auto-claim retry — and only for a miner that is by construction wedged and being steered laterally away from that ring. ⛔ No latch, no rate, no cap and no tenure rule changes; only *when* that miner's next arrival poll happens. **If QA judges this out of scope, deleting the two lines is self-contained and breaks nothing — it just makes `Sidestep` vacuous.**

### B. `WidenAndRepath` needs `StopMovement()`, and the dispatch's stated mechanism does not reach the gate it names.

The dispatch said clearing `bHasIssuedPointGoal` "is what *forces* the re-issue — the goal check at `:917-924` otherwise suppresses it." **That conflates two different gates**, and I implemented against the code:
- `:917-924` is `EnsureWalkingToNode`'s gate: `PathFollow && GetMoveStatus() != Idle && GetMoveGoal() == Node`. It **does not read `bHasIssuedPointGoal` at all** (that flag is read only by `DriveToPoint`'s band at `:851`), so clearing the latch has **zero** effect on it.
- A wedged miner's move toward its node **is** in flight — that is what makes `bAdvancing` true, which is what let the ladder climb to rung 2 in the first place. ⇒ Left alone, this gate returns early and the mining half of the rung does nothing.

So the rung breaks **both** latches: `bHasIssuedPointGoal = false` (which genuinely is the whole re-path for the Defend/Follow/station orders — `UpdateMining` calls `DriveToPoint` again later in the same poll and the band no longer suppresses it), **and** `StopMovement()` to open the node gate. ⛔ `StopMovement` is not a second driver — it is the one driver cancelling its own request, and it is what guarantees the re-issue is **one** request rather than two concurrent ones. It is the same call this file already makes in `StandInPlace` `:798`, `DriveToPoint` `:836` and the arrival block `:474`.

`EnsureWalkingToNode` is null-guarded: under Defend/Follow `LeaveMining()` has already nulled the target, and a null goal would give `MoveToActor` a `Failed` result plus the misleading "is the NavMeshBoundsVolume covering L_Arena" warning, latched for the match.

### C. `Abandon` is guarded on `!bArrivedAtNode` — this is what keeps the rung out of the income path.

`SeekBestMine()` overwrites `TargetGoldNode`. A **null** finder result (every mine depleted) would make the next poll read `bArrivedAtNode && bTargetDead` → `LeaveMining()` → **`RemoveMinerIncome`**. That rung would then be *ending a tenure and changing the gold rate*, which is squarely outside this task. The guard forbids it outright.

⭐ **And it costs nothing, because the case is all but unreachable:** arrival calls `StopMovement` (`:474`), so an **arrived** miner reads `GetMoveStatus() == Idle` → `bAdvancing` false → `Evaluate` re-anchors and returns `None` however long it stands there. **An arrived, undisplaced miner can never reach any rung.** The one path that can reach a rung holding a tenure is a *displaced* arrived miner walking back — and the guard makes that case provably safe rather than argued safe.

### D. 🚩 THE BOARD SPEC IS FACTUALLY WRONG ABOUT `Abandon`, AND I RECONCILED IT — QA SHOULD RULE.

The board (`TASKBOARD.md:7103`) says: *"`Abandon` ⇒ `StandInPlace` and let `UpdateMining`'s own retarget logic choose again next poll."* My dispatch prompt says: *"`Abandon` → `SeekBestMine()` for a different node."*

**Read against the shipped code, the board's version is a no-op.** `UpdateMining`'s retarget gate only re-seeks when `bTargetDead` — `!Node || IsDepleted() || GetGoldReserve() <= 0 || bOutOfOrderDisc` (`:371`). A perfectly healthy mine that is merely **unreachable** is not dead, so the "own retarget logic" does **not** choose again: the next poll falls straight through to `EnsureWalkingToNode(Node)` and walks right back at the same unreachable mine. `StandInPlace()` alone buys 0.25 s and nothing else.

**I implemented both**, which satisfies each source and is safe: `StandInPlace()` (the board's mechanism, and it is the one listed in the task's `names:` block) **then** `SeekBestMine()` under the tenure guard (the dispatch's mechanism). ⚠️ **I am not claiming this makes `Abandon` strong.** `FindBestMineFor` is a nearest-first finder and mines do not move, so from roughly the same position it will frequently return the **same** node. What genuinely changes is that the next poll issues a request from a fresh start poly against the navmesh as it stands *now* — and `Evaluate` has already `Reset` the ladder, so a second `Abandon` must earn the whole 6 s climb again. That is bounded and honest; it is not a guarantee the miner escapes.

⚠️ **Second-order note on `Abandon` under Hold/Ambush:** `SeekBestMine` is the map-wide finder, so it can briefly target a mine *outside* the position circle. The retarget gate's `bOutOfOrderDisc` test catches it on the very next poll and re-seeks with `SeekMineInDisc`, so it self-heals in 0.25 s with no bookkeeping involved (`bArrivedAtNode` is false under the guard, so that gate's `LeaveMining` branch cannot run). Picking between the two finders inside the rung would have meant a second `ResolveMinerOrder` on the same poll for a case that corrects itself.

### E. The lease is armed only when the sidestep point is actually walkable-to.

`DriveToPoint` treats anything inside `ArrivalRadius` (150 uu) as **arrived** and answers with `StopMovement` instead of a move (`:832-839`). A `SidestepDistance` tuned at or below 150 — or the degenerate `return Location` that `ComputeSidestepGoal` gives for a non-positive/NaN distance — would therefore **park** the miner for the whole 2 s lease and call that a rescue. `FSiegeStuckTuning` is `EditDefaultsOnly` and Jonathan tunes it without a recompile, so the rung has to be safe under arbitrary values (`NAV-§7`), not just the shipped 350. One `Dist2D` compare guards both cases.

## 6. THE REGRESSION LAW — an un-stuck miner is behaviourally IDENTICAL to today

Stated explicitly because the board makes it QA's criterion, and it is the cheapest regression proof in the batch.

`FSiegeStuckStatics::Evaluate`'s first branch (`SiegeStuckStatics.cpp:40-49`) returns `None` — after re-anchoring — for **any** of: not advancing, at or above `MinSpeedSq`, no anchor yet, or escaped `ProgressRadius`. **A miner that is walking normally, standing at its mine, waiting at a ring, or idle by design hits that branch on every poll and `HandleStuckEscalation` is never called at all.** The lease is 0 in all of those cases, so the early-out never fires either. ⇒ walk → register → +1 gold/s → wait-mode → eviction → all-depleted idle is **unchanged, line for line**.

⛔ **The income bookkeeping is untouched:** the retarget gate, `TryRegisterArrivedMiner`, `AddMinerIncome`/`RemoveMinerIncome`, `EndMineTenure`, the per-tenure latches (`bArrivedAtNode`/`bIncomeActive`), eviction (`NotifyMineDepleted`), the death path (`EndPlay`), `AGoldNode`'s exclusive-occupancy claim and the §3.3 cap are all **byte-for-byte unmodified**. The only writes my diff makes to this class's state are `SidestepLeaseRemaining`, `bHasIssuedPointGoal`, and `TargetGoldNode` **via `SeekBestMine()` under the `!bArrivedAtNode` guard**.

## 7. THE FOUR SEALS — all intact, each checked at the artifact

| # | seal | status |
|---|---|---|
| 1 | `StateCheckInterval = 0.f` (`MinerUnit.cpp:62`) | ⛔ **untouched.** Never read in this diff either — comment mentions only. |
| 2 | `AggroRadius = 0.f` (`MinerUnit.cpp:70`) | ⛔ **untouched.** No rung reads or writes it. |
| 3 | `ClearAllTimersForObject(this)` after `Super::BeginPlay` | ⛔ **untouched.** No new timer to survive it; the watchdog rides the existing poll. |
| 4 | `CanEverAttack() → false` (`MinerUnit.h`) | ⛔ **untouched.** No rung calls an acquisition or attack surface (they are private on the base and unreachable from this file), and no rung calls `EnterAttack`. |

`FreezeAI` still kills every miner timer: `AMinerUnit::FreezeAI` is **not modified** — `Super::FreezeAI()` (which also clears `SidestepLeaseRemaining` and `Reset`s `StuckState` at `SummonedUnit.cpp:697-698`), then `ClearTimer(MiningPollTimerHandle)`, then `StopMiningClink()`. A frozen miner's poll is dead, so the watchdog cannot run; and its lease is 0, so nothing is held.

## 8. Points QA should scrutinise

1. **§5 flag A — the lease read.** The declared third edit. Is a 2 s delay to the at-the-ring test acceptable for a demonstrably wedged miner, or should `Sidestep` ship vacuous? This is my most significant interpretation call.
2. **§5 flag B — `StopMovement()` in `WidenAndRepath`.** Confirm you agree the dispatch's stated mechanism (clearing `bHasIssuedPointGoal`) does not reach `EnsureWalkingToNode`'s gate, and that `StopMovement` is the one driver cancelling its own request rather than a second driver.
3. **§5 flag D — the board-vs-dispatch `Abandon` discrepancy**, and my claim that the board's "the retarget gate chooses again" is false against `MinerUnit.cpp:371`.
4. **Criterion 4 — the delta.** `ConsumeStuckDeltaSeconds()`, never `StateCheckInterval`; a `0` delta is inert; one consumer per miner (§2).
5. **Criterion 2 — no second driver.** `AMinerUnit` overrides **only** `HandleStuckEscalation`; no `SetTimer`; no `EnterAdvance` / `EnterAdvanceToLocation` / `EnterIdle` / `EnterAttack` anywhere in the diff.
6. **Criterion 8 — pin conformance.** One symbol, `protected`, `virtual … override`, exact spelling.
7. **The income boundary (§6) and the four seals (§7)** — the two things a miner change is most likely to break silently.
8. **Standing C++ sweep:** complete-type include added to `MinerUnit.h` for the enum parameter and to `MinerUnit.cpp` for `FSiegeStuckStatics` (both also reachable transitively; named per IWYU) · no shadowing of inherited reflected members — locals are `Action` (the pinned parameter name), `StalledGoal`, `Attempt`, `SidestepPoint`, `Node`, `AI`, and none collides with a member on either class (grepped) · no most-vexing-parse (no default-constructed locals declared with `()`) · every `case` body is braced, so the declarations inside them are legal · the `switch` is total with an explicit `None` arm · `FVector::Dist2D`'s double result is explicitly `static_cast<float>`, this build treating warnings as errors.
9. **Not compiled.** Braces balance in both files (103/103 and 8/8) and the pin matches, but **correctness as source is the only claim I can make** — TASK-538 owns the compile.

---

# ═══ QA LOOP 1 (2026-08-04) — THE BLOCKER FIX, ONE LINE ═══

**Status:** `ready-for-qa` (loop 1 of 3) · **Gate: TASK-537**, re-gate is `SC-§27` **diff-scoped**
**QA report fixed against:** `.claude/pipeline/qa/TASK-537.md` — THE ONE BLOCKER
**Files touched this loop: EXACTLY ONE — `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp`.**
⛔ `MinerUnit.h` was **not** reopened: this class adds **no member** for the fix, it inherits one.

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Structurally: this loop adds **nothing at all** to `AMinerUnit`. It reads
`ASummonedUnit::SidestepAttemptCount`, which TASK-532 declared **unreflected** — an unreflected
member cannot be replicated by accident. Units are server-only in M8 P1.

## 1. THE ONE-LINE CHANGE

In `AMinerUnit::HandleStuckEscalation`'s `Sidestep` case:

```cpp
// was: static_cast<int32>(StuckState.EscalationLevel) + static_cast<int32>(GetUniqueID() % 2)
const int32 Attempt = static_cast<int32>(SidestepAttemptCount++)
    + static_cast<int32>(GetUniqueID() % 2);
```

`SidestepAttemptCount` is `ASummonedUnit`'s new protected, unreflected, free-running `uint8`
(TASK-532 owns its declaration and its full rationale). `AMinerUnit : public ASummonedUnit`
(`MinerUnit.h:287`), so this is a plain inherited read — ⛔ **no member added, no shadow, and
`MinerUnit.h` untouched.**

**The stale comment went with it.** It claimed the level term *"alternates the side if a rung
ever sidesteps twice inside one stall"* — a branch brake 2 makes unreachable, which QA correctly
called a trap for the next reader. The replacement says: the attempt term alternates **this
miner's own side across successive stalls**; the id term is a fixed phase offset that
de-correlates neighbouring miners; the counter is **deliberately not in `FSiegeStuckState`**
because `Reset` clears that struct between stalls and that is exactly what made the old term a
per-unit constant; and **the `uint8` wrap at 255 -> 0 is harmless because only the parity is read.**

## 2. ⛔ THE MINER'S FENCES — ALL STILL INTACT, RE-CHECKED ON THE DIFF

- **The four seals:** `StateCheckInterval = 0.f` (`:62`) · `AggroRadius = 0.f` (`:71`) ·
  `ClearAllTimersForObject(this)` after `Super::BeginPlay` · `CanEverAttack() -> false`
  (`MinerUnit.h:354`) — ⛔ **not one of them is in this diff.** `FreezeAI` unmodified.
- **The income boundary is untouched:** no rung touches `TryRegisterArrivedMiner`,
  `AddMinerIncome`/`RemoveMinerIncome`, `EndMineTenure`, `bIncomeActive`, `NotifyMineDepleted`
  or the §3.3 cap. The only writes remain `SidestepLeaseRemaining`, `bHasIssuedPointGoal` and
  `TargetGoldNode` via the tenure-guarded `SeekBestMine()`.
- ⛔ **Nothing QA accepted was re-litigated.** All three of my declared additions stand exactly
  as shipped and as ruled ACCEPTED: `StopMovement()` in `WidenAndRepath`, `StandInPlace()` +
  guarded `SeekBestMine()` for `Abandon`, and the `SidestepLeaseRemaining` early-out in
  `UpdateMining`. Not one character of any of them moved.
- ⛔ No new timer, no second driver, no `EnterAdvanceToLocation`, no `Build.cs`, no `.umap`,
  no `.ini`. ⛔ **Not compiled** — TASK-538 owns the only compile.
- **Delta source unchanged:** `TickStuckWatchdog(ConsumeStuckDeltaSeconds())`;
  `StateCheckInterval` still appears in my diff only inside one explanatory comment.

## 3. ⚠️ WARN-7, CORRECTED IN THE ACCOUNTING AS QA REQUIRED

QA is right and my §5A list was incomplete. The lease early-out returns **above**
`ResolveMinerOrder()`, so an order arriving during a live lease **also delays `LeaveMining()`** —
i.e. the end of a mine tenure, and therefore `RemoveMinerIncome` — by up to `SidestepLeaseSeconds`
(2.0 s). §5A named only the at-the-ring test and omitted this. **It is bounded, self-healing on
the next poll, breaks no latch or invariant (≤ ~2 gold), and applies only to a miner that is
demonstrably wedged — so it ships**, but the corrected list of what the lease can delay is:
**(1)** the at-the-ring arrival/registration test and the wait-mode auto-claim retry, **and
(2) the end of a tenure via `LeaveMining()` when a new order lands mid-lease.** ⛔ Recorded here
so the next reader does not re-derive it as a defect. **No code change** — QA ruled it ships.

## 4. ⚠️ WHAT QA SHOULD RE-CHECK (diff-scoped)

1. The single changed statement and the comment above it.
2. That `MinerUnit.h` is **not** in this loop's diff and no member was added.
3. That the counter is read, never cleared, in this file — one hit, in the `Sidestep` case.
4. The four seals and the income boundary, which a miner change is most likely to break silently.
