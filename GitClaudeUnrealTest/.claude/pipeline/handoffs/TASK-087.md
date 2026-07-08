# TASK-087 Handoff — Pilot batch 2: SM_Archer + SM_Castle (TRELLIS.2 pipeline)

- **From:** art-director
- **Date:** 2026-07-08 (session of 2026-07-07 late; finish pass same night)
- **Status:** **COMPLETE — ready-for-integration** (orchestrator owns the board flip). Jonathan's two Reimport clicks landed 06:20 UTC; full post-click verification below, all readback-verified. Blockers ask 1783490849.018339 CLOSED.

## Stage 1 — GENERATE: SUCCESS, both first-try, zero re-rolls
- Ran bare (NO SSL_CERT_FILE — Norton exclusions held on the authenticated path, per TASK-086 must-know 1).
- **Archer:** seed 0, 86 s end-to-end on the PRO queue. `Cache/Archer/trellis_raw.glb` 20,635,768 bytes. 476,316 raw tris, 2× 2048² textures.
- **Castle:** seed 0, 123 s. `Cache/Castle/trellis_raw.glb` 21,312,124 bytes. 496,567 raw tris, 2× 2048² textures.
- Cache/Castle TASK-084 smoke residue cleaned BEFORE generation (rm -rf, same hygiene as TASK-086 did for Footman).
- Quota: ~3.5 GPU-min total this task; PRO headroom never approached (ruling 8 windows not needed).
- **Raw facing eyeball (both):** headless Workbench turntables → `Cache/<Asset>/raw_inspect/` (front/side/back/three_quarter/top + raw_info.json). BOTH front-face -Y ⇒ `pre_rotate_z_deg: 0` correct for both. Archer stance mirrored vs the blockout note (bow reads +X, quiver -X/back) — cosmetic, same as Footman's mirror. Castle front gate + rocky ramp at -Y.

## Stage 2 — REFINE: SUCCESS, ONE tune loop each (eyeball gate did its job)

Command per asset: `blender.exe --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/refine_trellis_glb.py -- --card-id <Asset>` (~25-35 s/run).

### Archer tune (run-1 gate FAIL → probe → run-2 PASS)
- Run-1 finding: manifest `cap` selector painted her entire bare HEAD (blue face — she has hair, not a helmet) and the `quiver` box splattered ragged patches across the surcoat back. 27.1% selected.
- Numeric probe of the exported FBX (face-area/normal/normalized-position stats): quiver+fletching at y_norm>0.85, x 0-0.45; hair back ALSO lives above z 0.85 ⇒ quiver box must cap at z 0.85 (fletching stays natural, no two-tone hair).
- **Tuned selectors** (manifest `_tuned` note): `pauldrons` (Footman shoulder-cap recipe: box z 0.70-0.86 + normal-up min_dot 0.55) + `quiver` (box x 0-0.55, y 0.84-1.0, z 0.40-0.85, no normal filter). Result: blue pauldron caps + blue quiver body, face/hair/heraldry natural, **6.3%** area.
- Final report: 15,000 tris exact; minZ 0.052; UVMap; slots `["TeamRegion","ArcherPBR"]`; D/N/ORM 1024².
- **Accepted WARN (TASK-088 WATCH):** conformed dims [90.7, 92.5, 180] vs blockout [125, 67.3, 180] — X/Y warn-only under fit_mode height (Y grew: bow held forward + quiver behind). Z exact. Same pattern as Footman's slimming.

### Castle tune (run-1 gate FAIL → probe → run-2 PASS)
- Run-1 finding: team region only **1.1%** — the box-fit Z-stretch (scale Z 17.61 vs Y 8.19 vs X 11.40) steepened every roof past the `min_dot 0.3` normal filter; only a few inner keep roofs selected.
- Probe: upward-face area lives in dot 0.10-0.30 after the stretch; perimeter tower cones sit at z_norm 0.30-0.55 (below the old 0.55 floor).
- **Tuned selector:** `roofs` box z 0.30-1.0, normal-up min_dot 0.10 → tower cones + wall-walk tops + keep roofs, **6.8%** area. Verticals stay natural (no wall contamination — measured).
- **UCX re-derived from the GENERATED mesh** (manifest `_comment` updated): wall planes probed via face-area histograms (front y_norm 0.17-0.28, back 0.79-0.87, west x 0.15-0.25, east 0.88-0.97), towers + keep measured. **11 hulls** (was 9): 4 walls + 4 corner towers + keep + TWO structures the blockout lacked — west annex chapel + forward gatehouse porch. Rock ramp/terrain shelf deliberately UNCOLLIDED (no slab, per law). **Front-most collision y = -328 vs blockout -410 ⇒ the M1 dead-zone SHRINKS; gold-node/miner clearance improves.**
- Final report: 40,000 tris exact (39,990 on independent re-import triangulation — noise, under budget); minZ 0.445; UVMap; slots `["TeamRegion","CastlePBR"]`; bounds EXACT 814.5×820×900 (box fit ⇒ ±10% rule satisfied by construction; CastleAnchor/HP-bar/L_Arena silhouette unchanged at the bounds level); D/N/ORM 2048².
- **Visual note (Jonathan sign-off item):** box fit stretches the flat concept proportions ~2.15× vertically relative to footprint — reads as dramatic gothic verticality, silhouette intact; judged acceptable at the gate. If it looks wrong in-arena, the escape is a manager ruling on `fit_mode` for buildings, not a re-roll.

### Independent FBX verification (both, headless re-import)
- Archer.fbx: visual object `SM_Archer`, materials IN ORDER `[TeamRegion, ArcherPBR]`, UVMap, 15,000 tris, 0 UCX (correct — unit hulls generated at Stage 3).
- Castle.fbx: visual object `SM_Castle`, `[TeamRegion, CastlePBR]`, UVMap, UCX_SM_Castle_00..10 (11 hulls).
- All six texture PNGs magic-checked at correct sizes (1024²/2048²).
- Concepts copied: `Content/RawAssets/Concepts/Archer.png`, `Castle.png`.

## Stage 3 — EDITOR (pre-click portion): COMPLETE
- Baselines (pre-swap blockouts): SM_Archer 1,558 tris, bounds X -74..51 / Y -39.2..28.1 / Z 0..180, one slot [MI_TeamColor_Blue], Nanite off. SM_Castle 2,414 tris, 814.5×820×900, one slot, Nanite off.
- Textures imported to /Game/Textures/: T_Archer_D (sRGB true, TC_Default), T_Archer_N (sRGB false, TC_Normalmap auto), T_Archer_ORM (**manual sRGB→false applied**, TC_Default); same trio for Castle. All readback-verified.
- MIs created at /Game/Materials/Instances/: MI_Archer_PBR + MI_Castle_PBR from /Game/Materials/M_AssetPBR; BaseColor/Normal/ORM texture params set and readback-verified. No scalar params (master has none).
- All 8 assets saved (`save_assets` true).
- Content Browser pre-navigated to /Game/Meshes; SM_Archer + SM_Castle selected (readback-confirmed).

## CLICK GATE record
- 🚨 Blockers batched ask ts **1783490849.018339** (thread 1783116296.221319): Don't-Import on any auto-reimport toast, then right-click Reimport SM_Archer and SM_Castle.
- Remaining after the clicks (next session/resume): (1) new-mesh readbacks — tris 15,000/40,000 & bounds vs refine reports; (2) slot names+order (reimport pulls [TeamRegion, <Asset>PBR] from FBX; fix order via set_material by NAME if shuffled); (3) assign slot TeamRegion → MI_TeamColor_Blue, ArcherPBR → MI_Archer_PBR, CastlePBR → MI_Castle_PBR; (4) Nanite OFF check; (5) Archer `generate_convex_collisions(hull_count=4, max_hull_verts=16)`; (6) Castle: verify UCX came through the reimport and NO auto-generated collision replaced it; (7) referencers intact (BP_Unit_Archer / Castle consumers); (8) zero import/MikkTSpace warnings in the log; (9) save all; (10) finalize this handoff + board flip by orchestrator.

## Post-click verification (2026-07-08 finish pass — all readback-verified)

Reimports landed at 06:20:17 and 06:20:27 (log time). Each FBX imported **twice**: both meshes were pre-selected in the Content Browser, so each right-click Reimport acted on the whole 2-asset selection. Harmless and idempotent — final state verified below. No blanket import ran: `/Game/RawAssets` registers **zero** assets.

### SM_Archer (/Game/Meshes/SM_Archer) — ALL PASS
- Tris **15,000** exact. Bounds X −45.13..45.28 / Y −46.28..45.87 / Z 0.052..179.94 = **90.41 × 92.15 × 179.89** — matches refine_report `bounds_ue` + `min_z` exactly.
- Slots `["TeamRegion","ArcherPBR"]` — names AND order came through the FBX correctly, no fix needed.
- Materials assigned + readback-verified post-save: TeamRegion → `/Game/Materials/Instances/MI_TeamColor_Blue`, ArcherPBR → `/Game/Materials/Instances/MI_Archer_PBR`. **Path correction:** MI_TeamColor_Blue lives under `/Game/Materials/Instances/` (spec draft said `/Game/Materials/`); resolved from SM_Footman's TeamRegion slot as the authority.
- Nanite **false** — unchanged by the reimport, no fix needed.
- Collision: `generate_convex_collisions(hull_count=4, max_hull_verts=16)` → true (units ≤4 hulls law). No hull-count readback tool exists — **TASK-088 structural check should confirm visually/PIE** (same caveat as Footman).
- Referencer intact: `/Game/Blueprints/Units/BP_Unit_Archer`.
- Log: **zero** warnings for both Archer imports (no MikkTSpace/degenerate/tangent/smoothing lines).

### SM_Castle (/Game/Meshes/SM_Castle) — PASS with two recorded notes
- Tris **40,000** exact. Bounds X −407.01..406.12 / Y −409.53..409.60 / Z 0.445..897.65 = **813.13 × 819.13 × 897.21** — matches refine_report exactly (raw-mesh bounds inside the 814.5×820×900 conform box).
- Slots `["TeamRegion","CastlePBR"]` correct; TeamRegion → MI_TeamColor_Blue, CastlePBR → `/Game/Materials/Instances/MI_Castle_PBR`; readback-verified post-save.
- Nanite **false** — unchanged.
- **UCX SURVIVAL: PASS.** `BodySetup.AggGeom` = exactly **11 convexElems**, all `bIsGenerated: false`, all 8-vert axis-aligned boxes; sphere/box/sphyl elem arrays **empty** — the FBX UCX set IS the simple collision, nothing auto-generated replaced or augmented it. Layout maps 1:1 to spec: 4 wall slabs (z 0–280), 4 corner towers (z 0–430), keep (x −261..229, y −294.5..180.5, z 0–750), west annex chapel (x −377..−287, z 0–460), gatehouse porch (x −76..84, y 179..329, z 0–420) protruding 59 uu past the gate wall (y 180..270). **No base slab; wall footprint intact.** Independent confirmation: Interchange MeshPayload `[12 40132 19906]` = 1 visual + 11 UCX objects, 40,000 + 11×12=132 box tris.
- **Note 1 — coordinate sign (not a defect):** in-editor the castle front (gate wall + porch + front towers) sits at **+Y**, front-most collision **y = +329**. The interim's "−328" was the Blender/FBX-space value; the standard FBX Y-flip on import mirrors it (|328|≈|329|, sub-uu rounding). The visual mesh flipped identically, so collision and visual stay aligned, and the dead-zone shrink vs the blockout's |410| holds exactly as promised. Bounds are Y-symmetric, so L_Arena placement is unaffected; the placed actor's rotation owns world-space facing.
- **Note 2 — accepted WARN (TASK-088 WATCH):** each Castle build logged `LogStaticMesh: Warning: SM_Castle has some nearly zero normals/tangents/bi-normals (Tolerance of 1E-4)` (3 lines × 2 builds = 6 total; the ONLY warnings in the reimport window). Known artifact of voxel-remeshed geometry — cosmetic shading risk at isolated verts only; mesh built clean and the gate previews were accepted. The zero-warnings criterion is therefore **not strictly met for Castle**; recorded, not suppressed. Archer met it fully.
- Referencer intact: `/Game/Maps/L_Arena` (castle is placed directly in the level).

### Cross-cutting
- **Stray assets: none.** Project-wide "Archer" = the expected 7 (6 pipeline assets + pre-existing `/Game/UI/CardArt/T_CardArt_Archer`, untouched lane); "Castle" = the expected 6 (5 pipeline + pre-existing `/Game/UI/WBP_CastleHealthBar`). `/Game/RawAssets` = 0 assets.
- **Saved:** SM_Archer + SM_Castle via `save_assets` → true; `is_dirty` false on both afterward. Textures/MIs were already saved pre-click; materials themselves were not modified by slot assignment.
- **Deliberately skipped:** post-import beauty thumbnail (base64-through-MCP is context-prohibitive; gate previews in Cache/ were already eyeballed and accepted). In-arena visual + Archer hull eyeball land in TASK-088's structural/PIE check.

## Notes for the M7 16-mesh batch
1. Selector tunes are the NORM, not the exception: 3/3 pilot assets needed one loop. The numeric FBX probe (face area × normal × normalized position, scratchpad probe_fbx.py pattern) converges in one loop — budget ~10 min/asset for gate+tune, not render-and-pray.
2. Buildings: box-fit Z-stretch distorts steep roofs ⇒ normal-filter min_dot must be tuned per generated mesh (0.10 worked here); z floors derived from measured, not blockout, heights.
3. Buildings: UCX must be re-derived from the generated mesh — TRELLIS adds/moves structures (annex, gatehouse) and terrain shelves that must NOT get hulls. Wall-plane histograms in a rock-free z-slab work well.
4. Units: concept characters WITHOUT helmets will get blue-face selections from any full-width top-z selector — prefer normal-up pauldron caps + equipment boxes.
5. Stage-1 generations can run back-to-back in one background command; poll the log/state.json (wake notifications DID fire this session, but the poll-loop belt-and-suspenders per must-know 2 cost nothing).
6. 1408×768 non-square concepts accepted again with a benign WARN.
7. Human clicks remain 1 per mesh (MCP reimport gap still open — manager tooling request stands before M7's 16 clicks).
8. **Finish-pass addition:** right-click Reimport acts on the WHOLE Content-Browser selection — with 2 assets selected, 2 clicks fired 4 imports (n² pattern). Idempotent here, but for the 16-mesh batch either pre-select ONE asset per click or accept redundant rebuild time.
9. **Finish-pass addition:** voxel-remeshed buildings will log `nearly zero normals/tangents/bi-normals (1E-4)` LogStaticMesh warnings at build — triage against the gate previews and record as accepted WARN; don't treat as an import failure (units were clean; Footman + Archer both zero-warning).
10. **Finish-pass addition:** MI_TeamColor_Blue's real path is `/Game/Materials/Instances/MI_TeamColor_Blue` — resolve placeholder paths from a known-good mesh slot, not from memory.
11. **Finish-pass addition:** FBX import Y-flips Blender space — a UCX front edge recorded at Blender −y lands at editor +y. Record front-edge expectations as |y| magnitude to keep post-click checks sign-proof.

## Commit manifest for TASK-088 (no Git in my lane)
Content/RawAssets/Archer.fbx + Castle.fbx (overwritten), Content/RawAssets/Textures/Archer/T_Archer_{D,N,ORM}.png + Textures/Castle/T_Castle_{D,N,ORM}.png (new), Content/RawAssets/Concepts/Archer.png + Castle.png (new), Content/Textures/T_Archer_*.uasset + T_Castle_*.uasset (new, 6), Content/Materials/Instances/MI_Archer_PBR.uasset + MI_Castle_PBR.uasset (new), Content/Meshes/SM_Archer.uasset + SM_Castle.uasset (after reimport+save), Tools/ArtPipeline/pipeline_manifest.json (Archer selector tune + Castle selector/UCX tune, `_tuned` notes). Lane isolation honored — no CardArt paths touched; no level/BP edits; no C++.

## Slack ledger
🎨 Art: 1783489586.107219 (start), 1783490644.982339 (Stages 1+2 complete), 1783490856.614129 (click gate), 1783492207.129619 (final ✅ completion). 🚨 Blockers: 1783490849.018339 (two-click ask) — CLOSED by 1783492217.060459.
