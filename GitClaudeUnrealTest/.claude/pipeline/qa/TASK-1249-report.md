# QA Report — TASK-1249 — Verdict: PASS
subject: TASK-1243 — [AURA-SETUP-DOCS-D] `Docs/setupdirections.md` Appendix D.1 + D.4 (+ the vault copy)
Blockers: 0 · Warns: 2 · Nits: 2
Reviewer: qa-reviewer · 2026-09-13 · gate over `handoffs/TASK-1243-programmer.md` · ruling R2 (`TASKBOARD.md:2343`) answering `qa/TASK-1238-report.md` WARN

## What I read / measured
- `.claude/pipeline/TASKBOARD.md` — WAVE-1 GATE RULINGS block (2341–2352), TASK-1243 row (2677–2686), TASK-1249 row (2743–2751).
- `handoffs/TASK-1243-programmer.md` (whole) · `handoffs/TASK-1222-buildmaster.md` (whole — the GRANT, the source D.1 must mirror) · `qa/TASK-1238-report.md` (whole — the WARN this row answers).
- `Docs/setupdirections.md` lines 860–901 (§11.4 step 3, §11.5) and 990–1126 (Appendix D in full, to EOF) by Read; whole-file Greps quoted below.
- `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md` lines 990–1126 by Read + the same Greps.
- **Inspector used (read-only Python lane, `execute_unreal_python_readonly`): sha256 / byte size / LF / CR / BOM of BOTH files.** No asset, graph, or setting touched; no engine-lifecycle tool called. This is the one measurement my earlier report (`TASK-1238`) had to accept as declared and this one does not.
- No `Bash`, no `git diff` — hunk confinement is a TEXT-LEVEL verdict (see item 1).

## Acceptance, item by item

### (1) Hunks confined to Appendix D.1 + D.4 — PASS (text-level; git-level owed to host `TASK-1242`)
Handoff declares three hunks: `@@ -1049 +1049,42 @@` (D.1 comma + comment + inspector wholesale + 32 names), `@@ -1052 +1093 @@` (D.1 `enabledMcpjsonServers`), `@@ -1079 +1120 @@` (D.4 bullet); `47 +++…--` = 44 ins / 3 del, net +41.
- Consistent with what I can measure, with ONE tally that does not close: `TASK-1238` read D.1 at 1003 / the two-server line at 1052 / the D.4 bullet at 1079 / the D.5 heading at 1083 / the D.5 body at 1085 in a 1086-line file; now the same anchors sit at 1003 / 1093 / 1120 / 1124 / 1126 in a 1126-line file. Every anchor below 1049 shifted by exactly +41 (= hunk 1's 42 new lines for 1 old: the changed Slack line + blank 1050 + comment 1051–1057 + 1058 + 32 names), everything at or above 1049 is unmoved. But the FILE grew by only +40 (1086 → 1126 LF — the programmer's own two counts, and 1126 LF measured by me), and the file now ends `surface.\n` with NO trailing blank line (tail bytes `…65 2e 0a`, measured on both copies). So the old line 1086 — a trailing blank line after the D.5 body, by the 1238 read — no longer exists. That is a fourth, whitespace-only change at EOF that the handoff's three-hunk / 3-deletion list does not declare. See WARN-2; `git diff` decides it.
- Every hit of `unreal_editor` / `unreal_inspector` / `enabledMcpjsonServers` / `TASK-1222` in the file (Grep, quoted): 865, 870, 873, 883, 886, 893, 896, 940, 972 (all pre-existing Chapter 11 / Appendix A / Appendix B sites, unchanged in wording vs my TASK-1238 read) + 1005 (pre-existing D.1 lead-in) + **1052–1058, 1059–1090, 1093, 1120** (the three declared hunks). Zero hits in D.2, D.3, D.5, or any chapter.
- Lines 990–1050, 1094–1119, 1121–1126 read word-identical to my TASK-1238 read of Appendix D. Chapter 11 §11.4/§11.5 (860–899) read unchanged — the handoff's "§11.4 / §11.5 themselves are NOT edited" holds.
- Limit: I cannot see a diff. `TASK-1242` should observe `git diff --stat HEAD -- Docs/setupdirections.md` = `47 +++…--` and NOTHING else changed in that file before committing it by pathspec.

### (2) ⛔ No `mcp__unreal_editor__*` inside the D.1 JSON block — PASS, quoted
Whole-file Grep `unreal_editor__\*`, repo copy:
```
896:- ⛔ **Never `mcp__unreal_editor__*`.** A wildcard there hands every agent C++ authoring,
972:  - **The real `mcp__unreal_editor__*` tool names** — obtainable only from `/mcp` after the
```
→ **0 hits in the block's range 1007–1095**; the two hits are the pre-existing §11.5 ban and the Appendix B nit already ruled a namespace reference in `TASK-1238`. Same two lines, same numbers, in the vault copy. The handoff's `sed -n '1007,1095p' | grep -c` → 0 is corroborated.

### (3) Every `mcp__unreal_editor__<name>` in the D.1 block ↔ `TASK-1222`'s granted list — PASS, SETS EQUAL, 32 = 32
Grep `-o mcp__unreal_editor__[a-z_]+` over the repo file returns EXACTLY 32 hits, all inside the block (1059–1090), none anywhere else in the file. The same regex over `handoffs/TASK-1222-buildmaster.md` returns the fenced grant block at 26–57 (32) plus its pairs table at 65–96 (the same 32). Triples (D.1 line · handoff line · name), identical string on both sides, same alphabetical order:

| # | D.1 | 1222 | name |
|---|---|---|---|
| 1 | 1059 | 26 | `mcp__unreal_editor__attach_pie_frames` |
| 2 | 1060 | 27 | `mcp__unreal_editor__capture_pie_frame` |
| 3 | 1061 | 28 | `mcp__unreal_editor__get_actor_by_name_in_pie` |
| 4 | 1062 | 29 | `mcp__unreal_editor__get_actor_property_in_pie` |
| 5 | 1063 | 30 | `mcp__unreal_editor__get_input_mapping_context_keys` |
| 6 | 1064 | 31 | `mcp__unreal_editor__get_player_transform` |
| 7 | 1065 | 32 | `mcp__unreal_editor__get_screenshot_of_objects_for_verification` |
| 8 | 1066 | 33 | `mcp__unreal_editor__get_widget_property_in_pie` |
| 9 | 1067 | 34 | `mcp__unreal_editor__inject_input_action` |
| 10 | 1068 | 35 | `mcp__unreal_editor__is_pie_active` |
| 11 | 1069 | 36 | `mcp__unreal_editor__load_level` |
| 12 | 1070 | 37 | `mcp__unreal_editor__record_burst` |
| 13 | 1071 | 38 | `mcp__unreal_editor__run_verification_sequence` |
| 14 | 1072 | 39 | `mcp__unreal_editor__set_player_transform` |
| 15 | 1073 | 40 | `mcp__unreal_editor__simulate_button_press` |
| 16 | 1074 | 41 | `mcp__unreal_editor__simulate_key_press` |
| 17 | 1075 | 42 | `mcp__unreal_editor__simulate_left_stick` |
| 18 | 1076 | 43 | `mcp__unreal_editor__simulate_right_stick` |
| 19 | 1077 | 44 | `mcp__unreal_editor__start_pie` |
| 20 | 1078 | 45 | `mcp__unreal_editor__start_pie_recording` |
| 21 | 1079 | 46 | `mcp__unreal_editor__start_state_recording` |
| 22 | 1080 | 47 | `mcp__unreal_editor__stop_pie` |
| 23 | 1081 | 48 | `mcp__unreal_editor__stop_pie_recording` |
| 24 | 1082 | 49 | `mcp__unreal_editor__stop_state_recording` |
| 25 | 1083 | 50 | `mcp__unreal_editor__survey_pie_scene` |
| 26 | 1084 | 51 | `mcp__unreal_editor__take_editor_screenshot` |
| 27 | 1085 | 52 | `mcp__unreal_editor__ui_perform` |
| 28 | 1086 | 53 | `mcp__unreal_editor__ui_snapshot` |
| 29 | 1087 | 54 | `mcp__unreal_editor__ui_wait_for` |
| 30 | 1088 | 55 | `mcp__unreal_editor__verification_agent` |
| 31 | 1089 | 56 | `mcp__unreal_editor__wait_pie_frames` |
| 32 | 1090 | 57 | `mcp__unreal_editor__wait_pie_seconds` |

Nothing in D.1 that the grant lacks; nothing in the grant that D.1 lacks. `"mcp__unreal_inspector__*"` present exactly once in the block (1058); the file's only other `mcp__unreal_inspector__*` is the pre-existing §11.5 line 893. Block entry tally: 27 pre-existing + 1 + 32 = 60, as the handoff's parse reported; the Slack entry (1049) carries its new trailing comma so the array stays valid JSON.

### (4) `enabledMcpjsonServers` = exactly the four names — PASS
Line 1093: `"enabledMcpjsonServers": ["unreal-mcp", "blender", "unreal_inspector", "unreal_editor"]`. Same four in D.4's bullet (1120) and Appendix A row 13 (940); §11.4 step 3 (865) and §11.5's heading (891) now point at a D.1/D.4 that agree with them — the `TASK-1238` WARN is discharged with no residual contradiction between Chapter 11 and Appendix D.

### (5) The hash pair — MEASURED (not accepted-as-declared), identical
Inspector read-only Python, both files read in binary:
```
setupdirections.md                | sha256=208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58 | bytes=69979 | LF=1126 | CR=0 | BOM=False | first3=232053
GitClaudeUnrealsetupdirections.md | sha256=208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58 | bytes=69979 | LF=1126 | CR=0 | BOM=False | first3=232053
IDENTICAL=True
```
Matches the handoff's declared pair (`208edbec…80f58`, 69,979 B, 1,126 lines, 0 CR, no BOM, first bytes `# S`) exactly. `TASK-1242` still re-measures at commit time per `SC-§68` — a match here does not survive a later drift.

### (6) D.4's "read-only" wording vs census §2a — WARN (not a blocker), pending manager ruling R10
See WARN-1 below.

### (7) No secrets — PASS; the one false positive confirmed
Whole-file case-insensitive Grep `service_role|api[_ ]key|token=|sbp_|eyJ|hf_[A-Za-z]|ghp_|sk-[A-Za-z0-9]|Bearer |access_token` → hits at 34, 40, 62, 512, 517, 566, 585, 587, 590, 733, 735, 764, 792 (all pre-existing Chapters 1/7/10/11 sites outside Appendix D) and **one hit inside the hunks: 1052** — `// TASK-1222 granted it (handoffs/TASK-1222-buildmaster.md):`, where `sk-[A-Za-z0-9]` matches the `SK-1` in `TASK-1222` case-insensitively. Confirmed a false positive; case-sensitive it is not a hit. The additions are tool-name strings, a four-name server list, and prose.

## Findings
- **[WARN-1]** `Docs/setupdirections.md:1120` (D.4) and `:1053` (D.1 comment) — both describe `unreal_inspector` as **read-only**, and so do the pre-existing `:883` (§11.4) and `:893` (§11.5). The census §2a (recorded verbatim in `handoffs/TASK-1222-buildmaster.md:108–119`) MEASURED 13 non-read names inside that server — 4 engine-lifecycle (`launch_unreal_project`, `recompile_unreal_project`, `shutdown_headless`, `cancel_operation`), 7 generation/image/mesh, 2 plan-bookkeeping — including a COMPILE PATH under the wholesale grant. "Read-only" is therefore a claim the census refuted (`SC-§101`), now standing in FOUR sites of this doc. **Not a blocker against TASK-1243:** the row's spec dictates the wording verbatim ("`unreal_inspector` (read-only, wholesale)"), R2 orders D.1 to mirror what was GRANTED and the grant IS wholesale, and a rescope is a board edit (`SC-§100`), not a programmer's liberty. **Ruling requested from R10:** whichever way it lands — (a) keep wholesale with the body law as the fence, or (b) narrow to an enumerated inspector list minus the 13 — it owes ONE follow-up row over `Docs/setupdirections.md` touching all four lines (883, 893, 1053, 1120; under (b) also D.1's entry 1058 and §11.5's first bullet), both copies, hash pair re-measured. Under (a) the honest wording is "read-only BY LAW (the census found 13 non-read names; the fence is the agent body law, not the harness)". Recorded here so the next reader does not "fix" one site and leave three.
- **[WARN-2]** `Docs/setupdirections.md` EOF (old line 1086 → gone) vs `handoffs/TASK-1243-programmer.md:37–44` — the declared diff (`44 ins / 3 del`, three hunks, net +41) and the declared line counts (`1,086 → 1,126`, net +40) disagree by one line, and the measured tail (`surface.\n`, no trailing blank line, both copies) says which: the old trailing blank line after the D.5 body was dropped. A whitespace-only EOF change is not a chapter or appendix edit and changes no reader-visible text, so it is not a blocker — but it is a hunk outside D.1/D.4 that the handoff does not list, and it means either the `--stat` was misread (it would be `44/4`, four hunks) or the old count was wrong (`SC-§104`: the state is what matters; here the state itself is in question). **Decisive check for `TASK-1242` before committing:** `git diff --stat HEAD -- Docs/setupdirections.md` and `git diff HEAD -U0 -- Docs/setupdirections.md | grep -c '^@@'`. If it reads `44/3` with 3 hunks, my inference is wrong and this WARN dissolves. If it reads `44/4` with a 4th hunk at the tail (or a `\ No newline at end of file` marker), name the EOF-newline rider on the commit line so it is not silent — do NOT restore the blank line by hand (that would re-drift the hash pair). Either way the content verdict stands.
- **[NIT-1]** `Docs/setupdirections.md:1055` — the comment carries the literal `32 names`. It is a doc snippet, not code, but it is a duplicated literal of the list length; if the grant ever changes by one name the comment goes stale silently. Leave; note for whoever edits the list next.
- **[NIT-2]** `handoffs/TASK-1243-programmer.md:13` — the "1,126 lines" count holds (1126 LF measured, file ends with exactly one newline). It is the OLD count the handoff quotes (1,086) that does not reconcile with its own hunk list — WARN-2's subject. Recorded here only so the two counts are not both taken as measured: one of them, or the `--stat` reading, is off by one.

## Limits on this verdict's provenance
- Hunk confinement is text-level (Greps + Reads against my own TASK-1238 baseline read), not diff-level. `TASK-1242`'s `git diff --stat HEAD` is the diff-level measurement.
- The hash pair IS measured by me via the inspector's read-only Python lane; it is the single measurement in this report that came from the running editor's process, and it read two files and wrote nothing.
- `handoffs/AURA-MCP-CENSUS.md` was not opened by me either — D.1's source of truth is the GRANT (`TASK-1222`) per R2, and that is what I compared against. The census §2a facts I cite are as recorded verbatim inside the `TASK-1222` handoff.

## Notes for build-master (TASK-1242)
- Commit `Docs/setupdirections.md` ONLY, by pathspec; the vault twin is outside the repo and is never staged.
- Re-measure both hashes immediately before the commit; expected `208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58` / 69,979 B on both — measured identical by me at gate time. A mismatch means a copy drifted after this gate; route back, do not copy over.
- The `git diff --stat` for this file should read `47 +++…--` (44/3) and the three hunks should sit entirely between `### D.1` (1003) and `### D.5` (1124). If instead you see `44/4` and a fourth hunk at the tail (the dropped trailing blank line — WARN-2), it is whitespace only: commit it WITH the three, name it on the commit line, and do not hand-restore the blank line (it would re-drift the measured hash pair). Any OTHER change outside that band in this file is not TASK-1243's and must not ride its commit.
