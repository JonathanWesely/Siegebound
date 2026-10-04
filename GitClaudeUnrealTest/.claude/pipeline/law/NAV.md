<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## Unit pathing — the stuck-unit watchdog + nav-rebuild concurrency (2026-08-04) — namespace **`NAV-§N`**

Added 2026-08-04 on Jonathan's directive, verbatim: *"ok, we need to fix the pathing of the Units. There are many times when a unit recieves a command, and on the way to it, it gets stuck behind a rock. Lets come up with a way to fix that without it being too costly to compute. It may have to be some sort of path calculator but I'm not sure."*

Design authority = the approved plan `C:\Users\wesel\.claude\plans\ok-there-are-a-cheerful-mccarthy.md` (plan mode, **every engine claim verified first-hand against installed UE 5.8 source at `C:\Program Files\Epic Games\UE_5.8\Engine\Source` and against the session logs in `Saved/Logs/`**, ExitPlanMode-approved). ⛔ **The plan file wins over any board summary. The architecture is NOT re-litigated** — this section is the naming/behaviour law derived from it plus the batch's link contract.

📌 **THIS SECTION IS BORN WITH ITS NAMESPACE PREFIX (`NAV-§N`), at authoring, per the `KBD-§N` precedent.** ⚖️ Cite as `NAV-§3`, never as a bare `§3` — this file already carries three sections whose bare `§12b`/`§14` collided (`qa/TASK-502.md`), and the cheapest time to prevent that is now.

⚠️ **NOTE THE PLAN-FILE COLLISION AND DO NOT MIS-CITE IT: `ok-there-are-a-cheerful-mccarthy.md` IS THE SAME FILENAME `KBD-§N` CITES.** The plan-file slot was **OVERWRITTEN** — `KBD`'s keyboard-layout plan is **gone**. ⚠️⚠️ **UPDATED 2026-08-05 — THE SLOT HAS NOW BEEN OVERWRITTEN AGAIN AND *THIS* SECTION'S PLAN IS GONE TOO. The file on disk today is the *AI-COMMANDER ROBUSTNESS* plan (`AS-§21`).** ⇒ ⛔ **A reader following EITHER `KBD-§`'s OR THIS section's citation lands on a THIRD feature's plan.** ✅ **`NAV-§0..§14` is complete on its own and there is nothing to re-read.** `KBD`'s law is complete in `KBD-§0..§11` and needs no plan re-read; **this is recorded so nobody "reconciles" the two by editing the wrong section.**
- ✅ **MIRRORED ONTO THE `KBD-§` HEADER ITSELF 2026-08-04.** ⚖️ **The warning had been recorded HERE ONLY — on the side that already knows.** The reader who gets hurt starts at `KBD-§`'s design-authority line, opens the path, finds a pathing plan, and has **no reason to scroll 230 lines into a different section to learn why.** ⇒ **The warning now sits at BOTH headers, and the `KBD-§` copy carries the explicit *"do not reconcile by editing `NAV-§`"* refusal.**

### NAV-§0. ⚖️ JONATHAN'S THREE RULINGS — DECIDED VIA `AskUserQuestion`, ⛔ BINDING, ⛔ NOT RE-OPENABLE BY ANY AGENT

1. ⛔ **HE HAS *NOT* TRACKED WHETHER THE WEDGING IS EARLY-MATCH OR THROUGHOUT.** ⇒ **The telemetry must cover BOTH hypotheses and no spec may be written as if either were established.** ⚠️ A task, QA finding or handoff that asserts *"the wedging is early-match"* (or *"it happens all match"*) as a premise is **wrong at the evidence** and is corrected by this clause. **Cause 1 predicts early-match; Cause 2 predicts throughout; both fixes ship, and the log decides which one was doing the work.**
2. ⛔ **TELEMETRY AND THE SAFE FIXES SHIP TOGETHER — ONE BATCH, ONE PLAYTEST.** He was offered the strict diagnose-first round trip (instrument, play, report, *then* fix) and **explicitly declined it** because it costs him an extra play session. ⇒ **`NAV-§9` is how DIAGNOSE-FIRST is discharged instead, and it is discharged, not waived.**
3. **NAV REBUILD CONCURRENCY = 8 CONCURRENT TILES.** He was offered 4 / 8 / leave-alone **with the game-thread cost stated** and chose **8**. ⇒ `MaxSimultaneousTileGenerationJobsCount=8`, ⛔ **an explicit number, never the uncapped `NumWorkerThreads * 2`** (~32 on this machine). **The 8 → 4 fallback is his lever at the playtest, not an agent's default.**

### NAV-§1. ⭐ THE DIAGNOSIS — THREE CAUSES, EACH CITED. ⛔ THIS IS NOT A MISSING PATH CALCULATOR

> ### ⭐ **MOVEMENT IS *ALREADY* REAL PATHFINDING.** Every destination goes through `AAIController::MoveToActor` / `MoveToLocation` with `bUsePathfinding=true`, at **four sites only** (`SummonedUnit.cpp:2446`, `:2473`, `:2479`, `:2585`, plus `MinerUnit.cpp:867` / `:932`). ⛔ **A task that proposes "add a path calculator" has misread the codebase, and Jonathan's own *"it may have to be some sort of path calculator but I'm not sure"* is answered by this line, not obeyed literally.**

**⭐ CAUSE 1 — THE NAVMESH REBUILDS *ONE TILE AT A TIME*, ~216 s PER MATCH.**
`Config/DefaultEngine.ini:294` sets `bDoFullyAsyncNavDataGathering=True` (TASK-217, to keep carve/heal off the game thread). **The engine's price for that is a hard serialization** — `Runtime/NavigationSystem/Private/NavMesh/RecastNavMeshGenerator.cpp:5892-5896`, read directly:
```cpp
// this is a temp solution to enforce only one worker thread if GatherGeometryOnGameThread == false
// due to missing safety features
const bool bDoAsyncDataGathering = GatherGeometryOnGameThread() == false;
const int32 NumTasksToSubmit = (bDoAsyncDataGathering ? 1 : MaxTileGeneratorTasks) - NumRunningTasks;
```
With it `False`, `MaxTileGeneratorTasks = min(max(NumWorkerThreads*2, 1), MaxSimultaneousTileGenerationJobsCount)` (`:5465`) — **tens of tiles in flight instead of one.**
- The runtime scatter drops **738 blocking HISM instances**, dirtying essentially the whole **28×13 = 364-column** grid. **MEASURED: 326 tiles / ~216 s / ~1.5 tiles/s** (`handoffs/TASK-366-buildmaster.md:143-152`), corroborated by an independent **178 s** far-castle nav-mark figure (`handoffs/TASK-350-buildmaster.md:19`).
- ⭐ **AND THIS IS WHY THE FAR FIELD IS THE WORST PLACE TO SEND A UNIT: tiles sort by seed location, and `GetSeedLocations` collects PLAYER-PAWN POSITIONS ONLY** (`RecastNavMeshGenerator.cpp:7047-7059`) — the field settles **outward from the hero**, so the far/mid field settles **LAST**. ⇒ **A unit ordered across the map paths over tiles still holding the PRE-SCATTER bake: the navmesh says open ground, the rock's collider says no. That is the reported symptom, exactly.**

**⭐ CAUSE 2 — NOTHING DETECTS A STUCK UNIT, SO A STALL BECOMES PERMANENT.**
`UPathFollowingComponent`'s engine defaults (`PathFollowingComponent.cpp:136-144`: `BlockDetectionDistance=10`, `BlockDetectionInterval=0.5`, `BlockDetectionSampleCount=10`) declare **`EPathFollowingResult::Blocked` after moving <10 uu in 5.0 s.** ⭐ **THE ENGINE ALREADY KNOWS.**
- ⛔ **But there is NO `OnMoveCompleted` override anywhere in the project and ZERO references to `EPathFollowingResult`, so the verdict is DISCARDED** — and 0.25 s later `SummonedUnit.cpp:2440`'s `GetMoveStatus() == EPathFollowingStatus::Idle` branch re-issues the **byte-identical** request. Same start poly, same goal, same filter, same path, same rock.
- ⇒ **A silent 5-second livelock loop, with NO telemetry** (the `Failed` log at `:2485` is gated on `bGoalChanged`). ⚖️ **We are not adding a detector the engine lacks; we are harvesting a verdict it is already computing and throwing away.**

**CAUSE 3 — THE TRAVERSABILITY GUARANTEE IS CONFIRMED AGAINST A ~3 %-BUILT NAVMESH, AND ITS RETRY PATH HOLDS A DETERMINISM HOLE.**
`BattlefieldScatter.cpp:1953` waits on `UNavigationSystemV1::IsNavigationBeingBuilt` and then **gives up on a 10 s cap and confirms anyway**. Log: scatter at `00.44.39`, `Traversability CONFIRMED` at `00.44.44` — **+5.03 s**, against a queue that takes ~216 s to drain. ⇒ The `NON-NEGOTIABLE` guarantee (CONVENTIONS "Battlefield & procedural terrain (M6.5)", the TRAVERSABILITY GUARANTEE bullet) **is not being tested.**

> ### ⭐ **CORRECTED 2026-08-04 (TASK-535 found it, `qa/TASK-537.md` RULING 3 verified BOTH halves first-hand against installed UE 5.8 source and the log). ⛔ THE CONCLUSION ABOVE STANDS UNCHANGED — THE CONFIRMATION *IS* DISHONEST — BUT THE MECHANISM THIS SECTION ORIGINALLY STATED WAS WRONG, AND IT WAS MY WORDING.**
>
> **WHAT I WROTE, AND WHY IT CANNOT BE TRUE:** the earlier text said the poll *"reads idle ~211 s before the queue drains"* — i.e. that `IsNavigationBeingBuilt` returns `false` while tiles are still pending. **It cannot.**
> - `UNavigationSystemV1::IsNavigationBeingBuilt` = `HasDirtyAreasQueued() || IsNavigationBuildInProgress()` (`NavigationSystem.cpp:5556`), and `IsNavigationBuildInProgress` ORs `FRecastNavMeshGenerator::IsBuildInProgressCheckDirty()` over `NavDataSet` (`:4900-4908`).
> - `IsBuildInProgressCheckDirty()` = `RunningDirtyTiles.Num() || PendingDirtyTiles.Num() || SyncTimeSlicedData.TileGeneratorSync.IsValid()` (`RecastNavMeshGenerator.cpp:7764-7769`) — and `GetNumRemaningBuildTasks()` is the **arithmetic sum of exactly those three terms** (`:7781-7784`, `RecastNavMeshGenerator.h:794`), summed over the same `NavDataSet` (`NavigationSystem.cpp:5549` / `:4889`).
> - ⇒ ⭐ **`IsNavigationBuildInProgress() == false` ⟺ `GetNumRemainingBuildTasks() == 0`.** The two are the same three terms under an OR and under a `+`. **The poll cannot read idle with 326 tiles pending**, and a fix that merely swapped the predicate for `GetNumRemainingBuildTasks() == 0` **would have been a no-op on this path.**
> - 🔎 **ONE REFINEMENT (QA's, which TASK-535 omitted): `IsNavigationBeingBuilt` ALSO returns `false` outright under `IsNavigationBuildingPermanentlyLocked()` (`NavigationSystem.cpp:5554`).** So the equivalence reads *"…unless nav building is permanently locked."* It does not disturb anything here (`RuntimeGeneration=Dynamic`, tiles demonstrably building) — **but the law says it, because the one case where the implication breaks is exactly the case a future reader will hit and not understand.**
>
> **⭐ THE ROUTE QA PROVED — `MaxNavSettleWait`'s 10 s CAP, VERIFIED IN THE LOG 4 OF 4:** `Saved/Logs/GitClaudeUnrealTest-backup-2026.08.04-02.55.13.log` lines **2583/2584 · 2769/2770 · 3040/3041 · 3241/3242** — each one `Navigation still building after 10.0 s (MaxNavSettleWait cap) — proceeding with the reachability validation anyway`, **immediately followed by `Traversability CONFIRMED`.** ⇒ **In that session the false `CONFIRMED` came from the WAIT GIVING UP, not from a false idle.** ⚖️ **The guarantee was never being tested — and a fix aimed only at the predicate would have missed this route entirely.** ✅ Arithmetic checks: scatter `02.38.09:139` → confirm `02.38.21:783` = **+12.6 s**, i.e. the 10 s cap plus the validation itself.
>
> ### ⛔⚠️ **AND A SECOND ROUTE EXISTS, WHICH I VERIFIED MYSELF AND WHICH QA'S CORRECTION DOES *NOT* EXPLAIN — ⛔ DO NOT CLOSE CAUSE 3 ON THE CAP ALONE.**
> **The `+5.03 s` datum THIS SECTION CITES IS FROM A DIFFERENT SESSION, AND THAT LOG CONTAINS *ZERO* CAP LINES.** `Saved/Logs/GitClaudeUnrealTest.log`: scatter `00.44.39:632` (`:1891`) → `Traversability CONFIRMED` `00.44.44:659` (`:1977`) = **+5.03 s**, with **no `MaxNavSettleWait` line, no `still building`, and no `PROVISIONAL` anywhere in the file** (checked case-insensitively across the whole log). **+5.03 s is less than half the 10 s cap ⇒ THE CAP CANNOT HAVE FIRED. The wait EXITED EARLY, having concluded nav was settled.**
> - ⇒ **Two sessions, the same pre-batch binary, TWO DIFFERENT ROUTES to a `CONFIRMED`.** ⛔ **QA's route is proven for the 02:38 session; it is NOT the explanation for the datum this section was built on.**
> - ⚠️ **WHICH MEANS THE CANDIDATES FOR THE +5.03 s CASE ARE STILL OPEN**, and they are exactly three: **(i)** nav had *genuinely* settled by +5 s (⚠️ hard to reconcile with ~326 dirtied tiles at ~1.5 tiles/s, **but not disproven** — that measurement is from a different run); **(ii)** ⭐ **the `IsNavigationBuildingPermanentlyLocked()` short-circuit** — QA's own refinement, which QA set aside as academic and which this datum **promotes to the leading candidate**; **(iii)** an absent/unregistered nav system making the wait exit vacuously.
> - ### ✅ **THIS DOES NOT WEAKEN THE BATCH — IT IS THE ONE THING STAGE 0 WAS BUILT TO ANSWER, AND IT NOW HAS A SECOND JOB.** **TASK-538's `[at-confirmation]` snapshot (`remaining=` · `dirtyAreas=` · `hasDirty=`) distinguishes all three on the first run**, and `NAV-§4`'s honest labelling means the run will say `PROVISIONAL` outright if tiles are pending. ⛔ **Both fixes ship either way; nothing here changes a line of code.**
> - ⛔ **AND THE INSTRUCTION THAT FOLLOWS FROM IT: a TASK-538 report showing `CONFIRMED` with a NON-ZERO `remaining=` is route (i) refuted and is a `NAV-§4` FAIL to be reported, not smoothed. A `CONFIRMED (nav settled: 0 pending)` at ~+5 s with the cap silent is route (i) CONFIRMED and closes cause 3 honestly.**
> - ⚖️ **Recorded 2026-08-04 by the manager, checked in the log files directly rather than accepted from the gate.** **I was correcting a clause whose defect was asserting an unverified mechanism; adopting a replacement mechanism without checking it against the datum it was replacing would have repeated the identical error one paragraph later.**
>
> ✅ **AND THE SHIPPED STRENGTHENING IS ACCEPTED, NOT MERELY TOLERATED:** TASK-535's `bNavSettled = (GetNumRemainingBuildTasks() == 0) && !IsNavigationBeingBuilt(World)` (`BattlefieldScatter.cpp:2085-2086`) is a **conjunction**, so it can only ever make `CONFIRMED` **harder** to print and the cull gate **stricter** — the direction `NAV-§4` demands. It also closes the dirty-areas-not-yet-tile-tasks hole one level up. ⛔ **DO NOT REVERT IT** on the strength of the equivalence above; "redundant today" is not "wrong", and this predicate is load-bearing for a law that must survive an engine upgrade.
>
> ⚖️ **RECORDED AS A METHOD NOTE, BECAUSE IT IS THE LESSON: this section asserted an engine mechanism that was never read at the source. The correction cost one QA cycle. `NAV-§5`'s closing line — *"an unverified engine assumption is how this feature got a 216-second navmesh in the first place"* — applies to the LAW FILE too, and this is the proof.**
- ⛔ **WORSE: the cull-retry path (`BattlefieldScatter.cpp:2044-2049`) DELETES INSTANCES on the strength of that timing-dependent query** ⇒ **the shipped instance set is not a pure function of the seed**, which is a live hole in the determinism law (`TASKBOARD.md:11044`). **`NAV-§4` closes it.**

### NAV-§2. ⛔ THREE THEORIES RULED OUT, WITH EVIDENCE — WORDED SO THEY CANNOT BE RE-PROPOSED

⚖️ **Each of these is the *first* thing a competent reader reaches for, which is exactly why the refusal is written down rather than left to be re-derived.** ⛔ **A task, QA finding, handoff or plan that proposes any of the three is answered by this clause and is a FINDING against its author, not a suggestion.**

**(a) ⛔ `AgentRadius` IS A RED HERRING. RAISING IT CANNOT WIDEN THE CLEARANCE AND CAN BREAK NAV REGISTRATION.**
- `RecastNavMeshGenerator.cpp:5306`: `OutConfig.walkableRadius = FMath::CeilToInt(AgentRadius / CellSize)`. With `CellSize = 32`, `ceil(34/32) = 2` voxels = **64 uu of REAL clearance** — which already covers **every shipped capsule** (30 / 40 / 45 / **60** Ogre).
- ⇒ ⭐ **ANY `AgentRadius` IN (32, 64] YIELDS A BYTE-IDENTICAL NAVMESH.** The knob is quantised; the number in the ini is not the number Recast uses.
- ⛔ **Raising it to 65+ LOSES walkable area, inflates the per-tile gather box (`:1811`), and — because `FNavAgentProperties::IsEquivalent` uses `Precision = 5.f` (`NavigationTypes.h:483-491`) — makes `L_Arena`'s serialized nav data FAIL REGISTRATION (`RegistrationFailed_AgentNotValid`).** The engine also forbids the actor-side route outright (`RecastNavMesh.cpp:4246`).

**(b) ⛔ `L_Arena`'s BAKE IS FINE. THE `loaded empty` WARNINGS BELONG TO THE MENU MAP.**
- The `FPImplRecastNavMesh::Serialize … loaded empty` warnings fire at **`00.44.33:603`**, inside the **`LoadMap: /Game/Maps/L_MainMenu`** window (log `:1747`). **`L_Arena` loads at `00.44.38:666` (`:1847`) and produces NO warning.** The stale bake is the **menu map's**, and the menu map holds no units.
- ⇒ ⛔ **NO `L_Arena` SAVE. NO NEW JONATHAN EXCEPTION. See `NAV-§5`.**
- ⚖️ **This one is recorded with special force because it is the theory that would spend a user exception on a non-problem** — and the exception that would be cited is already **spent** (`TASKBOARD.md:9485-9489`, *"do not cite it twice"*).

**(c) ⛔ CROWD / RVO AVOIDANCE IS THE WRONG TOOL — IT IS BOTH EXPENSIVE *AND* BLIND TO THE THING WE ARE STUCK ON.**
- `UCrowdFollowingComponent` costs an **O(neighbours) proximity query + velocity solve per agent, every crowd tick, forever** — a **permanent** tax at 60–120 units to fix a **transient** problem, and **no FPS baseline exists** to measure it against (M7's TASK-183 never ran). ⚠️ Jonathan's own constraint was *"without it being too costly to compute."*
- ⛔ **AND THE FATAL HALF: `bUseRVOAvoidance` AVOIDS *PAWNS*, NOT STATIC GEOMETRY.** Rocks are `ECC_WorldStatic` HISM colliders and are **invisible to it**. ⇒ **It would not move a unit around a rock at any radius.**

### NAV-§3. ⛔ THE ANTI-MILL LAW, RESTATED FOR THE WATCHDOG · THE NO-DOUBLE-DRIVER LAW · AND WHAT EACH RUNG IS ALLOWED TO DO

> ### ⛔ **A FOLLOW IMPLEMENTATION THAT RE-PATHS UNCONDITIONALLY IS A QA FAIL** (the standing law, CONVENTIONS "FOLLOW command…" §4). ⛔ **THE WATCHDOG INHERITS IT IN FULL: A STUCK LADDER THAT CAN RE-ISSUE A MOVE EVERY POLL IS THE TASK-280 / TASK-282 MILL WEARING A RESCUE'S CLOTHES.**

**TWO BRAKES, BOTH STRUCTURAL, BOTH REVIEWABLE — ⛔ AND THEY DO NOT BRAKE THE SAME THING:**
1. **BRAKE 1 — THE *RATE* BRAKE. A rung fires at most once per `EscalationCooldown` (1.0 s).**
2. **BRAKE 2 — THE *COUNT* BRAKE. `EscalationLevel` is MONOTONIC within a stall** — a rung never re-fires until the stall is cleared and `Reset` runs. ⇒ **≤ 3 rungs and ≤ 2 path requests PER STALL** (`Abandon` issues no request of its own).
⇒ ⭐ **WORST CASE AT THE SHIPPED DEFAULTS: ≤ 1 extra path request per unit per second — measured 0.32/unit/s — and ONLY for units that are demonstrably not moving.** ⛔ **Any implementation whose worst case exceeds that is a QA FAIL by construction, not a tuning note.**

> ### ⛔ **CORRECTED 2026-08-04 — `qa/TASK-537.md` RULING 2 / WARN-1. THE ORIGINAL WORDING OF THIS CLAUSE (AND THE SHIPPED COMMENTS AT `SiegeStuckStatics.cpp:107-134` / `.h:120-124`, WHICH TASK-531 IS CORRECTING IN THE SAME LOOP) CLAIMED *"EITHER BRAKE ALONE BOUNDS THE REQUEST RATE"*. ⛔ THAT IS FALSE, AND IT WAS MY WORDING.**
>
> **QA'S DERIVATION, RECORDED VERBATIM BECAUSE THE NUMBER IS NOW A TUNING CONSTRAINT AND NOT A BOAST:**
> > Per stall: ≤ 3 rungs, of which `Abandon` issues no request ⇒ **≤ 2 requests**. `Abandon`'s `Reset` drops `bHasAnchor` ⇒ ≥ 1 dead re-anchor poll per cycle ⇒ minimum cycle **4 polls at 0.25 s**. **Worst case over legal `EditDefaultsOnly` tuning = 2.0 requests/unit/s** (`EscalationCooldown = 0`, thresholds 0.25 / 0.50 / 0.75). **Shipped defaults = 0.32 requests/unit/s.** The `≤ 1/unit/s` ceiling rests on **`EscalationCooldown ≥ 1.0` ALONE** — brake 2 is a *count* brake, not a rate brake.
>
> **THE STEP THE OLD WORDING MISSED: a stall's DURATION is itself tunable.** Brake 2 caps requests *per stall*; it says nothing about how many stalls per second the thresholds permit. Only brake 1 divides by wall-clock time. For scale, the forbidden per-poll mill is **4.0/unit/s**, so the pathological-but-legal tuning is **half the mill and twice the stated ceiling**.
>
> ### ⛔ **⇒ THE CEILING IS A TUNING INVARIANT, AND `EscalationCooldown` IS THEREFORE NOT A FREE KNOB.**
> - ⛔ **`EscalationCooldown < 1.0` IS FORBIDDEN WITHOUT RE-DERIVING THIS CEILING AND RECORDING THE NEW NUMBER HERE.** ⚠️ It is `EditDefaultsOnly`, so **a feel-tune in the editor can breach the anti-mill law with no code review, no compile and no QA gate** — that is precisely why it is written down as law rather than left in a comment.
> - ⚠️ **A FEEL PASS THAT LOWERS `EscalationCooldown` MUST STATE THE RESULTING REQUESTS/UNIT/S**, using the formula above (`2 requests / (max(4 polls × 0.25 s, cycle implied by the thresholds and the cooldown))`). **Lowering `SidestepSeconds`/`WidenSeconds`/`AbandonSeconds` alone cannot breach the ceiling while `EscalationCooldown ≥ 1.0` holds** — the cooldown is the only term that bounds rate.
> - ✅ **The two automation tests that carry this corrected claim are `EscalationCooldownGatesTheLadder` and `MonotonicLevelHoldsWithoutTheCooldown`** — they are what make brake 1 and brake 2 independently load-bearing. ⛔ **Neither may be deleted or merged.**
> - ⚖️ **This was a WORDING defect, never a shipped one.** The implementation always had both brakes; the law overstated what one of them buys. TASK-536's finding 1 measured it correctly and independently.

**⛔ THE NO-DOUBLE-DRIVER LAW (CONVENTIONS "FOLLOW command…" §6, the double-drive clause) BINDS EVERY PIECE OF THIS FEATURE:**
- ⛔ **ZERO NEW TIMERS *IN THE WATCHDOG LANE*.** The watchdog rides the **two 0.25 s polls that already exist** — `ASummonedUnit::UpdateState` and `AMinerUnit::UpdateMining`. ⛔ **A second 0.25 s driver on one `UPathFollowingComponent` is forbidden and a `SetTimer` on a UNIT in this feature is a finding.**
  - ⚠️ **SCOPED 2026-08-04 (`qa/TASK-537.md` WARN-8). As originally written this clause read BATCH-WIDE, which its own justification does not support** — the justification is about *two steering authorities on one movement component*, and that hazard exists only on a pawn that has one.
  - ✅ **THE ONE DECLARED, BOUNDED EXCEPTION: `ASiegeBattlefieldScatter::DefinitiveCheckTimerHandle` (TASK-535).** `ASiegeBattlefieldScatter` **owns no movement component**, is **already timer-driven** (`TraversabilityTimerHandle`), and the new handle is a **one-shot 1 ms deferral that never re-arms itself** (`BattlefieldScatter.cpp:2379-2381`), cleared on `EndPlay` (`:248`) and on every re-scatter (`:464`), and kept **separate** from the settle-poll handle because both can legitimately be in flight. ⛔ **RULED IN by QA; do not re-flag it.**
  - ⭐ **AND THE ALTERNATIVE IS WORSE, WHICH IS WHY THE DEFERRAL EXISTS: culling HISM instances inline from inside the Recast generator's own tick re-enters the nav system it is dirtying.** ⚖️ **The 1 ms hop off the generator's stack is the cheap, correct shape — ⛔ do not "simplify" it away.**
  - ⛔ **THIS EXCEPTION IS EXHAUSTIVE.** Any *further* `SetTimer` in this feature is still a finding, and a new one on a unit is still forbidden outright.
- ⛔ **`ASiegeUnitAIController::OnMoveCompleted` MAY NOT ISSUE A MOVE.** ⭐ **THE ENGINE'S VERDICT IS *EVIDENCE*, NOT AN *ACTION*.** It emits one throttled log line and calls `ASummonedUnit::NotifyMoveBlocked()`; **only the watchdog tick ever issues a move.** ⚖️ Two code paths that can both re-path is precisely the defect this whole section exists to remove.
- ⛔ **`NotifyMoveBlocked()` DOES NOT BYPASS THE BRAKES.** It may advance the stall clock so the **next** watchdog tick fires a rung — it may **never** fire one itself, and it passes through the same cooldown and monotonicity gates.
  - ⚠️ **EMPHASIS CORRECTED 2026-08-04 (`qa/TASK-537.md` WARN-6): `NotifyMoveBlocked` IS CORROBORATING EVIDENCE, ⛔ NOT A PRIMARY TRIGGER — and in the exact case it was written for it contributes NOTHING.** A `Blocked` verdict means the request **finished**, so the next poll reads `!bAdvancing`, `Evaluate` re-anchors and `Reset`s, and the clock bump is wiped. **Read this bullet as "an extra, weaker input", never as "the engine tells us and we escalate."**
  - ⛔ **IT IS NOT DEAD CODE AND IS NOT REMOVED.** In the *corroborating* case — a live request still in flight on the next poll — the bump pulls the first rung forward by **0.75 s** and still passes both brakes. **Ships as-is** (`SummonedUnit.cpp:3051-3078`).
  - ⚖️ **The timing argument is what settles it: the engine declares `Blocked` at 5.0 s, by which point our own 1.5 s / 3.0 s rungs have ALREADY fired off our own clock.** ⇒ **The ladder never depended on this signal, and must not be written as if it did.**
  - 🔎 **TASK-538 EVIDENCE SIGNATURE: `blocked:` lines with NO `escalate:` line following on that unit are this hole behaving EXACTLY as documented.** ⛔ **Record them; do not report them as a bug.**
- ⛔ **`AMinerUnit` OVERRIDES `HandleStuckEscalation` ONLY.** The *decision* is identical; the *action* must differ, because the miner's walk is owned by `EnsureWalkingToNode` / `DriveToPoint` and running the base's rungs would put a **second steering authority** on its movement component.

**THE THREE RUNGS — the OUTCOME is law, the mechanism is the programmer's (with a written justification in the handoff):**

| rung | fires at | what it is ALLOWED to do | ⛔ what it may NEVER do |
|---|---|---|---|
| **`Sidestep`** | 1.5 s stalled | issue ONE move to `ComputeSidestepGoal(...)` and take a **lease** for `SidestepLeaseSeconds` | ⛔ clear or forget the standing order; ⛔ re-issue while the lease is live |
| **`WidenAndRepath`** | 3.0 s | re-issue the **ORIGINAL** goal ONCE with a genuine re-path (e.g. clear `CurrentMoveGoal` so the next `EnterAdvance*` really re-issues) | ⛔ add a second concurrent move request; ⛔ widen anything that persists past the stall |
| **`Abandon`** | 6.0 s | drop the current move goal, log once, let the standing body re-choose next poll (`EnterIdle()` is acceptable), then `Reset` | ⛔ cancel the unit's standing ORDER permanently; ⛔ enter Attack; ⛔ leave the unit inert forever |

- ⚠️ **THE SIDESTEP LEASE IS LOAD-BEARING, NOT A POLISH ITEM.** `EnterAdvanceToLocation` nulls `CurrentMoveGoal` (`SummonedUnit.cpp:2675` — ⚠️ **was cited here as `:2581`; drifted, corrected 2026-08-04 per `qa/TASK-537.md` NIT-5. ⛔ Grep the symbol, never the line**), so **without a lease the next poll's `bGoalChanged` cancels the sidestep after 0.25 s** and the rescue never happens. ⛔ **Clear the lease in `EnterIdle`, `EnterAttack`, on death, and in `FreezeAI`.** ⚖️ A lease that outlives the unit's state change is a stuck unit of a new kind.
  - ✅ **FIVE CLEAR SITES SHIPPED, NOT FOUR, AND THE FIFTH IS RULED IN:** `EnterIdle` · `EnterAttack` · `HandleDeath` · `FreezeAI` · **`ApplyFreeze`**. **`ApplyFreeze` writes `State = Idle` DIRECTLY (`SummonedUnit.cpp:928`) so it never routes through `EnterIdle`, whose own early-out would refuse the cleanup anyway — and it clears `StateTimerHandle`, so the lease cannot even drain.** ⇒ **Without it a thawing unit steers to a pre-freeze `SidestepGoal`.** ⛔ **The fifth site is now part of the law; a future edit that drops it re-opens the bug.**
  - ✅ **THE LEASE IS DISPATCH SUPPRESSION, ⛔ NOT PER-POLL RE-STEER** — settled at the gate, see `NAV-§14`(a).
- ⚠️ **`DeltaSeconds` IS A PARAMETER, ⛔ NEVER READ FROM `StateCheckInterval`.** `AMinerUnit` sets that to **0** by seal (`MinerUnit.cpp:62` — ⚠️ **cited as `:61` on the board; drifted, `qa/TASK-537.md` NIT-5**). ⭐ **This is the same latent bug shape `TrackChargeMovement` (`SummonedUnit.cpp:2733`) already has — harmless there only because `bCharge` gates it out.** ⇒ **Both call sites take the delta from the WORLD CLOCK via `ConsumeStuckDeltaSeconds()` (`NAV-§8`).** ⛔ **A `DeltaSeconds` of 0 must never advance a rung, never divide, and never assert** — it is an explicit automation-test case.
- ✅ **`bAdvancing` MEANS "THIS UNIT HAS AN ACTIVE PATH-FOLLOWING REQUEST THIS POLL"** — the shipped idiom is `GetMoveStatus()` (already read at `SummonedUnit.cpp:2440`). ⛔ **A unit that is idle BY DESIGN (holding, stationed, no request) must evaluate to `None` regardless of elapsed time**, or the ladder will "rescue" units that were told to stand still. **Frozen/dead units never reach the call at all — `UpdateState`'s freeze early-out is above the call site.**

**COST, STATED STRUCTURALLY — ⛔ no fps claim, because no baseline exists:** per unit per 0.25 s poll, **one `DistSquared`, one float compare, two adds, one `uint8` compare. Zero allocations, zero world queries.** ⭐ **FOR SCALE: `AcquireTarget` already runs a full-world `GetAllActorsWithInterface` WITH a `TArray` allocation, per unit, on that same poll** (`SummonedUnit.cpp:1453-1454`), plus six `TActorIterator` sweeps. **The watchdog is orders of magnitude cheaper than what already runs on that exact tick** — say so when anyone asks whether it is "too costly to compute."

### NAV-§4. ⛔ THE DETERMINISM LAW'S NEW OBLIGATION — **A CULL MAY ONLY BE DRIVEN BY A *SETTLED* QUERY**

> ### ⛔ **THE SHIPPED INSTANCE SET MUST BE A PURE FUNCTION OF THE SEED (`TASKBOARD.md:11044`). A CULL DECIDED BY A QUERY AGAINST A PARTIALLY-BUILT NAVMESH IS A CULL DECIDED BY *WALL-CLOCK TIMING*, AND IT BREAKS THAT LAW SILENTLY.**

- **HONEST LABELLING IS MANDATORY:** `ValidateTraversability` reads `GetNumRemainingBuildTasks()` and logs **either** `CONFIRMED (nav settled: 0 pending)` **or** `PROVISIONAL (N tile task(s) pending — PRE-SETTLE query)`. ⛔ **A pre-settle query may never print the word `CONFIRMED`** — that string is a claim, and today it is a false one.
- **`bCullOnProvisionalFailure` (EditDefaultsOnly, default `false`)**: ⛔ **a PROVISIONAL failure NEVER runs the widening cull.** ✅ **That single flag is what makes the instance set a pure function of the seed again.**
- **THE DEFINITIVE CHECK IS EVENT-DRIVEN:** bind `UNavigationSystemV1::OnNavigationGenerationFinishedDelegate` (`NavigationSystem.h:444`) and re-run the reachability check **ONCE**, when generation actually finishes. **One delegate bind, one extra path query per match — no polling, no 216 s stall.** ⛔ **Unbind on `EndPlay`; null-safe if the nav system is absent.**
- ⭐ **AND THE REASON THIS *KEEPS* THE NON-NEGOTIABLE GUARANTEE RATHER THAN WEAKENING IT: post-settle, the reachability answer is STABLE, so a cull driven by it is a pure function of the geometry — hence of the seed.** ⇒ **The cull is not removed; it is moved to the only moment at which it is both TRUE and DETERMINISTIC.**
- 🚩 **FLAGGED CONSEQUENCE, ACCEPTED, JONATHAN'S CALL AT THE PLAYTEST: a post-settle cull deletes instances LATER — potentially visibly (a rock popping out ~27 s in at 8× concurrency, or ~216 s if Stage 1 rolls back), where today it happens invisibly at +5 s.** ⚖️ **It only ever fires when the field is genuinely walled off, which is the HARD-FAILURE case the guarantee exists for — a visible pop is strictly better than an unwinnable match.** ⛔ **Do not "fix" this by restoring the provisional cull.**

### NAV-§5. ⛔ THE `L_Arena` NEVER-SAVE LAW — **RESTATED, AND EXPLICITLY UNTOUCHED BY THIS BATCH**

> ### ⛔ **NO TASK IN THIS BATCH OPENS, SAVES, OR COMMITS `Content/Maps/L_Arena.umap`. NO `.umap` WRITE OF ANY KIND. THE BATCH IS `.h`/`.cpp`/`.ini` ONLY.**

- ⛔ **THE ONE-TIME NAVMESH-SAVE EXCEPTION (`TASKBOARD.md:9485-9489`) IS SPENT AND STAYS SPENT** — its own text says *"do not cite it twice."* ⛔ **Citing it here is a finding.**
- ⛔ **AND THE PRE-EMPTIVE CLOSE, BECAUSE THIS IS EXACTLY WHERE SOMEBODY WOULD REACH FOR IT: if `NAV-§9`'s conditional shows the ini did NOT reach the serialized nav actor, THE ANSWER IS THE ROLLBACK, NOT A MAP SAVE.** ⚠️ *"We could just re-save the level with the right nav settings"* is the tempting, wrong move, and it is refused **in advance** so nobody has to refuse it under pressure.
- ⛔ **THE THIRD ROUTE IS CLOSED TOO: no agent may mutate the live `ARecastNavMesh` actor's config at runtime (BeginPlay or otherwise) to force the flag.** ⚠️ It is a *plausible* route and it is **unverified** — whether the generator re-reads `GatherGeometryOnGameThread()` after construction was **not** established. ⇒ **It is recorded as a CANDIDATE for a future, verification-first task with Jonathan's word, and is ⛔ NOT authorized in this batch.** ⚖️ *An unverified engine assumption is how this feature got a 216-second navmesh in the first place.*
- ✅ **BUILD-MASTER VERIFIES `L_Arena`'s SHA256 IS UNCHANGED at the gate** (the standing check, TASK-526 precedent).

### NAV-§6. ⚠️ THE LATENT TILE-POOL HAZARD — **INSTRUMENT IT, DO NOT FIX IT**

- `bFixedTilePoolSize=True` + `TilePoolSize=1024` against **364 columns = 2.81 layers/column**, below the engine's own `AverageLayersPerTile = 3` (`RecastNavMesh.cpp:514`).
- ⛔ **Overflow DROPS TILES with a PERMANENT hole and logs `tile limit reached!` (`RecastNavMeshGenerator.cpp:6366-6369`) — which would look EXACTLY like "stuck behind a rock, forever."** ⚠️ **Not seen in current logs, but the logs are not at a verbosity that guarantees it would show.**
- ⇒ **The telemetry prints `activeTiles=` against `poolCap=` at three points and the gate greps for `tile limit reached!`.** ⛔ **DO NOT raise `TilePoolSize` on this batch.** ⚖️ **A number changed on suspicion is the exact defect DIAGNOSE-FIRST exists to prevent, and this hazard has never once been observed to fire** (`SC-§32`: a mechanism never observed to function is not known to function — and the converse holds for a hazard never observed to fire).

### NAV-§7. NAMING + FOLDER LAW (the cross-task contract)

| thing | law |
|---|---|
| stuck statics | **`FSiegeStuckStatics`** — `Source/GitClaudeUnrealTest/Siegebound/SiegeStuckStatics.{h,cpp}`. Plain static library, **not a UObject**, `GITCLAUDEUNREALTEST_API`. Precedent: `FSiegeCombatStatics` (`SiegeCombatStatics.h:23`). ⭐ **ALL pure, testable ladder logic lives here.** |
| nav telemetry | **`FSiegeNavDiagnostics`** — `Source/GitClaudeUnrealTest/Siegebound/SiegeNavDiagnostics.{h,cpp}`. Same shape. ⛔ **Pure reads of public API: no state, no ticking, no behaviour change of any kind.** |
| state / tuning / action types | **`FSiegeStuckState`** · **`FSiegeStuckTuning`** · **`ESiegeStuckAction`** share `SiegeStuckStatics.h`. ✅ **This is the existing "pure data types may share a header when they form one concept" exception (`TeamId.h` precedent) — ⛔ QA must not flag it as a one-class-per-header violation.** |
| ⭐ log category (stuck) | **`LogSiegeStuck`** — declared in `SiegeStuckStatics.h`, defined in its `.cpp`. |
| ⭐ log category (nav) | **`LogSiegeNavDiag`** — declared in `SiegeNavDiagnostics.h`, defined in its `.cpp`. ⛔ **NOT `LogSiegeNav`, and this is deliberate: `LogSiegeNet` already exists (`SiegeSessionSubsystem.h:18`) and the two differ by ONE CHARACTER in a log file somebody will be skimming at 2 a.m.** ⚠️ **A "simplification" to `LogSiegeNav` re-introduces the collision and is a finding.** |
| tests | **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeStuckStaticsTest.cpp`**, `#if WITH_DEV_AUTOMATION_TESTS`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with `EAutomationTestFlags::EditorContext \| EAutomationTestFlags::EngineFilter` (the `SiegeSettingsTest.cpp:113-116` pattern). **Test names live under `Siegebound.Nav.Stuck.<Name>`.** |
| ini keys | `[/Script/NavigationSystem.RecastNavMesh]` (`Config/DefaultEngine.ini:266`) gains **`bDoFullyAsyncNavDataGathering=False`** (an EDIT of `:294`) and **`MaxSimultaneousTileGenerationJobsCount=8`** (NEW). ⛔ **Nothing else in that block moves.** |
| tunables | **`FSiegeStuckTuning`'s fields are the ONLY tunables this feature adds, ALL `EditDefaultsOnly`, ALL flagged for Jonathan's feel pass** (`ProgressRadius` 150 · `MinSpeedSq` 2500 · `SidestepSeconds` 1.5 · `WidenSeconds` 3.0 · `AbandonSeconds` 6.0 · `EscalationCooldown` 1.0 · `SidestepDistance` 350 · `SidestepLeaseSeconds` 2.0). ⛔ **No bare literals at a call site.** |

⛔ **THE LOG-LINE TOKENS ARE PART OF THE CONTRACT, BECAUSE THEY ARE THE GATE'S EVIDENCE.** The exact sentence wording is the programmer's; **these `key=` tokens are pinned and are what the PIE evidence greps for:**
- **config line (once):** `gatherOnGameThread=` · `maxTileJobs=` · `cellSize=` · `tileSizeUU=` · `agentRadius=` · `poolCap=` · `fixedPool=` · `runtimeGen=`
  - ⚠️ **`cellSize=` IS PRODUCED BY `GetCellSize(ENavigationDataResolution::Default)`, ⛔ NOT BY THE DEPRECATED `ARecastNavMesh::CellSize` MEMBER** (`RecastNavMesh.h:710-712` — a real `UE_DEPRECATED(all, …)`; corrected 2026-08-04, `qa/TASK-537.md` WARN-4, full reasoning in `NAV-§8`). **The TOKEN is unchanged and nothing downstream re-greps** — only the accessor behind it was mis-specified. ⛔ **`agentRadius=` still reads the `AgentRadius` member, which is NOT deprecated.**
- **snapshot line (three times):** a tag ∈ **`pre-scatter` | `post-scatter` | `at-confirmation`**, plus `remaining=` · `running=` · `dirtyAreas=` · `hasDirty=` · `activeTiles=` · `poolCap=`
- **stuck lines:** `blocked:` (the harvested engine verdict, throttled) and `escalate:` with `level=` · `action=` · `stalled=`
⛔ **The `names:` block of each task is the single source of truth (the standing cross-discipline rule). Every symbol above appears there character-for-character.**

### NAV-§8. ⚠️ PINNED CROSS-TASK SIGNATURE REGISTRY (this batch's link contract)

⚠️ **UBT compiles the whole module.** Every task in this batch compiles against this list **character-for-character**; "improving" a pinned signature breaks the link and is an **automatic QA FAIL.** **Access levels are part of the pin.** ⛔ **The shapes below are COPIED FROM THE APPROVED PLAN — do not invent variants.** Precedent for pinning before dispatch: `:453`, `:564`, `:915`, `:1345`, `KBD-§8`.

```cpp
// ── SiegeStuckStatics.h ───────────────────────────────────────────────────
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeStuck, Log, All);

/** What the ladder decided this poll. Plain enum — NOT a UENUM; it is never a UPROPERTY. */
enum class ESiegeStuckAction : uint8 { None, Sidestep, WidenAndRepath, Abandon };

/** Per-unit transient stall state. POD, ~32 bytes. ⛔ NOT a USTRUCT — it is runtime
 *  state, and reflecting it invites somebody to replicate it (NAV-§11). */
struct FSiegeStuckState
{
    FVector ProgressAnchor         = FVector::ZeroVector;
    float   StalledSeconds         = 0.f;
    float   SecondsSinceEscalation = 0.f;
    uint8   EscalationLevel        = 0;
    bool    bHasAnchor             = false;
};

/** ⭐ MANAGER ADDITION OVER THE PLAN, DECLARED NOT SILENT (SC-§15): the plan wrote a plain
 *  struct; this is a USTRUCT so the rungs are EditDefaultsOnly per-unit-class and Jonathan
 *  can tune the feel WITHOUT a recompile. The pinned FUNCTION signatures are unchanged. */
USTRUCT(BlueprintType)
struct FSiegeStuckTuning
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float ProgressRadius      = 150.f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float MinSpeedSq          = 2500.f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepSeconds     = 1.5f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float WidenSeconds        = 3.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float AbandonSeconds      = 6.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float EscalationCooldown  = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepDistance    = 350.f;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepLeaseSeconds = 2.0f;
};

class GITCLAUDEUNREALTEST_API FSiegeStuckStatics
{
public:
    static ESiegeStuckAction Evaluate(bool bAdvancing, const FVector& Location, float VelocitySizeSq,
                                      float DeltaSeconds, const FSiegeStuckTuning& Tuning,
                                      FSiegeStuckState& State);

    static FVector ComputeSidestepGoal(const FVector& Location, const FVector& Goal,
                                       float SidestepDistance, int32 Attempt);

    static void    Reset(FSiegeStuckState& State);
};

// ── SiegeNavDiagnostics.h ─────────────────────────────────────────────────
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeNavDiag, Log, All);

class GITCLAUDEUNREALTEST_API FSiegeNavDiagnostics
{
public:
    /** The RUNNING actor's config — this is the line that settles NAV-§9's conditional. */
    static void LogNavConfigOnce(const UWorld* World);

    /** Queue depth + pool headroom. Tag ∈ "pre-scatter" | "post-scatter" | "at-confirmation". */
    static void LogNavBuildSnapshot(const UWorld* World, const TCHAR* Tag);
};

// ── SummonedUnit.h ────────────────────────────────────────────────────────
// ASummonedUnit — public
void NotifyMoveBlocked();                        // called by ASiegeUnitAIController ONLY

// ASummonedUnit — protected
void         TickStuckWatchdog(float DeltaSeconds);
virtual void HandleStuckEscalation(ESiegeStuckAction Action);
float        ConsumeStuckDeltaSeconds();         // ⛔ world-clock delta; NEVER StateCheckInterval

FSiegeStuckState StuckState;                     // transient, unreflected
UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck")
FSiegeStuckTuning StuckTuning;
FVector SidestepGoal            = FVector::ZeroVector;
float   SidestepLeaseRemaining  = 0.f;
float   LastStuckTickTimeSeconds = 0.f;

// ── MinerUnit.h ───────────────────────────────────────────────────────────
// AMinerUnit — protected override
virtual void HandleStuckEscalation(ESiegeStuckAction Action) override;

// ── SiegeNavAreas.h ───────────────────────────────────────────────────────
// ASiegeUnitAIController — public override (existing class; ⛔ NO new class)
virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
```

- ⛔ **`OnMoveCompleted` HAS A DEPRECATED SECOND OVERLOAD (`AIController.h:232-233`, `UE_DEPRECATED_FORGAME(4.13, …)`). OVERRIDE ONLY THE `FPathFollowingResult` FORM (`AIController.h:230`, which is NOT deprecated).** ⚠️ **Overriding the deprecated one emits a deprecation warning.** If MSVC reports a hidden-overload warning (C4263/C4264), add **`using Super::OnMoveCompleted;`** — ⛔ **never silence it by implementing the deprecated overload.**
  > ### ⛔ **CORRECTED 2026-08-04 — `qa/TASK-537.md` RULING 4(b) / WARN-3. THIS BULLET USED TO END *"…and warnings are errors in this build."* ⛔ THAT IS NOT ESTABLISHED, AND I ASSERTED IT — HERE AND IN TWO DISPATCH PROMPTS.**
  >
  > **THE RECORD, CHECKED INDEPENDENTLY THREE TIMES (TASK-534 §8, TASK-536 finding 5, and QA's own grep of the whole of `Source/`): there is NO `bWarningsAsErrors` ANYWHERE** — not in `GitClaudeUnrealTest.Target.cs`, not in `GitClaudeUnrealTestEditor.Target.cs`, not in any `.Build.cs`; no `WarningLevel` or `DeprecationWarningLevel` either. **UBT's default is `WarningLevel.Warning`.** ⛔ **NO FUTURE TASK, HANDOFF, QA FINDING OR SPEC MAY CITE "warnings are errors here" AS ESTABLISHED.** If someone wants it to be true, that is a **boarded `Target.cs` change with Jonathan's word**, not a background assumption.
  >
  > ### ✅ **AND THE INSTRUCTION SURVIVES ITS WRONG JUSTIFICATION — ⛔ THE OVERLOAD CHOICE IS NOT IN DOUBT.**
  > **TASK-534 took the non-deprecated `AIController.h:230` overload and correctly omitted `using Super::OnMoveCompleted;`. BOTH ARE RIGHT AS SHIPPED and neither is re-opened by this correction.** The rule *"do not build on a deprecated API"* stands on its own feet and always did; **only the stated CONSEQUENCE was overstated** — it would have been a warning, not a build failure. ⚖️ **A right instruction with a wrong reason is still corrected, because the next reader inherits the reason, not the instruction.**
  > - ⚠️ **Practical consequence for TASK-538: a C4996/C4244 will NOT fail the build.** ⛔ **Report it anyway** — a deprecation warning from this batch means somebody reached for a deprecated API, and that is a finding regardless of exit code.
  > - ⛔ **The two shipped comments that assert it as fact (`SiegeNavDiagnostics.cpp:206`, `SummonedUnit.cpp:2873-2874`) are the programmer's to correct**, in the BLOCKER loop. Not mine, not this file's.
- ⭐ **`FSiegeNavDiagnostics::LogNavConfigOnce` READS `GetCellSize(ENavigationDataResolution::Default)` (`RecastNavMesh.h:1107`) — ⛔ NEVER THE `ARecastNavMesh::CellSize` MEMBER.**
  > ### ⛔ **CORRECTED 2026-08-04 — `qa/TASK-537.md` RULING 4(a) / WARN-4. THE ORIGINAL INSTRUCTION (this file's `NAV-§7` `cellSize=` token and `TASKBOARD.md:6997`'s *"the live `CellSize`"*) WAS UNFOLLOWABLE, AND IT WAS MY WORDING.**
  >
  > - **`ARecastNavMesh::CellSize` carries a REAL C++ deprecation** — `UE_DEPRECATED(all, "Use NavMeshResolutionParams …")` at `RecastNavMesh.h:710-712`, **not** mere reflection metadata. Same for `CellHeight` (`:714`) and `AgentMaxStepHeight` (`:718`).
  > - **And the value is not live anyway:** its migration into `NavMeshResolutionParams` is **editor-only and version-gated**, so the member can be stale at runtime even where it compiles. **`GetCellSize(ENavigationDataResolution::Default)` reads `NavMeshResolutionParams[Resolution].CellSize` — the value the generator actually uses.**
  > - ✅ **TASK-529 already shipped the correct call.** The pinned `cellSize=` **token is unchanged** — the token is the contract, the accessor is the implementation, and only the accessor was mis-specified. ⛔ **This is NOT a `NAV-§7` token change and nothing downstream re-greps.**
  > - ⚠️ **⛔ DO NOT OVER-CORRECT: `AgentRadius` (`RecastNavMesh.h:730`) is NOT deprecated** and is read directly, as specified. **`TileSizeUU`, `TilePoolSize`, `bFixedTilePoolSize` are likewise untouched by this correction.** ⚖️ *"One member of this class is deprecated"* does not license a sweep.
- ⛔ **`ASiegeUnitAIController` IS EXISTING (`SiegeNavAreas.h:116-125`). ⛔ NO NEW CONTROLLER CLASS, and `AIControllerClass` at `SummonedUnit.cpp:106` does not move.**
- ⛔ **NO `Build.cs` CHANGE** — `AIModule` and `NavigationSystem` are already public dependencies. ⚠️ **A `Build.cs` edit in this batch is a FINDING: it means somebody reached for the wrong header.**
- ⛔ **NO `.umap` WRITE (`NAV-§5`).**

### NAV-§9. ⚖️ THE DIAGNOSE-FIRST MANDATE, **DISCHARGED — NOT WAIVED** — AND THE CONDITIONAL THAT MAKES IT HONEST

> **`TASKBOARD.md:10201`: *"a fix committed without runtime evidence of the root cause is a QA FAIL by construction."*** ⛔ **That is binding, and Jonathan chose (NAV-§0 ruling 2) to ship telemetry and fixes together.** ⇒ **It is discharged by these four clauses, and a spec that merely asserts "diagnose-first is satisfied" has not discharged anything.**

1. **THE TELEMETRY SHIPS IN THE SAME BATCH AS THE FIXES**, and it is **Stage 0 — no behaviour change of any kind.** ⛔ A telemetry task that alters behaviour has broken the arrangement Jonathan agreed to.
2. **ITS LOG LINES ARE THE GATE'S ACCEPTANCE EVIDENCE**, not a nice-to-have. **The gate reads the tokens in `NAV-§7`, from a live run, and records the NUMBERS.**
3. ⭐ **THE CONCURRENCY FLIP (Stage 1) IS EXPLICITLY *CONDITIONAL*:**
   > ### ⛔ **IF THE PIE LOG STILL READS `gatherOnGameThread=false` AFTER THE FLIP, THE INI DID NOT REACH THE SERIALIZED NAV ACTOR. STAGE 1 IS REVERTED — BOTH KEYS — AND THE STUCK LADDER CARRIES THE FIX ALONE.**
   ⛔ **This is a BOARDED TASK (the batch's conditional lane), never a comment and never a promise.** ⛔ **DO NOT escalate to an `L_Arena` save over it (`NAV-§5`), and do not "try one more thing" first.**
4. **THE STUCK LADDER (Stage 2) IS INDEPENDENT AND CARRIES THE FIX EITHER WAY** — which is precisely what makes shipping them together safe rather than a gamble. ⚖️ **Two fixes, one playtest, and the only one that could fail to apply is the one that announces itself in the log.**

⚠️ **AND THE RISKIEST ASSUMPTION, NAMED RATHER THAN BURIED: whether the `bDoFullyAsyncNavDataGathering` ini flip reaches the serialized `L_Arena` `ARecastNavMesh` instance at all.** Config `UPROPERTY`s on level actors take the **CDO** value **unless a differing value was serialized**, and the instance matched the CDO at save time — **but the `.umap` is binary and this could NOT be proven statically.** ⭐ **This is the exact trap `DefaultEngine.ini:281-284` already documents in its own comment.** ✅ **Stage 0's telemetry settles it on the first run** — which is the entire reason Stage 0 exists.

### NAV-§10. THE QA GATE — ⛔ `.claude/pipeline/qa/TASK-537.md`, AND IT NAMES 529 · 530 · 531 · 532 · 533 · 534 · 535 · 536

⚠️ **The gate file is named for the GATE task** (`qa/TASK-506.md` / `qa/TASK-525.md` precedent, `SC-§29`). ⛔ **A gate covers the tasks it NAMES — reviewing a FILE is not gating a TASK.** The batch's **coverage ledger** lives on the board and **build-master REFUSES TO COMMIT until every code task in the batch names a gate.**

**Criteria (the numbered list on the gate task is authoritative; these are the ones that come from the LAW):**
1. ⛔ **The anti-mill brakes are STRUCTURAL** — cooldown + monotonic level — and the worst case **at the shipped defaults** is **≤1 extra path request per unit per second, for non-moving units only.** *(NAV-§3.)* ⛔ **CORRECTED 2026-08-04: brake 1 bounds the RATE, brake 2 bounds the COUNT PER STALL; the ceiling rests on `EscalationCooldown ≥ 1.0` ALONE and is therefore a TUNING INVARIANT.**
2. ⛔ **No second driver:** zero new timers **in the watchdog lane** · `OnMoveCompleted` issues no move · `AMinerUnit` overrides only `HandleStuckEscalation`. *(NAV-§3.)* ✅ **One ruled-in exception: `DefinitiveCheckTimerHandle`.**
3. ⛔ **The sidestep lease is cleared in `EnterIdle`, `EnterAttack`, death, `FreezeAI` AND `ApplyFreeze` — FIVE sites** — trace each one at the artifact. *(NAV-§3.)*
4. ⛔ **`DeltaSeconds` never comes from `StateCheckInterval`**, and `DeltaSeconds == 0` is inert. *(NAV-§3.)*
5. ⛔ **No `CONFIRMED` on a pre-settle query; `bCullOnProvisionalFailure` defaults `false`; the delegate is bound once and unbound on `EndPlay`.** *(NAV-§4.)*
6. ⛔ **No `.umap` write, no `Build.cs` change, no `TilePoolSize` change, no `AgentRadius` change, no crowd/RVO component.** *(NAV-§2, `NAV-§5`, `NAV-§6`, `NAV-§8`.)*
7. ⛔ **The telemetry is READ-ONLY** — no state, no ticking, no behaviour change; every engine accessor null-guarded. *(NAV-§9 clause 1.)*
8. **Pinned-registry conformance, character-for-character against `NAV-§8`**, including access levels and the `OnMoveCompleted` overload trap.
9. ⛔ **The M8 declaration is present VERBATIM** in every code task's handoff and in both new headers. *("Tier not declared" is a QA FAIL.)*
10. **Standing C++ sweep:** complete-type include law · most-vexing-parse · no shadowing of inherited reflected members (C4457/C4458) · `TestEqualSensitive` on every `FString` claim (`SC-§13`).
11. ⭐ **ADDED 2026-08-04 AFTER THE LOOP-1 BLOCKER — ⛔ THE CRITERION THAT WOULD HAVE CAUGHT IT: EVERY VALUE FED TO A RUNG'S ACTION MUST BE TRACED TO WHERE IT IS *PRODUCED*, NOT MERELY TO WHERE IT IS CONSUMED.** *(NAV-§13.)* **Specifically for this feature: `ComputeSidestepGoal`'s `Attempt` MUST vary across a unit's successive stalls** — ⛔ **a parity contract that is correct in the library and constant at the call site is a NO-OP, and the library's own test will still be green.** ⚠️ **Ask of every such value: what is its LIFETIME, and does the thing that resets it also need to reset THIS?**

### NAV-§11. M8 DECLARATION — STATED, BECAUSE *"THERE IS NOTHING TO DECLARE"* ONLY COUNTS WHEN IT IS STATED

⛔ **THIS FEATURE ADDS NO REPLICATED PROPERTY, NO NEW REPLICATED CLASS, AND NO NEW RELEVANCY TIER.**

- ✅ **AND THE REASON IS STRUCTURAL: in M8 P1 units are SERVER-ONLY, and every symbol this batch adds lives on the server-side unit, its controller, or a static library.** `FSiegeStuckState` is deliberately **NOT** a `USTRUCT` (`NAV-§8`) — **an unreflected struct cannot be replicated by accident**, which is the property that makes this declaration hold under a later refactor rather than merely today.
- ⚠️ **`FSiegeStuckTuning` IS reflected (EditDefaultsOnly, so Jonathan can tune it) — it is CONFIG, set at design time on the CDO, and it is identical on every machine by construction.** ⛔ It is not replicated and must not become so.

### NAV-§12. ⚠️ KNOWN LIMITATIONS + FLAGGED-UNVERIFIED — ⛔ NONE OF THEM ARE BUGS

Carried from the plan so a playtest report does not spend a QA loop on a designed outcome.

- ⚠️ **Whether 663 ms/tile is CPU-bound or latency-bound is UNKNOWN** — it decides whether 8× concurrency yields ~8× or less. **Either way it improves. ⛔ MEASURE, NEVER ASSERT.**
- ~~⚠️ **Which of `HasDirtyAreasQueued()` / `IsNavigationBuildInProgress()` returned false at the +5 s poll is UNKNOWN.** Stage 0 answers it.~~
  ### ✅ **ANSWERED 2026-08-04 — ⛔ NO LONGER AN OPEN QUESTION AND ⛔ NOT TO BE RE-ASKED. NEITHER OF THEM RETURNED FALSE: THE `MaxNavSettleWait` 10 s CAP FIRED AND THE CODE CONFIRMED ANYWAY.**
  **Verified two ways, both first-hand (`qa/TASK-537.md` RULING 3):** (a) at the engine source — `IsNavigationBuildInProgress() == false` ⟺ `GetNumRemainingBuildTasks() == 0`, the same three terms under an OR and under a `+`, so **neither predicate CAN read idle with 326 tiles pending** (full citations in the corrected `NAV-§1` cause 3); (b) in the log — **4 of 4** cap-then-`CONFIRMED` pairs at `Saved/Logs/GitClaudeUnrealTest-backup-2026.08.04-02.55.13.log:2583/2769/3040/3241`.
  ⚠️ **BUT ⛔ THE QUESTION IS NOT FULLY CLOSED, AND STAGE 0 STILL DECIDES THE OTHER HALF — see the corrected `NAV-§1` cause 3 in full.** The 4/4 cap evidence is from the **02:38** session. **The `+5.03 s` datum this feature was built on is from a DIFFERENT log (`Saved/Logs/GitClaudeUnrealTest.log:1891`/`:1977`) that contains ZERO cap lines — so in THAT session the wait exited EARLY and the route is still open** between: nav genuinely settled · the `IsNavigationBuildingPermanentlyLocked()` short-circuit · an absent nav system.
  ⇒ **TASK-538's `[at-confirmation]` `remaining=` / `dirtyAreas=` / `hasDirty=` distinguish all three on the first run.** ⛔ **What is CLOSED and not re-openable: the "false idle from a disagreeing predicate" theory** — the two predicates are provably the same three terms. ⛔ **What is OPEN: which of the three routes produced the early exit.** ⚖️ **Stating exactly which half is settled is the whole point of this bullet — "answered" and "partly answered" are different claims and the batch's honesty depends on not merging them.**
- ⚠️ **RUNG 0's PROGRESS TEST IS AN `OR`, AND A UNIT SCRAPING A COLLIDER IS NEVER RESCUED** (`SiegeStuckStatics.cpp:40-43`). A unit oscillating/grinding at **≥ `MinSpeedSq` (50 uu/s) with ZERO net progress** re-anchors every poll and the ladder never starts — TASK-536 test 18 measures **0 rungs in 60 s at 51 uu/s**. ⛔ **SHIPS AS SPECIFIED** (TASK-531 spec (2) and the `bAdvancing` bullet ordered the `OR` character-for-character; removing the speed term is an ARCHITECTURE change and `NAV-§` does not re-litigate the architecture at a gate). ✅ **THE REMEDY IS A TUNABLE, NOT A RECOMPILE: lower `MinSpeedSq`** (`EditDefaultsOnly`). ⭐ **⚠️ IF JONATHAN REPORTS "units still wedge" AFTER THE SIDESTEP BLOCKER FIX LANDS, THIS IS THE FIRST THING TO CHECK** — it is on TASK-539's watch list explicitly.
- ⚠️ **A SIDESTEPPING UNIT IS BRIEFLY DEAF (≤ `SidestepLeaseSeconds`, 2 s).** The lease early-out (`SummonedUnit.cpp:1364-1367`) also suppresses target acquisition and the attack dispatch. **Deliberate and bounded; recorded so it is not "found" at the playtest as a bug.**
- ⚠️ **A WEDGED MINER'S TENURE END CAN LAG BY UP TO 2 s.** The miner's lease early-out returns **above** `ResolveMinerOrder()` (`MinerUnit.cpp:357-360`), so an order arriving during a live lease also delays **`LeaveMining()`** — and therefore `RemoveMinerIncome`. **Bounded, self-healing on the next poll, no latch or invariant broken (≈ ≤ 2 gold), and only ever for a miner that is DEMONSTRABLY WEDGED.** ⛔ **Ships. Recorded here because the TASK-533 handoff's own "what it delays" list omitted it** (`qa/TASK-537.md` WARN-7) — ⚖️ **an unlisted consequence gets re-derived as a defect by the next reader, which is the cost this bullet buys off.**
- ⚠️ **`bNavSettled` PROVES *THE QUEUE IS EMPTY NOW*, ⛔ NOT *THE NAVMESH IS FINAL*.** A later dirtying event can re-open the build after a `CONFIRMED`. ✅ **The design is self-protecting: a cull re-dirties nav ⇒ the next pass reads PROVISIONAL ⇒ that pass is cull-free** (`NAV-§4`). ⛔ **Recorded, NOT actioned — the alternative is polling, which the spec forbids.** On Jonathan's list via the late-cull flag.
- ⚠️ **THE BARE STRING `Traversability CONFIRMED` NO LONGER EXISTS IN THE LOG.** Every verdict now carries a suffix — `CONFIRMED (nav settled: 0 pending)` or `PROVISIONAL (…)`. **Intended and spec-mandated (`NAV-§4`'s honest-labelling clause), but ⚠️ ANY tool, doc, or habit grepping the bare string is now SILENTLY EMPTY** — silently, which is the dangerous half. ✅ **TASK-538's evidence list is already correct.** ⛔ **Grep `Traversability CONFIRMED (` or just `CONFIRMED (nav settled:`.**
- ⚠️ **Whether `TilePoolSize=1024` is currently overflowing is UNKNOWN** (`NAV-§6`).
- ⚠️ **Capsule radii (30/40/45/60) are Blueprint-side**, taken from `handoffs/TASK-062.md:25` / `TASK-010.md:21`. **The `NAV-§2(a)` conclusion is insensitive to them — anything ≤ 64 uu is covered identically.**
- ⚠️ **Stage 1 moves gather work ONTO the game thread by design.** A hitch at match start is the expected cost and is **exactly** what the **8 → 4** fallback exists for. ⛔ **It is Jonathan's lever at the playtest, not an agent's default.**
- ⚠️ **The ladder RESOLVES a stall; it does not PREVENT one.** A unit may still stand still for up to 1.5 s before the first rung. ⚖️ **That is the deliberate anti-mill price** — a shorter fuse buys re-paths for units that were merely slow.

### NAV-§13. ⭐ THE SIDESTEP-ALTERNATION BLOCKER — AND THE GENERAL LAW IT PRODUCES

> ### ⛔ **A VALUE THAT MUST VARY *ACROSS* RESETS CANNOT LIVE IN THE STRUCT THAT THE RESET CLEARS.**
>
> ⚖️ **That sentence is the law. Everything below is the incident that taught it — but the law is a SHAPE, not an incident, and it will recur in any feature that pairs a per-episode state struct with a "try something different next time" counter.**

**THE INCIDENT (`qa/TASK-537.md`'s ONE BLOCKER, verified link by link at the artifact):** the Sidestep rung's `Attempt` argument was a **per-unit CONSTANT**, so the side alternation `ComputeSidestepGoal`'s parity contract exists to provide **was unreachable from either shipped call site**. A unit sidestepped to **the same side, every stall, forever** — and for the units whose fixed side is the blocked one, the ladder **detected the stall and could never escape it**: the gate's own named failure mode.

**THE FOUR LINKS, EACH LOAD-BEARING:**
1. `Evaluate` assigns `State.EscalationLevel = DesiredLevel` **BEFORE** the switch (`SiegeStuckStatics.cpp:135`) and returns `Sidestep` only for `DesiredLevel == 1` ⇒ **`EscalationLevel` is ALWAYS exactly 1 when the Sidestep rung runs.**
2. Brake 2 is strictly monotonic (`:102-105`) ⇒ **Sidestep fires at most ONCE per stall.**
3. `FSiegeStuckState` carries **no attempt counter**, and `Reset` assigns a default-constructed instance (`:209`) on **every** `Abandon` and **every** re-anchor ⇒ **nothing survives a stall.**
4. ⇒ the producer `EscalationLevel + (GetUniqueID() % 2)` collapses to **`1 + (uid % 2)` ∈ {1, 2}, fixed for the lifetime of the unit.**

⭐ **NOTE WHAT MAKES THIS HARD TO SEE, BECAUSE THAT IS THE REUSABLE PART: every one of those four links is individually CORRECT and individually DESIRABLE.** Assigning the level before the switch is tidy; monotonic escalation is the anti-mill law itself; a clean `Reset` is what keeps stalls independent; the `uid` term genuinely de-correlates neighbours. **The defect exists only in the composition** — which is why no single-file review found it and why it is written down here rather than left as a fixed bug.

**THE FIX'S SHAPE IS THE POINT:** a free-running, **unreflected** `uint8 SidestepAttemptCount` on `ASummonedUnit`, **deliberately OUTSIDE `FSiegeStuckState`** — ⛔ **because `Reset` is what caused the bug.** It is read-and-incremented at the two rung call sites; `uint8` wraps cleanly and `& 1` in `ComputeSidestepGoal` is total over the whole range, wrap included.
- ✅ **The `+ (uid % 2)` term STAYS** — it de-correlates neighbouring units, which is a different job from alternating one unit's successive attempts. **Two terms, two jobs.**
- ⛔ **`FSiegeStuckState`, `FSiegeStuckTuning` and every `NAV-§8` pinned signature are UNTOUCHED by the fix.** A **non-pinned, non-reflected** member is this batch's accepted practice (TASK-534 added one, TASK-535 added six); **the pin is the CROSS-TASK LINK CONTRACT**, and nothing outside the two owning files references this member.
- ⚠️ **THE TEST THAT ASSERTED THE HOLE MUST BE INVERTED.** `SiegeStuckStaticsTest.cpp`'s `SidestepSideIsConstantForOneUnit` **asserted the bug as shipped** and must now assert alternation across consecutive stalls. ⭐ **If it is still green ASSERTING CONSTANCY at TASK-538, the fix did not land — stop and route back.** ⚖️ **A test can pin a defect in place; that is worth its own moment of suspicion whenever a test name states a behaviour rather than a requirement.**

**⇒ THE STANDING CHECK THIS ADDS TO EVERY FUTURE REVIEW, IN ONE QUESTION:**
> ### ⛔ **For each field in a per-episode state struct: does anything need this value to CHANGE from one episode to the next? If yes, it does not belong in this struct.**
⚠️ **And its mirror, which is the failure mode in the other direction: a free-running counter that SHOULD have been reset produces "the unit is still avoiding a rock it cleared ten minutes ago."** ⚖️ **Both are cheap to get right at authoring and expensive to find at a playtest — the deciding question is always the LIFETIME, never the tidiness.**

### NAV-§14. ⚖️ SETTLED AT THE TASK-537 GATE — ⛔ NOT RE-OPENABLE

⛔ **Each of these was raised, argued, and DECIDED against the artifact at the batch's QA gate. They are recorded here so a later reader does not spend a loop re-deriving a settled answer.** ⚠️ **A finding that re-opens one of them without NEW EVIDENCE is a finding against its author.** ⚖️ Re-opening on genuinely new evidence is always allowed — that is the difference between a settled question and a closed one.

**(a) ⭐ THE SIDESTEP LEASE IS *DISPATCH SUPPRESSION*, NOT PER-POLL RE-STEER — ACCEPTED, AND THE LAW WAS ALWAYS ON ITS SIDE.** `NAV-§3`'s rung table forbids the literal reading outright (*"⛔ re-issue while the lease is live"*). The cost check confirms it: `EnterAdvanceToLocation`'s gate includes `GetMoveStatus() == EPathFollowingStatus::Idle` (`SummonedUnit.cpp:2677`), so **a sidestep goal the navmesh will not take leaves the status `Idle` every poll and a per-poll re-issue would fire 4 requests/unit/s for the whole 2 s lease** — the mill, exactly. ✅ Ordering verified on both drivers: the rung arms the lease and issues **before** the same poll's dispatch could cancel it (`SummonedUnit.cpp:1348` then `:1364`; `MinerUnit.cpp:332` then `:357`).

**(b) ⭐ THE HIGHER RUNGS DROP THE LEASE BEFORE ACTING — ACCEPTED, AND IT IS A GENUINE SAVE, NOT A TIDINESS EDIT.** Armed at 1.5 s for `SidestepLeaseSeconds = 2.0` ⇒ expires at 3.5 s; **`WidenSeconds = 3.0` fires INSIDE the lease.** Without `SidestepLeaseRemaining = 0.f` first, the poll would return early, the re-path would never reach a mover, and — **because `EscalationLevel` is monotonic — the rung could NEVER fire again.** ⇒ **a silent no-op rung, caught by the author.** Same for `Abandon`. ⛔ **Any future rung added above Sidestep inherits this obligation.**

**(c) TASK-533's THREE MINER ADDITIONS — ALL THREE ACCEPTED, each verified against shipped code rather than against its argument.**
- **(i) the lease read in `UpdateMining`** — without it, `EnsureWalkingToNode` re-issues `MoveToActor` **in the very next statement of the same call stack** and the sidestep is cancelled before it exists. The rung would be vacuous.
- **(ii) `StopMovement()` in the miner's `WidenAndRepath`** — this is **the one driver cancelling its OWN request**, ⛔ not a second steering authority. `EnsureWalkingToNode`'s early-out never reads `bHasIssuedPointGoal`, so the dispatch's stated mechanism does not reach that gate.
- **(iii) `StandInPlace()` + a `!bArrivedAtNode`-GUARDED `SeekBestMine()` for `Abandon`** — `UpdateMining`'s retarget gate fires only on `bTargetDead`, so **a healthy-but-unreachable mine is never re-chosen on its own** and the board's *"`UpdateMining` re-targets by itself"* is false against shipped code. ⭐ **The guard is what keeps a stuck-ladder rung OUT of the income path** — without it a null finder result would drive `LeaveMining()` → `RemoveMinerIncome` from a rescue rung. ⛔ **The guard is law now, not a detail.**

**(d) RUNG 0's `OR` SPEED CLAUSE SHIPS AS SPECIFIED, WITH `MinSpeedSq` AS THE NAMED REMEDY** — full statement in `NAV-§12`, and it is an explicit **TASK-539 watch item**. ⛔ **The hole is real and is NOT a defect in the delivery: the `OR` is what the spec ordered, character-for-character.**

**(e) `NotifyMoveBlocked` SHIPS AS-IS, with its emphasis corrected to "corroborating evidence"** — see the `NAV-§3` bullet. ⛔ **Not dead code; ⛔ not a trigger.**

**(f) TASK-535's `DefinitiveCheckTimerHandle` IS RULED IN** — the declared, bounded exception to the zero-timers clause, which is scoped to the watchdog lane. See the `NAV-§3` bullet.

**(g) TASK-529's `const`-only diagnostics library IS ACCEPTED, and the reason is worth keeping:** `FNavigationSystem::GetCurrent<T>(const UWorld*)` returns `const TNavSys*` and every accessor the library calls is `const` ⇒ ⭐ **no `const_cast` in a library whose entire selling point is "provably read-only."** **The route is closed by the TYPE SYSTEM, not by discipline** — which is the only kind of closure that survives a future edit. ✅ Its `TActorIterator` fallback is justified: route 1 returns null on `RegistrationFailed_AgentNotValid`, a named risk of this very batch.

**(h) ⚠️ CITATION DRIFT IS EXPECTED AND IS NOT A FINDING.** The batch moved lines under existing citations (the miner seal is `MinerUnit.cpp:62`, not `:61`; the watchdog call site is `:1348`, not `:1285`; `EnterAdvanceToLocation`'s null is `:2675`, not `:2581`). ⛔ **GREP THE SYMBOL, NEVER THE LINE.** ⚖️ Line numbers in this file are navigation aids with a shelf life; **the symbol name is the citation.**

