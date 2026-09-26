// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Siegebound/DeckBuilderWidget.h"
#include "Siegebound/SiegeMenuInputSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

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
 *
 *  ═════════════════════════════════════════════════════════════════════════════════════════
 *  🚨🚨 TASK-1475 — THE COVERAGE GAP THIS FILE WAS, AND WHAT THE THREE NEW TESTS BUY
 *  (marker `TASK-1475-NAV-STOP-SUITE-COVERAGE-GAP`; boarded from `qa/TASK-1470.md` WARN-3;
 *   law `SC-§39` · `SC-§50` · `SC-§95` · `SC-§101` · `SC-§104` · `SHIP-§9`)
 *  ═════════════════════════════════════════════════════════════════════════════════════════
 *
 *  ⛔ THE FINDING, STATED BEFORE THE FIX: until this row, every assertion in this file read
 *  through `GetMenuButtons()` / `GetFocusedMenuButton()`. TASK-1406 introduced a SECOND, wider
 *  notion of "a thing the ring lands on" — `IsNavFocusStop()` — and TASK-1469 then narrowed it
 *  twice (limb 2: ancestor visibility; limb 3: the stepper collapse). ⛔ THE TWO NOTIONS HAVE
 *  DIVERGED, AND THEY AGREE ON EXACTLY ONE SCREEN — the main menu — WHICH IS THE ONE THIS FILE
 *  DRIVES. ⇒ A GREEN SUITE WAS EVIDENCE ABOUT ONE SCREEN'S BUTTON LIST AND NOTHING ELSE.
 *
 *  ─── THE SIX-SCREEN, TWO-COLUMN DIVERGENCE TABLE (the row's deliverable (1)) ───────────────
 *
 *  Column A — `GetMenuButtons()` (`SiegeMenuInputSubsystem.cpp:389-407`). Walks
 *    `FindMainMenuWidget()` ⛔ UNCONDITIONALLY — it never consults `GetActiveNavTarget()`, so on
 *    five of the six screens it is not reporting that screen AT ALL. Admits `Cast<UButton>` only,
 *    and tests only the widget's OWN `GetIsFocusable() / GetIsEnabled() / IsVisible()`.
 *  Column B — the `IsNavFocusStop()` walker, collected by `GetMenuFocusStops()` (`:875`).
 *    Walks `GetActiveNavTarget()`. Four admitted classes (`UButton`, `UCheckBox`, `USlider`,
 *    `UEditableTextBox`), PLUS limb 2 (no Collapsed/Hidden Slate ancestor) PLUS limb 3 (a
 *    recognised stepper row yields its `Prev` member only).
 *
 *    | Screen                                   | A: GetMenuButtons()          | B: IsNavFocusStop() walker                                   |
 *    |------------------------------------------|------------------------------|--------------------------------------------------------------|
 *    | Main menu   `WBP_MainMenu`               | 7  (the menu's own buttons)  | 7  — ⭐ the SAME seven widgets in the SAME order: THE ONLY     |
 *    |                                          |                              |      AGREEMENT, and it is asserted in step 1 below            |
 *    | Settings    `USettingsMenuWidget`        | 7 / 0 (see note)             | 3  (2 on the unhappy path) — `ConfirmToggleCheckBox` is a     |
 *    |                                          |                              |      `UCheckBox`, invisible to column A BY CLASS              |
 *    | Graphics    `USiegeGraphicsMenuWidget`   | 7 / 0 (see note)             | 19 (was 24: −2 limb 2, −3 limb 3); ⚠️ 21 while a video-mode   |
 *    |                                          |                              |      confirm is pending — the count has no fixed value        |
 *    | Login       `UAccountMenuWidget`         | 7 / 0 (see note)             | 3 / 5 / 4 / 5 / 2–3 by mode (Chooser / CreateForm /            |
 *    |                                          |                              |      LoginForm / CloudLinkForm / LoggedIn)                    |
 *    | Deck builder `WBP_DeckBuilder`           | 7 / 0 (see note)             | 3 details closed / 4 details open (limb 2 on                  |
 *    |                                          |                              |      `Btn_DetailsClose`, inside the Collapsed `DetailsPanel`) |
 *    | Session     `WBP_SessionMenu`            | 7 / 0 (see note)             | 4 — `AddressTextBox` at index 3 is a `UEditableTextBox`,      |
 *    |                                          |                              |      invisible to column A BY CLASS                           |
 *
 *    ⛔ THE "7 / 0" NOTE, AND IT IS THE HEADLINE RATHER THAN A FOOTNOTE: on every screen but the
 *    main menu, column A returns the MAIN MENU's seven buttons whenever `WBP_MainMenu` is still a
 *    top-level widget, and zero when it is not. ⛔ NEITHER BRANCH DEPENDS ON THE SCREEN THE PLAYER
 *    IS LOOKING AT. Verb-labelled (`SC-§97`): column B's numbers are CITED from
 *    `handoffs/TASK-1469-programmer.md` §2 (gated by `qa/TASK-1470.md`, six rows independently
 *    confirmed); column A's "7 / 0" is DERIVED here from the call graph and is NOT measured.
 *
 *  ─── WHAT THE THREE NEW TESTS ASSERT, AND WHY THEY ARE SYNTHETIC ───────────────────────────
 *  `IsNavFocusStop()` is a public static predicate over a REALIZED `UWidget`. The three new
 *  tests therefore build a small `UWidgetTree`, realize it with `TakeWidget()`, and collect with
 *  `UWidgetTree::ForEachWidget` — which is the SAME traversal and the SAME predicate
 *  `GetMenuFocusStops()` runs (`:875-905`), with only the `GetActiveNavTarget()` lookup removed.
 *  ⭐ WHY NOT DRIVE THE REAL GRAPHICS SCREEN: the row forbids baking a Graphics count, and it is
 *  right to — 19 becomes 21 the moment a stepper is pressed. The MECHANISM is stable; the count
 *  is not. So the mechanism is asserted on fixtures shaped like the real screens and NAMED after
 *  the real controls, and the only real screen asserted end-to-end is the one whose seven stops
 *  are MEASURED at runtime (`qa/TASK-1413-verify.md` §5, by name, with the wrap).
 *
 *  ⛔ NO REFACTOR RIDES THIS ROW (deliverable (4), law `SC-§50`). `GetMenuButtons()` is NOT
 *  touched, NOT deprecated and NOT unified with `IsNavFocusStop()`. ⛔ THE DIVERGENCE IS THE
 *  SUBJECT OF THE MEASUREMENT; ERASING IT WOULD DESTROY IT — and it is additionally the live
 *  predicate of TASK-1400's `ApplyInitialFocus` guard and of step 1 below.
 *
 *  🚨 EXPECTED VALUES ARE EXPLICIT LITERALS, NAMED BY SCREEN AND BY LIMB, AND ARE NEVER DERIVED
 *  FROM THE CODE UNDER TEST. That is deliberate and it is the whole point: TASK-1474 (deck-bar
 *  descent) changes the walker and WILL turn these tests red. It holds a grant scoped to
 *  UPDATING THESE EXPECTED VALUES AND NOTHING ELSE — no new test, no deleted test, no changed
 *  assertion shape — with a duty to restate every moved value one by one in its handoff.
 *  ⛔ A walker change that can land silently is the thing this file now exists to prevent.
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

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1475 — STATE FORMATTERS. ⛔ EVERY NEW ASSERTION COMPARES ONE OF THESE STRINGS TO
	//  A LITERAL, NEVER A COUNT TO A COUNT (`SC-§104`): a tally that matches for the wrong
	//  reason is not coverage, and a failure that prints only "3 != 1" cannot be diagnosed.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/** `A | B | C` — the widget NAMES of a stop list, in the traversal order the walker produced. */
	static FString JoinWidgetNames(const TArray<UWidget*>& Widgets)
	{
		TArray<FString> Names;
		Names.Reserve(Widgets.Num());
		for (const UWidget* Widget : Widgets)
		{
			Names.Add(Widget ? Widget->GetName() : FString(TEXT("<null>")));
		}
		return FString::Join(Names, TEXT(" | "));
	}

	/** `A | B | C` — the LABELS of a button list (`GetButtonLabel`, i.e. the words 🧑 he reads on screen). */
	static FString JoinButtonLabels(const TArray<UButton*>& Buttons)
	{
		TArray<FString> Labels;
		Labels.Reserve(Buttons.Num());
		for (const UButton* Button : Buttons)
		{
			Labels.Add(USiegeMenuInputSubsystem::GetButtonLabel(Button));
		}
		return FString::Join(Labels, TEXT(" | "));
	}

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  TASK-1475 — THE SYNTHETIC FIXTURES.
	//
	//  ⚠️ WHY A FIXTURE HAS TO HOLD THE `TSharedPtr<SWidget>`, AND IT IS NOT TIDINESS:
	//  `UWidget::MyWidget` is a WEAK pointer. Discarding the `TSharedRef<SWidget>` that
	//  `TakeWidget()` returns destroys the realized Slate tree on the spot, after which
	//  `GetCachedWidget()` is invalid, `UWidget::IsVisible()` is false for EVERY widget, and
	//  every stop test would pass "correctly" for an entirely fabricated reason. The fixture
	//  owns that reference for the life of the assertions.
	//
	//  ⚠️ AND WHY IT MUST BE REALIZED AT ALL: limb 2 (`HasVisibleSlateAncestry`) walks
	//  `SWidget::GetParentWidget()`. An unrealized tree has no Slate side, so an unrealized
	//  fixture cannot distinguish limb 2 working from limb 2 absent.
	// ═════════════════════════════════════════════════════════════════════════════════════

	struct FSiegeNavStopFixture
	{
		TStrongObjectPtr<UWidgetTree> Tree;
		TSharedPtr<SWidget> SlateRoot;

		bool IsBuilt() const { return Tree.IsValid() && Tree->RootWidget != nullptr && SlateRoot.IsValid(); }
	};

	/** Realize the fixture's root and cache the owning Slate reference. Call ONCE, after the tree is assembled. */
	static void RealizeFixture(FSiegeNavStopFixture& Fixture)
	{
		if (Fixture.Tree.IsValid() && Fixture.Tree->RootWidget != nullptr)
		{
			Fixture.SlateRoot = Fixture.Tree->RootWidget->TakeWidget();
		}
	}

	/** The fixture widget with this exact `GetFName()`, or null (the tests assert on it, so a typo fails loudly). */
	static UWidget* FindFixtureWidget(const UWidgetTree* Tree, const TCHAR* WidgetName)
	{
		UWidget* Found = nullptr;
		if (Tree)
		{
			const FName Target(WidgetName);
			Tree->ForEachWidget([&Found, Target](UWidget* Widget)
			{
				if (Found == nullptr && Widget != nullptr && Widget->GetFName() == Target)
				{
					Found = Widget;
				}
			});
		}
		return Found;
	}

	/**
	 *  ⭐ THE CODE UNDER TEST, COLLECTED. ~~Byte-for-byte `GetMenuFocusStops()`'s body
	 *  (`SiegeMenuInputSubsystem.cpp:875-905`) with only the `GetActiveNavTarget()` lookup
	 *  replaced by the fixture's tree:~~ the same `UWidgetTree::ForEachWidget` depth-first
	 *  pre-order traversal, the same `USiegeMenuInputSubsystem::IsNavFocusStop` predicate.
	 *
	 *  ⛔⛔ TASK-1474 (2026-09-25) — THE "BYTE-FOR-BYTE" CLAIM IS ⛔ FALSIFIED BY TASK-1474 AND THE
	 *  DIVERGENCE IS ⛔ RECORDED DELIBERATELY RATHER THAN CLOSED (`SC-§120`; the choice is stated in
	 *  `handoffs/TASK-1474-programmer.md` and is the second of the two options that row was granted).
	 *  ⛔ WHAT DIVERGED: the shipped `GetMenuFocusStops()` no longer calls `ForEachWidget` directly.
	 *  It calls `CollectNavStopsFromTree`, which runs that same walk and ADDITIONALLY recurses into
	 *  the own `WidgetTree` of every nested user widget whose class is NATIVE
	 *  (`USiegeMenuInputSubsystem::IsCodeAuthoredSubWidget`). ⛔ This copy does not recurse.
	 *  ⭐ WHY THE DIVERGENCE IS ⛔ UNOBSERVABLE IN THIS FILE, MEASURED RATHER THAN ASSUMED
	 *  (`qa/TASK-1476.md`, re-confirmed on this row): ⛔ NO FIXTURE IN THIS FILE CONTAINS A NESTED
	 *  `UUserWidget` OF ANY CLASS. Every fixture tree is built from `UVerticalBox` / `UHorizontalBox`
	 *  / `UBorder` panels and leaf controls. ⇒ porting the recursion here would execute ⛔ ZERO times
	 *  and change ⛔ ZERO results — an inert edit to a test, which is review surface bought with
	 *  nothing. ⛔ THE HONEST CONSEQUENCE, SAID PLAINLY SO IT IS NOT DISCOVERED LATER: ⛔ THE DESCENT
	 *  IS ⛔ UNCOVERED BY THIS SUITE. Its evidence is the runtime count table, not this file.
	 *  🚨 ⛔ IF ANYONE EVER ADDS A FIXTURE WITH A NESTED `UUserWidget`, THIS COPY BECOMES ⛔ WRONG
	 *  RATHER THAN MERELY NARROWER, and the recursion must be ported here in the same diff.
	 */
	static void CollectNavStops(const UWidgetTree* Tree, TArray<UWidget*>& OutStops)
	{
		OutStops.Reset();
		if (!Tree)
		{
			return;
		}
		Tree->ForEachWidget([&OutStops](UWidget* Widget)
		{
			if (USiegeMenuInputSubsystem::IsNavFocusStop(Widget))
			{
				OutStops.Add(Widget);
			}
		});
	}

	/**
	 *  🚨 THE OTHER COLUMN — AND IT IS A REPLICA, WHICH IS STATED RATHER THAN HIDDEN.
	 *  This is the SHIPPED `GetMenuButtons()` admission rule transcribed from
	 *  `SiegeMenuInputSubsystem.cpp:403` (`Cast<UButton>` + `GetIsFocusable()` +
	 *  `GetIsEnabled()` + `IsVisible()`), applied to a fixture tree because the real function
	 *  is hard-wired to `FindMainMenuWidget()` and can never be pointed at anything else.
	 *
	 *  ⛔ WHAT IT CAN AND CANNOT CATCH, SAID PLAINLY: it is a DOCUMENTED CONSTANT, not a read of
	 *  production code, so it cannot notice a future edit to `GetMenuButtons()` itself. Its job
	 *  is the measurement the row asks for — showing that the two notions give DIFFERENT answers
	 *  on the same tree — and, as a side effect, making any attempt to "unify" them go red here
	 *  with the divergence named, which is the conscious restatement `SC-§50` wants.
	 */
	static void CollectLegacyMenuButtonStyleAdmissions(const UWidgetTree* Tree, TArray<UWidget*>& OutButtons)
	{
		OutButtons.Reset();
		if (!Tree)
		{
			return;
		}
		Tree->ForEachWidget([&OutButtons](UWidget* Widget)
		{
			UButton* Button = Cast<UButton>(Widget);
			if (Button && Button->GetIsFocusable() && Button->GetIsEnabled() && Button->IsVisible())
			{
				OutButtons.Add(Button);
			}
		});
	}

	/**
	 *  FIXTURE A — the Graphics screen's confirm bar, by name (limb 2).
	 *
	 *      RootPanel (UVerticalBox)
	 *        ├─ BackButton                       ← the control: outside the border, always a stop
	 *        └─ VideoModeConfirmBorder (UBorder) ← the ancestor the test collapses and un-collapses
	 *             └─ VideoModeConfirmRow (UVerticalBox)
	 *                  ├─ KeepSettingsButton
	 *                  └─ RevertSettingsButton
	 *
	 *  ⭐ `VideoModeConfirmRow` holds EXACTLY TWO `UButton`s on purpose: that is the real shape,
	 *  and it also proves limb 3 does not eat them — neither name ends in `PrevButton`/`NextButton`
	 *  and neither carries a `<` / `>` glyph, so `FindStepperPair` refuses on both discriminators.
	 */
	static FSiegeNavStopFixture BuildConfirmBorderFixture()
	{
		FSiegeNavStopFixture Fixture;
		Fixture.Tree = TStrongObjectPtr<UWidgetTree>(NewObject<UWidgetTree>(GetTransientPackage()));
		UWidgetTree* Tree = Fixture.Tree.Get();

		UVerticalBox* RootPanel = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
		UButton* BackButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
		UBorder* ConfirmBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VideoModeConfirmBorder"));
		UVerticalBox* ConfirmRow = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VideoModeConfirmRow"));
		UButton* KeepButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("KeepSettingsButton"));
		UButton* RevertButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RevertSettingsButton"));

		ConfirmRow->AddChildToVerticalBox(KeepButton);
		ConfirmRow->AddChildToVerticalBox(RevertButton);
		ConfirmBorder->AddChild(ConfirmRow);
		RootPanel->AddChildToVerticalBox(BackButton);
		RootPanel->AddChildToVerticalBox(ConfirmBorder);

		Tree->RootWidget = RootPanel;
		RealizeFixture(Fixture);
		return Fixture;
	}

	/**
	 *  FIXTURE B — three two-button rows, only two of which are steppers (limb 3).
	 *
	 *      RootPanel (UVerticalBox)
	 *        ├─ ScreenResolutionRow (UHorizontalBox)   ← `BuildStepperRow`'s NAME discriminator
	 *        │    ├─ ScreenResolutionPrevButton
	 *        │    └─ ScreenResolutionNextButton
	 *        ├─ GlyphStepperRow (UHorizontalBox)       ← the GLYPH fallback discriminator
	 *        │    ├─ GlyphStepLeftButton   ("<")
	 *        │    └─ GlyphStepRightButton  (">")
	 *        └─ SettingsFooterRow (UHorizontalBox)     ← 🚨 THE CONTROLLED NEGATIVE
	 *             ├─ GraphicsButton
	 *             └─ BackButton
	 *
	 *  🚨 THE FOOTER ROW IS THE POINT. It is `USettingsMenuWidget`'s own shape — two buttons in one
	 *  parent — and it MUST keep BOTH stops. A limb-3 rule that collapsed "any two-button row"
	 *  would eat `BackButton` and make Settings unexitable by keyboard. This fixture fails in BOTH
	 *  directions: remove limb 3 and the two stepper rows report two stops each; widen limb 3 and
	 *  the footer reports one.
	 */
	static FSiegeNavStopFixture BuildStepperRowFixture()
	{
		FSiegeNavStopFixture Fixture;
		Fixture.Tree = TStrongObjectPtr<UWidgetTree>(NewObject<UWidgetTree>(GetTransientPackage()));
		UWidgetTree* Tree = Fixture.Tree.Get();

		UVerticalBox* RootPanel = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));

		UHorizontalBox* NamedRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ScreenResolutionRow"));
		UButton* NamedPrev = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ScreenResolutionPrevButton"));
		UButton* NamedNext = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ScreenResolutionNextButton"));
		NamedRow->AddChildToHorizontalBox(NamedPrev);
		NamedRow->AddChildToHorizontalBox(NamedNext);

		UHorizontalBox* GlyphRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GlyphStepperRow"));
		UButton* GlyphLeft = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GlyphStepLeftButton"));
		UButton* GlyphRight = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GlyphStepRightButton"));
		UTextBlock* GlyphLeftText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GlyphStepLeftText"));
		UTextBlock* GlyphRightText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GlyphStepRightText"));
		GlyphLeftText->SetText(FText::FromString(TEXT("<")));
		GlyphRightText->SetText(FText::FromString(TEXT(">")));
		GlyphLeft->AddChild(GlyphLeftText);
		GlyphRight->AddChild(GlyphRightText);
		GlyphRow->AddChildToHorizontalBox(GlyphLeft);
		GlyphRow->AddChildToHorizontalBox(GlyphRight);

		UHorizontalBox* FooterRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsFooterRow"));
		UButton* GraphicsButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GraphicsButton"));
		UButton* FooterBackButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
		FooterRow->AddChildToHorizontalBox(GraphicsButton);
		FooterRow->AddChildToHorizontalBox(FooterBackButton);

		RootPanel->AddChildToVerticalBox(NamedRow);
		RootPanel->AddChildToVerticalBox(GlyphRow);
		RootPanel->AddChildToVerticalBox(FooterRow);

		Tree->RootWidget = RootPanel;
		RealizeFixture(Fixture);
		return Fixture;
	}

	/**
	 *  FIXTURE C — the four admitted classes, the two authored opt-outs, and one class that is
	 *  deliberately NOT a stop (fence (c)).
	 *
	 *      RootPanel (UVerticalBox)
	 *        ├─ VocabButton        (UButton)            ← stop
	 *        ├─ VocabCheckBox      (UCheckBox)          ← stop — ⛔ invisible to `GetMenuButtons()`
	 *        ├─ VocabSlider        (USlider)            ← stop — ⛔ invisible to `GetMenuButtons()`
	 *        ├─ VocabTextBox       (UEditableTextBox)   ← stop — ⛔ invisible to `GetMenuButtons()`
	 *        ├─ VocabTextBlock     (UTextBlock)         ← NOT a stop (not one of the four classes)
	 *        ├─ OptedOutButton     (UButton, IsFocusable=false) ← NOT a stop (author's opt-out honoured)
	 *        └─ DisabledButton     (UButton, IsEnabled=false)   ← NOT a stop
	 *
	 *  ⚠️ HOW THE OPT-OUT IS AUTHORED, AND WHY IT IS THE DEPRECATED FIELD RATHER THAN A SETTER.
	 *  `UButton` has NO `SetIsFocusable`, and `InitIsFocusable()` (`Button.h:206`) is `protected`
	 *  (`CheckBox.h:155` likewise) — measured in the 5.8 headers, not assumed. The only route an
	 *  outside caller has is the deprecated public field, which is EXACTLY what this project's own
	 *  shipped opt-out does at `SiegeControlsHelpWidget.cpp:176-178`, pragmas and all. ⭐ Mirroring
	 *  that call byte-for-byte is the point: the fixture opts out the way the codebase opts out,
	 *  so what `IsNavFocusStop` is tested against is the authored shape and not a test-only one.
	 *  ⛔ The pragmas are REQUIRED, not decorative: UBT builds with `-WarningsAsErrors`.
	 *
	 *  ⚠️ `VocabCheckBox` takes the constructor default (`CheckBox.cpp:41` sets `IsFocusable = true`)
	 *  because the setter is protected. The test asserts that default through the public
	 *  `GetIsFocusable()` BEFORE relying on it, so an engine change to that default fails loudly at
	 *  the precondition instead of quietly turning a stop assertion into a no-op.
	 *
	 *  ⚠️ `USlider::IsFocusable` is written as a public member because `USlider` did NOT take the
	 *  5.2 deprecation (`Slider.h:96` — no getter, no `UE_DEPRECATED`), which is the same reason
	 *  `IsNavFocusStop` reads it as a member. Setting it explicitly rather than trusting the
	 *  constructor's `true` keeps this test independent of an engine default.
	 */
	static FSiegeNavStopFixture BuildVocabularyFixture()
	{
		FSiegeNavStopFixture Fixture;
		Fixture.Tree = TStrongObjectPtr<UWidgetTree>(NewObject<UWidgetTree>(GetTransientPackage()));
		UWidgetTree* Tree = Fixture.Tree.Get();

		UVerticalBox* RootPanel = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));

		UButton* VocabButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("VocabButton"));
		UCheckBox* VocabCheckBox = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("VocabCheckBox"));
		USlider* VocabSlider = Tree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("VocabSlider"));
		UEditableTextBox* VocabTextBox = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("VocabTextBox"));
		UTextBlock* VocabTextBlock = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VocabTextBlock"));
		UButton* OptedOutButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OptedOutButton"));
		UButton* DisabledButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DisabledButton"));

		// `USlider` did NOT take the 5.2 deprecation, so this one is a plain public write.
		VocabSlider->IsFocusable = true;

		// ⭐ THE AUTHORED OPT-OUT, COPIED FROM THE ONLY PLACE THIS PROJECT AUTHORS ONE
		// (`SiegeControlsHelpWidget.cpp:176-178`). Same field, same pragmas, same order.
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		OptedOutButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		DisabledButton->SetIsEnabled(false);

		RootPanel->AddChildToVerticalBox(VocabButton);
		RootPanel->AddChildToVerticalBox(VocabCheckBox);
		RootPanel->AddChildToVerticalBox(VocabSlider);
		RootPanel->AddChildToVerticalBox(VocabTextBox);
		RootPanel->AddChildToVerticalBox(VocabTextBlock);
		RootPanel->AddChildToVerticalBox(OptedOutButton);
		RootPanel->AddChildToVerticalBox(DisabledButton);

		Tree->RootWidget = RootPanel;
		RealizeFixture(Fixture);
		return Fixture;
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

		// ═══════════════════════════════════════════════════════════════════════════════════
		//  🚨 TASK-1475 (3) — THE REGRESSION THAT MATTERS MOST: THE MAIN MENU IS STILL SEVEN,
		//  AND THE TWO NOTIONS STILL AGREE ON IT ELEMENT BY ELEMENT.
		//
		//  ⛔ THIS REPLACES A `Buttons.Num() >= 3` TALLY (`SC-§104`). `>= 3` was true of a menu
		//  that had lost four entries, and it was the only thing standing between this suite and
		//  a silently gutted ring.
		//
		//  PROVENANCE OF THE LITERALS, VERB-LABELLED (`SC-§97`):
		//   • the seven LABELS and their order are MEASURED at runtime by `qa/TASK-1413-verify.md`
		//     §5 — walked by name in both directions, `of 7` on every line, ring closed with the
		//     wrap at `6 -> 0`.
		//   • that the STOP walker and `GetMenuButtons()` return the SAME seven is DERIVED, and
		//     the derivation is short enough to state: TASK-1413's seven stops were measured on
		//     the PRE-1469 walker (own-visibility + four classes) and all seven are `UButton`s ⇒
		//     `GetMenuButtons()`, which is that rule minus the three extra classes, returns
		//     exactly them; `WBP_MainMenu`'s design-time `Btn_Jump` (`qa/TASK-1470.md` WARN-5) was
		//     in NEITHER set, so it fails a clause both rules share. TASK-1469's limbs can only
		//     REMOVE, and §3 of its handoff shows neither limb can remove any of the seven.
		//   • ⚠️ IF THIS GOES RED WITH A SHORTER LIST, THE RING WAS SEVERED, NOT RENAMED — that is
		//     the failure mode `qa/TASK-1413-verify.md` §6 measured on the Graphics screen.
		// ═══════════════════════════════════════════════════════════════════════════════════
		TArray<UWidget*> Stops;
		MenuInput->GetMenuFocusStops(Stops);

		TArray<UWidget*> ButtonsAsWidgets;
		ButtonsAsWidgets.Reserve(Buttons.Num());
		for (UButton* Button : Buttons)
		{
			ButtonsAsWidgets.Add(Button);
		}

		AddInfo(FString::Printf(TEXT("main menu: GetMenuButtons() = %d [%s]; IsNavFocusStop() walker = %d [%s]"),
			Buttons.Num(), *JoinWidgetNames(ButtonsAsWidgets), Stops.Num(), *JoinWidgetNames(Stops)));

		TestEqual(TEXT("MAIN MENU / GetMenuButtons() — the seven button labels, in tree order (TASK-1413 §5, MEASURED by name)"),
			JoinButtonLabels(Buttons),
			FString(TEXT("Play (vs Bot) | Sandbox (No Bot) | Deck Builder | Multiplayer | Settings | Login | Quit")));
		TestEqual(TEXT("MAIN MENU / GetMenuButtons() — exactly 7 buttons"), Buttons.Num(), 7);
		TestEqual(TEXT("MAIN MENU / IsNavFocusStop() walker — exactly 7 focus stops (TASK-1469 count table: 7 -> 7, limb 1/2/3 all no-ops here)"),
			Stops.Num(), 7);
		TestEqual(TEXT("MAIN MENU — the two notions name the same widgets in the same order (⛔ THE AGREEMENT THE WHOLE COVERAGE ARGUMENT RESTS ON)"),
			JoinWidgetNames(Stops), JoinWidgetNames(ButtonsAsWidgets));

		bool bElementwiseIdentical = (Stops.Num() == Buttons.Num());
		for (int32 StopIndex = 0; bElementwiseIdentical && StopIndex < Stops.Num(); ++StopIndex)
		{
			bElementwiseIdentical = (Stops[StopIndex] == static_cast<UWidget*>(Buttons[StopIndex]));
		}
		TestTrue(TEXT("MAIN MENU — element-by-element POINTER identity between GetMenuButtons() and the stop walker (a name match is not an identity match)"),
			bElementwiseIdentical);

		for (UButton* Button : Buttons)
		{
			TestTrue(*FString::Printf(TEXT("MAIN MENU — IsNavFocusStop('%s') is true"), *GetNameSafe(Button)),
				USiegeMenuInputSubsystem::IsNavFocusStop(Button));
		}

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

// ════════════════════════════════════════════════════════════════════════════
//  TASK-1475 — Siegebound.MenuInput.NavStop.CollapsedAncestorIsNotAStop  (LIMB 2)
//
//  ⛔ THE DEFECT THIS ASSERTS AGAINST IS MEASURED, NOT IMAGINED: `qa/TASK-1413-verify.md` §6
//  read the Graphics ring SEVERED at `KeepSettingsButton` (18) / `RevertSettingsButton` (19) —
//  `MoveFocus(+1): focus moved 17 -> 18 of 24 ('KeepSettingsButton')` SIX TIMES IN A ROW —
//  because both buttons reported their OWN visibility as visible while their
//  `VideoModeConfirmBorder` ancestor was Collapsed, and a stop that cannot take focus SWALLOWS
//  the ring rather than being skipped.
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMenuInputNavStopCollapsedAncestorTest,
	"Siegebound.MenuInput.NavStop.CollapsedAncestorIsNotAStop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMenuInputNavStopCollapsedAncestorTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMenuInputTestUtils;

	if (!TestTrue(TEXT("this test realizes Slate widgets and must run on the game thread"), IsInGameThread()))
	{
		return false;
	}

	FSiegeNavStopFixture Fixture = BuildConfirmBorderFixture();
	if (!TestTrue(TEXT("the Graphics-shaped confirm-border fixture built AND realized (an unrealized fixture would pass for the wrong reason)"), Fixture.IsBuilt()))
	{
		return false;
	}

	UWidget* ConfirmBorder = FindFixtureWidget(Fixture.Tree.Get(), TEXT("VideoModeConfirmBorder"));
	UWidget* KeepButton = FindFixtureWidget(Fixture.Tree.Get(), TEXT("KeepSettingsButton"));
	if (!TestNotNull(TEXT("fixture contains 'VideoModeConfirmBorder'"), ConfirmBorder) ||
		!TestNotNull(TEXT("fixture contains 'KeepSettingsButton'"), KeepButton))
	{
		return false;
	}

	TArray<UWidget*> Stops;
	TArray<UWidget*> LegacyAdmissions;

	// ─── ARM A — the border is VISIBLE. This is `ArmVideoModeCountdown()`'s state, and it is the
	//     reason limb 2 is NOT a permanent exclusion: on that frame both buttons are stops again.
	ConfirmBorder->SetVisibility(ESlateVisibility::Visible);
	CollectNavStops(Fixture.Tree.Get(), Stops);
	AddInfo(FString::Printf(TEXT("confirm border VISIBLE   -> stops: [%s]"), *JoinWidgetNames(Stops)));
	TestEqual(TEXT("GRAPHICS SHAPE / limb 2 / border VISIBLE — stop names in tree order"),
		JoinWidgetNames(Stops),
		FString(TEXT("BackButton | KeepSettingsButton | RevertSettingsButton")));

	// ─── ARM B — the border is COLLAPSED. Limb 2 must drop BOTH confirm buttons and NOTHING else.
	//     ⭐ SAME WIDGETS, SAME TREE, ONE PROPERTY FLIPPED: this pair is the controlled negative.
	//     Delete `HasVisibleSlateAncestry`'s call from `IsNavFocusStop` and THIS assertion reads
	//     the ARM-A string and goes red; that is the red this test is built to produce.
	ConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	CollectNavStops(Fixture.Tree.Get(), Stops);
	AddInfo(FString::Printf(TEXT("confirm border COLLAPSED -> stops: [%s]"), *JoinWidgetNames(Stops)));
	TestEqual(TEXT("GRAPHICS SHAPE / limb 2 / border COLLAPSED — a control inside a Collapsed ancestor is NOT collected"),
		JoinWidgetNames(Stops),
		FString(TEXT("BackButton")));

	// ─── THE PREMISE OF LIMB 2, ASSERTED RATHER THAN ASSUMED ───────────────────────────────────
	//     If this ever reads false, limb 2 has become unreachable code and ARM B above would be
	//     passing for a reason that has nothing to do with the ancestor walk.
	TestTrue(TEXT("GRAPHICS SHAPE / limb 2 premise — 'KeepSettingsButton' still reports its OWN UWidget::IsVisible() as TRUE under the Collapsed ancestor"),
		KeepButton->IsVisible());

	// ─── THE DIVERGENCE, MEASURED ON ONE TREE (the row's deliverable (1) as an assertion) ──────
	CollectLegacyMenuButtonStyleAdmissions(Fixture.Tree.Get(), LegacyAdmissions);
	AddInfo(FString::Printf(TEXT("confirm border COLLAPSED -> GetMenuButtons()-shaped admissions: [%s]"), *JoinWidgetNames(LegacyAdmissions)));
	TestEqual(TEXT("GRAPHICS SHAPE / DIVERGENCE — the GetMenuButtons() predicate (SiegeMenuInputSubsystem.cpp:403) is BLIND to the Collapsed ancestor and still admits all three"),
		JoinWidgetNames(LegacyAdmissions),
		FString(TEXT("BackButton | KeepSettingsButton | RevertSettingsButton")));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TASK-1475 — Siegebound.MenuInput.NavStop.StepperRowIsOneStop  (LIMB 3)
//
//  ⛔ MEASURED PROVENANCE: `qa/TASK-1410.md` WARN-2 found the Graphics screen giving SIX stops
//  where the screen shows THREE stepper rows. TASK-1469 limb 3 keeps the `Prev` member only,
//  and `StepFocusedStop` drives the whole row from it.
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMenuInputNavStopStepperRowTest,
	"Siegebound.MenuInput.NavStop.StepperRowIsOneStop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMenuInputNavStopStepperRowTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMenuInputTestUtils;

	if (!TestTrue(TEXT("this test realizes Slate widgets and must run on the game thread"), IsInGameThread()))
	{
		return false;
	}

	FSiegeNavStopFixture Fixture = BuildStepperRowFixture();
	if (!TestTrue(TEXT("the three-row stepper fixture built AND realized"), Fixture.IsBuilt()))
	{
		return false;
	}

	TArray<UWidget*> Stops;
	TArray<UWidget*> LegacyAdmissions;

	CollectNavStops(Fixture.Tree.Get(), Stops);
	CollectLegacyMenuButtonStyleAdmissions(Fixture.Tree.Get(), LegacyAdmissions);
	AddInfo(FString::Printf(TEXT("stepper fixture -> stops: [%s]"), *JoinWidgetNames(Stops)));
	AddInfo(FString::Printf(TEXT("stepper fixture -> GetMenuButtons()-shaped admissions: [%s]"), *JoinWidgetNames(LegacyAdmissions)));

	// ⭐ ONE ASSERTION CARRIES ALL THREE ROWS, AND THAT IS DELIBERATE: the named stepper keeps its
	//    `Prev`, the glyph stepper keeps its `<`, and the Settings footer keeps BOTH — a single
	//    ordered string that no one-sided change to limb 3 can satisfy.
	TestEqual(TEXT("STEPPER SHAPES / limb 3 — stop names in tree order: named pair -> Prev only, glyph pair -> '<' only, Settings footer -> BOTH"),
		JoinWidgetNames(Stops),
		FString(TEXT("ScreenResolutionPrevButton | GlyphStepLeftButton | GraphicsButton | BackButton")));

	// The individual limbs, spelled out so a failure names WHICH discriminator moved.
	TestTrue(TEXT("STEPPER SHAPES / limb 3 / NAME discriminator — 'ScreenResolutionPrevButton' IS a stop"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("ScreenResolutionPrevButton"))));
	TestFalse(TEXT("STEPPER SHAPES / limb 3 / NAME discriminator — 'ScreenResolutionNextButton' is NOT a stop"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("ScreenResolutionNextButton"))));
	TestTrue(TEXT("STEPPER SHAPES / limb 3 / GLYPH discriminator — the '<' member IS a stop"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("GlyphStepLeftButton"))));
	TestFalse(TEXT("STEPPER SHAPES / limb 3 / GLYPH discriminator — the '>' member is NOT a stop"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("GlyphStepRightButton"))));

	// 🚨 THE CONTROLLED NEGATIVE. `USettingsMenuWidget`'s footer holds exactly two buttons in one
	//    parent — the same STRUCTURE as a stepper — and both must survive. This is the assertion
	//    that goes red if limb 3 is ever widened into a structure-only rule, and it is the reason
	//    the Settings screen still reports 3 stops rather than 2.
	TestTrue(TEXT("SETTINGS FOOTER SHAPE / limb 3 CONTROLLED NEGATIVE — 'GraphicsButton' IS still a stop (two buttons in one parent is NOT a stepper)"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("GraphicsButton"))));
	TestTrue(TEXT("SETTINGS FOOTER SHAPE / limb 3 CONTROLLED NEGATIVE — 'BackButton' IS still a stop (⛔ eating it would make Settings unexitable by keyboard)"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("BackButton"))));

	// ─── THE DIVERGENCE ON THE SAME TREE: six admissions against four stops ────────────────────
	TestEqual(TEXT("STEPPER SHAPES / DIVERGENCE — the GetMenuButtons() predicate has NO stepper rule and admits all six buttons"),
		JoinWidgetNames(LegacyAdmissions),
		FString(TEXT("ScreenResolutionPrevButton | ScreenResolutionNextButton | GlyphStepLeftButton | GlyphStepRightButton | GraphicsButton | BackButton")));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TASK-1475 — Siegebound.MenuInput.NavStop.FocusStopVocabulary  (FENCE (c))
//
//  ⛔ WHY THIS IS THE THIRD TEST AND NOT A NICETY: the vocabulary is the OTHER half of the
//  divergence. On Settings the stop the player needs first is a `UCheckBox`; on Session it is a
//  `UEditableTextBox`; on Graphics eleven of the stops are `USlider`s. `GetMenuButtons()` cannot
//  see ANY of them, which is why a green `GetMenuButtons()` suite said nothing about those
//  screens even before limbs 2 and 3 existed.
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMenuInputNavStopVocabularyTest,
	"Siegebound.MenuInput.NavStop.FocusStopVocabulary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMenuInputNavStopVocabularyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMenuInputTestUtils;

	if (!TestTrue(TEXT("this test realizes Slate widgets and must run on the game thread"), IsInGameThread()))
	{
		return false;
	}

	// A null widget is never a stop — the one clause every caller depends on and nobody states.
	TestFalse(TEXT("VOCABULARY — IsNavFocusStop(nullptr) is false"),
		USiegeMenuInputSubsystem::IsNavFocusStop(nullptr));

	FSiegeNavStopFixture Fixture = BuildVocabularyFixture();
	if (!TestTrue(TEXT("the four-class vocabulary fixture built AND realized"), Fixture.IsBuilt()))
	{
		return false;
	}

	// ⭐ TASK-1469 (2a), ASSERTED: realization is NOT a separate limb. A structurally perfect
	//    button that was never realized has no cached `SWidget`, so it fails `UWidget::IsVisible()`
	//    and `HasVisibleSlateAncestry()` alike. Constructed in the tree but never parented, so the
	//    walk below never reaches it — and held in a `TStrongObjectPtr` precisely BECAUSE nothing
	//    in the tree references it, which is the one object here that GC could legally reclaim.
	const TStrongObjectPtr<UButton> UnrealizedButton(
		Fixture.Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UnrealizedButton")));
	TestFalse(TEXT("VOCABULARY — an UNREALIZED UButton is NOT a stop (TASK-1469 (2a): realization is a condition the predicate already tested)"),
		USiegeMenuInputSubsystem::IsNavFocusStop(UnrealizedButton.Get()));

	// ⚠️ THE ONE ENGINE DEFAULT THIS FIXTURE LEANS ON, ASSERTED RATHER THAN TRUSTED.
	// `UCheckBox::InitIsFocusable` is protected, so the check box takes the constructor's
	// `IsFocusable = true` (`CheckBox.cpp:41`). If a future engine flips that default, the stop
	// assertion below would start passing/failing for a reason that has nothing to do with this
	// project — so the default is checked here, through the public getter, first.
	const UCheckBox* VocabCheckBox = Cast<UCheckBox>(FindFixtureWidget(Fixture.Tree.Get(), TEXT("VocabCheckBox")));
	TestTrue(TEXT("VOCABULARY precondition — the fixture's UCheckBox is focusable by engine default (CheckBox.cpp:41)"),
		VocabCheckBox != nullptr && VocabCheckBox->GetIsFocusable());

	TArray<UWidget*> Stops;
	TArray<UWidget*> LegacyAdmissions;

	CollectNavStops(Fixture.Tree.Get(), Stops);
	CollectLegacyMenuButtonStyleAdmissions(Fixture.Tree.Get(), LegacyAdmissions);
	AddInfo(FString::Printf(TEXT("vocabulary fixture -> stops: [%s]"), *JoinWidgetNames(Stops)));
	AddInfo(FString::Printf(TEXT("vocabulary fixture -> GetMenuButtons()-shaped admissions: [%s]"), *JoinWidgetNames(LegacyAdmissions)));

	TestEqual(TEXT("VOCABULARY / fence (c) — the four admitted classes are stops, in tree order; UTextBlock, the opted-out button and the disabled button are not"),
		JoinWidgetNames(Stops),
		FString(TEXT("VocabButton | VocabCheckBox | VocabSlider | VocabTextBox")));

	TestFalse(TEXT("VOCABULARY — an authored IsFocusable == false is HONOURED, never stomped ('OptedOutButton')"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("OptedOutButton"))));
	TestFalse(TEXT("VOCABULARY — a disabled UButton is not a stop ('DisabledButton')"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("DisabledButton"))));
	TestFalse(TEXT("VOCABULARY — a UTextBlock is not one of the four admitted classes ('VocabTextBlock')"),
		USiegeMenuInputSubsystem::IsNavFocusStop(FindFixtureWidget(Fixture.Tree.Get(), TEXT("VocabTextBlock"))));

	// ─── THE DIVERGENCE: four stops, ONE admission ─────────────────────────────────────────────
	//     This is the Settings / Session / Graphics story in one line — the controls those screens
	//     are NAVIGATED BY are not buttons, and column A of the divergence table cannot see them.
	TestEqual(TEXT("VOCABULARY / DIVERGENCE — the GetMenuButtons() predicate admits ONLY the plain UButton: the CheckBox, the Slider and the EditableTextBox are invisible to it BY CLASS"),
		JoinWidgetNames(LegacyAdmissions),
		FString(TEXT("VocabButton")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
