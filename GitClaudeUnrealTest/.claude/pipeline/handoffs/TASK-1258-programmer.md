# TASK-1258 — programmer handoff (`Docs/AuraIndexIgnore.txt` rev 3: the `**/SiegeCloudDev.ini` twin)

- **subject:** TASK-1258 · marker `TASK-1258-AURA-INDEX-IGNORE-3` · law R12 · R3(c) · `VER-§7` cl. 6
- **status:** done (gate WAIVED into the host's integration greps + sync re-run per the row; host = `TASK-1242`, since `TASK-1253` has already committed rev 2 at `9f2c990`)
- **date:** 2026-09-13 · gameplay-programmer
- **baseline:** rev 2 = `HEAD` `9f2c990` (blob `4f7a15e`). All diffs below are vs `HEAD`, said explicitly per acceptance (2).

## What changed — exactly two lines

`Docs/AuraIndexIgnore.txt` (148 → 149 lines; non-comment/non-blank patterns **104 → 105**):

1. **Line 60 (comment, amended)** — appended ` — **/ twin per R12: the bare dir/file shape is unproven`. Full line now reads:
   ```
   # gitignored cloud ini (publishable key) — never indexed; the rest of Config/ (DefaultInput.ini etc.) IS indexed — **/ twin per R12: the bare dir/file shape is unproven
   ```
   The board's spec renders the append inside a Markdown code span; I wrote it WITHOUT backticks around `**/` to match the surrounding comment's own style (line 60 already names `DefaultInput.ini` bare). It is a `#` comment, so the matcher never sees it either way — if the host wants the backticks, it is a comment-only edit.
2. **Line 62 (pattern, NEW)** — inserted directly after `Config/SiegeCloudDev.ini` (line 61):
   ```
   **/SiegeCloudDev.ini
   ```
   Mirrors the one in-house precedent exactly: `.claude/pipeline/TASKBOARD.md` (line 41) shadowed by `**/TASKBOARD.md` (line 42) — the shape Aura's own template uses.

Nothing else moved: header `Source: TASK-1217` untouched, the `(22 plan + 4 house)` comment untouched, line 61 itself untouched. Line endings: HEAD blob and worktree both have 0 CR bytes (LF throughout; the `LF will be replaced by CRLF` warning is the repo's autocrlf and predates this row).

## Acceptance, item by item (all MEASURED, Bash)

**(1) both greps** — `grep -c '^\*\*/SiegeCloudDev.ini$' Docs/AuraIndexIgnore.txt` → **1** · `grep -c '^Config/SiegeCloudDev.ini$' Docs/AuraIndexIgnore.txt` → **1**.

**(2) hunk count vs HEAD (`9f2c990` = rev 2, committed by `TASK-1253`)** — `git diff -U0 HEAD -- Docs/AuraIndexIgnore.txt | grep -c '^@@'` → **2**. `git diff --numstat` → `2 1` (+2 / −1). The diff, verbatim:
```
@@ -60 +60 @@ Content/Art/
-# gitignored cloud ini (publishable key) — never indexed; the rest of Config/ (DefaultInput.ini etc.) IS indexed
+# gitignored cloud ini (publishable key) — never indexed; the rest of Config/ (DefaultInput.ini etc.) IS indexed — **/ twin per R12: the bare dir/file shape is unproven
@@ -61,0 +62 @@ Config/SiegeCloudDev.ini
+**/SiegeCloudDev.ini
```
(The dispatch guessed "1 hunk, +1 or +2"; the row spec says exactly 2 and 2 is what the unchanged line 61 between the two edits produces. Note for `SC-§102`: `git show HEAD:Docs/...` FAILS — the git root is one level up; `HEAD:./Docs/AuraIndexIgnore.txt` is the working form.)

**(3) keep-set still unmatched** — the new pattern names ONLY the basename `SiegeCloudDev.ini` at any depth, so the only way it could bite the keep-set is a file of that exact name living under a keep-set root. Census with `find` (the Bash equivalent of `Glob`), keep-set roots per the `TASK-1217` row = `Source/`, `Content/{Blueprints,UI,Data,Maps,Materials,VFX,Input,Characters,Meshes,Textures}`, `Docs/GDD.md`, `.claude/pipeline/CONVENTIONS.md` (all 11 dirs + 2 files confirmed present on disk first):
- `find <11 roots> -type f -name 'SiegeCloudDev.ini' | wc -l` → **0**.
- `find <11 roots> -type f -name '*.ini'` → **0** (no `.ini` of any name under the keep-set, so no near-miss either). The two keep-set files are `GDD.md` and `CONVENTIONS.md` — basename ≠ `SiegeCloudDev.ini`.
- **Positive control** (same `find` shape, same session): `find Source -type f -name '*.Target.cs'` → 2 files (`Source/GitClaudeUnrealTest.Target.cs`, `Source/GitClaudeUnrealTestEditor.Target.cs`) — the negative above is a real negative, not tool silence.
- Project-wide (excluding `.git/`) the basename occurs at exactly two paths: `Config/SiegeCloudDev.ini` (the target) and `Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini` (a staging copy already under `Saved/`, line 32 — so the twin's any-depth reach only doubly covers something already excluded; that is a feature, not a leak). `Config/SiegeCloudDev.ini.example` does NOT match the twin (basename differs by suffix) and stays indexed per `qa/TASK-1250` NIT-1 as ruled.
- `Config/DefaultInput.ini` (and the other `Default*.ini`) stay indexable: the only new pattern has basename `SiegeCloudDev.ini`, no wildcard in the basename, and no `Config/` folder line exists (rev 2's R3(c) reasoning in `qa/TASK-1250-report.md` item (3) is unchanged by this delta).

**(4) `aura_sync.ps1` — ⛔ NOT RUN, by dispatch instruction.** The orchestrator's dispatch said "nothing written under `Saved/` (the host re-syncs)", which overrides the row's step 4 for this agent. `git status --porcelain -- Saved` → empty (`Saved/` is gitignored regardless). Consequence the host must know: **the live `Saved/.Aura/INDEX_IGNORE.txt` is still BEHIND the canonical by these 2 hunks** until `Tools/aura_sync.ps1` is re-run (its line 68 maps `Docs\AuraIndexIgnore.txt` → `Saved\.Aura\INDEX_IGNORE.txt`). The row's own gate already assigns the sync re-run to the host, so no acceptance item is orphaned — but the destination-hash quote the row asked for is the host's to produce, not mine. `TASK-1259` (Jonathan's probe) remains blocked until that sync AND Aura's re-index.

**(5) `Config/SiegeCloudDev.ini` was NOT opened.** Only its existence and path were observed (`ls`/`find`); no `cat`, `Read`, or `Grep` touched it.

## Files touched
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\AuraIndexIgnore.txt` (2 lines, above)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1258-programmer.md` (this file)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\TASKBOARD.md` — ONLY the TASK-1258 `- status:` line (line 2854), anchor-checked `- status: backlog` before the write; the board was already dirty from other rows, untouched by me.

## Notes for the host (`TASK-1242`)
- Commit `Docs/AuraIndexIgnore.txt` + this handoff by pathspec from the git root one level up (`SC-§102`); re-measure `git diff -U0 HEAD -- GitClaudeUnrealTest/Docs/AuraIndexIgnore.txt | grep -c '^@@'` → expect **2** before staging, and `grep -c '^\*\*/SiegeCloudDev.ini$'` → **1** as the waived gate says.
- Re-run `Tools/aura_sync.ps1` and quote source vs destination hashes (item 4 above). ⛔ Nothing under `Saved/` is staged.
- Slack: posted in ⚙️ Dev & QA (`1783116269.740549`) — `p1789364786589069`.

## Things QA would scrutinize, said up front
- The twin is a PRESCRIPTION until `TASK-1259` measures the matcher (`SC-§101`) — `**/` is Aura's template shape, not a proven one either; this row adds the shape QA itself named and nothing more.
- Comment-line style choice (no backticks around `**/`) — deliberate, explained above, zero effect on matching.
