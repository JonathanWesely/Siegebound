# TASK-083 Handoff — Stage-2 refine tooling: refine_trellis_glb.py + pipeline_manifest.json + guard-secrets hf_ pattern (gameplay-programmer)

Status: **files authored, ready-for-qa.** FILE-ONLY task per spec: no Unreal editor, no C++ compile, no Git mutation, no full pipeline run. **Full round-trip validation (headless Blender run against a real mesh) is TASK-084's job — treat everything here as authored-and-syntax-smoked, not runtime-proven.**

## Files

| File | What |
|------|------|
| `Tools/ArtPipeline/refine_trellis_glb.py` | NEW — headless bpy Stage-2 refine (runs INSIDE Blender's Python; stdlib+numpy+bpy only, no pip deps) |
| `Tools/ArtPipeline/pipeline_manifest.json` | NEW — per-asset params; 3 pilot rows: Footman, Archer, Castle |
| `.claude/hooks/guard-secrets.sh` | ONE-LINE edit — `hf_[A-Za-z0-9]{20,}` added to the high-confidence token alternation (line 12); nothing else touched |

No overlap with TASK-082's files (scaffold/trellis_generate.py landed in parallel — verified disjoint) and nothing written under `Content/RawAssets/CardArt/` (lane-isolation ruling 4).

## Invocation

```
"<blender.exe>" --background --factory-startup --python-exit-code 1 ^
    --python Tools/ArtPipeline/refine_trellis_glb.py -- --card-id Footman
```

Flags: `--card-id` (required, manifest key) · `--manifest` · `--input` (mesh override, .glb/.gltf/.fbx) · `--mode bake|native` · `--smoke` (write confinement for TASK-084) · `--quick` (256² bakes, fast) · `--save-blend` (debug .blend to Cache). Exit codes: 0 OK / 1 stage failure / 2 usage-manifest-input error (sys.exit propagates out of `--python` and sets Blender's exit code; `--python-exit-code 1` additionally covers any uncaught exception).

**For TASK-084's round-trip smoke:** `-- --card-id Footman --input Content/RawAssets/Footman.fbx --smoke --quick` — `--smoke` reroutes the FBX to `Cache/Footman/smoke/Footman.fbx` and textures to `Cache/Footman/smoke/Textures/Footman/`; shipping `Content/RawAssets/**` is never written. Previews + `refine_report.json` land in `Cache/Footman/` as always.

## Stage walkthrough (mode "bake")

1. **IMPORT** — `Cache/<CardID>/trellis_raw.glb` (WebP textures fine in Blender 5.1; `.fbx` also accepted for smoke runs). Non-mesh and `UCX_*` import nodes dropped; world transforms baked into mesh data BEFORE parent empties are removed; multiple meshes joined to one.
2. **CLEANUP** (bmesh, no edit-mode ops) — `remove_doubles` (weld dist = `weld_distance_rel` × max dim), floating-island removal (union-find over verts through edges; components under `island_min_face_fraction` of the largest deleted), `holes_fill` (≤ `hole_fill_max_sides`), `recalc_face_normals`.
3. **CONFORM** — optional `pre_rotate_z_deg` so the front faces Blender −Y (blockout facing contract); scale per `fit_mode` (`height` = uniform on Z target for units, `box` = exact per-axis for buildings); origin = XY-bbox-center at (0,0), min Z = 0 (feet-center and ground-center are the same rule per TASK-014/037/038); transforms applied into mesh data (`mesh.transform` + identity matrix — no ops context needed). Dims vs target checked against `dims_tolerance` (±10%); deviation = report WARNING.
4. **SPLIT** — duplicate: dense donor (`<CardID>_DenseSource`) keeps original geometry/UVs/materials for baking; the copy becomes the low target, object AND mesh data named exactly `SM_<CardID>` (FBX render-node contract).
5. **REMESH/DECIMATE** — voxel remesh at `voxel_size_ue` then Decimate modifier applied to hit `tri_budget` (Footman/Archer 15k, Castle 40k). Over-budget after decimate = WARNING.
6. **UV** — `uv.smart_project` (66°, margin 0.02), UV layer renamed exactly **`UVMap`**, extra layers removed (TASK-038 MikkTSpace lesson).
7. **BAKE** — Cycles CPU, `use_selected_to_active=True` (dense selected, low active), `cage_extrusion`/`max_ray_distance` from manifest. Passes in this order: DIFFUSE (`pass_filter={'COLOR'}`) → `T_<CardID>_D` (sRGB); ROUGHNESS; NORMAL (tangent) → `T_<CardID>_N`; AO at 64 spp; **metallic via EMIT-rewire LAST** (Cycles has no metallic pass — each dense material's Metallic input is rerouted into an Emission shader wired to the output, then baked as EMIT; destructive to the donor, which is discarded after).
8. **ORM** — numpy-pack R=AO, G=Roughness, B=Metallic → `T_<CardID>_ORM` (Non-Color/linear). Individual AO/R/M debug PNGs → `Cache/<CardID>/bake_debug/` (never Content).
9. **SLOTS** — two-slot contract: slot 0 material named exactly `TeamRegion` (minority face-set from manifest selectors; preview-tinted with the MI_TeamColor_Blue linear color), slot 1 `<CardID>PBR` (Principled wired D → BaseColor, N → NormalMap(UVMap), ORM → SeparateColor G→Rough B→Metal).
10. **UCX** — buildings only: `UCX_SM_<CardID>_00..` box hulls from the manifest footprint spec, exported in the FBX (`hide_render=True` keeps them out of previews).
11. **EXPORT** — FBX → `Content/RawAssets/<CardID>.fbx` (existing blockout path, same-path swap law) with `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True`, `mesh_smooth_type='FACE'`; texture PNGs → `Content/RawAssets/Textures/<CardID>/T_<CardID>_D|_N|_ORM.png`.
12. **REPORT** — Workbench ortho previews (front/back/¾/top, TEXTURE color, view transform Standard) + one small Cycles beauty → `Cache/<CardID>/previews/`; `refine_report.json` → `Cache/<CardID>/` with tris, bounds (UE), min-Z, `uv_layer_ok`, texture inventory, material slots, team-region fraction, UCX names, warnings, per-stage stats, elapsed time.

**Mode "native"** (escape hatch for bake artifacts): skips destructive cleanup, remesh/decimate, and all baking — keeps TRELLIS's mesh/UVs/textures. Conform, UVMap rename (smart-project only as a no-UVs fallback), slot split, UCX, export, previews, report still run. Textures are extracted from the imported glTF materials and saved as PNG (WebP→PNG happens at save); the ORM is numpy-**recomposed** from glTF metallicRoughness (G=rough, B=metal) + occlusion so the R=AO/G=R/B=M channel law always holds regardless of source packing. Over-budget tris = WARNING, never a decimate (decimating would degrade the kept UVs).

## Manifest schema (`pipeline_manifest.json`)

`defaults` merge under per-asset overrides (dict values merge one level deep). All `*_ue` values are UE units (cm); script works in Blender meters (÷100) and exports `apply_unit_scale=True` like the blockouts. Per asset:

- `category` — `unit` | `building` (buildings get UCX + a warning if `ucx` is missing)
- `mode` — `bake` | `native` (CLI `--mode` overrides)
- `tri_budget` — 15000 units / 40000 castle (law)
- `bake_resolution` — 1024 units / 2048 buildings (law)
- `origin` — `feet-center` / `ground-center` (documentation; both = XY-center, minZ 0)
- `fit_mode` — `height` (uniform, units; X/Y deviation warn-only) | `box` (exact, buildings)
- `target_dims_ue` — Footman [147, 80, 180] (TASK-014), Archer [125, 67.3, 180] (TASK-037), Castle [814.5, 820, 900] (TASK-013)
- `pre_rotate_z_deg` — front-facing correction knob (0 until first live TRELLIS output is seen)
- `voxel_size_ue`, `weld_distance_rel`, `island_min_face_fraction`, `hole_fill_max_sides`
- `uv` — `angle_limit_deg` 66, `island_margin` 0.02
- `bake` — `margin_px` 16, `samples` 8, `ao_samples` 64, `cage_extrusion_ue` 3 (castle 8), `max_ray_distance_ue` 8 (castle 24)
- `team_region` — `max_fraction` + `selectors[]`; each selector has optional `box` (normalized conformed-bounds coords: x 0=−X…1=+X, y 0=front(−Y)…1=back(+Y), z 0=ground…1=top; face-center containment) and/or `normal` (`{direction, min_dot}`); AND within a selector, OR across selectors
- `ucx` — `null` for units (their ≤4 simple hulls are generated at Stage-3 import per the TASK-037 recipe); buildings carry `boxes[]` of `{center, size}` in UE units

Castle UCX row: 4 perimeter wall boxes (thickness 100, height 300) + 4 corner-tower boxes (200×200×500, flush with the outer corners) + central keep (360×360×600) = 9 hulls, **no full-base slab** (M1 ~410-unit dead-zone lesson honored).

## Flagged decisions for QA (explicit)

1. **`use_triangles=True` on FBX export** — one arg beyond the axis contract's four. Rationale: the low mesh is effectively all-tris after decimate anyway, and exporting the exact baked triangulation avoids normal-map shading mismatches from UE re-triangulating quads. The contract args themselves are verbatim.
2. **"Scale to manifest dims" interpretation** — units use uniform height fit (Z exact, X/Y warn-only; a TRELLIS mesh will never match blockout widths that include weapons), buildings use exact box fit (trivially satisfies the ±10% bounds ruling). `fit_mode` is per-asset in the manifest.
3. **Feet-center == ground-center implementation** — both blockout handoffs define them identically (XY bbox center, minZ = 0); one code path, `origin` field kept for documentation.
4. **Team-region selectors are starting guesses** — normalized-bounds boxes/normal filters authored against not-yet-generated meshes (helm crest + shield band for Footman; cap + quiver for Archer; upward-sloped roof faces for Castle). Zero-face or over-`max_fraction` selections WARN in the report instead of failing — the TASK-086/087 pre-import eyeball gate is the ruling authority, and slot 0 `TeamRegion` exists regardless.
5. **Bake params** — AO 64 spp (spec), all other passes 8 spp, denoising OFF for bakes (ON only for the beauty preview), margin 16 px, cage extrusion 3 UE units / castle 8, max ray 8 / castle 24. No physical cage object — extrusion+ray only.
6. **Bake order D→R→N→AO→EMIT(metal)** — the metallic EMIT-rewire destroys the dense materials, so it is last; the donor is deleted immediately after.
7. **All-smooth shading on the low mesh** before baking (continuous tangent basis), exported with `mesh_smooth_type='FACE'` = a single smoothing group for UE's MikkTSpace recompute.
8. **Castle UCX numbers are derived, not measured** — from TASK-013's blockout description (perimeter wall ring, corner towers, central keep). They satisfy the wall-footprint-exact ruling by construction but should be tuned against the actual generated mesh at TASK-087.
9. **Native-mode ORM is always recomposed via numpy** (never a straight copy of the glTF MR texture) so the R=AO/G=Rough/B=Metal law holds; missing normal texture skips `T_*_N` with a WARNING (Stage 3 imports from the report's texture inventory); missing MR falls back to constant rough 0.8 / metal 0.
10. **Unwired Metallic sockets** bake as the Principled input's default value (flat emission) — correct for materials that set metallic as a scalar.
11. **Write confinement is code, not convention** — every output funnels through `OutputGuard.check()`: allowed roots are `Tools/ArtPipeline/Cache/<CardID>/` plus (non-smoke only) `Content/RawAssets/`; any path containing a `CardArt` segment hard-fails (ruling 4). Debug intermediates (AO/R/M PNGs, optional .blend) go to Cache only.
12. **Preview color management forced to `Standard`** (not AgX/Filmic) so texture previews read truthfully at the eyeball gate.
13. **Headless pitfalls handled per spec** — `view_layer.objects.active` set via `select_only()` before every `mode_set`/`modifier_apply`/`voxel_remesh`/`smart_project`/`bake`; bake target image node created in the low mesh's sole temp material AND set `nodes.active` before every bake; `use_selected_to_active=True` on all bakes; transforms applied via `mesh.transform` instead of ops.
14. **guard-secrets** — exactly one line changed (the high-confidence alternation); pattern `hf_[A-Za-z0-9]{20,}` inserted between the GitHub and Slack patterns. Existing patterns byte-identical.

## Syntax smoke evidence (authorized: own-script smoke only)

Ran `blender.exe 5.1.2 --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py` three ways:
- `--card-id Footman` → clean exit **2**, `input mesh not found … run Stage 1 … or pass --input` (validates full module parse, bpy+numpy imports, argparse, manifest load, Footman param merge incl. nested `team_region`)
- `--card-id Castle --smoke` → clean exit **2**, same gate (validates Castle row incl. nested `bake` override merge and smoke-path setup)
- `--card-id Bogus` → clean exit **2**, `card-id 'Bogus' not in manifest; available: ['Archer', 'Castle', 'Footman']`

Zero filesystem writes occurred (guard only mkdirs on actual writes; all runs exited at the input gate). Everything past the input gate (import/cleanup/bake/export) is **unexercised** — TASK-084's round-trip smoke covers it.

## What QA should scrutinize

- Secret handling: this script touches no tokens at all — verify no env reads/echoes crept in; verify the guard-secrets edit is exactly one line.
- Write confinement: `OutputGuard` allowed-roots logic and that every write call site (`save_png`, `export_fbx`, previews, report, debug blend) goes through `guard.check()`.
- Headless-bpy context pitfalls (decision 13) and the bake-order/EMIT-rewire correctness (decisions 5–6).
- bpy API names against Blender 5.1 (`bmesh.ops.holes_fill`, `uv.smart_project(angle_limit=radians)`, `object.bake` kwargs `cage_extrusion`/`max_ray_distance`/`pass_filter`, `remesh_voxel_size`, `ShaderNodeSeparateColor`) — authored from 4.x/5.x conventions, runtime-proven only at TASK-084.
- Manifest numbers vs the blockout handoffs (dims table above) and the castle UCX plausibility.
