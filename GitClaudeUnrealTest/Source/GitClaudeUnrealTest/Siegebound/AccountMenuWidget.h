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
class USiegeCloudClient;
class USiegeMenuInputSubsystem;

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
	/** "Logged in as <DisplayName>" + Logout + Back (+ the P2 cloud block, ACC-§14). */
	LoggedIn,
	/**
	 *  P2 (TASK-646, ACC-§14): cloud email + the REUSED PasswordInputBox /
	 *  ConfirmPasswordInputBox (no duplicate password children) + Submit.
	 *  Confirm box FILLED = create a NEW cloud account (SignUp, then the A4
	 *  first-link upload - ACC-§13 trigger 2); left BLANK = sign into an
	 *  existing one (SignIn, then the cloud-login pull - ACC-§13 trigger 1).
	 */
	CloudLinkForm
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
 *  M8 DECLARATION - PHASE 2 (TASK-646, batch verbatim): Adds no replicated
 *  property, no new replicated class, no new relevancy tier, no RPC. All cloud
 *  traffic is client-local HTTPS from USiegeCloudClient (a
 *  UGameInstanceSubsystem); nothing crosses the UE networking layer. Does NOT
 *  consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's
 *  owed feedback items.
 *  ============================================================================
 *
 *  ---------------------------------------------------------------------------
 *  PHASE 2 (TASK-646) - THE CLOUD BLOCK, ADDITIVE ON THE QA-PASSED P1 TREE
 *  ---------------------------------------------------------------------------
 *  The ACC-§14 widget rows land here, character-for-character: EmailInputBox,
 *  CloudStatusText, LinkCloudButton + LinkCloudLabelText, SyncNowButton +
 *  SyncNowLabelText - all BindWidgetOptional, constructed-if-null inside
 *  ConstructAccountTree() (i.e. BEFORE Super::RebuildWidget(), the ACC-§5(b)
 *  order, unchanged). One mode is added: CloudLinkForm. Nothing shipped in P1
 *  moves: the panel still overlays the main menu without removing it, Back
 *  still removes only this widget in EVERY mode, BackdropBorder still absorbs
 *  clicks, and Escape stays permanently unabsorbed (AS-§6 A-2).
 *
 *  The LoggedIn screen grows the cloud block (ACC-§11 rules it):
 *    - cloud unconfigured  => the block STATES it and DISABLES; it never hides
 *                             and it gates NOTHING local (cloud gates nothing);
 *    - linked              => "Linked as <email>" + Sync Now;
 *    - unlinked            => the Link-to-Cloud entry.
 *
 *  Wiring (the ACC-§15 pinned surfaces only): auth via USiegeCloudClient
 *  (SignUp/SignIn), link state via TASK-644's USiegeAccountSubsystem API
 *  (IsCloudLinked/GetLinkedEmail/SetCloudLink), sync via FSiegeCloudSync
 *  (PushAll on first link - A4/trigger 2; PullAll on cloud login - trigger 1;
 *  SyncNow on the button - trigger 3). Everything is delegate-async: no
 *  gameplay or menu flow blocks on an HTTP round trip (ACC-§11), and every
 *  outcome lands in CloudStatusText.
 *
 *  P2-R6, as it binds this file: the typed cloud password lives in locals for
 *  the duration of SubmitCloudLink() only - never logged, never a member,
 *  never captured by a completion lambda. The refresh token is parsed from the
 *  auth response into a local and handed ONLY to SetCloudLink (the ACC-§11
 *  token law's one sanctioned home) - never logged. The raw auth payload
 *  (which carries tokens on success) is never logged from this file on any
 *  path. The P1 local hash/salt are not read, not written, not uploaded.
 *
 *  P2.1 (TASK-653, riders R1+R2 - ACC-§15 P2.1, the dated 2026-08-23 seam):
 *  THE POST-RESTART RE-AUTH WIRE. On the cloud-block refresh of a LoggedIn
 *  panel whose profile is linked while NO live session exists, exactly ONE
 *  USiegeCloudClient::RefreshSession attempt runs per panel activation, off
 *  the persisted refresh token read via the ACC-§11 single lawful reader
 *  (USiegeAccountSubsystem::GetCloudRefreshToken - this file is its ONLY
 *  consumer). Success re-stores the ROTATED token via SetCloudLink (a REAL
 *  mutation - saves + broadcasts, the upheld 644 decision 4); failure shows
 *  ONE honest CloudStatusText line and mutates NOTHING (local-first,
 *  ACC-§11). No retry loop, no tick, no poll; nothing blocks on HTTP; the
 *  token is never logged or displayed and never captured by a completion
 *  lambda. The refresh deliberately triggers NO sync - the law orders none;
 *  Sync Now (ACC-§13 trigger 3) stays the player's lane, and its pull phase
 *  is live against the persisted baseline (R2).
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
	 *  P2: LoggedIn -> CloudLinkForm. Guarded no-op unless logged in, cloud
	 *  configured (ACC-§11) and not yet linked - the only state in which
	 *  LinkCloudButton is shown enabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void LinkCloudPressed();

	/**
	 *  P2: the ACC-§13 trigger-3 manual sync - FSiegeCloudSync::SyncNow on the
	 *  active, cloud-linked profile. Async end to end; the outcome lands in
	 *  CloudStatusText; nothing local blocks or is gated (ACC-§11). Guest,
	 *  unlinked and unconfigured never sync (ACC-§13).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Account")
	void SyncNowPressed();

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

	/** OnClicked thunk for LinkCloudButton (P2). Forwards to LinkCloudPressed. */
	UFUNCTION()
	void HandleLinkCloudClicked();

	/** OnClicked thunk for SyncNowButton (P2). Forwards to SyncNowPressed. */
	UFUNCTION()
	void HandleSyncNowClicked();

	/**
	 *  USiegeAccountSubsystem::OnActiveProfileChanged handler (no params - the
	 *  ACC-§7 pinned delegate shape). Re-derives the mode from the subsystem so
	 *  the panel tracks login state changed from anywhere.
	 */
	UFUNCTION()
	void HandleActiveProfileChanged();

	/**
	 *  USiegeCloudClient::OnCloudStateChanged handler (no params - the ACC-§15
	 *  pinned delegate shape, P2). Re-derives ONLY the cloud block: a cloud
	 *  auth-state transition never changes the panel MODE and never gates a
	 *  local flow (ACC-§11: cloud gates NOTHING).
	 */
	UFUNCTION()
	void HandleCloudStateChanged();

	/** Null-safe subsystem resolve through this widget's world's game instance. */
	USiegeAccountSubsystem* ResolveAccountSubsystem() const;

	/** Null-safe USiegeCloudClient resolve (P2). Null or unconfigured => the cloud block states it and disables (ACC-§11). */
	USiegeCloudClient* ResolveCloudClient() const;

	// ------------------------------------------------------------------------
	// TASK-1419 [MENU-NAV-LOGIN] — 🧑 "we want to make sure everywhere in the
	// menu can be scrollable with the outline and arrow keys such that an agent
	// can navigate the entire menu."
	//
	// ⛔ THE WHOLE OF THIS ROW'S BEHAVIOUR IS THREE CALLS INTO TASK-1406's
	// PUBLIC API. There is deliberately NO key handler added here — no
	// NativeOnKeyDown, no NativeOnPreviewKeyDown, no FReply anywhere in this
	// class — because USiegeMenuInputSubsystem owns Up / Down / Left / Right /
	// Accept for the whole menu, and a per-screen key handler is the
	// wrong-layer defect the MENU-NAV epic exists to remove. ⇒ THE CLASS
	// COMMENT'S STANDING SENTENCE IS STILL TRUE CHARACTER-FOR-CHARACTER AFTER
	// THIS ROW: "No key handling is overridden anywhere in this class - in
	// particular `Escape` stays permanently unabsorbed, project-wide
	// (AS-§6 A-2)."
	//
	// ────────────────────────────────────────────────────────────────────────
	// ⛔ THE FOCUS STOPS THIS SCREEN OFFERS, IN `UWidgetTree::ForEachWidget`
	//    ORDER (depth-first pre-order from BackdropBorder), PER MODE — because
	//    this panel is MODE-SWITCHED and the stop set is therefore NOT ONE
	//    LIST. Every stop below is constructed in C++ by ConstructAccountTree()
	//    (construction route: C++; there is no /Game/UI/WBP_AccountMenu — it is
	//    RESERVED and UNAUTHORED, condition (c) above), and the non-stops are
	//    the UBorder, the UVerticalBox and the ten UTextBlocks, none of which
	//    is one of TASK-1406's four admitted classes.
	//
	//      Chooser        (3): CreateAccountButton, LoginExistingButton,
	//                          BackButton
	//      CreateForm     (5): NameInputBox, PasswordInputBox,
	//                          ConfirmPasswordInputBox, SubmitButton,
	//                          BackButton
	//      LoginForm      (4): NameInputBox, PasswordInputBox, SubmitButton,
	//                          BackButton
	//      CloudLinkForm  (5): EmailInputBox, PasswordInputBox,
	//                          ConfirmPasswordInputBox, SubmitButton,
	//                          BackButton
	//      LoggedIn     (2/3): LogoutButton, [LinkCloudButton XOR
	//                          SyncNowButton, only while the cloud is
	//                          CONFIGURED — RefreshCloudBlock() disables the
	//                          shown one when it is not, and a disabled widget
	//                          is not a stop], BackButton
	//
	//    ⚠️ BackButton is in EVERY list: ApplyMode() never touches its
	//    visibility and SetFormsEnabled() deliberately never touches its
	//    enablement, because a panel you cannot leave is worse than a panel
	//    that cannot log anyone in.
	//
	//    ⚠️ THE UNHAPPY PATH IS ONE (1): ShowUnavailable() → SetFormsEnabled
	//    (false) kills the other two Chooser stops, leaving BackButton alone.
	//    THE DISCRIMINATOR between a healthy 1 and the defect is the log line —
	//    a 1 WITH "[AccountMenu] USiegeAccountSubsystem could not be resolved"
	//    (Warning, LogSiegeAccount) is the ACC-§1 fail-safe working; a 1
	//    WITHOUT it is a broken tree.
	//
	// ────────────────────────────────────────────────────────────────────────
	// 🚨 THE TEXT-BOX RULE THIS SCREEN IMPLEMENTS, AND IT IS THE ENGINE'S OWN
	//    (that is WHY no key handler is needed, and it is MEASURED, not hoped):
	//
	//      Up / Down  LEAVE the field.   Left / Right  STAY (caret).
	//
	//    • THE AGENT LANE (an injected IA_MenuUp/IA_MenuDown) never offers the
	//      key to Slate at all — InjectInputForAction enters Enhanced Input
	//      directly, HandleMenuDown() → MoveFocus(+1) → FSlateApplication::
	//      SetUserFocus. A UEditableTextBox CANNOT absorb a key it is never
	//      shown. Egress on this lane is structural.
	//    • THE HUMAN LANE (a real key) is Slate's, and the engine already
	//      implements exactly the recommended rule: SEditableText::OnKeyDown
	//      → FSlateEditableTextLayout::HandleKeyDown (SlateEditableTextLayout
	//      .cpp:994) routes Up/Down into MoveCursor(Cardinal, (0, ∓1)), and
	//      MoveCursor at :2261-2266 hits `else { // Vertical movement not
	//      supported on single-line editable text controls - return false so we
	//      fallback to generic widget navigation \n return false; }`.
	//      BoolToReply(false) is FReply::Unhandled() ⇒ Slate runs its own
	//      directional navigation and the ring leaves the field. Left/Right go
	//      through TranslatedLocation and report Handled ⇒ the caret keeps
	//      them, which is the half a player would riot about losing.
	//
	// 🚨 AND THE DEFECT THIS ROW FOUND AND DID **NOT** FIX, BECAUSE THE FIX IS
	//    IN A FENCED FILE (declared, never self-adjudicated — SC-§101/SC-§50):
	//    SEditableTextBox::OnFocusReceived (SEditableTextBox.cpp:309-320)
	//    FORWARDS keyboard focus to its inner SEditableText, so after the ring
	//    lands on a UEditableTextBox the SWidget holding focus is the INNER
	//    text, not the SEditableTextBox that UMG caches. UWidget::HasUserFocus
	//    (Widget.cpp:641) is EXACT-widget (FSlateUser::HasFocus compares
	//    GetFocusedWidget() == Widget, SlateUser.cpp:182-185), so
	//    USiegeMenuInputSubsystem::GetFocusedNavStop() reads NULL while the
	//    ring is visibly in a field ⇒ MoveFocus() computes from Current = 0 and
	//    Down out of ANY text box always lands on stop 1. The named one-line
	//    remedy is `|| Stop->HasUserFocusedDescendants(PC)` in
	//    GetFocusedNavStop() (and the twin early-out in FocusFirstNavStop());
	//    it is TASK-1406's file and this row does not write it.
	//    ⛔ THIS FILE IS WRITTEN SO IT IS CORRECT EITHER WAY: the one place
	//    that asks "does a live stop still hold the ring?" (RefreshMenuNavRing)
	//    asks with HasUserFocus **or** HasUserFocusedDescendants, on this
	//    screen's own children, which is entirely inside this row's fence.
	// ------------------------------------------------------------------------

	/**
	 *  Hand menu navigation to THIS screen. Called from NativeConstruct AFTER
	 *  RefreshModeFromSubsystem(), and paired with UnregisterAsMenuNavTarget()
	 *  on every exit.
	 *
	 *  ⛔ THE "AFTER" IS LOAD-BEARING, and worse here than on the settings
	 *  panel. RegisterMenuNavTarget() logs the focus-stop count AND places the
	 *  ring on stop 0, both read from the tree's LIVE visible/enabled state —
	 *  and RefreshModeFromSubsystem() is what settles that state: it runs
	 *  ApplyMode(), which COLLAPSES ten of the thirteen possible children, and
	 *  on the unhappy path it then runs ShowUnavailable() → SetFormsEnabled
	 *  (false). Registering first would log 13 for a screen that has 3, and
	 *  could park the ring on a control collapsed or disabled one line later.
	 *
	 *  ⭐ The ring is ALREADY ON at open ⇒ the first Down moves to stop 1, not
	 *  to stop 0. A reader expecting stop 0 after one Down mis-reads a working
	 *  screen as broken.
	 *
	 *  Null-safe: no world or no subsystem ⇒ one Log line and the panel behaves
	 *  exactly as it did before this row (mouse-only). Never fatal.
	 */
	void RegisterAsMenuNavTarget();

	/**
	 *  Give menu navigation back. Called from BOTH BackPressed() (after this
	 *  panel's own teardown, BEFORE RemoveFromParent) and NativeDestruct() (the
	 *  first statement, LIFO against NativeConstruct). The double call is
	 *  deliberate and safe: USiegeMenuInputSubsystem::UnregisterMenuNavTarget
	 *  removes by IDENTITY and logs-not-warns a second call for a screen that
	 *  is already gone.
	 *
	 *  ⛔ BackPressed() alone is not enough — this panel can also stop existing
	 *  by level travel or viewport teardown, and a stranded registration would
	 *  pin the ring to a dead tree.
	 */
	void UnregisterAsMenuNavTarget();

	/**
	 *  Re-place the ring when THIS SCREEN'S OWN STATE CHANGE dropped it.
	 *
	 *  ⛔ THIS IS THE ONE THING THE SETTINGS AND GRAPHICS PANELS DID NOT NEED
	 *  AND THIS ONE CANNOT DO WITHOUT: those two screens have ONE stop set for
	 *  their whole lifetime; this panel is a MODE MACHINE (Chooser →
	 *  CreateForm → LoggedIn → CloudLinkForm) and every ApplyMode() collapses
	 *  the row the ring is standing on. Accept on "Create Account" collapses
	 *  CreateAccountButton itself ⇒ without this the ring vanishes on the very
	 *  first thing a keyboard user does, which is 🧑 his complaint reproduced
	 *  by the feature meant to fix it.
	 *
	 *  It is a NO-OP unless (a) this screen is the ACTIVE nav target — so it
	 *  can never steal the ring from a screen stacked on top of it, and it is
	 *  inert before registration and after unregistration — and (b) no LIVE
	 *  focus stop of this screen still holds the ring. When both hold it calls
	 *  RegisterMenuNavTarget(this) again, which re-tops the stack (no
	 *  duplicate), logs the NEW stop count, and places the ring on the new
	 *  mode's stop 0.
	 *
	 *  ⚠️ (b) IS ASKED WITH HasUserFocus **OR** HasUserFocusedDescendants, and
	 *  the "or" is the whole point: a UEditableTextBox forwards keyboard focus
	 *  to its inner SEditableText, so HasUserFocus alone reports FALSE for a
	 *  field that is visibly wearing the ring — and this helper would then yank
	 *  the ring back to stop 0 out from under a player who is typing.
	 */
	void RefreshMenuNavRing();

	/**
	 *  Null-safe resolve of the menu-input subsystem. ⚠️ Unlike
	 *  USiegeAccountSubsystem and USiegeCloudClient this one lives on the
	 *  WORLD, not the game instance, and it declines Editor worlds outright
	 *  (DoesSupportWorldType = Game | PIE only), so a null answer is ordinary
	 *  rather than an error.
	 */
	USiegeMenuInputSubsystem* ResolveMenuInputSubsystem() const;

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
	// P2 CLOUD LANE (TASK-646). Plain methods, not UFUNCTIONs - FSiegeCloudResult
	// is the ACC-§15 non-dynamic delegate and these are its C++-only handlers.
	// ------------------------------------------------------------------------

	/**
	 *  Which FSiegeCloudSync operation a completion belongs to - wording and
	 *  logging only. C++-only, never a BIE param (condition (d)).
	 */
	enum class ECloudSyncOpContext : uint8
	{
		/** ACC-§13 trigger 2 (A4): the first-link upload after SignUp. */
		FirstLinkUpload,
		/** ACC-§13 trigger 1: the cloud-login pull after SignIn. */
		LoginPull,
		/** ACC-§13 trigger 3: the Sync Now button. */
		ManualSync
	};

	/**
	 *  The CloudLinkForm submit lane: validate locally, then async
	 *  SignUp/SignIn on USiegeCloudClient. P2-R6: the password lives in this
	 *  call's locals only and both password boxes are cleared on every path.
	 */
	void SubmitCloudLink();

	/**
	 *  SignUp/SignIn completion. On success: SetCloudLink (TASK-644), then the
	 *  ACC-§13 trigger mapping exactly - PushAll for a sign-up (first link,
	 *  A4), PullAll for a sign-in. On failure: the error string (contractually
	 *  token-free, TASK-643) lands in CloudStatusText; the raw payload is never
	 *  logged from this file.
	 */
	void HandleCloudAuthResult(bool bOk, const FString& PayloadOrError, const FString& Email, bool bSignUpFlow);

	/** PushAll/PullAll/SyncNow completion: steady-state redraw, then the outcome line in CloudStatusText. */
	void HandleCloudSyncResult(bool bOk, const FString& PayloadOrError, ECloudSyncOpContext OpContext);

	/**
	 *  The LoggedIn screen's cloud rows, one owner: linked / unlinked /
	 *  unconfigured / request-in-flight (ACC-§11 states-and-disables law).
	 *  A deliberate no-op in every other mode.
	 */
	void RefreshCloudBlock();

	/**
	 *  Writes CloudStatusText only. Deliberately NOT routed through
	 *  OnAccountMenuStateChanged - the BIE's pinned (ModeName, StatusMessage)
	 *  signature is a shipped P1 contract this task does not move; a future
	 *  WBP_AccountMenu reads CloudStatusText directly.
	 */
	void ShowCloudStatus(const FString& CloudMessage);

	/** Marks a round trip in flight: busy line in CloudStatusText, the three cloud-lane buttons disabled. Back is never touched. */
	void StartCloudRequest(const FString& BusyMessage);

	/** Clears the in-flight flag, re-enables Submit, redraws the LoggedIn cloud rows. */
	void FinishCloudRequest();

	/**
	 *  Best-effort parse of a GoTrue auth response body: top-level
	 *  "refresh_token", and "user.id" as a fallback ONLY when InOutUserId
	 *  arrived empty (the pinned GetCloudUserId() getter is preferred). Not
	 *  JSON => both outputs left as they came in. Never logs the payload.
	 */
	void ParseAuthPayload(const FString& Payload, FString& InOutUserId, FString& OutRefreshToken) const;

	/**
	 *  P2.1 (TASK-653, rider R1 - ACC-§15 P2.1): the post-restart re-auth
	 *  attempt. Called from RefreshCloudBlock()'s linked branch, which has
	 *  already established linked + client resolved + configured + no request
	 *  in flight; this function adds the once-per-activation latch, the
	 *  live-session check and the token read (via the ACC-§11 single lawful
	 *  reader, GetCloudRefreshToken). When the whole predicate holds: exactly
	 *  ONE USiegeCloudClient::RefreshSession call - no retry loop, no tick, no
	 *  poll, nothing blocks on HTTP (ACC-§11). ⛔ The token lives in this
	 *  call's locals, is never logged or displayed, and is NOT captured by the
	 *  completion lambda. Plain method, not a UFUNCTION - P2.1 adds NO
	 *  reflected member by design (the SC-§26 pin).
	 */
	void TryRefreshCloudSession(USiegeAccountSubsystem& Account, USiegeCloudClient& Cloud);

	/**
	 *  RefreshSession completion (P2.1). Success: the client already adopted
	 *  the fresh session (643); the ROTATED refresh token is parsed from the
	 *  payload into a local and re-stored via SetCloudLink(GetLinkedEmail(),
	 *  GetCloudUserId(), NewToken) - a REAL mutation (saves + broadcasts; 644's
	 *  identical-values guard needs all THREE values identical, so a rotated
	 *  token always stores). An absent rotated token stores NOTHING - a success
	 *  never wipes the held token. Failure: the ONE honest pinned
	 *  CloudStatusText line and ⛔ ZERO state mutation - a transient network
	 *  error must never destroy the link (local-first, ACC-§11).
	 */
	void HandleCloudRefreshResult(bool bOk, const FString& PayloadOrError);

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

	// ------------------------------------------------------------------------
	// P2 PINNED CHILDREN (TASK-646) - CONVENTIONS ACC-§14 widget rows,
	// character-for-character. Same BindWidgetOptional law as above: an
	// asset-authored WBP_AccountMenu using these exact names binds here and the
	// code-authored branch never runs. Nothing below is ever hard-required.
	// ------------------------------------------------------------------------

	/** Cloud-account email entry (CloudLinkForm only). The P2 login identity is an EMAIL (A2/ACC-§10); DisplayName stays the in-game handle. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EmailInputBox;

	/** The cloud block's one status surface: linked/unlinked/unconfigured state, busy lines, every sync outcome (ACC-§13: every sync outcome is visible). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CloudStatusText;

	/** LoggedIn + unlinked: opens CloudLinkForm. Shown DISABLED when the cloud is unconfigured (ACC-§11 states-and-disables). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LinkCloudButton;

	/** "Link to Cloud" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LinkCloudLabelText;

	/** LoggedIn + linked: the ACC-§13 trigger-3 manual sync. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SyncNowButton;

	/** "Sync Now" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Account", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SyncNowLabelText;

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

	/**
	 *  P2: true while a cloud round trip (link auth, post-link sync, manual
	 *  sync) is outstanding - ONE at a time. Disables only the three cloud-lane
	 *  buttons; Back and every local flow stay live (ACC-§11: nothing blocks on
	 *  HTTP, cloud gates nothing).
	 */
	bool bCloudRequestInFlight = false;

	/**
	 *  P2.1 (TASK-653): latches the post-restart re-auth to exactly ONE
	 *  RefreshSession attempt per panel activation (reset in NativeConstruct;
	 *  set only at the moment an attempt actually launches, so a healthy
	 *  session or an absent token never burns the activation's attempt).
	 *  Plain member, deliberately NOT a UPROPERTY - P2.1 adds NO reflected
	 *  member by design (the SC-§26 pin for TASK-655).
	 */
	bool bCloudSessionRefreshAttempted = false;
};
