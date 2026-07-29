# TASK-355 — [M8-menu] `WBP_SessionMenu` + main-menu entry — art-director handoff

- **Author:** art-director · 2026-07-29 (RESUMED after the prior agent died mid-task on an API credit outage)
- **Status:** **BLOCKED — tooling wall.** Primary deliverable NOT achievable with the available editor automation. Escalated to 🚨 Blockers.
- **Editor left RUNNING with MCP up** (PID 37444). No PIE started. No `L_Arena` save. No foreign save. No Git operations.

---

## 1. AUDIT of the dead agent's work (mandated first step)

**Result: it left NOTHING on disk. Nothing was adopted; nothing needed deleting.**

| Check | Finding |
|---|---|
| `git status --short` for `Content/` | ZERO additions at session start — clean |
| `find Content -iname "*Session*"` | no matches |
| `/Game/UI/WBP_SessionMenu` existence (live editor probe) | `does_asset_exist = False` |
| `Saved/Autosaves/PackageRestoreData.json` | listed **exactly one** package: `/Game/UI/WBP_MainMenu` — and **no** `WBP_SessionMenu` |

So its `BlueprintTools.create` never produced an asset. Its only state was **in-memory dirty edits to `WBP_MainMenu`** (proven by the `12:19:18` autosave of `/Game/UI/WBP_MainMenu` — autosave only writes dirty packages). Those edits went to `Saved/Autosaves/`, never to `Content/`. They were **discarded** by the never-save close, per the law.

`PackageRestoreData.json` was moved aside to `PackageRestoreData.json.bak-2026-07-29-wedgekill-task355` (repo convention — prior `.bak-*-forcekill` / `-wedgekill` files) so the relaunch would not stall on a restore prompt and would not re-adopt the dead agent's `WBP_MainMenu` autosave.

## 2. ROOT CAUSE of the MCP outage (new, worth recording)

The editor was **not** merely "MCP wedged" — its **game thread was frozen for ~5 hours**. Log evidence:

```
[2026.07.29-12.26.57:698][421] LogModelContextProtocol: Dispatching toolset tool:
                               'editor_toolset.toolsets.blueprint.BlueprintTools.create'
```

The frame counter froze at `[421]` on that exact call and never advanced (12:26 → 17:42 log time). A window titled **"Message"** was open on the process. `Process.Responding` reported `True` — that is the **Win32 message pump, not UE's game thread**, and is a misleading health signal.

**Diagnosis: `BlueprintTools.create` opened a MODAL dialog and blocked forever headless.**

**Confirmed by experiment:** I called the same tool with an **explicit, concrete `asset_type`** (`/Script/UMG.UserWidget`) and it **returned instantly with no hang**. So the tool is safe *when given an unambiguous class*; an ambiguous/omitted class raises a picker modal that deadlocks an unattended editor.

> **Law worth adding to CONVENTIONS:** always pass a concrete `asset_type` class refPath to `BlueprintTools.create`. And when checking editor health, `Responding=True` is NOT proof of life — check the log's `][NNN]` frame counter for advancement (matches the TASK-239 wedge-watcher corollary).

## 3. Editor / MCP recovery

1. No PIE session active (zero PIE markers in the log) → Jonathan was not playing; the standing close/reopen grant applied.
2. Graceful `CloseMainWindow()` sent → **not processed** (game thread frozen) → waited past the 5-minute rule → force-killed PID 22620 (authorized fallback). Nothing on disk was at risk: `Content/` was clean.
3. Relaunched detached → **PID 37444**; MCP :8000 answered on poll. Editor healthy, MCP verified with live calls.
4. Python remote execution re-enabled (reverts on restart) via MCP `ObjectTools.set_properties` on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings` → `bRemoteExecution=true` (TASK-221 recipe). Runner: session scratchpad `ue_exec.py`.

## 4. THE BLOCKER — a design-time WidgetTree cannot be authored by any available lane

`USessionMenuWidget`'s auto-wire layer binds by **design-time widget NAME in the WidgetTree** (`BindWidgetOptional`). Route (A) therefore needs real design-time widgets. More fundamentally, **any** renderable UserWidget needs a **root widget**.

**Contract confirmation (this part is GOOD news — the C++ side is sound):**
- `USessionMenuWidget` resolves live at `/Script/GitClaudeUnrealTest.SessionMenuWidget`.
- A WBP reparented to it produces a CDO where **all six** `BindWidgetOptional` properties resolve: `HostButton`, `JoinButton`, `BackButton`, `AddressTextBox`, `StatusTextBlock`, `ErrorTextBlock` (verified `isinstance(cdo, unreal.SessionMenuWidget) == True`).
- Named widgets **can** be constructed: `unreal.new_object(unreal.Button, widget_tree, "HostButton")` succeeds, and `panel.add_child(btn)` returns a real `CanvasPanelSlot`.

**What is impossible — `UWidgetTree.RootWidget` cannot be written.** Every lane refuses it:

| Attempted lane | Result |
|---|---|
| Python `wt.set_editor_property("RootWidget", panel)` | `Property 'RootWidget' ... is protected and cannot be set` |
| Python `wt.get_editor_property("RootWidget")` | protected, cannot be read |
| MCP `ObjectTools.set_properties` on the WidgetTree | `the following properties could not be set: RootWidget` |
| MCP `ObjectTools.get_properties` on the WidgetTree | `could not be read: RootWidget, AllWidgets` |
| `WidgetBlueprintFactory.RootWidgetClass` (instance) | protected, cannot be read/set |
| MCP set on factory CDO `/Script/UMGEditor.Default__WidgetBlueprintFactory` | `could not be set: RootWidgetClass` |
| `unreal.WidgetTree` exposed methods | **none** beyond base `UObject` (no `ConstructWidget`) |

**And neither creation path emits a root.** Both `AssetToolsHelpers.create_asset(...WidgetBlueprintFactory)` and MCP `BlueprintTools.create` produce a WidgetBlueprint whose WidgetTree is **empty** — verified by saving each and reading the package name table (no `CanvasPanel`/panel class present).

**Why this is a real requirement, not my misreading:** every shipped WBP has design-time panels, i.e. they were authored in the actual UMG designer:

```
WBP_VictoryScreen  -> ['Overlay', 'SizeBox']
WBP_CardHand       -> ['HorizontalBox', 'Overlay', 'SizeBox', 'VerticalBox']
WBP_HUD            -> ['Overlay', 'SizeBox', 'VerticalBox']
WBP_DeckBuilder    -> ['Border', 'HorizontalBox', 'Overlay', 'SizeBox', 'VerticalBox']
WBP_MainMenu       -> ['Overlay', 'SizeBox', 'VerticalBox']
```

`WBP_MainMenu` builds its *menu buttons* at runtime, but into a **design-time root Overlay it already had** (its `EventConstruct` does `CastToOverlay(GetParent(GetParent(Btn_Jump)))`). A from-scratch WBP has no such root, so the runtime-construction technique has nothing to attach to.

The one route that would sidestep this — duplicate a rooted WBP and reparent — is **explicitly forbidden** by the task spec and the corruption law.

> This refines the memory note *"MCP UMG tooling CAN author full widget trees"*: it can author **runtime** trees **into an existing design-time root**. It cannot create the root, and cannot create design-time named widgets for `BindWidget*`.

## 5. What was produced, and its exact state

- **`/Game/UI/WBP_SessionMenu`** — EXISTS, correct CONVENTIONS name/path, **parent class `USessionMenuWidget` verified**, tree **EMPTY (no root ⇒ renders nothing)**. Built **fresh** (never duplicated). Two scratch widgets I created while probing did **not** serialize (unrooted ⇒ not referenced by the tree); the saved package name table confirms the asset is clean, not half-built.
- **Kept deliberately, not deleted.** Its parent binding is proven-correct work and it is the exact contract path; deleting leaves strictly less. It is *empty*, not *corrupt*. (MCP `delete` returned `false` anyway — a never-saved in-memory package ghost blocks reuse of the name; a delete needs an editor bounce.)
- ⚠️ **`Content/UI/WBP_SessionMenu.uasset` is STAGED in git** (`A ` in `git status --short`) — auto-staged by the editor's source-control integration, **not** by me (I ran no Git commands). **build-master: unstage it if TASK-355 is not landing** — do not let it ride along in an unrelated commit.
- `WBP_ZZRootProbe` — throwaway used to prove `create` doesn't hang with a concrete class; **deleted, gone from disk.** Zero residue.

## 6. §9.6 VictoryScreen Play-Again rewire — NOT ATTEMPTED (deliberate)

Taking the board's sanctioned fallback: **old wiring left intact; recorded here.**

Reasoning: TASK-355 cannot complete, so nothing from it will be committed. Editing a **shipped, working** widget now would leave a modified `WBP_VictoryScreen` parked uncommitted in the shared tree — the exact TASK-268/277 parked-change trap — for a task that isn't landing. Play Again remains host-only, the accepted fallback.

**Current wiring** (`WBP_VictoryScreen:EventGraph`, `OnClicked_Event`):
```
GetGameMode -> CastToSiegeGameMode -> Siegebound|Match|PlayAgain(gm) -> RemoveFromParent(self)
```
**Target:** `GetOwningPlayer -> CastToSiegePlayerController -> RequestPlayAgain -> RemoveFromParent(self)`

Vocabulary already resolved live for whoever finishes it:
- `Siegebound|Match|RequestPlayAgain` (confirmed `BlueprintCallable` on `ASiegePlayerController`, compiled into the live DLL)
- `Utilities|Casting|CastToSiegePlayerController`
- `Widget|GetOwningPlayer`
- `retarget_node_class` is the clean in-place swap for the existing cast node (preserves connections); then swap the `PlayAgain` call for `RequestPlayAgain`. Granular `create_node`/`connect_pins` only — never round-trip this graph.

## 7. §9.7 optional `SetLocalVictory` — NOT DONE (correctly out of reach)

`SiegePlayerController.cpp:1223-1238` calls it **by name** on the victory widget (`FindFunction("SetLocalVictory")`, requires exactly one `bool` param). It is therefore a **widget-side custom event to be added**, and it lives on the same `WBP_VictoryScreen` I deliberately did not modify. Skipped per the board ("skip freely"), remains a recorded P2 flag.

## 8. WHAT UNBLOCKS THIS (recommendation)

The cheapest unblock is a **~2-minute human step by Jonathan**, mirroring the FAB human-gate pattern:

1. Open `/Game/UI/WBP_SessionMenu` in the UMG designer (it already exists, correctly parented).
2. Drop in a root panel + the six widgets named **exactly**: `HostButton`, `JoinButton`, `BackButton` (Button), `AddressTextBox` (EditableTextBox, hint `127.0.0.1:7777`), `StatusTextBlock`, `ErrorTextBlock` (TextBlock). Save.
3. Hand back — the C++ base then auto-wires everything with **zero graph work** (QA `TASK-354.md` flag 5: *"TASK-355 may take route (A) with zero graph work"*), and I finish the main-menu entry + §9.6/§9.7.

Style to match the existing menu (read from `WBP_MainMenu`, do not invent): button = `Button` wrapping a `TextBlock`, **font size 28**, vertical-box slot padding **`MakeMargin(24, 12, 24, 12)`**, horizontally centered.

Alternatives if a human step is unacceptable:
- **(b)** A small C++ change in `USessionMenuWidget` to build its own tree in `NativeOnInitialized` (gameplay-programmer lane; makes the WBP a pure shell and removes the design-time dependency entirely).
- **(c)** An MCP UMG toolset with WidgetTree write access — the standing tooling ask; this is the third distinct task blocked by the missing UMG surface.

## 9. Main-menu "Play Online" entry — NOT ADDED (deliberate)

Withheld on purpose: wiring it now would ship a **dead-end button** opening a widget that renders nothing. The additive recipe is ready (exact template = the existing "Deck Builder" button in `WBP_MainMenu:EventGraph`):

```
ConstructObjectFromClass Button + TextBlock -> SetText "Play Online" -> SetFontSize 28
-> Button.AddChild(TextBlock) -> AddChildToVerticalBox(<the runtime VerticalBox>)
-> SetHorizontalAlignment -> SetPadding MakeMargin(24,12,24,12)
-> AssignOnClicked -> [RemoveFromParent(self); CreateWidget(WBP_SessionMenu_C); AddToViewport]
```
Additive granular `create_node`/`connect_pins` only. `Play (vs Bot)`, `Sandbox (No Bot)`, `Deck Builder`, `Quit` stay untouched (ruling 3). Note `AssignOnClicked` auto-renames handler events — re-connect pins deterministically after.

## 10. State ledger

- Editor **RUNNING** (PID 37444), **MCP up**, no PIE, `L_Arena` untouched and unsaved.
- Saved by me: **only** `/Game/UI/WBP_SessionMenu` (my own new asset). No other asset saved.
- `WBP_VictoryScreen` reads `is_dirty=true` from **inspection/boot-resave only** — I made **zero** edits to it and did **not** save it. Discard on next close (never-save law). Same for any other spurious boot-dirty package.
- `Content/` diff is exactly one file: the staged `WBP_SessionMenu.uasset` (see §5 warning).
- Zero Git commands run.
