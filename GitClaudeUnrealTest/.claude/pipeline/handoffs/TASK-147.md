# TASK-147 Handoff — Ogre pipeline prep (casing + measure + manifest entry)

- **From:** art-director
- **Date:** 2026-07-14
- **Status:** done. File-only prep, no editor/MCP, no HF quota.

## (1) Casing reconcile — DONE
- `Tools/ArtPipeline/Inbox/ogre.png` → `Tools/ArtPipeline/Inbox/Ogre.png` (Windows case-only rename via two-step `mv`; content byte-identical, not re-encoded). Lowercase `ogre.png` is gone. Inbox is gitignored (no git step).
- NOTE: `Inbox/Ogre.png` was LATER replaced during Stage 1 (TASK-148) with a single-subject CROP — see that handoff. The original 4-view sheet is preserved at `Inbox/Ogre_original_4view.png` (gitignored) and in the session scratchpad.

## (2) Blockout measurement — DONE (headless Blender, read-only)
Measured `Content/RawAssets/Ogre.fbx` (the existing M4 Set-II blockout, object `SM_Ogre`, 1572 tris) headless before Stage 2 overwrote it:
- **Dims (UE units): X 226.72 × Y 98.80 × Z 290.00**, feet-center (minZ ≈ 0), front -Y.
- Cross-checks the TASK-066 handoff EXACTLY (226.7 × 98.8 × 290.0, "roster's LARGEST unit, 290 tall", 1572 tris). Independent confirmation, no discrepancy.
- Z 290 is the roster's tallest; X 226.72 width includes the overhead club (weapon overhang).

## (3) Manifest entry — DONE
Added `assets.Ogre` to `Tools/ArtPipeline/pipeline_manifest.json` (inserted after Archer, before Castle — units grouped). UNIT path, modeled on Footman/Archer:
- `category:"unit"`, `mode:"bake"`, `tri_budget:15000`, `bake_resolution:1024`, `origin:"feet-center"`, `fit_mode:"height"`, `voxel_size_ue:1.5`, `pre_rotate_z_deg:0.0` (STARTING GUESS), `ucx:null`.
- `target_dims_ue:[226.72, 98.8, 290.0]` + `_dims_source` note recording the headless measurement.
- `team_region.max_fraction:0.35`, ONE selector `shoulder_caps` (box z 0.66–0.84, x 0.10–0.90, y 0.10–0.90 AND normal-up min_dot 0.55). **Deliberately NO helm_dome** (the ogre has a bare horned head — a full-width top-z dome would paint his face/horns, the Archer bare-head lesson TASK-087). `_guess` note records the reasoning.
- JSON validated (parses; `assets` keys = Footman, Archer, Ogre, Castle). Footman/Archer/Castle/`defaults` untouched.

## Acceptance — met
`Inbox/Ogre.png` exists (lowercase gone); `pipeline_manifest.json` parses with a complete measured Ogre UNIT entry; no other asset entries changed.
