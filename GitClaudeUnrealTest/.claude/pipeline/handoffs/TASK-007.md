# TASK-007 Handoff — PlayerController: Footman card play + placement mode (C++)

- author: gameplay-programmer
- date: 2026-07-02
- status: implementation complete, files-only (no compile, no editor, no Git, no TASKBOARD edit per task constraints)

## Files created

1. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
2. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

Nothing else touched. `GitClaudeUnrealTest.Build.cs` NOT modified — `UMG`, `Slate`, `EnhancedInput`, `InputCore` are already public dependencies. Include as `#include "Siegebound/SiegePlayerController.h"`.

## Class contract

`UCLASS() class GITCLAUDEUNREALTEST_API ASiegePlayerController : public APlayerController` — plain C++ class, no BP subclass planned for M1 (TASK-006 sets `PlayerControllerClass = ASiegePlayerController` directly). Because of that, ALL content/input references are soft paths resolved null-safe at runtime — no editor assignment is required for anything.

## Exact public signatures (for TASK-006 / TASK-011)

```cpp
UFUNCTION(BlueprintCallable) void EnterPlacementMode(FName CardID);   // HUD card button / IA_Card1
UFUNCTION(BlueprintCallable) void ExitPlacementMode();                // cancel; safe to call any time (idempotent)
UFUNCTION(BlueprintCallable) void HandleMatchEnd(ETeamId Winner);     // GameMode -> end screen + UI-only input
UFUNCTION(BlueprintCallable) void HandleMatchReset();                 // GameMode PlayAgain -> back to gameplay input
UFUNCTION(BlueprintPure)     bool IsInPlacementMode() const;
UFUNCTION(BlueprintPure)     bool HasMatchEnded() const;

UPROPERTY(BlueprintAssignable) FOnCardPlayRefused OnCardPlayRefused;  // (FName CardID, FText Reason)
```

`DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardPlayRefused, FName, CardID, FText, Reason);` — declared in SiegePlayerController.h.

## For TASK-006 (GameMode)

- On castle destroyed: call `SiegePC->HandleMatchEnd(Winner)` — Winner = `ETeamId::Blue` when the RED castle fell (Victory), `ETeamId::Red` when the Blue one fell (Defeat variant). HandleMatchEnd exits placement mode FIRST (releases melee suppression per the qa-note), latches `bMatchEnded` (blocks further card plays, double-call safe), shows the end screen, and switches to UI-only input.
- In `PlayAgain()`: call `SiegePC->HandleMatchReset()` — clears the match-ended latch, removes the victory widget if the widget didn't remove itself (idempotent with its own RemoveFromParent), restores GameOnly input + hidden cursor. Order vs. the rest of the reset doesn't matter to me; remember the TASK-006 qa-note about clearing timers BEFORE ResetGold (unrelated to this class).
- Hero death needs NOTHING from TASK-006: the controller binds `AHeroCharacter::OnHeroDied` itself in `OnPossess` (and unbinds in `OnUnPossess`).

## For TASK-011 (widgets) — exact contracts I emit

**WBP_HUD** (`/Game/UI/WBP_HUD.WBP_HUD_C`): created and `AddToViewport()`-ed once in `BeginPlay`, owning player = this controller. No parameters passed. Missing asset = one warning log, game continues. Card button calls `EnterPlacementMode("Footman")` (row name, PascalCase). Optionally bind `OnCardPlayRefused(FName CardID, FText Reason)` to flash refusal messages ("Not enough gold" / "Invalid placement location" / "Card data unavailable").

**WBP_VictoryScreen** (`/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C`): created by `HandleMatchEnd`, added at ZOrder 10 (above the HUD), then input goes UI-only with focus on it.
- **Winner contract:** implement a BlueprintCallable function (or custom event) named exactly **`SetWinner`** with exactly ONE input parameter of type **`ETeamId`** (any param name) and no return/outputs. I resolve it by name via `FindFunction("SetWinner")` and call it **after CreateWidget, BEFORE AddToViewport** — store the value into a variable; `Construct` runs after and can read it. If the function is absent the widget still shows (log only); if the signature differs (`ParmsSize != 1`) I skip the call with a warning rather than corrupt memory.
- Play Again button: call `ASiegeGameMode.PlayAgain()` then `RemoveFromParent` per your spec — `HandleMatchReset` tolerates the widget already being gone.

## For TASK-009 (input) — slot names + IMC wiring

- Controller UPROPERTY slots (Category `Input`, EditAnywhere): **`Card1Action`**, **`CancelPlaceAction`**. In M1 you do NOT need to assign them anywhere: at `SetupInputComponent` the controller falls back to soft paths **`/Game/Input/Actions/IA_Card1`** and **`/Game/Input/Actions/IA_CancelPlace`** (LoadSynchronous, null-safe). Just create the assets at those exact paths and map them in **IMC_Hero** (keyboard `1`; RMB AND Escape). The hero adds IMC_Hero itself (TASK-003 handoff) — action bindings on the controller's EnhancedInputComponent fire from the same context.
- **Confirm is NOT an action binding** (design call, documented for the spec's "your call"): the pawn's `AttackAction` is a protected member I can't reach without modifying QA-passed files, so while in placement mode I poll `WasInputKeyJustPressed(EKeys::LeftMouseButton)` in `PlayerTick`. The same physical click still triggers the hero's IA_Attack binding, where `SetMeleeSuppressed(true)` makes `DoMeleeAttack` a cooldown-free no-op (TASK-003 handoff) — so nothing double-fires. IA_Attack needs no controller-side mapping changes.
- **Cancel is belt-and-braces:** the `IA_CancelPlace` Started binding AND direct polling of RMB/Escape in `PlayerTick`. A missing TASK-009 asset can therefore never soft-lock the player (= never leave melee suppressed). Both firing on the same click is harmless — `ExitPlacementMode` is idempotent.

## Every SetMeleeSuppressed(false) path (QA: qa/TASK-003-report.md warning 2)

Suppression is set ONLY in `EnterPlacementMode` (recorded in `PlacementHero`). `ExitPlacementMode()` releases it on `PlacementHero` **before any early-out** — even if `bInPlacementMode` was already false. Callers:

1. **Confirm** — end of `TryConfirmPlacement` after a successful spawn.
2. **Cancel via IA_CancelPlace** — `OnCancelPlacePressed`.
3. **Cancel via polled RMB/Esc** — `PlayerTick` fallback.
4. **Match end** — first statement of `HandleMatchEnd` (before the latch check, so even a duplicate match-end call releases it).
5. **Hero death** — `HandleHeroDied`, bound to `FOnHeroDied` in `OnPossess` (`AddUniqueDynamic`), unbound in `OnUnPossess`.
6. **Unpossession** — `OnUnPossess` (covers pawn swap/destroy without a death event; uses the recorded `PlacementHero`, not `GetPawn()`).
7. **Teardown** — `EndPlay`.

Invalid-click and failed-SpendGold branches deliberately do NOT exit (spec: refuse, spend nothing, STAY in mode) — suppression correctly persists because the mode persists.

## Behavior notes / QA scrutiny points

1. **Cost is never hardcoded**: `EnterPlacementMode` reads `Cost` from `/Game/Data/DT_Cards` row `CardID` (soft load). Missing table/row = refuse + `OnCardPlayRefused` ("Card data unavailable") — it does NOT enter the mode with a guessed cost. Cost is cached as `PendingCost` for the confirm; gold income between enter and confirm can only increase gold, and `SpendGold` re-gates anyway.
2. **Exact-deduction ordering**: confirm does `SpawnActorDeferred` → `SpendGold(PendingCost)` → (`InitUnit` + `FinishSpawning`) . If SpendGold refuses, the half-spawned deferred actor is `Destroy()`-ed — so gold is spent if and only if a unit finishes spawning, and exactly once (§3.5 acceptance: deducts exactly 3).
3. **Deferred spawn + InitUnit** per TASK-004 handoff preferred path: `InitUnit(Team, CardID)` runs before BeginPlay binds stats; Team mirrors the hero's `GetTeamId()` when possessed (falls back to `ETeamId::Blue` — CONVENTIONS: local player is always Blue). Collision handling `AdjustIfPossibleButAlwaysSpawn`, spawn Z = ground hit + the unit class CDO's capsule half-height + 2.
4. **Unit class resolution**: `/Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C` built from the CONVENTIONS pattern (character-exact `BP_Unit_Footman` for Footman). Missing/incompatible class → log + fall back to raw `ASummonedUnit` (logic-complete, meshless) per spec.
5. **Validity rule**: blocking hit on the Visibility channel under the cursor AND `ImpactPoint.X <= 0` (`PlacementMaxX`, EditDefaultsOnly). No ground hit hides the ghost and is invalid. Known M1 quirk: any Visibility-blocking surface counts as "ground" — e.g. the Blue castle's roof is placeable (X<=0); the collision-adjusted spawn handles it. Pawns don't block Visibility, the ghost has all collision disabled, so neither can occlude the trace.
6. **Ghost**: transient `AStaticMeshActor` (movable, `SetActorEnableCollision(false)`, NoCollision profile, no nav impact), mesh `/Game/Meshes/SM_Footman`, one `UMaterialInstanceDynamic` of `/Game/Materials/M_Ghost` applied to all slots, vector param exactly `"GhostColor"` — `ValidGhostColor` (0,1,0) / `InvalidGhostColor` (1,0,0), opacity left to the material (TASK-012 handoff). Every ghost asset missing = warning + degraded preview, placement logic unaffected. Ghost yaw = `GhostYawOffset` (-90°) per the TASK-014 facing note.
7. **Input modes**: placement = `FInputModeGameAndUI` (DoNotLock, cursor visible during capture) + `bShowMouseCursor` — mouse steers the cursor instead of the camera while WASD keeps working; exit restores `FInputModeGameOnly` + hidden cursor; match end = `FInputModeUIOnly` focused on the victory widget. Minor UX caveat for playtest: while LMB is held during the confirm click the viewport captures the mouse, so a drag can nudge the camera for that instant.
8. **Re-entry guards**: `EnterPlacementMode` refuses while already placing, after match end, with `CardID == None`, with a dead hero, or without an `ASiegePlayerState` (logged as an error naming TASK-006).
9. Tick cost: `PlayerTick` early-outs unless placement mode is active; no timers; nothing runs per-frame outside the mode.
10. Not compiled (build-master's job); concurrent TASK-008 editor work was not touched.

## Acceptance mapping (§3.5)

- cost-3 card refused at 2 gold → `CanAfford` gate in EnterPlacementMode (+ HUD hook via OnCardPlayRefused)
- click on Blue half deducts exactly 3, spawns Footman at the point → TryConfirmPlacement ordering (note 2/3)
- click at X > 0 refused, no gold, red ghost → validity rule (note 5) + refuse-and-stay branch
- cancel spends nothing → ExitPlacementMode paths 2/3, no SpendGold call
- hero cannot melee while placing → SetMeleeSuppressed(true) on enter; released on the 7 exit paths above
