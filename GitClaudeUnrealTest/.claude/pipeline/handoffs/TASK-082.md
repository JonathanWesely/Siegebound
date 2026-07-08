# TASK-082 Handoff — Tools/ArtPipeline scaffold + trellis_generate.py (Stage 1)

- **From:** gameplay-programmer
- **Date:** 2026-07-07
- **Status:** ready-for-qa (board status update is the orchestrator's per dispatch)
- **Law:** CONVENTIONS.md "Textured mesh law (TRELLIS.2 art pipeline)" — tooling law; manager rulings 1/2/8 for TASK-082..088.

## File inventory (all new; no existing files modified)

| File | What |
|---|---|
| `Tools/ArtPipeline/pyproject.toml` | uv project, `requires-python ">=3.12,<3.13"`, deps `gradio_client` + `pillow`, `[tool.uv] package = false` (script-only) |
| `Tools/ArtPipeline/.python-version` | `3.12` |
| `Tools/ArtPipeline/uv.lock` | locked resolution (`requires-python "==3.12.*"`; gradio-client 2.5.0, pillow 12.3.0, httpx 0.28.1, huggingface-hub 1.22.0, 18 pkgs) |
| `Tools/ArtPipeline/trellis_generate.py` | Stage-1 CLI (641 lines) — details below |
| `Tools/ArtPipeline/README.md` | 3 stage commands, HF_TOKEN setup, concept-image guidance, manual-browser fallback, exit codes, dir layout |
| `Tools/ArtPipeline/Inbox/.gitkeep`, `Cache/.gitkeep` | working dirs (script also creates them on demand); .gitignore entries are TASK-084's |

Local, uncommitted byproduct: `Tools/ArtPipeline/.venv/` (created by `uv sync`; TASK-084's .gitignore should cover it — standard uv venv dir).

## Verified (allowed scope only — no network smoke, that is TASK-084's)

- `uv lock` + `uv sync` resolved on **managed CPython 3.12.13** (uv downloaded it; system Python 3.14.4 untouched).
- `uv run python trellis_generate.py --help` → OK.
- `py_compile` clean; file is pure ASCII (console-codepage safe).
- No-arg usage error → **exit 64**; `env -u HF_TOKEN ... Footman` → **exit 2** with setup instructions, exiting BEFORE any network call or dir creation, no token anywhere.
- **`--check` NOT run** — deliberate: the board spec says network smoke tests belong to TASK-084 (build-master), and my dispatch authorized only env creation + `--help`.

## trellis_generate.py behavior (spec mapping)

- **Input** `Inbox/<AssetName>.png` (Pillow-verified; warns on non-square/<512px). **Outputs** `Cache/<AssetName>/trellis_raw.glb` + `state.json` (seed, params, concept sha256, timestamps, gradio_client version, status) + `api_schema.json`.
- **Atomic session:** ONE `Client(SPACE, hf_token=...)` for preprocess → image_to_3d → extract_glb (gr.State is session-coupled; commented in code). Retries reuse the SAME Client, so session state survives per-call retries.
- **view_api() discovery:** hard assert of `/preprocess_image`, `/image_to_3d`, `/extract_glb`; full schema snapshot written beside the output every run and on `--check`.
- **`--check`:** tokenless `Client(SPACE_ID)` + view_api + endpoint assert + snapshot (to `Cache/api_schema.json` when no asset arg). No GPU call, no quota spend.
- **Exit codes:** 0 success · 1 generic failure · 2 HF_TOKEN unset · 3 quota · 4 API drift · 5 concept missing/unreadable · 64 CLI usage error.

## Flagged decisions for QA (explicit)

1. **Secret handling (audit these paths):**
   - Token read ONLY via `os.environ.get("HF_TOKEN")` inside `run_generate()`; argparse defines NO token argument; token is passed solely to `Client(..., hf_token=token)`.
   - **Global redactor:** every output path goes through `say()/warn()/fail()` → `redact()`, which scrubs (a) the registered live token value and (b) the generic `hf_[A-Za-z0-9]{15,}` shape (deliberately broader than the guard-secrets `hf_{20,}` pattern). Exception text is redacted before print AND before storage in `state.json`.
   - Files written are `state.json` (enumerated fields; error/result-shape strings pass through `redact()` first) and `api_schema.json` (public `view_api()` metadata only). Neither can carry the token.
   - `--check` constructs the Client with no token at all.
2. **Exit-code 64 remap (minor deviation):** argparse's default usage-error exit is 2, which would collide with the board-mandated "2 = HF_TOKEN unset". `_Parser.error()` remaps usage errors to 64 so 2 is unique. Documented in docstring/help/README.
3. **Timeouts:** default 30 min per endpoint call, hard floor 20 min (values below are clamped with a warning — the ≥20-min ruling can't be bypassed). Enforcement = `client.submit()` + `job.result(timeout=...)` (gradio_client has no reliable per-request timeout for queued GPU jobs; Job.result is the enforcement point), best-effort `job.cancel()` on failure.
4. **Retries:** 3 attempts per endpoint, exponential backoff 20 s → 60 s → 180 s. **Quota errors are never retried** — detected by heuristic (`"quota"`+`"gpu"` or `"zerogpu"` in the error text, case-insensitive), surfaced VERBATIM (redacted) with ruling-8 guidance (reset time is in the message; free tier ≈5 GPU-min/day; HF PRO before M7; escalate >48 h), exit 3.
5. **API-drift behavior (two tiers):** missing endpoint = hard `ApiDriftError` → exit 4 + pointer to the manual-browser fallback; missing PARAMETER (seed/resolution/decimation_target/texture_size checked against the live schema) = loud warning, Space default applies, requested value still recorded in `state.json`. Image inputs are bound by schema inspection (first param whose component/python_type mentions image/file/filepath), never blind positional binding; `/image_to_3d` with no image-like param is treated as session-state-fed (logged).
6. **GLB retrieval:** recursive walk of the extract result for `*.glb`, takes the LAST (Space convention: viewer model + download file), copies to `trellis_raw.glb`. No GLB → exit 1 with the result shape recorded in `state.json`.
7. **state.json is written on failure too** (status: success / quota-blocked / api-drift / failed + redacted error) — resume evidence for ruling 8 and debugging.
8. **Lazy imports:** `gradio_client`/`PIL` import inside functions so `--help` works even pre-sync and stays fast.
9. **`resolution` is passed as a string** (default `"1024"`) per the board spec (the Space expects the choice as a string).
10. **`.gitkeep` note for TASK-084:** if `Inbox/`+`Cache/` get blanket-ignored, the `.gitkeep` files will be excluded unless negated — TASK-084 adjudicates (dir creation was my spec; ignore entries are theirs). The script also `mkdir`s both on demand, so the .gitkeeps are a convenience, not load-bearing.
11. **Write confinement:** all writes are under `SCRIPT_DIR/Inbox` + `SCRIPT_DIR/Cache` (paths derived from `__file__`); nothing touches Content/, Source/, or the card-art lane (ruling 4).

## For downstream

- **TASK-084 (build-master):** `uv sync` then `uv run trellis_generate.py --check` — expect exit 0, three endpoints listed, snapshot at `Cache/api_schema.json`. Exit 4 = API drift → append to qa/TASK-082-report.md and route back.
- **TASK-085 (Jonathan):** README "One-time setup" + "Concept image guidance" sections are written for him.
- **TASK-083 (Stage 2):** consumes `Cache/<AssetName>/trellis_raw.glb`; `state.json` may be absent on a manual-browser-fallback delivery.

## Post-QA hardening (2026-07-07, per qa/TASK-082-report.md WARN-1/WARN-2)

QA verdict was PASS (0 blockers); the orchestrator asked for both WARNs closed before the TASK-084 commit. Two fixes applied to `trellis_generate.py`, nothing else:

1. **WARN-1 (security) — usage-error echo now redacted.** `_Parser.error()` previously exited with argparse's raw message, which mirrors argv back on unrecognized-argument/invalid-value errors — a token accidentally pasted on the command line would have been echoed unredacted. Now: `self.exit(64, f"{self.prog}: error: {redact(message)}\n")`. The generic `hf_[A-Za-z0-9]{15,}` pattern in `redact()` works pre-registration (no live token needed). **Verified live:** ran the script with a fake `hf_`+25-char argv (built via shell expansion, no token shape in the command text) → stderr shows `unrecognized arguments: [hf-token-redacted]`, exit 64. Side effect: the em-dash QA flagged as NIT-6 sat on the rewritten line — the file is pure ASCII again (grep-verified 0 non-ASCII).
2. **WARN-2 (provenance) — failure never clobbers a success record.** `write_state()` gained a `filename` parameter (default `"state.json"`); all three failure handlers (quota-blocked / api-drift / failed) now write **`state_failed.json`**, and only the success path writes `state.json`. Scheme: `state.json` always describes the LAST SUCCESSFUL run — the provenance of the `trellis_raw.glb` beside it — so a failed seed re-roll (TASK-086 flow) can no longer overwrite a good run's record while its GLB survives. Successive failures overwrite `state_failed.json` (latest failure evidence); a stale `state_failed.json` beside a newer success is history, not an error. One error-message string that always routes to a failure handler ("/extract_glb returned no .glb file path") was corrected to name `state_failed.json`. **README unchanged** — Stage 2 reads only the GLB, so nothing downstream reads differently (the coordinator's README condition was not met). Call-site audit: `write_state` at line 526 (success, `state.json`) and 538/545/552 (failures, `state_failed.json`).

**Regression re-verification:** `py_compile` clean; `uv run python trellis_generate.py --help` OK; `env -u HF_TOKEN ... Footman` → exit 2 (offline, before any network/dir activity); usage error → exit 64 with redacted echo. NIT-3/4/5/7 deliberately NOT picked up (out of the "exactly these two fixes" scope; NIT-5's version bounds earmarked for M7 batch time per QA).
