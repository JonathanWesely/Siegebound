// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeAssistantConsoleWidget.h"

#include "SiegeAssistantCommand.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
// ⚠️ REQUIRED, AND NOT INHERITED FROM CoreMinimal.h — CHECKED, NOT ASSUMED.
// CoreMinimal.h does not include HAL/PlatformTime.h (grepped at the UE 5.8
// source on this machine: zero hits). FPlatformTime::Seconds() backs the
// re-open suppression window in OpenConsole(), so this include is load-bearing.
#include "HAL/PlatformTime.h"

namespace SiegeAssistantConsole
{
	/**
	 *  Static CHROME strings. ⛔ These are the ONLY player-visible words this
	 *  file authors, and none of them describes an order, a unit, a place or an
	 *  outcome. Every sentence with game meaning arrives through
	 *  ShowTranscriptLine / ShowConfirmPrompt / SetConsoleEnabled from the
	 *  component's reason-code template table (CONVENTIONS §3: the model emits
	 *  symbols only and the templates are the single source of player text).
	 */
	static const TCHAR* InputHintText   = TEXT("Type an order, then press Enter");
	static const TCHAR* AcceptLabelText = TEXT("Accept");
	static const TCHAR* CancelLabelText = TEXT("Cancel");

	/** Shown on open when the FSM has not pushed a state label yet. */
	static const TCHAR* IdleStatusText  = TEXT("Ready");

	/**
	 *  ⛔ THE HARD CEILING ON THE RE-OPEN SUPPRESSION WINDOW, APPLIED IN CODE ON
	 *  EVERY READ — not merely a ClampMax in the details panel.
	 *
	 *  The suppression exists to swallow AT MOST ONE re-open racing a single
	 *  keypress, which is a sub-frame event. Anything approaching a quarter of a
	 *  second stops being "the same press" and starts being "the console ignored
	 *  me", and THAT failure — a console that refuses to open — is strictly worse
	 *  than the one the window prevents. Clamping here means no configuration, no
	 *  .ini, no Blueprint default and no future editing hand can reach it.
	 */
	static constexpr float MaxReopenSuppressionSeconds = 0.25f;

	/**
	 *  Legibility is law here, not decoration — the console renders over live
	 *  combat, and this project has already shipped a white fill on a white
	 *  track (the M5.5 health-bar defect). The plate is dark and mostly opaque
	 *  so game-authored text stays readable over any terrain.
	 */
	static const FLinearColor BackdropColor    = FLinearColor(0.015f, 0.020f, 0.030f, 0.86f);
	static const FLinearColor TranscriptColor  = FLinearColor(0.95f, 0.95f, 0.98f, 1.0f);
	static const FLinearColor StatusColor      = FLinearColor(0.62f, 0.70f, 0.85f, 1.0f);

	static constexpr float TranscriptFontSize = 18.f;
	static constexpr float StatusFontSize     = 14.f;
	static constexpr float ButtonLabelSize    = 16.f;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

USiegeAssistantConsoleWidget* USiegeAssistantConsoleWidget::CreateAndAddToViewport(
	APlayerController* OwningController,
	TSubclassOf<USiegeAssistantConsoleWidget> ConsoleClass,
	int32 ZOrder)
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("[AssistantConsole] CreateAndAddToViewport: no owning player controller — no console was created. The assistant simply has no UI; nothing else changes."));
		return nullptr;
	}

	// A future /Game/UI/WBP_AssistantConsole is passed here and wins with zero
	// change to this file (ruling A(b)/(c)). In v1 nobody passes one.
	//
	// ⚠️ `.Get()` IS LOAD-BEARING, NOT TIDYING — it is the fix for C2445, and the
	// SAME defect shape appears again at ContentParent below. TSubclassOf carries
	// BOTH a non-explicit `TSubclassOf(UClass*)` constructor (SubclassOf.h:33) and
	// a non-explicit `operator UClass*()` (:115), so a conditional whose arms are
	// `TSubclassOf<T>` and `UClass*` has TWO equally good common types and the
	// compiler must refuse to choose. Collapsing BOTH arms to `UClass*` removes
	// the choice; the single remaining conversion is the assignment back.
	// ⛔ Behaviour is byte-identical: `operator UClass*()` and `Get()` are the same
	// call (`return **this`), so the truthiness test and the taken arm cannot
	// disagree — including on the IsChildOf check `operator*()` performs.
	const TSubclassOf<USiegeAssistantConsoleWidget> ResolvedClass =
		ConsoleClass ? ConsoleClass.Get() : USiegeAssistantConsoleWidget::StaticClass();

	USiegeAssistantConsoleWidget* Console =
		CreateWidget<USiegeAssistantConsoleWidget>(OwningController, ResolvedClass);

	if (Console == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("[AssistantConsole] CreateAndAddToViewport: CreateWidget returned null for class '%s' — no console. Never fatal: the console is not a requirement for any action (CONVENTIONS §2)."),
			*GetNameSafe(ResolvedClass));
		return nullptr;
	}

	// AddToViewport BEFORE the caller drives it: the tree (and therefore
	// InputBox) does not exist until the widget is constructed. It is added
	// CLOSED — NativeConstruct collapses it — so nothing appears on screen and
	// nothing becomes hit-testable until OpenConsole().
	Console->AddToViewport(ZOrder);

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("[AssistantConsole] Created (class '%s', ZOrder %d), closed."), *GetNameSafe(ResolvedClass), ZOrder);

	return Console;
}

TSharedRef<SWidget> USiegeAssistantConsoleWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING AND IT IS THE HEADLINE FINDING OF THE RULING-A
	// REHEARSAL (handoffs/TASK-411-programmer.md §4.1, paid for by the probe
	// this task deletes). UUserWidget::RebuildWidget() reads
	// WidgetTree->RootWidget AS IT STANDS at the moment it is called and
	// returns an SSpacer when it is null. The natural-looking
	//     Super::RebuildWidget(); /* then build */ return Result;
	// yields a silently EMPTY widget that STILL PASSES every property and tree
	// readback, because the UWidget objects all exist and are correctly
	// parented. That is precisely the defect class ruling A(e) exists for.
	// ⛔ Do not reorder these two lines.
	ConstructConsoleTree();
	return Super::RebuildWidget();
}

void USiegeAssistantConsoleWidget::ConstructConsoleTree()
{
	using namespace SiegeAssistantConsole;

	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error,
			TEXT("[AssistantConsole] No WidgetTree at RebuildWidget — the console cannot build a tree. It will render empty; the assistant is unaffected."));
		return;
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		// Two very different situations, and conflating them is a real (if
		// small) lie that the qa/TASK-411 gate raised against the probe:
		//  - our own tree from an earlier rebuild of this instance, or
		//  - RULING A's ESCAPE HATCH: an asset-authored WBP_AssistantConsole
		//    tree, which wins WHOLE. The BindWidgetOptional members were
		//    already resolved by UMG, so there is nothing to construct and
		//    nothing to overwrite — zero C++ change to take the fallback.
		if (!bTreeWasCodeAuthored)
		{
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("[AssistantConsole] An asset-authored tree is present — the code-authored branch is skipped whole (ruling A escape hatch)."));
		}
		return;
	}

	if (RootPanel == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
	}

	if (RootPanel == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error,
			TEXT("[AssistantConsole] Could not construct RootPanel — there will be no text box. The assistant is unaffected; every key still works."));
		return;
	}

	// ⚠️ SECOND FINDING CARRIED FROM THE PROBE. UUserWidget already defaults
	// Visibility to SelfHitTestInvisible, but a code-authored root CONTAINER
	// does not: UVerticalBox defaults to hit-testable Visible and, filling the
	// screen, would swallow EVERY click — including the shipped right-mouse
	// cancel, which qa/TASK-411 measured as the one real casualty of the wrong
	// input posture. Only the box and the two buttons are hit-testable below.
	// ⚠️ This is deliberately the OPPOSITE of USettingsMenuWidget's
	// BackdropBorder (hit-test VISIBLE, because that panel is modal over a
	// menu). Do not "conform" the two: this one sits over live gameplay.
	RootPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootPanel;

	// A filling spacer first, so everything after it is pushed to the BOTTOM of
	// the screen — the console lives out of the way of the battlefield.
	if (USpacer* TopSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ConsoleTopSpacer")))
	{
		TopSpacer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UVerticalBoxSlot* SpacerSlot = RootPanel->AddChildToVerticalBox(TopSpacer))
		{
			SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	// The dark plate. Unpinned and internal: ruling A pins six member names and
	// this is not one of them, so it is owned by the WidgetTree and never
	// competes with a future WBP (which supplies its own tree entirely).
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ConsoleBackdrop"));
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ConsoleColumn"));

	if (Backdrop != nullptr && Column != nullptr)
	{
		Backdrop->SetBrushColor(BackdropColor);
		Backdrop->SetPadding(FMargin(18.f, 14.f, 18.f, 14.f));
		Backdrop->SetHorizontalAlignment(HAlign_Fill);
		Backdrop->SetVerticalAlignment(VAlign_Fill);

		// Click-through: the plate is a legibility device, not a modal shield.
		Backdrop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		Backdrop->SetContent(Column);

		if (UVerticalBoxSlot* BackdropSlot = RootPanel->AddChildToVerticalBox(Backdrop))
		{
			BackdropSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			BackdropSlot->SetPadding(FMargin(64.f, 0.f, 64.f, 48.f));
			BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
			BackdropSlot->SetVerticalAlignment(VAlign_Bottom);
		}
	}

	// From here on Column is the parent for the pinned children. If either of
	// the two above failed to construct, fall back to RootPanel so the console
	// still renders something usable rather than nothing at all.
	//
	// ⚠️ SECOND INSTANCE OF THE C2445 SHAPE ABOVE, and the reason it is worth
	// naming twice: TObjectPtr is the SAME two-way implicit trap as TSubclassOf —
	// `TObjectPtr(const U&)` (ObjectPtr.h:594) and `operator T*()` (:722) are both
	// non-explicit, so `raw ? raw : member` is ambiguous. ⛔ Any conditional in
	// this file mixing a TObjectPtr member with a raw pointer must call `.Get()`
	// on the TObjectPtr arm. (`X != nullptr` is NOT this shape — that resolves
	// against the exact-match nullptr comparison at :676 and is fine.)
	UVerticalBox* ContentParent = (Column != nullptr) ? Column : RootPanel.Get();

	if (TranscriptText == nullptr)
	{
		TranscriptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TranscriptText"));
		if (TranscriptText != nullptr)
		{
			TranscriptText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			TranscriptText->SetAutoWrapText(true);
			TranscriptText->SetFontSize(TranscriptFontSize);
			TranscriptText->SetColorAndOpacity(FSlateColor(TranscriptColor));
			TranscriptText->SetText(FText::GetEmpty());

			// ⛔ NEVER NAME A LOCAL `Slot` IN THIS CLASS — IT IS AN INHERITED
			// MEMBER, NOT A FREE NAME. UWidget declares `TObjectPtr<UPanelSlot>
			// Slot` (Widget.h:264), so USiegeAssistantConsoleWidget inherits it
			// through UUserWidget → UWidget and ANY local of that name inside a
			// member function shadows it (C4458). ⚠️ That is normally a warning;
			// this module builds warnings-as-errors, so it is FATAL. The file's
			// own established idiom is a QUALIFIED slot name — SpacerSlot,
			// BackdropSlot, RowSlot above — and these five sites had drifted off
			// it. Conformed, not invented.
			if (UVerticalBoxSlot* TranscriptSlot = ContentParent->AddChildToVerticalBox(TranscriptText))
			{
				TranscriptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				TranscriptSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
				TranscriptSlot->SetHorizontalAlignment(HAlign_Fill);
			}
		}
	}

	if (StatusText == nullptr)
	{
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		if (StatusText != nullptr)
		{
			StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			StatusText->SetAutoWrapText(true);
			StatusText->SetFontSize(StatusFontSize);
			StatusText->SetColorAndOpacity(FSlateColor(StatusColor));
			StatusText->SetText(FText::FromString(FString(IdleStatusText)));

			if (UVerticalBoxSlot* StatusSlot = ContentParent->AddChildToVerticalBox(StatusText))
			{
				StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
				StatusSlot->SetHorizontalAlignment(HAlign_Fill);
			}
		}
	}

	if (InputBox == nullptr)
	{
		InputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputBox"));
		if (InputBox != nullptr)
		{
			// Hit-testable ON PURPOSE — this one widget must take the click and
			// the caret. It is a single line at the bottom of the screen.
			InputBox->SetVisibility(ESlateVisibility::Visible);
			InputBox->SetHintText(FText::FromString(FString(InputHintText)));

			if (UVerticalBoxSlot* InputSlot = ContentParent->AddChildToVerticalBox(InputBox))
			{
				InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				InputSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				InputSlot->SetHorizontalAlignment(HAlign_Fill);
			}
		}
	}

	// The confirm row. Both buttons start hidden: they exist only while the
	// FSM has actually raised a prompt (ShowConfirmPrompt).
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ConsoleButtonRow"));
	if (ButtonRow != nullptr)
	{
		ButtonRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UVerticalBoxSlot* RowSlot = ContentParent->AddChildToVerticalBox(ButtonRow))
		{
			RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			RowSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}
	else
	{
		// Confirm/Cancel are the confirm step's whole player surface, so losing
		// the row is worth an error line rather than a quietly button-less
		// console. Everything below null-guards, so this degrades; it does not
		// crash, and it does not touch any key.
		UE_LOG(LogSiegeAssistant, Error,
			TEXT("[AssistantConsole] Could not construct the button row — Accept/Cancel will be absent. The keyboard path is unaffected."));
	}

	if (ConfirmButton == nullptr && ButtonRow != nullptr)
	{
		ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
		if (ConfirmButton != nullptr)
		{
			if (UTextBlock* AcceptLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmLabelText")))
			{
				AcceptLabel->SetText(FText::FromString(FString(AcceptLabelText)));
				AcceptLabel->SetFontSize(ButtonLabelSize);
				ConfirmButton->SetContent(AcceptLabel);
			}

			if (UHorizontalBoxSlot* ConfirmSlot = ButtonRow->AddChildToHorizontalBox(ConfirmButton))
			{
				ConfirmSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
				ConfirmSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (CancelButton == nullptr && ButtonRow != nullptr)
	{
		CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
		if (CancelButton != nullptr)
		{
			if (UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelLabelText")))
			{
				CancelLabel->SetText(FText::FromString(FString(CancelLabelText)));
				CancelLabel->SetFontSize(ButtonLabelSize);
				CancelButton->SetContent(CancelLabel);
			}

			if (UHorizontalBoxSlot* CancelSlot = ButtonRow->AddChildToHorizontalBox(CancelButton))
			{
				CancelSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	bTreeWasCodeAuthored = true;

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("[AssistantConsole] Code-authored tree built (ruling A). Children: RootPanel=%d TranscriptText=%d StatusText=%d InputBox=%d ConfirmButton=%d CancelButton=%d. ⚠️ Nothing about how this LOOKS is verified here — ruling A(e) closes on rendered pixels."),
		RootPanel != nullptr ? 1 : 0, TranscriptText != nullptr ? 1 : 0, StatusText != nullptr ? 1 : 0,
		InputBox != nullptr ? 1 : 0, ConfirmButton != nullptr ? 1 : 0, CancelButton != nullptr ? 1 : 0);
}

void USiegeAssistantConsoleWidget::WireChildWidgets()
{
	// Auto-wire the OPTIONAL named children. A WBP that uses different names
	// wires its own controls to the BlueprintCallable wrappers instead — both
	// routes are supported, exactly as USessionMenuWidget supports both.
	// AddUniqueDynamic keeps repeated construct cycles single-bound.
	if (InputBox)
	{
		InputBox->OnTextCommitted.AddUniqueDynamic(this, &USiegeAssistantConsoleWidget::HandleTextCommitted);
	}
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &USiegeAssistantConsoleWidget::HandleConfirmClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &USiegeAssistantConsoleWidget::HandleCancelClicked);
	}

	ApplyInputBoxContract();
}

void USiegeAssistantConsoleWidget::ApplyInputBoxContract()
{
	if (InputBox == nullptr)
	{
		return;
	}

	// ⚠️ THESE FOUR ARE CORRECTNESS, NOT STYLING, AND THEY ARE APPLIED TO AN
	// ASSET-AUTHORED BOX TOO — a designer cannot be expected to know that two
	// of them decide whether keys leak into the game.
	//
	//  1. Editable, obviously — SEditableText::SupportsKeyboardFocus() is what
	//     makes the box focusable at all, and focus is the whole mechanism.
	InputBox->SetIsReadOnly(false);

	//  2. ⛔ RevertTextOnEscape STAYS FALSE. HandleEscape() returns true only
	//     with search text, a selection, or RevertTextOnEscape && changed text
	//     (SlateEditableTextLayout.cpp:1448). False therefore leaves Escape
	//     UNHANDLED, so it bubbles to the viewport and the shipped
	//     WasInputKeyJustPressed(EKeys::Escape) cancel routes in
	//     ASiegePlayerController STILL FIRE while the console is open. Setting
	//     this true would silently disarm a shipped key — exactly the
	//     disturbance CONVENTIONS §2 forbids.
	//     ⇒ CONSEQUENCE, STATED RATHER THAN HIDDEN: Escape does not close this
	//     console. Cancel and the open key do.
	InputBox->SetRevertTextOnEscape(false);

	//  3. ⛔ ClearKeyboardFocusOnCommit IS TURNED OFF, AND THIS IS AN EXPLICIT
	//     DECISION THE PROBE'S SIDE FINDINGS DEMANDED (TASK-411 §3.1). The
	//     engine default is TRUE, so committing drops keyboard focus: after the
	//     first sentence the box would look focused and silently not be — read
	//     by any player as a bug. Worse, an unfocused box no longer absorbs
	//     Enter (SlateEditableTextLayout.cpp:1092 is what makes Enter safe to
	//     also be the open key), so the NEXT Enter would leak into Enhanced
	//     Input and re-toggle the console. Holding focus for the console's
	//     whole open lifetime closes both.
	InputBox->SetClearKeyboardFocusOnCommit(false);

	//  4. Do not select-all on focus: the box is cleared after every submit, so
	//     select-all only risks a stray keystroke wiping a half-typed order.
	InputBox->SetSelectAllTextWhenFocused(false);
}

void USiegeAssistantConsoleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ⚠️ A THIRD TRAP OF THE CODE-AUTHORED PATTERN, ALONGSIDE THE TWO THE PROBE
	// FOUND — AND THIS ONE BITES IN THE OPPOSITE DIRECTION FROM THE USUAL IDIOM.
	// The shipped idiom (USessionMenuWidget) wires its children in
	// NativeOnInitialized, and for a WBP that is correct: Initialize() resolves
	// the BindWidget members and THEN calls NativeOnInitialized
	// (UserWidget.cpp:168/175). But a CODE-AUTHORED tree is built in
	// RebuildWidget(), which does not run until TakeWidget()
	// (Widget.cpp:993) — long AFTER Initialize(). So in NativeOnInitialized
	// every child here is still NULL and every binding would silently no-op:
	// no compile error, no log, a console whose Enter and buttons do nothing.
	// NativeConstruct runs after the tree is built (UserWidget.cpp:1234) and is
	// correct for BOTH routes, which is why the wiring lives here.
	WireChildWidgets();

	// The console is born CLOSED. Nothing is on screen and nothing is
	// hit-testable until something asks for it.
	bConsoleOpen = false;
	bConfirmPromptVisible = false;
	ApplyConsoleVisualState();
}

void USiegeAssistantConsoleWidget::NativeDestruct()
{
	// ⚠️ Do NOT touch Slate focus during world teardown. NativeDestruct is
	// reachable from GC/teardown, and the qa/TASK-411 gate raised exactly this
	// against the probe: its posture restore ran unconditionally while the
	// focus restore beside it was correctly gated.
	const UWorld* World = GetWorld();
	const bool bWorldIsLive = (World != nullptr) && !World->bIsTearingDown;

	if (bConsoleOpen && bWorldIsLive)
	{
		ReleaseKeyboardFocusToGame();
	}

	bConsoleOpen = false;
	bConfirmPromptVisible = false;

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

void USiegeAssistantConsoleWidget::OpenConsole()
{
	if (!bConsoleEnabled)
	{
		// A faulted assistant leaves the console disabled and nothing else
		// (CONVENTIONS §2 fault posture). Refusing to open is quieter and
		// truer than opening a box that will never answer.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("[AssistantConsole] Open refused: the console is disabled. No key, card or command is affected."));
		return;
	}

	if (bConsoleOpen)
	{
		// Already open: re-assert focus rather than no-op, so a second press of
		// the open key recovers focus if something else took it.
		// ⚠️ The suppression guard below deliberately sits AFTER this: if the
		// console is already open there is no open to suppress, and a live stamp
		// cannot exist here anyway (an open consumes it).
		FocusInputBox();
		return;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// RE-OPEN SUPPRESSION — CONVENTIONS AS-§6 RULING A-2, THE ORDERING CLAUSE
	// ─────────────────────────────────────────────────────────────────────────
	// ⚠️ THE HAZARD THIS COVERS IS UNMEASURED, WHICH IS WHY IT IS BUILT TO BE
	// CORRECT IN BOTH WORLDS RATHER THAN TO BET ON ONE.
	//
	// Enter now has three jobs: OPEN the console (IA_AssistantConsole, bound in
	// ASiegePlayerController), SUBMIT a typed line, and CLOSE an empty box
	// (route 4, in HandleTextCommitted).
	//
	//  WORLD A — Slate consumes the key while the box holds keyboard focus (the
	//    standing reasoning, and what Jonathan's own "I cannot close it" report
	//    corroborates: the controller toggle at OnAssistantConsolePressed is
	//    already CLOSE-FIRST and UN-GATED, so if Enhanced Input were receiving
	//    Enter at all, the console would ALREADY have been closing on him).
	//    Enhanced Input never sees the press, nothing re-opens, and this guard
	//    never fires. ⚠️ CORROBORATION, NOT A MEASUREMENT.
	//
	//  WORLD B — Enhanced Input receives the same press too. Route 4 closes and
	//    broadcasts, the controller clears the posture, and OnAssistantConsole-
	//    Pressed then finds the console CLOSED and takes its OPEN branch — the
	//    SAME physical press closing and immediately re-opening, which reads to
	//    the player as "the close key does nothing". This guard is what makes
	//    that press a clean close instead.
	//
	// ✅ REFUSING HERE IS SAFE AND NEEDS NO CONTROLLER EDIT, AND I VERIFIED THAT
	// AT THE ARTIFACT RATHER THAN ASSERTING IT. ASiegePlayerController::
	// OnAssistantConsolePressed re-reads the widget after calling OpenConsole()
	// and rolls the posture back when the widget refused, quoted verbatim from
	// SiegePlayerController.cpp (symbol OnAssistantConsolePressed, at :4365):
	//
	//     if (!Console->IsConsoleOpen())
	//     {
	//         SetAssistantConsoleOpen(false);
	//
	// ⇒ A suppressed re-open cannot strand the cursor in GameAndUI. That rollback
	// is the property this guard depends on; if it is ever removed, this guard
	// becomes a cursor soft-lock and the two must be changed together.
	//
	// ⛔ THE WINDOW CANNOT STICK, AND IT IS BOUNDED TWO INDEPENDENT WAYS:
	//   (i)  TIME — a monotonic real-time clock that cannot be paused or dilated
	//        (see LastRoute4CloseRealTimeSeconds' comment for why not the world's).
	//   (ii) ONE SHOT — the stamp is CONSUMED by the first open attempt after the
	//        close, whatever the verdict. At most ONE open can ever be suppressed
	//        per close, so even a clock that misbehaved could not produce a
	//        console that refuses to open twice.
	// Either bound alone ends the window; both must hold to refuse.
	if (LastRoute4CloseRealTimeSeconds >= 0.0)
	{
		const double ElapsedSinceClose = FPlatformTime::Seconds() - LastRoute4CloseRealTimeSeconds;
		const double SuppressionWindow = static_cast<double>(
			FMath::Clamp(ReopenSuppressionSeconds, 0.0f, SiegeAssistantConsole::MaxReopenSuppressionSeconds));

		// ⛔ DISARM FIRST, DECIDE SECOND. Consuming the stamp before the branch is
		// what makes bound (ii) structural instead of a promise — every early
		// return below this line leaves the window closed.
		LastRoute4CloseRealTimeSeconds = -1.0;

		if (ElapsedSinceClose < SuppressionWindow)
		{
			// ⚠️ ONE LINE, AND ITS PRESENCE IS THE MEASUREMENT NOBODY HAS TAKEN:
			// this can only be reached in WORLD B. If this line never appears in a
			// log, Enter is not reaching Enhanced Input while the box has focus.
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("[AssistantConsole] Re-open suppressed %.0f ms after an empty-Enter close (AS-§6 A-2 route 4). The same keypress reached BOTH Slate and Enhanced Input; the console stays closed, as the player asked. No key, card or command is affected."),
				ElapsedSinceClose * 1000.0);
			return;
		}
	}

	bConsoleOpen = true;

	if (InputBox)
	{
		InputBox->SetText(FText::GetEmpty());
	}

	ApplyConsoleVisualState();
	FocusInputBox();

	UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Opened."));

	OnConsoleOpenStateChanged(true);
	OnConsoleOpenChanged.Broadcast(true);
}

void USiegeAssistantConsoleWidget::CloseConsole()
{
	if (!bConsoleOpen)
	{
		return;
	}

	bConsoleOpen = false;

	// The prompt comes down with the window, but ⛔ NO cancellation is
	// broadcast: "the player closed the window" and "the player discarded the
	// order" are different statements, and only the FSM may turn one into the
	// other. It hears OnConsoleOpenChanged(false) and decides.
	const bool bHadPrompt = bConfirmPromptVisible;
	bConfirmPromptVisible = false;

	ApplyConsoleVisualState();
	ReleaseKeyboardFocusToGame();

	UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Closed%s."),
		bHadPrompt ? TEXT(" with a confirm prompt up — the FSM still holds it; nothing was accepted and nothing was cancelled") : TEXT(""));

	// Keep the presentation surfaces honest: a BIE consumer that was told the
	// prompt was up must be told it came down, or it renders a phantom.
	if (bHadPrompt)
	{
		OnConfirmPromptChanged(false, FString());
	}

	OnConsoleOpenStateChanged(false);
	OnConsoleOpenChanged.Broadcast(false);
}

void USiegeAssistantConsoleWidget::ToggleConsole()
{
	if (bConsoleOpen)
	{
		CloseConsole();
	}
	else
	{
		OpenConsole();
	}
}

// ---------------------------------------------------------------------------
// Player entries
// ---------------------------------------------------------------------------

void USiegeAssistantConsoleWidget::SubmitPressed(const FString& UtteranceText)
{
	if (!bConsoleEnabled)
	{
		UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Submit ignored: the console is disabled."));
		return;
	}

	FString Trimmed = UtteranceText;
	Trimmed.TrimStartAndEndInline();

	if (Trimmed.IsEmpty())
	{
		// Refused locally and deliberately: a blank prompt would spend a model
		// call (queue depth is 1) to learn nothing. No broadcast, no state
		// change, no transcript line.
		if (InputBox)
		{
			InputBox->SetText(FText::GetEmpty());
		}
		FocusInputBox();
		return;
	}

	// Echo what the PLAYER typed. This is the one transcript line not authored
	// by the template table, and it is the player's own words — never a model
	// string (§3 is about model output reaching the player, and nothing here
	// came from a model).
	ShowTranscriptLine(FString::Printf(TEXT("> %s"), *Trimmed));

	if (InputBox)
	{
		InputBox->SetText(FText::GetEmpty());
	}

	// Focus is retained across a commit (ApplyInputBoxContract item 3), but
	// re-assert it: a WBP-authored box may carry the engine default.
	FocusInputBox();

	UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Submitted %d chars."), Trimmed.Len());

	OnConsoleSubmitted.Broadcast(Trimmed);
}

void USiegeAssistantConsoleWidget::ConfirmPressed()
{
	if (!bConfirmPromptVisible)
	{
		// ⛔ The gate that matters. Accept must never be able to execute an
		// order the player is not being shown — the confirm step exists
		// precisely because a valid-shaped command can be the wrong command.
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("[AssistantConsole] Accept ignored: no confirm prompt is up. Nothing was executed."));
		return;
	}

	bConfirmPromptVisible = false;
	ApplyConsoleVisualState();

	UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Accept pressed."));

	OnConfirmPromptChanged(false, FString());
	OnConsoleConfirmed.Broadcast();
}

void USiegeAssistantConsoleWidget::CancelPressed()
{
	if (!bConfirmPromptVisible)
	{
		// No prompt up: Cancel reads as "dismiss the console".
		UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Cancel with no prompt up — closing the console."));
		CloseConsole();
		return;
	}

	bConfirmPromptVisible = false;
	ApplyConsoleVisualState();

	UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Cancel pressed — the FSM discards; nothing is partially executed."));

	OnConfirmPromptChanged(false, FString());
	OnConsoleCancelled.Broadcast();
}

void USiegeAssistantConsoleWidget::HandleTextCommitted(const FText& CommittedText, ETextCommit::Type CommitMethod)
{
	// ⛔ ENTER ONLY. OnUserMovedFocus and OnCleared also arrive here, and
	// treating either as an order would let a click elsewhere on the screen
	// submit a half-typed sentence.
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// CLOSE ROUTE 4 — ENTER ON AN EMPTY BOX CLOSES THE CONSOLE
	// CONVENTIONS AS-§6 RULING A-2. Jonathan's directive, 2026-08-03: "if you
	// press enter without anything typed in the box then it will close".
	// ─────────────────────────────────────────────────────────────────────────
	// ⛔ IT LIVES HERE AND NOT IN SubmitPressed, AND THE DIFFERENCE IS NOT STYLE.
	// The filter above is what makes this branch mean "the player pressed Enter"
	// — OnUserMovedFocus and OnCleared arrive at the same delegate and are
	// already excluded. SubmitPressed is BlueprintCallable and reachable from
	// callers that are not the key, and ITS empty-string branch is a SEPARATE,
	// RATIFIED protection (no model call on a blank prompt; queue depth is 1).
	// Routing the close through it would make "some caller passed an empty
	// string" mean "close the window", which nobody asked for.
	//
	// ⛔ NOT GATED ON bConsoleEnabled, DELIBERATELY. A close that can be refused
	// is a close that can strand the cursor — the same reason RULING A-2 pins the
	// controller toggle's close half un-gated.
	FString CommittedTrimmed = CommittedText.ToString();
	CommittedTrimmed.TrimStartAndEndInline();

	if (CommittedTrimmed.IsEmpty())
	{
		if (!bConsoleOpen)
		{
			// Nothing to close. Reachable only if something closed the console
			// between the keypress and this callback — the WORLD B ordering where
			// Enhanced Input's toggle ran FIRST and already closed it. Not an
			// error, and deliberately NOT stamped: no re-open is racing this
			// press, so arming the window would only suppress a later, wanted one.
			return;
		}

		// ⛔ ARM BEFORE CLOSING, NOT AFTER. CloseConsole() ends in
		// OnConsoleOpenChanged.Broadcast(false); a consumer that re-opened
		// synchronously from inside that broadcast would slip past a window armed
		// afterwards. Costs nothing and removes the re-entrant case entirely.
		LastRoute4CloseRealTimeSeconds = FPlatformTime::Seconds();

		UE_LOG(LogSiegeAssistant, Log,
			TEXT("[AssistantConsole] Enter on an empty box — closing the console (AS-§6 A-2 route 4). Nothing was submitted, and nothing was cancelled: a close is not a cancel."));

		// ⛔ CloseConsole() IS REUSED VERBATIM AND NO NEW BROADCAST IS ADDED. It
		// already handles the confirm-prompt case on ratified terms: the prompt
		// comes down, OnConfirmPromptChanged(false, …) fires so no BIE consumer
		// renders a phantom, NO cancellation is broadcast, and the FSM still holds
		// the order and decides for itself on OnConsoleOpenChanged(false).
		// Empty-Enter closes even with a prompt up — that is the point of the
		// feature, and the FSM semantics are not this widget's to change.
		CloseConsole();
		return;
	}

	// ⛔ THE NON-EMPTY PATH IS BYTE-FOR-BYTE WHAT IT WAS. The trimmed copy above
	// exists ONLY to answer "is the box empty"; the RAW text is still what goes to
	// SubmitPressed, which does its own trimming exactly as before. Submit did not
	// become closable and close did not become a submit.
	SubmitPressed(CommittedText.ToString());
}

void USiegeAssistantConsoleWidget::HandleConfirmClicked()
{
	ConfirmPressed();
}

void USiegeAssistantConsoleWidget::HandleCancelClicked()
{
	CancelPressed();
}

// ---------------------------------------------------------------------------
// Inbound pushes from the FSM
// ---------------------------------------------------------------------------

void USiegeAssistantConsoleWidget::SetAssistantState(uint8 NewState, const FString& StateLabel)
{
	AssistantState = NewState;

	// ⛔ The number is stored and shown. It is NOT switched on — see the class
	// comment §3: interpreting a uint8 whose enum lives in another file, owned
	// by another task, would couple this widget to that enum's ORDER with
	// nothing to catch a reordering.
	if (StatusText)
	{
		StatusText->SetText(StateLabel.IsEmpty()
			? FText::FromString(FString(SiegeAssistantConsole::IdleStatusText))
			: FText::FromString(StateLabel));
	}

	OnAssistantStateChanged(NewState, StateLabel);
}

void USiegeAssistantConsoleWidget::ShowTranscriptLine(const FString& Line)
{
	if (Line.IsEmpty())
	{
		return;
	}

	TranscriptLines.Add(Line);

	const int32 KeepCount = FMath::Max(1, MaxTranscriptLines);
	if (TranscriptLines.Num() > KeepCount)
	{
		TranscriptLines.RemoveAt(0, TranscriptLines.Num() - KeepCount, EAllowShrinking::No);
	}

	RefreshTranscriptText();

	OnTranscriptLineShown(Line);
}

void USiegeAssistantConsoleWidget::ClearTranscript()
{
	TranscriptLines.Reset();
	RefreshTranscriptText();
}

void USiegeAssistantConsoleWidget::RefreshTranscriptText()
{
	if (TranscriptText == nullptr)
	{
		return;
	}

	TranscriptText->SetText(FText::FromString(FString::Join(TranscriptLines, TEXT("\n"))));
}

void USiegeAssistantConsoleWidget::ShowConfirmPrompt(const FString& SummaryLine)
{
	// The summary is GAME-AUTHORED, from the component's reason-code template
	// table. This widget renders it and never composes one.
	if (!SummaryLine.IsEmpty())
	{
		ShowTranscriptLine(SummaryLine);
	}

	bConfirmPromptVisible = true;
	ApplyConsoleVisualState();

	// ⚠️ NAMED HAZARD, DELIBERATELY NOT DECIDED HERE. A prompt raised while the
	// console is closed is invisible: the player is never asked, and the order
	// sits pending. The deferred-intent path is exactly this case — it fires up
	// to 120 s after the sentence was typed, on a board the player has not
	// looked at since, and re-enters AwaitConfirm. Auto-opening would be an FSM
	// policy decision, and policy is not this widget's to make (it would also
	// seize keyboard focus at a moment the player did not ask for it). So it is
	// logged loudly instead of swallowed, and flagged to the FSM's task.
	if (!bConsoleOpen)
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("[AssistantConsole] A confirm prompt was raised while the console is CLOSED — the player cannot see or answer it. The caller must open the console (OpenConsole) when it needs an answer."));
	}

	OnConfirmPromptChanged(true, SummaryLine);
}

void USiegeAssistantConsoleWidget::HideConfirmPrompt()
{
	if (!bConfirmPromptVisible)
	{
		return;
	}

	bConfirmPromptVisible = false;
	ApplyConsoleVisualState();

	OnConfirmPromptChanged(false, FString());
}

void USiegeAssistantConsoleWidget::SetConsoleEnabled(bool bEnabled, const FString& DisabledReason)
{
	if (bConsoleEnabled == bEnabled)
	{
		// Never broadcast on a no-op write (the delegate law + the qa/TASK-005
		// major-2 lesson: a surface that fires on non-changes trains its
		// consumers to ignore it).
		return;
	}

	bConsoleEnabled = bEnabled;

	if (!bConsoleEnabled)
	{
		// Close FIRST and let CloseConsole announce the prompt coming down —
		// clearing the flag before the call would swallow that announcement and
		// leave a BIE consumer rendering a prompt nobody can answer.
		if (bConsoleOpen)
		{
			CloseConsole();
		}

		// A prompt can also be latched while the console is closed (see the
		// named hazard in ShowConfirmPrompt), so it is cleared explicitly too.
		if (bConfirmPromptVisible)
		{
			bConfirmPromptVisible = false;
			OnConfirmPromptChanged(false, FString());
		}

		if (StatusText && !DisabledReason.IsEmpty())
		{
			StatusText->SetText(FText::FromString(DisabledReason));
		}

		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("[AssistantConsole] Disabled: %s. ⚠️ This disables the CONSOLE and nothing else — every keyboard command, card and order path is untouched (CONVENTIONS §2)."),
			DisabledReason.IsEmpty() ? TEXT("(no reason given)") : *DisabledReason);
	}
	else
	{
		UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Re-enabled."));
	}

	ApplyConsoleVisualState();

	OnConsoleEnabledChanged(bConsoleEnabled, DisabledReason);
}

// ---------------------------------------------------------------------------
// Presentation + focus
// ---------------------------------------------------------------------------

void USiegeAssistantConsoleWidget::ApplyConsoleVisualState()
{
	// The widget itself carries the open/closed decision, so a closed console
	// costs nothing to hit-test and nothing to paint.
	SetVisibility(bConsoleOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (InputBox)
	{
		InputBox->SetIsEnabled(bConsoleEnabled);
	}

	// The buttons exist only while a prompt is up. Collapsed rather than
	// hidden, so the row takes no space when there is nothing to confirm.
	const ESlateVisibility ButtonVisibility =
		bConfirmPromptVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(ButtonVisibility);
		ConfirmButton->SetIsEnabled(bConsoleEnabled);
	}
	if (CancelButton)
	{
		CancelButton->SetVisibility(ButtonVisibility);
		CancelButton->SetIsEnabled(bConsoleEnabled);
	}
}

void USiegeAssistantConsoleWidget::FocusInputBox()
{
	if (InputBox == nullptr)
	{
		if (!bWarnedNoInputBox)
		{
			bWarnedNoInputBox = true;
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("[AssistantConsole] No InputBox to focus — the console cannot take typing. (Logged once.) The assistant is otherwise unaffected."));
		}
		return;
	}

	// ⚠️ THE MEASURED POSTURE, INHERITED RATHER THAN RE-DERIVED (spike bar #6,
	// TASK-411/413): keyboard focus on this box does NOT starve Enhanced Input
	// of WASD, so the console ships over FInputModeGameAndUI and the camera
	// stays live while the player types.
	// ⛔ The INPUT MODE itself is not set here. ASiegePlayerController owns it
	// (CONVENTIONS "Input-mode ownership (level-travel law)"), and a second
	// SetInputMode caller is how a level ends up stranded in the wrong posture.
	InputBox->SetKeyboardFocus();
}

void USiegeAssistantConsoleWidget::ReleaseKeyboardFocusToGame()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	// Without this, focus can stay on a collapsed box and typing goes nowhere.
	// The game viewport widget is the real shipping keyboard-focus target when
	// no UI is up — and it is NOT implied by the input mode:
	// FInputModeDataBase::SetFocusAndLocking focuses only when WidgetToFocus is
	// valid, so GameAndUI with no widget focuses nothing (PlayerController.cpp
	// :6313; FInputModeGameOnly is the one that focuses the viewport, :6446).
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
