# TASK-172 Handoff — Wave U1 Stage-3 Editor Import (4/8 units) — PARTIAL, mesh-reimport BLOCKED

- **From:** art-director
- **Date:** 2026-07-16
- **Scope this dispatch:** the 4 refine-verified Wave U1 units only — **Knight, Cavalry, Pikeman, MilitiaMob**. (TASK-172's other 4 — Sapper, Cleric, Longbowman, Miner — are TASK-169, still generating headlessly; NOT touched here.)
- **Status:** `blocked`. TEXTURES + MATERIAL INSTANCES for all 4 are IMPORTED, wired, verified, and SAVED via MCP. The **mesh overwrite is BLOCKED on Jonathan's Content-Browser Reimport click** (known TASK-086/087/151 MCP debt — no MCP reimport route). Nothing committed (build-master's lane, TASK-183).

## Pre-import gate (mandatory) — PASSED for all 4
Read all 4 `Cache/<CardID>/refine_report.json` + eyeballed the `preview_beauty_cycles.png` renders before anything entered the editor:
- **Tris:** 15000 / budget 15000 (all 4). **UVMap:** `uv_layer_ok=true` (all 4). **Slots in FBX:** `[TeamRegion, <CardID>PBR]` (all 4). **Feet-center:** min_z Knight 0.01 / Cav 0.03 / Pike 0.057 / Mil −0.028. **Team-region area:** 3.5% / 4.3% / 9.1% / 2.8% (cap 35%). **D/N/ORM 1024² PNGs** present; **concepts** already copied to `Content/RawAssets/Concepts/<CardID>.png`.
- Previews read as intended archetypes (Knight sword+shield tank; Cavalry mounted rider; Pikeman long pike; MilitiaMob ragged axe peasant), blue team-region visible. Dark shading = known TRELLIS look (Ogre TASK-150 accepted dark). Only warnings = the warn-only X/Y dim deviation (height-fit keeps Z exact) already flagged/accepted at the TASK-168 eyeball gate.

## ✅ DONE via MCP (readback-verified + saved)

### Textures → `/Game/Textures/` (12 total, all 1024²)
| Asset | SRGB | Compression | Note |
|---|---|---|---|
| `T_<CardID>_D` | true | TC_Default | base color — import defaults correct |
| `T_<CardID>_N` | **false** | **TC_Normalmap** | auto-detected from `_N` suffix (verified all 4) |
| `T_<CardID>_ORM` | **false** | TC_Default | imported SRGB=true; **manually flipped SRGB→false** (ORM linear law); readback-verified false on all 4 |

Exact paths: `/Game/Textures/T_{Knight,Cavalry,Pikeman,MilitiaMob}_{D,N,ORM}`.

### Material instances → `/Game/Materials/Instances/` (4)
- `MI_{Knight,Cavalry,Pikeman,MilitiaMob}_PBR`, each `create`d from parent `/Game/Materials/M_AssetPBR` (master pre-exists, NOT re-authored).
- Master exposes exactly 3 texture params: `BaseColor`, `Normal`, `ORM`. Wired + verified per asset: `BaseColor→T_<CardID>_D`, `Normal→T_<CardID>_N`, `ORM→T_<CardID>_ORM`.
- Sphere-preview capture on all 4 MIs confirms baked D/N/ORM render (distinct per-unit color key, active normal depth, varied metallic/roughness).
- `save_assets` → true for all 12 textures + 4 MIs.

## ⛔ BLOCKED — the mesh overwrite (Jonathan's ONE click)
- `StaticMeshTools.import_file(folder=/Game/Meshes, asset=SM_Knight, source=…/RawAssets/Knight.fbx)` errored verbatim `import_asset: SM_Knight at /Game/Meshes already exists` — **non-mutating**, same as the Ogre (TASK-151). No reimport / console-exec tool in ANY toolset (EditorApp / Asset / StaticMesh / Object / Programmatic all checked). **Do NOT delete+recreate** — that breaks the `BP_Unit_<CardID>` hard-ref + the placement-ghost/cards.csv string soft-refs.
- **Blockout baseline (pre-swap, readback-confirmed still the OLD mesh):** SM_Knight 1244 tris / SM_Cavalry 804 / SM_Pikeman 532 / SM_MilitiaMob 452; each single slot `[MI_TeamColor_Blue]`. Post-reimport target = 15000 tris, 2 slots.
- **Referencers verified (must survive the in-place overwrite by construction):** SM_Knight←BP_Unit_Knight, SM_Cavalry←BP_Unit_Cavalry, SM_Pikeman←BP_Unit_Pikeman, SM_MilitiaMob←BP_Unit_MilitiaMob.
- **Reimport resolves with NO file-picker:** each `SM_<CardID>.AssetImportData.RelativeFilename` points at `../RawAssets/<CardID>.fbx`, the file Stage 2 (TASK-168) overwrote in place — the reimport pulls the new 15k-tri textured mesh.
- **STAGED FOR THE CLICK:** Content Browser at `/Game/Meshes`; the 4 `SM_` assets multi-**SELECTED** (readback-confirmed `[SM_Cavalry, SM_Knight, SM_MilitiaMob, SM_Pikeman]`).

### THE JONATHAN ACTION (cross-posted to 🚨 Blockers)
1. If an auto-reimport toast appears listing source-file changes → click **Don't Import** first (so no blanket import runs).
2. In `/Game/Meshes` (already open, 4 selected) → right-click → **Reimport** (does all 4).

## Post-click finish (art-director, next dispatch — mirrors TASK-151 §"Post-click work")
Per `SM_<CardID>`, after the reimport: (1) verify 15000 tris + fresh source MD5 + bounds vs refine_report; (2) slots should arrive `[TeamRegion, <CardID>PBR]` — assign `[0] TeamRegion → /Game/Materials/Instances/MI_TeamColor_Blue`, `[1] <CardID>PBR → /Game/Materials/Instances/MI_<CardID>_PBR`; (3) `set_nanite_enabled=false`; (4) `generate_convex_collisions(hull_count=4, max_hull_verts=16)`; (5) confirm `BP_Unit_<CardID>` referencer intact; (6) `save_assets`. Watch for the accepted `nearly zero normals/bi-normals (1E-4)` voxel-remesh WARN (Castle/Ogre parity, cosmetic — not a MikkTSpace/degenerate failure).

## Commit manifest (for build-master TASK-183 — NO Git in my lane, nothing committed by me)
- **On disk from TASK-168 (Stage 1/2):** `Content/RawAssets/{Knight,Cavalry,Pikeman,MilitiaMob}.fbx`, `Content/RawAssets/Textures/<CardID>/T_<CardID>_{D,N,ORM}.png`, `Content/RawAssets/Concepts/<CardID>.png`.
- **New this dispatch (imported+saved):** `Content/Textures/T_<CardID>_{D,N,ORM}.uasset` ×12, `Content/Materials/Instances/MI_<CardID>_PBR.uasset` ×4.
- **`Content/Meshes/SM_<CardID>.uasset` ×4 — NOT yet updated** (awaits reimport + post-click finish).
- **Lane isolation honored:** no `CardArt/` paths touched, no level/BP/C++ edits, `M_AssetPBR` not re-authored, `Content/RawAssets/` only READ (mesh batch untouched).

## Slack ledger
🎨 Art + 🚨 Blockers — posted this dispatch (see final report for ts).
