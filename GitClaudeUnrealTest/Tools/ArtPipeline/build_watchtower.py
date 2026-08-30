"""build_watchtower.py -- TASK-727 [TOWER-3] SM_WatchTower + the walkable ramp.

Procedurally authored, HEADLESS (the WR-4 / TASK-657 prop-authoring lane -- zero
TRELLIS / Meshy spend), with the FULL texture-bake lane (T_WatchTower_{D,N,ORM} +
the two-slot [TeamRegion, WatchTowerPBR] contract).

    "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background \
        --factory-startup --python-exit-code 1 \
        --python Tools/ArtPipeline/build_watchtower.py

WHY THIS IS HAND-AUTHORED AND NOT A TRELLIS RUN
-----------------------------------------------
The defining property of this asset is ARITHMETIC, not silhouette: TASK-725's nav
spike measured the real Recast slope ceiling on this project's coarsened cells
(CellSize 32 / CellHeight 20 => walkableClimb = ceil(35/20) = 2 voxels =>
rcFilterLedgeSpans clause B shreds any ramp whose rise-per-cell exceeds 1 voxel =>
ceiling = atan(20/32) = 32.005 deg, NOT the 44 deg AgentMaxSlope). A generative mesh
cannot hold 30.000 deg. Every number below is measured, none is a preference.

THE BUDGET (TASK-725 section 3, and it REFUTES the board's original 40 deg)
--------------------------------------------------------------------------
    slope            30.0 deg      under the 32.005 deg ledge-filter ceiling
    rise             1200 uu       T-5 / PlatformHeightUU
    run              2078.4610 uu  1200 / tan(30)
    sloped face      2400.0000 uu  1200 / sin(30)
    deck width       300 uu        NOT 200: ledge+erosion cost 128 uu
    clearance        >= 200 uu     nav agent 144, but ASummonedUnit's capsule is 176
    platform         600x600 uu    clear walkable, parapet OUTSIDE it
    junction         FLUSH         a lip > 40 uu (2 voxels) severs the connection
    form             SOLID WEDGE   ABuilding omits bFillCollisionUnderneathForNavmesh

COLLISION IS AUTHORED, NEVER DECOMPOSED
---------------------------------------
14 hand-placed UCX_SM_WatchTower_NN convex hulls, each an exact primitive.
Hull 00 is the ramp: a triangular prism whose TOP FACE IS THE 30 deg PLANE ITSELF,
so the walkable collision surface is mathematically identical to the render surface.
TL-2 (W6-R3) is the licence: UCX hulls DO survive UE 5.8 import when the node names
bind -- ONE render node (SM_WatchTower) and UCX_<that name>_NN byte-for-byte.
The script raycasts the exported hull soup against the analytic surfaces and reports
the worst-case deviation; that number is the deliverable, not a claim.

OUTPUTS
    Content/RawAssets/WatchTower.fbx
    Content/RawAssets/Textures/WatchTower/T_WatchTower_{D,N,ORM}.png
    Tools/ArtPipeline/Cache/WatchTower/{watchtower_report.json, previews/, debug/}
"""

import hashlib
import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "Tools" / "ArtPipeline" / "Cache" / "WatchTower"
PREVIEWS = CACHE / "previews"
DEBUG = CACHE / "debug"
RAW = ROOT / "Content" / "RawAssets"
RAWTEX = RAW / "Textures" / "WatchTower"

ASSET = "WatchTower"
NODE = "SM_WatchTower"
UE = 100.0                      # uu per metre; Blender works in metres


def m(uu):
    return uu / UE


# ============================================================ THE NAV BUDGET (TASK-725)

SLOPE_DEG = 30.0
RISE = 1200.0                                            # platform deck height
RUN = RISE / math.tan(math.radians(SLOPE_DEG))           # 2078.4610
FACE_LEN = RISE / math.sin(math.radians(SLOPE_DEG))      # 2400.0000

DECK_W = 300.0                  # clear walkable ramp corridor (NOT 200)
YI = DECK_W / 2.0               # 150 -- inner kerb face
KERB_W = 80.0
YO = YI + KERB_W                # 230 -- ramp solid half-width
KERB_H = 120.0                  # kerb rises this far above the ramp deck

RAMP_X0 = 380.0                 # the tower's ramp-side wall plane
RAMP_X1 = RAMP_X0 + RUN         # 2458.4610 -- ramp toe, flush with ground
APRON = 240.0                   # kerbs stop short of the toe: open, flared mouth
KERB_X1 = RAMP_X1 - APRON       # 2218.4610

# tower massing -- EVERY box stops at X = RAMP_X0 on the ramp side, so nothing
# whatsoever overhangs the ramp (the clearance guarantee is geometric, not measured).
PLINTH = (-560.0, 380.0, -470.0, 470.0, 0.0, 140.0)
SHAFT = (-480.0, 380.0, -390.0, 390.0, 140.0, 1000.0)
CORBEL = (-520.0, 380.0, -430.0, 430.0, 1000.0, 1080.0)
CAP = (-560.0, 380.0, -470.0, 470.0, 1080.0, 1200.0)
# String course STOPS SHORT of the ramp plane (x = 340, not 380). A decoration whose
# face lands EXACTLY on its host's face plane with the SAME normal Z-FIGHTS; run 1
# put its +X face on the shaft's +X face at x = 380 and rendered a black rectangle.
# Standing rule for this asset: a decoration is PROUD on every axis where it touches,
# or it stops short. (Coincident faces with OPPOSITE normals are backface-culled and
# are fine -- that is why every stacked box in the massing is safe.)
COURSE = (-500.0, 340.0, -410.0, 410.0, 540.0, 590.0)
BUTTRESS = (-600.0, -480.0, 230.0, 330.0, 0.0, 1000.0)   # mirrored in -Y
DOORWAY = (-500.0, -480.0, -90.0, 90.0, 140.0, 420.0)

PARA_Z0, PARA_Z1 = 1200.0, 1340.0
MERLON_Z1 = 1420.0
# parapet ring, in the outer 80 uu band of the 940x940 cap, with a DECK_W gap on +X
PARAPETS = {
    "W": (-560.0, -480.0, -470.0, 470.0),
    "N": (-480.0, 380.0, 390.0, 470.0),
    "S": (-480.0, 380.0, -470.0, -390.0),
    "EN": (300.0, 380.0, 150.0, 390.0),
    "ES": (300.0, 380.0, -390.0, -150.0),
}
# => clear walkable deck X[-480, 300] x Y[-390, 390] = 780 x 780 (spec floor 600x600)
DECK_CLEAR = (-480.0, 300.0, -390.0, 390.0)

POST_H = 160.0
POST_S = 70.0                   # post cross-section
POST_EMBED = 50.0               # sink into the sloped kerb top

FAM_ASHLAR, FAM_TIMBER, FAM_EARTH, FAM_COPING, FAM_PAVING = 0, 1, 2, 3, 4

# ---------------------------------------------------------------- bake / delight config
DELIGHT = {                     # THE LOCKED PROFILE (TASK-630 / DEFAULT-TRAP law)
    "ao_divide_strength": 1.0,
    "ao_floor": 0.25,
    "gamma": 0.55,
    "gain": 1.2,
    "shoulder": 0.80,
    "max_out": 0.98,
}
ALBEDO_FLOOR = 0.2536
BAKE_RES = 2048                 # building path
MARGIN_PX = 16
SAMPLES = 8
AO_SAMPLES = 64
AO_DISTANCE_M = 2.5

RNG = np.random.RandomState(727)


def inv_delight(post):
    return tuple(min(1.0, (c / DELIGHT["gain"]) ** (1.0 / DELIGHT["gamma"])) for c in post)


# intended POST-delight linear palettes (what should ship on screen)
POST = {
    "ashlar_light": (0.400, 0.388, 0.356),
    "ashlar_dark":  (0.286, 0.276, 0.253),
    "mortar":       (0.218, 0.211, 0.198),
    "timber_light": (0.382, 0.256, 0.138),
    "timber_dark":  (0.250, 0.155, 0.080),
    "earth_light":  (0.336, 0.277, 0.194),
    "earth_dark":   (0.229, 0.188, 0.130),
    "coping_light": (0.336, 0.328, 0.314),
    "coping_dark":  (0.234, 0.229, 0.219),
    "paving_light": (0.372, 0.362, 0.340),
    "paving_dark":  (0.262, 0.254, 0.238),
}
PRE = {k: inv_delight(v) for k, v in POST.items()}
INTENT_WEIGHTS = {              # rough area weights for the retention gate
    "ashlar_light": 0.15, "ashlar_dark": 0.15, "mortar": 0.04,
    "timber_light": 0.12, "timber_dark": 0.08,
    "earth_light": 0.15, "earth_dark": 0.11,
    "coping_light": 0.07, "coping_dark": 0.05,
    "paving_light": 0.05, "paving_dark": 0.03,
}


def log(msg):
    print(f"[T727] {msg}", flush=True)


def srgb_encode(lin):
    lin = np.clip(lin, 0.0, 1.0)
    return np.where(lin <= 0.0031308, lin * 12.92,
                    1.055 * np.power(np.maximum(lin, 0.0), 1.0 / 2.4) - 0.055)


def srgb_decode(enc):
    enc = np.clip(enc, 0.0, 1.0)
    return np.where(enc <= 0.04045, enc / 12.92,
                    np.power((enc + 0.055) / 1.055, 2.4))


# ============================================================================ scene utils

def wipe_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for coll in (bpy.data.meshes, bpy.data.materials, bpy.data.images,
                 bpy.data.lights, bpy.data.cameras):
        for block in list(coll):
            if block.users == 0:
                coll.remove(block)


def ensure_object_mode():
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def select_only(objs, active=None):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objs:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = active or objs[0]


def bm_to_object(bm, name, materials):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for mat in materials:
        mesh.materials.append(mat)
    return obj


def new_mat(name):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        if n.type not in ("BSDF_PRINCIPLED", "OUTPUT_MATERIAL"):
            nt.nodes.remove(n)
    return mat


def principled(mat):
    return next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")


def add_node(mat, type_name, **props):
    node = mat.node_tree.nodes.new(type_name)
    for key, val in props.items():
        setattr(node, key, val)
    return node


def link(mat, out_sock, in_sock):
    mat.node_tree.links.new(out_sock, in_sock)


# ============================================================================ geometry

def make_solid(name, faces, mats):
    """One CLOSED solid -> one object. Verts are welded WITHIN the solid (so the
    island is manifold and recalc_face_normals is deterministic) and NEVER across
    solids (cross-solid welding would create non-manifold point contacts and flip
    normals). `faces` is a list of (points_uu, family_index)."""
    bm = bmesh.new()
    for pts, fam in faces:
        verts = [bm.verts.new((m(p[0]), m(p[1]), m(p[2]))) for p in pts]
        f = bm.faces.new(verts)
        f.material_index = fam
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bm.faces.ensure_lookup_table()
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm_to_object(bm, name, mats)


def box_faces(x0, x1, y0, y1, z0, z1, fam, top_fam=None):
    """Closed axis-aligned box. `top_fam` overrides the +Z face family."""
    return [
        ([(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)], fam),
        ([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], fam),
        ([(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], fam),
        ([(x0, y1, z0), (x0, y1, z1), (x1, y1, z1), (x1, y1, z0)], fam),
        ([(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)], fam),
        ([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)],
         fam if top_fam is None else top_fam),
    ]


def wedge_faces():
    """THE RAMP: a closed triangular prism resting on the ground (a SOLID WEDGE,
    never a floating plank -- ABuilding omits bFillCollisionUnderneathForNavmesh).
    Its +Z face IS the 30 deg walkable plane, full width, unbroken end to end."""
    x0, x1 = RAMP_X0, RAMP_X1
    return [
        ([(x0, -YO, 0.0), (x0, YO, 0.0), (x0, YO, RISE), (x0, -YO, RISE)], FAM_EARTH),
        ([(x0, -YO, 0.0), (x1, -YO, 0.0), (x1, YO, 0.0), (x0, YO, 0.0)], FAM_EARTH),
        ([(x0, -YO, RISE), (x0, YO, RISE), (x1, YO, 0.0), (x1, -YO, 0.0)], FAM_TIMBER),
        ([(x0, -YO, 0.0), (x0, -YO, RISE), (x1, -YO, 0.0)], FAM_EARTH),
        ([(x0, YO, 0.0), (x0, YO, RISE), (x1, YO, 0.0)], FAM_EARTH),
    ]


def kerb_faces(sgn, fam=None):
    """Closed slanted prism (a parallelepiped -- convex) resting ON the ramp plane,
    OUTSIDE the 300 uu deck. TASK-725 section 3: a rail carved OUT of the deck
    would seed erosion from its own face and cost another 2 cells per side."""
    fam = FAM_TIMBER if fam is None else fam
    a, b = sgn * YI, sgn * YO
    h0, h1 = RISE, ramp_z(KERB_X1)
    p = [(RAMP_X0, a, h0), (RAMP_X0, a, h0 + KERB_H),
         (KERB_X1, a, h1 + KERB_H), (KERB_X1, a, h1),
         (RAMP_X0, b, h0), (RAMP_X0, b, h0 + KERB_H),
         (KERB_X1, b, h1 + KERB_H), (KERB_X1, b, h1)]
    return [([p[0], p[1], p[2], p[3]], fam),
            ([p[4], p[5], p[6], p[7]], fam),
            ([p[0], p[1], p[5], p[4]], fam),
            ([p[3], p[2], p[6], p[7]], fam),
            ([p[1], p[2], p[6], p[5]], fam),
            ([p[0], p[3], p[7], p[4]], fam)]


def ramp_z(x):
    """THE walkable plane. z = 1200 at x = 380, z = 0 at x = 2458.4610; 30.000 deg."""
    return RISE * (RAMP_X1 - x) / RUN


def merlon_centres(a, b, pitch):
    n = max(1, int((b - a) // pitch))
    mid = 0.5 * (a + b)
    return [mid + (i - (n - 1) * 0.5) * pitch for i in range(n)]


def build_render_mesh():
    mats = [new_mat("art_ashlar"), new_mat("art_timber"), new_mat("art_earth"),
            new_mat("art_coping"), new_mat("art_paving")]
    parts = []

    def solid(name, faces):
        parts.append(make_solid(name, faces, mats))

    # ---- tower body ----------------------------------------------------------
    solid("p_plinth", box_faces(*PLINTH, FAM_ASHLAR))
    solid("p_shaft", box_faces(*SHAFT, FAM_ASHLAR))
    solid("p_corbel", box_faces(*CORBEL, FAM_ASHLAR))
    # the cap's +Z face IS the platform deck -> its own paving family
    solid("p_cap", box_faces(*CAP, FAM_ASHLAR, top_fam=FAM_PAVING))
    solid("p_course", box_faces(*COURSE, FAM_COPING))
    solid("p_butt_p", box_faces(*BUTTRESS, FAM_ASHLAR))
    bx0, bx1, by0, by1, bz0, bz1 = BUTTRESS
    solid("p_butt_n", box_faces(bx0, bx1, -by1, -by0, bz0, bz1, FAM_ASHLAR))
    solid("p_door", box_faces(*DOORWAY, FAM_COPING))

    # ---- parapet ring, OUTSIDE the 780x780 clear deck ------------------------
    for key, (x0, x1, y0, y1) in PARAPETS.items():
        solid(f"p_para_{key}", box_faces(x0, x1, y0, y1, PARA_Z0, PARA_Z1, FAM_COPING))

    # ---- merlons (TeamRegion) ------------------------------------------------
    ml = 104.0
    for key, (x0, x1, y0, y1) in PARAPETS.items():
        if (x1 - x0) >= (y1 - y0):                            # long axis = X
            for i, c in enumerate(merlon_centres(x0, x1, 188.0)):
                solid(f"p_mer_{key}{i}", box_faces(c - ml / 2, c + ml / 2, y0, y1,
                                                   PARA_Z1, MERLON_Z1, FAM_COPING))
        else:                                                 # long axis = Y
            cs = merlon_centres(y0, y1, 188.0)
            if key == "ES":                                   # mirror EN exactly
                cs = [-c for c in merlon_centres(PARAPETS["EN"][2],
                                                 PARAPETS["EN"][3], 188.0)]
            for i, c in enumerate(cs):
                solid(f"p_mer_{key}{i}", box_faces(x0, x1, c - ml / 2, c + ml / 2,
                                                   PARA_Z1, MERLON_Z1, FAM_COPING))

    # ---- the ramp ------------------------------------------------------------
    solid("p_wedge", wedge_faces())
    solid("p_kerb_p", kerb_faces(1.0))
    solid("p_kerb_n", kerb_faces(-1.0))

    # ---- kerb posts (TeamRegion) -- the "walk here" marker rhythm ------------
    post_x = []
    x = RAMP_X0 + 190.0
    while x <= KERB_X1 - 190.0:
        post_x.append(x)
        x += 400.0
    for i, cx in enumerate(post_x):
        base = ramp_z(cx) + KERB_H - POST_EMBED
        for sgn, tag in ((1.0, "p"), (-1.0, "n")):
            cy = sgn * (YI + KERB_W / 2.0)
            solid(f"p_post_{tag}{i}",
                  box_faces(cx - POST_S / 2, cx + POST_S / 2,
                            cy - POST_S / 2, cy + POST_S / 2,
                            base, base + POST_H, FAM_COPING))

    ensure_object_mode()
    select_only(parts, active=parts[0])
    bpy.ops.object.join()
    obj = parts[0]
    obj.name = NODE
    obj.data.name = NODE
    for poly in obj.data.polygons:
        poly.use_smooth = False                  # FACE smoothing -- crisp masonry
    return obj, post_x


# ============================================================ collision (12 exact hulls)

def build_hulls():
    """14 hand-placed convex hulls, each an EXACT primitive. Hull 00's +Z face IS
    ramp_z() and hull 06's +Z face IS z = 1200, so the walkable collision surface
    is the walkable render surface -- zero deviation BY CONSTRUCTION, then measured.
    NOTHING here is a convex decomposition."""
    hulls = [
        ("ramp_wedge_walkable", make_solid(f"UCX_{NODE}_00", wedge_faces(), [])),
        ("kerb_+Y", make_solid(f"UCX_{NODE}_01", kerb_faces(1.0, 0), [])),
        ("kerb_-Y", make_solid(f"UCX_{NODE}_02", kerb_faces(-1.0, 0), [])),
        ("plinth", make_solid(f"UCX_{NODE}_03", box_faces(*PLINTH, 0), [])),
        ("shaft", make_solid(f"UCX_{NODE}_04", box_faces(*SHAFT, 0), [])),
        ("corbel_ring", make_solid(f"UCX_{NODE}_05", box_faces(*CORBEL, 0), [])),
        ("cap_platform_deck", make_solid(f"UCX_{NODE}_06", box_faces(*CAP, 0), [])),
    ]
    for i, key in enumerate(("W", "N", "S", "EN", "ES")):
        x0, x1, y0, y1 = PARAPETS[key]
        hulls.append((f"parapet_{key}",
                      make_solid(f"UCX_{NODE}_{7 + i:02d}",
                                 box_faces(x0, x1, y0, y1, PARA_Z0, PARA_Z1, 0), [])))
    bx0, bx1, by0, by1, bz0, bz1 = BUTTRESS
    hulls.append(("buttress_+Y",
                  make_solid(f"UCX_{NODE}_12", box_faces(*BUTTRESS, 0), [])))
    hulls.append(("buttress_-Y",
                  make_solid(f"UCX_{NODE}_13",
                             box_faces(bx0, bx1, -by1, -by0, bz0, bz1, 0), [])))
    return hulls


# ============================================================================ materials

def coord(mat, scale):
    tc = add_node(mat, "ShaderNodeTexCoord", location=(-1400, 0))
    mp = add_node(mat, "ShaderNodeMapping", location=(-1200, 0))
    mp.inputs["Scale"].default_value = (scale, scale, scale)
    link(mat, tc.outputs["Object"], mp.inputs["Vector"])
    return mp.outputs["Vector"]


def noise(mat, vec, scale, detail, loc):
    n = add_node(mat, "ShaderNodeTexNoise", location=loc)
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = detail
    link(mat, vec, n.inputs["Vector"])
    return n


def ramp2(mat, fac, c0, c1, loc):
    r = add_node(mat, "ShaderNodeValToRGB", location=loc)
    r.color_ramp.elements[0].color = (*c0, 1.0)
    r.color_ramp.elements[1].color = (*c1, 1.0)
    link(mat, fac, r.inputs["Fac"])
    return r


def finish_shader(mat, colour_sock, rough_base, height_sock, bump_strength):
    p = principled(mat)
    link(mat, colour_sock, p.inputs["Base Color"])
    p.inputs["Metallic"].default_value = 0.0
    rn = add_node(mat, "ShaderNodeMath", location=(-200, -260))
    rn.operation = "ADD"
    rn.inputs[1].default_value = rough_base
    link(mat, height_sock, rn.inputs[0])
    cl = add_node(mat, "ShaderNodeClamp", location=(-40, -260))
    cl.inputs["Min"].default_value = 0.35
    cl.inputs["Max"].default_value = 0.98
    link(mat, rn.outputs[0], cl.inputs["Value"])
    link(mat, cl.outputs["Result"], p.inputs["Roughness"])
    bump = add_node(mat, "ShaderNodeBump", location=(-200, -520))
    bump.inputs["Strength"].default_value = bump_strength
    bump.inputs["Distance"].default_value = 0.02
    link(mat, height_sock, bump.inputs["Height"])
    link(mat, bump.outputs["Normal"], p.inputs["Normal"])


def wall_vec(mat, vec, loc=(-1050, 380)):
    """Blender's Brick node lays columns along X and ROWS along Y, so feeding it raw
    object coords makes every vertical wall a set of VERTICAL STRIPES (run 1's defect:
    on a face whose normal is +-Y the pattern is constant in Z). Re-map to
    (x + y, z) so rows advance with HEIGHT on every vertical face, whichever way it
    looks. Horizontal faces degenerate under this map -- that is why the platform
    deck gets its own art_paving family in true (x, y)."""
    sep = add_node(mat, "ShaderNodeSeparateXYZ", location=loc)
    link(mat, vec, sep.inputs["Vector"])
    s = add_node(mat, "ShaderNodeMath", location=(loc[0] + 170, loc[1] - 90))
    s.operation = "ADD"
    link(mat, sep.outputs["X"], s.inputs[0])
    link(mat, sep.outputs["Y"], s.inputs[1])
    c = add_node(mat, "ShaderNodeCombineXYZ", location=(loc[0] + 340, loc[1]))
    link(mat, s.outputs[0], c.inputs["X"])
    link(mat, sep.outputs["Z"], c.inputs["Y"])
    return c.outputs["Vector"]


def art_ashlar(mat):
    vec = coord(mat, 1.0)
    wall = wall_vec(mat, vec)
    br = add_node(mat, "ShaderNodeTexBrick", location=(-900, 200))
    br.offset = 0.5
    br.squash = 1.0
    br.inputs["Scale"].default_value = 3.4
    br.inputs["Mortar Size"].default_value = 0.021
    br.inputs["Mortar Smooth"].default_value = 0.12
    br.inputs["Bias"].default_value = 0.0
    br.inputs["Brick Width"].default_value = 0.62
    br.inputs["Row Height"].default_value = 0.29
    br.inputs["Color1"].default_value = (*PRE["ashlar_light"], 1.0)
    br.inputs["Color2"].default_value = (*PRE["ashlar_dark"], 1.0)
    br.inputs["Mortar"].default_value = (*PRE["mortar"], 1.0)
    link(mat, wall, br.inputs["Vector"])
    n = noise(mat, vec, 14.0, 6.0, (-900, -220))
    # weathering: darken with fine noise via a multiply-style ColorRamp overlay
    wr = ramp2(mat, n.outputs["Fac"], (0.72, 0.72, 0.72), (1.06, 1.06, 1.06), (-620, -220))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-360, 60))
    mul.operation = "MULTIPLY"
    link(mat, br.outputs["Color"], mul.inputs[0])
    link(mat, wr.outputs["Color"], mul.inputs[1])
    # height: mortar recess + grain
    hm = add_node(mat, "ShaderNodeMath", location=(-620, -420))
    hm.operation = "MULTIPLY"
    hm.inputs[1].default_value = 0.35
    link(mat, n.outputs["Fac"], hm.inputs[0])
    hsum = add_node(mat, "ShaderNodeMath", location=(-440, -420))
    hsum.operation = "ADD"
    link(mat, br.outputs["Fac"], hsum.inputs[0])
    link(mat, hm.outputs[0], hsum.inputs[1])
    finish_shader(mat, mul.outputs["Vector"], 0.62, hsum.outputs[0], 0.32)


def art_timber(mat):
    vec = coord(mat, 1.0)
    # planks run ACROSS the ramp -> bands vary along X (the travel axis)
    w = add_node(mat, "ShaderNodeTexWave", location=(-900, 220))
    w.wave_type = "BANDS"
    w.bands_direction = "X"
    w.wave_profile = "SAW"
    w.inputs["Scale"].default_value = 0.62
    w.inputs["Distortion"].default_value = 0.6
    w.inputs["Detail"].default_value = 2.0
    w.inputs["Detail Scale"].default_value = 0.8
    link(mat, vec, w.inputs["Vector"])
    # per-plank tone jitter
    jn = add_node(mat, "ShaderNodeTexNoise", location=(-900, -60))
    jn.inputs["Scale"].default_value = 1.7
    jn.inputs["Detail"].default_value = 1.0
    link(mat, vec, jn.inputs["Vector"])
    tone = ramp2(mat, jn.outputs["Fac"], PRE["timber_dark"], PRE["timber_light"],
                 (-640, -60))
    # long grain
    gn = add_node(mat, "ShaderNodeTexNoise", location=(-900, -340))
    gn.inputs["Scale"].default_value = 26.0
    gn.inputs["Detail"].default_value = 5.0
    link(mat, vec, gn.inputs["Vector"])
    gr = ramp2(mat, gn.outputs["Fac"], (0.78, 0.78, 0.78), (1.08, 1.08, 1.08), (-640, -340))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-380, 40))
    mul.operation = "MULTIPLY"
    link(mat, tone.outputs["Color"], mul.inputs[0])
    link(mat, gr.outputs["Color"], mul.inputs[1])
    # groove between planks darkens the albedo too
    gv = ramp2(mat, w.outputs["Fac"], (0.55, 0.55, 0.55), (1.0, 1.0, 1.0), (-640, 220))
    mul2 = add_node(mat, "ShaderNodeVectorMath", location=(-200, 120))
    mul2.operation = "MULTIPLY"
    link(mat, mul.outputs["Vector"], mul2.inputs[0])
    link(mat, gv.outputs["Color"], mul2.inputs[1])
    hsum = add_node(mat, "ShaderNodeMath", location=(-380, -560))
    hsum.operation = "ADD"
    link(mat, w.outputs["Fac"], hsum.inputs[0])
    link(mat, gn.outputs["Fac"], hsum.inputs[1])
    finish_shader(mat, mul2.outputs["Vector"], 0.48, hsum.outputs[0], 0.55)


def art_earth(mat):
    """The embankment flank is the single LARGEST surface on the asset (2400 x 1200
    per side). Run 1 left it a featureless slab; it now carries horizontal earth
    strata + rubble so the ramp reads as a rammed-earth siege embankment."""
    vec = coord(mat, 1.0)
    strata = add_node(mat, "ShaderNodeTexWave", location=(-1000, 300))
    strata.wave_type = "BANDS"
    strata.bands_direction = "Z"
    strata.wave_profile = "SIN"
    strata.inputs["Scale"].default_value = 1.15
    strata.inputs["Distortion"].default_value = 4.5
    strata.inputs["Detail"].default_value = 3.0
    strata.inputs["Detail Scale"].default_value = 1.4
    link(mat, vec, strata.inputs["Vector"])
    base = ramp2(mat, strata.outputs["Fac"], PRE["earth_dark"], PRE["earth_light"],
                 (-740, 300))
    v = add_node(mat, "ShaderNodeTexVoronoi", location=(-1000, -60))
    v.feature = "F1"
    v.inputs["Scale"].default_value = 5.2
    link(mat, vec, v.inputs["Vector"])
    rub = ramp2(mat, v.outputs["Distance"], (0.58, 0.58, 0.58), (1.22, 1.22, 1.22),
                (-740, -60))
    n = noise(mat, vec, 22.0, 8.0, (-1000, -400))
    gr = ramp2(mat, n.outputs["Fac"], (0.80, 0.80, 0.80), (1.10, 1.10, 1.10), (-740, -400))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-480, 120))
    mul.operation = "MULTIPLY"
    link(mat, base.outputs["Color"], mul.inputs[0])
    link(mat, rub.outputs["Color"], mul.inputs[1])
    mul2 = add_node(mat, "ShaderNodeVectorMath", location=(-300, 120))
    mul2.operation = "MULTIPLY"
    link(mat, mul.outputs["Vector"], mul2.inputs[0])
    link(mat, gr.outputs["Color"], mul2.inputs[1])
    h1 = add_node(mat, "ShaderNodeMath", location=(-480, -560))
    h1.operation = "ADD"
    link(mat, v.outputs["Distance"], h1.inputs[0])
    link(mat, strata.outputs["Fac"], h1.inputs[1])
    h2 = add_node(mat, "ShaderNodeMath", location=(-300, -560))
    h2.operation = "ADD"
    link(mat, h1.outputs[0], h2.inputs[0])
    link(mat, n.outputs["Fac"], h2.inputs[1])
    finish_shader(mat, mul2.outputs["Vector"], 0.66, h2.outputs[0], 0.85)


def art_coping(mat):
    vec = coord(mat, 1.0)
    wall = wall_vec(mat, vec)
    br = add_node(mat, "ShaderNodeTexBrick", location=(-900, 200))
    br.offset = 0.5
    br.squash = 1.0
    br.inputs["Scale"].default_value = 6.0
    br.inputs["Mortar Size"].default_value = 0.026
    br.inputs["Mortar Smooth"].default_value = 0.10
    br.inputs["Brick Width"].default_value = 0.55
    br.inputs["Row Height"].default_value = 0.34
    br.inputs["Color1"].default_value = (*PRE["coping_light"], 1.0)
    br.inputs["Color2"].default_value = (*PRE["coping_dark"], 1.0)
    br.inputs["Mortar"].default_value = (*PRE["mortar"], 1.0)
    link(mat, wall, br.inputs["Vector"])
    n2 = noise(mat, vec, 30.0, 4.0, (-900, -200))
    gr = ramp2(mat, n2.outputs["Fac"], (0.82, 0.82, 0.82), (1.05, 1.05, 1.05), (-640, -200))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-360, 40))
    mul.operation = "MULTIPLY"
    link(mat, br.outputs["Color"], mul.inputs[0])
    link(mat, gr.outputs["Color"], mul.inputs[1])
    hsum = add_node(mat, "ShaderNodeMath", location=(-360, -440))
    hsum.operation = "ADD"
    link(mat, br.outputs["Fac"], hsum.inputs[0])
    link(mat, n2.outputs["Fac"], hsum.inputs[1])
    finish_shader(mat, mul.outputs["Vector"], 0.60, hsum.outputs[0], 0.34)


def art_paving(mat):
    """The PLATFORM DECK -- a walkable surface, so it gets a true (x, y) flagstone
    read rather than the vertical-wall course map."""
    vec = coord(mat, 1.0)
    br = add_node(mat, "ShaderNodeTexBrick", location=(-900, 200))
    br.offset = 0.5
    br.offset_frequency = 2
    br.squash = 1.0
    br.inputs["Scale"].default_value = 2.6
    br.inputs["Mortar Size"].default_value = 0.03
    br.inputs["Mortar Smooth"].default_value = 0.08
    br.inputs["Brick Width"].default_value = 0.72
    br.inputs["Row Height"].default_value = 0.5
    br.inputs["Color1"].default_value = (*PRE["paving_light"], 1.0)
    br.inputs["Color2"].default_value = (*PRE["paving_dark"], 1.0)
    br.inputs["Mortar"].default_value = (*PRE["mortar"], 1.0)
    link(mat, vec, br.inputs["Vector"])
    n = noise(mat, vec, 16.0, 6.0, (-900, -220))
    wr = ramp2(mat, n.outputs["Fac"], (0.76, 0.76, 0.76), (1.08, 1.08, 1.08), (-640, -220))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-360, 60))
    mul.operation = "MULTIPLY"
    link(mat, br.outputs["Color"], mul.inputs[0])
    link(mat, wr.outputs["Color"], mul.inputs[1])
    hsum = add_node(mat, "ShaderNodeMath", location=(-360, -440))
    hsum.operation = "ADD"
    link(mat, br.outputs["Fac"], hsum.inputs[0])
    link(mat, n.outputs["Fac"], hsum.inputs[1])
    finish_shader(mat, mul.outputs["Vector"], 0.66, hsum.outputs[0], 0.30)


# ============================================================================ UV + bake

def uv_atlas(obj):
    mesh = obj.data
    if not mesh.uv_layers:
        mesh.uv_layers.new(name="UVMap")
    uvmap = mesh.uv_layers["UVMap"]
    uvmap.name = "UVMap"
    mesh.uv_layers.active = uvmap
    uvmap.active_render = True
    bpy.context.scene.tool_settings.use_uv_select_sync = True
    select_only([obj])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=2.0)
    bpy.ops.uv.pack_islands(rotate=False, shape_method="AABB", margin=0.015)
    bpy.ops.object.mode_set(mode="OBJECT")


def cycles_setup():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.use_denoising = False


def set_bake_target(obj, image):
    for slot in obj.material_slots:
        nt = slot.material.node_tree
        node = nt.nodes.get("BakeTarget")
        if node is None:
            node = nt.nodes.new("ShaderNodeTexImage")
            node.name = "BakeTarget"
            node.location = (-1600, -700)
        node.image = image
        for n in nt.nodes:
            n.select = False
        node.select = True
        nt.nodes.active = node


def new_image(name, size, colorspace):
    img = bpy.data.images.new(name, width=size, height=size, alpha=False)
    img.colorspace_settings.name = colorspace
    return img


def image_to_np(img):
    w, h = img.size
    buf = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(buf)
    return buf.reshape(h, w, 4)


def np_to_image(name, arr, colorspace):
    h, w = arr.shape[:2]
    img = bpy.data.images.new(name, width=w, height=h, alpha=False)
    img.colorspace_settings.name = colorspace
    if arr.shape[2] == 3:
        arr = np.concatenate([arr, np.ones((h, w, 1), dtype=arr.dtype)], axis=2)
    img.pixels.foreach_set(arr.astype(np.float32).ravel())
    return img


def save_image(img, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    img.filepath_raw = str(path)
    img.file_format = "PNG"
    img.save()
    return path


def bake_pass(obj, image, bake_type, samples, use_clear, pass_filter=None):
    cycles_setup()
    bpy.context.scene.cycles.samples = samples
    set_bake_target(obj, image)
    select_only([obj])
    kwargs = dict(type=bake_type, margin=MARGIN_PX, use_clear=use_clear,
                  use_selected_to_active=False)
    if pass_filter is not None:
        kwargs["pass_filter"] = pass_filter
    bpy.ops.object.bake(**kwargs)


def bake_ao(obj, image, use_clear):
    ao_mat = new_mat(f"{obj.name}_AO_TMP")
    nt = ao_mat.node_tree
    for n in list(nt.nodes):
        if n.type == "BSDF_PRINCIPLED":
            nt.nodes.remove(n)
    out = next(n for n in nt.nodes if n.type == "OUTPUT_MATERIAL")
    ao_node = nt.nodes.new("ShaderNodeAmbientOcclusion")
    ao_node.inputs["Distance"].default_value = AO_DISTANCE_M
    emit = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(ao_node.outputs["Color"], emit.inputs["Color"])
    nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])

    dup = obj.copy()
    dup.data = obj.data.copy()
    bpy.context.scene.collection.objects.link(dup)
    dup.data.materials.clear()
    dup.data.materials.append(ao_mat)
    hidden = []
    for other in list(bpy.data.objects):
        if other is not dup and other.type == "MESH":
            hidden.append((other, other.hide_render))
            other.hide_render = True
    try:
        bake_pass(dup, image, "EMIT", AO_SAMPLES, use_clear)
    finally:
        for other, prev in hidden:
            other.hide_render = prev
        bpy.data.objects.remove(dup, do_unlink=True)
        bpy.data.materials.remove(ao_mat)


def apply_delight(d_img, ao_img, report):
    cfg = DELIGHT
    rgba = image_to_np(d_img)
    rgb = np.clip(rgba[:, :, :3].astype(np.float64), 0.0, 1.0)
    linear = srgb_decode(rgb)
    mean_before = float(linear.mean())
    ao = np.clip(image_to_np(ao_img)[:, :, 0].astype(np.float64), 0.0, 1.0)
    ao_floored = np.maximum(ao, cfg["ao_floor"])[:, :, None]
    linear = linear + (linear / ao_floored - linear) * cfg["ao_divide_strength"]
    linear = cfg["gain"] * np.power(np.maximum(linear, 0.0), cfg["gamma"])
    knee, ceiling = cfg["shoulder"], cfg["max_out"]
    over = np.maximum(linear - knee, 0.0)
    linear = np.where(linear <= knee, linear,
                      knee + (ceiling - knee) * over / ((ceiling - knee) + over))
    linear = np.clip(linear, 0.0, 1.0)
    report["albedo_delight"] = dict(cfg)
    report["albedo_delight"].update({
        "applied": True,
        "mean_linear_before": round(mean_before, 4),
        "mean_linear_after": round(float(linear.mean()), 4),
        "p99_linear_after": round(float(np.percentile(linear, 99.0)), 4),
        "shoulder_compressed_fraction": round(float(np.mean(linear > knee)), 4),
    })
    out = rgba.copy()
    out[:, :, :3] = srgb_encode(linear).astype(np.float32)
    return out


def pack_orm(ao_img, rough_img):
    ao = image_to_np(ao_img)[:, :, 0]
    rough = image_to_np(rough_img)[:, :, 0]
    h, w = ao.shape
    orm = np.zeros((h, w, 4), dtype=np.float32)
    orm[:, :, 0] = np.clip(ao, 0.0, 1.0)
    orm[:, :, 1] = np.clip(rough, 0.0, 1.0)
    orm[:, :, 2] = 0.0
    orm[:, :, 3] = 1.0
    return orm


def lin_to_lab_chroma(linear_rgb):
    M = np.array([[0.4124564, 0.3575761, 0.1804375],
                  [0.2126729, 0.7151522, 0.0721750],
                  [0.0193339, 0.1191920, 0.9503041]])
    xyz = linear_rgb @ M.T
    white = np.array([0.95047, 1.0, 1.08883])
    t = xyz / white
    f = np.where(t > 0.008856, np.cbrt(t), 7.787 * t + 16.0 / 116.0)
    a = 500.0 * (f[:, 0] - f[:, 1])
    b = 200.0 * (f[:, 1] - f[:, 2])
    return np.sqrt(a * a + b * b)


def measure_gates(final_d, orm, report):
    stored = final_d[:, :, :3].astype(np.float64)
    covered = np.any(stored > 0.0, axis=2)
    linear = srgb_decode(stored)
    cov = linear[covered]
    uv_norm = float(cov.mean()) if cov.size else 0.0
    ref = np.array([np.array(POST[k]) * w for k, w in INTENT_WEIGHTS.items()])
    ref_mean = float(sum(r.mean() for r in ref))
    retention = uv_norm / ref_mean if ref_mean > 0 else 0.0
    chroma = float(lin_to_lab_chroma(cov.reshape(-1, 3)).mean()) if cov.size else 0.0
    ref_cols = np.array([POST[k] for k in INTENT_WEIGHTS])
    ref_chroma = float((lin_to_lab_chroma(ref_cols) *
                        np.array(list(INTENT_WEIGHTS.values()))).sum())
    ao = orm[:, :, 0].astype(np.float64)[covered]
    rough = orm[:, :, 1].astype(np.float64)[covered]
    # The retention band [0.85, 1.25] is WARN-ONLY and this asset sits above it.
    # DECLARE the mechanism rather than hide it (the TASK-630 posture): the pinned
    # profile divides linear albedo by max(AO, ao_floor) BEFORE the gamma, so a mesh
    # with this much self-occlusion is lifted by (1/AO)^gamma on its own. Whatever is
    # left after dividing that out is the palette's real error.
    ao_mean = float(ao.mean()) if ao.size else 1.0
    ao_lift = (1.0 / max(ao_mean, DELIGHT["ao_floor"])) ** DELIGHT["gamma"]
    report["gates"] = {
        "coverage_fraction": round(float(covered.mean()), 4),
        "uv_norm_covered_mean_linear": round(uv_norm, 4),
        "albedo_floor": ALBEDO_FLOOR,
        "albedo_floor_pass": bool(uv_norm >= ALBEDO_FLOOR),
        "intent_reference_mean_linear": round(ref_mean, 4),
        "intent_retention": round(retention, 4),
        "intent_retention_band": [0.85, 1.25],
        "intent_retention_ao_divide_component": round(ao_lift, 4),
        "intent_retention_residual_vs_palette": round(retention / ao_lift, 4),
        "anti_bleach_operative_guard_tripped": bool(retention > 1.25 and uv_norm > 0.60),
        "chroma_covered_Cab": round(chroma, 2),
        "chroma_intent_Cab": round(ref_chroma, 2),
        "orm_ao_covered_mean": round(float(ao.mean()), 4) if ao.size else None,
        "orm_rough_covered_min_max": [round(float(rough.min()), 3),
                                      round(float(rough.max()), 3)] if rough.size else None,
        "orm_metal_max": 0.0,
    }
    return report["gates"]


# ============================================================ nav / collision measurement

def bvh_of(objs):
    verts, polys = [], []
    for obj in objs:
        base = len(verts)
        verts += [tuple(v.co) for v in obj.data.vertices]
        polys += [tuple(base + i for i in p.vertices) for p in obj.data.polygons]
    return BVHTree.FromPolygons(verts, polys, all_triangles=False, epsilon=0.0)


def top_hit(bvh, x_uu, y_uu, from_z_uu=2600.0):
    hit = bvh.ray_cast(Vector((m(x_uu), m(y_uu), m(from_z_uu))), Vector((0, 0, -1)))
    return None if hit[0] is None else hit[0].z * UE


def up_clearance(bvh, x_uu, y_uu, z_uu, lift=2.0, reach=4000.0):
    hit = bvh.ray_cast(Vector((m(x_uu), m(y_uu), m(z_uu + lift))), Vector((0, 0, 1)),
                       m(reach))
    return None if hit[0] is None else (hit[0].z * UE - z_uu)


def measure_nav(render_obj, hull_objs, report):
    hull_bvh = bvh_of(hull_objs)
    rend_bvh = bvh_of([render_obj])

    # ---- ramp deck: analytic vs authored collision -------------------------
    xs = np.linspace(RAMP_X0 + 1.0, RAMP_X1 - 1.0, 400)
    ys = np.linspace(-YI + 2.0, YI - 2.0, 21)
    dev_ramp, miss_ramp, clr_ramp = [], 0, []
    for x in xs:
        for y in ys:
            z_true = ramp_z(x)
            z_col = top_hit(hull_bvh, x, y)
            if z_col is None:
                miss_ramp += 1
                continue
            dev_ramp.append(z_col - z_true)
            c = up_clearance(hull_bvh, x, y, z_col)
            clr_ramp.append(1e9 if c is None else c)

    # ---- platform deck ------------------------------------------------------
    px = np.linspace(DECK_CLEAR[0] + 2.0, DECK_CLEAR[1] - 2.0, 60)
    py = np.linspace(DECK_CLEAR[2] + 2.0, DECK_CLEAR[3] - 2.0, 60)
    dev_deck, miss_deck, clr_deck = [], 0, []
    for x in px:
        for y in py:
            z_col = top_hit(hull_bvh, x, y)
            if z_col is None:
                miss_deck += 1
                continue
            dev_deck.append(z_col - RISE)
            c = up_clearance(hull_bvh, x, y, z_col)
            clr_deck.append(1e9 if c is None else c)

    # ---- the junction: the fragile seam ------------------------------------
    # Both surfaces are planes meeting at x = RAMP_X0. Sample each side one uu away
    # and extrapolate the ramp's own slope back to the seam; what is left is the LIP.
    delta = 1.0
    slope_rise = delta * math.tan(math.radians(SLOPE_DEG))
    seam, seam_render = [], []
    for y in (-140.0, -70.0, 0.0, 70.0, 140.0):
        z_lo = top_hit(hull_bvh, RAMP_X0 - delta, y)            # platform side
        z_hi = top_hit(hull_bvh, RAMP_X0 + delta, y)            # ramp side
        seam.append(abs(z_lo - (z_hi + slope_rise)))
        r_lo = top_hit(rend_bvh, RAMP_X0 - delta, y)
        r_hi = top_hit(rend_bvh, RAMP_X0 + delta, y)
        seam_render.append(abs(r_lo - (r_hi + slope_rise)))

    # ---- slope, measured from the collision surface itself -----------------
    z_a = top_hit(hull_bvh, RAMP_X0 + 20.0, 0.0)
    z_b = top_hit(hull_bvh, RAMP_X1 - 20.0, 0.0)
    slope_col = math.degrees(math.atan2(z_a - z_b, (RAMP_X1 - 20.0) - (RAMP_X0 + 20.0)))
    # and from the RENDER surface
    r_a = top_hit(rend_bvh, RAMP_X0 + 20.0, 0.0)
    r_b = top_hit(rend_bvh, RAMP_X1 - 20.0, 0.0)
    slope_rend = math.degrees(math.atan2(r_a - r_b, (RAMP_X1 - 20.0) - (RAMP_X0 + 20.0)))

    # ---- render-mesh clearance over both walking surfaces ------------------
    clr_rend = []
    for x in np.linspace(RAMP_X0 + 2.0, RAMP_X1 - 2.0, 200):
        for y in (-140.0, -70.0, 0.0, 70.0, 140.0):
            z = top_hit(rend_bvh, x, y)
            if z is None:
                continue
            c = up_clearance(rend_bvh, x, y, z)
            clr_rend.append(1e9 if c is None else c)
    for x in np.linspace(DECK_CLEAR[0] + 2.0, DECK_CLEAR[1] - 2.0, 40):
        for y in np.linspace(DECK_CLEAR[2] + 2.0, DECK_CLEAR[3] - 2.0, 40):
            z = top_hit(rend_bvh, x, y)
            if z is None:
                continue
            c = up_clearance(rend_bvh, x, y, z)
            clr_rend.append(1e9 if c is None else c)

    cell, cellh, climb = 32.0, 20.0, 2
    rise_per_cell = cell * math.tan(math.radians(slope_col))
    report["nav"] = {
        "target_slope_deg": SLOPE_DEG,
        "ledge_filter_ceiling_deg": round(math.degrees(math.atan(cellh / cell)), 3),
        "measured_slope_collision_deg": round(slope_col, 4),
        "measured_slope_render_deg": round(slope_rend, 4),
        "rise_per_cell_uu": round(rise_per_cell, 4),
        "rise_per_cell_voxels": round(rise_per_cell / cellh, 4),
        "walkable_climb_voxels": climb,
        "ledge_clause_b_pass": bool(rise_per_cell / cellh <= 1.0),
        "run_uu": round(RUN, 4),
        "sloped_face_len_uu": round(FACE_LEN, 4),
        "rise_uu": RISE,
        "ramp_deck_clear_width_uu": DECK_W,
        "ramp_corridor_after_erosion_uu": round(DECK_W - 128.0, 1),
        "platform_clear_uu": [round(DECK_CLEAR[1] - DECK_CLEAR[0], 1),
                              round(DECK_CLEAR[3] - DECK_CLEAR[2], 1)],
        "platform_after_erosion_uu": round(DECK_CLEAR[1] - DECK_CLEAR[0] - 128.0, 1),
        "junction_lip_uu_max": round(max(seam), 5),
        "junction_lip_render_uu_max": round(max(seam_render), 5),
        "junction_lip_budget_uu": 40.0,
        "collision_deviation_ramp_uu": {
            "max_abs": round(float(np.max(np.abs(dev_ramp))), 6),
            "mean_abs": round(float(np.mean(np.abs(dev_ramp))), 6),
            "samples": len(dev_ramp), "misses": miss_ramp,
        },
        "collision_deviation_platform_uu": {
            "max_abs": round(float(np.max(np.abs(dev_deck))), 6),
            "mean_abs": round(float(np.mean(np.abs(dev_deck))), 6),
            "samples": len(dev_deck), "misses": miss_deck,
        },
        "clearance_min_uu_collision": ("unbounded" if min(clr_ramp + clr_deck) > 1e8
                                       else round(min(clr_ramp + clr_deck), 2)),
        "clearance_min_uu_render": ("unbounded" if min(clr_rend) > 1e8
                                    else round(min(clr_rend), 2)),
        "clearance_budget_uu": 200.0,
        "unit_capsule_height_uu": 176.0,
        "solid_wedge": True,
    }
    return report["nav"]


# ============================================================================ export

def _flip_winding(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.reverse_faces(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()


def _ue_handedness_precomp(objs):
    mirror = Matrix.Diagonal((1.0, -1.0, 1.0, 1.0))
    for obj in objs:
        obj.data.transform(mirror)
        _flip_winding(obj.data)
        obj.data.update()


def export_fbx(objs, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    ensure_object_mode()
    _ue_handedness_precomp(objs)
    try:
        select_only(objs, active=objs[0])
        bpy.ops.export_scene.fbx(
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
    finally:
        _ue_handedness_precomp(objs)
    log(f"EXPORT {path} ({path.stat().st_size} bytes)")
    return path


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def measure(obj):
    mesh = obj.data
    xs = [v.co.x for v in mesh.vertices]
    ys = [v.co.y for v in mesh.vertices]
    zs = [v.co.z for v in mesh.vertices]
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    return {
        "dims_uu": [round((max(xs) - min(xs)) * UE, 3),
                    round((max(ys) - min(ys)) * UE, 3),
                    round((max(zs) - min(zs)) * UE, 3)],
        "bounds_min_uu": [round(min(xs) * UE, 3), round(min(ys) * UE, 3),
                          round(min(zs) * UE, 3)],
        "bounds_max_uu": [round(max(xs) * UE, 3), round(max(ys) * UE, 3),
                          round(max(zs) * UE, 3)],
        "min_z_uu": round(min(zs) * UE, 5),
        "tris": tris,
        "verts": len(mesh.vertices),
        "uv_layers": [l.name for l in mesh.uv_layers],
        "material_slots": [s.material.name if s.material else None
                           for s in obj.material_slots],
    }


def y_symmetry(obj, tol_uu=0.01):
    pts = np.array([[v.co.x, v.co.y, v.co.z] for v in obj.data.vertices]) * UE
    mirrored = pts.copy()
    mirrored[:, 1] *= -1.0
    worst = 0.0
    key = {(round(p[0], 3), round(p[1], 3), round(p[2], 3)) for p in pts}
    misses = 0
    for p in mirrored:
        if (round(p[0], 3), round(p[1], 3), round(p[2], 3)) not in key:
            d = np.min(np.linalg.norm(pts - p, axis=1))
            worst = max(worst, float(d))
            if d > tol_uu:
                misses += 1
    return {"y_symmetric": misses == 0, "worst_mismatch_uu": round(worst, 5),
            "misses": misses}


# ============================================================================ previews

def preview_env():
    world = bpy.data.worlds.new("W")
    bpy.context.scene.world = world
    world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (0.34, 0.42, 0.58, 1.0)
    bg.inputs["Strength"].default_value = 0.38
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    bpy.context.scene.collection.objects.link(sun)
    sun.data.energy = 3.4
    sun.data.color = (1.0, 0.93, 0.80)
    sun.data.angle = math.radians(2.0)
    sun.rotation_euler = (math.radians(50), 0.0, math.radians(150))
    # fill from the opposite quarter so the -Y embankment flank is not read as black
    fill = bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", "SUN"))
    bpy.context.scene.collection.objects.link(fill)
    fill.data.energy = 1.1
    fill.data.color = (0.78, 0.86, 1.0)
    fill.rotation_euler = (math.radians(62), 0.0, math.radians(-45))
    grass = new_mat("preview_grass")
    pg = principled(grass)
    pg.inputs["Base Color"].default_value = (0.055, 0.115, 0.028, 1.0)
    pg.inputs["Roughness"].default_value = 0.95
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=m(9000))
    ground = bm_to_object(bm, "preview_ground", [grass])
    ground.location = (m(1000), 0.0, -0.002)
    return [sun, fill, ground]


def make_human(name, at_uu):
    mat = bpy.data.materials.get("preview_human") or new_mat("preview_human")
    principled(mat).inputs["Base Color"].default_value = (0.62, 0.16, 0.14, 1.0)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=m(24), radius2=m(18),
                          depth=m(150), matrix=Matrix.Translation(Vector((0, 0, m(77)))))
    ret = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=m(15))
    bmesh.ops.translate(bm, verts=ret["verts"], vec=Vector((0, 0, m(167))))
    obj = bm_to_object(bm, name, [mat])
    obj.location = Vector([m(v) for v in at_uu])
    return obj


def render(path, cam_uu, look_uu, lens=42, res=(1100, 700), samples=40):
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    bpy.context.scene.collection.objects.link(cam)
    cam.location = Vector([m(v) for v in cam_uu])
    d = Vector([m(v) for v in look_uu]) - cam.location
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = lens
    scene = bpy.context.scene
    scene.camera = cam
    cycles_setup()
    scene.cycles.samples = samples
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.filepath = str(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    log(f"PREVIEW {path.name}")


def final_material(name, d_path, n_path, orm_path):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        if n.type not in ("BSDF_PRINCIPLED", "OUTPUT_MATERIAL"):
            nt.nodes.remove(n)
    p = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    d_img = bpy.data.images.load(str(d_path)); d_img.colorspace_settings.name = "sRGB"
    n_img = bpy.data.images.load(str(n_path)); n_img.colorspace_settings.name = "Non-Color"
    o_img = bpy.data.images.load(str(orm_path)); o_img.colorspace_settings.name = "Non-Color"
    dn = nt.nodes.new("ShaderNodeTexImage"); dn.image = d_img; dn.location = (-600, 300)
    on = nt.nodes.new("ShaderNodeTexImage"); on.image = o_img; on.location = (-600, 0)
    nn = nt.nodes.new("ShaderNodeTexImage"); nn.image = n_img; nn.location = (-600, -300)
    nt.links.new(dn.outputs["Color"], p.inputs["Base Color"])
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (-350, 0)
    nt.links.new(on.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Green"], p.inputs["Roughness"])
    nm = nt.nodes.new("ShaderNodeNormalMap"); nm.location = (-350, -300)
    nt.links.new(nn.outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], p.inputs["Normal"])
    p.inputs["Metallic"].default_value = 0.0
    return mat


# ============================================================================ team region

def team_face_mask(obj, post_x):
    """TeamRegion (slot 0) = merlons + kerb posts + the parapet coping top faces.
    Computed GEOMETRICALLY after the build, so it cannot drift from face order."""
    mask = []
    for poly in obj.data.polygons:
        c = poly.center * UE
        n = poly.normal
        team = False
        if c.z > MERLON_Z1 - 0.001 or (PARA_Z1 - 0.001 < c.z < MERLON_Z1 + 0.001):
            team = True                                   # merlon body + coping top
        elif abs(c.z - PARA_Z1) < 0.001 and n.z > 0.9:
            team = True
        else:
            for cx in post_x:                             # kerb posts
                base = ramp_z(cx) + KERB_H - POST_EMBED
                if (abs(c.x - cx) <= POST_S / 2 + 0.01
                        and abs(abs(c.y) - (YI + KERB_W / 2.0)) <= POST_S / 2 + 0.01
                        and base - 0.01 <= c.z <= base + POST_H + 0.01):
                    team = True
                    break
        mask.append(team)
    return mask


def face_area_uu2(obj, idx):
    return obj.data.polygons[idx].area * UE * UE


# ============================================================================ main

def main():
    wipe_scene()
    report = {"task": "TASK-727", "asset": ASSET, "node": NODE,
              "budget_source": "handoffs/TASK-725-programmer.md section 3"}

    log("build render mesh")
    obj, post_x = build_render_mesh()
    art_ashlar(obj.data.materials[0])
    art_timber(obj.data.materials[1])
    art_earth(obj.data.materials[2])
    art_coping(obj.data.materials[3])
    art_paving(obj.data.materials[4])

    log("uv atlas")
    uv_atlas(obj)

    log("bake D / N / R / AO @ %d" % BAKE_RES)
    d_img = new_image("WT_D_pre", BAKE_RES, "sRGB")
    n_img = new_image("WT_N", BAKE_RES, "Non-Color")
    r_img = new_image("WT_R", BAKE_RES, "Non-Color")
    ao_img = new_image("WT_AO", BAKE_RES, "Non-Color")
    bake_pass(obj, d_img, "DIFFUSE", SAMPLES, True, pass_filter={"COLOR"})
    bake_pass(obj, n_img, "NORMAL", SAMPLES, True)
    bake_pass(obj, r_img, "ROUGHNESS", SAMPLES, True)
    bake_ao(obj, ao_img, True)

    DEBUG.mkdir(parents=True, exist_ok=True)
    save_image(d_img, DEBUG / "T_WatchTower_D_predelight.png")
    save_image(ao_img, DEBUG / "T_WatchTower_AO.png")
    save_image(r_img, DEBUG / "T_WatchTower_R.png")

    final_d = apply_delight(d_img, ao_img, report)
    orm = pack_orm(ao_img, r_img)
    RAWTEX.mkdir(parents=True, exist_ok=True)
    d_path = RAWTEX / "T_WatchTower_D.png"
    n_path = RAWTEX / "T_WatchTower_N.png"
    orm_path = RAWTEX / "T_WatchTower_ORM.png"
    save_image(np_to_image("WT_D_final", final_d, "sRGB"), d_path)
    save_image(n_img, n_path)
    save_image(np_to_image("WT_ORM", orm, "Non-Color"), orm_path)
    gates = measure_gates(final_d, orm, report)
    log("gates: floor_pass=%s uv_norm=%.4f retention=%.3f"
        % (gates["albedo_floor_pass"], gates["uv_norm_covered_mean_linear"],
           gates["intent_retention"]))

    # ---- two-slot shipping contract ----------------------------------------
    log("swap to [TeamRegion, WatchTowerPBR]")
    mask = team_face_mask(obj, post_x)
    smooth = [p.use_smooth for p in obj.data.polygons]
    team_mat = new_mat("TeamRegion")
    tp = principled(team_mat)
    tp.inputs["Base Color"].default_value = (0.05, 0.30, 1.00, 1.0)   # MI_TeamColor_Blue
    tp.inputs["Roughness"].default_value = 0.55
    pbr_mat = final_material(f"{ASSET}PBR", d_path, n_path, orm_path)
    obj.data.materials.clear()                    # RESETS material_index -- reassign below
    obj.data.materials.append(team_mat)
    obj.data.materials.append(pbr_mat)
    team_area = other_area = 0.0
    for i, poly in enumerate(obj.data.polygons):
        poly.material_index = 0 if mask[i] else 1
        poly.use_smooth = smooth[i]
        a = poly.area * UE * UE
        if mask[i]:
            team_area += a
        else:
            other_area += a
    report["team_region"] = {
        "slot_order": ["TeamRegion", f"{ASSET}PBR"],
        "team_faces": int(sum(mask)),
        "total_faces": len(mask),
        "team_area_fraction": round(team_area / (team_area + other_area), 4),
        "max_fraction": 0.4,
        "selector": "merlons + kerb posts + parapet coping top faces",
    }

    # ---- collision ----------------------------------------------------------
    hulls = build_hulls()
    log("author %d UCX hulls" % len(hulls))
    hull_objs = [h[1] for h in hulls]
    report["collision"] = {
        "authoring": "HAND-PLACED exact convex primitives exported as UCX_ nodes; "
                     "NO convex decomposition anywhere in the chain",
        "hull_count": len(hulls),
        "render_nodes": [NODE],
        "hulls": [{"index": i, "name": h[1].name, "role": h[0],
                   "verts": len(h[1].data.vertices),
                   "bounds_uu": measure(h[1])["bounds_min_uu"] +
                                measure(h[1])["bounds_max_uu"]}
                  for i, h in enumerate(hulls)],
    }

    log("measure nav budget against the authored surfaces")
    nav = measure_nav(obj, hull_objs, report)
    log("nav: slope=%.4f deg lip=%.5f uu dev_ramp=%.6f uu clearance=%s"
        % (nav["measured_slope_collision_deg"], nav["junction_lip_uu_max"],
           nav["collision_deviation_ramp_uu"]["max_abs"],
           nav["clearance_min_uu_collision"]))

    report["mesh"] = measure(obj)
    report["mesh"]["tri_budget"] = 20000
    report["symmetry"] = y_symmetry(obj)

    # ---- previews (before the export mirror) --------------------------------
    log("previews")
    env = preview_env()
    humans = [make_human("h_toe", (RAMP_X1 - 260.0, 0.0, 0.0)),
              make_human("h_mid", (1400.0, 0.0, ramp_z(1400.0))),
              make_human("h_deck1", (-120.0, -120.0, RISE)),
              make_human("h_deck2", (60.0, 140.0, RISE))]
    for h in hull_objs:
        h.hide_render = True
    render(PREVIEWS / "watchtower_hero.png", (3450, -2600, 1500), (700, 0, 620))
    render(PREVIEWS / "watchtower_side_elevation.png", (1250, -5200, 700), (1250, 0, 640),
           lens=70)
    render(PREVIEWS / "watchtower_top.png", (900, 0, 4900), (900, 0, 0), lens=40)
    for h in humans:
        h.hide_render = True
    for h in hull_objs:
        h.hide_render = False
    obj.hide_render = True
    render(PREVIEWS / "watchtower_collision_hulls.png", (3450, -2600, 1500), (700, 0, 620))
    obj.hide_render = False

    for o in env + humans:
        bpy.data.objects.remove(o, do_unlink=True)

    # ---- export -------------------------------------------------------------
    fbx = RAW / f"{ASSET}.fbx"
    export_fbx([obj] + hull_objs, fbx)
    report["fbx"] = {"path": str(fbx.relative_to(ROOT)).replace("\\", "/"),
                     "bytes": fbx.stat().st_size, "sha256": sha256(fbx)}
    report["textures"] = {p.name: {"sha256": sha256(p), "bytes": p.stat().st_size}
                          for p in (d_path, n_path, orm_path)}
    report["exported_nodes"] = [obj.name] + [h.name for h in hull_objs]

    CACHE.mkdir(parents=True, exist_ok=True)
    with open(CACHE / "watchtower_report.json", "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
    log("REPORT %s" % (CACHE / "watchtower_report.json"))
    log("DONE")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        traceback.print_exc()
        sys.exit(1)
