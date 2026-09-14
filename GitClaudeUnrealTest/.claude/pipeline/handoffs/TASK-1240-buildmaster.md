# TASK-1240 — [AURA-HOST-A] — build-master handoff (2026-09-13)

marker `TASK-1240-AURA-HOST-A` · hosts `TASK-1217` (gate `1232` PASS) · `TASK-1218` (gate `1233` PASS) · `TASK-1219` (gate `1234` PASS) · law `SC-§91` · `SC-§102` · `SC-§103` · `SC-§104` · `SC-§106` · `SC-§68` · the STANDING EXCLUSION REGISTRY.

## (0) SC-§91 — state at my instant
- `HEAD` = `1306efe911dfe1030188236174b07d2e511c3224` (TASK-1241, Host B) — matched the dispatch's required pre-flight value, so I proceeded.
- `main...origin/main [ahead 1]` (`rev-list --left-right --count` = `0 1`).
- porcelain: ` M .claude/pipeline/TASKBOARD.md` · ` M handoffs/TASK-1241-buildmaster.md` (Host B's post-commit hash edit) · ` M .mcp.json` (⛔ 1242's) · untracked: `.claude/agents/playtest-verifier.md` (⛔ 1242's), `handoffs/TASK-1217/1218/1219-programmer.md`, `handoffs/TASK-1221-buildmaster.md` (⛔ 1242's), `handoffs/TASK-1223-programmer.md` (⛔ 1242's), `qa/TASK-1232/1233/1234-report.md`, `Docs/AuraIndexIgnore.txt`, `Docs/AuraProjectMemory.md`, `Tools/aura_sync.ps1`. Nothing under `Saved/`. Index empty (`git diff --cached --name-only` = nothing; the UE Git plugin auto-staged nothing).
- `.uproject`: clean in porcelain; its last commit is Jonathan's own `0399d1c` "setting up aura" ⇒ the row's `.uproject` rider is MOOT, the file was not touched or staged.
- `unreal-mcp :8000` ConnectionRefused this session — irrelevant to this text-only row (no engine step is owed).

## Re-measure (SC-§68 / SC-§104 — the three gates held no Bash, so every "declared" number below was executed here)

### `Docs/AuraIndexIgnore.txt` (TASK-1217 / gate 1232)
- 146 lines. All **22 / 22** plan-item-2 excludes present as exact whole lines (`grep -cxF` each = 1; loop over the full list of 22 reported no miss). Representative: `Content/Fab/` 1 · `Content/Realistic_Grass_and_plant/` 1 · `Content/Variant_*/` 1 · `Tools/ArtPipeline/.venv/` 1 · `.claude/pipeline/playtest-evidence/` 1 · `packagedZIPofGame/` 1 · `Models/` 1 · `DerivedDataCache/` 1.
- `grep -cxF '.claude/'` = **0** · `grep -cxF '**/.claude/**'` = **0**.
- `Content/Characters/`: **0 pattern lines**. The one textual hit is comment line 50 (`# ---- Aura's default template, carried forward (Content/Characters/ and the blanket .claude/ ...` — the sentence saying it was DROPPED). `grep -v '^#' | grep -c Characters` = 0.

### `Docs/AuraProjectMemory.md` (TASK-1218 / gate 1233)
- `wc -l` = **61**, `grep -c ''` = 61 (≤ 150).
- `grep -cF` each law = **1 / 1 / 1**: `Never compile via Live Coding — compilation belongs to build-master via Build.bat.` · `Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2.` · `Never run Git.` (the third as a whole line, `-x`).
- Secrets grep (`https?://|token|secret|api[_-]?key|password|bearer|hf_`, case-insensitive) = 0 hits.
- Re-diffs at MY instant (gate 1233's note 1): seven-agent table `CLAUDE.md:9-15` vs `AuraProjectMemory.md:9-15` **IDENTICAL** · prefix table `CONVENTIONS.md:7-29` vs `:21-43` **IDENTICAL** · texture-suffix line `CONVENTIONS.md:32` vs `:47` **IDENTICAL** · `Build.bat` command line vs `CLAUDE.md` **IDENTICAL**.

### `Tools/aura_sync.ps1` (TASK-1219 / gate 1234) — EXECUTED, three runs, cwd = `%TEMP%` (outside the repo), `powershell.exe -NoProfile -File`
Pre-run `Saved/.Aura/` state: `INDEX_IGNORE.txt` 3309 B mtime 21:12:26 · `project_memory.txt` 3469 B mtime 21:11:28 (the programmer's real run); `Skills/`, `project.json`, `CrashReportState.json`, `PythonRequirementsChecksum.txt` at 20:45–20:46 (Aura's own install).

```
RUN 1  (exit 0)
aura_sync: project root = C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest
aura_sync: destination  = C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\.Aura
aura_sync: no change   Docs\AuraIndexIgnore.txt -> Saved\.Aura\INDEX_IGNORE.txt   bytes 3309
    sha256 : source 56da74eb754ccb2511bb6b2fd5aa5607c874fbf591495630c1332acb7f70ec0e
    sha256 : dest   56da74eb754ccb2511bb6b2fd5aa5607c874fbf591495630c1332acb7f70ec0e
aura_sync: no change   Docs\AuraProjectMemory.md -> Saved\.Aura\project_memory.txt   bytes 3469
    sha256 : source 2c195b8a6a63fd75310e16b2e20ce540c79611a2dbe2acb3a136bf831afece8b
    sha256 : dest   2c195b8a6a63fd75310e16b2e20ce540c79611a2dbe2acb3a136bf831afece8b
aura_sync: summary - 0 copied, 2 unchanged, 0 mismatched
aura_sync: no change - every destination already matches its source.
RUN 2  (exit 0)  — byte-identical output to RUN 1: 0 copied, 2 unchanged, the explicit no-change line.
RUN 3  -WhatIf (exit 0) — same two `no change` rows, nothing written, no "What if:" line (nothing to copy).
```
- The first run reported `no change` rather than `copied` because the programmer's real run (handoff §3, ~21:12) had already regenerated both dests and neither source moved after its gate — the sources at MY instant hash exactly to the programmer's declared `56da74eb…ec0e` / `2c195b8a…ece8b`, so gate 1234's currency WARN is discharged: nothing went stale.
- Independent witness, `Get-FileHash -Algorithm SHA256` (not the script's own hasher): `Docs/AuraIndexIgnore.txt` = `Saved/.Aura/INDEX_IGNORE.txt` = `56da74eb754ccb2511bb6b2fd5aa5607c874fbf591495630c1332acb7f70ec0e` **EQUAL = True** · `Docs/AuraProjectMemory.md` = `Saved/.Aura/project_memory.txt` = `2c195b8a6a63fd75310e16b2e20ce540c79611a2dbe2acb3a136bf831afece8b` **EQUAL = True**.
- Post-run `Saved/.Aura/` listing: identical to pre-run — both dest mtimes UNCHANGED (21:12:26 / 21:11:28), nothing else touched.
- `git status --porcelain -- GitClaudeUnrealTest/Saved` = **EMPTY** (quoted: nothing between the header and `[end]`). `git check-ignore -v` → `GitClaudeUnrealTest/.gitignore:111:Saved/` for both dests.
- Token greps on the script (each must be 0): `&&` 0 · `??` 0 · `-AsHashtable` 0 · `Set-ExecutionPolicy` 0 · `||` 0 · network cmdlets 0 · `.claude/skills` 0 · non-ASCII bytes 0. 188 lines (`wc -l`; gate 1234's "189" counted the trailing newline — cosmetic).

## Gate 1234's four host re-run items — all four EXECUTED above
1. run from outside the repo after 1232 landed, both pairs + exit code quoted, second run `0 copied, 2 unchanged` — ✅. 2. `-WhatIf` on the live tree: two `no change`, exit 0, dest mtimes unchanged — ✅ (this also settles gate 1234's T0-label WARN: the live dest before ANY of my runs already hashed `56da74eb…`, i.e. the programmer's T0 caption "template pre-seeded" was the mislabel, the hash was right). 3. `Saved` porcelain empty — ✅. 4. independent `Get-FileHash` on all four files — ✅.

## (1) The commit
Staged by pathspec from the git root (`SC-§102`), ONLY: `GitClaudeUnrealTest/Docs/AuraIndexIgnore.txt` · `GitClaudeUnrealTest/Docs/AuraProjectMemory.md` · `GitClaudeUnrealTest/Tools/aura_sync.ps1` · `handoffs/TASK-1217-programmer.md` · `handoffs/TASK-1218-programmer.md` · `handoffs/TASK-1219-programmer.md` · `qa/TASK-1232-report.md` · `qa/TASK-1233-report.md` · `qa/TASK-1234-report.md` · `handoffs/TASK-1241-buildmaster.md` (Host B's post-commit hash line — the standing sweep) · `.claude/pipeline/TASKBOARD.md` · this file.

⛔ NOT staged, verified on the COMMIT (`git show --stat HEAD`), never the index: `.mcp.json` · `.claude/agents/playtest-verifier.md` · `handoffs/TASK-1221-buildmaster.md` · `handoffs/TASK-1223-programmer.md` (all `TASK-1242`'s) · `.claude/settings.local.json` · `GitClaudeUnrealTest.uproject` (Jonathan's `0399d1c`) · anything under `Saved/`. ⛔ No push. ⛔ No compile (none owed). ⛔ No Live Coding.

Message note: the dispatch prompt's first line (`TASK-1240: the Aura canonical files — …`) was used verbatim; the board row's cl. (1) proposed `TASK-1217/1218/1219: Aura context files — …`. The dispatch is the later instruction and names the same three rows and three gates — the same precedent as `TASK-1241`'s message note. Not a defect.

## (3) SC-§103 — the flips
`TASK-1217` · `TASK-1218` · `TASK-1219` → `done` with the hash; gates `TASK-1232` · `1233` · `1234` stamped with the host hash; `TASK-1240` → `done`. Each edit: fresh read, smallest anchor (the row's own `- status:` line), grep-back.

## Open items for the manager (not mine to rule — carried forward from the three gates, none blocking)
- 1232 WARN-1: `**/*.Target.cs` (Aura default) reaches the two `Source/*.Target.cs` build-rule files — delete the line if the letter of acceptance (2) is wanted.
- 1232 WARN-2: four unlisted Fab packs stay indexed (`Fire_Magic`, `Ice_Magic`, `IceAttack`, `MedievalWeaponsSFX`, ~1,900 files).
- 1232 NIT-3: `Config/` (Aura default, kept) hides `DefaultInput.ini` from the index.
- 1233 WARN: `AuraProjectMemory.md:49` one-line C++ prefix note drops the `CONVENTIONS.md:55` header-only exception — a one-line amend fits (89 lines of headroom).
- 1234 WARN-1: an unreadable (not missing) second source could leave a partial set — hash both sources in step 1 if "never a partial set" is wanted to the letter.

## Slack
One post in 🔧 Build & Git (`C0BF0QZP3CN`, thread `1783116286.945249`), `🔧 BUILD: ✅ TASK-1240` + hash + file list + the sync evidence.

## Commit
- hash: _(appended after the commit — see below)_
