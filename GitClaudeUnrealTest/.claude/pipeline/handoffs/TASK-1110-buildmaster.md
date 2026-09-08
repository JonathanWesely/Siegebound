# TASK-1110 — [TREE-USAGE-SHIP] — build-master handoff

# ✅ SHIPPED — `7444385`, 16 files, ONE commit, by explicit pathspec, verified ON THE COMMIT. `main` 8 AHEAD of `origin`, **NOT PUSHED.**

**Date:** 2026-09-06 · **HEAD before:** `12160ea` (main 7 ahead) · **HEAD after:** `7444385c56ba42dbdd15d9a8f622215d337cce11` (main 8 ahead)
**Editor:** UP, PID **24532**, MCP `127.0.0.1:8000` answering, `L_Arena` loaded, no play session.
**Absorbs `TASK-1108` in full** by that row's own cl. (9) rider. **No compile owed, and shown rather than asserted (§4).**

> ⚠️ **This handoff is born OUTSIDE its own commit** (`TL-§5e` cl. 7). It is the expected one-cycle lag: it did not exist when `7444385` was
> created, because it quotes that commit's own `git show --stat`. It is the next host's to sweep — and §7 below is about exactly what happens
> when that duty is assumed rather than assigned.

---

## 1. THE INTEGRATION CHECK — DONE BEFORE ANYTHING WAS STAGED

| check (row cl. 1) | result | how |
|---|---|---|
| Both materials carry `bUsedWithInstancedStaticMeshes = true` | ✅ **`true` / `true`** | **read OFF THE ASSET over MCP by me**, not taken from the handoff — the third independent read (artist read it in-session and again in a fresh process) |
| `BlendMode` unchanged | ✅ `BLEND_Masked` (leaf) / `BLEND_Opaque` (trunk) | matches `TASK-1109` §1 exactly |
| `bAutomaticallySetUsageInEditor` unchanged | ✅ `true` / `true` | same |
| `.uasset` bytes on disk | ✅ **17,826** / **23,142** | matches the handoff's claimed post-save sizes |
| Three pinned PNGs exist by **exact** name | ✅ all three | plus their **sha256 prefixes match the handoff's own §6 table** — `d59cca0481af` / `856516b07ef6` / `f4d2b6333b4e` |
| `git status --short Content/Tree_Pack_1/` | ✅ **EXACTLY TWO lines** | see §2 — two is correct here, three would have been the breach |
| `git status --short Content/Maps/` | ✅ **empty** | — |
| `L_Arena.umap` sha256 | ✅ `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` | **identical** to the declared ledger value |
| PIE / Simulate held by anyone | ✅ **`IsPIERunning` = `false`** | no session running; I started none and stopped none |
| `.git/index.lock` | ✅ absent | checked before staging; nothing waited on, nothing deleted |
| Index at my instant | ✅ **empty** — zero foreign paths | the editor's Git provider added **nothing** this window (`§25c` census: **no instance to report**) |

**MCP answered on every call.** No engine result in this document is inferred.

---

## 2. 🚨 TWO LINES UNDER THE PACK, NOT FOUR — AND THAT IS THE CORRECT NUMBER

The row's cl. (1) expects **four** `Content/Tree_Pack_1/**` lines and calls a **fifth** a scope breach. It shows **two**, and the missing pair is a
**measurement, not an omission**: `T_Trunk_Pack1` and `T_Trunk_Pack1_normal` **already carried the exact settings the row named** (`TC_Default`,
`TC_Normalmap` + `SRGB false`). Writing them again would have produced two `.uasset` diffs and a claimed ~12 MiB saving that never happened.

**The real blocker is dimensional:** both are **1414×1414**, and **1414 is not divisible by 4**, which UE's BCn encoder requires of the top mip. It
falls back to `B8G8R8A8` **silently** — no error, and every compression *setting* reads back green. The artist's 647-texture census settles it in
both directions: these are the **only two** non-multiple-of-4 textures in `/Game`, they are the **only two** that failed to compress, and **zero**
non-multiple-of-4 textures anywhere in the project are compressed.

⇒ **I checked the fence in the direction that matters: there is no THIRD line.** Two is under the bound, not over it. Nothing outside the two
materials moved under `Content/Tree_Pack_1/**`.

🙋 **CARRIED FORWARD TO JONATHAN, UNRESOLVED:** reaching BC needs a **build-time resize to 2048²** (`StretchToPowerOfTwo`), which resamples vendor
art and is not on the row's property list. Numbers for his decision are in `TASK-1109` §4 — including the part that stops a wrong expectation:
**the `.uasset` on disk will barely move either way**, because these store PNG source and the compressed data lives in the DDC. The saving is
resident/cooked memory, **not repository bytes**.

---

## 3. THE PATHSPEC — DERIVED AT MY OWN INSTANT (`TASK-1108` cl. 2, inherited verbatim)

`git status --short --untracked-files=all`, run by me immediately before staging. **It had already moved since the session-start snapshot** —
which is precisely the defect cl. (2) exists to prevent:

| gone since the session-start snapshot | where it went |
|---|---|
| `handoffs/TASK-1091-artist.md` | now **TRACKED** (`9daa641`) |
| `playtest-evidence/2026-09-06/MainCharacter_preview_{front,side,back}.png` | now **TRACKED** |
| `Content/RawAssets/Textures/MainCharacter/**` | now **TRACKED** |
| `Content/RawAssets/MainCharacter.fbx` | **ABSENT from disk** — deleted by `TASK-1107` under its 3-part gate |
| `Tools/ArtPipeline/pipeline_manifest.json` | now **TRACKED and clean** |
| `handoffs/TASK-1104-buildmaster.md` | **ALREADY TRACKED** (`12160ea`) ⇒ **accepted fewer**, per cl. (3)'s `ls-files` proviso |

**The 16 paths committed** (the floor was 15; `TASK-1099-artist.md` aside, nothing beyond the floor appeared, and `TASK-1106-programmer.md` **does
not exist**):

*The fix (6):*
1. `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset`
2. `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile.uasset`
3. `.claude/pipeline/handoffs/TASK-1109-artist.md`
4. `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1109-BEFORE-lane.png`
5. `…/TASK-1109-AFTER-lane.png`
6. `…/TASK-1109-SIDE-BY-SIDE.png`

*The absorbed sweep (8):*
7. `.claude/pipeline/handoffs/TASK-1099-artist.md` ← **the citation target, the reason `TASK-1108` existed**
8. `…/playtest-evidence/2026-09-06/TASK-1099-FORCELOD0-lane.png`
9. `…/TASK-1099-FORCELOD-AUTO-lane.png`
10. `…/TASK-1099-SIDE-BY-SIDE-lane.png`
11. `.claude/pipeline/handoffs/TASK-1085-buildmaster.md`
12. `.claude/pipeline/handoffs/TASK-1094-buildmaster.md`
13. `.claude/pipeline/handoffs/TASK-1098-buildmaster.md`
14. `.claude/pipeline/handoffs/TASK-1107-buildmaster.md`

*Records (2):*
15. `.claude/pipeline/TASKBOARD.md`
16. `.claude/pipeline/CONVENTIONS.md`

**NAMED-AND-LEFT (cl. 5) — and the honest report is that there was nothing to leave.** At my instant `Content/Maps/`, `Content/RawAssets/`,
`Tools/ArtPipeline/`, `testvideo/`, `Saved/`, `Intermediate/` and `Binaries/` were **all clean**. Every dirty path in the tree was in scope. **The
working tree was FULLY CLEAN immediately after the commit** — zero modified, zero untracked.

---

## 4. THE LFS TABLE — INDEX OID vs WORKING sha256, NEVER BY SIZE

Both `*.uasset` and `*.png` are LFS patterns (`.gitattributes` lines 1 and 4). Verified with `git cat-file -p :<repo-root-path>` against
`sha256sum`. **8 / 8 MATCH, 0 mismatches.**

| file | index oid (LFS pointer) | working sha256 | |
|---|---|---|---|
| `M_Pack1_Leaf_Mobile.uasset` | `6e3b3da4eac06aeea147…` | `6e3b3da4eac06aeea147…` | ✅ |
| `M_Pack1_Trunk_Mobile.uasset` | `e4605505aa3241c3d99d…` | `e4605505aa3241c3d99d…` | ✅ |
| `TASK-1099-FORCELOD0-lane.png` | `dbdf2fe31e3b8ec11996…` | `dbdf2fe31e3b8ec11996…` | ✅ |
| `TASK-1099-FORCELOD-AUTO-lane.png` | `c8d9a110bbdc6d775ac1…` | `c8d9a110bbdc6d775ac1…` | ✅ |
| `TASK-1099-SIDE-BY-SIDE-lane.png` | `c3058716467fdf8f96ab…` | `c3058716467fdf8f96ab…` | ✅ |
| `TASK-1109-BEFORE-lane.png` | `d59cca0481af77533298…` | `d59cca0481af77533298…` | ✅ |
| `TASK-1109-AFTER-lane.png` | `856516b07ef60881c451…` | `856516b07ef60881c451…` | ✅ |
| `TASK-1109-SIDE-BY-SIDE.png` | `f4d2b6333b4e82a46330…` | `f4d2b6333b4e82a46330…` | ✅ |

Pointer read back out of the **commit** as a final control:

```
$ git show HEAD:GitClaudeUnrealTest/Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset
version https://git-lfs.github.com/spec/v1
oid sha256:6e3b3da4eac06aeea14755e0b457d4feb19ba86185391f85c01b83ee07b8f88b
size 17826
```

⚠️ **One tooling note worth recording, because it produced a false alarm:** `git cat-file -p :<path>` resolves against the **repository root**, not
the cwd. The git root here is `C:/GitProjects/GitHub/GitClaudeUnrealTesting` with prefix `GitClaudeUnrealTest/` — the project directory is **not**
the repo root. My first pass used cwd-relative paths, every `cat-file` returned *"does not exist"*, and my comparison loop reported **8 MISMATCHES**.
They were not mismatches; they were empty strings failing to equal a real hash. ⇒ ***A verification loop must distinguish "the values differ" from
"I failed to read a value" — otherwise it fails safe in appearance and reports nothing useful in either direction.*** Re-run with root-relative
paths: 8/8 match.

---

## 5. VERIFY THE COMMIT, NOT THE INDEX (`§25c` cl. 2)

`git commit -F <msg> -- <16 explicit paths>` ⇒ **`7444385`**. `git show --stat HEAD`:

```
 .../.claude/pipeline/CONVENTIONS.md                |  18 +-
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  | 146 +++++++++-
 .../pipeline/handoffs/TASK-1085-buildmaster.md     | 194 +++++++++++++
 .../pipeline/handoffs/TASK-1094-buildmaster.md     | 307 +++++++++++++++++++++
 .../pipeline/handoffs/TASK-1098-buildmaster.md     | 140 ++++++++++
 .../.claude/pipeline/handoffs/TASK-1099-artist.md  | 139 ++++++++++
 .../pipeline/handoffs/TASK-1107-buildmaster.md     | 193 +++++++++++++
 .../.claude/pipeline/handoffs/TASK-1109-artist.md  | 270 ++++++++++++++++++
 .../2026-09-06/TASK-1099-FORCELOD-AUTO-lane.png    |   3 +
 .../2026-09-06/TASK-1099-FORCELOD0-lane.png        |   3 +
 .../2026-09-06/TASK-1099-SIDE-BY-SIDE-lane.png     |   3 +
 .../2026-09-06/TASK-1109-AFTER-lane.png            |   3 +
 .../2026-09-06/TASK-1109-BEFORE-lane.png           |   3 +
 .../2026-09-06/TASK-1109-SIDE-BY-SIDE.png          |   3 +
 .../Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset       |   4 +-
 .../Mobile_Tree_1/M_Pack1_Trunk_Mobile.uasset      |   4 +-
 16 files changed, 1417 insertions(+), 16 deletions(-)
```

**Diffed line-for-line against my STAGES list in §3: 16 = 16, ZERO strays.** No soft-reset was needed. **Not pushed** — `origin/main` is still at
`09b89cd`; `main` is **8 ahead**.

### NO COMPILE OWED — SHOWN, NOT ASSERTED (row cl. 4 / `TL-§5c` no-op clause)

```
$ git show --pretty=format: --name-only HEAD | grep -E "Source/|Tests/|\.cpp$|\.h$|\.cs$"
$ echo $?
1
```

**Empty output, exit 1 = no matches.** Zero `Source/**`, zero `Tests/`, zero `.cpp`/`.h`/`.cs` in the commit's **own** file list. The gate is
evaluated against the artefact that shipped, not against my intention.

### CLOSING READ-BACK (`TASK-1108` cl. 7, inherited) — THE ORPHAN IS ACTUALLY CLOSED

```
$ git ls-files --error-unmatch <path>
exit 0 : .claude/pipeline/handoffs/TASK-1099-artist.md
exit 0 : .claude/pipeline/playtest-evidence/2026-09-06/TASK-1099-FORCELOD0-lane.png
exit 0 : .claude/pipeline/playtest-evidence/2026-09-06/TASK-1099-FORCELOD-AUTO-lane.png
exit 0 : .claude/pipeline/playtest-evidence/2026-09-06/TASK-1099-SIDE-BY-SIDE-lane.png
```

**All four succeed.** `TREE-DARK-diagnosis-2.md` (in `HEAD` since `12160ea`) no longer reasons from proof the repository does not carry.

---

## 6. `TASK-1100` AND `TASK-1101` ARE **VOID** (row cl. 5 — the epitaph, in writing)

**LOD was never the cause of the dark trees, and this commit is not their delayed delivery.** The cause was measured: the two vendor materials
lacked `bUsedWithInstancedStaticMeshes`, so the engine substituted the Default Material on every HISM instance. Nobody should re-read those two
rows in three weeks and dispatch them. **And the reason they never shipped is itself worth keeping:** `TASK-1099`'s **REFUTED** verdict is the only
thing that stopped `TASK-1100` shipping a re-thresholding that would have changed nothing — which is exactly why that row's evidence is in this
commit rather than still sitting untracked on disk.

---

## 7. THE FOUR-HANDOFF FINDING — A DELIVERABLE, NOT A COURTESY (`TASK-1108` cl. 4)

Attribution is derived from the commit graph, not asserted. Today's chain, oldest first, with what each host swept from `handoffs/`:

`3e2b87e` → `e12907c` → `236b0a8` → `e37c899` → `9daa641` → `eeb29c4` → `12160ea` → **`7444385` (me)**

Every `TASK-NNNN-buildmaster.md` is written **after** the commit it describes (`TL-§5e` cl. 7), so the duty falls to the **next** host in the chain.

| handoff | born from | **should have been swept by** | **why it was not** |
|---|---|---|---|
| `TASK-1098-buildmaster.md` (09:45) | `236b0a8` | **`e37c899`** (TASK-1085, the field lane) | That host staged a hand-written pathspec naming **its own lane's** artefacts (`TASK-1083`/`TASK-1084` handoffs) and nothing else. 1098's handoff belonged to no lane by then, so no clause reached it. |
| `TASK-1085-buildmaster.md` (13:35) | `e37c899` | **`9daa641`** (TASK-1094, the character lane) | Same shape one link later: that host swept `TASK-1091`/`TASK-1093` — the **artist** handoffs its own row named — and had no standing duty toward the previous host's record. |
| **`TASK-1094-buildmaster.md` (13:45)** | `9daa641` | **`eeb29c4`** (TASK-1104, the death-camera roll) | ⚠️ **The sharp one.** That host's pathspec named exactly one handoff (`TASK-1102-programmer.md`). 1094's handoff appears in **no pathspec and no exclusion list anywhere on the board** — it was never fenced *and* never assigned. It is the only one of the four that nobody had ever written down. |
| `TASK-1104-buildmaster.md` (15:05) | `eeb29c4` | **`12160ea`** (TASK-1107) | ✅ **This one did NOT fail — it was swept, and is `ALREADY TRACKED`.** The chain worked here for one reason: `TASK-1107` carried an explicit records clause that **named `TASK-1104` by name**. |
| *(for completeness)* `TASK-1107-buildmaster.md` (15:30) | `12160ea` | **`7444385`** (me) | ✅ Swept here. The genuine, correctly-functioning one-cycle lag. |

⇒ ⚖️ ***The finding is not that four files were missed. It is that `TL-§5e` cl. 7's "expected one-cycle lag" is only self-correcting when the next
host has a STANDING sweep duty. Three consecutive hosts each staged only what their own row named — a defensible reading of `§25c`'s
explicit-pathspec rule every single time — and the lag silently became a backlog of three. It repaired itself exactly once, at `12160ea`, and only
because a human-written clause named the file. A rule that depends on someone remembering to name the file is not a rule; it is a habit.***
**`TASK-1094-buildmaster.md` is the proof: fenced and forgotten are different states, and only the second rots.**

---

## 8. WHAT STAYED DIRTY, AND WHOSE IT IS

The commit left the tree **completely clean**. I then made it dirty again, deliberately and by exactly two files — both mine, both the expected lag:

| path | owner | why it is outside `7444385` |
|---|---|---|
| `.claude/pipeline/TASKBOARD.md` | **mine** (`TASK-1110`) | Three `- status:` lines flipped **after** the commit, because two of them have to carry the hash `7444385`, which did not exist until the commit did. `Edit` tool only, one line each, no other row touched. |
| `.claude/pipeline/handoffs/TASK-1110-buildmaster.md` | **mine** (`TASK-1110`) | This file. Born outside its own commit (`TL-§5e` cl. 7) — it quotes that commit's `git show --stat`. |

**Rows flipped:** `TASK-1110` → `done 2026-09-06 (TASK-1110) — committed 7444385` · `TASK-1109` → `done — shipped 7444385` (its text kept as
history) · `TASK-1108` → `SUPERSEDED-BY-ABSORPTION — swept in 7444385`.

**Nothing else is dirty.** No `Content/**` outside the two materials, no `Tools/ArtPipeline/**`, no `Content/RawAssets/**`, no `Content/Maps/`.

---

## 9. FENCES HELD

| fence | state |
|---|---|
| `Content/Maps/L_Arena.umap` | ⛔ **never staged, never saved**; sha256 `1f78419d…5622` intact |
| `DA_BattlefieldScatter.uasset`, `MainCharacter`/`SK_MainCharacter` assets, any third `Tree_Pack_1` file | ⛔ not staged — **none was even dirty** |
| `Tools/ArtPipeline/**` · `Content/RawAssets/**` · `testvideo/**` · `Saved/`,`Intermediate/`,`Binaries/` | ⛔ not staged — all clean at my instant |
| my own handoff | ⛔ not staged (`TL-§5e` cl. 7) |
| push | ⛔ **NONE.** `main` 8 ahead of `origin`, `origin/main` still `09b89cd` |
| PIE / Simulate | ⛔ none started, none stopped; `IsPIERunning` `false` throughout |
| C++ | ⛔ zero bytes — proven in §5, not asserted |
| editor | left **UP, PID 24532**, MCP answering, nothing dirty in-editor |

---

## 10. OPEN, AND OWED TO JONATHAN

1. 🙋 **The trunk-texture half of `TASK-1109` is not done and needs one sentence from him** — BC requires a build-time resize to 2048² that
   resamples vendor art (1414 is not divisible by 4). Costings are in `TASK-1109` §4. **Nothing is blocked on it**; the tree fix he actually
   complained about is complete and shipped.
2. 🙋 **Eight commits are unpushed.** `main` 8 ahead of `origin`. No push was made and none is implied.
3. ⚠️ **The sweep-duty gap in §7 is unresolved as a process matter.** I closed today's backlog; I did not change anything that would stop it
   recurring. That is the manager's to board if it is worth a rule.
