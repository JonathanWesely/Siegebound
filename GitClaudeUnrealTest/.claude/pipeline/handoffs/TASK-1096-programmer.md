# TASK-1096 — [CACHE-FENCE] handoff (gameplay-programmer)

**THIS DEFECT IS PRE-EXISTING. It affects ALL THREE modes — `retexture`, `image3d` AND `multiimage` — it predates the multi-view work entirely, and it was NOT introduced by TASK-1088.** A reader meeting this row beside the multi-view lane will assume it is that lane's fallout; it is not. The unconfined state-file write path (`_run_mode`'s `asset_dir = CACHE_DIR / asset; asset_dir.mkdir()` plus the two writers) was in the file before `--mode multiimage` existed; `qa/TASK-1089.md` WARN-6 found it while gating TASK-1088 and routed it here precisely because it was out of that diff's fence. Measured, not asserted (`SC-§90`): the fence is placed BEFORE the mode dispatch and section 17 drives the row's shape through all three modes — every one of them refused today, every one of them escaped under MUTANT D.

**Marker:** `TASK-1096-CACHE-FENCE` · **Law:** `SC-§39.1` · `SC-§79` · `SC-§83` · `SC-§90` (cited, not restated)
**Status set to:** `ready-for-qa` · **Gate:** `TASK-1097` (one gate over 1095 + 1096) · **Host:** `TASK-1098`
**Started from:** TASK-1095's working-tree GOLD, verified BEFORE my first edit — `meshy_generate.py` `b1d30ccbe83b96b06929417cebcaad156bb9db9ca1996d0e98d7d25f9a3a6c95`, `test_meshy_multiimage.py` `7d3c4b5b60f419de58427acd307ae46029ea5bbf56aa478bfc4e99640609f606` — both matching `handoffs/TASK-1095-programmer.md` §1.

---

## 0. THE ROW'S TEST — the traversal-shaped CardID, quoted from the run

Section 17 of `test_meshy_multiimage.py`, the row's exact shape, `--mode multiimage` (the four other rows — forward slashes, a real-looking prefix then `..`, an ABSOLUTE path, and the row's shape under `--mode retexture` and `--mode image3d` — print the same eight lines each; full transcript in §6):

```
=== 17. THE STATE-FILE FENCE (TASK-1096, PRE-EXISTING, all three modes): a traversal-shaped CardID is refused BEFORE any file or directory is written ANYWHERE - and a normal CardID still writes BOTH state files where it always did ===
  [PASS] CONTROL: the row's shape, backslashes resolves OUTSIDE Cache/  -- C:\Users\wesel\AppData\Local\Temp\task1088_005_pmm8\deep\Content\X
  [PASS] CONTROL: the row's shape, backslashes stays inside the sandbox  -- C:\Users\wesel\AppData\Local\Temp\task1088_005_pmm8\deep\Content\X
  [PASS] the row's shape, backslashes: _run_mode returns 1 (the guard's RuntimeError -> the exit 1 the tail already maps it to)  -- got 1
  [PASS] the row's shape, backslashes: NOTHING appeared anywhere under the sandbox (outside Cache/ or inside it)  -- tree unchanged
  [PASS] the row's shape, backslashes: the escaped path was not created
  [PASS] the row's shape, backslashes: Cache/ itself is still empty
  [PASS] the row's shape, backslashes: no task, no network  -- 0 tail call(s), 0 wire call(s)
  [PASS] the row's shape, backslashes: the refusal names the fence and says no state file was written
```

Read the two CONTROL lines first: the shape genuinely resolves OUTSIDE `Cache/` (otherwise every refusal below it would be testing nothing), and it lands INSIDE the sandbox (which is what made the mutation safe to execute — see §4). "NOTHING appeared" is a set equality over every path under the sandbox root before vs after, not a spot check on the one place a write is expected — so it covers "none outside Cache/ AND none inside" in one assertion.

---

## 1. WRITES (⭐ `TASK-1098` derives its pathspec from THIS list)

| file | state | sha256 (working tree, final) |
|---|---|---|
| `Tools/ArtPipeline/meshy_generate.py` | MODIFIED by me on top of 1095: **+28 / −2, 3 hunks** (TASK-1096 alone) | `ef5589afc5a2a925611ed66c3ea8eed67266e1439f1c25b9469447fcec47aeca` |
| `Tools/ArtPipeline/test_meshy_multiimage.py` | MODIFIED by me on top of 1095: **+190 / −2, 4 hunks** (TASK-1096 alone) | `54fb4d102e5a55b33be3ac8490130b5ae5feb4cc45bf536bf8aaa5788af39159` |
| `.claude/pipeline/handoffs/TASK-1096-programmer.md` | this note | — |

⭐ **Separability, PROVEN not described:** inverse-applying my hunks (new→old, each target exactly once) to copies of both files reproduces TASK-1095's GOLD hashes **byte-exact** (`b1d30cc…6c95` and `7d3c4b5b…9606`, both `True`). The per-file unified diffs of TASK-1096 alone are what §2 quotes. `git diff --stat` vs `HEAD` (`3e2b87e`) shows the SUM of both rows (`meshy_generate.py` 156 lines, `test_meshy_multiimage.py` 700 lines) — that is 1095 + 1096, not mine alone.

⛔ **Scope census, `git status` at the git root:** under `Tools/ArtPipeline/` exactly my two files carry my bytes. `pipeline_manifest.json` `M` = ALREADY dirty in the session-start snapshot (TASK-1091's lane) — never opened. `TASKBOARD.md` `M` = pre-existing + my one status-line Edit. `CONVENTIONS.md` `M` and `playtest-evidence/2026-09-06/RosterSheet_Trees.png` `M` appeared during my session from OTHER lanes — I never opened either. All `??` entries (`TASK-1083-buildmaster.md`, `TASK-1091-artist.md`, `TASK-1095-programmer.md`, `MainCharacter_preview_*.png`, `Content/RawAssets/MainCharacter*`) belong to other lanes. Zero `Source/**`, zero `Content/**`, zero C++, zero engine, zero MCP, zero network, zero credits, no git add/commit/push.
⚠️ Git's `LF will be replaced by CRLF` warning for `test_meshy_multiimage.py` is pre-existing (1095 §1); my edits preserved each file's own endings (`meshy_generate.py` CRLF, the test LF) — the edit script detected the EOL per file and asserted every target occurred exactly once.

---

## 2. WHAT CHANGED — by SYMBOL, with post-edit line numbers (⚠️ the row's `:1561-1562` / `:1129-1138` were pre-1095 numbers)

The row's `:1561-1562` is today's `_run_mode:1632-1633` + the mkdir; its `:1129-1138` is today's `write_success_state` / `write_failed_state` write lines. Located by symbol, quoted after the edit.

### `meshy_generate.py` — ONE guard definition, THREE new call sites, GLB path byte-identical

| where | what |
|---|---|
| `:715` `_require_inside_cache()` | **UNCHANGED, the only definition.** Census after the edit: 1 `def`, 6 call sites — `:730` (`commit_staged`), `:752` + `:760` (`quarantine_staged`) unchanged; `:1178`, `:1192`, `:1645` new. No second copy (`SC-§39.1`; `qa/TASK-1089.md` E1 preserved). |
| `:1163-1188` `write_success_state()` | `dest = asset_dir / "state.json"` hoisted to `:1172`; **`:1178` `_require_inside_cache(dest)`** — placed BEFORE `_load_state()` (`:1179`) because `_load_state` can itself write `state_pre_meshy.json` (`:1159`) on an unparsable record, so that copy is inside the fence too. mkdir (`:1185`) and the write (`:1186`) follow, unchanged. |
| `:1190-1195` `write_failed_state()` | `dest = asset_dir / "state_failed.json"` hoisted to `:1191`; **`:1192` `_require_inside_cache(dest)`** before the mkdir (`:1193`) and the write (`:1194`). |
| `:1625` `_run_mode()` | `:1632-1633` `asset = args.asset` / `asset_dir = CACHE_DIR / asset` unchanged. **NEW `:1644-1651`:** `try: _require_inside_cache(asset_dir)` / `except RuntimeError as exc: fail(str(exc)); fail("CardID ... does not name a folder under Cache/. Nothing was created and no state file was written - there is no confined place to put one."); return 1`. The mkdir is now `:1652`, AFTER the gate. Both `fail()` lines go through `redact()`. |

The diff of TASK-1096 alone, `meshy_generate.py`:

```diff
@@ -1169,6 +1169,13 @@ def write_success_state(...)
     """
+    dest = asset_dir / "state.json"
+    # Write confinement (TASK-1096, SC-39.1) - a PRE-EXISTING gap in all three
+    # modes, not from the multi-view diff: ... BEFORE _load_state(), whose
+    # state_pre_meshy.json copy is also a write under asset_dir.
+    _require_inside_cache(dest)
     state = _load_state(asset_dir)
@@ -1176,14 +1183,14 @@
     asset_dir.mkdir(parents=True, exist_ok=True)
-    dest = asset_dir / "state.json"
     dest.write_text(json.dumps(state, indent=2, default=str), encoding="utf-8")
 
 def write_failed_state(asset_dir: Path, record: dict) -> None:
+    dest = asset_dir / "state_failed.json"
+    _require_inside_cache(dest)  # TASK-1096: the GLB's fence, on the failure record
     asset_dir.mkdir(parents=True, exist_ok=True)
-    dest = asset_dir / "state_failed.json"
     dest.write_text(json.dumps(record, indent=2, default=str), encoding="utf-8")
@@ -1623,6 +1630,25 @@ def _run_mode(args)
     asset = args.asset
     asset_dir = CACHE_DIR / asset
+    # Write confinement at the EARLIEST site (TASK-1096, SC-39.1). ... PRE-EXISTING
+    # and mode-independent ... NOT introduced by TASK-1088 ...
+    try:
+        _require_inside_cache(asset_dir)
+    except RuntimeError as exc:
+        fail(str(exc))
+        fail(f"CardID {asset!r} does not name a folder under Cache/. Nothing "
+             "was created and no state file was written - there is no "
+             "confined place to put one.")
+        return 1
     asset_dir.mkdir(parents=True, exist_ok=True)
```

**Why TWO sites and not the writers alone (the row says "route the writes through the guard"):** the row itself names `:1561-1562` — the `_run_mode` mkdir — as one of the two sites, and MUTANT D (§4) is the measurement: with BOTH writers guarded and only the `_run_mode` gate bypassed, the row's shape **mkdir'd `deep\Content\X` outside Cache/ and then exited 5** — the run never reached a writer, so a writer-only fence would have left the directory escape in place on every exit-5 path. The writers are guarded too so that a future direct caller (or a refactor that moves the mkdir) cannot escape either — the same shape as `commit_staged`, which guards at the write, not at the caller.

**GLB path:** `commit_staged` (`:724-731`), `quarantine_staged` (`:742-770`), `download_file`, `_finish_task_common`'s stage→validate→swap — **not one byte changed** (the three hunks above are the whole tool diff).

### `test_meshy_multiimage.py`

| where | what |
|---|---|
| `:40-45` | docstring item 7 naming the fence and its provenance |
| `:1152-1160` `_tree()` | every path under a root, relative — the set-equality instrument |
| `:1163-1328` `test_state_files_are_confined_to_cache()` | section 17 — **59 assertions**, quoted in §6 |
| `:1187` | the ABSOLUTE-path shape (`str(box.root / "Content" / "X")`) — no `..` at all; pathlib's `/` REPLACES the left operand when the right one is absolute, so `CACHE_DIR / "C:\..."` IS `"C:\..."` |
| `:1331` / `:1355` | `main()` header names TASK-1096; the section is called after `test_clause8_one_liners()` |
| `:1368-1369` | the ALL GREEN sentence gains "— and a state file can never land outside Cache/" |

---

## 3. EXIT CODE — **1**, and why (row cl. 4)

`_require_inside_cache()` raises **`RuntimeError`** (`:719`). What the tool ALREADY does with that exception: on the shipped GLB path it surfaces inside `_run_mode`'s `try` and is caught by the catch-all `except Exception` (`:1841-1845`) → `fail("Run failed: RuntimeError: ...")` → **`return 1`**. So exit **1** is the code this exception already carries in the contract; nothing new was minted. The contract `0/1/2/3/4/5/6/64` is closed and untouched (test 10 still asserts it).

**Why the `return 1` sits at the gate (`:1651`) rather than flowing through the shared tail:** every except-clause in the tail calls `write_failed_state(asset_dir, …)`. For THIS CardID that call would now itself raise (the writer is fenced) — an exception inside an except handler → a traceback, and a traceback is not a contract exit. There is also, honestly, no confined place to write the record: the asset directory IS the escape. So the gate returns 1 with two redacted `fail()` lines (the guard's own "Refusing to write outside Cache/: <resolved path>" and a sentence saying nothing was created and no state file was written) and touches nothing. Asserted end-to-end: `main(["--mode","image3d","..\\..\\Content\\X"])` with the REAL `_run_mode` returns `1` as a plain return — not a `SystemExit`, not a traceback (`main() returns 1 for the row's shape - a return, not a traceback -- return 1`).

Not 5: the CardID is not a missing INPUT (its inputs may well exist); the refusal is about where the tool would WRITE. Not 64: argparse accepted the string — it is a well-formed positional. Not 2/3/4/6: unrelated meanings.

---

## 4. THE MUTATIONS (`SC-§83`) — ⭐ RED WAS WITNESSED, HERE, BY ME, FOR EVERY AUTHORED GUARD SITE. "NO WITNESSED RED" does NOT apply.

Three authored pins ⇒ three mutants. Each applied by a script asserting the target occurs **exactly once**, run with `__pycache__/meshy_generate*.pyc` deleted first AND `python -B` (no bytecode is ever written, so 1095 §4's same-size same-second stale-pyc hazard cannot arise — verified `(no meshy/multiimage pyc)` after the last run), then restored from a scratch copy and compared to GOLD `ef5589af…aeca` — **all three restores `True`**.

### MUTANT D — the EARLIEST gate bypassed (⭐ hand THIS one to `TASK-1098`)
`meshy_generate.py:1645` `        _require_inside_cache(asset_dir)` → `        pass  # MUTANT D: earliest gate bypassed` (1 of 1; the writers stay guarded)
```
[D] suite exit 1; 148/169 assertions passed.; PASS=148 FAIL=21
    [FAIL] the row's shape, backslashes: _run_mode returns 1 (...)  -- got 5
    [FAIL] the row's shape, backslashes: NOTHING appeared anywhere under the sandbox (outside Cache/ or inside it)  -- new: ['deep\\Content', 'deep\\Content\\X']
    [FAIL] the row's shape, backslashes: the escaped path was not created
    [FAIL] the row's shape, backslashes: the refusal names the fence and says no state file was written
    [FAIL] an ABSOLUTE path: _run_mode returns 1 (...)  -- got 5
    [FAIL] an ABSOLUTE path: NOTHING appeared anywhere under the sandbox (...)  -- new: ['Content', 'Content\\X']
    [FAIL] an ABSOLUTE path: the escaped path was not created
    [FAIL] an ABSOLUTE path: the refusal names the fence and says no state file was written
    ... (same three per remaining shape/mode: returns 1 → got 5, escaped path exists, no fence text)
    [FAIL] main() returns 1 for the row's shape - a return, not a traceback  -- return 5
```
⭐ Read the detail, not the count: **`got 5` beside `new: ['deep\\Content', 'deep\\Content\\X']`** — under the bypass the tool CREATES A DIRECTORY OUTSIDE `Cache/`, then exits 5 (missing input) having never touched a state writer. That is the pre-existing defect caught in the act, on all three modes, and it is why the writer guards alone (still intact under D — every writer-level check stayed GREEN) do not close the row. The "NOTHING appeared" line reds only on the first shape per escaped target because the directory then already exists; the "escaped path was not created" line reds on every shape.

### MUTANT E — the failure-record fence bypassed
`:1192` `    _require_inside_cache(dest)  # TASK-1096: the GLB's fence, on the failure record` → `    pass  # MUTANT E: failure-record fence bypassed` (1 of 1)
```
[E] suite exit 1; 167/169 assertions passed.; PASS=167 FAIL=2
    [FAIL] write_failed_state refuses an escaped asset_dir  -- it WROTE
    [FAIL] the direct writer calls created nothing anywhere  -- new: ['deep\\Content', 'deep\\Content\\X', 'deep\\Content\\X\\state_failed.json']
```
⇒ a `state_failed.json` outside `Cache/` — the row's exact JSON-outside-Cache shape — while the `_run_mode` gate (intact) kept every section-17 run line green. Each guard is witnessed independently.

### MUTANT F — the success-record fence bypassed
`:1177-1178` `    # under asset_dir.` + `    _require_inside_cache(dest)` → `    pass  # MUTANT F: success-record fence bypassed` (anchored on the preceding comment line because a bare `    _require_inside_cache(dest)` also matches `commit_staged:730` — 2 occurrences, so the 2-line anchor is what makes it 1 of 1)
```
[F] suite exit 1; 167/169 assertions passed.; PASS=167 FAIL=2
    [FAIL] write_success_state refuses an escaped asset_dir  -- it WROTE
    [FAIL] the direct writer calls created nothing anywhere  -- new: ['deep\\Content', 'deep\\Content\\X', 'deep\\Content\\X\\state.json']
```

**Why the mutations were SAFE to execute (and why re-witnessing is too):** section 17 nests `CACHE_DIR` two levels inside the sandbox (`<tmp>/deep/deeper/Cache`) so the row's `..\..\Content\X` resolves to `<tmp>/deep/Content/X` — outside Cache/, inside the sandbox — and the absolute shape points at `<tmp>/Content/X`. Under D/E/F the stray writes landed there and `Sandbox.__exit__`'s `rmtree` removed them. Nothing was ever aimed at the real `Tools/ArtPipeline/Cache`, the real `%TEMP%`, or `Content/`.

---

## 5. CLAUSE (5) — the `asset_dir.mkdir()`-before-resolution NIT: **LEFT, deliberately**

It did NOT fall out of (2). `_run_mode`'s mkdir (now `:1652`) is **load-bearing for the GLB staging path**: `download_file()` writes `<dest>.part` into `asset_dir` and never mkdirs — censused, the file has exactly four `.mkdir(` sites: `quarantine_staged:761`, the two writers `:1185`/`:1193`, and `_run_mode:1652`. Moving or deleting it would mean adding a mkdir inside `download_file`/`_finish_task_common`, i.e. re-plumbing the artefact path the row says is UNTOUCHED (cl. 3). So a typo'd-but-confined CardID still leaves an empty `Cache/<CardID>/`. ✅ The shipped claim **"no state file is written on exit 5" STAYS TRUE**: the exit-5 branch (`:1788-1790`) still writes nothing, and it is asserted three times in the AFTER run — test 2 (`no state.json / state_failed.json was written for a missing input`), test 11/12 (`no state file was written for a refused set`), and section 17's `a failure still never writes state.json`.

---

## 6. TEST COUNTS — BEFORE and AFTER, pasted

Invocation: `cd Tools/ArtPipeline && uv run python -B test_meshy_multiimage.py` (the suite is a custom `check()` ledger, not pytest — `pytest` is not in the venv and adding it would touch `pyproject.toml`/`uv.lock`, out of fence; `-B` plus a pre-run `rm __pycache__/meshy_generate*.pyc` is the 1095 §4 discipline).

**BEFORE** (1095's bytes, pycache cleared):
```
110/110 assertions passed.
ALL GREEN - ... and --views can add a view but never drop Front, Side or Back.
SUITE EXIT: 0
```
**AFTER** (final bytes, run on GOLD after the three restores):
```
169/169 assertions passed.
ALL GREEN - every requested view reaches the multi-image endpoint in order, a missing view stops before the network, a refused endpoint never becomes a quiet single-image run, and --views can add a view but never drop Front, Side or Back - and a state file can never land outside Cache/.
SUITE EXIT: 0
```
The inherited 110 (sections 1-16) are all present and green in the AFTER run; the 59 new assertions are section 17. Per shape/mode (six rows × 8 = 48): the two CONTROLs, `returns 1`, `NOTHING appeared`, `escaped path not created`, `Cache/ still empty`, `no task, no network`, `refusal names the fence`. Then:
```
  [PASS] main() returns 1 for the row's shape - a return, not a traceback  -- return 1
  [PASS] main(): still nothing anywhere  -- tree unchanged
  [PASS] write_failed_state refuses an escaped asset_dir  -- Refusing to write outside Cache/: C:\Users\wesel\AppData\Local\Temp\ta
  [PASS] write_success_state refuses an escaped asset_dir  -- Refusing to write outside Cache/: C:\Users\wesel\AppData\Local\Temp\ta
  [PASS] the direct writer calls created nothing anywhere  -- tree unchanged
  [PASS] --mode retexture: state_failed.json is still written at Cache/<CardID>/state_failed.json with the run's record  -- rc 1, present, mode=retexture
  [PASS] --mode image3d: state_failed.json is still written at Cache/<CardID>/state_failed.json with the run's record  -- rc 1, present, mode=image3d
  [PASS] --mode multiimage: state_failed.json is still written at Cache/<CardID>/state_failed.json with the run's record  -- rc 1, present, mode=multiimage
  [PASS] a failure still never writes state.json
  [PASS] state.json is still written at Cache/<CardID>/state.json with the merged provenance  -- rc 0, present, engine=meshy-mi23d
  [PASS] CONTROL: the GLB path is untouched - meshy_raw.glb committed beside it
```
Cl. 6 (b) — "a normal CardID still writes BOTH state files where it always did" — is measured through the REAL paths: `state_failed.json` via the real `_run_mode` failure tail in ALL THREE modes (a 500 on create; retexture given a real `trellis_raw.glb` donor and style ref, image3d a real concept PNG), and `state.json` via the real `_finish_task_common` offline tail (validate → commit → merge → SUCCESS line), with the committed `meshy_raw.glb` beside it as the GLB-path control.

---

## 7. SEVERITY, MEASURED (`SC-§90`, row cl. 3)

Reachable only with a hostile or typo'd argv: the CardID is a positional the operator types; nothing in the pipeline (`pipeline_manifest.json`, `concept_prompts.json`, the sibling scripts) feeds `meshy_generate.py` a CardID programmatically — every mention of the siblings in this file is prose (1089 E3/§2). What such an argv WOULD have done before this row, exhibited under MUTANT D rather than argued: mkdir the escaped path on every mode and every exit path (even exit 5), then — on any path that reaches the tail — write `state_failed.json` or `state.json` there (MUTANT E/F). The GLB could never follow: `commit_staged`/`quarantine_staged` were always confined. Latent, not live; and now closed rather than logged.

---

## 8. FLAGGED DECISIONS FOR `TASK-1097` (⛔ where I am most likely to be wrong)

1. ⭐⭐ **Two-site fence, not writer-only.** The row's cl. 2 text says "route the writes through the guard"; a literal writer-only reading leaves the `_run_mode` mkdir (the row's own `:1561-1562`) unfenced, and MUTANT D shows the directory escaping on an exit-5 path with both writers guarded. So the earliest site is gated too. Cost: `_require_inside_cache` gains three call sites (one definition — census it: `:715` def; `:730/:752/:760` GLB unchanged; `:1178/:1192/:1645` new). Your call whether three sites of one function reads as reuse (my reading of `SC-§39.1`) or as spread.
2. ⭐ **A new `return 1` site, outside the shared tail (`:1651`).** Not a new code (§3) but a new place the code is returned from, bypassing the tail's `write_failed_state`. Alternatives rejected: let the `RuntimeError` propagate to `main()` (Python traceback exit 1, raw path outside the redactor); put the gate inside the `try` (the tail's own `write_failed_state` would raise inside the except handler → traceback). If you rule the gate should instead live in `main()` before `_run_mode`, note the suite calls `_run_mode` directly and 1095's precedent put the deciding-path fence in `_run_mode` for exactly that reason.
3. **The guard's return value is discarded at all three new sites** (`_require_inside_cache(dest)` then the UNRESOLVED `dest` is used), matching `commit_staged:730` rather than `quarantine_staged:752` (which uses the resolved return). Reason: `say("State merged: {dest}")` / `"Failure record written: {dest}"` keep printing the same path text as before, and `asset_dir` keeps its shape for test 3's `seen["asset_dir"].startswith(str(box.cache))`. A resolved path would have changed a log line for no confinement gain.
4. **`write_success_state`'s guard is BEFORE `_load_state()`**, which is not one of the row's two named write lines but is on the same path (`state_pre_meshy.json`, `:1159`). Fenced by position, not by a fourth call.
5. ⚠️ **An edge the guard PERMITS, pre-existing, NOT changed:** a CardID of `.` (or `""` if `main()`'s empty check is bypassed by a direct `_run_mode` call) resolves to `Cache/` itself, which `_require_inside_cache` allows by design (`resolved == root`, `:718`); a state file would then land at `Cache/state_failed.json` — INSIDE Cache/, so not this row's defect, and tightening it would be a second check shape. `main()` already refuses empty. Flagged, not fixed.
6. **`SC-§79` scope of section 17 — what it CAN and CANNOT detect.** CAN: an escape via `..` (three spellings, all three modes), an escape via an absolute CardID, a writer called directly with an escaped dir, and a fence that lets the mkdir through (D reds on the directory, not just on the return code). CANNOT: a TOCTOU where `Cache/<CardID>` is replaced by a junction between the gate and the write (the gate resolves symlinks at check time only — same limit as the GLB guard); an escape through a CardID that is confined but names a sibling asset's folder (`OtherKnight/../ThisKnight` — inside Cache/, by design allowed, as today); and it does not run the row's shape against the REAL `Tools/ArtPipeline/Cache` (deliberately — under a mutation that would mkdir `Tools/Content/X` for real).
7. **The absolute-path shape was added AFTER the first mutation pass** (161 → 169), and the whole runner — GOLD, separability, D/E/F, final — was re-run on the final bytes; the numbers in this note are from that second pass only. The first pass (161/161; D 144/161; E/F 159/161) is superseded, not summed.

## 9. WHAT THIS DOES NOT PROVE (`SC-§94` cl. B)

No credit was spent, no request left this machine, no real `Cache/`/`Inbox/` path was read or written (every `_run_mode` in section 17 runs with `INBOX_DIR`/`CACHE_DIR` redirected). 169/169 proves the fence at both sites offline, against doubles, on Windows path semantics (`..\`, `../`, and a drive-absolute path all measured). It does not prove POSIX behaviour (a backslash CardID is a single filename component there — no traversal, so also no escape), and it does not prove anything about the live Meshy tail — TASK-1091's run on 2026-09-06 (`view_count: 3`, `consumed_credits: 30`) remains the only live evidence, and this row does not touch the path it exercised.

---

**Note to `TASK-1098`:** my diff and TASK-1095's are in the SAME two files (`Tools/ArtPipeline/meshy_generate.py`, `Tools/ArtPipeline/test_meshy_multiimage.py`) and must be hosted TOGETHER — one commit, pathspec = those two files + `handoffs/TASK-1095-programmer.md` + this note + `qa/TASK-1097.md`. Re-witness MUTANT D (expect `148/169` with `got 5` beside `new: ['deep\\Content', 'deep\\Content\\X']`), ⛔ clear `__pycache__/meshy_generate*.pyc` before every run and prefer `python -B`, restore from a scratch copy and compare to `ef5589af…aeca`, ⛔ never `git restore` (both rows are uncommitted). No C++ ⇒ no compile. `Cache/` + `Inbox/` + `__pycache__/` stay gitignored. NEVER push.
