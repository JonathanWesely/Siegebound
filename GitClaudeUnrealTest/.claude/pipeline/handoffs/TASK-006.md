# TASK-006 Handoff — GameMode: win condition, hero respawn, Play Again reset (C++)

- author: gameplay-programmer
- date: 2026-07-02
- status: implementation complete, files-only (no compile, no editor, no Git, no TASKBOARD edit per task constraints)

## QA loop 1 fix (qa/TASK-006-report.md)

- **Blocker (finding 1) fixed** in `SiegeGameMode.h`: removed the `UFUNCTION(BlueprintPure, Category = "Siegebound|Match")` macro from `HasMatchEnded` and declared it `virtual bool HasMatchEnded() const override { return bMatchEnded; }`. The previous declaration implicitly overrode UE 5.8's `AGameModeBase::HasMatchEnded` (itself a UFUNCTION), and UHT rejects a new UFUNCTION macro on an override of an inherited UFUNCTION. Same inline body/semantics; no `.cpp` change was needed (the definition was header-inline). The class-contract block below is corrected accordingly — TASK-011's Blueprint node now comes from the inherited engine declaration.
- **Finding 6 nit applied** (sanctioned as an optional TODO by the report): `TODO(M8)` comment added at the `TrackedHero` member in `SiegeGameMode.h` noting the single-hero assumption.
- **Findings 2 and 5 TODO suggestions (`TODO(M2)` / `TODO(M3)` in `SiegeGameMode.cpp`) intentionally NOT applied**: the QA loop constraints allowed touching the `.cpp` only if the override required it (it did not), and the report classifies them as optional with no code change required for M1. Carry them into M2/M3 task specs.
- Nothing else changed; findings 2-6 stand as reviewed (WARN 2 accepted for M1, nits 3-5 documented deviations).

## Files touched

1. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` (new)
2. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` (new)
3. `Config/DefaultEngine.ini` (three values edited in the existing `[/Script/EngineSettings.GameMapsSettings]` section — see below)

Nothing else touched. No Build.cs change needed (Core/CoreUObject/Engine only). Include as `#include "Siegebound/SiegeGameMode.h"`. QA-passed headers (Castle, HeroCharacter, SiegePlayerState, SiegePlayerController, SummonedUnit) were NOT modified.

## Class contract

`UCLASS() class GITCLAUDEUNREALTEST_API ASiegeGameMode : public AGameModeBase`

Class defaults set in the constructor: `PlayerStateClass = ASiegePlayerState`, `PlayerControllerClass = ASiegePlayerController`, `DefaultPawnClass = AHeroCharacter` (hard fallback only — see pawn-class pattern).

### For TASK-011 (WBP_VictoryScreen's Play Again button) — the exact contract

```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
void PlayAgain();

// HasMatchEnded is NOT a new UFUNCTION on ASiegeGameMode. UE 5.8's AGameModeBase
// already declares it as UFUNCTION(BlueprintCallable, Category=Game) virtual bool
// HasMatchEnded() const, and UHT rejects a UFUNCTION macro on an override of an
// inherited UFUNCTION. ASiegeGameMode overrides it natively:
virtual bool HasMatchEnded() const override; // returns the bMatchEnded latch
```

- **TASK-011 still gets a Blueprint node for `HasMatchEnded`** — it comes from the inherited engine declaration (Category "Game", not "Siegebound|Match"; a BlueprintCallable const function renders as a pure-style node). Call it on the game mode (the cast to `ASiegeGameMode` is not even required for this node, though TASK-011 casts anyway for `PlayAgain`); virtual dispatch returns the correct Siegebound latch.

- Get the game mode via `GetGameMode` + cast to `ASiegeGameMode` (M1 is local-only, so the game mode always exists client-side), call `PlayAgain()`, then `RemoveFromParent` per the TASK-011 spec. `ASiegePlayerController::HandleMatchReset()` (which PlayAgain calls last) also removes the widget if it hasn't removed itself — both orders work.
- **Double-click safe:** re-entrant calls are dropped by an in-progress guard (`TGuardValue`), and a second sequential call just re-runs steps that are all idempotent (destroying zero units, resetting full castles, gold 50 -> 50 without a broadcast, resetting an alive hero, idempotent HandleMatchReset). The button does NOT need its own debounce.
- PlayAgain does not require the match to have ended — calling it mid-match is a legal full reset (useful for QA).

## PlayAgain order of operations (and qa-note compliance)

Exact order, chosen to satisfy the TASK-006 qa-note (from qa/TASK-005-report.md major 1 — `ASiegePlayerState::ResetGold()` RESTARTS the income timer):

1. **Clear this mode's own timers** — `GetWorldTimerManager().ClearTimer(HeroRespawnTimerHandle)`. This is the ONLY timer the class owns, and it is cleared **selectively and BEFORE ResetGold**. There is **no** `ClearAllTimersForObject` on foreign objects and **no** world-wide clear anywhere in the class — the qa-note's failure mode (income timer killed after ResetGold restarts it) is structurally impossible: nothing after step 1 clears any timer.
2. Destroy every `ASummonedUnit` (collected into an array first, then destroyed — never `Destroy()` out of a live `TActorIterator`). Their `EndPlay` clears their own AI/attack timers.
3. `ResetCastle()` on every `ACastle` — back to 2000/2000, visible, colliding; re-arms `OnCastleDestroyed`. The BeginPlay `AddUniqueDynamic` bindings persist on the actors, so no rebinding is needed.
4. `ResetGold()` on every `ASiegePlayerState` (via `GameState->PlayerArray`) — back to 50; restarts its own income timer, which now survives (see step 1).
5. `bMatchEnded = false` (re-arms the win condition), then `RestoreHeroAtStart()` — hero teleported to the start, repossessed if needed, `ResetHero()` (full HP, visibility, collision, movement, input). Works whether the hero ended the match dead or alive.
6. `ASiegePlayerController::HandleMatchReset()` on every Siege controller — removes the end screen, clears the controller's match latch, restores game-only input (TASK-007 contract; TASK-007 confirmed ordering vs. the rest is free).

**Timer-policy design call (spec asked to document):** per-handle `ClearTimer` on our own `FTimerHandle` was chosen over any `ClearAllTimersForObject(...)` pattern. Even `ClearAllTimersForObject(this)` would be fragile (a future timer added to the game mode would silently die on every reset), and clearing on other objects is exactly what the qa-note forbids. Each system owns and clears its own timers; the game mode only ever touches `HeroRespawnTimerHandle`.

## Win condition

- `BeginPlay` iterates all `ACastle` and binds `OnCastleDestroyed` with `AddUniqueDynamic` (handler is a UFUNCTION per the TASK-002 binding pattern). Zero castles found = warning naming the integration step (win condition unreachable until Castle_Blue/Castle_Red are placed).
- `OnCastleDestroyedHandler(ACastle*, ETeamId CastleTeam)`: Winner = the OTHER team (Red castle fell -> Blue wins = Victory; Blue fell -> Red wins = Defeat variant, wired though nothing damages Blue in M1). Latches `bMatchEnded` FIRST — two castles falling in the same frame or duplicate broadcasts can never double-end the match. Cancels any pending hero respawn (nothing revives under the end screen; PlayAgain owns restoration from there). Then `HandleMatchEnd(Winner)` on every `ASiegePlayerController` (M1: the single local one).
- A match cannot end any other way: `bMatchEnded` is written nowhere else (`true` only in this handler, `false` only in PlayAgain).

## Hero respawn design

- **Binding point:** `SetPlayerDefaults(APawn*)` override — `AGameModeBase::FinishRestartPlayer` calls it for EVERY pawn the mode hands to a player (initial spawn AND any restart fallback), so `OnHeroDied` is always bound on the current pawn with `AddUniqueDynamic`, and `TrackedHero` is updated. This is how "re-binding for the new/reset pawn" is handled: same-pawn respawns keep the existing binding (the instance never changed); fresh-pawn restarts flow through `SetPlayerDefaults` again.
- **Death:** `HandleHeroDied` schedules `RespawnHero` in exactly `HeroRespawnDelay` (5.0, EditDefaultsOnly, §3.1 "back within 5-6 s") — single-shot, `SetTimer` on the same handle replaces, so stacking is impossible. Suppressed after match end.
- **Respawn (`RestoreHeroAtStart`, shared with PlayAgain):** teleport the (still hidden) pawn to the spawn point -> repossess if the controller lost the pawn (BEFORE `ResetHero`, whose `EnableInput` mirrors death's `DisableInput` against the possessing controller — TASK-003 handoff; normally possession survived death and this is skipped) -> `ResetHero()` -> `SetControlRotation` so the camera faces the spawn direction. Same pawn instance is reused (TASK-003 explicitly supports this); a destroyed/missing pawn falls back to `RestartPlayerAtPlayerStart` (fresh default pawn, rebound via `SetPlayerDefaults`).
- **Spawn point resolution:** (1) the level's `APlayerStart` via `FindPlayerStart` (L_Arena: (-1700, 0, 100) yaw 0 on the Blue side, TASK-015) — `FindPlayerStart`'s WorldSettings fallback is explicitly rejected; (2) no PlayerStart -> the hero's own-team castle offset `HeroSpawnCastleOffset` (600, 0, 100) toward the centerline (clears the ~810-unit castle footprint, faces the enemy half); (3) arena origin +100 Z, with a warning.
- Melee suppression on death/respawn is NOT this class's job: `ASiegePlayerController` releases it on its own OnHeroDied binding (TASK-007 handoff, exit path 5).

## DefaultPawnClass pattern (spec asked to document the choice)

Lazy resolution in `GetDefaultPawnClassForController_Implementation` -> `ResolveHeroPawnClass()`:

- `HeroPawnClassAsset` = `TSoftClassPtr<AHeroCharacter>` defaulting to `/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C` (TASK-006 names block, character-exact).
- `LoadSynchronous()` at first spawn request; success is cached in `ResolvedHeroPawnClass`, failure is NOT cached (a blueprint imported later in an editor session is picked up on the next spawn) and warns once. `TSoftClassPtr<AHeroCharacter>::LoadSynchronous` already returns nullptr for non-AHeroCharacter classes, so the cached class is always spawnable and compatible.
- Fallback: `AHeroCharacter::StaticClass()` — non-abstract and fully input-null-safe by TASK-003 design (meshless but playable).
- `ConstructorHelpers::FClassFinder` was deliberately rejected: it logs a load error on every CDO construction while the asset is missing, and BP_HeroCharacter is produced in parallel by TASK-009 — not null-safe by this project's standard. Constructor keeps `DefaultPawnClass = AHeroCharacter` so direct property reads stay sane.

## Config/DefaultEngine.ini changes

In the existing `[/Script/EngineSettings.GameMapsSettings]` section (values edited in place, section NOT duplicated, `bOffsetPlayerGamepadIds=False` preserved):

```ini
GameDefaultMap=/Game/Maps/L_Arena.L_Arena
EditorStartupMap=/Game/Maps/L_Arena.L_Arena
GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode
```

No other section touched. Note for build-master: the editor will now open L_Arena on startup and PIE will boot ASiegeGameMode with no per-map override needed (M1 manager decision: config-default game mode boots the loop).

## For QA to scrutinize

1. **SetPlayerDefaults as the death-binding hook** — chosen because `FinishRestartPlayer` invokes it after possession for every mode-issued pawn. If QA prefers a belt-and-braces bind in BeginPlay too (for a hypothetical level-placed possessed hero, which M1 doesn't have), say so; left out to avoid a redundant path.
2. **Match end does NOT freeze the world**: units keep fighting and income keeps ticking under the Victory screen (input goes UI-only via TASK-007). The spec only demands the screen and the reset; flag if M1 should pause instead.
3. **Respawn cancelled on match end** (hero stays dead under the end screen) — my reading of §3.9/§3.1 interplay; PlayAgain revives regardless.
4. **`RestoreHeroAtStart` reuses the pawn instance** rather than destroy+respawn (TASK-003 handoff blesses both; reuse avoids re-running NotifyControllerChanged/IMC setup and keeps the controller's delegate bindings). The destroy-fallback path exists but no M1 flow triggers it.
5. `GetDefaultPawnClassForController_Implementation` ignores `InController` and `Super` — intentional: single pawn class for all players in M1, and the fallback is the same class the constructor put in `DefaultPawnClass`.
6. Teleport uses `SetActorLocationAndRotation(..., ETeleportType::TeleportPhysics)` with no sweep — the spawn point is clear by design (PlayerStart placement / offset past the castle footprint). Residual walk velocity on an alive-hero PlayAgain reset is carried through the teleport; considered harmless.
7. Not compiled (build-master's job). PlayerArray iteration assumes `GameState` is valid at PlayAgain time (it is — the match is running); guarded with an `if` anyway.

## Acceptance mapping (§3.9)

- 2000 damage to the Red castle triggers the Victory screen -> castle binding + OnCastleDestroyedHandler -> HandleMatchEnd(Blue)
- Play Again restores gold 50 / castles 2000/2000 / zero units / hero alive at spawn -> PlayAgain steps 1-6
- hero killed at 0 HP is back at its castle in 5-6 s -> HandleHeroDied timer (5.0 s exact) + RestoreHeroAtStart
