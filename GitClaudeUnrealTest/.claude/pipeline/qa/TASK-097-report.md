# QA Report — TASK-097
Verdict: **PASS**

Reviewed 2026-07-08 by qa-reviewer. Pre-compile review of `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` + `Docs/Data/cards.csv` against the TASK-097 spec, M5 manager decisions 2/9/12/13, CONVENTIONS "Spells & Set III (M5)" + "Data-driven card stats", and GDD §4 Set III. Blockers: 0. Warnings: 1. Nits: 2.

## Verification performed (independent, not trusting the handoff)

1. **CSV header ↔ UPROPERTY 1:1** — header has 29 named columns after the row-name cell; FCardRow declares exactly 29 UPROPERTYs in the same names, character-for-character (`DisplayName` … `CardArt`, `SpellEffect`, `EffectDuration`, `MaxTargets`, `GoldSteal`, `ChainTargets`, `ChainFalloff`). New columns appended at the END after `CardArt`; no existing member renamed or reordered. PASS.
2. **ESpellEffect** — `UENUM(BlueprintType) enum class : uint8 { None, AoEDamage, Freeze, TopTargetsDamage, AllyBuff, GoldSteal }`, character-for-character per the CONVENTIONS registry, declared before FCardRow. CSV enum cells use bare value names — same importer convention already proven by `CardType`/`Profile` since M1. PASS.
3. **Row count** — 28 data rows (22 existing + Fireball, FrostNova, Lightning, BattleCry, Pickpocket, CrystalTower). Field counts spot-verified by hand on Fireball, Pickpocket, CrystalTower: 30 fields each, matching the 30-field header; no commas inside Notes cells. PASS.
4. **Set III values vs GDD §4** (checked against the GDD table directly, not just the spec): costs 7/6/8/5/6/9 ✓; MaxCopies 3/3/2/3/2/3 ✓; Fireball Damage 100 / AoERadius 300 / AoEDamage ✓; FrostNova AoERadius 350 / EffectDuration 4 / Freeze ✓; Lightning Damage 200 / AoERadius 400 / MaxTargets 3 / TopTargetsDamage ✓; BattleCry AoERadius 400 / EffectDuration 8 / AllyBuff ✓; Pickpocket GoldSteal 10 / GoldSteal ✓; CrystalTower Building, HP 150, Damage 15, Range 800, Cadence 1.5, ChainTargets 3, ChainFalloff 5 (⇒ 15/10/5), bRanged false per ruling 9, SpellEffect None, AoERadius 0 (chain bounce radius is the TASK-101 mechanic UPROPERTY, correctly NOT in data). PASS.
5. **CardArt paths** — all 6 new rows carry `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` with CardID casing (T_CardArt_FrostNova, not "Frost Nova"). Ruling 12 satisfied; graceful fallback until TASK-106. PASS.
6. **DisplayName law** — spaced human names on FrostNova/BattleCry/CrystalTower; row names PascalCase CardIDs. PASS.
7. **Deck re-sum (ruling 13, independently summed)** — 5+3+2×14+1×6 (M4 block) + 2+1+2+1+1+1 (Set III) = **exactly 50**. DeckCount ≤ MaxCopies on all 28 rows (tight: Ogre 2≤2, SwiftBoots/WarBanner 1≤1, Lightning 2≤2). Every Set III card ≥1; all 22 M4-era cards remain ≥1. The 8 documented cuts (Footman 6→5, Archer 4→3, Miner 3→2, Wall 3→2, DeepMine 2→1, Masons 2→1, SharpenedBlade 2→1, PlateArmor 2→1) match the file exactly. PASS.
8. **No M5-column leakage** — all 22 pre-M5 rows end `None,0,0,0,0,0`. PASS.
9. **Shadow-scan (C4457/C4458/C4459, mandatory)** — the diff is a header-only struct/enum extension plus CSV: no function scopes, no locals, no member shadowing. `ESpellEffect::GoldSteal` vs the `int32 GoldSteal` member is a scoped-enum value vs a field — no collision at C++, UHT, or Blueprint level. **Clean.**
10. **UE 5.8 API check** — FTableRowBase, UENUM(BlueprintType), TSoftObjectPtr, appended-UPROPERTY DataTable reimport: all current, nothing deprecated. Header-only struct needs no .cpp (UHT emits CardRow.gen.cpp). PASS.
11. **Downstream-switch audit claims verified by reading the code** (line numbers drifted slightly from the handoff due to parallel-task work in flight; content matches):
    - `SiegePlayerController.cpp:383` PlayHandSlot switch — `case ECardType::Spell:` (line 401) refuses with "Spells are not available yet", AFTER the CanAfford check but with zero gold movement (RefuseCardPlay only). Set III spells in hand are safe until TASK-100 rewires this case. Confirmed.
    - `SiegePlayerController.cpp:1197` ResolveCardActorClass / `:1247` IsBuildingCard — CrystalTower (CardType Building) flows the composed `/Game/Blueprints/Buildings/BP_Building_CrystalTower` soft-class path with zero code change; spells are gated out by the PlayHandSlot switch and can never reach the placement path. Confirmed.
    - `SiegeBotController.cpp:61` IsUnplayableByBot — Spell/HeroUpgrade/Utility = rule-5 discard set (TASK-102 revises); `:38` IsDefensiveType — Unit/Building only, so CrystalTower becomes a legitimate bot defensive play once DT_Cards reimports. Confirmed.
    - `Building.cpp:175` — `Row->CardType != ECardType::Building` warning cannot fire for CrystalTower (row IS Building). Confirmed.
    - Grep for every other `ECardType::` site found none outside the handoff's audit (remaining hits are placement-type internals unreachable by spells and the HeroUpgrade instant path). The audit is complete.

## Findings

- [WARN] Docs/Data/cards.csv (window risk, not a data defect) — once TASK-104 reimports DT_Cards, CrystalTower is drawable and bot-playable (Building = defensive-play type) BEFORE `BP_Building_CrystalTower` / `SM_CrystalTower` exist (TASK-105/107). The composed soft-class path refuses null-safely with no gold moved (verified in ResolveCardActorClass), so it degrades to a temporarily dead card, not a crash. **Carry-forward to TASK-104/109:** sequence the DT_Cards reimport with (or after) BP_Building_CrystalTower creation, or accept the refusal window knowingly.
- [NIT] handoffs/TASK-097.md cuts table — the DeepMine row reads "Was: 15g | Now: 2→1" (cost leaked into the Was column). Documentation only; the CSV value (DeckCount 1) is correct and the sum holds.
- [NIT] CardRow.h:137 — the `AoERadius` doc comment still describes only "splash radius for area attackers"; spell reuse is documented in the M5 block comment (lines 170-173) instead of on the member. Acceptable as-is; fold into the member comment on the next touch of this file.

## Rulings on flagged decisions (all 7)

1. **ECardType already contains Spell — no enum change: ACCEPTED (PASS).** Verified: `Spell` has been in ECardType since M1 (CardRow.h:22). The spec's "add Spell if absent" condition is not met, and the required flag + downstream-switch audit was delivered and is accurate (verified point 11 above). This is exactly what ruling 2 asked for.
2. **CardRow.cpp not created: ACCEPTED (PASS).** Confirmed no CardRow.cpp exists in the tree; FCardRow has been header-only since M1 and the TASK-053 M4 column batch followed the same shape. The names block's "CardRow.h/.cpp" is the boilerplate file-pair pattern, not a mandate for a dead translation unit — UHT generates the reflection .gen.cpp regardless. Do NOT add an empty .cpp.
3. **Spell rows Cadence 0 (vs struct default 1.0): ACCEPTED (PASS).** Matches every existing stat-less row (Wall, Masons, all four upgrades carry Cadence 0 in the file today). No code reads Cadence for non-attacking cards; the struct default only applies to columns absent from the CSV, and Cadence is present.
4. **SharpenedBlade/PlateArmor cut to 1 copy — §3.10 stack cap 2 unreachable from the M5 test deck: ACCEPTED (PASS), with carry-forward.** The M4 stack-cap acceptance is already signed off (M4 sign-off 2026-07-08); ruling 13 explicitly authorizes carving 8 slots from the M4 spread, and cutting bot-dead cards is the least-regressive choice. Not a regression. **Carry-forward to M6 (deck-builder):** the curated-deck flow must re-verify the cap-2 path when players can again run 2 copies.
5. **Pickpocket = CardType Spell + SpellEffect GoldSteal, no instant flag in data: ACCEPTED (PASS).** Matches ruling 7 exactly (instant resolve is behavior, dispatched on `SpellEffect == GoldSteal`; CardType stays Spell). **Carry-forward to TASK-100 QA:** verify the controller special-cases GoldSteal spells BEFORE entering targeting mode (no reticle), and to TASK-102 QA that the bot never tries to "target" it.
6. **Lightning 200 with resolver-side castle rules: ACCEPTED (PASS).** Correct per rulings 3/4 — nothing in data encodes the 50% castle scaling (TASK-098's `USiegeDamageType_Spell` branch) or the castle exclusion (Lightning targeting per ruling 4; note Fireball's blast CAN hit the castle at 50%, Lightning's targeting EXCLUDES it entirely — the Notes cells state each correctly). Notes are designer-readability only; no code parses them.
7. **Shadow-scan clean claim: VERIFIED (PASS).** Independently re-scanned; concur (finding 9 above).

## Notes for build-master

- Files in this task compile-batch at TASK-103; no CardRow.cpp is expected — do not treat its absence as a missing file.
- After TASK-103, TASK-104 must reimport `/Game/Data/DT_Cards` from Docs/Data/cards.csv (28 rows) or none of the M5 columns exist at runtime; mind the WARN above about CrystalTower's BP window.
- No .uasset was touched by this task; worktree residue in SiegePlayerController.h / SiegeBotController.cpp / pipeline docs / Content/Fab belongs to parallel M5/M4.5 tasks, not TASK-097.
