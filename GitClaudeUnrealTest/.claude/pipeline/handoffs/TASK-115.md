# TASK-115 handoff — Curated legal 50-card default deck (cards.csv DeckCount re-author)

**Assignee:** gameplay-programmer
**Status:** ready-for-qa
**Scope:** data only. Re-authored ONLY the `DeckCount` column of `Docs/Data/cards.csv`. NO editor/MCP, NO compile, NO git, NO TASKBOARD edits. The DT_Cards reimport rides TASK-117 (build-master).

## File touched
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\Data\cards.csv` — `DeckCount` column only (column index 12, the field immediately after each row's `Notes` and before `bRanged`).

## What changed
24 of 28 data rows had their DeckCount value edited; 4 rows (Longbowman, Cleric, Ogre, Fireball) were already at their target value (2) and were left untouched. Verified by `git diff --word-diff`: on every changed line ONLY the DeckCount integer differs — Cost / MaxCopies / HP / Damage / stats / Notes / CardArt / spell columns are byte-identical (see "Collateral check" below). No columns added, no rows added/removed.

## Full 28-row DeckCount list (CardID → DeckCount, with Cost / MaxCopies)

| # | CardID | CardType | Cost | MaxCopies | DeckCount | Role in starter |
|---|--------|----------|-----:|----------:|----------:|-----------------|
| 1 | Footman | Unit | 3 | 12 | **12** | Frontline backbone (cheap melee spam) |
| 2 | Archer | Unit | 4 | 10 | **8** | Ranged backbone |
| 3 | Knight | Unit | 6 | 6 | **3** | Frontline heavy tank |
| 4 | Miner | Economy | 8 | 4 | **3** | Economy engine (+1 gold/s) |
| 5 | ArrowTower | Building | 5 | 8 | **3** | Anti-unit tower (defense) |
| 6 | Wall | Building | 4 | 10 | **4** | Pathing block (defense) |
| 7 | MilitiaMob | Unit | 5 | 6 | **3** | Swarm value frontline |
| 8 | Pikeman | Unit | 5 | 6 | **3** | Slayer / anti-tank frontline |
| 9 | Sapper | Unit | 5 | 4 | 0 | excluded (advanced siege suicide) |
| 10 | Cavalry | Unit | 7 | 4 | **3** | Charge finisher |
| 11 | Longbowman | Unit | 6 | 4 | **2** | Long-range ranged punch |
| 12 | Cleric | Unit | 6 | 3 | **2** | Support sustain (heal) |
| 13 | Ogre | Unit | 12 | 2 | **2** | Heavy siege win-condition (finisher) |
| 14 | BombTower | Building | 8 | 4 | 0 | excluded (niche AoE tower) |
| 15 | BallistaTower | Building | 7 | 4 | 0 | excluded (niche long-range tower) |
| 16 | Barracks | Building | 10 | 3 | 0 | excluded (advanced spawner) |
| 17 | DeepMine | Economy | 15 | 2 | 0 | excluded (15-cost economy; Miner covers econ) |
| 18 | Masons | Utility | 8 | 3 | 0 | excluded (situational repair) |
| 19 | SharpenedBlade | HeroUpgrade | 6 | 2 | 0 | excluded (hero-upgrade meta layer) |
| 20 | PlateArmor | HeroUpgrade | 6 | 2 | 0 | excluded (hero-upgrade meta layer) |
| 21 | SwiftBoots | HeroUpgrade | 5 | 1 | 0 | excluded (hero-upgrade meta layer) |
| 22 | WarBanner | HeroUpgrade | 8 | 1 | 0 | excluded (hero-upgrade meta layer) |
| 23 | Fireball | Spell | 7 | 3 | **2** | Spell reach / chip finisher (the one teaching spell) |
| 24 | FrostNova | Spell | 6 | 3 | 0 | excluded (advanced spell) |
| 25 | Lightning | Spell | 8 | 2 | 0 | excluded (advanced spell) |
| 26 | BattleCry | Spell | 5 | 3 | 0 | excluded (advanced spell) |
| 27 | Pickpocket | Spell | 6 | 2 | 0 | excluded (advanced spell) |
| 28 | CrystalTower | Building | 9 | 3 | 0 | excluded (advanced chain tower) |

Included cards (DeckCount > 0): **13 distinct**. Excluded (DeckCount 0): 15.

## Hard-constraint verification (verified by hand AND by script over the live file)

**sum(DeckCount) = 12+8+3+3+3+4+3+3+3+2+2+2+2 = 50** — EXACTLY 50. PASS.

**Per-card cap (DeckCount <= MaxCopies), only the non-zero rows (zeros trivially pass):**
- Footman 12 <= 12 (at cap, intentional) ✓
- Archer 8 <= 10 ✓
- Knight 3 <= 6 ✓
- Miner 3 <= 4 ✓
- ArrowTower 3 <= 8 ✓
- Wall 4 <= 10 ✓
- MilitiaMob 3 <= 6 ✓
- Pikeman 3 <= 6 ✓
- Cavalry 3 <= 4 ✓
- Longbowman 2 <= 4 ✓
- Cleric 2 <= 3 ✓
- Ogre 2 <= 2 (at cap, intentional) ✓
- Fireball 2 <= 3 ✓
All PASS.

**Average cost = sum(Cost x DeckCount) / 50 = 254 / 50 = 5.08.** In the §8 healthy band (~4–6; <4 spams, >7 bricks). PASS. (Script-confirmed sum(Cost x DeckCount) = 254.)

**Only real CardIDs:** every non-zero DeckCount is on an existing row; no new rows/IDs. PASS.

## Design rationale (coherent starter, all required pillars present)
Focused "Footman value" starter that teaches fundamentals with a smooth curve rather than the old one-of-everything M4/M5 test spread. Every pillar the spec named is represented:
- **Frontline:** Footman 12 (cheap backbone) + Knight 3 (tank) + MilitiaMob 3 (swarm) + Pikeman 3 (anti-tank slayer) = 21 melee cards.
- **Ranged:** Archer 8 + Longbowman 2 = 10 ranged cards.
- **Tower/Wall:** ArrowTower 3 + Wall 4 = 7 defensive buildings.
- **Economy:** Miner 3 (single, cheap-enough econ engine; DeepMine's 15-cost is too heavy for a starter).
- **Support:** Cleric 2 (sustain, bonus beyond the named pillars).
- **Finisher spread:** Cavalry 3 (charge) + Ogre 2 (heavy siege win-con) + Fireball 2 (spell reach) = a real, reliable top end.

Exclusions are deliberate: hero upgrades and the advanced spells/towers (FrostNova, Lightning, BattleCry, Pickpocket, BombTower, BallistaTower, Barracks, CrystalTower, DeepMine, Masons, Sapper) are the meta/advanced layer a player unlocks via the deck-builder — kept out of the basic starter so the default deck stays approachable and on-curve. The two "at cap" picks (Footman 12/12, Ogre 2/2) are intentional identity statements, not accidents, and remain legal (<=).

## Notes-column decision (why no in-CSV comment trail)
The spec allowed a one-line CSV Notes/comment "only if the format already supports a comment column." It does NOT: the `Notes` column is a per-row, already-populated design-note column (frozen), and a DataTable CSV import has no standalone comment/trailer-row syntax. Writing this composition into any CSV cell would touch a frozen column and/or risk the DT_Cards reimport. Therefore the composition trail lives HERE in the handoff only, as the spec's fallback directs. No Notes cell was altered.

## Collateral check for QA
`git diff --word-diff --unified=0 -- Docs/Data/cards.csv` shows 24 changed lines; on each, the ONLY delta is the DeckCount integer between the Notes text and `bRanged`. Every other field (Cost, MaxCopies, HP, Damage, Range, Cadence, Speed, Profile, Notes, all bool flags, SwarmCount/AoERadius/MinRange/Spawn*/Lifetime, CardArt, SpellEffect, EffectDuration, MaxTargets, GoldSteal, ChainTargets, ChainFalloff) is unchanged. 24 insertions / 24 deletions, 0 rows added/removed, 0 columns added.

## What QA should scrutinize
1. Re-sum the DeckCount column independently → must be exactly 50.
2. Re-check each non-zero DeckCount <= that same row's MaxCopies.
3. Confirm no other column moved (word-diff above).
4. Sanity-check the average (254/50 = 5.08) sits in the §8 band.
5. Confirm this is a coherent starter with the 5 named pillars (frontline/ranged/tower-wall/economy/finisher) — all present.

## Downstream
- TASK-117 (build-master) reimports `/Game/Data/DT_Cards` from this CSV and re-verifies sum==50 / caps on the live table.
- This DeckCount column is BOTH the deck-builder "reset to default" template AND the match fallback when no legal saved active deck exists (CONVENTIONS "Deck-builder & saved decks (M6)"; M6 ruling 3). Single source of truth (§3.0) — no deck is hardcoded anywhere else.
