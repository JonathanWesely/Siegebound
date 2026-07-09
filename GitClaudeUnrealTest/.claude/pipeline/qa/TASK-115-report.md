# QA Report — TASK-115
Verdict: PASS

Curated legal 50-card default deck — re-authored `DeckCount` column of `Docs/Data/cards.csv`.
Data-legality review (not a compile review). All numbers below are recomputed INDEPENDENTLY from the
worktree file (`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\Data\cards.csv`),
not taken from the handoff.

## 1. sum(DeckCount) == 50 — PASS (recomputed by hand across all 28 rows)

Non-zero rows only (all other 15 rows are DeckCount 0, trivially fine):

```
Footman     12
Archer       8   -> 20
Knight       3   -> 23
Miner        3   -> 26
ArrowTower   3   -> 29
Wall         4   -> 33
MilitiaMob   3   -> 36
Pikeman      3   -> 39
Cavalry      3   -> 42
Longbowman   2   -> 44
Cleric       2   -> 46
Ogre         2   -> 48
Fireball     2   -> 50
```
Running total = **50 exactly**. PASS. (13 distinct included cards; 15 excluded.)

## 2. Per-row cap: DeckCount <= that row's MaxCopies — PASS (every row checked against its own cells)

| CardID     | Cost | MaxCopies | DeckCount | <= cap? | Note |
|------------|-----:|----------:|----------:|:-------:|------|
| Footman    | 3    | 12        | 12        | ✓       | AT CAP (12/12) — legal, intentional |
| Archer     | 4    | 10        | 8         | ✓       | |
| Knight     | 6    | 6         | 3         | ✓       | |
| Miner      | 8    | 4         | 3         | ✓       | |
| ArrowTower | 5    | 8         | 3         | ✓       | |
| Wall       | 4    | 10        | 4         | ✓       | |
| MilitiaMob | 5    | 6         | 3         | ✓       | |
| Pikeman    | 5    | 6         | 3         | ✓       | |
| Cavalry    | 7    | 4         | 3         | ✓       | |
| Longbowman | 6    | 4         | 2         | ✓       | |
| Cleric     | 6    | 3         | 2         | ✓       | |
| Ogre       | 12   | 2         | 2         | ✓       | AT CAP (2/2) — legal, intentional |
| Fireball   | 7    | 3         | 2         | ✓       | |

All 15 excluded rows: DeckCount 0 <= MaxCopies (all MaxCopies >= 1). PASS.
The two at-cap picks are `<=` and therefore legal (validator uses `[0..MaxCopies]`, TASK-113 `IsDeckLegal`).

## 3. Only real CardIDs — PASS

All 13 non-zero rows are existing roster CardIDs. Row count = 28 data rows (header + 28 = 29 content lines),
matching the documented §4 28-card pool. No rows added, removed, or renamed. Row order unchanged vs the
handoff's 28-row list.

## 4. Average cost — PASS (recomputed)

sum(Cost x DeckCount): 36 + 32 + 18 + 24 + 15 + 16 + 15 + 15 + 21 + 12 + 12 + 24 + 14 = **254**.
254 / 50 = **5.08**. Sits inside the §8 healthy band (~4–6; <4 spams, >7 bricks). Matches the handoff. PASS.

## 5. Collateral-edit check (ONLY DeckCount changed) — PASS with WARN on method

IMPORTANT METHOD NOTE: qa-reviewer has **no Git/shell access by design** (the Bash tool is not enabled in
this context), so I could NOT run the authoritative `git diff --word-diff HEAD -- Docs/Data/cards.csv`
byte-diff that the task requested. I verified the constraint STRUCTURALLY instead, and found no evidence of
any collateral edit:

- **Header intact:** 30 columns, `DeckCount` at index 12 (0-based) — immediately after `Notes` (11) and
  before `bRanged` (13), exactly as the schema (CONVENTIONS "Data-driven card stats") and handoff state.
- **Field count intact:** spot-counted every field on representative rows (Footman, Fireball, Lightning,
  CrystalTower, Barracks) — each has exactly 30 fields; no column added/removed, no field-shift. No Notes
  cell contains an embedded comma, so no parse-quoting risk.
- **Frozen columns spot-check vs the documented M5 committed schema (all consistent):**
  Cavalry `bCharge=true`; Pikeman `bSlayer=true`; Sapper `bSuicide=true, AoERadius=250`;
  MilitiaMob `SwarmCount=4`; Barracks `SpawnCardID=Footman, SpawnInterval=8, Lifetime=60`;
  BallistaTower `MinRange=300`; Fireball `SpellEffect=AoEDamage, Damage=100, AoERadius=300`;
  FrostNova `Freeze, AoERadius=350, EffectDuration=4`; Lightning `TopTargetsDamage, Damage=200,
  AoERadius=400, MaxTargets=3`; BattleCry `AllyBuff, AoERadius=400, EffectDuration=8`;
  Pickpocket `GoldSteal, GoldSteal=10`; CrystalTower `ChainTargets=3, ChainFalloff=5`.
  Every `CardArt` path is the `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` law. Cost/MaxCopies/
  HP/Damage/Range/Cadence/Speed/Profile all hold plausible, schema-correct values.
- No structural evidence of any change outside the DeckCount column.

I could not byte-confirm the "4 rows already at target, 24 changed" claim (needs the pre-edit blob). Not a
defect in the artifact — a limitation of QA's access. See build-master handoff below; this is already
covered by TASK-117's spec.

## 6. Coherence sanity (soft — playable starter, not degenerate) — OK

All named pillars present: frontline (Footman 12 + Knight 3 + MilitiaMob 3 + Pikeman 3 = 21) · ranged
(Archer 8 + Longbowman 2 = 10) · tower/wall defense (ArrowTower 3 + Wall 4 = 7) · economy (Miner 3) ·
support (Cleric 2) · finisher spread (Cavalry 3 + Ogre 2 + Fireball 2 = 7). Totals 50. Smooth curve,
cheap-melee-forward with a real top end. Not degenerate. Coherent starter.

## Findings
- [WARN] Docs/Data/cards.csv — the authoritative `git diff` for the "ONLY DeckCount changed" guarantee was
  NOT run here (QA has no Git/shell access by design). Verified structurally instead (section 5); no
  collateral edit found, but build-master MUST run the byte-diff before commit (already in TASK-117 scope).
- [NIT] Docs/Data/cards.csv — Footman is at cap 12/12 = 24% of the 50-card deck (deliberate "Footman value"
  identity per handoff). Legal and in-band; noting the concentration for the playtest, not a fail.
- [NIT] Docs/Data/cards.csv — single economy card (Miner, 3 copies); DeepMine excluded for its 15 cost.
  Coherent for a starter; noting for balance visibility. Not a fail (taste, not legality).

## Notes for build-master (TASK-117)
1. Run the authoritative diff before the single M6 commit:
   `git diff --word-diff HEAD -- Docs/Data/cards.csv` — confirm the ONLY per-line delta is the DeckCount
   integer, 0 rows/columns added or removed. Any collateral edit to a frozen column (Cost/MaxCopies/stats/
   Notes/CardArt/spell columns) is a hard stop — route back, do NOT commit.
2. After reimporting `/Game/Data/DT_Cards`, re-verify on the LIVE table: `sum(DeckCount)==50` and each
   `DeckCount<=MaxCopies` (TASK-117 already owns this live re-check).
3. Independently confirmed values to match against the live table: sum = 50, avg cost = 5.08,
   13 included CardIDs (Footman 12 / Archer 8 / Knight 3 / Miner 3 / ArrowTower 3 / Wall 4 / MilitiaMob 3 /
   Pikeman 3 / Cavalry 3 / Longbowman 2 / Cleric 2 / Ogre 2 / Fireball 2).

Blockers: 0. Verdict: PASS.
