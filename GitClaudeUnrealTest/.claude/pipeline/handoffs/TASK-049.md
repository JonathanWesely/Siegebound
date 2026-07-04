# TASK-049 — Main menu: WBP_MainMenu + L_MainMenu + Play-vs-Bot flow (editor) — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-04
**Status:** ready-for-qa
**Editor:** UP (PID 30092, c8a40b2 DLL). I did NOT boot/close it. Left UP + on **L_Arena** (restored to
the pre-task level), PIE stopped (`IsPIERunning=false`). MCP healthy throughout — all failures encountered were
clean DSL/compile errors, never a transport death (path-tracing-disabled fix held; ~70 MCP calls, socket never
dropped).

## What shipped (3 new assets + 1 config edit)

### 1. `/Game/Maps/L_MainMenu` (NEW level)
- No "create empty level" verb exists in the MCP surface, and L_Arena is the only map — so L_MainMenu is a
  **duplicate of `/Game/Maps/L_Arena`** (AssetTools.duplicate), then stripped to a clean menu backdrop.
- **Stripped gameplay actors** (SceneTools.remove_from_scene): `Castle_0`, `Castle_1`, `GoldNode_0`,
  `GoldNode_1`, `BP_Building_ArrowTower_C_1`, `BP_Unit_Miner_C_2`, `BP_Unit_Footman_C_1` (the last three were
  stray playtest actors carried in the arena). **Kept** the lit backdrop: DirectionalLight, SkyAtmosphere,
  SkyLight, ExponentialHeightFog, VolumetricCloud, the floor StaticMeshActors, PlayerStart, NavMesh. (The two
  invisible `TargetPoint` castle-anchors + DecalActor were left — harmless, invisible.) This satisfies GDD §7
  "minimal menu level (camera + skybox/basic lighting is fine)".
- **WorldSettings GameMode override = `BP_MenuGameMode`** (`DefaultGameMode` on `WorldSettings_1` set +
  readback-confirmed = `/Game/Blueprints/BP_MenuGameMode.BP_MenuGameMode_C`). This is REQUIRED — without it the
  level would fall back to the global-default `SiegeGameMode` (which spawns the bot/deals decks). Saved.

### 2. `/Game/Blueprints/BP_MenuGameMode` (NEW — parent `GameModeBase`)
- Provides the menu game mode with a mouse cursor. `EventBeginPlay` (authored via write_graph_dsl, compiled,
  saved):
  1. `GetPlayerController(0)`
  2. `SetShowMouseCursor(true)` on that PC → cursor visible so the menu is clickable
  3. `CreateWidget(class = /Game/UI/WBP_MainMenu.WBP_MainMenu_C, OwningPlayer = PC)`
  4. `AddToViewport(ZOrder 0)`
  5. `SetInputModeUIOnly(PC, focus = the menu widget)`
- DefaultPawnClass left at the GameModeBase default (`ADefaultPawn`) so the PlayerController has a valid view of
  the lit backdrop; PlayerController class left default. No bot, no castles, no decks — plain menu mode.

### 3. `/Game/UI/WBP_MainMenu` (NEW UMG widget)
- **Duplicate of the UMG donor `/Game/Input/Touch/UI_TouchSimple`** per the CONVENTIONS UMG-donor rule (pure BP
  nodes, no C++ base). Donor tree: `Overlay_19` (root) → `SizeBox_0` → `Btn_Jump`, plus `Thumbstick_Move` /
  `Thumbstick_Aim`. The donor's touch widgets are neutralized (see below).
- `EventConstruct` builds the menu at runtime (MCP cannot author the designer tree — the M1 HUD/Victory runtime
  `ConstructObjectFromClass` + `AddChild` pattern):
  - Collapses `Btn_Jump`, `Thumbstick_Move`, `Thumbstick_Aim` (SetVisibility Collapsed) so no donor touch UI
    shows / hit-tests. `Btn_Jump` is retained only to locate the root Overlay
    (`CastToOverlay(GetParent(GetParent(Btn_Jump)))`, guarded — logs "WBP_MainMenu: root Overlay not found" on
    CastFailed).
  - Builds a `VerticalBox`, added to the root Overlay, **HAlign_Center / VAlign_Center**.
  - Three runtime `Button`s, each with a `TextBlock` label (font 28), `HAlign_Fill`, 24/12 margin, stacked in
    the VBox:
    1. **"Play (vs Bot)"** — OnClicked → **`ASiegeGameMode::StartMatch`** (`Siegebound|Match|StartMatch`, the
       static WorldContext entry from c8a40b2; WorldContext auto-fills from the widget `self`). StartMatch reads
       `ArenaLevel` from the CDO (initialized to `/Game/Maps/L_Arena.L_Arena` in the SiegeGameMode ctor, line 51)
       and `OpenLevelBySoftObjectPtr` → opens L_Arena into a fresh match vs the bot.
    2. **"Deck Builder (Coming Soon)"** — **`SetIsEnabled(false)`** → visibly greyed/disabled (M6 stub). No
       OnClicked. (`bInIsEnabled = false` pin-value readback-confirmed on the node.)
    3. **"Quit"** — OnClicked → **`Game|QuitGame`**.
- **Button-click wiring (verified by graph readback):** each button's OnClicked is bound via
  `Button|Event|AssignOnClicked` to an auto-generated handler custom event — `OnClicked_Event` (Play) and
  `OnClicked_Event_0` (Quit). Handler BODIES were added granularly (`create_node` + `connect_pins`, which
  preserves the delegate binding): `OnClicked_Event.then → StartMatch`, `OnClicked_Event_0.then → QuitGame`.
  Compiled clean, saved.

### 4. `Config/DefaultEngine.ini` — `GameDefaultMap`
- `[/Script/EngineSettings.GameMapsSettings] GameDefaultMap = /Game/Maps/L_MainMenu.L_MainMenu` (direct disk
  edit). **`EditorStartupMap` LEFT as `/Game/Maps/L_Arena.L_Arena`** so devs still open L_Arena directly.
- Also set the in-memory `UGameMapsSettings` CDO (`/Script/EngineSettings.Default__GameMapsSettings`)
  `GameDefaultMap = L_MainMenu` (readback-confirmed; EditorStartupMap left L_Arena) so the running editor agrees
  with disk — guards against an editor config-flush clobbering the disk edit. GameDefaultMap only affects a
  packaged/standalone boot (PIE always uses the open level), so it does not affect PIE verification.

## Verification (PIE from L_MainMenu + readbacks)
- **PIE from L_MainMenu** (4 s warmup, then StopPIE): `LogLoad: Game class is 'BP_MenuGameMode_C'` → the
  WorldSettings GameMode override is in effect. **No** "root Overlay not found", **no** "Accessed None", **no**
  LogPlayLevel/PIE errors → WBP_MainMenu's Construct ran and built the VBox + 3 buttons cleanly.
- **Graph readbacks** confirm: Play → `Siegebound|Match|StartMatch`; Quit → `Game|QuitGame`; Deck Builder →
  `SetIsEnabled` with `bInIsEnabled = false`; both button clicks bound via AssignOnClicked to the filled
  handlers.
- **Config readbacks**: disk ini + in-memory CDO both = `GameDefaultMap=L_MainMenu`, `EditorStartupMap=L_Arena`.
- All 3 assets `is_dirty = false` (saved). Editor restored to L_Arena.

### What MCP could NOT do / could not verify (for a manual pass)
- **Interactive clicks:** the MCP surface has no click-injection verb, so the actual button presses (Play →
  L_Arena live match; Quit → app exit) are for **TASK-052 / Jonathan** at the keyboard. This task delivers +
  verifies the WIRING (OnClicked graphs, level GameMode, config) and that the menu constructs at runtime without
  errors.
- **Cursor visibility** is wired (`SetShowMouseCursor(true)` + `SetInputModeUIOnly`) but not visually confirmed
  (no MCP cursor read). Confirm at TASK-052 PIE.
- **Designer-tree authoring is impossible via MCP** (documented since TASK-011): the 3 buttons are built at
  runtime in Construct, not in the designer tree. A future designer/M7 pass can rebuild them as real tree
  widgets for styling. Also: `(event OnClicked(WidgetName) ...)` designer-bound events can be READ but NOT
  authored via write_graph_dsl — that's why Play/Quit use runtime `AssignOnClicked` + granular body fills, not a
  designer-bound Btn_Jump.

## Cruft to sweep in a designer/M7 pass (harmless — do NOT let it confuse QA)
- **WBP_MainMenu** empty/unbound custom events left by the `AssignOnClicked` authoring dance: `PlayBtnClicked`,
  `QuitBtnClicked` (the AddEvent names I passed — ignored for the actual binding), and `OnClicked_Event_1` (a
  stray). All empty, unbound, unreachable. `AssignOnClicked` always auto-names its handler `OnClicked_Event[_N]`
  and ignores the name you pass; passing a name that collides with that auto-name is a hard compile error, hence
  the distinct-name + granular-fill approach.
- **WBP_MainMenu** donor designer-bound events preserved by write_graph_dsl: `StickInput(Thumbstick_*)`,
  `OnPressed(Btn_Jump)`, `OnReleased(Btn_Jump)` (touch jump). Their widgets are Collapsed, so they never
  fire — harmless. write_graph_dsl cannot delete component-bound events.
- **BP_MenuGameMode** has the default disabled `EventTick` stub (empty). Harmless.

## For QA to scrutinize
1. **Play wiring:** `OnClicked_Event` (bound to the Play button's AssignDelegate) → `Siegebound|Match|StartMatch`
   (static WorldContext, auto-fills `self`). StartMatch's `ArenaLevel` CDO default = `/Game/Maps/L_Arena.L_Arena`
   (SiegeGameMode.cpp:51) → opens L_Arena. No literal level path in the widget.
2. **Quit wiring:** `OnClicked_Event_0` → `Game|QuitGame` (defaults: first local PC, QuitPreference Quit).
3. **Deck Builder disabled:** `SetIsEnabled(bInIsEnabled=false)` on the Deck button — visibly greyed + not
   clickable (M6 stub). No OnClicked bound.
4. **L_MainMenu GameMode override** = BP_MenuGameMode_C (NOT SiegeGameMode) — confirmed by PIE
   `Game class is 'BP_MenuGameMode_C'`. So the menu does not spawn the bot/deal decks.
5. **Config:** GameDefaultMap → L_MainMenu; EditorStartupMap unchanged (L_Arena). L_Arena still opens directly.
6. Runtime-built buttons (not designer tree) + the harmless cruft above — confirm you agree this is within the
   MCP-UMG constraints (same posture QA accepted for WBP_HUD/WBP_VictoryScreen in TASK-011).
7. No C++ changed, no compile run, no Git. No shadow-of-inherited-member concern (no C++ this task).

## Files / assets touched
- NEW: `Content/Maps/L_MainMenu.umap`
- NEW: `Content/Blueprints/BP_MenuGameMode.uasset`
- NEW: `Content/UI/WBP_MainMenu.uasset`
- EDITED: `Config/DefaultEngine.ini` (GameDefaultMap line only)
- NOT touched on disk: L_Arena (reloaded fresh, `is_dirty=false`), the donor `UI_TouchSimple`, any C++.
- The UE Git provider may auto-stage the new assets — left as-is, **no git run**. TASK-052 commits these.
