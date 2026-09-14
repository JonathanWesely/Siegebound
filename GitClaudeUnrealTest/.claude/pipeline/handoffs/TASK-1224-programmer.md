# TASK-1224 — [AURA-VERIFIER-FINAL] gameplay-programmer handoff (2026-09-13)

**Status: ready-for-qa (gate `TASK-1235`, host `TASK-1242`). ⛔ Not committed, not staged — `.claude/agents/playtest-verifier.md` is still `??` (untracked) by design.**

## Acceptance (1) — FIRST, the wildcard line
```
grep -c 'unreal_editor__\*' .claude/agents/playtest-verifier.md
0
```
Measured before the edit (0) and after (0), by `grep` on the written file AND by a byte-count inside the edit script (`raw.count(b'unreal_editor__*')` = 0).

## What changed — exactly one line
File: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\agents\playtest-verifier.md`, frontmatter line 4 (`tools:`). Nothing else in the file was touched (see acceptance (4)).

**Old line (the TASK-1223 placeholder, verbatim):**
```
tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_thread  # ⚠️ PLACEHOLDER — unreal_editor PIE/verify/screenshot tools pending the /mcp census (TASK-1224)
```
(Note for QA: the dispatch prompt quoted the placeholder as `tools: ⚠️ tool names pending /mcp census — finalized by TASK-1224`; the file on disk carried the line above instead — same intent, different text. I replaced what was on disk. The trailing `# ⚠️ PLACEHOLDER …` comment is gone with it — a YAML frontmatter `tools:` line should carry only names.)

**New line (verbatim):**
```
tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, mcp__unreal_editor__attach_pie_frames, mcp__unreal_editor__capture_pie_frame, mcp__unreal_editor__get_actor_by_name_in_pie, mcp__unreal_editor__get_actor_property_in_pie, mcp__unreal_editor__get_input_mapping_context_keys, mcp__unreal_editor__get_player_transform, mcp__unreal_editor__get_screenshot_of_objects_for_verification, mcp__unreal_editor__get_widget_property_in_pie, mcp__unreal_editor__inject_input_action, mcp__unreal_editor__is_pie_active, mcp__unreal_editor__load_level, mcp__unreal_editor__record_burst, mcp__unreal_editor__run_verification_sequence, mcp__unreal_editor__set_player_transform, mcp__unreal_editor__simulate_button_press, mcp__unreal_editor__simulate_key_press, mcp__unreal_editor__simulate_left_stick, mcp__unreal_editor__simulate_right_stick, mcp__unreal_editor__start_pie, mcp__unreal_editor__start_pie_recording, mcp__unreal_editor__start_state_recording, mcp__unreal_editor__stop_pie, mcp__unreal_editor__stop_pie_recording, mcp__unreal_editor__stop_state_recording, mcp__unreal_editor__survey_pie_scene, mcp__unreal_editor__take_editor_screenshot, mcp__unreal_editor__ui_perform, mcp__unreal_editor__ui_snapshot, mcp__unreal_editor__ui_wait_for, mcp__unreal_editor__verification_agent, mcp__unreal_editor__wait_pie_frames, mcp__unreal_editor__wait_pie_seconds, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_thread
```
Shape = `Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, <32 unreal_editor names, sorted, one per name>, slack_send_message, slack_read_thread`. Fences measured on the written line: `Bash` = 0 · `unreal-mcp` = 0 · `blender` = 0 · `unreal_editor__*` = 0.

## How the 32 names were produced (not typed)
Script: scratchpad `task1224_tools_line.py` (session-temp, not in the repo). It
1. parsed census `## 3.` → 114 `mcp__unreal_editor__<name>` lines (unique = 114),
2. parsed census `### 4.5` → 32 names (unique = 32),
3. parsed `handoffs/TASK-1222-buildmaster.md` `## The 33 additions` fenced block → 32 `unreal_editor` names (unique = 32) — **this is the source of the line**, sorted,
4. asserted the sets, then replaced only the frontmatter `tools:` line via a byte-level split on the file's own EOL (LF, no BOM), writing every other byte back unchanged.

## Acceptance (2) — every `mcp__unreal_editor__<name>` in the line appears verbatim in census §3
Programmatic: `sorted(set(granted) - set(census_§3_names))` → `[]`. Each pair is (name in the tools line, identical string on a census §3 line); class per census §4.1–4.4:

| # | name in `tools:` line | census |
|---|---|---|
| 1 | `mcp__unreal_editor__attach_pie_frames` | §3 · §4.5 (§4.3 screenshot/recording) |
| 2 | `mcp__unreal_editor__capture_pie_frame` | §3 · §4.5 (§4.3) |
| 3 | `mcp__unreal_editor__get_actor_by_name_in_pie` | §3 · §4.5 (§4.2 verification) |
| 4 | `mcp__unreal_editor__get_actor_property_in_pie` | §3 · §4.5 (§4.2) |
| 5 | `mcp__unreal_editor__get_input_mapping_context_keys` | §3 · §4.5 (§4.2) |
| 6 | `mcp__unreal_editor__get_player_transform` | §3 · §4.5 (§4.2) |
| 7 | `mcp__unreal_editor__get_screenshot_of_objects_for_verification` | §3 · §4.5 (§4.3) |
| 8 | `mcp__unreal_editor__get_widget_property_in_pie` | §3 · §4.5 (§4.2) |
| 9 | `mcp__unreal_editor__inject_input_action` | §3 · §4.5 (§4.4 input) |
| 10 | `mcp__unreal_editor__is_pie_active` | §3 · §4.5 (§4.1 PIE) |
| 11 | `mcp__unreal_editor__load_level` | §3 · §4.5 (§4.1) |
| 12 | `mcp__unreal_editor__record_burst` | §3 · §4.5 (§4.3) |
| 13 | `mcp__unreal_editor__run_verification_sequence` | §3 · §4.5 (§4.2) |
| 14 | `mcp__unreal_editor__set_player_transform` | §3 · §4.5 (§4.4) |
| 15 | `mcp__unreal_editor__simulate_button_press` | §3 · §4.5 (§4.4) |
| 16 | `mcp__unreal_editor__simulate_key_press` | §3 · §4.5 (§4.4) |
| 17 | `mcp__unreal_editor__simulate_left_stick` | §3 · §4.5 (§4.4) |
| 18 | `mcp__unreal_editor__simulate_right_stick` | §3 · §4.5 (§4.4) |
| 19 | `mcp__unreal_editor__start_pie` | §3 · §4.5 (§4.1) |
| 20 | `mcp__unreal_editor__start_pie_recording` | §3 · §4.5 (§4.3) |
| 21 | `mcp__unreal_editor__start_state_recording` | §3 · §4.5 (§4.3) |
| 22 | `mcp__unreal_editor__stop_pie` | §3 · §4.5 (§4.1) |
| 23 | `mcp__unreal_editor__stop_pie_recording` | §3 · §4.5 (§4.3) |
| 24 | `mcp__unreal_editor__stop_state_recording` | §3 · §4.5 (§4.3) |
| 25 | `mcp__unreal_editor__survey_pie_scene` | §3 · §4.5 (§4.2) |
| 26 | `mcp__unreal_editor__take_editor_screenshot` | §3 · §4.5 (§4.3) |
| 27 | `mcp__unreal_editor__ui_perform` | §3 · §4.5 (§4.4) |
| 28 | `mcp__unreal_editor__ui_snapshot` | §3 · §4.5 (§4.2) |
| 29 | `mcp__unreal_editor__ui_wait_for` | §3 · §4.5 (§4.2) |
| 30 | `mcp__unreal_editor__verification_agent` | §3 · §4.5 (§4.2) |
| 31 | `mcp__unreal_editor__wait_pie_frames` | §3 · §4.5 (§4.1) |
| 32 | `mcp__unreal_editor__wait_pie_seconds` | §3 · §4.5 (§4.1) |

None of the census §5 names (C++ authoring / compile / Live Coding / Python-shell / Blueprint authoring / asset authoring / staging / profiling) is in the line; asserted by name for the sharpest ten (`edit_cpp_file`, `generate_cpp_file`, `trigger_live_coding`, `execute_unreal_python`, `python_agent`, `create_text_file`, `edit_text_file`, `pie_scene_edit`, `create_easy_level_for_verification`, `spawn_blueprint_actors`) — none present.

## Acceptance (3) — set equality with TASK-1222's allow-list, quoted
```
set(tools line unreal_editor) == set(TASK-1222 granted): True
set(tools line unreal_editor) == set(census §4.5):        True
in-line-not-allowed (would prompt forever): []
allowed-not-in-line (fine):                 []
```
32 = 32; the line and `permissions.allow` name the same 32 tools. Independent re-count on the written file: `grep -o 'mcp__unreal_editor__[a-z_]*' | wc -l` on the `tools:` line = 32.

## Acceptance (4) — body unchanged vs TASK-1223
sha256 of the file with the single `^tools: ` line removed (`grep -v '^tools: ' file | sha256sum`), measured independently of the edit script:
```
BEFORE 32eb680f0eef6067f0157e2b9f5ef7e026bd7327923b3fc9a8a8b35f8f3ff36e
AFTER  32eb680f0eef6067f0157e2b9f5ef7e026bd7327923b3fc9a8a8b35f8f3ff36e   (equal)
```
The script's own in-memory before/after computed the same digest (`equal=True`). Line count unchanged (96 per `wc -l`). Whole-file sha256 moved `8a03b359782db0306543a45a2faaf6dbae44701034de0996c4e064ea5260f836` → `0d365dd95e6c39ab93267120f5376b14334f70195ca6e3b05af78f3b07416449` — the one line.

## Acceptance (5) — still untracked
```
git status --porcelain -- .claude/agents/playtest-verifier.md
?? GitClaudeUnrealTest/.claude/agents/playtest-verifier.md
```
Same `??` before and after. No `git add`, no commit, no push.

## For QA to scrutinize
- The frontmatter is a comma-separated `tools:` list, the same shape the other six agent files use; if the harness needs a YAML sequence instead, that is a TASK-1223 body question, not this row's — I kept the existing shape.
- Whether the harness honours `mcp__unreal_inspector__*` as a glob in an agent `tools:` line is the open census §2a ruling (TASK-1222 handoff) — unchanged here, as specced.
- The live test is the agent actually loading with these 32 names (TASK-1230's run) — a name that never resolves would surface as a permission prompt, which acceptance (3) is designed to preclude.

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · no other file edited · only the TASK-1224 `status:` line edited on the board · Slack post in ⚙️ Dev & QA (`1783116269.740549`), wildcard grep = 0 line first.
