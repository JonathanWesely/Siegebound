# QA Report — TASK-060

**Task:** Bot v2 — play Set II + treat upgrades/instants as discard + rule-4 discard hardening (C++)
**Reviewer:** qa-reviewer · **Date:** 2026-07-04 · **Compile state:** NOT compiled (pre-compile gate; M4 batch at TASK-068)
**Files reviewed:** `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` / `.cpp` (only files changed)

## Verdict: PASS

Blockers: 0 · Warnings: 1 · Nits: 1

Cross-checked against: `### TASK-060` spec, `handoffs/TASK-060.md`, `handoffs/TASK-046.md` (loop being extended), `handoffs/TASK-059.md` (SpawnUnitSwarm contract), and the live headers for `SpawnUnitSwarm`, `DeckComponent`, `SiegePlayerState`, `Castle`, `GoldNode`, `Building`, `SummonedUnit`, `HeroCharacter`, `FCardRow`/`ECardType`.

---

## Findings

- **[WARN] SiegeBotController.cpp:806-809 (via SpawnUnitSwarm) — swarm-fan never-Blue-half is contingent, not hard-clamped.** The invariant "no swarm copy on X<0" holds only because the *validated* Center.X ≥ SwarmSpawnRadius. `ComputeValidBotSpawnPoint` guarantees Center.X ≥ 0, not ≥ 300. For MilitiaMob (the only SwarmCount>1 card, Count=4, Radius=300) the desired Center is the centerline `BotCenterlineSpawnX = 350`; in L_Arena that point always navmesh-projects, so Center.X = 350 and copies span X ∈ [50, 650] — all Red half, claim holds. The only path to a copy at X<0 is: the centerline projection *fails* AND the widening ring relocates Center to X ∈ [0, 300) (e.g. the radius-250 ring's 180° sample at X=100 → a copy at X=-200). That requires the open-arena centerline to be off-navmesh, which does not occur in L_Arena. This is the identical shared-helper property already qa-passed for the player in TASK-059, and the handoff flags it with a hard clamp available. Not an engine-crash / not a real-play violation → WARN, not BLOCKER. **Suggested fix (optional, follow-up):** clamp each ring copy's X to ≥ BotHalfBoundaryX inside the bot path, or have SpawnUnitSwarm take an own-half clamp, to make the guarantee independent of the 350-vs-300 relationship.

- **[NIT] SiegeBotController.cpp:497-499 — rule-4 decision log prints "for %d gold" unconditionally.** Inside the `DiscardFromHand()==true` block the LogSiegeBot line reports `for BotDiscardCost gold` even on the unreachable `SpendGold==false` tripwire path (where no gold actually moved). Cosmetic only; the path is unreachable (the `Gold >= BotDiscardCost` gate holds the same tick, income only adds) and is separately tripwire-logged. No action required.

---

## Invariant confirmations (the two hard ones)

**1. NEVER plays an unaffordable card — CONFIRMED.** Every play selector filters affordability at selection: `FindCheapestDefensiveCard` (`Cost > Gold` → skip), `FindMostExpensiveUnitCard` (`Cost > Gold` → skip), `FindAffordableMinerCard` (`Cost <= Gold`), `FindAffordableEconomyBuildingCard` (`Cost <= Gold`). `SpendGold(Cost)` is the LAST gate in both spawn paths with destroy-on-fail; `ConfirmPlayFromHand` (the draw) runs only after `SpawnBotCardActor` returns non-null (i.e. after a successful spend). Nothing spends between selection and SpendGold in a single tick (income only adds), so the spend cannot fail; if it ever did, the actor(s) are unwound and no draw occurs. Gold moves iff an actor appears. Byte-for-byte with TASK-046.

**2. NEVER spawns on the Blue half (X<0) — CONFIRMED for the center of every play; contingent for swarm copies (see WARN).** `IsBotHalfPointClear` rejects any point with `!IsOnOwnHalf(X)` (Red → X ≥ 0), applied to the navmesh-projected Center of every rule. For SwarmCount>1 the individual ring copies are not half-clamped inside `SpawnUnitSwarm`; they stay Red-half in L_Arena because the validated Center is the centerline (350) and Radius (300) < 350. Documented as WARN with an optional hard clamp.

## M3-core non-regression (TASK-046 preserved) — CONFIRMED

- **Rules 1 (defend) / 2a (Miner) / 3 (attack) selection + geometry:** unchanged. The only delta to the spawn call is the added `SwarmCount` argument; core-card selectors and the intruder/centerline/GoldNode geometry are identical.
- **Placement validity** (own-half X ≥ 0, plinth keep-out `CastlePlinthClearance = 420`, building `BuildingClearance = 200`, deterministic widening ring, no-navdata warn-once degrade): `ComputeValidBotSpawnPoint` / `IsBotHalfPointClear` byte-for-byte.
- **Match-active gate** (`IsMatchActive` → `!HasMatchEnded()`, permissive only when no SiegeGameMode resolves): unchanged.
- **Rule 2 fall-through:** the new 2b sits *between* 2a and rule 3, but in M3 the deck has no DeepMine, so `FindAffordableEconomyBuildingCard` always returns INDEX_NONE → 2b is a no-op and control falls through to rule 3 exactly as in TASK-046. If 2a's cap gate holds but no Miner is in hand, control falls through to 2b then rule 3 (return only on an actual play). No M3 behavior change.
- **Single-unit spawns (Footman/Archer/Knight/Miner):** SwarmCount=0 → `FMath::Max(1,0)=1` → `SpawnUnitSwarm` Count≤1 branch = single unit at the validated point with the same capsule lift (now owned by the helper). Byte-for-byte.
- **ResetBot / Stop/StartDecisionTimer / LogSiegeBot category:** untouched.

## Rule-4 hardening (closes TASK-046 WARN-2) — CONFIRMED GENUINELY FIXED

Order in `EvaluateDecisions` rule 4:
1. Fee guarded in the condition: `CardIndex != INDEX_NONE && Gold >= BotDiscardCost (1)` → **never charged at 0 gold.**
2. **Discard FIRST:** `if (Deck->DiscardFromHand(Chosen.Slot))` — `DiscardFromHand` returns `bool` (verified in DeckComponent.h:137). The fee is charged **only inside** the true branch.
3. **Charge on success only:** `SpendGold(BotDiscardCost)` runs only after the discard actually happened; a no-op discard (false return) skips the charge entirely and emits no decision line → **no gold bleed.**
4. **At most once:** the block `return`s after firing → **never double-charged, no re-entry within a tick.**
The unreachable `SpendGold==false` path is tripwire-logged on `LogGitClaudeUnrealTest` (separate category, so the "one LogSiegeBot line per fired rule" invariant is preserved).

## SwarmCount via SpawnUnitSwarm (TASK-059) — CONFIRMED

- **Signature match:** bot calls `ASiegePlayerController::SpawnUnitSwarm(World, ActorClass, CardID, BotTeam, /*SpawnOwner=*/ this, /*SpawnInstigator=*/ nullptr, SpawnPoint, FMath::Max(1, SwarmCount), SwarmSpawnRadius)` — matches the public static declaration in SiegePlayerController.h:227 exactly (arg types: UWorld*, UClass*, FName, ETeamId, AActor* [`this` is-a AActor via AController], APawn* [nullptr], const FVector&, int32, float). Public + static → accessible.
- **MilitiaMob → 4 Red copies for one cost:** SwarmCount=4 passed at rules 1/3 (`Chosen.Row->SwarmCount`); the helper fans 4 copies on the SwarmSpawnRadius circle; gold is spent once as the last gate.
- **Single units byte-for-byte:** Count≤1 branch spawns a single unit at Center; miners/DeepMine pass explicit `/*SwarmCount=*/ 0`.
- **Team material Red:** `BotTeam = Red` passed; `SpawnUnitSwarm` sets Team before `FinishSpawning` (TASK-059) so MI_TeamColor_Red applies at BeginPlay (TASK-044).
- **Gold last-gate destroy-on-fail across ALL copies:** spawn swarm first → `SpendGold` → on refusal the unwind loop `Destroy()`s every `IsValid` copy → no partial-spawn leak; exactly one Cost iff ≥1 actor appears.
- **SpawnPoint is the GROUND point** (helper applies the capsule lift); the old local lift + `CapsuleComponent.h`/`SiegeSpawnConstants.h` includes were removed — grep confirms no residual `SiegeSpawn::`/`CapsuleComponent`/`GetCapsuleComponent` reference in the bot cpp. Include swap (added SiegePlayerController.h) is clean and ODR-safe.

## Set II reach + Deep Mine routing — CONFIRMED

- **Rule 1 defensive** selects new towers (BombTower/BallistaTower/Barracks/Wall/ArrowTower) via `IsDefensiveType = Unit||Building` — all CardType Building → `bOutIsBuilding` routes them down the building spawn path. Affordability gate intact.
- **Rule 3 attack** selects new units (Ogre/Cavalry/Pikeman/Longbowman/Cleric/MilitiaMob) via `FindMostExpensiveUnitCard` (CardType Unit). Banking threshold 12 preserved → growing waves incl. Ogres.
- **Rule 2b Deep Mine** matched by `CardType==Economy && Cost<=Gold && BuildingEconomyCardIDs.Contains(CardID)` with `BuildingEconomyCardIDs = {DeepMine}`. **Miner excluded** (Economy but not in the set → stays in 2a as a unit). DeepMine routed with `bIsBuilding=true` → `/Game/Blueprints/Buildings/BP_Building_DeepMine`, ABuilding-checked. **No miner-cap interaction** in 2b (no `CanAddMiner`/`GetAliveMinerCount`). Miner (2a) gate/selection/geometry/log unchanged.

## Discard classification — CONFIRMED

`IsUnplayableByBot = Spell || HeroUpgrade || Utility`. Bot's HeroUpgrade cards (SharpenedBlade/PlateArmor/SwiftBoots/WarBanner) and Masons (Utility/Instant) are the rule-4 discard set — never selected by any play rule (not Unit/Building/Economy). The bot has no hero, so these are correctly cycled, never played. (Whether Masons' DT_Cards row is actually typed Utility is a TASK-061 data concern; the classification logic is correct given the type contract.)

## Ruling on the flagged decision (type-unplayable-only vs unaffordable-but-playable) — ACCEPTABLE per §4

The programmer classifies **only** type-unplayable cards (Spell/HeroUpgrade/Utility) as rule-4 discards and deliberately excludes "unaffordable-but-type-playable" cards (e.g. a banked Ogre at 11 gold). **I accept this.** Rationale:

1. **The spec's "unaffordable → unplayable" requirement is satisfied by the play rules, not the discard rule.** Rules 1-3 only ever select an affordable card, so the bot never plays an unaffordable card (the hard invariant holds). Treating unaffordable as "unplayable this tick" lives correctly in the play selectors.
2. **The alternative directly contradicts a stated acceptance criterion.** Rule 3 fires at gold ≥ 12 and the bot *banks toward* Ogres (cost 12); adding unaffordable Units to the discard set would cycle away the very Ogre the bot is saving for, breaking "attacks with growing Set II waves (incl. Ogres)." The two readings are mutually exclusive and the programmer chose the one consistent with acceptance.
3. **No unaffordable-hand deadlock exists.** The bot starts at 50 gold and MaxGold ≫ the most expensive card (Ogre = 12), and idle income is always positive, so gold monotonically accrues to afford any card in hand — an all-unaffordable hand cannot persist. Banking always terminates.
4. **The only residual soft-stall is pre-existing and astronomically rare, and this task makes it *less* likely.** A hand of 6 non-DeepMine buildings with no enemy intruder (rule 1 idle), no unit (rule 3 idle), no miner/DeepMine (rule 2 idle), and no type-unplayable card (rule 4 idle) would bank forever. That case existed identically in M3/TASK-046 (rule 4 never fired in M3), is not introduced here, and normal play (any player push → rule 1, or a mixed hand) turns the hand over. TASK-060 actively cycles HeroUpgrade/Utility cards it draws, *improving* turnover. A hard-stall breaker (e.g. discard the most-expensive card overall when no rule can fire for N ticks) is a reasonable **future follow-up**, not a blocker for this task.

## Compile-safety scan (pre-compile gate)

- **C4458/C4457 shadow (AAIController reflected members):** new param `SwarmCount`, locals `SwarmUnits`/`SwarmUnit`/`Card` do not shadow PlayerState/Pawn/Instigator/Owner/Controller; new members `SwarmSpawnRadius`/`BuildingEconomyCardIDs` shadow nothing. `/*Owner=*/`/`/*Instigator=*/` are comment labels, not declarations. Clean.
- **C4244 narrowing:** `FMath::Max(1, SwarmCount)` int/int; `SwarmSpawnRadius` float→float; `static_cast<float>(ToIntruder2D.Size())` explicit; ring math writes doubles into (double) FVector components. No new double→float narrowing.
- **Deprecated UE 5.8 APIs:** none. `UNavigationSystemV1::GetCurrent/ProjectPointToNavigation/GetDefaultNavDataInstance`, `TActorIterator`, `GetTimerManager().SetTimer/ClearTimer`, `GetAuthGameMode`, `SpawnActorDeferred`, `TSoftClassPtr/TSoftObjectPtr::LoadSynchronous`, `FindRow`, `GetPlayerState<T>` all current.
- **Null-safety:** `Deck`/`BotState`/`CardTable`/`World`/`ActorClass` null-checked; `HandCards[].Row` guaranteed non-null (only added when FindRow succeeds); `NearestIntruder` dereferenced only inside `if(NearestIntruder)`; swarm unwind IsValid-checks each copy; deferred building null-checked.
- **Header/cpp consistency:** `SpawnBotCardActor` gained `int32 SwarmCount` in both header (:327) and definition (:751); new UPROPERTYs declared in header; all helpers declared. Consistent.
- **One LogSiegeBot line per fired rule:** each rule emits exactly one LogSiegeBot line on a completed play/discard and returns; all diagnostics (missing BP, no valid point, zero-spawn, SpendGold tripwire) go to LogGitClaudeUnrealTest. Confirmed.

---

## Notes for build-master (PASS)

- Untouched-file guarantee holds: only `SiegeBotController.h/.cpp` changed; the bot *calls* `ASiegePlayerController::SpawnUnitSwarm` (public static) — no edit to SiegePlayerController or any other class.
- This is the LAST M4 code task; it depends at link time on TASK-059's `SpawnUnitSwarm` and the Set II card behaviors (TASK-054-057) — batch-compile at TASK-068 will surface any cross-task drift. No new module / no Build.cs change.
- Runtime assets referenced (built by editor tasks, all null-safe with logged skips): `/Game/Data/DT_Cards`, `/Game/Blueprints/Units/BP_Unit_<CardID>` (incl. MilitiaMob/Ogre/etc.), `/Game/Blueprints/Buildings/BP_Building_<CardID>` (incl. DeepMine), `MI_TeamColor_Red`, level actors `Castle_Red`/`GoldNode_Red`.
- WARN-1 (swarm-fan half clamp) is a recommended follow-up, not a build blocker.
