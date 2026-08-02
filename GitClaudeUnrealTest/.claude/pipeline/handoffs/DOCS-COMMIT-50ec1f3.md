# DOCS-COMMIT-50ec1f3 — pipeline law safety commit

**Agent:** build-master
**Date:** 2026-08-02
**Commit:** `50ec1f3` (full: `50ec1f3a8717acfc94f9377a9dac5bd25950c86d`)
**Parent:** `5fa10eb` (TASK-378 board/handoff hash record)
**Branch:** `main` — now **4 commits ahead of `origin/main`, UNPUSHED** (no push requested)

## Nature of this commit

Docs-only. **Carries NO integration claim.** It asserts nothing about whether any
code or asset works, and it does not satisfy, advance, or bypass Jonathan's
TASK-402 playtest gate. It exists solely to durably persist pipeline law that was
accumulated over a full session and was entirely uncommitted.

Rationale: standing lesson 3 requires committing TASKBOARD + CONVENTIONS at every
**batch** boundary, not every milestone. This repo has already lost a full day of
board state once, when a `git reset --hard` / `git clean -fd` pairing destroyed
never-committed board work that had to be reconstructed from surviving handoffs.

## Exact pathspec list committed

Staged by explicit pathspec, one `git add` per call — never `-a`, never `add -A`,
never a bare `git commit`:

1. `GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md`
2. `GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md`
3. `GitClaudeUnrealTest/.claude/pipeline/handoffs/`
4. `GitClaudeUnrealTest/.claude/pipeline/qa/`

**Result: 20 files, 6,620 insertions(+), 15 deletions(-).**

| Path | Lines |
|---|---|
| `.claude/pipeline/TASKBOARD.md` (M) | 1148 |
| `.claude/pipeline/CONVENTIONS.md` (M) | 361 |
| `.claude/pipeline/handoffs/SORCERER-DARKNESS-diagnosis.md` (A) | 951 |
| `.claude/pipeline/handoffs/TASK-379-programmer.md` (A) | 241 |
| `.claude/pipeline/handoffs/TASK-395-programmer.md` (A) | 169 |
| `.claude/pipeline/handoffs/TASK-396-programmer.md` (A) | 257 |
| `.claude/pipeline/handoffs/TASK-397-programmer.md` (A) | 165 |
| `.claude/pipeline/handoffs/TASK-398-programmer.md` (A) | 230 |
| `.claude/pipeline/handoffs/TASK-399-artist.md` (A) | 170 |
| `.claude/pipeline/handoffs/TASK-401-buildmaster.md` (A) | 235 |
| `.claude/pipeline/handoffs/TASK-404-artist.md` (A) | 144 |
| `.claude/pipeline/handoffs/TASK-409-buildmaster.md` (A) | 368 |
| `.claude/pipeline/handoffs/TASK-416-programmer.md` (A) | 252 |
| `.claude/pipeline/handoffs/TASK-417-programmer.md` (A) | 480 |
| `.claude/pipeline/handoffs/TASK-418-programmer.md` (A) | 138 |
| `.claude/pipeline/handoffs/TASK-420-buildmaster.md` (A) | 202 |
| `.claude/pipeline/handoffs/TASK-426-artist.md` (A) | 258 |
| `.claude/pipeline/qa/TASK-380-report.md` (A) | 300 |
| `.claude/pipeline/qa/TASK-400-report.md` (A) | 387 |
| `.claude/pipeline/qa/TASK-419.md` (A) | 179 |

## Exclusion verification (post-commit, machine-checked)

`git show --name-only --format="" 50ec1f3` piped through an inverse filter for
`^GitClaudeUnrealTest/\.claude/pipeline/` returned **empty**; file count **20**.
A pre-commit scan of the staged set for
`Plugins/|SiegeLlama|\.dll|\.lib|Content/|\.umap|L_Arena|\.uproject|\.gitattributes|\.gitignore|assistant_eval|Source/|\.cpp|\.h$|Tools/`
also returned **empty**.

Confirmed ABSENT from this commit:

- **No C++.** All 22 modified + 8 untracked `Source/Siegebound/` files and
  `Tests/` remain uncommitted. Both lanes compile green but are gated on
  TASK-402. **TASK-403 and TASK-422 own those commits.**
- **No `Plugins/SiegeLlama/`.** Re-verified at commit time: repo-root
  `.gitattributes` (7 lines) contains **no** `*.dll`, `*.lib`, or `Plugins`
  rule, and `git ls-files -- GitClaudeUnrealTest/Plugins/` returns **0**.
  The 72 MB / 20-binary payload (headlined by `ggml-vulkan.dll`, 49.9 MB) is
  still fully untracked. **TASK-414 owns it, LFS rule as blocking pre-flight.**
- **No `Docs/Data/assistant_eval_dev.csv` / `assistant_eval_holdout.csv`** —
  sealed corpus, belongs to **TASK-422**.
- **No `Content/`, no `.uproject`, no `.gitattributes`, no `.gitignore`.**
- **No `L_Arena.umap`.** `Content/Maps/` reported clean all session; file
  verified byte-identical at **535,522 B, 2026-07-29 03:53:38**.

## Index hazard caught

At session start the index was **not** empty: the editor's Git provider had
already auto-staged `GitClaudeUnrealTest/Content/Input/Actions/IA_CmdFollow.uasset`
as `A ` (staged addition). A bare `git commit` would have silently shipped a
Content asset inside a docs-only commit.

Cleared surgically with
`git restore --staged -- GitClaudeUnrealTest/Content/Input/Actions/IA_CmdFollow.uasset`.
Post-commit the file is back to `??` — **unstaged, not lost**. `git reset --hard`
and `git clean -fd` were never used; they remain BANNED.

This is standing confirmation that **the index is never trustworthy in this
repo** — always build the staged set explicitly and diff it before committing.

## Laws recorded in this batch

- **Quiet-module law** — file-disjointness is not build-disjointness. Two lanes
  touching disjoint files can still collide through the build graph (shared
  module, `Build.cs`, header reachability). `parallel-safe` must be judged on
  what the compiler links, not on what the diff touches.
- **Relayed-diagnosis law** — a diagnosis arriving relayed through another agent
  is a **lead, not a finding**. It must be independently reproduced against the
  artifact before any fix is authorized. Relaying confers no evidence.
- **Git hazard laws** — (a) `git diff` cannot see a line-ending-only change under
  `autocrlf`, so an empty diff is not proof of no change; (b) the LFS
  append-only trap: history is append-only, so a binary landing before its
  `.gitattributes` rule is raw forever and undoing it means a history rewrite on
  a repo Jonathan pushes — the rule must precede the blob.
- **Sealed-corpus law** — eval corpora split dev/holdout, corpus authored by a
  different owner than the prompt under test; an author who can see the holdout
  cannot certify against it.
- **LLM-assistant seams §9a / §9b / §9c** — boundaries separating the assistant's
  grammar, snapshot, and command layers.

## Follow-ups for the manager

1. **TASK-414 (LFS pre-flight) is now the highest-risk open item.** The trap is
   live and one careless `add -A` away from permanent. Land the `.gitattributes`
   `*.dll`/`*.lib` rule as its own commit *before* any `Plugins/` content.
2. `GitClaudeUnrealTest/.gitignore` is modified and uncommitted — unowned by any
   task. Worth confirming whether it is meant to cover `Plugins/` staging.
3. `Docs/ThirdPartyNotices.md` is untracked and unowned by any task — likely
   belongs with the SiegeLlama/TASK-414 licensing lane.
4. `main` is **4 ahead of `origin/main`, unpushed.** No push performed or
   requested.
