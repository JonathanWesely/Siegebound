# TASK-595 — build-master handoff: the small commit of the two refused `Tools/` scripts

**Status: done.** One commit, four paths, no push, no compile, no editor, no script execution.

⚠️ **This file is INSIDE the commit it describes, so it cannot carry its own hash** — the board asked for ONE commit including the handoff, and amending to inject the hash would rewrite the object it certifies. **The hash is reported to the orchestrator and posted in 🔧 Build & Git.** *(Prior batches split this into two commits — `a49f740` "the record for commit `93c5ec8`" — precisely to dodge this. The single-commit shape was specified here; the consequence is stated rather than worked around.)*

---

## 1. What this commit is, and why it exists

**`93c5ec8` shipped the ARTIFACTS these two scripts produced — `Castle.fbx`, `Torch.fbx`, `WarTable.fbx` — but REFUSED the scripts themselves**, because `CONVENTIONS.md:3464` makes `Tools/**/*.py` CODE and no gate had named them (`qa/TASK-565.md` contains zero `.py`; `qa/TASK-584.md` only read them in passing, and reviewing a file in passing is not gating it).

**That refusal is now discharged.** `qa/TASK-594.md` is the gate it was waiting for: **PASS, 0 BLOCKER / 8 WARN / 8 NIT.**

⚖️ **The refusal was correct and is worth keeping as precedent:** the artifacts were already in history and could not be un-shipped, so nothing was gained by committing the generators early — and the gate, once run, found two real follow-ups (WARN-1, WARN-7) that a waived gate would have buried.

## 2. The commit contents — exactly four paths, each staged as an explicit file pathspec

⛔ **No directory pathspec was used anywhere** (`GIT HAZARD (d)` — a directory pathspec is a bounded `git add -A`).

| path | kind | state | bytes |
|---|---|---|---|
| `GitClaudeUnrealTest/Tools/ArtPipeline/build_warroom_props.py` | text `.py` | **A** (new) | 33,378 |
| `GitClaudeUnrealTest/Tools/ArtPipeline/rescale_refined_fbx.py` | text `.py` | **A** (new) | 32,567 |
| `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-594.md` | text `.md` | **A** (new) | the gate report |
| `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-595-buildmaster.md` | text `.md` | **A** (new) | this file |

**Both byte counts match the board's own figures exactly** (33,378 / 32,567).

## 3. `SC-§25` / `§25b` — verification is PER FILE KIND, and the lane was CHECKED rather than assumed

⭐ **The point of `§25b` is that you establish which lane you are in before choosing an instrument, even when the answer is boring.** It is boring here, and it was still checked:

```
git check-attr filter -- <all four paths>   ->  filter: unspecified   (x4)
```

⇒ **All four are the plain-text lane.** A byte size and a `git diff` are legitimate instruments on these; the `oid`-vs-worktree-`sha256` digest procedure does not apply and was not performed.

⛔ **AND THE LFS LANE DOES EXIST — the correction matters more than the result.** The `.gitattributes` is at the **repo TOPLEVEL**, `C:\GitProjects\GitHub\GitClaudeUnrealTesting\.gitattributes` — **one level ABOVE the project directory** (391 B, tracked). It maps:

```
*.uasset *.umap *.fbx *.png *.jpg *.wav *.mp4 *.dll *.lib   ->  filter=lfs diff=lfs merge=lfs -text
```

Proven live: `git check-attr filter` reads **`lfs`** for `Content/Maps/L_Arena.umap` and `Content/RawAssets/Castle.fbx`.

⚠️⚠️ **THIS FALSIFIES A CLAIM STILL SITTING IN `HEAD`.** The board text committed at `93c5ec8` states *"there is ⛔ NO `.gitattributes` ANYWHERE IN THIS REPO (manager-checked 2026-08-16), so LFS tracking for `.uasset` is ⛔ UNSUBSTANTIATED."* **It is substantiated, and the LFS story was true all along** — the check was almost certainly run inside `GitClaudeUnrealTest/` rather than at the repo root, which is one directory too deep. ✅ **The manager has already removed that sentence in the working-tree board edit** (it appears as a deletion in the uncommitted diff), so no action is owed — recorded here so the reasoning survives, and as a standing lesson: **`git check-attr` answers this question from any depth; `ls .gitattributes` answers it only from the root.**

## 4. `SC-§29b` — the index reconciled file by file, and the difference EXPLAINED rather than waved through

`git diff --cached --name-only` after staging returned **exactly the four paths above and nothing else** — the hostile-index failure mode (9 paths auto-staged by the editor's SCC during the last batch) **did not recur**; the index was empty before the `add`.

**Three files were dirty in the worktree BEFORE I touched anything, and all three are DELIBERATELY NOT IN THIS COMMIT:**

| path | +/− | what it is | why excluded |
|---|---|---|---|
| `.claude/pipeline/TASKBOARD.md` | +109 / −6 | the manager's WR-40..42 wave (TASK-594/595/596) + the `.gitattributes` retraction | **not in the task's `names` list.** Also carries MY board flips (§6) |
| `.claude/pipeline/CONVENTIONS.md` | +45 | new law: **`§25b`** (the LFS digest rule this task just exercised) and **`TL-§5`** (a sweep total is a property of its scope, from TASK-593) | not in `names`; the manager's authorship |
| `Docs/GDD.md` | +30 | the as-built **§3.15 War Room** pass, dated 2026-08-16 | not in `names`; the manager's authorship |

⚖️ **All three are pipeline/doc authorship by the manager, all three predate my dispatch, and none is a `Tools/` script or a QA artifact.** ⇒ **Explained, not unexplained — no STOP.** They remain dirty in the worktree for whichever task owns them.

## 5. The prohibitions, each honored and each recorded as a RULING rather than an omission

- ⛔ **NO COMPILE.** `Build.bat` was **not run.** `Tools/**/*.py` is outside the UBT module, so nothing here can change a compiled byte — and the warm DLL on disk is the evidence TASK-570 §5 used to prove row (m)'s zero describes the committed tree. ⚖️ **A verification step that can mutate the artifact it certifies is not a free check.**
- ⛔⛔ **NEITHER SCRIPT WAS EXECUTED.** Running `rescale_refined_fbx.py` **is** the 27× hazard; its guard should never be tested in anger. `build_warroom_props.py` would have rewritten two committed FBXs with different bytes for identical geometry (QA NIT-2). **The review was static, at the artifact.**
- ⛔ **NO PUSH, NO BRANCH CUT.** `main` was **2 ahead** (`93c5ec8`, `a49f740`) and is now **3**. **The push is Jonathan's — standing law.**
- ⛔ **NO EDITOR TOUCHED**, running or not. Closing it is Jonathan's choice when he is present.
- 🔒 **`L_Arena` untouched, measured both ends.** `sha256 = b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` **before and after** — byte-identical, and no `.umap` appears in the diff.
- 🔒 **TASK-552's latch is UNSPENT and stays so** — nothing model-side, no corpus, no holdout, no token figure anywhere in this commit.
- ⛔ **Forbidden path kinds: ZERO.** No `.cpp`, no `.h`, no `.gen.cpp`, no `.uasset`, no `.umap`, no `Intermediate/`, no `Saved/`, no `Models/`, no `Content/`. ⭐ **TASK-596 stayed out, as the wave ruling requires** — it rides the next batch that already owns a compile.

## 6. Board flips (in the WORKING TREE, not in this commit)

`TASKBOARD.md` is not in this commit's four paths, so both flips are **uncommitted worktree edits** awaiting whichever task next commits the board. Each was made by locating the task's own block and editing **its own `- status:` line** — ⛔ never a bulk replace.

- **TASK-594** → **`qa-passed`** (PASS, 0 BLOCKER / 8 WARN / 8 NIT).
- **TASK-595** → **`done`**.

## 7. 📌 CARRY FORWARD — two WARNs deliberately NOT fixed here

⚖️ **The code was left alone on purpose.** This task's mandate was to make two files durable, not to change them; editing them would have invalidated the gate that authorizes the commit. **Both belong to a later task.**

### WARN-1 — the 27× guard is ONE-SIDED (`rescale_refined_fbx.py:670-686`)

The guard asks only *"is the SOURCE already at target?"* It **never asks whether `source × factor` LANDS on target.** ⇒ a wrong `--factor` sails straight through: **`--factor 2.0` against a manifest already re-derived for 3.0 would same-path-overwrite `Content/RawAssets/Castle.fbx` mis-scaled INSIDE hulls that are still 3×, and still print `RESCALE_OK`.**

QA supplied a one-line predicate that **subsumes** the existing one — require, for all i:

```
abs(pre_dims[i] * factor - target[i]) <= 0.10 * target[i]
```

Verified against the real on-disk numbers: **passes** the shipped run (2437.9 × 3 = 7313.7 vs 7326, −0.17 %), **refuses** the double-run (21940 vs 7326, +199 %), **refuses** `--factor 2.0` (4875.8, −33 %). ⭐ **Cheaper than the guard it replaces, and it is the highest-value repair in either file.**

### WARN-7 — a MECHANISM CORRECTION owed to `TL-§2` / TASK-588 (`build_warroom_props.py:580-617`)

⛔ **TASK-588 must DIAGNOSE before it SPECIFIES.** The on-disk probe records the written `WarTable.fbx` mesh-node inventory as **exactly `{SM_WarTable, UCX_SM_WarTable_00}`** (`props_report.json:217-277`), and tracing the source agrees: one object survives the `join`, one hull is added, `export_fbx` uses `use_selection=True` over exactly `[table, ucx]`. **The only other `WarTable` token this generator can emit is the MATERIAL name** (`make_material("WarTable", ...)`, the spec-mandated slot name).

⇒ **`TL-§2`'s recorded cause — *"both a `WarTable` and an `SM_WarTable` node"* — DOES NOT REPRODUCE from this generator.** ⚠️ **If the real trigger is the material/object name-stem collision rather than a duplicate render node, then TASK-588's planned "assert one render node" repair would PASS this file and FIX NOTHING.** `build_warroom_props.py:580-617` is the correct insertion point for whatever the repair turns out to be — `probe_fbx` filters to `MESH` (`:588`), so a stray Null/LimbNode is invisible to the only instrument in the file.

*(The other six WARNs and eight NITs are in `qa/TASK-594.md` §3 and are not restated here. WARN-2 / 3 / 8 are one family — both scripts compute good numbers and then decline to have an opinion about them — and QA suggests a shared `fail_on(report["warnings"])` convention across `Tools/ArtPipeline/**` closes it once.)*

## 8. Two small things found while verifying, neither blocking

- **Line counts, trivial:** `wc -l` reads **751 / 735**; the board and QA both say **752 / 736**. Both files **do** end in a newline (checked with `od`), so this is a plain off-by-one in the counting instrument, not a truncated file. ✅ **The BYTE counts — the figures that actually pin the artifact — match the board exactly.**
- **Encoding, corroborated independently of QA:** `file` reports `rescale_refined_fbx.py` as **pure ASCII** and `build_warroom_props.py` as **UTF-8**, which matches QA's glyph inventory (20 non-ASCII code points, none of them U+2B50 or U+1F50D — the two that actually crash cp1252).
- **My own secret sweep, run independently of the gate** (standing build-master duty — a secret must never enter a commit, a log excerpt or a report): **4 hits total, all benign prose.** Three are the word *"Meshy"* inside comments **asserting that no Meshy call is made**; one is `socket` in a geometry comment (*"boss / socket block where the shaft leaves the plate"* — a mechanical socket). **Neither file imports `os`**; no `getenv`, no `environ`, no network, no `subprocess`. **The token surface is empty.**

## 9. Follow-ups for the manager

1. ⭐ **WARN-1's predicate** — the cheapest real safety win available in `Tools/ArtPipeline/**`.
2. ⭐⛔ **TASK-588 must re-measure its premise before it is specified** (WARN-7). As currently worded it would ship a check that passes the file it was written for.
3. **WARN-2 / 3 / 8 as one task** — a shared `fail_on(report["warnings"])` convention, so these scripts' exit codes start meaning something.
4. 📌 **`TASKBOARD.md`, `CONVENTIONS.md` and `Docs/GDD.md` are still dirty** in the worktree (the manager's wave + `§25b` + `TL-§5` + the GDD §3.15 as-built pass), now carrying my two board flips. **They owe a commit from whichever task owns them.**
5. ⛔ **`main` is 3 ahead and UNPUSHED.** Jonathan's.
