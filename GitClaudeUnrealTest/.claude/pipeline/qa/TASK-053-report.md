# QA Report — TASK-053
Verdict: PASS

Card data: FCardRow keyword columns + cards.csv Set II rows + M4 test deck (files only; no editor/compile/Git).
Reviewed against: TASKBOARD `### TASK-053` spec + `### M4 design rulings`, GDD §4 (Core + Set II tables), `handoffs/TASK-053.md`.

Counts: BLOCKER 0 · WARN 0 · NIT 0

## Explicit confirmations (as requested)
- **header ↔ FCardRow 1:1 (name AND order): CONFIRMED.** FCardRow declares 22 UPROPERTYs in order
  DisplayName, CardType, Cost, MaxCopies, HP, Damage, Range, Cadence, Speed, Profile, Notes, DeckCount,
  bRanged, bCharge, bSlayer, bSuicide, SwarmCount, AoERadius, MinRange, SpawnCardID, SpawnInterval, Lifetime.
  The CSV header's 22 data columns (after the empty leading row-name column) are identical in name and order.
  Reimport at TASK-061 should warn ZERO.
- **DeckCount sum = 50: CONFIRMED (re-added independently).** Core 20 (Footman 6, Archer 4, Knight 2,
  Miner 3, ArrowTower 2, Wall 3) + Set II units 14 (MilitiaMob/Pikeman/Sapper/Cavalry/Longbowman/Cleric/
  Ogre = 2 each) + buildings 8 (BombTower 2, BallistaTower 2, Barracks 2, DeepMine 2) + instant/upgrade 8
  (Masons 2, SharpenedBlade 2, PlateArmor 2, SwiftBoots 1, WarBanner 1) = 50. Every row ≥1; every row
  ≤ MaxCopies (Ogre 2≤2, DeepMine 2≤2, SharpenedBlade 2≤2, PlateArmor 2≤2, SwiftBoots 1≤1, WarBanner 1≤1
  are at cap and valid).
- **All 16 Set II rows match GDD §4 / spec §4: CONFIRMED.** No deviation from any §4 value; no invented stat.

## Findings
(none)

## Detail — what was verified

### FCardRow.h (9 new UPROPERTYs)
- Types + defaults exact: `bool bCharge=false`, `bool bSlayer=false`, `bool bSuicide=false`,
  `int32 SwarmCount=0`, `float AoERadius=0.0f`, `float MinRange=0.0f`, `FName SpawnCardID=NAME_None`,
  `float SpawnInterval=0.0f`, `float Lifetime=0.0f`. All `UPROPERTY(EditAnywhere, BlueprintReadOnly)`
  (reflection-visible → importable). Appended after `bRanged`, matching header order.
- No enum values added. ECardType {Unit, Building, Economy, Spell, HeroUpgrade, Utility} and
  ECardProfile {None, Standard, Siege, Support} unchanged — both already carried every value Set II needs.
- No shadowing / no name collisions (plain FTableRowBase struct; C4457/58/59 N/A). Header comment updated.

### cards.csv structural
- 23 header fields (1 empty row-name + 22 properties); exactly 22 data rows; every row has exactly 23 fields.
- CardIDs (col 1) PascalCase, no spaces: Footman, Archer, Knight, Miner, ArrowTower, Wall, MilitiaMob,
  Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower, BallistaTower, Barracks, DeepMine, Masons,
  SharpenedBlade, PlateArmor, SwiftBoots, WarBanner. DisplayName carries the spaces (Militia Mob, Arrow
  Tower, Bomb Tower, Ballista Tower, Deep Mine, Sharpened Blade, Plate Armor, Swift Boots, War Banner).
- No legacy / duplicate / extra rows. Notes strings are comma-free (semicolons/parens only) → no CSV quoting.
- Enum text values are literal enum entry names (Unit/Building/Economy/Spell N/A here/HeroUpgrade/Utility;
  None/Standard/Siege/Support) → parse cleanly on DataTable import (same convention as M1–M3 core rows).
- `SpawnCardID` default written `None` → round-trips to NAME_None; Barracks writes `Footman` → FName.

### 6 core rows — stats byte-unchanged
Cost/HP/Damage/Range/Cadence/Speed/Profile/bRanged all identical to the pre-M4 file and to GDD Core table
(Footman 3/80/12/120/1.0/400; Archer 4/45/10/700/1.2/350 bRanged; Knight 6/200/15/120/1.2/300;
Miner 8/30/…/350 Economy; ArrowTower 5/150/15/900/1.5 bRanged; Wall 4/300). Only changes: DeckCount
redistributed (per the M4 test-deck ruling, which intentionally supersedes M3's core-only counts) and the
9 new columns appended with defaults (false/0/None). No accidental behavior edit to any core card.

### 16 Set II rows — each cross-checked vs §4 (Cost/Max/HP/Dmg/Range/Cadence/Speed/Type/Profile + keywords)
- MilitiaMob: Unit 5/6/25/6/120/1.0/400 Standard, SwarmCount=4 (only). ✓
- Pikeman: Unit 5/6/100/30/120/1.5/350 Standard, bSlayer=true (only). ✓
- Sapper: Unit 5/4/60/80/120/1.0/500 Siege, bSuicide=true + AoERadius=250 (GDD "contact/once" → spec-fixed
  Range 120 / Cadence 1.0). ✓
- Cavalry: Unit 7/4/140/20/120/1.0/600 Standard, bCharge=true (only). ✓
- Longbowman: Unit 6/4/70/18/1200/1.5/300 Standard, bRanged=true (only). ✓
- Cleric: Unit 6/3/90/8/400/1.0/350 Support (Dmg 8 = heal/s; GDD "—" cadence → spec-fixed 1.0); no keyword. ✓
- Ogre: Unit 12/2/500/35/120/1.5/250 Siege; no keyword. ✓
- BombTower: Building 8/4/180/25/800/2.5/0 None, bRanged=true + AoERadius=250. ✓
- BallistaTower: Building 7/4/120/45/1400/3.0/0 None, bRanged=true + MinRange=300. ✓
- Barracks: Building 10/3/250/0/0/0/0 None, SpawnCardID=Footman + SpawnInterval=8 + Lifetime=60. ✓
- DeepMine: Economy 15/2/200/0/0/0/0 None; no keyword (+2 gold/s is an ADeepMine UPROPERTY). ✓
- Masons: Utility 8/3, all stats 0, None; no keyword (Instant). ✓
- SharpenedBlade: HeroUpgrade 6/2, all stats 0, None. ✓
- PlateArmor: HeroUpgrade 6/2, all stats 0, None. ✓
- SwiftBoots: HeroUpgrade 5/1, all stats 0, None. ✓
- WarBanner: HeroUpgrade 8/1, all stats 0, None. ✓

Keyword-column sparsity verified across ALL 22 rows: bCharge only Cavalry; bSlayer only Pikeman; bSuicide
only Sapper; SwarmCount only MilitiaMob(4); AoERadius only Sapper(250)+BombTower(250); MinRange only
BallistaTower(300); SpawnCardID/Interval/Lifetime only Barracks; bRanged only Archer/ArrowTower/Longbowman/
BombTower/BallistaTower. No stray keyword on any other row.

## Notes for build-master (if PASS)
- Data-only PASS. No compile occurred here; TASK-053 compiles as part of the TASK-068 M4 C++ batch — the new
  FCardRow struct must be compiled BEFORE the DT_Cards reimport at TASK-061 (board already sequences this:
  061 blocked-by 068).
- On reimport at TASK-061, expect ZERO "column not found in row struct" warnings (header ↔ struct is 1:1).
- The M4 test-deck DeckCount (sum 50) intentionally supersedes M3's core-only counts — not a regression.
- Cadence=0 on non-attacking cards (Barracks, DeepMine, Masons, all upgrades; and core Wall/Miner) is by
  design; the FCardRow default of 1.0 is deliberately overridden to 0 in these rows. Any divide-by-cadence
  guard is a downstream (TASK-054..060) concern, not a data defect.
