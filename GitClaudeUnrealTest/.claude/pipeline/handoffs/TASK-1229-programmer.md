# TASK-1229 — [AURA-SHIP-PRESCREEN] — programmer handoff

**Status:** `ready-for-qa` (gate: `TASK-1239`; host: `TASK-1241`)
**Date:** 2026-09-13
**Law:** plan item 13 (`C:\Users\wesel\.claude\plans\look-into-a-new-moonlit-kernighan.md`) · `Docs/Aura AI for Unreal — Integration Plan.md` §4 Phase 3 step 7 · `SHIP-§8` / `SHIP-§8b` · `PKG-§5a` · `SC-§100`

## What changed

ONE file, ONE hunk: `.claude/commands/ship.md`, a new sub-section **`2b.1a`** inserted between the end of `2b.1` (its last bullet, the "CONTENT half" line) and the `2b.2` heading. Nothing else in the file was touched; no existing line was edited, moved or renumbered (`2b.2` / `2b.3` / `2b.4` keep their names, so the cross-references at "step 3 below" and "§2b.4" stay valid).

Measured diff (`git diff HEAD -- .claude/commands/ship.md`):

| Measurement | Value |
|---|---|
| Hunk count (`grep -c '^@@'`) | **1** |
| Hunk header | `@@ -144,6 +144,13 @@` (7 added lines, 0 removed, 0 modified) |
| Lines added | 7 (heading + blank + 4 bullets + blank) |
| Section | §2b only (between `2b.1` and `2b.2`) |
| `Tools/Packaging/ship.ps1` | `git diff --stat HEAD -- Tools/Packaging/ship.ps1` = **empty**; not touched |
| SHIP RESULT grammar (line 36) | byte-identical to HEAD (`sed -n 36p` both sides) |
| word "optional" | present (heading `OPTIONAL` + bullet 1 *"This step is optional."*) |
| phrase "does not adjudicate" | present, bullet 2, line 150 |
| `SHIP-§8` / `SHIP-§8b` | cited (rule 9, rule (7)), not restated |
| `PKG-§5a` | named as the one-sentence reason the stop stays (bullet 3) |

## The text (as inserted)

```
#### 2b.1a — ⭐ OPTIONAL: the Aura pre-screen of `ADJUDICATE C3` (`playtest-verifier`) — more evidence for the adjudicator, ⛔ never the adjudication

- This step is optional. When `playtest-verifier` is available (...), then once the run is suspended at `ADJUDICATE C3` the orchestrator MAY dispatch it to drive main menu → match → HUD-present against the SAME staged build the run captured (`<stage>\Windows\<shim>.exe` — never a different cook, never an editor session standing in for the stage) and attach to the suspension record, as ADDITIONAL evidence for Jonathan: the screenshot/video path it produced and its quoted widget observations (...).
- It does not adjudicate. The verdict record of 2b.3 is still written by the adjudicator against the run's own pending capture — `SHIP-§8b` rule 9 (...) — and his eye wins (`SHIP-§8b(7)`). `SHIP-§8` is unchanged by this step: no fourth verdict, no `PASS` path that an agent walks alone, and the pre-screen's own verdict word (if it prints one) carries no weight here.
- Why the stop stays, in one sentence: `PKG-§5a` — a build shipped with an empty deck, no HUD and no hero while a script said `PASS`, and better evidence is still evidence, not a verdict.
- Skipped ⇒ nothing changes. (...) `ship.ps1` knows nothing of this step and its verdict lines are untouched.
```

(Full wording is in the file; the diff is the authority.)

## Acceptance, per the row

1. Diff confined to §2b, hunk count quoted — **1 hunk**, `@@ -144,6 +144,13 @@`, inside §2b.
2. `ship.ps1` NOT touched — diff empty. ⚠️ Honest note on *"`git status` shows only `ship.md`"*: `git status` at the time of my work also shows OTHER lanes' files (`.claude/agents/qa-reviewer.md`, `.claude/agents/playtest-verifier.md`, `.mcp.json`, `CLAUDE.md`, `Docs/Aura*`, `Tools/aura_sync.ps1`, handoffs for 1218/1221/1225/1227, plus `TASKBOARD.md`) — that is wave 1 running in parallel per the section's dispatch order, not this lane. The claim this lane makes and can prove is: **this lane's writes are `ship.md` + this handoff + one `status:` line on the board, and `ship.ps1` is absent from the status list entirely.**
3. Words "optional" and "does not adjudicate" — present (heading + bullet 1; bullet 2).
4. SHIP RESULT grammar table (line 36) — unchanged, byte-identical.
5. `SHIP-§8` cited, not restated — the sub-section cites `SHIP-§8`, `SHIP-§8b` rule 9 and `SHIP-§8b(7)`; it reproduces none of the seven rules.

## What QA (`TASK-1239`) should scrutinize

- **Does any sentence let the pre-screen stand in for the human verdict?** I wrote three explicit fences: (a) *"It does not adjudicate"*, (b) the verdict record still binds to the run's OWN pending capture — a verifier screenshot is never bound as `capturePath`, (c) the verifier's own verdict word carries no weight. If any reading of bullet 1 ("attach to the suspension record") could be taken as *writing into `ship-adjudication.json`*, say so — the intent is "attach beside the record as evidence", and I am happy to sharpen the wording.
- **"SAME staged build"** — the spec says "against the STAGED shim". I wrote that as the requirement (`<stage>\Windows\<shim>.exe`, never a different cook, never an editor session). Whether `playtest-verifier` can actually drive a packaged exe rather than PIE is a property of `TASK-1223`/`1224`'s agent, not of this text; this text records what the evidence must be *of* to count. Flag if you think the doc should instead say it is a PIE run and therefore NOT the same bytes — that would make it weaker evidence, and I would rather the sentence be true than strong.
- **`SHIP-§8b` "rule 9"** — CONVENTIONS' `SHIP-§8b` enumerates 7 rules; "rule 9" is `ship.md`'s own pre-existing phrasing at 2b.1 step 3 (*"`SHIP-§8b` rule 9 rejects any other"*) and in the row spec itself. I matched the existing usage rather than introduce a third numbering. Not a change I made; noting it so QA does not count it as my defect.
- **TASK-1213's C3-crop caveat** — I searched `ship.md` for `crop`, `dimension`, `DPI`, `physical rect`, `TASK-1211`, `client rect`, `caveat`, `until it lands`: **no such sentence exists in `ship.md` as of HEAD `6930500`** (the only hit, line 64, is the `SHIP-§10` editor-alive rule). So there was nothing to leave alone in this file; the manager's pending strike presumably targets `CONVENTIONS.md` `SHIP-§9h` (line ~7392) and/or a sentence not yet written. Untouched either way.

## Not done, by law

- No compile, no editor, no Git beyond `git diff` / `git status` / `git show` (read-only).
- Board: ONLY the `status:` line of TASK-1229 flipped (`backlog` → `ready-for-qa`); no other row.
