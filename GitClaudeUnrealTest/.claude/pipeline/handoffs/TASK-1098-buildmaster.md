# TASK-1098 — [HARDENING-SHIP] handoff (build-master)

**Marker:** `TASK-1098-HARDENING-SHIP` · **Law:** `SC-§68` · `SC-§83` · `SC-§87` · `SC-§94` cl. B · `TL-§5d` · `TL-§5e` cl. 7 · `§25b` (cited, not restated)
**Hosted:** `TASK-1095` [VIEWS-FENCE] + `TASK-1096` [CACHE-FENCE] · **Gate:** `qa/TASK-1097.md` PASS (0 BLOCKER · 1 WARN · 5 NIT)
**Commit:** `236b0a8` (`236b0a87c664be9fe1280c9dd6b7a8afa9ceb806`) on `main`, **NOT pushed** — main is 3 ahead of `origin/main` (`3e2b87e`, `e12907c`, `236b0a8`).
**This file is OUTSIDE its own commit by construction** (`TL-§5e` cl. 7 — it records the hash above). It is the one expected untracked build-master handoff; the next doc-host sweeps it. ⚠️ The row's `- host-checklist:` (c) lists this file in the pathspec — that clause conflicts with `TL-§5e` cl. 7 and with the dispatch brief; the law was followed. Manager: strike it from (c).

---

## 0. THE ONE FINDING FIRST — THE SHARED INDEX WAS WRITTEN BY SOMEONE ELSE BETWEEN VERIFY AND COMMIT

My first commit, `749aece`, contained **8 files, not 5.** Three of `TASK-1093`'s un-gated assets — `Content/Characters/SK_MainCharacter.uasset`, `Content/Textures/T_MainCharacter_D.uasset`, `Content/Textures/T_MainCharacter_N.uasset` (LFS pointers, 3 lines each) — were in it. They were NOT in the index when I verified it: `git diff --cached --name-status` printed exactly five paths, every worktree-vs-index diff was empty, and both subject blobs matched `git hash-object`. Something staged those three files into the shared `.git/index` between that verification and `git commit -F`, and `git commit` with no pathspec commits the index as it stands.

**What I did:** `749aece` was local and unpushed. `git reset --soft HEAD~1` (HEAD back to `e12907c`), then `git reset -q` (mixed — index back to HEAD, working tree untouched: the strays returned to `??`, no file deleted or modified), `git add` of the five paths, a gated re-verification (abort unless the staged set is exactly five and every worktree-vs-index diff is empty), then **`git commit -F <msg> -- <the five paths>`** — a pathspec commit, which records ONLY the named paths whatever else the index holds. Verified on the COMMIT (`git show --stat HEAD`): 5 files, 1424 insertions, 30 deletions. `749aece` survives only in the reflog.

**The writer is still active.** At the instant of the redo, `git diff --cached` (before the reset) showed two MORE foreign adds that had landed since `749aece`: `Content/Materials/Instances/MI_MainCharacter_PBR.uasset` and `Content/Textures/T_MainCharacter_ORM.uasset`. All five are freshly imported `TASK-1093` assets; `UnrealEditor` PID 20564 has been up since 08:24 with other agents live in it. The likeliest hand is the editor's Git source-control plugin marking new assets for add on import — I hold no instrument that proves it (no process attribution on index writes), so it is a hypothesis, not a diagnosis.

**Binding for every host today (`TASK-1085`, `TASK-1094`) until the writer is found and switched off:** commit by `git commit -F <msg> -- <pathspec>`, never by "commit the index"; verify the COMMIT's file list (`git show --stat HEAD`) AFTER, not only the index BEFORE. `§25b`'s index verification is necessary and — measured today — not sufficient when another process holds the same index. Routed to the manager in 🚨 Blockers (`p1788713011295469`).

---

## 1. WHAT SHIPPED (the two rows, one commit)

| row | what `236b0a8` carries |
|---|---|
| `TASK-1095` | `require_pinned_views()` — one definition, membership by name (`:899`), called from `parse_views()` (CLI) and `_run_mode()` (deciding path); `--views Front,Side,ThreeQuarter` refused, exit 64 at both entry points, 0 tail calls, 0 wire calls, no state file; the `len(resolved) <` count-warn DELETED; 4-view superset / permutation / default still run; SUCCESS line carries `Views sent: N (order)` read from the POSTED body + provenance, never argv; WARN-4/5/7 one-liners |
| `TASK-1096` | `_require_inside_cache()` (existing, one definition `:715`) gains three call sites — `_run_mode:1645` before the mkdir (exit 1 at the gate), `write_success_state:1178` (before `_load_state()`), `write_failed_state:1192`; GLB path byte-untouched. **PRE-EXISTING defect, all three modes, NOT introduced by the multi-view diff** — stated in the commit's first line and body |

Commit message: `git log -1 --format=%B 236b0a8`. The attribution trailer is the required pair (`Co-Authored-By: Claude Fable 5.1` + `Claude-Session`).

## 2. PATHSPEC (derived from the two WRITES lists + the gate report; `git status` was never the source)

```
Tools/ArtPipeline/meshy_generate.py                    M   +130 / -26 (sum of 1095 + 1096)
Tools/ArtPipeline/test_meshy_multiimage.py             M   +699 / -4
.claude/pipeline/handoffs/TASK-1095-programmer.md      A   216 lines
.claude/pipeline/handoffs/TASK-1096-programmer.md      A   228 lines
.claude/pipeline/qa/TASK-1097.md                       A   151 lines
```
NOT staged, by name: `Tools/ArtPipeline/pipeline_manifest.json` (TASK-1091's, ratified, rides on TASK-1094) · `Cache/**` · `Inbox/**` · `__pycache__/**` (all gitignored, verified `.gitignore:25-28,68`) · `Content/Maps/L_Arena.umap` (not dirty; never touched) · TASKBOARD/CONVENTIONS (the row's records rule did not name them).

## 3. HASHES — my own, taken BEFORE mutating (`SC-§68`, sha256 never size)

| file | working tree at dispatch (= scratch GOLD copy, `cmp` clean) | matches |
|---|---|---|
| `meshy_generate.py` | `ef5589afc5a2a925611ed66c3ea8eed67266e1439f1c25b9469447fcec47aeca` | 1096 §1 FINAL GOLD ✔ (not 1095's intermediate `b1d30cc…6c95`) |
| `test_meshy_multiimage.py` | `54fb4d102e5a55b33be3ac8490130b5ae5feb4cc45bf536bf8aaa5788af39159` | 1096 §1 FINAL GOLD ✔ (not `7d3c4b5b…9606`) |

The tree had not moved. Mutation targets confirmed exactly-once by grep before any edit: `:899` (A) and `:1645` (D — the only `_require_inside_cache(asset_dir)`; the other five sites pass `dest`/`target`).

## 4. SUITE, EXECUTED (bounded, `SC-§87`)

Invocation, every run: `cd Tools/ArtPipeline && rm -f __pycache__/meshy_generate*.pyc __pycache__/test_meshy_multiimage*.pyc && timeout 300 uv run python -B test_meshy_multiimage.py` (custom `check()` ledger — pytest is not in the venv and was not added). The runner printed the pyc census before and after each run: empty both times, every time (`-B` honoured; the 1095 §4 same-size/same-second trap could not fire). Bound never fired; each run ~1 s. Python 3.12.13 in `.venv`.

Tail, identical on green runs #1 (GOLD, before mutating), #2 and #3 (final bytes, after both restores):
```
========================================================================
169/169 assertions passed.
ALL GREEN - every requested view reaches the multi-image endpoint in order, a missing view stops before the network, a refused endpoint never becomes a quiet single-image run, and --views can add a view but never drop Front, Side or Back - and a state file can never land outside Cache/.
SUITE EXIT: 0
```
(`PASS=169 FAIL=0`, rc 0, all three.)

## 5. THE REDS, RE-WITNESSED ON THE FINAL BYTES (`SC-§83`)

### MUTATION A — `:899` `absent = tuple(v for v in MULTIVIEW_DEFAULT_VIEWS if v not in views)` → `absent = MULTIVIEW_DEFAULT_VIEWS[len(views):]  # MUTANT A` (1 of 1; CRLF count 2012 → 2012)
**159/169, suite exit 1, 10 FAIL** — the ten labels the handoff and the gate predicted:
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
```
The evidence pair: **`got 0` beside `1 call(s)`** — a Back-less knight generated and reported as success. And section 12's `Front,Side` / `Front` / `Side,Back` refusals stayed `[PASS]` under the mutant: the count floor is green on the easy half and blind to the row's case.

### MUTATION D — `:1645` `_require_inside_cache(asset_dir)` → `pass  # MUTANT D` (1 of 1)
**148/169, suite exit 1, 21 FAIL** — per shape `returns 1 … got 5` + `escaped path was not created` + `refusal names the fence` (6 shapes × 3 = 18), `NOTHING appeared` on the two shapes whose target is first created (2), `main() … return 5` (1):
```
  [FAIL] the row's shape, backslashes: _run_mode returns 1 (...)  -- got 5
  [FAIL] the row's shape, backslashes: NOTHING appeared anywhere under the sandbox (outside Cache/ or inside it)  -- new: ['deep\\Content', 'deep\\Content\\X']
  [FAIL] an ABSOLUTE path: _run_mode returns 1 (...)  -- got 5
  [FAIL] an ABSOLUTE path: NOTHING appeared anywhere under the sandbox (outside Cache/ or inside it)  -- new: ['Content', 'Content\\X']
  [FAIL] main() returns 1 for the row's shape - a return, not a traceback  -- return 5
```
The evidence pair: **`got 5` beside a directory outside `Cache/`** — created on an exit-5 path that never reached a writer (both writer guards stayed green under D).

### Restores — from the SCRATCH copies, never `git restore`
After A: pre-restore `b08ed9b2…25b8` → post-restore `ef5589af…aeca` (`== FINAL GOLD: True`), test file `54fb4d10…9159` (`True`), `cmp` byte-identical to scratch GOLD, pycache cleared.
After D: pre-restore `bb1819f5…7a68` → post-restore `ef5589af…aeca` (`True`), `54fb4d10…9159` (`True`), `cmp` clean, pycache cleared.
Re-hashed after green #3: both still FINAL GOLD. Those are the bytes in `236b0a8` (index blob == `git hash-object` of each file: `6d4e0bee…` / `d00f1d49…`).

## 6. `--check` (row cl. 2, bounded)

`timeout 120 uv run meshy_generate.py --check` → **exit code 0**, 2 s (bound never fired). Degeneracy-guard control passed (11 fixtures, control goes red when the guard is broken); key resolved from process env `MESHY_TOKEN` and redacted by the tool; four free reads — balance OK (**3170 credits**; `3e2b87e` recorded 3200 and TASK-1091 spent 30 ⇒ consistent), retexture / image-to-3d / multi-image list endpoints all reachable (the multi-image list shows TASK-1091's 1 task). **Zero credits spent on this row.** No secret appears in any log, excerpt or this note.

## 7. INDEX VERIFICATION (`§25b`), FOR THE COMMIT THAT STANDS

- Staged set = exactly the five paths (`git diff --cached --name-status`: 3 `A`, 2 `M`).
- `git diff -- <path>` empty for all five (worktree == index).
- Subject blobs: index oid == `git hash-object` of the on-disk file, both files (`core.autocrlf=true`; the test file is LF on disk and git's "LF will be replaced by CRLF" warning is the pre-existing one 1095 §1 recorded — the stored blob is LF, identical to the file).
- `.gitattributes` (git root): `git check-attr filter diff merge` on all five = `unspecified` — no LFS filter on any staged path; LFS not involved.
- Committed set verified AFTER on `git show --stat HEAD`: 5 files, +1424 / −30. Index equals HEAD afterwards (`git diff --cached` empty).
- Compile: none — zero C++ in either diff. Editor/MCP never touched.

## 8. SCOPE, THE TREE HALF (row cl. 7, `SC-§89` rule 2 named set)

`git status --short Tools/ArtPipeline/` at dispatch: `M meshy_generate.py` · `M pipeline_manifest.json` · `M test_meshy_multiimage.py` — exactly the named three-line healthy state (`pipeline_manifest.json` = TASK-1091's, not staged). After the commit: `M pipeline_manifest.json` only. No `Source/**` dirty. No path outside the two subject files carries these rows' bytes.

## 9. WHAT REMAINED DIRTY AFTER `236b0a8`, AND WHOSE

| path | state | owner / host |
|---|---|---|
| `.claude/pipeline/TASKBOARD.md` | `M` | shared — other rows' in-progress lines + my one status-line Edit |
| `.claude/pipeline/CONVENTIONS.md` | `M` | manager's (other lanes' law edits) |
| `.claude/pipeline/playtest-evidence/2026-09-06/RosterSheet_Trees.png` | `M` | TASK-1083 (host 1085) |
| `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset`, `M_Pack1_Trunk_Mobile.uasset` | `M` | TASK-1083 D1 (host 1085) |
| `Tools/ArtPipeline/pipeline_manifest.json` | `M` | TASK-1091 (rides on 1094) |
| `.claude/pipeline/handoffs/TASK-1083-buildmaster.md` | `??` | TASK-1083 — an OPEN lane (`blocked-with-question`), not a finished host's orphan ⇒ NOT swept under `SC-§86`; travels with 1083's materials on 1085 |
| `.claude/pipeline/handoffs/TASK-1091-artist.md` | `??` | TASK-1091 (host 1094) |
| `playtest-evidence/2026-09-06/MainCharacter_*.png` (12) | `??` | TASK-1091/1093 (host 1094) |
| `playtest-evidence/2026-09-06/TASK-1083-D-{BEFORE,AFTER,SIDE-BY-SIDE}.png` | `??` | TASK-1083 (host 1085) — appeared mid-run |
| `Content/Characters/SK_MainCharacter.uasset` · `Content/Materials/Instances/MI_MainCharacter_PBR.uasset` · `Content/Textures/T_MainCharacter_{D,N,ORM}.uasset` | `??` | TASK-1093 (host 1094) — **the five that a foreign process staged; unstaged by me, files untouched** |
| `Content/RawAssets/MainCharacter.fbx` · `Content/RawAssets/Characters/MainCharacter.fbx` · `Content/RawAssets/Concepts/MainCharacter.png` · `Content/RawAssets/Textures/MainCharacter/` | `??` | TASK-1091/1093 (host 1094) |
| `.claude/pipeline/handoffs/TASK-1098-buildmaster.md` | `??` | this note — outside its own commit by `TL-§5e` cl. 7; next doc-host |

## 10. NOT THIS HOST'S — carried, not dropped

- **WARN-1** (`qa/TASK-1097.md` §5): `test_meshy_multiimage.py:737` and `:805-809` — the "names the dropped view" asserts are vacuous for any single dropped view because the refusal's second sentence always lists `(Front, Side, Back)`; fix = assert on `msg.splitlines()[0]`, one line each; the GUARD at `:903` is correct. A code edit ⇒ a programmer row; folds into the next row that opens the test file (row checklist (e)). Not fixed in `236b0a8`.
- The gate's optional follow-ups: NIT-2 (`.` CardID resolves to `Cache/` root, permitted) and the `SC-§89` instance-5 "closed by TASK-1095" law note — manager's.
- **The index-race writer** (§0) — needs a finding of WHO writes the index; if it is the editor's Git SCC plugin, that is a Jonathan/manager setting, not a host action.

## 11. WHAT THIS DOES NOT PROVE (`SC-§94` cl. B)

No multi-view generation ran on this row; no credit was spent; no request beyond the four `--check` reads left this machine. `236b0a8` proves, offline and against doubles, exactly what the gate said it would: the fence refuses the narrowing direction at both entry points and accepts the widening one; the SUCCESS line reports the POSTED body; no state file or asset directory can be created outside `Cache/` from any of the three modes. A 4-image create has never been exercised live by anyone; TASK-1091's default-three run remains the only live evidence, and this commit does not close that lane.
