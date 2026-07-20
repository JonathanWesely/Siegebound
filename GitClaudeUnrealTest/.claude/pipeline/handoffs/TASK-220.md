# TASK-220 handoff — gameplay-programmer (2026-07-18)

## Reimport LOD-group line ON MAIN (M7.6 sequencing law, pulled forward from Phase 4)

**Files touched:** `Tools/reimport_meshes.py` ONLY (main lane; no branch-owned files, no Content/, no Git).

## What changed

New step 4 in `_reimport_one()`, inserted DIRECTLY after the Nanite-OFF block (per spec ~line 392-396 of the
pre-change file), before the collision step. Old steps 4/5/6 renumbered 5/6/7 (comments only). Header
"WHAT IT DOES" list updated to match.

1. **Data-driven constants** (next to the HULL_* constants, mirroring the file's existing constant-block style):
   - `DEFAULT_LOD_GROUP = "LargeProp"`
   - `CASTLE_CLASS_CARD_IDS = ["Castle", "Castle_Crumble01", "Castle_Crumble02", "Castle_Crumble03"]`
     (matches on-disk `Content/Meshes/SM_Castle*.uasset` names exactly — the script derives `SM_<CardID>` from
     the CardID, so these four IDs cover SM_Castle + all three crumble stages)
   - `CASTLE_LOD_CHAIN = [(1.00, 1.00), (0.50, 0.40), (0.25, 0.15)]` — (percent_triangles, screen_size), LOD0
     first per the `set_lods` contract. LOD1 50% @ 0.4, LOD2 25% @ 0.15, NO LOD3 (landmark-silhouette law).
   Adding a future castle-class mesh = one list entry, no code-path edit (same spirit as the sidecar card list
   and the manifest-driven collision modes).

2. **New helper `_apply_lods(sm, card_id, result)`** (own section between the collision helpers and Per-card):
   - Default branch: `sm.set_editor_property("lod_group", DEFAULT_LOD_GROUP)` — the engine auto-generates the
     LargeProp 4-LOD reduction chain on build. Step recorded as `lod_group(LargeProp)`.
   - Castle-class branch: clears `lod_group` to `"None"` FIRST (explicit per-mesh reduction only applies with no
     group set — keeps a re-run idempotent even after a mistaken group assignment), then builds
     `unreal.EditorScriptingMeshReductionOptions` (`auto_compute_lod_screen_size=False` so the explicit screen
     sizes stick) with three `EditorScriptingMeshReductionSettings` from `CASTLE_LOD_CHAIN`, and calls
     `unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_lods(sm, options)` (spec-named API;
     `EditorStaticMeshLibrary.set_lods` getattr fallback mirrors the collision helpers' dual-path pattern).
     Negative return raises into the WARN path. Step recorded as `lods_castle(<n>)`.
   - NON-FATAL by design (collision-helper precedent): any exception → WARN note "mesh keeps prior LODs",
     reimport continues to collision/save. LODs never block a geometry reimport.

3. **Readback extension** (step 7, mirroring the hull/box try-except pattern): `result["lod_count"]` via
   `sm.get_num_lods()` + `result["lod_group"]`, both in the SUMMARY_JSON and appended to the per-card DONE log
   line (`lods=%s lod_group=%s`).

## Everything preserved (verified in the diff — 1 file, LOD-only hunks)

- Same-path `AssetImportTask(replace_existing=True)` reimport, object identity/refs — untouched.
- Nanite OFF block — byte-identical, LOD step runs AFTER it.
- Collision authoring (unit hulls / manifest boxes) — untouched, still runs after the LOD step, still writes
  `body_setup` directly (LOD count does not touch `agg_geom`).
- Commandlet contract (editor closed, `-run=pythonscript`), sidecar `Tools/reimport_cards.txt` flow,
  texture/MI step, GoldNode emissive exception — untouched. NOTE: the sidecar currently on disk lists the
  U2/building wave (no castle IDs), so the castle branch activates only when a batch actually lists a
  castle-class CardID — e.g. the M7.5 TASK-201/202 retexture wave (which is blocked-by this task landing
  QA-passed).

## Why the known-limitation flow (material-slot reset + MCP finalize step 2) cannot break

- The commandlet limitation is about the mesh's slot->material POINTERS being reset to WorldGridMaterial by the
  post-script build. The LOD step adds/regenerates LOD source models; it never writes `static_materials` and
  reduction never creates or reorders slots — reduced LODs reference the existing slot indices.
- `Tools/reimport_apply_materials_mcp.py` (finalize step 2) assigns materials by SLOT NAME at the mesh level
  (applies across all LODs) and uses `lod_index: 0` only for a triangle-count readback — verified by reading the
  script. Extra LODs are invisible to it.
- If anything, the LOD step benefits from the same post-script build that causes the slot reset: the LargeProp
  chain / explicit reduction is baked in the same rebuild.

## Validation (QA: reproduce with the scratchpad harness if desired)

- `python -m py_compile Tools/reimport_meshes.py` → clean (pycache artifact removed afterwards).
- Stubbed dry logic trace (fake `unreal` module; module import runs `main()` harmlessly — every card SKIPs at
  the FBX precondition against a nonexistent content dir, proving no top-level side effects): ALL CHECKS PASSED —
  - all 4 castle-class IDs route to `set_lods` with chain exactly [(1.0,1.0),(0.5,0.4),(0.25,0.15)],
    `auto_compute_lod_screen_size=False`, `lod_group` cleared to "None" BEFORE `set_lods`, step `lods_castle(3)`;
  - all 10 DEFAULT_CARD_IDS + Knight/Ogre/CrystalTower take the single `lod_group=LargeProp` call, `set_lods`
    NOT called;
  - a raising mesh object is non-fatal on both branches (returns False, WARN note, no propagation).
- NOT run: an actual reimport (spec: code change only; the batch wave applies it — TASK-202 rides this).

## QA should scrutinize

- UE 5.8 API names: `StaticMesh.lod_group` (FName editor property), `StaticMesh.get_num_lods()`,
  `StaticMeshEditorSubsystem.set_lods`, `EditorScriptingMeshReductionOptions/Settings`
  (`auto_compute_lod_screen_size`, `reduction_settings`, `percent_triangles`, `screen_size`) — all stock
  editor-python surface, but eyeball for typos.
- The `"None"` string coercing to NAME_None on the castle path (standard UE python Name coercion).
- Ordering: LOD step between Nanite OFF and collision — matches spec "directly after the Nanite-OFF block".

## Commit routing (per spec)

Rides TASK-196 if timing aligns, else standalone build-master micro-commit — orchestrator's call.
Cherry-pick/merge into `m7.6-arena10x` at Phase 4. Cross-link: M7.5 TASK-202 is blocked on this landing
QA-passed on main.

---

## LOOP-1 FIX (2026-07-18) — QA blocker resolved (qa/TASK-216-217-220-qa.md, TASK-220 section)

**Blocker:** the castle branch constructed `unreal.EditorScriptingMeshReductionOptions/Settings` — pre-5.0
names that do not exist in UE 5.8 (renamed `*_Deprecated`, no ScriptName alias), and the
`EditorStaticMeshLibrary.set_lods` fallback was a C++-only deprecated shim (no UFUNCTION). Every castle
reimport would have AttributeError'd into the non-fatal except and silently shipped 1 LOD.

**Fix (diff-scoped to `_apply_lods` + a 4-line batch-summary add in `main()`):**
- [BLOCKER] Struct names → `unreal.StaticMeshReductionOptions()` / `unreal.StaticMeshReductionSettings()`.
  Field names unchanged (`auto_compute_lod_screen_size`, `reduction_settings`, `percent_triangles`,
  `screen_size`) — INDEPENDENTLY re-verified against the installed engine before editing:
  `StaticMeshEditorSubsystemHelpers.h:18-53` (both USTRUCTs BlueprintType, all 4 fields
  BlueprintReadWrite) and `StaticMeshEditorSubsystem.h:44-51` (`SetLods` UFUNCTION(BlueprintCallable),
  int32 return, negative = failure — matching the existing `< 0` raise).
- [BLOCKER] `EditorStaticMeshLibrary` fallback DROPPED entirely from `_apply_lods`; subsystem-only, with an
  explicit `RuntimeError("StaticMeshEditorSubsystem unavailable")` into the non-fatal path if
  `get_editor_subsystem` returns None. (The collision helpers' pre-existing EditorStaticMeshLibrary
  getattr-fallbacks are untouched — out of TASK-220 scope, not a QA finding.)
- [WARN hardening, rides the fix] Failure notes/log lines now carry a grep-loud `LOD_STEP_FAILED` token;
  `_apply_lods` records `result["lod_ok"]` (True/False, absent on SKIPped cards); `main()` batch summary now
  emits `LOD_STEP_FAILED on N asset(s): [...]` via `_warn` when any LOD step failed — a fleet run with 0
  applied castle chains is loud, not buried.
- [NIT] The castle `lod_group` → "None" clear now happens AFTER options construction + subsystem resolve,
  immediately before `set_lods` — the failure path is state-neutral (verified by trace: options-ctor
  failure and no-subsystem failure both leave the mesh with zero property writes).

**Validation (re-run):** `py_compile` clean. Trace harness upgraded per QA: the fake `unreal` is now STRICT
(only the verified 5.8 surface exists — a regression to the dead names AttributeErrors and fails the castle
assertions), plus an API-surface section that greps the INSTALLED engine headers (no
`Intermediate/PythonStub/unreal.py` exists in the project — checked) asserting both USTRUCTs, all 4
BlueprintReadWrite fields, and the BlueprintCallable `SetLods` signature, and asserts the script source
constructs ONLY the 5.8 names (no `EditorScriptingMeshReduction*` anywhere; no `EditorStaticMeshLibrary`
in `_apply_lods` code). New trace cases: state-neutral options-ctor failure, no-subsystem path, lod_ok
recording, LOD_STEP_FAILED token. ALL CHECKS PASSED.
