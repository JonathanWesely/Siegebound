"""
retarget_meshy_to_siegebiped.py — Siegebound art pipeline, TASK-229.

The SANCTIONED Meshy-clip export path (CONVENTIONS "Meshy second engine (M7.5)" ->
"UE-5.8 retarget-export defect + SANCTIONED export path"): UE 5.8's
FIKRetargetBatchOperation exports ROOT-ONLY motion through all three of its doors
(commandlet, editor-python batch API, UI button — evidence handoffs/TASK-221-222.md
S7, foot amplitude 7.07 uu vs source ~57 uu). This tool bypasses UE's exporter
entirely: it retargets the Meshy 24-bone Mixamo-style preset clips onto our 21-bone
SiegeBiped rig IN BLENDER and exports per-clip animation-only FBXs for DIRECT import
onto the existing SK_Footman_Skeleton.

Sibling of rig_character.py. Runs INSIDE Blender's bundled Python (bpy + stdlib
ONLY), HEADLESS (the live Blender MCP bridge has a 30 s socket cap — never use it):

    "<blender.exe>" --background --factory-startup --python-exit-code 1 ^
        --python Tools/ArtPipeline/retarget_meshy_to_siegebiped.py -- --card-id Footman

Flags (after the "--" separator):
    --card-id <ID>          required; the unit to process (Footman, Knight, ...)
    --clips A,B,C           subset of Idle,Walk,Attack,Death (default: all four)
    --check                 validate inputs/environment only (no writes), exit 0/2
    --verify-only           skip retargeting; re-import the EXISTING output FBXs and
                            run the amplitude sampling + Walk gate. Exit 0/1/2.
    --min-walk-foot-uu <f>  Walk-clip foot amplitude floor in UE units (default 40.0
                            — the codified acceptance gate; the UE-5.8 root-only
                            failure signature is ~7 uu)
    --save-blend            save Cache/<CardID>/retarget/<clip>_debug.blend per clip

Inputs:
    Content/RawAssets/Characters/Meshy/<CardID>/<CardID>_<Clip>.fbx   (TASK-223/203)
    Content/RawAssets/Characters/<CardID>.fbx                         (SiegeBiped rig,
        armature OBJECT 'Footman_Rig' post-TASK-212, 21-bone shared hierarchy)

Outputs:
    Content/RawAssets/Characters/MeshyRetargeted/<CardID>/A_<CardID>_<Clip>_meshy.fbx
        animation-only (armature + baked action, NO mesh); export block mirrors
        rig_character.export_anim_fbx exactly (axis_forward=-Z, axis_up=Y,
        apply_unit_scale, FBX_SCALE_NONE, FACE smoothing, no leaf bones,
        primary/secondary bone axis Y/X, path_mode STRIP) with object_types
        {'ARMATURE'} as the single documented deviation (no mesh to export).
    Tools/ArtPipeline/Cache/<CardID>/retarget/retarget_report.json

Retarget method (constraint-based, then baked):
    1. Import the SiegeBiped rig FBX (armature only — mesh deleted) and the Meshy
       clip FBX (armature+action — mesh deleted). Scene fps is pinned to 30 before
       the clip import so FBX key times land on exact source frames (Meshy presets
       are 30 fps; Walk = frames 1..127 = 4.200 s, matching the UE-side evidence).
    2. For every mapped bone pair (CHAIN_MAP below — IK_MeshyBiped's 9 verified
       chains + the Hips->pelvis retarget root, incl. the Meshy spine-REVERSAL
       quirk Spine02=lowest -> Spine=chest), grow a HELPER bone on the source
       armature: parented to the source bone, rest-oriented to
       A @ (target bone's rest world orientation), where
         - 'delta' semantics (pelvis): A = identity — the target keeps its own rest
           orientation and receives the source bone's world-space rotation DELTA
           (the Mixamo Hips bone's geometric direction is arbitrary, so absolute
           direction tracking would be nonsense there);
         - 'aim' semantics (all anatomical chains): A = minimal rotation taking the
           target bone's rest direction onto the source bone's rest direction — the
           target bone then TRACKS the source bone's absolute world direction
           (proportion differences between per-unit Meshy rigs drop out; both rigs
           were fitted to the same mesh so A is small and twist transport is safe).
       A COPY_ROTATION constraint (WORLD->WORLD, REPLACE) on each target pose bone
       follows its helper: helper_world(t) = src_delta(t) @ A @ tgt_rest — exactly
       the FK rotation transfer the UE preview performs.
    3. Translation transfers ONLY on root/pelvis, scaled by the hip-height ratio
       (target pelvis rest Z / source Hips rest Z): the Hips world offset from rest
       is split into a HORIZONTAL part written to the 'root' bone (root motion is
       PRESERVED in the FBX — the UE import decides root-lock; it is never baked
       into the pelvis) and a VERTICAL part written to 'pelvis' (walk/idle bob and
       the death collapse survive a root-lock).
    4. BAKE: every frame is sampled through the evaluated depsgraph, then the
       constraints are STRIPPED and the captured world rotations (+ the computed
       root/pelvis translations) are written back as plain FK keyframes
       (quaternion, hemisphere-continuous) on a fresh action named
       A_<CardID>_<Clip>_meshy. No constraint survives into the export.
    5. VERIFY (the codified acceptance gate): the baked action is sampled in-scene
       AND the exported FBX is re-imported fresh and re-sampled — foot_l/foot_r
       world-position amplitude, 9 uniform samples, per-axis max-min, max over
       axes, in UE units (the TASK-203/221 spike method). The Walk clip must show
       BOTH feet >= --min-walk-foot-uu (default 40) or the run exits 1.

Write confinement (QA-auditable): every write is funneled through OutputGuard.check()
— only Tools/ArtPipeline/Cache/<CardID>/retarget/** and
Content/RawAssets/Characters/MeshyRetargeted/<CardID>/** are writable, and NEVER any
path containing "CardArt" (lane-isolation ruling 4). NOTE: Blender's FBX importer may
extract embedded Meshy textures into a sibling <clip>.fbm/ folder beside the INPUT
FBX (still inside Content/RawAssets/, and those folders already exist from prior
imports); the tool itself writes nothing there.

Exit codes: 0 = OK (gate passed), 1 = stage/gate failure, 2 = usage/input error.
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
    from mathutils import Matrix, Quaternion, Vector
except ImportError:
    print("[retarget] FATAL: bpy not importable — run inside Blender "
          "(blender.exe --background --factory-startup --python "
          "retarget_meshy_to_siegebiped.py -- ...)")
    sys.exit(2)

# --------------------------------------------------------------------------- paths / law
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parents[1]
CACHE_ROOT = SCRIPT_DIR / "Cache"
CHARACTERS_RAW = PROJECT_ROOT / "Content" / "RawAssets" / "Characters"
MESHY_RAW = CHARACTERS_RAW / "Meshy"
RETARGET_OUT_ROOT = CHARACTERS_RAW / "MeshyRetargeted"

UE = 100.0                            # 1 Blender meter = 100 UE units
CLIPS_ALL = ["Idle", "Walk", "Attack", "Death"]
TARGET_FPS = 30                       # Meshy preset clips are 30 fps (probe-verified)
SHARED_SKELETON_ROOT = "Footman_Rig"  # rig_character.SHARED_SKELETON_ROOT (TASK-212)
EXPECTED_TGT_BONES = 21               # SiegeBiped contract
VERIFY_SAMPLES = 9                    # spike method: 9 uniform samples
HELPER_PREFIX = "RT_helper_"

# The documented chain map: IK_MeshyBiped's 9 verified chains (handoffs/TASK-203 +
# TASK-221-222 S3/S7) + the Hips->pelvis retarget root. Meshy spine naming is
# REVERSED: Spine02 = lowest, Spine = chest (TASK-203 quirk, TASK-223-confirmed
# identical across all 8 remaining units). Unmapped source bones: LeftToeBase,
# RightToeBase, head_end, headfront (no SiegeBiped counterpart). Unmapped target
# bone: 'root' (receives the horizontal root-motion translation only).
CHAIN_MAP = [
    # (chain label,        [(src bone, tgt bone, semantics), ...])
    ("RetargetRoot",   [("Hips", "pelvis", "delta")]),
    ("Spine[REVERSED]", [("Spine02", "spine_01", "aim"),
                         ("Spine01", "spine_02", "aim"),
                         ("Spine",   "spine_03", "aim")]),
    ("Neck",           [("neck", "neck_01", "aim")]),
    ("Head",           [("Head", "head", "aim")]),
    ("LeftClavicle",   [("LeftShoulder", "clavicle_l", "aim")]),
    ("RightClavicle",  [("RightShoulder", "clavicle_r", "aim")]),
    ("LeftArm",        [("LeftArm", "upperarm_l", "aim"),
                        ("LeftForeArm", "lowerarm_l", "aim"),
                        ("LeftHand", "hand_l", "aim")]),
    ("RightArm",       [("RightArm", "upperarm_r", "aim"),
                        ("RightForeArm", "lowerarm_r", "aim"),
                        ("RightHand", "hand_r", "aim")]),
    ("LeftLeg",        [("LeftUpLeg", "thigh_l", "aim"),
                        ("LeftLeg", "calf_l", "aim"),
                        ("LeftFoot", "foot_l", "aim")]),
    ("RightLeg",       [("RightUpLeg", "thigh_r", "aim"),
                        ("RightLeg", "calf_r", "aim"),
                        ("RightFoot", "foot_r", "aim")]),
]
BONE_PAIRS = [pair for _, pairs in CHAIN_MAP for pair in pairs]
SRC_ROOT_BONE = "Hips"
TGT_PELVIS = "pelvis"
TGT_ROOT = "root"
AMP_BONES = ["pelvis", "foot_l", "foot_r", "hand_l", "hand_r"]
SRC_AMP_BONES = ["Hips", "LeftFoot", "RightFoot", "LeftHand", "RightHand"]


def log(msg):
    print(f"[retarget] {msg}", flush=True)


def fail(msg, code=1):
    print(f"[retarget] FATAL: {msg}", flush=True)
    sys.exit(code)


class OutputGuard:
    """Write confinement — mirror rig_character.py's guard, retarget lane roots."""

    def __init__(self, card_id):
        self.cache_dir = (CACHE_ROOT / card_id / "retarget").resolve()
        self.out_dir = (RETARGET_OUT_ROOT / card_id).resolve()
        self.allowed = [self.cache_dir, self.out_dir]

    def check(self, path):
        rp = Path(path).resolve()
        if any(part.lower() == "cardart" for part in rp.parts):
            fail(f"write-confinement violation: {rp} is inside a CardArt lane")
        if not any(rp == root or root in rp.parents for root in self.allowed):
            fail(f"write-confinement violation: {rp} outside allowed roots "
                 f"{[str(a) for a in self.allowed]}")
        rp.parent.mkdir(parents=True, exist_ok=True)
        return rp


# --------------------------------------------------------------------------- args
def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    p = argparse.ArgumentParser(prog="retarget_meshy_to_siegebiped.py")
    p.add_argument("--card-id", required=True)
    p.add_argument("--clips", default=",".join(CLIPS_ALL))
    p.add_argument("--check", action="store_true")
    p.add_argument("--verify-only", action="store_true")
    p.add_argument("--min-walk-foot-uu", type=float, default=40.0)
    p.add_argument("--save-blend", action="store_true")
    try:
        return p.parse_args(argv)
    except SystemExit:
        sys.exit(2)


def resolve_clips(arg):
    clips = [c.strip() for c in arg.split(",") if c.strip()]
    bad = [c for c in clips if c not in CLIPS_ALL]
    if bad or not clips:
        fail(f"--clips must be a subset of {CLIPS_ALL}, got {clips}", code=2)
    return [c for c in CLIPS_ALL if c in clips]   # canonical order


def input_paths(card_id, clips):
    rig_fbx = CHARACTERS_RAW / f"{card_id}.fbx"
    clip_fbx = {c: MESHY_RAW / card_id / f"{card_id}_{c}.fbx" for c in clips}
    return rig_fbx, clip_fbx


def run_check(card_id, clips):
    rig_fbx, clip_fbx = input_paths(card_id, clips)
    problems = []
    if not rig_fbx.is_file():
        problems.append(f"SiegeBiped rig FBX missing: {rig_fbx}")
    for c, p in clip_fbx.items():
        if not p.is_file():
            problems.append(f"Meshy {c} clip missing: {p}")
    log(f"--check card-id={card_id} clips={clips}")
    log(f"blender={bpy.app.version_string} rig={rig_fbx}")
    for c, p in clip_fbx.items():
        log(f"clip {c}: {p} {'OK' if p.is_file() else 'MISSING'}")
    if problems:
        for pr in problems:
            log(f"CHECK FAIL: {pr}")
        sys.exit(2)
    log(f"CHECK OK — outputs would go to {RETARGET_OUT_ROOT / card_id} "
        f"+ report to {CACHE_ROOT / card_id / 'retarget'}")
    sys.exit(0)


# --------------------------------------------------------------------------- bpy helpers
def ensure_object_mode():
    a = bpy.context.view_layer.objects.active
    if a is not None and a.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def reset_scene():
    """Factory-empty the scene AND purge datablocks so re-imports never get .001
    suffixes (the armature object MUST export as exactly 'Footman_Rig')."""
    ensure_object_mode()
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
    for coll in (bpy.data.armatures, bpy.data.meshes, bpy.data.materials,
                 bpy.data.images):
        for block in list(coll):
            if block.users == 0:
                coll.remove(block)


def select_only(objs, active=None):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    vl.objects.active = active if active is not None else (objs[0] if objs else None)


def import_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    return [o for o in bpy.data.objects if o not in before]


def bind_action(arm_obj, action):
    """rig_character.bind_action — Blender 4.4+ slotted actions must have the slot
    bound or the fcurves DO NOT EVALUATE (bake/export would see rest pose)."""
    ad = arm_obj.animation_data or arm_obj.animation_data_create()
    ad.action = action
    if action is not None:
        slots = getattr(action, "slots", None)
        if slots is not None and len(slots) > 0:
            try:
                ad.action_slot = slots[0]
            except (AttributeError, TypeError):
                pass


def clear_pose_transforms(arm_obj):
    for pb in arm_obj.pose.bones:
        pb.location = (0, 0, 0)
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.scale = (1, 1, 1)


def world_rot(obj):
    return obj.matrix_world.decompose()[1]


def bone_rest_world_rot(arm_obj, bone_name):
    m = arm_obj.matrix_world @ arm_obj.data.bones[bone_name].matrix_local
    return m.decompose()[1]


def bone_rest_world_dir(arm_obj, bone_name):
    b = arm_obj.data.bones[bone_name]
    h = arm_obj.matrix_world @ b.head_local
    t = arm_obj.matrix_world @ b.tail_local
    d = t - h
    if d.length < 1e-9:
        fail(f"zero-length rest bone '{bone_name}'")
    return d.normalized()


# --------------------------------------------------------------------------- stage: load rigs
def load_target_rig(rig_fbx, warnings):
    imported = import_fbx(rig_fbx)
    arms = [o for o in imported if o.type == "ARMATURE"]
    if len(arms) != 1:
        fail(f"expected exactly 1 armature in {rig_fbx.name}, got {len(arms)}")
    arm = arms[0]
    for o in imported:
        if o is not arm:
            bpy.data.objects.remove(o, do_unlink=True)  # animation-only output: no mesh
    if arm.name != SHARED_SKELETON_ROOT:
        warnings.append(f"target armature imported as '{arm.name}' — renamed to "
                        f"'{SHARED_SKELETON_ROOT}' (TASK-212 shared-root law)")
        arm.name = SHARED_SKELETON_ROOT
    n = len(arm.data.bones)
    if n != EXPECTED_TGT_BONES:
        fail(f"SiegeBiped rig has {n} bones, expected {EXPECTED_TGT_BONES}")
    missing = [t for _, t, _ in BONE_PAIRS if t not in arm.data.bones]
    if TGT_ROOT not in arm.data.bones:
        missing.append(TGT_ROOT)
    if missing:
        fail(f"target rig missing mapped bones: {missing}")
    loc, _rot, sca = arm.matrix_world.decompose()
    if max(abs(s - 1.0) for s in sca) > 1e-4 or loc.length > 1e-4:
        warnings.append(f"target armature object transform not identity "
                        f"(loc={tuple(loc)}, scale={tuple(sca)}) — world-space math "
                        "compensates, but check the source FBX")
    if arm.animation_data:
        bind_action(arm, None)
    clear_pose_transforms(arm)
    return arm


def load_source_clip(clip_fbx, tgt_arm):
    imported = import_fbx(clip_fbx)
    arms = [o for o in imported if o.type == "ARMATURE" and o is not tgt_arm]
    if len(arms) != 1:
        fail(f"expected exactly 1 armature in {clip_fbx.name}, got {len(arms)}")
    arm = arms[0]
    for o in imported:
        if o is not arm:
            bpy.data.objects.remove(o, do_unlink=True)  # constraints only need bones
    missing = [s for s, _, _ in BONE_PAIRS if s not in arm.data.bones]
    if missing:
        fail(f"Meshy rig missing mapped bones: {missing}")
    ad = arm.animation_data
    if ad is None or ad.action is None:
        fail(f"no action on the Meshy armature in {clip_fbx.name}")
    action = ad.action
    bind_action(arm, action)   # re-assert the slot binding explicitly
    fr = action.frame_range
    f0, f1 = int(math.floor(fr[0])), int(math.ceil(fr[1]))
    if f1 <= f0:
        fail(f"degenerate action frame range {fr[:]} in {clip_fbx.name}")
    return arm, action, f0, f1


# --------------------------------------------------------------------------- stage: constraints
def build_retarget(src_arm, tgt_arm, warnings):
    """Helper bones on the source armature + COPY_ROTATION (WORLD->WORLD, REPLACE)
    constraints on the target. Returns {tgt_bone: helper_name}."""
    src_rot = world_rot(src_arm)
    plans = []
    for src_name, tgt_name, mode in BONE_PAIRS:
        tgt_rest_q = bone_rest_world_rot(tgt_arm, tgt_name)
        if mode == "aim":
            a_q = bone_rest_world_dir(tgt_arm, tgt_name).rotation_difference(
                bone_rest_world_dir(src_arm, src_name))
        else:
            a_q = Quaternion((1, 0, 0, 0))
        helper_world_q = a_q @ tgt_rest_q
        helper_arm_q = src_rot.inverted() @ helper_world_q
        plans.append((src_name, tgt_name, helper_arm_q))

    ensure_object_mode()
    select_only([src_arm], active=src_arm)
    bpy.ops.object.mode_set(mode="EDIT")
    eb_all = src_arm.data.edit_bones
    helper_of = {}
    for src_name, tgt_name, helper_arm_q in plans:
        src_eb = eb_all[src_name]
        name = f"{HELPER_PREFIX}{tgt_name}"
        eb = eb_all.new(name)
        eb.head = src_eb.head.copy()
        eb.tail = src_eb.head + Vector((0, 0, 10.0))   # placeholder length, armature units
        eb.matrix = Matrix.LocRotScale(src_eb.head.copy(), helper_arm_q, None)
        eb.parent = src_eb
        eb.use_connect = False
        eb.use_deform = False
        helper_of[tgt_name] = name
    bpy.ops.object.mode_set(mode="OBJECT")

    for _src_name, tgt_name, _q in plans:
        pb = tgt_arm.pose.bones[tgt_name]
        c = pb.constraints.new("COPY_ROTATION")
        c.name = "RT_copy_rot"
        c.target = src_arm
        c.subtarget = helper_of[tgt_name]
        c.target_space = "WORLD"
        c.owner_space = "WORLD"
        c.mix_mode = "REPLACE"
        c.influence = 1.0
    log(f"CONSTRAIN: {len(plans)} helper bones + copy-rotation constraints "
        f"({sum(1 for *_, m in BONE_PAIRS if m == 'aim')} aim / "
        f"{sum(1 for *_, m in BONE_PAIRS if m == 'delta')} delta)")
    return helper_of


def strip_constraints(tgt_arm):
    n = 0
    for pb in tgt_arm.pose.bones:
        for c in list(pb.constraints):
            pb.constraints.remove(c)
            n += 1
    return n


# --------------------------------------------------------------------------- stage: capture + bake
def capture_frames(scene, src_arm, tgt_arm, f0, f1):
    """Per frame: every mapped target bone's constrained WORLD rotation + the source
    Hips world position (evaluated depsgraph — constraints applied)."""
    mapped = [t for _, t, _ in BONE_PAIRS]
    rots = {t: [] for t in mapped}
    hips_pos = []
    for f in range(f0, f1 + 1):
        scene.frame_set(f)
        dg = bpy.context.evaluated_depsgraph_get()
        tgt_eval = tgt_arm.evaluated_get(dg)
        src_eval = src_arm.evaluated_get(dg)
        tw = tgt_eval.matrix_world
        for t in mapped:
            rots[t].append((tw @ tgt_eval.pose.bones[t].matrix).decompose()[1])
        hips_pos.append((src_eval.matrix_world @
                         src_eval.pose.bones[SRC_ROOT_BONE].matrix).to_translation())
    return rots, hips_pos


def hierarchy_order(arm_obj):
    order, queue = [], [b for b in arm_obj.data.bones if b.parent is None]
    while queue:
        b = queue.pop(0)
        order.append(b.name)
        queue.extend(b.children)
    return order


def write_bake(tgt_arm, rots, hips_pos, src_arm, f0, f1, action_name):
    """Strip constraints, then write the captured world rotations + the scaled
    root/pelvis translation transfer as plain FK keyframes on a fresh action."""
    mapped = set(rots.keys())
    rest = {b.name: b.matrix_local.copy() for b in tgt_arm.data.bones}
    parent = {b.name: (b.parent.name if b.parent else None)
              for b in tgt_arm.data.bones}
    order = hierarchy_order(tgt_arm)
    tgt_obj_rot = world_rot(tgt_arm)
    tgt_obj_rot_inv = tgt_obj_rot.inverted()

    # hip-height ratio: target pelvis rest world Z / source Hips rest world Z
    src_hips_rest = (src_arm.matrix_world @
                     src_arm.data.bones[SRC_ROOT_BONE].matrix_local).to_translation()
    tgt_pelvis_rest_z = (tgt_arm.matrix_world @
                         tgt_arm.data.bones[TGT_PELVIS].matrix_local).to_translation().z
    if abs(src_hips_rest.z) < 1e-6:
        fail("source Hips rest height is ~0 — cannot scale translation")
    hip_ratio = tgt_pelvis_rest_z / src_hips_rest.z

    n = strip_constraints(tgt_arm)
    log(f"BAKE: stripped {n} constraints; hip_ratio={hip_ratio:.4f} "
        f"(tgt pelvis {tgt_pelvis_rest_z:.4f} m / src Hips {src_hips_rest.z:.4f} m)")

    action = bpy.data.actions.new(action_name)
    bind_action(tgt_arm, action)
    clear_pose_transforms(tgt_arm)
    pbones = {name: tgt_arm.pose.bones[name] for name in order}
    prev_q = {}
    max_travel = 0.0
    for i, f in enumerate(range(f0, f1 + 1)):
        d = (hips_pos[i] - src_hips_rest) * hip_ratio
        max_travel = max(max_travel, Vector((d.x, d.y, 0.0)).length)
        horiz_arm = tgt_obj_rot_inv @ Vector((d.x, d.y, 0.0))
        vert_arm = tgt_obj_rot_inv @ Vector((0.0, 0.0, d.z))
        pose_mat = {}
        for name in order:
            pb = pbones[name]
            par = parent[name]
            if name == TGT_ROOT:
                m = Matrix.Translation(horiz_arm) @ rest[name]
                basis = rest[name].inverted() @ m
            else:
                if par is None or par not in pose_mat:
                    fail(f"hierarchy walk broke at '{name}' (parent {par!r})")
                k = pose_mat[par] @ rest[par].inverted() @ rest[name]
                if name in mapped:
                    r_arm = tgt_obj_rot_inv @ rots[name][i]
                else:   # unmapped non-root bone (none on SiegeBiped today): hold rest
                    r_arm = k.decompose()[1]
                pos = k.to_translation()
                if name == TGT_PELVIS:
                    pos = pos + vert_arm
                m = Matrix.LocRotScale(pos, r_arm, None)
                basis = k.inverted() @ m
            q = basis.to_quaternion().normalized()
            if name in prev_q and prev_q[name].dot(q) < 0.0:
                q.negate()
            prev_q[name] = q.copy()
            pb.rotation_quaternion = q
            pb.keyframe_insert("rotation_quaternion", frame=f)
            if name in (TGT_ROOT, TGT_PELVIS):
                pb.location = basis.to_translation()
                pb.keyframe_insert("location", frame=f)
            pose_mat[name] = m
    log(f"BAKE: {f1 - f0 + 1} frames x {len(order)} bones -> action '{action.name}' "
        f"(max horizontal root travel {max_travel * UE:.1f} uu)")
    return action, hip_ratio, max_travel * UE


# --------------------------------------------------------------------------- stage: verify
def sample_amplitudes(arm_obj, f0, f1, bone_names):
    """The spike method: 9 uniform samples, world position per bone, per-axis
    max-min, max over axes, in UE units."""
    scene = bpy.context.scene
    pos = {b: [] for b in bone_names}
    for i in range(VERIFY_SAMPLES):
        t = f0 + (f1 - f0) * i / (VERIFY_SAMPLES - 1)
        scene.frame_set(int(t), subframe=t - int(t))
        dg = bpy.context.evaluated_depsgraph_get()
        arm_eval = arm_obj.evaluated_get(dg)
        for b in bone_names:
            if b in arm_eval.pose.bones:
                pos[b].append((arm_eval.matrix_world @
                               arm_eval.pose.bones[b].matrix).to_translation())
    amps = {}
    for b, pts in pos.items():
        if not pts:
            amps[b] = None
            continue
        amp = 0.0
        for axis in range(3):
            vals = [p[axis] for p in pts]
            amp = max(amp, max(vals) - min(vals))
        amps[b] = round(amp * UE, 2)
    return amps


def verify_artifact(fbx_path):
    """Re-import an exported FBX into a factory-empty scene and sample it — the
    artifact-level half of the Blender gate."""
    reset_scene()
    bpy.context.scene.render.fps = TARGET_FPS
    imported = import_fbx(fbx_path)
    arms = [o for o in imported if o.type == "ARMATURE"]
    if len(arms) != 1:
        fail(f"artifact verify: expected 1 armature in {fbx_path.name}, "
             f"got {len(arms)}")
    arm = arms[0]
    ad = arm.animation_data
    if ad is None or ad.action is None:
        fail(f"artifact verify: no action imported from {fbx_path.name}")
    bind_action(arm, ad.action)
    fr = ad.action.frame_range
    f0, f1 = int(math.floor(fr[0])), int(math.ceil(fr[1]))
    amps = sample_amplitudes(arm, f0, f1, AMP_BONES)
    n_bones = len(arm.data.bones)
    return amps, f0, f1, n_bones, arm.name


# --------------------------------------------------------------------------- stage: export
def export_anim_only_fbx(tgt_arm, f0, f1, out_path, guard):
    """Animation-only export. Mirrors rig_character.export_anim_fbx VERBATIM except
    object_types={'ARMATURE'} — there is deliberately no mesh in the file."""
    out = guard.check(out_path)
    scene = bpy.context.scene
    scene.frame_start = f0
    scene.frame_end = f1
    ensure_object_mode()
    select_only([tgt_arm], active=tgt_arm)
    bpy.ops.export_scene.fbx(
        filepath=str(out), use_selection=True,
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
    log(f"EXPORT: {out}")
    return out


# --------------------------------------------------------------------------- per-clip pipeline
def process_clip(card_id, clip, rig_fbx, clip_fbx, guard, save_blend, warnings):
    reset_scene()
    scene = bpy.context.scene
    scene.render.fps = TARGET_FPS
    tgt_arm = load_target_rig(rig_fbx, warnings)
    scene.render.fps = TARGET_FPS          # re-pin: the rig FBX carries 24 fps
    src_arm, action, f0, f1 = load_source_clip(clip_fbx, tgt_arm)
    source_action_name = action.name
    fps = scene.render.fps
    if fps != TARGET_FPS:
        warnings.append(f"{clip}: importer moved scene fps to {fps} — kept (source "
                        "timing preserved)")
    seconds = (f1 - f0) / fps
    log(f"CLIP {clip}: '{source_action_name}' frames {f0}..{f1} @ {fps} fps "
        f"= {seconds:.3f} s")

    src_amps = sample_amplitudes(src_arm, f0, f1, SRC_AMP_BONES)
    log(f"VERIFY (source, Meshy rig): " +
        " ".join(f"{b}={v}" for b, v in src_amps.items()) + " uu")

    build_retarget(src_arm, tgt_arm, warnings)
    rots, hips_pos = capture_frames(scene, src_arm, tgt_arm, f0, f1)
    action_name = f"A_{card_id}_{clip}_meshy"
    baked_action, hip_ratio, root_travel_uu = write_bake(
        tgt_arm, rots, hips_pos, src_arm, f0, f1, action_name)

    baked_amps = sample_amplitudes(tgt_arm, f0, f1, AMP_BONES)
    log(f"VERIFY (baked, in-scene): " +
        " ".join(f"{b}={v}" for b, v in baked_amps.items()) + " uu")

    if save_blend:
        blend_out = guard.check(guard.cache_dir / f"{clip.lower()}_debug.blend")
        bpy.ops.wm.save_as_mainfile(filepath=str(blend_out))
        log(f"saved debug blend -> {blend_out}")

    out_fbx = export_anim_only_fbx(
        tgt_arm, f0, f1, guard.out_dir / f"{action_name}.fbx", guard)

    art_amps, af0, af1, n_bones, arm_name = verify_artifact(out_fbx)
    log(f"VERIFY (artifact, re-imported): " +
        " ".join(f"{b}={v}" for b, v in art_amps.items()) +
        f" uu | frames {af0}..{af1}, {n_bones} bones, armature '{arm_name}'")
    if arm_name.split(".")[0] != SHARED_SKELETON_ROOT:
        warnings.append(f"{clip}: re-imported armature object is '{arm_name}' "
                        f"(expected '{SHARED_SKELETON_ROOT}')")
    if n_bones != EXPECTED_TGT_BONES:
        warnings.append(f"{clip}: artifact has {n_bones} bones "
                        f"(expected {EXPECTED_TGT_BONES})")

    return {
        "clip": clip,
        "source_action": source_action_name,
        "frames": [f0, f1],
        "fps": fps,
        "seconds": round(seconds, 3),
        "hip_ratio": round(hip_ratio, 4),
        "root_travel_uu": round(root_travel_uu, 1),
        "amplitude_uu_source": src_amps,
        "amplitude_uu_baked": baked_amps,
        "amplitude_uu_artifact": art_amps,
        "output_fbx": str(out_fbx),
        "artifact_bones": n_bones,
    }


def apply_walk_gate(results, min_walk_foot_uu, warnings):
    """The codified acceptance gate: Walk foot amplitude >= floor on BOTH the baked
    action and the re-imported artifact (7.07 uu root-only signature = fail)."""
    walk = next((r for r in results if r["clip"] == "Walk"), None)
    if walk is None:
        warnings.append("Walk clip not processed — amplitude gate NOT evaluated")
        return True, "SKIPPED (no Walk clip in this run)"
    checks = []
    ok = True
    for stage in ("amplitude_uu_baked", "amplitude_uu_artifact"):
        for foot in ("foot_l", "foot_r"):
            v = walk[stage].get(foot)
            passed = v is not None and v >= min_walk_foot_uu
            ok = ok and passed
            checks.append(f"{stage.split('_')[-1]}.{foot}={v}"
                          f"{'>=' if passed else '<'}{min_walk_foot_uu}")
    verdict = ("PASS" if ok else "FAIL") + " — " + ", ".join(checks)
    return ok, verdict


# --------------------------------------------------------------------------- verify-only mode
def run_verify_only(card_id, clips, guard, min_walk_foot_uu, report):
    results = []
    for clip in clips:
        out_fbx = guard.out_dir / f"A_{card_id}_{clip}_meshy.fbx"
        if not out_fbx.is_file():
            fail(f"--verify-only: output missing (run the retarget first): {out_fbx}",
                 code=2)
        amps, f0, f1, n_bones, arm_name = verify_artifact(out_fbx)
        log(f"VERIFY {clip}: " + " ".join(f"{b}={v}" for b, v in amps.items()) +
            f" uu | frames {f0}..{f1}, {n_bones} bones")
        results.append({
            "clip": clip, "frames": [f0, f1],
            "amplitude_uu_baked": amps,      # artifact IS the only source here
            "amplitude_uu_artifact": amps,
            "output_fbx": str(out_fbx), "artifact_bones": n_bones,
        })
    return results


# --------------------------------------------------------------------------- main
def main():
    args = parse_args()
    card_id = args.card_id
    clips = resolve_clips(args.clips)
    if args.check:
        run_check(card_id, clips)   # exits

    rig_fbx, clip_fbx = input_paths(card_id, clips)
    if not rig_fbx.is_file():
        fail(f"SiegeBiped rig FBX not found: {rig_fbx}", code=2)
    for c, p in clip_fbx.items():
        if not p.is_file():
            fail(f"Meshy {c} clip not found: {p}", code=2)

    guard = OutputGuard(card_id)
    warnings = []
    report = {
        "card_id": card_id,
        "clips": clips,
        "mode": "verify-only" if args.verify_only else "retarget",
        "blender_version": bpy.app.version_string,
        "rig_fbx": str(rig_fbx),
        "chain_map": {label: [f"{s} -> {t} ({m})" for s, t, m in pairs]
                      for label, pairs in CHAIN_MAP},
        "min_walk_foot_uu": args.min_walk_foot_uu,
        "verify_method": f"{VERIFY_SAMPLES}-sample world-position max-axis amplitude "
                         "(TASK-203/221 spike method), UE units",
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "warnings": warnings,
    }
    started = time.time()
    log(f"card-id={card_id} clips={clips} mode={report['mode']} "
        f"blender={bpy.app.version_string}")

    if args.verify_only:
        results = run_verify_only(card_id, clips, guard, args.min_walk_foot_uu, report)
    else:
        results = [process_clip(card_id, c, rig_fbx, clip_fbx[c], guard,
                                args.save_blend, warnings)
                   for c in clips]

    gate_ok, gate_verdict = apply_walk_gate(results, args.min_walk_foot_uu, warnings)
    report["results"] = results
    report["walk_gate"] = gate_verdict
    report["elapsed_seconds"] = round(time.time() - started, 1)

    report_path = guard.check(guard.cache_dir / "retarget_report.json")
    with open(report_path, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
    log(f"REPORT: {report_path}")
    for w in warnings:
        log(f"WARNING: {w}")
    log(f"WALK GATE: {gate_verdict}")
    log(f"DONE in {report['elapsed_seconds']}s — "
        + "; ".join(f"{r['clip']}: foot_l {r['amplitude_uu_artifact'].get('foot_l')} / "
                    f"foot_r {r['amplitude_uu_artifact'].get('foot_r')} uu"
                    for r in results))
    if not gate_ok:
        fail("Walk foot-amplitude gate FAILED (see WALK GATE line)")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        fail("unhandled exception (see traceback above)")
    sys.exit(0)
