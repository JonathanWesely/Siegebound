// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SiegeAssistantGrammar.generated.h"

/**
 *  THE GBNF GRAMMAR BUILDER (batch LLM-ASSISTANT, TASK-417 — CONVENTIONS
 *  "In-match LLM command assistant (v1, text-only) — 2026-08-02" §1, §9).
 *
 *  This is the crux of the whole feature. A local ~4B model has to emit a
 *  MACHINE-EXECUTABLE command, not prose. Constrained decoding against a GBNF
 *  grammar makes malformed output MECHANICALLY IMPOSSIBLE — the sampler masks
 *  every illegal token at every step — which is what makes a small model viable
 *  at all: single-shot structured output holds at ~82 % for 4B, where
 *  multi-turn agentic tool-calling collapses to 35 %.
 *
 *  ⚠️ THE GRAMMAR IS GENERATED PER REQUEST FROM LIVE GAME STATE, and that is
 *  where the grounding lives — NOT in the weights. If no Sorcerer is alive,
 *  "sorcerer" is simply not an alternative of the `kind` rule and the model
 *  physically cannot name one. Only `kind`, `where` and `zone` are generated;
 *  every other rule is fixed.
 *
 *  > THE LAW: grammar guarantees existence, executor guarantees legality, FSM
 *  > owns the conversation.
 *
 *  Three owners, three responsibilities, no overlap. Constrained decoding
 *  guarantees SYNTAX, never SEMANTICS — it cannot prevent a valid-shaped wrong
 *  command — which is why confirm-before-execute with ghost circles on the
 *  ground is a Wave-1 requirement rather than polish.
 *
 *  ⚖️ NET RELEVANCY TIER: adds no replicated property, no new replicated class,
 *  no new relevancy tier. (M8 DECLARATION DUTY, CONVENTIONS §8.)
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeAssistantGrammar : public UObject
{
	GENERATED_BODY()

public:

	/**
	 *  Smallest quantity the grammar will emit. 0 is deliberately NOT legal —
	 *  "send 0 footmen" is never a real order, and 0 already means "all" in
	 *  FSiegeAssistantCommand::Counts.
	 */
	static constexpr int32 GrammarCountMin = 1;

	/**
	 *  ⚠️⚠️ THE ONE DESIGN SUBTLETY IN THIS FILE. `count` runs 1..30 — NOT
	 *  1..live-max. THIS LOOKS LIKE A MISSED CONSTRAINT AND IT IS NOT. Do not
	 *  "tighten" it to the live roster; that is introducing the defect, not
	 *  fixing one (CONVENTIONS §1, and an explicit QA criterion at TASK-419).
	 *
	 *  If the grammar capped counts at the live maximum, a player asking for 10
	 *  when 8 exist would get a SILENTLY-EMITTED 8 — the exact
	 *  valid-shaped-wrong-command failure this entire design exists to prevent —
	 *  and the clarification ("there are only 8, is that OK?") would become
	 *  UNDETECTABLE, because nothing downstream could tell a request for 8 from
	 *  a clamped request for 10.
	 *
	 *  IDENTITY IS CLOSED-WORLD, QUANTITY IS OPEN. The grammar constrains WHO
	 *  can be named, absolutely. It leaves HOW MANY loose on purpose, and the
	 *  EXECUTOR is what detects the shortfall and hands it to the FSM.
	 *
	 *  30 is the §10 tunable `GrammarCountMax`.
	 */
	static constexpr int32 GrammarCountMax = 30;

	/**
	 *  Builds the complete GBNF grammar for one request.
	 *
	 *  ⚠️ PURE AND DETERMINISTIC BY DESIGN — no UWorld, no engine state, no
	 *  UObject inputs, no clock, no randomness. This shape is deliberate and is
	 *  pinned in CONVENTIONS §9: it decouples this task from the snapshot
	 *  entirely, and it makes this the ONE place in the whole feature where real
	 *  automation tests are cheap and genuinely valuable (no model in the loop).
	 *  Do not add a snapshot parameter later "for convenience".
	 *
	 *  The same inputs always produce a BYTE-IDENTICAL string. To make that true
	 *  rather than merely likely, symbols are lower-cased before emission (FName
	 *  preserves the case it was first constructed with, which would otherwise
	 *  let identical game state produce two different grammars), duplicates are
	 *  dropped, and the CALLER'S ORDERING IS PRESERVED VERBATIM — no sort, so
	 *  nothing depends on FName's comparison-index ordering, which is not stable
	 *  across processes.
	 *
	 *  DEGENERATE INPUTS ARE HANDLED, NOT ASSERTED ON. With no unit kinds the
	 *  `kind` / `count` / `at-least` / `item` / `selection` / `exceptlist` /
	 *  `except` rules are OMITTED ENTIRELY (they would be unreachable), `who`
	 *  collapses to "all" | "none" — or to `inplace` | "all" | "none" if the board
	 *  still has region-bearing places, since a region names no kind — and `when`
	 *  collapses to "now" — so the grammar
	 *  stays well-formed with every referenced rule defined, and with nothing
	 *  nameable the only expressible outputs are army-wide orders and questions.
	 *  That is the correct answer, not a workaround. With no place names, `where`
	 *  collapses to just "none". With no REGION-BEARING places the `zone` and
	 *  `inplace` rules are omitted entirely and `who` loses its `inplace`
	 *  alternative — the grammar is then BYTE-IDENTICAL to the one this builder
	 *  produced before the shape existed (AS-§21.4).
	 *
	 *  NOTE FOR THE CALLER (Wave 1's B2): the grammar closes the world over
	 *  EXACTLY the arrays it is handed. Passing the live roster is what delivers
	 *  the grounding guarantee; a caller that widened UnitKinds to the whole deck
	 *  would trade that guarantee away, so do not — the shortfall and
	 *  not-yet-spawned cases are the FSM's clarification and deferred-intent
	 *  paths, not the grammar's.
	 *
	 *  The emitted grammar carries NO comments and NO whitespace rule. Comments
	 *  are omitted because GBNF comment support is a property of whichever
	 *  llama.cpp build TASK-409 vendors and this string must parse there; the
	 *  annotated copy lives in handoffs/TASK-417-programmer.md instead. There is
	 *  no `ws` rule because permitting optional whitespace would spend tokens
	 *  from a 96-token output budget on nothing and add sampling branches at
	 *  every boundary — the model emits exactly one compact JSON object.
	 *
	 *  ⛔ EVERY RULE NAME IS KEBAB-CASE, AND THAT IS LOAD-BEARING, NOT STYLE.
	 *  llama.cpp reads a rule name as [a-zA-Z0-9-] and stops at anything else, so
	 *  ONE underscore in ONE rule name makes the WHOLE grammar unparseable and
	 *  generation silently runs unconstrained — which is what shipped as
	 *  `at_least` and cost TASK-413 two of its six bars. The JSON KEYS the rules
	 *  carry stay snake_case (`at_least`) because they are the wire format. The
	 *  builder now validates both halves at construction and names the offender;
	 *  Siegebound.Assistant.Grammar.RuleNameCharset asserts it in automation.
	 *
	 *  ⚠️ AND THE REASON THAT GUARD EXISTS RATHER THAN A CONVENTION: two reviews
	 *  diffed this generator against the spike's mirror rule-for-rule and found
	 *  them identical, which they were — identically unparseable. String-comparing
	 *  two generators can never prove either one is valid; only the target parser
	 *  can.
	 *
	 *  ⛔⭐ THE THIRD PARAMETER IS TRAILING AND DEFAULTED, AND THE COST OF THAT IS
	 *  NAMED RATHER THAN HIDDEN. RegionPlaceNames is the REGION-BEARING SUBSET of
	 *  PlaceNames — the places a shipped IsPointInZone can answer for (AS-§21.4:
	 *  exactly `mid`, `ancient_ground_near`, `ancient_ground_far` today, and the
	 *  SNAPSHOT decides which, never this builder). A CALLER THAT OMITS IT GETS A
	 *  GRAMMAR WITH NO `in` ALTERNATIVE AT ALL — the player then cannot say
	 *  "everyone in the mid", and the sampler cannot reach the shape.
	 *
	 *  ⚠️ THAT IS DECLARED BEHAVIOUR, NOT A SILENT FAILURE, and it is the same
	 *  trade SiegeAssistantValidateSelection's trailing defaults already make
	 *  (SiegeAssistantCommand.h — "a caller that omits the argument silently
	 *  validates NOTHING"). The shape was chosen so every existing call site stays
	 *  BYTE-IDENTICAL and the feature lands without editing files it does not own;
	 *  the price is that a NEW caller which forgets the argument loses a feature
	 *  quietly rather than failing to compile. ⇒ ANY CALLER HOLDING A SNAPSHOT
	 *  PASSES Snapshot->GetRegionPlaceNames(). The pin is AS-§21.9.
	 *
	 *  @param UnitKinds         canonical unit symbols the player may legally name, e.g. {"footman", "sorcerer"}.
	 *                           Empty, NAME_None, and the reserved symbols "all"/"none" are skipped.
	 *  @param PlaceNames        canonical place symbols, e.g. {"enemy_castle", "ancient_ground_near"}. Same filtering.
	 *  @param RegionPlaceNames  the region-bearing SUBSET, in the caller's order. Same filtering. Empty (the
	 *                           default) omits `zone` / `inplace` and the `who` alternative entirely.
	 *  @return                  a complete GBNF grammar whose entry rule is `root`, newline-terminated.
	 */
	static FString Build(const TArray<FName>& UnitKinds, const TArray<FName>& PlaceNames,
		const TArray<FName>& RegionPlaceNames = TArray<FName>());
};
