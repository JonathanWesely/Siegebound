# TASK-047 — Match resolution v3: win/lose by team + main-menu level-flow hook

**Status:** ready-for-qa
**Assignee:** gameplay-programmer
**Scope:** files only — NO compile, NO Git, NO editor, NO board edit. Did NOT touch `SiegeBotController.h/.cpp` (TASK-046's file) — only CALLED its public `StopDecisionTimer()`.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`

Only the GameMode changed. `SiegeGameState.h/.cpp` and `SiegePlayerController.h/.cpp` (also in my file set) needed **no** change — the win/lose-by-team resolution and the byte-`SetWinner` display path were already wired and qa-passed in M1/M2 + TASK-045 (see "Verified, unchanged" below).

## The four TASK-047 items

### 1. Win/lose by team (§3.9) — VERIFIED, no change needed
`ASiegeGameMode::OnCastleDestroyedHandler(ACastle*, ETeamId CastleTeam)` already resolves the winner by the DESTROYED castle's team (unchanged from the qa-passed M1 logic):

```
const ETeamId Winner = (CastleTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;
```

- **Red castle destroyed** → `CastleTeam == Red` → `Winner = Blue`. Local player is always Blue → **Victory**.
- **Blue castle destroyed** → `CastleTeam == Blue` → `Winner = Red` → local player (Blue) is not the winner → **Defeat**.

The winner is pushed to every `ASiegePlayerController::HandleMatchEnd(ETeamId Winner)`. The controller passes it to `WBP_VictoryScreen::SetWinner` via `ProcessEvent` under the **M1 byte deviation**: it guards `SetWinnerFunction->ParmsSize == sizeof(ETeamId)`. `ETeamId` is `enum class ETeamId : uint8` (TeamId.h), so `sizeof(ETeamId) == 1`, ABI-identical to the widget's `byte` param — the deviation is preserved byte-for-byte. **SetWinner byte value: Blue = 0 (Victory), Red = 1 (Defeat)** (enum order Blue, Red).

A match can end only through `OnCastleDestroyedHandler`, which is latched by `bMatchEnded` (double-end guard, re-armed only by `PlayAgain`). Nothing else ends a match.

### 2. Freeze the bot at match end — ADDED (the TASK-045 forward-dependency)
`ASiegeGameMode::FreezeWorldAtMatchEnd()` (called from `OnCastleDestroyedHandler` BEFORE the end screen goes up) already: (1) `FreezeAI()` on every `ASummonedUnit` — freezes the bot's already-spawned Red units too; (2) silences towers; (3) clears projectiles; (4) `PauseIncome()` on every `ASiegePlayerState` — the bot's Red PS included; (5) `StopClock()`.

**New step 6** stops the bot's DECISION loop — otherwise its own 2 s timer keeps ticking `EvaluateDecisions` under the Victory screen and spawns fresh Red units into a frozen match:

```
if (IsValid(BotController))
{
    BotController->StopDecisionTimer();   // public TASK-045 hook; idempotent
}
```

Call site: `FreezeWorldAtMatchEnd()`, immediately after `StopClock()`. Uses the single tracked `BotController` member (exactly one bot per match, set in `SpawnBot`). The freeze log line now reports the bot decision-loop state (`stopped` / `absent`).

### 3. Play Again resets BOTH sides — VERIFIED, no change needed
`ASiegeGameMode::PlayAgain()` (unchanged from TASK-045):
- **Both economies** reset in the generic `GameState->PlayerArray` loop (step 4): `ResetEconomy` + `ResetGold` + `ResumeIncome` on the player's Blue PS AND the bot's Red PS (both are in `PlayerArray`).
- **Player deck/hand** via `ASiegePlayerController::HandleMatchReset()` → `DeckComponent->ResetDeck()` (step 6).
- **Bot deck/hand + decision timer** via `BotController->ResetBot()` (step 4b), which runs `ResetDeck` + (idempotent) `ResetEconomy` + `StartDecisionTimer()` — the bot's 2 s decision loop **restarts** for the fresh match (`StopDecisionTimer` from item 2 is undone here).
- Clock (step 3b `ResetClock`, before economy so rate re-derives against a cleared overtime latch), all units + buildings + projectiles destroyed (steps 2/2b/2c), castles to 2000/2000 (step 3), hero restored (step 5), `bMatchEnded` cleared.

Net: gold/deck/hand/miners/buildings/clock/castles/hero reset for player + bot, and the bot resumes deciding at a clean 2 s beat.

### 4. Main-menu start-match hook — ADDED
`ASiegeGameMode::StartMatch(const UObject* WorldContextObject)` — **static, BlueprintCallable, WorldContext**:

```
UFUNCTION(BlueprintCallable, Category = "Siegebound|Match", meta = (WorldContext = "WorldContextObject"))
static void StartMatch(const UObject* WorldContextObject);
```

- Opens the arena via `UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, ArenaLevel)`.
- `ArenaLevel` is a new `UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UWorld>`, default `/Game/Maps/L_Arena.L_Arena` (CONVENTIONS map). Read from the CDO (`GetDefault<ASiegeGameMode>()`) so the function stays static while the path stays designer-editable.
- **Why static + WorldContext (not a member call):** TASK-049's L_MainMenu runs its own menu game mode, NOT an `ASiegeGameMode`, so `WBP_MainMenu` must start a match without an `ASiegeGameMode` instance present. A static node is callable from any Blueprint; the widget threads its own world context automatically.
- Null-safe: logs + no-ops if the world context or the CDO arena path can't be resolved. `OpenLevelBySoftObjectPtr` resolves the FULL asset path (robust vs a bare short name in packaged builds) and travels to a clean world — the new `ASiegeGameMode::BeginPlay` spawns the bot and deals both decks, so this route needs no in-place reset.

## Signature for TASK-049 (WBP_MainMenu)
Play (vs Bot) button → call **`Start Match`** (static node, class `ASiegeGameMode`, category `Siegebound|Match`). The `World Context Object` pin auto-fills from the widget. No cast, no Get Game Mode required. Opens `/Game/Maps/L_Arena`.

## C4458 / shadow check (CONVENTIONS coding law)
New identifiers introduced: parameter `WorldContextObject`, locals `Defaults`, `Arena`, `bBotStopped`, member `ArenaLevel`. None shadow an inherited reflected UPROPERTY (`Owner`/`PlayerState`/`Instigator`/`Controller`/`Pawn`/`GameState`/etc.). Clean.

## What QA should scrutinize
- **SetWinner byte mapping** stays the M1 deviation: `ProcessEvent(&WinnerParam)` where `WinnerParam` is `ETeamId` (uint8) — confirm the `ParmsSize == sizeof(ETeamId)` guard still matches the widget's single `byte` param and that Blue=Victory / Red=Defeat is intact (no regression to the M1/M2 path).
- **StopDecisionTimer call site**: it runs in the match-end freeze (before the end screen), and `ResetBot()`'s `StartDecisionTimer` re-arms it on Play Again — confirm the stop/restart pairing has no leak (stopping an already-idle bot is idempotent; restarting a stopped bot is a fresh SetTimer that replaces the handle).
- **Static StartMatch reading the CDO**: `GetDefault<ASiegeGameMode>()->ArenaLevel` — protected-member access from a static member of the same class is legal; confirm the soft path `/Game/Maps/L_Arena.L_Arena` matches the CONVENTIONS map name TASK-049 will open.
- **Include added**: `Kismet/GameplayStatics.h` (for `OpenLevelBySoftObjectPtr`).
- **Did NOT touch** `SiegeBotController.h/.cpp` (TASK-046 owns it) — only called its public `StopDecisionTimer()` + `ResetBot()`; `SiegeGameState`/`SiegePlayerController` unchanged (win path + byte display already qa-passed).

## Constraints honored
Files only. No compile (TASK-051 batches). No Git. No TASKBOARD edit (orchestrator owns the board). M1/M2 Blue-side match-end + Play-Again behavior preserved byte-for-byte where unchanged.
