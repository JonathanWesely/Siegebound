# QA Report — TASK-1252 — Verdict: PASS
subject: TASK-1247 (`.claude/commands/ship.md` §2b — ruling R4: the WARN-1 availability sentence, "beside the suspension" defined, `SHIP-§8b` "rule 9" → `§8b(1)` at both sites)
Date: 2026-09-13
Reviewer: qa-reviewer
Counts: 0 BLOCKER · 0 WARN · 1 NIT
Law: R4 · `SHIP-§8b(1)` · `SHIP-§8b(7)` · `SHIP-§8d` cl. 4 · `SHIP-§9` · `SC-§38a` · `SC-§71b` · `SC-§106`

## What I inspected, and how (provenance — `SC-§71b`)

- `Read` of `.claude/commands/ship.md` in full (332 lines — the same count `qa/TASK-1239` recorded, consistent with the handoff's "no line added or removed, three lines rewritten").
- `Read` of `handoffs/TASK-1247-programmer.md`, `qa/TASK-1239-report.md` (WARN-1 line 30, WARN-2 line 31, the ruling), the TASK-1247 + TASK-1252 rows and ruling R4 on the board.
- `Grep` (ripgrep) for the exactness checks below — the full WARN-1 sentence and the full WARN-2 phrase were each run as ONE escaped regex against `.claude/` so that a single-character deviation would have returned only the report, not both files.
- No `Bash`, no `git`. Hunk confinement vs HEAD and `ship.ps1` untouched are the handoff's git measurements, ACCEPTED AS DECLARED and consistent with what the text shows; the host `TASK-1253` re-measures both (`git diff --stat HEAD~1 -- Tools/Packaging/ship.ps1`, `grep -c 'rule 9'`). `unreal_inspector` not used — nothing here is an asset, graph or log.

## Acceptance — TASK-1252's five items (plus the row's BLOCKER test)

| # | Item | What I verified | Result |
|---|---|---|---|
| 1 | Appended sentence character-exact vs WARN-1 (`SC-§38a`) | One regex for the whole 447-char sentence (`— PIE renders the editor's uncooked content … ⛔ not admissible as `C3` evidence.`) matched exactly two lines in `.claude/`: `qa/TASK-1239-report.md:30` (the source) and `ship.md:149` (the copy). Ends bullet 1 of §2b.1a. | ✅ MEASURED (text) |
| 2 | `rule 9` = 0; both sites `SHIP-§8b(1)` | `Grep 'rule 9'` on `ship.md` → **0 occurrences**. `SHIP-§8b(1)` → exactly **2** hits, `:132` (§2b.1 step 3, gloss "rejects any other" kept) and `:150` (§2b.1a bullet 2, gloss "only the arena capture is the pending capture; a verifier screenshot is never bound as `capturePath`" kept). Referent checked: CONVENTIONS `SHIP-§8b` rule 1 at `:7279` = "IT BINDS TO THE EXACT CAPTURE AND THE EXACT BUILD … any mismatch is a STOP" — the citation says what the gloss claims. | ✅ MEASURED (text) |
| 3 | "beside the suspension" names both JSON files + `SHIP-§8d` cl. 4 | `ship.md:149`: `attach beside the suspension — in the `/ship` handoff's `C3` section — ⛔ never inside `ship-state.json` or `ship-adjudication.json` (`SHIP-§8d` cl. 4)` — character-exact vs `qa/TASK-1239-report.md:31` (one regex, both files matched). Referent checked: CONVENTIONS `:7300` cl. 4 = "NO HAND-WRITTEN `ship-state.json` OR ADJUDICATION RECORD". | ✅ MEASURED (text) |
| 4 | Hunks inside §2b only; SHIP RESULT grammar unchanged; "optional" + "does not adjudicate" present | Handoff declares 2 hunks `@@ -132 +132 @@` and `@@ -149,2 +149,2 @@`, 3+/3−; §2b spans 114–226, so both sit inside it. Text-level: every line I compared against my TASK-1239 reading outside 132/149–150 reads unchanged; line 36 still reads `SHIP RESULT: PASS \| STOP at <GATE-ID> - <reason> \| ADJUDICATE C3 - <capture path> \| DRYRUN-OK \| DRYRUN-WOULD-STOP` (five tokens, matching the 41–45 table); heading `:147` `OPTIONAL`, `:149` "**This step is optional.**", `:150` "⛔ **It does not adjudicate.**"; inbound refs "step 3 below" (`:204`) and "§2b.4" (`:266`) still resolve. Byte-identity to HEAD is the handoff's `sed -n 36p` diff, not mine. | ✅ text-level; hunks as declared |
| 5 | `ship.ps1` untouched | Declared (`git diff --stat HEAD -- Tools/Packaging/ship.ps1` empty; `git status --porcelain` empty). Consistent on read: its two "rule 9" strings are still at `:2691` (comment) and `:2846` (the printed `Say` line) — exactly the two R4 assigned to `TASK-1198`'s rider. Host re-measures. | ✅ as declared, consistent |
| — | No PIE-on-same-commit step added (R4 REFUSED it) | `Grep 'PIE\|Play-In-Editor'` on `ship.md` → 3 hits, ALL on line 149, ALL inside the verbatim WARN-1 sentence, whose only PIE content is the ban ("a PIE run is ⛔ not admissible as `C3` evidence"). No step, bullet or table row runs PIE anywhere in the file. | ✅ MEASURED (text) |
| — | The row's BLOCKER test (`SHIP-§8b(7)` — wording that lets the pre-screen stand in for the human verdict) | Unchanged fences all present: `:150` "It does not adjudicate … written by the adjudicator against the run's own pending capture … his eye wins (`SHIP-§8b(7)`) … no fourth verdict, no `PASS` path that an agent walks alone"; `:152` skipped ⇒ identity; `:144` Jonathan discharges it. The new text only NARROWS the step (availability condition + an inadmissibility clause) — it adds no path. | ✅ none found |

## Findings

- **[NIT] `ship.md:149` — the appended sentence opens with an em-dash, so bullet 1 ends `…⛔ not a summary). — PIE renders…` — a full stop followed by a spaced em-dash starting a new clause.** Ruling: **acceptable verbatim append, not a defect.** The spec said "append … VERBATIM from *"— PIE renders…"*" and the acceptance made it character-exact (`SC-§38a`); the source sentence (mine, `qa/TASK-1239:30`) carried the leading dash, so the programmer did exactly what was ruled and smoothing it would have failed acceptance (4). The seam is cosmetic — the file already joins clauses with spaced em-dashes throughout, the sentence is unambiguous, and nothing procedural depends on the punctuation. No action under this row. If anyone wants `). — PIE` read as `). PIE` or the sentence spliced inside the parenthetical, that is a wording change to my own source sentence and is the manager's to rule (`SC-§100`), then a one-move programmer edit on a later `ship.md` row — ⛔ not the host's to "tidy" at commit.

## Notes for build-master / host (`TASK-1253`)

- PASS: 0 BLOCKER, 0 WARN, 1 NIT (no action). Commit-eligible as-is; `ship.md` rides your commit with the other three rows' files.
- Re-measure at commit, per `SC-§71b`: `grep -c 'rule 9' .claude/commands/ship.md` = 0 · `git diff --stat HEAD~1 -- Tools/Packaging/ship.ps1` empty · the two hunks both inside lines 114–226. Expect `ship.ps1:2691` / `:2846` to still say "rule 9" — that is `TASK-1198`'s rider (R4), not a defect of this commit.
- Carry-forward, unchanged from `qa/TASK-1239`: the pre-screen is DORMANT until `handoffs/AURA-MCP-CENSUS.md` names an exe-launch + observe tool; the text now says so in as many words, which was the point of WARN-1.
