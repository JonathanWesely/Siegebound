# TASK-1225 — [AURA-CLAUDE-MD] — programmer handoff

- **assignee:** gameplay-programmer
- **status set:** `ready-for-qa`
- **gate:** TASK-1236 (qa-reviewer) · **host:** TASK-1241
- **files touched:** `CLAUDE.md` (the ONLY project file changed; this handoff + the board row are the only other writes)
- **no compile, no editor, no Git beyond `git diff`** — the unreal-mcp server was not reachable this session and was not needed.

## What changed — `git diff -- CLAUDE.md` = **4 hunks**, 8 insertions, 1 deletion

| # | Hunk (`@@`) | Region | Change |
|---|---|---|---|
| 1 | `@@ -12,6 +12,7 @@` | Team table | added row `\| playtest-verifier \| Runs Aura PIE verification, writes runtime evidence \| Editing code/art, compiling, Git \|` directly after `footage-analyst`. Table = **seven** agent rows. |
| 2 | `@@ -21,6 +22,7 @@` | How agents communicate | added one bullet `.claude/pipeline/qa/TASK-###-verify.md — runtime verification reports from playtest-verifier (VERIFIED / VERIFY-FAILED / UNOBSERVABLE; law: CONVENTIONS VER-§)` after the `qa/` bullet (board spec (d)). |
| 3 | `@@ -41,7 +43,10 @@` | Routing rule 5 | old single-step line replaced by a rule-5 header + **5a / 5b / 5c** sub-bullets (below). Rules 1-4, 6, 7 byte-identical; nothing renumbered. |
| 4 | `@@ -58,6 +63,8 @@` | Hard gates | two new bullets, character-exact, inserted after the existing "Nothing is committed to Git without a PASS QA report…" bullet. |

### Old rule-5 line (removed, quoted verbatim per acceptance (2))

```
5. `qa-passed` + `ready-for-integration` → invoke `build-master` to compile, assemble, and commit.
```

### New rule 5 (as written)

```
5. `qa-passed` → the compile/verify/commit chain (law: CONVENTIONS VER-§):
   - **5a** `qa-passed` → `build-master` compiles (`Result: Succeeded` law) and, for C++ changes, relaunches the editor on the new binaries (graceful-quit lane, never Live Coding). Status → `built`. No commit yet.
   - **5b** if the task's spec has a runtime acceptance criterion → invoke `playtest-verifier`. `verify-failed` → send back to `gameplay-programmer` with the verify report path; it counts as a QA loop (same max-3-then-escalate as rule 4). Blueprint/asset-only tasks skip 5a and go straight here.
   - **5c** `verified` (or `UNOBSERVABLE`, or no runtime criterion) → invoke `build-master` to assemble and commit as today.
```

### The two hard-gate sentences (character-exact, `SC-§38a`) — grep -cF returned 1 for each

```
- Nothing with a runtime acceptance criterion is committed without a VERIFIED report; UNOBSERVABLE is recorded on the row, not treated as a pass.
- Aura verification drives PIE; when Jonathan is present the dispatch announces it first and waits for a go.
```

## Acceptance self-check (board row)

1. Two hard-gate sentences character-exact — **yes** (`grep -cF` = 1 each).
2. 5a/5b/5c present, old single-step text gone — **yes** (`ready-for-integration` now has 0 hits in `CLAUDE.md`); old line quoted above.
3. Team table = seven rows — **yes**.
4. Diff touches only the four spec regions (a)(b)(c)(d) — **yes, 4 hunks**, one per region.
5. The stale manager-proxy clause (Slack-mirror bullet, line 34, "Corrected 2026-09-09…") — **NOT touched**; it lies outside every hunk (`git diff -U0` has 0 hits for it).

## Things QA should scrutinize / decisions I made

- **Region (d) vs the dispatch prompt.** The dispatch prompt listed "four regions" as team table / rule 5 / hard gates / "nothing else", but the BOARD row's spec (d) explicitly adds the `TASK-###-verify.md` line to "How agents communicate", and the gate row TASK-1236 checks "regions (a)–(d)". The board is the contract, so (d) is in — as hunk 2. If the ruling is that (d) was NOT wanted, it is a one-bullet revert.
- **Rule 6 left byte-identical.** The board spec says rule 6's "counts as a QA loop" wording "extends to verify failures"; the dispatch said keep rule 6/7 text intact. I satisfied both by putting the "counts as a QA loop (same max-3-then-escalate as rule 4)" clause inside 5b itself and not editing rule 6.
- **Reasoning not copied in** (spec: "the reasoning, not copied into CLAUDE.md — cite VER-§"): rule 5's header and the new comms bullet cite `CONVENTIONS VER-§`, which `TASK-1226` (manager) writes in parallel — it does not exist yet at handoff time, by design of the wave.
- **Line 3 still says "6-agent team".** Not in the four permitted regions, so left alone (`SC-§100`); flagging it for the manager/host as a follow-up one-word edit if wanted.
- `CLAUDE.md` is LF on disk (was before, still is); git's autocrlf warning on diff is pre-existing and not caused by this edit.
- Slack prefix: SLACK.md's registry (line 63) lists `⚙️ GAMEPLAY-PROGRAMMER:`; the dispatch said `⚙️ PROGRAMMER:` — I used the registered form.
