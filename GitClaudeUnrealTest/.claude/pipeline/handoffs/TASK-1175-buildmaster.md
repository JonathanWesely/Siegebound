# TASK-1175 — [FOGGREY-SHIP] — build-master handoff

⚠️ **THIS FILE IS BORN OUTSIDE ITS OWN COMMIT.** `74baab3` was sealed before this file existed, so it is
**not in it**. It is an untracked orphan on disk until a later row hosts it — exactly the defect
`TASK-1158-buildmaster.md` carried for a day, which is why that row's status now says so out loud.

**Commit `74baab3e7cc25ce3f246a217fe57270cbbbbf7a2` · `main` 7 ahead of `origin/main` · ⛔ NOT PUSHED.**

---

## 0. ORDER — the dispatch's load-bearing sequence, held

1. **Art committed FIRST** (`74baab3`) — `Content/` was dirty with two `.uasset`s and a revert run beside
   dirty art is how art is lost.
2. **THEN the code reverted** — `Source/**` returned to `20c1bea`.
3. **THEN recompiled**, because 🧑 Jonathan playtests the **binary**, not the tree.

⛔ Never reversed. At no point were both `Content/` and `Source/` dirty together after the commit.

---

## 1. ⭐ THE READER, CONTROLLED BEFORE ANY VERDICT WAS TRUSTED

`TASK-1149`'s comparator was fooled by `SC-§102`'s repo-root trap. **The git root is one level above the
project directory** — `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, not `.../GitClaudeUnrealTest`. Every
pathspec below is anchored `GitClaudeUnrealTest/...`.

| # | control | result |
|---|---|---|
| **C0** | **anchoring** — `git cat-file -p :Content/Blueprints/BP_SiegeFog.uasset` (**mis**-anchored, project-relative) | ⛔ `fatal: path ... does not exist` — **errors, does not silently return a stale pointer** |
| **C1** | the index really holds an **LFS pointer**, not the binary | `version https://git-lfs.github.com/spec/v1` + `oid sha256:...` + `size` ✅ |
| **C2** | comparator **positive** — `BP` oid vs `BP` file sha256 | **MATCH** ✅ |
| **C3** | comparator **negative** — `BP` oid vs **`MI`** file sha256 (deliberately crossed) | **MISMATCH** ✅ ⇒ **it discriminates; it does not answer MATCH to everything** |
| **C4** | would **size** have caught it? | `MI` disk size **7,288 B** == pointer size **7,288 B** ⇒ ⛔ **a size check passes on a stale blob.** `MI_SiegeFog_Grey` already changed content at an identical 7,288 B earlier in this lane. |

⇒ ⛔ **Verified by oid-vs-sha256 throughout. NEVER by size.** This discharges the check `TASK-1167` §7
recorded as **owed** and `TASK-1172` §13(6) inherited to this row.

---

## 2. THE PATHSPEC + EXISTENCE CHECK (`SC-§106`), WITH ITS MISSING CONTROL

Set derived from **disk at my own instant** (`SC-§91`), not from the dispatch's list.

```
EXISTS        38595 B  GitClaudeUnrealTest/Content/Blueprints/BP_SiegeFog.uasset
EXISTS         7288 B  GitClaudeUnrealTest/Content/Materials/Instances/MI_SiegeFog_Grey.uasset
EXISTS        42266 B  .../handoffs/TASK-1172-artist.md
EXISTS        31056 B  .../handoffs/TASK-1165-programmer.md
EXISTS        94101 B  .../handoffs/TASK-1165-reverted.patch
EXISTS        35662 B  .../handoffs/TASK-1167-buildmaster.md
EXISTS        26901 B  .../handoffs/TASK-1154-artist.md
EXISTS        15467 B  .../handoffs/TASK-1158-buildmaster.md        <-- the known orphan
EXISTS        49710 B  .../qa/TASK-1166-report.md
EXISTS      8366104 B  .../TASKBOARD.md
EXISTS      2848902 B  .../CONVENTIONS.md
MISSING             -  .../qa/TASK-1165.md                          <-- NEGATIVE CONTROL
```

🚨 **The negative control fired.** `qa/TASK-1165.md` — named by an older list — **has never existed**. Had
I staged it blind, `SC-§102` means it would have contributed **nothing, silently**, while
`git show --stat` read **green for all 36 other files**. The real report is **`qa/TASK-1166-report.md`**.

### 2a. ⚠️ DISK BEAT THE LIST (`SC-§91`) — one count in the dispatch was low
| prefix | dispatch said | **disk** | shipped |
|---|---|---|---|
| `TASK-1172rev2-*` | **6** | **7** | **7** |
| `TASK-1167rev2-*` | 8 | 8 | 8 |
| `TASK-1167-{A,B,C,D}-*` | 4 | 4 | 4 |
| `TASK-1172-*` (rev-1, incl. `BEFORE-vs-AFTER`) | 1 named | 7 | 7 |

`TASK-1172` §R12's own evidence table also names **7** rev-2 frames, so disk and the artist agree against
the dispatch. **26 evidence PNGs shipped.**

### 2b. Orphan sweep — nothing left behind
Every untracked path repo-wide was diffed against my staging set. **Zero unclaimed.** `TASK-1158`'s
handoff was the known orphan and is **in the commit**.

### 2c. Fences — asserted against the **commit**, not the index
`git show --pretty="" --name-only HEAD | grep -E 'Source/|Config/|Tools/|L_Arena|FogArea|testvideo'`
⇒ **no matches.** ⛔ Nothing fenced was committed.

---

## 3. LFS OID TABLE — all 28 binaries, index oid vs working-file sha256

| asset | oid (= sha256 of the working file) | size |
|---|---|---|
| **`Content/Blueprints/BP_SiegeFog.uasset`** | `411a8bc0859b99175cfe0e06e00fce4d50275b3f9abe60106423a273c1d19271` | **38,595 B** |
| **`Content/Materials/Instances/MI_SiegeFog_Grey.uasset`** | `554154c4c5564d0f494e2551b9135ccb615ce75a6d49b3f931503ebe1df33709` | **7,288 B** |
| 26 × `playtest-evidence/2026-09-08/*.png` | all verified individually | 58 KB – 5.26 MB |

**28 / 28 OK.** Both `.uasset` oids match `TASK-1172` §R7's declared hashes exactly ⇒ **rev-2 wrote zero
bytes**, confirmed independently of the handoff's own claim.

⭐ **Re-verified against the COMMIT afterwards, not just the index** — `git show HEAD:<path>` oid vs disk
sha256 ⇒ **MATCH on both**. (The UE Git plugin auto-stages; the index is never the evidence.)

---

## 4. `git show --stat HEAD`

```
commit 74baab3e7cc25ce3f246a217fe57270cbbbbf7a2
Author: Jonathan Wesely <wesely.jonathan@gmail.com>
Date:   Wed Sep 9 00:27:04 2026 -0700

    TASK-1175: the fog colour ships on a phase-mean gate - the same unchanged bytes clear the
    floor on 78 % of frames and fail it on 22 %, so a single frame never had a determinate
    verdict to give (TASK-1172 rev-2, FOG-§12.5c / FOG-§12.8a)

 .claude/pipeline/CONVENTIONS.md                              |  366 +++++-
 .claude/pipeline/TASKBOARD.md                                |  315 ++++-
 .claude/pipeline/handoffs/TASK-1154-artist.md                |   30 +-
 .claude/pipeline/handoffs/TASK-1158-buildmaster.md           |  278 +++++
 .claude/pipeline/handoffs/TASK-1165-programmer.md            |  229 ++++
 .claude/pipeline/handoffs/TASK-1165-reverted.patch           | 1251 ++++++++++++++++++++
 .claude/pipeline/handoffs/TASK-1167-buildmaster.md           |  401 +++++++
 .claude/pipeline/handoffs/TASK-1172-artist.md                |  612 ++++++++++
 .claude/pipeline/playtest-evidence/2026-09-08/  (26 PNGs)    |    3 + each
 .claude/pipeline/qa/TASK-1166-report.md                      |  282 +++++
 Content/Blueprints/BP_SiegeFog.uasset                        |    4 +-
 Content/Materials/Instances/MI_SiegeFog_Grey.uasset          |    2 +-
 37 files changed, 3812 insertions(+), 36 deletions(-)
```

**37 paths staged, 37 in the commit.** ⛔ No stray. Nothing to soft-reset.

---

## 5. THE REVERT — PROVEN BY BLOB OID IDENTITY, NOT BY A TEXT CENSUS

⭐ **Why oids and not a census:** a `grep -c $'\r$'` reported **1,305 CR lines** on a blob holding **zero**
CR bytes earlier in this lane. **A text census is not evidence about bytes.**

| file | target `20c1bea` | worktree after | index after | |
|---|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | `7e5cce4d3…` | `7e5cce4d3…` | `7e5cce4d3…` | ✅ **IDENTICAL** |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | `d16aeb25b…` | `d16aeb25b…` | `d16aeb25b…` | ✅ **IDENTICAL** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` | `fe861614b…` | `fe861614b…` | `fe861614b…` | ✅ **IDENTICAL** |

**Independent cross-check:** `git diff 20c1bea -- GitClaudeUnrealTest/Source/` ⇒ **EMPTY**.

### 5a. `git apply --check` — passed in **both** directions
| when | command | result |
|---|---|---|
| **BEFORE** the revert | `git apply --check --reverse TASK-1165-reverted.patch` | ✅ — proves the tree held **exactly** the archived bytes |
| **AFTER** the revert | `git apply --check TASK-1165-reverted.patch` | ✅ — proves the bytes remain **recoverable** |

⭐ **And the archive is provably the right one:** the patch's own `index` headers —
`7e5cce4..d21a54b` · `d16aeb2..2eef5ae` · `fe86161..1cb22f9` — **match the measured pre-revert worktree
oids** (`d21a54b20…`, `2eef5aee6…`, `1cb22f935…`) and the `20c1bea` targets. Patch is **94,101 B / 1,251
lines**, as declared, and is **committed** in `74baab3`.

---

## 6. COMPILE — `Result: Succeeded`, parsed from the LOG

⛔ **`Build.bat` returned exit code `0`, and that is NOT evidence** — it returns 0 on a failed build.
The log is the authority.

```
Result: Succeeded          <-- the only "Result:" line in the log
Total execution time: 16.07 seconds
Output binary: C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
```
- `Result: Failed` / `error C####` / `fatal error` ⇒ **0 occurrences**
- `0x800711C7` (Smart App Control) ⇒ **0** — not the failure mode of the day
- ⭐ **12 translation units + `UnrealEditor-GitClaudeUnrealTest.lib`/`.dll` genuinely relinked** ⇒ the
  revert **moved the binary**. It was **not** a no-op, which is the thing a "success" line alone cannot
  tell you.

🚨 **`COOKED TARGET NOT COMPILED`** (`SC-§111`). Editor target only. A packaged/cooked build of this state
does not exist and no claim is made about one.

---

## 7. SUITE — **554 / 0**, exactly as predicted

| | |
|---|---|
| `Result={Success}` | **554** |
| `Result={Fail}` | **0** |
| `Result={Error}` | **0** |
| `Test Started` / `Test Completed` | **554 / 554** ⇒ none started-and-lost |
| distinct `Result={...}` values in the whole log | **only `{Success}`** (`SC-§104`: state, not a tally) |
| engine's own exit | `**** TEST COMPLETE. EXIT CODE: 0 ****` |

⭐ **Reader control:** a bogus pattern `Result={ThisValueCannotExist}` returned **0** while
`Result={Success}` returned **554** ⇒ the grep discriminates and is not matching everything.

**Reconciliation:** `556 − 2 = 554`. `TASK-1165` declared **+2** tests in `SiegeFogVisualTest.cpp`; they
went **with the revert**. ⇒ **The suite figure is itself a measurement that the code route contributed
zero.** Runner: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi
-unattended -nopause -nosplash -NoLiveCoding -log -abslog=…`, wrapped in `timeout --signal=KILL 1500`.
⛔ Not killed; wall clock ≈ 40 s.

---

## 8. FENCES

| Fence | Result |
|---|---|
| **`Content/Maps/L_Arena.umap`** (`GFX-§11`) | ✅ `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — **UNMOVED at 3 checkpoints**: ① pre-commit ② post-commit ③ post-compile + post-suite. **Never saved.** |
| Editor | ⛔ **never launched.** Down at row start (`TASK-1172` killed it), down at the end. The standing close/relaunch grant was **not needed**. |
| `save_assets([])` | ⛔ **never called** — no MCP engine work was required for an asset-only commit. |
| `Source/**` · `Config/**` · `Tools/**` · `Content/FogArea/**` · `testvideo/**` | ⛔ **zero** in the commit, asserted against `git show --name-only HEAD` |
| `TASKBOARD.md` | **`Edit` tool only**, 7 in-place `- status:` lines. Total line count **34,457 → 34,457**, status-line count **1,176 → 1,176**, diff **7 insertions / 7 deletions**. ⛔ Never `Write`, never shell-rewritten. |
| Push | ⛔ **NEVER.** `main` 7 ahead of `origin/main`. |
| Git index after the run | ✅ **empty** — the headless suite auto-staged nothing. |

---

## 9. `SC-§103` — 7 IDS CARRIED, 7 FLIPS

| id | flip | why it was owed |
|---|---|---|
| **`TASK-1172`** | `done` — committed `74baab3` | the row this commit ships |
| **`TASK-1175`** | `done` — commit + revert + recompile + suite | this row |
| **`TASK-1165`** | `superseded — reverted unshipped`, **revert now physically executed** | its status was **ahead of reality**: the manager closed it 09-08 while the tree still held the patch **restored** and the binary compiled to match |
| **`TASK-1167`** | `done` — withheld artefacts shipped, LFS debt discharged | its handoff + 12 PNGs were uncommitted orphans; its §7 owed the oid check |
| **`TASK-1154`** | `superseded` — blocker gone, carrier annotations in git | its blocking sentence is no longer true of the shipped asset; the manager's 30 annotation lines landed |
| **`TASK-1158`** | `done` — **orphaned handoff finally in git** | it claimed `done — committed 20c1bea` while its own 278-line handoff was **never in `20c1bea`** |
| **`TASK-1166`** | `qa-passed` — report now in git, **filename corrected** | ships `qa/TASK-1166-report.md`; records that `qa/TASK-1165.md` never existed |

⛔ **`TASK-1173` got no flip** — it is named only as *unblocked*, and an id named to be excluded gets none.

---

## 10. WHAT STAYED DIRTY, AND WHOSE

| path | whose | why |
|---|---|---|
| `.claude/pipeline/TASKBOARD.md` | **mine** | the 7 status flips above. They record work **completed by** `74baab3`, so they necessarily post-date it. Needs a later host. |
| `.claude/pipeline/handoffs/TASK-1175-buildmaster.md` | **mine** | this file, born outside its own commit (§0). Untracked. Needs a later host. |

⛔ **Nothing else.** `Content/`, `Source/`, `Config/`, `Tools/` are **all clean**.

---

## 11. 🚨 WHAT THIS UNBLOCKS, AND THE ONE THING STILL OWED

- ✅ **`TASK-1173` IS UNBLOCKED.** The `Source/` dirt is gone, so no row has to separate this row's diff
  from `TASK-1165`'s held bytes — **and a pathspec cannot split a file**, which is why this mattered.
- 🙋 **`TASK-1159` IS OPEN AND IS THE POINT.** 🧑 Jonathan **has not seen this**. His eye outranks the
  measurement exactly as `FOG-§12.8a` cl. 4(i) says it would have outranked the opposite verdict.
  **What to observe: look at the far field, twice, three minutes apart** — because the fog moves on a
  ~181 s cycle and one glance cannot see it.
- ⚠️ **`FOG-§12.5c` cl. 5 names the discriminator that would VOID the window pin:** if the ~181 s cycle
  turns out to be an artefact of the hand-spawned probe rig and absent from a real match, the window is
  averaging the **rig**. The cheapest test is the same series with the fog **raised by the card** —
  which is `TASK-1173`'s capability. ⛔ **Untested. A lead, not a doubt.**

---

## 12. ⭐ THE ONE THING I'D CARRY FORWARD

**`TASK-1158` was marked `done — committed 20c1bea` for a full day, and the handoff describing that commit
was never in it.** The row was *truthful about its work* and *silently wrong about its record*, and
nothing in either artefact could have told you: the commit it named was real, `git show --stat` on it read
green, and the missing file simply wasn't in the pathspec.

⇒ 📌 ***A `done` flip asserts that the WORK landed. It asserts nothing about whether the RECORD OF THE
WORK landed — and the record is the only part a later row can read. Verifying a commit proves what is IN
it; only an orphan sweep proves what was LEFT OUT.*** That sweep took one `comm` against `git status`
and is why this row shipped six handoffs instead of one.
