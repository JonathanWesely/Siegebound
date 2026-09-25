// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
// FTimerHandle lives in its own header (Engine/TimerHandle.h), NOT in CoreMinimal.h and NOT in
// EngineTypes.h -- the same note SiegeKeyboardLayoutSubsystem.h:13-14 carries. Needed BY VALUE for
// TASK-1400's re-entry poll handle below.
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "SiegeMenuInputSubsystem.generated.h"

class UButton;
class UInputAction;
class UInputMappingContext;
class UUserWidget;

/**
 *  The main-menu input log category — the standing `LogSiege<Domain>` law (CONVENTIONS
 *  "Logging (C++)"). Declared here, defined in `SiegeMenuInputSubsystem.cpp`.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMenuInput, Log, All);

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
 *
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no RPC, no new
 *  relevancy tier — everything here is local-player Slate focus on the menu map.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeMenuInputSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** The ONLY map this subsystem arms on (short package name, PIE prefix stripped). */
	static const TCHAR* MenuMapName;

	/** The four authored assets (TASK-1274 deliverable (1)) — loaded, never written. */
	static const TCHAR* MenuMappingContextPath;
	static const TCHAR* MenuUpActionPath;
	static const TCHAR* MenuDownActionPath;
	static const TCHAR* MenuAcceptActionPath;

	/** The live menu widget's generated class path (`/Game/UI/WBP_MainMenu`). */
	static const TCHAR* MainMenuWidgetClassPath;

	/** Priority of `IMC_MainMenu`. Nothing else is applied on `L_MainMenu` (`applied_mapping_contexts: []`, TASK-671), so 0 is unambiguous. */
	static constexpr int32 MenuMappingContextPriority = 0;

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

	//~ UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  READ API — for the automation test (`Siegebound.MenuInput.*`) and the verifier.
	//  Every read walks the LIVE widget tree; nothing here is cached state.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/** True once `IMC_MainMenu` was applied and the three actions bound on this world's local controller. */
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

private:

	/** IA_MenuUp → focus the previous button (wrapping). */
	void HandleMenuUp();

	/** IA_MenuDown → focus the next button (wrapping). */
	void HandleMenuDown();

	/** IA_MenuAccept → `OnClicked.Broadcast()` on the focused menu button. */
	void HandleMenuAccept();

	/** Shared Up/Down body. */
	void MoveFocus(int32 Delta);

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
	 *  i.e. no panel covers it. Every handler returns early when this is false.
	 */
	bool IsMenuUncovered() const;

	/** Slate focus with `EFocusCause::Navigation` (the visible focus rectangle) for the local player. */
	bool FocusButton(UButton* Button) const;

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

	bool bArmed = false;
};
