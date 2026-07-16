# TASK-166 Handoff — Mesh batch prep: 16 manifest entries + blockout measure (art)

- **From:** art-director
- **Date:** 2026-07-15
- **Status:** `ready-for-integration` (file-only — rides the TASK-183 commit). No HF quota used, no editor/MCP, no Git.

## What was done
Measured all 16 M7 blockout FBXs HEADLESS (`blender.exe --background`, VISUAL mesh bounds excluding UCX_*) and authored their `pipeline_manifest.json` "assets" entries. JSON parses; the existing Footman/Archer/Ogre/Castle entries + `defaults` are UNTOUCHED (verified 20 total assets, 4 old + 16 new).

## Measured dims (UE units, feet/ground-center, min_z≈0) → `target_dims_ue`
| CardID | Path | dims_ue (X×Y×Z) | blockout tris |
|---|---|---|---|
| Knight | unit | 162.0 × 62.7 × 190.0 | 1244 |
| Miner | unit | 129.5 × 62.0 × 173.0 | 1124 |
| Cavalry | unit | 75.5 × **254.6** × 208.0 | 804 (mounted — big Y is the horse; Z-fit only) |
| Cleric | unit | 83.5 × 79.0 × 182.0 | 920 |
| Longbowman | unit | 81.8 × 57.4 × 184.0 | 1984 |
| MilitiaMob | unit | 74.5 × 34.3 × 149.0 | 452 (single figure; SwarmCount spawns 4) |
| Pikeman | unit | 76.8 × 60.3 × 190.0 | 532 |
| Sapper | unit | 73.9 × 98.0 × 169.0 | 620 |
| ArrowTower | building | 250 × 250 × 497.4 | 512 |
| Wall | building | 400 × 100 × 250 | 264 |
| BombTower | building | 250 × 250 × 451 | 580 |
| BallistaTower | building | 250 × 270 × 500 | 512 |
| Barracks | building | 400 × 419 × 348.9 | 312 |
| DeepMine | building | 300 × 305 × 300 | 1064 |
| CrystalTower | building | 250 × 250 × 487.8 | 1212 (has an M_CrystalGlow slot) |
| GoldNode | PROP | 190.2 × 200 × 249.4 | 222 |

## Manifest fields authored (per the M7 batch scope law)
- **UNITS (8):** `category:unit`, `fit_mode:height`, `tri_budget:15000`, `bake:1024`, feet-center, `voxel:1.5`, two-slot `team_region` STARTING GUESS (`ucx:null` → hulls generated at Stage-3 import). Helmeted recipe (helm_dome + shoulder caps) for **Knight, Pikeman**; bare/hooded recipe (shoulder caps only, NO helm_dome — avoids painting the face, Archer TASK-087 lesson) for **Miner, Cavalry, Cleric, Longbowman, MilitiaMob, Sapper**. All guesses tuned at the per-wave eyeball gates (TASK-168/169).
- **BUILDINGS (7):** `category:building`, `fit_mode:box`, `tri_budget:20000`, `bake:2048`, ground-center, `voxel:3.0`, bake cage 6/ray 16, two-slot `team_region` roof/top guess, `ucx` = STARTING-GUESS single full-height footprint box (re-derive wall-footprint-exact against the generated mesh at the import wave — Castle TASK-087 precedent; a solid tower box is fine, hollow structures MUST be re-hulled).
- **GoldNode PROP (1):** `team_region: null` (the EMISSIVE ECONOMY-PROP VARIANT — M7 decision 1). `tri_budget:12000`, box-fit, 2048, footprint ucx. `_variant` note records the single-slot `GoldNodePBR` + preserved warm-yellow emissive requirement.

## Flags for downstream (recorded in-manifest as `_` notes)
1. **Building tri budget = 20000** (art-director-set; the board delegated "tri per building budget" — only anchors are unit 15k / castle 40k). GoldNode 12000. Flagged for build-master perf review.
2. **GoldNode single-slot handling is a TASK-171 production concern** — the current `refine_trellis_glb.py.build_final_materials` still emits two slots when `team_region` is null; it must be extended at TASK-171 before GoldNode is run.
3. **CrystalTower emissive** — the blockout has an M_CrystalGlow slot; a plain two-slot PBR bake won't preserve the glow. TASK-170 decides bake-`T_CrystalTower_E`-into-MI vs a dedicated glow slot. Flagged, not resolved here.
4. **Concept casing reconcile** (step 3) is a no-op today — no lowercase `Inbox/<cardid>.png` drops exist yet; reconcile when TASK-167 concept drops land.

## Commit manifest (build-master TASK-183)
- `Tools/ArtPipeline/pipeline_manifest.json` (+16 entries) — CODE, rides the TASK-183 commit. Nothing else touched.
