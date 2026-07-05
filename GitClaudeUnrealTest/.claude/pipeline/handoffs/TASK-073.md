# TASK-073 — Sandbox mode integration: compile, PIE-verify, commit (build-master) — HANDOFF

**Agent:** build-master
**Date:** 2026-07-05
**Base:** main @ 5c1fcb7 → this commit. NOT pushed.
**Editor:** left UP (PID 16916), MCP healthy, PIE stopped, current level `/Game/Maps/L_Arena`, PlayNetMode restored to PIE_Standalone. Jonathan can play immediately.

## Gate check (preconditions)
- TASK-071 (code): `qa-passed` — `qa/TASK-071-report.md` Verdict **PASS** (0 blocker / 0 warn / 2 nit). ✔
- TASK-072 (editor/UMG): `ready-for-integration`. ✔
- Phase 1 (compile TASK-071 C++ via editor-bounce/Build.bat) was already **PASS**; editor booted on that DLL (PID 16916). This handoff is phase 2 (PIE-verify + commit).

## Compile
- Phase 1 clean compile confirmed by prior state (editor up on the new DLL; TASK-072 bound `Btn_Sandbox` OnClicked → `ASiegeGameMode::StartSandboxMatch` and compiled the widget clean, which requires the symbol to resolve). No recompile needed in phase 2.

## PIE verification — per line

| # | Check | Result | Evidence |
|---|-------|--------|----------|
| 2 | **REGRESSION — normal Play-vs-Bot path: bot STILL spawns as today** | **PASS (verified live)** | PIE on L_Arena (no option): `LogGitClaudeUnrealTest: [SiegeGameMode_0] Spawned bot opponent 'SiegeBotController_0' with a Red ASiegePlayerState 'SiegePlayerState_1' (GDD §4, TASK-045).` `LogSiegeBot` decisions firing (Rule 2/3/4). `find_actors`: **1** `ASiegeBotController` + **2** `ASiegePlayerState` (SiegePlayerState_0=Blue, _1=Red). Blue Gold started 50 (read 140 after income). Red bot PS Gold 220. → the InitGame override + SpawnBot gate did NOT break shipping behavior. |
| 1a | **SANDBOX — ZERO `ASiegeBotController`, no "Spawned bot opponent", no `LogSiegeBot`** | **NOT VERIFIED LIVE — tooling-blocked (see below)** | Static: `SpawnBot()` first statement is `if (bSandboxMatch){ log "SpawnBot skipped — Sandbox match"; return; }` (SiegeGameMode.cpp:772-778), QA-verified. |
| 1b | **SANDBOX — no Red bot `ASiegePlayerState`** | **NOT VERIFIED LIVE — tooling-blocked** | Static/QA: Red PS only comes from the bot's `bWantsPlayerState`; gated out by the early-return. QA's Red-PS null-safety sweep = complete (no unguarded reader). |
| 1c | **SANDBOX — Blue starts with SandboxStartingGold (~999 after MaxGold clamp)** | **NOT VERIFIED LIVE — tooling-blocked** | Live: `SandboxStartingGold=9999` reflected default confirmed on the running `SiegeGameMode_0`. Grant path (`GrantSandboxStartingGold` → `AddGold` → `SetGold` clamp to 999) QA-verified. |
| 1d | **SANDBOX — full roster playable vs static Castle_Red, win condition intact** | **NOT VERIFIED LIVE (sandbox delta)** | Roster mechanics are shipping-verified (M2/M3/M4). The sandbox delta only REMOVES the bot + ADDS gold; unit/card/targeting logic unchanged. Win condition reads the destroyed castle's TEAM enum, not a PS (Castle_Red is level-placed) — QA-confirmed fires without a Red PS. |
| 1e | **PlayAgain stays sandbox** | **NOT VERIFIED LIVE — tooling-blocked** | Static/QA: PlayAgain never re-runs InitGame (flag persists) and never calls SpawnBot; re-grant on step 4c. |
| 3 | **WBP_MainMenu loads with BOTH buttons (Play vs Bot + Sandbox (No Bot))** | **PASS (structural)** | TASK-072 `read_graph_dsl` readback: VBox = Play (vs Bot) · Sandbox (No Bot) · Deck Builder (disabled) · Quit; `Btn_Sandbox` OnClicked → `StartSandboxMatch`; widget compiles clean, `is_dirty=false`. Live menu-construct PIE + click not run (MCP cannot inject a widget click). |

### Why the SANDBOX no-bot path could not be verified live (genuine MCP tooling limitation, NOT a code defect)
The sandbox branch engages ONLY when `ASiegeGameMode::InitGame` parses `?Sandbox=1` from the level-open URL (`bSandboxMatch` is a **plain, non-reflected** C++ member — confirmed: `get_properties` on the live GameMode returns "bSandboxMatch could not be read" — so it cannot be forced via reflection; it is only ever set by `HasOption(Options, "Sandbox")`). To latch it I must get that option into `InitGame`, which the button does via `StartSandboxMatch → OpenLevelBySoftObjectPtr(..., "Sandbox=1")`. None of these are reachable through the available MCP tools:
- **No console-exec tool** anywhere in the toolset registry → cannot run `open L_Arena?Sandbox=1`.
- **No UFUNCTION-invoke tool** → cannot call the static `StartSandboxMatch`.
- **MCP `StartPIE` builds a bare map URL and ignores `ULevelEditorPlaySettings.AdditionalServerGameOptions`.** Proven empirically: set `PlayNetMode=PIE_ListenServer` + `AdditionalServerGameOptions="?Sandbox=1"`; `LogNet` confirmed a real listen server ("IpNetDriver listening on port 17777"), yet InitGame received no "Sandbox" option — the bot still spawned. So no play-settings route delivers the option.
- **No input-injection tool** → cannot click the menu button from L_MainMenu.

This is a harness gap, not a bug for 071/072 to fix: the code compiled, passed a thorough pre-compile QA (the gate is a trivial first-statement early-return; the gold grant clamps correctly; Red-PS null-safety is complete), and the byte-identical StartMatch means the regression — the risky "did we break shipping?" property — is the one I COULD and DID verify live, and it PASSES.

**Recommended confirmation:** Jonathan is at the keyboard — clicking **"Sandbox (No Bot)"** from L_MainMenu exercises the real path end-to-end in seconds (expect logs: `Sandbox match (?Sandbox=1…)` at InitGame, `SpawnBot skipped — Sandbox match`, then next tick `Sandbox: granted the Blue player 9999 starting gold (now 999 …)`, zero bot, Blue at 999).

## Byte-check
`git diff` on the 4 C++ files = **231 insertions, 0 deletions** (purely additive). `StartMatch`'s `OpenLevelBySoftObjectPtr(WorldContextObject, Arena)` is an unchanged context line — Play-vs-Bot is byte-identical, matching QA's read.

## Commit (SELECTIVE)
Staged EXACTLY the sandbox feature + its docs (never `git add -A`):
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h`, `SiegeGameMode.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h`, `SiegePlayerState.cpp`
- `Content/UI/WBP_MainMenu.uasset`
- `.claude/pipeline/handoffs/TASK-071.md`, `TASK-072.md`, `TASK-073.md`
- `.claude/pipeline/qa/TASK-071-report.md`

Commit message: `TASK-071..073: no-bot Sandbox mode — main-menu "Sandbox (No Bot)" button opens L_Arena?Sandbox=1, gates ASiegeGameMode::SpawnBot, 9999 starting gold (dev/test tooling)`. **NOT pushed.** Commit hash recorded in the build-master report to the orchestrator (this handoff is included in that commit).

### Deliberately NOT staged / residue
- `.claude/pipeline/TASKBOARD.md` — left modified/unstaged per task (manager's working doc).
- `.claude/pipeline/CONVENTIONS.md` — modified (the "Dev / test tooling" naming law for this feature) but NOT in the task's staging list, so left unstaged. **FLAG for orchestrator:** decide whether this should ride along in a follow-up commit (it is the naming contract 071/072 reference).
- **No Content residue to restore.** The editor was open across two PIE runs, but the ONLY on-disk Content change is the intended `WBP_MainMenu.uasset` (TASK-072). No boot-resave of L_Arena.umap, donor meshes, or other WBP. Nothing restored.
- Restored `ULevelEditorPlaySettings` (PlayNetMode → PIE_Standalone, AdditionalServerGameOptions → "") after the diagnostic listen-server run — these are editor user prefs, not committed files.

## Sandbox ready for playtest on main
**YES — committed to `main` (not pushed).** Code QA-passed + compiles; Play-vs-Bot regression verified live (unbroken); sandbox no-bot path is a trivial QA-vetted gate that could not be driven by MCP but is one menu click away for Jonathan. `main` advances by one commit; branches m2/m3/m4-testable untouched; still local-only.
