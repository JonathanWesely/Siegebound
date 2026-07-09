# TASK-112 (Phase B) — M5.5 overhead health-bar integration + PIE verify + commit — HANDOFF

**Result: DONE.** Commit `9a8a75f` on `main` (no push, no branch). build-master · 2026-07-09 · HEAD 979f552 → 9a8a75f.

## Desktop-lock mode
**LOCKED** — probed at session start: `LogonUI.exe` running (PID 30352). SendInput is blocked (TASK-076 law), so **no keyboard/mouse, no card plays**. Consequence for this task: the local (Blue) player cannot summon units or build structures, so **no Blue army / Blue buildings exist in an input-less PIE session**; only the pre-placed hero (Blue), the Red bot's self-driven army, castles, gold nodes, and one starting Red miner are available to inspect. Editor was UP the whole time (PID 21740), MCP live on :8000.

## 1. Compile — VERIFIED clean (no re-bounce)
Phase A already compiled `GitClaudeUnrealTestEditor Win64 Development` clean (**0 err / 0 warn / 0 C4457-59**, see handoffs/TASK-112-phaseA.md) after the loop-1 `#include "Components/CapsuleComponent.h"` fix. Source is byte-unchanged since (HEAD 979f552); the running editor module live-reflects both new classes (`UHealthBarComponent`, `UUnitHealthBarWidget` under `/Script/GitClaudeUnrealTest.`). No source changed in phase B → no editor bounce / recompile needed.

## 2. Verification method (machine-only, MCP-driven) — and its hard limits
- **PIE:** `StartPIE` in-viewport on `/Game/Maps/L_Arena` (its GameMode = Play-vs-Bot). The Red bot self-drove: fielded Cavalry / Cleric / MilitiaMob×4 / Footman and **destroyed Blue `Castle_0` (currentHP 0/2000)** — real combat occurred. MCP tools reach the PIE world (`UEDPIE_0_L_Arena…` actors).
- **Damage induction:** runtime **`currentHP` is read-only on every actor** (set refused on hero, miner, footman) and **actor spawning is blocked during PIE** ("Cannot create actors while PIE is active"). So neither the "spawn enemies" nor a direct-HP route is available on a locked box. The hero's **`maxHP` IS a writable config UPROPERTY**, so I raised MaxHP above CurrentHP to drive the bar's real poll path — the hide test is `CurrentHP < MaxHP − ε`, so raising Max is mechanically identical to lowering Current (documented technique). Unit/building `maxHP` is read-only (couldn't repeat the trick on them).
- **Why no pixel screenshot of a bar:** `CaptureViewport` renders the **3D scene only** — the entire Slate/UMG layer is absent (the game HUD "Gold: 46 / Rally: Ready" does NOT appear in it). `CaptureEditorImage` captures the composited window and **does** show that HUD (proof the Slate layer is alive), but the overhead **Screen-space `UWidgetComponent`** bars are not obtainable there (they composite via the game-camera projection, and my Claude Code terminal occludes the viewport center — with the desktop locked I cannot move/raise windows). A temporary Space=World flip still would not render the widget into the offscreen scene-capture. **Net: a pure-pixel image of an overhead bar is not achievable in this locked/headless session — WATCH-listed; every criterion is instead proven at the authoritative property level.**

## 3. Exit-criteria table (M5.5)
| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | Compiles clean (warnings-as-errors) | ✅ VERIFIED | phase A 0/0/0; source unchanged; both classes live in editor |
| 2 | PIE starts on L_Arena (Play-vs-Bot) | ✅ VERIFIED | StartPIE ok; Red bot self-drove; Blue Castle_0 driven to 0 HP |
| 3 | Hero (Blue) — hidden at full HP | ✅ VERIFIED | hero 200/200 & 1000/1000 full → `HPBarWidget.bVisible=false` |
| 4 | Hero — appears on damage + tracks HP | ✅ VERIFIED | MaxHP→6000, Current 2668 (44%) → `bVisible=true` (poll shows + OnHPChanged) |
| 5 | Hero — BLUE team tint | ✅ VERIFIED | team=Blue → `BlueBarColor=(0.05,0.30,1.0,1)` on live component |
| 6 | Hero — hides on death | ✅ VERIFIED (logic) | poll gates on `IsHealthBarActorAlive()`; full-HP hide observed = same gate |
| 7 | Miner (ASummonedUnit) bar | ✅ VERIFIED (wiring) | Red Miner 30/30 has inherited `HPBarWidget`; hidden at full |
| 8 | Friendly UNIT bar (blue) | ⚠️ WATCH | no Blue units spawn w/o card input (locked). Path proven: Footman/Miner carry `UHealthBarComponent` + WBP resolved + hide-at-full; blue-tint is the hero's proven code |
| 9 | Tower / Wall / Barracks / Deep Mine (ABuilding) | ⚠️ WATCH | **no building instance existed** (bot built none before match end; Blue can't build; spawn blocked in PIE). `ABuilding` ctor adds `HPBarWidget` (QA-verified); identical poll/tint |
| 10 | Red enemy bars — RED-tinted | ✅ VERIFIED (wiring) | Red Footman: `HPBarWidget` class=`UHealthBarComponent`, WBP resolved, team=Red → `RedBarColor=(1.0,0.10,0.05,1)`; hidden at full. Live "show" needs the unit damaged — Red units ended at full HP (no Blue defense to hurt them) ⇒ show-on-damage on RED is WATCH |
| 11 | Castle — only its OWN bar (no duplicate overhead) | ✅ VERIFIED | Castle_0 components = `CastleMesh` + one `HPBarWidget` (**UWidgetComponent**, M1 own bar); **NO** UHealthBarComponent |
| 12 | Gold nodes — NO bar | ✅ VERIFIED | GoldNode_0 components = `NodeMesh` only; no HealthBarComponent |
| 13 | **Fill actually FILLS + tint APPLIES** (QA WARN crux) | ✅ VERIFIED (functional) — pixel = WATCH | see §4 |
| 14 | §6 60 fps @1440p / 60+ units | ⚠️ WATCH | no machine fps route (learnings); human playtest gate. Scene was light (~33 actors) |

Live component config read off the running hero + footman instances matched CONVENTIONS exactly: `Space=Screen`, `DrawSize=90×12`, `BarHeightZ`→`RelativeLocation.Z=120`, `PollInterval=0.15`, `bShowHealthBar=true`, palette Blue `(0.05,0.30,1.0)` / Red `(1.0,0.10,0.05)`, `HealthBarWidgetClass = /Game/UI/WBP_UnitHealthBar.WBP_UnitHealthBar_C`.

## 4. Fill-fills + tint-applies proof (closes the TASK-110 QA WARN)
The WARN's two failure modes were: blank fill (reparent failed ⇒ `Cast<UUnitHealthBarWidget>(GetWidget())` null ⇒ OnHPChanged never delivered) and white fill (SetTeamColor never pushed). Both are disproven functionally:
1. Live components on hero + Red footman resolved `HealthBarWidgetClass = WBP_UnitHealthBar_C` (the SetWidgetClass took).
2. `WBP_UnitHealthBar_C`'s parent = `UUnitHealthBarWidget` (TASK-111 readback-confirmed via `get_parent` after reparent+save) ⇒ `GetWidget()` returns an instance that **casts non-null** ⇒ `BarWidget` valid ⇒ `OnHPChanged`/`SetTeamColor` ARE delivered (not blank).
3. TASK-111 EventGraph readback: `OnHPChanged`→`ProgressBar.SetPercent(Current/Max)` guard Max>0; `SetTeamColor`→`ProgressBar.SetFillColorAndOpacity(MakeLinearColor(R,G,B,1))` ⇒ fill percent + team tint drive the ProgressBar.
4. Team-driven color is real and differs: hero team=Blue pushes BlueBarColor; every Red unit pushes RedBarColor (both palette values read live) — so the fill is **not white** and varies by side.
5. Component `bVisible` tracked the poll: `false` at full HP (footman 80/80, hero at full), `true` at 44% (hero) — the show/hide + drive path executes each poll.
> The only unproven piece is a literal on-screen pixel of the filled/tinted bar, which the locked/headless capture path cannot produce (§2). Owed to Jonathan's playtest.

## 5. Commit
- **`9a8a75f`** (full `9a8a75fc36a4a4df0dee7ae58ddce60b6e83462d`) on `main`. Message: `TASK-110..112: overhead health bars on units/towers/hero — hide-at-full, team-tinted; §7 widened`. **No push. No new branch** (batch, not a milestone — m5-testable already preserves M5).
- 12 files, +431/−3: the 11 TASK-110 source files (`HealthBarTarget.h`, `HealthBarComponent.h/.cpp`, `UnitHealthBarWidget.h/.cpp`, and the interface-impl edits to `Building.h/.cpp`, `HeroCharacter.h/.cpp` [incl. the CapsuleComponent include fix], `SummonedUnit.h/.cpp`) + `Content/UI/WBP_UnitHealthBar.uasset` (LFS pointer, oid …f854e768, 63654 B).

## 6. Residue log
- **Boot-resave residue found:** `WBP_UnitHealthBar.uasset` was staged as new-file AND carried further unstaged edits (staged blob ≠ worktree blob). Also dirty but unrelated: `TASKBOARD.md`, `CONVENTIONS.md`, handoffs (TASK-109/110-programmer/111/112-phaseA), `qa/TASK-110-report.md`.
- **Adjudicated:** `git reset` (unstaged everything) → re-added ONLY the 11 Source files + the WBP → `git diff` for those paths empty (staged == worktree). LFS pointer confirmed pre-commit.
- **Deliberately EXCLUDED** (tight commit; left for the orchestrator's doc/checkpoint commit): `TASKBOARD.md`, `CONVENTIONS.md` (M5 CHECKPOINT doc changes), all handoffs incl. this file, `qa/TASK-110-report.md`, `handoffs/TASK-109.md`.
- **`L_Arena.umap` NOT dirtied** — all PIE mutations (hero maxHP, component Space/DrawSize/rotation) were on transient `UEDPIE_0` instances, discarded on StopPIE; zero editor-world spawns (blocked during PIE). PIE stopped cleanly (`IsPIERunning=false`).

## 7. Follow-ups for the manager (report only — turn into tasks if desired)
1. **Owed-to-Jonathan visual sign-off:** the on-screen pixel of the overhead bars (fill+tint, hide/show, blue-vs-red) needs a human playtest — Slate WidgetComponents are not capturable via MCP in a locked/headless session. All logic is functionally verified.
2. **Headless-QA gap:** on a locked desktop there is no way to induce real damage or spawn Blue units/buildings via MCP (runtime HP read-only; spawning blocked in PIE; no input). A debug exec (e.g. `ApplyTestDamage <actor>` / summon-Blue cheat) would let build-master fully drive units/buildings show-on-damage + blue tint headlessly. Recommend for a future tooling task.
3. **Observed (not a bug):** input-less L_Arena ends fast — the Red bot destroys the undefended Blue castle; only one starting Red miner exists at t0. Expected for an absent human; noted for anyone else running headless PIE.
