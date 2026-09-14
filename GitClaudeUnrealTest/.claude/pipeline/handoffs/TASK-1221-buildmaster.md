# TASK-1221 — [AURA-MCP-JSON] build-master handoff (2026-09-13)

**Status: done (config row, gate WAIVED per the board; live verification = `TASK-1216` stage B, the orchestrator's `/mcp` census). ⛔ NOT committed — `.mcp.json` is committed by `TASK-1242`.**

## What was written
1. `GitClaudeUnrealTest/.mcp.json` (tracked) — two new blocks appended under the existing `"mcpServers"` key after `blender`, tab-indented like the rest of the file, LF line endings preserved:
   - `unreal_inspector`: `"type": "stdio"`, `command` = `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe`, `args` = `["C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_inspector.py"]`
   - `unreal_editor`: same `command`, `args` = `["C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_editor.py"]`
   - No `env` block (Aura's own portable python; the blender UTF-8 env is not demonstrably needed).
2. `GitClaudeUnrealTest/.claude/settings.local.json` (gitignored, `git check-ignore -v` → `.gitignore:16`; NEVER staged) — `"unreal_inspector"` and `"unreal_editor"` appended to `enabledMcpjsonServers`. Nothing else touched: `permissions.allow` count 45 before and after (`TASK-1222` owns it).

## Acceptance quotes (measured 2026-09-13)
- `ConvertFrom-Json .mcp.json: SUCCESS - mcpServers keys = unreal-mcp, blender, unreal_inspector, unreal_editor`
- `ConvertFrom-Json settings.local.json: SUCCESS - enabledMcpjsonServers = ["unreal-mcp","blender","unreal_inspector","unreal_editor"] (count 4)`
- `Test-Path command  = True` · `Test-Path inspector = True` · `Test-Path editor    = True`
- `unreal-mcp` + `blender` untouched: sha256 of lines 1–22 (the whole pre-existing prefix through blender's `env` close) HEAD = working = `147C447E3ADC64C92C6AB1979BEF6BF0052A8011B65C93CB03A66556F32F773D`; `git diff .mcp.json` shows only the 16 added lines (+ the `},` continuation of blender's closing brace).
- `unreal-mcp.type/url = http http://127.0.0.1:8000/mcp` (unchanged)
- `git status --porcelain` → ` M GitClaudeUnrealTest/.mcp.json` only.
- `grep -c 'unreal_editor__\*'` in `settings.local.json` = 0.

## Row acceptance item (5) — `~/.claude/mcp.json`
ABSENT (`ls ~/.claude/mcp.json` → no such file). Nothing to ask Jonathan about. Note (read-only observation, not touched): `~/.claude.json` contains one `"unreal_inspector"` and one `"unreal_editor"` key each — the user-scope duplicate the orchestrator said it removes.

## Fences honoured
No commit · no push · no Claude Code restart (orchestrator restarts for `TASK-1216` B) · `~/.claude.json` untouched · no `permissions.allow` change · nothing under `Saved/`.

## Note for the orchestrator
`git diff` printed `warning: in the working copy of 'GitClaudeUnrealTest/.mcp.json', LF will be replaced by CRLF the next time Git touches it` — this is the repo's pre-existing autocrlf behaviour on an LF file, not something this row introduced (the file was LF at HEAD too; perl preserved it). Also: `unreal-mcp` (`:8000`) reported ConnectionRefused in this session — the editor is not up; irrelevant to this row but relevant to the census's "four connected" line.
