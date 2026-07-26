# TASK-276 handoff — HUD command (stance) indicator + HOLD-point marker

**Status:** ready-for-integration (stance indicator BUILT + wired + compiled + saved) · **Assignee:** art-director · **Date:** 2026-07-24
**Branch:** m7.6-arena10x · **Widget:** `/Game/UI/WBP_HUD` (the match HUD — controller `HUDWidgetClass`) · **Depends on:** TASK-274 (compiled + committed `70487d5`)
**Law:** CONVENTIONS "Unit commands (Shield Wall stances)…" → HUD-indicator bullet. Binds the TASK-274 command API.

Applied directly in the live editor. `WBP_HUD` compiles clean (`warnings_as_errors=true`), `is_dirty=false` (saved to `Content/UI/WBP_HUD.uasset`). PIE was idle the whole time (`IsPIERunning=false`) — nothing disrupted. No C++, no other assets, no Git.

---

## What shipped (the stance indicator)

A `CommandIndicatorText` TextBlock, constructed at runtime and attached top-CENTER of the root Overlay, showing the active stance and updating live off `OnUnitCommandChanged`. Built with the project's runtime-construct idiom (like `GoldText`/stat texts) — NO designer-tree hand-placement needed. Neutral "—" until the first command and after Play Again.

### Corrected recipe vs. the old (build-master's diagnosis was right)
- The old recipe illegally put an `Assign…`+Custom-Event inside a FUNCTION. Custom events are ubergraph-only. **Fix applied:** the `Assign` + auto custom event live in the EVENTGRAPH; the display logic lives in functions.
- `<Select …>` pseudo replaced with the real enum switch `Utilities|FlowControl|Switch|SwitchonESiegeUnitCommand` (case pins `Attack`/`Hold`/`Defend`).
- **Additive-only, EventConstruct never round-tripped:** `write_graph_dsl` used ONLY on the two brand-new function graphs; all EventGraph changes done granularly (`create_node`/`connect_pins`), so the fragile `GetDataTableRow`/`BreakCardRow` Construct is byte-intact (verified by readback).

## AS-BUILT graph (readback-verified)

**New member variable:** `CommandIndicatorText : TextBlock` (object ref; runtime-set, same kind as `GoldText`).

**New function `SetupCommandIndicator()`** (constructs + attaches + seeds):
```
ConstructObject(TextBlock) -> SetCommandIndicatorText -> SetFontSize 22 -> SetVisibility HitTestInvisible
-> UpdateCommandDisplay        ; SEED from current state (seed-then-bind law)
-> CastToOverlay(GetParent(GetParent(Btn_Jump)))
   :then  AddChildToOverlay -> HAlign_Center / VAlign_Top / Padding(0,12)
   :CastFailed  PrintString "root Overlay not found; command indicator not attached"
```

**New function `UpdateCommandDisplay()`** (re-reads getters — does NOT trust the delegate param):
```
_tb = GetCommandIndicatorText
CastToSiegePlayerController(GetOwningPlayer)
  :then   if HasIssuedCommand(pc):
             switch SwitchonESiegeUnitCommand(GetCurrentCommand(pc))
               Attack -> SetText "ATTACK"   Hold -> SetText "HOLD"   Defend -> SetText "DEFEND"
          else: SetText "—"
  :CastFailed  SetText "—"
```

**EventGraph — appended at the TAIL of the EventTick first-frame init block** (right after the existing `SetVisibility(Btn_Jump,"Collapsed")`, mirroring where `SetupStatTexts`/`SetupRallyIndicator` are called):
```
... SetVisibility(Btn_Jump,"Collapsed")
-> SetupCommandIndicator()
-> CastToSiegePlayerController(GetOwningPlayer)
-> AssignOnUnitCommandChanged(pc)  -- Delegate --> [Custom Event OnUnitCommandChanged_Event_0(NewCommand)]
                                                     `-> UpdateCommandDisplay()
```
So: on the HUD's first-frame setup it builds the text, **seeds** it from `GetCurrentCommand()`/`HasIssuedCommand()`, then **binds** `OnUnitCommandChanged`; every broadcast (T/R/E press, and the Play-Again reset broadcast) fires the custom event -> `UpdateCommandDisplay`, which re-reads the getters. Because it re-reads (not the `NewCommand` param), the Play-Again reset (broadcasts `Attack` while `bHasIssuedCommand=false`) correctly shows "—".

### How it binds the command API (exact nodes)
`Siegebound|Commands|AssignOnUnitCommandChanged` (target = `CastToSiegePlayerController(Widget|GetOwningPlayer)`), auto-generated signatured handler `OnUnitCommandChanged_Event_0(NewCommand: ESiegeUnitCommand)` -> `UpdateCommandDisplay`, which calls `Siegebound|Commands|HasIssuedCommand` + `Siegebound|Commands|GetCurrentCommand` and `Widget|SetText(Text)`.

### Cruft swept
One empty/unbound `OnUnitCommandChanged_Event` (no `_0`) — a stray from an earlier `get_node_type_pins` probe (the TASK-049 "AssignOn dance" pattern) — was deleted (fully disconnected; recompiled green after). No other strays.

## Hold-point marker (secondary) — DEFERRED (fast-follow)
Not built. Secondary per spec ("stance text is the must-have"). Recommended: a ground-decal actor at `GetHoldLocation()` shown while `GetCurrentCommand()==Hold`, hidden otherwise — cleaner owned by gameplay/build integration than by the HUD widget (a UMG widget can't cleanly own a world actor's lifetime). Flag to manager when pulled in-scope.

## WATCH — on-screen appearance owed to Jonathan's playtest (NOT confirmed by readback)
Per this project's UMG lesson, graph/property readback has passed on visually-broken UMG here. Verified: graph structure, clean compile (warnings-as-errors), variable present, saved. **Owed to a pixel/human check:**
- The indicator actually renders, legible, top-center, without overlapping GoldText (top-left) / the stat stack (top-right) / the bottom card hand (§6 legibility bar).
- Text flips ATTACK->HOLD->DEFEND on T/R/E and returns to "—" on Play Again.
- Font 22 / default white / top-center padding(0,12) are provisional — tune on the W1 look.

## Files
- Widget: `Content/UI/WBP_HUD.uasset` (`/Game/UI/WBP_HUD`) — saved, ready for build-master to commit.
- Handoff: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-276.md`
