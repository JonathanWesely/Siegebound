# QA Report — TASK-025
Verdict: PASS

Reviewed: `Source/GitClaudeUnrealTest/Siegebound/GoldNode.h/.cpp` and `MinerUnit.h/.cpp` against the TASKBOARD TASK-025 spec + `names:` block, handoffs/TASK-025.md, the frozen deps `SummonedUnit.h/.cpp` (base) and `SiegePlayerState.h` (miner API), handoffs/TASK-024.md (API contract), handoffs/TASK-028.md (FreezeAI extension rule), qa/TASK-021-report.md WARN-1 (no-attack-path rule), and CONVENTIONS.md. Pre-compile review only — NOT compiled (batch compile is TASK-039); this is the pre-compile safety gate.

Counts: **0 blockers, 1 warn, 1 nit.**

## Rulings on the three flagged items

**Flag 1 — the `StateCheckInterval=0` / `AggroRadius=0` no-attack seal — CORRECT AND LOAD-BEARING (PASS).**
Verified against the frozen base:
- Seal #1: `LoadStatsAndStart` (SummonedUnit.cpp:255) is the ONLY `SetTimer` on `StateTimerHandle`; `FTimerManager::SetTimer` with rate <= 0 clears the handle instead of scheduling, so the recurring `UpdateState` driver never runs. Confirmed the bStatsLoaded latch means that arming call can only run once.
- Seal #2: `LoadStatsAndStart` DOES call `UpdateState()` once synchronously (SummonedUnit.cpp:254) before the timer set. In that one call, `AcquireTarget` (line 341) rejects every candidate at `Distance > AggroRadius`; distance is closest-point-on-collision (always >= 0), so with `AggroRadius = 0` nothing is ever acquired, `CurrentTarget` stays null, and the `EnterAttack` gate (line 297, `if (CurrentTarget && ...)`) is never taken. The attack timer (which would otherwise loop at the 0.05 s clamped cadence) is never armed. Seal #2 alone is sufficient to prevent the single synchronous `UpdateState` from ever attacking; Seal #1 additionally kills the recurring driver.
- Seal #3: `ClearAllTimersForObject(this)` (MinerUnit.cpp:76) runs after `Super::BeginPlay` and BEFORE the miner arms its own poll (line 110) — a valid backstop.
The one synchronous `UpdateState` issues an `EnterAdvance` toward the enemy castle; `EnsureWalkingToNode` re-paths to the gold node in the same call stack (see WARN-1 for the one degenerate case where it does not). The three seals are independent and all verified. No code path reaches `EnterAttack`. The reading is accurate.

**Flag 2 — constructor writes `StateCheckInterval = 0.f` below the base property's editor `ClampMin = "0.05"` — ACCEPTABLE (PASS).**
`ClampMin` is a UPROPERTY editor-UI/import constraint; it does NOT constrain C++ member writes to the CDO. Setting `StateCheckInterval = 0.f` (and `AggroRadius = 0.f`) in the constructor is a plain, legal out-of-band CDO value. Intended and correct. Dependency constraint (documented, not a defect): TASK-034's `BP_Unit_Miner` must not set `StateCheckInterval` OR `AggroRadius` in the editor (a serialized value would be re-clamped to >= 0.05 and could re-arm the state timer). Its spec ("nothing stat-like on the BP") already forbids this, and even if `StateCheckInterval` were re-legalized, `AggroRadius = 0` (equally a CDO seal TASK-034 won't touch) keeps acquisition dead, so no attack occurs regardless. Defense-in-depth holds.

**Flag 3 — one-time `Profile != Standard` spawn log from the frozen base — NON-ISSUE (PASS).**
`LoadStatsAndStart` (SummonedUnit.cpp:244-249) warns once per unit when `Row->Profile != Standard`; the Miner row is Profile None (per qa/TASK-021), so every miner spawn logs one line. This is frozen-base behavior, single-line per spawn (gated by `bStatsLoaded`), not spam, no functional impact. The "running Standard behavior" text is cosmetically inaccurate for a miner (the seals disable that behavior) — see NIT-1 — but nothing this task can or should change (base is frozen qa-passed).

## Findings

- [WARN] MinerUnit.cpp:99-104 (BeginPlay, no-node fallback) — when `FindNearestSameTeamGoldNode` returns null, BeginPlay logs an error and returns WITHOUT cancelling the residual castle-bound `MoveToActor` that the one synchronous `UpdateState` issued during `Super::BeginPlay`. `UpdateMining` then early-outs on the null node (line 192-205) and never calls `EnsureWalkingToNode`, so the miner "idles" by walking to and standing at the ENEMY castle wall rather than standing still as the spec/handoff say ("log and idle"). It is acquisition-dead (seals hold — it never attacks) and harmless, and it is UNREACHABLE in a TASK-036-assembled arena (nodes present → the walk is always healed to the node). It only manifests in a half-assembled level (nodes not yet placed) where stats are already bound and an enemy castle exists. Non-blocking, edge-only, no engine risk. Optional fix: in the no-node branch, `StopMovement()` on the AI controller after the error log so the miner truly stands. Suggest accepting as-is or a one-line stop.

- [NIT] SummonedUnit.cpp:246-248 (inherited) — the base's `Profile != Standard` warning prints "running Standard behavior" for every miner spawn, which is misleading (the miner runs no Standard behavior — the seals disable the state machine). Frozen base, cosmetic, no action for this task; noted so it is not mistaken for a miner defect in TASK-039/040 PIE logs.

## Standard-check results (all clear)

- **Deprecated/removed UE 5.8 APIs:** none. Every API verified valid — `AAIController::MoveToActor(7-arg)/GetPathFollowingComponent/GetMoveStatus/StopMovement`, `UPathFollowingComponent::GetMoveGoal()` (confirmed present in the UE 5.8 engine header, returns `AActor*` — the `== Node` comparison is a valid pointer compare), `EPathFollowingStatus::Idle` / `EPathFollowingRequestResult::Failed` (already used by the frozen base), `TActorIterator`, `GetWorldTimerManager().SetTimer/ClearTimer/ClearAllTimersForObject`, `FVector::Dist2D`, `SetCanBeDamaged`, `UCollisionProfile::NoCollision_ProfileName`, `SetCollisionProfileName/SetCollisionEnabled/SetGenerateOverlapEvents/SetCanEverAffectNavigation`, `TSoftObjectPtr::LoadSynchronous/IsNull/ToString`, `GetStaticMesh/SetStaticMesh`, `GetGameState()/PlayerArray`, `TWeakObjectPtr::Get/IsExplicitlyNull`.
- **Soft-ref null-safety:** `AGoldNode::ResolveNodeMesh` is fully null-safe — early-out on `!NodeMesh || NodeMeshAsset.IsNull()`, `LoadSynchronous` with a load-failure branch + warn-once. The SM_GoldNode that arrives in TASK-038 cannot crash a node saved/placed before it exists (invisible-but-functional). Movable mobility retained so the deferred runtime `SetStaticMesh` is legal post-BeginPlay. Correct.
- **Arrival/income/death lifecycle:** verified character-for-character against handoffs/TASK-024.md — `RegisterMinerAlive` on BeginPlay exactly once (latched `bRegisteredAlive`, retried on the poll); `AddMinerIncome` exactly once on arrival (latched `bArrivedAtNode`, guarded `bRegisteredAlive && Owner`, sets `bIncomeActive`); `RemoveMinerIncome` at `EndPlay(Destroyed)` ONLY if `bIncomeActive`; `UnregisterMinerAlive` at `EndPlay(Destroyed)` ALWAYS if `bRegisteredAlive`; Remove-before-Unregister preserves the `MinerIncomeCount ⊆ AliveMinerCount` invariant. Killed-en-route (arrived=false) → Unregister fires, Remove does not → count drops, rate unchanged (§3.3). Arrival is a one-way latch (displacement never re-triggers income). Walk acceptance `0.8 × ArrivalRadius` (120) sits inside the 150 arrival ring, with an explicit `StopMovement` on arrival to cover the poll firing before path-following finishes. PlayAgain ordering (units destroyed at step 2 → EndPlay decrements — before ResetEconomy zeroes at step 4) avoids underflow; even reversed it hits the base's refuse-and-log guard, non-crashing.
- **FreezeAI:** base `Super::FreezeAI()` does `StopMovement` (halts the gold-node walk) per the TASK-028 contract; the miner extends it with `ClearTimer(MiningPollTimerHandle)` so nothing re-issues `MoveToActor` under the freeze. Plain C++ `override`, no UFUNCTION re-declaration (correct — base declares it UFUNCTION virtual). `UpdateMining` also gates on `IsUnitDead()/IsAIFrozen()`. Idempotent; base bDead early-out does not strand the subclass clear.
- **UE reflection / GC-safety:** `NodeMesh` is `TObjectPtr` UPROPERTY (GC-safe). `CachedOwnerState`/`TargetGoldNode` are `TWeakObjectPtr` (auto-null, always `.Get()`-checked — no dangling), correctly not requiring UPROPERTY. `GetTeam`/`HasArrivedAtNode` BlueprintPure; `ArrivalRadius`/`ArrivalCheckInterval` EditAnywhere UPROPERTY; FreezeAI/BeginPlay/EndPlay/OnConstruction virtual overrides. Header/cpp declarations and definitions match 1:1; includes cover every referenced symbol.
- **Performance:** no per-tick work (GoldNode tick disabled; MinerUnit uses a 0.25 s poll, never per-tick, per TASK-004 law). Actor iterations (`FindNearestSameTeamGoldNode` once at BeginPlay; `ResolveOwningPlayerState` over PlayerArray) are cheap and off the hot path. Soft-mesh `LoadSynchronous` only at OnConstruction/BeginPlay. No FindObject/LoadObject in any hot path.
- **CONVENTIONS / names block:** `AGoldNode` and `AMinerUnit` at the exact paths; `UPROPERTY ArrivalRadius = 150.f`; soft mesh `/Game/Meshes/SM_GoldNode.SM_GoldNode`; `CardID = "Miner"`; PlayerState API names exact. `NodeMesh` component name is correct — the `VisualMesh` component-name convention binds card actors (ASummonedUnit/ABuilding), not this location-marker AActor; AMinerUnit inherits the base `VisualMesh` and does not create a second mesh. `GetTeam()` is deliberately NOT `ITeamAgent::GetTeamId` (so enemy acquisition never targets the node) — correct per spec.
- **Stats never hardcoded:** `CardID="Miner"` is identity, not a stat; 30 HP / 350 speed bind via `Super::BeginPlay → LoadStatsAndStart` from DT_Cards. `ArrivalRadius`/`ArrivalCheckInterval` are behavior tunables (GDD §3.3 walk budget / poll cadence) with `// GDD §3.3` comments, not card stats. No hardcoded card stat anywhere.

## Notes for build-master (PASS)

- Two NEW class pairs, normal UBT pickup — no Build.cs / .uproject changes (AIModule already a PublicDependency; `MoveToActor`/`UPathFollowingComponent`/`TActorIterator`/`PlayerArray` are Engine/AIModule-level). Includes consume only Engine/AIModule headers plus frozen qa-passed Siegebound headers. Batch-compiles cleanly at TASK-039 alongside TASK-024/028 (no cyclic includes).
- Runtime deps for TASK-040 PIE: `GoldNode_Blue`/`GoldNode_Red` placed in L_Arena (TASK-036), `BP_Unit_Miner` reparented to `AMinerUnit` (TASK-034), NavMeshBoundsVolume covering the arena, DT_Cards reimported (TASK-031) so the Miner row binds 30 HP / 350 speed.
- Expected benign PIE log: the inherited "profile None … running Standard behavior" warning once per miner spawn (NIT-1) — cosmetic, not a defect.
- Documented M2 limitation (not a finding): a Red miner has no resolvable owner state (ASiegePlayerState carries no team field, frozen TASK-024 surface) — it walks/stands but registers/incomes nothing (warned once). All M2 miners are Blue; Red-miner tracking is an M3 concern.

---

## BUILD-MASTER — COMPILE FAILURE (TASK-039 batch, attempt #2, 2026-07-04)

**Verdict: qa-failed.** The M2 C++ batch (TASK-021..030) was recompiled via the standard Build.bat (`GitClaudeUnrealTestEditor Win64 Development -waitmutex`) after the TASK-029 UHT fix (attempt #1). This time the build got **past UnrealHeaderTool and into the C++ compile stage**, where it failed with **four C4458 "declaration hides class member" errors in `MinerUnit.cpp`** (UE treats C4458 as a hard error via `ShadowVariableWarningLevel = Error`). Nothing was committed; HEAD stays `4f95730`; the editor was left down. This is TASK-025's first build-fix loop (per-task counter 1/3); routing back to gameplay-programmer.

**Exact compiler output (from the build log):**

```
[6/19] Compile [x64] MinerUnit.cpp
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\MinerUnit.cpp(125,26): error C4458: declaration of 'Owner' hides class member
		if (ASiegePlayerState* Owner = CachedOwnerState.Get())
		                       ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\GameFramework\Actor.h(839,21): note: see declaration of 'AActor::Owner'
	TObjectPtr<AActor> Owner;
	                   ^
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\MinerUnit.cpp(231,23): error C4458: declaration of 'Owner' hides class member
			ASiegePlayerState* Owner = CachedOwnerState.Get();
			                   ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\GameFramework\Actor.h(839,21): note: see declaration of 'AActor::Owner'
	TObjectPtr<AActor> Owner;
	                   ^
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\MinerUnit.cpp(313,21): error C4458: declaration of 'Owner' hides class member
	ASiegePlayerState* Owner = ResolveOwningPlayerState();
	                   ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\GameFramework\Actor.h(839,21): note: see declaration of 'AActor::Owner'
	TObjectPtr<AActor> Owner;
	                   ^
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\MinerUnit.cpp(350,22): error C4458: declaration of 'PlayerState' hides class member
		for (APlayerState* PlayerState : GameState->PlayerArray)
		                   ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\GameFramework\Pawn.h(179,27): note: see declaration of 'APawn::PlayerState'
	TObjectPtr<APlayerState> PlayerState;
	                         ^

Result: Failed (OtherCompilationError)
```

**Root cause (diagnosis for the programmer — build-master does not edit code):** `AMinerUnit` is a pawn (`AMinerUnit : ASummonedUnit → ACharacter → APawn → AActor`), so it inherits two reflected members that these locals shadow:
- `AActor::Owner` (`TObjectPtr<AActor>`) — shadowed by the three local `ASiegePlayerState* Owner` declarations at lines 125, 231, 313. Rename each (e.g. `OwnerState` / `OwningPS`).
- `APawn::PlayerState` (`TObjectPtr<APlayerState>`) — shadowed by the ranged-for loop variable `APlayerState* PlayerState` at line 350. Rename it (e.g. `CandidatePS`).

None of these are stat/CardID/DT values, so renaming the locals is purely mechanical and does not touch any of the seams or the miner-API call sites the QA verified character-for-character. Only local identifiers move.

**Batch status:** all 14 other translation units compiled clean; the ONLY two files with errors this attempt are `MinerUnit.cpp` (this task, 4 errors) and `CardHandWidget.cpp` (TASK-029, 2 errors — see qa/TASK-029-report.md). The compiler processed every TU before failing, so this is the complete compile-stage error set. Link did not run (compile failed), so link-stage errors, if any, remain unverified until both files compile clean and TASK-039 is re-dispatched (attempt #3).
