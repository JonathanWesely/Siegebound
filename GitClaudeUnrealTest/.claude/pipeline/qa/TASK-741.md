# QA Report — TASK-741 — [GATE-1] THE ONE QA GATE over the whole ladder batch

**Verdict: PASS**
**Blockers: 0 · Warns: 8 · Nits: 6**

**Artifact under review (one artifact, four tasks):** TASK-733b (clip re-export) + TASK-734 (`AClimbableTower` nav link) + TASK-737 (`SM_WatchTower` mesh) + TASK-738 (`ASummonedUnit` climb + disarm).
**Law read before code, in this order:** `TOWER-§8.5a` (all eight clauses + the voiding condition) → `TOWER-§8.4(B)` **as amended** (`FromWorld`/`ToWorld`, no Z-ordering) → `TOWER-§8.3` **as amended** (three pinned numbers: climb line · ≥56 uu standoff · rung plane −22.0) → `§8.5` · `§8.6` · `§8.7` · `§9` · `§10` · `§8.0`'s scope fence.
**Reviewed from:** `handoffs/TASK-733-artist.md` (which carries the TASK-733b section), `TASK-737-artist.md`, `TASK-738-programmer.md`, `TASK-734-programmer.md`, plus the shipped diffs in `SummonedUnit.{h,cpp}`, `ClimbableTower.{h,cpp}`, `Tests/SiegeLadderClimbTest.cpp`, `Tests/SiegeClimbableTowerTest.cpp`.
**⛔ TASK-739's ABP wiring and the clip IMPORT are OUT OF SCOPE** and were not judged. Nothing below requires `ABP_Footman` to exist.
**Fences honoured:** no edits · no compile · no engine/MCP · no Git · no TASKBOARD write · `ABP_Footman` not opened, not saved.

---

## 0. ⭐ HOW THIS GATE READ THE LAW — stated first, because `§8.5`'s stale bullet is a live trap

`TOWER-§8.5`'s third bullet refuses "a raw `SetActorLocation`/`TeleportTo` LERP **outright**". **This gate did NOT rule against the shipped deck-breach window on that bullet.** `§8.5` itself points at `§8.5a`, `§8.5a` is a granted, scoped, conditional exception, and `§8.5a`'s own "WHAT A QA GATE MUST DO" block instructs the gate to check **the eight clauses** instead. That is what §1 below does, clause by clause.

**The licensing condition was re-checked, ⛔ not assumed:** `§8.5a` is licensed by `TOWER-§8.3`'s **≥56 uu standoff and nothing else**, and the standoff is voided by any `SM_WatchTower` re-author that does not re-measure it. **TASK-737 IS that re-author, and it DID re-measure and report: 59.624 uu worst case over the whole line** (`TASK-737-artist.md` §1 row "Standoff", §5). ⇒ **the exception is LIVE, not void.** TASK-737 also reported all three of `§8.3`'s pinned numbers (climb line 1,236.9317 · standoff 59.624 · and — via TASK-733b's independent parse of the same FBX — the rung plane −22.000), so the re-author handoff is **complete** under the `§8.3` mesh↔clip binding box.

---

## 1. ⭐ THE DECK-BREACH WINDOW (TASK-738) — `§8.5a`'s EIGHT CLAUSES, ONE BY ONE

| # | Clause | Shipped at | Verdict |
|---|---|---|---|
| **1** | Rides the line's **ELEVATED end, resolved by Z** — ⛔ never "the last stretch", ⛔ never argument order | `SummonedUnit.cpp:194` `State.bDeckIsAtEnd = (State.End.Z >= State.Start.Z)`; consumed at `:239` `DeckEnd = bDeckIsAtEnd ? End : Start` | ✅ **MET.** The window is anchored to geometry. Test 12(e) proves the descent case (`Descent.Start` non-swept, `Descent.End` swept); test 13(d) proves `bDeckIsAtEnd` is false on a descent. **The descent-jams-on-frame-one failure is genuinely prevented.** |
| **2** | **≤ 3 × capsule half-height in Z**, converted to line length by the line's **OWN slope** (⛔ never a hardcoded `sin 76°`) | `SummonedUnit.cpp:202-203` `BreachZUU = 3 × HalfHeight` (= **264.0 uu of Z**, exactly the ceiling) then `× (LengthUU / RiseUU)` — the line's own slope, computed from its own endpoints at `:199` | ✅ **MET, at the ceiling and not over it.** 264 × 1.03078 = **272.1 uu of line = 22.0 %** of 1,236.9. ⛔ No `sin`, no literal, no 76°. *(See **W-4** for the shallow-line clamp.)* |
| **3** | **Continuous drive at the climb rate — ⛔ not a teleport, ⛔ not a lerp of the traversal** | `SummonedUnit.cpp:3996-3998` — `StepUU = max(LadderClimbSpeedUU, MinClimbSpeedUU) × max(DeltaSeconds, 0)`, applied **per frame** along the same `ClimbDirection` | ✅ **MET.** Same rate, same straight segment, one frame's worth per frame. ⛔ There is no single-step jump to `End` anywhere except the arrival snap (clause 5). |
| **4** | **Velocity zeroed on EVERY breach frame** | `SummonedUnit.cpp:3991-3994` — `Movement->StopMovementImmediately()` inside the non-swept branch, unconditionally, before every step | ✅ **MET.** And the reasoning checks out at source: no `AddMovementInput` is issued on a breach frame, so `Acceleration` is 0 for the next `PerformMovement`, and `BrakingDecelerationFlying` is 0 ⇒ with `Velocity` zeroed, `PhysFlying`'s `SafeMoveUpdatedComponent` receives a zero delta and **does not sweep at all**, so the capsule sitting inside the slab is never depenetrated. **That is the property the whole window depends on, and it holds in either component-tick order.** |
| **5** | **Arrival snap ONLY on a REAL arrival** — a timed-out climb is never handed the deck | `SummonedUnit.cpp:3941-3944` — `if (bReachedTop) SetActorLocation(ArrivalTarget, bSweep=false);` and `bReachedTop`/`bTimedOut` are **mutually exclusive** by `Advance`'s structure (`:270-282`, arrival tested and returned **before** the watchdog) | ✅ **MET.** A `Timeout` exit reaches `EndLadderClimb` with **no** `SetActorLocation` call and drops from where it actually is. Test 7 pins the arrival-wins-on-the-expiry-frame ordering. *(Snap magnitude: see **W-3**.)* |
| **6** | Both endpoints lifted into capsule-centre space by `GetScaledCapsuleHalfHeight()` — **⛔ never a literal** — and the **SAME lift at both ends** | `SummonedUnit.cpp:3789-3791` reads `GetCapsuleComponent()->GetScaledCapsuleHalfHeight()` (fallback `SiegeSpawn::DefaultCapsuleHalfHeight`, ⛔ still not a literal in this file); `:176-181` applies **one** `Lift` vector to `Start` **and** `End` | ✅ **MET.** Length, direction and watchdog budget are provably unchanged (the lift is a common translation). ⭐ A BP child that resizes its capsule stays correct for free. |
| **7** | A stall **before** the window opens must fail **LOUDLY** | `SummonedUnit.cpp:3905-3914` — the `Timeout` exit is the **only** reason that logs, at `Warning`, naming "geometry blocking the climb line" and stating the unit was **dropped, not stranded** | ✅ **MET.** The watchdog is the second failure it catches, exactly as the clause requires. *(Its one blind spot is **W-1**.)* |
| **8** | Scoped to **this climb line and nothing else** | Grepped the whole diff: `SetActorLocation` appears in `SummonedUnit.cpp` at **exactly two sites**, `:3943` and `:3998`, both inside `TickLadderClimb`, both behind `if (!LadderClimb.bActive) return;` (`:3925`). `TeleportTo`: **0**. `ClimbableTower.cpp`: **0** of either | ✅ **MET.** ⛔ No general licence was taken; `NAV-§`'s refusal stands everywhere else. |

### ✅ AND THE SELF-CHECK `§8.5a` DEMANDS SURVIVES — the foot and the midpoint ARE swept

`Tests/SiegeLadderClimbTest.cpp:1052-1055` asserts `ShouldSweep(State, FootStandingCentre) == true` **and** `ShouldSweep(State, (Start+End)*0.5) == true`, with the comment naming the exact failure it excludes ("a driver that abandoned sweeping for the WHOLE traversal would pass every claim below"). `:1072-1073` additionally pins `DeckBreachUU < LengthUU × 0.5`.
⇒ **A whole-line non-swept traversal cannot pass this suite.** That is the one thing `§8.5` was written to stop, and it is instrumented.

---

## 2. ⭐ THE CAPSULE-CENTRE LIFT (TASK-738) — the second defect

- **Symmetric:** one `const FVector Lift(0,0,HalfHeight)` added to both `Start` and `End` (`SummonedUnit.cpp:177-181`). `LengthUU` is computed **after** the lift from the lifted pair (`:183`), so it is arithmetically identical to the un-lifted length; `ClimbDirection` and `TimeoutSeconds` follow from it. ✅ **Length, direction and watchdog budget unchanged.**
- **Read from the capsule, never a literal:** `GetScaledCapsuleHalfHeight()` at the call site (`:3789-3791`). ✅
- **Test 12(d) asserts the FEET, and excludes both named traps** (`SiegeLadderClimbTest.cpp:1077-1087`):
  - `FinalFeetZ = ArrivalTarget(State).Z − CapsuleHalfHeightUU` is `TestEqual`'d against `LadderTop.Z` at `Tolerance = 1e-3`. ✅
  - `⛔ NOT LadderTop.Z − 40.8` (the swept-stall position) — asserted **not** nearly-equal, band 5 uu. ✅
  - `⛔ NOT LadderTop.Z − 88` (the un-lifted line) — asserted **not** nearly-equal, band 5 uu. ✅
  - ⭐ The two exclusion bands (5 uu) are far narrower than the distances they exclude (40.8 and 88 uu), so neither row can be satisfied by accident, and the positive row is not tautological with them.
- ⭐ **QA confirms the programmer's framing:** the lift and the window are **both required and neither is optional**. A non-swept window without the lift teleports the unit *into* the slab; a lift without the window stalls it ~131 uu below the deck. The suite proves the composed outcome, not each half separately, which is the right instrument.

---

## 3. ⭐⭐ THE NINTH-EXIT CLASS — and the hunt for a tenth

### 3a. The eight exits, verified **at the call sites** (the tests prove the latch; this is the wiring read the handoff asked for)

| # | Exit | Verified at | Order-critical? | ✔ |
|---|---|---|---|---|
| 1 | Arrival | `SummonedUnit.cpp:3951` (via `TickLadderClimb`) | — | ✅ |
| 2 | `AbortLadderClimb()` | `:3871` | — | ✅ |
| 3 | New order (L-4) | `:2239`, inside `AssignCommandGroup`, **after** the id/offset write and **before** the target drop | — | ✅ *(see **W-2**)* |
| 4 | `HandleDeath` | `:4162`, immediately after `bDead = true` (`:4150`) and **BEFORE** the rigged path's `DisableMovement()` (`:4226`) | ⭐ **YES — correct.** Death's `MOVE_None` wins | ✅ |
| 5 | `FreezeAI` | `:848`, immediately after `bAIFrozen = true` (`:840`) | — | ✅ ⭐ **This is the invisible one and it is wired.** `FreezeAI` touches no movement mode anywhere else; without this line a climber would hang over the end screen |
| 6 | `ApplyFreeze` | `:1123`, after `bSpellFrozen = true` (`:1112`) and **BEFORE** its `DisableMovement()` (`:1147`) | ⭐ **YES — correct.** The freeze's `MOVE_None` wins | ✅ |
| 7 | `EndPlay` | `:789`, **before** `Super::EndPlay` (`:791`) | ⭐ **YES — correct.** The actor is still intact when the delegate fires | ✅ |
| 8 | Tower destroyed | `ClimbableTower.cpp:170` `Climber->AbortLadderClimb()` in `AClimbableTower::EndPlay`, **before** `Super::EndPlay` (`:177`) — plus `ASummonedUnit::EndPlay` as the independent belt | ⭐ **YES — correct** | ✅ |
| — | Timeout (declared ninth **reason**, a watchdog) | `:3951` | — | ✅ |

**One teardown, one latch:** `EndLadderClimb` (`:3874`) consumes `FSiegeLadderClimbStatics::End` **first** (`:3880`), before any effect, so a double exit (Death→EndPlay) and a listener that re-enters `AbortLadderClimb` from inside the broadcast are both inert. The restore (`:3894-3899`) is `StopMovementImmediately` → exact `MaxFlySpeed` restore → `SetDefaultMovementMode()`, and the broadcast is **last** (`:3920`). ✅ **`ClimbableTower::HandleLadderClimbEnded` therefore never observes a half-torn-down unit.**

### 3b. ✅ THE NINTH EXIT 734 FOUND AND CLOSED — **CONFIRMED CLOSED**

- `ClimbableTower.cpp:382` — `const bool bLadderOccupied = ActiveClimber.IsValid();` ⭐ **No identity comparison anywhere.** Grepped: `ActiveClimber.Get() != Unit` appears **nowhere** in the file.
- The re-entry path is therefore `LadderBusy` → `ResumeAgentPathFollowing` → `return`, and **the live climb is never touched**: `ReleaseClimber` is not called on that branch (`:389-406`).
- `ReleaseClimber`'s own guard (`:486`, `:500`) means it can only clear the ladder for a unit **this tower registered** (`bWasActive`) or when the handle has already gone stale — which is exactly the defensive occupant clear `TOWER-§8` (6) asks for.
- ⭐ **Verified at engine source, not on relay:** `PathFollowingComponent.cpp:1454-1478` — `StartUsingCustomLink` force-finishes the previous link and sets `CurrentCustomLinkOb` **before** calling `OnLinkMoveStarted`; `NavLinkCustomComponent.cpp:198-209` — `OnLinkMoveStarted` adds to `MovingAgents` **before** executing the delegate and returns `true` **merely because the delegate is bound**. Both of 734's load-bearing citations are exact. ⇒ **the refusal path IS load-bearing**, and every non-`Climb` verdict does resume (`:400`), as does the `BeginLadderClimb == false` path (`:447`).
- ⭐ **A bonus consequence QA verified and neither task claimed:** `BeginLadderClimb`'s `AI->StopMovement()` (`SummonedUnit.cpp:3825`) re-enters `AbortMove` → `OnPathFinished` → `FinishUsingCustomLink` **inside** `StartUsingCustomLink`. The engine handles this cleanly (`CurrentCustomLinkOb` was already assigned, is reset by the re-entrant finish, and the `bCustomMove == true` branch leaves it reset). **No stuck unit, no double-drive, and `HasMovingAgents()` is correctly false by the time the climb runs** — which independently vindicates 734's refusal to use it as the busy test.

### 3c. ⭐⭐ THE TENTH-EXIT HUNT — what I looked for, and what I found

**Method.** The catastrophic failure is not "an exit was missed" — the teardown is one latched branch. It is **"`bActive` is true and the driver cannot run"**, which is the `StopAttackLunge` class. So I enumerated every way `TickLadderClimb` can stop firing, and every way `MOVE_Flying` can survive.

| Candidate | Result |
|---|---|
| Any writer of the actor tick flag other than `RefreshActorTickEnabled` | ✅ **Grepped the whole `Source/` tree: `SetActorTickEnabled` appears at exactly ONE site, `SummonedUnit.cpp:3760`.** `StopAttackLunge` (`:3725`) and `UpdateLunge`'s stray-tick guard (`:3735`) both route through it. `PrimaryActorTick.bCanEverTick` is `true` (`:312`) and never re-written |
| A subclass that overrides `Tick` without `Super` | ✅ **None.** `AMinerUnit`/`ASorcererUnit` override `BeginPlay`/`EndPlay`/`FreezeAI` only; `AMinerUnit::EndPlay` and `::FreezeAI` both call `Super` (`MinerUnit.cpp:240`, `:249`) |
| Another `SetMovementMode`/`DisableMovement` on `ASummonedUnit` | ✅ Only `:1147` (`ApplyFreeze`), `:1208` (`EndSpellFreeze`), `:4226` (death hold) and `:3858`/`:3898` (the climb) — all after or inside a teardown |
| A "new order" lane that bypasses `AssignCommandGroup` | ⚠️ **None today** (only `SiegePlayerController.cpp:3281` and `:3622`) — but the exit is bound to a bookkeeping function, not to the order concept. **→ W-2** |
| A second steering authority not covered by the `UpdateState` fence | ⚠️ `AMinerUnit::UpdateMining` on `MiningPollTimerHandle` (declared watch item 9) — **→ W-5**. `PerformHeal` (`:2748-2774`) was checked and **issues no movement at all**, so a climbing Cleric is not a double-driver — **flagged decision 5 is mechanically safe** |
| Blueprint/`Destroy()`/`KillZ`/`PlayAgain`/level-teardown paths | ✅ All funnel to `EndPlay` (exit 7) or `HandleDeath` (exit 4) |

> ### ⭐⭐ **I DID NOT FIND A TENTH EXIT. I FOUND A TENTH-EXIT-CLASS HAZARD, AND IT IS THE DEEPEST FINDING IN THIS GATE: THE WATCHDOG RIDES THE VERY DRIVER IT IS WATCHING.**
> `TickLadderClimb` advances the clock **and** the clock is the only thing that can end a hung climb. ⇒ **if the actor tick flag is ever written `false` from outside this class — a `BP_Unit_*` child, a level Blueprint, a future C++ site — the climber hangs in `MOVE_Flying` FOREVER and the watchdog cannot fire, because it is on the dead driver.** That is precisely the `StopAttackLunge` bug the programmer correctly *fixed*; what remains is that the safety net would not have *caught* it.
> **Severity: WARN, ⛔ not a blocker** — the only in-tree writer is now correct and single. **See W-1 for a 1-line, zero-new-timer self-heal.**

---

## 4. TASK-734 — THE SMART LINK

| Requirement | Verified at | ✔ |
|---|---|---|
| **Smart, ⛔ never simple** | `UClimbableTowerLadderLink : public UNavLinkCustomComponent`, one CDO subobject (`ClimbableTower.cpp:131`). M-3 re-verified at source: `PathFollowingComponent.cpp:959-963` calls `StartUsingCustomLink` only on a non-`Invalid` `CustomNavLinkId` | ✅ |
| **`BothWays`, explicit** | Constructor `:96`, and again on `SetLinkData` `:350` | ✅ |
| **Sockets via `RTS_Actor`** | `:297` `Mesh->GetSocketTransform(SocketName, RTS_Actor).GetLocation()` | ✅ ⭐ **AND `RTS_Actor` IS LOAD-BEARING — CONFIRMED, ⛔ not taken on the handoff's word.** `ABuilding::ABuilding` does `SetRootComponent(VisualMesh)` (`Building.cpp:37-38`), so `VisualMesh` **is** the root ⇒ its `ComponentToWorld` **is** the actor transform ⇒ `RTS_Actor` returns the socket's authored mesh-local coordinate with the spawn squash's `SetRelativeScale3D` (±30 %, `SquashAmplitude 0.30`) **divided straight back out**. `RTS_World` would have baked the wobble into the stored link data permanently. **This is the single sharpest call in TASK-734.** |
| **Degrade open, one warning** | `:281-283` (no mesh / no static mesh / no owner / none name / socket absent) · `:320-325` (degenerate ⇒ **both** ends fall back) · `:331-342` **one** `Warning` naming exactly what was missing · ⛔ the link is never disabled | ✅ Matches `TOWER-§8.4(A)` exactly, including the "⛔ never a broken tower" half |
| **World points read back off the LINK** | `:412-413` `GetStartPoint()`/`GetEndPoint()` — ⛔ never re-derived from sockets. Verified at engine source: `NavLinkCustomComponent.cpp:518-526` transforms the owner-relative pair by the owner transform, so the traversal and the navmesh cannot disagree | ✅ |
| **Shell REMOVED** | `AscentGateVolume` · `AscentGateFloorUU` · `AscentGateHeadroomUU` · `AscentGateHalfExtentXY` · `ConfigureAscentGate()` · the `BoxComponent.h` include — **all absent** from both files. Test 7 asserts the four by name; test 4's dead self-check was replaced by `LadderLink` and the stronger claim promoted into test 7 | ✅ ⛔ The known-open pivot-vs-mesh tuning item was **dissolved, not fixed** — correct per `TOWER-§8.2` |
| **`CanTeamAscend` is now genuinely the LIVE rule** | Two real callers: `EvaluateLadderEntry` `:216` (the shipped entry gate, reached from `HandleLadderLinkReached:383`) and `ShouldLinkAllowPathfinding` `:269`. Precedence **identity → team → occupancy** is explicit and asserted (test 8(d)/(e)); 8(f) proves the gate *delegates* rather than owning a second copy of T-3 | ✅ ⭐ **The `MARK-§ M-4` / `TOWER-§8.5` "correct mechanism never driven" class is CLOSED for this function** |
| **No tick, no timer** | Grepped: **0** `SetTimer`, **0** `Tick` in `ClimbableTower.cpp`. Both edges pushed (`FOnMoveReachedLink`, `OnLadderClimbEnded`) | ✅ |
| **`HIGH-§3` fence intact** | `SummonedUnit.h` is included for **movement only**; no `HeightAdvantageMultiplier`, no elevation report, no `HIGH-§` header. And the reflected-member scan from the other side is genuinely closed: **no member `ASummonedUnit` declares contains `Tower`/`Platform`/`Occupan`/`Ascen`** (the only hits in the header are the words `AClimbableTower`, `ATower`, `bIsOnATower` **in comments**) | ✅ |
| **Cross-task token hazard** | `SiegeClimbableTowerTest.cpp:158-161` narrowed to `Tower`/`Platform`/`Occupan`/`Ascen`, with a self-check requiring `IsClimbing` to be **FOUND**. **Without this the suite would have gone red at TASK-742 on five correct, specified members.** Also verified: `LadderLink`, `PlatformHeightUU` and the `Ladder*DefaultRelative` naming trip **none** of `RefusedSubsystemTokens` (`Occupan`/`Garrison`/`Capacit`/`Full`/`Ranged`/`Fall`/`Land`) | ✅ Confirmed on both sides |
| **Subclassing a `MinimalAPI` engine class links** | ✅ **Verified in the generated header, ⛔ not assumed:** `NavLinkCustomComponent.generated.h:41-48` exports the constructor, the destructor **and** `DECLARE_VTABLE_PTR_HELPER_CTOR` as `NAVIGATIONSYSTEM_API` (⛔ not `NO_API`). `LinkRelativeStart`/`LinkRelativeEnd`/`LinkDirection` are `protected` (`NavLinkCustomComponent.h:181-189`), so the constructor's direct writes are legal. The `IsLinkPathfindingAllowed(const UObject*) const` override signature matches `:42` exactly | ✅ |

### ⚖️ RULING ON `TOWER-§8.6`'s OPTIONAL PATHFINDING LAYER: **SHIPS. THE MEASUREMENT IS ACCEPTED.**

The law permits the layer *only* on a measurement of `Querier`, and refuses to vouch for it. **I re-derived the measurement at engine source rather than accepting the handoff's citations:**

- `PImplRecastNavMesh.cpp:436-439` — `FRecastSpeciaLinkFilter::isLinkAllowed` → `CustomLink->IsLinkPathfindingAllowed(CachedOwnerOb)`. ✅ exact.
- `PImplRecastNavMesh.cpp:442-445` — `initialize()` sets `CachedOwnerOb = SearchOwner.Get()`. ✅ exact.
- ⭐ **And the engine states it in its own words:** `NavLinkCustomInterface.h:65-68` — *"Check if link allows path finding / **Querier is usually an AIController trying to find path**"*. That is independent corroboration the handoff did not cite.

Combined with `ShouldLinkAllowPathfinding`'s **fail-open by construction** (`ClimbableTower.cpp:249-265`: null/non-controller ⇒ `true`; no pawn or non-`ITeamAgent` ⇒ `true`), the worst case of a wrong reading is **exactly the behaviour of not shipping the layer** — ⛔ never an own-team unit that cannot path to its own tower. `ITeamAgent::GetTeamId()` is `const` (`TeamId.h:39`), so the `const ITeamAgent*` chain compiles.
⇒ ⭐ **KEEP IT.** It earns its place for the one case named in the handoff — an enemy whose *target* is a unit standing on the deck, a routine `MoveToActor` and exactly the `T-3` case. The recorded 4-line deletion path stays on file as a revert, ⛔ not as a pending action.

---

## 5. TASK-737 — THE MESH

| Claim | Evidence | Verdict |
|---|---|---|
| 8 hulls, `bIsGenerated == false` on all 8, 0 boxes/spheres/sphyls | Read back from a real UE import of the shipped FBX (`§3`), tabulated per hull with bounds | ✅ **ACCEPTED.** ⛔ No convex decomposition anywhere in the chain, and the `reimport_meshes.py` auto-decomposition trap was **simulated against the shipped function** and shown closed (`category=building` → `_apply_box_collision` → WARN + leave) |
| Deck deviation ≤ 0.000191 uu | 3,600 samples, zero ray misses, and the same max across the surviving nav poly X,Y ∈ [−236,236] | ✅ Float32 storage noise. The height-damage contract rests on a plane that is exact **by construction** (the hull's own +Z face) |
| Ladder **hull-free** so the foot cell is clear | 72 samples across a 34 uu radius at 5 heights, **zero hits**; nearest collision surface 116 uu; westmost collision vertex x = −300 | ✅ ⭐ **This is what makes `LadderFoot` reachable at all** — a hull at the foot would carve ground nav in the exact cell a unit must stand in. Independently corroborated by TASK-733b's own reader (`8 hulls, min x = −300.0`) |
| Both sockets on **y = 0** | `(−450, 0.0, 0)` and `(−150, 0.0002, 1200)` | ✅ ⭐ **And the reason matters:** the TASK-348 export pre-comp mirrors Y; both sockets sit where a Y negation is a **no-op**, so the handedness fix cannot corrupt them. That is design, not luck |
| Standoff re-measured (the `§8.5a` licensing condition) | **59.624 uu worst case over the whole line** | ✅ **The exception is not void** |
| ⚠️ Declared deviation: last 40.832 uu of line inside the deck slab | Capsule↔hull-07 clearance −53.95 uu; three escapes checked and each refused with a reason | ✅ **ACCEPTED AS DECLARED, AND CORRECTLY NOT PRESCRIBED.** The refusal to prescribe the fix was right — and TASK-738 then re-derived it independently and found it **worse** (the capsule's own 181.4 uu traverse), which is exactly why a non-relayed re-derivation was worth doing |
| ⚠️ Cosmetic residual: ladder head projects 108.7 uu above the deck, render-only, zero collision | Unit on `LadderTop` clears it by 37.1 uu | ✅ Accepted; cannot enter voxelization or cost a nav cell |

> ### ⚠️ **W-7 — `UStaticMesh::Sockets` IS NOT REFLECTED, SO MCP CANNOT READ SOCKET NAMES, AND A MISNAMED SOCKET IS SILENT.**
> TASK-737 proved the names at **file level** with an independent 13-row verifier (`verify_watchtower_fbx.py`) — that is the strongest instrument available headlessly and it is accepted. **But `TOWER-§8.4(A)` makes the code degrade OPEN**, so a socket named `Ladder_Foot`, `LadderFoot ` or `SOCKET_LadderFoot` (prefix not stripped) produces: a working tower · one `Warning` · the pinned literals · and **nothing observable in play**. The engine-side assertion is genuinely owed. **→ TASK-742 must-verify #1 (§8).**

---

## 6. TASK-733b — THE RE-EXPORT

| Claim | Verdict |
|---|---|
| −22 uu verified **independently** (own binary FBX reader, ⛔ not relayed from TASK-739) | ✅ **ACCEPTED, and the method is right.** The **Y-up + baked −90° X rotation** trap is named explicitly, the remap `UE (X,Y,Z) = (fbx X, −fbx Z, fbx Y)` is stated, and the parse is **validated against two figures it could not fudge** (render min x −436.418 and lateral ±66/±86, both TASK-737's declared numbers). ⭐ A parse validated against externally-declared figures is a real instrument, not a restatement |
| ⭐ The extra check nobody asked for: **is the rung plane constant along the ladder?** | ✅ **This is the check the whole fix depends on and it was not in the spec.** 12 u-buckets from u = 8 → 1199: mid-plane **−22.0**, thickness **20.0**, every bucket. ⇒ a **rigid** translation is provably the correct instrument. Bucketing by **proximity to the climb line** rather than `x < −300` (which would silently clip the ladder's upper half at u ≈ 618) is also correct |
| Rigid translation proven, ⛔ not asserted | ✅ **195 non-root curves at float32 quantisation** (max 9.155e-05 deg rotation, 4.470e-05 uu translation); root `Lcl Translation` delta **21.999999 uu along `n`, off-axis residual 0.000000000**, root track still **CONSTANT** both sides. ⇒ nothing re-solved, no cyclic translation introduced |
| The four original gates re-run | ✅ Seam identity 0.0 exact · contact drift ≤ 0.0000477 uu (four limbs) · IK residual 0.0000 · elbows 43.4–112.1° / knees 41.2–140.8°, bilaterally identical · corner ratio 0.4199. ⛔ The rejected velocity-based seam test was **not** reintroduced |
| 90.00 uu / 337.50 uu/s / band 0.4–1.4 unchanged | ✅ Confirmed by measurement, ⛔ not assumed — and correctly so: **the shift is perpendicular to travel**, so these are invariants the re-export had to *demonstrate* rather than inherit. `RateScale = 350 / 337.5 = 1.037037` stands |
| ⚠️ Declared deviation: `RUNG_PLANE_OFFSET_M` added instead of editing `BODY_STANDOFF_M` | ✅ **CORRECT, AND THE SPEC WAS WRONG.** `BODY_STANDOFF_M` **is the authored reach**; editing it would have re-solved every joint — the exact thing spec (2) forbade. Adding a separate constant and rewriting the reach budget to measure against the plane's new coordinate makes the 24 uu body→rung distance invariant **by construction rather than by luck**. Deviating from a spec that contradicted its own requirement is the right call, and declaring it is what makes it reviewable |

### ⚖️ RULING ON THE RUNG-SLAB OVERLAP (733b §6): **THE PROVISIONAL ACCEPTANCE IS CONFIRMED — ⛔ NOT OVERTURNED.**

**The trade as measured:** grips exactly on the pinned mid-plane (`hand_l`/`hand_r` m = −22.00 **constant** through the grip window; `foot_l`/`foot_r` m = −14.00 = mid-plane + the designed 8 uu ankle standoff) at the cost of `neck_01` **2.29 uu** and `pelvis` **1.92 uu** inside the rung's near face. Torso proper clear.

**Confirmed, on three independent grounds:**
1. **Magnitude.** ~2 uu on a ~180 uu character is **1.1 %** of character height, on a **top-down RTS camera**, against thin horizontal bars that are only at neck height for part of the cycle. At or below the visual threshold, as the manager judged.
2. **What it buys.** It removes a **12 uu visible grip miss** — hands closing on air — which is *the* "reads as broken" failure `A_SiegeBiped_Climb` exists to prevent, and it is at the exact centre of visual attention. A 6× magnitude improvement, moved from the salient feature to a non-salient one.
3. ⭐ **The law.** `TOWER-§8.3` pins the rung plane at the **mid-plane −22.0** and says in terms: *"The clip is re-exported to THIS number; ⛔ the law is ⛔ NOT bent to absorb the clip."* Gripping the near face (−12) grips a plane the law does not pin. **The delivered clip is the compliant one.**

> ### ⚠️ **AND A CORRECTION TO THE RECORD THAT MATTERS IF THE CONTINGENCY IS EVER TAKEN — this is QA's own arithmetic, ⛔ not in any handoff:**
> **(a)** The recorded `0.22 → 0.12` fix (one constant, no re-solve) is real and correctly costed **for the neck** (buys 7.71 uu of clearance). **⛔ But it does NOT fix the second residual:** `calf_l` at full flexion reaches m = **−54.6**; at 0.12 it would reach **−44.6**, still **12.6 uu past the far face**. ⛔ **Do not sell 0.12 as fixing both.** The shin needs a knee-pole re-aim, which **is** a re-solve.
> **(b)** ⭐ **If 0.12 is ever taken, it must land as a `TOWER-§8.3` AMENDMENT, ⛔ not as a constant edit** — the rung-plane row would then have to say the clip grips the **near face** and why. Otherwise the mesh↔clip binding box does exactly what it is designed to do on the next `SM_WatchTower` re-author: re-derive −22 and **silently revert the fix**. A fix that contradicts a pinned number and does not amend it is a fix with a scheduled expiry date.

**The shin residual itself (`calf_l` m = −54.6): ACCEPTED, with one honest re-characterisation → W-6.** It is correctly stated as pre-existing *in body-relative terms* and it does meet **open air** (§1.1: nothing at all in m ∈ [−60, −32) within the ±95 uu lateral band — measured, and a check worth having). ⚠️ But its relationship to the **real** slab changed materially: pre-shift it grazed the far face at −32.6; post-shift it passes **through** the 20 uu slab and 22.6 uu out the back. "Unchanged in body-relative terms" understates that. Not a defect, occluded from the RTS camera, zero gameplay effect — but it belongs on Jonathan's PIE eyeball list beside the 2 uu neck, and it is not on it.

---

## 7. FINDINGS

### BLOCKERS — **none.**

### WARNS

- **[WARN] W-1** — `SummonedUnit.cpp:3756-3761` + `:3923-3934` — ⭐⭐ **The watchdog rides the very driver it watches.** `TickLadderClimb` both advances the timeout clock and is the only thing that can end a hung climb. Any external write of the actor tick flag (a `BP_Unit_*` child, a level BP, a future C++ site) hangs the climber in `MOVE_Flying` permanently **and the watchdog cannot fire**. `RefreshActorTickEnabled` being the sole in-tree writer is the right fix for the *cause*; this is the gap in the *net*.
  **Suggested fix — 1 line, zero new timers, uses a driver that survives a tick-flag kill:** `StateTimerHandle`'s 0.25 s poll keeps running for the whole climb (`BeginLadderClimb` clears `AttackTimerHandle` only), and `UpdateState`'s fence is at `:1567`. Insert **above** that fence:
  `if (LadderClimb.bActive) { RefreshActorTickEnabled(); }`
  ⇒ the driver becomes self-healing against any external write, at a cost of one bool test per poll per unit.

- **[WARN] W-2** — `SummonedUnit.cpp:2222-2239` — **Exit 3 is bound to a bookkeeping function, not to the "new order" concept.** Verified that today this is complete: `AssignCommandGroup` has exactly two callers (`SiegePlayerController.cpp:3281` group confirm, `:3622` follow), and the deliberate *exclusion* of `ClearCommandGroup` is correct and correctly reasoned (a release is not an order, and it is also reached by `UpdateState`'s null-group self-heal). ⚠️ **But the parallel `MARK-§` / war-room wave is adding a new order grammar** ("move all units to hold 1"). Any new lane that steers a unit **without** routing through `AssignCommandGroup` will not abort the climb: the `UpdateState` fence makes the unit ignore the order, it keeps climbing to the watchdog, and **the order is silently dropped** — `TOWER-§8`'s "correct mechanism never driven" class again. ⛔ Not this batch's defect. **→ carry into the second wave's gate (§8, item 10).**

- **[WARN] W-3** — `SummonedUnit.cpp:3930/3955/3972/3943` — **The traversal runs on a line PARALLEL to the pinned one, not on it.** `ClimbDirection` is fixed from `Start→End`, but every step is applied from `GetActorLocation()`, and the unit's actual position when `HandleLadderLinkReached` fires is wherever path following accepted the link start (order `AgentRadius` ≈ 34 uu away, any direction). Nothing ever converges the capsule onto the line.
  **Ruled NOT a blocker, with the arithmetic:** the standoff has the margin to absorb it — 59.624 − 34 = **≥ 25.6 uu still clear of the body face**, and the ladder is hull-free — so the swept 78 % cannot jam on the tower. `ShouldSweep`'s Euclidean distance opens the window ~2 uu late at a 34 uu offset (negligible), and arrival detection is unaffected (it is an along-axis dot product). ⚠️ **The consequence is that the arrival snap (`:3943`) is a non-swept lateral move of unbounded magnitude** — `§8.5a` clause 5 licenses the snap on a real arrival but does not bound it. On the shipped deck (600×600, `LadderTop` 150 uu from the edge) a ~34 uu lateral snap lands safely, and other bodies are resolved by the same depenetration `TOWER-§4` already relies on.
  **Suggested (optional) hardening:** steer toward the closest point on the segment rather than along a constant direction — 2 lines in `ClimbDirection`'s caller — which would also make the snap a no-op.

- **[WARN] W-4** — `SummonedUnit.cpp:199-209` — **`DeckBreachUU` clamps to `LengthUU`, so a shallow line would be driven entirely non-swept.** `min(BreachZ × Length/Rise, Length)`: at rise = 1 uu and length = 1000 uu the clamp yields the **whole line**. `§8.5a` clause 2's ceiling is expressed **in Z** and is honoured (264 uu always), so this is not a clause-2 violation — and it is **unreachable from the shipped caller** (the link's rise is always 1,200 uu, and `MinimumLadderLineUU`'s fallback guarantees the pinned pair). ⚠️ But `BeginLadderClimb` is a public `BlueprintCallable`, and the "78 % swept" property lives only in test 12(c) **for the pinned line**, not as a code invariant.
  **Suggested fix, 1 line:** `State.DeckBreachUU = FMath::Min(BreachZUU * (LengthUU / RiseUU), LengthUU * 0.5f);` — makes "it is a window, not the whole line" true by construction for **any** line.

- **[WARN] W-5** — declared watch item 9, **upheld as declared.** `AMinerUnit::UpdateMining` → `EnsureWalkingToNode` runs on `MiningPollTimerHandle`, which `UpdateState`'s fence does not cover, and `CanTeamAscend` is **frozen** with no unit-type term ⇒ a miner *could* be admitted. **Reachability is very low** (no gold node exists on a deck, so no path terminates there and no link entry occurs) and the failure mode is a double-drive that the watchdog **drops** rather than hangs. `MinerUnit.{h,cpp}` is outside TASK-738's fence; **raising, not fixing, was correct.** ⛔ Do not add a unit-type term to `CanTeamAscend` — its signature is frozen by `TOWER-§8.6`.

- **[WARN] W-6** — TASK-733b §6 second residual — `calf_l` reaches m = **−54.6 uu**, i.e. **22.6 uu out the BACK of the 20 uu rung slab** at full knee flexion. Correctly measured, correctly shown to meet open air, and correctly attributed as pre-existing in body-relative terms — ⚠️ **but its relationship to the physical slab is materially new** (pre-shift it grazed the far face at −32.6). **Add it to Jonathan's PIE eyeball list beside the 2 uu neck**, and note it is **not** fixed by the recorded `0.12` contingency (see §6).

- **[WARN] W-7** — `SM_WatchTower` socket NAMES are unverified in-engine (`UStaticMesh::Sockets` unreflected ⇒ MCP-blind). **A misnamed socket is SILENT** because `TOWER-§8.4(A)` degrades open. File-level proof accepted; engine-level proof owed. **→ TASK-742 must-verify #1.**

- **[WARN] W-8** — `Content/Blueprints/Buildings/BP_Building_WatchTower.uasset` still carries `AscentGateVolume` in its FName table (offset 808, the inherited-component record). Once this code lands, that inherited component no longer exists on the native class. Expected: UE drops the orphaned node with a `LogBlueprint` warning and marks the BP dirty; it should **not** break the Blueprint. **→ TASK-742 must-verify #5 (load + resave + read the log).**

### NITS

- **[NIT] N-1** — `SummonedUnit.cpp:240` — `ShouldSweep` uses `FVector::Dist(CurrentWorld, DeckEnd)` (Euclidean) rather than the along-axis projection. With a 34 uu lateral offset the window opens ~2 uu late on a 272 uu window. Harmless; noted only because the same expression would matter on a shorter ladder.
- **[NIT] N-2** — `SummonedUnit.cpp:3996-3998` — the breach step is not clamped to the remaining distance, so the capsule can overshoot `End` by up to one frame's travel (≈11.7 uu at 30 fps) before the next frame's `bPassedTheTop` snaps it back. Non-swept, so no collision; sub-frame and invisible.
- **[NIT] N-3** — `ClimbableTower.cpp:237-270` — the threading note is more conservative than the shipped behaviour requires (this project's moves all go through `AAIController::MoveTo*`, i.e. synchronous pathfinding). The read-only, allocation-free construction is correct regardless. **No action.**
- **[NIT] N-4** — `SummonedUnit.cpp:148-151` — `CanBegin`'s `ContainsNaN()` refusal is deliberately not exercised (constructing a NaN `FVector` under `ENABLE_NAN_DIAGNOSTIC == 1` would redden the test's own fixture). **Covered by this diff read instead, as the handoff requested: the guard is present, is placed before every write, and returns `false` without touching the state.** ✅
- **[NIT] N-5** — line-number drift in `TASK-738-programmer.md` §2/§4 (cites `.cpp:1992/2760/3047/3874`; actual `2817/2049/3104/3934`). Every named site was located and verified; cosmetic only.
- **[NIT] N-6** — two stale figures in `TASK-734-programmer.md`: §7 quotes suite total **187** (pre-dates 738's +2 tests) and §5 quotes `State.Foot`/`State.Top` from a pre-amendment 738 (now `Start`/`End`). Both harmless; the shipped code is correct and the reasoning in §5 was **vindicated** — it became the `TOWER-§8.4(B)` amendment.

---

## 8. ⭐ SUITE ARITHMETIC — **COMPUTED HERE, ⛔ NOT TAKEN FROM ANY HANDOFF**

Counted repo-wide over `IMPLEMENT_*_AUTOMATION_TEST` in `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp` at the moment of this gate:

**LADDER BATCH = 171 (baseline) + 13 (TASK-738, new `SiegeLadderClimbTest.cpp`) + 5 (TASK-734, 5 → 10 in `SiegeClimbableTowerTest.cpp`) = ⭐ 189.** ✅ Matches the board.

⚠️⚠️ **THE ON-DISK TOTAL IS 247 ACROSS 20 FILES, AND THAT IS ⛔ NOT A DISCREPANCY.** The parallel MARKS + RECALL + GHOST + assistant wave has landed and grown files alongside this batch (TASK-738 measured 206 mid-task; it has since moved again). **The ladder batch's contribution is a measured +18 and that is the only number this gate owns.**

Per-file, so TASK-742 can reconcile without guessing:

| File | Tests | | File | Tests |
|---|---|---|---|---|
| SiegeAccountTest | 7 | | SiegeGhostPawnTest | 10 |
| SiegeAssistantGrammarTest | 12 | | SiegeHighGroundTest | 9 |
| SiegeAssistantGuardTest | 9 | | SiegeKeyboardLayoutTest | 7 |
| SiegeAssistantSelectionTest | 38 | | ⭐ **SiegeLadderClimbTest** | ⭐ **13** |
| SiegeAssistantZoneATest | 5 | | SiegeMapMarkTest | 9 |
| SiegeCastleTransformTest | 1 | | SiegeRecallTest | 12 |
| ⭐ **SiegeClimbableTowerTest** | ⭐ **10** | | SiegeRespawnLifecycleTest | 9 |
| SiegeCloudTest | 9 | | SiegeSettingsTest | 7 |
| SiegeControlsHelpTest | 13 | | SiegeStuckStaticsTest | 20 |
| SiegeDeckSlotsTest | 12 | | SiegeWarMapTest | 35 |
| | | | **TOTAL** | **247** |

🔧 **THE RULE FOR TASK-742, ⛔ not a fixed number:** assert the total **measured on disk at compile time**. If it differs from 247, diff **per file** and confirm every unit of the delta lies **outside** the two ladder files. ⛔ **`SiegeLadderClimbTest.cpp ≠ 13` or `SiegeClimbableTowerTest.cpp ≠ 10` is a STOP.** A total above 247 is expected and is the parallel wave's, ⛔ not a ladder regression.

---

## 9. NOTES FOR BUILD-MASTER — ⭐ WHAT TASK-742'S COMPILE AND EDITOR STEP MUST VERIFY

**Compile**
1. ⚠️⚠️ **Parse the log for `Result: Failed` — ⛔ NEVER trust `$LASTEXITCODE`.** Build.bat returns 0 on a failed build (the Live Coding mutex). *(And if it fails in ~2 s with `0x800711C7`, that is Smart App Control, ⛔ not a code error — do not loop QA.)*
2. **TASK-734 does not link without TASK-738's API — that is the designed state.** A link error naming `BeginLadderClimb`/`AbortLadderClimb`/`IsClimbing`/`OnLadderClimbEnded` means 738 did not land, ⛔ not that 734 is wrong. **Both are on disk now, so this should not fire.**
3. **Watch for the three `static_assert`s in `SiegeLadderClimbTest.cpp:210-218`.** If any fires, the pinned `TOWER-§8.4(B)` signature drifted — the message names the law. This is the designed early-warning and it fires **in that module**, ⛔ not as a link error inside 734.
4. **Pre-cleared by this gate, so treat a failure here as new information:** `UClimbableTowerLadderLink` subclassing the `MinimalAPI` `UNavLinkCustomComponent` links (constructor/destructor/vtable helper all `NAVIGATIONSYSTEM_API`, verified in the generated header); `LinkRelativeStart/End/LinkDirection` are `protected`; `AIModule` + `NavigationSystem` were already `PublicDependencyModuleNames` (⛔ no `Build.cs` change); every include the new code needs is present in both .cpp files.

**Suite**
5. **Run it and reconcile per §8.** ⛔ A red suite is a STOP, ⛔ not a note. ⭐ **Do not read a total above 189 as broken** — reconcile against the per-file table first.

**Editor step**
6. ⭐⭐ **THE SOCKET-NAME CHECK — the highest-value editor assertion in this batch (W-7).** After the same-path reimport of `Content/RawAssets/WatchTower.fbx` over `/Game/Meshes/SM_WatchTower`: read back `len(sm.sockets) == 2` and `[s.socket_name for s in sm.sockets] == ['LadderFoot', 'LadderTop']` at **(−450,0,0)** and **(−150,0,1200)**. ⚠️ **A missing or misnamed socket is SILENT** — the tower still works on the `TOWER-§8.3` fallback and logs one warning nobody will see. This assertion cannot be made from Python-free MCP property reads; use the socket API directly. **Also read back: 8 hulls · `bIsGenerated == false` ×8 · box_count 0 · deck collision top z = 1200. ⛔ Zero hulls is a STOP** (`reimport_meshes.py`'s decomposition trap).
7. **Textures must be reimported with the mesh** (baked UV atlas, new layout): `T_WatchTower_{D,N,ORM}` — **`_D` sRGB ON · `_N` normal-map · `_ORM` LINEAR / sRGB OFF**, and ⚠️ **read the sRGB flag back** ("the ORM imported sRGB" is a recorded silent defect in this lane). Slots stay `[TeamRegion → MI_TeamColor_Blue, WatchTowerPBR → MI_WatchTower_PBR]`; **Nanite OFF**. ⛔ Same path, ⛔ never delete-and-recreate.
8. ⭐ **`BP_Building_WatchTower` needs a LOAD + RESAVE (W-8)** — it still carries the removed inherited component `AscentGateVolume`. Expected: a `LogBlueprint` dropped-node warning + dirty. **Read the load log and STOP on anything worse than a dropped-node warning.** ⛔ The new `LadderLink` needs **no** BP work (native CDO subobject).
9. **After the mesh lands, confirm the degrade-open warning is GONE.** `AClimbableTower::ConfigureLadderLink` logs exactly one `Warning` naming the missing socket when it falls back. ⭐ **Its presence after a successful reimport is the loudest available signal that the socket names are wrong** — it is the cheap cross-check on item 6.
10. **Carry W-2 to the second wave's gate:** any new order lane must route through `ASummonedUnit::AssignCommandGroup` or add its own `EndLadderClimb(..., NewOrder)` call site, or `TOWER-§10` L-4 quietly stops holding.

**Owed to Jonathan's PIE eyeball (⛔ none of these is a gate — they are for his eye, and TASK-739's ABP work must land first)**
11. The **~2 uu neck/pelvis overlap** with the rung's near face (§6). If judged visible, the fix is `RUNG_PLANE_OFFSET_M 0.22 → 0.12` — ⭐ **and it must land as a `TOWER-§8.3` amendment, or the next re-author silently reverts it.**
12. The **shin passing through the rung slab** at full knee flexion (W-6). ⛔ **Not** fixed by the `0.12` contingency.
13. The **ladder head's 108.7 uu render-only overhang** above the deck — a unit walking directly under it will clip it (TASK-737 kept it deliberately as the top-down "the ladder arrives here" cue).

**Law bookkeeping — ⭐ already discharged, recorded so nobody re-opens it**
14. TASK-738's requested amendment **11** (the `§8.5` deck-breach exception) landed as **`TOWER-§8.5a`**; requested amendment **13** (the parameter rename) landed as the **`§8.4(B)` amendment**. **The shipped code matches both amended texts character-for-character.** ⛔ No further law action is owed by this batch.

---

## 10. DECLARED DEVIATIONS — EVERY ONE RULED

**TASK-738 (13):** (1) new test file — ✅ **correct**, no `ASummonedUnit` test file existed and 734's is fenced. (2) `Timeout` watchdog as a ninth *reason* — ✅ **KEEP.** It routes through the same teardown so it adds no restore path, `TimeoutScale = 4` (14.1 s vs a 3.53 s ascent) cannot end a healthy climb, and `§8.5a` clause 7 **requires** exactly this. ⛔ Do not remove it. (3) `static constexpr` tuning constants, ⛔ not UPROPERTYs — ✅ zero designer surface; `LadderClimbSpeedUU` remains the **only** new UPROPERTY, as `§8.5` requires. (4) the degenerate/NaN math guard as a fifth `CanBegin` reason — ✅ **correct as a math guard**, and refusing to add `!bStatsLoaded` (a fifth *policy* reason) was the right restraint against a contract 734 links to. (5) **a climbing Cleric still heals** — ✅ **UPHELD.** Jonathan ruled *"can't **attack** back"*; the shipped `UpdateStateFollow` precedent is explicit; and QA verified `PerformHeal` (`:2748-2774`) issues **no movement**, so it is not even a double-driver. ⛔ Extending the ruling would violate `TOWER-§9` in the other direction. (6) `bForce = true` on `AddMovementInput` — ✅ **correct and necessary**: `IsMoveInputIgnored()` is true for a pawn with no controller, and `AutoPossessAI` possession can land after `BeginPlay`; a traversal the tower is waiting on must not be silently suppressible. (7) `CurrentTarget` preserved — ✅ direct consequence of `TOWER-§9.2`'s "the instant the deck is reached"; dropping it would be a lingering penalty by another name. (8) guard 2 unreachable during a climb — ✅ accepted, correctly commented so it does not read as dead code. (9) `AMinerUnit` watch item — ✅ **raised, not silently fixed** = correct → **W-5**. (10) post-arrival `UpdateState` re-decide — ✅ shipped behaviour of every unit, new only on this surface. (11) the `§8.5` exception — ✅ **already granted as `§8.5a`**; verified clause by clause in §1. (12) the capsule-centre lift — ✅ a **defect fix**, correctly called out separately from the sweep change. (13) the parameter rename — ✅ **already landed as law**; `static_assert`s and 734's positional call site are provably unaffected (parameter names are not part of a function's type).

**TASK-734 (8):** (1) the pathfinding layer shipped — ✅ **RULED IN**, measurement re-verified at source (§4). (2) a new `UCLASS` in the SOLE-owned header — ✅ fine, ⛔ not replicated, ⛔ no tier change. (3) `EndPlay`-aborts-a-climb is not headlessly testable — ✅ **honest and correct**; fabricating world-less possessed actors would crash the suite rather than catch the bug, and 738 ships an independent belt. Recorded as an **observation debt** for Jonathan's playtest, ⛔ not a missing test. (4) `TOWER-§4a`'s landing residual — ✅ unchanged by design, ⛔ no nav-projecting teleport added. (5) the 0.15 s spawn-squash world-bounds wobble — ✅ accepted; the **stored** data is squash-proof (`RTS_Actor`) and a climb cannot start in that window. ⛔ Adding a timer to dodge it would cost the no-tick property. (6) `MinimumLadderLineUU = 68` distinct from 738's `MinClimbLineUU = 1` — ✅ **correct**: different resources, different questions (link geometry vs climb math). (7) the exposure restatement belongs to 738 — ✅ delivered there. (8) airlock/M8 — ✅ clean.

**TASK-737 (1):** the 40.832 uu deck-slab intrusion — ✅ **accepted, and the refusal to prescribe the fix was correct.**
**TASK-733b (2):** `RUNG_PLANE_OFFSET_M` instead of `BODY_STANDOFF_M` — ✅ **correct, and the spec was wrong** (§6). The rung-slab overlap — ✅ **acceptance confirmed** (§6), with the amendment condition and the `0.12`-doesn't-fix-the-shin correction attached.

**Scope fence (`TOWER-§8.0`):** ⛔ `HIGH-§`, the ×3, `Cost = 30`, `HP = 250`, `0/0/0`, `bRanged = false`, `DeckCount = 0`, CardID `WatchTower`, `PlatformHeightUU = 1200`, the 600×600 deck, `AClimbableTower : ABuilding`, `TOWER-§4a`, `TOWER-§4b` — **all verified untouched across the whole batch. No re-litigation found.**

**M8 / airlock:** ⛔ no replicated property, no RPC, no relevancy tier, no class-tier change. `FSiegeLadderClimbState`/`ESiegeLadderExit`/`ELadderEntryVerdict` are **deliberately unreflected** ⇒ they cannot be replicated by accident, which is what makes the declaration survive a refactor. `GateTeam` is not a UPROPERTY. ⛔ No `Capture()`, no `EnsureSnapshot()`, Zone A untouched, the 552 latch untouched, no token figure. ✅ **Clean on both tasks.**

---

*Gate closed 2026-09-01. `SC-§27` diff-scoped to TASK-733b/734/737/738. `qa/TASK-730.md` not re-opened.*

---

# ✅ COMPILE RESULT FOR THIS LANE — APPENDED BY BUILD-MASTER (TASK-742 PHASE 1), 2026-09-01

⭐⭐ **THE LADDER WAVE COMPILED 100 % CLEAN — ZERO errors, ZERO warnings across all four of its translation units.** Recorded here so this lane is ⛔ **NOT** routed back when the shared module's build is failed by the parallel batch.

| TU | step | diagnostics |
|---|---|---|
| `ClimbableTower.cpp` | `[2/32]` | ✅ none |
| `SiegeClimbableTowerTest.cpp` | `[19/32]` | ✅ none |
| `SiegeLadderClimbTest.cpp` | `[21/32]` | ✅ none |
| `SummonedUnit.cpp` | `[29/32]` | ✅ none |

**§9 item 4 DISCHARGED:** `UClimbableTowerLadderLink` subclassing the `MinimalAPI` `UNavLinkCustomComponent` **cleared UHT and compiled** — all eleven `Module.GitClaudeUnrealTest.*.gen.cpp` TUs built clean. The gate's pre-clearing was correct; ⛔ no `Build.cs` change was needed.

## ⛔ BUT THE MODULE DID NOT BUILD — the failure is the PARALLEL batch's

`Result: Failed (OtherCompilationError)` (parsed from the log; ⚠️ **the process exit code was `0` — the lie fired again**). Three diagnostics, **all in `qa/TASK-753.md`'s lane**, none in a ladder file:

- `SiegeGhostPawn.cpp(192,46)` + `SiegeGhostPawnTest.cpp(580,44)` — `error C2248`: `AHeroCharacter::GetEffectiveWalkSpeed` is **protected** (TASK-749)
- `SiegeGameMode.cpp(483,15)` — `error C4458`: local `Owner` shadows `AActor::Owner` (TASK-750)

⛔ Not Smart App Control (`0x800711C7` ×0, 61 s run). ⛔ Not a Live Coding lock (the machine was verified quiesced first; the sole `mutex` hit is the `-waitmutex` argument echo). **Full detail is appended to `qa/TASK-753.md`; ⛔ build-master authored no fix.**

## §8 SUITE ARITHMETIC — THE RULE THIS GATE WROTE WAS FOLLOWED, AND IT HELD

This gate's instruction was *"assert the total measured on disk at compile time; if it differs from 247, diff per file and confirm every unit of the delta lies OUTSIDE the two ladder files."*

**Measured on disk = 248 across 20 files** (line-anchored `IMPLEMENT_*_AUTOMATION_TEST`; all `SIMPLE`). The delta of **+1** against this gate's 247 is **entirely** `SiegeLadderClimbTest.cpp` **13 → 14**.

⚠️⚠️ **THAT UNIT LIES *INSIDE* A LADDER FILE, AND IT IS ⛔ STILL NOT A STOP.** This gate pinned `SiegeLadderClimbTest.cpp == 13` as a STOP condition; it now reads **14**. That is **TASK-760's declared +1 self-heal, which landed after this gate closed** (`handoffs/TASK-760-programmer.md`, reconciled in `qa/TASK-753.md` §10 rider 1). ⭐ **The pinned figure was correct when pinned and is simply superseded — ⛔ not a ladder regression.** ✅ `SiegeClimbableTowerTest.cpp` reads **10**, exactly as pinned. **Every other per-file count matches this gate's table character-for-character.**

⚠️ **The suite was ⛔ NOT RUN** — the headless `Automation RunTests` lane needs a binary and there is none. `248` is a count of **declarations**, ⛔ not a green suite.

*Appended 2026-09-01 by build-master. ⛔ No verdict of this gate was altered. ⛔ No Git command issued, ⛔ no file staged, ⛔ no editor launched, ⛔ no `Source/` file edited.*
