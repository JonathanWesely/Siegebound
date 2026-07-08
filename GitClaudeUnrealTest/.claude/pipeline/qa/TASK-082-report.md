# QA Report — TASK-082
Verdict: **PASS**

Reviewed: `Tools/ArtPipeline/pyproject.toml`, `.python-version`, `uv.lock` (existence/sanity), `trellis_generate.py` (641 lines, full read), `README.md`, `Inbox/.gitkeep`, `Cache/.gitkeep`, handoffs/TASK-082.md, TASKBOARD TASK-082 block + TRELLIS.2 rulings, CONVENTIONS "Textured mesh law" tooling clauses.
Counts: **0 BLOCKER / 2 WARN / 5 NIT**. Security-sensitive review per ruling 2 (Tools/**/*.py is CODE) — checklist item 7 applied in full (secret handling, timeouts, write confinement, error paths; headless-bpy N/A, no bpy here).

## Security audit — HF_TOKEN leakage paths (highest priority)

Every path traced. The env-only law holds:

- **Acquisition:** `os.environ.get("HF_TOKEN")` inside `run_generate()` only (line 374); argparse defines NO token argument; token flows solely into `Client(SPACE_ID, hf_token=token)` (line 431). `--check` never reads the env var at all — genuinely tokenless (`Client(SPACE_ID)`, line 351).
- **Redactor:** token registered at line 383 BEFORE any network activity; `redact()` scrubs (a) the exact live value (works even for non-`hf_`-shaped tokens) and (b) generic `hf_[A-Za-z0-9]{15,}` (broader than guard-secrets' `{20,}` — good). All of `say()/warn()/fail()` redact.
- **Exception paths:** all three handlers in `run_generate` (quota / drift / generic, lines 522–542) redact before print AND before `state["error"]` storage; `call_endpoint` redacts exception text at line 265 before it is reused (quota heuristic, retry warns, final RuntimeError). Uncatchable escapes (KeyboardInterrupt, ImportError pre-sync, OSError in `write_state`) print source lines only, never variable values — no token in those tracebacks.
- **Files:** `state.json` fields are enumerated (asset/space/timestamps/concept-sha/params/versions/status); all result-shape and error strings pass `redact()` before storage. `api_schema.json` is `view_api()` public metadata only — cannot carry client credential material (see NIT-3 for defense-in-depth).
- **No argv/env echo** anywhere; the "Connecting … authenticated via HF_TOKEN from env" line names the variable, never the value.

Verdict: token cannot reach disk, files, or logs through any path the script controls. One mirror-back gap on user-supplied argv → WARN-1.

## Findings

- **[WARN-1]** `trellis_generate.py:554-556` — `_Parser.error()` bypasses the redactor: `self.exit(64, f"{self.prog}: error: {message}\n")` echoes argparse's message, which embeds raw argv on unrecognized-argument/invalid-value errors. If a human accidentally pastes the token on the command line (`trellis_generate.py Footman hf_XXXX…`), the mistake is mirrored back unredacted to stderr (and into any Bash transcript). Marginal added exposure (the command line itself is already in history/transcript), but it breaks the handoff's "every output path goes through the redactor" claim. **Fix (one line):** `self.exit(64, f"{self.prog}: error: {redact(message)}\n")` — the generic `hf_` regex works pre-registration. Non-blocking.
- **[WARN-2]** `trellis_generate.py:522-542` vs `510-517` — failure-path `state.json` provenance clobber: `trellis_raw.glb` is only written on success (a failed run never destroys a prior good GLB — verified), but a failed re-roll OVERWRITES the success-run's `state.json`, leaving `status: failed` + the failed run's seed/params sitting beside a still-valid GLB from the earlier run. TASK-086's "reroll seed before refining" flow makes this a real path: Stage 2 / auditors can misattribute the GLB's generation params. **Fix suggestion:** on non-success, record `"prior_glb_present": dest_glb.exists()` (or preserve the previous success block under a `previous` key). Non-blocking — GLB data itself is never lost.
- **[NIT-3]** `trellis_generate.py:157-166` — `snapshot_schema` writes the serialized `view_api()` dict without a `redact()` pass. Provably token-free today (public endpoint metadata), but `redact(json.dumps(...))` is free defense-in-depth for the one file written every run including `--check`.
- **[NIT-4]** `trellis_generate.py:431,141` — `Client()` construction and `view_api()` rely on library-default HTTP timeouts. The ≥20-min ruling targets GPU calls and IS enforced at the right point (`job.result(timeout=…)`, line 261, floor-clamped at main():625-628 — clamps 0/negatives too); the metadata calls are bounded by httpx defaults, so no infinite-hang risk. Acceptable; noted for the TASK-084 smoke.
- **[NIT-5]** `pyproject.toml:9-12` — deps carry no version bounds (`gradio_client`, `pillow` bare). `uv.lock` governs (gradio-client 2.5.0, pillow 12.3.0 — both used; pillow IS exercised by `validate_concept`, so no dead dep), but a future `uv lock --upgrade` could jump gradio_client majors silently. Consider `>=2.5,<3` at M7 batch time.
- **[NIT-6]** `trellis_generate.py:554` — one em-dash in the `# noqa` comment; the handoff's "pure ASCII" claim is not literally true. Harmless — it is a comment, never printed; console-codepage safety of PRINTED strings verified separately (all say/warn/fail/help text is ASCII).
- **[NIT-7]** `trellis_generate.py:385-387` — the asset arg is not validated against CardID casing; on case-insensitive NTFS `footman` resolves `Inbox/Footman.png` but writes `Cache/footman/`, which a case-sensitive Stage-2 manifest lookup could miss. Low; the README documents exact casing.

## Per-flag rulings (handoffs/TASK-082.md flags 1–11)

1. **Secret handling — VERIFIED** (audit above), with the WARN-1 gap on the usage-error echo path and NIT-3 defense-in-depth suggestion. Core contract intact.
2. **Exit-64 remap — APPROVED.** Sound deviation: argparse's default exit 2 would collide with the board-mandated "2 = HF_TOKEN unset". Remap makes 2 unique; documented consistently in docstring (lines 22-30), epilog, and README (lines 68-70). Code paths match the contract exactly: 0/1/2/3/4/5/64 all verified against their return sites.
3. **Timeout floor — APPROVED.** Default 30 min ≥ ruling; sub-20 values (incl. 0/negative) clamp with a warning — the ruling cannot be bypassed. `client.submit()` + `job.result(timeout)` is the correct enforcement point for queued ZeroGPU jobs; best-effort `job.cancel()` on failure present.
4. **Retries/quota — APPROVED.** 3 attempts, backoff 20s→60s (→180s at higher --attempts); quota errors detected on redacted text, NEVER retried, raised with verbatim (redacted) Space message + full ruling-8 guidance block, exit 3. Heuristic false negative degrades to exit 1 after retries — acceptable.
5. **Two-tier API drift — APPROVED.** Missing endpoint = hard `ApiDriftError` → exit 4 + manual-fallback pointer; missing parameter = loud warn + Space default + requested value still in state.json. Image binding by schema inspection, never blind positional; session-state-fed `/image_to_3d` handled and logged. (Substring heuristic `"file" in blob` can match e.g. a param named "profile" — theoretical, first-match on `/preprocess_image` is the image in practice; covered by the schema snapshot evidence trail.)
6. **GLB retrieval — APPROVED.** Recursive walk, last `.glb` per Space viewer+download convention; no GLB → exit 1 with result shape recorded.
7. **state.json on failure — APPROVED** with the WARN-2 provenance caveat.
8. **Lazy imports — APPROVED.** `--help` verified dependency-free by the programmer; imports are function-local.
9. **resolution as string ("1024") — APPROVED** per board spec.
10. **.gitkeep vs .gitignore deferral — APPROVED**; correctly TASK-084's to adjudicate (negation needed if blanket-ignoring the dirs — or drop the .gitkeeps, since the script mkdirs on demand).
11. **Write confinement — VERIFIED.** All script writes are under `SCRIPT_DIR/Inbox` + `SCRIPT_DIR/Cache` (paths derived from `__file__`): state.json, api_schema.json, trellis_raw.glb, mkdir of the two working dirs. Zero writes to Source/, Content/, .claude/, or the card-art lane (ruling 4). (gradio_client's own download staging lands in the system temp before the copy — library-standard, outside the repo, not a confinement breach.)

## Scaffold & README checks

- `pyproject.toml`: `requires-python = ">=3.12,<3.13"` ✓ (3.14 excluded); `[tool.uv] package = false` appropriate for script-only. `.python-version` = 3.12 ✓. `uv.lock` sane: `requires-python == "3.12.*"`, gradio-client 2.5.0, pillow 12.3.0, ~18 pkgs — matches handoff.
- README: three stage commands present; exit-code table matches code; manual browser fallback accurately resumes Stage 2 from `Cache/<AssetName>/trellis_raw.glb` with missing-state.json provenance note; HF_TOKEN setup uses the Windows env-var GUI (keeps the token off command lines/shell history — does NOT suggest `setx` or files) and states the env-only law verbatim; concept guidance matches the script's warnings (square, ≥512px). Directory-layout Git column consistent with the TASK-084 plan.
- `--check` NOT executed — correct per spec ("author, do not run"); network smoke is TASK-084's.

## Notes for build-master (TASK-084 carry-forwards)

1. **.gitignore MUST cover `Tools/ArtPipeline/.venv/` in addition to `Inbox/` + `Cache/`, BEFORE the commit** — `.venv/` currently holds 700+ untracked files in the working tree; a `git add -A` sweep would commit a vendored site-packages tree.
2. `.gitkeep` adjudication: if you blanket-ignore `Inbox/` + `Cache/`, either negate the `.gitkeep`s (`!Tools/ArtPipeline/Inbox/.gitkeep` etc.) or delete them — the script mkdirs both on demand, so they are not load-bearing.
3. `--check` smoke contract: expect **exit 0** + three endpoints listed + snapshot at `Cache/api_schema.json` (top-level when run without an asset arg — expected, gitignored artifact). **Exit 4** = API drift → append the live `api_schema.json` findings to this report and route back (counts as a QA loop). **Exit 1** = Space unreachable (network problem, NOT drift — do not route back for a transient outage).
4. WARN-1 and WARN-2 are non-blocking; if the programmer picks them up before your commit, they are one-line and small respectively — no re-QA needed beyond confirming the redact() wrap on WARN-1.
