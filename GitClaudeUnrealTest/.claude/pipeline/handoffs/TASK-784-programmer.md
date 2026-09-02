# TASK-784 — the contact-climb trigger's CALL SITE for units — programmer handoff

**Status:** ready-for-qa · **Suite delta: +3** (tests 15, 16, 17 in `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp`; that file goes 14 → 17) **plus 2 new `static_assert`s** (compile-time, no suite count).

`AClimbableTower::TryBeginContactClimb` had **zero callers**. It now has exactly one.

---

## 0. ⛔⛔ ATTRIBUTION — READ BEFORE GATING

**The `ClimbableTower.h` edit in this diff belongs to TASK-784, ⛔ NOT TASK-777.** Board item (4) grants exactly one narrow, named cross-fence line: `LadderContactRadiusUU` **`150.f → 300.f`** (`CONTACT-§7` `K-6`) plus its consequence comment re-derived. ⛔ Nothing else in `ClimbableTower.{h,cpp}` is touched — not the other two tunables, not a line of logic.

⭐ **TASK-777's declared `+2` suite delta is ALREADY DISCHARGED and stays true for its own diff.** This task's delta is `+3` and is entirely in `SiegeLadderClimbTest.cpp`.

### ⚠️⚠️ AND MY LANDING ARMS THE TRIPWIRE THE BOARD PREDICTED — IT IS NOT MINE TO DISARM

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp:1584` asserts:

```cpp
TestEqual(TEXT("(g) LadderContactRadiusUU ships at K-5's 150 uu"), RadiusUU, PinnedContactRadiusUU, Tolerance);
```

`PinnedContactRadiusUU` is `150.f` (that file's fixture). **`K-6` makes the shipped value 300, so this row goes RED the moment my diff lands.** That file is **TASK-785's**, explicitly outside my fence (board item (7)), and the board says the pin was *"deliberately not pre-emptively silenced — whoever lands second reconciles the pin, its label and `K-5`→`K-6`."* I landed the radius; **I did not touch that file.** Whoever runs the compile gate must expect exactly one red row there and reconcile it (`150.f → 300.f` and the label `K-5` → `K-6`).

---

## 1. The poll, as placed, and its cadence

**One call site, on the shipped 0.25 s `StateTimerHandle` poll — ⛔ NOT the actor tick.**

`SummonedUnit.cpp` `UpdateState()`, immediately below `TickStuckWatchdog`:

```cpp
const float PollDeltaSeconds = ConsumeStuckDeltaSeconds();
TickStuckWatchdog(PollDeltaSeconds);

if (TryContactClimbAtNearestLadder(PollDeltaSeconds))
{
    return;
}
```

- **Placement** mirrors `TickStuckWatchdog`'s exactly: above the follow hoist and above the profile dispatch, so Standard / Siege / Support / Follow / Hold / Ambush are covered from one line; above the sidestep-lease early-out, because a unit being rescued from a rock is still a unit standing at a ladder (and `BeginLadderClimb` step (5) clears the lease itself). It is **below** the `bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || IsClimbing()` fence.
- **Zero new timers, zero new tick drivers, zero new members, zero new tunables, zero new UPROPERTYs.** The feature's whole footprint on `ASummonedUnit` is one private method and one call.
- **The real delta is passed, ⛔ never a literal 0.25** (board item (3)). The hoist is the only edit to existing behaviour and it is neutral: `ConsumeStuckDeltaSeconds()` is a *consuming* read (it latches `LastStuckTickTimeSeconds`), so a second call would hand the contact poll ~0 and the dwell would never advance. One clock read, one value, two consumers — the discipline that helper's own comment asks for. `TickStuckWatchdog` receives the identical value it always did.

### ⭐ Item (3a) — the free correctness property, named rather than re-guarded

`StateTimerHandle` is cleared on death, freeze and stand-down (`SummonedUnit.cpp:598`, `:679`, `:956`, `:990`). **A unit with no live handle is dead / frozen / stood-down — every one of which `CanBegin` already refuses.** ⇒ the poll cannot fire in a forbidden state **by construction**. I added **no redundant guard** and no test of a guard: `TryContactClimbAtNearestLadder` has no fence of its own at all, and relies on `UpdateState`'s existing one plus this property.

### ⭐ Verification of the `WantsActorTick` finding — independently measured, CONFIRMED

I reached this from the source before the correction arrived, and it is right:

- `SummonedUnit.cpp:139` — `PrimaryActorTick.bStartWithTickEnabled = false`.
- `RefreshActorTickEnabled` is the **only** writer of the tick flag; its value is `FSiegeLadderClimbStatics::WantsActorTick(bLungeActive, LadderClimb.bActive)`.
- ⇒ a unit that is neither mid-lunge nor **already climbing** has its actor tick **OFF** — exactly the state every contact climb begins in.
- ⇒ **a poll in `::Tick` would compile, review clean, pass the whole suite and never run** — the same silent class as the zero-caller gap this task exists to close.

**`RefreshActorTickEnabled` and `Tick` are byte-restored to their shipped behaviour.** I did add an OR'd third term at one point and reverted it under item (2a); the pure `WantsActorTick` (TASK-776's file) was never touched and TASK-760's truth table is intact. Both functions now carry a comment recording that a third term was considered and refused, so it is not re-proposed.

---

## 2. How the per-poll cost is bounded

**Steady state with no WatchTower in play: one class-hash lookup that finds an empty bucket. That is the whole cost.**

`ForEachObjectOfClass(AClimbableTower::StaticClass(), …)` — **not** `TActorIterator`.

- `TActorIterator` walks **every actor of every level** and class-tests each. This file already pays that five times per poll (`AcquireTarget`, `FindNearestEnemyCastle`, `FindOwnCastle`, `FindNearestEnemyBuilding`, `AcquireEnemyNearPoint`); a sixth would add well over a million class tests/second at 120 units × 4 polls/s, for a card usually not in play.
- `ForEachObjectOfClass` is a **hash-bucket lookup**: `UObjectHash.cpp:1885-1887` does one `TMap::Find` per class in the (tiny) derived-class set and iterates only that class's instance list. Cost = **O(live `AClimbableTower`s)** — **0** in most matches, 1–5 otherwise.
- With towers in play: 120 units × 4 polls/s × ≤5 towers ⇒ ≤2,400 `TryBeginContactClimb` calls/s, and the `TooFar` path is two link reads, one `IsAtTopEndpoint`, one `DistSquared2D` and a prune of a table bounded by the pawns actually at that ladder.
- **Only ONE tower is asked per poll — the nearest**, by the same 2D metric the tower's own proximity term uses, against the nearer of its two link endpoints (both, because `K-C` arms at both ends).

**Collect-first, act-second is an engine requirement:** `UObjectHash.h:243` — *"the operation must not modify UObject hash maps so it can not create, rename or destroy UObjects"* — and the callback runs under `FHashTableLock`. `TryBeginContactClimb` reaches `BeginLadderClimb` → `SetMovementMode` → a BP-observable movement-mode change, none of which may run inside that lock. `TInlineAllocator<4>` keeps it allocation-free.

The lambda filters on `IsValid` + `!IsActorBeingDestroyed()` + **`GetWorld() == World`** — the world filter is mandatory, not padding: the class hash spans every loaded world, so a level-placed tower in the editor world would otherwise be handed to a PIE unit.

---

## 3. Proof no tunable was duplicated

**There is not one authored float anywhere in the call site.** No radius, no cone cosine, no dwell, no search threshold, no cadence constant.

- **No pawn-side proximity threshold**: one would be a fourth tuning number that could silently become tighter than `LadderContactRadiusUU` and delete the feature with every tower test green. The nearest tower is asked **unconditionally**; the tower answers `TooFar` if the pawn is not near enough.
- **No team test, no eligibility test** — a pawn-side copy of the team gate would be the silent back door around `T-3` that `CONTACT-§4.3` forbids. `CanTeamAscend` is still reached only through `EvaluateLadderEntry`, from the tower's one return path; **I added no second route to it.** Enemy towers are asked and answer `WrongTeam`.
- Asserted from the pawn's side by **test 17**, stronger than the tower's 12(g): **two-sided** (each of the three names is first proven to resolve *on `AClimbableTower`*, so a rename cannot make the absence check pass vacuously), and it also bans `Dwell` / `IntentCos` / `ContactRadius` as substrings so a duplicate cannot hide under another name.

---

## 4. No ninth exit, and no second steering authority

**Ninth exit:** the call site never ends a climb — no `EndLadderClimb`, no `AbortLadderClimb`, no movement-mode write, no touch of `LadderClimb` or `LadderClimbSavedMaxFlySpeed`. On `Declined` the **tower** has already undone its own binding (`ReleaseClimber`). The eight exits, the deck-breach window, the capsule-centre lift and TASK-760's self-heal are untouched.

**Second steering authority — confirmed from my entry path, no second stop added:**
1. `BeginLadderClimb` step (4) calls `AI->StopMovement()` **before** setting `MOVE_Flying`. Verified in place; I added nothing.
2. **The `return` after a successful ask is the other half and is load-bearing.** Without it `UpdateState` falls through to the profile dispatch and calls `EnterAdvance`/`EnterAttack` on the same poll, handing the pawn back to path following, which writes the same input vector the climb steers with.
3. Every **later** poll is fenced out by the existing `IsClimbing()` early-out.

**Also preserved:** no `CanEverAttack()` override, the transient disarm untouched, `EvaluateLadderEntry` not re-implemented, no mesh dependency (endpoints are read off the **link** at runtime, so TASK-783 moving the ladder ~4.6 uu is invisible to this code).

---

## 5. ⚠️ TWO CORRECTIONS TO `K-6`'s STATED BASIS — the ruling stands, its arithmetic did not

I simulated the shipped predicate exactly (`FSiegeLadderContactStatics::WantsToClimb`, stepped, 16 sampling phases) at both 150 and 300.

### A) *"at the 0.25 s cadence no unit could ever have climbed either"* is **REFUTED**

Each poll credits a **full** `PollSeconds` of dwell for one instant that satisfied the terms, so the bar is not *"the window must be ≥ 2 × poll"* — it is *"two samples must land inside the window"*, i.e. **window > one poll period**, with probability `(window − Poll) / Poll`. At the pre-`K-6` 150 uu:

| Card | in-cone window | admitted at the 0.25 s poll |
|---|---|---|
| Ogre (250) | 0.600 s | **16/16 reliable** |
| Knight / Longbowman (300) | 0.500 s | **16/16 reliable** |
| Archer / Pikeman / Cleric / Wizard / Sorcerer (350) | 0.429 s | 11/16 — **by luck** |
| Footman / Militia Mob (400) | 0.375 s | 8/16 — **by luck** |
| Sapper (500) | 0.300 s | 3/16 — **by luck** |
| Cavalry (600) | 0.250 s | **0/16 — never** |

⚠️ This matters because it is **worse in playtest terms** than "never fires": a feature that works for the slow half of the roster and fires by luck for three cards is reported as *"sometimes it works"*, which is far harder to diagnose than a dead one. **`K-6` is still right and I landed it** — test 16 *derives* 300 rather than transcribing it: a card is reliable only while `Radius / Speed ≥ 2 × Poll`, so the smallest radius making the whole roster reliable is `2 × Poll × FastestSpeed = 2 × 0.25 × 600 = **300 uu**`. Verified by driving the shipped predicate at 300: **all six cards 16/16.**

### B) `K-6`'s declared cost was understated ~3×, and the model behind it is wrong

The board costed it as *"the abduction window scales with `R` (~0.4·R) ⇒ ±60 → ±125 uu."* Measured at the 60 Hz reference cadence:

| walk speed | half-window at R=150 | at R=300 | ratio for a 2× radius |
|---|---|---|---|
| 250 (Ogre) | ±77 uu | ±216 uu | 2.8× |
| **300 (the law's own convention)** | **±62 uu** | **±204 uu** | **3.3×** |
| 400 (Footman) | ±25 uu | ±182 uu | 7.3× |

⇒ the real figure is **±62 → ±204 uu at 300 uu/s, not ±125**. The window grows **faster than linearly** because the dwell only ever eats a fixed `v × 0.35 s` of approach, which is a smaller fraction of a bigger disc — so *any* linear model understates it. **Test 16(c) asserts the super-linearity** (doubling the radius more than doubles the window) by driving the shipped predicate, and the re-derived comment on `LadderContactRadiusUU` now carries the measured number. 🧑 The trade is still Jonathan's; he should just be making it against ±204, not ±125.

---

## 6. Declared non-coverage (board item (3b))

**`AMinerUnit` can never contact-climb.** `MinerUnit.cpp:62` seals `StateCheckInterval = 0`, and `SetTimer` with a rate ≤ 0 does not schedule ⇒ no state poll ⇒ no ask. ⛔ **I did not re-open the miner's structural seal.** This is the same limit TASK-760's self-heal records, and here it is also the **wanted** answer (`qa/TASK-741` W-5). Asserted as a *discriminating* test in 17(c) — it proves the seal separates the two shipped classes rather than reading true for both.

**No stale-state residual:** the feature keeps no state at all, so a tower destroyed between polls leaves nothing behind.

---

## 7. Test list — suite delta **+3**, plus 2 compile-time pins

All in `SiegeLadderClimbTest.cpp` (14 → 17). Headless: CDOs, reflection and the pure statics — no world, no PIE, no asset loads, no writes. **Every expectation was verified against a faithful simulation of `WantsToClimb` at both 150 uu and 300 uu**, so none is red merely because `K-6` landed.

**Two `static_assert`s (board item (6)'s enforcement half):**
- `WantsActorTick` is pinned at **exactly two inputs**. ⭐ This is the row that "goes RED if the poll is ever moved onto the actor tick": the move is impossible without widening that predicate, so widening it is now a **compile error in this module** with a message naming `CONTACT-§11.5` / `SC-§33` and what is being reversed.
- `TryBeginContactClimb` is pinned at `ELadderContactVerdict (ACharacter*, float)` — the contract the call site compiles against, so a drift in `ClimbableTower.h` fails here by name instead of inside `SummonedUnit.cpp`.

**15 — `TheShippedContactPollIsCoarserThanTheDwellItIntegratesAndBothCostsAreMeasured`**
- (a) self-check the walker discriminates: dead-on ⇒ 16/16, skimming the disc edge (0.95 R) ⇒ 0/16 — both structural, so they hold at any radius.
- (b) the premise: the poll is >10× coarser than the design cadence; the dwell needs ≥2 polls; the dwell exceeds one poll.
- (c) the reliable / by-luck / unreachable partition, derived from `Radius`, `Poll` and the roster, driving the shipped predicate; names the card in every message.
- (d) sweeps offsets as fractions of the shipped radius and asserts a pawn exists that the 60 Hz sampler refuses on every phase and the poll **admits** — the coarse sampler manufacturing dwell.
- (e) ⭐ the readable twin of the arity pin: `WantsActorTick(false,false)` is **false**, with a self-check that it is not always-false, plus the timer driver's positivity.

**16 — `TheRadiusEveryRosterCardNeedsAtTheShippedPollIsTwicePollTimesTheFastestSpeed`**
- (a) `2 × Poll × FastestSpeed = 300 uu`, derived from the shipped poll and roster.
- (b) self-checks it is a genuine threshold: at 300 the fastest card is 16/16; one poll-period of travel less it is not. Neither row reads the shipped radius.
- (c) the super-linear cost — Finding B, as an assertion.

**17 — `TheContactTuningLivesOnlyOnTheTowerAndTheProbeIsValidatedAgainstTheClassThatHasIt`**
- (a) two-sided probe on the three tunable names. (b) substring ban with a live-walk self-check. (c) the miner non-coverage as a discriminating test.

---

## 8. For QA to scrutinise

1. **The armed tripwire at `SiegeClimbableTowerTest.cpp:1584`** — expected red, not mine to fix (§0).
2. **The delta hoist** in `UpdateState` — I claim it is behaviourally neutral for `TickStuckWatchdog`.
3. **`ForEachObjectOfClass` is a new idiom here** (zero prior uses). Check no gameplay call sits inside the lambda.
4. **The `return` after a successful ask** — the only thing between a contact climb and a second steering authority on the starting poll.
5. **Findings A and B change no code beyond the granted `K-6` line.**

**Files touched:**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SummonedUnit.h`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SummonedUnit.cpp`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeLadderClimbTest.cpp`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\ClimbableTower.h` — **item (4)'s two named lines ONLY** (the `300.f` default + its consequence comment)

⛔ Not touched: `ClimbableTower.cpp` · `HeroCharacter.{h,cpp}` · `SiegeLadderClimbStatics.{h,cpp}` · `SiegeClimbableTowerTest.cpp` · any mesh · `CONVENTIONS.md`. ⛔ No compile, no editor, no MCP, no Git.
