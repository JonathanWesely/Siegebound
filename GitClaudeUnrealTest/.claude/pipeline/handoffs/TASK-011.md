# TASK-011 Handoff — HUD + Victory widgets (editor)

- author: gameplay-programmer
- date: 2026-07-02
- status: complete — both widgets compiled clean in-editor (warnings-as-errors) and saved. No C++ touched, no Git, L_Arena untouched (verified not saved).

## Deliverables

| Asset | Path | State |
|---|---|---|
| HUD widget | `/Game/UI/WBP_HUD` (Content/UI/WBP_HUD.uasset) | compiled + saved |
| Victory widget | `/Game/UI/WBP_VictoryScreen` (Content/UI/WBP_VictoryScreen.uasset) | compiled + saved |

Both are real `WidgetBlueprint` assets, parent class `UserWidget`, loadable by TASK-007's controller soft classes `/Game/UI/WBP_HUD.WBP_HUD_C` and `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C` (paths match the TASK-007 names block character-for-character).

## Build approach (IMPORTANT for QA — read first)

The Unreal MCP toolset has **no widget-tree authoring**: it can create an empty WidgetBlueprint and author graphs/variables/properties, but nothing can add widgets (TextBlock/Button/panels) to the designer tree, and the tree's `RootWidget` is not reachable via the property tools. No editor-Python route is exposed either (the "programmatic" toolset only orchestrates registered tools).

Workaround actually used:

1. **Donor duplication** — both widgets are duplicates of the template touch UI `/Game/Input/Touch/UI_TouchSimple` (parent UserWidget; tree: `Overlay_19` root → `SizeBox_0` → `Btn_Jump` (UMG Button, variable), plus `Thumbstick_Move` / `Thumbstick_Aim` child widgets).
2. **Asset-level property edits** on the duplicated trees (widget properties ARE editable via MCP): thumbsticks set `Collapsed` (never render/tick/hit-test), `SizeBox_0` resized (HUD 240x80, Victory 260x80), its OverlaySlot re-anchored (HUD: HAlign_Center/VAlign_Bottom, 40px bottom padding; Victory: HAlign_Center/VAlign_Center, 200px top padding), `Btn_Jump.isFocusable = false` on both (spec: non-focusable widgets).
3. **Donor graph fully stripped** on both duplicates (all 17/16 nodes deleted). This removed the donor's Construct that forced `SetInputModeGameAndUI` + `bShowMouseCursor=true` (would have fought the TASK-007 input-mode policy) and the `Btn_Jump` OnPressed/OnReleased → jump bindings (would have made the hero jump on card click). Verified by readback: zero donor nodes remain.
4. **Runtime-constructed text** — the gold counter, card label, victory title, and "Play Again" label are `UTextBlock`s created at Construct (`ConstructObjectFromClass`) and attached via `AddChildToOverlay` / `PanelWidget.AddChild`, since design-time widget creation is impossible. Layout via OverlaySlot alignment setters.
5. **Delegate bindings** are runtime `AssignDelegate` nodes created in Construct (the MCP `create_node` on an Assign type auto-creates the matching typed handler event — this is how the int32 `NewGold` param exists despite param tooling having no way to author it).

The duplicates keep two donor dependencies: `/Game/Input/Touch/UI_Thumbstick` and its engine texture (thumbstick widgets are still in the tree, Collapsed). Harmless for M1; a cleaner tree needs either a real UMG-authoring tool or a C++ widget base class (M7 candidate).

Note for build-master: `UI_TouchSimple` and `UI_LifeBar` show **in-memory dirty flags** from read-only inspection during this task. They were NOT modified and must NOT be saved — do not "save all"; only Content/UI/* changed on disk.

## WBP_HUD — bindings (verified by readback)

Event Construct (exact chain):
1. `GetOwningPlayer` → `Controller.PlayerState` → **Cast to ASiegePlayerState** (fail = logged, HUD degrades) → stored in `SiegePS` var.
2. `GetDataTableRow(/Game/Data/DT_Cards, Row "Footman")` → Break `FCardRow` → **`CardCost` var = row.Cost — no literal 3 anywhere** (RowNotFound = logged).
3. Card label TextBlock created and set to `DisplayName + " (" + Cost + ")"` → renders "Footman (3)" from table data — both label pieces come from the row.
4. Gold TextBlock created (font 28, HitTestInvisible), stored in `GoldText`.
5. **Seed (TASK-005 qa-note honored): `UpdateGoldDisplay(SiegePS.GetGold())` runs BEFORE the delegate bind**, so a widget constructed while gold is pinned (e.g. 999) is correct immediately.
6. `AssignOnGoldChanged(SiegePS)` → bound handler `OnGoldChanged_Event(NewGold:int32)` → `UpdateGoldDisplay(NewGold)`. FOnGoldChanged signature matches (DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(int32)).
7. `AssignOnClicked(Btn_Jump)` → `OnClicked_Event` → cast owning player to **ASiegePlayerController** → **`EnterPlacementMode("Footman")`** (row name, PascalCase, per contract).
8. Gold TextBlock attached to root Overlay top-left (24,12 padding).

`UpdateGoldDisplay(NewGold:int32)` (function graph, shared by seed + delegate):
- `Btn_Jump.SetIsEnabled(NewGold >= CardCost)` — enabled exactly at Gold >= 3, disabled below (§3.5).
- `Btn_Jump.SetBackgroundColor(green 0.3/0.85/0.4 when affordable, grey 0.25/0.25/0.25 when not)` — plus the engine's built-in disabled style. This is the "visually greyed" implementation.
- `GoldText.SetText("Gold: N")` (IsValid-guarded).

Input safety: root `Overlay_19` is `SelfHitTestInvisible`, `SizeBox_0` is `SelfHitTestInvisible`, gold text is `HitTestInvisible`, button `isFocusable=false` — the HUD steals no game input except actual clicks on the button itself. The HUD never touches input modes (controller owns them).

## WBP_VictoryScreen — bindings (verified by readback)

- **`SetWinner` — public BlueprintCallable function, EXACTLY ONE 1-byte parameter.** Entry-node pins verified by readback: `[then: Exec, Winner: Byte]`.
  - **Documented deviation from WARN-1's letter (orchestrator-approved):** the param type is `byte`, not `ETeamId`, because NO available MCP tool can author a project-enum-typed Blueprint parameter (param tools support primitives/structs/objects only; the graph DSL cannot create params; node pin internals are not reachable). It satisfies the contract's intent and ABI exactly: ETeamId is a uint8-backed UENUM, ProcessEvent copies 1 byte, the controller's `ParmsSize == 1` guard (SiegePlayerController.cpp:350) passes, and values map 1:1 — `0 (ETeamId::Blue)` → "Victory!", `else (1 = ETeamId::Red)` → "Defeat".
  - If QA's optional WARN-1 hardening (`FEnumProperty == StaticEnum<ETeamId>()` check) is ever implemented in C++, this widget will need a C++-assisted SetWinner (e.g. a `USiegeVictoryWidget` base with a BlueprintImplementableEvent) — carry to that task if it happens.
  - Body: sets `WinnerText` (Text var, **CDO default "Victory!"** — verified post-compile) and, if the title TextBlock already exists, pushes it directly — correct whether SetWinner is called before AddToViewport (the TASK-007 contract) or after.
- Construct: creates the title TextBlock (font 64, HitTestInvisible, centered, 220px bottom padding = above the button), sets it from `WinnerText`; creates the "Play Again" button label; binds `AssignOnClicked(Btn_Jump)`.
- Play Again OnClicked: `GetGameMode` → **Cast to ASiegeGameMode** → **`PlayAgain()`** → **`RemoveFromParent`** (spec order; `HandleMatchReset` tolerates the widget already gone). Cast fail = logged, no crash.
- Full-screen: root Overlay fills the viewport on AddToViewport (controller adds at ZOrder 10, UIOnly + focus). Placeholder styling — no background dim (M7).

## WARN-4 — card button clickability under game-only input (actual behavior)

- **Normal play (GameOnly input, hidden cursor): the card button is NOT clickable** — there is no cursor to click with. **Keyboard "1" (IA_Card1) is the guaranteed card-play trigger** and covers M1 acceptance.
- **During placement mode** the controller switches to GameAndUI + visible cursor, so the button IS clickable then — clicking it while already placing hits `EnterPlacementMode`'s re-entry guard and is refused (harmless, logged).
- The OnClicked → EnterPlacementMode wiring is real and verified; it becomes the mouse path automatically if a future task adopts a visible-cursor policy. No C++ was modified (per constraints).
- Victory screen is unaffected: match end = UIOnly + cursor, Play Again is fully clickable.

## Placeholder styling notes (premium pass is M7)

- Default engine button brush + white default-font labels; affordability shown by green/grey background tint + disabled state.
- Gold text "Gold: N" white 28pt top-left; victory title 64pt centered.
- Collapsed thumbstick widgets remain in the tree (ghosted in designer only, invisible in game).
- Internal widget names are donor names (`Btn_Jump`, `Overlay_19`, `SizeBox_0`) — cosmetic only; all spec-facing names (asset paths, SetWinner, EnterPlacementMode, PlayAgain, row "Footman") are exact.

## For QA to scrutinize

1. SetWinner byte-vs-ETeamId deviation above (orchestrator ruling in task thread; flag if you want it escalated instead).
2. Runtime-built UI instead of designer tree — acceptance criteria are all behavior-level and covered; confirm you agree the donor-duplication approach is within spec ("placeholder styling is fine; bindings must be exact").
3. `OnGoldChanged_Event` / `OnClicked_Event` are runtime AssignDelegate binds executed once in Construct — the HUD is created once per controller BeginPlay (TASK-007), so no double-bind risk; PlayAgain does not recreate the HUD.
4. Degraded paths: PS-cast fail / missing DT row log via PrintString (dev-only visibility) and skip dependent steps; no crash paths found.

## PIE checks for final assembly (build-master)

1. Boot L_Arena: HUD shows "Gold: 50" top-left, ticking +2/s live (delegate-driven, §3.2).
2. Card button reads "Footman (3)" (from DT_Cards, not a typed literal); it is grey/disabled below 3 gold and green/enabled at 3+ — spend down to <3 and watch it re-grey (§3.5).
3. Key "1" at 3+ gold enters placement mode (ghost appears; gold unchanged until confirm). Confirm on Blue half deducts exactly 3 and the button state updates immediately.
4. Card button click DURING placement mode is refused (guard) — expected.
5. Destroy the Red castle: full-screen "Victory!" (64pt) + Play Again button appears, cursor visible.
6. Play Again: match fully resets (gold 50 on HUD immediately — seed/delegate both fire via ResetGold broadcast), widget removed, input restored.
7. Defeat variant: `SetWinner(1)` → "Defeat" (unreachable in M1 gameplay; can be smoke-tested by calling SetWinner on the widget or damaging the Blue castle via console).
8. Confirm no thumbstick circles render in PIE and WASD/LMB behave normally with the HUD up (nothing steals input).

## Files touched

- `/Game/UI/WBP_HUD` + `/Game/UI/WBP_VictoryScreen` (created via duplication, edited, compiled, saved) — only disk changes, both under Content/UI/.
- This handoff file.
- NOT touched: L_Arena (verified not dirty/not saved), donor assets (in-memory dirty flag only — do not save), TASKBOARD.md, C++, Git.
