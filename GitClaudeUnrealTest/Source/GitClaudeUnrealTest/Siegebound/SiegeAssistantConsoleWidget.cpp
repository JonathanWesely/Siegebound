// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeAssistantConsoleWidget.h"

#include "SiegeAssistantCommand.h"
// TASK-519: the ACCEPT key is resolved positionally, never hard-coded —
// USiegeKeyboardLayoutSubsystem::GetPositionalKey / RefreshKeyboardLayout.
#include "SiegeKeyboardLayoutSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
// ⛔ Components/HorizontalBox.h + HorizontalBoxSlot.h WERE HERE AND ARE GONE WITH
// THE CONFIRM BUTTON ROW (TASK-519). Nothing in this file builds a horizontal box
// any more; a WBP that wants one builds its own tree.
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
// ⚠️ REQUIRED, NOT INHERITED (the HeroCharacter.cpp:11 precedent, same reason):
// GetGameInstance()->GetSubsystem<>() needs the COMPLETE UGameInstance type, and
// UserWidget.h only forward-declares it.
#include "Engine/GameInstance.h"
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

	/** Shown on open when the FSM has not pushed a state label yet. */
	static const TCHAR* IdleStatusText  = TEXT("Ready");

	/**
	 *  ⭐ THE CONFIRM STEP'S ENTIRE PLAYER SURFACE, NOW THAT BOTH BUTTONS ARE GONE
	 *  (TASK-519, Jonathan's ruling 3). Shown on the status line for exactly as
	 *  long as bConfirmPromptVisible is true.
	 *
	 *  ⛔ IT SAYS THE LETTER `Z` ON EVERY KEYBOARD LAYOUT, AND THAT IS NOT AN
	 *  INCONSISTENCY WITH THE POSITIONAL LOOKUP — IT IS THE POINT OF IT
	 *  (KBD-§8's last bullet, KBD-§0 ruling 1). The player is on QWERTY HARDWARE
	 *  with a Dvorak SOFTWARE layout: their keycap is physically printed `Z`, so
	 *  rendering GetAcceptKey()'s "Semicolon" here would be the bug, not the fix.
	 *  ⇒ THE LOOKUP IS FOR THE COMPARISON; THIS LITERAL IS FOR THE HUMAN.
	 *  ⛔ Never build this string from GetAcceptKey().
	 *
	 *  It names BOTH halves of the ruling, because the discard half is otherwise
	 *  undiscoverable: there is no longer a control anywhere on screen that says
	 *  "cancel", and an order that can only be dismissed by a gesture nobody
	 *  documented reads as "the assistant ate my order".
	 *
	 *  ⚠️ STATIC CHROME, so §3 is untouched: it names no order, unit, place or
	 *  outcome. Every sentence with game meaning still arrives from the
	 *  component's reason-code template table.
	 */
	static const TCHAR* ConfirmHintText = TEXT("Press Z to accept, or close this box to discard");

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
	// ⛔ ButtonLabelSize WAS HERE AND IS GONE WITH THE BUTTONS IT SIZED
	// (TASK-519). A leftover unused constant is not free in this module: it builds
	// warnings-as-errors.
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

	// ─────────────────────────────────────────────────────────────────────────
	// ⛔⛔ THE CONFIRM BUTTON ROW STOOD HERE AND IS GONE — Accept AND Cancel.
	// TASK-519 / CONVENTIONS AS-§6 RULING A (pinned child list, amended
	// 2026-08-04) + AS-§20.5. Jonathan's ruling 3, verbatim: "there is no cancel
	// button (they just simply close the chat box), and instead of an accept
	// button they press 'z'".
	// ─────────────────────────────────────────────────────────────────────────
	//  · CancelButton is STRUCK FROM THE PIN — the member, its HandleCancelClicked
	//    thunk and its CancelLabelText are all deleted (see the retirement comment
	//    in the header where the member used to be declared).
	//  · ConfirmButton is STILL PINNED, STILL DECLARED BindWidgetOptional, STILL
	//    BOUND AND STILL SHOWN/HIDDEN below — the code-authored tree simply stops
	//    CONSTRUCTING one. ⭐ That is ruling A(b)'s escape hatch behaving exactly as
	//    it was written ("construct a child only if that member is still null"): a
	//    future WBP_AssistantConsole that supplies a ConfirmButton wins with ZERO
	//    C++ change here, and the v1 tree carries no dead control.
	//  · The confirm step's player surface is now ONE LINE OF TEXT on StatusText —
	//    SiegeAssistantConsole::ConfirmHintText, raised and lowered by
	//    RefreshStatusLine() on exactly the bConfirmPromptVisible lifecycle the
	//    buttons had. The ACCEPT input is NativeOnPreviewKeyDown.
	//  ⚠️ CONSEQUENCE WORTH KNOWING WHILE READING THE REST OF THIS FUNCTION:
	//    InputBox is now the ONLY hit-testable widget the code-authored tree
	//    builds, which strengthens rather than weakens the click-through property
	//    the SelfHitTestInvisible comments above exist to protect.

	bTreeWasCodeAuthored = true;

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("[AssistantConsole] Code-authored tree built (ruling A). Children: RootPanel=%d TranscriptText=%d StatusText=%d InputBox=%d. No buttons are constructed (TASK-519: `Z` accepts, closing discards); ConfirmButton bound from an asset tree=%d. ⚠️ Nothing about how this LOOKS is verified here — ruling A(e) closes on rendered pixels."),
		RootPanel != nullptr ? 1 : 0, TranscriptText != nullptr ? 1 : 0, StatusText != nullptr ? 1 : 0,
		InputBox != nullptr ? 1 : 0, ConfirmButton != nullptr ? 1 : 0);
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
	// ⚠️ THIS BINDING IS NEVER TAKEN BY THE v1 TREE AND IT STAYS ANYWAY: the
	// code-authored path no longer constructs a ConfirmButton, so this is null
	// unless a WBP_AssistantConsole supplied one — which is exactly the case
	// ruling A(b) promises will work with zero C++ change (TASK-519).
	// ⛔ The CancelButton bind that sat below it is DELETED with the button.
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &USiegeAssistantConsoleWidget::HandleConfirmClicked);
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

	// ─────────────────────────────────────────────────────────────────────────
	// ONE KEYBOARD-LAYOUT PROBE PER OPEN — CONVENTIONS KBD-§8's RULED CONTRACT:
	// ⚖️ THE CALLER REFRESHES, THE ACCESSOR READS.
	// ─────────────────────────────────────────────────────────────────────────
	// GetPositionalKey() is `const` and deliberately does NOT re-probe, so the
	// accept key is only as current as the last refresh. This is the refresh the
	// pin names by this exact function.
	//
	// ⚠️ IT SITS AFTER THE SUPPRESSION GUARD ON PURPOSE: a REFUSED open is not an
	// open, and probing for one would be work done for a console that never
	// appeared.
	//
	// ✅ COST: one GetKeyboardLayout(0) + a 26-row scan-code walk, once, at the
	// moment a human pressed a key to open a text box. That is free by
	// GetPositionalContext's own standard — mechanism 1 re-probes on EVERY call.
	//
	// ⛔ AND IT IS WHY THIS WIDGET BINDS NOTHING: OnKeyboardLayoutChanged is
	// deliberately NOT subscribed (KBD-§8 forbids it for a widget) — that would be
	// new lifetime state to unbind wrongly, for a value re-read at every open and
	// re-resolved live at every keypress.
	if (USiegeKeyboardLayoutSubsystem* LayoutSubsystem = ResolveKeyboardLayoutSubsystem())
	{
		LayoutSubsystem->RefreshKeyboardLayout();
	}

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

// ⛔ HandleCancelClicked() STOOD HERE AND IS DELETED WITH ITS BUTTON (TASK-519 /
// AS-§6 ruling A, amended). ⚠️ CancelPressed() ITSELF IS UNTOUCHED, above — it is
// shipped BlueprintCallable public API and enumerated close route 2, and it is
// now deliberately without a caller. Read its header comment before "cleaning up"
// either of them.

// ---------------------------------------------------------------------------
// The accept key
// ---------------------------------------------------------------------------

FReply USiegeAssistantConsoleWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// ─────────────────────────────────────────────────────────────────────────
	// ⭐ `Z` ACCEPTS. CONVENTIONS AS-§20.5 + AS-§6 RULING A-2, both amended
	// 2026-08-04 by Jonathan's ruling 3. The mechanism is PINNED there and the
	// engine-source verification for it is quoted in this function's header
	// comment — read that before changing a line of this.
	// ─────────────────────────────────────────────────────────────────────────
	//
	// ⛔⛔ THE GATE IS THE NARROWEST IT CAN POSSIBLY BE, AND THE NARROWNESS IS THE
	// WHOLE REASON AS-§2 SURVIVES THIS FEATURE. `Z` is currently bound to nothing
	// (the shipped letters are W/A/S/D/Q/T/R/E/F/C), so no shipped key changes
	// behaviour — but ONLY under a grab this narrow. ⚠️ TYPING IS NOT BLOCKED
	// DURING AwaitConfirm: the box stays editable and a submission is refused at
	// the FSM, so a `Z` grab that ignored bConfirmPromptVisible would stop the
	// player typing the letter `z` for the console's whole open lifetime.
	//
	// bConsoleEnabled is in the gate because the Accept BUTTON carried
	// SetIsEnabled(bConsoleEnabled) (see ApplyConsoleVisualState) — the key
	// inherits the button's disabled semantics rather than inventing new ones. It
	// is belt-and-braces: SetConsoleEnabled(false) already lowers the prompt.
	const bool bAcceptIsLive = bConsoleOpen && bConsoleEnabled && bConfirmPromptVisible;

	// ⛔ MODIFIED PRESSES ARE NOT THE ACCEPT KEY, AND THIS IS A MEASURED GUARD
	// RATHER THAN A REFLEX: Ctrl+Z / Ctrl+Shift+Z are the text box's own UNDO and
	// REDO — FSlateEditableTextLayout::HandleKeyDown:1168-1176 reads
	//     (Key == EKeys::Z && InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown())
	// for redo, beside the undo command binding. Consuming those would BOTH kill
	// undo in the box AND execute an order the player never asked to execute,
	// which is the single worst outcome this confirm step exists to prevent.
	// ✅ Shift is deliberately ALLOWED — Shift+Z is still "the Z key" to a human,
	// and Ctrl+Shift+Z is already excluded by the Ctrl term.
	const bool bIsUnmodified =
		!InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown() && !InKeyEvent.IsCommandDown();

	// ⚠️ ORDER MATTERS FOR COST, NOT CORRECTNESS: && short-circuits, so
	// GetAcceptKey() (a subsystem resolve + one TMap lookup) only runs for an
	// unmodified press while a prompt is actually up.
	if (bAcceptIsLive && bIsUnmodified && InKeyEvent.GetKey() == GetAcceptKey())
	{
		// ⛔ THE KEY IS CONSUMED ONLY BY AN ACCEPT THAT ACTUALLY HAPPENED, AND
		// THAT IS STRUCTURAL HERE RATHER THAN CHECKED AFTERWARDS: ConfirmPressed()
		// has EXACTLY ONE refusal — `if (!bConfirmPromptVisible)` — and that is
		// the condition this branch has already established as true. So the call
		// cannot be refused, and Handled below cannot be a lie.
		//
		// ⚠️ AND IT STAYS TRUE UNDER RE-ENTRANCY, WHICH IS THE ONLY WAY THIS COULD
		// HAVE GONE WRONG: a consumer of OnConsoleConfirmed may synchronously
		// raise a NEW prompt (the deferred-intent path does re-enter AwaitConfirm),
		// leaving bConfirmPromptVisible true again on return. Re-reading the flag
		// afterwards would then report "not accepted" and let the SAME physical
		// press ALSO type a `z` into the box — after it had already executed an
		// order. That is why acceptance is proven from the PRE-condition.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("[AssistantConsole] Accept key pressed (physical QWERTY-Z position; this layout yields '%s'). The prompt is answered; nothing else sees this key."),
			*InKeyEvent.GetKey().ToString());

		ConfirmPressed();

		return FReply::Handled();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// ⛔⛔ EVERY OTHER KEY, IN EVERY OTHER STATE, FALLS THROUGH UNTOUCHED — AND
	// `Escape` IS THE ONE THIS PROJECT WILL BE JUDGED ON.
	// ─────────────────────────────────────────────────────────────────────────
	// Jonathan CLOSED AS-§6 A-2 on 2026-08-04: Escape is left exactly as it is,
	// permanently, so the shipped placement / spell-targeting / group-pick cancel
	// routes keep firing byte-identically while the console is open. That clause
	// names THIS FUNCTION as a way to break the ruling, and returning Handled for
	// Escape — even "harmlessly" — would break it. ⛔ Note what is NOT in the code
	// above: the token `Escape` does not appear in it at all, which is the only
	// implementation that cannot drift into absorbing it.
	//
	// ⛔ AND THE FALL-THROUGH GOES THROUGH Super RATHER THAN A BARE
	// FReply::Unhandled(), DELIBERATELY. UUserWidget::NativeOnPreviewKeyDown is
	//     return OnPreviewKeyDown(InGeometry, InKeyEvent).NativeReply;
	// (UserWidget.cpp:2500-2503) — i.e. the BlueprintImplementableEvent route. In
	// v1 nothing implements it, and an unimplemented BIE returns a
	// default-constructed FEventReply whose NativeReply is FReply::Unhandled()
	// (SlateWrapperTypes.h:134-137), so this IS Unhandled today. Hard-coding
	// Unhandled instead would silently delete a Blueprint entry point from every
	// future subclass — the ruling-A(b) WBP included.
	// ⚠️ THAT PUTS ONE DUTY ON A FUTURE WBP_AssistantConsole AUTHOR: implementing
	// OnPreviewKeyDown and returning Handled for Escape would overturn Jonathan's
	// ruling from Blueprint. Don't.
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

USiegeKeyboardLayoutSubsystem* USiegeAssistantConsoleWidget::ResolveKeyboardLayoutSubsystem() const
{
	// Null-safe at every hop — the USettingsMenuWidget::ResolveSettingsSubsystem
	// shape, cloned. UWidget::GetGameInstance() is itself exactly
	// `if (UWorld* World = GetWorld()) { return World->GetGameInstance(); }`
	// (Widget.cpp:1167-1175), so this is the sibling widgets' World->GameInstance
	// hop with the hop already written by the engine.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<USiegeKeyboardLayoutSubsystem>();
	}

	return nullptr;
}

FKey USiegeAssistantConsoleWidget::GetAcceptKey() const
{
	// ⛔ FAIL-SAFE, AND IT IS KBD-§5's LAW APPLIED TO A SCALAR: no world, no
	// GameInstance or no subsystem all fall back to a plain EKeys::Z — never
	// EKeys::Invalid, and never "accept is unavailable". The failure mode of this
	// whole feature must be "the console behaves like a machine that never had a
	// layout subsystem", not "the player cannot accept an order".
	if (const USiegeKeyboardLayoutSubsystem* LayoutSubsystem = ResolveKeyboardLayoutSubsystem())
	{
		// ⛔ DIRECTION, AND GETTING IT BACKWARDS COMPILES AND SILENTLY BINDS THE
		// WRONG KEY: the map is SOURCE (QWERTY) -> what the ACTIVE layout yields at
		// that PHYSICAL POSITION. On US-Dvorak this returns EKeys::Semicolon,
		// because the position QWERTY prints `Z` on produces `;` there. The pinned
		// call-site form is `InKeyEvent.GetKey() == GetPositionalKey(EKeys::Z)`;
		// ⛔ NEVER a reverse lookup of the pressed key back into QWERTY space.
		// ⛔ And the accessor is const and does NOT re-probe — OpenConsole() is what
		// refreshes (KBD-§8: the caller refreshes, the accessor reads).
		return LayoutSubsystem->GetPositionalKey(EKeys::Z);
	}

	return EKeys::Z;
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
	//
	// ⚠️ TASK-519 moved the WRITE, not the meaning: the label is stored and the
	// line is composed in ONE place (RefreshStatusLine), because the status line
	// now carries the accept-key hint too and two writers would race — a state
	// push arriving while a prompt is up would otherwise erase the only thing on
	// screen telling the player how to accept.
	LastStateLabel = StateLabel;
	RefreshStatusLine();

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

		// ⚠️ THIS WRITE SURVIVES THE ApplyConsoleVisualState() CALL BELOW, AND THAT
		// IS ARRANGED, NOT LUCKY (TASK-519): RefreshStatusLine() returns early
		// while !bConsoleEnabled precisely so the fault reason is not overwritten
		// by the state label. bConsoleEnabled was assigned above, before any of
		// this ran. See RefreshStatusLine's comment.
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
		// ⚠️ DECLARED SIDE EFFECT OF ROUTING StatusText THROUGH ONE COMPOSER
		// (TASK-519): re-enabling now RESTORES the last FSM state label (or
		// "Ready") over the stale fault reason, because ApplyConsoleVisualState
		// below reaches RefreshStatusLine with bConsoleEnabled true again. Before
		// this task the disabled reason stayed on screen until the FSM happened to
		// push another state. This is a strict improvement and it is called out
		// rather than slipped in.
		UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Re-enabled."));
	}

	ApplyConsoleVisualState();

	OnConsoleEnabledChanged(bConsoleEnabled, DisabledReason);
}

// ---------------------------------------------------------------------------
// ⭐ The war-map input-insert seam (TASK-561, CONVENTIONS WR-§6 + WR-§5)
// ---------------------------------------------------------------------------
//
// ⭐⭐ THIS IS THE WHOLE INTERFACE BETWEEN THE BATTLEFIELD MAP AND THE ASSISTANT,
// AND IT IS DELIBERATELY ONE FUNCTION THAT MOVES ONE STRING INTO ONE TEXT BOX.
// Everything the batch's safety argument rests on is visible from here:
//   ✅ no coordinate, dot, count or marker geometry crosses this line — only a
//      literal place symbol the model ALREADY reads out of [FORCES] (WR-§6, the
//      coordinate airlock);
//   ✅ nothing is submitted, so no prompt is built and no model call is spent;
//   ✅ Zone A, the grammar, the JSON schema, the `who` shapes and the confirm
//      step are all untouched — this file cannot reach any of them.
// ⛔ A future edit that makes this function submit, build a sentence, resolve a
// place, or consult the vocabulary has broken that argument, whatever it gains.

// ---------------------------------------------------------------------------
// ⭐ THE SEPARATOR COMPOSITION — A PURE STATIC (TASK-581, WR-§6 ruling W4-R1)
// ---------------------------------------------------------------------------
//
// ⭐⭐ WHY THIS IS A NAMED FUNCTION AND NOT FOUR LINES INSIDE AppendToInput, AND
// IT IS LAW RATHER THAN TASTE: TASK-564 TRIED TO TEST THIS RULE AND PROVED IT
// UNASSERTABLE. Inline, reaching it needed a Slate-REALIZED widget tree plus an
// OPEN, ENABLED console, and InputBox is protected with no public text getter —
// so the composed string was UNREADABLE THROUGH THE SHIPPED PUBLIC API. It
// reported that instead of writing a replica test that would have asserted
// nothing about shipped code. ⇒ WR-§6: a rule that cannot be read cannot be
// tested, and this seam is the war map's ONLY channel to the AI — if it composes
// "to ancient_ground_nearand" the parse fails and the whole feature reads broken.
//
// ⛔ A static needs NO instance, NO Slate, NO world and NO CDO. THAT IS THE ENTIRE
// POINT OF THE EXTRACTION: Siegebound.WarMap.ComposeAppendedInputWhitespaceRule
// calls this directly, headlessly, and asserts every case byte-exactly.
//
// ⛔⛔ THE MOVE IS BEHAVIOUR-FREE AND THAT IS THE WHOLE DELIVERABLE. The body below
// is TASK-561's body, moved verbatim; the ONLY textual change is the two parameter
// names (Existing -> ExistingText, Symbol -> TrimmedSymbol). ⛔ Not one operator,
// not one operand and not one order of evaluation moved, so the produced string is
// identical for every input. See handoffs/TASK-581-programmer.md for the before and
// after side by side — an assertion that "it is just a move" is not evidence.
//
// ⛔ EVERY REFUSAL STAYED BEHIND IN AppendToInput. This function has NO guards on
// purpose: the empty-symbol case is refused up there, and duplicating that refusal
// here would be a second authority on the same question (CONVENTIONS §19).

FString USiegeAssistantConsoleWidget::ComposeAppendedInput(const FString& ExistingText, const FString& TrimmedSymbol)
{
	// ⛔ IT COMPOSES A NEW STRING FROM THE EXISTING ONE AND TOUCHES NOTHING ELSE IN
	// IT: no global whitespace normalisation, no re-casing, no head trim. A
	// half-typed sentence is the player's, and a presentation layer may not repair
	// its input (CONVENTIONS §31).

	// Rule 1: a separator only where one is actually missing. Checking the LAST
	// CHARACTER — rather than "does it end with a space" — also covers a tab, and
	// costs the same. The IsEmpty() test is what makes the index safe, so it is
	// first in the && by construction, not by habit.
	// ⚠️ NO NEW INCLUDE IS NEEDED FOR FChar, AND THAT WAS CHECKED RATHER THAN
	// ASSUMED — the same discipline the HAL/PlatformTime.h note at the top of this
	// file records, which found the OPPOSITE answer. CoreMinimal.h:60 includes
	// Misc/Char.h directly, at the installed UE 5.8 source on this machine.
	// Precedent in this module: SiegeAssistantSnapshot.cpp's flattener.
	const bool bNeedsLeadingSpace =
		!ExistingText.IsEmpty() && !FChar::IsWhitespace(ExistingText[ExistingText.Len() - 1]);

	FString Composed = ExistingText;
	if (bNeedsLeadingSpace)
	{
		Composed.AppendChar(TEXT(' '));
	}
	Composed.Append(TrimmedSymbol);

	// Rule 2: ALWAYS exactly one trailing space. It is what makes a second click
	// idempotent under rule 1 — "mid " + "hero" reads the trailing space and adds
	// no second one, so repeated clicks give "mid hero ", never "mid  hero".
	Composed.AppendChar(TEXT(' '));

	return Composed;
}

bool USiegeAssistantConsoleWidget::AppendToInput(const FString& TextToInsert)
{
	// ⛔ EVERY REFUSAL BELOW IS LOUD, AND THAT IS THE SHIPPED DOCTRINE APPLIED TO A
	// NEW SURFACE: a seam that quietly does nothing reads to the player as "I
	// clicked the map and the game ignored me" — the same trust failure as "the
	// assistant ate my order", which is why AS-§6 A-2 ruled the silent-discard case
	// out. ⛔ Never a silent no-op here either.

	FString Symbol = TextToInsert;
	Symbol.TrimStartAndEndInline();

	if (Symbol.IsEmpty())
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("[AssistantConsole] Insert refused: there is nothing to insert. The input box is unchanged — in particular no stray separator was appended."));
		return false;
	}

	if (!bConsoleEnabled)
	{
		// The fault latch's posture, honoured exactly as SubmitPressed honours it.
		// ⚠️ THIS IS NOT A GATE ON THE CONSOLE (WR-§5 RULING 5, "the console still
		// works anywhere") — it is the SHIPPED disabled state, in which
		// ApplyConsoleVisualState has already called InputBox->SetIsEnabled(false)
		// and nothing the player types can be submitted. Writing here would strand
		// text he can neither send nor clear.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("[AssistantConsole] Insert ignored: the console is disabled. The input box is unchanged. No key, card or command is affected."));
		return false;
	}

	if (!bConsoleOpen)
	{
		// ⛔ AND IT DOES NOT OPEN THE CONSOLE TO FIX THIS. Two mechanisms, both in
		// the header comment and both checkable here:
		//  (i)  the INPUT POSTURE belongs to ASiegePlayerController — this widget
		//       never calls SetInputMode (class comment §2), so a widget-initiated
		//       open would show a console on the wrong posture;
		//  (ii) OpenConsole() clears the box on EVERY open
		//       (InputBox->SetText(FText::GetEmpty())), so a write into a closed
		//       console would be destroyed a moment later, silently.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("[AssistantConsole] Insert refused: the console is closed. The caller opens it through ASiegePlayerController (the input-posture owner) first; this widget never opens itself."));
		return false;
	}

	if (InputBox == nullptr)
	{
		// ⛔ bWarnedNoInputBox IS DELIBERATELY NOT REUSED — see its neighbour's
		// comment in the header. Consuming the shipped latch here would silence
		// FocusInputBox()'s own warning, and suppressing an existing diagnostic is a
		// behaviour change on an existing path.
		if (!bWarnedNoInputBoxForAppend)
		{
			bWarnedNoInputBoxForAppend = true;
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("[AssistantConsole] No InputBox to insert into — the war map cannot hand the console a symbol. (Logged once.) The console, the map and the assistant are otherwise unaffected."));
		}
		return false;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// THE WHITESPACE RULE — ⭐ ONE READ AND ONE CALL (TASK-581, WR-§6 ruling W4-R1)
	// ─────────────────────────────────────────────────────────────────────────
	// ⛔ THE RULE ITSELF LIVES IN ComposeAppendedInput() ABOVE, WHICH IS WHAT MAKES
	// IT TESTABLE AT ALL — Siegebound.WarMap.ComposeAppendedInputWhitespaceRule
	// asserts every case headlessly. ⛔ THE EXTRACTION WAS BEHAVIOUR-FREE: the body
	// moved verbatim and only the parameter names changed, so the string this
	// function writes is identical to the one TASK-561 shipped, for every input.
	// ⛔ Do NOT re-inline it, and ⛔ do NOT add a second composition path here.
	const FString Existing = InputBox->GetText().ToString();
	const FString Composed = ComposeAppendedInput(Existing, Symbol);

	// ⛔ SetText THEN FocusInputBox, matching SubmitPressed and OpenConsole rather
	// than inventing a third order. Both orderings land the caret at
	// END-OF-DOCUMENT by two INDEPENDENT engine mechanisms (header comment (b) and
	// (c)), so this order is a consistency choice and not a correctness bet.
	//
	// ⚠️ NO EXISTING HANDLER FIRES FROM THIS. WireChildWidgets binds ONLY
	// InputBox->OnTextCommitted, and SetText commits nothing — OnTextChanged has no
	// subscriber in this class. Verified at WireChildWidgets, not assumed.
	InputBox->SetText(FText::FromString(Composed));
	FocusInputBox();

	// ⚠️ THE LENGTH, NOT THE CONTENT — the same shape SubmitPressed logs, and for
	// the same reason: the text is on screen in front of the player, and a log is
	// not where player text should accumulate.
	UE_LOG(LogSiegeAssistant, Log,
		TEXT("[AssistantConsole] War map inserted %d chars; the box now holds %d. ⛔ Nothing was submitted — the player still presses Enter himself."),
		Symbol.Len(), Composed.Len());

	return true;
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

	// ⭐ THE CONFIRM STEP'S v1 PLAYER SURFACE IS THIS LINE (TASK-519). It raises
	// and lowers the accept-key hint on exactly the bConfirmPromptVisible
	// lifecycle the two buttons used to have, so every existing call site
	// (ShowConfirmPrompt / HideConfirmPrompt / ConfirmPressed / CancelPressed /
	// CloseConsole / SetConsoleEnabled) drives it with no new plumbing.
	RefreshStatusLine();

	// ⚠️ A ConfirmButton ONLY EXISTS IF AN ASSET-AUTHORED TREE SUPPLIED ONE — the
	// v1 code-authored tree constructs none (see ConstructConsoleTree). This block
	// is therefore ruling A(b)'s escape hatch, kept live and unchanged so a future
	// WBP_AssistantConsole needs ZERO C++ edit. ⛔ The CancelButton block that sat
	// beside it is deleted with its button.
	// Collapsed rather than hidden, so the control takes no space when there is
	// nothing to confirm.
	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(
			bConfirmPromptVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		ConfirmButton->SetIsEnabled(bConsoleEnabled);
	}
}

void USiegeAssistantConsoleWidget::RefreshStatusLine()
{
	if (StatusText == nullptr)
	{
		return;
	}

	// ⛔ THE DISABLED CONSOLE OWNS ITS OWN LINE, AND YIELDING IT HERE IS THE POINT.
	// SetConsoleEnabled(false) writes a game-authored DisabledReason into
	// StatusText and then calls ApplyConsoleVisualState — which lands here. If
	// this wrote "Ready" over it, the fault reason would be visible for exactly
	// zero frames and the fault latch would look like a console that simply
	// stopped working.
	// ⚠️ It is safe by ORDER, not by luck: SetConsoleEnabled assigns bConsoleEnabled
	// BEFORE it calls CloseConsole() or writes the reason, so every path that
	// reaches this function while disabled takes the return below.
	if (!bConsoleEnabled)
	{
		return;
	}

	const FString BaseLine = LastStateLabel.IsEmpty()
		? FString(SiegeAssistantConsole::IdleStatusText)
		: LastStateLabel;

	// ⛔ THE HINT SAYS `Z` ON EVERY LAYOUT AND IS NEVER BUILT FROM GetAcceptKey()
	// (KBD-§8's last bullet). The lookup is for the comparison in
	// NativeOnPreviewKeyDown; this literal is for the human, whose keycap is
	// physically printed `Z` whatever their software layout says.
	StatusText->SetText(FText::FromString(
		bConfirmPromptVisible
			? FString::Printf(TEXT("%s\n%s"), *BaseLine, SiegeAssistantConsole::ConfirmHintText)
			: BaseLine));
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
