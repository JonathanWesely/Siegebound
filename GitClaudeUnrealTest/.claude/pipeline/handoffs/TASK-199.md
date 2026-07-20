# Handoff — TASK-199 — Meshy A/B retexture run: Ogre through Stage-1.5 + Stage-2 → 3-arm decision board (art-director)

**Date:** 2026-07-18 · **Status:** done (evidence only — NO editor, NO import, NO commit, ZERO Meshy credits spent by this task)
**Runs:** headless only (uv venv + `blender.exe --background`). Shipped assets verified BIT-IDENTICAL at finish (§6).

## 1) Arm C — Stage-2 on the EXISTING Meshy-retextured donor (no regeneration)

- Donor: `Tools/ArtPipeline/Cache/Ogre/meshy_retex.glb` (TASK-198's live acceptance artifact, 30,928,280 bytes) —
  sha256 re-verified against `state.json` provenance before the run: `6fe26ac0…7c28ec` MATCH. **No credits spent.**
- Run: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id
  Ogre --input Cache/Ogre/meshy_retex.glb --smoke` — real `pipeline_manifest.json` (same params as the shipped Ogre;
  no `--manifest` override, no per-asset `albedo_delight` key exists), so the TASK-193 delight applied at its
  **in-script defaults**: `ao_divide_strength 0.6, ao_floor 0.35, gamma 0.85, gain 1.0, shoulder 0.8, max_out 0.98`
  (recorded in the arm-C refine_report). Exit 0, DONE 59.4 s.
  Log: `Cache/Ogre/AB_meshy/refine_ogre_meshy_smoke.log`.
- **INVARIANT HELD on arm C** (report `Cache/Ogre/AB_meshy/meshy/refine_report.json`): 15000 tris (budget 15000),
  slots exactly `[TeamRegion, OgrePBR]`, UV layer `UVMap` ok, feet-center origin, bounds [221.3, 227.6, 288.2] UE —
  same warn-only Y-width deviation as EVERY arm (club overhang; fit_mode height matches Z 290 exactly).
- Cleanup parity with the TRELLIS donor run: 454,834→221,395 verts (TRELLIS arm: 460,425→221,395), 182 islands,
  5 holes — Meshy's `enable_original_uv=true` preserved the geometry; only the texture source changed, as designed.

## 2) TeamRegion outcome on arm C — **LAW-CRITICAL: the two-slot split SURVIVED**

**351 faces / 2.1% area** (shipped + arm B: 353 / 2.1% — a 2-face jitter from remesh/decimate nondeterminism on the
near-identical cleaned mesh; area fraction identical, cap 35%). Triple-proven: report numbers, the white shoulder-cap
patch in the workbench TEXTURE previews, and the blue TeamRegion band rendering on the pauldron in the Cycles beauty —
same spot as arms A/B. The selector is GEOMETRY-driven (Stage-2 face-set boxes), so it is texture-source-agnostic:
**fleet rollout is safe on this axis for any Meshy-retextured asset whose selectors were already verified.**

## 3) The 3-arm decision board (durable Cache paths — 4 columns: A | B1 | B2 | C)

Same camera/lighting per row (identical script + manifest = identical preview rig; like-for-like guaranteed):

- `Tools/ArtPipeline/Cache/Ogre/AB_meshy/BOARD_Ogre_front_dark_lift_tuned_meshy.png`
- `Tools/ArtPipeline/Cache/Ogre/AB_meshy/BOARD_Ogre_threequarter_dark_lift_tuned_meshy.png`
- `Tools/ArtPipeline/Cache/Ogre/AB_meshy/BOARD_Ogre_beauty_cycles_dark_lift_tuned_meshy.png`
- `Tools/ArtPipeline/Cache/Ogre/AB_meshy/BOARD_Ogre_Dtexture_dark_lift_tuned_meshy.png`

Columns: **A** shipped dark · **B1** TASK-193 lift defaults · **B2** lift TUNED (TASK-195's win:
aoDiv 1.0/floor 0.25/γ0.55/gain 1.2) · **C** Meshy retexture + lift defaults. Raw arm C (D/N/ORM + previews + report):
`Cache/Ogre/AB_meshy/meshy/`. Arms A/B raw: `Cache/Ogre/AB_delight/{shipped_dark,lifted,tuned}/` (TASK-195).
This board + TASK-195's Ogre/Knight pair images = the complete TASK-200 gate package.

## 4) Objective stats (whole-atlas linear albedo means; atlas includes black UV margin)

| Arm | D source | Delight config | mean before → after | p99 after | shoulder clip |
|---|---|---|---|---|---|
| A shipped dark | TRELLIS bake | none (pre-TASK-193) | 0.0042 (as-is) | — | — |
| B1 lift defaults | TRELLIS bake | aoDiv 0.6 / γ0.85 / gain 1.0 | 0.0042 → 0.0123 | 0.128 | 0.0% |
| B2 lift tuned | TRELLIS bake | aoDiv 1.0 / γ0.55 / gain 1.2 | 0.0042 → 0.0586 | 0.426 | 0.0% |
| C Meshy + defaults | **Meshy retex bake** | aoDiv 0.6 / γ0.85 / gain 1.0 | **0.0146** → 0.0376 | 0.325 | 0.01% |

The load-bearing number: arm C's **pre-lift** mean is 0.0146 — the Meshy source albedo is ~3.5x brighter than the
TRELLIS bake before any lift touches it. C fixes the disease; B compensates for the symptom.

## 5) VERDICT (my honest ranking): **C > B2 > B1 > A**

- **C wins on the axis a levels lift cannot reach: CHROMA.** The D-texture row is decisive — C carries real color
  content (green skin, warm flesh-tone hands/face, red-brown hide, differentiated straps) where B2, though bright,
  stays a desaturated monochrome olive-gray: you cannot gamma-lift color that was never baked. On-model (front /
  three-quarter) C reads like the CONCEPT — material identity, painted variation — while B2 reads like a
  well-exposed photo of a gray statue. C at lift-DEFAULTS already out-reads B2 at tuned strength.
- **B2 remains a genuinely good FREE result** and the proof the delight step works; it is the right tool where 10
  credits aren't warranted.
- **Hybrid recommendation (fleet formula): Meshy retexture + delight BOTH on, at defaults** — C's pre-lift 0.0146
  means the tuned strength would likely over-lift a Meshy source; defaults land it right (0.0376, p99 0.325, ~zero
  clipping). Reserve the TUNED strengths for TRELLIS-sourced dark assets that DON'T get retextured (per-asset
  `albedo_delight` manifest override, the TASK-195 §3 recommendation — unchanged). Objective trigger worth adopting:
  a bake whose pre-lift mean < ~0.01 is a retexture candidate; 0.01+ can live on lift alone.
- **Where each arm fits per asset class:** units + hero-adjacent showcase assets (the 11 units — always on screen,
  team-colored, animated) = arm C treatment; large background buildings already accepted at the eyeball gates
  (Castle, towers, Barracks) = arm B2/override lift free-fix first, retexture only where the lift verdict fails the
  eyeball; GoldNode (emissive variant) + anything FAB-replaced later = no spend.
- Caveat carried from TASK-195: Cycles beauty previews under-sell every arm (dim scene); the workbench TEXTURE rows
  and D-texture row are the honest albedo evidence, and UE's brighter lighting will read stronger.

## 6) Fleet-cost math (balance LIVE-verified via free `--check` after the run: **3273 credits**)

10 credits/asset observed (Ogre retexture, TASK-198). All 20 pipeline meshes = 200 credits (6.1% of balance) ·
**units only (11) = 110 (3.4%)** · buildings only (9) = 90 (2.7%). Even the full-fleet case is cheap against a 3273
balance; the recommended units-first wave is ~3%. Note: balance moved 3290→3273 (17 credits) OUTSIDE this task
(this task spent zero; TASK-198's run ended at 3290) — presumably the animation-spike lane or account-side usage;
flagging for the TASK-200 ledger, not a blocker.

## 7) Shipped-asset integrity — PROVEN bit-identical

`sha256sum -c Cache/Ogre/AB_meshy/shipped_hashes_before.txt` → **all 6 OK** after the full run + restore:
`Content/RawAssets/Ogre.fbx`, `Textures/Ogre/T_Ogre_D|N|ORM.png`, `Cache/Ogre/meshy_retex.glb`, `Inbox/Ogre.png`.
`--smoke` confined FBX/textures to `Cache/Ogre/smoke/` (deleted after archiving); canonical
`Cache/Ogre/refine_report.json`, `previews/`, `bake_debug/` restored from `AB_meshy/_canonical_backup/` so canonical
Cache provenance again matches the SHIPPED asset (TASK-194 audit trail intact). `state.json`, `trellis_raw.glb`,
`teamregion_check/` untouched.

## 8) Downstream

- **TASK-200 (Jonathan):** gate package complete — the 4 boards in §3 + TASK-195's pairs. My recommendation for the
  ruling: units-first Meshy retexture wave (110 credits) with delight defaults; B2-strength overrides for
  non-retextured dark assets; engine-per-class + Wall/DeepMine routing per his call. The 📢 Planning & Feedback gate
  ask is the orchestrator/manager's post per SLACK.md (main-chat law).
- **TASK-201 (fleet):** if GO — donors already cached for all 20 (`Cache/<CardID>/trellis_raw.glb`), so the wave is
  Stage-1.5 (10 cr/asset) + a free Stage-2 rebake each; TeamRegion selectors need no rework (§2).
