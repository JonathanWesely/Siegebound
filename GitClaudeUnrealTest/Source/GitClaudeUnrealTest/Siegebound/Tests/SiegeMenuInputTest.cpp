// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Siegebound/DeckBuilderWidget.h"
#include "Siegebound/SiegeMenuInputSubsystem.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for the main-menu input actions (TASK-1274, MENU-INPUT-ACTIONS;
 *  law `VER-§5`, `KBD-§4`, `SC-§104`).
 *
 *  ⭐ THE ONE TEST THE ROW NAMES, ASSERTING STATE (`SC-§104` — never a tally): after
 *  `IA_MenuDown` ×2 from cold, the focused menu button's text is "Deck Builder"; after
 *  `IA_MenuAccept`, a `UDeckBuilderWidget` exists in the viewport.
 *
 *  ⚠️ THIS IS A LATENT PIE TEST — THE SUITE'S FIRST THAT DRIVES A MAP. The feature is a
 *  world subsystem gated on `L_MainMenu` whose observable is Slate focus inside the live
 *  `WBP_MainMenu` instance, and whose Accept path runs the Blueprint's own `OnClicked`
 *  handler to create the builder. None of that exists without the map, the local player,
 *  the game viewport and Enhanced Input ticking — so the test opens `/Game/Maps/L_MainMenu`
 *  through `AutomationOpenMap` (in the editor: `UEditorEngine::AutomationLoadMap` loads the
 *  map and starts a 1-client PIE; in a game: `Open L_MainMenu`), drives the SAME door the
 *  verifier uses (`UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` — Aura's
 *  `inject_input_action`), and ends the session with `FExitGameCommand`.
 *
 *  ⚠️ WHAT IT TOUCHES ON DISK, STATED: opening the shipped deck builder runs
 *  `UDeckBuilderWidget::NativeConstruct`, which calls `USiegeDeckSaveGame::MigrateToFixedSlots`
 *  on the ACTIVE PROFILE's deck slot and writes that slot ONLY when a migration was pending
 *  (`DeckBuilderWidget.cpp:545-550`). A post-`d8bfd23` save is already ten-slot, so the
 *  open is a read; this is the same open the row's acceptance (1) asks the verifier to
 *  perform in PIE. ⛔ The test mutates no deck and saves nothing itself.
 *
 *  Timing: `FWaitLatentCommand` waits are real-time (the engine's own map tests' idiom);
 *  1.0 s after map load covers the Blueprint's BeginPlay → Construct → the subsystem's
 *  next-tick initial focus; 0.25 s between injections lets Enhanced Input see the
 *  None → Triggered → None cycle so each injection is its own `Started` edge.
 *
 *  M8: adds no replicated property, no new replicated class, no RPC, no new relevancy tier.
 */

namespace SiegeMenuInputTestUtils
{
	/** The world the latent commands act on — the PIE/game world `AutomationOpenMap` produced. */
	static UWorld* GetMenuWorld()
	{
		return AutomationCommon::GetAnyGameWorld();
	}

	/** The subsystem under test on that world, or null. */
	static USiegeMenuInputSubsystem* GetMenuInput()
	{
		UWorld* World = GetMenuWorld();
		return World ? World->GetSubsystem<USiegeMenuInputSubsystem>() : nullptr;
	}

	/**
	 *  Inject one Boolean actuation through the local player's Enhanced Input subsystem —
	 *  the door `qa/TASK-787-verify.md` proved reaches our mappings. Returns false (and the
	 *  test asserts on it) when the door is not there.
	 */
	static bool InjectMenuAction(const TCHAR* ActionPath)
	{
		UWorld* World = GetMenuWorld();
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer
			? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
			: nullptr;
		const UInputAction* Action = LoadObject<UInputAction>(nullptr, ActionPath);
		if (!Input || !Action)
		{
			return false;
		}
		Input->InjectInputForAction(Action, FInputActionValue(true), TArray<UInputModifier*>(), TArray<UInputTrigger*>());
		return true;
	}

	/** The number of `UDeckBuilderWidget`s that are top-level (in-viewport) widgets of the menu world (the test asserts == 1). */
	static int32 CountDeckBuildersInViewport()
	{
		UWorld* World = GetMenuWorld();
		if (!World)
		{
			return 0;
		}
		TArray<UUserWidget*> Found;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UDeckBuilderWidget::StaticClass(), /*TopLevelOnly*/ true);
		return Found.Num();
	}
}

// ════════════════════════════════════════════════════════════════════════════
//  Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMenuInputDownTwiceThenAcceptOpensDeckBuilderTest,
	"Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMenuInputDownTwiceThenAcceptOpensDeckBuilderTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMenuInputTestUtils;

	// ⚠️ A STANDING ENGINE ERROR THE MAP HAS ALWAYS PRINTED, DECLARED SO THE FRAMEWORK DOES NOT RED
	// THIS TEST ON IT (TASK-1274 loop 1 — `qa/TASK-1280-report.md` "Build errors (TASK-1281)").
	// `BP_MenuGameMode` BeginPlay → `SetInputMode_UIOnlyEx(WidgetToFocus = WBP_MainMenu)` →
	// `FInputModeUIOnly::SetWidgetToFocus` (`PlayerController.cpp:6345`, compiled only outside
	// Shipping/Test) logs `LogPlayerController: Error: InputMode:UIOnly - Attempting to focus
	// Non-Focusable widget SObjectWidget [Widget.cpp(976)]!` because `WBP_MainMenu`'s `SObjectWidget`
	// is not focusable. It is on every `L_MainMenu` boot since before this diff; this test is merely
	// the first to run PIE on the map inside an automation capture window. Occurrences pinned to
	// EXACTLY 1 (the host measured one line per run, ×2): the test fails if the message is seen 0 or
	// 2+ times, so when the Blueprint is fixed (the owed follow-up row) THIS LINE GOES WITH IT.
	// Regex (default), `Contains`, case-insensitive; the `LogPlayerController:` category is NOT part
	// of the compared string (`AutomationTest.cpp:294` passes the message body).
	AddExpectedError(TEXT("InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget"),
		EAutomationExpectedErrorFlags::Contains, /*Occurrences*/ 1);

	// The map the subsystem arms on — and nothing else; a wrong map fails at step 1 by name.
	AutomationOpenMap(TEXT("/Game/Maps/L_MainMenu"));

	// Step 1 — the world is up, the menu is built, the subsystem is armed, the cold state is on record.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		USiegeMenuInputSubsystem* MenuInput = GetMenuInput();
		if (!TestNotNull(TEXT("USiegeMenuInputSubsystem exists on the L_MainMenu world"), MenuInput))
		{
			return true;
		}
		TestTrue(TEXT("IMC_MainMenu applied + IA_MenuUp/Down/Accept bound (IsArmed)"), MenuInput->IsArmed());
		TestNotNull(TEXT("live WBP_MainMenu instance found in the viewport"), MenuInput->FindMainMenuWidget());

		TArray<UButton*> Buttons;
		MenuInput->GetMenuButtons(Buttons);
		TestTrue(TEXT("the menu exposes at least three focusable buttons (index 2 must exist)"), Buttons.Num() >= 3);

		// "Cold" per the subsystem's model: either nothing focused, or the initial focus on
		// index 0 — both read as index 0. Anything else is a precondition failure, not a pass.
		const UButton* Focused = MenuInput->GetFocusedMenuButton();
		const int32 FocusedIndex = Focused ? Buttons.IndexOfByKey(Focused) : INDEX_NONE;
		AddInfo(FString::Printf(TEXT("cold state: focused index %d (\"%s\") of %d buttons"),
			FocusedIndex, *USiegeMenuInputSubsystem::GetButtonLabel(Focused), Buttons.Num()));
		TestTrue(TEXT("cold precondition: no menu button focused, or index 0"), FocusedIndex == INDEX_NONE || FocusedIndex == 0);
		TestEqual(TEXT("no deck builder is open before any input"), CountDeckBuildersInViewport(), 0);
		return true;
	}));

	// Step 2 — IA_MenuDown ×2 (each its own Started edge).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		TestTrue(TEXT("IA_MenuDown injected (1 of 2)"), InjectMenuAction(USiegeMenuInputSubsystem::MenuDownActionPath));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.25f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		TestTrue(TEXT("IA_MenuDown injected (2 of 2)"), InjectMenuAction(USiegeMenuInputSubsystem::MenuDownActionPath));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.25f));

	// Step 3 — STATE: the focused widget's text is "Deck Builder".
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		USiegeMenuInputSubsystem* MenuInput = GetMenuInput();
		const UButton* Focused = MenuInput ? MenuInput->GetFocusedMenuButton() : nullptr;
		if (TestNotNull(TEXT("a menu button holds Slate focus after IA_MenuDown x2"), Focused))
		{
			TestEqual(TEXT("focused button text after IA_MenuDown x2 from cold"),
				USiegeMenuInputSubsystem::GetButtonLabel(Focused), FString(TEXT("Deck Builder")));
		}
		return true;
	}));

	// Step 4 — IA_MenuAccept, then STATE: a UDeckBuilderWidget exists.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		TestTrue(TEXT("IA_MenuAccept injected"), InjectMenuAction(USiegeMenuInputSubsystem::MenuAcceptActionPath));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		TestEqual(TEXT("exactly one UDeckBuilderWidget is in the viewport after IA_MenuAccept"), CountDeckBuildersInViewport(), 1);
		return true;
	}));

	// Step 5 — the wrap ring, asserted on the pure function (no world needed, but it belongs beside its consumer).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		TestEqual(TEXT("WrapIndex(0, +2, 7) == 2"), USiegeMenuInputSubsystem::WrapIndex(0, +2, 7), 2);
		TestEqual(TEXT("WrapIndex(0, -1, 7) == 6 (Up from the top wraps to Quit)"), USiegeMenuInputSubsystem::WrapIndex(0, -1, 7), 6);
		TestEqual(TEXT("WrapIndex(6, +1, 7) == 0 (Down from Quit wraps to the top)"), USiegeMenuInputSubsystem::WrapIndex(6, +1, 7), 0);
		TestEqual(TEXT("WrapIndex(x, d, 0) == INDEX_NONE"), USiegeMenuInputSubsystem::WrapIndex(3, +1, 0), static_cast<int32>(INDEX_NONE));
		return true;
	}));

	// Tear the PIE/game session down the way the engine's own map tests do.
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
