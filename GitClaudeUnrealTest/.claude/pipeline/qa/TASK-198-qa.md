# QA Report — TASK-198 (Meshy second-engine client: meshy_generate.py + guard-secrets extension)

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-18. Files: `Tools/ArtPipeline/meshy_generate.py` (new) +
`.claude/hooks/guard-secrets.sh` (one-alternation extension). Reviewed against the board spec
(M7.5 → TASK-198), CONVENTIONS "Meshy second engine (M7.5)" AS AMENDED (canonical `MESHY_TOKEN`,
`MESHY_API_KEY` deprecated alias), handoffs/TASK-198.md, `trellis_generate.py` (the mirror
baseline), `pyproject.toml`, and the LIVE evidence (`Cache/Ogre/state.json` + GLBs on disk).
Board status NOT edited per dispatch.

Blockers: 0 · Warnings: 2 (non-blocking) · Nits: 4

---

## 1. Secret law: VERIFIED — no leak path found

- **Resolution order matches the amended law:** process env `MESHY_TOKEN` → `MESHY_API_KEY`
  (alias, never required) → HKCU `Environment` via winreg (both names, same order, Windows only).
  The value is registered with the redactor the moment it resolves (`require_api_key`:227) and
  only the SOURCE DESCRIPTION is ever printed. The winreg read never echoes, logs, or persists the
  value; a registry read failure prints exception type only. **Adjudication of the HKCU fallback:**
  within the ENV-ONLY law's spirit — it reads the same user-environment store Jonathan set (stale
  shells simply haven't inherited it), writes nothing, and is documented in the module docstring +
  board TASK-197 note. Accepted.
- **Every output path redacts:** `say`/`warn`/`fail` all wrap `redact()` (registered values + the
  generic `msy_[A-Za-z0-9]{15,}` shape); the argparse usage-error override redacts too, so a key
  pasted on argv is scrubbed even before registration (mirrors trellis/concept exactly).
- **File writes carry no credential material:** `state.json` meshy block = params (flags only —
  the data-URI payload is a separate dict copy that never reaches any state writer), input SHAs,
  output path/bytes/sha, credits, timestamps. `state_failed.json` error strings are explicitly
  `redact()`-ed BEFORE the file write (lines 837/844/851/858) — correct, since file writes bypass
  the printing wrappers. No api_schema.json equivalent is written (drift asserted per-response
  instead — sound design for a fixed REST contract).
- **Data-URI construction** embeds file bytes only — no key. Signed download URLs are logged
  host+path only (`_strip_query`); urllib exception strings don't embed URLs. Download failure
  messages redact.
- **My own sweeps:** `msy_[A-Za-z0-9]{20,}` over the tool → 0 matches (the in-file regex literal
  cannot self-match); over `Cache/Ogre/` → 0 matches. Handoff's sweep corroborated.
- **guard-secrets.sh pattern correct:** `msy_[A-Za-z0-9]{20,}` added inside the existing
  high-confidence `grep -E` alternation with a provenance comment; rest of the hook untouched.
  Catches the observed real key shape (`msy_` + 36 alnum); will not false-positive normal text
  (requires the literal prefix + 20 alnum run). Consistent with the `hf_{20,}` convention. The
  in-tool redactor being stricter (`{15,}`) is the safe direction for each side (redactor
  over-scrubs output; hook at 20 avoids deny-gate false positives).
- Prefix acquisition method (User-scope prefix-pattern-only read, full value never echoed) is the
  right way to have done it.

## 2. Exit-code map: VERIFIED — all six reachable and correctly routed

- **0** success (live Ogre run) / `--check` pass (live, 3300 credits).
- **2** uniquely key-unset: `require_api_key()` → 2 in both `run_check` and `_run_mode`; argparse
  remapped to 64 via the `_Parser` override (mirrors trellis). Exit-2 test evidence (env popped +
  non-win32 sim) is sound — the real HKCU var makes a native repro impossible without touching
  Jonathan's environment; the sim exercises the actual code path.
- **3** credits = expected pause: `HTTP 402 OR credit-worded body` → `InsufficientCreditsError` at
  the transport layer, plus credit-worded `task_error` at `assert_succeeded` — NEVER retried
  (`call_api` re-raises before the retry loop), surfaced verbatim + guidance, failure record
  written. **No misclassification path into 3:** 429 rate-limit has no credit wording → retried as
  transport → eventually 1; 400/401/404 propagate un-retried with verbatim bodies.
- **4** drift: non-JSON body, non-object/array JSON, missing `result` task id, unknown task
  status, missing `model_urls.glb`, balance-shape failure, and 404-in-`--check` all raise/route to
  4 with diagnostic key lists. Per-response assertion instead of a schema snapshot — appropriate.
- **5** input: donor GLB missing (`resolve_donor_glb`, names both candidates), concept/style PNG
  missing, or PIL-unreadable (`InputError`) — validated BEFORE any network spend.
- **64** usage: `--mode bogus`, missing mode/CardID, any argparse error (live-tested per handoff).
- **1** generic: transport-after-retries, non-402 HTTP, FAILED/CANCELED tasks (verbatim
  task_error), poll timeout (RuntimeError with resume guidance; task id already in the failure
  record — verified `state_record["task_id"]` is set pre-poll).
- **Poll loop is finite:** monotonic wall-clock deadline (default 30 min, floor 5), poll interval
  clamped ≥ 2 s, unknown statuses exit via drift, timeout surfaces with the task id + re-run note.

## 3. Contract fidelity: VERIFIED

- **CLI mirrors trellis_generate.py:** optional positional CardID + `--check` + shared
  `--timeout-minutes` (with floor clamp) / `--attempts` (≥1 clamp) pattern, plus mode-specific
  flags; same `_Parser` 64-remap; same local-import style keeping `--help` dependency-free.
- **`--check` requires-key deviation: ADJUDICATED ACCEPTABLE** — Meshy has no unauthenticated
  surface, and the board acceptance line itself pins "`--check` passes with key / exits 2
  without". Free read endpoints only (balance + two task-lists); zero credit spend; documented
  in-code and in the handoff.
- **`MESHY_TOKEN` naming deviation: NOT A FINDING** — matches the amended CONVENTIONS §"Meshy
  second engine" secret law exactly (canonical MESHY_TOKEN, alias accepted, never required).
- **Provenance merge: VERIFIED against the live file.** `Cache/Ogre/state.json`: the TRELLIS
  record (space, concept sha, params, trellis output_glb/bytes, timestamps) is preserved verbatim
  — the diff is additions only (`engine: meshy-retex` + `meshy` block). Prior meshy blocks shelve
  into `meshy_history`; unparsable pre-state is preserved to `state_pre_meshy.json` before a fresh
  write; non-dict JSON wraps as `pre_meshy_state`. Failures write `state_failed.json` ONLY
  (trellis QA WARN-2 scheme) — a surviving artifact keeps its success provenance.
- **Stdlib-only claim TRUE:** urllib/ssl/winreg/base64/hashlib/json/shutil + PIL — pillow is a
  PRE-EXISTING dependency (pyproject.toml untouched, verified: gradio_client/huggingface_hub/
  pillow only). No new deps.
- **TLS law held:** `ssl.create_default_context()` everywhere (verification never disabled,
  SSL_CERT_FILE honored); SSL failures get the Norton-playbook guidance verbatim. Live outcome
  (bare TLS OK on api. + assets.meshy.ai, Windows cert store explanation) is consistent.
- **Downloads:** streamed via `copyfileobj` (no whole-GLB in memory), `.part` → atomic
  `Path.replace`, retried with backoff.

## 4. Data-URI upload robustness: acceptable, one advisory WARN

- No silent-truncation risk: `read_bytes()` + `b64encode` are exact; the payload either arrives
  whole or the request fails loudly (verbatim body).
- Memory: a ~30 MB donor peaks around ~150 MB transient (bytes + b64 + json string + encoded
  body) — fine. A future hero-class dense donor (Castle-scale TRELLIS raw can be far larger than
  its 40k FINAL budget — the live Ogre donor was 22.5 MB pre-decimation) scales that peak
  linearly and could meet an undocumented Meshy request-size cap → expect HTTP 413/400, which is
  surfaced verbatim; 5xx/429/transport retries re-encode and re-upload the full body each attempt
  (bounded at 3). See WARN-2.
- Upload timeout 600 s is generous for ~40 MB bodies; the donor size IS printed before encoding,
  so an operator sees the risk factor.

## 5. Live-run evidence: CONSISTENT

- `Cache/Ogre/meshy_retex.glb` exists (with `trellis_raw.glb` beside it); state.json records
  30,928,280 bytes / sha `6fe26ac0…7c28ec` — matches the handoff. Task id
  `019f76c7-81a7-7af3-9d5d-a4d92ba35333`, endpoint `/openapi/v1/retexture`, donor sha
  `b4fa1938…0ee7c4`, style sha `323d0204…3374b7` — and the style sha equals the ORIGINAL TRELLIS
  concept sha in the same file (internal consistency). Credits: consumed 10, 3300 → 3290,
  ~2 min wall clock. `enable_original_uv=true` recorded (TASK-199's UV-preservation note holds).

## Findings

- [WARN] meshy_generate.py:751–753 — **no OutputGuard equivalent and no CardID validation**:
  `asset_dir = CACHE_DIR / args.asset` + `mkdir(parents=True)` runs BEFORE input validation, so a
  traversal-shaped CardID (`..\..\x`) creates directories (and on a contrived match, state files)
  outside Cache/. **Adjudication: inherited-parity, not a regression** — `trellis_generate.py`
  (shipped, QA-passed TASK-082) has the identical pattern (its line 414–417), and the
  board contract is "MIRROR trellis_generate.py". The tool writes only donor-derived artifacts
  under the composed dir, and its operators are the pipeline agents. RECOMMEND (follow-up, both
  tools): add concept_generate.py's `CARD_ID_RE` gate on the positional arg — one line each.
  Non-blocking.
- [WARN] meshy_generate.py:460–475/791–793 — retries on create re-encode and re-upload the full
  data-URI body (bounded ×3); a future oversized hero donor hitting a server request-size cap
  (413) would waste two retries before exit 1. RECOMMEND: treat 413 like 400 (no retry) and/or
  pre-flight-warn above a size threshold when Meshy documents one. Failure mode today is
  noisy-but-correct, never silent. Non-blocking.
- [NIT] `_looks_like_credit_error` wording set ("insufficient credit" / credit+exhaust/enough)
  would miss e.g. "you have run out of credits" → exit 1 instead of 3. HTTP 402 — the documented
  primary signal — is always caught, so only a wording-drift edge; verbatim body still surfaces.
- [NIT] In run mode a 404 maps to exit 1 (generic) rather than 4 — defensible (a mid-run 404 is
  ambiguous between endpoint-gone and task-id problems; `--check` is the drift adjudicator, and
  it does map 404→4) — recording the asymmetry.
- [NIT] Handoff §"QA should scrutinize" says "MeshyHttpError 401→1 with guidance" — the
  regenerate-key guidance exists only in `--check`; run-mode 401 exits 1 with the verbatim body
  but no hint. Doc imprecision only.
- [NIT] Redactor pattern `{15,}` vs hook `{20,}` — intentional-looking and safe in both
  directions, but the handoff quotes both numbers without noting the difference. Cosmetic.

## Notes for build-master / orchestrator

- TASK-199 is CLEAR from QA's side: arm-C's donor (`Cache/Ogre/meshy_retex.glb`) exists with full
  provenance; Stage-2 rebake can start on this verdict.
- Commit scope (rides the M7.5 integration flow): `Tools/ArtPipeline/meshy_generate.py` +
  `.claude/hooks/guard-secrets.sh`. Cache artifacts stay out (gitignored). CONVENTIONS was
  amended by manager separately — nothing for this task there.
- Carry-forward recommendation for the next tooling touch: the CARD_ID_RE gate in
  meshy_generate.py AND trellis_generate.py (WARN-1).
