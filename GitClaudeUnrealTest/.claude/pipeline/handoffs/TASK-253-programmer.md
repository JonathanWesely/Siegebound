# TASK-253 — AGoldNode → neutral depleting claimable mine (handoff)

**Author:** gameplay-programmer · **Date:** 2026-07-22 · **Branch:** `m7.6-arena10x` (GoldNode.{h,cpp} branch-owned per the M7.6 ownership extension)
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/GoldNode.h`, `Source/GitClaudeUnrealTest/Siegebound/GoldNode.cpp` — nothing else.
**NOT compiled** (TASK-258 batch compile, by spec). The tree will NOT compile until TASK-254/255/256 land — expected, see "Known breakage" below.

## API surface as built (the contract for TASK-254/255/256)

```cpp
// lifecycle (scatter, TASK-255)
void  InitMine(int32 InReserve);                      // clamp>=0, latch InitialGoldReserve, derive bDepleted,
                                                      // clear registry+claim+timer, gauge update, reserve broadcast.
                                                      // Safe before or after BeginPlay (fully resets either way).

// occupancy (miner, TASK-254)
bool  CanTeamMine(ETeamId MinerTeam) const;           // !depleted && reserve>0 && (unclaimed || claimed-by-MinerTeam)
bool  TryRegisterArrivedMiner(AMinerUnit* Miner);     // false: null/dying miner or CanTeamMine==false (-> WAIT MODE).
                                                      // true: registered; atomic claim + 1s drain start on 0->1.
                                                      // Idempotent on double-arrival (no duplicate registry entry).
                                                      // Reads Miner->GetTeamId() (ASummonedUnit) — no new surface needed.
void  UnregisterArrivedMiner(AMinerUnit* Miner);      // removes + sweeps stale weaks; on empty: claim released,
                                                      // drain stopped (mine claimable by either team again).

// finder (miner TASK-254 + bot TASK-256 — THE single finder)
static AGoldNode* FindBestMineFor(UWorld* World, ETeamId Team, const FVector& From);
                                                      // tier-1 nearest (2D) CanTeamMine; tier-2 nearest enemy-occupied
                                                      // non-depleted (wait target); nullptr = all depleted/none.
                                                      // Strict < on DistSquared2D => deterministic, no churn on ties.

// queries (BlueprintPure unless noted)
bool  IsDepleted() const;  int32 GetGoldReserve() const;  int32 GetInitialGoldReserve() const;  bool IsOccupied() const;
TOptional<ETeamId> GetOccupyingTeam() const;          // C++-only (TOptional not reflectable)

// delegates (BlueprintAssignable; HUD backlog — nothing binds this pass)
FOnMineDepleted       OnMineDepleted;                 // OneParam(AGoldNode* DepletedMine) — fires AFTER eviction
FOnMineReserveChanged OnMineReserveChanged;           // TwoParams(int32 NewReserve, int32 InitialReserve)
```

### PINNED cross-file contract for TASK-254 (declared here, implemented there)
`GoldNode.cpp` `Deplete()` calls **`Miner->NotifyMineDepleted(this)`** — TASK-254 must declare on `AMinerUnit`:
```cpp
public: void NotifyMineDepleted(AGoldNode* DepletedMine);
```
Call-site guarantees TASK-254 may rely on: when it fires, the mine is ALREADY `bDepleted == true`, registry empty, claim released, drain stopped — so re-entrant `UnregisterArrivedMiner` is a safe no-op and a re-entrant `FindBestMineFor` never returns the depleting mine. Fired only for still-valid miners, at most once per miner per depletion.

## Behavior notes
- **Drain tick (1 s, only while occupied):** stale-weak compaction (self-heals the claim if all occupants died un-unregistered) → reserve = max(0, reserve − n × DrainPerMinerPerSecond) → gauge + broadcast on actual change → `Deplete()` at 0. Timer is never re-armed mid-cycle (claim churn can't stretch the drain window).
- **Deplete() order (load-bearing, per plan):** latch + zero + stop timer → snapshot → clear registry+claim → per-miner NotifyMineDepleted → OnMineDepleted broadcast → gauge to ember → one Log line (`depleted (initial reserve %d) — evicted %d miner(s)`).
- **Gauge:** lazy transient MID via `CreateAndSetMaterialInstanceDynamic(0)` (retried until mesh+slot-0 material exist), drives `GlowIntensity` = Lerp(0.05, 1.0, Reserve/Initial). Missing param = visually inert MID write (silent no-op by design until TASK-257 authors it). Intensity modulation only — glows-regardless-of-team law held.
- **EndPlay:** drain timer cleared + registry/claim reset, NO miner notification (ClearScatter teardown safety; Play-Again kills miners before re-scatter per plan).
- **Invariant (QA focus):** `OccupyingTeam.IsSet() ⟺ ArrivedMiners.Num() > 0` after every public call and every compaction. Enforced at: claim (0→1 add), release (empty after remove), compaction (empty after sweep), Deplete, InitMine, EndPlay.
- **Laws held byte-faithfully:** NoCollision profile + SetCollisionEnabled(NoCollision) + no overlaps + SetCanEverAffectNavigation(false); SetCanBeDamaged(false); no ITeamAgent; soft `/Game/Meshes/SM_GoldNode.SM_GoldNode` resolve in OnConstruction (silent) + BeginPlay (warn-once); Movable-mobility rationale; tick off (drain is a timer).

## Known breakage until siblings land (expected — TASK-258 compiles the batch)
`Team`/`GetTeam()` removed; the 3 callers, each rewritten in-feature:
1. `MinerUnit.cpp:452` — `FindNearestSameTeamGoldNode` (dies at TASK-254 → FindBestMineFor)
2. `BattlefieldScatter.cpp:656` — RebuildKeepClearZones gold-node block (deleted at TASK-255)
3. `SiegeBotController.cpp:1017` — `GetGoldNodeRedLocation` (deleted at TASK-256)
Also: `NotifyMineDepleted` does not exist on AMinerUnit until TASK-254 (link/compile error until then — noted per dispatch).

## Deviations from plan/spec (each justified)
1. **SiegePlayerState.h cross-note NOT added.** CONVENTIONS says "both headers cross-note" but the lane law (same clause + appendix 2) says SiegePlayerState is MAIN-LANE FROZEN and untouched by this feature. GoldNode.h carries the full paired-tunable law text naming `ASiegePlayerState::MinerGoldPerTick` and citing CONVENTIONS; the PlayerState-side one-line comment reconciles at the Phase-6 merge / next docs pass. Flagging rather than violating the freeze.
2. **`DrainPerMinerPerSecond` is EditDefaultsOnly** (spec silent on edit level): matches its pair `MinerGoldPerTick` (EditDefaultsOnly) — a per-INSTANCE drain override would break the paired-tunable law on one mine invisibly.
3. **Extra accessors** beyond the specced trio: `GetInitialGoldReserve()` (BlueprintPure — HUD fraction denominator) and `GetOccupyingTeam()` (C++-only TOptional — bot/miner logic convenience).
4. **Drain cadence is `static constexpr` 1.0 s, not a UPROPERTY:** DrainPerMinerPerSecond is denominated per-second and paired to the 1.0 s income tick; a tunable cadence would silently break the law.
5. **Category rename** `Siegebound|GoldNode`/`Siegebound|Team` → `Siegebound|Mine` throughout (the team category is gone with Team). The two level instances serializing the removed `Team` property drop it silently on load and are deleted at TASK-257 anyway.

## Coordination flag for TASK-257 (art — M_GoldGlow)
Code drives `GlowIntensity` in **[0.05, 1.0] with 1.0 = the authored full look**. The param must therefore be authored as a NEW multiplicative scale (default **1.0**) inserted INTO the existing emissive chain — NOT as a replacement of the existing multiplier constant (default = K). Both readings satisfy "undriven ⇒ byte-identical", but only the default-1.0 scale keeps a FULL mine byte-identical once the code drives it. (CONVENTIONS' "default = current multiplier" wording is ambiguous on this; the Lerp(0.05, 1.0) law pins the 1.0-scale authoring.)

## QA pointers (laws-unchanged review mode)
- Constructor/OnConstruction/ResolveNodeMesh preserved essentially verbatim from the qa-passed original (diff them first — the law block is intentionally byte-faithful).
- Shadow scan: no `Team`/`Owner`/`World` member shadowing (Team member removed; `World` locals mirror the house `UWorld* World = GetWorld()` pattern; `FindBestMineFor` is static).
- Complete-type includes: TimerManager.h (FTimerManager methods), Materials/MaterialInstanceDynamic.h (SetScalarParameterValue), Siegebound/MinerUnit.h (GetTeamId + NotifyMineDepleted), EngineUtils.h (TActorIterator), Engine/World.h, Engine/TimerHandle.h in the header (FTimerHandle member).
- `TNumericLimits<float>::Max()` + explicit float cast of `DistSquared2D` (FReal/double in UE5) — no implicit-truncation warning.
