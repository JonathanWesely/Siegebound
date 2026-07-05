# TASK-072 — "Sandbox (No Bot)" main-menu button (editor / UMG) — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-05
**Status:** ready-for-qa
**Editor:** UP (PID 16916, the freshly-compiled DLL that exports `ASiegeGameMode::StartSandboxMatch`). I did NOT
boot or close it. Left UP, **no PIE running** (`IsPIERunning=false`), open level untouched — clean state for
TASK-073. MCP transport healthy throughout (`list_toolsets` full-schema health-ping passed after the function
write, after the compile, and after the save; ~30 MCP calls, socket never dropped).

## What shipped (ADDITIVE to `/Game/UI/WBP_MainMenu`, saved — `is_dirty=false`)

A new **"Sandbox (No Bot)"** button, placed **directly below Play (vs Bot)** in the runtime menu VerticalBox, its
`OnClicked` bound to the static `ASiegeGameMode::StartSandboxMatch` (WorldContext = the widget `self`).

### Context — how the existing menu is built (TASK-049)
WBP_MainMenu has **no designer-tree buttons**; the whole menu is constructed at runtime in `EventConstruct`. A
root Overlay is located via `Btn_Jump`, a `VerticalBox` is created + added to the overlay (HAlign/VAlign Center),
then three runtime buttons (each = a `Button` + child `TextBlock`, font 28, HAlign_Fill, padding MakeMargin
24/12/24/12) are stacked in the VBox:
1. **Play (vs Bot)** — `AssignOnClicked → OnClicked_Event → Siegebound|Match|StartMatch`
2. **Deck Builder (Coming Soon)** — `SetIsEnabled(false)`, no OnClicked
3. **Quit** — `AssignOnClicked → OnClicked_Event_0 → Game|QuitGame`

### 1. New helper function graph `BuildSandboxButton(ParentBox: VerticalBox) -> (OutButton: Button)`
Authored via `write_graph_dsl` on a **fresh** function graph (safe — the round-trip/lossy concern from
TASK-050/064 applies only to the *EventGraph*; a new function graph is not disturbed). It mirrors the Play
button's construction **exactly**:
```
(fn BuildSandboxButton (ParentBox)
  (bind btn (Game|ConstructObjectfromClass "/Script/UMG.Button" self))
  (bind txt (Game|ConstructObjectfromClass "/Script/UMG.TextBlock" self))
  (Widget|SetText(Text) txt (Utilities|Text|ToText(String) "Sandbox (No Bot)"))
  (Appearance|SetFontSize txt 28.0)
  (Widget|Panel|AddChild btn txt)
  (bind slot (Panel|AddChildToVerticalBox ParentBox btn))
  (Layout|VerticalBoxSlot|SetHorizontalAlignment slot)
  (Layout|VerticalBoxSlot|SetPadding slot (Utilities|Struct|MakeMargin 24.0 12.0 24.0 12.0))
  (return btn))
```
Same node type_ids, same font (28), same padding (24/12/24/12), same default HAlign as Play/Deck/Quit → style +
size match. Returns the created Button so the EventGraph can bind its OnClicked.

### 2. EventGraph splice (granular `create_node` / `connect_pins` — **no `write_graph_dsl` on the EventGraph**)
Two nodes were spliced into the `EventConstruct :then` chain **immediately after the Play button's
`AssignOnClicked` node** (`K2Node_AssignDelegate_0`) and **before the Deck Builder construct**
(`K2Node_GenericCreateObject_3`):
- `(bind _outbutton (CallFunction|BuildSandboxButton _returnvalue))` — calls the helper, passing the existing
  VerticalBox (`GenericCreateObject_0.ReturnValue`).
- `(Button|Event|AssignOnClicked _outbutton (AddEvent|Custom|OnClicked_Event_4))` — binds the sandbox button's
  OnClicked.

Because `BuildSandboxButton` runs **after** Play's `AddChildToVerticalBox` and **before** Deck's, the sandbox
button lands at **VBox index 1**. Final menu order:
**Play (vs Bot) · Sandbox (No Bot) · Deck Builder (Coming Soon) · Quit** — i.e. "directly next to Play-vs-Bot".

### 3. OnClicked handler → `StartSandboxMatch`
`AssignOnClicked` auto-created its companion custom event (`K2Node_CustomEvent_9`, auto-named
`OnClicked_Event_4`) and auto-bound it to the delegate pin. I filled its body:
```
(event Custom|OnClicked_Event_4
  (Siegebound|Match|StartSandboxMatch))
```
`StartSandboxMatch` is `static … (const UObject* WorldContextObject)` with `meta=(WorldContext=…)`. Its node shows
**only an `execute` input pin** — the WorldContextObject pin is hidden and **auto-filled with `self`** (the
widget), exactly like the existing `StartMatch` node. So **WorldContext = self** is satisfied with no extra wiring
(a `UUserWidget` is a valid world context). `StartSandboxMatch` opens `L_Arena` with the `?Sandbox=1` option
(TASK-071), so no bot spawns and the Blue player gets the generous sandbox gold.

## Splice detail — the ONE existing exec link touched (Play stays behaviourally byte-identical)
No index-controlled insert or reorder node is exposed to UMG BP graphs (checked: `find_node_types` for
`InsertChild` / `Child` — only `AddChild*`, `RemoveChildAt`, `GetChildAt`, no `InsertChildAt`/`ShiftChild`). To
land the button at index 1, the single exec continuation `AssignDelegate_0.then → GenericCreateObject_3.execute`
(Play's binding node → Deck's construct) was rerouted to thread through the two new nodes:
`AssignDelegate_0.then → BuildSandboxButton → sandbox AssignOnClicked → GenericCreateObject_3.execute`.
- The **Play button** node, its `TextBlock`, its style, and its `AssignOnClicked → OnClicked_Event → StartMatch`
  binding are **byte-untouched** — only the exec pin it hands off to changed, and Deck still constructs one hop
  later. Play-vs-Bot constructs + binds + opens L_Arena WITH the bot exactly as today.
- **Deck Builder** and **Quit** buttons + their bindings are untouched and still construct.
- The donor component-bound touch events (`StickInput(Thumbstick_*)`, `OnPressed/OnReleased(Btn_Jump)`) are
  preserved (verified in the full EventGraph readback).

If the reviewer prefers a strictly zero-link-touched addition, the alternative is appending at Quit's free
`then` pin — but that places the button **below Quit** (index 3), which is not "directly next to Play-vs-Bot".
I chose the index-1 splice because it satisfies the explicit placement while keeping every existing button
behaviourally identical; it is a granular insertion, **not** a `write_graph_dsl` round-trip of the menu logic.

## Verification
- **`compile_blueprint` → clean** (null return, no errors/warnings).
- **Full `read_graph_dsl` of the EventGraph** confirms: Play → `StartMatch` (untouched), Sandbox splice →
  `BuildSandboxButton` + `AssignOnClicked → OnClicked_Event_4 → StartSandboxMatch`, Deck `SetIsEnabled` untouched,
  Quit → `QuitGame` untouched, all component-bound touch events preserved.
- **`read_graph_dsl` of `BuildSandboxButton`** confirms the function body above (button "Sandbox (No Bot)", font
  28, padding 24/12, added to `ParentBox`, returns the button).
- **Saved** — `save_assets(["/Game/UI/WBP_MainMenu"])` → true; `is_dirty("/Game/UI/WBP_MainMenu")` → false.
- **MCP stability:** `list_toolsets` full-schema health-pings between batches all passed; transport alive at end.

### Not done here (by design — TASK-073's lane)
- **Interactive / live PIE** ("click Sandbox → L_Arena with no bot"; "click Play → bot still spawns"): MCP cannot
  inject widget clicks, and this is TASK-073's explicitly-assigned integration PIE from `L_MainMenu`. I did **not**
  run a menu-construct PIE either, to avoid changing the editor's open level (the orchestrator asked to leave the
  editor UP + undisturbed for TASK-073); the construct uses the identical node pattern already PIE-verified for
  Play/Deck/Quit (TASK-049/064), and the compile + structural readback are clean.

## For QA to scrutinize
1. **`StartSandboxMatch` resolves + WorldContext:** the OnClicked_Event_4 body node is
   `Siegebound|Match|StartSandboxMatch` (the symbol exists post-TASK-071 compile — it compiled clean, so it
   resolved). Its hidden WorldContext pin auto-fills `self`; confirm you agree this matches the existing
   `StartMatch` node's WorldContext handling (TASK-049 established the same auto-fill).
2. **Play-vs-Bot untouched:** the only edit to Play's subgraph is the exec pin it continues to (now threads
   through the sandbox nodes, then reaches Deck). Its button/text/style/`StartMatch` binding are byte-identical.
   Confirm this is acceptable as "purely additive / do not disturb" (see "Splice detail" for the trade-off + the
   zero-link alternative).
3. **Placement:** index-1 (directly below Play). No `InsertChildAt`/reorder BP node exists, so index-1 required
   the one exec-link reroute above rather than a tail-append.
4. **Style/size match:** font 28, MakeMargin 24/12/24/12, default HAlign_Fill — identical to Play/Deck/Quit.
5. **Cosmetic cruft (harmless, M7 sweep — do not chase):** an empty, unbound custom event `OnClicked_Event_3`
   was spawned as a stray during the `AssignOnClicked` node creation (same class of stub TASK-049/050/064
   documented — `OnClicked_Event_1/_2` already existed). It has no body and nothing binds it; compiles clean.
   The sandbox handler is the *bound* `OnClicked_Event_4`, not `_3`.
6. **No C++ changed, no compile run, no Git, TASKBOARD not edited** — per task constraints. No shadow-of-inherited
   -member concern (no C++ this task).

## Files / assets touched
- **MODIFIED + saved:** `Content/UI/WBP_MainMenu.uasset` — new function graph `BuildSandboxButton`; EventGraph
  splice + `OnClicked_Event_4` body. Only disk change under `Content/`.
- **NEW:** `.claude/pipeline/handoffs/TASK-072.md` (this file).
- **NOT touched:** any C++, other UI assets, L_Arena / L_MainMenu (not opened/PIE'd), Git, TASKBOARD.md.
- The UE Git provider may auto-stage the modified `.uasset` — left as-is, **no git run**. TASK-073 commits it.

## Handoff to TASK-073 (build-master)
- Editor left UP (PID 16916), no PIE running, WBP_MainMenu saved + compiles clean.
- Integration PIE from `L_MainMenu`: menu shows **four** entries (Play, Sandbox, Deck [disabled], Quit) →
  click **"Sandbox (No Bot)"** → `L_Arena` boots with **zero `ASiegeBotController`** + generous
  `SandboxStartingGold`; regression: click **"Play (vs Bot)"** → bot **still** spawns (StartMatch path unchanged).
