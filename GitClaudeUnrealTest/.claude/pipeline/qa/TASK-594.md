# QA Report — TASK-594

**Verdict: PASS**

| severity | count |
|---|---|
| **BLOCKER** | **0** |
| WARN | 8 |
| NIT | 8 |

**Scope:** whole-file review of two untracked, never-gated `Tools/**/*.py` adds (CONVENTIONS:3464 tooling law).
**Files:** `Tools/ArtPipeline/build_warroom_props.py` (752 lines, read in full) · `Tools/ArtPipeline/rescale_refined_fbx.py` (736 lines, read in full).
**Bar applied:** the board's own (spec item 3) — *safe and honest to keep*, BLOCKER reserved for a secret leak, a network/credit call in a headless tool, a bare text `open()`, or something that destroys a repo file when re-run. **None of the four is present.** Every other finding is a follow-up, not a commit gate.

⛔ **No code edited. No engine, no editor, no MCP, no PIE, no compile, no Git, no script execution** (running the rescaler is the 27x hazard itself). Nothing model-side; no holdout touched; no token figure quoted.

*(This report is deliberately free of the two `TL-§1` trapdoor glyphs, U+2B50 and U+1F50D — they are referred to by code point only.)*

---

## 1. How this was reviewed — the instruments, so the result is reproducible

| # | instrument | result |
|---|---|---|
| 1 | `Read` both files end to end (752 + 736 lines) | full-file review, no sampling |
| 2 | `Read` `handoffs/TASK-556-artist.md`, `handoffs/TASK-555-artist.md` | the claims under test |
| 3 | `Read` CONVENTIONS `TL-§1`..`TL-§5` (`CONVENTIONS.md:3503-3594`) + the tooling law (`:3497`) | the law as written, not as relayed |
| 4 | `Grep -o '[^\x00-\x7F]'` on **both** files | the non-ASCII inventory, exact |
| 5 | `Grep` both files for `open(\|environ\|getenv\|requests\|urllib\|http\|socket\|subprocess\|token\|argv\|random\|uuid\|smart_project\|shutil\|remove(\|write_text\|read_text` | every I/O, secret and nondeterminism site |
| 6 | `Grep 'warnings'` on `rescale_refined_fbx.py` | **1 hit, total** — see WARN-3 |
| 7 | `Read` `Cache/WarRoomProps/props_report.json:165-278` + `Grep flame_centre_uu` | the shipped readbacks, measured, not relayed |
| 8 | `Glob Tools/ArtPipeline/Cache/Castle/*` | backup state on disk |
| 9 | `Grep '"target_dims_ue"\|"ucx"'` on `pipeline_manifest.json` | all 21 asset entries, for the guard-reachability analysis |
| 10 | `Grep 'def generate_ucx\|create_cube'` on `refine_trellis_glb.py` | verified the "identical construction" docstring claim at the artifact |
| 11 | `Grep 'build_warroom_props\|rescale_refined_fbx'` repo-wide | board item (4) — the reference sweep |
| 12 | `Grep '\.py\b'` on `qa/TASK-565.md` | **0 occurrences** — the premise of this task confirmed at the artifact |

---

## 2. The seven named hazards — verdict on each

### H1. The idempotence guard — **REAL, REACHABLE, FIRES.** (the highest-value check)

`rescale_refined_fbx.py:670-686`. Verified three ways:

- **Polarity is correct in both directions, by arithmetic on the actual on-disk numbers.**
  Guard predicate: `all(abs(pre_dims[i] - target[i]) <= 0.10 * target[i])`.
  - *The legitimate first run:* source `[2437.9, 2461.5, 2694.2]` vs `target_dims_ue [7326, 7380, 8082]` → deviation ~67 % → **not at target → proceeds.** Correct.
  - *A second `--factor 3.0` run today:* source is now `[7313.576, 7384.367, 8082.610]` → `|7313.576-7326| = 12.4 <= 732.6`, and likewise on Y and Z → **at target → `fail(..., code=3)` → the 27x castle never gets written.** Correct.
- **The guard runs BEFORE anything destructive.** Order in `main()`: PRE readback (:652-668) → **guard (:677-686)** → backup (:691-696) → in-memory scale (:701) → hull rebuild (:711) → **same-path export (:716)**. A fired guard exits before the backup, so it is also state-neutral on the Cache dir.
- **The reference is the right invariant.** It asks *"is the disk already where the manifest says it should be?"* — so re-deriving `target_dims_ue` by hand for a further scale-up (the documented workflow) correctly re-opens the gate rather than defeating it.
- **The two bypasses are both explicit and acceptable:** `--force` (documented, and named in the refusal message) and restoring `Cache/Castle/Castle_pre_rescale.fbx` (the documented restore path). `Glob` confirms the backup exists and that **no `Castle_pre_rescale_01.fbx` was ever created** — consistent with exactly one export-path run.
- **Reachability across the whole manifest:** all 21 `assets.*` entries carry `target_dims_ue`, so the `if target and not args.force` conditional cannot silently skip for any asset nameable today. See WARN-1 for the one-sidedness and NIT-6 for the future-entry hole.

**One real gap inside an otherwise sound guard → WARN-1** (it catches the double-run but not the wrong-`--factor` run).

### H2. `TL-§1` — the encoding law: **CLEAN, both files.**

- **`open(` count: ZERO in both files.** All text I/O goes through `Path.read_text` / `Path.write_text`:
  - `rescale_refined_fbx.py:634` `read_text(encoding="utf-8")` — **confirmed at the artifact**, not inherited from `TL-§1`'s sweep (per the RELAYED-DIAGNOSIS LAW). `:727` `write_text(..., encoding="utf-8")`. Only other file op is `shutil.copy2` (:696, binary).
  - `build_warroom_props.py:736` `write_text(..., encoding="utf-8")`. Swept from scratch: **no other text I/O anywhere in the file.**
- **Glyph inventory, exact (`rg -o` over the raw bytes, occurrence by occurrence):**
  - `rescale_refined_fbx.py` — **pure ASCII. Zero non-ASCII code points.** (Its docstring writes "27x" and "SC-34" deliberately.)
  - `build_warroom_props.py` — **20 non-ASCII code points, of exactly three kinds: em dash U+2014 x15, section sign U+00A7 x4 (`:8` x2, `:18` x2), no-entry U+26D4 x1 (`:15`).** **U+2B50: ZERO. U+1F50D: ZERO** — neither of the two glyphs that actually crash is present in either file.
  - Per `TL-§1`'s corrected table, all three characters present mojibake rather than crash, so the five-byte trapdoor (`0x81 0x8D 0x8F 0x90 0x9D`) is **unreachable** here.
  - **18 of the 20 sit in docstrings/comments.** The only two that can reach a stream at runtime are the em dashes at `:653` and `:706` (the two `report["warnings"].append(...)` strings) — both cp1252-encodable, so even a cp1252 stdout would corrupt rather than raise. Recorded because the sweep ran, not because it found something.
- **Neither `.py` is ever opened as data by anything** (instrument 11), so the source encoding cannot become another tool's decode problem.

### H3. `TL-§2` — hull count read back, and zero as a STOP: **half met in both files → WARN-2, WARN-7.**

- **Read back and reported: YES, in both.** `rescale`: `ucx_count` (:306), `ucx_vs_manifest_mismatches` (:334), plus the non-tautological `ucx_drift` (:624) and `interior_hull_free` (:569). `props`: the full node inventory via `probe_fbx` (:614) and `ucx_nodes` (:616), and the shipped `props_report.json:275-277` records `["UCX_SM_WarTable_00"]`.
- **Zero as a STOP: NOT implemented in either script.** `rescale` prints `RESCALE_OK` (:732) unconditionally; `props` exits 0 regardless. A `ucx_count: 0` or an empty `ucx_nodes` produces a green-looking run in both.
- **Correctly scoped, though:** `TL-§2`'s acceptance rule binds **the import** (`reimport_meshes.py`'s readback), and TASK-566 performed it. These two files are authoring-side, so this is a defence-in-depth gap, **not an unguarded path to a collisionless building**.
- **"Zero on purpose":** `SM_Torch` is named in `TL-§2` itself as *required* to carry zero collision, so the torch's absent hull is declared by the law even though the props are deliberately manifest-free. The script does not record that intent in its own report — folded into WARN-7's fix, not scored separately.

### H4. The `UCX_<render-node-name>_NN` binding mechanism: **binding correct by construction; detection missing; and the recorded MECHANISM does not reproduce from this source.**

- **Correct by construction:** `build_table_ucx(table.name)` is called at `:678` — *after* the rename at `:387` — so the hull name is derived from the render object's final name and cannot drift. Name is `f"UCX_{mesh_name}_00"` (:414) ⇒ `UCX_SM_WarTable_00`. Byte-exact match.
- **Detection: absent** (WARN-7). Nothing asserts "exactly one render node" or "exactly one UCX node", and `probe_fbx` filters to `MESH` (`:588`), so a stray Null/LimbNode would be invisible to the only instrument in the file.
- **Evidence that cuts the other way, and it matters for TASK-588:** the probe recorded the **complete** mesh-node inventory of the written `WarTable.fbx` as exactly `{SM_WarTable, UCX_SM_WarTable_00}` (`props_report.json:217-277`). Tracing the source confirms it: one object survives the `join` (:385-388), one hull is added, and `export_fbx` uses `use_selection=True` over exactly `[table, ucx]` (:707). **The only other `WarTable` token this generator can put in the file is the MATERIAL name** — `make_material("WarTable", ...)` at `:349`, which is the spec-mandated slot name (`WarTable` -> `M_WarTable`) and therefore cannot simply be renamed away.
  ⇒ `TL-§2`'s stated cause (*"the WarTable FBX carries BOTH a `WarTable` node AND an `SM_WarTable` node"*) **is not reproducible from this source.** If the real trigger is the material/object name-stem collision rather than a duplicate render node, then TASK-588's "assert one render node" repair **would pass this file and change nothing.** ⛔ Reported as an evidence-bounded observation for the manager, **not asserted as fact** — I have no engine, did not open the FBX binary, and cannot re-measure the importer.

### H5. Determinism / no surviving non-reproducible call: **CLEAN, with one docstring overclaim (NIT-2).**

- **`bpy.ops.uv.smart_project` is not called anywhere.** Two textual mentions only: the docstring finding (:221) and a stale error string (:239, NIT-1). The shipped path is `uv.cube_project` (:233) + `pack_islands(rotate=False, margin=0.02, shape_method="AABB")` (:235) — no search, no rotation, deterministic.
- No `random`, no `uuid`, no time/clock seed, no `os.environ`, and no dict/set-order dependence in any output path (the `set()`s at :292-293 are membership tests only; face iteration at :296 walks `bm.faces` in creation order).
- `rescale`: `np.polyfit`, `np.median`, fixed-step probe loops — all deterministic. The proportional-sampling comment at `:595-600` is a genuine methodology fix (a fixed inset made an exactly-x3 geometry report drift ratios of 7 and 39), correctly reasoned and correctly implemented via `np.linspace(0.05, 0.95, 21)`.
- The one residual nondeterminism (export-time triangulation choosing different diagonals) is real, is measured benign in the handoff, and is **contradicted by the shipped docstring** — NIT-2.

### H6. `flame_centre_uu` = `(50, 0, 75)`: **TRUE of the code that ships. Verified by hand, then confirmed against the artifact.**

The comment cited in the dispatch as `:290` now lives at **`:325`**; the geometry that backs it is `:293-294`:

```python
flame_faces = set(add_lathe(bm, 50, 0,
                            [(0, 46), (9, 53), (15, 66), (12, 82), (5, 95), (0, 104)], 10))
```

Lathe about `(cx=50, cy=0)`, max radius **15**, 10 spin steps ⇒ angles are multiples of 36 deg, so cos = ±1 is hit exactly and sin peaks at sin(72 deg) = 0.95106:

| axis | derivation | value |
|---|---|---|
| X | `50 ± 15·cos(0 deg / 180 deg)` | `[35, 65]` → centre **50.0** |
| Y | `± 15·sin(72 deg) = ±14.2658` | `[-14.27, 14.27]` → centre **0.0** |
| Z | profile endpoints | `[46, 104]` → centre **75.0** |

`props_report.json:38-54` records exactly `flame_bounds_uu [[35.0, -14.27, 46.0], [65.0, 14.27, 104.0]]` and `flame_centre_uu [50.0, 0.0, 75.0]`. **Claim holds.**
Downstream note, out of scope for this gate: `Torch.h:236` still ships `TorchLightRelativeOffset = FVector::ZeroVector` and `Torch.cpp:201` applies the offset only when non-zero, so `(50, 0, 75)` remains an unlanded BP-side seam (TASK-569's). Nothing in this commit changes that.

### H7. Concurrent-edit safety on `pipeline_manifest.json`: **CLEAN — neither file can clobber it.**

- `build_warroom_props.py`: **zero manifest I/O.** Two comment mentions only (`:15`, `:408-409`) — which is exactly what `TL-§1`'s own sweep recorded, and it holds under a from-scratch grep.
- `rescale_refined_fbx.py`: **reads only** (`:634`). The sole `write_text` in the file targets `Cache/<Asset>/rescale_report.json` (`:727`). There is no code path in either file that writes the manifest — consistent with `TL-§1`'s finding that the tree contains **no manifest writer at all**.

---

## 3. Findings

### BLOCKER — none

### WARN

- **[WARN-1]** `rescale_refined_fbx.py:670-686` — **the idempotence guard is one-sided.** It only asks *"is the SOURCE already at target?"*; it never asks *"does source x factor LAND on target?"*. So a wrong `--factor` (e.g. `2.0` against a manifest already re-derived for `3.0`) passes the guard, same-path-overwrites `Content/RawAssets/Castle.fbx` with a mesh at the wrong scale **inside manifest hulls that are still 3x**, and the run still prints `RESCALE_OK`. **Fix — one predicate that subsumes the existing one:** require `abs(pre_dims[i] * factor - target[i]) <= 0.10 * target[i]` for all i. Verified against the real numbers: it **passes** the shipped run (`2437.9 x 3 = 7313.7` vs `7326`, -0.17 %), **refuses** the double-run (`21940` vs `7326`, +199 %) and **refuses** `--factor 2.0` (`4875.8`, -33 %).
- **[WARN-2]** `rescale_refined_fbx.py:288-732` — **`verify()` is a RECORDER, not a GATE.** `dims_vs_target_pct` (:316), `ucx_vs_manifest_mismatches` (:334), `tris_delta`/`verts_delta` (:310-311), `approach_max_step_ue` (:363) and `interior_hull_free` (:569) are all computed, written to JSON, and **never compared to anything**; `:732` prints `RESCALE_OK` unconditionally. The docstring's promise (*"re-imports what it wrote and MEASURES it"*) is kept; the implied verdict is not. A wrapper — or a hurried reader — that treats `RESCALE_OK` as a pass is reading a `print`. **Fix:** exit non-zero on any of: target deviation > 10 %, non-empty mismatch list, `ucx_count == 0`, `approach_max_step_ue > 40`.
- **[WARN-3]** `rescale_refined_fbx.py:647` — **`report["warnings"]` is initialised and never appended, anywhere in the file** (grep: exactly one hit, the initialiser). So `"warnings": []` in `rescale_report.json` is a hardcoded literal that reads as a measured result — in a report whose stated contract is *"nothing below is asserted"* (`:289`). **Fix:** delete the key, or populate it from WARN-2's checks. (The sibling script's `warnings` array **is** real — four producers — which makes the two reports look alike while meaning different things.)
- **[WARN-4]** `rescale_refined_fbx.py:321-334` — **`ucx_vs_manifest_mismatches` is tautological.** The hulls are generated *from* the manifest at `:711` and then compared *against the same manifest* at `:330`. It can only detect FBX round-trip corruption — it can **never** detect a manifest that disagrees with the mesh, which is the failure the collision authority exists to prevent. The handoff's *"hulls vs manifest: 0 mismatches"* row is therefore true but much weaker than it reads. The real signal lives in `ucx_drift` (:624) and `interior_hull_free` (:569). **Fix:** rename the field (`ucx_roundtrip_mismatches`) and/or state the bound in the report.
- **[WARN-5]** `build_warroom_props.py:292-297` with `:156` — **the material tagging depends on `BMFace` references surviving a later whole-mesh topology op.** `wood_faces` is captured at `:292`, and `add_lathe` (the flame, `:293`) then runs `bmesh.ops.remove_doubles(bm, verts=all, dist=1e-5)` at `:156` across the entire mesh. If any merge ever invalidated those faces, `face in wood_faces` (`:297`) would silently fail, the wood tag would vanish, and **the vertex-colour ACCENT mask would be lost with no error** — precisely the degradation the polarity law is designed around. It held this run (60 wood loops = 40 side + 20 cap loops = the cone's full face set, cross-checked against `props_report.json`), but it holds by coordinate luck, not by construction, and **nothing asserts `loops_wood_red0 > 0`**. **Fix:** assign `material_index` immediately after each group is created, or assert the recorded loop counts are non-zero.
- **[WARN-6]** `build_warroom_props.py:81-84` — **`reset_scene()` removes OBJECTS only, never mesh/material datablocks**, so the round-trip probe re-imports into a session that still holds the authored names. Consequence, visible in the shipped artifact: `props_report.json:199-200,240` reads back **`TorchBody.001` / `TorchFlame.001` / `WarTable.001`** — the one readback TASK-566 must trust for the slot-name-and-order contract is polluted by the script's own leftovers, and the handoff had to fall back to a manual binary string scan to establish the true names. A verification step that has to be explained away is half a verification. **Fix:** `bpy.data.orphans_purge(do_recursive=True)` inside `reset_scene`, or probe in a fresh scene. (Contrast `rescale_refined_fbx.py:91`, which uses `wm.read_factory_settings(use_empty=True)` and is clean — which is also why *its* `endswith(f"_{i:02d}")` hull matching at `:322` cannot be broken by a `.001` suffix.)
- **[WARN-7]** `build_warroom_props.py:580-617` — **no export-time node-name assertion, and the only instrument is blind to non-mesh nodes.** `probe_fbx` skips everything that is not `MESH` (`:588`) and nothing checks *exactly one render node* / *exactly one `UCX_` node* / *`UCX_` stem == render-node name* / *hull count is non-zero-or-declared*. This file is the natural home for the repair `TL-§2` names as its leading candidate (boarded as **TASK-588**) — this is its concrete insertion point. See H4 for evidence that the recorded mechanism may be mis-attributed; that should be re-measured before TASK-588 is specified.
- **[WARN-8]** `build_warroom_props.py:624, 650-653, 703-706, 740-742` — **the four real checks have no teeth.** The tri-budget and origin-plane checks append to `report["warnings"]` and are then merely `log`ged; **the process still exits 0.** A wrapper that trusts the exit code cannot distinguish a clean run from one that just shipped an off-origin prop — and those origin planes are load-bearing for TASK-562 (`min-X = 0` is the wall-mount face) and TASK-559 (`min-Z = 0` is the floor plane). The shipped run recorded `"warnings": []` (`props_report.json:170`), so nothing was masked this time. **Fix:** `sys.exit(1)` when `report["warnings"]` is non-empty.

### NIT

- **[NIT-1]** `build_warroom_props.py:239` — stale diagnostic: `raise RuntimeError(f"{obj.name}: smart_project produced no UV layer")` names the function the determinism fix deliberately removed. A future reader debugging that message will hunt for a call that is not there. Say `cube_project`.
- **[NIT-2]** `build_warroom_props.py:219-228` — the docstring claims a re-run *"reproduces the FBX byte-for-byte apart from the FBX header's creation timestamp"*; the handoff (§9.4) **measured the opposite** — export-time triangulation picks different diagonals, so loop order changes and the bytes differ (proven benign: identical vertex positions, identical UV multiset). The shipped file should carry the measured claim, not the stronger one: a future engineer who diffs two runs will either read a benign byte-diff as a defect, or "disprove" the docstring and distrust the whole determinism fix.
- **[NIT-3]** `build_warroom_props.py:283-288` — `iron_faces` is accumulated three times and never read. Dead.
- **[NIT-4]** `build_warroom_props.py:571-574` — `render_set` restores `hide_render` on every mesh at the end, but not under `try/finally`; a render exception leaves the scene's visibility flags flipped. Cosmetic only (the run aborts anyway).
- **[NIT-5]** `build_warroom_props.py` — **no CLI at all**: no `argparse`, no `--check`, no arguments; the "CLI" is the `blender --background --python` line and every path is a module constant. Defensible for a one-shot generator, and it claims no `--check`, so board criterion (c) is not violated — but it means the F4 lever the handoff advertises (*"re-run with the X half-extent at 150"*) requires **editing the source**, which is exactly the kind of edit that lands untested. A single `--table-half-x` argument would make that offer safe.
- **[NIT-6]** `rescale_refined_fbx.py:676-677` — the guard is conditional on `params.get("target_dims_ue")` being truthy and is silently skipped if absent; `verify()` then `KeyError`s on the same key at `:315`, i.e. **after** the same-path export. Not reachable today (all 21 manifest assets carry the key — checked), but the TASK-555 handoff explicitly invites TASK-567 to reuse this tool for the crumble trio, and a new entry without that key would land in exactly that hole. **Fix:** make it required — `fail()` when missing, before the export. *(Related and benign: an asset with `ucx: null` — all 17 unit entries — raises at `:711` `params["ucx"]["boxes"]`, which is **before** the destructive write at `:716`, so that path is state-neutral. `refine_trellis_glb.py:1219-1221` guards it; this copy does not.)*
- **[NIT-7]** `rescale_refined_fbx.py:85-86, 72-74, 735` — three small contract wrinkles: `--force`'s argparse help says *"see IDEMPOTENCE below"* and there is no "below" in `--help` output; `fail(..., code=3)` is not a code the shell can rely on under `blender --python-exit-code 1` (Blender may substitute its own), so refusals should be distinguished by the `FAIL:` line, not the number; and `main()` is called bare at module scope with no `if __name__ == "__main__":` guard (its sibling has one at `:745`), which makes the module un-importable for inspection.
- **[NIT-8]** `rescale_refined_fbx.py:474-480` — `gate["visual_open_width_max"]` is `max(xs) - min(xs)` over the unobstructed sample columns, which assumes the opening is **contiguous**; a central pier would be reported as one wide opening rather than two narrow ones. `len(xs)` is recorded alongside, so a careful reader can detect it — but the published *"1470 uu widest"* is a max span, not a proven clear span. *(Also `:639,:642`: `args.asset` is interpolated into two paths; it is constrained to manifest keys by the `:635-637` lookup, which runs first, so a traversal would need a hostile manifest key. Noted for completeness only.)*

---

## 4. Checked and clean — reported as results, because "nothing further" is a result

| board criterion / law | result |
|---|---|
| **(a) `TL-§1` bare text `open()`** | **ZERO `open(` in either file.** `rescale:634` `read_text(encoding="utf-8")` **confirmed at the artifact**; `rescale:727` and `props:736` `write_text(..., encoding="utf-8")`. `build_warroom_props.py` swept from scratch: no text I/O beyond `:736`. |
| **(b) SECRETS** | **No hits of any kind.** No `os` import in either file, no `environ`/`getenv`, no `HF_TOKEN` / `MESHY_TOKEN` / `hf_` string, nothing on argv but `--asset/--factor/--manifest/--no-previews/--force`, nothing logged. **There are no benign hits to name — the token surface is empty.** |
| **(c) CLI + exit codes** | `rescale`: proper `argparse` behind the `--` separator (`:77-87`), required `--asset/--factor`, sensible defaults, `fail()` -> `sys.exit`. `props`: no CLI, no `--check` claimed; top-level `try/except Exception` -> `traceback.print_exc()` + `sys.exit(1)` (`:745-751`). Gaps recorded as WARN-8 / NIT-5 / NIT-7. |
| **(d) NETWORK + CREDITS** | **Zero network surface in either file.** No `requests`, `urllib`, `http`, `socket`, `subprocess`, `gradio_client`. Imports are `argparse/json/math/shutil/sys/pathlib` + `bpy/bmesh/numpy/mathutils` (rescale) and `json/math/sys/pathlib` + `bmesh/bpy/numpy/mathutils` (props). **The zero-credits / offline claim is verified in the source.** |
| **(e) Re-runnability** | `props` is **idempotent by construction** — a pure generator built from literal constants that never reads its own previous output. `rescale` is **not, by nature**, and carries the guard verified in H1. The contrast is the correct design in both cases. |
| **Write confinement** | Every write site enumerated. `props`: `Content/RawAssets/{Torch,WarTable}.fbx`, `Cache/WarRoomProps/props_report.json`, `Cache/WarRoomProps/previews/*.png`. `rescale`: `Content/RawAssets/<Asset>.fbx` (declared same-path), `Cache/<Asset>/<Asset>_pre_rescale*.fbx`, `Cache/<Asset>/rescale_report.json`, `Cache/<Asset>/previews_<N>x/*.png`. **No escape, no `.blend` save, no `Source/`, no `.uasset`, no `L_Arena`, and no write into another chain's territory — `Content/RawAssets/CardArt/` and `/Game/UI/CardArt/` untouched (lane-isolation law).** `REPO` / `REPO_ROOT` are both derived correctly from `__file__`. |
| **Backup discipline** | `rescale:691-696` never clobbers an existing backup — it walks `_pre_rescale_NN` — so the FIRST (true pre-scale) source always survives. `Glob` confirms only `Castle_pre_rescale.fbx` exists. |
| **`generate_ucx` fidelity** | `rescale:183-204` is construction-identical to `refine_trellis_glb.py:1215-1236` — same `UCX_SM_{card_id}_{index:02d}` format, same unit conversion, same `create_cube(size=1.0)` + per-vertex scale/offset. **The docstring claim is verified at the artifact**, not taken on trust. |
| **Handedness involution** | `props:450-458` and `rescale:102-111` are the same `Matrix.Diagonal((1,-1,1,1))` + winding-flip involution, and both are called in a `try/finally` around the export so the scene is restored even on failure (`props:464-486`, `rescale:209-231`). Correct. |
| **Headless-bpy** | No UI-context assumptions: no `bpy.context.area` / `space_data` / `window`, no `outliner` or `screen` ops. `bpy.context.view_layer` / `scene` are valid under `--background`. The UI-adjacent ops used (`object.mode_set`, `object.join`, `uv.cube_project`, `uv.pack_islands`, `render.render`) are **proven on this build by the shipped artifact** — `props_report.json` carries `uv_layers: ["UVMap"]`, the joined 496-tri table with both vertex-colour islands, and 16 previews on disk — not merely by inspection. |
| **Board item (4) — reference sweep** | Repo-wide grep for both script names: hits **only** in `.claude/pipeline/**` docs and in the two files themselves. **No `Source/`, no other `Tools/*.py`, no `pipeline_manifest.json` reference.** Nothing at runtime depends on either file — confirmed. |
| **Task premise** | `qa/TASK-565.md` contains **zero** occurrences of `.py`. The board's statement that neither file was ever gated holds at the artifact. |

---

## 5. Notes for build-master (TASK-595)

1. **PASS — the commit gate is satisfied.** 0 BLOCKER. The 8 WARN / 8 NIT are follow-ups; none makes either file unsafe to keep, and per board item (3) the artifacts these scripts produced are already in history at `93c5ec8` and cannot be un-shipped by this review.
2. **Exactly two source paths**, staged as explicit file pathspecs: `Tools/ArtPipeline/build_warroom_props.py` and `Tools/ArtPipeline/rescale_refined_fbx.py` — plus `.claude/pipeline/qa/TASK-594.md` and your handoff. **Both are `.py`: expect `git check-attr filter` = `unspecified`** (not LFS), so a byte-size / `git diff` check is a legitimate instrument on these two.
3. **Independently confirmed for you:** no committed file anywhere in the tree references either script (instrument 11). This commit changes no runtime behaviour, no build, no asset — it is a pure record-keeping commit.
4. ⛔ **Nothing else may ride along** — no `.cpp`/`.h` (TASK-596 stays out), no `.uasset`, no `.umap`, no `Content/`. Note that both scripts *can* write into `Content/RawAssets/`; **do not run either one** — `rescale_refined_fbx.py` is the 27x hazard, and `build_warroom_props.py` would rewrite two committed FBXs with different bytes for identical geometry (NIT-2).
5. **Nothing model-side was touched by this gate**; the sealed holdout is untouched and no token figure appears in this report.

## 6. Follow-ups for the manager (not for this commit)

- **WARN-1's two-line predicate** is the highest-value repair in either file, and it is cheaper than the guard it replaces.
- **WARN-7 + H4** give **TASK-588** both a named insertion point and a reason to re-measure its premise first: the evidence on disk says this generator emits **one** render node, so the `WarTable` / `SM_WarTable` collision recorded in `TL-§2` may be the **material** name rather than a duplicate node. If so, the "assert one render node" repair passes this file and fixes nothing. **Diagnose before specifying.**
- **WARN-2 / 3 / 8** are one family: both scripts compute good numbers and then decline to have an opinion about them. A shared `fail_on(report["warnings"])` convention across `Tools/ArtPipeline/**` would close it once.

## 7. Board status flip requested (I do not edit the board)

- **TASK-594** (`TASKBOARD.md:8719`): `status: backlog — DISPATCHABLE IMMEDIATELY` -> **`status: qa-passed`** — PASS, 0 BLOCKER / 8 WARN / 8 NIT, report at `.claude/pipeline/qa/TASK-594.md`.
- **TASK-595** (`TASKBOARD.md:8743`): its hard gate is satisfied — **unblocked to dispatch.**
