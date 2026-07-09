# TASK-111 handoff — WBP_UnitHealthBar UMG asset (art)

**Status:** ready-for-integration (art skips QA → build-master integration check at TASK-112)
**Scope:** editor/MCP only. Editor was UP, PIE was OFF the whole time (IsPIERunning=false verified before any mutation; no PIE started — TASK-112 phase B owns PIE). No gameplay code, no Git, no TASKBOARD edits.

## Asset delivered
- **`/Game/UI/WBP_UnitHealthBar`** — Widget Blueprint, duplicated from the approved donor `/Game/UI/WBP_CastleHealthBar` (itself a `UI_LifeBar` duplicate; template-donor rule honored — tree never authored from scratch).
- **Parent class: `UUnitHealthBarWidget`** (`/Script/GitClaudeUnrealTest.UnitHealthBarWidget`) — **readback-confirmed** via `get_parent` after reparent AND after save. This is the crux the QA WARN-carryforward flagged (blank/untinted bar with no crash/log if the reparent doesn't take): the reparent took, and both BIEs re-bound to the new parent (see below).
- Compiles clean (0 errors, `compile_blueprint` returned null on 3 separate compiles). Saved to disk (`save_assets` → true).

## Both BlueprintImplementableEvents implemented (float params only)
Final EventGraph (read back via `read_graph_dsl`):
```
(event Siegebound|UI|EventOnHPChanged (CurrentHP MaxHP)
  (if (> MaxHP 0.0)
    (Progress|SetPercent (Variables|WBP_UnitHealthBar|GetBar) (/ CurrentHP MaxHP))))

(event Siegebound|UI|EventSetTeamColor (R G B)
  (Progress|SetFillColorAndOpacity (Variables|WBP_UnitHealthBar|GetBar) (Utilities|Struct|MakeLinearColor R G B 1.0)))
```
- **`OnHPChanged(float CurrentHP, float MaxHP)`** — carried over from the castle donor unchanged: guards `MaxHP > 0`, then `Bar.SetPercent(CurrentHP / MaxHP)`. After reparent the ProgressBar ref auto-renamed `WBP_CastleHealthBar|GetBar` → `WBP_UnitHealthBar|GetBar`. `list_events` reports `bIsImplemented: true` with the **UUnitHealthBarWidget** C++ description (not the castle's) — proof the handler re-bound to the new parent's BIE.
- **`SetTeamColor(float R, float G, float B)`** — newly authored (granular `add_event` + `create_node` + `connect_pins`, never whole-graph write): `MakeLinearColor(R, G, B, A=1.0)` → `Bar.SetFillColorAndOpacity(...)`. Tints the ProgressBar FILL. `bIsImplemented: true`. The component pushes this ONCE at init from `BlueBarColor`/`RedBarColor`; fixed regardless of HP.

## Widget tree + compact-overhead styling
Tree (verified — **no baked text**; the donor's Overlay has a single child, no TextBlock):
```
Overlay_0  (ROOT — Visibility HitTestInvisible)
  └─ Border_0  (Padding L2/T1/R2/B1; transparent fill + subtle rounded outline)
       └─ Bar  (ProgressBar — the "Bar" widget the BIEs drive; Visibility HitTestInvisible)
```
- **Thin/compact:** the C++ `UHealthBarComponent` renders this at **Screen-space DrawSize 90×12** (from TASK-110), so the on-screen footprint is already thin/compact (~150 px equivalent). I reduced `Border_0` padding from the donor's 5 to **2/1** so the fill isn't squished to a sliver inside a 12 px-tall draw region (5+5 would leave ~2 px).
- **Dark/translucent track:** the donor already carries it — `WidgetStyle.backgroundImage.tintColor = (0,0,0,0.5)`, drawn as RoundedBox. Left as-is.
- **Clean team-tinted fill:** `fillImage.drawAs = RoundedBox`, so Slate renders a solid rounded rect from the tint and **ignores** the leftover `DefaultWhiteGrid_Low` material resource — the fill shows as a clean solid bar in the team color, no grid. `fillImage.tintColor` stays white so `SetFillColorAndOpacity(teamRGB)` shows the pure team color.
- **Neutralized default:** `Bar.FillColorAndOpacity` reset from the inherited castle red `(0.708,0,0.026)` to **white (1,1,1,1)** so the design-time/pre-init default is neutral and the runtime team push reads true.
- **HitTestInvisible:** set on both the root `Overlay_0` and the `Bar` (belt-and-suspenders; the WidgetComponent is also NoCollision in C++).

## For TASK-112 (build-master) PIE verification
- Reparent + both BIEs are readback-confirmed here, so the "silent blank bar" failure mode is closed at authoring time. In PIE, confirm on damage: **hidden at full HP**, appears on first damage, **fill tracks HP down** (SetPercent), and fill is **team-tinted** (BLUE friendly units/towers/hero; RED via the bot's units/towers), hides on death/destruction. Castles keep their own `WBP_CastleHealthBar` (untouched); gold nodes show none.
- If a bar ever renders **white** in play, that means `SetTeamColor` was never pushed (owner not resolvable as `ITeamAgent`) — all three base classes implement `ITeamAgent`, so this shouldn't occur; white is the safe fallback, not red.
- No further art needed; the bar's on-screen size is governed by the component's DrawSize 90×12 — if playtest wants it larger/smaller that's a C++ DrawSize tweak (TASK-110 surface), not a widget change.

## Source / paths
- Asset: `/Game/UI/WBP_UnitHealthBar` (`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_UnitHealthBar.uasset`)
- Donor (unmodified): `/Game/UI/WBP_CastleHealthBar`
- No RawAssets/Blender output (pure UMG editor asset).
