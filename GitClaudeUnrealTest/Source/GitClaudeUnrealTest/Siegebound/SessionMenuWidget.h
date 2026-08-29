// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SessionMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class USiegeSessionSubsystem;

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
