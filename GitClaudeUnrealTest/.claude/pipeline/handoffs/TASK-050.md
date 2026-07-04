# TASK-050 Handoff — HUD/Victory v3: Rally cooldown indicator + Victory/Defeat display (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-04 (M3 editor wave 3/3)
- status: **complete / ready-for-qa.** Additive Rally indicator built + bound on WBP_HUD; WBP_VictoryScreen
  Victory/Defeat confirmed already-correct (no edit). Structural readback clean + full-match PIE boot in L_Arena
  with ZERO runtime errors. **Editor left UP (current level /Game/Maps/L_Arena, PIE stopped) for TASK-052.**

## MCP-stability outcome (headline)
- **SURVIVED end-to-end.** ~50 MCP calls: 3x `write_graph_dsl` (2 fresh function graphs), granular
  `create_node`/`connect_pins`/`get_node_infos` on the EventGraph, `compile_blueprint`, `save_assets`, plus a
  **full StartPIE → StopPIE cycle**. Transport verified alive at the end (`list_toolsets` returned full schemas).
- Discipline held: small batches, health-ping + save between phases, stop-on-first-failure, and the protected
  gold Construct was **never round-tripped** (granular EventGraph adds + fresh-graph writes only).
- One clean, atomic failure mid-task: the first `SetupRallyIndicator` write was rejected with a DSL logic error
  ("Unreachable code after branch/return") — I did NOT hammer; I restructured (moved the seed call before the
  terminal cast) and the rewrite succeeded. See "Stale warnings" below.

## Part 1 — WBP_HUD Rally cooldown indicator (ADDITIVE; gold Construct PROTECTED)

**Approach:** the M1 gold Construct is a known-lossy round-trip (`GetDataTableRowDT_Cards` fails on write, per
TASK-033). So I did **zero** `write_graph_dsl` on `WBP_HUD:EventGraph`. All bulk logic went into two **new**
function graphs (fresh-graph writes, safe), and the EventGraph got only a minimal **granular** append onto the
existing TASK-033 Tick do-once. Construct is byte-intact (verified by readback — gold counter, card label,
`AssignOnGoldChanged`, `AssignOnClicked`, overlay attach all identical).

Added on `/Game/UI/WBP_HUD`:
- **Member var `RallyText`** (`/Script/UMG.TextBlock` object ref).
- **Function `UpdateRallyDisplay(bReady: bool, CooldownRemaining: float)`** — `IsValid(RallyText)`-guarded;
  `bReady` → sets text `"Rally: Ready"`, else → `"Rally: " + Ceil(CooldownRemaining) + "s"`
  (`Math|Float|Ceil` → `BuildString(Integer)`). Shared by the seed and the delegate handler.
- **Function `SetupRallyIndicator()`** — `ConstructObjectFromClass(TextBlock)` → font 20, `HitTestInvisible`,
  stored in `RallyText`; **seeds** via `UpdateRallyDisplay(true, 0.0)`; then attaches top-right of the root
  Overlay (`CastToOverlay(GetParent(GetParent(Btn_Jump)))` → `AddChildToOverlay`, `HAlign_Right`/`VAlign_Top`,
  padding L0/T12/R24/B0). CastFailed logs `"WBP_HUD: root Overlay not found; rally indicator not attached"`.
- **EventGraph Tick do-once EXTENDED** (append after the existing `SetVisibility(Collapsed)`, terminal exec):
  `SetupRallyIndicator()` → `CastToHeroCharacter(GetOwningPlayerPawn)` → `AssignOnRallyStateChanged(hero,
  OnRallyStateChanged_Event_0)`. The auto-created typed handler **`OnRallyStateChanged_Event_0(bReady,
  CooldownRemaining)` → `UpdateRallyDisplay(bReady, CooldownRemaining)`**. Runs once (reuses the
  `bCardHandSpawned` do-once guard — no new guard var).

**Seed-then-bind (CONVENTIONS honored):** `SetupRallyIndicator` sets `RallyText` and seeds it to "Rally: Ready"
BEFORE `AssignOnRallyStateChanged` binds the delegate — the indicator is never a stale bind-only widget.

**Seed-source note (for QA):** `AHeroCharacter` (c8a40b2) exposes **no** Blueprint getter for the *live* rally
cooldown-remaining (`LastRallyTime`/`bDead` are private; only `GetRallyCooldown` etc. exist). I cannot add a C++
getter (no compile this task). At HUD-creation the hero's rally is **always ready** (fresh match / respawn), so
the constant "Rally: Ready" is the correct seed at creation. A future C++ getter (`IsRallyReady` /
`GetRallyCooldownRemaining`) would let a *mid-match-created* HUD seed the live value — out of scope here.

**Delegate-contract note (for QA):** `OnRallyStateChanged` broadcasts only on **state changes** (press →
false/20; refusal press → false/remaining; cooldown elapse → true/0; `ResetHero` → true/0). The indicator shows
each broadcast's **snapshot** (e.g. "Rally: 20s" at press, then "Rally: Ready" at elapse) — it does NOT tick down
every second (there is no per-second broadcast). A live per-second countdown would need a UMG timer; the
delegate-driven ready/cooling text fully meets the spec's "ready vs cooling-down + remaining seconds".

## Part 2 — WBP_VictoryScreen Victory/Defeat (CONFIRMED correct — no edit)

Readback of `SetWinner(byte Winner)` shows the Defeat state is **already wired** (M1 TASK-011, verified today):
- `Winner == 0` (Blue win) → `WinnerText = "Victory!"`; **else (1 = Red win) → `WinnerText = "Defeat"`**.
- `IsValid(TitleText)` → pushes `WinnerText` to the title TextBlock. Construct creates the title (font 64) and
  sets it from `WinnerText`, so the branch is correct whether `SetWinner` is called before AddToViewport (the
  TASK-007 contract) or after.
- Play Again (`OnClicked_Event`) → `CastToSiegeGameMode(GetGameMode)` → `PlayAgain()` → `RemoveFromParent` —
  intact.

So the task's "wire the Defeat state if missing" was already satisfied — **no change made**. WBP_VictoryScreen
was NOT modified. NOTE: it shows an in-memory dirty flag from my read-only inspection (`read_graph_dsl`); the
**disk `.uasset` is unchanged and correct — do NOT save it** (targeted-save protocol; same as the TASK-011
donor-dirty caveat).

## Verification
- **Structural readback:** EventGraph (Construct byte-intact; Tick chain + handler correct), both function
  graphs, and the `RallyText` var all read back correct. `compile_blueprint` clean.
- **PIE boot (L_Arena, in-viewport):** a FULL MATCH ran with **zero runtime errors** — no "Accessed None", no
  "Blueprint Runtime Error", and crucially **no "rally indicator not attached"** (→ the overlay cast succeeded
  and the indicator attached; the hero was possessed → `CastToHeroCharacter` succeeded → the delegate bound).
  The match played to `Castle_0 (Blue) destroyed — winner: Red`, so the **Defeat path (SetWinner Red=1) was
  exercised live and cleanly**, plus the match-end freeze + bot decision trace (`LogSiegeBot`) were healthy.
  PIE then StopPIE'd cleanly (`IsPIERunning=false`).
- **Interactive checks are TASK-052/Jonathan** (MCP has no keypress injection): live "press Q → indicator shows
  cooling 20s → restores to Ready" and the Victory (destroy Red castle) screen. The wiring is delivered +
  structurally + boot-verified.

## Stale warnings — DO NOT CHASE (not current state)
Two log warnings exist only from transient/rejected editing states; neither recurs at the final compile or PIE
runtime:
- `19:01:58 LogBlueprint: No execute pin found on node ...WBP_HUD:EventGraph.K2Node_Event_0` — intermediate edit
  state before the Tick append was connected.
- `19:04:43 LogScript: Unreachable code after branch/return: [...UpdateRallyDisplay...] in (fn SetupRallyIndicator)`
  — my **rejected** first write; the accepted rewrite moved the seed call before the terminal cast.

## Cosmetic cruft (harmless, M7 sweep) — do not let it confuse QA
- Empty stub custom events on WBP_HUD: `OnRallyStateChanged_Event` (unbound, empty) plus the existing
  `OnGoldChanged_Event_0..5` / `OnClicked_Event_0..5` — created as `create_node`/DSL side effects; nothing binds
  them; empty bodies. (One new `_5` pair appeared during this task's node churn — harmless.)

## Files touched
- `/Game/UI/WBP_HUD.uasset` — modified + saved (is_dirty=false on disk). Only disk change under Content/UI/.
- `.claude/pipeline/handoffs/TASK-050.md` — this file.
- **NOT touched:** WBP_VictoryScreen (confirmed correct; in-memory dirty from inspection only — do NOT save),
  the M1 gold Construct (byte-intact), L_Arena (only loaded/PIE'd, not modified), C++, Git, TASKBOARD.md.
- Auto-staged assets left for TASK-052 to commit (no Git run per constraints).

## For QA to scrutinize
1. Seed constant vs a live getter (rationale above — no C++ getter exists and no compile this task).
2. Delegate snapshot-not-per-second-countdown behavior (matches the C++ broadcast contract).
3. Construct byte-intact claim (EventGraph readback in this session confirms it; no `write_graph_dsl` on EventGraph).
4. WBP_VictoryScreen left unsaved despite in-memory dirty flag — intentional (no logical change).
