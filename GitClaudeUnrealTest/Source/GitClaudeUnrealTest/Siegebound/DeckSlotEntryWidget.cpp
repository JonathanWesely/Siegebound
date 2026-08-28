// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeckSlotEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "GitClaudeUnrealTest.h"
#include "InputCoreTypes.h"

namespace
{
	/**
	 *  DECK-§3 / DECK-§7 (pinned, character-for-character): THE one orange —
	 *  "this is my match deck". ⛔ ONE definition, THIS file, by law; no other
	 *  site may define or restate it, and no other visual may use it.
	 */
	const FLinearColor DeckActiveOutlineColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

	/** The rim when the entry is NOT active: fully transparent — the outline vanishes without moving a pixel of layout. */
	const FLinearColor DeckOutlineInactiveColor(0.0f, 0.0f, 0.0f, 0.0f);

	/**
	 *  EDITING fill tint (DECK-§3: SlotButton background ONLY, never a second
	 *  outline): a subtle steel blue, deliberately far from the pinned orange so
	 *  the two states can never be confused — and both can coexist on the one
	 *  entry that is active AND being edited.
	 */
	const FLinearColor DeckEditingFillTint(0.55f, 0.70f, 0.95f, 1.0f);

	/** The at-rest SlotButton background (UButton's stock white tint). */
	const FLinearColor DeckNeutralFillTint(1.0f, 1.0f, 1.0f, 1.0f);

	/** Outline thickness: the OutlineBorder padding ring the rim color shows through. */
	constexpr float DeckOutlineThickness = 3.0f;
}

TSharedRef<SWidget> UDeckSlotEntryWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING (DECK-§5(b), the corrected TASK-444 shape,
	// cloned from UAccountMenuWidget). UUserWidget::RebuildWidget() reads
	// WidgetTree->RootWidget AS IT STANDS when called and returns an SSpacer
	// when it is null — so the code-authored tree MUST exist BEFORE
	// Super::RebuildWidget(); building it afterwards yields a silently EMPTY
	// widget that still passes every property readback (the defect class
	// DECK-§5(e)'s pixel law exists for).
	//
	// Initialize() FIRST — the WARN-437-1 hardening, cloned: WidgetTree is
	// allocated inside Initialize(), and the call is public and idempotent.
	Initialize();
	ConstructEntryTree();

	// [BLOCKER 674-1] Apply the stored label now that the tree exists. The
	// shipping caller stamps SetSlotIndexAndLabel on a CreateWidget-fresh
	// entry BEFORE AddChildToHorizontalBox lazily triggers this rebuild via
	// TakeWidget() — at stamp time SlotLabelText was null and the write had
	// nowhere to land. Re-applying the stored member here makes the stamp
	// order-independent (and survives any future re-parent/rebuild).
	ApplyStoredLabel();

	return Super::RebuildWidget();
}

void UDeckSlotEntryWidget::ConstructEntryTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("UDeckSlotEntryWidget: no WidgetTree - the entry cannot build its tree."));
		return;
	}

	// The DECK-§5(c) escape hatch: an asset-authored tree using these child
	// names wins WHOLE (UMG already resolved the BindWidgetOptional members
	// from it). Never exercised this wave — /Game/UI/WBP_DeckSlotEntry is
	// RESERVED and no task authors it.
	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	// ---- OutlineBorder: the outline carrier, and the tree root --------------
	if (OutlineBorder == nullptr)
	{
		OutlineBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OutlineBorder"));
	}

	if (OutlineBorder == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("UDeckSlotEntryWidget: could not construct OutlineBorder - the entry has no root."));
		return;
	}

	// The rim IS the outline (DECK-§3): the border's brush shows only through
	// the padding ring around the button, so tinting the brush orange draws the
	// active outline and tinting it transparent removes it — the entry never
	// changes size between states (a bar that shifts on right-click would fail
	// the pixel gate). Constructed INACTIVE; UDeckBuilderWidget's
	// RefreshDeckBarStates assigns the truth right after the bar is built.
	OutlineBorder->SetPadding(FMargin(DeckOutlineThickness));
	OutlineBorder->SetBrushColor(DeckOutlineInactiveColor);
	OutlineBorder->SetHorizontalAlignment(HAlign_Fill);
	OutlineBorder->SetVerticalAlignment(VAlign_Fill);

	// Set BEFORE Super::RebuildWidget() reads it — DECK-§5(b), load-bearing.
	WidgetTree->RootWidget = OutlineBorder;

	// ---- SlotButton ---------------------------------------------------------
	if (SlotButton == nullptr)
	{
		SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotButton"));
	}

	if (SlotButton == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("UDeckSlotEntryWidget: could not construct SlotButton - the entry cannot be clicked."));
		return;
	}

	SlotButton->SetBackgroundColor(DeckNeutralFillTint);
	OutlineBorder->SetContent(SlotButton);

	// ---- SlotLabelText ------------------------------------------------------
	// No text is set here: the label arrives via SetSlotIndexAndLabel from the
	// ONE composer (USiegeDeckSaveGame::MakeFixedDeckName) — this file never
	// contains a deck-name literal (DECK-§1).
	if (SlotLabelText == nullptr)
	{
		SlotLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotLabelText"));
		if (SlotLabelText != nullptr)
		{
			SlotLabelText->SetFontSize(20.f);

			if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(SlotButton->SetContent(SlotLabelText)))
			{
				ContentSlot->SetPadding(FMargin(10.f, 6.f, 10.f, 6.f));
				ContentSlot->SetHorizontalAlignment(HAlign_Center);
				ContentSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else
		{
			// Not fatal: an unlabeled button still clicks; the pixel gate
			// (TASK-674) would catch a blank bar long before Jonathan does.
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UDeckSlotEntryWidget: could not construct SlotLabelText - the entry renders unlabeled."));
		}
	}
}

void UDeckSlotEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bound HERE and not in NativeOnInitialized, deliberately (the settings-lane
	// lifecycle trap, cloned via UAccountMenuWidget): the code-authored children
	// do not exist until RebuildWidget() runs, and the engine order is
	// Initialize() -> NativeOnInitialized() -> RebuildWidget() -> NativeConstruct().
	if (SlotButton != nullptr)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UDeckSlotEntryWidget::HandleSlotButtonClicked);
	}
}

void UDeckSlotEntryWidget::NativeDestruct()
{
	// Symmetric unbind (defensive — the UAccountMenuWidget shape).
	if (SlotButton != nullptr)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UDeckSlotEntryWidget::HandleSlotButtonClicked);
	}

	Super::NativeDestruct();
}

void UDeckSlotEntryWidget::HandleSlotButtonClicked()
{
	// LMB = "select this deck for EDITING" (D7, DECK-§3). The meaning lives in
	// the receiver (UDeckBuilderWidget routes to SelectDeckForEdit); this entry
	// only reports the gesture and its slot.
	OnLeftClicked.ExecuteIfBound(SlotIndex);
}

FReply UDeckSlotEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// DECK-§5: the RIGHT-CLICK mechanism lives HERE and only here. SButton
	// answers the left/touch gesture only — an RMB press falls through
	// OnClicked entirely, bubbles up the widget path, and lands in this
	// override. ⛔ Wiring right-click to a Button OnClicked is a dead gesture
	// by law.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// RMB = "make this my match deck" (DECK-§3). Handled() stops the press
		// from bubbling further — the gesture is consumed by the bar entry.
		OnRightClicked.ExecuteIfBound(SlotIndex);
		return FReply::Handled();
	}

	// Everything else returns Super's reply so the LEFT press keeps reaching
	// the inner SlotButton unchanged (DECK-§5's pinned contract). No key
	// handling exists in this class — Escape stays unabsorbed (AS-§6 A-2).
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDeckSlotEntryWidget::SetSlotIndexAndLabel(int32 InSlotIndex, const FString& Label)
{
	// [BLOCKER 674-1] Store UNCONDITIONALLY — never gate the stamp on child
	// liveness. The old null-guarded SetText silently skipped when the caller
	// stamped before the tree existed, rendering all ten bar labels blank.
	// The stored member is THE single label source (DECK-§1: composed only by
	// the caller via MakeFixedDeckName, never in this class), applied at both
	// possible timings: immediately below when the child already exists, and
	// from RebuildWidget() when the tree comes alive after the stamp.
	SlotIndex = InSlotIndex;
	StoredSlotLabel = Label;

	ApplyStoredLabel();
}

void UDeckSlotEntryWidget::ApplyStoredLabel()
{
	// No-op until BOTH sides exist — the stamp fence (SlotIndex != INDEX_NONE)
	// keeps an un-stamped entry from blanking an asset-authored label via the
	// DECK-§5(c) escape hatch. Idempotent: safe at every rebuild.
	if (SlotIndex != INDEX_NONE && SlotLabelText != nullptr)
	{
		SlotLabelText->SetText(FText::FromString(StoredSlotLabel));
	}
}

void UDeckSlotEntryWidget::SetOutlineActive(bool bActive)
{
	// The ONLY outline writer (DECK-§8). One meaning: "this is my match deck."
	if (OutlineBorder != nullptr)
	{
		OutlineBorder->SetBrushColor(bActive ? DeckActiveOutlineColor : DeckOutlineInactiveColor);
	}
}

void UDeckSlotEntryWidget::SetEditingHighlight(bool bEditing)
{
	// SlotButton fill tint ONLY (DECK-§3) — never an outline, never orange.
	if (SlotButton != nullptr)
	{
		SlotButton->SetBackgroundColor(bEditing ? DeckEditingFillTint : DeckNeutralFillTint);
	}
}
