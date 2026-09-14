# QA Report — TASK-1237
subject: TASK-1227 — [AURA-QA-INSPECTOR] `.claude/agents/qa-reviewer.md`
Verdict: PASS
reviewer: qa-reviewer · 2026-09-13 · marker `TASK-1237-AURA-GATE-1227`

⛔ This is the reviewer's OWN agent file. Verified by `Read` only; nothing edited (`SC-§64a` family — a reviewer may verify but never widen its own grant).

## What was checked, against TASK-1227's acceptance

**(1) `tools:` line — exactly one added token, nothing from `unreal_editor`, no `Bash`. ✅**
Line 4 as read from disk:
```
tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_channel, mcp__claude_ai_Slack__slack_read_thread, mcp__claude_ai_Slack__slack_search_channels
```
Token-by-token against the handoff's quoted "before": the only difference is the insertion of `mcp__unreal_inspector__*` between `Edit` and `mcp__claude_ai_Slack__slack_send_message`. Grep for `unreal_editor` and `Bash` on the file: 0 hits on line 4 (the only `Bash` mention anywhere in the file is absent; `unreal_editor` appears nowhere). The `playtest-verifier.md` draft also carries the wildcard — out of scope here (TASK-1223/1224/1235).

**(2) The paragraph names `SC-§71b` and prohibits the engine-lifecycle tools. ✅**
Line 14, second sentence: "(`SC-§71b`: a permission is not a capability, and a tool you do not hold cannot have produced a measurement)" — one occurrence, and the paraphrase is faithful to CONVENTIONS.md:5605–5612 (the law's own closing line: a role's tool list is part of its verdict's provenance). The prohibition is present, bold, ⛔-marked, and names all three verbs: "**NEVER call the inspector's engine-lifecycle tools — launch, recompile, shutdown — even though the server is read-only by name.**", followed by the ownership sentence (build-master and Jonathan).

**(3) No other line changed — hunk count 2, +3/−1. ✅ as DECLARED, consistent on read.**
I hold no `Bash`/Git (`SC-§71b` applies to this verdict), so the hunk count is the handoff's measurement, accepted as declared; the host `TASK-1241` re-measures it. What I can say from a top-to-bottom read: the file is 47 lines; line 4 is the replaced line (−1/+1), line 14 is the new paragraph and line 15 its trailing blank (+2) — that arithmetic is exactly +3/−1 and exactly two hunks. Every other line (frontmatter `name`/`description`, the "Your job" opener, the 2026-09-04 scoped-`Edit` paragraph, `## Inputs`, `## What you check` items 1–7 including the 2026-07-07 Python item, the `## Output` template, `## Slack`) reads as the file I know; no sentence outside lines 4/14/15 is unfamiliar. Placement matches the spec: directly after the scoped-`Edit` paragraph, directly before `## Inputs`.

**(4) The paragraph does not widen the grant and does not weaken the no-mutation posture. ✅**
Every clause is read-only or restrictive: "read-only window", enumerates only read surfaces (graphs, asset metadata, output log, read-only Python queries), restates the `qa/`-report-and-own-`status:`-only edit scope, adds "never change an asset, a graph, or a setting through the inspector", adds the lifecycle ban, and instructs that a missing/disconnected inspector is a provenance limit to be reported — "never work around it, and never claim you inspected what you could not reach". It also keeps `qa-passed` text-level for anything not inspected, which is the board header's own definition. Nothing grants mutation, PIE, screenshots, or `unreal_editor`.

## Findings
- [WARN] `handoffs/TASK-1227-programmer.md:38` — the handoff tells the gate "count the tokens: 10 before, 11 after". Measured on the quoted lines: **9 before, 10 after** (Read, Grep, Glob, Write, Edit + 4 Slack = 9; + inspector = 10). The delta of exactly one — which is what acceptance (1) requires — is correct; only the absolute tally is off by one. Not a defect of the diff. `SC-§104` (assert state, not tallies): downstream readers should cite "one token added", not "11 tokens". Suggested fix: the owning row (TASK-1227's author, or the host TASK-1241 in its note) corrects the handoff line; nothing in the subject file changes.
- [NIT] `.claude/agents/qa-reviewer.md:10` — the unchanged opener still says "you have no engine or Git access by design", which now sits one paragraph above a read-only engine window. Line 14 resolves the tension explicitly ("read-only window into the running editor"), and rewording line 10 would be a THIRD hunk, which TASK-1227's acceptance (hunk count = 2) forbids — so leaving it is correct for this row. If a later row touches this file, "no engine mutation or Git access" would remove the ambiguity. Not a widening; no action for 1227.
- [NIT] Report path: my agent file's `## Output` template names `qa/TASK-###-qa.md`; the TASK-1237 row names `qa/TASK-1237-report.md` ⛔ ONLY. The row is explicit and wins; this file is at the row's path (`SC-§102` silence hazard noted for any pathspec that cites the template's name instead).

BLOCKER: 0 · WARN: 1 · NIT: 2

## Limitations (provenance)
- `unreal_inspector` is NOT connected in this session (pending a Claude Code restart): none of the `mcp__unreal_inspector__*` tools are in my tool list, so the new grant could not be exercised. This is a limit on what this gate could observe, not a defect of the diff — the diff is a text grant, and its text is what the acceptance criteria are about.
- No Git: the "two hunks, +3/−1, every other line byte-identical to HEAD" claim is the handoff's, accepted as declared and found consistent on read (`SC-§71b`). `TASK-1241` must re-measure `git diff .claude/agents/qa-reviewer.md | grep -c '^@@'` = 2 itself, not inherit it from here.
- The handoff's autocrlf warning ("LF will be replaced by CRLF") is likewise declared; I cannot confirm there is no whole-file line-ending churn. The host's hunk count is the check that settles it (a line-ending rewrite would show as a whole-file hunk, not 2).

## Notes for build-master / host (TASK-1241)
- Stage `.claude/agents/qa-reviewer.md` normally (TRACKED per the handoff's `git ls-files`; registry retired 2026-09-06). Re-measure hunk count = 2 before staging.
- Optionally correct `handoffs/TASK-1227-programmer.md:38` to "9 before, 10 after" — or simply do not cite the tally.
- The standing constraint holds: `mcp__unreal_inspector__*` wholesale is permitted; `mcp__unreal_editor__*` wholesale is absent from this file.
