"""TASK-556 [WR-2] — SM_Torch + SM_WarTable, authored PROCEDURALLY, HEADLESS.

    "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background \
        --factory-startup --python-exit-code 1 \
        --python Tools/ArtPipeline/build_warroom_props.py

NO Meshy / TRELLIS call, NO credits, NO concept render, NO texture bake — this is
the PROP-CLASS EXCEPTION of CONVENTIONS WR-§4 / WR-§5: a sub-hero environment prop
under ~600 tris ships STOCK-NODE materials with no baked T_ set, so the albedo-floor
/ anti-bleach / retention gates do not apply. Materials here exist only to drive the
FBX material SLOT NAMES and the preview renders; the real materials (M_Torch /
M_TorchFlame / M_WarTable) are authored in UE by TASK-566 from the recipes in the
handoff.

⛔ This script does NOT read or write pipeline_manifest.json (TASK-555 owns it in the
   same wave). These two props are manifest-free by construction.

SCALE LAW (CONVENTIONS SC-§34 + WR-§1): the castle shell went 9x; the HUMAN DID NOT.
Every dimension below is keyed to the 180-uu roster human (Footman target_dims_ue
Z=180; hero capsule 2 x SiegeSpawn::DefaultCapsuleHalfHeight 88 = 176), NEVER to the
hall (2910 x 720, 1560 clear).  The one deliberate exception is the war table's
FOOTPRINT, which is room-keyed on purpose (a big table is a big table, not a scaled
human) while its HEIGHT stays body-keyed.  Both are stated in the handoff.

AXIS / SPACE CONTRACT (blockout-identical, TASK-014/037/038 + TASK-348 MIRROR-FIX):
axis_forward='-Z', axis_up='Y', apply_unit_scale=True, FACE smoothing, triangulated,
plus the UE handedness pre-compensation (mirror_Y + winding flip baked at export so
UE's right->left-handed import negation lands the mesh in authored space verbatim).
Both props are authored Y-SYMMETRIC, which the report proves by measurement, so the
mirror is a geometric no-op and the handedness trap cannot bite either asset.

Blender units are METRES; UE units are cm.  Every literal in this file is in uu and
passed through m() exactly once.
"""

import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

UE_UNITS_PER_METER = 100.0
HUMAN_UU = 180.0          # roster human height (Footman fit target / hero capsule)

REPO_ROOT = Path(__file__).resolve().parents[2]
RAW_DIR = REPO_ROOT / "Content" / "RawAssets"
CACHE_DIR = REPO_ROOT / "Tools" / "ArtPipeline" / "Cache" / "WarRoomProps"
PREVIEW_DIR = CACHE_DIR / "previews"


def m(uu):
    """uu -> Blender metres."""
    return float(uu) / UE_UNITS_PER_METER


def log(msg):
    print(f"[warroom-props] {msg}", flush=True)


# --------------------------------------------------------------------------- scene utils

def select_only(objs, active=None):
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
    ensure_object_mode()
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)


def mesh_bounds_uu(obj):
    """LOCAL-space bounds in uu (all transforms are baked into the data here)."""
    mesh = obj.data
    count = len(mesh.vertices)
    co = np.empty(count * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(count, 3) * UE_UNITS_PER_METER
    return co.min(axis=0), co.max(axis=0)


def tri_count(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


# --------------------------------------------------------------------------- geometry helpers

def _new_bm():
    return bmesh.new()


def add_box(bm, x0, x1, y0, y1, z0, z1, bevel=0.0):
    """Axis-aligned box from uu corner values. Returns the created faces."""
    before = set(bm.faces)
    matrix = (Matrix.Translation(Vector((m((x0 + x1) / 2.0), m((y0 + y1) / 2.0), m((z0 + z1) / 2.0))))
              @ Matrix.Diagonal((m(x1 - x0), m(y1 - y0), m(z1 - z0), 1.0)))
    bmesh.ops.create_cube(bm, size=1.0, matrix=matrix, calc_uvs=False)
    faces = [f for f in bm.faces if f not in before]
    if bevel > 0.0:
        edges = {e for f in faces for e in f.edges}
        res = bmesh.ops.bevel(bm, geom=list(edges), offset=m(bevel), offset_type="OFFSET",
                              segments=1, profile=0.5, affect="EDGES", clamp_overlap=True)
        faces = list({f for f in faces if f.is_valid} | set(res.get("faces", [])))
    return faces


def add_tapered_box(bm, cx, cy, z0, z1, half_x0, half_y0, half_x1, half_y1, bevel=0.0):
    """A box whose top cross-section differs from its bottom (a taper). uu."""
    before = set(bm.faces)
    verts_bottom = [bm.verts.new((m(cx + sx * half_x0), m(cy + sy * half_y0), m(z0)))
                    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    verts_top = [bm.verts.new((m(cx + sx * half_x1), m(cy + sy * half_y1), m(z1)))
                 for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    bm.faces.new(verts_bottom[::-1])
    bm.faces.new(verts_top)
    for i in range(4):
        j = (i + 1) % 4
        bm.faces.new((verts_bottom[i], verts_bottom[j], verts_top[j], verts_top[i]))
    faces = [f for f in bm.faces if f not in before]
    if bevel > 0.0:
        edges = {e for f in faces for e in f.edges}
        res = bmesh.ops.bevel(bm, geom=list(edges), offset=m(bevel), offset_type="OFFSET",
                              segments=1, profile=0.5, affect="EDGES", clamp_overlap=True)
        faces = list({f for f in faces if f.is_valid} | set(res.get("faces", [])))
    bmesh.ops.recalc_face_normals(bm, faces=[f for f in faces if f.is_valid])
    return faces


def add_lathe(bm, cx, cy, profile_uu, segments):
    """Revolve a (radius, z) profile about the vertical axis through (cx, cy). uu.

    A profile that starts and ends at radius 0 yields a CLOSED solid (poles merge)."""
    before = set(bm.faces)
    verts = [bm.verts.new((m(cx + r), m(cy), m(z))) for r, z in profile_uu]
    edges = [bm.edges.new((verts[i], verts[i + 1])) for i in range(len(verts) - 1)]
    bmesh.ops.spin(bm, geom=verts + edges, cent=(m(cx), m(cy), 0.0), axis=(0.0, 0.0, 1.0),
                   dvec=(0.0, 0.0, 0.0), angle=2.0 * math.pi, steps=segments,
                   use_merge=False, use_duplicate=False)
    faces = [f for f in bm.faces if f not in before]
    bmesh.ops.remove_doubles(bm, verts=[v for v in bm.verts if v.is_valid], dist=1e-5)
    faces = [f for f in faces if f.is_valid]
    # drop the degenerate loose profile edges left behind by the spin
    for e in [e for e in bm.edges if e.is_valid and len(e.link_faces) == 0]:
        bm.edges.remove(e)
    for v in [v for v in bm.verts if v.is_valid and len(v.link_edges) == 0]:
        bm.verts.remove(v)
    bmesh.ops.recalc_face_normals(bm, faces=[f for f in bm.faces if f.is_valid])
    return [f for f in faces if f.is_valid]


def add_cone_between(bm, p0_uu, p1_uu, r0, r1, segments):
    """A tapered cylinder from p0 to p1 (uu, 3-tuples), radii in uu."""
    before = set(bm.faces)
    p0 = Vector((m(p0_uu[0]), m(p0_uu[1]), m(p0_uu[2])))
    p1 = Vector((m(p1_uu[0]), m(p1_uu[1]), m(p1_uu[2])))
    direction = p1 - p0
    depth = direction.length
    rot = direction.to_track_quat("Z", "Y").to_matrix().to_4x4()
    matrix = Matrix.Translation((p0 + p1) / 2.0) @ rot
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=m(r0), radius2=m(r1), depth=depth,
                          matrix=matrix, calc_uvs=False)
    return [f for f in bm.faces if f not in before]


def bm_to_object(bm, name, materials):
    mesh = bpy.data.meshes.new(name)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for mat in materials:
        mesh.materials.append(mat)
    return obj


# --------------------------------------------------------------------------- materials (preview only)

def make_material(name, base_color, roughness=0.7, metallic=0.0, emission=None, emission_strength=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = (*base_color, 1.0)
        bsdf.inputs["Roughness"].default_value = roughness
        bsdf.inputs["Metallic"].default_value = metallic
        if emission is not None:
            if "Emission Color" in bsdf.inputs:
                bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
            elif "Emission" in bsdf.inputs:
                bsdf.inputs["Emission"].default_value = (*emission, 1.0)
            if "Emission Strength" in bsdf.inputs:
                bsdf.inputs["Emission Strength"].default_value = emission_strength
    mat.diffuse_color = (*base_color, 1.0)
    return mat


# --------------------------------------------------------------------------- UV + shading

def unwrap(obj):
    """DETERMINISTIC UV path — cube projection + an AABB, no-rotate pack.

    MEASURED FINDING (this task): `uv.smart_project` is NOT reproducible on this
    Blender build — two runs over bit-identical vertex data produced different UVs
    AND a different loop order (torch: verts identical, UV max delta 0.923), so the
    exported FBX changed size run to run. Cube projection is a pure per-face axis
    projection and the AABB packer with rotate=False performs no search, so a
    re-run of this script now reproduces the FBX byte-for-byte apart from the FBX
    header's creation timestamp. UV *layout* is cosmetically irrelevant to these
    props (PROP-CLASS EXCEPTION: no baked T_ set) — reproducibility is not."""
    ensure_object_mode()
    select_only([obj], active=obj)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=1.0, correct_aspect=True, scale_to_bounds=False)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=False, margin=0.02, shape_method="AABB")
    bpy.ops.object.mode_set(mode="OBJECT")
    layers = obj.data.uv_layers
    if not layers:
        raise RuntimeError(f"{obj.name}: smart_project produced no UV layer")
    layers[0].name = "UVMap"          # TASK-038 MikkTSpace law: named exactly UVMap
    while len(layers) > 1:
        layers.remove(layers[1])


def shade(obj, smooth_material_index=None):
    """Flat everywhere (stylised low-poly) except the slot marked smooth."""
    for poly in obj.data.polygons:
        poly.use_smooth = (smooth_material_index is not None
                           and poly.material_index == smooth_material_index)


# --------------------------------------------------------------------------- the props

TORCH = {}
TABLE = {}


def build_torch():
    """SM_Torch — wall bracket + shaft + fire bowl + flame.

    LOCAL SPACE CONTRACT (load-bearing for ACastle::TorchAnchors, TASK-562):
      * origin X = 0 is the WALL-MOUNT FACE (the back plane of the backplate);
        the mesh occupies +X only, so an anchor is a POINT ON A WALL.
      * +X is INTO THE ROOM  -> an anchor's rotation faces into the room, which is
        a StaticMeshComponent's natural forward.
      * origin Z = 0 is the VERTICAL CENTRE of the backplate (a wall point, not a
        floor point), so the anchor Z is the height the bracket is nailed at.
      * Y is symmetric about 0.
    Slots: 0 = TorchBody, 1 = TorchFlame.

    The "dark iron + wood" split inside slot 0 rides on VERTEX COLOUR, same
    mechanism and same polarity rule as SM_WarTable: red 1.0 = the PRIMARY material
    (iron), red 0.0 = the ACCENT (the wooden shaft).  A missing vertex-colour
    stream reads as white (1) and degrades to an ALL-IRON sconce, never to a wooden
    bowl holding fire.
    """
    body = make_material("TorchBody", (0.055, 0.052, 0.050), roughness=0.55, metallic=0.85)
    flame = make_material("TorchFlame", (1.0, 0.45, 0.10), roughness=0.9,
                          emission=(1.0, 0.55, 0.18), emission_strength=12.0)

    bm = _new_bm()
    iron_faces = []
    # backplate against the wall (the mount face is x=0)
    iron_faces += add_box(bm, 0, 14, -22, 22, -32, 32, bevel=2.5)
    # boss / socket block where the shaft leaves the plate
    iron_faces += add_box(bm, 11, 27, -11, 11, -14, 14, bevel=1.5)
    # fire bowl (a closed cup: up the outside, over the rim, down the inside)
    iron_faces += add_lathe(bm, 50, 0,
                            [(0, 26), (10, 26), (25, 52), (24.5, 56), (20, 52), (6, 30), (0, 29)],
                            12)
    # angled WOODEN shaft, plate -> bowl (material_index 2 is a temporary tag)
    wood_faces = set(add_cone_between(bm, (18, 0, -2), (50, 0, 30), 8.0, 6.0, 10))
    flame_faces = set(add_lathe(bm, 50, 0,
                                [(0, 46), (9, 53), (15, 66), (12, 82), (5, 95), (0, 104)], 10))

    for face in bm.faces:
        face.material_index = 1 if face in flame_faces else (2 if face in wood_faces else 0)

    scrap = make_material("_TorchWoodTag", (0.20, 0.12, 0.06))
    obj = bm_to_object(bm, "SM_Torch", [body, flame, scrap])
    unwrap(obj)
    shade(obj, smooth_material_index=1)

    mesh = obj.data
    attr = mesh.color_attributes.new(name="Col", type="BYTE_COLOR", domain="CORNER")
    wood_loops = 0
    for poly in mesh.polygons:
        red = 0.0 if poly.material_index == 2 else 1.0
        for loop_index in poly.loop_indices:
            attr.data[loop_index].color = (red, red, red, 1.0)
        if poly.material_index == 2:
            wood_loops += len(poly.loop_indices)
            poly.material_index = 0            # fold the tag back into slot 0 (body)
    mesh.color_attributes.active_color_index = 0
    mesh.color_attributes.render_color_index = 0
    mesh.materials.pop(index=2)
    bpy.data.materials.remove(scrap)
    reds = np.array([attr.data[i].color[0] for i in range(len(mesh.loops))])
    TORCH["vcol"] = {"layer": attr.name, "domain": attr.domain,
                     "loops_total": int(reds.size),
                     "loops_wood_red0": int((reds < 0.5).sum()),
                     "loops_iron_or_flame_red1": int((reds >= 0.5).sum()),
                     "unique_reds": sorted({round(float(v), 4) for v in reds})}

    # flame bbox centre = the point ATorch::TorchLight should sit at (TASK-558/562)
    obj.data.calc_loop_triangles()
    flame_verts = set()
    for poly in obj.data.polygons:
        if poly.material_index == 1:
            flame_verts.update(poly.vertices)
    co = np.array([list(obj.data.vertices[i].co) for i in sorted(flame_verts)]) * UE_UNITS_PER_METER
    TORCH["flame_bounds_uu"] = [co.min(axis=0).round(2).tolist(), co.max(axis=0).round(2).tolist()]
    TORCH["flame_centre_uu"] = ((co.min(axis=0) + co.max(axis=0)) / 2.0).round(2).tolist()
    TORCH["flame_tris"] = sum(1 for t in obj.data.loop_triangles
                              if obj.data.polygons[t.polygon_index].material_index == 1)
    return obj


def build_war_table():
    """SM_WarTable — low table, raised rim, parchment map surface.

    origin = FLOOR-CONTACT plane (min Z == 0), XY centred, Y-symmetric.
    ONE material slot (WarTable).  The wood/parchment split is carried by VERTEX
    COLOUR: red 1.0 = wood, red 0.0 = parchment, so M_WarTable lerps
    Lerp(A=parchment, B=wood, Alpha=VertexColor.R).  Polarity is deliberate — a
    missing vertex-colour stream reads as white (1) and degrades to an ALL-WOOD
    table instead of an all-parchment one.
    """
    wood_mat = make_material("WarTable", (0.115, 0.070, 0.040), roughness=0.75)

    # ---- wood body -------------------------------------------------------
    bm = _new_bm()
    leg_x, leg_y = 150.0, 90.0
    for sx in (-1, 1):
        for sy in (-1, 1):
            add_tapered_box(bm, sx * leg_x, sy * leg_y, 0.0, 82.0,
                            19.0, 19.0, 14.0, 14.0, bevel=3.0)
    # stretchers between the legs
    add_box(bm, -leg_x - 10, leg_x + 10, -12, 12, 26, 42, bevel=2.0)
    add_box(bm, -18, 18, -leg_y - 10, leg_y + 10, 26, 42, bevel=2.0)
    # table slab
    add_box(bm, -190, 190, -125, 125, 82, 95, bevel=2.5)
    # raised rim (4 bars on the slab edge)
    add_box(bm, -190, 190, -125, -95, 95, 110, bevel=2.0)
    add_box(bm, -190, 190, 95, 125, 95, 110, bevel=2.0)
    add_box(bm, -190, -160, -95, 95, 95, 110, bevel=2.0)
    add_box(bm, 160, 190, -95, 95, 95, 110, bevel=2.0)
    wood = bm_to_object(bm, "WarTable_wood", [wood_mat])

    # ---- parchment sheet (its own island => a CRISP vertex-colour boundary) --
    bm = _new_bm()
    add_box(bm, -157, 157, -92, 92, 95, 97.5, bevel=0.0)
    parch = bm_to_object(bm, "WarTable_parch", [wood_mat])

    for obj, red in ((wood, 1.0), (parch, 0.0)):
        mesh = obj.data
        attr = mesh.color_attributes.new(name="Col", type="BYTE_COLOR", domain="CORNER")
        for i in range(len(mesh.loops)):
            attr.data[i].color = (red, red, red, 1.0)
        mesh.color_attributes.active_color_index = 0
        mesh.color_attributes.render_color_index = 0

    ensure_object_mode()
    select_only([wood, parch], active=wood)
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = "SM_WarTable"
    obj.data.name = "SM_WarTable"

    unwrap(obj)
    shade(obj, smooth_material_index=None)

    mesh = obj.data
    attr = mesh.color_attributes[0]
    reds = np.array([attr.data[i].color[0] for i in range(len(mesh.loops))])
    TABLE["vcol_layer"] = attr.name
    TABLE["vcol_domain"] = attr.domain
    TABLE["vcol_loops_total"] = int(reds.size)
    TABLE["vcol_loops_parchment_red0"] = int((reds < 0.5).sum())
    TABLE["vcol_loops_wood_red1"] = int((reds >= 0.5).sum())
    TABLE["vcol_unique_reds"] = sorted({round(float(v), 4) for v in reds})
    return obj


def build_table_ucx(mesh_name):
    """ONE box hull spanning the full furniture volume (floor -> rim top).

    These props carry NO pipeline_manifest.json entry, so — unlike the castle,
    where the manifest ucx.boxes set is the sole collision authority — the FBX IS
    the collision authority for SM_WarTable.  Stated explicitly so TASK-566 does
    not apply the castle's FBX-COLLISION-GAP reading here."""
    bm = _new_bm()
    add_box(bm, -190, 190, -125, 125, 0, 110, bevel=0.0)
    obj = bm_to_object(bm, f"UCX_{mesh_name}_00", [])
    return obj


# --------------------------------------------------------------------------- export

def y_symmetry(obj, tol_uu=0.05):
    """Measured proof that the UE Y-negation is a no-op on this mesh."""
    mn, mx = mesh_bounds_uu(obj)
    mesh = obj.data
    count = len(mesh.vertices)
    co = np.empty(count * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", co)
    co = (co.reshape(count, 3) * UE_UNITS_PER_METER).round(3)
    original = {tuple(v) for v in co}
    mirrored = {(v[0], -v[1] if v[1] != 0 else 0.0, v[2]) for v in co}
    missing = 0
    for v in mirrored:
        if v in original:
            continue
        near = np.abs(co - np.array(v)).max(axis=1)
        if near.min() > tol_uu:
            missing += 1
    return {"y_min_uu": round(float(mn[1]), 3), "y_max_uu": round(float(mx[1]), 3),
            "verts": count, "unmatched_after_mirror": missing,
            "y_symmetric": missing == 0}


def _flip_winding(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.reverse_faces(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()


def _ue_handedness_precomp(objs):
    """TASK-348 MIRROR-FIX, verbatim contract: mirror_Y + winding flip baked at
    export time so UE's own (X,-Y,Z) negation + re-flip restores authored space.
    An involution — called again to restore the scene."""
    mirror = Matrix.Diagonal((1.0, -1.0, 1.0, 1.0))
    for obj in objs:
        obj.data.transform(mirror)
        _flip_winding(obj.data)
        obj.data.update()


def export_fbx(objs, path, colors=False):
    path.parent.mkdir(parents=True, exist_ok=True)
    ensure_object_mode()
    _ue_handedness_precomp(objs)
    try:
        select_only(objs, active=objs[0])
        kwargs = dict(
            filepath=str(path),
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
        if colors:
            kwargs["colors_type"] = "SRGB"
        bpy.ops.export_scene.fbx(**kwargs)
    finally:
        _ue_handedness_precomp(objs)
    log(f"EXPORT {path} ({path.stat().st_size} bytes)")
    return path


# --------------------------------------------------------------------------- previews

def make_human_proxy(name, at_uu):
    """A 180-uu roster human, for the SCALE renders only. Never exported."""
    bm = _new_bm()
    x, y = at_uu
    add_cone_between(bm, (x, y, 2), (x, y, 88), 21.0, 17.0, 12)      # legs/hips
    add_tapered_box(bm, x, y, 88, 148, 24.0, 15.0, 30.0, 17.0)       # torso
    add_lathe(bm, x, y, [(0, 148), (13, 156), (13, 172), (0, 180)], 12)  # head
    obj = bm_to_object(bm, name, [make_material(f"{name}_mat", (0.35, 0.36, 0.40), roughness=0.9)])
    return obj


def make_wall(name, x_uu, half_y_uu, z0, z1):
    bm = _new_bm()
    add_box(bm, x_uu - 30, x_uu, -half_y_uu, half_y_uu, z0, z1, bevel=0.0)
    return bm_to_object(bm, name, [make_material(f"{name}_mat", (0.28, 0.26, 0.24), roughness=1.0)])


def _make_camera(name):
    cam_data = bpy.data.cameras.new(name)
    cam_data.type = "ORTHO"
    cam_obj = bpy.data.objects.new(name, cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    return cam_obj


def _aim(cam, center, direction, distance, ortho_scale):
    direction = Vector(direction).normalized()
    cam.location = Vector(center) + direction * distance
    cam.rotation_euler = (-direction).to_track_quat("-Z", "Y").to_euler()
    cam.data.ortho_scale = ortho_scale


def world_bounds_uu(obj):
    """WORLD-space bounds in uu — preview objects carry a location offset, so the
    local-space readback used for the asset report cannot frame the camera."""
    mesh = obj.data
    count = len(mesh.vertices)
    co = np.empty(count * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", co)
    world = np.array([list(obj.matrix_world @ Vector(v)) for v in co.reshape(count, 3)])
    world *= UE_UNITS_PER_METER
    return world.min(axis=0), world.max(axis=0)


def render_set(tag, objs, views, px=768, color_type="MATERIAL"):
    scene = bpy.context.scene
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    shown = set(objs)
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.hide_render = obj not in shown
    pts = []
    for obj in objs:
        mn, mx = world_bounds_uu(obj)
        pts.append(mn)
        pts.append(mx)
    pts = np.array(pts)
    mn, mx = pts.min(axis=0), pts.max(axis=0)
    center = Vector(((mn + mx) / 2.0 / UE_UNITS_PER_METER).tolist())
    max_dim = float((mx - mn).max()) / UE_UNITS_PER_METER
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = color_type
    scene.display.shading.show_cavity = (color_type == "MATERIAL")
    scene.render.resolution_x = scene.render.resolution_y = px
    cam = _make_camera(f"Cam_{tag}")
    scene.camera = cam
    out_paths = []
    for view_name, direction in views.items():
        _aim(cam, center, direction, max_dim * 4.0, max_dim * 1.25)
        out = PREVIEW_DIR / f"{tag}_{view_name}.png"
        scene.render.filepath = str(out)
        bpy.ops.render.render(write_still=True)
        out_paths.append(str(out))
        log(f"  preview {out.name}")
    bpy.data.objects.remove(cam, do_unlink=True)
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.hide_render = False
    return out_paths


# --------------------------------------------------------------------------- round-trip probe

def probe_fbx(path, expect_ucx=False):
    """Re-import the written FBX into a fresh scene and apply diag(1,-1,1) to
    EMULATE UE's import negation — i.e. measure what the editor will actually get,
    rather than what this script believes it wrote."""
    reset_scene()
    bpy.ops.import_scene.fbx(filepath=str(path))
    result = {"file": str(path), "bytes": path.stat().st_size, "objects": {}}
    for obj in list(bpy.context.scene.objects):
        if obj.type != "MESH":
            continue
        mesh = obj.data
        count = len(mesh.vertices)
        co = np.empty(count * 3, dtype=np.float64)
        mesh.vertices.foreach_get("co", co)
        co = co.reshape(count, 3)
        world = np.array([list(obj.matrix_world @ Vector(v)) for v in co]) * UE_UNITS_PER_METER
        world[:, 1] *= -1.0                      # emulate UE's RH -> LH Y negation
        mesh.calc_loop_triangles()
        entry = {
            "bounds_min_uu": world.min(axis=0).round(2).tolist(),
            "bounds_max_uu": world.max(axis=0).round(2).tolist(),
            "dims_uu": (world.max(axis=0) - world.min(axis=0)).round(2).tolist(),
            "verts": count,
            "tris": len(mesh.loop_triangles),
            "uv_layers": [layer.name for layer in mesh.uv_layers],
            "material_slots": [ms.material.name if ms.material else None for ms in obj.material_slots],
            "color_attributes": [a.name for a in mesh.color_attributes],
        }
        if mesh.color_attributes:
            attr = mesh.color_attributes[0]
            reds = np.array([attr.data[i].color[0] for i in range(len(mesh.loops))])
            entry["vcol_unique_reds"] = sorted({round(float(v), 3) for v in reds})
            entry["vcol_loops_red0"] = int((reds < 0.5).sum())
            entry["vcol_loops_red1"] = int((reds >= 0.5).sum())
        result["objects"][obj.name] = entry
    if expect_ucx:
        result["ucx_nodes"] = [n for n in result["objects"] if n.upper().startswith("UCX_")]
    return result


# --------------------------------------------------------------------------- main

def main():
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    report = {"task": "TASK-556", "human_reference_uu": HUMAN_UU, "assets": {}, "warnings": []}

    # ============================================================= SM_Torch
    reset_scene()
    torch = build_torch()
    mn, mx = mesh_bounds_uu(torch)
    torch_entry = {
        "object": torch.name,
        "bounds_min_uu": mn.round(2).tolist(),
        "bounds_max_uu": mx.round(2).tolist(),
        "dims_uu": (mx - mn).round(2).tolist(),
        "height_vs_human": round(float(mx[2] - mn[2]) / HUMAN_UU, 3),
        "tris": tri_count(torch),
        "verts": len(torch.data.vertices),
        "tri_budget": 600,
        "uv_layers": [layer.name for layer in torch.data.uv_layers],
        "material_slots": [ms.material.name for ms in torch.material_slots],
        "origin_wall_mount_face_x0": round(float(mn[0]), 4),
        "origin_plate_centre_z": [round(float(mn[2]), 2), round(float(mx[2]), 2)],
        "flame_bounds_uu": TORCH["flame_bounds_uu"],
        "flame_centre_uu": TORCH["flame_centre_uu"],
        "flame_tris": TORCH["flame_tris"],
        "vertex_colour": TORCH["vcol"],
        "y_symmetry": y_symmetry(torch),
    }
    torch_entry["tri_budget_ok"] = torch_entry["tris"] <= 600
    if not torch_entry["tri_budget_ok"]:
        report["warnings"].append(f"SM_Torch {torch_entry['tris']} tris exceeds the 600 budget")
    if abs(torch_entry["origin_wall_mount_face_x0"]) > 1e-3:
        report["warnings"].append("SM_Torch min-X is not 0 — the origin is not the wall-mount face")
    export_fbx([torch], RAW_DIR / "Torch.fbx", colors=True)
    torch_entry["fbx"] = str(RAW_DIR / "Torch.fbx")
    torch_entry["fbx_bytes"] = (RAW_DIR / "Torch.fbx").stat().st_size

    torch_entry["previews"] = render_set("Torch", [torch], {
        "side": (0.0, -1.0, 0.0), "front_from_room": (1.0, 0.0, 0.0),
        "threequarter": (0.72, -0.62, 0.30), "top": (0.0, 0.0, 1.0)})
    # VERTEX-COLOUR pass: the iron/wood mask must be SEEN, not just counted.
    torch_entry["previews"] += render_set("TorchVCol", [torch], {
        "vcol_side": (0.0, -1.0, 0.0), "vcol_threequarter": (0.72, -0.62, 0.30)},
        color_type="VERTEX")
    # SCALE shot: a wall segment, the 180-uu roster human, and the torch hung at a
    # plausible sconce height. The mount height itself is TASK-562's TorchAnchors call.
    wall = make_wall("PreviewWall", 0.0, 200.0, -20.0, 560.0)
    human = make_human_proxy("HumanProxy_180uu", (200.0, 0.0))
    torch.location = Vector((0.0, 0.0, m(320.0)))
    torch_entry["previews"] += render_set("TorchScale", [torch, wall, human], {
        "scale_side": (0.0, -1.0, 0.0), "scale_threequarter": (0.75, -0.60, 0.20)})
    torch.location = Vector((0.0, 0.0, 0.0))
    report["assets"]["SM_Torch"] = torch_entry

    # ============================================================= SM_WarTable
    reset_scene()
    table = build_war_table()
    ucx = build_table_ucx(table.name)
    mn, mx = mesh_bounds_uu(table)
    umn, umx = mesh_bounds_uu(ucx)
    table_entry = {
        "object": table.name,
        "bounds_min_uu": mn.round(2).tolist(),
        "bounds_max_uu": mx.round(2).tolist(),
        "dims_uu": (mx - mn).round(2).tolist(),
        "height_vs_human": round(float(mx[2] - mn[2]) / HUMAN_UU, 3),
        "tris": tri_count(table),
        "verts": len(table.data.vertices),
        "tri_budget": 800,
        "uv_layers": [layer.name for layer in table.data.uv_layers],
        "material_slots": [ms.material.name for ms in table.material_slots],
        "origin_floor_contact_min_z": round(float(mn[2]), 4),
        "half_diagonal_uu": round(float(math.hypot(max(abs(mn[0]), mx[0]), max(abs(mn[1]), mx[1]))), 2),
        "vertex_colour": {k: v for k, v in TABLE.items()},
        "y_symmetry": y_symmetry(table),
        "ucx": {"object": ucx.name,
                "bounds_min_uu": umn.round(2).tolist(),
                "bounds_max_uu": umx.round(2).tolist(),
                "dims_uu": (umx - umn).round(2).tolist(),
                "tris": tri_count(ucx), "verts": len(ucx.data.vertices)},
    }
    table_entry["tri_budget_ok"] = table_entry["tris"] <= 800
    if not table_entry["tri_budget_ok"]:
        report["warnings"].append(f"SM_WarTable {table_entry['tris']} tris exceeds the 800 budget")
    if abs(table_entry["origin_floor_contact_min_z"]) > 1e-3:
        report["warnings"].append("SM_WarTable min-Z is not 0 — the origin is not the floor plane")
    export_fbx([table, ucx], RAW_DIR / "WarTable.fbx", colors=True)
    table_entry["fbx"] = str(RAW_DIR / "WarTable.fbx")
    table_entry["fbx_bytes"] = (RAW_DIR / "WarTable.fbx").stat().st_size

    bpy.data.objects.remove(ucx, do_unlink=True)
    table_entry["previews"] = render_set("WarTable", [table], {
        "front": (0.0, -1.0, 0.0), "threequarter": (-0.66, -0.66, 0.42),
        "top": (0.0, 0.0, 1.0), "side": (1.0, 0.0, 0.0)})
    # VERTEX-COLOUR pass: the wood/parchment mask must be SEEN, not just counted.
    table_entry["previews"] += render_set("WarTableVCol", [table], {
        "vcol_top": (0.0, 0.0, 1.0), "vcol_threequarter": (-0.66, -0.66, 0.42)},
        color_type="VERTEX")
    # SCALE shot: the 180-uu roster human standing clear of the table's +X end.
    human = make_human_proxy("HumanProxy_180uu", (300.0, 0.0))
    table_entry["previews"] += render_set("WarTableScale", [table, human], {
        "scale_front": (0.0, -1.0, 0.0), "scale_threequarter": (-0.62, -0.70, 0.28)})
    report["assets"]["SM_WarTable"] = table_entry

    # ============================================================= round-trip probes
    report["round_trip_probe_note"] = (
        "Each FBX re-imported into a fresh headless scene and multiplied by diag(1,-1,1) "
        "to emulate UE's right->left-handed import negation. These numbers are what the "
        "editor receives, not what the authoring scene believed it wrote.")
    report["round_trip"] = {
        "Torch.fbx": probe_fbx(RAW_DIR / "Torch.fbx"),
        "WarTable.fbx": probe_fbx(RAW_DIR / "WarTable.fbx", expect_ucx=True),
    }

    out = CACHE_DIR / "props_report.json"
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    log(f"REPORT {out}")
    log(json.dumps({k: {"dims_uu": v["dims_uu"], "tris": v["tris"]}
                    for k, v in report["assets"].items()}))
    if report["warnings"]:
        for warning in report["warnings"]:
            log(f"WARNING: {warning}")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        traceback.print_exc()
        sys.exit(1)
