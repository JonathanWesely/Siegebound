# TASK-352 — M8 P0 Networking/Replication Audit (READ-ONLY)

- **Auditor:** gameplay-programmer · 2026-07-28
- **Tree audited:** committed HEAD `8a903c7` on `main`. The TASK-349 (CASTLE-3X) live-edit lane — `Castle.{h,cpp}`, `SiegePlayerController.{h,cpp}`, `SiegeBotController.{h,cpp}`, `SummonedUnit.{h,cpp}`, `HeroCharacter.cpp`, `Config/DefaultEngine.ini` — was read at HEAD via `git show` (working tree NOT read). `SiegeNavAreas.{h,cpp}` are untracked TASK-349 new files and were NOT read (not at HEAD). `HeroCharacter.h` is clean and was read normally. All other files read from the working tree (clean = identical to HEAD).
- **Law:** CONVENTIONS "Networked 1v1 (M8)"; manager rulings 1–8 (TASKBOARD.md:1838-1847). This document is the input to TASK-353 (architecture). NO fixes were made; NO solutions are designed here beyond the §8 shortlist naming.
- ⚠️ **CODE-DELTA ADDENDUM OWED:** the CASTLE-3X team-gating layer (per-team object channels + gate blocker + per-team NavAreas, §7 below) is audited from its CONVENTIONS/rulings DESIGN only. After TASK-350 lands its code commit, this audit owes an addendum re-reading the landed `Castle/SiegePlayerController/SiegeBotController/SummonedUnit/HeroCharacter/SiegeNavAreas/DefaultEngine.ini` deltas.

---

## 0. Headline finding — the module has ZERO replication surface today

`git grep` across `Source/GitClaudeUnrealTest/Siegebound/` at HEAD for `bReplicates|SetReplicates|GetLifetimeReplicatedProps|DOREPLIFETIME|OnRep_|GetNetMode|IsLocallyControlled|ROLE_` returns **exactly two hits**, both in ACaptureZone (CaptureZone.cpp:91 `HasAuthority()`, CaptureZone.h:47 doc). Consequences in a naive two-client session today:

1. **Nothing spawned replicates.** Units, buildings, projectiles, miners, spell effects are spawned with `bReplicates` unset → they exist only on the machine that spawned them. The client sees none of the host's units and vice versa.
2. **Level-placed actors fork.** Castle_Blue/Red, GoldNode_Blue/Red, CaptureZone_Center, BattlefieldScatter exist independently on both machines with independent, unreplicated state (HP, reserves, owner, RNG seed). The client's castle HP never moves when the host damages it.
3. **PlayerState state forks.** APlayerState replicates as an actor, but `ASiegePlayerState`'s `Team`, `Gold`, miner counts are all plain UPROPERTYs (SiegePlayerState.h:273, 344, 348) — the client's copies stay at defaults (Team=Blue, Gold seeded locally) while the server mutates its own.
4. **Both machines run full simulations.** BeginPlay-armed timers run wherever the actor instance exists (see §5) — a client would accrue its own gold, tick its own clock, latch its own overtime.

Everything below details where that surfaces, class by class.

---

## 1. Single-player-assumption call sites

### 1a. `GetFirstPlayerController()` / index-0 / first-controller sites (ruling 4 — the BAN list)

| # | Site (HEAD) | What it does | MP consequence |
|---|---|---|---|
| 1 | **SummonedUnit.cpp:1118** (`UpdateState`) | Group-orders resolve: `Cast<ASiegePlayerController>(GroupWorld->GetFirstPlayerController())` → `FindUnitGroup(CommandGroupId)` every 0.25 s state tick. The known `3068286` offender. | On a listen server the FIRST controller is the HOST's. Host-side Blue units happen to work; any future Red-player unit polls the WRONG controller's `UnitGroups`. Server-side units cannot see a remote client's controller-local array at all (it lives client-side, §2 PC row). |
| 2 | **SummonedUnit.cpp:1145** (`UpdateState`) | Shield-Wall stance gate: `GetFirstPlayerController()` → `HasIssuedCommand()` / `UpdateStateStandardCommanded(*PC)`, gated `Team == ETeamId::Blue` (line 1141). | Same failure shape as #1. The stance (`CurrentCommand`/`bHasIssuedCommand`, SiegePlayerController.h:1158-1161) is controller-local unreplicated state. |
| 3 | **SiegeFeedbackLibrary.cpp:156** (`PlayLocalCameraShake`) | `UGameplayStatics::GetPlayerController(WorldContextObject, 0)` → `ClientStartCameraShake`. Called from ACastle::TakeDamage (Castle.cpp:226-228 HEAD). | Cosmetic-only, but index 0 on a listen server is always the HOST — the client never gets castle-hit shake, and the host gets shake for hits on BOTH castles. Needs a "local viewer" resolve, not controller 0. |
| 4 | **SiegeGameMode.cpp:768-789** `FindLocalSiegeController()` | Returns the FIRST `ASiegePlayerController` in the world ("the first ASiegePlayerController is THE player", comment 776-779). Consumer: `RestoreHeroAtStart()` (SiegeGameMode.cpp:394). | Server-only code (GameMode), but "first = the player" breaks with two controllers: hero respawn/Play-Again restore targets the host's controller only. |
| 5 | **SiegeGameMode.h:353-355** `TrackedHero` | Single tracked hero pawn + single `HeroRespawnTimerHandle` (h:380). Own TODO in source: *"TODO(M8): per-player hero/timer tracking for multiplayer (single-hero assumption holds through M7)"*. | Two players ⇒ two heroes; death tracking, 5 s respawn timer, and `GetHeroStartTransform` (SiegeGameMode.cpp:457, keyed off `Hero->GetTeamId()` at :433) must become per-controller. |

Near-misses that are ALREADY multiplayer-shaped (no change needed for iteration itself): `OnCastleDestroyedHandler` pushes `HandleMatchEnd` to **every** controller via `GetPlayerControllerIterator` (SiegeGameMode.cpp:234-241) — the iteration is fine; the direct C++ call is not (client controllers need an RPC/OnRep path, §2).

### 1b. Blue = local / Red = bot hardcodes (load-bearing sites only; symmetric team math excluded)

| # | Site | Assumption |
|---|---|---|
| 1 | **TeamId.h:11** | The contract comment itself: "the local player is always Blue; the enemy is Red" — retired by ruling 4. |
| 2 | **SiegeGameMode.cpp:170-176** (`InitNewPlayer`) | **EVERY real player login is tagged `SetTeam(ETeamId::Blue)`.** On a listen server the joining client would ALSO be tagged Blue → two Blue economies, `GetPlayerStateForTeam` resolves arbitrarily (first match wins, SiegeGameState.cpp:90-99). The single most load-bearing team bug for P1. |
| 3 | **SiegeGameMode.cpp:791-871** (`SpawnBot`) | A Red bot (`ASiegeBotController` + Red-tagged PS) is spawned **unconditionally** in every non-sandbox L_Arena world (BeginPlay:104). In a networked 1v1 the bot would fight alongside/instead of the Red human; bot PS + client PS would both claim Red. Needs a networked-match gate (ruling 3: standalone must stay byte-identical). |
| 4 | **HeroCharacter.h:336** | `Team = ETeamId::Blue` — and NOTHING ever assigns a hero team (no InitUnit-equivalent; grep confirms no `SetTeam` on heroes). Both heroes in MP would be Blue: friendly-fire checks, Rally targeting, castle resolve, gate-blocker channels (§7) all wrong for the client hero. |
| 5 | **SiegePlayerController.cpp:3230-3238** (`IsPointInOwnSpawnBox`) | Placement spawn box = the castle with `GetTeamId() != ETeamId::Blue` filtered out; comment: "player is always ETeamId::Blue (team contract)". Red client could never place. |
| 6 | **SiegePlayerController.cpp:3279** (`IsPointInCapturedZone`) | `Zone->CanTeamSpawnHere(ETeamId::Blue, Point)` — hardcoded Blue owner test. |
| 7 | **SiegePlayerController.cpp:1298, 1853, 2672, 2875** | Placement/targeting/instant/spell team = `Hero->GetTeamId()` with `ETeamId::Blue` fallback — inherits #4's Blue-everywhere and defaults Blue when the hero is dead/unresolved. |
| 8 | **SiegePlayerController.cpp:1082** (`HandleMatchEnd`) | `(Winner == ETeamId::Blue) ? VictoryMusic : DefeatMusic` — victory is "Blue won", not "MY team won". Same semantic risk for WBP_VictoryScreen's `SetWinner(ETeamId)` contract (SiegePlayerController.h:326-331). |
| 9 | **SummonedUnit.cpp:1141, 1520** | Command stance + group eligibility gated `Team == ETeamId::Blue` ("bot/Red never qualify", IsGroupCommandEligible comment 1513-1518). A Red human's units can never take orders. |
| 10 | **SiegeBotController.h:216** + fallback constants | `BotTeam = ETeamId::Red`; `CastleRedFallbackLocation`/`GoldNodeRedFallbackLocation` (CONVENTIONS World axes, TASK-133) — bot is Red-only by construction. Fine per ruling 3 (bot stays as practice mode) but reinforces that Red-as-human is a new path. |
| 11 | **SiegeGameMode.cpp:888** (`GrantSandboxStartingGold`) | Sandbox gold → `GetPlayerStateForTeam(ETeamId::Blue)`. Dev-only; fine if sandbox stays standalone. |
| 12 | **SiegeCheatManager.cpp:114, 154** | Cheat spawn team defaults Blue / `bRed` flag. Dev tooling — flag, don't fix for P1. |
| 13 | **CombatantHealthBarComponent.cpp:100** + SiegeFeedbackLibrary.cpp:166 (`TeamTint`) | Blue tint = friendly, Red = enemy — ABSOLUTE team colors. In MP the Red player sees their own units red. Consistent with "host=Blue/client=Red" identity colors; whether bars should be viewer-relative is a P2/P3 UX call (flag to manager — NOT a P1 item). |
| 14 | **CardHandWidget / HUD widgets** | All bind the OWNING local controller's delegates (CardHandWidget.h:91 `InitForController`) — correct shape for MP (each screen shows its own hand) once the underlying data is per-controller server state. |

### 1c. Client-authoritative gameplay mutations (the future `Server*` RPC surface)

Everything below currently executes on the machine pressing the input. On a listen server: HOST input = authority by luck; CLIENT input = local-only divergence. Per CONVENTIONS RPC law each is a `Server<Verb><Noun>` (Reliable, WithValidation) candidate on the owning controller. All line refs = HEAD `SiegePlayerController.cpp` unless noted.

| Surface | Site(s) | Mutates |
|---|---|---|
| Card play entry | `PlayHandSlot` (:575 ff) | Hand routing; affordability pre-checks read local PS |
| Placement confirm | `TryConfirmPlacement` — `SpendGold` :1327 (buildings) / :1370 (units, swarm-unwind), `SpawnUnitSwarm`/`SpawnActorDeferred` + `InitUnit`/`InitBuilding`, `ConfirmPlayFromHand` | Gold, world spawns, deck/hand |
| Discard | `DiscardHandSlot` — `SpendGold(DiscardCost)` :731, `DiscardFromHand` | Gold, deck/hand |
| Spell target confirm | `TryConfirmSpellTarget` — `SpendGold` :1859, refund `AddGold` :1887, `USpellLibrary::ResolveSpell` (world damage/freeze/buffs/gold-steal) | Gold + arbitrary world combat state |
| Instant spell | `ResolveSpellInstant` — :2661/:2691 spend/refund + resolve | Gold + world |
| Hero upgrades | `ResolveInstantPlay` — `ApplyUpgrade` then `SpendGold` :2836-2841 | Hero stats (client-owned pawn), gold |
| Masons | `ResolveInstantPlay` — `SpendGold` :2888 + `ACastle::HealOverTime` | Castle HP (level actor!) |
| Group orders | `BeginGroupPick` :2040, `ConfirmGroupPickStage` :2179 (builds `FSiegeUnitGroup`, steals members, pushes `AssignCommandGroup` to units), `PruneUnitGroups` :2525 (1 s timer armed :209), `ClearAllUnitGroups` :2563 | Controller-local `UnitGroups` + per-unit `CommandGroupId`/station offsets |
| Stance latch | `SetUnitCommand` (T/E handlers) | Controller-local stance the units poll |
| Hero melee | HeroCharacter.cpp:211 `DoMeleeAttack` → :317 `UGameplayStatics::ApplyDamage` from IA_Attack | Enemy HP, from a client-controlled pawn |
| Rally | HeroCharacter.cpp:359 — iterates friendlies, `ApplyMoveSpeedBuff` | Other units' movement state |
| War Banner aura | HeroCharacter.cpp:747/:760 pulse timer → `SetAuraDamageBonus` | Other units' damage state |
| Play Again | WBP_VictoryScreen → `ASiegeGameMode::PlayAgain` (SiegeGameMode.cpp:495) | ENTIRE world reset. GameMode does not exist on clients — the client's button finds nothing. Needs a Server RPC relay + replicated reset. |
| Deck build | PC BeginPlay :137-151 — `LoadGameFromSlot(SiegeDecks)` → `IsDeckLegal` → `SetPendingDeckList` → `BuildAndShuffle` | Deck/hand, from a CLIENT-LOCAL SaveGame (§6) |
| Cheats | `USiegeCheatManager` (installed PC ctor, HEAD :77) | Dev-only; must be authority-gated eventually — flag, not P1 |

**Constructed client-side that must become server-authoritative:** every `SpawnActorDeferred`/`SpawnActor` in the confirm paths (units/buildings/projectiles via towers on whichever machine runs them), spell resolution (`USpellLibrary::ResolveSpell` — pinned signature, SpellLibrary.h; today called by the client PC AND the server bot), and the group-order `FSiegeUnitGroup` store itself (UnitCommand.h:70-115 — owned by the controller, so today it lives on whichever machine the controller is local to; server-side units can never read a remote client's array).

---

## 2. Replication-needs matrix (per class)

Legend — **Rep:** properties needing `DOREPLIFETIME`(+condition) with `OnRep_*`; **Srv:** stays server-only; **Cos:** client-cosmetic (drive from OnReps). Phase = where the M8 ladder lands it (P1 per ruling 5; everything else P2/P3).

| Class | Rep (what + OnRep reaction) | Srv (authority-only) | Cos (client-side, OnRep-driven) | Phase |
|---|---|---|---|---|
| **ACastle** (HEAD Castle.{h,cpp}) | `bReplicates`; `CurrentHP` (+`MaxHP` if tunable) → `OnRep_CurrentHP` fires the EXISTING `OnCastleHPChanged` delegate (Castle.h:37/67-69 — bar + HUD unchanged); `bDestroyed` → hide/collision/`OnCastleDestroyed` visuals; `CrumbleStage` (int32, Castle.h:267) → `ApplyCrumbleStage` on the way up; `Team` (COND_InitialOnly — level-placed, but belt-and-braces for the client's TakeDamage-side checks) | `TakeDamage` scaling + friendly-fire resolve (Castle.cpp:161 instigator chain), Masons heal timer (:428-443), crumble threshold math | Hit-flash component, camera shake (via a client-side reaction, replacing SiegeFeedbackLibrary.cpp:156), debris VFX, HP-bar widget | **P1** |
| **ASiegePlayerState** | `Team` (COND_InitialOnly → recolor/team resolves on clients — TODAY unreplicated, so every client-side copy reads Blue); `Gold` (COND_OwnerOnly per TASK-356 spec) → `OnRep_Gold` fires `OnGoldChanged`; display rate + miner count (owner-only) or derive client-side from replicated inputs | The ENTIRE accrual engine: `BeginPlay`'s `ResetGold`→`StartIncomeTimer` (SiegePlayerState.cpp:16, :163-169) and `HandleGoldTick` (:121) MUST be authority-gated — today they run on client copies too (§5.1); `SpendGold`/`AddGold`/`AddIncome`/miner registry all `HasAuthority()`-guarded | HUD gold/rate/miner text (already delegate-driven, seed-then-bind) | **P1** |
| **ASiegeGameState** | Match clock (replicated float or server start-timestamp; today client Tick free-runs its own clock — SiegeGameState.cpp:35); `bOvertimeActive` → `OnRep` fires `OnOvertimeStarted` + sting (today client would latch + play sting on its OWN clock, :51-62); match-ended/winner state (new — today match end lives in GameMode `bMatchEnded` + a direct C++ call chain) | Clock accumulation, overtime latch decision, `GetPlayerStateForTeam` (iterates replicated PlayerArray — works client-side ONCE `Team` replicates) | Clock/overtime HUD (delegate-driven) | **P1** |
| **ASiegeGameMode** | n/a — server-only by engine design | Everything: win handler (cpp:201), freeze (:250), respawn (:364-455 — needs per-player rework, §1a#4/#5), `PlayAgain` (:495), `SpawnBot` (:791 — needs networked-match gate), `InitNewPlayer` (:159 — needs host=Blue/client=Red). `HandleMatchEnd` push (:238) must become a Client RPC / GameState-OnRep path for remote controllers | — | **P1** |
| **ASummonedUnit** (HEAD) | `bReplicates` + movement replication (ACharacter); `Team` + `CardID` (COND_InitialOnly → client-side BeginPlay recolor SummonedUnit.cpp:243 + stat display); `CurrentHP` → `OnRep` fires `OnHPChanged` (h:126-128, bar unchanged); death → ragdoll/hide; anim/facing: state-driven anim + `SkeletalVisualMesh` handling client-side; group assignment (`CommandGroupId` — only if P2 keeps unit-side resolve; the TASK-353 owning-controller design decides) | State machine (0.25 s `UpdateState`, timers :824/:965/:1070), targeting, damage application (:2402 TakeDamage instigator resolve), buffs/freeze bookkeeping | Lunge visual, hit-flash, mesh juice, damage numbers, overhead bar | **P2** (P1 touches ONLY the two GetFirstPlayerController sites) |
| **AHeroCharacter** (h clean; cpp HEAD) | Already an ACharacter under a PlayerController — pawn/movement replication comes with `bReplicates`; `Team` (must be ASSIGNED from PS at possess/restart — today never set, §1b#4); `CurrentHP`/effective-max → `OnRep` → `OnHPChanged`; upgrade stacks (owner-relevant HUD + server combat math); rally cooldown (owner HUD) | Melee damage application (cpp:317 → `ServerMeleeAttack`-shaped), Rally/War-Banner target iteration (:359/:760), regen, death/respawn via mode | Swing montage, impact VFX, `ClientStartCameraShake` (already client-scoped TASK-016 pattern) | **P2** (team assignment itself is P1 — gate (b) of TASK-357) |
| **ABuilding + ATower/ABarracks/ADeepMine** | `bReplicates`; `Team`/`CardID` (InitialOnly → recolor Building.cpp:169); `CurrentHP` → `OnRep` → `OnHPChanged`; destroyed state | Tower scan/fire loop (Tower.cpp:151), Barracks spawn/lifetime timers (Barracks.cpp:56/70), DeepMine income registration (DeepMine.cpp:79 → the OWNING team's server PS), freeze timers (Building.cpp:134) | Muzzle/impact VFX, bars | **P2** |
| **AGoldNode** | `bReplicates` (level-placed; still needs state rep); `GoldReserve`/`bDepleted` → `OnRep` drives the glow gauge + `OnMineDepleted` UI side | Drain tick (GoldNode.cpp:346), miner registry, `FindBestMineFor` (server callers only: miners + bot) | Glow intensity lerp | **P2** |
| **ACaptureZone** | `CaptureOwner` → `OnRep` → `ApplyOwnerColorToDecal` — the gap is ALREADY DOCUMENTED in source (CaptureZone.cpp:88-90: "a pure client would not run this and, with CaptureOwner unreplicated, would not tint — an accepted M8 gap") | Capture eval timer — ALREADY `HasAuthority()`-gated (CaptureZone.cpp:91-99). The model citizen; the only pre-M8 class that took the guard. | Decal tint | **P2** |
| **AProjectile** | Cosmetic-replication choice per TASK-353: either replicate the actor (movement) or keep server-only + multicast spawn-FX. Carries `Team` (Projectile.h:212), homing target, damage — all server-relevant | Homing tick, impact damage (`USiegeDamageType_*` scaling), terrain-destroy law | Trail/impact VFX | **P2** |
| **AMinerUnit** | Inherits ASummonedUnit's needs; arrival state (visual) | Mine walk/poll (MinerUnit.cpp:120), income registration against `GetPlayerStateForTeam` via `ResolveOwningPlayerState` (MinerUnit.h:214-222 — correct team-keyed shape, works server-side once teams are right) | Mining loop SFX | **P2** |
| **ASiegeBattlefieldScatter** | **The chosen RNG seed must replicate/be server-sent** (task-board named item): today `GenerateScatter` runs from BeginPlay (BattlefieldScatter.cpp:152) + the mode's reset path (SiegeGameMode.cpp:693) with `OverrideSeed=0` ⇒ random per machine (h:129-134) — host and client would generate DIFFERENT layouts: mismatched visuals, mismatched HISM collision vs server navmesh, client hero collides with obstacles the server doesn't have. Rep: `ChosenSeed` (+ generation index for Play-Again re-rolls) → `OnRep` = `ClearScatter`+`GenerateScatter(seed)` | Traversability validation, navmesh interaction | The scatter render itself | **P1-recommended** (see shortlist #7 — strictly the P1 gate text doesn't require it, but the client-side collision mismatch degrades the P1 session; manager call) |
| **ASiegeBotController** | None — AAIController, server-only (never travels to clients). Its auto-created `ASiegePlayerState` DOES replicate as an actor → needs `Team` replicated so client-side `GetPlayerStateForTeam` doesn't mis-resolve the bot PS as Blue (§2 PlayerState row) | Whole decision loop (2 s timer), spell casts via `ResolveSpell` (already server-side ✓) | — | **P1** only for the SpawnBot gate; bot itself unchanged (ruling 3) |
| **ASiegePlayerController** (HEAD) | Owner-only surfaces: hand/deck state once server-owned (P3), stance/prompt (owner HUD). `UnitGroups` (h:1170-1172) is THE controller-owned store — TASK-353 designs where it lives (server-side per-controller is the natural home; today client-local for a remote player) | Confirm/validation logic moves into `Server*` RPC bodies (§1c); placement VALIDITY preview stays client-side (ghost = cosmetic) with server re-validation at confirm (the confirm re-gate pattern already exists — miner cap re-check) | Ghost, reticle, pick circles, decals, HUD/Victory widgets — all local-only actors, correctly so | **P1** (team resolve + group-order poll replacement) / **P2** (full RPC surface) |
| **UDeckComponent** | Lives on both controllers. In MP the PLAYER deck must be server-side (validation) — owner-only replication of hand/preview OR server-authoritative component with owner-only state push; TASK-353 designs it. Bot deck already server-side | Pile shuffles (`FMath::RandRange` — server RNG), play/discard mutations | Hand UI via existing delegates | **P3** (per phase ladder; P1/P2 can run host-authoritative deck for the host and defer client deck) |
| **Widgets** (CardHandWidget, CastleHealthBarWidget, CombatantHealthBarWidget, DamageNumberWidget, DeckBuilderWidget, SessionMenu later) | Never replicate — pure clients of the delegate layer. No changes IF OnReps fire the same delegates | — | All | — |
| **Feedback components** (SiegeHitFlash, SiegeMeshJuice, CombatantHealthBarComponent, DamageNumberActor, SiegeFeedbackLibrary) | None — cosmetic; must be TRIGGERED client-side from OnReps/multicasts instead of server-side gameplay code paths | — | All (CombatantHealthBarComponent is push-model, no polling — TASK-130) | P2 wiring |

---

## 3. Timers & latency hazards

1. **Client-side gold accrual (worst):** `ASiegePlayerState::BeginPlay` unconditionally `ResetGold()` → `StartIncomeTimer()` (SiegePlayerState.cpp:16, :163-169; 1.0 s tick h:289). PlayerStates replicate to ALL clients and BeginPlay runs there → every client copy accrues its own divergent `Gold`. Must be authority-gated + `Gold` replicated owner-only.
2. **Client-side match clock:** `ASiegeGameState::Tick` free-runs on clients (SiegeGameState.cpp:35), latches its OWN overtime (:51), plays the sting locally (:60). Two machines = two clocks drifting by join latency. Needs replicated clock basis + OnRep overtime.
3. **The 0.25 s unit state tick** (SummonedUnit.cpp:824/:1070, `StateCheckInterval` h:454): with replication ON but no authority gate, client-side unit proxies would run their OWN AI (move orders fighting replicated movement). Every AI timer body needs the authority gate when `bReplicates` lands (P2).
4. **Building/tower/economy timers:** Tower fire loop (Tower.cpp:151), Barracks spawner (Barracks.cpp:56/:70), GoldNode drain (GoldNode.cpp:346), DeepMine income retry (DeepMine.cpp:79), Castle Masons heal (Castle.cpp:439), miner poll (MinerUnit.cpp:120) — same authority-gate class of change (P2). CaptureZone already gated (CaptureZone.cpp:91) — the pattern to copy.
5. **Group-order 1 s prune** (SiegePlayerController.cpp:209 → PruneUnitGroups :2525): controller-local; under the TASK-353 owning-controller design this becomes a server-side per-controller concern. Latency shape: pick confirms are 3-stage user input → a `ServerConfirmGroupStage`-shaped flow must tolerate stage state living client-side until the final confirm (only the final group is gameplay state).
6. **Play-Again reset flow:** `PlayAgain` (SiegeGameMode.cpp:495-697) destroys/rebuilds the world server-side and calls client-facing resets directly (`HandleMatchReset`, deck resets, scatter re-roll :693). Client widgets/decks/scatter must be reset via replication/RPC; ordering law (clock reset before economy reset, :603 comment) must hold on the server only, with clients converging via OnReps.
7. **Match-end freeze:** `FreezeWorldAtMatchEnd` (:250) freezes server sim; clients must SEE frozen units (movement replication handles it once units replicate) and get income-pause via replicated gold simply not changing. The end screen (`HandleMatchEnd` :238) needs the RPC/OnRep path — a latency window where a client still issues inputs after match end must be swallowed by server-side validation (`bMatchEnded` checks exist controller-side today, PC h:1071).
8. **Deferred-tick sandbox grant** (SiegeGameMode.cpp:114 `SetTimerForNextTick`) — sandbox is standalone-only; no MP exposure if sandbox stays un-networked (flag: refuse `?Sandbox=1` under listen server, or accept it as host-only dev tool).

---

## 4. Already replication-friendly vs poll-based

**Friendly (the good news — the UI layer survives M8 intact):**
- **Push-model delegate law everywhere:** `FOnCastleHPChanged` (Castle.h:37), `FOnCombatantHPChanged` via IHealthBarProvider (SummonedUnit.h:126-135, Building.h:73-78, HeroCharacter.h:124-133), `FOnGoldChanged`/`FOnGoldRateChanged`/`FOnMinerCountChanged` (SiegePlayerState.h:17-39), `FOnMatchClockChanged`/`FOnOvertimeStarted` (SiegeGameState.h:18-28), deck delegates (DeckComponent.h:24-34), refusal/prompt delegates (SiegePlayerController.h:33-65). `OnRep_*` handlers can fire these SAME delegates client-side — zero widget changes (the TASK-356 spec's "OnRep driving the existing bar/delegate path" is already the natural fit).
- **UCombatantHealthBarComponent is push, not poll** (TASK-130 rebuild; CombatantHealthBarComponent.h:22-23 "There is NO poll timer").
- **Single-choke-point mutations:** `SetGold` private choke (SiegePlayerState.h:309-315), `TakeDamage` single entries, `InitUnit`/`InitBuilding`/`InitProjectile` deferred-spawn contracts (server-side spawn slots in cleanly), `SpawnUnitSwarm` static + fully parameterized (SiegePlayerController.h:363-377).
- **Team-keyed resolves, not player-keyed:** `GetPlayerStateForTeam` (SiegeGameState.cpp:83), `ResolveOwningPlayerState` (MinerUnit.h:214-222), `FindBestMineFor` (GoldNode.h:153) — all survive host/client teams once `Team` replicates and is assigned correctly.
- **ACaptureZone** — HasAuthority-gated with the M8 gap documented in source (CaptureZone.cpp:88-99).
- **Self-documented M8 debts:** SiegeGameState.h:48-49 ("M8 multiplayer must revisit clock/overtime replication"), SiegeGameMode.h:353 (per-player hero TODO(M8)).

**Poll-based / needs conversion:**
- The group-orders unit-side poll (SummonedUnit.cpp:1114-1153) — polls the FIRST controller every state tick (ruling 4's named offender). Replacement designed at TASK-353.
- The stance poll (`HasIssuedCommand` live-read each tick, same site).
- `GetPlayerStateForTeam` warning-spam shape on clients before Team replicates (SiegeGameState.cpp:106-108) — cosmetic but noisy.
- Placement validity scans (obstacle/building clearance, navmesh projection) are per-frame client polls — fine as CLIENT PREVIEW; the authoritative re-check must run server-side at confirm (P2).

---

## 5. Team model today → host=Blue/client=Red requirements

**Today:** Castles/nodes/zones are level-placed with `Team` set per instance (Castle.h:151-152). `InitNewPlayer` tags every human Blue (SiegeGameMode.cpp:174). `SpawnBot` tags the bot PS Red (:858). Heroes default Blue and are never assigned (§1b#4). Units/buildings take Team from their spawner: player confirms pass the hero's team (Blue-fallback), the bot passes `BotTeam=Red`. `ASiegePlayerState::Team` is unreplicated.

**A host=Blue/client=Red mapping requires (facts, not design):**
1. Ordinal/PostLogin team assignment replacing the unconditional Blue tag (GameMode — clean file).
2. `Team` replicated on ASiegePlayerState (InitialOnly) so client-side resolves and HUD tinting work.
3. Hero team assigned from its owning controller's PS at possess/restart (HeroCharacter/GameMode).
4. Every §1b Blue literal in the player-command path keyed off the OWNING controller's team (PC :1298/:1853/:2672/:2875/:3238/:3279; SummonedUnit :1141/:1520).
5. Bot suppressed (or explicitly co-existing) in networked matches so exactly one PS per team resolves (`GetPlayerStateForTeam` returns FIRST match — two Red claimants would be order-dependent).
6. Win/lose presented as own-team-relative (PC :1082 + `SetWinner` widget contract).
7. Spawn geometry: PlayerStart is Blue-side only (SiegeGameMode.h:316-318); the Red human needs a Red-side start (level + `GetHeroStartTransform` already supports castle-relative fallback keyed by team, SiegeGameMode.cpp:457-479).

---

## 6. SaveGame deck locality (P3 shape note)

`USiegeDeckSaveGame` persists to the LOCAL machine (`Saved/SaveGames/SiegeDecks.sav`, slot law SiegeDeckSaveGame.h:33-37). The owning controller loads it at BeginPlay and pushes `SetPendingDeckList` before `BuildAndShuffle` (HEAD SiegePlayerController.cpp:137-151; legality gate `UDeckLibrary::IsDeckLegal`). In MP the server cannot read the client's save file — the deck must travel client→server once at join/PostLogin. Favorable facts: `FDeckList`/`FDeckCardEntry` are UPROPERTY FName/int32/FString structs (DeckTypes.h) — RPC-serializable as-is; `IsDeckLegal` is a static table-driven validator usable server-side as the `WithValidation` body; `SetPendingDeckList` is already the single injection seam on the (future server-side) DeckComponent. P3 owns the design (phase ladder ruling 6).

---

## 7. The two NAMED newest systems (ruling 7)

**7a. Group orders (TASK-344)** — covered throughout: the store is controller-owned `UnitGroups` (SiegePlayerController.h:1170-1178) + per-unit `CommandGroupId`/station offset (SummonedUnit.h:313-341); units resolve it via `GetFirstPlayerController` each 0.25 s tick (SummonedUnit.cpp:1114-1127). Under authority split, server-side units cannot read a remote client's controller-local array at all — the owning-team-controller resolve (TASK-353) plus server-side group state is the replacement. The 3-stage pick itself (circles, wheel, prompts) is correctly client-cosmetic; only the stage-3 confirmed group is gameplay state.

**7b. CASTLE-3X team gating (DESIGN-ONLY audit — code not at HEAD; addendum owed post-TASK-350).** Per CONVENTIONS "Castle 3× HOLLOW" + CASTLE-3X ruling 1: physical lane = `ECC_SiegeTeamBlue`/`ECC_SiegeTeamRed` object channels stamped on unit capsules at BeginPlay + `ACastle::GateBlockerVolume` BeginPlay-configured (Block enemy channel / Ignore own); pathing lane = `UNavArea_{Blue,Red}CastleInterior` via an `InteriorNavModifier` + `UNavFilter_Team{Blue,Red}` as unit AI default filters. Networked hazards the P1/P2 design MUST cover even before the code lands:
1. **BeginPlay-vs-Team ordering:** both lanes read `Team` at BeginPlay. Server-side deferred spawn (`InitUnit` before `FinishSpawning`) preserves that; but on CLIENT proxies BeginPlay fires when the actor channel opens, and `Team` must be there (InitialOnly rep or spawn-data) or the capsule gets stamped with the WRONG team channel client-side.
2. **Client-side collision truth for the HERO:** the hero is a client-controlled ACharacter — CharacterMovement predicts locally. If the client's own capsule channel or the gate blocker's per-team response isn't mirrored client-side, the client hero walks into the enemy castle locally and rubber-bands. The blocker's BeginPlay configuration must produce the SAME response matrix on both machines (castle `Team` is level-authored so this should hold — but it must be verified in the addendum, along with hero channel stamping timing at possess).
3. **Nav filters are server-only** (AI paths on the server) — safe by construction, but ONLY once unit AI is authority-gated (§3.3); a client-side proxy running MoveTo would bypass the filter.
4. **Crumble-stage mesh swaps replicate as visual state (§2 ACastle)** and the gating lives on ACTOR components by design ("mesh/crumble swaps can never disable it") — replication-friendly; the crumble UCX door-gap law is collision content, identical both sides.
5. `DefaultEngine.ini` at HEAD has no netcode config and no new channels yet (checked: GlobalDefaultGameMode + `RuntimeGeneration=Dynamic` are the relevant rows). TASK-349 owns the ini; any net config lands at TASK-357 (parallel law d).

---

## 8. Risk-ranked P1 shortlist (minimum set for the TASK-357 gate)

Gate restated (ruling 5): two clients, one `L_Arena` match, identical authoritative core state — castle HP both sides, own gold, match start/win/lose. Effort classes: **[NEW]** new-file-only (dispatchable under the parallel law), **[SHARED]** existing-file edit (M8 single-owner), **[SHARED-349]** in TASK-349's live lane ⇒ **blocked on TASK-350's code commit**.

| # | Item | Files | Effort / blocking | Risk if skipped |
|---|---|---|---|---|
| 1 | **Team assignment at PostLogin (host=Blue, client=Red) + PS `Team` replication + SpawnBot networked-match gate** | `SiegeGameMode.{h,cpp}` (login/spawn), `SiegePlayerState.{h,cpp}` (Team rep) | [SHARED] — clean files, NOT in the 349 lane; single-owner within M8 (TASK-356's named files) | Both players Blue + a bot claiming Red ⇒ every team-keyed resolve ambiguous; gate (b) fails outright |
| 2 | **Gold server-authoritative:** authority-gate the PS income timer + all mutation sites; replicate `Gold` COND_OwnerOnly; `OnRep_Gold` → `OnGoldChanged` | `SiegePlayerState.{h,cpp}` | [SHARED] — clean, not 349-lane | Client accrues its own divergent gold; gate (d) fails |
| 3 | **ACastle replication:** `bReplicates`, `CurrentHP`/`bDestroyed`/`CrumbleStage`(+`Team` InitialOnly), OnReps firing the existing delegates | `Castle.{h,cpp}` | **[SHARED-349] — blocked on TASK-350** | Castle HP identical-on-both-screens (gate c) impossible |
| 4 | **Match flow replication:** clock basis + overtime + match-ended/winner on GameState (OnReps → existing delegates); `HandleMatchEnd`/`HandleMatchReset` reach REMOTE controllers via RPC/OnRep; Play-Again entry becomes a server-routed request | `SiegeGameState.{h,cpp}`, `SiegeGameMode.{h,cpp}` (+ the controller-side receiving seam → the PC files are [SHARED-349]) | [SHARED] for GameState/GameMode; the PC seam **[SHARED-349]** | Gates (e)/(f) fail: end screens/Play-Again only on host |
| 5 | **`GetFirstPlayerController` P1-scoped replacements** (ruling 4 minimum): the two SummonedUnit sites → owning-team-controller resolve, byte-identical in standalone; the SiegeFeedbackLibrary index-0 shake → local-viewer resolve | `SummonedUnit.{h,cpp}` **[SHARED-349]**; `SiegeFeedbackLibrary.cpp` [SHARED, clean] | Blocked on TASK-350 for the unit files | Ban violation ships into P1; group orders break the moment a second controller exists |
| 6 | **Hero team from owning PS at possess/restart** (+ Red-side start resolution) | `HeroCharacter.{h,cpp}` **[SHARED-349]**, `SiegeGameMode.cpp` [SHARED] | Blocked on TASK-350 (HeroCharacter.cpp dirty) | Gate (b) "team visuals correct on both screens" fails for the client hero; CASTLE-3X gating channels stamp wrong |
| 7 | **Scatter seed server-sent** (`bReplicates` + `ChosenSeed` OnRep → regenerate) | `BattlefieldScatter.{h,cpp}` | [SHARED] — clean, not 349-lane | Not in the literal gate text, but host/client fight different battlefields (collision + navmesh mismatch for the client hero). Recommend P1; manager may defer to P2 explicitly |
| 8 | **Session plumbing** — `USiegeSessionSubsystem` + `USessionMenuWidget` (already specced as TASK-354) | New files only | **[NEW]** — dispatchable pre-350 | No way to reach the two-client session at all |

**Explicitly NOT P1** (scope fence, ruling 5): unit/hero/building/projectile replication (beyond #5/#6), combat authority, placement/spell/discard/group-order Server RPCs (#1c table = the P2 surface), deck handoff (P3), viewer-relative team colors (P2/P3 UX flag), cheat-manager gating.

---

## 9. Flags for the manager (non-blocking)

1. **SpawnBot gating semantics** (shortlist #1): "networked match" needs a detectable condition (URL option from `HostListenMatch()` vs `NumPlayers`-based). Standalone byte-identity (ruling 3) is easiest with an explicit travel option — TASK-353 should pin it.
2. **Two-Red ambiguity is order-dependent, not fail-fast** (`GetPlayerStateForTeam` first-match, SiegeGameState.cpp:90-99) — worth a defensive log/ensure when duplicate team tags appear.
3. **Victory-screen winner semantics** (§1b#8): `SetWinner(ETeamId)` widget contract needs an own-team-relative interpretation on each screen — a widget-contract note for the P1 art/UI seam, not a code redesign.
4. **Team-color viewer-relativity** (§1b#13): decide "identity colors" (host sees self blue, client sees self red — zero work) vs "viewer-relative" (each sees self blue — new work). Recommend identity colors for M8, matching the host=Blue/client=Red law.
5. **Sandbox under listen server:** suggest refusing/ignoring `?Sandbox=1` in networked sessions (one log line) rather than auditing the sandbox path for MP.
