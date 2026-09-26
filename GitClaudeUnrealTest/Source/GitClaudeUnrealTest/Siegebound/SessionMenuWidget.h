// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SessionMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class USiegeSessionSubsystem;
// TASK-1425 [MENU-NAV-SESSION]: the world subsystem that OWNS menu navigation.
// This screen only calls its public registration API; it never edits it.
class USiegeMenuInputSubsystem;

/**
 *  C++ base for /Game/UI/WBP_SessionMenu (TASK-354; the SIGNED TASK-353
 *  architecture doc, section 5 / D11). TASK-355 builds the WBP fresh (never
 *  duplicate+reparent - the corruption law) reparented to this class, and adds
 *  ONE "Multiplayer" entry to the existing main menu beside "Play (vs Bot)",
 *  which stays untouched (ruling 3).
 *
 *  The doc contract TASK-355 builds on - exactly these names:
 *  - BlueprintCallable wrappers: HostPressed() / JoinPressed(AddressText) /
 *    BackPressed() - wire the WBP buttons to these.
 *  - BlueprintImplementableEvents: OnSessionStatusUpdated(StatusText) /
 *    OnSessionErrorShown(ErrorText) - FString-only params (widget law).
 *
 *  CONVENIENCE ON TOP (all OPTIONAL - BindWidgetOptional, never required):
 *  if the WBP names its widgets exactly HostButton / JoinButton / BackButton
 *  (Button), AddressTextBox (EditableTextBox), StatusTextBlock / ErrorTextBlock
 *  (TextBlock), this base auto-wires the button clicks and keeps the two text
 *  blocks updated with ZERO graph work - the BIEs still fire for any extra
 *  presentation. A WBP that uses different names instead wires its buttons to
 *  the three wrappers and implements the two BIEs. Both routes are fully
 *  supported; nothing here hard-requires a named widget.
 *
 *  All session behavior lives in USiegeSessionSubsystem (host = listen travel,
 *  join = validated direct IP, leave = menu travel, error surfaces). This
 *  widget is a thin, null-safe forwarding shell: no subsystem resolves to a
 *  local error message, never a crash. Practice mode ("Play vs Bot") never
 *  routes through this widget (ruling 3).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Host button entry: forwards to USiegeSessionSubsystem::HostListenMatch (null-safe). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Session")
	void HostPressed();

	/**
	 *  Join button entry: forwards AddressText to
	 *  USiegeSessionSubsystem::JoinMatch. Validation lives in the subsystem -
	 *  an invalid address comes straight back through the error surface
	 *  (OnSessionErrorShown) with no travel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Session")
	void JoinPressed(const FString& AddressText);

	/**
	 *  Back button entry. If a networked session or a pending join connection
	 *  exists, this routes to USiegeSessionSubsystem::LeaveMatch (canceling the
	 *  pending join / leaving the match back to the menu). In the plain
	 *  standalone menu it performs the panel dismissal ITSELF, C++-side:
	 *  resolve /Game/UI/WBP_MainMenu.WBP_MainMenu_C -> CreateWidget on the
	 *  owning player -> AddToViewport -> THEN RemoveFromParent on self
	 *  (add-before-remove law: no frame ever renders with neither widget). A
	 *  failed resolve leaves this panel up and surfaces via ShowLocalError -
	 *  never a zero-UI viewport. It still deliberately does NOT reload
	 *  L_MainMenu: standalone Back is a viewport widget swap, never LeaveMatch
	 *  travel (a reload would flicker-reset the menu for no reason).
	 *
	 *  SUPERSEDED (2026-08-28, the SESSION-BACK ruling in CONVENTIONS, off
	 *  VID-002): the TASK-354 flagged decision deferred standalone dismissal
	 *  to "the WBP's own navigation", but the TASK-355 route-(A) zero-graph
	 *  WBP never authored that navigation - measured on pixels, nobody closed
	 *  the panel. The decision is REVERSED to C++-side dismissal.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Session")
	void BackPressed();

	/**
	 *  Implemented by WBP_SessionMenu (TASK-355): show session STATUS text
	 *  ("Hosting - waiting for opponent", "Joining 192.168.1.50:7777 ...").
	 *  FString-only params (widget law). Fires on every status broadcast, even
	 *  when the optional StatusTextBlock is bound and already updated by C++.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Session")
	void OnSessionStatusUpdated(const FString& StatusText);

	/**
	 *  Implemented by WBP_SessionMenu (TASK-355): show session ERROR text
	 *  (invalid IP, connection timeout, travel failure). FString-only params
	 *  (widget law). Fires on every error broadcast, even when the optional
	 *  ErrorTextBlock is bound and already updated by C++.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Session")
	void OnSessionErrorShown(const FString& ErrorText);

protected:

	//~ Begin UUserWidget interface
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** OnClicked thunk for the optional HostButton binding. */
	UFUNCTION()
	void HandleHostClicked();

	/** OnClicked thunk for the optional JoinButton binding: reads AddressTextBox (empty when unbound) and forwards to JoinPressed. */
	UFUNCTION()
	void HandleJoinClicked();

	/** OnClicked thunk for the optional BackButton binding. */
	UFUNCTION()
	void HandleBackClicked();

	/** Subsystem OnSessionStatus handler: updates StatusTextBlock (clears ErrorTextBlock - a new status supersedes a stale error), then fires OnSessionStatusUpdated. */
	UFUNCTION()
	void HandleSessionStatus(const FString& Message);

	/** Subsystem OnSessionError handler: updates ErrorTextBlock, then fires OnSessionErrorShown. */
	UFUNCTION()
	void HandleSessionError(const FString& Message);

	/** Null-safe subsystem resolve through this widget's world's game instance. */
	USiegeSessionSubsystem* ResolveSessionSubsystem() const;

	/** Surfaces a WIDGET-LOCAL failure (subsystem unresolvable) through the same error path the subsystem uses: ErrorTextBlock + OnSessionErrorShown + log. */
	void ShowLocalError(const FString& Message);

	// ═══════════════════════════════════════════════════════════════════════════
	//  TASK-1425 [MENU-NAV-SESSION] — THE ACTIVE-NAV-TARGET HANDSHAKE, AND
	//  NOTHING ELSE
	//
	//  This screen's ENTIRE contribution to keyboard / gamepad navigation is
	//  saying WHEN it is the screen the player is looking at. Up / Down / Left /
	//  Right / Accept / Back are owned by USiegeMenuInputSubsystem (TASK-1406 +
	//  TASK-1409); the walker that enumerates stops, reads IsFocusable, places
	//  the ring and logs the count lives THERE.
	//
	//  ⛔ WHAT IS DELIBERATELY NOT HERE: no NativeOnKeyDown, no
	//  NativeOnPreviewKeyDown, no SetKeyboardFocus, no navigation rule table, no
	//  IsFocusable write, no ISiegeMenuNavCloseTarget implementation. Each of
	//  those would put input handling back in the leaf widget, which is the
	//  defect the MENU-NAV epic is removing; spec (4) forbids a new key handler
	//  outright, and the interface is TASK-1454's row by the subsystem header's
	//  own words ("IMPLEMENTING THIS IS NOT THIS ROW'S WORK ... TASK-1454 wires
	//  the screens"). ⇒ IA_MenuBack is inert on this screen today, exactly as it
	//  is on Settings and Graphics.
	//
	//  ⭐ THE FOCUS-STOP CENSUS FOR /Game/UI/WBP_SessionMenu — READ OUT OF THE
	//  ASSET ITSELF, not guessed: the package export table plus the root
	//  UCanvasPanel's Slots array in Content/UI/WBP_SessionMenu.uasset. FOUR
	//  stops, in UWidgetTree::ForEachWidget order (depth-first pre-order,
	//  UPanelWidget children in GetChildAt(0..N-1) SLOT order):
	//
	//      stop 0   HostButton       UButton            asset-authored
	//      stop 1   JoinButton       UButton            asset-authored
	//      stop 2   BackButton       UButton            asset-authored
	//      stop 3   AddressTextBox   UEditableTextBox   asset-authored
	//
	//  The eight non-stops, in the same walk: CanvasPanel (root, UCanvasPanel) ·
	//  HostLabelText / JoinLabelText / BackLabelText (UTextBlock, each button's
	//  own content, walked because UButton is a UContentWidget : UPanelWidget,
	//  and correctly rejected) · StatusTextBlock / ErrorTextBlock / TitleText
	//  (UTextBlock) · BackdropBorder (UBorder). None of the eight is one of the
	//  four admitted classes. 12 widgets walked + this UUserWidget root = the 13
	//  nodes TASK-1399 §2 row 6 counted live.
	//
	//  ⛔ THE COUNT IS FIXED AT FOUR, IN EVERY STATE. There is no runtime-
	//  populated list on this screen: joining is a typed IPv4 address, not a
	//  server browser, so there is no empty-list count distinct from a populated
	//  one. Nothing here is ever collapsed, hidden or disabled by C++ — the two
	//  status surfaces are UTextBlocks whose TEXT changes, never their
	//  visibility — so the TASK-1418 hidden-panel hazard (IsNavFocusStop reads a
	//  widget's OWN Slate visibility, never its ancestors') has no subject here.
	//  ⇒ any reading other than FOUR is a defect, and the discriminator is the
	//  NAME that is missing, not the number.
	//
	//  🚨 THE ONE THING A READER MUST NOT ASSUME, AND IT IS MEASURED: ring order
	//  is SLOT order, and this tree's root is a UCanvasPanel whose children are
	//  ABSOLUTELY POSITIONED — so slot order is arbitrary with respect to what
	//  🧑 he sees. This is the exact caveat TASK-1406 declared at
	//  SiegeMenuInputSubsystem.cpp's GetMenuFocusStops and that TASK-1415 dodged
	//  by construction; this is the screen where it BITES. Measured slot order
	//  is Host → Join → Back → Status → Error → Address → Backdrop → Title,
	//  giving a ring of Host → Join → Back → AddressTextBox → (wrap), while the
	//  panel READS top-to-bottom AddressTextBox → Host → Join → Back. All four
	//  stops are reachable and the ring wraps; only the SEQUENCE is not visually
	//  monotone, and the ring opens on Host rather than on the visually topmost
	//  control. ⛔ NOT FIXED HERE: the fix is a slot reorder inside the .uasset,
	//  and this row's fence is "SessionMenuWidget.{cpp,h} ONLY, never an asset".
	//  Escalated by letter in handoffs/TASK-1425-programmer.md.
	// ═══════════════════════════════════════════════════════════════════════════

	/**
	 *  Hand menu navigation to THIS screen. Called from NativeConstruct, AFTER
	 *  the subsystem delegates are bound, and paired with
	 *  UnregisterAsMenuNavTarget on every exit.
	 *
	 *  ⭐ TASK-1406's RegisterMenuNavTarget also PLACES THE RING on stop 0, so
	 *  the panel opens with HostButton already outlined and the FIRST Down moves
	 *  to stop 1 (JoinButton), not to stop 0. A reader expecting stop 0 after one
	 *  Down will mis-read a working screen as broken.
	 *
	 *  Null-safe: no world or no subsystem ⇒ one Log line and the screen behaves
	 *  exactly as it did before this row (mouse-only). Never fatal.
	 */
	void RegisterAsMenuNavTarget();

	/**
	 *  Give menu navigation back. Called from BOTH BackPressed() — after the
	 *  replacement main menu is in the viewport and after the failed-resolve
	 *  early return, immediately before RemoveFromParent() — and
	 *  NativeDestruct(). The double call is deliberate and safe:
	 *  USiegeMenuInputSubsystem::UnregisterMenuNavTarget removes by IDENTITY and
	 *  logs-not-warns a second call for a screen already gone.
	 *
	 *  ⛔ IT IS NOT CALLED ON THE LeaveMatch() LEG, AND THAT IS MEASURED RATHER
	 *  THAN FORGOTTEN. USiegeSessionSubsystem::LeaveMatch
	 *  (SiegeSessionSubsystem.cpp:130-153) is UGameplayStatics::OpenLevel with
	 *  bAbsolute = true — a DEFERRED travel processed at the end of the tick,
	 *  with NO RemoveFromParent on this panel. The panel therefore stays on
	 *  screen, and rightly keeps the ring, for the rest of that frame and beyond;
	 *  unregistering there would strand the ring on a screen 🧑 he is still
	 *  looking at. The world — and this UWorldSubsystem with it — then tears
	 *  down, and NativeDestruct is the honest hook for that path.
	 */
	void UnregisterAsMenuNavTarget();

	/**
	 *  Null-safe resolve of the menu-input subsystem. ⚠️ Unlike
	 *  USiegeSessionSubsystem this one lives on the WORLD, not on the game
	 *  instance, and it declines Editor worlds outright
	 *  (DoesSupportWorldType = Game | PIE only), so a null answer is ordinary
	 *  rather than an error.
	 */
	USiegeMenuInputSubsystem* ResolveMenuInputSubsystem() const;

	/** OPTIONAL binding: the Host button. When bound, OnClicked auto-wires to HandleHostClicked. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Session", meta = (BindWidgetOptional))
	TObjectPtr<UButton> HostButton;

	/** OPTIONAL binding: the Join button. When bound, OnClicked auto-wires to HandleJoinClicked. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Session", meta = (BindWidgetOptional))
	TObjectPtr<UButton> JoinButton;

	/** OPTIONAL binding: the Back button. When bound, OnClicked auto-wires to HandleBackClicked. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Session", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	/** OPTIONAL binding: the IP entry box the auto-wired Join click reads. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Session", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> AddressTextBox;

	/** OPTIONAL binding: status line - kept updated by C++ when bound. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Session", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusTextBlock;

	/** OPTIONAL binding: error line - kept updated by C++ when bound. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Session", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ErrorTextBlock;
};
