# TASK-057 handoff — Barracks spawner + Deep Mine economy (gameplay-programmer)

**Status:** ready-for-qa. Files only, no editor, no compile, no Git, no TASKBOARD edit.
M4 wave 2, developed on `main`, parallel with TASK-054 (I did NOT touch DamageTypes/Castle/Building/SummonedUnit).

## Files written / changed

New class pairs:
- `Source/GitClaudeUnrealTest/Siegebound/Barracks.h`
- `Source/GitClaudeUnrealTest/Siegebound/Barracks.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/DeepMine.h`
- `Source/GitClaudeUnrealTest/Siegebound/DeepMine.cpp`

Edited:
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h` / `.cpp` — flat non-miner income API + GetGoldRate composition + ResetEconomy zeroing.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` — **shared-file edit** (freeze hook, see below). This is beyond the three enumerated file-pairs; it is the literal implementation of the spec's "hook the existing freeze sweep the same way towers/units do." No conflict with TASK-054 (which touches DamageTypes/Castle/Building/SummonedUnit, not the game mode).

**No `Build.cs` change** — `NavigationSystem` and `Engine` are already public deps; no new module. No full-rebuild forcing edit.

## 1) ABarracks : ABuilding (Barracks.h/.cpp)
- Constructor sets `CardID = "Barracks"` (the MinerUnit precedent — identity defaults in code, every stat still binds from DT_Cards).
- Overrides `OnStatsLoaded(Row)` (the ATower hook, NOT BeginPlay): reads the spawner triple `SpawnCardID` / `SpawnInterval` / `Lifetime` from the row, then:
  - arms a **looping** spawn timer at `SpawnInterval` (8 s) — first spawn one full interval after stats bind (ATower first-shot precedent);
  - arms a **one-shot** self-destruct timer at `Lifetime` (60 s).
  - Both behind the ATower Cadence-guard pattern: `SpawnInterval <= 0` / empty `SpawnCardID` never arms the loop (logged); `Lifetime <= 0` never arms self-destruct (logged). SetTimer(<=0) clears rather than schedules — a broken row is never clamped into validity.
- `SpawnUnit()`: composes `/Game/Blueprints/Units/BP_Unit_<SpawnCardID>_C` (Footman) via `ResolveSpawnUnitClass()` (cached after first load, null-safe, IsChildOf(ASummonedUnit) checked). Deferred-spawn → `InitUnit(Team, SpawnCardID)` → `FinishSpawning` — the exact controller/bot spawn pattern (Team set before BeginPlay so TASK-044 team material lands; Owner=this, Instigator=nullptr). Navmesh-projected front point + capsule lift via the shared `SiegeSpawn::` constants.
- `HandleLifetimeExpired()`: `Destroy()` self. **Already-spawned units persist** — they are independent actors, never touched.
- `FreezeAI()` (public, BlueprintCallable): stops BOTH timers, sets `bFrozen`. Idempotent.
- `EndPlay`: clears both timers (ATower EndPlay precedent — dies with Destroy/PlayAgain sweep).

**Spawn direction decision (scrutinize):** "in front of the Barracks" is implemented as a short offset **toward the enemy half** (`Team==Blue ? +X : -X`, per CONVENTIONS world axes), NOT raw `GetActorForwardVector()`. Rationale: all buildings spawn with `FRotator::ZeroRotator` (actor-forward is always +X), so raw forward would spawn the Red bot's footmen *behind* its Barracks. Toward-enemy is team-correct for both sides; units path to the enemy castle regardless. `SpawnFrontOffset` UPROPERTY = 300 (clears the ~400×400 SM_Barracks footprint, TASK-067).

## 2) ADeepMine : ABuilding (DeepMine.h/.cpp)
- Constructor sets `CardID = "DeepMine"`.
- `DeepMineIncome` UPROPERTY = **2** `// GDD §8` (mechanic rule → UPROPERTY, not a CSV column, per CONVENTIONS).
- `BeginPlay` → `Super::BeginPlay()` (HP 200 from row) → `TryRegisterIncome()`.
- `TryRegisterIncome()`: resolves the owning economy via `ASiegeGameState::GetPlayerStateForTeam(Team)` (the TASK-043 accessor, the AMinerUnit precedent) and calls `OwnerState->AddIncome(DeepMineIncome)` **immediately** (no walk). Latches `bIncomeRegistered`, caches the owner state. If unresolvable at BeginPlay (early-spawn ordering edge) it arms a light 0.25 s retry poll that self-clears on success. One-shots the team-lookup warning (AMinerUnit precedent — avoids log spam for a genuinely mis-teamed mine).
- `EndPlay(Destroyed)` → if registered, `CachedOwnerState->RemoveIncome(DeepMineIncome)` (drops the rate by 2). World teardown (non-Destroyed) does NOT unregister (income dies with the PS — AMinerUnit precedent).
- Destructible + blocks pathing inherited from ABuilding (200 HP, BlockAll, nav-relevant). No per-mine freeze hook needed — match-end `PauseIncome()` stops the flat income with all other accrual.

**Known benign warning (flagged):** the DeepMine row is `CardType Economy`, but ADeepMine is an ABuilding. `ABuilding::LoadStats` logs a one-time "row 'DeepMine' has CardType Economy, expected Building" **warning** per mine. HP still binds correctly (LoadStats binds HP regardless of CardType). I can't suppress it without editing Building.cpp (TASK-054's frozen file). **Heads-up for TASK-059:** `ResolveCardActorClass` currently routes `ECardType::Economy` to `/Game/Blueprints/Units/BP_Unit_<CardID>` (RequiredBase ASummonedUnit) — a Deep Mine (a building BP under `/Game/Blueprints/Buildings/`, ADeepMine) will need the play/spawn path to route it as a building. That routing is TASK-059's scope, not mine; my class is correct when spawned via the building deferred-spawn path.

## 3) ASiegePlayerState income-API extension (exact new signatures)
```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold") void AddIncome(int32 GoldPerTickDelta);
UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold") void RemoveIncome(int32 GoldPerTickDelta);
```
- New private accumulator `int32 FlatIncomePerTick = 0;` (VisibleInstanceOnly, Transient).
- `GetGoldRate()` now returns `BaseRate + (MinerIncomeCount * MinerGoldPerTick) + FlatIncomePerTick`.
- `AddIncome` / `RemoveIncome`: adjust the accumulator, then `RefreshGoldRate(false)` (broadcast on actual change). Guards: non-positive delta refused+logged; RemoveIncome clamps at 0 (underflow logged).
- `ResetEconomy()` now also zeros `FlatIncomePerTick` (belt-and-braces; buildings are destroyed first in PlayAgain, so it is normally already 0).
- **TASK-059/060 may consume `AddIncome`/`RemoveIncome`** — deliberately generic (int32 gold-per-tick), not DeepMine-specific.

### How the M2/M3 miner economy stays untouched (byte-for-byte)
- The Deep Mine path touches ONLY `FlatIncomePerTick` (new), the GetGoldRate additive term, and ResetEconomy's zeroing.
- It NEVER calls or mutates `AliveMinerCount`, `MinerIncomeCount`, `MaxActiveMiners`, `CanAddMiner`, `GetAliveMinerCount`, `RegisterMinerAlive`/`UnregisterMinerAlive`, `AddMinerIncome`/`RemoveMinerIncome`, or `OnMinerCountChanged`. The **miner cap does NOT apply to Deep Mines** (they never register as miners).
- With zero deep mines, `FlatIncomePerTick == 0`, so `GetGoldRate()` returns the identical value M2/M3 produced. Multi-team resolution is unchanged (Deep Mine resolves its owner through the same `GetPlayerStateForTeam` accessor miners use).

## 4) Freeze hook (SiegeGameMode.cpp)
- Added `#include "Siegebound/Barracks.h"`.
- In `FreezeWorldAtMatchEnd`, new **step 2b** (after the tower sweep, before projectiles): iterates `ABarracks` and calls `It->FreezeAI()` — the same shape as the ASummonedUnit `FreezeAI()` sweep (step 1) and the ATower silence sweep (step 2). ABarracks is not an ATower, so no double-handling. Deep Mines are covered by step 4's `PauseIncome()`.
- Freeze log line extended to report `%d barracks frozen`.
- PlayAgain already destroys all `ABuilding` (Barracks + Deep Mine included) → their EndPlay clears timers / removes income; no PlayAgain edit needed.

## What QA should scrutinize
1. **C4457/58/59 shadow scan** (CONVENTIONS coding law — cost 2 build loops in M2). I named all locals/params to avoid inherited reflected UPROPERTYs: `OwnerState` (not `Owner`), `GoldPerTickDelta` (param), `SpawnUnitCardID`, `SiegeGameState`, `ForwardSign`, `UnitClass`, `SpawnPoint`. SpawnActorDeferred's `Owner=`/`Instigator=` are call arguments, not local declarations. Please re-verify.
2. **Freeze correctness:** Barracks stops spawning at match end (step 2b); a mid-match PlayAgain destroys it. Confirm no spawn can fire after `FreezeAI` (timer cleared synchronously in the OnCastleDestroyedHandler call stack, before any further tick).
3. **Miner economy invariance** — confirm the Deep Mine path never reaches any miner symbol (item 3 above).
4. **Spawn-direction decision** (toward-enemy vs actor-forward) — flagged above as a design call.
5. **DeepMine CardType=Economy benign warning** + the TASK-059 routing heads-up.
6. **Deep Mine retry poll** — self-clears on register; under a (never-in-practice) victory-screen window it would register income to a paused economy (no accrual). Note, not a bug.
7. All stats data-driven: Barracks reads SpawnCardID/SpawnInterval/Lifetime + HP from DT_Cards; DeepMine reads HP from DT_Cards; only the +2/s is the documented UPROPERTY (§8, CONVENTIONS mechanic-rule).
