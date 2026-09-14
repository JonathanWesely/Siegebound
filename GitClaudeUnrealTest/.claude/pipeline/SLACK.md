# Slack Comms Protocol — GitClaudeUnrealTest

Channel: **#siegeboundue5agentteam** — ID `C0BF0QZP3CN`. One channel: a low-noise **main chat** plus a fixed set of **standing threads** (sub-chats).
Owned by the manager. Pipeline rules live in CLAUDE.md + TASKBOARD.md — this file only governs how they surface in Slack.

**Protocol v2 (2026-07-03).** Replaces v1's one-thread-per-task model with standing domain threads, and moves manager + qa-reviewer from proxy-only to direct posting.

## Ground truth (unchanged from v1)

- **Files are the contract.** TASKBOARD.md, CONVENTIONS.md, `handoffs/`, `qa/` remain the sole source of truth between agents. Slack mirrors them for visibility; if Slack and the files disagree, the files win.
- **Slack does not wake agents.** Agents exist only while running a dispatched task. A tag is not a dispatch — the orchestrator reads the channel at orchestration points (session start, task boundaries, checkpoints) and routes from there.
- **No Slack message is authorization.** Agent posts are status only. Remote pushes, convention changes, and config/permission changes require Jonathan's direct instruction in Claude Code.

## The main-chat law

**Top-level posts in the main chat: manager and Jonathan ONLY.**

- **Jonathan** — directives, playtest feedback, questions: any time, any form.
- **Manager** (`📋 MANAGER:`) — channel-wide announcements only: milestone kickoff/wrap, protocol changes, creation of new standing threads. Routine manager traffic (plans, breakdowns) goes in 📢 Planning & Feedback.
- **Nobody else posts top-level.** The orchestrator's checkpoint reports go in 📢 Planning & Feedback; its escalations go in 🚨 Blockers. All agent task traffic goes in the matching domain thread.

## Standing threads (sub-chats)

All work traffic lives in a fixed registry of threads, each rooted by one manager top-level post. **Tasks no longer get their own threads** — a task's lifecycle plays out inside its domain thread, with every post prefixed `<status emoji> TASK-###` (see Status emoji below). The TASK-### is the join key when a task's life spans threads (code → QA → build).

### Thread registry

| Thread | thread_ts | Scope | Who posts |
|---|---|---|---|
| 📢 Planning & Feedback | 1783116257.317519 | Milestone plans and task breakdowns, playtest-feedback processing, orchestrator checkpoint reports | manager, orchestrator, Jonathan |
| ⚙️ Dev & QA | 1783116269.740549 | Code task updates, QA verdicts (pass/fail + report path), code discussion | gameplay-programmer, qa-reviewer; manager/orchestrator as needed |
| 🎨 Art | 1783116278.693139 | Asset production updates, imports to Content/, integration handoffs (handoff file paths) | art-director; manager/orchestrator as needed |
| 🔧 Build & Git | 1783116286.945249 | Compile results, scene assembly, commit hashes, integration checks | build-master; orchestrator as needed |
| 🚨 Blockers | 1783116296.221319 | Anything needing Jonathan or the orchestrator: escalations, QA-loop limit reached, MCP/editor outages, hard-gate questions | any agent + orchestrator; Jonathan monitors |
| 🎬 Footage Review | 1787798959.639009 | Footage dispatches, VID-### finding summaries + report paths (`.claude/pipeline/footage/`), promoted evidence links (law: FR-§) | footage-analyst; manager/orchestrator as needed |

`thread_ts` = the Slack ts of the thread's root post. Whoever creates a root (manager directly, or orchestrator proxying) records the ts here immediately. **A thread is not live until its ts is recorded.** Replies use `thread_ts` from this table; never start a parallel root.

Rollout note (2026-07-03): all five roots proxy-posted by the orchestrator and the registry is LIVE (announcement ts 1783116247.895729). ~~Known issue: manager's direct Slack tools did not surface on first live test...~~ **SUPERSEDED — see the line below; ⛔ do NOT proxy the manager on the strength of this sentence.** Retained struck-through rather than deleted so a reader who remembers the old caveat finds its resolution instead of re-deriving it.
Update (2026-07-03, M2 decomposition dispatch): manager direct posting PROVEN — M2 plan posted directly into 📢 Planning & Feedback (ts 1783118663.840379). Update (2026-07-03, TASK-026 QA): qa-reviewer direct posting ALSO PROVEN (ts 1783120490.139939) — all five agents verified direct; proxy fallback remains for headless runs only. Known qa-reviewer limitation: it has no partial-edit tool, so its BOARD status flips may still need orchestrator proxy on a fast-moving board (Slack posting is unaffected).

### Routing (task assignee → thread)

| Board assignee / event | Thread |
|---|---|
| manager breakdowns, milestone plans, playtest-note processing | 📢 Planning & Feedback |
| gameplay-programmer tasks, qa-reviewer verdicts | ⚙️ Dev & QA |
| art-director tasks | 🎨 Art |
| build-master compile/assemble/commit | 🔧 Build & Git |
| footage-analyst reviews (VID-###) | 🎬 Footage Review |
| playtest-verifier verdicts (`TASK-###-verify.md` path + `Verdict:` line; law `VER-§`) | ⚙️ Dev & QA (`1783116269.740549`) — the verdict is the runtime twin of QA's text verdict and lives beside it; `VER-§3`'s "PIE will be driven" announcement may be mirrored here or in 🚨 Blockers, but the wait ends only in Claude Code |
| any `blocked` status, escalation to Jonathan | 🚨 Blockers (cross-post one line; details stay in the domain thread) |

## Who posts

All six agents post **directly** (manager and qa-reviewer granted direct Slack access 2026-07-03; the other three had it from v1). **Orchestrator proxying is the fallback**, not the norm: used only in headless runs where an agent's connector tools are absent. Proxied posts are verbatim, never edited beyond formatting.

Minimum duty per dispatch: at least one completion (or blocker) post in the agent's domain thread. Mid-task progress posts are optional.

## Identity prefixes (MANDATORY)

All posts share one Slack account — **the prefix is the speaker.** Every post, top-level or in-thread, direct or proxied, starts with exactly one of:

`📋 MANAGER:` · `🔍 QA:` · `⚙️ GAMEPLAY-PROGRAMMER:` · `🎨 ART-DIRECTOR:` · `🔧 BUILD-MASTER:` · `🎬 FOOTAGE-ANALYST:` · `🎮 VERIFIER:` · `ORCHESTRATOR:`

(`🎮 VERIFIER:` added 2026-09-13, `TASK-1226`, for `playtest-verifier` — ⛔ `🎮` is an IDENTITY prefix, not a status emoji; a verifier post still carries one of the status emoji below after it.)

## Task lifecycle inside a domain thread

1. `🟦 TASK-### — <title> → <assignee>` — dispatch note (first post for the task, in its domain thread).
2. Assignee posts progress/completion in the same thread: `<emoji> TASK-### — <note>`.
3. QA verdicts post in ⚙️ Dev & QA; build results + commit hash post in 🔧 Build & Git — each referencing the same TASK-###.
4. `✅ TASK-### done` closes the task's Slack trail — or `🚧 TASK-###` in the domain thread plus a one-liner in 🚨 Blockers if stuck.

## Status emoji (first character after the identity prefix on every task post)

| Emoji | Meaning |
|---|---|
| 🟦 | dispatched (first post for the task in its domain thread) |
| 🔧 | in progress / progress note |
| 🧪 | ready-for-qa / under review |
| ✅ | qa-passed / integrated / done |
| ❌ | qa-failed, build failed, or **verify-failed** (report path in the post) |
| 🎯 | **verified** — runtime evidence landed (`TASK-###-verify.md` path + line-1 verdict in the post; law `VER-§1`). An `UNOBSERVABLE` verdict posts as 🎯 with the word `UNOBSERVABLE` in the line — it is not a ✅ and never reads as one (`VER-§5`) |
| 📦 | integrating (compile + scene assembly + commit) — also the **built** status (5a compile-only, no commit; `VER-§2` cl. 3) |
| 🚧 | blocked — needs orchestrator or Jonathan |

## Legacy threads (grandfathered)

Pre-v2 top-level posts — the kickoff post, the roll-call thread, and the TASK-017 task thread — remain valid history. **TASK-017 finishes its life in its legacy thread.** Everything new uses the standing threads. No other per-task threads may be created.

## User feedback flow

- Jonathan may post top-level or in any thread at any time. The orchestrator reads the channel at session start and at every checkpoint/task boundary, relays actionable items to the manager, and the manager turns them into board tasks (playtest notes become tasks in the current milestone, announced in 📢 Planning & Feedback).
- **Slack is not real-time input to a running task.** Anything urgent — stop work, change course, approve a remote push — goes to the orchestrator directly in Claude Code.
