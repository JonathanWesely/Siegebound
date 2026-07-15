# TASK-149 Handoff — Ogre Stage 2: REFINE (headless Blender) — SUCCESS + EYEBALL-GATE PACKET (TASK-150)

- **From:** art-director
- **Date:** 2026-07-14
- **Status:** ready-for-integration. FBX + textures produced; STOP at the TASK-150 eyeball gate before Stage 3 import (TASK-151). No Unreal editor touched (build-master's parallel task owns it).

## Command (spec drift note)
Task spec said `--asset Ogre`, but `refine_trellis_glb.py` (TASK-083) actually requires `--card-id`. Ran the pilot invocation:
`& "C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Ogre` → exit 0, 42.7 s. (Manifest key is `Ogre`; that is what matters. Flagging the flag name for build-master/future runs.)

## Outputs (all written, verified)
- **`Content/RawAssets/Ogre.fbx`** — OVERWROTE the blockout in place (intended). Re-imported independently: object `SM_Ogre`, slots IN ORDER `[TeamRegion, OgrePBR]`, UVMap, 15,000 tris.
- **`Content/RawAssets/Textures/Ogre/T_Ogre_{D,N,ORM}.png`** — all 1024² RGB. D = baked albedo (see WATCH below); N = valid tangent normal (neutral blue background + per-island detail — bake confirmed good); ORM = packed AO/Rough/Metal.
- **`Content/RawAssets/Concepts/Ogre.png`** — the accepted CROP concept copied here (655×730). (Commit at TASK-152.)
- **`Cache/Ogre/refine_report.json`** + previews `Cache/Ogre/previews/preview_{front,back,threequarter,top,beauty_cycles}.png`.
- **`Cache/Ogre/teamregion_check/team_*.png`** — art-director-authored team-region HIGHLIGHT (slot 0 = magenta, slot 1 = grey, 6 angles) so the gate can judge placement unambiguously.

## Refine stats vs manifest
| Metric | Value | Note |
|---|---|---|
| Tris | 15,000 | = budget exactly |
| Bounds (UE) | 221.33 × 227.62 × 288.18 | Z matches target 290 (height-fit) |
| Conform dims | 221.6 × **228.6** × 290.0 | **WARN**: Y 228.6 vs target 98.8 |
| minZ (UE) | -0.161 | feet-center ≈ 0 (Footman +0.03 / Archer +0.05 pattern) |
| UV layer | `UVMap` | ok |
| Slots | `[TeamRegion, OgrePBR]` | correct names + order |
| Team region | 353 faces, **2.1%** area (cap 35%) | 1 selector `shoulder_caps` |
| Textures | D/N/ORM 1024² | bake OK |
| Cleanup | 460k→221k verts, 182 islands removed, 4 holes filled | |

**Conform WARN is EXPECTED/warn-only** under `fit_mode: height`: only Z is matched (290 exact); X/Y deviate. The generated ogre's maul juts FORWARD, so its Y depth grew to 228.6 vs the slim blockout's 98.8 — same "new mesh footprint ≠ blockout" pattern as Footman ([86,48,180] vs [147,80,180]) and Archer (Y grew). The new ogre has a DEEPER/bulkier footprint than the blockout (WATCH for capsule sizing at integration, but not a Stage-2 defect).

---

## ⬛ EYEBALL-GATE PACKET FOR JONATHAN (TASK-150) — what to judge

Preview images to open:
- Orientation/silhouette: `Cache/Ogre/previews/preview_front.png`, `preview_threequarter.png`, `preview_top.png`, `preview_beauty_cycles.png` (Cycles, shows the blue team region), plus `Cache/Ogre/raw_inspect/raw_*.png`.
- Team-region placement (clearest): `Cache/Ogre/teamregion_check/team_A_negY.png` (front) + `team_E_threeq.png` (¾) — magenta = the slot-0 TeamRegion face-set.

**(a) ORIENTATION — looks CORRECT, `pre_rotate_z_deg: 0.0`.** The ogre's front (face, chest, pauldron, maul) faces -Y in both the raw eyeball and the refine `preview_front` (figure faces camera). No rotation appears needed. If it reads rotated to you, set `assets.Ogre.pre_rotate_z_deg` (e.g. 180 if backwards, ±90 if side-on) and re-run TASK-149.

**(b) TEAM REGION — lands on the RIGHT zone (armor/shoulders), a valid PASS; optional tune available.** The magenta band sits across the **upper chest / shoulders / spiked pauldron** — it does NOT touch the head/face, the maul, or the large green skin / belly / legs. That is the "good" outcome (armor accent, not bare face/weapon/skin). Caveat: at 2.1% it is on the LIGHT side (Footman 5.7% / Archer 6.3%) and SPECKLED (upward-facing facets only) rather than a solid pauldron cap, so the RTS team-read is subtle. If you want a stronger/cleaner read, tune `assets.Ogre.team_region.selectors` (e.g. lower `min_dot` from 0.55 → ~0.3 to catch more of the angled pauldron faces, and/or add a dedicated box on his right shoulder) and re-run TASK-149. Acceptable as-is.

**(c) WATCH — DARK/DESATURATED TEXTURE (Stage-1 generation characteristic, NOT a refine bug).** TRELLIS generated a MOODY, near-black ogre and largely LOST the concept's vivid GREEN skin + tan leather — the baked `T_Ogre_D` is dark olive/near-black (confirmed against the raw GLB, which was already dark). The refine bake faithfully transferred it, so re-refining will NOT brighten it. If the dark read is acceptable for a menacing siege brute → APPROVE. If you want the green back or worry it reads as a dark blob / weak team contrast at gameplay distance, the fix is a **Stage-1 reroll** (different `--seed`, or a brighter/higher-contrast concept crop) — that loops back to TASK-148 and DOES cost HF quota (unlike a-/b- tunes, which are free Stage-2 re-runs).

**Gate outcomes:** APPROVE → unblocks Stage 3 (TASK-151). TUNE (a/b) → free headless TASK-149 re-run. REROLL (c) → TASK-148 (HF cost). The cached GLB means a/b tunes cost nothing.

## Commit manifest (for TASK-152 build-master — no Git in my lane)
`Content/RawAssets/Ogre.fbx` (refined, overwrites blockout), `Content/RawAssets/Textures/Ogre/T_Ogre_{D,N,ORM}.png` (new), `Content/RawAssets/Concepts/Ogre.png` (the CROP — new), `Tools/ArtPipeline/pipeline_manifest.json` (Ogre entry from TASK-147; add a `_tuned` note here if the gate tunes a/b). `Cache/Ogre/*` is gitignored — do NOT commit. Lane isolation honored: no CardArt paths, no /Game/ writes, no level/BP/C++ edits.
