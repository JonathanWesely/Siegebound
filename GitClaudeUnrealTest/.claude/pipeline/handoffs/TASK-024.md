# TASK-024 Handoff — Match clock, overtime, economy v2, match-end freeze, FellOutOfWorld respawn (C++)

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor/MCP, no Git, no TASKBOARD/CONVENTIONS edits per dispatch — orchestrator proxies the status flip to ready-for-qa; M2 wave 4 batch pattern)

## Files

1. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameState.h` (NEW) — `ASiegeGameState : AGameStateBase`
2. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameState.cpp` (NEW)
3. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h` (changed) — economy v2
4. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp` (changed)
5. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` (changed) — freeze + PlayAgain v2 docs, FreezeWorldAtMatchEnd decl, timer-policy amendment
6. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` (changed)
7. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` (changed) — FellOutOfWorld override decl
8. `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` (changed) — FellOutOfWorld body

**NOT touched (verified/deliberate):** `SiegePlayerController.h/.cpp` (OWNED by the concurrent TASK-023 — zero new symbols depended on; see decision 2), `SummonedUnit.*`, `Building.*`, `Tower.*`, `Projectile.*`, `DamageTypes.*`, `DeckComponent.*`, `CardRow.h`, `Castle.*`, `CardHandWidget.*` (frozen qa-passed contracts — include-and-call only), `Build.cs` (no new module deps — GameStateBase/TimerManager/actor iteration are Engine-level), no config/ini/board/conventions edits.

## What was built (spec items 1-5)

**(1) ASiegeGameState (new):** `MatchClockSeconds` (float) accumulates actor-tick DeltaSeconds from match start; `OvertimeStartSeconds = 420.f` UPROPERTY (`// GDD §3.2 — 7:00`); at the threshold `bOvertimeActive` latches and `OnOvertimeStarted` broadcasts EXACTLY once (re-armed only by ResetClock); `OnMatchClockChanged(int32 WholeSeconds)` once per new whole second; `StopClock()` freezes, `ResetClock()` → 0 + overtime cleared + running + unconditional `OnMatchClockChanged(0)` (reset-path law). BlueprintPure seed getters `GetMatchClockSeconds()` / `IsOvertimeActive()`. `ASiegeGameMode` constructor sets `GameStateClass = ASiegeGameState::StaticClass()`.

**(2) ASiegePlayerState v2 (rate-composed income):** `GetGoldRate() = (overtime ? GoldPerTick * OvertimeIncomeMultiplier : GoldPerTick) + MinerIncomeCount * MinerGoldPerTick` — overtime read LIVE off the game state every call. `HandleGoldTick` now adds `GetGoldRate()` (M1 accrual byte-equivalent: 2/tick with no overtime/miners) and is gated on `bIncomePaused`. Full new API per the names block: `AddMinerIncome/RemoveMinerIncome` (ARRIVED-miner income, §3.3), `RegisterMinerAlive/UnregisterMinerAlive/GetAliveMinerCount/CanAddMiner` (ALIVE-miner cap bookkeeping, `MaxActiveMiners = 6` `// GDD §3.3`), `GetGoldRate`, `PauseIncome/ResumeIncome`, `ResetEconomy`; delegates `FOnGoldRateChanged(int32 NewRate)` / `FOnMinerCountChanged(int32 AliveCount)` (members `OnGoldRateChanged` / `OnMinerCountChanged`, BlueprintAssignable) firing on every actual change + unconditionally on ResetEconomy. BeginPlay binds `HandleOvertimeStarted` to the game state's `OnOvertimeStarted` (AddUniqueDynamic; unbound in EndPlay) so the 2→4 rate change broadcasts the moment overtime starts. `ResetGold()` is BYTE-IDENTICAL to M1 (still restarts the income timer — qa/TASK-005 major-1 law).

**(3) Match-end freeze:** new private `ASiegeGameMode::FreezeWorldAtMatchEnd()`, called from `OnCastleDestroyedHandler` after the latch/respawn-cancel and BEFORE `HandleMatchEnd` (the Victory screen rises over an already-frozen world). Order: FreezeAI every ASummonedUnit → silence every ATower (see carry-forward 2) → destroy every in-flight AProjectile (carry-forward 1) → PauseIncome on every ASiegePlayerState → StopClock. Closes the qa/TASK-006-report.md finding-2 TODO(M2).

**(4) PlayAgain v2** (M1 steps preserved, new steps interleaved): 1. own-timer clear (unchanged, still before ResetGold — law) → 2. destroy units (unchanged) → **2b. destroy all ABuilding** (collect-then-destroy; ATower::EndPlay clears its own fire timer synchronously) → **2c. destroy all AProjectile** → 3. ResetCastle (unchanged) → **3b. ASiegeGameState::ResetClock()** (MUST precede 4 — economy re-derives against the cleared overtime latch) → 4. per player state **ResetEconomy() + ResetGold() + ResumeIncome()** (in that order) → 5. re-arm + hero restore (unchanged) → 6. HandleMatchReset (unchanged) **+ deck reset via `FindComponentByClass<UDeckComponent>()->ResetDeck()`** (null-safe + warn).

**(5) AHeroCharacter::FellOutOfWorld override:** deliberately does NOT call Super (AActor's default is a bare `Destroy()`); early-outs on `bDead` (the engine re-invokes per movement tick while an actor sits below KillZ); otherwise logs and calls the existing protected `HandleDeath()` — the exact combat-death path (hide, stop movement/input/collision, single `OnHeroDied` broadcast) → the game mode's standard 5 s respawn. TASK-036 supplies KillZ = -2000 + boundary volumes.

## Carry-forward closure

1. **qa/TASK-026-report.md WARN-1 (in-flight projectiles) — CLOSED on both paths.** Match end: `FreezeWorldAtMatchEnd` step 3 collect-then-destroys every `AProjectile` (and steps 1-2 guarantee nothing can spawn a new one afterward). PlayAgain: step 2c repeats the sweep — load-bearing for a MID-MATCH PlayAgain, where live projectiles exist that no match-end freeze ever swept.
2. **qa/TASK-027-report.md WARN-1 (towers fire at frozen units post-Victory) — CLOSED, no residual firing gap.** Mechanism: `World->GetTimerManager().ClearAllTimersForObject(Tower)` per `TActorIterator<ATower>` at match end. Tower.h/.cpp untouched (frozen): the fire loop lives in a PRIVATE `FireTimerHandle` with no public stop hook, so the public FTimerManager per-object surface is the only game-mode-side lever — and it is sufficient: qa/TASK-027 verified the fire loop is the ONLY timer the tower family ever arms, `OnStatsLoaded` is single-fire (`bStatsLoaded`) so nothing can re-arm it, and PlayAgain destroys all buildings anyway. Residual gap: none for firing/damage; see decision 1 for the (currently empty) class of "other timers on a tower" this would also clear.
3. **handoffs/TASK-028.md FreezeAI contract — CONSUMED verbatim.** `for (TActorIterator<ASummonedUnit> It(World); It; ++It) { It->FreezeAI(); }` — exactly the handoff's snippet; relies on its documented idempotence/permanence/bDead-safety. The handoff's reminder that in-flight projectiles are NOT covered by FreezeAI is honored by closure 1.

## Contract for TASK-025 (AMinerUnit consumes ASiegePlayerState)

```cpp
// All BlueprintCallable/Pure on ASiegePlayerState (Siegebound/SiegePlayerState.h):
void  RegisterMinerAlive();     // BeginPlay, exactly once per miner
void  UnregisterMinerAlive();   // death, exactly once — ALWAYS (arrived or not)
void  AddMinerIncome();         // exactly once, on ARRIVAL at the gold node (never on spawn)
void  RemoveMinerIncome();      // death, ONLY if AddMinerIncome had run for this miner
int32 GetAliveMinerCount() const;
bool  CanAddMiner() const;      // TASK-030's play-time cap gate (< MaxActiveMiners = 6)
int32 GetGoldRate() const;      // composed rate; +1 per arrived miner shows here
```
Rules: underflow calls (Remove/Unregister at 0) are refused + logged Error, no state change; over-cap Register and income>alive are Warning diagnostics only (the cap is ENFORCED at play time via CanAddMiner — TASK-030). Delegates fire automatically; a miner killed en route (Unregister without Remove) changes the miner count but not the rate — exactly §3.3.

## Notes for TASK-033 (HUD v2) — seed-then-bind pairs

- Gold rate "+N/s": seed `GetGoldRate()`, bind `OnGoldRateChanged` (ASiegePlayerState).
- Miner "x/6": seed `GetAliveMinerCount()` (cap: `MaxActiveMiners` is protected — the literal 6 in UMG text is fine, or expose later), bind `OnMinerCountChanged`.
- Overtime indicator: seed `IsOvertimeActive()`, bind `OnOvertimeStarted` (ASiegeGameState).
- Match clock (if shown): seed `GetMatchClockSeconds()`, bind `OnMatchClockChanged(int32 WholeSeconds)`.

## Flagged decisions for QA (please rule on each)

1. **Tower-silencing mechanism = `FTimerManager::ClearAllTimersForObject` per ATower at match end** — the ONE deliberate exception to the M1 "own timers only" policy, and the policy doc in SiegeGameMode.h is amended accordingly (the protected invariant — never touch the income timer's owner; clear-before-ResetGold in PlayAgain — is untouched; PauseIncome is the player state's own API). The qa/TASK-027 WARN-1 alternative "destroy all ABuildings at match END" was deliberately REJECTED: GDD §3.9 places building teardown in Play Again ("full state reset: ... buildings"), and buildings vanishing under the Victory screen contradicts the freeze-in-place aesthetic (units stand frozen; buildings should stand silent). Consequence accepted: ClearAllTimersForObject would also clear any future non-fire timer on a tower (none exist in M2; TASK-035 BPs add nothing per spec) — any future timered ATower feature must account for match-end silencing.
2. **Deck reset reached via `FindComponentByClass<UDeckComponent>()`, not a controller symbol.** SiegePlayerController.h/.cpp are owned by the concurrent TASK-023 (no handoff existed at implementation time), so PlayAgain depends only on the controller's frozen M1 surface (`HandleMatchReset`) plus the frozen TASK-022 component class. SEAM TO RECONCILE AT TASK-023 QA: TASK-023's spec says the controller calls "ResetDeck from the PlayAgain flow" — if it wires ResetDeck into HandleMatchReset, PlayAgain will reset the deck twice. Provably benign (ResetDeck is a full rebuild+reshuffle+redeal; both orders converge on one fresh hand; delegates just fire twice), but one of the two calls can be dropped in a later pass. If TASK-023 does NOT wire it, this call is the only §3.9 deck reset — which is why it stays.
3. **Projectile sweep duplicated in PlayAgain (2c) despite the match-end sweep** — load-bearing for the mid-match PlayAgain (a legal full reset per TASK-006, where no freeze ever ran); no-op cost after a normal match end.
4. **`ResetGold()` left byte-identical** (unconditionally restarts the income timer — M1 law). Pause coherence is achieved instead by the `bIncomePaused` gate inside `HandleGoldTick` plus `ResumeIncome()` in PlayAgain: if anything restarts the timer while paused, ticks fire but accrue nothing.
5. **`GetGoldRate()` reports the CONFIGURED rate while income is paused** (not 0): pausing accrual is a freeze, not a rate change — reporting 0 would fire spurious rate-delegate churn (N→0→N) across match end/PlayAgain and desync the HUD's "+N/s" from the §3.2 configuration it displays.
6. **Overtime is read LIVE in `GetGoldRate()`; the `OnOvertimeStarted` bind exists only to notify rate listeners.** Accrual correctness survives a missed/failed bind (wrong GameStateClass = warn once in BeginPlay, rate stays base — degraded but consistent).
7. **Symbols beyond the names block:** ASiegeGameState getters `GetMatchClockSeconds()`/`IsOvertimeActive()` (BlueprintPure seed hooks — TASK-010/018 precedent; TASK-033 needs both); ASiegePlayerState UPROPERTYs `OvertimeIncomeMultiplier = 2` (`// GDD §3.2`) and `MinerGoldPerTick = 1` (`// GDD §3.3`) per the CONVENTIONS mechanic-rule clause (the spec's "2, or 4" and "+1" become tunables; M1's `GoldPerTick` is reused as the BaseRate rather than adding a duplicate property); private helpers (`FreezeWorldAtMatchEnd`, `RefreshGoldRate`, `GetSiegeGameState`, `HandleOvertimeStarted`).
8. **Clock is actor-tick-driven** (accumulated DeltaSeconds), not a repeating 1 s timer: no drift against real time and no handle for any timer sweep to kill. `OnMatchClockChanged` fires once per NEW whole second — a multi-second hitch broadcasts only the newest value (display semantics), which is my reading of "each elapsed second".
9. **Reset paths broadcast unconditionally** (`ResetClock` → `OnMatchClockChanged(0)`; `ResetEconomy` → both delegates even at base values) — CONVENTIONS "and on reset paths" law, ACastle::ResetCastle precedent.
10. **`FellOutOfWorld` does not call Super and routes to `HandleDeath()`**: the bDead early-out absorbs the engine's repeated below-KillZ re-checks on the hidden corpse; after a match end, a fallen hero stays dead below KillZ until PlayAgain teleports + resets it (the M1 "nothing revives under the end screen" law — RestoreHeroAtStart teleports BEFORE ResetHero, so it never revives below KillZ).
11. **Miner bookkeeping misuse handling:** underflows are Error + refused; over-cap registration and income-exceeds-alive are Warning diagnostics with truthful counting (enforcement is TASK-030's play-time `CanAddMiner` gate, per the M2 ruling — the player state never blocks a spawn that already happened).
12. **Freeze ordering** units → towers → projectiles → income → clock, all inside one game-thread call before `HandleMatchEnd` — no timer can interleave mid-function, so no projectile can spawn between the tower silencing and the projectile sweep.
13. **`Register/UnregisterMinerAlive` broadcast unconditionally** — every non-refused call is an actual change (++ / guarded --), so this IS broadcast-on-actual-change.

## M1 preservation analysis

- **Match end → Victory:** the only insertion is `FreezeWorldAtMatchEnd()` between the existing log and the controller notify. In an M1-shaped world it freezes zero-or-more Blue footmen (spec'd M2 behavior replacing the accepted M1 "units keep fighting" WARN), finds no towers/projectiles, pauses income (spec'd), stops the new clock. Victory screen flow itself is untouched.
- **PlayAgain:** every M1 step is byte-identical and in the same relative order; new steps are no-ops in an M1-shaped world except ResetClock/ResetEconomy broadcasts (no listeners until TASK-033) and ResumeIncome (SetTimer replace microseconds after ResetGold's restart — imperceptible). The qa/TASK-005 major-1 ordering law verifiably holds: the only timer-clearing in PlayAgain is still step 1's own handle, before ResetGold.
- **Hero 5 s respawn:** untouched (HandleHeroDied/RespawnHero/RestoreHeroAtStart byte-identical); FellOutOfWorld is purely additive (previously the engine default Destroy — the "falls forever" carry-over).
- **Income accrual:** `HandleGoldTick` with no overtime/miners/pause adds exactly `GoldPerTick` (2) — value-identical to M1; `SetGold` choke point untouched.
- **GameStateClass swap** (AGameStateBase → ASiegeGameState) is additive-subclass; `PlayerArray` and the game mode's `HasMatchEnded` latch override are unaffected.

## Notes for build-master

- One new class pair (SiegeGameState) — normal UBT pickup, no Build.cs/.uproject changes. Includes consume only frozen qa-passed or M1 headers; compiles independently of TASK-023's in-flight controller changes (no new controller symbols referenced) but runtime deck-reset needs TASK-023's DeckComponent subobject to exist (same TASK-039 batch — the warn path covers any gap).
- PIE checks at TASK-039/040: M1 regression set (gold 50 + ~2/s, Victory once, PlayAgain full reset, hero 5-6 s respawn) must all hold; new: units/towers stop + no projectile lands after Victory; gold and clock frozen under the end screen; PlayAgain → clock 0, rate 2/s, 0 miners, no buildings, fresh 6-card hand; console `SetKillZ`-style or TASK-036 KillZ fall → respawn in 5-6 s. Overtime fast-check: temporarily lower `OvertimeStartSeconds` on the game state (EditDefaultsOnly) rather than waiting 7 minutes.
- TASKBOARD not edited per dispatch — orchestrator proxies `ready-for-qa`.
