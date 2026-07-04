# TASK-045 handoff — ASiegeBotController: AIController brain, economy + deck ownership, spawn + Play-Again reset (the bot SHELL)

**Status:** ready-for-qa · files-only (no compile, no editor, no Git, no board edit) · M3 wave 2 on `main`, parallel with TASK-044 · **did NOT touch TASK-044's files** (SummonedUnit / Building / MinerUnit).

## What this delivers
The bot SHELL only — NO decision rules yet (those are TASK-046). After this: on BeginPlay the match has exactly one `ASiegeBotController` (an `AAIController` possessing no pawn) that owns a **Red** `ASiegePlayerState` (gold accrues +2/s, +4/s in overtime — the reused M2 economy), a 50-card deck dealt to a hand of 6 (§3.4), and a 2 s repeating timer ticking an EMPTY `EvaluateDecisions()`. Play Again resets the bot's deck/economy/timer. The player's Blue economy is untouched.

## Files touched (3)
- **NEW** `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` — `ASiegeBotController : AAIController`.
- **NEW** `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` / `.cpp` — spawn one bot + tag its PS Team=Red; ResetBot in PlayAgain.

**No Build.cs change** — `AIModule` (provides `AAIController`) is already a public dependency (Build.cs line 17). So no NEW module is added and no module-dependency-driven full rebuild is forced. (The new `.h/.cpp` pair + the `SiegeGameMode.h` header changes still require a normal recompile — TASK-051 batches it. Header changes are NOT hot-reload-friendly.)

## The bot class structure (`ASiegeBotController`)
- **Constructor:** `bWantsPlayerState = true` (engine auto-creates the PlayerState of `PlayerStateClass` = `ASiegePlayerState` in `PostInitializeComponents`); `bStartAILogicOnPossess = false` (defensive — the bot uses no behavior tree and possesses nothing; the decision loop is timer-driven, not possession-driven); `DeckComponent = CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"))` (subobject name is the spec/CONVENTIONS contract; the component self-defaults its `CardTableAsset` to `/Game/Data/DT_Cards`, so the deck builds with no extra wiring).
- **BeginPlay:** `DeckComponent->BuildAndShuffle()` (deals the hand of 6, §3.4) then `StartDecisionTimer()`. The component never self-builds (TASK-022 flagged decision 12) — the controller owns the timing, exactly as `ASiegePlayerController` does.
- **EndPlay:** `StopDecisionTimer()` (own-timer hygiene only).
- **`EvaluateDecisions()`** — EMPTY body (a comment only). Plain protected member fired by the timer via member-fn pointer (no UFUNCTION needed).
- **`StartDecisionTimer()`** — `SetTimer(DecisionTimerHandle, this, &EvaluateDecisions, DecisionIntervalSeconds, /*loop*/true)`. SetTimer on the same handle replaces, so repeated calls (BeginPlay, then ResetBot per Play Again) never stack.
- **`StopDecisionTimer()`** (public) — clears the handle. **This is the hook TASK-047 calls at match end** to stop the bot deciding under the Victory screen.
- **`ResetBot()`** (public) — Play-Again entry (see below).
- Members: `TObjectPtr<UDeckComponent> DeckComponent`; `ETeamId BotTeam = ETeamId::Red` (single source of truth for the bot's team); `float DecisionIntervalSeconds = 2.f // GDD §4`; `FTimerHandle DecisionTimerHandle`.
- Accessors for TASK-046: `GetBotTeam()`, `GetBotPlayerState()` (`GetPlayerState<ASiegePlayerState>()` — verified on `AController` in UE 5.8), `GetDeckComponent()`.

## Red-PS tagging point (completes the TASK-043 forward-ref)
`ASiegeGameMode::SpawnBot()` (run from BeginPlay) spawns the bot, then:
```
if (ASiegePlayerState* BotPS = BotController->GetBotPlayerState())
    BotPS->SetTeam(BotController->GetBotTeam());   // Red
```
- I set the PS team **from the bot's own `BotTeam`** (not a hardcoded `Red` at the call site) so the PS team, the §4 spawn geometry (TASK-046), and `GetPlayerStateForTeam` can never diverge — one source of truth.
- The old `// TASK-045: bot PS Team=Red …` comment in `InitNewPlayer` is rewritten to point at `SpawnBot()`. The mode now sets **both** teams (Blue in `InitNewPlayer`, Red in `SpawnBot`) — the "GameMode sets both" M3 ruling.
- **Why setting Team after the PS's BeginPlay is safe:** `Team` is pure identity — `SetTeam` fires no economy delegate and `GetGoldRate()` never reads `Team`. The bot PS's BeginPlay (`ResetGold` → income timer, overtime bind, `CachedGoldRate` seed) is complete and correct before the tag lands; the tag only routes miners/decisions to the Red economy via `GetPlayerStateForTeam`, and nothing needs it during match-start BeginPlay (no miner spawns then).

## GameMode spawn + Play-Again wiring
- **Spawn (BeginPlay → `SpawnBot()`):** guarded against a second spawn (`IsValid(BotController)`); resolves `BotControllerClass` (new `EditDefaultsOnly` UPROPERTY defaulting to `ASiegeBotController::StaticClass()` in the ctor — a pure C++ class, so a direct StaticClass default is safe, unlike the lazily-resolved hero BP); `SpawnActor<ASiegeBotController>(Class, FTransform::Identity, Params)` with `AlwaysSpawn` + `RF_Transient`; then tags the PS Red. **Lifecycle point:** BeginPlay is after `InitGameState` (GameState exists) and after the local player login (`InitNewPlayer` already tagged the player PS Blue and set `PlayerStateClass`), so the bot's auto-PS resolves. BeginPlay runs once per world begin and PlayAgain is in-place (never re-runs BeginPlay), so exactly one bot exists per match, reused across resets.
- **Play Again (`PlayAgain()` new step 4b, after the generic PlayerArray economy loop):** `if (IsValid(BotController)) BotController->ResetBot();`. Placed after step 3b `ResetClock()`, so `ResetBot`'s `ResetEconomy` re-derives the rate against a cleared overtime latch.
- **`ResetBot()` does:** (1) `DeckComponent->ResetDeck()` — fresh §3.4 deck+hand; (2) `GetBotPlayerState()->ResetEconomy()` — miner/rate state to base; (3) `StartDecisionTimer()` — fresh 2 s cadence.

## How the player economy stays completely untouched
- Every `ASiegePlayerState` owns an **independent** gold value, income `FTimerHandle`, and miner counts. Adding a second (Red) PS cannot perturb the Blue PS's accrual — there is zero shared mutable economy state. The only shared thing is the `ASiegeGameState` overtime latch, which is **read-only** for both.
- Miners already resolve their economy via `GetPlayerStateForTeam(OwnTeam)` (TASK-043) — a Blue miner still finds the Blue PS. With exactly one PS per team there is never ambiguity.
- The two `PlayerArray` loops in the GameMode (`FreezeWorldAtMatchEnd` PauseIncome; `PlayAgain` ResetEconomy/ResetGold/ResumeIncome) are **already generic over all player states** (comment in FreezeWorldAtMatchEnd literally says "M3 bot ready"). They now also hit the bot PS — the correct/intended behavior — and the Blue PS's calls are byte-identical to M2.
- The player's HUD/gold binds through the controller's **own** `GetPlayerState<ASiegePlayerState>()` (per-controller, unambiguous) — no `PlayerArray[0]`/"first PS" assumption anywhere (I grepped the module).

## Single-PS assumption audit (the critical "don't break M2" check)
I swept the whole Siegebound module for single-/first-PlayerState assumptions. Findings:
- **`GetPlayerControllerIterator()` loops** (GameMode `HandleMatchEnd`, `HandleMatchReset`, `FindLocalSiegeController`) iterate **APlayerControllers only** — the bot is an `AAIController`, so it is **correctly excluded**. Consequences, both intended: (a) the bot never takes the end screen; (b) the bot's deck is **not** reset by the player's `HandleMatchReset` loop — which is exactly why `ResetBot` owns the bot's `ResetDeck`. I added a clarifying comment on `FindLocalSiegeController`.
- **`PlayerArray` loops** (freeze-income, PlayAgain economy) — generic; correctly **include** the bot PS. No change needed; M2-preserving by design.
- **No `PlayerArray[0]` / "first PlayerState" / `GetGameState()->PlayerArray[...]` assumption exists** for gold display or win-condition. Win condition keys off castle team, not PS count.
- **Nothing to flag as unresolved.** The design is clean: player-controller loops skip the bot (AIController), PlayerArray loops embrace it (generic).

## Flagged decisions for QA
1. **`ResetBot` calls `ResetEconomy` redundantly** with PlayAgain's generic PlayerArray loop (which does ResetEconomy+ResetGold+ResumeIncome on the bot PS too). I followed the spec literally ("ResetBot: ResetDeck + ResetEconomy + clear timer") and made it **idempotent belt-and-braces** so ResetBot is self-contained if ever called alone. The bot's gold-to-50 + income-restart genuinely come from the loop (ResetBot does NOT call ResetGold/ResumeIncome — those belong to the generic loop that the bot PS is part of). If QA prefers ResetBot own the FULL bot economy reset and the loop be left to the player only, note that the loop can't practically exclude the bot without special-casing (which would erode the M2-preserving generic design) — so I left the loop generic and ResetBot idempotent. Documented in code.
2. **"clear the decision timer" (spec) vs "resets the timer" (acceptance):** I implemented ResetBot to **restart** the timer (`StartDecisionTimer`, which clears-then-sets), not merely clear it, because the acceptance requires the bot to resume deciding after Play Again. A bare clear would leave the bot dead after the first reset. Flagging the wording tension explicitly.
3. **Match-end does NOT yet stop the bot decision timer** — `FreezeWorldAtMatchEnd` pauses the bot's income (generic PlayerArray loop) but does not touch the decision timer. Per the spec this is **TASK-047's** job ("bot decision timer stopped"); I provided the public `StopDecisionTimer()` hook for it. In this shell `EvaluateDecisions` is empty, so a timer ticking after match end is a harmless no-op. Called out so QA doesn't read it as a miss.
4. **`bStartAILogicOnPossess = false`** in the ctor — defensive/documentational only (the bot uses no BT and possesses nothing). Harmless; flagging in case QA wants it dropped as noise.

## API surface handed to TASK-046 (fill `EvaluateDecisions()`)
Everything TASK-046 needs is reachable from inside `ASiegeBotController::EvaluateDecisions()`:
- **Team:** `GetBotTeam()` → `ETeamId::Red`.
- **Economy / gold:** `ASiegePlayerState* BotPS = GetBotPlayerState();` then `BotPS->GetGold()`, `CanAfford(Cost)`, `SpendGold(Cost)`, `CanAddMiner()`, `GetGoldRate()`, `GetAliveMinerCount()`.
- **Deck / hand:** `UDeckComponent* Deck = GetDeckComponent();` then `GetHandCardID(0..5)`, `PeekNextCardID()`, `ConfirmPlayFromHand(Slot)`, `DiscardFromHand(Slot)`, `GetHandSize()`.
- **World economy resolution:** `GetWorld()->GetGameState<ASiegeGameState>()->GetPlayerStateForTeam(GetBotTeam())` (equals `GetBotPlayerState()` for the bot, but this is the team-generic path).
- **Card stats:** read the `FCardRow` from `/Game/Data/DT_Cards` by CardID (soft, null-safe) for Cost/CardType/etc. — never hardcode (§3.0). Mirror `ASiegePlayerController::ResolveCardRow`.
- **Spawn / placement:** reuse the placement-validity rules (TASK-030) — own half = **X ≥ 0** for Red, navmesh projection, building clearance 200; compose `/Game/Blueprints/Units/BP_Unit_<CardID>` (Unit/Economy) and `/Game/Blueprints/Buildings/BP_Building_<CardID>` (Building), spawn Team=Red. Targets: `GoldNode_Red`, `Castle_Red`.
- **Logging:** declare `LogSiegeBot` (CONVENTIONS Logging) and emit exactly one line per fired rule.
- **Cadence gating:** if TASK-046 wants the bot to stop deciding after match end before TASK-047 lands, it can gate on the world/match state; otherwise TASK-047's `StopDecisionTimer()` wiring handles it.

## C4458 / inherited-reflected-member shadow sweep (CONVENTIONS coding law)
- `ASiegeBotController` members `DeckComponent`, `BotTeam`, `DecisionIntervalSeconds`, `DecisionTimerHandle` — none is a reflected member of `AAIController`/`AController`. Safe.
- Locals: `StartDecisionTimer`/`StopDecisionTimer` use `World`; `ResetBot` uses `BotPS`; `GetBotPlayerState` has none. **No local/param/loop var named `PlayerState`, `Pawn`, `Instigator`, `Owner`, `Controller`.** `GetBotPlayerState` deliberately uses `GetPlayerState<T>()` rather than a local `PlayerState`.
- `ASiegeGameMode::SpawnBot` locals `World`, `ClassToSpawn`, `SpawnParams`, `BotPS` — none shadow an inherited reflected member (`World` is already a compiled-clean local elsewhere in this file). New members `BotControllerClass`, `BotController` — no `AGameModeBase` reflected member by those names. Safe.

## Verified against engine source (UE 5.8, since I cannot compile)
- `AAIController::bWantsPlayerState` — public `uint32:1` UPROPERTY (AIController.h:118-120). `bStartAILogicOnPossess` — protected (AIController.h:97-98), reachable from a subclass ctor.
- `UWorld::SpawnActor<T>(UClass*, FTransform const&, const FActorSpawnParameters&)` — World.h:3823.
- `AController::GetPlayerState<T>() const` — Controller.h:192.

## Constraints honored
Files only. No compile, no Git, no TASKBOARD edit. Did not touch TASK-044's files (SummonedUnit / Building / MinerUnit). No Build.cs change.
