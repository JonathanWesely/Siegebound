# TASK-1088 — [MESHY-MULTIVIEW] handoff (gameplay-programmer)

**Marker:** `TASK-1088-MESHY-MULTIVIEW` · **Law:** `CHAR-3` · `SC-39.1` · `SC-79` · `SC-83` · `SC-94`
**Status set to:** `ready-for-qa` · **Gate:** `TASK-1089` · **Host:** `TASK-1090`

---

## 0. CLAUSE 0 — THE PREFLIGHT, AND IT IS THE HEADLINE

### ✅ VERDICT: THE MULTI-IMAGE ENDPOINT IS **AVAILABLE ON THIS ACCOUNT**. Lane takes **BRANCH A**.

Established **before a line of code was written**, in three steps, **zero credits spent**:

| # | probe | result |
|---|---|---|
| 1 | Meshy API reference for `multi-image-to-3d` | route documented: `POST /openapi/v1/multi-image-to-3d`, images passed as **`image_urls`**, an **ordered array of 1–4** ("Provide 1 to 4 images for Meshy to use in model creation") |
| 2 | `GET /openapi/v1/multi-image-to-3d?page_size=1` with the live account key | **HTTP 200**, JSON array. **Control:** `GET /openapi/v1/image-to-3d` also 200. `GET /openapi/v1/balance` → 200, **balance 3200** |
| 3 | `POST /openapi/v1/multi-image-to-3d` with an **empty body `{}`** | **HTTP 400** `{"message":"Either image_urls or input_task_id must be provided"}` |

**Why step 3 is the load-bearing one, and why it cost nothing.** Step 2 only proves the route is *readable*. Plan gating is enforced at *create* time, so a read-only 200 could still sit in front of a 403 on POST. An empty body **cannot create a task** — there are no images and no `input_task_id`, so there is nothing to bill — yet the response came back from **the route's own field validator**, not from an authorization layer. A second probe (`{"image_urls": [], "ai_model": "nope"}`) returned `400 Invalid values: AIModel must be one of [meshy-5 meshy-6 meshy-7 latest]`, i.e. the request reached **parameter validation on this account**. Not 404 (absent), not 403 (plan-gated).

⚠️ **What the preflight does NOT prove, declared:** that a *valid* create returns 200 and a mesh comes back. Only a real generation shows that, that costs credits, and **this row did not authorise spending them** — so I did not. That evidence is `TASK-1091`'s to produce.

✅ **The preflight is now a standing instrument, not a one-off:** `--check` probes `EP_MULTIIMAGE` alongside balance/retexture/image3d. Run live after the diff — **exit 0**:

```
[meshy]   /openapi/v1/balance - OK (credits remaining: 3200)
[meshy]   /openapi/v1/retexture - OK (list endpoint reachable, 0 task(s) in first page)
[meshy]   /openapi/v1/image-to-3d - OK (list endpoint reachable, 0 task(s) in first page)
[meshy]   /openapi/v1/multi-image-to-3d - OK (list endpoint reachable, 0 task(s) in first page)
[meshy] --check PASSED: key valid, API reachable, documented endpoints present.
```
⇒ if the route ever leaves this plan, `--check` says so **before** a run spends credits discovering it.

---

## 1. WRITES (⭐ `TASK-1090` derives its pathspec from THIS list)

| file | state |
|---|---|
| `Tools/ArtPipeline/meshy_generate.py` | **MODIFIED** — +245 / −14 |
| `Tools/ArtPipeline/test_meshy_multiimage.py` | **NEW** — the named test file |
| `.claude/pipeline/handoffs/TASK-1088-programmer.md` | this note |

⛔ **NOT touched, censused:** `trellis_generate.py` · `refine_trellis_glb.py` · `concept_generate.py` · `pipeline_manifest.json` · all of `Source/**` · all of `Content/**` · `CONVENTIONS.md`. `git status -- Tools/ArtPipeline/` lists **exactly the two files above**.
⚠️ `CONVENTIONS.md` and `Content/UI/WBP_CardHand.uasset` are dirty in the tree — **not mine**, parallel lanes. I only ever *read* `CONVENTIONS.md`.
⛔ Nothing staged, no commit, no push. `Cache/` and `Inbox/` untouched and still gitignored. No editor, no MCP, no `L_Arena`.

---

## 2. THE MODE

`--mode multiimage <CardID>` → reads `Inbox/<CardID>_Front.png`, `_Side.png`, `_Back.png` (the `CHAR-2` pinned pattern) → one `POST /openapi/v1/multi-image-to-3d` carrying `image_urls` **in the requested order** → `Cache/<CardID>/meshy_raw.glb` + merged `state.json`.

**It adds no new machinery.** Resolution + payload are ~40 lines; everything after that is the **existing** `_finish_task_common()` — the same create → poll → download → validate → commit tail both shipped modes use. There is **no second copy of the artefact guard**, which is the thing most likely to have gone wrong here.

**Inherited laws, each by name:**
- **Secret** — key never on argv, never in a file, never printed; `state.json` carries no credential material. The new path adds no print that bypasses `redact()`.
- **TLS** — untouched. Census: `verify=False` / `_create_unverified` / `CERT_NONE` / `check_hostname=False` = **0 occurrences** in both files.
- **`SC-39.1` 3-phase guard** — reached via the shared tail, unmodified: download → `.part`, `validate_glb_file()`, `commit_staged()` only on pass, rejection **quarantined** to `Cache/<CardID>/_rejected/`.
- **Exit codes** — all preserved. New reachable meanings: **`5`** missing view · **`4`** endpoint refused (below) · `3` quota still an expected pause, never retried · `6` degenerate.
- **THE INVARIANT** — output is `Cache/<CardID>/meshy_raw.glb`; `Content/` appears in this file **only in prose** (3 comment/docstring lines, censused). `_require_inside_cache()` still gates the only write.
- **Provenance** — merges into `state.json` without clobbering: `view_count`, `view_order`, and a **sha256 per image**. Engine tag is **`meshy-mi23d`**, deliberately distinct from image3d's `meshy-i23d`, so *"was this body built from one view or three?"* is answerable from provenance alone.

**Two small judgement calls, flagged rather than buried:**
1. `--views` (default `Front,Side,Back`) exists because the endpoint takes **1–4** images and the concept sheet has more than three views. Asking for **fewer than three prints a `warn()`** naming what you gave up. It is never a *silent* narrowing — but if the gate considers any sub-3 path unacceptable surface, say so and I will drop the flag.
2. Output name is **shared with `image3d`** (`meshy_raw.glb`) **on purpose** — Stage 2 consumes that name whatever produced it. A re-run shelves the prior record into `state.json`'s `meshy_history` rather than erasing it.

---

## 3. HOW A FAILED / UNAVAILABLE MODE ANNOUNCES ITSELF

⭐ **This is the clause the row exists for, so it is stated as behaviour, not intent.**

**(a) A missing view → exit `5`, before the network.** `resolve_multiview_images()` names **every** absent path, lists the views that *were* present, and says `exit 5`. It runs **before** the key is used for anything. Measured: the tripwire standing where the network is records **0 calls**, and **no `state.json`/`state_failed.json` is written**.
> Rationale in the code: a run that dropped `_Back.png` would upload two images, spend credits, print `SUCCESS`, return `0` — and hand back a character generated without the surface the back view existed to specify. Nothing in the exit code, the log or `state.json` would say so.

**(b) The endpoint refuses the account (403/404) → exit `4`, and it says out loud that nothing fell back.** Verbatim body surfaced, then:
> *"NOTHING was generated and NOTHING fell back to --mode image3d. A single-view run cannot see the back of the subject, so it is a different job with a different result; it is never substituted for this one silently. Re-run with --mode image3d yourself if you accept that loss (CHAR-3 Branch B)."*

⇒ **Branch B is a decision a human makes on the command line, out loud.** The tool will never take it on your behalf.

⚠️ **Scoped change, declared for the gate:** the 403/404 → exit-4 branch is guarded by `args.mode == "multiimage"`. `retexture` and `image3d` keep their shipped **exit 1** for those statuses, byte-for-byte. **There is a control asserting exactly that**, so "scoped" is measured, not claimed.

---

## 4. THE MUTATION (`SC-83`) — ⭐ **RED WAS WITNESSED, HERE, BY ME**

I could run this one. Python + `uv` are available to me, so **"NO WITNESSED RED" does NOT apply** — both mutations were applied, run, and restored **byte-exact from a scratch copy** (never `git restore`; the diff is uncommitted).

**Baseline: `uv run test_meshy_multiimage.py` → 55/55, exit 0.**

### MUTATION A (primary — hand this one to `TASK-1090`)
**In `resolve_multiview_images()`, change the line `    if missing:` to `    if False:`.** Unique in the file; one token.

```
MUTATION A APPLIED
  [FAIL] exit code is 5 (input missing)  -- got 0
  [FAIL] no task was created (create/poll/download never entered)  -- 1 call(s) - must be 0
  [FAIL] the message names the missing file  -- TestKnight_Back.png
  [FAIL] the message says MISSING and names the exit code
51/55 assertions passed.   (suite exit 1)
```
⭐ **Read the failure detail, not just the word FAIL:** `got 0` with **`1 call(s)`** into the task tail. The mutant *generates from two views and returns success*. That is the `SC-36.1` shape — a surface that reviews clean and does the wrong thing — caught in the act.

### MUTATION B (secondary — the order)
**Change `    payload["image_urls"] = [image_data_uri(path) for _view, path in resolved]` to iterate `sorted(resolved)`.**
```
  [FAIL] each image_urls[i] is byte-identical to the view at index i  -- mismatched: index 0 (Front), index 1 (Side), index 2 (Back)
  [FAIL] reversing --views reverses image_urls
53/55 assertions passed.
```
⇒ the count stays right and the order silently changes — which is precisely why the test hashes each decoded image against the file at its index instead of counting.

**Restore (both times):** `sha256 04c0d0ba0fa9902d12392cbfe1d004ebe490686342f1b68ab413d8e9d9343fbc` — **BYTE-EXACT**. Final re-run **55/55 green**.

---

## 5. WHAT QA SHOULD SCRUTINISE (⛔ where I am most likely wrong)

1. ⭐⭐ **The 403/404 mode guard.** It is the one place I changed a **shared** except-clause. I scoped it to `multiimage` and wrote a control — **check I did not shift `retexture`/`image3d` behaviour for any other status.**
2. ⭐ **`--views` sub-3.** A `warn()` — not a refusal. Is a loud narrowing acceptable surface, or should the flag go? Your call; I flagged rather than decided.
3. ⭐ **Shared `meshy_raw.glb` output name.** Deliberate (Stage 2's donor), but it means a multiimage run overwrites an image3d donor. `meshy_history` preserves the record; the *artefact* is replaced.
4. **Params sent.** `ai_model`/`topology`/`target_polycount`/`should_remesh`/`should_texture`/`enable_pbr` — copied from `image3d`. I did **not** wire the multi-image-only extras (`ultra_mode`, `pose_mode`, `texture_image_urls`, `multi_view_thumbnails`) — out of fence, and `pose_mode: t-pose` may be worth a later row given the concept is a T-pose.
5. **Non-ASCII.** Printed strings are ASCII, matching this file's own `SC-39.1` convention. The section sign survives **only** in docstrings/comments (lines 15/17/31/72/135/909) — line 72 is the pristine file's own.

## 6. WHAT THIS DOES NOT PROVE (`SC-94` cl. B)

⛔ **NO MESH HAS BEEN GENERATED.** Nothing here spent a credit or received a GLB. 55/55 green + a live `--check` prove the **route is present and the payload is correct**; they do **not** prove Meshy returns a usable multi-view mesh for this character. **That is answered at `TASK-1091`, and only there.**
