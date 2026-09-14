# TASK-1274 — [MENU-INPUT-ACTIONS] programmer handoff (2026-09-14)

Status flipped: `ready-for-qa`. Gate: see the row boarded 2026-09-14 (qa-reviewer). Verify: `playtest-verifier` (acceptance (1)). Host: the same build-master wave as TASK-1270/1271 (acceptance (4)).

⛔ Not compiled, not Live-Coded, nothing staged, no Git. The compile gate is build-master's.

## 1. The measured class on `L_MainMenu` (read live from the editor, PID 6136, `execute_unreal_python_readonly`)

| what | measured value |
|---|---|
| `L_MainMenu` game mode | `/Game/Blueprints/BP_MenuGameMode.BP_MenuGameMode_C` — `isGameModeBase=True isGameMode=False` (parent `GameModeBase`; `handoffs/TASK-049.md:22-23` records the WorldSettings override that names it) |
| `BP_MenuGameMode_C` CDO `PlayerControllerClass` | `/Script/Engine.PlayerController` |
| `BP_MenuGameMode_C` CDO `DefaultPawnClass` | `/Script/Engine.DefaultPawn` (= the `DefaultPawn_0` N3 read) |
| `BP_MenuGameMode_C` CDO `HUDClass` | `/Script/Engine.HUD` |
| project-wide default | `Config/DefaultEngine.ini:12 GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode` — overridden by the map |

⇒ **No project C++ class runs on the menu map.** The row's deliverable (2) says "the class that runs on L_MainMenu … adds IMC_MainMenu at BeginPlay". The two ways to obtain such a class are (a) assign `PlayerControllerClass` on `BP_MenuGameMode` (a saved edit to `Content/Blueprints/BP_MenuGameMode.uasset`, an asset the row's WRITES list does not name) or (b) a code-side path that touches no asset. The dispatch said "prefer a code-side path and say which" ⇒ **(b): `USiegeMenuInputSubsystem : UWorldSubsystem`**, gated on the map name at `OnWorldBeginPlay`, Game | PIE worlds only. `L_MainMenu` is NOT dirtied; `BP_MenuGameMode` is NOT dirtied (acceptance (5) — census below).

Ordering proof (engine 5.8 source, quoted in the header): `SpawnPlayActor` (creates the `APlayerController`, runs `SetupInputComponent`) precedes `World->BeginPlay()` on both the PIE path (`GameInstance.cpp:538` vs `:566`) and the LoadMap path (`UnrealEngine.cpp:16647` vs `:16661`); `UWorld::BeginPlay` runs subsystem `OnWorldBeginPlay` BEFORE `GameMode->StartPlay()` (`World.cpp`), i.e. before the Blueprint's BeginPlay creates `WBP_MainMenu` — so the bindings are installed at BeginPlay, the menu widget is resolved lazily per input, and the initial focus is placed via `SetTimerForNextTick`.

## 2. The key table (`IMC_MainMenu`, read back with `get_input_mapping_context_keys` — `mapping_count: 6`; on-disk string census of `IMC_MainMenu.uasset`: `IA_MenuUp 2 · IA_MenuDown 1 · IA_MenuAccept 1 · Gamepad_DPad_Up 1 · Gamepad_DPad_Down 1 · Gamepad_FaceButton_Bottom 1 · Enter 1`)

| action (Boolean, `bConsumeInput=True` — the `IA_Card1` pattern) | keyboard | gamepad | handler | bound on |
|---|---|---|---|---|
| `/Game/Input/Actions/IA_MenuUp` | `Up` | `Gamepad_DPad_Up` | focus previous button, wrapping | `ETriggerEvent::Started` |
| `/Game/Input/Actions/IA_MenuDown` | `Down` | `Gamepad_DPad_Down` | focus next button, wrapping | `ETriggerEvent::Started` |
| `/Game/Input/Actions/IA_MenuAccept` | `Enter` | `Gamepad_FaceButton_Bottom` | focused `UButton`'s `OnClicked.Broadcast()` | `ETriggerEvent::Started` |

⛔ No letter (`KBD-§4`); ⛔ Tab untouched; ⛔ no modifiers, no triggers on any mapping. `IMC_MainMenu` priority 0 (nothing else is applied on the menu — `applied_mapping_contexts: []`, TASK-671). The context is applied VERBATIM — no `GetPositionalContext` call, so the `KBD-§` remap is never entered.

## 3. Files (`git status --porcelain`, quoted; git root is one level up, `SC-§102`)

```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                                   ← the manager's concurrent edits + my one status-line flip
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuAccept.uasset                      ← NEW (1130 B)
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuDown.uasset                        ← NEW (1122 B)
?? GitClaudeUnrealTest/Content/Input/Actions/IA_MenuUp.uasset                          ← NEW (1114 B)
?? GitClaudeUnrealTest/Content/Input/IMC_MainMenu.uasset                               ← NEW (4176 B)
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp
```
Plus this handoff (written after the status was taken). Acceptance (6): only the named files (the test file is NEW, see §5 for why). Acceptance (3): `git diff -- .claude/pipeline/CONVENTIONS.md | wc -l` = **0** — the `KBD-§` tables are untouched. `IMC_Hero` / `IMC_Default` / `IMC_MouseLook` / `L_MainMenu` / `L_Arena` / `BP_MenuGameMode` / `WBP_MainMenu`: `git status --porcelain` on those paths is EMPTY (clean).

How the assets were authored: `create_input_actions` (3 IAs, `Digital` = `ValueType=BOOLEAN`), the IMC via `AssetTools.create_asset(... InputMappingContext_Factory)`, the six mappings via `add_input_action_to_mapping_context` (an editor-time authoring append — `KBD-§2a`'s carve; ⛔ no `MapKey` in `Source/`). Those tools auto-saved: the files were on disk before any save call of mine, and the editor's dirty census read `get_dirty_content_packages() = []`, `get_dirty_map_packages() = []` afterwards — every fenced asset `dirty=False`, every new asset `dirty=False`. No save of mine was needed, so none was issued.

## 4. The focus brush (deliverable (3))

`FSlateApplication::SetUserFocus(UserIndex, ButtonSlateWidget, EFocusCause::Navigation)`. `SlateApplication.cpp:3099` sets `ShowFocus = (InCause == EFocusCause::Navigation)`; `SWidget::Paint` (`SWidget.cpp:1748-1751`) draws `GetFocusBrush()` when the user's `ShouldShowFocus` is true; `SWidget::GetFocusBrush` (`SWidget.cpp:1012-1014`) returns `FAppStyle::Get().GetBrush("FocusRectangle")`, which is `FCoreStyle`'s `"Old/DashedBorder"` border brush, margin 6/32, `FLinearColor(1,1,1,0.5)` (`CoreStyle.cpp:305`). ⇒ **the focused button wears Slate's default dashed white (50 % alpha) focus rectangle.** No button style is edited; `SButton::OnFocusReceived` (`SButton.cpp`) changes no brush, so the hover tint stays the mouse's. The initial focus lands on `Button_0` ("Play (vs Bot)") one tick after BeginPlay — a visible change at boot for mouse users, stated here for Jonathan's eye. `UWidget::SetUserFocus` was NOT used because it passes `SetDirectly`, which paints nothing.

## 5. The test — `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` (`Tests/SiegeMenuInputTest.cpp`, NEW)

Asserts STATE (`SC-§104`): after `IA_MenuDown` ×2 from cold, `GetFocusedMenuButton()`'s label is `"Deck Builder"`; after `IA_MenuAccept`, exactly one `UDeckBuilderWidget` is a top-level viewport widget. Also pins the cold precondition (no button focused, or index 0), `IsArmed()`, the live `WBP_MainMenu` instance, ≥3 buttons, 0 builders before input, and the pure wrap ring (`WrapIndex`: 0+2→2, 0−1→6, 6+1→0, count 0→INDEX_NONE).

⚠️ QA, scrutinize this: **it is the suite's first latent PIE test.** The observable (Slate focus inside the live Blueprint menu, the Blueprint's own `OnClicked` handler creating the builder) does not exist without the map, so the test calls `AutomationOpenMap("/Game/Maps/L_MainMenu")` (in the editor `UEditorEngine::AutomationLoadMap` loads the map and starts a 1-client PIE — `EditorEngine.cpp:6627`), injects through `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` (the SAME door Aura's `inject_input_action` uses, the one `qa/TASK-787-verify.md` proved reaches our mappings), and ends with `FExitGameCommand`. Waits: 1.0 s after load, 0.25 s between injections, 0.5 s after Accept (real-time `FWaitLatentCommand`, the engine's own map-test idiom). Why a NEW file: the row's names list says "the existing menu/UI automation test file", but none exists for the main menu — `SiegeGraphicsMenuTest.cpp` / `SiegeSettingsTest.cpp` are panel/persistence tests built on `NewObject` scratch widgets with no world; putting a map-driving latent test inside one of them would change their offline character. If QA rules otherwise it is a file move, not a logic change. Flags mirror the suite (`EditorContext | EngineFilter`).

⚠️ Disk touch, stated: opening the shipped builder runs `UDeckBuilderWidget::NativeConstruct` → `MigrateToFixedSlots` on the ACTIVE profile's deck slot, which writes ONLY when a migration is pending (`DeckBuilderWidget.cpp:545-550`); post-`d8bfd23` saves are ten-slot, so it is a read. This is the same open the row's acceptance (1) asks the verifier to perform. The test mutates no deck and saves nothing itself.

⚠️ For build-master: in a `-nullrhi` / unattended lane the PIE start needs a game viewport; if the suite lane cannot host PIE, this test will fail at step 1 by name (`USiegeMenuInputSubsystem exists on the L_MainMenu world`) rather than pass silently — say so in the wave note rather than skipping it.

## 6. Acceptance evidence

| # | criterion | evidence |
|---|---|---|
| (1) RUNTIME | verifier's — `inject_input_action IA_MenuDown ×2 + IA_MenuAccept` opens the builder (`ui_snapshot` shows `DeckBar` with 10 children) | not mine; recipe note: inject with a hold ≥ 1 frame; the handlers are inert while ANY other visible top-level widget covers the menu (so a second Accept after the builder opens does nothing — no double-open) |
| (2) mouse unchanged | Accept calls `UButton::OnClicked.Broadcast()` — `UButton::SlateHandleClicked` (`Button.cpp:278-280`) IS `OnClicked.Broadcast()`; no `OnClicked` body touched, `WBP_MainMenu` untouched (`git status` clean on it). One click per button still fires — measured in PIE by the verifier |
| (3) `KBD-§` untouched | `git diff -- CONVENTIONS.md` = 0 lines (quoted §3) |
| (4) compile + suite +1 | build-master's wave |
| (5) `L_MainMenu` not dirtied | class assignment lives in CODE (`UWorldSubsystem`), not in the map and not in `BP_MenuGameMode`; editor dirty census after authoring: `L_MainMenu=False L_Arena=False BP_MenuGameMode=False WBP_MainMenu=False`; `git status` clean on `Content/Maps`, `BP_MenuGameMode.uasset`, `WBP_MainMenu.uasset`. Saved assets = the four NEW ones only |
| (6) only named files | §3 quoted; the one extra is `Tests/SiegeMenuInputTest.cpp` (NEW instead of an "existing" file — §5) |

## 7. The honest side (from the row's "why not", made concrete)

`BP_MenuGameMode` sets `FInputModeUIOnly` at boot (CONVENTIONS "Input-mode ownership"). `FInputModeUIOnly::ApplyInputMode` calls `GameViewportClient.SetIgnoreInput(true)` (`PlayerController.cpp`), and `UGameViewportClient::InputKey` returns early on `IgnoreInput()` (`GameViewportClient.cpp:767`) ⇒ a REAL keyboard/gamepad press never reaches the player controller on this map, so it never reaches these Enhanced Input handlers. What reaches them is injection (Aura, the test). The PLAYER benefit is the initial Slate focus on `Button_0`: with a focused `SButton`, Slate's own navigation config handles arrow keys and `SButton::OnKeyDown`'s Accept path (`SButton.cpp:296`) handles Enter / gamepad Accept. That real-key route is a Slate-side consequence, UNMEASURED here (no agent has a real-input lane) — 🙋 owed to Jonathan's hands: on the menu, press Down twice and Enter; the dashed rectangle should walk to "Deck Builder" and open it.

## 8. What QA should scrutinize

- `USiegeMenuInputSubsystem::FocusButton` — the user-index resolution mirrors `UWidget::SetUserFocus` (`Widget.cpp`) but passes `EFocusCause::Navigation`; fallback to `LocalPlayer->GetSlateOperations().SetUserFocus(...)` when the immediate set is refused.
- `IsMenuUncovered()` — the "menu is the only visible top-level user widget" gate; if the menu map ever gains an always-present overlay widget, the feature goes inert (loud in the test, silent in play — name it if you know of one).
- `GetMenuButtons` order = `UWidgetTree::ForEachWidget` slot order — the row's `Button_0..6`; QA may want the verifier to quote the label sequence.
- `Cast<const UTextBlock>` / `Cast<const UPanelWidget>` in `GetButtonLabel` (const-correct; the first draft cast away const and was fixed before handoff).
- `TObjectPtr<const UInputMappingContext>` UPROPERTY — precedent `SiegeKeyboardLayoutSubsystem.h:414`.
- Engine signatures checked at the 5.8 headers, not memory: `AddMappingContext(const UInputMappingContext*, int32, const FModifyContextOptions&)` (`EnhancedInputSubsystemInterface.h:265`), `FSlateApplication::SetUserFocus(uint32, const TSharedPtr<SWidget>&, EFocusCause)` (`SlateApplication.h:678`), `UButton::OnClicked` public (`Button.h:76`), `UWidget::HasUserFocus(APlayerController*) const` (`Widget.h:664`), `UWidgetTree::ForEachWidget(TFunctionRef<void(UWidget*)>) const` (`WidgetTree.h:78`), `FTimerManager::SetTimerForNextTick(UserClass*, TMethodPtr)` (`TimerManager.h:249`), `UWorld::RemovePIEPrefix` static (`World.h:4456`), `InjectInputForAction(const UInputAction*, FInputActionValue, const TArray<UInputModifier*>&, const TArray<UInputTrigger*>&)` (`EnhancedInputSubsystemInterface.h:153`), `AutomationOpenMap` / `FWaitLatentCommand` / `FExitGameCommand` (`AutomationCommon.h:213/218/244`), `FFunctionLatentCommand` (`AutomationTest.h:4738`).

M8: adds no replicated property, no new replicated class, no RPC, no new relevancy tier.

## Loop 1 fix (2026-09-14)

**Trigger:** `qa/TASK-1280-report.md` `## Build errors (TASK-1281)` — compile `Result: Succeeded`, suite 556 / 554 / 2 red ×2, `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` red DETERMINISTIC on exactly ONE captured engine error while every assertion of its own passed. QA loop 1 of 3 (routing rule 6).

### The cause, read at the engine source (not the handoff's claim)
`FInputModeUIOnly::SetWidgetToFocus` — `Engine/Source/Runtime/Engine/Private/PlayerController.cpp:6341-6347`:
```cpp
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (InWidgetToFocus.IsValid() && !InWidgetToFocus->SupportsKeyboardFocus())
	{
		UE_LOGF(LogPlayerController, Error, "InputMode:UIOnly - Attempting to focus Non-Focusable widget %ls!", *InWidgetToFocus->ToString());
	}
#endif
```
The `%ls` is the `SWidget::ToString()` of `WBP_MainMenu`'s `SObjectWidget` — `"SObjectWidget [Widget.cpp(976)]"`, its CREATION site (`UWidget::TakeWidget`, `UMG/Private/Components/Widget.cpp:976`); the `[Widget.cpp(976)]` is the widget's provenance, not the log site. It fires at `SetWidgetToFocus` time, i.e. inside `BP_MenuGameMode`'s BeginPlay `SetInputMode_UIOnlyEx(WidgetToFocus = WBP_MainMenu)`, once per menu boot, in every non-Shipping/Test build. `UUserWidget` is not focusable by default and `WBP_MainMenu` never sets it. ⇒ Pre-existing, one line per `L_MainMenu` boot, exactly as the host measured. Not introduced by this diff; this test is the first to run PIE on the map inside an automation capture window.

### Shape chosen: **(A)** — `AddExpectedError` in the test, occurrence count pinned to 1
Why (A) and not (B):
- (B) has two lawful seats and both are outside the row. The clean fix is on the asset side — either `WBP_MainMenu` `bIsFocusable = true` (a `WBP_MainMenu.uasset` save, and it changes what Slate treats as a focusable ancestor of the seven buttons — a behaviour change to the very focus ring this row ships) or dropping / retargeting `WidgetToFocus` in `BP_MenuGameMode`'s BeginPlay (a `BP_MenuGameMode.uasset` save — acceptance (5) allows ONE named saved asset, but the row's names list does not name it, and I would be editing a Blueprint graph nobody has reviewed under this row's gate). A code-only (B) does not exist: the subsystem runs `OnWorldBeginPlay` BEFORE the game mode's BeginPlay (proved in §1) and the widget it would need to make focusable is created INSIDE that BeginPlay, one node before the `SetInputMode` call; the only code seat is mutating the `WBP_MainMenu_C` CDO's `bIsFocusable` at world start, a hack that also changes the shipped menu's focus tree. ⇒ (A) is the only shape that leaves every fenced asset at diff 0 and changes no behaviour.
- (A) is honest about scope: the expectation is pinned to EXACTLY 1 occurrence (`ExpectedNumberOfOccurrences > 0` ⇒ the test fails on 0 or 2+ — `AutomationTest.cpp:1826`), so the moment (B) lands in the Blueprint this test goes RED and the line has to be removed with it. The test therefore documents the defect until it is fixed instead of hiding it (`SetSuppressLogErrors` / a negative count would hide it; not used).

### The diff — ONE file, `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp`
1. **Lines 117-130 (new), inside `RunTest`, BEFORE `AutomationOpenMap`** — a 12-line comment block (cause, source line, the pin, the removal rule) and:
```cpp
	AddExpectedError(TEXT("InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget"),
		EAutomationExpectedErrorFlags::Contains, /*Occurrences*/ 1);
```
   Pattern = the narrowest string that names only this message: the mode tag, the verb phrase and the widget type; the `[Widget.cpp(976)]` creation-site suffix is deliberately left out (an engine-version line number is not this test's claim). No regex metacharacters in it (`:`, `-`, spaces, letters), so regex-vs-plain makes no difference; `Contains` (the default) because the framework compares the message BODY — `FAutomationTestMessageFilter::Serialize` passes `FString(V)` (`AutomationTest.cpp:294`) and the `UE_LOGF` record path passes `Record.FormatMessageTo(Line)` (`:316-318`) — so the `LogPlayerController:` category the host quoted from the controller's re-log is NOT part of the compared string and must not be in the pattern.
2. **Line 90 — NIT-2 from `TASK-1280`:** the `CountDeckBuildersInViewport` doc comment now says it returns a count ("the test asserts == 1") instead of "True when at least one…". Comment only.

No other file touched: `SiegeMenuInputSubsystem.h/.cpp` untouched (NIT-1's comment fix waits for the next touch of those files, as QA worded it); the four input assets untouched; `IMC_Hero` / `KBD-§` / `WBP_MainMenu` / `BP_MenuGameMode` / `L_MainMenu` untouched. ⛔ No asset saved, no editor call, no compile, no Live Coding, nothing staged.

### Header lines read (UE 5.8, not memory)
| what | file:line |
|---|---|
| `void AddExpectedError(FString ExpectedPatternString, EAutomationExpectedErrorFlags::MatchType CompareType = Contains, int32 Occurrences = 1, bool IsRegex = true)` + its doc (`> 0` ⇒ exact count or fail; `== 0` ⇒ ≥1; `< 0` ⇒ silently ignored; "If an expected pattern is not encountered, it will cause this test to fail") | `Core/Public/Misc/AutomationTest.h:1844-1854` |
| `namespace EAutomationExpectedMessageFlags { enum MatchType { Exact, Contains }; }` · `namespace EAutomationExpectedErrorFlags = EAutomationExpectedMessageFlags;` (alias) | `AutomationTest.h:154-178` |
| `FAutomationExpectedMessage` ctor — `FRegexPattern(..., ERegexPatternFlags::CaseInsensitive)`, `Exact` wraps `^…$`; `Matches()` = `FRegexMatcher::FindNext()` | `AutomationTest.h:651-661`, `:685-697` |
| `AddExpectedError` impl → `AddExpectedMessage(…, ELogVerbosity::Warning, …)` (inclusive: Warning + Error) | `Core/Private/Misc/AutomationTest.cpp:1971-1975` |
| the compared string = message body (`FString(V)` / `FormatMessageTo`) | `AutomationTest.cpp:294`, `:316-318` |
| `IsExpectedMessage` verbosity fold + `Matches` | `AutomationTest.cpp:2692-2712` |
| `HasMetExpectedMessages` — `ExpectedNumberOfOccurrences > 0 && != Actual` ⇒ fail | `AutomationTest.cpp:1808-1865` |
| the error site `FInputModeUIOnly::SetWidgetToFocus` | `Engine/Private/PlayerController.cpp:6339-6349` |
| the widget's `ToString()` provenance `[Widget.cpp(976)]` = `UWidget::TakeWidget` `SNew(SObjectWidget, …)` | `UMG/Private/Components/Widget.cpp:962-980` |

### (B) is OWED as a follow-up row — a claim for the manager (`SC-§101`)
`BP_MenuGameMode` asks `SetInputMode_UIOnlyEx` to focus a widget that cannot take focus, on every menu boot, in every Development/Debug build (Shipping/Test compile the check out, so players never see the line — a developer-log defect, cosmetic at runtime because `SetFocusAndLocking` then focuses nothing and this row's subsystem places the real focus on `Button_0` a tick later). Durable fix = an asset edit outside this row's names: either clear/retarget `WidgetToFocus` in `BP_MenuGameMode`'s BeginPlay, or make `WBP_MainMenu` focusable (with the focus-tree consequence named above). Whoever takes it MUST delete the `AddExpectedError` line in `SiegeMenuInputTest.cpp` in the same diff (the pinned count of 1 makes the test red otherwise — by design). The Aura-plugin `401` race (Red A) is NOT mine and I did not touch it.

### Re-entry for the host
`TASK-1281` re-enters at cl. (2): graceful close of the GUI editor (PID 10128, on the previous new binaries), compile, suite expected 556 started / 556 green modulo the Red A race, relaunch. The test's own line should now read `Result={Success}`; if it instead reports `Expected error … matching 'InputMode:UIOnly …' to occur 1 times but it occurred N times`, N is the new fact to bring back (a count other than 1 means the menu booted more or fewer times inside the window than measured — do not loosen the pin from the host seat).
