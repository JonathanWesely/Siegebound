# Handoff — TASK-195 — Track-C validation renders: FLUX-dev concept probe + albedo-lift rebake A/B (art-director)

**Date:** 2026-07-18 · **Status:** done (validation evidence only — NO editor, NO import, NO commit; evidence rides TASK-196)
**Runs:** headless only (uv venv + `blender.exe --background`). Shipped game assets verified BIT-IDENTICAL at finish (see §5).

## 1) Part (a) — FLUX.1-dev concept probe: SUCCESS, no license gate

- **The feared 403/gating did NOT occur.** `provider=auto` served `black-forest-labs/FLUX.1-dev` without a license
  prompt; generation succeeded on attempt 1 in 4.6 s. No 🚨 Blockers post needed.
- **Scratch id:** `FluxDevProbe` (letter-first per the QA-corrected note — underscore ids exit 64). Entry was a
  temporary clone of the **Knight** prompt (same prompt text, same seed 71001) added to `concept_prompts.json` for the
  run only; the file was then **restored bit-identically** (sha256 `a793016e…1724` before == after). No accepted Inbox
  PNG touched; no `--force` used anywhere.
- **Output beside the schnell original:** `Tools/ArtPipeline/Inbox/FluxDevProbe.png` (506,485 bytes) sits beside the
  accepted `Inbox/Knight.png`. Durable copies + composite:
  - `Tools/ArtPipeline/Cache/Knight/AB_concept/Knight_schnell_accepted.png`
  - `Tools/ArtPipeline/Cache/Knight/AB_concept/Knight_fluxdev_probe.png`
  - **Pair image:** `Tools/ArtPipeline/Cache/Knight/AB_concept/AB_Knight_schnell_vs_fluxdev.png`
- **TLS note (environment, not code):** first attempt failed `CERTIFICATE_VERIFY_FAILED` — the KNOWN
  `router.huggingface.co` Norton-exclusion gap (memory/norton-tls-interception, TASK-185 precedent). Fixed with the
  proven combined CA bundle via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE` (session scratchpad copy of the TASK-185 bundle,
  Norton root verified present). Durable fix remains Jonathan's Norton exclusion for `router.huggingface.co`.
- **Probe verdict (honest):** dev at 40 steps / guidance 3.5 gives **cleaner, smoother sculpted forms** and a tidier
  single-subject sheet (good TRELLIS input), but **less hand-painted texture richness** and weaker adherence to the
  chunky 2.5-heads proportion than the accepted schnell Knight. NOT an automatic quality upgrade for the house style —
  recommend a steps/guidance sweep (e.g. 28/3.0 vs 40/3.5 vs 50/4.0) or prompt re-weighting of "hand-painted textures,
  chunky proportions" before any dev-model roster re-gen. No roster re-gen performed (out of scope).

## 2) Part (b) — Albedo-lift rebake A/B (Stage-2-only, quota-FREE, both caches survived)

Both reruns used `--smoke` — full-quality bake with FBX/texture writes confined to `Cache/<CardID>/smoke/` by the
OutputGuard itself, so **Content/RawAssets was never opened for write** (belt: hash verification, §5). The TASK-193
delight step ran conservative-ON from the real manifest (no per-asset override exists — TASK-194 deliberately reserved
them for this verdict).

### Ogre (THE recorded-dark asset, TASK-150) — dark vs DEFAULTS vs TUNED
- Logs: `Cache/Ogre/AB_delight/refine_ogre_smoke.log` (defaults, DONE 55.6 s) ·
  `Cache/Ogre/AB_delight/refine_ogre_tuned.log` (tuned, DONE 51.8 s). Invariants held on both: 15000 tris, slots
  `[TeamRegion, OgrePBR]`, `UVMap` ok.
- DELIGHT stats (whole-atlas linear means; atlas includes black UV margin):
  - **defaults** (aoDiv 0.6/floor 0.35, gamma 0.85, gain 1.0): mean 0.0042 → 0.0123, p99 0.128, shoulder 0.0%
  - **tuned** (aoDiv 1.0/floor 0.25, gamma 0.55, gain 1.2): mean 0.004 → 0.059, p99 0.426, **shoulder 0.0% — zero
    highlight clipping even at this strength**
- Tuned run used a **SCRATCH manifest copy** (`Cache/Ogre/AB_delight/scratch_manifest_tuned.json`) via `--manifest` —
  the real `pipeline_manifest.json` untouched by me (its git `M` is TASK-194's pending change).
- **Pair/board images (durable, = arm B of the TASK-199/200 Meshy board):**
  - `Cache/Ogre/AB_delight/AB_Ogre_front_dark_vs_lifted.png`
  - `Cache/Ogre/AB_delight/AB_Ogre_threequarter_dark_vs_lifted.png`
  - `Cache/Ogre/AB_delight/AB_Ogre_beauty_cycles_dark_vs_lifted.png`
  - `Cache/Ogre/AB_delight/AB_Ogre_Dtexture_dark_vs_lifted.png`
  - 3-panel (adds tuned): `AB3_Ogre_front|threequarter|beauty_cycles|Dtexture_dark_defaults_tuned.png` (same folder)
  - Raw arms: `AB_delight/shipped_dark/` (previews + refine_report + T_Ogre_D) · `AB_delight/lifted/` ·
    `AB_delight/tuned/`

### Knight (optional second sample — cache survived)
- Log: `Cache/Knight/AB_delight/refine_knight_smoke.log` (defaults, DONE 84.6 s; 15000 tris, `[TeamRegion, KnightPBR]`).
- DELIGHT defaults: mean 0.006 → 0.015, p99 0.157, shoulder 0.0%. Same visual story as Ogre.
- **Pair images:** `Cache/Knight/AB_delight/AB_Knight_front|threequarter|beauty_cycles|Dtexture_dark_vs_lifted.png`
  (+ `shipped_dark/` and `lifted/` raw arms in the same folder). No tuned Knight run — the Ogre datapoint generalizes;
  final strengths are TASK-200's ruling anyway.

## 3) LIFT VERDICT: **works, but NEEDS STRENGTH TUNING for the recorded-dark donors**

- The TASK-193 step is correct and safe (real texel recovery in the D, zero clipping at every setting tried), but the
  **conservative defaults are visually insufficient** on assets as dark as Ogre/Knight: a 3x mean-linear lift of a
  near-black albedo still reads black on-model. The defaults are fine as fleet-wide defaults for normal assets.
- The **tuned setting is a clear visual win** on Ogre — reads mossy olive-gray with recovered surface variation, no
  blowouts (see `AB3_Ogre_threequarter_dark_defaults_tuned.png`): proven values
  `{"ao_divide_strength": 1.0, "ao_floor": 0.25, "gamma": 0.55, "gain": 1.2}`.
- **Recommendation (for TASK-200's ruling / TASK-194-lane manifest edit AFTER it):** keep in-script defaults as-is;
  add per-asset `albedo_delight` overrides at ~the tuned values for the recorded-dark units (Ogre certainly; Knight
  likely; other dark units per eyeball). Knob order confirmed as documented: `ao_divide_strength` → `gamma`.
- Caveat for the board reader: the Cycles beauty previews under-sell the lift (dim scene lighting dominates); the
  workbench TEXTURE-color previews and the D-texture pairs are the honest albedo evidence. In UE's brighter game
  lighting the lift will read stronger than the beauty renders suggest.

## 4) Canonical Cache state restored

The reruns overwrite `Cache/<CardID>/refine_report.json`, `previews/`, `bake_debug/` in place. After copying the
lifted/tuned arms to `AB_delight/`, I **restored the canonical files from the shipped-dark backups**, so
`Cache/Ogre/` + `Cache/Knight/` canonical provenance again matches the SHIPPED assets (TASK-194's audit trail intact).
Transient `Cache/<CardID>/smoke/` dirs deleted (evidence already in `AB_delight/`). `state.json`,
`trellis_raw.glb`, `teamregion_check/`, `rig/` were never touched by the script (verified by grep + timestamps).

## 5) Shipped-asset integrity — PROVEN bit-identical

`sha256sum -c` against pre-run hashes (`Cache/Ogre/AB_delight/shipped_hashes_before.txt`): **all 11 OK** —
`Content/RawAssets/Ogre.fbx`, `Knight.fbx`, `Textures/Ogre/T_Ogre_D|N|ORM.png`, `Textures/Knight/T_Knight_D|N|ORM.png`,
`Tools/ArtPipeline/concept_prompts.json`, `Inbox/Knight.png`, `Inbox/Ogre.png`. NO editor, NO reimport, NO commit.

## 6) Downstream

- **TASK-196 (build):** last blocker cleared — Track-C commit can proceed (scripts + manifest + this evidence
  referenced by path; Cache/ and Inbox/ are untracked evidence, cite paths in the message rather than committing).
- **TASK-199 (Meshy A/B):** arm B = `Cache/Ogre/AB_delight/` renders above (durable). Arm A = `shipped_dark/` copies.
- **TASK-200 (Jonathan):** the strength ruling belongs here; recommended override values in §3.
- **TASK-170/171 (Wall/DeepMine Stage-2):** when they finally bake, consider the tuned-strength lesson if their
  Stage-1 output comes out dark; defaults may be fine if the concepts bake bright.
