# TASK-1398 — [MENU-NAV-STATIC-CENSUS] programmer handoff (2026-09-23)

Marker: `TASK-1398-MENU-NAV-STATIC-CENSUS`
Law: `VER-§8` cl. 2 · `SC-§39` · `SC-§50` · `SC-§101` · `SC-§105` · `SC-§134` cl. 7(a)

**Ran:** `Read` / `Grep` / `Glob` only. No code, no asset, no compile, no PIE, no MCP, no git, no editor lifecycle action. Neither running editor (PID 26992 GUI, PID 39980 `-game`) was touched or read.
**Wrote:** this file, and this row's own `status:` line on `TASKBOARD.md`.

---

## 0. HEADLINE — READ THIS BEFORE THE TABLE

⭐⭐ **THE FOUNDING HYPOTHESIS OF THIS EPIC IS REFUTED FOR 8 OF THE 10 MENU SCREENS, AND THAT MAKES THE EPIC BIGGER, NOT SMALLER.**

The board's sizing premise (`TASKBOARD.md:5959`) was: *"the deck builder is not missing navigation, it has navigation on the WRONG LAYER ⇒ much of this epic is likely ROUTING an existing handler through an `IA_Menu*` action, not writing focus logic from scratch."*

Measured, per screen: **the deck builder is the ONLY screen in the project that has list navigation on the wrong layer.** Every other menu screen has **no key handler on ANY layer** — not Slate, not Enhanced Input, not Blueprint-reachable C++. There is nothing to route. See §3 F1.

⭐⭐ **AND THE SUBSYSTEM CANNOT REACH ANY SCREEN BUT THE MAIN MENU — BY THREE INDEPENDENT FENCES, ANY ONE OF WHICH IS SUFFICIENT.** Routing a handler through an `IA_Menu*` action would not be enough on its own; the subsystem would decline to act. See §3 F2.

⛔ **These two findings are the reason this row existed.** Sizing the epic off the routing hypothesis would have under-sized it and produced rows that compile, review clean, and do nothing.

---

## 1. THE (2) STARTING LIST — PROVEN INCOMPLETE, AND EXTENDED

### 1.1 The searches I ran (named, per acceptance)

| # | Search | Result |
|---|---|---|
| S1 | `Glob Content/**/W*_*.uasset` | 10 `WBP_*` assets, all in `Content\UI\` |
| S2 | `Glob Content/UI/**` | 47 entries = 10 widget BPs + 37 textures (`CardArt`, `WarMap` icons) |
| S3 | `Glob Content/**/{UI_,BPI_,BP_Menu,WB}*.uasset` | catches widget assets **not** named `WBP_` — found 6 engine-template widgets + `BP_MenuGameMode`; **no Siegebound widget outside `Content\UI\`** |
| S4 | `Grep "class\s+\w*\s*U\w+\s*:\s*public\s+UUserWidget"` over `Source\` | **18** `UUserWidget` subclasses (16 Siegebound + 2 template) |
| S5 | `Grep "CreateWidget\|AddToViewport\|AddToPlayerScreen\|RemoveFromParent\|RemoveFromViewport"` over `Source\` | every C++ creation/teardown site |
| S6 | `Glob Content/Input/**` | **exactly 3** `IA_Menu*` actions + **1** `IMC_MainMenu` |
| S7 | `Glob Content/Maps/**` | **exactly 2** maps: `L_MainMenu.umap`, `L_Arena.umap` |
| S8 | `Grep "SetGamePaused\|SetPause(\|IA_Pause\|PauseMenuClass\|UPauseMenu"` over `Source\` | **zero matches** |
| S9 | `Glob Saved/.Aura/indexed_files_aura/*WBP_*.json` + reads | parent class, bound-widget variables and BP **function** names for all 10 `WBP_*` — a *partial* pierce of the BP blind spot (§4) |

### 1.2 What the handed list was MISSING — four corrections

⛔ **(a) THE SETTINGS / GRAPHICS / LOGIN SCREENS ARE NOT IN `Content/UI/` BECAUSE THEY HAVE NO ASSET AT ALL.** They are **100% code-authored C++ widgets** whose Blueprint paths are reserved-and-deliberately-unauthored:
- `USettingsMenuWidget` — `/Game/UI/WBP_SettingsMenu` declared RESERVED, not authored (`SettingsMenuWidget.h:63`)
- `USiegeGraphicsMenuWidget` — no `WBP_GraphicsMenu`; resolves `MenuClass ? MenuClass.Get() : USiegeGraphicsMenuWidget::StaticClass()` (`SiegeGraphicsMenuWidget.cpp:573-574`) and its one caller passes `nullptr` (`SettingsMenuWidget.cpp:555-556`) ⇒ **the `StaticClass()` fallback is the live path**
- `UAccountMenuWidget` (the LOGIN screen) — `/Game/UI/WBP_AccountMenu is RESERVED, NOT AUTHORED (verified absent at decomposition)` (`AccountMenuWidget.h:133`)

⇒ ⭐ **For these three, the BP blind spot does not exist — the whole screen is greppable C++.** That is good news and it is measured, not assumed.

⛔ **(b) THERE IS NO PAUSE SCREEN. IT DOES NOT EXIST.** The board's suggested order item 5 (`TASKBOARD.md:5985`) reads *"Victory / defeat + pause."* Measured: **zero** matches for `SetGamePaused`, `SetPause(`, `IA_Pause`, `PauseMenuClass`, `UPauseMenu` anywhere in `Source\` (S8); no pause widget among the 18 `UUserWidget` subclasses (S4); no pause asset (S1–S3). The nearest thing is `USessionMenuWidget`, which `FogVolume.cpp:1164` already names as *"there IS an in-match menu — `USessionMenuWidget` — it just holds no Settings/Graphics entry."*
⇒ ⛔ **A row boarded against "the pause menu" would be boarded against nothing.** Whether one should exist is 🧑 **his product call, not mine.**

⛔ **(c) FOUR SCREENS THE HANDED LIST DID NOT NAME AT ALL:** `USiegeControlsHelpWidget` (+ its two sub-widgets), `USiegeAssistantConsoleWidget`, `USiegeFrameRateCounterWidget`, `UDeckSlotEntryWidget`.

⛔ **(d) SIX ENGINE-TEMPLATE WIDGETS EXIST AND ARE OUT OF SCOPE, NAMED SO NOBODY RE-FINDS THEM:** `Content\Variant_SideScrolling\UI\UI_SideScrolling`, `Content\Variant_Combat\UI\UI_LifeBar`, `Content\{Input,Variant_*}\...\UI_TouchInterface_*` ×3, `Content\Input\Touch\UI_Thumbstick`, `UI_TouchSimple`. They belong to the `Lvl_Combat` / `Lvl_Platforming` / `Lvl_SideScrolling` template maps, which are **not** among the project's 2 shipped maps (S7).

### 1.3 THE SCREEN COUNT

**19 Siegebound widget classes/assets enumerated**, of which:
- **12 are top-level viewport widgets** (`AddToViewport`),
- **10 of those 12 are menu/overlay screens** (HUD and the FPS counter are display-only, no focus stops),
- **7 are focus-relevant sub-widgets** (grid tiles, deck-bar slots, card hand, help rows/detail),
- **0 are pause screens.**

---

## 2. THE TABLE — ONE ROW PER SCREEN, EVERY CELL CITED

Legend for the **layer** column: `SLATE` = a `NativeOnKeyDown`/`NativeOnPreviewKeyDown` override · `IA` = an Enhanced Input action · `NONE` = neither, measured · `BP?` = blind spot (§4).

### 2.1 Menu screens (the epic's actual subjects)

| # | Screen | Class · C++/BP | Asset | Visible top-level ⇒ trips `IsMenuUncovered()` (`SiegeMenuInputSubsystem.cpp:170-199`) | Own key handler | Hooked to `IA_Menu*`? | What focuses it on OPEN | What happens to focus on CLOSE |
|---|---|---|---|---|---|---|---|---|
| 1 | **Main menu** | `WBP_MainMenu` — **BLUEPRINT**, parent is plain `UserWidget` (Aura index, §4) | `/Game/UI/WBP_MainMenu` | It **is** the menu (`:187-190`); `bMenuPresent = true` | **`BP?` — UNKNOWN, BP graph, not text-readable.** Zero C++ handler exists for it | ✅ **YES — the only screen that is.** `IMC_MainMenu` + `IA_MenuUp`/`Down`/`Accept` bound at `SiegeMenuInputSubsystem.cpp:109-111` | `ApplyInitialFocus()` (`:359-372`), fired **once**, `SetTimerForNextTick` at `:116` inside `OnWorldBeginPlay` | ⛔ **Nothing.** No re-arm site exists (§5) |
| 2 | **Deck builder** | `WBP_DeckBuilder` → `UDeckBuilderWidget` — **BOTH** (C++ base + BP layout, `DeckBuilderWidget.h:20-27`) | `/Game/UI/WBP_DeckBuilder` | ✅ yes ⇒ **menu goes inert** while open | ✅ **SLATE, both passes.** `NativeOnPreviewKeyDown` `DeckBuilderWidget.cpp:1769`, `NativeOnKeyDown` `:1791`, both delegating to `HandleCardGridKey` `:1616`. Keys: `Enter`/`Virtual_Gamepad_Accept`/`Gamepad_FaceButton_Bottom` `:1639`; `Delete`/`Gamepad_FaceButton_Left` `:1663`; `Escape`/`Gamepad_FaceButton_Right`/`Virtual_Gamepad_Back` `:1727`; arrows via `NavigationFromKey` `:1737` | ⛔ **NO** — zero `IA_` reference in the file | `AcquireBuilderFocus()` `:1472`, called from `NativeConstruct` `:670`; grid tiles focused by `FocusCardTile` `:1124` (`EFocusCause::Navigation`) | ⛔ **Nothing.** `NativeDestruct` `:1846-1857` clears only a diagnostic flag `bFocusReadbackPending`; **no focus call**. Close/exit itself is `BP?` — no `RemoveFromParent` for the builder exists anywhere in `Source\` |
| 3 | **Settings** | `USettingsMenuWidget` — **C++ ONLY**, tree built in code (`SettingsMenuWidget.cpp:138,176,191,214,225,264,292,302,339,349`) | ⛔ **none** — `WBP_SettingsMenu` RESERVED, unauthored (`SettingsMenuWidget.h:63`) | ✅ yes, ZOrder 10 (`SettingsMenuWidget.h:80`) ⇒ **menu inert** | ⛔ **NONE.** Zero match for `NativeOnKeyDown\|NativeOnPreviewKeyDown\|OnKeyDown\|OnKeyChar\|FReply` in `.h` **or** `.cpp`. Only overrides are `RebuildWidget/NativeConstruct/NativeDestruct` (`:185-187`) | ⛔ **NO** — zero match for `IA_Menu\|IMC_\|InputAction\|BindAction\|EnhancedInput` in either file | ⛔ **NOTHING.** Zero match for any focus API in either file | ⛔ **Nothing.** `BackPressed()` `:499-509` is one log + `RemoveFromParent();` |
| 4 | **Graphics** | `USiegeGraphicsMenuWidget` — **C++ ONLY** | ⛔ none; `StaticClass()` fallback is live (`.cpp:573-574`, caller passes `nullptr` at `SettingsMenuWidget.cpp:555-556`) | ✅ yes, ZOrder **20** (`.cpp:585`) ⇒ **menu inert** | ⛔ **NONE** (same zero-match sweep) | ⛔ **NO** (same zero-match sweep) | ⛔ **NOTHING** | ⛔ **Nothing.** `BackPressed()` `.cpp:2026-2056` = `DisarmVideoModeCountdown(); DiscardStagedVideoMode(...); RemoveFromParent();` |
| 5 | **Login / Account** | `UAccountMenuWidget` — **C++ ONLY** | ⛔ none — RESERVED, verified absent (`.h:133`) | ✅ yes, ZOrder 10 (`.h:159`) ⇒ **menu inert** | ⛔ **NONE**, and stated as law in-file: `.h:170-171` *"No key handling is overridden anywhere in this class — in particular `Escape` stays permanently unabsorbed, project-wide (AS-§6 A-2)."* | ⛔ **NO** | ⛔ **NOTHING** — the three `UEditableTextBox` fields (`.h:489,:493,:532`) open with **no keyboard focus** | ⛔ **Nothing.** `BackPressed()` `.cpp:660-669` = one log + `RemoveFromParent();` |
| 6 | **Multiplayer / Session** | `WBP_SessionMenu` → `USessionMenuWidget` — **C++ + a ZERO-GRAPH BP** (Aura index: 0 variables, 0 functions, §4) | `/Game/UI/WBP_SessionMenu` | ✅ yes — ⭐ **and it is the ONE screen that REMOVES the main menu** rather than covering it (`.cpp:139-141`) | ⛔ **NONE** (zero-match sweep; overrides are `NativeOnInitialized/NativeConstruct/NativeDestruct`, `.h:105-107`) | ⛔ **NO** | ⛔ **NOTHING** | ⛔ **Nothing — and this is the one re-entry gap that is fully C++-visible.** `BackPressed()` `.cpp:151-165` does `LoadClass(WBP_MainMenu_C)` → `CreateWidget` → `MainMenu->AddToViewport();` → `RemoveFromParent();` — **a brand-new main-menu instance with no focus call of any kind** |
| 7 | **Victory / defeat** | `WBP_VictoryScreen` — **BLUEPRINT**, parent is plain `UserWidget` (Aura index, §4) | `/Game/UI/WBP_VictoryScreen` | ✅ yes, ZOrder 10 (`SiegePlayerController.cpp:2245`) — **but on `L_Arena`, where the subsystem never arms** | **`BP?` UNKNOWN** in the graph; ⛔ **no C++ handler.** The C++ relies on Slate's own `SButton::OnKeyDown` Accept path (`SiegePlayerController.cpp:2370-2372`) | ⛔ **NO — and it cannot be.** `SiegePlayerController.cpp:2373-2377`: *"`FInputModeUIOnly` applies `SetIgnoreInput(true)` … so Enhanced Input is deaf under this mode by design, and on `L_Arena` `USiegeMenuInputSubsystem` hard-returns before binding anything at all"* | ⚠️ **AN ATTEMPT THAT CURRENTLY DECLINES.** `InputMode.SetWidgetToFocus(PlayAgainSlate)` `:2405`, guarded by `if (PlayAgainSlate->SupportsKeyboardFocus())` `:2403`. ⛔ **Measured 2026-09-19: `Btn_Jump` carries authored `IsFocusable=False`** (`:2397-2402`) ⇒ the guard declines and logs. **Nothing is focused today** | ⛔ Nothing; `RemoveFromParent()` `:2492` |
| 8 | **War map** ⚠️ FLAGGED | `WBP_WarMap` → `UWarMapWidget` — **C++ + a ZERO-GRAPH BP** (Aura index: 0 vars, 0 funcs, §4) | `/Game/UI/WBP_WarMap` | ✅ yes, ZOrder 4 (`SiegePlayerController.cpp:118`); added **`Collapsed`** (`WarMapWidget.cpp:1081`) ⇒ only counts while open. `L_Arena` only | ⛔ **NO KEY HANDLER.** ⭐ **MOUSE ONLY:** `NativeOnMouseButtonDown` `.h:1136`, `NativeOnMouseWheel` `.h:1154`, `NativeOnMouseMove` `.h:1169`; comparisons are `EKeys::RightMouseButton` `.cpp:2629` / `EKeys::LeftMouseButton` `.cpp:2642` | ⛔ **NO `IA_Menu*`.** Its **toggle** is `IA_WarMap` (`SiegePlayerController.cpp:236`, bound `:742`) — that opens/closes the screen, it does not navigate inside it | ⛔ **NOTHING** — zero focus API in either file | ⛔ Nothing. `CloseWarMap` `:6955-6970` → `ApplyCursorInputState()` → `SetInputMode(FInputModeGameOnly())` `:6472` |
| 9 | **Controls help (Tab)** | `USiegeControlsHelpWidget` + `USiegeControlsDetailWidget` + `USiegeControlsHelpRowWidget` — **C++ ONLY** | ⛔ none — `/Game/UI/WBP_ControlsHelp` RESERVED + unauthored (`SiegePlayerController.cpp:241`); `StaticClass()` fallback `.cpp:2578-2579` | ✅ yes, ZOrder **6** (`SiegePlayerController.cpp:134`); `Collapsed` when shut `.cpp:2928`, ⚠️ **`SelfHitTestInvisible` when open** `.cpp:3011` — which **still reads as visible** to `IsVisible()` | ⛔ **NONE, and the absence is written down as deliberate:** `.cpp:2062-2063` *"⛔ NO KEY HANDLER OF ANY KIND EXISTS BELOW. In particular there is no `NativeOnKeyDown`, no `NativeOnPreviewKeyDown`, no Enhanced Input action and no `FReply::Handled()` on any key"*; repeated for the sub-widgets at `.h:732-733`. The ~35 `EKeys::` tokens are **display data** (`Row.QwertyReferenceKeys`, e.g. `.cpp:1484`) | ⛔ **NO `IA_Menu*`.** Toggle is `IA_ControlsHelp` (`SiegePlayerController.cpp:240`, bound `:761`). It only *reads* other actions for key-chip labels (`QueryKeysMappedToAction` `.cpp:3351`) | ⛔ **NOTHING — worse, it actively DE-focuses:** `Button->IsFocusable = false;` `.cpp:177` (applied to `CloseButton` `:2829`), `DetailScrollBox->SetIsFocusable(false);` `.cpp:2240`, `RowScrollBox->SetIsFocusable(false);` `.cpp:2787` | ⛔ Nothing; `.cpp:2958` *"⛔ NO `SetInputMode` AND NO `bShowMouseCursor` ANYWHERE IN THIS FUNCTION OR THIS FILE."* |
| 10 | **Assistant console** | `USiegeAssistantConsoleWidget` — **C++ ONLY** | ⛔ none; `StaticClass()` fallback taken unconditionally (`.cpp:132-133`) | ✅ yes, ZOrder 5 (`.h:657`, `.cpp:150`). `L_Arena` only | ⚠️ **ONE `SLATE` handler — but it is NOT navigation.** `NativeOnPreviewKeyDown` `.h:723` / `.cpp:845`; the single comparison is `.cpp:884` `if (bAcceptIsLive && bIsUnmodified && InKeyEvent.GetKey() == GetAcceptKey())` → `ConfirmPressed()` `:903`. Accept key = `GetPositionalKey(EKeys::Z)` `.cpp:966`, fallback `EKeys::Z` `:969`. It is a **one-key confirm on a prompt**, no Up/Down, no list | ⛔ **NO `IA_Menu*`.** Toggle is `IA_AssistantConsole` (`SiegePlayerController.cpp:235`, bound `:725`) | ⭐ **YES — the one screen that focuses on open.** `InputBox->SetKeyboardFocus();` `.cpp:1400`, from `OpenConsole` `:513` (+ `:620,:697,:714,:1296`) | ⭐ **YES — the ONLY screen that re-homes focus on close.** `CloseConsole()` `.cpp:628` → `ReleaseKeyboardFocusToGame()` `:645` → `FSlateApplication::Get().SetAllUserFocusToGameViewport();` `.cpp:1416`. Rationale `:1404-1412` *"Without this, focus can stay on a collapsed box and typing goes nowhere."* |

### 2.2 Non-menu top-level widgets — they matter only because they can make the menu inert

| # | Widget | C++/BP | Top-level | Key handler | Note |
|---|---|---|---|---|---|
| 11 | `WBP_HUD` | **BLUEPRINT**, parent plain `UserWidget` (§4) | ✅ ZOrder **0** default (`SiegePlayerController.cpp:444`), **never collapsed in C++** | `BP?` UNKNOWN; no C++ handler | `L_Arena` only. 13 BP functions, all `Update*`/`Setup*` display (§4) |
| 12 | `USiegeFrameRateCounterWidget` | C++ only | ✅ ZOrder **30** (`SiegePlayerController.h:3579`, `.cpp:3285`) | ⛔ NONE | readout only |

### 2.3 Focus-relevant sub-widgets (not top-level, but they are where a ring must land)

| # | Widget | C++/BP | Parent | Focusable? | Key handler |
|---|---|---|---|---|---|
| 13 | `WBP_DeckCardTile` | **BLUEPRINT**, parent plain `UserWidget` (§4) | deck-builder grid | ⭐ **YES** — `Tile->SetIsFocusable(true);` `DeckBuilderWidget.cpp:1089`; focused by `FocusCardTile` `:1124` | `BP?` UNKNOWN; the **parent** builder's Slate handler serves it |
| 14 | `UDeckSlotEntryWidget` (×10, the deck bar) | C++ only; no asset (`StaticClass()` via `CreateWidget<UDeckSlotEntryWidget>(this)` `DeckBuilderWidget.cpp:607`) | deck-builder `DeckBar` | ⛔ no focus API at all | ⛔ **NONE** for keys; `NativeOnMouseButtonDown` `.h:104` only (`RightMouseButton` `.cpp:195`). `.cpp:205-206` states the absence deliberately |
| 15 | `UCardHandWidget` / `WBP_CardHand` | C++ + BP | child of `WBP_HUD` (created BP-side, `CardHandWidget.cpp:94`) | ⛔ none | ⛔ **NONE.** Its keys are the controller's `IA_Card1..6` (`SiegePlayerController.cpp:650,:661`) |
| 16-19 | `USiegeControlsHelpRowWidget`, `USiegeControlsDetailWidget`, `WBP_CastleHealthBar`, `WBP_CombatantHealthBar` (+ `UDamageNumberWidget`) | mixed | children | ⛔ none | ⛔ NONE |

---

## 3. THE FINDINGS, STATED AS FINDINGS

### F1 — ⭐⭐ THE ROUTING HYPOTHESIS IS **REFUTED** FOR 8 OF 10 SCREENS. CONFIRMED FOR 1. PARTIAL FOR 1.

| Verdict | Screens | Evidence |
|---|---|---|
| ✅ **CONFIRMED — navigation exists, wrong layer** | **Deck builder** (1) | Full arrow + Accept + Remove + Back table on Slate: `DeckBuilderWidget.cpp:1616-1766`, entered from `:1769` and `:1791` |
| ⚠️ **PARTIAL — a key handler exists but it is not navigation** | **Assistant console** (1) | One Accept key for a confirm prompt, `.cpp:884`; no Up/Down, no list |
| ⛔ **REFUTED — no handler on ANY layer, nothing to route** | **Settings · Graphics · Login · Session · War map · Controls help · Victory · Main menu(C++ side)** (8) | the zero-match sweeps quoted per row in §2.1 |

⇒ ⛔ **For 8 screens the work is AUTHORING focus logic, not routing it.** `SC-§101`: the hypothesis was measured for one screen and generalised; the generalisation does not hold.

### F2 — ⭐⭐ THREE INDEPENDENT FENCES KEEP `IA_Menu*` OFF EVERY SCREEN BUT THE MAIN MENU

Each is sufficient on its own; all three are live today.

1. ⛔ **THE MAP GATE.** `SiegeMenuInputSubsystem.cpp:46-50` — `if (MapName != MenuMapName) { return; }`, `MenuMapName = TEXT("L_MainMenu")` (`:25`). ⇒ on `L_Arena` **nothing is ever armed**: war map, controls help, assistant console, victory and HUD are structurally out of reach. Corroborated independently at `SiegePlayerController.cpp:2375-2376`.
2. ⛔ **THE COVERAGE GATE.** `IsMenuUncovered()` `:170-199` returns `false` the instant **any other visible top-level widget** exists (`:191-196`), and every handler early-outs on it (`MoveFocus` `:306`, `HandleMenuAccept` `:336`, `ApplyInitialFocus` `:361`). ⇒ **the moment settings / graphics / login / the deck builder opens, the `IA_Menu*` actions go inert BY DESIGN** (stated as intent at `.h:118-122`). That gate is correct for what it was built for and is exactly wrong for 🧑 his ask.
3. ⛔ **THE TREE GATE.** `GetMenuButtons()` `:201-220` walks **only** `FindMainMenuWidget()`'s tree (`:204`) and collects **only** `Cast<UButton>` (`:214`).

⇒ ⛔ **"Route the existing handler through an `IA_Menu*` action" is not sufficient for any screen.** The action would fire and the subsystem would decline.

### F3 — ⭐ THE `IA_Menu*` VOCABULARY IS **UP / DOWN / ACCEPT ONLY**. THERE IS NO LEFT, NO RIGHT, NO BACK.

Measured (S6): `Content\Input\Actions\` contains exactly `IA_MenuUp.uasset`, `IA_MenuDown.uasset`, `IA_MenuAccept.uasset`. `IMC_MainMenu` `mapping_count: 6` — `Up`+`Gamepad_DPad_Up`, `Down`+`Gamepad_DPad_Down`, `Enter`+`Gamepad_FaceButton_Bottom` (`handoffs/TASK-1274-programmer.md:21-27`).

⇒ 🧑 **his words were "scrollable with the outline and arrow keys" (plural).** A settings row with a **detent stepper** (`UButton` pairs, `SiegeGraphicsMenuWidget.cpp:1196`, `:1254`) or a **slider** (`USlider`, `:1052`) needs **Left/Right**, and a Back needs its own action. ⛔ **Those actions do not exist and creating them is asset work (art-director / an asset row), not code.** Flagged, not decided.

### F4 — ⭐ THE WIDGET VOCABULARY EXCEEDS `UButton`, SO A GENERIC "WALK THE BUTTONS" RE-USE WOULD SILENTLY SKIP EXACTLY WHAT 🧑 HE NAMED

`GetMenuButtons` casts to `UButton` only (`:214`). Measured control types per code-authored screen:

| Screen | Interactive control types (cited) |
|---|---|
| Settings | `UCheckBox` `:225` · `UButton` `:302`, `:349` |
| **Graphics** | `UCheckBox` `:871` · **`USlider` `:1052`** · `UButton` `:1196`, `:1254`, `:1510` · inside a `UScrollBox` `:702` |
| Login | `UButton` `:340` · **`UEditableTextBox` `:380`** |
| Controls help | `UButton` `:1857`, `:2326`, `:2825` · `UScrollBox` `:2233`, `:2780` (**both explicitly `SetIsFocusable(false)`**) · `UWidgetSwitcher` `:2667` |

⇒ 🧑 **his sentence was specifically about the settings menu.** A `UButton`-only walker lands on the Back/Graphics buttons and **skips every checkbox and the slider** — it would look implemented and fail his ask.
⭐ **One helpful existing primitive:** the graphics panel already calls `RootScrollBox->ScrollWidgetIntoView(...)` (`SiegeGraphicsMenuWidget.cpp:2232`), so *"scrollable with the outline"* has a hook already in the file.

### F5 — 🧑 HIS RE-ENTRY GAP, MECHANISM CONFIRMED AT SOURCE (and it is `TASK-1400`'s to fix, not mine)

`ApplyInitialFocus()` (`:359-372`) has **exactly one caller**: `SetTimerForNextTick` at `:116`, inside `OnWorldBeginPlay`. Closing a sub-screen does not re-enter `OnWorldBeginPlay` (no level load — `SessionMenuWidget.cpp:134-136` records that reloading `L_MainMenu` on Back was deliberately refused). ⇒ ⛔ **the hypothesis in the row's (4) is CONFIRMED: nothing re-fires it.**
**The same shape recurs at:** deck-builder `NativeDestruct` `:1846-1857` (no focus call) · `USettingsMenuWidget::BackPressed` `:499-509` · `USiegeGraphicsMenuWidget::BackPressed` `:2026-2056` · `UAccountMenuWidget::BackPressed` `:660-669` · `USessionMenuWidget::BackPressed` `:151-165`. **Five close paths, zero focus calls.** The lone counter-example in the whole project is the assistant console (`:645`).

### F6 — ⚠️ AN INCIDENTAL DEFECT FOUND AND **NOT** FIXED (out of scope; reported so it is not lost)

`Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp:1161` reads *"and `USettingsMenuWidget::CreateAndAddToViewport` has ZERO call sites"* — and flags itself as **load-bearing** (`:1158-1159`, *"the day it stops being true is the day somebody must re-read this"*).
⛔ **That function does not exist.** `Grep CreateAndAddToViewport` over `SettingsMenuWidget.h` ⇒ **zero matches**; `USettingsMenuWidget` declares no such member. The real function is `USiegeGraphicsMenuWidget::CreateAndAddToViewport` (`SiegeGraphicsMenuWidget.h:330`), which has **exactly one** C++ caller (`SettingsMenuWidget.cpp:555-556`) and is **not** `UFUNCTION(BlueprintCallable)` (plain `static`), so no BP graph can reach it either.
⭐ **THE COMMENT'S CONCLUSION SURVIVES; ONLY ITS CITATION IS WRONG.** The graphics panel is still unreachable in a match: its one caller is the settings panel, which is created only from `WBP_MainMenu` on `L_MainMenu`. ⛔ **A future reader checking that named symbol would find nothing and could conclude either "fixed" or "the file moved" — a fail-silent in a comment that asks to be re-read.** No code was touched (this row writes prose only).

---

## 4. ⭐ THE (3) BLUEPRINT BLIND SPOT — DECLARED PER SCREEN, WITH A PARTIAL INSTRUMENT AND ITS **PROVEN LIMIT**

### 4.1 The instrument I found, and what it can and cannot see

`Saved\.Aura\indexed_files_aura\*.json` holds an Aura index record per asset carrying **parent class**, **bound-widget variables**, **BP function names**, **referencers** and **dependencies**. It is a real partial pierce and I used it.

⛔ **BUT ITS LIMIT IS PROVEN, NOT ASSUMED — I HAVE A POSITIVE CONTROL:**
- `BP_MenuGameMode` **must** have an `Event BeginPlay` (it depends on `/Game/UI/WBP_MainMenu`, and `SessionMenuWidget.cpp:146-147` records that it owns `L_MainMenu`'s UIOnly + cursor posture *at level boot*). Its index record lists **one** function: `UserConstructionScript`.
- `WBP_MainMenu` **must** have an `Event Construct` (the subsystem's own comment `:210-211` says *"buttons the Blueprint's Construct added to `VerticalBox_0` at runtime"*, and `TASK-1274` measured 7 of them, `Button_0..6`). Its index record lists **one** function: `BuildSandboxButton`.

⇒ ⛔ **PROVEN: the index does NOT enumerate EventGraph events.**
⇒ ⚠️ **UNPROVEN, AND THEREFORE NOT CLAIMED: whether it lists an overridden UMG *function* such as `On Key Down` (which returns `FReply` and gets its own graph).** I have no positive control for that case. **A missing `OnKeyDown` in an index record is NOT an absence** (`SC-§39`). ⛔ **This is the exact discriminator `TASK-1399` should settle at runtime.**

### 4.2 Per-screen blind-spot declaration

| Screen | Verdict |
|---|---|
| **`WBP_MainMenu`** | 🚨 **BP-OPAQUE, AND IT IS THE WORST ONE.** Parent = plain `UserWidget`. Index lists 1 function (`BuildSandboxButton`) and 1 variable (`In String = Hello`). **The 7 menu entries, their labels, their `OnClicked` handlers, and any key override are all in the EventGraph the index cannot see.** Its dependencies (`WBP_DeckBuilder`, `WBP_SessionMenu`, `/Script/GitClaudeUnrealTest`) confirm it is the creator of the sub-screens, but ⛔ **it does NOT depend on `IMC_MainMenu` or any `IA_Menu*`** — consistent with the subsystem owning all of that. Hand to `TASK-1399`. |
| **`WBP_DeckBuilder`** | ⚠️ BP-OPAQUE for the graph, but **C++-visible for key handling** — the C++ base carries both Slate passes. Index: parent `DeckBuilderWidget`, 14 bound widgets, 5 functions (`BuildSandboxButton, RefreshAll, BuildDetailsPanel, RefreshDetailsPanel, SplitGrid`). ⛔ Its **Exit/close** path is BP-opaque (no `RemoveFromParent` for the builder exists in `Source\`). |
| **`WBP_SessionMenu`** | ✅ **BLIND SPOT EFFECTIVELY CLOSED.** Index: parent `SessionMenuWidget`, **0 variables, 0 functions** — a zero-graph WBP, corroborated independently at `SessionMenuWidget.cpp:130-131` (*"the TASK-355 route-(A) zero-graph WBP"*). Subject to §4.1's unproven-override caveat. |
| **`WBP_WarMap`** | ✅ **BLIND SPOT EFFECTIVELY CLOSED.** Index: parent `WarMapWidget`, **0 variables, 0 functions**. Same caveat. |
| **`WBP_VictoryScreen`** | 🚨 **BP-OPAQUE.** Parent = plain `UserWidget` (**no C++ base at all**). Index: 2 variables, 1 function (`SetWinner`). |
| **`WBP_HUD`** | ⚠️ BP-OPAQUE. Parent = plain `UserWidget`, 13 variables, 13 functions — all `Update*`/`Setup*`/`Show*`. No navigation-shaped name, but that is suggestive, **not** an absence. |
| **`WBP_CardHand`** | ⚠️ BP-OPAQUE. Parent `CardHandWidget`, 12 vars, 7 functions. |
| **`WBP_DeckCardTile`** | ⚠️ BP-OPAQUE. Parent plain `UserWidget`, 12 vars, 3 functions. **It is the grid's focus stop**, so its graph matters. |
| **`BP_MenuGameMode`** | 🚨 BP-OPAQUE — **owns `L_MainMenu`'s input posture** (`SessionMenuWidget.cpp:146-147`) in an EventGraph the index cannot read. |
| **Settings · Graphics · Login · Controls help · Assistant console · FPS counter · Deck slot entry** | ✅ **NO BLIND SPOT — 100% C++, no asset exists.** Their zero-handler / zero-focus findings are **complete**, not merely unenumerated. |

---

## 5. ⭐⭐ (4) THE RE-ARM ANSWER — **ONE ROW**, WITH THE ARGUMENT

**Answer: a single shared re-arm point IS reachable. The re-entry fix is ONE ROW, not four.**

**The argument, in four measured steps:**

1. ⭐ **THE PREDICATE AND THE ACTION ALREADY EXIST, TOGETHER, IN ONE FUNCTION.** `ApplyInitialFocus()` (`:359-372`) *is* literally *"if the menu is uncovered and nothing is focused, focus button 0"* — which is 🧑 **exactly his product fact (a): the ring goes on the TOP option, not the one he came back from.** Nothing has to be written; something has to **call it**.
2. ⭐ **IT IS SAFE TO CALL AT ANY INSTANT, BY CONSTRUCTION.** Every input it needs is resolved **live, never cached**: the menu instance (`:157-159`, *"resolved LIVE on every call and never cached across inputs"*), the button list (`:212-219`), the focused button (`:222-240`), the coverage predicate (`:170-199`). It is also **idempotent** — `:368` guards `&& !GetFocusedMenuButton()`. ⇒ a repeated call is a no-op, so the trigger does not have to be precise.
3. ⛔ **THE PER-CLOSE-PATH ALTERNATIVE IS NOT EVEN FULLY AVAILABLE.** The four sub-screens do **not** share a close mechanism, and at least one is out of C++'s reach: deck-builder Exit is **BP-only** (no `RemoveFromParent` for the builder anywhere in `Source\`); settings/graphics/login Backs are C++ (`:508`, `:2055`, `:668`) but their **opens** are BP; session Back is C++ and already re-creates the menu (`:151-165`). ⇒ *"touch every close path"* would be **4+ code rows plus at least one Blueprint edit**, and it would still miss any future screen. **The N-row route is both larger and leakier.**
4. ⚠️ **THE ONE THING THAT ACTUALLY BLOCKS IT IS TRIVIAL AND IS IN THE SAME FILE.** `ApplyInitialFocus()` is declared under `private:` (`SiegeMenuInputSubsystem.h:177`, `:192`) and is **not** a `UFUNCTION` (**zero** `UFUNCTION` matches in that header). ⇒ today **nothing outside the class can call it** — neither another translation unit nor any Blueprint graph. A shared re-arm therefore lives **inside `SiegeMenuInputSubsystem.cpp/.h` and nowhere else.**

⛔ **WHAT I AM NOT ANSWERING, DELIBERATELY:** *which* trigger (a repeating timer, `FTickableGameObject`, a viewport/focus delegate) is `TASK-1400`'s design choice. I measured only that the subsystem **has no tick and no timer today** other than the one-shot at `:116`, so the trigger is new code — **one mechanism, one file.**

⇒ ⭐ **This resolves `TASK-1400`'s pre-branched scope (3) to branch (i): a single shared re-arm covers every sub-screen.**
⚠️ **SCOPE FENCE:** this answers *re-arming the MAIN MENU on return*. It does **not** give the sub-screens their own navigation — that is F1/F2's problem and it is the other 90% of the epic.

---

## 6. SUGGESTED GROUPING INTO WORK UNITS + ROW-COUNT ESTIMATE

⛔ **SIZING INPUT ONLY. This is not a recommendation to build anything — scope is 🧑 HIS call** (`SC-§100`), and nothing here is settled until `TASK-1399` measures the runtime half.

| # | Work unit | Screens | Shape | Why it groups |
|---|---|---|---|---|
| A | **Main-menu re-entry** | Main menu | **1 site** | §5. Already specced as `TASK-1400`. Smallest, and it is 🧑 his named gap |
| B | **The three code-authored panels** | Settings · Graphics · Login | **authoring, not routing** | Identical shape: pure C++, no asset, zero handlers, zero focus, same `BackPressed` idiom. ⛔ **Graphics is the heavy one** (`UCheckBox` + `USlider` + stepper `UButton`s in a `UScrollBox`) and is the one that needs F3's missing Left/Right |
| C | **Deck builder** | Deck builder (bar + grid) | ⭐ **the ONLY true routing job** | Handler exists at `:1616-1766`; the work is making it answer `IA_Menu*` **and** getting past F2's coverage gate |
| D | **Session + Victory** | Session menu · Victory screen | authoring + **one asset flip** | Victory additionally needs `Btn_Jump`'s authored `IsFocusable=False` flipped in the Blueprint editor and hand-saved (`SiegePlayerController.cpp:2397-2402`) — ⛔ **an art/asset row, not a code row** |
| E | **In-match overlays** | Controls help · Assistant console (· HUD) | ⛔ **BLOCKED ON A PRIOR DECISION** | F2 fence 1: the subsystem never arms on `L_Arena`. ⛔ **Someone must first decide whether `IA_Menu*` extends to `L_Arena` at all** — that is a design question, not a row |
| F | **Infrastructure the above share** | — | ⛔ **NOT OPTIONAL** | F2's three fences + F3's missing actions + F4's `UButton`-only walker. ⛔ **If this is not boarded, B/C/D produce rows that compile, review clean and do nothing** |
| — | ⚠️ **War map** | War map | 🧑 **FLAGGED, NOT SIZED** | Mouse-only spatial pan/zoom/click (`.h:1136`, `:1154`, `:1169`). ⛔ *"Arrow keys walk a list"* may be the wrong design. ⛔ **I propose nothing here — 🧑 he owes a design sentence first** |
| — | ⛔ **Pause** | — | ⛔ **DOES NOT EXIST** (§1.2b) | Not a row until 🧑 he says a pause menu should exist |

**Row-count estimate — and it is HIGHER than the board's `≈22–25`:**

| Unit | Code | QA | Verify | Subtotal |
|---|---|---|---|---|
| A | 1 | 1 | 1 | 3 |
| B | 3 | 3 | 3 | 9 |
| C | 1 | 1 | 1 | 3 |
| D | 2 code + 1 asset | 2 | 2 | 7 |
| F (infrastructure) | 2 code + 1 asset (the new `IA_` actions, F3) | 2 | 2 | 7 |
| E | ⛔ gated on a decision | — | — | 0–6 |
| census + commit hosts | — | — | — | 3 |
| **TOTAL** | | | | ⭐ **≈32 rows (38 if E proceeds)** |

⛔ **WHY IT GREW RATHER THAN SHRANK, IN ONE LINE:** the board estimated on the assumption that most screens had navigation on the wrong layer (**routing, cheap**). Measured, 8 of 10 have **no navigation at all** (**authoring, dearer**), and F2 adds a shared-infrastructure unit nobody had costed.

---

## 7. WHAT `TASK-1399` SHOULD MEASURE (handed over, per (3))

1. ⛔ **The §4.1 discriminator first:** does an overridden UMG `On Key Down` appear in an Aura index record? A single positive or negative control settles the whole BP blind spot for this epic.
2. `WBP_MainMenu`'s 7 `Button_0..6` labels in order (BP-opaque; `TASK-1274` measured the count and index 2 = `"Deck Builder"`, never the sequence).
3. Whether `WBP_MainMenu` / `WBP_VictoryScreen` / `WBP_HUD` / `WBP_CardHand` / `WBP_DeckCardTile` override any key handler in their graphs.
4. ⭐ **The F2 prediction, which is the cheapest high-value control available:** inject `IA_MenuDown` **while the settings or deck-builder panel is open**. My reading predicts **no focus movement** — `IsMenuUncovered()` returns `false` at `:195`. A *positive* there would refute §3 F2 and I would want to know.
5. Whether `WBP_DeckBuilder`'s Exit button re-creates or re-shows the main menu (BP-opaque; decides whether A's trigger must survive a fresh `WBP_MainMenu` instance — note `USessionMenuWidget::BackPressed` demonstrably **does** create a fresh one, `:151-165`).

---

## 8. Not examined / limitations

- ⛔ **Every `.uasset` EventGraph.** Read through the Aura index only, whose EventGraph blindness is **proven** (§4.1) and whose function-override coverage is **unproven**. No asset was opened.
- ⛔ **The Aura index may be stale.** I did not verify any record against the editor's live asset registry — that needs the engine, which this row may not touch. Timestamps not checked.
- ⛔ **`IMC_MainMenu`'s live key table** was taken from `handoffs/TASK-1274-programmer.md:21-27`, not re-measured (`.uasset`, needs the editor). I did independently confirm the **asset set** on disk (S6).
- ⛔ **No runtime behaviour was observed.** Every "does nothing" in this document is a **static** claim about code that exists, never a claim about what happened in a session.
- ⛔ **Slate's own default navigation is NOT censused.** `FNavigationConfig` moves focus between focusable widgets with arrows **with no project code at all**, so a screen reported here as "no handler" may still navigate under a **real** key while remaining un-drivable by `inject_input_action` (`VER-§5` cl. 5, unrelaxed under `1.0.6`, `CONVENTIONS.md:12273`). ⛔ **That is precisely the invisible failure 🧑 his ask is aimed at, and only `TASK-1399` can separate the two.**
- ⛔ **Two files I read are outside the row's enumerated READS list** and I name them rather than hide them: `Saved\.Aura\indexed_files_aura\*.json` (the §4 instrument — read-only, and finding an instrument that pierces the BP blind spot is this row's core duty) and `CONVENTIONS.md` (read only, for the `VER-§5` cl. 5 citation; the row's prohibition on `CONVENTIONS.md` is a **write** prohibition).
- ⛔ **Sub-agent-assisted.** Two read-only `Explore` agents gathered §2.1 rows 3-6 and 8-10 plus the cross-cutting input-mode census. **Every cell they returned carries a file:line and I spot-checked the load-bearing ones myself** (the `FogVolume.cpp:1161` misattribution in F6 was independently re-measured by me before being written down, `SC-§138`).
- ⛔ **Row counts in §6 are estimates from static shape**, not from measured effort. The `≈32` is honest arithmetic over the grouping, not a promise.
