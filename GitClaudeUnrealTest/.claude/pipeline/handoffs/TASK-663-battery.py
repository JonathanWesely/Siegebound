# TASK-663 — THE POST-ROTATION WALK BATTERY (reusable; TASK-667 re-runs byte-for-byte).
# ProgrammaticToolset script (Unreal MCP execute_tool_script). Read-only: native
# physics overlaps via SceneTools.find_actors bounds + ObjectTypeQuery1-10 only.
#
# Frame law (ROT-S1): castle-local (lx, ly) -> world under yaw theta:
#   wx = Cx + lx*cos(t) - ly*sin(t);  wy = Cy + lx*sin(t) + ly*cos(t)
#   Blue  Castle_0 (-25000, 0) yaw +90:  wx = -25000 - ly, wy = +lx
#   Red   Castle_1 (+25000, 0) yaw -90:  wx = +25000 + ly, wy = -lx
# A castle-local axis-aligned probe box swaps its x/y half-sizes in world under +/-90.
#
# Stations/supports are the TASK-656 instrument (local y, support z), unchanged:
# the geometry is castle-local and must reproduce byte-true in the new world frame.
import json

CHANNELS = ["ObjectTypeQuery%d" % i for i in range(1, 11)]

CASTLES = {
    "blue": {"cx": -25000.0, "cy": 0.0, "sgn": +1},  # yaw +90
    "red":  {"cx": +25000.0, "cy": 0.0, "sgn": -1},  # yaw -90
}

# 656 centre-lane stations: (castle-local y, support z). Local x = 0.
LANE = [(-3700, 0), (-3600, 29), (-3500, 43.5), (-3300, 58), (-3000, 72.5),
        (-2700, 87), (-2500, 101.5), (-2200, 116), (-1950, 130.5),
        (-1700, 148), (-1550, 161), (-1300, 174), (-1000, 174),
        (-500, 174), (0, 174), (700, 174), (1300, 174)]

# NEW-approach pre-mouth stations (spawn lane -> mouth), grass support 0.
# Local y -3992 is the measured branch-3 hero spawn lane (colliding half-extent
# 3692.18 + clearance 300); -3850/-3775 fill the gap to the -3700 mouth line.
PRE = [(-3992, 0), (-3850, 0), (-3775, 0)]

# Riser-top slab probes (local y, z slab starts) bracketing 29 / 101.5 / 148 / 174.
TOPS = [(-3600, [24.0, 27.0, 30.0, 33.0]),
        (-2500, [96.0, 99.5, 102.5, 106.0]),
        (-1750, [143.0, 146.5, 149.5, 153.0]),
        (-1300, [169.0, 172.5, 175.5, 179.0])]

# Old d1 seal-face bisection (skirt_toe_01, manifest face local x 3657.5) — proves
# the seal rode the rotation onto the world-Y flank (the ToeRock premise datum).
SEAL_X = [3665.0, 3660.0, 3657.0, 3650.0]

# Forward-of-spawn open-field stations (the direction the hero FACES), local y
# past the spawn away from the castle: y -4300 / -4800 / -5500, grass support 0.
FWD = [(-4300, 0), (-4800, 0), (-5500, 0)]


def overlap_world(x0, x1, y0, y1, z0, z1):
    b = {"min": {"x": min(x0, x1), "y": min(y0, y1), "z": z0},
         "max": {"x": max(x0, x1), "y": max(y0, y1), "z": z1}, "isValid": True}
    r = execute_tool(
        "editor_toolset.toolsets.scene.SceneTools.find_actors",
        json.dumps({"root": None, "name": "", "actor_type": None, "tag": "",
                    "bounds": b, "collision_channels": CHANNELS}))
    return [a["refPath"].split(".")[-1] for a in (r.get("returnValue") or [])]


def local_box(c, lx0, lx1, ly0, ly1, z0, z1):
    """Castle-local AABB -> world AABB under the castle's +/-90 yaw, then overlap."""
    if c["sgn"] > 0:   # Blue, yaw +90: wx = cx - ly, wy = cy + lx
        xs = (c["cx"] - ly0, c["cx"] - ly1)
        ys = (c["cy"] + lx0, c["cy"] + lx1)
    else:              # Red, yaw -90: wx = cx + ly, wy = cy - lx
        xs = (c["cx"] + ly0, c["cx"] + ly1)
        ys = (c["cy"] - lx0, c["cy"] - lx1)
    return overlap_world(xs[0], xs[1], ys[0], ys[1], z0, z1)


def run():
    out = {}
    for name, c in CASTLES.items():
        rows = {}
        # 1) pre-mouth approach stations (hero capsule box 84x84x184, feet sup+8)
        for ly, sup in PRE:
            rows["pre_y%d" % ly] = local_box(c, -42, 42, ly - 42, ly + 42,
                                             sup + 8, sup + 192)
        # 2) the 656 centre-lane battery, rotated frame
        for ly, sup in LANE:
            rows["lane_y%d" % ly] = local_box(c, -42, 42, ly - 42, ly + 42,
                                              sup + 8, sup + 192)
        # 3) riser-top slab bisections
        for ly, zlist in TOPS:
            t = {}
            for z in zlist:
                t["z%.1f" % z] = local_box(c, -6, 6, ly - 6, ly + 6, z, z + 3.0)
            rows["top_y%d" % ly] = t
        # 4) old d1 seal-face bisection at local (x, 0) — 6-uu slabs z 200..300
        s = {}
        for lx in SEAL_X:
            s["x%.0f" % lx] = local_box(c, lx - 3, lx + 3, -30, 30, 200, 300)
        rows["seal_face"] = s
        # 5) forward-of-spawn field stations (what the spawn FACING looks into)
        for ly, sup in FWD:
            rows["fwd_y%d" % ly] = local_box(c, -42, 42, ly - 42, ly + 42,
                                             sup + 8, sup + 192)
        out[name] = rows
    return out
