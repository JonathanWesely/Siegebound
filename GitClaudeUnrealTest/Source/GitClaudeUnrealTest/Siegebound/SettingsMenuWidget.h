// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenuWidget.generated.h"

class UBorder;
class UButton;
class UCheckBox;
class UTextBlock;
class UVerticalBox;
class USiegeSettingsSubsystem;

/**
 *  TASK-437 [SET-2] - the settings screen.
 *
 *  ============================================================================
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no new
 *  relevancy tier.
 *  It is true by construction rather than by inspection: this widget only reads
 *  and writes USiegeSettingsSubsystem, whose single value is CLIENT-LOCAL - it
 *  governs a LOCAL review step (whether the player is shown the parsed order
 *  before it executes) and never an authoritative outcome. Nothing here is ever
 *  sent to, or read by, the server.
 *  ============================================================================
 *
 *  ---------------------------------------------------------------------------
 *  WHY THERE IS NO .uasset - THE RULING THIS WIDGET IS BUILT UNDER
 *  ---------------------------------------------------------------------------
 *  CONVENTIONS "Settings screen + the assistant CONFIRM STEP + the
 *  non-orderable-kind guard (2026-08-03)" SECTION 3 authorises a CODE-AUTHORED
 *  widget tree for USettingsMenuWidget, and TASKBOARD SETTINGS+CONFIRM manager
 *  ruling 8 records it.
 *
 *  That is a SECOND, SEPARATE, NAMED ruling. It is NOT "In-match LLM command
 *  assistant" section 6 ruling A - that one is scoped to
 *  USiegeAssistantConsoleWidget ONLY and says in as many words that citing it
 *  for another widget is a misuse. Neither ruling inherits from the other, and
 *  a THIRD widget may cite neither.
 *
 *  Section 3's grounds, briefly:
 *   1. The "U<Name>Widget <-> WBP_<Name>" law exists so an artist's asset name
 *      and a programmer's code reference cannot drift. With no asset there is
 *      nothing to drift from - the law is satisfied vacuously, not broken.
 *   2. The alternative is this project's most expensive UI failure mode. A new
 *      WBP_ starts as a DUPLICATE of a donor under the template-donor rule, and
 *      a duplicated+reparented WidgetBlueprint has SILENTLY BROKEN RUNTIME
 *      REPAINT here - design time looked perfect and it cost ~9 wasted fixes.
 *      A code-authored tree cannot have that defect.
 *   3. The one route avoiding both - hand-authoring a fresh design-time tree -
 *      is a JONATHAN HAND-STEP (TASK-355 proved unreal.new_object design-time
 *      widgets skip the designer's GUID registration). A human step on the
 *      critical path is not owed for a two-control panel.
 *
 *  The five conditions of that ruling, all of which are QA criteria:
 *   (a) SCOPE - this class only.
 *   (b) THE ESCAPE HATCH IS BUILT IN. Every child below is
 *       UPROPERTY(meta=(BindWidgetOptional)) with the exact pinned name, and
 *       ConstructSettingsTree() constructs a child only when it is still null.
 *       An asset-authored tree wins WHOLE and the code branch never runs.
 *   (c) /Game/UI/WBP_SettingsMenu is RESERVED, NOT AUTHORED. Nothing else may
 *       take that path; taking the fallback costs one art task and ZERO C++.
 *   (d) THE CONTRACT IS THE SHIPPED USessionMenuWidget CONTRACT, CLONED EXACTLY
 *       - BindWidgetOptional members, BlueprintCallable wrappers, and
 *       FString/int32/bool/uint8-only BlueprintImplementableEvents. Never an
 *       enum or a struct in a BIE parameter.
 *   (e) VERIFICATION IS A HUMAN PIXEL CHECK. There is no .uasset to read back,
 *       and MCP readback has repeatedly passed on visually-broken UMG on this
 *       project (TASK-355: six controls read back 6/6 correct while stacked in
 *       a 165x48 px box in the corner - a binding readback cannot see
 *       geometry). NOTHING ON SCREEN IS VERIFIED BY THIS FILE'S AUTHOR. The
 *       checklist Jonathan must look at is in handoffs/TASK-437-programmer.md.
 *
 *  ---------------------------------------------------------------------------
 *  NAVIGATION (section 4) - THE PANEL SITS ON TOP OF THE MAIN MENU
 *  ---------------------------------------------------------------------------
 *  TASK-438 adds Btn_Settings to the existing /Game/UI/WBP_MainMenu:
 *      CreateWidget(USettingsMenuWidget) -> AddToViewport(ZOrder 10)
 *  and deliberately does NOT RemoveFromParent the main menu. BackPressed()
 *  therefore calls RemoveFromParent() on THIS widget and nothing else - the
 *  menu underneath was never removed, is already alive and already correct.
 *  Re-creating it would put navigation state in the leaf plus a soft asset path
 *  to get wrong.
 *
 *  ==> THAT IS WHY BackdropBorder IS HIT-TEST **VISIBLE**, AND IT IS
 *      CORRECTNESS, NOT STYLING. It is the OPPOSITE of WBP_SessionMenu's
 *      backdrop: TASK-355's plate was deliberately HIT_TEST_INVISIBLE because
 *      it sat over nothing clickable. Copying that choice here would ship a
 *      live click-through into Play / Sandbox / Deck Builder / QUIT while the
 *      panel looks modal.
 *      QA criterion: with the panel open, a click on the backdrop reaches
 *      NOTHING behind it.
 *
 *  ---------------------------------------------------------------------------
 *  DEPENDENCY
 *  ---------------------------------------------------------------------------
 *  Reads/writes USiegeSettingsSubsystem (TASK-436, same batch, pinned in
 *  CONVENTIONS section 8). This file WILL NOT COMPILE ALONE and is not expected
 *  to - the TASK-416/417 precedent; the batch links together at TASK-447's
 *  single compile gate. Do not open a QA loop over it.
 *
 *  Null-safe throughout: no subsystem resolvable => the setting row renders
 *  DISABLED with a hint saying why, logs ONCE, and never crashes. That is the
 *  USessionMenuWidget::ShowLocalError shape. A settings screen that hard-faults
 *  on a missing subsystem is worse than no settings screen.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Back entry (CONVENTIONS section 8 pin). Calls RemoveFromParent() on this
	 *  widget AND NOTHING ELSE (section 4 / spec item 4). It does NOT re-create
	 *  or re-open the main menu - the menu is alive underneath.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings")
	void BackPressed();

	/**
	 *  Confirm-toggle entry (CONVENTIONS section 8 pin). Forwards to
	 *  USiegeSettingsSubsystem::SetAssistantConfirmEnabled, which writes the
	 *  in-memory value and saves the slot. Null-safe: no subsystem => the row
	 *  goes disabled with an explanatory hint, logged once, never a crash.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings")
	void ConfirmTogglePressed(bool bChecked);

	/**
	 *  TASK-1115 [GFX-PANEL] — the "Graphics" entry in the settings tree, and the
	 *  ONLY route into USiegeGraphicsMenuWidget.
	 *
	 *  It CreateWidget + AddToViewport(ZOrder 20) — ABOVE this panel's own 10 —
	 *  and DELIBERATELY DOES NOT remove this panel, which is the TASK-438
	 *  navigation shape cloned one level deeper:
	 *      WBP_MainMenu --(Btn_Settings)--> USettingsMenuWidget @ 10
	 *      USettingsMenuWidget --(GraphicsButton)--> Graphics   @ 20
	 *  ⇒ the Graphics panel's Back is RemoveFromParent() on ITSELF and nothing
	 *  else, and the player lands back here on a panel that was never destroyed.
	 *
	 *  🚨 /Game/UI/WBP_MainMenu IS NEVER OPENED by this lane (GFX-§1 / board
	 *  cl. 6): the entry lives in THIS code-authored tree, so the whole graphics
	 *  feature costs ZERO .uasset writes and ZERO Jonathan UMG minutes.
	 *
	 *  Null-safe: no owning player controller ⇒ logged, no panel, never fatal.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings")
	void GraphicsPressed();

	/**
	 *  Fired at the widget for presentation on top of what C++ already does to
	 *  the bound controls - once when the row is SEEDED (so a future
	 *  WBP_SettingsMenu never opens stale: the qa/TASK-005 major-2 lesson) and
	 *  again whenever the observed value actually CHANGES.
	 *
	 *  It does NOT fire on a no-op (the delegate law: a delegate that fires on
	 *  refused or unchanged writes trains consumers to ignore it).
	 *
	 *  FString/bool params only - the widget-param law. Never an enum, never a
	 *  struct.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Settings")
	void OnSettingsValueChanged(const FString& SettingName, bool bValue);

	/**
	 *  The setting name this widget reports through OnSettingsValueChanged:
	 *  TEXT("bAssistantConfirmBeforeExecute"), the CONVENTIONS section 2 / 8
	 *  pinned FIELD name.
	 *
	 *  Deliberately NOT the token USiegeSettingsSubsystem::OnSettingsChanged
	 *  broadcasts - that token is TASK-436's to choose and is NOT pinned, so
	 *  assuming it would be an unpinned cross-task guess. This widget passes
	 *  through a name it owns, and re-reads the value on ANY broadcast rather
	 *  than filtering on a string it cannot pin.
	 */
	static const TCHAR* ConfirmSettingName;

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** OnCheckStateChanged thunk for ConfirmToggleCheckBox. Forwards to ConfirmTogglePressed. */
	UFUNCTION()
	void HandleConfirmToggleChanged(bool bIsChecked);

	/** OnClicked thunk for BackButton. Forwards to BackPressed. */
	UFUNCTION()
	void HandleBackClicked();

	/** OnClicked thunk for GraphicsButton (TASK-1115). Forwards to GraphicsPressed. */
	UFUNCTION()
	void HandleGraphicsClicked();

	/**
	 *  USiegeSettingsSubsystem::OnSettingsChanged handler. Re-reads the confirm
	 *  value regardless of which setting the broadcast names (see
	 *  ConfirmSettingName for why the name is not filtered on).
	 */
	UFUNCTION()
	void HandleSettingsChanged(FName SettingName);

	/** Null-safe subsystem resolve through this widget's world's game instance. */
	USiegeSettingsSubsystem* ResolveSettingsSubsystem() const;

	/**
	 *  Builds the code-authored tree. Called from RebuildWidget() BEFORE
	 *  Super::RebuildWidget() - see the comment there, the order is load-bearing.
	 *  Returns immediately when an asset-authored tree is present (condition (b),
	 *  the escape hatch).
	 */
	void ConstructSettingsTree();

	/** Seed-then-bind: reads the live value, pushes it to the row, then arms the delegates. */
	void SeedAndBind();

	/** Symmetric unbind of everything SeedAndBind armed. */
	void UnbindAll();

	/**
	 *  Pushes a confirm value onto the row and fires OnSettingsValueChanged only
	 *  when the pushed value is new for this widget instance (first seed always
	 *  counts as new).
	 */
	void ApplyConfirmValueToRow(bool bValue);

	/**
	 *  The subsystem could not be resolved: disable the row, replace the hint
	 *  with the reason, log ONCE per widget instance. Never a crash.
	 */
	void ShowRowUnavailable();

	// ------------------------------------------------------------------------
	// PINNED CHILDREN - CONVENTIONS section 3, character-for-character.
	// All BindWidgetOptional, never BindWidget: an asset-authored
	// WBP_SettingsMenu using these exact names binds here and the code-authored
	// branch never runs (condition (b)). Nothing below is ever hard-required.
	// ------------------------------------------------------------------------

	/** Hit-test VISIBLE modal plate; the tree root. See the class comment - this is correctness, not styling. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BackdropBorder;

	/** The panel column: title, the one setting row, Back. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

	/** "Settings". */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** The one setting in v1: bAssistantConfirmBeforeExecute. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> ConfirmToggleCheckBox;

	/** The toggle's player-facing label. In the code-authored tree it is the check box's CONTENT, so clicking the words toggles too. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmToggleLabelText;

	/** The toggle's explanatory hint - and the surface that says WHY the row is disabled when the subsystem is missing. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmToggleHintText;

	/**
	 *  TASK-1115 [GFX-PANEL] — opens the Graphics submenu. GFX-§10 pins BOTH names
	 *  character-for-character (`GraphicsButton` + `GraphicsLabelText`), and board
	 *  cl. (6) pins the position: ABOVE BackButton, so Back stays last.
	 *  BindWidgetOptional like every other child — a future WBP_SettingsMenu using
	 *  these names binds here and the code-authored branch never runs.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UButton> GraphicsButton;

	/** "Graphics" — the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GraphicsLabelText;

	/** Dismisses this panel only. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	/** "Back" - the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackLabelText;

private:

	/** Last confirm value pushed to the row - drives the "no BIE on a no-op" rule. */
	bool bLastPushedConfirmValue = false;

	/** False until the first seed, so the first push always notifies. */
	bool bHasPushedConfirmValue = false;

	/** Latches the "subsystem unavailable" log to ONE line per widget instance. */
	bool bLoggedSubsystemUnavailable = false;
};
