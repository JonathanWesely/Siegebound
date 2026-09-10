# TASK-1190 — handoff (gameplay-programmer, 2026-09-09)

**Deliverable:** the census table is **`.claude/pipeline/handoffs/TASK-1190-census.md`** (the path the dispatch named). This note is the handoff the role requires; the census file is the only permitted number source for `TASK-1191`.

⚠️ **Path note for the orchestrator / `TASK-1191`:** the board row's `names:` block says the census lives in `handoffs/TASK-1190-programmer.md`; the dispatch prompt said `handoffs/TASK-1190-census.md`. I followed the dispatch. `TASK-1191`'s `§ Addendum (TASK-1191)` should be appended to **the census file**, so the gate has one table.

## What was done
- Read-only census: text reads of `Source/**`, `Config/**`, `Docs/**`, `Plugins/SiegeLlama/**`, engine source, plus a binary tagged-property readback (scratchpad Python, no engine process) of `Content/Data/DT_Cards.uasset` (all 34 rows), `Content/Input/IMC_Hero.uasset` (28 mappings), `Content/Data/DA_BattlefieldScatter.uasset` (7 layers), and name/label scans of the menu/HUD widgets and `L_Arena.umap`.
- No editor, no MCP, no compile, no Git write, no `Source/**`/`Content/**`/`Config/**` edit.

## cl. 1 verdict
`cards.csv` and `DT_Cards.uasset` were both last committed in `1a457df` (2026-09-04 21:03:28 −0700), both clean; the asset's 34 rows × 31 columns read back equal to the CSV at the value level (enum namespace spelling aside), `DeckCount` sums to 50, `NoticeRange` is 0.0 on every row. **PROCEED.** The asset's embedded import record (MD5 `ef265e94…`, ≈ 2026-07-22) is stale and was not used as evidence.

## Files touched
- `.claude/pipeline/handoffs/TASK-1190-census.md` (new)
- `.claude/pipeline/handoffs/TASK-1190-programmer.md` (this note)
- `.claude/pipeline/TASKBOARD.md` — my `- status:` line only

## What QA / the README author should scrutinise
- `ship.ps1` is being modified by the live ship; the census cites the `C4-NO-MODELS` gate at both HEAD and working-tree lines.
- The cloud-sync "REACHABLE if staged" verdict rests on UAT's `StageConfigFiles` (stages every `Config/*.ini` unless a `[Staging]` deny list exists, and `DefaultGame.ini` has none) — the package itself was not inspected.
- The bot's Fireball rule is inert with the two shipped curated decks (neither contains Fireball) — a finding, not on any prior list.
- Every `NOT MEASURED` item is in census §11 and may not be typed into the README as a number.
