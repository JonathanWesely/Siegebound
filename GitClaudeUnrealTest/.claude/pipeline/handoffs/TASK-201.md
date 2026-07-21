# Handoff — TASK-201 — Fleet Meshy retexture, UNITS WAVE (art-director)

**Date:** 2026-07-21 · **Status:** UNITS WAVE DONE — 11/11 concept-true, ZERO held. HEADLESS only (no editor,
no import — TASK-202's lane), no Git mutations. Buildings wave (incl. Wall+DeepMine `--mode image3d`) NOT started —
separate dispatch per the TASK-200 ruling.

## Formula executed (the TASK-199-proven hybrid, per asset)
Stage-1.5 `meshy_generate.py --mode retexture <CardID>` (dense donor `Cache/<CardID>/trellis_raw.glb` + style ref
`Inbox/<CardID>.png`) → Stage-2 `refine_trellis_glb.py --input Cache/<CardID>/meshy_retex.glb` with **delight at
in-script DEFAULTS** (aoDiv 0.6 / floor 0.35 / γ0.85 / gain 1.0) → production outputs overwrote
`Content/RawAssets/<CardID>.fbx` + `Textures/<CardID>/T_<CardID>_{D,N,ORM}.png`.

**Override handling (per dispatch):** the 5 dark units (Ogre, Knight, Cavalry, Pikeman, MilitiaMob) carry TASK-225
tuned `albedo_delight` pins in `pipeline_manifest.json`. Per the TASK-199 finding (tuned strengths OVER-lift a Meshy
source), those 5 ran via `--manifest <temp copy>` = the real manifest minus exactly those 5 override objects (a CLI-level
override; **the real pipeline_manifest.json was NOT edited by this task** — its current working-tree diff is TASK-225's
recorded edit). Each refine_report.json records which manifest file it used. NOTE for manager: the pins remain in the
manifest and still make sense for TRELLIS-sourced rebakes; if the fleet stays Meshy-sourced, a future amendment could
scope them ("trellis-source only") — not mine to rule.

## Per-asset results (all: 15000/15000 tris, slots `[TeamRegion, <CardID>PBR]`, UVMap ok, feet-ground origin,
## warnings BYTE-PARITY with shipped reports, engine=meshy-retex in state.json)

| Asset | Credits | Mean linear albedo (pre-lift → post) | p99 | Shoulder-soft frac | Team slot0 | Fidelity verdict |
|---|---|---|---|---|---|---|
| Ogre | 0 (TASK-198 GLB reused) | 0.0146 → 0.0376 | 0.325 | 0.0001 | 2.1% | **concept-true** (green skin class + brown hide — the ruling's bar, met) |
| Footman | 10 | 0.0294 → 0.0710 | 0.304 | 0.0001 | 5.7% | **concept-true** (green surcoat + gold gryphon, readable shield device) |
| Archer | 10 | 0.0467 → 0.1008 | 0.681 | 0.0052 | 6.3% | **concept-true** (auburn hair, green gambeson + gold crest, tan leathers) |
| Knight | 10 | 0.0214 → 0.0553 | 0.449 | 0.0008 | 3.5% | **concept-true** (tan tabard, fur collar, leather pouches; plate reads dark-steel in the dim workbench preview — known caveat, UE reads brighter) |
| Miner | 10 | 0.0350 → 0.0800 | 0.723 | 0.0065 | 4.1% | **concept-true** (warm brown coat, tan scarf, wooden pick haft) |
| Cavalry | 10 | 0.0453 → 0.0929 | 0.885 | 0.0157 | 4.3% | **concept-true** (CHESTNUT HORSE recovered + red plume + cream blanket — was the fleet's darkest at 0.0009 pre-lift TRELLIS; Meshy source ~50× brighter; TASK-225 residual CLOSED) |
| Cleric | 10 | 0.1029 → 0.1959 | 0.929 | 0.0603 | 2.2% | **concept-true** (cream robes, skin tones, sandals; 6% shoulder-soft = the legitimately-white robes, zero hard clip) |
| Longbowman | 10 | 0.0911 → 0.1669 | 0.935 | 0.0533 | 3.8% | **concept-true** (green tunic, tan hood, yellow trousers, warm bow) |
| MilitiaMob | 10 | 0.0404 → 0.0892 | 0.741 | 0.0077 | 2.8% | **concept-true** (green kettle helm, wooden pitchfork, brown shield) |
| Pikeman | 10 | 0.0497 → 0.1061 | 0.885 | 0.0179 | 9.1% | **concept-true** (orange-brown pike shafts — the concept's signature — plus yellow tabard, blue-gray trousers, green kneepads) |
| Sapper | 10 | 0.0442 → 0.0961 | 0.846 | 0.0131 | 3.3% | **concept-true** (skin tones, brown leather coat, cream collar, iron bomb + warm fuse flame) |

**Held list: EMPTY.** No asset came out off-palette; no backup restores were needed.

## Credits ledger
100 credits consumed this task (10 × 10 new retextures; Ogre reused TASK-198's paid GLB — no double spend).
Balance **3137 → 3037** (live, from per-run `credits_before/after` in each `Cache/<CardID>/state.json` — task ids,
input shas, output shas all recorded there; TRELLIS provenance preserved by the merge law).

## Evidence (durable Cache paths)
- **Triptych strips (before | after | concept, front + threequarter):**
  `Tools/ArtPipeline/Cache/_TASK201_report/TRIPTYCH_<CardID>.png` × 11 — the per-asset color-fidelity record.
- Per-asset canonical: `Cache/<CardID>/refine_report.json` + `previews/` (now match the NEW shipped RawAssets).
- Stage logs: session scratchpad `t201/` (ephemeral); refine DONE lines + meshy SUCCESS lines quoted in reports.

## Backups (rollback = copy back over Content/RawAssets)
Per asset `Tools/ArtPipeline/Cache/<CardID>/TASK201_shipped_backup/`: shipped FBX, `Textures/T_<CardID>_{D,N,ORM}.png`,
`shipped_hashes_before.txt` (sha256 × 4), `refine_report_shipped.json`, `previews_shipped/`. Post-run check: all 11
assets show 4/4 files changed vs backup (full overwrite — Meshy rebakes N/ORM too, unlike TASK-225's D-only lift;
TASK-202 should therefore reimport ALL THREE textures + FBX, not just D).

## Incidents / notes (none blocking)
1. **Batch-cap kill, zero orphans:** the first 5 retextures ran as one background bash which hit the 10-min cap right
   at the end — every task had already completed + downloaded (verified per-asset GLB + state.json; Cavalry landed
   16 s before the cap). Two transient local DNS failures (getaddrinfo) during a precautionary recovery probe
   self-resolved; NO task was re-created, NO double spend. Lesson recorded: run retextures one-per-call foreground
   (~2–4 min each) — that's how batch 2 ran.
2. **Tool gap (minor, for manager/programmer):** `refine_report.json` does NOT carry the `engine` field forward
   (CONVENTIONS "Provenance" expects it) — engine provenance currently lives only in `state.json`. Cosmetic; flag for
   a future meshy/refine touch.
3. `Cache/Footman/state_failed.json` is PRE-EXISTING TRELLIS-era dirt (old gradio `hf_token` TypeError), not from this
   run; left untouched (surviving-artifact law).
4. Working tree carries ~143 dirty files from other lanes (anim lane, board/CONVENTIONS edits, TASK-225's manifest
   edit). This task's writes are confined to: `Content/RawAssets/<11 units>.fbx`, `Content/RawAssets/Textures/<11>/`,
   `Tools/ArtPipeline/Cache/` (state/reports/previews/backups/GLBs/_TASK201_report). No Git mutations by me.

## Downstream
- **TASK-202 (art, editor-serial):** reimport the 11 units via the proven commandlet lane — FBX + ALL THREE textures
  per asset (see backup note above); TASK-220's LOD-group line applies for free; MCP readback of slots/Nanite-OFF.
- **TASK-241 (build-master):** commit window after TASK-202's integration check.
- **Buildings wave (rest of TASK-201):** Castle/towers/Barracks/GoldNode per the ruling + Wall/DeepMine via
  `--mode image3d` — needs its own dispatch; ~90 credits at the observed rate.

---

# Wave 2 — BUILDINGS retexture ×6 + Wall/DeepMine image3d ×2 (art-director, 2026-07-21)

**Status: WAVE 2 DONE — 8/8 CONCEPT-TRUE, ZERO held. TASK-201 complete (11 units + 8 buildings = 19 runs; the 16/16
card roster is closed for the retexture program; Castle excluded by the ruling's wave shape — already shipped quality).**
HEADLESS only (no editor, no import — TASK-202's lane), no Git mutations by me.

## Formula
Retexture ×6 (existing TRELLIS donors): `meshy_generate.py --mode retexture <CardID>` (donor `Cache/<CardID>/trellis_raw.glb`
+ style ref `Inbox/<CardID>.png`) → Stage-2 `refine_trellis_glb.py --input Cache/<CardID>/meshy_retex.glb`.
Image3d ×2 (NEW meshes, roster closers — TRELLIS lane still Space-broken): `meshy_generate.py --mode image3d <CardID>`
(concept → `Cache/<CardID>/meshy_raw.glb`, topology=triangle, target_polycount=300000, PBR on) → FULL Stage-2 conform as
new buildings. ALL 8 ran **delight at in-script DEFAULTS** (aoDiv 0.6 / floor 0.35 / γ0.85 / gain 1.0) with the REAL
manifest — no building carries a TASK-225 pin, so no temp-manifest override was needed this wave. One-call-per-asset
FOREGROUND serial (the wave-1 lesson; zero batch-cap incidents this wave).

## Per-asset results (all: tris at budget, slots exactly `[TeamRegion, <CardID>PBR]`, UVMap ok, ground-center origin,
## box-fit dims within ±10%, state.json engine + credits recorded)

| Asset | Mode | Credits | Mean linear albedo (pre→post) | p99 | Shoulder | Team slot0 | Fidelity verdict |
|---|---|---|---|---|---|---|---|
| ArrowTower | retex | 10 | 0.019 → 0.041 | 0.477 | 0.2% | 9.5% | **concept-true** (light-gray masonry + warm wood door + gold flag; roof-cone color moot — TeamRegion recolors it in-game) |
| BombTower | retex | 10 | 0.030 → 0.052 | 0.546 | 0.2% | 10.7% | **concept-true** (navy cannon + cannonball stack recovered, warm door, cream/gold flag) |
| BallistaTower | retex | 10 | 0.014 → 0.031 | 0.415 | 0.2% | 5.8% | **concept-true** (warm cedar timber frame recovered from cold dark brown; gray stone base; gold flags) |
| Barracks | retex | 10 | 0.029 → 0.058 | 0.789 | 0.9% | 15.4% | **concept-true** (blue-gray stone lightened, warm timbers, cream banner, brown arched door; roof = team slot) |
| CrystalTower | retex | 10 | 0.032 → 0.055 | 0.931 | 1.6% | 8.3% | **concept-true** (violet/cyan crystal palette recovered from near-black; gray masonry; tan banner) |
| GoldNode | retex | 10 | 0.071 → 0.103 | 0.950 | 7.0% | 0.0% (variant law) | **concept-true** (saturated gold crystals + cool gray rock + warm crevice glow baked in D; coins gold; 7% shoulder = legitimately-bright gold, zero hard clip) |
| Wall | **image3d** | **30** | 0.051 → 0.081 | 0.663 | 0.5% | 2.1% | **concept-true** (gray crenellated masonry, moss, cream banner, arched postern; NEW mesh 20000/20000 tris @ 400×100×250) |
| DeepMine | **image3d** | **30** | 0.019 → 0.037 | 0.426 | 0.1% | 2.2% (tuned) | **concept-true** (charcoal rock dome, brown timber portal, cream banner, cart + rails + rope; NEW mesh 20000/20000 tris @ 300×305×300) |

**Held list: EMPTY.** No off-palette output; no backup restores needed.

## Credits ledger — NOTE the image3d cost finding
**`--mode image3d` costs 30 credits/run, not the ~10 estimated on the board** (retexture = 10, confirmed ×6).
Wave-2 total = 6×10 + 2×30 = **120 credits** (board estimate was ~90). Balance **3037 → 2917** (live, per-run
`credits_before/after` in each `Cache/<CardID>/state.json` with Meshy task ids + input/output shas; TRELLIS provenance
preserved by the merge law). TASK-201 grand total: 220 credits (100 units + 120 buildings), balance 3137 → 2917.

## Wall + DeepMine conform details (the roster closers)
- **Wall:** raw Meshy mesh is NEAR-CUBIC (1.68×1.90×1.72 m — the concept's drawn perspective depth taken literally).
  Box-fit compresses Y ~4:1 to the manifest 400×100×250; front/back faces (the wall's dominant in-game read) are
  undistorted, end-cap bricks compress — ACCEPTED at the eyeball (ends abut walls/terrain in play; reads as a proper
  wall segment with double-parapet walk). Team selector (crenellations z≥0.70 up-band) fired as guessed: 2.1% / 479
  faces, merlon tops + walk deck, no facade striping — manifest `_pending` → `_tuned` (mesh-verified, kept as-is).
  UCX: manifest single footprint box 400×100×250 authored into the FBX (`UCX_SM_Wall_00`); solid barrier, correct fit.
- **DeepMine:** the Barracks-style z 0.5–1.0 roof-band selector **MISFIRED on the real mesh** — no constructed
  headframe exists; it painted the entire NATURAL ROCK dome crown (18.8% / 3471 faces, probe-measured). Per the
  paint-constructions-not-nature misfire law (the Archer bare-head lesson's building analog), REPLACED with
  `portal_beams` (timber entrance lintel/crossbeam tops, front y-band, box [0.10,0.0,0.35]–[0.90,0.35,0.72] dot≥0.3):
  2.2% / 583 faces — the accepted subtle band (Ogre 2.1%, Cleric 2.2%); dome-crown alternative (6.1%) probed +
  REJECTED. Stage-2 re-ran (no credit cost). Manifest `_pending` → `_tuned` with the full adjudication. Shaft-opening
  watch: no shaft faces selected.
  UCX: manifest single footprint box 300×305×300 (`UCX_SM_DeepMine_00`); entrance uncarved = blockout-parity collision.
- Both concepts copied to `Content/RawAssets/Concepts/{Wall,DeepMine}.png` per the accepted-concept law (they were the
  last two missing — Concepts/ now covers the full pipeline set).

## Special-law compliance
- **CrystalTower:** texture/mesh refresh ONLY — Stage-2 emitted the SAME two-slot `[TeamRegion, CrystalTowerPBR]`
  arrangement as shipped (8.3% ≈ shipped 8.3%), so the TASK-190 in-engine slot-1 → MI_CrystalGlow repoint is NOT
  disturbed by a same-path reimport. **TASK-202 must keep slot 1 pointed at MI_CrystalGlow (not MI_CrystalTower_PBR)
  and verify MI_CrystalGlow still resolves the refreshed T_CrystalTower_{D,N,ORM}.** Flag only — nothing to fix here.
- **GoldNode:** single-slot emissive VARIANT law — outputs refresh textures only; FBX still carries the two-slot
  byte-parity arrangement with ZERO TeamRegion faces (the 1 warning = the known variant-law line, byte-parity with the
  shipped report). **Its MI wiring (all slots → M_GoldGlow) is IMPORT-SIDE** — TASK-202 re-applies it.

## Evidence + backups
- Triptychs (before | after | concept, front + threequarter): `Tools/ArtPipeline/Cache/_TASK201_report/TRIPTYCH_<CardID>.png`
  × 8 new (19 total). Wall/DeepMine "before" = blockout renders (matched preview style).
- Per-asset canonical: `Cache/<CardID>/refine_report.json` + `previews/` (match the shipped RawAssets).
- Backups per asset `Cache/<CardID>/TASK201_shipped_backup/`: retexture ×6 = shipped FBX + 3 PNGs + sha256 + shipped
  report + previews (post-run check: **4/4 files changed** on all 6 — Meshy rebakes N/ORM too; TASK-202 reimports ALL
  THREE textures + FBX). Wall/DeepMine = blockout FBX + sha256 (22 KB / 38 KB → 771 KB / 801 KB textured).

## Notes for manager
1. `pipeline_manifest.json` edited by THIS task: Wall + DeepMine `team_region` blocks only (`_pending` → `_tuned`,
   DeepMine selector replaced). The file ALSO still carries TASK-225's pre-existing albedo_delight pins — two lanes,
   one dirty file; a commit message should name both.
2. Board credit-estimate correction: image3d = 30 cr/run (recorded above).
3. Wave-1's refine_report `engine`-field gap applies here too (engine provenance in state.json only).

## Downstream
- **TASK-202 (art, editor-serial):** all 8 ready — 6 same-path refreshes (FBX + D/N/ORM ×3; CrystalTower + GoldNode MI
  laws above) + Wall/DeepMine FIRST TEXTURED import over the blockout SM_ via the box-UCX branch (authored UCX inside
  the FBX; refs must survive; slots `[TeamRegion → MI_TeamColor_Blue, <CardID>PBR → MI_<CardID>_PBR]`, GoldNode
  variant, Nanite OFF). Closes the TASK-170/171/173 lineage 8/8 → the 16/16 roster.
- **TASK-241 (build-master):** commit window after TASK-202.
