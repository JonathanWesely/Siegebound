# QA Report — TASK-537 (UNIT-PATHING batch gate)

**Verdict: FAIL** — 1 BLOCKER · 10 WARN · 5 NIT
**Covers (SC-§29, roster cross-checked): TASK-529 · 530 · 531 · 532 · 533 · 534 · 535 · 536.** All eight handoffs name TASK-537 as their gate; the union IS the roster; no gap.

> ⛔ **THE LIMIT OF THIS VERDICT, STATED FIRST.** Nothing was compiled. No test was run. No PIE session happened and **no navmesh was observed.** Every criterion below was checked **statically, by reading the artifact**. A PASS on the seven non-blocking criteria means *correct as source* and nothing more; the runtime claims are settled by TASK-538's log and TASK-539's play. Where I re-verified an engine or log claim first-hand I say so and give the file:line.
>
> ⚠️ **Citation drift:** every line number below was re-derived by grepping the symbol in the file as it stands today. Board/CONVENTIONS citations from before the batch landed have drifted (e.g. the miner seal is `MinerUnit.cpp:62`, not `:61`; the watchdog call site is `SummonedUnit.cpp:1348`, not `:1285`).

---

## THE ONE BLOCKER

### [BLOCKER] `SummonedUnit.cpp:2978-2979` (and the identical idiom at `MinerUnit.cpp:1037-1038`) — the Sidestep rung's `Attempt` is a per-unit CONSTANT, so the side alternation the spec requires does not exist, and for the units that draw the wrong side the ladder detects the stall and can never escape it.

**The chain, verified link by link at the artifact — not accepted from the handoffs:**

1. `FSiegeStuckStatics::Evaluate` assigns `State.EscalationLevel = DesiredLevel` **before** the switch (`SiegeStuckStatics.cpp:135`), and returns `Sidestep` only for `DesiredLevel == 1`. ⇒ **`StuckState.EscalationLevel` is ALWAYS exactly 1 at the moment `HandleStuckEscalation(Sidestep)` runs.**
2. Brake 2 (`SiegeStuckStatics.cpp:102-105`, `DesiredLevel <= State.EscalationLevel ⇒ None`) is strictly monotonic, so **`Sidestep` fires at most once per stall.**
3. `FSiegeStuckState` (`SiegeStuckStatics.h:102-109`) carries **no attempt counter**, and `Reset` assigns a default-constructed instance (`SiegeStuckStatics.cpp:209`), which `Evaluate` calls on **every** `Abandon` and on **every** re-anchor. ⇒ nothing survives a stall.
4. The shipped producer is `Attempt = static_cast<int32>(StuckState.EscalationLevel) + static_cast<int32>(GetUniqueID() % 2)`. With (1), that is **`1 + (uid % 2)` ∈ {1, 2} — fixed for the lifetime of the unit.**

⇒ `ComputeSidestepGoal`'s parity contract is implemented **correctly** (`SiegeStuckStatics.cpp:195`, and TASK-536's test 14 exercises it over the whole `int32` range) and **is unreachable from either shipped call site.** A given unit sidesteps to the **same side, every stall, forever**.

**Why this is a BLOCKER and not a WARN — the three reasons, ranked:**

- **It silently no-ops the only rung that changes where the unit goes.** `WidenAndRepath` re-issues the **same** goal and `Abandon` stands down and re-chooses; `Sidestep` is the one rung that produces a *different destination*. For a unit whose fixed side is the blocked one, that rung issues a move that cannot help — on every stall, indefinitely — at ~0.32 requests/unit/s. That is the gate's own named failure mode: *"a ladder that detects stalls but cannot escape them."*
- **The spec mandated the alternation and the wiring does not deliver it.** TASK-531 spec (5) requires *"`Attempt` parity alternating the side (left/right/left…)"*, and the pinned signature takes `Attempt` **as a parameter precisely so the caller can vary it** (`NAV-§8:2595-2596`). This is not a design I am re-litigating (`NAV-§` forbids that) — it is a delivered behaviour that does not match its own spec.
- **The shipped comments document a branch that provably cannot occur.** `SummonedUnit.cpp:2973-2974` and `MinerUnit.cpp:1031-1033` both say *"The level term alternates the side if a rung ever sidesteps twice inside one stall."* Point (2) above makes that impossible. A comment describing an unreachable branch as live is a trap for the next reader.
- ⚠️ **And it contaminates the batch's diagnostic value, which is the other half of what this batch is for.** `NAV-§0` ruling 1 says the playtest log decides whether cause 1 or cause 2 was doing the work. If rung 1 is half-dead, a TASK-539 report of *"units still wedge"* cannot distinguish "cause 2 is real and unfixed" from "our rung 1 picked the wrong side." That is a whole play session of Jonathan's time, which is the resource this batch was explicitly shaped to conserve.

**Suggested fix — small, localised, and ⛔ it does NOT touch the pinned struct or any pinned signature.**

Add a free-running, **unreflected** per-unit counter on `ASummonedUnit` (protected, beside `SidestepLeaseRemaining` at `SummonedUnit.h:956`):

```cpp
/** Sidestep attempts this unit has made, EVER. ⛔ Deliberately NOT in FSiegeStuckState:
 *  Reset() clears that struct on every Abandon and every re-anchor, which is exactly what
 *  made the side constant. Free-running is the point — successive stalls must alternate. */
uint8 SidestepAttemptCount = 0;
```

then in `ASummonedUnit::HandleStuckEscalation`'s `Sidestep` case:

```cpp
const int32 Attempt = static_cast<int32>(SidestepAttemptCount++) + static_cast<int32>(GetUniqueID() % 2);
```

and the identical one-line read in `AMinerUnit::HandleStuckEscalation` (`MinerUnit.cpp:1037-1038`), which inherits the base member.

- ⛔ **`FSiegeStuckState` is NOT touched, `FSiegeStuckTuning` is NOT touched, no pinned signature moves, no `NAV-§8` amendment is required.** Adding a *non-pinned, non-reflected* member is already this batch's accepted practice (TASK-534 added `LastBlockedLogTimeSeconds`; TASK-535 added six members) — the pin is the **cross-task link contract**, and nothing outside these two files references this member.
- ✅ It preserves the half that already works: the `+ (uid % 2)` term still de-correlates neighbours (TASK-536 test 19's second block asserts that and should stay green).
- `uint8` wraps cleanly; `& 1` in `ComputeSidestepGoal` is total over the whole range (`SiegeStuckStatics.cpp:190-195`), including the wrap.

**Owners:** `SummonedUnit.{h,cpp}` = **TASK-532**; `MinerUnit.cpp` = **TASK-533** (one line). ⛔ Neither may edit the other's file (RULING 1, file ownership).
**Tests to update in the same loop:** `SiegeStuckStaticsTest.cpp:1519-1594` — test 19 `SidestepSideIsConstantForOneUnit` currently **asserts the hole as shipped** and will go red. It must be inverted to assert alternation across consecutive stalls (owner: TASK-536), and `ShippedAttemptForSidestep` (`:202-205`) re-modelled.
**Re-gate:** `SC-§27` diff-scoped — changed lines plus the fences the fix must not disturb (the pin, the five lease-clear sites, brake 1, brake 2). ⛔ Not a re-litigation of the passed design.

---

## THE SIX RULINGS — ANSWERED

### RULING 1 — **IT BLOCKS.** See above. Fix is ~5 lines across two files + one test inversion; no pin amendment.

### RULING 2 — the anti-mill rate claim is WRONG in the shipped comment AND in `NAV-§3`. **WARN, corrected in the same loop.** Here is the worst case, **derived by me, not accepted:**

Let `P` = poll period (**0.25 s on both drivers**: `ASummonedUnit`'s `StateTimerHandle` and `AMinerUnit::ArrivalCheckInterval = 0.25f`, `MinerUnit.h:493`), thresholds `S ≤ W ≤ A`, cooldown `C`.

- Per stall, `Evaluate` returns at most **3** non-`None` answers (levels strictly increasing, brake 2). Level 3 = `Abandon`, which issues **no** path request of its own. ⇒ **≤ 2 extra path requests per stall.**
- A stall ends only by `Abandon`'s internal `Reset` (`SiegeStuckStatics.cpp:152`) or by a re-anchor. `Reset` drops `bHasAnchor`, so the **next** poll must take the re-anchor branch and return `None`. ⇒ **≥ 1 dead poll per cycle.**
- Minimum cycle length is therefore **4 polls** (Sidestep, Widen, Abandon, re-anchor), reachable with `S ≤ P`, `W ≤ 2P`, `A ≤ 3P`, `C ≤ P` — e.g. `0.25 / 0.50 / 0.75`, `C = 0`, all legal `EditDefaultsOnly` values.

> ### ⇒ **WORST CASE OVER LEGAL TUNING = 2 requests / (4 × 0.25 s) = 2.0 EXTRA PATH REQUESTS / UNIT / SECOND.**
> ### ⇒ **WORST CASE AT THE SHIPPED DEFAULTS (1.5 / 3.0 / 6.0, C = 1.0) = 2 requests / 6.25 s = 0.32 / UNIT / SECOND.**
> ### ⇒ The `≤ 1 / unit / s` ceiling holds **for any `EscalationCooldown ≥ 1.0`, and rests on BRAKE 1 ALONE.**

**Brake 2 is a COUNT brake, not a rate brake** — a stall's *duration* is itself tunable, which is the step the comment misses. For scale, the forbidden per-poll mill is **4.0/unit/s**, so the pathological tuning is half the mill and twice the stated ceiling.

⇒ **This is a wording defect, not a shipped defect** (TASK-536's finding 1 is correct and its measurement matches my derivation). **It must be corrected before the commit**, because the comment is shipped source that asserts a false safety property:
- `SiegeStuckStatics.cpp:107-134` — the block headed *"EITHER BRAKE ALONE BOUNDS THE REQUEST RATE"*. Owner **TASK-531**. Replace with: brake 1 bounds the **rate** (`≤ 1 fire per EscalationCooldown`); brake 2 bounds the **count per stall** (`≤ 3 rungs, ≤ 2 path requests`); the `≤ 1/unit/s` ceiling is conditional on `EscalationCooldown ≥ 1.0`, and the worst case at `EscalationCooldown = 0` is 2.0/unit/s.
- `SiegeStuckStatics.h:120-124` carries the same false claim (*"EITHER ALONE bounds the request rate"*) and needs the same edit.
- **`NAV-§3` (`CONVENTIONS.md:2479-2482`) and `handoffs/TASK-531-programmer.md` §2** — manager correction owed; `NAV-§3`'s *"WORST CASE: ≤ 1 extra path request per unit per second"* must be qualified with *"at `EscalationCooldown ≥ 1.0`"*.

### RULING 3 — `NAV-§1` cause 3's stated mechanism does not hold. **TASK-535 is right; I verified both halves first-hand. Correction owed. The declared strengthening is ACCEPTED.**

- `UNavigationSystemV1::IsNavigationBeingBuilt` = `HasDirtyAreasQueued() || IsNavigationBuildInProgress()` (`NavigationSystem.cpp:5556`), and `IsNavigationBuildInProgress` ORs `FRecastNavMeshGenerator::IsBuildInProgressCheckDirty()` over `NavDataSet` (`:4900-4908`).
- `IsBuildInProgressCheckDirty()` = `RunningDirtyTiles.Num() || PendingDirtyTiles.Num() || SyncTimeSlicedData.TileGeneratorSync.IsValid()` (`RecastNavMeshGenerator.cpp:7764-7769`); `GetNumRemaningBuildTasks()` = `GetNumRemaningBuildTasksHelper()` = the **arithmetic sum of exactly those three terms** (`:7781-7784`, `RecastNavMeshGenerator.h:794`).
- ⇒ **`IsNavigationBuildInProgress() == false` ⟺ `GetNumRemainingBuildTasks() == 0`.** The poll **cannot** read idle with 326 tiles pending, and the pinned predicate alone would indeed have been a no-op on that path.
- 🔎 **ONE REFINEMENT TASK-535 OMITTED:** `IsNavigationBeingBuilt` also returns `false` outright when `IsNavigationBuildingPermanentlyLocked()` (`NavigationSystem.cpp:5554`). So the implication is *"…unless nav building is permanently locked."* It does not disturb the conclusion here (`RuntimeGeneration=Dynamic`, tiles demonstrably building), but the corrected law text should say it.
- **The real route, verified by me in the log, not relayed:** `Saved/Logs/GitClaudeUnrealTest-backup-2026.08.04-02.55.13.log` lines **2583/2584 · 2769/2770 · 3040/3041 · 3241/3242** — **4 of 4**, each `Navigation still building after 10.0 s (MaxNavSettleWait cap) — proceeding … anyway` immediately followed by `Traversability CONFIRMED`. **The false `CONFIRMED` comes from the 10 s cap, not from a false idle.**
- ⇒ **Manager corrections owed:** `NAV-§1` cause 3 (`CONVENTIONS.md:2454`, the `IsNavigationBeingBuilt` sentence) and `NAV-§12`'s *"which of `HasDirtyAreasQueued()` / `IsNavigationBuildInProgress()` returned false is UNKNOWN"* (`:2685` — it is now **answered**: neither did; the cap fired).
- ✅ **The `&& !IsNavigationBeingBuilt(World)` conjunct (`BattlefieldScatter.cpp:2085-2086`) is ACCEPTED and should NOT be reverted.** It is a *conjunction*, so it can only make `CONFIRMED` harder to print and the cull gate stricter — which is exactly the direction `NAV-§4` demands. It also closes the dirty-areas-not-yet-tile-tasks hole one level up. Keep it.

### RULING 4 — both false spec claims confirmed. **Corrections owed; neither changes any shipped code.**

**(a) `ARecastNavMesh::CellSize` is a real C++ deprecation.** Verified at `RecastNavMesh.h:710-712`: `UE_DEPRECATED(all, "Use NavMeshResolutionParams …")` on the member itself (and `CellHeight` `:714`, `AgentMaxStepHeight` `:718`). `AgentRadius` (`:730`) is **not** deprecated. `GetCellSize(ENavigationDataResolution) const` (`:1107`) reads `NavMeshResolutionParams[Resolution].CellSize` — the live value.
⇒ **TASK-529's substitution is CORRECT and the pinned `cellSize=` token is unchanged.** `NAV-§8`'s `SiegeNavDiagnostics.h` block / `NAV-§7`'s token list and `TASKBOARD.md:6997` must be amended to name `GetCellSize(ENavigationDataResolution::Default)`, because the next reader will otherwise reach for the member. **Manager owns.**

**(b) "This project compiles warnings as errors" is NOT established — the record is corrected here.** I grepped the whole of `Source/` for `bWarningsAsErrors` / `WarningLevel` / `DeprecationWarningLevel`: **zero matches**, in `GitClaudeUnrealTest.Target.cs`, `GitClaudeUnrealTestEditor.Target.cs` and every `.Build.cs`. UBT's default is `WarningLevel.Warning`. TASK-534 §8 and TASK-536 finding 5 are both right.

**Does it matter for the `OnMoveCompleted` question? No — the instruction survives its wrong justification.**
- Verified `AIController.h:230` (the `FPathFollowingResult` form, **not** deprecated) and `:232-233` (`UE_DEPRECATED_FORGAME(4.13, …)` on the `EPathFollowingResult::Type` form). `SiegeNavAreas.h:154` overrides **`:230`**. ✅ Correct overload, for the right reason (do not build on a deprecated API), just not for the stated reason (it would have been a warning, not an error).
- ✅ **The absence of `using Super::OnMoveCompleted;` is correct.** C4263/C4264 are off by default in MSVC and UBT does not enable them; adding it speculatively would name the deprecated overload in this module. The remedy is documented at `handoffs/TASK-534-programmer.md` §3 if TASK-538's compile ever emits it.
- ⛔ **No recursion hazard:** `AAIController::OnMoveCompleted(FAIRequestID, const FPathFollowingResult&)` forwards to the deprecated overload by unqualified call in the **base's** scope (`AIController.cpp:985-989`), which `ASiegeUnitAIController` does not override. Verified.
- ⇒ **Correct the record in three places:** `NAV-§8`'s bullet (`CONVENTIONS.md:2639`), `TASKBOARD.md:7124`, and the shipped comments that now assert it as fact — `SiegeNavDiagnostics.cpp:206` (*"would emit C4996 in a build where warnings are errors"*) and `SummonedUnit.cpp:2873-2874` (*"this build treats warnings as errors"*). ⛔ **No future task may cite it as established.**

### RULING 5 — **all three scope additions ACCEPTED.** Each verified against the code, not the argument.

**(a) TASK-532's lease-as-dispatch-suppression, not per-poll re-steer — ACCEPTED, and the law is on its side.**
`NAV-§3`'s rung table forbids the literal reading outright: Sidestep *"⛔ re-issue while the lease is live."* And the cost argument checks out at the artifact: `EnterAdvanceToLocation`'s gate is `if (bWasActorMove || bPointChanged || AI->GetMoveStatus() == EPathFollowingStatus::Idle)` (`SummonedUnit.cpp:2677`) — a sidestep goal the navmesh will not take leaves the status `Idle` every poll, so a per-poll re-issue really would fire **4 requests/unit/s for the whole 2 s lease**. Suppression is the correct implementation. ✅ Ordering verified: the watchdog runs at `:1348`, the lease early-out at `:1364` — the rung arms the lease and issues **before** the same poll's dispatch could cancel it. Identical, correct ordering on the miner (`MinerUnit.cpp:332` then `:357`).

**(b) Dropping the lease inside `WidenAndRepath`/`Abandon` — ACCEPTED, and it is a genuine save.**
Arithmetic confirmed: armed at 1.5 s for `SidestepLeaseSeconds = 2.0` ⇒ expires 3.5 s; `WidenSeconds = 3.0` fires **inside** the lease. Without `SidestepLeaseRemaining = 0.f` first (`SummonedUnit.cpp:3000`, `MinerUnit.cpp:1075`), `UpdateState`/`UpdateMining` would return early, the re-path would never reach a mover, and — because `EscalationLevel` is monotonic — **the rung could never fire again.** A silent no-op rung, caught by the author. Same for `Abandon` (`:3021` / `:1120`).

**(c) TASK-533's three additions — ACCEPTED, all three, and (i) is the same genuine save as (b).**
- **(i) the lease read in `UpdateMining` (`MinerUnit.cpp:357-360`).** Verified: `EnsureWalkingToNode` clears `bHasIssuedPointGoal` unconditionally at `:949` and re-issues `MoveToActor` when its goal check falls through (`:940-970`); `DriveToPoint`'s band suppresses only when `bMoveInFlight && bWithinBand` (`:904-910`), and a sidestep point ~350 uu off the order point fails the band. ⇒ **without the lease, the very next statement in the same call stack cancels the sidestep.** The rung would be vacuous. Ship it. (One honesty gap in its accounting — see WARN-7.)
- **(ii) `StopMovement()` in `WidenAndRepath` (`MinerUnit.cpp:1100-1103`).** Confirmed: `EnsureWalkingToNode`'s early-out reads `PathFollow && GetMoveStatus() != Idle && GetMoveGoal() == Node` and **never reads `bHasIssuedPointGoal`** — that flag is read only by `DriveToPoint`'s band at `:905`. The dispatch's stated mechanism does not reach that gate, and the author is right. `StopMovement` here is **the one driver cancelling its own request**, not a second driver.
- **(iii) `StandInPlace()` + guarded `SeekBestMine()` for `Abandon` (`MinerUnit.cpp:1125-1158`).** Confirmed: `UpdateMining`'s retarget gate fires only on `bTargetDead`, so a healthy-but-unreachable mine is never re-chosen and the board's *"`UpdateMining` re-targets on its own"* is false against shipped code. ⭐ The `!bArrivedAtNode` guard is the right call and is what keeps the rung out of the income path — without it, a null finder result would drive `LeaveMining()` → `RemoveMinerIncome` from a **stuck-ladder rung**.

**(d) TASK-529's const overload + actor-iterator fallback — ACCEPTED.** Verified: `FNavigationSystem::GetCurrent<T>(const UWorld*)` returns `const TNavSys*` (`NavigationSystemBase.h:121-124`) and is the only overload reachable from a `const UWorld*`; `GetDefaultNavDataInstance() const` (`NavigationSystem.h:759`) and every accessor the library calls are `const`; `TActorIterator` takes `const UWorld*` (`EngineUtils.h:582`). ⇒ **no `const_cast` in a library whose whole selling point is "provably read-only."** Correct instinct. The fallback is justified: route 1 returns null on `RegistrationFailed_AgentNotValid`, which is a named risk of this very batch.

### RULING 6 — **both holes SHIP, both as WARNs with named remedies. Neither blocks.**

**(a) The rung-0 `OR` speed clause (`SiegeStuckStatics.cpp:40-43`) — SHIPS.** ⚠️ The hole is real (a unit scraping/oscillating at ≥ 50 uu/s with zero net progress re-anchors every poll and is never rescued; TASK-536 test 18 measures 0 rungs in 60 s at 51 uu/s). **But the `OR` is what the spec ordered, character-for-character** (TASK-531 spec (2), `NAV-§3`'s `bAdvancing` bullet). Removing the speed term is an **architecture change**, and `NAV-§` says the architecture is not re-litigated at this gate. ⇒ WARN-5, with the remedy named: **lower `MinSpeedSq` — `EditDefaultsOnly`, no recompile — and it is on Jonathan's TASK-539 list explicitly.** ⚠️ If Jonathan reports *"units still wedge"* after the BLOCKER above is fixed, **this is the first thing to check.**

**(b) `NotifyMoveBlocked`'s clock bump (`SummonedUnit.cpp:3051-3078`) — SHIPS, and the honest label is "corroborating evidence, not a trigger."** TASK-536's reading is stronger than TASK-532's and it is right: a `Blocked` verdict means the request **finished**, so the next poll reads `!bAdvancing`, `Evaluate` re-anchors and `Reset`s, and the bump is wiped. **In the case it was written for, it contributes exactly nothing.** ⛔ It is **not** dead code — in the corroborating case (a live request on the next poll) it pulls the first rung forward by 0.75 s and still passes both brakes. And the timing argument holds: the engine declares `Blocked` at 5.0 s, by which point the 1.5/3.0 s rungs have already fired off our own clock. ⇒ **Keep it, correct the emphasis** (`SummonedUnit.h:320-331` and `NAV-§3`'s `NotifyMoveBlocked` bullet read as though it were a primary signal). **TASK-538 evidence signature:** `blocked:` lines with **no** `escalate:` line following on that unit = this hole, working exactly as documented.

---

## THE NAMED VERIFICATIONS

| ⛔ check | result |
|---|---|
| **`OnMoveCompleted` is EVIDENCE, not ACTION** | ✅ **PASS.** `SiegeNavAreas.cpp:88-162` contains **no** `MoveTo*`, `StopMovement`, `RequestMove`, `SetFocus` or any steering call. `Super::OnMoveCompleted` is first and unconditional (`:99`); `Blocked`-only filter (`:105`); null-safe `GetPawn()` + cast (`:115-119`); `Unit->NotifyMoveBlocked()` is the **last statement** and is **outside** the throttle block (`:161`) — the log is rate-limited, the evidence feed is not. ⭐ **`TickStuckWatchdog` is the only thing in the batch that steers.** |
| **No new timers in the watchdog lane** | ✅ **PASS.** `TickStuckWatchdog`, `HandleStuckEscalation`, `NotifyMoveBlocked`, `ConsumeStuckDeltaSeconds` and both override bodies contain **no `SetTimer`**. Both drivers ride existing polls: `StateTimerHandle` (`SummonedUnit.cpp:1348`) and `MiningPollTimerHandle`/`ArrivalCheckInterval` (`MinerUnit.cpp:332`). |
| **TASK-535's `DefinitiveCheckTimerHandle`** | ✅ **RULED IN — accept.** `NAV-§3`'s zero-timers law is written against *"a second 0.25 s driver on one `UPathFollowingComponent`"*; `ASiegeBattlefieldScatter` owns no movement component and is already timer-driven (`TraversabilityTimerHandle`). The new handle is a **one-shot 1 ms deferral** that never re-arms itself (`BattlefieldScatter.cpp:2379-2381`), cleared on `EndPlay` (`:248`) and on every re-scatter (`:464`), and kept separate from the settle-poll handle for a stated reason (both can legitimately be in flight). ⭐ **The alternative is worse:** culling HISM instances inline from **inside the Recast generator's own tick** re-enters the nav system it is dirtying. See WARN-8 for the wording fix. |
| **The miner's four seals** | ✅ **ALL INTACT.** `StateCheckInterval = 0.f` (`MinerUnit.cpp:62`) · `AggroRadius = 0.f` (`:71`) · `ClearAllTimersForObject(this)` after `Super::BeginPlay` (`:150`) · `CanEverAttack() const override { return false; }` (`MinerUnit.h:354`). `AMinerUnit::FreezeAI` (`:243-255`) is unmodified: `Super::FreezeAI()` (which now also zeroes the lease and `Reset`s the state, `SummonedUnit.cpp:697-698`) then `ClearTimer(MiningPollTimerHandle)`. **Income/registration/wait-mode/eviction/cap: no rung touches `TryRegisterArrivedMiner`, `AddMinerIncome`/`RemoveMinerIncome`, `EndMineTenure`, `bIncomeActive`, `NotifyMineDepleted` or the §3.3 cap.** The only writes the rungs make are `SidestepLeaseRemaining`, `bHasIssuedPointGoal` and `TargetGoldNode` via the tenure-guarded `SeekBestMine()`. |
| **`ConsumeStuckDeltaSeconds()`, never `StateCheckInterval`** | ✅ **PASS, checked on the miner path specifically.** `MinerUnit.cpp:332` passes `ConsumeStuckDeltaSeconds()`. `StateCheckInterval` appears in the miner diff **only inside the comment at `:316`**, never as a read. The helper reads the world clock (`SummonedUnit.cpp:2867-2891`), returns 0 on the first call (`:2879-2883`) and clamps both ways. ⭐ **The failure this avoids is silent:** a 0 delta accumulates nothing ⇒ `DesiredLevel` stays 0 ⇒ brake 2 returns `None` **forever**, with no crash and no log — on exactly the class most likely to walk into a rock. |
| **`DeltaSeconds == 0` is inert on both paths** | ✅ **PASS.** `SiegeStuckStatics.cpp:60` — `(DeltaSeconds > 0.f) ? DeltaSeconds : 0.f`. ⭐ Written as a `>` ternary rather than `FMath::Max` **on purpose**, so NaN lands on 0 too. Nothing in `Evaluate` divides by the delta. `TickStuckWatchdog` re-clamps at `SummonedUnit.cpp:2898`. |
| **Determinism (`TASKBOARD.md:11044`)** | ✅ **PASS, checked by reading, not by accepting the claim.** Every seeded body is untouched: `ScatterLayer`, `PlaceMines`, `PlaceAncientGrounds`, `RegroundMines`, `IsInKeepClear*`, `GroundZAt`, `ResolveHillAwareGroundZ`, `ResolveCastleLocation`, `ClearScatter`, `CullCorridorBlockers` and `RemoveBlockingInstancesInDisc` bodies. **Not one `FRandomStream` construction, draw or draw order moved.** TASK-535's only edit inside `RunScatterPasses` is a latch/timer/bind block at `:452-468`, **after** the two layer passes, `PlaceMines`, `PlaceAncientGrounds` **and** the `!bAuthoritativeGenerate` early-return (`:439-445`) — i.e. after the last draw, on the authority path only. |
| **`NAV-§4`'s settled-only cull has ONE entrance** | ✅ **PASS.** `BattlefieldScatter.cpp:2175-2188` is the only gate and it sits directly above `++ReachabilityAttempt` (`:2194`); everything destructive is below it. `CullCorridorBlockers` has exactly one call (`:2203`) and `RemoveBlockingInstancesInDisc` exactly one on this path (`:2222`) — both under the gate. The other two `RemoveBlockingInstancesInDisc` calls (`:1583-1584`, `:1860-1861`) are **seed-deterministic placement clearance** inside `PlaceMines`/`PlaceAncientGrounds`, not nav-driven culls, and are untouched. ⛔ A suppressed pass does **not** consume `ReachabilityAttempt` and arms **no** re-poll. |
| **`bCullOnProvisionalFailure`** | ✅ `UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability")`, **default `false`** (`BattlefieldScatter.h:686-687`). |
| **No `CONFIRMED` on a pre-settle query** | ✅ **PASS.** The word is spent only under `if (bNavSettled)` (`:2131-2135`); every other verdict prints `PROVISIONAL (… PRE-SETTLE query; …)`, including both terminal `Error` branches (`:2279`). |
| **Delegate: bound once, unbound on `EndPlay`, re-run cannot re-enter** | ✅ **PASS.** `AddUniqueDynamic` behind the `bNavGenerationFinishedBound` latch + authority + null guards (`:2288-2328`); `RemoveDynamic` through a `TWeakObjectPtr` so a torn-down nav system is forgotten (`:2330-2342`); `UFUNCTION() void OnNavGenerationFinished(ANavigationData*)` (`BattlefieldScatter.h:505-506`) — a dynamic delegate **requires** the `UFUNCTION`, and it is there. Re-entrancy is closed by the `bDefinitiveCheckDone \|\| bDefinitiveCheckPending` pair (`:2349`) plus the 1 ms deferral; `EndPlay` clears **both** handles and unbinds unconditionally (`:244-256`); every terminal verdict discharges the bind (`:2146`, `:2266`, `:2284`). |
| **No `.umap` write / no `L_Arena` edit / no `Build.cs` change** | ✅ **PASS as far as source can show it.** No save/package API anywhere in the batch. `GitClaudeUnrealTest.Build.cs` is unchanged by this batch — `AIModule` (`:17`) and `NavigationSystem` (`:18`) were already public deps and the file's newest comment still cites TASK-443. ⚠️ **The `L_Arena` SHA256 is TASK-538's check; I have no shell and cannot hash it.** |
| **No runtime mutation of the live `ARecastNavMesh` config (`NAV-§5`)** | ✅ **PASS.** `FSiegeNavDiagnostics` holds every nav pointer as `const` (`SiegeNavDiagnostics.cpp:114`, `:167-168`, `:239`, `:264`) and calls only `const` accessors — the route is closed by the type system, not by discipline. Module-wide grep for `AgentRadius =` / `TilePoolSize =`: the only hit is pre-existing template code (`Variant_Platforming/PlatformingCharacter.cpp:61`, `NavAgentProps.AgentRadius`, unrelated). |
| **The three ruled-out theories stayed ruled out (`NAV-§2`)** | ✅ **PASS.** No `AgentRadius` change (ini `:266-334` — only the two intended keys moved). No `.umap`. **Zero hits** module-wide for `UCrowdFollowingComponent`, `bUseRVOAvoidance`, `SetAvoidanceEnabled`. `TilePoolSize=1024` (`DefaultEngine.ini:332`) and `bFixedTilePoolSize=True` (`:331`) **unchanged** (`NAV-§6`: instrument, do not fix — and the instrument is there). |
| **TASK-530's comment carries the conditional + the ladder** | ✅ **PASS.** `DefaultEngine.ini:290-328` carries the `RecastNavMeshGenerator.cpp:5892-5896` citation **with the engine's own comment quoted verbatim** (`:295-299`), the measured `326 tiles / ~216 s / ~1.5 tiles/s` with its provenance (`:300-303`), the `:5465` reasoning for the explicit 8 (`:307-313`), the **8 → 4 → revert both keys** ladder (`:322-327`), and the conditional in terms: *"if the PIE log's `gatherOnGameThread=` still reads false … REVERT both keys. Do NOT escalate to an L_Arena save"* (`:319-321`). ⭐ **And the `:281-284` trap note is byte-unchanged and kept its exact line numbers**, so every existing citation to it still resolves. |
| **Pinned-registry conformance (`NAV-§8`), access levels included** | ✅ **PASS, all 14 symbols.** `NotifyMoveBlocked()` **public** (`SummonedUnit.h:332`, nearest specifier `public:` `:122`) · `TickStuckWatchdog` `:906`, `HandleStuckEscalation` `:923`, `ConsumeStuckDeltaSeconds` `:933`, `StuckState` `:936`, `StuckTuning` `:940` (`UPROPERTY(EditDefaultsOnly, Category="Siegebound|Stuck")`), `SidestepGoal` `:943`, `SidestepLeaseRemaining` `:956`, `LastStuckTickTimeSeconds` `:959` — all **protected** (`protected:` `:593`, `private:` `:961`) · `AMinerUnit::HandleStuckEscalation … override` **protected** (`MinerUnit.h:474`, `protected:` `:418`) · `ASiegeUnitAIController::OnMoveCompleted` **public** (`SiegeNavAreas.h:154`) · both static libraries exactly two/three `public` statics, `GITCLAUDEUNREALTEST_API`, not `UObject`s · `LogSiegeNavDiag` ⛔ not `LogSiegeNav` (`SiegeNavDiagnostics.h:18`) · `LogSiegeStuck` (`SiegeStuckStatics.h:73`) · `ESiegeStuckAction` plain enum, `FSiegeStuckState` unreflected POD, `FSiegeStuckTuning` `USTRUCT(BlueprintType)` with all eight `EditDefaultsOnly` fields at the pinned defaults. |
| **M8 declaration present verbatim** | ✅ **PASS, 8/8 handoffs + both new headers** (`SiegeNavDiagnostics.h:56-59`, `SiegeStuckStatics.h:54-58`). |
| **The lease lifecycle — traced individually (`SC-§21`)** | ✅ **PASS, five of five**, each read at the site, each paired with `FSiegeStuckStatics::Reset(StuckState)`: **1** `EnterIdle` `SummonedUnit.cpp:2710-2711` (after the `State == Idle` early-out — correct: arming goes through `EnterAdvanceToLocation`, which sets `State = Advance`) · **2** `EnterAttack` `:2472-2473` (inside the `State != Attack` transition, beside the existing `CurrentMoveGoal = nullptr`) · **3** `HandleDeath` `:3466-3467` (a rigged corpse lingers up to `DeathAnimMaxHoldSeconds`) · **4** `FreezeAI` `:697-698` · **5** `ApplyFreeze` `:942-943`. ⭐ **The fifth is ACCEPTED and is a genuine catch:** `ApplyFreeze` writes `State = Idle` **directly** (`:928`) so it never routes through `EnterIdle`, and `EnterIdle`'s own early-out (`:2696`) means a later call could not clean up either — **and it clears `StateTimerHandle`, so the lease cannot even drain.** Without it a thawing unit steers to a pre-freeze `SidestepGoal`. Keep it. **Armed once** (`:2989`, before the move, so state is consistent whatever the request returns); **drained once per poll**, unconditionally and before `Evaluate` (`:2903-2906`). |
| **`TestEqualSensitive` on `FString` claims (`SC-§13`)** | ✅ **PASS — correctly not applicable.** `SiegeStuckStaticsTest.cpp` asserts **no `FString` value**; the library under test has no string surface. Every string is `What`-parameter message text. Every float claim passes an explicit tolerance (`ClockTolerance` 1e-4, `GeometryTolerance` 0.01) and every `FVector` claim uses the tolerance overload. Confirmed the overloads exist in 5.8 (`AutomationTest.h:2029-2049`). |
| **Standing C++ sweep** | ✅ **PASS.** Complete-type includes named per-symbol in all five `.cpp`s (`SiegeNavDiagnostics.cpp:5-39` is exemplary — one comment per include saying which symbol it is for) · `SummonedUnit.h:10` and `MinerUnit.h:6` correctly include `SiegeStuckStatics.h` for **by-value members and an enum parameter**, not transitively · no most-vexing-parse (the two parenthesised locals in `SiegeNavDiagnostics.cpp` take variables, never types) · no shadowing of inherited reflected members — the new locals (`Delta`, `Now`, `Action`, `StalledGoal`, `Attempt`, `StalledAtEscalation`, `bAdvancing`, `AI`, `Node`, `SidestepPoint`, `TelemetryWorld`) collide with nothing on either hierarchy · every `switch` on `ESiegeStuckAction` is total with an explicit `None` arm · no delegate broadcast on a no-op · **no per-tick logging at default verbosity** (telemetry = 4 `Log` lines/match, every guard path `Verbose`; `escalate:` is structurally throttled by the two brakes to ≤3 lines/stall; `blocked:` is throttled per unit at 10 s). |
| **Engine-API validity for UE 5.8** | ✅ Spot-verified every non-obvious call against installed source: `ShouldGatherDataOnGameThread()` `RecastNavMesh.h:1499` · `GetMaxSimultaneousTileGenerationJobsCount()` `:1280` · `GetCellSize(…)` `:1107` · `GetNavMeshTilesCount()` `:1168` · `GetNumActiveTiles()` `:1170` · `GetRuntimeGenerationMode()` `NavigationData.h:658` · `GetNumRemainingBuildTasks/RunningBuildTasks/DirtyAreas/HasDirtyAreasQueued` `NavigationSystem.h:1106/:1109/:977/:976` — **all `const`**, so the const-pointer library compiles · `FPathFollowingResult::Code` / `::ToString() const` `PathFollowingComponent.h:124/:136` · `FObjectKey::ResolveObjectPtr()` `ObjectKey.h:126` · `IsInGameThread()` `CoreGlobals.h:703` · `AI/Navigation/NavigationDataResolution.h` exists. **No deprecated or removed API is used anywhere in the batch.** |

---

## Findings

### BLOCKER

- **[BLOCKER] `SummonedUnit.cpp:2978-2979` + `MinerUnit.cpp:1037-1038`** — `Attempt` is a per-unit constant, so the spec-required sidestep alternation is unreachable and rung 1 is a permanent no-op for units whose fixed side is blocked. **Full analysis and the ~5-line fix are at the top of this report.** Owners: TASK-532 (`SummonedUnit.{h,cpp}`), TASK-533 (`MinerUnit.cpp`, one line), TASK-536 (invert test 19).

### WARN

- **[WARN-1] `SiegeStuckStatics.cpp:107-134` and `SiegeStuckStatics.h:120-124`** — *"EITHER BRAKE ALONE BOUNDS THE REQUEST RATE"* is false; brake 2 bounds the **count per stall**, not the rate. **Fix:** state that the `≤1/unit/s` ceiling rests on `EscalationCooldown ≥ 1.0` alone, and that the worst case over legal tuning is **2.0/unit/s** (derivation in RULING 2). Owner TASK-531; land it in the BLOCKER loop. **Also owed by the manager:** `NAV-§3` (`CONVENTIONS.md:2479-2482`) and `handoffs/TASK-531-programmer.md` §2.
- **[WARN-2] `CONVENTIONS.md:2454` (`NAV-§1` cause 3) and `:2685` (`NAV-§12`)** — the stated mechanism is wrong; `IsNavigationBeingBuilt()==false` **proves** `GetNumRemainingBuildTasks()==0` (unless nav building is permanently locked). The real route is the `MaxNavSettleWait` 10 s cap, confirmed 4/4 in the log. **Manager correction, before the checkpoint.** The conclusion and the fix both stand.
- **[WARN-3] `CONVENTIONS.md:2639` (`NAV-§8`), `TASKBOARD.md:7124`, `SiegeNavDiagnostics.cpp:206`, `SummonedUnit.cpp:2873-2874`** — *"warnings are errors in this build"* is not established (no `bWarningsAsErrors` anywhere in `Source/`). **Correct the law and the two shipped comments** so no future task cites it. No code behaviour depends on it.
- **[WARN-4] `CONVENTIONS.md` `NAV-§8`'s `SiegeNavDiagnostics.h` block / `NAV-§7` token list / `TASKBOARD.md:6997`** — the instruction to read the live `CellSize` is unfollowable (`RecastNavMesh.h:710-712`, `UE_DEPRECATED(all)`). **Amend to `GetCellSize(ENavigationDataResolution::Default)`.** TASK-529 already ships the right call.
- **[WARN-5] `SiegeStuckStatics.cpp:40-43`** — the rung-0 speed clause is an `OR`; a unit scraping a collider at ≥ `MinSpeedSq` with zero net progress is never rescued. Ships as specified. **Add it explicitly to TASK-539's watch list**, with the remedy: lower `MinSpeedSq` (`EditDefaultsOnly`, no recompile). ⚠️ First thing to check if wedging survives the fix.
- **[WARN-6] `SummonedUnit.cpp:3051-3078` and `SummonedUnit.h:320-331`** — `NotifyMoveBlocked` is inert in exactly the case it was written for. Ships; it is corroborating evidence, not a trigger. **Soften the header's and `NAV-§3`'s emphasis.** TASK-538's signature for it: `blocked:` with no following `escalate:` on that unit.
- **[WARN-7] `MinerUnit.cpp:357-360`** — the lease early-out returns **above** `ResolveMinerOrder()`, so an order arriving during a live lease also delays **`LeaveMining()`** — i.e. the end of a mine tenure, and therefore `RemoveMinerIncome` — by up to `SidestepLeaseSeconds` (2 s). The handoff's "what it delays" list (§5A) names only the at-the-ring test and **omits this.** Bounded, self-healing on the next poll, no latch or invariant broken (≤ ~2 gold), and only for a miner that is demonstrably wedged — **so it ships** — but the accounting must be corrected in the handoff so the next reader does not re-derive it as a defect.
- **[WARN-8] `CONVENTIONS.md:2485` (`NAV-§3`'s "⛔ ZERO NEW TIMERS")** — as written it reads batch-wide and TASK-535's one-shot `DefinitiveCheckTimerHandle` technically contradicts it. **Scope the clause to the watchdog lane** (which is what its own justification says), and record the deferral as a declared, bounded exception.
- **[WARN-9] downstream greps** — the bare `Traversability CONFIRMED` string no longer exists; every verdict now carries `(nav settled: 0 pending)` or `PROVISIONAL`. Intended and spec-mandated, but **any tool, doc or habit grepping the bare string is now silently empty.** TASK-538's evidence list is already correct; flagging it for the M6.5 traversability convention text.
- **[WARN-10] `BattlefieldScatter.cpp:2085`** — `bNavSettled` proves *the queue is empty now*, not *the navmesh is final*. TASK-535 flags this honestly (§6.5) and the design's self-protection (a cull re-dirties nav ⇒ the next pass is provisional ⇒ cull-free) is sound. **Recorded, not actioned** — the alternative is polling, which the spec forbids. On Jonathan's list via the late-cull flag.

### NIT

- **[NIT-1] `SiegeStuckStatics.cpp:98-101`** — **this is the stale comment**, not TASK-532's clamp. It describes a single large delta jumping level 0 → 3 as *"the honest reading"*, but no shipped caller can now produce one (`MaxStuckDeltaSeconds = 1.f`, `SummonedUnit.cpp:103`, applied at `:2889`). Both are correct in their own scope; add one clause noting the callers clamp. ⛔ Do not "fix" one to match the other.
- **[NIT-2] `SiegeNavAreas.h:154`** — no `using Super::OnMoveCompleted;`. **Correct as shipped** (C4263/C4264 are off by default). Recorded only so TASK-538 has the one-line remedy if MSVC surprises us. ⛔ Never implement the deprecated overload.
- **[NIT-3] `SummonedUnit.cpp:1364-1367`** — the lease early-out also suppresses target acquisition and the attack dispatch for up to 2 s, so a sidestepping unit is briefly deaf. Deliberate and bounded; recorded so it is not "found" at the playtest as a bug.
- **[NIT-4] `SummonedUnit.cpp:2947`** — the pinned `level=` token logs the **action ordinal**, not `StuckState.EscalationLevel`. They are equal by construction and the ordinal survives `Abandon`'s internal `Reset`, so this is right — but TASK-538 should read it as the rung index, and TASK-536 test 2 is what keeps the two aligned.
- **[NIT-5] citation drift** — the miner seal is `MinerUnit.cpp:62` (board says `:61`); the watchdog call site is `SummonedUnit.cpp:1348` (board says `:1285`); `EnterAdvanceToLocation`'s null of `CurrentMoveGoal` is `:2675` (law says `:2581`). Cosmetic; grep the symbol, never the line.

---

## What I could check ONLY statically

Everything. Nothing compiled, nothing ran, no navmesh observed. Specifically **unverifiable at this gate**: whether the ini reaches `L_Arena`'s serialized `ARecastNavMesh` (`NAV-§9`'s whole conditional); the actual values of every pinned token; whether the settle behaviour changes at 8× concurrency; the `L_Arena` SHA256; whether `Siegebound.Nav.Stuck.*` compiles or passes; and the entire lease/rung **action** layer, which TASK-536 correctly states it cannot reach headlessly (`handoffs/TASK-536-programmer.md` §5). ⚖️ TASK-536's honesty about its own coverage boundary is the best artifact in this batch and I am adopting its §5 verbatim as the list of what remains unproven.

---

## Notes for build-master (TASK-538) — ⛔ NOT YET; this gate is FAILED

⛔ **Do not start.** TASK-538 is blocked on a **PASS**. When the BLOCKER lands and the diff-scoped re-gate returns PASS, this is what to watch:

**At compile**
1. ⭐ **`SiegeStuckStatics.h` is the batch's first new `USTRUCT` in a non-`UCLASS` header** — a UHT failure here (`FSiegeStuckTuning`, `SiegeStuckStatics.generated.h` at `:12`) is the highest-probability first error, and it is **this batch's**, not foreign.
2. **`SummonedUnit.h` and `MinerUnit.h` now include `SiegeStuckStatics.h`** — a UHT ordering error in either is this batch's.
3. **C4263/C4264 or `-Woverloaded-virtual` on `ASiegeUnitAIController`** ⇒ add `using Super::OnMoveCompleted;` in its `public:` block. ⛔ **Never** implement the deprecated overload. That fix is CODE and owes an `SC-§27` diff-scoped verdict.
4. ⚠️ **Warnings are NOT errors here** (WARN-3) — a C4996 or C4244 will **not** fail the build. Report them anyway; a deprecation warning in this batch means somebody reached for a deprecated API and that is a finding.
5. ⛔ **Diagnostic attribution:** `Plugins/SiegeLlama/` compiles inside this target. An error naming a file this batch does not own is FOREIGN.
6. ⛔ Parse the log for `Result: Failed`; **never** trust `$LASTEXITCODE`. A ~2 s failure is Smart App Control, not a code error.

**At the suite**
7. Report all `Siegebound.Nav.Stuck.*` **individually by name**. ⭐ Name **`ImmediateReEvaluationIsRefused`** (the anti-mill assertion the cost claim rests on) explicitly. Also name **`EscalationCooldownGatesTheLadder`** and **`MonotonicLevelHoldsWithoutTheCooldown`** — they are the only two that make brake 1 and brake 2 independently load-bearing, and after WARN-1 they are the tests that carry the corrected claim.
8. ⚠️ **`SidestepSideIsConstantForOneUnit` must have been INVERTED by the BLOCKER fix.** If it is still green **asserting constancy**, the fix did not land — stop and route back.
9. Whole-suite count against the baseline **53**.

**At the PIE run (ASK first; an ini change needs an editor RESTART to take effect at all)**
10. ⭐ **`gatherOnGameThread=`** — quote the whole `nav-config:` line verbatim. `false` ⇒ **TASK-540 branch A**, revert both keys. ⛔ Not an `L_Arena` save.
11. `nav-build [post-scatter]: remaining= running=` — expect `running=8` if concurrency is live. Also capture `[pre-scatter]` and `[at-confirmation]`.
12. ⭐ **`[at-confirmation]`'s `dirtyAreas=` / `hasDirty=` are what close `NAV-§12`'s open question** — record them, they are the numbers WARN-2's correction rests on.
13. `activeTiles=` vs `poolCap=`, **and grep `tile limit reached!`** — `NAV-§6`'s first-ever instrument. ⚠️ `activeTiles=-1 poolCap=-1` means **unavailable**, never zero.
14. Settle wall-clock against the **216 s** baseline. ⛔ Record the number; do not gate on a target.
15. Exactly **one** `PROVISIONAL` at ~+5 s and **one** `CONFIRMED (nav settled: 0 pending)` later, the second carrying ` [definitive: OnNavigationGenerationFinished]`. ⛔ **A `CONFIRMED` with a non-zero `remaining=` beside it in the `at-confirmation` snapshot is a FAIL of `NAV-§4` and must be reported, not smoothed.**
16. `LogSiegeStuck` — every `escalate:` (`level=` `action=` `stalled=`) and every `blocked:`. ⭐ **`blocked:` lines with no `escalate:` following on the same unit are WARN-6 behaving exactly as documented — record them, do not report them as a bug.**
17. 🔒 `L_Arena` SHA256 before and after, both pasted. A PIE session dirties packages; `NAV-§5` and the editor-close discipline govern.
18. Stage by explicit file path (GIT HAZARD LAW (d)); ⛔ **never push.**

---

## Board status — ⚠️ I CANNOT WRITE IT

I have no partial-edit tool and `TASKBOARD.md` is an 11k-line hot shared file with a write-race history; a whole-file rewrite is not an acceptable risk. **Please proxy these flips:**

- **TASK-537** → `qa-failed` (report `.claude/pipeline/qa/TASK-537.md`, 1 BLOCKER)
- **TASK-532** → `qa-failed` — owns the BLOCKER fix (`SummonedUnit.{h,cpp}`)
- **TASK-533** → `qa-failed` — one-line follow (`MinerUnit.cpp:1037-1038`)
- **TASK-536** → `qa-failed` — invert test 19 (`SiegeStuckStaticsTest.cpp:1519-1594`)
- **TASK-531** → `qa-failed` — WARN-1 comment correction (`SiegeStuckStatics.cpp:107-134`, `.h:120-124`); no behaviour change
- **TASK-529 · 530 · 534 · 535** → `qa-passed` — **no defect found in any of them**; their findings are law/wording corrections owed by the manager, not code edits
- **TASK-538** stays `backlog` — blocked until the re-gate returns PASS
- **Manager corrections owed before the checkpoint:** `NAV-§1` cause 3 · `NAV-§3`'s rate ceiling · `NAV-§8`'s `CellSize` and warnings-as-errors bullets · `NAV-§12`'s answered limitation

---
---

# ══════════════════════════════════════════════════════════════
# LOOP 1 — DIFF-SCOPED RE-GATE (`SC-§27`)
# ══════════════════════════════════════════════════════════════

## Verdict: **PASS** — 0 BLOCKER · 0 new WARN · 1 new NIT

**⛔ Everything above this banner is the loop-0 record and is unchanged.** This section supersedes only the loop-0 *board status* and the loop-0 *build-master watch list* where the two disagree; every loop-0 WARN and NIT that was not re-opened still stands.

**Scope, per `SC-§27`:** the changed lines plus the fences the fix must not have disturbed. ⛔ I did **not** re-review TASK-529 · 530 · 534 · 535, and I confirmed they were not reopened (fence F8). ⛔ Nothing that passed at loop 0 was re-litigated.

**Files in scope (5), and they are exactly the four tasks I failed:** `SummonedUnit.h` · `SummonedUnit.cpp` · `MinerUnit.cpp` · `SiegeStuckStatics.{h,cpp}` (comment-only) · `Tests/SiegeStuckStaticsTest.cpp`. `MinerUnit.h` correctly **not** reopened — the counter is inherited.

---

## RULING A — **the fix works.** Traced, not accepted.

**The producer is now free-running and nothing clears it.** Module-wide grep for `SidestepAttemptCount` returns exactly: the declaration `SummonedUnit.h:980`, the two post-increments (`SummonedUnit.cpp:2992`, `MinerUnit.cpp:1044`), and the test model. ⭐ **There is no assignment to it anywhere** — not in `EnterIdle`, not in `EnterAttack`, not in `HandleDeath`, not in `FreezeAI`, not in `ApplyFreeze`, and — the one that mattered — **it is not reachable by `FSiegeStuckStatics::Reset`, because it is not a field of `FSiegeStuckState`.** Surviving the `Reset` *is* the property, and it now holds structurally rather than by discipline.

**Per-unit alternation.** Brake 2 still lets `Sidestep` fire exactly once per stall, so the counter advances exactly once per stall. Successive stalls on one unit therefore see `Attempt = k + (uid % 2)` for `k = 0, 1, 2, …` ⇒ **the parity flips every stall** ⇒ `ComputeSidestepGoal`'s `((Attempt & 1) == 0) ? +Lateral : -Lateral` (`SiegeStuckStatics.cpp:212`) returns exact mirrors on consecutive stalls. **A unit wedged on a rock's left face now tries the right on its next stall.** The defect is closed at its root.

**Neighbour de-correlation survives, and I checked the interaction rather than assuming it.** `uid % 2` is a **fixed per-unit phase offset**, and adding a constant to both terms of an alternating sequence preserves the *difference* in parity. So two units of opposite id parity, stepping in lockstep, are on opposite sides at **every** attempt index. ✅ Both halves now work simultaneously — which was the one way this fix could plausibly have broken something, and it does not.

**The `uint8` wrap is safe, and the reasoning in the member comment is correct.** 255 is odd, 0 is even, so the parity flips across the boundary exactly as it does everywhere else; only `Attempt & 1` is ever read. ⭐ **The prohibition the comment attaches to it is the right one** — *"nothing may depend on this value monotonically"* — and I verified it is currently true: the value is never compared with `<`, `>` or a threshold, never logged, never reported.

**One thing I checked that nobody asked about:** the miner increments the counter **before** its `Dist2D > ArrivalRadius` lease guard (`MinerUnit.cpp:1044` vs `:1058`), so a sidestep that turns out to be a no-op still advances the counter. ✅ **That is the right order** — a spent attempt should move the side on, or a miner with a degenerate `SidestepDistance` would retry the same useless side forever. Deliberate or not, it is correct.

## RULING B — **the rename is ACCEPTED. Explicitly, and TASK-538 greps the new name.**

`Siegebound.Nav.Stuck.SidestepSideIsConstantForOneUnit` → **`Siegebound.Nav.Stuck.SidestepSideAlternatesAcrossStallsForOneUnit`** (`SiegeStuckStaticsTest.cpp:1551`; class `FSiegeStuckAttemptAlternatesTest`).

I did not prescribe it and the programmer is right that I should rule on it. **The argument is sound and it is my own argument:** a test named `SideIsConstant` that asserts alternation is precisely the "name describing behaviour that is not what happens" defect I blocked the batch on. Keeping the old name to protect a line in my watch list would have been the tail wagging the dog.
✅ **And the landing signal is genuinely stronger:** the old *registered* name is now absent from the suite entirely, so a stale runner reports a **missing test** rather than a green one that means the opposite of its name.
⚠️ **One precision for TASK-538:** the old string still appears in **two comments** (`:50`, `:1521`) that record the history deliberately — ⛔ so a `grep` of the *source* for `SidestepSideIsConstantForOneUnit` will hit twice and that is **not** a failure. **Grep the automation runner's output, not the file.** The names to look for in the run are in the watch list below.

## RULING C — **the fences are intact.** Each re-verified at today's line numbers.

| # | fence | result |
|---|---|---|
| **F1** | **`NAV-§8` pin — no signature, parameter or access-level change** | ✅ `Evaluate` / `ComputeSidestepGoal` / `Reset` unchanged; `NotifyMoveBlocked` still **public** (`SummonedUnit.h:332`, `public:` `:122`); the watchdog trio and every pinned member still **protected** (`protected:` `:593` … `private:` `:985`); `AMinerUnit::HandleStuckEscalation … override` untouched (`MinerUnit.h:474`, header not reopened). |
| **F2** | ⛔ **No field added to `FSiegeStuckState` or `FSiegeStuckTuning`** | ✅ **The single most important fence, and it holds.** `FSiegeStuckState` still has exactly its five pinned fields (`SiegeStuckStatics.h:102-109`); `FSiegeStuckTuning` still has exactly its eight `EditDefaultsOnly` fields at the pinned defaults (`:135-163`). ⭐ **The new counter is deliberately outside both** — which is not merely pin-compliance, it is the fix. |
| **F3** | **`SidestepAttemptCount` itself** | ✅ **protected** (`:980`, inside `593…985`), **unreflected** (no `UPROPERTY`) ⇒ the `NAV-§11` M8 declaration still holds structurally: it cannot be replicated by accident. Not an `FSiegeStuckTuning` field ⇒ `NAV-§7`'s "these eight are the only tunables" is intact. |
| **F4** | **The five lease-clear sites** | ✅ **All five, each still paired with `Reset(StuckState)`:** `FreezeAI` `697-698` · `ApplyFreeze` `942-943` · `EnterAttack` `2472-2473` · `EnterIdle` `2710-2711` · `HandleDeath` `3480-3481` *(shifted from `:3466` by the comment growth — the site itself is unchanged)*. Plus the two rung-internal clears at `:3014` (Widen) and `:3035` (Abandon), still **first** in their branches. |
| **F5** | **Both brakes** | ✅ **Byte-identical logic.** Brake 1 `SiegeStuckStatics.cpp:78-81`; the high-to-low rung selection `:88-91`; brake 2 `:102-105`; the level/cooldown writes `:152-153`; `Abandon`'s internal `Reset` `:169`. Only comments moved. |
| **F6** | **The watchdog call site + the lease plumbing** | ✅ `TickStuckWatchdog(ConsumeStuckDeltaSeconds())` still at `SummonedUnit.cpp:1348`, still immediately after `TrackChargeMovement()` and still below the freeze/death early-out; the lease early-out still at `:1364`; the drain still at `:2903`. Miner: `:332` then `:357`, unchanged. |
| **F7** | **`NotifyMoveBlocked`** | ✅ Body unchanged (`:3065-3092`, shifted); still no move, still no rung call, still the `FMath::Max` bump only (`:3091`). |
| **F8** | **The four passed files were never reopened** | ✅ `SiegeNavDiagnostics.{h,cpp}` and `BattlefieldScatter.{h,cpp}` contain **no** reference to TASK-537 at all. `SiegeNavAreas.{h,cpp}`'s two TASK-537 mentions (`cpp:138`, `h:161`) are **verbatim the loop-0 text I already read** — pre-existing, not new. `Config/DefaultEngine.ini` is unchanged: both keys still at `:333`/`:334`, `TilePoolSize=1024` at `:332`, `bFixedTilePoolSize=True` at `:331`, no TASK-537 mention. |
| **F9** | **Miner seals + income boundary** | ✅ `StateCheckInterval = 0.f` `:62` · `AggroRadius = 0.f` `:71` · `ClearAllTimersForObject` `:150` · `CanEverAttack()` `MinerUnit.h:354`. **The only `SetTimer` in the file is the pre-existing `MiningPollTimerHandle` at `:178`** — still zero new timers. The loop-1 diff touches only the `Attempt` expression; no income symbol is within ten lines of it. |
| **F10** | **Determinism / single cull entrance** | ✅ Untouched by construction — `BattlefieldScatter.cpp` was not opened, and nothing in the loop-1 diff reaches a `FRandomStream` or a cull. |
| **F11** | **Compile surface of the diff** | ✅ No new include needed anywhere; `static_cast<int32>(uint8)` and post-increment on a `uint8` member are unremarkable; the member is accessible from `AMinerUnit` (protected on the base); the test helper's signature change to `(uint8&, uint32)` has **no stale caller** — all seven call sites are inside test 19. |

**Test-19 arithmetic, hand-checked against the shipped expression:** `PretendUniqueID = 12345678` (even ⇒ id term 0) ⇒ Attempts `[0, 1, 2]` across three real stall cycles ⇒ parities `even, odd, even` ⇒ goals mirror pairwise and goal[2] == goal[0] ✅ exactly what `:1609-1628` asserts. The wrap block's ordering is right: two calls take 254→255→**0**, and the `NearWrap == 0` assertion at `:1641` sits between that pair and the next ✅. The lockstep neighbour loop `:1684-1691` is the interaction check I wanted, and it is asserted rather than argued.

---

## The manager's two points

### 1. ⭐ **Agreed on the datum — and I can narrow the second route further than "unknown". The manager was right to check.**

**Verified first-hand, not relayed.** `Saved/Logs/GitClaudeUnrealTest.log`: `GenerateScatter seed=364587905` at `00.44.39:632` (`:1891`) and `Traversability CONFIRMED` at `00.44.44:659` (`:1977`) = **+5.027 s**, with **zero** matches for `MaxNavSettleWait` or `still building`, case-insensitive, in the whole file. ⇒ **The cap did not fire there.** Two sessions, two routes; my loop-0 RULING 3 explained one of them, and the manager is correct that it does not explain the cited datum.

**⇒ But I can rule OUT the permanent-lock candidate for that session, from the timing alone.** `StartNavSettlePoll` re-polls `IsNavigationBeingBuilt(World)` and only falls through to `ValidateTraversability()` when it reads **false** or the cap trips (`BattlefieldScatter.cpp:2001-2023`). If `IsNavigationBuildingPermanentlyLocked()` were true, `IsNavigationBeingBuilt` would return `false` on the **very first** poll and the confirmation would have landed at ~+0.25 s, not +5.03 s. It also could not be true in the *other* session, which polled all the way to the 10 s cap. ⇒ **The poll ran ~20 iterations returning "still building" and then genuinely went idle.**

**⇒ The leading candidate is therefore the LULL, not the lock** — a real, transient instant in which `RunningDirtyTiles`, `PendingDirtyTiles`, `TileGeneratorSync` and the dirty-area queue were **all** empty while tile work was still owed, i.e. between waves of dirty areas as the scatter's 738 instances register. ⭐ **That is not a new theory: it is precisely the residual TASK-535 declared in its own §6.5** (*"a queue that reads empty at +5 s can be a lull between waves"*) — and it now has a measured session attached to it, which is strictly better than a flagged hypothesis. I am folding this into **WARN-10**, not opening a new finding, and ⛔ **not** re-opening TASK-535: its code is the strictly-better behaviour either way (a settle-poll `CONFIRMED` deliberately does **not** discharge the delegate bind, `BattlefieldScatter.cpp:2143-2147`, so the definitive post-settle verdict still runs).

⚠️ **The honest consequence, stated rather than smoothed:** on the lull route the shipped labelling would still print `CONFIRMED (nav settled: 0 pending)` at ~+5 s, because both predicates genuinely read idle and the code has no way to know better **without polling, which the spec forbids**. The determinism hole is *narrowed* (the definitive check still runs and still repairs), not hermetically closed. That is a known, bounded, declared residual — **not a defect to fix in this batch**, and ⛔ not grounds to disturb a passed task.

**⇒ Yes, it changes my evidence requirements for TASK-538, in three concrete ways** (folded into the watch list below): the `[at-confirmation]` snapshot must be read as a **four-number tuple**, the **absence** of a cap line must be stated explicitly (the manager's instinct is right — here an absence *is* evidence, and it is the only thing that distinguishes the two routes), and a `Nav generation FINISHED` line arriving **after** a `CONFIRMED` is the positive signature of the lull route. ✅ **I agree with the manager's disposition: cause 3 is `partly` answered, and it is answered the rest of the way by one PIE log, not by more reading.**

### 2. **Correct, and it is worth one sentence — NIT-6.**

The corrected block is accurate and correctly scoped, and it does **not** call brake 2 bookkeeping. But it argues brake 2 purely as a *count* bound, and a hasty reader could conclude the ladder's safety now rests on brake 1 alone and brake 2 is removable. **It is not:** delete brake 2 and a permanently wedged unit re-fires rungs at `1/EscalationCooldown` **forever** — a sustained mill that satisfies the stated ceiling exactly. Brake 2 is what makes each rung fire **once** and the ladder **terminate**.

⇒ **[NIT-6]** — one sentence to add to `SiegeStuckStatics.cpp` (the `BRAKE 2` block at `:93-105`), for whoever next opens the file: *"⛔ Brake 2 is not merely a count: it is what makes the ladder TERMINATE. Without it a permanently wedged unit re-fires at 1/EscalationCooldown forever — inside the stated ceiling and still a mill."* ⛔ **This does not gate anything** — the live guard already exists in the suite (`MonotonicLevelHoldsWithoutTheCooldown` goes red if brake 2 is deleted, cooldown or no cooldown), and I am not spending a loop on a sentence. Fold it into the manager's `NAV-§3` correction pass.

---

## Loop-1 findings

- **[NIT-6] `SiegeStuckStatics.cpp:93-105`** — the corrected brake-2 comment omits its **termination** role. One sentence, given verbatim above. Non-gating.

**That is the complete list.** No new WARN, no new BLOCKER. Loop-0's WARN-1 is **discharged** by the corrected comment (`SiegeStuckStatics.cpp:107-151`, `.h:120-133`), which carries my derivation faithfully — including the explicit *"⛔ TREAT `EscalationCooldown` AS THE ANTI-MILL KNOB AND DO NOT LOWER IT BELOW 1.0 WITHOUT RE-DERIVING THE CEILING"*, which is a better landing than I asked for. **All other loop-0 WARNs and NITs stand as written**; the manager corrections to CONVENTIONS (`NAV-§1` cause 3 — now *partly* answered per point 1 above · `NAV-§3`'s ceiling · `NAV-§8`'s `CellSize` and warnings-as-errors bullets · `NAV-§12` · `NAV-§3`'s zero-timers scope) are still owed and are **not** code work.

⚠️ **Line numbers shifted again in `SiegeStuckStatics.cpp` and `SummonedUnit.cpp`** (the comment blocks grew). The programmer converted its own citations to symbols per NIT-5; **handoff, CONVENTIONS and TASKBOARD citations into those two files now read stale. Grep symbols, never lines** — including every line number in this report.

---

## ⭐ TASK-538 — the watch list, superseding loop-0's items 7-16

⛔ **TASK-538 is now UNBLOCKED.** Loop-0's compile items 1-6, 17 and 18 stand unchanged. These replace items 7-16.

**At the suite**
- **7′.** Report every `Siegebound.Nav.Stuck.*` **individually, by name**. Four are load-bearing and must be named explicitly in the handoff:
  - ⭐ **`ImmediateReEvaluationIsRefused`** — the anti-mill assertion the whole cost claim rests on.
  - ⭐⭐ **`SidestepSideAlternatesAcrossStallsForOneUnit`** — **the blocker's proof.** ⚠️ **`SidestepSideIsConstantForOneUnit` must return ZERO results from the runner.** If the runner reports it at all, the fix did not land — stop and route back. ⛔ Do not grep the *source* for that string; it survives in two history comments by design.
  - **`EscalationCooldownGatesTheLadder`** and **`MonotonicLevelHoldsWithoutTheCooldown`** — after WARN-1 these are the two tests that carry the corrected brake claim, and (point 2 above) the second is the live guard on brake 2's termination role.
- **8′.** Whole-suite count against the baseline **53**. ⚠️ The batch adds 20; **73 is the expected floor**, and a *drop* elsewhere is this batch's problem.

**At the PIE run** (ASK first; an ini change needs an editor **restart** to take effect at all)
- **9′.** ⭐ **`gatherOnGameThread=`** — quote the whole `nav-config:` line verbatim. `false` ⇒ **TASK-540 branch A**, revert both keys. ⛔ Never an `L_Arena` save.
- **10′.** All three `nav-build [...]` lines verbatim — `[pre-scatter]`, `[post-scatter]`, `[at-confirmation]`. Expect `running=8` at post-scatter if the concurrency flip is live.
- **11′.** ⭐⭐ **THE `[at-confirmation]` LINE HAS TWO JOBS NOW — read it as a four-number tuple** (`remaining=` `running=` `dirtyAreas=` `hasDirty=`), because it is what separates the two false-`CONFIRMED` routes:
  - `remaining>0` or `hasDirty=true` beside a **`PROVISIONAL`** line ⇒ the **cap route**, and the fix is working as designed.
  - `remaining=0 running=0 dirtyAreas=0 hasDirty=false` beside a **`CONFIRMED`** line **at ~+5 s** ⇒ the **lull route** (point 1 above) — the queue really was momentarily empty. ⛔ **Not a bug; record it.** Its confirming signature is a **`Nav generation FINISHED`** line arriving **later in the same match**, i.e. work was still owed.
- **12′.** ⛔ **STATE THE ABSENCE EXPLICITLY.** Grep for `MaxNavSettleWait` / `still building` and **say in the handoff whether the count is zero or non-zero.** ⚠️ **An absence is evidence here** — it is the only thing that tells the two routes apart, and *"no cap line"* is a finding, not a blank.
- **13′.** ⛔ **A `CONFIRMED` printed beside a NON-ZERO `remaining=` is a `NAV-§4` failure** and must be reported verbatim, not smoothed.
- **14′.** `activeTiles=` vs `poolCap=`, **and grep `tile limit reached!`** (`NAV-§6`'s first-ever instrument). ⚠️ `activeTiles=-1 poolCap=-1` means **unavailable**, never zero.
- **15′.** Settle wall-clock against the **216 s** baseline. ⛔ Record the number; do not gate on a target.
- **16′.** `LogSiegeStuck` — every `escalate:` (`level=` `action=` `stalled=`) and every `blocked:`. ⭐ **Two signatures worth naming in the handoff:** `blocked:` with no following `escalate:` on that unit = **WARN-6** behaving exactly as documented (record, do not report as a bug); and **two `escalate: … action=Sidestep` lines for the same unit** is the first live evidence that the loop-1 fix is doing its job — ⛔ **their `stalled=` values will look similar; that is expected. The alternation is invisible in the log and only the unit's path shows it.** Jonathan's eyes (TASK-539) are the instrument for the side actually flipping.

---

## Board status — loop 1 (⚠️ still no partial-edit tool; please proxy)

- **TASK-537** → **`qa-passed`** — loop 1, diff-scoped, 0 blockers. Report `.claude/pipeline/qa/TASK-537.md`.
- **TASK-531 · 532 · 533 · 536** → **`qa-passed`** — all four fixes verified at the artifact.
- **TASK-529 · 530 · 534 · 535** → **unchanged, `qa-passed`** (loop 0). ⛔ Not reopened, not re-reviewed, and confirmed untouched by the loop-1 diff (fence F8).
- **TASK-538** → **`ready-for-integration`** — unblocked. Watch list above.
- **Still owed by the manager, before the post-538 checkpoint** (documentation, not code): `NAV-§1` cause 3 → **partly** answered, two routes, with the lull now the leading second candidate · `NAV-§3`'s rate ceiling → qualify with `EscalationCooldown ≥ 1.0`, and add brake 2's termination role · `NAV-§8`'s `CellSize` bullet → `GetCellSize(ENavigationDataResolution::Default)` · `NAV-§8`'s warnings-as-errors bullet → not established · `NAV-§12`'s "which predicate returned false" limitation → answered for the cap session, open for the lull session · `NAV-§3`'s ZERO-NEW-TIMERS clause → scope to the watchdog lane.
