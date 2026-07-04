# QA Report — TASK-043
Verdict: PASS

Multi-team economy: PlayerState Team tag + GetPlayerStateForTeam + miner team-resolution (C++). UE 5.8, pre-compile review. Not compiled — this is the pre-compile gate.

Counts: 0 BLOCKER · 1 WARN · 2 NIT

## Findings

- [WARN] MinerUnit.cpp:187-190 / SiegeGameState.cpp:96-100 — The miner's 0.25 s upkeep poll (`UpdateMining`) re-runs `TryRegisterWithOwnerState` → `ResolveOwningPlayerState` → `GetPlayerStateForTeam(Team)` every tick while `!bRegisteredAlive`. `GetPlayerStateForTeam` logs a Warning unconditionally on the not-found path, and the miner-side one-shot guard `bWarnedNoOwnerState` only covers the "no GameState" branch — NOT the "no PS for this team" branch (the comment at MinerUnit.cpp:351-355 defers that log to the accessor). So a genuinely mis-teamed miner (or a delayed bot-PS creation) would emit ~4 Warning lines/sec indefinitely. This does NOT fire in the designed M2/M3 flows (Blue PS present from match start; the M3 bot's Red PS exists before any Red miner spawns, TASK-045), does not crash, and does not touch M2 behavior. Suggested fix (optional, non-blocking): add a one-shot guard on the miner's null-PS-resolution path (mirror `bWarnedNoOwnerState`) or rate-limit the log inside `GetPlayerStateForTeam`. Acceptable to ship as spec-literal.

- [NIT] SiegeGameMode.cpp:119 — `SiegePS->SetTeam(ETeamId::Blue)` is redundant with the `Team = ETeamId::Blue` default (both code and handoff call it belt-and-braces). Harmless; keep — it establishes the explicit tagging pattern that TASK-045 mirrors for the bot's Red PS.

- [NIT] MinerUnit.cpp:8-9 — `GameFramework/GameStateBase.h` is now effectively redundant (resolution goes through `SiegeGameState.h`, which pulls `AGameStateBase`); `PlayerState.h` is still legitimately used (`ASiegePlayerState`). Handoff acknowledges keeping both for minimal diff. Harmless.

## Rulings on the 3 flagged decisions

1. **Unconditional log in `GetPlayerStateForTeam` — ACCEPT (spec-literal).** The log fires ONLY on the not-found path (the happy path returns early at SiegeGameState.cpp:86 with no log), so it is NOT per-call — it already implements the "log only on the null path" option. It correctly surfaces a genuinely mis-teamed consumer instead of silently untracking it. The only residual is the miner's poll re-invoking it on a persistent-failure path (see WARN above), which never occurs in the designed flows. No change required.

2. **`InitNewPlayer` as the tagging seam — ACCEPT (correct, robust hook).** Runs once per real player login, with `PlayerState` valid post-`Super` (the engine itself dereferences it there), and the code null-checks `GetPlayerState<ASiegePlayerState>()`. Signature matches UE 5.8 `AGameModeBase::InitNewPlayer(APlayerController*, const FUniqueNetIdRepl&, const FString&, const FString& = TEXT(""))` exactly, and it calls and returns `Super::InitNewPlayer(...)`. `SetPlayerDefaults` (already overridden) would be wrong — it runs per-pawn/per-respawn, semantically incompatible with an identity tag. Because the PS default is already Blue and the economy never reads `Team` (only lookups do), timing relative to economy start is non-critical; the call is a correct belt-and-braces. The bot's Red PS is deliberately NOT tagged here — InitNewPlayer only runs for real logins; TASK-045 tags it at the bot's spawn site (forward-ref at SiegeGameMode.cpp:123-126). Correct.

3. **`BlueprintCallable`-const with `ETeamId` param — ACCEPT.** `BlueprintCallable`+`const` is valid and node-usable; choosing it over `BlueprintPure` because of the log side-effect is correct (pure nodes can be invoked multiple times and should be side-effect-free). The `ETeamId` param is fine in a C++ UFUNCTION — the "no enum params" rule in CONVENTIONS is an MCP Blueprint-authoring restriction only, not a UHT/C++ one; `ETeamId` is `UENUM(BlueprintType)` (TeamId.h:13-18) so it is a valid reflected BP pin. Returning a non-const `ASiegePlayerState*` from a const method (a pointer held in `PlayerArray`, not part of `this`) is legal and warning-free.

## M2 Blue-side economy — byte-for-byte preservation: CONFIRMED

- **Same resolved object:** with only the Blue PS in `PlayerArray`, `GetPlayerStateForTeam(Blue)` returns that single `ASiegePlayerState` — the identical object M2's "first player state" lookup returned. A player miner's `Team` is Blue (`ASummonedUnit::Team` default Blue, set by the spawner's `InitUnit`), so `GetPlayerStateForTeam(Blue)` resolves it. The dropped M2 `if (Team != Blue) → untracked` branch never executed for a Blue miner, so removing it is a no-op for Blue.
- **No register/income/death split:** `TryRegisterWithOwnerState` latches `CachedOwnerState = OwnerState` and calls `RegisterMinerAlive` on it; arrival `AddMinerIncome`, and death `RemoveMinerIncome`/`UnregisterMinerAlive` (EndPlay/Destroyed), ALL route through the same `CachedOwnerState.Get()`. Resolution is latched-once and null-safe: a miner with no matching-team PS logs (arrival `bWarnedIncomeSkipped`) and idles — no crash.
- **`Team` is identity, never economy state:** `ResetEconomy` (touches only `AliveMinerCount`/`MinerIncomeCount` + broadcasts), `ResetGold` (gold + timer), and `PlayAgain` (iterates PS → ResetEconomy/ResetGold/ResumeIncome) never write `Team`. `Team` defaults Blue and survives Play Again — the local economy stays Blue across resets.
- **Single Team drives both node choice and economy:** `FindNearestSameTeamGoldNode` (`Node->GetTeam() != Team`) and `ResolveOwningPlayerState` (`GetPlayerStateForTeam(Team)`) both key off the same inherited `ASummonedUnit::Team`, so a Red miner walks to `GoldNode_Red` AND feeds only the Red economy; killing a Red miner drops only the bot's rate.
- **No change to accrual / overtime / pause / resume / reset paths** — those files are unchanged aside from the additive `Team` member/getter/setter; the gold rate composition never reads `Team`.

## Standard checks
- **Deprecated UE 5.8 APIs:** none. `World->GetGameState<>()`, `PlayerArray` (TObjectPtr) iteration, `GetPlayerState<>()`, `Cast<>`, `MoveToActor`, `GetPathFollowingComponent`/`GetMoveStatus`/`GetMoveGoal`, `FMath::FloorToInt32`, `FVector::Dist2D`, `FUniqueNetIdRepl`, `TActorIterator` are all current.
- **InitNewPlayer override signature:** correct for UE 5.8 (see ruling 2); calls Super; header default arg `= TEXT("")` matches base, .cpp omits it (correct).
- **Null-safety:** PlayerArray Cast-guarded; `GetPlayerState<>()` null-checked; `GetGameState<>()` null-checked; `CachedOwnerState.Get()` checked before every use.
- **C4457/C4458/C4459 shadow scan: CLEAN.** No local/param/loop var shadows an inherited reflected UPROPERTY. New member `ASiegePlayerState::Team` — `APlayerState`/`AInfo`/`AActor` carry no reflected `Team` (not a shadow). Param `Team` on `GetPlayerStateForTeam` — `AGameStateBase` has no `Team` member; loop var `IterPlayerState` (not `PlayerState`), temp `SiegePS` — safe. `InitNewPlayer` locals `ErrorMessage`/`SiegePS` — safe; the base-signature param names are the override's own, not shadows. `MinerUnit::ResolveOwningPlayerState` locals `World`/`SiegeGameState` — no inherited reflected member of that name (used in M2, compiled clean).
- **Header/cpp consistency:** all four declarations match definitions; includes correct (TeamId.h where ETeamId is used by value; SiegePlayerState.h in the two consumers; fwd-decls in headers).
- **Naming vs names: block:** `ASiegePlayerState::Team`, `ASiegeGameState::GetPlayerStateForTeam`, `ASiegeGameMode` PS tagging, `AMinerUnit` resolution via `GetPlayerStateForTeam`, `ETeamId` — all match TASK-043's names block character-for-character.

## Notes for build-master
- HeaderChanges to SiegePlayerState.h / SiegeGameState.h / SiegeGameMode.h / MinerUnit.h add a UPROPERTY, two UFUNCTIONs, a new function, and a virtual override → requires UHT + full recompile (not hot-reload). Batch with the rest of M3 (TASK-051), as the handoff notes.
- No engine/content dependency introduced by this task (bot Red PS is deferred to TASK-045); this compiles standalone against the current module.
