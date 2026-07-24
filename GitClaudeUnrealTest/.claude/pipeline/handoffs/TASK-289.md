# TASK-289 handoff — M7.6 Phase 4 LOD apply + W3 (build-master)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · **HEAD at start:** `8fe4991` · NO C++ compile.
**Outcome:** conflict-free parts done + **two blockers flagged for Jonathan** (SK-LOD apply not MCP-reachable AND is M7.5-fleet merge territory). The high-value W3 win surfaced by the audit is a **missing Tree donor LOD chain** — see §Donor audit.

## 1. LargeProp LOD tooling line — PRESENT (no port needed)
`Tools/reimport_meshes.py` already carries the TASK-220 line on the branch: `DEFAULT_LOD_GROUP = "LargeProp"` (line 114), the `// LODs (M7.6 classic-LOD law, TASK-220)` doc, and `_apply_lods` applies `lod_group='LargeProp'` (cpp 384/390/391) with `percent_triangles` (373). SM scatter/prop reimports auto-generate the LOD chain. Conflict-free (no change committed).

## 2. SK-unit LOD apply — DEFERRED, two blockers (FLAGGED to Jonathan)
- **BLOCKER A (tooling reachability):** the SK-LOD `regenerate_lod` is editor-python only. `SkeletalMeshTools` exposes NO LOD-generation tool (only `get_lod_count`), and `ProgrammaticToolset` runs only registered tools (no arbitrary `unreal.` API). So **SK-LOD regen is NOT reachable via MCP** — same class as the TASK-287 nav-build blocker. It needs a headless `-run=pythonscript` commandlet (the `reimport_meshes.py` route) or Jonathan's manual editor action.
- **BLOCKER B (M7.5 cross-batch):** the 11 SK fleet meshes ARE M7.5's main-lane retexture territory (TASK-201/202, gated on Jonathan's TASK-200 A/B — NOT started). Regenerating+committing LODs on those exact `.uasset` binaries on the branch would create binary merge conflicts with M7.5's pending retexture at Phase-6. Per the cross-batch ruling ("when unsure, prefer conflict-free + flag over a risky stomp"), I did NOT touch them.
- **URO note:** the OnlyTickPoseWhenRendered + UpdateRateOptimizations flags are ALREADY applied in C++ (TASK-285, on `SkeletalVisualMesh`, committed) — the SK-LOD step's URO part is already live; only the per-asset LOD chain is outstanding.
- **Recommended sequencing (for Jonathan's call):** apply SK-LODs on MAIN, AFTER M7.5's retexture reimport, via the same `regenerate_lod` + the TASK-288 `<CardID>.lod.json` recipes (LOD1 50%@0.4 / LOD2 20%@0.15; `percent_triangles`→`NumOfTrianglesPercentage`), so retexture + LOD land together with zero cross-lane conflict.

**SK fleet LOD baseline (readback, before):** all 11 = **LOD0-only** (`lods=1`), 14k-22k verts —
SK_Footman 14142, SK_MilitiaMob 16978, SK_Cleric 17008, SK_Archer 17055, SK_Pikeman 17789, SK_Cavalry 17496, SK_Longbowman 17557, SK_Knight 18456, SK_Miner 19382, SK_Sapper 19468, SK_Ogre 21899.

## 3. Cross-batch decision (M7.5 ↔ M7.6)
DID (conflict-free): verified LargeProp line (present), donor audit (read-only), SK baseline readback, PIE re-confirm. FLAGGED (not touched): the whole SK fleet LOD apply (blockers A+B). This is the coordinator-sanctioned "conflict-free parts + flag" path. **Nothing M7.5-owned was modified or committed.**

## 4. Donor LOD audit (read-only) — the W3 lever

| Layer | donor sampled | LODs | tris LOD0 | instances (TASK-287) | verdict |
|---|---|---|---|---|---|
| Trees | SM-Mobile_Tree_1 / _6 | **1 (MISSING)** | ~2450 | **340** | ⚠ #1 W3 lever — 340 blocking instances, no reduction |
| Hill | SM_Hill_01 | **1 (MISSING)** | 636 | 40 | minor (low count/tris) |
| Rocks | SM_Rock_1 | 6 ✓ | 3554 | 300 | has chain |
| Boulders | SM_Rock_11 | 6 ✓ | 18488 | 30 | has chain |
| Slabs | SM_Rock_31 | 6 ✓ | 11154 | 40 | has chain |
| Grass | SM_Grass_1 | 5 ✓ | 236 | 12250 | has chain |
| Plants | SM_Plant_6 | 5 ✓ | 448 | 2000 | has chain |

- **Rocks/Boulders/Slabs/Grass/Plants** donors already ship 5-6 LODs (Fab) — the dominant instance counts (grass 12250, plants 2000) are covered.
- **⚠ Trees (Tree_Pack_1 "Mobile" variants) ship NO LODs** — 340 blocking instances × ~2450 tris = ~833k tris at full detail always. **This is the single biggest W3 draw-cost win available.** Trees donors are Fab environment (NOT the M7.5 unit fleet), so generating their LODs is conflict-free — but donor generation was OUT of this task's read-only-audit scope. **RECOMMEND a follow-up task:** generate Tree (+Hill) donor LODs via `StaticMeshTools.generate_lods([0.5,0.25,0.125])` + `set_lod_thresholds` (or reimport through the LargeProp path), MCP-reachable, conflict-free.

## 5. PIE — traversability holds, LWC ensure unchanged
No asset changed this task, so the scatter is byte-identical to TASK-287's committed density (`8fe4991`) whose traversability PASSED ×3. Re-confirmed this session: **Traversability CONFIRMED** (seed 306390145, 0 culls). The TASK-287 large-world matrix-precision ensure (`DoubleFloat.cpp:19` / `Matrix.h:468`) continues to appear **intermittently** on session PIE generates (fired ~20.13; not on my 20.41 run) — unchanged by this task, still the standing W2 rendering WATCH.

## 6. W3 objective LOD evidence
Machine fps/`stat unit` remains not-capturable (no console-exec/stat-read via MCP; detached-editor throttle) → W3 fps is Jonathan's gameplay-cam. Objective LOD evidence delivered = the §4 audit table (per-layer donor LOD counts + LOD0 tris) + the §2 SK-fleet LOD0-only baseline. **Before/after vs TASK-287 density baseline:** no LODs were APPLIED this task (SK deferred/unreachable; donors read-only), so the mesh LOD state is the "before" recorded above; the actionable W3 delta is the Trees-donor-LOD follow-up (§4) + the M7.5-coordinated SK-LOD apply (§2).

## Commit / files
Docs only — no asset or tooling change (LargeProp already present; SK deferred; donors read-only). Committed: this handoff + TASKBOARD status. `DeckBuilderWidget`/`WBP_DeckBuilder` (parked) + all M7.5 fleet SK_/SM_ assets untouched.
