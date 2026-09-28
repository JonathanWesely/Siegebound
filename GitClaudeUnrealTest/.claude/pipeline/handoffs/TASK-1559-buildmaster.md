# TASK-1559 — SHOWROOM-TRIO-DELETE-HOST — build-master handoff

- **Agent:** build-master, 2026-09-27. Marker `TASK-1559-SHOWROOM-TRIO-DELETE-HOST`.
- **Result: COMMITTED `ab57522`** (`ab57522a83cfde6193c882dffabdc9fbecb7354b`), parent `9a67a27`. ⛔ NOT pushed.
- **Status reached:** `TASK-1559` → `done` · `TASK-1557` → `done — COMMITTED ab57522 (host TASK-1559)` · `TASK-1558` → report COMMITTED · `TASK-1440` → `— remedy COMMITTED ab57522 (host TASK-1559)` appended. Reference, not flipped: `TASK-1538` (its (5c) was the integration check).

## 0. Blockers, measured at my instant

| Check | Reading |
|---|---|
| Gate | `qa/TASK-1558.md` line 1 `PASS` |
| (5c) | `handoffs/TASK-1538-buildmaster.md` §10 "(5c) `TASK-1557`'s integration check, on PID 34780 — **PASS**" |
| Live processes | `UnrealEditor.exe` 34780 (the relaunched GUI editor) + `UnrealTraceServer` 17896 only. No `Build`/UBT/dotnet process, so no compile live. No other commit host was dispatched. |
| HEAD / origin | `9a67a27` / `origin/main` = `a35ba29`; `rev-list --left-right --count origin/main...HEAD` = `0 6` |
| Index before staging | `git diff --cached --name-status` → empty |

Note: `origin/main` reads `a35ba29` at my instant, not the `40c824b` some older notes carry. I only read it; nothing was fetched or pushed.

## 1. Derivation (git root `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, `SC-§102`)

`git status --porcelain=v1 --untracked-files=all -- GitClaudeUnrealTest/Content/ GitClaudeUnrealTest/Config/ GitClaudeUnrealTest/GitClaudeUnrealTest.uproject`:
```
 D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.uasset
 D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.uasset
 D GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap
```
Control: the same `Content/` pathspec matches 4,773 tracked files (`git ls-files | wc -l`), so the three lines are a real reading and not a mis-anchored silence. No other `D`, and no `M` / `A`, under `Content/`; nothing under `Config/` or the `.uproject`. So row (1)'s STOP did not fire.

**The derived `D` set equals `TASK-1557`'s deletion set path for path** (3 tracked files; the 4th, `LVL_Showroom_BuiltData.uasset`, is ignored by `GitClaudeUnrealTest/.gitignore:120` `*_BuiltData.uasset` and was never tracked, so it is not named).

**`handoffs/TASK-1441-buildmaster.md`:** `git ls-files` returns it: it is **TRACKED**, last touched by `40c824b` ("trying to make everything agent usable"), and clean in the tree. Per the row's rider it did **not** ride.

## 2. Staging and the commit

Pathspec, the five paths only:
1. `GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.uasset` (removal)
2. `GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.uasset` (removal)
3. `GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap` (removal)
4. `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1557-art.md` (new), sha256 `e089685fce8e105e2c24b6338f7aa53535df7aa65f900063d0bff9ea0451430b`
5. `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1558.md` (new), sha256 `25c2f264906a87a825754f64c1bec9d3ee83a90b7da7f65b97d20f7663153c4b`

`git add -- <5 paths>`, then the index read exactly those five (`A` ×2, `D` ×3) immediately before `git commit -F <msg> -- <5 paths>`. After the commit the index is empty.

**`git show --stat HEAD`** (the verification, §25c):
```
 .../.claude/pipeline/handoffs/TASK-1557-art.md     | 336 +++++++++++++++++++++
 .../.claude/pipeline/qa/TASK-1558.md               | 157 ++++++++++
 .../Blueprints/BP_Basic_Movement.uasset            |   3 -
 .../Blueprints/GM_TestGamemode.uasset              |   3 -
 .../sA_ArcheryVfxPack/Levels/LVL_Showroom.umap     |   3 -
 5 files changed, 493 insertions(+), 9 deletions(-)
 create mode 100644 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1557-art.md
 create mode 100644 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1558.md
 delete mode 100644 GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement.uasset
 delete mode 100644 GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Blueprints/GM_TestGamemode.uasset
 delete mode 100644 GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/Levels/LVL_Showroom.umap
```
Exactly 3 `delete mode` + 2 text files, nothing else. (Each deletion is 3 lines because the tracked blob is an LFS pointer.)

**`git ls-files -- GitClaudeUnrealTest/Content/sA_ArcheryVfxPack/`: 76 → 73** (= 76 − 3).

## 3. The revert path, checked

The three deleted pointers at `ab57522~1` carry these LFS oids, and each equals `TASK-1557`'s pre-delete sha256. Each object is **present in the local LFS store** (`.git/lfs/objects/…`), and its re-hash equals the oid:

| File | oid (= sha256) | size |
|---|---|---|
| `BP_Basic_Movement.uasset` | `609dad8e8660483ae59c4a2d63893228145d3b6a2cad0f9eba249accc65b2170` | 75,510 |
| `GM_TestGamemode.uasset` | `ad6a8eef4b87046982f13d3c4e5bcd6e9e5011f54c1e567b02ff493558b49d65` | 17,212 |
| `LVL_Showroom.umap` | `51833d6e0f7b392925e07268b66e923815c422dcc1d7e852467be1fefa8dfec4` | 179,816 |

So `git revert ab57522` (or `git checkout ab57522~1 -- <the three paths>`) restores the showroom byte-exact without a network fetch. The BuiltData is not restored by git (never tracked); building lighting on the map regenerates it. Not exercised: I did not run a revert.

## 4. Message

Subject: `TASK-1559: delete the archery pack's demo trio (showroom map, its game mode, BP_Basic_Movement) in a commit of its own, so one git revert restores the showroom`. The body quotes his `TASK-1440` choice, "Delete the pack's demo trio", states the final state as 4 of 4 deleted (QA NIT-1: the handoff header still says `blocked`; `## Resume` is final), says the pack's effects stay (73 files byte-identical), and gives the revert and checkout lines. It ends with the `Co-Authored-By` trailer.

## 5. Ahead count and origin

Before: `0 6` vs `a35ba29`. After: `0 7` vs `a35ba29`. Origin unchanged. ⛔ Nothing pushed.

## 6. Left dirty after the commit (NAMED-AND-LEFT, none of them this row's cargo)

Modified:
- `GitClaudeUnrealTest/.claude/agents/playtest-verifier.md` (agent file, never rides a code commit; H or its own row)
- `GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md` (G)
- `GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md` (G; now also carries this row's flips, below)
- `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp`, `DeckBuilderWidget.h`, `SiegeControlsHelpWidget.cpp`, `SiegeControlsHelpWidget.h`, `SiegeMenuInputSubsystem.cpp`, `SiegeMenuInputSubsystem.h`, `SiegePlayerController.cpp`, `Tests/SiegeControlsHelpTest.cpp`, `Tests/SiegeMenuInputTest.cpp` (G)
- `GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md`, `RCP-deckbuilder-slot-and-card-edit.md`, `RCP-menu-to-deckbuilder.md`, `RCP-vsbot-capture-center-and-summon.md`, `README.md` (H)

Untracked:
- `handoffs/TASK-1439-programmer.md`, `TASK-1443-programmer.md`, `TASK-1445-programmer.md`, `TASK-1480-programmer.md`, `TASK-1536-buildmaster.md`, `TASK-1538-buildmaster.md`, `TASK-1541-programmer.md`, `TASK-1548-programmer.md`, `TASK-1554-programmer.md`
- `qa/TASK-1404-verify.md`, `TASK-1481-loop1.md`, `TASK-1481.md`, `TASK-1493-verify.md`, `TASK-1499-verify.md`, `TASK-1546-loop1.md`, `TASK-1546.md`, `TASK-1549-loop1.md`, `TASK-1549.md`, `TASK-1555.md`
- and now this file, `handoffs/TASK-1559-buildmaster.md`.

`CLAUDE.md` was clean: not authored, not staged.

## 7. The lag (next host)

**G (`TASK-1540`) has not derived** (its status is `backlog` at my instant), so this handoff and my `TASKBOARD.md` flips are **G's lag**. `TASK-1540-AMENDED-3-2026-09-27` already says so: `handoffs/TASK-1559-buildmaster.md` is a second expected untracked host handoff beside `TASK-1536`'s, so G stages both and it is not a finding.

The `TASKBOARD.md` hunks I wrote, for G's read-back:
- row `1559` status line (`done`, with the prior `in-progress — 5c COMMIT RUNNING` kept as "Was:");
- row `1557` status line (prefixed `done — COMMITTED ab57522 (host TASK-1559)`, the (5c) text kept as "Was:");
- row `1558` status line (appended "— report COMMITTED `ab57522` (host TASK-1559)" + the 1441-tracked note);
- row `1440` status line (appended "— remedy COMMITTED `ab57522` (host TASK-1559)").

## 8. Findings for the manager (not blockers)

- `origin/main` is `a35ba29`, not `40c824b`. Something moved origin since the older notes were written (probably his own push). I did not investigate.
- QA's open items stand, untouched by this row: WARN-1 (the write tool re-executes a script after a force-delete, 3/3), WARN-2 (ratify the orchestrator's double-run ruling on `TASK-1557`), NIT-3 (the kept list counted 5 other Blueprints; there are 7), and the empty `Content/sA_ArcheryVfxPack/Levels/` directory left on disk (git does not track it).

## Not examined / limitations

- No editor query. The registry and `BS_ERROR` state rest on `TASK-1538` (5c) and `TASK-1557`; this row only read git and the file system.
- I did not execute a revert; §3 checks that its inputs (the oids and local LFS objects) are present and correct.
- The LF→CRLF warnings on the two text files are git's normal `core.autocrlf` notice; the committed blobs are what `git add` normalised.
