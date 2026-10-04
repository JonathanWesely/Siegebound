<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-681 — [VID2S1-2] 🔍 THE QA GATE — one report over 680's diff (qa-reviewer)
- assignee: qa-reviewer
- status: **✅ done — 2026-08-28 (orchestrator flip; report `qa/TASK-681.md` PASS 0/0/2; 678 live items pinned: the machine-driven `BackPressed()` + exact-line log grep (new present/old absent) + the menu-returns pixel + one second round-trip proving the re-created menu re-wires; 2 pre-existing nits recorded, no rider owed).** *(Was: backlog.)*
- blocked-by: TASK-680 (ready-for-qa)
- parallel-safe: yes (vs TASK-677 — different diff, different report; same reviewer runs them in either order)
- spec: >
    Report `qa/TASK-681.md` over the full 680 diff. Criteria, each at the artifact: **(a)** file set EXACTLY `SessionMenuWidget.{h,cpp}`; **(b)** add-before-remove order (`AddToViewport` precedes `RemoveFromParent` in control flow) and the resolve-failure path leaves the panel up with `ShowLocalError`; **(c)** the net-active branch byte-untouched; **(d)** `BackPressed()` signature unchanged; **(e)** the old log line GONE, the new one matches the handoff declaration character-for-character; **(f)** comment truth at `.h` contract + `.cpp` rationale (superseded-not-deleted); **(g)** no new BIE/test/WBP/include-graph surprises. Verdict PASS/FAIL per the standard gate.
    **Slack: ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`), prefix `🔍 QA:`, verdict + report path, emoji + TASK-681.**
- names: >
    gate file `qa/TASK-681.md` · law: the SESSION-BACK ruling · inputs `handoffs/TASK-680-programmer.md` + the 680 diff + the VID-002 report.

