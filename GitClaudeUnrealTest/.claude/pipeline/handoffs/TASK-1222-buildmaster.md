# TASK-1222 — [AURA-PERMISSIONS] build-master handoff (2026-09-13)

**Status: done (gate WAIVED per the board — gitignored permissions file + a verbatim census copy; the live test is `TASK-1224`'s agent loading and `TASK-1230`'s run). ⛔ NOT committed — `settings.local.json` is gitignored and never staged; `AURA-PHASE0.md` is committed by `TASK-1242`.**

## Acceptance (1) — FIRST, the wildcard line
```
grep -c 'mcp__unreal_editor__\*' .claude/settings.local.json
0
```
Also asserted inside the merge script before the write (`any(x == 'mcp__unreal_editor__*' …)` → False) and re-grepped after.

## What was written
1. `GitClaudeUnrealTest/.claude/settings.local.json` (⛔ gitignored — `git check-ignore -v` → `GitClaudeUnrealTest/.gitignore:16`; `git status --porcelain | grep -i settings` → empty). `permissions.allow` MERGED (Python `list.extend` on the parsed array — never replaced): **count before = 45, after = 78, additions = 33** (45 = 78 − 33). Index 0 and index 44 (the last pre-existing entry) unchanged; new entries occupy indices 45–77. `enabledMcpjsonServers` untouched = `["unreal-mcp","blender","unreal_inspector","unreal_editor"]`. File re-serialised with 2-space indent (the file's existing style), LF, no BOM, 4426 bytes.
2. `GitClaudeUnrealTest/.claude/pipeline/qa/AURA-PHASE0.md` → `## MCP tool census` filled: the granted allow-list · the §4.6 decisions · the §2a finding as an OPEN MANAGER RULING (verbatim) · a byte-exact copy of `handoffs/AURA-MCP-CENSUS.md` between `<!-- BEGIN/END verbatim copy … -->` markers. The two other sections (`## The three cases` etc. from `TASK-1220`, `## Tier`) untouched — except one word: `Nothing about credit` → `Nothing concerning credit` in the `TASK-1220` prose, because `TASK-1220`'s acceptance (1) bans the word "about" and my own grep found one non-numeric occurrence; recorded here so the edit is not silent.
3. This handoff.

## The 33 additions (verbatim, as written into `permissions.allow`)

Wholesale (plan item 7, `VER-§7` cl. 3):
```
mcp__unreal_inspector__*
```

Enumerated `unreal_editor` — 32 names = census §4.5 EXACTLY (6 PIE lifecycle + 10 verification + 9 screenshot/recording + 7 input simulation). **`TASK-1224` copies this list verbatim into the verifier's `tools:` line:**
```
mcp__unreal_editor__attach_pie_frames
mcp__unreal_editor__capture_pie_frame
mcp__unreal_editor__get_actor_by_name_in_pie
mcp__unreal_editor__get_actor_property_in_pie
mcp__unreal_editor__get_input_mapping_context_keys
mcp__unreal_editor__get_player_transform
mcp__unreal_editor__get_screenshot_of_objects_for_verification
mcp__unreal_editor__get_widget_property_in_pie
mcp__unreal_editor__inject_input_action
mcp__unreal_editor__is_pie_active
mcp__unreal_editor__load_level
mcp__unreal_editor__record_burst
mcp__unreal_editor__run_verification_sequence
mcp__unreal_editor__set_player_transform
mcp__unreal_editor__simulate_button_press
mcp__unreal_editor__simulate_key_press
mcp__unreal_editor__simulate_left_stick
mcp__unreal_editor__simulate_right_stick
mcp__unreal_editor__start_pie
mcp__unreal_editor__start_pie_recording
mcp__unreal_editor__start_state_recording
mcp__unreal_editor__stop_pie
mcp__unreal_editor__stop_pie_recording
mcp__unreal_editor__stop_state_recording
mcp__unreal_editor__survey_pie_scene
mcp__unreal_editor__take_editor_screenshot
mcp__unreal_editor__ui_perform
mcp__unreal_editor__ui_snapshot
mcp__unreal_editor__ui_wait_for
mcp__unreal_editor__verification_agent
mcp__unreal_editor__wait_pie_frames
mcp__unreal_editor__wait_pie_seconds
```

## Acceptance (2) — every granted `unreal_editor` name appears verbatim in the census (the pairs)
The 32 names were EXTRACTED programmatically from the census §4.5 fenced block (not typed), then each asserted present in the census §3 114-name verbatim block (`missing = []`). So every pair is (granted name, census §3 line = census §4.5 line), identical string:

| granted name | census location |
|---|---|
| `mcp__unreal_editor__attach_pie_frames` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__capture_pie_frame` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__get_actor_by_name_in_pie` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__get_actor_property_in_pie` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__get_input_mapping_context_keys` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__get_player_transform` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__get_screenshot_of_objects_for_verification` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__get_widget_property_in_pie` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__inject_input_action` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__is_pie_active` | §3 + §4.5 (§4.1 PIE) |
| `mcp__unreal_editor__load_level` | §3 + §4.5 (§4.1 PIE) |
| `mcp__unreal_editor__record_burst` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__run_verification_sequence` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__set_player_transform` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__simulate_button_press` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__simulate_key_press` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__simulate_left_stick` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__simulate_right_stick` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__start_pie` | §3 + §4.5 (§4.1 PIE) |
| `mcp__unreal_editor__start_pie_recording` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__start_state_recording` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__stop_pie` | §3 + §4.5 (§4.1 PIE) |
| `mcp__unreal_editor__stop_pie_recording` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__stop_state_recording` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__survey_pie_scene` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__take_editor_screenshot` | §3 + §4.5 (§4.3 screenshot) |
| `mcp__unreal_editor__ui_perform` | §3 + §4.5 (§4.4 input) |
| `mcp__unreal_editor__ui_snapshot` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__ui_wait_for` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__verification_agent` | §3 + §4.5 (§4.2 verification) |
| `mcp__unreal_editor__wait_pie_frames` | §3 + §4.5 (§4.1 PIE) |
| `mcp__unreal_editor__wait_pie_seconds` | §3 + §4.5 (§4.1 PIE) |

## Excluded by name (spec: "anything the census shows as C++ authoring / compile / Live Coding / shell / git")
All 82 census §5 names are outside the grant. Asserted by name in the script for the sharpest ten: `edit_cpp_file`, `generate_cpp_file`, `trigger_live_coding`, `execute_unreal_python`, `python_agent`, `create_text_file`, `edit_text_file`, `pie_scene_edit`, `create_easy_level_for_verification`, `spawn_blueprint_actors` — none present. Measured after the write: `grep -c '"mcp__unreal_editor__' settings.local.json` = 32 = the §4.5 count, so nothing outside §4.5 slipped in.

## Census §4.6 judgment calls — overrules: NONE
Every §4.6 call inherited as classed, each with the reason recorded in `AURA-PHASE0.md`:
- `pie_scene_edit` — NOT granted. Reason: live PIE-world mutation; `VER-§7` cl. 4 gives the verifier no mutation surface, and the pilot's three cases need no staging.
- `create_easy_level_for_verification`, `spawn_blueprint_actors` — NOT granted (census: EXCLUDE; they write `Content/` / dirty the level).
- profiler ×4 — NOT granted (outside the four classes; not needed for a `VER-§` verdict).
- `load_level`, `set_player_transform`, `verification_agent`, `get_input_mapping_context_keys` — granted as classed. `load_level` rider stands: the verifier never passes `discard_unsaved` without saying so on its report line.

## ⚠️ Census §2a — recorded verbatim, OPEN MANAGER RULING (not decided here)
The spec says grant `mcp__unreal_inspector__*` wholesale; that is what was written, NOT narrowed. The census §2a finding, verbatim:

> The plan grants it wholesale on the strength of its name. The census finds these non-read tools inside it, recorded for `TASK-1222` (the grant) and for the manager (a ruling may be owed, `SC-§101`):
>
> - **engine lifecycle (never invoked by QA per qa-reviewer.md TASK-1227; the verifier has no reason to either):** `launch_unreal_project`, `recompile_unreal_project`, `shutdown_headless`, `cancel_operation`
> - **generation / image / mesh (writes files under Saved/ or the Aura cache, not Content/; still not read-only):** `generate_images`, `generate_model_from_image`, `generate_model_from_text`, `edit_images`, `derive_normal_map`, `rig_model_from_mesh`, `get_recent_generated_images`
> - **plan bookkeeping (Aura-internal):** `create_or_edit_plan`, `lock_plan_layer`
>
> `recompile_unreal_project` is a COMPILE PATH under a wholesale grant. The body law already says compilation belongs to build-master via Build.bat (`Docs/AuraProjectMemory.md` law 1; the `qa-reviewer.md` TASK-1227 paragraph), but a wildcard means the harness will not stop an agent that forgets. The census flags it; it does not decide it.

**Ruling owed (manager):** keep the wholesale inspector grant with the body law as the fence, or replace it with an enumerated inspector list minus the 13 names above. Same finding recorded in `AURA-PHASE0.md` → `## MCP tool census`.

## Acceptance summary
1. `grep -c 'mcp__unreal_editor__\*' .claude/settings.local.json` = **0** ✅
2. Every granted `unreal_editor` name verbatim in the census — 32/32, pairs above, `missing = []` ✅
3. Pre-existing entries all present: before **45** = after **78** − additions **33** ✅ (idx 0 / idx 44 spot-read unchanged)
4. JSON parses: `json.load` SUCCESS after write; `enabledMcpjsonServers` = the four names ✅
5. `AURA-PHASE0.md` census section filled; the census copy region sha256 `23d376fad53330b6359762f8632d6c530f4e4202e7605ef03a2983c76208910f` == source file sha256 (byte-identical, `reg == census: True`); `## The three cases` table still 3 rows; `## Tier` heading + its filler line intact and positioned after the copy ✅ (the one-word "about" edit declared above)
6. No agent `tools:` line edited (`playtest-verifier.md` untouched — `TASK-1224`) ✅

## Fences honoured
No git add / commit / push · no editor / MCP engine call · no `Saved/` write · no `.mcp.json` edit · no agent file edit · only the `TASK-1222` `status:` line edited on the board.

## Note for the orchestrator
Claude Code reads `settings.local.json` permissions at session start / on `/permissions` reload; a running session may not see the 33 new grants until reloaded. `TASK-1224`'s verifier `tools:` line = `Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, <the 32 names above>, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_thread`.
