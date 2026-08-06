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

/** Whole KINDS subtractable from an "all" selection. Mirrors SiegeAssistantMaxSelectionKinds
 *  deliberately: bounded in the GRAMMAR as an alternation, and re-checked in the PARSER
 *  because M8 P2 takes this struct over the wire from a peer that passed no grammar.
 *  ⛔ Widening it is a MANAGER RULING (the ruling-15 precedent), not a tuner's edit.
 *
 *  ⚠️ THERE IS NO COUNT-CONTROLLED VARIANT AND THERE MUST NEVER BE ONE (Jonathan's
 *  ruling 3, 2026-08-04 — CONVENTIONS AS-§20.1). "all except 5 archers" was DECLINED,
 *  and the decline is enforced STRUCTURALLY: the exclusion list is an array of BARE
 *  kind symbols, never {"kind":…,"n":…} items, so the shape has no place in the
 *  grammar for a count to be sampled into. A declined feature made INEXPRESSIBLE
 *  cannot be re-introduced by a prompt tweak or a tired author. Do not add a parallel
 *  ExcludeCounts array.
 */
static constexpr int32 SiegeAssistantMaxExclusionKinds = 3;

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
 *
 *  SEVEN FIELDS, and the last two are the only ones that are not positive
 *  statements about which units to take:
 *   - ExcludeKinds SUBTRACTS whole kinds from the "all" selection ("send everyone
 *     except the miners" — Jonathan's ruling 3, CONVENTIONS AS-§20.1). Added
 *     2026-08-04 by TASK-518, AFTER the five above, and the five keep their
 *     shipped order and their shipped types so the wire property is untouched.
 *   - RegionPlace NARROWS the "all" selection to the units STANDING IN a named
 *     region ("send all units currently in an ancient ground" — Jonathan's
 *     directive, CONVENTIONS AS-§21.5). Added 2026-08-05 by TASK-545, AFTER the
 *     six above, with the identical discipline: appended, never inserted, and an
 *     FName rather than a box so the wire property survives a second time.
 *
 *  ⚠️ BOTH OF THEM ARE FILTERS, WHICH IS WHY THEY OBEY THE SAME LAW INSTEAD OF
 *  EACH GETTING ITS OWN: a filter handed to a verb that never runs the selector is
 *  parsed and then silently DROPPED, and a dropped filter is an order that looks
 *  obeyed while meaning something nobody asked for. See the cross-field checks
 *  documented on ParseSiegeAssistantCommand.
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

	/**
	 *  Kinds SUBTRACTED from an "all" selection. Non-empty ONLY when Kinds is empty,
	 *  and ONLY when SiegeAssistantIntentTakesSelection(Intent). No counts, by design.
	 *
	 *  ⚠️ THE SIXTH FIELD, APPENDED AFTER THE FIVE SHIPPED ONES — never inserted
	 *  among them. Kinds/Counts stay PARALLEL ARRAYS and stay index-aligned; this is
	 *  a THIRD, INDEPENDENT array and it is NOT index-aligned with either of them.
	 *
	 *  ⛔ THE TWO INVARIANTS, AND THEY ARE THE WHOLE SEMANTICS:
	 *   - EXCLUSION IS ONLY MEANINGFUL AGAINST "all". Kinds non-empty AND ExcludeKinds
	 *     non-empty is a parse FAILURE (SiegeAssistantReason::ExcludeConflict), NEVER a
	 *     merge — "send 10 footmen except miners" is a confused sentence and the FSM
	 *     must ask rather than guess which half of it to honour.
	 *   - EXCLUSION IS ONLY MEANINGFUL ON A SELECTION-BEARING VERB. Charge / Fallback
	 *     execute through ASiegePlayerController::ApplyArmyWideStance and Rally through
	 *     AHeroCharacter::Rally() — NONE of them passes through the selector, so an
	 *     exception handed to them would be parsed and then silently DROPPED. "Fall back
	 *     except the miners" executing as "fall back INCLUDING the miners" is the exact
	 *     valid-shaped-wrong-command this whole design exists to prevent, so the parser
	 *     REFUSES it (ExcludeConflict) instead of accepting an order it cannot keep.
	 *
	 *  ⚠️ EXCLUDING EVERY LIVE KIND IS A LEGAL PARSE, NOT A PARSE ERROR, AND THAT IS
	 *  A RULING RATHER THAN AN OVERSIGHT (AS-§20.1 "EMPTY-AFTER-EXCLUSION").
	 *  ParseSiegeAssistantCommand is PURE — it has no world, no roster and no snapshot,
	 *  and is never handed the live kind list — so "did that subtract everything?" is a
	 *  question it structurally CANNOT answer. It is the executor's, and the executor
	 *  refuses the whole order through the EXISTING unsupported-ask outcome with the
	 *  arithmetic in the log. ⛔ Never a silent no-op: "nothing happened and nothing was
	 *  said" is the shape that reads as "the assistant ate my order".
	 */
	UPROPERTY()
	TArray<FName> ExcludeKinds;

	/**
	 *  The region whose OCCUPANTS ARE the selection — the units standing inside the
	 *  named place ("send everyone in the mid"). NAME_None == no region was named.
	 *
	 *  ⚠️ THE SEVENTH FIELD, APPENDED AFTER THE SIX SHIPPED ONES — never inserted
	 *  among them. It is an FName and deliberately NOT an FVector, an FBox or a
	 *  soft actor path: that is what keeps this struct uint8 / int32 / FName ONLY,
	 *  so the "M8 P2 takes it over the wire AS-IS" property stated at the top of
	 *  this header SURVIVES the addition. That property is the whole reason this
	 *  design is possible — the geometry stays game-side behind
	 *  USiegeAssistantSnapshot::ResolvePlaceRegion exactly as the destination
	 *  geometry already stays behind ResolvePlace, so the model names
	 *  `ancient_ground_near` and still never sees a number that means a position.
	 *
	 *  ⛔ THE TWO INVARIANTS, AND THEY ARE THE WHOLE SEMANTICS:
	 *   - A REGION IS ONLY MEANINGFUL AGAINST "all". Non-None ONLY when Kinds AND
	 *     ExcludeKinds are BOTH empty. "Send 10 footmen in the mid" is a filtered
	 *     COUNT — a different feature that was not asked for — and "everyone except
	 *     the miners, in the mid" stacks two filters whose interaction nobody ruled
	 *     on. Both are a parse FAILURE (SiegeAssistantReason::RegionConflict), NEVER
	 *     a merge and never a silent choice of which half to honour.
	 *   - A REGION IS ONLY MEANINGFUL ON A SELECTION-BEARING VERB. Non-None ONLY
	 *     when SiegeAssistantIntentTakesSelection(Intent). Charge / Fallback execute
	 *     through ASiegePlayerController::ApplyArmyWideStance and Rally through
	 *     AHeroCharacter::Rally() — NONE of them passes through the selector, so a
	 *     region handed to them would be parsed and then silently DROPPED. "Fall
	 *     back, but only the ones in the mid" executing as "fall back, EVERYONE" is
	 *     the same valid-shaped wrong command as its exclusion twin one field up, so
	 *     the parser REFUSES it (RegionConflict) rather than accept an order it
	 *     cannot keep.
	 *
	 *  ⚠️ A REGION THAT CONTAINS NOBODY IS A LEGAL PARSE, NOT A PARSE ERROR, AND
	 *  THAT IS A RULING RATHER THAN AN OVERSIGHT (AS-§21.5, applying AS-§20.1's
	 *  "EMPTY-AFTER-EXCLUSION" clause verbatim). ParseSiegeAssistantCommand is PURE —
	 *  no world, no roster, no snapshot, and it is never handed a unit position — so
	 *  "was anybody standing there?" is a question it structurally CANNOT answer. It
	 *  is the executor's, and the executor refuses the whole order through the
	 *  EXISTING unsupported-ask outcome with the arithmetic in the log. ⛔ Never a
	 *  silent no-op, and ⛔ never a fall-through to an UNFILTERED order: "send
	 *  everyone in the mid" degrading into "send everyone" is an army moving that
	 *  the player never asked to move. Fail closed.
	 */
	UPROPERTY()
	FName RegionPlace = NAME_None;
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

	/**
	 *  Command object: the selection — an array of 1..SiegeAssistantMaxSelectionKinds
	 *  items, or "all", or "none", or the exclusion object {"all_except":[…]}, or the
	 *  region object {"in":…}. FIVE shapes for ONE key; the top-level key set has
	 *  never changed and must not (see In and AllExcept below for why).
	 */
	inline constexpr const TCHAR* Who = TEXT("who");

	/**
	 *  Exclusion object (a `who` VALUE, never a top-level key): the array of whole
	 *  kinds subtracted from "all". {"who":{"all_except":["miner"]}}
	 *
	 *  ⛔ IT IS A THIRD SHAPE FOR AN EXISTING KEY AND NOT A FOURTH TOP-LEVEL KEY, AND
	 *  THAT WAS RULED RATHER THAN PREFERRED (AS-§20.1). ValidateExactKeySet demands an
	 *  EXACT top-level key set and the prompt law emits every key ALWAYS, so a new
	 *  top-level key would have had to appear in every emission — rewriting all seven
	 *  few-shots, every corpus row's expected JSON and the parser's key set at once.
	 *  As a `who` shape it is STRICTLY ADDITIVE AT THE WIRE: every JSON that parses
	 *  today still parses, byte-for-byte, and nothing that passes starts failing.
	 *
	 *  ⛔ snake_case because it is WIRE FORMAT. The GBNF RULE that carries it is
	 *  `exceptlist`, with no underscore — the `at_least` / `at-least` split, one rule
	 *  over. See SiegeAssistantGrammar.cpp.
	 */
	inline constexpr const TCHAR* AllExcept = TEXT("all_except");

	/**
	 *  Region object (a `who` VALUE, never a top-level key): the place whose
	 *  OCCUPANTS are the selection. {"who":{"in":"ancient_ground_near"}}
	 *
	 *  ⛔ IT IS THE FIFTH SHAPE FOR AN EXISTING KEY AND NOT A FOURTH TOP-LEVEL KEY,
	 *  REFUSED ON THE IDENTICAL ARGUMENT THAT REFUSED "except" ONE SHAPE EARLIER
	 *  (AS-§21.5). ValidateExactKeySet demands an EXACT top-level key set and the
	 *  prompt law emits every key ALWAYS, so a new top-level key would have had to
	 *  appear in every emission — rewriting all seven few-shots, every corpus row's
	 *  expected JSON and the parser's key set at once. As a `who` shape it is
	 *  STRICTLY ADDITIVE AT THE WIRE: every JSON that parses today parses
	 *  BYTE-IDENTICALLY after it, and nothing that passes starts failing.
	 *
	 *  ⛔ NO UNDERSCORE, AND THAT IS LOAD-BEARING RATHER THAN TASTE. A GBNF rule name
	 *  is [a-zA-Z0-9-] and llama.cpp STOPS AT AN UNDERSCORE, which is why the key one
	 *  entry up needed the `exceptlist` rule-name departure. This key is one word and
	 *  so is the rule that carries it (`inplace`), so no split is needed anywhere.
	 *  See SiegeAssistantGrammar.cpp.
	 */
	inline constexpr const TCHAR* In = TEXT("in");

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

	/**
	 *  The exclusion list was empty or longer than SiegeAssistantMaxExclusionKinds.
	 *  Payload: the length.
	 *
	 *  ⚠️ THE TWO HALVES ARE CHECKED IN DIFFERENT PLACES AND THAT IS DELIBERATE. The
	 *  parser checks BOTH bounds, because it is reading a `{"all_except":[…]}` object
	 *  whose whole point is to carry at least one kind. SiegeAssistantValidateSelection
	 *  checks only the UPPER bound, because an EMPTY ExcludeKinds is the normal, correct
	 *  state of every command that excludes nothing.
	 */
	inline constexpr const TCHAR* ExcludeArity = TEXT("exclude_arity");

	/**
	 *  An exclusion was paired with something it cannot combine with. Payload names the
	 *  offender. THREE cases, and the third is the load-bearing one:
	 *   - with a positive selection — payload "<kinds>/<excludes>". ⛔ Never a merge.
	 *   - with `who` = "none" — payload "none".
	 *   - on an ARMY-WIDE intent (charge / fallback / rally) — payload the intent symbol,
	 *     e.g. "exclude_conflict:fallback". Those three execute through
	 *     ApplyArmyWideStance / Rally() and never reach the selector, so honouring an
	 *     exception there would mean editing a shipped controller API two keys depend on,
	 *     or building an assistant-side parallel army-wide path — which AS-§2 forbids by
	 *     name. Refusing at the parser is what keeps "fall back except the miners" from
	 *     executing as "fall back INCLUDING the miners".
	 */
	inline constexpr const TCHAR* ExcludeConflict = TEXT("exclude_conflict");

	/**
	 *  A REGION was paired with something it cannot combine with. Payload names the
	 *  offender. FOUR cases — the three ExcludeConflict already names, for word-for-word
	 *  the same reasons (a region and an exclusion are BOTH FILTERS), plus one that
	 *  exists only because there are now two filters that could be stacked:
	 *   - with a positive selection — payload "<kinds>/<region>". ⛔ Never a merge:
	 *     "send 10 footmen in the mid" is a filtered COUNT, a different feature nobody
	 *     asked for, and guessing which half to honour is the class this design stops.
	 *   - with an EXCLUSION — payload "all_except/<region>". Two filters stacked, and
	 *     nobody ruled on how they compose, so the parser refuses instead of inventing
	 *     an order of operations.
	 *   - with `who` = "none" — payload "none".
	 *   - on an ARMY-WIDE intent (charge / fallback / rally) — payload the intent
	 *     symbol, e.g. "region_conflict:fallback". Those three execute through
	 *     ApplyArmyWideStance / Rally() and never reach the selector, so a region
	 *     handed to them would be parsed and then DROPPED IN SILENCE. Refusing at the
	 *     parser is what keeps "fall back, but only the ones in the mid" from executing
	 *     as "fall back, EVERYONE". ⚠️ "Everyone in the mid, fall back" is therefore
	 *     REFUSED BY DESIGN (AS-§21.6, AS-§21.11 item 1) — it is not a bug.
	 */
	inline constexpr const TCHAR* RegionConflict = TEXT("region_conflict");

	/**
	 *  The region symbol inside {"in":…} was empty, or was one of the three reserved
	 *  wire symbols. Payload: the value.
	 *
	 *  ⚠️ "none" IS THE LOAD-BEARING ONE AND IT IS REJECTED RATHER THAN IGNORED.
	 *  FName is case-insensitive and FName(TEXT("none")) IS NAME_None, so storing it
	 *  would collapse into "no region was named" — i.e. {"who":{"in":"none"}} would
	 *  quietly become "everyone", turning a filter the player typed into an unfiltered
	 *  army order. That is the exact silent-drop class the whole feature refuses, so
	 *  the symbol is caught in the parser BEFORE the FName is constructed. "all" and
	 *  "" are refused with it: the grammar never emits a reserved symbol as a place
	 *  alternative, so neither can name a real region.
	 */
	inline constexpr const TCHAR* BadRegion = TEXT("bad_region");

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
 *  ⛔⚠️ ExcludeKinds IS A TRAILING DEFAULTED PARAMETER, AND THE REASON IS RECORDED
 *  HERE RATHER THAN LEFT TO BE REDISCOVERED AS AN ODD SIGNATURE. This function has
 *  callers in TWO files TASK-518 does not own — USiegeAssistantComponent's
 *  receive-side gate and the automation suite — and a REQUIRED fourth parameter
 *  would have broken the module's compile in files the task is forbidden to edit.
 *  A trailing default is the only shape that adds the invariant without reaching
 *  into another task's file. It is why the parameter sits AFTER OutError instead of
 *  beside the two arrays it belongs with.
 *
 *  ⚠️ THE COST OF THAT SHAPE, STATED SO IT IS NOT DISCOVERED LATER: a caller that
 *  omits the argument silently validates NOTHING about exclusion. ⇒ ANY CALLER
 *  HOLDING A WHOLE FSiegeAssistantCommand MUST PASS Command.ExcludeKinds. The
 *  default exists for the arrays-only call sites, not as permission to skip the check.
 *
 *  ⛔⚠️ RegionPlace IS A FIFTH TRAILING DEFAULTED PARAMETER, ADDED 2026-08-05 BY
 *  TASK-545 ON THE IDENTICAL PRECEDENT, AND ITS COST IS IDENTICAL AND WORSE FOR
 *  BEING THE SECOND ONE. The function NAME does not change (AS-§20.1 ruled it keeps
 *  its name; renaming it is a 26-site diff across five files for zero behaviour, and
 *  it would destroy the "the diff is additive" property this batch's whole safety
 *  argument rests on). ⇒ A CALLER THAT OMITS THIS ARGUMENT SILENTLY VALIDATES
 *  NOTHING ABOUT THE REGION, AND NO COMPILER DIAGNOSTIC STANDS BEHIND IT — the
 *  omission compiles, links, runs and reports SUCCESS. Only this comment and the
 *  cross-field gate in ParseSiegeAssistantCommand stop it. ⇒ ANY CALLER HOLDING A
 *  WHOLE FSiegeAssistantCommand MUST PASS BOTH Command.ExcludeKinds AND
 *  Command.RegionPlace — a QA criterion (AS-§21.9), not a style note.
 *
 *  ⛔ A SIXTH TRAILING DEFAULT IS FORBIDDEN. Two is already one more than the shape
 *  can carry honestly. 📌 The right long-term shape is an OVERLOAD taking a
 *  `const FSiegeAssistantCommand&`, which cannot be under-called at all; it is
 *  RECORDED here as a follow-up and deliberately NOT taken in this batch (churn at a
 *  compile gate). The NEXT field-shaped addition takes the overload instead.
 *
 *  @param Kinds         the selection's unit symbols.
 *  @param Counts        the index-aligned quantities.
 *  @param OutError      a SiegeAssistantReason code with a ':detail' payload. Empty when valid.
 *  @param ExcludeKinds  the subtracted kinds. Must be empty whenever Kinds is non-empty,
 *                       within SiegeAssistantMaxExclusionKinds, and free of repeats.
 *  @param RegionPlace   the region whose occupants are the selection, or NAME_None.
 *                       Must be NAME_None whenever Kinds OR ExcludeKinds is non-empty,
 *                       and may never be the reserved symbol "all".
 *  @return              true when Kinds.Num() == Counts.Num(), both lengths are within
 *                       their caps, no symbol repeats in either list, and the selection,
 *                       the exclusion and the region are not populated together.
 */
bool SiegeAssistantValidateSelection(const TArray<FName>& Kinds, const TArray<int32>& Counts, FString& OutError,
                                     const TArray<FName>& ExcludeKinds = TArray<FName>(), FName RegionPlace = NAME_None);

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
 *  `who` HAS FIVE SHAPES: the selection array, "all", "none", the exclusion object
 *  {"all_except":["miner"]}, and the region object {"in":"ancient_ground_near"}.
 *  The fourth and the fifth are each ADDITIVE — neither introduced a new top-level
 *  key and neither changed an existing shape — so every JSON that parsed before
 *  TASK-518 parses identically after it, and every JSON that parses before
 *  TASK-545 parses BYTE-IDENTICALLY after it. ⭐ That property outranks every other
 *  acceptance criterion this file has, and it is the FIRST thing the suite checks.
 *
 *  ⚠️ NEVER PARTIALLY FILLS. OutCommand is reset to a default-constructed value
 *  on entry AND again on every failure path, so a caller that ignores the
 *  return value gets an inert None command rather than half an order.
 *
 *  Key matching is CASE-SENSITIVE (JSON keys are); symbol VALUES are compared
 *  case-insensitively, because FName is case-insensitive anyway and the
 *  grammar only ever emits lower case.
 *
 *  ⚠️ IT PERFORMS EXACTLY THREE CROSS-FIELD CHECKS — ONE until TASK-518, TWO until
 *  TASK-545 — and each earns its place for the SAME narrow reason: the pairing is
 *  one the STRUCT ITSELF cannot represent honestly, so accepting it would produce a
 *  well-formed order that means something nobody asked for.
 *
 *   1. `who` = "none" with a selection-bearing intent is rejected, because "none"
 *      and "all" both land on an empty Kinds/Counts pair and the struct has no
 *      field to tell them apart — so accepting it would silently turn "send
 *      nobody" into "send everybody".
 *   2. An EXCLUSION on an army-wide intent is rejected, because charge / fallback /
 *      rally never reach the selector — so accepting it would silently turn "fall
 *      back except the miners" into "fall back INCLUDING the miners". Same failure
 *      class, opposite sign: check 1 stops a selection being invented, check 2
 *      stops an exception being discarded.
 *   3. A REGION on an army-wide intent is rejected, and it earns its place on
 *      CHECK 2's IDENTICAL ARGUMENT rather than on a new one — which is precisely
 *      why it is a check and not a special case. Charge and Fallback execute
 *      through ASiegePlayerController::ApplyArmyWideStance and Rally through
 *      AHeroCharacter::Rally(); NONE of the three passes through the selector, so a
 *      region handed to them would be PARSED AND THEN SILENTLY DROPPED, and "fall
 *      back, but only the ones in the mid" would execute as "fall back, EVERYONE" —
 *      a valid-shaped wrong command that LOOKS obeyed, which is worse than a
 *      refusal because nothing in the game or the log would contradict it. An
 *      exclusion and a region are BOTH FILTERS and both fail the same way for the
 *      same reason, so check 3 is the SECOND INSTANCE OF ONE PRINCIPLE rather than
 *      a second principle. ⛔ Silently ignoring RegionPlace on any path is an
 *      automatic QA FAIL (AS-§21.6). ⚠️ Its designed consequence — "everyone in the
 *      mid, fall back" is REFUSED — is on Jonathan's playtest sheet and is NOT a bug.
 *
 *  ⛔ ALL THREE ARE GATED BY THE ONE SHIPPED PREDICATE, SiegeAssistantIntentTakesSelection.
 *  There is deliberately NO second list of army-wide verbs anywhere in this file: a
 *  second list is a thing that can drift out of step with the executor seam it claims
 *  to describe, and this one would have had to be edited by a task that never opens
 *  the executor.
 *
 *  ⛔ AND THAT IS THE WHOLE LIST. Every OTHER cross-field question — is that place
 *  resolvable, are there enough units, DID THE EXCLUSION SUBTRACT EVERYTHING, DID
 *  THAT REGION CONTAIN ANYBODY, is this intent legal right now, does this player have
 *  authority — is the EXECUTOR's, and this function deliberately does not answer any
 *  of them. Grammar guarantees existence, executor guarantees legality, FSM owns the
 *  conversation.
 *
 *  @param Json        the model's raw output (already constrained by the GBNF).
 *  @param OutCommand  the parsed command; default-constructed on any failure.
 *  @param OutError    a SiegeAssistantReason code, optionally ':detail'-suffixed. Empty on success.
 *  @return            true only when an EXECUTABLE command was produced. A well-formed
 *                     `question` returns FALSE with OutError = "ask:<code>".
 */
bool ParseSiegeAssistantCommand(const FString& Json, FSiegeAssistantCommand& OutCommand, FString& OutError);
