# TASK-1190 — README stats census (read-only)

**Role:** gameplay-programmer · **Date:** 2026-09-09 · **Instrument:** text reads of `Source/**`, `Config/**`, `Docs/**`, plus a binary tagged-property readback of three `.uasset` files (no editor, no MCP, no compile, no engine process launched, no Git write).
**Git instant:** `HEAD = 8c444ca` ("fog visual updates", Jonathan, 2026-09-09 12:44:22 −0700; `git show --stat` = pipeline docs, `Tools/SuiteRunnerFixtures/*`, `Tools/run_suite_bounded.ps1` only — no `Source/**`, `Content/**`, `Config/**` or `Docs/**`). Dirty at my finish: `CONVENTIONS.md`, `TASKBOARD.md`, `CLAUDE.md`, `Tools/Packaging/ship.ps1` (the concurrent `TASK-1193` ship) plus untracked `TASK-1193` handoff/QA files. ⇒ every cited `Source/**`, `Content/**`, `Config/**`, `Docs/**` line is `HEAD` content; the one working-tree-only citation is `ship.ps1` (both line sets given).
**Law:** `SHIP-§3a` (CONV:7184) · `SC-§97` (CONV:4869) · `SC-§101` (CONV:4824) · `SC-§102` (CONV:4810) · `SC-§104` (CONV:4756) · `SC-§109` (CONV:4679) · `SC-§110` (CONV:4662) · `TL-§5b` (CONV:5864).

## Citation key

| Key | Path |
|---|---|
| `SB/` | `Source/GitClaudeUnrealTest/Siegebound/` |
| `CSV:n` | `Docs/Data/cards.csv` line *n* (line 1 = header; rows on lines 2–35) |
| `DT` | `Content/Data/DT_Cards.uasset` — value-level readback of the 34 tagged-property rows (this census, §1) |
| `IMC` | `Content/Input/IMC_Hero.uasset` — decoded `Mappings` array, 28 entries (this census, §4a) |
| `DA` | `Content/Data/DA_BattlefieldScatter.uasset` — decoded `Layers[7]` (this census, §6.5) |
| `CONV:n` | `.claude/pipeline/CONVENTIONS.md` line *n* |
| `TB:n` | `.claude/pipeline/TASKBOARD.md` line *n* |
| `GDD:n` | `Docs/GDD.md` line *n* |
| `ENG/` | `C:/Program Files/Epic Games/UE_5.8/Engine/Source/` |
| `PC` | `SB/SiegePlayerController.{h,cpp}` · `HC` = `SB/HeroCharacter.{h,cpp}` · `SU` = `SB/SummonedUnit.{h,cpp}` · `GM` = `SB/SiegeGameMode.{h,cpp}` · `PS` = `SB/SiegePlayerState.{h,cpp}` · `GS` = `SB/SiegeGameState.{h,cpp}` · `BS` = `SB/BattlefieldScatter.cpp` · `SCfg` = `SB/ScatterConfig.h` · `FV` = `SB/FogVolume.{h,cpp}` · `SL` = `SB/SpellLibrary.cpp` · `CH` = `SB/SiegeControlsHelpWidget.cpp` · `DB` = `SB/DeckBuilderWidget.cpp` · `Bot` = `SB/SiegeBotController.{h,cpp}` |

A cell reading `NOT MEASURED` may not be typed into the README (§11 lists them all with the reason).

---

## 1. cl. 1 — CSV ↔ asset sync: **MEASURED, PROCEED**

| Fact | Value | Citation |
|---|---|---|
| Last commit touching `Docs/Data/cards.csv` | `1a457df` 2026-09-04 21:03:28 −0700 ("TASK-987: the fog batch ships") | `git log -- GitClaudeUnrealTest/Docs/Data/cards.csv` (root one level up, `SC-§102`); prior: `1aa0fee`, `20db517`, `6e79a24`, `0d717c0` |
| Last commit touching `Content/Data/DT_Cards.uasset` | `1a457df` (the same commit) | `git log -- GitClaudeUnrealTest/Content/Data/DT_Cards.uasset`; prior: `1aa0fee`, `3e4773a`, `80c47e8`, `0d717c0` |
| Working-tree state of both | clean (neither appears in `git status --porcelain`) | measured |
| Asset commit ≥ CSV commit | **yes (equal)** ⇒ proceed | — |
| Value-level proof (stronger than commit order) | All **34 rows × 31 columns** decoded from the asset agree with the CSV: every numeric, bool, string and enum cell matches; the only textual difference is that enum cells are stored with the C++ namespace (`ECardType::Unit` for CSV `Unit`, `ESpellDelivery::Auto` for a blank cell, `ESpellEffect::None`), which is the same value. `DeckCount` sums to **50** in the asset. `NoticeRange` reads **0.0 on all 34 rows** in the asset. `CardArt` cells were not decoded (soft-path storage) — the CSV cell is cited for art paths. | `DT` readback (parser: scratchpad `uasset_tags.py`, `run_dtcards.py`; row block at byte 7427, `NumRows = 34`) |
| Asset's embedded import record | `FileMD5 = ef265e94f2853a62e89260f0fd313467`, `Timestamp = 1784711117` (≈ 2026-07-22) vs current CSV MD5 `b71e57340536e95e4f99b3905a9b4769` | `DT` `AssetImportData` string; **informational only** — the record was not refreshed by the 09-04 reimport path, and the value-level readback above is the proof that the rows are current |
| Row type | `FCardRow`, "Property names MUST match the CSV header columns 1:1" | `SB/CardRow.h:182-186` |

---

## 2. Global rules and match structure

### 2.1 Flow, win and loss

| Rule | Value | Citation |
|---|---|---|
| Boot map / arena | `/Game/Maps/L_MainMenu` boots; `/Game/Maps/L_Arena` is the match; game mode `ASiegeGameMode` | `Config/DefaultEngine.ini:10-12`; `GM.cpp:74` |
| Play (vs Bot) | opens the arena; a bot opponent is spawned on the Red side | `GM.cpp:1595-1617` (`StartMatch`), `GM.cpp:1669-1765` (`SpawnBot`) |
| Sandbox (No Bot) | opens the arena with `?Sandbox=1`; no bot; Blue is granted **9999** gold on the first tick | `GM.cpp:1628-1646`, `GM.cpp:89`, `GM.h:569`, `GM.cpp:1765-1798` |
| Win / loss | when a castle is destroyed the match ends and the *other* team wins; the world is frozen; Victory/Defeat screen with Play Again | `GM.cpp:523-572` (`OnCastleDestroyedHandler`, `Winner = other team`), `GM.cpp:618` (`FreezeWorldAtMatchEnd`), `GS.cpp:213-214`; screen strings `Victory!` / `Defeat` / `Play Again` in `Content/UI/WBP_VictoryScreen.uasset` |
| Play Again | full reset: gold/economy, hero upgrades, capture zone, castle, scatter re-generated with a fresh seed | `GM.cpp:1305` (`PlayAgain`), `GM.cpp:1422-1424` (zones), `GM.cpp:1527` (`ResetUpgrades`), `PS.cpp:478` (`ResetEconomy`), `SB/BattlefieldScatter.h:211` (`bReRandomizeOnMatchReset = true`) |
| Match clock / overtime | overtime starts at **420 s** (7:00) and doubles base income | `GS.h:180`, `GS.cpp:124-126`, `PS.h:336` |
| Hero death | the hero does not lose the match; the player possesses a **ghost pawn** and the hero respawns after **180 s** at its own castle (offset (1500, 0, 100) from the castle, clearance 300) | `GM.cpp:732-772` (`HandleHeroDied`), `GM.h:461` (`HeroRespawnDelay = 180`), `GM.cpp:780` (`SpawnAndPossessGhost`), `GM.h:536`, `GM.h:558` |
| Card play while dead | `PlayHandSlot` carries no hero-alive check; `IsGhostPossessed()` has no caller in the controller | `PC.cpp:960-980`, `PC.cpp:1773` (definition only) — *whether the ghost's camera can complete a placement in practice is NOT MEASURED* |

### 2.2 Gold economy

| Rule | Value | Citation |
|---|---|---|
| Starting gold | **10** | `PS.h:312` |
| Base income | **+1 gold every 1 s** (tick interval 1.0 s, base period 1 tick) | `PS.h:316`, `PS.h:320`, `PS.h:324`, `PS.cpp:197-217` |
| Overtime income | base × **2** (= +2/s) from 420 s | `PS.h:336`, `PS.cpp:206-213`, `GS.h:180` |
| Miner income | **+1/s per miner that has arrived at a claimable mine**; nothing until arrival; removed on death or depletion | `PS.h:340`, `SB/MinerUnit.cpp:507-560` (arrival radius 150, `SB/MinerUnit.h:484`), `SB/MinerUnit.cpp:223`, `1340-1345`, `1361` |
| Miner cap | **6 alive** per player; a 7th Miner card is refused with "Miner limit reached" and no gold is spent | `PS.h:332`, `PS.cpp:399-424`, `PC.cpp:1970-1975`, `PC.cpp:2547-2552` |
| Deep Mine income | **+2/s** the moment it is built; removed when destroyed | `SB/DeepMine.h:66`, `SB/DeepMine.cpp:91`, `SB/DeepMine.cpp:46` |
| Gold cap | **999** | `PS.h:328`, `PS.cpp:181-190` |
| HUD rate | `GetGoldRate = base(×2 in overtime) + miners + flat` | `PS.cpp:259-282` |
| Discard whole hand (H) | **20 gold flat**, however many cards; draws a full replacement; refused while placing/targeting | `PC.h:1786`, `PC.cpp:1524`, `CH:515-570` |
| Per-card discard | **retired** — `DiscardHandSlot` (1 gold, `PC.h:1767`, `PC.cpp:1399`) has no shipped caller: `UCardHandWidget::RequestDiscardSlot` (`SB/CardHandWidget.cpp:108-118`) is not bound by `WBP_CardHand` (its name table holds no discard element); the help text states the per-card lever is gone | `PC.h:1760-1767` (doc: "RETIRED WITH DiscardHandSlot … CARDBAR-§6"), `CH:551-553`, `CONV:9184` |
| Enemy reveal (war map) | **30 gold**, repeatable; refused before any spend under 30 | `SB/CommanderNpc.h:311`, `PC.cpp:6790-6792`, `PC.cpp:7128-7131` |

### 2.3 Deck and hand

| Rule | Value | Citation |
|---|---|---|
| Deck size | **exactly 50** cards (`SiegeLegalDeckSize = 50`); unknown card or negative count refused | `SB/DeckTypes.h:73`, `SB/DeckLibrary.cpp` (`IsDeckLegal`, "a legal deck is exactly %d"), `SB/DeckComponent.h:187` |
| Per-card copy cap | **abolished** — `MaxCopies` is now the hero-upgrade stack cap only | `SB/CardRow.h:204-212`, `CONV:6835` (`UNCAP-§2`) |
| Hand | **6** slots; playing/discarding draws immediately; discard pile reshuffled into the draw pile when it empties | `SB/DeckComponent.h:183`, `:192-212` |
| Next-card preview | shown (name, cost, art) | `SB/CardHandWidget.h:1-80` (doc), `WBP_CardHand` elements `Img_NextCardArt`, `PreviewNameText`, `PreviewCostText` |
| Default deck | the `DeckCount` column (sums to 50): Footman 9 · Archer 8 · Wall 4 · Knight 3 · Miner 3 · ArrowTower 3 · MilitiaMob 3 · Pikeman 3 · Cavalry 3 · Longbowman 2 · Cleric 2 · Ogre 2 · Fireball 2 · Sorcerer 2 · FrostNova 1 | `CSV:2-35` col `DeckCount`; `DT` readback sum 50 |
| Saved decks | **10 fixed slots**, per profile; the active slot is used by the next match | `SB/SiegeDeckSaveGame.h:57`, `DB:1240`, `SB/DeckBuilderWidget.h:1-120` (doc) |

### 2.4 Playing cards and placement

| Rule | Value | Citation |
|---|---|---|
| Hotkeys | `1`–`6` play hand slots 1–6 | `IMC` (`IA_Card1..6` ← `One..Six`), `PC.cpp:598-609` |
| Unit / Building / Economy | placement mode: a ghost follows the cursor; LMB confirms; RMB or Escape cancels at no cost | `CH:429-460`, `CH:578-600`, `IMC` (`IA_CancelPlace` ← `RightMouseButton`, `Escape`) |
| Legal placement regions | **(a)** a square of half-extent **7380 uu** centred on your own castle (the castle interior included); **(b)** the mid capture zone while your team owns it | `SB/Castle.h:394`, `PC.h:2109`, `PC.cpp:6074-6079` (`CanTeamSpawnHere`) |
| Building clearance | **200 uu** from any other building (grown by the placed footprint); obstacle clearance **150 uu**; units need **0** | `PC.h:2153`, `PC.cpp:5250-5274`, `PC.cpp:5463`, `PC.h:2206`, `PC.h:2235` |
| Footprint resize | mouse wheel while placing a building: ×1.0 … ×1.5 in 0.1 steps (no shrinking) | `PC.h:2287-2322`, `CH:752-753` |
| Stack upgrade | while placing a building, hover your own building of the **same card**: outline turns blue, the click makes it taller instead — height = original × (1 + n) capped at **×5**; max HP × **1.5 per upgrade** (uncapped; current HP is not repaired); price = the card's own cost | `CH:665-682`, `SB/Building.h:451`, `SB/Building.h:477`, `SB/Building.cpp:406-489`, `PC.cpp:2765-2789` ("the price is the card's OWN DT_Cards Cost") |
| Swarm circle | 4 Militia spawn spread in a **300 uu** circle | `PC.h:2330`, `Bot.h:392`, `CSV:8` |
| Spells | targeting mode with a ground reticle (default radius 150); Fireball/FrostNova use the reticle as the **aim point of a hero-origin line**; Lightning/BattleCry resolve at the reticle; Pickpocket/Fog/BrightSun are instant with no reticle | `PC.h:1866`, `SL:788-800`, `SL:602-633`, `CSV:24-35` |
| Instant cards | hero upgrades, Masons, Pickpocket, Fog, BrightSun resolve on the key press | `PC.cpp:5041-5071` (Masons), `PC.cpp:5021` (upgrade refusal), `SL:630-756` |
| Refusals | refund / never charge: spell refused (`%d gold refunded`), upgrade at cap ("no gold spent"), Masons with no castle, Miner cap | `PC.cpp:3583`, `4863`, `5021`, `5051`, `1975` |

### 2.5 Units — notice, retention, orders

| Rule | Value | Citation |
|---|---|---|
| **Notice range (all cards)** | class default `UnitEngagementRadiusUU = 5000 uu`; the CSV `NoticeRange` cell is blank on every row and the asset holds 0.0 on every row, so every unit resolves to 5000 | `SU.h:914`, `SU.h:1426`, `SU.cpp:1310`, `SU.cpp:4537-4550` (`ResolveNoticeRadiusUU`: 0 ⇒ class default), `CSV:2-35` col 31, `DT` readback |
| Notice under fog | the vision funnel applies `min(request, FogVisionCeilingUU = 609.6)` while fog is up ⇒ effective notice **609.6 uu** for every unit; towers run the same funnel with their `AttackRange`, so tower reach is fog-clamped too | `SB/SiegeFogStatics.h:268`, `SB/SiegeCombatStatics.cpp:257` (`ResolveFogClampedReachUU`), `SB/SiegeFogStatics.cpp:122` (`return FMath::Min(RequestedRadiusUU, Ceiling)`; the request is returned unchanged when fog is off), `SU.cpp:2544`, `SU.cpp:1806-1818` (`AcquireTarget` gathers within `AggroRadius`), `SB/Tower.cpp:235-238` |
| Non-combat classes | Miner and Sorcerer seal `AggroRadius = 0` in their constructors — they never acquire | `SB/MinerUnit.cpp:72`, `SB/SorcererUnit.cpp:23`, `SU.cpp:4540` |
| **Retention (leash)** | `EffectiveLeash = max(LeashRange 8000, Notice × LeashMarginMultiplier 1.5)` = **max(8000, 7500) = 8000 uu** for every card; the drop test is itself fog-clamped: `distance > min(8000, 609.6)` under fog ⇒ **609.6 uu** retention while fog is up | `SU.h:1448`, `SU.h:1470`, `SU.cpp:4573-4583`, `SU.cpp:4603-4606`, `SU.cpp:1752`, `SU.cpp:2024` |
| Attack reach under fog | `AttackRange` is fog-clamped at every in-range test ⇒ ranged units fire no farther than 609.6 while fog is up; melee (120) unaffected | `SU.cpp:1785`, `1972`, `2049`, `2167`, `2757`, `4148` |
| Target choice (Standard) | nearest hostile within notice; pawns (units/hero) preferred over structures unless the structure is nearer by more than the pawn–structure gap of **100 uu** (`TieBreakDistance`) | `SU.cpp:1795-1858`, `SU.h:1474` |
| Siege profile | walks to the nearest enemy **building**, else the castle; ignores units and the hero | `SU.cpp:1635-1638`, `SU.cpp:2737-2740`, `SU.cpp:1770`, `DB:183` (glossary) |
| Support profile | never attacks (`AttackDamage > 0 && Profile != Support` gate) | `SU.cpp:924-927` |
| Spawn default (player) | every commandable Blue unit enrols in the hero's **follow group** on spawn and does not fight until ordered; Ogre/Sapper auto-march; Miner spawns mining | `PC.cpp:4397`, `PC.cpp:4478` (`EnrollInDefaultFollowGroup`), `SU.cpp:1712-1720` (`HasIssuedCommand`), `SB/MinerUnit.h:520` |
| Bot units | auto-advance (no command layer) | `Bot.cpp:1663-1800` (`SpawnBotCardActor`), `GDD:216` |
| State tick | 0.25 s | `SU.h:1517`, `SU.cpp:1098` |
| Defend band | a defending unit engages within `castle half-width + DefendRadius 1281 uu` of its own castle | `SU.h:1513`, `SU.cpp:2650-2710` |
| Stuck watchdog | sidestep after stall (values in `SB/SiegeStuckStatics.h` — NOT re-measured here) | — |

### 2.6 Keywords and damage rules

| Rule | Value | Citation |
|---|---|---|
| Damage composition (units) | `base × Charge(×2) × Slayer(×2) × WarBanner aura(×1.2) × permanent stacks × height (ranged only)` | `SU.cpp:4703-4780` |
| Charge | first attack after **2 s** of uninterrupted movement (> 50 uu/s) deals **×2**; consumed by that attack; interrupted by stopping | `SU.h:1603-1615`, `SU.cpp:4255-4277`, `SU.cpp:4712-4717`, `DB:68` |
| Slayer | **×2** vs any target whose max HP ≥ **150** (units, buildings, castle, hero) | `SU.h:1619-1623`, `SU.cpp:4723-4725`, `SU.cpp:4736` (`GetTargetMaxHP`), `DB:71` |
| Suicide (Sapper) | detonates once on reaching attack range **or** on death: `Damage × permanent multiplier` as Siege-typed radial damage over `AoERadius`, then dies | `SU.cpp:2766`, `SU.cpp:5526-5556`, `DB:144` |
| Swarm | one play spawns `SwarmCount` copies in a 300 uu circle | `SB/CardRow.h:260`, `PC.h:2330` |
| Chain (Crystal Tower) | instant zap through up to `ChainTargets` enemies, `Damage − ChainFalloff × hit`, each bounce within **350 uu** of the previous target | `SB/Tower.h:114`, `SB/Tower.cpp:388-501`, `DB:150` |
| Siege damage type | **×2** vs castle and buildings | `SB/Castle.cpp:1141-1143`, `SB/Building.cpp:558-560` |
| Projectile damage type | **×0.5** vs castle (full vs units/buildings/hero) | `SB/Castle.cpp:1145-1147`, `SB/DamageTypes.h:38-40`, `DB:194` |
| Spell damage type | **×0.5** vs castle | `SB/Castle.cpp:1149-1151`, `DB:197` |
| Hero melee vs castle | **×1** (plain `UDamageType`, unscaled) | `HC.cpp:641`, `SB/Castle.cpp:1139-1153` |
| Friendly fire | castle ignores damage from its own team; every radial (splash/blast/spell) damage gathers **hostile** agents only; melee and towers acquire through the same hostile funnel | `SB/Castle.cpp:1120-1124`, `SB/SiegeCombatStatics.cpp:441-486` (`ApplyRadialDamage` → `GatherHostileAgents(World, Team, …, IncludeVeiled)`), `SB/SiegeCombatStatics.h:267`, `SB/Tower.cpp:238` |
| Height advantage | `×(1 + 0.10 × (attackerZ − targetZ) / 152.4)` when the attacker is higher — **ranged attackers only**, continuous (not stepped) | `SU.h:1687`, `SU.h:1700`, `SU.cpp:4681-4690`, `SU.cpp:4772-4778`, `CONV:7505` (`HIGH-§1`) |
| Projectiles | homing, **1500 uu/s**, impact radius 30, destroyed on impact; blocked by actors tagged `Terrain`/`Obstacle` | `SB/Projectile.h:150`, `:159`, `SB/Projectile.cpp:411-447` |
| Ranged splash | a unit's projectile carries its row `AoERadius` (Wizard 250) | `SU.cpp:4863`, `SB/Tower.cpp:382` |
| Building freeze | FrostNova freezes units and buildings for `EffectDuration`; Barracks stop spawning while frozen | `SL:286-330`, `SB/Building.cpp:111`, `SB/Barracks.cpp:85` |

### 2.7 Castle

| Rule | Value | Citation |
|---|---|---|
| Max HP | **2000** | `SB/Castle.h:359` (`MaxHP = 2000.0f`), `:885` (`CurrentHP`) |
| Crumble stages | at **75 % / 50 % / 25 %** HP | `SB/Castle.h:757-765`, `SB/Castle.cpp:1320-1400` |
| Damage scaling | see §2.6 (Siege ×2, projectile ×0.5, spell ×0.5, melee ×1) | `SB/Castle.cpp:1106-1160` |
| Repair | Masons: **+300 HP over 10 s** (tick 0.2 s), refused with no castle standing | `PC.h:1741`, `PC.h:1745`, `PC.cpp:5041-5071`, `SB/Castle.cpp:1565-1642`, `SB/Castle.h:724` |
| Furnishings | per castle: **6 torches** and **1 commander NPC**, spawned by the castle | `SB/Castle.h:563`, `SB/Castle.cpp:152-153`, `SB/Castle.cpp:714` |
| Spawn box | half-extent 7380 (the placement region) | `SB/Castle.h:394` |
| Positions | Blue castle at X = −25000, Red at +25000 (scatter fallback / bot constant; the level's actual actor transform was not decoded) | `BS:1458-1462`, `Bot.h:477` |

### 2.8 Hero

| Rule | Value | Citation |
|---|---|---|
| HP | **200**; out-of-combat regen **5 HP/s** after **8 s** | `HC.h:1270`, `HC.h:1274-1278`, `HC.cpp:249-255` |
| Speed | walk **500**, sprint **750** (hold Shift); jump Z-velocity **600**; step 50; walkable slope 50° | `HC.h:1103`, `:1107`, `:1126`, `:1118-1122`, `HC.cpp:448-459` |
| Melee (LMB) | **20** damage to every enemy within **150 uu** inside a **60°** cone (half-angle 30°), **0.5 s** cooldown | `HC.h:1208-1220`, `HC.cpp:479-641` |
| Rally (Q) | friendly units within **600 uu** get **+25 % move speed for 5 s**; **20 s** cooldown | `HC.h:1224-1236`, `HC.cpp:683-742` |
| Recall (B) | **10 s** channel; cancelled by moving more than **25 uu**; on completion teleports home and heals to full effective max | `HC.h:1328`, `HC.h:1341`, `HC.cpp:1506-1615`, `GM.cpp:479` |
| Upgrades | Sharpened Blade **+10 melee/stack**, Plate Armor **+100 max HP/stack and heals 100**, Swift Boots **+25 % walk & sprint**, War Banner **aura 600 uu, friendly units +20 % damage**, pulsed every 0.5 s; caps = row `MaxCopies` (2 / 2 / 1 / 1); a copy beyond the cap is refused with no gold spent; upgrades survive death and reset only on Play Again | `HC.h:1285-1305`, `HC.h:545`, `:883-892`, `HC.cpp:1019-1067`, `HC.cpp:1329-1340`, `PC.cpp:5021`, `GM.cpp:1527`, `CSV:20-23` |
| Death | ghost pawn (moves/looks with `IMC_Hero`; cannot attack or rally — those actions are bound on the hero only) for 180 s | `SB/SiegeGhostPawn.cpp:507-524`, `SB/SiegeGhostPawn.h:1-120` (doc), `GM.h:461` |

### 2.9 Fog and Bright Sun (the two global spells)

| Rule | Value | Citation |
|---|---|---|
| Fog scope | battlefield-wide, no reticle, no radius | `SB/CardRow.h:80-88`, `DB:131` |
| Fog duration | **300 s**; a re-cast **refreshes** the expiry (never stacks) | `FV.h:999`, `FV.cpp:351` (`RaiseFog`), `SB/CardRow.h:96-101` |
| Fog refused | while a Bright Sun prevention window is up (no gold spent) | `SL:686-695`, `FV.cpp:351-360` |
| Vision ceiling | **609.6 uu** (20 ft) on notice, attack reach and leash while fog is up (§2.5); transmittance 2 % at the ceiling | `SB/SiegeFogStatics.h:268`, `:325` |
| Render floor | while fog is up the volumetric-fog render floor is enforced (grid pixel 16, grid Z 64, volumetric fog on) regardless of the player's graphics setting | `FV.h:877-883`, `FV.h:1041` |
| Bright Sun | clears fog instantly and **prevents new fog for `120 s + 60 s × floor(H / 1524 uu)`**, H = the caster team's living hero height above the flat-ground datum (`ArenaGroundReferenceZUU = 0`), sampled once at the cast, uncapped; with no living hero the window is the base 120 s | `FV.h:1059`, `:1072`, `:1094`, `:1139`, `FV.cpp:556-625` (`FloorToFloat` at 620) |
| Bright Sun refused | if the new window would be **shorter** than the live one (no gold spent) | `FV.cpp:406-420`, `SL:743` |
| One-way door | when the window ends the field **stays clear**; fog never returns on its own | `FV.h` class doc (state diagram, "SHIELDED expires to CLEAR"), `DB:165` |
| Dev commands | `Siege.Fog.Raise` / `Siege.Fog.Clear` / `Siege.Fog.Status` — Development builds only (§4d) | `FV.cpp:56-58`, `:232-250` |

### 2.10 Invisibility (the Witch's veil)

| Rule | Value | Citation |
|---|---|---|
| Target | the nearest friendly unit inside the Witch's **400 uu** position circle that is currently visible to the enemy | `SU.cpp:3322-3335`, `SU.cpp:3406`, `SU.cpp:3430`, `SU.h:1593` |
| Cast | **3 s**, interruptible, one at a time | `SU.h:1574`, `SU.cpp:3496` |
| Effect | a veiled unit is skipped by enemy target acquisition and by the enemy's war-map reveal; it stays attackable | `SU.cpp:1806` (`SuppressVeiled`), `PC.cpp:7185-7195`, `SB/SiegeInvisibilityStatics.h:1-60` (doc) |
| Break | permanently on **Attack**, **Heal**, **Mine** (enum values); never self-restores; only a new Witch cast re-veils | `SB/SiegeInvisibilityStatics.h` enum `ESiegeVeilBreakReason` (values at `:29`, `:41`, `:61`), `CONV:9836` (`WITCH-§3`) |
| Known gap | the veil's shimmer is visible to the enemy (`J-W18`, ruling open) | `CONV:10020`, `TB:1935` |

---

## 3. Cards — all 34 rows

Columns are the CSV/asset cells (`DT` readback agrees with every cell). `Notice` and `Leash` are the C++ derivations of §2.5 and are identical for every unit card: **notice 5000 uu (609.6 under fog), leash 8000 uu (609.6 under fog)**; they do not apply to buildings, spells, upgrades, Masons, or the sealed Miner/Sorcerer. `MaxCopies` is the hero-upgrade stack cap only (§2.3). `Deck` = copies in the default deck (0 = not in it).

| # | CardID (line) | Type | Cost | HP | Dmg | Range | Cadence | Speed | Profile | Ranged | Keywords / extra cells | Deck | MaxCopies |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Footman (`CSV:2`) | Unit | 9 | 80 | 12 | 120 | 1.0 | 400 | Standard | no | — | 9 | 12 |
| 2 | Archer (`CSV:3`) | Unit | 12 | 45 | 10 | 2100 | 1.2 | 350 | Standard | yes | — | 8 | 10 |
| 3 | Knight (`CSV:4`) | Unit | 18 | 200 | 15 | 120 | 1.2 | 300 | Standard | no | — | 3 | 6 |
| 4 | Miner (`CSV:5`) | Economy | 24 | 30 | 0 | 0 | 0 | 350 | None | no | AggroRadius sealed 0 | 3 | 4 |
| 5 | ArrowTower (`CSV:6`) | Building | 15 | 150 | 15 | 900 | 1.5 | 0 | None | yes | — | 3 | 8 |
| 6 | Wall (`CSV:7`) | Building | 12 | 300 | 0 | 0 | 0 | 0 | None | no | — | 4 | 10 |
| 7 | MilitiaMob (`CSV:8`) | Unit | 15 | 25 | 6 | 120 | 1.0 | 400 | Standard | no | SwarmCount 4 | 3 | 6 |
| 8 | Pikeman (`CSV:9`) | Unit | 15 | 100 | 30 | 120 | 1.5 | 350 | Standard | no | bSlayer | 3 | 6 |
| 9 | Sapper (`CSV:10`) | Unit | 15 | 60 | 80 | 120 | 1.0 | 500 | Siege | no | bSuicide, AoERadius 250 | 0 | 4 |
| 10 | Cavalry (`CSV:11`) | Unit | 21 | 140 | 20 | 120 | 1.0 | 600 | Standard | no | bCharge | 3 | 4 |
| 11 | Longbowman (`CSV:12`) | Unit | 18 | 70 | 18 | 3600 | 1.5 | 300 | Standard | yes | — | 2 | 4 |
| 12 | Cleric (`CSV:13`) | Unit | 18 | 90 | 8 | 400 | 1.0 | 350 | Support | no | heals (Damage = heal rate) | 2 | 3 |
| 13 | Ogre (`CSV:14`) | Unit | 36 | 500 | 35 | 120 | 1.5 | 250 | Siege | no | — | 2 | 2 |
| 14 | BombTower (`CSV:15`) | Building | 24 | 180 | 25 | 800 | 2.5 | 0 | None | yes | AoERadius 250 | 0 | 4 |
| 15 | BallistaTower (`CSV:16`) | Building | 21 | 120 | 45 | 1400 | 3.0 | 0 | None | yes | MinRange 300 | 0 | 4 |
| 16 | Barracks (`CSV:17`) | Building | 30 | 250 | 0 | 0 | 0 | 0 | None | no | SpawnCardID Footman, SpawnInterval 8, Lifetime 60 | 0 | 3 |
| 17 | DeepMine (`CSV:18`) | Economy | 45 | 200 | 0 | 0 | 0 | 0 | None | no | +2 gold/s (C++) | 0 | 2 |
| 18 | Masons (`CSV:19`) | Utility | 24 | 0 | 0 | 0 | 0 | 0 | None | no | +300 HP / 10 s (C++) | 0 | 3 |
| 19 | SharpenedBlade (`CSV:20`) | HeroUpgrade | 18 | 0 | 0 | 0 | 0 | 0 | None | no | +10 melee / stack | 0 | **2 (cap)** |
| 20 | PlateArmor (`CSV:21`) | HeroUpgrade | 18 | 0 | 0 | 0 | 0 | 0 | None | no | +100 max HP / stack, heals 100 | 0 | **2 (cap)** |
| 21 | SwiftBoots (`CSV:22`) | HeroUpgrade | 15 | 0 | 0 | 0 | 0 | 0 | None | no | +25 % speed | 0 | **1 (cap)** |
| 22 | WarBanner (`CSV:23`) | HeroUpgrade | 24 | 0 | 0 | 0 | 0 | 0 | None | no | aura 600, +20 % | 0 | **1 (cap)** |
| 23 | Fireball (`CSV:24`) | Spell | 21 | 0 | 100 | 0 | 0 | 0 | None | no | AoEDamage, AoERadius 300, delivery **HeroLine** | 2 | 3 |
| 24 | FrostNova (`CSV:25`) | Spell | 18 | 0 | 0 | 0 | 0 | 0 | None | no | Freeze, AoERadius 350, EffectDuration 4, delivery **HeroLine** | 1 | 3 |
| 25 | Lightning (`CSV:26`) | Spell | 24 | 0 | 200 | 0 | 0 | 0 | None | no | TopTargetsDamage, AoERadius 700, MaxTargets 3 | 0 | 2 |
| 26 | BattleCry (`CSV:27`) | Spell | 15 | 0 | 0 | 0 | 0 | 0 | None | no | AllyBuff, AoERadius 400, EffectDuration 8 | 0 | 3 |
| 27 | Pickpocket (`CSV:28`) | Spell | 18 | 0 | 0 | 0 | 0 | 0 | None | no | GoldSteal 10 | 0 | 2 |
| 28 | CrystalTower (`CSV:29`) | Building | 27 | 150 | 15 | 800 | 1.5 | 0 | None | no | ChainTargets 3, ChainFalloff 5 | 0 | 3 |
| 29 | Wizard (`CSV:30`) | Unit | 24 | 45 | 15 | 2100 | 1.6 | 350 | Standard | yes | AoERadius 250 | 0 | 4 |
| 30 | Sorcerer (`CSV:31`) | Unit | 60 | 70 | 0 | 0 | 0 | 350 | Standard | no | empowerer; AggroRadius sealed 0 | 2 | 2 |
| 31 | WatchTower (`CSV:32`) | Building | 30 | 250 | 0 | 0 | 0 | 0 | None | no | climbable (C++) | 0 | 3 |
| 32 | Witch (`CSV:33`) | Unit | 50 | 70 | 0 | 400 | 0 | 350 | Support | no | Range 400 = veil circle | **0 — inert vs the bot** | 2 |
| 33 | Fog (`CSV:34`) | Spell | 50 | 0 | 0 | 0 | 0 | 0 | None | no | FogCover, EffectDuration 300 (cell not read by the fog arm; `FV.h:999` is the authority) | **0 — inert vs the bot** | 2 |
| 34 | BrightSun (`CSV:35`) | Spell | 60 | 0 | 0 | 0 | 0 | 0 | None | no | FogClear, EffectDuration 120 (cell not read; `FV.h:1059` is the authority) | **0 — inert vs the bot** | 2 |

Spell delivery: rows 23–24 pin `HeroLine` in their `SpellDelivery` cell; every other row is blank = `Auto`, which resolves to a ground circle for every non-AoEDamage/Freeze effect (`SB/CardRow.h:157-181`, `SL:780-800`).

### 3.1 Per-card notes (C++ behaviour beyond the cells) and the shipped description text

The **shipped description text** is the deck-builder glossary (`namespace SiegeboundCardGlossary`, `DB:71-268`) plus stat lines the builder composes from the row (`DB:1360-1614`: `Health: N`, `Damage: N per attack (splash: every enemy within R units of the hit)`, `Attacks every Ns`, `Range: N units (fires a homing shot | melee - it must close to contact | instant hit … | it has to reach its target to detonate)`, `Blind spot: …`, `Move speed: N units per second`). The `Notes` CSV cell is designer prose and is read by no code (`SB/CardRow.h:229-231`).

| Card | Behaviour (code) | Shipped glossary line(s) |
|---|---|---|
| Footman / Knight / Pikeman / MilitiaMob / Cavalry | melee (must close to 120); Standard targeting §2.5; commandable | stat lines; Pikeman + `DB:71` (Slayer); Cavalry + `DB:68` (Charge); MilitiaMob + `DB:147` (Swarm, "%d of them … 300-unit circle") |
| Archer / Longbowman / Wizard | homing projectiles at 1500 uu/s; **50 % vs castle**; ranged height bonus (§2.6); reach fog-clamped; Wizard shots splash 250 | stat lines + `DB:194` (`ScalingRangedVsCastle`) |
| Miner | walks to the best claimable mine; +1/s after arrival (150 uu); cap 6; retargets on depletion; commandable with miner semantics (T mine nearest, E idle in castle, R/F mine inside circle, C follow); never attacks | `DB:76` (`MinerRole`) |
| Cleric | Support: never attacks; heals the most-hurt friendly within **400 uu** at **8 × 0.1 HP every 0.1 s = 8 HP/s** (clamped to max HP); healing breaks its own veil; keeps healing while following | `DB:256` (`ProfileSupportFmt`, args Range 400 / Damage 8) — `SU.h:1549`, `SU.cpp:2996-3043` |
| Ogre / Sapper | Siege: ignore units and the hero, march to the nearest enemy building else the castle; **×2 vs castle/buildings**; not commandable; Sapper detonates once (80 in 250, Siege-typed) | `DB:183` (`ProfileSiege`), `DB:191` (`ScalingSiege`), `DB:144` (`SuicideFmt`) |
| ArrowTower / BombTower / BallistaTower / CrystalTower | fire on their own at the **nearest enemy unit or hero** within range — never at castles/walls/structures; Ballista cannot hit inside 300; Bomb splash 250; Crystal chain 15/10/5 within 350 bounce radius, instant | `DB:130` (`StructureRole`), `DB:133` (`TowerRole`), `DB:150` (`ChainFmt`) — `SB/Tower.cpp:209-336` (acquire: units/hero only), `:244` (min range) |
| Wall / WatchTower / Barracks / DeepMine (structures) | stationary, block the ground; WatchTower: **no attack**, a **1200 uu** platform reached by a ladder (contact radius 350, own team only), giving ranged units on top the height bonus (×1.787 vs a ground target at platform height, from §2.6's formula); Barracks: a free Footman every **8 s**, self-destructs at **60 s**, summoned units stay; DeepMine +2/s | `DB:130`; Barracks `DB:153`, `DB:156`; DeepMine `DB:79`; WatchTower `SB/ClimbableTower.h:670`, `:770`, `:87` |
| Masons | +300 castle HP over 10 s; refused with no castle, no gold spent | `DB:82` |
| Sharpened Blade / Plate Armor / Swift Boots / War Banner | §2.8 | `DB:45`, `DB:48`, `DB:51`, `DB:54`, tail `DB:127` (`UpgradeTailFmt`, "your hero can hold %d of these") |
| Fireball | **100** damage along a **hero-origin line 900 uu long, 100 uu half-width**, sweeping over 0.3 s, through walls and bodies; castle takes 50 %; a bolt that hits nothing is still spent; the bot fires it from its castle (muzzle height 150) | `DB:175` (`DeliveryHeroLine`), `DB:197` (`ScalingSpellVsCastle`) — `SB/SpellLineSweep.h:284-314`, `SB/SpellLineSweep.cpp:100-248`, `SL:165-230` |
| Frost Nova | freezes every enemy unit and building the same 900×100 line catches for **4 s** (castle unaffected) | `DB:175` — `SB/SpellLineSweep.cpp:235-248` |
| Lightning | **200** damage to the **3 highest-current-HP** enemies (units, buildings, hero) within **700 uu** of the reticle; the castle is excluded | `DB:178` (`DeliveryGroundCircle`) — `SL:344-445` |
| Battle Cry | friendly units within **400 uu** of the reticle: **+50 % attack speed, +25 % move speed for 8 s** | `DB:178` — `SL:461-521`, `SU.h:1632-1636` |
| Pickpocket | steals `min(10, victim's gold)` instantly, no reticle | `SL:522-570` |
| Sorcerer | never attacks; standing inside an Ancient Ground grants every other friendly attacker in that ground **+5 % base damage per second per Sorcerer**, permanent, cap **80 stacks = +400 %**, lost only on death; a Sorcerer never boosts itself; commandable like any Standard unit | `DB:85` (`SorcererRole`), `DB:112` (`SorcererGroundBoostFmt`, args 5 / 400 from `SU.h:1649`, `:1659`) — `SB/AncientGround.cpp:198-300`, `SB/SorcererUnit.h:81`, `SU.cpp:911`, `SU.cpp:5658` |
| Witch | Support: never attacks, never heals (Damage 0); veils per §2.10 | no dedicated glossary line (the Support line needs Damage > 0, `DB:1591-1596`) |
| Fog / Bright Sun | §2.9 | `DB:131` (`SpellFogCover`), `DB:159` (`SpellFogClear`), `DB:165` (`SpellFogClearWindowRules`) |

---

## 4. Commands

### 4a. Keyboard and mouse (measured from `IMC_Hero.uasset`'s 28 mappings; handlers cited)

`IMC_Hero` is the context the hero and the ghost add (`HC.cpp:373`, `SB/SiegeGhostPawn.cpp:465`; `Content/Blueprints/BP_HeroCharacter.uasset` references `/Game/Input/IMC_Hero`). Every **letter** binding is positional: on Windows the layout subsystem retargets the mapping context so the physical QWERTY position is what counts (Dvorak etc.), digits/modifiers/Enter/Escape/Space/mouse stay literal, always on, no setting (`SB/SiegeKeyboardLayoutSubsystem.h:1-80` doc, `CONV:2427` `KBD-§`).

| Key (IMC) | Action | Handler | What it does |
|---|---|---|---|
| `W` `S` `A` `D` | `IA_Move` | `HC` / `SB/SiegeGhostPawn.cpp:507` | move (Negate/Swizzle modifiers on S/A/W per asset) |
| `Mouse2D` | `IA_Look` | `SB/SiegeGhostPawn.cpp:519`, hero look | camera |
| `SpaceBar` | `IA_Jump` | engine jump, Z-velocity 600 (`HC.h:1126`) | jump; falling out of the world is a death (`CH:352`) |
| `LeftShift` (hold) | `IA_Sprint` | `HC.cpp:390-392`, `448-459` | 750 uu/s while held |
| `LeftMouseButton` | `IA_Attack` | `HC.cpp:402`, `479` | melee cleave 20 / 150 uu / 60° / 0.5 s |
| `Q` | `IA_Rally` | `HC.cpp:412`, `683` | +25 % speed to friendlies within 600 for 5 s, 20 s cooldown |
| `B` | `IA_Recall` | `HC.cpp:435`, `1506` | 10 s channel → teleport home + full heal; moving > 25 uu cancels |
| `One`…`Six` | `IA_Card1`…`IA_Card6` | `PC.cpp:598-609` | play hand slot 1–6 |
| `LeftAlt` (hold) | `IA_UICursor` | `PC.cpp:618-620` | show the cursor for HUD clicks; look suspended |
| `RightMouseButton`, `Escape` | `IA_CancelPlace` | `PC.cpp:627` | cancel placement / targeting / a group pick at no cost (also polled raw, `CH:596-600`) |
| `T` | `IA_CmdAttack` | `PC.cpp:636`, `PC.h:1681` | army-wide **Attack** stance: every commandable unit advances on the enemy castle engaging the nearest enemy within notice; clears every group |
| `E` | `IA_CmdDefend` | `PC.cpp:644`, `PC.h:1690` | army-wide **Defend**: fall back to own castle and fight within castle half-width + 1281; clears every group |
| `R` | `IA_CmdHold` | `PC.cpp:640`, ~~`PC.h:1684`~~ ⚖️ **STRUCK 2026-09-09 (manager, `SC-§53` cl. 2 — struck, not deleted): `:1684` is `void OnCmdHoldPressed();`, the handler, no number. As measured at `8c444ca` the three radii lived at `PC.h:2131` (`GroupSelectRadiusDefault = 1200.f`) · `:2135` (`GroupPositionRadiusDefault = 700.f`) · `:2139` (`GroupAttackRadiusDefault = 1500.f`); §4c's `SiegeAssistantComponent.h:1726-1730` are self-declared MIRRORS (700/1500) and never held 1200 — a landmark, not a source. Correction of record: `handoffs/TASK-1191-programmer.md` § Loop 1 row 2 (found by `qa/TASK-1192-report.md` BLOCKER 2).** | 3-stage **Hold** pick: select circle (opens 1200) → station circle (700) → attack circle (1500); the group drops a target that leaves both circles |
| `F` | `IA_CmdAmbush` | `PC.cpp:652`, `PC.h:1687` | 3-stage **Ambush** pick: same stages; chases a live target with no leash, then returns to station |
| `C` | `IA_CmdFollow` | `PC.cpp:661`, ~~`PC.h:2569-2583`~~ ⚖️ **STRUCK 2026-09-09 (manager, `SC-§53` cl. 2 — struck, not deleted): `:2569-2583` are the `CancelGroupPick` / `OnCmdFollowPressed` declarations — the C handler, no number. As measured at `8c444ca` the 900 lived at `PC.h:451` (`float FollowFormationRadius = 900.f;`, doc `:444-449`). Correction of record: `handoffs/TASK-1191-programmer.md` § Loop 1 row 1 (found by `qa/TASK-1192-report.md` BLOCKER 1).** | 1-stage **Follow** pick: circled units escort the hero (formation within 900) and do not fight |
| `H` | `IA_DiscardAll` | `PC.cpp:727`, `1524` | discard the whole hand for 20 gold, draw 6 |
| `Enter` | `IA_AssistantConsole` | `PC.cpp:673`, `PC.h:2604` | open/close the assistant chat box (§5c) |
| `M` | `IA_WarMap` | `PC.cpp:690`, `PC.h:2674` | open/close the war map — only within 400 uu of your own commander NPC |
| `Tab` | `IA_ControlsHelp` | `PC.cpp:709`, `PC.h:2761` | controls overlay; the battle keeps running |

> ⚖️ **MANAGER ANNOTATION 2026-09-09 (`SC-§53` cl. 2–3; this census is a dated record of `HEAD 8c444ca` and is not re-measured):** rows **C** and **R** above cited handler *declarations* as the source of three C++ numbers (Follow 900; Hold 1200 / 700 / 1500). The numbers were correct; the citations were not (`SC-§110`). Found by `qa/TASK-1192-report.md` BLOCKER 1–2; corrected in the README (`Docs/Packaging/README-source.md:710-717`) by `TASK-1191` loop 1, verified at source by its author. **The correction of record is `handoffs/TASK-1191-programmer.md` § Loop 1 rows 1–2**; the corrected `file:line` is also written inline in each struck cell so a reader of this table alone gets the right citation. The author (`TASK-1190`) is not re-dispatched — this file was outside `TASK-1191`'s write list and the propagation duty is the manager's (`SC-§82`). Nothing else in this file is touched by this annotation; the § Addendum below is `TASK-1191`'s own.

Raw (non-mapped) inputs: **`Z`** accepts the assistant's plan (`CH:1168-1175`, literal key); **mouse wheel** resizes the placement footprint (×1.0–1.5, step 0.1, `PC.h:2287-2322`), the active command circle (**100 uu per notch, clamped 200–5000**, `PC.h:2119-2127`) and a war-map mark (6 px per notch, 12–240 px, `SB/WarMapWidget.h:1292-1317`); **LMB** confirms a circle stage / drops a numbered war-map mark; **RMB** deletes a mark / cancels a pick (`CH:978-1072`, `CH:1433-1434`). Engine-level: `F11` and `Alt+Enter` toggle fullscreen (`Config/DefaultInput.ini:63-64`).

Gamepad: `IMC_Default` maps sticks/`Gamepad_FaceButton_Bottom` to Move/Look/Jump (`Content/Input/IMC_Default.uasset` decoded), but it is added only by the template controller `AGitClaudeUnrealTestPlayerController` (`Source/GitClaudeUnrealTest/GitClaudeUnrealTestPlayerController.cpp:46-48`), and `ASiegePlayerController` derives from `APlayerController`, not from it (`PC.h:204`) — whether any gamepad context is active in a match is **NOT MEASURED**.

### 4b. In-match UI actions (the controls overlay's full row list, `CH:303-1482`)

Hero: Move · Look · Jump · Sprint · Attack · Rally. Cards: Play a card · Show the mouse cursor · Discard your whole hand · Cancel · Stack a tower taller · Resize what you are placing. Orders: Attack (army order) · Defend (army order) · Hold · Ambush · Follow. Pick mode: Confirm the circle · Resize the circle · Exit the command. Interface: AI chat · Accept the assistant's plan · Open the map · Reveal enemy positions · Click a place on the map · Draw circles on the map · Controls. HUD elements: gold, gold rate, miner count, `OVERTIME`, `Rally: Ready`/cooldown, command indicator, hand of 6 with key chips + refusal text + next-card preview (`Content/UI/WBP_HUD.uasset`, `WBP_CardHand.uasset` name tables), castle health bars (`WBP_CastleHealthBar`), overhead bars on every combat actor with a boost row (`SB/CombatantHealthBarComponent.h`).

### 4c. The assistant's grammar (what a typed sentence can say)

| Element | Values | Citation |
|---|---|---|
| Intents (7) | `Send`, `Guard` (both → a Hold group, R's path), `Ambush` (F), `Follow` (C), `Charge` (army Attack, T), `Fallback` (army Defend, E), `Rally` (Q) | `SB/SiegeAssistantCommand.h:66-75`; synonyms `SB/SiegeAssistantVocabulary.cpp:219-225` |
| Places (7 symbols) | `own_castle`, `enemy_castle`, `mid`, `ancient_ground_near`, `ancient_ground_far`, `nearest_mine`, `hero` | `SB/SiegeAssistantVocabulary.cpp:157-187` |
| Units | every unit kind by name + synonyms (archer, cavalry, cleric, footman, knight, longbowman, militiamob, miner, ogre, pikeman, sapper, sorcerer, wizard …); "mage" resolves to nothing by design | `SB/SiegeAssistantVocabulary.cpp:120-142`, `GDD:271` |
| Selection shapes | up to 3 kinds with counts, "everyone except …", "everyone in <region>" (mid / the two ancient grounds only), all, none; region radii: position 700, attack 1500 | `SB/SiegeAssistantComponent.h:1726-1730`, `GDD:264-265` |
| Triggers | "when I have at least N …" latched for **120 s**, re-checked at **1 Hz**, still confirms before firing | `SB/SiegeAssistantComponent.h:1704`, `:1713` |
| Confirm step | on by default; **Z** accepts, closing the box discards; Escape never closes the console | `SB/SiegeSettingsSubsystem.h:293`, `SB/SettingsMenuWidget.cpp:33`, `SB/SiegeAssistantConsoleWidget.h` (doc) |
| Limits | commands units only — never plays a card, never spends gold; emits symbols, never coordinates | `SB/SiegeAssistantComponent.h:1-160` (doc), `SB/CommanderNpc.h:57-59` |

### 4d. Console commands and cheats — **Development builds only; ABSENT from a Shipping package**

| Command | Signature / effect | Citation |
|---|---|---|
| Console itself | `~` opens it (`ConsoleKeys=Tilde`); compiled out in Shipping: `ALLOW_CONSOLE = ALLOW_CONSOLE_IN_SHIPPING (0)` | `Config/DefaultInput.ini:84-85`, `ENG/Runtime/Core/Public/Misc/Build.h:214-215`, `:348`, `ENG/Runtime/Engine/Private/GameViewportClient.cpp:2807` |
| Cheat manager | `USiegeCheatManager` is the controller's `CheatClass`; the engine only creates a cheat manager when `UE_WITH_CHEAT_MANAGER (= !UE_BUILD_SHIPPING)` and `AllowCheats` (standalone or editor) | `PC.cpp:211`, `ENG/Runtime/Engine/Classes/GameFramework/CheatManagerDefines.h:9`, `ENG/Runtime/Engine/Private/PlayerController.cpp:1148-1160`, `ENG/Runtime/Engine/Private/GameModeBase.cpp:1411-1414` |
| `SummonTestUnit <CardID> <bRed>` | spawn a unit for a team | `SB/SiegeCheatManager.h:71-72` |
| `ApplyTestDamage <Amount>` | damage under the cursor/target | `SB/SiegeCheatManager.h:84-85` |
| `AddTestGold <Amount>` | grant gold | `SB/SiegeCheatManager.h:94-95` |
| `SetTestDamageBoost <Percent> <bAllFriendly>` | set permanent damage stacks | `SB/SiegeCheatManager.h:126-127`, `.cpp:480`, `:540` |
| `DumpAssistantPrompt <Utterance>` | write the assistant's prompt bytes (body compiled out in Shipping) | `SB/SiegeCheatManager.h:170-171`, `.cpp:585-681` |
| `Siege.Fog.Raise` / `Siege.Fog.Clear` / `Siege.Fog.Status` | dev fog triggers, `ECVF_Cheat`, inside `#if !UE_BUILD_SHIPPING` | `FV.cpp:56-58`, `:232-250` |
| `siege.Input.LayoutPollEnabled` (cvar, default 1) | dev/test lever for the layout poll (Windows) | `SB/SiegeKeyboardLayoutSubsystem.cpp:75-82` |

---

## 5. "The AI commander" — everything the phrase resolves to

### 5a. The bot opponent (`ASiegeBotController`) — what plays against you

| Fact | Value | Citation |
|---|---|---|
| Team / hero | Red; controls **no hero** | `Bot.h:217`, `GDD:416` |
| Cadence | evaluates every **2 s**; the first rule that fires owns the tick | `Bot.h:221`, `Bot.cpp:369-389` |
| Deck | one of two curated 50-card decks, chosen **at random** per match: **"Bot Aggro Rush"** (Footman 12, MilitiaMob 6, Pikeman 6, Knight 6, Archer 6, Cavalry 4, Sapper 4, Wall 4, Miner 2) or **"Bot Defensive Economy"** (Wall 8, ArrowTower 8, Knight 6, BombTower 4, BallistaTower 4, Miner 4, Barracks 3, CrystalTower 3, Cleric 3, Ogre 2, DeepMine 2, Lightning 2, Longbowman 1); falls back to the `DeckCount` default only if a curated deck is illegal | `Bot.cpp:253-290`, `Bot.cpp:299-337` |
| Rule 1 — defend | an enemy unit on the bot's half ⇒ the cheapest affordable defensive card: a tower placed **750 uu** in front of the castle face toward the intruder, or a unit in a **±900 uu** lane spread | `Bot.cpp:433-490`, `Bot.h:443`, `Bot.h:376` |
| Rule 2 — economy | no intruder ⇒ a Miner while alive miners < **3** and a claimable mine exists (never a doomed miner), else a Deep Mine | `Bot.cpp:496-610`, `Bot.h:227`, `Bot.h:239` |
| Rule 3a — Fireball | at a cluster of ≥ **3** player units within the card's radius — **inert with the shipped decks: neither curated deck contains Fireball** | `Bot.cpp:701-733`, `Bot.h:269`, `Bot.cpp:253-290` |
| Rule 3b — Lightning | at a player tower with ≥ **2** units within the card's radius (Defensive deck only) | `Bot.cpp:735-766`, `Bot.h:273` |
| Rule 4 — attack | gold ≥ **36** ⇒ the most expensive affordable unit at the castle front (offset **1343.15**), which auto-marches | `Bot.cpp:769-800`, `Bot.h:231`, `Bot.h:372` |
| Rule 5 — discard | the most expensive unplayable non-spell card for **1 gold** | `Bot.cpp:804-810`, `Bot.h:235` |
| Spawn region | its castle box, or the mid zone while Red owns it | `Bot.h:703`, `Bot.cpp:1622-1660` |
| Line spells | fired from its castle (no hero) | `SL:195-200` |
| Never | commands units, veils (no Witch in any deck), casts Fog/BrightSun/Fireball/upgrades (not in its decks); not networked (no bot in a networked match) | `Bot.cpp:253-290`, `GM.cpp:1719-1725` |

### 5b. The commander NPC and the war map (`ACommanderNpc`, `UWarMapWidget`)

| Fact | Value | Citation |
|---|---|---|
| Where | one per castle, inside the keep; **own team only**; interact radius **400 uu** | `SB/Castle.cpp:153`, `SB/CommanderNpc.h:295`, `.cpp:219-232` |
| Map (M) | top-down map from the arena extent; own units/hero as live dots refreshed **4×/s**; elevation relief; the 7 place markers (clicking one appends its symbol to the console box — nothing is submitted); numbered marks (LMB drop, wheel resize, RMB delete) | `SB/WarMapWidget.h:1186`, `:1218-1240`, `:1280-1364`, `CH:1239-1434` |
| Reveal | **30 gold** shows every enemy unit currently visible to your team (veiled units excluded) and the enemy hero as a frozen snapshot, capped at **512** dots; discarded when the map closes; refused before any spend under 30 | `SB/CommanderNpc.h:311`, `PC.cpp:6790-6792`, `PC.cpp:7128-7131`, `PC.cpp:7185-7218`, `PC.cpp:147` (`MaxEnemyRevealDots = 512`), `CH:1318-1319` |
| Match does not pause | — | `CH:72` |

### 5c. The in-match assistant (the LLM)

| Fact | Value | Citation |
|---|---|---|
| What it is | a local `llama.cpp` model (`Qwen3-4B-Q4_K_M.gguf`, expected under `<project>/Models/`) turning one typed sentence into one of the §4c orders through the same code the keys call | `Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSettings.h:72`, `Private/SiegeLlamaSettings.cpp:15-48`, `SB/SiegeAssistantComponent.h` (doc) |
| Without the model | "the in-match assistant is unavailable this session … THE MATCH IS FULLY PLAYABLE"; console disabled; every key byte-identical | `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:1994`, `:2012`, `:2085` |
| In the shipped zip | the ship gate `C4-NO-MODELS` asserts **0** `.gguf` files and **0** `Models/` dirs in the stage ⇒ **EXISTS-INERT** in the package; re-enable path per source: drop the file at `<package>\Windows\GitClaudeUnrealTest\Models\Qwen3-4B-Q4_K_M.gguf` or pass `-siegellm.model=<path>` (not observed) | `Tools/Packaging/ship.ps1` gate `C4-NO-MODELS` — at `HEAD 8c444ca` lines 2835-2839; in the working tree (locally modified by the live ship) lines 2930-2934 — `CONV:6952` (`PKG-§4`) |
| Setting | "Confirm AI orders before they execute" — default **on** | `SB/SettingsMenuWidget.cpp:33`, `SB/SiegeSettingsSubsystem.h:293` |

---

## 6. Areas of interest

All counts are **per match**; §6.1–6.2 are re-placed every match and every Play Again from a new random seed (`BS:290-306`: `OverrideSeed > 0` else `FMath::RandRange(1, MAX_int32 − 1)`; `bReRandomizeOnMatchReset = true`, `SB/BattlefieldScatter.h:211`). Symmetry law: everything is generated on the **Blue half (X < 0)** and mirrored as its **180° rotational twin at (−X, −Y), yaw 180°** (`SCfg:57-64`, `SCfg:346` default `Rotational180`; the data asset overrides none of the mine/ground/symmetry fields — its name table lacks them, `DA`).

### 6.1 Neutral gold mines (`AGoldNode`) — **6 (3 per side)**

| Fact | Value | Citation |
|---|---|---|
| Count | `MineCountPerSide = 3` ⇒ 3 drawn on the Blue half + 3 twins = **6** | `SCfg:415`, `BS:1631-1790` (`PlaceMines`), `BS:1784` (twin at (−X, −Y)) |
| Reserve | **300 gold** each | `SCfg:448` → `SB/GoldNode.cpp:97-101`, `SB/GoldNode.h:236` |
| Claim | the first miner to arrive claims the mine for its team; the other team's miners cannot register while it is occupied; it frees when the last miner leaves | `SB/GoldNode.cpp:114-160` (`CanTeamMine`, `OccupyingTeam`), `:163-180` |
| Drain | **1 gold/s per working miner** (exactly the payout) on a 1 s tick | `SB/GoldNode.h:253`, `:352`, `.cpp:317-337` |
| Depletion | at 0 it depletes **for the rest of the match**, evicts its miners (they retarget), glow dims to 0.05 | `SB/GoldNode.cpp:343-354`, `SB/GoldNode.h:265-269`, `SB/MinerUnit.cpp:1361` |
| Placement rule | stream `Seed ^ 0x4D494E45`; X = −U(1500, 26000 − 600), Y = U(−(12000 − 600), +(12000 − 600)) [MinAbsX = max(clearance 600, spacing/2)]; ≥ **3000 uu** from every prior mine; outside every keep-clear disc inflated by 600 (castles r = **4500** at the castle actors (fallback ±25000, 0); player starts r = **800** (fallback −23800, 0)); ground slope ≤ **30°**; up to 2 × MaxPlacementAttemptsPerInstance tries, then a fallback point at half-field | `BS:1659-1790`, `SCfg:356` (arena half-extent 26000 × 12000), `:428`, `:440`, `:452`, `:462`, `:382`, `:386`, `BS:1422-1487` (keep-clear zones), `BS:1504-1512` |

### 6.2 Ancient grounds (`AAncientGround`) — **2 (1 per half)**

| Fact | Value | Citation |
|---|---|---|
| Count | one drawn on the Blue half + its twin = **2** | `BS:1950-2090`, `SCfg:470-476` |
| Footprint | **1680 × 1680 uu** (half-extent 840), a rune decal — no collision, no navmesh footprint | `SB/AncientGround.h:178`, `SCfg:494`, `BS:2096-2104` (paired-tunable check) |
| Placement rule | stream `Seed ^ 0x41474E44`; X = −U(**4000**, min(**16080**, 26000 − 840)), \|Y\| ≤ min(**10800**, 12000 − 840); ≥ **1800 uu** from every mine (both ends); keep-clear radius **1200**; a flat surface at both ends; 48 attempts | `BS:1970-2085`, `SCfg:503`, `:552`, `:565`, `:576`, `:588` |
| Effect | every **1 s**: count live Sorcerers inside per team; every *other* live friendly unit inside that can attack gains that many **permanent damage stacks** (+5 % each, cap 80 = +400 %); kept when leaving, cleared only on death | `SB/AncientGround.h:191`, `.cpp:198-300`, `SU.h:1649`, `:1659`, `SU.cpp:940-953`, `SU.cpp:5658` |
| Ownership | team-neutral, never captured (both sides can be empowered at once) | `SB/AncientGround.h` (doc), `GDD:203` |

### 6.3 Mid capture zone (`ACaptureZone`) — **1, fixed at the map origin**

| Fact | Value | Citation |
|---|---|---|
| Placement | a level-placed actor at world origin (not scatter) | `Content/Maps/L_Arena.umap` name table (`CaptureZone`), `SB/CaptureZone.h` (doc "sits at world origin") |
| Footprint | 1680 × 1680 (half 840) | `SB/CaptureZone.h:190` |
| Capture | every **0.5 s** count units + hero inside (buildings/castles/mines excluded): one team alone ⇒ owner; both ⇒ **neutral** (contested); empty ⇒ last owner stays | `SB/CaptureZone.h:194`, `:204`, `.cpp:136-230` |
| Effect | the owner may **place cards inside it** (the only forward placement region); the bot uses it too | `PC.cpp:6074-6079`, `Bot.cpp:1622-1660` |
| Reset | Play Again resets it | `GM.cpp:1422-1424` |

### 6.4 Castles and their spawn boxes — **2**

See §2.7: level-placed, X ≈ ±25000, each a 7380-half-extent placement square, hollow and enterable, with 6 torches and the commander NPC (`SB/Castle.h:394`, `:563`, `SB/Castle.cpp:152-153`, `L_Arena.umap` name table `Castle`).

### 6.5 Procedural terrain (the scatter, `DA_BattlefieldScatter`) — decorative except where noted

| Layer (`DA`) | Instances | Min spacing | Blocking | On hills | Region | Scale |
|---|---|---|---|---|---|---|
| Trees | 340 | 300 | yes | yes | edge-biased | 0.8–1.2 |
| Rocks | 300 | 350 | yes | yes | edge-biased | 0.5–1.5 |
| Boulders | 30 | 900 | yes | no | edge-biased | 0.8–1.3 |
| **Hill** | **40** | 2000 | yes | no | whole field | 0.4–2.5 |
| Slabs | 40 | 1500 | yes | no | edge-biased | 3.0–5.0 |
| Grass | 36750 | 50 | no | yes | whole field | 0.7–1.5 |
| Plants | 2000 | 200 | no | yes | whole field | 0.7–1.5 |

Rules: all layers respect the same seed and 180° symmetry (`BS:365-405`), a guaranteed clear castle-to-castle corridor of half-width **1000** (`SCfg:399`), and the keep-clear discs of §6.1; hills are climbable and are where the ranged **height bonus** (§2.6) is earned; blocking scatter carves the navmesh, and the scatter actor tags itself `Terrain` so its meshes block projectiles (`BS:191`, `SB/Projectile.cpp:411-447`). Instance counts are the asset's `InstanceCount` per layer. The per-layer draw-distance cull bands are also in the asset (Trees 24000→32000, Rocks 14000→20000, Grass 6000→9000, Plants 8000→12000, in uu) and are **scaled by the Foliage graphics setting** at runtime (`BS:459`, `BS:988-1075` `ApplyFoliageCullBands` → `SetCullDistances`); the scaled values per detent are NOT MEASURED here.

### 6.6 NOT areas of interest — card-summoned structures (so the README does not call them spawning areas)

Watch Tower, Barracks, Deep Mine, Wall and the four attack towers are **played from the hand into a placement region** (§2.4); none is scatter-placed. The two mid-field `SM_GoldNodeProp` meshes in `L_Arena` are visual props only (`L_Arena.umap` name table; `GDD:439`).

---

## 7. "Other features" — what exists in the shipped build (REACHABLE / EXISTS-INERT / ABSENT)

| Feature | Status | From where / how | Citation |
|---|---|---|---|
| Main menu | REACHABLE | boot map; entries **Play (vs Bot)** · **Sandbox (No Bot)** · **Deck Builder** · **Multiplayer** · **Settings** · **Login** · **Quit** | `Config/DefaultEngine.ini:10`, `Content/UI/WBP_MainMenu.uasset` (label strings + `PlayBtnClicked`/`StartSandboxMatch`/`MultiplayerBtnClicked`/`SettingsBtnClicked`/`LoginBtnClicked`/`QuitBtnClicked`, `WBP_DeckBuilder_C`, `WBP_SessionMenu_C`, `AccountMenuWidget`, `SettingsMenuWidget`) |
| Sandbox (No Bot) | REACHABLE (dev affordance kept in the menu) | no opponent, 9999 gold | `GM.cpp:1628-1646`, `GM.h:569` |
| Deck builder | REACHABLE | 10 fixed slots, exactly-50 rule, no per-card cap, active deck feeds the next match, x/50 total + average cost, card detail panel with the glossary text, Play / Exit buttons | `SB/SiegeDeckSaveGame.h:57`, `SB/DeckLibrary.cpp` (`IsDeckLegal`), `DB:1240`, `WBP_DeckBuilder` (`TotalText`, `AvgText`, `Btn_DetailsClose`, strings `Play`/`Exit`) |
| Settings | REACHABLE | one toggle ("Confirm AI orders before they execute", default on) + **Graphics** + Back | `SB/SettingsMenuWidget.cpp:33`, `:56`, `:64`, `SB/SiegeSettingsSubsystem.h:293` |
| Graphics menu | REACHABLE (Settings → Graphics) | rows: Auto-Detect Quality · Overall Quality slider (levels **0–4**, `Custom` when mixed) · Resolution Scale **50–100 %** · Screen Resolution stepper (engine-enumerated list) · Window Mode (Fullscreen / Borderless Window / Windowed) · V-Sync checkbox · Frame Rate Limit stepper (**30, 60, 90, 120, 144, 165, 240, Unlimited**) · ten per-group sliders (View Distance, Anti-Aliasing, Shadow, Global Illumination, Reflection, Post Process, Texture, Effects, Foliage, Shading) · "Show FPS counter during a match" (default off) · Back; a display change asks **Keep / Revert** with a **10 s** countdown; FPS/ms readout refreshes every 0.5 s | `SB/SiegeGraphicsMenuWidget.cpp:41-145`, `:378`, `:799-896`, `:978`, `:860-871`, `SB/SiegeGraphicsSettingsSubsystem.h:150-175`, `:642`, `.cpp:29-38`, `:87-96`, `:735-738`, `:843-846`, `SB/SiegeGraphicsMenuWidget.h:374`, `:410`, `:418`, `SB/SiegeSettingsSubsystem.h:296` |
| Graphics auto-revert | EXISTS — **never observed live** | the 10 s revert has not been watched against a real timer | `TB:391`, `TB:1958`, `CONV:11215` (`GFX-§5`) |
| In-match FPS counter | REACHABLE (checkbox above) | `USiegeFrameRateCounterWidget`, 0.5 s cadence | `SB/SiegeGraphicsMenuWidget.h:1311`, `.cpp:3248` |
| Accounts (local) | REACHABLE (main menu **Login**) | Create Account / Log into existing account / Log In / Log Out / Back; display name 3–24, password ≥ 4; profiles keep separate decks and settings; guest is the default and login gates nothing | `SB/AccountMenuWidget.cpp:30-63`, `SB/SiegeAccountSubsystem.h:114-130`, `CONV:6319` (`ACC-§`) |
| Cloud sync (Supabase) | REACHABLE **if the cook stages `Config/SiegeCloudDev.ini`** — UAT stages every project `Config/*.ini` unless denied in `[Staging]`, and `DefaultGame.ini` has no `[Staging]` section; the ini exists (gitignored) on the cooking machine; **the package itself was not inspected** | Link to Cloud (email + password), Sync Now; cloud OFF when the ini is missing | `SB/SiegeCloudClient.cpp:108-115`, `:201`, `:303-304`, `Config/SiegeCloudDev.ini:1-14`, `.gitignore:63`, `ENG/Programs/AutomationTool/Scripts/CopyBuildToStagingDirectory.Automation.cs:1801` + `StageConfigFiles`, `Config/DefaultGame.ini` (whole file), `SB/AccountMenuWidget.cpp:45-104` |
| Multiplayer | EXISTS (host plays; joiner observes) | main menu **Multiplayer** → Host / Join (address, default port 7777) / Back; listen server, LAN / direct IP; no bot in a networked match; the joining client's card play is refused on non-authority (`PlayHandSlot`), and building stacking/authoring is server-only — the client is an observer of match state (M8 Phase 1) | `Content/UI/WBP_SessionMenu.uasset` (strings `Multiplayer`, `Host`, `Join`, `Back`, `Ready - host a match or join an address.`, `AddressTextBox`), `SB/SiegeSessionSubsystem.cpp:26`, `:48`, `:69`, `:89`, `:127`, `.h:62`, `GM.cpp:1719-1725`, `PC.cpp:974`, `SB/Building.cpp:415`, `GDD:524-527` |
| War map | REACHABLE (M within 400 uu of own commander) | §5b | — |
| Controls overlay | REACHABLE (Tab) | §4b | `CH:71-78` |
| Assistant console | EXISTS-INERT in the zip (no model; §5c); REACHABLE in a checkout with the model | Enter | — |
| Match end / Play Again | REACHABLE | Victory! / Defeat + Play Again | `Content/UI/WBP_VictoryScreen.uasset`, `GM.cpp:1305` |
| Positional keyboard remap | REACHABLE (always on, Windows, no UI) | — | `SB/SiegeKeyboardLayoutSubsystem.h:1-80`, `.cpp:75` |
| Key rebinding UI | ABSENT | — | `GDD:501` (flagged gap); no rebinding widget in `SB/` |
| Footage / replay | ABSENT | no `Replay`/`DemoNetDriver` symbol in `Source/` outside one test file (`SB/Tests/SiegeCastBarTest.cpp`) | grep (this census) |
| Console / cheats | ABSENT in Shipping; present in Development | §4d | — |
| Per-card discard | EXISTS-INERT | §2.2 | `PC.h:1760-1767` |

---

## 8. `PKG-§11` — inert or unobserved mechanics (with reasons)

| Mechanic | Status | Reason | Citation |
|---|---|---|---|
| Witch veil vs the bot | inert vs bot | `DeckCount = 0` and the Witch is in neither bot deck ⇒ the bot never veils; a player only meets the card by decking it | `CSV:33`, `Bot.cpp:253-290`, `TB:1958` |
| Fog / Bright Sun vs the bot | inert vs bot | `DeckCount = 0`, absent from both bot decks | `CSV:34-35`, `Bot.cpp:253-290` |
| Bot Fireball rule (3a) | inert | neither curated deck holds Fireball | `Bot.cpp:253-290`, `:701-733` |
| Watch Tower, Wizard, Sapper (player side), every `DeckCount 0` card | reachable only via the deck builder | 19 of 34 rows ship at `DeckCount 0` | `CSV:2-35` |
| Fog visibility cycle | unobserved cause | the fog's far-field visibility swings on a ~181 s cycle (`min/max = 0.3399`); mechanism open at `TASK-1184` | `TB:1515`, `TB:1596-1600`, `TB:1801` |
| Graphics Keep/Revert auto-revert | never observed against a real `FTimerManager` | — | `TB:391`, `TB:1958` |
| Veil shimmer visible to the enemy | open ruling `J-W18` | — | `CONV:10020` |
| Assistant | inert in the zip | no model weights (`C4-NO-MODELS`) | `Tools/Packaging/ship.ps1` (HEAD 2835-2839 / working tree 2930-2934) |
| Cheats, `Siege.Fog.*`, console | absent in Shipping | `ALLOW_CONSOLE`, `UE_WITH_CHEAT_MANAGER` | §4d |
| Per-card discard (`DiscardHandSlot`) | dead path | retired at `CARDBAR-§6`; no shipped caller | `PC.h:1760-1767`, `CONV:9184` |
| `IsGhostPossessed()` | dead code | no caller | `PC.cpp:1773` |
| Multiplayer joiner | observer only | client-side play refused | `PC.cpp:974`, `GDD:525` |
| Cloud sync in the package | conditional on staging (§7) | — | — |

---

## 9. GDD divergences — the code wins

| Topic | GDD says | Code says | Citations |
|---|---|---|---|
| Hero respawn | 5 s at own castle (`GDD:47`) | ghost pawn, **180 s** | `GM.h:461` |
| Discard | 1 gold per card (`GDD:133`, `GDD:506`) | whole-hand **20 gold flat** (H); per-card retired | `PC.h:1786`, `PC.h:1760-1767` |
| Deck copy caps | Max Copies enforced per card (`GDD:73`, `GDD:492`) | abolished; only the exactly-50 total binds | `SB/CardRow.h:204-212` |
| Notice / aggro radius | 600 (`GDD:147`, `GDD:222`) | **5000** (609.6 under fog) | `SU.h:914` |
| Leash | 900 (`GDD:159`) | **8000** (609.6 under fog) | `SU.h:1448`, `SU.cpp:1752` |
| Defend engagement | "within 2500 of it" (`GDD:223`) | castle half-width + **1281** | `SU.h:1513`, `SU.cpp:2710` |
| Castle spawn box | half-extent 2460 (`GDD:125`, `GDD:446`) | **7380** | `SB/Castle.h:394` |
| High ground | "physical only, no stat bonuses" (`GDD:437`) | **+10 % ranged damage per 152.4 uu** of height | `SU.h:1687-1700` |
| Ranged ranges | Archer 700, Longbowman 1200, Wizard 700 (`GDD:359`, `:373`, `:405`) | **2100 / 3600 / 2100** | `CSV:3`, `:12`, `:30` |
| Cleric heal timer | "1.0 s cadence is the heal's timer" (`GDD:374`) | 0.1 s tick × 0.8 HP (8 HP/s); `Cadence` unused for healing | `SU.h:1549`, `SU.cpp:3043` |
| Card pool | 30 cards, 13 unit-spawning (`GDD:350`) | **34 rows** (Witch, Watch Tower, Fog, Bright Sun added); 14 unit-spawning (13 `Unit` + Miner) | `CSV:2-35` |
| Cards absent from GDD | — | Witch, Watch Tower, Fog, Bright Sun, the height-advantage rule, Recall (B), Discard-all (H), the ghost, the graphics menu, the FPS counter | §2–§7 |
| Lightning notes cell | GDD itself flags the stale 400 (`GDD:399`) | `AoERadius 700` ships; the `Notes` cell is not read | `CSV:26`, `SB/CardRow.h:229-231` |
| Gold nodes "800 units from each castle" (`GDD:429`) | superseded in the GDD (`GDD:443`) | 6 neutral mines, scatter-placed | §6.1 |
| Assistant weights | "does NOT yet ship WITH the game" (`GDD:251`) | still true — the ship gate forbids them | `ship.ps1` gate `C4-NO-MODELS` (HEAD 2835-2839) |
| Main menu | seven entries (`GDD:493`) | matches (Play, Sandbox, Deck Builder, Multiplayer, Settings, Login, Quit) | `WBP_MainMenu` |

Agreements worth stating for the README author (code = GDD): start 10 / +1 s / overtime 7:00 ×2 / cap 999 / 6 miners / mine reserve 300 / castle 2000 / crumble 75-50-25 / hero 200-500-750-20-150-60°-0.5 s / rally 600-25 %-5 s-20 s / upgrade magnitudes and caps / command circles 1200-700-1500, wheel 100, 200–5000 / follow 900 / 30-gold reveal at 400 uu / capture zone 0.5 s rule / ancient-ground magnitudes / bot rules and thresholds (2 s, 3 miners, 36).

---

## 10. Manager's cited facts — verdicts

| Claim (from the board preamble) | Verdict | Measured value / refinement |
|---|---|---|
| Notice = `UnitEngagementRadiusUU = 5000` (`CardRow.h:289`), every `NoticeRange` cell blank, capped by fog `609.6` | **MEASURED** | the class constant is `SU.h:914` (`CardRow.h:289` is the comment that names it); blank on all 34 CSV rows and 0.0 on all 34 asset rows; the cap is a `min` inside the vision funnel (`SB/SiegeCombatStatics.cpp:257`), applied to notice, attack reach **and the leash test** |
| Retention = `max(LeashRange, notice × LeashMarginMultiplier)` | **MEASURED, with the numbers** | `max(8000, 5000 × 1.5 = 7500) = 8000 uu` for every card (`SU.cpp:4573-4583`); guards: a non-finite notice returns the floor, a multiplier ≤ 1 is treated as 1; the leash comparison is fog-clamped to 609.6 while fog is up (`SU.cpp:1752`, `:2024`) — a refinement the preamble omits |
| Castle `MaxHP = 2000.0f` (`Castle.h:359`) | **MEASURED** | `SB/Castle.h:359` |
| `MineCountPerSide = 3` (`ScatterConfig.h:415`) | **MEASURED** | `SCfg:415`; not overridden by the data asset |
| Ancient grounds "ONE ground on the Blue half" mirrored (`ScatterConfig.h:476`) | **MEASURED** | `SCfg:470-476`, `BS:1950-2090` |
| `EnemyRevealCost` read at `SiegePlayerController.cpp:30` | **MEASURED, line refined** | `:30` is the include; the reads are `PC.cpp:6790` and `:7128`; the value 30 is `SB/CommanderNpc.h:311` |
| Graphics "18 rows / 10 groups / 10 s" | 10 groups and 10 s **MEASURED**; the row list is enumerated in §7 rather than counted (`SC-§104`) | `SB/SiegeGraphicsSettingsSubsystem.cpp:87-96`, `SB/SiegeGraphicsMenuWidget.h:410` |
| Refuted | **none** | — |

---

## 11. NOT MEASURED (may not appear in the README as numbers)

| Item | Why |
|---|---|
| The packaged zip's contents (model absence, config staging, exe names) | the cook is running concurrently (`PKG-§6`); only the ship recipe and UAT source were read |
| The fog's ~181 s visibility cycle and its cause | pixel/runtime measurement owned by `TASK-1177`/`TASK-1184` |
| Whether the Keep/Revert countdown actually reverts | never observed live (`TASK-1125`) |
| Whether a ghost can complete a card placement | no code gate found; behaviour needs a runtime check |
| Actual castle / player-start transforms in `L_Arena` | actor transforms were not decoded; ±25000 / −23800 are the code fallbacks that agree with the level per `BS:1446-1487` comments |
| Scatter layer mesh lists | `SoftObjectProperty` arrays not decoded (names only) |
| Quality-level display names other than `Custom` | not found as literals; only the 0–4 range is measured |
| Screen-resolution stepper values | enumerated from the engine at runtime |
| Whether `IMC_Default`'s gamepad bindings are active in a match | the template context's activation under `ASiegePlayerController` was not traced |
| The stuck-unit watchdog thresholds | `SB/SiegeStuckStatics.h` not re-read this pass |
| `CardArt` object paths from the asset | asset cell not decoded; the CSV `CardArt` cell is the citation |
| Foliage-scaled cull distances per graphics detent | only the authored bands (asset) and the scaling site (`BS:988-1075`) were read; the per-detent scale factors were not |

---

## § Addendum (TASK-1191) — 2026-09-09, gameplay-programmer

Numbers and facts the README needed that the table above did not carry. Each was measured at source by text read at the same git instant (`HEAD = 8c444ca`; last `Source/**`/`Content/**`/`Config/**` commit `ea7b4d2`, earlier than HEAD, so every line below is HEAD content). No editor, no MCP, no compile, no Git write.

| # | Fact | Value | Citation |
|---|---|---|---|
| A1 | Default graphics API / shader model | `DefaultGraphicsRHI=DefaultGraphicsRHI_DX12`; D3D12 targeted shader format `PCD3D_SM6` (SM5 removed) | `Config/DefaultEngine.ini:207-210` |
| A2 | Displayed identity | `ProjectName=Siegebound`; `ProjectDisplayedTitle=…"Siegebound"`; `CompanyName=Jonathan Wesely`; one-line `Description` (the pitch quoted in the README) | `Config/DefaultGame.ini:19`, `:26`, `:36`, `:29` |
| A3 | The click target under Shipping | the root shim `Windows\GitClaudeUnrealTest.exe` is the player's click target; the game binary is `Windows\GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe` | `Tools/Packaging/ship.ps1:1090-1091`, `:1174-1188` (`Get-StageShimExeRel`), `:836` — all at `HEAD 8c444ca` (`git show HEAD:…`; the working-tree copy is dirty from the live ship); `CONV:7264` (the measured Development/Shipping name asymmetry) |
| A4 | Shipping process working set | `WS 1,640 MB` at the menu, 2026-08-30 (`PID 9104 GitClaudeUnrealTest-Win64-Shipping … Title='Siegebound'`) | `Tools/Packaging/ship.ps1:841` (HEAD 8c444ca) — a comment recording a measurement, cited as such |
| A5 | RAM guidance from the previous package | "~4 GB free — the game measured ~1.6–2.2 GB resident" (Development, 2026-08-29) | `packagedZIPofGame/README.md` (2026-08-29) *Minimum requirements* row RAM — quoted in the README as *last measured 2026-08-29*, per the board's rule |
| A6 | Model weights size | "~2.5 GB" | `Tools/fetch_llm_model.py:8` (a source comment, not a measurement; the 2026-08-29 README said ~2.3 GB — the discrepancy is noted in the TASK-1191 handoff) |
| A7 | Shipping is log-silent | ruled and measured at the engine (`bUseLoggingInShipping` is a UBT property, unusable on an installed engine) | `CONV:7043` (`PKG-§9a-1`) |
| A8 | Watch Tower stack ceiling | `MaxStackHeightMultiplier = 2` (every other building 5) | `SB/ClimbableTower.cpp:223`; `SB/Building.h:451`; the wheel exclusion `SB/ClimbableTower.h:464` |
| A9 | Account limits | display name 3–24 characters after trimming; password ≥ 4 | `SB/SiegeAccountSubsystem.cpp:46`, `:50-54` |
| A10 | Deck builder right-click / auto-save | right-click on a fixed slot marks it the ACTIVE deck for the next match; edits persist through the one auto-save funnel to the editing slot | `SB/DeckBuilderWidget.h:93-96`, `:370-373` |
| A11 | Support glossary gate (why the Witch has no keyword line) | `Profile == Support && Range > 0 && Damage > 0` | `DB:1593` |
| A12 | Line-spell geometry (the census cited `SpellLineSweep.h:284-314`, which does not exist — the file is 190 lines) | `LineRange = 900`, `LineHalfWidth = 100`, `TravelDuration = 0.3`, `CastleMuzzleHeight = 150` | `SB/SpellLineSweep.h:103`, `:112`, `:121`, `:133` |
| A13 | Veil-break enum values (the census cited doc lines `:29/:41/:61`) | `ESiegeVeilBreakReason { Attack, Heal, Mine }` | `SB/SiegeInvisibilityStatics.h:97`, `:125`, `:137`, `:157` |
| A14 | Fog cycle figures — board lines re-located at this instant | `~181 s` cycle; `min/max 0.3399` | `TB:1600` (181 s), `TB:1637` (0.3399) — the census's `TB:1515`/`1596-1600`/`1801` have drifted; `:1600` still holds the cycle |
| A15 | Fullscreen toggles | `bAltEnterTogglesFullscreen=True`, `bF11TogglesFullscreen=True` | `Config/DefaultInput.ini:63-64` (confirms the census) |
| A16 | Hero-upgrade magnitudes, per-line | `MeleeDamageBonus = 10` `:1285`; `MaxHPBonus = 100` `:1289`; `MoveSpeedBonus = 0.25` `:1293`; `WarBannerAuraRadius = 600` `:1297`; `WarBannerDamageBonus = 0.20` `:1301`; `WarBannerPulseInterval = 0.5` `:1305` | `HC.h` — the census's `1285-1305` range, split so each README number cites one line |
| A17 | Watch Tower height bonus (derived) | `1 + 0.10 × 1200 / 152.4 = 1.787` | `SU.h:1687`, `:1700`, `SB/ClimbableTower.h:670` |
| A18 | Commits since the previous package | 56 commits, `22728c8` (2026-08-29 17:28 −0700, "final ship touches") → `8c444ca`; `origin/main...main = 0 0` at 2026-09-09 (this reading) | `git rev-list --count 22728c8..HEAD`, anchored one level up (`SC-§102`) |

Citations in the census that the README author did **not** rely on because they did not resolve as written: `SB/SpellLineSweep.h:284-314` (A12 replaces it); `SB/SiegeInvisibilityStatics.h:29/:41/:61` (A13 replaces it); `TB:1515`, `TB:1801` for the fog cycle (A14 replaces them). Everything else spot-checked at the cited line held its number (see the TASK-1191 handoff for the list).
