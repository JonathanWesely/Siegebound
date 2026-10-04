<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-507 — [CC-BUILD] ⛔ **COMPILE ONLY — NO GIT, NO MCP, NO MEASUREMENT** (build-master)
- assignee: build-master
- status: **done** — 2026-08-03 23:46. `Result: Succeeded`, 0 errors / 0 warnings, 13.44 s.
  `UnrealEditor-GitClaudeUnrealTest.dll` relinked (3,804,160 → 3,810,816 B, 23:46:16).
  ⛔ **NOTHING COMMITTED — Stage-A hold intact, HEAD still `0b7dd2f`, `main` still 29 ahead, tree dirt unchanged.**
  🔒 `L_Arena` SHA256 identical before AND after (`B3DBC5D9…F8268`, 535,522 B). Handoff `handoffs/TASK-507-buildmaster.md`.
- blocked-by: **TASK-506 (PASS)** · ⛔ **QUIET MODULE: no other game-module task in flight, from any board**
- parallel-safe: no (the module's only compile)
- spec: >
    ⛔ **THE SCOPE OF THIS TASK IS ONE VERB: COMPILE. Jonathan is mid-playtest and this exists so he can resume with the fix in the binary**
    (ruling 3).
    **(1) ⚠️ THE EDITOR MUST BE CLOSED AND CLOSING IT IS JONATHAN'S CHOICE, NOT YOURS.** He is at the keyboard. ⛔ **Never force-kill, never drive
    an unprompted close — ASK, and wait.** (Standing law; the unattended-gate exception does not apply while he is present.)
    **(2) COMPILE.** ⛔ **Parse the log for `Result: Failed` — NEVER trust `$LASTEXITCODE`** (Build.bat returns 0 on a failed build under the Live
    Coding mutex). ⛔ **A ~2 s failure is Smart App Control, NOT a code error — do not loop QA on it; tell Jonathan.**
    **(3) ⛔ THE TREE ALREADY CARRIES UNCOMMITTED STAGE-A SOURCE (TASK-481) AND IT IS HELD ON PURPOSE.** ⛔ **You are compiling BOTH. That is
    intended.** ⛔ **NO `git add`, NO `git commit`, NO `git reset`, NO `git checkout`, NO push — `main` is 29 ahead.** ⚠️ **If TASK-505's file fails
    to compile, report the errors and hand back to the programmer; ⛔ DO NOT revert or stash an uncommitted file — it has no commit to fall back to,
    and Stage A must not be disturbed.**
    **(4) ⛔ DO NOT RUN `SpikeEval`, `SpikePrompt`, `DumpAssistantPrompt`, PIE, or ANY MCP call.** ⚠️ **The §16 dev-split proof needs the whole GPU
    and an exclusive session, and Jonathan wants to play** — **it stays in TASK-481 and runs when he hands the machine over.** ⚖️ **Taking that
    measurement now would race his playtest for the one 8 GB card, which is a failure this project has already hit twice.**
    **(5) 🔒 `L_Arena` IS NEVER OPENED AND NEVER SAVED.** No editor work of any kind beyond the build.
    **(6) REPORT, IN ONE SHORT PARAGRAPH:** the verdict line, error/warning counts, whether both DLLs relinked, and ⛔ **the plain statement that
    NOTHING WAS COMMITTED and the Stage-A hold is intact.** Then tell Jonathan he can relaunch.
    Handoff `handoffs/TASK-507-buildmaster.md`. Post in 🔧 Build & Git.
- names: >
    Build: `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex`.
    Law: CONVENTIONS "⛔ THE QUIET-MODULE LAW" · "UE Build.bat exit code lies" · "Smart App Control blocks UE compile" · the never-save `L_Arena` law ·
    "Editor close is Jonathan's choice" · `SC-§16` (one GPU lane per session).

