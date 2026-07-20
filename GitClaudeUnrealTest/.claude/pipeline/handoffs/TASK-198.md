# Handoff — TASK-198 — Meshy client tool: meshy_generate.py (retexture + image-to-3D) (gameplay-programmer)

**Date:** 2026-07-18 · **Status:** ready-for-qa (tooling QA — secret + exit-code review per board spec)
**Live acceptance run:** PASSED end-to-end (Ogre retexture, exit 0, 10 credits). No editor, no Content/ writes, no Git.

## ⚠️ RECORDED SPEC DEVIATION — env var is `MESHY_TOKEN`, not `MESHY_API_KEY`

The board/CONVENTIONS spec named the secret `MESHY_API_KEY`. **Jonathan's actual HKCU user var is `MESHY_TOKEN`**
(set 2026-07-18, same discipline as HF_TOKEN; premium sub active). The actual name is law (TASK-197 board note):
- The tool reads **`MESHY_TOKEN` as canonical** and accepts **`MESHY_API_KEY` as a documented fallback alias**.
- A CONVENTIONS amendment is routed to manager SEPARATELY — CONVENTIONS.md was NOT edited by this task.
- QA: judge the secret law against `MESHY_TOKEN`; the CONVENTIONS §"Meshy second engine" text still says
  MESHY_API_KEY until the manager amendment lands. Not a finding — recorded here.

## What was built

### 1) `Tools/ArtPipeline/meshy_generate.py` (NEW — the only new pipeline file)

Mirrors `trellis_generate.py` in structure, CLI shape, secret-handling, and exit-code contract. Stdlib-only HTTP
(urllib + ssl; no new deps in `pyproject.toml`) against the official API at `https://api.meshy.ai`
(docs.meshy.ai): retexture `/openapi/v1/retexture`, image-to-3D `/openapi/v1/image-to-3d`, balance
`/openapi/v1/balance`. Task create → poll-with-wall-clock-timeout (default 30 min, floor 5, `--poll-seconds` 10)
→ signed-URL GLB download (streamed, `.part` then atomic rename). Transport/5xx/429 retried with 20s→60s→180s
backoff; 400/401/404 and credit-402 never retried. API errors surfaced VERBATIM (task_error.message, HTTP bodies).

**Modes (per CONVENTIONS "Meshy second engine (M7.5)"):**
- `--mode retexture <CardID>` (Stage 1.5): dense donor `Cache/<CardID>/trellis_raw.glb` (else `meshy_raw.glb`;
  `--donor auto|trellis|meshy`) + style ref `Inbox/<CardID>.png`, both uploaded as base64 data URIs (documented
  Meshy shapes) → `Cache/<CardID>/meshy_retex.glb`. Defaults: `enable_original_uv=true` (`--no-preserve-uv` to
  flip), `enable_pbr=true` (`--no-pbr`), `target_formats=["glb"]`, `--ai-model latest`.
- `--mode image3d <CardID>` (Stage-1 alternative): `Inbox/<CardID>.png` → `Cache/<CardID>/meshy_raw.glb`.
  Defaults: `topology=triangle`, `target_polycount=300000` (API max for standard — dense donor; Stage 2 does the
  real budget cut), `should_remesh=true`, `should_texture=true`, `enable_pbr=true`.

**Exit codes (board contract, argparse-2 remapped to 64 exactly like trellis):**
0 ok/`--check` · 1 generic · 2 key unset · 3 credits exhausted (402 or credit-worded task_error; expected pause,
verbatim + guidance, never retried) · 4 API drift (non-JSON/shape/missing-endpoint/missing model_urls.glb) ·
5 input missing OR unreadable (PIL-verified) · 64 usage.

**Secret law:** key resolution order = process env `MESHY_TOKEN` → process env `MESHY_API_KEY` →
**Windows HKCU `Environment` fallback** (winreg, same order) — covers shells spawned before the user var existed
(the HF_TOKEN/HKCU rollback precedent; documented in the module docstring). The value is registered with the
redactor the moment it is resolved; EVERY print (say/warn/fail/usage-error) passes through `redact()`, which
scrubs the live value AND the generic `msy_[A-Za-z0-9]{15,}` shape. Never on argv, never written to any file
(state.json carries no credential material), signed download URLs logged host+path only (query stripped).

**`--check` (documented deviation from trellis's tokenless check):** Meshy has no unauthenticated surface and the
board acceptance pins "--check passes with key / exits 2 without" — so `--check` REQUIRES the key, then hits only
FREE read endpoints: balance (numeric assert = schema check, prints remaining credits) + both task-list endpoints
(reachability/drift probe). Zero credit spend.

**Provenance (MERGE, never clobber):** success MERGES into `Cache/<CardID>/state.json` — the existing TRELLIS
success record is preserved verbatim; adds top-level `engine` (`meshy-retex`/`meshy-i23d`) + a `meshy` block
(task id, endpoint, params, input shas [donor GLB + style/concept PNG], output GLB path/bytes/sha256,
consumed_credits, balance before/after, UTC timestamps). A prior meshy block shelves into `meshy_history`.
Failures write `state_failed.json` ONLY (trellis QA WARN-2 scheme) — a surviving artifact keeps its provenance.

### 2) `.claude/hooks/guard-secrets.sh` (extended — one alternation)

Added `msy_[A-Za-z0-9]{20,}` to the high-confidence token regex. Prefix `msy_` was OBSERVED via a User-scope
prefix-pattern-only read (`[Environment]::GetEnvironmentVariable('MESHY_TOKEN','User')` → length 40, prefix
`msy_` + 36 alnum; the full value was never echoed/logged anywhere).

## Verification evidence (all live, 2026-07-18)

| Test | Result |
|---|---|
| `uv run python -m py_compile meshy_generate.py` | OK |
| `--check` with key ABSENT (in-process sim: env popped + HKCU branch skipped via non-win32 platform — Jonathan's real HKCU var untouchable; driver in session scratchpad `test_exit2.py`) | **exit 2**, setup guidance, no leak |
| `--check` live, bare TLS, key via HKCU fallback | **exit 0** — balance OK (3300 credits), both task-list endpoints OK |
| `--mode bogus` / missing CardID | **exit 64** both (argparse remap) |
| `--mode retexture NoSuchAssetZz` | **exit 5**, names both donor candidates (test dir cleaned) |
| **LIVE RETEXTURE (acceptance)** `--mode retexture Ogre` | **exit 0** end-to-end, ~2 min task wall-clock |
| Secret-leak sweep: `msy_[A-Za-z0-9]{20,}` over state.json, tool, hook, full run log | **no match** (redactor + no-write law held) |

**Live-run artifacts (the TASK-199 A/B arm-C seed):**
- `Tools/ArtPipeline/Cache/Ogre/meshy_retex.glb` — 30,928,280 bytes, sha256 `6fe26ac0…7c28ec`
- Meshy task id `019f76c7-81a7-7af3-9d5d-a4d92ba35333` (SUCCEEDED, progress 100)
- Donor: `trellis_raw.glb` sha256 `b4fa1938…0ee7c4` · style ref `Inbox/Ogre.png` sha256 `323d0204…3374b7`
- Credits: **10 consumed** (balance 3300 → 3290)
- `Cache/Ogre/state.json` merged: `engine=meshy-retex` + full meshy block; TRELLIS record intact (diff = additions only)

## TLS outcome: **BARE — no SSL_CERT_FILE, no Norton exclusion needed**

Both `api.meshy.ai` (API) and `assets.meshy.ai` (signed download) verified bare. Root cause of the difference vs
the HF/certifi failures: stdlib `ssl.create_default_context()` loads the WINDOWS cert stores, which contain the
Norton root — so even if Norton MITMs, verification succeeds. `SSL_CERT_FILE` is still honored by the same call
(TASK-185/195 fallback stays available, documented in-script); verification is never disabled. No 🚨 post needed.

## QA should scrutinize
1. Secret paths: `resolve_api_key()` (HKCU winreg fallback), `redact()` coverage on every output path incl. the
   argparse usage-error override; confirm nothing writes the key (state writers, schema-free — no api_schema.json
   equivalent is written; Meshy is a fixed REST contract, drift is asserted per-response instead).
2. Exit-code mapping in `_run_mode`'s except ladder + `run_check` (402→3, drift→4, FileNotFoundError/InputError→5,
   MeshyHttpError 401→1 with guidance).
3. The `--check` requires-key deviation and the MESHY_TOKEN naming deviation (both documented, both intentional).
4. `write_success_state` merge semantics (trellis record preservation, `meshy_history` shelving, unparsable-state
   fallback to `state_pre_meshy.json`).

## Files touched
- `Tools/ArtPipeline/meshy_generate.py` (new)
- `.claude/hooks/guard-secrets.sh` (one-line regex extension + comment)
- NOT touched: CONVENTIONS.md (amendment is manager's), pyproject.toml (stdlib+PIL only), Content/, any shipped
  asset, `rig_character.py`/`fix_rig_root.py` (pre-existing dirt from another lane, untouched).

## Downstream
- **TASK-199 (art-director):** arm C's donor already exists — `Cache/Ogre/meshy_retex.glb` + provenance above; the
  Stage-2 rebake + 3-arm board can start as soon as this passes QA. Suggested Stage-2 note: donor is ~31 MB with
  Meshy PBR maps embedded; `enable_original_uv=true` kept the TRELLIS UV layout.
- **Manager:** CONVENTIONS "Meshy second engine (M7.5)" secret-law amendment MESHY_API_KEY → MESHY_TOKEN (alias
  documented) — routed separately per dispatch.
