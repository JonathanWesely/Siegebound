// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeAssistantComponent.h"

#include "Engine/DecalActor.h"         // complete type: the ghost-circle ADecalActor the confirm step raises
#include "Engine/World.h"
#include "EngineUtils.h"               // TActorIterator - the selector's and the deferred trigger's only survey
#include "GameFramework/Actor.h"       // complete type: GetOwner()->HasAuthority()
#include "GameFramework/Controller.h"  // complete type: OwningController->PlayerState
#include "GameFramework/PlayerState.h" // complete type: the Cast<ASiegePlayerState> source
#include "Engine/GameInstance.h"       // complete type: GetSubsystem<USiegeSettingsSubsystem>() / <USiegeLlamaSubsystem>()
#include "TimerManager.h"              // FTimerManager::SetTimer / ClearTimer - the deferred intent's poll

#include "Siegebound/HeroCharacter.h"            // AHeroCharacter::Rally - the hero ability, called exactly as IA_Rally calls it
#include "Siegebound/SiegeAssistantConsoleWidget.h" // the console widget's inbound/outbound API - AttachConsoleWidget binds both directions
#include "Siegebound/SiegeAssistantGrammar.h"    // USiegeAssistantGrammar::Build + GrammarCountMin/Max (the short-circuit's range IS the grammar's)
#include "Siegebound/SiegeAssistantRegionStatics.h" // ⭐ THE ONLY CALL SITE IN THE MODULE (TASK-544 shipped with zero, deliberately): IsPointInRegion, the selector's region membership test
#include "Siegebound/SiegeAssistantSnapshot.h"   // Capture / BuildZoneA|B|C / GetOrderableCount / ResolvePlace / ResolvePlaceRegion / GetRegionPlaceNames / ValidateCommandAgainstSnapshot
#include "Siegebound/SiegeAssistantVocabulary.h" // DA_AssistantVocabulary, for Zone A's synonym block
#include "Siegebound/SiegePlayerController.h"    // CreateUnitGroup / SpawnGroupCircleDecal / EnrollInDefaultFollowGroup / SetUnitCommand
#include "Siegebound/SiegePlayerState.h"         // ASiegePlayerState::GetTeam - the ordering team
#include "Siegebound/SiegeSettingsSubsystem.h"   // ⛔ THE TOGGLE: IsAssistantConfirmEnabled(), read live at confirm time
#include "Siegebound/SummonedUnit.h"             // IsGroupCommandEligible / IsFollowCommandEligible - the SHIPPED predicates, never reimplemented

// ⚠️ THE PLUGIN LANE, AND THIS IS THE FIRST GAME-LANE FILE EVER TO REACH INTO IT
// (TASK-443 adds "SiegeLlama" to GitClaudeUnrealTest.Build.cs for exactly this
// line). The cross-lane surface is (prompt, gbnf) -> string and nothing else:
// llama.h stays PRIVATE to the plugin, so nothing native is on this include path.
#include "SiegeLlamaSubsystem.h"                 // USiegeLlamaSubsystem + FSiegeLlamaCompletionSignature (CONVENTIONS §9, character-for-character)

// ═══════════════════════════════════════════════════════════════════════════
// ⛔ THE THREE-ROLE BUDGET INVARIANTS, CHECKED AT COMPILE TIME (TASK-455).
//
// This translation unit is the ONLY one that sees both lanes' budget constants,
// which is exactly why the checks live here and not in a header: they cost the
// snapshot header no coupling to the plugin, and a game header dragging
// SiegeLlamaSubsystem.h into two automation-test TUs would be a real price for a
// comment's worth of benefit.
//
// ⚠️ THESE ARE THE MECHANICAL FORM OF A LAW CONVENTIONS §8 STATES AS PROSE:
// "A PRE-FILTER THAT CAN REJECT WHAT THE AUTHORITY WOULD ACCEPT IS NOT A
// PRE-FILTER. IT IS A SECOND, HIDDEN AUTHORITY." A prose law in a 500 KB
// document is checked by whoever remembers to read it; a static_assert is
// checked by the compiler, every build, forever - and the defect this wave keeps
// finding is precisely a constant that drifted while its documentation did not.
// ═══════════════════════════════════════════════════════════════════════════

// (1) THE PRE-FILTER MUST NEVER CUT WHAT THIS LANE DELIBERATELY BUILT. BuildZoneC
//     trims its roster to SnapshotTrimBudgetChars; if the plugin's char pre-filter
//     ever sat at or below that figure it would reject snapshots this lane had
//     already sized to fit, i.e. re-impose a character verdict IN FRONT OF the
//     tokenizer that exists to end them. Today: 1085 vs 3000.
static_assert(USiegeAssistantSnapshot::SnapshotTrimBudgetChars < USiegeLlamaSubsystem::SnapshotPreFilterMaxChars,
	"The plugin's char PRE-FILTER has dropped to or below this lane's TRIM budget. A pre-filter that can reject what the builder deliberately produced is a second, hidden authority (CONVENTIONS 'In-match LLM command assistant' section 8). Raise SnapshotPreFilterMaxChars, or lower SnapshotTrimBudgetChars - never leave them crossed.");

// (2) ZONE B MAY NOT BE RESERVED MORE THAN THE WHOLE SNAPSHOT'S BUDGET. A reserve
//     that met or exceeded the trim budget would drive RosterBudget negative and
//     the roster would collapse to zero kinds on EVERY board - a total prompt
//     degradation reachable by a one-line constant edit. Today: 192 vs 1085.
static_assert(USiegeAssistantSnapshot::ZoneBCharReserve < USiegeAssistantSnapshot::SnapshotTrimBudgetChars,
	"ZoneBCharReserve has reached SnapshotTrimBudgetChars. RosterBudget would go negative and every board would print an empty roster.");

// (3) THE TWO PLAYER-TEXT LINES TOGETHER MAY NOT SWALLOW THE WHOLE SNAPSHOT.
//     `pending:` and `order:` are each bounded by MaxUtteranceBytes and BOTH sit in
//     BuildZoneC's Tail, so both are subtracted before the roster is given a budget
//     (qa/TASK-416.md WARN-1 - the dimension a roster-width analysis missed).
//     ⚠️ NECESSARY, NOT SUFFICIENT, AND SAY SO: the head and the rest of the tail
//     also spend the budget, so clearing this assert does not prove the roster gets
//     a usable slice - only that the two player lines ALONE cannot eat all of it.
//     The actual squeeze is reported at RUNTIME by BuildZoneC's collapse Warning,
//     which is the observable half. Today: 2 x 240 = 480 vs 1085 - 192 = 893.
static_assert(2 * USiegeAssistantSnapshot::MaxUtteranceBytes
	< USiegeAssistantSnapshot::SnapshotTrimBudgetChars - USiegeAssistantSnapshot::ZoneBCharReserve,
	"Two player-text lines at MaxUtteranceBytes now consume the entire Zone C budget. The roster would be squeezed to nothing on any board where the player types a long sentence with a long pending line - qa/TASK-416.md WARN-1, arriving for real.");

// ═══════════════════════════════════════════════════════════════════════════
// ⛔ THIS FILE IS THE ONLY PLACE PLAYER-FACING TEXT EXISTS (CONVENTIONS
// "In-match LLM command assistant" §3). The model emits SYMBOLS; every sentence
// a player reads is a game-authored template filled from a REASON CODE. There is
// no "let the model phrase it nicely" path, and a UI literal anywhere else in
// the assistant lane is a §3 violation.
//
// ⚠️ TWO TEXT DOMAINS LIVE HERE AND THEY HAVE OPPOSITE RULES. Keep them apart:
//   · PLAYER TEXT   - FText / NSLOCTEXT, localizable, built inside a function
//                     (never at module static-init: localization may not be up).
//   · PROMPT TEXT   - the pending line only. ASCII ONLY, terse, byte-counted:
//                     it lands inside Zone C, where a multi-byte glyph costs
//                     tokens and breaks the char-vs-byte cap arithmetic (the
//                     ASCII law at the head of SiegeAssistantSnapshot.cpp).
// ═══════════════════════════════════════════════════════════════════════════

namespace SiegeAssistantComponentInternal
{
	/** DA_AssistantVocabulary (CONVENTIONS §5). Resolved null-safe ONCE - see USiegeAssistantComponent::GetVocabulary. */
	static const TCHAR* const VocabularyAssetPath = TEXT("/Game/Data/DA_AssistantVocabulary.DA_AssistantVocabulary");

	/**
	 *  THE SHORT-CIRCUIT'S CLOSED WORD LISTS.
	 *
	 *  ⚠️ EXACT MATCHES ONLY, AFTER TRIMMING / LOWERCASING / STRIPPING TRAILING
	 *  PUNCTUATION - never a substring or prefix test. "no archers" must NOT match
	 *  "no", and a substring test is exactly how it would: that reply is a real
	 *  order and belongs to the model, not to a keyword table. The whole value of
	 *  this table is that it is TIMID; a permissive one is a second, worse parser
	 *  competing with the thing we shipped a 4B model to do (§2).
	 */
	static const TCHAR* const AffirmativeReplies[] =
	{
		TEXT("y"), TEXT("yes"), TEXT("yeah"), TEXT("yep"), TEXT("yup"),
		TEXT("ok"), TEXT("okay"), TEXT("sure"), TEXT("fine"), TEXT("aye"),
		TEXT("confirm"), TEXT("accept"), TEXT("do it"), TEXT("go ahead")
	};

	static const TCHAR* const NegativeReplies[] =
	{
		TEXT("n"), TEXT("no"), TEXT("nope"), TEXT("nah"), TEXT("cancel"),
		TEXT("stop"), TEXT("abort"), TEXT("forget it"), TEXT("never mind"),
		TEXT("nevermind"), TEXT("no thanks")
	};

	/** Trailing punctuation a player types without meaning anything by it ("yes!", "8."). */
	static bool IsTrailingPunctuation(TCHAR Character)
	{
		return Character == TEXT('.') || Character == TEXT('!') || Character == TEXT('?')
			|| Character == TEXT(',') || Character == TEXT(';') || Character == TEXT(':');
	}

	/**
	 *  PLAYER TEXT - the verb, capitalised, for the order summary. Separate from
	 *  SiegeAssistantIntentToSymbol on purpose: that one produces the WIRE symbol
	 *  ("fallback") which the grammar and the prompt share, and this one produces
	 *  the SENTENCE ("Fall back") which only a human reads. Merging them would put
	 *  a localizable string into the prompt.
	 *
	 *  ⚠️ SINCE TASK-465 THIS IS ALSO THE {Intent} TEMPLATE ARGUMENT (PushMessage),
	 *  so a verb missing from this switch is a verb missing from a player's
	 *  sentence. ✅ The default FAILS SAFE - an unknown intent yields an EMPTY word,
	 *  never a WRONG one, which is the whole point of the change that added the
	 *  argument. ⛔ A new ESiegeAssistantIntent member still owes a row here AND a
	 *  word in the AskWhichIntent template.
	 */
	static const FText& IntentDisplayText(ESiegeAssistantIntent Intent)
	{
		switch (Intent)
		{
		case ESiegeAssistantIntent::Send:     { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Send",     "Send");      return T; }
		case ESiegeAssistantIntent::Guard:    { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Guard",    "Guard");     return T; }
		case ESiegeAssistantIntent::Ambush:   { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Ambush",   "Ambush");    return T; }
		case ESiegeAssistantIntent::Follow:   { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Follow",   "Follow");    return T; }
		case ESiegeAssistantIntent::Charge:   { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Charge",   "Charge");    return T; }
		case ESiegeAssistantIntent::Fallback: { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Fallback", "Fall back"); return T; }
		case ESiegeAssistantIntent::Rally:    { static const FText T = NSLOCTEXT("Siegebound", "Assistant_Intent_Rally",    "Rally");     return T; }
		default:                              { static const FText T = FText::GetEmpty();                                                return T; }
		}
	}

	/**
	 *  PROMPT TEXT - the `; problem: ...` half of the pending line. ASCII only,
	 *  terse, and it names the SAME reason code the player-facing template used,
	 *  so the model and the player are told about the same thing in their own
	 *  registers.
	 *
	 *  ⚠️ AskUnsupported deliberately produces NOTHING. An unsupported ask is a
	 *  refusal, not a question about a partial order: there is no context to carry
	 *  and inventing one would put a claim in the prompt that nothing backs.
	 */
	static FString BuildProblemClause(ESiegeAssistantReasonCode Reason, int32 Available, FName Kind)
	{
		switch (Reason)
		{
		case ESiegeAssistantReasonCode::ShortfallCount:
			return FString::Printf(TEXT("problem: only %d %s available"), Available, *Kind.ToString());
		case ESiegeAssistantReasonCode::AskWhichUnit:
			return TEXT("problem: which unit");
		case ESiegeAssistantReasonCode::AskWhichPlace:
			return TEXT("problem: which place");
		case ESiegeAssistantReasonCode::AskHowMany:
			return TEXT("problem: how many");
		case ESiegeAssistantReasonCode::AskWhichIntent:
			return TEXT("problem: which order");
		default:
			return FString();
		}
	}

	/**
	 *  TRUE for the three SELECTION-BEARING ZONE VERBS, and ⛔ NARROWER THAN THE
	 *  SHIPPED SiegeAssistantIntentTakesSelection(), which also returns true for
	 *  Follow. The difference is the whole reason the non-orderable-kind guard is
	 *  not a regression: the Cleric FOLLOWS and CANNOT take zone orders, so a
	 *  Cleric row is `Count > 0, Orderable == 0`, and a predicate that lumped
	 *  Follow in here would refuse "clerics follow me" - a legal shipped order and
	 *  verbatim the eval's own DEV-20 utterance.
	 *
	 *  ⚠️ IT IS THE SAME PREDICATE TASK-441 KEEPS FILE-LOCAL IN
	 *  SiegeAssistantSnapshot.cpp (SiegeAssistantGuardInternal::IntentTakesZoneOrder)
	 *  AND THE DUPLICATION IS FLAGGED, NOT HIDDEN: both are file-local statics, so
	 *  neither can be reached from the other, and the shared home would be
	 *  SiegeAssistantCommand.h - a PINNED header owned by another task. Recorded in
	 *  the handoff as a consolidation for whoever next owns that file. ⛔ The two
	 *  MUST agree; if one gains a verb, so does the other.
	 */
	static bool IntentTakesZoneOrder(ESiegeAssistantIntent Intent)
	{
		return Intent == ESiegeAssistantIntent::Send
			|| Intent == ESiegeAssistantIntent::Guard
			|| Intent == ESiegeAssistantIntent::Ambush;
	}

	/** Log-only name for a guard rejection, so a refusal line names its reason instead of a byte. ⛔ NOT player-facing - the guard invents no player surface (§6). */
	static const TCHAR* RejectReasonName(ESiegeAssistantRejectReason Reason)
	{
		switch (Reason)
		{
		case ESiegeAssistantRejectReason::None:            return TEXT("None");
		case ESiegeAssistantRejectReason::KindNotOrderable: return TEXT("KindNotOrderable");
		case ESiegeAssistantRejectReason::KindUnknown:      return TEXT("KindUnknown");
		default:                                            return TEXT("<unknown>");
		}
	}

	/** Log-only name for a state, so an FSM transition line is readable without decoding a byte. */
	static const TCHAR* StateName(ESiegeAssistantState InState)
	{
		switch (InState)
		{
		case ESiegeAssistantState::Idle:         return TEXT("Idle");
		case ESiegeAssistantState::Composing:    return TEXT("Composing");
		case ESiegeAssistantState::Thinking:     return TEXT("Thinking");
		case ESiegeAssistantState::AwaitConfirm: return TEXT("AwaitConfirm");
		case ESiegeAssistantState::Clarify:      return TEXT("Clarify");
		case ESiegeAssistantState::Failed:       return TEXT("Failed");
		case ESiegeAssistantState::Deferred:     return TEXT("Deferred");
		default:                                 return TEXT("<unknown>");
		}
	}
}

// ---------------------------------------------------------------------------
// THE SHORT-CIRCUIT'S CLASSIFIER - pure, free, no UObject and no engine state.
// ---------------------------------------------------------------------------

bool SiegeAssistantParseClarificationReply(const FString& Reply, ESiegeAssistantClarifyReply& OutKind, int32& OutQuantity)
{
	using namespace SiegeAssistantComponentInternal;

	// NEVER PARTIALLY FILLS: a caller that ignores the return value gets the safe
	// answer, exactly as ParseSiegeAssistantCommand's contract does.
	OutKind = ESiegeAssistantClarifyReply::Unrecognized;
	OutQuantity = 0;

	FString Normalized = Reply.TrimStartAndEnd().ToLower();
	while (!Normalized.IsEmpty() && IsTrailingPunctuation(Normalized[Normalized.Len() - 1]))
	{
		Normalized = Normalized.LeftChop(1);
	}
	Normalized.TrimStartAndEndInline();

	if (Normalized.IsEmpty())
	{
		return false;
	}

	// A BARE INTEGER, AND NOTHING ELSE. "send 8" is a real order and must cost an
	// honest model call - it is not the game's business to decide that the 8 in it
	// answers the question the game happened to ask last.
	bool bAllDigits = true;
	for (const TCHAR Character : Normalized)
	{
		if (!FChar::IsDigit(Character))
		{
			bAllDigits = false;
			break;
		}
	}

	if (bAllDigits)
	{
		// ⚠️ LENGTH-GUARD BEFORE THE CONVERSION, NOT AFTER. A 25-digit reply
		// overflows Atoi64 SILENTLY and can wrap into a value the range check then
		// waves through - "the check happens after the damage" is the same shape as
		// every silent-success defect this lane keeps finding.
		if (Normalized.Len() > 9)
		{
			return false;
		}

		// ⚠️ THE RANGE IS THE GRAMMAR'S, READ FROM THE GRAMMAR. A local 1..30 here
		// would be a second copy of a number CONVENTIONS §1 spends a paragraph on.
		const int64 Value = FCString::Atoi64(*Normalized);
		if (Value >= USiegeAssistantGrammar::GrammarCountMin && Value <= USiegeAssistantGrammar::GrammarCountMax)
		{
			OutKind = ESiegeAssistantClarifyReply::Quantity;
			OutQuantity = static_cast<int32>(Value);
			return true;
		}

		// Out of range ⇒ NOT a confident classification. The model gets it.
		return false;
	}

	// Already lower-cased, so an exact byte compare is the whole test. Strcmp
	// rather than FString::Equals: no temporary FString per candidate word, and no
	// question about which overload a TCHAR* selects.
	for (const TCHAR* const Word : AffirmativeReplies)
	{
		if (FCString::Strcmp(*Normalized, Word) == 0)
		{
			OutKind = ESiegeAssistantClarifyReply::Affirmative;
			return true;
		}
	}

	for (const TCHAR* const Word : NegativeReplies)
	{
		if (FCString::Strcmp(*Normalized, Word) == 0)
		{
			OutKind = ESiegeAssistantClarifyReply::Negative;
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------------------
// THE PLAYER-FACING TEMPLATE TABLE (§3). THE ONLY SOURCE OF PLAYER SENTENCES.
// ---------------------------------------------------------------------------

const FText& SiegeAssistantReasonTemplate(ESiegeAssistantReasonCode Code)
{
	switch (Code)
	{
	case ESiegeAssistantReasonCode::AskWhichUnit:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_AskWhichUnit", "Which units do you mean?");
		return T;
	}
	case ESiegeAssistantReasonCode::AskWhichPlace:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_AskWhichPlace", "Where should they go?");
		return T;
	}
	case ESiegeAssistantReasonCode::AskHowMany:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_AskHowMany", "How many?");
		return T;
	}
	case ESiegeAssistantReasonCode::AskWhichIntent:
	{
		// ⚠️ THIS SENTENCE ENUMERATES ESiegeAssistantIntent BY HAND AND NOTHING
		// ENFORCES THAT IT STILL DOES. ✅ Correct as of TASK-465's sweep: the seven
		// words below are exactly the seven non-None members (SiegeAssistantCommand.h),
		// which is why this is an ANCHOR and not a finding. ⛔ AN EIGHTH VERB MUST
		// BE ADDED HERE AND TO IntentDisplayText, or the game asks the player to
		// pick from a list that no longer describes what it accepts.
		// ⚠️ DELIBERATELY NOT JOINED FROM THE ENUM AT RUNTIME: a comma-and-"or"
		// list assembled in code is not translatable - the conjunction and the word
		// order belong to the translator, not to us - so the hand-written sentence
		// is the correct trade and the drift is paid for with this comment.
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_AskWhichIntent", "What should they do - send, guard, ambush, follow, charge, fall back or rally?");
		return T;
	}
	case ESiegeAssistantReasonCode::AskUnsupported:
	{
		// ⚠️ ONE SURFACE, THREE SOURCES, AND THAT IS THE DESIGN ("Settings
		// screen..." §6): the model's {"ask":"unsupported"}, TASK-443's
		// non-orderable-kind guard, and a refused execution all land here. The
		// guard "reuses the existing unsupported-ask outcome and invents no new
		// player-facing surface" - so this sentence must stay true of all three,
		// which is why it says what the ASSISTANT cannot do rather than
		// diagnosing the board.
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_AskUnsupported", "I can't turn that into an order.");
		return T;
	}
	case ESiegeAssistantReasonCode::ShortfallCount:
	{
		// ⛔ THE VERB IS A PARAMETER, NOT A LITERAL - AND IT IS ONE STRING, NOT
		// THREE. This row serves EVERY zone verb: FindShortfall returns true for
		// Send, Guard AND Ambush and for nothing else (its intent gate is the
		// whole of that guarantee), so the hard-coded "Send" that used to sit here
		// answered a player who typed "guard the mine with 10 pikemen" by talking
		// about SENDING them - the one defect in this feature a player meets
		// without reading a log (qa/TASK-464.md WARN-4).
		// ⚠️ FORKING THIS ROW PER INTENT WOULD BE THREE SENTENCES TO DRIFT (§19 /
		// §22). {Intent} is the SAME game-authored word DescribeCommandForPlayer
		// prints, from the SAME IntentDisplayText switch, keyed off the PARSED
		// COMMAND's own intent - never off anything the model emitted (§3).
		// ⛔ AND NOT {Order}, WHICH IS THE OBVIOUS-LOOKING ALTERNATIVE AND IS
		// WRONG: at shortfall time Counts still holds the count that CANNOT be met
		// (the player's 10), so {Order} would offer back verbatim the order this
		// sentence exists to say is impossible.
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_ShortfallCount", "You asked for {Requested} {Kind} - {Available} can take that order. {Intent} {Available}?");
		return T;
	}
	case ESiegeAssistantReasonCode::RefusedNoAuthority:
	{
		// ⛔ CHARACTER-FOR-CHARACTER THE KEYS' APPROVED OBSERVER LOCKOUT
		// (ASiegePlayerController's GetObserverLockoutText, M8 doc §4.1 /
		// sign-off ruling §9.2), down to the NAMESPACE and the KEY, so the two
		// dedupe into ONE localization entry and cannot drift apart. CONVENTIONS
		// §7: the assistant refuses with the SAME wording as the keys and must
		// never be more capable than them. The controller's copy is a
		// translation-unit-local static and is deliberately not exported; if that
		// wording is ever revised, this key is the grep that finds this one.
		static const FText T = NSLOCTEXT("Siegebound", "Refused_OnlineObserver", "Not available yet in online matches");
		return T;
	}
	case ESiegeAssistantReasonCode::RefusedAssistantUnavailable:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_Unavailable", "The assistant is not available.");
		return T;
	}
	case ESiegeAssistantReasonCode::RefusedBusy:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_Busy", "Still working on the last order.");
		return T;
	}
	case ESiegeAssistantReasonCode::RefusedAwaitingConfirm:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_AwaitingConfirm", "Accept or cancel the pending order first.");
		return T;
	}
	case ESiegeAssistantReasonCode::FailedParse:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_FailedParse", "I couldn't read that order.");
		return T;
	}
	case ESiegeAssistantReasonCode::FailedModelError:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_FailedModelError", "The assistant could not answer.");
		return T;
	}
	case ESiegeAssistantReasonCode::FailedTimeout:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_FailedTimeout", "The assistant took too long.");
		return T;
	}
	case ESiegeAssistantReasonCode::ConfirmPrompt:
	{
		// {Order} is DescribeCommandForPlayer's output - GAME-AUTHORED from the
		// typed command struct, ⛔ never a model-produced string.
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_ConfirmPrompt", "{Order} - accept?");
		return T;
	}
	case ESiegeAssistantReasonCode::Cancelled:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_Cancelled", "Order cancelled.");
		return T;
	}
	case ESiegeAssistantReasonCode::Executed:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_Executed", "Ordered: {Order}");
		return T;
	}
	case ESiegeAssistantReasonCode::DeferredArmed:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_DeferredArmed", "Waiting for {Requested} {Kind}, then: {Order}");
		return T;
	}
	case ESiegeAssistantReasonCode::DeferredExpired:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_DeferredExpired", "Stopped waiting for {Kind}.");
		return T;
	}
	case ESiegeAssistantReasonCode::DeferredCancelled:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_DeferredCancelled", "Dropped the order that was waiting.");
		return T;
	}
	default:
	{
		// None, and anything a later enum member forgets to add. Empty rather
		// than a placeholder: PushMessage skips an empty template, so a missing
		// row is silent to the player and LOUD in the log, which is the right way
		// round.
		static const FText T = FText::GetEmpty();
		return T;
	}
	}
}

const FText& SiegeAssistantStateLabel(ESiegeAssistantState InState)
{
	// PLAYER TEXT, so it lives here rather than beside the log helper. ⛔ DISPLAY
	// ONLY: TASK-444's widget shows the state byte and draws no conclusion from
	// it, and nothing may key behaviour off this string either.
	switch (InState)
	{
	case ESiegeAssistantState::Composing:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_State_Composing", "Ready");
		return T;
	}
	case ESiegeAssistantState::Thinking:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_State_Thinking", "Thinking...");
		return T;
	}
	case ESiegeAssistantState::AwaitConfirm:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_State_AwaitConfirm", "Accept or cancel");
		return T;
	}
	case ESiegeAssistantState::Clarify:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_State_Clarify", "Waiting for your answer");
		return T;
	}
	case ESiegeAssistantState::Deferred:
	{
		static const FText T = NSLOCTEXT("Siegebound", "Assistant_State_Deferred", "Waiting");
		return T;
	}
	case ESiegeAssistantState::Idle:
	case ESiegeAssistantState::Failed:
	default:
	{
		// Idle and Failed carry NO label on purpose. Idle is the resting state
		// and needs no chrome; Failed has already said what happened through a
		// reason-code template, and a second, vaguer word beside it would only
		// compete with the sentence that was specific.
		static const FText T = FText::GetEmpty();
		return T;
	}
	}
}

// ---------------------------------------------------------------------------
// CONSTRUCTION / LIFETIME
// ---------------------------------------------------------------------------

USiegeAssistantComponent::USiegeAssistantComponent()
{
	// ⛔ NEVER TICKS. CONVENTIONS §4: the snapshot is captured PER SENTENCE and
	// never per frame, and the cheapest way to guarantee that is to have no tick
	// to put it in.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// ⚖️ M8: NOT replicated, and deliberately not made so. "adds no replicated
	// property, no new replicated class, no new relevancy tier." Inference and the
	// whole conversation are client-local; the ORDERS this produces go through the
	// same authoritative controller APIs the keys use.

	// Brace-initialised - CONVENTIONS' MOST-VEXING-PARSE law: a hoisted path
	// constant is exactly the shape that turns a parenthesised initialiser into a
	// function declaration.
	VocabularyAsset = TSoftObjectPtr<USiegeAssistantVocabulary>{ FSoftObjectPath(SiegeAssistantComponentInternal::VocabularyAssetPath) };
}

void USiegeAssistantComponent::BeginPlay()
{
	Super::BeginPlay();

	// ⚠️ EVERYTHING HERE IS CHEAP AND NONE OF IT CAN BLOCK MATCH START (§2). The
	// snapshot object is an empty UObject, the vocabulary is a small UDataAsset,
	// and Zone A is a string build.
	//
	// ⛔ NO MODEL IS TOUCHED, AND THE MECHANISM IS STATED CORRECTLY HERE BECAUSE
	// THE OLD ONE WAS FALSE (§22; QA-464 WARN-3). This function used to claim the
	// component "holds no reference into the SiegeLlama plugin at all" - it never
	// did: NotifyConsoleOpened, DispatchTurnToModel and AbortInFlightRequest all
	// call ResolveLlamaSubsystem, and the EnsureStaticPrefixRegistered call below
	// adds a fourth. ⚖️ THE CONCLUSION SURVIVES; ONLY THE REASON CHANGES, which is
	// the hardest stale claim to catch. The real mechanism is threefold and each
	// part is checkable: (i) the subsystem pointer is RESOLVED LIVE AND NEVER
	// CACHED, so there is no reference to dangle; (ii) every call through it is
	// null-tolerant - a null subsystem is a silent no-op; and (iii) the plugin's
	// own entry points are non-blocking (RequestCompletion "returns false
	// IMMEDIATELY when !IsReady()"), so a missing GGUF costs a map read and a bool.
	// ⇒ "A missing GGUF never blocks match start" is still true, for reasons that
	// still exist.
	EnsureSnapshot();

	// Pre-warm Zone A so the FIRST typed sentence does not pay for it, and so its
	// size is on the record before any capture happens.
	const FString& ZoneA = GetCachedZoneA();

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("USiegeAssistantComponent ready on '%s' (state %s). zoneA_chars=%d, vocabulary=%s. This component NEVER ticks and never caches the subsystem pointer; every plugin call is null-tolerant and non-blocking, so a model fault can only ever disable the console."),
		*GetNameSafe(GetOwner()), SiegeAssistantComponentInternal::StateName(State), ZoneA.Len(),
		ResolvedVocabulary ? *GetNameSafe(ResolvedVocabulary) : TEXT("none"));

	// ⚠️ ARM THE §8 BUDGET GUARDRAIL AT MATCH START (TASK-463 item 3). ⛔ THIS
	// CHANGES **WHEN** THE PREFIX IS REGISTERED AND NOTHING ABOUT **WHAT** IS
	// CHECKED - EnsureStaticPrefixRegistered is called unmodified, and the string
	// it registers is the same GetCachedZoneA() every other caller uses.
	//
	// The observed problem (TASK-447, first live run): registration happened first
	// at console OPEN, so a match in which the console was never opened printed
	// "LLM_BUDGET ASSERTION IS INCOMPLETE -- NO STATIC PREFIX HAS BEEN REGISTERED"
	// and left both snapshot budgets inert for its whole length.
	//
	// ⚖️ WHAT THIS ACTUALLY BUYS, STATED HONESTLY RATHER THAN OVERSOLD - because
	// the IsReady() gate inside means it is a DELIBERATE NO-OP on a cold start:
	//   · MATCH 2+ IN A SESSION, and any level travel or restart: USiegeLlamaSubsystem
	//     is a GameInstance subsystem and SURVIVES travel, so the model is ALREADY
	//     loaded when this runs and the prefix arms HERE - before anything can open a
	//     console. That window closes completely.
	//   · A COLD FIRST MATCH: the model is still loading (measured 1874 ms, async on a
	//     below-normal worker), IsReady() is false, and this returns silently without
	//     latching - exactly as designed, since ApplyPendingStaticPrefix DISCARDS a
	//     prefix that arrives before the weights land. The existing console-open and
	//     dispatch retries still do the work.
	// ⛔ THAT RESIDUAL WINDOW IS NOT CLOSABLE FROM THIS LANE AND IT IS ALSO
	// UNREACHABLE, WHICH IS WHY NO POLL/TIMER/TICK IS ADDED HERE: while !IsReady()
	// the plugin refuses to hold a prefix at all, AND RequestCompletion returns
	// false immediately - so there is no interleaving in which an unguarded request
	// reaches llama_decode during it. Adding a tick to chase it would break this
	// component's "never ticks" law (§4) to buy nothing.
	EnsureStaticPrefixRegistered(ResolveLlamaSubsystem());
}

void USiegeAssistantComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// No dangling work into a component that is going away: abort anything in
	// flight, drop any latch, tear down any preview, and let go of the console
	// widget. All four are safe to call when there is nothing to do (that is part
	// of each seam's contract).
	//
	// ⚠️ DetachConsoleWidget IS NOT OPTIONAL HERE. The widget's lifetime is the
	// CONTROLLER's, not this component's, so it can outlive us - and a surviving
	// binding into a destroyed component is the silent kind of dangling.
	AbortInFlightRequest();
	ClearDeferredIntent();
	ClearConfirmPreview();
	ClearPendingIntent();
	DetachConsoleWidget();

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// CONSOLE LIFECYCLE
// ---------------------------------------------------------------------------

void USiegeAssistantComponent::NotifyConsoleOpened()
{
	bConsoleOpen = true;

	if (bAssistantFaulted)
	{
		// The latch disables the console AND NOTHING ELSE. Say so once per open,
		// so the player is never left typing into something that will not answer.
		PushMessage(ESiegeAssistantReasonCode::RefusedAssistantUnavailable);
		return;
	}

	if (State == ESiegeAssistantState::Idle)
	{
		SetState(ESiegeAssistantState::Composing);
	}

	// Every other state is PRESERVED. A latched Deferred intent in particular
	// must survive a console that was shut and reopened: the latch is a promise
	// the game made, not a screen the player was looking at.

	// ⚠️ THE EARLY HALF OF THE TASK-455 STATIC-PREFIX WIRING, AND "EARLY" IS THE
	// ENTIRE REASON IT IS HERE RATHER THAN ONLY AT DISPATCH. The plugin's CHAR
	// pre-filter reads the APPLIED prefix on the game thread inside
	// RequestCompletion, so a prefix registered in the same frame as the request is
	// too late for it - the worker has not tokenized it yet. Registering when the
	// console OPENS gives the worker the seconds the player spends typing, which is
	// what makes the pre-filter live on the FIRST sentence instead of the second.
	//
	// ⛔ IT SITS AFTER EVERY FSM DECISION IN THIS FUNCTION AND CHANGES NONE OF
	// THEM. It reads no state, sets no state, and returns void; the faulted path
	// above has already returned, so a disabled console never registers anything.
	// Nothing about the console's behaviour is conditional on it.
	EnsureStaticPrefixRegistered(ResolveLlamaSubsystem());
}

void USiegeAssistantComponent::NotifyConsoleClosed()
{
	bConsoleOpen = false;

	if (State == ESiegeAssistantState::Deferred)
	{
		// ⚠️ THE ONE STATE A CLOSE DOES NOT END. See NotifyConsoleOpened.
		UE_LOG(LogSiegeAssistant, Verbose, TEXT("Console closed with a deferred intent latched - the latch is PRESERVED."));
		return;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// ⭐ DID THIS CLOSE DESTROY AN ORDER THE PLAYER WAS BEING ASKED ABOUT?
	// CONVENTIONS AS-§6 RULING A-2, the 2026-08-04 close-is-a-cancel amendment.
	// ─────────────────────────────────────────────────────────────────────────
	// ⛔ SAMPLED BEFORE THE DISCARD RUNS, because the discard is what destroys
	// the evidence: three lines below, State is Idle and PendingArgs is empty, so
	// asking afterwards can only ever answer "nothing happened".
	//
	// ⚠️ AwaitConfirm IS THE WHOLE TEST, and it is not a proxy for one - it is
	// the state's definition. EnterAwaitConfirm is the only way in and it is
	// reached with PendingArgs already filled, so "State == AwaitConfirm" and
	// "there is a parsed order the player has been shown and not yet accepted"
	// are the same statement. Every other state is silent for a REASON, not by
	// omission:
	//   · Deferred      - returned above; the latch SURVIVES a close, so nothing
	//                     was discarded and a "cancelled" line would be a lie.
	//   · Thinking      - the model has not answered yet, so there is no ORDER to
	//                     report; the abort stays silent exactly as it shipped.
	//                     ⚠️ NOTE THIS DIFFERS FROM CancelPressed(), which does
	//                     print here - AS-§6 A-2 names the AwaitConfirm→Idle path
	//                     ONLY, and that narrowing is deliberate, not an oversight.
	//   · Idle/Composing/Failed - nothing pending. ⛔ A "cancelled" line on every
	//                     close would be noise on the common case and would teach
	//                     the player to stop reading the transcript.
	const bool bDiscardedPendingOrder = (State == ESiegeAssistantState::AwaitConfirm);

	switch (State)
	{
	case ESiegeAssistantState::Thinking:
		AbortInFlightRequest();
		break;

	case ESiegeAssistantState::AwaitConfirm:
		// Closing the console is a discard, and a discard leaves NOTHING
		// partially executed - the preview comes down with it.
		ClearConfirmPreview();
		break;

	default:
		break;
	}

	ClearPendingIntent();
	SetState(ESiegeAssistantState::Idle);

	// ⛔ LAST, AND THE ORDERING IS THE CONTRACT: the player is told the order is
	// gone only once it actually IS gone - preview down, intent cleared, state
	// settled. A line pushed earlier would be a promise the FSM had not kept yet,
	// and any handler of OnAssistantMessage that re-entered this component would
	// observe a half-torn-down confirm.
	if (bDiscardedPendingOrder)
	{
		// ⛔ THE EXISTING GAME-AUTHORED TEMPLATE, BYTE-FOR-BYTE THE CALL
		// CancelPressed() MAKES (§3: every sentence the player reads comes from
		// the reason-code table, never from a call site). ⛔ Do NOT author a
		// close-specific string here: "the order was discarded" is the same fact
		// however the player expressed it, and a second wording for it would be a
		// second source of truth for one sentence.
		//
		// ⭐ WHY THIS EXISTS AT ALL (AS-§6 A-2, amended): Jonathan's ruling
		// removes the Cancel button, so closing the box IS the cancel gesture.
		// The FSM already discarded correctly - it just did it SILENTLY, and an
		// order that vanishes with nothing on screen reads as "the assistant ate
		// my order", which is a trust failure rather than a UI nit.
		//
		// ⚠️ THE WIDGET IS ALREADY HIDDEN WHEN THIS RUNS (CloseConsole applies its
		// visual state before broadcasting OnConsoleOpenChanged(false)), so the
		// line lands in the transcript BUFFER and is read on the next open. That
		// is a property of the surface, not of this decision: the transcript is
		// the only game-authored channel this component owns, and ⛔ AS-§6 A-2
		// forbids solving it by moving the decision into the widget. If the line
		// needs to be visible at the instant of the close, that is a NEW on-screen
		// surface and a NEW task - it is NOT a reason to broadcast from the widget.
		PushMessage(ESiegeAssistantReasonCode::Cancelled);
	}
}

// ---------------------------------------------------------------------------
// THE ONE ENTRY POINT FOR A TYPED SENTENCE
// ---------------------------------------------------------------------------

void USiegeAssistantComponent::SubmitUtterance(const FString& RawUtterance)
{
	// ── 1. EMPTY INPUT - ignored, and with ZERO side effects ──────────────────
	// ⚠️ IT IS FIRST, AND THE ORDER IS THE POINT. Pressing Enter on an empty box
	// is not a statement: it must not print a refusal, must not consume a turn,
	// and - the one that actually bit - must not reach the Deferred admission
	// below and drop a latched order the player never asked to drop.
	const FString Utterance = RawUtterance.TrimStartAndEnd();
	if (Utterance.IsEmpty())
	{
		UE_LOG(LogSiegeAssistant, Verbose, TEXT("Empty utterance ignored (no turn started, no model call, nothing changed)."));
		return;
	}

	// ── 2. THE FAULT LATCH (§2) ───────────────────────────────────────────────
	if (bAssistantFaulted)
	{
		PushMessage(ESiegeAssistantReasonCode::RefusedAssistantUnavailable);
		return;
	}

	// ── 3. AUTHORITY (§7) ─────────────────────────────────────────────────────
	// V1 is host/standalone only - FORCED, not chosen - and the refusal uses the
	// KEYS' approved wording, character-for-character. ⛔ The assistant must never
	// be more capable than the keyboard, so it refuses exactly where they do.
	const AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("SubmitUtterance refused - P1 client observer posture (M8 doc 4.1). The assistant refuses exactly where the keys do."));
		PushMessage(ESiegeAssistantReasonCode::RefusedNoAuthority);
		return;
	}

	// ── 4. ADMISSION (the table on the class comment) ─────────────────────────
	switch (State)
	{
	case ESiegeAssistantState::Thinking:
		// QUEUE DEPTH 1 IS THE DESIGN, NOT A BUG (CONVENTIONS §9: a second
		// RequestCompletion returns false). Refusing here rather than at the
		// subsystem keeps the player's answer immediate and honest.
		PushMessage(ESiegeAssistantReasonCode::RefusedBusy);
		return;

	case ESiegeAssistantState::AwaitConfirm:
		PushMessage(ESiegeAssistantReasonCode::RefusedAwaitingConfirm);
		return;

	case ESiegeAssistantState::Deferred:
		// A player who types a fresh order while one is waiting means the fresh
		// one. Drop the latch LOUDLY - a silently discarded promise is worse than
		// a refused one.
		UE_LOG(LogSiegeAssistant, Log, TEXT("A new utterance arrived while a deferred intent was latched - dropping the latch."));
		ClearDeferredIntent();
		ClearPendingIntent();
		// Leave Deferred BEFORE the turn runs, so the FSM is never mid-turn in a
		// state whose latch it has already dropped.
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::DeferredCancelled);
		break;

	default:
		break;
	}

	// ── 5. THE SHORT-CIRCUIT - the cheapest correct answer never enters the model
	if (TryShortCircuitClarification(Utterance))
	{
		return;
	}

	// ── 6. BEGIN THE TURN ─────────────────────────────────────────────────────
	BeginTurn();

	// ── 7. THE SNAPSHOT - once per SENTENCE, never per tick (§4) ──────────────
	if (!CaptureTurnSnapshot())
	{
		// A world or a team we cannot resolve is a refusal, never a guess: a
		// wrong ordering team surveys the wrong army and every answer downstream
		// of it is confidently wrong.
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::RefusedAssistantUnavailable);
		return;
	}

	// ── 8. COMPOSE - the FSM's job, never the executor's (mechanism #2) ───────
	// ⚠️ ComposeTurnPrompt reads BuildPendingLine(), which is how a clarification
	// carries context WITHOUT the model ever seeing its own previous output.
	const FString Prompt = ComposeTurnPrompt(Utterance);
	const FString Grammar = ComposeTurnGrammar();

	// ── 9. DISPATCH - ONE model call for this turn ────────────────────────────
	if (bModelDispatchedThisTurn)
	{
		// Unreachable through this function (BeginTurn just cleared the flag) and
		// kept anyway: it is the invariant §1 rests on, and an invariant that is
		// only true by inspection is a comment.
		UE_LOG(LogSiegeAssistant, Error,
			TEXT("REFUSED a SECOND model dispatch inside turn %d. One utterance + one snapshot -> ONE constrained JSON (CONVENTIONS 1)."), TurnId);
		return;
	}

	bModelDispatchedThisTurn = true;
	SetState(ESiegeAssistantState::Thinking);

	if (!DispatchTurnToModel(Prompt, Grammar))
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Turn %d was not dispatched (prompt %d chars, grammar %d chars). Nothing is in flight; the console stays usable."),
			TurnId, Prompt.Len(), Grammar.Len());

		bModelDispatchedThisTurn = false;
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::RefusedAssistantUnavailable);
	}
}

void USiegeAssistantComponent::ConfirmPressed()
{
	if (State != ESiegeAssistantState::AwaitConfirm)
	{
		UE_LOG(LogSiegeAssistant, Verbose, TEXT("ConfirmPressed ignored in state %s."), SiegeAssistantComponentInternal::StateName(State));
		return;
	}

	// The preview comes down FIRST, whichever way the execution goes, so no ghost
	// circle can outlive the decision it belonged to.
	ClearConfirmPreview();

	// ⚠️ TASK-443 EXTRACTED THE TAIL INTO ExecuteAndReport() AND THE BEHAVIOUR IS
	// UNCHANGED, STATEMENT FOR STATEMENT. It is not a tidy-up: the toggle-OFF path
	// runs the SAME tail, and two copies of it could drift apart on exactly the
	// claim §5 requires to stay true ("every check that runs with the toggle ON
	// runs with it OFF"). One function with two callers makes that structural.
	ExecuteAndReport();
}

void USiegeAssistantComponent::CancelPressed()
{
	const bool bHasSomethingPending =
		PendingReason != ESiegeAssistantReasonCode::None ||
		PendingArgs.Command.Intent != ESiegeAssistantIntent::None;

	switch (State)
	{
	case ESiegeAssistantState::Thinking:
		AbortInFlightRequest();
		break;

	case ESiegeAssistantState::AwaitConfirm:
		// ⚠️ NOTHING PARTIALLY EXECUTED: the order was never executed, and the
		// preview is the only thing on the ground.
		ClearConfirmPreview();
		break;

	case ESiegeAssistantState::Deferred:
		ClearDeferredIntent();
		break;

	case ESiegeAssistantState::Idle:
		return;

	default:
		if (!bHasSomethingPending)
		{
			// Cancel with nothing to cancel says nothing. A refusal message for a
			// no-op teaches the player to ignore the transcript.
			return;
		}
		break;
	}

	ClearPendingIntent();
	SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
	PushMessage(ESiegeAssistantReasonCode::Cancelled);
}

// ---------------------------------------------------------------------------
// THE MODEL COMPLETION - parse, route, and NEVER keep a word of it
// ---------------------------------------------------------------------------

void USiegeAssistantComponent::HandleModelCompletion(int32 InTurnId, bool bSuccess, const FString& Output, const FString& Error)
{
	// ⛔ THE STALE-COMPLETION GUARD (mechanism #4). A completion for a turn that
	// is no longer current is DROPPED, never applied: it is the answer to a
	// sentence the player has already replaced or cancelled, and letting two
	// answers meet is precisely the multi-turn shape §1 forbids.
	if (InTurnId != TurnId)
	{
		UE_LOG(LogSiegeAssistant, Verbose,
			TEXT("Dropped a completion for turn %d (current turn is %d). A superseded answer never reaches the executor."), InTurnId, TurnId);
		return;
	}

	if (State != ESiegeAssistantState::Thinking)
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Completion for turn %d arrived in state %s, not Thinking. Dropped."), InTurnId, SiegeAssistantComponentInternal::StateName(State));
		return;
	}

	if (!bSuccess)
	{
		// ⚠️ THE SUBSYSTEM'S ERROR IS LOGGED AND NEVER SHOWN (§3). A backend
		// string is not a game-authored template, and the player gets one.
		UE_LOG(LogSiegeAssistant, Warning, TEXT("Turn %d failed in the subsystem: %s"), InTurnId, *Error);
		EnterFailed(ESiegeAssistantReasonCode::FailedModelError);
		return;
	}

	// ⛔ `Output` IS A LOCAL AND STAYS ONE (mechanism #1). Nothing below writes it
	// to a member, and there is no member it could be written to.
	FSiegeAssistantCommand Command;
	FString ParseError;

	// ⭐ THE RAW MODEL OUTPUT - THE ONE FAILURE CLASS THAT LEFT NO ARTIFACT
	// (AS-§21.8). EVERY OTHER OUTCOME ALREADY NAMES ITSELF BELOW: a decline logs
	// `ask=`, a parse failure logs its reason and detail, the selection invariant
	// logs its error, a shortfall logs its arithmetic, a non-orderable kind logs
	// the kind. The ONE class with nothing to read afterwards was "well-formed,
	// passed every machine check, and means something nobody asked for" - which is
	// exactly the class AS-§20.1 calls an automatic QA FAIL and AS-§20.4 says must
	// be FOUND, NOT GUESSED. This line is the finding instrument.
	//
	// ⛔ VERBOSITY IS `Log`, AND BOTH ALTERNATIVES ARE REFUSED ON PURPOSE so they
	// are not "improved" in later: NOT `Verbose`, because the report arrives after
	// the session and nobody re-runs a lost turn at a raised verbosity; NOT
	// `Warning`, because this fires on the HAPPY path and the automation runner
	// reads Warning as failure.
	//
	// ⛔ IT SITS BEFORE THE PARSE CALL SO IT FIRES WHETHER OR NOT THE PARSE
	// SUCCEEDS - a malformed emission is the case most in need of its own bytes,
	// and a line that only printed on success would be missing exactly then.
	//
	// ⚠️ THE LENGTH IS PRINTED BESIDE THE TEXT, AND THE TEXT IS BRACKETED, so an
	// EMPTY completion (`(0 chars): []`) and a TRUNCATED one are distinguishable
	// from a merely malformed one, and trailing whitespace or a stray newline is
	// visible as a gap before the `]`. ⛔ THE STRING IS PRINTED IN FULL AND IS
	// NEVER ELIDED: the tail is precisely where a malformed or invented field
	// would sit, so an ellipsis here would drop the evidence this line exists to
	// capture. The grammar bounds the shape and the sampler bounds the length.
	//
	// ⛔ THIS IS AN OBSERVATION AND NOTHING ELSE. It reads a parameter that already
	// exists and writes nothing: no branch, no early return, no member, no state
	// read or write, no player-facing surface (§3 - the player gets game-authored
	// templates, never model bytes), and ZERO prompt bytes. ⚖️ PRINTING IS NOT
	// KEEPING: AS-§1's "never keep a word of it" bans a word of model output
	// SURVIVING INTO ANOTHER TURN, and a log line is write-only and one-directional
	// - ComposeTurnPrompt() reads Zone A/B/C builders and has no route to a log,
	// so nothing here can ever re-enter a prompt. `Output` is still a local and
	// still dies with the frame.
	UE_LOG(LogSiegeAssistant, Log, TEXT("Turn %d: RAW MODEL OUTPUT (%d chars): [%s]"), InTurnId, Output.Len(), *Output);

	if (ParseSiegeAssistantCommand(Output, Command, ParseError))
	{
		// The index-alignment / cap / uniqueness invariant, re-asserted on
		// something we did not build ourselves. ⚠️ It runs with the confirm toggle
		// OFF exactly as it runs with it ON: the toggle removes a HUMAN REVIEW
		// STEP and NEVER A MACHINE CHECK ("Settings screen..." §5).
		//
		// ⛔⭐ THE FOURTH AND FIFTH ARGUMENTS ARE PASSED DELIBERATELY AND MUST NEVER
		// BE DROPPED (TASK-518 §5 / TASK-522 for the fourth; TASK-548 / AS-§21.9 for
		// the fifth). SiegeAssistantValidateSelection's ExcludeKinds and RegionPlace
		// parameters are both TRAILING AND DEFAULTED — a shape forced by file
		// ownership, not chosen — so this call COMPILES CLEANLY WITHOUT EITHER and
		// then silently validates NOTHING about the dropped one: the cap, the
		// no-repeats rule and the "never a selection AND an exclusion" rule would all
		// report SAFE while checking nothing. That is a guardrail reporting safe,
		// which is the one failure shape this feature's law names by hand. ⛔ Every
		// caller holding a whole FSiegeAssistantCommand passes BOTH
		// Command.ExcludeKinds AND Command.RegionPlace; the defaults are for the
		// arrays-only call sites (the automation suite), never permission to skip the
		// check. ⚠️ Removing either argument would raise no compiler diagnostic
		// anywhere — the ONLY thing standing behind them is this comment and the QA
		// grep it invites.
		//
		// ⚠️ THE FIFTH WAS ADDED HERE BY TASK-548 AND THE GAP IT CLOSED IS RECORDED
		// RATHER THAN QUIETLY FIXED: between TASK-545 landing the parameter and this
		// line, THIS CALL SITE — the receive-side gate, the last machine check
		// standing between a well-formed model emission and the executor — validated
		// nothing whatsoever about the region. It compiled, linked and reported
		// SUCCESS the whole time. That is the identical shape as the ExcludeKinds
		// hazard one batch earlier, which is exactly why AS-§21.9 forbids a SIXTH
		// default and pins an FSiegeAssistantCommand overload as the next shape.
		FString SelectionError;
		if (!SiegeAssistantValidateSelection(Command.Kinds, Command.Counts, SelectionError, Command.ExcludeKinds, Command.RegionPlace))
		{
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Turn %d parsed but violated the selection invariant (%s). ⛔ A mismatch is a failure, NEVER a silent truncation."),
				InTurnId, *SelectionError);
			EnterFailed(ESiegeAssistantReasonCode::FailedParse);
			return;
		}

		// THE SHORTFALL - arithmetic the game already has in hand, so it costs no
		// second model call.
		int32 ShortfallIndex = INDEX_NONE;
		int32 Available = 0;
		if (FindShortfall(Command, ShortfallIndex, Available))
		{
			FSiegeAssistantMessageArgs ShortfallArgs;
			ShortfallArgs.Command = Command;
			ShortfallArgs.Requested = Command.Counts[ShortfallIndex];
			ShortfallArgs.Available = Available;
			ShortfallArgs.Kind = Command.Kinds[ShortfallIndex];

			EnterClarify(ESiegeAssistantReasonCode::ShortfallCount, ShortfallArgs);
			PendingShortfallIndex = ShortfallIndex;

			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Turn %d: SHORTFALL answered game-side with NO model call - %d %s requested, %d can take that order."),
				InTurnId, ShortfallArgs.Requested, *ShortfallArgs.Kind.ToString(), Available);
			return;
		}

		// Hand the parsed order to the executor lane. ⛔ THE GUARD RUNS IN THERE,
		// BEFORE EXECUTION, ALWAYS, AND INDEPENDENTLY OF THE MODEL.
		PendingArgs.Command = Command;
		PendingArgs.Requested = 0;
		PendingArgs.Available = 0;
		PendingArgs.Kind = NAME_None;
		PendingReason = ESiegeAssistantReasonCode::None;
		PendingShortfallIndex = INDEX_NONE;

		RouteParsedCommand(Command);
		return;
	}

	// A well-formed QUESTION returns false with "ask:<code>" - not a failure of
	// the model but a DECLINE, and the FSM routes it to a conversation rather than
	// to an error (CONVENTIONS §9 / SiegeAssistantReason::Ask).
	const FString ReasonCodeOnly = SiegeAssistantReasonCode(ParseError);
	if (ReasonCodeOnly == SiegeAssistantReason::Ask)
	{
		const FString AskCode = ParseError.Mid(ReasonCodeOnly.Len() + 1);

		ESiegeAssistantReasonCode ClarifyCode = ESiegeAssistantReasonCode::AskUnsupported;
		if (AskCode == SiegeAssistantAsk::WhichUnit)
		{
			ClarifyCode = ESiegeAssistantReasonCode::AskWhichUnit;
		}
		else if (AskCode == SiegeAssistantAsk::WhichPlace)
		{
			ClarifyCode = ESiegeAssistantReasonCode::AskWhichPlace;
		}
		else if (AskCode == SiegeAssistantAsk::HowMany)
		{
			ClarifyCode = ESiegeAssistantReasonCode::AskHowMany;
		}
		else if (AskCode == SiegeAssistantAsk::WhichIntent)
		{
			ClarifyCode = ESiegeAssistantReasonCode::AskWhichIntent;
		}
		// else: unsupported, or an ask code the parser accepted and this switch
		// has not learned. AskUnsupported is the safe default - it refuses rather
		// than inventing a question, and the log below names the code.

		UE_LOG(LogSiegeAssistant, Log, TEXT("Turn %d: the model declined with ask=%s."), InTurnId, *AskCode);

		// ⚠️ NO PARTIAL COMMAND EXISTS ON THIS PATH, so the pending line carries
		// only the problem clause. That is a real limit of a decline-without-a-
		// command and it is stated rather than papered over: the next turn is a
		// fresh single-turn call and the model will not remember asking.
		FSiegeAssistantMessageArgs AskArgs;
		EnterClarify(ClarifyCode, AskArgs);
		PendingShortfallIndex = INDEX_NONE;
		return;
	}

	UE_LOG(LogSiegeAssistant, Warning, TEXT("Turn %d did not parse: %s"), InTurnId, *ParseError);
	EnterFailed(ESiegeAssistantReasonCode::FailedParse);
}

void USiegeAssistantComponent::MarkAssistantFaulted(ESiegeAssistantReasonCode Reason)
{
	if (bAssistantFaulted)
	{
		return;
	}

	bAssistantFaulted = true;

	// ⚠️ ONE LINE, AT ERROR, ONCE PER SESSION - and it states the blast radius,
	// because the whole point of the latch is that the blast radius is small.
	UE_LOG(LogSiegeAssistant, Error,
		TEXT("ASSISTANT FAULTED (reason code %d) - the console is disabled for this session and NOTHING ELSE changes. Every keyboard command keeps working byte-identically; match start was never blocked (CONVENTIONS 2)."),
		static_cast<int32>(Reason));

	if (State == ESiegeAssistantState::Thinking)
	{
		AbortInFlightRequest();
	}
	if (State == ESiegeAssistantState::AwaitConfirm)
	{
		ClearConfirmPreview();
	}
	ClearDeferredIntent();
	ClearPendingIntent();

	SetState(ESiegeAssistantState::Failed);

	const ESiegeAssistantReasonCode ShownReason =
		(Reason == ESiegeAssistantReasonCode::None) ? ESiegeAssistantReasonCode::RefusedAssistantUnavailable : Reason;
	PushMessage(ShownReason);

	// ⚠️ THE LATCH'S ENTIRE BLAST RADIUS, expressed as one event: the console
	// goes unavailable and nothing else moves. The reason travels as a
	// GAME-AUTHORED template (§3) so the widget's disabled state can show the
	// same sentence the transcript did.
	OnAssistantAvailabilityChanged.Broadcast(false, SiegeAssistantReasonTemplate(ShownReason).ToString());
}

// ---------------------------------------------------------------------------
// THE PLAYER-FACING SURFACE (§3)
// ---------------------------------------------------------------------------

void USiegeAssistantComponent::PushMessage(ESiegeAssistantReasonCode Code, const FSiegeAssistantMessageArgs& Args)
{
	const FText& MessageTemplate = SiegeAssistantReasonTemplate(Code);
	if (MessageTemplate.IsEmpty())
	{
		// Silent to the player, LOUD in the log - a template row that was never
		// authored must not become a blank line in the transcript.
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("No player-facing template for reason code %d. Nothing was shown. Every outcome the player can see needs a row in SiegeAssistantReasonTemplate (CONVENTIONS 3)."),
			static_cast<int32>(Code));
		return;
	}

	FFormatNamedArguments FormatArgs;
	FormatArgs.Add(TEXT("Requested"), FText::AsNumber(Args.Requested));
	FormatArgs.Add(TEXT("Available"), FText::AsNumber(Args.Available));
	FormatArgs.Add(TEXT("Kind"), FText::FromName(Args.Kind));
	FormatArgs.Add(TEXT("Order"), DescribeCommandForPlayer(Args.Command));

	// ⚠️ THE ORDER'S VERB, so a template that names an action names the one the
	// PLAYER asked for and not one a template author happened to type. Its only
	// input is Args.Command - a struct pinned to uint8/int32/FName (§3), so there
	// is no slot here through which model text could reach a sentence - and it
	// comes from the SAME IntentDisplayText switch that builds {Order}, so the two
	// can never disagree about the word.
	// ⚠️ ADDED FOR EVERY CODE RATHER THAN AT ONE CALL SITE, WHICH COSTS NOTHING:
	// FText::Format ignores an argument the pattern does not name, exactly as this
	// function has always handed {Requested}/{Kind}/{Available}/{Order} to rows
	// like "Order cancelled." that name none of them.
	FormatArgs.Add(TEXT("Intent"), SiegeAssistantComponentInternal::IntentDisplayText(Args.Command.Intent));

	LastMessage = FText::Format(MessageTemplate, FormatArgs).ToString();

	UE_LOG(LogSiegeAssistant, Log, TEXT("[assistant] %s"), *LastMessage);

	// ⚠️ AN EVENT, NOT A VALUE: it fires on every message, including a repeat of
	// the same sentence. The delegate law's "never on a no-op write" governs
	// VALUES, and two identical refusals are two things the player must see.
	OnAssistantMessage.Broadcast(LastMessage);
}

void USiegeAssistantComponent::PushMessage(ESiegeAssistantReasonCode Code)
{
	const FSiegeAssistantMessageArgs EmptyArgs;
	PushMessage(Code, EmptyArgs);
}

FText USiegeAssistantComponent::DescribeCommandForPlayer(const FSiegeAssistantCommand& Command) const
{
	// ⛔ GAME-AUTHORED, FROM SYMBOLS ONLY. Its only input is a struct pinned to
	// uint8 / int32 / FName, so there is no string here that the model wrote.
	//
	// ⚠️ THE UNIT SYMBOL IS PRINTED RAW ("footman", not "Footmen"), DELIBERATELY.
	// A display-name table would be a SECOND source of truth for a name DT_Cards
	// already owns, and the roster line the player can see in the log prints the
	// same symbol. A prettifier belongs with a card-name lookup, not here.
	if (Command.Intent == ESiegeAssistantIntent::None)
	{
		return FText::GetEmpty();
	}

	FString Selection;
	const int32 SelectionNum = FMath::Min(Command.Kinds.Num(), Command.Counts.Num());
	for (int32 Index = 0; Index < SelectionNum; ++Index)
	{
		if (Index > 0)
		{
			Selection += TEXT(", ");
		}

		if (Command.Counts[Index] == 0)
		{
			// 0 == "all" (CONVENTIONS §9). Printing "0 footman" would be a
			// valid-shaped wrong sentence about a valid command.
			Selection += FString::Printf(TEXT("all %s"), *Command.Kinds[Index].ToString());
		}
		else
		{
			Selection += FString::Printf(TEXT("%d %s"), Command.Counts[Index], *Command.Kinds[Index].ToString());
		}
	}

	// ⛔⭐ THE EXCEPTION IS PART OF THE ORDER, SO IT IS PART OF THE SENTENCE THE
	// PLAYER IS ASKED TO ACCEPT (TASK-522). Without this, "send everyone except
	// the miners to mid" renders as "Send (mid) - accept?" - a confirm prompt that
	// DESCRIBES A DIFFERENT ORDER FROM THE ONE THAT WILL EXECUTE, which is exactly
	// the defect the confirm step exists to prevent, and the same sentence is
	// reused by the Executed line and by the deferred "waiting for..." line.
	//
	// ⚠️ THE GHOST CIRCLES CANNOT CARRY THIS AND IT IS SAID HERE RATHER THAN
	// ASSUMED: SpawnConfirmPreview draws TWO PLACE DECALS at the resolved
	// destination and nothing per-unit, so no preview geometry depends on WHICH
	// units were selected - the exclusion is invisible on the ground either way.
	// ⇒ This line IS the unit-facing half of the review. (An "N units" count would
	// need the selector to run at prompt time, on a board that can change before
	// the player presses accept - a second, staler survey, so it is not taken.)
	//
	// ⛔ NO NEW FRAME AND NO NEW TEMPLATE ROW. The clause is built into
	// {Selection}, so the three existing frames render it unchanged and §30's
	// "one string, not three" stays true. It follows the "all %s" idiom five lines
	// above character-for-character: a raw symbol, never a display-name lookup.
	// Read aloud on every path its values can supply: "Send all except miner
	// (mid)", "Guard all except miner, cleric (mine_near)", "Follow all except
	// miner".
	if (Command.ExcludeKinds.Num() > 0)
	{
		if (Selection.IsEmpty())
		{
			Selection += TEXT("all except ");
		}
		else
		{
			// ⚠️ UNREACHABLE, AND WRITTEN ANYWAY. A positive selection AND an
			// exclusion is refused by the parser, by SiegeAssistantValidateSelection
			// and by the selector's own backstop - but a DESCRIBER that drops an
			// exception it was handed would print a reassuring sentence about an
			// order nobody gave, and this function is called from paths (LastMessage
			// re-seeding, the deferred summary) that do not re-run those gates.
			Selection += TEXT(", except ");
		}

		for (int32 Index = 0; Index < Command.ExcludeKinds.Num(); ++Index)
		{
			if (Index > 0)
			{
				Selection += TEXT(", ");
			}

			Selection += Command.ExcludeKinds[Index].ToString();
		}
	}

	// ⛔⭐ THE REGION IS PART OF THE ORDER, SO IT IS PART OF THE SENTENCE THE PLAYER
	// IS ASKED TO ACCEPT (TASK-548; the TASK-522 argument above, verbatim and for
	// the second time). Without this, "send everyone in the ancient ground to the
	// enemy castle" renders as "Send (enemy_castle)" - A CONFIRM PROMPT THAT
	// DESCRIBES A DIFFERENT ORDER FROM THE ONE THAT WILL EXECUTE, which defeats the
	// confirm step's entire purpose. It would describe moving the WHOLE ARMY while
	// the executor moves only the occupants of one ground, and the player would
	// accept the wrong sentence. The same string is reused by the Executed line and
	// by the deferred "waiting for..." line.
	//
	// ⛔ NO NEW FRAME AND NO NEW TEMPLATE ROW. The clause is built into {Selection},
	// exactly as the exception above is, so all three existing frames render it
	// unchanged and §30's "one string, not three" stays true. It follows the
	// "all except " idiom character-for-character: a raw symbol, never a
	// display-name lookup. Read aloud on every path its values can supply: "Send all
	// in ancient_ground_near (enemy_castle)", "Guard all in mid (own_castle)",
	// "Follow all in ancient_ground_far".
	//
	// ⛔ THE PRESENTATION LAYER DOES NOT REPAIR ITS INPUT (SC-§31). It describes what
	// WILL RUN - including a region that will resolve to nobody. The refusal for
	// that case belongs to the selector, which owns the arithmetic and the log; a
	// describer that quietly softened the sentence would be hiding the very order the
	// player is being asked to approve. ⚠️ And it does NOT run the selector to count
	// occupants: that would be a second, staler survey of a board that can change
	// before the player presses accept.
	//
	// ⚠️ THE GHOST CIRCLES STRUCTURALLY CANNOT CARRY THIS, AND IT IS SAID HERE
	// RATHER THAN ATTEMPTED (TASK-522's finding, which applies unchanged):
	// SpawnConfirmPreview draws TWO PLACE DECALS at the resolved DESTINATION and
	// nothing per-unit - identical geometry for 3 units or 30 - so no preview shape
	// depends on WHICH units were selected. A selection filter is invisible on the
	// ground either way. ⇒ This line IS the unit-facing half of the review.
	if (Command.RegionPlace != NAME_None)
	{
		if (Selection.IsEmpty())
		{
			Selection += FString::Printf(TEXT("all in %s"), *Command.RegionPlace.ToString());
		}
		else
		{
			// ⚠️ UNREACHABLE, AND WRITTEN ANYWAY - the same reasoning as the
			// exception's twin branch above. A region beside a selection or an
			// exclusion is refused by the parser, by SiegeAssistantValidateSelection
			// and by the selector's own backstop - but a DESCRIBER that drops a
			// filter it was handed would print a reassuring sentence about an order
			// nobody gave, and this function is called from paths (LastMessage
			// re-seeding, the deferred summary) that do not re-run those gates.
			Selection += FString::Printf(TEXT(", in %s"), *Command.RegionPlace.ToString());
		}
	}

	FFormatNamedArguments FormatArgs;
	FormatArgs.Add(TEXT("Intent"), SiegeAssistantComponentInternal::IntentDisplayText(Command.Intent));
	FormatArgs.Add(TEXT("Selection"), FText::FromString(Selection));
	FormatArgs.Add(TEXT("Place"), FText::FromName(Command.Where));

	const bool bHasSelection = !Selection.IsEmpty();
	const bool bHasPlace = Command.Where != NAME_None;

	// ⛔ THE PLACE IS PRESENTED, NEVER RELATED - CONVENTIONS §30 ("a template
	// substitutes NOUNS AND NUMBERS, never GRAMMAR"). These frames used to say
	// "{Intent} ... to {Place}", and "to" is true of only SOME of the verbs that
	// reach them: it is a DESTINATION for send / charge / fall back / rally and a
	// LOCATION for guard / ambush, so the shipped line read "Guard 8 footman TO
	// mine_near - accept?" on the CONFIRM line and the EXECUTED line of every
	// guard or ambush order (qa/TASK-464.md WARN-5, CONVENTIONS §30).
	//
	// ⛔ AND {Preposition} IS BARRED, WHICH IS WHY THE PARENTHESIS IS DOING THE
	// WORK. A substituted preposition is a LOCALIZATION FRAGMENT, not a sentence -
	// bound to the verb before it AND the noun after it, with bindings that differ
	// per verb and per language. English has no single preposition that is true of
	// both readings, so the frame must stop ASSERTING a relation it cannot verify
	// and simply PRESENT the place. A parenthetical apposition attaches to the
	// whole clause without claiming anything about the verb - so ONE string is
	// correct for all seven intents.
	//
	// ⛔ AND NOT " - ", WHICH IS THE OBVIOUS-LOOKING SEPARATOR AND IS WRONG HERE:
	// {Order} is always read INSIDE a wrapper, and Assistant_ConfirmPrompt is
	// "{Order} - accept?" - so a dash would render "Guard 8 footman - mine_near -
	// accept?", two dashes at one level with no way for a reader to tell which
	// splits the order from the question. ✅ Parentheses are SELF-DELIMITING, so
	// the frame composes into every wrapper ("{Order} - accept?", "Ordered:
	// {Order}", "Waiting for {Requested} {Kind}, then: {Order}") unambiguously.
	//
	// ⚠️ THESE THREE FRAMES ARE *NOT* THE "ONE STRING, NOT THREE" VIOLATION §30
	// FORBIDS. They fork on WHICH DATA THE COMMAND CARRIES (a selection, a place,
	// both), never on WHICH VERB carries it - and that fork is forced, because a
	// frame naming {Selection} cannot render a command that has none. ⛔ Forking
	// any ONE of them per intent is the thing that is barred.
	if (bHasSelection && bHasPlace)
	{
		return FText::Format(NSLOCTEXT("Siegebound", "Assistant_Order_SelectionPlace", "{Intent} {Selection} ({Place})"), FormatArgs);
	}
	if (bHasSelection)
	{
		// ✅ Verb-neutral already - it asserts no relation, so §30 leaves it alone.
		return FText::Format(NSLOCTEXT("Siegebound", "Assistant_Order_Selection", "{Intent} {Selection}"), FormatArgs);
	}
	if (bHasPlace)
	{
		// ⚠️ THE UNCITED HALF OF THE SAME DEFECT (§22: a finding names where the
		// reporter LOOKED, never where the defect IS). WARN-5 and §30 both quote
		// only the frame above, but this one carried the SAME "to" and IS
		// REACHABLE: `who:"all"` parses to an EMPTY selection (SiegeAssistantCommand
		// .cpp, the one cross-field check), so "guard everything at the mine" lands
		// here and used to render "Guard to mine_near".
		return FText::Format(NSLOCTEXT("Siegebound", "Assistant_Order_Place", "{Intent} ({Place})"), FormatArgs);
	}

	// The army-wide verbs (charge / fallback / rally) carry neither, by law.
	return SiegeAssistantComponentInternal::IntentDisplayText(Command.Intent);
}

// ═══════════════════════════════════════════════════════════════════════════
// THE EXECUTOR LANE (TASK-443) - THE EIGHT SEAMS, FILLED IN PLACE.
//
// ⛔ THE ONE LAW THIS SECTION EXISTS TO KEEP: THE CONFIRM TOGGLE REMOVES A HUMAN
// REVIEW STEP AND NEVER A MACHINE CHECK ("Settings screen..." §5). The toggle is
// read in exactly ONE function (IsConfirmBeforeExecuteEnabled) from exactly ONE
// call site (RouteParsedCommandInternal's LAST step), and every machine check in
// the feature lies strictly before that branch. The full ON/OFF trace, check for
// check, is on the class comment in the header.
// ═══════════════════════════════════════════════════════════════════════════

bool USiegeAssistantComponent::DispatchTurnToModel(const FString& Prompt, const FString& Grammar)
{
	USiegeLlamaSubsystem* Llama = ResolveLlamaSubsystem();
	if (!Llama)
	{
		// ⛔ NOT A SESSION FAULT. An unresolvable subsystem refuses THIS TURN; the
		// console stays usable and every key is untouched (§2).
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Turn %d: USiegeLlamaSubsystem did not resolve off the GameInstance. Refusing the turn; the console stays usable and NOTHING was executed."), TurnId);
		return false;
	}

	if (!Llama->IsReady())
	{
		// ⚠️ "NOT READY" IS NOT "BROKEN", AND CONFLATING THEM WOULD COST THE PLAYER
		// THE MATCH'S CONSOLE. The model loads asynchronously and deliberately never
		// blocks match start (§2), so a sentence typed in the first seconds arrives
		// before the weights do. The pinned §9 API exposes no way to tell "still
		// loading" from "failed forever" - so this refuses the TURN and does NOT
		// latch bAssistantFaulted. A latch here would disable the console for the
		// rest of the match as a punishment for typing early.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Turn %d: the inference subsystem is not ready (still loading, or load failed). Refusing this turn only - the console is NOT disabled."), TurnId);
		return false;
	}

	if (Llama->IsBusy())
	{
		// QUEUE DEPTH 1 IS THE DESIGN, NOT A BUG. The FSM's admission table already
		// refuses a second sentence while Thinking; this is the subsystem-side half
		// of the same rule, and reaching it means the two disagreed.
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Turn %d: the inference subsystem reports BUSY while the FSM believed it was free. Refusing - queue depth 1 is the design (CONVENTIONS 9)."), TurnId);
		return false;
	}

	// ⛔ THE BACKSTOP HALF OF THE TASK-455 STATIC-PREFIX WIRING. Its placement is
	// load-bearing in BOTH directions:
	//   · AFTER the IsBusy() guard, because SetStaticPrefix REFUSES while a request
	//     is in flight (the prefix may not move under a live KV cache). Reaching it
	//     here means the subsystem is ready and idle, so the one transient refusal
	//     reason is already excluded.
	//   · BEFORE RequestCompletion, because the worker's run loop applies a pending
	//     prefix BEFORE it picks up pending work (SiegeLlamaSubsystem.cpp's Run:
	//     ApplyPendingStaticPrefix, then the bWorkPending branch). ⇒ A prefix
	//     submitted in this very frame ALMOST ALWAYS makes the TOKEN authority live
	//     on THIS request rather than the next one.
	//
	// ⚠️ "ALMOST ALWAYS", AND THE GAP IS STATED RATHER THAN ROUNDED OFF. The worker
	// can be sitting between those two statements when the game thread submits both
	// flags, in which case the prefix applies on its NEXT iteration and this one
	// request runs unmeasured. The window is a few instructions wide and is reached
	// only on a wake, so it is vanishingly unlikely - but it is a RACE, not a
	// guarantee, and an ordering claim asserted without a lock is exactly what
	// CONVENTIONS §15 keeps catching. ✅ ITS WORST CASE IS BENIGN: the budget goes
	// live one turn later. The CONTEXT budget is enforced on every request either
	// way, so there is no unguarded path to llama_decode in any interleaving.
	//
	// ⚠️ THE CHAR PRE-FILTER IS A CERTAIN, NOT A PROBABLE, ONE-TURN LAG AND IT IS
	// STATED RATHER THAN GLOSSED: it runs on the GAME thread inside
	// RequestCompletion and reads the APPLIED prefix, which the worker cannot have
	// written yet on a same-frame registration. So a session that first registers
	// HERE has the authority live on turn 1 and the pre-filter live from turn 2.
	// NotifyConsoleOpened exists to make both windows empty in normal play.
	//
	// ⛔ IT CANNOT REFUSE OR ALTER THE TURN. It returns void, touches no FSM state
	// and no confirm-path state, and every check above and below it runs
	// identically whether it succeeds or not.
	EnsureStaticPrefixRegistered(Llama);

	// ⚠️ THE TURN ID RIDES IN THE CAPTURE, NOT IN A MEMBER. FSiegeLlamaCompletionSignature
	// is a plain (non-dynamic) delegate, so a weak lambda is the binding and
	// HandleModelCompletion is deliberately not a UFUNCTION. BindWeakLambda means a
	// completion that arrives after this component is gone is DROPPED by the engine
	// before it can touch a destroyed object - the second half of the stale-answer
	// guarantee whose first half is the turn-id check inside HandleModelCompletion.
	const int32 DispatchedTurnId = GetTurnId();

	FSiegeLlamaCompletionSignature OnDone;
	OnDone.BindWeakLambda(this,
		[this, DispatchedTurnId](bool bSuccess, const FString& Output, const FString& Error)
		{
			// ⛔ The raw model output goes STRAIGHT INTO A FUNCTION PARAMETER and is
			// never stored (mechanism #1). There is no member it could be put in.
			HandleModelCompletion(DispatchedTurnId, bSuccess, Output, Error);
		});

	// ⛔ EXACTLY ONE RequestCompletion PER TURN. The prompt and the grammar arrived
	// FINISHED (mechanism #2) - this function has no route to a zone builder, so it
	// physically cannot feed the model anything the model itself produced.
	const bool bDispatched = Llama->RequestCompletion(Prompt, Grammar, OnDone);

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Turn %d dispatched=%s (prompt %d chars, grammar %d chars). ONE utterance + ONE snapshot -> ONE constrained JSON."),
		DispatchedTurnId, bDispatched ? TEXT("yes") : TEXT("NO"), Prompt.Len(), Grammar.Len());

	return bDispatched;
}

void USiegeAssistantComponent::RouteParsedCommand(const FSiegeAssistantCommand& Command)
{
	// The seam signature TASK-442's FSM calls, preserved EXACTLY. Only the
	// deferred-intent fire passes true, and it can only ever force MORE review.
	RouteParsedCommandInternal(Command, /*bForceConfirmReview*/ false);
}

void USiegeAssistantComponent::RouteParsedCommandInternal(const FSiegeAssistantCommand& Command, bool bForceConfirmReview)
{
	using namespace SiegeAssistantComponentInternal;

	// ── (1) ⛔ THE NON-ORDERABLE-KIND GUARD ───────────────────────────────────
	// ALWAYS, BEFORE EXECUTION, INDEPENDENTLY OF THE MODEL, AND INDEPENDENTLY OF
	// THE CONFIRM TOGGLE. This is "Settings screen..." §6's third owner finally
	// being implemented: grammar guarantees EXISTENCE, EXECUTOR GUARANTEES
	// LEGALITY, FSM owns the conversation - and §12f measured that the executor
	// half was asserted in the document and absent from the code.
	if (!Snapshot)
	{
		// No survey ⇒ no legality answer ⇒ refuse. ⚠️ Unreachable in practice
		// (CaptureTurnSnapshot ran at gate 7), and it refuses rather than assumes
		// precisely because it is unreachable: the day it becomes reachable, the
		// safe direction is the one already written down.
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Turn %d: no snapshot at routing time - REFUSED. Legality cannot be answered without a survey, and it is never assumed."), TurnId);

		ClearPendingIntent();
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::AskUnsupported);
		return;
	}

	ESiegeAssistantRejectReason RejectReason = ESiegeAssistantRejectReason::None;
	FName OffendingKind = NAME_None;

	if (!ValidateCommandAgainstSnapshot(Command, Snapshot->GetRoster(), RejectReason, OffendingKind))
	{
		// ⚠️ Log, NEVER Warning, AND THE LEVEL IS A DECISION (TASK-441). A refusal
		// here is the MODEL being wrong - expected traffic, not a code defect - and
		// the automation runner treats a logged Warning as a test failure, which
		// would make the guard untestable by its own tests.
		//
		// ⛔ A MULTI-KIND SELECTION WITH ONE BAD KIND IS REFUSED AS A WHOLE. The
		// good kinds are neither executed nor silently dropped: answering "footmen
		// AND a sorcerer" with only footmen is the valid-shaped-wrong-command
		// failure this architecture exists to stop.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Turn %d REFUSED BY THE NON-ORDERABLE-KIND GUARD: reason=%s offending_kind=%s intent=%s kinds=%d. The whole order is refused, nothing was executed, and nothing was silently dropped. ⛔ This runs identically with the confirm toggle ON and OFF."),
			TurnId, RejectReasonName(RejectReason), *OffendingKind.ToString(),
			*SiegeAssistantIntentToSymbol(Command.Intent), Command.Kinds.Num());

		// ⚠️ THE EXISTING UNSUPPORTED-ASK OUTCOME, AND NO NEW PLAYER SURFACE (§6).
		// The guard routes to a template that already exists; it does not author one.
		ClearPendingIntent();
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::AskUnsupported);
		return;
	}

	// ── (2) ELIGIBILITY THE GRAMMAR CANNOT KNOW ───────────────────────────────
	// A ZONE order needs somewhere to go. The grammar generates `where` from the
	// live resolvable places, so a named place normally resolves - but `where`
	// may legitimately be "none", and "send the footmen" with no destination is a
	// question, not an error.
	if (IntentTakesZoneOrder(Command.Intent))
	{
		FVector UnusedDestination = FVector::ZeroVector;
		if (Command.Where == NAME_None || !Snapshot->ResolvePlace(Command.Where, UnusedDestination))
		{
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Turn %d: a zone order named place '%s', which did not resolve. Asking rather than guessing - ⛔ a made-up destination is an army marching somewhere nobody chose."),
				TurnId, *Command.Where.ToString());

			// The EXISTING AskWhichPlace clarification. ⛔ No new surface (§3).
			FSiegeAssistantMessageArgs PlaceArgs;
			PlaceArgs.Command = Command;

			EnterClarify(ESiegeAssistantReasonCode::AskWhichPlace, PlaceArgs);
			PendingShortfallIndex = INDEX_NONE;
			return;
		}
	}

	// The order the confirm step and the executor will act on. ⚠️ Assigned HERE,
	// from the caller's local copy, so all three call sites converge on one state.
	PendingArgs.Command = Command;
	PendingReason = ESiegeAssistantReasonCode::None;
	PendingShortfallIndex = INDEX_NONE;

	// ── (3) THE DEFERRED-TRIGGER FORK ─────────────────────────────────────────
	// ⚠️ It sits BEFORE the toggle deliberately: a latched order is reviewed when
	// it FIRES, not when it is typed, and the toggle has no say in that.
	if (Command.TriggerKind != NAME_None)
	{
		EnterDeferredIntent();
		return;
	}

	// ── (4) ⛔ THE TOGGLE. THE ONE READ, AND THE LAST STEP ────────────────────
	// Everything above ran already. Everything inside ExecutePendingCommand still
	// runs below. The ONLY thing this branch decides is WHETHER A HUMAN LOOKS.
	const bool bConfirmSetting = IsConfirmBeforeExecuteEnabled();
	const bool bReviewFirst = bConfirmSetting || bForceConfirmReview;

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Turn %d passed EVERY machine check (parse, selection invariant, shortfall, NON-ORDERABLE-KIND GUARD, place eligibility). confirm_setting=%s force_review=%s -> %s. ⛔ The toggle removes a HUMAN REVIEW STEP and never a machine check (CONVENTIONS \"Settings screen...\" 5)."),
		TurnId,
		bConfirmSetting ? TEXT("ON") : TEXT("OFF"),
		bForceConfirmReview ? TEXT("yes (deferred fire)") : TEXT("no"),
		bReviewFirst ? TEXT("AwaitConfirm") : TEXT("EXECUTE NOW"));

	if (bReviewFirst)
	{
		EnterAwaitConfirm();
		return;
	}

	// ⚠️ THE OFF PATH IS THE ON PATH WITH THE HUMAN REMOVED, AND IT IS WRITTEN TO
	// LOOK THAT WAY. ClearConfirmPreview() is a no-op here (nothing was raised)
	// and is called anyway, so the two paths reach ExecuteAndReport() through the
	// same two statements and cannot drift apart under a later edit.
	ClearConfirmPreview();
	ExecuteAndReport();
}

void USiegeAssistantComponent::ExecuteAndReport()
{
	// ⛔ THE SHARED TAIL OF BOTH TOGGLE PATHS. ConfirmPressed (toggle ON, after the
	// player accepted) and RouteParsedCommandInternal (toggle OFF) both end here,
	// so "ON and OFF do the same work downstream of the branch" is a property of
	// the code rather than a claim in a handoff.
	const FSiegeAssistantMessageArgs ExecutedArgs = PendingArgs;
	const bool bExecuted = ExecutePendingCommand();

	ClearPendingIntent();
	SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);

	// A refused execution reuses the EXISTING unsupported-ask outcome ("Settings
	// screen..." §6) rather than inventing a second way of saying no.
	PushMessage(bExecuted ? ESiegeAssistantReasonCode::Executed : ESiegeAssistantReasonCode::AskUnsupported, ExecutedArgs);
}

void USiegeAssistantComponent::EnterAwaitConfirm()
{
	// Never stack two previews, and never leave the previous prompt claiming to be
	// about this order.
	ClearConfirmPreview();

	// ⚠️ THE GHOST CIRCLES ARE THE HALF THAT CATCHES THE MEASURED FAILURES.
	// "Settings screen..." §1: FOUR OF THE FIVE stable eval failures are
	// wrong-place / wrong-count, and both are visible on the ground BEFORE
	// anything moves. Constrained decoding guarantees SYNTAX, never SEMANTICS.
	SpawnConfirmPreview();

	SetState(ESiegeAssistantState::AwaitConfirm);

	// ⛔ A GAME-AUTHORED TEMPLATE, FILLED FROM A REASON CODE (§3). {Order} is
	// DescribeCommandForPlayer's output, assembled from a struct pinned to
	// uint8 / int32 / FName - never a model-produced string.
	PushMessage(ESiegeAssistantReasonCode::ConfirmPrompt, PendingArgs);

	const FString Summary = DescribeCommandForPlayer(PendingArgs.Command).ToString();
	bConfirmPromptUp = true;
	OnAssistantConfirmPromptShown.Broadcast(Summary);

	if (!bConsoleOpen)
	{
		// ⚠️ THE DEFERRED-FIRE CASE, AND IT IS FLAGGED RATHER THAN PAPERED OVER: a
		// latched order can fire up to DeferredIntentTTLSeconds after it was typed,
		// on a board the player has not looked at since, with the console shut. The
		// order is NOT executed and NOT dropped - it sits in AwaitConfirm - but the
		// player is not being shown it right now.
		// ⛔ Auto-opening is NOT done here: this component owns no widget's
		// lifetime and seizing keyboard focus at a moment the player did not ask
		// for it is a worse defect than a late prompt. GetPendingConfirmSummary()
		// exists so whoever opens the console next SEEDS this prompt correctly.
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("A confirm prompt was raised while the console is CLOSED - the order is pending and NOTHING was executed. Whoever opens the console next must SEED from GetPendingConfirmSummary(), or the player will never be asked."));
	}
}

void USiegeAssistantComponent::ClearConfirmPreview()
{
	// ⚠️ SAFE WHEN NOTHING IS UP, AND FIVE CALLERS DEPEND ON THAT: EndPlay,
	// CancelPressed, NotifyConsoleClosed, ConfirmPressed and MarkAssistantFaulted
	// all call it unconditionally, as does EnterAwaitConfirm and the toggle-OFF
	// path.
	if (ConfirmPositionDecal)
	{
		ConfirmPositionDecal->Destroy();
		ConfirmPositionDecal = nullptr;
	}

	if (ConfirmAttackDecal)
	{
		ConfirmAttackDecal->Destroy();
		ConfirmAttackDecal = nullptr;
	}

	// The delegate law: fire on an ACTUAL change, never on a no-op. bConfirmPromptUp
	// - not the decals - is the truth, because a missing M_SpellReticle yields a
	// real prompt with no circles (TASK-440 contract #7).
	if (bConfirmPromptUp)
	{
		bConfirmPromptUp = false;
		OnAssistantConfirmPromptHidden.Broadcast();
	}
}

void USiegeAssistantComponent::SpawnConfirmPreview()
{
	ASiegePlayerController* Controller = ResolveOrderingController();
	if (!Controller || !Snapshot)
	{
		return;
	}

	const FSiegeAssistantCommand& Command = PendingArgs.Command;

	// ⚠️ ONLY THE ZONE ORDERS PUT ANYTHING ON THE GROUND, AND THAT IS HONEST
	// RATHER THAN INCOMPLETE. Follow's anchor is the live hero, and charge /
	// fallback / rally are army-wide with no place at all - drawing a circle for
	// any of them would be a picture of something that is not going to happen.
	// Those four are reviewed by their SUMMARY LINE, which is the same review the
	// keys never offered at all.
	if (!SiegeAssistantComponentInternal::IntentTakesZoneOrder(Command.Intent))
	{
		return;
	}

	FVector Destination = FVector::ZeroVector;
	if (!Snapshot->ResolvePlace(Command.Where, Destination))
	{
		// Step (2) already refused an unresolvable place, so this is belt and
		// braces - and it degrades to "no circles", never to a circle at the origin.
		return;
	}

	// ⚠️ NULL-SAFE BY CONTRACT: SpawnGroupCircleDecal returns nullptr when
	// M_SpellReticle is missing (TASK-440 contract #7). The confirm step still
	// works - the summary line and Accept / Cancel are unaffected.
	ConfirmPositionDecal = Controller->SpawnGroupCircleDecal(AssistantPositionRadius);
	ConfirmAttackDecal = Controller->SpawnGroupCircleDecal(AssistantAttackRadius);

	// The pick spawns its circles HIDDEN at the origin and moves them under the
	// cursor every tick (UpdateGroupPickReticle). The assistant has no cursor, so
	// it does that positioning ONCE, here, to the resolved destination.
	if (ConfirmPositionDecal)
	{
		ConfirmPositionDecal->SetActorLocation(Destination);
		ConfirmPositionDecal->SetActorHiddenInGame(false);
	}

	if (ConfirmAttackDecal)
	{
		ConfirmAttackDecal->SetActorLocation(Destination);
		ConfirmAttackDecal->SetActorHiddenInGame(false);
	}
}

bool USiegeAssistantComponent::ExecutePendingCommand()
{
	ASiegePlayerController* Controller = ResolveOrderingController();
	if (!Controller)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("No ordering controller - NOTHING was executed."));
		return false;
	}

	// ── ⛔ AUTHORITY, RE-CHECKED AT THE CALL SITE ─────────────────────────────
	// This is NOT a duplicate of SubmitUtterance's gate 3 and it is not
	// belt-and-braces: CreateUnitGroup carries NO authority guard by design
	// (TASK-440 contract #1 - BeginGroupPick owns the M8 D5 lockout for the pick
	// path, so a guard inside CreateUnitGroup would have been unreachable and
	// would have broken its zero-behaviour-change criterion). ⇒ Guarding HERE is
	// the whole guard for this path. It also covers the deferred case, where the
	// authority that was true at typing time is being re-asserted at firing time.
	if (!Controller->HasAuthority())
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Execution refused - no authority (P1 client observer posture, M8 doc 4.1). ⛔ The assistant is never more capable than the keys."));
		return false;
	}

	const FSiegeAssistantCommand& Command = PendingArgs.Command;

	// ⛔ THE SAME PUBLIC APIs THE KEYS CALL (§2), NEVER A PARALLEL IMPLEMENTATION.
	// The mapping is CONVENTIONS §8's executor seam, character-for-character.
	switch (Command.Intent)
	{
	case ESiegeAssistantIntent::Send:
	case ESiegeAssistantIntent::Guard:
		return ExecuteZoneOrder(Command, ESiegeGroupCommandType::Hold, *Controller);

	case ESiegeAssistantIntent::Ambush:
		return ExecuteZoneOrder(Command, ESiegeGroupCommandType::Ambush, *Controller);

	case ESiegeAssistantIntent::Follow:
		return ExecuteFollowOrder(Command, *Controller);

	case ESiegeAssistantIntent::Charge:
		// ⛔ THE SHARED ENTRY POINT, NEVER THE BARE LATCH (TASK-454/456). The T key
		// calls ApplyArmyWideStance and so does this line, so there is ONE
		// implementation of "release, then latch" - which is §2, not a convenience.
		// §8's executor seam is HONOURED, not departed from: SetUnitCommand(Attack)
		// is still what happens, as ApplyArmyWideStance's LAST statement, behind
		// CancelGroupPick() + ClearAllUnitGroups() (the TASK-344 release law) and
		// the bMatchEnded guard - exactly the T key's four statements in order.
		// ⛔ Do NOT "restore" SetUnitCommand here: that re-opens the divergence
		// where an assistant `charge` left standing group orders parked and the
		// same word typed on the T key freed them.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("EXECUTED CHARGE through ASiegePlayerController::ApplyArmyWideStance(Attack) - the SAME API the T key calls, release law included."));
		Controller->ApplyArmyWideStance(ESiegeUnitCommand::Attack);
		return true;

	case ESiegeAssistantIntent::Fallback:
		// The E key's twin of Charge, through the same shared entry point.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("EXECUTED FALLBACK through ASiegePlayerController::ApplyArmyWideStance(Defend) - the SAME API the E key calls, release law included."));
		Controller->ApplyArmyWideStance(ESiegeUnitCommand::Defend);
		return true;

	case ESiegeAssistantIntent::Rally:
		return ExecuteRallyOrder(*Controller);

	default:
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Execution refused - intent '%s' has no executor mapping. NOTHING was executed."), *SiegeAssistantIntentToSymbol(Command.Intent));
		return false;
	}
}

bool USiegeAssistantComponent::ExecuteZoneOrder(const FSiegeAssistantCommand& Command, ESiegeGroupCommandType GroupType, ASiegePlayerController& Controller) const
{
	// ⛔ THE COORDINATE AIRLOCK (§3). The model emitted a SYMBOL; the FVector is
	// produced here, game-side, and the model never saw a number that means a
	// position.
	FVector Destination = FVector::ZeroVector;
	if (!Snapshot || !Snapshot->ResolvePlace(Command.Where, Destination))
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Zone order refused - place '%s' did not resolve at execution time. NOTHING was executed."), *Command.Where.ToString());
		return false;
	}

	// ⚠️ A FRESH LOCAL ARRAY THAT NEVER ALIASES INTO UnitGroups (TASK-440 contract
	// #3): CreateUnitGroup's step 6 appends to UnitGroups and can reallocate it,
	// which would dangle a reference into a live group's own Members.
	TArray<TWeakObjectPtr<ASummonedUnit>> Members;
	if (!SelectUnitsForOrder(Command, Destination, /*bFollowOrder*/ false, Members))
	{
		// SelectUnitsForOrder logged the precise reason (absent kind, or a genuine
		// shortfall it refused to truncate).
		return false;
	}

	// ⛔ TASK-440's EXTRACTION - THE ONE IMPLEMENTATION OF "A ZONE GROUP IS
	// FORMED", shared with the cursor pick rather than copied.
	//
	// ⚠️ MARKERS ARE PASSED AS NULL, DELIBERATELY. The parameters are
	// OWNERSHIP-TRANSFERRING (TASK-440 contract #2), but TASK-442's ConfirmPressed
	// tears the ghost circles down BEFORE it executes - so that no ghost outlives
	// the decision it belonged to - and by this line there is nothing left to
	// transfer. ⇒ An assistant-formed group owns no persistent ground marker,
	// unlike a key-formed one. Declared in the handoff, not discovered later.
	const int32 NewGroupId = Controller.CreateUnitGroup(
		GroupType,
		Destination, AssistantPositionRadius,
		Destination, AssistantAttackRadius,
		Members,
		/*PositionMarker*/ nullptr,
		/*AttackMarker*/ nullptr);

	if (NewGroupId == INDEX_NONE)
	{
		// "Every member was dead" - an outcome, not an error (TASK-440 contract #4).
		// The caller owns the player-facing message; this is the log half.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Zone order formed no group - every selected member died between selection and formation. NOTHING was executed."));
		return false;
	}

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("EXECUTED %s: group %d formed with %d unit(s) at place '%s' (position r=%.0f, attack r=%.0f), through ASiegePlayerController::CreateUnitGroup - the SAME API the R/F keys call."),
		GroupType == ESiegeGroupCommandType::Ambush ? TEXT("AMBUSH") : TEXT("HOLD"),
		NewGroupId, Members.Num(), *Command.Where.ToString(), AssistantPositionRadius, AssistantAttackRadius);

	return true;
}

bool USiegeAssistantComponent::ExecuteFollowOrder(const FSiegeAssistantCommand& Command, ASiegePlayerController& Controller) const
{
	// ⚠️ THE ONE ORDER WHOSE ANCHOR IS THE HERO RATHER THAN A PIECE OF GROUND, so
	// "nearest to the target" means nearest to the hero here. A dead hero is NOT
	// an anchor (the shipped hero-death ruling) - but that does NOT refuse the
	// order: EnrollInDefaultFollowGroup is null-safe, and the follow body holds
	// position while the anchor is missing and resumes on respawn. Refusing would
	// be worse behaviour than the shipped C key's.
	const AActor* Anchor = Controller.GetFollowAnchor();
	const FVector SortAnchor = Anchor ? Anchor->GetActorLocation() : FVector::ZeroVector;

	TArray<TWeakObjectPtr<ASummonedUnit>> Members;
	if (!SelectUnitsForOrder(Command, SortAnchor, /*bFollowOrder*/ true, Members))
	{
		return false;
	}

	// ⛔ THE SHIPPED SEAM, PER UNIT (§8): already public, already idempotent, and
	// it owns the eligibility gate, the lazy group creation, the steal-out-of-any-
	// other-group law and the sunflower station. ⛔ Never a parallel enrolment.
	int32 Enrolled = 0;
	for (const TWeakObjectPtr<ASummonedUnit>& Member : Members)
	{
		if (ASummonedUnit* Unit = Member.Get())
		{
			Controller.EnrollInDefaultFollowGroup(Unit);
			++Enrolled;
		}
	}

	if (Enrolled == 0)
	{
		UE_LOG(LogSiegeAssistant, Log, TEXT("Follow order enrolled nobody - NOTHING was executed."));
		return false;
	}

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("EXECUTED FOLLOW: %d unit(s) enrolled through ASiegePlayerController::EnrollInDefaultFollowGroup - the SAME API the C key and the spawn auto-enrol call."), Enrolled);

	return true;
}

bool USiegeAssistantComponent::ExecuteRallyOrder(ASiegePlayerController& Controller) const
{
	// GetFollowAnchor() is the shipped "live hero pawn, or null" accessor and it
	// already returns null for a pawn that is missing, pending-kill or IsDead() -
	// so using it buys the hero-death ruling for free instead of re-deriving it.
	AHeroCharacter* Hero = Cast<AHeroCharacter>(Controller.GetFollowAnchor());
	if (!Hero)
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Rally refused - no live hero to rally with. ⚠️ Refused rather than silently no-oped, so the player is told nothing happened."));
		return false;
	}

	// ⛔ The hero ability itself, called exactly as IA_Rally calls it. It owns its
	// own cooldown, radius and refusal; a press while on cooldown is a no-op there
	// and re-implementing that check here would be a second source of truth.
	Hero->Rally();

	UE_LOG(LogSiegeAssistant, Log, TEXT("EXECUTED RALLY through AHeroCharacter::Rally() - the SAME API the IA_Rally key calls."));
	return true;
}

bool USiegeAssistantComponent::SelectUnitsForOrder(const FSiegeAssistantCommand& Command, const FVector& SortAnchor, bool bFollowOrder,
	TArray<TWeakObjectPtr<ASummonedUnit>>& OutMembers) const
{
	OutMembers.Reset();

	// ── ⛔ AN EXCLUSION IS NEVER SILENTLY IGNORED (AS-§20.1) ──────────────────
	// ExcludeKinds is meaningful ONLY against the "all" selection, and the branch
	// below is the only one that can subtract anything. A command carrying BOTH a
	// positive selection and an exclusion would therefore reach the per-kind loop,
	// which has no subtraction step, and the exception would be parsed and then
	// DROPPED IN SILENCE - "send 10 footmen except the miners" executing as "send
	// 10 footmen", a valid-shaped wrong command that looks obeyed.
	//
	// ⚠️ THIS IS A BACKSTOP, NOT A SECOND ENFORCEMENT, and the distinction matters
	// because a divergent duplicate of a parser rule is its own defect. Two gates
	// already refuse this state with the same verdict: the parser (ExcludeConflict,
	// cross-field check 2) and SiegeAssistantValidateSelection, which
	// HandleModelCompletion now calls WITH Command.ExcludeKinds. So this is
	// unreachable from the model and stays unreachable from the M8 P2 wire path,
	// which passes the same validator. It exists because "unreachable" is a claim
	// about today's callers, and the cost of being wrong about it is the exact
	// failure the schema was shaped to prevent.
	//
	// ⚠️ Warning, not Log, AND THAT IS A DECISION: the refusals below are the MODEL
	// being wrong (expected traffic, logged at Log so the automation runner does
	// not read them as failures), whereas reaching THIS line is a CODE defect - a
	// struct that passed neither gate. No automation test can trigger it (the
	// selection tests have no world), so the Warning costs no green bar.
	if (Command.ExcludeKinds.Num() > 0 && Command.Kinds.Num() > 0)
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Selector refused - a command reached the selector with BOTH a %d-kind selection and a %d-kind exclusion. ⛔ The parser and SiegeAssistantValidateSelection both refuse that shape (exclude_conflict), so this is a code or wire defect, not a model error. The whole order is refused; the exception is NEVER dropped in silence. NOTHING was executed."),
			Command.Kinds.Num(), Command.ExcludeKinds.Num());
		return false;
	}

	// ── ⛔ A REGION IS NEVER MERGED WITH A SELECTION OR AN EXCLUSION (AS-§21.5) ──
	// THE EXCLUSION BACKSTOP ABOVE, SECOND INSTANCE - and it is the second instance
	// of ONE principle rather than a second principle, which is the same argument
	// AS-§21.6 used to earn the parser's third cross-field check.
	//
	// ⚠️ NOTE THE ASYMMETRY WITH THE BACKSTOP ABOVE, BECAUSE IT CHANGES THE REASON
	// RATHER THAN THE VERDICT. An exclusion beside a selection would be DROPPED IN
	// SILENCE (the per-kind loop has no subtraction step). A region beside either
	// would NOT be dropped - the predicate below sits in the shared candidate
	// gather, so it would apply and the two filters would COMPOSE. That is worse in
	// a different way and refused for a different sentence: AS-§21.5 rules the
	// combination a PARSE FAILURE, NEVER A MERGE. "Send 10 footmen in the mid" is a
	// FILTERED COUNT - a different feature, not asked for - and executing the
	// intersection would be this architecture answering a question nobody put to it.
	// ⛔ Guessing which half to honour is the valid-shaped-wrong-command class; so is
	// silently honouring both.
	//
	// ⚠️ THE EXCLUSION LEG IS A DELIBERATE WIDENING BEYOND THE LITERAL DISPATCH,
	// DECLARED RATHER THAN SLIPPED IN. The task spec names region + SELECTION; AS-
	// §21.5 says in terms that "`all_except` plus a region is the same failure", the
	// parser refuses both with region_conflict, and SiegeAssistantValidateSelection
	// refuses both. Refusing only one of the two would leave the other composing
	// quietly. Both legs are unreachable from the model and from the M8 P2 wire path
	// for the same reason the exclusion backstop is: two gates already refuse them.
	// It exists because "unreachable" is a claim about TODAY'S CALLERS.
	//
	// ⚠️ Warning, not Log, on the same reasoning as the backstop above: the refusals
	// further down are the MODEL being wrong (expected traffic), whereas reaching
	// THIS line is a CODE defect - a struct that passed neither gate.
	if (Command.RegionPlace != NAME_None && (Command.Kinds.Num() > 0 || Command.ExcludeKinds.Num() > 0))
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Selector refused - a command reached the selector naming region '%s' AND a %d-kind selection / a %d-kind exclusion. ⛔ AS-§21.5 rules that combination a PARSE FAILURE (region_conflict), never a merge. The parser and SiegeAssistantValidateSelection both refuse it, so this is a code or wire defect, not a model error. The whole order is refused; the region is NEVER quietly composed with another filter. NOTHING was executed."),
			*Command.RegionPlace.ToString(), Command.Kinds.Num(), Command.ExcludeKinds.Num());
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ETeamId OrderingTeam = ETeamId::Blue;
	if (!ResolveOrderingTeam(OrderingTeam))
	{
		// ⛔ NEVER GUESS A TEAM. A wrong ordering team selects the wrong army.
		return false;
	}

	// ── ⛔⭐ THE REGION RESOLVES ONCE, ABOVE THE LOOP, AND A NAMED REGION THAT DOES
	//    NOT RESOLVE IS A REFUSAL - ⛔ NEVER AN UNFILTERED ORDER (AS-§21.5) ───────
	//
	// ⚖️ THIS IS THE HARD RULE THE WHOLE FEATURE RESTS ON. Falling through here
	// would execute "send everyone in the ancient ground" as "send everyone" - an
	// ARMY MOVING THAT THE PLAYER NEVER ASKED TO MOVE, and precisely the AS-§20.1
	// valid-shaped-wrong-command class this design exists to make unreachable. Fail
	// closed; there is no acceptable open failure mode here. ⛔ There is deliberately
	// no "whole map" fallback box and no zero-extent default: ResolvePlaceRegion
	// leaves BOTH out-params untouched on failure for exactly this reason, and
	// inventing a box here would re-open the hole from the other side.
	//
	// ⛔ ONE RESOLVE, NOT ONE PER UNIT. The geometry is snapshot-time and constant
	// for the call; only the POSITIONS are live (AS-§21.4: the units move, the
	// grounds do not). Resolving inside the loop would be a per-candidate FName
	// lookup for an answer that cannot change.
	//
	// ⚠️ THE REACHABLE CAUSE IS DECLARED RATHER THAN TREATED AS IMPOSSIBLE: the
	// grammar can only emit a region symbol GetRegionPlaceNames() published this
	// match, so an unknown symbol is near-unreachable - but the snapshot's own
	// declared residual (a ground DESTROYED between Capture() and execution) lands
	// exactly here, and so does a missing snapshot. Both take the same refusal.
	//
	// ⚠️ Log, not Warning, matching ExecuteZoneOrder's sibling "place did not
	// resolve at execution time" line: this is a board condition, not a code defect,
	// and the automation runner reads Warning as failure.
	FVector RegionCentre = FVector::ZeroVector;
	FVector2D RegionHalfExtent = FVector2D::ZeroVector;
	const bool bHasRegion = Command.RegionPlace != NAME_None;
	if (bHasRegion && (!Snapshot || !Snapshot->ResolvePlaceRegion(Command.RegionPlace, RegionCentre, RegionHalfExtent)))
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Selector refused - region '%s' did not resolve at execution time (no snapshot, not a region-bearing place, or the region is gone). ⛔ The whole order is refused and NOTHING was executed. ⚖️ It is NEVER downgraded to an unfiltered order: \"send everyone in the ancient ground\" executing as \"send everyone\" is an army moving that the player never asked to move (AS-§21.5, fail closed)."),
			*Command.RegionPlace.ToString());
		return false;
	}

	/** One eligible candidate with its precomputed distance, so the sort compares numbers rather than recomputing geometry. */
	struct FSelectorCandidate
	{
		double DistSq = 0.0;
		ASummonedUnit* Unit = nullptr;
	};

	// ⚠️ THE TWO COUNTERS THE REFUSAL ARITHMETIC IS BUILT FROM, AND THEY ARE
	// COUNTED WHERE THE REJECTION HAPPENS RATHER THAN RE-DERIVED AFTERWARDS.
	// ⛔ IneligibleCount IS A WHOLE-TEAM FIGURE AND IS LABELLED AS ONE EVERYWHERE IT
	// IS PRINTED - it is NOT "ineligible units inside the region", because the
	// region test never runs on a unit the eligibility gate already dropped, so that
	// number is not measured and must not be implied.
	int32 OutsideRegionCount = 0;
	int32 IneligibleCount = 0;

	// ONE pass over the world - not one per kind. ⛔ No registry, no actor cache,
	// no dirty flag, no subscription list: CONVENTIONS §4 rejects all four on
	// sight, and this array is a function local that dies with the call.
	TArray<FSelectorCandidate> Eligible;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		ASummonedUnit* Unit = *It;
		if (!IsValid(Unit) || Unit->IsUnitDead())
		{
			continue;
		}

		if (Unit->GetTeamId() != OrderingTeam)
		{
			continue;
		}

		// ⛔ THE SHIPPING PREDICATES, NEVER REIMPLEMENTED (§8). The
		// Cleric-follows-but-cannot-hold split has exactly one owner and it is
		// ASummonedUnit. This is also the SAME split the cursor pick makes
		// (ConfirmGroupPickStage's Select case), so the assistant and the keys
		// select from identical populations.
		const bool bEligible = bFollowOrder ? Unit->IsFollowCommandEligible() : Unit->IsGroupCommandEligible();
		if (!bEligible)
		{
			++IneligibleCount;
			continue;
		}

		// ── ⭐ THE REGION PREDICATE (TASK-548, AS-§21.5) ──────────────────────
		// ⛔ IT IS A PREDICATE INSIDE A LOOP THAT ALREADY EXISTS - still ONE pass
		// over the world, still no registry, no actor cache, no dirty flag and no
		// subscription list (§4 rejects all four on sight). It adds two float
		// compares per candidate and not one traversal.
		//
		// ⭐ IT SITS IN THE SHARED CANDIDATE GATHER RATHER THAN IN THE
		// Kinds.Num() == 0 BRANCH, AND THAT PLACEMENT IS THE WHOLE REASON IT
		// APPLIES TO BOTH SELECTOR BRANCHES FROM ONE PIECE OF CODE. The exclusion
		// is "all"-only by its own semantics; a region is not - "the footmen in the
		// ancient ground" is as sensible an order as "everyone in the ancient
		// ground". ⚠️ TODAY the per-kind branch cannot be reached with a region
		// (the parser refuses region + selection, and the backstop at the top of
		// this function refuses it again), so this predicate's effect on that
		// branch is currently unobservable - it is written correctly anyway,
		// because if a later ruling opens the combination the filter must already
		// be there. ⛔ A region that is parsed and then dropped by a branch that
		// never learned about it is the exact failure mode AS-§21.6 names.
		//
		// ⛔ A PURE CALL TO THE SHIPPED PREDICATE - ⛔ never an inline box test and
		// ⛔ never AAncientGround::IsPointInZone directly. The executor holds no
		// actor: the SNAPSHOT holds the geometry, deliberately (AS-§21.4). The
		// boundary is INCLUSIVE and Z is IGNORED, which are copied semantics, not
		// re-decided ones - the assistant has to give the same answer the game
		// already gives, or a unit is "in the mid" for capture scoring and "not in
		// the mid" for selection.
		//
		// ⚠️ THE POSITION IS READ LIVE, HERE, AT EXECUTION TIME. Snapshot-time
		// geometry, execution-time membership: a unit that walked OUT of the ground
		// between the typed sentence and the order landing is not selected.
		if (bHasRegion && !FSiegeAssistantRegionStatics::IsPointInRegion(Unit->GetActorLocation(), RegionCentre, RegionHalfExtent))
		{
			++OutsideRegionCount;
			continue;
		}

		FSelectorCandidate Candidate;
		Candidate.Unit = Unit;
		Candidate.DistSq = FVector::DistSquared2D(Unit->GetActorLocation(), SortAnchor);
		Eligible.Add(Candidate);
	}

	// ── ⛔⭐ EMPTY AFTER THE REGION IS A LOUD REFUSAL WITH THE ARITHMETIC IN THE
	//    LOG - ⛔ NEVER A SILENT NO-OP (AS-§21.5, the EMPTY-AFTER-EXCLUSION clause
	//    applied verbatim) ────────────────────────────────────────────────────────
	//
	// ⚠️ THE PARSER STRUCTURALLY CANNOT ANSWER THIS. ParseSiegeAssistantCommand is
	// PURE - no world, no roster, no snapshot - so "did that region contain
	// anybody?" is the executor's question and only the executor's. Returning false
	// routes to the EXISTING unsupported-ask outcome in ExecuteAndReport: ⛔ no new
	// ask symbol (the ask alternatives are grammar the model samples from) and ⛔ no
	// new reason code authored at a call site (§3). "Nothing happened and nothing
	// was said" is the one report shape this feature cannot afford - it reads as
	// "the assistant ate my order".
	//
	// ⭐ IT IS CHECKED HERE, BEFORE THE SORT AND BEFORE EITHER BRANCH, PRECISELY SO
	// NEITHER BRANCH CAN MISREPORT IT. An empty candidate set would otherwise fall
	// into the "no eligible unit at all for an army-wide selection" line below (or,
	// on the per-kind side, "no ELIGIBLE '<kind>' is alive") - both of which are
	// TRUE-SOUNDING AND WRONG when the units exist and are simply standing
	// elsewhere. A refusal that names the wrong cause is worse than a generic one:
	// it sends the reader to look at the roster instead of at the ground.
	//
	// ⛔ AND THE ARITHMETIC IS HONEST ABOUT WHAT IT DID NOT MEASURE (AS-§21.11
	// designed outcome 2). Ogres, Sappers and Clerics can never take a zone order at
	// all - ASummonedUnit::CanTakeZoneOrders() is Standard-profile only - so a region
	// order correctly skips them, and they are counted as NEVER ELIGIBLE, never as
	// "outside the region". ⚠️ That count is a WHOLE-TEAM figure and says so in the
	// line: the region test never ran on those units, so this log CANNOT distinguish
	// "the ground is empty" from "the ground is full of units that could never take
	// this order", and it must not pretend otherwise. The eligibility predicate that
	// actually ran is named, because it differs between a zone order and a follow.
	if (bHasRegion && Eligible.Num() == 0)
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Selector refused - the region emptied the selection: %d eligible unit(s) on the ordering team, %d standing OUTSIDE region '%s', %d left. ⚠️ A further %d live unit(s) on the team never reached the region test at all - %s rejected them first (a zone order is Standard-profile only, so Ogres, Sappers and Clerics are never candidates: AS-§21.11 designed outcome 2, NOT a bug), and that is a WHOLE-TEAM figure, so this line cannot tell you whether the region is EMPTY or merely holds units that could never take this order. ⛔ The whole order is refused and the player is TOLD through the existing unsupported-ask outcome; \"nothing happened and nothing was said\" is the one report shape this feature cannot afford. NOTHING was executed."),
			OutsideRegionCount + Eligible.Num(), OutsideRegionCount, *Command.RegionPlace.ToString(), Eligible.Num(),
			IneligibleCount, bFollowOrder ? TEXT("IsFollowCommandEligible()") : TEXT("IsGroupCommandEligible()"));
		return false;
	}

	if (bHasRegion)
	{
		// ⚠️ THE SUCCESS LINE IS NOT DECORATION - IT IS THIS PATH'S ONLY INSTRUMENT,
		// for the same stated reason the exclusion's is (TASK-522): A LOG THAT ONLY
		// SPEAKS ON FAILURE CANNOT PROVE A SUCCESS. The region filter cannot be
		// reached by the automation suite (EditorContext simple tests have no world
		// and no actors), so the evidence that a region actually FILTERED rather than
		// being parsed and dropped is this line plus Jonathan's playtest at TASK-552.
		// ⚠️ It prints CANDIDATES, not "selected": the per-kind take and the
		// exclusion both run downstream of here and own the final figure. Today they
		// cannot coexist with a region (the backstop above), so the numbers coincide
		// - but the word must stay true of the code rather than of today's callers.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Selector applied a REGION: %d eligible, %d outside region '%s', %d INSIDE and carried forward as candidates. ⚠️ A further %d live unit(s) on the team never reached the region test - %s rejected them first (AS-§21.11 designed outcome 2). The order moves ONLY the units STANDING IN the named region, and membership was tested against LIVE positions at execution time."),
			OutsideRegionCount + Eligible.Num(), OutsideRegionCount, *Command.RegionPlace.ToString(), Eligible.Num(),
			IneligibleCount, bFollowOrder ? TEXT("IsFollowCommandEligible()") : TEXT("IsGroupCommandEligible()"));
	}

	// ⚠️ NEAREST TO THE TARGET, NOT TO THE HERO (§8's executor seam, verbatim).
	// Sorting to the hero would send the back rank across the map while the front
	// rank stayed home. ⛔ THE REGION FILTERS; IT DOES NOT RE-RANK - DistSq computes
	// exactly what it always computed, against the same anchor, and this comparator
	// is untouched.
	Eligible.Sort([](const FSelectorCandidate& A, const FSelectorCandidate& B)
	{
		return A.DistSq < B.DistSq;
	});

	// ── who:"all" (and every army-wide verb) - an EMPTY selection means EVERY
	//    eligible unit, never "nobody". The parser already refused who:"none" for
	//    a selection-bearing intent, which is the only case those two differ in.
	//
	// ⭐ AND THIS IS THE BRANCH THE EXCLUSION SUBTRACTS FROM (TASK-522 /
	//    AS-§20.1): who:{"all_except":["miner"]} parses to an EMPTY Kinds plus a
	//    populated ExcludeKinds, so "everyone" is built here and the named kinds
	//    are simply not added. ⛔ It is a PREDICATE INSIDE A LOOP THAT ALREADY
	//    EXISTS - still ONE pass over the world, still no registry, no actor
	//    cache, no dirty flag and no subscription list (§4 rejects all four on
	//    sight).
	if (Command.Kinds.Num() == 0)
	{
		// ⛔ NO SECOND LOWER-CASING AND NO RE-DERIVED MAPPING. FName comparison is
		// case-insensitive (TArray::Contains uses the same operator== as the
		// sibling seam below), the snapshot owns the card-row -> symbol transform
		// (SiegeAssistantSnapshot's CanonicalKind: the row name lower-cased), and
		// the grammar only ever emits a symbol that transform produced. So
		// "miner" finds the "Miner" card row here exactly as it does 40 lines down.
		const int32 EligibleCount = Eligible.Num();
		int32 ExcludedCount = 0;

		for (const FSelectorCandidate& Candidate : Eligible)
		{
			if (Command.ExcludeKinds.Contains(Candidate.Unit->GetCardID()))
			{
				++ExcludedCount;
				continue;
			}

			OutMembers.Add(Candidate.Unit);
		}

		if (OutMembers.Num() == 0)
		{
			// ⛔⭐ EMPTY AFTER EXCLUSION IS A REFUSAL, LOUDLY, AND NEVER A NO-OP
			// (AS-§20.1's "EMPTY-AFTER-EXCLUSION"). ⚠️ The parser CANNOT answer
			// this: ParseSiegeAssistantCommand is pure, holds no world and no
			// roster, and is never handed the live kind list, so "did that
			// exception subtract everybody?" is structurally the executor's
			// question. Returning false routes to the EXISTING unsupported-ask
			// outcome in ExecuteAndReport - ⛔ no new ask symbol (the ask
			// alternatives are part of the grammar the model samples from) and no
			// new reason code authored at a call site (§3). The LOG carries the
			// precise arithmetic, exactly as the shortfall path below does and for
			// the same stated reason.
			if (ExcludedCount > 0)
			{
				UE_LOG(LogSiegeAssistant, Log,
					TEXT("Selector refused - the exclusion emptied the selection: %d eligible unit(s) on the ordering team, %d removed by the %d-kind exclusion, %d left. ⛔ The whole order is refused and the player is TOLD through the existing unsupported-ask outcome; \"nothing happened and nothing was said\" is the one report shape this feature cannot afford. NOTHING was executed."),
					EligibleCount, ExcludedCount, Command.ExcludeKinds.Num(), OutMembers.Num());
			}
			else
			{
				UE_LOG(LogSiegeAssistant, Log, TEXT("Selector found no eligible unit at all for an army-wide selection. NOTHING was executed."));
			}

			return false;
		}

		if (ExcludedCount > 0)
		{
			// ⚠️ THE SUCCESS LINE IS NOT DECORATION - IT IS THIS PATH'S ONLY
			// INSTRUMENT. The exclusion filter cannot be reached by TASK-523's
			// automation tests (EditorContext simple tests have no world and no
			// actors), so the evidence that an exception was actually SUBTRACTED
			// rather than parsed-and-dropped is this line plus Jonathan's playtest
			// at TASK-527. It prints the arithmetic on the path that WORKED,
			// because a log that only speaks on failure cannot prove a success.
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Selector applied an EXCLUSION: %d eligible, %d removed by the %d-kind exclusion, %d selected. The order moves everyone EXCEPT the named kind(s)."),
				EligibleCount, ExcludedCount, Command.ExcludeKinds.Num(), OutMembers.Num());
		}
		else if (Command.ExcludeKinds.Num() > 0)
		{
			// ⚠️ AN EXCLUSION THAT SUBTRACTED NOBODY IS NOT AN ERROR AND IS NOT
			// PAPERED OVER. "Send everyone except the miners" with no live miner
			// is the same order as "send everyone", and refusing it would punish
			// the player for a correct sentence. ⛔ The kind-does-not-exist-at-all
			// refusal is NOT this layer's job: the grammar can only emit a symbol
			// the live roster produced, and Zone A's [FORCES] rule answers
			// unsupported for a noun that is not a kind at all (the corpus row
			// DEV-32). Re-refusing here would paper over exactly that.
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Selector applied a %d-kind exclusion that removed NOBODY - no unit of the excluded kind(s) is eligible and alive. %d unit(s) selected. This is the same order as \"all\", and it is executed rather than refused."),
				Command.ExcludeKinds.Num(), OutMembers.Num());
		}

		return true;
	}

	const int32 SelectionNum = FMath::Min(Command.Kinds.Num(), Command.Counts.Num());
	for (int32 Index = 0; Index < SelectionNum; ++Index)
	{
		const FName Kind = Command.Kinds[Index];
		const int32 Requested = Command.Counts[Index];

		int32 Taken = 0;
		for (const FSelectorCandidate& Candidate : Eligible)
		{
			// ⚠️ FName COMPARISON IS CASE-INSENSITIVE, WHICH IS WHY THERE IS NO
			// SECOND LOWER-CASING HERE. The snapshot's canonical symbol is the card
			// row name lower-cased ("footman"); the unit carries the row name
			// ("Footman"). Re-deriving the transform locally would be a second
			// source of truth for a mapping the snapshot already owns.
			if (Candidate.Unit->GetCardID() != Kind)
			{
				continue;
			}

			OutMembers.Add(Candidate.Unit);
			++Taken;

			// 0 == "all" (§9), which can never be short of itself.
			if (Requested > 0 && Taken >= Requested)
			{
				break;
			}
		}

		if (Taken == 0)
		{
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Selector refused - no ELIGIBLE '%s' is alive on the ordering team at execution time. The whole order is refused. NOTHING was executed."),
				*Kind.ToString());
			OutMembers.Reset();
			return false;
		}

		if (Requested > 0 && Taken < Requested)
		{
			// ⛔ ON SHORTFALL IT RETURNS IT, NEVER TRUNCATES SILENTLY (§8, verbatim).
			// The FSM's FindShortfall already asked about the board as it stood when
			// the sentence was typed; this is the narrower case where units died
			// between that question and this execution. Handing back 7 units for an
			// order that said 10 is the valid-shaped-wrong-command failure.
			// ⚠️ The player is told the order did not happen, through the existing
			// unsupported-ask outcome - the LOG carries the precise arithmetic,
			// because a more specific sentence would need a new reason code and §3
			// forbids authoring one at a call site.
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Selector refused - '%s' is SHORT at execution time (%d requested, %d eligible and alive). ⛔ The whole order is refused rather than silently truncated. NOTHING was executed."),
				*Kind.ToString(), Requested, Taken);
			OutMembers.Reset();
			return false;
		}
	}

	return OutMembers.Num() > 0;
}

void USiegeAssistantComponent::EnterDeferredIntent()
{
	// Never two latches.
	ClearDeferredIntent();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("No world - a deferred intent cannot be latched. NOTHING was executed and nothing is waiting."));

		ClearPendingIntent();
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::RefusedAssistantUnavailable);
		return;
	}

	const FSiegeAssistantCommand Latched = PendingArgs.Command;

	// ── ⚠️ THE RELATIVE-WORD TRAP, AND IT IS A MEASURED ONE ───────────────────
	// The eval's DEV-11 is "wait until i have 2 MORE knights then send them mid",
	// and it is the row that flips between runs. The grammar's `at_least` is
	// ABSOLUTE (1..30) and has no way to express "more", so a model that reads the
	// sentence relatively emits at_least:2 while the player meant CurrentCount+2.
	// The two readings are indistinguishable in the JSON.
	//
	// ⇒ If the threshold is ALREADY satisfied, the absolute reading fires
	// instantly - which is exactly what a misread "more" looks like, and it is the
	// one case where a deferred order does something the player plainly did not
	// ask for. So it RE-ASKS rather than firing: the ambiguity is resolved against
	// the LIVE roster by the only party who actually knows, which is the player.
	//
	// ⚖️ The alternative (fire now -> AwaitConfirm) was considered and rejected:
	// it is correct for the absolute reading, but it converts a known ambiguity
	// into a confident action, and "confidently wrong" is the failure class this
	// whole design exists to remove. Re-asking costs one sentence; firing wrongly
	// costs an army.
	const int32 AlreadyPresent = CountLiveUnitsOfKind(Latched.TriggerKind);
	if (AlreadyPresent >= Latched.TriggerAtLeast)
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Deferred intent NOT latched - the trigger is already satisfied (%d '%s' alive, threshold %d). ⚠️ That is what a misread relative 'N MORE' looks like (the eval's DEV-11), so the game RE-ASKS instead of firing. NOTHING was executed and nothing is waiting."),
			AlreadyPresent, *Latched.TriggerKind.ToString(), Latched.TriggerAtLeast);

		// The EXISTING how-many clarification. ⛔ No new player surface (§3).
		FSiegeAssistantMessageArgs AskArgs;
		AskArgs.Command = Latched;
		AskArgs.Requested = Latched.TriggerAtLeast;
		AskArgs.Available = AlreadyPresent;
		AskArgs.Kind = Latched.TriggerKind;

		EnterClarify(ESiegeAssistantReasonCode::AskHowMany, AskArgs);
		PendingShortfallIndex = INDEX_NONE;
		return;
	}

	DeferredCommand = Latched;
	bDeferredIntentLatched = true;
	DeferredIntentExpiryTime = World->GetTimeSeconds() + static_cast<double>(DeferredIntentTTLSeconds);

	// ⚠️ THE TIMER IS ARMED ONLY WHILE LATCHED, AND ClearDeferredIntent IS THE ONLY
	// THING THAT STOPS IT. A poll that outlives its latch is a component doing work
	// nobody asked for, forever.
	// ⛔ The period is clamped before the divide - a 0 Hz tunable would otherwise
	// produce an infinity and a timer that never fires or fires every frame.
	const float PollPeriod = 1.f / FMath::Max(DeferredIntentPollHz, 0.1f);
	World->GetTimerManager().SetTimer(DeferredIntentPollTimer, this,
		&USiegeAssistantComponent::PollDeferredIntent, PollPeriod, /*bLoop*/ true);

	SetState(ESiegeAssistantState::Deferred);

	FSiegeAssistantMessageArgs ArmedArgs;
	ArmedArgs.Command = Latched;
	ArmedArgs.Requested = Latched.TriggerAtLeast;
	ArmedArgs.Available = AlreadyPresent;
	ArmedArgs.Kind = Latched.TriggerKind;

	PendingArgs = ArmedArgs;
	PushMessage(ESiegeAssistantReasonCode::DeferredArmed, ArmedArgs);

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Deferred intent LATCHED: fire once at least %d '%s' exist (%d now), TTL %.0f s, polling at %.2f Hz. ⛔ It will RE-RESOLVE and return to AwaitConfirm - it never executes blind, and that is true with the confirm toggle OFF as well."),
		Latched.TriggerAtLeast, *Latched.TriggerKind.ToString(), AlreadyPresent, DeferredIntentTTLSeconds, DeferredIntentPollHz);
}

void USiegeAssistantComponent::PollDeferredIntent()
{
	if (!bDeferredIntentLatched)
	{
		// A poll with no latch means the timer outlived its reason. Stop it.
		ClearDeferredIntent();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		ClearDeferredIntent();
		return;
	}

	if (bAssistantFaulted)
	{
		// MarkAssistantFaulted already drops the latch; this is the belt to that
		// braces, and it costs one branch per second.
		ClearDeferredIntent();
		return;
	}

	const FSiegeAssistantCommand Latched = DeferredCommand;

	// ── TTL FIRST, SO AN EXPIRED ORDER CANNOT FIRE ON THE SAME TICK IT DIES ───
	if (World->GetTimeSeconds() >= DeferredIntentExpiryTime)
	{
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Deferred intent EXPIRED after %.0f s waiting for %d '%s'. NOTHING was executed."),
			DeferredIntentTTLSeconds, Latched.TriggerAtLeast, *Latched.TriggerKind.ToString());

		FSiegeAssistantMessageArgs ExpiredArgs;
		ExpiredArgs.Requested = Latched.TriggerAtLeast;
		ExpiredArgs.Kind = Latched.TriggerKind;

		ClearDeferredIntent();
		ClearPendingIntent();
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::DeferredExpired, ExpiredArgs);
		return;
	}

	// ── THE TRIGGER ───────────────────────────────────────────────────────────
	const int32 LiveCount = CountLiveUnitsOfKind(Latched.TriggerKind);
	if (LiveCount < Latched.TriggerAtLeast)
	{
		// Keep waiting. ⚠️ Deliberately silent - a log line per second for two
		// minutes would bury the lines that matter.
		return;
	}

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Deferred trigger MET (%d '%s' alive, threshold %d) - RE-RESOLVING against the live board."),
		LiveCount, *Latched.TriggerKind.ToString(), Latched.TriggerAtLeast);

	// Stop the timer BEFORE anything downstream can re-enter this function.
	ClearDeferredIntent();

	// ── ⚠️ THE ONE PLACE A SECOND Capture() IS CORRECT, AND THE DISTINCTION IS
	//    WORTH STATING. The header forbids re-capturing FROM THE EXECUTOR MID-TURN
	//    ("a second survey mid-turn would silently answer a different question
	//    from the one the model was asked") - and this is not mid-turn. The turn
	//    ended up to DeferredIntentTTLSeconds ago and RE-RESOLVING AGAINST THE
	//    CURRENT BOARD IS THE ENTIRE POINT OF A DEFERRED ORDER. It is also still
	//    "per sentence, never per tick" (§4): one capture per FIRE, not one per
	//    poll - the poll itself only counts units and surveys nothing.
	if (!CaptureTurnSnapshot())
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("A deferred intent fired but the board could not be re-surveyed - REFUSED. NOTHING was executed."));

		ClearPendingIntent();
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::RefusedAssistantUnavailable);
		return;
	}

	// The trigger is spent. Clearing it is what stops the fired order from
	// re-entering step (3) and latching itself again, forever.
	FSiegeAssistantCommand FiredCommand = Latched;
	FiredCommand.TriggerKind = NAME_None;
	FiredCommand.TriggerAtLeast = 0;

	ClearPendingIntent();

	// ⛔ FORCED REVIEW, AND THIS IS THE WHOLE REASON bForceConfirmReview EXISTS. A
	// deferred order fires on a board the player has not looked at since they
	// typed it, which is the ONE case where the review is worth MORE, not less -
	// so it goes back to AwaitConfirm ⚠️ EVEN WITH THE CONFIRM TOGGLE OFF. The
	// full guard, the place eligibility and every executor check run again on the
	// FRESH snapshot on the way there.
	RouteParsedCommandInternal(FiredCommand, /*bForceConfirmReview*/ true);
}

void USiegeAssistantComponent::ClearDeferredIntent()
{
	// ⚠️ SAFE WHEN NOTHING IS LATCHED - five callers invoke it unconditionally
	// (EndPlay, CancelPressed, MarkAssistantFaulted, SubmitUtterance's Deferred
	// admission, and EnterDeferredIntent itself).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredIntentPollTimer);
	}
	DeferredIntentPollTimer.Invalidate();

	bDeferredIntentLatched = false;
	DeferredIntentExpiryTime = 0.0;
	DeferredCommand = FSiegeAssistantCommand();
}

int32 USiegeAssistantComponent::CountLiveUnitsOfKind(FName Kind) const
{
	if (Kind == NAME_None)
	{
		return 0;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	ETeamId OrderingTeam = ETeamId::Blue;
	if (!ResolveOrderingTeam(OrderingTeam))
	{
		return 0;
	}

	// ⛔ ONE FILTERED ITERATION, KEPT NOWHERE. Not a registry, not an actor cache,
	// not a dirty flag, not a subscription list - CONVENTIONS §4 rejects all four
	// ON SIGHT, and a deferred trigger is exactly the feature that would tempt
	// somebody to add one.
	//
	// ⚠️ EXISTENCE, NOT ORDERABILITY. "Fire once at least N of these EXIST" is
	// what a trigger means - which is also why ValidateCommandAgainstSnapshot
	// deliberately does not validate TriggerKind: a trigger kind that is absent
	// RIGHT NOW is the whole point of waiting.
	int32 Count = 0;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		const ASummonedUnit* Unit = *It;
		if (!IsValid(Unit) || Unit->IsUnitDead())
		{
			continue;
		}

		if (Unit->GetTeamId() != OrderingTeam)
		{
			continue;
		}

		// FName comparison is case-insensitive, so the snapshot's lower-cased
		// canonical symbol matches the unit's card row name with no second
		// transform (see SelectUnitsForOrder).
		if (Unit->GetCardID() != Kind)
		{
			continue;
		}

		++Count;
	}

	return Count;
}

void USiegeAssistantComponent::AbortInFlightRequest()
{
	// ⚠️ SAFE WHEN NOTHING IS IN FLIGHT, and three callers depend on it (EndPlay,
	// CancelPressed, NotifyConsoleClosed, MarkAssistantFaulted).
	//
	// ⚠️ IT GUARDS ON OUR OWN DISPATCH FLAG, NOT ONLY ON IsBusy(). IsBusy() is a
	// property of the SUBSYSTEM; cancelling on it alone would let this component
	// abort a request it did not start. bModelDispatchedThisTurn is the only
	// evidence that the in-flight request is ours.
	if (!bModelDispatchedThisTurn)
	{
		return;
	}

	USiegeLlamaSubsystem* Llama = ResolveLlamaSubsystem();
	if (!Llama || !Llama->IsBusy())
	{
		return;
	}

	// ⚠️ A LATENCY COURTESY ON TOP OF A CORRECTNESS GUARANTEE, NOT INSTEAD OF ONE.
	// HandleModelCompletion's turn-id check already makes a late answer harmless,
	// so this is about freeing the worker sooner - which matters because queue
	// depth is 1 and the next sentence cannot start until it is free.
	Llama->CancelActiveRequest();

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Cancelled the in-flight request for turn %d. Any answer that still arrives is dropped by the turn-id guard."), TurnId);
}

// ---------------------------------------------------------------------------
// RESOLUTION HELPERS - and the ONE toggle read in the whole feature
// ---------------------------------------------------------------------------

bool USiegeAssistantComponent::IsConfirmBeforeExecuteEnabled() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInst = World ? World->GetGameInstance() : nullptr;
	const USiegeSettingsSubsystem* Settings = GameInst ? GameInst->GetSubsystem<USiegeSettingsSubsystem>() : nullptr;

	// ⛔ THE FALLBACK IS true (CONFIRM ON), NEVER false ("Settings screen..." §5 +
	// TASK-436's published contract). An unresolvable subsystem must degrade to
	// MORE human review, not less: silently executing because a LOOKUP failed is
	// precisely the failure this feature exists to prevent. ⚠️ Anyone tempted to
	// "simplify" this to `Settings && Settings->...` has inverted the safety
	// direction with a two-character edit.
	return Settings ? Settings->IsAssistantConfirmEnabled() : true;
}

USiegeLlamaSubsystem* USiegeAssistantComponent::ResolveLlamaSubsystem() const
{
	// ⚠️ RESOLVED LIVE, NEVER CACHED. The subsystem lives on the GameInstance and
	// survives level travel, but a cached raw pointer across a travel is exactly
	// the class of bug the Input-mode ownership law keeps re-teaching. The lookup
	// is a map read.
	const UWorld* World = GetWorld();
	UGameInstance* GameInst = World ? World->GetGameInstance() : nullptr;
	return GameInst ? GameInst->GetSubsystem<USiegeLlamaSubsystem>() : nullptr;
}

ASiegePlayerController* USiegeAssistantComponent::ResolveOrderingController() const
{
	// This component is a default subobject on ASiegePlayerController (TASK-440),
	// so the owner IS the ordering controller. ⛔ GetFirstPlayerController() is
	// BANNED in gameplay code (the M8 TEAM LAW) and is not used here.
	return Cast<ASiegePlayerController>(GetOwner());
}

FString USiegeAssistantComponent::GetPendingConfirmSummary() const
{
	if (State != ESiegeAssistantState::AwaitConfirm || !bConfirmPromptUp)
	{
		return FString();
	}

	return DescribeCommandForPlayer(PendingArgs.Command).ToString();
}

// ---------------------------------------------------------------------------
// THE CONSOLE-WIDGET JOIN - TASK-443 implements it, ⛔ TASK-453 calls it
// ---------------------------------------------------------------------------

void USiegeAssistantComponent::AttachConsoleWidget(USiegeAssistantConsoleWidget* InWidget)
{
	if (!InWidget)
	{
		// NULL-SAFE BY CONTRACT. ⚠️ It deliberately does NOT detach: "attach
		// nothing" and "let go of what you had" are different requests, and
		// DetachConsoleWidget() is the second one.
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("AttachConsoleWidget(null) ignored - nothing attached and nothing detached. Call DetachConsoleWidget() to let go of a widget."));
		return;
	}

	// IDEMPOTENT AND RE-TARGETABLE IN ONE STROKE: every binding is dropped before
	// any is made, so calling this ten times on the same widget leaves exactly one
	// of each, and pointing it at a NEW widget cannot leave the old one listening.
	DetachConsoleWidget();

	// ── ⛔ SEED FIRST, THEN BIND (the standing qa/TASK-005 major-2 lesson) ─────
	// The widget is created LAZILY at the first successful open, so it can be born
	// at any point in a conversation - including with an order already awaiting
	// confirmation, because a deferred intent's timer runs while the console is
	// shut. A bind-only join would leave that widget silently wrong until the next
	// event, which for a pending order may be never.
	InWidget->SetAssistantState(GetStateAsByte(), GetStateLabel());
	InWidget->SetConsoleEnabled(IsConsoleAvailable(),
		IsConsoleAvailable() ? FString() : SiegeAssistantReasonTemplate(ESiegeAssistantReasonCode::RefusedAssistantUnavailable).ToString());

	if (!LastMessage.IsEmpty())
	{
		InWidget->ShowTranscriptLine(LastMessage);
	}

	const FString PendingSummary = GetPendingConfirmSummary();
	if (!PendingSummary.IsEmpty())
	{
		InWidget->ShowConfirmPrompt(PendingSummary);
	}
	else
	{
		InWidget->HideConfirmPrompt();
	}

	// ── INBOUND: the widget's four outbound delegates -> this FSM ─────────────
	// TASK-444's header declares these under "the owning USiegeAssistantComponent
	// binds these", so this is the shipped design intent rather than a new one.
	InWidget->OnConsoleSubmitted.AddDynamic(this, &USiegeAssistantComponent::SubmitUtterance);
	InWidget->OnConsoleConfirmed.AddDynamic(this, &USiegeAssistantComponent::ConfirmPressed);

	// ⛔ THIS BINDING IS LIVE API WITH NO LIVE CALLER IN v1, AND THAT IS RULED,
	// NOT ROTTEN. DO NOT "CLEAN IT UP". (CONVENTIONS AS-§6 RULING A-2, amended
	// 2026-08-04 by Jonathan's ruling 3: "there is no cancel button (they just
	// simply close the chat box)".)
	//   · The console's Cancel BUTTON is gone, so the only thing that ever
	//     broadcast OnConsoleCancelled is gone with it. The delegate, the widget's
	//     CancelPressed() and this component's CancelPressed() all SURVIVE with
	//     their exact signatures - deleting shipped BlueprintCallable public API
	//     to remove a button is a breaking change bought for nothing.
	//   · They join ToggleConsole() in the deliberately-uncalled set: reachable
	//     from Blueprint, from a future WBP_AssistantConsole, from a gamepad path
	//     or from an accessibility path.
	//   · ⚠️ WHAT THIS COSTS, NAMED RATHER THAN HIDDEN: CancelPressed()'s
	//     Thinking-ABORT and Deferred-DROP handling is now unreachable FROM THE
	//     CONSOLE. Both are correct and both stay. The close path
	//     (NotifyConsoleClosed) covers the case that actually loses an order -
	//     AwaitConfirm - and prints the same Cancelled line for it.
	InWidget->OnConsoleCancelled.AddDynamic(this, &USiegeAssistantComponent::CancelPressed);

	InWidget->OnConsoleOpenChanged.AddDynamic(this, &USiegeAssistantComponent::HandleConsoleOpenChanged);

	// ── OUTBOUND: this FSM's delegates -> the widget's inbound API ────────────
	// ⚠️ EVERY ONE IS A DIRECT FORWARD WITH NO ADAPTER, because TASK-442 matched
	// these signatures to TASK-444's character-for-character. A translation layer
	// is where two tasks' assumptions get to disagree quietly.
	OnAssistantStateChanged.AddDynamic(InWidget, &USiegeAssistantConsoleWidget::SetAssistantState);
	OnAssistantMessage.AddDynamic(InWidget, &USiegeAssistantConsoleWidget::ShowTranscriptLine);
	OnAssistantAvailabilityChanged.AddDynamic(InWidget, &USiegeAssistantConsoleWidget::SetConsoleEnabled);
	OnAssistantConfirmPromptShown.AddDynamic(InWidget, &USiegeAssistantConsoleWidget::ShowConfirmPrompt);
	OnAssistantConfirmPromptHidden.AddDynamic(InWidget, &USiegeAssistantConsoleWidget::HideConfirmPrompt);

	AttachedConsoleWidget = InWidget;

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Console widget '%s' ATTACHED and SEEDED (state=%s, available=%s, pending_confirm=%s). 8 forwards bound, 0 adapters."),
		*GetNameSafe(InWidget), SiegeAssistantComponentInternal::StateName(State),
		IsConsoleAvailable() ? TEXT("yes") : TEXT("no"),
		PendingSummary.IsEmpty() ? TEXT("none") : TEXT("YES - seeded"));
}

void USiegeAssistantComponent::DetachConsoleWidget()
{
	USiegeAssistantConsoleWidget* Widget = AttachedConsoleWidget.Get();
	if (!Widget)
	{
		// Nothing attached, or it has already been destroyed - in which case its
		// own delegates died with it and ours compact themselves.
		AttachedConsoleWidget = nullptr;
		return;
	}

	// ⚠️ RemoveAll BY OBJECT rather than RemoveDynamic PER FUNCTION, deliberately:
	// a per-function list has to be kept in step with the bind list by hand, and
	// the failure mode of getting that wrong is a SURVIVING binding into a widget
	// that is supposed to be gone - silent, and exactly the shape this project
	// keeps paying for.
	OnAssistantStateChanged.RemoveAll(Widget);
	OnAssistantMessage.RemoveAll(Widget);
	OnAssistantAvailabilityChanged.RemoveAll(Widget);
	OnAssistantConfirmPromptShown.RemoveAll(Widget);
	OnAssistantConfirmPromptHidden.RemoveAll(Widget);

	Widget->OnConsoleSubmitted.RemoveAll(this);
	Widget->OnConsoleConfirmed.RemoveAll(this);
	Widget->OnConsoleCancelled.RemoveAll(this);
	Widget->OnConsoleOpenChanged.RemoveAll(this);

	AttachedConsoleWidget = nullptr;

	UE_LOG(LogSiegeAssistant, Verbose, TEXT("Console widget '%s' DETACHED - all 8 forwards dropped, both directions."), *GetNameSafe(Widget));
}

USiegeAssistantConsoleWidget* USiegeAssistantComponent::GetAttachedConsoleWidget() const
{
	return AttachedConsoleWidget.Get();
}

void USiegeAssistantComponent::HandleConsoleOpenChanged(bool bOpen)
{
	// The one arity adapter in the join: ONE widget delegate, TWO component entry
	// points. ⛔ It adds no policy - NotifyConsoleOpened / NotifyConsoleClosed own
	// every decision, including the one that a CLOSE is not a CANCEL.
	if (bOpen)
	{
		NotifyConsoleOpened();
	}
	else
	{
		NotifyConsoleClosed();
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// THE FSM - TASK-442's, and finished
// ═══════════════════════════════════════════════════════════════════════════

void USiegeAssistantComponent::SetState(ESiegeAssistantState NewState)
{
	if (State == NewState)
	{
		// The delegate law: broadcast on an ACTUAL change, never on a no-op write.
		// A widget that is taught to expect redundant pushes learns to ignore them.
		return;
	}

	UE_LOG(LogSiegeAssistant, Verbose, TEXT("FSM %s -> %s (turn %d)."),
		SiegeAssistantComponentInternal::StateName(State), SiegeAssistantComponentInternal::StateName(NewState), TurnId);

	State = NewState;

	// ⚠️ uint8 + a game-authored label, NEVER the enum - the widget-param law.
	// The signature is USiegeAssistantConsoleWidget::SetAssistantState's, so the
	// wiring is a forward and never a translation.
	OnAssistantStateChanged.Broadcast(static_cast<uint8>(State), SiegeAssistantStateLabel(State).ToString());
}

void USiegeAssistantComponent::BeginTurn()
{
	// ⚠️ THE ONLY PLACE TurnId MOVES. It identifies a MODEL turn: the
	// short-circuit deliberately does not bump it, because a turn that never
	// reaches the model has no completion that could arrive late.
	++TurnId;
	bModelDispatchedThisTurn = false;
}

void USiegeAssistantComponent::ClearPendingIntent()
{
	PendingArgs = FSiegeAssistantMessageArgs();
	PendingReason = ESiegeAssistantReasonCode::None;
	PendingShortfallIndex = INDEX_NONE;
}

void USiegeAssistantComponent::EnterFailed(ESiegeAssistantReasonCode Code)
{
	// ⚠️ THIS IS A TURN FAILING, NOT THE SESSION. bAssistantFaulted is untouched
	// here on purpose: a single unparseable answer must not cost the player their
	// console for the rest of the match. MarkAssistantFaulted is the latch, and
	// only a real fault calls it.
	ClearPendingIntent();
	SetState(ESiegeAssistantState::Failed);
	PushMessage(Code);
}

void USiegeAssistantComponent::EnterClarify(ESiegeAssistantReasonCode Code, const FSiegeAssistantMessageArgs& Args)
{
	// ⚠️ DOES NOT TOUCH PendingShortfallIndex - the caller sets it, because only
	// the caller knows which entry of the selection the question is about.
	PendingArgs = Args;
	PendingReason = Code;

	SetState(ESiegeAssistantState::Clarify);
	PushMessage(Code, Args);
}

bool USiegeAssistantComponent::TryShortCircuitClarification(const FString& Utterance)
{
	// ── THE ~15-LINE SHORT-CIRCUIT (task spec §4) ─────────────────────────────
	// Not an optimisation: a latency AND a reliability decision. The cheapest
	// correct answer is the one that never enters the model, and a turn the model
	// never sees is a turn it cannot get wrong.
	if (State != ESiegeAssistantState::Clarify)
	{
		return false;
	}

	// ⚠️ ONLY THE SHORTFALL. It is the one question whose answer the game already
	// holds ("you asked for 10 and 8 can take that order" is arithmetic). The
	// which_unit / which_place / how_many asks are the MODEL's questions about a
	// sentence, and answering them locally would be a keyword parser pretending to
	// be a translator.
	if (PendingReason != ESiegeAssistantReasonCode::ShortfallCount ||
		!PendingArgs.Command.Counts.IsValidIndex(PendingShortfallIndex))
	{
		return false;
	}

	ESiegeAssistantClarifyReply ReplyKind = ESiegeAssistantClarifyReply::Unrecognized;
	int32 Quantity = 0;
	if (!SiegeAssistantParseClarificationReply(Utterance, ReplyKind, Quantity))
	{
		// ⚠️ THE COMMON EXIT, AND THE SAFE ONE: anything the classifier is not
		// certain of becomes an honest FRESH SINGLE-TURN model call.
		return false;
	}

	if (ReplyKind == ESiegeAssistantClarifyReply::Negative)
	{
		UE_LOG(LogSiegeAssistant, Log, TEXT("Short-circuit: clarification declined by the player. NO model call. Nothing was executed."));
		ClearPendingIntent();
		SetState(bConsoleOpen ? ESiegeAssistantState::Composing : ESiegeAssistantState::Idle);
		PushMessage(ESiegeAssistantReasonCode::Cancelled);
		return true;
	}

	if (ReplyKind == ESiegeAssistantClarifyReply::Affirmative)
	{
		// "yes" means "the number you just told me", which the game computed.
		Quantity = PendingArgs.Available;
	}
	else if (ReplyKind != ESiegeAssistantClarifyReply::Quantity)
	{
		return false;
	}

	if (Quantity > PendingArgs.Available)
	{
		// Still short - re-ask with the NEW number, still with zero model calls.
		FSiegeAssistantMessageArgs NextArgs = PendingArgs;
		NextArgs.Requested = Quantity;

		const int32 KeptIndex = PendingShortfallIndex;
		EnterClarify(ESiegeAssistantReasonCode::ShortfallCount, NextArgs);
		PendingShortfallIndex = KeptIndex;
		return true;
	}

	// Accepted for THIS kind. Apply the resolved quantity to the pending order.
	PendingArgs.Command.Counts[PendingShortfallIndex] = Quantity;

	// ⚠️ THEN RE-RUN THE ARITHMETIC, BECAUSE A MULTI-KIND SELECTION CAN BE SHORT
	// IN MORE THAN ONE PLACE. FindShortfall reports the FIRST short kind, so
	// answering it settles one entry and says nothing about the rest; "send 10
	// footmen and 5 sorcerers" with 8 and 2 available is two questions, not one.
	// Walking off the end of the first answer would hand the confirm step a count
	// the player never agreed to - quietly, and inside an order they are about to
	// accept. Still ZERO model calls.
	int32 NextShortfallIndex = INDEX_NONE;
	int32 NextAvailable = 0;
	if (FindShortfall(PendingArgs.Command, NextShortfallIndex, NextAvailable))
	{
		FSiegeAssistantMessageArgs NextArgs = PendingArgs;
		NextArgs.Requested = PendingArgs.Command.Counts[NextShortfallIndex];
		NextArgs.Available = NextAvailable;
		NextArgs.Kind = PendingArgs.Command.Kinds[NextShortfallIndex];

		EnterClarify(ESiegeAssistantReasonCode::ShortfallCount, NextArgs);
		PendingShortfallIndex = NextShortfallIndex;

		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Short-circuit: one shortfall settled at %d, and the selection is short on '%s' too (%d requested, %d orderable). Asking again - still NO model call."),
			Quantity, *NextArgs.Kind.ToString(), NextArgs.Requested, NextAvailable);
		return true;
	}

	// Every entry is satisfiable. Hand the order to the executor lane - ⛔ which
	// still runs the guard, the eligibility checks and the confirm step. A
	// short-circuited clarification skips the MODEL, never a machine check.
	PendingArgs.Requested = Quantity;
	PendingReason = ESiegeAssistantReasonCode::None;
	PendingShortfallIndex = INDEX_NONE;

	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Short-circuit: clarification resolved to %d with NO model call (turn %d was never re-dispatched)."), Quantity, TurnId);

	// ⚠️ A COPY, NOT PendingArgs.Command ITSELF. RouteParsedCommand is TASK-443's
	// and is free to reset the pending state; handing it a reference INTO that
	// state would leave it reading a cleared struct partway through its own body.
	// Both call sites of this function pass a local for the same reason.
	const FSiegeAssistantCommand ResolvedCommand = PendingArgs.Command;
	RouteParsedCommand(ResolvedCommand);
	return true;
}

bool USiegeAssistantComponent::FindShortfall(const FSiegeAssistantCommand& Command, int32& OutKindIndex, int32& OutAvailable) const
{
	OutKindIndex = INDEX_NONE;
	OutAvailable = 0;

	if (!Snapshot)
	{
		return false;
	}

	// ⚠️ ZONE VERBS ONLY. GetOrderableCount is IsGroupCommandEligible's tally -
	// "may I SEND these?" - and Follow's tally (IsFollowCommandEligible, the
	// Cleric-follows-but-cannot-hold split) is not exposed. Answering a follow
	// shortfall from the orderable column would be a silent wrong answer, so
	// Follow gets no shortfall clarification in v1.
	if (Command.Intent != ESiegeAssistantIntent::Send &&
		Command.Intent != ESiegeAssistantIntent::Guard &&
		Command.Intent != ESiegeAssistantIntent::Ambush)
	{
		return false;
	}

	if (Command.Kinds.Num() != Command.Counts.Num())
	{
		// Defensive: the invariant already ran. Never truncate to "fix" it.
		return false;
	}

	int32 FirstShortIndex = INDEX_NONE;
	int32 FirstShortAvailable = 0;

	for (int32 Index = 0; Index < Command.Kinds.Num(); ++Index)
	{
		const int32 Orderable = Snapshot->GetOrderableCount(Command.Kinds[Index]);

		if (Orderable <= 0)
		{
			// ⛔ NOT OURS. A kind with nothing orderable is the NON-ORDERABLE-KIND
			// GUARD's to refuse, and it refuses the command AS A WHOLE through the
			// existing unsupported-ask outcome ("Settings screen..." §6). Two
			// surfaces for one situation is how a player gets told two different
			// things about the same order - so this path declines entirely and
			// lets the guard rule.
			return false;
		}

		const int32 Requested = Command.Counts[Index];

		// 0 == "all", which can never be short of itself.
		if (Requested > 0 && Requested > Orderable && FirstShortIndex == INDEX_NONE)
		{
			FirstShortIndex = Index;
			FirstShortAvailable = Orderable;
		}
	}

	if (FirstShortIndex == INDEX_NONE)
	{
		return false;
	}

	OutKindIndex = FirstShortIndex;
	OutAvailable = FirstShortAvailable;
	return true;
}

FString USiegeAssistantComponent::BuildPendingLine() const
{
	// ⛔ PROMPT TEXT. ASCII only, terse, byte-counted - it becomes the VALUE of
	// Zone C's `pending:` key, which the snapshot caps at MaxUtteranceBytes and
	// now reports when it cuts. CONVENTIONS §1 illustrates this line with a
	// multiplication sign; `x` is written here instead, and the difference is
	// deliberate: the document's glyph is prose, this one is a byte in a token
	// budget.
	//
	// ⛔ NO PARAMETERS (mechanism #3). Its only inputs are PendingArgs
	// (uint8/int32/FName) and PendingReason (an enum), so there is no signature
	// through which model text could be routed into the prompt.
	const FSiegeAssistantCommand& Command = PendingArgs.Command;

	FString Line;

	if (Command.Intent != ESiegeAssistantIntent::None)
	{
		Line += SiegeAssistantIntentToSymbol(Command.Intent);

		const int32 SelectionNum = FMath::Min(Command.Kinds.Num(), Command.Counts.Num());
		for (int32 Index = 0; Index < SelectionNum; ++Index)
		{
			Line += (Index == 0) ? TEXT(" ") : TEXT(", ");
			Line += Command.Kinds[Index].ToString();

			if (Command.Counts[Index] == 0)
			{
				Line += TEXT(" xall");
			}
			else
			{
				Line.Appendf(TEXT(" x%d"), Command.Counts[Index]);
			}
		}

		if (Command.Where != NAME_None)
		{
			Line.Appendf(TEXT(" -> %s"), *Command.Where.ToString());
		}
	}

	const FString Problem = SiegeAssistantComponentInternal::BuildProblemClause(PendingReason, PendingArgs.Available, PendingArgs.Kind);
	if (!Problem.IsEmpty())
	{
		if (!Line.IsEmpty())
		{
			Line += TEXT(" ; ");
		}
		Line += Problem;
	}

	// Empty ⇒ Zone C prints `pending: none`, which is the fixed-key law working
	// as intended: the key is always emitted, only its value varies.
	return Line;
}

// ---------------------------------------------------------------------------
// SNAPSHOT + PROMPT
// ---------------------------------------------------------------------------

bool USiegeAssistantComponent::ResolveOrderingTeam(ETeamId& OutTeam) const
{
	const AController* OwningController = Cast<AController>(GetOwner());
	if (!OwningController)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("The assistant component is not on a controller - no ordering team, so no turn."));
		return false;
	}

	const ASiegePlayerState* OrderingState = Cast<ASiegePlayerState>(OwningController->PlayerState);
	if (!OrderingState)
	{
		// ⛔ NEVER GUESS. A default of Blue would survey the wrong army on a Red
		// controller and every answer downstream would be confidently wrong -
		// which is worse than refusing, and much harder to notice.
		UE_LOG(LogSiegeAssistant, Warning, TEXT("No ASiegePlayerState yet - refusing the turn rather than assuming a team."));
		return false;
	}

	OutTeam = OrderingState->GetTeam();
	return true;
}

void USiegeAssistantComponent::EnsureSnapshot()
{
	if (!Snapshot)
	{
		// ONE object for the life of the component. Capture() resets it first
		// thing, so nothing stale can survive a sentence. ⛔ This is not a cache
		// and not a registry - CONVENTIONS §4 rejects those on sight.
		Snapshot = NewObject<USiegeAssistantSnapshot>(this);
	}
}

bool USiegeAssistantComponent::CaptureTurnSnapshot()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("No world - refusing the turn."));
		return false;
	}

	ETeamId OrderingTeam = ETeamId::Blue;
	if (!ResolveOrderingTeam(OrderingTeam))
	{
		return false;
	}

	EnsureSnapshot();

	// ⛔ ONCE PER SENTENCE, NEVER PER TICK (CONVENTIONS §4). ~0.2 ms of actor
	// iteration, paid a handful of times per match, against one inference call.
	Snapshot->Capture(World, OrderingTeam);

	return true;
}

FString USiegeAssistantComponent::ComposeTurnPrompt(const FString& Utterance)
{
	// ⚠️ THE FIXED ZONE ORDER IS LOAD-BEARING (CONVENTIONS §8): Zone A is the
	// static prefix llama_memory_seq_rm keeps warm, Zone B is slow, Zone C is
	// fast. Reordering them, or making Zone A state-dependent, silently destroys
	// the KV reuse and shows up as a latency regression rather than a wrong
	// answer - which is exactly why it is written down at every site that could
	// do it.
	//
	// ⚠️ EACH ZONE IS BUILT EXACTLY ONCE. Building Zone C twice (once to log,
	// once to send) would double-count its truncation warnings and make the
	// first-execution audit report events that happened once as twice.
	const FString& ZoneA = GetCachedZoneA();
	const FString ZoneB = Snapshot->BuildZoneB();
	const FString ZoneC = Snapshot->BuildZoneC(Utterance, BuildPendingLine());

	ReportFirstCapture(ZoneA, ZoneB, ZoneC);

	FString Prompt;
	Prompt.Reserve(ZoneA.Len() + ZoneB.Len() + ZoneC.Len());
	Prompt += ZoneA;
	Prompt += ZoneB;
	Prompt += ZoneC;

	return Prompt;
}

#if !UE_BUILD_SHIPPING
FString USiegeAssistantComponent::DebugCaptureAndComposePrompt(const FString& RawUtterance)
{
	// ⚠️ DEFINED IMMEDIATELY BELOW ComposeTurnPrompt RATHER THAN IN HEADER ORDER,
	// ON PURPOSE. The whole safety argument for this function is "IT DELEGATES",
	// and the cheapest way to keep that checkable by eye is to put the two
	// functions where one screen shows both. ⛔ The argument itself lives on the
	// DECLARATION - the SpawnGroupCircleDecal idiom of putting the access
	// rationale where the next reader meets it, not where the body hides it.
	//
	// ⛔ THIS FUNCTION RUNS SubmitUtterance's STEPS 7 AND 8 AND NOTHING ELSE.
	// No BeginTurn, no TurnId, no dispatch, no SetState, no PushMessage.

	// ── MIRRORS SubmitUtterance GATE 1, WITH THE SAME SINGLE CALL ─────────────
	// ⚠️ TrimStartAndEnd() is the ONLY transform the shipped lane applies to the
	// raw string before step 8. Everything else the utterance undergoes
	// (SanitizeForPrompt, the MaxUtteranceBytes cap, the flattening warnings)
	// happens INSIDE BuildZoneC and is therefore INHERITED by delegating - never
	// repeated here, which would be the re-implementation this dump exists to
	// avoid measuring.
	const FString Utterance = RawUtterance.TrimStartAndEnd();
	if (Utterance.IsEmpty())
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("DebugCaptureAndComposePrompt REFUSED: empty utterance. The shipped lane discards it at gate 1 and NEVER reaches the composer, so there are no bytes to read."));
		return FString();
	}

	// ── MIRRORS SubmitUtterance GATE 3 (AUTHORITY, §7) ────────────────────────
	// ⚠️ REFUSAL-ONLY, AND IT IS ABOUT THE ARTIFACT RATHER THAN ABOUT PERMISSION:
	// on a client the composer is never reached at all, so a dump taken there
	// would describe a lane that does not run - which is precisely the
	// "the measured lane is not the shipped lane" defect this reading exists to
	// close, re-committed by the instrument built to close it.
	const AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("DebugCaptureAndComposePrompt REFUSED: no authority. The shipped lane refuses here too (gate 3), so this machine composes no prompt to read."));
		return FString();
	}

	// ── THE AT-REST WHITELIST ─────────────────────────────────────────────────
	// ⛔ Capture() RE-SURVEYS THE ONE SNAPSHOT OBJECT IN PLACE. Idle and Composing
	// hold nothing pending; Failed's turn is over and its intent cleared.
	// Thinking, AwaitConfirm, Clarify and Deferred each still consult the CURRENT
	// survey downstream (RouteParsedCommandInternal, ConfirmPressed's execution,
	// FindShortfall), and re-surveying under any of them would "silently answer a
	// different question from the one the model was asked" - GetTurnSnapshot()'s
	// own stated contract, which this must not breach from the outside either.
	//
	// ⚠️ A WHITELIST, NOT A BLACKLIST, AND THAT IS THE SAFE DIRECTION: a state
	// added later is REFUSED BY DEFAULT rather than admitted by omission.
	if (State != ESiegeAssistantState::Idle &&
		State != ESiegeAssistantState::Composing &&
		State != ESiegeAssistantState::Failed)
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("DebugCaptureAndComposePrompt REFUSED: the FSM is '%s', which is not at rest. Re-surveying now would replace the snapshot the live turn is still reading. Finish or cancel the turn and re-issue."),
			*GetStateLabel());
		return FString();
	}

	// ── STEP 7 - THE SNAPSHOT (USiegeAssistantSnapshot::Capture, reached through
	//    the ONE shipped caller, never directly) ─────────────────────────────
	if (!CaptureTurnSnapshot())
	{
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("DebugCaptureAndComposePrompt REFUSED: the board could not be surveyed (no world, or the ordering team did not resolve). Nothing composed - a refusal, never a guess."));
		return FString();
	}

	// ── STEP 8 - COMPOSE, BY DELEGATION ───────────────────────────────────────
	// ⛔ THE ONE COMPOSER, AND THIS LINE IS THE ENTIRE FUNCTION'S REASON TO EXIST:
	// the returned bytes ARE the shipped lane's, not a re-derivation that would
	// have to guess the vocabulary lane (the measured 3029-vs-5116 defect) and
	// the pending line, and would then be scored as if it were the artifact.
	// ⚠️ ReportFirstCapture() fires INSIDE this call when the session latch is
	// still unspent - see "what it consumes" on the declaration.
	return ComposeTurnPrompt(Utterance);
}
#endif // !UE_BUILD_SHIPPING

FString USiegeAssistantComponent::ComposeTurnGrammar() const
{
	if (!Snapshot)
	{
		return FString();
	}

	// ⚠️ GENERATED PER REQUEST FROM LIVE STATE - that is where the grounding
	// lives, not in the weights. GetUnitKinds() is NEVER truncated (unlike the
	// PRINTED roster), so a kind the prompt collapsed into `other_kinds:` is
	// still sayable; that asymmetry is deliberate and is the snapshot's law, not
	// this file's to correct.
	//
	// ⛔⭐ THE THIRD ARGUMENT IS THE SECOND TRAILING-DEFAULT HAZARD IN THIS FILE AND
	// IT IS THE ONE THAT WOULD HAVE KILLED THE WHOLE FEATURE SILENTLY (TASK-548,
	// AS-§21.9). USiegeAssistantGrammar::Build's RegionPlaceNames parameter is
	// TRAILING AND DEFAULTED, so this line compiled, linked and ran perfectly
	// without it - and produced a grammar with NO `inplace` rule, NO `zone`
	// alternation and NO `in` alternative on `who`. Constrained decoding cannot
	// sample a shape the grammar does not contain, so `{"in":"ancient_ground_near"}`
	// would have been UNREACHABLE AT RUNTIME no matter what the model wanted to
	// emit, and the parser, the validator, the snapshot and the executor would all
	// have sat there correct and unreached.
	//
	// ⚠️ AND THE FAILURE WOULD HAVE BEEN MISREAD, WHICH IS WHY IT IS WRITTEN DOWN
	// HERE RATHER THAN JUST FIXED: AS-§21.11 outcome 5 already warns that a rule
	// line is a weaker teaching signal than an exemplar for a brand-new output
	// shape, so "the model never emitted `in`" at Stage 5 would have been read as
	// the PROMPT under-teaching the shape - a conclusion about the model drawn from
	// a missing function argument. ⛔ This is the ONLY shipped (non-test) caller of
	// Build in the module; TASK-546 owns the builder but could not reach this line.
	// ⇒ ANY CALLER HOLDING A SNAPSHOT PASSES Snapshot->GetRegionPlaceNames().
	//
	// ✅ AN EMPTY LIST IS A LEGAL, HANDLED STATE, NOT A REASON TO GUARD HERE: a map
	// with no ancient ground and no capture zone publishes no regions, and the
	// builder then omits `zone` / `inplace` and the `who` alternative ENTIRELY -
	// which is required, because an empty alternation would leave `zone` undefined
	// and make the WHOLE grammar unparseable (AS-§21.4 - the `at_least` disaster).
	// The snapshot decides which places are region-bearing; this line only relays.
	return USiegeAssistantGrammar::Build(Snapshot->GetUnitKinds(), Snapshot->GetPlaceNames(), Snapshot->GetRegionPlaceNames());
}

const FString& USiegeAssistantComponent::GetCachedZoneA()
{
	EnsureSnapshot();

	if (!bZoneABuilt)
	{
		bZoneABuilt = true;
		CachedZoneA = Snapshot->BuildZoneA(GetVocabulary());

		if (CachedZoneA.IsEmpty())
		{
			UE_LOG(LogSiegeAssistant, Warning, TEXT("Zone A built EMPTY. The prompt has no system block, schema or few-shots; every answer after this is untethered."));
		}

		return CachedZoneA;
	}

#if !UE_BUILD_SHIPPING
	if (!bZoneAIdentityChecked)
	{
		// ⚠️ CONVENTIONS §8 states a QA CRITERION - "calling BuildZoneA twice in
		// one process returns byte-identical strings" - and until now nothing had
		// ever executed it. This runs it ONCE per session, on the second turn, at
		// the cost of one extra string build. "Observable" is not "observed", and
		// a criterion nothing runs is a sentence.
		bZoneAIdentityChecked = true;

		const FString Rebuilt = Snapshot->BuildZoneA(GetVocabulary());
		if (Rebuilt != CachedZoneA)
		{
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("ZONE A IS NOT BYTE-IDENTICAL ACROSS CALLS (%d chars then %d chars). Every cached KV prefix is thrown away on every turn, and this is a QA FAIL under CONVENTIONS 8 - the symptom is latency, not a wrong answer, which is why it needs a log line to be visible at all."),
				CachedZoneA.Len(), Rebuilt.Len());
		}
		else
		{
			UE_LOG(LogSiegeAssistant, Verbose, TEXT("Zone A byte-identity re-check PASSED (%d chars)."), CachedZoneA.Len());
		}
	}
#endif

	return CachedZoneA;
}

const USiegeAssistantVocabulary* USiegeAssistantComponent::GetVocabulary()
{
	if (bVocabularyResolved)
	{
		return ResolvedVocabulary;
	}

	// ⚠️ SET BEFORE THE LOAD, AND THAT IS THE POINT: a failed resolve is
	// remembered as a RESULT. Retrying on a later turn could succeed mid-session
	// and CHANGE ZONE A, which throws away every cached prefix. The snapshot's
	// caller contract is "pass the SAME vocabulary object every turn", and this
	// function honours it by deciding the lane exactly once.
	bVocabularyResolved = true;
	ResolvedVocabulary = VocabularyAsset.LoadSynchronous();

	if (ResolvedVocabulary)
	{
		// ⚠️ THE ASSET LANE OVERRIDES THE C++ DEFAULTS WHOLESALE - UnitSynonyms,
		// PlaceSynonyms and IntentSynonyms are all serialised properties, so a saved
		// asset replaces every row rather than adding to them. ⛔ THAT MEANS THE
		// ASSET LANE IS **NOT** THE LANE `zoneA_chars=5116` WAS MEASURED ON unless
		// the asset reproduces the constructor defaults byte for byte;
		// `Siegebound.Assistant.ZoneA.TwoLaneByteEquality` tests the DEFAULT lane and
		// says nothing about this one. Whoever authors DA_AssistantVocabulary is
		// therefore changing Zone A, which is a CONVENTIONS §12a event, not content.
		UE_LOG(LogSiegeAssistant, Log,
			TEXT("Assistant vocabulary: the ASSET lane is live (%s). CAUTION: an asset OVERRIDES the C++ constructor defaults wholesale, so Zone A is NOT necessarily the lane the eval ladder was measured on - CONVENTIONS 12a binds any eval claim to a PASSING ZoneA.TwoLaneByteEquality, and that test covers the DEFAULT lane only."),
			*GetNameSafe(ResolvedVocabulary));
		return ResolvedVocabulary;
	}

	// ⛔⛔ THE C++ FALLBACK. TASK-417's spec required this class to "ship with sane
	// C++ defaults so the feature works before any asset exists" (and its header
	// still says so) - the CLASS delivered that, and this CALL SITE did not use it.
	// The measured cost of that gap: the first live run printed `zoneA_chars=3029`
	// with `vocabulary=none`, which is exactly 5116 - 2092 + Len("none\n") - i.e.
	// the whole synonym table absent from the player's prompt, while every eval
	// number this project has (18 -> 19 -> 20/25) came from the SPIKE lane, which
	// carries that table baked into C++. ⇒ The shipped prompt was not the prompt
	// any number described (CONVENTIONS §12a, the lane clause).
	//
	// ⚠️ `NewObject`, NOT `GetMutableDefault`, AND THE REASON IS EVIDENTIARY RATHER
	// THAN STYLISTIC. `Siegebound.Assistant.ZoneA.TwoLaneByteEquality` builds its
	// comparison object with `NewObject<USiegeAssistantVocabulary>()`, so using the
	// same construction here makes that test's PASS a statement about the object
	// the shipped lane actually renders - not merely about one of the same class.
	// It also keeps a process-global CDO pointer out of a non-const member, where a
	// later edit through `ResolvedVocabulary` would corrupt the defaults for every
	// component in the process.
	//
	// Outered to `this` and held by the Transient UPROPERTY, so it is GC-rooted
	// twice over and dies with the component. Byte-stability is unaffected: the
	// rows come from the constructor, and BuildSynonymTable normalises order.
	ResolvedVocabulary = NewObject<USiegeAssistantVocabulary>(this);

	if (!bWarnedMissingVocabulary)
	{
		bWarnedMissingVocabulary = true;

		if (ResolvedVocabulary)
		{
			// ⚠️ `Log`, NOT `Warning`, AND IT IS A DELIBERATE SEVERITY CHANGE - see the
			// handoff, where it is declared. Before the fallback existed, a missing
			// asset silently gutted Zone A and a Warning was right. Now a missing asset
			// yields the C++ default table, which IS the lane every measurement was
			// taken on, so warning about it would flag the CORRECT state as a fault -
			// the "evidence-shaped false warning" CONVENTIONS §22 ranks as the worse
			// half of a stale claim. The line still names the lane, because §12a binds
			// eval claims to it and a reader must be able to tell which one ran.
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("DA_AssistantVocabulary did not resolve ('%s') - falling back to the C++ CONSTRUCTOR DEFAULTS, which is TASK-417's designed path and NOT a degradation. The synonym table is present and Zone A is the DEFAULT lane (the one ZoneA.TwoLaneByteEquality checks against the spike fixture). Decided ONCE and never retried: a mid-session success would change Zone A and throw away every cached KV prefix."),
				SiegeAssistantComponentInternal::VocabularyAssetPath);
		}
		else
		{
			// ⛔ THE ONLY REMAINING PATH TO AN EMPTY SYNONYM TABLE, AND IT KEEPS A
			// WARNING BECAUSE IT IS THE DEFECT THIS FUNCTION EXISTS TO CLOSE. If the
			// allocation itself fails, BuildZoneA(nullptr) prints `none` again and the
			// prompt silently stops being the measured lane - the exact failure that
			// went unnoticed for a whole batch. It must never be silent twice.
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("DA_AssistantVocabulary did not resolve ('%s') AND the C++ default vocabulary could not be allocated. Zone A's synonym block prints `none` - the prompt is the DEGRADED lane and NO eval figure describes it (CONVENTIONS 12a). Deterministic and one-shot; the KV prefix stays stable and accuracy is what pays."),
				SiegeAssistantComponentInternal::VocabularyAssetPath);
		}
	}

	return ResolvedVocabulary;
}

void USiegeAssistantComponent::ReportFirstCapture(const FString& ZoneA, const FString& ZoneB, const FString& ZoneC)
{
	if (bLoggedFirstCapture || !Snapshot)
	{
		return;
	}

	bLoggedFirstCapture = true;

	const int32 ZoneBCChars = ZoneB.Len() + ZoneC.Len();

	// ⚠️ THE FIRST-EXECUTION AUDIT ("Settings screen..." §7). USiegeAssistantSnapshot
	// had ZERO callers and ZERO tests until this component ran, and its truncation
	// logging had never emitted a line - so every "now observable" clause in
	// CONVENTIONS §8/§10 described code that had never executed. This line is what
	// TASK-447 quotes: real numbers off the live builder, not an impression.
	UE_LOG(LogSiegeAssistant, Log,
		TEXT("FIRST LIVE CAPTURE of USiegeAssistantSnapshot (CONVENTIONS \"Settings screen...\" 7 - the owed first-execution audit; this class had NO caller before this line ran).\n")
		TEXT("  zoneA_chars=%d  zoneB_chars=%d  zoneC_chars=%d  zoneB+C_chars=%d  SnapshotTrimBudgetChars=%d  headroom=%d\n")
		TEXT("  roster_kinds=%d  MaxRosterKinds=%d  roster_rows=%d  places=%d\n")
		TEXT("  Reference figures to compare against: the TASK-413 spike FIXTURE measured zoneB=68 zoneC=887 (B+C=955) on a 13-kind board. ⚠️ The old 'SHIPPED builder DERIVED at B+C=738' figure is DEAD and was removed: it was derived at MaxRosterKinds=8, which TASK-517 raised to 13 on Jonathan's 2026-08-04 ruling. The shipped builder's Zone C on a 13-kind board is now 887 of an 893-char budget (head 108 / roster 621 / tail 158, re-derived from this builder's own format at TASK-517) - i.e. ~6 chars of headroom, an accepted risk. ⛔ The shipped zoneB has still NEVER been printed, so there is no shipped B+C figure to compare against; taking one is what the OWED READING below is for, and this remains the first MEASURED shipped-builder reading.\n")
		TEXT("  >>> OWED READING - THIS IS THE LINE TASK-455 NEEDED AND COULD NOT TAKE. ZoneBCharReserve is %d and this board's zoneB_chars is %d, i.e. it over-charges Zone B by %d chars and steals exactly that many from the roster. It is 192 ONLY because no PRINTED figure from THIS builder existed (the 68/71 on record were printed by the SPIKE's AppendZoneB, a different lane). Size it from this figure PLUS the widest value each of the four fixed keys can take - both castles at 100%%, mid: neutral, a four-digit gold band - because one live board is a sample, not a worst case. Lowering it can only WIDEN the roster.\n")
		TEXT("  The character figures above are a TRIM budget, not the cap: the authority is the plugin's MaxSnapshotTokens=400, counted by llama_tokenize, and it REJECTS rather than truncates.\n")
		TEXT("  Any roster/utterance truncation is reported by the snapshot's own Warning lines; their absence here means nothing was cut on this board.\n")
		TEXT("--- ZONE B ---\n%s--- ZONE C ---\n%s--- END ---"),
		ZoneA.Len(), ZoneB.Len(), ZoneC.Len(), ZoneBCChars,
		USiegeAssistantSnapshot::SnapshotTrimBudgetChars, USiegeAssistantSnapshot::SnapshotTrimBudgetChars - ZoneBCChars,
		Snapshot->GetUnitKinds().Num(), USiegeAssistantSnapshot::MaxRosterKinds, Snapshot->GetRoster().Num(), Snapshot->GetPlaceNames().Num(),
		USiegeAssistantSnapshot::ZoneBCharReserve, ZoneB.Len(), USiegeAssistantSnapshot::ZoneBCharReserve - ZoneB.Len(),
		*ZoneB, *ZoneC);
}

void USiegeAssistantComponent::EnsureStaticPrefixRegistered(USiegeLlamaSubsystem* Llama)
{
	if (bStaticPrefixRegistered || Llama == nullptr)
	{
		return;
	}

	// ⛔ THE IsReady() GATE IS THE CORRECTNESS OF THIS FUNCTION, NOT AN
	// OPTIMISATION. SiegeLlamaSubsystem.cpp's ApplyPendingStaticPrefix DISCARDS a
	// prefix that arrives before the model has loaded - it has nothing to tokenize
	// with - and says so at Warning on the WORKER. SetStaticPrefix would still have
	// returned TRUE, so an eager call here would latch, log a cheerful success, and
	// leave BOTH snapshot budgets inert for the whole session. ⚠️ That is precisely
	// the "a guard that cannot see says nothing and reads as a pass" shape this
	// task exists to close; recreating it one layer up would be the worst possible
	// way to close it.
	//
	// Deliberately silent and deliberately NOT latched: "still loading" is the
	// normal state for the first seconds of a match and is not a fault. The next
	// console open or the next turn tries again.
	if (!Llama->IsReady())
	{
		return;
	}

	// ⚠️ THE REGISTERED STRING MUST BE THE PROMPT'S ACTUAL PREFIX, BYTE FOR BYTE.
	// The plugin's char pre-filter tests Prompt.StartsWith(Prefix, CaseSensitive)
	// and SKIPS ITSELF on a mismatch, so a "close enough" string would silently
	// disable the very guard this call exists to arm. GetCachedZoneA() is the SAME
	// call ComposeTurnPrompt makes, and Zone A is byte-identical for the life of the
	// process by CONVENTIONS §8 law (re-checked once per session in GetCachedZoneA)
	// - which is what makes "the same string" structural rather than a promise.
	const FString& ZoneA = GetCachedZoneA();
	if (ZoneA.IsEmpty())
	{
		// GetCachedZoneA has already reported this at Warning, and SetStaticPrefix
		// rejects an empty string anyway. A second line here would report one fault
		// twice, which is how a log stops being read.
		return;
	}

	if (!Llama->SetStaticPrefix(ZoneA))
	{
		// ⛔ NOT LATCHED. Past the IsReady() gate the only reachable refusal is
		// "a request is in flight", which is transient by definition - so this
		// retries on the next open or turn. The WARNING is latched separately so a
		// retry can never become per-sentence spam.
		if (!bWarnedStaticPrefixRefused)
		{
			bWarnedStaticPrefixRefused = true;
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("SetStaticPrefix was REFUSED (Zone A, %d chars). Until it succeeds the snapshot budgets stay INERT - neither MaxSnapshotTokens nor SnapshotPreFilterMaxChars can identify the Zone B+C region without the prefix - and the CONVENTIONS 8 Zone-A size assertion runs with ZoneA_tokens treated as 0. The CONTEXT budget is still enforced on every request, so nothing reaches llama_decode unguarded. This is RETRIED on the next console open or turn; logged once."),
				ZoneA.Len());
		}
		return;
	}

	bStaticPrefixRegistered = true;

	// ⚠️ "SUBMITTED", NOT "APPLIED", AND THE WORDING IS DELIBERATE. The worker
	// tokenizes it on its own thread and prints the MEASURED zoneA token count plus
	// the re-run budget assertion (LogSiegeLlama). This lane cannot read that result
	// back through the pinned §9 surface, so the plugin's line is the one that
	// proves the guards actually went live. ⛔ Do not claim these budgets are
	// enforced on the strength of THIS line alone.
	UE_LOG(LogSiegeAssistant, Log,
		TEXT("Static prefix SUBMITTED to USiegeLlamaSubsystem: Zone A, %d chars. This is the game-lane caller CONVENTIONS 8's budget guards were waiting on - MaxSnapshotTokens and SnapshotPreFilterMaxChars can now identify the Zone B+C region, and the Zone-A size assertion re-runs with a MEASURED ZoneA_tokens. Confirm on the LogSiegeLlama 'STATIC PREFIX REGISTERED' line; a success here is a submission, not an application."),
		ZoneA.Len());
}
