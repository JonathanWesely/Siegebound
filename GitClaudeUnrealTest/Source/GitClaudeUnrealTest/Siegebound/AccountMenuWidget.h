// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AccountMenuWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;
class USiegeAccountSubsystem;

/**
 *  Internal mode machine for the account panel (ACC-§5).
 *
 *  Deliberately a PLAIN enum class - no UENUM(), no BlueprintType, and it is
 *  NEVER a BlueprintImplementableEvent parameter (ACC-§5: "C++-only, never a
 *  BIE param"; condition (d)'s FString/int32/bool/uint8-only BIE law). Any
 *  future WBP_AccountMenu receives the mode as an FString name through
 *  OnAccountMenuStateChanged instead.
 */
enum class EAccountMenuMode : uint8
{
	/** The two buttons Jonathan named: Create Account / Log into existing account. */
	Chooser,
	/** Name + password + confirm password + Submit. */
	CreateForm,
	/** Name + password + Submit. */
	LoginForm,
	/** "Logged in as <DisplayName>" + Logout + Back. */
	LoggedIn
};

/**
 *  TASK-603 [ACC-5] - the account panel: Login chooser / Create Account /
 *  Log In / Logged-in screens, one widget, mode-switched.
 *
 *  ============================================================================
 *  M8 DECLARATION (batch header, verbatim): Adds no replicated property, no new
 *  replicated class, no new relevancy tier, no RPC. All account state is
 *  client-local (UGameInstanceSubsystem + local USaveGame); the display name
 *  touches no session/player name (A7). Does NOT consume the M8 Phase-1
 *  checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
 *  ============================================================================
 *
 *  ---------------------------------------------------------------------------
 *  WHY THERE IS NO .uasset - THE RULING THIS WIDGET IS BUILT UNDER
 *  ---------------------------------------------------------------------------
 *  CONVENTIONS ACC-§5 authorises a CODE-AUTHORED widget tree for
 *  UAccountMenuWidget, and TASKBOARD ACCOUNTS manager RULING 5 records it as a
 *  NEW, NARROW exception argued on its own facts. It deliberately does NOT
 *  inherit the settings-lane ruling (scoped to USettingsMenuWidget only) or the
 *  assistant-console ruling (scoped to USiegeAssistantConsoleWidget only);
 *  a FOURTH widget may cite none of the three.
 *
 *  The five conditions, all QA criteria (ACC-§5):
 *   (a) SCOPE - this class only.
 *   (b) THE ORDER (the corrected law, already paid for once): the tree is
 *       built and WidgetTree->RootWidget is set FIRST, then
 *       `return Super::RebuildWidget();` - the TASK-444 shipped shape.
 *       Anything constructed after Super is discarded (UserWidget.cpp:1214)
 *       and the widget renders empty while passing every property readback.
 *       Every child below is UPROPERTY(meta=(BindWidgetOptional)) and is
 *       constructed only while still null.
 *   (c) /Game/UI/WBP_AccountMenu is RESERVED, NOT AUTHORED (verified absent at
 *       decomposition). A later asset-authored tree using these exact child
 *       names wins WHOLE with zero C++ change.
 *   (d) THE CONTRACT is the shipped USessionMenuWidget/USettingsMenuWidget
 *       contract, cloned: BindWidgetOptional members, BlueprintCallable
 *       wrappers, FString/int32/bool/uint8-only BlueprintImplementableEvents.
 *   (e) VERIFICATION IS A HUMAN PIXEL CHECK (TASK-609). MCP readback has
 *       repeatedly passed on visually-broken UMG here; NOTHING ON SCREEN IS
 *       VERIFIED BY THIS FILE'S AUTHOR.
 *
 *  ---------------------------------------------------------------------------
 *  THE HONEST-CREDENTIAL LAW (ACC-§2) AS IT BINDS THIS FILE
 *  ---------------------------------------------------------------------------
 *  The plaintext password exists here ONLY as locals inside SubmitPressed()
 *  and inside the two password UEditableTextBox controls the player types
 *  into. It is NEVER logged, NEVER stored in a member that outlives the
 *  submit call, and both password boxes are cleared on every mode change and
 *  after every submit attempt. Hashing/persistence is USiegeAccountSubsystem's
 *  job (TASK-600); this widget hands the plaintext across one call boundary
 *  and drops it. Phase-1 credentials are a local convenience, NOT security -
 *  real auth is the Phase-2 backend's job.
 *
 *  ---------------------------------------------------------------------------
 *  NAVIGATION (ACC-§5, the settings §4 overlay law cloned)
 *  ---------------------------------------------------------------------------
 *  TASK-607 adds Btn_Login to /Game/UI/WBP_MainMenu:
 *      CreateWidget(UAccountMenuWidget) -> AddToViewport(ZOrder 10)
 *  and does NOT remove the main menu. BackPressed() therefore calls
 *  RemoveFromParent() on THIS widget and nothing else, in every mode.
 *
 *  ==> THAT IS WHY BackdropBorder IS HIT-TEST **VISIBLE**, AND IT IS
 *      CORRECTNESS, NOT STYLING (ACC-§5): the panel overlays WBP_MainMenu, and
 *      an invisible plate would let clicks fall through into Play / Quit while
 *      the panel looks modal - the settings lane's click-through-into-Quit
 *      lesson, same geometry.
 *
 *  No key handling is overridden anywhere in this class - in particular
 *  `Escape` stays permanently unabsorbed, project-wide (AS-§6 A-2).
 *
 *  ---------------------------------------------------------------------------
 *  DEPENDENCY
 *  ---------------------------------------------------------------------------
 *  All model calls go to USiegeAccountSubsystem (TASK-600, same batch, pinned
 *  in CONVENTIONS ACC-§7 character-for-character). This file WILL NOT COMPILE
 *  ALONE and is not expected to - the TASK-416/417 precedent; the batch links
 *  at TASK-606's single compile gate. Do not open a QA loop over it.
 *
 *  Null-safe throughout: no subsystem resolvable => StatusText says why, the
 *  forms render DISABLED, Back still works, logged ONCE, never a crash - the
 *  ResolveSettingsSubsystem fail-safe shape, cloned. The game continues as
 *  guest either way (ACC-§1: login gates nothing).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UAccountMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Chooser -> CreateForm. No-op with the unavailable status when the subsystem is unresolvable. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void CreateAccountChosen();

	/** Chooser -> LoginForm. No-op with the unavailable status when the subsystem is unresolvable. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void LoginExistingChosen();

	/**
	 *  Submits the current form (CreateForm or LoginForm) to
	 *  USiegeAccountSubsystem::CreateAccount / Login. Failures render the
	 *  subsystem's OutReason in StatusText - never a crash, never a silent
	 *  no-op (ACC-§5). The plaintext password lives in locals for the duration
	 *  of this call only, and both password boxes are cleared before it
	 *  returns (ACC-§2).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void SubmitPressed();

	/** LoggedIn -> Logout on the subsystem, then back to the Chooser. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void LogoutPressed();

	/**
	 *  Back entry. Calls RemoveFromParent() on this widget AND NOTHING ELSE, in
	 *  every mode (ACC-§5 navigation: the main menu underneath was never
	 *  removed, is already alive and already correct).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void BackPressed();

	/**
	 *  Fired for presentation on top of what C++ already does to the bound
	 *  controls - on every REAL (mode, status) change, first seed included.
	 *  It does NOT fire on a no-op (the delegate law, cloned from the settings
	 *  lane: a delegate that fires on unchanged state trains consumers to
	 *  ignore it). ModeName is the mode's FString name ("Chooser" /
	 *  "CreateForm" / "LoginForm" / "LoggedIn"); the enum itself never crosses
	 *  (condition (d), the FString/int32/bool/uint8-only BIE law).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Account")
	void OnAccountMenuStateChanged(const FString& ModeName, const FString& StatusMessage);

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** OnClicked thunk for CreateAccountButton. Forwards to CreateAccountChosen. */
	UFUNCTION()
	void HandleCreateAccountClicked();

	/** OnClicked thunk for LoginExistingButton. Forwards to LoginExistingChosen. */
	UFUNCTION()
	void HandleLoginExistingClicked();

	/** OnClicked thunk for SubmitButton. Forwards to SubmitPressed. */
	UFUNCTION()
	void HandleSubmitClicked();

	/** OnClicked thunk for LogoutButton. Forwards to LogoutPressed. */
	UFUNCTION()
	void HandleLogoutClicked();

	/** OnClicked thunk for BackButton. Forwards to BackPressed. */
	UFUNCTION()
	void HandleBackClicked();

	/**
	 *  USiegeAccountSubsystem::OnActiveProfileChanged handler (no params - the
	 *  ACC-§7 pinned delegate shape). Re-derives the mode from the subsystem so
	 *  the panel tracks login state changed from anywhere.
	 */
	UFUNCTION()
	void HandleActiveProfileChanged();

	/** Null-safe subsystem resolve through this widget's world's game instance. */
	USiegeAccountSubsystem* ResolveAccountSubsystem() const;

	/**
	 *  Builds the code-authored tree. Called from RebuildWidget() BEFORE
	 *  Super::RebuildWidget() - see the comment there, the order is
	 *  load-bearing (ACC-§5(b)). Returns immediately when an asset-authored
	 *  tree is present (the escape hatch).
	 */
	void ConstructAccountTree();

	/**
	 *  Constructs one pinned button + its pinned content label, only where the
	 *  members are still null, in the shipped WBP_MainMenu family idiom (label
	 *  font 28, content padding 24/12/24/12, HAlign_Fill row).
	 */
	void ConstructPanelButton(TObjectPtr<UButton>& ButtonMember, TObjectPtr<UTextBlock>& LabelMember,
		const TCHAR* ButtonName, const TCHAR* LabelName, const TCHAR* LabelString);

	/** Constructs one pinned input box, only where the member is still null. */
	void ConstructInputBox(TObjectPtr<UEditableTextBox>& BoxMember,
		const TCHAR* BoxName, const TCHAR* HintString, bool bIsPassword);

	/** Applies a mode: per-mode child visibility, the mode's default status line, password-box hygiene (ACC-§2). */
	void ApplyMode(EAccountMenuMode NewMode);

	/** Re-derives the mode from the subsystem: logged in => LoggedIn, guest => Chooser, unresolvable => the fail-safe. */
	void RefreshModeFromSubsystem();

	/** Writes StatusText and fires OnAccountMenuStateChanged with the current mode's name. */
	void ShowStatus(const FString& StatusMessage);

	/**
	 *  The subsystem could not be resolved: disable every account control
	 *  except Back, explain in StatusText, log ONCE per widget instance.
	 *  Never a crash - the game continues as guest (ACC-§1).
	 */
	void ShowUnavailable();

	/** Enables/disables the interactive account controls. Back is deliberately never touched - a panel you cannot leave is worse. */
	void SetFormsEnabled(bool bEnabled);

	/** Clears both password boxes (ACC-§2 hygiene). The name box is left alone - it is not a credential. */
	void ClearPasswordBoxes();

	// ------------------------------------------------------------------------
	// PINNED CHILDREN - CONVENTIONS ACC-§5, character-for-character.
	// All BindWidgetOptional, never BindWidget: an asset-authored
	// WBP_AccountMenu using these exact names binds here and the code-authored
	// branch never runs. Nothing below is ever hard-required.
	// ------------------------------------------------------------------------

	/** Hit-test VISIBLE modal plate; the tree root. See the class comment - correctness, not styling (ACC-§5). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BackdropBorder;

	/** The panel column. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

	/** "Account". */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Per-mode guidance, submit-failure OutReason text, "Logged in as <DisplayName>", and the unavailable explanation. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Chooser: opens the create form. Jonathan's first named button. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreateAccountButton;

	/** "Create Account" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreateAccountLabelText;

	/** Chooser: opens the login form. Jonathan's second named button. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoginExistingButton;

	/** "Log into existing account" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LoginExistingLabelText;

	/** Display-name entry (both forms). NOT a credential; never a password box. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NameInputBox;

	/** Password entry (both forms). SetIsPassword(true) on the code-authored path (ACC-§5). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> PasswordInputBox;

	/** Confirm-password entry (CreateForm only). SetIsPassword(true) on the code-authored path (ACC-§5). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> ConfirmPasswordInputBox;

	/** Submits the visible form. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SubmitButton;

	/** "Create Account" / "Log In" depending on the visible form - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubmitLabelText;

	/** LoggedIn only. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LogoutButton;

	/** "Log Out" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LogoutLabelText;

	/** Dismisses this panel only, in every mode. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	/** "Back" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackLabelText;

private:

	/** The mode currently applied to the tree. C++-only (see EAccountMenuMode). */
	EAccountMenuMode CurrentMode = EAccountMenuMode::Chooser;

	/**
	 *  Last (mode name, status) pushed through OnAccountMenuStateChanged -
	 *  drives the "no BIE on a no-op" rule. Status strings never contain a
	 *  password (OutReason is contractually clean per TASK-600 / ACC-§2), so
	 *  caching one here is safe.
	 */
	FString LastNotifiedModeName;
	FString LastNotifiedStatus;

	/** False until the first ShowStatus, so the first push always notifies. */
	bool bHasNotifiedState = false;

	/** Latches the "subsystem unavailable" log to ONE line per widget instance. */
	bool bLoggedSubsystemUnavailable = false;
};
