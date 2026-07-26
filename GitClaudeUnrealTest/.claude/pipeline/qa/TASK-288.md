# QA Report — TASK-288
Verdict: PASS

Scope: `Tools/ArtPipeline/rig_character.py` — new final "SK-LOD" stage (stage 9) emitting a
deterministic `<CardID>.lod.json` recipe sidecar beside the exported FBX. Python tooling, no compile.
Reviewed against CONVENTIONS "Arena 10x scale-up & LOD/perf (M7.6)" SK-unit LOD + URO law (line 173),
the `reimport_meshes.py::_apply_lods` contract shape, board spec "#### TASK-288", and handoff TASK-288.md.

## Architecture ruling (the load-bearing interpretation)
FAITHFUL realization of the spec's intent — NOT a gap.

The spec said "mirror the existing `regenerate_lod` editor-python helper." `rig_character.py` runs
HEADLESS IN BLENDER (`import bpy`; module docstring: it NEVER touches the Unreal editor) — it has no
`unreal` module, so it physically CANNOT call `SkeletalMeshEditorSubsystem.regenerate_lod` nor set the
`SkeletalMeshComponent` URO properties. A literal in-script UE apply is impossible at this tool boundary.
The constraint-satisfying realization is exactly what shipped: emit the EXACT LOD chain + URO flags as a
deterministic recipe using the SAME `(percent_triangles, screen_size)` contract as
`reimport_meshes.py::_apply_lods`, deferring the literal apply to TASK-289 (build-master, on-editor). This
is the project's established contract-by-path pattern (`SM_`/`NS_`/`S_` soft paths). The literal
`regenerate_lod` apply genuinely belongs in TASK-289, not here. Ruling: the recipe-sidecar is the correct
architecture; the "mirror `regenerate_lod`" intent is honored at the data-contract level.

## Chain / flags match-law check (verified EXACT)
CONVENTIONS line 173 SK-unit law vs. shipped `SK_LOD_CHAIN` (rig_character.py:127-131) + URO consts (134-135):
- LOD0 (1.00, 1.00) base — full detail. MATCH.
- LOD1 50% @ screenSize 0.4  →  (0.50, 0.40). MATCH.
- LOD2 20% @ screenSize 0.15 →  (0.20, 0.15). MATCH — correctly **20%**, NOT the castle chain's 25%
  (`reimport_meshes.py::CASTLE_LOD_CHAIN` line 122 is 0.25). The exact distinguishing value is right; the
  programmer did not copy-paste the SM landmark value.
- `visibility_based_anim_tick_option = "OnlyTickPoseWhenRendered"`. MATCH (VisibilityBasedAnimTickOption).
- `enable_update_rate_optimizations = true`. MATCH (bEnableUpdateRateOptimizations; mirrors TASK-285 runtime).
- Contract field names `percent_triangles` + `screen_size` per LOD (line 846) are IDENTICAL to
  `_apply_lods`'s `set_lods` reduction props (reimport_meshes.py:373-374) → TASK-289 applies it 1:1.
Build-in guard: `build_sk_lod_recipe` asserts LOD0 == (1.0,1.0) and strictly-decreasing percent/screen so a
future edit cannot silently drift the chain (fails loud, and the write path swallows it non-fatally).

## Idempotency check (verified byte-identical on re-run)
- `build_sk_lod_recipe(card_id)` is a PURE function of `card_id` + module constants — no timestamps, no
  measured/rig data. The only timestamp in the pipeline (`report["started_utc"]`) is in `rig_report.json`,
  NOT in the recipe (recipe dict has zero time/random fields).
- Serialized `json.dumps(recipe, indent=2, sort_keys=True)` → deterministic key order + whitespace.
- Written with mode `"w"` (overwrite, never append). Re-run ⇒ byte-identical content; distinct per CardID.

## No-side-effects check (verified)
- Purely additive: docstring stage-9 line (46-50), constants block (107-136), two new functions
  (824-878), and ONE guarded call + comment in `main()` (1109-1114). Every existing stage is byte-intact
  at each insertion seam — export_skeletal_fbx (:1108), the anim-FBX loop (:1115+), and the rig_report
  write (:1130-1132) are unmodified. Consistent with the handoff's `git diff --numstat = 99 / 0`
  (numstat itself not re-run — no Bash/git tool in this session — but the additive seams are confirmed by
  static read).
- `rig_report.json` is NOT mutated: the recipe path is deliberately never written into `report`; the recipe
  is a separate file. Report content is independent of the recipe stage.
- Non-fatal + report-neutral on failure: `write_sk_lod_recipe` wraps everything in `try/except Exception`
  → logs `SKIPPED (non-fatal)` + returns None; it cannot break the FBX/anim/report deliverables.
- Write-confined + smoke-routed: `guard.check(Path(mesh_fbx_path).with_name(f"{card_id}.lod.json"))`
  inherits the FBX's already-validated directory → smoke lands at `Cache/<CardID>/rig/smoke/<CardID>.lod.json`,
  shipping at `Content/RawAssets/Characters/<CardID>.lod.json`; both inside OutputGuard's allowed roots.
  Runs unconditionally, correct under `--smoke` and `--no-anim-fbx` (call is before the anim-fbx branch).

## Python correctness
- All new code parses cleanly (valid defs / returns / dict + list-comp literals / f-strings / try-except);
  `json` (import :66) and `Path` (:72) are imported. Hook call in `main()` is additive, guard-confined,
  and smoke-routed. No syntax error, no shadowing, no headless-bpy pitfall (writes JSON only — no `bpy.ops`,
  no `bpy.context` UI/window/view-layer dependency in the new stage).

## Findings
- [NIT] rig_character.py:842-844 — the human-readable `apply_with` prose names the SK reduction property
  `number_of_triangles_percentage` while the authoritative DATA field is `percent_triangles` (the SM contract
  name). This is intentional and correct: the VALUE (fraction of triangles to KEEP, 0.50 / 0.20) is identical
  for both the SM `percent_triangles` and the SkeletalMesh `NumOfTrianglesPercentage` reduction property.
  Not a defect — flagged only so TASK-289 maps the field value to the SkeletalMesh reduction property, not the
  StaticMesh one.
- [NIT] rig_character.py:865-877 — the "always non-fatal" claim has one theoretical exception: a
  write-confinement violation routes through `OutputGuard.check` → `fail()` → `sys.exit()`, which raises
  `SystemExit` (a `BaseException`, NOT caught by `except Exception`) and would halt the pipeline. Unreachable
  in practice (the sidecar shares the FBX's already-validated directory) and — if ever reached — halting on a
  write-confinement breach is the CORRECT behavior (safety law outranks the non-fatal convenience). No change
  required.

## Notes for build-master (TASK-289)
- Read `<CardID>.lod.json` → `lod_chain[]`: apply each `{percent_triangles, screen_size}` via
  `regenerate_lod`/`set_lods` reduction. For SkeletalMesh, `percent_triangles` (fraction to KEEP) maps to the
  reduction property `NumOfTrianglesPercentage` (same 0-1 keep-fraction value); `screen_size` maps directly.
- Apply `component_defaults`: `visibility_based_anim_tick_option = OnlyTickPoseWhenRendered` +
  `enable_update_rate_optimizations = true` on the SkeletalMeshComponent (matches the TASK-285 runtime set).
- Sidecar location: shipping = `Content/RawAssets/Characters/<CardID>.lod.json` (beside `<CardID>.fbx`);
  smoke = `Cache/<CardID>/rig/smoke/`. `skeletal_mesh_asset` field names the target `SK_<CardID>`.
- No existing rig is affected by this task — the recipe is generated only when a FUTURE rig is (re)run
  through `rig_character.py`; existing rigs get their LODs via TASK-289's editor-python `regenerate_lod` path.

Blockers: 0
