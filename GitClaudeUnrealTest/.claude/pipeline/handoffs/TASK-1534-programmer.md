# TASK-1534 — handoff (gameplay-programmer) — VERIFIER-BODY-1519-FOLLOWTHROUGH

Marker `TASK-1534-VERIFIER-BODY-1519-FOLLOWTHROUGH`. Gate: `TASK-1535`. Host: `TASK-1536` (docs only). Routing: no 5a/5b.

## Files touched
- `.claude/agents/playtest-verifier.md`: BODY only. Every hunk is at line 48 or below; the closing `---` is line 7.
- `.claude/pipeline/TASKBOARD.md`: this row's `status:` line only (backlog → in-progress → ready-for-qa; each write re-read first, anchored uniquely, and grepped back).
- This handoff.
- NOT touched: `CLAUDE.md` (never authored, never staged), `CONVENTIONS.md` (read only; its working-tree `M` predates me and is the manager's `TASK-1532`), anything under `Tools/` (`TASK-1533`'s lane), the frontmatter.
- No `playtest-verifier` was live during the edit (per the dispatch).

## Start state (`SC-§71a`, marker `SC-71A-READ-ONLY-GIT-START-STATE`)
Read-only git only, every call `--no-optional-locks`: `status --short`, `log --oneline`, `log --diff-filter=A`, `show <rev>:./<path>`, `rev-parse --short HEAD`, `ls-files`, `diff --numstat`, `diff -U0`. Nothing mutating.
- HEAD `ea2b791`. `.claude/agents/playtest-verifier.md` was clean against HEAD (absent from `status --short`). Last commit touching it: `cce9f6b`.
- Working-copy whole-file sha256 at start: `1dffc539d7ff16bd0a4092fb4ce39d767209ef5c54275390500ff27b2d843558` (= the dispatch's figure; the file had not moved). 19794 bytes, 216 CR / 216 LF / 216 CRLF.
- Other dirt at start (not mine): `M CONVENTIONS.md`, `M TASKBOARD.md`, `?? handoffs/TASK-1530-buildmaster.md`, `?? qa/TASK-1519-verify.md`.
- One read errored and was re-run: `git show cce9f6b:.claude/agents/…` fails because the git root is one level up (`SC-§102`); `show <rev>:./<path>` is the working form. The errored call printed the empty-input sha `e3b0c442…`, which is not a measurement.
- `RCP-deckbuilder-set-active-by-keyboard.md` was ADDED in `cce9f6b` (`log --diff-filter=A`), which confirms item (2)'s premise.

## Frontmatter byte-identity proof
| measurement | before my first edit | after my last edit |
|---|---|---|
| `head -7 \| sha256sum` (working tree, CRLF, raw) | `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` | `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` |
| `head -7 \| tr -d '\r' \| sha256sum` (CR-stripped) | `5c2b409f16987def46a9acd142c60f1c928e37a649395c401e931f11a4660749` | `5c2b409f16987def46a9acd142c60f1c928e37a649395c401e931f11a4660749` |
| `git show HEAD:./… \| head -7 \| sha256sum` (LF blob) | `5c2b409f…4660749` | n/a (HEAD unchanged) |
| `cmp` of first 7 lines, pre-edit copy vs post-edit file | n/a | identical |
| whole file sha256 | `1dffc539d7ff16bd0a4092fb4ce39d767209ef5c54275390500ff27b2d843558` | `e6dcf6dc646e1240db6a53a5ee2ed0be20a057f68527023d0d72cd98337536fd` |
| bytes / CR / LF / CRLF / lone LF / lone CR | 19794 / 216 / 216 / 216 / 0 / 0 | 20883 / 227 / 227 / 227 / 0 / 0; ends in CRLF |

Both anchors match `TASK-1517`'s status line and `qa/TASK-1518.md`: raw `653fc50c…a4997e`, CR-stripped `5c2b409f…4660749`. The file stays uniform CRLF, counted by bytes (python `rb`), not by regex.

## Diff shape
`git diff --numstat`: `22  11  GitClaudeUnrealTest/.claude/agents/playtest-verifier.md`. Line count 216 → 227 (+11 = 22 − 11).
`git diff -U0` hunk headers (none at or above line 7; the lowest is 48):
```
@@ -48 +48,3 @@
@@ -138,2 +140,3 @@
@@ -141,2 +144,2 @@
@@ -147 +150,6 @@
@@ -165 +173 @@
@@ -181 +189 @@
@@ -198,3 +206,6 @@
```

## (1)–(6): the new text, with every pre-existing sentence I changed named
Line numbers are new-file lines.

### (1) S4, the right button (new 140–142)
- CHANGED. Before (old 138–139): *"`ui_perform` has no right mouse button (`VER-§8` cl. 12; `simulate_key_press` is being re-measured by `TASK-1519`)."*
- After (the row's words, verbatim): *"Neither `ui_perform` nor `simulate_key_press` (`RMB` / `RightMouseButton`, a tap) reaches the right-click handler (`VER-§8` cl. 12, marker `VER-8-12-SIMULATE-KEY-PRESS-RMB-MEASURED-NEGATIVE`)."*
- Source: `qa/TASK-1519-verify.md` line 1 is `Verdict: MEASURED` (re-read at start). The marker resolves to CONVENTIONS.md `VER-§8` cl. 12's 2026-09-27 sub-bullet, which cites that report. "a tap" keeps the law's "Still NOT measured: a held press" limit.

### (2) N13, the stale `TASK-1515` reference (new 143–145)
- CHANGED. Before (old 140–142): *"Set-active is actuable without it: `IA_MenuSecondary` on a focused deck-bar slot (`VER-§8` cl. 12 amendment; recipe `RCP-deckbuilder-set-active-by-keyboard.md` once `TASK-1515` ships it)."*
- After: *"Set-active is actuable without it: `IA_MenuSecondary` on a focused deck-bar slot (`VER-§8` cl. 12 amendment; recipe `RCP-deckbuilder-set-active-by-keyboard.md`)."* Only "once `TASK-1515` ships it" was dropped.
- Re-wrap only: the next sentence, *"Resolve every target with `ui_snapshot` and act by plain name plus offset, …"*, now starts its own line (145). Its text is byte-unchanged; the old line 142 held its first words, so that line shows in the diff.

### (3) N12, the two-limb exception (new 189)
- CHANGED. Before (old 180–181): *"Verdict rules: derive line 1 from the table's last column in precedence order, first match wins (`VER-§1` cl. 5/5a):"*
- After: *"Verdict rules: derive line 1 from the table's last column in precedence order, first match wins (`VER-§1` cl. 5/5a) (except a two-limb row: `VER-§1` cl. 7):"*. The row's parenthetical is inserted verbatim after the existing citation. Only line 189 changed; line 188 is untouched.

### (4) N14, the announcement line (`VER-§3` cl. 6, marker `VER-3-6-THE-REPORT-RECORDS-THE-ANNOUNCEMENT`)
- (a) Jonathan-present "you REPORT it" bullet. ADDED (new 48–50), after *"⛔ Never drive PIE silently."*, which is unchanged: *"Your report's `Editor/Aura state:` line records what the dispatch said about the announcement, quoted, or "the dispatch did not say" (`VER-§3` cl. 6, marker `VER-3-6-THE-REPORT-RECORDS-THE-ANNOUNCEMENT`)."* No pre-existing sentence in the bullet changed. The diff shows old line 48 only because "silently." shares that line.
- (b) Output template. CHANGED (old 165 → new 173), the `Editor/Aura state:` field list gained one trailing field: `…, credit if visible, what the dispatch said about the PIE announcement (quoted, or "the dispatch did not say")>; model (self-reported): <string>`. Every earlier field, including `editor instance identified by COMMAND LINE (SC-§118)`, is byte-unchanged.

### (5) `VER-§10` cl. 8, the non-run rule (new 206–211)
- CHANGED. Before (old 198–200): *"A run that could not start (editor down, Aura disconnected, row not `built`/`qa-passed`) produces NO verdict — report the blocker instead. A missing per-run "go" is not a blocker (`VER-§3` cl. 6)."*
- After: *"A run that could not start (editor down, Aura disconnected) produces NO verdict — report the blocker instead. A missing per-run "go" is not a blocker (`VER-§3` cl. 6)."* Only "row not `built`/`qa-passed`" left the first sentence; the second sentence is unchanged apart from re-wrapping.
- ADDED (208–211): *"A pre-flight stop (`integrating`, or a compile announced: How you work step 1) or an eligibility refusal (row not `built`/`qa-passed`: step 2) moves NO status and writes no `*-verify.md`; post the `🚧` or the refusal line and return (`VER-§10` cl. 8). Only a `🚧` outage flips the row to `blocked` (the next paragraph)."*
- The next paragraph (`Then flip ONLY …` and the `🚧`-outage `blocked` text, new 213–221) is byte-unchanged.

### (6) `VER-§7` cl. 2's declaration duty (new 150–155, end of S4)
- ADDED after *"… a measured run decides (`VER-§12` cl. 7c)."*, which is unchanged: *"A step reached through `run_verification_sequence` that is not on your `tools:` line (e.g. `pie_scene_edit` `call_actor_function`) is reachable, not granted (`VER-§7` cl. 2, marker `VER-7-2-GRANT-PUT-AND-DECLINED`): declare every such call in the report under `## Not examined / limitations this run` (op, target, arguments, count), and never read a row's wording as a grant."*

## FORBIDDEN list: what I did not touch
Frontmatter (hash above) · `CLAUDE.md` · `CONVENTIONS.md` · `Tools/` · the `Edit`-scope paragraph (20–29, outside every hunk) · the `SC-§118` census text (the census bullet 51–52 is outside every hunk; the template field is byte-intact inside a changed line) · STEP 6 (80–96, outside every hunk) · every `VER-§3` cl. 6 fence: announcement and report (kept verbatim, one sentence appended), census first, `-game` untouched, `.sav` proven, cl. 4 (all outside every hunk) · S1/S2/S3/S5/S6, steps 1–7, the verdict list 1–4, the MEASURED paragraph, the flip paragraph and the Slack paragraph (all outside every hunk) · git mutation (none).

## For QA (`TASK-1535`) to check closely
1. **(6) heading wording.** The row wrote `## Not examined / limitations`. I used the template's full heading, `## Not examined / limitations this run`, so the verifier can find it by exact match; the body already cites it that way at line 193. The row's two sentences are joined with a colon so the item is one sentence, as the row asks, and the marker is cited inline. If QA wants the row's literal short form, it is a one-token change.
2. **(5) "(the next paragraph, unchanged)".** I read "unchanged" as a direction to me (leave that paragraph alone), not as text for the body, and wrote *"(the next paragraph)"*.
3. **(5) scope versus the law.** `VER-§10` cl. 8's eligibility case is "a C++ row that is not yet `built`". The row, and now the body, say "row not `built`/`qa-passed`", which matches step 2's own wording (a Blueprint/asset row below `qa-passed` is refused too). That is the row's text; I did not narrow or widen it. Likewise, "or a compile announced" comes from cl. 8 (`VER-§2` cl. 2) and is not in step 1's own text, which names only `integrating`. That is why the pointer names step 1 as the place to look, not as the source of the compile case.
4. **(3) two adjacent parentheticals** (`(…cl. 5/5a) (except …cl. 7)`). Kept verbatim per the row rather than merged, so the pre-existing citation stays byte-intact.
5. **(1)'s pronoun.** In the unchanged next sentence, *"Set-active is actuable without it"*, "it" used to mean "a right mouse button" and now means "the right-click handler". It reads correctly either way. I did not touch that sentence beyond item (2), because the FORBIDDEN list bars sentences the row does not name.
6. **(4b)** is inside the template's code block, so it carries no marker. The marker lives in (4a), which names the field.
