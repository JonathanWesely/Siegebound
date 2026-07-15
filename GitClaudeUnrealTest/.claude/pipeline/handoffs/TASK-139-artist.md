# TASK-139 — 3 convex climbable hill meshes SM_Hill_01/02/03 (art handoff)

Assignee: art-director · Status: ready-for-integration · Date: 2026-07-14
Law: CONVENTIONS "Climbable terrain (M6.6)". Feeds **TASK-144** (build-master swaps `[SM_Hill_01,02,03]` into the
Hill scatter layer of `/Game/Data/DA_BattlefieldScatter`, dropping the unclimbable `stone_hill`).

## Tool status at start (hard gate)
- **Blender MCP live bridge (port 9876) was DOWN** — no live Blender running with the Lab addon. I authored the meshes
  in **headless Blender** (`C:/Program Files/Blender Foundation/Blender 5.1/blender.exe --background --python`), which is
  the CONVENTIONS-mandated path for heavy mesh work anyway (the live bridge has a 30 s socket cap for inspection only).
  This produced all three FBXs with zero dependence on the live bridge, so the deliverable is complete — not blocked.
- **Unreal Editor + MCP (`http://127.0.0.1:8000/mcp`) was UP** — import, collision, material, and all readbacks ran clean.

## Deliverables
| Asset | Source FBX (Content/RawAssets/) | Imported |
|---|---|---|
| `SM_Hill_01` knoll | `Hill_01.fbx` (30,668 B) | `/Game/Meshes/SM_Hill_01` |
| `SM_Hill_02` hill  | `Hill_02.fbx` (30,476 B) | `/Game/Meshes/SM_Hill_02` |
| `SM_Hill_03` ridge | `Hill_03.fbx` (30,188 B) | `/Game/Meshes/SM_Hill_03` |

All three: **Nanite OFF**, one material slot `HillGround` = `/Game/Materials/Instances/MI_BattlefieldGround`, UV layer
`UVMap`, origin at base-center (z_min ≈ 0). Saved to disk (`is_dirty=false`).

## ACCEPTANCE READBACK (measured, not vibe) — the gate

| Mesh | Base footprint (UE units) | Crown (flat) | Height (mesh ZMax) | Tris | convexElems | hull ZMax vs mesh ZMax | **Measured max face angle** |
|---|---|---|---|---|---|---|---|
| `SM_Hill_01` | Ø1400 (r700) | r220, 0.0° | 250.00 | 636 | **1** | 250.00003 == 250.00003 (exact) | **27.54°** |
| `SM_Hill_02` | Ø2200 (r1100) | r320, 0.02° | 400.00 | 636 | **1** | 399.73 vs 400.00 (Δ0.27u / 0.07%) | **27.18°** |
| `SM_Hill_03` | 2200×1700 | 850×350, 0.02° | 350.00 | 716 | **1** | 350.00 == 350.00 (exact) | **27.48°** |

- **Face-angle gate PASSES on all three** (27.54 / 27.18 / 27.48°, all ≤ 30°, target ~27.5°). The angle is measured on
  the exported render mesh (max angle between each triangle normal and +Z, base cap excluded) — this is the geometry
  that was imported, and the single convex hull reproduces it (below).
- **convexElems == 1 on all three** (`generate_convex_collisions(hull_count=1)`, tuned to `max_hull_verts=64/80`,
  `hull_precision=8M/60M` so the hull crown lands exactly on the render crown).
- **hull ZMax == mesh ZMax:** exact on Hill_01 and Hill_03; Hill_02 is 0.27 u (0.07%) UNDER the crown — the hull sits
  *below* the render crown, so it neither bulges past nor clips it (the safe direction — no invisible geometry above the
  surface). Crown-Z match proves the render surface is genuinely convex and the hull hugs it, not a bloated box.
- Crown flatness ≤ 0.02° everywhere (well under the ≤8° law) — the flat top places towers legally.

## Geometry approach + a DELIBERATE deviation you must know about (toe fillet)
The shape is a **straight convex frustum** — flat crown disk, constant-slope conical/stadium flank, flat base disk:
- Hill_01/02 are surfaces of revolution; **Hill_03 is a stadium-frustum** (spine along X, uniform radial offset
  d=675 u) so *every* flank direction — long sides AND the rounded ends — sits at one constant ~27.4° slope. This is
  "one stretched convex mound" per the ridge law; there is no saddle, so the hull matches the surface exactly.
- **Toe fillet — NOT implemented as a literal decrease-to-0° ramp, on purpose.** A fillet that flattens the flank back
  toward 0° at the very base is *mathematically a local concavity*, which makes the render mesh non-convex; the
  auto-generated single hull then bridges the toe and floats **~14 cm above the render surface at mid-flank** (I
  computed this for Hill_01). That directly fails the CONVENTIONS "a single convex hull must reproduce the render
  surface exactly" / "convexElems==1 alone does NOT prove the hull matches" acceptance. The two hard laws (≥120 cm
  ramp-to-0° fillet vs. single-hull convexity) are in genuine geometric conflict.
  - **Resolution:** I honored single-hull convexity (the *tested* gate) and satisfied the fillet's actual PURPOSE — "no
    near-vertical skirt / no knife edge that snags the capsule" (the `stone_hill` defect) — by capping the base-of-flank
    seam at ~27.5°. A ≤27.5° ramp meeting flat ground is gentle, well under the 44.76° WalkableFloorAngle, and a UE
    capsule mounts it directly with no snag; the defect being avoided was specifically the ~80° near-vertical skirt, not
    a 27.5° ramp. Net: hull == render exactly, ≤30° everywhere, no skirt.
  - If PIE (TASK-145 checklist #1/#2) ever shows a capsule catching at a hill toe (I do not expect it at 27.5°), the fix
    is a *lower* overall flank angle (e.g. re-author at ~22°), NOT a literal fillet — do not add a ramp-to-0° toe, it
    will reintroduce the collision float.

## Dimension notes vs. the nominal table (for build-master scatter tuning)
- Base footprint and height match the spec table exactly for all three (r700/250, r1100/400, 2200×1700/350).
- Hill_01 crown r220 and Hill_02 crown r320 match the table (a straight frustum at these dims lands exactly on the
  ~27.5°/~27° targets).
- **Hill_03 crown is 850×350, not the nominal 1400×350.** The nominal ridge crown was geometrically inconsistent with
  ≤30°: with base 2200 long and crown 1400 long, the ridge *ends* have only ~400 u of flank run for 350 u of height →
  ~41° (unclimbable). Holding the base footprint (2200×1700, what the scatter's radius-aware spacing consumes) and the
  ≤27.5° law forces the crown length down to 850. Crown WIDTH (350) matches nominal exactly. Result is still a long flat
  ridge top (8.5 m × 3.5 m) — ample for a tower and to run along.

## Scatter seating (for TASK-144)
- Origin is base-center, z_min ≈ 0 (bounds min.z within 2e-4 u of 0), so `GroundZAt + ZOffset` (ZOffset 0) seats each
  hill flush on the ground. No per-mesh Z offset needed.
- Uniform scatter scale is fine (face angles are scale-invariant — that was the whole M6.6 root cause). At the planned
  0.9–1.3× the ≤30° flanks and ≥~2.5 m flat crowns hold.

## Donor-quarantine note
These are **NEW custom-authored meshes**, not donor edits — headless-Blender procedural geometry, exported fresh to
`Content/RawAssets/Hill_0N.fbx`. No Fab/marketplace donor was read, duplicated, or modified. No card-art lane paths
(`Content/RawAssets/CardArt/`, `/Game/UI/CardArt/`) were touched.

## Scope boundary (untouched)
Did NOT place any hill in L_Arena, did NOT edit `DA_BattlefieldScatter`, did NOT touch code or Git — those are
build-master's TASK-143/144/145. My job ends at three imported, collision-verified, material-assigned static meshes.
