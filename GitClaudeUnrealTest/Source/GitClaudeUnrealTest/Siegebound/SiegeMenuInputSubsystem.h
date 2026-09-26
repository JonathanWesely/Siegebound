// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
// FTimerHandle lives in its own header (Engine/TimerHandle.h), NOT in CoreMinimal.h and NOT in
// EngineTypes.h -- the same note SiegeKeyboardLayoutSubsystem.h:13-14 carries. Needed BY VALUE for
// TASK-1400's re-entry poll handle below.
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
// TASK-1409: `UInterface` for the close-handler interface below. ⛔ NOT in `CoreMinimal.h` — the
// project's other two interface headers (`HealthBarProvider.h:6`, `TeamId.h:6`) include it
// explicitly for the same reason.
#include "UObject/Interface.h"
#include "SiegeMenuInputSubsystem.generated.h"

class UButton;
class UCheckBox;
// TASK-1429: the in-match arm remembers WHICH input component it bound its handlers to, so a
// second arm in the same world cannot double-bind them. A forward declaration is all a
// `TWeakObjectPtr` member needs (the same way `TWeakObjectPtr<UUserWidget>` gets by below).
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class USlider;
class UUserWidget;
class UWidget;
// TASK-1474: `CollectNavStopsFromTree` takes the tree BY POINTER, so a forward declaration is all
// this header needs — `Blueprint/WidgetTree.h` is already included by the .cpp, which is the only
// translation unit that dereferences it.
class UWidgetTree;

/**
 *  The main-menu input log category — the standing `LogSiege<Domain>` law (CONVENTIONS
 *  "Logging (C++)"). Declared here, defined in `SiegeMenuInputSubsystem.cpp`.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMenuInput, Log, All);

/**
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *  TASK-1409 (3) — THE CLOSE HANDLER. ⛔ BACK NEVER CALLS `RemoveFromParent` FROM HERE.
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *
 *  A screen that wants `IA_MenuBack` to close it implements this interface; the subsystem
 *  ASKS it to close and the screen runs its OWN teardown. ⛔ The alternative — a subsystem
 *  that just removes the widget — is measurably wrong on a screen this project already
 *  ships: `USiegeGraphicsMenuWidget::BackPressed` calls `DisarmVideoModeCountdown()` and
 *  `DiscardStagedVideoMode(...)` BEFORE `RemoveFromParent()`, and its own comment says a
 *  staged-but-unconfirmed video mode that outlives the panel "makes the facade refuse EVERY
 *  SAVE **and** AUTO-DETECT for the rest of the session, silently". A generic remove skips
 *  both calls and leaks exactly that state.
 *
 *  ⭐ WHY AN INTERFACE AND NOT A DELEGATE STORED ON THE REGISTRATION STACK (the row offered
 *  either): a delegate has to live somewhere, and the only place is `NavTargetStack`, which
 *  would have to become an array of a new `USTRUCT` holding a script-delegate `UPROPERTY` —
 *  i.e. TASK-1406's shipped stack, its weak-pointer contract and all five of its lambdas
 *  rewritten, to carry a pointer-to-function. The interface adds NO member, touches NO line
 *  of TASK-1406's stack, and reaches Blueprints the same way (a WidgetBlueprint implements it
 *  from Class Settings → Interfaces and gets the event in its graph), which was the whole
 *  reason the registration API is `BlueprintCallable` in the first place.
 *
 *  ⛔ IMPLEMENTING THIS IS NOT THIS ROW'S WORK. On the day TASK-1409 lands NO screen
 *  implements it, so `IA_MenuBack` is INERT EVERYWHERE — by design ((4): never guess a
 *  teardown) and logged once per press naming the target. TASK-1454 wires the screens.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class USiegeMenuNavCloseTarget : public UInterface
{
	GENERATED_BODY()
};

class ISiegeMenuNavCloseTarget
{
	GENERATED_BODY()

public:

	/**
	 *  `IA_MenuBack` was pressed while THIS screen owned menu navigation. Close yourself,
	 *  your own way: disarm what you armed, discard what you staged, THEN remove yourself.
	 *  ⚠️ Pair it with `UnregisterMenuNavTarget(self)` inside your own teardown — the
	 *  subsystem deliberately does not unregister on your behalf, because a screen that
	 *  declines to close (an unsaved-changes prompt, say) must keep the ring.
	 *
	 *  A `BlueprintNativeEvent`, so a C++ screen overrides `_Implementation` (typically one
	 *  line: `BackPressed();`) and a Blueprint screen implements the event in its graph.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Siegebound|Menu Input")
	void OnMenuNavBackRequested();
};

/**
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *  MAIN-MENU INPUT ACTIONS (TASK-1274, MENU-INPUT-ACTIONS; law `VER-§5`, `KBD-§4`, `SC-§104`)
 *  `USiegeMenuInputSubsystem` — `IMC_MainMenu` applied on `L_MainMenu` ONLY, Up/Down moving
 *  Slate focus across the `WBP_MainMenu` buttons (wrapping), Accept firing the focused
 *  button's `OnClicked` — the SAME delegate a mouse click fires.
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *
 *  WHY THIS CLASS EXISTS, IN ONE SENTENCE: five of seven main-menu flows (deck builder,
 *  sandbox, settings, login, multiplayer) are verifier-blind (`qa/TASK-671-verify.md`: eight
 *  `simulate_key_press Tab` each `applied_mapping_contexts: []`, every button `focused: false`;
 *  `qa/TASK-787-verify.md`: three `ui_perform` click shapes all `down handled=true / up
 *  handled=false`), while `inject_input_action` reached OUR mappings on every leg that used it
 *  (`IA_Move`, `IA_Card1`, `IA_DiscardAll`). A menu mapping context is the cheapest door, and
 *  it doubles as keyboard/gamepad menu navigation for players.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ THE MEASURED CLASSES ON `L_MainMenu`, AND WHY THIS IS A `UWorldSubsystem`
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  Read live from the editor (TASK-1274 handoff §1): `L_MainMenu`'s game mode is
 *  `/Game/Blueprints/BP_MenuGameMode.BP_MenuGameMode_C` (parent `GameModeBase`), and its CDO
 *  reads `PlayerControllerClass = /Script/Engine.PlayerController`,
 *  `DefaultPawnClass = /Script/Engine.DefaultPawn` (the `DefaultPawn_0` N3 saw),
 *  `HUDClass = /Script/Engine.HUD`. ⇒ NO project C++ class runs on the menu map. Giving the
 *  menu a project controller would be a class assignment INSIDE `BP_MenuGameMode.uasset`
 *  (a saved Blueprint edit the row does not list), so the code-side path is taken: a world
 *  subsystem that gates itself on the map name at `OnWorldBeginPlay` and touches NO asset
 *  (acceptance (5): `L_MainMenu` is not dirtied, and neither is `BP_MenuGameMode`).
 *
 *  ⚠️ ORDERING, MEASURED AT THE ENGINE SOURCE (5.8):
 *    • `UGameInstance::StartPlayInEditorGameInstance` (`GameInstance.cpp:538`) and
 *      `UEngine::LoadMap` (`UnrealEngine.cpp:16647`) both call `SpawnPlayActor` — which
 *      creates the `APlayerController`, sets its player and runs `SetupInputComponent` —
 *      BEFORE `World->BeginPlay()` (`:566` / `:16661`). ⇒ At `OnWorldBeginPlay` the local
 *      controller and its `UEnhancedInputComponent` (`DefaultInput.ini:82`) already exist.
 *    • `UWorld::BeginPlay` (`World.cpp`) runs `OnWorldBeginPlay` on every subsystem BEFORE
 *      `GameMode->StartPlay()` — i.e. before `BP_MenuGameMode`'s BeginPlay creates
 *      `WBP_MainMenu`. ⇒ The mapping context and the bindings are installed here; the widget
 *      is resolved LAZILY on every input (never cached), and the initial focus is placed on
 *      the next tick, once the Blueprint's Construct has built the buttons.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⛔ WHAT THIS CLASS NEVER DOES (the row's fences)
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *    • ⛔ No button's `OnClicked` body changes; ⛔ `WBP_MainMenu`'s Construct is not rewritten.
 *      Accept calls `UButton::OnClicked.Broadcast()` — `UButton::SlateHandleClicked`
 *      (`Button.cpp:278-280`) is exactly `OnClicked.Broadcast()`, so every existing Blueprint
 *      handler runs unchanged and a mouse click is unaffected (acceptance (2)).
 *    • ⛔ No letters (`KBD-§4`): Up / Down / Enter / gamepad D-pad / FaceButtonBottom only, so
 *      `USiegeKeyboardLayoutSubsystem`'s letter remap is never entered and `IMC_MainMenu` is
 *      applied VERBATIM (no `GetPositionalContext` call, deliberately — there is nothing to
 *      retarget). ⛔ Tab is untouched (Slate's own).
 *    • ⛔ `IMC_Hero` / `IMC_Default` / `IMC_MouseLook` are never referenced.
 *    • ⛔ No `MapKey` / `UnmapKey` in `Source/` (`KBD-§2`): the four assets are authored in
 *      the editor; this class only LOADS them.
 *    • ⛔ Nothing here runs on any map but `L_MainMenu` (`MenuMapName`), and nothing runs in
 *      an Editor world (`DoesSupportWorldType` = Game | PIE only).
 *      ⚠️ TASK-1429 QUALIFIED THAT SECOND FENCE RATHER THAN DELETING IT, AND A READER MUST NOT
 *      TAKE IT AT FACE VALUE ANY MORE. It is still exactly true of `OnWorldBeginPlay`: on any
 *      map but `L_MainMenu` that function returns before loading, binding or applying anything,
 *      and that return is UNCHANGED. What TASK-1429 added is a SECOND, LATER door that only a
 *      registered in-match screen can open — see "⭐⭐ THE IN-MATCH ARM" below. With nothing
 *      registered, an off-menu map is byte-for-byte what it always was.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️ THE HONEST SIDE — WHICH KEYS REACH THIS CLASS, AND WHICH DO NOT
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  `BP_MenuGameMode` sets `FInputModeUIOnly` at boot (CONVENTIONS "Input-mode ownership").
 *  `FInputModeUIOnly::ApplyInputMode` (`PlayerController.cpp`) calls
 *  `GameViewportClient.SetIgnoreInput(true)`, and `UGameViewportClient::InputKey` returns
 *  early on `IgnoreInput()` ⇒ a REAL keyboard/gamepad press never reaches the player
 *  controller on this map, so it never reaches Enhanced Input, so it never reaches these
 *  handlers. What DOES reach them: `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction`
 *  (Aura's `inject_input_action`, and the automation test) — that is the door the row names.
 *  For a REAL key the player benefit comes from the INITIAL FOCUS this class places on the
 *  first button: with a focused `SButton`, Slate's own navigation config maps the arrow keys
 *  to focus moves and Enter / gamepad Accept to `SButton::OnKeyDown`'s Accept path
 *  (`SButton.cpp:296`). That Slate-side route is a consequence, not a claim this class makes;
 *  it is unmeasured here (no real-input lane exists for an agent) and is named for Jonathan.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  THE VISIBLE FOCUS STATE (deliverable (3))
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  Focus is set with `FSlateApplication::SetUserFocus(UserIndex, Widget, EFocusCause::Navigation)`.
 *  `FSlateApplication::SetUserFocus` computes `ShowFocus = (InCause == EFocusCause::Navigation)`
 *  (`SlateApplication.cpp:3099`) and `SWidget::Paint` draws `GetFocusBrush()` when
 *  `ShowUserFocus` is true (`SWidget.cpp:1748-1751`). `SWidget::GetFocusBrush` returns
 *  `FAppStyle::Get().GetBrush("FocusRectangle")` (`SWidget.cpp:1012-1014`), which in a game
 *  resolves to `FCoreStyle`'s `"Old/DashedBorder"` border brush, margin 6/32, white at 50 %
 *  alpha (`CoreStyle.cpp:305`). ⇒ THE FOCUSED BUTTON WEARS SLATE'S DEFAULT DASHED FOCUS
 *  RECTANGLE. No button style is edited; `SButton::OnFocusReceived` changes no brush
 *  (`SButton.cpp`), so the hover tint stays the mouse's alone.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  THE NAVIGATION MODEL (the row's test, `SC-§104` — STATE, never a tally)
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  The menu's buttons are collected from the live `WBP_MainMenu` widget tree in tree order
 *  (`Overlay_19/VerticalBox_0/Button_0..6`: "Play (vs Bot)", "Sandbox (No Bot)",
 *  "Deck Builder", "Multiplayer", "Settings", "Login", "Quit" — measured in
 *  `qa/TASK-671-verify.md`). The focused index is READ from Slate on every input (never
 *  cached); "cold" (no menu button focused) reads as index 0, the top. Down moves +1, Up
 *  moves −1, both wrapping ⇒ from cold, Down ×2 focuses index 2 = "Deck Builder", and Accept
 *  there broadcasts the button that opens `UDeckBuilderWidget` — the row's test.
 *
 *  ⛔ INERT WHILE A PANEL IS OPEN: the deck builder / settings / login / session panels are
 *  added ON TOP of `WBP_MainMenu` (they never remove it), so if any OTHER visible top-level
 *  user widget is in the viewport the menu is covered and Up / Down / Accept do nothing.
 *  Without that gate, Accept after "Deck Builder" would broadcast the same button again and
 *  open a SECOND builder.
 *  ⚠️ TASK-1406 QUALIFIED THAT PARAGRAPH RATHER THAN DELETING IT: it is still exactly true of the
 *  DEFAULT target (nothing registered). What changed is that "covered" is no longer the ONLY
 *  question the handlers can ask — see "THE ACTIVE NAV TARGET" below.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE ACTIVE NAV TARGET AND THE FOCUS-STOP VOCABULARY (TASK-1406; fences (b) and (c))
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  TASK-1398 §3 F2 measured THREE independent fences keeping `IA_Menu*` off every screen but the
 *  main menu. This class owns two of them; the third (the `L_MainMenu` map gate at the top of
 *  `OnWorldBeginPlay`) is ⛔ NOT touched here — it is TASK-1429's, with a collision census this
 *  row does not have.
 *
 *  ⛔ FENCE (b) — THE QUESTION CHANGED. The handlers used to ask "is the main menu uncovered?",
 *  which goes false the instant ANY other visible top-level widget exists ⇒ the actions went
 *  INERT BY DESIGN the moment a sub-screen opened. They now ask "which screen owns menu
 *  navigation right now?". A screen answers by calling `RegisterMenuNavTarget(self)` when it
 *  opens and `UnregisterMenuNavTarget(self)` when it closes; the subsystem drives THAT tree.
 *  ⭐ Both entry points are `UFUNCTION(BlueprintCallable)` because `WBP_MainMenu`,
 *  `WBP_DeckBuilder` and `WBP_VictoryScreen` are Blueprints and a C++-only API is unreachable
 *  from their graphs (TASK-1398 §4.2).
 *
 *  ⭐ THE REGRESSION CONTRACT, AND IT IS ONE `if`: with NOTHING registered,
 *  `GetRegisteredNavTarget()` is null, `GetActiveNavTarget()` IS `FindMainMenuWidget()` and
 *  `IsNavTargetActionable()` IS `IsMenuUncovered()` — the same two calls the shipped code made,
 *  with their bodies untouched. The default path is byte-for-byte what it was.
 *
 *  ⛔ FENCE (c) — THE VOCABULARY. `GetMenuButtons()` collects `Cast<UButton>` and nothing else.
 *  Pointing a `UButton`-only walker at the Settings tree would collect the Back and Graphics
 *  buttons while SKIPPING every `UCheckBox` and the `USlider` — it would look implemented and
 *  ship 🧑 his exact complaint. `GetMenuFocusStops()` therefore admits FOUR classes —
 *  `UButton`, `UCheckBox`, `USlider`, `UEditableTextBox` (the measured control types,
 *  TASK-1398 §3 F4) — and it HONOURS an authored `IsFocusable == false` as an opt-out rather
 *  than stomping it (deliberate in this project at `SiegeControlsHelpWidget.cpp:177`).
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ LEFT / RIGHT / BACK, AND WHAT THEY MEAN PER CONTROL TYPE (TASK-1409)
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  TASK-1406 gave the ring FOUR classes to land on; reaching a slider it cannot move is 🧑 his
 *  complaint wearing an implementation's clothes. `IA_MenuLeft` / `IA_MenuRight` therefore carry
 *  a semantic PER FOCUS-STOP TYPE, and `IA_MenuBack` carries none of its own at all:
 *
 *    • `USlider`          — step the value by the control's OWN `GetStepSize()`, clamped to
 *                           `[GetMinValue(), GetMaxValue()]`. ⛔ The step is NOT invented: the
 *                           shipped graphics sliders already declare one (`1/4` on the eleven
 *                           0..1 quality sliders = one detent per press; `1.0` on the 50..100
 *                           resolution-scale slider = one percent). A slider that declares
 *                           `StepSize <= 0` gets `DefaultSliderStepFraction` of its range.
 *                           ⛔ AND THEN THE COMMIT EDGE — see asymmetry (ii) below; moving the
 *                           handle is NOT changing the setting on this project's sliders.
 *    • `UCheckBox`        — Left sets OFF, Right sets ON (never a toggle: a direction key that
 *                           inverts state means holding Right flickers). Accept toggles.
 *    • a stepper `UButton` PAIR — Left presses the "<" member, Right the ">" member. The pair
 *                           is `USiegeGraphicsMenuWidget::BuildStepperRow`'s shape and it is
 *                           recognised by the two things that row authors: the names
 *                           `<Base>PrevButton` / `<Base>NextButton`, or the glyphs "<" / ">".
 *    • a plain `UButton`  — Left/Right do NOTHING, and the nothing is DELIBERATE and logged
 *                           (Verbose), never a silent swallow.
 *    • anything else      — ONE line naming the stop and its class. Same rule for Accept
 *                           (TASK-1409 (4b), from `qa/TASK-1407.md` WARN-6).
 *
 *  🚨 TWO ASYMMETRIES A READER MUST KNOW, AND THE SECOND ONE IS THE ONE THAT BITES.
 *
 *  (i) THE ENGINE'S, measured in the engine and not assumed: `USlider::SetValue` CALLS
 *  `HandleOnValueChanged` and therefore ALREADY BROADCASTS `OnValueChanged` (`Slider.cpp`,
 *  `USlider::SetValue` → `USlider::HandleOnValueChanged`), while `UCheckBox::SetIsChecked` does
 *  NOT broadcast `OnCheckStateChanged` at all (`CheckBox.cpp`, `UCheckBox::SetIsChecked` — only
 *  `BroadcastFieldValueChanged` / `BroadcastEnumPostStateChange`). ⇒ the slider path must NOT
 *  re-broadcast `OnValueChanged` and the check-box path MUST broadcast `OnCheckStateChanged`.
 *  This project measured the same asymmetry independently at `SiegeGraphicsMenuWidget.cpp`'s
 *  `SeedAndBind()`, which unbinds before it seeds for exactly this reason.
 *
 *  (ii) 🚨 THIS PROJECT'S, and getting (i) right is NOT enough to get this right (it is the
 *  BLOCKER `qa/TASK-1410.md` raised against the first cut of this row): every `USlider` here is
 *  TWO-PHASE. `OnValueChanged` is bound to a LABEL-ONLY handler that is FORBIDDEN to write (a
 *  drag fires it once per frame ⇒ up to fifty ini writes), and the facade write hangs off
 *  `OnMouseCaptureEnd` + `OnControllerCaptureEnd`, which Slate raises only from a real mouse
 *  release or controller-lock release. A programmatic `SetValue` CANNOT reach them. ⇒ the slider
 *  path must ALSO fire the COMMIT edge (`StepSlider` broadcasts `OnControllerCaptureEnd`, one
 *  edge, only on a real change) or the handle and the label move while the setting does not —
 *  and the next `SeedAndBind` silently reverts it. ⛔ "The engine already broadcasts, so I must
 *  not" is true of `OnValueChanged` ONLY; it is not true of the signal that WRITES.
 *
 *  ⛔ `IA_MenuBack` asks the active target to close ITSELF (`ISiegeMenuNavCloseTarget` above).
 *  It removes nothing, guesses nothing, and is inert on a screen that implements no handler.
 *
 *  ⛔ `Escape` IS NOT BOUND AND MUST NOT BE (🧑 his ruling — the card grid only, "nowhere else in
 *  this project" — and it is the editor's PIE-stop key). Back is `Backspace` +
 *  `Gamepad_FaceButton_Right`, authored in `IMC_MainMenu` by TASK-1408.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  🚨⭐⭐ THE IN-MATCH ARM (TASK-1429) — THE MAP TEST BECOMES A **DEMAND** TEST
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  🚨 A MATCH WHERE THE ARROW KEYS BELONG TO A MENU IS A REGRESSION, NOT A FEATURE. Everything
 *  in this section exists to make that sentence structurally true rather than hoped for.
 *
 *  ⛔ THE EXACT PREDICATE THAT SEPARATES "AN IN-MATCH SCREEN IS OPEN" FROM "THE PLAYER IS
 *  PLAYING", stated once and implemented once, in `IsInMatchScreenOpen()`:
 *
 *        GetRegisteredNavTarget() != nullptr
 *
 *  i.e. at least one entry of `NavTargetStack` is a LIVE `UUserWidget` that `IsInViewport()`
 *  AND `IsVisible()`. ⭐ IT IS A DEMAND TEST, NOT A SURVEY: a screen is "open" here only because
 *  it CALLED `RegisterMenuNavTarget(self)`. Nothing can make it true by merely existing, being
 *  drawn, or being on top — which is the whole reason it answers (6) with no HUD special case.
 *
 *  ⛔ THE DEFAULT STATE IN A MATCH WITH NOTHING OPEN IS BYTE-FOR-BYTE WHAT IT WAS, AND THE PROOF
 *  IS THE SIZE OF THE DIFF, NOT A PARAGRAPH: `OnWorldBeginPlay`'s `if (MapName != MenuMapName)
 *  { return; }` IS UNTOUCHED. On `L_Arena` this subsystem still loads no asset, binds no action,
 *  applies no context and arms no timer at level boot. The arm is reached from exactly ONE
 *  rising edge — `RegisterMenuNavTarget` — and a match in which no screen ever registers never
 *  reaches it.
 *
 *  ⛔ AND IT IS **REMOVED**, NOT IGNORED, on the falling edge: `DisarmInMatchMenuVocabulary`
 *  calls `RemoveMappingContext`. Leaving the context applied and merely declining in the
 *  handlers would still hand the keys to a menu, and a consumed key is a regression whatever
 *  the handler then decides to do with it.
 *
 *  ─── THE KEY-COLLISION CENSUS (TASK-1429 (2); `HELP-§4`, the FLAG-never-stomp law) ──────────
 *  Read live off the assets 2026-09-25, not inferred from source. `IMC_MainMenu` = 12 mappings;
 *  `IMC_Hero` = 28. Every mapping context in the project was checked, not just the hero's:
 *  `IMC_Default` and `IMC_MouseLook` exist, but their ONLY asset referencers are the UE-template
 *  controllers (`BP_ThirdPersonPlayerController`, `BP_CombatPlayerController`,
 *  `BP_PlatformingPlayerController`), and `ASiegePlayerController` derives from
 *  `APlayerController`, not from the template controller ⇒ the only context live in a Siegebound
 *  match is `IMC_Hero`, applied at priority 1 by `BP_HeroCharacter` / `BP_SiegeGhostPawn`.
 *
 *    SHADOWED: **`Enter`** — `IMC_MainMenu` → `IA_MenuAccept`, `IMC_Hero` → `IA_AssistantConsole`.
 *              Both actions carry `bConsumeInput = true` (the engine default, `InputAction.h:96`).
 *    CLEAN:    `Up` · `Down` · `Left` · `Right` · `Backspace` · `Gamepad_DPad_Up` /
 *              `_Down` / `_Left` / `_Right` · `Gamepad_FaceButton_Bottom` ·
 *              `Gamepad_FaceButton_Right` — eleven of twelve, mapped by NO context live in a match.
 *    ⚠️ EXPIRY, PRINTED ON IT: `IMC_Default` maps `Up`/`Down`/`Left`/`Right` to `IA_Move` and
 *              `Gamepad_FaceButton_Bottom` to `IA_Jump`. If a future row ever puts a template
 *              controller in the arena, this census is wrong by five keys, not by one.
 *
 *  ─── (3) THE PRIORITY, AND WHY IT IS **BELOW** THE HERO AND NOT ABOVE ───────────────────────
 *  ⛔ THE ROW ASKED FOR ABOVE. THE CENSUS IS WHY IT SHIPPED BELOW, AND THE ARGUMENT IS NOT A
 *  PREFERENCE. Contexts are walked in DESCENDING priority
 *  (`EnhancedInputSubsystemInterface.cpp:947`); a mapping is added only when no higher-priority
 *  context already claimed its key (`:1041`); a claimed key is recorded when the action has
 *  `bConsumeInput` (`:1054`). So the higher-priority mapping does not merely WIN — the lower one
 *  is NEVER BUILT. ⇒ For the eleven keys nobody else maps, the priority number is a behavioural
 *  NO-OP: above or below, the menu mapping is added either way. For the twelfth, "above" IS
 *  precisely and only the act of taking `Enter` away from the assistant console — and (2) makes
 *  that 🧑 HIS call, not this row's. `InMatchMenuMappingContextPriority = 0` therefore sits below
 *  `AHeroCharacter`'s `HeroMappingContextPriority = 1` and stomps NOTHING. Flipping it is a
 *  one-line change to that constant; the letter asking for the ruling is in the handoff.
 *
 *  ⭐ AND BELOW BUYS TWO SAFETY PROPERTIES ABOVE CANNOT:
 *    (i)  THE WORST CASE OF A LEAKED ARM IS BOUNDED. If the context were ever left applied with
 *         nothing registered, it would still take no key from the player — because it claims no
 *         key `IMC_Hero` claims — and the handlers would decline anyway (`IsNavTargetActionable()`
 *         is false in-match with nothing registered: `FindMainMenuWidget()` is null and
 *         `IsMenuUncovered()` returns false at the first visible non-menu top-level widget, i.e.
 *         `WBP_HUD`). The failure mode is log noise, not a stolen match.
 *    (ii) THE DISARM CANNOT RE-TRIGGER A HERO ACTION. `RemoveMappingContext` hands keys back to
 *         lower-priority contexts; at priority 0 this context never held one, so there is nothing
 *         to hand back and no hero action can fire as a side effect of a screen closing.
 *
 *  ─── 🚨 (5) MEASURED, NOT INHERITED: IN-MATCH, ENHANCED INPUT IS **NOT** DEAF ────────────────
 *  `qa/TASK-1424.md` measured the `SetIgnoreInput` chain TRUE — but it measured it on
 *  `L_MainMenu` under `BP_MenuGameMode`, so it NARROWS this question and does not settle it.
 *  Measured here for the in-match game mode instead: `ASiegePlayerController::ApplyCursorInputState`
 *  (`SiegePlayerController.cpp:6457-6475`) is the ONE input-mode owner in a match and applies
 *  either `FInputModeGameAndUI` or `FInputModeGameOnly` — and BOTH call
 *  `GameViewportClient.SetIgnoreInput(**false**)` (`PlayerController.cpp:6410` and `:6449`),
 *  against `FInputModeUIOnly`'s `SetIgnoreInput(true)` at `:6384`. ⇒ IN A MATCH THE VIEWPORT IS
 *  LIVE AND ENHANCED INPUT HEARS REAL KEYS. The (5)(i) branch holds: no input-mode change is
 *  needed and none is proposed.
 *
 *  ⚠️ THE ONE EXCEPTION, AND IT IS THE VICTORY SCREEN: `HandleMatchEnd` applies
 *  `FInputModeUIOnly` (`SiegePlayerController.cpp:2368`) once a victory widget exists ⇒ on the
 *  END SCREEN, and only there, a REAL key is swallowed before Enhanced Input. Injection is
 *  unaffected (`InjectInputForAction` never touches the viewport). That screen's keyboard route
 *  is Slate's, exactly as `SiegePlayerController.cpp:2370-2377` already says.
 *
 *  ⭐⭐ AND THE CONSEQUENCE THE ROW ASKED ABOUT — "if both lanes are live, could one real key
 *  drive both?" — IS **NO**, MEASURED: Slate routes a key down the FOCUS PATH, bubbling from the
 *  focused widget upward (`SlateApplication.cpp:5044-5073`), and `SViewport` — the only door to
 *  `UGameViewportClient::InputKey` and therefore to Enhanced Input — is an ANCESTOR of every UMG
 *  widget in the viewport. `SWidget::OnKeyDown` returns `Handled().SetNavigation(...)` for an
 *  arrow key on any focusable widget (`SlateCore SWidget.cpp:416-429`) and `SButton::OnKeyDown`
 *  handles the Accept keys itself (`SButton.cpp:293-316`). ⇒ WITH A STOP FOCUSED, THE BUBBLE
 *  STOPS AT THE STOP AND NEVER REACHES THE VIEWPORT: Slate navigates, Enhanced Input never sees
 *  the key, and nothing fires twice. WITH NOTHING FOCUSED (ordinary play — `FInputModeGameOnly`
 *  focuses `SViewport`), the viewport IS the focused widget and Enhanced Input DOES see the
 *  arrow keys ⇒ a context applied with nothing open would genuinely hand a live match to a menu.
 *  ⛔ THAT IS WHY THE DEMAND GATE IS LOAD-BEARING AND NOT BELT-AND-BRACES.
 *
 *  ⚠️ ⭐ IT ALSO RESIZES THE `Enter` COLLISION HONESTLY: with a stop focused, `SButton::OnKeyDown`
 *  consumes `Enter` before the viewport at ANY priority, so `IA_AssistantConsole` would not have
 *  fired at that instant either way. The dominant mechanism is TASK-1406's already-shipped FOCUS
 *  PLACEMENT, not this row's mapping context — which is what makes "below" cost so little.
 *
 *  ─── (6) THE HUD AND THE FPS COUNTER, CONFIRMED IN A MATCH-SHAPED CASE, NO SPECIAL CASE ──────
 *  Both are always-visible top-level widgets (ZOrder 0 and 30) that would have made the OLD
 *  coverage gate false forever. TASK-1406's active-target model handles them because they never
 *  REGISTER: `IsNavTargetActionable()` short-circuits on `GetRegisteredNavTarget() != nullptr`,
 *  and `GetMenuFocusStops()` walks `GetActiveNavTarget()`'s OWN `WidgetTree` — the registered
 *  screen's, never the viewport's. ⇒ `WBP_HUD`'s `Btn_Jump` (the (6b) rider's measured
 *  `IsFocusable = False` leftover, object path `WBP_HUD:Overlay_19/SizeBox_0/Btn_Jump`) is NOT a
 *  stop that is collected and then refused — it is a stop that is **NEVER COLLECTED**, because no
 *  walk ever enters the HUD's tree. ⛔ The two are indistinguishable in a bare count, which is
 *  why this says which one it is. `WBP_CardHand`'s twin is inert for the same reason, on top of
 *  being `Collapsed`. ⛔ No walker change, no HUD clause: the flag is not moved and not stomped.
 *
 *  ─── THE BACKSTOP, AND WHY IT ONLY EVER DISARMS ──────────────────────────────────────────────
 *  A screen destroyed or removed WITHOUT its paired `UnregisterMenuNavTarget` announces nothing
 *  to subscribe to — the same measured shape TASK-1400's re-entry poll exists for. Left to the
 *  edges alone, a forgotten unregister would leave the vocabulary armed for the rest of the
 *  match. `InMatchDemandPollTimerHandle` therefore runs `PollInMatchDemand()` while, and only
 *  while, the vocabulary is armed. ⭐ IT PASSES `bMayArm = false`, so it is a ONE-WAY VALVE: the
 *  backstop can only ever hand the match BACK to the player and can never take it. In a match
 *  with nothing open the timer does not exist at all.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE SELF-DRIVING SCREEN FLAG (TASK-1471) — A SCREEN THAT DRIVES ITSELF CAN SAY SO
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  🚨 THE HAZARD THIS CLOSES WAS MEASURED IN PIE, NOT REASONED ABOUT (`qa/TASK-1427-verify.md`):
 *  EVERY `IA_MenuDown` pressed in the deck builder moved this subsystem's ring onto stop 1 —
 *  object-path-anchored there as `WBP_DeckBuilder_C_0 → Overlay_19/VerticalBox_0/HorizontalBox_1/
 *  Button_1`, the **"Reset to Default"** button, which WIPES 🧑 the player's saved deck AND
 *  AUTO-SAVES THE WIPE (`qa/TASK-1424.md` WARN-1) — WITHIN THE PRESS, before
 *  `UDeckBuilderWidget::RouteMenuNavKey`'s precondition repair pulled the focus back onto the card
 *  tile. ⛔ That repair works, is proven (the next press read `Current = 0` cold again) and is NOT
 *  removed by this section. What this section removes is the TRANSIT.
 *
 *  ⛔ WHY A SCREEN CANNOT FIX THIS BY ITSELF, WHICH IS WHY THE FLAG IS HERE AND NOT THERE: this
 *  subsystem binds its `IA_Menu*` handlers at `OnWorldBeginPlay`, i.e. BEFORE any widget's
 *  `NativeConstruct`, so its delegate sits EARLIER in the shared `UEnhancedInputComponent`'s
 *  binding array and `MoveFocus(+1)` runs FIRST on the very same press. A screen cannot out-order
 *  that; it can only be ASKED FIRST — which is exactly what the flag is.
 *
 *  ─── THE DECLARATION ────────────────────────────────────────────────────────────────────────
 *  A screen opens with `RegisterSelfDrivingMenuNavTarget(self)` instead of
 *  `RegisterMenuNavTarget(self)`. ⭐ A SEPARATE ENTRY POINT RATHER THAN A PARAMETER ON THE SHIPPED
 *  ONE, and both reasons are mechanical rather than aesthetic: (i) `RegisterMenuNavTarget` is a
 *  `UFUNCTION` that WidgetBlueprint graphs call, and widening its signature moves every existing
 *  node's pin set in an asset this row may not touch; (ii) the declaration MUST be recorded BEFORE
 *  `RegisterMenuNavTarget`'s closing `FocusFirstNavStop()` runs, which a call that records-then-
 *  delegates guarantees by construction and a "set the flag afterwards" setter does not.
 *  ⛔ The declaration lives exactly as long as the registration: `UnregisterMenuNavTarget` clears
 *  it, and nothing else does.
 *
 *  ─── ⛔ WHAT THE SUBSYSTEM **STOPS** DOING while the ACTIVE target has declared it ───────────
 *  Exactly four things, and every one of them is "the generic ring touching this screen's controls":
 *    1. `MoveFocus(±1)`       — Up/Down move no focus on its tree. ⛔ THIS is the measured transit.
 *    2. `HandleMenuAccept()`  — no `OnClicked.Broadcast()`, no check-box toggle. ⛔ THIS is the
 *                               reach onto *"Reset to Default"*, and it is CLOSED, not narrowed:
 *                               the one line that could press that button is not reached at all.
 *    3. `StepFocusedStop(±1)` — Left/Right mutate none of its sliders, check boxes or steppers.
 *    4. `FocusFirstNavStop()` — registration places no ring on its stop 0 (and neither does the
 *                               hand-back inside `UnregisterMenuNavTarget`, which routes here too).
 *  Each refusal emits ONE line naming the screen and its class. ⛔ NO EXISTING LOG STRING IS
 *  REWORDED — TASK-1395 / TASK-1402 read four of them as grep discriminators.
 *
 *  ─── ⭐ WHAT IT **KEEPS** DOING, AND THIS HALF IS THE WHOLE POINT ───────────────────────────
 *  🚨 A flag that turned the ring fully off would make the screen UNREACHABLE again, which is the
 *  defect this milestone exists to remove. The declared screen therefore stays ENUMERABLE:
 *    • it stays on `NavTargetStack`, so `GetActiveNavTarget()` still names it and the stack still
 *      nests (a panel opened on top of it is an ordinary, undeclared target and the ring drives it);
 *    • `GetMenuFocusStops()` and `IsNavFocusStop()` are ⛔ NOT TOUCHED, so `LogNavTargetRetarget`
 *      still prints THIS SCREEN'S focus-stop count — the binding instrument of the board's
 *      `EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24` ruling and the only way that screen gets a
 *      focus-stop-count line on any lane. ⛔ Un-registering the screen would have "fixed" the
 *      transit by deleting the measurement; this deletes neither.
 *    • `IsNavTargetActionable()` is unchanged, so the registration still ANSWERS the coverage
 *      question and the screens underneath are unaffected;
 *    • `IA_MenuBack` is unchanged. It drives nothing and activates nothing — it ASKS the active
 *      target to close ITSELF, which is already the screen's own decision — and gating it would
 *      suppress the single diagnostic line TASK-1454 needs;
 *    • 🚨 THE IN-MATCH ARM IS UNCHANGED, AND THAT IS LOAD-BEARING RATHER THAN INCIDENTAL.
 *      `IsInMatchScreenOpen()` is still `GetRegisteredNavTarget() != nullptr`, and a declared
 *      screen is still registered ⇒ it still arms and disarms `IMC_MainMenu` on its two edges. A
 *      self-driving screen listens to the SAME six `IA_Menu*` assets on the SAME input component,
 *      so a flag that suppressed the arm would kill that screen's OWN keys off the menu map.
 *
 *  ─── ⛔ DEFAULT OFF, AND THE DEFAULT IS THE WHOLE REGRESSION CONTRACT ───────────────────────
 *  `SelfDrivingScreens` is empty until a screen calls the new entry point;
 *  `GetSelfDrivingActiveTarget()` returns null on an empty array; every gate is
 *  `if (null) → fall through`. ⇒ the five other registered screens (Settings, Graphics, Login,
 *  Session, Controls Help) and the DEFAULT `WBP_MainMenu` target reach byte-identical code, and the
 *  focus-stop COUNT of all six is unchanged because the walker is not edited.
 *
 *  ⚠️ THE ONE ORDERING FACT A READER MUST NOT GET BACKWARDS: the gate reads
 *  `GetRegisteredNavTarget()`, ⛔ never `GetActiveNavTarget()`. So the DEFAULT target can never be
 *  gated by somebody else's declaration, and a declared screen stops gating the instant it stops
 *  being the top LIVE registration. ⭐ It also makes the gate's placement provably harmless: when
 *  `GetSelfDrivingActiveTarget()` is non-null, `GetRegisteredNavTarget()` is non-null, so
 *  `IsNavTargetActionable()` is necessarily TRUE — the new gate can therefore sit BEFORE that one
 *  (where the spec requires it, ahead of the move) without being able to change which branch any
 *  press takes.
 *
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no RPC, no new
 *  relevancy tier — everything here is local-player Slate focus on the menu map.
 */
UCLASS(BlueprintType)
class GITCLAUDEUNREALTEST_API USiegeMenuInputSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/**
	 *  The only map this subsystem arms on AT LEVEL BOOT (short package name, PIE prefix stripped).
	 *  ⚠️ TASK-1429: it is no longer the only map on which the menu vocabulary can be LIVE — an
	 *  off-menu map arms it ON DEMAND while a screen is registered. This name still gates
	 *  `OnWorldBeginPlay` exactly as it always did; see "THE IN-MATCH ARM" in the class comment.
	 */
	static const TCHAR* MenuMapName;

	/** The four authored assets (TASK-1274 deliverable (1)) — loaded, never written. */
	static const TCHAR* MenuMappingContextPath;
	static const TCHAR* MenuUpActionPath;
	static const TCHAR* MenuDownActionPath;
	static const TCHAR* MenuAcceptActionPath;

	/**
	 *  TASK-1409 (1): the three actions TASK-1408 authored (`IMC_MainMenu` 6 → 12 mappings:
	 *  `Left` / `Gamepad_DPad_Left`, `Right` / `Gamepad_DPad_Right`, `Backspace` /
	 *  `Gamepad_FaceButton_Right`). ⛔ DEGRADE OPEN: a missing one of these makes ITS key inert
	 *  with one warning naming it — it never fails the arm for the other three.
	 */
	static const TCHAR* MenuLeftActionPath;
	static const TCHAR* MenuRightActionPath;
	static const TCHAR* MenuBackActionPath;

	/** The live menu widget's generated class path (`/Game/UI/WBP_MainMenu`). */
	static const TCHAR* MainMenuWidgetClassPath;

	/** Priority of `IMC_MainMenu`. Nothing else is applied on `L_MainMenu` (`applied_mapping_contexts: []`, TASK-671), so 0 is unambiguous. */
	static constexpr int32 MenuMappingContextPriority = 0;

	/**
	 *  TASK-1429 (3): the priority `IMC_MainMenu` is applied at IN-MATCH, on demand.
	 *
	 *  ⛔ IT IS A SEPARATE CONSTANT FROM `MenuMappingContextPriority` THAT HAPPENS TO SHARE ITS
	 *  VALUE, AND THAT IS DELIBERATE. The two numbers answer different questions and are
	 *  justified by different facts — the menu-map one by "nothing else is applied there", this
	 *  one by a key-collision census against a context that IS applied — so a future change to
	 *  either must not silently move the other.
	 *
	 *  ⭐ WHY 0, i.e. BELOW `AHeroCharacter::HeroMappingContextPriority` (1), when the row asked
	 *  for ABOVE: the census found exactly ONE collision (`Enter`: `IA_MenuAccept` vs
	 *  `IA_AssistantConsole`). Enhanced Input walks contexts in descending priority and simply
	 *  does not build the lower-priority mapping for a key a higher one already claimed
	 *  (`EnhancedInputSubsystemInterface.cpp:947` / `:1041` / `:1054`), so for the ELEVEN keys no
	 *  live context maps this number is a behavioural no-op, and for the twelfth "above" IS the
	 *  act of taking a key from a shipped in-match feature — a 🧑 for-Jonathan call under
	 *  `HELP-§4`, not an agent's. Full argument, and the two safety properties "below" buys, in
	 *  the class comment's "(3) THE PRIORITY" section.
	 *  ⚠️ IF HE RULES THE OTHER WAY, THIS LINE IS THE WHOLE CHANGE: 0 → 2 (above the hero's 1).
	 */
	static constexpr int32 InMatchMenuMappingContextPriority = 0;

	/**
	 *  TASK-1429: how often the in-match backstop re-asks "is a screen still open?", in seconds.
	 *
	 *  ⛔ A SEPARATE CONSTANT FROM `FocusReentryPollSeconds` FOR THE SAME REASON THE PRIORITY IS
	 *  SEPARATE — same value, different job (that one re-places focus on the menu map; this one
	 *  releases the match back to the player). 0.2 s matches the cadence this project already
	 *  chose for "a transition that announces itself to nobody", and the cost argument is the
	 *  same one: the timer exists ONLY while the vocabulary is armed, so a match in which no
	 *  screen is ever opened never creates it, and each tick is one walk of a stack that holds
	 *  at most a handful of weak pointers.
	 */
	static constexpr float InMatchDemandPollSeconds = 0.2f;

	/**
	 *  TASK-1400 (MENU-REENTRY-FOCUS): how often the re-entry poll re-checks the menu, in seconds.
	 *
	 *  WHY A POLL AND NOT AN EVENT -- the reason is a MEASUREMENT, not a preference: BOTH return
	 *  paths to this menu emit NOTHING to subscribe to. The deck builder's `Exit` is a pure
	 *  Blueprint chain `CreateWidget(WBP_MainMenu_C) -> Is Valid -> AddToViewport(ZOrder 0) ->
	 *  RemoveFromParent(self)` with no focus node and no dispatcher (TASK-1399 §5.3), and
	 *  `USessionMenuWidget::BackPressed` (`SessionMenuWidget.cpp:151-165`) is the SAME shape in
	 *  C++. A Slate `OnFocusChanging` hook would be provably insufficient as well: TASK-1399's
	 *  table read ALL nodes `focused:false` on the Settings / Login / Session panels, so closing
	 *  those changes no focus and would fire no event. A tickable subsystem is this same poll at
	 *  60+ Hz. ⇒ a low-rate look is the only mechanism that covers a transition that announces
	 *  itself to nobody.
	 *
	 *  0.2 s is chosen so the focus is back before a returning player's hand reaches a key, while
	 *  the cost stays negligible: the timer is armed ONLY on `L_MainMenu`, AFTER the map gate, so
	 *  it does not exist on `L_Arena`; and each tick's early-out is `IsMenuUncovered()`, which is
	 *  false for the whole time any sub-screen is open.
	 */
	static constexpr float FocusReentryPollSeconds = 0.2f;

	/**
	 *  TASK-1409 (2): the Left/Right step for a `USlider` that declares NO usable step of its
	 *  own (`GetStepSize() <= 0`), expressed as a FRACTION OF THAT SLIDER'S RANGE.
	 *
	 *  ⛔ IT IS A FALLBACK, NOT THE POLICY. Every slider this project ships declares a real
	 *  `StepSize` (`SiegeGraphicsMenuWidget.cpp`'s `BuildSliderRow` passes `1/MaxQualityLevel`
	 *  for the eleven 0..1 quality sliders and `1.0f` for the 50..100 resolution scale), so on
	 *  today's screens this constant is never reached. It exists so a future slider that left
	 *  `StepSize` at 0 moves at all instead of reading as a dead control.
	 *
	 *  WHY 0.05 AND WHY A FRACTION: a fraction is the only choice that is correct for BOTH a
	 *  0..1 slider and a 50..100 one — an absolute step would be 20 % of one and 2 % of the
	 *  other. 1/20 puts the full range 20 presses apart: coarse enough to cross by hand, fine
	 *  enough to aim. ⛔ Not the engine's own 0.01 default, which would need 100 presses.
	 */
	static constexpr float DefaultSliderStepFraction = 0.05f;

	//~ UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  READ API — for the automation test (`Siegebound.MenuInput.*`) and the verifier.
	//  Every read walks the LIVE widget tree; nothing here is cached state.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  True once `IMC_MainMenu` was applied and the three CORE actions (`IA_MenuUp` /
	 *  `IA_MenuDown` / `IA_MenuAccept`) were bound on this world's local controller.
	 *  ⚠️ TASK-1409 did NOT widen this flag: the three navigation-extension actions
	 *  (`IA_MenuLeft` / `IA_MenuRight` / `IA_MenuBack`) degrade open, so `bArmed` would stop
	 *  meaning "the shipped three work" if a missing new asset could clear it.
	 */
	bool IsArmed() const { return bArmed; }

	/** The live `WBP_MainMenu` instance in the viewport, or null (not on the menu map / not yet constructed). */
	UUserWidget* FindMainMenuWidget() const;

	/** The menu's focusable, visible, enabled buttons in widget-tree order (`Button_0..6`). Empty when the menu is absent. */
	void GetMenuButtons(TArray<UButton*>& OutButtons) const;

	/** The menu button holding the local player's Slate focus, or null (cold / focus elsewhere). */
	UButton* GetFocusedMenuButton() const;

	/** The label text of a menu button (its first `UTextBlock` descendant), or empty. */
	static FString GetButtonLabel(const UButton* Button);

	/** Pure wrap arithmetic: the index Delta steps from Current in a ring of Count. Count <= 0 ⇒ INDEX_NONE. */
	static int32 WrapIndex(int32 Current, int32 Delta, int32 Count);

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1406 — THE ACTIVE NAV TARGET (fence (b)). The Blueprint-reachable registration API.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  Make `Screen` the screen that owns menu navigation: Up / Down / Accept drive ITS tree
	 *  from now on, and the coverage question stops being asked (an explicit registration
	 *  ANSWERS it — the screen that put itself forward is the one the player is looking at).
	 *
	 *  ⚠️ CALL IT WITH `self` FROM THE SCREEN'S OWN OPEN PATH, AND ALWAYS PAIR IT WITH
	 *  `UnregisterMenuNavTarget`. A screen that registers and never unregisters would strand
	 *  the ring on a dead tree; the stack drops entries that were destroyed without
	 *  unregistering, but that is a net, not a licence.
	 *
	 *  Registering the same screen twice MOVES it to the top rather than duplicating it.
	 *  Registrations NEST: Settings → Graphics → Back falls back to Settings, not to the menu.
	 *
	 *  ⭐ It also places the ring on the new target's first focus stop, on exactly the contract
	 *  `ApplyInitialFocus()` uses for the default target (stop 0, and only when nothing on the
	 *  target already holds focus). Without that, a registered screen would open ringless and
	 *  the first Down would skip stop 0 — navigable and still failing 🧑 the ask.
	 *
	 *  🚨 TASK-1429 — WHAT THIS CALL NOW COSTS OFF THE MENU MAP, STATED HERE BECAUSE THIS IS THE
	 *  DECLARATION AN IN-MATCH SCREEN'S AUTHOR READS: on any map but `L_MainMenu` this is THE
	 *  RISING EDGE that applies `IMC_MainMenu` and makes the menu keys live. It is the only way to
	 *  arm them in a match, and it is a promise to call `UnregisterMenuNavTarget(self)` — a
	 *  registered screen that never unregisters would leave the vocabulary armed until the
	 *  backstop poll notices the widget has gone. ⛔ The keys it arms are `Up` / `Down` / `Left` /
	 *  `Right` / `Enter` / `Backspace` and their six gamepad twins, and at the in-match priority
	 *  they take NOTHING from `IMC_Hero` — see `InMatchMenuMappingContextPriority` and the class
	 *  comment's census. ⚠️ `Enter` is the one key both contexts map: in-match it stays with
	 *  `IA_AssistantConsole`, so a screen that wants Enter to activate its focused control gets
	 *  that from Slate's own Accept path (a focused `SButton`), not from `IA_MenuAccept`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Menu Input")
	void RegisterMenuNavTarget(UUserWidget* Screen);

	/**
	 *  TASK-1471 — REGISTER, AND DECLARE THAT THIS SCREEN DRIVES ITS OWN NAVIGATION.
	 *
	 *  Everything `RegisterMenuNavTarget` does — the stack, the in-match arm, the one retarget log
	 *  line WITH THIS SCREEN'S FOCUS-STOP COUNT — happens exactly as it does for any other screen.
	 *  The single difference is that, while this screen owns navigation, the subsystem's GENERIC
	 *  ring does not walk its tree: Up / Down move no focus on it, Accept presses none of its
	 *  buttons and toggles none of its check boxes, Left / Right mutate none of its controls, and
	 *  registration places no ring on its stop 0. The screen's own key handlers are untouched,
	 *  because this subsystem never had anything to do with them.
	 *
	 *  ⛔ CALL IT INSTEAD OF `RegisterMenuNavTarget`, NEVER AS WELL AS IT, and call it with `self`
	 *  from the screen's own open path. It is paired with the SAME `UnregisterMenuNavTarget(self)`
	 *  — there is no second un-declare, because the declaration is dropped with the registration.
	 *
	 *  ⭐ WHEN A SCREEN SHOULD USE THIS, IN ONE TEST: does it bind the `IA_Menu*` actions itself, or
	 *  navigate a structure the walker cannot see? `UDeckBuilderWidget` answers yes twice — it binds
	 *  all six actions on the shared input component, and it navigates a 2-D grid of nested
	 *  `WBP_DeckCardTile` user widgets, which are not one of the four admitted focus-stop classes and
	 *  ~~are not even reached by `UWidgetTree::ForEachWidget`~~ are still not reached by the walker
	 *  after TASK-1474. Settings, Graphics, Login, Session and Controls Help all answer no: the ring
	 *  IS their navigation, and they must keep calling `RegisterMenuNavTarget`.
	 *
	 *  ⛔ TASK-1474 (2026-09-25) — the struck clause is corrected, not deleted (`SC-§120`): the
	 *  CONCLUSION stands and the MECHANISM given for it was wrong twice over. The tiles were never
	 *  excluded by "`ForEachWidget` descends through `UPanelWidget` only" — a `UUserWidget` IS an
	 *  `INamedSlotInterface`, so that limb is entered; they were excluded because no `UNamedSlot`
	 *  exists anywhere in this project. And they are STILL excluded now that the walker descends,
	 *  because `WBP_DeckCardTile` is a Blueprint class and `IsCodeAuthoredSubWidget` refuses it.
	 *  ⇒ ⛔ this screen's reason to be self-driving is UNCHANGED by TASK-1474.
	 *
	 *  🚨 WHY THIS IS NOT "DO NOT REGISTER AT ALL", which is the shortcut it exists to refuse: the
	 *  registration is what makes the screen ENUMERABLE — it is the only path on which
	 *  `LogNavTargetRetarget` prints a focus-stop count for it, the instrument the board's
	 *  `EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24` ruling makes binding — and off the menu map it is
	 *  the only thing that applies `IMC_MainMenu`, i.e. the only reason the screen's OWN keys are
	 *  live at all. This flag keeps both and removes only the walking.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Menu Input")
	void RegisterSelfDrivingMenuNavTarget(UUserWidget* Screen);

	/**
	 *  Give up menu navigation. The target reverts to the next live registered screen, or — when
	 *  the stack empties — to the DEFAULT: `WBP_MainMenu` under the unchanged coverage gate.
	 *  ⛔ It deliberately does NOT re-place focus when the stack empties: TASK-1400's 0.2 s
	 *  re-entry poll owns that case and it is the only path carrying the coverage check, so a
	 *  focus call here could land on the main menu while the closing screen is still drawn.
	 *
	 *  🚨 TASK-1429 — OFF THE MENU MAP THIS IS THE FALLING EDGE, AND IT IS WHAT GIVES THE MATCH ITS
	 *  KEYS BACK: when it leaves no LIVE registered screen behind, `IMC_MainMenu` is REMOVED (not
	 *  merely ignored). ⛔ Call it from your own teardown even on a path that also destroys the
	 *  widget — the backstop poll will catch a forgotten one, but it is a net, not a licence, and
	 *  it runs up to `InMatchDemandPollSeconds` late.
	 *
	 *  ⭐ TASK-1471: it is ALSO where a `RegisterSelfDrivingMenuNavTarget` declaration is dropped, so
	 *  the flag's lifetime is exactly the registration's and a screen can never leave one behind.
	 *  ⛔ There is deliberately no separate un-declare call to forget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Menu Input")
	void UnregisterMenuNavTarget(UUserWidget* Screen);

	/** The widget whose tree the handlers drive right now: the registered screen, else `FindMainMenuWidget()`. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Menu Input")
	UUserWidget* GetActiveNavTarget() const;

	/**
	 *  The active nav target's focus stops, in widget-tree traversal order (fence (c)).
	 *  Four admitted classes: `UButton`, `UCheckBox`, `USlider`, `UEditableTextBox`.
	 *  ⛔ NOT a replacement for `GetMenuButtons()`, which stays exactly as it was — the
	 *  automation test and TASK-1400's `ApplyInitialFocus` guard both read through that one.
	 *
	 *  ⭐ TASK-1474 — IT NOW DESCENDS, BUT ONLY THROUGH ONE KIND OF BOUNDARY. The walk enters the
	 *  own `WidgetTree` of a nested user widget when — and only when — `IsCodeAuthoredSubWidget`
	 *  says that widget's class is NATIVE. Read that predicate's comment for the argument; the
	 *  one-line version is that a C++-authored subtree's stop set moves only in a reviewed diff
	 *  and a Blueprint-authored one moves when somebody drags a button.
	 *  ⛔ WHAT THIS BUYS AND WHAT IT DOES NOT, so nobody has to infer it: it ADDS the deck bar's
	 *  ten `UDeckSlotEntryWidget::SlotButton`s to this screen's COUNT, and it adds ⛔ NOTHING on
	 *  any other registered screen (measured — see `handoffs/TASK-1474-programmer.md`'s table).
	 *  ⛔ IT DOES NOT MAKE THEM RING-WALKABLE: the deck builder is a TASK-1471 self-driving screen,
	 *  so `MoveFocus` declines before it reaches this function there.
	 */
	void GetMenuFocusStops(TArray<UWidget*>& OutStops) const;

	/**
	 *  The focus stop holding the local player's Slate focus, or null.
	 *  ⚠️ TASK-1469 limb 1(a): "holding focus" is `HasUserFocus(PC) || HasUserFocusedDescendants(PC)`,
	 *  in TWO PASSES — exact focus is asked of every stop first. `SEditableTextBox` FORWARDS focus
	 *  to its inner `SEditableText`, so the exact-widget question alone answered `false` on a box
	 *  that was visibly wearing the ring and this function returned null. The two-pass order is not
	 *  style: the focus path contains the focused widget's whole ANCESTOR chain, so an ancestor stop
	 *  also answers the descendant question, and the walk is pre-order (ancestor first).
	 *  ⭐ Its three callers — `MoveFocus`, `HandleMenuAccept`, `StepFocusedStop` — all read through
	 *  here, so all three stop logging `'None' (none focused)` at a ring that is plainly in a field.
	 */
	UWidget* GetFocusedNavStop() const;

	/**
	 *  True when `Widget` is one of the four admitted classes AND is enabled, visible, EFFECTIVELY
	 *  visible (TASK-1469 limb 2 — no Collapsed/Hidden Slate ancestor) and not opted out of focus.
	 *  ⛔ TASK-1469 limb 3: the `Next` member of a recognised stepper pair is NOT a stop — the row
	 *  is one stop, and `StepFocusedStop`/`FindStepperPair` drive both members from it.
	 */
	static bool IsNavFocusStop(const UWidget* Widget);

private:

	/**
	 *  TASK-1469 limb 2 — EFFECTIVE visibility: true when every SLATE ancestor of `Widget` reports
	 *  `EVisibility::IsVisible()`, false when `Widget` has no cached `SWidget` at all.
	 *
	 *  ⭐ IT MIRRORS THE RULE THAT ACTUALLY DECIDES FOCUS, which is why it is a Slate-side walk and
	 *  not a UMG `GetParent()` one: `FSlateApplication::SetUserFocus` succeeds only when
	 *  `FindPathToWidget` reaches the widget through arranged children filtered by
	 *  `EVisibility::Visible`, so a Collapsed ancestor makes the widget unfocusable while its OWN
	 *  `UWidget::IsVisible()` still reads true. Admitting such a stop does not merely waste a press:
	 *  `MoveFocus` re-reads the index from Slate, so the failed request leaves the index unmoved and
	 *  the ring is SEVERED at that stop (measured, `qa/TASK-1413-verify.md` §6).
	 *
	 *  ⛔ REALIZATION IS NOT A SEPARATE TEST. An unrealized widget has no cached `SWidget`, which
	 *  fails here AND already fails `UWidget::IsVisible()`. See `handoffs/TASK-1469-programmer.md`
	 *  for the A/B/C answer and the evidence that settles it.
	 */
	static bool HasVisibleSlateAncestry(const UWidget* Widget);

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1474 — BOUNDED WIDGET-TREE DESCENT. ⛔ Read `GetMenuFocusStops()`'s definition
	//  comment before changing either function below: the descent and the boundary are one
	//  decision, and the boundary is the whole of the safety argument.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  TASK-1474 — THE BOUNDARY. True when `SubWidget` is a nested user widget whose own
	 *  `WidgetTree` this walker may enter: it has a tree, and its class is NATIVE
	 *  (`CLASS_Native`), i.e. C++-authored rather than a `UWidgetBlueprintGeneratedClass`.
	 *
	 *  🚨 WHY AUTHORSHIP AND NOT VISIBILITY, A NAME LIST OR A PATH — AND THIS IS THE WHOLE
	 *  ARGUMENT FOR THE SHAPE. `UWidgetTree::ForEachWidgetAndDescendants` (`WidgetTree.cpp:227`)
	 *  enters EVERY nested user widget unconditionally. In this project that is not a widening,
	 *  it is a flood: `Btn_Jump` — a mobile touch-control button copied out of the template — sits
	 *  at `Overlay_19/SizeBox_0/Btn_Jump` in NINE design-time trees (`qa/TASK-1470.md` WARN-4),
	 *  and `WBP_DeckCardTile` is one of them, so the deck builder renders 34 + 1 of them at once.
	 *  Nothing about a `Btn_Jump` disqualifies it: it is a `UButton`, focusable and enabled, and
	 *  whether its own `Construct` happens to collapse it is a fact about CONTENT that no C++
	 *  reviewer sees in a diff. The two copies that ARE collapsed (`WBP_DeckBuilder`,
	 *  `WBP_MainMenu`) are collapsed by Blueprint graphs nobody on this row may read at review
	 *  time. ⇒ ⛔ A STOPPER THAT LIVES IN AN ASSET IS NOT A STOPPER THIS WALKER CAN PROMISE.
	 *
	 *  ⭐ WHAT THE FLAG ACTUALLY BUYS, STATED AS A PROPERTY: a C++-authored subtree's stop set is
	 *  a function in `Source/` — it moves only in a diff that QA reads, in the same review as the
	 *  walker. A Blueprint-authored subtree's stop set moves when someone drags a button in UMG,
	 *  with no C++ diff at all. This walker enters the first and stops at the second, so the ring
	 *  can only ever grow through code review.
	 *
	 *  ⚠️ THE COST, DECLARED RATHER THAN HOPED PAST (`SC-§101`): it is CONSERVATIVE IN ONE
	 *  DIRECTION ONLY. A genuinely navigable Blueprint sub-widget is left unreachable — never
	 *  admitted as a phantom. `USiegeControlsHelpWidget::RowWidgetClass` /
	 *  `DetailWidgetClass` are `TSoftClassPtr`s whose resolvers fall back to
	 *  `USiegeControlsHelpRowWidget::StaticClass()` / `USiegeControlsDetailWidget::StaticClass()`
	 *  when unset — which they are today, there being no `WBP_ControlsHelp*` asset — so those rows
	 *  ARE entered (cited by text, `CITE-BY-TEXT-RULED-2026-09-24`: that file is another row's).
	 *  Pointing either soft class at a WBP would silently stop the descent at that boundary. That
	 *  is a real future trip and it fails SAFE.
	 */
	static bool IsCodeAuthoredSubWidget(const UUserWidget* SubWidget);

	/**
	 *  TASK-1474 — the traversal `GetMenuFocusStops()` runs, factored out so it can recurse.
	 *  `Tree->ForEachWidget` exactly as before (depth-first pre-order, named slots included),
	 *  appending every `IsNavFocusStop()` widget, and additionally recursing into the own
	 *  `WidgetTree` of each nested user widget that passes `IsCodeAuthoredSubWidget`, AT THE
	 *  POINT THE PARENT WALK REACHES IT — so the result is a strict SUPERSET of the old one with
	 *  nothing permuted, which is what makes a count delta attributable.
	 *
	 *  ⛔ `Depth` IS A TERMINATION BOUND, NOT A POLICY. A widget tree cannot contain itself, so
	 *  this cannot cycle; the bound exists so that a pathological content nesting costs a bounded
	 *  walk per press rather than a stack overflow. Exceeding it drops stops (never invents them).
	 */
	static void CollectNavStopsFromTree(const UWidgetTree* Tree, TArray<UWidget*>& OutStops, int32 Depth);

	/** TASK-1474 — the descent's termination bound. Today's deepest real nesting is 1. */
	static constexpr int32 MaxNavDescentDepth = 8;

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1429 — THE IN-MATCH ARM. ⛔ Read the class comment's "THE IN-MATCH ARM" section
	//  before changing anything here: this is the only code in the project that can decide
	//  whether the arrow keys belong to a menu or to a live match.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  🚨 THE PREDICATE. "An in-match screen is open" ⇔ `GetRegisteredNavTarget() != nullptr`,
	 *  i.e. some entry of `NavTargetStack` is a live `UUserWidget` that `IsInViewport()` and
	 *  `IsVisible()`. ⛔ IT IS A DEMAND TEST: only an explicit `RegisterMenuNavTarget(self)` can
	 *  make it true, so nothing becomes "open" by being drawn, being on top, or existing — which
	 *  is exactly why the HUD and the FPS counter need no special case.
	 *  ⭐ ONE-LINE BODY ON PURPOSE. It is a NAME for the predicate, so that every site reads the
	 *  same definition and the spec's "state the exact predicate" has a single address in code.
	 */
	bool IsInMatchScreenOpen() const;

	/**
	 *  Make the APPLIED state equal the DEMAND. Called from the two registration edges with
	 *  `bMayArm = true`, and from the backstop poll with `bMayArm = false`.
	 *
	 *  ⛔ `bMayArm` IS THE ONE-WAY VALVE AND IT IS NOT A CONVENIENCE: the backstop exists to catch
	 *  a screen that vanished without unregistering, and a backstop that could also ARM would be
	 *  able to hand a live match to a menu on its own. With it false the poll can only ever give
	 *  the keys back.
	 *
	 *  ⛔ THE MENU MAP IS EXCLUDED BY `bArmed`, NOT BY A SECOND MAP TEST. `bArmed` is assigned
	 *  true in exactly one place — `OnWorldBeginPlay`, AFTER the `MenuMapName` gate — so it is
	 *  true on `L_MainMenu` and nowhere else, and a second copy of that gate here is a second
	 *  thing to drift. ⚠️ The degenerate case is named rather than hidden: if the menu map's own
	 *  arm FAILED (no controller, missing asset) `bArmed` stays false and a registration there
	 *  would take the in-match path — which applies the same context at the same priority the
	 *  menu map wanted anyway, i.e. it degrades toward repair, not toward breakage.
	 */
	void ReconcileInMatchArming(const TCHAR* Reason, bool bMayArm);

	/**
	 *  Load, bind (once per input component) and APPLY `IMC_MainMenu` at
	 *  `InMatchMenuMappingContextPriority`. Idempotent; returns true when the vocabulary is armed
	 *  on return. ⛔ Every failure is named in a Warning and leaves the match untouched.
	 */
	bool ArmInMatchMenuVocabulary(const TCHAR* Reason);

	/**
	 *  ⛔ REMOVE the context — `RemoveMappingContext`, never "leave it and decline in the
	 *  handlers". A key a menu context has claimed is a key the player has lost, whatever the
	 *  handler then does with it. Also retires the backstop timer. Idempotent.
	 */
	void DisarmInMatchMenuVocabulary(const TCHAR* Reason);

	/** The backstop tick: `ReconcileInMatchArming(..., bMayArm = false)`. See that function. */
	void PollInMatchDemand();

	/** IA_MenuUp → focus the previous button (wrapping). */
	void HandleMenuUp();

	/** IA_MenuDown → focus the next button (wrapping). */
	void HandleMenuDown();

	/** IA_MenuAccept → `OnClicked.Broadcast()` on the focused menu button. */
	void HandleMenuAccept();

	/** Shared Up/Down body. */
	void MoveFocus(int32 Delta);

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1409 — LEFT / RIGHT / BACK
	// ═════════════════════════════════════════════════════════════════════════════════════

	/** IA_MenuLeft → step the focused stop DOWN / press a stepper's "<" member. */
	void HandleMenuLeft();

	/** IA_MenuRight → step the focused stop UP / press a stepper's ">" member. */
	void HandleMenuRight();

	/**
	 *  IA_MenuBack → ask the ACTIVE nav target to close itself through
	 *  `ISiegeMenuNavCloseTarget::OnMenuNavBackRequested`. ⛔ It removes nothing itself.
	 */
	void HandleMenuBack();

	/**
	 *  Shared Left/Right body: `Direction` is −1 (Left) or +1 (Right). It dispatches on the
	 *  CLASS of the focused stop — that dispatch IS the deliverable, the binding is not.
	 *  ⛔ Every path ends in a log line; there is no branch that returns in silence.
	 */
	void StepFocusedStop(int32 Direction);

	/**
	 *  `USlider` semantics: value += Direction × (its own `GetStepSize()`, else the fallback),
	 *  clamped — AND THEN THE COMMIT EDGE. ⛔ The second half is not optional: `SetValue` fires
	 *  `OnValueChanged`, which on this project's sliders is the LABEL-ONLY handler, while the
	 *  facade write lives on `OnMouseCaptureEnd` / `OnControllerCaptureEnd` and is unreachable
	 *  from a programmatic set. On a REAL change this broadcasts `OnControllerCaptureEnd` (one
	 *  edge; `OnMouseCaptureEnd` only if that is the sole bound one), so the stepped value is
	 *  actually applied and saved. ⛔ Nothing is broadcast when the value did not move.
	 */
	void StepSlider(USlider* Slider, int32 Direction) const;

	/**
	 *  `UCheckBox` semantics, shared by Left/Right (set) and Accept (toggle). Returns true when
	 *  the state actually changed. ⛔ Broadcasts `OnCheckStateChanged` ONLY on a real change
	 *  (CONVENTIONS "Delegates (C++)": never on a refused or no-op mutation) — and it MUST
	 *  broadcast, because `UCheckBox::SetIsChecked` does not.
	 */
	bool SetCheckBoxChecked(UCheckBox* CheckBox, bool bNewChecked, const FString& Context) const;

	/**
	 *  Is `Focused` one half of a `<` / `>` stepper pair? On true, `OutPrev` / `OutNext` are the
	 *  two members (either may BE `Focused`). ⛔ Two independent discriminators, both taken from
	 *  the code that builds the only steppers this project has, and both required to be
	 *  UNAMBIGUOUS — a panel holding any other count of buttons is refused outright, so a
	 *  "Back / Apply" footer pair can never be mistaken for a stepper and pressed by an arrow key.
	 */
	static bool FindStepperPair(const UButton* Focused, UButton*& OutPrev, UButton*& OutNext);

	/**
	 *  Place the visible focus on the TOP button (`Buttons[0]`, "Play (vs Bot)") when the menu is
	 *  uncovered and nothing on it holds focus. Idempotent, and it holds NO widget pointer -- every
	 *  input is re-resolved from the live viewport on every call.
	 *
	 *  ⚠️ TWO CALLERS SINCE TASK-1400 (this comment said "next-tick after BeginPlay" and "fired
	 *  once" while that was true; leaving it would have been a fail-silent):
	 *    1. `SetTimerForNextTick` in `OnWorldBeginPlay` -- the INITIAL placement at level boot.
	 *    2. the looping `FocusReentryPollTimerHandle` -- the RE-ENTRY re-arm (see that member).
	 *  ⛔ It never RESTORES a remembered button: the target is always index 0, by 🧑 his own ask.
	 */
	void ApplyInitialFocus();

	/**
	 *  True when `WBP_MainMenu` is the only visible top-level user widget in the viewport —
	 *  i.e. no panel covers it.
	 *  ⚠️ TASK-1406: no longer called directly by the handlers — they call
	 *  `IsNavTargetActionable()`, which falls through to this ⛔ UNCHANGED function whenever
	 *  nothing is registered. Its body is not edited by that row.
	 */
	bool IsMenuUncovered() const;

	/**
	 *  TASK-1406 (fence (b)): the registered screen that owns menu navigation, or null.
	 *  ⭐ NULL IS THE REGRESSION BRANCH — every caller falls back to the shipped behaviour on it.
	 *  Reads the stack top-down and skips entries that are dead, removed from the viewport or
	 *  hidden, so a screen destroyed without unregistering cannot pin the ring to a stale tree.
	 */
	UUserWidget* GetRegisteredNavTarget() const;

	/**
	 *  TASK-1406 (fence (b)): the gate every handler now runs.
	 *  Registered ⇒ true (the registration ANSWERS the coverage question).
	 *  Nothing registered ⇒ `IsMenuUncovered()`, unchanged.
	 */
	bool IsNavTargetActionable() const;

	/**
	 *  TASK-1471: the screen that owns navigation right now AND has declared through
	 *  `RegisterSelfDrivingMenuNavTarget` that it drives its own — or null, which is the default and
	 *  the regression branch.
	 *
	 *  ⛔ IT ASKS `GetRegisteredNavTarget()`, NEVER `GetActiveNavTarget()`, and the difference is the
	 *  whole safety argument: `GetActiveNavTarget()` falls back to `WBP_MainMenu`, so reading it here
	 *  would let a declaration made by a screen that has closed (or that is buried under a newer
	 *  registration) silence the ring on a screen that never declared anything. A declaration is only
	 *  ever honoured for the screen that currently owns navigation.
	 *  ⭐ ONE-LINE MEANING, ONE ADDRESS: every gate below reads through here, so "this screen drives
	 *  itself" has exactly one definition in code.
	 */
	UUserWidget* GetSelfDrivingActiveTarget() const;

	/**
	 *  TASK-1471: the four gates in one call — true (and ONE log line naming the screen and its
	 *  class) when the active target has declared it drives its own navigation, false otherwise.
	 *
	 *  ⛔ IT LOGS RATHER THAN RETURNING SILENTLY, and that is this file's standing doctrine, not
	 *  politeness: a refused press that prints nothing is indistinguishable from an action that never
	 *  arrived, and four rows of this epic were spent on exactly that ambiguity. `Site` carries the
	 *  caller AND its direction (`MoveFocus(+1)`, `StepFocusedStop(-1)`) because one body serves both
	 *  directions and a line that cannot tell them apart is not an instrument.
	 *  ⚠️ `Log`, not `Verbose`: this fires at most once per menu key press, and it is the line that
	 *  proves the flag is doing the job the runtime criterion asks about.
	 */
	bool DeclineIfActiveTargetSelfDriving(const FString& Site) const;

	/**
	 *  TASK-1406 (5): ONE `UE_LOG` line per retarget, naming the new target and its focus-stop
	 *  count. ⭐ A count of 0 on a screen that visibly HAS controls is the single most
	 *  diagnostic line this epic can emit — it separates "the action never arrived" from "the
	 *  walker reached the tree and the tree admitted nothing".
	 */
	void LogNavTargetRetarget(const TCHAR* Event) const;

	/**
	 *  TASK-1406: place the ring on the active target's stop 0, unless a stop already holds
	 *  focus. Returns true only when focus was actually requested and taken.
	 *  ⚠️ The already-focused early-out is not politeness: `FSlateApplication::SetUserFocus`
	 *  early-returns false when the target is ALREADY focused (`SlateApplication.cpp:3028-3033`),
	 *  so a re-request on a focused stop reads as a failure that is not one (TASK-1446).
	 *  ⛔ TASK-1469 limb 1(b): that early-out carried the SAME exact-widget focus read
	 *  `GetFocusedNavStop()` did — inlined here so the tree is walked once — and had to move with
	 *  it. Left blind, a player standing in a `UEditableTextBox` reads as "nothing focused" and
	 *  every re-registration (`RegisterMenuNavTarget` ends here; `UAccountMenuWidget::ApplyMode`
	 *  re-registers on every mode change) would YANK THE RING OUT OF THE FIELD mid-typing.
	 */
	bool FocusFirstNavStop();

	/** Slate focus with `EFocusCause::Navigation` (the visible focus rectangle) for the local player. */
	bool FocusButton(UButton* Button) const;

	/**
	 *  TASK-1406: the generalisation of `FocusButton` to any focus stop. `FocusButton` is now a
	 *  one-line delegation to this, so TASK-1400's `ApplyInitialFocus` sees exactly what it saw.
	 */
	bool FocusWidget(UWidget* Widget) const;

	/** The local player controller of this world, or null. */
	APlayerController* GetLocalController() const;

	/** Kept alive for the world's lifetime so the bound `UInputAction` pointers stay valid (they are also referenced by the applied context). */
	UPROPERTY(Transient)
	TObjectPtr<const UInputMappingContext> MenuMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> MenuUpAction;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> MenuDownAction;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> MenuAcceptAction;

	/**
	 *  TASK-1409: the three navigation-extension actions. ⚠️ ANY OF THESE MAY BE NULL and that
	 *  is a supported state, not a bug — the degrade-open branch in `OnWorldBeginPlay` binds
	 *  only the ones that loaded. Nothing else in this class dereferences them.
	 */
	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> MenuLeftAction;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> MenuRightAction;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> MenuBackAction;

	/**
	 *  TASK-1400: the LOOPING re-entry poll's handle.
	 *
	 *  ⚠️ HEADER CHANGE, DECLARED LOUDLY (the row's (5) fence): this row needed NO new method --
	 *  the poll calls the EXISTING `ApplyInitialFocus()`, unchanged -- but `FTimerManager::SetTimer`
	 *  cannot arm or clear a looping timer without a stored handle, so this one member (plus its
	 *  `Engine/TimerHandle.h` include and the `FocusReentryPollSeconds` constant) is unavoidable.
	 *
	 *  Armed in `OnWorldBeginPlay` AFTER the `L_MainMenu` map gate; cleared in `Deinitialize`.
	 *  ⚠️ Clearing is NOT belt-and-braces: `UWorld::GetTimerManager()` resolves to the OWNING GAME
	 *  INSTANCE's manager, which OUTLIVES this world, so an uncleared loop would survive the travel
	 *  off the menu map.
	 */
	FTimerHandle FocusReentryPollTimerHandle;

	/**
	 *  TASK-1406 (fence (b)): the registered nav targets, most recent LAST.
	 *
	 *  A STACK rather than a single pointer because registrations genuinely nest: Settings opens
	 *  Graphics on top of itself (ZOrder 10 → 20, `SiegeGraphicsMenuWidget.cpp:585`), and closing
	 *  Graphics must hand navigation back to Settings, not to the main menu three layers down.
	 *
	 *  ⚠️ WEAK ON PURPOSE. This subsystem must never keep a closed screen alive, and a screen
	 *  destroyed without its paired `UnregisterMenuNavTarget` must fall out by itself rather than
	 *  stranding the ring on a tree that no longer exists. `GetRegisteredNavTarget()` skips dead
	 *  and no-longer-in-viewport entries on every read; `RegisterMenuNavTarget()` compacts them.
	 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UUserWidget>> NavTargetStack;

	/**
	 *  TASK-1471: the screens that have declared, through `RegisterSelfDrivingMenuNavTarget`, that
	 *  they drive their own navigation. ⛔ EMPTY IS THE DEFAULT AND EMPTY IS THE WHOLE REGRESSION
	 *  CONTRACT — `GetSelfDrivingActiveTarget()` returns null on an empty array and every gate falls
	 *  through, so a build in which no screen ever declares reaches byte-identical code.
	 *
	 *  ⚠️ A SET OF MARKS BESIDE THE STACK, NOT A FIELD INSIDE IT, and the reason is the same one the
	 *  close-handler interface was chosen for (see `ISiegeMenuNavCloseTarget` above): giving
	 *  `NavTargetStack` a per-entry field means turning it into an array of a new `USTRUCT`, i.e.
	 *  rewriting TASK-1406's shipped stack, its weak-pointer contract and all five of its lambdas to
	 *  carry one bool. This adds a member and touches no line of that stack.
	 *
	 *  ⚠️ WEAK, AND DECLARED EXACTLY LIKE ITS TWIN ABOVE so no reader has to ask why they differ.
	 *  ⛔ The `UPROPERTY` buys nothing for lifetime — a `TWeakObjectPtr` keeps nothing alive and is
	 *  nulled by the GC with or without reflection — and `Transient` keeps it off disk; it is here
	 *  for symmetry with `NavTargetStack`, which holds the same kind of pointer to the same screens.
	 *  Dead entries are compacted on every declare and dropped on the matching unregister, and
	 *  `GetSelfDrivingActiveTarget()` compares against a LIVE registered screen, so a stale mark can
	 *  never match anything (a recycled object index carries a new serial number).
	 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UUserWidget>> SelfDrivingScreens;

	bool bArmed = false;

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1429 — THE IN-MATCH ARM'S STATE. ⛔ THREE PLAIN MEMBERS, NO `UPROPERTY`, NO
	//  `UFUNCTION`, NO NEW GC SURFACE: the reflection surface of this class is UNCHANGED by
	//  this row, which is a property the build-master measures on `.generated.h`.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  The backstop poll's handle. ⚠️ Armed ONLY by `ArmInMatchMenuVocabulary` and cleared by
	 *  `DisarmInMatchMenuVocabulary` and `Deinitialize`, so it does not exist in a match with
	 *  nothing open. Cleared for the same measured reason `FocusReentryPollTimerHandle` is:
	 *  `UWorld::GetTimerManager()` resolves to the OWNING GAME INSTANCE's manager, which outlives
	 *  this world.
	 */
	FTimerHandle InMatchDemandPollTimerHandle;

	/**
	 *  The `UEnhancedInputComponent` this subsystem's six handlers are bound to, or null.
	 *
	 *  ⛔ IT IS A BINDING LEDGER, NOT AN OPTIMISATION. `BindAction` APPENDS, so arming twice on
	 *  one component would call every handler twice per press — and the in-match arm, unlike
	 *  `OnWorldBeginPlay`, can be reached many times in one world. Comparing against the LIVE
	 *  component rather than holding a bool also covers the case where the component is replaced
	 *  under us: a stale bool would report "already bound" onto a component that carries nothing.
	 *  ⚠️ Weak by construction — this subsystem must never keep a dead controller's component
	 *  alive — and deliberately NOT a `UPROPERTY` (a weak pointer needs no reflection to be safe,
	 *  and adding one would move the reflection surface this row is asserting it does not move).
	 */
	TWeakObjectPtr<UEnhancedInputComponent> BoundInMatchInputComponent;

	/**
	 *  True while `IMC_MainMenu` is applied by the IN-MATCH path at
	 *  `InMatchMenuMappingContextPriority`. ⛔ Never set on `L_MainMenu` (that map's application
	 *  is `OnWorldBeginPlay`'s and is reported by `bArmed`), so the two flags are disjoint and a
	 *  reader can always tell which door applied the context.
	 */
	bool bInMatchVocabularyArmed = false;
};
