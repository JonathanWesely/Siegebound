# TASK-184 — Concept-gen tool: `concept_generate.py` (Stage 0 text→image) — handoff

**Status:** ready-for-qa
**Assignee:** gameplay-programmer
**Scope:** headless Python tooling only. No engine, no compile, no Git. Mirrors `trellis_generate.py`.

## What I built

A text→image concept-art generator that turns a per-CardID art-direction prompt into
`Tools/ArtPipeline/Inbox/<CardID>.png` (the exact filename the TRELLIS Stage-1 client
consumes). It is PROMPT-AGNOSTIC — no card design is hardcoded; all art direction lives
in `concept_prompts.json` (art-director authors the 16 M7 prompts in TASK-185).

## Files touched

- **NEW `Tools/ArtPipeline/concept_generate.py`** — the tool (the deliverable).
- **NEW `Tools/ArtPipeline/concept_prompts.json`** — schema + `_doc` + 2 example entries
  (Knight = unit, Wall = building) proving the format for both categories.
- **`Tools/ArtPipeline/pyproject.toml`** — added `huggingface_hub` to `dependencies`
  (I import `InferenceClient`/`model_info` directly; it was already a transitive dep of
  `gradio_client`, so it was already installed + locked).
- **`Tools/ArtPipeline/uv.lock`** — re-locked by `uv`; diff is ONLY the `huggingface-hub`
  direct-dependency marker (no package added, no version churn — verified).
- Board: `TASK-184` status backlog → ready-for-qa (surgical anchored edit).

I did NOT touch `pipeline_manifest.json`, `rig_character.py`, or `rig_manifest.json` —
they showed as pre-existing working-tree changes from other M7 work before I started.

## Model / endpoint (swappable) — for QA + art-director

- **Constant `MODEL_ID = "black-forest-labs/FLUX.1-schnell"`**, driven via the HF
  Inference API (`huggingface_hub.InferenceClient.text_to_image`) with the existing HF
  PRO token. FLUX.1-schnell: fast, Apache-2.0, strong single-subject stylized output.
- **To swap the endpoint** (all top-of-file constants, no other code changes):
  `MODEL_ID` (repo id), `PROVIDER` (default `"auto"` routing; can pin `hf-inference`,
  `fal-ai`, etc.), `MODEL_SUPPORTS_NEGATIVE_PROMPT` (flip **True** for a CFG model like
  SDXL — schnell is guidance-distilled and IGNORES negatives, so we do NOT forward them
  to it), `DEFAULT_STEPS` / `DEFAULT_GUIDANCE` (schnell ~4 steps / no CFG).
- Chose the **HF Inference API over a gradio_client Space** deliberately: a single atomic
  `text_to_image` call (no `gr.State` session coupling, no ZeroGPU Space-sleep/quota
  surprises) is more reliable for headless single-subject generation. Secret-handling +
  exit-code discipline still mirror `trellis_generate.py` 1:1.

## `concept_prompts.json` schema

Top level `{ "_doc": {...}, "version": 1, "prompts": { <CardID>: <entry> } }` (mirrors
`pipeline_manifest.json`'s `_doc`/`version`/`assets` shape). Each entry:
- `prompt` — **required**, non-empty string, the SUBJECT description only.
- `negative_prompt` — optional string (forwarded only when the active model supports CFG).
- `seed` — optional int (CLI `--seed` overrides).

**Shared framing suffix (baked in, per spec):** authors write only the subject; the tool
APPENDS `PROMPT_SUFFIX` (single centered subject, full body, orthographic front
three-quarter, plain flat neutral grey background, even lighting, no ground shadow, clean
silhouette) to every prompt — so TRELLIS image-to-3D gets a clean single-subject reference.

## CLI

`concept_generate.py <CardID>` · `--all` (every CardID, through ONE client) · `--check`
(tokenless smoke) · `--probe` (with `--check`: tokenless public `model_info` reachability,
informational only) · `--list` · `--force` (overwrite; default SKIPS an existing
`Inbox/<CardID>.png` so Jonathan's manual drops/edits are never clobbered) · `--seed N` ·
`--width/--height/--steps/--timeout-minutes/--attempts`.

## Exit codes (identical to `trellis_generate.py`)

`0` ok/`--check` · `1` generic failure (after retries) · `2` HF_TOKEN unset · `3`
quota/rate-limit · `4` API/model drift · `5` prompts missing/malformed/CardID-not-found ·
`64` CLI usage (argparse's default 2 remapped so 2 uniquely = token unset).
- `--all` aggregation: token checked once up-front; a `3` (quota) or `4` (drift) STOPS the
  batch immediately (resume-friendly — finished PNGs are skipped without `--force`); a
  per-card `1`/`5` is remembered and the loop continues.

## Secret-handling (the critical mirror — please scrutinize)

- `HF_TOKEN` is **ENV-ONLY**: read via `os.environ.get("HF_TOKEN")` at runtime; never
  read from a file, never accepted on argv, never written to any file.
- Every printed line goes through `redact()` → `say/warn/fail`, which scrubs both the
  live registered token value AND the generic `hf_[A-Za-z0-9]{15,}` shape (same regex as
  the guard-secrets hook). The argparse error path also redacts, so a token pasted on the
  command line by mistake is never echoed.
- Generation exceptions are `redact()`-ed before printing (a hostile provider error can't
  leak the token).

## Safety (Inbox writes)

- CardID is regex-validated (`^[A-Za-z][A-Za-z0-9]*$`) — no path separators / traversal.
- `atomic_write_png` writes to `Inbox/.<CardID>.png.tmp-<pid>` then `os.replace` (atomic),
  and REFUSES any destination whose parent isn't the resolved `Inbox/` dir (confinement
  guard, defence-in-depth on top of the regex).
- Default is skip-if-present; only `--force` overwrites.

## Verification I ran (no live image API — only `--check` per the task)

- `uv run concept_generate.py --check` → **EXIT 0**; reports deps OK, InferenceClient
  signature OK, `concept_prompts.json` OK (2 entries: Knight, Wall), HF_TOKEN present.
- Exit-code paths proven without any GPU/image call:
  - `--list` → 0
  - no args / `CardID + --all` / bad CardID `../evil` → **64** (usage, with redaction)
  - unknown CardID `Nonexistent` → **5**
  - `HF_TOKEN=` (unset) `Knight` → **2** (with setup guidance, no token echoed)
- Internal unit checks (via `python -c`): redactor scrubs live token + generic `hf_`
  shape; quota vs drift classifiers; prompt-suffix assembly; negatives gated OFF for
  schnell; Inbox confinement guard fires on an out-of-Inbox path; `inbox_path_for`
  maps CardID → `Inbox/<CardID>.png`.
- Confirmed `Inbox/` has NO stray `.tmp`/new files after the run; removed the generated
  `__pycache__` (not gitignored).

## What QA should scrutinize

1. **Secret-handling parity** — the ENV-only read, the `redact()` coverage on ALL output
   paths (say/warn/fail + argparse error + exception messages), and that no code path can
   write/echo the token. This is the load-bearing mirror of `trellis_generate.py`.
2. **Exit-code map** — matches the trellis contract exactly (esp. 2 = token-unset uniquely,
   64 remap; 3 quota not retried; 4 drift not retried; 5 prompts).
3. **Inbox write safety** — CardID regex + confinement guard + atomic temp-then-rename +
   skip-without-`--force` (never clobber a manual drop).
4. **Model-swap seam** — MODEL_ID/PROVIDER/negative-prompt-gate/steps are top-of-file
   constants so the endpoint is swappable without touching logic (spec requirement).
5. **`--all` aggregation + resume** — quota/drift stop the batch; skip-if-present makes a
   resume idempotent.
6. Note: `--check` is tokenless + OFFLINE by design (deps/signature/schema/token-presence,
   no GPU call) per the ruling; the live tokenless reachability is opt-in via `--probe`
   and is informational only (never flips the exit). The REAL image generation is
   art-director's TASK-185.

## Downstream (TASK-185, art-director)

Author the 16 M7 prompts in `concept_prompts.json` (replace/extend the 2 examples) and run
`concept_generate.py <CardID>` / `--all` → `Inbox/<CardID>.png` × 16. Surface any non-zero
exit verbatim (2 token · 3 quota-pause/resume · 4 drift → fix MODEL_ID · 5 prompts).
