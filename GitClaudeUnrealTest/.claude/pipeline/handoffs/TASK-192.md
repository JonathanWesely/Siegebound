# TASK-192 handoff — Stage-0 upgrade: FLUX.1-schnell → FLUX.1-dev (gameplay-programmer)

Date: 2026-07-18 · Status set: ready-for-qa · File touched: `Tools/ArtPipeline/concept_generate.py` (ONLY — TASK-193's `refine_trellis_glb.py` untouched)

## What changed (constants + comments only; zero logic/flow edits)

1. **`MODEL_ID`**: `black-forest-labs/FLUX.1-schnell` → **`black-forest-labs/FLUX.1-dev`**. Swap-note comment block rewritten for dev (provenance, license note, negative-prompt rationale).
2. **`DEFAULT_STEPS`**: `4` → **`40`**. Board-spec window is ~28–50; 40 chosen as the quality-leaning midpoint (detail gains flatten past ~30–40; 50 mostly costs more compute/quota). Per-run override via the existing `--steps` flag is unchanged.
3. **`DEFAULT_GUIDANCE`**: `None` → **`3.5`** (FLUX.1-dev model-card recommended value). The existing `generate_one()` already forwards `guidance_scale` whenever `DEFAULT_GUIDANCE is not None` — no code change needed, the constant now activates that path.
4. **`MODEL_SUPPORTS_NEGATIVE_PROMPT` stays `False`** (deliberate): FLUX.1-dev is guidance-distilled — `guidance_scale` drives an embedded/distilled guidance vector, NOT classifier-free guidance, so dev does not consume negative prompts. Per-entry `negative_prompt` values keep producing the existing informational warn + are not forwarded (same behavior as schnell).
5. Doc-only touch-ups: `--steps` help text (`FLUX.1-dev wants ~28-50`), `build_negative_prompt()` docstring. No other lines changed.

## Explicitly UNCHANGED (spec invariants — QA checklist)

- CLI shape: no flags added/removed/renamed (guidance is a constant, NOT a new flag — spec said "expose as constants").
- Exit-code map 0/2/3/4/5/64 (incl. the argparse 2→64 remap) — untouched.
- `HF_TOKEN` ENV-ONLY secret law, `redact()` scrubbing, `hf_` pattern — untouched.
- `--check` tokenless contract, `--probe`, `--list`, `--force` skip-if-exists, retry/backoff, quota/drift classifiers — untouched.
- PascalCase Inbox output (`Inbox/<CardID>.png`), atomic confined write, prompt source `concept_prompts.json` — untouched.

## Verified locally (free, no quota)

`uv run python concept_generate.py --check` → **exit 0**: deps import OK, InferenceClient signature OK, 16 prompt entries valid, token present (value never echoed), `--check PASSED: tool wires up (model constant: black-forest-labs/FLUX.1-dev)`.

## How to validate (the paid probe = TASK-195, NOT run here per dispatch scoping)

The spec-acceptance dev-model PNG run + schnell-vs-dev comparison lands at TASK-195 (art-director concept probe, blocked on this task's QA pass). For the prober:
- **Do NOT use `_FluxDevProbe` as the scratch id** — the spec's example id starts with `_`, but `CARD_ID_RE` (`^[A-Za-z][A-Za-z0-9]*$`, the Inbox path-safety law, unchanged) rejects a leading underscore, and an underscore-keyed entry in `concept_prompts.json` would fail `--check`/`--list` with exit 5. Use a letter-first scratch id (e.g. `FluxDevProbe`) or back up the target roster PNG before a `--force` run.
- Existing Inbox PNGs are skip-protected without `--force`, so a plain rerun never clobbers accepted concepts.
- A/B on the SAME prompt: schnell arm via `--steps 4` is no longer reproducible in-tree (model constant moved) — the accepted Inbox PNGs ARE the schnell arm; lay the dev probe beside one.

## Flags for QA / orchestrator awareness

- **License/gating (surfaced, not a code issue):** FLUX.1-dev is a GATED HF repo under the "FLUX.1 [dev] Non-Commercial License" (schnell was Apache-2.0). The HF account behind `HF_TOKEN` may need to accept the license once on the model page; a 403/gated error at generation time is that, not drift. `PROVIDER = "auto"` routing may also serve dev via a partner provider under HF PRO credits. Recorded in the in-file comment block. Commercial-use posture is Jonathan's call — flagged, not blocking (the board spec explicitly chose dev).
- Dev at 40 steps costs meaningfully more per image than schnell at 4 — expected and approved (M7.5 quality-first directive); quota exhaustion still exits 3 with resume guidance, unchanged.
