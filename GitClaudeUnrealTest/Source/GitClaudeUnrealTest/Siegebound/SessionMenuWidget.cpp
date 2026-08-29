// Copyright Epic Games, Inc. All Rights Reserved.

#include "SessionMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"  // TASK-680: CreateWidget's owner static_assert needs the complete type
#include "UObject/UObjectGlobals.h"          // TASK-680: LoadClass<> (explicit IWYU - no compile verifies transitive pulls)
#include "SiegeSessionSubsystem.h"

void USessionMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Auto-wire the OPTIONAL named buttons once per widget instance. A WBP
	// that skips these names wires its own buttons to the three BlueprintCallable
	// wrappers instead - both routes are supported (class comment contract).
	if (HostButton)
	{
		HostButton->OnClicked.AddUniqueDynamic(this, &USessionMenuWidget::HandleHostClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddUniqueDynamic(this, &USessionMenuWidget::HandleJoinClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USessionMenuWidget::HandleBackClicked);
	}
}

void USessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind the subsystem text surfaces for the on-screen lifetime of the menu.
	// AddUniqueDynamic keeps repeated construct cycles single-bound.
	if (USiegeSessionSubsystem* Session = ResolveSessionSubsystem())
	{
		Session->OnSessionStatus.AddUniqueDynamic(this, &USessionMenuWidget::HandleSessionStatus);
		Session->OnSessionError.AddUniqueDynamic(this, &USessionMenuWidget::HandleSessionError);
	}
	else
	{
		// No crash, no dead button mystery: the menu says why it cannot work.
		ShowLocalError(TEXT("Session system unavailable."));
	}
}

void USessionMenuWidget::NativeDestruct()
{
	// Symmetric unbind (defensive - dynamic delegates tolerate dead objects,
	// but a destructed menu must not keep receiving session text).
	if (USiegeSessionSubsystem* Session = ResolveSessionSubsystem())
	{
		Session->OnSessionStatus.RemoveDynamic(this, &USessionMenuWidget::HandleSessionStatus);
		Session->OnSessionError.RemoveDynamic(this, &USessionMenuWidget::HandleSessionError);
	}

	Super::NativeDestruct();
}

void USessionMenuWidget::HostPressed()
{
	USiegeSessionSubsystem* Session = ResolveSessionSubsystem();
	if (!Session)
	{
		ShowLocalError(TEXT("Session system unavailable."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Host pressed."));
	Session->HostListenMatch();
}

void USessionMenuWidget::JoinPressed(const FString& AddressText)
{
	USiegeSessionSubsystem* Session = ResolveSessionSubsystem();
	if (!Session)
	{
		ShowLocalError(TEXT("Session system unavailable."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Join pressed (address text '%s')."), *AddressText);
	// Validation is the subsystem's job: an invalid address returns through
	// OnSessionError -> HandleSessionError -> OnSessionErrorShown, no travel.
	Session->JoinMatch(AddressText);
}

void USessionMenuWidget::BackPressed()
{
	USiegeSessionSubsystem* Session = ResolveSessionSubsystem();
	if (!Session)
	{
		ShowLocalError(TEXT("Session system unavailable."));
		return;
	}

	const UWorld* World = GetWorld();
	const bool bNetActive = World && (World->GetNetMode() != NM_Standalone);

	// A join whose connection is still PENDING keeps the menu world standalone
	// - detect it via the world context so Back can cancel it (doc section 5:
	// leave is the universal bail-out; gate (f) never-hang).
	bool bPendingConnection = false;
	if (GEngine && World)
	{
		if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
		{
			bPendingConnection = (WorldContext->PendingNetGame != nullptr);
		}
	}

	if (bNetActive || bPendingConnection)
	{
		UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - leaving session (NetActive=%d, PendingConnection=%d)."),
			bNetActive ? 1 : 0, bPendingConnection ? 1 : 0);
		Session->LeaveMatch();
		return;
	}

	// Plain standalone menu: Back dismisses the panel HERE, C++-side.
	// SUPERSEDED RATIONALE (2026-08-28, the SESSION-BACK ruling in
	// CONVENTIONS, off VID-002): the TASK-354 flagged decision read "Back is
	// WBP-side panel navigation", but the TASK-355 route-(A) zero-graph WBP
	// never authored that navigation - measured on pixels, three full press
	// cycles rendered and nobody closed the panel. The decision is REVERSED.
	// The half of the old rationale that SURVIVES is the LeaveMatch refusal:
	// reloading L_MainMenu on every standalone Back press would flicker-reset
	// the menu for no reason, so dismissal is a viewport widget swap, never
	// travel.
	//
	// The swap is the exact INVERSE of the TASK-355 open transition (the
	// main-menu Multiplayer entry: RemoveFromParent(self) ->
	// CreateWidget(WBP_SessionMenu_C) -> AddToViewport - the main menu is
	// fully REMOVED from the viewport, not hidden, so Back must re-create
	// it). Add-before-remove is LAW: the main menu enters the viewport BEFORE
	// this panel leaves it, so no frame renders with neither widget; a failed
	// class resolve leaves THIS panel up (ShowLocalError - one log, one error
	// line), never a zero-UI strand. Deliberately NO SetInputMode on any
	// path: L_MainMenu's posture (UIOnly + visible cursor) is owned by
	// BP_MenuGameMode at level boot (Input-mode ownership law) and survives
	// viewport widget swaps - the open transition made no input-mode call
	// either, and VID-002 shows the swapped-in panel fully hover- and
	// click-interactive under the surviving posture.
	UClass* MainMenuClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C"));
	APlayerController* OwningPlayer = GetOwningPlayer();
	UUserWidget* MainMenu = (MainMenuClass && OwningPlayer)
		? CreateWidget<UUserWidget>(OwningPlayer, MainMenuClass)
		: nullptr;
	if (!MainMenu)
	{
		// Panel STAYS up - the player keeps a live, clickable UI.
		ShowLocalError(TEXT("Main menu unavailable."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - no session active; returning to main menu."));
	MainMenu->AddToViewport();
	RemoveFromParent();
}

void USessionMenuWidget::HandleHostClicked()
{
	HostPressed();
}

void USessionMenuWidget::HandleJoinClicked()
{
	const FString Address = AddressTextBox ? AddressTextBox->GetText().ToString() : FString();
	JoinPressed(Address);
}

void USessionMenuWidget::HandleBackClicked()
{
	BackPressed();
}

void USessionMenuWidget::HandleSessionStatus(const FString& Message)
{
	if (StatusTextBlock)
	{
		StatusTextBlock->SetText(FText::FromString(Message));
	}
	if (ErrorTextBlock)
	{
		// A fresh status supersedes a stale error line (e.g. a corrected IP
		// after a rejected one) - only the bound widget is cleared; BIE
		// consumers decide their own presentation.
		ErrorTextBlock->SetText(FText::GetEmpty());
	}
	OnSessionStatusUpdated(Message);
}

void USessionMenuWidget::HandleSessionError(const FString& Message)
{
	if (ErrorTextBlock)
	{
		ErrorTextBlock->SetText(FText::FromString(Message));
	}
	OnSessionErrorShown(Message);
}

USiegeSessionSubsystem* USessionMenuWidget::ResolveSessionSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeSessionSubsystem>() : nullptr;
}

void USessionMenuWidget::ShowLocalError(const FString& Message)
{
	UE_LOG(LogSiegeNet, Warning, TEXT("[SessionMenu] %s"), *Message);
	if (ErrorTextBlock)
	{
		ErrorTextBlock->SetText(FText::FromString(Message));
	}
	OnSessionErrorShown(Message);
}
