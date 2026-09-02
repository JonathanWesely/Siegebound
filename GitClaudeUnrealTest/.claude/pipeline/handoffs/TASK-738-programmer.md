# TASK-738 — [LADDER-3] THE CLIMB STATE + THE DISARM (`ASummonedUnit`)

**Agent:** gameplay-programmer · **Date:** 2026-09-01 · **Status:** `ready-for-qa`
**Law:** `TOWER-§8.4(B)` · `TOWER-§8.5` · `TOWER-§9` · `TOWER-§10` L-4/L-5 · `HIGH-§1` · `SHIP-§9c` · `NAV-§3`
**QA gate:** TASK-741 · **Compile + suite + commit:** TASK-742

---

## Files touched (SOLE ownership honoured)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | +~400 lines: the pure climb types, the pinned API, one tunable, four private members/helpers |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | the pure statics, the actor-side climb, 8 exit call sites, 3 disarm guard terms, 1 traversal fence, 3 tick-flag call-site changes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp` | **NEW** — 13 automation tests |

⚡ **AMENDED after delivery, on TASK-737's cross-task finding (the ladder mesh): the deck slab makes a swept ascent stall silently. See §5a — it changed the driver, the state and two more tests, and it exposed a second defect (the capsule-centre lift) that nobody had flagged.**

⛔ **`ClimbableTower.{h,cpp}` and `Tests/SiegeClimbableTowerTest.cpp` were NOT modified** (TASK-734's). I read them to verify the cross-task contract — see "Cross-task reconciliation" — and changed nothing.
⛔ No compile, no editor, no MCP, no Git. ⛔ `Tools/Packaging/` untouched. 🔒 Airlock clean: no `Capture()`, no `EnsureSnapshot()`, Zone A untouched, no token figure.

---

## 1. THE PUBLISHED API — transcribed from `TOWER-§8.4(B)`, ⛔ not redesigned

```cpp
// ─── ASummonedUnit, public. Declared by TASK-738; called by TASK-734. ───
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ASummonedUnit*, Unit, bool, bReachedTop);

UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit|Climb")
bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld);   // ⚠️ params RENAMED — see §13

UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit|Climb")
void AbortLadderClimb();

UFUNCTION(BlueprintPure, Category = "Siegebound|Unit|Climb")
bool IsClimbing() const;

UPROPERTY(BlueprintAssignable, Category = "Siegebound|Unit|Climb")
FSiegeLadderClimbEnded OnLadderClimbEnded;

// protected, EditDefaultsOnly
float LadderClimbSpeedUU = 350.f;
```

⭐ **Types, count and order character-for-character as pinned.** ⚠️ **ONE change: the two parameter NAMES** (`LadderFootWorld`/`LadderTopWorld` → `FromWorld`/`ToWorld`) — TASK-734's finding, taken. **Full reasoning and the amendment request in §13.**
The test file pins it a second way, at **compile time**: three `static_assert`s on member-function-pointer identity (`SiegeLadderClimbTest.cpp`, fixture namespace). A signature drift fails **in this module with a message naming the law**, rather than as a link error inside TASK-734.

---

## 2. HOW THE DISARM ATTACHES TO THE THREE SHIPPED GUARD POINTS

Jonathan: *"they should be attackable while climbing, but they can't attack back."*

**The gate is ONE pure function with TWO INDEPENDENT TERMS** (`SummonedUnit.h`):

```cpp
static bool IsAttackAllowed(bool bCanEverAttack, bool bClimbing) { return bCanEverAttack && !bClimbing; }
```

| Guard | Site | Term |
|---|---|---|
| **1 of 3** | `EnterAttack()` (`.cpp:1992`) | `if (!IsAttackAllowed(CanEverAttack(), IsClimbing())) { EnterIdle(); return; }` — stands DOWN to Idle, exactly as the class seal does |
| **2 of 3** | `UpdateStateGrouped()` (`.cpp:2760`) | same gate ⇒ `CurrentTarget = nullptr`, falls through to station-keeping |
| **3 of 3** | `PerformAttack()` (`.cpp:3047`) | joins the shipped `bDead \|\| !bStatsLoaded \|\| bAIFrozen \|\| bSpellFrozen` line |

⛔ **`CanEverAttack()` IS NOT OVERRIDDEN AND NOT TOUCHED.** It stays the `const` class-identity seal. Keeping the two terms separate is the requirement, not a style choice: a Sorcerer is disarmed with `IsClimbing() == false`, and a climbing Footman is disarmed with `CanEverAttack() == true`, so the code can still tell a permanent inability from a 3.5-second one. Test 8 asserts exactly that pair, plus the full 2×2 truth table.

⛔ **No fourth guard point. ⛔ No new suppression mechanism.**

**What actually stops the shots (the structural half):** `BeginLadderClimb` clears `AttackTimerHandle` (the `ApplyFreeze` idiom — the cadence timer is the only thing that can still land a hit), and `UpdateState`'s traversal fence means a climber runs **no decision loop at all**. The three guard points are therefore defence-in-depth in the shipped configuration — exactly the posture the `CanEverAttack()` seal already ships with (guard 3's own comment calls itself "belt-and-braces"). ⭐ Guard 3 is the one that would actually catch a shot if anything ever re-armed the timer.

---

## 3. ✅ "ATTACKABLE" — ⛔ NO MECHANISM WAS WRITTEN. A TEST PROVES IT.

Measured in the shipped code: the per-candidate acquisition gate in `AcquireTarget` / `AcquireEnemyNearPoint` (`.cpp:2095-2104`) is

```
Candidate != this  &&  IsTargetAlive(Candidate)  &&  enemy team  &&  inside a 2D disc
```

and `IsTargetAlive`'s only unit-state term is `IsUnitDead()` (`.cpp:3738-3741`). ⛔ No movement mode, ⛔ no Z, ⛔ no state. **A climber is already a valid target and nothing in this diff narrows that.**

**Test 9** proves it structurally: a reflection walk over `ASummonedUnit`'s own declared members, erroring on any name containing `Targetab` / `Untargetab` / `Acquirab` / `Invulnerab` / `Immun` / `Evasi` / `Dodge`, with a live-walk self-check.
⚠️ **What test 9 honestly CANNOT do, stated rather than papered over:** run an acquisition sweep with a climbing unit in it. That needs a `UWorld` — see §6. The behavioural half is Jonathan's playtest.

---

## 4. ⚠️⚠️ THE EIGHT `MOVE_Flying` EXITS — every one, and how each restores the mode

**ONE teardown, `EndLadderClimb(bReachedTop, Reason)`, reached from every exit. It is the ONLY place in the codebase that restores the climb's movement mode or broadcasts `OnLadderClimbEnded`.** Its first statement consumes an **exactly-once latch** (`FSiegeLadderClimbStatics::End`), so a double exit and a delegate listener that re-enters `AbortLadderClimb()` are both inert.

The restore is `StopMovementImmediately()` → `MaxFlySpeed = saved` → **`SetDefaultMovementMode()`** — the project's shipped idiom (`EndSpellFreeze` uses it), and it does the right thing in mid-air: with no movement base it goes **straight to `MOVE_Falling`** (`CharacterMovementComponent.cpp:1326-1338`), so the unit **drops from wherever it is** and survives (⛔ no fall damage, `TOWER-§4a`). Broadcast is **last**, after the restore.

| # | Exit | Call site | Reason | Test |
|---|---|---|---|---|
| 1 | **Arrival** | `TickLadderClimb` `.cpp:3874` | `Arrival` | **4** (arrival detection, 5 rows) + **5** (release) + **6** |
| 2 | **`AbortLadderClimb()`** | `.cpp:3806` | `Abort` | **6** row `Abort` |
| 3 | **A NEW ORDER (L-4)** | `AssignCommandGroup` `.cpp:2182` | `NewOrder` | **6** row `NewOrder` |
| 4 | **`HandleDeath`** | `.cpp:4054` (right after `bDead = true`) | `Death` | **6** row `Death` |
| 5 | **`FreezeAI`** | `.cpp:791` (right after `bAIFrozen = true`) | `MatchEndFreeze` | **6** row `MatchEndFreeze` |
| 6 | **`ApplyFreeze`** | `.cpp:1066` (before its `DisableMovement`) | `SpellFreeze` | **6** row `SpellFreeze` |
| 7 | **`EndPlay`** | `.cpp:732` (before `Super::EndPlay`) | `EndPlay` | **6** row `EndPlay` |
| 8 | **TOWER DESTROYED mid-climb** | **`AbortLadderClimb()`** — `AClimbableTower::EndPlay` calls it (`ClimbableTower.cpp:170`), **plus** this class's own `EndPlay` as the independent belt | `Abort` (+ `EndPlay`) | **6** rows `Abort` **and** `EndPlay`, and the double-exit rows |
| — | ⚠️ **Timeout (DECLARED NINTH — a watchdog, ⛔ not a law exit)** | `TickLadderClimb` | `Timeout` | **7** |

**⭐ EXIT 5 IS THE ONE THAT WOULD HAVE BEEN INVISIBLE:** `FreezeAI` stops timers, stops the walk and parks the unit Idle — it does **not touch the movement mode at all**. A climber frozen at match end would have kept `MOVE_Flying` and hung in the air over the end screen with every other line of that function perfectly correct.

**Order is load-bearing at exits 4 and 6:** both `HandleDeath` (rigged path) and `ApplyFreeze` deliberately call `DisableMovement()` (`MOVE_None`). The teardown runs **first** at both, so their `MOVE_None` wins — which is what we want.

⚠️ **DECLARED RESIDUAL at exit 6, ⛔ not a defect:** a unit frost-frozen mid-ascent is parked in `MOVE_None` **in mid-air** for the freeze duration and drops when `EndSpellFreeze` calls `SetDefaultMovementMode()`. That is the shipped freeze contract ("the pause holds") applied to a new surface — the climb is over, `IsClimbing()` is false, and a restore is guaranteed by the expiry timer, with `FreezeAI`'s match-end precedence as backstop. I did **not** change `ApplyFreeze`'s semantics to avoid it.

### ⚠️⚠️ THE TRAP I FOUND WHILE WIRING THIS — worth QA's eye

`ASummonedUnit`'s actor tick previously had **one** driver (the TASK-020 lunge), and `StopAttackLunge()` — which runs on **every** attack exit — called `SetActorTickEnabled(false)` unconditionally, as did `UpdateLunge`'s stray-tick guard. A climb begun on a unit that was mid-swing would have had its per-frame driver switched off underneath it, leaving it in `MOVE_Flying` **with all eight exits still perfectly correct**.
⇒ Added `RefreshActorTickEnabled()` = `bLungeActive || LadderClimb.bActive`, and **it is now the only writer of the tick flag** (verified by grep: 1 write site).

---

## 5. THE TRAVERSAL — and one term that is ⛔ NOT a fourth guard point

`MOVE_Flying` + swept `AddMovementInput` along the ONE straight `Foot → Top` segment, capped by `MaxFlySpeed = LadderClimbSpeedUU`. ⛔ No `MOVE_Custom`, ⛔ no `SetActorLocation`/`TeleportTo` lerp.

⚠️ **`UpdateState()` gained an `IsClimbing()` early-out (`.cpp:1510`), and it is a MOVEMENT concern.** Without it a 0.25 s state poll landing mid-ascent would run the standing body, acquire a target and call `EnterAdvance` — handing the pawn to **path following** while it is in `MOVE_Flying`. Path following writes the *same input vector* the climb steers with (`FollowPathSegment` → `RequestPathMove` → `AddInputVector`), so the unit would be dragged horizontally off its own ladder by a second steering authority — the exact double-drive `NAV-§3` forbids. It sits on the line that already carries `bAIFrozen`/`bSpellFrozen`. **Please do not read it as a fourth attack guard; the code comment says so explicitly.**

**Other state `BeginLadderClimb` touches, each with a reason:**
- clears `AttackTimerHandle`, calls `StopAttackLunge()` (no mid-swing pose riding up the ladder)
- `AI->StopMovement()` — one steering authority
- **the SIXTH sidestep-lease clear site, DECLARED not buried** (the `ApplyFreeze` "declared fifth exit" precedent): `UpdateState` early-outs for the whole climb, so `TickStuckWatchdog` never runs and the lease can never *drain*. A unit that landed would resume steering to a `SidestepGoal` chosen before it left the ground. `FSiegeStuckStatics::Reset(StuckState)` also stops the ascent being charged as stall time.
- `State = Idle`, `CurrentMoveGoal = nullptr` (the `ApplyFreeze` idiom, written directly rather than via `EnterIdle()` whose `State == Idle` early-out would skip the goal clear)
- ⛔ **`CurrentTarget` is deliberately KEPT** — the one place `ApplyFreeze`'s pattern is *not* copied. `TOWER-§9.2` rules the disarm ends the **instant** the deck is reached; dropping the target would force a re-acquire on landing and cost up to one 0.25 s poll of enforced silence. That is a lingering penalty by another name.
- ⛔ **Healing is NOT stopped** — see the flagged decisions below.

---

## 5a. ⚠️⚠️ THE DECK SLAB — TASK-737's finding, and the driver change it forced (ADDED after the first delivery)

**The measurement, relayed and then re-derived here before I touched anything:** `LadderTop` is pinned **150 uu inside a SOLID deck slab** and is approached from below at 76°, so **the last 40.8 uu of the climb line lies inside the deck geometry.**

I re-derived it rather than taking it on relay, and it checks out exactly — **and it is worse than 40.8 uu suggests:**

| Quantity | Value |
|---|---|
| Line length `sqrt(300² + 1200²)` | 1,236.93 uu |
| Line per unit of Z (`1 / sin 75.96°`) | 1.03078 |
| **Slab thickness implied by 737's 40.8 uu** | 40.8 × sin 76° = **39.58 uu of Z** ⇒ a ~40 uu slab ✅ consistent |
| ⚠️ **The 176 uu capsule's own traverse of the deck plane** | 176 × 1.03078 = **181.4 uu of line** |

⇒ ⛔ **THE SWEEP DOES NOT STALL AT THE SLAB — IT STALLS ~131 uu BELOW IT**, because the capsule's *top* reaches the slab's underside long before its centre does. **The unit stops roughly a capsule-height short of the deck, silently, with all eight exits still perfectly correct.** ⚠️ Exactly the class of the `StopAttackLunge` tick bug: a correct state machine driven by a mechanism that quietly cannot run.

### ⚠️ AND A SECOND DEFECT THE SAME MEASUREMENT EXPOSED, WHICH NOBODY HAD FLAGGED

The sockets sit on **generated navmesh** (`TOWER-§8.3`) — i.e. on the ground and on the deck **surface**. But the thing that travels the line is the capsule's **CENTRE**, which stands one half-height above whatever it is on. **My first delivery drove the capsule centre to the bare socket** ⇒ the unit would have "arrived" with its **feet 88 uu below the deck, buried in the slab**, and depenetration would have dropped it back down the tower. ⛔ A non-swept final stretch alone would NOT have fixed the feature; it would have teleported the unit *into* the slab.

### THE FIX — two parts, both in the pure core so both are testable

1. **SURFACE → CAPSULE-CENTRE LIFT.** `Begin` lifts *both* endpoints by the capsule half-height, **read from `GetCapsuleComponent()->GetScaledCapsuleHalfHeight()`** — ⛔ never a literal (the `TOWER-§7` "geometry comes from the thing" principle, applied a third time; a BP child that resized its capsule stays correct for free). ⭐ The same lift at both ends, so `LengthUU`, the direction and the watchdog budget are **all unchanged**.
2. **A NON-SWEPT WINDOW AT THE LINE'S ELEVATED END** — `FSiegeLadderClimbStatics::ShouldSweep()`. Inside it the driver stops feeding `AddMovementInput` and instead drives a **continuous non-swept step** at the same `LadderClimbSpeedUU`.

**Window size — derived, with the arithmetic written out (`DeckBreachCapsuleHalfHeights = 3`):**

| Term (in Z, at the elevated end) | Why it earns its place |
|---|---|
| 1 × 88 | the capsule's **top** reaches the slab's underside before its centre does |
| 1 × 88 | allowance for how far the slab hangs **below its own surface**. ⚠️ **An assumption** — this file cannot read the mesh — and it is **2.2× the ~40 uu that ships** |
| 1 × 88 | the capsule's **bottom** must clear the deck **surface** before sweeping is safe again |
| **= 264 uu of Z** | → **272.1 uu of line** (converted by the line's *own* slope, ⛔ not a hardcoded `sin 76°`) |

⭐ **Why being generous here is FREE — the part that makes this safe rather than merely necessary:** `TOWER-§8.3` pins **≥56 uu of standoff between the capsule surface and the tower body face along the WHOLE line.** So the only static geometry a non-swept stretch anywhere on this line can pass through is **the deck slab itself** — the very thing it must. The sweep's remaining job is pawn-vs-pawn, and that is covered: `TOWER-§10` L-1 is one-climber-at-a-time, and a unit already standing on the deck is resolved by the same depenetration that resolves any two of the 4–6 bodies `TOWER-§4` expects up there. **78% of the line stays swept.**

⚠️ **If the slab is thicker than one capsule half-height, the sweep stalls before the window opens — and that fails LOUDLY through the watchdog's warning rather than silently.** That is the whole reason the watchdog (declared deviation 2) exists, and this is the second failure it now catches.

**Three more details, each deliberate:**
- ⭐ **It is a CONTINUOUS drive, ⛔ not a teleport** — same rate, same straight line, no visual discontinuity. A unit that popped 272 uu up a ladder would read as broken, which is the exact failure `A_SiegeBiped_Climb` (TASK-733) exists to prevent.
- ⚠️ **Velocity is zeroed each breach frame** (`StopMovementImmediately`), or the two drivers fight: `PhysFlying` would keep sweeping the capsule from residual velocity (`BrakingDecelerationFlying` is 0, so it never decays) and re-jam it against the slab we are stepping through.
- ⭐ **Arrival snaps to the exact target**, non-swept — so the unit finishes standing ON the deck rather than wherever the last frame's step landed. ⛔ **Only on a real arrival**: a timed-out climb drops from where it actually is and is never handed the deck it failed to reach.

**⭐ The window rides the line's ELEVATED end, ⛔ not "the last stretch"** — because on a **descent** the unit starts *standing on the deck* and must pass **down** through the slab it is standing on, where a swept move jams on **frame one**. `bDeckIsAtEnd` resolves it by comparing Z. ⚠️ **That is a fact about the geometry, ⛔ not an ordering constraint on the API** — both argument orders remain legal, and test 13 exists to keep it that way.

---

## 6. ⚠️ WHY THE TESTS ARE PURE-CORE + CDO + REFLECTION — a measurement, ⛔ not a preference

I intended to drive a world-less `ASummonedUnit` through the eight exits and read `MovementMode` back. **That would CRASH the suite, not fail it:**

> `UCharacterMovementComponent::SetDefaultMovementMode()` → `CanEverSwim() && IsInWater()` → `UMovementComponent::GetPhysicsVolume()`, which is
> ```cpp
> if (UpdatedComponent) { return UpdatedComponent->GetPhysicsVolume(); }
> return GetWorld()->GetDefaultPhysicsVolume();   // ← unconditional deref
> ```
> (`MovementComponent.cpp:290-298`). A `NewObject`'d unit has no world and no registered `UpdatedComponent`.

And this project has **zero** world-creating tests (`UWorld::CreateWorld` = 0 hits, `SpawnActor` = 0 hits across `Siegebound/Tests/`).

⇒ Every **decision** the climb makes lives in `FSiegeLadderClimbStatics` (the `FSiegeStuckStatics` / `HeightAdvantageMultiplier` seam precedent) and is exercised exhaustively; the **wiring** of the eight call sites is TASK-741's diff read. ⛔ Stated rather than hidden (`SC-§32`).

`SetMovementMode` itself **is** world-safe (`CharacterMovementComponent.cpp:1392-1396`: *"We allow setting movement mode before we have a component to update"*) — it is only the restore path that is not.

---

## 7. THE TEST LIST — 11 tests, and what each would catch

| # | Test | What it would catch if it went red |
|---|---|---|
| 1 | `RefusesEveryPinnedReasonAndARefusedBeginChangesNothing` | a dropped refusal term (dead / `bAIFrozen` / `bSpellFrozen` / already-climbing, one row each); a **half-armed** refusal (every field checked, ⛔ not just `bActive`); a refused `Begin` **stealing a live climb's line or resetting its clock** |
| 2 | `BeginArmsTheLineAndSizesTheWatchdogFromTheRateAtArmingTime` | a wrong line length; a watchdog budget so tight it could cut a healthy climb short; a **divide-by-zero on a 0 rate**; a mid-climb retune extending a running climb; a sub-second budget on a short ladder |
| 3 | `TheSteerIsMostlyVerticalWhichIsExactlyWhatAWalkingPawnWouldHaveDeleted` | someone "simplifying" the steer to horizontal. **Asserts the `TOWER-§8.1` M-4 measurement directly**: plane-project the steer as `ConstrainInputAcceleration` would and 97% of its magnitude is gone. Also re-derives the 76.0° lean and its distance above Recast's 32.005° ceiling |
| 4 | `ArrivesTheInstantTheDeckIsReachedAndNotBefore` | a too-large arrival radius (⭐ the "three tolerance-widths short still climbs" row is what makes the tolerance mean anything); a missing **overshoot** test (a 20 fps step covers 17.5 uu > the 16 uu radius and would sail past); a phantom arrival on an un-armed state; a negative delta running the clock |
| 5 | `TheDisarmReleasesOnTheVeryStepTheDeckIsReachedWithNoGraceWindow` | a decay timer, a grace window, or a **second flag** — the whole-struct reset is asserted field by field. Also asserts the gate flips `false → true` on that same step |
| 6 | `EveryExitEndsTheClimbExactlyOnceSoTheMovementModeIsAlwaysRestored` | ⭐⭐ **the headline.** All 8 reasons × (first exit fires · second does nothing · third does nothing · state pristine). A latch hard-wired to `true` (self-check), a **double broadcast** on death→EndPlay, a reason-dependent teardown, and a 9th enum value added without a row |
| 7 | `TheWatchdogDropsAHangingClimberButAnArrivalAlwaysWins` | a trigger-happy watchdog (half-budget row); **the ordering bug** — a unit reaching the deck on the frame its budget expires must ARRIVE, not be dropped off a tower it had already reached; a budget a healthy 3.53 s ascent could reach |
| 8 | `ATransientClimbIsDistinguishableFromTheSorcerersPermanentClassSeal` | the disarm implemented via `CanEverAttack()`. Full 2×2 truth table (a one-term gate would not compile) + the three CDOs: `ASummonedUnit` true / `ASorcererUnit` false / `AMinerUnit` false, all three `IsClimbing() == false` |
| 9 | `AClimberIsStillAValidTargetBecauseNothingWasAddedThatCouldMakeItNotOne` | a future "climbers can't be hit" mechanism — a token scan over declared members, with a live-walk self-check. **The free half, proven as an absence** |
| 10 | `ThePinnedApiIsPresentUnderItsPinnedNamesTypesAndReflection` | a rename or type drift that would break TASK-734: the 3 UFUNCTIONs by name, the delegate's **arity and both parameter types**, and `LadderClimbSpeedUU` being a float UPROPERTY that is still **EditDefaultsOnly** and still 350 |
| 11 | `TheShippedRateReproducesTheExposureTableJonathanWasHanded` | a silent retune that invalidates `TOWER-§9.3`. Recomputes the whole table from the shipped default, including the ramp comparison — ⭐ with a self-check at 250 uu/s that yields **three** shots, so the table provably tracks the rate rather than being baked in |
| **12** ⭐ *NEW* | `TheFinalStretchIsNotSweptSoTheClimberLandsOnTheDeckNotJammedUnderIt` | ⭐⭐ **the TASK-737 finding, and it is the row that fails if the stretch were swept.** (a) self-check that the foot and midpoint ARE swept — without it a whole-line teleport would pass everything else, and that is what `TOWER-§8.5` refuses; (b) **not swept at the deck**, nor one capsule-height below it; (c) the window covers the capsule's full traverse **plus 737's measured 40.8 uu**, and is still under half the line; (d) ⭐⭐ **the final FEET position is exactly `LadderTop.Z`**, with both traps named — ⛔ **not** ~40 uu below (the swept-stall position) and ⛔ **not** 88 uu below (the un-lifted-line bug); (e) the window rides the elevated end on **descent** too |
| **13** ⭐ *NEW* | `DescendingIsTheSameCallWithTheEndpointsSwappedAndNothingEnforcesAnOrdering` | a future "tidy-up" enforcing foot-then-top and **silently breaking climbing down** (TASK-734's finding). Both directions admit; the descent's direction is the **exact negation** of the ascent's (the self-check that stops `Begin` ignoring its arguments); each arrives where it was *sent*, ⛔ not where Z would sort it; and the deck is identified by **height**, so `bDeckIsAtEnd` is false on a descent |

⚠️ **One shipped guard is deliberately NOT exercised: `CanBegin`'s `ContainsNaN()` refusal.** This build runs with `ENABLE_NAN_DIAGNOSTIC == 1` (it is 0 only in Shipping/Test), so merely *constructing* an `FVector` holding a NaN calls `TVector::DiagnosticCheckNaN()` and raises an engine error — the test would go red on its own fixture rather than on its subject. Covered by QA's diff read instead. Noted in the file.

### 📊 SUITE TOTAL — ⚠️ AND THE ON-DISK NUMBER IS NO LONGER THE LADDER BATCH'S NUMBER

| | Tests |
|---|---|
| Baseline (`TASKBOARD` figure) | **171** |
| **TASK-738 delta** | **+13** (new file `SiegeLadderClimbTest.cpp`; was +11, **+2 for the deck-slab and both-ways tests**) |
| TASK-734 delta (their file, 5 → 10) | **+5** |
| **⭐ LADDER-BATCH TOTAL** | **189** |

⚠️⚠️ **BUT THE ON-DISK TOTAL IS 206, AND TASK-742 MUST NOT ASSERT 189 BLINDLY.** Two test files from tasks **outside this batch** landed on disk while I was working:

| File | Tests | Owner |
|---|---|---|
| `SiegeGhostPawnTest.cpp` | 8 | ⛔ not this batch (untracked, appeared during TASK-738) |
| `SiegeMapMarkTest.cpp` | 9 | ⛔ not this batch (untracked, appeared during TASK-738) |

✅ **It reconciles exactly: 171 + 13 + 5 + 8 + 9 = 206**, and `grep -c` across `Siegebound/Tests/*.cpp` = **206**.

⇒ 🔧 **FOR TASK-742: the number to assert is whatever is on disk at compile time, and the LADDER batch's contribution to it is +18 (13 + 5).** ⛔ **Do not treat a total above 189 as a discrepancy** — reconcile against the two foreign files first. ⚠️ The coordinator's quoted "batch total was 187" is already stale for the same reason.

---

## 8. ⚠️ THE EXPOSURE TABLE, RE-STATED AGAINST THE RATE I ACTUALLY SHIPPED

**Shipped rate: `LadderClimbSpeedUU = 350.f`. ⛔ Unchanged from the law, so `TOWER-§9.3`'s table stands as written** — recomputed here rather than copied, and reproduced in test 11.

- Climb line **1,236.9 uu** (`sqrt(300² + 1200²)`, from the pinned sockets) ÷ 350 uu/s = **T = 3.534 s** of helplessness.
- One defending Longbowman (Dmg **18**, Cadence **1.5 s**, Range **3,600**): `floor(3.534 / 1.5)` = **2** shots, **3** if already firing when the climb starts.
- **36–54 damage.**

| Climber | HP | Best case | Worst case |
|---|---|---|---|
| **Archer / Wizard** | **45** | ⚠️ survives at **9 HP** | ⛔ **DIES** |
| **Longbowman** | 70 | survives at 34 HP | ✅ survives at 16 HP |

- ⚠️ The Longbowman is firing from **3,600 uu — 4.8× the tower's entire 750 uu footprint.** It is not even in the fight.
- ✅ **Counterweight, measured:** the ramp was 2,078 uu of run at 30° = 2,399 uu of surface = **6.86 s**. **The ladder is 3.53 s — 48% less exposure.** What changed is not the duration; it is that the climber cannot answer.
- ✅ The ground shooter gets **no height bonus** firing upward (`HIGH-§2`), and the climb is **abortable** — the unit drops free, no fall damage.
- ⛔ **Not softened.** The lever is `LadderClimbSpeedUU`, and it is his (row **T-7**).

⚠️ **ONE HONEST CAVEAT ON THE NUMBER, MEASURED AND TABLE-NEUTRAL:** `MaxAcceleration` is the engine default 2048 uu/s², so the unit reaches 350 uu/s in ~0.17 s and loses ~30 uu to the ramp-up ⇒ the real window is **≈3.62 s**, not 3.534. `floor(3.62 / 1.5)` is still **2**, so **every row of the table above is unchanged**. I did not touch `MaxAcceleration` — it is shared with walking, and restoring it exactly would be one more thing to get wrong.

---

## 9. ⚖️ DECLARED DEVIATIONS AND FLAGGED DECISIONS — for QA to rule on

1. **⭐ A NEW TEST FILE rather than extending an existing one.** Spec (7) says "extend the existing unit test files — CHECK FIRST". I checked: there is no `ASummonedUnit` test file; `SiegeHighGroundTest.cpp` is a *different feature* on the same class, and `SiegeClimbableTowerTest.cpp` is **TASK-734's fenced file**. Every test file in this project is feature-named. ⇒ `Tests/SiegeLadderClimbTest.cpp`. **⛔ Not a duplicate frame** — no existing test covers any of this.
2. **⭐ A ninth exit REASON: `Timeout` (a watchdog).** Not in the law's eight. **Justification:** "hangs in mid-air forever" is this feature's named catastrophic failure, and geometry that blocks the sweep produces it *with all eight exits correct*. It routes through the same teardown, so it adds no new restore path. `TimeoutScale = 4.f` (14.1 s against a 3.53 s ascent) — deliberately far too generous to ever end a healthy climb. **Happy to remove it if QA rules it out of scope.**
3. **⭐ A second `EditDefaultsOnly`-adjacent constant set** (`TimeoutScale`, `ArrivalToleranceUU`, `MinClimbLineUU`, `MinTimeoutSeconds`, `MinClimbSpeedUU`) — all `static constexpr` on `FSiegeLadderClimbStatics`, **not** UPROPERTYs, so they add **zero** designer surface and cannot be confused with Jonathan's one lever. `TOWER-§8.5`'s "the climb rate is ONE `EditDefaultsOnly` float" is honoured exactly: **`LadderClimbSpeedUU` is the only new UPROPERTY.**
4. **⭐ A FIFTH refusal reason in `CanBegin`: a degenerate (< 1 uu) or NaN line.** A **math guard**, not policy — `ClimbDirection` would normalise a zero vector and the unit would float steering at nothing. Deliberately did **not** add `!bStatsLoaded` (the pinned doc comment enumerates exactly four policy reasons; adding a fifth would be a silent divergence from a contract 734 compiles against).
5. **⛔ A CLIMBING CLERIC STILL HEALS.** `PerformHeal` is untouched. His ruling is *"they can't **attack** back"*, and the shipped precedent is explicit: `UpdateStateFollow`'s "A FOLLOWING CLERIC STILL HEALS (manager ruling 9 — healing is not attacking)". Stopping it would extend his ruling into something he did not ask for, which `TOWER-§9` forbids **in both directions**. **Flagged for QA — reverse it in one line if ruled otherwise.**
6. **⭐ `AddMovementInput(..., bForce = true)`** — path following passes `false`. `APawn::Internal_AddMovementInput` drops the vector whenever `IsMoveInputIgnored()`, which returns **true for a pawn with no controller at all** — a real window, since `AutoPossessAI` possession can land after `BeginPlay` (the miner's controller poll exists for exactly that). A scripted traversal the tower is *waiting on* must not be silently suppressible.
7. **⭐ `CurrentTarget` preserved across a climb** where `ApplyFreeze` clears it — reasoned in §5. This one is a direct consequence of `TOWER-§9.2`'s "the instant the deck is reached".
8. **⚠️ Guard 2 (`UpdateStateGrouped`) is UNREACHABLE during a climb**, because the `UpdateState` fence returns first. The term is present because `TOWER-§9.2` names those three points and because the shipped seal is defence-in-depth at all three — **not** because it is the mechanism. Said plainly in the code comment so it does not read as dead code.
9. **⚠️ WATCH ITEM I CANNOT FIX FROM INSIDE MY FENCE: `AMinerUnit`'s independent arrival poll.** `UpdateMining` → `EnsureWalkingToNode` is a second steering authority that `UpdateState`'s fence does **not** cover, and `CanTeamAscend` is FROZEN with no unit-type term, so a miner *could* be admitted to a ladder. Mitigations: a miner has no reason to reach a deck (no gold nodes there), and the watchdog **drops** rather than hangs. `MinerUnit.{h,cpp}` is outside TASK-738's file set — **raised, ⛔ not silently fixed.**
10. **⚠️ Interaction worth a QA eye (⛔ not a defect):** on arrival the `UpdateState` fence lifts, so the unit's next 0.25 s poll may issue its own `MoveTo` and supersede the path following TASK-734 just resumed. That is the shipped behaviour of this project for *every* unit (`UpdateState` re-decides every poll); noted because it is new *on this surface*.

---

11. **⚠️⚠️ A SCOPED, MEASURED EXCEPTION TO `TOWER-§8.5` — AND IT IS THE ONE THING IN THIS DIFF THAT NEEDS A LAW RULING.** That section refuses **"A RAW `SetActorLocation` / `TeleportTo` LERP … OUTRIGHT"**, naming three harms. The deck-breach window (§5a) uses `SetActorLocation(..., bSweep = false)` for the final **272 uu (22%)** of the line.
    **The three harms, one by one:**
    - *"through the tower body"* — ⛔ **still refused.** `TOWER-§8.3` pins ≥56 uu of standoff from the body face **along the whole line**, so the body is not in this window.
    - *"through other units"* — ⛔ **still swept for 78% of the line**; L-1 is one-climber-at-a-time, and a unit on the deck is resolved by the depenetration `TOWER-§4` already relies on.
    - *"through the deck"* — ⭐ **TASK-737 has since MEASURED this to be REQUIRED**, because the pinned socket is 150 uu inside a solid slab and the mesh cannot be changed without the castle-floor defect class.
    ⇒ **The law was written before that measurement existed.** ⭐ It is also **not a lerp of the traversal**: 78% is genuine swept `MOVE_Flying`, and the window is a continuous drive at the same rate, ⛔ not a teleport. **REQUESTED AMENDMENT: `TOWER-§8.5` should carry the deck-breach exception explicitly, scoped to the elevated end and justified by the standoff contract.** ⛔ **I have not edited `CONVENTIONS.md`** — QA/manager rule on it.
12. **⭐ The capsule half-height lift** (§5a part 1) changes where a climb *ends* versus my first delivery. It is a **defect fix**, not a preference: without it the unit arrives with its feet 88 uu below the deck. Called out separately because it is invisible in a diff that also changes the sweep.
13. **⭐ THE PINNED PARAMETER NAMES ARE RENAMED: `LadderFootWorld`/`LadderTopWorld` → `FromWorld`/`ToWorld`.** TASK-734's suggestion; I agree and took it. **Why it matters, in their words and mine:** the link is `BothWays` (`TOWER-§8.7`), so a descent passes the endpoints the other way round — and names containing "Foot"/"Top" imply an ordering a future "fix" could *enforce*, **silently breaking climbing down** and manufacturing exactly the stranded unit `TOWER-§8.7` exists to prevent. I also renamed the state's fields (`Foot`/`Top` → `Start`/`End`) for the same reason, and **test 13 now makes the ordering-freedom assertable.**
    ✅ **Safety of the rename, checked:** parameter names are **not part of a function's type**, so the `static_assert`s and TASK-734's call site are untouched (it calls positionally with locals already named `FromWorld`/`ToWorld`). The reflected UFUNCTION pin names change, and **nothing binds them yet** — TASK-739 reads only `IsClimbing`. **REQUESTED AMENDMENT: the `TOWER-§8.4(B)` code block should read `FromWorld`/`ToWorld`.** ⛔ **I have not edited `CONVENTIONS.md`.**

---

## 10. 🤝 CROSS-TASK RECONCILIATION WITH TASK-734 (read-only; ⛔ their files unmodified)

I read `ClimbableTower.{h,cpp}` and `SiegeClimbableTowerTest.cpp` **to verify the contract only.** ✅ **The two lanes converge exactly:**

- `ClimbableTower.cpp:426` binds `HandleLadderClimbEnded(ASummonedUnit*, bool)` — **matches my delegate signature exactly.**
- `ClimbableTower.cpp:433` calls `Unit->BeginLadderClimb(FromWorld, ToWorld)` → `bool`; `:170` calls `Climber->AbortLadderClimb()`. ✅
- `ClimbableTower.cpp:435-443` handles the **refused** case itself (`ReleaseClimber` + a Verbose log). ✅ Correct — **my `BeginLadderClimb` broadcasts NOTHING on refusal** (a refused Begin changes nothing at all). Their comment anticipating a *synchronous* broadcast is still safe: it binds before the call, which is right for the instant-arrival case (a unit already standing on `LadderTop` arrives on its first tick, not inside `Begin`).
- **Ordering is compatible:** my teardown restores the movement mode **before** broadcasting, so their `ResumeAgentPathFollowing` never sees a half-torn-down unit.

### ⚠️ THE COLLISION THAT WOULD HAVE FAILED THE GATE — already resolved by 734, recorded so QA can confirm it

`SiegeClimbableTowerTest.cpp`'s `HIGH-§3` decoupling scan (test 5(d)) walks **every reflected member `ASummonedUnit` declares** and errors on banned substrings. Its **previous** list contained **`"Climb"`** — which the law's own pinned API (`BeginLadderClimb`, `AbortLadderClimb`, `IsClimbing`, `OnLadderClimbEnded`, `LadderClimbSpeedUU`) would have tripped **five times**, reddening the suite at TASK-742 with correct, specified code on both sides.

✅ **TASK-734 has already narrowed the list to `Tower` / `Platform` / `Occupan` / `Ascen` and added a self-check requiring `IsClimbing` to be FOUND** (`SiegeClimbableTowerTest.cpp:148-161`, `:634`). **Verified against my shipped surface: none of my reflected members contains any of those four tokens, and `IsClimbing` is present as a reflected UFUNCTION** — their self-check passes. **⛔ No action needed; recorded because it is a genuine cross-task hazard that neither task could see from its own side.**

---

## 11. M8 + AIRLOCK DECLARATION

- ⛔ **No replicated property, no new replicated class, no new relevancy tier, no RPC, no class-tier change.**
- ⭐ **And the reason is structural:** `FSiegeLadderClimbState` and `ESiegeLadderExit` are **deliberately unreflected** (the `FSiegeStuckState` / `ESiegeStuckAction` discipline, `NAV-§8`/`§11`) — an unreflected type cannot be replicated by accident, which is what makes this declaration survive a later refactor rather than merely be true today. The one new UPROPERTY (`LadderClimbSpeedUU`) is CDO config, identical on every machine by construction.
- 🔒 No `Capture()`, no `EnsureSnapshot()`, Zone A untouched, the 552 latch untouched, ⛔ no token figure (`AS-§12g`).

---

## 12. WHAT QA SHOULD SCRUTINISE HARDEST

1. **The eight exits, one by one, at the call sites** — the tests prove the latch, ⛔ not the wiring (§6). Confirm each site calls the teardown and that ordering at `HandleDeath` / `ApplyFreeze` puts it **before** their `DisableMovement()`.
2. **`RefreshActorTickEnabled` is the ONLY writer of the tick flag** (grep: 1 site). A future `SetActorTickEnabled(false)` anywhere in this class is a latent hang.
3. **The `UpdateState` fence** — please rule whether it counts as a fourth guard point (my argument that it is a movement concern is in §5 and in the code).
4. **Flagged decision 5** (the climbing Cleric still heals) and **flagged decision 2** (the watchdog).
5. **The suite arithmetic (§7).** ⚠️ **The LADDER batch is +18 (13 + 5) on a 171 baseline = 189, but the on-disk total is 206** because two non-batch test files landed mid-task. **Reconcile before calling it a discrepancy.**
6. **Watch item 9** — `AMinerUnit`'s second steering authority, out of my fence.
7. ⚠️⚠️ **DEVIATION 11 — the scoped `TOWER-§8.5` exception (§5a).** This is the ruling I most need: a measured, declared, 22%-of-the-line non-swept window, forced by TASK-737's geometry. **The alternative is a feature that silently does nothing**, so "refuse the exception" must come with a different mechanism, not a revert.
8. **The two named traps in test 12(d)** — `LadderTop.Z − 40.8` (swept-stall) and `LadderTop.Z − 88` (un-lifted line). Both were real, reachable outcomes of this diff before the fix; please confirm the assertions actually exclude them.
