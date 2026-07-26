# TASK-288 handoff — SK-LOD step for future rigs (rig_character.py)

**Assignee:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-07-24
**Lane:** main-lane-safe (Tools/ Python only; no branch-owned asset, no scatter/unit C++, no Git, no compile)

## File touched (the ONLY change)
- `Tools/ArtPipeline/rig_character.py` — **git diff = 99 insertions, 0 deletions** (purely additive; no existing line modified or removed).
  - NOTE: the board spec path is `Tools/rig_character.py`; the file actually lives at `Tools/ArtPipeline/rig_character.py` (sibling of `refine_trellis_glb.py` / `fix_rig_root.py`). Same file, correct target.

## What the LOD step does + where it hooks in
A new final pipeline stage (**stage 9, "SK-LOD"**) that **emits a deterministic, UE-consumable LOD recipe sidecar** — `Content/RawAssets/Characters/<CardID>.lod.json` (smoke → `Cache/<CardID>/rig/smoke/<CardID>.lod.json`) — beside the exported `<CardID>.fbx`. The recipe pins the CONVENTIONS SK-unit LOD law:
- **LOD chain:** LOD0 100%@1.0 (base) / **LOD1 50% @ screenSize 0.4** / **LOD2 20% @ 0.15** — expressed as `(percent_triangles, screen_size)` pairs, the SAME reduction contract `reimport_meshes.py::_apply_lods` feeds to `set_lods` (SK mirror of `CASTLE_LOD_CHAIN`).
- **URO component defaults:** `visibility_based_anim_tick_option = "OnlyTickPoseWhenRendered"` + `enable_update_rate_optimizations = true` (mirrors TASK-285's runtime `SkeletalVisualMesh` setting, baked into the export tooling so every FUTURE unit imports LOD/URO-ready).

**Hook point (one additive call in `main()`, ~line 1114):** immediately after
`report["outputs"]["mesh_fbx"] = str(export_skeletal_fbx(...))`, before the anim-FBX exports:
```python
write_sk_lod_recipe(card_id, mesh_fbx, guard)
```
Runs unconditionally (LODs are automatic for future rigs) and in `--smoke`/`--no-anim-fbx` alike; the sidecar inherits `mesh_fbx`'s directory so it is smoke-routed and OutputGuard-confined exactly like the FBX.

New code:
- Constants (lines 126-136): `SK_LOD_CHAIN`, `SK_URO_VISIBILITY_TICK`, `SK_URO_ENABLE_UPDATE_RATE`, `SK_LOD_RECIPE_SCHEMA`.
- `build_sk_lod_recipe(card_id)` (line 825): PURE — returns the recipe dict from CardID + constants only. Asserts the chain is LOD0-full + strictly-decreasing so it can't silently drift on a future edit.
- `write_sk_lod_recipe(card_id, mesh_fbx_path, guard)` (line 856): guard-checks + writes `json.dumps(recipe, indent=2, sort_keys=True)`; non-fatal + report-neutral.
- Docstring: added a "9. SK-LOD" line to the pipeline overview (documentation only).

## Why a recipe sidecar and not a direct UE call (LOAD-BEARING — please read)
`rig_character.py` runs **headless in Blender** (`import bpy`; its own docstring: *"this script NEVER touches the Unreal editor"*). It has **no `unreal` module**, so it physically cannot call `SkeletalMeshEditorSubsystem.regenerate_lod` nor set `VisibilityBasedAnimTickOption` / `bEnableUpdateRateOptimizations` (those are UE editor-python / SkeletalMeshComponent APIs). The only UE-side tool in the repo is `reimport_meshes.py` (`import unreal`), and it imports **static** meshes only — no SK import path to bolt onto.

So the faithful, constraint-satisfying realization of "add the SK-LOD step to the rig tooling" is to **emit the exact LOD chain + URO flags as a deterministic recipe** that the UE-side consumer applies verbatim — the project's established **contract-by-path** pattern (`SM_<CardID>`, `NS_Spell_<CardID>`, `S_<Event>`). Consumers:
- **TASK-289** (build-master) — the editor-python `regenerate_lod` helper for existing rigs; it can read `<CardID>.lod.json` and apply the identical chain/flags to new rigs.
- **TASK-162** — the in-editor SK import.

The recipe's `apply_with` field + the `(percent_triangles, screen_size)` shape name the exact reduction call to mirror, so "mirror the regenerate_lod pattern" is honored at the data-contract level (the same pattern `reimport_meshes.py` uses for SMs).

## Idempotency approach (no double-apply, no drift)
The recipe is a **pure function of CardID + module constants** — no timestamps, no measured/rig data. `json.dumps(..., sort_keys=True)` makes byte order deterministic. Re-running the rig pipeline **overwrites** the sidecar (never appends) with **byte-identical** content. Proven standalone (constants+logic reproduced): two runs → identical SHA-256; distinct per CardID. The `assert` guards in `build_sk_lod_recipe` fail loudly if a future edit breaks the chain invariant (LOD0 full + strictly decreasing), preventing silent drift.

## Confirmation: no behavior change to any other stage
- **`git diff --numstat` = 99 insertions / 0 deletions** — not one existing line changed. import / measure / armature / skin / animate / export / preview / **report** run byte-identical; `rig_report.json` is deliberately **not** mutated (the recipe path is intentionally NOT recorded into `report` — a separate file keeps the report untouched).
- On the happy path the new stage adds nothing to `report["warnings"]`; on a (rare) write failure it is **non-fatal** and logs to stdout only, so it never breaks the FBX/anim/report deliverables and never mutates the report JSON.
- Static syntax validated (in-memory AST parse; no compile, no bytecode artifacts).

## For QA to scrutinize
1. **Idempotency** — confirm the recipe is timestamp-free/pure and `sort_keys` deterministic (re-run byte-identical). `--smoke` run writes to `Cache/<CardID>/rig/smoke/<CardID>.lod.json` for readback without touching shipping raw assets.
2. **No side-effects** — the 0-deletions diff; report untouched; call placed after mesh export; runs correctly under `--smoke` and `--no-anim-fbx`.
3. **Design-model check (flag for orchestrator/manager if you disagree):** the recipe-sidecar interpretation vs. a literal (impossible-in-Blender) in-script UE call. The values (LOD1 50%@0.4 / LOD2 20%@0.15, `OnlyTickPoseWhenRendered`, update-rate ON) match CONVENTIONS SK-unit LOD/URO law + TASK-285 exactly. If a UE-side apply is wanted instead, its natural home is the TASK-289 `regenerate_lod` helper reading this recipe — no change to this Blender script.
4. **Path note** — spec said `Tools/rig_character.py`; real path `Tools/ArtPipeline/rig_character.py`.

## Not done (out of scope / correctly deferred)
- No UE editor calls, no asset import, no `regenerate_lod` invocation (UE-side, TASK-289).
- No compile, no Git (file-only).
- No change to `reimport_meshes.py` (already carries the SM `LargeProp`/castle LOD line, TASK-220).
