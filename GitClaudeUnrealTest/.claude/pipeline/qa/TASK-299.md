# QA Report — TASK-299

Verdict: PASS

Scope: data review. File: `Docs/Data/cards.csv` (one row appended). Cross-checked against the header schema and the CONVENTIONS "Wizard unit — AoE fireball caster (2026-07-26)" defaults.

## Findings
- No blockers, warnings, or nits.

## Verified
- **Column count = 31, no reorder/added columns.** Header (line 1) has 31 columns; the appended `Wizard` row (line 30) has exactly 31 fields (30 commas + trailing empty `SpellDelivery`), aligned position-for-position with the header. Appended AFTER `CrystalTower` (line 29); header and all prior rows unchanged.
- **Values match CONVENTIONS Wizard defaults exactly:** Cost 24 / MaxCopies 4 / HP 45 / Damage 15 / Range 700 / Cadence 1.6 / Speed 350 / Profile Standard / bRanged true / bCharge,bSlayer,bSuicide false / SwarmCount 0 / **AoERadius 250** / MinRange 0 / SpawnCardID None / SpawnInterval,Lifetime 0 / SpellEffect None / EffectDuration,MaxTargets,GoldSteal,ChainTargets,ChainFalloff 0 / SpellDelivery empty.
- **CardArt path correct:** `/Game/UI/CardArt/T_CardArt_Wizard.T_CardArt_Wizard` (full `Package.Object` object path, matches CONVENTIONS and the Archer/roster pattern).
- **AoERadius 250** is present and non-zero — this is the data that drives the TASK-298 splash, so once TASK-298 ships the M7.7 auto-description ("splash radius 250") is truthful.
- **DeckCount = 0** — the sum of column 13 across all rows remains **50** (Wizard contributes 0), preserving the curated-deck invariant. No other row's DeckCount was touched.
- **CSV integrity:** the `Notes` cell "Ranged AoE fireball unit; splash over AoERadius (defaults - tune at playtest)" contains NO comma (semicolon + hyphen only), so it stays a single unquoted field — consistent with the file's unquoted-Notes style. Trailing comma for the empty `SpellDelivery` matches the Archer/roster formatting.

## Notes for build-master
`Docs/Data/cards.csv` is the source of truth; the `/Game/Data/DT_Cards` same-path reimport + `Wizard`-row readback (Cost 24, bRanged true, AoERadius 250, DeckCount 0) is TASK-304. Data is clean and ready to reimport.
