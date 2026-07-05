# QA Report — TASK-071 (Sandbox bot-spawn gate + generous economy, C++ pre-compile)

**Verdict: PASS** (0 BLOCKER, 0 WARN, 2 NIT — non-blocking)
Reviewer: qa-reviewer · Base: main @ 5c1fcb7 · files-only, not compiled (correct — build-master TASK-073 owns compile)
Files reviewed: `SiegeGameMode.h`, `SiegeGameMode.cpp`, `SiegePlayerState.h`, `SiegePlayerState.cpp` (+ read-only cross-check of `SiegeGameState.cpp`, `SiegeBotController.cpp`, `MinerUnit.cpp`, `DeepMine.cpp`, `SiegePlayerController.cpp`).

Orchestrator rulings 1 (AddGold accepted as scope) and 2 (999 MaxGold clamp accepted) were NOT re-litigated — the implementation was verified against them (both honored, see below).

## Check-by-check verdict

### 1. Red-PS null-safety completeness — PASS (no unguarded reader found)
Independent grep sweep of EVERY enemy/Red-PlayerState touch point, not just the handoff's 7:
- `GetPlayerStateForTeam` callers: `SiegeGameMode::GrantSandboxStartingGold` (Blue only), `InitNewPlayer` (Blue tag), `AMinerUnit::ResolveOwningPlayerState` (own team), `ADeepMine::ResolveOwningPlayerState` (own team). **No caller ever resolves the ENEMY team's PS.** `GetPlayerStateForTeam(Red)` returning `nullptr` is the documented normal M2 path (SiegeGameState.cpp:96-100) — sandbox just reproduces it.
- Miner/DeepMine resolvers both `if (!OwnerState)` null-check and one-shot the warning (MinerUnit.cpp:371-375, DeepMine.cpp:129-132). In sandbox only Blue miners/mines exist (bot never plays); they resolve the Blue PS. No deref of a null Red PS.
- `GetBotPlayerState` / `EvaluateDecisions` / `ResetBot` are all internally null-safe (`if (!Deck || !BotState) return;`), and unreachable in sandbox anyway — the bot controller never spawns, so its decision timer never arms.
- `PlayerArray` usages (FreezeWorldAtMatchEnd:325, PlayAgain:600, GetPlayerStateForTeam:80) are all generic `for` loops with a `Cast<ASiegePlayerState>` guard. **No `PlayerArray[i]` indexing and no `Num()==2` assumption anywhere** (grep confirmed). Blue-only PlayerArray iterates cleanly.
- Bot freeze/reset hooks are `IsValid(BotController)`-guarded (SiegeGameMode.cpp:351, 618) → skipped in sandbox.
- Every `GetGold()` call site is either the Blue player's own PS (SiegePlayerController) or `BotState` (only reachable through the bot controller). No enemy-PS reader in any HUD/controller.
- **Win condition:** `OnCastleDestroyedHandler` reads the destroyed castle's `CastleTeam` enum (SiegeGameMode.cpp:213), never a PS. `Castle_Red` stays level-placed as a static dummy; destroying it → `Winner = Blue`. Fires correctly with no Red PS. CONFIRMED.

### 2. SpawnBot() gate — PASS
`if (bSandboxMatch)` early-return (cpp:772-778) is the FIRST statement, before the `IsValid(BotController)` guard, before `GetWorld()`, before `SpawnActor<ASiegeBotController>`, and before the Red-PS `SetTeam`. Nothing after the guard runs in sandbox. Single caller (BeginPlay:102); PlayAgain never calls SpawnBot. CONFIRMED.

### 3. InitGame override — PASS
`Super::InitGame(...)` called FIRST (cpp:57), then `bSandboxMatch = UGameplayStatics::HasOption(Options, TEXT("Sandbox"))` (cpp:67). Token string exactly `"Sandbox"` per CONVENTIONS. Declared `virtual ... override;` with NO `UFUNCTION` macro (correct — base is virtual, non-UFUNCTION; matches the HasMatchEnded pattern). Flag is set once and never re-cleared — PlayAgain is in-place and never re-runs InitGame, so sandbox persists across Play Again. CONFIRMED.

### 4. StartMatch byte-identical — PASS
`StartMatch` (cpp:666-697) contains zero sandbox logic — same null-context guard, same CDO `ArenaLevel` resolve, same `OpenLevelBySoftObjectPtr(WorldContextObject, Arena)` (2-arg, no options). `StartSandboxMatch` (cpp:699-733) is a wholly separate static function that mirrors it and appends `TEXT("Sandbox=1")` as the 4th arg with explicit `bAbsolute=true`. Play-vs-Bot path untouched. CONFIRMED. (Verified by reading; recommend build-master's `git diff` as final byte-check, but no behavioral delta is present.)

### 5. AddGold correctness — PASS (routes through the choke point, rate untouched)
`ASiegePlayerState::AddGold(int32 Amount)` (cpp:85-102): non-positive `Amount` refused + logged; positive path is `SetGold(Gold + Amount)` — routes through the single `SetGold()` choke point, so the `[0, MaxGold]` clamp AND `OnGoldChanged` broadcast both apply. **No raw `Gold` field write. No `AddIncome` — the composed rate is untouched.** Sole caller is `GrantSandboxStartingGold` (grep-confirmed the normal match never calls it). Honors orchestrator ruling 1 exactly. Header/cpp signatures match; `UFUNCTION(BlueprintCallable)` present.

### 6. Deferred gold-grant timing — PASS (no UAF / stale-timer)
`SetTimerForNextTick(this, &ASiegeGameMode::GrantSandboxStartingGold)` (cpp:112): the UObject-method overload binds a weak ref to `this`; if the game mode is torn down before next tick it will not fire. `GrantSandboxStartingGold` additionally self-guards `bSandboxMatch`, null-checks `Cast<ASiegeGameState>(GameState)` and the resolved `BlueState` (cpp:844-862) — no-op + warn if unresolvable, never a crash. Deferral rationale is correct: next tick guarantees the Blue PS's own `BeginPlay`→`ResetGold`(50) has run, so the grant lands on top (50 + 9999 → clamp 999) rather than being clobbered. PlayAgain re-grant (cpp:630-633) is synchronous — correct, since the PS already exists and was ResetGold'd to 50 in step 4; AddGold tops it to 999. A replayed sandbox correctly re-grants. CONFIRMED.

### 7. Shadow-var scan (C4457/58/59) — PASS (clean)
Scanned every new local/param in the diff against inherited reflected UPROPERTYs:
- `InitGame(MapName, Options, ErrorMessage)` — engine's canonical base signature; `Options` ≠ the base's `OptionsString` member, no reflected `Options` exists. No shadow.
- `GrantSandboxStartingGold` locals `SiegeGameState`, `BlueState` — deliberately NOT `GameState` (which would shadow `AGameModeBase::GameState`); neither name matches any inherited reflected member.
- `StartSandboxMatch` locals `Defaults`, `Arena`, param `WorldContextObject` — no reflected collisions (same as the proven-clean StartMatch).
- `AddGold(int32 Amount)` — no reflected `Amount`.
- No local named `Owner`/`Controller`/`PlayerState`/`Instigator`/`Slot`/`Team`/`GameState` introduced. Clean — the project's #1 compile-breaker is absent.

### 8. UE 5.8 API / includes / macros — PASS
- `UGameplayStatics::HasOption(Options, TEXT("Sandbox"))` and `OpenLevelBySoftObjectPtr(WorldContextObject, Arena, /*bAbsolute*/ true, TEXT("Sandbox=1"))` — valid UE 5.8 signatures (4th param is `FString Options`). `Kismet/GameplayStatics.h` included (cpp:10).
- `FTimerManager::SetTimerForNextTick(UserClass*, method)` — valid; `TimerManager.h` included (cpp:22). `GrantSandboxStartingGold` correctly a plain member (no `UFUNCTION` needed for the method-pointer overload).
- `StartSandboxMatch` / `AddGold` / `InitGame` header↔cpp signatures all consistent; `UFUNCTION`/`UPROPERTY` specifiers present and correct; `SandboxStartingGold` is `EditDefaultsOnly` with `ClampMin=0`.
- No deprecated/removed APIs. No null-deref on `WorldContextObject` (guarded first in StartSandboxMatch).

## Rulings on NEW flagged decisions (beyond the two pre-ruled)
- **Flag 3 (re-grant on Play Again, not literally "once"): ACCEPT.** Within the accepted AddGold scope; `PlayAgain`'s ResetGold drops every PS to 50, so a re-grant is required for a replayed sandbox to remain a usable test bench. Correct and intentional.
- **Flag 4 (deferred next-tick initial grant): ACCEPT.** The one-tick defer is the correct fix for the PS-seed race; the callback is fully null-safe. No UAF.

## Non-blocking NITs (no action required pre-compile)
- **NIT** SiegeGameMode.h:271 — `SandboxStartingGold = 9999` effectively lands at 999 after the MaxGold clamp (per accepted ruling 2). The doc comment and the runtime log already state this, so it is not misleading in practice; a designer could set it to `999` for literal truth, but 9999 is harmless. No change required.
- **NIT** SiegeGameMode.cpp:73 — the InitGame log prints the pre-clamp `SandboxStartingGold` (9999); the actual granted value (999) is correctly logged later in GrantSandboxStartingGold. Cosmetic only.

## Notes for build-master (TASK-073)
- PASS — clear to compile + editor-bounce + PIE. Nothing here should touch the Play-vs-Bot path; a quick `git diff` byte-check on `StartMatch` is the only residual (read-verified unchanged, but the diff is the authoritative confirmation).
- Expect at match start (sandbox): `SpawnBot skipped` log, then next tick `granted the Blue player 9999 starting gold (now 999 ...)`. PIE-verify Blue starts at 999, no bot exists, Castle_Red destroy → Victory, and a sandbox Play Again re-lands Blue at 999 with still no bot.
- Non-sandbox regression: normal Play-vs-Bot should be byte-for-byte as before (bot spawns, Blue starts at 50).
