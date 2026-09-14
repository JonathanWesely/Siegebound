# TASK-1254 — [AURA-PERMISSIONS-2] build-master handoff (2026-09-13)

**Status: done (gate WAIVED per the board — gitignored permissions file, no commit; the greps ARE the check; live test = `TASK-1255`'s agent loading + `TASK-1230`). ⛔ NOT committed — `settings.local.json` is gitignored (`git check-ignore -v` → `GitClaudeUnrealTest/.gitignore:16`) and never staged; `AURA-PHASE0.md` + this handoff are committed by `TASK-1242`.** Law: ruling R10 · `VER-§7` cl. 1–3 (as amended 2026-09-13).

## Acceptance (1) — FIRST, both wildcard lines (measured after the write)
```
grep -c 'mcp__unreal_editor__\*'    .claude/settings.local.json   -> 0
grep -c 'mcp__unreal_inspector__\*' .claude/settings.local.json   -> 0
```

## Acceptance (2)–(5)
```
grep -c '"mcp__unreal_inspector__' .claude/settings.local.json   -> 49
grep -c '"mcp__unreal_editor__'    .claude/settings.local.json   -> 32
allow count BEFORE 78 ; AFTER 126  (126 − 48 = 78: 49 added, 1 removed)
13-excluded alternation grep (all 13 names, one -E pattern)      -> 0
json.load after write: OK ; enabledMcpjsonServers = ["unreal-mcp","blender","unreal_inspector","unreal_editor"] (unchanged)
```
Acceptance (6): `AURA-PHASE0.md` verbatim-copy region sha256 after the edit = `23d376fad53330b6359762f8632d6c530f4e4202e7605ef03a2983c76208910f` (== the source file, unchanged). Acceptance (7): no agent `tools:` line touched (`playtest-verifier.md`, `qa-reviewer.md` untouched — `TASK-1255` / `TASK-1256`).

## The set-difference assertion (in-script, before any write; ⛔ nothing typed by hand)
The 62 names were EXTRACTED from the census `## 2.` fenced block (regex over `handoffs/AURA-MCP-CENSUS.md`); the 13 were EXTRACTED from the backticked names in the three `### 2a.` bullets; then asserted:
```
|S2| = 62 · |S2a| = 13 · |S2 − S2a| = 49 · S2a ⊂ S2: True · S2a ∩ result = ∅: True · S2a == the 13 named in the dispatch/row spec: True
```
The merge was a `list.pop` of the one wildcard at its index + `list.insert` of the 49 (sorted) at that same index — the parsed array was MERGED, never replaced. Asserted post-merge: index 0 and index 44 (the last pre-existing entry) unchanged; the 32 `unreal_editor` entries identical in content and order; 49 inspector entries; none of the 13 present. File re-serialised 2-space indent, LF, no BOM, 7321 bytes.

## The 49 granted `unreal_inspector` names — verbatim, sorted, one per line (⛔ `TASK-1255` / `TASK-1256` / `TASK-1262` COPY FROM THIS BLOCK)
```
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
mcp__unreal_inspector__query_unreal_project_assets
mcp__unreal_inspector__quicksearch
mcp__unreal_inspector__read_datatable_keys
mcp__unreal_inspector__read_datatable_values
mcp__unreal_inspector__review_blueprint
mcp__unreal_inspector__search_geometry_scripts
```

## The 13 EXCLUDED (census §2a — granted to no pipeline agent, `VER-§7` cl. 3)
```
mcp__unreal_inspector__cancel_operation
mcp__unreal_inspector__create_or_edit_plan
mcp__unreal_inspector__derive_normal_map
mcp__unreal_inspector__edit_images
mcp__unreal_inspector__generate_images
mcp__unreal_inspector__generate_model_from_image
mcp__unreal_inspector__generate_model_from_text
mcp__unreal_inspector__get_recent_generated_images
mcp__unreal_inspector__launch_unreal_project
mcp__unreal_inspector__lock_plan_layer
mcp__unreal_inspector__recompile_unreal_project
mcp__unreal_inspector__rig_model_from_mesh
mcp__unreal_inspector__shutdown_headless
```

## What was written
1. `.claude/settings.local.json` (⛔ gitignored, never staged) — `permissions.allow`: `mcp__unreal_inspector__*` removed, the 49 above inserted in its place; 78 → 126.
2. `.claude/pipeline/qa/AURA-PHASE0.md` → `## MCP tool census`, directly under the `**Ruling owed (manager):**` paragraph of the OPEN MANAGER RULING: ONE italic line — *RULED R10 2026-09-13 — enumerated, not wholesale; the 49 granted `unreal_inspector` names are in `handoffs/TASK-1254-buildmaster.md`, the 13 excluded are census §2a.* — preceded by one blank separator line so it renders as its own paragraph (546 → 548 lines; declared here so the diff-stat is not a surprise). Nothing else in that file touched; copy-region sha256 unchanged (above).
3. This handoff.

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · no `Saved/` write · no `.mcp.json` edit · no agent `tools:` line edit · only the `TASK-1254` `status:` line edited on the board.

## Note for the orchestrator
Claude Code reads `settings.local.json` permissions at session start / on `/permissions` reload — a running session may not see the 49 enumerated grants (or the wildcard's removal) until reloaded. Until then a running session holding the old wildcard is still permissive; nothing in this row can force that reload.
