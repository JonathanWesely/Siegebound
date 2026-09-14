# TASK-1228 — [AURA-SETUP-DOCS] gameplay-programmer handoff (2026-09-13)

**Status: ready-for-qa** (gate `TASK-1238`; host `TASK-1241` commits the `Docs/` copy only and re-measures the pair).

## The hash pair (SC-§68) — MEASURED, identical
```
51d112392e25dcb5a9ea6ea0ac6ed7556c3ad22e1a9b41e79fbf12330ed9a876  Docs/setupdirections.md
51d112392e25dcb5a9ea6ea0ac6ed7556c3ad22e1a9b41e79fbf12330ed9a876  C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md
```
`cmp` → BYTE-IDENTICAL · 67,374 bytes each (was 55,770 / `6b4a22cd…d5358` on both before the edit — the pair was already identical at my instant). LF line endings preserved (`grep -c $'\r'` = 0). The vault copy was produced by `cp -f` of the finished repo file; it is outside the repo and was never staged.

## Files touched
- `Docs/setupdirections.md` — EDITED (164 insertions, 0 deletions).
- `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md` — overwritten with the identical bytes (path used verbatim from `names:`).
- `.claude/pipeline/TASKBOARD.md` — ONLY my row's `status:` line (`backlog` → `ready-for-qa`, board line 2504). Other TASKBOARD diff lines in `git status` belong to the wave-1 parallel rows.
- This handoff.

## Hunk list (`git diff -U0 -- Docs/setupdirections.md`) — four hunks, ALL pure insertions, ⛔ no existing chapter edited
| Hunk | Where | What |
|---|---|---|
| `@@ -39,0 +40 @@` | THE LIST, after row 16 | **Row 17** — Aura AI for Unreal account (trial first → paid after the pilot measures credit-per-verification; no API key — account login; training-toggle decision BEFORE the first index; Ch. 11) |
| `@@ -780,0 +782,142 @@` | between Chapter 10's `---` and `## Appendix A` | **Chapter 11 — Aura** (§11.1–11.7), 142 lines |
| `@@ -795,0 +939,5 @@` | Appendix A, after row 11 | **Rows 12–16** |
| `@@ -811,0 +960,16 @@` | Appendix B, after the last existing bullet | the ⚠️ unverified-items block (one parent bullet, six sub-bullets) |

Measured numbers at my instant (matches the spec's): last chapter was 10 → now 11; THE LIST ended at 16 → now 17; Appendix A ended at 11 → now 16. Chapter header line 782; `## Appendix A` now at 924, `## Appendix B` at 945.

## Appendix A rows 12–16 — character-exact (SC-§38a)
Rows were extracted from the source doc's table with `grep '^| 1[2-6] |'` into a scratch file and spliced as bytes, never retyped. Verified: `diff <(grep '^| 1[2-6] |' "Docs/Aura AI for Unreal — Integration Plan.md") <(sed -n '/^## Appendix A/,/^## Appendix B/p' Docs/setupdirections.md | grep '^| 1[2-6] |')` → empty (BYTE-IDENTICAL). Note the header/separator row of the existing Appendix A table (`|---|-------|--------|`) is unchanged — only rows were added.

## Chapter 11 — the eight required topics, where each lives
1. Engine-level install for 5.8 + plugin enable → §11.1 steps 3–4 (installs under `<UE_5.8>/Engine/Plugins/Marketplace/Aura/`; `.uproject` gains an `Aura` entry — measured in the live `.uproject`: `"Name": "Aura", "Enabled": true` + `SupportedTargetPlatforms`; build-master commits it under a task).
2. `Aura.exe` tray + local port 41200 → §11.1 step 5.
3. Set up Unreal MCP (Aura → Epic's MCP; our `unreal-mcp` `:8000` untouched) → §11.2 step 1.
4. Filesystem Sandbox (experimental, 5.8-only, `Intermediate/Sandboxes/AuraSandbox`, C++ not covered) → §11.2 step 2.
5. `Saved/.Aura` gitignored ⇒ `INDEX_IGNORE.txt` + `project_memory.txt` regenerated from `Docs/AuraIndexIgnore.txt` + `Docs/AuraProjectMemory.md` by `Tools/aura_sync.ps1` as a session-start step; no skills mirror → §11.3 (with a two-column canonical→copy table).
6. The two stdio servers with the MEASURED paths (`…/Aura/PortablePython/Windows/python.exe`, `…/Aura/MCP/unreal_inspector.py` / `unreal_editor.py`, `"type": "stdio"`), the MEASURED one-click fact (writes `~/.claude.json` → `mcpServers`, user scope; `~/.claude/mcp.json` never created — Aura's doc is wrong), and the house rule (project `.mcp.json` + `enabledMcpjsonServers`, remove the user-scope copy, restart) → §11.4. Sources: `handoffs/TASK-1221-buildmaster.md`, the live `.mcp.json`, and a read-only key-name probe of `~/.claude.json` (no values echoed).
7. The allow-list law (`mcp__unreal_inspector__*` wholesale; `unreal_editor` enumerated from the `/mcp` census; ⛔ never `mcp__unreal_editor__*`) → §11.5.
8. `playtest-verifier` + the `verified` gate, one paragraph (after compile, one at a time, announce when Jonathan is present, `VERIFIED | VERIFY-FAILED | UNOBSERVABLE`, hard-gate line, advisory-until-three-matches) → §11.6.
Plus: the training-toggle privacy note (§11.1 step 2 + LIST row 17), the secrets-law restatement (no API key), and §11.7 Verify pointing at Appendix A rows 12–16.

## Appendix B entries (spec: the ⚠️ items from §3 + §7)
Fab $150 SKU (single-sourced) · Enhanced Input under input simulation · whether the Sandbox adds a second `.uproject` entry · credit per verification (tier input) · the real `mcp__unreal_editor__*` tool names (census-only) · `USiegeAssistantSnapshot`/cheat-manager observability (written generically as "the game's C++ assistant-snapshot / cheat-manager state" — the setup doc's convention generalizes project specifics).

## Corrections-table compliance (plan OVERRIDES source doc)
- `INDEX_IGNORE.txt` documented at `Saved/.Aura/` (NOT project root as the source doc §4 step 2.3 says).
- The one-click's real target documented as `~/.claude.json` / `mcpServers` (source doc + Aura doc say `~/.claude/mcp.json` / `servers`).
- Skills-mirror step explicitly DROPPED (`.claude/skills/` does not exist).
- Verifier thread named only by role; ⚙️ Dev & QA is the thread — the doc's "🧪 Dev & QA" wording is not reproduced.
- Tool names stated as census-only, never guessed.

## Secrets check (⛔ no secrets)
`grep -n -i "service_role\|api[_ ]key\|token=\|sbp_\|eyJ"` over the finished file → only pre-existing Chapter 7/10 lines (Meshy key location, Supabase `service_role` law) plus my own sentence "there is no API key to store anywhere". Nothing new that is a secret; the Aura paths are paths.

## What QA should scrutinize
- The hunk list above vs `git diff -U0` (four hunks, all `-N,0`).
- Appendix A rows 12–16 vs the source table (they were spliced as bytes; the diff command above reproduces the check).
- The hash pair is ACCEPTED-AS-DECLARED at the gate (`SC-§71b`) and re-measured by `TASK-1241`.
- `Tools/aura_sync.ps1`, `Docs/AuraIndexIgnore.txt`, `Docs/AuraProjectMemory.md` were ABSENT when I started and are PRESENT (untracked) now — wave-1 siblings landed them; the chapter references them by the spec's names either way.

## Fences honoured
No Git beyond `git diff`/`git status` · no editor · no compile · nothing staged · vault copy never staged · no other chapter of the setup doc touched · only my board row's `status:` line changed.
