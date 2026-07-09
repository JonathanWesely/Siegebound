# TASK-119 — WBP_MainMenu: enable + wire the Deck Builder button → WBP_DeckBuilder — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-09
**Status:** ready-for-integration (editor/MCP task → build-master PIE integration check at TASK-120)
**Editor:** UP, MCP :8000 healthy throughout (~40 MCP calls, transport never dropped). `IsPIERunning=false`
verified BEFORE any mutation AND after save — **no PIE started** (TASK-120 owns PIE). Editor's open level
untouched. No C++, no compile of C++, no Git, no TASKBOARD.md edits.

## The button's REAL name (the reconciliation the spec asked for)
**There is NO designer-tree widget named `Btn_DeckBuilder`.** The CONVENTIONS/spec expected name is a
placeholder — TASK-049 authored WBP_MainMenu with **zero designer-tree buttons**; the entire menu (Play,
Sandbox, Deck Builder, Quit) is **constructed at runtime in `EventConstruct`** via
`ConstructObjectFromClass "/Script/UMG.Button"` + `AddChild`. So the "Deck Builder button" is:

- a runtime `UMG.Button` with **no Blueprint variable / no persistent widget name**;
- identified in the `EventGraph` as construct node **`K2Node_GenericCreateObject_3`** (DSL local `_returnvalue_5`);
- its label TextBlock is `K2Node_GenericCreateObject_4` (`_returnvalue_6`), font 28, previously reading
  **"Deck Builder (Coming Soon)"**.

This matches exactly how TASK-072 already referenced it ("the Deck Builder construct,
`K2Node_GenericCreateObject_3`"). Recorded here so the `Btn_DeckBuilder` name in CONVENTIONS is understood
as notional, not a real UWidget.

## What changed (3 surgical, ADDITIVE edits to `/Game/UI/WBP_MainMenu` — saved, `is_dirty=false`)
All done granularly (`set_pin_value` / `create_node` / `connect_pins` / `delete_node`). **No `write_graph_dsl`
was run on the EventGraph** (round-trip is lossy/rejected on this protected Construct — per the standing
caution). Compiled clean twice.

### 1. ENABLED the button
The disable node `(Widget|SetIsEnabled _returnvalue_5)` (`K2Node_CallFunction_28`) had its `bInIsEnabled`
input pin flipped **false → true**. Readback DSL now shows `(Widget|SetIsEnabled _returnvalue_5 true)`.
(Chose a pin flip over deleting the node — zero exec-chain disturbance, and it leaves an explicit
"enabled" statement in the graph.)

### 2. Relabeled the button
The deck label `ToText` node (`K2Node_CallFunction_21`) `InString` pin changed
**"Deck Builder (Coming Soon)" → "Deck Builder"** — the "Coming Soon" was now false/misleading on a live
button. Single pin edit on the deck button's own text; no other text touched. (Not explicitly requested by
the spec but squarely part of "enable this button"; flagged here for QA/build-master to accept or revert.)

### 3. WIRED OnClicked → open the deck builder (overlay nav, no level/game-mode change)
- Created a `Button|Event|AssignOnClicked` node (`K2Node_AssignDelegate_5`) and **appended** it at the very
  **tail** of `EventConstruct` — after the Quit button's existing `AssignOnClicked` (`K2Node_AssignDelegate_1`),
  whose `.then` was free. **This rerouted NO existing exec link** (unlike TASK-072's index-1 splice); the deck
  button already exists, so it only needed its OnClicked bound — a pure tail append.
  - `self` ← the deck button `K2Node_GenericCreateObject_3.ReturnValue`.
  - `execute` ← `K2Node_AssignDelegate_1.then` (the free tail).
- `create_node` auto-created the companion custom event `OnClicked_Event_9` (`K2Node_CustomEvent_18`) and
  auto-bound its `OutputDelegate → AssignDelegate_5.Delegate` (this time the auto-bind landed correctly, so
  **no break/rewire dance was needed** — I fill the same event the button is bound to; verified by readback).
- Filled `OnClicked_Event_9`'s body (spec order exactly):
  ```
  (event Custom|OnClicked_Event_9
    (Widget|RemoveFromParent self)                                             ; remove WBP_MainMenu
    (bind _rv (UserInterface|CreateWidget "/Game/UI/WBP_DeckBuilder.WBP_DeckBuilder_C"))
    (UserInterface|Viewport|AddToViewport _rv))                                ; ZOrder 0
  ```
  - `RemoveFromParent.self` ← the existing `Self` node (`K2Node_Self_0`).
  - `CreateWidget.Class` = `/Game/UI/WBP_DeckBuilder.WBP_DeckBuilder_C` — the class **resolved** (node's
    ReturnValue pin re-typed to `WBP Deck Builder Object Reference`, confirming the reference is live).
  - `AddToViewport.self` ← `CreateWidget.ReturnValue`.
  - This is the exact **reverse** of WBP_DeckBuilder's `Btn_Back` (TASK-118), so the round-trip nav is symmetric.

### Cleanup
`create_node` of the AssignOnClicked spawned one stray empty custom event `OnClicked_Event_8`
(`K2Node_CustomEvent_15`) — I confirmed it had zero pin connections and **deleted it** (it was mine, spawned
this task). Recompiled clean afterward.

## Play-vs-Bot / Sandbox / Quit bindings — BYTE-UNCHANGED (verified by full EventGraph readback)
- **Play (vs Bot):** `(Button|Event|AssignOnClicked _returnvalue_2 (AddEvent|Custom|OnClicked_Event))` →
  `(event Custom|OnClicked_Event (Siegebound|Match|StartMatch))` — untouched.
- **Sandbox (No Bot):** `(CallFunction|BuildSandboxButton …)` + `(AssignOnClicked _outbutton
  (AddEvent|Custom|OnClicked_Event_4))` → `(event Custom|OnClicked_Event_4 (Siegebound|Match|StartSandboxMatch))`
  — untouched. `BuildSandboxButton` function graph not opened/edited.
- **Quit:** `(AssignOnClicked _returnvalue_8 (AddEvent|Custom|OnClicked_Event_0))` →
  `(event Custom|OnClicked_Event_0 (Game|QuitGame 0))` — untouched (I only READ its free `.then`, which is
  allowed to fan a new exec continuation without altering its existing behavior).
- Donor component-bound touch events (`StickInput(Thumbstick_*)`, `OnPressed/OnReleased(Btn_Jump)`) preserved.

## Harmless cruft (do NOT let it confuse review — same class QA accepted in TASK-049/072)
Pre-existing empty, unbound custom events remain: `OnClicked_Event_1/_2/_3/_5/_6/_7` (inherited from prior
AssignOnClicked authoring dances). All bodyless, nothing binds them, never fire, compile clean. The **bound**
deck handler is `OnClicked_Event_9`. (My own stray `_8` was removed — see Cleanup.)

## Verification done (readback, no PIE)
- `compile_blueprint` → clean (null) twice (after wiring; after the `_8` delete).
- Full `read_graph_dsl` confirms: `SetIsEnabled _returnvalue_5 true`; label `"Deck Builder"`; deck
  `AssignOnClicked → OnClicked_Event_9 → RemoveFromParent(self) + CreateWidget(WBP_DeckBuilder_C) +
  AddToViewport`; Play/Sandbox/Quit bindings intact.
- `get_node_infos` confirms `CreateWidget.Class = /Game/UI/WBP_DeckBuilder.WBP_DeckBuilder_C` and ReturnValue
  type `WBP Deck Builder Object Reference`.
- `save_assets(["/Game/UI/WBP_MainMenu"])` → true; `is_dirty("/Game/UI/WBP_MainMenu")` → false.
- `exists("/Game/UI/WBP_DeckBuilder")` → true (target present).
- `IsPIERunning` → false before and after.

## Deliberate decisions / flags for TASK-120 (build-master PIE integration)
1. **`CreateWidget.OwningPlayer` left unconnected (null).** Mirrors WBP_DeckBuilder's Back button (the reverse
   nav) and standard widget-to-widget overlay nav — the engine defaults to the first local PC. No input-mode
   reset is issued here; the deck builder inherits the menu's UIOnly + visible-cursor posture (BP_MenuGameMode,
   TASK-049). If PIE shows the deck builder isn't receiving clicks/focus, the trivial fix is to wire
   `OwningPlayer ← GetOwningPlayer(self)` and/or add a `SetInputModeUIOnly` — but this matches the accepted
   TASK-118 reverse pattern, so I did not add it speculatively.
2. **Nav is overlay-only** — no `OpenLevel`, no new game mode, stays on L_MainMenu. WBP_DeckBuilder's `Btn_Back`
   reverses it (RemoveFromParent(self) + CreateWidget WBP_MainMenu + AddToViewport).
3. **Label change** to "Deck Builder" — revert if the team wants the original string kept.

### What TASK-120 should PIE-verify (M6 exit slice, from L_MainMenu)
1. The **Deck Builder** button is **enabled** (not greyed) and reads **"Deck Builder"**.
2. Click it → **WBP_MainMenu disappears and WBP_DeckBuilder appears** (28-card grid etc.).
3. Deck builder's **Back** returns to the main menu, and the menu's buttons are live again.
4. **Regression:** **Play (vs Bot)** still opens L_Arena with the bot; **Sandbox (No Bot)** still opens L_Arena
   with no bot; **Quit** still quits.

## Files / assets touched
- **MODIFIED + saved:** `Content/UI/WBP_MainMenu.uasset` (EventGraph only). Only disk change under `Content/`.
- **NEW:** `.claude/pipeline/handoffs/TASK-119.md` (this file).
- **NOT touched:** any C++, WBP_DeckBuilder / WBP_DeckCardTile, L_MainMenu / L_Arena (not opened/PIE'd),
  `BuildSandboxButton` graph, Git, TASKBOARD.md.
- UE Git provider may auto-stage the modified `.uasset` — left as-is, **no git run**. TASK-120 commits it.
