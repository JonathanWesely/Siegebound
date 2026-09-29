# Handoff: TASK-1564 (gameplay-programmer) — marker `TASK-1564-VSBOT-RECIPE-PROVENANCE-FIX`

Date 2026-09-28. Status → `ready-for-qa`. Gate: `TASK-1565`. Host: the first commit host to derive after `TASK-1565` PASS (expected `TASK-1562`, per the row).

Text only. No editor, no compile, no PIE, no mutating git. RUNS declared (`SC-§71a`): `Read` / `Grep` / `Edit` / `Write` (this handoff), read-only `sha256sum`, `wc`, `od`, `tr`, and `git --no-optional-locks diff` / `status` / `log`. There is also one read-only Python reverse-derivation script, in my scratchpad and outside the repo. It opens both files `rb` and writes nothing into the repo.

Point of truth: `qa/TASK-1555.md` §5 items 1–3 and 5–7, and §3 N1 (the optional item (5)). Every site was found by its text. §5's line numbers still matched at my instant because the files were byte-equal to §6.

## Files touched

| file | before sha256 (= `qa/TASK-1555.md` §6) | after sha256 | bytes | lines | EOL |
|---|---|---|---|---|---|
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | `c695238df318ea27f9015d6070b168b346b9cb79325e1d7d1e3de553f5730983` | `0a63186421c3daf48af0d64b32e6ad17db6b45a239ca4864d041ee439c3ee2ea` | 33176 → 35521 | 248 → 248 | LF, CR 0, no BOM, trailing LF |
| `Tools/Verify/recipes/README.md` (the H3 line only) | `f5531a1f20956b233e8da6afcc55f61738996591770af05b168e64981e96cffb` | `4f0eaaf634553b05e36536f9d1de858ef292abd3cdc9823a322162c4052d465b` | 18463 → 18640 | 105 → 105 | LF, CR 0, no BOM, trailing LF |
| `.claude/pipeline/handoffs/TASK-1564-programmer.md` | new | (this file) | | | |
| `.claude/pipeline/TASKBOARD.md` | this row's `status:` line only | | | | |

Git index lines (read-only `git diff`): vsbot `ce2e413..f2c11a5`, README `f4f0ab9..1b14371`. The before-blobs equal §6's computed blobs `ce2e4138…` and `f4f0ab99…`. `git diff --stat`: 2 files, 12 insertions(+), 12 deletions(−). Git printed its usual `LF will be replaced by CRLF` staging notice. The working tree is LF.

**Reverse derivation (my own cut, measured).** On each changed line I either cut off the appended text or un-struck the old sentence and dropped the rest. vsbot → `c695238d…5730983`, 33176 B; README → `f5531a1f…e96cffb`, 18463 B. Both equal §6. The changed-line sets are vsbot {3, 10, 44, 57, 58, 59, 60, 192, 204, 229, 236} and README {105}. Neither :26 (the full-path `[M:]` tag) nor README :93 (the plugin-version list) is among them.

**Old-text form:** the two FALSE sentences are struck with `~~…~~` and kept in full: remove the first `~~` pair and the old line is a byte prefix of the new one. Every other changed line keeps its old text as a byte prefix. On the four table rows, the prefix is the old line minus its closing ` |` cell delimiter, the same form `qa/TASK-1555.md` §0.3 accepted for README :56. Nothing is deleted and nothing is re-verdicted.

## `git diff -U0` excerpt, each new or struck sentence beside its source line

### (1) FALSE, corrected: struck and scoped to Step 1 rows 1–4 (§5 items 1–2)

- **vsbot :10**, struck: `~~**Measured once:** one run, one summon.~~`
  - Added: "Corrected 2026-09-28 by `TASK-1564`: **One summon. The walk, the capture, the summon and every step and row except Step 1 rows 1–4 were measured once** (a2, 2026-09-26). Step 1 rows 1–4 (the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, the in-batch `record_burst {"seconds":1}`) have a second run, `1493` (2026-09-27), which re-verified them in-run **y** and did no walk, capture or summon. `[M: 1493 Recipes used]`"
  - Sources: `qa/TASK-1493-verify.md:95`: "**Step 1 rows 1–4 only** (the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, in-batch `record_burst {"seconds":1}`). **Re-verified in-run: y** for those rows." … "No walk, capture or summon was done." a2's date is from vsbot :14, "run **2026-09-26**". `1493`'s date is from the :3 addition below.
- **vsbot :192** (Fences), struck: `~~**One run (a2), one summon.** No second run exists, so other days, map states and bot behaviours are untested.~~`
  - Added: "Corrected 2026-09-28 by `TASK-1564`: **One run (a2, 2026-09-26) of the walk, the capture, the summon and every step and row except Step 1 rows 1–4; one summon.** No second run of them exists, so for them other days, map states and bot behaviours are untested. Step 1 rows 1–4 have a second run on another day, `1493` (2026-09-27), re-verified in-run **y** for those rows only; it did no walk, capture or summon. `[M: 1493 Recipes used]`"
  - Sources: the same as :10.
  - Scope: the fence is kept in full for the walk, the capture, the summon and everything else. Rows 1–4 are credited with "a second run on another day" only. No claim is made about other map states or bot behaviours for them.

### (2) Incomplete, added: `1493`'s values beside Step 1 rows 1–4, and the short name (§5 items 3 and 7)

- **vsbot :57** (row 1), cell gains: "· `1493` menu t=3.85 → `Button_0` `focused: true`, "Play (vs Bot)" `[M: 1493 Recipes used]`"
  - Source `1493:95`: "`Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85."
- **vsbot :58** (row 2), cell gains: "· `1493`: the travel to `L_Arena` landed; the game world read `UEDPIE_0_L_Arena` at t=6.02 `[M: 1493 Recipes used]`"
  - Source `1493:95`: "The travel to `L_Arena` landed (the game world read `UEDPIE_0_L_Arena` at t=6.02)".
  - t=6.02 is quoted bare. The report does not say which clock it is on, so I did not label it menu or arena.
- **vsbot :59** (row 3), cell gains: "· `1493`: run ("`wait 3`"), no value quoted `[M: 1493 Recipes used]`"
  - Source `1493:95` names `wait 3` among the rows used. It gives no value for it.
- **vsbot :60** (row 4), cell gains: "· `1493`: "the burst auto-started the arena film" `[M: 1493 Recipes used]`"
  - Source `1493:95`, verbatim.
- **vsbot :3** (short names), gains at the line's end: "Added 2026-09-28 by `TASK-1564`: `1493` = `.claude/pipeline/qa/TASK-1493-verify.md` (Step 1 rows 1–4 and the second H3 sighting only). That report states no run date; its own status flip on `TASKBOARD.md` (`#### TASK-1493`) does: "H1 HOLDS (2026-09-27, playtest-verifier; marker `TASK-1493-H1-HOLDS-2026-09-27`)"."
  - Source: `TASKBOARD.md` `#### TASK-1493` `- status:` line (at my instant `:8564`; `qa/TASK-1555.md` N1 cited `:8558`, and the board has grown since). It reads "**H1 HOLDS (2026-09-27, playtest-verifier; marker `TASK-1493-H1-HOLDS-2026-09-27`).**" This is spec (5), N1's first-party date source, applied only at a line this row already touches.
  - The short name's scope in parentheses follows the existing `1524` entry's "(Precondition 2 only)" form.
- **vsbot :26's `[M: qa/TASK-1493-verify.md Recipes used]` is untouched** (ruling 3). It is not in the changed-line set.

### (3) The second H3 sighting (§5 item 5), at four sites, each saying it stays `HYPOTHESIS`

The quote at all four sites comes from `1493:12` (the report's **Recording:** paragraph): "A second film, armed at `start_pie`, holds the menu only".
- **vsbot :44** (Precondition 5), gains: "A second sighting: in `1493`, "A second film, armed at `start_pie`, holds the menu only". `[M: 1493 Recording]` Two sightings and no control: H3 stays `HYPOTHESIS`."
- **vsbot :204** (Fences *Films*), gains: "A second sighting: `1493`'s "A second film, armed at `start_pie`, holds the menu only" `[M: 1493 Recording]`. Two sightings and no control: still `HYPOTHESIS`."
- **vsbot :236** (Known hazards *Films*), gains: "A second sighting of H3: in `1493`, "A second film, armed at `start_pie`, holds the menu only" `[M: 1493 Recording]`. Two sightings and no control: H3 stays `HYPOTHESIS`."
- **README :105** (the H3 line; the only README line written), gains: "A second sighting of H3: "A second film, armed at `start_pie`, holds the menu only" (`qa/TASK-1493-verify.md` *Recording*). Two sightings and no control: H3 stays `HYPOTHESIS`."
  - The README uses the full path, as :105 already does for `1512`.

### (4) Low priority, same file (§5 item 6)

- **vsbot :229** gains at the line's end: "Step 0 was also run by two later runs, each inside the menu recipe's Steps 0–4: `qa/TASK-1524-verify.md` (Steps 0–4, re-verified in-run "y, with a mismatch"; the mismatches were at Steps 1 and 4) and `qa/TASK-1519-verify.md` (Step 0 `Button_0` focused, t=6.70), each under its *Recipes used*."
  - Sources: `1524:55` "`RCP-menu-to-deckbuilder.md` · Steps 0–4 · re-verified in-run **y, with a mismatch**: Step 1's batched `IA_MenuDown` ×2 … landed **1 of 2** … Step 4 found `EditingDeckIndex = 2`". `1519:58` "Steps 0–4 · re-verified in-run **y**: Step 0 `Button_0` focused (t=6.70)".
  - `1524` quotes no Step 0 stamp (the menu recipe's own Precondition 4 says the same), so none is written.
  - `1512`'s Step 0 stays out, per §5 item 6: it was this recipe's in-batch row 1.
  - I used full paths because the line already does, and because vsbot's `1524` short name is scoped "(Precondition 2 only)".

### (5) Optional date source (§3 N1 / N2)

Applied at :3 only (above). N1's own sites (vsbot :26, README :56) and N2's (README :62) are **not** lines this row touches, so both stay as they are, per the spec's "only at lines this row already touches".

### Ruled NOT to change

README :93 (the plugin-version list) is unchanged. It is not in the changed-line set.

## For QA to scrutinise

1. **The section name `Recording` for `1493`.** In `1493` it is a bold paragraph label (`**Recording:**`, line 12), not a `##` heading. `1512`'s tags use `Recording` the same way.
2. **The "Corrected … by `TASK-1564`" / "Added … by `TASK-1564`" markers** at :3, :10 and :192. They follow README :62's `TASK-1554` precedent ("Corrected 2026-09-27 by `TASK-1554`:"). The other additions carry only their `[M:]` citations, the library's provenance form.
   - I did not add an "Amended by `TASK-1564`" line under `## Source` (the library's usual amendment record, vsbot :27–28): it would be a sentence outside the named sites, which (6) forbids. If the gate wants one, that is a manager call for a follow-up row.
3. **The added "(the mismatches were at Steps 1 and 4)" at :229.** Without it, quoting `1524`'s verdict "y, with a mismatch" could be read as a Step 0 mismatch. It is a lift of `1524:55`.
4. **Scope of "every step and row except Step 1 rows 1–4"** at :10 and :192. This mirrors :26's already-gated "Every other step and row has not been re-verified since; for them a2 is still the only run".

## Not examined / limitations

- **`1493` was read as written**, not against its frames, film or log. "Re-verified in-run: y" and the t=3.85 / t=6.02 values are the report's own. The recipe quotes them and does not extend them.
- **The run date 2026-09-27** rests on the verifier's status flip on `TASKBOARD.md`, not on the report. That is a first-party statement by the same agent, but it is not the report's own text. The report itself still states no run date, and :3 says so.
- **"Nothing else moved"** rests on the reverse derivation of both files to §6's hashes plus the `git diff -U0` above. `TASKBOARD.md` was not diffed: the manager and a parallel `TASK-1560` programmer edit it concurrently. My only write there is this row's `status:` line.
- **Markdown rendering was not viewed.** That includes `~~**…**~~` inside a blockquote list item and the longer table cells.
- **The other recipes, the agent file, `CONVENTIONS.md`, `CLAUDE.md`, `Source/` and `Content/` were not opened for write and not changed.** The diffstat names only the two recipe files.
- No recipe was run, and there was no PIE, compile or editor call.
