"""
rig_character.py — Siegebound art pipeline, Stage 3b (SKELETAL RIG + ANIM, TASK-160).

Sibling of refine_trellis_glb.py. Runs INSIDE Blender's bundled Python (bpy + numpy +
stdlib ONLY). Heavy work runs HEADLESS (the live Blender MCP bridge has a 30 s socket
cap — this pipeline is FAR over it, so it is background-only, same law as refine).

Invocation (from the repo root):

    "<blender.exe>" --background --factory-startup --python-exit-code 1 ^
        --python Tools/ArtPipeline/rig_character.py -- --card-id Footman

Flags (after the "--" separator):
    --card-id <ID>      required; must be a key of rig_manifest.json "assets"
    --manifest <path>   rig-manifest override (default: rig_manifest.json beside this script)
    --input <path>      game-ready mesh override (.fbx). Default: the asset's "input_fbx".
    --smoke             WRITE CONFINEMENT: FBX/preview writes go to Cache/<CardID>/rig/smoke/**
                        instead of Content/RawAssets/Characters/** (shipping raw assets untouched)
    --quick             fast iteration: smaller previews, fewer turntable frames
    --no-anim-fbx       skip the per-anim FBX exports (mesh + previews only)
    --save-blend        also save Cache/<CardID>/rig/rig_debug.blend for inspection

Pipeline (takes a GAME-READY textured mesh -> UE-importable skeletal mesh + anims):
    1. IMPORT   the game-ready SM_<CardID>.fbx; PRESERVE the two-slot material contract
                [0 TeamRegion, 1 <CardID>PBR] + the "UVMap" layer VERBATIM (this SK keeps
                the same materials as its SM source — CONVENTIONS skeletal law).
    2. MEASURE  adaptive anchors from the mesh (height, per-region half-widths) so one
                normalized skeleton spec fits any humanoid silhouette.
    3. ARMATURE build the shared 'SiegeBiped' skeleton (21 bones, UE-style names) fitted
                to the measured anchors x normalized proportions.
    4. SKIN     parent mesh->armature; bone-heat automatic weights, with a deterministic
                segment-distance ENVELOPE fallback (never fails — generalizes to the batch).
    5. ANIMATE  author Idle / Walk / Attack / Death actions (pose-bone keyframes computed
                in WORLD axes via each bone's rest matrix, so motion is orientation-correct
                regardless of bone roll). Attack style + weapon side from the manifest.
    6. EXPORT   rigged mesh FBX -> Content/RawAssets/Characters/<CardID>.fbx (bind/rest pose,
                armature+mesh, axis contract IDENTICAL to the static pipeline so SK faces the
                same way as SM); one anim FBX per action -> Characters/Anims/<CardID>_<Action>.fbx.
    7. PREVIEW  EEVEE (fallback Workbench): a bind-pose turntable mp4 + a per-anim mp4 +
                bind stills (front/side/3-4) + a per-anim contact-sheet PNG, to Cache/<CardID>/rig/.
    8. REPORT   rig_report.json (bones, skinning method + unweighted %, per-action frame
                ranges, material/UV readback, preview paths, warnings).

The AM_<CardID>_Attack MONTAGE is NOT exported here — it is authored in-editor at TASK-162
from the imported A_<CardID>_Attack sequence (a montage wraps a sequence). Editor import of
SK_<CardID> + the anims + ABP_<CardID> is TASK-162 (after the TASK-161 eyeball gate) — this
script NEVER touches the Unreal editor.

Write confinement (QA-auditable): every output path is funneled through OutputGuard.check(),
which hard-fails unless the path is under Tools/ArtPipeline/Cache/<CardID>/ or (when not
--smoke) Content/RawAssets/Characters/ — and NEVER under any path containing "CardArt"
(lane-isolation ruling 4).

Exit codes: 0 = OK, 1 = stage failure, 2 = usage/manifest/input error.
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
    from mathutils import Matrix, Vector, Quaternion
except ImportError:
    print("[rig] FATAL: bpy not importable — run inside Blender "
          "(blender.exe --background --factory-startup --python rig_character.py -- ...)")
    sys.exit(2)

try:
    import numpy as np
except ImportError:
    print("[rig] FATAL: numpy not importable (ships with Blender's Python).")
    sys.exit(2)

# --------------------------------------------------------------------------- paths
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parents[1]
CACHE_ROOT = SCRIPT_DIR / "Cache"
CONTENT_RAW = PROJECT_ROOT / "Content" / "RawAssets"
CHARACTERS_RAW = CONTENT_RAW / "Characters"

UE = 100.0  # 1 Blender meter = 100 UE units
X_AXIS, Y_AXIS, Z_AXIS = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))


def log(msg):
    print(f"[rig] {msg}", flush=True)


def fail(msg, code=1):
    print(f"[rig] FATAL: {msg}", flush=True)
    sys.exit(code)


class OutputGuard:
    """Write confinement — mirror refine_trellis_glb.py's guard."""

    def __init__(self, card_id, smoke):
        self.cache_dir = (CACHE_ROOT / card_id / "rig").resolve()
        self.smoke = smoke
        self.allowed = [self.cache_dir]
        if not smoke:
            self.allowed.append(CHARACTERS_RAW.resolve())

    def check(self, path):
        rp = Path(path).resolve()
        if any(part.lower() == "cardart" for part in rp.parts):
            fail(f"write-confinement violation: {rp} is inside a CardArt lane")
        if not any(rp == root or root in rp.parents for root in self.allowed):
            fail(f"write-confinement violation: {rp} outside allowed roots "
                 f"{[str(a) for a in self.allowed]}")
        rp.parent.mkdir(parents=True, exist_ok=True)
        return rp


# --------------------------------------------------------------------------- args / manifest
def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    p = argparse.ArgumentParser(prog="rig_character.py")
    p.add_argument("--card-id", required=True)
    p.add_argument("--manifest", default=None)
    p.add_argument("--input", default=None)
    p.add_argument("--smoke", action="store_true")
    p.add_argument("--quick", action="store_true")
    p.add_argument("--no-anim-fbx", action="store_true")
    p.add_argument("--save-blend", action="store_true")
    return p.parse_args(argv)


def load_manifest(path):
    if not path.is_file():
        fail(f"rig manifest not found: {path}", code=2)
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except json.JSONDecodeError as err:
        fail(f"rig manifest is not valid JSON: {path}: {err}", code=2)


def merged_params(manifest, card_id):
    assets = manifest.get("assets", {})
    if card_id not in assets:
        fail(f"card-id '{card_id}' not in rig manifest; available: {sorted(assets)}", code=2)
    params = dict(manifest.get("defaults", {}))
    for key, value in assets[card_id].items():
        if isinstance(value, dict) and isinstance(params.get(key), dict):
            merged = dict(params[key]); merged.update(value); params[key] = merged
        else:
            params[key] = value
    skel_name = params.get("skeleton", "SiegeBiped")
    skels = manifest.get("skeletons", {})
    if skel_name not in skels:
        fail(f"skeleton '{skel_name}' not in rig manifest 'skeletons'", code=2)
    params["_skeleton_spec"] = skels[skel_name]
    params["_skeleton_name"] = skel_name
    return params


# --------------------------------------------------------------------------- bpy helpers
def select_only(objs, active=None):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    vl.objects.active = active if active is not None else (objs[0] if objs else None)


def ensure_object_mode():
    a = bpy.context.view_layer.objects.active
    if a is not None and a.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def reset_scene():
    ensure_object_mode()
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)


def verts_np(obj):
    m = obj.data
    n = len(m.vertices)
    buf = np.empty(n * 3, dtype=np.float32)
    m.vertices.foreach_get("co", buf)
    return buf.reshape(-1, 3)


# --------------------------------------------------------------------------- stage: import
def import_mesh(input_path, card_id, report):
    before = set(bpy.data.objects)
    if input_path.suffix.lower() != ".fbx":
        fail(f"expected a .fbx game-ready mesh, got {input_path.suffix}", code=2)
    bpy.ops.import_scene.fbx(filepath=str(input_path))
    imported = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in imported if o.type == "MESH" and not o.name.upper().startswith("UCX_")]
    junk = [o for o in imported if o not in meshes]
    if not meshes:
        fail("game-ready FBX contained no usable mesh")
    if len(meshes) > 1:
        ensure_object_mode()
        select_only(meshes, active=meshes[0])
        bpy.ops.object.join()
        mesh = bpy.context.view_layer.objects.active
    else:
        mesh = meshes[0]
    for o in junk:
        bpy.data.objects.remove(o, do_unlink=True)
    # apply any import transform into the data so measured space == mesh space
    mw = mesh.matrix_world.copy()
    mesh.data.transform(mw)
    mesh.matrix_world = Matrix.Identity(4)
    mesh.name = f"SK_{card_id}"
    mesh.data.name = f"SK_{card_id}"

    # PRESERVE the two-slot contract + UVMap verbatim; re-assert names if Blender suffixed.
    mats = mesh.data.materials
    slot_names = [m.name if m else None for m in mats]
    pbr_name = f"{card_id}PBR"
    if len(mats) >= 1 and mats[0] and mats[0].name != "TeamRegion":
        mats[0].name = "TeamRegion"
    if len(mats) >= 2 and mats[1] and mats[1].name != pbr_name:
        mats[1].name = pbr_name
    if mesh.data.uv_layers:
        mesh.data.uv_layers[0].name = "UVMap"
        mesh.data.uv_layers.active_index = 0
    mesh.data.calc_loop_triangles()
    report["import"] = {
        "input": str(input_path),
        "object": mesh.name,
        "tris": len(mesh.data.loop_triangles),
        "verts": len(mesh.data.vertices),
        "material_slots_in": slot_names,
        "material_slots": [m.name if m else None for m in mats],
        "uv_layer": mesh.data.uv_layers[0].name if mesh.data.uv_layers else None,
    }
    log(f"IMPORT: {mesh.name} {len(mesh.data.loop_triangles)} tris, "
        f"slots {[m.name if m else None for m in mats]}, "
        f"uv {mesh.data.uv_layers[0].name if mesh.data.uv_layers else None}")
    if [m.name if m else None for m in mats][:2] != ["TeamRegion", pbr_name]:
        report["warnings"].append(
            f"material slots {[m.name if m else None for m in mats]} != expected "
            f"['TeamRegion','{pbr_name}'] — check the source FBX")
    return mesh


# --------------------------------------------------------------------------- stage: measure anchors
def measure_anchors(mesh, report):
    v = verts_np(mesh)
    mn, mx = v.min(axis=0), v.max(axis=0)
    height = float(mx[2] - mn[2])
    base_z = float(mn[2])

    def half_width_at(z_lo_frac, z_hi_frac):
        lo = base_z + z_lo_frac * height
        hi = base_z + z_hi_frac * height
        band = v[(v[:, 2] >= lo) & (v[:, 2] <= hi)]
        if len(band) < 8:
            return None
        return float(max(abs(band[:, 0].min()), abs(band[:, 0].max())))

    overall_half = float(max(abs(mn[0]), abs(mx[0])))
    shoulder_half = half_width_at(0.76, 0.84) or overall_half * 0.85
    hip_half = half_width_at(0.46, 0.54) or overall_half * 0.75
    depth = float(mx[1] - mn[1])
    anchors = {
        "height_m": height, "base_z": base_z, "depth_m": depth,
        "shoulder_half_m": shoulder_half, "hip_half_m": hip_half,
        "overall_half_m": overall_half,
    }
    report["anchors_ue"] = {
        "height": round(height * UE, 2), "depth": round(depth * UE, 2),
        "shoulder_half": round(shoulder_half * UE, 2), "hip_half": round(hip_half * UE, 2),
    }
    log(f"MEASURE: height {height*UE:.1f} shoulder_half {shoulder_half*UE:.1f} "
        f"hip_half {hip_half*UE:.1f} depth {depth*UE:.1f} (UE)")
    return anchors


# --------------------------------------------------------------------------- stage: armature
def build_armature(anchors, spec, card_id, report):
    """Build the SiegeBiped skeleton fitted to measured anchors. _l bones at +X
    (character LEFT), _r at -X (character RIGHT = -X weapon side). Front is -Y."""
    P = spec["proportions"]
    H = anchors["height_m"]
    base = anchors["base_z"]
    sh = anchors["shoulder_half_m"]
    hh = anchors["hip_half_m"]

    def z(frac):
        return base + frac * H

    hip_x = hh * P["hip_x_frac"]
    foot_x = hh * P["foot_x_frac"]
    clav_x = sh * P["clavicle_root_x_frac"]
    sho_x = sh * P["shoulder_x_frac"]
    elb_x = sh * P["elbow_x_frac"]
    wri_x = sh * P["wrist_x_frac"]
    foot_fwd = P["foot_forward_frac"] * H  # -Y front

    # (name, parent, head, tail, deform, connected)
    spine_top = z(P["spine3_top_z"])
    bones = [
        ("root",     None,      (0, 0, 0),            (0, 0, z(0.06)),           False, False),
        ("pelvis",   "root",    (0, 0, z(P["pelvis_z"])), (0, 0, z(P["spine1_top_z"])), True, False),
        ("spine_01", "pelvis",  (0, 0, z(P["spine1_top_z"])), (0, 0, z(P["spine2_top_z"])), True, True),
        ("spine_02", "spine_01",(0, 0, z(P["spine2_top_z"])), (0, 0, spine_top),         True, True),
        ("spine_03", "spine_02",(0, 0, spine_top),   (0, 0, z(P["neck_top_z"])),         True, True),
        ("neck_01",  "spine_03",(0, 0, z(P["neck_top_z"])), (0, 0, z(0.905)),            True, True),
        ("head",     "neck_01", (0, 0, z(0.905)),    (0, 0, z(P["head_top_z"])),         True, True),
    ]
    for side, sx in (("l", 1.0), ("r", -1.0)):
        bones += [
            (f"clavicle_{side}", "spine_03",
             (sx * clav_x, 0, spine_top), (sx * sho_x, 0, z(P["shoulder_z"])), True, False),
            (f"upperarm_{side}", f"clavicle_{side}",
             (sx * sho_x, 0, z(P["shoulder_z"])), (sx * elb_x, 0, z(P["elbow_z"])), True, True),
            (f"lowerarm_{side}", f"upperarm_{side}",
             (sx * elb_x, 0, z(P["elbow_z"])), (sx * wri_x, 0, z(P["wrist_z"])), True, True),
            (f"hand_{side}", f"lowerarm_{side}",
             (sx * wri_x, 0, z(P["wrist_z"])), (sx * wri_x, 0, z(P["hand_z"])), True, True),
            (f"thigh_{side}", "pelvis",
             (sx * hip_x, 0, z(P["hip_z"])), (sx * foot_x, 0, z(P["knee_z"])), True, False),
            (f"calf_{side}", f"thigh_{side}",
             (sx * foot_x, 0, z(P["knee_z"])), (sx * foot_x, 0, z(P["ankle_z"])), True, True),
            (f"foot_{side}", f"calf_{side}",
             (sx * foot_x, 0, z(P["ankle_z"])), (sx * foot_x, -foot_fwd, z(P["foot_z"])), True, False),
        ]

    arm_data = bpy.data.armatures.new(f"{card_id}_Armature")
    arm_obj = bpy.data.objects.new(f"{card_id}_Rig", arm_data)
    bpy.context.scene.collection.objects.link(arm_obj)
    select_only([arm_obj], active=arm_obj)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm_data.edit_bones
    deform = {}
    for name, parent, head, tail, is_deform, conn in bones:
        b = eb.new(name)
        b.head = Vector(head)
        b.tail = Vector(tail)
        b.roll = 0.0
        b.use_deform = is_deform
        deform[name] = is_deform
        if parent:
            b.parent = eb[parent]
            b.use_connect = conn
    bpy.ops.object.mode_set(mode="OBJECT")
    report["armature"] = {
        "name": arm_obj.name, "bone_count": len(bones),
        "deform_bones": [n for n, d in deform.items() if d],
        "skeleton": card_id,
    }
    log(f"ARMATURE: {len(bones)} bones ({sum(deform.values())} deform)")
    return arm_obj


# --------------------------------------------------------------------------- stage: skin
def _segment_distance(p, a, b):
    ab = b - a
    denom = ab.dot(ab)
    t = 0.0 if denom < 1e-12 else max(0.0, min(1.0, (p - a).dot(ab) / denom))
    return (p - (a + ab * t)).length


def envelope_weights(mesh, arm_obj, params, report):
    """Deterministic segment-distance envelope skinning — never fails."""
    sk = params.get("skinning", {})
    power = float(sk.get("envelope_power", 2.5))
    max_bones = int(sk.get("max_bones_per_vert", 4))
    segs = []
    for b in arm_obj.data.bones:
        if b.use_deform:
            segs.append((b.name, b.head_local.copy(), b.tail_local.copy()))
    for vg in list(mesh.vertex_groups):
        mesh.vertex_groups.remove(vg)
    groups = {name: mesh.vertex_groups.new(name=name) for name, _, _ in segs}
    v = verts_np(mesh)
    for vi in range(len(v)):
        p = Vector((float(v[vi, 0]), float(v[vi, 1]), float(v[vi, 2])))
        dist = [(name, _segment_distance(p, a, b)) for name, a, b in segs]
        dist.sort(key=lambda t: t[1])
        picks = dist[:max_bones]
        weights = [(n, 1.0 / (d ** power + 1e-6)) for n, d in picks]
        total = sum(w for _, w in weights) or 1.0
        for n, w in weights:
            groups[n].add([vi], w / total, "REPLACE")
    if not any(m.type == "ARMATURE" for m in mesh.modifiers):
        mod = mesh.modifiers.new("Armature", "ARMATURE")
        mod.object = arm_obj
    else:
        for m in mesh.modifiers:
            if m.type == "ARMATURE":
                m.object = arm_obj
    mesh.parent = arm_obj
    return "envelope"


def unweighted_fraction(mesh):
    n = len(mesh.data.vertices)
    if n == 0:
        return 1.0
    weighted = np.zeros(n, dtype=bool)
    for vi, vert in enumerate(mesh.data.vertices):
        if any(g.weight > 1e-4 for g in vert.groups):
            weighted[vi] = True
    return 1.0 - float(weighted.sum()) / n


def skin(mesh, arm_obj, params, report):
    sk = params.get("skinning", {})
    method = sk.get("method", "auto_then_envelope")
    used = None
    frac = 1.0
    if method in ("auto_then_envelope", "auto"):
        try:
            ensure_object_mode()
            select_only([mesh, arm_obj], active=arm_obj)
            bpy.ops.object.parent_set(type="ARMATURE_AUTO")
            frac = unweighted_fraction(mesh)
            thresh = float(sk.get("unweighted_fallback_fraction", 0.02))
            if frac <= thresh:
                used = "auto_heat"
                log(f"SKIN: auto bone-heat OK ({frac*100:.2f}% unweighted <= {thresh*100:.1f}%)")
            else:
                log(f"SKIN: auto heat left {frac*100:.2f}% unweighted (> "
                    f"{thresh*100:.1f}%) — using envelope fallback")
        except Exception as exc:  # noqa
            log(f"SKIN: auto heat raised ({exc!r}) — using envelope fallback")
    if used is None:
        # clear any partial parent from a failed auto attempt
        mesh.parent = None
        for m in list(mesh.modifiers):
            if m.type == "ARMATURE":
                mesh.modifiers.remove(m)
        used = envelope_weights(mesh, arm_obj, params, report)
        frac = unweighted_fraction(mesh)
        log(f"SKIN: envelope ({frac*100:.2f}% unweighted)")
    report["skinning"] = {"method_requested": method, "method_used": used,
                          "unweighted_fraction": round(frac, 4)}
    if frac > 0.05:
        report["warnings"].append(f"{frac*100:.1f}% of verts unweighted after {used} — "
                                  "some faces may not deform (eyeball gate)")
    return used


# --------------------------------------------------------------------------- stage: animation
def clear_pose_transforms(arm_obj):
    """Zero every pose bone WITHOUT touching the active action (so authoring keyframes
    land on the intended action, not a throwaway)."""
    for pb in arm_obj.pose.bones:
        pb.location = (0, 0, 0)
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.scale = (1, 1, 1)


def bind_action(arm_obj, action):
    """Assign an action AND bind its slot. Blender 4.4+ 'slotted actions' require the
    animation_data.action_slot to be set or the fcurves DO NOT EVALUATE (pose renders /
    bakes as rest). keyframe_insert auto-creates+binds a slot the first time; RE-assigning
    an action later needs the slot re-bound explicitly — this is that fix."""
    ad = arm_obj.animation_data or arm_obj.animation_data_create()
    ad.action = action
    if action is not None:
        slots = getattr(action, "slots", None)
        if slots is not None and len(slots) > 0:
            try:
                ad.action_slot = slots[0]
            except (AttributeError, TypeError):
                pass


def reset_pose(arm_obj):
    bind_action(arm_obj, None)
    clear_pose_transforms(arm_obj)


def _rest_rot(pb):
    return pb.bone.matrix_local.to_3x3()


def world_quat(pb, world_axis, angle_deg):
    """Local pose-bone quaternion that applies a rotation of angle about the given
    WORLD axis (through the bone), independent of bone roll. Accurate when the
    bone's parent is at/near rest; for chain children it reads as a natural bend."""
    R = _rest_rot(pb)
    Rw = Matrix.Rotation(math.radians(angle_deg), 3, Vector(world_axis).normalized())
    return (R.inverted() @ Rw @ R).to_quaternion()


def world_translate(pb, world_vec):
    """Local pose location that yields the given WORLD translation (root bone)."""
    return _rest_rot(pb).inverted() @ Vector(world_vec)


class Animator:
    """Keyframes a named action on the armature. Rotations authored in WORLD axes:
    +X = the left/right horizontal axis (fore/aft swing), +Z = up (twist), +Y = depth.
    Character faces -Y, so a limb swings FORWARD with a NEGATIVE X-angle."""

    def __init__(self, arm_obj, name, fps):
        self.arm = arm_obj
        self.fps = fps
        if arm_obj.animation_data is None:
            arm_obj.animation_data_create()
        self.action = bpy.data.actions.new(name)
        bind_action(arm_obj, self.action)     # keep THIS action active while keyframing
        clear_pose_transforms(arm_obj)         # zero pose but DON'T clear the action
        # first keyframe_insert auto-creates+binds the slot on self.action
        self.pb = {pb.name: pb for pb in arm_obj.pose.bones}

    def rot(self, bone, frame, axis, angle_deg):
        pb = self.pb[bone]
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = world_quat(pb, axis, angle_deg)
        pb.keyframe_insert("rotation_quaternion", frame=frame)

    def rot_multi(self, bone, frame, rotations):
        """Compose several world-axis rotations on one bone (applied in order)."""
        pb = self.pb[bone]
        q = Quaternion((1, 0, 0, 0))
        for axis, ang in rotations:
            q = q @ world_quat(pb, axis, ang)
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = q
        pb.keyframe_insert("rotation_quaternion", frame=frame)

    def loc(self, bone, frame, world_vec):
        pb = self.pb[bone]
        pb.location = world_translate(pb, world_vec)
        pb.keyframe_insert("location", frame=frame)

    def hold_rest(self, bone, frame):
        pb = self.pb[bone]
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.keyframe_insert("rotation_quaternion", frame=frame)

    def finalize(self, loop, length):
        for fc in _iter_fcurves(self.action):
            for kp in fc.keyframe_points:
                kp.interpolation = "BEZIER"
            fc.update()
        return {"action": self.action.name, "start": 0, "end": length, "loop": loop}


def _iter_fcurves(action):
    """Yield an action's fcurves across the pre-4.4 flat API and the 4.4+ slotted
    (layers -> strips -> channelbags) API (Blender 5.1 removed action.fcurves)."""
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


def _cyc(a, t):
    """convenience: sine over a normalized 0..1 phase, amplitude a."""
    return a * math.sin(t * 2 * math.pi)


def author_animations(arm_obj, mesh, params, report):
    fps = int(params.get("fps", 30))
    anims = params.get("anims", {})
    H = report["anchors_ue"]["height"] / UE  # meters
    weapon = params.get("weapon_side", "r")
    off = "l" if weapon == "r" else "r"
    style = params.get("attack_style", "thrust")
    results = {}

    # ---------------- IDLE : gentle breathing + weight settle ----------------
    L = int(anims.get("Idle", {}).get("length", 60))
    A = Animator(arm_obj, f"A_{params['_card_id']}_Idle", fps)
    for f in (0, L):
        A.loc("root", f, (0, 0, 0))
        A.rot("spine_02", f, X_AXIS, 2.0)
        A.rot(f"upperarm_{weapon}", f, Y_AXIS, 6.0)
        A.rot(f"upperarm_{off}", f, Y_AXIS, -6.0)
    A.loc("root", L // 2, (0, 0, 0.012 * H))
    A.rot("spine_02", L // 2, X_AXIS, -1.5)
    A.rot("head", L // 2, X_AXIS, 1.5)
    A.rot(f"upperarm_{weapon}", L // 2, Y_AXIS, 3.0)
    A.rot(f"upperarm_{off}", L // 2, Y_AXIS, -3.0)
    results["Idle"] = A.finalize(True, L)

    # ---------------- WALK : one stride cycle (2 steps) ----------------
    L = int(anims.get("Walk", {}).get("length", 30))
    A = Animator(arm_obj, f"A_{params['_card_id']}_Walk", fps)
    swing, arm_sw, knee = 26.0, 20.0, 34.0
    half = L // 2
    # contact poses at 0 / half / L ; passing at half/2 and 3L/4
    for f, s in ((0, 1), (half, -1), (L, 1)):
        A.rot("thigh_l", f, X_AXIS, -swing * s)   # forward = -X
        A.rot("thigh_r", f, X_AXIS, swing * s)
        A.rot("calf_l", f, X_AXIS, 8.0)
        A.rot("calf_r", f, X_AXIS, 8.0)
        A.rot("upperarm_l", f, X_AXIS, swing * 0.7 * s)   # arms counter-swing
        A.rot("upperarm_r", f, X_AXIS, -swing * 0.7 * s)
        A.loc("root", f, (_cyc(0.02 * H, f / L), 0, 0.0))
    for f, back_leg in ((half // 2, "r"), (half + half // 2, "l")):
        A.rot(f"calf_{back_leg}", f, X_AXIS, knee)   # knee bends on the lifting leg
        A.loc("root", f, (0, 0, 0.02 * H))           # pelvis rises at passing
        A.rot("spine_02", f, Z_AXIS, 4.0 if back_leg == "r" else -4.0)
    results["Walk"] = A.finalize(True, L)

    # ---------------- ATTACK ----------------
    L = int(anims.get("Attack", {}).get("length", 40))
    A = Animator(arm_obj, f"A_{params['_card_id']}_Attack", fps)
    w_up, w_lo, w_spine, w_head = f"upperarm_{weapon}", f"lowerarm_{weapon}", "spine_02", "head"
    p1, p2, p3 = int(L * 0.30), int(L * 0.55), int(L * 0.78)  # windup / strike / recover
    if style in ("thrust", "mine"):
        # rest at frame 0 for EVERY bone this action rotates (or constant extrapolation
        # snaps them to their first keyed value before that key)
        for b in (w_up, w_lo, w_spine, w_head):
            A.hold_rest(b, 0)
        A.loc("root", 0, (0, 0, 0))
        # windup: pull weapon arm back+up, wind torso
        A.rot_multi(w_up, p1, [(X_AXIS, 42), (Z_AXIS, -18)])
        A.rot(w_lo, p1, X_AXIS, -50)
        A.rot(w_spine, p1, Z_AXIS, 22)
        A.loc("root", p1, (0, 0.02 * H, 0))
        # strike: drive arm forward (thrust), extend, step in, torso unwinds
        A.rot_multi(w_up, p2, [(X_AXIS, -78), (Z_AXIS, 6)])
        A.rot(w_lo, p2, X_AXIS, -8)
        A.rot(w_spine, p2, Z_AXIS, -14)
        A.rot(w_head, p2, X_AXIS, -6)
        A.loc("root", p2, (0, -0.14 * H, 0))    # lunge forward (-Y)
        # recover toward rest
        A.rot_multi(w_up, p3, [(X_AXIS, -18), (Z_AXIS, 0)])
        A.rot(w_lo, p3, X_AXIS, -22)
        A.rot(w_spine, p3, Z_AXIS, 4)
        A.loc("root", p3, (0, -0.03 * H, 0))
    else:  # generic overhead swing
        A.rot(w_up, p1, X_AXIS, 120)
        A.rot(w_lo, p1, X_AXIS, -30)
        A.rot(w_spine, p1, Z_AXIS, 18)
        A.rot(w_up, p2, X_AXIS, -40)
        A.rot(w_spine, p2, Z_AXIS, -18)
        A.loc("root", p2, (0, -0.10 * H, 0))
        A.rot(w_up, p3, X_AXIS, -10)
    for b in (w_up, w_lo, w_spine, w_head):
        A.hold_rest(b, L)
    A.loc("root", L, (0, 0, 0))
    results["Attack"] = A.finalize(False, L)

    # ---------------- DEATH : stagger + collapse backward ----------------
    L = int(anims.get("Death", {}).get("length", 48))
    A = Animator(arm_obj, f"A_{params['_card_id']}_Death", fps)
    off_arm = f"upperarm_{off}"
    s1, s2, s3 = int(L * 0.22), int(L * 0.66), L
    death_bones = ("root", "spine_01", "spine_02", "calf_l", "calf_r", w_up, off_arm)
    # frame-0 rest for every rotated bone (else constant extrapolation pre-snaps them)
    for b in death_bones:
        A.hold_rest(b, 0)
    A.loc("root", 0, (0, 0, 0))
    # stagger (still UPRIGHT — root barely rotates): recoil, arms fly out
    A.rot("root", s1, X_AXIS, -6)
    A.rot("spine_02", s1, X_AXIS, -16)
    A.rot(w_up, s1, X_AXIS, -34)
    A.rot(off_arm, s1, X_AXIS, -28)
    A.loc("root", s1, (0, 0.015 * H, 0.0))
    # collapse: whole body rotates back about the feet, knees buckle
    A.rot("root", s2, X_AXIS, -80)          # fall onto back (+Y)
    A.rot("calf_l", s2, X_AXIS, 58)
    A.rot("calf_r", s2, X_AXIS, 52)
    A.rot("spine_02", s2, X_AXIS, 20)
    A.rot("spine_01", s2, X_AXIS, 12)
    A.rot(w_up, s2, X_AXIS, -52)
    A.rot(off_arm, s2, X_AXIS, -48)
    A.loc("root", s2, (0, 0, 0.0))
    # settle (limp)
    A.rot("root", s3, X_AXIS, -86)
    A.rot("spine_02", s3, X_AXIS, 12)
    A.rot("calf_l", s3, X_AXIS, 46)
    A.rot("calf_r", s3, X_AXIS, 42)
    A.loc("root", s3, (0, 0, 0.0))
    results["Death"] = A.finalize(False, L)

    reset_pose(arm_obj)
    report["animations"] = {k: v for k, v in results.items()}
    log("ANIM: authored " + ", ".join(f"{k}[{v['end']}f]" for k, v in results.items()))
    return results


# --------------------------------------------------------------------------- stage: preview materials
def prep_preview_materials(mesh, card_id, tex_dir):
    """Wire T_<CardID>_D into the PBR slot's Base Color for a truthful preview, and set
    the TeamRegion slot to the blue placeholder. Does NOT change slot names/order/faces."""
    d_png = tex_dir / f"T_{card_id}_D.png"
    for i, mat in enumerate(mesh.data.materials):
        if mat is None:
            continue
        mat.use_nodes = True
        nt = mat.node_tree
        bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if bsdf is None:
            continue
        if mat.name == "TeamRegion":
            bsdf.inputs["Base Color"].default_value = (0.05, 0.30, 1.0, 1.0)
            bsdf.inputs["Roughness"].default_value = 0.5
        else:
            if d_png.is_file():
                img = bpy.data.images.load(str(d_png), check_existing=True)
                tex = nt.nodes.new("ShaderNodeTexImage")
                tex.image = img
                nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])


# --------------------------------------------------------------------------- stage: export
def _select_for_export(mesh, arm_obj):
    ensure_object_mode()
    select_only([arm_obj, mesh], active=arm_obj)


def export_skeletal_fbx(mesh, arm_obj, out_path, guard):
    """Rigged mesh at REST/bind pose. Axis contract IDENTICAL to the static pipeline
    (axis_forward='-Z', axis_up='Y') so SK faces the same way as SM."""
    out = guard.check(out_path)
    reset_pose(arm_obj)
    _select_for_export(mesh, arm_obj)
    bpy.ops.export_scene.fbx(
        filepath=str(out), use_selection=True,
        object_types={"ARMATURE", "MESH"},
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Z", axis_up="Y", mesh_smooth_type="FACE",
        use_mesh_modifiers=False,      # keep the Armature modifier live (do NOT bake it)
        add_leaf_bones=False, bake_anim=False,
        use_armature_deform_only=False,
        primary_bone_axis="Y", secondary_bone_axis="X",
        path_mode="STRIP",
    )
    log(f"EXPORT mesh: {out}")
    return out


def export_anim_fbx(mesh, arm_obj, action_name, frame_end, out_path, guard):
    out = guard.check(out_path)
    action = bpy.data.actions.get(action_name)
    if action is None:
        fail(f"anim action '{action_name}' missing at export")
    bind_action(arm_obj, action)   # bind the slot or the exporter bakes rest pose
    clear_pose_transforms(arm_obj)  # bones this action doesn't key stay at rest (no carryover)
    scene = bpy.context.scene
    scene.frame_start = 0
    scene.frame_end = int(frame_end)
    _select_for_export(mesh, arm_obj)
    bpy.ops.export_scene.fbx(
        filepath=str(out), use_selection=True,
        object_types={"ARMATURE", "MESH"},
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
    log(f"EXPORT anim {action_name}: {out}")
    return out


# --------------------------------------------------------------------------- stage: preview render
def _setup_scene_render(px):
    scene = bpy.context.scene
    scene.render.resolution_x = scene.render.resolution_y = px
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    engine = None
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE", "BLENDER_WORKBENCH"):
        try:
            scene.render.engine = eng
            engine = eng
            break
        except Exception:  # noqa
            continue
    if engine == "BLENDER_WORKBENCH":
        scene.display.shading.light = "STUDIO"
        scene.display.shading.color_type = "TEXTURE"
    # world + key light
    if scene.world is None:
        scene.world = bpy.data.worlds.new("RigPreviewWorld")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.05, 0.06, 0.08, 1.0)
        bg.inputs["Strength"].default_value = 0.6
    if not any(o.type == "LIGHT" for o in bpy.data.objects):
        sd = bpy.data.lights.new("RigSun", "SUN")
        sd.energy = 4.0
        sun = bpy.data.objects.new("RigSun", sd)
        bpy.context.scene.collection.objects.link(sun)
        sun.rotation_euler = Vector((-0.5, -0.2, -0.9)).to_track_quat("-Z", "Y").to_euler()
    return engine


def _place_camera(center, max_dim, direction):
    cam = bpy.data.objects.get("RigPreviewCam")
    if cam is None:
        cd = bpy.data.cameras.new("RigPreviewCam")
        cam = bpy.data.objects.new("RigPreviewCam", cd)
        bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    d = Vector(direction).normalized()
    cam.location = Vector(center) + d * max_dim * 2.6
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 60
    cam.data.clip_end = max_dim * 12
    return cam


def _frame_center(mesh):
    v = verts_np(mesh)
    mn, mx = v.min(axis=0), v.max(axis=0)
    center = Vector(((mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2, (mn[2] + mx[2]) / 2))
    max_dim = float(max(mx - mn))
    return center, max_dim


def _render_still(scene, path):
    scene.render.image_settings.file_format = "PNG"
    scene.render.use_file_extension = False
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def _tile_strip(png_paths, out_path):
    """Concatenate equal-size PNGs horizontally into one strip PNG (numpy; no PIL)."""
    tiles = []
    for p in png_paths:
        img = bpy.data.images.load(str(p))
        w, h = img.size
        buf = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(buf)
        tiles.append(buf.reshape(h, w, 4))
        bpy.data.images.remove(img)
    strip = np.concatenate(tiles, axis=1)
    sh, sw = strip.shape[:2]
    out_img = bpy.data.images.new(out_path.stem, width=sw, height=sh, alpha=True)
    out_img.pixels.foreach_set(strip.ravel())
    out_img.filepath_raw = str(out_path)
    out_img.file_format = "PNG"
    out_img.save()
    bpy.data.images.remove(out_img)
    return str(out_path)


def render_previews(mesh, arm_obj, params, anims, guard, cache_dir, quick, report):
    """Video-free preview packet (this Blender build has no FFMPEG/GIF output):
    bind stills + an 8-angle turntable strip + per-anim contact strips + full per-anim
    PNG frame SEQUENCES (the genuine clip frames — assemble to mp4 externally / scrub in
    the editor). EEVEE-shaded with the real T_<CardID>_D + blue TeamRegion."""
    card_id = params["_card_id"]
    pv = params.get("preview", {})
    px = 360 if quick else int(pv.get("px", 480))
    cpx = 300 if quick else int(pv.get("contact_px", 360))
    n_contact = 7          # frames per anim contact strip
    n_angles = 8           # turntable strip angles
    fps = int(params.get("fps", 30))

    engine = _setup_scene_render(px)
    center, max_dim = _frame_center(mesh)
    scene = bpy.context.scene
    scene.render.fps = fps
    frames_dir = guard.check(cache_dir / "previews" / "frames")
    report.setdefault("previews", {})

    threeq = (-0.62, -0.66, 0.42)

    # ---- bind stills (rest pose) ----
    reset_pose(arm_obj)
    if arm_obj.animation_data:
        arm_obj.animation_data.action = None
    scene.frame_set(0)
    scene.render.resolution_x = scene.render.resolution_y = px
    for name, d in {"bind_front": (0, -1, 0.05), "bind_side": (-1, 0, 0.05),
                    "bind_threequarter": threeq}.items():
        _place_camera(center, max_dim, d)
        out = guard.check(cache_dir / "previews" / f"{name}.png")
        _render_still(scene, out)
        report["previews"][name] = str(out)
    log(f"  bind stills x3 ({px}px)")

    # ---- turntable strip: n_angles views of the bind pose ----
    scene.render.resolution_x = scene.render.resolution_y = cpx
    angle_pngs = []
    for i in range(n_angles):
        ang = (i / n_angles) * 2 * math.pi
        d = (math.sin(ang) * -1.0, -math.cos(ang), 0.28)
        _place_camera(center, max_dim, d)
        p = frames_dir / f"tt_{i}.png"
        _render_still(scene, p)
        angle_pngs.append(p)
    tt = _tile_strip(angle_pngs, guard.check(cache_dir / "previews" / "turntable_strip.png"))
    report["previews"]["turntable_strip"] = tt
    log(f"  turntable_strip ({n_angles} angles) -> {Path(tt).name}")

    # ---- per anim: contact strip (quick look) + full PNG sequence (the clip) ----
    report["previews"]["anim_sequences"] = {}
    for name, meta in anims.items():
        action = bpy.data.actions.get(meta["action"])
        bind_action(arm_obj, action)   # bind the slot or frames render as rest
        clear_pose_transforms(arm_obj)  # unkeyed bones stay at rest (no cross-anim carryover)
        end = meta["end"]
        # contact strip
        _place_camera(center, max_dim, threeq)
        scene.render.resolution_x = scene.render.resolution_y = cpx
        strip_pngs = []
        for i in range(n_contact):
            f = int(round(end * i / (n_contact - 1)))
            scene.frame_set(f)
            p = frames_dir / f"{name.lower()}_{i}.png"
            _render_still(scene, p)
            strip_pngs.append(p)
        sheet = _tile_strip(strip_pngs, guard.check(cache_dir / "previews" / f"contact_{name.lower()}.png"))
        report["previews"][f"contact_{name}"] = sheet
        # full sequence (real clip frames)
        seq_dir = guard.check(cache_dir / "previews" / "seq" / name.lower())
        _place_camera(center, max_dim, threeq)
        scene.render.resolution_x = scene.render.resolution_y = px
        scene.render.image_settings.file_format = "PNG"
        scene.render.use_file_extension = True
        scene.frame_start, scene.frame_end = 0, int(end)
        scene.render.filepath = str(seq_dir / "frame_")
        bpy.ops.render.render(animation=True)
        report["previews"]["anim_sequences"][name] = str(seq_dir)
        log(f"  anim {name}: contact strip + {int(end)+1}-frame seq -> {Path(sheet).name}")

    reset_pose(arm_obj)
    if arm_obj.animation_data:
        arm_obj.animation_data.action = None
    report["preview_engine"] = engine
    report["preview_note"] = ("This Blender build has NO FFMPEG/GIF output — clips are "
                              "delivered as per-anim PNG frame SEQUENCES (previews/seq/<anim>/) "
                              "plus contact strips. Assemble to mp4 externally or scrub the "
                              "sequence; real playback is judged in-editor at TASK-162.")


# --------------------------------------------------------------------------- main
def main():
    args = parse_args()
    card_id = args.card_id
    manifest_path = Path(args.manifest).resolve() if args.manifest \
        else SCRIPT_DIR / "rig_manifest.json"
    manifest = load_manifest(manifest_path)
    params = merged_params(manifest, card_id)
    params["_card_id"] = card_id

    input_fbx = Path(args.input).resolve() if args.input \
        else (PROJECT_ROOT / params.get("input_fbx", f"Content/RawAssets/{card_id}.fbx")).resolve()
    if not input_fbx.is_file():
        fail(f"game-ready input FBX not found: {input_fbx}", code=2)

    guard = OutputGuard(card_id, args.smoke)
    cache_dir = (CACHE_ROOT / card_id / "rig")
    tex_dir = CONTENT_RAW / "Textures" / card_id
    if args.smoke:
        mesh_fbx = cache_dir / "smoke" / f"{card_id}.fbx"
        anim_dir = cache_dir / "smoke" / "Anims"
    else:
        mesh_fbx = CHARACTERS_RAW / f"{card_id}.fbx"
        anim_dir = CHARACTERS_RAW / "Anims"

    log(f"card-id={card_id} smoke={args.smoke} quick={args.quick}")
    log(f"input={input_fbx}")
    log(f"mesh-out={mesh_fbx}")
    log(f"blender={bpy.app.version_string}")

    report = {
        "card_id": card_id, "smoke": args.smoke, "quick": args.quick,
        "input": str(input_fbx), "manifest": str(manifest_path),
        "skeleton": params["_skeleton_name"],
        "blender_version": bpy.app.version_string,
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "warnings": [],
    }
    started = time.time()

    reset_scene()
    mesh = import_mesh(input_fbx, card_id, report)
    anchors = measure_anchors(mesh, report)
    arm_obj = build_armature(anchors, params["_skeleton_spec"], card_id, report)
    skin(mesh, arm_obj, params, report)
    anims = author_animations(arm_obj, mesh, params, report)

    prep_preview_materials(mesh, card_id, tex_dir)

    # exports
    report["outputs"] = {}
    report["outputs"]["mesh_fbx"] = str(export_skeletal_fbx(mesh, arm_obj, mesh_fbx, guard))
    if not args.no_anim_fbx:
        report["outputs"]["anim_fbx"] = {}
        for name, meta in anims.items():
            out = export_anim_fbx(mesh, arm_obj, meta["action"], meta["end"],
                                  anim_dir / f"{card_id}_{name}.fbx", guard)
            report["outputs"]["anim_fbx"][name] = str(out)

    render_previews(mesh, arm_obj, params, anims, guard, cache_dir, args.quick, report)

    if args.save_blend:
        blend_out = guard.check(cache_dir / "rig_debug.blend")
        bpy.ops.wm.save_as_mainfile(filepath=str(blend_out))
        log(f"saved debug blend -> {blend_out}")

    report["elapsed_seconds"] = round(time.time() - started, 1)
    report_path = guard.check(cache_dir / "rig_report.json")
    with open(report_path, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
    log(f"REPORT: {report_path}")
    for w in report["warnings"]:
        log(f"WARNING: {w}")
    log(f"DONE in {report['elapsed_seconds']}s "
        f"(skin={report['skinning']['method_used']}, "
        f"anims={list(anims.keys())})")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        fail("unhandled exception (see traceback above)")
    sys.exit(0)
