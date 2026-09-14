# TASK-1282 — programmer handoff (AURA-IGNORE-COMMENT-R16)

**Status:** ready-for-integration — gate WAIVED into the host's literal greps (`TASK-1284`, `TASK-1258` precedent). Text is the manager's verbatim prescription from the row / ruling R16.

## What changed (2 files in the repo, 1 outside it)

1. `Docs/AuraIndexIgnore.txt` line 60 — the whole comment line replaced with the row's prescribed text (`LISTED for exclusion; ⛔ exclusion UNPROVEN: index-visible measured 2026-09-14 (TASK-1265, R16 …)`). ⛔ Lines 61–62 (`Config/SiegeCloudDev.ini`, `**/SiegeCloudDev.ini`) untouched. Exactly 1 hunk (`@@ -60 +60 @@`).
2. `Docs/setupdirections.md` §11.3 fence paragraph — (a) line 845: the one word `excluded` → `listed-for-exclusion`; (b) the row's one sentence appended to the end of the same paragraph (line 848, hard-wrapped to the doc's ~92-col convention, so it occupies 848–851; no blank line inserted — same markdown paragraph). Exactly 2 hunks, both inside the §11.3 fence paragraph: `@@ -845 +845 @@` and `@@ -848 +848,4 @@`.
3. `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md` — byte-for-byte copy of (2). ⛔ Outside the repo, never staged.

Edits were made by anchored replacement (each anchor matched exactly once, files stayed LF). ⛔ `Config/SiegeCloudDev.ini` was NOT opened; no credential value appears anywhere in these files or this note.

## Read-back the host re-runs (`TASK-1284`) — measured 2026-09-14

```
grep -c 'exclusion UNPROVEN' Docs/AuraIndexIgnore.txt                      → 1
grep -c 'excluded from the INDEX' Docs/AuraIndexIgnore.txt                 → 0
grep -c '^Config/SiegeCloudDev.ini$' Docs/AuraIndexIgnore.txt              → 1
grep -c '^\*\*/SiegeCloudDev.ini$' Docs/AuraIndexIgnore.txt                → 1
grep -cvE '^\s*(#|$)' Docs/AuraIndexIgnore.txt                             → 105   (pattern count, unchanged)
git diff -U0 -- Docs/AuraIndexIgnore.txt | grep -c '^@@'                   → 1
grep -c 'the excluded `Config/SiegeCloudDev.ini`' Docs/setupdirections.md  → 0
grep -c 'listed-for-exclusion' Docs/setupdirections.md                     → 1
grep -c 'exclusion itself is UNPROVEN' Docs/setupdirections.md             → 1
git diff -U0 -- Docs/setupdirections.md | grep -c '^@@'                    → 2   (845 word swap · 848 appended sentence; both §11.3)
grep -c "$JWT" Docs/AuraIndexIgnore.txt                                    → 0   (JWT="e"+"yJ", the JWT-header prefix; spelled split so THIS note stays at 0)
grep -c "$JWT" Docs/setupdirections.md                                     → 0
grep -c "$JWT" .claude/pipeline/handoffs/TASK-1282-programmer.md           → 0
```

### The setup-doc hunks, quoted
```
@@ -845 +845 @@
-the sections and key NAMES of the excluded `Config/SiegeCloudDev.ini` in one tool call
+the sections and key NAMES of the listed-for-exclusion `Config/SiegeCloudDev.ini` in one tool call
@@ -848 +848,4 @@
-config home holds the publishable pair and nothing else.
+config home holds the publishable pair and nothing else. ⚠️ And the exclusion itself is UNPROVEN:
+on 2026-09-14, asked from the index only with no tool call, Aura still named the file, its
+section and its key names (`TASK-1265`, ruling R16) — both ignore shapes were on the list when
+that index was built. Treat the list as best-effort hygiene, never as a guarantee.
```

### Hash pair (`SC-§68`) — IDENTICAL, `cmp` clean
```
8e4097d94c04eb9d84d9324c5669c14116bb88e6bb82cdd49405a22ad37e2ebe  Docs/setupdirections.md
8e4097d94c04eb9d84d9324c5669c14116bb88e6bb82cdd49405a22ad37e2ebe  C:/GitProjects/GitHub/MyObsidianVault/JonWesOBVault/GitClaudeUnrealsetupdirections.md
```
(pre-edit pair was `7dea699f…dd6e` on both — the twin was already in step before this task.)

### `Tools/aura_sync.ps1` — exit 0, 1 copied / 1 unchanged / 0 mismatched
```
Docs/AuraIndexIgnore.txt  → Saved/.Aura/INDEX_IGNORE.txt   3793 B   copied
   source fd5bace24f87986e14f5ac9e7cc674b610fee730d3c93d5f5adb896a3a8bd360
   dest   fd5bace24f87986e14f5ac9e7cc674b610fee730d3c93d5f5adb896a3a8bd360
Docs/AuraProjectMemory.md → Saved/.Aura/project_memory.txt 3602 B   no change
   source 4bb799687cbc5c8c39f84165e60dee90087b03016e233b6946877b6f1f514297
   dest   4bb799687cbc5c8c39f84165e60dee90087b03016e233b6946877b6f1f514297
git status --porcelain -- Saved                                → (empty)
```

### `git status --porcelain` after this task (git root is one level up, `SC-§102`)
```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md        ← NOT mine (manager's 2 ACC-§11 hunks, R16)
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md          ← my row's status flip only, on top of pre-existing dirt
 M GitClaudeUnrealTest/Docs/AuraIndexIgnore.txt               ← TASK-1282
 M GitClaudeUnrealTest/Docs/setupdirections.md                ← TASK-1282
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1274-programmer.md   ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1282-programmer.md   ← this note
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuAccept.uasset          ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuDown.uasset            ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuUp.uasset              ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/Content/Input/IMC_MainMenu.uasset                   ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp   ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h     ← NOT mine (TASK-1274)
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp  ← NOT mine (TASK-1274)
```
I touched none of the "NOT mine" paths. Nothing staged; no Git operation run beyond read-only `status`/`diff`.

## For the host to know
- Git prints `warning: LF will be replaced by CRLF` for both docs on every `diff` — autocrlf noise; the working copies are LF and were LF before this task (pre-edit `git diff --stat` on both was empty). Not a change of mine.
- The appended sentence is hard-wrapped across 4 lines (848–851) to match the doc's wrapping; the row's grep phrases (`listed-for-exclusion`, `exclusion itself is UNPROVEN`) each sit whole on one line.
- The `Saved/.Aura/INDEX_IGNORE.txt` now on disk carries the new comment; the INDEX itself was not rebuilt (no editor touch, GUI PID 6136 left alone) — a rebuild is not in this row's spec.
- Board: my one edit is the `TASK-1282` `status:` line (L3194). The board's hunk count is 13 both before and after that edit — the TASK-1282..1285 block is new against `d742cd5` (the manager's `@@ -3152,0 +3166,68 @@` hunk), so my flip lives inside that already-added hunk and adds none of its own. The other 12 hunks are the manager's R16 boarding + TASK-1274-lane flips, not mine.
