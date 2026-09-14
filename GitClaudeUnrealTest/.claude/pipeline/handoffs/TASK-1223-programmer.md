# Handoff — TASK-1223 [AURA-VERIFIER-DRAFT] — gameplay-programmer, 2026-09-13

## What I did
Wrote the DRAFT seventh-agent file `.claude/agents/playtest-verifier.md` (⛔ NEW, ⛔ UNTRACKED — no `git add`, and none until `TASK-1242`). Frontmatter = the three keys; body = the source doc's §6 "Draft body for `playtest-verifier.md`" with the plan's overrides applied, plus the three additions the dispatch asked for (scoped-Edit paragraph in the qa-reviewer shape, a "Jonathan present ⇒ do not start until told go" section, and the row's exact law sentence).

## Files touched (this row only)
- `.claude/agents/playtest-verifier.md` — NEW, untracked, 96 lines, sha256 `8a03b359782db0306543a45a2faaf6dbae44701034de0996c4e064ea5260f836` (the body-hash baseline `TASK-1224` acceptance (4) compares against — note that hash covers the whole file, so `TASK-1224` should hash lines 6–96 (everything after the closing `---`) before and after its frontmatter-only edit).
- `.claude/pipeline/TASKBOARD.md` — ONLY the `TASK-1223` `status:` line (`backlog` → `done …`). Verified by `git diff -U0`: the other five changed lines in the working copy are concurrent rows' own flips (1214 discharge, 1216 stage A, 1221 done, two `ready-for-qa` rows) and all survived. Line endings intact (LF before and after).
- `.claude/pipeline/handoffs/TASK-1223-programmer.md` — this note.
- ⛔ No other agent file touched. `qa-reviewer.md` shows modified in `git status` — that is `TASK-1227`'s parallel edit (its `unreal_inspector` grant + paragraph), not this row's.

## Acceptance, measured
1. **Three frontmatter keys** — `name: playtest-verifier` · `description:` = §4 Phase 3 step 4 verbatim · `tools:` = the placeholder line (below). Lines 1–5 quoted in the measurement run.
2. **The six override sites, quoted from the file:**
   - Thread — line 91–92: `Post once in the ⚙️ Dev & QA standing thread of \`#siegeboundue5agentteam\` (channel \`C0BF0QZP3CN\`, thread_ts \`1783116269.740549\`; …)` (⛔ not "🧪 Dev & QA").
   - Prefix — line 93: `prefixed \`🎮 VERIFIER:\` + status emoji + TASK-###`.
   - Report path — line 69 heading `## Output — \`.claude/pipeline/qa/TASK-###-verify.md\`` (also line 15, the scoped-Edit paragraph, and line 95).
   - Verdicts — line 72: `Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE`, plus the "Verdict rules" paragraph defining each.
   - One run at a time — line 40: `1. ONE verification at a time. Never run while build-master is assembling or art-director is importing (\`VER-§\` serialization) … Never queue a second run behind the first.`
   - Only `built` / `qa-passed` rows — line 43–44: `AND that the task's row reads \`built\` (C++) or \`qa-passed\` (Blueprint/asset-only) — you only ever test binaries that exist.` (+ an explicit list of statuses that are NOT verifiable).
   - Evidence naming (the plan lists this as part of the override set) — line 60–62: `\`VER\` prefix: \`VER-###[-t<MM>m<SS>s]-<symptom>.png\`, where \`###\` is the TASK number (not a sequential VID counter)` under `.claude/pipeline/playtest-evidence/<YYYY-MM-DD>/` (`FR-§1` shape, `VID`→`VER`).
3. **"never edits code or assets, never compiles, never runs Git"** — present character-exact at line 3 (description) AND line 11 (body).
4. **The UNOBSERVABLE rule** — line 90: `(UNOBSERVABLE leaves the status at \`qa-passed\` and appends \`verify: unobservable\`).`
5. **Untracked** — `git status --porcelain -- .claude/agents/` → `?? GitClaudeUnrealTest/.claude/agents/playtest-verifier.md`; `git ls-files --others --exclude-standard` lists it. No `git add` was run.
6. **No other agent file touched** — see Files touched.
7. **Wildcard law** — `grep -c 'unreal_editor__\*'` = `0`; `grep -c 'mcp__unreal_editor__'` = `0` (no `unreal_editor` name of any kind appears — nothing guessed).

## Two dispatch-vs-row deviations, recorded (QA / orchestrator should rule)
- **The placeholder `tools:` line.** The row's literal is `tools: ⚠️ tool names pending /mcp census — finalized by TASK-1224`. The dispatch prompt specified a different exact line and I followed the dispatch (it is the orchestrator's course correction and is a *parseable* tools list, so the file loads if someone tries it):
  `tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_thread  # ⚠️ PLACEHOLDER — unreal_editor PIE/verify/screenshot tools pending the /mcp census (TASK-1224)`
  The row's intent ("⛔ no MCP name guessed") is preserved: `mcp__unreal_inspector__*` is a wholesale grant the section head explicitly permits, and the two Slack names are the registry's known names; zero `unreal_editor` names. `TASK-1224` replaces this whole line.
- **Slack prefix.** The dispatch said `⚙️ PROGRAMMER:`; the row and `SLACK.md` "Identity prefixes (MANDATORY)" both say `⚙️ GAMEPLAY-PROGRAMMER:`. I posted with the registered prefix.

## Things QA (TASK-1235, on the final file) should scrutinize
- **Promotion without a shell.** The body promotes evidence into `playtest-evidence/<date>/` but the tools line has no `Bash` (1224 forbids it) and `Write` cannot copy a PNG. I wrote the honest fallback: if the screenshot tool cannot write to that path directly, record the `Saved/` path and mark the promotion as owed to the host row — never claim a path that does not exist. Whether Aura's screenshot tool takes an output path is a census question (`TASK-1216` B / `TASK-1224`); if it does not, `VER-§` (the CONVENTIONS row) should name who copies.
- **`VER-###` = TASK number**, per the row, deliberately NOT the sequential `VID-###` counter of `FR-§1` — the file says so explicitly so no one "fixes" it back.
- **Body-hash baseline for 1224 acceptance (4)** — hash the body (lines 6–96) not the whole file, since 1224 legitimately changes line 4.
- The §6 draft's `CONVENTIONS.md VER-§` reference is kept verbatim; `VER-§` does not exist yet (plan item 10's row writes it).
- Status flipped to `done`, not `ready-for-qa`: the row says "this draft is not separately gated — `TASK-1235` reviews the FINAL file after `TASK-1224`".

## Not done (by law)
No compile, no editor, no Git write, no `Saved/` write, no `.mcp.json` / `settings.local.json` touch.
