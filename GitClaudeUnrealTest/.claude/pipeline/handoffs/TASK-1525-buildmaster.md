# TASK-1525 — build-master handoff (commit D, the held-key repeat filter)

- Date: 2026-09-26
- Marker: `TASK-1525-SET-ACTIVE-REPEAT-FILTER-HOST-D`
- Commit: `0500d51` (`0500d5110eb9fa52a0fba4a6f5f599c922726051`), parent `cce9f6b` (commit C, `TASK-1520`).
- **LOCAL ONLY. NOT PUSHED.** main is **4 ahead / 0 behind** `origin/main` (`a35ba29`).
- Editor: not touched. PID 3108 (the GUI editor, by command line; `-game` count 0) was left running. No compile, no PIE, no MCP call.

## (0) Status flips

`TASK-1525` went to `in-progress — 5c COMMIT RUNNING` before the first `git add`, so that version of the row is inside the commit (grep on `HEAD:` = 1 hit). After the commit it went to `done` + hash.

## Gates re-read at my instant

| gate | read |
|---|---|
| `qa/TASK-1522.md` line 1 | `PASS` (0 BLOCKER · 0 WARN · 4 NIT) |
| `qa/TASK-1524-verify.md` line 1 | `Verdict: VERIFIED`, verify: partial (2/3 observable) |
| 5a `TASK-1523` | `Result: Succeeded`, 566/566 |
| The committed code is the built code | Source mtimes `.h` 22:40:28.983 · `.cpp` 22:41:03.386 · test 22:44:54.573 are all before the DLL at 23:03:41.639. The DLL re-hashed now = `e06ff68ca93a8edc9dad91b4f74e3084352d34dfc73e6ce09e02388494125db4`, which equals `TASK-1523` §2. |
| Row (2) STOP condition | `TASK-1480` is `backlog` (never dispatched). The source diff's hunks all sit in `TASK-1521`'s regions: header `:176` helper + the two adapter doc comments, `.cpp` adapter + helper, test `+370`. No foreign edit, so no half-staging question. |
| Commit C in | HEAD was `cce9f6b` and the index was empty before my stage. |

## (1) The derived pathspec: 11 files, verified on `git show --stat HEAD`

The git root is one level up (`C:/GitProjects/GitHub/GitClaudeUnrealTesting`). Every path below is under `GitClaudeUnrealTest/`.

```
.claude/pipeline/TASKBOARD.md                                                          |  14 +-
.claude/pipeline/handoffs/TASK-1520-buildmaster.md                                     | 106 +   (one-cycle lag)
.claude/pipeline/handoffs/TASK-1521-programmer.md                                      | 172 +
.claude/pipeline/handoffs/TASK-1523-buildmaster.md                                     | 123 +
.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1524-t00m35s-a1-deck4-set-active.png        | LFS
.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1524-t01m12s-a1-deck3-restored-active.png   | LFS
.claude/pipeline/qa/TASK-1522.md                                                       | 198 +
.claude/pipeline/qa/TASK-1524-verify.md                                                |  61 +
Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp                            |  51 +-
Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h                              |  39 +-
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp                     | 371 +
11 files changed, 1124 insertions(+), 17 deletions(-)
```

- Fence grep on the commit's name list (`CLAUDE.md`, `.claude/agents/`, `Tools/`, `Saved/`, `testvideo/`, `settings.local`, `CONVENTIONS.md`) returns **0**.
- Committed source blobs = working-tree blobs: `.cpp` `85a6d8c8…`, `.h` `7e243aa3…`, test `10654bd4…`.
- Staged `TASKBOARD.md` blob = working tree at commit (`dc8b2217…`), so there was no law-file race this time.
- Index checked twice for foreign paths (after `git add`, and right before `git commit`): 11 paths, 0 foreign. The UE Git plugin staged nothing.

## (2) HELD: nothing

At my instant, `CLAUDE.md`, `.claude/agents/*`, `CONVENTIONS.md` and `Tools/Verify/recipes/*` were all **clean**, so there was nothing to name and leave. `TASK-1527` has not started. The post-commit tree is clean except for my own flips and this handoff.

## (3) LFS: three-way hash, `VER-§4` cl. 1 (oid vs sha256, never size)

| target (`playtest-evidence/2026-09-26/`) | source (`Saved/AuraVerify/`) | sha256(source) = sha256(target) = oid in `HEAD` |
|---|---|---|
| `VER-TASK-1524-t01m12s-a1-deck3-restored-active.png` | `TASK-1524/a1_end_deck3_active_fullres.png` | `183195090ea6430780758e1c2c6c5dc4a108c91b173b51d46943d7119f45b4a9` |
| `VER-TASK-1524-t00m35s-a1-deck4-set-active.png` | `pie_composited_c1_t35.04s_f14471.png` | `89cbf87b9436d68281505f55749a549156afdce653e1ce99513ae1293cc757e3` |

- Copied with `cp -n`, never moved. Both sources are still in `Saved/`.
- The names are exactly the strings the report's `## Evidence (promoted)` lists. They were not renamed (`VER-§4` cl. 2, `VER-4-2-THE-TOOLS-OWN-STAMP-SATISFIES`). `a1` in the slug is acceptance line A1, not an attempt index, as with the `TASK-1494`/`1498`/`1512` frames. There was one attempt, so `-a<N>` is omitted.
- I viewed both frames with `Read`, and they match the report's descriptions. The `t00m35s` frame (1086 px) shows focus on deck4, **not** the orange outline. That caveat is in the commit message; the observable is the `BrushColor` read.
- `pie_composited_c2_t69.04s_f16453.png` was not promoted because the report does not cite it.

## (4) `SC-§103` flips, written after the commit, each with `0500d51`

`1521` `verified` → `done` · `1522` `done` + COMMITTED · `1523` `done` + COMMITTED · `1524` `verified` → `done` (the owed promotion marked DISCHARGED) · `1525` → `done`. I re-read each status line before its `Edit` and anchored on text unique to its own row. A grep-back shows all 5 carry `0500d51`. The board's post-commit delta is exactly those 5 lines, and it rides the next host (`TASK-1530`).

The message names passengers and references separately:
- **Passengers:** 1521, 1522, 1523, 1524, 1525.
- **References, not flipped:** 1520 (its handoff rides as the lag; the row is already `done` at `cce9f6b`), 1509, 1480, 1529, 1530.

## His A3 hand check: carried as an ACCOUNT, not measured here

The commit message carries, labelled as the orchestrator's account:
- His verbatim sentence.
- The orchestrator's log read: 12 refusal Warnings, one per press, spaced 0.25–8.5 s apart, zero bursts, in two sessions at 06:15:34 and 06:16:50 UTC.
- The label: *the filter held on his real key, BY LOG READ; his visual "no warning" is contradicted by the log file.*

I did not read the log myself. I did not edit the verify report (`VER-§8` cl. 3(c)). I did not merge the account into any verdict. It does not gate the commit. The manager records it on the rows at `TASK-1529`.

## Owed downstream (report only)

- **Manager, `TASK-1529` (now unblocked):** close `DECK-§9` cl. 10's W1 bullet with `0500d51` and label the two Slate doors' addresses with it. QA's N4 gives `:1874`/`:1896` at this revision; re-grep them, don't copy them. Record his A3 account beside the verdict.
- **Save hygiene (from `qa/TASK-1524-verify.md`, not mine):** the guest deck `.sav` bytes changed (`5bdd2fc0…` → `13f41048…`) with every field read equal. H1 (a header custom-version table reordered) is a hypothesis. Nobody has boarded it.
- **Recipe candidates** in the verify report's S3 (Menu Downs need spacing; `EditingDeckIndex` at open = the active deck's slot; return by `IA_MenuLeft`) are unboarded. They are the manager's to triage.
- **`TASK-1530`** carries this handoff (one-cycle lag) plus the 5 flips above.
- ⛔ NOT PUSHED. main is 4 ahead of `origin/main`.

## Not examined / limitations

- His A3 log read was not re-measured by this host. It is carried as an account.
- The suite was not re-run. 5a's 566/566 is taken from `TASK-1523`. The build-identity proof is mtimes before the DLL plus the DLL re-hash, not a rebuild.
- The committed `.cpp`/`.h` blobs were not compared to the bytes QA read (QA recorded no whole-file sha256). The line-ending and diff-shape checks agree with QA's §compile-shape: `numstat` 45/6, 35/4, 371/0, and no whole-file churn.
