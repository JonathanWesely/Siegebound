# QA Report — TASK-057

**Task:** New buildings — Barracks spawner + Deep Mine economy (C++, files only, M4 wave 2)
**Reviewer:** qa-reviewer
**Date:** 2026-07-04
**Verdict:** PASS
**Counts:** 0 BLOCKER · 1 WARN · 3 NIT

Pre-compile review only (code compiles with the M4 batch at TASK-068). Files reviewed:
`Barracks.h/.cpp`, `DeepMine.h/.cpp`, `SiegePlayerState.h/.cpp`, `SiegeGameMode.cpp`, cross-checked
against `Building.h/.cpp`, `CardRow.h`, `SiegeSpawnConstants.h`, `SummonedUnit.h`, `SiegeGameState.h`,
`MinerUnit.cpp` (precedent), and the TASK-057 board spec + CONVENTIONS.

---

## CRITICAL confirmation — M2/M3 miner economy is BYTE-PRESERVED

Verified by symbol trace of every new/changed code path in `SiegePlayerState`:

- **New accumulator is isolated.** `FlatIncomePerTick` is a brand-new private `int32` (VisibleInstanceOnly,
  Transient). `AddIncome` / `RemoveIncome` mutate ONLY `FlatIncomePerTick` then call `RefreshGoldRate(false)`.
  Neither reads or writes `AliveMinerCount`, `MinerIncomeCount`, `MaxActiveMiners`, `CanAddMiner`,
  `GetAliveMinerCount`, `RegisterMinerAlive`, `UnregisterMinerAlive`, `AddMinerIncome`, `RemoveMinerIncome`,
  or any miner delegate. Confirmed the Deep Mine cap-bypass: `ADeepMine` never registers as a miner, so
  `MaxActiveMiners` genuinely does NOT gate it.
- **GetGoldRate identical with zero mines.** New body: `BaseRate + (MinerIncomeCount * MinerGoldPerTick) + FlatIncomePerTick`.
  The base + miner terms are unchanged from M3; only `+ FlatIncomePerTick` was appended. With zero Deep Mines
  `FlatIncomePerTick == 0`, so the return value is byte-for-byte the M3 value (base, base×overtime, +1/arrived
  miner). Flat income correctly is NOT doubled by overtime (mirrors miner income — only the base doubles, §3.2).
- **ResetEconomy zeroes it (Play Again).** `ResetEconomy()` now also sets `FlatIncomePerTick = 0`; the miner
  zeroing and the two unconditional reset-path broadcasts (`OnMinerCountChanged`, `RefreshGoldRate(true)`) are
  untouched. PlayAgain order is sound: game-mode step 2b destroys every `ABuilding` first
  (`ADeepMine::EndPlay(Destroyed)` → `RemoveIncome`, driving the accumulator to 0), THEN step 4 `ResetEconomy()`
  zeroes it (no-op) — no double-subtract, no spurious "removed more than added" Error in the normal flow.
- **Multi-team is unchanged.** `ADeepMine` resolves its owner via `ASiegeGameState::GetPlayerStateForTeam(Team)`
  (TASK-043), identical to `AMinerUnit`. A Red Deep Mine feeds only the bot's Red economy.

**Miner economy non-regression: CONFIRMED.**

## CRITICAL confirmation — freeze hook is CLEAN

- Step 2b (`FreezeWorldAtMatchEnd`) is a NEW, self-contained loop iterating `TActorIterator<ABarracks>` →
  `It->FreezeAI()`, inserted after the tower sweep (step 2) and before projectiles (step 3). It does not modify
  steps 1/2/3/4/5/6.
- **No double-handling.** `ABarracks` derives from `ABuilding`, NOT `ATower`, so the step-2 `ATower` iterator
  (`ClearAllTimersForObject`) never picks it up. `FreezeAI()` neither spawns nor destroys, so it is safe inside
  the live iterator.
- **No spawn can fire post-freeze.** `FreezeWorldAtMatchEnd` runs entirely synchronously from
  `OnCastleDestroyedHandler`; timers cannot interleave. Units already spawned are frozen by step 1; the Barracks
  is then prevented from making more. `FreezeAI()` clears BOTH the spawn and self-destruct timers and latches
  `bFrozen`; `SpawnUnit()` also early-outs on `bFrozen || IsBuildingDestroyed()` as belt-and-braces.
- **Deep Mine income freezes via existing PauseIncome.** No per-mine hook — step 4's `PauseIncome()` gates
  `HandleGoldTick` on `bIncomePaused`, halting flat income with all other accrual. The registered rate stays
  "configured" (matches arrived-miner semantics). Confirmed.
- Log line correctly extended: 6 format specifiers, 6 args (`FrozenBarracks` added) — no format mismatch.

**Freeze hook: CLEAN.**

---

## Findings

- **[WARN] DeepMine.cpp:60-95 / 123-134 — genuinely mis-teamed mine leaves an idle 0.25 s retry timer running
  until destroyed.** If `GetPlayerStateForTeam(Team)` returns null with a live GameState, `bWarnedNoTeamPlayerState`
  latches and `ResolveOwningPlayerState` returns null forever; `TryRegisterIncome`'s looping retry timer then
  fires every 0.25 s doing nothing (no re-arm — `IsTimerActive` is true; no log — one-shot guard). Bounded by
  actor lifetime (EndPlay clears it; raid/PlayAgain destroys the mine) and NEVER occurs in designed flows (the
  owning economy always exists before a 15-gold mine can be placed). Non-blocking, but note the divergence from
  the `AMinerUnit` precedent: there the poll is armed unconditionally and does real arrival work, so the same
  latch costs nothing extra; here the timer exists solely to register. Acceptable as-is; a future tidy could stop
  the poll once the team lookup is known-dead. No fix required for PASS.

- **[NIT] DeepMine.cpp:76-83 — retry could register income to a paused economy under a victory-screen window.**
  Only reachable if BeginPlay could not resolve the PS and the match ends before the retry succeeds; `AddIncome`
  then raises the rate on a paused economy (no accrual, cleared by PlayAgain). Flagged in the handoff; genuinely
  harmless. Note only.

- **[NIT] Barracks.cpp:107-108 — spawn direction is toward-enemy world-X, not `GetActorForwardVector()`.**
  Correct design call: all buildings spawn with `FRotator::ZeroRotator` (actor-forward always +X), so raw forward
  would spawn Red's footmen behind its Barracks. `Team == Blue ? +X : -X` is team-correct for both sides and the
  unit paths to the enemy castle regardless; the offset only needs to clear the ~400×400 footprint. Confirmed
  sound, not a defect. Note only.

- **[NIT] DeepMine — benign "CardType Economy, expected Building" LoadStats warning per mine.** The DeepMine row
  is `CardType Economy` but `ADeepMine` is an `ABuilding`, so `ABuilding::LoadStats` logs one Warning per mine.
  Verified HP still binds correctly: `MaxHP = Row->HP` (200) executes BEFORE the CardType check, unaffected by it.
  Carry-forward for TASK-059 (its `ResolveCardActorClass` must route the Economy-typed DeepMine down the building
  spawn path) is correctly out of scope here. Note only.

---

## Detail checks (all pass)

**API / UE 5.8 validity.** No deprecated/removed APIs. `UNavigationSystemV1::GetCurrent` +
`ProjectPointToNavigation(Point, OutLocation, Extent)`, `SpawnActorDeferred<T>` + `FinishSpawning`,
`GetDefaultObject<ASummonedUnit>()`, `GetScaledCapsuleHalfHeight()`, `World->GetGameState<T>()`,
`GetWorldTimerManager().SetTimer/ClearTimer/IsTimerActive`, `TSoftClassPtr<>::LoadSynchronous`,
`TWeakObjectPtr<>::Get` — all current for 5.8. `ASummonedUnit : ACharacter` confirmed, so `GetCapsuleComponent()`
is valid. Spawn transform math mirrors the QA-passed `ASiegePlayerController` precedent (capsule half-height from
CDO, ZeroRotator + `CapsuleHalfHeight + SpawnGroundClearance` lift, `AdjustIfPossibleButAlwaysSpawn`).

**C4457/C4458/C4459 shadow scan (CONVENTIONS coding law).** CLEAN. All locals/params renamed off inherited
reflected UPROPERTYs: `OwnerState` (not `Owner`), `GoldPerTickDelta`, `SpawnUnitCardID`, `SiegeGameState`,
`ForwardSign`, `UnitClass`, `SpawnPoint`, `FrozenBarracks`/`It`. The `SpawnActorDeferred(..., /*Owner=*/ this,
/*Instigator=*/ nullptr, ...)` are positional call args with comment labels, not declarations. No member in
`Barracks.h`/`DeepMine.h`/`SiegePlayerState.h` shadows an `AActor`/`ABuilding`/`APlayerState` reflected member.

**Null-safety.** `Barracks::SpawnUnit` guards `World`, `UnitClass`, `NavSys` (if-scoped), `UnitCDO`, `Capsule`,
and the spawned `Unit`. `ResolveSpawnUnitClass` is null-safe with `IsChildOf(ASummonedUnit)` validation and a
one-shot missing-BP warning. `DeepMine` guards `World`, `SiegeGameState`, and `CachedOwnerState.Get()` (weak ptr)
before `RemoveIncome`. `AddIncome`/`RemoveIncome` refuse non-positive deltas (logged) and `RemoveIncome` clamps
at 0 with an Error log — rate can never go below base.

**Timer / lifetime discipline.** Barracks arms the loop only for `SpawnInterval > 0 && !SpawnCardID.IsNone()`
and the one-shot only for `Lifetime > 0` (the ATower Cadence-guard precedent; `SetTimer(<=0)` clears not
schedules — a broken row is never clamped into validity, both logged). `EndPlay` clears both timers
synchronously; `FreezeAI` is idempotent and safe before stats bind. Spawned units persist after Barracks
self-destruct (Owner relationship does not propagate `Destroy()`; units are independent actors). DeepMine
`EndPlay(Destroyed) && bIncomeRegistered` removes income exactly once (latch cleared); non-Destroyed teardown
correctly does NOT unregister (income dies with the PS — the AMinerUnit precedent); retry timer cleared in
EndPlay. `IncomeRegisterRetryInterval` double-guarded via `FMath::Max(..., 0.05f)`.

**UE reflection / GC.** `CachedSpawnUnitClass` is `TObjectPtr<UClass>` UPROPERTY(Transient) — GC-safe.
`CachedOwnerState` is `TWeakObjectPtr<ASiegePlayerState>` — correct for a non-owning cross-actor reference.
Overrides carry `override`; `FreezeAI`/`AddIncome`/`RemoveIncome` are new (correctly not marked override).
Data-driven: Barracks reads SpawnCardID/SpawnInterval/Lifetime + HP from DT_Cards; DeepMine reads HP; only the
documented `DeepMineIncome = 2 // GDD §8` and feel-value offsets are UPROPERTYs (CONVENTIONS mechanic-rule).

**Conventions / paths.** Class + file names match the spec names block (`ABarracks`→`Barracks.h/.cpp`,
`ADeepMine`→`DeepMine.h/.cpp`). Composed spawn path `/Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C`
matches the CONVENTIONS unit contract. Includes are complete for every referenced symbol.

---

## Notes for build-master (on PASS)

- Compiles with the M4 batch at TASK-068 (no isolated build requested). No `Build.cs` change
  (`NavigationSystem`/`Engine` already public deps) — no full-rebuild forcing edit.
- Shared-file edit to `SiegeGameMode.cpp` (freeze step 2b + `#include "Siegebound/Barracks.h"`) is additive and
  does not conflict with TASK-054 (DamageTypes/Castle/Building/SummonedUnit).
- Expect one benign per-mine "row 'DeepMine' has CardType Economy, expected Building" Warning at runtime until
  TASK-059 routing lands — not a build error; HP binds correctly.
- Runtime spawn of `BP_Unit_Footman` / `BP_Building_Barracks` / `BP_Building_DeepMine` depends on TASK-062/063
  BP assets; missing BPs are null-safe (logged refusal, no crash).
