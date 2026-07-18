# Handoff — TASK-194 — pipeline_manifest.json quality pass (art-director, data-only)

**Date:** 2026-07-18
**File touched:** `Tools/ArtPipeline/pipeline_manifest.json` ONLY (no editor, no Blender, no generation runs, no Git). JSON re-validated after edit (ConvertFrom-Json PASS).

## 1) Hero TRELLIS Stage-1 resolution pinned to 1536

Per-asset key `"trellis_resolution": "1536"` (STRING — the Space expects the choice as a string) added to the five hero assets:

- Castle
- CrystalTower
- ArrowTower
- BombTower
- BallistaTower

All other assets carry no pin = the script default "1024". A `_doc.trellis_resolution` entry documents the contract. **Applies to FUTURE re-gens only; nothing was re-run.**

OPS NOTE for whoever runs the next hero re-gen: `trellis_generate.py` currently takes `--resolution` on the CLI and does not yet read this key (script edits belong to the tooling lane, TASK-192/193 own those files). Until a script picks the key up, pass `--resolution 1536` per this pin — the manifest key is the pin of record. Stage-2 (`refine_trellis_glb.py`) parses the manifest with tolerant `.get()` + a required-field list, so the new keys are inert to the current pipeline (verified by inspection, and JSON validity re-checked).

`_doc.albedo_delight` also added as a RESERVED key note (per CONVENTIONS "Meshy second engine (M7.5)"): **no per-asset overrides pinned** — TASK-193 defines the schema and defaults conservative-ON in-script; per-asset strengths should only be set after the TASK-195 Ogre rebake verdict.

## 2) `_guess` team-region debt cleared — all 16 adjudicated (zero `_guess` markers remain)

Method: for every shipped asset I read the measured `team_region` block in `Tools/ArtPipeline/Cache/<Asset>/refine_report.json` AND eyeballed the Cache preview renders (slot-0 TeamRegion renders untextured gray in the workbench previews, so the landing is directly visible; Ogre additionally has dedicated `teamregion_check/` tint renders). Markers renamed: `_verified` (values confirmed correct, kept as-is), `_pending` (no generated mesh yet). The 3 pre-existing `_tuned` entries (Footman, Archer, Castle) were not touched.

| Asset | Measured area | Verdict | Evidence (what the shipped renders show) |
|---|---|---|---|
| Ogre | 2.1% / 353 faces | verified | Band rides the pauldron/shoulder line; zero head/horn/face paint (teamregion_check renders) |
| Knight | 3.5% / 453 faces | verified | Helm crown + pauldron caps; visor/face untouched — Footman recipe transferred cleanly |
| Miner | 4.1% / 572 faces | verified | Shoulder/strap caps; bare face + hat crown spared |
| Cavalry | 4.3% / 700 faces | verified | Rider armor + saddle flecks; NO horse head/ear paint; caparison add unnecessary |
| Cleric | 2.2% / 433 faces | verified | Mantle/collar shelf below the hood; hood + face spared; robe-mantle tune unnecessary |
| Longbowman | 3.8% / 558 faces | verified | Hood crown (garment, not skin) + shoulder band; quiver-box add unnecessary |
| MilitiaMob | 2.8% / 413 faces | verified | Clean shoulder mantle; light read but gate-accepted (SwarmCount 4 multiplies it on-field) |
| Pikeman | 9.1% / 1318 faces | verified | Helm/hood crown + shoulder/upper-chest caps; no pike striping; z-bands sit slightly low because the pike tip defines mesh height — recorded, not fixed (shipped landing is correct) |
| Sapper | 3.3% / 449 faces | verified | Collar/mantle band; NO helmet on the generated mesh so the no-helm_dome call was right |
| ArrowTower | 9.6% / 2309 faces | verified | Full cone roof + parapet ring — textbook top-down read |
| BombTower | 10.7% / 2661 faces | verified | Mortar barrel + inner platform deck |
| BallistaTower | 5.8% / 1349 faces | verified | Ballista mount/mast/railings; the feared arm exclusion proved unnecessary |
| Barracks | 15.3% / 3078 faces | verified | Full gabled roof — largest region, still well under the 0.4 cap |
| CrystalTower | 8.3% / 1952 faces | verified **with deviation recorded** | Selection landed on the upward CRYSTAL-TOP facets, not the intended stone roofline. Accepted as shipped: TASK-190 (7ec9916) kept slot-0 team recolor + put the height-masked glow on slot 1, so team-tinted crystal tops + glowing steep facets coexist. Future-re-gen note in the manifest: shrink the box XY to exclude the crystal footprint if the glow should own the whole crystal |
| Wall | — (no mesh) | **pending** | Stage-1 quota-blocked (handoffs/TASK-170-171-artist.md); top-band recipe matches the 4 shipped tower/Barracks outcomes (5.8–15.3% clean) — sound prior; adjudicate at the TASK-170 gate |
| DeepMine | — (no mesh) | **pending** | Same; roof band mirrors Barracks (15.3% clean); keep the shaft-opening watch item; adjudicate at the TASK-171 gate |

All shipped fractions sit at 2.1–15.3%, comfortably inside the 0.35 (units) / 0.4 (buildings) caps — no misfire-law violations found (no bare-head painting, no weapon-arm striping).

## 3) Downstream

- **TASK-195 (validation renders):** manifest is current — this was its last file-side blocker from me; 192/193 QA still gate it.
- **TASK-196 (build-master):** `Tools/ArtPipeline/pipeline_manifest.json` is ready to ride the Track-C commit. No other files touched by this task.
- **TASK-170/171 (Wall/DeepMine rerun):** the two `_pending` notes point at their gates; Stage 2 waits on TASK-193 per M7.5 decision 3.
- Selector VALUES were changed on ZERO assets — every shipped selector was confirmed correct against ground truth, so future re-gens reproduce the accepted looks exactly.
