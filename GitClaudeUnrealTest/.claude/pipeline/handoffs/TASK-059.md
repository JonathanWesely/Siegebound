# TASK-059 — Play v3: Instant/upgrade routing + Masons castle-heal + Swarm multi-spawn + stack-cap refund (handoff)

**Status:** ready-for-qa. Files only — no compile, no Git, no editor, no TASKBOARD edit (orchestrator owns the board). M4 wave 5 on `main`. Serialized after TASK-058 (consumes `ApplyUpgrade`/`EHeroUpgradeResult`) and TASK-057 (folds the DeepMine routing carry-forward).

## Files changed (exactly the allowed set)
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` / `.cpp` — Instant routing, shared Swarm spawn helper, DeepMine building routing, stack-cap refund.
- `Source/GitClaudeUnrealTest/Siegebound/Castle.h` / `.cpp` — `HealOverTime` (Masons heal-over-time receiver).
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` — **ONE-LINER** carry-forward B (`TrackedHero->ResetUpgrades()` in `PlayAgain`). Flagged below; no other task is editing SiegeGameMode.

**No new files, no `Build.cs` change.** Every include I use was already in the module link set except two additions to `Castle.cpp` (`TimerManager.h`, `GitClaudeUnrealTest.h` — Castle.cpp had no timers/logs before). No new module.

---

## 1) INSTANT routing (HeroUpgrade / Utility) — `ResolveInstantPlay`
`PlayHandSlot`'s type switch now routes `HeroUpgrade`/`Utility` into a new private `ResolveInstantPlay(int32 Slot, FName CardID, const FCardRow& Row, ASiegePlayerState& SiegeState)` instead of the old "instants arrive in M4" refusal. No placement step. The affordability `CanAfford` pre-check that already runs BEFORE the switch is the net-zero gate (no gold moved); the actual `SpendGold` happens inside only on success.

- **Hero upgrades** (SharpenedBlade/PlateArmor/SwiftBoots/WarBanner): `Hero = Cast<AHeroCharacter>(GetPawn())`. **Apply FIRST** — `Hero->ApplyUpgrade(CardID)` (TASK-058) — because there is no `RemoveUpgrade`, so the result must be known before spending:
  - `Applied` → `SpendGold(Row.Cost)` then `ConfirmInstantDraw(Slot)` (redraw). SpendGold cannot fail (same-stack `CanAfford` held; nothing spends between, income only adds) — a false return is logged as a hard-invariant tripwire.
  - `RefusedAtMaxStacks` → **NO spend**, `RefuseCardPlay(CardID, "{DisplayName} at max stacks")` (fires OnCardRefused + OnCardPlayRefused). §3.10 full refund.
  - `RefusedInvalidCard` → **NO spend**, refuse "Upgrade unavailable".
  - No hero pawn → refuse "Hero unavailable", no spend. (A DEAD hero is NOT refused — `ApplyUpgrade` supports it: stack added, alive-only effects deferred to respawn per TASK-058. Deliberate.)
- **Utility Masons**: pre-check a living friendly castle → `SpendGold` → `HealOverTime` → `ConfirmInstantDraw` (spec order). Any other Utility CardID → refuse "Card not available", no spend.

## 2) MASONS — `ACastle::HealOverTime(float Total, float Duration)`
Generic heal-over-time receiver on ACastle (the caller owns the magnitudes — CONVENTIONS mechanic-rule). A repeating `HealTimerHandle` at `HealTickInterval` (0.2s UPROPERTY, impl detail) delivers `HealPerTick = HealRemaining / (Duration/HealTickInterval)` per tick, each **clamped to MaxHP** and broadcasting the existing `FOnCastleHPChanged` only on an ACTUAL change (mirrors `TakeDamage`). Self-terminates when the pool empties OR HP reaches MaxHP (**never over MaxHP**). Restack: a second call ADDS `Total` and re-derives the rate over the new Duration. Cancelled by `HandleDestroyed()` and `ResetCastle()` (both now call `StopHealOverTime()`). No-op on a destroyed castle or non-positive args.

Masons magnitudes are **UPROPERTYs on ASiegePlayerController** (`MasonsHealAmount = 300 // GDD §4`, `MasonsHealDuration = 10 // GDD §4`, plus `MasonsCardID = "Masons"`), so ACastle stays a generic receiver. Masons Cost (8) comes from DT_Cards.

## 3) SWARM multi-spawn — shared reusable helper (TASK-060 CONSUMES THIS)
**EXACT signature (public static on ASiegePlayerController — call as `ASiegePlayerController::SpawnUnitSwarm(...)`):**
```cpp
static TArray<ASummonedUnit*> SpawnUnitSwarm(
    UWorld* World, UClass* UnitClass, FName CardID, ETeamId Team,
    AActor* SpawnOwner, APawn* SpawnInstigator,
    const FVector& Center, int32 Count, float Radius);
```
- `Count > 1` (Swarm; MilitiaMob `SwarmCount = 4`): copies arranged evenly on a circle of `Radius` around `Center`, **each ring point navmesh-projected** (`ProjectPointToNavigation`, extent `(max(Radius/2,100), max(Radius/2,100), 200)`), falling back to the raw ring point when there is no nav data / the projection misses.
- `Count <= 1`: single unit spawned **AT `Center` with NO reprojection** — the confirm path already validated it on the navmesh, so the M1/M2 single-unit spawn is byte-for-byte.
- Each copy: capsule-lifted from the class CDO + `SiegeSpawn::` constants, deferred-spawn → `InitUnit(Team, CardID)` → `FinishSpawning` (stats bind before BeginPlay). Static + fully parameterized, **no gold logic** — the caller owns the gold gate.
- Returns the spawned units (empty on total failure).

**Player confirm path (`TryConfirmPlacement`, unit/economy branch):** spawn the swarm FIRST via the helper (`Count = PendingSwarmCount`, `Radius = SwarmSpawnRadius = 300 // GDD §3.0`), then gate gold — the M1 discipline generalized to N: 0 spawned → refuse, no gold, stay in mode; `SpendGold` fails → destroy EVERY copy, refuse, stay in mode; else success, ONE Cost. Gold cannot actually drop during placement (plays/discards refused in-mode, income only adds) so the spend refusal is defensive — same as the pre-existing single-unit path.

**Bot (TASK-060):** call `SpawnUnitSwarm` directly with `Team = Red`, its own Owner, `Instigator = nullptr` (bot has no pawn), an own-half navmesh-valid `Center`, `Count = row.SwarmCount`, `Radius = 300`. Team material (TASK-044) lands because Team is set before BeginPlay. The bot spends its own gold before calling — the helper never touches gold.

## 4) Carry-forward A — DeepMine building routing (folds TASK-057 heads-up)
DeepMine's row is `CardType Economy` but ADeepMine is an ABuilding under `/Game/Blueprints/Buildings/`. **Approach: an editable UPROPERTY set `BuildingEconomyCardIDs = { "DeepMine" }`** + a single-source-of-truth predicate `bool IsBuildingCard(FName CardID, ECardType CardType)` = `CardType==Building || (CardType==Economy && BuildingEconomyCardIDs.Contains(CardID))`. Chosen over a speculative "does BP_Building_<CardID> exist?" load (that logs a miss for every Miner play) and over a bare hardcoded CardID (the set is extensible with no code change, matching the existing `MinerCardID`/`MasonsCardID` pattern). Used in THREE places so they can never disagree:
- `EnterPlacementMode` captures `bPendingIsBuilding = IsBuildingCard(...)`;
- `UpdatePlacementGhost` §3.5 building-clearance gate now keys off `bPendingIsBuilding` (DeepMine gets clearance);
- `TryConfirmPlacement` branch + `ResolveCardActorClass` route DeepMine to `/Game/Blueprints/Buildings/BP_Building_DeepMine` (ABuilding). Miner (Economy, NOT in the set) still routes to `BP_Unit_Miner`. Building cards unchanged.

## 5) Carry-forward B — PlayAgain upgrade reset (SiegeGameMode.cpp one-liner)
Per `handoffs/TASK-058-programmer.md`: in `ASiegeGameMode::PlayAgain()` step 5, immediately BEFORE `RestoreHeroAtStart()`:
```cpp
if (IsValid(TrackedHero)) { TrackedHero->ResetUpgrades(); }
```
`ResetHero` is the shared respawn+PlayAgain path so upgrades can't self-distinguish a reset from a respawn; this clears them on Play Again while they still persist through respawn. **This edits SiegeGameMode.cpp beyond my names block** — flagged. No other task currently edits SiegeGameMode (TASK-057's freeze edit is already landed and untouched here).

## Preserved invariants
- **Melee suppression (qa/TASK-003 warning 2):** instants never enter placement mode → no suppression to manage. The swarm/building changes add NO new exit path that bypasses `ExitPlacementMode` — stay-in-mode refusals keep suppression, success funnels through `ExitPlacementMode`. Placement entry/exit unchanged.
- **Net-zero refund (§3.0):** every instant refusal and every swarm failure moves NO gold; a spawn/spend failure unwinds all committed actors.
- **Byte-for-byte for existing cards:** single-unit spawn (Count≤1) uses the identical lift + deferred spawn at the validated point; building branch logic unchanged (only its gate condition became `bPendingIsBuilding`, which is `true` for every real Building card exactly as `PendingCardType==Building` was); Spell still M5-refused; placement v2 validity/ghost untouched.

## C4457/58/59 shadow scan (CONVENTIONS coding law)
New params/locals deliberately avoid inherited reflected UPROPERTYs: `SpawnOwner` (not `Owner`), `SpawnInstigator` (not `Instigator`), plus `SwarmUnit`, `SwarmUnits`, `FriendlyTeam`, `FriendlyCastle`, `GroundPoint`, `RingProjectExtent`, `NavSys`, `TicksOverDuration`, `HealRemaining`, `HealPerTick`. `Slot`/`CardID`/`World`/`Team` follow existing precedent in these files (ASiegePlayerController is not a UWidget, so `Slot` is not an inherited reflected member). SpawnActorDeferred `/*Owner=*/`/`/*Instigator=*/` are comment labels, not declarations.

## What QA should scrutinize
1. **Apply-then-spend for upgrades** (not spend-then-apply): required by TASK-058 (`ApplyUpgrade` return decides the refund; no RemoveUpgrade). The Applied→SpendGold leak is a documented dead path (CanAfford holds same-stack) with a tripwire log.
2. **Full-castle Masons is ALLOWED** (spends + draws; `HealOverTime` clamps to a no-op when already at MaxHP). The spec pre-check is "a living friendly castle," not "a damaged one" — I did not add an unspecified "castle full" refusal. Flag if you want it refused instead.
3. **Masons heal ticking under a victory screen** if the WINNER's castle was mid-heal at match end: ACastle is not in `FreezeWorldAtMatchEnd`'s sweep (I'm scoped to the one-liner, can't add it). Harmless (bar fills; self-terminates ≤ Duration) and cleared by `ResetCastle` on Play Again. A LOSER's castle stops via `HandleDestroyed`→`StopHealOverTime`. Benign — flagged, not fixed.
4. **Swarm ring off-navmesh fallback:** a ring point that fails projection spawns at the raw point (units self-path). Rare in the open arena at Radius 300 around a validated center. Documented degrade-open.
5. **BuildingEconomyCardIDs routing approach** (§4 above) vs a speculative-load or hardcoded-CardID alternative — confirm the editable-set choice is acceptable.
6. **SpawnUnitSwarm is a public static on ASiegePlayerController** (files-only constraint blocked a new shared file). TASK-060's bot will include SiegePlayerController.h to call it. Confirm this home is acceptable, or note a preferred relocation for a later refactor.
7. **Castle.cpp new includes** (`TimerManager.h`, `GitClaudeUnrealTest.h`) — the log-category include is required because the file had no `UE_LOG` before.
8. **SiegeGameMode.cpp one-liner** is outside my names block (carry-forward B, sanctioned by the task + TASK-058 handoff).
