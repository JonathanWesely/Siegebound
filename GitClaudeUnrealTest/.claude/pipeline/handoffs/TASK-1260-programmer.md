# TASK-1260 — [AURA-VERIFIER-BODY] gameplay-programmer handoff (2026-09-13)

**Status: ready-for-qa — gate `TASK-1261` (over `1255` + `1260` together); host `TASK-1242` stages the file (⛔ it is still UNTRACKED — no `git add` here).** Law: ruling R13 · `VER-§1` cl. 1–2 · `VER-§3` cl. 4 · `VER-§4` cl. 2 · `VER-§5` cl. 2 · `VER-§7` cl. 4 · registry note. Started from `TASK-1255`'s on-disk result (body hash `32eb680f…ff36e` re-measured BEFORE the first write — equal to 1255's and 1224's).

## What changed — the BODY of `.claude/agents/playtest-verifier.md`, five sites, ⛔ line 4 never
File 96 → 109 lines, LF, no BOM. Method: one Python script (scratchpad `task1260.py`) operating on bytes — each of the seven anchors asserted to occur EXACTLY once before replacement, line 4 asserted byte-equal before/after the write, the file read back and re-hashed on disk. Nothing typed by hand into the file. Line numbers below are the FINAL file's (the QA report's numbers were the 96-line file's).

### (1) WARN-1 — evidence filename → `VER-§4` cl. 2, at BOTH sites
**Step 6 prose (was ≈61–63, now 67–72). BEFORE:**
```
   `VER` prefix: `VER-###[-t<MM>m<SS>s]-<symptom>.png`, where `###` is the TASK number
   (not a sequential VID counter) and the optional `-t<MM>m<SS>s` is the PIE timestamp
   of the observation. Promote what the report cites — not everything you looked at.
```
**AFTER:**
```
   `VER` prefix: `VER-TASK-###[-a<N>][-t<MM>m<SS>s]-<observable-slug>.png`, where `###`
   is the TASK number (not a sequential VID counter), `-a<N>` = the attempt index
   (omitted when only one attempt exists — `VER-§1` cl. 6 keeps every attempt),
   `-t<MM>m<SS>s` = the PIE time of the frame when known, and `<observable-slug>` = the
   acceptance-table observable in kebab-case, ⛔ never the word "pass"/"fail" (`VER-§4`
   cl. 2). Promote what the report cites — not everything you looked at.
```
**Template `## Evidence (promoted)` line (was 77, now 86). BEFORE:**
`- .claude/pipeline/playtest-evidence/<YYYY-MM-DD>/VER-###[-t<MM>m<SS>s]-<symptom>.png — one-line pixel description`
**AFTER:**
`- .claude/pipeline/playtest-evidence/<YYYY-MM-DD>/VER-TASK-###[-a<N>][-t<MM>m<SS>s]-<observable-slug>.png — one-line pixel description`

### (2) WARN-2 — report template → `VER-§1` cl. 1–2 (Verdict on line 1, pilot suffix, SC-§118 command line)
**Fenced block head (was 71–73, now 80–82). BEFORE:**
```
# Verification — TASK-###
Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE
Editor/Aura state: <connected, map, PIE mode, attempts used>
```
**AFTER:**
```
Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE (advisory — VER-§6 pilot)
# Verification — TASK-###
Editor/Aura state: <connected y/n, map, PIE mode, editor instance identified by COMMAND LINE (SC-§118), attempts used of 3, wall time, credit if visible>
```
**Directly under the closing fence (new lines 90–92; before: nothing — the fence was followed by a blank line and `Verdict rules:`):**
```
Line 1 is the verdict, byte-literal — `head -1` of the report IS the verdict (`VER-§1`
cl. 1); the ` (advisory — VER-§6 pilot)` suffix stays until Jonathan rules the lane
binding (`VER-§6`).
```
Clauses quoted: `VER-§1` cl. 1 — *"Line 1 is the verdict, byte-literal … nothing precedes it (the H1 `# Verification — TASK-###` is line 2), so `head -1` of any report IS its verdict. During the `VER-§6` pilot the line carries the suffix ` (advisory — VER-§6 pilot)`."* · cl. 2 — *"`Editor/Aura state:` (connected y/n, map, PIE mode, editor instance identified by command line per `SC-§118`, attempts used of 3, wall time, credit if visible)."*

### (3) WARN-3 — `UNOBSERVABLE` status semantics → `VER-§5` cl. 2 (was 90, now 102–103)
**BEFORE:** `(UNOBSERVABLE leaves the status at `qa-passed` and appends `verify: unobservable`).`
**AFTER:**
```
(UNOBSERVABLE does NOT move the status — it stays `built` (C++) or `qa-passed`
(Blueprint/asset-only) — and appends `verify: unobservable`; `VER-§5` cl. 2).
```
This SHARPENS the Check-5 `built`/`qa-passed` override site; the step-2 eligibility sentence (now lines 48–52) is untouched.

### (4) WARN-4 (body half) — the inspector-lifecycle fence, `qa-reviewer.md` (`TASK-1227`) shape, after the line-11 law sentence (new lines 12–16; before: nothing)
```
⛔ NEVER call `unreal_inspector`'s engine-lifecycle tools (`launch_unreal_project`,
`recompile_unreal_project`, `shutdown_headless`, `cancel_operation`) or its generation /
plan tools — none of the 13 census-§2a names is on your `tools:` line and none is ever
called; process lifecycle is build-master's lane and generation is no pipeline agent's;
even if such a name were granted, calling it is a failed task (`VER-§7` cl. 4).
```
One sentence; names the four lifecycle tools; states both halves the dispatch asked for (the 13 are NOT granted — line 4 carries the 49-name enumeration per R10 — AND are never called). `recompile_unreal_project` now occurs on exactly ONE line of the file (this fence; line 4 does not carry it, `TASK-1255` `forbidden = []`).

### (5) NIT-2 — `VER-§3` cl. 4, end of the *Jonathan present* section (was line 37 `observed.`, now 42–43)
**BEFORE:** `…is a failed run, whatever it\nobserved.`
**AFTER:** `…is a failed run, whatever it\nobserved. If the editor is already in PIE when you look, that is his session — ⛔ never\nstop it; report and wait (`VER-§3` cl. 4).`
Clause quoted: `VER-§3` cl. 4 — *"If the editor is already in PIE when the verifier looks — that is his session: ⛔ never stop it, report and wait."*

## The `description:` (line 3) — NOT changed, recorded
It still reads *"Use when a code task is qa-passed …"* (`TASK-1235` NIT-4). Ruling R13: **LEAVE** — source-doc verbatim by `TASK-1223`'s spec; the body carries the `built` rule (lines 48–50 and 102–103). The row's "nothing else moves" list names the description explicitly. Line 3 is byte-identical before/after (asserted by the script).

## Acceptance — measured on the on-disk file AFTER the write
| # | check | result |
|---|---|---|
| 1 | five sites quoted before/after | above |
| 2 | `grep -c 'VER-TASK-###'` / `grep -c 'VER-###\['` | **2** / **0** |
| 3 | line directly after the Output fence's opening ``` | **line 80:** `Verdict: VERIFIED \| VERIFY-FAILED \| UNOBSERVABLE (advisory — VER-§6 pilot)` (fence opens at 79 under the `## Output` heading at 78) |
| 4 | `grep -c 'leaves the status at'` / `grep -c 'stays .built. (C++)'` | **0** / **1** |
| 5 | `grep -c 'recompile_unreal_project'` | **1** (the fence sentence, line 13) |
| 6 | `grep -c 'already in PIE'` | **1** (line 42) |
| 7 | line 4 byte-identical to `TASK-1255`'s | sha256 of line 4 (`sed -n 4p \| sha256sum`) **`0f49aa40027cd86196dc410c3b4f08d5389b97968cef9c7baebbb268f764c717`** BEFORE = AFTER; 3952 chars, 88 tokens, 49 `mcp__unreal_inspector__` + 32 `mcp__unreal_editor__`; `grep -c 'mcp__unreal_editor__\*'` **0**, `grep -c 'mcp__unreal_inspector__\*'` **0** |
| 8 | `grep -c '1783116269.740549'` / `'🎮 VERIFIER:'` / `'TASK-###-verify.md'` | **1** / **1** / **3** (lines 15→now 20, 78, 108) |
| 9 | `git status --porcelain -- GitClaudeUnrealTest/.claude/agents/playtest-verifier.md` (from the git root one level up, `SC-§102`) | `?? GitClaudeUnrealTest/.claude/agents/playtest-verifier.md` — still UNTRACKED, never staged |

**Body hash** (`sed '4d' file \| sha256sum`, i.e. file minus line 4):
```
BEFORE (= TASK-1255 / TASK-1224)  32eb680f0eef6067f0157e2b9f5ef7e026bd7327923b3fc9a8a8b35f8f3ff36e
AFTER                             660d8a70bbf7b7b487394efbe22bb85d04212e2af721ca6a74b11b7c339633b8
```
Everything else in the body is unchanged: the seven replacements above are the ONLY diff (the script replaces exact anchors and nothing else; a `diff` of the pre-write bytes vs post-write bytes contains exactly those hunks — lines 11/12–16 (insert), 41–43, 59–63→65–72, 71–73→80–82, 77→86, 89 (insert 90–92), 90→102–103). The six `TASK-1235` Check-5 override sites (thread ts, `🎮 VERIFIER:` prefix, `qa/TASK-###-verify.md` path, the verdict triple, the `built`/`qa-passed` rule, the `VER` prefix) all stand.

## For QA (`TASK-1261`) to scrutinize
- Re-derive the five sites character-exact against `CONVENTIONS.md` `VER-§1` cl. 1–2 (:11687–11688), `§3` cl. 4 (:11705), `§4` cl. 2 (:11710), `§5` cl. 2 (:11717), `§7` cl. 4 (:11732) — the slug wording in site (1) is the law's, the template line 82 is cl. 2 verbatim with the row's capitalisation of COMMAND LINE.
- Line 4 vs `handoffs/TASK-1255-programmer.md`: same sha256 as quoted there is not recorded (1255 quoted chars/tokens, not a line hash) — re-measure 3952 / 88 / 49 / 32 and the two wildcard zeros yourself.
- `grep -c 'TASK-###-verify.md'` = 3, not 2 — the third is the scoped-Edit paragraph (line 20); it was 3 before this row too (report lines 15/69/95).
- The fence sentence at 12–16 says "generation / plan tools" where the row literal said "generation tools" — the plan-bookkeeping pair (`create_or_edit_plan`, `lock_plan_layer`) is in the census §2a thirteen and `VER-§7` cl. 3 names it; flag if you read that as more than the row allowed.

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · line 4 untouched (asserted byte-equal) · description untouched · only the `TASK-1260` `status:` line edited on the board.
