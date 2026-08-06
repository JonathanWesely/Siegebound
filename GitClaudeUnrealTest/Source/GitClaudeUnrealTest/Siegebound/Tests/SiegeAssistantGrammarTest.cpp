// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Algo/Reverse.h"

#include "Siegebound/SiegeAssistantCommand.h"
#include "Siegebound/SiegeAssistantGrammar.h"
#include "Siegebound/SiegeAssistantVocabulary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for the assistant's grammar builder, command parser and
 *  vocabulary (batch LLM-ASSISTANT, TASK-417).
 *
 *  ⚠️ WHY REAL UNIT TESTS EXIST HERE AND NOWHERE ELSE IN THIS FEATURE.
 *  USiegeAssistantGrammar::Build was deliberately pinned as a PURE function of
 *  two plain arrays — no UWorld, no snapshot object, no engine state, no clock —
 *  precisely so that this one file can assert real behaviour with no editor
 *  world, no PIE session and no model in the loop. The parser and the vocabulary
 *  builder are pure for the same reason. Everything downstream of them (the
 *  subsystem, the FSM, the executor) needs a live world and is verified by
 *  Jonathan's playtest gate instead, so these are the checks that have to carry
 *  the weight.
 *
 *  The laws under test, in the order they matter:
 *   0. ⛔ EVERY RULE NAME AND REFERENCE IS [a-zA-Z0-9-]. This is law ZERO
 *      because a single violation makes the WHOLE grammar unparseable, at which
 *      point laws 1-4 are all still true of a string llama.cpp will not load
 *      and the feature silently runs unconstrained. That is not hypothetical:
 *      `at_least` shipped, and TASK-413's spike found
 *      llama_sampler_init_grammar returning NULL on every generation of all six
 *      bench runs, with the model emitting `<think>` prose instead of JSON.
 *   1. The grammar is DETERMINISTIC — same state in, byte-identical grammar out.
 *   2. GROUNDING: a unit that is not alive is not an alternative, so the model
 *      physically cannot name it.
 *   3. `count` is 1..30 and NOT the live maximum. A test that "fixes" this to
 *      the roster size has broken the feature, not the test.
 *   4. The multi-kind selection is CAPPED IN THE GRAMMAR at three kinds
 *      (manager ruling 15) — not merely rejected afterwards.
 *   5. The parser is strict, never partially fills, and never truncates.
 *
 *  ⚠️ AND THE LIMIT OF ALL OF IT, STATED SO NOBODY RELIES ON MORE THAN IS HERE:
 *  none of these tests parse the grammar with llama.cpp. They assert properties
 *  a HUMAN or a MIRROR can check. Two careful reviews compared this generator
 *  against the spike's mirror rule-for-rule and found them identical, which they
 *  were — identically unparseable. STRING-COMPARING TWO GENERATORS CAN NEVER
 *  PROVE EITHER IS VALID; ONLY THE TARGET PARSER CAN. Law 0 is the cheap
 *  mechanical stand-in for the one failure mode that review demonstrably misses;
 *  the real proof stays a live `llama_sampler_init_grammar` call.
 */

namespace SiegeAssistantTestUtils
{
	/** A plausible mid-match Blue roster. */
	static TArray<FName> MidMatchKinds()
	{
		return TArray<FName>{ TEXT("footman"), TEXT("archer"), TEXT("sorcerer"), TEXT("cleric") };
	}

	/** The arena's named places, mines included. */
	static TArray<FName> MidMatchPlaces()
	{
		return TArray<FName>{ TEXT("enemy_castle"), TEXT("own_castle"), TEXT("mid"), TEXT("ancient_ground_near"), TEXT("ancient_ground_far") };
	}

	/**
	 *  ⭐ THE REGION-BEARING SUBSET (TASK-549, CONVENTIONS AS-§21.4) — the three
	 *  places a shipped `IsPointInZone` answers for. `USiegeAssistantGrammar::Build`
	 *  takes these as a THIRD, TRAILING, DEFAULTED parameter, and every fixture that
	 *  omits it emits NO `inplace` and NO `zone` rule.
	 *
	 *  ⛔ THIS EXISTS BECAUSE LAW ZERO BELOW WAS BLIND TO THE TWO NEW RULES.
	 *  TASK-546 found it: `RuleNameCharset` built four grammars under a comment
	 *  claiming "every shape the builder can produce", and ALL FOUR used the
	 *  two-argument overload — so `inplace` and `zone` were invisible to the one
	 *  test guarding the rule-name charset, in a project that has already shipped
	 *  `at_least` and caught `except_list`.
	 */
	static TArray<FName> MidMatchRegionPlaces()
	{
		return TArray<FName>{ TEXT("mid"), TEXT("ancient_ground_near"), TEXT("ancient_ground_far") };
	}

	/** Returns the right-hand side of `RuleName ::= ...`, or an empty string when the rule is absent. */
	static FString GetRuleRhs(const FString& Grammar, const TCHAR* RuleName)
	{
		TArray<FString> Lines;
		Grammar.ParseIntoArrayLines(Lines, /*bCullEmpty*/ true);

		const FString Prefix = FString(RuleName) + TEXT(" ::= ");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return Line.RightChop(Prefix.Len());
			}
		}

		return FString();
	}

	/** True when a rule with this name is defined at all. */
	static bool HasRule(const FString& Grammar, const TCHAR* RuleName)
	{
		return Grammar.Contains(FString(RuleName) + TEXT(" ::= "), ESearchCase::CaseSensitive);
	}

	/** Splits a rule body on GBNF's top-level ' | '. Safe here because no emitted symbol contains a pipe. */
	static TArray<FString> SplitAlternatives(const FString& Rhs)
	{
		TArray<FString> Alternatives;
		Rhs.ParseIntoArray(Alternatives, TEXT(" | "), /*InCullEmpty*/ true);
		return Alternatives;
	}

	/** Counts whole-word occurrences of a rule reference inside an alternative. */
	static int32 CountRuleReferences(const FString& Alternative, const TCHAR* RuleName)
	{
		TArray<FString> Tokens;
		Alternative.ParseIntoArrayWS(Tokens);

		int32 Total = 0;
		for (const FString& Token : Tokens)
		{
			if (Token.Equals(RuleName, ESearchCase::CaseSensitive))
			{
				++Total;
			}
		}

		return Total;
	}

	/**
	 *  ⛔ THE GBNF RULE-NAME CHARSET. llama.cpp reads a rule name as a run of
	 *  [a-zA-Z0-9-] and STOPS at the first character outside it, so `at_least`
	 *  parses as the name `at`, after which the parser demands `::=`, finds
	 *  `_least`, and rejects the WHOLE grammar.
	 *
	 *  ⚠️ UNDERSCORES ARE CORRECT EVERYWHERE ELSE IN THIS FEATURE — canonical
	 *  place symbols (`ancient_ground_near`) and JSON keys (`at_least`) are lower
	 *  snake_case and are wire format. Only the RULE NAME is kebab-case. That
	 *  asymmetry is exactly what made the defect look reasonable on the page.
	 *
	 *  ⛔ THIS PREDICATE IS THE SINGLE DEFINITION OF THE CHARSET IN THIS FILE.
	 *  Both consumers — IsLegalGbnfRuleName (whole-name legality) and
	 *  IsGrammarWellFormed's identifier scanner (where a reference STOPS) — must
	 *  ask it, because those are the two halves of one question and the defect
	 *  class under test is precisely two things disagreeing about a charset.
	 *  The scanner previously used FChar::IsAlnum, which is Unicode/locale-aware,
	 *  while its sibling used this explicit ASCII range. The divergence happened
	 *  to be safe and unreachable, and it is still exactly how this starts.
	 *
	 *  ⚠️ AND IT CONVERGED ON THE ASCII RANGE, NOT ON THE FRIENDLIER-LOOKING
	 *  FChar::IsAlnum, deliberately: llama.cpp's rule-name scan is plain ASCII,
	 *  so IsAlnum would bless characters (an accented letter, a full-width digit)
	 *  that the real parser breaks an identifier on. Widening the test's notion
	 *  of legality past the parser's is the direction that HIDES a defect, which
	 *  is the whole failure this suite was written after.
	 */
	static bool IsGbnfNameChar(const TCHAR Char)
	{
		return (Char >= TEXT('a') && Char <= TEXT('z'))
			|| (Char >= TEXT('A') && Char <= TEXT('Z'))
			|| (Char >= TEXT('0') && Char <= TEXT('9'))
			|| Char == TEXT('-');
	}

	/**
	 *  Whole-name legality: non-empty, and every character in the charset above.
	 *
	 *  ⚠️ DELIBERATELY A SEPARATE COPY FROM THE PRODUCTION ONE IN
	 *  SiegeAssistantGrammar.cpp, and it has to be: that one has internal linkage
	 *  inside an anonymous namespace and is not declared in the header, so this
	 *  file cannot reach it. Asserting the charset with the very function under
	 *  test would also be circular. The spike's third copy
	 *  (SiegeLlamaSpike.cpp) is separated by a module boundary the plugin is
	 *  architecturally forbidden to cross. Three copies is the cost of those two
	 *  constraints — but WITHIN a file there is exactly one definition, which is
	 *  the part that was actually fixable.
	 */
	static bool IsLegalGbnfRuleName(const FString& Identifier)
	{
		if (Identifier.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Char : Identifier)
		{
			if (!IsGbnfNameChar(Char))
			{
				return false;
			}
		}

		return true;
	}

	/**
	 *  A minimal GBNF well-formedness check: every rule name and every rule
	 *  reference is spelled in llama.cpp's charset, every rule referenced by some
	 *  right-hand side is itself defined, and `root` exists.
	 *
	 *  This is what catches the degenerate-input bug that would otherwise ship
	 *  silently — omitting `kind` because the roster is empty while still
	 *  referencing it from `item` produces a grammar that llama.cpp refuses to
	 *  compile, and nothing else in this suite would notice.
	 *
	 *  ⚠️ THE CHARSET HALF WAS ADDED AFTER THE FACT, AND IT IS WORTH SAYING WHY:
	 *  the identifier scanner below used to accept `_` as an identifier
	 *  character, which is what let `at_least` sail through this very function as
	 *  a well-formed, fully-resolved rule reference. The check agreed with the
	 *  generator instead of with the parser. It now agrees with the parser.
	 */
	static bool IsGrammarWellFormed(const FString& Grammar, FString& OutError)
	{
		OutError.Reset();

		TSet<FString> Defined;
		TArray<FString> Referenced;

		TArray<FString> Lines;
		Grammar.ParseIntoArrayLines(Lines, /*bCullEmpty*/ true);

		for (const FString& Line : Lines)
		{
			const int32 ArrowIndex = Line.Find(TEXT(" ::= "), ESearchCase::CaseSensitive);
			if (ArrowIndex == INDEX_NONE)
			{
				OutError = FString(TEXT("line is not a rule: ")) + Line;
				return false;
			}

			const FString RuleName = Line.Left(ArrowIndex);
			const FString Rhs = Line.RightChop(ArrowIndex + 5);

			if (!IsLegalGbnfRuleName(RuleName))
			{
				OutError = FString(TEXT("illegal rule name (llama.cpp accepts [a-zA-Z0-9-] only): ")) + RuleName;
				return false;
			}

			bool bAlreadyDefined = false;
			Defined.Add(RuleName, &bAlreadyDefined);
			if (bAlreadyDefined)
			{
				OutError = FString(TEXT("duplicate rule: ")) + RuleName;
				return false;
			}

			// Collect identifiers that sit OUTSIDE terminal literals.
			bool bInQuote = false;
			bool bEscaped = false;
			FString Token;

			for (const TCHAR Char : Rhs)
			{
				if (bEscaped)
				{
					bEscaped = false;
					continue;
				}

				if (Char == TEXT('\\'))
				{
					bEscaped = true;
					continue;
				}

				if (Char == TEXT('"'))
				{
					bInQuote = !bInQuote;
					continue;
				}

				if (bInQuote)
				{
					continue;
				}

				// ⚠️ '-', NOT '_'. That one character is the whole point: with '_'
				// accepted here, `at_least` read as a single well-formed reference
				// and this function blessed a grammar llama.cpp cannot load.
				//
				// Shares IsGbnfNameChar with IsLegalGbnfRuleName above rather than
				// carrying its own predicate: where an identifier STOPS and whether
				// an identifier is LEGAL are one question, and two scanners in one
				// file answering it differently is this defect class in miniature.
				if (IsGbnfNameChar(Char))
				{
					Token.AppendChar(Char);
				}
				else if (!Token.IsEmpty())
				{
					Referenced.Add(Token);
					Token.Reset();
				}
			}

			if (!Token.IsEmpty())
			{
				Referenced.Add(Token);
			}

			if (bInQuote)
			{
				OutError = FString(TEXT("unterminated terminal in rule: ")) + RuleName;
				return false;
			}
		}

		if (!Defined.Contains(TEXT("root")))
		{
			OutError = TEXT("no root rule");
			return false;
		}

		for (const FString& Reference : Referenced)
		{
			if (!Defined.Contains(Reference))
			{
				// The scanner can only ever emit [a-zA-Z0-9-] tokens, so an illegal
				// REFERENCE cannot arrive here intact — it arrives as its fragments.
				// `at_least` shows up as an undefined `at` plus an undefined `least`,
				// which is a genuinely confusing thing to read at 2 a.m., so the
				// message says what it is really telling you.
				OutError = FString(TEXT("undefined rule referenced: ")) + Reference
					+ TEXT(" (if this looks like half an identifier, the reference contains a character outside [a-zA-Z0-9-] — most likely an underscore — and llama.cpp split it exactly here)");
				return false;
			}
		}

		return true;
	}

	/** True when the command is exactly default-constructed — the "never partially fills" law. */
	static bool IsDefaultCommand(const FSiegeAssistantCommand& Command)
	{
		return Command.Intent == ESiegeAssistantIntent::None
			&& Command.Kinds.Num() == 0
			&& Command.Counts.Num() == 0
			&& Command.Where.IsNone()
			&& Command.TriggerKind.IsNone()
			&& Command.TriggerAtLeast == 0;
	}
}

// ---------------------------------------------------------------------------
// 0. ⛔ THE RULE-NAME CHARSET — LAW ZERO
// ---------------------------------------------------------------------------
//
// ⚠️ WHY THIS TEST IS FIRST AND WHY IT EXISTS AT ALL.
//
// `at_least` shipped as a rule name in USiegeAssistantGrammar::Build. llama.cpp
// reads a rule name as [a-zA-Z0-9-] and stops at the underscore, so the parser
// saw the name `at`, demanded `::=`, found `_least`, and REJECTED THE ENTIRE
// GRAMMAR:
//
//     parse: error parsing grammar: expecting ::= at _least ::= "1" | "2" | ...
//
// TASK-413's spike then measured the consequence: llama_sampler_init_grammar
// returned NULL on every generation of all six bench runs, and with nothing
// constraining it the model emitted `<think>` reasoning prose instead of JSON on
// every iteration, burning the whole output budget to the abort timeout. Bars #2
// and #5 came back NOT MEASURED.
//
// ⚠️ EVERY OTHER TEST IN THIS FILE PASSED THROUGHOUT. So did two careful human
// reviews, which diffed this generator against the spike's mirror rule-for-rule
// and correctly found them identical — THEY WERE IDENTICALLY UNPARSEABLE. That
// is the specific thing review cannot do and a test can: a property of the
// TARGET PARSER, asserted mechanically, rather than a property shared by two
// copies of the same mistake.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarRuleNameCharsetTest,
	"Siegebound.Assistant.Grammar.RuleNameCharset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarRuleNameCharsetTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	// Every shape the builder can produce, because a rule that is only emitted on
	// one branch is exactly the rule that escapes review.
	//
	// ⚠️⚠️ AMENDED 2026-08-05 (TASK-549, batch AI-COMMANDER ROBUSTNESS) — AND THE
	// AMENDMENT IS THE WHOLE POINT OF THIS COMMENT.
	//
	// ⛔ THE CLAIM ABOVE WAS TRUE WHEN WRITTEN AND BECAME FALSE WITHOUT ANYONE
	// EDITING THIS FILE. `USiegeAssistantGrammar::Build` gained a THIRD, TRAILING,
	// DEFAULTED parameter (`RegionPlaceNames`, TASK-546) which gates two NEW rules
	// — `inplace` and `zone`. All four fixtures below used the TWO-argument
	// overload, so those two rules were emitted on a branch this test never took:
	// the one test guarding the rule-name charset was BLIND to the only new rule
	// names in the batch, and it would have gone on reporting SAFE.
	//
	// ⚖️ THIS IS THE `AS-§21.1` SHAPE, IN A TEST INSTEAD OF IN A PROMPT: an
	// assertion about the rest of the system, falsified by an edit somewhere else,
	// with nobody re-reading it. ⇒ 📌 A FIXTURE LIST THAT CLAIMS TO BE EXHAUSTIVE
	// OWES AN ENTRY TO EVERY NEW DEFAULTED PARAMETER OF THE FUNCTION IT CALLS —
	// because a defaulted parameter is precisely a branch a caller can take without
	// mentioning it.
	//
	// ⛔ AND THE HISTORY IS WHY IT MATTERS RATHER THAN BEING TIDINESS: `at_least`
	// SHIPPED as a rule name and cost TASK-413 two of its six bars; `except_list`
	// was caught before it shipped; `in_place` / `zone_list` would have been the
	// third. Every other property anyone asserts about a grammar is true of a
	// string llama.cpp never loads.
	const TArray<FString> Grammars =
	{
		USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces()),
		USiegeAssistantGrammar::Build(TArray<FName>(), MidMatchPlaces()),
		USiegeAssistantGrammar::Build(MidMatchKinds(), TArray<FName>()),
		USiegeAssistantGrammar::Build(TArray<FName>(), TArray<FName>()),

		// ⭐ THE REGION-BEARING SHAPES (TASK-549). `inplace` is gated on REGIONS
		// ONLY, never on `bHasKinds` (AS-§21.4), so the no-kinds row is not
		// redundant with the kinds row — it is the branch where `who` collapses to
		// `inplace | "all" | "none"` and `zone` is the only generated rule left.
		USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces(), MidMatchRegionPlaces()),
		USiegeAssistantGrammar::Build(TArray<FName>(), MidMatchPlaces(), MidMatchRegionPlaces()),
		USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces(), TArray<FName>{ TEXT("mid") }),
		USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces(), TArray<FName>())
	};

	for (const FString& Grammar : Grammars)
	{
		TArray<FString> Lines;
		Grammar.ParseIntoArrayLines(Lines, /*bCullEmpty*/ true);
		TestTrue(TEXT("The grammar has at least one rule"), Lines.Num() > 0);

		for (const FString& Line : Lines)
		{
			const int32 ArrowIndex = Line.Find(TEXT(" ::= "), ESearchCase::CaseSensitive);
			TestTrue(*FString::Printf(TEXT("Line is a rule: %s"), *Line), ArrowIndex != INDEX_NONE);
			if (ArrowIndex == INDEX_NONE)
			{
				continue;
			}

			const FString RuleName = Line.Left(ArrowIndex);
			TestTrue(
				*FString::Printf(TEXT("Rule name \"%s\" is legal GBNF — llama.cpp accepts [a-zA-Z0-9-] ONLY, and one bad character rejects the WHOLE grammar"), *RuleName),
				IsLegalGbnfRuleName(RuleName));
		}

		// The reference half of the same defect: `at_least` appeared once as a
		// definition and once as a reference, and either alone is fatal.
		// IsGrammarWellFormed's scanner now breaks identifiers on exactly the
		// characters llama.cpp breaks on, so an illegal reference arrives as
		// unresolvable fragments and fails here.
		FString WellFormedError;
		const bool bWellFormed = IsGrammarWellFormed(Grammar, WellFormedError);
		TestTrue(*FString::Printf(TEXT("Every rule reference resolves under llama.cpp's identifier rules (%s)"), *WellFormedError),
			bWellFormed);

	}

	// ⛔⛔ THE SWEEP MUST ACTUALLY HAVE REACHED THE NEW RULES (TASK-549). Without
	// this, the loop above could pass on eight grammars none of which emitted
	// `inplace` — which is EXACTLY the hole this amendment closes, reproduced one
	// level up. A fixture list is only exhaustive if something asserts that it is.
	{
		int32 GrammarsDefiningInplace = 0;
		int32 GrammarsDefiningZone = 0;
		for (const FString& Grammar : Grammars)
		{
			GrammarsDefiningInplace += Grammar.Contains(TEXT("inplace ::="), ESearchCase::CaseSensitive) ? 1 : 0;
			GrammarsDefiningZone += Grammar.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive) ? 1 : 0;
		}

		TestEqual(TEXT("⛔ THREE of the eight fixtures actually EMITTED `inplace` — otherwise this test is blind to the new rule names again"),
			GrammarsDefiningInplace, 3);
		TestEqual(TEXT("⛔ …and the same three emitted `zone`. An `inplace` without a `zone` leaves a DANGLING REFERENCE, which llama.cpp answers by rejecting the whole grammar."),
			GrammarsDefiningZone, 3);

		// The two names this batch could have got wrong, asserted by name so the
		// failure says WHICH spelling arrived rather than "some rule name is illegal".
		TestTrue(TEXT("`inplace` is a legal rule name"), IsLegalGbnfRuleName(TEXT("inplace")));
		TestTrue(TEXT("`zone` is a legal rule name"), IsLegalGbnfRuleName(TEXT("zone")));
		TestFalse(TEXT("⛔ `in_place` is REJECTED — it would parse as the name `in`, and llama.cpp would reject the WHOLE grammar (the shipped `at_least` defect, third instance)"),
			IsLegalGbnfRuleName(TEXT("in_place")));
		TestFalse(TEXT("⛔ `zone_list` is REJECTED for the same reason"), IsLegalGbnfRuleName(TEXT("zone_list")));

		// ⛔ And the JSON KEY `in` needs no kebab-case split at all — it has no
		// underscore to lose. ⚠️ That coincidence is NOT permission to unify
		// `at_least`/`at-least` or `all_except`/`exceptlist`, which do.
		TestTrue(TEXT("The JSON key `in` is emitted inside a TERMINAL, where its (absent) underscore would have been safe anyway"),
			Grammars.IsValidIndex(4) && Grammars[4].Contains(TEXT("\\\"in\\\""), ESearchCase::CaseSensitive));
	}

	// ⚠️ THE OTHER HALF OF THE ASYMMETRY, ASSERTED SO THE RENAME CANNOT BE
	// "MADE CONSISTENT" IN THE WRONG DIRECTION. Rule names are kebab-case, but
	// the JSON keys and canonical place symbols carried INSIDE terminals are
	// snake_case wire format and must keep their underscores — Zone A's schema,
	// the sealed evaluation corpus and ParseSiegeAssistantCommand all assert
	// them. A well-meaning sweep that kebab-cased these would produce a grammar
	// that parses perfectly and emits commands nothing downstream can read.
	{
		const FString Populated = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());
		TestTrue(TEXT("The JSON key \"at_least\" keeps its underscore"), Populated.Contains(TEXT("at_least")));
		TestTrue(TEXT("The place symbol \"ancient_ground_near\" keeps its underscores"),
			Populated.Contains(TEXT("ancient_ground_near")));
		TestTrue(TEXT("The place symbol \"own_castle\" keeps its underscore"), Populated.Contains(TEXT("own_castle")));
	}

	// The guard itself is worth one row: a test that always passes is not a gate.
	TestTrue(TEXT("kebab-case is accepted"), IsLegalGbnfRuleName(TEXT("at-least")));
	TestTrue(TEXT("plain lower-case is accepted"), IsLegalGbnfRuleName(TEXT("root")));
	TestTrue(TEXT("digits are accepted"), IsLegalGbnfRuleName(TEXT("item2")));
	TestFalse(TEXT("snake_case is REJECTED — this is the shipped defect"), IsLegalGbnfRuleName(TEXT("at_least")));
	TestFalse(TEXT("a space is rejected"), IsLegalGbnfRuleName(TEXT("at least")));
	TestFalse(TEXT("an empty name is rejected"), IsLegalGbnfRuleName(FString()));

	return true;
}

// ---------------------------------------------------------------------------
// 1. DETERMINISM
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarDeterminismTest,
	"Siegebound.Assistant.Grammar.Determinism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	const FString First = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());
	const FString Second = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());

	TestTrue(TEXT("Build produced a non-empty grammar"), First.Len() > 0);
	TestEqualSensitive(TEXT("Two builds from equal inputs are byte-identical"), Second, First);

	// FName reports the case it was FIRST constructed with anywhere in the
	// process, so identical game state could otherwise yield two different
	// grammars depending on load order. Build lower-cases every symbol precisely
	// to make that impossible; this asserts the guarantee rather than the habit.
	const TArray<FName> MixedCaseKinds = { TEXT("Footman"), TEXT("ARCHER"), TEXT("SoRcErEr"), TEXT("cleric") };
	const TArray<FName> MixedCasePlaces = { TEXT("Enemy_Castle"), TEXT("OWN_CASTLE"), TEXT("mid"), TEXT("Ancient_Ground_Near"), TEXT("ancient_ground_far") };
	const FString FromMixedCase = USiegeAssistantGrammar::Build(MixedCaseKinds, MixedCasePlaces);

	TestEqualSensitive(TEXT("Symbol casing does not change the emitted grammar"), FromMixedCase, First);

	// Duplicates collapse; caller ordering is preserved and never sorted.
	const TArray<FName> DuplicatedKinds = { TEXT("footman"), TEXT("archer"), TEXT("footman"), TEXT("sorcerer"), TEXT("cleric"), TEXT("archer") };
	TestEqualSensitive(TEXT("Duplicate kinds collapse to the same grammar"),
		USiegeAssistantGrammar::Build(DuplicatedKinds, MidMatchPlaces()), First);

	// Reserved sentinels may never become generated alternatives.
	const TArray<FName> WithReserved = { TEXT("footman"), TEXT("all"), TEXT("archer"), TEXT("none"), TEXT("sorcerer"), TEXT("cleric") };
	TestEqualSensitive(TEXT("Reserved sentinels are dropped from generated alternatives"),
		USiegeAssistantGrammar::Build(WithReserved, MidMatchPlaces()), First);

	return true;
}

// ---------------------------------------------------------------------------
// 2. GROUNDING — an absent kind is not an alternative
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarGroundingTest,
	"Siegebound.Assistant.Grammar.Grounding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarGroundingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	const TArray<FName> WithSorcerer = { TEXT("footman"), TEXT("archer"), TEXT("sorcerer") };
	const TArray<FName> WithoutSorcerer = { TEXT("footman"), TEXT("archer") };

	const FString Alive = USiegeAssistantGrammar::Build(WithSorcerer, MidMatchPlaces());
	const FString Dead = USiegeAssistantGrammar::Build(WithoutSorcerer, MidMatchPlaces());

	const FString AliveKindRule = GetRuleRhs(Alive, TEXT("kind"));
	const FString DeadKindRule = GetRuleRhs(Dead, TEXT("kind"));

	TestTrue(TEXT("A live sorcerer IS an alternative of `kind`"), AliveKindRule.Contains(TEXT("sorcerer")));

	// THE CORE CLAIM OF THE WHOLE FEATURE: with no sorcerer alive the symbol is
	// absent from the entire grammar, so constrained decoding masks every token
	// path that could spell it. Grounding lives in the grammar, not the weights.
	TestFalse(TEXT("A dead sorcerer is NOT an alternative of `kind`"), DeadKindRule.Contains(TEXT("sorcerer")));
	TestFalse(TEXT("A dead sorcerer appears NOWHERE in the grammar"), Dead.Contains(TEXT("sorcerer")));

	TestTrue(TEXT("Surviving kinds are still present"), DeadKindRule.Contains(TEXT("footman")) && DeadKindRule.Contains(TEXT("archer")));

	// The same closed-world rule applies to places.
	const TArray<FName> OnePlace = { TEXT("mid") };
	const FString OnePlaceGrammar = USiegeAssistantGrammar::Build(WithSorcerer, OnePlace);
	const FString OnePlaceRule = GetRuleRhs(OnePlaceGrammar, TEXT("where"));

	TestTrue(TEXT("A single-entry place list yields a well-formed `where`"), OnePlaceRule.Contains(TEXT("\\\"mid\\\"")));
	TestFalse(TEXT("An unlisted place is absent"), OnePlaceGrammar.Contains(TEXT("enemy_castle")));

	// "none" is ALWAYS reachable for `where`: the army-wide verbs have no
	// destination, and forcing the model to name one invites a confident guess.
	TestTrue(TEXT("`where` always offers the none sentinel"), OnePlaceRule.Contains(TEXT("none")));
	TestEqual(TEXT("A one-place `where` has exactly two alternatives (the place + none)"),
		SplitAlternatives(OnePlaceRule).Num(), 2);

	return true;
}

// ---------------------------------------------------------------------------
// 3. THE SEVEN INTENTS
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarIntentTest,
	"Siegebound.Assistant.Grammar.Intents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarIntentTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	const FString Grammar = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());
	const FString IntentRule = GetRuleRhs(Grammar, TEXT("intent"));

	const TArray<FString> Alternatives = SplitAlternatives(IntentRule);
	TestEqual(TEXT("`intent` offers exactly seven alternatives"), Alternatives.Num(), 7);

	const TCHAR* ExpectedIntents[] = { TEXT("send"), TEXT("guard"), TEXT("ambush"), TEXT("follow"), TEXT("charge"), TEXT("fallback"), TEXT("rally") };
	for (const TCHAR* Expected : ExpectedIntents)
	{
		TestTrue(*FString::Printf(TEXT("`intent` includes %s"), Expected), IntentRule.Contains(Expected));
	}

	// ESiegeAssistantIntent::None is the parser's reset value and is never a
	// thing the model may say.
	TestFalse(TEXT("`intent` does NOT include the None value"), IntentRule.Contains(TEXT("\\\"none\\\"")));

	// The emitted vocabulary and the accepted vocabulary come from the same
	// reflection walk, so they cannot drift apart.
	TArray<FString> Symbols;
	SiegeAssistantIntentSymbols(Symbols);
	TestEqual(TEXT("The reflected symbol list has seven entries"), Symbols.Num(), 7);

	for (const FString& Symbol : Symbols)
	{
		ESiegeAssistantIntent RoundTripped = ESiegeAssistantIntent::None;
		TestTrue(*FString::Printf(TEXT("Symbol %s parses back to an intent"), *Symbol),
			SiegeAssistantIntentFromSymbol(Symbol, RoundTripped));
		TestEqualSensitive(*FString::Printf(TEXT("Symbol %s round-trips"), *Symbol),
			SiegeAssistantIntentToSymbol(RoundTripped), Symbol);
	}

	ESiegeAssistantIntent Unused = ESiegeAssistantIntent::None;
	TestFalse(TEXT("\"none\" is not an acceptable intent symbol"), SiegeAssistantIntentFromSymbol(TEXT("none"), Unused));
	TestFalse(TEXT("An invented verb is not an acceptable intent symbol"), SiegeAssistantIntentFromSymbol(TEXT("teleport"), Unused));

	return true;
}

// ---------------------------------------------------------------------------
// 4. ⚠️ THE COUNT RANGE — 1..30, NOT the live maximum
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarCountRangeTest,
	"Siegebound.Assistant.Grammar.CountRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarCountRangeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	// Two live footmen. If `count` were "helpfully" clamped to the live roster —
	// which is the single most likely well-intentioned regression in this file —
	// a player asking for 10 would get a silently-emitted 2 and the shortfall
	// clarification would become undetectable. The range must stay 1..30.
	const TArray<FName> TinyRoster = { TEXT("footman") };
	const FString Grammar = USiegeAssistantGrammar::Build(TinyRoster, MidMatchPlaces());
	const FString CountRule = GetRuleRhs(Grammar, TEXT("count"));

	const TArray<FString> Alternatives = SplitAlternatives(CountRule);
	TestEqual(TEXT("`count` offers 30 numbers plus \"all\""), Alternatives.Num(), USiegeAssistantGrammar::GrammarCountMax + 1);

	for (int32 Quantity = USiegeAssistantGrammar::GrammarCountMin; Quantity <= USiegeAssistantGrammar::GrammarCountMax; ++Quantity)
	{
		const FString Expected = FString(TEXT("\"")) + FString::FromInt(Quantity) + TEXT("\"");
		TestTrue(*FString::Printf(TEXT("`count` includes the terminal %d"), Quantity), Alternatives.Contains(Expected));
	}

	TestTrue(TEXT("`count` includes the \"all\" selector"), CountRule.Contains(TEXT("all")));
	TestFalse(TEXT("`count` stops at exactly 30 — 31 is not an alternative"), Alternatives.Contains(TEXT("\"31\"")));
	TestFalse(TEXT("`count` does not offer 0"), Alternatives.Contains(TEXT("\"0\"")));

	// The deferred trigger takes the same numeric range WITHOUT "all": "wait
	// until I have all footmen" is a condition that can never become true.
	//
	// ⛔ THE RULE IS `at-least`. THE JSON KEY IS `at_least`. Asserting the rule
	// under the key's spelling is what this test used to do, and it passed for as
	// long as the generator emitted an unparseable grammar.
	const FString AtLeastRule = GetRuleRhs(Grammar, TEXT("at-least"));
	const TArray<FString> AtLeastAlternatives = SplitAlternatives(AtLeastRule);
	TestEqual(TEXT("`at-least` offers exactly 30 numbers"), AtLeastAlternatives.Num(), USiegeAssistantGrammar::GrammarCountMax);
	TestFalse(TEXT("`at-least` does NOT offer \"all\""), AtLeastRule.Contains(TEXT("all")));

	// The rule was renamed; the WIRE FORMAT was not. `when`'s deferred branch
	// must still emit the JSON key `at_least`, because Zone A's schema, the
	// sealed evaluation corpus and ParseSiegeAssistantCommand all assert it.
	const FString WhenRule = GetRuleRhs(Grammar, TEXT("when"));
	TestTrue(TEXT("`when` still emits the JSON key \"at_least\" (snake_case wire format)"),
		WhenRule.Contains(TEXT("at_least")));
	TestTrue(TEXT("`when` references the rule `at-least` (kebab-case GBNF name)"),
		CountRuleReferences(WhenRule, TEXT("at-least")) == 1);

	return true;
}

// ---------------------------------------------------------------------------
// 5. THE MULTI-KIND SELECTION CAP (manager ruling 15)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarSelectionCapTest,
	"Siegebound.Assistant.Grammar.SelectionCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarSelectionCapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	const FString Grammar = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());
	const FString SelectionRule = GetRuleRhs(Grammar, TEXT("selection"));

	TestTrue(TEXT("A non-empty roster defines a `selection` rule"), SelectionRule.Len() > 0);

	const TArray<FString> Alternatives = SplitAlternatives(SelectionRule);
	TestEqual(TEXT("`selection` offers exactly one alternative per permitted arity"),
		Alternatives.Num(), SiegeAssistantMaxSelectionKinds);

	// Alternative N must reference `item` exactly N times: 1, 2, 3 — and there is
	// no fourth alternative, so a 4-kind selection is not reachable by the
	// sampler at all rather than being caught downstream.
	for (int32 Index = 0; Index < Alternatives.Num(); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("`selection` alternative %d holds %d items"), Index + 1, Index + 1),
			CountRuleReferences(Alternatives[Index], TEXT("item")), Index + 1);
	}

	// ⚠️ THE CAP MUST BE A BOUNDED ALTERNATION, NOT A REPETITION. An unbounded
	// repetition rule is exactly what a small model rambles into, so the absence
	// of GBNF's repetition operators anywhere in the grammar is the assertion
	// that keeps the cap real.
	TestFalse(TEXT("The grammar contains no '*' repetition operator"), Grammar.Contains(TEXT("*")));
	TestFalse(TEXT("The grammar contains no '+' repetition operator"), Grammar.Contains(TEXT("+")));
	TestFalse(TEXT("The grammar contains no '?' optional operator"), Grammar.Contains(TEXT("?")));

	// ── `who` WRAPS THE CAPPED SELECTION ALONGSIDE THE WHOLE-ARMY SELECTORS ────
	//
	// ⭐ RE-BASED 3 → 4 ON 2026-08-04 (TASK-523, batch ASSISTANT-EXCLUDE).
	// TASK-518 added the `except` alternative — `{"all_except":["miner"]}`, a
	// THIRD SHAPE FOR AN EXISTING KEY and deliberately NOT a fourth top-level key
	// (AS-§20.1) — and left this assertion for TASK-523 rather than editing a test
	// file it did not own.
	//
	// ⚠️ THE COUNT IS RE-BASED, NOT LOOSENED. The obvious "fix" for a broken count
	// assertion is `>= 3`, which would then pass for any future widening of `who`
	// including an accidental one; `who`'s alternative set is a SCHEMA, and an
	// exact count is the only assertion that notices a fifth shape arriving
	// unannounced.
	//
	// ⛔ THE EMPTY-ROSTER COUNT STAYS 2 AND IS UNCHANGED — see
	// `Siegebound.Assistant.Grammar.DegenerateInputs`. `except` is gated on
	// `bHasKinds` for exactly the reason `selection` is: an empty roster has
	// nothing to exclude, and emitting the alternative anyway would leave
	// `exceptlist` referencing an UNDEFINED `kind` rule, which breaks the whole
	// grammar rather than one rule of it.
	//
	// 📌 The `except` / `exceptlist` rules themselves — the 1..3 arity bound, the
	// BARE kind strings that make Jonathan's declined "all except 5 archers"
	// inexpressible, and the empty-roster omission — are asserted in
	// `Siegebound.Assistant.Selection.GrammarAdmitsExceptOnlyWithKinds`. They are
	// NOT duplicated here: this test's subject is the SELECTION cap.
	const FString WhoRule = GetRuleRhs(Grammar, TEXT("who"));
	TestEqual(TEXT("`who` offers selection | except | \"all\" | \"none\" (4 alternatives since TASK-518; was 3)"),
		SplitAlternatives(WhoRule).Num(), 4);
	TestTrue(TEXT("`who` references `selection`"), CountRuleReferences(WhoRule, TEXT("selection")) == 1);
	TestTrue(TEXT("`who` references `except`"), CountRuleReferences(WhoRule, TEXT("except")) == 1);

	return true;
}

// ---------------------------------------------------------------------------
// 6. DEGENERATE INPUTS
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGrammarDegenerateTest,
	"Siegebound.Assistant.Grammar.DegenerateInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGrammarDegenerateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	// EMPTY ROSTER. With nothing nameable, the selection machinery is unreachable
	// and is omitted entirely rather than emitted as a dangling `kind ::= "none"`
	// that `item` would then reference. What remains is exactly what is true: you
	// can still charge, fall back, rally, or ask a question.
	const FString EmptyRoster = USiegeAssistantGrammar::Build(TArray<FName>(), MidMatchPlaces());

	// Evaluated into locals first: reading the error string inside the same call
	// that populates it would report an empty reason, because argument
	// evaluation order is not sequenced.
	FString WellFormedError;
	{
		const bool bWellFormed = IsGrammarWellFormed(EmptyRoster, WellFormedError);
		TestTrue(*FString::Printf(TEXT("An empty roster still yields a well-formed grammar (%s)"), *WellFormedError), bWellFormed);
	}

	TestFalse(TEXT("An empty roster omits `kind`"), HasRule(EmptyRoster, TEXT("kind")));
	TestFalse(TEXT("An empty roster omits `item`"), HasRule(EmptyRoster, TEXT("item")));
	TestFalse(TEXT("An empty roster omits `selection`"), HasRule(EmptyRoster, TEXT("selection")));
	TestFalse(TEXT("An empty roster omits `count`"), HasRule(EmptyRoster, TEXT("count")));
	TestFalse(TEXT("An empty roster omits `at-least`"), HasRule(EmptyRoster, TEXT("at-least")));

	TestTrue(TEXT("An empty roster still defines root/command/question/intent/where/who/when"),
		HasRule(EmptyRoster, TEXT("root"))
		&& HasRule(EmptyRoster, TEXT("command"))
		&& HasRule(EmptyRoster, TEXT("question"))
		&& HasRule(EmptyRoster, TEXT("intent"))
		&& HasRule(EmptyRoster, TEXT("where"))
		&& HasRule(EmptyRoster, TEXT("who"))
		&& HasRule(EmptyRoster, TEXT("when")));

	TestEqual(TEXT("An empty roster leaves `who` with only the two whole-army selectors"),
		SplitAlternatives(GetRuleRhs(EmptyRoster, TEXT("who"))).Num(), 2);
	TestEqual(TEXT("An empty roster leaves `when` with only \"now\""),
		SplitAlternatives(GetRuleRhs(EmptyRoster, TEXT("when"))).Num(), 1);

	// EMPTY PLACE LIST. `where` collapses to the none sentinel and stays legal.
	const FString NoPlaces = USiegeAssistantGrammar::Build(MidMatchKinds(), TArray<FName>());
	{
		const bool bWellFormed = IsGrammarWellFormed(NoPlaces, WellFormedError);
		TestTrue(*FString::Printf(TEXT("An empty place list still yields a well-formed grammar (%s)"), *WellFormedError), bWellFormed);
	}
	TestEqual(TEXT("An empty place list leaves `where` with only the none sentinel"),
		SplitAlternatives(GetRuleRhs(NoPlaces, TEXT("where"))).Num(), 1);

	// BOTH EMPTY — the worst case, and still a compilable grammar.
	const FString Nothing = USiegeAssistantGrammar::Build(TArray<FName>(), TArray<FName>());
	{
		const bool bWellFormed = IsGrammarWellFormed(Nothing, WellFormedError);
		TestTrue(*FString::Printf(TEXT("Two empty arrays still yield a well-formed grammar (%s)"), *WellFormedError), bWellFormed);
	}
	TestEqualSensitive(TEXT("Two empty arrays are deterministic too"),
		USiegeAssistantGrammar::Build(TArray<FName>(), TArray<FName>()), Nothing);

	// The populated case must obviously also be well-formed.
	const FString Populated = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());
	{
		const bool bWellFormed = IsGrammarWellFormed(Populated, WellFormedError);
		TestTrue(*FString::Printf(TEXT("The populated grammar is well-formed (%s)"), *WellFormedError), bWellFormed);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 7. ROUND-TRIP: grammar-conforming JSON -> struct
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantCommandRoundTripTest,
	"Siegebound.Assistant.Command.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantCommandRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	// Build the grammar this output would have been decoded against, so the test
	// documents the pairing even though the parser is independent of it.
	const FString Grammar = USiegeAssistantGrammar::Build(MidMatchKinds(), MidMatchPlaces());
	TestTrue(TEXT("The fixture grammar was produced"), Grammar.Len() > 0);

	// --- 1 kind ---------------------------------------------------------------
	{
		const FString Json = TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10}],\"where\":\"ancient_ground_near\",\"when\":\"now\"}");

		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("A single-kind order parses"), ParseSiegeAssistantCommand(Json, Command, Error));
		TestEqual(TEXT("Single-kind: intent"), static_cast<int32>(Command.Intent), static_cast<int32>(ESiegeAssistantIntent::Send));
		TestEqual(TEXT("Single-kind: one kind"), Command.Kinds.Num(), 1);
		TestEqualSensitive(TEXT("Single-kind: kind symbol"), Command.Kinds[0].ToString(), FString(TEXT("footman")));
		TestEqual(TEXT("Single-kind: count"), Command.Counts[0], 10);
		TestEqualSensitive(TEXT("Single-kind: where"), Command.Where.ToString(), FString(TEXT("ancient_ground_near")));
		TestTrue(TEXT("Single-kind: \"now\" leaves no trigger"), Command.TriggerKind.IsNone() && Command.TriggerAtLeast == 0);
	}

	// --- 2 kinds — THE FEATURE'S FLAGSHIP SENTENCE ----------------------------
	// "send 10 footmen with a sorcerer to the nearest ancient ground". This is
	// the utterance the whole feature was requested for, and it is the exact
	// sentence the original singular-Kind pin could not express (manager ruling
	// 15). If this row ever fails, the struct has been narrowed again.
	{
		const FString Json = TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1}],\"where\":\"ancient_ground_near\",\"when\":\"now\"}");

		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The flagship two-kind order parses"), ParseSiegeAssistantCommand(Json, Command, Error));
		TestEqual(TEXT("Flagship: two kinds"), Command.Kinds.Num(), 2);
		TestEqual(TEXT("Flagship: counts are index-aligned with kinds"), Command.Counts.Num(), Command.Kinds.Num());
		TestEqualSensitive(TEXT("Flagship: kind 0"), Command.Kinds[0].ToString(), FString(TEXT("footman")));
		TestEqual(TEXT("Flagship: count 0"), Command.Counts[0], 10);
		TestEqualSensitive(TEXT("Flagship: kind 1"), Command.Kinds[1].ToString(), FString(TEXT("sorcerer")));
		TestEqual(TEXT("Flagship: count 1"), Command.Counts[1], 1);
		TestEqualSensitive(TEXT("Flagship: where"), Command.Where.ToString(), FString(TEXT("ancient_ground_near")));
	}

	// --- 3 kinds, exactly at the cap ------------------------------------------
	{
		const FString Json = TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"footman\",\"n\":8},{\"kind\":\"archer\",\"n\":4},{\"kind\":\"cleric\",\"n\":1}],\"where\":\"mid\",\"when\":\"now\"}");

		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("A three-kind order at the cap parses"), ParseSiegeAssistantCommand(Json, Command, Error));
		TestEqual(TEXT("Cap: three kinds"), Command.Kinds.Num(), SiegeAssistantMaxSelectionKinds);
		TestEqual(TEXT("Cap: three counts"), Command.Counts.Num(), SiegeAssistantMaxSelectionKinds);
		TestEqual(TEXT("Cap: intent"), static_cast<int32>(Command.Intent), static_cast<int32>(ESiegeAssistantIntent::Guard));
	}

	// --- "all" maps to 0 -------------------------------------------------------
	{
		const FString Json = TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":\"all\"}],\"where\":\"mid\",\"when\":\"now\"}");

		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("An \"all\" quantity parses"), ParseSiegeAssistantCommand(Json, Command, Error));
		TestEqual(TEXT("\"all\" maps to Count 0"), Command.Counts[0], 0);
	}

	// --- the whole-army selectors ---------------------------------------------
	{
		FSiegeAssistantCommand Command;
		FString Error;

		TestTrue(TEXT("charge + who:none parses"),
			ParseSiegeAssistantCommand(TEXT("{\"intent\":\"charge\",\"who\":\"none\",\"where\":\"none\",\"when\":\"now\"}"), Command, Error));
		TestEqual(TEXT("An army-wide order carries no selection"), Command.Kinds.Num(), 0);
		TestTrue(TEXT("where:none leaves Where unset"), Command.Where.IsNone());

		TestTrue(TEXT("fallback + who:all parses"),
			ParseSiegeAssistantCommand(TEXT("{\"intent\":\"fallback\",\"who\":\"all\",\"where\":\"own_castle\",\"when\":\"now\"}"), Command, Error));
		TestEqual(TEXT("who:all carries no explicit kinds"), Command.Kinds.Num(), 0);
		TestEqualSensitive(TEXT("fallback keeps its place"), Command.Where.ToString(), FString(TEXT("own_castle")));

		// A selection-bearing verb MAY name every eligible unit.
		TestTrue(TEXT("send + who:all parses"),
			ParseSiegeAssistantCommand(TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":\"now\"}"), Command, Error));
	}

	// --- the deferred trigger --------------------------------------------------
	{
		const FString Json = TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10}],\"where\":\"enemy_castle\",\"when\":{\"kind\":\"footman\",\"at_least\":10}}");

		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("A deferred order parses"), ParseSiegeAssistantCommand(Json, Command, Error));
		TestEqualSensitive(TEXT("Deferred: trigger kind"), Command.TriggerKind.ToString(), FString(TEXT("footman")));
		TestEqual(TEXT("Deferred: threshold"), Command.TriggerAtLeast, 10);
	}

	// --- the question branch ---------------------------------------------------
	// A question is well-formed output that is NOT an executable command, so the
	// parser returns false and hands the FSM an "ask:" reason to route to Clarify
	// rather than Failed.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("A question is not an executable command"),
			ParseSiegeAssistantCommand(TEXT("{\"ask\":\"which_unit\"}"), Command, Error));
		TestEqualSensitive(TEXT("A question reports the ask code"), Error, FString(TEXT("ask:which_unit")));
		TestEqualSensitive(TEXT("SiegeAssistantReasonCode splits the payload off"), SiegeAssistantReasonCode(Error), FString(TEXT("ask")));
		TestTrue(TEXT("A question leaves the command default"), IsDefaultCommand(Command));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 8. THE REJECTION BATTERY
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantCommandRejectionTest,
	"Siegebound.Assistant.Command.Rejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantCommandRejectionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	struct FRejectionCase
	{
		const TCHAR* Description;
		const TCHAR* Json;
		const TCHAR* ExpectedCode;
	};

	const FRejectionCase Cases[] =
	{
		{ TEXT("empty string"), TEXT(""), SiegeAssistantReason::EmptyInput },
		{ TEXT("whitespace only"), TEXT("   \n\t "), SiegeAssistantReason::EmptyInput },
		{ TEXT("not JSON at all"), TEXT("send ten footmen"), SiegeAssistantReason::MalformedJson },
		{ TEXT("truncated object"), TEXT("{\"intent\":\"send\","), SiegeAssistantReason::MalformedJson },
		{ TEXT("a JSON array root"), TEXT("[1,2,3]"), SiegeAssistantReason::MalformedJson },

		{ TEXT("an extra key"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":\"now\",\"speed\":2}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("a case-wrong key"), TEXT("{\"Intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("a missing key"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\"}"), SiegeAssistantReason::MissingKey },
		{ TEXT("a question with an extra key"), TEXT("{\"ask\":\"which_unit\",\"intent\":\"send\"}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("an invented ask code"), TEXT("{\"ask\":\"why\"}"), SiegeAssistantReason::UnknownAsk },

		{ TEXT("the None intent"), TEXT("{\"intent\":\"none\",\"who\":\"all\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::UnknownIntent },
		{ TEXT("an invented verb"), TEXT("{\"intent\":\"teleport\",\"who\":\"all\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::UnknownIntent },
		{ TEXT("a numeric intent"), TEXT("{\"intent\":1,\"who\":\"all\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::BadType },

		{ TEXT("a count of 0"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":0}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::CountOutOfRange },
		{ TEXT("a count of 31"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":31}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::CountOutOfRange },
		{ TEXT("a negative count"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":-4}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::CountOutOfRange },
		{ TEXT("a fractional count"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":2.5}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::CountOutOfRange },
		{ TEXT("a stringified count"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":\"5\"}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::CountOutOfRange },

		{ TEXT("an empty selection"), TEXT("{\"intent\":\"send\",\"who\":[],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::WhoArity },
		{ TEXT("a FOUR-kind selection"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":1},{\"kind\":\"archer\",\"n\":1},{\"kind\":\"cleric\",\"n\":1},{\"kind\":\"ogre\",\"n\":1}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::WhoArity },
		{ TEXT("a repeated kind"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":5},{\"kind\":\"footman\",\"n\":3}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::DuplicateKind },
		{ TEXT("an item with an extra key"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":5,\"hp\":10}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("an item missing its count"), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\"}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::MissingKey },
		{ TEXT("the reserved kind \"none\""), TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"none\",\"n\":5}],\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::BadKind },
		{ TEXT("an invented who selector"), TEXT("{\"intent\":\"send\",\"who\":\"nobody\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::BadWho },
		{ TEXT("a numeric who"), TEXT("{\"intent\":\"send\",\"who\":3,\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::BadType },

		// ⚠️ THE REPRESENTABILITY GUARD. "none" and "all" both land on an empty
		// selection, so accepting who:none for a selection-bearing verb would
		// silently turn "send nobody" into "send everybody".
		{ TEXT("send with who:none"), TEXT("{\"intent\":\"send\",\"who\":\"none\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::WhoRequired },
		{ TEXT("guard with who:none"), TEXT("{\"intent\":\"guard\",\"who\":\"none\",\"where\":\"mid\",\"when\":\"now\"}"), SiegeAssistantReason::WhoRequired },
		{ TEXT("follow with who:none"), TEXT("{\"intent\":\"follow\",\"who\":\"none\",\"where\":\"none\",\"when\":\"now\"}"), SiegeAssistantReason::WhoRequired },

		{ TEXT("an empty place symbol"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"\",\"when\":\"now\"}"), SiegeAssistantReason::BadWhere },
		{ TEXT("a numeric place"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":7,\"when\":\"now\"}"), SiegeAssistantReason::BadType },

		{ TEXT("an invented when"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":\"later\"}"), SiegeAssistantReason::BadWhen },
		{ TEXT("a trigger missing at_least"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":{\"kind\":\"footman\"}}"), SiegeAssistantReason::MissingKey },
		{ TEXT("a trigger with an extra key"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":{\"kind\":\"footman\",\"at_least\":2,\"within\":30}}"), SiegeAssistantReason::UnknownKey },
		// "wait until I have ALL footmen" can never become true, so it is not accepted.
		{ TEXT("a trigger threshold of \"all\""), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":{\"kind\":\"footman\",\"at_least\":\"all\"}}"), SiegeAssistantReason::CountOutOfRange },
		{ TEXT("a trigger threshold of 0"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":{\"kind\":\"footman\",\"at_least\":0}}"), SiegeAssistantReason::CountOutOfRange },
		{ TEXT("a numeric when"), TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":5}"), SiegeAssistantReason::BadType }
	};

	for (const FRejectionCase& Case : Cases)
	{
		FSiegeAssistantCommand Command;
		FString Error;

		const bool bAccepted = ParseSiegeAssistantCommand(Case.Json, Command, Error);

		TestFalse(*FString::Printf(TEXT("REJECT: %s"), Case.Description), bAccepted);
		TestEqualSensitive(*FString::Printf(TEXT("REJECT: %s reports the right code"), Case.Description),
			SiegeAssistantReasonCode(Error), FString(Case.ExpectedCode));

		// ⚠️ NEVER PARTIALLY FILLS. A caller that ignores the return value must be
		// left holding an inert None command, not half an order.
		TestTrue(*FString::Printf(TEXT("REJECT: %s leaves the command default-constructed"), Case.Description),
			IsDefaultCommand(Command));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 9. "NEVER PARTIALLY FILLS" against a pre-poisoned out-param
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantCommandNeverPartialTest,
	"Siegebound.Assistant.Command.NeverPartiallyFills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantCommandNeverPartialTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantTestUtils;

	// A previous, valid order still sitting in the out-param. If the parser
	// reset only on entry and then bailed midway, the caller would execute a
	// stale command believing it was the new one.
	FSiegeAssistantCommand Poisoned;
	Poisoned.Intent = ESiegeAssistantIntent::Ambush;
	Poisoned.Kinds.Add(TEXT("ogre"));
	Poisoned.Counts.Add(7);
	Poisoned.Where = TEXT("enemy_castle");
	Poisoned.TriggerKind = TEXT("archer");
	Poisoned.TriggerAtLeast = 3;

	FString Error;

	// Fails LATE — intent, who and where all parse before `when` is rejected.
	const bool bAccepted = ParseSiegeAssistantCommand(
		TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":4}],\"where\":\"mid\",\"when\":\"eventually\"}"),
		Poisoned, Error);

	TestFalse(TEXT("A late failure is still a failure"), bAccepted);
	TestEqualSensitive(TEXT("A late failure reports bad_when"), SiegeAssistantReasonCode(Error), FString(SiegeAssistantReason::BadWhen));
	TestTrue(TEXT("A late failure wipes the previously valid command"), IsDefaultCommand(Poisoned));

	// The success path must also fully overwrite prior contents.
	FSiegeAssistantCommand Reused;
	Reused.Kinds.Add(TEXT("ogre"));
	Reused.Counts.Add(7);
	Reused.TriggerKind = TEXT("archer");
	Reused.TriggerAtLeast = 3;

	TestTrue(TEXT("A later success parses"),
		ParseSiegeAssistantCommand(TEXT("{\"intent\":\"rally\",\"who\":\"none\",\"where\":\"none\",\"when\":\"now\"}"), Reused, Error));
	TestEqual(TEXT("Success clears stale kinds"), Reused.Kinds.Num(), 0);
	TestEqual(TEXT("Success clears stale counts"), Reused.Counts.Num(), 0);
	TestTrue(TEXT("Success clears a stale trigger"), Reused.TriggerKind.IsNone() && Reused.TriggerAtLeast == 0);
	TestEqualSensitive(TEXT("Success reports an empty error"), Error, FString());

	return true;
}

// ---------------------------------------------------------------------------
// 10. THE SELECTION INVARIANTS (the M8 P2 receive-side validator)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionInvariantTest,
	"Siegebound.Assistant.Command.SelectionInvariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionInvariantTest::RunTest(const FString& Parameters)
{
	FString Error;

	// A mismatch cannot arise from the parser — it fills both arrays in one walk
	// — but in M8 P2 this struct arrives OVER THE WIRE from a peer that never
	// passed through anyone's grammar. This is that receive-side check, and the
	// law it encodes is "reject, never truncate".
	{
		const TArray<FName> Kinds = { TEXT("footman"), TEXT("archer") };
		const TArray<int32> Counts = { 10 };

		TestFalse(TEXT("A length mismatch is rejected"), SiegeAssistantValidateSelection(Kinds, Counts, Error));
		TestEqualSensitive(TEXT("A length mismatch reports selection_mismatch"),
			SiegeAssistantReasonCode(Error), FString(SiegeAssistantReason::SelectionMismatch));
		TestEqualSensitive(TEXT("The mismatch payload records both lengths"), Error, FString(TEXT("selection_mismatch:2/1")));
	}

	{
		const TArray<FName> Kinds = { TEXT("footman"), TEXT("archer"), TEXT("cleric"), TEXT("ogre") };
		const TArray<int32> Counts = { 1, 1, 1, 1 };

		TestFalse(TEXT("Four kinds exceed the cap"), SiegeAssistantValidateSelection(Kinds, Counts, Error));
		TestEqualSensitive(TEXT("An over-cap selection reports selection_overflow"),
			SiegeAssistantReasonCode(Error), FString(SiegeAssistantReason::SelectionOverflow));
	}

	{
		const TArray<FName> Kinds = { TEXT("footman"), TEXT("footman") };
		const TArray<int32> Counts = { 5, 3 };

		TestFalse(TEXT("A repeated kind is rejected"), SiegeAssistantValidateSelection(Kinds, Counts, Error));
		TestEqualSensitive(TEXT("A repeated kind reports duplicate_kind"),
			SiegeAssistantReasonCode(Error), FString(SiegeAssistantReason::DuplicateKind));
	}

	{
		const TArray<FName> Kinds = { TEXT("footman"), NAME_None };
		const TArray<int32> Counts = { 5, 3 };

		TestFalse(TEXT("A NAME_None kind is rejected"), SiegeAssistantValidateSelection(Kinds, Counts, Error));
		TestEqualSensitive(TEXT("A NAME_None kind reports bad_kind"),
			SiegeAssistantReasonCode(Error), FString(SiegeAssistantReason::BadKind));
	}

	{
		// The legal shapes: empty (whole army), and 1..cap distinct kinds.
		TestTrue(TEXT("An empty selection is valid"), SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), Error));
		TestEqualSensitive(TEXT("A valid selection reports no error"), Error, FString());

		const TArray<FName> Kinds = { TEXT("footman"), TEXT("archer"), TEXT("sorcerer") };
		const TArray<int32> Counts = { 10, 5, 1 };
		TestTrue(TEXT("A full-cap distinct selection is valid"), SiegeAssistantValidateSelection(Kinds, Counts, Error));
	}

	// The verb split the executor seam rests on.
	TestTrue(TEXT("send takes a selection"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Send));
	TestTrue(TEXT("guard takes a selection"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Guard));
	TestTrue(TEXT("ambush takes a selection"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Ambush));
	TestTrue(TEXT("follow takes a selection"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Follow));
	TestFalse(TEXT("charge is army-wide"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Charge));
	TestFalse(TEXT("fallback is army-wide"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Fallback));
	TestFalse(TEXT("rally is hero-only"), SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Rally));

	return true;
}

// ---------------------------------------------------------------------------
// 11. THE VOCABULARY — byte-stability and the Sorcerer/Wizard collision
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantVocabularyTest,
	"Siegebound.Assistant.Vocabulary.SynonymTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantVocabularyTest::RunTest(const FString& Parameters)
{
	const USiegeAssistantVocabulary* Vocabulary = GetDefault<USiegeAssistantVocabulary>();
	if (Vocabulary == nullptr)
	{
		AddError(TEXT("USiegeAssistantVocabulary has no CDO"));
		return false;
	}

	// Zone A must be byte-identical for the life of the process, or the KV-prefix
	// reuse that turns a ~500-token prefill into ~150 silently stops working.
	const FString First = Vocabulary->BuildSynonymTable();
	const FString Second = Vocabulary->BuildSynonymTable();
	TestEqualSensitive(TEXT("BuildSynonymTable is byte-stable across calls"), Second, First);
	TestTrue(TEXT("The default vocabulary is not empty"), First.Len() > 0);

	// Row ORDER in the asset must not change the bytes, because an artist
	// re-ordering DA_AssistantVocabulary would otherwise silently invalidate the
	// cached prefix.
	{
		USiegeAssistantVocabulary* Shuffled = NewObject<USiegeAssistantVocabulary>();
		TestTrue(TEXT("A vocabulary instance was created"), Shuffled != nullptr);

		if (Shuffled != nullptr)
		{
			Algo::Reverse(Shuffled->UnitSynonyms);
			Algo::Reverse(Shuffled->PlaceSynonyms);
			Algo::Reverse(Shuffled->IntentSynonyms);

			for (FSiegeAssistantSynonym& Row : Shuffled->UnitSynonyms)
			{
				Algo::Reverse(Row.Aliases);
			}

			TestEqualSensitive(TEXT("Row and alias ordering do not change the emitted table"),
				Shuffled->BuildSynonymTable(), First);
		}
	}

	// ⚠️ THE SORCERER/WIZARD COLLISION. They are different cards, and a shared
	// alias would make the assistant confidently command the wrong unit — worse
	// than failing. CONVENTIONS §11 makes this a mandatory adversarial row in the
	// sealed evaluation corpus, so it is asserted here too.
	TestTrue(TEXT("The table lists wizard"), First.Contains(TEXT("wizard <- ")));
	TestTrue(TEXT("The table lists sorcerer"), First.Contains(TEXT("sorcerer <- ")));

	for (const FSiegeAssistantSynonym& Row : Vocabulary->UnitSynonyms)
	{
		const FString Canonical = Row.Canonical.ToString().ToLower();
		const bool bIsCaster = (Canonical == TEXT("wizard")) || (Canonical == TEXT("sorcerer"));

		// ⚠️⚠️ THE GUARD THAT WOULD HAVE CAUGHT TASK-419's BLOCKER-1 MECHANICALLY.
		// A unit symbol is a CardID lower-cased with NO separator inserted, and
		// CardIDs are PascalCase single tokens — so a unit canonical can never
		// legitimately contain an underscore. `militia_mob` shipped here and was
		// unreachable, because USiegeAssistantSnapshot::CanonicalKind is a pure
		// total derivation that only ever emits `militiamob`. The prompt then
		// taught the model a spelling the sampler forbids while Zone C printed a
		// different one for the same unit.
		//
		// ⚠️ This assertion is deliberately NOT applied to PlaceSynonyms: place
		// symbols are hand-authored in the snapshot's PlaceVocabulary table and
		// underscores are CORRECT there (`ancient_ground_near`). That asymmetry is
		// exactly what made the original defect look reasonable on the page.
		TestFalse(*FString::Printf(TEXT("Unit canonical \"%s\" has no underscore (it is a CardID lower-cased, not snake_case)"), *Canonical),
			Canonical.Contains(TEXT("_")));

		for (const FString& Alias : Row.Aliases)
		{
			const FString Lowered = Alias.ToLower();

			// ⚠️ SUBSTRING, not whole-string — deliberately STRICTER than §9b's
			// token prohibition (TASK-419 WARN-1). This table is prompt text read
			// by an LLM, not an exact-match lookup, so `fire mage` pulled toward
			// Wizard just as a bare `mage` would and threatened the scored rows
			// DEV-06 and HOLD-09. No unit may claim any of these words as part of
			// an alias; the [notes] block routes them to a which_unit question.
			TestFalse(*FString::Printf(TEXT("Unit \"%s\" does not embed an ambiguous caster word in alias \"%s\""), *Canonical, *Lowered),
				Lowered.Contains(TEXT("mage")) || Lowered.Contains(TEXT("caster")) || Lowered.Contains(TEXT("spellcaster")));

			if (bIsCaster)
			{
				// The genuinely ambiguous words belong to NEITHER card — the notes
				// block routes them to a which_unit question instead of guessing.
				TestFalse(*FString::Printf(TEXT("%s does not claim the ambiguous alias \"%s\""), *Canonical, *Lowered),
					Lowered == TEXT("mage") || Lowered == TEXT("caster") || Lowered == TEXT("spellcaster") || Lowered == TEXT("magic user"));
			}

			if (Canonical == TEXT("wizard"))
			{
				TestFalse(TEXT("wizard never aliases the word sorcerer"), Lowered.Contains(TEXT("sorcerer")));
			}

			if (Canonical == TEXT("sorcerer"))
			{
				TestFalse(TEXT("sorcerer never aliases the word wizard"), Lowered.Contains(TEXT("wizard")));
			}
		}
	}

	// The same reasoning for the verb that sits exactly on the executor seam.
	for (const FSiegeAssistantSynonym& Row : Vocabulary->IntentSynonyms)
	{
		const FString Canonical = Row.Canonical.ToString().ToLower();
		for (const FString& Alias : Row.Aliases)
		{
			TestFalse(*FString::Printf(TEXT("%s does not claim the ambiguous verb \"defend\""), *Canonical),
				Alias.ToLower() == TEXT("defend"));
		}
	}

	// Every intent row must name a real intent, or the prompt would teach the
	// model a verb the grammar cannot express.
	for (const FSiegeAssistantSynonym& Row : Vocabulary->IntentSynonyms)
	{
		ESiegeAssistantIntent Intent = ESiegeAssistantIntent::None;
		TestTrue(*FString::Printf(TEXT("Intent row \"%s\" names a real intent"), *Row.Canonical.ToString()),
			SiegeAssistantIntentFromSymbol(Row.Canonical.ToString(), Intent));
	}

	// The disambiguation notes are LAW and are authored in C++ so they survive an
	// artist re-authoring the DataAsset.
	TestTrue(TEXT("The notes block ships"), First.Contains(TEXT("[notes]")));
	TestTrue(TEXT("The notes disambiguate wizard from sorcerer"), First.Contains(TEXT("wizard != sorcerer")));
	TestTrue(TEXT("The notes route the ambiguous verb to a question"), First.Contains(TEXT("which_intent")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
