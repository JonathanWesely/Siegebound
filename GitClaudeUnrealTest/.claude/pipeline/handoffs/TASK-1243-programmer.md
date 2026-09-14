# TASK-1243 — [AURA-SETUP-DOCS-D] gameplay-programmer handoff (2026-09-13)

**Status: ready-for-qa (gate `TASK-1249`; host `TASK-1242` re-measures the hash pair). ⛔ Not committed, not compiled, no git operations. The vault twin is outside the repo and is never staged.**

Ruling R2 (`TASKBOARD.md` WAVE-1 GATE RULINGS), answering `qa/TASK-1238-report.md` WARN: §11.4 step 3 and §11.5 pointed at "(Appendix D)" for the four-server `enabledMcpjsonServers` line and the Aura allow-list, but D.1 still read `["unreal-mcp", "blender"]` and D.4 still listed two local servers.

## Acceptance (1) — FIRST, the identical-hash line (`SC-§68`)
```
sha256sum Docs/setupdirections.md  C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md
208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58  Docs/setupdirections.md
208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58  .../JonWesOBVault/GitClaudeUnrealsetupdirections.md
```
Both **69,979 bytes · 1,126 lines** (was 67,374 / 1,086 at `51d11239…9a876`, the TASK-1228 pair — re-measured identical on both copies BEFORE this edit, so the edit started from a clean twin). `cmp` → byte-identical. 0 CR bytes, no BOM (first bytes `23 20 53` = `# S`), LF, UTF-8 — the file's existing style.

## Acceptance (3) — the wildcard line, quoted
Inside the D.1 fenced `jsonc` block (lines 1007–1095):
```
sed -n '1007,1095p' Docs/setupdirections.md | grep -c 'mcp__unreal_editor__\*'   → 0
sed -n '1007,1095p' Docs/setupdirections.md | grep -c 'unreal_editor__\*'        → 0
```
Whole-file hits of the token are exactly the two PRE-EXISTING prose sites (`:896` the §11.5 ⛔ ban, `:972` the Appendix B nit QA-1238 already ruled a namespace reference) — neither is inside the JSON block, per the spec ("the §11.5 prose that BANS it is not a hit"). Also asserted in the edit script before the write (`"mcp__unreal_editor__*" not in names`) and by parsing the block after (below).

## What was changed — Appendix D.1 + D.4 ONLY

### D.1 (`### D.1 The permanent allow-list`)
- `"mcp__claude_ai_Slack__slack_read_channel"` gained its trailing comma (it was the last entry).
- Appended, under a 7-line `//` comment naming the read-only / enumerated split and citing `handoffs/TASK-1222-buildmaster.md`:
  - `"mcp__unreal_inspector__*"` — wholesale, as granted.
  - **32** `"mcp__unreal_editor__<name>"` entries, one per line, alphabetical = the handoff's order.
- `"enabledMcpjsonServers": ["unreal-mcp", "blender"]` → `["unreal-mcp", "blender", "unreal_inspector", "unreal_editor"]` (line 1093).

### D.4 (`### D.4 Adjacent grants that are not permission rules`)
- The "Local MCP servers" bullet (now line 1120) names the four servers, points at Chapters 4–5, 11, says the two Aura stdio servers live in the project `.mcp.json` (§11.4), and describes `unreal_inspector` as read-only/wholesale and `unreal_editor` as the mutating server (PIE/verify/screenshot bundled with C++ authoring, Live Coding compile, shell) allowed only by D.1's enumerated names — never by wildcard (§11.5). Same description §11.4 step 3 and §11.5 already give.

⛔ No other chapter, appendix, list row, or table touched. §11.4 / §11.5 themselves are NOT edited — their pointers now simply resolve.

## Acceptance (2) — hunk list vs HEAD (`git diff HEAD -U0`)
```
@@ -1049 +1049,42 @@   D.1: Slack entry comma + comment + inspector wholesale + 32 editor names (41 inserted, 1 changed)
@@ -1052 +1093    @@   D.1: enabledMcpjsonServers two → four servers (1 changed)
@@ -1079 +1120    @@   D.4: the "Local MCP servers" bullet (1 changed)
git diff --stat HEAD → Docs/setupdirections.md | 47 +++++++++++++++++++++++++++--   (44 insertions, 3 deletions)
```
Three hunks, all between `### D.1` (line 1003) and `### D.5` (was 1083, now 1124). Nothing above line 1049 or in D.2 / D.3 / D.5.

## Acceptance (3) — the 32 pairs (D.1 block ↔ `handoffs/TASK-1222-buildmaster.md` fenced list)
The names were EXTRACTED programmatically from the handoff's fenced block under "Enumerated `unreal_editor`" (regex on the fence, not typed), asserted `len == 32` and `fullmatch mcp__unreal_editor__[a-z_]+`, then written. Re-measured after the write: `grep -o` of the D.1 block sorted vs the handoff block sorted → `diff` empty → **IDENTICAL sets, 32 = 32**. Each pair is (D.1 entry, handoff line) with the same string:

| # | D.1 entry == handoff line |
|---|---|
| 1 | `mcp__unreal_editor__attach_pie_frames` |
| 2 | `mcp__unreal_editor__capture_pie_frame` |
| 3 | `mcp__unreal_editor__get_actor_by_name_in_pie` |
| 4 | `mcp__unreal_editor__get_actor_property_in_pie` |
| 5 | `mcp__unreal_editor__get_input_mapping_context_keys` |
| 6 | `mcp__unreal_editor__get_player_transform` |
| 7 | `mcp__unreal_editor__get_screenshot_of_objects_for_verification` |
| 8 | `mcp__unreal_editor__get_widget_property_in_pie` |
| 9 | `mcp__unreal_editor__inject_input_action` |
| 10 | `mcp__unreal_editor__is_pie_active` |
| 11 | `mcp__unreal_editor__load_level` |
| 12 | `mcp__unreal_editor__record_burst` |
| 13 | `mcp__unreal_editor__run_verification_sequence` |
| 14 | `mcp__unreal_editor__set_player_transform` |
| 15 | `mcp__unreal_editor__simulate_button_press` |
| 16 | `mcp__unreal_editor__simulate_key_press` |
| 17 | `mcp__unreal_editor__simulate_left_stick` |
| 18 | `mcp__unreal_editor__simulate_right_stick` |
| 19 | `mcp__unreal_editor__start_pie` |
| 20 | `mcp__unreal_editor__start_pie_recording` |
| 21 | `mcp__unreal_editor__start_state_recording` |
| 22 | `mcp__unreal_editor__stop_pie` |
| 23 | `mcp__unreal_editor__stop_pie_recording` |
| 24 | `mcp__unreal_editor__stop_state_recording` |
| 25 | `mcp__unreal_editor__survey_pie_scene` |
| 26 | `mcp__unreal_editor__take_editor_screenshot` |
| 27 | `mcp__unreal_editor__ui_perform` |
| 28 | `mcp__unreal_editor__ui_snapshot` |
| 29 | `mcp__unreal_editor__ui_wait_for` |
| 30 | `mcp__unreal_editor__verification_agent` |
| 31 | `mcp__unreal_editor__wait_pie_frames` |
| 32 | `mcp__unreal_editor__wait_pie_seconds` |

Source = the GRANT (`TASK-1222` handoff), never `handoffs/AURA-MCP-CENSUS.md` directly — the census was not opened.

**Block parses:** comment-stripped `json.loads` of lines 1008–1094 succeeds; `permissions.allow` = 60 entries (27 pre-existing + inspector wholesale + 32 editor names); `unreal_editor` entries = 32; wildcard present = `False`; `enabledMcpjsonServers` = the four names.

## Acceptance (4) — the pointers resolve
§11.4 step 3 (`:865` "…added to `enabledMcpjsonServers` in `.claude/settings.local.json` (Appendix D)") and §11.5's heading (`:891` "(Appendix D — merge, never replace)") + its first two bullets now land on a D.1 that shows the four-server line and the wholesale-inspector / enumerated-editor allow-list, and a D.4 that describes the same split in the same words — no contradiction remains between Chapter 11 and Appendix D.

## Acceptance (5) — no secrets
Case-insensitive grep of the ADDED diff lines for `service_role|api[_ ]key|token=|sbp_|eyJ|hf_…|ghp_|sk-…|Bearer |access_token` → **1 hit, a FALSE POSITIVE**: `sk-[A-Za-z0-9]` matches the `SK-1` inside the word `TASK-1222` on my comment line (`// TASK-1222 granted it (handoffs/TASK-1222-buildmaster.md):`). Case-sensitive it is 0. The additions are tool-name strings, a server list, and prose; no key, token, path outside the engine install, or tokenised URL.

## What QA should scrutinize
- That the 32 names in D.1 are the handoff's list and not the census's §4.5 (they are the same 32 strings by TASK-1222's own assertion, but the spec names the handoff as the source — I read only the handoff).
- The `jsonc` block is illustrative (`<python>`, `<repo>` placeholders pre-exist in it); the parse check above stripped `//` comments only.
- Hunk (1) touches the pre-existing Slack line for its comma — that is the one changed pre-existing line inside D.1, required by JSON syntax.

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · no `Saved/` write · no `.mcp.json` / `settings.local.json` / agent-file edit · Chapter 11 not edited · only the `TASK-1243` `- status:` line edited on the board · the vault twin written by `cp` of the finished repo file (write once, copy).

## Note for build-master (TASK-1242)
Commit `Docs/setupdirections.md` ONLY; re-measure both hashes before the commit — expected `208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58` / 69,979 bytes on both. A mismatch means a copy drifted after this handoff — route back, do not "fix" by copying. Git's `core.autocrlf` warning ("LF will be replaced by CRLF") is the pre-existing repo setting, same as at TASK-1228; the working file is LF.
