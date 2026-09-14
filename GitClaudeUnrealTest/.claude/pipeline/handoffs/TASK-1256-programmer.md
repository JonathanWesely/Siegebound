# TASK-1256 — [AURA-QA-INSPECTOR-2] gameplay-programmer handoff (2026-09-13)

**Status: done (gate WAIVED into `TASK-1242`'s integration check per the row — wildcard grep = 0 + inspector token count = 49; a narrowing of QA's own file, `TASK-1237` guarded widening and this row cannot widen).** Law: ruling R10 · `VER-§7` cl. 3 (as amended 2026-09-13) · `SC-§71b` · registry note. ⛔ NOT staged, NOT committed — `.claude/agents/qa-reviewer.md` is tracked (`1306efe`); the change sits in the working tree as ` M` for Host C `TASK-1242` to commit (`git diff --cached --stat` on the path = empty, measured).

## What changed — line 4 of `.claude/agents/qa-reviewer.md`, and nothing else
The single token `mcp__unreal_inspector__*` (added by `TASK-1227`, committed `1306efe`) was replaced, in place, by the 49 `mcp__unreal_inspector__<name>` tokens from the fenced block in `handoffs/TASK-1254-buildmaster.md` (`## The 49 granted …`), comma-separated, sorted, one per name. Every other token stays in its original position and order:

- before the swap point (unchanged): `Read, Grep, Glob, Write, Edit`
- after the swap point (unchanged): `mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_channel, mcp__claude_ai_Slack__slack_read_thread, mcp__claude_ai_Slack__slack_search_channels`
- token count 10 → 58 (58 − 10 = 48 = 49 added, 1 removed); ⛔ no `Bash` token (grep on line 4 = 0); ⛔ no `unreal_editor` token of any kind.

The names were EXTRACTED by regex from the handoff's fenced block, never typed (script: session scratchpad `task1256.py`; ⛔ not a repo file). In-script assertions before the write: |granted| = 49 distinct, sorted; |excluded (§2a block)| = 13; granted ∩ excluded = ∅; census `## 2.` block = 62 names; granted ∪ excluded == census §2 (set equality); `missing = []` · `forbidden = []`. The parsed token list was `pop`-at-index + `insert`-at-index — merged, never rebuilt. File encoding preserved: UTF-8, no BOM, LF (asserted no `\r\n` before and after).

## Acceptance — measured after the write
```
(1) grep -c 'mcp__unreal_inspector__\*' .claude/agents/qa-reviewer.md      -> 0
    grep -c 'unreal_editor'              .claude/agents/qa-reviewer.md      -> 0   (HEAD also 0 — unchanged)
(2) grep -o 'mcp__unreal_inspector__' … | wc -l                              -> 49  (== TASK-1254's 49, list-equal, same order)
    missing = []   forbidden = []   (each of the 49 in census §2; none of the 13 §2a names)
(3) git diff --stat HEAD -- GitClaudeUnrealTest/.claude/agents/qa-reviewer.md   (run from the git root one level up, SC-§102)
      GitClaudeUnrealTest/.claude/agents/qa-reviewer.md | 2 +-
      1 file changed, 1 insertion(+), 1 deletion(-)
    hunks (`grep -c '^@@'`) = 1 ; the -/+ pair is the `tools:` line only
(4) body sha256 (file minus line 4, `sed 4d | sha256sum`)
      before = 3c7097f54de393352303f5a29b7f7b833755dd7bd66098c391cf7fb9cb89f61a
      after  = 3c7097f54de393352303f5a29b7f7b833755dd7bd66098c391cf7fb9cb89f61a   (identical)
    file bytes 5862 -> 8373
```

## The `TASK-1227` paragraph — checked, still reads true, untouched
Line 14 still carries, verbatim: *"⛔ **NEVER call the inspector's engine-lifecycle tools — launch, recompile, shutdown — even though the server is read-only by name.**"* — and the rest of the paragraph (inspect to check a declaration; no-mutation posture unchanged; edits nothing but the `qa/` report and its own `status:`; a missing inspector is a provenance limit, not something to work around). After this row the grant and the paragraph agree from both sides: the paragraph forbids calling `launch_unreal_project` / `recompile_unreal_project` / `shutdown_headless` (+ `cancel_operation`), and the `tools:` line no longer carries them (all four are in the 13 excluded). The sentence is now belt-and-braces rather than the only fence — which is what `VER-§7` cl. 1/3 ask for ("a body law is not a harness"). No wording change was needed or made.

## For Host C (`TASK-1242`) — scrutinise
- Re-run the two greps in (1)/(2) before staging; both are cheap and are the waived gate.
- `git diff` prints `warning: … LF will be replaced by CRLF the next time Git touches it` for this path — that is the repo's pre-existing autocrlf configuration, not a change from this row (the working file is LF before and after; HEAD's blob is LF). Flagged so the warning is not read as a defect.
- A running `qa-reviewer` session (if any) keeps the tools it was loaded with; the enumerated line takes effect on the next agent launch.

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · no `Saved/` write · no `.mcp.json` / `settings.local.json` edit · only line 4 of `qa-reviewer.md` + this handoff written · only the `TASK-1256` `status:` line edited on the board.
