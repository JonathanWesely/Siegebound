# TASK-1503 — VERIFIER-BATCHING-DOCTRINE — programmer handoff

- **Row:** `TASKBOARD.md` marker `TASK-1503-VERIFIER-BATCHING-DOCTRINE`
- **Author:** gameplay-programmer, 2026-09-26
- **File touched (the only one besides this handoff and my own status line):** `.claude/agents/playtest-verifier.md`, body only
- **Placement choice (the spec's "your call; say which"):** a new **`## Speed (`VER-§13`) …` section** inserted between the end of `## How you work` (step 7) and `## Output`. I did not append numbered steps. The items are labelled **S1–S6** so none of them can be read as a second "step 6" next to the STEP 6 (`Read` the frame) that other documents cite by number. No existing step was renumbered or reworded.

## Frontmatter proof (spec (8), dispatch note on `TASK-1504`)

- `head -7` sha256 **before** my first edit: `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e`. **After** my last edit: `653fc50c…a4997e`, identical. Lines 1–7 (including `TASK-1504`'s `model:`/`effort:` lines 5–6 and the closing `---` at line 7) are byte-identical to what was on disk when I started.
- `tools:` is still line 4. Its content hash `0f49aa40…764c717` matches `HEAD`'s line 4.
- Whole-file sha256: `677568e4…28f33c` at start → `4db4ee6a0c5fb4ac6055a47de9de7d0a838e23d5fb7c03e7d76bc4d5d2fa177b` now.
- Line endings: the file is CRLF on disk (`core.autocrlf=true`). After the edit, CR = LF = 183, so no mixed endings.

## `git diff -U0` (git root one level up, `SC-§102`), relative to `HEAD` with `TASK-1504`'s lines present

```
@@ -4,0 +5,2 @@ tools: …          <- TASK-1504's (model/effort), NOT mine; the only hunk above the closing ---
@@ -94,0 +97,44 @@ …             <- mine: the ## Speed section (pure insertion)
@@ -97 +143 @@ …                  <- mine: template line 1
-Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE
+Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE | MEASURED
@@ -99 +145 @@ …                  <- mine: template Editor/Aura state line
-Editor/Aura state: <connected y/n, map, PIE mode, editor instance identified by COMMAND LINE (SC-§118), attempts used of 3, wall time, credit if visible>
+Editor/Aura state: <connected y/n, map, PIE mode, editor instance identified by COMMAND LINE (SC-§118), attempts used of 3, wall time, credit if visible>; model (self-reported): <string>
@@ -105,0 +152,2 @@ …             <- mine: two template section lines (pure insertion)
@@ -115,0 +164,8 @@ …             <- mine: the MEASURED verdict rule (pure insertion)
```

`git diff --numstat` on the file = `58 2`. `TASK-1504` accounts for `2 0`, so this row's share is **56 added, 2 deleted**, and the 2 deletions are the two template lines above. **Zero hunks of mine are at or above the closing `---` (line 7).**

## Where each spec item landed (line numbers in the file as it stands now, 183 lines)

| item | lines | quoted |
|---|---|---|
| (1) Batching | 102–111 (S1) | "Repeated or predictable input (N gestures on one button, a known key sequence, a menu walk) goes into ONE `run_verification_sequence`, not one tool call per gesture. ⛔ Rapid batches DROP gestures: 42 of 49 `double_click`s landed at 3-frame spacing and 27 of 30 at 20-frame spacing (`qa/PLAYTEST-archer50-verify.md` §1(c)). … After every batched edit, READ the state it changed, send a top-up batch sized to the measured shortfall, and repeat until the read matches. Your report states sent vs landed for every batch …" |
| (2) One-batch rule | 112–116 (S2) | "A state SET, its CONFIRM, and every read that depends on the set share ONE `run_verification_sequence` (`qa/TASK-1391-verify.md` C3: 0.854 s and 0.874 s end to end in its two attempts, …). ⛔ Never split that group to make a batch smaller or a run faster." |
| (3) Recipes first | 117–124 (S3), plus template lines 152–153 | "Before you plan, `Read` `Tools/Verify/recipes/README.md`. Load every `RCP-*.md` recipe that covers a step … and cite it under `## Recipes used` in your report. A recipe is a CLAIM (`SC-§101`): on its first use after an Aura plugin update (`VER-§8` cl. 5) or after an edit to the screen it drives, re-verify it inside your run. … ⛔ You never write under `Tools/`: propose a new sequence under `## Recipe candidates` in your report, and the manager boards the promotion." |
| (4) Pointer facts | 125–130 (S4) | "`ui_perform` `double_click` fires `UButton.OnClicked` exactly once per gesture; `click` and `press`/`release` do not, and they stay named dead ends (`VER-§5` cl. 5). There is no right mouse button on this lane (`VER-§8` cl. 12). Resolve every target with `ui_snapshot` and act by plain name plus offset, because a `name_path` selector that misses does not error: it acts at screen centre and reports success (`VER-§12` cl. 7a)." |
| (5) Recording | 131–134 (S5) | "The PIE recorder writes a raw `.h264` elementary stream. Name that `.h264` path in your report as the tool returned it. ⛔ Never claim an `.mp4` you did not see: the remux is the orchestrator's, because you hold no shell." |
| (6) Model self-report | 145 (template) + 135–139 (S6) | template: "`…credit if visible>; model (self-reported): <string>`". S6: "… the model string you observe for yourself, quoted as seen, or `not observed`. ⛔ Never copy the `model:` line of your frontmatter into it … Effort is not self-observable: do not report one." |
| (7) Template line 1 + `VER-§10` pointer | 143 (template) + 164–171 (verdict rules) | "`Verdict: VERIFIED \| VERIFY-FAILED \| UNOBSERVABLE \| MEASURED`" and "… Its board flip is `VER-§10` cl. 2: `status:` → `verified`, with the word `MEASURED` in the status line's first sentence, stating that the verdict word is `MEASURED`, not `VERIFIED`; the row also records `verify: measured — <the one-line finding>` (`VER-§1` cl. 5a)." |

## Pre-existing sentences changed

**Two lines, both in the Output template:**
1. Template line 1 (now line 143): `| MEASURED` appended.
2. Template `Editor/Aura state:` line (now line 145): `; model (self-reported): <string>` appended after the closing `>`.

Every other change is a pure insertion. The `Edit`-scope paragraph (lines 20–29), the Jonathan-present section (37–45), steps 1–7 including STEP 6 (67–83), the table header (147, which `VER-§1` cl. 3 pins character for character), the "Line 1 is the verdict" paragraph, the existing verdict-rule sentences, the flip sentence and the Slack paragraph are all byte-unchanged.

## Additions beyond the literal (1)–(7), declared so QA can rule on them

- **Two template lines, 152–153:** `## Recipes used (…or "none")` and `## Recipe candidates (…omit if none)`. (3) says "cite it in the report" and "propose … as `## Recipe candidates`" but names no place for the citation. A fixed, mandatory `## Recipes used` section (with "none" allowed) makes a missing citation visible as an empty section. `## Recipe candidates` is the name `VER-§13` cl. 3 already uses.
- **`verify: measured — <finding>`** in the MEASURED rule. `VER-§1` cl. 5a requires it alongside `VER-§10` cl. 2's flip, so the pointer names both.
- **S1's overshoot guard** ("the card tile "+" greys only at the per-card 50, and the deck total has no cap") is `VER-§13` cl. 1's own last sentence. The "+" grey-at-50 fact is measured in `archer50` §1(c).
- **S6's `not observed` fallback**, so a verifier that cannot see a model string has an honest token instead of guessing or copying the pin.
- **The section preamble** says speed never overrides the `Edit` scope, the Jonathan-present wait, one-verification-at-a-time, the ≤3-attempt budget or STEP 6. It strengthens those rules and weakens none of them.

## Findings for the manager (out of this row's scope; I left them unfixed)

- **F1 — stale line citations in live law.** `CONVENTIONS.md` `VER-§1` cl. 1 cites the template at `.claude/agents/playtest-verifier.md:80`, and cl. 3 cites the table header at `:84`. Both were already stale before this row: with `TASK-1504`'s lines they were at 99 and 103. After this row they are at **143** and **147**. My suggestion is to cite them by content or anchor, not by line number. `CONVENTIONS.md` is forbidden to this row.
- **F2 — `VER-§10` cl. 1 (`blocked`) is not taught by the agent file.** The body still says a run that could not start "produces NO verdict — report the blocker instead", and the flip sentence still names only `verified`/`verify-failed`. `VER-§10` cl. 1 widened the grant to `blocked` on 2026-09-24. (7) scopes this row to MEASURED's flip, so I did not add it.
- **F3 — the verdict rule for `VERIFIED` is narrower than `VER-§1` cl. 5.** The body's "`VERIFIED` only when every acceptance line with a runtime signal was observed passing" is stricter than cl. 5's rule (no `fail` and ≥1 `pass` ⇒ `VERIFIED`, with `verify: partial (n/m)`). This predates this row, and (8) forbids changing the meaning of existing text, so I left it unchanged.
- **F4 (historical, no action):** `qa/TASK-1459-verify.md:34` cites `playtest-verifier.md:10` for "You VERIFY ONLY". That text has been at line 11 since `TASK-1504`. Historical reports are not re-scored.

## Not examined / limitations

- There is no runtime check: no editor, no PIE. The first live use is `TASK-1511`. Whether the verifier can actually observe its own model string is **not measured**, which is why S6 carries the `not observed` token.
- I did not check how Markdown renders the new section. I did check that no continuation line starts with a list marker (`+`, `-`, `*`, `N.`) that would break the S-list.
- `CONVENTIONS.md`, `CLAUDE.md` and everything under `Tools/` were read only. No write, no git write, no compile.

## QA should scrutinise

- S1–S6 against `VER-§13` cl. 1–4, `VER-§5` cl. 5 (2026-09-26 narrowing), `VER-§8` cl. 12 and `VER-§12` cl. 7a/7b, for anything that restates the law beyond a pointer or contradicts it.
- The MEASURED rule (164–171) against `VER-§1` cl. 3a/5a and `VER-§10` cl. 2/4.
- Whether the two added template lines (152–153) are acceptable report-shape additions under `VER-§1`.
