// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"               // FTimerHandle BY VALUE (the deferred intent's 1 Hz poll)
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/SiegeAssistantCommand.h" // FSiegeAssistantCommand by value + LogSiegeAssistant + the reason/ask code namespaces
#include "Siegebound/TeamId.h"                // ETeamId out-param on ResolveOrderingTeam
#include "Siegebound/UnitCommand.h"           // ESiegeGroupCommandType by value on the executor's zone-order helper
#include "SiegeAssistantComponent.generated.h"

class ADecalActor;
class ASiegePlayerController;
class ASummonedUnit;
class USiegeAssistantConsoleWidget;
class USiegeAssistantSnapshot;
class USiegeAssistantVocabulary;
class USiegeLlamaSubsystem;

/**
 *  THE IN-MATCH ASSISTANT'S FSM (batch SETTINGS+CONFIRM, TASK-442 = Wave 1 B2a;
 *  CONVENTIONS "In-match LLM command assistant (v1, text-only) - 2026-08-02"
 *  §1, §2, §3, §4, §8, §9 + "Settings screen + the assistant CONFIRM STEP +
 *  the non-orderable-kind guard (2026-08-03)" §5, §7).
 *
 *  ⚖️ M8 DECLARATION DUTY (CONVENTIONS "Settings screen..." §8, stated verbatim
 *  as required): "adds no replicated property, no new replicated class, no new
 *  relevancy tier." Every member below is client-local, transient FSM state on a
 *  component that only ever acts on the authority (see §7 AUTHORITY, below);
 *  nothing here crosses the wire.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  ⛔ THE CENTRAL LAW - AND THIS CLASS IS THE ONE THAT CAN BREAK IT (§1)
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  > One utterance + one snapshot -> one constrained JSON; the game owns the
 *  > dialogue; a multi-turn model loop is a QA FAIL.
 *
 *  The model never sees a prior turn, never holds conversation state, never
 *  chains a call. Each clarification turn is a FRESH SINGLE-TURN CALL, and the
 *  context is carried forward as ONE GAME-AUTHORED LINE in Zone C
 *  (`pending: guard footman x10 -> ancient_ground_near ; problem: only 8
 *  available`). BFCL multi-turn collapses with model size (4B ~35 %) while
 *  single-shot structured output holds at ~82 %: we are building a TRANSLATOR,
 *  NOT AN AGENT.
 *
 *  ⚠️ THAT LAW IS MADE STRUCTURALLY TRUE HERE, NOT MERELY OBSERVED. Four
 *  mechanisms, each of which a later edit would have to DELETE rather than
 *  merely overlook - which is the difference between a rule and a comment:
 *
 *   1. ⛔ NO MEMBER OF THIS CLASS EVER HOLDS MODEL OUTPUT. The raw JSON is a
 *      function LOCAL in HandleModelCompletion and dies with the frame. The only
 *      thing that survives a turn is FSiegeAssistantMessageArgs, whose every
 *      field is uint8 / int32 / FName (FSiegeAssistantCommand is itself pinned
 *      to those three by CONVENTIONS §9). ⇒ There is nowhere to put a
 *      transcript, so "just append the last reply" is not a small edit, it is a
 *      new member and a new type.
 *   2. ⛔ THE FSM - NOT THE EXECUTOR, AND NOT THE MODEL CALL - COMPOSES THE
 *      PROMPT. ComposeTurnPrompt() is private and assembles Zone A + Zone B +
 *      Zone C itself; the TASK-443 seam receives a FINISHED prompt string and
 *      has no route to a zone builder. ⇒ The one function that could feed the
 *      model its own previous words cannot reach the model, and the one that
 *      reaches the model cannot compose.
 *   3. ⛔ THE PENDING LINE IS BUILT FROM TYPED SYMBOLS ONLY. BuildPendingLine()
 *      takes NO parameters and reads only PendingArgs (uint8/int32/FName) and
 *      PendingReason (an enum). There is no FString input, so model text cannot
 *      be routed into it without changing the signature.
 *   4. ⛔ ONE DISPATCH PER TURN, ENFORCED BY A COUNTER AND NOT BY DISCIPLINE.
 *      TurnId increments in BeginTurn() and nowhere else; bModelDispatchedThisTurn
 *      refuses a second dispatch inside one turn; and HandleModelCompletion
 *      DROPS any completion whose TurnId is not the current one. A "retry with
 *      the model's answer appended" would have to fake a turn id to run at all.
 *
 *  ⚠️ AND THE CHEAPEST CORRECT ANSWER IS THE ONE THAT NEVER ENTERS THE MODEL
 *  (§4 of the task spec). TryShortCircuitClarification() answers the common
 *  clarification reply - "yes" / "no" / a bare number against a KNOWN shortfall -
 *  with ZERO model calls, because "you asked for 10 and 8 can take that order"
 *  is arithmetic the game already has in hand. It is a latency AND a reliability
 *  decision, not an optimisation: a turn that never reaches the model cannot be
 *  got wrong by one.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  THE STATES (task spec §2) - AND EVERY EDGE OUT OF EACH
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *      Idle -> Composing -> Thinking -> { AwaitConfirm | Clarify | Failed }
 *      plus Deferred
 *
 *   Idle          console shut, nothing pending. NotifyConsoleOpened -> Composing.
 *   Composing     console open, waiting for a sentence. SubmitUtterance -> Thinking
 *                 (or straight back to Composing on a refusal, which never leaves
 *                 the state machine mid-turn).
 *   Thinking      exactly one model request is in flight (queue depth 1 - a second
 *                 SubmitUtterance is REFUSED, never queued). Leaves on
 *                 HandleModelCompletion, CancelPressed or NotifyConsoleClosed.
 *   AwaitConfirm  a parsed, guard-passed order is on screen with its ghost circles
 *                 (TASK-443 owns the body). ConfirmPressed executes; CancelPressed
 *                 discards with NOTHING partially executed - and so does
 *                 NotifyConsoleClosed, which is the route the player actually has
 *                 in v1 (AS-§6 A-2, amended: closing the box IS the cancel) and
 *                 which prints the SAME game-authored Cancelled line.
 *   Clarify       the game is holding a question. The pending line carries the
 *                 context; the next utterance is a FRESH single-turn call unless
 *                 the short-circuit answers it first.
 *   Failed        this TURN failed (parse / model error / timeout). It is NOT the
 *                 session latch - the next utterance starts a clean turn.
 *                 bAssistantFaulted is the latch, and it is separate on purpose.
 *   Deferred      a deferred intent is latched (TASK-443 owns the timer + TTL +
 *                 re-resolve). It SURVIVES the console being closed, because the
 *                 latch is a background promise rather than a screen.
 *
 *  ADMISSION TABLE for SubmitUtterance - pinned here so TASK-443 does not have to
 *  infer it, and so QA can check the code against a table rather than a habit:
 *
 *      Idle | Composing | Clarify | Failed  ->  a new turn starts
 *      Thinking                             ->  REFUSED, RefusedBusy (queue depth 1)
 *      AwaitConfirm                         ->  REFUSED, RefusedAwaitingConfirm
 *      Deferred                             ->  the latch is CANCELLED and a new
 *                                               turn starts (a player who types a
 *                                               fresh order means the fresh one)
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  §3 - THE MODEL EMITS SYMBOLS ONLY, SO THIS FILE IS THE ONLY PLACE PLAYER
 *  TEXT EXISTS
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  Every sentence the player reads is a GAME-AUTHORED TEMPLATE filled from a
 *  REASON CODE (ESiegeAssistantReasonCode -> SiegeAssistantReasonTemplate). There
 *  is no "let the model phrase it nicely" path and there is no second table.
 *  ⇒ TASK-443 AUTHORS NO STRINGS: the confirm summary it needs is
 *  DescribeCommandForPlayer(), and everything else is PushMessage(Code, Args).
 *  A player-facing literal appearing anywhere but SiegeAssistantReasonTemplate is
 *  a §3 violation.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  §2 / §6 / §7 - FAULT POSTURE AND AUTHORITY
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  - A model-load failure, a GGML fault, a timeout or a missing GGUF must NEVER
 *    block match start and NEVER degrade any key. bAssistantFaulted is a SESSION
 *    LATCH that disables the console AND NOTHING ELSE. This component never
 *    ticks and never blocks.
 *    ⚠️ THE THIRD CLAUSE OF THIS LINE USED TO READ "and holds no reference into
 *    the plugin", AND IT WAS FALSE (QA-464 WARN-3; swept under §22). Four call
 *    sites resolve the subsystem - BeginPlay, NotifyConsoleOpened,
 *    DispatchTurnToModel and AbortInFlightRequest. ⚖️ The CONCLUSION is unchanged
 *    and the real mechanism is stronger than the one that was claimed: the
 *    pointer is RESOLVED LIVE AND NEVER CACHED (so nothing can dangle across a
 *    travel), every call is null-tolerant, and the plugin's entry points are
 *    non-blocking - RequestCompletion returns false immediately when !IsReady().
 *    ⛔ A true conclusion resting on a dead mechanism reads as verified and is
 *    not; that is why the mechanism is spelled out rather than reasserted.
 *  - V1 is host/standalone only - FORCED, not chosen. SubmitUtterance refuses on
 *    !HasAuthority() with the SAME APPROVED WORDING as the keys
 *    (NSLOCTEXT("Siegebound", "Refused_OnlineObserver", ...), character-for-
 *    character with ASiegePlayerController's observer lockout). ⛔ The assistant
 *    must never be more capable than the keyboard.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  §4 / §5 - THE SNAPSHOT IS CAPTURED PER SENTENCE, NEVER PER TICK
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  ONE USiegeAssistantSnapshot is created once and re-Capture()d per sentence
 *  (Capture resets itself first, so no stale row can survive). ⛔ DO NOT ADD A
 *  UNIT REGISTRY, AN ACTOR CACHE, A DIRTY FLAG OR A SUBSCRIPTION LIST -
 *  CONVENTIONS §4 rejects all four ON SIGHT and this is the clause to cite.
 *
 *  ⚠️ AND THIS COMPONENT IS THE FIRST CALLER USiegeAssistantSnapshot HAS EVER
 *  HAD. That class has ZERO callers and ZERO tests today; its truncation logging
 *  has NEVER EMITTED A LINE, and every "now observable" clause in CONVENTIONS
 *  §8/§10 describes code that has never run. "Observable" is not "observed."
 *  ⇒ ReportFirstCapture() prints the first live capture IN FULL with its
 *  character counts, once per session, so TASK-447's owed first-execution audit
 *  ("Settings screen..." §7) has a real observation to quote instead of a
 *  "looks fine".
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  THE SEAM MAP - TASK-442 LEFT THESE NAMED AND EMPTY; TASK-443 HAS FILLED THEM
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  All eight bodies are now implemented IN PLACE. No name, no signature and no
 *  call site changed, and no parallel set was added:
 *
 *      DispatchTurnToModel(Prompt, Grammar)  the ONE USiegeLlamaSubsystem::
 *                                            RequestCompletion per turn. Prompt
 *                                            and grammar arrive FINISHED.
 *      RouteParsedCommand(Command)           guard -> eligibility -> deferred ->
 *                                            toggle -> AwaitConfirm | execute.
 *      EnterAwaitConfirm()                   summary line + SpawnGroupCircleDecal
 *                                            ghost circles.
 *      ClearConfirmPreview()                 tear the ghost circles down.
 *      ExecutePendingCommand()               CreateUnitGroup / EnrollInDefault-
 *                                            FollowGroup / SetUnitCommand /
 *                                            Rally - THE SAME PUBLIC APIs THE
 *                                            KEYS CALL, never a parallel one.
 *      EnterDeferredIntent()                 latch + 120 s TTL + the 1 Hz timer.
 *      ClearDeferredIntent()                 drop the latch and stop the timer.
 *      AbortInFlightRequest()                USiegeLlamaSubsystem::CancelActiveRequest.
 *
 *  ⚠️ EVERYTHING ELSE IN THIS FILE IS TASK-442's AND FINISHED, AND THE THINGS
 *  THAT LOOK LIKE THEY BELONG TO THE EXECUTOR ARE DELIBERATELY NOT ITS: the
 *  parse, the selection invariant, the ask-code routing, the shortfall
 *  arithmetic, the reason codes, the template table and the pending line are ALL
 *  the FSM's, because "FSM owns the conversation" is the third of §1's three
 *  owners.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  ⛔ THE TOGGLE REMOVES A HUMAN REVIEW STEP AND NEVER A MACHINE CHECK
 *  ("Settings screen..." §5 - THE LAW THAT MAKES THE TOGGLE SAFE TO SHIP)
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  bAssistantConfirmBeforeExecute is read LIVE, at confirm time, in exactly ONE
 *  place: IsConfirmBeforeExecuteEnabled(), called from exactly ONE place:
 *  RouteParsedCommandInternal's LAST step. ⚠️ THAT PLACEMENT IS THE WHOLE
 *  ARGUMENT, AND IT IS STRUCTURAL RATHER THAN A PROMISE - every machine check in
 *  the feature lies STRICTLY BEFORE that branch, so the branch cannot skip one
 *  even if a later editor wanted it to:
 *
 *      SubmitUtterance gates 1-8      empty / fault latch / AUTHORITY /
 *                                     admission / short-circuit / BeginTurn /
 *                                     snapshot / compose          ── before
 *      HandleModelCompletion          stale-turn guard / state guard / success
 *                                     guard / ParseSiegeAssistantCommand /
 *                                     SiegeAssistantValidateSelection (⛔ CALLED
 *                                     WITH Command.ExcludeKinds - the 4th
 *                                     parameter is DEFAULTED, so omitting it
 *                                     compiles and checks nothing) /
 *                                     FindShortfall               ── before
 *      RouteParsedCommandInternal (1) ValidateCommandAgainstSnapshot - THE
 *                                     NON-ORDERABLE-KIND GUARD    ── before
 *      RouteParsedCommandInternal (2) eligibility (a zone verb needs a place
 *                                     that RESOLVES)              ── before
 *      RouteParsedCommandInternal (3) the deferred-trigger fork   ── before
 *      RouteParsedCommandInternal (4) >>> THE TOGGLE IS READ HERE, AND NOWHERE
 *                                     ELSE IN THE FEATURE <<<
 *      ExecutePendingCommand          HasAuthority re-check / ResolvePlace /
 *                                     the SHIPPED eligibility predicates /
 *                                     the ExcludeKinds subtraction + its
 *                                     empty-after-exclusion refusal /
 *                                     the never-truncate-silently rule
 *                                                                 ── AFTER
 *
 *  ⇒ ON runs 1-4 then EnterAwaitConfirm; OFF runs 1-4 then ExecuteAndReport.
 *  ⛔ THE ONLY DIFFERENCE IS WHETHER A HUMAN LOOKS. ⚠️ AND IT FAILS SAFE: an
 *  unresolvable USiegeSettingsSubsystem returns TRUE (confirm ON), never false -
 *  failing safe means MORE review, never less.
 *  ⚠️ bForceConfirmReview (the deferred-fire path) can only ever force the
 *  toggle branch TOWARDS review. There is no argument, flag or setting anywhere
 *  in this class that can force it the other way.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  THE CONSOLE-WIDGET WIRING - A FORWARD, NOT A TRANSLATION
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  ⚠️ THIS FILE DELIBERATELY DOES NOT INCLUDE SiegeAssistantConsoleWidget.h AND
 *  HOLDS NO WIDGET POINTER. The FSM must not own a widget's lifetime, and a
 *  component that compiles against a UUserWidget is a component that cannot be
 *  driven by anything else (a console command, a test, a later voice path). The
 *  push surface is therefore three delegates, and TASK-444's widget declares an
 *  inbound API whose signatures MATCH THEM EXACTLY - so whoever wires the two
 *  writes forwards, never adapters:
 *
 *      component (out)                     widget (in)
 *      ─────────────────────────────────   ────────────────────────────────────
 *      OnAssistantStateChanged(u8, FStr) -> SetAssistantState(uint8, const FString&)
 *      OnAssistantMessage(FStr)          -> ShowTranscriptLine(const FString&)
 *      OnAssistantAvailabilityChanged    -> SetConsoleEnabled(bool, const FString&)
 *          (bool, FStr)
 *      OnAssistantConfirmPromptShown     -> ShowConfirmPrompt(const FString&)
 *          (const FStr&)                    [TASK-443: EnterAwaitConfirm broadcasts]
 *      OnAssistantConfirmPromptHidden    -> HideConfirmPrompt()
 *          ()                               [TASK-443: ClearConfirmPreview broadcasts]
 *
 *      widget (out)                        component (in)
 *      ─────────────────────────────────   ────────────────────────────────────
 *      OnConsoleSubmitted(const FString&)-> SubmitUtterance(const FString&)
 *      OnConsoleConfirmed()              -> ConfirmPressed()
 *      OnConsoleCancelled()              -> CancelPressed()
 *          ⛔ BOUND, CORRECT, AND WITHOUT A LIVE CALLER IN v1 - the Cancel button
 *          is gone (AS-§6 A-2, amended). Kept as public API; see CancelPressed().
 *      OnConsoleOpenChanged(bool)        -> NotifyConsoleOpened / NotifyConsoleClosed
 *          ⭐ THE CLOSE HALF IS NOW THE PLAYER'S CANCEL GESTURE, and it is still
 *          the FSM - not the widget - that decides so. The widget says only "the
 *          window closed"; NotifyConsoleClosed turns that into a discard AND
 *          prints the Cancelled line when an order was actually pending.
 *
 *  ⚠️ SEED BEFORE YOU BIND (the standing qa/TASK-005 major-2 lesson): push
 *  GetStateAsByte() + GetStateLabel(), GetLastMessage(), IsConsoleAvailable()
 *  AND GetPendingConfirmSummary() into the widget FIRST, then bind. A widget
 *  created at a state nobody has broadcast since is a widget that is silently
 *  wrong until the next event.
 *
 *  ⛔ AND SEEDING IS NOT OPTIONAL FOR THE CONFIRM PROMPT, BECAUSE THE WIDGET IS
 *  CREATED LAZILY AT FIRST OPEN (TASK-449). The FSM can ALREADY be sitting in
 *  AwaitConfirm when the widget is constructed - a DEFERRED intent fires on a
 *  1 Hz timer that keeps running while the console is shut, re-resolves, and
 *  enters AwaitConfirm with nothing on screen. A binder that only listens will
 *  therefore MISS the broadcast that already happened, and the player will be
 *  shown a console with no prompt and no buttons for an order that is genuinely
 *  pending. GetPendingConfirmSummary() returns exactly that missed line (empty
 *  when nothing is pending), so seeding it costs one call and closes the hole.
 *
 *  🚩 FLAGGED BY TASK-442, STILL OPEN AT TASK-443: this component deliberately
 *  holds NO widget pointer and includes NO widget header (see above), so it
 *  CANNOT bind these forwards itself - binding requires the complete widget type
 *  and an instance. TASK-440 makes the subobject, TASK-442 exposed the surface,
 *  TASK-444 built the widget, TASK-449 owns creation/visibility/posture, and
 *  TASK-443 (this) owns the FSM and the executor. ⚠️ THE JOIN ITSELF IS STILL
 *  UNOWNED and is a `SiegePlayerController` edit, which this task is forbidden.
 *  The table above exists so whoever takes it has nothing left to decide.
 */

/**
 *  The FSM's states (task spec §2).
 *
 *  ⚠️ PUSHED TO THE WIDGET AS A uint8, NEVER AS AN ENUM (the widget-param law,
 *  CONVENTIONS "Widgets with C++ bases" + §6 ruling A condition (d)). That is
 *  what FOnAssistantStateChanged and GetStateAsByte() are for; nothing outside
 *  this component should need the enum, and no BlueprintImplementableEvent may
 *  take it.
 */
UENUM()
enum class ESiegeAssistantState : uint8
{
	/** Console shut, nothing pending, no request in flight. */
	Idle = 0,

	/** Console open, waiting for a sentence. */
	Composing = 1,

	/** Exactly one model request in flight. Queue depth 1: a second sentence is refused, never queued. */
	Thinking = 2,

	/** A parsed, guard-passed order is awaiting the player's Accept / Cancel (TASK-443 owns the body). */
	AwaitConfirm = 3,

	/** The game is holding a question; the pending line carries the context into the NEXT fresh single-turn call. */
	Clarify = 4,

	/** THIS TURN failed. ⚠️ NOT the session latch - see bAssistantFaulted, which is separate on purpose. */
	Failed = 5,

	/** A deferred intent is latched (TASK-443 owns the timer). Survives the console being closed. */
	Deferred = 6
};

/**
 *  ⛔ THE REASON CODES - AND THE ONLY KEYS THE PLAYER-FACING TEMPLATE TABLE HAS
 *  (§3). The model emits SYMBOLS; the game emits SENTENCES; a reason code is the
 *  join between them. Adding a player-visible outcome means adding a code here
 *  AND a template in SiegeAssistantReasonTemplate - never a literal at a call
 *  site.
 *
 *  ⚠️ THERE IS DELIBERATELY NO CODE FOR "THAT KIND CANNOT TAKE ORDERS."
 *  CONVENTIONS "Settings screen..." §6 is explicit that the non-orderable-kind
 *  guard "REUSES THE EXISTING UNSUPPORTED-ASK OUTCOME AND INVENTS NO NEW
 *  PLAYER-FACING SURFACE" - so TASK-443's guard rejection routes to
 *  AskUnsupported, and the shortfall path below deliberately declines to answer
 *  the zero case so the two can never disagree in front of the player.
 */
UENUM()
enum class ESiegeAssistantReasonCode : uint8
{
	/** No outcome. Never printed. */
	None = 0,

	//~ ── the five model-emitted clarification codes (SiegeAssistantAsk) ──

	/** {"ask":"which_unit"} - no unit named, or an ambiguous one. */
	AskWhichUnit = 1,

	/** {"ask":"which_place"} - no place named, or an ambiguous one. */
	AskWhichPlace = 2,

	/** {"ask":"how_many"} - units named with no quantity. */
	AskHowMany = 3,

	/** {"ask":"which_intent"} - the verb sat between two orders. */
	AskWhichIntent = 4,

	/**
	 *  {"ask":"unsupported"} - not an order the assistant can express.
	 *  ⚠️ ALSO the destination of TASK-443's non-orderable-kind guard rejection
	 *  and of a refused execution (§6 - one surface, not three).
	 */
	AskUnsupported = 5,

	//~ ── game-side, no model call (the short-circuit) ──

	/**
	 *  The player asked for more than can take that order. Filled from Requested /
	 *  Available / Kind, ⚠️ AND FROM THE COMMAND'S OWN INTENT (TASK-465): this row
	 *  serves Send, Guard AND Ambush, so its verb is the {Intent} argument and
	 *  never a literal. A hard-coded verb here tells a player who typed "guard"
	 *  that the game misheard them.
	 */
	ShortfallCount = 6,

	//~ ── refusals ──

	/** !HasAuthority(). ⚠️ The wording is the KEYS' approved observer lockout, character-for-character (§7). */
	RefusedNoAuthority = 7,

	/** Faulted session latch, an unresolvable subsystem, or a dispatch that refused. */
	RefusedAssistantUnavailable = 8,

	/** A request is already in flight. Queue depth 1 is the design, not a bug. */
	RefusedBusy = 9,

	/** A parsed order is on screen awaiting Accept / Cancel. */
	RefusedAwaitingConfirm = 10,

	//~ ── this turn failed ──

	/** The model's output did not parse, or violated the selection invariant. */
	FailedParse = 11,

	/** The request failed (backend error, GGML fault, cancelled mid-flight). */
	FailedModelError = 12,

	/** The request exceeded its hard timeout. */
	FailedTimeout = 13,

	//~ ── outcomes TASK-443 needs a TEMPLATE for and must NOT author itself ──

	/** The AwaitConfirm one-line summary. Filled from DescribeCommandForPlayer - ⛔ never a model-produced string. */
	ConfirmPrompt = 14,

	/** Cancel / discard. NOTHING is left partially executed. */
	Cancelled = 15,

	/** The order was executed. */
	Executed = 16,

	/** A deferred intent was latched. Filled from Requested / Kind + the order description. */
	DeferredArmed = 17,

	/** A latched deferred intent hit its TTL without firing. */
	DeferredExpired = 18,

	/** A latched deferred intent was dropped because the player typed a new order. */
	DeferredCancelled = 19
};

/**
 *  What the SHORT-CIRCUIT made of a clarification reply. ⚠️ Unrecognized is the
 *  SAFE default and it is the common case by design: anything the game is not
 *  certain of falls through to a FRESH SINGLE-TURN MODEL CALL rather than being
 *  guessed at locally. A short-circuit that guesses is a second, worse parser.
 */
enum class ESiegeAssistantClarifyReply : uint8
{
	/** Not confidently one of the three below ⇒ hand it to the model as a fresh turn. */
	Unrecognized = 0,

	/** "yes" / "ok" / "sure" / "do it" - accept what the game offered. */
	Affirmative = 1,

	/** "no" / "cancel" / "forget it" - drop the pending intent. */
	Negative = 2,

	/** A BARE number ("8"), inside the grammar's 1..GrammarCountMax range. */
	Quantity = 3
};

/**
 *  THE SHORT-CIRCUIT'S CLASSIFIER - a PURE FREE FUNCTION over one string, for
 *  exactly the reason ValidateCommandAgainstSnapshot takes the roster array and
 *  USiegeAssistantGrammar::Build takes TArray<FName>: no UObject, no UWorld, no
 *  snapshot, no model ⇒ it is testable in isolation the moment anyone wants to.
 *
 *  ⚠️ IT IS INTENTIONALLY TINY AND INTENTIONALLY TIMID. It recognises a closed
 *  list of bare English affirmatives / negatives and a bare integer, and nothing
 *  else - not "send 8", not "yes but the archers", not a sentence with a verb in
 *  it. Every one of those returns false and costs one honest model call.
 *  ⛔ Widening it into a phrase parser rebuilds, badly, the thing the model is
 *  there to do (§2's "never a parallel implementation").
 *
 *  @param Reply       the player's raw reply, untrimmed
 *  @param OutKind     the classification; Unrecognized whenever false is returned
 *  @param OutQuantity the parsed integer for Quantity; 0 otherwise
 *  @return            true only for a CONFIDENT classification
 */
bool SiegeAssistantParseClarificationReply(const FString& Reply, ESiegeAssistantClarifyReply& OutKind, int32& OutQuantity);

/**
 *  THE PLAYER-FACING TEMPLATE TABLE (§3) - the ONE place a sentence the player
 *  reads is authored. Free function so it is reachable without an instance and
 *  so a test can walk every code.
 *
 *  ⚠️ EVERY FText IS BUILT INSIDE THE FUNCTION, NEVER AT MODULE STATIC-INIT:
 *  localization may not be up when a translation unit's statics run (the
 *  ASiegePlayerController::GetObserverLockoutText precedent, which is a
 *  function-local static for exactly this reason).
 *
 *  Format arguments are NAMED ({Requested}, {Available}, {Kind}, {Order},
 *  {Intent}) so a template can reorder them without touching a call site.
 *
 *  ⛔ A WORD THAT IS TRUE OF ONLY SOME OF A ROW'S CALLERS IS AN ARGUMENT, NEVER A
 *  LITERAL (TASK-465). Before writing a verb, a place, a count or a unit kind
 *  into a row, check every path that reaches that reason code: {Intent} exists
 *  because ShortfallCount serves three different orders and said "Send" to all
 *  three. ⚠️ The repair is one parameterised string, NOT one row per caller -
 *  three sentences drift, one cannot.
 */
const FText& SiegeAssistantReasonTemplate(ESiegeAssistantReasonCode Code);

/**
 *  The short player-facing label for a state ("Thinking...", "Confirm?").
 *
 *  ⚠️ A LABEL IS PLAYER TEXT, so it lives here beside the template table and not
 *  in a log helper (§3). ⛔ It is NOT a semantic channel: TASK-444's widget
 *  deliberately DISPLAYS the state byte and draws no conclusion from it, so
 *  nothing may key behaviour off this string.
 */
const FText& SiegeAssistantStateLabel(ESiegeAssistantState InState);

/**
 *  FSM state changed. ⚠️ uint8, NEVER the enum - the widget-param law. Fires only
 *  on an ACTUAL change (the delegate law); seed from GetStateAsByte() +
 *  GetStateLabel() FIRST, then bind.
 *
 *  ⚠️ THE SIGNATURE IS `(uint8, const FString&)` TO MATCH
 *  USiegeAssistantConsoleWidget::SetAssistantState(uint8, const FString&)
 *  CHARACTER-FOR-CHARACTER, so the wiring is a forward and never a translation.
 *  A translation layer is where two tasks' assumptions get to disagree quietly.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssistantStateChanged, uint8, NewState, const FString&, StateLabel);

/**
 *  One GAME-AUTHORED player-facing line. Matches
 *  USiegeAssistantConsoleWidget::ShowTranscriptLine(const FString&).
 *
 *  ⚠️ This is an EVENT, not a value, so it fires on every message including a
 *  repeat of the same text - the delegate law's "never on a no-op write" clause
 *  governs VALUES, and two identical refusals are two events the player must
 *  see. Seed from GetLastMessage() for the transcript's initial contents.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssistantMessage, const FString&, Message);

/**
 *  The console's availability changed - in v1 that means exactly one thing: the
 *  session fault latch closed. Matches
 *  USiegeAssistantConsoleWidget::SetConsoleEnabled(bool, const FString&).
 *
 *  ⚠️ THIS IS THE ENTIRE BLAST RADIUS OF A FAULT (§2). It disables the console
 *  and NOTHING ELSE - no key, no card, no stance, no group. A consumer that
 *  reacts to this by touching anything but the console has misread the law.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssistantAvailabilityChanged, bool, bAvailable, const FString&, Reason);

/**
 *  ⚠️ THE CONFIRM STEP IS UP, AND THIS IS THE ONLY CHANNEL THAT SAYS SO
 *  (TASK-443; "Settings screen..." §5). Carries the GAME-AUTHORED one-line
 *  summary of the parsed order - DescribeCommandForPlayer's output, ⛔ NEVER a
 *  model-produced string (§3).
 *
 *  ⚠️ IT IS A SEPARATE CHANNEL FROM OnAssistantMessage ON PURPOSE, AND NOT
 *  REDUNDANT WITH IT. The transcript line is HISTORY - it scrolls away. The
 *  confirm prompt is a LIVE MODAL STATE that raises the Accept / Cancel buttons,
 *  and TASK-444's widget gates ConfirmPressed() on it precisely so Accept can
 *  never execute an order the player is not being shown. Folding the two would
 *  make "is a prompt up?" a string comparison against the transcript.
 *
 *  Matches USiegeAssistantConsoleWidget::ShowConfirmPrompt(const FString&)
 *  character-for-character, so the wiring is a forward and never a translation.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssistantConfirmPromptShown, const FString&, SummaryLine);

/**
 *  The confirm step came down - accepted, cancelled, superseded, faulted, or the
 *  component going away. ⚠️ FIRED FROM ClearConfirmPreview(), which every one of
 *  those paths already calls unconditionally, so there is exactly ONE place a
 *  prompt can be taken down and no path can leave a phantom one on screen.
 *
 *  Matches USiegeAssistantConsoleWidget::HideConfirmPrompt() - zero parameters,
 *  so the forward is a direct AddDynamic with no adapter.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssistantConfirmPromptHidden);

/**
 *  Everything a template can be filled from.
 *
 *  ⛔ EVERY FIELD IS uint8 / int32 / FName, AND THERE IS NO FString AND NO FText
 *  FIELD - DELIBERATELY, AND IT IS MECHANISM #1 OF THE FOUR ON THE COMPONENT
 *  ABOVE. FSiegeAssistantCommand is itself pinned to those three types
 *  (CONVENTIONS §9), so THERE IS NO SLOT IN WHICH A MODEL-PRODUCED STRING COULD
 *  TRAVEL to a player-facing sentence. A future "just pass the model's wording
 *  through" is not an edit to a call site; it is a new field, in a struct whose
 *  comment forbids it.
 *
 *  ⚠️ The one model-derived thing that DOES reach the player is a CANONICAL
 *  SYMBOL (`footman`, `ancient_ground_near`) - and the grammar guarantees those
 *  came from the live roster / the fixed place vocabulary, never from free text.
 *  That is "a template filled from a reason code", exactly as §3 describes it.
 */
USTRUCT()
struct FSiegeAssistantMessageArgs
{
	GENERATED_BODY()

	/** The order under discussion, when there is one. Typed symbols only. */
	UPROPERTY()
	FSiegeAssistantCommand Command;

	/** What the player asked for (ShortfallCount, DeferredArmed). */
	UPROPERTY()
	int32 Requested = 0;

	/** What can actually take the order right now (ShortfallCount). */
	UPROPERTY()
	int32 Available = 0;

	/** The kind the message is about. NAME_None when the message is not kind-specific. */
	UPROPERTY()
	FName Kind = NAME_None;
};

/**
 *  The FSM / executor component (CONVENTIONS §5: `USiegeAssistantComponent`,
 *  `SiegeAssistantComponent.h/.cpp`). Lives on ASiegePlayerController as a
 *  default subobject created by TASK-440 - ⚠️ THIS FILE NEVER CREATES ITSELF AND
 *  NEVER EDITS THE CONTROLLER.
 *
 *  ⚠️ PINNED LINK, DESIGNED AND NOT A DEFECT: this pair compiles against the
 *  CONVENTIONS §9 / "Settings screen..." §8 registries, several of whose symbols
 *  land in sibling tasks of the same batch. It CANNOT COMPILE ALONE and is not
 *  expected to - the module compiles as ONE UBT unit at TASK-447 (the
 *  TASK-395/396 and TASK-416/417 precedent). Do not "fix" that by declaring a
 *  second log category or by re-deriving a sibling's function locally.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API USiegeAssistantComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USiegeAssistantComponent();

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent interface

	// ─────────────────────────────────────────────────────────────────────────
	// PUSH SURFACE (the console widget binds here - TASK-444 owns the widget)
	// ─────────────────────────────────────────────────────────────────────────

	/** FSM state changed. Push is a uint8 + a label, never an enum. Seed-then-bind: read GetStateAsByte() + GetStateLabel() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnAssistantStateChanged OnAssistantStateChanged;

	/** One game-authored player-facing line. Seed-then-bind: read GetLastMessage() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnAssistantMessage OnAssistantMessage;

	/** The console's availability changed (v1: the fault latch closed). Seed-then-bind: read IsConsoleAvailable() first. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnAssistantAvailabilityChanged OnAssistantAvailabilityChanged;

	/** The confirm step went UP, carrying its game-authored summary. ⛔ Seed-then-bind: read GetPendingConfirmSummary() FIRST - a lazily-created widget can be born after this fired. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnAssistantConfirmPromptShown OnAssistantConfirmPromptShown;

	/** The confirm step came DOWN. Fired from ClearConfirmPreview(), the one teardown every path already calls. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Assistant")
	FOnAssistantConfirmPromptHidden OnAssistantConfirmPromptHidden;

	/** The FSM state as the byte the widget consumes. ⚠️ The ONLY state accessor a widget may use. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	uint8 GetStateAsByte() const { return static_cast<uint8>(State); }

	/** The current state's short player-facing label, for seeding before binding. ⛔ Display only - never a semantic channel. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	FString GetStateLabel() const { return SiegeAssistantStateLabel(State).ToString(); }

	/** C++-side state accessor. Not Blueprint-exposed (the widget-param law forbids an enum crossing that boundary). */
	ESiegeAssistantState GetState() const { return State; }

	/** The last game-authored line, for seeding a transcript before binding. Empty until something is said. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	FString GetLastMessage() const { return LastMessage; }

	/**
	 *  ⚠️ THE SESSION LATCH (§2). True once a model-load failure, a GGML fault, a
	 *  timeout or a missing GGUF has been reported. It disables the console AND
	 *  NOTHING ELSE: every keyboard command keeps working byte-identically, and
	 *  match start is never blocked. It never clears within a session.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	bool IsAssistantFaulted() const { return bAssistantFaulted; }

	/** Whether the console should accept input at all. False while faulted. TASK-444's widget disables itself on false. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	bool IsConsoleAvailable() const { return !bAssistantFaulted; }

	/**
	 *  ⚠️ THE SEED FOR THE CONFIRM PROMPT (TASK-443) - the game-authored one-line
	 *  summary of the order currently awaiting Accept / Cancel, or EMPTY when no
	 *  confirm step is up.
	 *
	 *  ⛔ THIS EXISTS BECAUSE THE WIDGET IS CREATED LAZILY AT FIRST OPEN AND THE
	 *  FSM DOES NOT WAIT FOR IT. A deferred intent's 1 Hz timer keeps running
	 *  while the console is shut; when it fires it re-resolves and enters
	 *  AwaitConfirm, broadcasting OnAssistantConfirmPromptShown to whoever was
	 *  listening - which, before the first open, is NOBODY. A binder that only
	 *  subscribes would then show a console with no prompt and no Accept button
	 *  for an order that is genuinely pending, with no error and no log: the
	 *  silent-failure shape this project keeps paying for. Seed from this getter
	 *  BEFORE binding and that case simply cannot arise.
	 *
	 *  ⛔ NEVER A MODEL-PRODUCED STRING - it is DescribeCommandForPlayer's output,
	 *  assembled from a struct pinned to uint8 / int32 / FName (§3).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Assistant")
	FString GetPendingConfirmSummary() const;

	// ─────────────────────────────────────────────────────────────────────────
	// THE CONSOLE-WIDGET JOIN (TASK-443 implements it; ⛔ TASK-453 CALLS IT)
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  ⛔ THE ONE ENTRY POINT THAT JOINS THIS FSM TO A CONSOLE WIDGET. It SEEDS
	 *  the widget from the live FSM state and THEN binds all eight forwards -
	 *  four inbound (widget -> component) and four outbound (component ->
	 *  widget). TASK-444's header already declared its four outbound delegates
	 *  under "the owning USiegeAssistantComponent binds these", so this is the
	 *  shipped design intent; what was missing was only the TRIGGER, and only the
	 *  controller knows when a lazily-created widget appears.
	 *
	 *  ⛔ THE CALL SITE IS TASK-453's (`SiegePlayerController`), NEVER THIS
	 *  FILE'S. This component never creates a widget, never shows one, never
	 *  closes one and never owns one's lifetime - the pointer it keeps is WEAK
	 *  for exactly that reason.
	 *
	 *  ⚠️ IT MUST BE CALLED AT OPEN TIME, NOT AT BeginPlay, AND THE DIFFERENCE IS
	 *  A SILENT-FAILURE CLASS RATHER THAN A STYLE POINT. The widget is created
	 *  LAZILY on the first SUCCESSFUL open, so at BeginPlay there is nothing to
	 *  attach: a binder that ran there would bind to null and no-op FOREVER -
	 *  no error, no log, no ensure, no crash - and would surface only at a
	 *  playtest as a console that does nothing. This function is therefore built
	 *  to be called REPEATEDLY, at ANY point in the widget's life.
	 *
	 *  CONTRACT:
	 *   - NULL-SAFE: a null argument detaches nothing and does nothing but log.
	 *   - IDEMPOTENT: re-attaching the SAME widget re-seeds it and never
	 *     double-binds (every binding is removed before it is made, so calling
	 *     this ten times leaves exactly one of each).
	 *   - RE-TARGETABLE: attaching a DIFFERENT widget cleanly detaches the old
	 *     one first, so a widget rebuilt mid-match cannot leave a ghost listener.
	 *   - SEED-THEN-BIND (qa/TASK-005 major-2): the widget is pushed the current
	 *     state, availability, last line AND any pending confirm prompt BEFORE
	 *     any delegate is attached, so a widget born mid-conversation is correct
	 *     on its first frame rather than on its next event.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void AttachConsoleWidget(USiegeAssistantConsoleWidget* InWidget);

	/** Drop every binding to the attached console widget, both directions. ⚠️ Safe when nothing is attached; called by AttachConsoleWidget and by EndPlay. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void DetachConsoleWidget();

	/** The attached console widget, or null. ⚠️ WEAK BY DESIGN - the FSM must never own a widget's lifetime. */
	USiegeAssistantConsoleWidget* GetAttachedConsoleWidget() const;

	// ─────────────────────────────────────────────────────────────────────────
	// CONSOLE LIFECYCLE
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  The console was opened. Idle -> Composing; every other state is preserved
	 *  (a latched Deferred intent must survive a console that was shut and
	 *  reopened - the latch is a promise, not a screen).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void NotifyConsoleOpened();

	/**
	 *  The console was closed. An in-flight request is aborted, an unconfirmed
	 *  order is discarded with its preview, and the FSM returns to Idle -
	 *  ⚠️ EXCEPT from Deferred, which is preserved for the reason above.
	 *
	 *  ⭐ AND FROM AwaitConfirm IT ALSO PRINTS - the game-authored Cancelled
	 *  template, the same one CancelPressed() uses (AS-§6 RULING A-2, amended
	 *  2026-08-04). Closing the box IS the player's cancel gesture now that the
	 *  Cancel button is gone, and this component is the ONLY thing that turns a
	 *  close into a discard - the widget still broadcasts nothing but "the window
	 *  closed". ⛔ The line is pushed on the AwaitConfirm path ONLY, AFTER the
	 *  discard is complete: a close with nothing pending says nothing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void NotifyConsoleClosed();

	// ─────────────────────────────────────────────────────────────────────────
	// THE ONE ENTRY POINT FOR A TYPED SENTENCE
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  ONE UTTERANCE -> ONE SNAPSHOT -> ONE CONSTRAINED JSON (§1). The order of
	 *  the gates is pinned, and it is the order QA reads:
	 *
	 *      1. empty input              (ignored, with ZERO side effects - FIRST on
	 *                                   purpose: Enter on an empty box must not
	 *                                   print, must not spend a turn, and must not
	 *                                   reach the Deferred admission and drop a
	 *                                   latch the player never asked to drop)
	 *      2. the fault latch          (§2 - a faulted session refuses everything)
	 *      3. AUTHORITY                (§7 - the keys' approved wording, verbatim)
	 *      4. admission                (the table on the class comment)
	 *      5. THE SHORT-CIRCUIT        (may finish the turn with ZERO model calls)
	 *      6. BeginTurn                (the ONLY place TurnId moves)
	 *      7. the snapshot             (once per SENTENCE, never per tick)
	 *      8. compose prompt + grammar (the FSM's, never the executor's)
	 *      9. dispatch                 (the TASK-443 seam - one call, queue depth 1)
	 *
	 *  ⚠️ NONE OF 1-8 IS SKIPPABLE BY THE CONFIRM TOGGLE. "Settings screen..." §5:
	 *  the toggle removes a HUMAN REVIEW STEP and NEVER A MACHINE CHECK, and the
	 *  tell is that every check that runs with it ON runs with it OFF. The toggle
	 *  is read in TASK-443's RouteParsedCommand and nowhere near this function.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void SubmitUtterance(const FString& RawUtterance);

	/** Accept, in AwaitConfirm. Ignored in every other state. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void ConfirmPressed();

	/**
	 *  Cancel / discard, from any state that has something to drop. ⚠️ It NEVER
	 *  leaves anything partially executed: an in-flight request is aborted, an
	 *  unconfirmed order and its ghost circles are discarded, a latched deferred
	 *  intent is dropped, and the FSM returns to the console's resting state.
	 *
	 *  ⛔ DELIBERATELY UNCALLED IN v1, EXACTLY LIKE ToggleConsole() - AND THAT IS
	 *  RULED, NOT ROTTEN (AS-§6 RULING A-2, route 2, amended 2026-08-04). The
	 *  console's Cancel button is gone, so nothing in the shipped tree broadcasts
	 *  OnConsoleCancelled any more; ⛔ the signature and behaviour are UNCHANGED
	 *  because deleting shipped BlueprintCallable public API to remove a button is
	 *  a breaking change bought for nothing. It remains the non-key discard route
	 *  for Blueprint, a future WBP_AssistantConsole, a gamepad or an
	 *  accessibility path.
	 *  ⚠️ The case the console actually still hits - a close that discards an
	 *  order awaiting confirmation - is handled by NotifyConsoleClosed(), which
	 *  prints the SAME Cancelled template.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void CancelPressed();

	// ─────────────────────────────────────────────────────────────────────────
	// THE MODEL-COMPLETION ENTRY POINT (TASK-443 binds the subsystem to this)
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  ⚠️ CALLED ON THE GAME THREAD, EXACTLY ONCE PER DISPATCHED TURN, BY
	 *  TASK-443's completion binding. Not a UFUNCTION: FSiegeLlamaCompletionSignature
	 *  is a plain (non-dynamic) delegate, so the binding is a weak lambda and the
	 *  turn id rides in the capture. The exact shape TASK-443 should use:
	 *
	 *      const int32 DispatchedTurnId = GetTurnId();
	 *      FSiegeLlamaCompletionSignature OnDone;
	 *      OnDone.BindWeakLambda(this,
	 *          [this, DispatchedTurnId](bool bSuccess, const FString& Output, const FString& Error)
	 *          {
	 *              HandleModelCompletion(DispatchedTurnId, bSuccess, Output, Error);
	 *          });
	 *
	 *  ⛔ A COMPLETION WHOSE TURN ID IS NOT THE CURRENT ONE IS DROPPED, NEVER
	 *  APPLIED. That is mechanism #4 of §1's four: a late answer to a cancelled or
	 *  superseded sentence cannot reach the executor, so there is no path by which
	 *  two model outputs can ever meet.
	 *
	 *  @param InTurnId  the TurnId that was current when the request was dispatched
	 *  @param bSuccess  the subsystem's verdict
	 *  @param Output    the raw constrained-decoding result. ⛔ A LOCAL FOREVER - never stored (mechanism #1)
	 *  @param Error     the subsystem's error text; logged, ⛔ NEVER shown to the player (§3)
	 */
	void HandleModelCompletion(int32 InTurnId, bool bSuccess, const FString& Output, const FString& Error);

	/**
	 *  Latch the session fault (§2). Logs once at Error, disables the console,
	 *  aborts anything in flight, and says so with a game-authored template.
	 *  ⛔ It changes NOTHING outside the assistant: no key, no stance, no group.
	 *
	 *  @param Reason RefusedAssistantUnavailable / FailedModelError / FailedTimeout - whichever the caller measured
	 */
	void MarkAssistantFaulted(ESiegeAssistantReasonCode Reason);

	// ─────────────────────────────────────────────────────────────────────────
	// WHAT THE EXECUTOR (TASK-443) READS
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  The snapshot captured for the CURRENT turn - the same object the prompt
	 *  was built from, so the executor resolves places (ResolvePlace) and checks
	 *  orderability (ValidateCommandAgainstSnapshot / GetOrderableCount) against
	 *  the state the model actually saw. ⚠️ Null before the first capture.
	 *  ⛔ Do NOT re-Capture() from the executor: a second survey mid-turn would
	 *  silently answer a different question from the one the model was asked.
	 */
	const USiegeAssistantSnapshot* GetTurnSnapshot() const { return Snapshot; }

	/** The order awaiting confirmation / execution. Default-constructed (Intent None) when there is none. */
	const FSiegeAssistantCommand& GetPendingCommand() const { return PendingArgs.Command; }

	/** The current turn id. Increments once per turn in BeginTurn() and nowhere else. */
	int32 GetTurnId() const { return TurnId; }

	// ─────────────────────────────────────────────────────────────────────────
	// THE PLAYER-FACING SURFACE (§3) - TASK-443 AUTHORS NO STRINGS, IT CALLS THESE
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 *  Fill a template and push it to the widget + the log. ⚠️ THE ONLY ROUTE A
	 *  SENTENCE TAKES TO THE PLAYER. A UI string built anywhere else is a §3
	 *  violation and a QA finding.
	 */
	void PushMessage(ESiegeAssistantReasonCode Code, const FSiegeAssistantMessageArgs& Args);

	/** Convenience overload for the many codes that need no arguments. */
	void PushMessage(ESiegeAssistantReasonCode Code);

	/**
	 *  THE GAME-AUTHORED ONE-LINE SUMMARY OF A PARSED ORDER - what AwaitConfirm
	 *  shows ("Settings screen..." §5) and what the Executed / DeferredArmed
	 *  templates embed as {Order}.
	 *
	 *  ⛔ NEVER A MODEL-PRODUCED STRING. Its only input is FSiegeAssistantCommand,
	 *  which is uint8 / int32 / FName by pin, so the sentence is assembled here
	 *  from symbols the grammar already guaranteed exist.
	 *
	 *  ⛔ ITS FRAMES PRESENT THE PLACE, THEY NEVER RELATE IT TO THE VERB (§30, and
	 *  the reason TASK-471 exists). The frames carry NO preposition, because "to"
	 *  is a destination for send / charge / fall back / rally and a LOCATION for
	 *  guard / ambush - one word cannot be true of both, and a substituted
	 *  {Preposition} would be a localization FRAGMENT rather than a sentence.
	 *  ⚠️ The frames fork on WHICH DATA the command carries, ⛔ NEVER on which verb
	 *  carries it; forking one of them per intent is barred. The test is mechanical
	 *  and lives in handoffs/TASK-471-programmer.md: READ THE FRAME ALOUD WITH
	 *  EVERY VALUE ITS PATH CAN SUPPLY - if any combination is ungrammatical, the
	 *  FRAME is wrong, not the value.
	 *
	 *  ⭐ IT NAMES THE EXCEPTION (TASK-522). An ExcludeKinds order renders
	 *  "Send all except miner (mid)" - the clause is built into {Selection}, so no
	 *  new frame and no new template row was needed. ⛔ A confirm prompt that says
	 *  "Send (mid)" for an order carrying an exception describes a DIFFERENT order
	 *  from the one that will execute, and the ghost circles cannot cover for it:
	 *  SpawnConfirmPreview draws two PLACE decals and nothing per-unit, so this
	 *  sentence is the whole unit-facing half of the review.
	 */
	FText DescribeCommandForPlayer(const FSiegeAssistantCommand& Command) const;

	// ─────────────────────────────────────────────────────────────────────────
	// THE DEV-ONLY OBSERVER (TASK-479 — CONVENTIONS "THE FINE-TUNE RUNG" §13)
	// ─────────────────────────────────────────────────────────────────────────

#if !UE_BUILD_SHIPPING
	/**
	 *  ⛔ DEV-ONLY. Returns the EXACT BYTES THE SHIPPED LANE WOULD COMPOSE for
	 *  RawUtterance, by capturing a live snapshot and then DELEGATING to the one
	 *  composer. ⚠️ IT EXISTS BECAUSE NOTHING IN THIS PROJECT HAS EVER PRINTED
	 *  THE SHIPPED BuildZoneA (CONVENTIONS "In-match LLM command assistant"
	 *  §12g's standing WARN): every byte figure on record - the 3029, the 5116,
	 *  the 68/71 - was printed by the SPIKE's AppendZoneA/AppendZoneB, A
	 *  DIFFERENT LANE, and the fine-tune rung's M3 artifact diff is the first
	 *  reading ever taken from THIS one.
	 *
	 *  ═══════════════════════════════════════════════════════════════════════
	 *  ⚖️ WHY THIS DOES NOT SPEND MECHANISM #2 - WRITTEN HERE, WHERE THE NEXT
	 *  READER MEETS IT, BECAUSE AN ADDITION THAT LEAVES NO WRITTEN TRACE OF WHY
	 *  IT IS SAFE HAS QUIETLY CONVERTED A MECHANISM INTO A COMMENT.
	 *  ═══════════════════════════════════════════════════════════════════════
	 *
	 *  MECHANISM #2 (the class comment, §1's four) reads: ⛔ THE FSM - NOT THE
	 *  EXECUTOR, AND NOT THE MODEL CALL - COMPOSES THE PROMPT. "ComposeTurnPrompt()
	 *  is private and assembles Zone A + Zone B + Zone C itself; the TASK-443
	 *  seam receives a FINISHED prompt string and HAS NO ROUTE TO A ZONE BUILDER.
	 *  ⇒ The one function that could feed the model its own previous words cannot
	 *  reach the model, and the one that reaches the model cannot compose."
	 *
	 *  ⚠️ ITS PROPERTY IS "NO ROUTE TO A ZONE BUILDER" - ⛔ NOT "no public
	 *  function ever returns a prompt". That distinction IS the ruling:
	 *
	 *   · THIS HANDS OUT A FINISHED, IMMUTABLE FString AND NOTHING ELSE. No zone
	 *     builder, no USiegeAssistantSnapshot, no vocabulary, no pending line. A
	 *     caller holding the return value still cannot assemble a prompt, cannot
	 *     re-order the three zones, and cannot build a fourth.
	 *   · ⛔ ComposeTurnPrompt() STAYS PRIVATE. There is still EXACTLY ONE
	 *     COMPOSER; this CALLS it and never reproduces it. Two composers would be
	 *     the same defect this dump was created to measure - a confident green
	 *     describing a string the game does not build (traps T1/T8).
	 *   · THE TASK-443 SEAM IS UNTOUCHED. It still receives a finished string and
	 *     still cannot compose. ⚠️ NOTHING THAT COULD NOT REACH A ZONE BUILDER
	 *     YESTERDAY CAN REACH ONE TODAY - which is the mechanism, stated as a
	 *     property rather than as an access keyword.
	 *
	 *  ⇒ THIS IS AN ADDITION, NOT A RE-EXPOSURE. ⛔ Widening ComposeTurnPrompt
	 *  itself WOULD delete the stated barrier, and it was REFUSED (CONVENTIONS
	 *  "THE FINE-TUNE RUNG" §13(b)). So was `friend class USiegeCheatManager`, on
	 *  the ground ASiegePlayerController::SpawnGroupCircleDecal's own comment
	 *  already records (cited by SYMBOL, not by line - §18c) - friendship exposes
	 *  EVERY private member to reach one function, i.e. it is THE WIDER GRANT,
	 *  NOT THE NARROWER ONE.
	 *
	 *  ⛔ NOT A UFUNCTION, AND COMPILED OUT OF SHIPPING. No exec, no Blueprint
	 *  node, no reflection entry - so it is not a surface a Blueprint or a later
	 *  seam can find. Its ONLY caller is USiegeCheatManager::DumpAssistantPrompt,
	 *  itself on a class the engine never instantiates in a Shipping build.
	 *  ⚠️ The guard token is `!UE_BUILD_SHIPPING` in ALL THREE places (this
	 *  declaration, the definition, and the cheat manager's call site); changing
	 *  it in one place only is a link error, not a compile error.
	 *
	 *  ⛔ WHAT IT DELIBERATELY DOES NOT DO - a diagnostic that moves the thing it
	 *  measures is worth nothing: NO BeginTurn, NO TurnId increment, NO model
	 *  dispatch, NO SetState, NO PushMessage, NO order execution, NO deferred
	 *  latch touched, NO MaxRosterKinds read-modify. It runs SubmitUtterance's
	 *  STEPS 7 AND 8 (capture, compose) and STOPS.
	 *
	 *  ⚠️ IT REFUSES UNLESS THE FSM IS AT REST, AND THE WHITELIST IS THE POINT.
	 *  Capture() RE-SURVEYS the one snapshot object IN PLACE, and
	 *  GetTurnSnapshot()'s contract is that the executor reads the survey THE
	 *  MODEL SAW - "a second survey mid-turn would silently answer a different
	 *  question from the one the model was asked". Idle / Composing / Failed hold
	 *  nothing; Thinking, AwaitConfirm, Clarify and Deferred each still consult
	 *  the current survey downstream. ⇒ It is a WHITELIST so that a state added
	 *  later is REFUSED BY DEFAULT rather than admitted by omission.
	 *
	 *  ⚠️ WHAT IT CONSUMES, DECLARED RATHER THAN OVERLOOKED - both are
	 *  session-latched one-shots that fire INSIDE the delegated path, so
	 *  delegation cannot avoid them without becoming a second composer:
	 *   · bLoggedFirstCapture  - ReportFirstCapture() is called INSIDE
	 *     ComposeTurnPrompt, so the FIRST LIVE CAPTURE audit is spent by whatever
	 *     reaches the composer first, INCLUDING THE PLAYER'S FIRST SENTENCE. It
	 *     was never uniquely this function's to protect. ⛔ This must NOT call
	 *     ReportFirstCapture a second time. ⇒ CONVENTIONS "THE FINE-TUNE RUNG"
	 *     §13(c): WHOEVER CALLS A LATCHED REPORTER FIRST OWNS ITS OUTPUT AND MUST
	 *     PUBLISH IT - so this exec and the owed audit are taken in ONE session,
	 *     by ONE task, with the audit line quoted (TASK-485).
	 *   · bZoneAIdentityChecked - GetCachedZoneA()'s §8 byte-identity re-check
	 *     runs on the SECOND call of a session; this may be that call.
	 *
	 *  @param RawUtterance  the sentence to compose for. ⚠️ TRIMMED HERE with the
	 *                       SAME single TrimStartAndEnd() SubmitUtterance gate 1
	 *                       applies - the bytes must be the ones the shipped lane
	 *                       would build for the same typed string, and that trim
	 *                       is the ONLY transform the shipped lane applies before
	 *                       step 8 (everything else, including SanitizeForPrompt
	 *                       and the MaxUtteranceBytes cap, happens INSIDE
	 *                       BuildZoneC and is therefore inherited, not repeated).
	 *  @return the composed prompt, or ⛔ AN EMPTY STRING ON ANY REFUSAL (empty
	 *          utterance / no authority / FSM not at rest / the survey failed),
	 *          each logged under LogSiegeAssistant. ⚠️ A successful compose can
	 *          never be empty - Zone A alone is thousands of chars - so empty is
	 *          an unambiguous "NO READING WAS TAKEN", never a short reading.
	 */
	FString DebugCaptureAndComposePrompt(const FString& RawUtterance);
#endif // !UE_BUILD_SHIPPING

private:

	// ═════════════════════════════════════════════════════════════════════════
	// THE EXECUTOR LANE - TASK-442 declared these eight as empty seams and
	// TASK-443 FILLED THEM IN PLACE. No name, no signature and no call site
	// moved, and no parallel set was added.
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 *  THE ONE MODEL CALL PER TURN. Resolves USiegeLlamaSubsystem from the
	 *  GameInstance, checks IsReady()/IsBusy() and calls
	 *  RequestCompletion(Prompt, Grammar, OnComplete) with the weak-lambda binding
	 *  documented on HandleModelCompletion.
	 *
	 *  ⚠️ THE PROMPT AND THE GRAMMAR ARRIVE FINISHED, AND THAT IS THE POINT
	 *  (mechanism #2): this function has no route to a zone builder, so it cannot
	 *  feed the model anything the model itself produced.
	 *
	 *  ⛔ A REFUSAL HERE IS NEVER A SESSION FAULT. !IsReady() is indistinguishable
	 *  from "the 2.5 GB model is still loading asynchronously" through the pinned
	 *  §9 API, and latching bAssistantFaulted on it would kill the console for the
	 *  match because the player typed early. Refuse the TURN, keep the console.
	 *
	 *  @return false ⇒ nothing was dispatched; the FSM refuses the turn with
	 *          RefusedAssistantUnavailable and returns to a resting state.
	 */
	bool DispatchTurnToModel(const FString& Prompt, const FString& Grammar);

	/**
	 *  EVERYTHING BETWEEN A PARSED COMMAND AND THE GROUND. A one-line forward to
	 *  RouteParsedCommandInternal(Command, /bForceConfirmReview/ false) - the seam
	 *  signature TASK-442's FSM calls is preserved EXACTLY, and the deferred-fire
	 *  path is the only caller that passes true.
	 *
	 *  ⚠️ CALLER CONTRACT, AND IT IS LOAD-BEARING: `Command` is ALWAYS A LOCAL
	 *  COPY, never a reference into PendingArgs. The body is free to reset the
	 *  pending state at any point without its own argument changing underneath
	 *  it. All THREE call sites honour it (HandleModelCompletion, the
	 *  short-circuit, and the deferred fire) - keep it that way.
	 */
	void RouteParsedCommand(const FSiegeAssistantCommand& Command);

	/**
	 *  THE FOUR STEPS, IN THIS ORDER, AND THE ORDER IS THE SAFETY ARGUMENT:
	 *   1. ⛔ ValidateCommandAgainstSnapshot against the LIVE roster, ALWAYS and
	 *      INDEPENDENTLY OF THE MODEL. A multi-kind selection with ONE bad kind is
	 *      rejected AS A WHOLE (never silently dropped) and routed to
	 *      PushMessage(AskUnsupported) - the EXISTING outcome, no new surface (§6).
	 *   2. eligibility the grammar cannot know: a ZONE verb needs a place that
	 *      actually RESOLVES. Missing / unresolvable -> the EXISTING AskWhichPlace
	 *      clarification, never a new surface.
	 *   3. a deferred trigger (Command.TriggerKind != NAME_None) -> EnterDeferredIntent().
	 *   4. USiegeSettingsSubsystem::IsAssistantConfirmEnabled(), read LIVE.
	 *      ON, or the subsystem unresolvable (FAIL SAFE MEANS MORE REVIEW, NEVER
	 *      LESS), or bForceConfirmReview -> EnterAwaitConfirm().
	 *      OFF -> ExecuteAndReport().
	 *
	 *  ⛔ STEPS 1-3 ARE NOT REACHABLE BY THE TOGGLE. It is read at step 4 and
	 *  nowhere else in the feature, so there is no code path by which turning it
	 *  off can skip a machine check - see the class comment's full ON/OFF trace.
	 *
	 *  @param bForceConfirmReview true ONLY on the deferred-intent fire. A latched
	 *         order fires on a board the player has not looked at since they typed
	 *         it, which is the one case where the review is worth MORE, not less -
	 *         so the deferred path NEVER executes blind, and ⚠️ that stays true
	 *         with the confirm toggle OFF. ⛔ This flag can only force review ON.
	 */
	void RouteParsedCommandInternal(const FSiegeAssistantCommand& Command, bool bForceConfirmReview);

	/** Show the DescribeCommandForPlayer summary (PushMessage(ConfirmPrompt)), raise the ghost circles, broadcast OnAssistantConfirmPromptShown, then SetState(AwaitConfirm). */
	void EnterAwaitConfirm();

	/** Tear down whatever EnterAwaitConfirm put on the ground and broadcast OnAssistantConfirmPromptHidden. ⚠️ Safe to call when nothing is up - EndPlay, CancelPressed, NotifyConsoleClosed, ConfirmPressed and MarkAssistantFaulted all call it unconditionally. */
	void ClearConfirmPreview();

	/**
	 *  Execute PendingArgs.Command through THE SAME PUBLIC APIs THE KEYS CALL
	 *  (§2): CreateUnitGroup (TASK-440's extraction) for Send/Guard/Ambush,
	 *  EnrollInDefaultFollowGroup for Follow, SetUnitCommand for Charge/Fallback,
	 *  AHeroCharacter::Rally for Rally. ⛔ Never a parallel implementation.
	 *
	 *  ⛔ IT RE-CHECKS HasAuthority() ITSELF. CreateUnitGroup carries NO authority
	 *  guard by design (TASK-440 contract #1 - BeginGroupPick owns the M8 D5
	 *  lockout for the pick path, so a second guard there would have been
	 *  unreachable), which makes guarding at THIS call site mandatory rather than
	 *  belt-and-braces.
	 *
	 *  @return false ⇒ nothing was executed; the FSM says so with AskUnsupported
	 *          (the existing outcome) and returns to a resting state.
	 */
	bool ExecutePendingCommand();

	/** Latch the deferred intent: kind + count trigger, DeferredIntentTTLSeconds, a timer armed ONLY while latched, re-resolve on fire and return to AwaitConfirm. ⛔ It NEVER executes blind, and that is true with the confirm toggle OFF as well. */
	void EnterDeferredIntent();

	/** Drop a latched deferred intent and stop its timer. ⚠️ Safe to call when nothing is latched - four callers invoke it unconditionally. */
	void ClearDeferredIntent();

	/** USiegeLlamaSubsystem::CancelActiveRequest for the in-flight turn. ⚠️ Safe to call when nothing is in flight. */
	void AbortInFlightRequest();

	// ═════════════════════════════════════════════════════════════════════════
	// THE EXECUTOR'S OWN HELPERS (TASK-443). None of these is a seam and none is
	// called by TASK-442's FSM - they exist so the four bodies above read as the
	// policy they are rather than as one long function.
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 *  The shared execute-then-report tail, called by BOTH the confirm-ON path
	 *  (ConfirmPressed) and the confirm-OFF path (RouteParsedCommandInternal).
	 *
	 *  ⚠️ ONE FUNCTION, TWO CALLERS, DELIBERATELY - IT IS THE EVIDENCE FOR §5's
	 *  LAW RATHER THAN A TIDY-UP. Two copies of this sequence could drift, and
	 *  the drift would land exactly on the claim the toggle has to keep true
	 *  ("every check that runs with it ON runs with it OFF"). Sharing the body
	 *  makes ON and OFF provably identical downstream of the branch instead of
	 *  identical-by-inspection.
	 */
	void ExecuteAndReport();

	/**
	 *  ⛔ THE TOGGLE READ - THE ONLY ONE IN THE FEATURE. Live, in-memory, at
	 *  confirm time; the subsystem is on the GameInstance so it survives the
	 *  menu -> arena travel.
	 *
	 *  ⛔ AN UNRESOLVABLE SUBSYSTEM RETURNS true (CONFIRM ON), NEVER false
	 *  ("Settings screen..." §5 + TASK-436's published contract). Failing safe
	 *  means MORE human review, never less; silently executing because a lookup
	 *  failed is precisely the failure this feature exists to prevent.
	 */
	bool IsConfirmBeforeExecuteEnabled() const;

	/** The inference subsystem off the GameInstance, or null. ⚠️ Null is a REFUSED TURN, never a session fault - see DispatchTurnToModel. */
	USiegeLlamaSubsystem* ResolveLlamaSubsystem() const;

	/** The owning controller as the ordering controller, or null. ⛔ This component never creates it and never edits it. */
	ASiegePlayerController* ResolveOrderingController() const;

	/**
	 *  THE SELECTOR. Resolves Command.Kinds/Counts into concrete live units.
	 *
	 *  ⛔ IT FILTERS ON THE SHIPPING ELIGIBILITY PREDICATES AND NEVER REIMPLEMENTS
	 *  THEM (CONVENTIONS §8): ASummonedUnit::IsGroupCommandEligible() for the zone
	 *  orders, IsFollowCommandEligible() for Follow. The Cleric-follows-but-cannot-
	 *  hold split has exactly one owner and it is not this file.
	 *
	 *  ⚠️ IT SORTS NEAREST TO THE TARGET, NOT TO THE HERO (§8's executor seam).
	 *
	 *  ⛔ ON SHORTFALL IT RETURNS false RATHER THAN TRUNCATING SILENTLY. Handing
	 *  back 7 units for an order that said 10 is the valid-shaped-wrong-command
	 *  failure this whole architecture exists to stop.
	 *
	 *  ⭐ IT IS ALSO THE ONE PLACE Command.ExcludeKinds MEANS ANYTHING (TASK-522,
	 *  AS-§20.1). who:{"all_except":[…]} parses to an EMPTY Kinds plus a populated
	 *  ExcludeKinds, so the subtraction is a predicate inside the "every eligible
	 *  unit" branch - one pass over the world, no registry and no cache (§4).
	 *  ⛔ THE FOUR INTENTS THAT REACH THIS FUNCTION ARE EXACTLY THE FOUR FOR WHICH
	 *  SiegeAssistantIntentTakesSelection() IS TRUE - Send/Guard/Ambush via
	 *  ExecuteZoneOrder, Follow via ExecuteFollowOrder. Charge and Fallback go to
	 *  ASiegePlayerController::ApplyArmyWideStance and Rally to
	 *  AHeroCharacter::Rally, so an exclusion could never be honoured there, which
	 *  is why the PARSER refuses it for those three rather than this file
	 *  re-deciding it.
	 *
	 *  ⛔ AN EXCLUSION THAT EMPTIES THE SELECTION RETURNS false - the whole order is
	 *  refused through the EXISTING unsupported-ask outcome, with the arithmetic in
	 *  the log. ⛔ NEVER a silent no-op: the parser is pure and has no roster, so
	 *  "did that exception subtract everybody?" is only answerable here.
	 *
	 *  @param Command     the order being executed
	 *  @param SortAnchor  the world point to sort nearest-first against
	 *  @param bFollowOrder true ⇒ IsFollowCommandEligible, false ⇒ IsGroupCommandEligible
	 *  @param OutMembers  the resolved members; ⚠️ a FRESH LOCAL ARRAY that never
	 *                     aliases into UnitGroups (TASK-440 contract #3 - step 6
	 *                     of CreateUnitGroup can reallocate that array)
	 *  @return            true only when EVERY requested count was fully satisfied
	 */
	bool SelectUnitsForOrder(const FSiegeAssistantCommand& Command, const FVector& SortAnchor, bool bFollowOrder,
		TArray<TWeakObjectPtr<ASummonedUnit>>& OutMembers) const;

	/** Send / Guard -> Hold, Ambush -> Ambush. Resolves the place, selects, and calls ASiegePlayerController::CreateUnitGroup. */
	bool ExecuteZoneOrder(const FSiegeAssistantCommand& Command, ESiegeGroupCommandType GroupType, ASiegePlayerController& Controller) const;

	/** Follow -> ASiegePlayerController::EnrollInDefaultFollowGroup per selected unit (already public, already idempotent). */
	bool ExecuteFollowOrder(const FSiegeAssistantCommand& Command, ASiegePlayerController& Controller) const;

	/** Rally -> AHeroCharacter::Rally() on the live anchor pawn. ⚠️ A DEAD hero is not an anchor (the shipped hero-death ruling), so this refuses rather than no-oping silently. */
	bool ExecuteRallyOrder(ASiegePlayerController& Controller) const;

	/** Raise the ghost circles for PendingArgs.Command via ASiegePlayerController::SpawnGroupCircleDecal. ⚠️ Null-safe: a missing M_SpellReticle means no visual and the confirm step still works (TASK-440 contract #7). */
	void SpawnConfirmPreview();

	/** The deferred intent's poll: TTL first, then the trigger. Armed at DeferredIntentPollHz ONLY while latched. */
	void PollDeferredIntent();

	/**
	 *  Live count of one unit kind on the ordering team, counted straight off the
	 *  world.
	 *
	 *  ⛔ NOT A REGISTRY, NOT A CACHE, NOT A DIRTY FLAG AND NOT A SUBSCRIPTION
	 *  LIST - CONVENTIONS §4 rejects all four ON SIGHT and this is the clause to
	 *  cite. It is one filtered actor iteration, computed on demand and kept
	 *  nowhere.
	 *
	 *  ⚠️ EXISTENCE, NOT ORDERABILITY. A deferred trigger means "fire once at
	 *  least N of these EXIST", which is why the guard deliberately does not
	 *  validate TriggerKind either.
	 */
	int32 CountLiveUnitsOfKind(FName Kind) const;

	// ═════════════════════════════════════════════════════════════════════════
	// THE FSM - TASK-442's, and FINISHED
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 *  The widget's OnConsoleOpenChanged(bool) forward. ⚠️ It exists because that
	 *  ONE delegate maps to TWO component entry points (NotifyConsoleOpened /
	 *  NotifyConsoleClosed) and AddDynamic needs a signature-matching UFUNCTION -
	 *  it is an adapter for an arity mismatch, NOT a policy decision. Every other
	 *  forward binds straight through with no adapter.
	 */
	UFUNCTION()
	void HandleConsoleOpenChanged(bool bOpen);

	/** Transition + log. ⚠️ Broadcasts ONLY on an actual change (the delegate law). */
	void SetState(ESiegeAssistantState NewState);

	/** Start a fresh turn: bump TurnId, clear the per-turn dispatch flag. ⚠️ THE ONLY PLACE TurnId MOVES. */
	void BeginTurn();

	/** Drop the pending order, its reason code and its shortfall bookkeeping. Called on every path that leaves a conversation. */
	void ClearPendingIntent();

	/** SetState(Failed) + the template + a cleared pending intent. ⚠️ NOT the session latch. */
	void EnterFailed(ESiegeAssistantReasonCode Code);

	/** Enter Clarify holding a question. The pending line is what carries this into the NEXT fresh single-turn call. */
	void EnterClarify(ESiegeAssistantReasonCode Code, const FSiegeAssistantMessageArgs& Args);

	/**
	 *  THE SHORT-CIRCUIT (task spec §4) - answers a clarification reply with ZERO
	 *  model calls. Only ever runs in Clarify, and only against a shortfall the
	 *  game itself computed, because that is the only question whose answer the
	 *  game already holds.
	 *
	 *  ⚠️ IT NEVER GUESSES. Anything the classifier is not certain of returns
	 *  false and becomes an honest fresh single-turn call.
	 *
	 *  @return true ⇒ the turn is finished here and SubmitUtterance must return
	 */
	bool TryShortCircuitClarification(const FString& Utterance);

	/**
	 *  THE SHORTFALL ARITHMETIC - "you asked for 10 and 8 can take that order."
	 *
	 *  ⚠️ IT DELIBERATELY DECLINES THE ZERO CASE. A kind with 0 orderable is the
	 *  NON-ORDERABLE-KIND GUARD's to refuse ("Settings screen..." §6: it reuses the
	 *  unsupported-ask outcome and invents no new surface), so if ANY kind in the
	 *  selection is 0-orderable this returns false and lets TASK-443's guard rule
	 *  on the whole command. Two surfaces for one situation is how a player gets
	 *  told two different things about the same order.
	 *
	 *  ⚠️ AND IT ONLY RUNS FOR THE ZONE VERBS (Send / Guard / Ambush). The count
	 *  it reads is GetOrderableCount, which is IsGroupCommandEligible's tally;
	 *  FOLLOW's tally (KindFollowable - the Cleric follows but takes no zone
	 *  order) is NOT exposed by the snapshot, and answering "you have 8" from the
	 *  wrong column would be a silent wrong answer. Follow therefore gets no
	 *  shortfall clarification in v1 - stated rather than left to be discovered.
	 *
	 *  @param Command      the parsed order
	 *  @param OutKindIndex index into Command.Kinds of the short kind
	 *  @param OutAvailable how many of that kind can actually take the order (> 0)
	 *  @return             true ⇒ a genuine partial shortfall exists
	 */
	bool FindShortfall(const FSiegeAssistantCommand& Command, int32& OutKindIndex, int32& OutAvailable) const;

	/**
	 *  THE GAME-AUTHORED PENDING LINE (§1) - how context crosses a clarification
	 *  turn WITHOUT the model ever seeing its own previous output.
	 *
	 *  ⛔ NO PARAMETERS, BY DESIGN (mechanism #3). Its only inputs are PendingArgs
	 *  (uint8/int32/FName) and PendingReason (an enum). There is no FString in
	 *  its signature, so model text cannot be routed through it.
	 *
	 *  ⚠️ ASCII ONLY - it becomes a PROMPT LITERAL inside Zone C, where a stray
	 *  multi-byte glyph costs tokens and breaks the char-vs-byte cap arithmetic
	 *  (the ASCII law at the head of SiegeAssistantSnapshot.cpp). CONVENTIONS §1
	 *  illustrates this line with a multiplication sign; this writes `x`, and the
	 *  difference is deliberate - the doc's glyph is prose, the code's is a byte.
	 *
	 *  @return the VALUE of Zone C's `pending:` key ("guard footman x10 -> ancient_ground_near ; problem: only 8 available"), or empty for `none`
	 */
	FString BuildPendingLine() const;

	/** Resolve the ordering player's team. ⚠️ Returns false rather than GUESSING - a wrong team surveys the wrong army and every downstream answer is confidently wrong. */
	bool ResolveOrderingTeam(ETeamId& OutTeam) const;

	/** Create the one snapshot object if it does not exist yet. One object for the life of the component; Capture() resets it. */
	void EnsureSnapshot();

	/** Survey the world for THIS SENTENCE. ⛔ Never called from a tick, a timer or a delegate that fires per frame. */
	bool CaptureTurnSnapshot();

	/** Zone A + Zone B + Zone C, in the fixed order §8 pins. ⚠️ The FSM's job, never the executor's (mechanism #2). */
	FString ComposeTurnPrompt(const FString& Utterance);

	/** The per-request GBNF, generated from the LIVE kinds and places so the model physically cannot name what is not there. */
	FString ComposeTurnGrammar() const;

	/**
	 *  Zone A, built ONCE and cached: it is byte-identical for the life of the
	 *  process and that is what keeps llama_memory_seq_rm's prefix warm (§8).
	 *  ⚠️ On the SECOND call only, and only outside Shipping, it rebuilds and
	 *  compares - turning §8's "calling BuildZoneA twice returns byte-identical
	 *  strings" from a documented criterion into an executed one, at the cost of
	 *  one extra string build per session.
	 */
	const FString& GetCachedZoneA();

	/**
	 *  The vocabulary, resolved ONCE, and ⛔ NEVER NULL IN PRACTICE (TASK-463).
	 *
	 *  ⚠️ A FAILED RESOLVE IS REMEMBERED AS A RESULT: retrying later could succeed
	 *  mid-session and CHANGE ZONE A, which throws away every cached prefix. The
	 *  snapshot's caller contract is "pass the SAME vocabulary object every turn",
	 *  and deciding the lane exactly once honours it.
	 *
	 *  ⛔⛔ TWO LANES, AND WHICH ONE RAN IS AN EVAL-CRITICAL FACT, NOT A DETAIL:
	 *    · the ASSET lane - /Game/Data/DA_AssistantVocabulary loaded. It OVERRIDES
	 *      the C++ defaults WHOLESALE, so it is NOT necessarily the lane any eval
	 *      number describes, and `ZoneA.TwoLaneByteEquality` does not cover it.
	 *    · the DEFAULT lane - a NewObject of this class, i.e. the C++ constructor
	 *      rows. ⭐ THIS IS THE LANE THE WHOLE EVAL LADDER WAS MEASURED ON.
	 *
	 *  ⚠️ THE HISTORY IS KEPT BECAUSE IT IS THE REASON THE FALLBACK EXISTS: until
	 *  TASK-463 this returned the raw LoadSynchronous result, so with no asset on
	 *  disk it returned NULL, BuildZoneA printed `synonyms:\nnone`, and the first
	 *  live run measured zoneA_chars=3029 against a measured lane of 5116. The
	 *  class had always shipped sane defaults (TASK-417); nothing had ever used
	 *  them. CONVENTIONS §12a's lane clause is the law that finding produced.
	 */
	const USiegeAssistantVocabulary* GetVocabulary();

	/**
	 *  ⚠️ THE FIRST-EXECUTION AUDIT ("Settings screen..." §7). USiegeAssistantSnapshot
	 *  has never run: this prints the first live capture IN FULL with zoneA/zoneB/
	 *  zoneC character counts, the cap, and the roster width, so TASK-447 can quote
	 *  an observation instead of an impression. Once per session, at Log level.
	 */
	void ReportFirstCapture(const FString& ZoneA, const FString& ZoneB, const FString& ZoneC);

	/**
	 *  ⛔ THE MISSING CALLER (TASK-455). REGISTERS ZONE A AS THE SUBSYSTEM'S STATIC
	 *  PROMPT PREFIX, WHICH IS WHAT TAKES TWO OF THE THREE BUDGET GUARDS OFF
	 *  "INERT".
	 *
	 *  ⚠️ THE PROBLEM IT SOLVES IS NOT A MISSING CHECK - IT IS A CHECK THAT CANNOT
	 *  SEE. `USiegeLlamaSubsystem` enforces §8's B+C budget on the SNAPSHOT REGION,
	 *  and the only way it can tell that region from the prompt is by knowing where
	 *  Zone A ends. Without a registered prefix, `MaxSnapshotTokens` and
	 *  `SnapshotPreFilterMaxChars` BOTH skip, and §8's ZONE-A SIZE BOUND assertion
	 *  runs with `ZoneA_tokens` treated as 0. TASK-450 logs all three at Warning
	 *  rather than letting the silence read as a pass; this function is what makes
	 *  the Warning stop being true.
	 *  ✅ The CONTEXT budget was always enforced, so there was never an unguarded
	 *  path to `llama_decode` - the gap was the B+C budget, not safety of the call.
	 *
	 *  ⛔ IT WAITS FOR `IsReady()`, AND THAT GATE IS THE WHOLE CORRECTNESS OF THE
	 *  FUNCTION. `SiegeLlamaSubsystem.cpp`'s `ApplyPendingStaticPrefix` DISCARDS a
	 *  prefix registered before the weights land ("Register it after IsReady() goes
	 *  true") - so an eager call would return TRUE, latch here, and leave both
	 *  budgets inert for the whole session. That is the exact failure shape this
	 *  task exists to close, and it must not be recreated one layer up.
	 *
	 *  ⚠️ IDEMPOTENT, AND IT ONLY LATCHES ON SUCCESS. A refusal is retried on the
	 *  next console open or turn, because the one refusal reachable from here (a
	 *  request already in flight) is transient. The Warning is latched separately so
	 *  a retry loop cannot become per-sentence log spam.
	 *
	 *  ⚠️ CALLED FROM THREE PLACES AND ALL THREE ARE DELIBERATE:
	 *    · BeginPlay - THE EARLIEST ARMING POINT (TASK-463 item 3), added because the
	 *      first live run OBSERVED the §8 guardrail inert for a whole match: with
	 *      registration starting at console open, a match in which the console was
	 *      never opened never armed it at all. ⚖️ ON A COLD START THIS IS A
	 *      DELIBERATE NO-OP - the model is still loading, IsReady() is false, and the
	 *      two callers below do the work. What it closes completely is MATCH 2+ IN A
	 *      SESSION and any level travel: the subsystem lives on the GameInstance and
	 *      SURVIVES travel, so the model is already loaded and the prefix arms before
	 *      anything can open a console. ⛔ The remaining cold-start window is
	 *      UNREACHABLE, not merely small: while !IsReady() the plugin discards a
	 *      prefix AND RequestCompletion returns false immediately, so no request can
	 *      reach llama_decode unguarded during it.
	 *    · NotifyConsoleOpened - EARLY, so the worker has APPLIED the prefix before
	 *      the player finishes typing. This is what makes the CHAR pre-filter live
	 *      on turn 1: it reads the APPLIED prefix on the game thread inside
	 *      RequestCompletion, so a prefix registered in the same frame is too late
	 *      for it.
	 *    · DispatchTurnToModel - the BACKSTOP, for the session where the model
	 *      became ready between the open and the sentence. The worker applies a
	 *      pending prefix BEFORE it picks up pending work, so even a same-frame
	 *      registration makes the TOKEN authority live on that very request.
	 *
	 *  @param Llama the resolved subsystem, or null (null ⇒ no-op, no log, no latch)
	 */
	void EnsureStaticPrefixRegistered(USiegeLlamaSubsystem* Llama);

	// ═════════════════════════════════════════════════════════════════════════
	// STATE. ⚠️ All of it is per-session or per-turn, and NONE of it is a
	// conversation buffer - see mechanism #1 on the class comment.
	// ═════════════════════════════════════════════════════════════════════════

	/** The FSM state. */
	ESiegeAssistantState State = ESiegeAssistantState::Idle;

	/** ⚠️ THE SESSION LATCH (§2). Disables the console and NOTHING else. Never cleared within a session. */
	bool bAssistantFaulted = false;

	/** Whether the console is currently open. Purely informational for the FSM; the widget owns its own visibility. */
	bool bConsoleOpen = false;

	/** Increments once per turn in BeginTurn(). The stale-completion guard compares against it. */
	int32 TurnId = 0;

	/** ⛔ ONE DISPATCH PER TURN. Set in SubmitUtterance's dispatch step, cleared in BeginTurn. */
	bool bModelDispatchedThisTurn = false;

	/** The last game-authored line, for seed-then-bind. ⚠️ GAME-AUTHORED - it is PushMessage's output, never model text. */
	FString LastMessage;

	/**
	 *  The pending order + its message arguments. ⛔ uint8 / int32 / FName only,
	 *  by the struct's own pin - this is the whole of what survives a turn.
	 */
	UPROPERTY(Transient)
	FSiegeAssistantMessageArgs PendingArgs;

	/** Why the FSM is holding a conversation. None ⇒ nothing pending, and BuildPendingLine returns empty. */
	ESiegeAssistantReasonCode PendingReason = ESiegeAssistantReasonCode::None;

	/** Index into PendingArgs.Command.Kinds of the kind the shortfall is about, or INDEX_NONE. */
	int32 PendingShortfallIndex = INDEX_NONE;

	/** THE ONE SNAPSHOT. Created once, re-Capture()d per sentence. ⛔ Not a cache, not a registry - see §4. */
	UPROPERTY(Transient)
	TObjectPtr<USiegeAssistantSnapshot> Snapshot;

	/** DA_AssistantVocabulary (TASK-421's asset instance), resolved once. Missing ⇒ the C++ constructor defaults are used instead (TASK-463) - see GetVocabulary. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant")
	TSoftObjectPtr<USiegeAssistantVocabulary> VocabularyAsset;

	/** The live vocabulary - the loaded asset, else a NewObject carrying the C++ defaults. Null ONLY if that allocation failed, which logs at Warning. Pinned for the life of the component - see GetVocabulary. */
	UPROPERTY(Transient)
	TObjectPtr<USiegeAssistantVocabulary> ResolvedVocabulary;

	/** True once the resolve has been ATTEMPTED, whatever it returned. */
	bool bVocabularyResolved = false;

	/** Zone A, built once. ⚠️ Byte-identical for the life of the process is the §8 law this cache enforces. */
	FString CachedZoneA;

	/** True once CachedZoneA has been built. A separate flag rather than CachedZoneA.IsEmpty(), so a (pathological) empty Zone A is built once and reported, not rebuilt every turn. */
	bool bZoneABuilt = false;

	/** One-shot latch for the outside-Shipping Zone-A byte-identity re-check. */
	bool bZoneAIdentityChecked = false;

	/** One-shot latch for the §7 first-execution audit log. */
	bool bLoggedFirstCapture = false;

	/** One-shot latch so the vocabulary LANE is reported once, not once per sentence. ⚠️ Named "Warned" historically; since TASK-463 the missing-asset case logs at Log (it is the designed default path), and only a failed fallback allocation still warns. */
	bool bWarnedMissingVocabulary = false;

	/**
	 *  True once USiegeLlamaSubsystem::SetStaticPrefix has RETURNED TRUE (TASK-455).
	 *
	 *  ⚠️ IT RECORDS A SUBMISSION, NOT AN OUTCOME, AND THAT LIMIT IS REAL. The
	 *  worker tokenizes the prefix on its own thread and can still report a
	 *  tokenisation failure there; the pinned §9 surface exposes no way for this
	 *  lane to read that back, so the plugin's own Warning is the only signal.
	 *  ⛔ Do not add a getter for it - the fix for an unobservable failure is the
	 *  log the plugin already emits, not a new cross-lane accessor.
	 */
	bool bStaticPrefixRegistered = false;

	/** One-shot latch so a REFUSED SetStaticPrefix is reported once. Separate from bStaticPrefixRegistered on purpose: the refusal is retried, so latching them together would either spam the log or stop the retry. */
	bool bWarnedStaticPrefixRefused = false;

	// ═════════════════════════════════════════════════════════════════════════
	// THE EXECUTOR'S STATE (TASK-443). ⚠️ Still all per-session or per-turn, and
	// still none of it is a conversation buffer: FSiegeAssistantCommand is pinned
	// to uint8 / int32 / FName, so DeferredCommand cannot hold model TEXT either.
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 *  The POSITION-zone ghost circle raised by the confirm step, or null.
	 *
	 *  ⛔ PREVIEW-ONLY, AND IT IS NEVER TRANSFERRED TO THE FORMED GROUP. TASK-440
	 *  contract #2 makes CreateUnitGroup's marker parameters OWNERSHIP-TRANSFERRING,
	 *  but TASK-442's ConfirmPressed tears the preview down BEFORE it executes (so
	 *  no ghost can outlive the decision it belonged to) - by which point there is
	 *  nothing left to transfer. ⇒ CreateUnitGroup is called with NULL markers and
	 *  an assistant-formed group owns no persistent ground marker, unlike a
	 *  key-formed one. Declared, not discovered - see the handoff.
	 */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> ConfirmPositionDecal;

	/** The ATTACK-zone ghost circle raised by the confirm step, or null. Same preview-only ownership rule as ConfirmPositionDecal. */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> ConfirmAttackDecal;

	/**
	 *  Whether a confirm prompt is currently UP.
	 *
	 *  ⚠️ IT IS TRACKED SEPARATELY FROM THE DECALS ON PURPOSE: SpawnGroupCircleDecal
	 *  returns null when M_SpellReticle is missing (TASK-440 contract #7), so
	 *  "are there decals?" is NOT the same question as "is the player being asked?"
	 *  - and on a machine with no reticle material the second must still be true.
	 *  It is also what keeps OnAssistantConfirmPromptShown/Hidden strictly paired,
	 *  so the delegate law's "never broadcast a no-op" holds for both.
	 */
	bool bConfirmPromptUp = false;

	/** The latched deferred order, with its trigger still attached. Default-constructed (Intent None) when nothing is latched. */
	UPROPERTY(Transient)
	FSiegeAssistantCommand DeferredCommand;

	/** True while a deferred intent is latched. ⚠️ The timer is armed ONLY while this is true. */
	bool bDeferredIntentLatched = false;

	/** World time at which the latch expires. Compared against UWorld::GetTimeSeconds, so it stops with a paused world exactly as the rest of the game does. */
	double DeferredIntentExpiryTime = 0.0;

	/** The 1 Hz poll handle. ⛔ Armed in EnterDeferredIntent and cleared in ClearDeferredIntent, and nowhere else. */
	FTimerHandle DeferredIntentPollTimer;

	/**
	 *  Deferred-intent TTL, seconds (CONVENTIONS "Settings screen…" §9 registry:
	 *  DeferredIntentTTLSeconds = 120). ⚠️ FLAGGED for Jonathan's feel pass - it
	 *  is the window between "I said wait" and "I have forgotten I said wait".
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant", meta = (ClampMin = "1.0"))
	float DeferredIntentTTLSeconds = 120.f;

	/**
	 *  Deferred-intent poll rate, Hz (§9 registry: DeferredIntentPollHz = 1).
	 *  ⚠️ 1 Hz is deliberate: the trigger is "at least N of a kind exist", which
	 *  changes on a summon, not on a frame. ⛔ Clamped away from 0 because the
	 *  period is 1/Hz, and the code clamps again before dividing.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant", meta = (ClampMin = "0.1"))
	float DeferredIntentPollHz = 1.f;

	/**
	 *  ⚠️ PAIRED TUNABLE - MIRRORS ASiegePlayerController::GroupPositionRadiusDefault
	 *  (700), AND THE DUPLICATION IS FLAGGED RATHER THAN HIDDEN. That member is
	 *  `protected`, so this component genuinely cannot read it, and hard-coding
	 *  the number at the call site would have hidden the second source of truth
	 *  instead of naming it (the SpawnBoxHalfExtent 3-way paired-tunable
	 *  precedent). ⇒ KEEP THE TWO IN LOCKSTEP. The clean fix is one access change
	 *  on the controller - a `SiegePlayerController` edit, which this task is
	 *  forbidden - after which this member is deleted.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant", meta = (ClampMin = "0"))
	float AssistantPositionRadius = 700.f;

	/** ⚠️ PAIRED TUNABLE - mirrors ASiegePlayerController::GroupAttackRadiusDefault (1500), for the identical reason. Keep in lockstep. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant", meta = (ClampMin = "0"))
	float AssistantAttackRadius = 1500.f;

	/**
	 *  The attached console widget (TASK-453 supplies it through
	 *  AttachConsoleWidget).
	 *
	 *  ⛔ WEAK, AND THAT IS THE WHOLE POINT OF THE POINTER'S TYPE. TASK-442's rule
	 *  was that the FSM must not own a widget's lifetime, and a weak pointer keeps
	 *  that true while still allowing the join: the controller creates, shows,
	 *  hides and destroys the widget, and if it goes away underneath us this
	 *  simply reads null. ⚠️ A TObjectPtr here would keep a dead console alive and
	 *  make this component the thing that decides when a UI object dies.
	 */
	TWeakObjectPtr<USiegeAssistantConsoleWidget> AttachedConsoleWidget;
};
