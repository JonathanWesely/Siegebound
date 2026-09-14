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

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] %s: IMC_MainMenu applied at priority %d on '%s'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started)."),
		MenuMapName, MenuMappingContextPriority, *PC->GetName());
}

void USiegeMenuInputSubsystem::Deinitialize()
{
	// The world is going away with its controller, input component and applied contexts;
	// nothing to unbind — the bindings live on the controller's component, the context on
	// the local player's subsystem, both torn down by their owners.
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
	MoveFocus(+1);
}

void USiegeMenuInputSubsystem::MoveFocus(int32 Delta)
{
	if (!IsMenuUncovered())
	{
		return;
	}

	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	if (Buttons.Num() == 0)
	{
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
