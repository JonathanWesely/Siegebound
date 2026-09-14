# QA Report — TASK-1263 — Verdict: PASS
subject: TASK-1262 — [AURA-SETUP-DOCS-D-2] `Docs/setupdirections.md` Appendix D.1 inspector wildcard → 49 names; D.4 + the four §11 "read-only / wholesale" sentences; (+ the vault copy)
Blockers: 0 · Warns: 0 · Nits: 2
Reviewer: qa-reviewer · 2026-09-13 · the re-gate ordered by ruling R10 (`TASKBOARD.md:2353`) over `handoffs/TASK-1262-programmer.md`; `TASK-1249` is SPENT (it passed the pre-R10 file) and is not re-run here.

## What I read / measured
- `.claude/pipeline/TASKBOARD.md` — TASK-1262 row (2901–2910), TASK-1263 row (2912–2920), ruling R10 (2353) and the dispatch order (2359).
- `CONVENTIONS.md` `VER-§7` cl. 1–6 as amended 2026-09-13 (11728–11734) — cl. 3 is the law this row executes.
- `handoffs/TASK-1262-programmer.md` (whole) · `handoffs/TASK-1254-buildmaster.md` (whole — the 49-name block at 30–78 and the 13-excluded block at 83–95, the source D.1 mirrors) · `handoffs/TASK-1222-buildmaster.md:24–58` (the 32-name grant) · `qa/TASK-1249-report.md` (whole — WARN-1 and WARN-2, the two findings this row closes).
- `Docs/setupdirections.md` lines 860–904 (§11.4 step 3, §11.5) and 1000–1179 (Appendix D in full, to EOF) by Read; lines 241–242 and 333–334 (the two QA-posture sites) by Read; whole-file Greps quoted below.
- **Inspector used (read-only Python lane, `execute_unreal_python_readonly`): sha256 / byte size / LF / CR / BOM / tail bytes of BOTH copies, plus the in-block (lines 1011–1148) byte counts of both wildcard tokens and both `"mcp__…__` name prefixes on both copies.** Two files read, nothing written; no asset, graph or setting touched; no engine-lifecycle tool called.
- No `Bash`, no `git diff` — hunk confinement is a TEXT-LEVEL verdict (item 4 below); the cumulative-vs-HEAD diff is the host's measurement.

## Acceptance, item by item

### (1) ⛔ No wildcard of EITHER server inside the D.1 JSON block — PASS, 0 / 0, quoted
D.1 fenced block = lines **1011–1148** (measured: line 1011 = ```` ```jsonc ````, line 1148 = ```` ``` ````). Whole-file Grep `unreal_inspector__\*|unreal_editor__\*`, repo copy:
```
895:- `mcp__unreal_inspector__*` is ⛔ **NEVER allowed wholesale** (R10, 2026-09-13 — the server
900:- ⛔ **Never `mcp__unreal_editor__*`.** A wildcard there hands every agent C++ authoring,
976:  - **The real `mcp__unreal_editor__*` tool names** — obtainable only from `/mcp` after the
```
→ **0 hits inside 1011–1148** for either token. The three whole-file hits are prose: 895 is §11.5's new ban (the token is the SUBJECT of the ban), 900 is §11.5's pre-existing editor ban, 976 is the Appendix B namespace reference already ruled in `TASK-1238`. Corroborated byte-level via the inspector lane on both copies: `insp_wild_in_block=0 · ed_wild_in_block=0`. The vault twin's whole-file Grep returns the same three lines at the same numbers.

### (2) The 49 inspector names ↔ `TASK-1254`'s block; the 32 editor names ↔ `TASK-1222`'s block; the 13 excluded — PASS, SETS EQUAL
- Grep `-o "mcp__unreal_inspector__[A-Za-z_]+"` over the repo file → **exactly 49 hits, all inside the block at 1063–1111**, none elsewhere in the file. Compared one-for-one against `handoffs/TASK-1254-buildmaster.md:30–78`: D.1 line *n* (1063 ≤ *n* ≤ 1111) carries the identical string to handoff line *n − 1033* (1063 ↔ 30 `execute_unreal_python_readonly` … 1088 ↔ 55 `grep` … 1089 ↔ 56 `import_ActorComponentsAndSubobjects_understanding` … 1111 ↔ 78 `search_geometry_scripts`), same strings, same order. **`missing = []`**, nothing in D.1 the grant lacks, nothing in the grant D.1 lacks. In-block count via the inspector lane: `insp_names_in_block=49` on both copies.
- Grep `-o "mcp__unreal_editor__[A-Za-z_]+"` → **exactly 32 hits, all inside the block at 1112–1143**, none elsewhere. Same strings, same order, as `handoffs/TASK-1222-buildmaster.md:26–57` and as `qa/TASK-1249-report.md`'s 32-row table shifted by exactly +53 (1059→1112 … 1090→1143). The 32 are untouched. `ed_names_in_block=32` on both copies.
- **The 13 excluded (census §2a, `TASK-1254:83–95`):** `cancel_operation · create_or_edit_plan · derive_normal_map · edit_images · generate_images · generate_model_from_image · generate_model_from_text · get_recent_generated_images · launch_unreal_project · lock_plan_layer · recompile_unreal_project · rig_model_from_mesh · shutdown_headless`. Whole-file alternation Grep → **ONE hit, line 896**: `carries \`recompile_unreal_project\` and engine-lifecycle tools); the 49 read names are` — prose inside §11.5's ban, outside the block, not a `"mcp__…"` entry. **0 of the 13 inside the block; 0 granted. `forbidden = []`.**
- `enabledMcpjsonServers` — line 1146: `["unreal-mcp", "blender", "unreal_inspector", "unreal_editor"]`, the four names, unchanged (Grep hits 865, 1009, 1146, 1173 — the same four sites as before, +53 where below the block). Block entry tally 27 + 49 + 32 = 108, as the handoff's parse reported.

### (3) The four `TASK-1249` WARN-1 sites re-read — PASS; nothing left calls the inspector server read-only or its grant wholesale
Whole-file Grep `-i read-only` → **4 hits**, quoted in full:
```
242:| **qa-reviewer** | Reviews code BEFORE it compiles; writes pass/fail reports to `qa/` | Editing code, engine, Git | Read-only + report Write + Slack |
334:in CONVENTIONS before the task is issued; QA is read-only; only the integrator touches git.
1025:      // Read-only probes (zero risk, highest frequency)
1030:      // Read-only git ONLY — enumerated deliberately. ⛔ NEVER "git *":
```
242 = the agent table's qa-reviewer row (QA's POSTURE); 334 = the spec-fence invariant (QA's POSTURE, the site the spec says to leave); 1025 = the PowerShell probe comment; 1030 = the read-only-git comment. **None describes the `unreal_inspector` server or its grant.** I read none of the four as being about the server.

The four former sites, read against the spec's replacement text:
- **§11.4 step 3 → `:883–885`**: *`unreal_inspector` is MOSTLY read tools — the census found 13 that are not (engine lifecycle incl. a recompile path, generation, plan bookkeeping), so it is granted by NAME, 49 tools, never wholesale (`VER-§7` cl. 3, R10);* — the spec's sentence verbatim, re-wrapped; `unreal_editor`'s sentence (886–887) unchanged.
- **§11.5 first bullet → `:895–897`**: *`mcp__unreal_inspector__*` is ⛔ **NEVER allowed wholesale** (R10, 2026-09-13 — the server carries `recompile_unreal_project` and engine-lifecycle tools); the 49 read names are enumerated in Appendix D.1.* — the spec's sentence verbatim.
- **D.1 comment → `:1055–1062`**: *both servers ENUMERATED (VER-§7 cl. 1 + cl. 3, R10): unreal_inspector = its 49 read tools (census §2 minus §2a), as TASK-1254 granted them … The 13 lifecycle / generation / plan names are granted to no agent. unreal_editor = the 32 PIE / verify / screenshot / input names, as TASK-1222 granted them … ⛔ Never a wildcard of either.* — carries every clause of the spec's text plus the two handoff cites; the wildcard entry that followed it is gone, replaced by the 49 entries.
- **D.4 bullet → `:1173`**: *`unreal_inspector` is allowed only by its **49 enumerated read tools** (never wholesale — the census found 13 lifecycle / generation / plan names inside it, excluded per R10; the list is in D.1)* — the spec's "(49 read tools, enumerated)" in fuller form; the `unreal_editor` half of the bullet reads as the mutating-server description with "allowed only by the enumerated names in D.1 — never by wildcard (§11.5)", consistent with my TASK-1249 read of the old 1120.

Whole-file Grep `-i wholesale` → 885, 895, 1173 — every hit is the phrase **never wholesale / NEVER allowed wholesale**; no line describes the grant AS wholesale.

### (4) Hunks confined to §11.4–§11.5 / D.1 / D.4 — PASS (text-level; git-level owed to host `TASK-1242`)
Handoff declares four own hunks: `@@ -883 +883,3` (+2) · `@@ -893 +895,3` (+2) · `@@ -1051,8 +1055,57` (+49) · `@@ -1120 +1173` (0) = 64 ins / 11 del, net **+53**, 1,126 → 1,179.
- The line delta MEASURED (LF count 1126 in `TASK-1249` → 1179 now) = +53 = the hunk sum. This time the two counts reconcile exactly (contrast `TASK-1249` WARN-2).
- Every anchor I hold from `TASK-1249` moved by the amount the four hunks predict and nothing else: lines 242 / 334 (below every hunk) unmoved; §11.5 heading 891 → 893 (+2, after hunk 1); the D.1 heading 1003 → 1007, block open 1007 → 1011, the `read-only` comments 1021/1026 → 1025/1030 (+4, after hunks 1+2); block close 1095 → 1148, `enabledMcpjsonServers` 1093 → 1146, the 32 editor names 1059–1090 → 1112–1143, D.4 bullet 1120 → 1173, D.5 heading 1124 → 1177, D.5 body 1126 → 1179 (+53, after all four). The pre-existing secrets-grep sites (34 … 792) are at the same numbers as in `TASK-1249`.
- Lines 1000–1054, 1112–1172, 1174–1179 and 860–882, 886–892, 898–904 read word-identical to my `TASK-1249` read of the same passages (modulo the shift).
- **Limit:** I cannot see a diff. The programmer's CUMULATIVE reading vs HEAD `9f2c990` — `104 ++…--` (99 ins / 5 del), 5 hunks (`-883`, `-893`, `-1049,+1053,91`, `-1052,+1146`, `-1079,+1173`), 0 `No newline` markers — is internally consistent with TASK-1243's declared 44/3 plus this row's 64/11 merging on the shared D.1/D.4 lines, but it is ACCEPTED AS DECLARED (`SC-§71b`); `TASK-1242` observes it before committing by pathspec.

### (5) The hash pair — MEASURED (not accepted-as-declared), identical
Inspector read-only Python, both files read in binary:
```
setupdirections.md                | sha256=ae48dc8ef6cb769efebcb77677933a5f068c47edd01035ee557748ded731e425 | bytes=73427 | LF=1179 | CR=0 | BOM=False | tail=652e0a
GitClaudeUnrealsetupdirections.md | sha256=ae48dc8ef6cb769efebcb77677933a5f068c47edd01035ee557748ded731e425 | bytes=73427 | LF=1179 | CR=0 | BOM=False | tail=652e0a
IDENTICAL=True
```
Matches the handoff's declared pair (`ae48dc8e…1e425`, 73,427 B, 1,179 lines, 0 CR, no BOM, tail `surface.\n`) exactly. Because the two copies are byte-identical, every block measurement in (1)–(3) holds for the vault twin too (its own Grep: 81 = 49 + 32 quoted names; the same three wildcard prose lines at 895/900/976). `TASK-1242` still re-measures at commit time per `SC-§68`.

### (6) `TASK-1249` WARN-2 (the possible EOF-blank hunk) — DISSOLVED, not re-litigated
The spec says this is `1242`'s by `git diff --stat`; recorded here only so the finding is closed where it was opened. What I can measure: the tail is `65 2e 0a` (`surface.\n`) on both copies, unchanged from `TASK-1249`; the handoff's hunk arithmetic reconciles with the measured line delta (+53 = +53) — the one-line discrepancy that raised WARN-2 does not recur. What the programmer measured and I accept as declared: `git diff -U0 9f2c990` shows 5 hunks, none at the tail, 0 `\ No newline` markers, and the 5 deletions are exactly the 5 changed lines — so HEAD already ended `surface.\n`, TASK-1243's `44/3` was the true reading, and my "dropped trailing blank line" inference was wrong (the 1238 Read's apparent line 1086 was not a byte in the file). Nothing was hand-restored — correct, since a restore would have re-drifted the hash pair. Host: expect `99/5`, 5 hunks, 0 markers; anything else in that file is not this lane's.

### (7) No secrets — PASS; the false positive confirmed, case-sensitive 0
Whole-file case-insensitive Grep `service_role|api[_ ]key|token=|sbp_|eyJ|hf_[A-Za-z]|ghp_|sk-[A-Za-z0-9]|Bearer |access_token` → 17 hits: 34, 40, 62, 512, 517, 566, 585, 587, 590, 733, 735, 764, 792 (all pre-existing Chapters 1/7/10/11 sites, same numbers as `TASK-1249`) + **4 inside the hunks: 1056, 1057, 1059, 1060** — the D.1 comment lines citing `TASK-1254` / `handoffs/TASK-1254-buildmaster.md` / `TASK-1222` / `handoffs/TASK-1222-buildmaster.md`, where `sk-[A-Za-z0-9]` matches the `SK-1` case-insensitively. **Case-sensitive Grep `sk-[A-Za-z0-9]` → 0.** The additions are tool-name strings, prose and handoff-path cites.

## Findings
- **[NIT-1]** `Docs/setupdirections.md:885`, `:896`, `:1056`, `:1059`, `:1173` — the list lengths `49` and `32` now appear as literals in five prose/comment sites (up from `TASK-1249` NIT-1's one). They are the spec's own wording and a doc, not code; but the next census that changes either set by one name has five sites to keep honest. Leave; recorded for whoever edits the lists next.
- **[NIT-2]** `handoffs/TASK-1262-programmer.md:30` — "BEFORE (8 hits)" is followed by NINE line numbers (`242 334 883 893 1021 1026 1051 1053 1120`); nine is the true pre-edit count (`TASK-1249` WARN-1 named 883/893/1053/1120 plus the 1051 comment line, and the four non-server sites). A tally slip in the handoff's prose, not in the file (`SC-§104`); the AFTER list (4) is measured correct above.

## Limits on this verdict's provenance
- Hunk confinement is text-level (Greps + Reads against my own `TASK-1249` baseline and the handoff's hunk list), not diff-level. `TASK-1242`'s `git diff --stat 9f2c990 -- GitClaudeUnrealTest/Docs/setupdirections.md` is the diff-level measurement (`SC-§102`: run from the git root one level up).
- The hash pair, the in-block wildcard counts and the in-block name counts on BOTH copies ARE measured by me via the inspector's read-only Python lane; it read two files and wrote nothing.
- The 49 and the 13 were compared against the `TASK-1254` handoff blocks, which the board names as the source D.1 mirrors; `handoffs/AURA-MCP-CENSUS.md` itself was not opened here.
- The D.4 bullet's `unreal_editor` half is "unchanged" by the handoff's word and by consistency with my `TASK-1249` read; I did not hold a verbatim copy of the old line 1120 to diff against.

## Notes for build-master (TASK-1242)
- `Docs/setupdirections.md` is cleared to stage ONLY after this PASS; commit it by pathspec; the vault twin is outside the repo and is never staged.
- Re-measure both hashes immediately before the commit; expected `ae48dc8ef6cb769efebcb77677933a5f068c47edd01035ee557748ded731e425` / 73,427 B on both — measured identical by me at gate time. A mismatch means a copy drifted after this gate; route back, do not copy over.
- Expected `git diff --stat 9f2c990` for this file: `104 ++…--` (99/5), 5 hunks, 0 `No newline` markers, every hunk between line 883 and line 1173 (§11.4 step 3 … D.4). Any hunk outside that band in this file is not this lane's and must not ride its commit. `TASK-1249` WARN-2 is closed above; no EOF rider is expected.
- After the commit, the file's D.1 block and `.claude/settings.local.json`'s `permissions.allow` should agree on both name sets (49 + 32); `TASK-1254`'s greps are the check on the settings side.
