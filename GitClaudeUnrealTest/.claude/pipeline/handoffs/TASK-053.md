# TASK-053 handoff — Card data: FCardRow keyword columns + cards.csv Set II + M4 test deck

**Status:** ready-for-qa
**Scope:** files only. No editor, no compile, no Git. Board not touched (Jonathan owns it).

## Files changed
- `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` — added the nine M4 UPROPERTYs to `FCardRow` (after `bRanged`).
- `Docs/Data/cards.csv` — appended nine columns to header + all 6 core rows (default-valued), added 16 Set II rows, redistributed `DeckCount` across all 22 rows.

## 1) FCardRow — nine new UPROPERTYs (CSV headers 1:1 for reimport)
Added exactly as the CONVENTIONS registry / board spec specify, no enum-value changes:

| Column | Type | Default | Category | Semantics (for TASK-054..060) |
|---|---|---|---|---|
| `bCharge` | bool | false | Keywords | Charge: first attack after >=2s uninterrupted movement deals 2x (GDD 3.0). Cavalry. Magnitudes (2s/2x) are UPROPERTY rules in TASK-055, not columns. |
| `bSlayer` | bool | false | Keywords | Slayer: 2x damage vs target MaxHP >= 150 (GDD 3.0). Pikeman. Threshold/mult are TASK-055 UPROPERTYs. |
| `bSuicide` | bool | false | Keywords | Explode on contact/death for `Damage` as AoE over `AoERadius`, then die (GDD 4). Sapper (Siege). Detonation logic in TASK-055. |
| `SwarmCount` | int32 | 0 | Keywords | >0 => playing the card spawns this many copies in a 300-unit circle for one cost (GDD 3.0). MilitiaMob = 4. Consumed spawn-side (TASK-059/060). |
| `AoERadius` | float | 0.0f | Stats | Splash radius for area attackers; 0 = single target. Sapper 250, BombTower 250. Used by TASK-055 radial helper + TASK-056 projectile. |
| `MinRange` | float | 0.0f | Stats | Inner blind-spot radius; cannot fire at targets closer than this. BallistaTower 300 (TASK-056). |
| `SpawnCardID` | FName | NAME_None | Spawner | Spawner building: CardID to spawn (TASK-057 Barracks = Footman). |
| `SpawnInterval` | float | 0.0f | Spawner | Seconds between spawns. Barracks = 8. |
| `Lifetime` | float | 0.0f | Spawner | Self-destruct after N seconds. Barracks = 60. |

Enums unchanged — `ECardType {Unit,Building,Economy,Spell,HeroUpgrade,Utility}` and `ECardProfile {None,Standard,Siege,Support}` already carried all needed values; I added no enum entries.

No shadowing introduced (these are struct members with unique names; the C4457/58/59 law doesn't apply to a plain FTableRowBase struct, and none of the new names collide with anything).

## 2) cards.csv — header + 16 Set II rows
Header (23 columns; first is the empty row-name column, then the 22 FCardRow properties in struct order):
```
,DisplayName,CardType,Cost,MaxCopies,HP,Damage,Range,Cadence,Speed,Profile,Notes,DeckCount,bRanged,bCharge,bSlayer,bSuicide,SwarmCount,AoERadius,MinRange,SpawnCardID,SpawnInterval,Lifetime
```

Sixteen new rows, all values character-exact per board spec / GDD §4; keyword columns set ONLY where §4 specifies:

| CardID | Display | Type | Cost | Max | HP | Dmg | Range | Cad | Speed | Profile | Keyword cols set |
|---|---|---|---|---|---|---|---|---|---|---|---|
| MilitiaMob | Militia Mob | Unit | 5 | 6 | 25 | 6 | 120 | 1.0 | 400 | Standard | SwarmCount=4 |
| Pikeman | Pikeman | Unit | 5 | 6 | 100 | 30 | 120 | 1.5 | 350 | Standard | bSlayer=true |
| Sapper | Sapper | Unit | 5 | 4 | 60 | 80 | 120 | 1.0 | 500 | Siege | bSuicide=true, AoERadius=250 |
| Cavalry | Cavalry | Unit | 7 | 4 | 140 | 20 | 120 | 1.0 | 600 | Standard | bCharge=true |
| Longbowman | Longbowman | Unit | 6 | 4 | 70 | 18 | 1200 | 1.5 | 300 | Standard | bRanged=true |
| Cleric | Cleric | Unit | 6 | 3 | 90 | 8 | 400 | 1.0 | 350 | Support | (none; Dmg 8 = heal/s) |
| Ogre | Ogre | Unit | 12 | 2 | 500 | 35 | 120 | 1.5 | 250 | Siege | (none) |
| BombTower | Bomb Tower | Building | 8 | 4 | 180 | 25 | 800 | 2.5 | 0 | None | bRanged=true, AoERadius=250 |
| BallistaTower | Ballista Tower | Building | 7 | 4 | 120 | 45 | 1400 | 3.0 | 0 | None | bRanged=true, MinRange=300 |
| Barracks | Barracks | Building | 10 | 3 | 250 | 0 | 0 | 0 | 0 | None | SpawnCardID=Footman, SpawnInterval=8, Lifetime=60 |
| DeepMine | Deep Mine | Economy | 15 | 2 | 200 | 0 | 0 | 0 | 0 | None | (none; +2 gold/s is a UPROPERTY on ADeepMine) |
| Masons | Masons | Utility | 8 | 3 | 0 | 0 | 0 | 0 | 0 | None | (none; Instant, 300HP/10s is a UPROPERTY) |
| SharpenedBlade | Sharpened Blade | HeroUpgrade | 6 | 2 | 0 | 0 | 0 | 0 | 0 | None | (none; Instant hero upgrade) |
| PlateArmor | Plate Armor | HeroUpgrade | 6 | 2 | 0 | 0 | 0 | 0 | 0 | None | (none) |
| SwiftBoots | Swift Boots | HeroUpgrade | 5 | 1 | 0 | 0 | 0 | 0 | 0 | None | (none) |
| WarBanner | War Banner | HeroUpgrade | 8 | 1 | 0 | 0 | 0 | 0 | 0 | None | (none) |

Notes strings are descriptive only (no gameplay effect, no commas → no CSV quoting) and invent no stats. `SpawnCardID` empty default written as `None` (round-trips to `NAME_None` on import).

## 3) DeckCount redistribution — M4 test deck (sums to EXACTLY 50)
Per the M4 design ruling, redistributed across all 22 rows (each >=1, each <= MaxCopies):

- Core (6): Footman 6, Archer 4, Knight 2, Miner 3, ArrowTower 2, Wall 3 = **20**
- Set II units (7): MilitiaMob 2, Pikeman 2, Sapper 2, Cavalry 2, Longbowman 2, Cleric 2, Ogre 2 = **14**
- Buildings (4): BombTower 2, BallistaTower 2, Barracks 2, DeepMine 2 = **8**
- Instant/upgrade (5): Masons 2, SharpenedBlade 2, PlateArmor 2, SwiftBoots 1, WarBanner 1 = **8**
- **TOTAL = 20 + 14 + 8 + 8 = 50** ✓

Note the tight caps: Ogre, DeepMine, SharpenedBlade, PlateArmor are 2≤2; SwiftBoots, WarBanner are 1≤1. All valid.

## Core rows: behavior unchanged
The 6 core rows are byte-unchanged except (a) `DeckCount` (redistributed per the M4 test-deck ruling — this intentionally supersedes M3's core-only counts) and (b) the nine new columns appended with defaults (`false`/`0`/`None`). All original stat columns (Cost/HP/Damage/Range/Cadence/Speed/Profile/bRanged) are identical to the pre-M4 file. Archer and ArrowTower keep `bRanged=true`.

## Verification run (scripted, in handoff prep)
- 23 header fields; 22 data rows; every row exactly 23 fields.
- DeckCount sum = 50; all rows satisfy 1 <= DeckCount <= MaxCopies.
- CSV data columns == FCardRow UPROPERTY names, same set AND same order (1:1) → **TASK-061 reimport should warn zero**.

## For QA to scrutinize
- Every §4 stat value character-exact vs the board spec table above (Cost/HP/Dmg/Range/Cadence/Speed/profile).
- Keyword columns set ONLY on the specified cards (nowhere else).
- DeckCount arithmetic = 50 and per-card <= MaxCopies (Ogre/DeepMine/upgrades are at the cap).
- Header order = struct order (aids the zero-warning reimport at TASK-061).

## Downstream consumers
- TASK-054: Profile Siege/Support (Ogre, Sapper, Cleric).
- TASK-055: bCharge/bSlayer/bSuicide/AoERadius/SwarmCount magnitudes as UPROPERTY rules; radial helper.
- TASK-056: AoERadius (BombTower), MinRange (BallistaTower).
- TASK-057: SpawnCardID/SpawnInterval/Lifetime (Barracks), DeepMine economy.
- TASK-058: HeroUpgrade rows (SharpenedBlade/PlateArmor/SwiftBoots/WarBanner) + Masons Utility.
- TASK-059/060: SwarmCount consumed spawn-side; bot v2 treats HeroUpgrade/Utility as discards.
