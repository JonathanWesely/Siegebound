# TASK-046 handoff — Bot decision loop (2 s ordered rules + placement + LogSiegeBot trace)

**Author:** gameplay-programmer · **Status:** ready-for-qa · **Compile:** NOT compiled (TASK-051 batches) · **Git:** untouched · **Board:** NOT edited (orchestrator owns it)

## Scope guarantee (parallel-safety with TASK-047)
Everything is **bot-internal**. Only two files touched:
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`

**I did NOT edit `SiegePlayerController.h/.cpp`, `SiegeGameMode.*`, `SiegeGameState.*`, or any other file.** The TASK-030 placement rules were **re-implemented bot-side** (they are short) rather than reused from the controller — no shared file was opened for write. I read the controller/game-state/game-mode purely to mirror behavior. No STOP/serialize was needed.

## What I implemented — the empty `EvaluateDecisions()` (TASK-045 shell) is now the §4 loop
Every 2 s the bot plays the **first** rule whose full conditions hold, then returns (a fired rule owns the tick; a refused spawn point simply retries next tick):

1. **Rule 1 — Defend.** If an alive **enemy (Blue) intruder** stands on the bot half (X ≥ 0) AND an affordable **defensive** card (Unit or Building) is in hand → play the **cheapest** affordable defensive card (ties prefer a Unit, which is always placeable). A **unit** spawns at the bot centerline; a **building** spawns between the nearest intruder and Castle_Red (standoff clamped outside the plinth keep-out and short of the intruder).
2. **Rule 2 — Economy.** Else if the half is clear AND alive miners `< TargetMinerCount (3)` AND `CanAddMiner()` AND an affordable **Miner** card is in hand → play Miner, spawned just in front of GoldNode_Red so it walks the last stretch and activates its +1/s there.
3. **Rule 3 — Attack.** Else if `gold ≥ AttackBankThreshold (12)` AND an affordable **Unit** card exists → play the **most-expensive affordable Unit** at the bot centerline (banking to 12 is what makes waves grow as income scales).
4. **Rule 4 — Cycle.** Else if the hand holds an **unplayable** card (Spell / HeroUpgrade / Utility — forward-compat, M4/M5) AND `gold ≥ BotDiscardCost (1)` → `SpendGold(1)` + discard the most-expensive unplayable card.
5. Otherwise: bank gold, no log line.

All plays route through `DeckComponent->ConfirmPlayFromHand(slot)` (discards through `DiscardFromHand(slot)`) and `SpendGold` on the bot's Red `ASiegePlayerState` (`GetBotPlayerState()`). Card **Cost/CardType are read from `/Game/Data/DT_Cards`** each tick — nothing hardcoded (GDD §3.0). Rule gates that are card costs (Miner ≥ 8) are data-driven affordability checks, not literals; strategic thresholds (miner target 3, attack bank 12, discard fee 1) are `UPROPERTY` defaults with `// GDD` comments (CONVENTIONS: mechanic rules aren't CSV columns).

## Spawning (Team = Red, TASK-044 recolors at BeginPlay)
`SpawnBotCardActor()` resolves the composed soft-class by CardID — `/Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C` (Unit/Economy, must be `ASummonedUnit`) or `/Game/Blueprints/Buildings/BP_Building_<CardID>.BP_Building_<CardID>_C` (Building, must be `ABuilding`). **Missing/incompatible BP → log (module category) + return nullptr with NO gold spent, no draw** (CONVENTIONS composed soft-class law). Spawn uses the controller's exact deferred pattern: `SpawnActorDeferred → SpendGold (destroy-on-fail, LAST gate) → InitUnit/InitBuilding(Team=Red, CardID) → FinishSpawning`, so gold moves **iff** the actor appears. Units are lifted by the CDO capsule half-height; buildings spawn flush. Instigator = nullptr (bot has no pawn; unit/tower team attribution resolves through the DamageCauser's own ITeamAgent, TASK-002/004 chain).

## Placement validity — re-implemented for the Red half (mirrors TASK-030)
`ComputeValidBotSpawnPoint(Desired, bIsBuilding, OutPoint)`:
- Snaps a **synthetic** desired point onto the navmesh via `UNavigationSystemV1::ProjectPointToNavigation` (generous extent `(200,200,1000)` so a guessed ground Z still finds the floor; the horizontal extent is small so the snapped point stays near intent).
- Validates on the snapped point via `IsBotHalfPointClear`: **own half X ≥ 0** (Red), **castle-plinth keep-out** (2D box, `CastlePlinthClearance = 420`, iterating `ACastle`), and — **buildings only** — **≥ 200 clearance** from every live `ABuilding` (`IsBuildingDestroyed()`-skipped).
- Searches Desired first, then a deterministic widening ring (radii 0/250/500/800/1100 × 8 dirs), first valid wins → so a clearance/plinth failure walks outward instead of stalling. No nav data in the world → degrade-open to the half/plinth rule with a warn-once latch (never fires in L_Arena).

**The final returned point is always X ≥ 0 → the bot can never spawn on the Blue half, and never plays a card it can't afford.**

## Match-active gate (TASK-045 forward-dependency)
`EvaluateDecisions` early-returns at the top when `!IsMatchActive()`, which reads `ASiegeGameMode::HasMatchEnded()` (authoritative, latched synchronously in `OnCastleDestroyedHandler` before the Victory screen). Permissive only if no `ASiegeGameMode` resolves (degenerate world). This is the **bot-internal half** of the belt-and-suspenders; TASK-047 additionally wires `StopDecisionTimer()` into the match-end freeze. Result: nothing plays under the Victory screen.

## LogSiegeBot decision trace
`DECLARE_LOG_CATEGORY_EXTERN(LogSiegeBot, Log, All)` in `SiegeBotController.h` + `DEFINE_LOG_CATEGORY(LogSiegeBot)` in the .cpp. **The category did NOT exist yet** — grep confirmed it lived only in comments/docs (TASK-045 h/.cpp comments, CONVENTIONS, board), never declared. Per the task ("if not, declare it") and CONVENTIONS Logging ("declared in the owning module"), I declared it in the bot's own header (its only user). **Exactly one `LogSiegeBot` line per completed play/discard** (rule # + card + spawn location + gold delta); all non-decision diagnostics (missing BP, no valid point this tick) go to `LogGitClaudeUnrealTest` so the trace stays one clean line per decision. Example:
`[Bot BP_SiegeBot_C_0] Rule 3 (Attack): played unit 'Knight' (cost 6) at centerline (350, -212, 0) — gold 14->8.`

## Flagged decisions for QA
1. **The enemy HERO counts as an intruder for rule 1** (alongside enemy `ASummonedUnit`), both filtered to alive + own-half. Rationale: the acceptance "player pushes onto the bot half → a defensive play within 2 s" must hold whether the player advances with summoned units OR their own hero, and the hero is a Blue `ITeamAgent` combatant standing on the bot half. Downside: while the hero camps the bot half the bot stays in rule-1 defense and doesn't build economy — which is correct defensive behavior, and "player idle" (hero at PlayerStart, Blue half) still yields rule 2 → rule 3 as specced. If QA prefers the strict Siegebound "units ≠ hero" reading, delete the second `TActorIterator<AHeroCharacter>` loop in `FindNearestEnemyIntruderOnBotHalf` — no other code changes.
2. **"Defensive play" = Unit OR Building** (a body to block, or a tower/wall to shoot/reroute). A `Wall` (Damage 0) qualifies as a defensive building; the log labels any building "building" (not "tower") for accuracy. Cheapest-affordable selection with a Unit tie-break keeps placement robust (units have no clearance constraint).
3. **"discard the most-expensive card" (rule 4) = most-expensive UNPLAYABLE card**, not most-expensive overall — cycling out a dead card without throwing away a playable unit/building. In the M3 core set (all Unit/Building/Economy) there are **no** unplayable cards, so rule 4 never fires in M3; it is pure forward-compat for M4/M5.
4. **Rule 2 "gold ≥ 8" and Rule 1's gate are data-driven affordability checks** (read Miner/defensive card Cost from DT_Cards), not the literal 8 — tracks CSV rebalances. Rule 3's "gold ≥ 12" and rule-4's "gold ≥ 1" are strategic thresholds → UPROPERTY defaults.
5. **Miner/attack spawn Y is randomized** across `±BotSpawnLaneSpread (900)` so waves fan out; the ring search then snaps to the nearest valid navmesh point. Behavior is reasoned, not position-exact.

## Things QA should scrutinize
- **C4458 inherited-reflected-member shadows:** I deliberately avoided `Owner`, `Instigator`, `Controller`, `Pawn`, `PlayerState` as local/param/loop names (bot PS local is `BotState`/`BotPS`; loop iterators `It`; row loop var `Index`/`SlotIndex`). Please re-scan — this class cost 2 loops elsewhere in the batch.
- **UE5 double/float:** `FVector` components and `FVector::Size()`/`DistSquared2D` are `double`; I widened `IsOnOwnHalf(double)`, cast `Size()` to float for the standoff math, and made the ring angle `double` to avoid C4244.
- **Gold-spend correctness:** every play checks affordability (cost ≤ gold) before spawning; `SpendGold` is the last gate with destroy-on-fail, so no path spends without producing an actor, and `ConfirmPlayFromHand` (the draw) only runs after a successful spend+spawn.
- **NavigationSystem module** is already in Build.cs (TASK-030). No Build.cs edit here.
- **Miner economy resolution:** the spawned `AMinerUnit` is Team=Red and resolves its income via `GetPlayerStateForTeam(Red)` = the bot PS (TASK-043/025), so `GetAliveMinerCount()` on `BotState` reflects it synchronously after `FinishSpawning`.

## Files
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` — LogSiegeBot decl; §4 rule/geometry UPROPERTY tunables; private helper decls.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` — LogSiegeBot def; constructor CardTableAsset default; full `EvaluateDecisions` + helpers (`IsMatchActive`, `IsOnOwnHalf`, `FindNearestEnemyIntruderOnBotHalf`, `GetCastleRedLocation`, `GetGoldNodeRedLocation`, `ComputeValidBotSpawnPoint`, `IsBotHalfPointClear`, `ResolveBotCardActorClass`, `SpawnBotCardActor`) + file-local hand-scan statics.

## Assets referenced (built by parallel/earlier tasks; all null-safe)
`/Game/Data/DT_Cards` · `/Game/Blueprints/Units/BP_Unit_<CardID>` (Footman/Archer/Knight/Miner) · `/Game/Blueprints/Buildings/BP_Building_<CardID>` (ArrowTower/Wall) · level actors `Castle_Red` (`ACastle` Team=Red), `GoldNode_Red` (`AGoldNode` Team=Red).
