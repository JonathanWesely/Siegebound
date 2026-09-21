# AURA-PHASE0 — the measurement record of Jonathan's three verification cases

**Written by:** build-master, 2026-09-13, under `TASK-1220` (plan Part B item 5). **Law:** `SC-§97` (cl. 3 labels) · `VER-§6` cl. 2 (the pilot = these three cases). **This file RECORDS; it DECIDES NOTHING.** No agent-derived number appears below — every measurement is copied from Jonathan's own words as relayed on `TASK-1215`'s status line (orchestrator relay of his Aura-chat session, 2026-09-13). A column he did not supply reads `OWED`, never an estimate.

**Source of every value:** `TASKBOARD.md` → `TASK-1215-AURA-GATE-P0` → `status:` line (stage A discharge, 2026-09-13). The three cases are HIS OWN PROBES run in Aura's own chat — ⚠️ they are NOT the plan's three `qa-passed` board rows (the status line says so), hence the `TASK-###` column reads `n/a (his probe)` for all three.

## The three cases

| # | `TASK-###` | expected outcome (his words) | Aura's verdict | match Y/N | credit consumed | wall time | evidence he mentioned |
|---|---|---|---|---|---|---|---|
| 1 | n/a (his probe — not a board row) | "tell me what the cost of an archer card is" — he knew the answer beforehand; correctness judged by him | "12 gold + other card info" — MEASURED BY JONATHAN (Aura chat, 2026-09-13) | **Y** — "judged correct by him" — MEASURED BY JONATHAN (Aura chat, 2026-09-13) | OWED (per-case $-credit not supplied; see the total below) | OWED | none — STATIC (no PIE); the answer text in the chat |
| 2 | n/a (his probe — not a board row) | sprint time, castle entrance → centre spawn — he knew the answer beforehand; correctness judged by him | "28.4 s, computed by Aura from the numbers" — MEASURED BY JONATHAN (Aura chat, 2026-09-13) | **Y** — "judged correct" — MEASURED BY JONATHAN (Aura chat, 2026-09-13) | OWED | OWED | none — STATIC (no PIE); Aura computed from project numbers, no run |
| 3 | n/a (his probe — not a board row) | "run an instance of the game where the playable character runs towards the middle and does a spin" | Aura launched PIE, drove the hero to the middle, spun, and pasted a VIDEO RECORDING in the chat — "he WATCHED it" — MEASURED BY JONATHAN (Aura chat, 2026-09-13) | **Y** — he watched the run do what he asked — MEASURED BY JONATHAN (Aura chat, 2026-09-13) | OWED | OWED | a VIDEO RECORDING pasted into the Aura chat (machine-local / chat-local; ⛔ not promoted — `VER-§4` cl. 3 video is never promoted) |

**Row count:** 3 (acceptance (2) — no more).

**What case 3 proves, in his words' scope only:** INPUT SIMULATION + PIE + VIDEO are PROVEN LIVE on this machine — MEASURED BY JONATHAN (Aura chat, 2026-09-13). Nothing concerning credit, wall time, or which input layer was driven is proven by it (see the next two sections).

## Totals he supplied

- **Context gauge, all three cases together:** "18% used context (90k of 500k tokens)" — MEASURED BY JONATHAN (Aura chat, 2026-09-13). ⚠️ **This is Aura's CONTEXT-WINDOW gauge, not a premium-credit dollar figure.** It is recorded here as a context-window reading and nowhere in this file is it treated as credit.
- **$-credit burn (the plan's "credit consumed" column):** OWED — to be read by him from the Aura account/usage page before `TASK-1215` stage B. No per-case split exists.
- **Wall time per case:** OWED — not supplied.
- **Training:** ON (kept) — MEASURED BY JONATHAN (Aura chat, 2026-09-13).

## Enhanced-Input finding (`KBD-§` positional layout)

- **Question the plan asks:** did Aura's input simulation drive OUR Enhanced Input mappings via the positional `KBD-§` layout, or only default bindings?
- **His observation, verbatim from the status line:** NOT YET SUPPLIED — "whether case (3) used the positional KBD-§ layout" is listed on `TASK-1215` under *NOT YET SUPPLIED*.
- **Recorded value:** OWED. Case 3 DID move the hero and spin it (his observation), so SOME input path reached the pawn; which layer (Enhanced Input `IMC_Hero` through the `KBD-§` layout vs. a direct transform / default binding) is UNMEASURED. ⛔ Not inferred here.

## MCP tool census

**Filled by `TASK-1222` (build-master, 2026-09-13) from `handoffs/AURA-MCP-CENSUS.md`** (orchestrator's `/mcp` census, `TASK-1216` stage B). This section shows WHAT WAS ENUMERABLE (the verbatim census copy below) and WHAT WAS GRANTED (the allow-list). ⛔ The census is a name list, not a grant; the grant is what `TASK-1222` wrote into `.claude/settings.local.json` → `permissions.allow` (gitignored, never staged).

### What TASK-1222 granted — `permissions.allow` additions, 33 entries (merged, array never replaced; count 45 → 78)

1 wholesale entry, per plan item 7 / `VER-§7` cl. 3:

```
mcp__unreal_inspector__*
```

32 enumerated `unreal_editor` entries = census §4.5 EXACTLY (PIE lifecycle 6 + verification 10 + screenshot/recording 9 + input simulation 7), one entry per tool, ⛔ no `mcp__unreal_editor__*` wildcard (`grep -c 'mcp__unreal_editor__\*' .claude/settings.local.json` = 0 at write time):

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

**Census §4.6 judgment calls — TASK-1222's decisions:** none of the eight judgment calls was overruled. Specifically: `pie_scene_edit` NOT granted (a live-world mutation; `VER-§7` cl. 4 forbids the verifier any mutation surface, and no `VER-§1` verdict needs staging in the pilot) · `create_easy_level_for_verification` and `spawn_blueprint_actors` NOT granted (census recommends EXCLUDE — they write `Content/` / dirty the level) · the four profiler tools NOT granted (outside the four classes, not needed for a `VER-§` verdict) · `load_level`, `set_player_transform`, `verification_agent`, `get_input_mapping_context_keys` granted as the census classed them. Every one of the 82 §5 names stays outside the grant.

### ⚠️ OPEN MANAGER RULING (census §2a, recorded verbatim; `SC-§101`) — `unreal_inspector` is NOT purely read-only

The spec of `TASK-1222` says grant `mcp__unreal_inspector__*` wholesale, and that is what was written — the grant was NOT narrowed by build-master. The census finding is copied here so the record shows the grant was made with this known:

> The plan grants it wholesale on the strength of its name. The census finds these non-read tools inside it, recorded for `TASK-1222` (the grant) and for the manager (a ruling may be owed, `SC-§101`):
>
> - **engine lifecycle (never invoked by QA per qa-reviewer.md TASK-1227; the verifier has no reason to either):** `launch_unreal_project`, `recompile_unreal_project`, `shutdown_headless`, `cancel_operation`
> - **generation / image / mesh (writes files under Saved/ or the Aura cache, not Content/; still not read-only):** `generate_images`, `generate_model_from_image`, `generate_model_from_text`, `edit_images`, `derive_normal_map`, `rig_model_from_mesh`, `get_recent_generated_images`
> - **plan bookkeeping (Aura-internal):** `create_or_edit_plan`, `lock_plan_layer`
>
> `recompile_unreal_project` is a COMPILE PATH under a wholesale grant. The body law already says compilation belongs to build-master via Build.bat (`Docs/AuraProjectMemory.md` law 1; the `qa-reviewer.md` TASK-1227 paragraph), but a wildcard means the harness will not stop an agent that forgets. The census flags it; it does not decide it.

**Ruling owed (manager):** keep the wholesale `mcp__unreal_inspector__*` (body law as the fence) OR replace it with an enumerated inspector list excluding the 13 names above. Until ruled, the wholesale grant stands as the spec ordered.

*RULED R10 2026-09-13 — enumerated, not wholesale; the 49 granted `unreal_inspector` names are in `handoffs/TASK-1254-buildmaster.md`, the 13 excluded are census §2a.*

### Verbatim copy of `handoffs/AURA-MCP-CENSUS.md` (sha256 of the source file at copy time: `23d376fad53330b6359762f8632d6c530f4e4202e7605ef03a2983c76208910f`)

The region between the two HTML-comment markers is a byte-exact copy of the census file (its own `##` headings included, undemoted — "verbatim" is the law here). The `## Tier` heading of THIS file follows the END marker.

<!-- BEGIN verbatim copy of handoffs/AURA-MCP-CENSUS.md -->
# AURA-MCP-CENSUS — the `/mcp` tool-name census (TASK-1216 stage B)

**Written by:** orchestrator (main session), 2026-09-13, after a full Claude Code restart on the wave-1 commits (HEAD `11e9ea2` + the uncommitted `.mcp.json` from TASK-1221).
**Law:** TASK-1216 spec stage B · plan *Corrections* rows 2 + 5 · `SC-§97`. ⛔ **This is a NAME LIST, not a grant.** Nothing is allowed by its presence here; `TASK-1222` decides `permissions.allow`, `TASK-1224` decides the verifier's `tools:` line.

## 1. Four servers connected (plan Verification probe 2)

`claude mcp list` (2026-09-13, this session):

```
unreal-mcp: http://127.0.0.1:8000/mcp (HTTP) - ✔ Connected
blender: C:/Program Files/Blender Foundation/Blender 5.1/5.1/python/bin/python.exe -u -X utf8 .../Tools/blender_mcp_bridge.py - ✔ Connected
unreal_inspector: C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_inspector.py - ✔ Connected
unreal_editor: C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_editor.py - ✔ Connected
```

Live probes, same session (a listed server is not a working one, `SC-§79`):

| Server | Probe | Result |
|---|---|---|
| `unreal_inspector` | `get_headless_status` | `status: editor_connected` |
| `unreal_editor` | `is_pie_active` | `success: true`, `is_active: false`, `recommended_max_pie_instances: 1` (RTX 5070 Laptop GPU, 7891 MB dedicated, 3054 MB available at probe time) |
| `unreal-mcp` | `list_toolsets` | 19 toolsets returned (the editor's :8000 server is up) |
| `blender` | not called | `claude mcp list` shows the bridge Connected; a Blender-side call needs Blender running and is not this row's business |

**Config sources at census time:** project `.mcp.json` carries all four under `mcpServers` (TASK-1221); `.claude/settings.local.json` `enabledMcpjsonServers` = `["unreal-mcp","blender","unreal_inspector","unreal_editor"]`; `~/.claude/mcp.json` ABSENT; `~/.claude.json` user-scope `mcpServers` = `[]`. The one-click's user-scope copy noted on TASK-1216's status line is already gone, so no `claude mcp remove -s user` was needed (measured, not assumed). One source of truth holds.

**Tool-name shape as Claude Code exposes them:** `mcp__<server>__<tool>`, so the Aura names are `mcp__unreal_inspector__<tool>` and `mcp__unreal_editor__<tool>` (server key with the underscore, exactly as keyed in `.mcp.json`).

## 2. `unreal_inspector` — 62 tools, verbatim

```
mcp__unreal_inspector__cancel_operation
mcp__unreal_inspector__create_or_edit_plan
mcp__unreal_inspector__derive_normal_map
mcp__unreal_inspector__edit_images
mcp__unreal_inspector__execute_unreal_python_readonly
mcp__unreal_inspector__fetch_animation_skill
mcp__unreal_inspector__fetch_curve_best_practices
mcp__unreal_inspector__fetch_eqs_best_practices
mcp__unreal_inspector__fetch_gas_best_practices
mcp__unreal_inspector__fetch_level_design_skill
mcp__unreal_inspector__fetch_performance_best_practices
mcp__unreal_inspector__fetch_python_best_practices
mcp__unreal_inspector__fetch_timeline_best_practices
mcp__unreal_inspector__fetch_ui_best_practices
mcp__unreal_inspector__fetch_understandings
mcp__unreal_inspector__generate_images
mcp__unreal_inspector__generate_model_from_image
mcp__unreal_inspector__generate_model_from_text
mcp__unreal_inspector__get_asset_graph
mcp__unreal_inspector__get_asset_meta
mcp__unreal_inspector__get_asset_structs
mcp__unreal_inspector__get_attribute_set
mcp__unreal_inspector__get_available_actors_in_level
mcp__unreal_inspector__get_blueprint_material_properties
mcp__unreal_inspector__get_blueprint_properties_specifiers
mcp__unreal_inspector__get_code_examples
mcp__unreal_inspector__get_enums
mcp__unreal_inspector__get_gameplay_tags
mcp__unreal_inspector__get_headless_status
mcp__unreal_inspector__get_recent_generated_images
mcp__unreal_inspector__get_text_file_contents
mcp__unreal_inspector__get_unreal_context
mcp__unreal_inspector__get_unreal_output_logs
mcp__unreal_inspector__grep
mcp__unreal_inspector__import_ActorComponentsAndSubobjects_understanding
mcp__unreal_inspector__import_AssetCreation_understanding
mcp__unreal_inspector__import_AssetRegistry_understanding
mcp__unreal_inspector__import_AssetType_Blueprint_understanding
mcp__unreal_inspector__import_AssetType_DataTable_understanding
mcp__unreal_inspector__import_AssetType_GameplayEffect_understanding
mcp__unreal_inspector__import_AssetType_Level_understanding
mcp__unreal_inspector__import_AssetType_NiagaraSystem_understanding
mcp__unreal_inspector__import_AssetType_UserWidget_understanding
mcp__unreal_inspector__import_AssetValidation_understanding
mcp__unreal_inspector__import_Color_understanding
mcp__unreal_inspector__import_CurveAsset_understanding
mcp__unreal_inspector__import_FileSystem_understanding
mcp__unreal_inspector__import_IncludeOrImportModules_understanding
mcp__unreal_inspector__import_Logs_understanding
mcp__unreal_inspector__import_PropertyModification_understanding
mcp__unreal_inspector__import_Subsystems_understanding
mcp__unreal_inspector__launch_unreal_project
mcp__unreal_inspector__lock_plan_layer
mcp__unreal_inspector__query_unreal_project_assets
mcp__unreal_inspector__quicksearch
mcp__unreal_inspector__read_datatable_keys
mcp__unreal_inspector__read_datatable_values
mcp__unreal_inspector__recompile_unreal_project
mcp__unreal_inspector__review_blueprint
mcp__unreal_inspector__rig_model_from_mesh
mcp__unreal_inspector__search_geometry_scripts
mcp__unreal_inspector__shutdown_headless
```

### 2a. ⚠️ `unreal_inspector` is NOT purely read-only

The plan grants it wholesale on the strength of its name. The census finds these non-read tools inside it, recorded for `TASK-1222` (the grant) and for the manager (a ruling may be owed, `SC-§101`):

- **engine lifecycle (never invoked by QA per qa-reviewer.md TASK-1227; the verifier has no reason to either):** `launch_unreal_project`, `recompile_unreal_project`, `shutdown_headless`, `cancel_operation`
- **generation / image / mesh (writes files under Saved/ or the Aura cache, not Content/; still not read-only):** `generate_images`, `generate_model_from_image`, `generate_model_from_text`, `edit_images`, `derive_normal_map`, `rig_model_from_mesh`, `get_recent_generated_images`
- **plan bookkeeping (Aura-internal):** `create_or_edit_plan`, `lock_plan_layer`

`recompile_unreal_project` is a COMPILE PATH under a wholesale grant. The body law already says compilation belongs to build-master via Build.bat (`Docs/AuraProjectMemory.md` law 1; the `qa-reviewer.md` TASK-1227 paragraph), but a wildcard means the harness will not stop an agent that forgets. The census flags it; it does not decide it.

## 3. `unreal_editor` — 114 tools, verbatim

```
mcp__unreal_editor__add_blueprint_node_to_strand
mcp__unreal_editor__add_fab_asset_to_project
mcp__unreal_editor__add_input_action_to_mapping_context
mcp__unreal_editor__add_or_replace_rows_in_data_table
mcp__unreal_editor__add_widget_property_binding
mcp__unreal_editor__analyze_profiler_capture
mcp__unreal_editor__attach_pie_frames
mcp__unreal_editor__audio_agent
mcp__unreal_editor__auto_uv
mcp__unreal_editor__autouv_and_retexture
mcp__unreal_editor__bake_all_mesh_textures
mcp__unreal_editor__bake_texture_to_uv_channel
mcp__unreal_editor__bake_texture_transfer
mcp__unreal_editor__behavior_tree_agent
mcp__unreal_editor__bp_agent
mcp__unreal_editor__capture_pie_frame
mcp__unreal_editor__check_uv_mapping
mcp__unreal_editor__collapse_graph
mcp__unreal_editor__compile_blueprint
mcp__unreal_editor__connect_blueprint_nodes
mcp__unreal_editor__create_assets
mcp__unreal_editor__create_blueprint_interface
mcp__unreal_editor__create_easy_level_for_verification
mcp__unreal_editor__create_edit_material
mcp__unreal_editor__create_gameplay_tag
mcp__unreal_editor__create_input_actions
mcp__unreal_editor__create_new_todo_list
mcp__unreal_editor__create_text_file
mcp__unreal_editor__delete_pin_from_tunnel
mcp__unreal_editor__delete_variable
mcp__unreal_editor__disconnect_blueprint_nodes
mcp__unreal_editor__edit_blueprint
mcp__unreal_editor__edit_cpp_file
mcp__unreal_editor__edit_curve
mcp__unreal_editor__edit_data_assets
mcp__unreal_editor__edit_enumeration
mcp__unreal_editor__edit_eqs
mcp__unreal_editor__edit_mesh_postprocessing
mcp__unreal_editor__edit_structure
mcp__unreal_editor__edit_text_file
mcp__unreal_editor__edit_timeline
mcp__unreal_editor__edit_todo_list
mcp__unreal_editor__edit_widget
mcp__unreal_editor__execute_unreal_python
mcp__unreal_editor__fetch_audio_skill
mcp__unreal_editor__fetch_blueprint_best_practices
mcp__unreal_editor__fetch_enhanced_input_skill
mcp__unreal_editor__fetch_material_best_practices
mcp__unreal_editor__fetch_niagara_best_practices
mcp__unreal_editor__fetch_niagara_skill
mcp__unreal_editor__generate_audio
mcp__unreal_editor__generate_cpp_file
mcp__unreal_editor__get_actor_by_name_in_pie
mcp__unreal_editor__get_actor_property_in_pie
mcp__unreal_editor__get_blueprint_component_properties
mcp__unreal_editor__get_blueprint_properties
mcp__unreal_editor__get_data_asset_type_info
mcp__unreal_editor__get_data_asset_types
mcp__unreal_editor__get_input_mapping_context_keys
mcp__unreal_editor__get_player_transform
mcp__unreal_editor__get_python_script_text
mcp__unreal_editor__get_screenshot_of_objects_for_verification
mcp__unreal_editor__get_todo_list
mcp__unreal_editor__get_widget_property_in_pie
mcp__unreal_editor__inject_input_action
mcp__unreal_editor__insert_node_on_exec_wire
mcp__unreal_editor__is_pie_active
mcp__unreal_editor__level_design_agent
mcp__unreal_editor__link_pin_to_tunnel
mcp__unreal_editor__load_level
mcp__unreal_editor__lock_edit_plan
mcp__unreal_editor__lock_vfx_plan
mcp__unreal_editor__material_agent
mcp__unreal_editor__move_node_into_subgraph
mcp__unreal_editor__move_node_outof_subgraph
mcp__unreal_editor__pie_scene_edit
mcp__unreal_editor__python_agent
mcp__unreal_editor__record_burst
mcp__unreal_editor__remove_blueprint_nodes
mcp__unreal_editor__remove_rows_from_data_table
mcp__unreal_editor__rename_blueprint_graph
mcp__unreal_editor__rename_blueprint_node
mcp__unreal_editor__replace_variable_reference
mcp__unreal_editor__run_verification_sequence
mcp__unreal_editor__search_fab
mcp__unreal_editor__search_unreal_python_api
mcp__unreal_editor__set_blueprint_node_properties
mcp__unreal_editor__set_cpu_throttle_override
mcp__unreal_editor__set_node_pins_defaults
mcp__unreal_editor__set_player_transform
mcp__unreal_editor__simulate_button_press
mcp__unreal_editor__simulate_key_press
mcp__unreal_editor__simulate_left_stick
mcp__unreal_editor__simulate_right_stick
mcp__unreal_editor__spawn_blueprint_actors
mcp__unreal_editor__start_pie
mcp__unreal_editor__start_pie_recording
mcp__unreal_editor__start_profiler_capture
mcp__unreal_editor__start_state_recording
mcp__unreal_editor__stop_pie
mcp__unreal_editor__stop_pie_recording
mcp__unreal_editor__stop_profiler_capture
mcp__unreal_editor__stop_state_recording
mcp__unreal_editor__survey_pie_scene
mcp__unreal_editor__take_editor_screenshot
mcp__unreal_editor__trigger_live_coding
mcp__unreal_editor__ui_perform
mcp__unreal_editor__ui_snapshot
mcp__unreal_editor__ui_wait_for
mcp__unreal_editor__uncollapse_subgraph
mcp__unreal_editor__verification_agent
mcp__unreal_editor__vfx_agent
mcp__unreal_editor__wait_pie_frames
mcp__unreal_editor__wait_pie_seconds
```

## 4. Classification of `unreal_editor` — the four classes TASK-1222 / TASK-1224 consume

Every one of the 114 names is placed exactly once (asserted by the generating script). Classification is from each tool's own description as served this session (fetched via ToolSearch), not from the name alone.

### 4.1 PIE lifecycle — 6

- `mcp__unreal_editor__start_pie`
- `mcp__unreal_editor__stop_pie`
- `mcp__unreal_editor__is_pie_active`
- `mcp__unreal_editor__wait_pie_frames`
- `mcp__unreal_editor__wait_pie_seconds`
- `mcp__unreal_editor__load_level`

### 4.2 verification (PIE state reads + the sequence runner) — 10

- `mcp__unreal_editor__get_actor_by_name_in_pie`
- `mcp__unreal_editor__get_actor_property_in_pie`
- `mcp__unreal_editor__get_widget_property_in_pie`
- `mcp__unreal_editor__get_player_transform`
- `mcp__unreal_editor__survey_pie_scene`
- `mcp__unreal_editor__get_input_mapping_context_keys`
- `mcp__unreal_editor__ui_snapshot`
- `mcp__unreal_editor__ui_wait_for`
- `mcp__unreal_editor__run_verification_sequence`
- `mcp__unreal_editor__verification_agent`

### 4.3 screenshot / recording — 9

- `mcp__unreal_editor__capture_pie_frame`
- `mcp__unreal_editor__take_editor_screenshot`
- `mcp__unreal_editor__get_screenshot_of_objects_for_verification`
- `mcp__unreal_editor__start_pie_recording`
- `mcp__unreal_editor__stop_pie_recording`
- `mcp__unreal_editor__record_burst`
- `mcp__unreal_editor__attach_pie_frames`
- `mcp__unreal_editor__start_state_recording`
- `mcp__unreal_editor__stop_state_recording`

### 4.4 input simulation — 7

- `mcp__unreal_editor__inject_input_action`
- `mcp__unreal_editor__simulate_key_press`
- `mcp__unreal_editor__simulate_button_press`
- `mcp__unreal_editor__simulate_left_stick`
- `mcp__unreal_editor__simulate_right_stick`
- `mcp__unreal_editor__ui_perform`
- `mcp__unreal_editor__set_player_transform`

### 4.5 The candidate grant set (union of 4.1–4.4) — 32 names, sorted, one per line

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

### 4.6 Judgment calls, stated (so TASK-1222 can overrule with a reason instead of inheriting one silently)

- **`pie_scene_edit`** — mutates the LIVE PIE world only (spawn/delete/teleport/set property); the handler refuses outside PIE and never writes to disk. A verification STAGING tool, but a mutation, so it is left outside the four classes; grant or not is TASK-1222's call with a reason.
- **`create_easy_level_for_verification`** — CREATES A LEVEL ASSET (floor + lights + PlayerStart) under Content/. Verification-named, authoring-classed. Recommend EXCLUDE.
- **`spawn_blueprint_actors`** — spawns actors into the CURRENT EDITOR LEVEL (not PIE), dirtying the level package. Authoring-classed. Recommend EXCLUDE.
- **`load_level`** — opens a saved level in the editor before PIE; refuses on unsaved changes unless discard_unsaved=True. Needed to point a run at L_Arena, so it is classed PIE lifecycle. The verifier must never pass discard_unsaved without saying so on the report line.
- **`set_player_transform`** — teleports the PIE pawn: a transient PIE-only write, classed input simulation (the plan's 'drive the hero' shape).
- **`verification_agent`** — Aura's own multi-agent PIE orchestrator; runs for minutes, loads the level itself, spends Aura credit. Classed verification.
- **`get_input_mapping_context_keys`** — read-only asset read (which keys an IMC binds); classed verification because it answers the KBD-§ Enhanced-Input question and is in run_verification_sequence's own whitelist.
- **`start_profiler_capture / stop_profiler_capture / analyze_profiler_capture / set_cpu_throttle_override`** — performance instrumentation; PIE-adjacent, outside the four classes. Not needed for a VER-§ verdict; not recommended for the pilot.

## 5. `unreal_editor` names OUTSIDE the four classes — 82 names, grouped

⛔ Nothing in this section may appear in `permissions.allow` or in `playtest-verifier.md`'s `tools:` line unless TASK-1222 states a reason on its handoff.

### C++ authoring / compile / Live Coding (never granted to any agent) — 3

- `mcp__unreal_editor__edit_cpp_file`
- `mcp__unreal_editor__generate_cpp_file`
- `mcp__unreal_editor__trigger_live_coding`

### arbitrary Python / shell-equivalent (never granted) — 4

- `mcp__unreal_editor__execute_unreal_python`
- `mcp__unreal_editor__python_agent`
- `mcp__unreal_editor__create_text_file`
- `mcp__unreal_editor__edit_text_file`

### Blueprint / graph authoring — 30

- `mcp__unreal_editor__edit_blueprint`
- `mcp__unreal_editor__bp_agent`
- `mcp__unreal_editor__compile_blueprint`
- `mcp__unreal_editor__add_blueprint_node_to_strand`
- `mcp__unreal_editor__connect_blueprint_nodes`
- `mcp__unreal_editor__disconnect_blueprint_nodes`
- `mcp__unreal_editor__remove_blueprint_nodes`
- `mcp__unreal_editor__insert_node_on_exec_wire`
- `mcp__unreal_editor__set_blueprint_node_properties`
- `mcp__unreal_editor__set_node_pins_defaults`
- `mcp__unreal_editor__rename_blueprint_node`
- `mcp__unreal_editor__rename_blueprint_graph`
- `mcp__unreal_editor__collapse_graph`
- `mcp__unreal_editor__uncollapse_subgraph`
- `mcp__unreal_editor__move_node_into_subgraph`
- `mcp__unreal_editor__move_node_outof_subgraph`
- `mcp__unreal_editor__link_pin_to_tunnel`
- `mcp__unreal_editor__delete_pin_from_tunnel`
- `mcp__unreal_editor__delete_variable`
- `mcp__unreal_editor__replace_variable_reference`
- `mcp__unreal_editor__create_blueprint_interface`
- `mcp__unreal_editor__add_widget_property_binding`
- `mcp__unreal_editor__edit_widget`
- `mcp__unreal_editor__edit_structure`
- `mcp__unreal_editor__edit_enumeration`
- `mcp__unreal_editor__edit_timeline`
- `mcp__unreal_editor__edit_curve`
- `mcp__unreal_editor__edit_eqs`
- `mcp__unreal_editor__behavior_tree_agent`
- `mcp__unreal_editor__lock_edit_plan`

### asset authoring / import / generation — 25

- `mcp__unreal_editor__create_assets`
- `mcp__unreal_editor__create_edit_material`
- `mcp__unreal_editor__material_agent`
- `mcp__unreal_editor__vfx_agent`
- `mcp__unreal_editor__lock_vfx_plan`
- `mcp__unreal_editor__audio_agent`
- `mcp__unreal_editor__generate_audio`
- `mcp__unreal_editor__level_design_agent`
- `mcp__unreal_editor__edit_data_assets`
- `mcp__unreal_editor__add_or_replace_rows_in_data_table`
- `mcp__unreal_editor__remove_rows_from_data_table`
- `mcp__unreal_editor__create_gameplay_tag`
- `mcp__unreal_editor__create_input_actions`
- `mcp__unreal_editor__add_input_action_to_mapping_context`
- `mcp__unreal_editor__add_fab_asset_to_project`
- `mcp__unreal_editor__search_fab`
- `mcp__unreal_editor__auto_uv`
- `mcp__unreal_editor__autouv_and_retexture`
- `mcp__unreal_editor__check_uv_mapping`
- `mcp__unreal_editor__bake_all_mesh_textures`
- `mcp__unreal_editor__bake_texture_to_uv_channel`
- `mcp__unreal_editor__bake_texture_transfer`
- `mcp__unreal_editor__edit_mesh_postprocessing`
- `mcp__unreal_editor__spawn_blueprint_actors`
- `mcp__unreal_editor__create_easy_level_for_verification`

### todo / plan bookkeeping (Aura-internal) — 3

- `mcp__unreal_editor__create_new_todo_list`
- `mcp__unreal_editor__edit_todo_list`
- `mcp__unreal_editor__get_todo_list`

### read-only reference lookups (harmless, outside the four classes) — 12

- `mcp__unreal_editor__get_blueprint_component_properties`
- `mcp__unreal_editor__get_blueprint_properties`
- `mcp__unreal_editor__get_data_asset_type_info`
- `mcp__unreal_editor__get_data_asset_types`
- `mcp__unreal_editor__get_python_script_text`
- `mcp__unreal_editor__search_unreal_python_api`
- `mcp__unreal_editor__fetch_audio_skill`
- `mcp__unreal_editor__fetch_blueprint_best_practices`
- `mcp__unreal_editor__fetch_enhanced_input_skill`
- `mcp__unreal_editor__fetch_material_best_practices`
- `mcp__unreal_editor__fetch_niagara_best_practices`
- `mcp__unreal_editor__fetch_niagara_skill`

### PIE-world mutation (staging) — 1

- `mcp__unreal_editor__pie_scene_edit`

### profiling — 4

- `mcp__unreal_editor__start_profiler_capture`
- `mcp__unreal_editor__stop_profiler_capture`
- `mcp__unreal_editor__analyze_profiler_capture`
- `mcp__unreal_editor__set_cpu_throttle_override`

## 6. Acceptance B, self-check

- ≥1 name under each server: `unreal_inspector` 62, `unreal_editor` 114 ✓
- four-servers-connected line: §1 ✓
- every `unreal_editor` name classified exactly once: 32 in-class + 82 out-of-class = 114 = 114 ✓
- ⛔ no `.mcp.json` edit from this row ✓ · ⛔ no permission granted by this file ✓

**Consumers:** `TASK-1222` (allow-list: `mcp__unreal_inspector__*` + §4.5 minus whatever it excludes with a reason) · `TASK-1224` (the verifier `tools:` line = the set TASK-1222 granted) · `TASK-1222` (`AURA-PHASE0.md` §MCP tool census, verbatim copy) · committed by `TASK-1242`.
<!-- END verbatim copy of handoffs/AURA-MCP-CENSUS.md -->


## Tier

🧑 Jonathan's decision — recorded on TASK-1215 stage B, copied here verbatim by the orchestrator

**Filled by:** build-master, 2026-09-21, under `TASK-1369`. **Source of every figure below:** `TASKBOARD.md` → `TASK-1215-AURA-GATE-P0` → `status:` line (stage B discharge, 2026-09-21) — **COPIED, not paraphrased.** ⛔ This row visited no web page, opened no pricing page, re-derived no price and estimated nothing. **This section RECORDS; it DECIDES NOTHING.**

### 🚨 Read this before the table — this section breaks the file header's rule, and says so rather than doing it silently

The header of this file promises: *"No agent-derived number appears below — every measurement is copied from Jonathan's own words."* That promise holds for every other section here. It does **not** hold for the price table below, and the difference is not cosmetic (`SC-§97` cl. 3 · `SC-§139` cl. 4(b)):

- 🧑 **HIS WORDS — the decision, and the only thing in this section sourced to him.** Verbatim, in Claude Code, 2026-09-21:

  > "I just upgraded to the Aura Pro subscription."

  ⚠️ **Provenance, labelled as an ACCOUNT and NOT as a measurement (`SC-§139` cl. 4(b)):** his sentence reached `TASK-1215` through the **orchestrator lane**. ⛔ No agent on this project can measure who typed it (`SC-§139` cl. 2) and none claims to. ⛔ The **permission-class content here is nil** — this records a purchase he made; it authorises nothing.
- 🌐 **NOT HIS WORDS — the price table.** It is an **account of a third-party page at a point in time**: Aura's own about page, **read 2026-09-21** by the orchestrator. ⛔ Not a measurement. ⛔ Not law. ⛔ Not re-measurable on this machine. ⛔ **It will go stale without telling anyone** — a price the vendor changes on their server changes nothing in this file, so a later reader must re-read the page rather than trust this table's age.

### The decision

**TIER = `Pro`. DECIDED 2026-09-21, by Jonathan.**

### The three tiers — 🌐 THIRD-PARTY ACCOUNT (Aura's about page as read 2026-09-21) · ⛔ NOT a measurement · ⛔ NOT his words · ⛔ will go stale silently

| tier | monthly | annual | premium credit | overage | other |
|---|---|---|---|---|---|
| Indie | $20/mo | $10/mo billed annually (yearly total: **OWED** — not on the source line) | $15 | ⛔ **NONE** | — |
| **`Pro` ← CHOSEN** | **$40/mo** | **$360/yr** | **$60** | ✅ **PURCHASABLE** | higher rate limits · Super mode |
| Ultimate | $200/mo | $1,800/yr | $335 | **OWED** — not on the source line (⛔ not inferred from the rows above) | highest limits |

**All three tiers:** unlimited usage via **Auto Mode** + a **2-week trial** — 🌐 same third-party account, same 2026-09-21 reading.

⚠️ Any cell the source line does not carry reads **OWED**, never an estimate — the same rule §The three cases and §Totals he supplied already follow for wall time and credit.

### Why `Pro` and not `Ultimate` — recorded because a bare "Pro" loses the WHY

- **Ultimate is 5× the price for ~5.6× the credit** ⇒ **NEAR-LINEAR.** ⚠️ Those two ratios are `TASK-1215`'s own words, copied; they are consistent with the third-party table above and **inherit its label** — ⛔ they are as good as that one reading and no better.
- ⇒ Ultimate buys **HEADROOM, NOT EFFICIENCY.** You pay it to escape **rate limits**; you do not pay it to spend less per unit of work.
- ⛔ **And nobody has measured this project's actual burn.** §Pilot below reads `credit: not visible` on **all five** legs — **absence of the field, NOT a zero.**
- ⭐ **`Pro`'s decisive property is that overage is PURCHASABLE — the top-up is ITSELF the measurement.** If he buys overage every month, Ultimate becomes a **real question with a real number behind it**. Until then it is a guess wearing a price tag.

### The Fab ~$150 lifetime SKU — ⛔ UNCONFIRMED, recorded as a NON-FINDING

A Fab-marketplace **~$150 lifetime-MCP SKU** was carried in the orchestrator's notes as *"check this first"*. At the **2026-09-21** reading, **neither Aura's pricing-explained page nor its about page mentions it** (orchestrator's search, that date, those two pages). ⇒ **UNCONFIRMED / possibly never existed as recorded.** Stage B's *"the Fab SKU if it exists"* clause is **ANSWERED: NOT FOUND.**

🚨 **It is written down here for one reason: so that nobody later re-derives it as a missed opportunity.** `SC-§39` — **absence of evidence is not refutation.** What this entry says is *two named pages did not mention it on one date*. That is ⛔ **not** *"the SKU does not exist"*, ⛔ **not** *"a cheaper option was passed over"*, and ⛔ **not** evidence of a mistake.

### Cost characterisation — ⛔ STILL UNMEASURED

🚨 **This subsection exists because a reader who sees a filled §Tier will assume the cost question is closed. It is not.**

⚖️ **THE SPLIT, SAID PLAINLY AND ONCE: the TIER is KNOWN · the $-PER-VERIFICATION is UNMEASURED AND MAY STAY SO. ⛔ A DECIDED TIER IS NOT A CHARACTERISED COST. Two different things.**

**Why it is unmeasured — the citation is this file's own §Pilot, below:**

- §Pilot's table has **5 rows** (N1 · N3 · N2 · Probe 5 · N4) and its **`credit` column reads `not visible` in every one of them — 5/5.**
- §Pilot's lane facts close with: *"**Credit:** not visible in any tool reply across all five legs."*
- ⛔ **That is the absence of the FIELD in every tool reply. It is NOT a zero and must never be read as one.** No Aura tool reply carries a credit field at all, so there was nothing to record.
- ⇒ This is an **instrument ceiling, not an omission.** Nobody failed to write a number down; the number was never emitted. The ceiling itself is enumerated in `CONVENTIONS.md` `VER-§8`, which is where it stays — restating a ceiling is how a ceiling quietly gets relaxed.

**The only figure anyone has ever supplied, and what it is NOT:** §Totals he supplied records *"18% used context (90k of 500k tokens)"* across his three chat probes — 🧑 MEASURED BY JONATHAN (Aura chat, 2026-09-13). ⚠️ **That is Aura's CONTEXT-WINDOW gauge. It is not dollars and it is not credit**, and no arithmetic anywhere in this file converts it into either.

**What would close this — and only these two:**

1. 🧑 **Jonathan reading Aura's own account/usage page** and supplying the $-credit figure. ⛔ No agent can reach that page; it is a human dashboard read.
2. 🧑 **An overage top-up purchase** — ⭐ **the top-up is itself the measurement.** A month in which he buys overage produces the first real number this project has ever had for Aura burn, and it arrives as a **receipt** rather than an estimate. (This is also exactly why `Pro` was the right tier to sit on: `Indie` has no overage to buy, so it could not have produced the measurement at all.)

**Until one of those lands, the plan's `credit consumed` column stays `OWED`** — as §The three cases and §Totals he supplied already record it — and ⛔ is never filled with an estimate.

**Consumers pointing at this subsection by name** (both shipped in commit `a9880b9` while it did not yet exist, both correctly hedged *"being written under `TASK-1369`"*): `Docs/Aura AI for Unreal — Integration Plan.md` (its *Credit per verification* row and its Phase 0 checklist item) · `Docs/setupdirections.md` §E.2 item 3. ⇒ **This row is what makes those two pointers resolve. It does not change what either document claims.**

### ⛔ What this row did and did not do

⛔ **`TASK-1369` DECIDES NOTHING AND RECORDS ONLY.** The tier was chosen by Jonathan before this row existed. The prices were read off a third-party page by the orchestrator. The `credit: not visible` cells were measured by the playtest-verifier. This row copied three already-existing records into one place and **labelled which is which**. It changed no §Pilot cell, no §MCP tool census line, no pinned sha256, no byte of the verbatim-copy region, no grant, and no decision.

## Pilot (TASK-1230) — 2026-09-14

**Written by:** build-master, 2026-09-14, at the close of the `TASK-1230` five-leg pilot (`VER-§6` cl. 2 — the board-row legs, as distinct from the three chat probes above). **This section RECORDS; it DECIDES NOTHING.** Every number below is copied from the five verify reports (`qa/TASK-787-verify.md`, `qa/TASK-671-verify.md`, `qa/TASK-1068-verify.md`, `qa/TASK-1230-verify.md`, `qa/TASK-780-verify.md`) with their PIE times — MEASURED BY THE PLAYTEST-VERIFIER (Aura, PIE, 2026-09-14) unless labelled otherwise; nothing is re-derived here. The `match (manager)` column is the manager's adjudication on the `TASK-1230` board row, written concurrently — ⛔ no Y/N is invented here. `credit` reads `not visible` in every row because no tool reply carries a credit field (MEASURED BY THE PLAYTEST-VERIFIER — absence of the field, not a zero). Wall times are the verifier's own dispatch-to-report estimates. Order = execution order (N1 → N3 → N2 → Probe 5 → N4).

| case | row | Aura verdict | 🧑 his playtest sentence | match (manager) | credit | wall time | commit |
|---|---|---|---|---|---|---|---|
| N1 | `TASK-787` (host `TASK-780`, `1231bc4`) | `VERIFIED (advisory) partial 1/3` — row 1 pass: Z 98.15 → 252.69, `speed 350.0` = `LadderClimbSpeedUU`; rows 2+3 `unobs` — the hold released at 1.5 s ⇒ designed `H-3` abort, then the hero died and attempt 3 drove the ghost | *"Walking my hero into the tower's ladder foot climbs it onto the deck about 1200 uu up, and I can climb it again straight after."* | manager: see TASK-1230 row | not visible | ≈35 min | `51ac7d5` |
| N3 | `TASK-671` (host `TASK-674`, `d8bfd23`) | `UNOBSERVABLE (advisory)` — both rows `unobs` for ONE cause: the deck builder opens only from the main menu's "Deck Builder" button and no verifier input reached it (8× `simulate_key_press Tab` → `binding_found: false`, every `ui_snapshot` `focused: false`; no `UDeckBuilderWidget` constructed) | *"Opening the deck builder shows ten slots deck1–deck10 with the orange rim on deck1 only."* (manager's redraft; his confirmation pending) | manager: see TASK-1230 row | not visible | ≈12 min (PIE ≈56 s) | `e398c13` |
| N2 | `TASK-1068` (host `TASK-1070`, `c6bb376`) | `VERIFIED (advisory) 2/2` — row 1: `BP_SiegeFog_C` 0→1 within 0.93 s of `IA_Card1`, `RelativeLocation.Z` 7000, `RelativeScale3D` (640, 360, 260), `FogActiveUntilTimeSeconds` 363.196615 at world 64.139 s ⇒ 299.06 s ∈ (295, 300], gold 73→24; row 2: `Fog VISUAL destroyed` 300.106 s after the spawn line, census 0, `FogVisualActor` = `None` | *"Playing the Fog card raises one battlefield-wide fog visual immediately and the fog timer reads five minutes."* | manager: see TASK-1230 row | not visible | ≈30 min (PIE 7 min 03 s) | `72292a3` |
| Probe 5 | deliberately broken rerun of `TASK-1068` (`DT_Cards` Fog Cost 50→5000, data-only, `R-PROBE5`) | `VERIFY-FAILED (advisory)` — press refused (`hand slot 0 ('Fog') refused — cost 5000, gold 88`; HUD "Not enough gold"), no `BP_SiegeFog_C` spawned, gold unspent; detection 0.81 s after the press; failure class `SC-§79` entry-point-unreachable (a data cell invisible to compile / suite / text QA, the `SC-§36.1` shape) | n/a — the exhibit line: the verifier was expected to FAIL and did | manager: see TASK-1230 row | not visible | ≈20 min (PIE 2 min 49 s) | `d3ade81` |
| N4 | `TASK-780` (host `1231bc4`) | `VERIFIED (advisory) 2/2` — row 1: Z 98.15 → 1298.40 = deck 1296 +2.4 uu (inside ±25), `speed 350.0` = `LadderClimbSpeedUU`, `MOVE_Walking` on the deck, `CurrentHP 200`; row 2: a second climb admitted 2.8 s after the first's deck arrival, four admissions across two sessions, zero refusals (attempt 2 void — hero killed by the bot at PIE t≈02m51.7s) | *"Walking my hero into the tower's ladder foot climbs it onto the deck about 1200 uu up, and I can climb it again straight after."* | manager: see TASK-1230 row | not visible | ≈16 min (two PIE sessions) | `fa97de8` |

**Row count:** 5 (four board-row legs + the one deliberate-failure exhibit). Evidence for every row = the promoted `VER-TASK-###` stills under `playtest-evidence/2026-09-14/` in the commit named; videos never promoted (`VER-§4` cl. 3).

### Lane facts (§Pilot cl. 4) — what the five legs measured about the LANE, not the rows

- **Verifier has no copy tool:** the `playtest-verifier` holds no shell or binary-copy tool, so every promotion (`Saved/AuraVerify/` → `playtest-evidence/`) is OWED to the committing host row — copy-by-sha256, oid-vs-sha256 in the commit (N1, N3, N2, Probe 5, N4 all promoted by build-master).
- **UMG buttons un-actuable (N1, N3):** Aura's `ui_perform` (three click shapes) never fires `OnClicked`; `simulate_key_press Tab` ×8 on `L_MainMenu` → `binding_found: false`, no focus move (`applied_mapping_contexts: []` — no input mapping context on the menu pawn). Every UMG-button-gated flow (deck builder, sandbox, settings, login…) is verifier-blind; a human click is the only known route. Route B (direct `load_level L_Arena`, vs-bot) is the working substitute.
- **Enhanced Input reached (N2, N4):** `inject_input_action` on OUR actions (`IA_Card1`, `IA_Move`) drove `IMC_Hero` — the card was played, the hero walked and climbed.
- **VRAM banner (N3, N2):** a red "Video memory has been exhausted (N MB over budget)" banner appeared on frames — in N3 with a `-game` co-run (`gpu.current_usage_mb 3830` vs `budget_mb 3674`), in N2 with NO co-run and DXGI reading UNDER budget. No quoted number was read off pixels, so it affected no verdict; the banner/DXGI disagreement is unmeasured.
- **Same-size DT break (Probe 5):** the mutated and the committed `DT_Cards.uasset` were BOTH 46510 bytes; only oid-vs-sha256 proved the break existed and then that it was reverted (the `SHIP-§` LFS law, confirmed again) — MEASURED BY BUILD-MASTER (handoff §6/§7).
- **Input hold sustained (N4):** `inject_input_action` `hold_seconds` 8 and 5 were each sustained in ONE call — no chaining gap, exit `H-3` never tripped; N1's 1.5 s release was the recipe, not the tool. A held W walks the hero off the 472-uu deck in <1 s after arrival, so a "+7 s" read assumes a stop that does not happen.
- **Stale log after a relaunch (N4):** after the Probe 5 relaunch, `get_unreal_output_logs` served the OLD instance's `Saved/Logs/GitClaudeUnrealTest_2.log` (`log_source=editor-cached`); the live PID 6136 writes `Saved/Logs/GitClaudeUnrealTest.log`. Log reads after a relaunch must name the file whose line-1 `Log file open` matches the instance's creation time.
- **Bot kills the hero ~t=2m50s in Route B (N1, N4):** in a vs-bot `L_Arena` match the idle hero dies around PIE t≈02m50s (N4 session 1: `'BP_HeroCharacter_C_0' died` at t≈02m51.7s; N1's attempt 3 drove the ghost); a ghost cannot climb by type. Drives must finish inside that window or restart PIE.
- **Fog is timer-destroyed, not match-end-destroyed (N2):** the fog visual outlived the match end (Defeat at t≈04m24s) by 99.4 s — the destroy fired on the pinned expiry, not on the match-end path. Whether a match end SHOULD clear it is a design question for the manager, not a finding against the row.
- **~60 s wait cap (N2):** `run_verification_sequence` waits at most ≈60 s per call, so a 300 s hold is five calls.
- **Credit:** not visible in any tool reply across all five legs — the plan's "credit consumed" column stays OWED to Jonathan's Aura usage page.
