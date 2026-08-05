# TASK-536 — `Tests/SiegeStuckStaticsTest.cpp` — the headless ladder suite

**Status:** ready-for-qa · **QA gate: TASK-537** · **Author:** gameplay-programmer
**Files touched — EXACTLY ONE, NEW:**
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeStuckStaticsTest.cpp` (1690 lines, 20 tests)

⛔ Nothing else moved. No `Build.cs` change (UBT compiles every `.cpp` under the module; `Tests/` already holds six files). ⛔ No shipped `.h`/`.cpp` edited — not one character of `SiegeStuckStatics.*`, `SummonedUnit.*`, `MinerUnit.*`, `SiegeNavAreas.*`, `BattlefieldScatter.*`, `SiegeNavDiagnostics.*` or `Config/DefaultEngine.ini`. ⛔ No `.umap` write. ⛔ No compile, no test run, no Git, no editor/MCP/PIE — **TASK-538 owns the only compile and the only run.**

**Assets referenced:** none.

## M8 DECLARATION (verbatim, `NAV-§11`)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

This is a test file compiled only under `WITH_DEV_AUTOMATION_TESTS`; it adds no shipped surface at all.

---

## 0. WHAT I READ BEFORE WRITING A LINE

CONVENTIONS `NAV-§3` (the anti-mill law + the rung table), `NAV-§7` (naming + the tunables row), `NAV-§8` (the pinned registry), plus `NAV-§0/§1/§2/§4/§9/§10/§11/§12`. Then the **finished** code: `SiegeStuckStatics.{h,cpp}` in full, `SummonedUnit.cpp` (`ConsumeStuckDeltaSeconds` `:2859`, `TickStuckWatchdog` `:2894`, `HandleStuckEscalation` `:2953`, `NotifyMoveBlocked` `:3051`, the five lease-clear sites), `MinerUnit`'s override, `SiegeNavAreas.cpp`'s `OnMoveCompleted`, the TASK-530 ini block, and all seven handoffs (529–535). Pattern files: `SiegeSettingsTest.cpp:113-116` (flags) and `SiegeKeyboardLayoutTest.cpp` (the injected-seam / honest-limits shape).

---

## 1. THE TWENTY TESTS — every name, and what each would catch

All under `Siegebound.Nav.Stuck.<Name>`, all `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`, all inside `#if WITH_DEV_AUTOMATION_TESTS`.

| # | test | what it would catch |
|---|---|---|
| 1 | `ShippedTuningDefaults` | A silent retune of any of the eight `NAV-§7` tunables. Also asserts the rung ordering **and** that every rung gap exceeds `EscalationCooldown` — i.e. it asserts the very fact that makes tests 10 and 11 necessary. |
| 2 | `RungOrdinalsMatchTheLogContract` | ⭐ A reorder of `ESiegeStuckAction`. TASK-532 logs the pinned `level=` token as `static_cast<int32>(Action)` (`SummonedUnit.cpp:2948`) — reordering the enum would make the gate's own evidence report the wrong rung, silently. |
| 3 | `MovingUnitReAnchors` | An `Evaluate` that accumulates before the cheap path, or that anchors once instead of every poll (a unit could then drift 150 uu at a time and never reset). Includes the `>= MinSpeedSq` boundary on both sides. |
| 4 | `IdleByDesignUnitIsNeverRescued` | Five simulated minutes of `bAdvancing == false`. A ladder keyed on velocity alone would "rescue" every Hold / Ambush / stationed / mid-attack unit in the game. |
| 5 | `SidestepFiresExactlyOnceAtThreshold` | Early or late rung 1, and a rung 1 that re-fires. Asserts the exact poll (7th), the exact clock (1.5 s), `EscalationLevel == 1`, and the cooldown reload. |
| 6 | ⭐ `ImmediateReEvaluationIsRefused` | **THE ANTI-MILL TEST.** A `Sidestep` returned on every poll past the threshold. Re-evaluates at delta 0 and at 0.25 s, then measures 200 polls: a mill would return ~199 fires; the shipped ladder returns 24. |
| 7 | `LadderClimbsToWidenThenAbandon` | Rung 2 / rung 3 timing, and — the load-bearing half — **`Abandon`'s auto-`Reset`**. An `Abandon` that left `EscalationLevel == 3` would wedge the ladder ("nothing above 3 is reachable") and silently disable the watchdog on that unit for the match. |
| 8 | `EscapingProgressRadiusResetsTheLadder` | A partial reset. A reset that cleared the clocks but kept the level would make the unit's *next* stall skip `Sidestep` — the only rung that actually steers around the rock. Includes the `>` boundary (exactly `ProgressRadius` is **not** an escape). |
| 9 | ⭐ `ZeroDeltaNeverAdvancesTheLadder` | The miner's seal value. 2000 polls at `DeltaSeconds == 0`, 500 more mid-ladder, negatives, **NaN**, and `+INF`. ⭐ The NaN block is what distinguishes the shipped ternary from `FMath::Max` — `FMath::Max` passes the zero and negative cases and fails **only** on NaN. |
| 10 | ⭐⭐ `EscalationCooldownGatesTheLadder` | **Deleting brake 1.** See §2. |
| 11 | ⭐⭐ `MonotonicLevelHoldsWithoutTheCooldown` | **Deleting brake 2**, and `<=` written as `<`. See §2. |
| 12 | `SustainedStallRespectsTheRequestCeiling` | `NAV-§10` criterion 1 as a **measurement**: 60 s of unbroken stall, both bounds asserted, plus the "only for units that are demonstrably not moving" half (a moving unit and a request-less unit both cost exactly 0 over the identical run). |
| 13 | `SidestepGoalIsPerpendicularAtDistance` | Six geometry cases × both parities: exact distance, Z taken from the unit (not the goal — a sidestep that inherited the goal's Z would send a ground unit into the air), zero projection on the travel direction, and ⭐ the documented **continuity** claim (the +Y fallback is exactly what the formula yields for a unit facing +X). |
| 14 | `SidestepGoalAlternatesOnAttemptParity` | ⛔ The `% 2 == 0` bug the implementation explicitly avoids — twelve attempts including `-1`, `-3`, `MAX_int32` and `MIN_int32`. Also asserts the two sides are exact mirrors, not merely different. |
| 15 | `SidestepGoalIsDeterministic` | Any RNG, clock read or hidden static state. 1000 bit-identical calls, then 200 interleaved calls with other inputs, then the reference again. |
| 16 | `SidestepGoalDegenerateInputsAreSafe` | NaN from `Goal == Location`, a goal directly overhead, and non-positive / NaN `SidestepDistance` (which must return `Location` unchanged). |
| 17 | `ResetClearsEveryField` | A `Reset` that misses a field, is not idempotent, or does not genuinely restart the climb at rung 1. |
| 18 | ⚠️ `SlidingUnitNeverEscalates` | **HOLE 1, pinned as shipped.** See §3. |
| 19 | ⭐⚠️ `SidestepSideIsConstantForOneUnit` | **HOLE 2, pinned as shipped.** See §3. |
| 20 | ⚠️ `BlockedClockBumpIsDiscardedWhenNotAdvancing` | **HOLE 3, pinned as shipped.** See §3. |

---

## 2. ⭐⭐ THE ANTI-MILL CASES, AND WHY DEFAULT TUNING WAS NOT ENOUGH

**TASK-531's note to this task was correct and I confirmed it at the artifact.** At the shipped tuning the rung gaps are 1.5 s → 3.0 s → 6.0 s and `EscalationCooldown` is 1.0 s, so **every gap exceeds the cooldown**. `SecondsSinceEscalation` accumulates alongside `StalledSeconds`, so it already reads 1.5 when the first rung comes due. ⇒ **Brake 1 never DECIDES anything at the defaults: every poll it refuses, brake 2 would have refused too.** A suite that only used default tuning could delete the cooldown check entirely and stay green — leaving the whole anti-mill gate untested, which is the law (`NAV-§3`, and `CONVENTIONS.md:535`'s *"a follow implementation that re-paths unconditionally is a QA FAIL"*). Test 1 now asserts that fact rather than leaving it as prose.

**Test 10 — brake 1 made load-bearing.** `SidestepSeconds = 1.5`, `WidenSeconds = 1.6` (both legal `EditDefaultsOnly` values Jonathan can type at the playtest without a recompile). `Sidestep` fires at 1.5 s; on the very next poll the stall clock is 1.75 s — **past `WidenSeconds`, with brake 2 satisfied** (`DesiredLevel 2 > EscalationLevel 1`) — so the only thing that can return `None` is the cooldown. Three polls are asserted `None` **with the reason proved from the post-call state** (`StalledSeconds >= WidenSeconds && EscalationLevel == 1 && SecondsSinceEscalation < EscalationCooldown`), and `WidenAndRepath` is released on the poll where `SecondsSinceEscalation` reaches exactly 1.0 — 2.5 s, not 1.75 s. **Delete the cooldown check and this goes red immediately.** The test also covers the fully degenerate tuning (all three thresholds equal), where only `Abandon` is reachable and — because `Abandon` issues no path request — the request rate is **zero**.

**Test 11 — brake 2 alone, with `EscalationCooldown = 0`.** Brake 1 is then structurally disabled (`SecondsSinceEscalation < 0` is never true), so every refusal is monotonicity doing the work. Five consecutive polls after `Sidestep` return `None` with the cooldown provably out of the picture. The sustained half (120 s) asserts: ≤ 1 `Sidestep` per stall, ≤ 1 `Widen` per stall, ≤ 3 rungs per stall ever, ≤ 2 **path requests** per stall (Abandon issues none), and that the fire sequence is the exact cyclic `Sidestep → WidenAndRepath → Abandon` with no repeat, inversion or skip.

⚠️ **AND SEE §4 FINDING 1: the third block of test 11 measures something that contradicts TASK-531's handoff §2.**

---

## 3. ⚠️ THE THREE KNOWN HOLES — PINNED AS SHIPPED, FLAGGED, NOT PAPERED OVER

⛔ **I did not touch a shipped file to change any of these.** Each test asserts the behaviour that ships today, so if TASK-537 rules a change is owed, the test that must be edited is named.

### HOLE 1 — the rung-0 speed clause is an `OR` (test 18)

`!bAdvancing **OR** speed >= MinSpeedSq **OR** escaped ProgressRadius`. Because the speed clause is an `OR`, a unit **scraping along a collider** (or oscillating in place) at ≥ 50 uu/s re-anchors on every poll and is **never rescued**, even with zero net displacement — the displacement clause alone would catch it, and it is the clause that measures the thing that actually matters. **Measured in the test:** 60 s at 51 uu/s with ±4 uu of oscillation ⇒ **0 rungs**. The same unit at 49.99 uu/s ⇒ the ladder runs normally. ⚠️ **This is a THRESHOLD hole, not a broken ladder, and it is NOT a claim about the observed wedge shape** (`NAV-§0` ruling 1 forbids that). **It is a TASK-539 playtest watch:** if Jonathan reports "units still wedge on rocks", check this first — and both fixes (drop the clause, or lower `MinSpeedSq`) live inside the pinned signature, the second needing no recompile.

### HOLE 2 — ⭐ `Attempt` cannot vary for a single unit (test 19)

`ComputeSidestepGoal`'s parity contract is implemented **perfectly** (test 14 proves it over the whole `int32` range). **The shipped call sites cannot exercise it.** Each link verified at the artifact:
- `Sidestep` fires **exactly once per stall** (brake 2 is monotonic), so `Attempt` could only vary **across** stalls;
- `FSiegeStuckState` (pinned, `NAV-§8`) has **no attempt counter**, and `Reset` clears the whole struct on every `Abandon` and every re-anchor;
- the shipped expression is `EscalationLevel + (GetUniqueID() % 2)` (`SummonedUnit.cpp:2978-2979`; `MinerUnit` uses the identical idiom), and **`EscalationLevel` is always 1** at the moment `Sidestep` is returned (`SiegeStuckStatics.cpp:135` assigns `DesiredLevel` before the switch).

⇒ **`Attempt` is a per-unit CONSTANT — 1 or 2, forever.** Test 19 runs three consecutive full stall cycles and asserts all three produce the **identical waypoint**. ⛔ **A unit wedged on a rock's left face sidesteps into the rock every single time; the "retry the other side" behaviour does not exist.** ✅ The half TASK-532 claims *does* work and is asserted separately: two neighbours of opposite unit-id parity go to opposite sides, so a clump on one rock does not fan into itself. **Fixing the per-unit half needs a new persistent field — a change to the `NAV-§8` pin — which this task may not make. TASK-537 rules.**

### HOLE 3 — `NotifyMoveBlocked`'s clock bump (test 20)

TASK-532's flag C calls it *"usually discarded on the very next poll."* **The pure half is testable and the honest reading is stronger than "usually":**
- `EPathFollowingResult::Blocked` means the request **finished**, so `GetMoveStatus()` is `Idle` until something re-issues — and the re-issue happens **later in the same poll** than the watchdog call (`SummonedUnit.cpp:1348` sits above the profile dispatch at `:2440`);
- ⇒ the next `Evaluate` sees `bAdvancing == false`, takes the re-anchor branch, and `Reset`s — **wiping the bump**.
⇒ **In the scenario it was written for, the bump contributes exactly nothing.** Test 20 asserts the wipe. It also asserts the corroborating case: when the next poll *does* have a live request the bump survives, and ⛔ **it still does not bypass the brakes** — brake 1 holds the rung for a full `EscalationCooldown`, so it can only pull the first rung forward (measured: from 1.75 s to 1.00 s, worth 0.75 s) and can never skip `Sidestep` to reach `Abandon`. ⛔ **The world half — that a `Blocked` verdict implies an `Idle` status on the next poll — is engine-contract reasoning and is NOT asserted by this suite.**

---

## 4. ⚠️ DISAGREEMENTS WITH THE SEVEN IMPLEMENTATIONS I READ — TASK-537 RULES

### FINDING 1 (load-bearing) — ⛔ "EITHER BRAKE ALONE BOUNDS THE REQUEST RATE" IS HALF FALSE

`handoffs/TASK-531-programmer.md` §2 and `SiegeStuckStatics.cpp:109-123` both state the two brakes are **independently rate-bounding**, and that this redundancy is *"what makes the bound hold under arbitrary `EditDefaultsOnly` tuning."* **Brake 2 bounds the COUNT PER STALL (≤ 3 rungs, ≤ 2 path requests). It does not bound the RATE**, because a stall's *duration* is itself a tunable. Measured in test 11(c): with `EscalationCooldown = 0` and thresholds `0.25 / 0.50 / 0.75` — all legal `EditDefaultsOnly` values — the ladder issues **2.0 path requests/unit/s**, i.e. **twice** `NAV-§3`'s *"≤ 1 extra path request per unit per second"* ceiling. ⇒ **The rate ceiling rests on `EscalationCooldown ≥ 1.0` ALONE; brake 2 is a count brake.** ✅ **The shipped defaults are unaffected** (test 12 measures 0.333/unit/s), and 2/s is still far below the forbidden per-poll mill's 4/s — so this is a **wording/claim** defect, not a shipped defect. ⛔ I asserted the per-stall bound (brake 2's actual guarantee) as a hard test and recorded the rate via `AddInfo`, because red-failing on a tuning nobody ships would be dishonest. **The correction belongs in `SiegeStuckStatics.cpp`'s comment block and in `NAV-§3`, and only the manager/QA may make it.**

### FINDING 2 — ⚠️ THE NaN CLAMP IS VERIFIED ONLY IN THE TARGET THAT RUNS THE TEST

`SiegeStuckStatics.cpp:60`'s `(DeltaSeconds > 0.f) ? DeltaSeconds : 0.f` relies on **every comparison against NaN being false**. I verified the toolchain rather than assuming: UBT sets `FPSemantics = Precise` (`/fp:precise`) for **Editor and Program** targets when `DefaultBuildSettings >= V7` (`TargetRules.cs:3739`, `VCToolChain.cs:1324-1346`), and both `GitClaudeUnrealTest.Target.cs` and `GitClaudeUnrealTestEditor.Target.cs` are V7. ⇒ **The Editor target — the one this suite runs in — is `/fp:precise`, so test 9(d) is a valid check there.** ⚠️ **But the GAME target is `TargetType.Game`, which keeps `Imprecise` ⇒ `/fp:fast`**, where the compiler is permitted optimisations that do not preserve NaN semantics. ⇒ **A green NaN test in the editor does not transfer to the packaged game.** ⛔ Not a defect in TASK-531's code and **not** a reason to change it: no shipped caller can produce a NaN delta (`ConsumeStuckDeltaSeconds` clamps to `[0, MaxStuckDeltaSeconds]`). Recorded so nobody later cites the green test as proof about a shipped build. The test guards against a vacuous pass by asserting `FMath::IsNaN` on its own constant first.

### FINDING 3 — ⚠️ `+INF` IS NOT CLAMPED (declared, harmless, pinned in test 9(e))

The clamp catches `<= 0` and NaN. `+INF` passes `> 0.f`, so it satisfies every threshold at once and the ladder jumps straight to `Abandon`. ✅ **This is safe by construction and I assert it rather than call it a bug:** `Abandon` issues **no** path request and its own `Reset` scrubs the infinity out of the state, so the unit is left clean. Unreachable from both shipped call sites (`MaxStuckDeltaSeconds == 1.f`, `SummonedUnit.cpp:2889`). Recorded so it is not "discovered" later as a crash risk.

### FINDING 4 — ⚠️ TASK-532 FLAG B's CLAMP AND TASK-531's COMMENT TAKE OPPOSITE POSITIONS, AND BOTH ARE RIGHT

`SiegeStuckStatics.cpp:98-101` says a single large delta jumping level 0 → 3 is *"the honest reading."* `SummonedUnit.cpp:103`'s `MaxStuckDeltaSeconds = 1.f` clamps precisely that away. ✅ **Not a conflict in behaviour** — TASK-532's rationale (a poll GAP caused by `ApplyFreeze` is not elapsed stall time) is correct and the clamp is on the caller's side of a pure function. ⚠️ **It is a conflict in the DOCUMENTED contract:** the library's comment describes a behaviour no shipped caller can now produce. My test 9(e) exercises the unclamped path directly, so the library's stated behaviour is covered even though the callers suppress it. **No change requested; recorded so a future reader does not "fix" one to match the other.**

### FINDING 5 — ⚠️ TASK-534's §8 IS CORRECT AND IT MATTERS TO `NAV-§8`

TASK-534 could not verify *"warnings are errors in this build"*. **I independently confirm the premise is false as stated:** neither target sets `bWarningsAsErrors`, and UBT's default is `[BasicWarningLevelDefault(WarningLevel.Warning)]`. ⇒ `NAV-§8`'s bullet on the deprecated `OnMoveCompleted` overload has the right **instruction** and the wrong **consequence**. Repeated here because two independent readers now agree and the CONVENTIONS wording should be corrected by the manager rather than re-derived a third time.

### FINDING 6 — the coverage boundary the split created (`NAV-§3`'s cost claim)

`NAV-§3` states a per-poll cost ("one `DistSquared`, one float compare, two adds, one `uint8` compare, zero allocations, zero world queries"). ⭐ **That claim is CHECKABLE at the source precisely because the library is pure — but it is not ASSERTABLE by this suite**, which measures decisions, not instructions. It is QA's read of the artifact, not a test result. Saying so is spec item (C)'s inexpressibility clause discharged.

---

## 5. ⛔ WHAT I COULD **NOT** COVER — STATED PLAINLY, NOT IMPLIED

Everything below needs a world, a pawn or a navmesh and is **unreachable** from a pure-static suite. It is also written into the test file's header comment so a reader of the artifact sees it without opening this note.

- **`ASummonedUnit::TickStuckWatchdog`'s call site and its ordering** (that it sits above the profile dispatch and below the freeze early-out), the `bAdvancing` read via `GetMoveStatus()`, `ConsumeStuckDeltaSeconds`'s world-clock delta, its first-call inert latch and its `MaxStuckDeltaSeconds` clamp.
- **The SIDESTEP LEASE, in full** — arming, draining, the `UpdateState` early-out, and all five clear sites (`EnterIdle` `:2710`, `EnterAttack` `:2472`, `HandleDeath` `:3466`, `FreezeAI` `:697`, `ApplyFreeze` `:942`), including TASK-532's declared fifth site. ⚠️ **`NAV-§10` criterion 3 says "trace each one at the artifact" — that remains entirely QA's job; no test here touches it.**
- **Every rung's ACTION.** `EnterAdvanceToLocation`, the three latch invalidations in `WidenAndRepath`, `CurrentTarget = nullptr` + `EnterIdle` in `Abandon`; and on the miner `DriveToPoint`, `EnsureWalkingToNode`, `StopMovement`, `StandInPlace`, `SeekBestMine` and the `!bArrivedAtNode` tenure guard. The suite proves **which rung is chosen**, never **what the rung does**.
- **`ASiegeUnitAIController::OnMoveCompleted`** — the `Blocked` filter, the unconditional `Super::` call, the 10 s throttle, and that `NotifyMoveBlocked()` sits outside it. **`ASummonedUnit::NotifyMoveBlocked` itself is never called by this suite** (test 20 models its two writes only).
- **The `Attempt` values the call sites actually pass** — test 19 models the expression; it does not execute `GetUniqueID()`.
- **Everything in TASK-529 and TASK-535** — `FSiegeNavDiagnostics`, the three telemetry call sites, the pinned `gatherOnGameThread=` / `activeTiles=` / `poolCap=` tokens, `CONFIRMED` vs `PROVISIONAL`, the settled-only cull, `bCullOnProvisionalFailure`, the generation-finished delegate and its unbind.
- **All of TASK-530** — whether the ini flip reaches `L_Arena`'s serialized `ARecastNavMesh`. `NAV-§9`'s conditional is settled by TASK-538's PIE log and by nothing in this file.
- **The `NAV-§3` cost claim** (finding 6) and **the shipped-build NaN behaviour** (finding 2).

⚖️ **A full green run here means THE LADDER'S ARITHMETIC IS RIGHT — nothing more.** The batch's real gates are TASK-538's log and TASK-539, Jonathan's playtest.

---

## 6. STANDING C++ SWEEP + `SC-§13`

- **`SC-§13`:** ⛔ this file makes **no `FString` claim of any kind** — the library under test has no string surface. Every string is message text passed to a `What` parameter, so `TestEqual`-on-`FString` case-insensitivity cannot bite. `TestEqualSensitive` is therefore not used, and that is the correct outcome rather than an omission. **Every float comparison passes an explicit tolerance** (`ClockTolerance` 1e-4, `GeometryTolerance` 0.01); every `FVector` comparison uses the tolerance overload.
- **Complete-type include law:** `Misc/AutomationTest.h`, `Containers/Array.h`, `HAL/UnrealMemory.h` (`FMemory::Memcpy`), `Math/UnrealMathUtility.h`, `Math/Vector.h`, `Siegebound/SiegeStuckStatics.h`. Nothing else, and no `Build.cs` change.
- **Overload safety:** `FAutomationTestBase` has **no `bool` overload** of `TestEqual`, so every bool claim uses `TestTrue`/`TestFalse`; `uint8` and `SIZE_T`/`int32` values are `static_cast<int32>` at the call to keep resolution unambiguous. All float literals carry the `f` suffix.
- ⭐ **A `Memcmp`-based drift guard on `FSiegeStuckState` was considered and DELIBERATELY REJECTED** (with the reason written into the file): the struct carries trailing padding, padding bytes are indeterminate in both instances, and `Reset`'s implicit copy-assignment is memberwise — a byte compare would be an **intermittent** test. The five named field assertions are the guard instead.
- **No shadowing, no most-vexing-parse:** every scratch harness lives in its own brace scope; no default-constructed local is declared with `()`.
- **Determinism of the tests themselves:** every delta is `0.25` (exactly representable, `2^-2`) and every shipped threshold (1.5 / 3.0 / 6.0) is exactly representable, so accumulations land **exactly** on a threshold and rung timings are asserted at a poll index rather than with a fudge factor. NaN and `+INF` are built from IEEE-754 bit patterns via `FMemory::Memcpy` (⛔ never `0.f/0.f`, which is UB), and **each is asserted to really be NaN/INF before use** so a folded constant reports itself instead of passing vacuously.
- **⛔ Not compiled and not run.** Braces balance (108/108), 20 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros against 20 `RunTest` bodies, and every expected poll index in the file was hand-traced against `SiegeStuckStatics.cpp`. **Correctness as source is the only claim I can make** — TASK-538 owns the compile and the run.

## 7. WHAT QA SHOULD SCRUTINISE FIRST

1. ⭐⭐ **Finding 1** — the "either brake alone bounds the rate" claim in `SiegeStuckStatics.cpp:109-123` and `handoffs/TASK-531-programmer.md` §2. Measured, not argued.
2. ⭐ **Hole 2** (§3) — `Attempt` is a per-unit constant. This is the one with a gameplay consequence: *a unit wedged on a rock's left face steps into the rock every time.*
3. **Hole 3** (§3) — `NotifyMoveBlocked` is inert in exactly the case it was written for, not merely "usually".
4. **Hole 1** (§3) — the `OR` in rung 0, and whether it should be on Jonathan's TASK-539 list explicitly.
5. **§5 in full** — the coverage boundary. ⚖️ A test file that implies more coverage than it has is worse than a smaller honest one, so please check §5 against the file's own header comment; they are meant to say the same thing.
6. **Findings 2–5** — the toolchain FP-semantics split, `+INF`, the clamp/comment tension, and TASK-534's warnings-as-errors correction (now independently confirmed).

## 8. STATUS

`TASK-536` → **`ready-for-qa`**. Gate: **TASK-537** (`.claude/pipeline/qa/TASK-537.md`).

---

# ═══ QA LOOP 1 (2026-08-04) — TEST 19 INVERTED ═══

**Status:** `ready-for-qa` (loop 1 of 3) · **Gate: TASK-537**, re-gate is `SC-§27` **diff-scoped**
**QA report fixed against:** `.claude/pipeline/qa/TASK-537.md` — THE ONE BLOCKER
**Files touched this loop: EXACTLY ONE — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeStuckStaticsTest.cpp`.**
⛔ No shipped `.h`/`.cpp`/`.ini` touched by THIS task. **Test count unchanged: still 20.**

> **adds no replicated property, no new replicated class, no new relevancy tier.**

A test file compiled only under `WITH_DEV_AUTOMATION_TESTS`; it adds no shipped surface at all.

## 1. ⭐ HOLE 2 WAS RULED A BLOCKER, SO TEST 19 HAD TO BE INVERTED — IT WOULD HAVE GONE RED

Test 19 asserted the hole **as shipped** (three consecutive stall cycles ⇒ the *identical*
waypoint). TASK-532/533's fix makes that assertion false, so leaving it would have red-failed
the suite at TASK-538 on a **correct** codebase. It now proves the corrected behaviour.

⚠️ **DECLARED DEVIATION FROM QA'S PRESCRIPTION — THE TEST WAS RENAMED, AND QA'S BUILD-MASTER
CHECKLIST ITEM 8 NEEDS THIS ONE LINE UPDATED:**

| | |
|---|---|
| **was** | `Siegebound.Nav.Stuck.SidestepSideIsConstantForOneUnit` (class `FSiegeStuckAttemptConstantTest`) |
| **now** | `Siegebound.Nav.Stuck.SidestepSideAlternatesAcrossStallsForOneUnit` (class `FSiegeStuckAttemptAlternatesTest`) |

QA named the old string in its TASK-538 watch list, so I am flagging the rename rather than
burying it. **Reason:** a test named *"SideIsConstant"* that asserts alternation is precisely the
*"comment describing an unreachable branch"* defect QA blocked the batch on, one layer up. ⛔ **The
signal QA wanted is preserved and strengthened:** the old name is now **absent from the suite
entirely**, which is unambiguous evidence the fix landed — stronger than the old name merely
going red. **If QA prefers the pinned string, reverting is a two-token edit** (the
`IMPLEMENT_SIMPLE_AUTOMATION_TEST` name argument and the class name) and nothing else changes.

## 2. HOW TEST 19 READS NOW — FOUR BLOCKS

**(a) THE FIX ITSELF — three consecutive REAL stall cycles, one unit, one counter.** The harness
still runs the genuine ladder (`Sidestep -> Widen -> Abandon -> re-anchor`, three times). Now
asserted: the counter **advanced once per fired `Sidestep` and survived all three `Reset`s**
(reads 3); consecutive stalls compute a **different `Attempt` PARITY**; the waypoints are
**different**, and — stronger than different — **exact mirrors about the unit** (their offsets
sum to zero); and the third stall **returns to the first stall's waypoint**, so it is an
**alternation, not a drift**.

**(b) ⚠️ THE `uint8` WRAP, ASSERTED RATHER THAN ASSUMED.** The counter is free-running and will
wrap on a long-lived, frequently-wedged unit. Driven to 254 and stepped past the boundary: the
counter is asserted to **wrap to 0** rather than saturate; attempts read 254 / 255 / 0 / 1; and
the **side still alternates across the wrap** (the first post-wrap goal mirrors the last pre-wrap
one; 254 and 0 land on the same side, 255 and 1 on the other) — because 256 is even.

**(c) ✅ THE NEIGHBOUR DE-CORRELATION, KEPT.** QA required this half stay green and it does:
two neighbours of opposite unit-id parity go to **opposite sides**. ⭐ **Extended by one block**
that guards the interaction the fix could plausibly have broken: two neighbours stepping in
**lockstep** stay on opposite sides at **every** attempt index, because the id term is a fixed
**phase offset** rather than a competing counter.

**(d) The `EscalationLevel == 1` assertion is KEPT, with its meaning inverted.** It used to
document why `Attempt` was pinned; it now documents **why the counter may never live in the
ladder state** — the level reads 1 at every `Sidestep`, on every stall, forever, so any term
derived from it is a constant. That is the regression this test guards.

## 3. `ShippedAttemptForSidestep` RE-MODELLED (QA named this explicitly)

```cpp
// was: (const FSiegeStuckState& StateAtSidestep, uint32 UnitUniqueID)
//        -> EscalationLevel + (UnitUniqueID % 2)
static int32 ShippedAttemptForSidestep(uint8& SidestepAttemptCount, uint32 UnitUniqueID)
{
    return static_cast<int32>(SidestepAttemptCount++) + static_cast<int32>(UnitUniqueID % 2);
}
```

⭐ **The counter is taken BY REFERENCE and post-incremented here exactly as the shipped line does**
— modelling that side effect is the whole point. The test declares its `uint8` **outside**
`FStallHarness` (which owns the `FSiegeStuckState` the ladder `Reset`s), because **surviving those
`Reset`s is the property under test.** ⛔ It still MODELS the expression: it does not execute
`GetUniqueID()` and does not construct a unit — §5's coverage boundary is unchanged and still
accurate.

## 4. ⛔ WHAT WAS NOT TOUCHED

- **Tests 1–18 and 20 are byte-unchanged**, including test 14 (the parity contract over the whole
  `int32` range), test 6 `ImmediateReEvaluationIsRefused`, tests 10/11 (the two brakes made
  independently load-bearing) and test 12's measured ceiling. **Baseline count 53 is unaffected.**
- **Hole 1 (test 18) and hole 3 (test 20) still ship as pinned** — QA ruled both WARN, not
  BLOCKER, and I did not touch them.
- The file header comment was updated to say **two** holes remain pinned (18, 20) and that hole 2
  was ruled a BLOCKER and fixed in shipped source at loop 1, naming the rename. ⛔ Nothing else in
  the header moved.
- ⛔ **Not compiled and not run** — TASK-538 owns both. Correctness as source is my only claim.

## 5. FINDING 1 IS NOW DISCHARGED IN SHIPPED SOURCE

My §4 FINDING 1 (*"either brake alone bounds the request rate" is half false*) was upheld as
**WARN-1** and corrected this loop by TASK-531 in `SiegeStuckStatics.{h,cpp}`. ⚠️ **My test
assertions did not change**: test 11(c) already asserted brake 2's true guarantee (the per-stall
**count**) as a hard test and recorded the **rate** via `AddInfo`, which is exactly right now that
the comment says the same thing. ⛔ The remaining correction is **manager-owed**: `NAV-§3`'s
`CONVENTIONS.md:2479-2482` still states the ceiling unconditionally.
