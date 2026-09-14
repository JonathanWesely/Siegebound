# TASK-1227 — [AURA-QA-INSPECTOR] — programmer handoff

marker `TASK-1227-AURA-QA-INSPECTOR` · assignee gameplay-programmer · 2026-09-13
Gate: `TASK-1237` (qa-reviewer, reviewing its own file — verify only, may not widen) · Host: `TASK-1241`

## What changed

One file, two hunks, measured with `git diff` (hunk count 2, numstat `+3 / -1`):

**Hunk 1 — the `tools:` line (frontmatter line 4), one added token, placed after `Edit`, before the Slack tools.**

Before:
```
tools: Read, Grep, Glob, Write, Edit, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_channel, mcp__claude_ai_Slack__slack_read_thread, mcp__claude_ai_Slack__slack_search_channels
```
After:
```
tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_channel, mcp__claude_ai_Slack__slack_read_thread, mcp__claude_ai_Slack__slack_search_channels
```
Nothing from `unreal_editor`, no `Bash` (standing constraint: `mcp__unreal_inspector__*` may be granted wholesale; `mcp__unreal_editor__*` never).

**Hunk 2 — one new paragraph (plus its trailing blank line) inserted directly after the "Your job" scoped-`Edit` paragraph, before `## Inputs`.** It states, in order:
- QA may now INSPECT via `unreal_inspector` — read-only: Blueprint graphs, asset metadata, the editor output log, read-only Python queries — to check a claim it previously accepted as declared, citing **`SC-§71b`** (a permission is not a capability; a tool you do not hold cannot have produced a measurement).
- The report must say what was inspected and what was not; a `qa-passed` remains a text-level verdict for anything not inspected.
- No-mutation posture unchanged: still edits nothing but its `qa/` report and its own row's `status:`; never changes an asset/graph/setting through the inspector.
- ⛔ **NEVER call the inspector's engine-lifecycle tools (launch / recompile / shutdown) even though the server is read-only by name** — the editor's lifecycle belongs to build-master and Jonathan.
- A missing/disconnected inspector is reported as a provenance limit, never worked around, and never claimed as inspected.

## Files touched
- `.claude/agents/qa-reviewer.md` — the only edit (TRACKED; `git ls-files` confirms; stage normally at `TASK-1241`)
- `.claude/pipeline/handoffs/TASK-1227-programmer.md` — this note
- `.claude/pipeline/TASKBOARD.md` — my row's `status:` only (`backlog` → `ready-for-qa`)

## Assets referenced
None. No code, no engine, no Git beyond `git diff` / `git ls-files` / `git status` on the one path.

## For the gate (TASK-1237) to scrutinize
1. `tools:` = the prior line + exactly `mcp__unreal_inspector__*` — count the tokens: 10 before, 11 after.
2. The paragraph names `SC-§71b` (once, in the second sentence) and the lifecycle prohibition (the ⛔ bold sentence naming launch, recompile, shutdown).
3. Hunk count = 2 — `git diff .claude/agents/qa-reviewer.md | grep -c '^@@'` prints `2`. The gate holds no `Bash` (`SC-§71b`), so it verifies by `Read`: line 4 and the paragraph at lines 14–15; every other line is byte-identical to `HEAD`. The host (`TASK-1241`) re-measures the hunk count with Git.
4. `git diff` emits an "LF will be replaced by CRLF" warning on this path: that is the repo's autocrlf setting reporting on the working copy, not a line-ending rewrite by this edit — the diff shows two hunks, no whole-file churn.

## Slack
Posted once in ⚙️ Dev & QA (`1783116269.740549`, channel `C0BF0QZP3CN`), prefix `⚙️ GAMEPLAY-PROGRAMMER:` per `SLACK.md` line 63 (the dispatch prompt said `⚙️ PROGRAMMER:`; the board row and SLACK.md both say `GAMEPLAY-PROGRAMMER`, so the law's spelling was used).
