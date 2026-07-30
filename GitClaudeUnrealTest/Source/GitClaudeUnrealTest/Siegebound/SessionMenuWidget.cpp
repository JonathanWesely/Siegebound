// Copyright Epic Games, Inc. All Rights Reserved.

#include "SessionMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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

	// Plain standalone menu: Back is WBP-side panel navigation. Deliberately
	// NOT LeaveMatch here - reloading L_MainMenu on every Back press would
	// flicker-reset the menu for no reason (flagged in the TASK-354 handoff).
	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - no session active; WBP handles panel dismissal."));
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
