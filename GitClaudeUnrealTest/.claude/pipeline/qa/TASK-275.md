# QA Report — TASK-275
Verdict: PASS (WARN-1 RESOLVED via delta review 2026-07-23)

Scope reviewed: `SummonedUnit.{h,cpp}` + `Castle.{h,cpp}` (the 4 edited files), against the board spec
(TASKBOARD "#### TASK-275"), CONVENTIONS "Unit commands (Shield Wall stances)…", the programmer handoff
`handoffs/TASK-275.md`, and the API contract `handoffs/TASK-274.md`. Pre-compile review only (no build/Git).

## Delta review (2026-07-23) — HOLD kite-out WARN fix VERIFIED, WARN RESOLVED
The programmer applied the prescribed one-line fix in `EnterAdvanceToLocation` (SummonedUnit.cpp:1878-1881).
Scoped delta review confirms all five acceptance points:
1. `const bool bWasActorMove = (CurrentMoveGoal != nullptr);` (l.1878) is captured BEFORE `CurrentMoveGoal =
   nullptr;` (l.1879) — reads the correct pre-clear value.
2. It is OR'd into the re-path guard (l.1881: `if (bWasActorMove || bPointChanged || GetMoveStatus()==Idle)`),
   so a return-to-point immediately after an actor-move always re-issues `MoveToLocation`.
3. `EnterAdvance(AActor*)` (1790-1841) is byte-for-byte untouched — the change is contained entirely within
   `EnterAdvanceToLocation`.
4. No new shadow/include/null-safety issue: `bWasActorMove` is a fresh `const bool` local (shadows no inherited
   reflected member and no other local), a plain pointer-null comparison on an existing member — no new include.
5. Genuinely closes the WARN: when the last move was an actor move, even with an unchanged point and non-Idle
   status, the guard now re-issues `MoveToLocation(Point)`, dropping the stale `MoveToActor(enemy)` — the unit
   can no longer be kited out of the hold position. No steady-state regression: on a pure point-advance sequence
   `CurrentMoveGoal` stays null → `bWasActorMove` false → re-path governed by `bPointChanged`/Idle as before (no
   new per-tick churn).

## #1 scrutiny — legacy-protection gate (the load-bearing check)
VERIFIED SAFE. The gate is inserted at `SummonedUnit.cpp:1049-1062`, entirely ABOVE the legacy body's first
statement `const FVector MyLocation = GetActorLocation();` (line 1064). The legacy Standard body is lines
1064-1102 and is purely the M1/M2 leash/acquire/goal/attack-or-advance body — no command state referenced,
byte-identical by construction (the only change to the function is the additive gate block).

Gate = `Profile == ECardProfile::Standard && Team == ETeamId::Blue` → `if (GetWorld())` →
`if (Cast<ASiegePlayerController>(World->GetFirstPlayerController()))` → `if (PC->HasIssuedCommand())` →
`UpdateStateStandardCommanded(*PC); return;`. Every failure mode falls through to the unchanged legacy body:
- no world → inner `if` skipped;
- `GetFirstPlayerController()` null → `Cast<>(null)` = null → skipped;
- wrong PC class → `Cast<>` = null → skipped;
- `HasIssuedCommand()` false (pre-first-command) → skipped;
- `Profile != Standard` (miners = `None`, plus `Siege`/`Support`) → outer `if` false;
- `Team != Blue` (all bot/Red units) → outer `if` false.
The `PC &&` requirement is satisfied implicitly by the `if (const ... PC = Cast<>(...))` init-guard.

- **Miners (Profile None):** excluded by the `Profile == Standard` clause → run the legacy body. Confirmed load-bearing.
- **Siege / Support:** dispatched and `return` at lines 1026-1035, above the gate — they never reach it.
  `UpdateStateSiege` / `UpdateStateSupport` and the `None` path are untouched.
- **Freeze/match-end precedence:** the `bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen` early-out is
  `SummonedUnit.cpp:1010`, UPSTREAM of the gate (1049) — a frozen/commanded unit still freezes. Confirmed.

## MoveToLocation signature (programmer's explicit ask)
CORRECT for UE 5.8. `EnterAdvanceToLocation` (`SummonedUnit.cpp:1883-1885`) calls:
`AI->MoveToLocation(Point, StructureMoveAcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true,
/*bProjectDestinationToNavigation=*/true, /*bCanStrafe=*/true, /*FilterClass=*/nullptr, /*bAllowPartialPath=*/true)`.
This matches the UE 5.8 8-arg overload
`EPathFollowingRequestResult::Type AAIController::MoveToLocation(const FVector& Dest, float AcceptanceRadius,
bool bStopOnOverlap, bool bUsePathfinding, bool bProjectDestinationToNavigation, bool bCanStrafe,
TSubclassOf<UNavigationQueryFilter> FilterClass, bool bAllowPartialPath)` — arg order, types, and return type
all correct. `nullptr` → `TSubclassOf<UNavigationQueryFilter>` is valid (implicit null ctor; the existing
`MoveToActor` call at line 1830 already passes `nullptr` the same way). Includes complete: `AIController.h`
(l.5) + `Navigation/PathFollowingComponent.h` (l.24). Will compile at TASK-277.
`EnterAdvance(AActor*)` (1790-1841) is unchanged (standard actor-move body, no command logic); only the NEW
`EnterAdvanceToLocation(FVector)` (1843-1897) was added.

## Branch behavior
- **ATTACK (1279-1322):** leash/reacquire + `AcquireTarget()` + in-aggro attack/advance mirror the legacy body
  exactly; only the no-aggro goal changes to `FindNearestEnemyInSpawnBox(Cast<ACastle>(FindNearestEnemyCastle()))
  ?? EnemyCastleActor`, `!Goal ⇒ EnterIdle`. `FindNearestEnemyInSpawnBox` excludes the castle itself
  (`Candidate == EnemyCastle` skip, 1424) and returns null on a null castle (1403). Correct.
- **HOLD (1213-1244):** `AcquireEnemyNearPoint(GetHoldLocation(), GetHoldRadius())` disc-only, re-picked each
  tick (drops a target that leaves the disc → CurrentTarget null); goal = target else march to point via
  `EnterAdvanceToLocation`, idle on 2D arrival ≤ `HoldArrivalTolerance` (150, file-local const, 1234). Correct.
  Movement-layer drop of a fled target now correct after the delta fix.
- **DEFEND (1246-1277):** `AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), DefendRadius=2500)`; goal =
  target else `EnterAdvance(OwnCastle)`. `FindOwnCastle()` (1453) is same-team (`GetTeamId() != Team` skip),
  destroyed-skip, null-safe → `EnterIdle` on null. `EnterAttack` fires ONLY on an enemy `CurrentTarget`, so the
  own castle is never attacked. Correct.

## Helpers + Castle
- `AcquireEnemyNearPoint` (1326-1399) is a faithful `AcquireTarget` clone: same alive/enemy-team/self filters,
  pawn-vs-other bucketing, nearest-to-self selection, and `TieBreakDistance` tie-break — the ONLY change is the
  eligibility gate (`DistSquared2D(Candidate->GetActorLocation(), Center) <= Radius²`, 1363) replacing the
  AggroRadius-from-self gate. `AcquireTarget` itself (1104-1175) is untouched. Null world → nullptr.
- `ACastle::IsPointInSpawnBox` (Castle.cpp:364-371) + `SpawnBoxHalfExtent = (840,840)` (Castle.h:167,
  EditDefaultsOnly, ClampMin 0, BlueprintPure) is additive and null-safe (no pointer deref). The bot's
  `ASiegeBotController::IsPointInBotSpawnBox` + its own `SpawnBoxHalfExtent = (840,840)` are UNTOUCHED
  (SiegeBotController.h:361,518 — outside the 4-file edit set). 3-way (840,840) duplication is documented
  in-header and flagged out-of-scope; noted, not blocking.

## Usual filter
- UE 5.8 APIs: `GetFirstPlayerController()` (const UWorld), `MoveToLocation` (above), `TActorIterator<ACastle>`,
  `UGameplayStatics::GetAllActorsWithInterface`, `FVector::DistSquared2D`, `FMath::Square` — all valid.
- Includes complete: `SiegePlayerController.h` added (l.38, complete type for the const getters + transitively
  `ESiegeUnitCommand`); `Castle.h` (l.29), `AIController.h`, `EngineUtils.h`, `PathFollowingComponent.h` present.
- Null-safety: every new lookup guarded (GetWorld, PC cast, FindOwnCastle null, FindNearestEnemyInSpawnBox
  null-castle/null-world, AcquireEnemyNearPoint null-world). All degrade to EnterIdle or the legacy body.
- Shadow scan: new locals/params (incl. delta `bWasActorMove`) — NONE shadow an inherited reflected member
  (Owner/Instigator/Controller/PlayerState).
- New members `DefendRadius` (EditDefaultsOnly/ClampMin 0), `CurrentMoveGoalLocation`, `bHasMoveGoalLocation`
  are POD (no GC pointer) — no UPROPERTY needed; consistent with `CurrentMoveGoal` handling.
- const-correctness OK; no new per-frame Tick cost (stance re-eval inside the existing timer-driven UpdateState).

## Findings
- [RESOLVED] SummonedUnit.cpp:1878-1881 (`EnterAdvanceToLocation`) — the actor-move → same-point re-path
  staleness (HOLD kite-out-of-position) raised in the first pass is CLOSED by the `bWasActorMove` guard.
  Delta-verified 2026-07-23 (see "Delta review" above). No blockers, no open WARNs.
- [NIT] Castle.h:158-167 / paired-tunable — the (840,840) value now lives in 3 places
  (ACastle ≡ ASiegePlayerController ≡ ASiegeBotController). Documented + flagged out-of-scope in-header; no action
  this task, but the future delegation flag should not be lost.
- [NIT] SummonedUnit.cpp DEFEND fallback — once a unit is idling flush at its own castle with no attacker,
  `EnterAdvance(OwnCastle)` re-issues `MoveToActor` every tick (goal unchanged but path status Idle). Harmless
  churn (unit stays put), and identical to the pre-existing legacy castle-advance behavior — intentionally left
  as-is; informational only.

## Notes for build-master
- Compiles cleanly expected: `MoveToLocation` 8-arg overload confirmed against UE 5.8; all complete-type includes
  present; no shadow of inherited reflected members; the delta adds one `const bool` local + one guard clause.
- WARN-1 is RESOLVED — nothing outstanding gates the compile/integration.
- Scope confirmed: only the 4 declared files changed for this task; bot spawn-box helper untouched; legacy
  Standard body, Siege/Support/None dispatch, and freeze gating all intact.
- TASK-275 is CLEAR for build-master (TASK-277).
