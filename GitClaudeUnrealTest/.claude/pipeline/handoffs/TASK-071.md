# TASK-071 handoff — Sandbox bot-spawn gate + generous economy (C++, files-only)

**Status:** ready-for-qa
**Author:** gameplay-programmer
**Base:** main @ 5c1fcb7 — files only, NOT compiled, NOT committed, no editor touched.

## Summary
Adds a dev/test "Sandbox (No Bot)" affordance to `ASiegeGameMode`: a level-open URL option
`?Sandbox=1` (NOT a GameInstance) latches `bSandboxMatch` in `InitGame`; `SpawnBot()` early-returns
so ZERO `ASiegeBotController` and no Red bot `ASiegePlayerState` exist; the Blue player is granted a
generous starting pile. Castle_Red remains as a static target dummy so the full 22-card roster is
playable and the win condition still fires. Play-vs-Bot (`StartMatch`) is untouched.

## Files touched
1. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h`
2. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`
3. **`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h`** — see FLAGGED DECISION #1
4. **`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp`** — see FLAGGED DECISION #1

## Audit-confirm (step 0) — CONFIRMED
- `ASiegeGameMode::SpawnBot()` (now ~line 758, called from `BeginPlay` ~line 92) spawns the single
  `ASiegeBotController` into the `BotController` member; the Red bot `ASiegePlayerState` comes from the
  controller's `bWantsPlayerState` (tagged Team=Red via `GetBotPlayerState()->SetTeam(...)`). Confirmed.
- No `USiegeGameInstance` exists and none was introduced. Mechanism is the level-open option only.

## What changed in SiegeGameMode.h / .cpp
- **`bool bSandboxMatch = false;`** (private member) — the latch.
- **`virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;`**
  — calls `Super::InitGame(...)` FIRST, then `bSandboxMatch = UGameplayStatics::HasOption(Options, TEXT("Sandbox"))`.
  Token string exactly `"Sandbox"` (CONVENTIONS). Runs once, before BeginPlay/SpawnBot; persists for the
  world's life (PlayAgain never re-runs InitGame → sandbox stays sandbox).
- **`SpawnBot()` gate:** early-returns (with a Log line) when `bSandboxMatch` — no `ASiegeBotController`
  spawned, no Red PlayerState seeded, before any existing logic. Placed as the FIRST statement.
- **`static void StartSandboxMatch(const UObject* WorldContextObject)`** (`UFUNCTION BlueprintCallable`,
  `meta=(WorldContext=...)`) — mirrors `StartMatch` verbatim (null-safe world-context + CDO ArenaLevel
  resolve + logs), but calls
  `UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, Arena, /*bAbsolute*/ true, TEXT("Sandbox=1"))`.
- **`UPROPERTY(EditDefaultsOnly, Category="Siegebound|Sandbox", meta=(ClampMin="0")) int32 SandboxStartingGold = 9999;`**
  (with the `// dev sandbox — full roster freely playable` intent captured in the doc comment).
- **`void GrantSandboxStartingGold();`** (private) — resolves the Blue PS via
  `ASiegeGameState::GetPlayerStateForTeam(ETeamId::Blue)` (null-safe, logs + no-ops if unresolved) and
  calls `BlueState->AddGold(SandboxStartingGold)`. Does NOT touch the rate.
  - Called **deferred one tick** from `BeginPlay` (`GetWorldTimerManager().SetTimerForNextTick(...)`) so the
    Blue PS's own `BeginPlay` (which seeds gold to 50 via `ResetGold`) has already run — a synchronous grant
    could be clobbered by the later PS seed.
  - Called **synchronously** from `PlayAgain` (new step 4c, after the per-PS ResetEconomy/ResetGold loop and
    the ResetBot step) so a sandbox Play Again restores the pile (the PS already exists there → no race).

`StartMatch` — NOT touched (byte-identical). The only change to the non-sandbox path is the new `InitGame`
override, which calls `Super` first and merely sets `bSandboxMatch=false`; no match behavior changes.

## Red-PS null-safety audit (step 2 — every reader of the now-absent Red ASiegePlayerState)
Key fact that makes this safe: **the codebase already ran without a Red PS in all of M2** — the bot was
added in M3 (TASK-045). `GetPlayerStateForTeam(Red)` returning `nullptr` is *explicitly documented* in
`SiegeGameState.cpp` as "the normal M2 result (no bot)". Sandbox reproduces that supported configuration
in an M4-roster world. Each reader, and how it is guarded:

| # | Reader | Guard when Red PS absent |
|---|--------|--------------------------|
| 1 | `SiegeGameMode::SpawnBot()` → `GetBotPlayerState()` | Unreachable — my `bSandboxMatch` early-return returns before the spawn + PS tag. |
| 2 | `SiegeGameMode::FreezeWorldAtMatchEnd()` — `GameState->PlayerArray` loop calling `PauseIncome()` | Iterates whatever PlayerArray holds (Blue-only in sandbox); no per-team assumption. The bot stop is `if (IsValid(BotController))` → false → skipped. |
| 3 | `SiegeGameMode::PlayAgain()` — PlayerArray ResetEconomy/ResetGold/ResumeIncome loop | Same generic PlayerArray loop (Blue-only). `ResetBot` is `if (IsValid(BotController))` → false → skipped. |
| 4 | `SiegeBotController::EvaluateDecisions()` / `ResetBot()` → `GetBotPlayerState()` | The bot controller never exists in sandbox, so its decision timer never starts → these never run. |
| 5 | `AMinerUnit::ResolveOwningPlayerState()` → `GetPlayerStateForTeam(Team)` | Resolves the miner's OWN team. Blue miners resolve the Blue PS. Red miners can only come from the bot playing a Miner — which never happens in sandbox. It also null-checks `if (!OwnerState)` and one-shots the warning. |
| 6 | `ADeepMine::ResolveOwningPlayerState()` → `GetPlayerStateForTeam(Team)` | Identical pattern to #5; null-checks and one-shots. Red Deep Mines only come from the bot → never in sandbox. |
| 7 | `ASiegeGameState::GetPlayerStateForTeam(Red)` itself | Already returns `nullptr` + one Warning log for the "no Red PS" case — the documented normal M2 path. |

- **Win condition:** `OnCastleDestroyedHandler` reads the destroyed **castle's Team enum**
  (`CastleTeam == ETeamId::Red ? Blue : Red`), never a PlayerState. `Castle_Red` is level-placed and still
  present; Blue units target it as a Red actor (team/actor-based targeting, PS-independent — as in M1 which
  had no bot). Destroying `Castle_Red` → Winner=Blue → Victory. Verified fires without a Red PS.
- **Overtime / rate application:** applied per-`ASiegePlayerState` inside `GetGoldRate()` reading the shared
  `ASiegeGameState` latch. Freeze/PlayAgain iterate PlayerArray generically. No Red PS needed.
- **Income tick:** each PS ticks its own income on its own timer. Blue ticks; there is simply no Red PS to
  tick. Correct.
- Broadened grep for other enemy-PS readers (`EnemyState`/`RedState`/`ETeamId::Red`): all `ETeamId::Red`
  hits are **Team-enum** comparisons (team-material selection on Castle/Building/Projectile/SummonedUnit,
  bot spawn geometry, the winner calc) — **none dereference a Red PlayerState.** No HUD/controller
  enemy-PS reader exists.

Conclusion: nothing dereferences the Red PS unconditionally; no crash/hang path with only a Blue PS present.

## Shadow-var self-scan (step 5 — C4457/58/59)
- `InitGame` params `MapName` / `Options` / `ErrorMessage` are the engine's canonical override names; none
  shadow an inherited reflected UPROPERTY (`Options` ≠ the base's `OptionsString`). No NEW locals added there.
- `GrantSandboxStartingGold` locals: `SiegeGameState` (deliberately NOT `GameState` — that would shadow the
  inherited `AGameModeBase::GameState` UPROPERTY; this exact local name is already used safely in `PlayAgain`)
  and `BlueState`. Neither shadows anything.
- `StartSandboxMatch` locals: `Defaults`, `Arena` — same as `StartMatch`, already proven clean.
- `ASiegePlayerState::AddGold` param `Amount` — no reflected `Amount`. Clean.
- No local named `Owner`/`Controller`/`PlayerState`/`Instigator`/`Slot`/`Team` introduced anywhere.

## FLAGGED DECISIONS FOR QA
1. **SCOPE DEVIATION — added a minimal `ASiegePlayerState::AddGold(int32)` (2 files beyond "SiegeGameMode only").**
   The dispatch said touch `SiegeGameMode.h/.cpp` only, but step 4 requires granting a **lump** of gold
   "**through the existing gold API**", "**do NOT hardcode a raw field write**", and "**keep the normal gold
   rate**". I verified the existing public gold API has **no lump-grant entry point** — only `ResetGold()`
   (→ fixed `StartingGold`=50) and `AddIncome()` (which permanently changes the **rate**). Those cannot
   express "grant N gold now, rate unchanged". `StartingGold` is `protected` + `EditDefaultsOnly` (not
   writable cross-class, and would be a raw-config write). So satisfying step 4's substantive requirements
   is impossible in `SiegeGameMode`-only scope.
   - Resolution chosen: a minimal, additive, non-breaking public `AddGold(int32)` — the sibling of
     `SpendGold`, routing through the same private `SetGold()` choke point (so the `[0, MaxGold]` clamp and
     the `OnGoldChanged` broadcast both apply — NOT a raw field write). It does not touch the rate. Its
     **sole caller** is the sandbox grant; the normal match never calls it, so Play-vs-Bot is unchanged.
   - Alternatives rejected: (a) `AddIncome`+tick+`RemoveIncome` — delays the grant one tick AND flashes a
     huge `+N/s` on the HUD, violating "keep the normal rate"; (b) leave step 4 unimplemented — fails
     acceptance ("Blue starts with SandboxStartingGold"); (c) raw field write — explicitly forbidden.
   - **QA/orchestrator please rule:** accept the `AddGold` deviation, or direct me to revert
     `SiegePlayerState.*` and pursue an alternative. Precedent for flagged, well-reasoned deviations:
     TASK-011 byte-param (orchestrator-approved).
2. **MaxGold clamps 9999 → 999.** `ASiegePlayerState::MaxGold = 999` is the hard cap, so `SandboxStartingGold
   = 9999` effectively lands the Blue player at **999** gold. Still a full generous pile for the 22-card
   roster (most expensive cards are single/low-double-digit cost). I did NOT change `MaxGold` (out of scope,
   and it would affect the normal match). Captured in the UPROPERTY doc comment + a runtime Log line. Flagged
   in case the intent was literally 9999 (which would require a sandbox-conditional `MaxGold` in
   `ASiegePlayerState` — a larger change).
3. **Sandbox re-grants on Play Again (not literally "once").** Step 1 says the flag persists so PlayAgain
   stays sandbox; step 4 says grant "once at match start". I re-grant on each sandbox Play Again (step 4c),
   because PlayAgain's `ResetGold` drops every PS back to 50 — without a re-grant a replayed sandbox match
   would start broke, defeating the test bench. Deliberate; confirm this is desired.
4. **Deferred (next-tick) initial grant.** The BeginPlay grant is scheduled via
   `SetTimerForNextTick` so it runs AFTER the Blue PS seeds its own gold to 50 (a synchronous grant in
   BeginPlay risks being clobbered). If, in some flow, the Blue PS is not resolvable on the next tick,
   `GrantSandboxStartingGold` logs a warning and no-ops (null-safe) — the player would stay at 50 rather
   than crash. Standard for PIE single-player; noted for completeness.

## Acceptance mapping
- `?Sandbox=1` (via `StartSandboxMatch`) → `InitGame` latches → `SpawnBot` early-returns → **zero
  `ASiegeBotController` at BeginPlay, no bot ever plays a card.** ✔
- Blue starts with `SandboxStartingGold` (clamped to MaxGold=999). ✔ (see flag #2)
- Full hand/roster playable vs `Castle_Red` (static target dummy; win condition still fires). ✔
- Sandbox PlayAgain does NOT spawn a bot (PlayAgain has no SpawnBot call; ResetBot is IsValid-guarded). ✔
- Without the option (`StartMatch` / Play-vs-Bot): `bSandboxMatch=false`, bot spawns and behaves exactly as
  today; `StartMatch` byte-identical. ✔
- Nothing hardcoded that lives in DT_Cards. ✔

## Do NOT (respected)
No editor, no compile, no Git, no TASKBOARD edit. build-master (TASK-073) owns the compile + editor-bounce
+ PIE verify + commit; TASK-072 authors the `Btn_Sandbox` button against the compiled `StartSandboxMatch`.
