"""build_watchtower.py -- SM_WatchTower.

RE-AUTHORED 2026-09-01 for TASK-737 [LADDER-1]: the ascent is a LEANING SIEGE
LADDER, not a walkable ramp. Jonathan's directive: "I like what you did with the
tower, but I was thinking instead of making it a ramp that you walk up it instead
has a ladder you climb up."

    "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background \
        --factory-startup --python-exit-code 1 \
        --python Tools/ArtPipeline/build_watchtower.py

WHAT SURVIVED THE REDESIGN, BYTE FOR BYTE (TOWER-8.0 scope fence)
----------------------------------------------------------------
    platform rise    1200 uu       PlatformHeightUU / the whole HIGH- contract
    deck             600 x 600     at z = 1200
    material         MI_WatchTower_PBR reused, NOT re-authored
    paths            Content/RawAssets/WatchTower.fbx -> /Game/Meshes/SM_WatchTower

WHAT WAS DELETED
----------------
    the 2078.461 uu / 30 deg ramp wedge and the 14 hulls tuned to it.

THE PINNED GEOMETRY (TOWER-8.3) -- BUILD TARGETS, NOT A RANGE TO INTERPRET
-------------------------------------------------------------------------
    body footprint   600 x 600, X/Y in [-300, +300], rising to the deck at z 1200
    LadderFoot       (-450, 0, 0)      86 uu clear of the body's eroded nav carve
    LadderTop        (-150, 0, 1200)   86 uu inside the deck's surviving nav poly
    climb line       ONE straight segment, length 1236.9 uu, lean 76.0 deg
    ladder width     >= 120 uu clear
    standoff         >= 56 uu between the capsule surface and the body face
    sockets          LadderFoot / LadderTop -- PascalCase, no prefix, no underscore

BOTH SOCKET COORDINATES ARE NAVMESH ARITHMETIC, NOT STYLE. Nothing in this file
re-derives them; they are literals and every measurement is taken AGAINST them.

THE SHAPE THE PINNED NUMBERS FORCE, AND WHY
-------------------------------------------
The climb line crosses the body's west face plane (x = -300) at z = 600, so for
the top HALF of the ascent the climb line is inside the body's plan footprint. A
solid 600x600x1200 block is therefore NOT buildable against the >= 56 uu standoff
(it holds only to z = 240). The resolution that keeps every pinned number is a
HOLLOW KEEP WITH AN OPEN WEST BAY: the ladder leans through the bay and rises
inside it. The ground carve stays a full solid 600x600 (the plinth), which is what
LadderFoot's 86 uu clearance is computed from.

    plinth      z [0, 160]      SOLID 600x600 -- this is the nav carve
    walls       z [160, 1160]   N / S / E + two west piers; west bay open |y| < 130
    fill wedge  45 deg          kills the unreachable interior nav island
    deck slab   z [1160, 1200]  600x600, its +Z face IS the walkable deck
    ladder      NO COLLISION    it is traversed by a nav link, never by pathing

COLLISION IS AUTHORED, NEVER DECOMPOSED
---------------------------------------
8 hand-placed UCX_SM_WatchTower_NN convex hulls, each an exact primitive. Hull 07's
+Z face IS z = 1200, so the walkable collision surface is the walkable render
surface -- zero deviation BY CONSTRUCTION, then measured by raycast. The ladder
gets ZERO hulls, deliberately: a hull at its foot would carve nav exactly where a
unit has to stand to start climbing.

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


# =========================================================== THE PINNED GEOMETRY (TOWER-8.3)
# LITERALS. Nothing below re-derives them; every measurement is taken against them.

RISE = 1200.0                   # platform / deck height  -- UNCHANGED by the redesign
DECK_HALF = 300.0               # 600 x 600 deck          -- UNCHANGED by the redesign
BODY_HALF = 300.0               # 600 x 600 body footprint, X/Y in [-300, +300]

LADDER_FOOT = (-450.0, 0.0, 0.0)
LADDER_TOP = (-150.0, 0.0, 1200.0)
SOCKETS = {"LadderFoot": LADDER_FOOT, "LadderTop": LADDER_TOP}

CLIMB_LEN_PINNED = 1236.9       # law's stated length
CLIMB_LEAN_PINNED = 76.0        # law's stated lean, degrees from horizontal
LADDER_CLEAR_W_MIN = 120.0      # clear width between the stiles
STANDOFF_MIN = 56.0             # capsule surface -> body face, along the whole line

CAPSULE_R = 34.0                # SiegeSpawnConstants.h:9 -- never InitCapsuleSize'd
CAPSULE_HALF_H = 88.0           # => a 176 uu tall unit; the nav agent is only 144

# derived from the two sockets ONLY (never typed twice)
_D = (LADDER_TOP[0] - LADDER_FOOT[0], LADDER_TOP[1] - LADDER_FOOT[1],
      LADDER_TOP[2] - LADDER_FOOT[2])
CLIMB_LEN = math.sqrt(_D[0] ** 2 + _D[1] ** 2 + _D[2] ** 2)          # 1236.93169
CLIMB_LEAN = math.degrees(math.atan2(_D[2], math.hypot(_D[0], _D[1])))  # 75.96376
U_AXIS = Vector((_D[0] / CLIMB_LEN, _D[1] / CLIMB_LEN, _D[2] / CLIMB_LEN))
# in-plane perpendicular, pointing AWAY from the tower (west-and-up)
W_AXIS = Vector((-U_AXIS.z, 0.0, U_AXIS.x))
V_AXIS = Vector((0.0, 1.0, 0.0))


def climb_point(t):
    """A point on the pinned climb line, t uu from LadderFoot."""
    return Vector(LADDER_FOOT) + U_AXIS * t


# ------------------------------------------------------------------ the massing
PLINTH_Z1 = 160.0               # solid 600x600 to here -- THE nav carve
SHAFT_HALF = 276.0              # walls inset 24 uu so the deck reads as a cap
WALL_T = 80.0
INNER_HALF = SHAFT_HALF - WALL_T                       # 196
BAY_HALF_Y = 130.0              # west bay opening: |y| < 130  => 260 uu wide
WALL_Z0, WALL_Z1 = PLINTH_Z1, 1160.0

DECK_Z0, DECK_Z1 = 1160.0, RISE                        # 40 uu slab, top IS the deck
TEAM_RING_W = 75.0                                     # deck-top team band
RING_IN = DECK_HALF - TEAM_RING_W                      # 225

CORBEL_Z0, CORBEL_Z1 = 1090.0, 1160.0                  # render-only bracket band
COURSE_Z0, COURSE_Z1 = 620.0, 660.0                    # render-only string course
COURSE2_Z0, COURSE2_Z1 = 880.0, 915.0                  # second, upper course
COURSE_OUT = 292.0
SLIT_TIERS = ((330.0, 560.0), (700.0, 855.0))          # window-slit bands
SLIT_OFF = 120.0                                       # +/- offset along the wall
SLIT_HW = 24.0
SLIT_PROUD = 5.0

FILL_SLOPE = 1.0                # 45 deg: > the 32.005 deg ceiling => never navigable


def fill_top_z(x):
    """The interior fill's 45 deg top plane. Exists ONLY to deny Recast a flat,
    unreachable island on the plinth top inside the shaft."""
    return PLINTH_Z1 + (x + SHAFT_HALF) * FILL_SLOPE


# ------------------------------------------------------------------ the ladder
STILE_CY = 76.0                 # stile centre |y|
STILE_HY = 10.0                 # => inner faces at |y| = 66, clear width 132
STILE_HW = 10.0                 # half-thickness along W_AXIS
LADDER_OFF = 22.0               # ladder plane sits this far TOWARDS the tower from
                                # the climb line, so the climber is outboard of it
RUNG_PITCH = 40.0
RUNG_HU = 9.0
RUNG_HW = 9.0
# Solved, not guessed: the stile's lowest corner lands EXACTLY on z = 0, so the
# asset keeps min_z == 0 (the manifest's origin contract) and the ladder still
# reads as planted rather than floating.
STILE_T0 = (LADDER_OFF + STILE_HW) * W_AXIS.z / U_AXIS.z
STILE_T1 = CLIMB_LEN + 115.0    # head projects above the deck

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
BAKE_RES = 2048
MARGIN_PX = 16
SAMPLES = 8
AO_SAMPLES = 64
AO_DISTANCE_M = 2.5

RNG = np.random.RandomState(737)


def inv_delight(post):
    return tuple(min(1.0, (c / DELIGHT["gain"]) ** (1.0 / DELIGHT["gamma"])) for c in post)


# intended POST-delight linear palettes -- IDENTICAL to the ramp build (TASK-727).
# The silhouette changed; the palette deliberately did not.
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
INTENT_WEIGHTS = {
    "ashlar_light": 0.19, "ashlar_dark": 0.19, "mortar": 0.05,
    "timber_light": 0.10, "timber_dark": 0.06,
    "earth_light": 0.09, "earth_dark": 0.06,
    "coping_light": 0.07, "coping_dark": 0.05,
    "paving_light": 0.09, "paving_dark": 0.05,
}


def log(msg):
    print(f"[T737] {msg}", flush=True)


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
    """One CLOSED solid -> one object. Verts weld WITHIN the solid only."""
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


def fill_faces(fam):
    """THE INTERIOR FILL: a triangular prism on the plinth top, top plane at 45 deg.
    Its ONLY job is to deny Recast a flat unreachable island inside the shaft --
    45 deg is 1.4x over the 32.005 deg ledge-filter ceiling (TOWER-2a), so no nav
    span survives on it. It is nowhere near the climb line (measured in the report)."""
    x0, x1 = -SHAFT_HALF, INNER_HALF
    y0, y1 = -INNER_HALF, INNER_HALF
    z0 = PLINTH_Z1
    z1 = fill_top_z(x1)
    return [
        ([(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)], fam),      # bottom
        ([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], fam),      # east
        ([(x0, y0, z0), (x0, y1, z0), (x1, y1, z1), (x1, y0, z1)], fam),      # slope
        ([(x0, y0, z0), (x1, y0, z1), (x1, y0, z0)], fam),                    # -Y end
        ([(x0, y1, z0), (x1, y1, z0), (x1, y1, z1)], fam),                    # +Y end
    ]


def decoration_boxes():
    """The RENDER-ONLY dressing (name, extent, material family). Listed once, at
    module scope, so the standoff sweep measures the SAME boxes the render mesh is
    built from -- a decoration measured from a second copy of its numbers is a
    decoration that can silently drift into the climb corridor.

    TWO THINGS WERE TRIED HERE, RENDERED, AND REJECTED ON THE PICTURE:
      * a team-coloured corbel band -> a blue slab hanging under the deck;
      * full-height corner quoins   -> they framed each 552 x 930 wall into one
        recessed panel and the side elevation read as a FRIDGE DOOR.
    What replaces them is horizontal: two string courses and a rhythm of window
    slits, which is what actually gives a 600 x 1200 shaft scale."""
    S, B, C, D = SHAFT_HALF, BAY_HALF_Y, COURSE_OUT, DECK_HALF
    out = []
    # string courses -- GAPPED at the west bay: a course drawn across the opening
    # would sit exactly where the climbing capsule passes.
    for tag, (z0, z1) in (("lo", (COURSE_Z0, COURSE_Z1)), ("hi", (COURSE2_Z0, COURSE2_Z1))):
        out += [
            (f"course_{tag}_N", (-C, C, S, C, z0, z1), FAM_COPING),
            (f"course_{tag}_S", (-C, C, -C, -S, z0, z1), FAM_COPING),
            (f"course_{tag}_E", (S, C, -S, S, z0, z1), FAM_COPING),
            (f"course_{tag}_Wp", (-C, -S, B, S, z0, z1), FAM_COPING),
            (f"course_{tag}_Wn", (-C, -S, -S, -B, z0, z1), FAM_COPING),
        ]
    # corbel band under the deck. GAPPED at the bay for the same reason, and
    # MEASURED rather than eyeballed: a continuous west band came to 48.5 uu of the
    # climbing capsule -- clear air, but under the pinned 56, and a decoration is
    # not where you spend a standoff budget.
    out += [
        ("corbel_N", (-D, D, S, D, CORBEL_Z0, CORBEL_Z1), FAM_COPING),
        ("corbel_S", (-D, D, -D, -S, CORBEL_Z0, CORBEL_Z1), FAM_COPING),
        ("corbel_E", (S, D, -S, S, CORBEL_Z0, CORBEL_Z1), FAM_COPING),
        ("corbel_Wp", (-D, -S, B, S, CORBEL_Z0, CORBEL_Z1), FAM_COPING),
        ("corbel_Wn", (-D, -S, -S, -B, CORBEL_Z0, CORBEL_Z1), FAM_COPING),
    ]
    # window slits -- the "watch tower" cue, on the three CLOSED faces only. Never
    # on the west face: that is the bay the ladder rises through.
    for tier, (z0, z1) in enumerate(SLIT_TIERS):
        for c in (-SLIT_OFF, SLIT_OFF):
            out += [
                (f"slit_N{tier}{'p' if c > 0 else 'n'}",
                 (c - SLIT_HW, c + SLIT_HW, S, S + SLIT_PROUD, z0, z1), FAM_TIMBER),
                (f"slit_S{tier}{'p' if c > 0 else 'n'}",
                 (c - SLIT_HW, c + SLIT_HW, -S - SLIT_PROUD, -S, z0, z1), FAM_TIMBER),
                (f"slit_E{tier}{'p' if c > 0 else 'n'}",
                 (S, S + SLIT_PROUD, c - SLIT_HW, c + SLIT_HW, z0, z1), FAM_TIMBER),
            ]
    return out


def obox(centre, hu, hv, hw, fam):
    """A box oriented to the climb line: hu along U_AXIS, hv along Y, hw along W_AXIS."""
    pts = {}
    for su in (-1, 1):
        for sv in (-1, 1):
            for sw in (-1, 1):
                p = centre + U_AXIS * (su * hu) + V_AXIS * (sv * hv) + W_AXIS * (sw * hw)
                pts[(su, sv, sw)] = (p.x, p.y, p.z)
    q = [
        [(-1, -1, -1), (-1, 1, -1), (-1, 1, 1), (-1, -1, 1)],
        [(1, -1, -1), (1, -1, 1), (1, 1, 1), (1, 1, -1)],
        [(-1, -1, -1), (-1, -1, 1), (1, -1, 1), (1, -1, -1)],
        [(-1, 1, -1), (1, 1, -1), (1, 1, 1), (-1, 1, 1)],
        [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1)],
        [(-1, -1, 1), (-1, 1, 1), (1, 1, 1), (1, -1, 1)],
    ]
    return [([pts[k] for k in face], fam) for face in q]


def ladder_parts():
    """The ladder: 2 stiles + N rungs on the pinned climb line. RENDER ONLY --
    it gets ZERO collision hulls, deliberately (see build_hulls)."""
    out = []
    mid_t = 0.5 * (STILE_T0 + STILE_T1)
    hu = 0.5 * (STILE_T1 - STILE_T0)
    for sgn, tag in ((1.0, "p"), (-1.0, "n")):
        c = climb_point(mid_t) - W_AXIS * LADDER_OFF + V_AXIS * (sgn * STILE_CY)
        out.append((f"p_stile_{tag}", obox(c, hu, STILE_HY, STILE_HW, FAM_TIMBER)))
    n = 0
    t = 70.0
    while t <= CLIMB_LEN - 15.0:
        c = climb_point(t) - W_AXIS * LADDER_OFF
        out.append((f"p_rung_{n:02d}", obox(c, RUNG_HU, STILE_CY + STILE_HY,
                                            RUNG_HW, FAM_TIMBER)))
        n += 1
        t += RUNG_PITCH
    return out, n


# ============================================================================ render mesh

def build_render_mesh():
    mats = [new_mat("art_ashlar"), new_mat("art_timber"), new_mat("art_earth"),
            new_mat("art_coping"), new_mat("art_paving")]
    parts = []

    def solid(name, faces):
        parts.append(make_solid(name, faces, mats))

    S, I, B = SHAFT_HALF, INNER_HALF, BAY_HALF_Y

    # ---- plinth: SOLID 600x600. THIS is the ground nav carve LadderFoot clears ----
    solid("p_plinth", box_faces(-BODY_HALF, BODY_HALF, -BODY_HALF, BODY_HALF,
                                0.0, PLINTH_Z1, FAM_ASHLAR))

    # ---- keep walls; the WEST BAY (|y| < 130) is OPEN so the ladder can rise in it --
    solid("p_wall_N", box_faces(-S, S, I, S, WALL_Z0, WALL_Z1, FAM_ASHLAR))
    solid("p_wall_S", box_faces(-S, S, -S, -I, WALL_Z0, WALL_Z1, FAM_ASHLAR))
    solid("p_wall_E", box_faces(I, S, -I, I, WALL_Z0, WALL_Z1, FAM_ASHLAR))
    solid("p_pier_Wp", box_faces(-S, -I, B, I, WALL_Z0, WALL_Z1, FAM_ASHLAR))
    solid("p_pier_Wn", box_faces(-S, -I, -I, -B, WALL_Z0, WALL_Z1, FAM_ASHLAR))

    # ---- interior fill (45 deg) -- denies Recast an unreachable island ------------
    solid("p_fill", fill_faces(FAM_EARTH))

    # ---- render-only dressing, from THE SAME list the standoff sweep measures ----
    for name, ext, fam in decoration_boxes():
        solid(f"p_{name}", box_faces(*ext, fam))

    # ---- the deck. Built as a TEAM RING + a centre panel, both topping out at
    #      EXACTLY z = 1200, so the walkable surface is one unbroken plane and the
    #      team colour is a flush inlay -- never a parapet carved out of the deck
    #      (TOWER-2a: a rail cut from the deck seeds erosion from its own face).
    R = RING_IN
    solid("p_deck_ring_N", box_faces(-DECK_HALF, DECK_HALF, R, DECK_HALF,
                                     DECK_Z0, DECK_Z1, FAM_PAVING))
    solid("p_deck_ring_S", box_faces(-DECK_HALF, DECK_HALF, -DECK_HALF, -R,
                                     DECK_Z0, DECK_Z1, FAM_PAVING))
    solid("p_deck_ring_W", box_faces(-DECK_HALF, -R, -R, R, DECK_Z0, DECK_Z1, FAM_PAVING))
    solid("p_deck_ring_E", box_faces(R, DECK_HALF, -R, R, DECK_Z0, DECK_Z1, FAM_PAVING))
    solid("p_deck_mid", box_faces(-R, R, -R, R, DECK_Z0, DECK_Z1, FAM_PAVING))

    # ---- the ladder ---------------------------------------------------------------
    lad, rungs = ladder_parts()
    for name, faces in lad:
        solid(name, faces)

    ensure_object_mode()
    select_only(parts, active=parts[0])
    bpy.ops.object.join()
    obj = parts[0]
    obj.name = NODE
    obj.data.name = NODE
    for poly in obj.data.polygons:
        poly.use_smooth = False                  # FACE smoothing -- crisp masonry
    return obj, rungs


# ============================================================ collision (8 exact hulls)

HULL_SPEC = [
    ("plinth -- THE 600x600 ground nav carve; LadderFoot clears its erosion by 86 uu",
     lambda: box_faces(-BODY_HALF, BODY_HALF, -BODY_HALF, BODY_HALF, 0.0, PLINTH_Z1, 0)),
    ("keep wall +Y",
     lambda: box_faces(-SHAFT_HALF, SHAFT_HALF, INNER_HALF, SHAFT_HALF,
                       WALL_Z0, WALL_Z1, 0)),
    ("keep wall -Y",
     lambda: box_faces(-SHAFT_HALF, SHAFT_HALF, -SHAFT_HALF, -INNER_HALF,
                       WALL_Z0, WALL_Z1, 0)),
    ("keep wall +X (east)",
     lambda: box_faces(INNER_HALF, SHAFT_HALF, -INNER_HALF, INNER_HALF,
                       WALL_Z0, WALL_Z1, 0)),
    ("west bay pier +Y",
     lambda: box_faces(-SHAFT_HALF, -INNER_HALF, BAY_HALF_Y, INNER_HALF,
                       WALL_Z0, WALL_Z1, 0)),
    ("west bay pier -Y",
     lambda: box_faces(-SHAFT_HALF, -INNER_HALF, -INNER_HALF, -BAY_HALF_Y,
                       WALL_Z0, WALL_Z1, 0)),
    ("interior fill wedge (45 deg -- unnavigable by construction)",
     lambda: fill_faces(0)),
    ("deck slab -- ITS +Z FACE IS THE WALKABLE DECK AT z = 1200",
     lambda: box_faces(-DECK_HALF, DECK_HALF, -DECK_HALF, DECK_HALF,
                       DECK_Z0, DECK_Z1, 0)),
]


def build_hulls():
    """8 hand-placed convex hulls, each an EXACT primitive. NOTHING here is a convex
    decomposition. Hull 07's +Z face IS z = 1200, so the walkable collision surface
    is the walkable render surface.

    THE LADDER GETS ZERO HULLS, AND THAT IS THE POINT:
      * it must NOT be a walkable surface -- it is traversed by a nav link, and at
        76 deg Recast would shred it anyway;
      * a hull at its foot would carve ground nav at exactly (-450, 0, 0), which is
        where a unit has to STAND to start the climb. A floating or badly hulled
        ladder base punches a nav hole in the one cell the feature needs."""
    hulls = []
    for i, (role, fn) in enumerate(HULL_SPEC):
        hulls.append((role, make_solid(f"UCX_{NODE}_{i:02d}", fn(), [])))
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
    """Blender's Brick node lays ROWS along Y, so raw object coords make every
    vertical wall a set of VERTICAL STRIPES. Re-map to (x + y, z) so rows advance
    with HEIGHT on any vertical face. (TASK-727 run-1 defect, kept fixed.)"""
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
    wr = ramp2(mat, n.outputs["Fac"], (0.72, 0.72, 0.72), (1.06, 1.06, 1.06), (-620, -220))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-360, 60))
    mul.operation = "MULTIPLY"
    link(mat, br.outputs["Color"], mul.inputs[0])
    link(mat, wr.outputs["Color"], mul.inputs[1])
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
    """THE LADDER. Grain runs along the stiles, and the per-plank tone jitter gives
    each rung its own value so the rungs read individually at RTS distance."""
    vec = coord(mat, 1.0)
    w = add_node(mat, "ShaderNodeTexWave", location=(-900, 220))
    w.wave_type = "BANDS"
    w.bands_direction = "Z"
    w.wave_profile = "SAW"
    w.inputs["Scale"].default_value = 1.30
    w.inputs["Distortion"].default_value = 0.6
    w.inputs["Detail"].default_value = 2.0
    w.inputs["Detail Scale"].default_value = 0.8
    link(mat, vec, w.inputs["Vector"])
    jn = add_node(mat, "ShaderNodeTexNoise", location=(-900, -60))
    jn.inputs["Scale"].default_value = 2.6
    jn.inputs["Detail"].default_value = 1.0
    link(mat, vec, jn.inputs["Vector"])
    tone = ramp2(mat, jn.outputs["Fac"], PRE["timber_dark"], PRE["timber_light"],
                 (-640, -60))
    gn = add_node(mat, "ShaderNodeTexNoise", location=(-900, -340))
    gn.inputs["Scale"].default_value = 26.0
    gn.inputs["Detail"].default_value = 5.0
    link(mat, vec, gn.inputs["Vector"])
    gr = ramp2(mat, gn.outputs["Fac"], (0.78, 0.78, 0.78), (1.08, 1.08, 1.08), (-640, -340))
    mul = add_node(mat, "ShaderNodeVectorMath", location=(-380, 40))
    mul.operation = "MULTIPLY"
    link(mat, tone.outputs["Color"], mul.inputs[0])
    link(mat, gr.outputs["Color"], mul.inputs[1])
    gv = ramp2(mat, w.outputs["Fac"], (0.60, 0.60, 0.60), (1.0, 1.0, 1.0), (-640, 220))
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
    """The interior fill seen through the west bay -- rammed-earth-and-rubble."""
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
    """THE DECK -- a walkable surface, so a true (x, y) flagstone read."""
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


class ConvexProbe:
    """EXACT signed distance to one convex solid, in uu.

    The first cut of this used max-of-planes, which is exact inside but a
    CONSERVATIVE under-estimate outside near edges and corners -- it reported the
    plinth at 43 uu when the true clearance is ~75. A standoff gate that
    under-reports fails safe, but it also fails HONESTLY only if you say so, and a
    number nobody can trust is not a measurement. So: BVH nearest-surface distance
    for the magnitude (exact), max-of-planes ONLY for the inside/outside sign."""

    def __init__(self, name, obj):
        self.name = name
        verts = [tuple(v.co) for v in obj.data.vertices]
        polys = [tuple(p.vertices) for p in obj.data.polygons]
        self.bvh = BVHTree.FromPolygons(verts, polys, all_triangles=False, epsilon=0.0)
        self.planes = []
        for poly in obj.data.polygons:
            n = poly.normal.normalized()
            p0 = obj.data.vertices[poly.vertices[0]].co * UE
            self.planes.append((np.array([n.x, n.y, n.z]),
                                float(n.x * p0.x + n.y * p0.y + n.z * p0.z)))

    def inside(self, p):
        return max(float(np.dot(n, p) - d) for n, d in self.planes) <= 0.0

    def sdf(self, p):
        hit = self.bvh.find_nearest(Vector((m(p[0]), m(p[1]), m(p[2]))))
        dist = 1e9 if hit[0] is None else hit[3] * UE
        return -dist if self.inside(p) else dist


def make_probe(name, faces):
    """A throwaway solid used only for measurement, removed straight after."""
    obj = make_solid(f"__probe_{name}", faces, [])
    probe = ConvexProbe(name, obj)
    bpy.data.objects.remove(obj, do_unlink=True)
    return probe


def measure_ladder_contract(hulls, hull_objs, render_obj, report):
    """EVERY number here is measured AGAINST the pinned literals, never derived
    from a preference. The pinned coordinates are inputs to this function."""
    hull_bvh = bvh_of(hull_objs)
    rend_bvh = bvh_of([render_obj])
    probes = [(role, ConvexProbe(obj.name, obj), True) for role, obj in hulls]
    probes += [(f"render-only dressing: {n}", make_probe(n, box_faces(*ext, 0)), False)
               for n, ext, _fam in decoration_boxes()]

    # ---- 1. the deck: is its collision top EXACTLY z = 1200, everywhere? --------
    dev_deck, miss_deck, clr_deck = [], 0, []
    gx = np.linspace(-DECK_HALF + 2.0, DECK_HALF - 2.0, 60)
    for x in gx:
        for y in gx:
            z = top_hit(hull_bvh, x, y)
            if z is None:
                miss_deck += 1
                continue
            dev_deck.append(z - RISE)
            c = up_clearance(hull_bvh, x, y, z)
            clr_deck.append(1e9 if c is None else c)

    # the surviving nav polygon after ledge-nulling + erosion (64 uu per side)
    nav_half = DECK_HALF - 64.0
    dev_nav = []
    nx = np.linspace(-nav_half + 1.0, nav_half - 1.0, 40)
    for x in nx:
        for y in nx:
            z = top_hit(hull_bvh, x, y)
            dev_nav.append(1e9 if z is None else z - RISE)

    # ---- 2. the two sockets, checked against what they are FOR ------------------
    ground_nav_x = -BODY_HALF - 64.0                    # carve + 2 cells of erosion
    foot_margin = ground_nav_x - LADDER_FOOT[0]         # must be > 0
    top_margin = nav_half - abs(LADDER_TOP[0])          # must be > 0
    deck_z_under_top = top_hit(hull_bvh, LADDER_TOP[0], LADDER_TOP[1])

    # is there ANY collision within a unit's footprint of LadderFoot? there must not be
    foot_block = []
    for ang in np.linspace(0.0, 2.0 * math.pi, 24, endpoint=False):
        for r in (0.0, 17.0, 34.0):
            px = LADDER_FOOT[0] + r * math.cos(ang)
            py = LADDER_FOOT[1] + r * math.sin(ang)
            for pz in (2.0, 44.0, 88.0, 132.0, 174.0):
                p = np.array([px, py, pz])
                foot_block.append(min(pr.sdf(p) for _, pr, is_hull in probes if is_hull))
    foot_ground_z = top_hit(hull_bvh, LADDER_FOOT[0], LADDER_FOOT[1], from_z_uu=600.0)

    # ---- 3. THE STANDOFF: capsule vs every hull, along the WHOLE pinned line ----
    # two capsule models, because the law does not say which point of the unit the
    # line describes. Reported per hull so nothing is averaged away.
    per_solid = {}
    ts = np.linspace(0.0, CLIMB_LEN, 420)
    dzs = np.linspace(-(CAPSULE_HALF_H - CAPSULE_R), (CAPSULE_HALF_H - CAPSULE_R), 9)
    for role, probe, is_hull in probes:
        worst_sphere, worst_caps, at_t = 1e9, 1e9, None
        for t in ts:
            P = climb_point(float(t))
            base = np.array([P.x, P.y, P.z])
            worst_sphere = min(worst_sphere, probe.sdf(base) - CAPSULE_R)
            d_c = min(probe.sdf(base + np.array([0.0, 0.0, dz])) for dz in dzs) - CAPSULE_R
            if d_c < worst_caps:
                worst_caps, at_t = d_c, float(t)
        per_solid[probe.name] = {
            "role": role,
            "collides": is_hull,
            "min_clearance_sphere_uu": round(worst_sphere, 3),
            "min_clearance_capsule_uu": round(worst_caps, 3),
            "at_t_uu": round(at_t, 1),
            "pass_56": bool(worst_caps >= STANDOFF_MIN),
        }

    # the same sweep, restricted to everything EXCEPT the deck slab
    deck_name = f"UCX_{NODE}_{len(HULL_SPEC) - 1:02d}"
    hull_names = [p.name for _, p, is_hull in probes if is_hull]
    body_only = {k: v for k, v in per_solid.items() if k in hull_names and k != deck_name}
    body_min = min(v["min_clearance_capsule_uu"] for v in body_only.values())
    body_min_sphere = min(v["min_clearance_sphere_uu"] for v in body_only.values())
    body_min_at = min(body_only.items(), key=lambda kv: kv[1]["min_clearance_capsule_uu"])
    deco = {k: v for k, v in per_solid.items() if k not in hull_names}
    deco_min = min(v["min_clearance_capsule_uu"] for v in deco.values())

    # how much of the climb line is inside the deck slab (the declared consequence)
    deck_probe = next(p for _, p, _ in probes if p.name == deck_name)
    inside = [float(t) for t in np.linspace(0.0, CLIMB_LEN, 3000)
              if deck_probe.sdf(np.array(list(climb_point(float(t))))) < 0.0]
    line_in_deck = (max(inside) - min(inside)) if inside else 0.0

    # ---- 4. the ladder as built vs the pinned line -----------------------------
    stile_inner = 2.0 * (STILE_CY - STILE_HY)
    stile_outer = 2.0 * (STILE_CY + STILE_HY)

    # ---- 5. clearance ABOVE the deck (collision AND render) ---------------------
    clr_rend, head_xs, head_ys = [], [], []
    for x in np.linspace(-DECK_HALF + 4.0, DECK_HALF - 4.0, 121):
        for y in np.linspace(-DECK_HALF + 4.0, DECK_HALF - 4.0, 121):
            z = top_hit(rend_bvh, x, y)
            if z is None or z < RISE - 1.0:
                continue
            c = up_clearance(rend_bvh, x, y, RISE)
            clr_rend.append(1e9 if c is None else c)
            if c is not None:                     # the ladder head, the ONLY such thing
                head_xs.append(x)
                head_ys.append(y)
    # how close does the ladder head come to a unit STANDING on LadderTop?
    head_gap = 1e9
    if head_xs:
        for hx, hy in zip(head_xs, head_ys):
            head_gap = min(head_gap, math.hypot(hx - LADDER_TOP[0], hy - LADDER_TOP[1]))
        head_gap -= CAPSULE_R

    # ---- 6. body footprint, measured from the COLLISION at ground --------------
    ground_extent = [0.0, 0.0]
    for hull_role, hull_obj in hulls:
        zs = [v.co.z * UE for v in hull_obj.data.vertices]
        if min(zs) > 1.0:
            continue
        ground_extent[0] = max(ground_extent[0],
                               max(abs(v.co.x) * UE for v in hull_obj.data.vertices))
        ground_extent[1] = max(ground_extent[1],
                               max(abs(v.co.y) * UE for v in hull_obj.data.vertices))

    report["ladder_contract"] = {
        "pinned": {
            "rise_uu": RISE,
            "deck_uu": [2 * DECK_HALF, 2 * DECK_HALF],
            "body_footprint_uu": [2 * BODY_HALF, 2 * BODY_HALF],
            "LadderFoot": list(LADDER_FOOT),
            "LadderTop": list(LADDER_TOP),
            "climb_len_uu": CLIMB_LEN_PINNED,
            "climb_lean_deg": CLIMB_LEAN_PINNED,
            "ladder_clear_width_min_uu": LADDER_CLEAR_W_MIN,
            "standoff_min_uu": STANDOFF_MIN,
        },
        "as_built": {
            "climb_len_uu": round(CLIMB_LEN, 4),
            "climb_lean_deg": round(CLIMB_LEAN, 4),
            "deck_top_z_at_LadderTop_uu": (None if deck_z_under_top is None
                                           else round(deck_z_under_top, 5)),
            "deck_collision_dev_from_1200_uu": {
                "max_abs": round(float(np.max(np.abs(dev_deck))), 6),
                "mean_abs": round(float(np.mean(np.abs(dev_deck))), 6),
                "samples": len(dev_deck), "misses": miss_deck,
            },
            "deck_nav_poly_dev_from_1200_uu_max": round(float(np.max(np.abs(dev_nav))), 6),
            "ladder_clear_width_uu": stile_inner,
            "ladder_overall_width_uu": stile_outer,
            "body_ground_collision_half_extent_uu": [round(ground_extent[0], 3),
                                                     round(ground_extent[1], 3)],
            "clearance_above_deck_collision": ("unbounded" if min(clr_deck) > 1e8
                                               else round(min(clr_deck), 2)),
            "clearance_above_deck_render_min_uu": ("unbounded" if not clr_rend or
                                                   min(clr_rend) > 1e8
                                                   else round(min(clr_rend), 2)),
            "render_geometry_above_deck": {
                "what": "the ladder head ONLY -- render-only, ZERO collision, so it "
                        "cannot enter voxelization and cannot cost a nav cell",
                "plan_extent_x_uu": ([round(min(head_xs), 1), round(max(head_xs), 1)]
                                     if head_xs else None),
                "plan_extent_y_uu": ([round(min(head_ys), 1), round(max(head_ys), 1)]
                                     if head_ys else None),
                "gap_to_capsule_standing_on_LadderTop_uu": (None if head_gap > 1e8
                                                            else round(head_gap, 2)),
            },
        },
        "socket_arithmetic": {
            "ground_nav_starts_at_x_uu": ground_nav_x,
            "LadderFoot_clears_ground_carve_by_uu": round(foot_margin, 1),
            "deck_nav_poly_x_uu": [-nav_half, nav_half],
            "LadderTop_inside_deck_nav_poly_by_uu": round(top_margin, 1),
            "min_collision_sdf_around_LadderFoot_uu": round(min(foot_block), 3),
            "collision_hit_below_LadderFoot": foot_ground_z,
            "LadderFoot_is_standable": bool(min(foot_block) > 0.0 and foot_ground_z is None),
        },
        "standoff": {
            "model": "EXACT signed distance (BVH nearest surface + convex inside "
                     "test). Two unit models, because the law does not say which "
                     "point of the unit the line describes: a sphere r=34 on the "
                     "line, and a vertical capsule r=34 h=176 centred on it.",
            "min_clearance_body_only_capsule_uu": round(body_min, 3),
            "min_clearance_body_only_sphere_uu": round(body_min_sphere, 3),
            "min_clearance_body_only_at": body_min_at[0],
            "body_only_pass_56": bool(body_min >= STANDOFF_MIN),
            "min_clearance_render_dressing_capsule_uu": round(deco_min, 3),
            "render_dressing_pass_56": bool(deco_min >= STANDOFF_MIN),
            "per_solid": per_solid,
            "deck_slab_hull": deck_name,
            "climb_line_length_inside_deck_slab_uu": round(line_in_deck, 3),
            "climb_line_fraction_inside_deck_slab": round(line_in_deck / CLIMB_LEN, 5),
            "deck_slab_note": "DECLARED, NOT SILENTLY FIXED. LadderTop is pinned 150 "
                              "uu inside a solid 600x600 deck and is approached from "
                              "below at 76 deg, so the final stretch of ANY climb "
                              "line ends inside ANY solid deck. No mesh can remove "
                              "this without moving a pinned coordinate.",
        },
    }
    return report["ladder_contract"]


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


def make_sockets(parent):
    """UE builds a static-mesh socket from any FBX node named SOCKET_<Name> that is
    a child of the mesh node, and STRIPS the prefix -- so the shipped socket names
    are LadderFoot / LadderTop, per the "Static-mesh SOCKET names" clause.

    BOTH sockets sit on y = 0, which is why the TASK-348 handedness mirror (a Y
    negation) cannot move them: -0 == 0. Their x and z are untouched by it."""
    out = []
    for name, (x, y, z) in SOCKETS.items():
        e = bpy.data.objects.new(f"SOCKET_{name}", None)
        e.empty_display_type = "PLAIN_AXES"
        e.empty_display_size = 0.4
        bpy.context.scene.collection.objects.link(e)
        e.parent = parent
        e.matrix_parent_inverse = Matrix.Identity(4)
        e.location = (m(x), m(y), m(z))
        e.rotation_euler = (0.0, 0.0, 0.0)
        e.scale = (1.0, 1.0, 1.0)
        out.append(e)
    return out


def export_fbx(mesh_objs, empties, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    ensure_object_mode()
    _ue_handedness_precomp(mesh_objs)
    try:
        select_only(mesh_objs + empties, active=mesh_objs[0])
        bpy.ops.export_scene.fbx(
            filepath=str(path),
            use_selection=True,
            object_types={"MESH", "EMPTY"},
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
        _ue_handedness_precomp(mesh_objs)
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
    sun.rotation_euler = (math.radians(52), 0.0, math.radians(205))
    fill = bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", "SUN"))
    bpy.context.scene.collection.objects.link(fill)
    fill.data.energy = 1.2
    fill.data.color = (0.78, 0.86, 1.0)
    fill.rotation_euler = (math.radians(62), 0.0, math.radians(-40))
    grass = new_mat("preview_grass")
    pg = principled(grass)
    pg.inputs["Base Color"].default_value = (0.055, 0.115, 0.028, 1.0)
    pg.inputs["Roughness"].default_value = 0.95
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=m(9000))
    ground = bm_to_object(bm, "preview_ground", [grass])
    ground.location = (0.0, 0.0, -0.002)
    return [sun, fill, ground]


def make_human(name, at_uu, colour=(0.62, 0.16, 0.14, 1.0)):
    mat = bpy.data.materials.get(f"preview_human_{name}") or new_mat(f"preview_human_{name}")
    principled(mat).inputs["Base Color"].default_value = colour
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=m(24), radius2=m(18),
                          depth=m(150), matrix=Matrix.Translation(Vector((0, 0, m(77)))))
    ret = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=m(15))
    bmesh.ops.translate(bm, verts=ret["verts"], vec=Vector((0, 0, m(167))))
    obj = bm_to_object(bm, name, [mat])
    obj.location = Vector([m(v) for v in at_uu])
    return obj


def render(path, cam_uu, look_uu, lens=42, res=(1100, 780), samples=40):
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

def team_face_mask(obj):
    """TeamRegion (slot 0) = the deck-top border ring, and NOTHING ELSE. Computed
    GEOMETRICALLY after the join, so it cannot drift from face order.

    The ring is a FLUSH INLAY in the deck top -- never a parapet, which TOWER-8.3
    forbids twice over (a rail carved OUT of the deck seeds erosion from its own
    face, and a rail OUTSIDE it would breach the pinned 600x600 / 750 uu span).

    Run 1 also painted the corbel band's outer faces. EYEBALLED AND REJECTED: at
    24 uu proud and 70 uu tall it rendered as a solid blue slab hanging under the
    deck, and with the deck edge above it the tower wore three stacked bands. The
    deck ring alone is what the top-down RTS camera actually reads."""
    mask = []
    for poly in obj.data.polygons:
        c = poly.center * UE
        n = poly.normal
        out = max(abs(c.x), abs(c.y))
        top_ring = abs(c.z - RISE) < 0.01 and n.z > 0.9 and out > RING_IN + 0.01
        # the deck slab's 40 uu outer rim: the ring alone is invisible from a low
        # camera, and the rim is the ONE band that reads from every angle without
        # becoming a slab. Together they make the deck edge the ownership marker.
        rim = abs(n.z) < 0.1 and DECK_Z0 + 0.01 < c.z < RISE - 0.01 and out > DECK_HALF - 1.0
        mask.append(bool(top_ring or rim))
    return mask


# ============================================================================ main

def main():
    wipe_scene()
    report = {
        "task": "TASK-737", "asset": ASSET, "node": NODE,
        "supersedes": "TASK-727 (the 30 deg ramp build) -- ramp wedge + its 14 hulls "
                      "deleted; the 1200 uu rise, the 600x600 deck and the paths kept",
        "law": ["TOWER-8.2", "TOWER-8.3", "TOWER-8.4(A)", "TOWER-2a (nav constraints)",
                "Static-mesh SOCKET names"],
    }

    log("build render mesh")
    obj, rungs = build_render_mesh()
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
    mask = team_face_mask(obj)
    smooth = [p.use_smooth for p in obj.data.polygons]
    team_mat = new_mat("TeamRegion")
    tp = principled(team_mat)
    tp.inputs["Base Color"].default_value = (0.05, 0.30, 1.00, 1.0)   # MI_TeamColor_Blue
    tp.inputs["Roughness"].default_value = 0.55
    pbr_mat = final_material(f"{ASSET}PBR", d_path, n_path, orm_path)
    obj.data.materials.clear()
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
        "selector": "deck-top border ring ONLY (75 uu, flush inlay). NO merlons and "
                    "NO parapet: TOWER-8.3 forbids carving a rail out of the deck, "
                    "and anything outside it would breach the pinned 600x600 "
                    "footprint / 750 uu total span. The corbel band was tried and "
                    "rejected on the preview -- it read as a blue slab.",
    }

    # ---- collision ----------------------------------------------------------
    hulls = build_hulls()
    log("author %d UCX hulls (ladder deliberately gets ZERO)" % len(hulls))
    hull_objs = [h[1] for h in hulls]
    report["collision"] = {
        "authoring": "HAND-PLACED exact convex primitives exported as UCX_ nodes; "
                     "NO convex decomposition anywhere in the chain",
        "hull_count": len(hulls),
        "render_nodes": [NODE],
        "ladder_hulls": 0,
        "ladder_hulls_reason": "the ladder is traversed by a nav link, never by "
                               "pathing (76 deg is 2.4x the 32.005 deg ceiling), and "
                               "a hull at its foot would carve ground nav at exactly "
                               "the cell a unit must stand in to start climbing",
        "hulls": [{"index": i, "name": h[1].name, "role": h[0],
                   "verts": len(h[1].data.vertices),
                   "bounds_uu": measure(h[1])["bounds_min_uu"] +
                                measure(h[1])["bounds_max_uu"]}
                  for i, h in enumerate(hulls)],
    }

    log("measure the pinned contract against the authored surfaces")
    lc = measure_ladder_contract(hulls, hull_objs, obj, report)
    log("deck z dev max=%.6f | body standoff min=%.2f uu | foot standable=%s"
        % (lc["as_built"]["deck_collision_dev_from_1200_uu"]["max_abs"],
           lc["standoff"]["min_clearance_body_only_capsule_uu"],
           lc["socket_arithmetic"]["LadderFoot_is_standable"]))

    report["mesh"] = measure(obj)
    report["mesh"]["tri_budget"] = 20000
    report["mesh"]["rungs"] = rungs
    report["symmetry"] = y_symmetry(obj)

    # ---- sockets -------------------------------------------------------------
    empties = make_sockets(obj)
    report["sockets"] = {
        "shipped": {e.name: [round(e.location.x * UE, 4), round(e.location.y * UE, 4),
                             round(e.location.z * UE, 4)] for e in empties},
        "ue_socket_names": list(SOCKETS.keys()),
        "note": "FBX node SOCKET_<Name>; UE strips the prefix => socket name is <Name>",
    }

    # ---- previews (before the export mirror) --------------------------------
    log("previews")
    env = preview_env()
    t_climb = 0.62 * CLIMB_LEN
    cp = climb_point(t_climb)
    humans = [
        make_human("h_foot", (LADDER_FOOT[0] - 70.0, -110.0, 0.0)),
        make_human("h_climb", (cp.x, cp.y, cp.z), colour=(0.72, 0.46, 0.10, 1.0)),
        make_human("h_deck1", (LADDER_TOP[0], 0.0, RISE)),
        make_human("h_deck2", (110.0, 130.0, RISE)),
    ]
    for h in hull_objs:
        h.hide_render = True
    render(PREVIEWS / "watchtower_hero.png", (-1750, -1500, 900), (-120, 0, 700))
    render(PREVIEWS / "watchtower_side_elevation.png", (-100, -4200, 640),
           (-100, 0, 640), lens=38)
    render(PREVIEWS / "watchtower_ladder_detail.png", (-1050, -720, 1450),
           (-230, 0, 1130), lens=62)
    render(PREVIEWS / "watchtower_top.png", (0, 0, 3400), (0, 0, 0), lens=40)
    for h in humans:
        h.hide_render = True
    for h in hull_objs:
        h.hide_render = False
    obj.hide_render = True
    render(PREVIEWS / "watchtower_collision_hulls.png", (-1750, -1500, 900), (-120, 0, 700))
    obj.hide_render = False

    for o in env + humans:
        bpy.data.objects.remove(o, do_unlink=True)

    # ---- export -------------------------------------------------------------
    fbx = RAW / f"{ASSET}.fbx"
    export_fbx([obj] + hull_objs, empties, fbx)
    report["fbx"] = {"path": str(fbx.relative_to(ROOT)).replace("\\", "/"),
                     "bytes": fbx.stat().st_size, "sha256": sha256(fbx)}
    report["textures"] = {p.name: {"sha256": sha256(p), "bytes": p.stat().st_size}
                          for p in (d_path, n_path, orm_path)}
    report["exported_nodes"] = ([obj.name] + [h.name for h in hull_objs]
                                + [e.name for e in empties])

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
