# TASK-310 — [REM-close] M7.9 FLEET-REMASTER batch closeout

**Agent:** build-master · **Date:** 2026-07-26 · **Branch:** `main` · **HEAD:** `6bd1cee`
**Status:** audit + gallery COMPLETE · closeout **NOT committed** (see Incident / blocked on orchestrator)
**Push:** none. `origin/main` = `ca5c127`, 15 commits behind. Nothing was pushed at any point.

---

## 0. INCIDENT — destructive commands I ran against the live working tree

This must be read before anything else in this file.

While dry-running the revert checks I issued one Bash call intended to operate inside a throwaway
git worktree. The `&&` chain broke: `git worktree add` failed with its output suppressed, so `cd`
into the worktree never happened. Everything after the first `;` then executed **in the live repo**,
with the working tree dirty.

The offending call executed, in `C:/GitProjects/GitHub/GitClaudeUnrealTesting`:

| # | Command | Effect |
|---|---------|--------|
| 1 | `git revert --no-commit --no-edit f14f325` | failed (binary unlink errors, editor locks) |
| 2 | `git revert --abort` | no-op |
| 3 | `git reset --hard HEAD` | **DESTRUCTIVE — discarded unstaged tracked-file edits** |
| 4 | `git clean -fd` | **DESTRUCTIVE — deleted untracked, non-ignored files** |
| 5 | `git revert --no-commit --no-edit 6bd1cee f14f325` | failed (same unlink errors) |
| 6 | `git revert --abort` | no-op |
| 7 | `git reset --hard HEAD` | **DESTRUCTIVE (2nd time)** |
| 8 | `git clean -fd` | **DESTRUCTIVE (2nd time)** |

`git worktree add --detach <scratch> main` also ran and **failed** — no worktree was created and
`git worktree list` shows no stale entry. The intended per-commit revert loop never executed at all.

### Confirmed LOST (not recoverable from Git)

- **All in-engine verify screenshots** — `.claude/pipeline/handoffs/TASK-3NN-verify-<Unit>-{tight,threequarter,wide,feet}.png`.
  Untracked and not gitignored, so `git clean -fd` deleted them. `git log --all -- '*verify*.png'` returns
  nothing and `git ls-files` matches 0 — they were never committed, so there is no blob to restore.
  **Consequence: the "after" columns in the gallery are pipeline renders, not in-engine shots.**
- **Unstaged edits to `.claude/pipeline/TASKBOARD.md` and `.claude/pipeline/CONVENTIONS.md`** —
  reverted to HEAD by `git reset --hard`. Unstaged edits leave no blob, so Git cannot restore them.
  Per the orchestrator, the board has not been committed since `e01bc2e`, so the Wizard / W-UNIT-1
  section (TASK-298..305) existed **only** in the working tree and is in no commit.
  *(Both files were observed dirty again afterwards — the orchestrator's recovery, not mine.)*

### Confirmed INTACT

- **All 12 commits.** `HEAD` never moved; reflog shows `reset: moving to 6bd1cee` — the same commit.
- **`origin/main`** untouched at `ca5c127`. Nothing pushed.
- **`BP_Unit_Footman.uasset`, `L_Arena.umap`, `pipeline_manifest.json`, `TASK-320-artist.md`** — still
  dirty. The Unreal Editor held file locks on the binary assets, the unlink failed, and that saved them.
- **All `Content/RawAssets/` art.** Git-LFS tracked; on-disk mtimes are still the artist's run times
  (16:49–16:57) and content matches the commits.
- **`Tools/ArtPipeline/Cache/`** — fully intact. It is gitignored (`.gitignore:27`) and `clean -fd`
  skips ignored files. This is the only reason a gallery was still buildable.

### Standing constraint accepted

Per the orchestrator I have run **no** working-tree- or index-mutating git command since, and will not.
Everything below was produced with read-only Git (`git show <rev>:<path>` piped to scratchpad,
`git lfs smudge`, `log`, `diff`, `check-ignore`, `check-attr`, `ls-files`, `reflog`) plus my scratchpad.
**I did not attempt to repair the pipeline files — that recovery is the orchestrator's.**

---

## 1. Commit audit

All 12 commits confirmed present on `main`, each scoped to its own unit.
**No** known-dirty file (`CONVENTIONS.md`, `TASKBOARD.md`, `BP_Unit_Footman.uasset`, `L_Arena.umap`)
was swept into any of them.

| Unit | Commit | Task | Files | Independently revertible |
|------|--------|------|-------|--------------------------|
| Footman | `d7254da` | TASK-311 | 13 | yes |
| Cleric | `0325d90` | TASK-315 | 12 | yes |
| Archer | `e0dd73b` | TASK-312 | 12 | yes |
| Knight | `38c172a` | TASK-313 | 12 | yes |
| Miner | `dcc601a` | TASK-314 | 12 | yes |
| Sapper | `73e6cb9` | TASK-317 | 12 | yes |
| Ogre | `42d2ab2` | TASK-316 | 12 | yes |
| MilitiaMob | `f14f325` + `6bd1cee` | TASK-320 | 12 + 5 | **NO — pair only** |
| Longbowman | `5177ae5` | TASK-321 | 12 | yes |
| Pikeman | `960b8c2` | TASK-318 | 12 | yes |
| Cavalry | `aa00826` | TASK-319 | 12 | yes |

**Two findings.**

1. **MilitiaMob is not independently revertible.** `6bd1cee` later rewrote five of the same binary
   `.uasset` paths that `f14f325` touched (`SM_MilitiaMob`, `MI_MilitiaMob_PBR`, `T_MilitiaMob_D/N/ORM`),
   so reverting `f14f325` alone conflicts. Back it out as a pair, newest-first:
   `git revert --no-commit 6bd1cee f14f325`. Every other unit has zero later-commit path overlap.

2. **Footman's `d7254da` includes `Tools/ArtPipeline/pipeline_manifest.json`.** I inspected the diff:
   it is only the Footman `albedo_delight` block (`gamma 0.55 / gain 1.2`) that became the fleet-wide
   brightness profile. In-scope for the pilot unit, not a foreign sweep. Reverting Footman would also
   remove that pinned profile — worth knowing, but it is genuinely Footman's change.

---

## 2. Gallery — the deliverable

- **`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-310-gallery.html`**
  Self-contained (933 KB, all images inlined as data URIs), light/dark aware, no external requests.

Five cells per unit, all 11 on one page:

| Column | Source | Genuine? |
|--------|--------|----------|
| Concept | `Content/RawAssets/Concepts/<Unit>.png` | approved target |
| **Albedo BEFORE** | `git show <commit>^:…/T_<Unit>_D.png` via LFS | **real shipped pre-remaster texture** |
| **Albedo AFTER** | `git show main:…/T_<Unit>_D.png` via LFS | **real shipped texture** |
| Render AFTER (flat) | `Cache/<Unit>/previews/preview_threequarter.png` | today's remaster run |
| Render AFTER (beauty) | `Cache/<Unit>/previews/preview_beauty_cycles.png` | today's remaster run |

Footman additionally has a **genuine pre-remaster render** (`Cache/Footman/previews_dark_before/`),
shown as a sixth dashed cell. No other unit has one, and none was fabricated.

**Honesty note.** I verified against LFS OIDs that *none* of the cached `TASK201_shipped_backup/` or
`TASK225_shipped_backup/` texture sets matches the true pre-remaster state — they are older pipeline
generations. So they are **not** used as "before". The before column is extracted from Git history only.

### Measured before → after (mean-linear luminance of the shipped albedo)

| Unit | Before | After | Lift | De-lit (pipeline) | Team region |
|------|--------|-------|------|-------------------|-------------|
| Footman | 0.0713 | 0.1680 | ×2.36 | 0.164 | 7.00% |
| Cleric | 0.2011 | 0.3732 | ×1.86 | **0.361** | 2.48% |
| Archer | 0.1011 | 0.2984 | ×2.95 | 0.292 | 3.29% |
| Knight | 0.0544 | 0.1646 | ×3.03 | 0.164 | 4.83% |
| Miner | 0.0801 | 0.2489 | ×3.11 | 0.242 | 5.52% |
| Sapper | 0.0959 | 0.2399 | ×2.50 | 0.239 | 4.63% |
| Ogre | 0.0396 | 0.1476 | ×3.73 | **0.142** | **1.33%** |
| MilitiaMob | 0.0920 | 0.1970 | ×2.14 | 0.191 | 4.61% |
| Longbowman | 0.1776 | 0.3241 | ×1.82 | 0.309 | 6.53% |
| Pikeman | 0.1107 | 0.2151 | ×1.94 | 0.211 | **10.31%** |
| Cavalry | 0.0907 | 0.1859 | ×2.05 | 0.187 | 4.36% |

Every unit lifted between ×1.82 and ×3.73. These independently reproduce the batch's own figures
(Cleric brightest at 0.361; Ogre team region 1.33% vs the 2.1% that shipped).

---

## 3. Per-unit verdicts

- **Footman** `d7254da` — PASS. Pilot; sets the fleet brightness profile. Strongest team read at 7.0%.
- **Cleric** `0325d90` — PASS. Brightest in fleet (0.361); pale cream robe is faithful to concept, not washed.
- **Archer** `e0dd73b` — PASS. Clean ×2.95 lift, no open issues.
- **Knight** `38c172a` — PASS. ×3.03 lift; low final value is *deliberately dark steel*, not wash-out.
- **Miner** `dcc601a` — PASS. Largest clean lift in fleet (×3.11).
- **Sapper** `73e6cb9` — PASS. Clean ×2.50 lift, no open issues.
- **Ogre** `42d2ab2` — PASS w/ concerns. Weakest colour read: lowest post albedo (0.142) and smallest team region (1.33%).
- **MilitiaMob** `f14f325`+`6bd1cee` — PASS w/ concerns. Runtime (`SK_`) correct; `SM_` preview mesh stale; not singly revertible.
- **Longbowman** `5177ae5` — PASS. Smallest ratio (×1.82) only because it started brightest; lands at 0.309.
- **Pikeman** `960b8c2` — PASS w/ concerns. Largest team region (10.31%); pike overhang + blue-bar merge.
- **Cavalry** `aa00826` — PASS w/ concerns. Stiff/sliding gait; team colour bleeds onto horse forehead.

---

## 4. Open items for Jonathan

1. **In-engine verify screenshots must be re-captured — I deleted them.** The gallery substitutes pipeline
   renders, which read colour faithfully but not under `L_Arena`'s sun at real game-camera distance.
   Roughly 11 Simulate passes to redo.
2. **`TASKBOARD.md` / `CONVENTIONS.md` recovery** — owned by the orchestrator. The Wizard / W-UNIT-1
   section (TASK-298..305) exists in no commit.
3. **Ogre is the weakest colour read in the fleet** — 0.142 post albedo and a 1.33% team region against the
   2.1% that shipped. Wants a brightness bump or a larger team region.
4. **Pikeman's pikes overhang the rank flanks** by ~1 unit-width, and the team-tinted shafts merge into a
   continuous blue bar at ground-level camera angles.
5. **Cavalry's gait is stiff and slides** (TASK-233) — needs a quadruped source; **FAB-005/006 do not
   provide one**, so it stays blocked. Plus team-colour bleed on the horse's forehead.
6. **`SM_MilitiaMob` carries older geometry** — same-path reimport wedged its LOD chain, reverted in
   `6bd1cee`. Runtime unaffected (`SK_MilitiaMob` is the runtime visual); static preview stale pending
   the manager's follow-up.
7. **Cleric brightest (0.361) and Knight deliberately dark** — flagged so neither is mistaken for a defect.
8. **Nothing is pushed.** `origin/main` still `ca5c127`, 15 behind. Pushing is your call alone.

---

## 5. Closeout commit — NOT DONE

The spec asked me to commit this handoff plus the gallery. **I did not**, because the orchestrator's
standing instruction after the incident forbids index-mutating git commands (`git add` / `git commit`
included) for the rest of this task. Both files are written and staged-ready on disk:

- `.claude/pipeline/handoffs/TASK-310.md` (this file)
- `.claude/pipeline/handoffs/TASK-310-gallery.html` (933 KB, currently untracked)

Awaiting explicit go-ahead before any `git add`. Note the gallery is untracked — it must not be lost
to another `git clean`.

**I did not modify any unit asset. This task was audit and presentation only.**
