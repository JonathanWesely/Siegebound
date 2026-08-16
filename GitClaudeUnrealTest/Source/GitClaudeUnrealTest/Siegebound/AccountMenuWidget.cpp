// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccountMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SiegeAccountSubsystem.h"

namespace SiegeAccountMenuText
{
	// ------------------------------------------------------------------------
	// ALL PLAYER-FACING TEXT ON THIS SCREEN IS GAME-AUTHORED AND LIVES HERE.
	// The two chooser labels are Jonathan's exact two buttons, quoted
	// character-for-character from his request via ACC-§5.
	// ------------------------------------------------------------------------

	static const TCHAR* Title = TEXT("Account");

	static const TCHAR* CreateAccountLabel = TEXT("Create Account");
	static const TCHAR* LoginExistingLabel = TEXT("Log into existing account");

	static const TCHAR* SubmitCreateLabel = TEXT("Create Account");
	static const TCHAR* SubmitLoginLabel  = TEXT("Log In");
	static const TCHAR* LogoutLabel       = TEXT("Log Out");
	static const TCHAR* BackLabel         = TEXT("Back");

	static const TCHAR* NameHint            = TEXT("Display name");
	static const TCHAR* PasswordHint        = TEXT("Password");
	static const TCHAR* ConfirmPasswordHint = TEXT("Confirm password");

	/**
	 *  Per-mode default status lines. The numbers in the create line are the
	 *  ACC-§3 rules (name 3-24 chars, password 4+ chars) stated to the player
	 *  up front rather than discovered one rejection at a time. Honest per
	 *  ACC-§2: the create line says LOCAL and claims no security.
	 */
	static const TCHAR* ChooserStatus =
		TEXT("Profiles keep separate decks and settings on this machine.");
	static const TCHAR* CreateFormStatus =
		TEXT("New local profile: display name 3-24 characters, password at least 4 characters.");
	static const TCHAR* LoginFormStatus =
		TEXT("Enter the profile's display name and password.");
	// The LoggedIn line is the ACC-§5 pinned "Logged in as <DisplayName>". It
	// is a Printf literal at its one use site rather than a constant here,
	// because FString::Printf statically requires a TCHAR ARRAY literal - a
	// TCHAR* constant fails the engine's format-string static_assert.

	static const TCHAR* PasswordsDoNotMatch = TEXT("Passwords do not match.");

	/**
	 *  Shown INSTEAD of the mode status when the account subsystem cannot be
	 *  resolved. Worded to the actual fail-safe (ACC-§1): the game continues as
	 *  guest and nothing is gated, so the honest message is "you lose nothing",
	 *  not an error wall.
	 */
	static const TCHAR* Unavailable =
		TEXT("Accounts are unavailable right now. You are playing as a guest - decks and settings still save normally.");
}

TSharedRef<SWidget> UAccountMenuWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING (ACC-§5(b), the corrected TASK-444 shape).
	// UUserWidget::RebuildWidget() reads WidgetTree->RootWidget AS IT STANDS at
	// the moment it is called and returns an SSpacer when it is null (engine
	// source: UserWidget.cpp, UE 5.8). So the code-authored tree MUST be
	// constructed BEFORE Super::RebuildWidget(); building it afterwards yields
	// a silently EMPTY widget that still passes every property readback - the
	// exact defect class ACC-§5(e)'s human-pixel law exists for.
	//
	// Initialize() FIRST - the WARN-437-1 hardening, cloned. WidgetTree is
	// allocated INSIDE Initialize() (UserWidget.cpp:159-162), and
	// Super::RebuildWidget() self-heals an un-initialised widget only AFTER our
	// tree-building would already have bailed on a null WidgetTree. Initialize()
	// is public and idempotent (no-ops unless !bInitialized and not the CDO), so
	// the call is free; hardening, not a live bug.
	Initialize();
	ConstructAccountTree();
	return Super::RebuildWidget();
}

void UAccountMenuWidget::ConstructAccountTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeAccount, Error,
			TEXT("[AccountMenu] No WidgetTree - the account panel cannot build its tree."));
		return;
	}

	// ------------------------------------------------------------------------
	// THE ESCAPE HATCH (ACC-§5(c)). If an asset-authored tree exists (a future
	// /Game/UI/WBP_AccountMenu), it wins WHOLE: UMG has already resolved the
	// BindWidgetOptional members from it, so there is nothing to construct and
	// nothing to overwrite. Taking that fallback costs one art task and ZERO
	// C++ change - which is what makes the ruling a ruling, not a one-way door.
	// ------------------------------------------------------------------------
	if (WidgetTree->RootWidget != nullptr)
	{
		UE_LOG(LogSiegeAccount, Log,
			TEXT("[AccountMenu] An asset-authored tree is present - the code-authored branch is skipped (ACC-§5(c))."));
		return;
	}

	// ---- BackdropBorder: the modal plate, and the tree root -----------------
	if (BackdropBorder == nullptr)
	{
		BackdropBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackdropBorder"));
	}

	if (BackdropBorder == nullptr)
	{
		UE_LOG(LogSiegeAccount, Error,
			TEXT("[AccountMenu] Could not construct BackdropBorder - the account panel has no root."));
		return;
	}

	// ⛔ THIS LINE IS CORRECTNESS, NOT STYLING (ACC-§5). The panel is added ON
	// TOP of WBP_MainMenu without removing it; a HIT_TEST_INVISIBLE plate would
	// let clicks fall straight through into Play / Sandbox / Deck Builder /
	// QUIT while the panel looks modal - the settings lane's
	// click-through-into-Quit lesson, same geometry. Slate hit-tests on
	// VISIBILITY and geometry, not painted pixels: the dimming below is
	// appearance; the click blocking is this line. The owning UUserWidget stays
	// at its UMG default of SelfHitTestInvisible - the border is the child
	// doing the absorbing.
	BackdropBorder->SetVisibility(ESlateVisibility::Visible);

	// Appearance only - a dim plate so the menu underneath reads as inactive.
	// A future WBP_AccountMenu overrides all of this for free.
	BackdropBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	BackdropBorder->SetPadding(FMargin(0.f));
	BackdropBorder->SetHorizontalAlignment(HAlign_Center);
	BackdropBorder->SetVerticalAlignment(VAlign_Center);

	WidgetTree->RootWidget = BackdropBorder;

	// ---- RootPanel: the column ---------------------------------------------
	if (RootPanel == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
	}

	if (RootPanel == nullptr)
	{
		UE_LOG(LogSiegeAccount, Error,
			TEXT("[AccountMenu] Could not construct RootPanel - the account panel has no content column."));
		return;
	}

	BackdropBorder->SetContent(RootPanel);

	// ---- TitleText ----------------------------------------------------------
	if (TitleText == nullptr)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		if (TitleText != nullptr)
		{
			TitleText->SetText(FText::FromString(FString(SiegeAccountMenuText::Title)));
			TitleText->SetFontSize(36.f);

			if (UVerticalBoxSlot* TitleSlot = RootPanel->AddChildToVerticalBox(TitleText))
			{
				TitleSlot->SetPadding(FMargin(24.f, 24.f, 24.f, 8.f));
				TitleSlot->SetHorizontalAlignment(HAlign_Center);
				TitleSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- StatusText ---------------------------------------------------------
	// One surface carries the per-mode guidance, every submit-failure
	// OutReason, "Logged in as <DisplayName>", and the unavailable
	// explanation - so the player is never rejected silently (ACC-§5).
	if (StatusText == nullptr)
	{
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		if (StatusText != nullptr)
		{
			StatusText->SetFontSize(18.f);
			StatusText->SetAutoWrapText(true);
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.f)));

			if (UVerticalBoxSlot* StatusSlot = RootPanel->AddChildToVerticalBox(StatusText))
			{
				StatusSlot->SetPadding(FMargin(24.f, 0.f, 24.f, 16.f));
				StatusSlot->SetHorizontalAlignment(HAlign_Fill);
				StatusSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- The chooser: Jonathan's two named buttons --------------------------
	ConstructPanelButton(CreateAccountButton, CreateAccountLabelText,
		TEXT("CreateAccountButton"), TEXT("CreateAccountLabelText"), SiegeAccountMenuText::CreateAccountLabel);
	ConstructPanelButton(LoginExistingButton, LoginExistingLabelText,
		TEXT("LoginExistingButton"), TEXT("LoginExistingLabelText"), SiegeAccountMenuText::LoginExistingLabel);

	// ---- The form inputs ----------------------------------------------------
	ConstructInputBox(NameInputBox, TEXT("NameInputBox"), SiegeAccountMenuText::NameHint, /*bIsPassword=*/false);
	ConstructInputBox(PasswordInputBox, TEXT("PasswordInputBox"), SiegeAccountMenuText::PasswordHint, /*bIsPassword=*/true);
	ConstructInputBox(ConfirmPasswordInputBox, TEXT("ConfirmPasswordInputBox"), SiegeAccountMenuText::ConfirmPasswordHint, /*bIsPassword=*/true);

	// ---- Submit / Logout ----------------------------------------------------
	// SubmitLabelText's string is per-mode; ApplyMode owns it. The constructed
	// default is the create label only so the block is never empty.
	ConstructPanelButton(SubmitButton, SubmitLabelText,
		TEXT("SubmitButton"), TEXT("SubmitLabelText"), SiegeAccountMenuText::SubmitCreateLabel);
	ConstructPanelButton(LogoutButton, LogoutLabelText,
		TEXT("LogoutButton"), TEXT("LogoutLabelText"), SiegeAccountMenuText::LogoutLabel);

	// ---- BackButton, last and unconditional ---------------------------------
	ConstructPanelButton(BackButton, BackLabelText,
		TEXT("BackButton"), TEXT("BackLabelText"), SiegeAccountMenuText::BackLabel);

	if (BackButton == nullptr)
	{
		// This one IS worth an Error: without Back the only way out of a modal
		// backdrop is to quit the game.
		UE_LOG(LogSiegeAccount, Error,
			TEXT("[AccountMenu] Could not construct BackButton - the account panel cannot be dismissed from itself."));
	}
}

void UAccountMenuWidget::ConstructPanelButton(TObjectPtr<UButton>& ButtonMember, TObjectPtr<UTextBlock>& LabelMember,
	const TCHAR* ButtonName, const TCHAR* LabelName, const TCHAR* LabelString)
{
	// Construct-only-if-null per member (ACC-§5(b)'s escape-hatch semantics) -
	// an asset-authored partial tree keeps whatever it authored.
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	if (LabelMember == nullptr)
	{
		LabelMember = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		if (LabelMember != nullptr)
		{
			LabelMember->SetText(FText::FromString(FString(LabelString)));
			// The shipped WBP_MainMenu button family idiom (font 28,
			// content padding 24/12/24/12, HAlign_Fill row) so the overlay and
			// the menu underneath read as one family.
			LabelMember->SetFontSize(28.f);
		}
	}

	if (ButtonMember == nullptr)
	{
		ButtonMember = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		if (ButtonMember != nullptr)
		{
			if (LabelMember != nullptr)
			{
				if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(ButtonMember->SetContent(LabelMember)))
				{
					ContentSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 12.f));
					ContentSlot->SetHorizontalAlignment(HAlign_Center);
					ContentSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UVerticalBoxSlot* RowSlot = RootPanel->AddChildToVerticalBox(ButtonMember))
			{
				RowSlot->SetPadding(FMargin(24.f, 8.f, 24.f, 8.f));
				RowSlot->SetHorizontalAlignment(HAlign_Fill);
				RowSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (ButtonMember == nullptr)
	{
		// Not fatal - the panel must never trap; Back is handled separately.
		UE_LOG(LogSiegeAccount, Error,
			TEXT("[AccountMenu] Could not construct button '%s'."), ButtonName);
	}
}

void UAccountMenuWidget::ConstructInputBox(TObjectPtr<UEditableTextBox>& BoxMember,
	const TCHAR* BoxName, const TCHAR* HintString, bool bIsPassword)
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	if (BoxMember == nullptr)
	{
		BoxMember = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), BoxName);
		if (BoxMember != nullptr)
		{
			BoxMember->SetHintText(FText::FromString(FString(HintString)));

			if (UVerticalBoxSlot* RowSlot = RootPanel->AddChildToVerticalBox(BoxMember))
			{
				RowSlot->SetPadding(FMargin(24.f, 4.f, 24.f, 4.f));
				RowSlot->SetHorizontalAlignment(HAlign_Fill);
				RowSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (BoxMember != nullptr && bIsPassword)
	{
		// ACC-§5's pinned requirement on the code-authored path: a password
		// field that echoes its characters is a correctness bug on this panel,
		// not a style choice. (An asset-authored tree never reaches this
		// function - the escape hatch returns first - so a future
		// WBP_AccountMenu owns its own IsPassword flags, which ACC-§5 pins on
		// it by name.)
		BoxMember->SetIsPassword(true);
	}

	if (BoxMember == nullptr)
	{
		UE_LOG(LogSiegeAccount, Error,
			TEXT("[AccountMenu] Could not construct input box '%s' - its form cannot be submitted usefully."), BoxName);
	}
}

void UAccountMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Everything is bound HERE and not in NativeOnInitialized, deliberately and
	// for one mechanical reason (the settings-lane §11 lifecycle trap, cloned):
	// the code-authored children do not exist until RebuildWidget() runs, and
	// the engine order is
	//   Initialize() -> NativeOnInitialized() -> RebuildWidget() -> NativeConstruct()
	// (UserWidget.cpp: NativeConstruct is called from OnWidgetRebuilt). Binding
	// in NativeOnInitialized would silently bind nothing on the code-authored
	// path while working fine on a future WBP path. One binding site covers
	// BOTH paths.
	if (CreateAccountButton != nullptr)
	{
		CreateAccountButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleCreateAccountClicked);
	}
	if (LoginExistingButton != nullptr)
	{
		LoginExistingButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleLoginExistingClicked);
	}
	if (SubmitButton != nullptr)
	{
		SubmitButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleSubmitClicked);
	}
	if (LogoutButton != nullptr)
	{
		LogoutButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleLogoutClicked);
	}

	// Back is bound unconditionally and LAST: it must work even when the
	// account subsystem is missing and every form above is dead. A panel you
	// cannot leave is worse than a panel that cannot log anyone in.
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleBackClicked);
	}

	// Track login-state changes made from anywhere while the panel is open
	// (the ACC-§7 pinned no-param delegate).
	if (USiegeAccountSubsystem* Account = ResolveAccountSubsystem())
	{
		Account->OnActiveProfileChanged.AddUniqueDynamic(this, &UAccountMenuWidget::HandleActiveProfileChanged);
	}

	// Seed the mode LAST, after every binding is armed: opening while logged
	// in lands on LoggedIn (ACC-§5); guest lands on the Chooser; an
	// unresolvable subsystem lands on the disabled fail-safe.
	RefreshModeFromSubsystem();
}

void UAccountMenuWidget::NativeDestruct()
{
	// Symmetric unbind (defensive - dynamic delegates tolerate dead objects,
	// but a dismissed panel must not keep reacting to profile broadcasts).
	if (USiegeAccountSubsystem* Account = ResolveAccountSubsystem())
	{
		Account->OnActiveProfileChanged.RemoveDynamic(this, &UAccountMenuWidget::HandleActiveProfileChanged);
	}

	if (CreateAccountButton != nullptr)
	{
		CreateAccountButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleCreateAccountClicked);
	}
	if (LoginExistingButton != nullptr)
	{
		LoginExistingButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleLoginExistingClicked);
	}
	if (SubmitButton != nullptr)
	{
		SubmitButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleSubmitClicked);
	}
	if (LogoutButton != nullptr)
	{
		LogoutButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleLogoutClicked);
	}
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleBackClicked);
	}

	Super::NativeDestruct();
}

void UAccountMenuWidget::CreateAccountChosen()
{
	if (ResolveAccountSubsystem() == nullptr)
	{
		ShowUnavailable();
		return;
	}

	ApplyMode(EAccountMenuMode::CreateForm);
}

void UAccountMenuWidget::LoginExistingChosen()
{
	if (ResolveAccountSubsystem() == nullptr)
	{
		ShowUnavailable();
		return;
	}

	ApplyMode(EAccountMenuMode::LoginForm);
}

void UAccountMenuWidget::SubmitPressed()
{
	if (CurrentMode != EAccountMenuMode::CreateForm && CurrentMode != EAccountMenuMode::LoginForm)
	{
		// The submit button is hidden outside the two forms; reaching here
		// means a Blueprint called the wrapper out of turn. Harmless no-op.
		UE_LOG(LogSiegeAccount, Verbose, TEXT("[AccountMenu] SubmitPressed outside a form mode - ignored."));
		return;
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	if (Account == nullptr)
	{
		ShowUnavailable();
		return;
	}

	const FString DisplayName = (NameInputBox != nullptr) ? NameInputBox->GetText().ToString() : FString();

	// ⛔ ACC-§2. The plaintext password lives in these LOCALS for the duration
	// of this call only: it is handed to the subsystem (which hashes and drops
	// it), it is NEVER logged, NEVER stored in a member, NEVER placed in a
	// status string, and both password boxes are cleared before every return
	// below. OutReason is safe to display and log - TASK-600's contract
	// guarantees the password never appears in it.
	const FString Password = (PasswordInputBox != nullptr) ? PasswordInputBox->GetText().ToString() : FString();

	if (CurrentMode == EAccountMenuMode::CreateForm)
	{
		const FString ConfirmPassword =
			(ConfirmPasswordInputBox != nullptr) ? ConfirmPasswordInputBox->GetText().ToString() : FString();

		// Widget-side check only - the subsystem's signature takes ONE
		// password (ACC-§7), so the confirm box is this panel's job.
		// Case-sensitive on purpose: passwords compare exactly.
		if (!Password.Equals(ConfirmPassword, ESearchCase::CaseSensitive))
		{
			ClearPasswordBoxes();
			ShowStatus(FString(SiegeAccountMenuText::PasswordsDoNotMatch));
			return;
		}

		FString OutReason;
		const bool bCreated = Account->CreateAccount(DisplayName, Password, OutReason);
		ClearPasswordBoxes();

		if (!bCreated)
		{
			UE_LOG(LogSiegeAccount, Log, TEXT("[AccountMenu] Create Account rejected: %s"), *OutReason);
			ShowStatus(OutReason);
			return;
		}

		UE_LOG(LogSiegeAccount, Log, TEXT("[AccountMenu] Created and logged into profile '%s'."),
			*Account->GetActiveDisplayName());

		// The subsystem's OnActiveProfileChanged broadcast has already landed
		// on HandleActiveProfileChanged during the call above; this direct
		// apply is belt-and-braces for an unbound-delegate world and is
		// idempotent (ApplyMode + the ShowStatus no-op suppression).
		ApplyMode(EAccountMenuMode::LoggedIn);
	}
	else // LoginForm
	{
		FString OutReason;
		const bool bLoginOk = Account->Login(DisplayName, Password, OutReason);
		ClearPasswordBoxes();

		if (!bLoginOk)
		{
			UE_LOG(LogSiegeAccount, Log, TEXT("[AccountMenu] Login rejected: %s"), *OutReason);
			ShowStatus(OutReason);
			return;
		}

		UE_LOG(LogSiegeAccount, Log, TEXT("[AccountMenu] Logged into profile '%s'."),
			*Account->GetActiveDisplayName());

		ApplyMode(EAccountMenuMode::LoggedIn);
	}
}

void UAccountMenuWidget::LogoutPressed()
{
	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	if (Account == nullptr)
	{
		ShowUnavailable();
		return;
	}

	UE_LOG(LogSiegeAccount, Log, TEXT("[AccountMenu] Logging out of profile '%s'."),
		*Account->GetActiveDisplayName());

	// Broadcasts OnActiveProfileChanged -> HandleActiveProfileChanged lands the
	// panel on the Chooser; the direct apply below is the same belt-and-braces
	// as SubmitPressed's.
	Account->Logout();
	ApplyMode(EAccountMenuMode::Chooser);
}

void UAccountMenuWidget::BackPressed()
{
	// ⛔ RemoveFromParent(self) AND NOTHING ELSE, in every mode (ACC-§5).
	// TASK-607 adds this panel ON TOP of WBP_MainMenu with
	// AddToViewport(ZOrder 10) and never removes the menu, so the menu
	// underneath is already alive and already correct. Re-creating it would
	// put navigation state in the leaf plus a soft asset path to get wrong.
	UE_LOG(LogSiegeAccount, Log, TEXT("[AccountMenu] Back pressed - dismissing the account panel only."));
	RemoveFromParent();
}

void UAccountMenuWidget::HandleCreateAccountClicked()
{
	CreateAccountChosen();
}

void UAccountMenuWidget::HandleLoginExistingClicked()
{
	LoginExistingChosen();
}

void UAccountMenuWidget::HandleSubmitClicked()
{
	SubmitPressed();
}

void UAccountMenuWidget::HandleLogoutClicked()
{
	LogoutPressed();
}

void UAccountMenuWidget::HandleBackClicked()
{
	BackPressed();
}

void UAccountMenuWidget::HandleActiveProfileChanged()
{
	// The pinned delegate carries no payload (ACC-§7) - re-derive everything
	// from the subsystem rather than trusting call order.
	RefreshModeFromSubsystem();
}

void UAccountMenuWidget::RefreshModeFromSubsystem()
{
	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	if (Account == nullptr)
	{
		// Land on the chooser layout, then disable it with the explanation -
		// the panel stays legible and Back stays live (ACC-§1 fail-safe).
		ApplyMode(EAccountMenuMode::Chooser);
		ShowUnavailable();
		return;
	}

	SetFormsEnabled(true);
	ApplyMode(Account->IsLoggedIn() ? EAccountMenuMode::LoggedIn : EAccountMenuMode::Chooser);
}

void UAccountMenuWidget::ApplyMode(EAccountMenuMode NewMode)
{
	CurrentMode = NewMode;

	const bool bChooser  = (NewMode == EAccountMenuMode::Chooser);
	const bool bCreate   = (NewMode == EAccountMenuMode::CreateForm);
	const bool bLogin    = (NewMode == EAccountMenuMode::LoginForm);
	const bool bLoggedIn = (NewMode == EAccountMenuMode::LoggedIn);

	// Collapsed (not Hidden) so hidden rows release their layout space and
	// each mode reads as its own screen. TitleText, StatusText and BackButton
	// are visible in every mode and are never touched here.
	auto SetShown = [](UWidget* Widget, bool bShown)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	};

	SetShown(CreateAccountButton, bChooser);
	SetShown(LoginExistingButton, bChooser);
	SetShown(NameInputBox, bCreate || bLogin);
	SetShown(PasswordInputBox, bCreate || bLogin);
	SetShown(ConfirmPasswordInputBox, bCreate);
	SetShown(SubmitButton, bCreate || bLogin);
	SetShown(LogoutButton, bLoggedIn);

	if (SubmitLabelText != nullptr)
	{
		SubmitLabelText->SetText(FText::FromString(
			FString(bCreate ? SiegeAccountMenuText::SubmitCreateLabel : SiegeAccountMenuText::SubmitLoginLabel)));
	}

	// ACC-§2 hygiene: no typed password survives a mode change.
	ClearPasswordBoxes();

	FString Status;
	switch (NewMode)
	{
	case EAccountMenuMode::CreateForm:
		Status = SiegeAccountMenuText::CreateFormStatus;
		break;

	case EAccountMenuMode::LoginForm:
		Status = SiegeAccountMenuText::LoginFormStatus;
		break;

	case EAccountMenuMode::LoggedIn:
	{
		// The ACC-§5 pinned line: "Logged in as <DisplayName>".
		USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
		const FString DisplayName = (Account != nullptr) ? Account->GetActiveDisplayName() : FString();
		Status = FString::Printf(TEXT("Logged in as %s"), *DisplayName);
		break;
	}

	case EAccountMenuMode::Chooser:
	default:
		Status = SiegeAccountMenuText::ChooserStatus;
		break;
	}

	ShowStatus(Status);
}

void UAccountMenuWidget::ShowStatus(const FString& StatusMessage)
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::FromString(StatusMessage));
	}

	// Presentation event for a future WBP_AccountMenu. The mode enum never
	// crosses (condition (d)); the name string does.
	const TCHAR* ModeName = TEXT("Chooser");
	switch (CurrentMode)
	{
	case EAccountMenuMode::CreateForm: ModeName = TEXT("CreateForm"); break;
	case EAccountMenuMode::LoginForm:  ModeName = TEXT("LoginForm");  break;
	case EAccountMenuMode::LoggedIn:   ModeName = TEXT("LoggedIn");   break;
	case EAccountMenuMode::Chooser:
	default:                           ModeName = TEXT("Chooser");    break;
	}

	// The delegate law, cloned from the settings lane: notify on a real
	// change (and on the first seed), never on a no-op. This is what makes the
	// success path's belt-and-braces ApplyMode after the subsystem's own
	// broadcast cost zero duplicate events.
	const FString ModeNameString(ModeName);
	const bool bIsNew = !bHasNotifiedState
		|| !ModeNameString.Equals(LastNotifiedModeName, ESearchCase::CaseSensitive)
		|| !StatusMessage.Equals(LastNotifiedStatus, ESearchCase::CaseSensitive);

	LastNotifiedModeName = ModeNameString;
	LastNotifiedStatus = StatusMessage;
	bHasNotifiedState = true;

	if (bIsNew)
	{
		OnAccountMenuStateChanged(ModeNameString, StatusMessage);
	}
}

void UAccountMenuWidget::ShowUnavailable()
{
	// LOG ONCE per widget instance - this is reachable from the seed, from
	// every click and from a broadcast, and a panel that spams the log every
	// time the player pokes a dead button is a panel nobody reads the log of.
	if (!bLoggedSubsystemUnavailable)
	{
		bLoggedSubsystemUnavailable = true;
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[AccountMenu] USiegeAccountSubsystem could not be resolved - the account forms are disabled. The game continues as guest (ACC-§1 fail-safe)."));
	}

	SetFormsEnabled(false);
	ShowStatus(FString(SiegeAccountMenuText::Unavailable));
}

void UAccountMenuWidget::SetFormsEnabled(bool bEnabled)
{
	auto SetEnabled = [bEnabled](UWidget* Widget)
	{
		if (Widget != nullptr)
		{
			Widget->SetIsEnabled(bEnabled);
		}
	};

	SetEnabled(CreateAccountButton);
	SetEnabled(LoginExistingButton);
	SetEnabled(NameInputBox);
	SetEnabled(PasswordInputBox);
	SetEnabled(ConfirmPasswordInputBox);
	SetEnabled(SubmitButton);
	SetEnabled(LogoutButton);

	// BackButton is deliberately NOT in this list, in either direction: it must
	// stay live when everything else is dead.
}

void UAccountMenuWidget::ClearPasswordBoxes()
{
	if (PasswordInputBox != nullptr)
	{
		PasswordInputBox->SetText(FText::GetEmpty());
	}
	if (ConfirmPasswordInputBox != nullptr)
	{
		ConfirmPasswordInputBox->SetText(FText::GetEmpty());
	}
}

USiegeAccountSubsystem* UAccountMenuWidget::ResolveAccountSubsystem() const
{
	// Null-safe at every hop - the shipped ResolveSettingsSubsystem shape,
	// cloned. The subsystem lives on the GAME INSTANCE so login state survives
	// the L_MainMenu -> L_Arena travel (ACC-§4).
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeAccountSubsystem>() : nullptr;
}
