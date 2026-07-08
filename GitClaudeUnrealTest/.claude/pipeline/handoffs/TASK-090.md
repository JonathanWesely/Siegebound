# TASK-090 handoff — Balance integration: compile + residue chores + PIE economy verify + bot re-measure + commit (build-master, 2026-07-08)

Integration for the gold-economy chain (TASK-089 qa-passed). TASK-076 pattern: editor-down residue
chores → compile → relaunch → PIE verification → single commit. QA carry-forwards CF-1..8 from
qa/TASK-089-report.md were the PIE checklist additions.

## Compile

**PASS, clean, 17.4 s** — Build.bat GitClaudeUnrealTestEditor Win64 Development, 6 actions
(SiegePlayerState.cpp + SiegeGameMode.cpp compile, Module link, DLL link). Zero warnings/errors.
Editor bounced per protocol: MCP save-then-graceful-quit (old PID 34120) → build → detached relaunch
(new PID 28484) → MCP back up on port 8000.

Post-relaunch CDO readback (live module proof): StartingGold=10, GoldPerTick=1, BaseIncomeTickPeriod=2,
GoldTickInterval=1.0, OvertimeIncomeMultiplier=2, MinerGoldPerTick=1, MaxGold=999, MaxActiveMiners=6. ✓

## Residue adjudication record (ruling 9)

- **WBP_MainMenu.uasset (9a):** `git restore` executed while the editor was DOWN — restored cleanly, no
  streaming-lock error. Post-relaunch structural readback: **both buttons bound** — "Play (vs Bot)" →
  OnClicked_Event → `StartMatch`; BuildSandboxButton output → OnClicked_Event_4 → `StartSandboxMatch`. ✓
  Branch taken: **restore held on disk.** The fresh session marked the package dirty IN MEMORY only
  (compile-on-load during my graph readback; also "1/2 Unsaved" indicator in-editor) — I did **not**
  save it, and no save-all was issued this session, so the worktree file stayed clean through commit.
  The 9a fallback (commit knowingly) was NOT triggered. Note for the next bounce window: any future
  save-all with this editor session up will re-dirty it on disk — same standing situation as before.
- **BP_Unit_Footman + BP_Unit_Archer .uassets (9b):** left in place and committed knowingly in this
  chain's commit (TASK-088 residue: mesh-reimport component re-registration, verified benign, saved by
  Jonathan 2026-07-08).

## CF-7 git-side checks (commit-time proofs QA could not do)

- SiegeGameMode.cpp diff = **3 hunks, all `//` comment lines** (~L106 seed comment, ~L579 Play-Again
  clock rationale, ~L867 sandbox grant note). Comment-only confirmed via git diff. ✓
- **Neither delegate DECLARE line appears in the diff** (`git diff | grep -E '^[+-].*DECLARE'` = zero
  hits): FOnGoldChanged (h:17) + FOnGoldRateChanged (h:30) byte-identical. ✓
- Worktree residue at the bounce window matched ruling-9 expectations exactly (both BP_Unit deltas +
  WBP_MainMenu + TASK-089 sources + board + 2 new pipeline docs; QA's F5-vs-snapshot discrepancy was
  QA snapshot staleness, as the orchestrator had already reconciled). ✓

## PIE economy verification (direct-boot /Game/Maps/L_Arena, 3 PIE sessions)

Method note: gold/clock readbacks via MCP ObjectTools property sampling against ASiegePlayerState /
ASiegeGameState in PIE (0.2–0.7 s cadence); arithmetic law checked everywhere:
**gold(clock) = 10 + floor((clock − 0.5) / 2)** pre-overtime (income timer starts ~0.5 s after the
match clock; first base grant lands on tick 2 ≈ clock 2.5). CachedGoldRate and bClockRunning are
non-reflected (by design) — not readable; display-rate proof came from the HUD pixels instead.

- **(a) Seed 10 — PASS.** Every sample across all sessions fits seed=10 exactly (e.g. clock 46.18 →
  gold 32 = 10+22; clock 16.8 fresh boot → 18 = 10+8). Boot-path first grant lands at tick 2 (~2.5 s
  wall), matching the QA order-of-operations proof.
- **(b) Base drip +1 per exactly 2.000 s — PASS.** Grant transitions observed at …48.5, 50.5, 52.5,
  54.5, 56.5… (session 1) and equivalents in sessions 2/3; never a jump >1 pre-miner; **zero drift over
  a full 71 s match** (froze at 45 = predicted 45) and over the full 758 s overtime match (see (e)).
  One-SetGold-per-grant is consistent with the never->1-step observation + HUD/state equality
  (Gold: 38 on HUD == 38 in the PS readback at match end; Gold: 281 == OT sampling window).
- **(c) HUD "+1/s" pre-overtime — EXPECTED, recorded, not flagged** (ruling-4 round-up display law;
  true base 0.5/s). Verified on screenshots in both regimes.
- **(d) Miner accrual — mechanics PASS (bot-side), Blue-HUD portion → WATCH.** The bot played 2 Miners
  in session 3 (arrived: Red MinerIncomeCount=2). Red gold per-tick deltas over a clean no-spend 14 s
  window: **+2,+3,+2,+3,…** = exactly 2×(+1 miner gold every 1 s tick) with the base +1 composing on
  every 2nd tick — miner per-second income unchanged and independent of base cadence (CF-4). The
  Blue-side HUD "+2/s" rate-text check needed a machine-played Miner; the Miner never appeared in
  Blue's dealt hands and desktop input injection was unavailable (see constraint note below) — folded
  into the standing human WATCH (one hotkey + one click).
- **(e) Overtime at 7:00 — economy PASS (exact), HUD indicator FAIL (pre-existing defect, root-caused).**
  Harness: PIE Castle_0 lifted to Z=4000 right after boot (TASK-081 precedent; PIE-world actor only —
  no editor-world/level dirt). bOvertimeActive latched at 420.1 s, log "Overtime started at 420.1 s
  (threshold 420.0) — base income doubles" fired exactly once. Boundary arithmetic exact: gold 219
  after the 418.5 s grant (pre-OT), **first post-flip grant at 420.5 s added +2 → 221** (live latch
  read on the grant tick, ruling 3), then +2 per exactly 2 s (= 1 gold/s, exactly double) for the rest
  of the match; full-match ledger closes: frozen end gold 557 = 219 + 169×2 predicted 557. Rate text
  correctly stays "+1/s" (CF-3 — display base 1→1, silent OnGoldRateChanged is correct).
  **Defect found: the HUD OVERTIME indicator never appears.** Root cause (readback-diagnosed):
  /Game/UI/WBP_HUD function **ShowOvertime** — the function bound to OnOvertimeStarted via the
  SetupStatTexts CreateEvent — calls `UpdateOvertimeDisplay(false)` with a **hardcoded false literal**
  (should be true). UpdateOvertimeDisplay itself is correct (sets "OVERTIME", toggles visibility), the
  construct-time seed (IsOvertimeActive()=false → Collapsed) is correct, the binding fires — the pin
  literal makes it a no-op. NOT from this chain (zero UMG in TASK-089/090; delegate DECLAREs
  byte-identical). This was the first live 7:00 crossing that checked the indicator. **Follow-up for
  manager: 1-pin UMG fix + a shortened-threshold verify (set OvertimeStartSeconds low in PIE).**
- **(f) Play Again — freeze PASS; reset determinism [finalized below].** Match-end income freeze proven
  twice at runtime: gold/clock frozen (38/56.516 for 22+ s observed; 557/757.997 post-OT) with zero
  further OnGoldChanged movement.
- **(g) Log sweep — PASS.** Zero NEW errors/warnings from the change. Knowns fired as expected:
  victory-focus error ×3 (once per match end); CrowdManager "Unable to find RecastNavMesh" nav known at
  PIE boundaries; MoveToActor spam burst toward the lifted Castle_0 (harness artifact, predicted by the
  spec). **DeepMine CardType-2 known did NOT fire at all this session** (recorded as absent — possibly
  settled by the TASK-081 DT_Cards resave; watch, don't chase). All remaining log lines are my own
  harness probes (LogScript non-reflected-property reads, editor-in-play-mode tool errors, graph-read
  CustomEvent-name warnings — none saved to any asset).
- **(h) CF git re-checks — PASS** (see CF-7 section above).

## Balance ledger (ruling 7 / CF-8)

**Undefended-Blue-castle kill time vs the bot under the 10-gold / 1-per-2s economy:**

| Measure | Value |
|---|---|
| Run 1 (match clock, authoritative) | **56.5 s** |
| Run 2 | **71.3 s** |
| Prior mark TASK-088 (old economy + Trellis meshes) | ~33 s |
| Prior mark TASK-076 | ~48 s |

Run variance is bot draw order. The slowed shared economy roughly **doubles** undefended survival.
Bot behavior note for the next balance pass: the bot played **zero Miners in both rush matches**
(it did play Miners at TASK-088's faster economy, and played 2 in the long overtime match once gold
accumulated) — its early-game card mix shifts cheaper under the new economy; pre-existing bot logic,
not a regression.

## Operating constraints hit this session (process notes)

- **Desktop contention:** Jonathan was actively using the machine (browser foreground) during the PIE
  interaction phase — input injection was suspended per common sense + the TASK-076 lock lesson
  generalized: SendInput checks need not just an unlocked desktop but an UNCONTENDED one. One early
  "neutral click" landed in his browser before the contention was detected (harmless spot, recorded
  here for transparency). The session was restructured to be readback-only (castle lift, overtime,
  freeze) with the two input-dependent checks moved to the end / delegated to Jonathan via Slack.
- PIE session 1 tore down unexpectedly at 08:54:05 with no game-side log cause while the desktop was
  contended (likely a human Esc; unattributable). Cost: one extra rush match (which doubled as ledger
  run 2).
- The editor-owned "Terminal" Slate window (a plugin window that boots with the editor) floats over
  the viewport and appears in CaptureEditorImage — minimized via HWND for HUD captures.
- MoveToActor lifted-castle spam is a bounded burst (~25 lines over ~55 s after the lift), then the
  units settle — cheaper artifact than feared.

## Files committed (commit hash below)

- Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h / .cpp (TASK-089 economy change)
- Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp (comment-only ×3 hunks)
- Content/Blueprints/Units/BP_Unit_Footman.uasset + BP_Unit_Archer.uasset (chore: benign reimport
  re-registration, TASK-088 residue, ruling 9b)
- .claude/pipeline/TASKBOARD.md, handoffs/TASK-089.md, handoffs/TASK-090.md, qa/TASK-089-report.md

**Excluded:** Content/UI/WBP_MainMenu.uasset (restored, ruling 9a primary branch). **NOT pushed.**

## (f)/(d) endgame — WATCH fallback taken (time-boxed)

The two input-dependent live checks could not be machine-driven this session: Jonathan was in
continuous active desktop use (browser foreground, idle 0 s at every probe) through the entire
interaction window, and injecting clicks against an active human is worse than deferring. A Slack ask
(🔧 thread) offering him the one-click path went unanswered within the time box. Per the TASK-076
doctrine (felt-input folded into the WATCH when the desktop was locked), both fold into ONE standing
human WATCH — a single PIE session covers everything, including TASK-081's still-open grey-tint item:

**WATCH (human, one PIE session):** (1) at a match end, click **Play Again** — expect gold snaps to 10
and the first +1 lands ~2 s later (never a double grant / never a 3 s first grant); (2) play a **Miner**
(hotkey + ground click) — expect rate text flips to **+2/s** on arrival and gold gains +1 every 1 s on
top of the base cadence; (3) TASK-081 leftover: card art grey-tint when unaffordable.

Risk assessment for what the WATCH covers: LOW. (f) reset determinism already has QA's static
order-of-operations proof + the boot-path runtime proof (same ResetGold→timer seam, 3 sessions) + the
TASK-081 live Play Again precedent (button + re-deal + gold reset worked before this chain; this chain
touched neither the button nor the delegates). (d) display rate is a QA-verified pure formula; the
accrual arithmetic is proven live on the bot's miners (+2,+3 pattern).

## Final session record

- 3 PIE sessions, all direct-boot L_Arena: (1) rush/ledger run 1 + cadence + freeze; (2) rush/ledger
  run 2 (after session 1's unattributed teardown); (3) lifted-castle overtime run (0→758 s, OT flip,
  drop, defeat, freeze).
- PIE stopped cleanly at close; editor left UP on /Game/Maps/L_Arena on the committed DLL (PID 28484).
- Final worktree sweep pre-commit: exactly the scoped files — NO fresh boot-resave dirt appeared this
  bounce (WBP_MainMenu restore held on disk through all three PIE sessions).
- Commit: see hash in the board status + 🔧 Build & Git post. NOT pushed.
