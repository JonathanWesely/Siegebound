// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeckSlotEntryWidget.generated.h"

/**
 *  Gesture delegate for one deck-bar entry (DECK-§8, pinned). Plain non-dynamic
 *  delegate — bound in C++ by UDeckBuilderWidget only, never exposed to BP.
 *  The payload is the entry's 0-based fixed slot index.
 */
DECLARE_DELEGATE_OneParam(FOnDeckSlotGesture, int32 /*SlotIndex*/);

/**
 *  TASK-671 [DB3-3] — ONE deck-bar entry: a bordered, labeled button for a
 *  fixed deck slot ("deck1".."deck10"), left-clickable (select for EDITING) and
 *  RIGHT-clickable (make ACTIVE for the next match).
 *
 *  ============================================================================
 *  M8 DECLARATION: Adds no replicated property, no new replicated class, no new
 *  relevancy tier, no RPC. This is client-local menu UI only.
 *  ============================================================================
 *
 *  ---------------------------------------------------------------------------
 *  WHY THERE IS NO .uasset — THE RULING THIS WIDGET IS BUILT UNDER (DECK-§5)
 *  ---------------------------------------------------------------------------
 *  CONVENTIONS DECK-§5 authorises a CODE-AUTHORED tree for UDeckSlotEntryWidget
 *  — a fresh, narrow ruling argued on THIS widget's facts (the ACC-§5 shape,
 *  re-argued as its five conditions demand; it inherits neither the settings,
 *  assistant-console nor account-menu rulings, and no other widget may cite it):
 *   (a) SCOPE — this class ONLY.
 *   (b) THE ORDER (TASK-444, paid for once already): the tree is built and
 *       WidgetTree->RootWidget is set FIRST, then `return Super::RebuildWidget();`.
 *       Anything constructed after Super is discarded and the widget renders
 *       EMPTY while passing every property readback.
 *   (c) /Game/UI/WBP_DeckSlotEntry is RESERVED, NOT AUTHORED. An asset-authored
 *       tree using these exact child names would bind here and win whole.
 *   (d) Children are BindWidgetOptional, constructed only while still null.
 *   (e) Render/interaction correctness closes on PIXELS (TASK-674) and
 *       Jonathan's hands (TASK-675) — never on tree/property readback.
 *
 *  ---------------------------------------------------------------------------
 *  THE GESTURE CONTRACT (DECK-§3 / DECK-§5, pinned)
 *  ---------------------------------------------------------------------------
 *  Tree: OutlineBorder (UBorder, the outline carrier) > SlotButton (UButton) >
 *  SlotLabelText (UTextBlock).
 *
 *  LMB: SButton answers the left/touch gesture — SlotButton->OnClicked →
 *  HandleSlotButtonClicked → OnLeftClicked(SlotIndex).
 *
 *  RMB: an RMB press falls through SButton entirely (⛔ a Button OnClicked can
 *  NEVER deliver right-click — the dead-gesture trap DECK-§5 exists to kill),
 *  bubbles up the widget path, and is caught in NativeOnMouseButtonDown:
 *  EKeys::RightMouseButton → OnRightClicked(SlotIndex) → FReply::Handled().
 *  EVERYTHING else returns Super's reply so the left press keeps reaching the
 *  inner button unchanged.
 *
 *  ⛔ No key handling exists anywhere in this class — Escape stays permanently
 *  unabsorbed, project-wide (AS-§6 A-2).
 *
 *  ---------------------------------------------------------------------------
 *  THE TWO STATES (DECK-§3 — one meaning per channel, never conflated)
 *  ---------------------------------------------------------------------------
 *  ACTIVE  ("this is my match deck") = the ORANGE OUTLINE: the border's rim
 *          tinted DeckActiveOutlineColor (the pinned orange, ONE definition in
 *          DeckSlotEntryWidget.cpp) vs fully transparent. SetOutlineActive is
 *          the ONLY outline writer.
 *  EDITING ("the grid edits this deck") = the SlotButton FILL TINT only — a
 *          subtler, non-orange highlight, deliberately a different visual
 *          channel so both states can coexist on one entry without a second
 *          outline (two outlines on one bar is the confusable-signal defect
 *          class).
 *
 *  This widget renders states and reports gestures; it holds NO deck model. It
 *  never composes a deck name (DECK-§1 one-composer law: labels arrive from
 *  USiegeDeckSaveGame::MakeFixedDeckName via SetSlotIndexAndLabel) and never
 *  touches the SaveGame. UDeckBuilderWidget owns all model routing.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UDeckSlotEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Stamp this entry's 0-based fixed slot index and its label text (the caller passes MakeFixedDeckName(SlotIndex) — DECK-§1: this class never composes a name). ORDER-INDEPENDENT [BLOCKER 674-1]: both are stored in members unconditionally; the label is applied to SlotLabelText immediately when the child already exists, or from RebuildWidget() when the tree comes alive after the stamp. */
	void SetSlotIndexAndLabel(int32 InSlotIndex, const FString& Label);

	/** ACTIVE state (DECK-§3): tint the OutlineBorder rim orange or fully transparent. Idempotent; null-safe. */
	void SetOutlineActive(bool bActive);      // DeckActiveOutlineColor vs transparent — the ONLY outline writer

	/** EDITING state (DECK-§3): tint the SlotButton background — never an outline, never orange. Idempotent; null-safe. */
	void SetEditingHighlight(bool bEditing);  // SlotButton fill tint ONLY (DECK-§3)

	FOnDeckSlotGesture OnLeftClicked;         // from SlotButton->OnClicked
	FOnDeckSlotGesture OnRightClicked;        // from NativeOnMouseButtonDown, RMB only

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;  // root-first, then Super (TASK-444 order)
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/**
	 *  OnClicked thunk for SlotButton (dynamic delegates need a UFUNCTION —
	 *  the registry's "from SlotButton->OnClicked" lane). Forwards to
	 *  OnLeftClicked(SlotIndex).
	 */
	UFUNCTION()
	void HandleSlotButtonClicked();

	// Pinned children (DECK-§7/§8, character-for-character). BindWidgetOptional
	// by law (DECK-§5(d)): constructed in ConstructEntryTree only while null.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UBorder>     OutlineBorder;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UButton>     SlotButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UTextBlock>  SlotLabelText;

private:

	/**
	 *  Builds the code-authored tree. Called from RebuildWidget() BEFORE
	 *  Super::RebuildWidget() — the order is load-bearing (DECK-§5(b), the
	 *  TASK-444 shape). Returns immediately when an asset-authored tree is
	 *  present (the DECK-§5(c) escape hatch — never exercised while
	 *  WBP_DeckSlotEntry stays reserved-unused).
	 */
	void ConstructEntryTree();

	/**
	 *  [BLOCKER 674-1] Writes StoredSlotLabel onto SlotLabelText. No-op until
	 *  BOTH sides exist: the stamp (SlotIndex != INDEX_NONE — fences an
	 *  un-stamped entry so an asset-authored design-time label is never
	 *  blanked) and the SlotLabelText child. Idempotent; called from the two
	 *  possible timings — SetSlotIndexAndLabel (stamp-after-tree) and
	 *  RebuildWidget (tree-after-stamp, the shipping order). The stored member
	 *  is the ONE label source (DECK-§1: composed only by the caller).
	 */
	void ApplyStoredLabel();

	/** This entry's 0-based fixed deck slot; INDEX_NONE until SetSlotIndexAndLabel stamps it. */
	int32 SlotIndex = INDEX_NONE;

	/** The label as stamped by SetSlotIndexAndLabel — stored UNCONDITIONALLY so a stamp landing before the tree exists survives until ApplyStoredLabel can write it (the BLOCKER 674-1 ordering hole). Never composed in this class (DECK-§1). */
	FString StoredSlotLabel;
};
