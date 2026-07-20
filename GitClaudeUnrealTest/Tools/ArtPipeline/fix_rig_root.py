"""
fix_rig_root.py — Siegebound art pipeline, TASK-212 batch armature-root fix.

Root cause (handoffs/TASK-211.md, evidence-verified): each unit rig FBX exported its
Blender armature OBJECT as '<Unit>_Rig'. UE's FBX importer converts that node into an
extra ROOT bone, so every SK_<Unit> roots at '<Unit>_Rig' != the shared skeleton's root
'Footman_Rig', and USkeleton::MergeAllBonesToBoneTree silently fails on every load (the
recurring missing-bones Message Log warnings).

This script renames the armature OBJECT to the constant 'Footman_Rig' IN PLACE for the
8 existing non-Footman rig FBXs — and ONLY that. Bone hierarchy/names, mesh geometry,
the two-slot material contract [TeamRegion, <Unit>PBR] and the 'UVMap' layer are
untouched and machine-verified pre/post.

Runs INSIDE Blender's bundled Python (bpy + stdlib ONLY), headless:

    "<blender.exe>" --background --factory-startup --python-exit-code 1 ^
        --python Tools/ArtPipeline/fix_rig_root.py -- [flags]

Flags (after the "--" separator):
    --units A,B,C     comma list of unit CardIDs to process (default: the 8 from
                      TASK-211: Pikeman,Cleric,Longbowman,MilitiaMob,Miner,Sapper,
                      Cavalry,Knight). Footman is ALWAYS given a read-only root check
                      and is never rewritten.
    --verify-only     inspect + report only; no backups, no writes.
    --force           rewrite even if the armature object is already 'Footman_Rig'.

Per-unit flow (fix mode):
    1. BACKUP   Content/RawAssets/Characters/<Unit>.fbx ->
                Tools/ArtPipeline/Cache/RigRootFix/backup/<Unit>.fbx  (sha256 recorded;
                Cache/ is gitignored so backups never ride a commit).
    2. IMPORT   the FBX into a factory-empty scene; snapshot armature (object/data name,
                every bone's name/parent/connect/deform/head/tail) + mesh stats (verts,
                polys, tris, material slots, UV layers, vertex groups, world matrix).
    3. RENAME   armature object -> 'Footman_Rig'. If the importer also stamped the
                armature DATA block '<Unit>_Rig', the data block is renamed
                '<Unit>_Armature' (rig_character.py's data naming) so no '<Unit>_Rig'
                string survives anywhere in the file. Nothing else is modified.
    4. EXPORT   to Cache/RigRootFix/out/<Unit>.fbx with the EXACT exporter block of
                rig_character.export_skeletal_fbx (axis_forward=-Z, axis_up=Y,
                apply_unit_scale, FBX_SCALE_NONE, FACE smoothing, no leaf bones,
                bake_anim=False, primary/secondary bone axis Y/X, path_mode STRIP).
    5. VERIFY   re-import the exported file in a fresh scene and assert: armature object
                'Footman_Rig'; bone set/hierarchy/rest pose IDENTICAL (1e-4 m tolerance
                on head/tail); mesh verts/polys/tris/slots/UVs/vgroups identical.
                Byte-scan the file: b'Footman_Rig' present, b'<Unit>_Rig' absent.
                (use_connect flips are recorded as non-fatal notes — the flag is not
                serialized in FBX; see compare_snapshots.)
    6. REPLACE  os.replace the verified temp file onto the Content path (the original is
                never overwritten by an unverified export), then re-run the byte scan on
                the in-place file.

Report: Cache/RigRootFix/fix_report.json (fix mode) / verify_report.json (--verify-only),
per-unit pre/post snapshots + verdicts.
Exit codes: 0 = all PASS, 1 = any FAIL, 2 = usage/input error.
"""

import argparse
import hashlib
import json
import os
import shutil
import sys
import time
import traceback
from datetime import datetime, timezone
from pathlib import Path

try:
    import bpy
except ImportError:
    print("[fixroot] FATAL: bpy not importable — run inside Blender "
          "(blender.exe --background --factory-startup --python fix_rig_root.py -- ...)")
    sys.exit(2)

# --------------------------------------------------------------------------- paths / law
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parents[1]
CHARACTERS_RAW = PROJECT_ROOT / "Content" / "RawAssets" / "Characters"
CACHE_DIR = SCRIPT_DIR / "Cache" / "RigRootFix"
BACKUP_DIR = CACHE_DIR / "backup"
OUT_TMP_DIR = CACHE_DIR / "out"

SHARED_SKELETON_ROOT = "Footman_Rig"   # must match rig_character.SHARED_SKELETON_ROOT
EXPECTED_BONE_COUNT = 21               # SiegeBiped contract
EXPECTED_ROOT_BONE = "root"

DEFAULT_UNITS = ["Pikeman", "Cleric", "Longbowman", "MilitiaMob",
                 "Miner", "Sapper", "Cavalry", "Knight"]

POS_TOL = 1e-4   # meters — rest-pose head/tail + matrix tolerance across the round trip


def log(msg):
    print(f"[fixroot] {msg}", flush=True)


def fail(msg, code=1):
    print(f"[fixroot] FATAL: {msg}", flush=True)
    sys.exit(code)


def guard_write(path):
    """Write confinement: only Cache/RigRootFix/** and Content/RawAssets/Characters/**."""
    rp = Path(path).resolve()
    allowed = [CACHE_DIR.resolve(), CHARACTERS_RAW.resolve()]
    if any(part.lower() == "cardart" for part in rp.parts):
        fail(f"write-confinement violation: {rp} is inside a CardArt lane")
    if not any(rp == root or root in rp.parents for root in allowed):
        fail(f"write-confinement violation: {rp} outside {[str(a) for a in allowed]}")
    rp.parent.mkdir(parents=True, exist_ok=True)
    return rp


# --------------------------------------------------------------------------- helpers
def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    p = argparse.ArgumentParser(prog="fix_rig_root.py")
    p.add_argument("--units", default=",".join(DEFAULT_UNITS))
    p.add_argument("--verify-only", action="store_true")
    p.add_argument("--force", action="store_true")
    return p.parse_args(argv)


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def fresh_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def ensure_object_mode():
    a = bpy.context.view_layer.objects.active
    if a is not None and a.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


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
    imported = [o for o in bpy.data.objects if o not in before]
    arms = [o for o in imported if o.type == "ARMATURE"]
    meshes = [o for o in imported if o.type == "MESH"]
    if len(arms) != 1:
        fail(f"{path.name}: expected exactly 1 armature object, got "
             f"{[a.name for a in arms]}")
    if not meshes:
        fail(f"{path.name}: no mesh objects imported")
    return arms[0], meshes


def _round_vec(v, nd=6):
    return [round(float(c), nd) for c in v]


def _round_mat(m, nd=6):
    return [[round(float(c), nd) for c in row] for row in m]


def snapshot(arm, meshes):
    """Everything the rig contract cares about, for pre/post comparison."""
    bones = {}
    for b in arm.data.bones:
        bones[b.name] = {
            "parent": b.parent.name if b.parent else None,
            "use_deform": bool(b.use_deform),
            "use_connect": bool(b.use_connect),
            "head": _round_vec(b.head_local),
            "tail": _round_vec(b.tail_local),
        }
    mesh_snaps = []
    for m in sorted(meshes, key=lambda o: o.name):
        m.data.calc_loop_triangles()
        mesh_snaps.append({
            "object": m.name,
            "verts": len(m.data.vertices),
            "polys": len(m.data.polygons),
            "tris": len(m.data.loop_triangles),
            "material_slots": [s.material.name if s.material else None
                               for s in m.material_slots],
            "uv_layers": [uv.name for uv in m.data.uv_layers],
            "vertex_groups": sorted(vg.name for vg in m.vertex_groups),
            "matrix_world": _round_mat(m.matrix_world),
        })
    return {
        "armature_object": arm.name,
        "armature_data": arm.data.name,
        "armature_matrix_world": _round_mat(arm.matrix_world),
        "bone_count": len(bones),
        "bones": bones,
        "meshes": mesh_snaps,
    }


def _vec_close(a, b, tol=POS_TOL):
    return all(abs(x - y) <= tol for x, y in zip(a, b))


def _mat_close(a, b, tol=POS_TOL):
    return all(_vec_close(ra, rb, tol) for ra, rb in zip(a, b))


def compare_snapshots(pre, post, problems, notes):
    """post must equal pre in everything except the armature object/data names.

    use_connect is NON-FATAL (-> notes): it is not serialized in FBX at all — it is
    Blender's IMPORT-side inference (child head coinciding with parent tail within an
    exact epsilon), which can flip on sub-1e-6 float drift even when head/tail round-trip
    identically (they are compared FATALLY right below, at POS_TOL). UE builds the ref
    skeleton from the bone node transforms and never sees a connect flag, so a flipped
    inference with identical transforms is cosmetic to every consumer of these files.
    """
    if set(pre["bones"]) != set(post["bones"]):
        problems.append(f"bone SET changed: only-pre={sorted(set(pre['bones']) - set(post['bones']))} "
                        f"only-post={sorted(set(post['bones']) - set(pre['bones']))}")
    else:
        for name, pb in pre["bones"].items():
            qb = post["bones"][name]
            if pb["parent"] != qb["parent"]:
                problems.append(f"bone '{name}' parent {pb['parent']} -> {qb['parent']}")
            if pb["use_deform"] != qb["use_deform"]:
                problems.append(f"bone '{name}' use_deform changed")
            if pb["use_connect"] != qb["use_connect"]:
                notes.append(f"bone '{name}' import-side use_connect inference flipped "
                             f"{pb['use_connect']} -> {qb['use_connect']} (non-fatal; "
                             f"transforms verified identical)")
            if not _vec_close(pb["head"], qb["head"]) or not _vec_close(pb["tail"], qb["tail"]):
                problems.append(f"bone '{name}' rest pose moved > {POS_TOL} m: "
                                f"head {pb['head']}->{qb['head']} tail {pb['tail']}->{qb['tail']}")
    if not _mat_close(pre["armature_matrix_world"], post["armature_matrix_world"]):
        problems.append("armature world matrix changed")
    if len(pre["meshes"]) != len(post["meshes"]):
        problems.append(f"mesh object count {len(pre['meshes'])} -> {len(post['meshes'])}")
        return
    for pm, qm in zip(pre["meshes"], post["meshes"]):
        for key in ("object", "verts", "polys", "tris", "material_slots",
                    "uv_layers", "vertex_groups"):
            if pm[key] != qm[key]:
                problems.append(f"mesh '{pm['object']}' {key}: {pm[key]} -> {qm[key]}")
        if not _mat_close(pm["matrix_world"], qm["matrix_world"]):
            problems.append(f"mesh '{pm['object']}' world matrix changed")


def byte_scan(path, unit, problems, where):
    data = Path(path).read_bytes()
    if SHARED_SKELETON_ROOT.encode() not in data:
        problems.append(f"{where}: b'{SHARED_SKELETON_ROOT}' NOT found in file bytes")
    bad = f"{unit}_Rig".encode()
    if unit != "Footman" and bad in data:
        problems.append(f"{where}: stale b'{unit}_Rig' still present in file bytes")


def export_fbx_like_rig_character(arm, meshes, out_path):
    """EXACT mirror of rig_character.export_skeletal_fbx's exporter block."""
    out = guard_write(out_path)
    ensure_object_mode()
    select_only([arm] + list(meshes), active=arm)
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
    return out


def contract_checks(unit, snap, problems, expect_root_name):
    if snap["bone_count"] != EXPECTED_BONE_COUNT:
        problems.append(f"bone count {snap['bone_count']} != {EXPECTED_BONE_COUNT}")
    roots = [n for n, b in snap["bones"].items() if b["parent"] is None]
    if roots != [EXPECTED_ROOT_BONE]:
        problems.append(f"root BONE(s) {roots} != ['{EXPECTED_ROOT_BONE}']")
    if SHARED_SKELETON_ROOT in snap["bones"]:
        problems.append(f"'{SHARED_SKELETON_ROOT}' unexpectedly exists as a BONE")
    if expect_root_name is not None and snap["armature_object"] != expect_root_name:
        problems.append(f"armature object '{snap['armature_object']}' != '{expect_root_name}'")


# --------------------------------------------------------------------------- per-unit
def process_unit(unit, verify_only, force):
    src = CHARACTERS_RAW / f"{unit}.fbx"
    if not src.is_file():
        fail(f"missing rig FBX: {src}", code=2)
    row = {"unit": unit, "path": str(src), "size_before": src.stat().st_size,
           "sha256_before": sha256_of(src)}
    problems = []

    fresh_scene()
    arm, meshes = import_fbx(src)
    pre = snapshot(arm, meshes)
    row["pre"] = pre
    contract_checks(unit, pre, problems, expect_root_name=None)
    log(f"{unit}: armature object '{pre['armature_object']}' (data '{pre['armature_data']}'), "
        f"{pre['bone_count']} bones, mesh "
        f"{[ (m['object'], m['verts'], m['tris']) for m in pre['meshes'] ]}, "
        f"slots {pre['meshes'][0]['material_slots']}")

    already_ok = pre["armature_object"] == SHARED_SKELETON_ROOT
    if verify_only:
        byte_scan(src, unit, problems, "verify-only scan")
        if not already_ok:
            problems.append(f"armature object is '{pre['armature_object']}', "
                            f"needs '{SHARED_SKELETON_ROOT}'")
        row["status"] = "PASS" if not problems else "FAIL"
        row["problems"] = problems
        return row

    expected_old = f"{unit}_Rig"
    if not already_ok and pre["armature_object"] != expected_old:
        problems.append(f"unexpected armature object name '{pre['armature_object']}' "
                        f"(expected '{expected_old}' or '{SHARED_SKELETON_ROOT}') — refusing to touch")
        row["status"] = "FAIL"
        row["problems"] = problems
        return row

    if already_ok and not force:
        byte_scan(src, unit, problems, "already-ok scan")
        row["status"] = "ALREADY_OK" if not problems else "FAIL"
        row["problems"] = problems
        log(f"{unit}: already '{SHARED_SKELETON_ROOT}' — left untouched")
        return row

    # 1. backup
    backup = guard_write(BACKUP_DIR / f"{unit}.fbx")
    shutil.copy2(src, backup)
    row["backup"] = str(backup)
    row["backup_sha256"] = sha256_of(backup)
    if row["backup_sha256"] != row["sha256_before"]:
        fail(f"{unit}: backup sha mismatch — aborting before any write")

    # 2. rename the armature OBJECT (and only that; data block only if it carries '<Unit>_Rig')
    arm.name = SHARED_SKELETON_ROOT
    if arm.name != SHARED_SKELETON_ROOT:  # collision suffix — impossible in a fresh scene
        fail(f"{unit}: object rename collided -> '{arm.name}'")
    row["renamed_object"] = f"{expected_old} -> {SHARED_SKELETON_ROOT}"
    if expected_old in arm.data.name:
        old_data = arm.data.name
        arm.data.name = f"{unit}_Armature"
        row["renamed_data_block"] = f"{old_data} -> {arm.data.name}"
        log(f"{unit}: armature DATA block '{old_data}' -> '{arm.data.name}' "
            f"(purges the stale string from the file; not a node UE reads)")

    # 3. export to temp, NEVER directly over the original
    tmp_out = export_fbx_like_rig_character(arm, meshes, OUT_TMP_DIR / f"{unit}.fbx")
    log(f"{unit}: exported temp {tmp_out}")

    # 4. verify the temp file before it replaces anything
    notes = []
    fresh_scene()
    arm2, meshes2 = import_fbx(tmp_out)
    post = snapshot(arm2, meshes2)
    row["post"] = post
    contract_checks(unit, post, problems, expect_root_name=SHARED_SKELETON_ROOT)
    compare_snapshots(pre, post, problems, notes)
    byte_scan(tmp_out, unit, problems, "temp-file scan")
    row["notes"] = notes
    for n in notes:
        log(f"{unit}: NOTE {n}")

    if problems:
        row["status"] = "FAIL"
        row["problems"] = problems
        log(f"{unit}: FAIL — original left untouched ({len(problems)} problem(s))")
        return row

    # 5. atomic replace + final in-place scan
    dst = guard_write(src)
    os.replace(tmp_out, dst)
    byte_scan(dst, unit, problems, "in-place scan")
    row["size_after"] = dst.stat().st_size
    row["sha256_after"] = sha256_of(dst)
    row["status"] = "PASS" if not problems else "FAIL"
    row["problems"] = problems
    log(f"{unit}: {row['status']} — {dst.name} rewritten "
        f"({row['size_before']} -> {row['size_after']} bytes)")
    return row


def check_footman():
    """Read-only: Footman.fbx must ALREADY root at Footman_Rig. Never rewritten."""
    src = CHARACTERS_RAW / "Footman.fbx"
    if not src.is_file():
        fail(f"missing rig FBX: {src}", code=2)
    problems = []
    fresh_scene()
    arm, meshes = import_fbx(src)
    snap = snapshot(arm, meshes)
    contract_checks("Footman", snap, problems, expect_root_name=SHARED_SKELETON_ROOT)
    byte_scan(src, "Footman", problems, "footman scan")
    row = {"unit": "Footman", "path": str(src), "mode": "read-only check",
           "pre": snap, "status": "PASS" if not problems else "FAIL",
           "problems": problems}
    log(f"Footman: read-only check {row['status']} "
        f"(armature object '{snap['armature_object']}', {snap['bone_count']} bones)")
    return row


# --------------------------------------------------------------------------- main
def main():
    args = parse_args()
    units = [u.strip() for u in args.units.split(",") if u.strip()]
    if "Footman" in units:
        fail("Footman is check-only and may not be listed in --units", code=2)
    log(f"units={units} verify_only={args.verify_only} force={args.force}")
    log(f"blender={bpy.app.version_string}")

    report = {
        "task": "TASK-212",
        "blender_version": bpy.app.version_string,
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "shared_skeleton_root": SHARED_SKELETON_ROOT,
        "verify_only": args.verify_only,
        "rows": [],
    }
    started = time.time()

    report["rows"].append(check_footman())
    for unit in units:
        report["rows"].append(process_unit(unit, args.verify_only, args.force))

    report["elapsed_seconds"] = round(time.time() - started, 1)
    verdicts = {r["unit"]: r["status"] for r in report["rows"]}
    report["verdicts"] = verdicts
    report_name = "verify_report.json" if args.verify_only else "fix_report.json"
    report_path = guard_write(CACHE_DIR / report_name)
    with open(report_path, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
    log(f"REPORT: {report_path}")
    log("VERDICTS: " + ", ".join(f"{u}={s}" for u, s in verdicts.items()))
    if any(s == "FAIL" for s in verdicts.values()):
        fail("one or more units FAILED — originals with failures were left untouched")
    log(f"DONE in {report['elapsed_seconds']}s")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        fail("unhandled exception (see traceback above)")
    sys.exit(0)
