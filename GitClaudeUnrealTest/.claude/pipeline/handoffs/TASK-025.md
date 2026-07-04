# TASK-025 Handoff — Miner unit + gold node (C++)

- author: gameplay-programmer
- date: 2026-07-04
- status: implementation complete, files-only (no compile, no editor/MCP, no Git, no TASKBOARD edit — orchestrator owns the status flip to ready-for-qa this session; M2 wave 5)
- REDISPATCH NOTE: this task was audited-first per the resume instructions. A prior in-flight agent died at shutdown with **no handoff**. **All four target files already existed on disk and are complete + correct** — the prior agent finished the code but never wrote the handoff or posted Slack. I did NOT blindly overwrite; I verified every symbol and acceptance criterion against the live (frozen qa-passed) dependencies and left the code as-is. Reconciliation detail below.

## Files (all four already present on disk — verified, unchanged by me)

1. `Source/GitClaudeUnrealTest/Siegebound/GoldNode.h` (NEW) — `AGoldNode : AActor`
2. `Source/GitClaudeUnrealTest/Siegebound/GoldNode.cpp` (NEW)
3. `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h` (NEW) — `AMinerUnit : ASummonedUnit`
4. `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp` (NEW)

**NOT touched (deliberate):** every other Siegebound file. This task is two NEW class pairs only — no base-class edit, no Build.cs edit (AIModule already a PublicDependency; `MoveToActor`/`UPathFollowingComponent`/`TActorIterator`/`AGameStateBase::PlayerArray` are all Engine/AIModule-level), no config/ini/board/conventions edits. `SummonedUnit.*`, `SiegePlayerState.*` consumed include-and-call only (both frozen qa-passed).

## Reconciliation (audit-first, per resume instructions)

I read all four partial files, then verified against: `SummonedUnit.h/.cpp` (base), `SiegePlayerState.h` (the miner API), `handoffs/TASK-024.md` (the API contract), `qa/TASK-021-report.md` WARN-1 (no-attack-path rule), CONVENTIONS.md, and Build.cs. Result: **complete and correct — no code changes required.** Nothing was partial or stubbed; every branch is implemented and every referenced symbol exists.

## PlayerState API symbols called (exact — TASK-024 contract, character-for-character)

- `RegisterMinerAlive()` — MinerUnit.cpp `TryRegisterWithOwnerState()`, exactly once (latched by `bRegisteredAlive`), from BeginPlay + retried on the poll if the player state isn't in PlayerArray yet.
- `AddMinerIncome()` — `UpdateMining()`, exactly once on arrival (latched by `bArrivedAtNode`; income guarded by `bRegisteredAlive && Owner`).
- `RemoveMinerIncome()` — `EndPlay(Destroyed)`, ONLY if `bIncomeActive` (had arrived). Called BEFORE Unregister so `MinerIncomeCount ⊆ AliveMinerCount` holds at every step.
- `UnregisterMinerAlive()` — `EndPlay(Destroyed)`, ALWAYS if `bRegisteredAlive` (arrived or not).
- (`CanAddMiner()` deliberately NOT called here — the §3.3 cap is enforced at play time by TASK-030 before gold moves, per the M2 ruling.)

## How each acceptance criterion is met

- **AGoldNode**: `AActor`; `EditAnywhere Team` (ETeamId); `UStaticMeshComponent` root; soft mesh `/Game/Meshes/SM_GoldNode.SM_GoldNode` resolved null-safe in OnConstruction (silent, editor) + BeginPlay (warn-once); `SetCanBeDamaged(false)`; NO `ITeamAgent` (so enemy acquisition never targets it) — team read via `GetTeam()`; NoCollision profile + no overlaps + `SetCanEverAffectNavigation(false)` so it blocks nothing and does not carve the navmesh it is the walk destination of. Component stays Movable so the deferred runtime `SetStaticMesh` is legal post-BeginPlay.
- **AMinerUnit stats from DT_Cards**: `CardID = "Miner"` in ctor (identity only); `Super::BeginPlay()` binds 30 HP / 350 speed from the DT_Cards Miner row — nothing stat-like hardcoded.
- **No-attack-path (qa/TASK-021 WARN-1, BINDING)** — sealed structurally through protected base DATA (base file frozen): (1) `StateCheckInterval = 0` → the ONE `SetTimer` in `LoadStatsAndStart` clears instead of arms (verified SummonedUnit.cpp:255), so `UpdateState` never loops; (2) `AggroRadius = 0` → `AcquireTarget` rejects every candidate at distance > 0 (verified SummonedUnit.cpp:341), so `CurrentTarget` stays null and `UpdateState` never enters Attack — even the single synchronous `UpdateState` inside `Super::BeginPlay` is acquisition-dead and at most issues a castle-bound Advance that `EnsureWalkingToNode` supersedes in the same call stack; (3) `ClearAllTimersForObject(this)` right after `Super::BeginPlay` (before the miner arms its own poll) sweeps anything a serialized value could have armed. The base clamps row Cadence to 0.05 s min, so these seals are load-bearing (a miner reaching EnterAttack would swing 20×/s) — not belt-and-braces.
- **Walk**: `MoveToActor` toward the nearest same-team `AGoldNode` (actor iteration, `FindNearestSameTeamGoldNode`); acceptance `0.8 × ArrivalRadius`; no node → Error log + idle (poll still runs so deferred registration completes).
- **Arrival**: `UPROPERTY ArrivalRadius = 150.f` (`// GDD §3.3 ~10 s walk`, ClampMin 0); 2D distance test (`Dist2D`) so the ~90 u capsule-center height doesn't eat the budget; explicit `StopMovement` on arrival (arrival ring 150 > walk acceptance 120); `AddMinerIncome` once.
- **Death bookkeeping** at the single choke point `EndPlay(EEndPlayReason::Destroyed)` — covers combat death (base `HandleDeath → Destroy`), PlayAgain sweep, KillZ fall; world teardown deliberately excluded.
- **Remains attackable**: the miner does NOT disable `SetCanBeDamaged`; it inherits the base `ITeamAgent` + `TakeDamage`, so enemies acquire and kill it (§3.3 raidable investment).
- **FreezeAI**: `virtual ... override` (plain C++, no UFUNCTION re-decl — handoffs/TASK-028 rule); `Super::FreezeAI()` does the base park + `StopMovement` (halts the walk), then the subclass clears `MiningPollTimerHandle` so nothing re-issues MoveToActor under the match-end freeze. Poll body also gates on `IsUnitDead()/IsAIFrozen()`.
- **Miner count delegate**: driven entirely by the PlayerState's Register/Unregister (which broadcast `OnMinerCountChanged`); a miner killed en route calls Unregister but not Remove → count drops, rate unchanged (exactly §3.3).

## Symbols beyond the names block (please rule)

- `AGoldNode::GetTeam()` (BlueprintPure) — ownership accessor; deliberately NOT `ITeamAgent::GetTeamId` (implementing the interface would make enemy units target the node). Members `NodeMesh`, `NodeMeshAsset` (soft), private `ResolveNodeMesh`, `bWarnedMissingMesh`.
- `AMinerUnit::HasArrivedAtNode()` (BlueprintPure) — PIE verification hook, the `IsAIFrozen/IsUnitDead` house pattern.
- `AMinerUnit` `UPROPERTY ArrivalCheckInterval = 0.25f` (ClampMin 0.05) — the miner's poll cadence, its replacement for the sealed combat state timer (never per-tick, TASK-004 law; clamped `>= 0.05` at arm time per the WARN-1 guard rule). Plus private helpers/one-shot warn guards documented inline.

## Things QA should scrutinize

1. **The no-attack-path seal is the crux.** Confirm my reading of the base: `StateCheckInterval = 0` disarms the state timer (SetTimer rate<=0 clears), and `AggroRadius = 0` kills acquisition, so no path reaches `EnterAttack` (which would arm the attack timer at the clamped 0.05 s). Seal #3 (`ClearAllTimersForObject`) is the backstop. All three are independent.
2. **Constructor writes `StateCheckInterval = 0.f` below the base property's editor `ClampMin = "0.05"`.** ClampMin constrains editor UI only, not C++ CDO writes — intended. TASK-034's `BP_Unit_Miner` MUST NOT touch this property (its spec: "nothing stat-like on the BP"); seals #2/#3 cover it even if a serialized value re-legalized the interval. Please confirm this is an acceptable out-of-band constructor value.
3. **Expected harmless log at every miner spawn:** the base `LoadStatsAndStart` warns once that the Miner row's `Profile` (None) != Standard ("running Standard behavior"). This is the frozen base's behavior, not a miner defect — the miner runs no Standard behavior because the seals disable it. Not something this task can/should change (base is frozen qa-passed).
4. **Red miners are untracked until M3.** `ASiegePlayerState` carries no team field (frozen TASK-024 surface) and the local player is always Blue, so a Red miner walks/stands but registers/incomes nothing (warned once). Documented for the M3 breakdown. All M2 miners are Blue.
5. **The one synchronous `UpdateState` in `Super::BeginPlay`** may issue a MoveToActor toward the enemy castle before the miner's own walk is issued. `EnsureWalkingToNode` checks the live `UPathFollowingComponent::GetMoveGoal()` (not the base's private `CurrentMoveGoal`) so it correctly re-paths to the node in the same call stack; subsequent polls see `bWalkingToNode` true and don't spam re-paths.

## Notes for build-master (TASK-039 batch compile)

- Two new class pairs, normal UBT pickup; no Build.cs / .uproject changes. Includes consume only Engine/AIModule headers + frozen qa-passed Siegebound headers.
- Runtime deps at PIE (TASK-036/040): `GoldNode_Blue`/`GoldNode_Red` placed in L_Arena at (-1200,0)/(+1200,0); `BP_Unit_Miner` reparented to `AMinerUnit` (TASK-034); NavMeshBoundsVolume covering the arena so `MoveToActor` succeeds; DT_Cards reimported (TASK-031) so the Miner row binds 30 HP/350 speed.
- No compile/Git run here per dispatch.

---

## Build-fix (loop 1) — C4458 shadow sweep 2026-07-04

Trigger: the M2 batch compile (TASK-039) attempt #2 got PAST UnrealHeaderTool this time and reached the C++ compiler, which failed with `error C4458: declaration of '<name>' hides class member` (UE treats C4458/C4457/C4459 as hard errors). Attempt #1's earlier sweep only covered UHT-visible UFUNCTION params and missed these `.cpp`-internal local/loop shadows. `AMinerUnit : ASummonedUnit : ACharacter : APawn : AActor`, so local identifiers named after inherited member variables (`AActor::Owner`, `APawn::PlayerState`) shadow them.

### Exact errors fixed in `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp`

| Site (orig line) | Scope | Old identifier → New | Uses updated in scope |
|---|---|---|---|
| `:125` | `EndPlay(Destroyed)` | `ASiegePlayerState* Owner` → `OwnerState` | `Owner->RemoveMinerIncome()`, `Owner->UnregisterMinerAlive()` |
| `:231` | `UpdateMining()` arrival block | `ASiegePlayerState* Owner` → `OwnerState` | `if (bRegisteredAlive && Owner)`, `Owner->AddMinerIncome()` |
| `:313` | `TryRegisterWithOwnerState()` | `ASiegePlayerState* Owner` → `OwnerState` | `if (!Owner)`, `CachedOwnerState = Owner`, `Owner->RegisterMinerAlive()` |
| `:350` | `ResolveOwningPlayerState()` loop | `for (APlayerState* PlayerState : ...)` → `IterPlayerState` | `Cast<ASiegePlayerState>(PlayerState)` → `Cast<...>(IterPlayerState)` |

Rename targets verified non-shadowing: neither `OwnerState` nor `IterPlayerState` is a member on the `AMinerUnit` base chain. All four are pure local-identifier renames — no UFUNCTION signature, seam, stat, DT_Cards value, or public API changed. The member `CachedOwnerState` (the actual field) and the TASK-024 PlayerState API calls are untouched; only the transient locals were renamed.

### Shared batch-wide shadow sweep (see TASK-029 handoff for the same result)

Grepped **every** `.cpp` and `.h` in `Source/GitClaudeUnrealTest/Siegebound/` for local vars / params / loop vars matching any reflected inherited member name (`Owner`, `PlayerState`, `Instigator`, `Controller`, `Slot`, `Role`, `RemoteRole`, `Tags`, `InputComponent`, `RootComponent`, `Children`, `Name`, `Outer`, `bReplicates`, `CustomTimeDilation`, plus UWidget/UUserWidget members `Visibility`, `Cursor`, `bIsEnabled`, `Clipping`, `ToolTipText/Widget`, `RenderOpacity/Transform`, `Padding`, `ColorAndOpacity`, `ForegroundColor`, `Priority`, `WidgetTree`, `bIsVariable`, `bIsFocusable`). **Genuine shadows found = exactly the 4 MinerUnit sites above + the 2 CardHandWidget sites (TASK-029).** Every other hit is a false positive:

- `UDeckComponent` `int32 Slot` params + `for (int32 Slot ...)` loop — parent `UActorComponent : UObject` has NO `Slot` member (`Slot` lives on `UWidget` only). Not a shadow. This is confirmed by the fact that attempt #2 flagged CardHandWidget's `Slot` but NOT DeckComponent's.
- `ASiegePlayerController` `int32 Slot` params (`PlayHandSlot`/`DiscardHandSlot`/`OnCardSlotKeyPressed`) — `APlayerController : AController : AActor` has no `Slot` member. Not a shadow.
- `UCardHandWidget::InitForController(ASiegePlayerController* Controller)` — `Controller` is NOT a member of `UUserWidget`/`UWidget` (it IS a member of `APawn`, but no pawn here has such a param). Not a shadow.
- `SummonedUnit.cpp` / `Tower.cpp` `SpawnParameters.Owner` / `.Instigator` — struct-field assignment on a local `FActorSpawnParameters`, not a member shadow.
- `SiegePlayerController.cpp` `/*Owner=*/` `/*Instigator=*/` — comment labels on `SpawnActor` args, not declarations. `Cast<UEnhancedInputComponent>(InputComponent)` — READ of the inherited `AActor::InputComponent` member (correct usage), local is `EnhancedInputComponent`.
- `Role`/`RemoteRole`/`Tags`/`RootComponent`/`Children`/`bReplicates`/`CustomTimeDilation` — **zero occurrences** anywhere in the module.
- `Name`/`Outer` — no local declarations; and both are stored as `NamePrivate`/`OuterPrivate` in modern UE, so a local named `Name`/`Outer` would not even C4458.
- `Priority`/`Visibility`/`Cursor` hits are comment/string text in non-widget classes (`AHeroCharacter`, `ASiegePlayerController`), not declarations.

Confirmation: after these fixes the module is shadow-clean; attempt #3 should have zero C4458s.

Files-only. Did NOT compile (build-master re-runs TASK-039 as attempt #3), did NOT run Git, did NOT edit TASKBOARD.md.
