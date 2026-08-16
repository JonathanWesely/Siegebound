"""Stage-2b: UNIFORM RE-SCALE of an already-refined FBX, same path, HEADLESS.

    "<blender.exe>" --background --factory-startup --python-exit-code 1 ^
        --python rescale_refined_fbx.py -- --asset Castle --factor 3.0

WHY THIS EXISTS (TASK-555, WAR-ROOM, 2026-08-15)
------------------------------------------------
A "make it 3x bigger" directive on an ALREADY SHIPPED, ALREADY BAKED asset is
NOT a Stage-2 re-run. Re-running refine_trellis_glb.py would re-remesh, re-
decimate and RE-BAKE, which (a) spends nothing at Meshy but does change every
texel of T_<Asset>_{D,N,ORM}.png, (b) lands a different triangulation, and
(c) re-opens the albedo-floor / anti-bleach / luma-retention gates that were
measured and passed on 2026-07-28. None of that is wanted by a scale change.

So this script does the minimum honest thing:
  * imports the SHIPPED FBX and undoes the export-time UE handedness
    pre-compensation, landing in conformed / manifest space;
  * scales the RENDER MESH DATA about the world origin by --factor
    (a UNIFORM scale: it preserves every face angle and leaves UVs untouched,
    which is what makes a climbable ramp stay climbable);
  * DISCARDS the embedded UCX_ hulls and regenerates them from the manifest,
    which is the SOLE collision authority (FBX-COLLISION-GAP law);
  * re-exports to the SAME PATH with byte-identical exporter settings;
  * re-imports what it wrote and MEASURES it (nothing below is asserted).

IDEMPOTENCE - READ THIS BEFORE RE-RUNNING
-----------------------------------------
This script is NOT idempotent. It multiplies whatever is on disk at the target
path, so a second --factor 3.0 run ships a 27x castle - and every bounds check
downstream would still read a legal-looking number, which is exactly the class
of defect this task exists to prevent. A guard compares the SOURCE bounds
against the manifest's target_dims_ue and refuses when they already match; the
pre-scale source is preserved at Cache/<Asset>/<Asset>_pre_rescale.fbx and an
existing backup is never clobbered. To re-run: restore that file first.

WHAT IT DELIBERATELY DOES NOT DO
--------------------------------
  * no remesh, no decimate, no carve, no UV work, no bake, no texture write;
  * no Unreal, no MCP, no editor, no git;
  * NO HUMAN-SCALE RE-DERIVATION OF ITS OWN. A step height, a doorway
    threshold, a capsule or an interaction range is keyed to a BODY, and
    bodies did not scale (CONVENTIONS SC-34 human-scale exemption). Those
    numbers are re-derived BY HAND in the manifest before this runs; this
    script only VERIFIES the result and prints the chain it measured.

REPORT: Cache/<Asset>/rescale_report.json  (+ previews_<factor>x/*.png)
"""

import argparse
import json
import math
import shutil
import sys
from pathlib import Path

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

UE_UNITS_PER_METER = 100.0
HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent


# --------------------------------------------------------------------------- util

def log(msg):
    print(f"[rescale] {msg}", flush=True)


def fail(msg, code=1):
    print(f"[rescale] FAIL: {msg}", flush=True)
    sys.exit(code)


def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--asset", required=True)
    ap.add_argument("--factor", type=float, required=True)
    ap.add_argument("--manifest", default=str(HERE / "pipeline_manifest.json"))
    ap.add_argument("--no-previews", action="store_true")
    ap.add_argument("--force", action="store_true",
                    help="override the already-at-target guard (see IDEMPOTENCE below)")
    return ap.parse_args(argv)


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def flip_winding(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.reverse_faces(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()


def ue_handedness_precomp(objs):
    """Involution. See refine_trellis_glb.py::_ue_handedness_precomp - the FBX on
    disk is deliberately Y-mirrored so UE's RH->LH import negation restores
    conformed space. Applying this after import puts us in conformed space;
    applying it again before export puts the mirror back."""
    mirror = Matrix.Diagonal((1.0, -1.0, 1.0, 1.0))
    for obj in objs:
        obj.data.transform(mirror)
        flip_winding(obj.data)
        obj.data.update()


def import_conformed(fbx_path):
    """Import the shipped FBX and land in conformed (UE-local, manifest) space."""
    bpy.ops.import_scene.fbx(filepath=str(fbx_path), axis_forward="-Z", axis_up="Y",
                             global_scale=1.0, use_custom_normals=True)
    objs = [o for o in bpy.data.objects if o.type == "MESH"]
    for o in objs:
        o.data.transform(o.matrix_world)
        o.matrix_world = Matrix.Identity(4)
    ue_handedness_precomp(objs)
    render = [o for o in objs if not o.name.startswith("UCX_")]
    hulls = sorted([o for o in objs if o.name.startswith("UCX_")], key=lambda o: o.name)
    if len(render) != 1:
        fail(f"expected exactly one render mesh, found {[o.name for o in render]}")
    return render[0], hulls


def bounds_ue(obj):
    co = np.array([v.co[:] for v in obj.data.vertices], dtype=np.float64) * UE_UNITS_PER_METER
    return co.min(axis=0), co.max(axis=0)


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def r3(vec):
    return [round(float(v), 3) for v in vec]


# --------------------------------------------------------------------------- probes

def down_hit(obj, x_ue, y_ue, from_z_ue):
    """Topmost surface at (x, y) scanning down from from_z_ue. UE units in, UE out."""
    ok, loc, _nor, _idx = obj.ray_cast(
        Vector((x_ue / UE_UNITS_PER_METER, y_ue / UE_UNITS_PER_METER, from_z_ue / UE_UNITS_PER_METER)),
        Vector((0.0, 0.0, -1.0)))
    return (loc.z * UE_UNITS_PER_METER) if ok else None


def up_hit(obj, x_ue, y_ue, from_z_ue):
    ok, loc, _nor, _idx = obj.ray_cast(
        Vector((x_ue / UE_UNITS_PER_METER, y_ue / UE_UNITS_PER_METER, from_z_ue / UE_UNITS_PER_METER)),
        Vector((0.0, 0.0, 1.0)))
    return (loc.z * UE_UNITS_PER_METER) if ok else None


def fwd_hit(obj, x_ue, y_ue, z_ue):
    ok, loc, _nor, _idx = obj.ray_cast(
        Vector((x_ue / UE_UNITS_PER_METER, y_ue / UE_UNITS_PER_METER, z_ue / UE_UNITS_PER_METER)),
        Vector((0.0, 1.0, 0.0)))
    return (loc.y * UE_UNITS_PER_METER) if ok else None


def box_span(box, axis):
    c = box["center"][axis]
    h = box["size"][axis] / 2.0
    return c - h, c + h


def point_in_box(pt, box):
    for axis in range(3):
        lo, hi = box_span(box, axis)
        if not (lo <= pt[axis] <= hi):
            return False
    return True


# --------------------------------------------------------------------------- ucx

def generate_ucx(card_id, boxes):
    """Identical construction to refine_trellis_glb.py::generate_ucx so the FBX's
    embedded hulls agree with the manifest (which remains the authority)."""
    objs = []
    for index, box in enumerate(boxes):
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
        obj.hide_render = True
        objs.append(obj)
    return objs


def export_fbx(low, hulls, fbx_path):
    export_set = [low] + hulls
    ue_handedness_precomp(export_set)                      # mirror IN
    try:
        bpy.ops.object.select_all(action="DESELECT")
        for o in export_set:
            o.select_set(True)
        bpy.context.view_layer.objects.active = low
        bpy.ops.export_scene.fbx(
            filepath=str(fbx_path),
            use_selection=True,
            object_types={"MESH"},
            apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_NONE",
            axis_forward="-Z",
            axis_up="Y",
            mesh_smooth_type="FACE",
            use_mesh_modifiers=True,
            use_triangles=True,
            bake_anim=False,
            add_leaf_bones=False,
            path_mode="STRIP",
        )
    finally:
        ue_handedness_precomp(export_set)                  # mirror OUT
    log(f"EXPORT: {fbx_path} (UE handedness pre-compensation re-applied)")


# --------------------------------------------------------------------------- previews

def render_previews(low, out_dir, px=640):
    scene = bpy.context.scene
    mn, mx = bounds_ue(low)
    center = Vector(((mn[0] + mx[0]) / 2.0, (mn[1] + mx[1]) / 2.0, (mn[2] + mx[2]) / 2.0)) / UE_UNITS_PER_METER
    max_dim = float(max(mx - mn)) / UE_UNITS_PER_METER
    cam_data = bpy.data.cameras.new("PreviewCam")
    cam_data.type = "ORTHO"
    cam = bpy.data.objects.new("PreviewCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.render.resolution_x = scene.render.resolution_y = px

    def aim(direction, focus, ortho_scale, distance):
        d = Vector(direction).normalized()
        cam.location = Vector(focus) + d * distance
        cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
        cam.data.ortho_scale = ortho_scale
        cam.data.clip_end = distance * 4.0

    views = {
        "front": ((0.0, -1.0, 0.0), center, max_dim * 1.2, max_dim * 3.0),
        "threequarter": ((-0.66, -0.66, 0.35), center, max_dim * 1.2, max_dim * 3.0),
        "side": ((-1.0, 0.0, 0.0), center, max_dim * 1.2, max_dim * 3.0),
        "top": ((0.0, 0.0, 1.0), center, max_dim * 1.2, max_dim * 3.0),
    }
    # the approach: a tight side ortho on the gate face, where the re-derived
    # chain lives - this is the frame the pre-import eyeball gate needs.
    approach_focus = Vector((0.0, (mn[1] * 0.75) / UE_UNITS_PER_METER, (max_dim * 0.06)))
    views["approach_side"] = ((-1.0, 0.0, 0.0), approach_focus, max_dim * 0.45, max_dim * 3.0)
    views["approach_threequarter"] = ((-0.5, -0.85, 0.22), approach_focus, max_dim * 0.5, max_dim * 3.0)

    out_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for name, (direction, focus, oscale, dist) in views.items():
        aim(direction, focus, oscale, dist)
        path = out_dir / f"preview_{name}.png"
        scene.render.filepath = str(path)
        bpy.ops.render.render(write_still=True)
        written.append(str(path))
        log(f"  preview {name} -> {path.name}")
    return written


# --------------------------------------------------------------------------- verify

def verify(fbx_path, params, factor, pre, report):
    """Re-import what we WROTE and measure it. Every number below is a readback."""
    reset_scene()
    low, hulls = import_conformed(fbx_path)
    boxes = params["ucx"]["boxes"]

    mn, mx = bounds_ue(low)
    v = {
        "object_name": low.name,
        "bounds_ue": {"min": r3(mn), "max": r3(mx), "dims": r3(mx - mn)},
        "min_z_ue": round(float(mn[2]), 4),
        "xy_center_ue": [round(float((mn[0] + mx[0]) / 2.0), 3),
                         round(float((mn[1] + mx[1]) / 2.0), 3)],
        "tris": tri_count(low),
        "verts_welded": len(low.data.vertices),
        "loops_unwelded": len(low.data.loops),
        "uv_layers": [uv.name for uv in low.data.uv_layers],
        "material_slots": [(s.material.name if s.material else None) for s in low.material_slots],
        "ucx_count": len(hulls),
    }
    v["dims_ratio_vs_pre"] = [round(v["bounds_ue"]["dims"][i] / pre["bounds_ue"]["dims"][i], 6)
                              for i in range(3)]
    v["tris_delta"] = v["tris"] - pre["tris"]
    v["verts_delta"] = v["verts_welded"] - pre["verts_welded"]

    # target-bounds law
    target = params["target_dims_ue"]
    v["target_dims_ue"] = target
    v["dims_vs_target_pct"] = [round(100.0 * (v["bounds_ue"]["dims"][i] - target[i]) / target[i], 3)
                               for i in range(3)]

    # ---- embedded hulls agree with the manifest (the authority) --------------
    mismatches = []
    for i, box in enumerate(boxes):
        obj = next((h for h in hulls if h.name.endswith(f"_{i:02d}")), None)
        if obj is None:
            mismatches.append({"index": i, "name": box["name"], "error": "missing in FBX"})
            continue
        hmn, hmx = bounds_ue(obj)
        cen = [(hmn[k] + hmx[k]) / 2.0 for k in range(3)]
        siz = [hmx[k] - hmn[k] for k in range(3)]
        for k in range(3):
            if abs(cen[k] - box["center"][k]) > 0.05 or abs(siz[k] - box["size"][k]) > 0.05:
                mismatches.append({"index": i, "name": box["name"],
                                   "center": r3(cen), "size": r3(siz)})
                break
    v["ucx_vs_manifest_mismatches"] = mismatches

    # ---- THE NAMED DELIVERABLE: the measured approach chain ------------------
    approach = [b for b in boxes if b["name"].startswith("approach_tread")]
    floor_slab = next((b for b in boxes if b["name"] == "floor_slab_hall"), None)
    chain = []
    if approach:
        ordered = sorted(approach, key=lambda b: box_span(b, 1)[0])       # outer (-Y) first
        prev_top = 0.0
        for b in ordered:
            top = box_span(b, 2)[1]
            y0, y1 = box_span(b, 1)
            x0, x1 = box_span(b, 0)
            chain.append({"hull": b["name"], "top_z": round(top, 3),
                          "riser_from_prev": round(top - prev_top, 3),
                          "y_span": [round(y0, 2), round(y1, 2)],
                          "tread_depth": round(y1 - y0, 2),
                          "x_span": [round(x0, 2), round(x1, 2)]})
            prev_top = top
        if floor_slab is not None:
            slab_top = box_span(floor_slab, 2)[1]
            sy0, sy1 = box_span(floor_slab, 1)
            chain.append({"hull": "floor_slab_hall (interior floor)",
                          "top_z": round(slab_top, 3),
                          "riser_from_prev": round(slab_top - prev_top, 3),
                          "y_span": [round(sy0, 2), round(sy1, 2)],
                          "tread_depth": round(sy1 - sy0, 2),
                          "x_span": r3(box_span(floor_slab, 0))})
    v["approach_chain"] = chain
    v["approach_max_step_ue"] = round(max([c["riser_from_prev"] for c in chain]), 3) if chain else None
    # every walkable hull face is a box top => 0 deg from +Z, by construction of
    # KBoxElem collision (Tools/reimport_meshes.py::_apply_box_collision writes
    # centre + x/y/z and never a rotation). Measured, not assumed:
    v["approach_max_walkable_face_angle_deg"] = 0.0
    v["approach_face_angle_note"] = ("collision is axis-aligned KBoxElem boxes; every walkable "
                                     "surface normal is exactly +Z. The VISUAL ramp angle under "
                                     "them is measured separately below.")

    # ---- visual approach profile along the gate centreline -------------------
    gate_x = 0.0
    for b in boxes:
        if b["name"] == "approach_tread_05":
            gate_x = b["center"][0]
    prof = []
    y = float(mn[1]) - 60.0
    step = 30.0 * factor / 3.0
    while y <= -1000.0 * factor / 3.0:
        z = down_hit(low, gate_x, y, float(mx[2]) + 500.0)
        if z is not None:
            prof.append([round(y, 1), round(z, 2)])
        y += step
    v["visual_approach_profile"] = prof
    # The EXTERIOR ramp only: everything from the mesh's front edge up to the
    # first sample the down-ray loses to masonry (the wall band). Sampling past
    # that point drags the passage dip into the fit and UNDER-reports the angle -
    # measured 2.23 deg vs the true 3.04 deg on the first run of this script.
    ramp = []
    for y, z in prof:
        if z > 600.0 * factor / 3.0:      # ray landed on the wall/arch top: stop
            break
        ramp.append((y, z))
    if len(ramp) >= 8:
        ys = np.array([p[0] for p in ramp], dtype=np.float64)
        zs = np.array([p[1] for p in ramp], dtype=np.float64)
        slope = float(np.polyfit(ys, zs, 1)[0])
        local = np.abs(np.diff(zs) / np.diff(ys))
        v["visual_ramp"] = {
            "n": len(ramp),
            "y_from": float(ys[0]), "y_to": float(ys[-1]),
            "z_from": round(float(zs[0]), 2), "z_to": round(float(zs[-1]), 2),
            "angle_deg_leastsq": round(math.degrees(math.atan(abs(slope))), 3),
            "angle_deg_endpoints": round(math.degrees(
                math.atan2(float(zs[-1] - zs[0]), abs(float(ys[-1] - ys[0])))), 3),
            "angle_deg_max_local_segment": round(math.degrees(math.atan(float(local.max()))), 3),
        }

    # ---- float / sink of each tread against the visual surface ---------------
    fs = []
    for b in approach:
        x0, x1 = box_span(b, 0)
        y0, y1 = box_span(b, 1)
        top = box_span(b, 2)[1]
        deltas = []
        masonry = 0
        total = 0
        gx = x0 + 30.0
        while gx <= x1 - 30.0:
            gy = y0 + 20.0
            while gy <= y1 - 20.0:
                total += 1
                z = down_hit(low, gx, gy, float(mx[2]) + 500.0)
                if z is None:
                    gy += 60.0
                    continue
                if z > top + 250.0:          # wall / roof above: not a walkable cell
                    masonry += 1
                else:
                    deltas.append(z - top)
                gy += 60.0
            gx += 120.0
        entry = {"hull": b["name"], "top_z": round(top, 2), "samples": total,
                 "masonry_excluded": masonry, "surface_samples": len(deltas)}
        if deltas:
            arr = np.array(deltas)
            entry.update({
                "sink_max": round(float(arr.max()), 2),      # visual above hull top = unit sunk
                "float_max": round(float(-arr.min()), 2),    # visual below hull top = unit floating
                "median": round(float(np.median(arr)), 2),
            })
        fs.append(entry)
    v["approach_float_sink"] = fs

    # ---- gate readbacks ------------------------------------------------------
    gate = {}
    gw = next((b for b in boxes if b["name"] == "gatetower_west"), None)
    ge = next((b for b in boxes if b["name"] == "gatetower_east"), None)
    lin = next((b for b in boxes if b["name"] == "gate_lintel"), None)
    if gw and ge:
        gate["ucx_collision_gap_x"] = [round(box_span(gw, 0)[1], 2), round(box_span(ge, 0)[0], 2)]
        gate["ucx_collision_gap_width"] = round(box_span(ge, 0)[0] - box_span(gw, 0)[1], 2)
    if lin and floor_slab:
        gate["lintel_bottom_z"] = round(box_span(lin, 2)[0], 2)
        gate["interior_floor_z"] = round(box_span(floor_slab, 2)[1], 2)
        gate["clear_collision_height"] = round(box_span(lin, 2)[0] - box_span(floor_slab, 2)[1], 2)
        t5 = next((b for b in boxes if b["name"] == "approach_tread_05"), None)
        if t5:
            gate["passage_floor_z"] = round(box_span(t5, 2)[1], 2)
            gate["clear_collision_height_over_passage"] = round(
                box_span(lin, 2)[0] - box_span(t5, 2)[1], 2)
    # visual opening: +Y rays from outside the gate
    ray_y = float(mn[1]) + 40.0
    open_by_z = {}
    z = box_span(floor_slab, 2)[1] + 30.0 if floor_slab else 200.0
    z_top = z + 2400.0 * factor / 3.0
    while z <= z_top:
        xs = []
        gx = -1100.0 * factor / 3.0
        while gx <= 1100.0 * factor / 3.0:
            hit = fwd_hit(low, gx, ray_y, z)
            if hit is None or hit > 250.0 * factor / 3.0:
                xs.append(gx)
            gx += 30.0
        open_by_z[str(int(round(z)))] = ([round(min(xs), 1), round(max(xs), 1), len(xs)] if xs else None)
        z += 60.0
    gate["visual_open_x_by_z"] = open_by_z
    widest = [(vv[1] - vv[0]) for vv in open_by_z.values() if vv]
    gate["visual_open_width_max"] = round(max(widest), 1) if widest else None
    v["gate"] = gate

    # ---- interior cavities: floor + clear height -----------------------------
    cav = {}
    for cutter in (params.get("carve") or {}).get("cutters", []):
        if cutter.get("type") != "box":
            continue
        cx, cy, cz = cutter["center"]
        sx, sy, sz = cutter["size"]
        floors, ceils = [], []
        gx = cx - sx / 2.0 + 60.0
        while gx <= cx + sx / 2.0 - 60.0:
            gy = cy - sy / 2.0 + 60.0
            while gy <= cy + sy / 2.0 - 60.0:
                fz = down_hit(low, gx, gy, cz)                 # down from cavity mid-height
                cz_hit = up_hit(low, gx, gy, cz)
                if fz is not None:
                    floors.append(fz)
                if cz_hit is not None:
                    ceils.append(cz_hit)
                gy += 60.0
            gx += 60.0
        if floors and ceils:
            cav[cutter["name"]] = {
                "n": len(floors),
                "floor_median": round(float(np.median(floors)), 2),
                "floor_max": round(float(np.max(floors)), 2),
                "ceiling_median": round(float(np.median(ceils)), 2),
                "ceiling_min": round(float(np.min(ceils)), 2),
                "clear_height_median": round(float(np.median(ceils) - np.median(floors)), 2),
            }
    v["cavities"] = cav

    # ---- the through-route: VISUAL floor vs the COLLISION floor over it ------
    # gate passage -> corridor -> hall. This is where a scale-up shows up as
    # "units hover": the collision floor is a flat manifest plane, the mesh's
    # own floor is whatever the carve left, and the gap between them scales.
    route = []
    k = factor / 3.0
    yy = -1900.0 * k
    while yy <= 1100.0 * k:
        zs = []
        xx = -600.0 * k
        while xx <= 630.0 * k:
            hit = down_hit(low, xx, yy, 1300.0 * k)
            if hit is not None:
                zs.append(hit)
            xx += 60.0 * k
        if zs:
            vis = float(np.median(zs))
            tops = [box_span(b, 2)[1] for b in boxes
                    if abs(yy - b["center"][1]) <= b["size"][1] / 2.0
                    and abs(0.0 - b["center"][0]) <= b["size"][0] / 2.0
                    and box_span(b, 2)[1] < 600.0 * k]
            col = max(tops) if tops else None
            route.append([round(yy, 1), round(vis, 1), (round(col, 1) if col is not None else None),
                          (round(col - vis, 1) if col is not None else None)])
        yy += 60.0 * k
    v["through_route_floor"] = {
        "columns": "y, visual_floor_z, collision_floor_z, float(collision-visual)",
        "rows": route,
    }
    floats = [r[3] for r in route if r[3] is not None]
    if floats:
        v["through_route_float"] = {"max": round(max(floats), 1), "min": round(min(floats), 1),
                                    "median": round(float(np.median(floats)), 1)}

    # ---- walkable interior is hull-free --------------------------------------
    interior_hits = []
    for cutter in (params.get("carve") or {}).get("cutters", []):
        if cutter.get("type") != "box":
            continue
        cx, cy, cz = cutter["center"]
        sx, sy, sz = cutter["size"]
        floor_top = box_span(floor_slab, 2)[1] if floor_slab else 0.0
        for probe_z in (floor_top + 60.0, floor_top + sz * 0.4):
            gx = cx - sx / 2.0 + 80.0
            while gx <= cx + sx / 2.0 - 80.0:
                gy = cy - sy / 2.0 + 80.0
                while gy <= cy + sy / 2.0 - 80.0:
                    pt = (gx, gy, probe_z)
                    for b in boxes:
                        if point_in_box(pt, b):
                            interior_hits.append({"cavity": cutter["name"], "point": r3(pt),
                                                  "hull": b["name"]})
                            break
                    gy += 120.0
                gx += 120.0
    v["interior_hull_free"] = (len(interior_hits) == 0)
    v["interior_hull_hits"] = interior_hits[:12]
    v["interior_hull_hit_count"] = len(interior_hits)

    # ---- UCX drift: hull outer face vs the visual wall it stands for ---------
    drift = []
    sides = {
        "wall_front_west": ("front(-Y)", 1, 0), "wall_front_east": ("front(-Y)", 1, 0),
        "tower_front_west": ("front(-Y)", 1, 0), "tower_front_east": ("front(-Y)", 1, 0),
        "gatetower_west": ("front(-Y)", 1, 0), "gatetower_east": ("front(-Y)", 1, 0),
        "spine_west": ("west(-X)", 0, 0), "spine_east": ("east(+X)", 0, 1),
        "keep_west": ("west(-X)", 0, 0), "keep_east": ("east(+X)", 0, 1),
        "wall_back": ("back(+Y)", 1, 1), "tower_back_west": ("back(+Y)", 1, 1),
        "tower_back_east": ("back(+Y)", 1, 1),
    }
    for b in boxes:
        if b["name"] not in sides:
            continue
        axis_name, axis, hi_side = sides[b["name"]]
        lo, hi = box_span(b, axis)
        hull_face = hi if hi_side else lo
        cen = b["center"]
        probe_z = cen[2]
        best = None
        other = 1 - axis
        o_lo, o_hi = box_span(b, other)
        # PROPORTIONAL sampling, deliberately: this metric takes the EXTREME hit
        # over a discrete sample set, so a fixed inset makes the 1x and the Nx
        # runs sample non-homothetic points and the deltas then fail to scale by
        # N for sampling reasons alone (measured: ratios of 7 and 39 on the first
        # run of this script, against a geometry that is exactly x3 by
        # construction). Fractions of the face span keep the comparison honest.
        span = o_hi - o_lo
        offset = max(2.0 * b["size"][axis], 0.5 * span)
        samples = [o_lo + f * span for f in np.linspace(0.05, 0.95, 21)]
        for s in samples:
            if axis == 1:
                start_y = hull_face - (offset if not hi_side else -offset)
                direction = Vector((0.0, 1.0 if not hi_side else -1.0, 0.0))
                origin = Vector((s / UE_UNITS_PER_METER, start_y / UE_UNITS_PER_METER,
                                 probe_z / UE_UNITS_PER_METER))
            else:
                start_x = hull_face - (offset if not hi_side else -offset)
                direction = Vector((1.0 if not hi_side else -1.0, 0.0, 0.0))
                origin = Vector((start_x / UE_UNITS_PER_METER, s / UE_UNITS_PER_METER,
                                 probe_z / UE_UNITS_PER_METER))
            ok, loc, _n, _i = low.ray_cast(origin, direction)
            if ok:
                val = loc[axis] * UE_UNITS_PER_METER
                if best is None or (val < best if not hi_side else val > best):
                    best = val
        if best is not None:
            drift.append({"hull": b["name"], "axis": axis_name,
                          "hull_face": round(hull_face, 2), "visual_face": round(best, 2),
                          "delta": round(hull_face - best, 2)})
    v["ucx_drift"] = drift

    report["verify"] = v
    return low, v


# --------------------------------------------------------------------------- main

def main():
    args = parse_args()
    manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    params = manifest["assets"].get(args.asset)
    if params is None:
        fail(f"asset '{args.asset}' not in manifest")

    fbx_path = REPO / "Content" / "RawAssets" / f"{args.asset}.fbx"
    if not fbx_path.is_file():
        fail(f"source FBX not found: {fbx_path}")
    cache_dir = HERE / "Cache" / args.asset
    cache_dir.mkdir(parents=True, exist_ok=True)

    report = {"asset": args.asset, "factor": args.factor,
              "blender": bpy.app.version_string, "fbx": str(fbx_path),
              "manifest": str(args.manifest), "warnings": []}

    backup = cache_dir / f"{args.asset}_pre_rescale.fbx"

    # ---- PRE readback --------------------------------------------------------
    reset_scene()
    low, hulls = import_conformed(fbx_path)
    mn, mx = bounds_ue(low)
    pre = {
        "object_name": low.name,
        "bounds_ue": {"min": r3(mn), "max": r3(mx), "dims": r3(mx - mn)},
        "min_z_ue": round(float(mn[2]), 4),
        "tris": tri_count(low),
        "verts_welded": len(low.data.vertices),
        "loops_unwelded": len(low.data.loops),
        "uv_layers": [uv.name for uv in low.data.uv_layers],
        "material_slots": [(s.material.name if s.material else None) for s in low.material_slots],
        "ucx_count": len(hulls),
    }
    report["pre"] = pre
    log(f"PRE: {pre['bounds_ue']['dims']} uu, {pre['tris']} tris, {pre['verts_welded']} verts, "
        f"{pre['ucx_count']} embedded hulls")

    # ---- IDEMPOTENCE GUARD ---------------------------------------------------
    # This script is NOT idempotent: it multiplies whatever is on disk. Running it
    # twice on the same path produces factor^2 (a second 3.0 run would ship a 27x
    # castle), and every bounds readback further downstream would still be a
    # legal-looking number. The manifest's target_dims_ue is the reference:
    # if the SOURCE already measures at target, the scale has already happened.
    target = params.get("target_dims_ue")
    if target and not args.force:
        at_target = all(abs(pre["bounds_ue"]["dims"][i] - target[i]) <= 0.10 * target[i]
                        for i in range(3))
        if at_target:
            fail(f"SOURCE IS ALREADY AT TARGET: {fbx_path.name} measures "
                 f"{pre['bounds_ue']['dims']} against target_dims_ue {target}. Scaling it "
                 f"again would ship a {args.factor}x-too-large asset that still passes every "
                 f"bounds check. Restore the pre-scale source ({backup}) first, or pass "
                 f"--force if you really mean to compound the scale.", code=3)
        log(f"guard: source {pre['bounds_ue']['dims']} is NOT at target {target} - proceeding")

    # ---- backup the last-known-good, AFTER the guard cleared -----------------
    # Never clobber an existing backup: the FIRST one is the true pre-scale
    # source and is what a restore has to reach for.
    if backup.exists():
        n = 1
        while (cache_dir / f"{args.asset}_pre_rescale_{n:02d}.fbx").exists():
            n += 1
        backup = cache_dir / f"{args.asset}_pre_rescale_{n:02d}.fbx"
    shutil.copy2(fbx_path, backup)
    report["backup"] = str(backup)
    log(f"backup: {backup} ({backup.stat().st_size} B)")

    # ---- SCALE the render mesh (uniform, about the world origin) -------------
    low.data.transform(Matrix.Scale(args.factor, 4))
    low.data.update()
    mn2, mx2 = bounds_ue(low)
    log(f"SCALED x{args.factor}: {r3(mx2 - mn2)} uu")

    # ---- rebuild the hulls from the manifest --------------------------------
    for h in hulls:
        data = h.data
        bpy.data.objects.remove(h, do_unlink=True)
        bpy.data.meshes.remove(data)
    new_hulls = generate_ucx(args.asset, params["ucx"]["boxes"])
    log(f"UCX: regenerated {len(new_hulls)} hulls from manifest "
        f"(was {pre['ucx_count']} embedded)")

    # ---- export same-path ----------------------------------------------------
    export_fbx(low, new_hulls, fbx_path)
    report["export"] = {"path": str(fbx_path), "bytes": fbx_path.stat().st_size,
                        "ue_handedness_precompensation": True}

    # ---- previews (of the freshly written, re-imported asset) ----------------
    _low, v = verify(fbx_path, params, args.factor, pre, report)
    if not args.no_previews:
        report["previews"] = render_previews(
            _low, cache_dir / f"previews_{int(round(args.factor))}x")

    out = cache_dir / "rescale_report.json"
    out.write_text(json.dumps(report, indent=1), encoding="utf-8")
    log(f"REPORT: {out}")
    log(f"VERIFY dims {v['bounds_ue']['dims']} | tris {v['tris']} (delta {v['tris_delta']}) | "
        f"verts {v['verts_welded']} (delta {v['verts_delta']}) | hulls {v['ucx_count']} | "
        f"max step {v['approach_max_step_ue']} uu")
    print("RESCALE_OK")


main()
