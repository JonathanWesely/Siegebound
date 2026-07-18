# TASK-193 Handoff — Stage-2 albedo de-light/brighten in refine_trellis_glb.py — ready-for-qa

- **From:** gameplay-programmer
- **Date:** 2026-07-18
- **Status:** `ready-for-qa` (tooling QA per M7.5 decision 5; nothing committed — TASK-196 owns the Track-C commit)
- **Files touched:** `Tools/ArtPipeline/refine_trellis_glb.py` ONLY (single file; disjoint from TASK-192's `concept_generate.py` per the parallel-safe ruling). Manifest NOT edited (TASK-194 owns manifest writes — the `albedo_delight` key is READ-only here).

## What was added

The direct counter to the recorded "dark TRELLIS look" (shading baked into albedo; accepted-dark precedents handoffs/TASK-151.md, TASK-172.md): a new stage **8b DELIGHT** in the numpy post-bake area, applied to the baked D **before its PNG write**. Technique = **BOTH spec options combined** (AO-divide AND levels lift), each strength-tunable:

1. **Linear-space AO-divide** — `D_linear + (D_linear / max(AO, ao_floor) − D_linear) × ao_divide_strength`, where AO = the **already-packed ORM.R occlusion** (exactly the spec's source). Physically un-bakes the occlusion shading; the floor caps crevice amplification at ~1/ao_floor; the strength blend keeps some grounding (full divide reads chalky).
2. **Levels/gamma lift** — `gain × x^gamma` in linear space (gamma < 1 brightens midtones/shadows).
3. **Highlight-clip protection** — a C1-continuous rational soft shoulder: identity below `shoulder`, slope-1 at the knee, asymptotic to `max_out` (0.98) — highlights COMPRESS, they never hard-clip; final clip(0,1) is a formality.

### Defaults (conservative-ON in-script, `ALBEDO_DELIGHT_DEFAULTS`)
`enabled: true · ao_divide_strength: 0.6 · ao_floor: 0.35 · gamma: 0.85 · gain: 1.0 · shoulder: 0.80 · max_out: 0.98`

### Manifest override (READ-only)
Optional per-asset/defaults key `albedo_delight` in `pipeline_manifest.json`, resolved through the existing `merged_params` chain then over the in-script defaults. Accepts a **bool** (on/off) or a **partial object** over the keys above. Unknown keys → report WARNING (keys starting `_` are treated as manifest comments and skipped); wrong type → WARNING + defaults **at both the container AND the per-key level** (per-key guarantee true as of the loop-1 fix below — the first submission only guarded the container type): every per-key numeric coercion is try/except-guarded, so `null` / string / list / bool-for-numeric / NaN / Infinity values warn and fall back to that key's default, never raising; `enabled` accepts a strict JSON bool only. All numeric values sanitized to safe ranges (ao_floor ≥ 0.05 also guards divide-by-zero; shoulder is forced below max_out).

## New/changed code (all in refine_trellis_glb.py)

- Header docstring: stage **8b DELIGHT** entry + native-mode paragraph note.
- New section "stage: albedo de-light (TASK-193)": `ALBEDO_DELIGHT_DEFAULTS`, `_srgb_to_linear` / `_linear_to_srgb` (piecewise IEC 61966-2-1), `resolve_delight_config`, `apply_albedo_delight`.
- `main()`: ONE call site — `img_d = apply_albedo_delight(img_d, img_orm, params, report)` at the point where bake and native modes converge, immediately before the D `save_png` and before `build_final_materials` (so the refine_report previews render the LIFTED D — that is what TASK-195's A/B board needs).

## Deliberate design decisions QA should scrutinize

1. **Blender pixel-space handling (the subtle one):** `Image.pixels` on BYTE images returns the raw STORED encoding (no colorspace transform — rna_Image_pixels is a plain byte/255 copy). The D image is byte+sRGB in both modes, so pixels arrive sRGB-ENCODED; the AO/ORM images are byte+Non-Color, so ORM.R arrives raw/linear. The step therefore explicitly decodes D sRGB→linear, does all math in linear (where AO-divide is physically meaningful), and re-encodes on write. A `img_d.is_float` guard skips decode/encode for float-buffer images (pixels already scene-linear). Round-trip is identity (validated to 1.7e-16).
2. **Never mutates the source image:** the delighted result is written into a NEW generated image (`T_<CardID>_D_delight`, colorspace copied from the source) returned to main. Avoids in-place-write pitfalls on the file-backed/packed native-mode D and guarantees the input D + N/ORM are untouched. The saved PNG filename still comes from the existing `save_png` path (`T_<CardID>_D.png`) — the Blender image datablock name is not the file name.
3. **N/ORM byte-untouched:** ORM.R is READ only (`_image_channel(img_orm, 0)`); nothing writes to N/AO/R/M/ORM images. Two-slot split, budgets, UCX, FBX export, previews: no code path changed — the only behavioral delta in the whole script is which image datablock is saved/wired as the D.
4. **Applies in BOTH modes (bake AND native):** the spec anchors on "the baked D", but native-mode D carries the same TRELLIS-baked shading (the defect is upstream of Stage 2), and Meshy retexture donors re-enter through Stage 2 in either mode (CONVENTIONS M7.5 invariant). Native's flat-AO=1.0 fallback degrades the divide to an exact no-op (validated), leaving only the mild levels lift; per-asset `albedo_delight: false` disables entirely. Flagging as a justified scope call, not scope creep.
5. **AO resolution mismatch:** ORM.R is `_resize_nearest`-mapped to the D's actual width/height (no-op in bake mode where all bakes share one resolution; matters for native TRELLIS textures).
6. **Report/provenance:** `report["albedo_delight"]` records the resolved config + `applied`, `source_is_float_buffer`, `mean_linear_before/after`, `p99_linear_after`, `shoulder_compressed_fraction` — lands in `refine_report.json` (feeds the CONVENTIONS M7.5 provenance note and makes TASK-195's "visibly brighter" verdict measurable). One grep-able `DELIGHT:` log line.
7. **OutputGuard confinement:** unchanged — the step writes NO files itself; the D still exits only through the existing guarded `save_png`.
8. **No new deps / CLI:** bpy + numpy + stdlib only; no new flags (spec mandates in-script defaults + manifest read, not a CLI surface).

## Validation performed (file-work scope — no full Blender rerun; that is TASK-195)

- `py_compile` clean on **Blender 5.1's bundled Python 3.13** (the actual runtime) and the ArtPipeline venv Python.
- Numerical core replicated bpy-free and asserted on Blender's numpy (scratchpad `test_delight_math.py`, seed 193): synthetic dark albedo (AO-shaped shading multiplied in) lifted **mean linear 0.166 → 0.280 (+68%)** under defaults; occluded pixels gain **2.89×** vs **1.28×** unoccluded (true de-shadowing, not a flat exposure push); **no output pixel reaches max_out** (max 0.954 < 0.98), shoulder monotonic + C1 at the knee (slopes 1.000/0.997); sRGB round-trip identity; enabled/bool/object/partial-override/unknown-key/bad-type/sanitization paths all pass; flat AO=1 exactly equals the lift-only result.

## QA loop-1 fix (2026-07-18 — qa/TASK-192-193-qa.md, re-submitted ready-for-qa)

Diff scope: `resolve_delight_config` + one line at the delight output-image creation. All findings addressed:

1. **BLOCKER (per-key bad-type crash) FIXED:** the bare `float()` coercions are replaced by a `sanitized_number(key, lo, hi)` helper — `try/except (TypeError, ValueError)` → report WARNING + that key's `ALBEDO_DELIGHT_DEFAULTS` value. QA's exact repros (`{"gamma": null}`, `{"gamma": "bright"}`, `{"ao_floor": [0.35]}`) now warn-and-default instead of exiting 1 post-bake. Added hardening in the same helper: bool-for-numeric rejected (`float(True)` would silently coerce to 1.0) and non-finite rejected (`json.load` admits `NaN`/`Infinity` literals, and NaN survives `min`/`max` clamping). QA's optional early-resolve hardening is moot under warn+default semantics — no config value can fail the run at any point anymore.
2. **WARN (truthiness `enabled`) FIXED:** strict-bool check — `{"enabled": "false"}` (or `1`) now warns + uses the default instead of silently staying ON; real JSON bools (both container-bool and object forms) honored unchanged.
3. **NIT (alpha loss) FIXED:** the delight output image mirrors the SOURCE's channel layout via `Image.depth` (RGBA = 32/64/128 bits/pixel) — a native-mode D carrying alpha keeps it in the written PNG; bake-mode D (RGB byte, depth 24) is bit-identical to the first submission.
4. **Flagged native-mode decision:** unchanged, per QA's RULED ACCEPTABLE.

**Re-validation (same bpy-free harness, Blender 5.1 bundled Python, scratchpad `test_delight_math.py`):** all previous checks still pass (mean lift +68%, no hard clip, C1 knee, sRGB round-trip identity, flat-AO no-op) plus new PASS 8 — the three QA repros + bool/NaN/Infinity each produce exactly one warning, that key's default, zero exceptions, rest of config intact — and PASS 9 — strict-bool `enabled` (string/int → warn + default; all real-bool forms honored). `py_compile` clean.

## For downstream

- **TASK-195 (art):** Stage-2-only rerun on `Cache/Ogre/trellis_raw.glb` picks the step up automatically (conservative-ON). Strength tuning knob order if the lift is too weak/strong: `ao_divide_strength` first, then `gamma`.
- **TASK-194 (art):** per-asset overrides go under `assets.<CardID>.albedo_delight` (bool or partial object; `_`-prefixed comment keys are safe).
- **TASK-196 (build):** commit gated on this task's PASS QA report.
