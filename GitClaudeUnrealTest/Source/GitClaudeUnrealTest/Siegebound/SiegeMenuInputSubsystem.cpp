// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeMenuInputSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogSiegeMenuInput);

// ─── The names (TASK-1274 deliverable (1); CONVENTIONS naming — `/Game/Input/IMC_*`, `/Game/Input/Actions/IA_*`) ───
const TCHAR* USiegeMenuInputSubsystem::MenuMapName             = TEXT("L_MainMenu");
const TCHAR* USiegeMenuInputSubsystem::MenuMappingContextPath  = TEXT("/Game/Input/IMC_MainMenu.IMC_MainMenu");
const TCHAR* USiegeMenuInputSubsystem::MenuUpActionPath        = TEXT("/Game/Input/Actions/IA_MenuUp.IA_MenuUp");
const TCHAR* USiegeMenuInputSubsystem::MenuDownActionPath      = TEXT("/Game/Input/Actions/IA_MenuDown.IA_MenuDown");
const TCHAR* USiegeMenuInputSubsystem::MenuAcceptActionPath    = TEXT("/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept");
const TCHAR* USiegeMenuInputSubsystem::MainMenuWidgetClassPath = TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C");

bool USiegeMenuInputSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Game and PIE only. The engine default also admits EWorldType::Editor; an Editor world
	// must never arm input bindings or place Slate focus (and never touches L_MainMenu's
	// dirty state — acceptance (5)).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USiegeMenuInputSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// ⛔ L_MainMenu ONLY. The world's package is "UEDPIE_0_L_MainMenu" in PIE and
	// "L_MainMenu" in a game; strip the PIE prefix, then compare the short asset name.
	const FString MapName = FPackageName::GetShortName(UWorld::RemovePIEPrefix(InWorld.GetOutermost()->GetName()));
	if (MapName != MenuMapName)
	{
		return;
	}

	// The local controller exists here: SpawnPlayActor runs before World->BeginPlay on both
	// the PIE (GameInstance.cpp:538 vs :566) and the LoadMap (UnrealEngine.cpp:16647 vs
	// :16661) paths. Measured class on this map: /Script/Engine.PlayerController.
	APlayerController* PC = GetLocalController();
	if (!PC)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: no local player controller at OnWorldBeginPlay — menu input actions NOT armed."),
			MenuMapName);
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!InputSubsystem || !InputComponent)
	{
		// DefaultInput.ini:81-82 pin EnhancedPlayerInput / EnhancedInputComponent, so a null
		// here means the config moved — say so instead of failing silently.
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: Enhanced Input unavailable on the local controller (subsystem %s, component %s) — menu input actions NOT armed."),
			MenuMapName,
			InputSubsystem ? TEXT("ok") : TEXT("null"),
			InputComponent ? TEXT("ok") : TEXT("null / not UEnhancedInputComponent"));
		return;
	}

	// Load the four authored assets. LoadObject (not a UPROPERTY slot on a Blueprint) because
	// no project class exists on this map to carry a slot — see the header. A missing asset
	// is logged by name (the Artist/asset side of the row) and the feature stays off.
	MenuMappingContext = LoadObject<UInputMappingContext>(nullptr, MenuMappingContextPath);
	MenuUpAction       = LoadObject<UInputAction>(nullptr, MenuUpActionPath);
	MenuDownAction     = LoadObject<UInputAction>(nullptr, MenuDownActionPath);
	MenuAcceptAction   = LoadObject<UInputAction>(nullptr, MenuAcceptActionPath);
	if (!MenuMappingContext || !MenuUpAction || !MenuDownAction || !MenuAcceptAction)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: input assets missing (IMC_MainMenu %s, IA_MenuUp %s, IA_MenuDown %s, IA_MenuAccept %s) — menu input actions NOT armed."),
			MenuMapName,
			MenuMappingContext ? TEXT("ok") : TEXT("MISSING"),
			MenuUpAction       ? TEXT("ok") : TEXT("MISSING"),
			MenuDownAction     ? TEXT("ok") : TEXT("MISSING"),
			MenuAcceptAction   ? TEXT("ok") : TEXT("MISSING"));
		return;
	}

	// ⛔ Applied VERBATIM — no USiegeKeyboardLayoutSubsystem::GetPositionalContext call.
	// IMC_MainMenu carries no letter (KBD-§4), so there is nothing to retarget, and routing a
	// letterless context through the remap would only add a duplicate to reason about.
	InputSubsystem->AddMappingContext(MenuMappingContext, MenuMappingContextPriority);

	// ETriggerEvent::Started — the hero's idiom for one-shot actions (HeroCharacter.cpp:402,
	// :412, :435). A Boolean action with no explicit trigger fires Started ONCE on the
	// actuation edge, so a sustained injection (Aura's hold_seconds) or a held key is ONE
	// menu step, never a per-frame scroll.
	InputComponent->BindAction(MenuUpAction,     ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuUp);
	InputComponent->BindAction(MenuDownAction,   ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuDown);
	InputComponent->BindAction(MenuAcceptAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuAccept);
	bArmed = true;

	// The menu widget does not exist yet (OnWorldBeginPlay precedes GameMode->StartPlay,
	// World.cpp UWorld::BeginPlay); the first button is focused on the next tick.
	InWorld.GetTimerManager().SetTimerForNextTick(this, &USiegeMenuInputSubsystem::ApplyInitialFocus);

	// ─── TASK-1400 (MENU-REENTRY-FOCUS): THE RE-ENTRY RE-ARM ────────────────────────────────
	// 🧑 His words: "if you exit the deck builder and go back to the main menu, the outline is no
	// longer there ... exiting to the main menu results in that outline appearing at the top
	// option, not just when you start up the game."
	//
	// ⛔ The obvious fix -- re-fire against the menu we focused at boot -- WOULD NOT WORK, and that
	// is measured, not feared. BOTH return paths REPLACE the widget rather than re-showing it:
	//   • deck builder Exit  -- BP `CreateWidget(WBP_MainMenu_C) -> Is Valid ->
	//     AddToViewport(ZOrder 0) -> RemoveFromParent(self)`, no focus node (TASK-1399 §5.3);
	//   • `USessionMenuWidget::BackPressed` (`SessionMenuWidget.cpp:151-165`) -- the same shape.
	// ⇒ a re-arm that re-fires against a remembered pointer passes every static read and works
	// NOWHERE he actually goes.
	//
	// ⭐ WHY THIS SURVIVES A FRESH WIDGET: it holds no widget at all. `ApplyInitialFocus()` takes no
	// argument and caches nothing; `IsMenuUncovered()`, `GetMenuButtons()` and
	// `GetFocusedMenuButton()` each re-resolve through `GetAllWidgetsOfClass(..., TopLevelOnly)`
	// matched on the CLASS PATH string and filtered by `IsInViewport()`. A `RemoveFromParent`'d
	// instance drops out of that set; a freshly `AddToViewport`'d one appears in it. There is no
	// pointer to go stale -- the trigger is pointer-free BY CONSTRUCTION.
	//
	// ⛔ AND IT CANNOT STEAL FOCUS: `ApplyInitialFocus`'s own `!GetFocusedMenuButton()` guard is
	// LEFT EXACTLY AS IT WAS, so a repeat is a no-op whenever a menu button already holds focus,
	// and `IsMenuUncovered()` is false for the whole time any sub-screen is up. The only state this
	// poll ever acts in is "menu visible, uncovered, nothing on it focused" -- precisely 🧑 his gap.
	// ⚠️ Keeping that guard is also what keeps us out of TASK-1446's trap: `SetUserFocus`
	// early-returns false when the target is ALREADY focused (`SlateApplication.cpp:3028-3033`).
	//
	// ⛔ Armed HERE -- after the `L_MainMenu` map gate and the arming returns above -- so the timer
	// does not exist on any other map.
	InWorld.GetTimerManager().SetTimer(
		FocusReentryPollTimerHandle, this, &USiegeMenuInputSubsystem::ApplyInitialFocus,
		FocusReentryPollSeconds, /*bLoop=*/ true);

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] %s: IMC_MainMenu applied at priority %d on '%s'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started)."),
		MenuMapName, MenuMappingContextPriority, *PC->GetName());
}

void USiegeMenuInputSubsystem::Deinitialize()
{
	// The world is going away with its controller, input component and applied contexts;
	// nothing to unbind — the bindings live on the controller's component, the context on
	// the local player's subsystem, both torn down by their owners.
	//
	// ⛔ TASK-1400: the re-entry poll is the ONE thing here that does NOT die with the world.
	// `UWorld::GetTimerManager()` resolves to the OWNING GAME INSTANCE's manager, which survives
	// the travel off `L_MainMenu`, so the looping timer is cleared explicitly. (The delegate is
	// weak-bound to `this`, so a leaked loop could never call into a dead subsystem -- but it would
	// still sit on the game instance's manager, and that is worth one line to avoid.)
	// ⚠️ Safe here: `DoesSupportWorldType` admits Game | PIE only, so every world that reaches this
	// point has an owning game instance; and `ClearTimer` on an unset handle is a no-op, which is
	// the ordinary case (every map but the menu never armed it).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusReentryPollTimerHandle);
	}

	bArmed = false;
	MenuMappingContext = nullptr;
	MenuUpAction = nullptr;
	MenuDownAction = nullptr;
	MenuAcceptAction = nullptr;
	Super::Deinitialize();
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  READ API
// ═════════════════════════════════════════════════════════════════════════════════════════

APlayerController* USiegeMenuInputSubsystem::GetLocalController() const
{
	const UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return (PC && PC->IsLocalPlayerController()) ? PC : nullptr;
}

UUserWidget* USiegeMenuInputSubsystem::FindMainMenuWidget() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Top-level only (IsInViewport): the SessionMenu's standalone Back re-creates
	// WBP_MainMenu (SessionMenuWidget.cpp:151-164), so the instance is resolved LIVE on
	// every call and never cached across inputs.
	TArray<UUserWidget*> TopLevel;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, TopLevel, UUserWidget::StaticClass(), /*TopLevelOnly*/ true);
	for (UUserWidget* Widget : TopLevel)
	{
		if (Widget && Widget->GetClass() && Widget->GetClass()->GetPathName() == MainMenuWidgetClassPath)
		{
			return Widget;
		}
	}
	return nullptr;
}

bool USiegeMenuInputSubsystem::IsMenuUncovered() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	bool bMenuPresent = false;
	TArray<UUserWidget*> TopLevel;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, TopLevel, UUserWidget::StaticClass(), /*TopLevelOnly*/ true);
	for (const UUserWidget* Widget : TopLevel)
	{
		if (!Widget || !Widget->IsVisible())
		{
			continue;
		}
		if (Widget->GetClass() && Widget->GetClass()->GetPathName() == MainMenuWidgetClassPath)
		{
			bMenuPresent = true;
		}
		else
		{
			// Any other visible top-level widget (deck builder, settings, login, session
			// panel) sits ON TOP of the menu — the menu is covered; the handlers stay inert.
			return false;
		}
	}
	return bMenuPresent;
}

void USiegeMenuInputSubsystem::GetMenuButtons(TArray<UButton*>& OutButtons) const
{
	OutButtons.Reset();
	const UUserWidget* Menu = FindMainMenuWidget();
	if (!Menu || !Menu->WidgetTree)
	{
		return;
	}

	// ForEachWidget walks panel children in slot order, so buttons the Blueprint's Construct
	// added to VerticalBox_0 at runtime are visited top-to-bottom (Button_0..6).
	Menu->WidgetTree->ForEachWidget([&OutButtons](UWidget* Widget)
	{
		UButton* Button = Cast<UButton>(Widget);
		if (Button && Button->GetIsFocusable() && Button->GetIsEnabled() && Button->IsVisible())
		{
			OutButtons.Add(Button);
		}
	});
}

UButton* USiegeMenuInputSubsystem::GetFocusedMenuButton() const
{
	APlayerController* PC = GetLocalController();
	if (!PC)
	{
		return nullptr;
	}

	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	for (UButton* Button : Buttons)
	{
		if (Button->HasUserFocus(PC))
		{
			return Button;
		}
	}
	return nullptr;
}

FString USiegeMenuInputSubsystem::GetButtonLabel(const UButton* Button)
{
	if (!Button)
	{
		return FString();
	}

	// The shipped WBP_MainMenu button idiom is a UButton whose content is one UTextBlock
	// (AccountMenuWidget.cpp:331 "font 28" family). Walk the content subtree for the first
	// text block rather than assuming depth 1, so a wrapping panel does not blind the read.
	TArray<const UWidget*> Pending;
	for (int32 Index = 0; Index < Button->GetChildrenCount(); ++Index)
	{
		Pending.Add(Button->GetChildAt(Index));
	}
	while (Pending.Num() > 0)
	{
		const UWidget* Current = Pending.Pop(EAllowShrinking::No);
		if (!Current)
		{
			continue;
		}
		if (const UTextBlock* Text = Cast<const UTextBlock>(Current))
		{
			return Text->GetText().ToString();
		}
		if (const UPanelWidget* Panel = Cast<const UPanelWidget>(Current))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				Pending.Add(Panel->GetChildAt(Index));
			}
		}
	}
	return FString();
}

int32 USiegeMenuInputSubsystem::WrapIndex(int32 Current, int32 Delta, int32 Count)
{
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	// ((x % n) + n) % n: a true ring for negative steps too (Up from the top lands on the last).
	const int32 Raw = (Current + Delta) % Count;
	return (Raw + Count) % Count;
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  HANDLERS
// ═════════════════════════════════════════════════════════════════════════════════════════

void USiegeMenuInputSubsystem::HandleMenuUp()
{
	MoveFocus(-1);
}

void USiegeMenuInputSubsystem::HandleMenuDown()
{
	// TASK-1394 instrument. FIRST STATEMENT, unconditional, before MoveFocus is called: this is
	// the line that makes a SILENCE readable. Placed after any branch it would print nothing in
	// exactly the cases a reader must tell apart -- "the handler never ran" (the action did not
	// route here at all) vs "it ran and declined" (MoveFocus took an early exit). Same category
	// and verbosity as the already-proven-live IA_MenuAccept line below.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1)."));

	MoveFocus(+1);
}

void USiegeMenuInputSubsystem::MoveFocus(int32 Delta)
{
	if (!IsMenuUncovered())
	{
		// TASK-1394 instrument. Delta rides on EVERY exit line because MoveFocus is SHARED with
		// HandleMenuUp: a line that cannot tell Up from Down is not an instrument. The condition
		// itself is untouched -- the log is the whole of the addition.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d) declined: menu covered."), Delta);
		return;
	}

	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	if (Buttons.Num() == 0)
	{
		// TASK-1394 instrument.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d) declined: no menu buttons."), Delta);
		return;
	}

	// The current index is READ from Slate each time (never cached). Cold — no menu button
	// focused, e.g. focus on the viewport or on a widget the mouse touched — reads as 0, the
	// top; so from cold Down ×2 lands on index 2 ("Deck Builder"), the row's test.
	int32 Current = 0;
	if (const UButton* Focused = GetFocusedMenuButton())
	{
		Current = Buttons.IndexOfByKey(Focused);
	}

	const int32 Next = WrapIndex(Current, Delta, Buttons.Num());
	if (Buttons.IsValidIndex(Next))
	{
		// TASK-1394 instrument. Reported from the index MoveFocus CHOSE, never from FocusButton's
		// return value: FSlateApplication::SetUserFocus early-returns false when the target is
		// ALREADY focused (SlateApplication.cpp:3028-3033), so a false there is NOT a failure and
		// must never be logged as a refusal (TASK-1446).
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d): focus moved %d -> %d of %d ('%s')."),
			Delta, Current, Next, Buttons.Num(), *Buttons[Next]->GetName());
		FocusButton(Buttons[Next]);
	}
}

void USiegeMenuInputSubsystem::HandleMenuAccept()
{
	if (!IsMenuUncovered())
	{
		return;
	}

	UButton* Focused = GetFocusedMenuButton();
	if (!Focused)
	{
		// Cold Accept does nothing, deliberately: firing "Play (vs Bot)" on an Enter that
		// landed with no visible focus would travel to the arena with no indication of why.
		return;
	}

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."),
		*Focused->GetName(), *GetButtonLabel(Focused));

	// The SAME delegate a mouse click fires: UButton::SlateHandleClicked (Button.cpp:278-280)
	// is exactly `OnClicked.Broadcast()`. Every Blueprint handler bound on the button runs
	// unchanged — ⛔ no OnClicked body is touched by this feature.
	Focused->OnClicked.Broadcast();
}

void USiegeMenuInputSubsystem::ApplyInitialFocus()
{
	if (!IsMenuUncovered())
	{
		return;
	}

	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	if (Buttons.Num() > 0 && !GetFocusedMenuButton())
	{
		// ─── TASK-1400 deliverable (4) ──────────────────────────────────────────────────────
		// ⛔ UNCONDITIONAL AT ITS POINT: inside the branch that places focus, before the call, so
		// it reports the button this function CHOSE. It is never gated on `FocusButton`'s return
		// -- TASK-1446 measured that `FSlateApplication::SetUserFocus` early-returns false when the
		// target is ALREADY focused (`SlateApplication.cpp:3028-3033`), so a false there is not a
		// failure and must never be logged as one. Same category and verbosity as the Accept line
		// below, which TASK-1399 §3 proved live in 🧑 his own process.
		//
		// ⭐ WHY THE MENU INSTANCE NAME IS IN THE LINE, and it is the whole point of logging here:
		// this row's central claim is that the return paths build a BRAND-NEW `WBP_MainMenu`. A
		// boot placement therefore names one instance and a placement after an Exit names a
		// DIFFERENT one. ⇒ 🧑 his own sitting discriminates "re-armed on the fresh widget" from
		// "never re-armed" with no extra state and no instrumentation on his side -- two lines
		// naming two instances IS the measurement.
		//
		// ⚠️ A REPEAT IS SIGNAL, NOT SPAM: the guard above means a successful placement silences
		// the next poll. If this line repeats at the poll rate, the focus request is NOT taking --
		// which is exactly what a reader needs to see, and could not see before.
		//
		// ⚠️ SHIPPING: `Log` verbosity is compiled out entirely under Shipping
		// (`USE_LOGGING_IN_SHIPPING` = 0 => `NO_LOGGING` = 1; no Target.cs override in this
		// project -- documented at `SiegeAssistantGrammar.cpp:226-270`). In a packaged build this
		// line DOES NOT EXIST. The precedent Accept line shares the property; it is a precondition
		// on the READER, never a reason to change the verbosity.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] ApplyInitialFocus: focus placed on the TOP option '%s' (\"%s\"), index 0 of %d, in menu instance '%s'."),
			*Buttons[0]->GetName(), *GetButtonLabel(Buttons[0]), Buttons.Num(), *GetNameSafe(FindMainMenuWidget()));

		FocusButton(Buttons[0]);
	}
}

bool USiegeMenuInputSubsystem::FocusButton(UButton* Button) const
{
	APlayerController* PC = GetLocalController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Button || !LocalPlayer || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	TSharedPtr<SWidget> SlateButton = Button->GetCachedWidget();
	if (!SlateButton.IsValid() || !SlateButton->SupportsKeyboardFocus())
	{
		return false;
	}

	// EFocusCause::Navigation, NOT UWidget::SetUserFocus's SetDirectly: FSlateApplication::
	// SetUserFocus sets ShowFocus = (InCause == Navigation) (SlateApplication.cpp:3099), and
	// that flag is what makes SWidget::Paint draw the "FocusRectangle" brush (SWidget.cpp:1748)
	// — the visible focus state of deliverable (3). Same user-index resolution as
	// UWidget::SetUserFocus (Widget.cpp), same fallback to the local player's deferred
	// Slate operations if the immediate set is refused this frame.
	const int32 UserIndex = FSlateApplication::Get().GetUserIndexForController(LocalPlayer->GetControllerId());
	FReply& DelayedSlateOperations = LocalPlayer->GetSlateOperations();
	if (FSlateApplication::Get().SetUserFocus(UserIndex, SlateButton, EFocusCause::Navigation))
	{
		DelayedSlateOperations.CancelFocusRequest();
		return true;
	}

	DelayedSlateOperations.SetUserFocus(SlateButton.ToSharedRef(), EFocusCause::Navigation);
	return false;
}
