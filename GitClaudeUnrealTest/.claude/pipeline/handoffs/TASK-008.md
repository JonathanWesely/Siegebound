# TASK-008 Handoff — Import DT_Cards data table (editor)

- author: gameplay-programmer
- date: 2026-07-02
- status: import complete, asset saved (no Git per task constraints)

## Import complete

- Asset: `/Game/Data/DT_Cards` (on disk: `Content/Data/DT_Cards.uasset`, saved, not dirty)
- Source: `Docs/Data/cards.csv`, imported via the editor CSVImportFactory with row struct `/Script/GitClaudeUnrealTest.CardRow` (FCardRow)
- Importer log: `LogCSVImportFactory: Imported DataTable 'DT_Cards' - 0 Problems` — zero warnings, zero errors; no other log entries reference the asset or the CSV.
- Rows: exactly one row, name `Footman`.

## Verified row values (Footman), field-by-field

| Property | Table value | GDD §4 spec | Match |
|---|---|---|---|
| DisplayName | Footman | Footman | yes |
| CardType | Unit | Unit | yes |
| Cost | 3 | 3 | yes |
| MaxCopies | 12 | 12 | yes |
| HP | 80 | 80 | yes |
| Damage | 12 | 12 | yes |
| Range | 120 | 120 | yes |
| Cadence | 1.0 | 1.0 | yes |
| Speed | 400 | 400 | yes |
| Profile | Standard | Standard | yes |
| Notes | Basic melee line unit | (informational) | yes |

## Import-source binding (§3.0 re-import path)

Asset registry tags on `/Game/Data/DT_Cards`:

- `AssetImportData.RelativeFilename` = `../../Docs/Data/cards.csv` (relative to `Content/`, i.e. the project's `Docs/Data/cards.csv`) with stored timestamp + MD5 — future balance edits are plain CSV re-imports via Reimport.
- `RowStructure` = `/Script/GitClaudeUnrealTest.CardRow`

## Runtime path check

`load_asset` on the exact soft object path `/Game/Data/DT_Cards.DT_Cards` resolves successfully — this is the path `ASummonedUnit` (TASK-004) and `ASiegePlayerController` (TASK-007) load at runtime.

## Scope touched

Only `/Game/Data/` (folder created by the import) and this handoff file. No other editor state, code, or config modified.

## For QA to scrutinize

- Enum cells were authored as short names (`Unit`, `Standard`) and resolved without warnings against ECardType/ECardProfile — the get_rows readback shows the resolved enum display values, matching spec.
- The MCP readback camelCases property names (`hP`, `maxCopies`) — that is the tool's JSON convention, not the actual UPROPERTY names; the schema struct is FCardRow verbatim.
