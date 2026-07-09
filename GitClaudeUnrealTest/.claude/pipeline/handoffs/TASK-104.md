# TASK-104 handoff — DT_Cards in-place update: 28-row Set III + M5 test deck (gameplay-programmer, 2026-07-08)

## What was done
In-place update of `/Game/Data/DT_Cards` via DataTableTools (MCP) from the source of truth `Docs/Data/cards.csv`:

1. Pre-checks: `IsPIERunning` = **false** (qa/TASK-021 WARN-2 law honored — this ran before any PIE). Schema readback confirmed the six M5 columns are live post-TASK-103 (spellEffect / effectDuration / maxTargets / goldSteal / chainTargets / chainFalloff, plus ECardType includes `Spell` and ESpellEffect exposes all six values).
2. `add_rows`: Fireball, FrostNova, Lightning, BattleCry, Pickpocket, CrystalTower (appended after WarBanner — table order now matches cards.csv row order exactly).
3. `set_rows` ×2 (14 + 14 rows): **all 28 rows, every column**, per learnings law — `import_file` was NOT used (refuses DataTable overwrite), no delete+recreate, CardArt written as **plain string paths** (never the `{"refPath": ...}` object form).
4. Readback-verified every cell (below), saved, then post-save spot readback re-confirmed.

## Readback verification (get_rows, all 28 rows, every column)
Every cell compared against cards.csv: **28/28 rows match, 0 mismatches.**

- All stat columns (Cost/MaxCopies/HP/Damage/Range/Cadence/Speed/Profile/DeckCount/keyword bools/SwarmCount/AoERadius/MinRange/SpawnCardID/SpawnInterval/Lifetime) byte-equivalent to cards.csv. Note: cadence `1.0` in CSV reads back as `1` — identical float value, JSON formatting only, not a mismatch.
- All 28 CardArt cells stored as non-null soft paths `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` — the NULL-storage trap did NOT trigger (plain-string form used).
- Barracks spawner triplet intact: SpawnCardID=Footman, SpawnInterval=8, Lifetime=60.

### Set III six-column verification (exact per cards.csv)
| Row | CardType | Cost | MaxCopies | Damage | AoERadius | SpellEffect | EffectDuration | MaxTargets | GoldSteal | ChainTargets | ChainFalloff | DeckCount |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Fireball | Spell | 7 | 3 | 100 | 300 | AoEDamage | 0 | 0 | 0 | 0 | 0 | 2 |
| FrostNova | Spell | 6 | 3 | 0 | 350 | Freeze | 4 | 0 | 0 | 0 | 0 | 1 |
| Lightning | Spell | 8 | 2 | 200 | 400 | TopTargetsDamage | 0 | 3 | 0 | 0 | 0 | 2 |
| BattleCry | Spell | 5 | 3 | 0 | 400 | AllyBuff | 8 | 0 | 0 | 0 | 0 | 1 |
| Pickpocket | Spell | 6 | 2 | 0 | 0 | GoldSteal | 0 | 0 | 10 | 0 | 0 | 1 |
| CrystalTower | Building | 9 | 3 | 15 | 0 | None | 0 | 0 | 0 | 3 | 5 | 1 |

CrystalTower also verified: HP 150, Range 800, Cadence 1.5, bRanged=false (instant-hit chain, M5 ruling 9). All 22 M4 rows carry SpellEffect=None and zeros in the five numeric M5 columns.

## Deck sum checks (readback values)
DeckCount per row (DeckCount/MaxCopies): Footman 5/12, Archer 3/10, Knight 2/6, Miner 2/4, ArrowTower 2/8, Wall 2/10, MilitiaMob 2/6, Pikeman 2/6, Sapper 2/4, Cavalry 2/4, Longbowman 2/4, Cleric 2/3, Ogre 2/2, BombTower 2/4, BallistaTower 2/4, Barracks 2/3, DeepMine 1/2, Masons 1/3, SharpenedBlade 1/2, PlateArmor 1/2, SwiftBoots 1/1, WarBanner 1/1, Fireball 2/3, FrostNova 1/3, Lightning 2/2, BattleCry 1/3, Pickpocket 1/2, CrystalTower 1/3.

- **Sum = 50 exactly** (M4 rows 42 + Set III 8, matching the ruling-13 carve Fireball 2 / FrostNova 1 / Lightning 2 / BattleCry 1 / Pickpocket 1 / CrystalTower 1).
- **Every DeckCount ≤ MaxCopies** (28/28; tightest: Ogre 2/2, SwiftBoots 1/1, WarBanner 1/1, Lightning 2/2).

## CardArt resolution checks
`AssetTools.exists` = true for `/Game/UI/CardArt/T_CardArt_Footman` (M4 row, TASK-077 batch), `/Game/UI/CardArt/T_CardArt_Fireball` and `/Game/UI/CardArt/T_CardArt_CrystalTower` (Set III, TASK-106 batch) — the stored soft paths resolve to real assets.

## Save state
`save_assets(["/Game/Data/DT_Cards"])` = true; `is_dirty` = **false** (clean). Post-save spot readback (Footman / Fireball / CrystalTower) confirmed all cells — including CardArt strings — survived serialization.

## Row count / order
`list_rows` = 28 rows, exactly cards.csv order: Footman … WarBanner, Fireball, FrostNova, Lightning, BattleCry, Pickpocket, CrystalTower.

## Scope discipline
Editor mutations limited to `/Game/Data/DT_Cards` only. No git, no compile, no code changes, no TASKBOARD.md edits (orchestrator ruling for this dispatch). The two passive editor prompts were not interacted with; no MCP call was blocked by a modal.

## For QA / downstream
- TASK-107 (BP_Building_CrystalTower) can now pull live stats from the CrystalTower row; the TASK-097 WARN dead-card window closes when 107 lands — still no PIE until then per the orchestrator ruling.
- TASK-106's deferred DT_Cards row-resolution check is satisfied by the readbacks above (record for TASK-109).
- Data table verified only against cards.csv @ working tree (28 rows); if the CSV changes again, rerun this task's set_rows pass.
