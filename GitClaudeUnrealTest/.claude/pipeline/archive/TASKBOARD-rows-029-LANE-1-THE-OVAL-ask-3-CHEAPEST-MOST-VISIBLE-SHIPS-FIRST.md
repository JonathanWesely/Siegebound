<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-1080 — [OVAL-SHIP] 🔧⛔ **THE INTEGRATION CHECK + COMMIT FOR ⭐ `TASK-1079` — ⛔ AND THE STAGING HAZARD IS ⛔ NAMED, BECAUSE ⛔ THIS EXACT ASSET HAS ⛔ ALREADY DONE IT ONCE.** (build-master) — ⭐ **NEW 2026-09-06, marker `TASK-1080-OVAL-SHIP`**
- assignee: build-master
- status: **done 2026-09-06** (`TASK-1080-OVAL-SHIP`) - SHIPPED. Oval-gone + all-six-cards re-verified by build-master in a 4th independent PIE hand (0 arc px in the band above the chips, 6 chip clusters). Play Again at match end NOT REACHED - the MCP bridge exposes no console/exec lane; owner: a human playtest.
- blocked-by: ⭐ **`TASK-1079`**
- parallel-safe: ⛔ **no vs ⭐ `TASK-1085` / ⭐ `TASK-1094`** — ⛔ **ONE commit host at a time; ⛔ concurrent hosts race the index.**
- spec: >
    Law: ⭐⭐ **`SC-§68`** (⛔ verify by ⛔ oid-vs-`sha256`, ⛔ NEVER by size) · ⭐⭐ `SC-§94` cl. B · ⭐ `SC-§89` · `CARDBAR-§11a`.
    **(1) 🚨⛔⛔ THE STAGING HAZARD, ⛔ BY NAME: at ⭐ `TASK-811`, ⛔ `WBP_CardHand` was ⛔ STALE IN THE GIT INDEX (⛔ blob `5c3ce72e`) and was ⛔ FLAGGED BY ⛔ NOBODY — ⛔ a commit as-found would have shipped the ⛔ OLD WIDGET ⛔ WHILE LOOKING ⛔ FULLY STAGED.** ⇒ ⛔ **⛔ RE-ADD IT EXPLICITLY AND ⛔ VERIFY ⛔ oid-vs-`sha256`. ⛔ A size match is ⛔ NOT evidence.**
    **(2) ⛔ RUN ⛔ ANY OF ⭐ `TASK-1079`'s ⛔ THREE OUTCOME CHECKS IT ⛔ DECLARED IT COULD NOT REACH** (⛔ oval gone mid-match · ⛔ ⛔ all six cards + chips ⛔ still build · ⛔ Play Again ⛔ still works at match end). ⛔ **⛔ IF ⛔ (b) FAILS, ⛔ THE CARD BAR IS ⛔ GONE — ⛔ STOP, ⛔ DO NOT COMMIT, ⛔ route to ⛔ 🚨 Blockers. ⛔ That is the `§11a` catastrophe and it is ⛔ recoverable only ⛔ before the commit.**
    **(3) ⛔ NO C++ CHANGED ⇒ ⛔ NO COMPILE IS REQUIRED. ⛔ If you compile anyway, ⛔ parse the log for `Result:` — ⛔ NEVER trust `$LASTEXITCODE`** (⛔ `Build.bat` returns ⛔ 0 on a ⛔ FAILED build).
    **(4) ⛔ COMMIT ⛔ EXACTLY the widget + the handoff + the evidence PNGs. ⛔ Pathspec is ⛔ DERIVED FROM ⭐ `TASK-1079`'s handoff WRITES list — ⛔ a file it did not name is a file that does ⛔ NOT get committed.** ⛔ **⛔ `Content/Maps/L_Arena.umap` ⛔ NEVER staged. ⛔ NEVER push.**
    **Slack: 🔧 Build & Git (`C0BF0QZP3CN`, thread `1783116286.945249`), prefix `🔧 BUILD-MASTER:`, ≥1 post with the ⛔ commit hash + the ⛔ oid-vs-sha256 result + the ⛔ three outcome answers, emoji + TASK-1080.**
- names: > `handoffs/TASK-1080-buildmaster.md` · ⛔ **STAGES: `Content/UI/WBP_CardHand.uasset` (⛔ oid-verified) + `handoffs/TASK-1079-artist.md` + `.claude/pipeline/playtest-evidence/2026-09-06/*.png`** · ⛔ **NEVER STAGED: `Content/Maps/L_Arena.umap`** · Law: `SC-§68` · `CARDBAR-§11a`.

---

