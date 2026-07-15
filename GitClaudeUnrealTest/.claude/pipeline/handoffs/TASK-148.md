# TASK-148 Handoff — Ogre Stage 1: GENERATE (TRELLIS.2) — SUCCESS after an in-lane concept fix

- **From:** art-director
- **Date:** 2026-07-14
- **Status:** done. `Cache/Ogre/trellis_raw.glb` + `state.json` + `api_schema.json` written; raw mesh eyeballed = one clean, plausible ogre.

## Health probe — PASS
`uv run trellis_generate.py --check` → exit 0. Space reachable; all three endpoints present (`/preprocess_image`, `/image_to_3d`, `/extract_glb`); NO API drift. Ran bare (no SSL_CERT_FILE — Norton HF exclusions held, per TASK-086/087).

## Generation run #1 — technically succeeded, mesh MANGLED (concept problem)
- `uv run trellis_generate.py Ogre` on the renamed `Inbox/Ogre.png` (the 4-view sheet, 1408×768) → exit 0, seed 0, ~96 s, glb 21,023,912 bytes.
- **Raw eyeball caught the failure:** the concept Jonathan dropped is a **4-VIEW TURNAROUND SHEET** (left profile + front ¾ + head close-up + back view) on a gridded background. TRELLIS's cutout kept ALL FOUR subjects and extruded them into one fused blob (X was the longest axis = four figures side-by-side; renders in `Cache/Ogre/raw_inspect/` from run #1 showed the left profile, the central ogre, a floating head, and a back-view body all as separate 3D geometry).
- A seed reroll would NOT fix this — the root cause is the multi-subject concept (README single-subject law), not seed variance. So I did not burn quota on blind rerolls.

## In-lane fix — single-subject concept crop (art-director concept-prep authority)
- Per the README "Concept image guidance" (single subject, ¾ view — my lane), I CROPPED the central front ¾ ogre out of the sheet: `cropA_wide` = 655×730, capturing the full figure + its maul, excluding the other three subjects.
- **Original preserved** at `Inbox/Ogre_original_4view.png` (gitignored) and session scratchpad `Ogre_ORIGINAL_4view.png`. The crop is now `Inbox/Ogre.png` (what Stage 1 reads). The TASK-147 "do not alter image content" instruction was scoped to the rename step; concept prep for TRELLIS is documented art-director work.

## Generation run #2 (on the crop) — SUCCESS, clean single ogre
- `uv run trellis_generate.py Ogre` → exit 0, seed 0, ~119 s (preprocess 6.6s / image_to_3d 55.6s / extract_glb 56.5s). `Cache/Ogre/trellis_raw.glb` = 22,559,448 bytes. Benign non-square WARN (655×730, accepted — same benign class as the Footman/Archer 1408×768 inputs).
- **Raw eyeball (re-render `Cache/Ogre/raw_inspect/`):** ONE coherent, complete ogre. 475,801 tris, single mesh `geometry_0`, 2 TRELLIS textures. Bounds now Z-tallest (0.762 × 0.786 × 0.997), i.e. a standing figure not a wide multi-blob. Faithful to the concept: horns, spiked pauldron (his right shoulder), chest straps + pendant/chains, wide belt w/ pouches, tattered loincloth, barefoot, stone/bone maul held low in his fist. Front (face/chest/maul) reads toward -Y ⇒ early sign `pre_rotate_z_deg 0.0` is right (the refined previews at the TASK-150 gate are the authority).

## Provenance for the gate / commit
- **HF exit codes seen:** all 0 (no 2/3/4/5 — no blocker, no quota pause). ~2.7 GPU-min total across both runs; HF PRO headroom fine.
- **The ACCEPTED concept is the CROP**, not the 4-view sheet. Committed record (TASK-152) should use `Content/RawAssets/Concepts/Ogre.png` = the crop (already copied at TASK-149). Flag this substitution to Jonathan at the gate.
- HF_TOKEN never echoed/logged/argv'd (env-only law honored).
