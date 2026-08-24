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
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SiegeAccountSubsystem.h"
#include "SiegeCloudClient.h"
#include "SiegeCloudSync.h"

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

	// ---- P2 cloud block (TASK-646, ACC-§14) --------------------------------
	static const TCHAR* LinkCloudLabel  = TEXT("Link to Cloud");
	static const TCHAR* SyncNowLabel    = TEXT("Sync Now");
	static const TCHAR* SubmitLinkLabel = TEXT("Link to Cloud");

	static const TCHAR* EmailHint                = TEXT("Email address");
	static const TCHAR* CloudConfirmPasswordHint = TEXT("Confirm password (new cloud accounts only)");

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
	 *  P2 (TASK-646) cloud-lane text. Honest per ACC-§2/§11: nothing here calls
	 *  anything "secure" or "encrypted"; the unconfigured line says the exact
	 *  ACC-§11 truth - cloud OFF means Phase-1 behavior, nothing is lost. The
	 *  CloudLinkForm status states the confirm-box convention up front (filled
	 *  = new cloud account, blank = existing) instead of letting the player
	 *  discover it one rejection at a time.
	 */
	static const TCHAR* CloudLinkFormStatus =
		TEXT("Link this profile to the cloud: enter your cloud email and password. Fill the confirm box to create a NEW cloud account, or leave it blank to sign into an existing one.");
	static const TCHAR* CloudEnterEmail    = TEXT("Enter the email address for the cloud account.");
	static const TCHAR* CloudEnterPassword = TEXT("Enter the cloud account's password.");
	static const TCHAR* CloudNotConfigured =
		TEXT("Cloud sync is not set up on this machine, so the cloud buttons are disabled. Decks and settings still save locally - nothing is lost.");
	static const TCHAR* CloudNotLinked =
		TEXT("Not linked to the cloud. Link to back up this profile's decks and settings.");
	static const TCHAR* CloudBusyLinking = TEXT("Contacting the cloud...");
	static const TCHAR* CloudBusySyncing = TEXT("Syncing with the cloud...");
	static const TCHAR* CloudUploadingAfterLink =
		TEXT("Cloud account linked - uploading this profile's decks and settings...");
	static const TCHAR* CloudPullingAfterLink =
		TEXT("Cloud account linked - pulling your cloud decks and settings...");
	static const TCHAR* CloudNoUserId =
		TEXT("The cloud accepted the sign-in but returned no user id. Nothing was linked - try again.");

	// ---- P2.1 re-auth wire (TASK-653, rider R1 - ACC-§15 P2.1) -------------
	// The failure line is the seam law's own pinned wording ("cloud session
	// expired — sign in again to re-link"), carried in this file's established
	// ASCII-hyphen posture for player-visible literals (every shipped string
	// above uses '-'; the em dash lives only in comments) - declared in the
	// TASK-653 handoff, not silently transformed.
	static const TCHAR* CloudBusyRestoringSession = TEXT("Restoring your cloud session...");
	static const TCHAR* CloudSessionExpired =
		TEXT("cloud session expired - sign in again to re-link");
	// The linked steady-state line is the TASK-646 pinned "Linked as <email>".
	// Like the P1 LoggedIn line it is a Printf literal at its use sites - a
	// TCHAR* constant fails the engine's format-string static_assert.

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
	// EmailInputBox (P2, ACC-§14) rides between the name row and the two
	// password rows; CloudLinkForm shows email + the REUSED password boxes
	// (ACC-§14: no duplicate password children), the P1 forms collapse it.
	ConstructInputBox(NameInputBox, TEXT("NameInputBox"), SiegeAccountMenuText::NameHint, /*bIsPassword=*/false);
	ConstructInputBox(EmailInputBox, TEXT("EmailInputBox"), SiegeAccountMenuText::EmailHint, /*bIsPassword=*/false);
	ConstructInputBox(PasswordInputBox, TEXT("PasswordInputBox"), SiegeAccountMenuText::PasswordHint, /*bIsPassword=*/true);
	ConstructInputBox(ConfirmPasswordInputBox, TEXT("ConfirmPasswordInputBox"), SiegeAccountMenuText::ConfirmPasswordHint, /*bIsPassword=*/true);

	// ---- Submit / Logout ----------------------------------------------------
	// SubmitLabelText's string is per-mode; ApplyMode owns it. The constructed
	// default is the create label only so the block is never empty.
	ConstructPanelButton(SubmitButton, SubmitLabelText,
		TEXT("SubmitButton"), TEXT("SubmitLabelText"), SiegeAccountMenuText::SubmitCreateLabel);
	ConstructPanelButton(LogoutButton, LogoutLabelText,
		TEXT("LogoutButton"), TEXT("LogoutLabelText"), SiegeAccountMenuText::LogoutLabel);

	// ---- The P2 cloud block (ACC-§14): status + Link to Cloud + Sync Now ----
	// LoggedIn rows (CloudStatusText also rides CloudLinkForm for round-trip
	// feedback); ApplyMode/RefreshCloudBlock own visibility and enablement.
	// ⛔ When the cloud is unconfigured the block STATES it and DISABLES - it
	// never hides, and it gates NOTHING local (ACC-§11).
	if (CloudStatusText == nullptr)
	{
		CloudStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloudStatusText"));
		if (CloudStatusText != nullptr)
		{
			CloudStatusText->SetFontSize(16.f);
			CloudStatusText->SetAutoWrapText(true);
			CloudStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.8f, 0.9f, 1.f)));

			if (UVerticalBoxSlot* CloudStatusSlot = RootPanel->AddChildToVerticalBox(CloudStatusText))
			{
				CloudStatusSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 4.f));
				CloudStatusSlot->SetHorizontalAlignment(HAlign_Fill);
				CloudStatusSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	ConstructPanelButton(LinkCloudButton, LinkCloudLabelText,
		TEXT("LinkCloudButton"), TEXT("LinkCloudLabelText"), SiegeAccountMenuText::LinkCloudLabel);
	ConstructPanelButton(SyncNowButton, SyncNowLabelText,
		TEXT("SyncNowButton"), TEXT("SyncNowLabelText"), SiegeAccountMenuText::SyncNowLabel);

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
	if (LinkCloudButton != nullptr)
	{
		LinkCloudButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleLinkCloudClicked);
	}
	if (SyncNowButton != nullptr)
	{
		SyncNowButton->OnClicked.AddUniqueDynamic(this, &UAccountMenuWidget::HandleSyncNowClicked);
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

	// Track cloud auth-state transitions while the panel is open (the ACC-§15
	// pinned no-param delegate). Cloud state only ever redraws the cloud BLOCK.
	if (USiegeCloudClient* Cloud = ResolveCloudClient())
	{
		Cloud->OnCloudStateChanged.AddUniqueDynamic(this, &UAccountMenuWidget::HandleCloudStateChanged);
	}

	// P2.1 (TASK-653): a fresh panel activation gets a fresh - single -
	// re-auth attempt. Reset BEFORE the mode seed below, whose LoggedIn path
	// runs RefreshCloudBlock and may spend it.
	bCloudSessionRefreshAttempted = false;

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

	if (USiegeCloudClient* Cloud = ResolveCloudClient())
	{
		Cloud->OnCloudStateChanged.RemoveDynamic(this, &UAccountMenuWidget::HandleCloudStateChanged);
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
	if (LinkCloudButton != nullptr)
	{
		LinkCloudButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleLinkCloudClicked);
	}
	if (SyncNowButton != nullptr)
	{
		SyncNowButton->OnClicked.RemoveDynamic(this, &UAccountMenuWidget::HandleSyncNowClicked);
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
	if (CurrentMode == EAccountMenuMode::CloudLinkForm)
	{
		// The P2 cloud lane - async, never blocking (ACC-§11). Its own function
		// keeps the qa-passed P1 local flows below textually untouched.
		SubmitCloudLink();
		return;
	}

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

void UAccountMenuWidget::HandleLinkCloudClicked()
{
	LinkCloudPressed();
}

void UAccountMenuWidget::HandleSyncNowClicked()
{
	SyncNowPressed();
}

void UAccountMenuWidget::HandleCloudStateChanged()
{
	// The ACC-§15 pinned no-param delegate: cloud auth state moved (sign-in,
	// sign-out, refresh). Only the cloud BLOCK re-derives - cloud state never
	// changes the panel MODE and never gates a local flow (ACC-§11).
	RefreshCloudBlock();
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

	const bool bChooser   = (NewMode == EAccountMenuMode::Chooser);
	const bool bCreate    = (NewMode == EAccountMenuMode::CreateForm);
	const bool bLogin     = (NewMode == EAccountMenuMode::LoginForm);
	const bool bLoggedIn  = (NewMode == EAccountMenuMode::LoggedIn);
	const bool bCloudLink = (NewMode == EAccountMenuMode::CloudLinkForm);

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
	SetShown(EmailInputBox, bCloudLink);
	SetShown(PasswordInputBox, bCreate || bLogin || bCloudLink);
	SetShown(ConfirmPasswordInputBox, bCreate || bCloudLink);
	SetShown(SubmitButton, bCreate || bLogin || bCloudLink);
	SetShown(LogoutButton, bLoggedIn);

	// The P2 cloud rows: CloudStatusText rides LoggedIn (the block) and
	// CloudLinkForm (round-trip feedback); the two cloud buttons default
	// collapsed here - RefreshCloudBlock() at the bottom owns them in LoggedIn.
	SetShown(CloudStatusText, bLoggedIn || bCloudLink);
	SetShown(LinkCloudButton, false);
	SetShown(SyncNowButton, false);

	if (SubmitLabelText != nullptr)
	{
		SubmitLabelText->SetText(FText::FromString(FString(
			bCreate ? SiegeAccountMenuText::SubmitCreateLabel
			: bCloudLink ? SiegeAccountMenuText::SubmitLinkLabel
			: SiegeAccountMenuText::SubmitLoginLabel)));
	}

	// The confirm box is REUSED by CloudLinkForm (ACC-§14: no duplicate
	// password children); there, filling it means "create a NEW cloud account"
	// - the hint says so, and says the P1 thing everywhere else.
	if (ConfirmPasswordInputBox != nullptr)
	{
		ConfirmPasswordInputBox->SetHintText(FText::FromString(FString(
			bCloudLink ? SiegeAccountMenuText::CloudConfirmPasswordHint : SiegeAccountMenuText::ConfirmPasswordHint)));
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

	case EAccountMenuMode::CloudLinkForm:
		Status = SiegeAccountMenuText::CloudLinkFormStatus;
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

	if (bLoggedIn)
	{
		// The LoggedIn screen's cloud rows - linked / unlinked / unconfigured /
		// in-flight - have ONE owner (the ACC-§11 states-and-disables law).
		RefreshCloudBlock();
	}
	else if (bCloudLink)
	{
		// Fresh entry into the link form: no stale cloud line. (An async
		// failure does NOT re-enter ApplyMode, so its error text survives.)
		ShowCloudStatus(FString());
	}
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
	case EAccountMenuMode::CreateForm:    ModeName = TEXT("CreateForm");    break;
	case EAccountMenuMode::LoginForm:     ModeName = TEXT("LoginForm");     break;
	case EAccountMenuMode::LoggedIn:      ModeName = TEXT("LoggedIn");      break;
	case EAccountMenuMode::CloudLinkForm: ModeName = TEXT("CloudLinkForm"); break;
	case EAccountMenuMode::Chooser:
	default:                              ModeName = TEXT("Chooser");       break;
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
	SetEnabled(EmailInputBox);
	SetEnabled(PasswordInputBox);
	SetEnabled(ConfirmPasswordInputBox);
	SetEnabled(SubmitButton);
	SetEnabled(LogoutButton);

	// BackButton is deliberately NOT in this list, in either direction: it must
	// stay live when everything else is dead. LinkCloudButton/SyncNowButton are
	// also deliberately absent - RefreshCloudBlock() owns their enablement (the
	// ACC-§11 states-and-disables law needs them independently controllable).
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

// ============================================================================
// P2 CLOUD LANE (TASK-646) - everything below is additive; the qa-passed P1
// flows above are untouched. All I/O is delegate-async (ACC-§11: nothing
// blocks on HTTP, cloud gates NOTHING), and only the ACC-§15 pinned surfaces
// of the sibling tasks are consumed.
// ============================================================================

void UAccountMenuWidget::LinkCloudPressed()
{
	if (bCloudRequestInFlight)
	{
		return; // one round trip at a time - the button is disabled in flight anyway
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	if (Account == nullptr)
	{
		ShowUnavailable();
		return;
	}

	if (!Account->IsLoggedIn())
	{
		// Guest never links or syncs (ACC-§13). Unreachable through the UI (the
		// button only exists on the LoggedIn screen); a Blueprint calling the
		// wrapper out of turn lands back on the derived mode.
		RefreshModeFromSubsystem();
		return;
	}

	USiegeCloudClient* Cloud = ResolveCloudClient();
	if (Cloud == nullptr || !Cloud->IsCloudConfigured())
	{
		// ACC-§11: the block states it and disables; nothing local is gated.
		RefreshCloudBlock();
		return;
	}

	if (Account->IsCloudLinked())
	{
		RefreshCloudBlock(); // already linked - a stale click just redraws the block
		return;
	}

	ApplyMode(EAccountMenuMode::CloudLinkForm);
}

void UAccountMenuWidget::SyncNowPressed()
{
	if (bCloudRequestInFlight)
	{
		return;
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	if (Account == nullptr)
	{
		ShowUnavailable();
		return;
	}

	USiegeCloudClient* Cloud = ResolveCloudClient();
	const bool bReady = Account->IsLoggedIn() && Account->IsCloudLinked()
		&& Cloud != nullptr && Cloud->IsCloudConfigured();
	if (!bReady)
	{
		// Guest, unlinked and unconfigured never sync (ACC-§13, ACC-§11) -
		// redraw the block so the on-screen state says why.
		RefreshCloudBlock();
		return;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (GameInstance == nullptr)
	{
		return;
	}

	StartCloudRequest(FString(SiegeAccountMenuText::CloudBusySyncing));

	// ACC-§13 trigger 3, exactly: pull-newer -> push-all -> LastSyncUtc =
	// server now - ALL of it inside FSiegeCloudSync (TASK-645); this widget
	// only shows the outcome. The TSharedRef captured by the completion lambda
	// keeps the sync engine alive across its async hops even if this panel is
	// dismissed mid-flight (Back is never disabled); the weak-this guard makes
	// the UI update safe either way.
	TSharedRef<FSiegeCloudSync> Sync = MakeShared<FSiegeCloudSync>();
	TWeakObjectPtr<UAccountMenuWidget> WeakThis(this);
	Sync->SyncNow(*GameInstance, FSiegeCloudResult::CreateLambda(
		[WeakThis, Sync](bool bOk, const FString& PayloadOrError)
		{
			if (UAccountMenuWidget* Widget = WeakThis.Get())
			{
				Widget->HandleCloudSyncResult(bOk, PayloadOrError, ECloudSyncOpContext::ManualSync);
			}
		}));
}

void UAccountMenuWidget::SubmitCloudLink()
{
	if (bCloudRequestInFlight)
	{
		return; // Submit is disabled in flight; a raced click is a no-op
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	if (Account == nullptr)
	{
		ShowUnavailable();
		return;
	}

	if (!Account->IsLoggedIn())
	{
		// Guest never links (ACC-§13). The form is only reachable from
		// LoggedIn; losing the profile mid-form lands back on the derived mode.
		ClearPasswordBoxes();
		RefreshModeFromSubsystem();
		return;
	}

	USiegeCloudClient* Cloud = ResolveCloudClient();
	if (Cloud == nullptr || !Cloud->IsCloudConfigured())
	{
		ClearPasswordBoxes();
		ShowStatus(FString(SiegeAccountMenuText::CloudNotConfigured));
		return;
	}

	const FString Email =
		(EmailInputBox != nullptr) ? EmailInputBox->GetText().ToString().TrimStartAndEnd() : FString();

	// ⛔ ACC-§2's wire extension + P2-R6. The plaintext password lives in these
	// LOCALS only: it crosses ONE call boundary (USiegeCloudClient - GoTrue
	// bcrypts it server-side, ACC-§11), it is NEVER logged, NEVER stored in a
	// member, NEVER captured by the completion lambda (which captures Email and
	// the flow flag only), and both password boxes are cleared before every
	// return below. The P1 local hash/salt are not read, written or uploaded.
	const FString Password =
		(PasswordInputBox != nullptr) ? PasswordInputBox->GetText().ToString() : FString();
	const FString ConfirmPassword =
		(ConfirmPasswordInputBox != nullptr) ? ConfirmPasswordInputBox->GetText().ToString() : FString();

	if (Email.IsEmpty() || !Email.Contains(TEXT("@")))
	{
		// Light check only - GoTrue owns real address validation; this just
		// saves an obviously-doomed round trip.
		ClearPasswordBoxes();
		ShowStatus(FString(SiegeAccountMenuText::CloudEnterEmail));
		return;
	}

	if (Password.IsEmpty())
	{
		ClearPasswordBoxes();
		ShowStatus(FString(SiegeAccountMenuText::CloudEnterPassword));
		return;
	}

	// Confirm box FILLED = create a NEW cloud account (must match); EMPTY =
	// sign into an existing one - exactly what the form's status line tells
	// the player up front. Case-sensitive: passwords compare exactly (the P1
	// rule, cloned).
	const bool bSignUp = !ConfirmPassword.IsEmpty();
	if (bSignUp && !Password.Equals(ConfirmPassword, ESearchCase::CaseSensitive))
	{
		ClearPasswordBoxes();
		ShowStatus(FString(SiegeAccountMenuText::PasswordsDoNotMatch));
		return;
	}

	StartCloudRequest(FString(SiegeAccountMenuText::CloudBusyLinking));

	TWeakObjectPtr<UAccountMenuWidget> WeakThis(this);
	FSiegeCloudResult OnDone = FSiegeCloudResult::CreateLambda(
		[WeakThis, Email, bSignUp](bool bOk, const FString& PayloadOrError)
		{
			if (UAccountMenuWidget* Widget = WeakThis.Get())
			{
				Widget->HandleCloudAuthResult(bOk, PayloadOrError, Email, bSignUp);
			}
		});

	// Async end to end - nothing here waits on HTTP (ACC-§11), and Back stays
	// live for the whole round trip.
	if (bSignUp)
	{
		Cloud->SignUp(Email, Password, OnDone);
	}
	else
	{
		Cloud->SignIn(Email, Password, OnDone);
	}

	ClearPasswordBoxes();
}

void UAccountMenuWidget::HandleCloudAuthResult(bool bOk, const FString& PayloadOrError, const FString& Email, bool bSignUpFlow)
{
	if (!bOk)
	{
		// ⛔ P2-R6 discipline for this file: the payload is NEVER logged on any
		// path. On failure it is an error string (contractually token- and
		// password-free, TASK-643) and it is DISPLAYED, not logged.
		UE_LOG(LogSiegeCloud, Log,
			TEXT("[AccountMenu] Cloud %s failed - the reason is on the panel. Local play is untouched (ACC-§11)."),
			bSignUpFlow ? TEXT("sign-up") : TEXT("sign-in"));
		FinishCloudRequest();
		ShowCloudStatus(PayloadOrError);
		return;
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	USiegeCloudClient* Cloud = ResolveCloudClient();
	if (Account == nullptr || Cloud == nullptr || !Account->IsLoggedIn())
	{
		// The local profile vanished mid-flight (a logout raced the round
		// trip). There is no profile to link - drop the result, honestly.
		UE_LOG(LogSiegeCloud, Warning,
			TEXT("[AccountMenu] Cloud auth succeeded but no active local profile remains - the link was NOT stored."));
		FinishCloudRequest();
		return;
	}

	// The pinned getter first (ACC-§15); the response body only fills gaps.
	FString UserId = Cloud->GetCloudUserId();
	FString RefreshToken;
	ParseAuthPayload(PayloadOrError, UserId, RefreshToken);

	if (UserId.IsEmpty())
	{
		UE_LOG(LogSiegeCloud, Warning,
			TEXT("[AccountMenu] Cloud auth succeeded but returned no user id - the link was NOT stored."));
		FinishCloudRequest();
		ShowCloudStatus(FString(SiegeAccountMenuText::CloudNoUserId));
		return;
	}

	if (RefreshToken.IsEmpty())
	{
		// Token-free log line (P2-R6). An empty refresh token only means the
		// link cannot silently re-authenticate later; it is stored as-is.
		UE_LOG(LogSiegeCloud, Log,
			TEXT("[AccountMenu] No refresh token in the auth response - the cloud link is session-only until the next sign-in."));
	}

	// TASK-644's API: mutates the ACTIVE profile, saves the registry,
	// broadcasts OnActiveProfileChanged - which lands this panel on LoggedIn
	// (still in flight, so RefreshCloudBlock keeps the cloud buttons disabled
	// until the sync below completes). The refresh token goes ONLY into
	// SetCloudLink - the ACC-§11 token law's one sanctioned home - and dies
	// with this call's locals.
	Account->SetCloudLink(Email, UserId, RefreshToken);

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (GameInstance == nullptr)
	{
		FinishCloudRequest();
		return;
	}

	// ACC-§13's trigger mapping, exactly:
	//   sign-UP (new cloud account)      => trigger 2, the A4 FIRST-LINK UPLOAD (PushAll)
	//   sign-IN (existing cloud account) => trigger 1, the CLOUD-LOGIN PULL     (PullAll)
	// LastSyncUtc bookkeeping is FSiegeCloudSync's (TASK-645), not this
	// widget's - it never calls SetLastSyncUtc.
	ShowCloudStatus(FString(bSignUpFlow
		? SiegeAccountMenuText::CloudUploadingAfterLink
		: SiegeAccountMenuText::CloudPullingAfterLink));

	TSharedRef<FSiegeCloudSync> Sync = MakeShared<FSiegeCloudSync>();
	TWeakObjectPtr<UAccountMenuWidget> WeakThis(this);
	const ECloudSyncOpContext OpContext =
		bSignUpFlow ? ECloudSyncOpContext::FirstLinkUpload : ECloudSyncOpContext::LoginPull;
	FSiegeCloudResult OnSyncDone = FSiegeCloudResult::CreateLambda(
		[WeakThis, Sync, OpContext](bool bSyncOk, const FString& SyncPayloadOrError)
		{
			if (UAccountMenuWidget* Widget = WeakThis.Get())
			{
				Widget->HandleCloudSyncResult(bSyncOk, SyncPayloadOrError, OpContext);
			}
		});

	if (bSignUpFlow)
	{
		Sync->PushAll(*GameInstance, OnSyncDone);
	}
	else
	{
		Sync->PullAll(*GameInstance, OnSyncDone);
	}
}

void UAccountMenuWidget::HandleCloudSyncResult(bool bOk, const FString& PayloadOrError, ECloudSyncOpContext OpContext)
{
	// Steady state first (visibility, enables, the pinned "Linked as <email>"),
	// then the outcome line lands on top of it in CloudStatusText.
	FinishCloudRequest();

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	const FString LinkedEmail = (Account != nullptr) ? Account->GetLinkedEmail() : FString();

	const TCHAR* OpDoneText =
		(OpContext == ECloudSyncOpContext::FirstLinkUpload)
			? TEXT("your decks and settings were uploaded to the cloud.")
		: (OpContext == ECloudSyncOpContext::LoginPull)
			? TEXT("your cloud decks and settings were pulled to this machine.")
		: TEXT("sync complete.");

	if (bOk)
	{
		UE_LOG(LogSiegeCloud, Log,
			TEXT("[AccountMenu] Cloud sync operation completed (context %d)."), static_cast<int32>(OpContext));
		ShowCloudStatus(FString::Printf(TEXT("Linked as %s - %s"), *LinkedEmail, OpDoneText));
	}
	else
	{
		// ACC-§11/§13: a failed sync leaves local state untouched and gates
		// nothing; the link itself stands and Sync Now is the retry. The error
		// string is displayed, never logged (P2-R6 discipline for this file).
		UE_LOG(LogSiegeCloud, Warning,
			TEXT("[AccountMenu] Cloud sync operation failed (context %d) - the reason is on the panel; local saves are untouched."),
			static_cast<int32>(OpContext));
		ShowCloudStatus(FString::Printf(TEXT("Linked as %s - sync failed: %s"), *LinkedEmail, *PayloadOrError));
	}
}

void UAccountMenuWidget::RefreshCloudBlock()
{
	// Only the LoggedIn screen carries the cloud rows (ACC-§14); every other
	// mode collapsed them in ApplyMode. A deliberate no-op elsewhere.
	if (CurrentMode != EAccountMenuMode::LoggedIn)
	{
		return;
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	USiegeCloudClient* Cloud = ResolveCloudClient();

	const bool bLinked = (Account != nullptr) && Account->IsCloudLinked();
	const bool bConfigured = (Cloud != nullptr) && Cloud->IsCloudConfigured();

	auto SetShown = [](UWidget* Widget, bool bShown)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	};
	auto SetEnabled = [](UWidget* Widget, bool bEnabled)
	{
		if (Widget != nullptr)
		{
			Widget->SetIsEnabled(bEnabled);
		}
	};

	// One button at a time (the TASK-646 spec's mapping): the Link-to-Cloud
	// entry while unlinked, Sync Now while linked.
	SetShown(LinkCloudButton, !bLinked);
	SetShown(SyncNowButton, bLinked);

	if (bCloudRequestInFlight)
	{
		// A round trip is running: the rows keep their shape, the buttons go
		// quiet, and the busy/interim line set by the request lane stays.
		SetEnabled(LinkCloudButton, false);
		SetEnabled(SyncNowButton, false);
		return;
	}

	if (!bConfigured)
	{
		// ⛔ ACC-§11: cloud OFF => the block STATES it and DISABLES. It never
		// hides, and it gates NOTHING local - the panel above is untouched and
		// the game is byte-identical Phase-1 everywhere else.
		SetEnabled(LinkCloudButton, false);
		SetEnabled(SyncNowButton, false);
		ShowCloudStatus(FString(SiegeAccountMenuText::CloudNotConfigured));
		return;
	}

	if (bLinked)
	{
		SetEnabled(SyncNowButton, true);
		// The TASK-646 pinned LoggedIn line: "Linked as <email>".
		ShowCloudStatus(FString::Printf(TEXT("Linked as %s"), *Account->GetLinkedEmail()));

		// P2.1 (TASK-653, rider R1): the post-restart re-auth attempt rides the
		// cloud-block refresh, exactly as the seam law names it. Both pointers
		// are non-null here by construction: bLinked implies Account resolved,
		// and the !bConfigured branch above already returned for a null or
		// unconfigured client. At most ONE attempt per activation (the latch
		// inside); with a live session or no held token it does nothing.
		TryRefreshCloudSession(*Account, *Cloud);
	}
	else
	{
		SetEnabled(LinkCloudButton, true);
		ShowCloudStatus(FString(SiegeAccountMenuText::CloudNotLinked));
	}
}

void UAccountMenuWidget::ShowCloudStatus(const FString& CloudMessage)
{
	if (CloudStatusText != nullptr)
	{
		CloudStatusText->SetText(FText::FromString(CloudMessage));
	}

	// Deliberately NOT routed through OnAccountMenuStateChanged: the BIE's
	// pinned (ModeName, StatusMessage) signature is a shipped P1 contract this
	// task does not move (condition (d)); a future WBP_AccountMenu reads
	// CloudStatusText directly.
}

void UAccountMenuWidget::StartCloudRequest(const FString& BusyMessage)
{
	bCloudRequestInFlight = true;
	ShowCloudStatus(BusyMessage);

	// The three cloud-lane entry points go quiet while a round trip runs. Back
	// is DELIBERATELY untouched - dismissing the panel must always work - and
	// no local flow is gated by a cloud request (ACC-§11).
	auto SetEnabled = [](UWidget* Widget, bool bEnabled)
	{
		if (Widget != nullptr)
		{
			Widget->SetIsEnabled(bEnabled);
		}
	};
	SetEnabled(SubmitButton, false);
	SetEnabled(LinkCloudButton, false);
	SetEnabled(SyncNowButton, false);
}

void UAccountMenuWidget::FinishCloudRequest()
{
	bCloudRequestInFlight = false;

	if (SubmitButton != nullptr)
	{
		SubmitButton->SetIsEnabled(true);
	}

	// LoggedIn redraws its cloud rows (visibility + enables + steady text);
	// outside LoggedIn this is a no-op and the async handler's own line stands.
	RefreshCloudBlock();
}

void UAccountMenuWidget::ParseAuthPayload(const FString& Payload, FString& InOutUserId, FString& OutRefreshToken) const
{
	// Best-effort parse of a GoTrue auth response body (ACC-§11 transport). Not
	// JSON => leave the outputs exactly as they came in. ⛔ Nothing here logs
	// the payload or anything parsed from it (P2-R6).
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return;
	}

	Root->TryGetStringField(TEXT("refresh_token"), OutRefreshToken);

	if (InOutUserId.IsEmpty())
	{
		const TSharedPtr<FJsonObject>* UserObject = nullptr;
		if (Root->TryGetObjectField(TEXT("user"), UserObject) && UserObject != nullptr && UserObject->IsValid())
		{
			(*UserObject)->TryGetStringField(TEXT("id"), InOutUserId);
		}
	}
}

// ============================================================================
// P2.1 RE-AUTH WIRE (TASK-653, riders R1+R2 - ACC-§15 P2.1). Additive on the
// qa-passed 646 lane above; the only consumer of the ACC-§11 single lawful
// token reader, USiegeAccountSubsystem::GetCloudRefreshToken.
// ============================================================================

void UAccountMenuWidget::TryRefreshCloudSession(USiegeAccountSubsystem& Account, USiegeCloudClient& Cloud)
{
	// The P2.1 predicate, split caller/callee: RefreshCloudBlock's linked
	// branch established linked + client resolved + configured + no request in
	// flight; the three checks below complete it. A predicate miss here does
	// NOT burn the activation's attempt - only a launched request does.
	if (bCloudSessionRefreshAttempted || Cloud.IsCloudAuthenticated())
	{
		return; // already attempted this activation, or a live session needs nothing
	}

	// ⛔ ACC-§11 / P2-R6: THE ONE lawful read of the persisted refresh token
	// (the ACC-§11 dated addition names this exact consumer). The value lives
	// in this local, is handed to RefreshSession's HTTPS grant, and is NEVER
	// logged, NEVER displayed, and NOT captured by the completion lambda below.
	const FString StoredRefreshToken = Account.GetCloudRefreshToken();
	if (StoredRefreshToken.IsEmpty())
	{
		// A session-only link (646's honest state: no token was returned at
		// link time). Nothing to attempt - the player re-links via the cloud
		// form when they want the cloud back; no status change, no log spam.
		return;
	}

	// Latch BEFORE the async call: whatever the round trip does, this
	// activation has spent its one attempt (no retry loop, ever - ACC-§11).
	bCloudSessionRefreshAttempted = true;

	// Token-free by construction (P2-R6): the line states the situation only.
	UE_LOG(LogSiegeCloud, Log,
		TEXT("[AccountMenu] Linked profile with no live cloud session - attempting the one P2.1 session refresh."));

	StartCloudRequest(FString(SiegeAccountMenuText::CloudBusyRestoringSession));

	// Async end to end (ACC-§11): nothing blocks, Back stays live, and the
	// lambda captures ONLY the weak widget - never the token.
	TWeakObjectPtr<UAccountMenuWidget> WeakThis(this);
	Cloud.RefreshSession(StoredRefreshToken, FSiegeCloudResult::CreateLambda(
		[WeakThis](bool bOk, const FString& PayloadOrError)
		{
			if (UAccountMenuWidget* Widget = WeakThis.Get())
			{
				Widget->HandleCloudRefreshResult(bOk, PayloadOrError);
			}
		}));
}

void UAccountMenuWidget::HandleCloudRefreshResult(bool bOk, const FString& PayloadOrError)
{
	if (!bOk)
	{
		// ⛔ THE P2.1 FAILURE LAW: ONE honest CloudStatusText line, ZERO state
		// mutation - no ClearCloudLink, no SetCloudLink, no stamp, nothing: a
		// transient network error must never destroy the link (local-first,
		// ACC-§11). The pinned line stands INSTEAD of the raw error text, and
		// the payload is neither displayed nor logged on this path. Order per
		// the 646 idiom: steady state first (FinishCloudRequest redraws
		// "Linked as <email>"), then the outcome line lands on top of it.
		UE_LOG(LogSiegeCloud, Log,
			TEXT("[AccountMenu] The P2.1 session refresh failed - the pinned line is on the panel; the link and all local state are untouched (ACC-§11)."));
		FinishCloudRequest();
		ShowCloudStatus(FString(SiegeAccountMenuText::CloudSessionExpired));
		return;
	}

	USiegeAccountSubsystem* Account = ResolveAccountSubsystem();
	USiegeCloudClient* Cloud = ResolveCloudClient();
	if (Account == nullptr || Cloud == nullptr || !Account->IsLoggedIn() || !Account->IsCloudLinked())
	{
		// A logout or unlink raced the round trip - there is no link to store a
		// rotated token onto. Drop the result honestly, mutating nothing; the
		// client-side session the refresh adopted is unaffected (643's state).
		UE_LOG(LogSiegeCloud, Warning,
			TEXT("[AccountMenu] The P2.1 session refresh succeeded but no cloud-linked profile remains - the rotated token was NOT stored."));
		FinishCloudRequest();
		return;
	}

	// The pinned getter first (ACC-§15), the payload only fills gaps - the 646
	// idiom, which equals the seam law's own SetCloudLink(GetLinkedEmail(),
	// Client->GetCloudUserId(), NewToken) shape whenever the getter answers.
	// The rotated token is parsed into a LOCAL, reaches exactly one sink
	// (SetCloudLink - the ACC-§11 token law's one sanctioned home) and dies
	// with this call. ⛔ Never logged, never displayed (P2-R6).
	FString UserId = Cloud->GetCloudUserId();
	FString RotatedToken;
	ParseAuthPayload(PayloadOrError, UserId, RotatedToken);

	if (!RotatedToken.IsEmpty())
	{
		// The P2.1 pinned re-store. Rotation is a REAL mutation: 644's
		// identical-values guard needs ALL THREE values identical, so a new
		// token with the unchanged email/user id stores, saves the registry and
		// broadcasts (the upheld 644 decision 4). If GoTrue returned the very
		// same token (its reuse window), the whole triple is identical and the
		// no-op guard correctly saves and broadcasts nothing. An empty UserId
		// corner (getter empty AND no user.id in the payload) is refused whole
		// by SetCloudLink's trim/refuse guard - never a half-link.
		Account->SetCloudLink(Account->GetLinkedEmail(), UserId, RotatedToken);
		UE_LOG(LogSiegeCloud, Log,
			TEXT("[AccountMenu] Cloud session restored - the rotated refresh token was re-stored (token: held)."));
	}
	else
	{
		// A success payload without a rotated token: store NOTHING. Wiping the
		// held token on a success would be a destructive write the law does not
		// order (SetCloudLink treats an empty token as a real value - 644).
		UE_LOG(LogSiegeCloud, Log,
			TEXT("[AccountMenu] Cloud session restored - no rotated refresh token in the payload; the stored token was kept."));
	}

	// Steady state: RefreshCloudBlock redraws "Linked as <email>" with Sync Now
	// live - the honest signed-in surface. No sync is triggered here: the seam
	// law orders none (Sync Now stays the player's lane, its pull now live per
	// rider R2).
	FinishCloudRequest();
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

USiegeCloudClient* UAccountMenuWidget::ResolveCloudClient() const
{
	// The ResolveAccountSubsystem shape, cloned (P2). USiegeCloudClient is a
	// UGameInstanceSubsystem (ACC-§15) so the cloud session survives the
	// L_MainMenu -> L_Arena travel exactly like the account state does. Null or
	// unconfigured lands on the ACC-§11 states-and-disables branch, never a
	// crash and never a gate.
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeCloudClient>() : nullptr;
}
