// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Types/SlateEnums.h"
#include "SiegeAssistantInputProbe.generated.h"

class APawn;
class APlayerController;
class UEditableTextBox;
class UPawnMovementComponent;
class UTextBlock;
class UVerticalBox;

/**
 *  SPIKE MEASUREMENT #6 (batch LLM-ASSISTANT, TASK-411) — a THROWAWAY probe
 *  that answers exactly one binary question:
 *
 *      Does a FOCUSED UEditableTextBox starve Enhanced Input of WASD?
 *
 *  THE HAZARD: the in-match console has to run over a posture where the camera
 *  is still live, i.e. FInputModeGameAndUI. GameAndUI leaves the game viewport
 *  receiving input (SetIgnoreInput(false)), so a naive console would walk the
 *  hero every time the player typed a "w". The intended fix is SetKeyboardFocus
 *  on a focusable UEditableTextBox; this file measures whether that fix
 *  actually holds in UE 5.8 on this project.
 *
 *  ACCEPTANCE CRITERION, EXACT: typing "wasd send footmen" into the focused box
 *  must not move the hero.
 *      TRUE  => Wave 1's console uses FInputModeGameAndUI, camera stays live.
 *      FALSE => the fallback is FInputModeUIOnly while the box has focus, which
 *               freezes the camera (and every game key) while typing.
 *
 *  ⚠️ THIS IS A DEV/SPIKE AFFORDANCE, NOT GDD CONTENT. It is created ONLY by
 *  the console command Siege.Assistant.InputProbe — nothing auto-spawns it, no
 *  shipped class references it, and Wave 1's B3
 *  (USiegeAssistantConsoleWidget) DELETES this .h/.cpp pair.
 *
 *  ⚖️ NET RELEVANCY TIER: this file adds no replicated property, no new
 *  replicated class, no new relevancy tier. (M8 DECLARATION DUTY, CONVENTIONS
 *  §8 — "there is nothing to declare" only counts when it is stated.)
 *
 *  ===========================================================================
 *  THE TASK-413 FIX (2026-08-02). READ THIS BEFORE CHANGING ANYTHING BELOW.
 *  ===========================================================================
 *  TASK-413's run produced ZERO usable rows, in two different environments, and
 *  the two symptoms turned out to have ONE root cause — not two:
 *
 *      the CONTROL pass called FSlateApplication::ClearKeyboardFocus().
 *
 *  FSlateApplication::ProcessKeyDownEvent (SlateApplication.cpp:5018) routes a
 *  key event ONLY along SlateUser->GetFocusPath(). With focus cleared that path
 *  is EMPTY, so:
 *    (a) the synthetic W/A/S/D reached NO widget — never SViewport, so
 *        UGameViewportClient::InputKey was never called and UPlayerInput never
 *        saw them. The hero could not move. That is the standalone -game
 *        CONTROL-FAILED with PATH length 0.00, and it is a HARNESS fault, not a
 *        result about Slate or Enhanced Input; and
 *    (b) the event then fell through to the UnhandledKeyDownEventHandler
 *        (SlateApplication.cpp:5069), which the editor binds to
 *        FMainFrameActionCallbacks::OnUnhandledKeyDownEvent — and that forwards
 *        to FPlayWorldCommands::GlobalPlayWorldActions, where StopPlaySession
 *        is bound to the Escape CHORD (DebuggerCommands.cpp:358). The probe's
 *        own Escape observation therefore ENDED THE PIE SESSION at the end of
 *        pass 1, before any row was banked.
 *
 *  ⚠️ AND THE PRIOR "-ExecCmds fires on frame 0" DIAGNOSIS WAS NOT THE CAUSE.
 *  It is a real hazard (the readiness gate below still handles it), but it does
 *  not explain a CONTROL that ran seconds after frame 0. The focus path does.
 *
 *  THE THREE FIXES, one per defect:
 *
 *  1. THE CONTROL PASS NOW FOCUSES THE GAME VIEWPORT WIDGET, which is the real
 *     shipping keyboard-focus target when no UI is up — and it is NOT implied
 *     by the input mode: FInputModeDataBase::SetFocusAndLocking
 *     (PlayerController.cpp:6313) sets focus ONLY when WidgetToFocus is valid,
 *     so GameAndUI/UIOnly with no widget focus NOTHING. (FInputModeGameOnly is
 *     the only one that focuses the viewport itself, at :6446.) The control now
 *     asserts that focus every arming frame and VERIFIES it before typing; if
 *     it cannot be established the row says so in those words instead of
 *     blaming the hero for not moving.
 *
 *  2. ESCAPE IS NEVER INJECTED IN A PIE WORLD (default policy Auto). This is
 *     not a focus-dependent hazard — a focused UEditableTextBox does NOT absorb
 *     Escape: FSlateEditableTextLayout::HandleKeyDown routes Escape to
 *     HandleEscape() (:1102), which returns FALSE with RevertTextOnEscape=false
 *     and no selection and no search text (:1448-1476), i.e. UNHANDLED. It then
 *     bubbles to SViewport, and FSceneViewport::OnKeyDown returns Unhandled
 *     whenever the viewport client does not consume the key (:1288), so it
 *     keeps bubbling to SGlobalPlayWorldActions and stops PIE. ⚠️ Escape ends
 *     PIE in EVERY pass, focused or not. In standalone -game none of that
 *     machinery exists, so the full Escape matrix IS measured there.
 *     A suppressed Escape is reported as NOT INJECTED, NEVER as "swallowed" —
 *     reporting a key that was never sent as absorbed would be exactly the
 *     confident-wrong-binary this probe exists to refuse.
 *
 *  3. THE RUN IS ARMED, NOT STARTED, BY THE CONSOLE COMMAND. -ExecCmds is
 *     queued once on frame 0 (UnrealEngine.cpp) and UE 5.8 has no delay
 *     facility for it, and PIE on L_Arena sits at ~3 FPS for ~25 s while nav
 *     builds and SK_Miner streams. So the command registers an FTSTicker that
 *     waits for a world + controller + possessed pawn + UPlayerInput + a valid
 *     game viewport widget, AND for a window of consecutive warm frames, before
 *     it creates the widget at all. On top of that the CONTROL pass is itself
 *     retried a bounded number of times: the control IS the readiness test, and
 *     a control that only passes on attempt 3 is reported as such, never hidden.
 *
 *  WHAT DID NOT CHANGE, AND MUST NOT: every guard that makes this probe refuse
 *  to answer. A failed control still stamps MODE A and MODE B INCONCLUSIVE,
 *  never PASS. Lost focus still stamps INCONCLUSIVE. Text that did not land in
 *  the box still stamps INCONCLUSIVE. A probe that returns a confident wrong
 *  binary is far worse than one that returns INCONCLUSIVE.
 *
 *  HOW IT MEASURES — and why it measures THREE passes, not two.
 *  A probe that only ran the two candidate postures could report a confident
 *  PASS for a reason that has nothing to do with Slate focus: if the synthetic
 *  keystrokes never reached the input stack at all, BOTH postures would show a
 *  zero delta and the probe would be lying. So pass 1 is a CONTROL with focus
 *  on the GAME VIEWPORT, and it MUST show movement. If it does not, every other
 *  pass is reported INCONCLUSIVE rather than PASS. Two further self-checks guard
 *  the same failure shape: focus is re-verified every sampled frame (a pass that
 *  lost focus is INCONCLUSIVE, not PASS), and the typed text is read back out
 *  of the box (if the characters never landed, nothing was "typed").
 *
 *  Keystrokes are injected through FSlateApplication::ProcessKeyDownEvent /
 *  ProcessKeyCharEvent — the SAME entry point FWindowsApplication calls after
 *  the platform message pump — so the focus path, the Slate bubble, SViewport,
 *  UGameViewportClient::InputKey and Enhanced Input are all the real shipping
 *  ones. Only the OS message pump is bypassed, and that is not what is under
 *  test. (It is also not SendInput, so it works on a locked desktop — the
 *  standing TASK-076/112 constraint.)
 */

/** Which input posture a single probe pass measures. Passes run in this order. */
enum class ESiegeAssistantProbePass : uint8
{
	/**
	 *  CONTROL — GameAndUI, keyboard focus on the GAME VIEWPORT WIDGET (the
	 *  real shipping state with no UI up). Runs FIRST so a dead injection path
	 *  is caught BEFORE any result that could be misread as a PASS, and because
	 *  it doubles as the readiness test for a cold world. This pass is EXPECTED
	 *  to move the hero, and it is retried while it does not.
	 */
	Control = 0,

	/** MODE A — FInputModeGameAndUI + SetKeyboardFocus on InputBox. The hoped-for answer. */
	GameAndUI = 1,

	/** MODE B — FInputModeUIOnly while focused. The fallback. */
	UIOnly = 2,

	Count = 3
};

/**
 *  Whether the EKeys::Escape observation may be injected.
 *
 *  ⚠️ In a PIE world Escape is the editor's StopPlaySession chord and ENDS THE
 *  SESSION from any pass, focused or not (see the file header). Auto is the
 *  only safe default.
 */
enum class ESiegeAssistantProbeEscapePolicy : uint8
{
	/** Inject Escape only OUTSIDE PIE. Default. */
	Auto = 0,

	/** Inject Escape everywhere INCLUDING PIE, accepting that PIE will end. */
	Force = 1,

	/** Never inject Escape anywhere. */
	Never = 2
};

/**
 *  Run options, parsed once from the console command's arguments and fixed for
 *  the whole run. Defaults are chosen for PIE on L_Arena, whose first ~25 s run
 *  at ~3 FPS while nav builds and SK_Miner streams synchronously.
 */
struct FSiegeAssistantProbeOptions
{
	/** Minimum wall-clock time after arming before the probe may start. */
	float WarmUpSeconds = 5.0f;

	/** Total wall-clock time the arming gate may wait before it gives up or starts anyway. */
	float ReadyBudgetSeconds = 120.0f;

	/** A frame at or under this delta counts as "warm". 0.05 s = 20 FPS. */
	float WarmFrameSeconds = 0.050f;

	/** How many CONSECUTIVE warm frames are required before starting. */
	int32 RequiredWarmFrames = 45;

	/** How many times the CONTROL pass may be re-run while it keeps failing. */
	int32 MaxControlAttempts = 4;

	/** Wall-clock budget, from BeginProbe, inside which control retries may happen. */
	float ControlBudgetSeconds = 60.0f;

	/** Pause between two control attempts. */
	float ControlRetrySeconds = 1.5f;

	ESiegeAssistantProbeEscapePolicy EscapePolicy = ESiegeAssistantProbeEscapePolicy::Auto;

	/** "force": start as soon as the hard requirements are met, skipping the warm-frame window. */
	bool bSkipWarmFrameGate = false;
};

/** One synthetic key or mouse-button press inside a probe pass. */
struct FSiegeAssistantProbeAction
{
	/** The FKey handed to the synthetic event. */
	FKey Key;

	/** Platform virtual-key code, matching what Windows would supply. */
	uint32 KeyCode = 0;

	/**
	 *  Platform character code, matching ::MapVirtualKey(VK, MAPVK_VK_TO_CHAR).
	 *  ⚠️ LOAD-BEARING: FSlateEditableTextLayout::HandleKeyDown's final branch
	 *  absorbs a key ONLY when GetCharacter() != 0. Passing 0 here would make
	 *  the probe report a FALSE FAIL.
	 */
	uint32 CharCode = 0;

	/** The character for the follow-up FCharacterEvent, or 0 to send none. */
	TCHAR Character = 0;

	/** How long the key is held down, in seconds. */
	float HoldSeconds = 0.f;

	/** True for RightMouseButton, which routes through the pointer path instead. */
	bool bIsMouseButton = false;

	/** Human-readable tag used in the report ("type", "escape", "enter", "rmb"). */
	FName Tag;

	/**
	 *  True when policy forbids sending this action at all (today: Escape in a
	 *  PIE world). The action KEEPS its slot in the timeline so the phase
	 *  structure is identical in both environments, but nothing is injected and
	 *  the report says NOT INJECTED rather than inventing an observation.
	 */
	bool bSuppressed = false;

	/** True once the key-down was actually handed to FSlateApplication. */
	bool bInjected = false;

	/** InputBox focus state sampled at the instant of the press. */
	bool bBoxFocusedAtPress = false;

	/** Set by the sampler if this key was observed pressed on UPlayerInput while held. */
	bool bReachedPlayerInput = false;
};

/** The measurement produced by one probe pass. This struct IS the report row. */
struct FSiegeAssistantProbeResult
{
	FString PassName;

	/** False when the pass could not run at all (no pawn, no controller...). */
	bool bRan = false;

	/** True when this pass asked for keyboard focus on InputBox. */
	bool bFocusRequested = false;

	/**
	 *  CONTROL ONLY: true when keyboard focus was verified to be on the game
	 *  viewport widget at the moment typing began. False means the harness had
	 *  no route into the input stack, which is a HARNESS fault and is reported
	 *  in those words — it is NOT evidence about Slate or Enhanced Input.
	 */
	bool bViewportFocusEstablished = false;

	/** CONTROL ONLY: how many attempts were spent, and what the failures looked like. */
	int32 ControlAttemptsUsed = 0;
	FString ControlAttemptHistory;

	/**
	 *  Sampled frames INSIDE THE TYPING WINDOW where InputBox (or its inner
	 *  SEditableText) held keyboard focus.
	 *
	 *  ⚠️ Scored against FocusWindowSamples, NOT SampledFrames. The synthetic
	 *  Enter later in the pass commits the text and UEditableTextBox defaults
	 *  ClearKeyboardFocusOnCommit to true, so focus is legitimately GONE for the
	 *  RMB observation and the settle window. Measuring focus over the whole
	 *  pass would score roughly 87% and report a false INCONCLUSIVE on a
	 *  perfectly good PASS.
	 */
	int32 FocusHeldSamples = 0;

	/** Sampled frames inside the typing window — the denominator for FocusHeldSamples. */
	int32 FocusWindowSamples = 0;

	/** Total sampled frames in the pass (typing window + the Escape/Enter/RMB probes + settle). */
	int32 SampledFrames = 0;

	/** Wall time and worst frame inside the pass — the quantization evidence. */
	double PassSeconds = 0.0;
	float WorstFrameSeconds = 0.f;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;

	/** |EndLocation - StartLocation| in cm. The spec's "location delta". */
	double NetDeltaCm = 0.0;

	/**
	 *  Accumulated per-frame distance in cm. Reported ALONGSIDE the net delta
	 *  because "wasd send footmen" contains both W and S and both A and D, so a
	 *  hero that walked a full square could still land near its start. Path
	 *  length cannot cancel; it is the primary movement evidence.
	 */
	double PathLengthCm = 0.0;

	/**
	 *  Path length accumulated ONLY while the sentence was being typed. Reported
	 *  next to PathLengthCm so a human can see whether anything moved AFTER the
	 *  typing window (the Escape / Enter / RMB observations bind to nothing that
	 *  moves the hero, so the two numbers should agree).
	 */
	double TypingWindowPathLengthCm = 0.0;

	float MaxSpeedCms = 0.f;

	/** Largest UPawnMovementComponent::GetLastInputVector() magnitude seen. */
	float MaxMoveInputMagnitude = 0.f;

	/** True if W, A, S or D was ever observed down on UPlayerInput (i.e. IA_Move could fire). */
	bool bMoveKeyReachedPlayerInput = false;

	/**
	 *  Shipped teardown routes: ASiegePlayerController polls these with
	 *  WasInputKeyJustPressed. Each "reached" bool is only meaningful when its
	 *  "injected" bool is true — see the NOT INJECTED wording in the report.
	 */
	bool bEscapeInjected = false;
	bool bEscapeReachedPlayerInput = false;
	bool bEnterInjected = false;
	bool bEnterReachedPlayerInput = false;
	bool bRightMouseInjected = false;
	bool bRightMouseReachedPlayerInput = false;

	/**
	 *  Whether InputBox still held focus at the instant each special key was
	 *  pressed. Escape is NOT absorbed by an editable text box (HandleEscape
	 *  returns false on a plain box), so it can bubble into the game and change
	 *  the posture BEFORE Enter and RMB are observed. Printing the focus state
	 *  per observation is what makes a contaminated row visible instead of
	 *  silently wrong.
	 */
	bool bBoxFocusedAtEscape = false;
	bool bBoxFocusedAtEnter = false;
	bool bBoxFocusedAtRightMouse = false;

	/** OS cursor position when RMB was injected — the RMB route is the pointer path, not the focus path. */
	float RightMouseCursorX = 0.f;
	float RightMouseCursorY = 0.f;

	/** InputBox contents read back immediately after the typing window. */
	FString TextInBoxAfterTyping;

	/** True when TextInBoxAfterTyping equals the probe sentence exactly. */
	bool bTypedTextLanded = false;

	/** True if the box raised OnTextCommitted with ETextCommit::OnEnter. */
	bool bEnterCommittedText = false;

	/** True if the box still held focus after the synthetic Enter (it should not — ClearKeyboardFocusOnCommit). */
	bool bFocusSurvivedEnter = false;

	/** PASS / FAIL / INCONCLUSIVE / CONTROL-OK / CONTROL-FAILED. */
	FString Verdict;

	/** One sentence saying WHY, in the probe's own words. */
	FString VerdictNote;
};

/**
 *  The probe's throwaway widget. Its tree is CODE-AUTHORED (RebuildWidget +
 *  WidgetTree->ConstructWidget) as a deliberate rehearsal of manager RULING A,
 *  including ruling A's escape hatch: every child is a BindWidgetOptional
 *  member and is constructed only when it is still null, so an asset-authored
 *  tree would win with zero C++ change.
 */
UCLASS(NotBlueprintable)
class GITCLAUDEUNREALTEST_API USiegeAssistantInputProbeWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	USiegeAssistantInputProbeWidget(const FObjectInitializer& ObjectInitializer);

	/** The sentence the acceptance criterion names, character-for-character. */
	static const TCHAR* GetProbeSentence();

	/**
	 *  Runs the three-pass sequence. Safe to call twice: the second call is
	 *  refused, not queued. Callers come from the ARMING TICKER, never straight
	 *  from the console command — see fix 3 in the file header.
	 */
	void BeginProbe(const FSiegeAssistantProbeOptions& InOptions);

	/** True while a pass sequence is in flight. */
	bool IsProbeRunning() const { return bProbeRunning; }

	/**
	 *  True when InputBox, or the SEditableText that SEditableTextBox forwards
	 *  focus to, currently holds keyboard focus. Walks the focused widget's
	 *  parent chain, because SEditableTextBox::OnFocusReceived re-targets focus
	 *  to its inner editable text — the box itself is NOT the focused widget.
	 */
	bool IsInputBoxFocused() const;

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget interface

protected:

	/**
	 *  RULING A pinned member. A future /Game/UI asset naming its box InputBox
	 *  binds here and the code-authored branch below never runs.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|AssistantProbe", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> InputBox;

	/** Optional on-screen readout so a human watching PIE can follow the passes. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|AssistantProbe", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Optional root container. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|AssistantProbe", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

private:

	/** Phases of a single pass. */
	enum class EProbePhase : uint8
	{
		Idle,
		Arming,
		Acting,
		Settling,
		/** Between two CONTROL attempts. Nothing is measured; nothing is injected. */
		Cooldown,
		Finished
	};

	/** Builds the code-authored tree. No-op when an asset-authored tree already exists. */
	void ConstructProbeTree();

	/** Applies the posture for PassIndex, snapshots the start state and fills Actions. */
	void EnterPass();

	/** Records the pass result and advances, retries the control, or finishes the sequence. */
	void LeavePass();

	/** Restores the captured input posture, removes the widget and clears any held key. */
	void EndProbe(const TCHAR* ReasonLiteral);

	/** Per-frame sampling of pawn movement and UPlayerInput key state. */
	void SampleFrame(float InDeltaTime);

	/** Presses / releases the action at ActionIndex. */
	void PressCurrentAction();
	void ReleaseCurrentAction();

	/** Releases whatever is currently held. Safe to call when nothing is. */
	void ReleaseHeldAction();

	/**
	 *  Puts keyboard focus on the game viewport widget — the shipping state with
	 *  no UI up, and the ONLY way the CONTROL pass has a route into the input
	 *  stack. Returns true when the focus is verified afterwards.
	 */
	bool FocusGameViewport();

	/** True when the focus path ends at the game viewport widget rather than inside the probe. */
	bool IsGameViewportFocused() const;

	void SetStatusLine(const FString& Line);

	APlayerController* GetProbeController() const;
	APawn* GetProbePawn() const;
	UPawnMovementComponent* GetProbeMovement() const;

	/** Bound to InputBox::OnTextCommitted so the Enter observation is real, not inferred. */
	UFUNCTION()
	void HandleProbeTextCommitted(const FText& CommittedText, ETextCommit::Type CommitMethod);

	bool bProbeRunning = false;

	EProbePhase Phase = EProbePhase::Idle;

	/** Index into the three passes. */
	int32 PassIndex = 0;

	/** Index into Actions for the current pass. */
	int32 ActionIndex = 0;

	/** True while the current action's key is held down. */
	bool bActionHeld = false;

	/** Seconds spent in the current phase / action sub-step. */
	float PhaseTime = 0.f;

	/** The synthetic presses for the current pass. */
	TArray<FSiegeAssistantProbeAction> Actions;

	/** Accumulating result for the current pass. */
	FSiegeAssistantProbeResult Working;

	/** Fixed for the whole run, parsed from the console command. */
	FSiegeAssistantProbeOptions ProbeOptions;

	/** True when the probe's world is EWorldType::PIE — the Escape hazard. */
	bool bInPIEWorld = false;

	/** Resolved once from ProbeOptions.EscapePolicy and bInPIEWorld. */
	bool bEscapeInjectionAllowed = false;

	/** Zero-based CONTROL attempt counter, and the running log of failed attempts. */
	int32 ControlAttempt = 0;
	FString ControlAttemptHistory;

	/** FPlatformTime::Seconds() at BeginProbe — the control-retry budget clock. */
	double ProbeStartRealSeconds = 0.0;

	/** Pawn location on the previous sampled frame, for the path-length integral. */
	FVector LastSampledLocation = FVector::ZeroVector;

	/** True once at least one frame has been sampled in this pass. */
	bool bHasLastSampledLocation = false;

	/** Set when the typing window has ended, so the text read-back happens exactly once. */
	bool bCapturedTypedText = false;

	/**
	 *  True from the first typed key until the first non-typing action. Focus is
	 *  only REQUIRED to be held inside this window — see FocusHeldSamples.
	 */
	bool bInTypingWindow = false;

	/** Posture captured at BeginProbe so EndProbe can put it back. */
	bool bRestoreShowMouseCursor = false;
	bool bRestoreEnableClickEvents = false;
	bool bCapturedRestoreState = false;
};
