"""
author_climb_anim.py — Siegebound art pipeline, TASK-733 (LADDER-REDESIGN).

Authors the ONE shared ladder-climb cycle `A_SiegeBiped_Climb` on the 21-bone
SiegeBiped rig and exports an ANIMATION-ONLY FBX for direct import onto the
existing /Game/Characters/SK_Footman_Skeleton.

Law: TOWER-§0 (one clip, not three) · TOWER-§8.1 (the clip is REQUIRED) ·
TOWER-§8.3 (the pinned climb line: LadderFoot (-450,0,0) -> LadderTop
(-150,0,1200); length 1236.93 uu; lean 75.964 deg from horizontal).

TASK-733b: the grips sit on the SHIPPED RUNG PLANE, which is 22 uu toward the
tower from TOWER-§8.3's pinned line (TASK-737 moved the ladder mesh there
deliberately; measured off Content/RawAssets/WatchTower.fbx and constant over the
ladder's whole run).  Applied as a RIGID translation of the entire rig, so the
authored 24 uu reach is preserved and no joint angle re-solves — see
RUNG_PLANE_OFFSET_M.  The shift is PERPENDICULAR to travel, so the advance, the
native speed and the RateScale arithmetic are all untouched.

Runs HEADLESS in Blender's bundled Python (bpy + stdlib + numpy ONLY) — the live
Blender MCP bridge has a 30 s socket cap and is never used for this:

    "<blender.exe>" --background --factory-startup --python-exit-code 1 \
        --python Tools/ArtPipeline/author_climb_anim.py -- [flags]

Sibling of rig_character.py / retarget_meshy_to_siegebiped.py. The export block is
copied VERBATIM from retarget_meshy_to_siegebiped.export_anim_only_fbx (the
sanctioned animation-export path; UE 5.8's own retarget exporter is root-only).

WHY THIS IS NOT A VERTICAL CLIMB TRANSLATED DIAGONALLY
-----------------------------------------------------
Every hand and foot target is solved by analytic 2-bone IK onto a RUNG PLANE that
is built from the pinned climb line itself: the body is leaned 14.036 deg (so the
spine is parallel to the 75.964 deg line) and the grip points advance along that
line, not along world +Z.

WHY IT IS RATE-DECOUPLED
------------------------
  * The root track is CONSTANT (lean + standoff only). The clip contributes no
    translation and no bob at any playback rate.
  * A gripping limb's LOCAL travel is exactly -CycleAdvance per cycle along the
    climb line, so world contact is preserved for ANY RateScale that satisfies
        RateScale = LadderClimbSpeedUU / NativeClimbSpeedUU
    The contract is the FORMULA, not a baked duration.
  * The cycle is C1 across the loop seam BY CONSTRUCTION (grip is linear in u,
    the swing is a cubic Hermite whose end derivatives match the grip's), and
    frame 0 sits mid-grip / mid-swing, never on a plant or release event.
"""

import bpy
import json
import math
import sys
from pathlib import Path

import numpy as np
from mathutils import Matrix, Quaternion, Vector

# --------------------------------------------------------------------------- paths
THIS = Path(__file__).resolve()
TOOLS = THIS.parent
REPO = TOOLS.parent.parent
CHAR_RAW = REPO / "Content" / "RawAssets" / "Characters"
ANIM_RAW = CHAR_RAW / "Anims"
CACHE = TOOLS / "Cache" / "SiegeBipedClimb"

RIG_FBX = CHAR_RAW / "Footman.fbx"
OUT_FBX = ANIM_RAW / "SiegeBiped_Climb.fbx"

SHARED_SKELETON_ROOT = "Footman_Rig"   # rig_character.SHARED_SKELETON_ROOT
ACTION_NAME = "A_SiegeBiped_Climb"
EXPECTED_BONES = 21
UE = 100.0                              # 1 Blender metre = 100 UE units

# --------------------------------------------------------------------------- TOWER-§8.3 pinned geometry (UE units, tower-local)
LADDER_FOOT_UU = Vector((-450.0, 0.0, 0.0))
LADDER_TOP_UU = Vector((-150.0, 0.0, 1200.0))

# --------------------------------------------------------------------------- cycle contract
# fps 60 is a DECLARED deviation from the rig lane's 30: at the shipped
# LadderClimbSpeedUU=350 this clip runs a 0.267 s cycle, so 30 fps would give it
# only 8 source keys per cycle.  60 fps doubles that for zero cost.  Nothing in
# the project couples to this clip's fps (RateScale arithmetic is fps-free).
FPS = 60
CYCLE_FRAMES = 16            # 0..16 inclusive; frame 16 == frame 0
CYCLE_ADVANCE_UU = 90.0      # ladder-line travel per loop (see --advance)

# body placement
BODY_STANDOFF_M = 0.24       # body sits this far BEHIND the rung plane (THE AUTHORED
                             # REACH — every joint angle is solved against this and
                             # this alone; TASK-733b did NOT change it)

# --------------------------------------------------------------------------- TASK-733b: the shipped rung plane is NOT the pinned line
# TOWER-§8.3 pins the climb LINE, and TASK-733 built the grips exactly on it
# (offset 0).  TASK-737 then re-authored the ladder mesh deliberately 22 uu toward
# the tower — "a unit stands AT the foot of a ladder, not inside it" — and declared
# the deviation.  Both lanes were internally correct and built to two readings of
# one law; the grips therefore landed 22 uu short of the rungs.
#
# MEASURED OFF THE LANDED Content/RawAssets/WatchTower.fbx (TASK-733b, own binary
# FBX reader, independent of TASK-739's):
#     ladder slab near face  m = -12.000 uu   (climber side)
#     ladder slab far face   m = -32.000 uu   -> 20.000 uu thick
#     ⇒ rung MID-PLANE      m = -22.000 uu
#   and CONSTANT to the digit over the ladder's whole run (u = 8 -> 1199 uu, 12
#   buckets, min/max -13/-31 in every one) — which is what makes a RIGID shift the
#   correct fix rather than a fan or a re-solve.
#   Hull-safe: all 8 UCX_ hulls stop at x = -300 uu, so nothing collides there.
#   (m = the in-plane normal to the climb direction pointing AWAY from the tower;
#    this script's `n` is its negation, so m = -22 uu  <=>  n = +0.22 m.)
#
# Applied as a RIGID translation of the whole rig along +n: the grip plane and the
# body move together, so BODY_STANDOFF_M — the distance the IK actually solves
# against — is untouched and NOTHING RE-SOLVES.  Every bone rotation is bit-identical
# to the pre-shift clip; only the root's location channel differs.
RUNG_PLANE_OFFSET_M = 0.22   # rung plane sits this far along +n from the pinned line

HAND_LATERAL_M = 0.18        # grip half-separation on the rung
ANKLE_BEHIND_RUNG_M = 0.08   # ankle sits behind the rung; the toe rests on it
FOOT_LATERAL_M = 0.115

# reach safety
ARM_REACH_SAFETY = 0.955
LEG_REACH_SAFETY = 0.955
# The low end of a grip is bounded by a MINIMUM shoulder-to-hand DISTANCE, not by
# a height: distance is minimised at shoulder level, so a band that straddles the
# shoulder collapses the elbow no matter where it is centred.
HAND_MIN_REACH_FRAC = 0.38   # closest grip, as a fraction of full arm reach
FOOT_HIGH_LIMIT_FRAC = -0.28  # highest foot, as a fraction of full leg reach, rel. hip

# swing shaping
HAND_SWING_BACK_M = 0.075
FOOT_SWING_BACK_M = 0.095

X_AXIS, Y_AXIS, Z_AXIS = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))

WARN = []


def log(msg):
    print(f"[climb] {msg}", flush=True)


def fail(msg, code=1):
    print(f"[climb] FAIL: {msg}", flush=True)
    sys.exit(code)


# --------------------------------------------------------------------------- args
def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    out = {"advance": CYCLE_ADVANCE_UU, "frames": CYCLE_FRAMES, "previews": True,
           "save_blend": False, "probe": False, "verify_fbx": False}
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--advance":
            out["advance"] = float(argv[i + 1]); i += 2
        elif a == "--frames":
            out["frames"] = int(argv[i + 1]); i += 2
        elif a == "--no-previews":
            out["previews"] = False; i += 1
        elif a == "--save-blend":
            out["save_blend"] = True; i += 1
        elif a == "--probe":
            out["probe"] = True; i += 1
        elif a == "--verify-fbx":
            out["verify_fbx"] = True; i += 1
        else:
            fail(f"unknown flag {a}")
    return out


# --------------------------------------------------------------------------- bpy helpers
def ensure_object_mode():
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def select_only(objs, active=None):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    vl.objects.active = active if active is not None else (objs[0] if objs else None)


def reset_scene():
    ensure_object_mode()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=True)
    for coll in (bpy.data.meshes, bpy.data.armatures, bpy.data.actions,
                 bpy.data.materials, bpy.data.images, bpy.data.cameras,
                 bpy.data.lights):
        for blk in list(coll):
            if blk.users == 0:
                coll.remove(blk)


def upd():
    bpy.context.view_layer.update()


def import_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    return [o for o in bpy.data.objects if o not in before]


def bind_action(arm_obj, action):
    """Blender 4.4+ slotted actions: the slot MUST be bound or the fcurves do not
    evaluate and the exporter bakes the rest pose (rig_character.bind_action)."""
    ad = arm_obj.animation_data or arm_obj.animation_data_create()
    ad.action = action
    if action is not None:
        slots = getattr(action, "slots", None)
        if slots is not None and len(slots) > 0:
            try:
                ad.action_slot = slots[0]
            except (AttributeError, TypeError):
                pass


def clear_pose(arm_obj):
    for pb in arm_obj.pose.bones:
        pb.location = (0, 0, 0)
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.scale = (1, 1, 1)


def iter_fcurves(action):
    flat = getattr(action, "fcurves", None)
    if flat is not None:
        for fc in flat:
            yield fc
        return
    for layer in getattr(action, "layers", []):
        for strip in getattr(layer, "strips", []):
            for cbag in getattr(strip, "channelbags", []):
                for fc in cbag.fcurves:
                    yield fc


# --------------------------------------------------------------------------- rig load
def load_rig():
    if not RIG_FBX.is_file():
        fail(f"SiegeBiped rig FBX missing: {RIG_FBX}")
    imported = import_fbx(RIG_FBX)
    arms = [o for o in imported if o.type == "ARMATURE"]
    meshes = [o for o in imported if o.type == "MESH"]
    if len(arms) != 1:
        fail(f"expected exactly 1 armature in {RIG_FBX.name}, got {len(arms)}")
    arm = arms[0]
    if arm.name != SHARED_SKELETON_ROOT:
        WARN.append(f"armature imported as '{arm.name}' -> renamed "
                    f"'{SHARED_SKELETON_ROOT}' (TASK-212 shared-root law)")
        arm.name = SHARED_SKELETON_ROOT
    n = len(arm.data.bones)
    if n != EXPECTED_BONES:
        fail(f"SiegeBiped rig has {n} bones, expected {EXPECTED_BONES}")
    loc, _rot, sca = arm.matrix_world.decompose()
    if max(abs(s - 1.0) for s in sca) > 1e-4 or loc.length > 1e-4:
        WARN.append(f"armature object transform not identity "
                    f"(loc={tuple(round(v,5) for v in loc)}, "
                    f"scale={tuple(round(v,5) for v in sca)}) — all math is in "
                    "ARMATURE space so this does not affect the clip")
    if arm.animation_data:
        bind_action(arm, None)
    clear_pose(arm)
    upd()
    return arm, meshes


# --------------------------------------------------------------------------- ladder frame
def ladder_frame():
    """The pinned climb line, expressed in the CHARACTER's armature space.

    World (tower-local) -> character: the climber faces the tower (+X world), so
    char front (-Y) maps to world +X, char up (+Z) to world +Z, char left (+X) to
    world +Y.  d = climb direction, n = body -> rung-plane normal, e = char left.
    """
    delta_uu = LADDER_TOP_UU - LADDER_FOOT_UU
    length_uu = delta_uu.length
    lean_deg = math.degrees(math.atan2(delta_uu.z, math.hypot(delta_uu.x, delta_uu.y)))
    tilt_deg = 90.0 - lean_deg                      # from vertical
    s, c = math.sin(math.radians(tilt_deg)), math.cos(math.radians(tilt_deg))
    d = Vector((0.0, -s, c)).normalized()           # up + forward
    n = Vector((0.0, -c, -s)).normalized()          # perpendicular, toward the rungs
    e = Vector((1.0, 0.0, 0.0))                     # character LEFT
    assert abs(d.dot(n)) < 1e-9
    return {"d": d, "n": n, "e": e, "length_uu": length_uu,
            "lean_deg": lean_deg, "tilt_deg": tilt_deg}


def rung_point(fr, u, lateral, perp):
    """A point on/near the rung plane: u along the climb line, lateral across it,
    perp along the plane normal (0 = exactly on the rung plane).

    TASK-733b: the rung plane is RUNG_PLANE_OFFSET_M along +n from the pinned line,
    because that is where the shipped ladder mesh actually is (see the constant).
    The root carries the same offset, so this is a rigid translation."""
    return (fr["d"] * u + fr["e"] * lateral
            + fr["n"] * (perp + RUNG_PLANE_OFFSET_M))


# --------------------------------------------------------------------------- limb cycle
def hermite_swing(s):
    """h(0)=0, h(1)=1, h'(0)=h'(1)=-1  ->  C1 with the linear grip phase.
    Its natural -4.4% dip / +4.4% overshoot read as release-anticipation and
    reach-overshoot; they are counted in the reach budget."""
    return -4.0 * s ** 3 + 6.0 * s ** 2 - s


SWING_MIN = -0.04434   # min of hermite_swing on [0,1]
SWING_MAX = 1.04434    # max of hermite_swing on [0,1]


def limb_state(phi):
    """Grip on phase [0.25,0.75); swing on [0.75,1.25).  Frame 0 therefore lands
    MID-grip / MID-swing — never on a plant or release event — which is what makes
    the loop seam derivative-continuous instead of a corner."""
    p = phi % 1.0
    if 0.25 <= p < 0.75:
        return "grip", (p - 0.25) / 0.5
    return "swing", ((p - 0.75) % 1.0) / 0.5


def limb_uv(phi, u_top, adv, back_m):
    """-> (u along the climb line, perpendicular offset toward -n)."""
    mode, s = limb_state(phi)
    half = adv * 0.5
    if mode == "grip":
        return u_top - half * s, 0.0
    return (u_top - half) + half * hermite_swing(s), -back_m * math.sin(math.pi * s) ** 2


# --------------------------------------------------------------------------- IK
def two_bone_ik(root_p, target_p, l1, l2, pole):
    v = target_p - root_p
    dist = v.length
    max_d = (l1 + l2) * 0.9995
    min_d = abs(l1 - l2) * 1.02 + 1e-4
    dc = min(max(dist, min_d), max_d)
    u = v.normalized() if dist > 1e-9 else Vector((0, 0, -1))
    w = pole - u * pole.dot(u)
    if w.length < 1e-6:
        w = u.orthogonal()
    w.normalize()
    cos_a = (l1 * l1 + dc * dc - l2 * l2) / (2.0 * l1 * dc)
    a = math.acos(min(max(cos_a, -1.0), 1.0))
    elbow = root_p + (u * math.cos(a) + w * math.sin(a)) * l1
    reach_p = root_p + u * dc
    return elbow, reach_p, (dist - dc)


def aim_matrix(head, direction, ref):
    y = direction.normalized()
    r = ref.normalized()
    x = y.cross(r)
    if x.length < 1e-6:
        x = y.cross(Z_AXIS)
        if x.length < 1e-6:
            x = y.cross(X_AXIS)
    x.normalize()
    z = x.cross(y)
    rot = Matrix((x, y, z)).transposed().to_4x4()
    return Matrix.Translation(head) @ rot


def set_aim(pb, direction, ref):
    """Set a pose bone's ARMATURE-space orientation, holding its current head
    (which the already-posed parent determines) so nothing translates."""
    upd()
    head = pb.head.copy()
    pb.matrix = aim_matrix(head, direction, ref)
    upd()


def rest_rot(pb):
    return pb.bone.matrix_local.to_3x3()


def world_quat(pb, axis, angle_deg):
    """rig_character.world_quat — a local quaternion producing a rotation about a
    WORLD axis through the bone (exact at rest parents, natural bend in a chain)."""
    R = rest_rot(pb)
    Rw = Matrix.Rotation(math.radians(angle_deg), 3, Vector(axis).normalized())
    return (R.inverted() @ Rw @ R).to_quaternion()


def set_local_rot(pb, rots):
    q = Quaternion((1, 0, 0, 0))
    for axis, ang in rots:
        q = q @ world_quat(pb, axis, ang)
    pb.rotation_mode = "QUATERNION"
    pb.rotation_quaternion = q


# --------------------------------------------------------------------------- pose build
TORSO_LEAN_EXTRA = {"spine_01": 1.5, "spine_02": 3.5, "spine_03": 2.0}
NECK_X, HEAD_X = -16.0, -26.0
TWIST_CHEST, TWIST_PELVIS = 7.0, 4.0
CLAV_ROLL, CLAV_SHRUG = 5.0, 7.0


def build_pose(arm, fr, phi, geom, adv):
    """Pose the rig for one phase. Order is parent-before-child with a depsgraph
    update between, so every IK solve reads a TRUE shoulder / hip position."""
    pb = arm.pose.bones
    d, n, e = fr["d"], fr["n"], fr["e"]

    clear_pose(arm)

    # ---- root: the LEAN + the constant standoff. No cyclic term: the root track
    #      is constant, so the clip contributes zero translation at any rate.
    root = pb["root"]
    root.rotation_mode = "QUATERNION"
    root.rotation_quaternion = world_quat(root, X_AXIS, fr["tilt_deg"])
    # TASK-733b: body = rung plane MINUS the standoff.  The standoff itself is
    # unchanged, so the reach the IK solves against is unchanged.
    root.location = rest_rot(root).inverted() @ (
        -n * (BODY_STANDOFF_M - RUNG_PLANE_OFFSET_M))
    upd()

    tw = math.sin(2.0 * math.pi * phi)
    set_local_rot(pb["pelvis"], [(Z_AXIS, TWIST_PELVIS * tw)])
    upd()
    for b, ang in TORSO_LEAN_EXTRA.items():
        twist = -TWIST_CHEST * tw if b == "spine_02" else (-TWIST_CHEST * 0.4 * tw
                                                           if b == "spine_03" else 0.0)
        set_local_rot(pb[b], [(X_AXIS, ang), (Z_AXIS, twist)])
        upd()
    set_local_rot(pb["neck_01"], [(X_AXIS, NECK_X)])
    upd()
    set_local_rot(pb["head"], [(X_AXIS, HEAD_X + 2.0 * tw)])
    upd()

    # ---- clavicles: rolled forward onto the ladder + a shrug that tracks the
    #      hand that is currently high.
    for side, sgn in (("l", 1.0), ("r", -1.0)):
        phase = phi if side == "l" else phi + 0.5
        high = 0.5 + 0.5 * math.sin(2.0 * math.pi * phase)
        set_local_rot(pb[f"clavicle_{side}"],
                      [(Z_AXIS, -CLAV_ROLL * sgn),
                       (Y_AXIS, -CLAV_SHRUG * sgn * high)])
        upd()

    residual = 0.0
    contacts = {}

    # ---- arms (IK onto the rung plane) -----------------------------------
    for side, sgn in (("l", 1.0), ("r", -1.0)):
        phase = phi if side == "l" else phi + 0.5
        u, perp = limb_uv(phase, geom["hand_u_top"], adv, HAND_SWING_BACK_M)
        tgt = rung_point(fr, u, sgn * HAND_LATERAL_M, perp)
        up_b, lo_b, hd_b = pb[f"upperarm_{side}"], pb[f"lowerarm_{side}"], pb[f"hand_{side}"]
        upd()
        sh = up_b.head.copy()
        pole = (e * sgn * 0.75 - d * 0.55 - n * 0.35)
        elbow, reach, err = two_bone_ik(sh, tgt, geom["l_upper"], geom["l_lower"], pole)
        residual = max(residual, abs(err))
        set_aim(up_b, elbow - sh, pole)
        set_aim(lo_b, reach - elbow, pole)
        mode, _s = limb_state(phase)
        grip_dir = (n * 0.55 - d * 0.83) if mode == "grip" else (n * 0.30 + d * 0.95)
        set_aim(hd_b, grip_dir, e * sgn)
        contacts[f"hand_{side}"] = {"mode": mode, "u": u, "perp": perp,
                                    "pos": tuple(reach), "target": tuple(tgt)}

    # ---- legs (IK onto the rungs) ----------------------------------------
    for side, sgn in (("l", 1.0), ("r", -1.0)):
        phase = (phi + 0.5) if side == "l" else phi     # contralateral to the hands
        u, perp = limb_uv(phase, geom["foot_u_top"], adv, FOOT_SWING_BACK_M)
        tgt = rung_point(fr, u, sgn * FOOT_LATERAL_M, perp - ANKLE_BEHIND_RUNG_M)
        th_b, cf_b, ft_b = pb[f"thigh_{side}"], pb[f"calf_{side}"], pb[f"foot_{side}"]
        upd()
        hip = th_b.head.copy()
        pole = (e * sgn * 0.55 + n * 0.80 + d * 0.10)
        knee, reach, err = two_bone_ik(hip, tgt, geom["l_thigh"], geom["l_calf"], pole)
        residual = max(residual, abs(err))
        set_aim(th_b, knee - hip, pole)
        set_aim(cf_b, reach - knee, pole)
        mode, _s = limb_state(phase)
        foot_dir = (n * 1.0 + d * 0.25) if mode == "grip" else (n * 0.55 - d * 0.55)
        set_aim(ft_b, foot_dir, e * sgn)
        contacts[f"foot_{side}"] = {"mode": mode, "u": u, "perp": perp,
                                    "pos": tuple(reach), "target": tuple(tgt)}

    upd()
    return residual, contacts


# --------------------------------------------------------------------------- geometry budget
def measure(arm, fr, adv):
    b = arm.data.bones
    geom = {
        "l_upper": b["upperarm_l"].length, "l_lower": b["lowerarm_l"].length,
        "l_thigh": b["thigh_l"].length, "l_calf": b["calf_l"].length,
        "height_m": float(b["head"].tail_local.z),
    }
    arm_reach = geom["l_upper"] + geom["l_lower"]
    leg_reach = geom["l_thigh"] + geom["l_calf"]

    # neutral (lean + standoff, no cycle) shoulder / hip, in the ladder frame
    build_pose_static(arm, fr)
    d, n, e = fr["d"], fr["n"], fr["e"]
    sh = arm.pose.bones["upperarm_l"].head.copy()
    hip = arm.pose.bones["thigh_l"].head.copy()

    def decomp(p):
        return p.dot(d), p.dot(n), p.dot(e)

    s_d, s_n, s_e = decomp(sh)
    h_d, h_n, h_e = decomp(hip)

    # TASK-733b: the rung plane's n-coordinate is RUNG_PLANE_OFFSET_M, not 0.  The
    # shoulder/hip moved by exactly the same amount (build_pose_static carries the
    # offset too), so both differences below are INVARIANT under the shift and the
    # whole reach budget is unchanged — which is the point of a rigid translation.
    q_hand = math.hypot(RUNG_PLANE_OFFSET_M - s_n, HAND_LATERAL_M - s_e)
    q_foot = math.hypot((RUNG_PLANE_OFFSET_M - ANKLE_BEHIND_RUNG_M) - h_n,
                        FOOT_LATERAL_M - h_e)

    hand_hi = math.sqrt(max(1e-6, (arm_reach * ARM_REACH_SAFETY) ** 2 - q_hand ** 2))
    r_low = HAND_MIN_REACH_FRAC * arm_reach
    hand_lo = math.sqrt(r_low ** 2 - q_hand ** 2) if r_low > q_hand else 0.0
    foot_lo = -math.sqrt(max(1e-6, (leg_reach * LEG_REACH_SAFETY) ** 2 - q_foot ** 2))
    foot_hi = FOOT_HIGH_LIMIT_FRAC * leg_reach

    hand_band = hand_hi - hand_lo
    foot_band = foot_hi - foot_lo
    need = (SWING_MAX - SWING_MIN) * adv * 0.5     # 1.0887 * half-cycle advance

    # Centre each limb's excursion inside its own reachable band.
    # u_top is the TOP of the grip phase; the excursion runs
    #   [u_top - (1-SWING_MIN)*half , u_top - (1-SWING_MAX)*half]
    # whose centre is u_top - 0.5*half (SWING_MIN + SWING_MAX == 1 exactly).
    half = adv * 0.5
    hand_centre = s_d + (hand_hi + hand_lo) * 0.5
    foot_centre = h_d + (foot_hi + foot_lo) * 0.5
    geom["hand_u_top"] = hand_centre + 0.5 * half
    geom["foot_u_top"] = foot_centre + 0.5 * half

    geom.update({
        "arm_reach_m": arm_reach, "leg_reach_m": leg_reach,
        "hand_band_m": hand_band, "foot_band_m": foot_band,
        "need_m": need,
        "hand_headroom_m": hand_band - need, "foot_headroom_m": foot_band - need,
        "shoulder_ladder": (s_d, s_n, s_e), "hip_ladder": (h_d, h_n, h_e),
    })
    clear_pose(arm)
    upd()
    return geom


def build_pose_static(arm, fr):
    """Lean + standoff only — the neutral frame the reach budget is measured in."""
    clear_pose(arm)
    root = arm.pose.bones["root"]
    root.rotation_mode = "QUATERNION"
    root.rotation_quaternion = world_quat(root, X_AXIS, fr["tilt_deg"])
    root.location = rest_rot(root).inverted() @ (
        -fr["n"] * (BODY_STANDOFF_M - RUNG_PLANE_OFFSET_M))
    upd()
    for b, ang in TORSO_LEAN_EXTRA.items():
        set_local_rot(arm.pose.bones[b], [(X_AXIS, ang)])
        upd()


# --------------------------------------------------------------------------- authoring
ANIM_BONES = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "head"] + [
    f"{b}_{s}" for s in ("l", "r")
    for b in ("clavicle", "upperarm", "lowerarm", "hand", "thigh", "calf", "foot")
]


def author(arm, fr, geom, adv, frames):
    action = bpy.data.actions.new(ACTION_NAME)
    bind_action(arm, action)
    pb = arm.pose.bones
    prev_q = {}
    max_residual = 0.0
    samples = []

    for f in range(frames + 1):
        phi = f / float(frames)
        res, contacts = build_pose(arm, fr, phi, geom, adv)
        max_residual = max(max_residual, res)

        # hemisphere-continuous quaternions: a sign flip between frames makes the
        # fcurve take the long way round.
        for name in ANIM_BONES:
            b = pb[name]
            q = b.rotation_quaternion.copy()
            p = prev_q.get(name)
            if p is not None and q.dot(p) < 0.0:
                q = Quaternion((-q.w, -q.x, -q.y, -q.z))
                b.rotation_quaternion = q
            prev_q[name] = q
            if name != "root":
                if b.location.length > 1e-5:
                    WARN.append(f"non-root bone '{name}' drifted "
                                f"{b.location.length*UE:.4f} uu at frame {f} — zeroed")
                b.location = (0, 0, 0)
            b.keyframe_insert("rotation_quaternion", frame=f)
            b.keyframe_insert("location", frame=f)
        upd()

        joints = {}
        for s in ("l", "r"):
            for tag, (a, b, c) in (
                    (f"elbow_{s}", (f"upperarm_{s}", f"lowerarm_{s}", f"hand_{s}")),
                    (f"knee_{s}", (f"thigh_{s}", f"calf_{s}", f"foot_{s}"))):
                v1 = pb[a].head - pb[b].head
                v2 = pb[c].head - pb[b].head
                if v1.length > 1e-6 and v2.length > 1e-6:
                    joints[tag] = math.degrees(math.acos(
                        max(-1.0, min(1.0, v1.normalized().dot(v2.normalized())))))
        samples.append({"frame": f, "phi": phi, "contacts": contacts, "joints": joints,
                        "bones": {n: tuple(pb[n].head) for n in
                                  ("hand_l", "hand_r", "foot_l", "foot_r",
                                   "head", "pelvis")},
                        "tips": {n: tuple(pb[n].tail) for n in
                                 ("hand_l", "hand_r", "foot_l", "foot_r")}})

    # per-frame keys on a smooth periodic function: LINEAR keeps the seam's
    # velocity exact (BEZIER end-handles would flatten frame 0 / frame N).
    for fc in iter_fcurves(action):
        for kp in fc.keyframe_points:
            kp.interpolation = "LINEAR"
        fc.update()

    return action, max_residual, samples


# --------------------------------------------------------------------------- verification
def verify(action, samples, fr, adv_m, frames):
    adv = adv_m * UE                       # uu, for the declared cycle numbers
    """Three numeric gates: pose identity at the seam, derivative continuity across
    the seam, and world-space contact (the anti-skate proof)."""
    rep = {}

    # ---- 1. seam pose identity: every fcurve's key at N equals its key at 0
    worst = 0.0
    worst_ch = None
    for fc in iter_fcurves(action):
        kv = {int(round(kp.co[0])): kp.co[1] for kp in fc.keyframe_points}
        if 0 in kv and frames in kv:
            dv = abs(kv[frames] - kv[0])
            if dv > worst:
                worst, worst_ch = dv, f"{fc.data_path}[{fc.array_index}]"
    rep["seam_pose_max_delta"] = worst
    rep["seam_pose_worst_channel"] = worst_ch

    # ---- 2. seam CORNER test.  The frame track is periodic by construction, so a
    #     naive v(N)-v(0) difference only measures CURVATURE, not a break.  The
    #     real question is whether frame 0 is a corner: compute the wrapped second
    #     difference at every frame and check the seam is not an outlier.  The
    #     genuine corners are the plant/release frames at quarter phase, which is
    #     exactly why frame 0 was placed mid-grip / mid-swing.
    def bpos(i, name):
        return Vector(samples[i % frames]["bones"][name]) * UE

    corner = {}
    for name in ("hand_l", "hand_r", "foot_l", "foot_r", "head", "pelvis"):
        d2 = [((bpos(f + 1, name) - bpos(f, name) * 2 + bpos(f - 1, name)).length)
              for f in range(frames)]
        others = d2[1:]
        corner[name] = {
            "seam_uu": d2[0],
            "max_elsewhere_uu": max(others),
            "argmax_frame": 1 + others.index(max(others)),
            "seam_over_max": d2[0] / max(others) if max(others) > 1e-9 else 0.0,
        }
    rep["seam_corner"] = corner
    rep["seam_corner_max_ratio"] = max(c["seam_over_max"] for c in corner.values())

    # ---- 3. contact: a gripping limb's WORLD position, once the traversal's
    #        advance (adv * phi along d) is added back, must be STATIONARY.
    d = fr["d"]
    skate = {}
    for limb in ("hand_l", "hand_r", "foot_l", "foot_r"):
        runs, cur = [], []
        for s in samples[:-1]:                      # frame N duplicates frame 0
            if s["contacts"][limb]["mode"] == "grip":
                world = Vector(s["contacts"][limb]["pos"]) + d * adv_m * s["phi"]
                cur.append((s["frame"], world))
            elif cur:
                runs.append(cur); cur = []
        if cur:
            runs.append(cur)
        # the grip window wraps the seam for one limb; measure each contiguous run
        worst_run = 0.0
        for run in runs:
            if len(run) < 2:
                continue
            pts = [p for _f, p in run]
            ctr = sum(pts, Vector((0, 0, 0))) / len(pts)
            worst_run = max(worst_run, max((p - ctr).length for p in pts) * UE)
        skate[limb] = worst_run
    rep["contact_skate_uu"] = skate
    rep["contact_skate_max_uu"] = max(skate.values()) if skate else 0.0

    # ---- 3b. joint sanity: 180 deg = locked straight, < ~35 deg = collapsed
    jt = {}
    for tag in samples[0]["joints"]:
        vals = [s["joints"][tag] for s in samples[:-1]]
        jt[tag] = {"min_deg": min(vals), "max_deg": max(vals)}
    rep["joints"] = jt
    rep["joint_min_deg"] = min(v["min_deg"] for v in jt.values())
    rep["joint_max_deg"] = max(v["max_deg"] for v in jt.values())

    # ---- 4. declared cycle numbers
    lean = math.radians(fr["lean_deg"])
    rep["cycle"] = {
        "frames": frames, "fps": FPS,
        "duration_s": frames / float(FPS),
        "advance_along_line_uu": adv,
        "advance_vertical_uu": adv * math.sin(lean),
        "native_speed_along_line_uu_s": adv / (frames / float(FPS)),
        "native_vertical_speed_uu_s": adv * math.sin(lean) / (frames / float(FPS)),
        "strokes_per_cycle": 2,
        "stroke_advance_uu": adv * 0.5,
    }
    return rep


# --------------------------------------------------------------------------- export
def export_anim_only(arm, frames, out_path):
    """VERBATIM from retarget_meshy_to_siegebiped.export_anim_only_fbx."""
    out_path.parent.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.fps = FPS          # pinned BEFORE export (rig_character's known trap)
    scene.render.fps_base = 1.0
    scene.frame_start = 0
    scene.frame_end = frames
    ensure_object_mode()
    select_only([arm], active=arm)
    bpy.ops.export_scene.fbx(
        filepath=str(out_path), use_selection=True,
        object_types={"ARMATURE"},
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Z", axis_up="Y", mesh_smooth_type="FACE",
        use_mesh_modifiers=False, add_leaf_bones=False,
        bake_anim=True, bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False, bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        use_armature_deform_only=False,
        primary_bone_axis="Y", secondary_bone_axis="X",
        path_mode="STRIP",
    )
    log(f"EXPORT: {out_path}")
    return out_path


# --------------------------------------------------------------------------- preview
def build_ladder_proxy(fr, u_lo, u_hi, half_w=0.66, spacing=0.30, stiles=(-1, 1),
                       name="LadderProxy"):
    """A throwaway visual reference at the PINNED line — previews only, never
    selected for export."""
    verts, faces = [], []
    d, n, e = fr["d"], fr["n"], fr["e"]

    def box(centre, ax, ay, az):
        base = len(verts)
        for sx in (-1, 1):
            for sy in (-1, 1):
                for sz in (-1, 1):
                    verts.append(centre + ax * sx + ay * sy + az * sz)
        idx = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1),
               (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
        faces.extend([tuple(base + i for i in f) for f in idx])

    # TASK-733b: the proxy now stands where the SHIPPED ladder actually stands —
    # centred on the rung plane (+RUNG_PLANE_OFFSET_M along n) and carrying the
    # measured 20 uu slab thickness (n = 0.12 .. 0.32 m  <=>  m = -12 .. -32 uu)
    # and the measured ±66 uu clear span between ±86 uu stiles.  A preview drawn at
    # the pinned line would show the hands meeting a ladder that is not there.
    off = n * RUNG_PLANE_OFFSET_M
    half_t = 0.10                       # half of the measured 20 uu slab
    stile_half = 0.10                   # ±86 outer / ±66 inner => 20 uu wide stile
    for sgn in stiles:
        centre = fr["d"] * ((u_lo + u_hi) * 0.5) + e * (sgn * (half_w + stile_half)) + off
        box(centre, e * stile_half, n * half_t, d * ((u_hi - u_lo) * 0.5))
    k = int((u_hi - u_lo) / spacing) + 1
    for i in range(k):
        u = u_lo + i * spacing
        box(d * u + off, e * half_w, n * half_t, d * 0.028)

    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(v) for v in verts], [], faces)
    me.update()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    mat = bpy.data.materials.new("LadderProxyMat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (0.42, 0.24, 0.10, 1.0)
    ob.data.materials.append(mat)
    return ob


def setup_render(px):
    scene = bpy.context.scene
    scene.render.resolution_x = scene.render.resolution_y = px
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE", "BLENDER_WORKBENCH"):
        try:
            scene.render.engine = eng
            break
        except Exception:
            continue
    if scene.world is None:
        scene.world = bpy.data.worlds.new("ClimbPreviewWorld")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.06, 0.07, 0.09, 1.0)
        bg.inputs["Strength"].default_value = 0.8
    if not any(o.type == "LIGHT" for o in bpy.data.objects):
        ld = bpy.data.lights.new("ClimbSun", "SUN")
        ld.energy = 4.5
        sun = bpy.data.objects.new("ClimbSun", ld)
        bpy.context.scene.collection.objects.link(sun)
        sun.rotation_euler = Vector((-0.6, -0.3, -0.8)).to_track_quat("-Z", "Y").to_euler()


def place_cam(centre, dist, direction, name="ClimbCam"):
    cam = bpy.data.objects.get(name)
    if cam is None:
        cd = bpy.data.cameras.new(name)
        cam = bpy.data.objects.new(name, cd)
        bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    dv = Vector(direction).normalized()
    cam.location = Vector(centre) + dv * dist
    cam.rotation_euler = (-dv).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 55
    cam.data.clip_end = dist * 12
    return cam


def render_still(path):
    scene = bpy.context.scene
    scene.render.image_settings.file_format = "PNG"
    scene.render.use_file_extension = False
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def tile_strip(pngs, out_path, cols=None):
    imgs = []
    for p in pngs:
        im = bpy.data.images.load(str(p))
        w, h = im.size
        a = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)
        imgs.append(a)
        bpy.data.images.remove(im)
    cols = cols or len(imgs)
    rows = math.ceil(len(imgs) / cols)
    h, w, _ = imgs[0].shape
    canvas = np.zeros((h * rows, w * cols, 4), dtype=np.float32)
    canvas[..., 3] = 1.0
    for i, a in enumerate(imgs):
        r, c = divmod(i, cols)
        r = rows - 1 - r                     # bottom-up pixel order
        canvas[r * h:(r + 1) * h, c * w:(c + 1) * w] = a
    out = bpy.data.images.new("strip", width=w * cols, height=h * rows, alpha=True)
    out.pixels = canvas.reshape(-1).tolist()
    out.file_format = "PNG"
    out.filepath_raw = str(out_path)
    out.save()
    bpy.data.images.remove(out)
    return out_path


def neutralize_materials(meshes):
    """The rig FBX's textures are path-stripped (raw-asset law), so the imported
    material renders as the missing-texture magenta. Previews are about MOTION —
    swap in flat clay so the silhouette reads."""
    clay = bpy.data.materials.new("ClimbPreviewClay")
    clay.use_nodes = True
    b = clay.node_tree.nodes.get("Principled BSDF")
    if b:
        b.inputs["Base Color"].default_value = (0.72, 0.73, 0.76, 1.0)
        if "Roughness" in b.inputs:
            b.inputs["Roughness"].default_value = 0.55
    for m in meshes:
        m.data.materials.clear()
        m.data.materials.append(clay)


def make_marker(pos, radius, colour=(0.90, 0.10, 0.08, 1.0), name="ContactMarker"):
    me = bpy.data.meshes.new(name)
    verts, faces = [], []
    seg = 10
    for i in range(seg + 1):
        th = math.pi * i / seg
        for j in range(seg):
            ph = 2 * math.pi * j / seg
            verts.append((pos.x + radius * math.sin(th) * math.cos(ph),
                          pos.y + radius * math.sin(th) * math.sin(ph),
                          pos.z + radius * math.cos(th)))
    for i in range(seg):
        for j in range(seg):
            a = i * seg + j
            b = i * seg + (j + 1) % seg
            faces.append((a, b, b + seg, a + seg))
    me.from_pydata(verts, [], faces)
    me.update()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    mat = bpy.data.materials.new(f"{name}Mat")
    mat.use_nodes = True
    b = mat.node_tree.nodes.get("Principled BSDF")
    if b:
        b.inputs["Base Color"].default_value = colour
        if "Emission Color" in b.inputs:
            b.inputs["Emission Color"].default_value = colour
            b.inputs["Emission Strength"].default_value = 1.2
    ob.data.materials.append(mat)
    return ob


def body_points(arm, offset=Vector((0, 0, 0))):
    pts = []
    for pb in arm.pose.bones:
        pts.append(pb.head + offset)
        pts.append(pb.tail + offset)
    return pts


def frame_points(pts, direction, fill=1.22, lens=55.0):
    mn = Vector((min(p[i] for p in pts) for i in range(3)))
    mx = Vector((max(p[i] for p in pts) for i in range(3)))
    centre = (mn + mx) * 0.5
    radius = max(1e-3, (mx - mn).length * 0.5)
    half_fov = math.atan(18.0 / lens)
    dist = radius / math.tan(half_fov) * fill
    return place_cam(centre, dist, direction)


def render_previews(arm, meshes, fr, adv_m, frames, cache):
    cache.mkdir(parents=True, exist_ok=True)
    setup_render(600)
    neutralize_materials(meshes)
    d = fr["d"]
    movers = [arm] + list(meshes)
    base_loc = {o.name: o.location.copy() for o in movers}
    shots = []
    picks = list(range(0, frames, max(1, frames // 6)))[:6]

    def strip(tag, cam_pts, cam_dir, frame_list, advance=False, fill=1.22,
              proxy_stiles=(-1, 1), u_lo=-0.9, u_hi=3.1):
        prox = build_ladder_proxy(fr, u_lo, u_hi, stiles=proxy_stiles, name=f"Prox_{tag}")
        bpy.context.scene.frame_set(frame_list[0])
        upd()
        frame_points(cam_pts(), cam_dir, fill=fill)
        pngs = []
        for f in frame_list:
            bpy.context.scene.frame_set(f % frames)
            if advance:
                phi = f / float(frames)
                for o in movers:
                    o.location = base_loc[o.name] + d * adv_m * phi
            upd()
            p = cache / f"climb_{tag}_f{f:02d}.png"
            render_still(p)
            pngs.append(p)
        for o in movers:
            o.location = base_loc[o.name]
        upd()
        bpy.data.objects.remove(prox, do_unlink=True)
        out = tile_strip(pngs, cache / f"climb_{tag}_strip.png", cols=len(pngs))
        shots.append(out)
        return out

    # (a) SIDE — the pose against the rung plane. The NEAR stile is omitted so it
    #     does not occlude; the far stile + rungs give the reference.
    strip("side", lambda: body_points(arm), Vector((1.0, -0.20, 0.06)),
          picks, proxy_stiles=(-1,), fill=1.30)

    # (b) BACK three-quarter — read the lean and the reach without looking through
    #     the ladder.
    strip("back34", lambda: body_points(arm), Vector((0.80, 0.70, 0.30)),
          picks[:4], fill=1.28)

    # (c) TRAVERSAL COMPOSITE — the armature is advanced along the pinned line by
    #     adv*phi exactly as TASK-738's traversal will. This is the eyeball form of
    #     the numeric contact gate: the ladder is fixed, the climber ascends.
    strip("traversal",
          lambda: (body_points(arm) + body_points(arm, d * adv_m)),
          Vector((1.0, -0.28, 0.05)), picks, advance=True,
          proxy_stiles=(-1,), fill=1.20)

    # (d) CONTACT PROOF — hand_l grips through frames 4..12 (phase .25 -> .75).
    #     A red marker is pinned at the grip point the numeric gate says the hand
    #     holds; with the traversal advance applied the hand must stay ON the
    #     marker in every tile while the body climbs past it.
    bpy.context.scene.frame_set(8)
    upd()
    hand = arm.pose.bones["hand_l"].head + d * adv_m * 0.5
    marker = make_marker(hand, 0.045)
    strip("contact", lambda: [hand + Vector((x, y, z))
                              for x in (-0.55, 0.55) for y in (-0.55, 0.55)
                              for z in (-0.55, 0.55)],
          Vector((0.60, -0.74, 0.28)), [4, 6, 8, 10, 12], advance=True, fill=1.05)
    bpy.data.objects.remove(marker, do_unlink=True)

    # (e) HERO stills, large, for the pose-quality eyeball gate
    prox = build_ladder_proxy(fr, -0.9, 3.1, stiles=(-1,), name="Prox_hero")
    setup_render(900)
    hero = []
    for f, dirv, tag in ((8, Vector((1.0, -0.20, 0.06)), "side"),
                         (4, Vector((0.85, 0.55, 0.25)), "back"),
                         (12, Vector((0.40, -0.95, 0.22)), "front")):
        bpy.context.scene.frame_set(f)
        upd()
        frame_points(body_points(arm), dirv, fill=1.18)
        p = cache / f"climb_hero_{tag}_f{f:02d}.png"
        render_still(p)
        hero.append(p)
    shots.append(tile_strip(hero, cache / "climb_hero_strip.png", cols=3))
    bpy.data.objects.remove(prox, do_unlink=True)

    bpy.context.scene.frame_set(0)
    return [str(s) for s in shots]


# --------------------------------------------------------------------------- FBX read-back gate
def verify_exported_fbx(frames, adv_m, fr):
    """Re-import the EXPORTED file cold and re-measure it. The export call
    returning success is not evidence — this is."""
    if not OUT_FBX.is_file():
        fail(f"no exported FBX at {OUT_FBX}")
    reset_scene()
    imported = import_fbx(OUT_FBX)
    arms = [o for o in imported if o.type == "ARMATURE"]
    meshes = [o for o in imported if o.type == "MESH"]
    if len(arms) != 1:
        fail(f"exported FBX has {len(arms)} armatures, expected 1")
    arm = arms[0]
    rep = {"file": str(OUT_FBX), "bytes": OUT_FBX.stat().st_size,
           "armature_object": arm.name,
           "mesh_objects": [m.name for m in meshes],
           "bone_count": len(arm.data.bones),
           "bones": sorted(b.name for b in arm.data.bones),
           "scene_fps_after_import": bpy.context.scene.render.fps,
           "frame_range_after_import": [bpy.context.scene.frame_start,
                                        bpy.context.scene.frame_end]}
    ad = arm.animation_data
    act = ad.action if ad else None
    rep["action"] = act.name if act else None
    if act is None:
        fail("exported FBX carries no action")
    kf = sorted({int(round(kp.co[0])) for fc in iter_fcurves(act)
                 for kp in fc.keyframe_points})
    rep["key_frames"] = [min(kf), max(kf)]
    rep["key_count"] = len(kf)
    rep["fcurve_count"] = sum(1 for _ in iter_fcurves(act))

    # pose read-back: bone world positions at every frame, straight off the file
    track = {}
    for f in range(min(kf), max(kf) + 1):
        bpy.context.scene.frame_set(f)
        upd()
        track[f] = {pb.name: pb.head.copy() for pb in arm.pose.bones}
    f0, fN = min(kf), max(kf)
    rep["roundtrip_seam_max_uu"] = max(
        (track[fN][n] - track[f0][n]).length * UE for n in track[f0])

    # Contact: with the traversal advance added back, a gripping limb is static.
    # The cycle is walked TWICE (pose is periodic, advance is not) so a grip window
    # that wraps the seam is one contiguous run instead of two half-runs a full
    # stroke apart — measuring it unwrapped is what makes the seam itself testable.
    d = fr["d"]
    span = fN - f0
    skate = {}
    for limb in ("hand_l", "hand_r", "foot_l", "foot_r"):
        phase_off = 0.0 if limb in ("hand_l", "foot_r") else 0.5
        runs, cur = [], []
        for k in range(2 * span):
            phi = k / float(span)
            mode, _s = limb_state(phi + phase_off)
            if mode == "grip":
                cur.append(track[f0 + (k % span)][limb] + d * adv_m * phi)
            elif cur:
                runs.append(cur); cur = []
        if cur:
            runs.append(cur)
        worst = 0.0
        for run in runs:
            if len(run) < 2:
                continue
            ctr = sum(run, Vector((0, 0, 0))) / len(run)
            worst = max(worst, max((p - ctr).length for p in run) * UE)
        skate[limb] = worst
    rep["roundtrip_contact_skate_uu"] = skate
    rep["roundtrip_contact_skate_max_uu"] = max(skate.values())

    log(f"FBX READ-BACK: bones={rep['bone_count']} action='{rep['action']}' "
        f"keys={rep['key_frames']} ({rep['key_count']}) fcurves={rep['fcurve_count']} "
        f"fps={rep['scene_fps_after_import']} meshes={rep['mesh_objects']}")
    log(f"FBX READ-BACK: seam={rep['roundtrip_seam_max_uu']:.5f} uu  "
        f"contact skate max={rep['roundtrip_contact_skate_max_uu']:.4f} uu")
    out = CACHE / "climb_fbx_readback.json"
    CACHE.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(rep, indent=2), encoding="utf-8")
    log(f"READBACK REPORT: {out}")
    print("READBACK_OK")


# --------------------------------------------------------------------------- main
def main():
    args = parse_args()
    frames = args["frames"]
    adv_uu = args["advance"]
    adv = adv_uu / UE               # EVERYTHING below this line is in METRES

    reset_scene()
    bpy.context.scene.render.fps = FPS
    bpy.context.scene.render.fps_base = 1.0
    bpy.context.scene.unit_settings.system = "METRIC"

    log(f"blender={bpy.app.version_string} rig={RIG_FBX}")
    if args["verify_fbx"]:
        verify_exported_fbx(frames, adv, ladder_frame())
        return
    arm, meshes = load_rig()
    fr = ladder_frame()
    log(f"LINE: len={fr['length_uu']:.3f} uu  lean={fr['lean_deg']:.4f} deg  "
        f"tilt={fr['tilt_deg']:.4f} deg")

    geom = measure(arm, fr, adv)
    log(f"REACH: arm={geom['arm_reach_m']*UE:.1f} uu band={geom['hand_band_m']*UE:.1f} uu | "
        f"leg={geom['leg_reach_m']*UE:.1f} uu band={geom['foot_band_m']*UE:.1f} uu | "
        f"need={geom['need_m']*UE:.1f} uu | headroom hand="
        f"{geom['hand_headroom_m']*UE:+.1f} foot={geom['foot_headroom_m']*UE:+.1f}")
    if args["probe"]:
        print(json.dumps({"geom": {k: v for k, v in geom.items()
                                   if isinstance(v, (int, float))},
                          "line": {k: v for k, v in fr.items()
                                   if isinstance(v, (int, float))}}, indent=2))
        return
    if geom["hand_headroom_m"] < -1e-4 or geom["foot_headroom_m"] < -1e-4:
        WARN.append(f"advance {adv_uu:.1f} uu exceeds the rig's reach budget "
                    f"(hand headroom {geom['hand_headroom_m']*UE:+.2f} uu, "
                    f"foot {geom['foot_headroom_m']*UE:+.2f} uu)")

    action, residual, samples = author(arm, fr, geom, adv, frames)
    log(f"AUTHOR: action='{action.name}' frames=0..{frames} "
        f"max IK residual={residual*UE:.4f} uu")

    rep = verify(action, samples, fr, adv, frames)
    rep["ik_residual_max_uu"] = residual * UE
    log(f"VERIFY: seam pose delta={rep['seam_pose_max_delta']:.3e} "
        f"({rep['seam_pose_worst_channel']}) | seam corner ratio="
        f"{rep['seam_corner_max_ratio']:.3f} (1.0 = seam is the sharpest frame in "
        f"the cycle) | max contact skate={rep['contact_skate_max_uu']:.3f} uu")
    for n, c in rep["seam_corner"].items():
        log(f"   corner {n:9s} seam={c['seam_uu']:.4f} uu  "
            f"max elsewhere={c['max_elsewhere_uu']:.4f} uu @f{c['argmax_frame']}  "
            f"ratio={c['seam_over_max']:.3f}")
    log(f"JOINTS: " + "  ".join(f"{k}={v['min_deg']:.0f}..{v['max_deg']:.0f}"
                                for k, v in sorted(rep["joints"].items())))

    out = export_anim_only(arm, frames, OUT_FBX)
    rep["fbx"] = str(out)
    rep["fbx_bytes"] = out.stat().st_size
    rep["action"] = action.name
    rep["asset"] = "A_SiegeBiped_Climb"
    rep["skeleton"] = "/Game/Characters/SK_Footman_Skeleton"
    rep["geometry"] = {k: (v if isinstance(v, (int, float)) else list(v))
                       for k, v in geom.items()}
    rep["line"] = {k: v for k, v in fr.items() if isinstance(v, (int, float))}
    rep["body_standoff_uu"] = BODY_STANDOFF_M * UE
    rep["root_track"] = "CONSTANT (lean + standoff); no cyclic translation"

    # ---- TASK-733b: the rigid shift onto the shipped rung plane ----------
    # Signs are reported in `m` (the normal pointing AWAY from the tower, the
    # convention TASK-737/739 measured in) = -n, so they read against the mesh.
    rep["rung_plane"] = {
        "offset_along_n_m": RUNG_PLANE_OFFSET_M,
        "grip_plane_m_uu": -RUNG_PLANE_OFFSET_M * UE,          # -22.0
        "body_root_m_uu": (BODY_STANDOFF_M - RUNG_PLANE_OFFSET_M) * UE,   # +2.0
        "body_to_rung_distance_uu": BODY_STANDOFF_M * UE,       # 24.0 — UNCHANGED
        "measured_ladder_near_face_m_uu": -12.0,
        "measured_ladder_far_face_m_uu": -32.0,
        "measured_slab_thickness_uu": 20.0,
        "source": "Content/RawAssets/WatchTower.fbx (TASK-737), measured TASK-733b",
        "note": ("rigid translation along +n: grips and body move together, so the "
                 "authored reach is preserved and no joint angle re-solves"),
    }

    if args["previews"]:
        rep["previews"] = render_previews(arm, meshes, fr, adv, frames, CACHE)

    if args["save_blend"]:
        CACHE.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(CACHE / "climb_debug.blend"))

    rep["warnings"] = WARN
    CACHE.mkdir(parents=True, exist_ok=True)
    (CACHE / "climb_report.json").write_text(json.dumps(rep, indent=2), encoding="utf-8")
    log(f"REPORT: {CACHE / 'climb_report.json'}")
    for w in WARN:
        log(f"WARN: {w}")
    print("CLIMB_OK")


main()
