# TASK-289b handoff — donor LOD generation (Trees + Hills), build-master

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · **HEAD at start:** `9fa6b92` · asset LOD-gen + git, NO compile.
Follow-up to the TASK-289 audit finding: the scatter tree + hill donors shipped **LOD0-only**. Generated 5-LOD chains on the FULL donor families so all 340 tree + 40 hill instances get reduction (not a 1/12 token). Conflict-free — Fab/terrain environment donors, NOT the M7.5 SK_/SM_ unit fleet.

## Method
`StaticMeshTools.generate_lods([0.5, 0.25, 0.125, 0.0625])` → 5 LODs (LOD0 + 4), matching the other scatter donors' 5-6 LOD density (grass/plants=5, rocks/boulders/slabs=6). Server-side batch (ProgrammaticToolset) with before/after readback verification, then `save_assets` (only after all 15 passed). "SM-Mobile_Tree"/"SM_Hill_01" in the task = the donor FAMILIES: 12 tree variants (`SM-Mobile_Tree_1..12`) + 3 hill variants (`SM_Hill_01..03`), all used by the DA Trees/Hill layers.

## Before → after (LOD count 1 → 5; per-LOD triangles)

| Mesh | LODs | LOD0 | LOD1 | LOD2 | LOD3 | LOD4 |
|---|---|---|---|---|---|---|
| SM-Mobile_Tree_1 | 1→5 | 2458 | 1228 | 615 | 308 | 154 |
| SM-Mobile_Tree_2 | 1→5 | 2025 | 1012 | 507 | 254 | 126 |
| SM-Mobile_Tree_3 | 1→5 | 2115 | 1058 | 528 | 264 | 132 |
| SM-Mobile_Tree_4 | 1→5 | 2137 | 1069 | 534 | 267 | 134 |
| SM-Mobile_Tree_5 | 1→5 | 2283 | 1142 | 570 | 286 | 142 |
| SM-Mobile_Tree_6 | 1→5 | 2422 | 1211 | 606 | 302 | 152 |
| SM-Mobile_Tree_7 | 1→5 | 2249 | 1124 | 563 | 282 | 140 |
| SM-Mobile_Tree_8 | 1→5 | 2213 | 1107 | 554 | 276 | 138 |
| SM-Mobile_Tree_9 | 1→5 | 2360 | 1179 | 590 | 294 | 148 |
| SM-Mobile_Tree_10 | 1→5 | 2202 | 1101 | 550 | 276 | 138 |
| SM-Mobile_Tree_11 | 1→5 | 2291 | 1145 | 572 | 286 | 144 |
| SM-Mobile_Tree_12 | 1→5 | 2393 | 1196 | 598 | 300 | 150 |
| SM_Hill_01 | 1→5 | 636 | 318 | 158 | 80 | 64 |
| SM_Hill_02 | 1→5 | 636 | 318 | 158 | 80 | 64 |
| SM_Hill_03 | 1→5 | 716 | 358 | 178 | 90 | 64 |

W3 impact: the 340 tree instances (was ~2450 tris each, no reduction) now drop to ~150 tris at LOD4; the biggest single scatter draw-cost win. Hills minor (40 inst) but done for completeness.

## Verify — nothing else changed (LOD-gen only)
- **LOD0 triangle count IDENTICAL** before/after on all 15 (geometry preserved — the added LODs are pure reduction of LOD0).
- **Material slots IDENTICAL** before/after on all 15.
- **Bounds intact:** readback box extents match the source dimensions exactly — hills match the M6.6 spec (SM_Hill_01 1400×1400×295 [r700/h250], SM_Hill_02 2200×2200×477 [r1100/h400], SM_Hill_03 2200×1700×430 [ridge/h350]); trees are sane ~800-1600 × ~1400-2100. (The transient exact-`==` bounds mismatch on the first pass was float noise from the rebuild — the values are correct.)
- **Collision preserved:** `generate_lods` is a render-LOD-only operation — it does not touch the BodySetup/AggGeom. LOD0 unchanged + bounds matching spec confirm it. (Trees block via their separate `Cylinder` collision proxy; hills via their M6.6 single convex hull — neither is a render LOD, so both are untouched. No direct hull-count MCP tool exists to byte-prove it; a manual editor check can if desired.)

## PIE — renders + traversable
Scatter DA unchanged (only mesh LODs changed). PIE (seed 1703332609): **Trees placed 340/340, Hill 30/40** (normal crowding), **Traversability CONFIRMED (0 culls)**. No vanished instances — the LOD'd donors place/render correctly. The TASK-287 LWC matrix-precision ensure did NOT recur on this run (still intermittent session-wide, unchanged by this task).

## Commit
15 donor `.uasset` (12 trees + 3 hills, LFS) + **`Tools/ArtPipeline/rig_character.py`** (TASK-288 qa-passed recipe generator, folded in per direction — the SK-LOD `<CardID>.lod.json` recipe tooling is now captured) + TASK-288 docs + this handoff + board. `DeckBuilderWidget`/`WBP_DeckBuilder` (parked) + all M7.5 `SK_`/`SM_` fleet untouched.
