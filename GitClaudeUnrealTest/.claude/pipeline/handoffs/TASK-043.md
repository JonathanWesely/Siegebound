# TASK-043 handoff — Multi-team economy (PlayerState Team tag + GetPlayerStateForTeam + miner team-resolution)

**Status:** ready-for-qa · files-only (no compile, no editor, no Git) · M3 wave 1 on `main`, parallel with TASK-042.

## What this enables
Two coexisting economies in one match — the player's Blue `ASiegePlayerState` and (once TASK-045 lands) the bot's Red `ASiegePlayerState`. Each team's miners now resolve and feed the correct economy. Before this, the miner assumed the single/first player state (M2 had exactly one).

## Files touched (4 pairs)
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h` — new `Team` tag + `GetTeam()`/`SetTeam()`.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameState.h` / `.cpp` — new `GetPlayerStateForTeam(ETeamId)`.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` / `.cpp` — `InitNewPlayer` override tags the local player's PS `Team=Blue`.
- `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h` / `.cpp` — economy resolution now goes through `GetPlayerStateForTeam(Team)`.

No other files touched. **Did NOT** touch TASK-042's files (`HeroCharacter.*`, `SummonedUnit.*`).

## Change 1 — `ASiegePlayerState::Team`
- New `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Siegebound|Team") ETeamId Team = ETeamId::Blue;` plus public `GetTeam()` (BlueprintPure) and `SetTeam(ETeamId)` (BlueprintCallable). Added `#include "Siegebound/TeamId.h"`.
- **Identity, not economy state:** `Team` is NOT reset by `ResetEconomy()`/`ResetGold()` — Play Again keeps each side's team. Default `Blue` means a single-PS world (M2) resolves the local economy for Blue with zero behavior change.
- Not marked `Replicated` — the project is local-only through M7 (per `SiegeGameState.h` note); M8 multiplayer must revisit.

## Change 2 — `ASiegeGameState::GetPlayerStateForTeam(ETeamId Team) const`
- Iterates `PlayerArray`, returns the first `ASiegePlayerState` whose `GetTeam()` matches; `nullptr` + Warning log if none. Loop var is `IterPlayerState` (never `PlayerState`); the match temp is `SiegePS`.
- Header adds `#include "Siegebound/TeamId.h"` (ETeamId param by value) + fwd-decl `class ASiegePlayerState;`. Cpp adds `#include "GameFramework/PlayerState.h"` + `#include "Siegebound/SiegePlayerState.h"`.
- Marked `BlueprintCallable` (not Pure) because it has a logging side-effect; `const`, so it is still node-usable and C++-callable everywhere.

## Change 3 — `ASiegeGameMode::InitNewPlayer` (player PS = Blue at creation)
- Overrides `AGameModeBase::InitNewPlayer`; after `Super`, does `NewPlayerController->GetPlayerState<ASiegePlayerState>()->SetTeam(ETeamId::Blue)` (null-safe). InitNewPlayer is the canonical per-player-login creation hook (runs for the local player in PIE/standalone; the engine itself dereferences `PlayerState` there, so it is valid post-Super).
- The default is already `Blue`, so this is belt-and-braces for the player; the M2 economy is untouched.
- **Bot NOT implemented here** — left the explicit forward-ref: `// TASK-045: bot PS Team=Red where ASiegeGameMode spawns the ASiegeBotController (bWantsPlayerState=true)`. InitNewPlayer only runs for real player logins, so it is deliberately not the bot's tagging site.

## Change 4 — `AMinerUnit` economy resolution
- `ResolveOwningPlayerState()` rewritten: get `World->GetGameState<ASiegeGameState>()`, then `SiegeGameState->GetPlayerStateForTeam(Team)`. Dropped the old M2 `if (Team != Blue) → untracked` branch and the old direct `PlayerArray` iteration entirely.
- Both the gold-node choice (`FindNearestSameTeamGoldNode`, uses `Team`) and the economy resolution (uses `Team`) key off the same `Team`, so a Red miner walks to `GoldNode_Red` AND feeds the Red economy; Blue → `GoldNode_Blue` + Blue economy. Death bookkeeping (`RemoveMinerIncome`/`UnregisterMinerAlive`) goes to the SAME `CachedOwnerState` it registered with (unchanged), so killing a Red miner drops only the bot's rate.
- Added `#include "Siegebound/SiegeGameState.h"`. Kept the now-unused `GameStateBase.h`/`PlayerState.h` includes (harmless; minimal diff).
- Updated the stale doc comments that claimed "ASiegePlayerState carries no team field / Red untracked until M3" (class member doc, `bWarnedNoOwnerState` doc, and the arrival "no owner state" warning text).

## How M2 single-player (Blue) behavior is preserved byte-for-byte
- With only the Blue PS present, `GetPlayerStateForTeam(Blue)` returns that one Blue state — the exact same object the M2 miner's "first `ASiegePlayerState`" returned. Register/AddMinerIncome/RemoveMinerIncome/Unregister all run against it identically.
- A miner only spawns mid-match (player needs 8 gold, so ~4s+ in) — the Blue PS is long-since in `PlayerArray`, so resolution succeeds on the **first** call: the arrival poll never re-queries and `GetPlayerStateForTeam` never logs on the Blue happy path. Log output is identical to M2 (none).
- `Team` default `Blue` + not reset on Play Again = the local economy stays Blue across resets.

## C4458 shadow sweep (the M2 trap — CONVENTIONS coding law)
- New member `Team` on `ASiegePlayerState`: `APlayerState` has NO reflected `Team` member → not a shadow (and it is a member, not a local/param/loop var).
- Param `Team` on `GetPlayerStateForTeam`: `AGameStateBase` has NO `Team` member → safe (verified per the task note). Loop var `IterPlayerState`, temp `SiegePS` — safe.
- `InitNewPlayer`: locals `ErrorMessage`, `SiegePS` — safe; the base param names (`NewPlayerController`/`UniqueId`/`Options`/`Portal`) are the override's own params, not shadows.
- `MinerUnit::ResolveOwningPlayerState`: locals `World`, `SiegeGameState` — both names already used elsewhere in the module and compiled clean in M2 (`SiegePlayerState.cpp` uses local `SiegeGameState`; this same `.cpp` uses local `World`). No inherited reflected member is named either.

## For QA to scrutinize
1. **`GetPlayerStateForTeam` logs unconditionally when none is found** (per the spec's "null + log if none"). The miner does NOT duplicate that log. In every real Blue/Red flow the economy exists before any miner spawns, so the null path is never taken and there is no poll-spam. It only fires for a genuinely mis-teamed consumer (a miner for a team with no PS) — surfacing a real bug. If QA prefers a one-shot/rate-limit on a pure accessor, flag it — I kept it literal to the spec.
2. **`InitNewPlayer` as the tagging hook** — confirm it is the right per-player creation seam vs. `PostLogin`/`SetPlayerDefaults`. Chosen because it runs once at creation, for local players, with `PlayerState` valid post-Super. `SetPlayerDefaults` (already overridden) runs per-pawn/per-respawn — semantically wrong for an identity tag.
3. **`BlueprintCallable const` on `GetPlayerStateForTeam`** — intentional (log side-effect); confirm the ETeamId param is fine (it is — the enum-param limitation is MCP BP-authoring only, not C++ UFUNCTION).
4. Bot side is explicitly deferred to TASK-045 (forward-ref comment in `SiegeGameMode.cpp`) — no bot code here.

## Build note for TASK-051 (build-master)
Header changes to `SiegePlayerState.h`, `SiegeGameState.h`, `SiegeGameMode.h`, `MinerUnit.h` (new UPROPERTY/UFUNCTION + a virtual override) require UHT + a real recompile — not hot-reload-friendly. Batch with the rest of M3.
