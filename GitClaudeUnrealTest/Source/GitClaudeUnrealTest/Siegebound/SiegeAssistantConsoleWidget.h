// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "Types/SlateEnums.h"
#include "SiegeAssistantConsoleWidget.generated.h"

class APlayerController;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;

/** The player committed a line in the box. Carries the trimmed text; never empty. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeAssistantConsoleSubmitted, const FString&, Utterance);

/** Accept was pressed while a confirm prompt was up. Never fires without one. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeAssistantConsoleConfirmed);

/** Cancel was pressed while a confirm prompt was up: discard, execute nothing. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeAssistantConsoleCancelled);

/** The console opened or closed. The FSM decides what a close means to it. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeAssistantConsoleOpenChanged, bool, bOpen);

/**
 *  ============================================================================
 *  THE IN-MATCH ASSISTANT CONSOLE — the text box the player types orders into.
 *  Wave 1 B3 (batch SETTINGS+CONFIRM, TASK-444).
 *  ============================================================================
 *
 *  This is the surface the whole LLM-assistant lane exists to serve: one
 *  utterance in, one game-authored line back. It owns NO game logic. It parses
 *  nothing, calls no model, resolves no place, and never decides what the
 *  player reads — it renders text it is handed and reports what the player did.
 *
 *  ⚖️ NET RELEVANCY TIER: this file adds no replicated property, no new
 *  replicated class, no new relevancy tier. (M8 DECLARATION DUTY, CONVENTIONS
 *  "Settings screen …" §8 — "there is nothing to declare" only counts when it
 *  is stated.) It is client-local by construction: it is a local input surface
 *  whose only outputs are delegate broadcasts to a local component.
 *
 *  ---------------------------------------------------------------------------
 *  1. THE TREE IS CODE-AUTHORED, AND THAT IS A NAMED, SCOPED RULING
 *  ---------------------------------------------------------------------------
 *  CONVENTIONS "In-match LLM command assistant" §6 RULING A: this class builds
 *  its tree in RebuildWidget() via WidgetTree->ConstructWidget<> and ships with
 *  NO .uasset. The ruling is scoped to THIS widget; citing it for another
 *  widget is a misuse it names in as many words. (USettingsMenuWidget has its
 *  OWN, separate ruling — §3 of the settings section. Neither inherits from the
 *  other.)
 *
 *  Ruling A's five conditions are QA criteria, and all five are honoured here:
 *   (a) SCOPE — this class only.
 *   (b) ESCAPE HATCH — every child below is a UPROPERTY(BindWidgetOptional)
 *       with the exact pinned name, and the code-authored branch is skipped
 *       whole when an asset-authored tree is present. A WBP_AssistantConsole
 *       authored later therefore wins with ZERO C++ change.
 *   (c) RESERVED NAME — /Game/UI/WBP_AssistantConsole is reserved and NOT
 *       authored in v1. Taking the fallback is one art task and zero C++.
 *   (d) CONTRACT UNCHANGED — USessionMenuWidget's shipped contract, cloned:
 *       BindWidgetOptional members, BlueprintCallable wrappers, and
 *       FString/int32/bool/uint8-only BlueprintImplementableEvents. FSM state
 *       is pushed as a uint8, NEVER an enum (the widget-param law).
 *   (e) ⚠️ VERIFICATION IS A HUMAN PIXEL CHECK. There is no .uasset to read
 *       back, and MCP readback has repeatedly passed on visually-broken UMG on
 *       this project. Nothing about how this looks on screen is verified by the
 *       author, by a readback, or by a compile. It closes on Jonathan's eyes
 *       (TASK-448).
 *
 *  ⛔ NEVER BUILD THE FALLBACK WBP BY DUPLICATE+REPARENT. That has silently
 *  broken RUNTIME repaint on this project (design-time fine, ~9 wasted fixes).
 *  Build it fresh or build it inline.
 *
 *  ---------------------------------------------------------------------------
 *  2. THE CONSOLE IS NEVER A REQUIREMENT FOR ANY ACTION (§2, and it is law)
 *  ---------------------------------------------------------------------------
 *  Every keyboard command still works byte-identically with this widget open or
 *  closed. Consequences that are DESIGN, not accident, and that a later edit
 *  must not "tidy" away:
 *
 *   - The root container is SelfHitTestInvisible, so the panel never swallows a
 *     click meant for the game. Only the box and the two buttons are
 *     hit-testable. ⚠️ This is the OPPOSITE of USettingsMenuWidget's
 *     BackdropBorder, which is deliberately hit-test VISIBLE because it is
 *     modal over a menu. This one is NOT modal and sits over live gameplay;
 *     copying that choice here would eat the shipped right-mouse cancel.
 *   - Escape is NOT absorbed (RevertTextOnEscape stays false), so the shipped
 *     WasInputKeyJustPressed(EKeys::Escape) cancel routes in
 *     ASiegePlayerController still fire while the console is open. Measured
 *     from engine source at TASK-411 §3 and confirmed at the qa/TASK-411 gate.
 *     ⇒ Escape does NOT close this console. ⛔ AND IT MAY NOT BE MADE TO: that
 *     is Jonathan's open ruling (AS-§6 A-2), not an oversight to tidy up.
 *
 *  ⛔ THE FOUR CLOSE ROUTES ARE ENUMERATED IN CONVENTIONS AS-§6 RULING A-2, AND
 *  THAT LIST IS THE CONTRACT. Repeated here because the gap surfaced TWICE by
 *  being written down nowhere, and a behaviour nobody enumerated is a behaviour
 *  every later task re-decides:
 *      1. The open key pressed while open — ASiegePlayerController::
 *         OnAssistantConsolePressed is a toggle whose close half is
 *         deliberately UN-GATED (a close that can be refused can strand the
 *         cursor in GameAndUI).
 *      2. CancelPressed() with no confirm prompt up.
 *      3. SetConsoleEnabled(false) — the fault latch.
 *      4. Enter committed on an EMPTY (whitespace-trimmed) box — Jonathan's
 *         directive, 2026-08-03. It lives in HandleTextCommitted, NEVER in
 *         SubmitPressed; see the comment at both.
 *  ⛔ A CLOSE IS NOT A CANCEL. Every route above reuses CloseConsole() verbatim
 *  and none of them broadcasts a cancellation — see CloseConsole()'s comment.
 *   - This widget NEVER calls SetInputMode. Input posture in L_Arena is owned
 *     by ASiegePlayerController::ApplyCursorInputState() (CONVENTIONS
 *     "Input-mode ownership (level-travel law)"), and a second owner is exactly
 *     how a level ends up stranded in the wrong posture. The posture this
 *     console needs — FInputModeGameAndUI — is stated in the handoff as a
 *     contract for the task that binds the open key; see §4 below.
 *   - A faulted assistant calls SetConsoleEnabled(false, Reason). That disables
 *     THIS WIDGET and nothing else. No key, no card, no command changes.
 *
 *  ---------------------------------------------------------------------------
 *  3. WHY THIS WIDGET NEVER INTERPRETS THE uint8 IT IS HANDED
 *  ---------------------------------------------------------------------------
 *  The FSM (USiegeAssistantComponent, TASK-442) pushes its state as a uint8 per
 *  the widget-param law. This class STORES and DISPLAYS that number and draws
 *  no conclusion from its value — the confirm buttons are raised and lowered by
 *  the explicit ShowConfirmPrompt()/HideConfirmPrompt() calls instead.
 *
 *  That is deliberate. A widget that switched on the numeric value would be
 *  silently coupled to the ORDER of an enum it does not own, in a different
 *  file, owned by a different task — and a reordered enum would move the
 *  confirm buttons to the wrong state with nothing to catch it: no compile
 *  error, no log line, no readback difference. The semantic call sites cost one
 *  extra line at the component and remove the whole failure class.
 *
 *  ---------------------------------------------------------------------------
 *  4. THE SEAM — what drives this widget, and what this widget reports
 *  ---------------------------------------------------------------------------
 *  INBOUND (the component/controller calls these):
 *      OpenConsole / CloseConsole / ToggleConsole
 *      SetAssistantState(uint8, label)      — display only, never interpreted
 *      ShowTranscriptLine(FString)          — GAME-AUTHORED text only (§3)
 *      ShowConfirmPrompt(FString) / HideConfirmPrompt()
 *      SetConsoleEnabled(bool, reason)      — the fault latch's only effect here
 *
 *  OUTBOUND (the component binds these):
 *      OnConsoleSubmitted(FString)  — the player pressed Enter in the box
 *      OnConsoleConfirmed()         — Accept, and ONLY while a prompt is up
 *      OnConsoleCancelled()         — Cancel, discard whatever is pending
 *      OnConsoleOpenChanged(bool)   — the console opened or closed
 *
 *  ⛔ EVERY PLAYER-FACING SENTENCE ARRIVES FROM OUTSIDE. The model emits symbols
 *  only (§3); the reason-code template table lives on the component. The only
 *  strings authored in this file are static chrome (the two button labels, the
 *  hint text and the idle status line) and widget-local failure notices — never
 *  a rendering of anything a model produced.
 *
 *  ---------------------------------------------------------------------------
 *  5. WHAT THE INPUT LANE MUST SATISFY (TASK-445 + whoever binds the key)
 *  ---------------------------------------------------------------------------
 *   - IA_AssistantConsole at /Game/Input/Actions/IA_AssistantConsole, mapped in
 *     /Game/Input/IMC_Hero. ⚠️ Enter is the PROPOSED open key and may be taken;
 *     nothing in this file assumes it, and no key is hard-coded here.
 *   - SUBMIT IS ALWAYS ENTER-IN-THE-BOX, and that is Slate, not Enhanced Input:
 *     a focused editable text box absorbs Enter via HandleCarriageReturn in
 *     BOTH input modes (SlateEditableTextLayout.cpp:1092; the qa/TASK-411
 *     ruling that corrected TASK-413's contradictory row). So the open key can
 *     be Enter without fighting the box: the box is not focused when the
 *     console is closed. It stays true whatever key is chosen.
 *   - The console ships on FInputModeGameAndUI with keyboard focus on the box —
 *     ⚠️ INHERITED MEASUREMENT, spike bar #6, NOT re-derived here: a focused
 *     UEditableTextBox does NOT starve Enhanced Input of WASD. The camera stays
 *     live while typing.
 *   - ⚠️ THE POSTURE OWNER IS THE CONTROLLER. Adding the open console as a
 *     cursor/posture owner in ApplyCursorInputState() is a controller edit and
 *     is NOT part of TASK-444 (which is forbidden from touching that file).
 *     Flagged in handoffs/TASK-444-programmer.md as a gap in the wave.
 *
 *  ---------------------------------------------------------------------------
 *  6. THIS CLASS REPLACES SiegeAssistantInputProbe.{h,cpp}, WHICH IT DELETES
 *  ---------------------------------------------------------------------------
 *  The probe was throwaway by construction and its question is now answered, so
 *  it is gone rather than kept as a "useful dev tool" — a retired diagnostic
 *  left half-alive is a second input path nobody remembers. Three findings it
 *  paid for are carried here rather than lost with it, and each is commented at
 *  its site in the .cpp:
 *    (i)   the tree must be built BEFORE Super::RebuildWidget(),
 *    (ii)  a code-authored root container must be told SelfHitTestInvisible,
 *    (iii) committing clears keyboard focus unless you say otherwise.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeAssistantConsoleWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	//~ ---------------------------------------------------------------------
	//~ Outbound seam. The owning USiegeAssistantComponent binds these.
	//~ ---------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnSiegeAssistantConsoleSubmitted OnConsoleSubmitted;

	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnSiegeAssistantConsoleConfirmed OnConsoleConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnSiegeAssistantConsoleCancelled OnConsoleCancelled;

	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnSiegeAssistantConsoleOpenChanged OnConsoleOpenChanged;

	//~ ---------------------------------------------------------------------
	//~ Open / close. Never touches the input mode (see the class comment §2).
	//~ ---------------------------------------------------------------------

	/**
	 *  Shows the console and puts keyboard focus in the box. Refused (with a
	 *  log and no visible change) while the console is disabled, so a faulted
	 *  assistant cannot present a box that will never answer.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void OpenConsole();

	/**
	 *  Hides the console and hands keyboard focus back to the game viewport.
	 *  ⚠️ It does NOT broadcast OnConsoleCancelled: closing the window is not
	 *  the same statement as "discard the pending order", and inventing that
	 *  equivalence here would put an FSM decision in the widget. The FSM hears
	 *  OnConsoleOpenChanged(false) and decides.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void CloseConsole();

	/** Open when closed, close when open. The natural binding for the open key. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ToggleConsole();

	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	bool IsConsoleOpen() const { return bConsoleOpen; }

	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	bool IsConsoleEnabled() const { return bConsoleEnabled; }

	//~ ---------------------------------------------------------------------
	//~ Player entries. The WBP route wires its own controls to these; the
	//~ code-authored route reaches them through the thunks below. Both are
	//~ supported, exactly as USessionMenuWidget supports both.
	//~ ---------------------------------------------------------------------

	/**
	 *  Submits one utterance. Trims first; an empty or whitespace-only line is
	 *  refused locally and never reaches the assistant (a blank prompt would
	 *  spend a model call to learn nothing).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void SubmitPressed(const FString& UtteranceText);

	/**
	 *  Accept. ⛔ Ignored unless a confirm prompt is actually up — a stray
	 *  Accept must never be able to execute an order the player was not being
	 *  shown.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ConfirmPressed();

	/**
	 *  Cancel. With a prompt up it lowers the prompt and broadcasts
	 *  OnConsoleCancelled (discard; nothing partially executed). With no prompt
	 *  up it simply closes the console — the same button reading as "dismiss".
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void CancelPressed();

	//~ ---------------------------------------------------------------------
	//~ Inbound pushes from the FSM. All display; none decide anything.
	//~ ---------------------------------------------------------------------

	/**
	 *  Pushes the FSM state for display. ⛔ uint8, never an enum (the
	 *  widget-param law), and ⛔ never interpreted here — StateLabel is what is
	 *  shown. See the class comment §3 for why the number is not switched on.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void SetAssistantState(uint8 NewState, const FString& StateLabel);

	/** The raw uint8 last pushed. Display/diagnostic only. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	uint8 GetAssistantState() const { return AssistantState; }

	/**
	 *  Appends one GAME-AUTHORED line to the transcript (clarification,
	 *  refusal, shortfall, confirmation summary). ⛔ Never a model-produced
	 *  string: the model emits symbols only and every sentence the player reads
	 *  comes from the component's reason-code template table (§3).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ShowTranscriptLine(const FString& Line);

	/** Empties the transcript (match reset / new session). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ClearTranscript();

	/**
	 *  Raises the confirm prompt: writes the game-authored one-line summary
	 *  into the transcript and shows Accept/Cancel. The ghost circles on the
	 *  ground are the component's half of the same step (SpawnGroupCircleDecal)
	 *  and are NOT this widget's business.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ShowConfirmPrompt(const FString& SummaryLine);

	/** Lowers the confirm prompt without broadcasting anything. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void HideConfirmPrompt();

	/**
	 *  The fault latch's only effect on the UI. Disabling closes the console,
	 *  lowers any prompt, and shows DisabledReason (a game-authored string).
	 *  ⛔ It changes nothing outside this widget — no key, no card, no command.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void SetConsoleEnabled(bool bEnabled, const FString& DisabledReason);

	//~ ---------------------------------------------------------------------
	//~ BlueprintImplementableEvents — FString/int32/bool/uint8 params ONLY
	//~ (the widget-param law; the USessionMenuWidget contract, cloned). Each
	//~ fires even when the matching optional child is bound and already
	//~ updated in C++, so a WBP can add presentation without re-plumbing.
	//~ ---------------------------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Assistant")
	void OnConsoleOpenStateChanged(bool bOpen);

	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Assistant")
	void OnAssistantStateChanged(uint8 NewState, const FString& StateLabel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Assistant")
	void OnTranscriptLineShown(const FString& Line);

	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Assistant")
	void OnConfirmPromptChanged(bool bVisible, const FString& SummaryLine);

	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Assistant")
	void OnConsoleEnabledChanged(bool bEnabled, const FString& Reason);

	/**
	 *  Creates the console for OwningController and adds it to the viewport,
	 *  CLOSED. Deliberately a plain static (not a UFUNCTION): it is a C++
	 *  construction helper for the owning component, and a Blueprint that wants
	 *  one uses CreateWidget like any other widget.
	 *
	 *  ConsoleClass lets a future /Game/UI/WBP_AssistantConsole be passed with
	 *  zero change here; null falls back to this class (ruling A's v1 state).
	 *  Returns null — never crashes — if the controller or CreateWidget fails.
	 */
	static USiegeAssistantConsoleWidget* CreateAndAddToViewport(
		APlayerController* OwningController,
		TSubclassOf<USiegeAssistantConsoleWidget> ConsoleClass = nullptr,
		int32 ZOrder = 5);

protected:

	//~ Begin UUserWidget interface
	//~ ⚠️ NO NativeOnInitialized OVERRIDE, AND ITS ABSENCE IS DELIBERATE: a
	//~ code-authored tree does not exist yet at that point, so the child wiring
	//~ lives in NativeConstruct. The reason is spelled out at that call site —
	//~ read it before "restoring" the shipped USessionMenuWidget idiom here.
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/**
	 *  OnTextCommitted thunk. Acts on Enter ONLY — a focus change is not an order.
	 *  ⚠️ THIS IS ALSO WHERE CLOSE ROUTE 4 LIVES (Enter on an empty box closes the
	 *  console), and it is here PRECISELY BECAUSE this filter is the keypress:
	 *  SubmitPressed is BlueprintCallable and reachable by routes that are not the
	 *  key. See the comment at the branch, and AS-§6 RULING A-2.
	 */
	UFUNCTION()
	void HandleTextCommitted(const FText& CommittedText, ETextCommit::Type CommitMethod);

	/** OnClicked thunk for the optional ConfirmButton binding. */
	UFUNCTION()
	void HandleConfirmClicked();

	/** OnClicked thunk for the optional CancelButton binding. */
	UFUNCTION()
	void HandleCancelClicked();

	//~ ---------------------------------------------------------------------
	//~ RULING A pinned children. The names are the contract a future
	//~ WBP_AssistantConsole reads; CONVENTIONS §6 ruling A's last bullet is the
	//~ single source. Every one is OPTIONAL: nothing here hard-requires a
	//~ named widget, and every use site below is null-guarded.
	//~ ---------------------------------------------------------------------

	/** Root container. ⚠️ Code-authored, it MUST be told SelfHitTestInvisible — see the .cpp. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

	/** The rolling game-authored dialogue. Never model text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TranscriptText;

	/** One line: the FSM state label, or the reason the console is disabled. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Where the order is typed. Enter commits (Slate, in both input modes). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> InputBox;

	/** Accept — shown only while a confirm prompt is up. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

	/** Cancel — shown only while a confirm prompt is up. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;

	/**
	 *  How many transcript lines are kept on screen. Tunable, and FLAGGED FOR
	 *  JONATHAN'S FEEL PASS — the console overlays live combat, so this trades
	 *  readable history against occluded battlefield and only a playtest can
	 *  settle it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant")
	int32 MaxTranscriptLines = 6;

	/**
	 *  How long OpenConsole() refuses a re-open immediately after CLOSE ROUTE 4
	 *  (Enter-on-an-empty-box) closed the console. See CONVENTIONS "In-match LLM
	 *  command assistant" AS-§6 RULING A-2's ordering clause, and the long
	 *  comment at the guard in OpenConsole().
	 *
	 *  A human cannot press a key twice this fast; keyboard auto-repeat is the
	 *  only thing that can, and suppressing that is correct too.
	 *
	 *  ⛔ HARD-CLAMPED IN CODE (SiegeAssistantConsole::MaxReopenSuppressionSeconds)
	 *  regardless of what is configured here. A long window is a console that
	 *  REFUSES TO OPEN, and that failure must not be reachable by configuration.
	 *  Setting this to 0 disables the suppression entirely and is a clean off
	 *  switch.
	 *
	 *  ⚠️ A float is correct HERE because this is a DURATION. The TIMESTAMP it is
	 *  compared against must be a double — see LastRoute4CloseRealTimeSeconds.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant",
		meta = (ClampMin = "0.0", ClampMax = "0.25", UIMin = "0.0", UIMax = "0.25"))
	float ReopenSuppressionSeconds = 0.12f;

private:

	/**
	 *  Builds the code-authored tree. No-op when an asset-authored tree is
	 *  already present (ruling A's escape hatch) and no-op on a second rebuild
	 *  of the same instance.
	 */
	void ConstructConsoleTree();

	/**
	 *  Binds the optional named children's events and applies the input-box
	 *  contract. Called from NativeConstruct (NOT NativeOnInitialized) —
	 *  see the comment at that call site; it is a trap, not a style choice.
	 */
	void WireChildWidgets();

	/** Applies bConsoleOpen / bConsoleEnabled / bConfirmPromptVisible to the widgets. Null-safe throughout. */
	void ApplyConsoleVisualState();

	/** Rewrites TranscriptText from TranscriptLines. */
	void RefreshTranscriptText();

	/** Puts keyboard focus in the box. Logs once if there is no box to focus. */
	void FocusInputBox();

	/** Hands keyboard focus back to the game viewport. Never called during world teardown. */
	void ReleaseKeyboardFocusToGame();

	/** Behaviour-critical box settings, applied to a code-authored OR asset-authored box. See the .cpp. */
	void ApplyInputBoxContract();

	/** True while the console is showing. Starts closed. */
	bool bConsoleOpen = false;

	/** False once the assistant faults. Purely local: it disables this widget and nothing else. */
	bool bConsoleEnabled = true;

	/** True while Accept/Cancel are up. The ONLY gate on ConfirmPressed. */
	bool bConfirmPromptVisible = false;

	/** The last uint8 the FSM pushed. Stored, displayed, never interpreted. */
	uint8 AssistantState = 0;

	/** The transcript, oldest first, capped at MaxTranscriptLines. */
	TArray<FString> TranscriptLines;

	/**
	 *  True once THIS class built the tree. ⚠️ Without it, a second
	 *  RebuildWidget() on the same instance would find its own root and log
	 *  "an asset-authored tree is present", which is false — the exact NIT the
	 *  qa/TASK-411 gate raised against the probe. Fixed here rather than
	 *  inherited.
	 */
	bool bTreeWasCodeAuthored = false;

	/** One-shot log guards, so a missing child cannot spam a frame loop. */
	bool bWarnedNoInputBox = false;

	/**
	 *  FPlatformTime::Seconds() at the moment CLOSE ROUTE 4 (Enter-on-empty)
	 *  closed the console, or a NEGATIVE SENTINEL when no such close is pending.
	 *  Read and CONSUMED by the guard in OpenConsole(). Route 1/2/3 closes do NOT
	 *  set it — only route 4 can have a re-open racing the same keypress.
	 *
	 *  ⛔ THIS MUST BE A double AND MUST NEVER BE NARROWED TO A float, AND THAT IS
	 *  NOT STYLE. FWindowsPlatformTime::Seconds() adds a deliberate offset of
	 *  16777216.0 == 2^24 (WindowsPlatformTime.h:48), whose stated purpose is "add
	 *  big number to make bugs apparent where return value is being passed to
	 *  float" (:28). A float has a 24-bit mantissa, so at 2^24 its ULP is exactly
	 *  1.0 SECOND — a float here would quantize every timestamp to whole seconds
	 *  and turn a 0.12 s window into a 0-or-1 s window. Verified at the engine
	 *  source, not remembered.
	 *
	 *  ⚠️ WHY THIS CLOCK AND NOT THE WORLD'S. FPlatformTime::Seconds() is
	 *  QueryPerformanceCounter-based (WindowsPlatformTime.h:23): monotonic, never
	 *  paused, never time-dilated, never reset by level travel, and available
	 *  without a UWorld. UWorld::GetTimeSeconds() has none of those properties —
	 *  it STOPS while the game is paused, so a window armed just before a pause
	 *  would never expire and the console would refuse to open forever. That is
	 *  the exact defect this member is shaped to be incapable of.
	 */
	double LastRoute4CloseRealTimeSeconds = -1.0;
};
