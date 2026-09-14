# Handoff — TASK-1247 [AURA-SHIP-PRESCREEN-2] — gameplay-programmer

Subject: `.claude/commands/ship.md` §2b only — ruling R4 (from `qa/TASK-1239-report.md` WARN-1 / WARN-2 / NIT-1).
Status set: `ready-for-qa`. Gate: `TASK-1252`. Host: `TASK-1253`.
No compile, no git write, no editor. `Tools/Packaging/ship.ps1` was NOT opened.

## What changed (three edits, two hunks)

| # | Site | Before | After |
|---|---|---|---|
| 1 | §2b.1 step 3 (line 132) | `` (`SHIP-§8b` rule 9 rejects any other) `` | `` (`SHIP-§8b(1)` rejects any other) `` — gloss kept |
| 2 | §2b.1a bullet 1 (line 149) | `and attach to the suspension record, as **ADDITIONAL evidence for Jonathan**:` | `and attach beside the suspension — in the `/ship` handoff's `C3` section — ⛔ never inside `ship-state.json` or `ship-adjudication.json` (`SHIP-§8d` cl. 4), as **ADDITIONAL evidence for Jonathan**:` (QA WARN-2 text, 150 chars, character-exact) |
| 2 | §2b.1a bullet 1 (line 149, end) | *(nothing)* | appended QA WARN-1's sentence VERBATIM: `— PIE renders the editor's uncooked content … Until then it is not run, and a PIE run is ⛔ not admissible as `C3` evidence.` (447 chars, character-exact vs the report — `SC-§38a`) |
| 3 | §2b.1a bullet 2 (line 150) | `` `SHIP-§8b` rule 9 (only the arena capture is the pending capture; … `` | `` `SHIP-§8b(1)` (only the arena capture is the pending capture; … `` — gloss kept |

Referents checked before citing: CONVENTIONS `SHIP-§8b` rule 1 (line 7279, "IT BINDS TO THE EXACT CAPTURE AND THE EXACT BUILD") and `SHIP-§8d` cl. 4 (line 7300, "NO HAND-WRITTEN `ship-state.json` OR ADJUDICATION RECORD") both exist and say what the citations claim.

QA's BLOCKER remedy ("PIE on the same commit, labelled NOT the shipped bytes") was NOT adopted, per R4 — no PIE step was added anywhere.

## Acceptance — measured (`git`, run from the project dir; `SC-§71b`)

1. **Hunk count: 2**, both inside §2b (§2b spans lines 114–226; `2b.1a` = 147–153, `2b.2` starts 154). `git diff -U0 HEAD -- .claude/commands/ship.md | grep '^@@'` → `@@ -132 +132 @@` and `@@ -149,2 +149,2 @@`. `git diff --stat` → `1 file changed, 3 insertions(+), 3 deletions(-)`. No line added or removed; three lines rewritten.
2. **`grep -c 'rule 9' .claude/commands/ship.md` → `0`.** `SHIP-§8b(1)` now appears 2× in the file (the two corrected sites), 1× inside 2b.1a.
3. **`git diff --stat HEAD -- Tools/Packaging/ship.ps1` → empty**; `git status --porcelain -- Tools/Packaging/ship.ps1` → empty. Its two "rule 9" strings (`:2691`, `:2846`) remain `TASK-1198`'s rider, untouched.
4. **Sentence (1) character-exact**: the WARN-1 text extracted from the report between `append to bullet 1 *"` and `"*` (447 chars) is a substring of the new `ship.md` → `True`. Same test for the WARN-2 "attach beside…" text (150 chars) → `True`.
5. **"optional" and "does not adjudicate" still present**: `This step is optional` ×1 (line 149), `It does not adjudicate` ×1 (line 150). Heading at 147 still reads `OPTIONAL`.
6. Extra: **SHIP RESULT grammar line 36 byte-identical to HEAD** (`diff` of `sed -n 36p` both sides → identical). Line-ending census unchanged (0 CRLF lines at HEAD, 0 now; the `LF will be replaced by CRLF` warning is the repo's autocrlf notice, not a change).
7. "Beside the suspension" now names BOTH JSON files, as R4 requires: `ship-state.json` and `ship-adjudication.json` both appear inside 2b.1a.

## For QA (`TASK-1252`) to scrutinize

- **Placement of the appended sentence.** QA's WARN-1 said "append to bullet 1"; the sentence begins with an em-dash, so I appended it at the very END of bullet 1, after the closing parenthesis and period of "(… ⛔ not a summary)." It therefore reads `…not a summary). — PIE renders…`. Verbatim was the law (`SC-§38a`), so the leading dash was kept as written rather than smoothed. If the gate would rather have it spliced directly after "standing in for the stage" inside the parenthetical, that is a one-move edit, but it would no longer be "appended".
- The gloss after each `SHIP-§8b(1)` was left as it was (spec: "the parenthetical gloss after each stays").
- Nothing outside lines 132 and 149–150 moved; the inbound cross-references "step 3 below" (line 204) and "§2b.4" (line 266) still resolve.
- `git status` also shows other wave-1/wave-2 lanes' files dirty (board, SLACK.md, other handoffs) — not this row's; this row's write set is `ship.md` + this handoff + its own `- status:` line.
