"""verify_watchtower_fbx.py -- TASK-737 round-trip probe.

Re-imports the EXPORTED Content/RawAssets/WatchTower.fbx into a clean headless
Blender and re-measures the shipped contract FROM THE FILE. This is the check that
catches a silent export-time failure: the build script can be right in every digit
and still ship an FBX whose sockets, hull names or axes did not survive
serialisation.

    "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background \
        --factory-startup --python-exit-code 1 \
        --python Tools/ArtPipeline/verify_watchtower_fbx.py

Writes Tools/ArtPipeline/Cache/WatchTower/roundtrip_report.json and prints a
PASS/FAIL line per contract row. Exit code 1 if any row fails.
"""

import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
FBX = ROOT / "Content" / "RawAssets" / "WatchTower.fbx"
OUT = ROOT / "Tools" / "ArtPipeline" / "Cache" / "WatchTower" / "roundtrip_report.json"

NODE = "SM_WatchTower"
UE = 100.0

# the pinned contract, restated here ONLY so the probe is independent of the build
# script's own constants -- a verifier that imports the thing it verifies proves
# nothing.
#
# TASK-783 (2026-09-02): both sockets moved OUTWARD by (-10, 0, 0) on Jonathan's K-1
# ruling (option A), at the magnitude CONTACT-7a's banner pins. The hero capsule is
# r 42 (NOT the nav agent's 34), which left only 51.61875 uu of standoff at the old
# coordinates and VOIDED TOWER-8.5a for the hero. This is a PURE TRANSLATION: Delta
# stays (300, 0, 1200), so the length, the lean and the -22.0 uu rung plane are all
# untouched. Re-measured after: spine 103.3201, hero 61.3201.
EXPECT_SOCKETS = {"LadderFoot": (-460.0, 0.0, 0.0), "LadderTop": (-160.0, 0.0, 1200.0)}
EXPECT_CLIMB_DELTA = (300.0, 0.0, 1200.0)   # the property the translation preserves
EXPECT_HULLS = 8
EXPECT_SLOTS = ["TeamRegion", "WatchTowerPBR"]
EXPECT_UV = ["UVMap"]
DECK_Z = 1200.0
DECK_HALF = 300.0
BODY_HALF = 300.0


def main():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    if not FBX.exists():
        print("FAIL  fbx missing: %s" % FBX)
        sys.exit(1)

    bpy.ops.import_scene.fbx(filepath=str(FBX))

    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    empties = [o for o in bpy.data.objects if o.type == "EMPTY"]
    render_nodes = sorted(o.name for o in meshes if not o.name.startswith("UCX_"))
    ucx = sorted(o.name for o in meshes if o.name.startswith("UCX_"))

    rep = {"fbx": str(FBX), "bytes": FBX.stat().st_size}
    rows = []

    def check(key, got, ok, note=""):
        rows.append((key, got, bool(ok), note))
        rep[key] = got

    # ---- 1. the TL-2 binding contract: EXACTLY ONE render node -----------------
    check("render_nodes", render_nodes, render_nodes == [NODE],
          "UCX binds only when there is exactly one render node named SM_WatchTower")
    check("ucx_nodes", ucx,
          len(ucx) == EXPECT_HULLS
          and ucx[0] == f"UCX_{NODE}_00"
          and ucx[-1] == f"UCX_{NODE}_{EXPECT_HULLS - 1:02d}",
          f"expect {EXPECT_HULLS} hulls named UCX_{NODE}_NN")

    obj = bpy.data.objects.get(NODE)
    if obj is None:
        print("FAIL  render node %s absent after import" % NODE)
        json.dump(rep, open(OUT, "w"), indent=2)
        sys.exit(1)

    # ---- 2. the sockets -- the artist->programmer seam --------------------------
    got_sockets = {}
    for e in empties:
        name = e.name
        if name.startswith("SOCKET_"):
            w = e.matrix_world.translation * UE
            got_sockets[name[len("SOCKET_"):]] = [round(w.x, 4), round(w.y, 4),
                                                  round(w.z, 4)]
    ok_sock = True
    for want_name, want_xyz in EXPECT_SOCKETS.items():
        got = got_sockets.get(want_name)
        if got is None or max(abs(g - w) for g, w in zip(got, want_xyz)) > 0.01:
            ok_sock = False
    check("sockets_world_uu", got_sockets, ok_sock and len(got_sockets) == 2,
          "names are a literal FName contract: LadderFoot / LadderTop, PascalCase, "
          "no prefix, no underscore -- UE strips the SOCKET_ node prefix")
    check("socket_parents", {e.name: (e.parent.name if e.parent else None)
                             for e in empties},
          all(e.parent is not None and e.parent.name == NODE for e in empties
              if e.name.startswith("SOCKET_")),
          "a socket node must be a CHILD of the mesh node or UE ignores it")

    # TASK-783: the climb line is the DIFFERENCE of the two sockets, and it is what
    # the traversal, the watchdog budget and the clip all actually depend on. A
    # translation that moved only ONE socket would pass every row above and silently
    # change the length, the lean and the 8.5a window percentage. Read it from the
    # FILE and check the vector itself.
    fs = got_sockets.get("LadderFoot")
    tsk = got_sockets.get("LadderTop")
    if fs and tsk:
        dlt = [tsk[i] - fs[i] for i in range(3)]
        length = math.sqrt(sum(c * c for c in dlt))
        lean = math.degrees(math.atan2(dlt[2], math.hypot(dlt[0], dlt[1])))
        check("climb_delta_uu", [round(c, 4) for c in dlt],
              max(abs(dlt[i] - EXPECT_CLIMB_DELTA[i]) for i in range(3)) < 0.01,
              "Delta is the invariant a PURE TRANSLATION must preserve: move both "
              "sockets by the same vector or the length, the lean, sin(theta) and "
              "TOWER-8.5a's window percentage all change")
        check("climb_length_uu", round(length, 4), abs(length - 1236.9317) < 0.01)
        check("climb_lean_deg", round(lean, 4), abs(lean - 75.9638) < 0.01)
    else:
        check("climb_delta_uu", None, False, "both sockets are required")

    # ---- 3. mesh identity -------------------------------------------------------
    mesh = obj.data
    xs = [v.co.x * UE for v in mesh.vertices]
    ys = [v.co.y * UE for v in mesh.vertices]
    zs = [v.co.z * UE for v in mesh.vertices]
    check("uv_layers", [l.name for l in mesh.uv_layers],
          [l.name for l in mesh.uv_layers] == EXPECT_UV, "UE reads UVMap by name")
    check("material_slots", [s.material.name if s.material else None
                             for s in obj.material_slots],
          [s.material.name.split(".")[0] if s.material else None
           for s in obj.material_slots] == EXPECT_SLOTS,
          "slot ORDER is the contract: TeamRegion first")
    check("tris", sum(len(p.vertices) - 2 for p in mesh.polygons), True)
    check("bounds_uu", [round(min(xs), 3), round(min(ys), 3), round(min(zs), 3),
                        round(max(xs), 3), round(max(ys), 3), round(max(zs), 3)],
          abs(min(zs)) < 0.01, "min_z must be 0: the pivot is base-centre, on grade")

    # ---- 4. the deck: measured from the ROUND-TRIPPED collision -----------------
    hulls = [bpy.data.objects[n] for n in ucx]
    verts, polys = [], []
    for h in hulls:
        base = len(verts)
        verts += [tuple(v.co) for v in h.data.vertices]
        polys += [tuple(base + i for i in p.vertices) for p in h.data.polygons]
    from mathutils.bvhtree import BVHTree
    bvh = BVHTree.FromPolygons(verts, polys, all_triangles=False, epsilon=0.0)

    def top(x, y, from_z=2600.0):
        hit = bvh.ray_cast(Vector((x / UE, y / UE, from_z / UE)), Vector((0, 0, -1)))
        return None if hit[0] is None else hit[0].z * UE

    devs, misses = [], 0
    n = 41
    for i in range(n):
        for j in range(n):
            x = -DECK_HALF + 3.0 + (2 * DECK_HALF - 6.0) * i / (n - 1)
            y = -DECK_HALF + 3.0 + (2 * DECK_HALF - 6.0) * j / (n - 1)
            z = top(x, y)
            if z is None:
                misses += 1
            else:
                devs.append(abs(z - DECK_Z))
    check("deck_collision_max_dev_uu", round(max(devs), 6) if devs else None,
          bool(devs) and max(devs) < 0.01 and misses == 0,
          "the deck's +Z collision face must BE z = 1200, with no hole")
    check("deck_collision_misses", misses, misses == 0)

    z_at_top = top(EXPECT_SOCKETS["LadderTop"][0], EXPECT_SOCKETS["LadderTop"][1])
    check("collision_z_under_LadderTop", None if z_at_top is None else round(z_at_top, 5),
          z_at_top is not None and abs(z_at_top - DECK_Z) < 0.01,
          "LadderTop must land on SOLID deck -- a socket over a hole is the castle-"
          "floor defect class: every readback correct, nothing can use it")

    # ---- 5. LadderFoot must be STANDABLE: no collision at or over it ------------
    fx, fy = EXPECT_SOCKETS["LadderFoot"][0], EXPECT_SOCKETS["LadderFoot"][1]
    foot_hits = []
    for k in range(24):
        a = 2.0 * math.pi * k / 24.0
        for r in (0.0, 17.0, 34.0):
            foot_hits.append(top(fx + r * math.cos(a), fy + r * math.sin(a),
                                 from_z=600.0))
    check("collision_over_LadderFoot_footprint", [h for h in foot_hits if h is not None],
          all(h is None for h in foot_hits),
          "a hull at the foot would carve ground nav in the exact cell a unit must "
          "stand in to start climbing")

    # ---- 6. the ground carve is the full 600x600 (LadderFoot's 86 uu depends on it)
    ground_half = [0.0, 0.0]
    for h in hulls:
        hzs = [v.co.z * UE for v in h.data.vertices]
        if min(hzs) > 1.0:
            continue
        ground_half[0] = max(ground_half[0], max(abs(v.co.x) * UE for v in h.data.vertices))
        ground_half[1] = max(ground_half[1], max(abs(v.co.y) * UE for v in h.data.vertices))
    check("ground_carve_half_extent_uu", [round(ground_half[0], 3), round(ground_half[1], 3)],
          abs(ground_half[0] - BODY_HALF) < 0.01 and abs(ground_half[1] - BODY_HALF) < 0.01,
          "ground nav starts at x <= -364 only if the carve really is 600x600")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as fh:
        json.dump(rep, fh, indent=2)

    bad = 0
    for key, got, ok, note in rows:
        print("%-4s %-38s %s" % ("PASS" if ok else "FAIL", key, got))
        if not ok:
            bad += 1
            print("       ^ %s" % note)
    print("ROUNDTRIP %d/%d rows pass -> %s" % (len(rows) - bad, len(rows), OUT))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
