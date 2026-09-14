# TASK-1275 — [CLOUD-EXAMPLE-DBPASSWORD-RETIRE] — gameplay-programmer handoff (2026-09-14)

Marker `TASK-1275-CLOUD-EXAMPLE-DBPASSWORD-RETIRE`. Boarded from `TASK-1266` §4 finding 2. Law: `ACC-§11` (2026-09-13 amendment, R15), `SC-§97`, `SC-§101`, `SC-§102`.

## 1. What changed

One line deleted from the tracked template `Config/SiegeCloudDev.ini.example` — the retired `; DbPassword=<…>` placeholder (line 16 before the edit). Deleted by TEXT anchor (a `sed` address matching the whole line), not by line number. Nothing else moved: lines 1–15 are byte-identical, the file went 16 → 15 lines, trailing newline preserved.

No neighbouring comment referred to "the custody line" or to the retired key (`grep -n -i 'custody\|DbPassword'` before the edit hit line 16 ONLY), so no clause was reworded.

## 2. The hunk (`git diff` from the git root — one level up, `SC-§102`)

```
diff --git a/GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example b/GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example
index 9ed491b..a8f22bd 100644
--- a/GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example
+++ b/GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example
@@ -13,4 +13,3 @@
 ;   Quotes are stripped on read; the client receives the bare URL.
 ProjectUrl="https://<project-ref>.supabase.co"
 AnonKey=<anon-or-publishable-key>
-; DbPassword=<optional custody comment line — the game NEVER reads this>
```

`git diff --stat -- GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example` (git root):

```
 GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example | 1 -
 1 file changed, 1 deletion(-)
```

Git also printed `warning: in the working copy ... LF will be replaced by CRLF the next time Git touches it` — that is the repo's autocrlf notice, not a change: `cat -A` shows the working copy is LF on every line before and after, and the diff is exactly the one deletion (0 insertions, no line-ending churn).

## 3. Measurements — before / after (all run from `GitClaudeUnrealTest/`)

| Probe | Before | After |
|---|---|---|
| Acceptance (1) `rg --no-ignore -c '^\s*;?\s*DbPassword=' Config/SiegeCloudDev.ini.example` | `1` (rg exit 0) | **`0`** — rg printed nothing, exit **1** |
| `grep -c 'DbPassword' Config/SiegeCloudDev.ini.example` | `1` | **`0`** |
| `grep -c '^ProjectUrl="' Config/SiegeCloudDev.ini.example` | `1` | `1` |
| `grep -c '^ProjectUrl=' Config/SiegeCloudDev.ini.example` | `1` | `1` |
| `grep -c '^AnonKey=<' Config/SiegeCloudDev.ini.example` | `1` | `1` |
| `grep -c '^AnonKey=' Config/SiegeCloudDev.ini.example` | `1` | `1` |
| `grep -c 'service_role' Config/SiegeCloudDev.ini.example` (line 6, the forbidding comment) | `1` | `1` |
| `wc -l` | `16` | `15` |
| `rg 'ini\.example' Source/` | 0 files (rg exit 1) | 0 files (re-measured — nothing in code or tests reads the template; no compile, no suite) |
| `git diff --stat` on the template (git root) | clean (empty) | `1 file changed, 1 deletion(-)` |

## 4. Fences honoured

- ⛔ `Config/SiegeCloudDev.ini` (the real, gitignored file) was NEVER opened, read, or edited — that is Jonathan's own `TASK-1277`. `git status --porcelain | grep -c 'SiegeCloudDev.ini$'` = `0`.
- ⛔ `Saved/**` untouched. 📌 The Aura index copy (`Saved/.Aura/indexed_files_aura/…_SiegeCloudDev_ini_example.json`) is now STALE until Jonathan's next Delete Previous Index + Sync Files (FACT 2, `Docs/setupdirections.md` §11.3) — expected, not a defect.
- ⛔ No credential value anywhere in this handoff (acceptance (4): the JWT-prefix grep on this file = 0 — measured after write, quoted in the report-back).
- ⛔ Nothing staged, no Git write, no editor touch (GUI editor PID 6136 left up), no compile, no suite.
- `git status --porcelain` from this row: `M GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example` + this handoff (`??`) + the one status line on `TASKBOARD.md`. Every other dirty path in the tree (`CONVENTIONS.md`, `AURA-PHASE0.md`, `AuraIndexIgnore.txt`, `setupdirections.md`, the 1230/1266/1267 handoffs, the 1268 report) pre-dates this row and belongs to other lanes.

## 5. Files touched

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Config\SiegeCloudDev.ini.example` — line 16 deleted.
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1275-programmer.md` — this note.
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\TASKBOARD.md` — TASK-1275 `status:` line only.

## 6. For QA (`TASK-1276`)

- Re-run acceptance (1) yourself; the meaningful signal is rg's exit code 1 with empty output (the spec says quote `0`).
- Confirm the hunk is a pure deletion: `git diff --numstat` from the git root should read `0	1	GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example`.
- Line 6 (`; ⛔ The service_role key NEVER goes in any file, anywhere (ACC-§11).`) intentionally stays — the manager ruled a comment that FORBIDS a credential is lawful on the template (row spec; `TASK-1266` lead 3).
- Host `TASK-1279` should expect the autocrlf warning on stage; it is benign.
