# TASK-595b — build-master handoff: the orphaned docs commit

**Commit:** `0e9ec1f` (`0e9ec1f0bd40be4b625eec7e529643f06b079689`)
**Date:** 2026-08-16
**Kind:** ⛔ **DOCS-ONLY. NO INTEGRATION CLAIM. NO COMPILE. NO EDITOR. NO PIE. NOT PUSHED.**
**Precedent followed:** `c275134` · `bccc282` · `a49f740` — a separately-labelled docs commit.

---

## 1. Why this commit exists

The three files were authored after `a49f740` and **no task owned them**. TASK-595 flagged them at its
close — *"they owe a commit from whichever task owns them"* — and nobody did. This is that commit.

⭐ **The stake was not bookkeeping.** `CONVENTIONS.md` carries the two laws this batch bought at real
cost. Leaving them in the worktree meant the next batch would inherit the **old, false guidance from
`HEAD`** — including a prescribed LFS check that would have shipped a broken commit.

## 2. What landed — measured, not relayed

`SC-§9` re-measurement. The relayed diffstats were confirmed exactly; **no drift, nothing added since.**

| Path | +/− | `git check-attr filter` |
|---|---|---|
| `GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md` | +45 / −0 | `unspecified` |
| `GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md` | +103 / −6 | `unspecified` |
| `GitClaudeUnrealTest/Docs/GDD.md` | +30 / −0 | `unspecified` |
| **total** | **+178 / −6** | — |

- **CONVENTIONS** — `§25b` (LFS verified by `oid` vs worktree `sha256`, never by size), `TL-§5` (a
  sweep's total is a property of its scope), plus `(e)`'s **fourth operand** (an existence check for a
  repo-level file runs from the toplevel; a negative finding may not be promoted into a premise for an
  instrument), the `WR-§2` row-2 `900`-not-`780` as-built correction, the row-13
  `CommanderWarTableForwardOffset` naming correction, and the `TL-§3` `LOD_STEP_FAILED` amendment.
- **GDD** — TASK-572's as-built `§3.15 The War Room`, the `§5` and `§8` as-built notes, and a dated
  Design Change Log entry.
- **TASKBOARD** — the manager's WR-40..42 wave (TASK-594 · 595 · 596) and the TASK-572/580 status flips.

**All 6 deletions enumerated and explained** (`SC-§29b`): the TASK-572 board row (replaced with a `done`
row), the retracted false size-check paragraph, and two `status:`/`blocked-by:` pairs for TASK-572 and
TASK-580. ⛔ **No deletion was unaccounted for.**

## 3. The correction this commit puts into history

⭐ **A claim in `HEAD` was false and is now corrected.** The board stated *"there is NO `.gitattributes`
ANYWHERE IN THIS REPO."*

**There is exactly one, and I verified it independently:**

- Path `C:/GitProjects/GitHub/GitClaudeUnrealTesting/.gitattributes` — the **repo toplevel**, one level
  **above** the project directory.
- Tracked, mode `100644`, blob `59cc7f95…`.
- **9 patterns**, wider than the relayed list: `*.uasset` `*.umap` `*.fbx` `*.png` `*.jpg` `*.wav`
  `*.mp4` `*.dll` `*.lib` → `filter=lfs diff=lfs merge=lfs -text`.
- `git ls-files ":/*.gitattributes"` returns **one** entry, repo-wide.

The original check ran one directory too deep. **The relayed LFS story was true all along.**

📌 **A size footnote, and it is `§25b`'s own distinction living in the wild.** The file is **391 B on
disk** and **382 B as the stored blob** — 9 lines, CRLF worktree vs LF blob. **Both figures are correct;
they measure different artifacts.** The documents say 391 (the worktree figure) and that is right.

## 4. ⚠️ A GIT HAZARD (d) near-miss, hit live during this task — worth propagating

⛔ **`git status --porcelain` prints paths relative to the REPO TOPLEVEL, but a PATHSPEC resolves
relative to the CWD.** This project's cwd sits one level below the toplevel, so **copying a porcelain
path straight into a git command silently addresses nothing.**

Measured here: `git diff -- "GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md"` from the project
directory returned **empty** while `git diff --stat` reported `+45` for that same file. Had that path
been handed to `git commit --`, **the commit would have contained nothing and reported success.**

✅ **The discharge used:** run git from the toplevel (or anchor with `:/`), and reconcile the staged set
against `git status --porcelain` before committing. This is the **same mechanism** as `(e)`'s fourth
operand — the toplevel is one level up — now with a **fifth** consequence. ⚖️ Exactly the reason `(e)`
insists a hazard law be filed under its **mechanism** rather than its symptom.

## 5. Verification performed

- ✅ Staged set reconciled against `git status --porcelain` — **exactly 3, zero difference** (`SC-§29b`).
- ✅ Forbidden-path scan of the staged set — **no** `.gen.cpp`, `Intermediate/`, `.umap`, `.uasset`,
  `Saved/`, `Models/`, `Source/`.
- ✅ `git check-attr filter` on all three → `unspecified` ⇒ not LFS ⇒ a plain diff is the legitimate
  instrument **by `§25b`'s own rule**. Control from the same depth: `.uasset`/`.umap` → `lfs`, so the
  instrument was confirmed live rather than assumed.
- ✅ Secret scan over the full diff — **no match** on `hf_*` / `msy_*` / `ghp_*` / `sk-*` / token
  assignment shapes. (A loose first pass matched only `TASK-###`, `risk`, `asked`.)
- ✅ `Source/` clean ⇒ **TASK-596's comment-only C++ never appeared** — nothing to exclude. It remains
  held for a batch that owns a compile.
- ✅ `L_Arena` untouched. 🔒 **TASK-552's latch UNSPENT.**
- ⛔ `Build.bat` **NOT** run — the warm DLL that is TASK-593 row (m)'s evidence is intact.
- ✅ Post-commit worktree **clean**; `origin/main...main` = `0 4`.

## 6. ⛔ What this commit does NOT claim

- ⛔ **The PIE matrix did NOT pass.** 11 of 18 rows, plus row (r), row (n)'s respawn half and row (e)'s
  *Play Again* half, remain **unobserved** — no input-injection lane exists. **All inherited by
  TASK-571.**
- ⛔ **No appearance claim** — no agent rendered a pixel. ⛔ **No token figure.**
- ✅ Zone A **is** byte-frozen at **5,658 characters** and green at runtime.
- ⛔ **This did not complete the batch's integration** — `93c5ec8` did that, and `88884ca` landed
  TASK-595's two `Tools/` scripts.

## 7. Carried forward — recorded, deliberately NOT fixed

- **WARN-1** — the 27× guard is **one-sided**: `--factor 2.0` against a 3.0-derived manifest overwrites
  mis-scaled geometry and still prints `RESCALE_OK`. Code untouched.
- **WARN-7** — ⭐ **TASK-588's premise does not reproduce from this generator.** The second `WarTable`
  token is the **MATERIAL** name; as worded, its repair **would pass the file it was written for and fix
  nothing**. Code untouched.
- **The `wc -l` 751/735 vs 752/736 off-by-one** — both files end in a newline. **The byte counts are the
  figures that pin the artifact and they match exactly.**

## 8. 📌 New observation for the manager — a stale figure now in history

The board's `BATCH STATUS 2026-08-16` block, committed here verbatim, reads
*"`git rev-list --left-right --count origin/main...main` = `0 2` ⇒ TWO COMMITS UNPUSHED."*
That was true when written. It is now **`0 4`** (`93c5ec8`, `a49f740`, `88884ca`, `0e9ec1f`).

⛔ **Not fixed here** — the board is the manager's, and this commit is scoped to landing the documents as
authored. ⚖️ It is precisely the `WR-§2` row-2 shape the same batch just wrote a law about: **a value
correct when written and wrong when read.**

## 9. Follow-ups

1. 🙋 **TASK-571 — Jonathan's playtest.** The binding open item; take it in one sitting with TASK-552.
2. **TASK-580** unblocks for a ruling only **after** TASK-571 (fixing the first-open gap destroys
   TASK-579's only acceptance instrument).
3. **TASK-596** stays boarded, not dispatched — it owes a gate and a compile in whichever batch it rides.
4. **This handoff is itself uncommitted**, matching the standing pattern (`a49f740` recorded `93c5ec8`
   after the fact). It owes a future docs commit.
5. **The push is Jonathan's — standing law.** `main` is **4 ahead**, unpushed.
