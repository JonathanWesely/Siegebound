"""build_entry_dressing_props.py -- TASK-657 [F1-2] the castle-entry DISCOVERABILITY asset set.

Three procedurally-authored castle-prop families (GH-R4: zero Meshy/TRELLIS spend),
full texture-bake lane (NOT the WR-4 stock-node prop exception -- the F1-R2 activation
ruling pins T_ sets + MIs parented to M_AssetPBR):

  SM_Castle_GateBanner   -- gate-ward marker for the south channel mouth (tall, crimson
                            + gold chevron-down heraldry, silhouette-readable from the
                            spawn line ~4300 uu away).
  SM_Castle_TramplePath  -- trampled-dirt road ribbon, tileable along local X (the
                            travel axis), straight ends / ruffled sides, wheel ruts.
  SM_Castle_ToeRock01/02 -- cold-grey angular granite dressing for the east d1 seal
                            line (local x ~3650) so the invisible +506 wall gets a
                            VISIBLE cause. Both rocks share ONE texture set / MI.

LAWS HONOURED:
  - Pivot at ground contact (min Z = 0), +X forward -- the TASK-661 cross-lane contract.
  - COLLISIONLESS ON PURPOSE: no UCX authored (SM_Torch precedent); pipeline_manifest
    declares ucx: null; TASK-661 additionally enforces NoCollision code-side.
  - Bake recipe mirrors TASK-630 (refine_trellis_glb.py::bake_all): Cycles CPU, DIFFUSE
    pass_filter={COLOR}, ROUGHNESS, NORMAL tangent, AO via AmbientOcclusion-node->EMIT
    (distance 2.5 m), numpy ORM pack R=AO / G=rough / B=0 (no metal -- gold is
    albedo+roughness, the house B=0 pattern).
  - Albedo delight: the PINNED LOCKED profile {ao_divide_strength 1.0, ao_floor 0.25,
    gamma 0.55, gain 1.2} + shoulder 0.80 / max_out 0.98 (the DEFAULT-TRAP law --
    never the script defaults). Authored palettes are the analytic AO=1 inversion of
    the INTENDED post-delight palette, exactly the TASK-630 declared posture.
  - Gates measured in-script: UV-normalised covered mean linear vs floor 0.2536,
    declared-intent retention band 0.85..1.25 (warn-only; operative bleach guard =
    retention > 1.25 AND UV-norm > 0.60), chroma C*ab report, ORM sanity.
  - UV: cube_project + pack_islands(rotate=False, shape_method='AABB') -- the TASK-556
    determinism finding (smart_project is NOT reproducible on this build). Layer 'UVMap'.
  - Export: TASK-556/348 contract verbatim -- ue_handedness_precomp (mirror_Y + winding
    flip involution), axis -Z/Y, apply_unit_scale, FACE smoothing, triangulated, STRIP.

Reproduce (per family, ~5-8 min each on CPU):
  "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background
      --factory-startup --python-exit-code 1
      --python Tools/ArtPipeline/build_entry_dressing_props.py -- --family GateBanner
  (--family one of GateBanner | TramplePath | ToeRock | all)

Outputs:
  Content/RawAssets/Castle_{GateBanner,TramplePath,ToeRock01,ToeRock02}.fbx
  Content/RawAssets/Textures/Castle/T_Castle_{GateBanner,TramplePath,ToeRock}_{D,N,ORM}.png
  Tools/ArtPipeline/Cache/EntryDressing/{props_report.json, previews/, debug/}
"""

import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "Tools" / "ArtPipeline" / "Cache" / "EntryDressing"
PREVIEWS = CACHE / "previews"
DEBUG = CACHE / "debug"
RAW = ROOT / "Content" / "RawAssets"
RAWTEX = RAW / "Textures" / "Castle"

UE = 100.0            # uu per metre; Blender works in metres


def m(uu):
    return uu / UE


# ---------------------------------------------------------------- pinned delight profile
# THE LOCKED PROFILE (TASK-630 / DEFAULT-TRAP law). Never the refine-script defaults.
DELIGHT = {
    "ao_divide_strength": 1.0,
    "ao_floor": 0.25,
    "gamma": 0.55,
    "gain": 1.2,
    "shoulder": 0.80,
    "max_out": 0.98,
}
ALBEDO_FLOOR = 0.2536         # UV-normalised covered mean linear (the fleet gate)
BAKE_RES = 1024
MARGIN_PX = 16
SAMPLES = 8
AO_SAMPLES = 64
AO_DISTANCE_M = 2.5           # 250 uu -- the TASK-630 pinned finite AO radius

RNG = np.random.RandomState(657)


def inv_delight(post):
    """Authored (pre-delight) linear albedo from INTENDED post-delight linear, at AO=1
    (the TASK-630 posture: the AO-divide's extra crevice lift is declared, not hidden)."""
    return tuple(min(1.0, (c / DELIGHT["gain"]) ** (1.0 / DELIGHT["gamma"])) for c in post)


# intended POST-delight palettes (linear) -- these are what should ship on screen
POST = {
    "banner_crimson": (0.400, 0.055, 0.070),
    "banner_gold":    (0.620, 0.430, 0.120),
    "banner_wood":    (0.165, 0.100, 0.058),
    # path palette lifted +12% 2026-08-27 run 2: run-1 covered mean 0.2464 vs the
    # 0.2536 albedo floor (intent card itself was under-floor; retention was 1.036)
    "path_dirt":      (0.405, 0.315, 0.210),
    "path_rut":       (0.265, 0.195, 0.122),
    "path_edge":      (0.225, 0.290, 0.113),
    "rock_base":      (0.335, 0.330, 0.325),
    "rock_dark":      (0.170, 0.168, 0.172),
    "rock_warm":      (0.295, 0.255, 0.185),
}
PRE = {k: inv_delight(v) for k, v in POST.items()}


def log(msg):
    print(f"[T657] {msg}", flush=True)


def srgb_encode(lin):
    lin = np.clip(lin, 0.0, 1.0)
    return np.where(lin <= 0.0031308, lin * 12.92,
                    1.055 * np.power(np.maximum(lin, 0.0), 1.0 / 2.4) - 0.055)


def srgb_decode(enc):
    enc = np.clip(enc, 0.0, 1.0)
    return np.where(enc <= 0.04045, enc / 12.92,
                    np.power((enc + 0.055) / 1.055, 2.4))


# --------------------------------------------------------------------------- scene utils

def wipe_scene():
    bpy.ops.object.select_all(action="SELECT") if bpy.context.object else None
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for coll in (bpy.data.meshes, bpy.data.materials, bpy.data.images,
                 bpy.data.lights, bpy.data.cameras, bpy.data.worlds):
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


# --------------------------------------------------------------------------- node helpers

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


def set_bake_target(obj, image):
    """Ensure every material on obj carries a selected+active image node aimed at image."""
    for slot in obj.material_slots:
        mat = slot.material
        nt = mat.node_tree
        node = nt.nodes.get("BakeTarget")
        if node is None:
            node = nt.nodes.new("ShaderNodeTexImage")
            node.name = "BakeTarget"
            node.location = (-900, -600)
        node.image = image
        for n in nt.nodes:
            n.select = False
        node.select = True
        nt.nodes.active = node


def new_image(name, size, colorspace, rgba=None):
    img = bpy.data.images.new(name, width=size, height=size, alpha=False)
    img.colorspace_settings.name = colorspace
    if rgba is not None:
        img.pixels.foreach_set(rgba.astype(np.float32).ravel())
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


# --------------------------------------------------------------------------- spectral noise

def spectral2d(rng, W, H, octaves, periodic=True):
    """Deterministic sum-of-gratings noise in [0,1]; integer wavenumbers -> tileable
    in BOTH axes by construction (the TramplePath X-tiling law)."""
    u, v = np.meshgrid(np.arange(W) / W, np.arange(H) / H)
    out = np.zeros((H, W))
    total = 0.0
    for kmax, amp in octaves:
        for _ in range(4):
            ku = int(rng.randint(1, kmax + 1))
            kv = int(rng.randint(1, kmax + 1))
            ph = rng.uniform(0.0, 2.0 * np.pi)
            out += amp * np.sin(2.0 * np.pi * (ku * u + kv * v) + ph)
            total += amp
    out = out / (2.0 * total) + 0.5
    return np.clip(out, 0.0, 1.0)


def spectral3d_factory(rng, octaves):
    """Deterministic 3D scalar field f(p_m) built from random-direction sinusoids."""
    waves = []
    for freq, amp in octaves:
        for _ in range(5):
            direction = rng.normal(size=3)
            direction = direction / np.linalg.norm(direction) * freq
            waves.append((direction, rng.uniform(0, 2 * np.pi), amp))

    def field(p):
        val = 0.0
        for k, ph, amp in waves:
            val += amp * math.sin(k[0] * p[0] + k[1] * p[1] + k[2] * p[2] + ph)
        return val
    return field


# --------------------------------------------------------------------------- UV + bake

def uv_atlas(obj, art_layer=False):
    """Deterministic atlas on layer 'UVMap' (cube_project + AABB pack -- TASK-556)."""
    mesh = obj.data
    if not mesh.uv_layers:
        mesh.uv_layers.new(name="UVMap")
    uvmap = mesh.uv_layers["UVMap"]
    mesh.uv_layers.active = uvmap
    uvmap.active_render = True
    bpy.context.scene.tool_settings.use_uv_select_sync = True
    select_only([obj])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=2.0)
    bpy.ops.uv.pack_islands(rotate=False, shape_method="AABB", margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")


def scale_uvs(obj, u_scale, u_offset):
    uvmap = obj.data.uv_layers["UVMap"]
    for datum in uvmap.data:
        datum.uv[0] = datum.uv[0] * u_scale + u_offset


def cycles_setup():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.use_denoising = False


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


def bake_ao(obj, image, use_clear, hide=()):
    """AO via AmbientOcclusion-node -> EMIT on a temp copy (the TASK-630 lane)."""
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


def apply_delight(d_img, ao_img, report_slot):
    """The refine_trellis_glb.py::apply_albedo_delight algorithm, PINNED profile."""
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

    report_slot["albedo_delight"] = dict(cfg)
    report_slot["albedo_delight"].update({
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


# --------------------------------------------------------------------------- gates

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


def measure_gates(final_d_rgba, orm_rgba, intent_post, intent_weights, report_slot):
    stored = final_d_rgba[:, :, :3].astype(np.float64)
    covered = np.any(stored > 0.0, axis=2)          # the fleet nonzero rule
    linear = srgb_decode(stored)
    cov_lin = linear[covered]
    uv_norm = float(cov_lin.mean()) if cov_lin.size else 0.0

    ref = np.array([np.array(intent_post[k]) * w for k, w in intent_weights.items()])
    ref_mean = float(sum(r.mean() for r in ref))
    retention = uv_norm / ref_mean if ref_mean > 0 else 0.0

    chroma = float(lin_to_lab_chroma(cov_lin.reshape(-1, 3)).mean()) if cov_lin.size else 0.0
    ref_cols = np.array([intent_post[k] for k in intent_weights])
    ref_chroma = float((lin_to_lab_chroma(ref_cols) *
                        np.array(list(intent_weights.values()))).sum())

    ao = orm_rgba[:, :, 0].astype(np.float64)[covered]
    rough = orm_rgba[:, :, 1].astype(np.float64)[covered]
    bleach_tripped = retention > 1.25 and uv_norm > 0.60
    report_slot["gates"] = {
        "coverage_fraction": round(float(covered.mean()), 4),
        "uv_norm_covered_mean_linear": round(uv_norm, 4),
        "albedo_floor": ALBEDO_FLOOR,
        "albedo_floor_pass": bool(uv_norm >= ALBEDO_FLOOR),
        "intent_reference_mean_linear": round(ref_mean, 4),
        "intent_retention": round(retention, 4),
        "intent_retention_band": [0.85, 1.25],
        "anti_bleach_operative_guard_tripped": bool(bleach_tripped),
        "chroma_covered_Cab": round(chroma, 2),
        "chroma_intent_Cab": round(ref_chroma, 2),
        "orm_ao_covered_mean": round(float(ao.mean()), 4) if ao.size else None,
        "orm_rough_covered_min_max": [round(float(rough.min()), 3),
                                      round(float(rough.max()), 3)] if rough.size else None,
        "orm_metal_max": 0.0,
    }
    return report_slot["gates"]


# --------------------------------------------------------------------------- measure mesh

def measure(obj):
    mesh = obj.data
    xs = [v.co.x for v in mesh.vertices]
    ys = [v.co.y for v in mesh.vertices]
    zs = [v.co.z for v in mesh.vertices]
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    return {
        "dims_uu": [round((max(xs) - min(xs)) * UE, 2),
                    round((max(ys) - min(ys)) * UE, 2),
                    round((max(zs) - min(zs)) * UE, 2)],
        "bounds_min_uu": [round(min(xs) * UE, 2), round(min(ys) * UE, 2),
                          round(min(zs) * UE, 2)],
        "bounds_max_uu": [round(max(xs) * UE, 2), round(max(ys) * UE, 2),
                          round(max(zs) * UE, 2)],
        "min_z_uu": round(min(zs) * UE, 4),
        "tris": tris,
        "verts": len(mesh.vertices),
        "uv_layers": [l.name for l in mesh.uv_layers],
        "material_slots": [s.material.name if s.material else None
                           for s in obj.material_slots],
    }


# --------------------------------------------------------------------------- export (556 verbatim)

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


# --------------------------------------------------------------------------- artwork

def banner_artwork():
    """Painted cloth albedo, AUTHORED (pre-delight) sRGB-encoded. 512x640."""
    W, H = 512, 640
    rng = np.random.RandomState(6571)
    crimson = np.array(PRE["banner_crimson"])
    gold = np.array(PRE["banner_gold"])
    mottle = spectral2d(rng, W, H, [(6, 1.0), (14, 0.6), (30, 0.35)])[..., None]
    art = crimson[None, None, :] * (0.80 + 0.40 * mottle)

    x = np.arange(W)[None, :].repeat(H, axis=0).astype(np.float64)
    y = np.arange(H)[:, None].repeat(W, axis=1).astype(np.float64)  # 0 = bottom row
    cx = (W - 1) / 2.0

    gold_mask = np.zeros((H, W), dtype=bool)
    border = 26
    gold_mask |= (x < border) | (x > W - 1 - border)
    gold_mask |= (y > H - 1 - border) | (y < border)
    # bold chevron, apex DOWN at centre (the "here, downward, enter" cue)
    apex_y = 0.30 * H
    slope = 0.62
    d1 = np.abs(y - (apex_y + slope * np.abs(x - cx)))
    gold_mask |= (d1 < 40) & (np.abs(x - cx) < 0.80 * cx)
    # second thinner chevron above
    d2 = np.abs(y - (apex_y + 150 + slope * np.abs(x - cx)))
    gold_mask |= (d2 < 15) & (np.abs(x - cx) < 0.80 * cx)

    gold_var = (0.85 + 0.30 * spectral2d(rng, W, H, [(10, 1.0), (24, 0.5)]))[..., None]
    art = np.where(gold_mask[..., None], gold[None, None, :] * gold_var, art)
    weave = spectral2d(rng, W, H, [(64, 1.0)])[..., None]
    art = art * (0.94 + 0.12 * weave)
    return np.concatenate([srgb_encode(np.clip(art, 0, 1)),
                           np.ones((H, W, 1))], axis=2), gold_mask


def path_artwork():
    """Trampled dirt albedo + height, tileable in u (u runs along local X). 512x512."""
    W = H = 512
    rng = np.random.RandomState(6572)
    dirt = np.array(PRE["path_dirt"])
    rut = np.array(PRE["path_rut"])
    edge = np.array(PRE["path_edge"])

    base_noise = spectral2d(rng, W, H, [(5, 1.0), (13, 0.6), (29, 0.4), (61, 0.25)])
    art = dirt[None, None, :] * (0.78 + 0.44 * base_noise[..., None])

    v = np.arange(H)[:, None].repeat(W, axis=1) / (H - 1.0)   # 0..1 across width (y)
    wobble = 0.02 * np.sin(np.arange(W) / W * 2 * np.pi * 3 + 1.1)[None, :]
    rut_mask = np.zeros((H, W))
    for centre in (0.30, 0.70):
        rut_mask += np.exp(-((v - centre + wobble) ** 2) / (2 * 0.035 ** 2))
    rut_mask = np.clip(rut_mask, 0.0, 1.0)
    art = art * (1 - rut_mask[..., None]) + rut[None, None, :] * \
        (0.82 + 0.3 * base_noise[..., None]) * rut_mask[..., None]

    # foot-churn speckle between the ruts
    speck = spectral2d(rng, W, H, [(40, 1.0), (80, 0.6)])
    speck_mask = (speck > 0.62).astype(np.float64) * 0.5
    art = art * (1 - speck_mask[..., None] * 0.35)

    # grassy blend at the ribbon edges
    edge_w = np.clip((np.abs(v - 0.5) - 0.36) / 0.14, 0.0, 1.0)
    edge_n = spectral2d(rng, W, H, [(9, 1.0), (21, 0.7)])
    edge_mask = np.clip(edge_w * (0.5 + 0.8 * edge_n), 0.0, 1.0)
    art = art * (1 - edge_mask[..., None]) + edge[None, None, :] * edge_mask[..., None]

    height = 0.5 - 0.30 * rut_mask + 0.18 * (speck - 0.5) + 0.10 * (base_noise - 0.5)
    height = np.clip(height, 0.0, 1.0)
    art_rgba = np.concatenate([srgb_encode(np.clip(art, 0, 1)),
                               np.ones((H, W, 1))], axis=2)
    h_rgba = np.concatenate([np.repeat(height[..., None], 3, axis=2),
                             np.ones((H, W, 1))], axis=2)
    masks = {"rut": rut_mask, "edge": edge_mask}
    return art_rgba, h_rgba, masks


# --------------------------------------------------------------------------- builders

def build_gatebanner():
    """SM_Castle_GateBanner: plinth + tapered pole + crossarm + finials + shaped cloth.
    Pivot: pole axis at (0,0), min Z 0 (ground contact). Cloth face normal +X forward."""
    wood = new_mat("banner_wood_author")
    p = principled(wood)
    tex = add_node(wood, "ShaderNodeTexWave", location=(-600, 200))
    tex.wave_type = "BANDS"
    tex.inputs["Scale"].default_value = 3.0
    tex.inputs["Distortion"].default_value = 4.0
    ramp = add_node(wood, "ShaderNodeValToRGB", location=(-350, 200))
    ramp.color_ramp.elements[0].color = (*PRE["banner_wood"], 1.0)
    dark = tuple(c * 0.55 for c in PRE["banner_wood"])
    ramp.color_ramp.elements[1].color = (*dark, 1.0)
    link(wood, tex.outputs["Color"], ramp.inputs["Fac"])
    link(wood, ramp.outputs["Color"], p.inputs["Base Color"])
    p.inputs["Roughness"].default_value = 0.72
    bump = add_node(wood, "ShaderNodeBump", location=(-350, -150))
    bump.inputs["Strength"].default_value = 0.25
    link(wood, tex.outputs["Color"], bump.inputs["Height"])
    link(wood, bump.outputs["Normal"], p.inputs["Normal"])

    gold = new_mat("banner_gold_author")
    pg = principled(gold)
    pg.inputs["Base Color"].default_value = (*PRE["banner_gold"], 1.0)
    pg.inputs["Roughness"].default_value = 0.32
    pg.inputs["Metallic"].default_value = 0.0     # house B=0 ORM pattern

    cloth = new_mat("banner_cloth_author")
    pc = principled(cloth)
    art_rgba, _mask = banner_artwork()
    art_img = np_to_image("banner_art", art_rgba, "sRGB")
    uvn = add_node(cloth, "ShaderNodeUVMap", location=(-900, 200))
    uvn.uv_map = "ArtMap"
    texn = add_node(cloth, "ShaderNodeTexImage", location=(-650, 200))
    texn.image = art_img
    texn.extension = "EXTEND"
    link(cloth, uvn.outputs["UV"], texn.inputs["Vector"])
    link(cloth, texn.outputs["Color"], pc.inputs["Base Color"])
    pc.inputs["Roughness"].default_value = 0.85
    noise = add_node(cloth, "ShaderNodeTexNoise", location=(-650, -150))
    noise.inputs["Scale"].default_value = 220.0
    cb = add_node(cloth, "ShaderNodeBump", location=(-400, -150))
    cb.inputs["Strength"].default_value = 0.10
    link(cloth, noise.outputs["Fac"], cb.inputs["Height"])
    link(cloth, cb.outputs["Normal"], pc.inputs["Normal"])

    # ---- rigid parts (wood=0, gold=1)
    bm = bmesh.new()
    def cone(r1, r2, z0, z1, seg, matx=None, at=(0.0, 0.0)):
        mat = Matrix.Translation(Vector((m(at[0]), m(at[1]), m((z0 + z1) / 2))))
        if matx:
            mat = mat @ matx
        bmesh.ops.create_cone(bm, cap_ends=True, segments=seg,
                              radius1=m(r1), radius2=m(r2),
                              depth=m(z1 - z0), matrix=mat)
    # plinth
    ret = bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, verts=ret["verts"],
                        matrix=Matrix.Translation(Vector((0, 0, m(19)))) @
                        Matrix.Diagonal((m(78), m(78), m(38), 1.0)).to_4x4())
    # pole 0..1100, taper 15 -> 10
    cone(15, 10, 30, 1100, 12)
    # crossarm along Y at z 1032, slightly forward of the pole axis
    arm = Matrix.Rotation(math.radians(90.0), 4, "X")
    mat = Matrix.Translation(Vector((m(14), 0, m(1032)))) @ arm
    bmesh.ops.create_cone(bm, cap_ends=True, segments=10, radius1=m(8), radius2=m(8),
                          depth=m(520), matrix=mat)
    for v in bm.verts:
        v.co.z = max(v.co.z, 0.0)
    wood_faces = len(bm.faces)
    # gold: finial cone + crossarm end caps
    cone(16, 1.5, 1100, 1168, 10)
    for ye in (-262, 262):
        ret = bmesh.ops.create_icosphere(bm, subdivisions=1, radius=m(12))
        bmesh.ops.translate(bm, verts=ret["verts"],
                            vec=Vector((m(14), m(ye), m(1032))))
    rigid = bm_to_object(bm, "rigid_tmp", [wood, gold])
    for idx, poly in enumerate(rigid.data.polygons):
        poly.material_index = 0 if idx < wood_faces else 1
        poly.use_smooth = True

    # ---- cloth sheet (shaped columns, swallowtail, sway) -> material 0 on its object
    NC, NR = 19, 23
    top_z, base_z, half_w = 1015.0, 380.0, 210.0
    verts, faces = [], []
    for i in range(NC):
        yy = -half_w + 2 * half_w * i / (NC - 1)
        notch = max(0.0, 1.0 - abs(yy) / 105.0)
        bot = base_z + 150.0 * notch
        for j in range(NR):
            t = j / (NR - 1)                     # 0 top -> 1 bottom
            zz = top_z + (bot - top_z) * t
            xx = 26.0 + 20.0 * math.sin(math.pi * t) \
                + 3.0 * math.sin(yy / 38.0 + t * 7.0)
            verts.append((m(xx), m(yy), m(zz)))
    for i in range(NC - 1):
        for j in range(NR - 1):
            a = i * NR + j
            faces.append((a, a + NR, a + NR + 1, a + 1))
    cmesh = bpy.data.meshes.new("cloth_tmp")
    cmesh.from_pydata(verts, [], faces)
    cobj = bpy.data.objects.new("cloth_tmp", cmesh)
    bpy.context.scene.collection.objects.link(cobj)
    cmesh.materials.append(cloth)
    cbm = bmesh.new()
    cbm.from_mesh(cmesh)
    bmesh.ops.recalc_face_normals(cbm, faces=cbm.faces)
    bmesh.ops.solidify(cbm, geom=list(cbm.verts) + list(cbm.edges) + list(cbm.faces),
                       thickness=m(4))
    cbm.to_mesh(cmesh)
    cbm.free()
    for poly in cmesh.polygons:
        poly.use_smooth = True

    # ArtMap on the cloth (planar y/z), plus empty layers so the join keeps names
    for objx in (rigid, cobj):
        if "UVMap" not in objx.data.uv_layers:
            objx.data.uv_layers.new(name="UVMap")
        if "ArtMap" not in objx.data.uv_layers:
            objx.data.uv_layers.new(name="ArtMap")
    artl = cobj.data.uv_layers["ArtMap"]
    for poly in cmesh.polygons:
        for li in poly.loop_indices:
            vco = cmesh.vertices[cmesh.loops[li].vertex_index].co
            artl.data[li].uv = ((vco.y * UE + half_w) / (2 * half_w),
                                (vco.z * UE - base_z) / (top_z - base_z))

    select_only([rigid, cobj], active=rigid)
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = "SM_Castle_GateBanner"
    obj.data.name = "SM_Castle_GateBanner"
    return obj, {"art_images": ["banner_art"]}


def build_tramplepath():
    """SM_Castle_TramplePath: 600x~330 ribbon, straight X ends (tileable), ruffled Y
    edges (periodic ruffle -> end profiles match), crowned top with wheel ruts."""
    art_rgba, h_rgba, masks = path_artwork()
    art_img = np_to_image("path_art", art_rgba, "sRGB")
    h_img = np_to_image("path_height", h_rgba, "Non-Color")

    mat = new_mat("path_author")
    p = principled(mat)
    uvn = add_node(mat, "ShaderNodeUVMap", location=(-950, 100))
    uvn.uv_map = "ArtMap"
    texn = add_node(mat, "ShaderNodeTexImage", location=(-700, 220))
    texn.image = art_img
    link(mat, uvn.outputs["UV"], texn.inputs["Vector"])
    link(mat, texn.outputs["Color"], p.inputs["Base Color"])
    hn = add_node(mat, "ShaderNodeTexImage", location=(-700, -120))
    hn.image = h_img
    link(mat, uvn.outputs["UV"], hn.inputs["Vector"])
    bump = add_node(mat, "ShaderNodeBump", location=(-420, -120))
    bump.inputs["Strength"].default_value = 0.45
    link(mat, hn.outputs["Color"], bump.inputs["Height"])
    link(mat, bump.outputs["Normal"], p.inputs["Normal"])
    # roughness: dirt 0.88, ruts glossier-packed 0.80 via height as cheap proxy
    ramp = add_node(mat, "ShaderNodeValToRGB", location=(-420, 60))
    ramp.color_ramp.elements[0].color = (0.80, 0.80, 0.80, 1.0)
    ramp.color_ramp.elements[1].color = (0.92, 0.92, 0.92, 1.0)
    link(mat, hn.outputs["Color"], ramp.inputs["Fac"])
    link(mat, ramp.outputs["Color"], p.inputs["Roughness"])

    L, HW = 600.0, 165.0
    NX, NY = 33, 15
    rng = np.random.RandomState(6573)
    phases = [(1, rng.uniform(0, 2 * np.pi)), (2, rng.uniform(0, 2 * np.pi)),
              (3, rng.uniform(0, 2 * np.pi)), (5, rng.uniform(0, 2 * np.pi))]

    def ruffle(xx):
        t = (xx + L / 2) / L
        val = sum(math.sin(2 * math.pi * k * t + ph) / (1.5 + k)
                  for k, ph in phases)
        return 1.0 + 0.09 * val

    def top_h(xx, yy_frac):
        crown = 1.0 - (abs(yy_frac) ** 1.6)
        rutd = sum(math.exp(-((abs(yy_frac) - 0.42) ** 2) / (2 * 0.10 ** 2))
                   for _ in (0,))
        t = (xx + L / 2) / L
        ripple = 0.35 * math.sin(2 * math.pi * 4 * t + 0.7) \
            + 0.25 * math.sin(2 * math.pi * 7 * t + 2.1)
        return max(0.6, 1.1 + 1.3 * crown - 1.5 * rutd + ripple)

    verts = []
    for i in range(NX):
        xx = -L / 2 + L * i / (NX - 1)
        r = ruffle(xx)
        for j in range(NY):
            yf = -1.0 + 2.0 * j / (NY - 1)
            yy = yf * HW * r
            verts.append((m(xx), m(yy), m(top_h(xx, yf))))
    n_top = len(verts)
    for i in range(NX):
        xx = -L / 2 + L * i / (NX - 1)
        r = ruffle(xx)
        for j in range(NY):
            yf = -1.0 + 2.0 * j / (NY - 1)
            verts.append((m(xx), m(yf * HW * r), 0.0))
    faces = []
    for i in range(NX - 1):
        for j in range(NY - 1):
            a = i * NY + j
            faces.append((a, a + NY, a + NY + 1, a + 1))
            b = n_top + i * NY + j
            faces.append((b + 1, b + NY + 1, b + NY, b))
    for i in range(NX - 1):                     # side skirts
        a0, a1 = i * NY, (i + 1) * NY
        faces.append((n_top + a0, n_top + a1, a1, a0))
        a0, a1 = i * NY + NY - 1, (i + 1) * NY + NY - 1
        faces.append((a0, a1, n_top + a1, n_top + a0))
    for j in range(NY - 1):                     # end caps (straight, tile-butt faces)
        a = j
        faces.append((a + 1, n_top + a + 1, n_top + a, a))
        a = (NX - 1) * NY + j
        faces.append((a, n_top + a, n_top + a + 1, a + 1))

    mesh = bpy.data.meshes.new("SM_Castle_TramplePath")
    mesh.from_pydata(verts, [], faces)
    obj = bpy.data.objects.new("SM_Castle_TramplePath", mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(mat)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    for poly in mesh.polygons:
        poly.use_smooth = True

    mesh.uv_layers.new(name="UVMap")
    artl = mesh.uv_layers.new(name="ArtMap")
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            vco = mesh.vertices[mesh.loops[li].vertex_index].co
            artl.data[li].uv = ((vco.x * UE + L / 2) / L,
                                (vco.y * UE + HW * 1.2) / (2 * HW * 1.2))
    return obj, {"art_images": ["path_art", "path_height"]}


def _rock(name, radii, subdiv, seed, cuts, warp=0.34):
    rng = np.random.RandomState(seed)
    field = spectral3d_factory(rng, [(2.2, 0.55), (5.0, 0.28), (11.0, 0.14)])
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    for v in bm.verts:
        n = v.co.normalized()
        disp = 1.0 + warp * field((n.x * 2.1, n.y * 2.1, n.z * 2.1))
        v.co = Vector((n.x * radii[0] * disp, n.y * radii[1] * disp,
                       n.z * radii[2] * disp))
    # angular facet chips
    for _ in range(cuts):
        direction = Vector(rng.normal(size=3)).normalized()
        dist = 0.72 + 0.18 * rng.uniform()
        co = Vector((direction.x * radii[0], direction.y * radii[1],
                     direction.z * radii[2])) * dist
        res = bmesh.ops.bisect_plane(
            bm, geom=list(bm.verts) + list(bm.edges) + list(bm.faces),
            plane_co=co, plane_no=direction, clear_outer=True, clear_inner=False)
        edges = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        if edges:
            bmesh.ops.holes_fill(bm, edges=edges, sides=64)
    # sink 22% then ground-cut at z 0
    drop = radii[2] * 0.22
    bmesh.ops.translate(bm, verts=list(bm.verts), vec=Vector((0, 0, radii[2] - drop)))
    res = bmesh.ops.bisect_plane(
        bm, geom=list(bm.verts) + list(bm.edges) + list(bm.faces),
        plane_co=Vector((0, 0, 0)), plane_no=Vector((0, 0, -1)),
        clear_outer=True, clear_inner=False)
    edges = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
    if edges:
        bmesh.ops.holes_fill(bm, edges=edges, sides=128)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def rock_material():
    mat = new_mat("toerock_author")
    p = principled(mat)
    vor = add_node(mat, "ShaderNodeTexVoronoi", location=(-800, 250))
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 5.5
    ramp = add_node(mat, "ShaderNodeValToRGB", location=(-560, 250))
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[0].color = (*PRE["rock_dark"], 1.0)
    ramp.color_ramp.elements[1].position = 0.10
    ramp.color_ramp.elements[1].color = (*PRE["rock_base"], 1.0)
    link(mat, vor.outputs["Distance"], ramp.inputs["Fac"])
    noise = add_node(mat, "ShaderNodeTexNoise", location=(-800, 0))
    noise.inputs["Scale"].default_value = 3.2
    noise.inputs["Detail"].default_value = 6.0
    mix = add_node(mat, "ShaderNodeMix", location=(-300, 220))
    mix.data_type = "RGBA"
    mix.inputs["Factor"].default_value = 0.0
    grad = add_node(mat, "ShaderNodeMix", location=(-90, 220))
    grad.data_type = "RGBA"
    # warm ground-dirt gradient near the base (position z)
    geo = add_node(mat, "ShaderNodeNewGeometry", location=(-800, -260))
    sep = add_node(mat, "ShaderNodeSeparateXYZ", location=(-620, -260))
    link(mat, geo.outputs["Position"], sep.inputs["Vector"])
    maprange = add_node(mat, "ShaderNodeMapRange", location=(-440, -260))
    maprange.inputs["From Min"].default_value = 0.0
    maprange.inputs["From Max"].default_value = 0.55
    maprange.inputs["To Min"].default_value = 1.0
    maprange.inputs["To Max"].default_value = 0.0
    link(mat, sep.outputs["Z"], maprange.inputs["Value"])
    # mix crackle ramp with noise-mottle of itself
    mmix = add_node(mat, "ShaderNodeMix", location=(-380, 90))
    mmix.data_type = "RGBA"
    mmix.inputs["Factor"].default_value = 0.35
    dark2 = tuple(c * 0.8 for c in PRE["rock_base"])
    mmix.inputs["B"].default_value = (*dark2, 1.0)
    link(mat, ramp.outputs["Color"], mmix.inputs["A"])
    link(mat, noise.outputs["Fac"], mmix.inputs["Factor"])
    grad.inputs["A"].default_value = (0, 0, 0, 1)
    link(mat, mmix.outputs["Result"], grad.inputs["A"])
    grad.inputs["B"].default_value = (*PRE["rock_warm"], 1.0)
    link(mat, maprange.outputs["Result"], grad.inputs["Factor"])
    link(mat, grad.outputs["Result"], p.inputs["Base Color"])
    p.inputs["Roughness"].default_value = 0.85
    bump = add_node(mat, "ShaderNodeBump", location=(-90, -120))
    bump.inputs["Strength"].default_value = 0.35
    link(mat, vor.outputs["Distance"], bump.inputs["Height"])
    link(mat, bump.outputs["Normal"], p.inputs["Normal"])
    return mat


def build_toerocks():
    """SM_Castle_ToeRock01 (tall cluster) + 02 (low wide slab). One shared material,
    one shared texture atlas (01 left half, 02 right half)."""
    mat = rock_material()
    r1 = _rock("SM_Castle_ToeRock01", (m(150), m(122), m(178)), 3, 65731, 6)
    small = _rock("tr01_small", (m(76), m(62), m(70)), 2, 65732, 4)
    small.location = (m(30), m(128), 0.0)
    select_only([r1, small], active=r1)
    bpy.ops.object.join()
    r1 = bpy.context.view_layer.objects.active
    r1.name = "SM_Castle_ToeRock01"
    r1.data.name = "SM_Castle_ToeRock01"
    r2 = _rock("SM_Castle_ToeRock02", (m(196), m(118), m(112)), 3, 65733, 7, warp=0.30)
    for obj in (r1, r2):
        obj.data.materials.clear()
        obj.data.materials.append(mat)
        for poly in obj.data.polygons:
            poly.use_smooth = False           # faceted granite read
    return r1, r2


# --------------------------------------------------------------------------- previews

def preview_env():
    world = bpy.context.scene.world or bpy.data.worlds.new("W")
    bpy.context.scene.world = world
    world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (0.34, 0.42, 0.58, 1.0)
    bg.inputs["Strength"].default_value = 0.35
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    bpy.context.scene.collection.objects.link(sun)
    sun.data.energy = 3.2
    sun.data.color = (1.0, 0.92, 0.78)
    sun.data.angle = math.radians(2.0)
    sun.rotation_euler = (math.radians(48), 0.0, math.radians(135))

    grass = new_mat("preview_grass")
    pg = principled(grass)
    noise = add_node(grass, "ShaderNodeTexNoise", location=(-500, 100))
    noise.inputs["Scale"].default_value = 6.0
    ramp = add_node(grass, "ShaderNodeValToRGB", location=(-280, 100))
    ramp.color_ramp.elements[0].color = (0.045, 0.105, 0.022, 1.0)
    ramp.color_ramp.elements[1].color = (0.085, 0.165, 0.035, 1.0)
    link(grass, noise.outputs["Fac"], ramp.inputs["Fac"])
    link(grass, ramp.outputs["Color"], pg.inputs["Base Color"])
    pg.inputs["Roughness"].default_value = 0.95
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=m(6000))
    ground = bm_to_object(bm, "preview_ground", [grass])
    ground.location.z = -0.001
    return [sun, ground]


def make_berm_proxy():
    """Chartreuse berm ridge (the shipped rim look) behind the toe rocks."""
    mat = new_mat("preview_berm")
    p = principled(mat)
    p.inputs["Base Color"].default_value = (0.45, 0.60, 0.085, 1.0)
    p.inputs["Roughness"].default_value = 0.9
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=48, radius1=m(900),
                          radius2=m(650), depth=m(96),
                          matrix=Matrix.Translation(Vector((0, 0, m(48)))))
    ret = bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, verts=ret["verts"],
                        matrix=Matrix.Diagonal((m(2600), m(900), m(150), 1)).to_4x4() @
                        Matrix.Translation(Vector((0, 0, 0.0))))
    obj = bm_to_object(bm, "preview_berm", [mat])
    obj.scale = (1.6, 0.55, 1.0)
    return obj


def make_human():
    mat = new_mat("preview_human")
    principled(mat).inputs["Base Color"].default_value = (0.35, 0.36, 0.40, 1.0)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=10, radius1=m(20), radius2=m(16),
                          depth=m(150), matrix=Matrix.Translation(Vector((0, 0, m(77)))))
    ret = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=m(14))
    bmesh.ops.translate(bm, verts=ret["verts"], vec=Vector((0, 0, m(166))))
    return bm_to_object(bm, "preview_human", [mat])


def render(path, cam_loc_uu, look_at_uu, lens=42, res=(960, 620)):
    scene = bpy.context.scene
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    scene.collection.objects.link(cam)
    cam.location = Vector([m(v) for v in cam_loc_uu])
    direction = Vector([m(v) for v in look_at_uu]) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = lens
    scene.camera = cam
    cycles_setup()
    scene.cycles.samples = 32
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.filepath = str(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    log(f"PREVIEW {path.name}")


# --------------------------------------------------------------------------- final material

def final_material(fam, d_path, n_path, orm_path):
    """Exactly what ships: Principled + the three FINAL PNGs (preview-render truth)."""
    mat = bpy.data.materials.new(f"{fam}PBR")
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        if n.type not in ("BSDF_PRINCIPLED", "OUTPUT_MATERIAL"):
            nt.nodes.remove(n)
    p = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    d_img = bpy.data.images.load(str(d_path))
    d_img.colorspace_settings.name = "sRGB"
    n_img = bpy.data.images.load(str(n_path))
    n_img.colorspace_settings.name = "Non-Color"
    o_img = bpy.data.images.load(str(orm_path))
    o_img.colorspace_settings.name = "Non-Color"
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
    return mat


# --------------------------------------------------------------------------- family passes

def texture_pipeline(fam, objs, report_slot, intent_weights, rock_split=False):
    """Bake D/N/R/AO for objs into one atlas set, delight, pack ORM, write PNGs."""
    d_img = new_image(f"{fam}_D_pre", BAKE_RES, "sRGB")
    n_img = new_image(f"{fam}_N", BAKE_RES, "Non-Color")
    r_img = new_image(f"{fam}_R", BAKE_RES, "Non-Color")
    ao_img = new_image(f"{fam}_AO", BAKE_RES, "Non-Color")

    for k, obj in enumerate(objs):
        clear = k == 0
        log(f"{fam}: bake DIFFUSE({obj.name})")
        bake_pass(obj, d_img, "DIFFUSE", SAMPLES, clear, pass_filter={"COLOR"})
        log(f"{fam}: bake NORMAL({obj.name})")
        bake_pass(obj, n_img, "NORMAL", SAMPLES, clear)
        log(f"{fam}: bake ROUGHNESS({obj.name})")
        bake_pass(obj, r_img, "ROUGHNESS", SAMPLES, clear)
        log(f"{fam}: bake AO({obj.name})")
        bake_ao(obj, ao_img, clear)

    DEBUG.mkdir(parents=True, exist_ok=True)
    save_image(d_img, DEBUG / f"T_Castle_{fam}_D_predelight.png")
    save_image(ao_img, DEBUG / f"T_Castle_{fam}_AO.png")
    save_image(r_img, DEBUG / f"T_Castle_{fam}_R.png")

    final_d = apply_delight(d_img, ao_img, report_slot)
    orm = pack_orm(ao_img, r_img)

    RAWTEX.mkdir(parents=True, exist_ok=True)
    d_path = RAWTEX / f"T_Castle_{fam}_D.png"
    n_path = RAWTEX / f"T_Castle_{fam}_N.png"
    orm_path = RAWTEX / f"T_Castle_{fam}_ORM.png"
    save_image(np_to_image(f"{fam}_D_final", final_d, "sRGB"), d_path)
    save_image(n_img, n_path)
    save_image(np_to_image(f"{fam}_ORM", orm, "Non-Color"), orm_path)

    measure_gates(final_d, orm, POST, intent_weights, report_slot)
    report_slot["textures"] = {
        p.name: {"sha256": sha256(p), "bytes": p.stat().st_size}
        for p in (d_path, n_path, orm_path)
    }
    return d_path, n_path, orm_path


def finish_family(fam, objs, d_path, n_path, orm_path, slot_name, report_slot):
    """Swap to the shipping material (single slot), strip ArtMap, measure."""
    mat = final_material(slot_name.replace("PBR", ""), d_path, n_path, orm_path)
    mat.name = slot_name
    for obj in objs:
        smooth_flags = [p.use_smooth for p in obj.data.polygons]
        obj.data.materials.clear()
        obj.data.materials.append(mat)
        for poly, sm in zip(obj.data.polygons, smooth_flags):
            poly.material_index = 0
            poly.use_smooth = sm
        art = obj.data.uv_layers.get("ArtMap")
        if art:
            obj.data.uv_layers.remove(art)
        report_slot.setdefault("meshes", {})[obj.name] = measure(obj)


def do_gatebanner(report):
    slot = report["families"].setdefault("GateBanner", {})
    obj, _ = build_gatebanner()
    uv_atlas(obj)
    weights = {"banner_crimson": 0.50, "banner_gold": 0.20,
               "banner_wood": 0.28, "rock_dark": 0.02}
    d, n, o = texture_pipeline("GateBanner", [obj], slot, weights)
    finish_family("GateBanner", [obj], d, n, o, "GateBannerPBR", slot)

    helpers = preview_env()
    human = make_human()
    human.location = (m(150), m(-260), 0)
    render(PREVIEWS / "GateBanner_threequarter.png",
           (900, -700, 330), (0, 0, 560), lens=45)
    render(PREVIEWS / "GateBanner_front_far.png",
           (2500, 0, 165), (0, 0, 520), lens=50)
    render(PREVIEWS / "GateBanner_detail.png",
           (420, -280, 720), (20, 0, 780), lens=50)
    for h in helpers + [human]:
        bpy.data.objects.remove(h, do_unlink=True)

    fbx = export_fbx([obj], RAW / "Castle_GateBanner.fbx")
    slot["fbx"] = {"path": str(fbx.relative_to(ROOT)), "sha256": sha256(fbx),
                   "bytes": fbx.stat().st_size}


def do_tramplepath(report):
    slot = report["families"].setdefault("TramplePath", {})
    obj, _ = build_tramplepath()
    uv_atlas(obj)
    weights = {"path_dirt": 0.62, "path_rut": 0.22, "path_edge": 0.16}
    d, n, o = texture_pipeline("TramplePath", [obj], slot, weights)
    finish_family("TramplePath", [obj], d, n, o, "TramplePathPBR", slot)

    helpers = preview_env()
    human = make_human()
    human.location = (m(-180), m(-320), 0)
    # three chained segments -- the tiling check
    copies = []
    for k in (-1, 1):
        dup = obj.copy()
        bpy.context.scene.collection.objects.link(dup)
        dup.location.x = k * m(600)
        copies.append(dup)
    render(PREVIEWS / "TramplePath_chain_threequarter.png",
           (500, -900, 420), (0, 0, 0), lens=40)
    render(PREVIEWS / "TramplePath_walk_view.png",
           (-800, 0, 170), (600, 0, 0), lens=38)
    render(PREVIEWS / "TramplePath_detail.png",
           (220, -260, 260), (80, 0, 0), lens=45)
    for h in helpers + [human] + copies:
        bpy.data.objects.remove(h, do_unlink=True)

    fbx = export_fbx([obj], RAW / "Castle_TramplePath.fbx")
    slot["fbx"] = {"path": str(fbx.relative_to(ROOT)), "sha256": sha256(fbx),
                   "bytes": fbx.stat().st_size}


def do_toerock(report):
    slot = report["families"].setdefault("ToeRock", {})
    r1, r2 = build_toerocks()
    uv_atlas(r1)
    uv_atlas(r2)
    scale_uvs(r1, 0.47, 0.0)
    scale_uvs(r2, 0.47, 0.53)
    weights = {"rock_base": 0.62, "rock_dark": 0.18, "rock_warm": 0.20}
    d, n, o = texture_pipeline("ToeRock", [r1, r2], slot, weights, rock_split=True)
    finish_family("ToeRock", [r1, r2], d, n, o, "ToeRockPBR", slot)

    helpers = preview_env()
    berm = make_berm_proxy()
    berm.location = (m(-700), 0, 0)
    human = make_human()
    human.location = (m(320), m(-420), 0)
    # a dressed line: alternate 01/02 with yaw jitter (the 661 spacing ~200)
    line = []
    rng = np.random.RandomState(6574)
    for idx, yy in enumerate(range(-500, 501, 200)):
        src = r1 if idx % 2 == 0 else r2
        dup = src.copy()
        bpy.context.scene.collection.objects.link(dup)
        dup.location = (0.0, m(yy), 0.0)
        dup.rotation_euler = (0, 0, rng.uniform(0, 2 * math.pi))
        line.append(dup)
    r1.hide_render = True
    r2.hide_render = True
    render(PREVIEWS / "ToeRock_sealline_vs_berm.png",
           (1150, -650, 240), (-150, 0, 100), lens=40)
    render(PREVIEWS / "ToeRock_walkup_view.png",
           (950, 0, 170), (-400, 0, 110), lens=38)
    r1.hide_render = False
    r2.hide_render = False
    for dup in line:
        bpy.data.objects.remove(dup, do_unlink=True)
    render(PREVIEWS / "ToeRock_pair_detail.png",
           (620, -520, 300), (0, 60, 130), lens=45)
    for h in helpers + [human, berm]:
        bpy.data.objects.remove(h, do_unlink=True)

    f1 = export_fbx([r1], RAW / "Castle_ToeRock01.fbx")
    f2 = export_fbx([r2], RAW / "Castle_ToeRock02.fbx")
    slot["fbx"] = {}
    for f in (f1, f2):
        slot["fbx"][f.name] = {"path": str(f.relative_to(ROOT)), "sha256": sha256(f),
                               "bytes": f.stat().st_size}


# --------------------------------------------------------------------------- roundtrip probe

def probe_fbx(path):
    """Re-import + UE handedness emulation (diag(1,-1,1)) -- what the EDITOR gets."""
    wipe_scene()
    bpy.ops.import_scene.fbx(filepath=str(path))
    out = {}
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        select_only([obj])
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        obj.data.transform(Matrix.Diagonal((1.0, -1.0, 1.0, 1.0)))
        obj.data.update()
        out[obj.name] = measure(obj)
    return out


# --------------------------------------------------------------------------- main

def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--family", default="all",
                        choices=["GateBanner", "TramplePath", "ToeRock", "all"])
    args = parser.parse_args(argv)

    CACHE.mkdir(parents=True, exist_ok=True)
    report_path = CACHE / "props_report.json"
    if report_path.exists():
        report = json.loads(report_path.read_text(encoding="utf-8"))
    else:
        report = {"task": "TASK-657", "families": {}, "delight_profile": DELIGHT,
                  "albedo_floor": ALBEDO_FLOOR,
                  "pivot_law": "ground contact (min Z 0), +X forward (TASK-661 contract)",
                  "collision": "COLLISIONLESS ON PURPOSE (no UCX authored; manifest ucx null)"}

    jobs = {"GateBanner": do_gatebanner, "TramplePath": do_tramplepath,
            "ToeRock": do_toerock}
    run = list(jobs) if args.family == "all" else [args.family]
    for fam in run:
        log(f"=== {fam} ===")
        wipe_scene()
        jobs[fam](report)
        # roundtrip probe on the exported FBX(s)
        slot = report["families"][fam]
        probes = {}
        fbx_entries = slot.get("fbx", {})
        if "sha256" in fbx_entries:
            probes.update(probe_fbx(ROOT / fbx_entries["path"]))
        else:
            for entry in fbx_entries.values():
                probes.update(probe_fbx(ROOT / entry["path"]))
        slot["roundtrip_probe_ue_space"] = probes
        report_path.write_text(json.dumps(report, indent=1), encoding="utf-8")
        log(f"{fam} report written")

    log("DONE")


if __name__ == "__main__":
    main()
