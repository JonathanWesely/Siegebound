# TASK-021 Handoff — Core-set card data: FCardRow columns + cards.csv rows

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no board edits — per dispatch gates while Jonathan is away)

## Files changed

1. `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` — two new UPROPERTYs appended to FCardRow (after `Notes`):
   - `int32 DeckCount = 0;` — Category "Card", comment cites GDD section 3.4
   - `bool bRanged = false;` — Category "Stats", comment cites GDD section 3.0
   Names match the CSV headers character-for-character (CONVENTIONS column registry). Defaults match the registry (0 / false).
2. `Docs/Data/cards.csv` — header extended with `,DeckCount,bRanged`; Footman row extended with `,12,false`; five new rows added in GDD section 4 table order.

## Resulting CSV (14 columns, 6 data rows)

Header: `,DisplayName,CardType,Cost,MaxCopies,HP,Damage,Range,Cadence,Speed,Profile,Notes,DeckCount,bRanged`

| RowName | DisplayName | Type | Cost | Max | HP | Dmg | Range | Cadence | Speed | Profile | DeckCount | bRanged |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Footman | Footman | Unit | 3 | 12 | 80 | 12 | 120 | 1.0 | 400 | Standard | 12 | false |
| Archer | Archer | Unit | 4 | 10 | 45 | 10 | 700 | 1.2 | 350 | Standard | 10 | true |
| Knight | Knight | Unit | 6 | 6 | 200 | 15 | 120 | 1.2 | 300 | Standard | 6 | false |
| Miner | Miner | Economy | 8 | 4 | 30 | 0 | 0 | 0 | 350 | None | 4 | false |
| ArrowTower | Arrow Tower | Building | 5 | 8 | 150 | 15 | 900 | 1.5 | 0 | None | 8 | true |
| Wall | Wall | Building | 4 | 10 | 300 | 0 | 0 | 0 | 0 | None | 10 | false |

Verified mechanically: 6 data rows, 14 cells per row, DeckCount sums to exactly 50 (12+10+6+4+8+10), bRanged true only on Archer + ArrowTower, all values match GDD section 4 / task spec. Encoding: pure ASCII, LF line endings, trailing newline — byte-checked with `tr`/`od` (note: MSYS grep `$'\r'` gives false positives on this box; trust `tr -cd '\r' | wc -c`).

Byte-compatibility (dispatch constraint): the M1 Footman row bytes are preserved as an exact prefix; the only change to that line is the appended `,12,false`.

## Flagged decisions — QA must rule on each

1. **New columns appended AFTER `Notes`** (last two columns; struct properties likewise appended after `Notes`). Rationale: keeps the M1 Footman row byte-prefix-stable per the dispatch constraint, and keeps struct declaration order matching CSV column order 1:1. UE DataTable CSV import maps by name, not position, so this is order-safe. The spec's row listings end at "DeckCount, bRanged" and never mention Notes — I read that as elision, not deletion: `Notes` is an FCardRow UPROPERTY, and removing its column would break the 1:1 header/struct mapping TASK-031 needs for a zero-warning reimport.
2. **Notes text for the five new rows** ("Ranged line unit", "Heavy melee tank", "Non-combat economy unit; +1 gold/s on arrival (GDD 3.3)", "Anti-unit tower", "Blocks unit pathing"). Notes is documented in CardRow.h as designer free text not used by gameplay; wording paraphrases GDD section 4's notes column, ASCII-only, comma-free (so no CSV quoting anywhere). No stat content lives in Notes. If QA prefers empty Notes cells for new rows, that is a 5-cell edit with no code impact.
3. **Bool cells are lowercase `true`/`false`** — mirrors the spec's own casing ("bRanged true"); UE bool text import is case-insensitive and also accepts 0/1.
4. **Cadence 0 on Miner and Wall rows** (spec-mandated; GDD shows em-dash). This deliberately overrides FCardRow's in-class `Cadence = 1.0f` default, which TASK-001 chose as a zero-interval guard for OMITTED cells — an explicit 0 cell wins over the default. Downstream contract note: any code that starts a looping timer directly from row Cadence must guard `<= 0` (FTimerManager treats rate <= 0 as clear-timer, so worst case is a silent no-op, not a spin). Affects TASK-025 (AMinerUnit — overrides the combat state machine entirely, never attacks) and TASK-027 (Wall is plain ABuilding with no attack; ATower reads ArrowTower's 1.5). Existing ASummonedUnit only ever receives combat-unit rows (Footman/Archer/Knight, all Cadence > 0).
5. **Zeros written as `0` (not `0.0`) in float columns** — matches the spec text literally ("Damage 0, Range 0, Cadence 0"); UE's numeric property import parses both identically.

## Downstream notes

- **TASK-022 (deck build):** DeckCount column is authoritative for the default 50-card deck; per-row values above give the section 3.4 copy counts (12/10/10/8/6/4 when sorted).
- **TASK-028 (ranged units):** `bRanged` is true only for Archer among Unit rows. ArrowTower's `true` is data-complete per spec; ATower (TASK-027) fires projectiles by class design and does not need to read it.
- **TASK-031 (reimport):** import `Docs/Data/cards.csv` into `/Game/Data/DT_Cards` with row struct FCardRow. Expect zero warnings: every header column after the row-name column maps 1:1 onto a UPROPERTY; no extra or missing columns; row names come out as the six CardIDs (PascalCase, no spaces: Footman, Archer, Knight, Miner, ArrowTower, Wall).
- **Not compiled** (standing gate — batch compile after round-2 sign-off, TASK-039). The header change is additive-only to a POD-style USTRUCT; no existing code references change meaning.
- TASKBOARD.md deliberately not edited (orchestrator owns board writes for this dispatch).
