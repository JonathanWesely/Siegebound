// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeAssistantInputProbe.h"

#include "SiegeAssistantCommand.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericWindow.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWidget.h"

namespace SiegeAssistantInputProbe
{
	/** The sentence named by the acceptance criterion. Do not "improve" it. */
	static const TCHAR* ProbeSentence = TEXT("wasd send footmen");

	/** Seconds spent letting the deferred SlateOperations FReply settle before typing. */
	static constexpr float ArmSeconds = 0.35f;

	/** How long each typed key is held. A realistic keypress is ~0.1s; several frames at 60 Hz. */
	static constexpr float TypeHoldSeconds = 0.15f;

	/** Gap between two typed keys. */
	static constexpr float TypeGapSeconds = 0.05f;

	/** Hold for the Escape / Enter / RMB observations. */
	static constexpr float SpecialHoldSeconds = 0.20f;

	/** Post-typing window, so braking distance lands inside the measurement. */
	static constexpr float SettleSeconds = 0.35f;

	/** Below this the pass is treated as "did not move". Raw values are printed regardless. */
	static constexpr double MoveEpsilonCm = 5.0;

	/** Fraction of sampled frames that must have held focus for a focused pass to be conclusive. */
	static constexpr double RequiredFocusFraction = 0.90;

	/** Frames that must elapse after arming before the probe may start, whatever else is true. */
	static constexpr uint64 MinFramesAfterArm = 4;

	/** A pass whose mean frame time exceeds this gets a quantization warning printed beside it. */
	static constexpr double SlowPassFrameSeconds = 0.050;

	/** Windows virtual-key codes, so the synthetic events match what the platform layer supplies. */
	static constexpr uint32 VirtualKeyEscape = 0x1B;
	static constexpr uint32 VirtualKeyReturn = 0x0D;

	/**
	 *  ::MapVirtualKey(VK_ESCAPE / VK_RETURN, MAPVK_VK_TO_CHAR) values. Reproduced
	 *  so the events are byte-faithful to FWindowsApplication's WM_KEYDOWN path.
	 */
	static constexpr uint32 CharCodeEscape = 27;
	static constexpr uint32 CharCodeReturn = 13;

	static const FName TagType(TEXT("type"));
	static const FName TagEscape(TEXT("escape"));
	static const FName TagEnter(TEXT("enter"));
	static const FName TagRightMouse(TEXT("rmb"));

	/** Results survive the widget, so Siege.Assistant.InputProbeReport still works after teardown. */
	static TArray<FSiegeAssistantProbeResult>& GetResults()
	{
		static TArray<FSiegeAssistantProbeResult> Results;
		return Results;
	}

	/**
	 *  Set true ONLY when all three passes banked a row. A run that died halfway
	 *  leaves this false and the report prints a PARTIAL banner over whatever
	 *  rows it does have — rows are banked one at a time precisely so a session
	 *  that ends mid-run still leaves evidence behind.
	 */
	static bool& GetCompleted()
	{
		static bool bCompleted = false;
		return bCompleted;
	}

	/** True between BeginProbe and EndProbe, so a second arm can be refused rather than queued. */
	static bool& GetInFlight()
	{
		static bool bInFlight = false;
		return bInFlight;
	}

	/** One line describing the world the rows were measured in. Printed with every report. */
	static FString& GetEnvironmentLine()
	{
		static FString EnvironmentLine;
		return EnvironmentLine;
	}

	/** One line describing what the arming gate waited for before it started the run. */
	static FString& GetWarmUpLine()
	{
		static FString WarmUpLine;
		return WarmUpLine;
	}

	/** Why the Escape observation was not injected, in the probe's own words. Empty when it was. */
	static FString& GetEscapeSuppressionReason()
	{
		static FString Reason;
		return Reason;
	}

	static const TCHAR* GetPassDisplayName(ESiegeAssistantProbePass Pass)
	{
		switch (Pass)
		{
		case ESiegeAssistantProbePass::Control:   return TEXT("CONTROL (GameAndUI, focus on the GAME VIEWPORT)");
		case ESiegeAssistantProbePass::GameAndUI: return TEXT("MODE A  (GameAndUI + SetKeyboardFocus)");
		case ESiegeAssistantProbePass::UIOnly:    return TEXT("MODE B  (UIOnly + SetKeyboardFocus)");
		default: break;
		}
		return TEXT("?");
	}

	/**
	 *  Injects a key-down through the SAME entry point FWindowsApplication uses
	 *  after the platform message pump, so the focus path, the Slate bubble,
	 *  SViewport, UGameViewportClient::InputKey and Enhanced Input are all the
	 *  real shipping ones.
	 */
	static void InjectKeyDown(const FSiegeAssistantProbeAction& Action)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}

		FSlateApplication& SlateApp = FSlateApplication::Get();
		const FKeyEvent KeyEvent(
			Action.Key,
			SlateApp.GetModifierKeys(),
			static_cast<uint32>(SlateApp.GetUserIndexForKeyboard()),
			/*bIsRepeat*/ false,
			Action.CharCode,
			Action.KeyCode);
		SlateApp.ProcessKeyDownEvent(KeyEvent);

		if (Action.Character != 0)
		{
			// Windows follows WM_KEYDOWN with WM_CHAR carrying the shift-adjusted
			// character; the key-down carries the UPPERCASE MapVirtualKey code.
			// Reproducing both is what makes the text actually land in the box.
			const FCharacterEvent CharEvent(
				Action.Character,
				SlateApp.GetModifierKeys(),
				static_cast<uint32>(SlateApp.GetUserIndexForKeyboard()),
				/*bIsRepeat*/ false);
			SlateApp.ProcessKeyCharEvent(CharEvent);
		}
	}

	static void InjectKeyUp(const FSiegeAssistantProbeAction& Action)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}

		FSlateApplication& SlateApp = FSlateApplication::Get();
		const FKeyEvent KeyEvent(
			Action.Key,
			SlateApp.GetModifierKeys(),
			static_cast<uint32>(SlateApp.GetUserIndexForKeyboard()),
			/*bIsRepeat*/ false,
			Action.CharCode,
			Action.KeyCode);
		SlateApp.ProcessKeyUpEvent(KeyEvent);
	}

	static void InjectMouseButtonDown(const FKey& Button)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}

		FSlateApplication& SlateApp = FSlateApplication::Get();
		// auto, deliberately: GetCursorPos() returns
		// UE::Slate::FDeprecateVector2DResult, whose own header says client code
		// must not name it. It converts implicitly into FPointerEvent's
		// FDeprecateVector2DParameter, so it is passed straight through.
		const auto CursorPos = SlateApp.GetCursorPos();

		TSet<FKey> PressedButtons;
		PressedButtons.Add(Button);

		const FPointerEvent PointerEvent(
			/*PointerIndex*/ 0,
			CursorPos,
			CursorPos,
			PressedButtons,
			Button,
			/*WheelDelta*/ 0.f,
			SlateApp.GetModifierKeys());

		SlateApp.ProcessMouseButtonDownEvent(TSharedPtr<FGenericWindow>(), PointerEvent);
	}

	static void InjectMouseButtonUp(const FKey& Button)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}

		FSlateApplication& SlateApp = FSlateApplication::Get();
		// auto, deliberately: GetCursorPos() returns
		// UE::Slate::FDeprecateVector2DResult, whose own header says client code
		// must not name it. It converts implicitly into FPointerEvent's
		// FDeprecateVector2DParameter, so it is passed straight through.
		const auto CursorPos = SlateApp.GetCursorPos();

		const TSet<FKey> PressedButtons;

		const FPointerEvent PointerEvent(
			/*PointerIndex*/ 0,
			CursorPos,
			CursorPos,
			PressedButtons,
			Button,
			/*WheelDelta*/ 0.f,
			SlateApp.GetModifierKeys());

		SlateApp.ProcessMouseButtonUpEvent(PointerEvent);
	}

	/**
	 *  Describes ONE key observation without ever inventing one. The distinction
	 *  between "the key was sent and the game did not see it" and "the key was
	 *  never sent" is the whole difference between a measurement and a fiction.
	 */
	static FString DescribeObservation(bool bInjected, bool bReached, bool bFocusedAtPress, const TCHAR* AbsorbedNote, const FString& NotInjectedReason)
	{
		if (!bInjected)
		{
			return FString::Printf(TEXT("NOT INJECTED — %s"),
				NotInjectedReason.IsEmpty() ? TEXT("the pass ended before this observation was reached") : *NotInjectedReason);
		}

		if (bReached)
		{
			return FString::Printf(TEXT("YES — reached UPlayerInput   (box focused at press: %s)"),
				bFocusedAtPress ? TEXT("yes") : TEXT("NO — this observation is contaminated, the box had already lost focus"));
		}

		return FString::Printf(TEXT("NO — never reached UPlayerInput   (box focused at press: %s%s)"),
			bFocusedAtPress ? TEXT("yes") : TEXT("no"),
			bFocusedAtPress ? AbsorbedNote : TEXT(""));
	}

	/** Prints the stored comparison table. Shared by the two console commands. */
	static void LogReport()
	{
		const TArray<FSiegeAssistantProbeResult>& Results = GetResults();

		UE_LOG(LogSiegeAssistant, Display, TEXT("================ Siege.Assistant.InputProbe — SPIKE MEASUREMENT #6 (TASK-411) ================"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("QUESTION: does a focused UEditableTextBox starve Enhanced Input of WASD?"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("CRITERION: typing '%s' into the focused box must not move the hero."), ProbeSentence);

		if (Results.Num() == 0)
		{
			UE_LOG(LogSiegeAssistant, Warning, TEXT("No probe has produced a row in this session. Run 'Siege.Assistant.InputProbe' first."));
			return;
		}

		if (!GetEnvironmentLine().IsEmpty())
		{
			UE_LOG(LogSiegeAssistant, Display, TEXT("ENVIRONMENT: %s"), *GetEnvironmentLine());
		}
		if (!GetWarmUpLine().IsEmpty())
		{
			UE_LOG(LogSiegeAssistant, Display, TEXT("ARMING     : %s"), *GetWarmUpLine());
		}

		if (!GetCompleted())
		{
			UE_LOG(LogSiegeAssistant, Warning, TEXT("⚠️ PARTIAL RUN — %d of %d passes banked a row. The rows below are real measurements,"),
				Results.Num(), static_cast<int32>(ESiegeAssistantProbePass::Count));
			UE_LOG(LogSiegeAssistant, Warning, TEXT("   but the COMPARISON IS INCOMPLETE and no binary answer may be read off it."));
		}

		for (const FSiegeAssistantProbeResult& Row : Results)
		{
			UE_LOG(LogSiegeAssistant, Display, TEXT("---- %s ----"), *Row.PassName);

			if (!Row.bRan)
			{
				UE_LOG(LogSiegeAssistant, Warning, TEXT("  DID NOT RUN — %s"), *Row.VerdictNote);
				continue;
			}

			UE_LOG(LogSiegeAssistant, Display, TEXT("  VERDICT              : %s"), *Row.Verdict);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  why                  : %s"), *Row.VerdictNote);

			if (Row.ControlAttemptsUsed > 0)
			{
				UE_LOG(LogSiegeAssistant, Display, TEXT("  control attempts     : %d"), Row.ControlAttemptsUsed);
				if (!Row.ControlAttemptHistory.IsEmpty())
				{
					UE_LOG(LogSiegeAssistant, Display, TEXT("  earlier attempts     : %s"), *Row.ControlAttemptHistory);
				}
				UE_LOG(LogSiegeAssistant, Display, TEXT("  viewport focus       : %s"),
					Row.bViewportFocusEstablished
						? TEXT("VERIFIED on the game viewport widget before typing")
						: TEXT("⛔ NOT ESTABLISHED — the keystrokes had no route into the input stack"));
			}

			UE_LOG(LogSiegeAssistant, Display, TEXT("  hero start           : %s"), *Row.StartLocation.ToString());
			UE_LOG(LogSiegeAssistant, Display, TEXT("  hero end             : %s"), *Row.EndLocation.ToString());
			UE_LOG(LogSiegeAssistant, Display, TEXT("  NET delta (cm)       : %.2f"), Row.NetDeltaCm);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  PATH length (cm)     : %.2f   <-- primary movement evidence (cannot cancel)"), Row.PathLengthCm);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  PATH while typing(cm): %.2f"), Row.TypingWindowPathLengthCm);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  max speed (cm/s)     : %.2f"), Row.MaxSpeedCms);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  max move-input mag   : %.3f   (UPawnMovementComponent::GetLastInputVector — 'did IA_Move fire')"), Row.MaxMoveInputMagnitude);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  W/A/S/D on PlayerInput: %s"), Row.bMoveKeyReachedPlayerInput ? TEXT("YES — the viewport got them") : TEXT("no — Slate absorbed them"));
			UE_LOG(LogSiegeAssistant, Display, TEXT("  focus held (typing)  : %d / %d frames (requested: %s); %d frames sampled in the whole pass"),
				Row.FocusHeldSamples, Row.FocusWindowSamples, Row.bFocusRequested ? TEXT("yes") : TEXT("no"), Row.SampledFrames);

			const double MeanFrameSeconds = (Row.SampledFrames > 0) ? (Row.PassSeconds / static_cast<double>(Row.SampledFrames)) : 0.0;
			UE_LOG(LogSiegeAssistant, Display, TEXT("  pass timing          : %.2f s over %d frames — mean %.1f ms, worst %.1f ms"),
				Row.PassSeconds, Row.SampledFrames, MeanFrameSeconds * 1000.0, Row.WorstFrameSeconds * 1000.f);
			if (MeanFrameSeconds > SlowPassFrameSeconds)
			{
				UE_LOG(LogSiegeAssistant, Warning, TEXT("  ⚠️ this pass ran BELOW 20 FPS — every per-key hold window quantized to whole frames."));
			}

			UE_LOG(LogSiegeAssistant, Display, TEXT("  text landed in box   : %s  ('%s')"),
				Row.bTypedTextLanded ? TEXT("YES") : TEXT("no"), *Row.TextInBoxAfterTyping);
			UE_LOG(LogSiegeAssistant, Display, TEXT("  Escape -> PlayerInput: %s"),
				*DescribeObservation(Row.bEscapeInjected, Row.bEscapeReachedPlayerInput, Row.bBoxFocusedAtEscape, TEXT(""), GetEscapeSuppressionReason()));
			UE_LOG(LogSiegeAssistant, Display, TEXT("  Enter  -> PlayerInput: %s"),
				*DescribeObservation(Row.bEnterInjected, Row.bEnterReachedPlayerInput, Row.bBoxFocusedAtEnter, TEXT(" — the box absorbs Enter via HandleCarriageReturn"), FString()));
			UE_LOG(LogSiegeAssistant, Display, TEXT("  Enter committed text : %s"), Row.bEnterCommittedText ? TEXT("YES") : TEXT("no"));
			UE_LOG(LogSiegeAssistant, Display, TEXT("  focus survived Enter : %s"), Row.bFocusSurvivedEnter ? TEXT("YES") : TEXT("no — ClearKeyboardFocusOnCommit dropped it"));
			UE_LOG(LogSiegeAssistant, Display, TEXT("  RMB    -> PlayerInput: %s"),
				*DescribeObservation(Row.bRightMouseInjected, Row.bRightMouseReachedPlayerInput, Row.bBoxFocusedAtRightMouse, TEXT(""), FString()));
			UE_LOG(LogSiegeAssistant, Display, TEXT("  RMB cursor was at    : (%.0f, %.0f)   (RMB routes through the POINTER path — it must be over the game viewport to mean anything)"),
				Row.RightMouseCursorX, Row.RightMouseCursorY);
		}

		UE_LOG(LogSiegeAssistant, Display, TEXT("------------------------------------------------------------------------------------------"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("READING IT: the CONTROL row must be CONTROL-OK. If it is CONTROL-FAILED the injection never"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("reached the input stack and the other two rows say nothing — they are reported INCONCLUSIVE,"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("never PASS. MODE A PASS => the console ships on FInputModeGameAndUI and the camera stays live."));
		UE_LOG(LogSiegeAssistant, Display, TEXT("MODE A FAIL => fall back to FInputModeUIOnly while the box has focus (MODE B), which also"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("kills mouse-look and the shipped Escape/RMB cancel routes for as long as the console is open."));
		UE_LOG(LogSiegeAssistant, Display, TEXT("SECONDARY RESULT: compare the Enter and RMB rows of MODE A against MODE B. UIOnly calls"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("SetIgnoreInput(true) (PlayerController.cpp:6384) and GameAndUI calls SetIgnoreInput(false)"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("(:6410), so a UIOnly fallback is expected to swallow BOTH — that is a real cost the design"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("must price in, because RMB cancels group picks and Enter is the console's own open key."));
		UE_LOG(LogSiegeAssistant, Display, TEXT("⚠️ A 'NOT INJECTED' row is NOT a swallow. In PIE the Escape observation is deliberately not"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("sent at all (it is the editor's StopPlaySession chord); the Escape matrix comes from a"));
		UE_LOG(LogSiegeAssistant, Display, TEXT("standalone -game run only."));
		UE_LOG(LogSiegeAssistant, Display, TEXT("=========================================================================================="));
	}
}

USiegeAssistantInputProbeWidget::USiegeAssistantInputProbeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Nothing to configure. Two defaults this probe RELIES on, named because
	// neither is obvious and B3 will need both:
	//  1. UUserWidget already defaults Visibility to SelfHitTestInvisible, so the
	//     probe never blocks the game viewport's mouse input.
	//  2. TickFrequency defaults to EWidgetTickFrequency::Auto, which ticks a
	//     widget "if the widget inherits from something other than UserWidget
	//     ... so that native C++ or inherited ticks function". That is what
	//     drives NativeTick here; a Blueprint-only widget would NOT tick.
}

const TCHAR* USiegeAssistantInputProbeWidget::GetProbeSentence()
{
	return SiegeAssistantInputProbe::ProbeSentence;
}

TSharedRef<SWidget> USiegeAssistantInputProbeWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING, AND THIS IS THE HEADLINE FINDING FOR RULING A.
	// UUserWidget::RebuildWidget() reads WidgetTree->RootWidget AS IT STANDS at
	// the moment it is called and returns an SSpacer when it is null. So a
	// code-authored tree MUST be constructed BEFORE Super::RebuildWidget();
	// building it after (and returning Super's result) yields a silently EMPTY
	// widget that still passes every property readback. That is exactly the
	// class of defect this project's UMG verification law exists for.
	ConstructProbeTree();
	return Super::RebuildWidget();
}

void USiegeAssistantInputProbeWidget::ConstructProbeTree()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	// RULING A ESCAPE HATCH: if an asset-authored tree exists, it wins whole.
	// The BindWidgetOptional members were already resolved by UMG, so there is
	// nothing to construct and nothing to overwrite. A future WBP therefore
	// costs zero C++ change.
	if (WidgetTree->RootWidget != nullptr)
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("USiegeAssistantInputProbeWidget: an asset-authored tree is present — the code-authored branch is skipped (ruling A escape hatch)."));
		return;
	}

	if (RootPanel == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
	}

	if (RootPanel == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("USiegeAssistantInputProbeWidget: could not construct RootPanel — the probe cannot show a text box."));
		return;
	}

	// Keep the container itself click-through so the RMB observation below can
	// still reach the game viewport; only the box and the label are hit-testable.
	RootPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootPanel;

	if (StatusText == nullptr)
	{
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		if (StatusText != nullptr)
		{
			StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (UVerticalBoxSlot* StatusSlot = RootPanel->AddChildToVerticalBox(StatusText))
			{
				StatusSlot->SetPadding(FMargin(24.f, 24.f, 24.f, 4.f));
				StatusSlot->SetHorizontalAlignment(HAlign_Fill);
				StatusSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (InputBox == nullptr)
	{
		InputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputBox"));
		if (InputBox != nullptr)
		{
			// Defaults that matter to the measurement, made explicit rather than
			// inherited: an editable (not read-only) box is what
			// SEditableText::SupportsKeyboardFocus() reports focusable, and
			// RevertTextOnEscape decides whether Escape is absorbed or bubbles.
			InputBox->SetIsReadOnly(false);
			InputBox->SetRevertTextOnEscape(false);
			InputBox->SetClearKeyboardFocusOnCommit(true);
			InputBox->SetHintText(FText::FromString(FString(TEXT("TASK-411 probe — type here"))));

			if (UVerticalBoxSlot* BoxSlot = RootPanel->AddChildToVerticalBox(InputBox))
			{
				BoxSlot->SetPadding(FMargin(24.f, 4.f, 24.f, 24.f));
				BoxSlot->SetHorizontalAlignment(HAlign_Fill);
				BoxSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (InputBox == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("USiegeAssistantInputProbeWidget: could not construct InputBox — the probe has nothing to focus."));
	}
}

void USiegeAssistantInputProbeWidget::NativeDestruct()
{
	if (bProbeRunning)
	{
		EndProbe(TEXT("the probe widget was destroyed mid-run"));
	}

	Super::NativeDestruct();
}

APlayerController* USiegeAssistantInputProbeWidget::GetProbeController() const
{
	return GetOwningPlayer();
}

APawn* USiegeAssistantInputProbeWidget::GetProbePawn() const
{
	const APlayerController* Controller = GetProbeController();
	return Controller != nullptr ? Controller->GetPawn() : nullptr;
}

UPawnMovementComponent* USiegeAssistantInputProbeWidget::GetProbeMovement() const
{
	const APawn* ProbePawn = GetProbePawn();
	return ProbePawn != nullptr ? ProbePawn->GetMovementComponent() : nullptr;
}

bool USiegeAssistantInputProbeWidget::IsInputBoxFocused() const
{
	if (InputBox == nullptr || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	const TSharedPtr<SWidget> BoxSlate = InputBox->GetCachedWidget();
	if (!BoxSlate.IsValid())
	{
		return false;
	}

	const TSharedPtr<SWidget> FocusedSlate = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (!FocusedSlate.IsValid())
	{
		return false;
	}

	// SEditableTextBox::OnFocusReceived re-targets focus to its inner
	// SEditableText, so the focused widget is a DESCENDANT of the box, never the
	// box itself. Comparing against the box directly would report "not focused"
	// on a correctly focused box — a false INCONCLUSIVE.
	// NOTE: this local must NOT be called "Cursor" — UWidget declares a reflected
	// UPROPERTY of that name, and shadowing it is C4458, which is FATAL here
	// (warnings-as-errors). Standing no-shadowing-inherited-members rule.
	for (TSharedPtr<SWidget> Walker = FocusedSlate; Walker.IsValid(); Walker = Walker->GetParentWidget())
	{
		if (Walker == BoxSlate)
		{
			return true;
		}
	}

	return false;
}

bool USiegeAssistantInputProbeWidget::IsGameViewportFocused() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const UWorld* ProbeWorld = GetWorld();
	const UGameViewportClient* ProbeViewportClient = (ProbeWorld != nullptr) ? ProbeWorld->GetGameViewport() : nullptr;
	if (ProbeViewportClient == nullptr)
	{
		return false;
	}

	const TSharedPtr<SViewport> ProbeViewportSlate = ProbeViewportClient->GetGameViewportWidget();
	if (!ProbeViewportSlate.IsValid())
	{
		return false;
	}

	const TSharedPtr<SWidget> FocusedSlate = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (!FocusedSlate.IsValid())
	{
		return false;
	}

	// ⚠️ THE BOX EXCLUSION IS THE LOAD-BEARING HALF. The probe's own widget is a
	// DESCENDANT of the game viewport widget (AddToViewport parents it under the
	// viewport's game layers), so "the viewport is somewhere up the chain" is
	// ALSO true when the text box holds focus. Without this refusal a focused-box
	// pass could be mistaken for a valid control.
	if (IsInputBoxFocused())
	{
		return false;
	}

	// What the CONTROL actually needs is that the focus path REACHES SViewport,
	// because that is the only route by which a key reaches
	// UGameViewportClient::InputKey. Identity is the expected case
	// (FSceneViewport::OnFocusReceived does not re-target focus to a child), but
	// accepting an ancestor match too costs nothing and cannot manufacture a
	// false CONTROL-OK: a control only passes if the hero MOVED, which cannot
	// happen unless the keys genuinely reached the game.
	const SWidget* ProbeViewportRaw = ProbeViewportSlate.Get();
	for (TSharedPtr<SWidget> Walker = FocusedSlate; Walker.IsValid(); Walker = Walker->GetParentWidget())
	{
		if (Walker.Get() == ProbeViewportRaw)
		{
			return true;
		}
	}

	return false;
}

bool USiegeAssistantInputProbeWidget::FocusGameViewport()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	FSlateApplication& SlateApp = FSlateApplication::Get();

	// ⚠️ THE TASK-413 ROOT CAUSE, FIXED HERE. The old control called
	// ClearKeyboardFocus(), which leaves SlateUser::GetFocusPath() EMPTY —
	// FSlateApplication::ProcessKeyDownEvent routes ONLY along that path
	// (SlateApplication.cpp:5018), so no synthetic key reached any widget, the
	// hero could not move, and Escape fell through to the editor's
	// UnhandledKeyDownEventHandler, which stops PIE. The shipping state with no
	// UI up is "the game viewport widget holds keyboard focus", and that is what
	// the control must reproduce. Note it is NOT implied by the input mode:
	// FInputModeDataBase::SetFocusAndLocking focuses only a valid WidgetToFocus.
	const UWorld* ProbeWorld = GetWorld();
	const UGameViewportClient* ProbeViewportClient = (ProbeWorld != nullptr) ? ProbeWorld->GetGameViewport() : nullptr;
	const TSharedPtr<SViewport> ProbeViewportSlate = (ProbeViewportClient != nullptr) ? ProbeViewportClient->GetGameViewportWidget() : TSharedPtr<SViewport>();

	if (ProbeViewportSlate.IsValid())
	{
		SlateApp.SetKeyboardFocus(ProbeViewportSlate, EFocusCause::SetDirectly);
	}
	else
	{
		// Fallback for a viewport Slate does know about but the world does not
		// hand out. Costs nothing and cannot make things worse.
		SlateApp.SetUserFocusToGameViewport(static_cast<uint32>(SlateApp.GetUserIndexForKeyboard()), EFocusCause::SetDirectly);
	}

	return IsGameViewportFocused();
}

void USiegeAssistantInputProbeWidget::SetStatusLine(const FString& Line)
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::FromString(Line));
	}
}

void USiegeAssistantInputProbeWidget::HandleProbeTextCommitted(const FText& CommittedText, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		Working.bEnterCommittedText = true;
	}
}

void USiegeAssistantInputProbeWidget::BeginProbe(const FSiegeAssistantProbeOptions& InOptions)
{
	using namespace SiegeAssistantInputProbe;

	if (bProbeRunning)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("Siege.Assistant.InputProbe: a probe is already running — the second request is refused, not queued."));
		return;
	}

	APlayerController* Controller = GetProbeController();
	if (Controller == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: no owning player controller — cannot measure."));
		return;
	}

	if (GetProbePawn() == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: the player controller has no pawn — there is no hero to watch."));
		return;
	}

	if (InputBox == nullptr)
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: InputBox was never constructed — cannot measure."));
		return;
	}

	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: FSlateApplication is not initialized — cannot inject keystrokes."));
		return;
	}

	ProbeOptions = InOptions;

	const UWorld* ProbeWorld = GetWorld();
	bInPIEWorld = (ProbeWorld != nullptr) && (ProbeWorld->WorldType == EWorldType::PIE);

	// ---- the Escape policy, resolved ONCE ---------------------------------
	// In a PIE world Escape is the editor's StopPlaySession chord and it ends
	// the session from ANY pass: a plain UEditableTextBox does not absorb it
	// (HandleEscape returns false with RevertTextOnEscape=false, no selection,
	// no search text), FSceneViewport::OnKeyDown returns Unhandled when the
	// viewport client does not consume the key, and the event then bubbles to
	// SGlobalPlayWorldActions.
	switch (ProbeOptions.EscapePolicy)
	{
	case ESiegeAssistantProbeEscapePolicy::Force:
		bEscapeInjectionAllowed = true;
		break;
	case ESiegeAssistantProbeEscapePolicy::Never:
		bEscapeInjectionAllowed = false;
		break;
	case ESiegeAssistantProbeEscapePolicy::Auto:
	default:
		bEscapeInjectionAllowed = !bInPIEWorld;
		break;
	}

	if (bEscapeInjectionAllowed)
	{
		GetEscapeSuppressionReason().Reset();
		if (bInPIEWorld)
		{
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Siege.Assistant.InputProbe: 'escape' was forced in a PIE world. Escape is the editor's StopPlaySession chord — THIS RUN WILL END PIE. Rows banked before that point survive in the log."));
		}
	}
	else if (ProbeOptions.EscapePolicy == ESiegeAssistantProbeEscapePolicy::Never)
	{
		GetEscapeSuppressionReason() = TEXT("'noescape' was requested on the command line");
	}
	else
	{
		GetEscapeSuppressionReason() = TEXT("PIE world — EKeys::Escape is the editor's StopPlaySession chord and would end the session; run standalone -game for the Escape matrix");
	}

	InputBox->OnTextCommitted.AddUniqueDynamic(this, &USiegeAssistantInputProbeWidget::HandleProbeTextCommitted);

	// Capture the posture so EndProbe puts it back. ASiegePlayerController owns
	// the real composition in ApplyCursorInputState(); this probe only restores
	// the two flags it found and the matching mode, and never calls into that
	// class (TASK-411 owns no shipped file).
	bRestoreShowMouseCursor = Controller->bShowMouseCursor;
	bRestoreEnableClickEvents = Controller->bEnableClickEvents;
	bCapturedRestoreState = true;

	GetResults().Reset();
	GetCompleted() = false;
	GetInFlight() = true;

	GetEnvironmentLine() = FString::Printf(
		TEXT("%s · world '%s' · Escape observation %s"),
		bInPIEWorld ? TEXT("PIE") : TEXT("standalone game"),
		ProbeWorld != nullptr ? *ProbeWorld->GetName() : TEXT("?"),
		bEscapeInjectionAllowed ? TEXT("INJECTED") : TEXT("NOT INJECTED"));

	bProbeRunning = true;
	PassIndex = 0;
	ControlAttempt = 0;
	ControlAttemptHistory.Reset();
	ProbeStartRealSeconds = FPlatformTime::Seconds();

	UE_LOG(LogSiegeAssistant, Display,
		TEXT("Siege.Assistant.InputProbe: starting 3 passes (CONTROL, MODE A GameAndUI+focus, MODE B UIOnly+focus). Sentence: '%s'. Environment: %s."),
		ProbeSentence, *GetEnvironmentLine());

	EnterPass();
}

void USiegeAssistantInputProbeWidget::EnterPass()
{
	using namespace SiegeAssistantInputProbe;

	const ESiegeAssistantProbePass Pass = static_cast<ESiegeAssistantProbePass>(PassIndex);

	Working = FSiegeAssistantProbeResult();
	Working.PassName = GetPassDisplayName(Pass);
	Working.bRan = true;

	Actions.Reset();
	ActionIndex = 0;
	bActionHeld = false;
	bHasLastSampledLocation = false;
	bCapturedTypedText = false;
	bInTypingWindow = false;
	PhaseTime = 0.f;

	APlayerController* Controller = GetProbeController();
	if (Controller == nullptr || InputBox == nullptr)
	{
		Working.bRan = false;
		Working.VerdictNote = TEXT("controller or InputBox went away between passes");
		LeavePass();
		return;
	}

	// ---- posture ---------------------------------------------------------
	// The cursor must be up for either UI posture to be usable at all.
	Controller->bShowMouseCursor = true;
	Controller->bEnableClickEvents = true;

	const TSharedPtr<SWidget> BoxSlate = InputBox->GetCachedWidget();

	if (Pass == ESiegeAssistantProbePass::UIOnly)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		if (BoxSlate.IsValid())
		{
			InputMode.SetWidgetToFocus(BoxSlate);
		}
		Controller->SetInputMode(InputMode);
		Working.bFocusRequested = true;
	}
	else
	{
		// Byte-identical to the cursor branch ASiegePlayerController::
		// ApplyCursorInputState() applies (read, not edited): GameAndUI +
		// DoNotLock + HideCursorDuringCapture(false).
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);

		if (Pass == ESiegeAssistantProbePass::GameAndUI && BoxSlate.IsValid())
		{
			// SetWidgetToFocus AND SetKeyboardFocus below, deliberately. The
			// FReply that SetInputMode fills is DEFERRED (ULocalPlayer's
			// accumulated SlateOperations, applied once per frame by
			// LaunchEngineLoop's ProcessPlayerControllersSlateOperations), so a
			// SetKeyboardFocus made in the same frame can be quietly re-applied
			// over. Setting both makes the two agree.
			InputMode.SetWidgetToFocus(BoxSlate);
			Working.bFocusRequested = true;
		}

		Controller->SetInputMode(InputMode);
	}

	if (Pass == ESiegeAssistantProbePass::Control)
	{
		// The control deliberately does NOT focus the box. It focuses the GAME
		// VIEWPORT WIDGET instead, which is the real shipping keyboard-focus
		// target with no UI up — and, critically, the only thing that gives the
		// synthetic keys a focus path to travel along at all. See FocusGameViewport.
		Working.bFocusRequested = false;
		FocusGameViewport();
	}
	else
	{
		InputBox->SetText(FText::GetEmpty());
		InputBox->SetKeyboardFocus();
	}

	// ---- the action script ----------------------------------------------
	const FString Sentence(ProbeSentence);
	for (int32 CharIndex = 0; CharIndex < Sentence.Len(); ++CharIndex)
	{
		const TCHAR Character = Sentence[CharIndex];

		// Windows' WM_KEYDOWN carries ::MapVirtualKey(VK, MAPVK_VK_TO_CHAR),
		// which is the UPPERCASE form for letters and 0x20 for space; the
		// virtual-key code is the same value for both. Reproduce exactly.
		const uint32 UpperCode = static_cast<uint32>(FChar::ToUpper(Character));

		const FKey ResolvedKey = FInputKeyManager::Get().GetKeyFromCodes(UpperCode, UpperCode);
		if (!ResolvedKey.IsValid())
		{
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Siege.Assistant.InputProbe: no FKey for character index %d — that character is skipped."), CharIndex);
			continue;
		}

		FSiegeAssistantProbeAction Action;
		Action.Key = ResolvedKey;
		Action.KeyCode = UpperCode;
		Action.CharCode = UpperCode;
		Action.Character = Character;
		Action.HoldSeconds = TypeHoldSeconds;
		Action.Tag = TagType;
		Actions.Add(Action);
	}

	{
		// Escape BEFORE Enter: Enter commits, and ClearKeyboardFocusOnCommit
		// then drops focus, which would invalidate any later observation.
		//
		// ⚠️ The Escape slot is KEPT even when policy forbids sending it, so the
		// pass timeline is identical in PIE and standalone and the two runs stay
		// comparable. Nothing is injected for a suppressed action and the report
		// says NOT INJECTED — it never reports an observation it did not make.
		FSiegeAssistantProbeAction EscapeAction;
		EscapeAction.Key = EKeys::Escape;
		EscapeAction.KeyCode = VirtualKeyEscape;
		EscapeAction.CharCode = CharCodeEscape;
		EscapeAction.HoldSeconds = SpecialHoldSeconds;
		EscapeAction.Tag = TagEscape;
		EscapeAction.bSuppressed = !bEscapeInjectionAllowed;
		Actions.Add(EscapeAction);

		FSiegeAssistantProbeAction EnterAction;
		EnterAction.Key = EKeys::Enter;
		EnterAction.KeyCode = VirtualKeyReturn;
		EnterAction.CharCode = CharCodeReturn;
		EnterAction.HoldSeconds = SpecialHoldSeconds;
		EnterAction.Tag = TagEnter;
		Actions.Add(EnterAction);

		FSiegeAssistantProbeAction MouseAction;
		MouseAction.Key = EKeys::RightMouseButton;
		MouseAction.HoldSeconds = SpecialHoldSeconds;
		MouseAction.bIsMouseButton = true;
		MouseAction.Tag = TagRightMouse;
		Actions.Add(MouseAction);
	}

	Phase = EProbePhase::Arming;
	SetStatusLine(FString::Printf(TEXT("TASK-411 probe — arming %s"), *Working.PassName));

	if (Pass == ESiegeAssistantProbePass::Control)
	{
		UE_LOG(LogSiegeAssistant, Display, TEXT("Siege.Assistant.InputProbe: pass %d/%d — %s (control attempt %d of %d)"),
			PassIndex + 1, static_cast<int32>(ESiegeAssistantProbePass::Count), *Working.PassName,
			ControlAttempt + 1, ProbeOptions.MaxControlAttempts);
	}
	else
	{
		UE_LOG(LogSiegeAssistant, Display, TEXT("Siege.Assistant.InputProbe: pass %d/%d — %s"),
			PassIndex + 1, static_cast<int32>(ESiegeAssistantProbePass::Count), *Working.PassName);
	}
}

void USiegeAssistantInputProbeWidget::SampleFrame(float InDeltaTime)
{
	const APawn* ProbePawn = GetProbePawn();
	const APlayerController* Controller = GetProbeController();
	if (ProbePawn == nullptr || Controller == nullptr)
	{
		return;
	}

	++Working.SampledFrames;
	Working.PassSeconds += static_cast<double>(InDeltaTime);
	Working.WorstFrameSeconds = FMath::Max(Working.WorstFrameSeconds, InDeltaTime);

	const FVector CurrentLocation = ProbePawn->GetActorLocation();
	if (bHasLastSampledLocation)
	{
		const double StepCm = FVector::Dist(LastSampledLocation, CurrentLocation);
		Working.PathLengthCm += StepCm;
		if (bInTypingWindow)
		{
			Working.TypingWindowPathLengthCm += StepCm;
		}
	}
	LastSampledLocation = CurrentLocation;
	bHasLastSampledLocation = true;

	Working.MaxSpeedCms = FMath::Max(Working.MaxSpeedCms, static_cast<float>(ProbePawn->GetVelocity().Size()));

	if (const UPawnMovementComponent* Movement = GetProbeMovement())
	{
		Working.MaxMoveInputMagnitude = FMath::Max(
			Working.MaxMoveInputMagnitude,
			static_cast<float>(Movement->GetLastInputVector().Size()));
	}

	// Focus is only REQUIRED while the sentence is being typed. After the
	// synthetic Enter, ClearKeyboardFocusOnCommit legitimately drops it, and
	// counting those frames against the pass would produce a false INCONCLUSIVE.
	if (bInTypingWindow)
	{
		++Working.FocusWindowSamples;
		if (Working.bFocusRequested && IsInputBoxFocused())
		{
			++Working.FocusHeldSamples;
		}
	}

	// "Did the key reach Enhanced Input" — UPlayerInput only ever sees a key that
	// UGameViewportClient::InputKey forwarded, so this is a direct read of
	// whether Slate let it through.
	if (Controller->IsInputKeyDown(EKeys::W) || Controller->IsInputKeyDown(EKeys::A)
		|| Controller->IsInputKeyDown(EKeys::S) || Controller->IsInputKeyDown(EKeys::D))
	{
		Working.bMoveKeyReachedPlayerInput = true;
	}

	if (bActionHeld && Actions.IsValidIndex(ActionIndex))
	{
		FSiegeAssistantProbeAction& Current = Actions[ActionIndex];

		// ⚠️ Only sample a key the probe ACTUALLY SENT. Reading UPlayerInput for a
		// suppressed action could pick up a press from somewhere else entirely and
		// report it as this probe's observation.
		if (Current.bInjected && Current.Key.IsValid() && Controller->IsInputKeyDown(Current.Key))
		{
			Current.bReachedPlayerInput = true;
		}
	}
}

void USiegeAssistantInputProbeWidget::PressCurrentAction()
{
	if (!Actions.IsValidIndex(ActionIndex))
	{
		return;
	}

	FSiegeAssistantProbeAction& Current = Actions[ActionIndex];

	// The typing window ends at the first non-typing action; read the box back
	// exactly once, before Enter can commit-and-clear it.
	if (Current.Tag != SiegeAssistantInputProbe::TagType && !bCapturedTypedText)
	{
		bCapturedTypedText = true;
		bInTypingWindow = false;
		if (InputBox != nullptr)
		{
			Working.TextInBoxAfterTyping = InputBox->GetText().ToString();
			Working.bTypedTextLanded = Working.TextInBoxAfterTyping.Equals(FString(SiegeAssistantInputProbe::ProbeSentence), ESearchCase::CaseSensitive);
		}
	}

	// Sampled BEFORE the injection, so it describes the state the key arrived
	// into rather than the state the key produced.
	Current.bBoxFocusedAtPress = IsInputBoxFocused();

	if (Current.bSuppressed)
	{
		// The slot is held open for timing parity, but nothing is sent. bActionHeld
		// still goes true so the phase machine advances identically.
		Current.bInjected = false;
		bActionHeld = true;
		return;
	}

	if (Current.bIsMouseButton)
	{
		if (FSlateApplication::IsInitialized())
		{
			const auto CursorPos = FSlateApplication::Get().GetCursorPos();
			Working.RightMouseCursorX = static_cast<float>(CursorPos.X);
			Working.RightMouseCursorY = static_cast<float>(CursorPos.Y);
		}
		SiegeAssistantInputProbe::InjectMouseButtonDown(Current.Key);
	}
	else
	{
		SiegeAssistantInputProbe::InjectKeyDown(Current);
	}

	Current.bInjected = true;
	bActionHeld = true;
}

void USiegeAssistantInputProbeWidget::ReleaseCurrentAction()
{
	if (!bActionHeld || !Actions.IsValidIndex(ActionIndex))
	{
		bActionHeld = false;
		return;
	}

	const FSiegeAssistantProbeAction& Current = Actions[ActionIndex];

	if (Current.bInjected)
	{
		if (Current.bIsMouseButton)
		{
			SiegeAssistantInputProbe::InjectMouseButtonUp(Current.Key);
		}
		else
		{
			SiegeAssistantInputProbe::InjectKeyUp(Current);
		}
	}

	bActionHeld = false;

	if (Current.Tag == SiegeAssistantInputProbe::TagEnter)
	{
		Working.bFocusSurvivedEnter = IsInputBoxFocused();
	}
}

void USiegeAssistantInputProbeWidget::ReleaseHeldAction()
{
	if (bActionHeld)
	{
		ReleaseCurrentAction();
	}
}

void USiegeAssistantInputProbeWidget::LeavePass()
{
	using namespace SiegeAssistantInputProbe;

	const ESiegeAssistantProbePass Pass = static_cast<ESiegeAssistantProbePass>(PassIndex);

	if (Working.bRan)
	{
		if (const APawn* ProbePawn = GetProbePawn())
		{
			Working.EndLocation = ProbePawn->GetActorLocation();
		}
		Working.NetDeltaCm = FVector::Dist(Working.StartLocation, Working.EndLocation);

		for (const FSiegeAssistantProbeAction& Action : Actions)
		{
			if (Action.Tag == TagEscape)
			{
				Working.bEscapeInjected = Action.bInjected;
				Working.bEscapeReachedPlayerInput = Action.bReachedPlayerInput;
				Working.bBoxFocusedAtEscape = Action.bBoxFocusedAtPress;
			}
			else if (Action.Tag == TagEnter)
			{
				Working.bEnterInjected = Action.bInjected;
				Working.bEnterReachedPlayerInput = Action.bReachedPlayerInput;
				Working.bBoxFocusedAtEnter = Action.bBoxFocusedAtPress;
			}
			else if (Action.Tag == TagRightMouse)
			{
				Working.bRightMouseInjected = Action.bInjected;
				Working.bRightMouseReachedPlayerInput = Action.bReachedPlayerInput;
				Working.bBoxFocusedAtRightMouse = Action.bBoxFocusedAtPress;
			}
		}

		const bool bMoved = (Working.PathLengthCm >= MoveEpsilonCm)
			|| (Working.NetDeltaCm >= MoveEpsilonCm)
			|| Working.bMoveKeyReachedPlayerInput;

		if (Pass == ESiegeAssistantProbePass::Control)
		{
			if (!Working.bViewportFocusEstablished)
			{
				// ⚠️ NOT the same failure as "the hero did not move". This says the
				// harness never had a route into the input stack, which is a fault
				// in the instrument and NOT a result about Slate or Enhanced Input.
				Working.Verdict = TEXT("CONTROL-FAILED");
				Working.VerdictNote = TEXT("keyboard focus could NOT be put on the game viewport widget, so the synthetic keys had no focus path to travel along (FSlateApplication::ProcessKeyDownEvent routes only along SlateUser::GetFocusPath) — this is a HARNESS fault, not evidence about the input stack");
			}
			else if (bMoved)
			{
				Working.Verdict = TEXT("CONTROL-OK");
				Working.VerdictNote = TEXT("the synthetic keystrokes DO reach Enhanced Input when the game viewport holds focus, so the two focused passes below are meaningful");
			}
			else
			{
				Working.Verdict = TEXT("CONTROL-FAILED");
				Working.VerdictNote = TEXT("the hero did NOT move with keyboard focus verified on the game viewport — the injection reached no game binding, so nothing below is evidence of anything");
			}
		}
		else
		{
			const bool bControlOk = GetResults().Num() > 0 && GetResults()[0].Verdict == TEXT("CONTROL-OK");
			const bool bFocusHeld = Working.FocusWindowSamples > 0
				&& (static_cast<double>(Working.FocusHeldSamples) / static_cast<double>(Working.FocusWindowSamples)) >= RequiredFocusFraction;

			if (!bControlOk)
			{
				Working.Verdict = TEXT("INCONCLUSIVE");
				Working.VerdictNote = TEXT("the CONTROL pass failed, so a zero delta here proves nothing");
			}
			else if (!bFocusHeld)
			{
				Working.Verdict = TEXT("INCONCLUSIVE");
				Working.VerdictNote = TEXT("keyboard focus was not held on InputBox for the measurement window — this pass did not test what it claims to test");
			}
			else if (!Working.bTypedTextLanded)
			{
				Working.Verdict = TEXT("INCONCLUSIVE");
				Working.VerdictNote = TEXT("the sentence did not land in the box, so nothing was actually typed");
			}
			else if (bMoved)
			{
				Working.Verdict = TEXT("FAIL");
				Working.VerdictNote = TEXT("typing moved the hero — a focused UEditableTextBox does NOT starve Enhanced Input in this posture");
			}
			else
			{
				Working.Verdict = TEXT("PASS");
				Working.VerdictNote = TEXT("typing did not move the hero and no W/A/S/D reached UPlayerInput — the focused box absorbed them");
			}
		}
	}

	// ---- the control retry ------------------------------------------------
	// The CONTROL pass IS the readiness test: it is the only thing that proves
	// the harness can move the hero at all, and the likeliest reason for an
	// early failure is a world that is not warm yet (-ExecCmds fires on frame 0;
	// PIE on L_Arena runs at ~3 FPS for ~25 s). Retrying a POSITIVE proof is
	// legitimate — but only while it is reported, which is what
	// ControlAttemptHistory is for. Nothing is hidden and nothing is retried
	// once the control has passed.
	if (Working.bRan
		&& Pass == ESiegeAssistantProbePass::Control
		&& Working.Verdict == TEXT("CONTROL-FAILED")
		&& (ControlAttempt + 1) < ProbeOptions.MaxControlAttempts
		&& (FPlatformTime::Seconds() - ProbeStartRealSeconds) < static_cast<double>(ProbeOptions.ControlBudgetSeconds))
	{
		const FString AttemptLine = FString::Printf(
			TEXT("#%d path=%.2fcm viewportFocus=%s wasd=%s;"),
			ControlAttempt + 1,
			Working.PathLengthCm,
			Working.bViewportFocusEstablished ? TEXT("yes") : TEXT("NO"),
			Working.bMoveKeyReachedPlayerInput ? TEXT("yes") : TEXT("no"));
		ControlAttemptHistory += AttemptLine;

		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Siege.Assistant.InputProbe: CONTROL attempt %d FAILED (%s) — retrying in %.1fs."),
			ControlAttempt + 1, *AttemptLine, ProbeOptions.ControlRetrySeconds);

		++ControlAttempt;
		ReleaseHeldAction();
		Phase = EProbePhase::Cooldown;
		PhaseTime = 0.f;
		SetStatusLine(FString::Printf(TEXT("TASK-411 probe — CONTROL failed, retrying (%d/%d)"), ControlAttempt + 1, ProbeOptions.MaxControlAttempts));
		return;
	}

	if (Pass == ESiegeAssistantProbePass::Control)
	{
		Working.ControlAttemptsUsed = ControlAttempt + 1;
		Working.ControlAttemptHistory = ControlAttemptHistory;
	}

	GetResults().Add(Working);

	// ⚠️ BANK-AS-YOU-GO. The row is logged the moment it exists, so a session
	// that dies later (an Escape that reaches the editor, a PIE stop, a crash)
	// still leaves this measurement in the log. TASK-413 lost an entire run to
	// rows that only existed in memory.
	UE_LOG(LogSiegeAssistant, Display,
		TEXT("Siege.Assistant.InputProbe: BANKED row %d/%d — %s | %s | path=%.2fcm net=%.2fcm wasd=%s focus=%d/%d"),
		GetResults().Num(), static_cast<int32>(ESiegeAssistantProbePass::Count),
		*Working.PassName, *Working.Verdict, Working.PathLengthCm, Working.NetDeltaCm,
		Working.bMoveKeyReachedPlayerInput ? TEXT("YES") : TEXT("no"),
		Working.FocusHeldSamples, Working.FocusWindowSamples);

	++PassIndex;
	if (PassIndex < static_cast<int32>(ESiegeAssistantProbePass::Count))
	{
		EnterPass();
		return;
	}

	GetCompleted() = true;
	EndProbe(TEXT("all passes complete"));
}

void USiegeAssistantInputProbeWidget::EndProbe(const TCHAR* ReasonLiteral)
{
	using namespace SiegeAssistantInputProbe;

	ReleaseHeldAction();

	bProbeRunning = false;
	Phase = EProbePhase::Idle;
	GetInFlight() = false;

	if (InputBox != nullptr)
	{
		InputBox->OnTextCommitted.RemoveDynamic(this, &USiegeAssistantInputProbeWidget::HandleProbeTextCommitted);
	}

	if (APlayerController* Controller = GetProbeController())
	{
		if (bCapturedRestoreState)
		{
			Controller->bShowMouseCursor = bRestoreShowMouseCursor;
			Controller->bEnableClickEvents = bRestoreEnableClickEvents;

			// Mirrors the two branches of ASiegePlayerController::
			// ApplyCursorInputState() exactly, so the match posture the probe
			// found is the match posture it leaves behind. That function will
			// re-compose it anyway on the next placement / targeting /
			// group-pick / IA_UICursor transition.
			if (bRestoreShowMouseCursor)
			{
				FInputModeGameAndUI RestoreMode;
				RestoreMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				RestoreMode.SetHideCursorDuringCapture(false);
				Controller->SetInputMode(RestoreMode);
			}
			else
			{
				Controller->SetInputMode(FInputModeGameOnly());
			}
		}
	}

	// ⚠️ GIVE THE PLAYER THEIR KEYBOARD BACK. The last pass leaves focus on the
	// probe's text box, and FInputModeGameAndUI does NOT focus the viewport
	// (SetFocusAndLocking only honours a valid WidgetToFocus). Removing the
	// widget would then leave the focus path pointing at a dead widget and the
	// session unplayable — which would read as "the probe broke the game".
	// FInputModeGameOnly focuses the viewport itself, but doing it here covers
	// both branches. Skipped on a world that is already tearing down, where
	// there is no session left to hand back.
	UWorld* ProbeWorld = GetWorld();
	const bool bWorldIsLive = (ProbeWorld != nullptr) && !ProbeWorld->bIsTearingDown;
	if (bWorldIsLive)
	{
		FocusGameViewport();
	}

	UE_LOG(LogSiegeAssistant, Display, TEXT("Siege.Assistant.InputProbe: finished — %s. Posture restored; use Siege.Assistant.InputProbeReport to re-print."), ReasonLiteral);

	if (GetResults().Num() > 0)
	{
		LogReport();
	}

	// EndProbe is reached from inside NativeTick, i.e. while Slate is walking the
	// widget tree this widget is part of. Removing it on the NEXT world tick
	// instead of here keeps the teardown entirely outside that walk.
	if (bWorldIsLive)
	{
		ProbeWorld->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]() { RemoveFromParent(); }));
	}
	else
	{
		RemoveFromParent();
	}
}

void USiegeAssistantInputProbeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	using namespace SiegeAssistantInputProbe;

	if (!bProbeRunning)
	{
		return;
	}

	APlayerController* Controller = GetProbeController();
	if (Controller == nullptr || GetProbePawn() == nullptr)
	{
		Working.bRan = false;
		Working.VerdictNote = TEXT("the controller or its pawn disappeared mid-pass");
		LeavePass();
		return;
	}

	PhaseTime += InDeltaTime;

	switch (Phase)
	{
	case EProbePhase::Arming:
	{
		// Re-assert focus every frame of the arming window: the SetInputMode
		// FReply is applied LATER on ULocalPlayer's accumulated SlateOperations,
		// so a single focus call can be quietly undone.
		if (Working.bFocusRequested)
		{
			if (InputBox != nullptr && !IsInputBoxFocused())
			{
				InputBox->SetKeyboardFocus();
			}
		}
		else if (!IsGameViewportFocused())
		{
			// The CONTROL pass. Without this the focus path can be empty, and an
			// empty focus path is exactly what made TASK-413's control fail.
			FocusGameViewport();
		}

		if (PhaseTime >= ArmSeconds)
		{
			if (const APawn* ProbePawn = GetProbePawn())
			{
				Working.StartLocation = ProbePawn->GetActorLocation();
				LastSampledLocation = Working.StartLocation;
				bHasLastSampledLocation = true;
			}

			// Recorded ONCE, at the instant typing begins, for the CONTROL pass's
			// own verdict. A control that never had viewport focus is a harness
			// fault and says so, instead of blaming the hero for standing still.
			if (static_cast<ESiegeAssistantProbePass>(PassIndex) == ESiegeAssistantProbePass::Control)
			{
				Working.bViewportFocusEstablished = IsGameViewportFocused();
				if (!Working.bViewportFocusEstablished)
				{
					UE_LOG(LogSiegeAssistant, Warning,
						TEXT("Siege.Assistant.InputProbe: CONTROL could not take keyboard focus on the game viewport widget — this attempt cannot measure anything."));
				}
			}

			Phase = EProbePhase::Acting;
			PhaseTime = 0.f;
			ActionIndex = 0;
			bInTypingWindow = true;

			SetStatusLine(FString::Printf(TEXT("TASK-411 probe — %s : typing"), *Working.PassName));
			PressCurrentAction();
		}
		break;
	}

	case EProbePhase::Acting:
	{
		SampleFrame(InDeltaTime);

		if (!Actions.IsValidIndex(ActionIndex))
		{
			Phase = EProbePhase::Settling;
			PhaseTime = 0.f;
			break;
		}

		const FSiegeAssistantProbeAction& Current = Actions[ActionIndex];
		const float HoldFor = Current.HoldSeconds;
		const float TotalFor = HoldFor + TypeGapSeconds;

		if (bActionHeld && PhaseTime >= HoldFor)
		{
			ReleaseCurrentAction();
		}

		if (PhaseTime >= TotalFor)
		{
			ReleaseHeldAction();
			++ActionIndex;
			PhaseTime = 0.f;

			if (Actions.IsValidIndex(ActionIndex))
			{
				PressCurrentAction();
			}
			else
			{
				Phase = EProbePhase::Settling;
				SetStatusLine(FString::Printf(TEXT("TASK-411 probe — %s : settling"), *Working.PassName));
			}
		}
		break;
	}

	case EProbePhase::Settling:
	{
		SampleFrame(InDeltaTime);

		if (PhaseTime >= SettleSeconds)
		{
			if (!bCapturedTypedText && InputBox != nullptr)
			{
				bCapturedTypedText = true;
				Working.TextInBoxAfterTyping = InputBox->GetText().ToString();
				Working.bTypedTextLanded = Working.TextInBoxAfterTyping.Equals(FString(ProbeSentence), ESearchCase::CaseSensitive);
			}

			Phase = EProbePhase::Finished;
			PhaseTime = 0.f;
			LeavePass();
		}
		break;
	}

	case EProbePhase::Cooldown:
	{
		// Between two CONTROL attempts. Nothing is sampled and nothing is
		// injected here — a retry window is not a measurement window.
		if (PhaseTime >= ProbeOptions.ControlRetrySeconds)
		{
			EnterPass();
		}
		break;
	}

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Console commands + the ARMING GATE.
//
// CONVENTIONS §5: dev/spike commands register via the FAutoConsoleCommand family
// in a NEW file — NEVER as a UFUNCTION(exec) on a shipped class. USiegeCheatManager
// and ASiegePlayerController stay untouched by this batch, which is what keeps
// the whole inference lane new-files-only under the M8 PARALLEL LAW.
// Namespace: Siege.Assistant.* (game lane).
//
// ⚠️ THE COMMAND ARMS THE RUN, IT DOES NOT START IT. -ExecCmds is queued once on
// frame 0 by UnrealEngine.cpp and UE 5.8 offers no delay facility for it, so a
// command that measured immediately would measure a world whose pawn, input
// stack and viewport may not exist yet. And PIE on L_Arena sits at ~3 FPS for
// ~25 s while nav builds and SK_Miner streams synchronously, which quantizes
// every 0.15 s key hold to a whole ~330 ms frame. The gate below waits for both.
// ---------------------------------------------------------------------------

namespace SiegeAssistantInputProbe
{
	/** Everything the arming ticker needs. One run at a time, by construction. */
	struct FArmState
	{
		bool bArmed = false;
		TWeakObjectPtr<UWorld> WeakWorld;

		/** True when a game world already existed at arm time, so losing it is an ABORT and not "still waiting". */
		bool bHadWorldAtArm = false;

		FSiegeAssistantProbeOptions Options;
		double ArmRealSeconds = 0.0;
		uint64 ArmFrame = 0;
		int32 ConsecutiveWarmFrames = 0;
		float LastFrameSeconds = 0.f;
		FTSTicker::FDelegateHandle TickerHandle;
	};

	static FArmState& GetArmState()
	{
		static FArmState ArmState;
		return ArmState;
	}

	/**
	 *  Stops the arming ticker. bFromTicker must be true when called from inside
	 *  the ticker itself, where returning false is what unregisters it and a
	 *  second RemoveTicker would be redundant.
	 */
	static void Disarm(bool bFromTicker)
	{
		FArmState& Arm = GetArmState();
		if (!bFromTicker && Arm.TickerHandle.IsValid())
		{
			FTSTicker::RemoveTicker(Arm.TickerHandle);
		}
		Arm.TickerHandle.Reset();
		Arm.bArmed = false;
		Arm.bHadWorldAtArm = false;
		Arm.WeakWorld.Reset();
	}

	/**
	 *  The world the probe should measure. Prefers the world the command was
	 *  issued against, because -ExecCmds can fire before that world is the only
	 *  one, but tolerates it being null on frame 0.
	 */
	static UWorld* ResolveProbeWorld(const TWeakObjectPtr<UWorld>& PreferredWorld)
	{
		if (UWorld* Candidate = PreferredWorld.Get())
		{
			if (Candidate->IsGameWorld() && !Candidate->bIsTearingDown)
			{
				return Candidate;
			}
		}

		if (GEngine != nullptr)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType != EWorldType::PIE && Context.WorldType != EWorldType::Game)
				{
					continue;
				}

				UWorld* Candidate = Context.World();
				if (Candidate != nullptr && Candidate->IsGameWorld() && !Candidate->bIsTearingDown)
				{
					return Candidate;
				}
			}
		}

		return nullptr;
	}

	/** Creates the widget and hands off to BeginProbe. */
	static void StartArmedProbe(UWorld* ProbeWorld, const FSiegeAssistantProbeOptions& Options, double WaitedSeconds, float LastFrameSeconds, bool bWarm)
	{
		APlayerController* Controller = ProbeWorld->GetFirstPlayerController();
		if (Controller == nullptr)
		{
			UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: no player controller at start time — nothing was measured."));
			return;
		}

		GetWarmUpLine() = FString::Printf(
			TEXT("waited %.1fs after arming; last frame %.1f ms; warm-frame window %s"),
			WaitedSeconds, LastFrameSeconds * 1000.f,
			bWarm ? TEXT("SATISFIED") : TEXT("⚠️ NOT SATISFIED — started on the budget expiring, treat timing-sensitive readings with suspicion"));

		if (!bWarm)
		{
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Siege.Assistant.InputProbe: STARTING WITHOUT A WARM FRAME WINDOW — the readiness budget expired with the last frame at %.1f ms. Per-key hold windows will quantize to frames."),
				LastFrameSeconds * 1000.f);
		}

		USiegeAssistantInputProbeWidget* ProbeWidget =
			CreateWidget<USiegeAssistantInputProbeWidget>(Controller, USiegeAssistantInputProbeWidget::StaticClass());

		if (ProbeWidget == nullptr)
		{
			UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: CreateWidget returned null."));
			return;
		}

		ProbeWidget->AddToViewport(1000);
		ProbeWidget->BeginProbe(Options);
	}

	/** One frame of the readiness gate. Returns false to unregister itself. */
	static bool TickArmedProbe(float InDeltaSeconds)
	{
		FArmState& Arm = GetArmState();
		if (!Arm.bArmed)
		{
			Arm.TickerHandle.Reset();
			return false;
		}

		const double WaitedSeconds = FPlatformTime::Seconds() - Arm.ArmRealSeconds;

		Arm.LastFrameSeconds = InDeltaSeconds;
		if (InDeltaSeconds <= Arm.Options.WarmFrameSeconds)
		{
			++Arm.ConsecutiveWarmFrames;
		}
		else
		{
			Arm.ConsecutiveWarmFrames = 0;
		}

		UWorld* ProbeWorld = ResolveProbeWorld(Arm.WeakWorld);

		// The world existed when the command was issued and is gone now: PIE was
		// stopped, or the map travelled. Waiting out the whole budget would leave
		// a per-frame ticker running in the editor for two minutes over a run that
		// can no longer happen.
		if (ProbeWorld == nullptr && Arm.bHadWorldAtArm)
		{
			UE_LOG(LogSiegeAssistant, Error,
				TEXT("Siege.Assistant.InputProbe: ABORTED after %.1fs — the game world went away before the run could start (PIE stopped, or the map travelled). NOTHING WAS MEASURED."),
				WaitedSeconds);
			Disarm(/*bFromTicker*/ true);
			return false;
		}

		APlayerController* Controller = (ProbeWorld != nullptr) ? ProbeWorld->GetFirstPlayerController() : nullptr;
		const UGameViewportClient* ViewportClient = (ProbeWorld != nullptr) ? ProbeWorld->GetGameViewport() : nullptr;

		const bool bHasPawn = (Controller != nullptr) && (Controller->GetPawn() != nullptr);
		const bool bHasPlayerInput = (Controller != nullptr) && (Controller->PlayerInput != nullptr);
		const bool bHasViewportWidget = (ViewportClient != nullptr) && ViewportClient->GetGameViewportWidget().IsValid();
		const bool bFramesElapsed = (GFrameCounter - Arm.ArmFrame) >= MinFramesAfterArm;

		// The HARD requirements. Without every one of these there is literally
		// nothing to measure, and no amount of waiting substitutes for them.
		const bool bHardReady = (ProbeWorld != nullptr)
			&& (Controller != nullptr)
			&& bHasPawn
			&& bHasPlayerInput
			&& bHasViewportWidget
			&& FSlateApplication::IsInitialized()
			&& bFramesElapsed;

		// The SOFT requirement: a settled frame rate. Not a correctness gate — a
		// measurement-quality one — so the budget may override it, loudly.
		const bool bWarm = (WaitedSeconds >= static_cast<double>(Arm.Options.WarmUpSeconds))
			&& (Arm.Options.bSkipWarmFrameGate || Arm.ConsecutiveWarmFrames >= Arm.Options.RequiredWarmFrames);

		// Copied BEFORE Disarm, which is entitled to clear the arm state entirely.
		const FSiegeAssistantProbeOptions StartOptions = Arm.Options;
		const float StartFrameSeconds = Arm.LastFrameSeconds;

		if (bHardReady && bWarm)
		{
			Disarm(/*bFromTicker*/ true);
			StartArmedProbe(ProbeWorld, StartOptions, WaitedSeconds, StartFrameSeconds, /*bWarm*/ true);
			return false;
		}

		if (WaitedSeconds >= static_cast<double>(Arm.Options.ReadyBudgetSeconds))
		{
			if (bHardReady)
			{
				Disarm(/*bFromTicker*/ true);
				StartArmedProbe(ProbeWorld, StartOptions, WaitedSeconds, StartFrameSeconds, /*bWarm*/ false);
				return false;
			}

			// Name the condition that failed. "It did not run" without saying why
			// is the failure mode this whole batch has been fighting.
			UE_LOG(LogSiegeAssistant, Error,
				TEXT("Siege.Assistant.InputProbe: ABORTED after %.1fs — the world never became measurable. world=%s controller=%s pawn=%s playerInput=%s viewportWidget=%s slate=%s. NOTHING WAS MEASURED."),
				WaitedSeconds,
				(ProbeWorld != nullptr) ? TEXT("yes") : TEXT("NO"),
				(Controller != nullptr) ? TEXT("yes") : TEXT("NO"),
				bHasPawn ? TEXT("yes") : TEXT("NO"),
				bHasPlayerInput ? TEXT("yes") : TEXT("NO"),
				bHasViewportWidget ? TEXT("yes") : TEXT("NO"),
				FSlateApplication::IsInitialized() ? TEXT("yes") : TEXT("NO"));

			Disarm(/*bFromTicker*/ true);
			return false;
		}

		return true;
	}

	/** Parses the console arguments. An unrecognised token is reported, never ignored. */
	static FSiegeAssistantProbeOptions ParseOptions(const TArray<FString>& Args)
	{
		FSiegeAssistantProbeOptions Parsed;

		for (const FString& Token : Args)
		{
			float FloatValue = 0.f;
			int32 IntValue = 0;

			if (FParse::Value(*Token, TEXT("warmup="), FloatValue))
			{
				Parsed.WarmUpSeconds = FMath::Clamp(FloatValue, 0.f, 600.f);
			}
			else if (FParse::Value(*Token, TEXT("budget="), FloatValue))
			{
				Parsed.ReadyBudgetSeconds = FMath::Clamp(FloatValue, 1.f, 900.f);
			}
			else if (FParse::Value(*Token, TEXT("warmframes="), IntValue))
			{
				Parsed.RequiredWarmFrames = FMath::Clamp(IntValue, 0, 600);
			}
			else if (FParse::Value(*Token, TEXT("controlattempts="), IntValue))
			{
				Parsed.MaxControlAttempts = FMath::Clamp(IntValue, 1, 10);
			}
			else if (Token.Equals(TEXT("force"), ESearchCase::IgnoreCase))
			{
				Parsed.bSkipWarmFrameGate = true;
			}
			else if (Token.Equals(TEXT("escape"), ESearchCase::IgnoreCase))
			{
				Parsed.EscapePolicy = ESiegeAssistantProbeEscapePolicy::Force;
			}
			else if (Token.Equals(TEXT("noescape"), ESearchCase::IgnoreCase))
			{
				Parsed.EscapePolicy = ESiegeAssistantProbeEscapePolicy::Never;
			}
			else
			{
				UE_LOG(LogSiegeAssistant, Warning,
					TEXT("Siege.Assistant.InputProbe: unrecognised argument '%s' — IGNORED. Valid: warmup=<s> budget=<s> warmframes=<n> controlattempts=<n> force escape noescape"),
					*Token);
			}
		}

		return Parsed;
	}
}

static void SiegeAssistantInputProbeCommand(const TArray<FString>& Args, UWorld* World)
{
	using namespace SiegeAssistantInputProbe;

	if (GetInFlight())
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("Siege.Assistant.InputProbe: a probe is already running — the second request is refused, not queued."));
		return;
	}

	FArmState& Arm = GetArmState();
	if (Arm.bArmed)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("Siege.Assistant.InputProbe: a run is already ARMED and waiting. Use Siege.Assistant.InputProbeCancel to drop it."));
		return;
	}

	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogSiegeAssistant, Error, TEXT("Siege.Assistant.InputProbe: FSlateApplication is not initialized — cannot inject keystrokes, so there is nothing to arm."));
		return;
	}

	Arm.Options = ParseOptions(Args);
	Arm.WeakWorld = World;
	Arm.bHadWorldAtArm = (World != nullptr) && World->IsGameWorld();
	Arm.ArmRealSeconds = FPlatformTime::Seconds();
	Arm.ArmFrame = GFrameCounter;
	Arm.ConsecutiveWarmFrames = 0;
	Arm.LastFrameSeconds = 0.f;
	Arm.bArmed = true;
	Arm.TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateStatic(&TickArmedProbe), 0.0f);

	// ⚠️ NOT "starting". The world is measurable only when the gate says so, and
	// saying "starting" here is how a frame-0 run gets mistaken for a real one.
	UE_LOG(LogSiegeAssistant, Display,
		TEXT("Siege.Assistant.InputProbe: ARMED. Waiting for a measurable world (controller + possessed pawn + UPlayerInput + game viewport widget), then %.1fs and %d consecutive frames at or under %.0f ms. Budget %.0fs. Escape policy: %s."),
		Arm.Options.WarmUpSeconds,
		Arm.Options.bSkipWarmFrameGate ? 0 : Arm.Options.RequiredWarmFrames,
		Arm.Options.WarmFrameSeconds * 1000.f,
		Arm.Options.ReadyBudgetSeconds,
		(Arm.Options.EscapePolicy == ESiegeAssistantProbeEscapePolicy::Force) ? TEXT("FORCE (ends PIE)")
			: (Arm.Options.EscapePolicy == ESiegeAssistantProbeEscapePolicy::Never) ? TEXT("never")
			: TEXT("auto (not injected in PIE)"));
}

static void SiegeAssistantInputProbeCancelCommand(UWorld* /*World*/)
{
	using namespace SiegeAssistantInputProbe;

	if (!GetArmState().bArmed)
	{
		UE_LOG(LogSiegeAssistant, Display, TEXT("Siege.Assistant.InputProbeCancel: nothing is armed."));
		return;
	}

	Disarm(/*bFromTicker*/ false);
	UE_LOG(LogSiegeAssistant, Display, TEXT("Siege.Assistant.InputProbeCancel: the armed run was dropped. Nothing was measured."));
}

static void SiegeAssistantInputProbeReportCommand(UWorld* /*World*/)
{
	// Reads only the file-static results, so it still works after the probe
	// widget has been torn down (and after PIE has been stopped and restarted,
	// though the rows will then be from the previous session).
	SiegeAssistantInputProbe::LogReport();
}

static FAutoConsoleCommandWithWorldAndArgs GSiegeAssistantInputProbeCommand(
	TEXT("Siege.Assistant.InputProbe"),
	TEXT("TASK-411 spike measurement #6: types 'wasd send footmen' into a focused UEditableTextBox and reports whether the hero moved. ARMS a three-pass run (control, GameAndUI+focus, UIOnly+focus) that starts once the world is measurable and warm. Args: warmup=<s> budget=<s> warmframes=<n> controlattempts=<n> force escape noescape. Escape is NOT injected in PIE (it is the editor's Stop chord) unless 'escape' is passed."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeAssistantInputProbeCommand));

static FAutoConsoleCommandWithWorld GSiegeAssistantInputProbeCancelCommand(
	TEXT("Siege.Assistant.InputProbeCancel"),
	TEXT("TASK-411: drops an armed-but-not-yet-started Siege.Assistant.InputProbe run."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SiegeAssistantInputProbeCancelCommand));

static FAutoConsoleCommandWithWorld GSiegeAssistantInputProbeReportCommand(
	TEXT("Siege.Assistant.InputProbeReport"),
	TEXT("TASK-411: re-prints the last Siege.Assistant.InputProbe comparison table."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SiegeAssistantInputProbeReportCommand));
