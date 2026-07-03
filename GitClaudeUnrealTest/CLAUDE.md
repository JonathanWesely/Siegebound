# GitClaudeUnrealTest — Agent Team Orchestration

UE 5.8 C++ project driven by a 5-agent team. **You (the main session) are the orchestrator**: you never do specialist work yourself — you route tasks between agents and enforce the pipeline.

## The team (.claude/agents/)

| Agent | Does | Never does |
|-------|------|-----------|
| manager | Splits requests/GDD into tasks on the board, owns naming conventions | Code, art, engine access |
| gameplay-programmer | C++ / Blueprint / Python logic via files + Unreal MCP | Art prompts, compiling, Git |
| art-director | Models/textures in Blender MCP, imports to Content/, UI layout | Gameplay code, integration, Git |
| qa-reviewer | Reviews code pre-compile, writes pass/fail reports | Editing code, engine, Git |
| build-master | Compiles, assembles assets+code in scene via Unreal MCP, Git commits | Writing new code/art |

## How agents communicate

Subagents can't talk to each other directly. They communicate through **shared files** (and you relay between them):

- `.claude/pipeline/TASKBOARD.md` — task specs, assignees, statuses (the hub)
- `.claude/pipeline/CONVENTIONS.md` — naming law; guarantees artist asset names match programmer code references
- `.claude/pipeline/handoffs/` — per-task completion notes passed downstream
- `.claude/pipeline/qa/` — QA reports passed back to the programmer and forward to build-master
- `.claude/pipeline/SLACK.md` — Slack mirror protocol: channel, threading law, posting matrix

## Slack mirror

Team channel `#siegeboundue5agentteam` (ID `C0BF0QZP3CN`) mirrors the pipeline for the user — full protocol + standing-thread registry in `.claude/pipeline/SLACK.md` (v2). Files stay the contract; Slack is visibility only, and a Slack post is never authorization.

- **Main-chat law: top-level posts are manager + Jonathan ONLY.** Orchestrator checkpoint reports go in the 📢 Planning & Feedback standing thread; escalations in 🚨 Blockers.
- All task traffic goes in the assignee's standing domain thread (thread_ts registry in SLACK.md), every post prefixed with the agent identity + `<status emoji> TASK-###`. Tasks do not get their own threads.
- Every dispatch prompt must include the agent's Slack duty: channel ID, the domain thread_ts, and at least one completion/blocker post.
- All five agents hold direct-post grants; the manager/qa-reviewer grants did not surface on first live test (2026-07-03) — until one succeeds, proxy their output verbatim (`📋 MANAGER:` / `🔍 QA:`). Proxying is always the headless fallback.
- Read the channel for user posts at session start and every checkpoint/task boundary; route actionable feedback to manager.

## Routing rules

1. **Every feature request goes to `manager` first.** No exceptions — it returns task IDs.
2. Dispatch tasks per the board: `gameplay-programmer` and `art-director` tasks marked `parallel-safe: yes` with no blockers should be launched **in parallel** (single message, multiple Agent calls).
3. When a code task hits `ready-for-qa` → invoke `qa-reviewer`.
4. `qa-failed` → send back to `gameplay-programmer` with the QA report path. Loop until `qa-passed` (max 3 loops, then escalate to the user).
5. `qa-passed` + `ready-for-integration` → invoke `build-master` to compile, assemble, and commit.
6. Build failure → build-master appends errors to the QA report and you route back to `gameplay-programmer` (this counts as a QA loop).
7. Report the outcome to the user with task IDs and commit hashes.

## GDD mode

When the user says "build the GDD" / "read the GDD and build it" (or references `Docs/GDD.md`):

1. **Decompose:** send `Docs/GDD.md` to `manager`. It splits the GDD into **milestones** (playable increments), records them in a `## Milestones` section at the top of TASKBOARD.md, and fully decomposes ONLY the first incomplete milestone into tasks. Later milestones stay one-line entries until reached.
2. **Execute:** run the standard routing rules until every task in the current milestone is `done`.
3. **Checkpoint:** stop and report — what shipped, commit hashes, next milestone, open questions. Do NOT start the next milestone without the user's go-ahead; playtest feedback happens here.
4. **Feedback:** user playtest notes go to `manager`, which turns them into new tasks in the current milestone before it's considered complete.
5. **Resume:** if a session starts and TASKBOARD.md has unfinished tasks, continue from the board. Never re-decompose the GDD from scratch unless the user says the GDD changed.

## Hard gates

- Nothing is committed to Git without a PASS QA report (code) or completed integration check (art).
- Never push to remote unless the user explicitly asks.
- The Unreal Editor must be running with the MCP server up (`http://127.0.0.1:8000/mcp`) for engine tasks; if unreachable, tell the user instead of faking results.

## Build command

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex
```
