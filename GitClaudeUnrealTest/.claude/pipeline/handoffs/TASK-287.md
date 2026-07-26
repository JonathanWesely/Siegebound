# TASK-287 handoff — M7.6 Phase 3 density fill (build-master, DA-only)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · **HEAD at start:** `2249444`
**Change:** `Content/Data/DA_BattlefieldScatter.uasset` ONLY (data). No code, no compile. `L_Arena` NOT touched (nav not rebuilt — see §Nav).
**Jonathan's ruling:** "default the numbers, tune at W2." These defaults are applied + committed; the table below is HIS W2 tuning lever.

## Density / cull / shadow TABLE (the W2 tuning levers)

Method: scale each layer's Phase-0 `instanceCount` ≈4.9× (keep proportions), total 3052 → **15000**. Cull bands camera-scaled for the 10× field (grass short → structural never). Shadows: obstacle layers ON, grass/plants OFF.

| Layer | Blocking | count (was → now) | CullStart (uu) | CullEnd (uu) | bCastShadows | meshVariants |
|---|---|---|---|---|---|---|
| Trees | yes | 70 → **340** | 24000 | **32000** (FAR) | ON | 12 |
| Rocks | yes | 60 → **300** | 14000 | **20000** (MID) | ON | 10 |
| Boulders | yes | 6 → **30** | 0 | **0** (NEVER) | ON | 2 |
| Hill | yes | 8 → **40** | 0 | **0** (NEVER) | ON | 3 |
| Slabs | yes | 8 → **40** | 0 | **0** (NEVER) | ON | 5 |
| Grass | no | 2500 → **12250** | 6000 | **9000** (SHORT) | OFF | 10 |
| Plants | no | 400 → **2000** | 8000 | **12000** (MID) | OFF | 5 |

- **Total instances:** 15000 (blocking obstacles 750 + non-blocking decoration 14250).
- **Rocks shadow = ON** (obstacle layer; Jonathan's OFF list was grass/plants only — rocks kept casting per the obstacle-silhouette default).
- Cull is RENDER-only — collision/nav for blocking layers is unaffected by CullEnd, so culled-far obstacles still block/route units.
- All OTHER FScatterLayer fields (soft meshes, bBlocking, minSpacing, scaleRange, footprintRadius, collisionProxyMesh, overrideMaterial=M_HillGrass on Hill, bAllowOnHills, etc.) were preserved byte-for-byte — server-side deepcopy edit via ProgrammaticToolset, readback-verified: mesh-ref integrity PASS, Hill `M_HillGrass` PASS, Trees `Cylinder` proxy PASS.

## Traversability (NON-NEGOTIABLE) — PASS at density

3 fresh PIE generates (Play-Again equivalent, `OverrideSeed=0` → 3 seeds):

| Seed | Placed (Hill/Grass shown; others hit target) | Traversability |
|---|---|---|
| 494877441 | Boulders 30, Hill 30/40, Slabs 40, Trees 340, Rocks 300, Grass 12246/12250, Plants 2000 | **CONFIRMED, 0 culls** |
| 197269377 | Hill 29/40, Grass 12245/12250 (rest on target) | match ran full (bot economy+attack) → validated |
| 1159629313 | Hill 28/40, Grass 12248/12250 (rest on target) | **CONFIRMED, 0 culls** |

- Hill places 28-30 of 40 target (minSpacing 2000 + scale-to-2.5× crowding on the field — normal rejection sampling, not an error); Grass 12245-12248 of 12250 (trivial). All others hit target exactly.
- 6 mines/generate; reserved corridor (half-width 1000) + keep-clear guarantee the lane; reachability validation found **nothing to cull** on the confirmed runs.
- Match runs healthily at density: bot plays the full rule ladder — Rule 2 Economy (Deep Mine, Miners) + Rule 4 Attack (Cleric, Knight), units marching.

## ⚠ FINDING for Jonathan's W2 look — large-world matrix-precision ensure (non-fatal)

An intermittent, NON-FATAL ensure fired on **2 of 3** dense PIE generates (not the 3rd, not in TASK-286's old-density run):
```
Ensure condition failed: OriginX <= OriginMax && OriginY <= OriginMax && OriginZ <= OriginMax [DoubleFloat.cpp:19]
Found precision loss while converting matrix to GPU format ... view transform invalid, or PreViewTranslation/ViewOrigin not set up correctly
```
(run 2 fired a related `Matrix.h:468` matrix-validity ensure.) It is a **Large-World-Coordinates rendering-precision transient** on PIE startup at the 10× arena's far coordinates — caught (not a crash), match + traversability fully intact each run. Intermittent (2/3) + view-transform wording ⇒ likely a PIE-startup view-precision hiccup, plausibly a touch more likely at higher instance density but NOT deterministic per-generate. **W2 gameplay-cam WATCH** (rendering/perf) — dial density back or an LWC project setting if it correlates with a visible artifact; does NOT block the density (data correct, gameplay healthy). Other PIE warnings were pre-existing/benign (miner material usage-flag; nav-settle 10s cap; font-cache flush).

## Nav

Editor-python nav-build is NOT reachable via MCP (ProgrammaticToolset only invokes registered tools; no `unreal.` nav-build API, no registered nav-build tool). Per direction I did NOT fake it and did NOT rebuild+resave nav → **`L_Arena.umap` NOT committed**. `L_Arena`'s `RecastNavMesh` is `RuntimeGeneration=Dynamic` (TASK-136), so the navmesh regenerates at PIE against the new blockers — non-breaking (all 3 PIE runs confirmed traversability). Flagged for Jonathan: one manual **Build > Navigation** click (then resave) if he wants the committed/editor-preview baked nav to match the new density.

## PERF

Not machine-capturable (no console-exec/stat-read via MCP; detached-editor tick throttled unfocused). The dense field is exactly where fps matters — **Jonathan W2 gameplay-cam WATCH**.

## Files
- `Content/Data/DA_BattlefieldScatter.uasset` (density fill; LFS)
- this handoff + TASKBOARD status
