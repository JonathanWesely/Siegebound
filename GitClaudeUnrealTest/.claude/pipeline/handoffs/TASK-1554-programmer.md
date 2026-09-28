# TASK-1554 — programmer handoff: [VSBOT-RECIPE-REVERIFIED-RECORD]

- **Row:** `TASK-1554` (marker `TASK-1554-VSBOT-RECIPE-REVERIFIED-RECORD`), 2026-09-27, gameplay-programmer. **Gate:** `TASK-1555`. **Host:** `TASK-1550` (docs).
- **Source of the fact:** `.claude/pipeline/qa/TASK-1493-verify.md` *Recipes used* (line 95). Boarded from `qa/TASK-1549.md` §5 item 1 (lines 92–96).
- **Text only.** Two files written under `Tools/Verify/recipes/`, three lines changed (README `:56`, `:62`; vsbot `:26`). No `Source/`, `Content/`, `CONVENTIONS.md`, agent file or `CLAUDE.md` byte. No compile, no PIE, no git write. Read-only git with `--no-optional-locks`, declared (`SC-§71a`). No `playtest-verifier` was live (dispatch prompt).
- **Method: strike, then append.** At each site the stale text is struck with `~~…~~` and kept, never deleted (spec (2)). The dated record follows it on the same line. Commit E (`ea2b791`) deleted the same parenthetical from the set-active entry when it recorded `TASK-1524`. I did not follow that precedent, because this row forbids deletion.
- **Scope of every new record:** `TASK-1493`'s Step 1 rows 1–4 and nothing more. Each record also says in words that the rest of the recipe is not re-verified. No record is re-verdicted.

## §1 — Start state, re-measured before editing (`Get-FileHash`, twice: at arrival and again just before the first `Edit`)

| file | sha256 at start | bytes | LF / CR | anchor it matches |
|---|---|---|---|---|
| `Tools/Verify/recipes/README.md` | `89347e54082573dd4b5ea84ed18027d4ecee0cf1d275a008e2ded3e2a56d7bd7` | 17367 | 105 / 0 | `qa/TASK-1549-loop1.md:88` (line 1 `PASS`); also `qa/TASK-1549.md:113` |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | `e1ae86a6acb7cf69843ed60165f47cbb571a86eb919cbb8071274f5f638d8f13` | 32433 | 248 / 0 | `qa/TASK-1549-loop1.md:89`; also `qa/TASK-1549.md:114` |

Both matched at both reads. The pre-edit git blob ids that `git diff` printed (README `a99b58f`, vsbot `a1f647d`) match `qa/TASK-1549.md` §6's computed blobs `a99b58f9…` and `a1f647de…`.

## §2 — After state and reverse derivation

| file | sha256 after | bytes | LF / CR | BOM | reverse-derived sha256 (each new string swapped back to its old string) | = start? |
|---|---|---|---|---|---|---|
| `README.md` | `f5531a1f20956b233e8da6afcc55f61738996591770af05b168e64981e96cffb` | 18463 | 105 / 0 | no | `89347e54082573dd4b5ea84ed18027d4ecee0cf1d275a008e2ded3e2a56d7bd7` | **yes** |
| `RCP-vsbot-capture-center-and-summon.md` | `c695238df318ea27f9015d6070b168b346b9cb79325e1d7d1e3de553f5730983` | 33176 | 248 / 0 | no | `e1ae86a6acb7cf69843ed60165f47cbb571a86eb919cbb8071274f5f638d8f13` | **yes** |

- Each new string occurs exactly once in its file. The line counts did not change (105, 248), so no line was added or removed.
- The script (Python `hashlib` over `rb` bytes) is in my scratchpad, not the repo. A gate can repeat it: replace each `+` line below with its `-` line and hash.

## §3 — `git diff -U0` excerpts (read-only, `--no-optional-locks`)

**vsbot: `git --no-optional-locks diff -U0` against `HEAD`** (the file was clean against `HEAD` before this row: `index a1f647d..ce2e413`):

```diff
@@ -26 +26 @@
-- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26. Not re-verified since; a2 is the only run.
+- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26. ~~Not re-verified since; a2 is the only run.~~ **Used in part 2026-09-27 by `TASK-1493`**, re-verified in-run **y** for **Step 1 rows 1–4 only** ("the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, in-batch `record_burst {"seconds":1}`"): `Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85; the travel to `L_Arena` landed (the game world read `UEDPIE_0_L_Arena` at t=6.02); the burst auto-started the arena film. "No walk, capture or summon was done." `[M: qa/TASK-1493-verify.md Recipes used]` Every other step and row has not been re-verified since; for them a2 is still the only run. The report states no run date: 2026-09-27 is the date of the marker it ran after, `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` (its line 10). Recorded by `TASK-1554`, 2026-09-27.
```

**README: `git --no-optional-locks diff --no-index -U0 <start bytes> <now>`.** The file also carries `TASK-1548`'s uncommitted cargo against `HEAD`, so a `HEAD` diff would mix that in. The left side is the reverse-derived copy, whose sha256 is the start anchor `89347e54…` (`index a99b58f..f4f0ab9`):

```diff
@@ -56 +56 @@
-| [`RCP-vsbot-capture-center-and-summon.md`](…) | … | `qa/TASK-1512-verify.md` A1–A3, ctl; *Attempt 1*; *Recording*; *Recipe candidates* | 2026-09-26 (source run; not re-verified since) |
+| [`RCP-vsbot-capture-center-and-summon.md`](…) | … | `qa/TASK-1512-verify.md` A1–A3, ctl; *Attempt 1*; *Recording*; *Recipe candidates* | 2026-09-26 (source run; ~~not re-verified since~~). **Used in part 2026-09-27 by `TASK-1493`**, re-verified in-run **y** for Step 1 rows 1–4 only (the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, the in-batch `record_burst {"seconds":1}`): `Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85; the travel to `L_Arena` landed; the burst auto-started the arena film. "No walk, capture or summon was done" (`qa/TASK-1493-verify.md` *Recipes used*). Every other step and row has not been re-verified since the source run. The report states no run date: 2026-09-27 is the date of the marker it ran after, `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` (its line 10). `TASK-1554` recorded it here and in the recipe. |
@@ -62 +62 @@
-- the other two have not been re-run since they were seeded.
+- ~~the other two have not been re-run since they were seeded.~~ Corrected 2026-09-27 by `TASK-1554`: `RCP-vsbot-capture-center-and-summon.md` was used in part by `TASK-1493` on 2026-09-27 and re-verified in-run for Step 1 rows 1–4 only; the rest of it has not been re-verified since it was seeded (see the table). `RCP-deckbuilder-slot-and-card-edit.md` alone has not been re-run since it was seeded: no report's *Recipes used* section in `qa/` names it, as of 2026-09-27.
```

The `…` in the README excerpt stands for the unchanged link target and description columns, elided here for width only. The tool output carried them in full, and they are identical on both sides.

## §4 — Each new sentence beside its source line

`1493` = `.claude/pipeline/qa/TASK-1493-verify.md`. "Mine" = my own measurement this row, labelled as such.

| new text (all three sites unless noted) | source |
|---|---|
| strike of "not re-verified since" / "the other two have not been re-run…" / "Not re-verified since; a2 is the only run." | `1493:95`: "`RCP-vsbot-capture-center-and-summon.md`: **Step 1 rows 1–4 only** … **Re-verified in-run: y** for those rows." |
| "Used in part" (sites 1, 3) / "used in part" (site 2) | `1493:95` "Step 1 rows 1–4 only"; the play recipe's "Used in part … by `TASK-1512`" is the table's precedent wording (`README.md:54`). |
| "re-verified in-run **y** for Step 1 rows 1–4 only" | `1493:95` "**Re-verified in-run: y** for those rows". |
| "(the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, [the] in-batch `record_burst {"seconds":1}`)" | `1493:95`, verbatim (site 1 adds "the" before "in-batch"; site 3 quotes it exactly). |
| "`Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85" | `1493:95` "`Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85." |
| "the travel to `L_Arena` landed" · site 3 adds "(the game world read `UEDPIE_0_L_Arena` at t=6.02)" | `1493:95` "The travel to `L_Arena` landed (the game world read `UEDPIE_0_L_Arena` at t=6.02)". |
| "the burst auto-started the arena film" | `1493:95` "the burst auto-started the arena film"; also `1493:12` "auto-started by the in-batch `record_burst` after the level travel". |
| ""No walk, capture or summon was done"" | `1493:95`, verbatim. |
| "Every other step and row has not been re-verified since [the source run]" · site 2: "the rest of it has not been re-verified since it was seeded" | `1493:95` limits its "y" to "those rows" and says "No walk, capture or summon was done". **Mine:** among `qa/*-verify.md`, the vsbot filename appears only at `1512:70` (the source run's own *Recipe candidates*) and `1493:95`. No other report records a use. |
| "for them a2 is still the only run" (site 3) | the struck original's own claim ("a2 is the only run"), narrowed to the rows `1493` did not run; `1493:95` "No walk, capture or summon was done" + the census in the row above. |
| "The report states no run date: 2026-09-27 is the date of the marker it ran after, `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` (its line 10)" | `1493:10`: "with `TASK-1491` and `TASK-1404` recorded as done first and `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` recorded, so I ran it." **Mine:** the only date strings in `1493` are that marker, at `:10` and `:30` (grep `2026-09-2\d`). The marker gives a lower bound only. The upper bound is **mine**: the report's file mtime is 2026-09-27 11:50:48, and today is 2026-09-27. The row's own title also says "on 2026-09-27" (`TASKBOARD.md` `#### TASK-1554`). |
| "`[M: qa/TASK-1493-verify.md Recipes used]`" (site 3) | the `[M: <run> <section>]` tag (`README.md` provenance table). The recipe's short-name line (`:3`) has no `1493` entry, and adding one would be another sentence, so the tag uses the full path. |
| "Corrected 2026-09-27 by `TASK-1554`" / "`TASK-1554` recorded it here and in the recipe" / "Recorded by `TASK-1554`, 2026-09-27" | this row. |
| site 2: "`RCP-deckbuilder-slot-and-card-edit.md` alone has not been re-run since it was seeded: no report's *Recipes used* section in `qa/` names it, as of 2026-09-27" | **Mine, the census in §5.** It agrees with the slot recipe's own `:14` "Not re-verified since." and README `:53` "(source run; not re-verified since)", both untouched. |

## §5 — The README "other two" check at source (spec (2), last sentence)

Question: has any report since seeding re-run `RCP-deckbuilder-slot-and-card-edit.md`? **No.**

- **Every *Recipes used* section in `qa/` (grep `Recipes used`, then each section read):**
  - `TASK-1511-verify.md:60–61`: menu.
  - `TASK-1512-verify.md:65–67`: play (in part) + menu Step 0.
  - `TASK-1524-verify.md:54–56`: menu + set-active.
  - `TASK-1519-verify.md:57–59`: menu + set-active.
  - `TASK-1499-verify.md:175–177`: none.
  - `TASK-1404-verify.md:60–61`: none.
  - `TASK-1493-verify.md:93–96`: vsbot Step 1 rows 1–4; menu "not used".
- **Every `qa/*-verify.md` line naming the slot recipe file (grep `RCP-deckbuilder-slot-and-card-edit`):** only `TASK-1493-verify.md:64`. That line is in *Blast radius*: a suggestion that the manager look at the recipe ("Worth a manager look at …"), not a use of it.
- **Verify reports written since 2026-09-25 (by mtime):** `1413`, `1421`, `1427`, `1436`, `1489`, `1491`, `1494`, `1498`, `PLAYTEST-archer50` (the slot recipe's own source run), `1511`, `1512`, `1524`, `1519`, `1499`, `1404`, `1493`. Those after archer50 are the seven above.
- ⇒ The sentence now says the slot recipe **alone** is un-re-run. The spec's "cite it if one did" branch did not fire.

## §6 — Found stale or incomplete, NAMED, NOT FIXED (spec (3) forbids any other sentence)

1. **vsbot `:10`** "**Measured once:** one run, one summon." The summon half still holds. "One run" is now false for Step 1 rows 1–4, which `1493` ran.
2. **vsbot `:192`** (Fences) "**One run (a2), one summon.** No second run exists, so other days, map states and bot behaviours are untested." "No second run exists" is false for rows 1–4 since `1493`, which ran on a different day. The fence's conclusion, that the walk, capture and summon are untested beyond a2, still holds.
3. **vsbot Step 1 table, rows 1–4 (`:57–60`), source column:** only a2's values are cited. `1493`'s values (menu t=3.85; `UEDPIE_0_L_Arena` at t=6.02) are not beside them. Incomplete, not false.
4. **README `:93`** (common hazards) "Plugin version was not read in any source run: … not in `1511`, `1512`, `1524` or `1519`". `1493` also used a recipe and also did not read the version (`1493:90`). The list omits it. Incomplete, not false.
5. **A second sighting of H3 nobody has recorded:** `1493:12` "A second film, armed at `start_pie`, holds the menu only". That bears on vsbot Precondition 5 / Fences *Films* (H3) and README `:105`. Nothing there is stale, but it is outside Step 1 rows 1–4, so I did not record it.
6. **vsbot `:229`** "That step was measured on its own (archer50 §1(a), t=4.84) and again in-run by `1511` (t=4.89)." `1519` (t=6.70) and `1524` also ran menu Step 0. Incomplete, not false. Low priority, and not about this recipe's own record.

Items 1 and 2 are the closest to this row's defect. A follow-up row could fix them the same way (strike + dated record).

## §7 — Files touched

- `Tools/Verify/recipes/README.md`: lines 56, 62.
- `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md`: line 26.
- `.claude/pipeline/handoffs/TASK-1554-programmer.md` (this file).
- `.claude/pipeline/TASKBOARD.md`: `TASK-1554`'s `status:` line only.
- Assets referenced: none. `CLAUDE.md`: not authored, not staged.

## For QA (`TASK-1555`) — what to scrutinise

- **(1) scope:** every new record says "rows 1–4 only" and names the rest as not re-verified. Check that no clause claims the walk, capture or summon, or any row past 4.
- **(1) the date:** it rests on the marker (lower bound, `1493:10`) plus the file mtime (upper bound, mine). The in-file text labels it "The report states no run date". Rule on whether that label is enough.
- **(2) strike vs byte prefix:** struck, never deleted. The reverse derivation in §2 swaps each whole new line back to the old line. Because the struck text sits inside the new line, the old text is recoverable by removing the `~~` markers and the appended record.
- **(3):** the README reverse-derives to `89347e54…` (`qa/TASK-1549-loop1.md:88`), and the vsbot recipe to `e1ae86a6…` (`qa/TASK-1549.md` §6, `:114`).
- **(4):** §5's census.

## Not examined / limitations

- **No PIE, no recipe run.** `1493` was read as it stands. I did not check it against its frames, log or film.
- **`1493` names the four actions but does not quote its action objects.** That they equal the recipe's rows 1–4 JSON is the report's own claim ("Re-verified in-run: y"). I did not check it.
- **The run date is derived, not stated** (see §4). The upper bound is a file mtime, which a later rewrite of the report would move.
- **The census covers `qa/` only**, by grep plus reading each *Recipes used* section. Handoffs, Slack and `Saved/AuraVerify/` were not censused for runs. The verifier writes its runs only to `qa/*-verify.md`.
- **Markdown rendering not viewed.** `~~…~~` inside a GFM table cell and inside a list item should render as strikethrough; not checked by eye.
- **`TASKBOARD.md` not diffed.** The manager edits it concurrently. I re-read my row before writing, changed only my `status:` line through a unique anchor, and grepped it back.
- **Git printed "LF will be replaced by CRLF the next time Git touches it"** for both files. The working tree is LF (CR 0) before and after, the same as at the start anchors. The warning is git's line-ending note for staging (build-master's lane), not a change I made.
- Read-only git only: `rev-parse --show-toplevel`, `log -S`, `show ea2b791`, `diff -U0`, `diff --no-index -U0`, all with `--no-optional-locks`. The git root is one level up (`C:/GitProjects/GitHub/GitClaudeUnrealTesting`).
