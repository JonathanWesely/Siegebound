# TASK-1220 — [AURA-PHASE0-RECORD] build-master handoff (2026-09-13)

**Status: done (gate WAIVED per the board — a record of 🧑 Jonathan-supplied measurements; host `TASK-1242` reads it back). ⛔ NOT committed — no git add / commit from this dispatch.**

## What was written
1. `GitClaudeUnrealTest/.claude/pipeline/qa/AURA-PHASE0.md` (⛔ NEW) — the measurement record. One table, three rows, columns exactly per the spec: `TASK-###` · expected outcome (his words) · Aura's verdict · match Y/N · credit consumed · wall time · evidence he mentioned. Then the totals he supplied, the Enhanced-Input (`KBD-§`) finding, and the two empty headings.
2. This handoff.

## Source of every value
`TASKBOARD.md` → `TASK-1215-AURA-GATE-P0` → `status:` line (stage A discharge, orchestrator relay of his Aura-chat words, 2026-09-13). Nothing else was consulted for a number. The three cases are his own probes, not board rows — the `TASK-###` column reads `n/a (his probe — not a board row)` ×3, as the status line itself warns.

## The three match verdicts (his, copied)
- Case 1 (archer card cost → "12 gold + other card info"): **Y** — STATIC, no PIE.
- Case 2 (sprint castle entrance → centre spawn → "28.4 s, computed by Aura from the numbers"): **Y** — STATIC, no PIE.
- Case 3 ("run an instance … runs towards the middle and does a spin" → PIE launched, hero driven to the middle, spin, video pasted in chat): **Y** — PIE + input simulation + video PROVEN LIVE on this machine.

## Total credit
- Context gauge: "18% used context (90k of 500k tokens)" — MEASURED BY JONATHAN (Aura chat, 2026-09-13). ⚠️ Recorded as a CONTEXT-WINDOW gauge, not as credit.
- $-credit: **OWED** (Aura account/usage page, before `TASK-1215` stage B). Per-case credit, per-case wall time, and the `KBD-§` positional-layout observation for case 3: all **OWED** — marked so in the file, none estimated.

## Acceptance (self-check, measured against the written file)
1. Every number labelled `MEASURED BY JONATHAN (Aura chat, 2026-09-13)` — the four numbers in the file (`12 gold`, `28.4 s`, `18%`, `90k of 500k`) each carry the label at their occurrence; no "about", no agent-derived number. ✅
2. Three table rows, no more (`grep -c '^| [123] |'` = 3). ✅
3. The two empty headings present, verbatim: `## MCP tool census` followed by the line `filled by TASK-1222 from handoffs/AURA-MCP-CENSUS.md`; `## Tier` followed by `🧑 Jonathan's decision — recorded on TASK-1215 stage B, copied here verbatim by the orchestrator`. ✅
4. No tier recommendation anywhere in the file (`grep -ci 'recommend'` on the file = 0; the words Pro / Indie / Trial / Fab do not appear). ✅
5. No compile, no editor call, no commit from this row. ✅

## Fences honoured
No git add, no commit, no push · no editor / MCP engine call · no `Saved/` write · only the `TASK-1220` `status:` line edited on the board.

## Note for the orchestrator / manager
`TASK-1215` stage B (the tier) still waits on the three OWED columns; `AURA-PHASE0.md` → `## Tier` stays empty until his ruling is copied there verbatim by the orchestrator.
