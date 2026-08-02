// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SiegeAssistantCommand.generated.h"

/**
 *  THE PURE-DATA HEADER for the in-match LLM command assistant (batch
 *  LLM-ASSISTANT, TASK-417 — CONVENTIONS "In-match LLM command assistant (v1,
 *  text-only) — 2026-08-02" §3, §9). Follows the UnitCommand.h / TeamId.h
 *  precedent: enums, one POD struct, free functions and the game-lane log
 *  category, so every consumer (the snapshot, the grammar, the FSM/executor,
 *  the console widget) can include it without a heavy dependency.
 *
 *  ⚖️ NET RELEVANCY TIER: this file adds no replicated property, no new
 *  replicated class, no new relevancy tier. (M8 DECLARATION DUTY, CONVENTIONS
 *  §8 — "there is nothing to declare" only counts when it is stated.)
 *  FSiegeAssistantCommand is deliberately uint8 / int32 / FName ONLY so M8 P2
 *  can take it over the wire AS-IS (~30 bytes at the cap) with inference
 *  staying client-local. Do not add a pointer, an FVector, an FString or a soft
 *  path to it — that is the entire forward-compatibility story and it costs
 *  nothing now.
 *
 *  ⚠️ THE CONTENTS OF THIS HEADER ARE PINNED (CONVENTIONS §9 registry).
 *  TASK-416's USiegeAssistantSnapshot includes it for LogSiegeAssistant.
 *  Renaming anything here breaks the link and is an automatic QA FAIL.
 */

/**
 *  Game-lane log category: snapshots, grammars, commands, FSM transitions,
 *  refusals. Deliberately SEPARATE from the plugin's LogSiegeLlama (tokens,
 *  backends, timings, faults) — the plugin is architecturally forbidden to know
 *  about Siegebound, so it must not share a Siegebound category. Defined in
 *  SiegeAssistantCommand.cpp.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeAssistant, Log, All);

/**
 *  The seven orders the assistant may translate an utterance into, plus None.
 *
 *  ⚠️ THIS ENUM IS THE ONE SOURCE for the grammar's `intent` alternatives.
 *  USiegeAssistantGrammar::Build derives them by reflection
 *  (SiegeAssistantIntentSymbols) and ParseSiegeAssistantCommand maps them back
 *  through the same helper, so the emitted vocabulary and the accepted
 *  vocabulary cannot drift. Adding a member here widens both at once — which is
 *  exactly the property we want, and exactly why nobody may hand-write the
 *  seven strings a second time anywhere.
 *
 *  None is NEVER emittable: it is the "no command" value the parser resets to,
 *  and it is skipped when the symbol list is built.
 *
 *  THE EXECUTOR SEAM (CONVENTIONS §8, named so no implementer re-invents it) —
 *  Attack/Defend are LATCHED ARMY-WIDE STANCES WITH NO SELECTION, so the
 *  selection-bearing verbs are kept strictly separate from the army-wide ones:
 *    Send, Guard  -> ASiegePlayerController::CreateUnitGroup(Hold, ...)
 *    Ambush       -> ASiegePlayerController::CreateUnitGroup(Ambush, ...)
 *    Follow       -> ASiegePlayerController::EnrollInDefaultFollowGroup(Unit)
 *    Charge       -> SetUnitCommand(Attack)   (whole army, no selection)
 *    Fallback     -> SetUnitCommand(Defend)   (whole army, no selection)
 *    Rally        -> AHeroCharacter::Rally()  (hero ability, no selection)
 *  Wave 1's B2 owns that mapping; it is recorded here because the split is the
 *  reason the vocabulary has seven verbs instead of three.
 */
UENUM()
enum class ESiegeAssistantIntent : uint8
{
	None,
	Send,
	Guard,
	Ambush,
	Follow,
	Charge,
	Fallback,
	Rally
};

/**
 *  ⚠️ THE SELECTION IS MULTI-KIND, AND THIS IS THE HARD CAP ON IT (manager
 *  ruling 15, 2026-08-02 — CONVENTIONS §9/§10). The first pin carried a
 *  SINGULAR Kind, which could not express the feature's own flagship sentence,
 *  "send 10 footmen WITH A SORCERER to the nearest ancient ground".
 *
 *  THE CAP IS ENFORCED IN BOTH PLACES, DELIBERATELY:
 *   - in the GRAMMAR, as a bounded alternation of exactly 1, 2 or 3 pairs, so a
 *     4-kind selection is not merely rejected afterwards but is UNREACHABLE by
 *     the sampler. An unbounded repetition rule is precisely what a small model
 *     rambles into, and a bounded alternation costs nothing.
 *   - in the PARSER, because in M8 P2 this struct arrives over the wire and a
 *     corrupt or hostile peer never passed through anyone's grammar.
 *
 *  Pinned as a `static constexpr` in §9 — this is NOT one of §10's feel-pass
 *  tunables. Widening it changes the wire payload and the grammar shape
 *  together and needs a manager ruling.
 */
static constexpr int32 SiegeAssistantMaxSelectionKinds = 3;

/**
 *  One translated order — the ENTIRE output surface of the model, after
 *  parsing. uint8 / int32 / FName ONLY (see the net-relevancy note above).
 *
 *  Every field is a SYMBOL, never a coordinate and never player-facing text
 *  (CONVENTIONS §3). Resolved FVectors for named places stay game-side behind
 *  USiegeAssistantSnapshot::ResolvePlace; the model sees `ancient_ground_near`
 *  and never sees a number that means a position.
 *
 *  ⚠️ Counts carries the player's REQUESTED quantities, not legal ones. The
 *  grammar's range is 1..30 and is NOT clamped to the live roster — see
 *  USiegeAssistantGrammar::GrammarCountMax for the full reasoning. The executor
 *  is what detects "you asked for 10 and 8 exist"; if this field had already
 *  been clamped, that clarification would be undetectable.
 */
USTRUCT()
struct FSiegeAssistantCommand
{
	GENERATED_BODY()

	/** The verb. None means "no command" — the value the parser resets to on every failure. */
	UPROPERTY()
	ESiegeAssistantIntent Intent = ESiegeAssistantIntent::None;

	/**
	 *  Canonical unit symbols, e.g. "footman". Empty means no kind was named
	 *  (every eligible unit, or an army-wide verb that takes no selection).
	 *
	 *  ⚠️ INDEX-ALIGNED WITH Counts, AND THEY ARE PARALLEL ARRAYS RATHER THAN AN
	 *  ARRAY OF STRUCTS ON PURPOSE — that is what keeps the payload to FName and
	 *  int32 only and preserves the "M8 P2 takes this as-is over the wire"
	 *  property. Do not "tidy" this into a TArray of a pair struct.
	 *
	 *  Kinds.Num() == Counts.Num() and Kinds.Num() <= SiegeAssistantMaxSelectionKinds
	 *  are INVARIANTS, checked by SiegeAssistantValidateSelection. A violation is
	 *  a hard failure, never a silent truncation — silently dropping the tail is
	 *  exactly the valid-shaped-wrong-command class this design exists to stop.
	 *  Kinds also never repeats a symbol: two counts for one kind would leave the
	 *  executor to pick one and silently discard the other.
	 */
	UPROPERTY()
	TArray<FName> Kinds;

	/** Requested quantity per entry of Kinds, index-aligned. 0 == "all". The grammar range is 1..30 (NOT the live max). */
	UPROPERTY()
	TArray<int32> Counts;

	/** Canonical place symbol, e.g. "ancient_ground_near". NAME_None means no place was named. */
	UPROPERTY()
	FName Where = NAME_None;

	/** Deferred-intent trigger kind. NAME_None == "now" (execute immediately). */
	UPROPERTY()
	FName TriggerKind = NAME_None;

	/** Deferred-intent threshold: fire once at least this many TriggerKind exist. 0 when TriggerKind is NAME_None. */
	UPROPERTY()
	int32 TriggerAtLeast = 0;
};

/**
 *  The reserved wire symbols. These are the only three strings that mean
 *  something structural rather than naming a game object, which is why
 *  USiegeAssistantGrammar::Build refuses to emit any of them as a GENERATED
 *  kind/place alternative — a unit literally called "all" would otherwise
 *  collide with the selector sentinel.
 */
namespace SiegeAssistantSymbols
{
	/** "nothing named" — a `who` with no units, a `where` with no place, and the degenerate empty-roster `kind`. */
	inline constexpr const TCHAR* None = TEXT("none");

	/** "every eligible unit" — parses to an empty Kinds/Counts pair. */
	inline constexpr const TCHAR* All = TEXT("all");

	/** "execute immediately" — parses to TriggerKind = NAME_None, TriggerAtLeast = 0. */
	inline constexpr const TCHAR* Now = TEXT("now");
}

/**
 *  The JSON schema's key names — ONE source, shared by the grammar emitter
 *  (which builds the object shape from them) and the parser (which validates
 *  the exact key set against them). A schema change is therefore a one-line
 *  change that moves both halves together.
 */
namespace SiegeAssistantJsonKeys
{
	/** Command object: the verb. */
	inline constexpr const TCHAR* Intent = TEXT("intent");

	/** Command object: the selection — an array of 1..SiegeAssistantMaxSelectionKinds items, or "all", or "none". */
	inline constexpr const TCHAR* Who = TEXT("who");

	/** Command object: the destination place symbol, or "none". */
	inline constexpr const TCHAR* Where = TEXT("where");

	/** Command object: "now", or the deferred trigger object. */
	inline constexpr const TCHAR* When = TEXT("when");

	/** Item object / trigger object: the unit kind symbol. */
	inline constexpr const TCHAR* Kind = TEXT("kind");

	/** Item object: the requested count (1..30, or "all"). Short on purpose — the output token budget is 96. */
	inline constexpr const TCHAR* N = TEXT("n");

	/** Trigger object: the numeric threshold (1..30 — "all" is NOT legal here). */
	inline constexpr const TCHAR* AtLeast = TEXT("at_least");

	/** Question object: the clarification code the model is asking for. */
	inline constexpr const TCHAR* Ask = TEXT("ask");
}

/**
 *  The closed set of clarification codes the model may emit instead of a
 *  command (the grammar's `question` branch).
 *
 *  ⚠️ WHY A QUESTION BRANCH EXISTS AT ALL: without it a model handed an
 *  unparseable utterance is FORCED by the grammar to emit some command, because
 *  declining is not expressible. Constrained decoding cannot make a model
 *  correct — it can only make it well-formed — so the one thing it must always
 *  leave reachable is "I cannot turn this into an order." These are reason
 *  CODES, never player-facing text (CONVENTIONS §3): the FSM picks the
 *  game-authored template.
 */
namespace SiegeAssistantAsk
{
	/** The utterance named no unit, or named an ambiguous one ("the caster"). */
	inline constexpr const TCHAR* WhichUnit = TEXT("which_unit");

	/** The utterance named no place, or an ambiguous one ("the mine" with several listed). */
	inline constexpr const TCHAR* WhichPlace = TEXT("which_place");

	/** The utterance named units but no quantity. */
	inline constexpr const TCHAR* HowMany = TEXT("how_many");

	/** The verb was ambiguous ("defend" sits between guard and fallback). */
	inline constexpr const TCHAR* WhichIntent = TEXT("which_intent");

	/** The utterance is not an order the assistant can express at all. */
	inline constexpr const TCHAR* Unsupported = TEXT("unsupported");
}

/**
 *  ParseSiegeAssistantCommand's reason codes. The FSM compares the CODE — the
 *  part before the first ':' — and some codes carry a `:detail` payload for the
 *  log. Use SiegeAssistantReasonCode to split. No entry here is player-facing
 *  text; every sentence a player reads is a game-authored template filled from
 *  one of these (CONVENTIONS §3).
 */
namespace SiegeAssistantReason
{
	/** The input string was empty or whitespace. */
	inline constexpr const TCHAR* EmptyInput = TEXT("empty_input");

	/** Not parseable as a JSON object. */
	inline constexpr const TCHAR* MalformedJson = TEXT("malformed_json");

	/** A key outside the schema was present. Payload: the key. */
	inline constexpr const TCHAR* UnknownKey = TEXT("unknown_key");

	/** A required key was absent. Payload: the key. */
	inline constexpr const TCHAR* MissingKey = TEXT("missing_key");

	/** A key held the wrong JSON type. Payload: the key. */
	inline constexpr const TCHAR* BadType = TEXT("bad_type");

	/** The intent symbol is not one of the seven. Payload: the value. */
	inline constexpr const TCHAR* UnknownIntent = TEXT("unknown_intent");

	/** `who` was a string that was neither "all" nor "none". Payload: the value. */
	inline constexpr const TCHAR* BadWho = TEXT("bad_who");

	/** `who` was an array whose length was outside 1..SiegeAssistantMaxSelectionKinds. Payload: the length. */
	inline constexpr const TCHAR* WhoArity = TEXT("who_arity");

	/** A selection-bearing intent was paired with `who` = "none". Payload: the intent. */
	inline constexpr const TCHAR* WhoRequired = TEXT("who_required");

	/** The same unit kind appeared twice in one selection. Payload: the kind. */
	inline constexpr const TCHAR* DuplicateKind = TEXT("duplicate_kind");

	/** Kinds.Num() != Counts.Num(). Payload: "<kinds>/<counts>". NEVER truncate to fix this. */
	inline constexpr const TCHAR* SelectionMismatch = TEXT("selection_mismatch");

	/** More than SiegeAssistantMaxSelectionKinds entries. Payload: the length. */
	inline constexpr const TCHAR* SelectionOverflow = TEXT("selection_overflow");

	/** A kind symbol was empty or the reserved "none". Payload: the value. */
	inline constexpr const TCHAR* BadKind = TEXT("bad_kind");

	/** The place symbol was empty. Payload: the value. */
	inline constexpr const TCHAR* BadWhere = TEXT("bad_where");

	/** `when` was a string that was not "now", or an object of the wrong shape. Payload: the value. */
	inline constexpr const TCHAR* BadWhen = TEXT("bad_when");

	/** A count or threshold fell outside 1..GrammarCountMax. Payload: the value. */
	inline constexpr const TCHAR* CountOutOfRange = TEXT("count_out_of_range");

	/** The `ask` code is not one of the five. Payload: the value. */
	inline constexpr const TCHAR* UnknownAsk = TEXT("unknown_ask");

	/**
	 *  Not a failure of the model — the model DECLINED to produce a command and
	 *  asked for clarification instead. Payload: the SiegeAssistantAsk code.
	 *  ParseSiegeAssistantCommand returns false for this (a question is not an
	 *  executable command) with OutCommand fully reset, and the FSM routes it to
	 *  Clarify rather than Failed.
	 *
	 *  It rides the OutError channel because FSiegeAssistantCommand has no field
	 *  for an ask code and the struct is PINNED (CONVENTIONS §9). Giving it one
	 *  would be a registry change and needs a manager ruling.
	 */
	inline constexpr const TCHAR* Ask = TEXT("ask");
}

/**
 *  Splits a reason string into its bare code, dropping any ':detail' payload,
 *  so the FSM can switch on it without hand-rolling the same substring search.
 */
FString SiegeAssistantReasonCode(const FString& Reason);

/** The canonical lower-case wire symbol for an intent ("send"). Empty for None and for anything unknown. */
FString SiegeAssistantIntentToSymbol(ESiegeAssistantIntent Intent);

/** Reverse of SiegeAssistantIntentToSymbol. Case-insensitive. Returns false for "none" and anything unknown. */
bool SiegeAssistantIntentFromSymbol(const FString& Symbol, ESiegeAssistantIntent& OutIntent);

/** Every emittable intent symbol, in enum declaration order, with None and the UHT _MAX entry excluded. */
void SiegeAssistantIntentSymbols(TArray<FString>& OutSymbols);

/** Every legal clarification code, in a fixed order. Shared by the grammar's `ask` rule and the parser. */
void SiegeAssistantAskCodes(TArray<FString>& OutCodes);

/**
 *  True when Intent is one of the SELECTION-BEARING verbs (Send / Guard /
 *  Ambush / Follow) rather than an army-wide one (Charge / Fallback / Rally).
 *  The split is the executor seam described on ESiegeAssistantIntent, and it is
 *  a property of the verb alone, so it lives here rather than in the FSM where
 *  three separate callers would each re-derive it.
 */
bool SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent Intent);

/**
 *  Checks the index-alignment, cap and uniqueness invariants of a selection.
 *
 *  ⚠️ THIS IS NOT CEREMONIAL AND IT IS NOT DEAD CODE. Nothing that comes out of
 *  ParseSiegeAssistantCommand can violate it — the parser builds both arrays in
 *  one walk — but in M8 P2 an FSiegeAssistantCommand arrives OVER THE WIRE, and
 *  a corrupt or hostile peer never passed through anyone's grammar or parser.
 *  This is the receive-side validator, written now so P2 has one and so the law
 *  ("a mismatch is a parse failure, never a silent truncation") is expressed as
 *  an executable check rather than a comment. The parser calls it as its final
 *  gate; the Wave-1 executor should call it on anything it did not parse itself.
 *
 *  @param Kinds     the selection's unit symbols.
 *  @param Counts    the index-aligned quantities.
 *  @param OutError  a SiegeAssistantReason code with a ':detail' payload. Empty when valid.
 *  @return          true when Kinds.Num() == Counts.Num(), the length is within the
 *                   cap, and no symbol repeats.
 */
bool SiegeAssistantValidateSelection(const TArray<FName>& Kinds, const TArray<int32>& Counts, FString& OutError);

/**
 *  Parses ONE constrained-decoding result into a command. STRICT by design —
 *  this is the last line of defence between a well-formed string and the
 *  executor, and the design's whole claim is that a valid-shaped WRONG command
 *  never reaches the game silently.
 *
 *  Rejects: unknown keys, missing keys, wrong JSON types, unknown intents,
 *  non-integer or out-of-range counts, a `who` array outside
 *  1..SiegeAssistantMaxSelectionKinds, a repeated kind, and `at_least: "all"`.
 *  Maps "all" -> Count 0 and "now" -> TriggerKind = NAME_None.
 *
 *  ⚠️ NEVER PARTIALLY FILLS. OutCommand is reset to a default-constructed value
 *  on entry AND again on every failure path, so a caller that ignores the
 *  return value gets an inert None command rather than half an order.
 *
 *  Key matching is CASE-SENSITIVE (JSON keys are); symbol VALUES are compared
 *  case-insensitively, because FName is case-insensitive anyway and the
 *  grammar only ever emits lower case.
 *
 *  ⚠️ IT PERFORMS EXACTLY ONE CROSS-FIELD CHECK, and only because that one pair
 *  is REPRESENTATIONALLY ambiguous: `who` = "none" with a selection-bearing
 *  intent is rejected, because "none" and "all" both land on an empty
 *  Kinds/Counts pair and the struct has no field to tell them apart — so
 *  accepting it would silently turn "send nobody" into "send everybody". Every
 *  OTHER cross-field question — is that place resolvable, are there enough
 *  units, is this intent legal right now, does this player have authority — is
 *  the EXECUTOR's, and this function deliberately does not answer any of them.
 *  Grammar guarantees existence, executor guarantees legality, FSM owns the
 *  conversation.
 *
 *  @param Json        the model's raw output (already constrained by the GBNF).
 *  @param OutCommand  the parsed command; default-constructed on any failure.
 *  @param OutError    a SiegeAssistantReason code, optionally ':detail'-suffixed. Empty on success.
 *  @return            true only when an EXECUTABLE command was produced. A well-formed
 *                     `question` returns FALSE with OutError = "ask:<code>".
 */
bool ParseSiegeAssistantCommand(const FString& Json, FSiegeAssistantCommand& OutCommand, FString& OutError);
