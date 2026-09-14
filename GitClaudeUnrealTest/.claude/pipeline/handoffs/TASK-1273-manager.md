# TASK-1273 — [VER-LAW-PILOT-DISCHARGE] — manager handoff — 2026-09-14

**Ruling discharged:** 🧑 Jonathan, in Claude Code, one word: *"binding"* (orchestrator relay; recorded labelled on `TASK-1230`'s status line, acceptance (5)). Gates at write time: `TASK-1269` committed `32d2b1e` (`CONVENTIONS.md` clean on disk before this row wrote).

## The `@@` count `TASK-1272` pre-flights against

`git diff -U0 -- GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md | grep -c '^@@'` — **expected 9.** Nine non-contiguous edit sites, all inside the `VER-LANE-2026-09-13` section (lines ~11688–11740 pre-edit); every pair is separated by at least one unchanged line, so `-U0` merges none of them. If the measured number differs, quote both and list the `@@` headers — do not reconcile silently.

| # | site | change |
|---|---|---|
| 1 | `VER-§1` cl. 1 + cl. 2 (adjacent lines, one hunk) | cl. 1: the pilot suffix sentence struck `~~…~~`, dated · cl. 2: the log file is NAMED by its line-1 `Log file open` time vs the instance's creation time (N4's `_2.log` hazard) |
| 2 | `VER-§1` cl. 5 | exhibit discharged (`d3ade81`) + a deliberate break is proven by oid-vs-sha256 both ways, `SC-§68`, the 46510-B coincidence quoted |
| 3 | `VER-§4` cl. 1 | promotion is the HOST's copy-out; three-way hash (source = target = LFS oid in the COMMIT) |
| 4 | `VER-§5` cl. 5 (NEW) | the UMG-button blind spot: `ui_perform` / `simulate_key_press` on `L_MainMenu` / Tab named DEAD ENDS; one attempt, `UNOBSERVABLE`, never a false verdict either way; `TASK-1274` the optional door |
| 5 | `VER-§6` header + cl. 1 (adjacent, one hunk) | header strike + "DISCHARGED 2026-09-14"; cl. 1's ADVISORY sentence struck, `verify: advisory` row note retired |
| 6 | `VER-§6` cl. 3 | the pilot-era manager-routing of `VERIFY-FAILED` struck, dated |
| 7 | `VER-§6` cl. 5 + cl. 6 (NEW, adjacent, one hunk) | **the DATED AMENDMENT 2026-09-14** — his word, the R-COUNT numbers, IN FORCE (i)–(iv), what does NOT change · cl. 6: the recipe facts |
| 8 | `VER-§7` cl. 2 | the sequence runner's step whitelist is a GRANT SURFACE (`pie_scene_edit spawn_actor` reached on N1/N4 with no `tools:`/`permissions.allow` entry) — action = census re-run, ⛔ no grant |
| 9 | `VER-§7` cl. 4 | "and the `VER-§4` copy-out" struck from the verifier's legal writes (consistent with site 3) |

## Other files this row wrote

- `.claude/agents/playtest-verifier.md` (TRACKED; named on this row's `names:` line) — line 80: `Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE` (suffix removed) · lines 90–93: the "suffix stays until Jonathan rules" sentence replaced by the binding statement. `grep -c 'advisory — VER-§6 pilot'` = **0** there.
- `TASKBOARD.md` — `TASK-1273` status → done · `TASK-1230` status appended (`law: VER-§6 cl. 5 amended 2026-09-14 (TASK-1273)`) · `TASK-1272` blocked-by → GO + a MANAGER AMENDMENT block in its spec (7-path pathspec, the must-not-sweep list, the two flips).
- This file.

## Residual literal count (read-back (b))

`grep -c 'advisory — VER-§6 pilot'` on `CONVENTIONS.md` = **2** (measured): line ~11688 = the struck `VER-§1` cl. 1 sentence; line ~11728 = the ORIGINAL `VER-§6` cl. 5 prescription that names the literal ("strikes the ` (advisory — VER-§6 pilot)` suffix") — referential law text, kept verbatim because the amendment sits beneath it; neither is an emitted suffix. The five pilot reports under `qa/` keep theirs as history (`VER-§1` cl. 1 says so).

## Fences honoured

`CLAUDE.md` untouched (5b was already written; cl. 5 (i) makes it live) · no section outside `VER-§` touched · no engine touch · nothing staged (a manager holds no Git).

## For `TASK-1272` — stage exactly these 7, nothing else

1. `GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md`
2. `GitClaudeUnrealTest/.claude/pipeline/qa/AURA-PHASE0.md`
3. `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1230-buildmaster.md`
4. `GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md`
5. `GitClaudeUnrealTest/.claude/agents/playtest-verifier.md`
6. `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1273-manager.md`
7. `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1272-buildmaster.md`

⛔ NOT this host's (name them, leave them): `Config/SiegeCloudDev.ini.example` · `handoffs/TASK-1275-programmer.md` · `qa/TASK-1276-report.md` · anything under `Saved/` · `settings.local.json` · any `.uasset` (`DT_Cards.uasset` must NOT appear — Probe 5's revert is proven, handoff §7).
