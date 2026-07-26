# TASK-299 handoff — Wizard row in cards.csv (gameplay-programmer)

**Status:** ready-for-qa · file-only (no compile, no editor, no Git, no reimport)
**Files touched:** `Docs/Data/cards.csv` — ONLY. One row appended, no other row changed, no columns added/reordered.

## Header (31 columns, unchanged)
```
,DisplayName,CardType,Cost,MaxCopies,HP,Damage,Range,Cadence,Speed,Profile,Notes,DeckCount,bRanged,bCharge,bSlayer,bSuicide,SwarmCount,AoERadius,MinRange,SpawnCardID,SpawnInterval,Lifetime,CardArt,SpellEffect,EffectDuration,MaxTargets,GoldSteal,ChainTargets,ChainFalloff,SpellDelivery
```

## Row added (appended after CrystalTower)
```
Wizard,Wizard,Unit,24,4,45,15,700,1.6,350,Standard,Ranged AoE fireball unit; splash over AoERadius (defaults - tune at playtest),0,true,false,false,false,0,250,0,None,0,0,/Game/UI/CardArt/T_CardArt_Wizard.T_CardArt_Wizard,None,0,0,0,0,0,
```

## Column-by-column (matches the Archer ranged-unit pattern)
| Col | Header | Wizard | Note |
|-----|--------|--------|------|
| 1 | (row name) | Wizard | PascalCase FName, CONVENTIONS Wizard block |
| 2 | DisplayName | Wizard | |
| 3 | CardType | Unit | like Archer |
| 4 | Cost | 24 | FLAGGED tunable |
| 5 | MaxCopies | 4 | FLAGGED |
| 6 | HP | 45 | FLAGGED |
| 7 | Damage | 15 | FLAGGED |
| 8 | Range | 700 | FLAGGED (Archer parity) |
| 9 | Cadence | 1.6 | FLAGGED |
| 10 | Speed | 350 | FLAGGED (Archer parity) |
| 11 | Profile | Standard | obeys Shield-Wall stances like Archer |
| 12 | Notes | Ranged AoE fireball unit; splash over AoERadius (defaults - tune at playtest) | designer-only, never surfaced; NO commas (semicolon+hyphen only) |
| 13 | DeckCount | 0 | keeps sum==50; no other row touched |
| 14 | bRanged | true | ranged caster |
| 15 | bCharge | false | |
| 16 | bSlayer | false | |
| 17 | bSuicide | false | NOT a Sapper |
| 18 | SwarmCount | 0 | |
| 19 | AoERadius | 250 | the splash — drives TASK-298 fireball |
| 20 | MinRange | 0 | |
| 21 | SpawnCardID | None | |
| 22 | SpawnInterval | 0 | |
| 23 | Lifetime | 0 | |
| 24 | CardArt | /Game/UI/CardArt/T_CardArt_Wizard.T_CardArt_Wizard | full object path, CONVENTIONS |
| 25 | SpellEffect | None | not a spell |
| 26 | EffectDuration | 0 | |
| 27 | MaxTargets | 0 | |
| 28 | GoldSteal | 0 | |
| 29 | ChainTargets | 0 | |
| 30 | ChainFalloff | 0 | |
| 31 | SpellDelivery | (empty) | trailing comma, like Archer |

## Verification (PowerShell, run against the saved file)
- Header columns = **31**; Wizard row columns = **31**.
- DeckCount (col 13) sum across all rows = **50** (unchanged; Wizard adds 0).
- Notes cell contains no comma (CSV-safe unquoted, matches the file's existing unquoted-Notes style).

## What QA should scrutinize (TASK-299)
- 31-col schema match + no reorder; AoERadius 250 present so TASK-298 gives the Wizard splash + M7.7 auto-description is truthful; 50-sum intact.
- `Docs/Data/cards.csv` is the source of truth; DT_Cards.uasset reimport is TASK-304 (build-master) — NOT touched here.
