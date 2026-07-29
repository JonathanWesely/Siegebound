"""
refine_trellis_glb.py — Siegebound art pipeline, Stage 2 (TASK-083).

Runs INSIDE Blender's bundled Python (bpy + numpy + stdlib ONLY — no pip deps).
Heavy work runs HEADLESS per the TRELLIS.2 chain ruling 3 (the live Blender MCP
bridge has a 30 s socket cap and must never be used for this).

Invocation (from the repo root):

    "<blender.exe>" --background --factory-startup --python-exit-code 1 ^
        --python Tools/ArtPipeline/refine_trellis_glb.py -- --card-id Footman

Flags (after the "--" separator):
    --card-id <ID>      required; must be a key of pipeline_manifest.json "assets"
    --manifest <path>   manifest override (default: pipeline_manifest.json beside this script)
    --input <path>      input mesh override (.glb/.gltf/.fbx). Default:
                        Tools/ArtPipeline/Cache/<CardID>/trellis_raw.glb
    --mode bake|native  manifest "mode" override
    --smoke             WRITE CONFINEMENT for TASK-084's round-trip smoke: the FBX and
                        texture PNGs go to Cache/<CardID>/smoke/** instead of
                        Content/RawAssets/** (shipping raw assets are never touched)
    --quick             fast-iteration: 256^2 bakes, fewer samples, coarser voxels,
                        smaller previews (pair with --smoke for the TASK-084 smoke)
    --save-blend        also save Cache/<CardID>/refine_debug.blend for inspection

Stages (mode "bake", the standard path):
    1. IMPORT   Cache/<CardID>/trellis_raw.glb (WebP textures OK in Blender 5.1);
                non-mesh and UCX_* import nodes dropped; remaining meshes joined.
    2. CLEANUP  bmesh: merge-by-distance, delete floating islands, fill small holes,
                recalculate normals.
    3. CONFORM  optional pre-rotation so the FRONT faces Blender -Y (blockout facing
                convention, TASK-014/037), scale to manifest dims (units: uniform
                height fit; buildings: exact box fit), origin to feet-center /
                ground-center (both = XY bbox center, min Z = 0), transforms applied.
    4. SPLIT    duplicate: dense source (bake donor) + low target "SM_<CardID>".
    5. REMESH   voxel remesh + decimate the low target to the manifest tri budget.
    6. UV       uv.smart_project; UV layer renamed exactly "UVMap" (the TASK-038
                MikkTSpace lesson — missing/misnamed UVs = degenerate-tangent warnings).
    7. BAKE     Cycles CPU, selected-to-active dense->low, cage/ray params from the
                manifest: DIFFUSE(color only) -> T_<CardID>_D, tangent NORMAL ->
                T_<CardID>_N, AO (64 spp default), ROUGHNESS, and METALLIC via the
                EMIT-rewire gotcha (Cycles has no metallic bake pass: each dense
                material's Metallic input is rerouted into an Emission shader and
                baked as EMIT — done LAST because it destroys the dense materials).
    8. ORM      numpy-pack R=AO, G=Roughness, B=Metallic -> T_<CardID>_ORM (linear).
                Per-pass debug PNGs also land in Cache/<CardID>/bake_debug/.
    8b. DELIGHT albedo de-light/brighten (TASK-193) — counters the recorded "dark
                TRELLIS look" (shading baked into the albedo; TASK-150/151/172
                accepted-dark precedents). numpy, applied to the D ONLY, right
                before its PNG write: linear-space AO-divide against the packed
                ORM.R occlusion (strength-blended, divisor floored) + a mild
                gamma/gain levels lift, then a C1-continuous soft-shoulder clamp so
                highlights COMPRESS instead of clipping. N/ORM stay byte-untouched.
                Conservative-ON by default (ALBEDO_DELIGHT_DEFAULTS below);
                per-asset override via optional manifest key "albedo_delight"
                (bool, or a partial object over the defaults; READ-only here —
                TASK-194 owns manifest edits). Runs in BOTH modes (a native-mode
                flat AO=1 degrades it gracefully to the levels lift alone).
    9. SLOTS    two-slot contract (CONVENTIONS "Textured mesh law"): slot 0 material
                named exactly "TeamRegion" (minority face-set from manifest selectors),
                slot 1 "<CardID>PBR" wired to the baked textures.
   10. UCX      buildings only: UCX_SM_<CardID>_00.. box hulls from the manifest
                footprint spec (castle = wall-footprint-exact, NOT a full plinth slab —
                the M1 ~410-unit dead-zone lesson).
   11. EXPORT   FBX -> Content/RawAssets/<CardID>.fbx with the axis contract
                (axis_forward='-Z', axis_up='Y', apply_unit_scale=True,
                mesh_smooth_type='FACE') PLUS the UE handedness pre-compensation
                (TASK-348 MIRROR-FIX, 2026-07-28): Blender is right-handed, UE is
                left-handed, and UE's FBX importer resolves that by negating Y on
                every vertex (FFbxDataConverter::ConvertPos = (X,-Y,Z)) — a
                rotation-only axis contract can NEVER avoid it, so every asset
                used to land Y-MIRRORED against its conformed/manifest space
                (trace-proven on the hollow castle, TASK-350). The exporter now
                bakes a compensating Y-mirror + winding flip into the exported
                mesh copies (render mesh AND UCX hulls) so engine-negation x
                pre-mirror = identity: the asset lands in UE exactly in conformed
                space and every manifest number (ucx.boxes, carve, blockers, nav)
                is a valid UE-local coordinate verbatim. Bake/textures/previews
                all run in conformed space BEFORE this and are unaffected.
                Texture PNGs -> Content/RawAssets/Textures/<CardID>/T_<CardID>_D|_N|_ORM.png.
   12. REPORT   Workbench ortho previews (front/back/three-quarter/top, TEXTURE color)
                + one small Cycles beauty + refine_report.json to Cache/<CardID>/.

Mode "native" (escape hatch for bake artifacts): skips CLEANUP-destructive ops,
REMESH and BAKE entirely — keeps TRELLIS's own mesh, UVs and textures. Textures are
extracted from the imported GLB materials and converted WebP->PNG (the ORM is
numpy-recomposed to guarantee R=AO/G=Rough/B=Metal regardless of the source packing;
the D still passes through the stage-8b albedo de-light unless disabled).
Conform, slot split, UCX, export, previews and report all still run. Over-budget tri
counts WARN instead of failing.

Headless-bpy pitfalls deliberately handled:
    * view_layer.objects.active is set (via select_only) before every mode_set /
      modifier_apply / voxel_remesh / smart_project / bake operator call.
    * The bake target Image Texture node exists in the low mesh's (sole, temporary)
      material AND is made nodes.active before every bake call.
    * All bakes pass use_selected_to_active=True with the dense donor selected and
      the low target active.
    * Object transforms are applied by transforming mesh data directly
      (mesh.transform + identity matrix) instead of relying on ops context.
    * sys.exit codes: 0 = OK, 1 = stage failure, 2 = usage/manifest/input error
      (SystemExit propagates out of --python scripts and sets Blender's exit code).

Write confinement (QA-auditable): every output path is funneled through guard_out(),
which hard-fails unless the path is under Tools/ArtPipeline/Cache/<CardID>/ or (when
not --smoke) Content/RawAssets/ — and NEVER under any path containing "CardArt"
(lane-isolation ruling 4: Content/RawAssets/CardArt belongs to TASK-077..081).

Full round-trip validation of this script is TASK-084's job — do not treat authoring
review as a runtime pass.
"""

import argparse
import json
import math
import sys
import time
import traceback
from datetime import datetime, timezone
from pathlib import Path

try:
    import bpy
    import bmesh
    from mathutils import Matrix, Vector
except ImportError:
    print("[refine] FATAL: bpy not importable — this script must run inside Blender "
          "(blender.exe --background --factory-startup --python refine_trellis_glb.py -- ...)")
    sys.exit(2)

try:
    import numpy as np
except ImportError:
    print("[refine] FATAL: numpy not importable (it ships with Blender's Python — "
          "do not run with a stripped interpreter).")
    sys.exit(2)

# --------------------------------------------------------------------------- paths

SCRIPT_DIR = Path(__file__).resolve().parent                  # Tools/ArtPipeline
PROJECT_ROOT = SCRIPT_DIR.parents[1]                          # repo root
CACHE_ROOT = SCRIPT_DIR / "Cache"
CONTENT_RAW = PROJECT_ROOT / "Content" / "RawAssets"

UE_UNITS_PER_METER = 100.0

_LOG_PREFIX = "[refine]"


def log(msg):
    print(f"{_LOG_PREFIX} {msg}", flush=True)


def fail(msg, code=1):
    print(f"{_LOG_PREFIX} FATAL: {msg}", flush=True)
    sys.exit(code)


class OutputGuard:
    """Write confinement: every output path must pass through .check()."""

    def __init__(self, card_id, smoke):
        self.cache_dir = (CACHE_ROOT / card_id).resolve()
        self.smoke = smoke
        self.allowed = [self.cache_dir]
        if not smoke:
            self.allowed.append(CONTENT_RAW.resolve())

    def check(self, path):
        rp = Path(path).resolve()
        # Lane isolation (ruling 4): the card-art chain owns CardArt — never write there.
        if any(part.lower() == "cardart" for part in rp.parts):
            fail(f"write-confinement violation: {rp} is inside a CardArt lane "
                 f"(owned by TASK-077..081)")
        if not any(rp == root or root in rp.parents for root in self.allowed):
            fail(f"write-confinement violation: {rp} is outside the allowed roots "
                 f"{[str(a) for a in self.allowed]}"
                 + (" (--smoke confines writes to Cache/)" if self.smoke else ""))
        rp.parent.mkdir(parents=True, exist_ok=True)
        return rp


# --------------------------------------------------------------------------- args / manifest

def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    parser = argparse.ArgumentParser(
        prog="refine_trellis_glb.py",
        description="Siegebound Stage-2 refine: TRELLIS GLB -> game-ready FBX + D/N/ORM PNGs.")
    parser.add_argument("--card-id", required=True,
                        help="asset key in pipeline_manifest.json (e.g. Footman)")
    parser.add_argument("--manifest", default=None, help="manifest path override")
    parser.add_argument("--input", default=None,
                        help="input mesh override (.glb/.gltf/.fbx); default "
                             "Cache/<CardID>/trellis_raw.glb")
    parser.add_argument("--mode", choices=["bake", "native"], default=None,
                        help="override the manifest's mode")
    parser.add_argument("--smoke", action="store_true",
                        help="confine FBX/texture writes to Cache/<CardID>/smoke/ "
                             "(TASK-084 round-trip smoke; never touches Content/)")
    parser.add_argument("--quick", action="store_true",
                        help="fast iteration: 256^2 bakes, fewer samples, coarse voxels")
    parser.add_argument("--save-blend", action="store_true",
                        help="save Cache/<CardID>/refine_debug.blend")
    return parser.parse_args(argv)


def load_manifest(path):
    if not path.is_file():
        fail(f"manifest not found: {path}", code=2)
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except json.JSONDecodeError as err:
        fail(f"manifest is not valid JSON: {path}: {err}", code=2)


def merged_params(manifest, card_id):
    """defaults <- per-asset overrides (one level deep for dict values)."""
    assets = manifest.get("assets", {})
    if card_id not in assets:
        fail(f"card-id '{card_id}' not in manifest; available: {sorted(assets)}", code=2)
    params = dict(manifest.get("defaults", {}))
    for key, value in assets[card_id].items():
        if isinstance(value, dict) and isinstance(params.get(key), dict):
            merged = dict(params[key])
            merged.update(value)
            params[key] = merged
        else:
            params[key] = value
    for required in ("category", "tri_budget", "bake_resolution", "origin",
                     "fit_mode", "target_dims_ue"):
        if required not in params:
            fail(f"manifest entry '{card_id}' missing required field '{required}'", code=2)
    return params


# --------------------------------------------------------------------------- bpy helpers

def select_only(objs, active=None):
    """Deselect everything, select objs, set the active object (headless pitfall:
    ops like mode_set / modifier_apply / bake NEED view_layer.objects.active)."""
    view_layer = bpy.context.view_layer
    for obj in view_layer.objects:
        obj.select_set(False)
    for obj in objs:
        obj.select_set(True)
    view_layer.objects.active = active if active is not None else (objs[0] if objs else None)


def ensure_object_mode():
    active = bpy.context.view_layer.objects.active
    if active is not None and active.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def reset_scene():
    """--factory-startup still ships a cube/camera/light; start truly empty."""
    ensure_object_mode()
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)


def mesh_bounds(obj):
    """(min, max) of the mesh in LOCAL space as numpy arrays (transforms are always
    applied into the data by this script, so local == conformed space)."""
    mesh = obj.data
    count = len(mesh.vertices)
    if count == 0:
        fail("mesh has no vertices")
    coords = np.empty(count * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", coords)
    coords = coords.reshape(-1, 3)
    return coords.min(axis=0), coords.max(axis=0)


def apply_world_transform(obj):
    """Bake the object's full world matrix into the mesh data (no ops context needed)."""
    matrix = obj.matrix_world.copy()
    obj.parent = None
    obj.data.transform(matrix)
    obj.matrix_world = Matrix.Identity(4)


def tri_count(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    return len(mesh.loop_triangles)


# --------------------------------------------------------------------------- stage: import

def import_input(input_path):
    before = set(bpy.data.objects)
    suffix = input_path.suffix.lower()
    if suffix in (".glb", ".gltf"):
        bpy.ops.import_scene.gltf(filepath=str(input_path))
    elif suffix == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(input_path))
    else:
        fail(f"unsupported input extension '{suffix}' (expected .glb/.gltf/.fbx)", code=2)
    imported = [o for o in bpy.data.objects if o not in before]
    if not imported:
        fail("importer produced no objects")

    meshes, discard = [], []
    for obj in imported:
        if obj.type == "MESH" and not obj.name.upper().startswith("UCX_"):
            meshes.append(obj)
        else:
            discard.append(obj)
    if not meshes:
        fail("input contained no usable mesh objects")
    # Apply world transforms BEFORE deleting parents (empties may carry the scale).
    for obj in meshes:
        apply_world_transform(obj)
    for obj in discard:
        bpy.data.objects.remove(obj, do_unlink=True)

    if len(meshes) > 1:
        ensure_object_mode()
        select_only(meshes, active=meshes[0])
        bpy.ops.object.join()
        merged = bpy.context.view_layer.objects.active
    else:
        merged = meshes[0]

    # Orientation guard (TASK-348): a source node with a NEGATIVE scale (mirror)
    # bakes an inside-out winding into the mesh data when transforms are applied
    # — the 2026-07-28 Meshy Castle GLB shipped exactly that (signed volume
    # -1463 m^3). An inverted dense donor makes the Cycles AO bake read ~0
    # everywhere (rays start "inside" the solid) and flips the baked tangent
    # normals. Enforce outward winding by signed volume; the voxel-remeshed low
    # self-heals either way, so this only corrects the BAKE DONOR truth.
    mesh = merged.data
    mesh.calc_loop_triangles()
    coords = np.empty(len(mesh.vertices) * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", coords)
    coords = coords.reshape(-1, 3)
    tri_idx = np.empty(len(mesh.loop_triangles) * 3, dtype=np.int64)
    mesh.loop_triangles.foreach_get("vertices", tri_idx)
    tri_idx = tri_idx.reshape(-1, 3)
    volume6 = float(np.einsum("ij,ij->i", coords[tri_idx[:, 0]],
                              np.cross(coords[tri_idx[:, 1]],
                                       coords[tri_idx[:, 2]])).sum())
    if volume6 < 0.0:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
        bm.to_mesh(mesh)
        bm.free()
        mesh.update()
        log(f"IMPORT: inverted winding detected (signed volume {volume6 / 6.0:.1f}) "
            f"-> all faces flipped outward")

    log(f"IMPORT: {input_path.name} -> 1 mesh object "
        f"({len(merged.data.vertices)} verts, {tri_count(merged)} tris, "
        f"{len(merged.data.materials)} material slot(s))")
    return merged


# --------------------------------------------------------------------------- stage: cleanup

def _delete_small_islands(bm, min_face_fraction):
    """Union-find over verts (through edges); drop components whose face count is
    below min_face_fraction of the largest component's."""
    bm.verts.ensure_lookup_table()
    bm.verts.index_update()
    parent = list(range(len(bm.verts)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for edge in bm.edges:
        ra, rb = find(edge.verts[0].index), find(edge.verts[1].index)
        if ra != rb:
            parent[rb] = ra

    face_counts = {}
    for face in bm.faces:
        root = find(face.verts[0].index)
        face_counts[root] = face_counts.get(root, 0) + 1
    if len(face_counts) <= 1:
        return 0
    largest = max(face_counts.values())
    small_roots = {r for r, c in face_counts.items() if c < min_face_fraction * largest}
    if not small_roots:
        return 0
    doomed_verts = [v for v in bm.verts if find(v.index) in small_roots]
    bmesh.ops.delete(bm, geom=doomed_verts, context="VERTS")
    return len(small_roots)


def cleanup(obj, params, report):
    mesh = obj.data
    verts_before = len(mesh.vertices)
    mn, mx = mesh_bounds(obj)
    max_dim = float(max(mx - mn))
    weld_dist = params.get("weld_distance_rel", 0.0002) * max_dim

    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=weld_dist)
    islands_removed = _delete_small_islands(bm, params.get("island_min_face_fraction", 0.01))
    fill = bmesh.ops.holes_fill(bm, edges=bm.edges,
                                sides=int(params.get("hole_fill_max_sides", 16)))
    holes_filled = len(fill.get("faces", []))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    # Orientation guard, cleanup half (TASK-348): recalc_face_normals UNIFIES a
    # mixed-winding source but can pick the INWARD side (the 2026-07-28 Meshy
    # Castle: +0.5 m^3 at import -> -0.5 after recalc). An inward bake donor
    # collapses the Cycles AO bake to ~0 (rays start "inside" the solid) and
    # flips the baked tangent normals. Enforce outward by signed volume.
    mesh.calc_loop_triangles()
    coords = np.empty(len(mesh.vertices) * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", coords)
    coords = coords.reshape(-1, 3)
    tri_idx = np.empty(len(mesh.loop_triangles) * 3, dtype=np.int64)
    mesh.loop_triangles.foreach_get("vertices", tri_idx)
    tri_idx = tri_idx.reshape(-1, 3)
    volume6 = float(np.einsum("ij,ij->i", coords[tri_idx[:, 0]],
                              np.cross(coords[tri_idx[:, 1]],
                                       coords[tri_idx[:, 2]])).sum())
    if volume6 < 0.0:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
        bm.to_mesh(mesh)
        bm.free()
        mesh.update()
        log(f"CLEANUP: recalc unified winding INWARD (signed volume "
            f"{volume6 / 6.0:.2f}) -> all faces flipped outward")

    stats = {"verts_before": verts_before, "verts_after": len(mesh.vertices),
             "weld_distance": weld_dist, "islands_removed": islands_removed,
             "holes_filled": holes_filled}
    report["cleanup"] = stats
    log(f"CLEANUP: verts {verts_before} -> {stats['verts_after']}, "
        f"islands removed {islands_removed}, holes filled {holes_filled}")


# --------------------------------------------------------------------------- stage: conform

def conform(obj, params, report):
    mesh = obj.data
    pre_rot = float(params.get("pre_rotate_z_deg", 0.0))
    if pre_rot:
        mesh.transform(Matrix.Rotation(math.radians(pre_rot), 4, "Z"))

    mn, mx = mesh_bounds(obj)
    dims = mx - mn
    if float(min(dims)) <= 1e-9:
        fail("degenerate mesh bounds after import/cleanup")
    target_m = np.asarray(params["target_dims_ue"], dtype=np.float64) / UE_UNITS_PER_METER

    fit_mode = params["fit_mode"]
    if fit_mode == "height":          # units: uniform scale so Z == target height
        s = float(target_m[2] / dims[2])
        scale = (s, s, s)
    elif fit_mode == "box":           # buildings: exact per-axis fit to the blockout box
        scale = tuple(float(target_m[i] / dims[i]) for i in range(3))
    else:
        fail(f"unknown fit_mode '{fit_mode}' (expected 'height' or 'box')", code=2)
    mesh.transform(Matrix.Diagonal((*scale, 1.0)))

    # Origin: feet-center (units) and ground-center (buildings) are geometrically the
    # same rule — XY bbox center at (0,0), min Z at 0 (TASK-014/037/038 definitions).
    mn, mx = mesh_bounds(obj)
    offset = Vector((-(mn[0] + mx[0]) / 2.0, -(mn[1] + mx[1]) / 2.0, -float(mn[2])))
    mesh.transform(Matrix.Translation(offset))
    obj.matrix_world = Matrix.Identity(4)

    mn, mx = mesh_bounds(obj)
    dims_ue = (mx - mn) * UE_UNITS_PER_METER
    target_ue = np.asarray(params["target_dims_ue"], dtype=np.float64)
    tolerance = float(params.get("dims_tolerance", 0.10))
    deviation = np.abs(dims_ue - target_ue) / target_ue
    within = bool(np.all(deviation <= tolerance))
    report["conform"] = {
        "pre_rotate_z_deg": pre_rot, "fit_mode": fit_mode, "scale": list(scale),
        "origin_rule": params["origin"],
        "dims_ue": [round(float(v), 2) for v in dims_ue],
        "target_dims_ue": list(target_ue),
        "dims_tolerance": tolerance, "dims_within_tolerance": within,
    }
    if not within:
        report["warnings"].append(
            f"conformed dims {dims_ue.round(1).tolist()} deviate >{tolerance:.0%} from "
            f"target {target_ue.tolist()} on some axis (fit_mode={fit_mode})")
    log(f"CONFORM: dims(UE)={[round(float(v), 1) for v in dims_ue]} "
        f"target={target_ue.tolist()} within±{tolerance:.0%}={within}, "
        f"origin={params['origin']}")


# --------------------------------------------------------------------------- stage: remesh/decimate

def remesh_and_decimate(low, params, quick, report):
    voxel_ue = float(params.get("voxel_size_ue", 2.0)) * (2.0 if quick else 1.0)
    low.data.remesh_voxel_size = max(voxel_ue / UE_UNITS_PER_METER, 1e-5)
    ensure_object_mode()
    select_only([low], active=low)              # pitfall: active before the remesh op
    bpy.ops.object.voxel_remesh()
    tris_dense = tri_count(low)

    budget = int(params["tri_budget"])
    if tris_dense > budget:
        modifier = low.modifiers.new("PipelineDecimate", "DECIMATE")
        modifier.ratio = budget / tris_dense
        select_only([low], active=low)          # pitfall: active before modifier_apply
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    tris_final = tri_count(low)
    report["remesh"] = {"voxel_size_ue": voxel_ue, "tris_after_remesh": tris_dense,
                        "tri_budget": budget, "tris_final": tris_final}
    if tris_final > budget:
        report["warnings"].append(f"low mesh {tris_final} tris still exceeds budget {budget}")
    log(f"REMESH: voxel {voxel_ue} UE -> {tris_dense} tris, decimated -> "
        f"{tris_final} tris (budget {budget})")


# --------------------------------------------------------------------------- stage: carve (Castle 3x HOLLOW law, TASK-348)

def _carve_profile_arch(width, z_floor, z_spring, segments):
    """Closed 2D profile (x, z) for a round-arch passage: rectangle from z_floor
    up to z_spring, capped by a semicircle of radius width/2 (apex at
    z_spring + width/2). Counter-clockwise; units already in meters."""
    half = width / 2.0
    points = [(half, z_floor), (half, z_spring)]
    for step in range(1, segments):
        angle = math.pi * step / segments
        points.append((half * math.cos(angle), z_spring + half * math.sin(angle)))
    points.append((-half, z_spring))
    points.append((-half, z_floor))
    return points


def build_carve_cutters(params, report):
    """CONVENTIONS "Castle 3x HOLLOW (2026-07-28)" SHELL law: Meshy generates a
    closed exterior shell; Stage-2 AUTHORS the hollow interior, the gate cut at
    the concept's arch, and the flat walkable interior floor. Cutter geometry is
    manifest-driven (per-asset key "carve", UE units in conformed space) - the
    same single-edit-path law as ucx.boxes; in-editor/hand edits stay banned.
    Cutter types: "box" {center,size} and "arch" {x_center,y_min,y_max,width,
    z_floor,z_spring,[segments]} (round-arch prism extruded along Y).
    New faces the boolean creates inherit the cutter's "InteriorStone" material
    (flat warm-stone principled, manifest "interior_color_srgb"), so the D bake
    gives the interior a plausible shadowed-masonry read instead of void black.
    Returns the cutter object list (deleted by the caller after use)."""
    carve_cfg = params.get("carve")
    if not carve_cfg or not carve_cfg.get("cutters"):
        return []
    color = carve_cfg.get("interior_color_srgb", [0.52, 0.42, 0.30])
    interior_mat = bpy.data.materials.new("InteriorStone")
    interior_mat.use_nodes = True
    principled = _find_principled(interior_mat.node_tree)
    if principled is not None:
        principled.inputs["Base Color"].default_value = (
            float(color[0]), float(color[1]), float(color[2]), 1.0)
        principled.inputs["Roughness"].default_value = 0.9
        principled.inputs["Metallic"].default_value = 0.0

    cutter_objects = []
    for index, spec in enumerate(carve_cfg["cutters"]):
        kind = spec.get("type", "box")
        name = f"CARVE_{spec.get('name', kind)}_{index:02d}"
        mesh = bpy.data.meshes.new(name)
        if kind == "box":
            center = np.asarray(spec["center"], dtype=np.float64) / UE_UNITS_PER_METER
            size = np.asarray(spec["size"], dtype=np.float64) / UE_UNITS_PER_METER
            bm = bmesh.new()
            bmesh.ops.create_cube(bm, size=1.0)
            for vert in bm.verts:
                vert.co.x = vert.co.x * size[0] + center[0]
                vert.co.y = vert.co.y * size[1] + center[1]
                vert.co.z = vert.co.z * size[2] + center[2]
            bm.to_mesh(mesh)
            bm.free()
        elif kind == "arch":
            x_center = float(spec.get("x_center", 0.0)) / UE_UNITS_PER_METER
            y_lo = float(spec["y_min"]) / UE_UNITS_PER_METER
            y_hi = float(spec["y_max"]) / UE_UNITS_PER_METER
            width = float(spec["width"]) / UE_UNITS_PER_METER
            z_floor = float(spec["z_floor"]) / UE_UNITS_PER_METER
            z_spring = float(spec["z_spring"]) / UE_UNITS_PER_METER
            segments = max(4, int(spec.get("segments", 24)))
            profile = _carve_profile_arch(width, z_floor, z_spring, segments)
            count = len(profile)
            verts = ([(x_center + x, y_lo, z) for x, z in profile]
                     + [(x_center + x, y_hi, z) for x, z in profile])
            faces = [list(range(count))[::-1], list(range(count, 2 * count))]
            for i in range(count):
                j = (i + 1) % count
                faces.append([i, j, count + j, count + i])
            mesh.from_pydata(verts, [], faces)
            mesh.update()
        else:
            fail(f"carve cutter '{name}': unknown type '{kind}' (box|arch)", code=2)
        # Consistent outward normals (boolean EXACT correctness).
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        bm.to_mesh(mesh)
        bm.free()
        mesh.materials.append(interior_mat)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        obj.hide_render = True
        cutter_objects.append(obj)
    report.setdefault("carve", {})["cutters"] = [o.name for o in cutter_objects]
    log(f"CARVE: built {len(cutter_objects)} manifest cutter volume(s)")
    return cutter_objects


def apply_carve(target, cutter_objects, report, key):
    """Boolean-DIFFERENCE every cutter out of the target (EXACT solver,
    hole-tolerant). Applied to BOTH the low target (crisp gate/floor planes,
    post-decimate) and the dense bake donor (so selected-to-active bakes see
    co-located interior surfaces carrying the InteriorStone color)."""
    if not cutter_objects:
        return
    tris_before = tri_count(target)
    ensure_object_mode()
    for cutter in cutter_objects:
        modifier = target.modifiers.new("PipelineCarve", "BOOLEAN")
        modifier.operation = "DIFFERENCE"
        modifier.solver = "EXACT"
        if hasattr(modifier, "use_hole_tolerant"):
            modifier.use_hole_tolerant = True
        modifier.object = cutter
        select_only([target], active=target)   # pitfall: active before modifier_apply
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    tris_after = tri_count(target)
    report.setdefault("carve", {})[key] = {"tris_before": tris_before,
                                           "tris_after": tris_after}
    log(f"CARVE[{key}]: tris {tris_before} -> {tris_after}")


def probe_carve(target, params, report, key):
    """Verification probes (measure, don't assume): for each arch cutter, cast a
    ray INTO the gate at passage mid-height - it must clear the wall band (first
    hit beyond y_max or none = open passage). For each box cutter, cast straight
    down from just under the cavity ceiling - the hit is the as-built floor Z."""
    carve_cfg = params.get("carve") or {}
    probes = {}
    for spec in carve_cfg.get("cutters", []):
        name = spec.get("name", spec.get("type", "?"))
        if spec.get("type") == "arch":
            x_c = float(spec.get("x_center", 0.0)) / UE_UNITS_PER_METER
            y_lo = min(float(spec["y_min"]), float(spec["y_max"])) / UE_UNITS_PER_METER
            y_hi = max(float(spec["y_min"]), float(spec["y_max"])) / UE_UNITS_PER_METER
            z_mid = ((float(spec["z_floor"]) + float(spec["z_spring"])) / 2.0
                     / UE_UNITS_PER_METER)
            origin = Vector((x_c, y_lo - 0.5, z_mid))
            hit, location, _n, _i = target.ray_cast(origin, Vector((0.0, 1.0, 0.0)))
            first_hit_y = round(location.y * UE_UNITS_PER_METER, 1) if hit else None
            probes[name] = {
                "ray": "through-gate +Y at passage mid-height",
                "first_hit_y_ue": first_hit_y,
                "open_through_wall": bool((not hit) or
                                          location.y > y_hi - 1e-4),
            }
        elif spec.get("type") == "box":
            center = spec["center"]
            size = spec["size"]
            origin = Vector((center[0] / UE_UNITS_PER_METER,
                             center[1] / UE_UNITS_PER_METER,
                             (center[2] + size[2] / 2.0 - 1.0) / UE_UNITS_PER_METER))
            hit, location, _n, _i = target.ray_cast(origin, Vector((0.0, 0.0, -1.0)))
            probes[name] = {
                "ray": "cavity-ceiling straight down",
                "floor_hit_z_ue": round(location.z * UE_UNITS_PER_METER, 1)
                                  if hit else None,
            }
    report.setdefault("carve", {}).setdefault("probes", {})[key] = probes
    log(f"CARVE probes[{key}]: {json.dumps(probes)}")


# --------------------------------------------------------------------------- stage: UV

def smart_uv(low, params, report):
    uv_params = params.get("uv", {})
    ensure_object_mode()
    select_only([low], active=low)              # pitfall: active before mode_set
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(
        angle_limit=math.radians(float(uv_params.get("angle_limit_deg", 66.0))),
        island_margin=float(uv_params.get("island_margin", 0.02)))
    bpy.ops.object.mode_set(mode="OBJECT")
    rename_uvmap(low, report)


def rename_uvmap(obj, report):
    """UV layer must be named exactly 'UVMap' — TASK-038 MikkTSpace lesson."""
    layers = obj.data.uv_layers
    if len(layers) == 0:
        report["warnings"].append("mesh has NO UV layer — UE import will warn (MikkTSpace)")
        return False
    layers[0].name = "UVMap"
    layers.active_index = 0
    while len(layers) > 1:                      # single clean channel for the importer
        layers.remove(layers[1])
    return True


# --------------------------------------------------------------------------- stage: bake

def _new_bake_image(name, resolution, colorspace):
    image = bpy.data.images.new(name, width=resolution, height=resolution, alpha=False)
    image.colorspace_settings.name = colorspace
    return image


def save_png(image, path, guard):
    out = guard.check(path)
    image.filepath_raw = str(out)
    image.file_format = "PNG"
    image.save()
    log(f"  wrote {out}")
    return out


def ensure_dense_materials(dense):
    """Bakes need node materials on the donor; blockout-FBX smoke runs may have none."""
    mesh = dense.data
    if len(mesh.materials) == 0:
        mesh.materials.append(None)
    for index, mat in enumerate(mesh.materials):
        if mat is None:
            mat = bpy.data.materials.new(f"{dense.name}_AutoMat_{index}")
            mesh.materials[index] = mat
        if not mat.use_nodes:
            mat.use_nodes = True


def _find_principled(node_tree):
    for node in node_tree.nodes:
        if node.type == "BSDF_PRINCIPLED":
            return node
    return None


def rewire_metallic_to_emission(dense):
    """Cycles has no METALLIC bake pass. Reroute each dense material's Metallic input
    into an Emission shader wired straight to the output, then bake EMIT.
    DESTRUCTIVE to the dense materials — must be the LAST bake pass."""
    for mat in dense.data.materials:
        node_tree = mat.node_tree
        output = next((n for n in node_tree.nodes
                       if n.type == "OUTPUT_MATERIAL" and n.is_active_output), None)
        if output is None:
            output = node_tree.nodes.new("ShaderNodeOutputMaterial")
        emission = node_tree.nodes.new("ShaderNodeEmission")
        principled = _find_principled(node_tree)
        if principled is not None:
            metallic_socket = principled.inputs["Metallic"]
            if metallic_socket.is_linked:
                node_tree.links.new(metallic_socket.links[0].from_socket,
                                    emission.inputs["Color"])
            else:
                value = float(metallic_socket.default_value)
                emission.inputs["Color"].default_value = (value, value, value, 1.0)
        else:
            emission.inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)
        for link in list(output.inputs["Surface"].links):
            node_tree.links.remove(link)
        node_tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])


def bake_all(dense, low, params, quick, report):
    bake_params = params.get("bake", {})
    resolution = 256 if quick else int(params["bake_resolution"])
    samples = int(bake_params.get("samples", 8))
    ao_samples = (16 if quick else int(bake_params.get("ao_samples", 64)))
    margin = int(bake_params.get("margin_px", 16))
    cage_m = float(bake_params.get("cage_extrusion_ue", 3.0)) / UE_UNITS_PER_METER
    ray_m = float(bake_params.get("max_ray_distance_ue", 8.0)) / UE_UNITS_PER_METER

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"                 # ruling: Cycles CPU bakes
    scene.cycles.use_denoising = False

    ensure_dense_materials(dense)

    # Temporary bake material on the low target: the target image node must EXIST
    # and be nodes.active in the ACTIVE object's material (headless pitfall).
    bake_mat = bpy.data.materials.new("PipelineBakeTarget")
    bake_mat.use_nodes = True
    target_node = bake_mat.node_tree.nodes.new("ShaderNodeTexImage")
    low.data.materials.clear()
    low.data.materials.append(bake_mat)

    for polygon in low.data.polygons:           # smooth shading = clean bake tangent basis
        polygon.use_smooth = True

    card_id = params["_card_id"]
    images = {
        "D":  _new_bake_image(f"T_{card_id}_D", resolution, "sRGB"),
        "N":  _new_bake_image(f"T_{card_id}_N", resolution, "Non-Color"),
        "AO": _new_bake_image(f"T_{card_id}_AO", resolution, "Non-Color"),
        "R":  _new_bake_image(f"T_{card_id}_R", resolution, "Non-Color"),
        "M":  _new_bake_image(f"T_{card_id}_M", resolution, "Non-Color"),
    }

    def run_bake(bake_type, image, sample_count, **extra):
        scene.cycles.samples = sample_count
        target_node.image = image
        bake_mat.node_tree.nodes.active = target_node   # pitfall: active image node
        select_only([dense, low], active=low)           # pitfall: donor selected, target active
        bpy.ops.object.bake(type=bake_type, use_selected_to_active=True,
                            cage_extrusion=cage_m, max_ray_distance=ray_m,
                            margin=margin, **extra)
        log(f"  baked {bake_type} -> {image.name} ({resolution}^2, {sample_count} spp)")

    # Order matters: EMIT/metallic rewire destroys the dense materials, so it goes last.
    run_bake("DIFFUSE", images["D"], samples, pass_filter={"COLOR"})
    run_bake("ROUGHNESS", images["R"], samples)
    run_bake("NORMAL", images["N"], samples, normal_space="TANGENT")
    run_bake("AO", images["AO"], ao_samples)
    rewire_metallic_to_emission(dense)
    run_bake("EMIT", images["M"], samples)

    report["bake"] = {"resolution": resolution, "samples": samples,
                      "ao_samples": ao_samples, "margin_px": margin,
                      "cage_extrusion_ue": cage_m * UE_UNITS_PER_METER,
                      "max_ray_distance_ue": ray_m * UE_UNITS_PER_METER}
    return images


def _image_channel(image, channel_index):
    width, height = image.size
    buffer = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(buffer)
    return buffer.reshape(height, width, 4)[:, :, channel_index]


def _resize_nearest(grid, width, height):
    src_h, src_w = grid.shape
    if (src_w, src_h) == (width, height):
        return grid
    row_idx = (np.arange(height) * src_h) // height
    col_idx = (np.arange(width) * src_w) // width
    return grid[row_idx][:, col_idx]


def pack_orm(card_id, ao_grid, rough_grid, metal_grid, resolution):
    """numpy-pack R=AO, G=Roughness, B=Metallic into a LINEAR image (ORM law)."""
    ao = _resize_nearest(ao_grid, resolution, resolution)
    rough = _resize_nearest(rough_grid, resolution, resolution)
    metal = _resize_nearest(metal_grid, resolution, resolution)
    rgba = np.ones((resolution, resolution, 4), dtype=np.float32)
    rgba[:, :, 0] = np.clip(ao, 0.0, 1.0)
    rgba[:, :, 1] = np.clip(rough, 0.0, 1.0)
    rgba[:, :, 2] = np.clip(metal, 0.0, 1.0)
    image = _new_bake_image(f"T_{card_id}_ORM", resolution, "Non-Color")
    image.pixels.foreach_set(rgba.ravel())
    return image


# --------------------------------------------------------------------------- stage: albedo de-light (TASK-193)

ALBEDO_DELIGHT_DEFAULTS = {
    # Conservative-ON in-script (TASK-193 ruling); per-asset manifest key
    # "albedo_delight" (bool or partial object) overrides — READ-only here.
    "enabled": True,
    "ao_divide_strength": 0.6,   # 0 = no divide, 1 = full D/max(AO, floor) blend weight
    "ao_floor": 0.35,            # divisor floor: max(AO, floor) caps crevice gain at ~1/0.35
    "gamma": 0.85,               # linear-space levels lift (out = in^gamma); <1 brightens
    "gain": 1.0,                 # linear multiplier applied after the gamma lift
    "shoulder": 0.80,            # soft-clamp knee (linear); identity below this value
    "max_out": 0.98,             # soft-clamp asymptote — output approaches, never reaches it
}


def _srgb_to_linear(rgb):
    """Piecewise IEC 61966-2-1 decode (byte D images expose pixels sRGB-ENCODED)."""
    return np.where(rgb <= 0.04045, rgb / 12.92,
                    np.power((rgb + 0.055) / 1.055, 2.4))


def _linear_to_srgb(rgb):
    return np.where(rgb <= 0.0031308, rgb * 12.92,
                    1.055 * np.power(np.maximum(rgb, 0.0), 1.0 / 2.4) - 0.055)


def resolve_delight_config(params, report):
    """In-script defaults <- optional manifest key 'albedo_delight' (bool = on/off
    switch; object = partial override). Unknown keys warn; values are sanitized to
    safe ranges (the divisor floor also guards divide-by-zero). PER-KEY bad values
    (null / string / list / bool-for-numeric / NaN) NEVER raise — each falls back
    to that key's ALBEDO_DELIGHT_DEFAULTS value with a report warning (QA loop-1
    blocker: an uncaught TypeError/ValueError here would land AFTER the full
    Cycles bake and destroy a complete multi-minute Stage-2 run)."""
    cfg = dict(ALBEDO_DELIGHT_DEFAULTS)
    raw = params.get("albedo_delight")
    if isinstance(raw, bool):
        cfg["enabled"] = raw
    elif isinstance(raw, dict):
        for key, value in raw.items():
            if key.startswith("_"):          # manifest comment keys (_comment, _tuned, ...)
                continue
            if key in cfg:
                cfg[key] = value
            else:
                report["warnings"].append(
                    f"albedo_delight: unknown manifest key '{key}' ignored "
                    f"(known: {sorted(ALBEDO_DELIGHT_DEFAULTS)})")
    elif raw is not None:
        report["warnings"].append(
            f"albedo_delight: manifest value must be bool or object, got "
            f"{type(raw).__name__} — in-script defaults used")

    # 'enabled' is STRICT bool: a JSON string like "false" is truthy and would
    # silently keep the step ON (QA loop-1 warn) — warn + default instead.
    if not isinstance(cfg["enabled"], bool):
        report["warnings"].append(
            f"albedo_delight: 'enabled' must be true/false, got "
            f"{cfg['enabled']!r} — default {ALBEDO_DELIGHT_DEFAULTS['enabled']} used")
        cfg["enabled"] = ALBEDO_DELIGHT_DEFAULTS["enabled"]

    def sanitized_number(key, lo, hi):
        """float-coerce cfg[key] with warn+default on ANY bad value — never raises."""
        default = float(ALBEDO_DELIGHT_DEFAULTS[key])
        value = cfg[key]
        if isinstance(value, bool):          # float(True)=1.0 would coerce silently
            report["warnings"].append(
                f"albedo_delight: '{key}' must be a number, got {value!r} — "
                f"default {default} used")
            value = default
        else:
            try:
                value = float(value)
            except (TypeError, ValueError):
                report["warnings"].append(
                    f"albedo_delight: '{key}' must be a number, got {value!r} — "
                    f"default {default} used")
                value = default
        if not math.isfinite(value):         # json.load admits NaN/Infinity literals
            report["warnings"].append(
                f"albedo_delight: '{key}' is not finite ({value!r}) — "
                f"default {default} used")
            value = default
        return float(min(max(value, lo), hi))

    cfg["ao_divide_strength"] = sanitized_number("ao_divide_strength", 0.0, 1.0)
    cfg["ao_floor"] = sanitized_number("ao_floor", 0.05, 1.0)
    cfg["gamma"] = sanitized_number("gamma", 0.2, 2.0)
    cfg["gain"] = sanitized_number("gain", 0.25, 4.0)
    cfg["max_out"] = sanitized_number("max_out", 0.5, 1.0)
    cfg["shoulder"] = sanitized_number("shoulder", 0.1, cfg["max_out"] - 0.01)
    return cfg


def apply_albedo_delight(img_d, img_orm, params, report):
    """TASK-193: de-light/brighten the baked albedo BEFORE its PNG write. TRELLIS
    bakes shading into the albedo (the recorded dark look — TASK-150/151/172); this
    un-bakes it in LINEAR space: (1) AO-divide against the already-packed ORM.R
    occlusion, strength-blended and divisor-floored, (2) a mild gamma/gain levels
    lift, (3) a C1-continuous rational soft-shoulder clamp so highlights compress
    toward max_out instead of hard-clipping. Returns the image to write/wire as
    T_<CardID>_D — a NEW generated image; the input D and the N/ORM images are
    never mutated (N/ORM byte-untouched law). Blender pixel-space note: byte images
    expose .pixels in their STORED encoding (sRGB for D), float buffers are already
    scene-linear — is_float selects the decode/encode path."""
    cfg = resolve_delight_config(params, report)
    report["albedo_delight"] = dict(cfg)
    if not cfg["enabled"]:
        report["albedo_delight"]["applied"] = False
        log("DELIGHT: disabled (manifest albedo_delight) — D passes through untouched")
        return img_d
    width, height = img_d.size
    if width == 0 or height == 0:
        report["albedo_delight"]["applied"] = False
        report["warnings"].append("albedo_delight: D image has no pixels — step skipped")
        return img_d

    buffer = np.empty(width * height * 4, dtype=np.float32)
    img_d.pixels.foreach_get(buffer)
    rgba = buffer.reshape(height, width, 4)
    is_float = bool(img_d.is_float)
    rgb = np.clip(rgba[:, :, :3].astype(np.float64), 0.0, 1.0)
    linear = rgb if is_float else _srgb_to_linear(rgb)
    mean_before = float(linear.mean())

    # (1) AO-divide: albedo ~= baked_D / occlusion. ORM.R is the baked AO (bake mode)
    # or TRELLIS's own occlusion (native; flat 1.0 fallback = divide is a no-op).
    strength = cfg["ao_divide_strength"]
    if strength > 0.0:
        ao = _resize_nearest(_image_channel(img_orm, 0), width, height)
        ao = np.maximum(ao.astype(np.float64), cfg["ao_floor"])[:, :, None]
        linear = linear + (linear / ao - linear) * strength

    # (2) Levels lift: gamma <1 brightens midtones/shadows; gain is a plain multiplier.
    linear = cfg["gain"] * np.power(np.maximum(linear, 0.0), cfg["gamma"])

    # (3) Highlight-safe soft clamp: identity below `shoulder`; above it a rational
    # shoulder with slope 1 at the knee, asymptotic to `max_out` — no hard clip.
    knee, ceiling = cfg["shoulder"], cfg["max_out"]
    over = np.maximum(linear - knee, 0.0)
    linear = np.where(linear <= knee, linear,
                      knee + (ceiling - knee) * over / ((ceiling - knee) + over))
    linear = np.clip(linear, 0.0, 1.0)

    mean_after = float(linear.mean())
    p99_after = float(np.percentile(linear, 99.0))
    shoulder_fraction = float(np.mean(linear > knee))

    out_rgba = rgba.copy()
    out_rgba[:, :, :3] = (linear if is_float else _linear_to_srgb(linear)).astype(np.float32)
    card_id = params["_card_id"]
    # Mirror the SOURCE's channel layout (QA loop-1 nit): a native-mode D carrying
    # alpha keeps it in the written PNG; bake-mode D (RGB, depth 24) stays RGB.
    # Image.depth is bits/pixel — RGBA is 32 (byte) / 64 (16-bit) / 128 (float).
    has_alpha = int(getattr(img_d, "depth", 0)) in (32, 64, 128)
    out_img = bpy.data.images.new(f"T_{card_id}_D_delight", width=width, height=height,
                                  alpha=has_alpha, float_buffer=is_float)
    out_img.colorspace_settings.name = img_d.colorspace_settings.name
    out_img.pixels.foreach_set(out_rgba.ravel())

    report["albedo_delight"].update({
        "applied": True,
        "source_is_float_buffer": is_float,
        "mean_linear_before": round(mean_before, 4),
        "mean_linear_after": round(mean_after, 4),
        "p99_linear_after": round(p99_after, 4),
        "shoulder_compressed_fraction": round(shoulder_fraction, 4),
    })
    log(f"DELIGHT: AO-divide strength {strength} (floor {cfg['ao_floor']}) + gamma "
        f"{cfg['gamma']} gain {cfg['gain']}; mean(linear) {mean_before:.3f} -> "
        f"{mean_after:.3f}, p99 {p99_after:.3f}, soft-clamped <= {ceiling} "
        f"({shoulder_fraction:.1%} of pixels in the shoulder)")
    return out_img


# --------------------------------------------------------------------------- stage: native textures

def _find_upstream_image(socket):
    stack, seen = [socket], set()
    while stack:
        current = stack.pop()
        for link in current.links:
            node = link.from_node
            if id(node) in seen:
                continue
            seen.add(id(node))
            if node.type == "TEX_IMAGE" and node.image is not None:
                return node.image
            stack.extend(node.inputs)
    return None


def extract_native_textures(obj, params, report):
    """Native mode: keep TRELLIS's own textures. Locate baseColor / normal /
    metallicRoughness / occlusion images on the imported glTF materials and return
    (img_d, img_n, orm_image). WebP sources become PNG at save time; the ORM is
    numpy-recomposed so the channel law (R=AO, G=Rough, B=Metal) always holds."""
    card_id = params["_card_id"]
    base_img = normal_img = mr_img = occlusion_img = None
    for mat in obj.data.materials:
        if mat is None or not mat.use_nodes:
            continue
        principled = _find_principled(mat.node_tree)
        if principled is None:
            continue
        base_img = base_img or _find_upstream_image(principled.inputs["Base Color"])
        normal_img = normal_img or _find_upstream_image(principled.inputs["Normal"])
        mr_img = (mr_img or _find_upstream_image(principled.inputs["Metallic"])
                  or _find_upstream_image(principled.inputs["Roughness"]))
        if occlusion_img is None:
            for node in mat.node_tree.nodes:      # glTF importer parks AO on a group node
                if node.type == "GROUP":
                    for group_input in node.inputs:
                        if "occlusion" in group_input.name.lower():
                            occlusion_img = _find_upstream_image(group_input)
        if base_img and normal_img and mr_img and occlusion_img:
            break

    resolution = int(params["bake_resolution"])
    if mr_img is not None:
        resolution = max(mr_img.size[0], mr_img.size[1])

    if base_img is not None:
        base_img.name = f"T_{card_id}_D"
    else:
        report["warnings"].append("native mode: no baseColor texture found — flat D written")
        base_img = _new_bake_image(f"T_{card_id}_D", resolution, "sRGB")
        flat = np.full((resolution, resolution, 4), 0.5, dtype=np.float32)
        flat[:, :, 3] = 1.0
        base_img.pixels.foreach_set(flat.ravel())

    if normal_img is not None:
        normal_img.name = f"T_{card_id}_N"
        normal_img.colorspace_settings.name = "Non-Color"
    else:
        report["warnings"].append("native mode: no normal texture found — T_*_N skipped")

    # glTF metallicRoughness packs G=roughness, B=metallic (R unused); occlusion is
    # its own reference (often the same image's R). Recompose explicitly.
    if mr_img is not None:
        rough_grid = _image_channel(mr_img, 1)
        metal_grid = _image_channel(mr_img, 2)
    else:
        report["warnings"].append("native mode: no metallicRoughness texture — "
                                  "constant rough=0.8 metal=0.0 ORM written")
        rough_grid = np.full((resolution, resolution), 0.8, dtype=np.float32)
        metal_grid = np.zeros((resolution, resolution), dtype=np.float32)
    if occlusion_img is not None:
        ao_grid = _image_channel(occlusion_img, 0)
    else:
        ao_grid = np.ones((resolution, resolution), dtype=np.float32)

    orm_image = pack_orm(card_id, ao_grid, rough_grid, metal_grid, resolution)
    log(f"NATIVE TEXTURES: D={'ok' if base_img else 'flat'} "
        f"N={'ok' if normal_img else 'MISSING'} ORM recomposed at {resolution}^2")
    return base_img, normal_img, orm_image


# --------------------------------------------------------------------------- stage: slots / team region

def _selector_matches(selector, normalized_center, normal):
    box = selector.get("box")
    if box is not None:
        lo, hi = box["min"], box["max"]
        for axis in range(3):
            if not (lo[axis] <= normalized_center[axis] <= hi[axis]):
                return False
    normal_rule = selector.get("normal")
    if normal_rule is not None:
        direction = Vector(normal_rule["direction"]).normalized()
        if normal.dot(direction) < float(normal_rule.get("min_dot", 0.5)):
            return False
    return "box" in selector or "normal" in selector


def build_final_materials(low, params, img_d, img_n, img_orm, report):
    """Two-slot contract: slot 0 'TeamRegion' (minority face-set from manifest
    selectors), slot 1 '<CardID>PBR' wired to the D/N/ORM textures.
    Slot ORDER is the contract — the BeginPlay team recolor hardcodes slot 0."""
    card_id = params["_card_id"]

    team_mat = bpy.data.materials.new("TeamRegion")
    team_mat.use_nodes = True
    team_principled = _find_principled(team_mat.node_tree)
    if team_principled is not None:              # MI_TeamColor_Blue placeholder tint
        team_principled.inputs["Base Color"].default_value = (0.05, 0.30, 1.00, 1.0)

    pbr_mat = bpy.data.materials.new(f"{card_id}PBR")
    pbr_mat.use_nodes = True
    node_tree = pbr_mat.node_tree
    principled = _find_principled(node_tree)
    if principled is not None:
        node_d = node_tree.nodes.new("ShaderNodeTexImage")
        node_d.image = img_d
        node_tree.links.new(node_d.outputs["Color"], principled.inputs["Base Color"])
        if img_n is not None:
            node_n = node_tree.nodes.new("ShaderNodeTexImage")
            node_n.image = img_n
            normal_map = node_tree.nodes.new("ShaderNodeNormalMap")
            normal_map.uv_map = "UVMap"
            node_tree.links.new(node_n.outputs["Color"], normal_map.inputs["Color"])
            node_tree.links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])
        node_orm = node_tree.nodes.new("ShaderNodeTexImage")
        node_orm.image = img_orm
        separate = node_tree.nodes.new("ShaderNodeSeparateColor")
        node_tree.links.new(node_orm.outputs["Color"], separate.inputs["Color"])
        node_tree.links.new(separate.outputs["Green"], principled.inputs["Roughness"])
        node_tree.links.new(separate.outputs["Blue"], principled.inputs["Metallic"])

    mesh = low.data
    mesh.materials.clear()
    mesh.materials.append(team_mat)              # slot 0 — TeamRegion (contract)
    mesh.materials.append(pbr_mat)               # slot 1 — <CardID>PBR

    team_cfg = params.get("team_region", {}) or {}
    selectors = team_cfg.get("selectors", [])
    mn, mx = mesh_bounds(low)
    span = np.maximum(mx - mn, 1e-9)
    team_area = total_area = 0.0
    team_faces = 0
    for polygon in mesh.polygons:
        center = np.asarray(polygon.center, dtype=np.float64)
        normalized = (center - mn) / span        # x:0=-X..1=+X  y:0=front(-Y)  z:0=ground
        matched = any(_selector_matches(s, normalized, polygon.normal) for s in selectors)
        polygon.material_index = 0 if matched else 1
        total_area += polygon.area
        if matched:
            team_area += polygon.area
            team_faces += 1

    fraction = (team_area / total_area) if total_area > 0 else 0.0
    max_fraction = float(team_cfg.get("max_fraction", 0.40))
    report["team_region"] = {"faces": team_faces, "area_fraction": round(fraction, 4),
                             "max_fraction": max_fraction, "selectors": len(selectors)}
    if team_faces == 0:
        report["warnings"].append("TeamRegion selectors matched ZERO faces — team recolor "
                                  "will be invisible; tune manifest selectors (eyeball gate)")
    elif fraction > max_fraction:
        report["warnings"].append(f"TeamRegion covers {fraction:.0%} of surface area "
                                  f"(> max {max_fraction:.0%}) — should be a minority "
                                  f"face-set; tune manifest selectors")
    log(f"SLOTS: [TeamRegion, {card_id}PBR]; team faces={team_faces} "
        f"({fraction:.1%} of area, cap {max_fraction:.0%})")


# --------------------------------------------------------------------------- stage: UCX

def generate_ucx(params, report):
    """Buildings: authored UCX_SM_<CardID>_00.. box hulls from the manifest footprint
    spec. Castle law: wall-footprint-exact, NOT a full plinth slab (M1 ~410-unit
    placement dead-zone lesson). Centers/sizes are UE units in conformed space."""
    ucx_cfg = params.get("ucx")
    if not ucx_cfg or not ucx_cfg.get("boxes"):
        return []
    card_id = params["_card_id"]
    ucx_objects = []
    for index, box in enumerate(ucx_cfg["boxes"]):
        name = f"UCX_SM_{card_id}_{index:02d}"
        center = np.asarray(box["center"], dtype=np.float64) / UE_UNITS_PER_METER
        size = np.asarray(box["size"], dtype=np.float64) / UE_UNITS_PER_METER
        mesh = bpy.data.meshes.new(name)
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for vert in bm.verts:
            vert.co.x = vert.co.x * size[0] + center[0]
            vert.co.y = vert.co.y * size[1] + center[1]
            vert.co.z = vert.co.z * size[2] + center[2]
        bm.to_mesh(mesh)
        bm.free()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        obj.hide_render = True                   # keep hulls out of the previews
        ucx_objects.append(obj)
    report["ucx"] = {"count": len(ucx_objects), "names": [o.name for o in ucx_objects],
                     "spec_note": ucx_cfg.get("_comment", "")}
    log(f"UCX: generated {len(ucx_objects)} authored hull boxes "
        f"({ucx_objects[0].name}..{ucx_objects[-1].name})")
    return ucx_objects


# --------------------------------------------------------------------------- stage: export

def _flip_winding(mesh):
    """Reverse every face loop (normals flip side). Winding-only — never moves a
    vertex; per-loop data (UVs) stays with its vertices; material indices and
    smooth flags survive the bmesh round-trip."""
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.reverse_faces(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()


def _ue_handedness_precomp(objs):
    """UE handedness pre-compensation (TASK-348 MIRROR-FIX, 2026-07-28).

    UE's FBX importer converts the right-handed FBX scene to UE's left-handed
    space by negating Y on every position/direction (FFbxDataConverter::
    ConvertPos/ConvertDir = (X,-Y,Z)) and re-flipping triangle winding. That is
    an orientation-REVERSING map (det -1): no rotation-only axis_forward/axis_up
    choice can cancel it, so an FBX exported straight from conformed space lands
    in-engine Y-MIRRORED against the manifest space that authors UCX hulls,
    carve cutters, gate blockers and nav volumes (the TASK-350 castle blocker:
    visual gate +Y, collision door gap -Y).

    Compensation: bake mirror_Y + winding flip into the mesh data here, at
    export time only. UE's own negation+flip then restores conformed space
    exactly, so in-engine geometry == bake-time geometry (tangent-space normal
    maps stay valid) and manifest numbers are UE-local coordinates verbatim.

    The transform is an involution — calling this a second time restores the
    scene state (report bounds/previews run after export and must see conformed
    space)."""
    mirror = Matrix.Diagonal((1.0, -1.0, 1.0, 1.0))
    for obj in objs:
        obj.data.transform(mirror)
        _flip_winding(obj.data)
        obj.data.update()


def export_fbx(low, ucx_objects, fbx_path, guard, report=None):
    """Axis contract (blockout-identical, TASK-014/037/038): axis_forward='-Z',
    axis_up='Y', apply_unit_scale=True, mesh_smooth_type='FACE' — PLUS the UE
    handedness pre-compensation (see _ue_handedness_precomp): the written FBX is
    deliberately Y-mirrored so UE's import negation lands it in conformed space.
    NOTE for offline probes: a Blender round-trip of the exported file shows
    geometry at -Y of its conformed position — that is the pre-compensation, not
    a defect; emulate UE by applying diag(1,-1,1) after import."""
    out = guard.check(fbx_path)
    ensure_object_mode()
    export_set = [low] + ucx_objects
    _ue_handedness_precomp(export_set)          # mirror IN (export-time only)
    try:
        select_only(export_set, active=low)
        bpy.ops.export_scene.fbx(
            filepath=str(out),
            use_selection=True,
            object_types={"MESH"},
            apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_NONE",
            axis_forward="-Z",
            axis_up="Y",
            mesh_smooth_type="FACE",
            use_mesh_modifiers=True,
            use_triangles=True,      # bake fidelity: UE sees the exact baked triangulation
            bake_anim=False,
            add_leaf_bones=False,
            path_mode="STRIP",       # UE imports textures separately from the PNGs
        )
    finally:
        _ue_handedness_precomp(export_set)      # mirror OUT (restore conformed space)
    if report is not None:
        report["export"] = {
            "ue_handedness_precompensation": True,
            "note": "FBX baked with mirror_Y + winding flip so UE's RH->LH import "
                    "negation restores conformed/manifest space (TASK-348 MIRROR-FIX)",
        }
    log(f"EXPORT: {out} (UE handedness pre-compensation: mirror_Y + winding flip)")
    return out


# --------------------------------------------------------------------------- stage: previews

def _make_camera(name):
    cam_data = bpy.data.cameras.new(name)
    cam_data.type = "ORTHO"
    cam_obj = bpy.data.objects.new(name, cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    return cam_obj


def _aim_camera(cam_obj, center, direction, distance, ortho_scale):
    direction = Vector(direction).normalized()
    cam_obj.location = Vector(center) + direction * distance
    view_dir = -direction
    cam_obj.rotation_euler = view_dir.to_track_quat("-Z", "Y").to_euler()
    cam_obj.data.ortho_scale = ortho_scale
    cam_obj.data.clip_end = distance * 4.0


def render_previews(low, params, quick, guard, cache_dir, report):
    scene = bpy.context.scene
    preview_cfg = params.get("preview", {})
    workbench_px = 384 if quick else int(preview_cfg.get("workbench_px", 768))
    cycles_px = 384 if quick else int(preview_cfg.get("cycles_px", 640))
    cycles_samples = 8 if quick else int(preview_cfg.get("cycles_samples", 32))

    mn, mx = mesh_bounds(low)
    center = Vector(((mn[0] + mx[0]) / 2.0, (mn[1] + mx[1]) / 2.0, (mn[2] + mx[2]) / 2.0))
    max_dim = float(max(mx - mn))
    ortho_scale = max_dim * 1.2
    distance = max_dim * 3.0

    camera = _make_camera("PreviewCam")
    scene.camera = camera
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"   # truthful texture read

    # Workbench orthos, TEXTURE color (front = -Y per the facing contract).
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.render.resolution_x = scene.render.resolution_y = workbench_px

    views = {
        "front": (0.0, -1.0, 0.0),
        "back": (0.0, 1.0, 0.0),
        "threequarter": (-0.66, -0.66, 0.35),
        "top": (0.0, 0.0, 1.0),
    }
    previews = []
    for view_name, direction in views.items():
        _aim_camera(camera, center, direction, distance, ortho_scale)
        out = guard.check(cache_dir / "previews" / f"preview_{view_name}.png")
        scene.render.filepath = str(out)
        bpy.ops.render.render(write_still=True)
        previews.append(str(out))
        log(f"  preview {view_name} -> {out.name}")

    # One small Cycles beauty from the three-quarter view.
    sun_data = bpy.data.lights.new("PreviewSun", "SUN")
    sun_data.energy = 3.0
    sun = bpy.data.objects.new("PreviewSun", sun_data)
    bpy.context.scene.collection.objects.link(sun)
    sun.rotation_euler = Vector((-0.4, -0.3, -1.0)).to_track_quat("-Z", "Y").to_euler()
    if scene.world is None:
        scene.world = bpy.data.worlds.new("PreviewWorld")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    if background is not None:
        background.inputs["Strength"].default_value = 0.6

    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = cycles_samples
    scene.cycles.use_denoising = True
    scene.render.resolution_x = scene.render.resolution_y = cycles_px
    _aim_camera(camera, center, views["threequarter"], distance, ortho_scale)
    out = guard.check(cache_dir / "previews" / "preview_beauty_cycles.png")
    scene.render.filepath = str(out)
    bpy.ops.render.render(write_still=True)
    previews.append(str(out))
    log(f"  preview beauty (Cycles, {cycles_samples} spp) -> {out.name}")

    report["previews"] = previews


# --------------------------------------------------------------------------- main

def main():
    args = parse_args()
    card_id = args.card_id
    manifest_path = Path(args.manifest).resolve() if args.manifest \
        else SCRIPT_DIR / "pipeline_manifest.json"
    manifest = load_manifest(manifest_path)
    params = merged_params(manifest, card_id)
    params["_card_id"] = card_id
    mode = args.mode or params.get("mode", "bake")
    if mode not in ("bake", "native"):
        fail(f"manifest mode '{mode}' invalid (bake|native)", code=2)

    cache_dir = CACHE_ROOT / card_id
    input_path = Path(args.input).resolve() if args.input \
        else cache_dir / "trellis_raw.glb"
    if not input_path.is_file():
        fail(f"input mesh not found: {input_path} "
             f"(run Stage 1 trellis_generate.py first, or pass --input)", code=2)

    guard = OutputGuard(card_id, args.smoke)
    if args.smoke:
        fbx_path = cache_dir / "smoke" / f"{card_id}.fbx"
        tex_dir = cache_dir / "smoke" / "Textures" / card_id
    else:
        fbx_path = CONTENT_RAW / f"{card_id}.fbx"
        tex_dir = CONTENT_RAW / "Textures" / card_id

    log(f"card-id={card_id} mode={mode} smoke={args.smoke} quick={args.quick}")
    log(f"input={input_path}")
    log(f"fbx-out={fbx_path}")
    log(f"blender={bpy.app.version_string}")

    report = {
        "card_id": card_id, "mode": mode, "smoke": args.smoke, "quick": args.quick,
        "input": str(input_path), "manifest": str(manifest_path),
        "blender_version": bpy.app.version_string,
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "warnings": [],
    }
    started = time.time()

    reset_scene()
    obj = import_input(input_path)

    if mode == "bake":
        cleanup(obj, params, report)
        conform(obj, params, report)
        # Duplicate: obj becomes the dense bake donor; the copy is the low target.
        low = obj.copy()
        low.data = obj.data.copy()
        bpy.context.scene.collection.objects.link(low)
        dense = obj
        dense.name = f"{card_id}_DenseSource"
        low.name = f"SM_{card_id}"               # FBX render-node name contract
        low.data.name = f"SM_{card_id}"

        remesh_and_decimate(low, params, args.quick, report)
        # Castle 3x HOLLOW SHELL law (TASK-348): author the hollow interior, the
        # gate cut and the flat walkable floor AFTER decimate (crisp planes) and
        # BEFORE UV/bake (interior faces need real UVs + baked texels). The dense
        # donor is carved with the SAME cutters so the bake sees co-located,
        # InteriorStone-colored interior surfaces.
        carve_cutters = build_carve_cutters(params, report)
        if carve_cutters:
            apply_carve(low, carve_cutters, report, "low")
            probe_carve(low, params, report, "low")
            apply_carve(dense, carve_cutters, report, "dense")
            for cutter in carve_cutters:
                bpy.data.objects.remove(cutter, do_unlink=True)
        smart_uv(low, params, report)
        images = bake_all(dense, low, params, args.quick, report)

        # Debug intermediates -> Cache only (never Content).
        debug_dir = cache_dir / "bake_debug"
        save_png(images["D"], debug_dir / f"T_{card_id}_D_predelight.png", guard)
        save_png(images["AO"], debug_dir / f"T_{card_id}_AO.png", guard)
        save_png(images["R"], debug_dir / f"T_{card_id}_R.png", guard)
        save_png(images["M"], debug_dir / f"T_{card_id}_M.png", guard)

        img_orm = pack_orm(card_id,
                           _image_channel(images["AO"], 0),
                           _image_channel(images["R"], 0),
                           _image_channel(images["M"], 0),
                           images["AO"].size[0])
        img_d, img_n = images["D"], images["N"]
        bpy.data.objects.remove(dense, do_unlink=True)   # donor no longer needed
    else:  # native
        if params.get("carve"):
            report["warnings"].append("carve spec present but mode is 'native' — the "
                                      "hollow/gate/floor authoring runs in BAKE mode "
                                      "only (native keeps source mesh/UVs untouched)")
        conform(obj, params, report)             # keep TRELLIS mesh/UVs — no destructive ops
        low = obj
        low.name = f"SM_{card_id}"
        low.data.name = f"SM_{card_id}"
        if not rename_uvmap(low, report):
            log("native mode: no UVs present — falling back to smart_project")
            smart_uv(low, params, report)
        budget = int(params["tri_budget"])
        tris = tri_count(low)
        report["remesh"] = {"skipped": "native mode", "tris_final": tris,
                            "tri_budget": budget}
        if tris > budget:
            report["warnings"].append(f"native mesh {tris} tris exceeds budget {budget} "
                                      f"(native mode never decimates — consider bake mode)")
        img_d, img_n, img_orm = extract_native_textures(low, params, report)

    # Albedo de-light/brighten (TASK-193) — BEFORE the D PNG write; both modes.
    # Returns a new image (or the untouched original when disabled); the delighted
    # D is what gets saved AND wired into the preview materials below.
    img_d = apply_albedo_delight(img_d, img_orm, params, report)

    # Texture PNGs (WebP sources become PNG here in native mode).
    textures = {}
    textures["D"] = str(save_png(img_d, tex_dir / f"T_{card_id}_D.png", guard))
    if img_n is not None:
        textures["N"] = str(save_png(img_n, tex_dir / f"T_{card_id}_N.png", guard))
    textures["ORM"] = str(save_png(img_orm, tex_dir / f"T_{card_id}_ORM.png", guard))
    report["textures"] = textures

    build_final_materials(low, params, img_d, img_n, img_orm, report)
    ucx_objects = generate_ucx(params, report)
    if params.get("category") == "building" and not ucx_objects:
        report["warnings"].append("building has no manifest UCX spec — importer would "
                                  "auto-generate collision (check the manifest)")

    export_fbx(low, ucx_objects, fbx_path, guard, report)

    mn, mx = mesh_bounds(low)
    report["result"] = {
        "fbx": str(fbx_path),
        "object_name": low.name,
        "tris": tri_count(low),
        "bounds_ue": [round(float(v) * UE_UNITS_PER_METER, 2) for v in (mx - mn)],
        "min_z_ue": round(float(mn[2]) * UE_UNITS_PER_METER, 3),
        "uv_layer": low.data.uv_layers[0].name if low.data.uv_layers else None,
        "uv_layer_ok": bool(low.data.uv_layers) and low.data.uv_layers[0].name == "UVMap",
        "material_slots": [m.name if m else None for m in low.data.materials],
    }

    render_previews(low, params, args.quick, guard, cache_dir, report)

    if args.save_blend:
        blend_out = guard.check(cache_dir / "refine_debug.blend")
        bpy.ops.wm.save_as_mainfile(filepath=str(blend_out))
        log(f"saved debug blend -> {blend_out}")

    report["elapsed_seconds"] = round(time.time() - started, 1)
    report_path = guard.check(cache_dir / "refine_report.json")
    with open(report_path, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
    log(f"REPORT: {report_path}")
    if report["warnings"]:
        for warning in report["warnings"]:
            log(f"WARNING: {warning}")
    log(f"DONE in {report['elapsed_seconds']}s "
        f"({report['result']['tris']} tris, slots {report['result']['material_slots']})")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        fail("unhandled exception (see traceback above)")
    sys.exit(0)
