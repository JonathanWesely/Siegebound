# QA Report — TASK-059

Verdict: **PASS**

Play v3: Instant/upgrade routing + Masons castle-heal + Swarm multi-spawn + stack-cap refund + DeepMine routing + PlayAgain upgrade reset.
Pre-compile review (M4 batch compiles at TASK-068). Files reviewed: `SiegePlayerController.h/.cpp`, `Castle.h/.cpp`, `SiegeGameMode.cpp` (one-liner).

Counts: **0 BLOCKER · 2 WARN · 2 NIT**

---

## Findings

- [WARN] Castle.cpp:276 (HealOverTime) — a Masons heal on the WINNER's castle that is mid-heal at match end keeps ticking for up to `MasonsHealDuration` (10 s) after `HandleMatchEnd`; `ACastle` is not in the match-end freeze sweep, and TASK-059 is scoped out of that file. **Benign** (bar fills, self-terminates ≤ Duration, cleared by `ResetCastle` on Play Again; a LOSER's castle stops via `HandleDestroyed`→`StopHealOverTime`). Carry-forward note only — no fix required in this task.
- [WARN] SiegePlayerController.cpp:1386 (SpawnUnitSwarm) — a swarm ring point whose `ProjectPointToNavigation` fails falls back to the raw ring point, so a copy can spawn slightly off-navmesh and self-path. Rare at Radius 300 around an already-validated center in the open arena; documented degrade-open. Acceptable.
- [NIT] SiegePlayerController.cpp:1263 / Castle.cpp:302 (full-castle Masons) — playing Masons on a full-HP castle spends the 8 gold + draws a replacement while `HealOverTime` clamps to a no-op (flagged item #2, ruled ACCEPTED below). Standard card-game waste behavior; a "castle already full" refusal would be an unspecified extra rule. Optional UX tweak only.
- [NIT] SiegePlayerController.h:227 (SpawnUnitSwarm home) — the shared swarm helper lives as a `public static` on `ASiegePlayerController` because the files-only constraint blocked a new shared file (flagged item #6). Acceptable — TASK-060's bot includes `SiegePlayerController.h` to call it. Consider relocating to a shared statics home (mirrors the `SiegeCombatStatics` consolidation NIT from qa/TASK-055) in a later refactor.

No BLOCKER or correctness-affecting issue found.

---

## Required confirmations (dispatch checklist)

**Melee-suppression law (qa/TASK-003 warning 2) — CONFIRMED intact.**
Instants (`HeroUpgrade`/`Utility`) route through `ResolveInstantPlay`, which NEVER calls `EnterPlacementMode` or `SetMeleeSuppressed` — no suppression to manage, no Instant path leaves suppression set. The Swarm/DeepMine changes add NO new placement-exit path: success funnels through `ExitPlacementMode()` (TryConfirmPlacement:974, which releases suppression before any early-out); every mid-confirm refusal (invalid point, 0-spawn, spend-refusal) STAYS in mode (suppression correctly retained); miner-cap / missing-BP exits still funnel through `ExitPlacementMode`. Placement entry/exit is byte-preserved from TASK-030. All exit paths documented in the header (lines 91-97) remain valid.

**Net-zero refund (§3.0) — CONFIRMED on every refusal.** Traced each branch:
- Unaffordable → refused in `PlayHandSlot` (CanAfford, line 355) BEFORE the type switch; no gold moved.
- HeroUpgrade no-pawn / `RefusedAtMaxStacks` / `RefusedInvalidCard` → refuse, NO `SpendGold`.
- Masons no-living-castle → refuse, NO spend. Masons `SpendGold` false (unreachable) → refuse, no heal/draw (false return means gold NOT deducted → net-zero).
- Other Utility CardID → refuse, no spend. Spell → M5-refused, no spend. Empty slot / no row / no PlayerState → refuse/return, no spend.
- Placement branch unchanged: invalid-point stay-in-mode no spend; miner-cap exit no spend; missing-BP exit no spend; swarm 0-spawn no spend; spend-refusal destroys EVERY spawned copy before returning (no partial-spawn gold leak, cpp:929-943).

**Apply-then-spend safety (the crux) — CONFIRMED safe.**
`CanAfford(Row.Cost)` is pre-checked in `PlayHandSlot` before the switch. In `ResolveInstantPlay` the HeroUpgrade path calls `Hero->ApplyUpgrade(CardID)` FIRST, then `SpendGold` ONLY on `EHeroUpgradeResult::Applied`, then `ConfirmInstantDraw`. There is no `RemoveUpgrade`, so the result-then-spend order is required (matches TASK-058 contract).
- Paying-without-buffing: impossible — spend happens only on `Applied` (buff already granted).
- Buffing-without-paying: only if `SpendGold` fails after `Applied`; CanAfford held in the same synchronous stack with nothing spending between, so this is a documented dead path guarded by a hard-invariant tripwire log (cpp:1231). Acceptable.
Enum values (`Applied`/`RefusedAtMaxStacks`/`RefusedInvalidCard`) and `ApplyUpgrade`→`EHeroUpgradeResult` signature verified against HeroCharacter.h:55-64/198. A dead (still-possessed) hero is deliberately allowed to buy upgrades (deferred to respawn re-apply); an unpossessed pawn → "Hero unavailable" refuse, net-zero — both safe.

**Masons `ACastle::HealOverTime` — CONFIRMED correct.**
Interval timer at `HealTickInterval` (0.2 s UPROPERTY); `HealPerTick = HealRemaining / max(Duration/Interval, 1)` → 300/50 = 6/tick over 50 ticks = exactly 300 over 10 s. Each tick clamps `NewHP = min(CurrentHP+Delta, MaxHP)` (NEVER over MaxHP) and broadcasts `FOnCastleHPChanged` only on an ACTUAL change (`Applied > 0`, mirrors TakeDamage). Restack ADDS `Total` and re-derives the rate. Self-terminates at pool-empty OR MaxHP. `StopHealOverTime` (ClearTimer + zero pool/rate) is called by `HandleDestroyed` AND `ResetCastle` — no dangling timer on a destroyed/reset castle. No-op on a destroyed castle or non-positive args (early return with log). Full-castle call zeroes the just-added pool and never arms the timer (no double-heal, no leak). `Engine/TimerHandle.h`/`TimerManager.h`/`GitClaudeUnrealTest.h` includes present.

**`SpawnUnitSwarm` static — CONFIRMED clean + consumable by TASK-060.**
Signature matches header↔cpp exactly: `static TArray<ASummonedUnit*>(UWorld*, UClass*, FName, ETeamId, AActor*, APawn*, const FVector&, int32, float)`, `public`, fully parameterized, NO gold logic (caller owns the gate). `Count>1` → even ring on `Radius`, each point navmesh-projected with fallback; `Count<=1` (`FMath::Max(1,Count)`, ring branch gated on `SpawnCount>1`) → single spawn AT Center with no reprojection = byte-for-byte with the M1/M2 single-unit path (same capsule-lift from CDO + `SiegeSpawn::` constants + deferred `InitUnit`→`FinishSpawning`). Player confirm spends ONE Cost and unwinds ALL copies on spend-failure. Bot (TASK-060) can call `ASiegePlayerController::SpawnUnitSwarm(...)` with `Instigator=nullptr`.

**DeepMine routing — CONFIRMED, no mis-routing.**
Single-source predicate `IsBuildingCard(CardID, CardType)` = `Building || (Economy && BuildingEconomyCardIDs.Contains(CardID))`, `BuildingEconomyCardIDs = {DeepMine}`. Used in all three sites (`EnterPlacementMode`→`bPendingIsBuilding`, `UpdatePlacementGhost` clearance gate, `TryConfirmPlacement` branch + `ResolveCardActorClass`) so they can never disagree: DeepMine → building clearance + navmesh + `BP_Building_DeepMine` (ABuilding); Miner (Economy, not in set) → `BP_Unit_Miner` (unit path); every other card unchanged. Miner cap keys off `MinerCardID` only, so DeepMine is correctly NOT miner-capped (matches TASK-057).

**Existing-play non-regression — CONFIRMED.**
Unit / Building / Economy(Miner) plays + TASK-030 placement v2 (validity, ghost, plinth/navmesh/clearance) untouched; Spell still M5-refused; discard path unchanged; the miner-cap entry+confirm re-gate intact. Single-unit spawn is byte-preserved through the `Count<=1` helper path.

**PlayAgain reset — CONFIRMED.** `SiegeGameMode.cpp:597-600`: `if (IsValid(TrackedHero)) TrackedHero->ResetUpgrades();` placed immediately BEFORE `RestoreHeroAtStart()`. `ResetUpgrades()` exists (HeroCharacter.h:208). Because `ResetHero` is the shared respawn+PlayAgain path, clearing stacks here means the subsequent `ResetHero` re-applies zero stacks (clean base hero) while upgrades still persist through a normal respawn — respawn-persistence undisturbed.

**C4457/58/59 shadow scan — CONFIRMED clean.** New params/locals avoid inherited reflected UPROPERTYs (`SpawnOwner`/`SpawnInstigator` not `Owner`/`Instigator`; `Slot` is not a member on APlayerController — only on UWidget). Only `Owner`/`Instigator` textual matches are the `/*Owner=*/` `/*Instigator=*/` SpawnActorDeferred comment labels. Castle helpers introduce no member-shadowing locals.

**Deprecated UE 5.8 APIs — none.** `UNavigationSystemV1::GetCurrent` / `GetDefaultNavDataInstance` / `ProjectPointToNavigation`, `GetWorldTimerManager().SetTimer/ClearTimer`, `SpawnActorDeferred`/`FinishSpawning`, `FindFunction`/`ProcessEvent`, `FInputMode*`, `FText::Format`/`NSLOCTEXT` all valid for 5.8.

**Header/cpp consistency + null-safety — CONFIRMED.** All new declarations match their definitions; `Row`/`SiegeState`/`Hero`/`FriendlyCastle`/`World`/`DeckComponent`/`UnitClass` are guarded before use; cross-task symbols verified: `InitUnit`, `InitBuilding`, `IsBuildingDestroyed`, `GetCapsuleComponent`, `CanAfford`, `SpendGold`, `GetGold`, `CanAddMiner`, `GetAliveMinerCount`, `ApplyUpgrade`, `ResetUpgrades`, `FCardRow::{DisplayName(FString), SwarmCount(int32), CardType, Cost}`, `SiegeSpawn::{DefaultCapsuleHalfHeight, SpawnGroundClearance}`.

---

## Rulings on the 2 flagged items

1. **SiegeGameMode.cpp one-liner (`TrackedHero->ResetUpgrades()` before `RestoreHeroAtStart`) — ACCEPTED / SANCTIONED.** Outside TASK-059's names block but explicitly authorized by the task spec + TASK-058 carry-forward. Correctly placed (before the shared `ResetHero` re-apply), IsValid-guarded, no other task edits `SiegeGameMode::PlayAgain`. Behaviorally correct: clears upgrades on Play Again without disturbing respawn-persistence.

2. **Full-castle Masons no-op vs refusal — ACCEPTED as coded.** The spec pre-check is "a living friendly castle," not "a damaged one." `HealOverTime` clamps to MaxHP so a full-castle play is a harmless no-op that still spends + draws — standard card-game behavior. Refusing it would add an unspecified rule (scope creep). If the manager later wants a "castle already full" refusal for UX polish, that is a one-line design tweak, not a correctness fix (recorded as NIT above).

---

## Notes for build-master (TASK-068 M4 batch)
- No `Build.cs` change; two Castle.cpp includes added (`TimerManager.h`, `GitClaudeUnrealTest.h`) — required (Castle.cpp had no timers/UE_LOG before). No new files, no new module.
- `SpawnUnitSwarm` is the shared spawn path TASK-060 (Bot v2) consumes via `SiegePlayerController.h` — do not relocate before TASK-060 lands.
- Two benign carry-forward WARNs (post-match Masons tick on the winner's castle; off-navmesh ring fallback) — informational, no action required at integration.
