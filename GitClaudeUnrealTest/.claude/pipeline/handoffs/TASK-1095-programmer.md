# TASK-1095 — [VIEWS-FENCE] handoff (gameplay-programmer)

**Marker:** `TASK-1095-VIEWS-FENCE` · **Law:** `CHAR-§2` · `SC-§89` · `SC-§79` · `SC-§83` · `SC-§90` · `SC-§94` cl. A
**Status set to:** `ready-for-qa` · **Gate:** `TASK-1097` · **Host:** `TASK-1098`
**Provenance (row cl. 0):** not a bug fix, not a regression — `qa/TASK-1089.md` WARN-1 (+ WARN-4/5/7), a PASS with 0 BLOCKER, remedied on this follow-up row so the byte-frozen TASK-1090 diff was never re-opened. Started from the committed bytes: working-tree `sha256 04c0d0ba…3fbc` verified BEFORE the first edit, matching `3e2b87e`.

---

## 0. THE TEST THAT EARNS THE ROW — `--views Front,Side,ThreeQuarter` ⇒ REFUSED

Section 11 of `test_meshy_multiimage.py`, quoted from the run, not paraphrased. Note the two CONTROL lines first: every crop the request names EXISTS on disk, and the request has exactly 3 views — so resolution passes and a count floor passes. Only identity stops it.

```
=== 11. THE ROW'S TEST: --views Front,Side,ThreeQuarter is REFUSED (three views - a COUNT check passes it) ===
  [PASS] CONTROL: every requested crop exists on disk (resolution alone would pass)  -- TestKnight_Front.png, TestKnight_Side.png, TestKnight_ThreeQuarter.png
  [PASS] CONTROL: the request has exactly as many views as the pinned set  -- 3 == 3 - a count floor cannot see this case
  [PASS] parse_views REFUSES Front,Side,ThreeQuarter  -- --views Front,Side,ThreeQuarter drops Back. --mode multiimage always sends the C
  [PASS] the refusal NAMES the dropped pinned view (Back)
  [PASS] the refusal names --mode image3d as the legitimate single-view route
  [PASS] main() exits 64 (CLI usage) for Front,Side,ThreeQuarter  -- usage 64
  [PASS] the run was never entered from the CLI  -- 0 call(s) - must be 0
  [PASS] the CLI message names Back and --mode image3d
  [PASS] _run_mode returns 64 for Front,Side,ThreeQuarter  -- got 64
  [PASS] no task was created (the shared tail was never entered)  -- 0 call(s) - must be 0
  [PASS] the network was never reached  -- 0 call(s) - must be 0
  [PASS] the deciding-path message names Back and --mode image3d
  [PASS] the old cardinality warn is GONE - replaced, not stacked under the fence
  [PASS] no state.json / state_failed.json was written for a refused set  -- asset dir empty or absent
```

Refused at BOTH entry points: the parser (`main()` → `SystemExit 64`) and the deciding path (`_run_mode()` → `return 64`, before any file is probed, 0 tail calls, 0 wire calls, nothing written).

**And the same case under MUTATION A (the count floor re-created) — see §4 — is ACCEPTED, `main()` reaches the run, `_run_mode` returns 0 with `1 call(s)` into the tail.** That is a Back-less knight generated and reported as success, caught in the act.

---

## 1. WRITES (⭐ `TASK-1098` derives its pathspec from THIS list)

| file | state | sha256 (working tree) |
|---|---|---|
| `Tools/ArtPipeline/meshy_generate.py` | MODIFIED, +126 / −28 (8 hunks) | `b1d30ccbe83b96b06929417cebcaad156bb9db9ca1996d0e98d7d25f9a3a6c95` |
| `Tools/ArtPipeline/test_meshy_multiimage.py` | MODIFIED, +515 (6 new sections, one docstring addition) | `7d3c4b5b60f419de58427acd307ae46029ea5bbf56aa478bfc4e99640609f606` |
| `.claude/pipeline/handoffs/TASK-1095-programmer.md` | this note | — |

⛔ **Scope census, `git status` at the git root (`GitClaudeUnrealTesting/`):** under `Tools/ArtPipeline/` exactly my two files are modified by me. `pipeline_manifest.json` shows `M` — it was ALREADY dirty in the session-start snapshot (TASK-1091's live run) and I never opened it for writing. `TASKBOARD.md` `M` = pre-existing + my one status-line Edit. All `??` entries (`TASK-1083-buildmaster.md`, `TASK-1091-artist.md`, `playtest-evidence/2026-09-06/*.png`, `Content/RawAssets/MainCharacter*`) belong to the other lanes. Zero `Source/**`, zero `Content/**`, zero `CONVENTIONS.md`, zero C++, zero engine, zero MCP, zero network, zero credits, no git add/commit/push.
⚠️ Git warns `LF will be replaced by CRLF` for `test_meshy_multiimage.py` — pre-existing (the file was authored LF in TASK-1088; `meshy_generate.py` is CRLF and got no warning). Edits preserved each file's own endings (the tool diff is 8 hunks, not 1,900 lines).

---

## 2. WHAT CHANGED — `meshy_generate.py`, by line (post-edit numbers)

| where | what |
|---|---|
| `:143-146` | comment under `MULTIVIEW_MAX_IMAGES` naming the fence and the `--mode image3d` route |
| `:881-921` **`require_pinned_views(views)`** — NEW, the single definition of the fence | `absent = tuple(v for v in MULTIVIEW_DEFAULT_VIEWS if v not in views)` — membership by NAME, not `len()`. Returns `views` unchanged or raises `ValueError` whose message (a) names every dropped pinned view, (b) says a 4th may be ADDED but none dropped, (c) on a case slip adds `(view names are exact: 'front' is not 'Front')`, (d) names `` `--mode image3d <CardID>` (CHAR-3 Branch B) `` and says `This is exit 64.` |
| `:924-951` `parse_views()` | after the existing empty / cap / duplicate checks, `return require_pinned_views(views)`. The shipped `main()` mapping `ValueError → build_parser().error() → exit 64` is untouched (`:1979`). |
| `:997-998` `resolve_multiview_images()` message | the exit-5 text used to end *"or ask for a smaller set EXPLICITLY with --views"* — that route is now exit 64, so the sentence would have taught a dead route. Now: *"--views cannot drop a pinned view (exit 64); a deliberately single-view job is `--mode image3d` (CHAR-3 Branch B)."* |
| `:1675-1689` `_run_mode()` multiimage branch | **THE COUNT CHECK IS GONE.** `if len(resolved) < len(MULTIVIEW_DEFAULT_VIEWS): warn(...)` is deleted, not softened. In its place, BEFORE `resolve_multiview_images()`: `views = parse_views(args.views) if isinstance(args.views, str) else require_pinned_views(tuple(args.views))` inside a local `try/except ValueError` → `fail(str(exc)); return 64`. Resolution then runs on `views`. |
| `:1594-1614` `_finish_task_common()` SUCCESS line | `sent_images = payload.get("image_urls")`; `views_note = f" Views sent: {len(sent_images)} ({' -> '.join(inputs_block.get('view_order') or ())})."` when `image_urls` is a list, else `""`. The `say()` becomes `SUCCESS: {dest} - validated: {...}.{views_note}{credits_note} Next: ...`. Other modes' line is byte-identical (no `image_urls` ⇒ empty note; asserted). |
| `:1794-1801` 403/404 branch | `if str(exc.context).startswith("POST"):` now gates ONLY the *"The multi-image endpoint refused this account"* `fail()`. The verbatim-body line, the *"NOTHING fell back"* line and `return 4` are unchanged. |
| `:1886-1895` `--views` help | states the pinned three are REQUIRED, a 4th may be added, dropping one is exit 64, `--mode image3d` is the single-view route |
| `:1899` `--ai-model` help | `meshy-5, meshy-6, meshy-7, or latest` |

Censuses for the gate (`rg` over the tool, post-edit): `len(resolved) <` / `len(MULTIVIEW_DEFAULT_VIEWS)` = **0 live hits** (3 hits, all in a docstring and a comment describing the removed check). `args.views` = **3 hits**: the fence call at `:1684-1685`, `main()`'s `parse_views` at `:1979`, and a comment at `:1599` — **never inside the SUCCESS f-string.** `require_pinned_views` = 1 definition, 2 call sites.

## 3. TEST COUNTS — BEFORE and AFTER, pasted

**BEFORE** (committed bytes, `uv run test_meshy_multiimage.py`):
```
55/55 assertions passed.
ALL GREEN - every requested view reaches the multi-image endpoint in order, a missing view stops before the network, and a refused endpoint never becomes a quiet single-image run.
SUITE EXIT: 0
```
**AFTER** (both files as they stand; two clean consecutive runs):
```
110/110 assertions passed.
ALL GREEN - every requested view reaches the multi-image endpoint in order, a missing view stops before the network, a refused endpoint never becomes a quiet single-image run, and --views can add a view but never drop Front, Side or Back.
SUITE EXIT: 0
```
The original 55 (sections 1-10) are all still present and green in the AFTER run: `55 PASS / 0 FAIL` counted by section range. The 55 new assertions are sections 11-16 (`test_meshy_multiimage.py:701-1130`); full transcript of sections 11-16 quoted in §6 below.

## 4. THE MUTATIONS (`SC-§83`) — ⭐ RED WAS WITNESSED, HERE, BY ME. "NO WITNESSED RED" does NOT apply.

Each mutant was applied by a script that asserts the target occurs **exactly once**, run, then the file restored from a scratch copy and its sha256 compared to the post-change GOLD `b1d30cc…6c95` — all three restores matched.

### MUTATION A — the row's own guard: identity check → count floor (⭐ hand THIS one to `TASK-1098`)
`meshy_generate.py:900`: `    absent = tuple(v for v in MULTIVIEW_DEFAULT_VIEWS if v not in views)` → `    absent = MULTIVIEW_DEFAULT_VIEWS[len(views):]  # MUTANT A: count floor` (1 of 1 occurrence)
```
  [FAIL] parse_views REFUSES Front,Side,ThreeQuarter  -- it was ACCEPTED
  [FAIL] the refusal NAMES the dropped pinned view (Back)
  [FAIL] the refusal names --mode image3d as the legitimate single-view route
  [FAIL] main() exits 64 (CLI usage) for Front,Side,ThreeQuarter  -- reached None
  [FAIL] the run was never entered from the CLI  -- 1 call(s) - must be 0
  [FAIL] the CLI message names Back and --mode image3d
  [FAIL] _run_mode returns 64 for Front,Side,ThreeQuarter  -- got 0
  [FAIL] no task was created (the shared tail was never entered)  -- 1 call(s) - must be 0
  [FAIL] the deciding-path message names Back and --mode image3d
  [FAIL] a case-slipped view ('front') is refused  -- it was ACCEPTED
100/110 assertions passed.   (suite exit 1)
```
⭐ Read the detail, not the word: **`got 0` beside `1 call(s)`** — under a count floor the ThreeQuarter set *generates and returns success*. And the 2-view / 1-view refusals in section 12 stay GREEN under this mutant, which is the whole finding: a count floor is green on the easy half and blind to exactly the case the row exists for.

### MUTATION B — `SC-§94`'s own falsifiable test, pointed at the new instrument: the SUCCESS line echoes argv
`:1602` `sent_images = payload.get("image_urls")` → `sent_images = list(args.views)`, and `:1605` `' -> '.join(inputs_block.get('view_order') or ())` → `' -> '.join(args.views)` (each 1 of 1)
```
  [FAIL] CONTROL (SC-94): argv says 4 views incl. 'Phantom'; the line reports the 3 that were SENT  -- Views sent: 4 (Front -> Side -> Back -> Phantom)
  [FAIL] CONTROL: a 2-image body prints exactly 'Views sent: 2 (Front -> Side)' - the instrument follows the body  -- Views sent: 3 (Front -> Side -> Back)
  [FAIL] CONTROL: the image3d SUCCESS line carries no Views clause (unchanged)
107/110 assertions passed.
```
⇒ the echo prints a view that was never sent and fails to move when the body shrinks — the `(640, 360, 260)` shape, on a log line, in a test.

### MUTATION C — the WARN-4 gate removed
`:1797` `if str(exc.context).startswith("POST"):` → `if True:  # MUTANT C: narrative ungated` (1 of 1)
```
  [FAIL] a poll 404 does NOT say the endpoint refused the account
109/110 assertions passed.
```

### 🚨 A FINDING FOR `TASK-1098`, FOUND THE HARD WAY — STALE BYTECODE SURVIVES A SAME-SIZE, SAME-SECOND RESTORE
After restoring from MUTATION C the suite ran **109/110 five times in a row** with the source byte-exact at GOLD. Cause, measured: the mutant line is **39 characters, exactly the original's length**, and the mutant write and the restore landed in the **same integer second** (`__pycache__/meshy_generate.cpython-312.pyc` mtime `08:58:57.417`, restored source `08:58:57.738`). Python validates a `.pyc` by source mtime (seconds) + size, so the **mutant bytecode stayed valid** — the cached module had 0 `startswith` references, the source has 1. `rm __pycache__/meshy_generate*.pyc` ⇒ **110/110, twice**. `__pycache__/` is gitignored (`.gitignore:68`).
⇒ ⛔ **When re-witnessing: delete `Tools/ArtPipeline/__pycache__/meshy_generate*.pyc` before EVERY run, or run `uv run python -B test_meshy_multiimage.py`.** The failure has both signs — a restore that "stays red" (this case) and, worse, a mutant that "stays green" because the GOLD pyc outlived it, which would read as *"the test is blind"*. Neither is what happened to the file.

## 5. CLAUSE (8) — the three one-liners: ALL THREE DONE

| item | done? | the change | one line? |
|---|---|---|---|
| WARN-4 poll-404 narrative | ✅ | `:1797` `if str(exc.context).startswith("POST"):` around the *"refused this account"* `fail()` only | one line of logic; the 4 continuation lines of that `fail()` were re-indented mechanically and a 3-line comment added. Exit 4 unchanged (asserted: poll 404 still `got 4`; create 404 still says "refused"). |
| WARN-5 retexture control | ✅ | `test:1067-1097` `mode="retexture"` + 403 ⇒ exit 1, AND the tail was entered (`1 call(s)` — the control is live, not vacuous), AND no multi-image narrative leaks | one test case (3 checks) |
| WARN-7 `--ai-model` help | ✅ | `:1899` adds `meshy-7` | one line; asserted from the real parser |

## 6. THE OTHER DIRECTION, THE DEFAULT, AND THE INSTRUMENT — quoted

```
=== 12. a NARROWED --views (Front,Side / Front / Side,Back) is exit 64, no network, no task ===
  [PASS] parse_views refuses 'Front,Side'  -- --views Front,Side drops Back. --mode multiimage always sends the CHAR
  [PASS] 'Front,Side': the refusal names every dropped view  -- Back
  [PASS] parse_views refuses 'Front'  -- --views Front drops Side, Back. --mode multiimage always sends the CHA
  [PASS] 'Front': the refusal names every dropped view  -- Side, Back
  [PASS] parse_views refuses 'Side,Back'  -- --views Side,Back drops Front. --mode multiimage always sends the CHAR
  [PASS] 'Side,Back': the refusal names every dropped view  -- Front
  [PASS] main() exits 64 for --views Front,Side  -- usage 64
  [PASS] the run is never entered from the CLI  -- 0 call(s) - must be 0
  [PASS] the CLI message names --mode image3d
  [PASS] _run_mode returns 64 for (Front, Side)  -- got 64
  [PASS] no task, no network for the narrowed set  -- 0 tail call(s), 0 wire call(s)
  [PASS] no state file was written for the narrowed set  -- asset dir empty or absent
  [PASS] a case-slipped view ('front') is refused AND the message names the exact spelling  -- ["  (view names are exact: 'front' is not 'Front')"]

=== 13. the OTHER direction: a 4-view superset and any order still PASS (the flag survives) ===
  [PASS] parse_views accepts a 4-view superset in the order given  -- ('Front', 'Side', 'Back', 'ThreeQuarter')
  [PASS] the 4th slot may come FIRST - the fence is a set check, not a prefix check
  [PASS] a permutation of the pinned three is accepted in the order given
  [PASS] a 4-view run returns 0  -- got 0
  [PASS] the 4-view run reaches the shared tail exactly once  -- 1 call(s)
  [PASS] four images are in the payload  -- 4
  [PASS] provenance records all four, in order  -- ['Front', 'Side', 'Back', 'ThreeQuarter']
  [PASS] each of the four image_urls[i] is the view at index i, byte for byte  -- sha256-matched 0..3
  [PASS] a permuted pinned set runs and keeps the REQUESTED order  -- rc 0, order ['Back', 'Front', 'Side']

=== 14. the DEFAULT (no --views) is still the pinned three and still exit 0 ===
  [PASS] main() with no --views returns 0  -- return 0
  [PASS] the run receives exactly the CHAR-2 pinned three, in order  -- ('Front', 'Side', 'Back')
  [PASS] --views Front,Side,Back (the default, spelled out) is identical  -- return 0 ('Front', 'Side', 'Back')

=== 15. the SUCCESS line carries the view COUNT and ORDER - read from the set SENT, never from argv ===
  [PASS] the offline tail returns 0  -- got 0
  [PASS] a SUCCESS line was printed  -- [meshy] SUCCESS: C:\Users\wesel\AppData\Local\Temp\task1088_
  [PASS] the SUCCESS line carries the view COUNT  -- Views sent: 3 (Front -> Side -> Back)
  [PASS] the SUCCESS line carries the view ORDER  -- Views sent: 3 (Front -> Side -> Back)
  [PASS] CONTROL (SC-94): argv says 4 views incl. 'Phantom'; the line reports the 3 that were SENT  -- Views sent: 3 (Front -> Side -> Back)
  [PASS] CONTROL: a 2-image body prints exactly 'Views sent: 2 (Front -> Side)' - the instrument follows the body  -- Views sent: 2 (Front -> Side)
  [PASS] CONTROL: the image3d SUCCESS line carries no Views clause (unchanged)  -- [meshy] SUCCESS: C:\Users\wesel\AppData\Local\Temp\task1088_12jsq8u1\C

=== 16. qa/TASK-1089 WARN-4/5/7: poll-404 narrative, retexture control, --ai-model help ===
  [PASS] CONTROL: --mode retexture still exits 1 on a 403 (shipped, unchanged)  -- got 1
  [PASS] the retexture tail was actually entered (the control is live, not vacuous)  -- 1 call(s)
  [PASS] no multi-image narrative leaks into retexture
  [PASS] a poll 404 is STILL exit 4 (safety unchanged)  -- got 4
  [PASS] a poll 404 does NOT say the endpoint refused the account
  [PASS] the verbatim body and the no-fallback sentence still print
  [PASS] CONTROL: a CREATE 404 still says the endpoint refused the account  -- rc 4
  [PASS] --ai-model help lists meshy-7 (the live enum)  -- Meshy model id: meshy-5, meshy-6, meshy-7, or latest (defaul
  [PASS] --views help names the fence (exit 64) and the image3d route
```
Section 15 drives the **real** `_finish_task_common()` (validate → commit → `state.json` → the `say()`), with only the remote calls stubbed (`fetch_balance`, `create_task`, `poll_task`, `download_file` writing a must-ACCEPT guard fixture to the `.part` path; `api_request` tripwired). The SUCCESS line under test is the one the operator will read.

## 7. LATENT, NOT LIVE — MEASURED, NOT ASSUMED (`SC-§90`, row cl. 6)

Nothing in this row is an incident. Read-only measurement of the real `Tools/ArtPipeline/Inbox/` this morning:
```
MainCharacter_Front.png          exists=True  is_file=True  is_dir=False
MainCharacter_Side.png           exists=True  is_file=True  is_dir=False
MainCharacter_Back.png           exists=True  is_file=True  is_dir=False
MainCharacter_Detail             exists=True  is_file=False is_dir=True     (16 entries)
MainCharacter_Detail.png         exists=False is_file=False is_dir=False
root-level MainCharacter_*.png: ['MainCharacter_Back.png', 'MainCharacter_Front.png', 'MainCharacter_Side.png']
```
⇒ the only view names that resolve are the pinned three. The 16 detail crops sit in a **subdirectory**, and `resolve_multiview_images()` uses `path.is_file()`, which rejects a directory; `MainCharacter_Detail.png` does not exist. So on the committed code `--views Front,Side,Detail` was **exit 5** (missing input) — a substitute view could not be reached without first authoring a new root-level `<CardID>_<View>.png`. The live TASK-1091 run's provenance agrees: `Cache/MainCharacter/state.json` → `meshy.inputs.view_count: 3`, `view_order: ["Front","Side","Back"]`. After this row the same command is **exit 64 before any file is probed** (Back dropped), and `--views Front,Side,Back,Detail` is exit 5 (Detail does not resolve) — both loud, neither a run.

And the row's ruling, kept: this was NOT solved by logging harder. `state.json` already carried `view_order`; it made a violation auditable. The fence makes it impossible; the SUCCESS-line note is the cheap mitigation the gate asked for on top, not a substitute.

## 8. FLAGGED DECISIONS FOR `TASK-1097` (⛔ where I am most likely to be wrong)

1. ⭐⭐ **The fence is on the deciding path too, and `_run_mode()` can now return 64.** `parse_views()` alone would fence the CLI, but `_run_mode()` is the path that actually sends, and the suite calls it directly — a guard only in `main()` would have left the deciding path unfenced (cl. 2's *"do NOT leave a bare count as the only thing standing"* read literally). One function, two call sites, no second copy (`SC-§39.1`). The cost: `_run_mode` returns `64` where previously only argparse did. Same shipped code, same meaning ("CLI usage"), not a tenth code — but it IS a new return site for it. Your call whether that reads as contract drift.
2. ⭐ **Exact-case identity.** `--views front,Side,Back` is refused (Windows would have resolved the file; Linux would not; provenance would have carried a non-canonical name). The message names the exact spelling so the refusal is actionable. If you rule this an `SC-§89`-shaped literal STOP on a correct state, the relaxation is one `.lower()` in `require_pinned_views` — I chose canonical names because `view_order` is provenance.
3. **The SUCCESS line's ORDER is provenance-side, its COUNT is body-side.** `len(payload["image_urls"])` is the request body `create_task()` posted — the same object. The names come from `inputs_block["view_order"]`, the resolved set that built the body (the payload carries data URIs, not names). Neither is `args.views` (the tell — asserted by the Phantom control and by MUTATION B). I did NOT re-derive the order from the body by hashing each data URI against `view_images[i].sha256` — that would be a stronger instrument, but it is not "one f-string" and test 1 already pins body order to request order by hash. ⚠️ **What it is NOT: a Meshy-side read-back.** The task object does not echo its inputs; the rung above this line is a live run's returned mesh (TASK-1091's, already taken).
4. **Two texts outside cl. (8) were changed because they had become lies:** the exit-5 message (`:997`) and the `--views` help (`:1886`) both told the operator to narrow with `--views` — now exit 64. Leaving either would have printed a dead route at the exact moment the operator needs a live one.
5. **The WARN-4 gate is the narrowest reading:** only the mis-attributing sentence is gated; the *"NOTHING fell back"* sentence still prints on a poll 404 because it is still true. I added no `else` narrative for the poll case (the verbatim `GET …/{task_id} (poll)` context line already names the actual call) — adding one would not have been a one-line change.
6. **Nothing of `TASK-1096` was done.** `write_success_state` / `write_failed_state` / `_require_inside_cache` untouched; the `asset_dir.mkdir()`-before-resolution NIT is untouched. No collision expected: my hunks are `:140`, `:874-1000`, `:1542-1620`, `:1608-1690`, `:1720-1800`, `:1811-1900` (pre-edit numbering).

## 9. WHAT THIS DOES NOT PROVE (`SC-§94` cl. B)

⛔ No credit was spent and no request left this machine. 110/110 green proves the fence refuses the narrowing direction at both entry points, accepts the widening direction, and that the SUCCESS line reports the body rather than argv — **offline, against doubles.** It does not prove Meshy's behaviour for a 4-image create (never exercised live by anyone; TASK-1091 ran the default three), and it does not prove the SUCCESS line's `Views sent:` agrees with what Meshy consumed — only with what was posted. The rung above is a live run reading its own returned mesh; TASK-1091 took it for the 3-view default on 2026-09-06 (`consumed_credits: 30`, `view_count: 3`).

**Notes for `TASK-1098`:** pathspec = the two `Tools/ArtPipeline/` files + this handoff + `qa/TASK-1097.md`; re-witness MUTATION A (expect 100/110 with `got 0` / `1 call(s)`), ⛔ **clear `__pycache__/meshy_generate*.pyc` before every run** (§4), restore from a scratch copy and compare to `b1d30cc…6c95`, never `git restore` (uncommitted); no C++ ⇒ no compile; `Cache/` + `Inbox/` stay gitignored; NEVER push.
