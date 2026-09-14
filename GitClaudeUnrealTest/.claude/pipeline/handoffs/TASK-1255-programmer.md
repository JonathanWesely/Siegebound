# TASK-1255 — [AURA-VERIFIER-INSPECTOR] gameplay-programmer handoff (2026-09-13)

**Status: done — awaiting `TASK-1260` (body, same file, sequential) + re-gate `TASK-1261`; host `TASK-1242` stages the file (⛔ it is still UNTRACKED — no `git add` here).** Law: ruling R10 · `VER-§7` cl. 2–3 (as amended 2026-09-13) · registry note (`.claude/agents/*.md` edits get their own row).

## What changed — ONE line
`.claude/agents/playtest-verifier.md` frontmatter **line 4 (`tools:`) ONLY**: the single token `mcp__unreal_inspector__*` (token index 5, between `Edit` and `mcp__unreal_editor__attach_pie_frames`) was popped and the 49 `mcp__unreal_inspector__<name>` tokens from `handoffs/TASK-1254-buildmaster.md` were inserted at that same index, sorted, comma-separated. Every other token kept its content and order. Line 4: 40 → 88 tokens (1441 → 3952 chars); file 97 lines, LF, no BOM, 10,345 bytes.

## Method (⛔ nothing typed by hand)
Python script (scratchpad `task1255.py`): the 49 were EXTRACTED by regex from the fenced block under `## The 49 granted` in `handoffs/TASK-1254-buildmaster.md`, the 13 from `## The 13 EXCLUDED`; cross-checked against the census — `handoffs/AURA-MCP-CENSUS.md` §2 fenced block (62) minus the §2a backticked names (13). Asserted BEFORE any write:
```
census |S2|=62 |S2a|=13 |S2-S2a|=49
missing = []          (each of the 49 is verbatim in census §2 AND in 1254's block)
forbidden = []        (none of the 13 §2a names is on the line)
set(granted) == S2 - S2a : True ; set(excluded) == S2a : True ; handoff block sorted : True
tokens[:5] == [Read, Grep, Glob, Write, Edit] ; wildcard at index 5, count 1
new[5:54] == the 49 ; new[54:] == old[6:] (tail unmoved) ; editor list identical in content+order ; Slack pair identical
```
Then the merged line was written as bytes (LF preserved) and READ BACK: line count, line 4 and body hash re-asserted on the on-disk file.

## Acceptance — measured on the file AFTER the write
**(1) FIRST, both wildcard greps:**
```
grep -c 'mcp__unreal_editor__\*'    .claude/agents/playtest-verifier.md  -> 0
grep -c 'mcp__unreal_inspector__\*' .claude/agents/playtest-verifier.md  -> 0
```
**(2)** `mcp__unreal_inspector__<name>` tokens on line 4 = **49** (`missing = []`, `forbidden = []` above).
**(3)** `mcp__unreal_editor__<name>` tokens on line 4 = **32**, identical in content and order to before.
**(4)** Body sha256 (file minus line 4, `sed '4d' | sha256sum`):
```
BEFORE 32eb680f0eef6067f0157e2b9f5ef7e026bd7327923b3fc9a8a8b35f8f3ff36e
AFTER  32eb680f0eef6067f0157e2b9f5ef7e026bd7327923b3fc9a8a8b35f8f3ff36e
```
— equal to each other AND to `TASK-1224`'s recorded body hash, i.e. the body has not moved since 1224 landed it (frontmatter-only edit, 1224's method).
**(5)** `git status --porcelain -- .claude/agents/playtest-verifier.md` → `?? GitClaudeUnrealTest/.claude/agents/playtest-verifier.md` — still UNTRACKED, never staged.
Also: `grep -cE 'Bash|unreal-mcp|blender'` on line 4 → **0**; no `*` anywhere on the line; token 54 = `mcp__unreal_inspector__search_geometry_scripts`, token 55 = `mcp__unreal_editor__attach_pie_frames` (the seam); last two tokens = the two Slack tools, unchanged.

## For QA (`TASK-1261`) to scrutinize
- The 49 on line 4 vs `handoffs/TASK-1254-buildmaster.md`'s fenced block and census §2 — the triples (name / census / 1254) all hold per the script; re-derive rather than inherit.
- `TASK-1260` edits the BODY of this same file after this row; the body hash quoted above is the pre-1260 baseline — 1260's handoff should quote a NEW body hash and an UNCHANGED line 4 (3952 chars, 88 tokens, 49/32).
- Loading caveat (mirrors 1254's note): a running Claude Code session read the agent registry at start; the 49-name line is seen by the next session/reload. No live-load was attempted here (not this row's acceptance).

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · no body edit · no `Bash` added to the grant · only the `TASK-1255` `status:` line edited on the board.
