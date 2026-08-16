// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// ⚠️ REQUIRED BY TASK-519, NOT INHERITED: GetAcceptKey() returns an FKey BY VALUE
// and the ACCEPT-KEY comparison names EKeys::Z. Both live in InputCoreTypes.h.
// Blueprint/UserWidget.h happens to pull it in transitively (FKeyEvent holds an
// FKey), but the complete-type include law says a file includes what it USES —
// a transitive include is a dependency on somebody else's include list.
#include "InputCoreTypes.h"
#include "Templates/SubclassOf.h"
#include "Types/SlateEnums.h"
#include "SiegeAssistantConsoleWidget.generated.h"

class APlayerController;
class UButton;
class UEditableTextBox;
class USiegeKeyboardLayoutSubsystem;
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
 *     click meant for the game. ⚠️ AMENDED BY TASK-519: the code-authored tree no
 *     longer builds ANY button, so InputBox is now the ONLY hit-testable widget
 *     in it. ⚠️ This is the OPPOSITE of USettingsMenuWidget's
 *     BackdropBorder, which is deliberately hit-test VISIBLE because it is
 *     modal over a menu. This one is NOT modal and sits over live gameplay;
 *     copying that choice here would eat the shipped right-mouse cancel.
 *   - Escape is NOT absorbed (RevertTextOnEscape stays false), so the shipped
 *     WasInputKeyJustPressed(EKeys::Escape) cancel routes in
 *     ASiegePlayerController still fire while the console is open. Measured
 *     from engine source at TASK-411 §3 and confirmed at the qa/TASK-411 gate.
 *     ⇒ Escape does NOT close this console.
 *     ⛔⛔ AND IT MAY NEVER BE MADE TO — CLOSED 2026-08-04 BY JONATHAN, so this
 *     is no longer an open question with a default: it is the decision
 *     (AS-§6 A-2, option (a)). ⛔ NativeOnPreviewKeyDown below MUST NOT return
 *     Handled for Escape, "harmlessly" or otherwise; A-2 names that exact
 *     mechanism as a way to break the ruling. It consumes the accept key and
 *     NOTHING else.
 *
 *  ⛔ THE FOUR CLOSE ROUTES ARE ENUMERATED IN CONVENTIONS AS-§6 RULING A-2, AND
 *  THAT LIST IS THE CONTRACT. Repeated here because the gap surfaced TWICE by
 *  being written down nowhere, and a behaviour nobody enumerated is a behaviour
 *  every later task re-decides:
 *      1. The open key pressed while open — ASiegePlayerController::
 *         OnAssistantConsolePressed is a toggle whose close half is
 *         deliberately UN-GATED (a close that can be refused can strand the
 *         cursor in GameAndUI).
 *      2. CancelPressed() with no confirm prompt up. ⚠️ ROUTE 2 IS DELIBERATELY
 *         WITHOUT A CALLER FROM 2026-08-04 (TASK-519): the Cancel button that
 *         was its only v1 caller is gone. The route SURVIVES as public API —
 *         see CancelPressed()'s comment, and do not "clean it up".
 *      3. SetConsoleEnabled(false) — the fault latch.
 *      4. Enter committed on an EMPTY (whitespace-trimmed) box — Jonathan's
 *         directive, 2026-08-03. It lives in HandleTextCommitted, NEVER in
 *         SubmitPressed; see the comment at both.
 *  ⛔ A CLOSE IS NOT A CANCEL *IN THIS WIDGET*, AND TASK-519 DID NOT CHANGE THAT.
 *  Every route above reuses CloseConsole() verbatim and none of them broadcasts
 *  a cancellation — see CloseConsole()'s comment.
 *  ⚖️ WHAT JONATHAN'S 2026-08-04 RULING 3 MOVED, STATED SO THE TWO ARE NOT
 *  CONFUSED: closing the box while a confirm prompt is up is now the PLAYER'S
 *  cancel GESTURE. The half that did NOT move is the one this clause protects —
 *  the WIDGET still broadcasts nothing new, and USiegeAssistantComponent's FSM
 *  is still the only thing that turns a close into a discard (it already did:
 *  NotifyConsoleClosed() clears a pending AwaitConfirm order). The player-facing
 *  "Cancelled" transcript line for that path is TASK-520's, on the COMPONENT.
 *
 *   - This widget NEVER calls SetInputMode. Input posture in L_Arena is owned
 *     by ASiegePlayerController::ApplyCursorInputState() (CONVENTIONS
 *     "Input-mode ownership (level-travel law)"), and a second owner is exactly
 *     how a level ends up stranded in the wrong posture. The posture this
 *     console needs — FInputModeGameAndUI — is stated in the handoff as a
 *     contract for the task that binds the open key; see §4 below.
 *   - A faulted assistant calls SetConsoleEnabled(false, Reason). That disables
 *     THIS WIDGET and nothing else. No key, no card, no command changes.
 *
 *  ⛔⛔ AND FROM 2026-08-15 THE CONVERSE IS LAW TOO, AND IT IS WRITTEN HERE
 *  BECAUSE THIS IS WHERE THE NEXT AUTHOR WOULD ADD THE THING IT FORBIDS: NOTHING
 *  IS A REQUIREMENT FOR *THE CONSOLE* EITHER. The war room ships a commander NPC
 *  and a battlefield map at the castle (CONVENTIONS WR-§5/WR-§6), and Jonathan's
 *  RULING 5 on it is verbatim: "the console still works anywhere". ⇒ ⛔ NO
 *  PROXIMITY CHECK, NO NPC REFERENCE, NO RANGE CONDITION AND NO MAP-OPEN
 *  CONDITION MAY EVER ENTER THIS FILE OR ITS OPEN PATH. The NPC is an AVATAR for
 *  the same one assistant, never a second AI and never a gatekeeper; the map is
 *  an ADVANTAGE, never a requirement. The map's ONLY channel into this class is
 *  AppendToInput() below, which writes text the player still has to send.
 *
 *  ---------------------------------------------------------------------------
 *  2b. THE CONFIRM STEP HAS NO BUTTONS. `Z` ACCEPTS; CLOSING DISCARDS.
 *  ---------------------------------------------------------------------------
 *  Jonathan's ruling 3, 2026-08-04, verbatim: "instead of it being a cancel
 *  button and an accept button, lets make it to where there is no cancel button
 *  (they just simply close the chat box), and instead of an accept button they
 *  press 'z' ('z' button for QWERTY, make it that same button location for other
 *  keyboards)". CONVENTIONS AS-§20.5 is the law; the mechanism is pinned there
 *  and is NOT re-decidable here:
 *   - NativeOnPreviewKeyDown, because preview TUNNELS DOWN the focus path from
 *     the root BEFORE the focused leaf. NativeOnKeyDown BUBBLES UP from the leaf
 *     and would never fire for a printable key SEditableText already ate. The
 *     engine-source verification is quoted at the override's declaration below.
 *   - ⛔ NOT an Enhanced Input action (rejected in AS-§20.5, with the reason).
 *   - The key is resolved POSITIONALLY through
 *     USiegeKeyboardLayoutSubsystem::GetPositionalKey(EKeys::Z) — on US-Dvorak
 *     that is EKeys::Semicolon, because the physical position QWERTY prints `Z`
 *     on yields `;` there. OpenConsole() refreshes the layout once per open;
 *     ⛔ this widget binds OnKeyboardLayoutChanged to nothing (KBD-§8).
 *   - ⛔ BUT THE PROMPT THE PLAYER READS SAYS `Z` ON EVERY LAYOUT (KBD-§8,
 *     KBD-§0 ruling 1): they are on QWERTY HARDWARE with a Dvorak SOFTWARE
 *     layout, so their keycap reads `Z` and "press ;" would be the bug.
 *     ⇒ THE LOOKUP IS FOR THE COMPARISON; THE LITERAL IS FOR THE HUMAN.
 *   - The player is TOLD, on the status line, for exactly as long as the prompt
 *     is up: "Press Z to accept, or close this box to discard". It names both
 *     halves because with both buttons gone there is no longer any control on
 *     screen that reads as "cancel", and an order that vanishes to an
 *     undocumented gesture reads as "the assistant ate my order".
 *   - ⚠️ ACCEPTED CONSEQUENCE, RECORDED NOT DISCOVERED (AS-§20.5): while a
 *     confirm prompt is up the player cannot type the letter `z` into the box.
 *     Judged acceptable because a submission during AwaitConfirm is ALREADY
 *     refused by the FSM — the box is editable but useless in that state. It is
 *     on the human feel gate, not asserted here.
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
 *      AppendToInput(FString) -> bool       — ⭐ THE WAR MAP'S ONLY CHANNEL TO THE
 *                                     ASSISTANT (TASK-561, WR-§6). Writes ONE
 *                                     literal place symbol into the box and
 *                                     focuses it. ⛔ It does NOT submit, and it
 *                                     does NOT open the console — the player
 *                                     still presses Enter himself. That single
 *                                     property is why the entire war-map feature
 *                                     spends ZERO prompt characters, adds no
 *                                     `who` shape, and changes no grammar and no
 *                                     schema.
 *
 *  OUTBOUND (the component binds these):
 *      OnConsoleSubmitted(FString)  — the player pressed Enter in the box
 *      OnConsoleConfirmed()         — Accept, and ONLY while a prompt is up.
 *                                     From 2026-08-04 its ONE live source is the
 *                                     `Z` key (NativeOnPreviewKeyDown), not a
 *                                     button.
 *      OnConsoleCancelled()         — Cancel, discard whatever is pending.
 *                                     ⛔⛔ AS OF TASK-519 THIS DELEGATE HAS **NO
 *                                     LIVE BROADCASTER**: its only v1 source was
 *                                     the Cancel button, and CancelPressed() —
 *                                     the one remaining thing that broadcasts it
 *                                     — is now deliberately uncalled. It is KEPT
 *                                     as public API (AS-§6 A-2 route 2), and the
 *                                     component's binding is kept too, but ⚠️ the
 *                                     FSM's Thinking-abort and Deferred-drop
 *                                     branches behind it are UNREACHABLE FROM
 *                                     THE CONSOLE. Recorded, not hidden; it is a
 *                                     manager call, not a thing to quietly fix
 *                                     here. See handoffs/TASK-519-programmer.md.
 *      OnConsoleOpenChanged(bool)   — the console opened or closed
 *
 *  ⛔ EVERY PLAYER-FACING SENTENCE ARRIVES FROM OUTSIDE. The model emits symbols
 *  only (§3); the reason-code template table lives on the component. The only
 *  strings authored in this file are static chrome (the input hint, the idle
 *  status line and the accept-key hint) and widget-local failure notices — never
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
	 *
	 *  ⚠️ ITS BODY IS BYTE-UNCHANGED BY TASK-519; only its CALLER moved. The v1
	 *  live caller is now NativeOnPreviewKeyDown (the `Z` key), and
	 *  HandleConfirmClicked survives for a future WBP-supplied ConfirmButton.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ConfirmPressed();

	/**
	 *  Cancel. With a prompt up it lowers the prompt and broadcasts
	 *  OnConsoleCancelled (discard; nothing partially executed). With no prompt
	 *  up it simply closes the console — the same reading as "dismiss".
	 *
	 *  ⛔⛔ DELIBERATELY UNCALLED FROM 2026-08-04, AND SAYING SO IS THE WHOLE
	 *  POINT OF THIS PARAGRAPH — exactly like ToggleConsole() below/above it. An
	 *  uncalled BlueprintCallable that does not ADMIT it is uncalled is how the
	 *  next reader concludes it is dead and deletes it.
	 *
	 *  Jonathan's ruling 3 (AS-§6 RULING A-2, route 2, amended 2026-08-04)
	 *  removed the Cancel BUTTON, which was this function's only v1 caller. The
	 *  FUNCTION survives BYTE-UNCHANGED, in behaviour and in signature, because
	 *  it is shipped BlueprintCallable public API and an ENUMERATED CLOSE ROUTE:
	 *  deleting a shipped entry point in order to remove a button is a breaking
	 *  change bought for nothing. It is now the NON-KEY discard route — for a
	 *  future /Game/UI/WBP_AssistantConsole, a gamepad path, or an accessibility
	 *  path.
	 *
	 *  ⚠️ AND THE COST IS NAMED RATHER THAN LEFT TO BE DISCOVERED: because
	 *  nothing calls this, OnConsoleCancelled has NO live broadcaster, so the
	 *  FSM's Thinking-abort and Deferred-drop branches cannot be reached from the
	 *  console at all. ⛔ That is NOT fixed here (it is the component's file and
	 *  another task's scope) — it is recorded in handoffs/TASK-519-programmer.md
	 *  for the manager, and at the component's binding site by TASK-520.
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
	 *  Raises the confirm prompt: writes the game-authored one-line summary into
	 *  the transcript and puts the accept-key hint on the status line ("Press Z
	 *  to accept, or close this box to discard"). ⚠️ TASK-519: there are no
	 *  buttons to raise any more — the hint IS the confirm step's player surface,
	 *  and it is the only thing that tells the player the key exists. The ghost
	 *  circles on the ground are the component's half of the same step
	 *  (SpawnGroupCircleDecal) and are NOT this widget's business.
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
	//~ ⭐ THE WAR-MAP INPUT-INSERT SEAM (TASK-561, CONVENTIONS WR-§6 + WR-§5).
	//~ The ENTIRE interface between the battlefield map and the assistant, and
	//~ it is one function that moves one string into one text box. ⚖️ That is
	//~ the design working, not a shortcut: a click becomes TEXT IN A BOX THE
	//~ PLAYER STILL HAS TO SEND, so nothing about the assistant's contract —
	//~ prompt, grammar, schema, `who` shapes, confirm step — changes at all.
	//~ ---------------------------------------------------------------------

	/**
	 *  Appends one literal PLACE SYMBOL to the input box and puts keyboard focus
	 *  back in it. ⛔ IT NEVER SUBMITS. The player reads what landed in the box
	 *  and presses Enter himself, exactly as if he had typed it.
	 *
	 *  @param  TextToInsert  the literal symbol, e.g. TEXT("ancient_ground_near").
	 *                        Trimmed on the way in; empty or whitespace-only is
	 *                        refused with a log and no text change.
	 *  @return true iff the box's text actually changed.
	 *
	 *  ⚠️ THE WHITESPACE RULE — A DECISION, NOT A DETAIL.
	 *
	 *  ⭐⛔ CORRECTED 2026-08-15 (TASK-581), AND THE ORIGINAL WORDING IS NAMED RATHER
	 *  THAN QUIETLY REPLACED BECAUSE IT WAS *WRONG*: this block used to read "AND
	 *  TASK-564 TESTS EACH CASE BY NAME". ⛔ TASK-564 COULD NOT. The composition was
	 *  inline here, behind a Slate-realized + open + enabled console, and InputBox is
	 *  protected with no public text getter, so the composed string was unreadable
	 *  through the shipped public API — TASK-564 proved it and reported it instead of
	 *  writing a replica test that would have asserted nothing.
	 *  ⇒ THE THREE RULES BELOW ARE NOW ComposeAppendedInput()'s CONTRACT, ⛔ NOT
	 *    THIS FUNCTION'S — this function READS the box and CALLS it — and
	 *    `Siegebound.WarMap.ComposeAppendedInputWhitespaceRule` asserts every case
	 *    headlessly (CONVENTIONS WR-§6, ruling W4-R1: a testability obligation
	 *    without a testability seam is an unfunded mandate):
	 *    1. ONE space is placed BEFORE the symbol iff the existing text is
	 *       non-empty AND does not already end in whitespace. ⇒ "…to " + "mid"
	 *       gives "…to mid ", never "…tomid" and never "…to  mid".
	 *    2. Exactly ONE trailing space always follows the symbol, so the next word
	 *       — or the next click — starts cleanly, and rule 1 then sees it. Two
	 *       clicks in a row therefore give "mid hero ", never "mid  hero".
	 *    3. ⛔ NOTHING ELSE IN THE PLAYER'S TEXT IS TOUCHED — no global whitespace
	 *       normalisation, no re-casing, no head trim, no reordering. The
	 *       half-typed sentence is his (CONVENTIONS §31: a presentation layer may
	 *       not repair its input).
	 *
	 *  ⛔⛔ DECLARED DEPARTURE (CONVENTIONS §15) — THE SPEC SAID "AT THE CARET";
	 *  THIS APPENDS AT THE END, AND ON *THIS* WIDGET THE TWO ARE THE SAME
	 *  OBSERVABLE BEHAVIOUR.
	 *  ✅ STATUS 2026-08-15 — THE DEPARTURE IS NO LONGER OPEN, AND THE HEADING ABOVE
	 *  IS KEPT ONLY AS THE RECORD OF HOW THAT WAS REACHED: qa/TASK-565.md criterion
	 *  (10)(b) RATIFIED it leg by leg against the installed source, and CONVENTIONS
	 *  WR-§6 was then AMENDED to match — ruling W4-R3, "placed at the caret" ⇒
	 *  APPENDED AT THE END. ⛔ This is settled law now; do NOT re-litigate it below.
	 *  📌 The standing lesson W4-R3 drew, worth more than this one function: SPECIFY
	 *  THE OBSERVABLE AND LEAVE THE MECHANISM TO THE IMPLEMENTER — the retired wording
	 *  named a mechanism the platform does not expose, and (a) is why.
	 *  Traced at the installed UE 5.8 source, not assumed:
	 *    (a) ⛔ THERE IS NO CARET TO READ. UEditableTextBox exposes no caret getter
	 *        and no caret setter, and MyEditableTextBlock is protected
	 *        (EditableTextBox.h:300). SEditableTextBox has GoTo() but NO
	 *        GetCursorLocation() — that getter exists only on the MULTI-LINE box
	 *        (SMultiLineEditableTextBox.h:482). Nothing public can read the caret
	 *        of a single-line UMG text box, so no implementation could honour the
	 *        line as written.
	 *    (b) THE ENGINE MOVES THE CARET TO THE END ON A PROGRAMMATIC SetText WHILE
	 *        FOCUSED: FSlateEditableTextLayout::OnBoundTextChanged runs
	 *        JumpTo(ETextLocation::EndOfDocument, ECursorAction::MoveCursor) under
	 *        `bForceBoundTextReview && Widget->HasAnyUserFocus().IsSet()
	 *         && !bWasFocusedByLastMouseDown`
	 *        (SlateEditableTextLayout.cpp:4280-4284), and SetText is precisely what
	 *        raises that flag (same file, symbol SetText).
	 *        ⚠️ THE THIRD CONJUNCT IS CARRIED HERE BY qa/TASK-565.md NIT N3 — an
	 *        earlier revision of this citation stated the condition as TWO terms and
	 *        omitted `!bWasFocusedByLastMouseDown`. ✅ THE VERDICT IS UNCHANGED, and
	 *        the reason is a lifetime, not an assumption: that flag is SET at
	 *        mouse-DOWN (symbol HandleMouseButtonDown, same file :1280) and CLEARED at
	 *        mouse-UP (symbol HandleMouseButtonUp, :1374) ON THE TEXT BOX ITSELF ⇒ it
	 *        is true only while a button is held down INSIDE this box, and false in
	 *        every window in which AppendToInput can run (a war-map marker click is a
	 *        press-and-release on a DIFFERENT widget). ⇒ route (b) is live for us.
	 *    (c) AND IT MOVES IT TO THE END AGAIN ON THE FOCUS THIS FUNCTION TAKES:
	 *        HandleFocusReceived runs GoTo(ETextLocation::EndOfDocument) for any
	 *        non-mouse focus cause when ShouldJumpCursorToEndWhenFocused() (same
	 *        file, symbol HandleFocusReceived), which reads
	 *        bIsCaretMovedWhenGainFocus — default TRUE (EditableTextBox.cpp:31),
	 *        deliberately never changed by ApplyInputBoxContract. And
	 *        UWidget::SetKeyboardFocus() focuses with EFocusCause::SetDirectly
	 *        (Widget.cpp, symbol UWidget::SetKeyboardFocus → SlateApplication.h's
	 *        default argument), which is neither Mouse nor OtherWidgetLostFocus.
	 *  ⇒ BOTH ORDERINGS CONVERGE ON END-OF-DOCUMENT: an already-focused box takes
	 *    (b), an unfocused one takes (c). A hypothetical "insert at the caret"
	 *    could place the text mid-string and would STILL leave the caret at the
	 *    end — a worse result, bought with new retained state and a delegate
	 *    subscription. ✅ THE FUNCTION IS NAMED FOR WHAT IT ACTUALLY DOES.
	 *
	 *  ⛔⛔ IT NEVER OPENS THE CONSOLE, AND THAT IS TWO MECHANISMS, NOT A TASTE:
	 *    (i)  Opening requires the INPUT POSTURE change ASiegePlayerController
	 *         owns — this widget never calls SetInputMode (class comment §2), so a
	 *         widget-initiated open would put a visible console on the wrong
	 *         posture and strand the cursor.
	 *    (ii) A write into a CLOSED console would be destroyed anyway:
	 *         OpenConsole() calls InputBox->SetText(FText::GetEmpty()) on every
	 *         open. Silently losing the player's click is the one outcome this
	 *         seam must not have.
	 *  ⇒ A CLOSED CONSOLE REFUSES, LOUDLY, AND RETURNS false. The caller
	 *    (TASK-563, on the controller) opens through the shipped posture-owning
	 *    path first, then calls this.
	 *
	 *  ⛔ NO PROXIMITY, NPC OR RANGE CONDITION MAY EVER ENTER THIS FUNCTION OR THIS
	 *  FILE — WR-§5, Jonathan's RULING 5 verbatim: "the console still works
	 *  anywhere". The map gate is TASK-563's and applies to THE MAP ONLY.
	 *
	 *  ⛔ IT KNOWS NOTHING ABOUT THE PLACE VOCABULARY, AND MUST NOT LEARN. It moves
	 *  an OPAQUE string. Which symbols exist is USiegeAssistantVocabulary's; which
	 *  of them are clickable is UWarMapWidget's (TASK-560). A validation branch
	 *  here would be a second, drifting copy of a vocabulary this file does not own.
	 *
	 *  ⛔ AND NO LENGTH CAP LIVES HERE. USiegeAssistantSnapshot::MaxUtteranceBytes
	 *  is the shipped authority on how much player text can reach a prompt, and it
	 *  is already static_asserted against the Zone C budget. A second copy inside a
	 *  widget is CONVENTIONS §19's duplicated-authority defect — a guardrail that
	 *  reports safe while the real one moves away from it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	bool AppendToInput(const FString& TextToInsert);

	/**
	 *  ⭐⭐ THE SEPARATOR COMPOSITION — THE WHITESPACE RULE AS A PURE FUNCTION, AND
	 *  THE EXTRACTION *IS* THE DELIVERABLE (TASK-581, CONVENTIONS WR-§6 ruling
	 *  W4-R1).
	 *
	 *  Given the box's CURRENT text and one ALREADY-TRIMMED symbol, returns the
	 *  text the box should hold afterwards. ⛔ It reads no member, writes no member,
	 *  touches no widget and logs nothing — a static needs NO instance, NO Slate, NO
	 *  world and NO CDO, and that is the entire point of it existing.
	 *
	 *  @param  ExistingText   the input box's current contents; may be empty.
	 *  @param  TrimmedSymbol  the literal place symbol, e.g. TEXT("ancient_ground_near").
	 *  @return the composed text — ⛔ ALWAYS with exactly one trailing space.
	 *
	 *  ⚠️ PRECONDITION — DOCUMENTED HERE RATHER THAN DEFENDED IN CODE:
	 *  TrimmedSymbol IS ALREADY TRIMMED AND NON-EMPTY. AppendToInput trims it and
	 *  refuses the empty case before this is ever reached, and that refusal is the
	 *  SINGLE authority on the question. A second guard here would be CONVENTIONS
	 *  §19's duplicated-authority defect — two answers that can drift apart.
	 *
	 *  THE THREE RULES IT OWNS, and they are the SHIPPED behaviour, ⛔ not a redesign:
	 *    1. ONE space before the symbol iff ExistingText is non-empty AND does not
	 *       already end in whitespace. ⛔ THE TEST IS ON THE LAST CHARACTER, VIA
	 *       FChar::IsWhitespace — ⛔ NOT EndsWith(" "), BECAUSE A TAB MUST COUNT.
	 *       ⛔ AND !IsEmpty() STAYS FIRST IN THE && : it is what makes the index
	 *       access safe, and reordering it is an out-of-bounds read on an empty box.
	 *    2. ⛔ ALWAYS exactly ONE trailing space. That is the IDEMPOTENCE property:
	 *       "mid " + "hero" reads the trailing space, adds no second one, and gives
	 *       "mid hero " — ⛔ never "mid  hero".
	 *    3. ⛔ NOTHING ELSE IN THE PLAYER'S TEXT IS TOUCHED — no global whitespace
	 *       normalisation, no re-casing, no head trim, no reordering (CONVENTIONS
	 *       §31: a presentation layer may not repair its input). ExistingText is a
	 *       byte-exact PREFIX of the result whenever no separator is added, and is
	 *       byte-exact up to the inserted separator when one is.
	 *
	 *  ⛔ IT IS DELIBERATELY *NOT* A UFUNCTION. It is a TEST SEAM, ⛔ not a Blueprint
	 *  API: reflecting it would put a bare string helper on the palette and invite a
	 *  second caller that bypasses AppendToInput's refusal ladder entirely.
	 *
	 *  ⛔ AND IT TAKES EXACTLY TWO PARAMETERS, ⛔ NEITHER DEFAULTED (CONVENTIONS
	 *  SC-§33). ⚠️ THE PRECISE TRAP THAT LAW EXISTS FOR HERE IS A
	 *  `bool bAddTrailingSpace = true`: rule 2 IS what makes rule 1 idempotent, so a
	 *  caller that switched it off would break rule 1 with ⛔ no compiler diagnostic
	 *  and ⛔ no test failure. A third parameter is forbidden on this signature.
	 */
	static FString ComposeAppendedInput(const FString& ExistingText, const FString& TrimmedSymbol);

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

	/**
	 *  ⭐ THE ACCEPT KEY (TASK-519, CONVENTIONS AS-§20.5). Consumes the
	 *  positionally-resolved `Z` and calls ConfirmPressed(), and does NOTHING
	 *  ELSE WHATSOEVER.
	 *
	 *  WHY *PREVIEW*, VERIFIED FROM THE INSTALLED UE 5.8 SOURCE ON THIS MACHINE
	 *  RATHER THAN REMEMBERED (there is ZERO precedent for this mechanism
	 *  anywhere in Source/ — no NativeOnKeyDown, no OnKeyChar, no FReply
	 *  override existed before this one):
	 *
	 *   1. FSlateApplication::ProcessKeyDownEvent runs the TUNNEL pass FIRST:
	 *        Reply = FEventRouter::RouteAlongFocusPath(this,
	 *                    FEventRouter::FTunnelPolicy(EventPath), InKeyEvent,
	 *                    [](const FArrangedWidget& CurrentWidget, const FKeyEvent& Event)
	 *                    { ... CurrentWidget.Widget->OnPreviewKeyDown(...) ... });
	 *      and only "Send out key down events" (the BUBBLE pass) afterwards, and
	 *      only `if (!Reply.IsEventHandled())`.
	 *      (SlateApplication.cpp:5025-5046.)
	 *   2. FTunnelPolicy starts at `WidgetIndex(0)` and increments — i.e. it walks
	 *      the focus path ROOT → LEAF (SlateApplication.cpp:347-366), so an
	 *      ANCESTOR of the focused widget is offered the key BEFORE the focused
	 *      SEditableText can turn it into the character `z`.
	 *   3. SObjectWidget forwards that pass straight into this class:
	 *        FReply SObjectWidget::OnPreviewKeyDown(...) {
	 *            if (CanRouteEvent()) { return WidgetObject->NativeOnPreviewKeyDown(...); } ... }
	 *      (SObjectWidget.cpp:221-228.)
	 *
	 *  ⇒ NativeOnKeyDown ALONE IS NOT VIABLE and that is not a preference: the
	 *  BUBBLE pass starts at the focused leaf, which has already handled a
	 *  printable key, so it never reaches us.
	 *
	 *  ⛔ SetIsFocusable(true) IS DELIBERATELY *NOT* CALLED, AND ITS ABSENCE IS
	 *  THE VERIFIED ANSWER, NOT AN OVERSIGHT. Focusability decides whether a
	 *  widget can BE the focus target; the tunnel pass walks the focus PATH, and
	 *  every ancestor of the focused box is on that path regardless. Nothing in
	 *  the three quotes above consults SupportsKeyboardFocus(). Calling it would
	 *  be a live risk for zero gain — UUserWidget's own property comment says
	 *  bIsFocusable "is only set at construction and is not modifiable at
	 *  runtime" (UserWidget.h:1030), and a focusable console competing with its
	 *  own InputBox is the one thing this widget cannot survive.
	 *
	 *  ⚠️ THE ONE REAL PRECONDITION, STATED SO IT IS NOT MISTAKEN FOR A BUG: the
	 *  tunnel only reaches us while KEYBOARD FOCUS IS SOMEWHERE INSIDE THIS
	 *  WIDGET. OpenConsole() → FocusInputBox() is what puts it there, and
	 *  ApplyInputBoxContract item 3 (ClearKeyboardFocusOnCommit = false) is what
	 *  keeps it there across a submit. If a player clicks the world and focus
	 *  leaves the box, `Z` stops accepting until the box is focused again — the
	 *  same condition that already governs typing at all.
	 *
	 *  ⛔⛔ ESCAPE IS NOT NAMED IN THE IMPLEMENTATION AND MUST NEVER BE. AS-§6 A-2
	 *  is CLOSED on "Escape is left exactly as it is", and it names
	 *  NativeOnPreviewKeyDown explicitly as a way to break that ruling.
	 */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
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

	/**
	 *  OnClicked thunk for the optional ConfirmButton binding.
	 *  ⚠️ KEPT AND STILL WIRED even though the v1 tree no longer CONSTRUCTS a
	 *  ConfirmButton: a future WBP_AssistantConsole that supplies one binds to
	 *  this with zero C++ change (ruling A(b)). The `Z` key is the v1 route.
	 *
	 *  ⛔ THE CANCEL THUNK THAT SAT HERE IS GONE, DELETED WITH ITS BUTTON
	 *  (TASK-519 / AS-§6 ruling A, amended: "the CancelButton member, its
	 *  HandleCancelClicked thunk and its CancelLabelText all go"). Do not restore
	 *  it: CancelPressed() is still callable directly, which is the whole reason
	 *  the thunk is redundant rather than missing.
	 */
	UFUNCTION()
	void HandleConfirmClicked();

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

	/**
	 *  Accept — shown only while a confirm prompt is up.
	 *
	 *  ⛔ STILL PINNED, STILL BOUND, AND FROM 2026-08-04 NO LONGER CONSTRUCTED
	 *  (TASK-519 / AS-§6 ruling A, amended). Jonathan's ruling replaced the only
	 *  WAY to accept; it did not forbid the button EXISTING. So the v1
	 *  code-authored tree builds none, and every use site below stays exactly as
	 *  it was — which is precisely ruling A(b)'s escape hatch working as written:
	 *  a future WBP_AssistantConsole may supply a child named ConfirmButton and it
	 *  binds, shows, hides and fires with ZERO C++ change.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Assistant", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

	//~ ⛔⛔ RETIRED 2026-08-04 — `CancelButton` (UButton) STOOD HERE AND IS GONE.
	//~ It is STRUCK from ruling A's pinned child list, not merely left
	//~ unconstructed, and the CONVENTIONS pin keeps the strikethrough so the next
	//~ author meets a RETIREMENT rather than an absence. This comment is the same
	//~ retirement, at the code site, for the same reason.
	//~ ⚠️ A FUTURE WBP_AssistantConsole MUST NOT ADD A CHILD NAMED `CancelButton`
	//~ EXPECTING IT TO BIND — nothing declares it, so it would bind to nothing and
	//~ still LOOK wired. There is no cancel button in this design: closing the box
	//~ IS the cancel gesture (AS-§6 A-2, amended), and CancelPressed() survives as
	//~ the non-key discard route for a WBP / gamepad / accessibility path.

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

	/**
	 *  Rewrites StatusText from LastStateLabel + bConfirmPromptVisible, so the
	 *  accept-key hint appears exactly while a prompt is up and disappears with
	 *  it — the same lifecycle the two buttons used to have. Null-safe.
	 *  ⛔ It YIELDS THE LINE while the console is disabled: SetConsoleEnabled owns
	 *  StatusText then, and a fault reason must not be overwritten by "Ready".
	 */
	void RefreshStatusLine();

	/**
	 *  Null-safe at every hop — the USettingsMenuWidget::ResolveSettingsSubsystem
	 *  shape, cloned. ⛔ Returns null freely; BOTH call sites degrade rather than
	 *  branch on trust (KBD-§5's fail-safe law).
	 */
	USiegeKeyboardLayoutSubsystem* ResolveKeyboardLayoutSubsystem() const;

	/**
	 *  The key that accepts a confirm prompt, resolved POSITIONALLY for the
	 *  ACTIVE keyboard layout: GetPositionalKey(EKeys::Z), i.e. "whatever the
	 *  physical position QWERTY prints `Z` on yields here". US-Dvorak ⇒
	 *  EKeys::Semicolon.
	 *
	 *  ⛔ NEVER EKeys::Invalid AND NEVER "no accept key": no world / no
	 *  GameInstance / no subsystem all fall back to a plain EKeys::Z, which is the
	 *  exact behaviour of a machine that never had this feature.
	 *  ⛔ ITS RESULT IS FOR THE COMPARISON ONLY. The player-facing hint says the
	 *  letter `Z` on every layout and does not call this (KBD-§8, KBD-§0 ruling 1).
	 *  ⚠️ Resolved LIVE on each press rather than cached at open: the subsystem's
	 *  1 Hz poll re-probes a mid-session Win+Space, and a cached FKey would go
	 *  stale exactly when it mattered. OpenConsole()'s RefreshKeyboardLayout()
	 *  covers the case where that poll has been switched off for testing.
	 */
	FKey GetAcceptKey() const;

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

	/**
	 *  True while a confirm prompt is up. The ONLY gate on ConfirmPressed — and
	 *  from 2026-08-04 also the ONLY state in which NativeOnPreviewKeyDown
	 *  consumes a key. ⛔ That narrowness is load-bearing, not tidiness: typing is
	 *  NOT blocked during AwaitConfirm (the box stays editable; submissions are
	 *  refused at the FSM), so a `Z` grab that ignored this flag would break
	 *  typing outright and would breach AS-§2.
	 */
	bool bConfirmPromptVisible = false;

	/** The last uint8 the FSM pushed. Stored, displayed, never interpreted. */
	uint8 AssistantState = 0;

	/**
	 *  The last label SetAssistantState pushed. Held ONLY so RefreshStatusLine
	 *  can re-compose StatusText when the confirm prompt rises or falls without
	 *  losing what the FSM last said. ⛔ Display state, never interpreted — the
	 *  same law as AssistantState above.
	 */
	FString LastStateLabel;

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
	 *  AppendToInput's OWN one-shot "there is no InputBox" latch (TASK-561).
	 *
	 *  ⚠️ IT IS SEPARATE FROM bWarnedNoInputBox ON PURPOSE, AND THE REASON IS THE
	 *  ADDITIVE LAW RATHER THAN TIDINESS: sharing the shipped latch would let a
	 *  war-map insert CONSUME it, so FocusInputBox()'s own warning — a diagnostic
	 *  that has shipped since TASK-444 — would silently never print. Suppressing an
	 *  existing log line is a behaviour change on an existing path, which this task
	 *  is forbidden to make. One bool buys byte-identical shipped behaviour.
	 */
	bool bWarnedNoInputBoxForAppend = false;

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
