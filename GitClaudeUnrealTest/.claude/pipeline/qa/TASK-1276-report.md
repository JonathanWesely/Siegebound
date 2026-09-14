# QA Report — TASK-1276 — Verdict: PASS — blockers: 0 (warn 0, nit 0) — subject: TASK-1275

subject: TASK-1275 — [CLOUD-EXAMPLE-DBPASSWORD-RETIRE]
gate: TASK-1276 — [CLOUD-EXAMPLE-GATE] (qa-reviewer, 2026-09-14)
law: `ACC-§11` (2026-09-13 amendment, R15) · `SC-§38a` · `SC-§71b` · `SC-§97` · `SC-§106`
host: TASK-1279 re-measures acceptance (1) and the staged diff.

## Provenance (what I did and did not do)

- ⛔ **No `Bash` was used.** Every git-side number below (diff-stat, numstat, exit codes) is the programmer's declaration, ACCEPTED-AS-DECLARED under `SC-§71b`; `TASK-1279` re-measures on stage.
- `Read` (whole file, `SC-§38a`): `Config/SiegeCloudDev.ini.example`, `handoffs/TASK-1275-programmer.md`, `CONVENTIONS.md:6505–6510`, `TASKBOARD.md:3099–3118`.
- `Grep` (ripgrep, project-side): the template, the handoff, `Source/`, `CONVENTIONS.md`.
- `unreal_inspector` read-only Python (one call): opened the TEMPLATE in binary mode and counted bytes/LF/CR — the one measurement `Read` cannot make. Nothing mutated; no editor lifecycle tool touched.
- ⛔ `Config/SiegeCloudDev.ini` (the real, gitignored file) was NOT opened, read, grepped, or byte-counted by me. The `ACC-§11` criterion's "BOTH files" clause is measured here on the `.example` only; the real file is `TASK-1266`'s standing census / Jonathan's `TASK-1277`.
- ⛔ No credential value appears in this report; the only quoted strings are the angle-bracket placeholder shapes.

## Checks (row spec, in order)

| # | Check | Method | Result |
|---|---|---|---|
| (1) | no line matches `^\s*;?\s*DbPassword=` | count by eye over all 15 lines = **0**; `Grep` on that regex = **0**; case-insensitive `DbPassword\|custody` = **0**; raw bytes `has_DbPassword=False` | ✅ 0 — the handoff's `rg` zero (exit 1) accepted-as-declared, `TASK-1279` re-measures |
| (2) | `ProjectUrl="https://<project-ref>.supabase.co"` then `AnonKey=<anon-or-publishable-key>`, verbatim, in that order | `Read` lines 14 and 15 | ✅ verbatim; `ProjectUrl` still double-quoted (2026-08-23 rider stands), `AnonKey` unquoted; `AnonKey=` is the LAST line of the file |
| (3) | `service_role` forbidding comment present | `Read` line 6: `; ⛔ The service_role key NEVER goes in any file, anywhere (ACC-§11).` | ✅ present, unchanged; it names no credential (row ruling) |
| (4) | file = committed `.example` minus exactly one line; no second deletion, no added line | `Read` = 15 content lines (was 16 per handoff + board §2.7); handoff hunk `@@ -13,4 +13,3 @@` shows context 13–15 matching my read byte-for-byte and one `-` line; handoff diff-stat `1 file changed, 1 deletion(-)`; byte count `bytes=1002 LF=15 CR=0 ends_with_LF=True` — no CRLF churn, trailing newline preserved | ✅ 16 → 15; the git-side diff-stat is accepted-as-declared (no Bash) — expect `0 1` on `numstat` at `TASK-1279` |
| (5) | no `eyJ`, no `sb_secret_`, no 3-segment JWT in template or handoff | `Grep` `eyJ\|sb_secret_\|<20+>.<20+>.<20+>` on template = **0**, on handoff = **0**; raw bytes `has_eyJ=False has_sb_secret=False` | ✅ 0 / 0 |
| (6) | real ini not opened by me | — | ✅ not opened |

Supplementary:
- **Neighbouring comment references to the deleted key:** none. Lines 1–13 mention ProjectUrl quoting, the anon key's ruled home, and the `service_role` prohibition; no line says "custody" or "DbPassword" (case-insensitive count 0), so no clause needed rewording and none was — consistent with the handoff's "lines 1–15 byte-identical".
- **`ACC-§11` on disk:** `CONVENTIONS.md:6507` carries the struck-through `; DbPassword=` custody allowance marked RETIRED 2026-09-13 (R15); `:6509` is the CONTENTS RULE amendment whose QA criterion is `rg --no-ignore -c '^\s*;?\s*DbPassword='` = 0 on BOTH files. On `Config/SiegeCloudDev.ini.example` that criterion now reads **0** (was 1 per `TASK-1266` §2.7). The retained lines are exactly the amendment's allowed set: the section header, quoted `ProjectUrl`, unquoted `AnonKey`, and comment lines naming no credential.
- **Code reach:** `Grep 'ini\.example'` over `Source/` = 0 files — nothing in code or tests reads the template; no compile, no suite implicated. Matches the row's blocked-by rationale and the handoff's re-measure.
- **Fences (handoff §4):** declared — real ini never opened, `Saved/**` untouched (Aura index copy goes stale until Jonathan's next Delete Previous Index + Sync Files, expected), nothing staged. Accepted-as-declared; the `git status` snapshot at my session start shows ` M Config/SiegeCloudDev.ini.example` and no `SiegeCloudDev.ini` entry, consistent with it.

## Findings

- none — 0 BLOCKER, 0 WARN, 0 NIT.

## Notes for build-master (TASK-1279)

- Stage `GitClaudeUnrealTest/Config/SiegeCloudDev.ini.example` from the git root (one level up, `SC-§102`); expect `git diff --cached --numstat` = `0	1` on it and the autocrlf "LF will be replaced by CRLF" warning (benign — working copy measured CR=0, LF=15).
- Re-measure acceptance (1) yourself: `rg --no-ignore -c '^\s*;?\s*DbPassword=' Config/SiegeCloudDev.ini.example` → empty output, exit 1 (quote `0`).
- Do not stage `Config/SiegeCloudDev.ini`; it must not appear in `git status` at all.
