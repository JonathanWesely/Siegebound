# TASK-532 — `ASummonedUnit` watchdog wiring + sidestep lease + resets

**Status:** ready-for-qa · **QA gate: TASK-537** · **Author:** gameplay-programmer
**Files touched (EXACTLY two, both pre-declared as my exclusive ownership):**
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp`

⛔ Not touched: `MinerUnit.*` (533) · `SiegeNavAreas.*` (534) · `BattlefieldScatter.*` (535) · `SiegeStuckStatics.*` (531) · `Tests/` (536) · `Build.cs` · any `.umap` · any `.ini`.
⛔ No compile, no Git, no editor/MCP/PIE. **Not compiled — TASK-538 owns the only compile.**

## M8 DECLARATION (verbatim, as required)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Reason it holds structurally: `StuckState`, `SidestepGoal`, `SidestepLeaseRemaining` and `LastStuckTickTimeSeconds` are **unreflected** (no `UPROPERTY`), so they cannot be replicated by accident; `StuckTuning` is `EditDefaultsOnly` CDO **config**, identical on every machine by construction, and must not become replicated. Units are server-only in M8 P1.

---

## 1. The pinned signatures, exactly as written

Verified character-for-character against `NAV-§8`. All nine symbols, with the pinned access levels:

```cpp
// SummonedUnit.h — public (line 332)
void NotifyMoveBlocked();

// SummonedUnit.h — protected (lines 906 / 923 / 933)
void         TickStuckWatchdog(float DeltaSeconds);
virtual void HandleStuckEscalation(ESiegeStuckAction Action);
float        ConsumeStuckDeltaSeconds();

// SummonedUnit.h — protected members (lines 936–959)
FSiegeStuckState StuckState;
UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck")
FSiegeStuckTuning StuckTuning;
FVector SidestepGoal            = FVector::ZeroVector;
float   SidestepLeaseRemaining  = 0.f;
float   LastStuckTickTimeSeconds = 0.f;
```

⚠️ **Dispatch-prompt discrepancy, resolved in favour of the pin:** my dispatch listed the members as `… SidestepLeaseRemaining, and the named SidestepLeaseSeconds tunable` and omitted `LastStuckTickTimeSeconds`. `SidestepLeaseSeconds` is **not** an `ASummonedUnit` member — it is a `FSiegeStuckTuning` field owned by TASK-531 (`SiegeStuckStatics.h:153`), and I read it as `StuckTuning.SidestepLeaseSeconds`. `LastStuckTickTimeSeconds` **is** in the pin and is required by `ConsumeStuckDeltaSeconds`, so it is declared. **The pin governs; nothing was invented.**

✅ I verified my calls against TASK-531's **delivered** `SiegeStuckStatics.{h,cpp}` (landed while I worked), not just the pin: `Evaluate` / `ComputeSidestepGoal` / `Reset` signatures, the eight `FSiegeStuckTuning` field names, and `enum class ESiegeStuckAction : uint8 { None, Sidestep, WidenAndRepath, Abandon }`. TASK-534's `SiegeNavAreas.cpp:161` calls `Unit->NotifyMoveBlocked();` — matches my public declaration.

## 2. The call site — one line, `UpdateState`, immediately after `TrackChargeMovement()`

`SummonedUnit.cpp:1348` (was `:1285` pre-edit):

```cpp
TrackChargeMovement();

// … comment block …
TickStuckWatchdog(ConsumeStuckDeltaSeconds());
```

✅ **Confirmed at the artifact, as the spec required:** `UpdateState`'s early-out `if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen) return;` sits **above** this line, so **dead, match-end-frozen and spell-frozen units never reach the watchdog at all.**
✅ It is **above** the follow hoist and **above** the profile dispatch, so **Standard, Siege, Support, Follow and Hold/Ambush are covered by this single line** — exactly the `TrackChargeMovement` precedent. No call was scattered into any individual body.
✅ **Zero new timers.** Rides the existing 0.25 s `StateTimerHandle`. No `SetTimer` added anywhere.

## 3. THE SIDESTEP LEASE — armed, held, drained, cleared

**Armed** — `HandleStuckEscalation`, `Sidestep` rung only, `SummonedUnit.cpp:2989`:
```cpp
SidestepLeaseRemaining = StuckTuning.SidestepLeaseSeconds;
EnterAdvanceToLocation(SidestepGoal);
```
Set **before** the move so state is consistent whatever the request returns.

**Held** — `UpdateState`, immediately after the watchdog call:
```cpp
if (SidestepLeaseRemaining > 0.f)
{
    return;
}
```

⚠️ **READ THIS ONE, IT IS THE DESIGN DECISION QA SHOULD PRESS HARDEST:** the lease block **re-issues nothing** — it only *suppresses the profile dispatch*. The spec's phrasing ("keep steering to `SidestepGoal` for the lease's duration") reads like a per-poll `EnterAdvanceToLocation(SidestepGoal)`, and **I deliberately did not do that, because it would break the `NAV-§3` worst-case bound.** `EnterAdvanceToLocation`'s guard (`:2677`) falls through on `GetMoveStatus() == Idle`; a sidestep goal the navmesh rejects (and `ComputeSidestepGoal` is documented raw/unprojected, *"NOT guaranteed reachable, by design"* — `SiegeStuckStatics.h:230`) leaves the status Idle every poll, so a per-poll re-issue would fire **4 path requests/second for the whole 2 s lease** — over the *"≤ 1 extra path request per unit per second"* ceiling that gate criterion 1 calls a **FAIL by construction**. Suppressing the dispatch is sufficient: the `MoveToLocation` is already in flight and path-following carries it out. **One rung, one request.**

**Drained** — `TickStuckWatchdog`, unconditionally, on the same `Delta` as the ladder, **before** `Evaluate`, so it expires on schedule even on polls where the unit is moving fine.

**Cleared (with `FSiegeStuckStatics::Reset(StuckState)`) — trace each individually:**

| # | site | line | note |
|---|---|---|---|
| 1 | `EnterIdle` | `:2710` | after the `State == Idle` early-out; an already-Idle unit cannot hold a lease (arming goes through `EnterAdvanceToLocation`, which sets `State = Advance`) |
| 2 | `EnterAttack` | `:2472` | inside the `State != Attack` transition block, beside the existing `CurrentMoveGoal = nullptr` — the line the dispatch pointed at |
| 3 | `HandleDeath` (death) | `:3466` | beside the timer clears; a rigged corpse lingers up to `DeathAnimMaxHoldSeconds` and must not hold a live lease |
| 4 | `FreezeAI` | `:697` | after its `CurrentMoveGoal = nullptr` |
| **5** | **`ApplyFreeze`** | **`:942`** | ⚠️ **DECLARED ADDITION — see §6 flag A** |

Rungs 2 and 3 also clear the lease (`SidestepLeaseRemaining = 0.f`) — see §4.

## 4. What each rung does

| rung | action | why |
|---|---|---|
| **`Sidestep`** | `SidestepGoal = FSiegeStuckStatics::ComputeSidestepGoal(GetActorLocation(), StalledGoal, StuckTuning.SidestepDistance, Attempt)`; arm lease; **one** `EnterAdvanceToLocation(SidestepGoal)` | `StalledGoal` = live `CurrentMoveGoal`'s location → else `CurrentMoveGoalLocation` (if `bHasMoveGoalLocation`) → else own location (degenerate; TASK-531 returns the stable +Y fallback, no NaN) |
| **`WidenAndRepath`** | **clears the lease FIRST**, then `CurrentMoveGoal = nullptr; bHasMoveGoalLocation = false; bHasFollowGoalLocation = false;` | a **genuine** re-path toward the **ORIGINAL** goal |
| **`Abandon`** | clears the lease; invalidates the same three latches; `CurrentTarget = nullptr`; `EnterIdle()` | stand-down; the standing body re-chooses next poll. ⛔ No `Reset` call — `Evaluate` already reset internally (`SiegeStuckStatics.cpp:152`) |

**⚠️ Lease-clear ordering in `WidenAndRepath` is load-bearing** and is the second-most-likely thing to get wrong after the lease itself: with `SidestepSeconds` 1.5, `SidestepLeaseSeconds` 2.0 and `WidenSeconds` 3.0, **the lease is still live when Widen fires** (armed 1.5 → expires 3.5; Widen at 3.0). If the rung did not drop the lease first, `UpdateState` would keep returning early, the re-path would never reach a mover, and because `EscalationLevel` is **monotonic the rung could never fire again** — a silent no-op rung. Same reasoning applies to `Abandon`.

**⚠️ `WidenAndRepath` is implemented as latch invalidation, not as a literally widened acceptance radius** — a declared departure from my dispatch's wording, taken straight from `NAV-§3`'s own sanctioned mechanism: *"re-issue the ORIGINAL goal ONCE with a genuine re-path (e.g. clear `CurrentMoveGoal` so the next `EnterAdvance*` really re-issues)"*, and its ⛔ *"never widen anything that persists past the stall."* Every acceptance radius in this file (`StructureMoveAcceptanceRadius`, the `AttackRange * 0.8f` pawn radius) is a **shared** constant the normal path reads too, so a genuinely widened radius would either persist past the stall or require threading a parameter through `EnterAdvance`/`EnterAdvanceToLocation` — which the "minimal diff / don't touch the movement call sites" instruction and the persistence prohibition both rule out. **Three** latches are invalidated because three different bodies own them and any one left set would swallow the re-issue: the actor gate (`:2533`), the point gate (`:2676-2677`), and the follow body's `LastFollowGoalLocation` drift latch.

⛔ **No rung cancels the standing ORDER:** `CommandGroupId`, the group and the follow enrolment survive every rung. ⛔ **No rung enters Attack.** ⛔ **No rung leaves the unit inert forever** — `Abandon`'s `EnterIdle` is a stand-down and the next poll re-dispatches.

## 5. `NotifyMoveBlocked` — evidence only, `NAV-§3` honoured

```cpp
if (bDead || bAIFrozen || bSpellFrozen) { return; }
if (!StuckState.bHasAnchor) { StuckState.ProgressAnchor = GetActorLocation(); StuckState.bHasAnchor = true; }
StuckState.StalledSeconds = FMath::Max(StuckState.StalledSeconds, StuckTuning.SidestepSeconds);
```

⛔ **It issues no move, and it does not call `HandleStuckEscalation`.** It advances the stall clock **to** the first rung's threshold — never past it, never backwards — so it can only ever pull the **first** escalation forward by one poll, and it **cannot** skip `Sidestep` and jump a unit to `Abandon`. Both brakes still apply: the next `Evaluate` re-checks `EscalationCooldown` and the monotonic `EscalationLevel`. **`TickStuckWatchdog` remains the only thing in this class that steers.**

## 6. FLAGGED — decisions I made rather than buried. QA rules on these.

**A. The declared FIFTH lease-clear site: `ApplyFreeze` (`:942`).** The spec names four; I found a fifth by tracing the lease and added it. `ApplyFreeze` (TASK-099 spell freeze) parks the unit by writing `State = ESummonedUnitState::Idle` **directly**, so it never routes through `EnterIdle` — and `EnterIdle`'s own `State == Idle` early-out (`:2696`) means a later `EnterIdle` could not clean up after it either. It also clears `StateTimerHandle`, so `UpdateState` stops running and the lease **cannot even drain**. Without this, a spell-frozen unit thaws holding a live lease and ignores its standing body for up to `SidestepLeaseSeconds`, steering to a `SidestepGoal` chosen before the freeze. Same rule, same two lines. **If QA judges this out of scope, deleting those two lines is self-contained and breaks nothing.**

**B. `MaxStuckDeltaSeconds = 1.f`, a new file-local `constexpr` (`SummonedUnit.cpp:103`).** Upper clamp on one watchdog delta. ⚠️ **TASK-531's `Evaluate` explicitly takes the opposite stance** — its comment (`SiegeStuckStatics.cpp:98-101`) says a single large delta jumping level 0 → 3 is *"the honest reading — the unit really has been stalled that long"*. I still clamp, because the gap this guards is **not** a stall: `ApplyFreeze` stops `UpdateState` for the whole freeze (up to 4 s), and that unit was **frozen**, not failing to path. In practice the two readings converge — `ApplyFreeze` calls `StopMovement`, so `bAdvancing` is `false` on the resume poll and `Evaluate` re-anchors regardless — so the clamp makes provable what would otherwise rest on a second-order argument. **Deliberately NOT an `FSiegeStuckTuning` field:** `NAV-§7` pins those eight as the feature's only tunables, and this is a safety rail, not a feel knob. **Named constant, not a bare literal.**

**C. ⚠️ `NotifyMoveBlocked`'s evidence is usually discarded on the very next poll, and I could not fix it inside my file.** This is a genuine spec-level tension and the thing I most want QA to rule on. `Evaluate`'s cheap path re-anchors and **zeroes the clocks** when `!bAdvancing` (`SiegeStuckStatics.cpp:40-49`). A `Blocked` verdict means the path request **finished**, so `GetMoveStatus()` is `Idle` on the next poll ⇒ `bAdvancing == false` ⇒ my `StalledSeconds` bump is wiped before any rung sees it. Setting the anchor (above) removes one of the two wipe conditions; the `!bAdvancing` one I cannot remove without redefining the `bAdvancing` idiom, which `NAV-§3` pins to `GetMoveStatus()` and whose whole purpose is to stop the ladder rescuing units told to stand still. **The feature does not depend on this:** the engine only declares `Blocked` after 5.0 s, by which time our own ladder (1.5 / 3.0 / 6.0 s) has already fired `Sidestep` and `WidenAndRepath` off its own clock — during a livelock the re-issued request is live, so `GetMoveStatus()` is `Moving` and the ladder accumulates normally. `NotifyMoveBlocked` is therefore **corroborating** evidence rather than the primary trigger. `NAV-§3` says it *"may"* advance the clock, so this is compliant — but the honest reading is that **rung 7 of my spec buys less than its emphasis implies**, and TASK-536 cannot test it headlessly.

**D. `Attempt` = `StuckState.EscalationLevel + (GetUniqueID() % 2)`.** The level term alternates the side if a rung ever sidesteps twice within one stall; the unit-id term **de-correlates neighbours**, so a clump wedged on the same rock does not all sidestep the same way into each other (one stuck unit becoming several). `GetUniqueID()` is not stable across runs — ✅ **this does not touch `NAV-§4`'s determinism law**, which governs the `BattlefieldScatter` instance set as a pure function of the seed; no `FRandomStream` draw or ordering is involved here, and `ComputeSidestepGoal` itself remains pure and deterministic in its inputs. TASK-531 documents `Attempt` as *"total over the whole int32 range including negatives"*, so the non-negative value I pass is safe either way.

**E. `Abandon` clears `CurrentTarget`.** `NAV-§3` says *"drop the current move goal … let the standing body re-choose"* and names `EnterIdle()` as acceptable; `EnterIdle` alone does **not** clear `CurrentTarget`, so a Standard unit would re-derive the same goal from the same target and re-wedge. Clearing it is what makes the next poll a **re-target** rather than a re-chase. It is not an order cancellation (`CommandGroupId` untouched). Attackers cannot reach this rung at all — `EnterAttack` calls `StopMovement`, so `bAdvancing` is false.

**F. The `escalate:` line is logged in `TickStuckWatchdog`, not in `HandleStuckEscalation`.** So TASK-533's miner override emits the same pinned tokens without having to re-implement them. `level=` uses the **action's ordinal** (`None` 0 / `Sidestep` 1 / `WidenAndRepath` 2 / `Abandon` 3), which is the rung index by construction and — unlike `StuckState.EscalationLevel` — **survives the `Reset` that `Evaluate` performs internally on `Abandon`**. `stalled=` is reconstructed as `StalledSeconds + Delta` captured **before** `Evaluate`, for the same reason; verified against `SiegeStuckStatics.cpp:62` (`State.StalledSeconds += SafeDelta;` happens before rung selection), so it is exactly the value the ladder compared against its thresholds.

## 7. Points QA should scrutinise

1. **Trace all five lease-clear sites** (table in §3) — criterion 3. Confirm each also calls `FSiegeStuckStatics::Reset(StuckState)`.
2. **The lease block re-issues nothing** (§3) — confirm you agree this satisfies criterion 1 rather than violating the spec's "keep steering" wording. This is my most significant interpretation call.
3. **Lease-clear ordering inside `WidenAndRepath`/`Abandon`** (§4) — without it those rungs are silent no-ops under the shipped defaults.
4. **`NotifyMoveBlocked` issues no move and calls no rung** — criterion 2, and flag C is the honest limitation.
5. **`DeltaSeconds` never comes from `StateCheckInterval`** — criterion 4. Grep confirms `StateCheckInterval` appears in my diff only inside explanatory comments, never as a value read.
6. **No new timers** — criterion 6/2. No `SetTimer` in my diff.
7. **Standing C++ sweep:** complete-type include added to `SummonedUnit.h` (by-value members require it); no shadowing of inherited reflected members (locals are `Delta`, `Now`, `Action`, `StalledGoal`, `Attempt`, `StalledAtEscalation`, `bAdvancing`, `AI`, `World` — none collide with any member, checked module-wide); no most-vexing-parse (no default-constructed locals declared with `()`); `switch` on `ESiegeStuckAction` is total with an explicit `None`/`default` arm.
8. **Not compiled.** Braces balance in both files and all nine pinned symbols match the registry, but **correctness as source is the only claim I can make** — TASK-538 owns the compile.

---

# ═══ QA LOOP 1 (2026-08-04) — THE BLOCKER FIX ═══

**Status:** `ready-for-qa` (loop 1 of 3) · **Gate: TASK-537**, re-gate is `SC-§27` **diff-scoped**
**QA report fixed against:** `.claude/pipeline/qa/TASK-537.md` — THE ONE BLOCKER
**Files touched this loop (EXACTLY the two I own):** `SummonedUnit.h`, `SummonedUnit.cpp`

> **adds no replicated property, no new replicated class, no new relevancy tier.**

It still holds, and the new member is why it holds *structurally*: `SidestepAttemptCount` is
**unreflected** (no `UPROPERTY`), so it cannot be replicated by accident, and it is not an
`FSiegeStuckTuning` field either — it is state, not a feel knob. Units are server-only in M8 P1.

## 1. WHAT WAS WRONG — QA'S CHAIN, ACCEPTED IN FULL

`Evaluate` assigns `State.EscalationLevel = DesiredLevel` **before** its switch and returns
`Sidestep` only for level 1, so `StuckState.EscalationLevel` was **always exactly 1** when
`HandleStuckEscalation(Sidestep)` ran. Brake 2 lets `Sidestep` fire once per stall, and `Reset`
assigns a default-constructed `FSiegeStuckState` between stalls. ⇒ my `Attempt` was
`1 + (uid % 2)` — **a per-unit constant**, so `ComputeSidestepGoal`'s parity contract was
correct and **unreachable**, and a unit wedged on a rock's left face sidestepped into that rock
on every stall, forever. **The rung that produces a different destination was a no-op for half
the army.** ⛔ I do not contest any link of this; my own `handoffs/TASK-531-programmer.md`
FLAG A had named the hazard and I then shipped the constant anyway. QA is right.

## 2. THE FIX — TWO EDITS, EXACTLY WHAT QA PRESCRIBED

**(a) `SummonedUnit.h`, protected, between `SidestepLeaseRemaining` and
`LastStuckTickTimeSeconds`** — a new **free-running, unreflected** member:

```cpp
uint8 SidestepAttemptCount = 0;
```

with a comment block stating: why it is deliberately **outside** `FSiegeStuckState` (Reset
clearing that struct between stalls is precisely what caused the bug); that it **wraps at
255 -> 0 and that this is harmless and intended** because only `Attempt & 1` is ever read and
256 is even; and ⛔ that **nothing may depend on it monotonically** — it is not a count anyone
reports, not a clock, not an index, and it is never compared with `>`, `<` or a threshold.

**(b) `SummonedUnit.cpp`, `HandleStuckEscalation`'s `Sidestep` case:**

```cpp
// was: static_cast<int32>(StuckState.EscalationLevel) + static_cast<int32>(GetUniqueID() % 2)
const int32 Attempt = static_cast<int32>(SidestepAttemptCount++)
    + static_cast<int32>(GetUniqueID() % 2);
```

**And the stale comment is gone.** It used to say *"the level term alternates the side if a rung
ever sidesteps twice inside one stall"* — a branch brake 2 makes **provably unreachable**, i.e.
exactly the trap-for-the-next-reader QA called out. The replacement states what each term
actually does: the counter alternates **this unit's own side across successive stalls**, the
unit-id term is a **fixed phase offset** that de-correlates neighbours without ever defeating
that alternation.

## 3. ⛔ WHAT WAS **NOT** TOUCHED — THE FENCES, NAMED

- ⛔ **`NAV-§8`'s pinned registry is UNTOUCHED.** No signature moved, no parameter changed, no
  access level changed, and ⛔ **no field was added to `FSiegeStuckState` or `FSiegeStuckTuning`.**
  All nine pinned `SummonedUnit.h` symbols still read character-for-character as pinned;
  `SidestepAttemptCount` is a **non-pinned, non-reflected** addition, the practice QA already
  accepted for TASK-534's `LastBlockedLogTimeSeconds` and TASK-535's six members.
- ⛔ **The five lease-clear sites are byte-unchanged** (`EnterIdle`, `EnterAttack`, `HandleDeath`,
  `FreezeAI`, `ApplyFreeze`), as are brake 1 and brake 2, the watchdog call site, the lease
  early-out, `NotifyMoveBlocked`, the `escalate:` log line and every rung body but the two lines
  above. ⭐ **`SidestepAttemptCount` is deliberately NOT cleared at any of those sites** — a lease
  clear ends a *stall*, and surviving stalls is the entire point.
- ⛔ No new timer, no second steering driver, no `Build.cs` change, no `.umap`, no `.ini`.
- ⛔ Nothing QA accepted was re-litigated: the lease-as-suppression design, the higher rungs
  dropping the lease, the rung-0 `OR`, and `NotifyMoveBlocked` shipping as-is all stand.
- ⛔ **Not compiled** — TASK-538 owns the only compile.

## 4. STANDING C++ SWEEP ON THE DIFF

- **No shadowing.** `SidestepAttemptCount` is module-wide unique (grepped `Source/`: zero prior
  hits). `AMinerUnit : public ASummonedUnit`, and it declares **no** member of that name — it
  reads the inherited one, so there is no shadow of a reflected or unreflected member.
- **Accessibility:** `protected:` at `SummonedUnit.h:593` governs, `private:` follows the member
  — so `AMinerUnit::HandleStuckEscalation` can read it. Verified at the artifact.
- **The post-increment is well defined:** `uint8` is unsigned, so `255 -> 0` is a defined wrap,
  not UB; `x++` yields the pre-increment value as a `uint8`, so `static_cast<int32>` sees
  `0..255` and `Attempt` stays non-negative (`0..256`) — the callee's `& 1` was already total
  over the whole `int32` range either way.
- **One increment per fired rung.** The statement is inside the `Sidestep` case, which brake 2
  reaches at most once per stall, so the counter cannot be advanced twice by one stall and the
  parity cannot silently skip a side.
- No most-vexing-parse, no new include (the member is a built-in type), `switch` still total.

## 5. ⚠️ WHAT QA SHOULD RE-CHECK (diff-scoped)

1. The two changed lines and the comment that replaced the unreachable-branch text.
2. That `SidestepAttemptCount` is **not** cleared anywhere — grep it; two hits in this file
   (declaration + use) and one in `MinerUnit.cpp` is the complete set.
3. That the pin is intact: the nine `SummonedUnit.h` symbols, their order and their access.
4. ⚠️ **One thing I changed beyond the two lines, declared rather than buried:** my comment
   edits in this loop expanded `SiegeStuckStatics.cpp` (TASK-531), which drifted some line
   citations. I replaced the line-number citations **I authored** with **symbol** citations
   (`FSiegeStuckStatics::Reset`, `::ComputeSidestepGoal`) per QA's own NIT-5 guidance. No
   behaviour, no signature and no pinned text is involved.
