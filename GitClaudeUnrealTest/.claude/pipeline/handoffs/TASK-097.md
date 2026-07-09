# TASK-097 handoff — gameplay-programmer — 2026-07-08

Card data Set III: `ESpellEffect` + 6 M5 FCardRow columns + cards.csv 28 rows + M5 test deck.

## Files touched (complete list)

1. `Source/GitClaudeUnrealTest/Siegebound/CardRow.h`
   - NEW `UENUM(BlueprintType) enum class ESpellEffect : uint8 { None, AoEDamage, Freeze, TopTargetsDamage, AllyBuff, GoldSteal }` — declared after ECardProfile, values character-for-character per CONVENTIONS "Spells & Set III (M5)".
   - NEW FCardRow UPROPERTYs appended after `CardArt`, CSV-header names 1:1: `SpellEffect` (ESpellEffect, `None`), `EffectDuration` (float, 0), `MaxTargets` (int32, 0), `GoldSteal` (int32, 0), `ChainTargets` (int32, 0), `ChainFalloff` (int32, 0). Categories: the four spell-only columns under `Spell`; ChainTargets/ChainFalloff under `Keywords` (Chain is the M4-deferred keyword arriving typed, per the CONVENTIONS deferral note).
   - No existing member renamed/reordered; no other file includes changed.
2. `Docs/Data/cards.csv`
   - Header extended with the 6 new columns appended at the END (after `CardArt`) — importer maps by header name, not order (TASK-079 precedent).
   - All 22 existing rows extended with defaults `None,0,0,0,0,0`; no existing stat cell changed except the 8 DeckCount cuts below.
   - 6 NEW Set III rows (row names character-for-character): `Fireball`, `FrostNova`, `Lightning`, `BattleCry`, `Pickpocket`, `CrystalTower`.

NOT touched: TASKBOARD.md, DT_Cards (.uasset reimport = TASK-104), any other Source file. Parallel-task residue visible in the worktree (SiegePlayerController.h, pipeline docs, Content/Fab/) is NOT mine.

## Set III row data (verified against GDD §4 Set III + task spec)

| Row | Type | Cost | Max | Deck | Stats |
|---|---|---|---|---|---|
| Fireball | Spell | 7 | 3 | 2 | Damage 100, AoERadius 300, SpellEffect AoEDamage |
| FrostNova | Spell | 6 | 3 | 1 | AoERadius 350, EffectDuration 4, SpellEffect Freeze |
| Lightning | Spell | 8 | 2 | 2 | Damage 200, AoERadius 400, MaxTargets 3, SpellEffect TopTargetsDamage |
| BattleCry | Spell | 5 | 3 | 1 | AoERadius 400, EffectDuration 8, SpellEffect AllyBuff |
| Pickpocket | Spell | 6 | 2 | 1 | GoldSteal 10, SpellEffect GoldSteal |
| CrystalTower | Building | 9 | 3 | 1 | HP 150, Dmg 15, Range 800, Cadence 1.5, ChainTargets 3, ChainFalloff 5, bRanged FALSE (chain is instant-hit, M5 ruling 9), SpellEffect None |

DisplayNames carry the spaced human names (`Frost Nova`, `Battle Cry`, `Crystal Tower`) per the CardID/DisplayName law. CardArt cells written UP FRONT for all 6 (ruling 12): `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` — graceful text-only fallback until TASK-106 imports the textures.

## Verification (scripted CSV parse, not eyeballed)

- Data rows: **28** (22 existing + 6 Set III). Header 30 fields; every row 30 fields.
- DeckCount sum: **exactly 50**.
- DeckCount ≤ MaxCopies: **no violations** (Lightning 2≤2 and Pickpocket 1≤2 are the tight ones).
- Every Set III card ≥1 in deck (ruling 13); manager's suggested spread used verbatim: Fireball 2 / FrostNova 1 / Lightning 2 / BattleCry 1 / Pickpocket 1 / CrystalTower 1 = 8 slots.
- No non-Set-III row has a non-default value in any of the 6 new columns.
- All 22 M4-era cards remain ≥1 copy (whole roster still reachable pre-M6-deck-builder).

## The 8 deck cuts from the M4 spread (ruling 13 — documented for QA re-sum)

| Card | Was | Now | Rationale |
|---|---|---|---|
| Footman | 6 | 5 | most-duplicated common; still the densest unit (Fireball acceptance wants clustered Footmen — kept high) |
| Archer | 4 | 3 | second-most-duplicated common |
| Miner | 3 | 2 | economy tempo trim; miner cap unaffected |
| Wall | 3 | 2 | utility trim |
| DeepMine | 15g | 2→1 | slowest-tempo economy pick |
| Masons | 2 | 1 | slow utility; bot-dead card (bot discards Utility) |
| SharpenedBlade | 2 | 1 | bot-dead (bot discards HeroUpgrade); see flagged decision 4 |
| PlateArmor | 2 | 1 | bot-dead (bot discards HeroUpgrade); see flagged decision 4 |

Net: −8 existing +8 Set III = 50. Bot dead-card ratio roughly preserved once TASK-102 makes Fireball/Lightning bot-playable.

## Flagged decisions (QA scrutiny list)

1. **ECardType already contains `Spell` — NO enum change made** (ruling 2 flag). Current shape since M1: `{ Unit, Building, Economy, Spell, HeroUpgrade, Utility }` (CardRow.h). Every downstream ECardType branch, audited:
   - `SiegePlayerController.cpp:382` `PlayHandSlot` switch — `case ECardType::Spell:` (line ~400) currently REFUSES with "Spells are not available yet" (M2-era stub). **TASK-100 must rewire this case into targeting mode** (and TASK-098's instant path for GoldSteal spells). Until 100 lands, Set III spells in hand are safely refused, gold untouched — deck is shippable mid-wave.
   - `SiegePlayerController.cpp:1152` `ResolveCardActorClass` / `:1202` `IsBuildingCard` — CrystalTower is CardType **Building**, so it flows through the existing `BP_Building_<CardID>` soft-path composition with ZERO code change; spells never reach these (gated by the PlayHandSlot switch).
   - `SiegeBotController.cpp:52` `IsUnplayableByBot` — Spell currently = bot discard fodder; TASK-102 revises for Fireball/Lightning. `:36` `IsDefensiveType` — Unit/Building only, unaffected.
   - `Building.cpp:175` LoadStats "expected Building" warning — CrystalTower row IS Building, no warning fires.
2. **CardRow.cpp does not exist and was not created.** The names block says "CardRow.h/.cpp", but FCardRow has been header-only since M1 and the M4 column batch (TASK-053) followed the same shape; an empty .cpp would be dead weight. If QA reads the names block as mandating the file, it is a two-line mechanical add.
3. **Spell rows' Cadence = 0** (not the struct default 1.0) — matches every existing stat-less row (Wall, Masons, upgrades). No code reads Cadence for non-attacking cards.
4. **SharpenedBlade/PlateArmor cut to 1 copy** means the §3.10 stack-cap-2 ceiling is not reachable from the M5 test deck. Deliberate: M4 stack acceptance already passed and signed off; the M6 deck-builder restores player control. Called out so nobody reads it as a regression.
5. **Pickpocket row is CardType Spell with SpellEffect GoldSteal** — instant-resolve (no reticle) is the recorded ruling-7 deviation from §3.5; the DATA carries no special instant flag, the resolver/controller dispatch on `SpellEffect == GoldSteal` (TASK-098/100 contract).
6. **Lightning Damage 200 + Spell-damage law**: castle takes 50% via `USiegeDamageType_Spell` in `ACastle::TakeDamage` ONLY (TASK-098); buildings take FULL 200 so the Arrow Tower (150 HP) dies — nothing in the data encodes the 50%, it is resolver-side. Notes column says "50% vs castle" on Fireball / "castle excluded" on Lightning for designer readability only.
7. **Shadow-scan (C4458): clean.** Header-only struct + enums, no function scopes, no member shadows; `ESpellEffect::GoldSteal` vs the `GoldSteal` member is scoped-enum vs field — no conflict, no warning.

## Downstream consumers (for the orchestrator's routing)

- TASK-098 (USpellLibrary) dispatches on `SpellEffect`, reads Damage/AoERadius/EffectDuration/MaxTargets/GoldSteal.
- TASK-100 (targeting mode) rewires the PlayHandSlot Spell case (see flag 1).
- TASK-101 (Crystal Tower chain) reads ChainTargets/ChainFalloff.
- TASK-104 reimports DT_Cards from this CSV (editor task — NOT done here).
- TASK-106 fills the 6 CardArt paths already referenced.
