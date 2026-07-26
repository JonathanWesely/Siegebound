# TASK-294 handoff — Vista-ring NO-OVERHANG reposition (build-master)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-25 · HEAD at start `c35f146` (TASK-293 fill) · NO compile.
Worked SYNCHRONOUSLY; ONE seeded compute + ONE atomic editor-python write that moved all targets and SAVED `L_Arena` to
disk IMMEDIATELY (git-verified umap modified BEFORE any screenshot/PIE). MCP client wedged → drove everything via the raw-HTTP
MCP fallback (`.claude/tmp/mcpc.py`, dual-channel SSE) + `ProgrammaticToolset.execute_tool_script`.

## Problem (Jonathan)
The vista meshes are NoCollision (correct) but scaled huge (6–15×), so their GEOMETRY overhangs inward past the walkable
boundary even though their ORIGINS sit outside it — units path through the overhanging parts. "Many vista objects are still
within the walkable area."

## Live measurements (read pass)
- Nav (`NavMeshBounds_Arena`) half-extents: **X±28000 / Y±12500**, center (0,0).
- 78 vista dressing actors found (16 `Vista_*` + 62 `VistaFill_*`) via label regex `^(Vista|VistaFill)_\d+$` — POIs / gold /
  scatter / walls excluded by the regex, never touched.
- **World AABB half-extents up to ~17,600 uu** (scale 6–15× on big rock/hill/tree meshes) → **52 of 78 overhang** the nav rect;
  worst (`VistaFill_23`) reached **8,351 uu INTO the walkable area**.
- Ground-plane extent probe (12 azimuths, trace-down): +X/−X & corners to ~44–48k, short ±Y sides to ~30–32k (ground extends
  well past nav everywhere).

## Reposition approach (deterministic, per-object radial push)
For each object: took its live world AABB (already accounts for yaw+scale), computed the outward radial unit vector from center
through the actor origin, and solved the **minimal outward translation `t`** so the translated AABB no longer overlaps the
nav rectangle expanded by **margin = 2500 uu** (separation on whichever axis clears first — the natural side). Bigger meshes
move farther out (their extent is larger). Kept z / yaw / scale unchanged → layered depth-band + yaw variety preserved.
- **Objects already clearing ≥2500 were LEFT IN PLACE** (26) — pushing them further out would *reduce* occlusion. Only the
  52 overhangers moved. All 78 now clear ≥ margin.
- **Occlusion preserved by construction:** because the meshes are so large, pushing each until its INNER edge sits at
  nav+2500 lands that inner face at ~30,500 (X) / ~15,000 (Y) — still far INSIDE the ground edge (44k / 30k), so every vista's
  near wall keeps hugging the field edge and backs the horizon. Pre-flight check: the tightest inner-face-vs-ground margin is
  **+2,846 uu** (`VistaFill_26`, inner face ~41,154 vs ground edge 44,000); NO object's inner face exceeds the ground extent →
  no void revealed. **No taller vistas and no ground-plane extension were needed.**

## VERIFY (all after SAVE)
- **Write result:** moved **52/52** (0 fails), matched 78, `save_result=True`. `git status` → `Content/Maps/L_Arena.umap`
  modified on disk (LFS pointer changed) BEFORE screenshots/PIE.
- **No-overhang proof (independent fresh re-scan + recompute):** **0/78 overhang**; **MIN clearance across all 78 = 2,499 uu**
  (target 2500; the 1 uu is coordinate rounding). Every vista inner bounds edge is ≥2,499 uu outside the nav boundary.
- **Vista flags preserved:** readback of all 78 → `flags_ok=78/78`, `flags_fixed=0` (translation preserves component flags):
  `CastShadow` / `bVisibleInRayTracing` / `bCanEverAffectNavigation` / `bAffectDistanceFieldLighting` /
  `bAffectDynamicIndirectLighting` all=false. Collision left at mesh default (no baked geometry; nav-off is the guard).
- **Occlusion proof (screenshots, gameplay-cam height, SENT — unstaged in handoffs/):**
  - `TASK-294-topdown.png` — ring sits clearly OUTSIDE the nav rectangle with a clean margin.
  - `TASK-294-low-posY.png` / `TASK-294-low-negY.png` — the tight short (±Y) sides: solid near-hill wall backs the horizon,
    ground meets it with NO void strip, no seeing-over-the-edge.
  - `TASK-294-low-posX.png` — near hill + grey rock peaks on the skyline, no drop-off void.
  - `TASK-294-low-corner.png` — +X+Y corner (tightest occlusion margin): rocks/trees/hills rise above the boundary wall, no gap.
  - `TASK-294-oblique-posY.png` — the +Y ring is a continuous hills+trees+rocks chain (no sky-gap).
- **LWC clean + traversability — ONE PIE generate:** `InverseFast` NaN = **0**, `OriginX<=OriginMax` ensure = **0**,
  `non-invertible` = **0** across the whole session; `Traversability CONFIRMED — Blue→Red castle path + 6 mine path(s)
  (after 0 cull(s))`; 7 scatter layers on target; StopPIE clean.

## Diff / hygiene
Branch-owned diff = **`Content/Maps/L_Arena.umap` only** (+ board + this handoff). NO code / DA / fleet / gameplay source
touched. `DeckBuilderWidget.{cpp,h}` + `WBP_DeckBuilder.uasset` (parked) left MODIFIED-but-UNSTAGED. Screenshots + `.claude/tmp/`
untracked. LFS handles the umap. NO push. Existing scatter / POIs / 2 mid-field gold props untouched.

## Follow-ups for the manager (report-only; non-fatal)
1. A few big-mesh pivots on the short ±Y sides now sit just past the visual ground edge (base pivot over void), but their INNER
   faces stay on-ground and occlude — no visible void from gameplay height. If a future top-down art pass wants pivots fully
   on-ground, the lever is extending the VISUAL ground plane outward on the ±Y sides (nav/walkable stays unchanged) — not needed
   for the no-overhang / occlusion requirement.
2. `InputMode:UIOnly` HUD-focus error (pre-existing, unrelated to dressing) still stands — HUD/deck-builder owner's item.
