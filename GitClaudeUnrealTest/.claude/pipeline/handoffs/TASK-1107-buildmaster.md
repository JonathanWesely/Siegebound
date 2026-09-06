# TASK-1107 — [DIAG-HOST] build-master handoff (2026-09-06)

**Commit: `12160ea`** · main **7 ahead** of origin, **NOT PUSHED** · base `eeb29c4`
**No compile. No suite. No engine.** Editor left running (PID 21076), untouched by this row.

> ⚠️ **This handoff is BORN OUTSIDE ITS OWN COMMIT** — `TL-§5e` cl. 7's standing tail. A build-master
> handoff records the hash of the commit that would have to contain it, which is arithmetically
> impossible, and amending only moves the hash again. **The class is bounded at one, not zero: the
> NEXT doc-host takes this file.** A row that tried to include itself would be wrong, not thorough.

---

## 1. The pathspec — exactly the row's clause (2) list, resolved at my own instant

`git commit -F <msg> -- <paths>`, five paths, hand-named, **no `add -A`, no `add .`, no `commit -a`,
no bare `git commit`** (`§25c` cl. 1):

| # | path | state before | note |
|---|---|---|---|
| 1 | `.claude/pipeline/handoffs/TREE-DARK-diagnosis.md` | untracked | **the `FIELD-§7` citation target — THE reason this row exists** |
| 2 | `.claude/pipeline/handoffs/TREE-DARK-diagnosis-2.md` | untracked | the CAUSE-FOUND diagnosis |
| 3 | `.claude/pipeline/CONVENTIONS.md` | modified | today's manager edits (`SC-§95` new · `SC-§83` addendum · `TL-§5c` ledger · `FIELD-§7` amended) |
| 4 | `.claude/pipeline/TASKBOARD.md` | modified | |
| 5 | `.claude/pipeline/handoffs/TASK-1104-buildmaster.md` | untracked | the **expected** one-cycle lag (`TL-§5e` cl. 7) |

**`.claude/pipeline/handoffs/TASK-1106-programmer.md` — MEASURED ABSENT on disk, therefore omitted.**
Checked with an explicit `-f` test before staging; the row's clause (2) conditions it on *"IF IT EXISTS"*
and clause (3) of the dispatch forbids inventing a path. TASK-1106 had not landed at my instant.

## 2. `git show --stat HEAD` — pasted, `§25c` cl. 2

```
commit 12160eaaa49350d61c20fd996cb833a9a35eb951
Author: Jonathan Wesely <wesely.jonathan@gmail.com>
Date:   Sun Sep 6 15:28:16 2026 -0700

    TASK-1107: the tree-dark diagnoses enter the repository, so FIELD-§7 stops citing a file HEAD does not contain

 .../.claude/pipeline/CONVENTIONS.md                |  44 +++-
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  |  83 +++++-
 .../pipeline/handoffs/TASK-1104-buildmaster.md     | 293 +++++++++++++++++++++
 .../pipeline/handoffs/TREE-DARK-diagnosis-2.md     | 161 +++++++++++
 .../pipeline/handoffs/TREE-DARK-diagnosis.md       | 131 +++++++++
 5 files changed, 700 insertions(+), 12 deletions(-)
```

`git diff-tree --no-commit-id --name-only -r HEAD` → **count: 5**, diffed line-for-line against the
pathspec above. **Exact set, exact count, ZERO foreign paths.** No `Content/**` reached this commit
despite the editor being up — the pathspec is what made that true, not luck (`§25c` INSTANCE 2's lesson:
the index moved once with *nothing running*). The index was **empty immediately before staging** and is
**empty now**; no stray needed unstaging this run.

## 3. ✅ CLAUSE (6) — THE POSITIVE CONTROL, QUOTED VERBATIM. **The orphan is CLOSED.**

```
$ git ls-files --error-unmatch .claude/pipeline/handoffs/TREE-DARK-diagnosis.md
.claude/pipeline/handoffs/TREE-DARK-diagnosis.md
exit=0

$ git ls-files --error-unmatch .claude/pipeline/handoffs/TREE-DARK-diagnosis-2.md
.claude/pipeline/handoffs/TREE-DARK-diagnosis-2.md
exit=0
```

**Both SUCCEED.** `FIELD-§7`, which has been in `HEAD` since `eeb29c4`, now cites a file the repository
actually contains. The finding `TASK-1104` refused to absorb silently is resolved by **hosting**, not by
rewording — the citation stayed and the evidence moved, so the law remains falsifiable against its source.

## 4. ✅ CLAUSE (7) — the no-compile demonstration, SHOWN not asserted (`TL-§5c` no-op clause)

The basis is **my own pathspec**, not a claim about it:

```
$ git diff-tree --no-commit-id --name-only -r HEAD | grep -E "Source/|Tests/|\.cpp$|\.h$|\.cs$"
grep exit=1   ← no match
```

All five committed paths are text under `.claude/pipeline/`. **Zero `Source/**` bytes, zero `Tests/**`
bytes, zero `.cpp`/`.h`/`.cs`, zero `/Game/` assets** ⇒ there is nothing a compile could compile and
nothing a suite could cover. UBT was never invoked and the editor was never asked for anything. An
unexecuted gate wearing a green shirt is exactly what that clause exists to catch, so the negative is
printed above rather than promised.

## 5. 🧑⚖️ THE GATED FBX DELETION — all three gates PASSED, **file DELETED**

Inherited from `TASK-1101` cl. (5) by name (1101 is VOID), manager ruling 1. Verified **at my own
instant** (`SC-§91`), not relayed:

| gate | requirement | measured | verdict |
|---|---|---|---|
| **(i)** | `git ls-files --error-unmatch Content/RawAssets/MainCharacter.fbx` **FAILS** | `error: pathspec ... did not match any file(s) known to git`, **exit=1** | ✅ PASS — untracked |
| **(ii)** | `git ls-files --error-unmatch Content/RawAssets/Characters/MainCharacter.fbx` **SUCCEEDS** | printed the path, **exit=0** (committed in `9daa641`) | ✅ PASS — survivor tracked |
| **(iii)** | md5s **DIFFER** and survivor's **matches `SK_MainCharacter`'s `AssetImportData`** | superseded `d605148553b3c231c9f155c08cbf3256` · survivor `90526b4193054d7e4ba4ea02e10cb73a` | ✅ PASS |

**Gate (iii) was verified against the ASSET, not against the dispatch's relayed digest.** I read
`Content/Characters/SK_MainCharacter.uasset` (6,594,027 bytes) directly and scanned it for 32-hex runs:

- exactly **one** such run exists in the whole file: **`90526b4193054d7e4ba4ea02e10cb73a`** = the survivor.
- **`d605148553b3c231c9f155c08cbf3256` is ABSENT** from the asset.
- the stored source path contains **`RawAssets/Characters`**; the string `RawAssets/MainCharacter.fbx`
  is **absent**.

⇒ the shipped `SK_MainCharacter` was imported from the **survivor**, and the deleted file was never its
source. That is the discriminator the gate was asking for, taken from the artefact itself.

**Action:** gate (i) was **re-confirmed a second time at the deletion instant** (untracked, immediately
before `rm`), then `Content/RawAssets/MainCharacter.fbx` was deleted. A session-scoped copy sits at
`…/scratchpad/MainCharacter.superseded.fbx` (md5 re-verified `d605148553…`) — **stated honestly: that
scratchpad dies with the session and is NOT a recovery guarantee**, only an in-session undo.

**Why it had to go** (ruling 1, not re-derived): two plausible sources with the *same filename* and
different bytes is exactly the shape that has already cost this project a reimport.

⛔ **`Content/RawAssets/Textures/MainCharacter/**` was NOT touched** — explicitly KEPT. All three
(`T_MainCharacter_D/N/ORM.png`) are **tracked and present on disk**, verified after the delete. Nothing
else under `Content/RawAssets/` was removed; the survivor is intact at 1,374,924 bytes.

## 6. Clause (3) — NAMED-AND-LEFT. **Nothing silently omitted.**

| named file | tracking state | what I did |
|---|---|---|
| `Content/Maps/L_Arena.umap` | **TRACKED, clean** (absent from `git status`) | left; never staged |
| `Tools/ArtPipeline/pipeline_manifest.json` | **TRACKED, clean** — it was dirty at session open but went in with `9daa641`; **nothing to leave any more** | left; never staged |
| `Content/RawAssets/Textures/MainCharacter/**` | **TRACKED** (3 files) | KEPT, untouched (ruling 1) |
| `.claude/pipeline/handoffs/TASK-1091-artist.md` | **TRACKED** | ⇒ **`TASK-1091`'s own host DID already take it.** Nothing owed. |
| `playtest-evidence/2026-09-06/MainCharacter_preview_{front,back,side}.png` | **all three TRACKED** | ⇒ same — **1091's host took them.** Nothing owed. |

**The clause (3) conditional resolves cleanly: TASK-1091's records are already in `HEAD`, so this row
had nothing to sweep there.**

### 6b. `TASK-1099`'s records — deliberately LEFT, and why

`.claude/pipeline/handoffs/TASK-1099-artist.md` and the three
`playtest-evidence/2026-09-06/TASK-1099-{FORCELOD0,FORCELOD-AUTO,SIDE-BY-SIDE}-lane.png` are **untracked
and were NOT staged.**

The dispatch asked me to stage them *"if your row licenses it"*. **It does not.** My row's `names:` line
reads **"STAGES: the clause (2) list ⛔ EXACTLY"**, and clause (2) does not contain them — nor does
clause (3) name them. `§25b` cl. R's hand-named pathspec is not mine to widen at the terminal, and
`TL-§5e` cl. 7's derived-pathspec exception is **granted in the row, never assumed** — my row grants no
such exception.

⚠️ **But I agree with the dispatch's reasoning and I am not letting it drop: `TASK-1099`'s REFUTED
verdict is load-bearing evidence** — it eliminated the LOD/geometry hypothesis at the lane vantage and
stopped a no-op fix shipping, and `TREE-DARK-diagnosis-2.md` (now in `HEAD`) reasons *from* it. **The
committed diagnosis now cites an uncommitted proof** — which is the same `TL-§5e` orphan shape this very
row exists to close, one lane over. ⇒ **This needs a row. Escalated to the orchestrator/manager, not
absorbed.**

## 7. ⚠️ FINDING — the lagging-handoff registry note is breached, 4 not 1

The registry note says **ONE** lagging build-master handoff is expected and **TWO is a finding**. On disk,
untracked, at this instant:

| file | disposition |
|---|---|
| `TASK-1085-buildmaster.md` | ⛔ NEVER-staged by my dispatch |
| `TASK-1094-buildmaster.md` | **named by nobody** — not in my clause (2), not in my exclusions |
| `TASK-1098-buildmaster.md` | ⛔ NEVER-staged by my dispatch |
| `TASK-1104-buildmaster.md` | ✅ swept into `12160ea` |

**I swept the one I was told to and left the other three**, because `names:` binds me to the clause (2)
list exactly. **`TASK-1094-buildmaster.md` is the one that should alarm somebody: it is not excluded, it
is simply unmentioned** — the difference between *fenced* and *forgotten*, and only the second one rots.
By `TL-§5e` cl. 7 the answer is a sweeper row, **not more diligence from the next host** — reported, not
fixed.

## 8. What stayed dirty, and whose it is

After the commit and the board edit:

| path | owner |
|---|---|
| `.claude/pipeline/TASKBOARD.md` (modified) | **mine** — the `done` status line records `12160ea`, so it necessarily lands *after* the commit. Same self-reference as this handoff. Next doc-host. |
| `.claude/pipeline/handoffs/TASK-1107-buildmaster.md` (untracked) | **mine** — `TL-§5e` cl. 7's bounded tail |
| `handoffs/TASK-1085-buildmaster.md`, `TASK-1094-buildmaster.md`, `TASK-1098-buildmaster.md` | other lanes / unowned — see §7 |
| `handoffs/TASK-1099-artist.md` + 3 `TASK-1099-*-lane.png` | the tree lane — see §6b, **needs a row** |

**Index: EMPTY.** No `Content/**` was staged by the editor's Git provider during my window, and I
restored/tidied nothing belonging to another lane. The `Provider=Git` setting was **not** changed — it is
Jonathan's call and remains open.

## 9. Not pushed

`main` is **7 ahead of `origin/main`, 0 behind**. **`git push` was not run** and no remote was contacted.

## 10. Downstream — the fix is with Jonathan

The tree fix is **two property writes plus a save** (`bUsedWithInstancedStaticMeshes` on
`M_Pack1_Trunk_Mobile` and `M_Pack1_Leaf_Mobile`). It is **deliberately not boarded** and is **not in this
commit**: it is a vendor-pack edit under `/Game/Tree_Pack_1/`, and the save forces a shader recompile that
wedged this editor 100+ min twice on `TASK-239`. **Neither is an agent's call.** This row shipped the
*evidence*, not the repair.
