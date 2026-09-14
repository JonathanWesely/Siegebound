# TASK-1278 — [CENSUS-LAW-LINES] — manager handoff (2026-09-14)

**Verdict line: two dated amendments written, two edit sites, expected `@@` = 2 — `ACC-§11` (the standing census) + `SC-§39` (the instrument-absent-from-the-shell false-clean). Nothing else in `CONVENTIONS.md` moved. No credential value, no match text, anywhere.**

Law: `ACC-§11` · `SC-§39` · `SC-§82` (manager-owned law, no QA gate — `TASK-1279`'s read-back cl. (2)(c) is the check) · `SC-§100` · `SC-§101` · `SC-§105`.
Source: `handoffs/TASK-1266-buildmaster.md` §0 · §2.1 · §2.8c · §3b · §4 (findings 6, 7) · §5 (items 1, 3) · §6; the `TASK-1266` row's 2026-09-14 rulings (`ENGINE` / `NOISE`, twin deletion DECLINED).

---

## 1. Baseline

`CONVENTIONS.md` was CLEAN on disk against HEAD (`cff9807`) when this row started — `TASK-1272` (`bbee7d9`) and its board back-reference (`cff9807`) had landed, so the R15 / `TASK-1273` pins were spent. Baseline `@@` = 0. **Every `CONVENTIONS.md` hunk `TASK-1279` sees is mine.**

## 2. Edit sites (the list `TASK-1279` pre-flights against)

| # | Section | Where | Shape | Read-back string for `TASK-1279` cl. (2)(c) |
|---|---|---|---|---|
| 1 | `SC-§39` | one new top-level bullet appended after the 2026-09-02 "MIRROR IMAGE … `TASK-851`" bullet, immediately before the blank line + `#### SC-§39.1` (file line ~3667) | 1 inserted line, 0 removed | hunk first line begins `- ⛔⛔⭐⭐ **AMENDED 2026-09-14 — ⛔ THE INSTRUMENT CAN BE ⛔ ABSENT FROM THE SHELL THAT RUNS IT` — contains **`shell FUNCTION`** |
| 2 | `ACC-§11` | one new sub-bullet appended after the 2026-09-13 "THE CONTENTS RULE" sub-bullet, immediately before `- ⭐ **LOCAL-FIRST REMAINS THE LAW.**` (file line ~6511 after edit 1's shift) | 1 inserted line, 0 removed | hunk first line begins `  - ⭐ **DATED AMENDMENT 2026-09-14 — THE STANDING CENSUS` — contains **`ENGINE`** and **`export -f rg`** |

- **Expected `git diff -U0 -- GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md | grep -c '^@@'` = 2.** The two sites are ~2,840 lines apart; they cannot merge into one hunk.
- **Expected `git diff --stat`: `CONVENTIONS.md` +2 / −0.**
- **`grep -c 'ENGINE' CONVENTIONS.md` delta = 1 line** (measured on disk after the edit: 62 lines; baseline therefore 61). The `ACC-§11` amendment is one physical line holding 5 uses of `ENGINE`; the `SC-§39` amendment holds 0 (deliberately, so the delta is a single line). `grep -c` counts lines, not uses.
- Acceptance (3) `git diff --stat` = `CONVENTIONS.md` only: this row touched `CONVENTIONS.md`, `TASKBOARD.md` (its own status line + `TASK-1279`'s `blocked-by`) and this handoff — nothing else. The board and handoff are `TASK-1279`'s carries, not extra diff.

## 3. What each clause says (one line each; the clauses cite the handoff, they do not restate its tables)

**`ACC-§11` — DATED AMENDMENT 2026-09-14, THE STANDING CENSUS:**
- (i) pattern **(9)** `(?i)"(password|passwd|secret|token)"\s*:\s*"[^"]{8,}"` joins the eight — (8) cannot see a JSON-quoted key (handoff §5 item 3); the first `ENGINE` hit was found only by it (§2.8c).
- (ii) classes **`NOISE`** (base64 image dumps + PEM bodies where `eyJ` occurs by chance — §2.1 + Appendix A) and **`ENGINE`** (engine-generated or Epic-shipped, machine-local, gitignored, never shipped) join `VALUE` / `PLACEHOLDER` / `PROSE` / `TEST`. The two KNOWN `ENGINE` members are named BY PATH ONLY: `Saved/Cooked/Windows/ue.projectstore` (`zenserver.hostauth`) and `Saved/Temp/Win64/Engine/Plugins/MetaHuman/MetaHumanSDK/Config/DefaultMetaHumanSDK.ini` (`ClientCredentialsSecret`). Rule: a future census REPORTS these two as `ENGINE` and does not STOP; any NEW `ENGINE`-shaped path is still a STOP in 🚨 for a ruling.
- (iii) the expected `VALUE` set is exactly two — the `AnonKey` line in `Config/SiegeCloudDev.ini` + its `Saved/Temp/Win64/GitClaudeUnrealTest/Config/` twin (regenerates on every stage/cook; deletion DECLINED). A third `VALUE` is a STOP.
- (iv) the recipe's first line is `export -f rg` (handoff §6).

**`SC-§39` — AMENDED 2026-09-14, the instrument can be absent from the shell that runs it:**
- `rg` is a shell FUNCTION here (delegates to `claude.exe`'s bundled ripgrep 14.1.1), invisible to a child `bash script.sh`; `TASK-1266`'s first run read 0/0 on all eight patterns; the only tells were `rg: command not found` on stderr and pattern (6) reading 0 on a file KNOWN to hold an `AnonKey=` line.
- Rule: a census that reports ZERO first runs its instrument against a KNOWN POSITIVE and reads stderr. Standing control: pattern (6) `^AnonKey=\S` on `Config/SiegeCloudDev.ini` = 1 (counted, never opened).
- Family: `SC-§39`'s own positive-control rule; `SC-§105` ("a recipe is published by someone who ran it" extends to "…in the SHELL the recipe names"); `SC-§101` (the census STOPPED on the beyond-set hits rather than classify them — the ruling came from the manager, not the instrument).

## 4. Fences honoured

- ⛔ Nothing outside `ACC-§11` / `SC-§39` in `CONVENTIONS.md`: the two `Edit` anchors were the last line of each section's existing text; no other section was touched.
- ⛔ No value: the only credential-shaped strings in either clause are the regex patterns themselves and the two `ENGINE` paths + their KEY NAMES. No match text, no length that identifies a value beyond what the handoff already published.
- Read-only on `Config/SiegeCloudDev.ini`: never opened by this row (the manager's `Grep` honours `.gitignore`; the file's contents were never needed).

## 5. Board edits made on this pass

- `TASK-1278` `status:` → `done — written 2026-09-14 … (host TASK-1279 — flips to done — committed <hash> on its edit)`; `blocked-by` annotated CLEARED (`bbee7d9` + `cff9807`).
- `TASK-1279` `blocked-by:` → `none — ⛔ dispatchable NOW (amended 2026-09-14, manager): 1275 qa-passed ✅ qa/TASK-1276-report.md · 1278 ✅ (handoffs/TASK-1278-manager.md — @@ = 2, both hunks in CONVENTIONS.md) · 1272 ✅ bbee7d9`, with the seven carries listed verbatim and the four-row flip instruction.

## 6. For `TASK-1279` (build-master) — the pre-flight this handoff promises

Expected `git status --porcelain` from the git root (ONE LEVEL UP, `SC-§102`), and nothing else:
```
 M GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1275-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1276-report.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1278-manager.md
```
`git diff -U0 -- GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md | grep -c '^@@'` → **2**. If it reads anything else, a third author has written to the file since this handoff — NAME it, do not stage it. Run `export -f rg` before any child shell for read-back (a).
