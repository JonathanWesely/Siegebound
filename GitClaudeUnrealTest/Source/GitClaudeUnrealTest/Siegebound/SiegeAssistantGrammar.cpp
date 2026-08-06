// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeAssistantGrammar.h"

#include "SiegeAssistantCommand.h"

namespace
{
	/**
	 *  Escapes a run of characters for use as a GBNF terminal and wraps it in
	 *  the terminal's own double quotes. The payload is the LITERAL CHARACTERS
	 *  the model must produce, so a JSON double quote inside it arrives here as
	 *  a bare '"' and leaves as '\"'.
	 */
	FString GbnfTerminal(const FString& Chars)
	{
		FString Escaped;
		Escaped.Reserve(Chars.Len() + 8);
		for (const TCHAR Char : Chars)
		{
			if (Char == TEXT('\\') || Char == TEXT('"'))
			{
				Escaped.AppendChar(TEXT('\\'));
			}
			Escaped.AppendChar(Char);
		}

		return FString(TEXT("\"")) + Escaped + TEXT("\"");
	}

	/**
	 *  A GBNF terminal that produces a JSON STRING. Two escaping layers, applied
	 *  in the right order: the value is escaped for JSON and wrapped in JSON
	 *  quotes, and the result is then escaped for GBNF by GbnfTerminal. For a
	 *  canonical lower snake_case symbol neither layer changes anything, but the
	 *  nesting has to be correct for the defensive cases.
	 */
	FString GbnfJsonString(const FString& Value)
	{
		FString JsonEscaped;
		JsonEscaped.Reserve(Value.Len() + 8);
		for (const TCHAR Char : Value)
		{
			if (Char == TEXT('\\') || Char == TEXT('"'))
			{
				JsonEscaped.AppendChar(TEXT('\\'));
			}
			JsonEscaped.AppendChar(Char);
		}

		return GbnfTerminal(FString(TEXT("\"")) + JsonEscaped + TEXT("\""));
	}

	/** The opening fragment of a JSON object whose first key is Key, e.g. {"intent": */
	FString JsonObjectOpen(const TCHAR* Key)
	{
		return GbnfTerminal(FString(TEXT("{\"")) + Key + TEXT("\":"));
	}

	/** The separator before a subsequent key, e.g. ,"who": */
	FString JsonNextKey(const TCHAR* Key)
	{
		return GbnfTerminal(FString(TEXT(",\"")) + Key + TEXT("\":"));
	}

	/** Joins alternatives with GBNF's ' | '. */
	FString JoinAlternatives(const TArray<FString>& Alternatives)
	{
		return FString::Join(Alternatives, TEXT(" | "));
	}

	/**
	 *  ⛔ THE GBNF RULE-NAME CHARSET — AND WHY IT IS CHECKED RATHER THAN TRUSTED.
	 *
	 *  llama.cpp reads a rule name as a run of [a-zA-Z0-9-] and STOPS at the
	 *  first character outside it. `at_least ::= "1" | ...` therefore parses as
	 *  the name `at`, after which the parser demands `::=`, finds `_least`, and
	 *  REJECTS THE ENTIRE GRAMMAR:
	 *
	 *      parse: error parsing grammar: expecting ::= at _least ::= "1" | "2" ...
	 *
	 *  ⚠️ THAT SHIPPED. TASK-413's spike measured the consequence:
	 *  llama_sampler_init_grammar returned NULL on every generation of all six
	 *  bench runs, and with nothing constraining it the model emitted `<think>`
	 *  prose instead of JSON on every iteration — a total loss of the mechanism
	 *  the whole feature rests on, in the file CONVENTIONS §9c names as THE
	 *  AUTHORITY.
	 *
	 *  ⚠️ IT ALSO SURVIVED TWO CAREFUL REVIEWS, AND THAT IS THE REASON THIS
	 *  FUNCTION EXISTS. Both diffed this generator against the spike's mirror
	 *  rule-for-rule and correctly reported that they matched. They did match.
	 *  THEY WERE IDENTICALLY UNPARSEABLE.
	 *
	 *  > STRING-COMPARING TWO GENERATORS CAN NEVER PROVE EITHER ONE IS VALID.
	 *  > ONLY THE TARGET PARSER CAN. A dump is not a parse. This check is the
	 *  > cheap stand-in for the parser, applied at the one point where the answer
	 *  > is still a named identifier in a log line instead of a silent capability
	 *  > loss at runtime.
	 *
	 *  ⚠️ DO NOT CONFUSE THIS CHARSET WITH THE SYMBOL CHARSET IN
	 *  CanonicalizeSymbols ABOVE. A canonical SYMBOL ("ancient_ground_near") and
	 *  a JSON KEY ("at_least") are lower snake_case and MUST keep their
	 *  underscores — they are wire format, and the output schema, the sealed
	 *  evaluation corpus and ParseSiegeAssistantCommand all assert them. Only the
	 *  GBNF RULE NAME is kebab-case. The two sit one token apart inside `when`:
	 *
	 *      when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
	 *                                              ^ JSON key      ^ rule name
	 */
	bool IsLegalGbnfRuleName(const FString& Identifier)
	{
		if (Identifier.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Char : Identifier)
		{
			const bool bLegal =
				(Char >= TEXT('a') && Char <= TEXT('z')) ||
				(Char >= TEXT('A') && Char <= TEXT('Z')) ||
				(Char >= TEXT('0') && Char <= TEXT('9')) ||
				Char == TEXT('-');

			if (!bLegal)
			{
				return false;
			}
		}

		return true;
	}

	/**
	 *  Collects the rule REFERENCES from a right-hand side — every identifier
	 *  that sits OUTSIDE a terminal literal.
	 *
	 *  ⚠️ THE DEFECT HAD TWO HALVES. `at_least` appeared once as a definition and
	 *  once as a reference, and validating only definitions would have left the
	 *  reference to rot — llama.cpp rejects the reference for exactly the same
	 *  reason it rejects the definition. Both halves are checked.
	 *
	 *  Terminals are SKIPPED AS WHOLE UNITS, honouring GBNF's backslash escape,
	 *  rather than the right-hand side being split on whitespace. A generated
	 *  symbol is PERMITTED to contain a space — CanonicalizeSymbols warns about
	 *  it but still emits it, correctly escaped, because silently dropping a live
	 *  unit kind would be worse — and a whitespace split would tear `"foo bar"`
	 *  in half and then report the fragment as an illegal rule name. A validator
	 *  that fires on legal input is worse than no validator, because it teaches
	 *  people to ignore it.
	 */
	void CollectRuleReferences(const FString& Rhs, TArray<FString>& OutReferences)
	{
		FString Identifier;
		bool bInTerminal = false;
		bool bEscaped = false;

		for (const TCHAR Char : Rhs)
		{
			if (bInTerminal)
			{
				if (bEscaped)
				{
					bEscaped = false;
				}
				else if (Char == TEXT('\\'))
				{
					bEscaped = true;
				}
				else if (Char == TEXT('"'))
				{
					bInTerminal = false;
				}

				continue;
			}

			if (Char == TEXT('"'))
			{
				bInTerminal = true;
				continue;
			}

			// Outside a terminal this generator emits only identifiers, ' | ' and
			// spaces. Anything that is not a separator is therefore part of an
			// identifier — INCLUDING an illegal character, which is precisely what
			// must be captured rather than skipped past.
			const bool bIsSeparator = (Char == TEXT(' ')) || (Char == TEXT('\t')) || (Char == TEXT('|'));
			if (bIsSeparator)
			{
				if (!Identifier.IsEmpty())
				{
					OutReferences.Add(Identifier);
					Identifier.Reset();
				}

				continue;
			}

			Identifier.AppendChar(Char);
		}

		if (!Identifier.IsEmpty())
		{
			OutReferences.Add(Identifier);
		}
	}

	/**
	 *  Emits one complete rule line: `Name ::= <Rhs>` plus its newline.
	 *
	 *  ⚠️ EVERY RULE NAME AND EVERY RULE REFERENCE IN THE GRAMMAR PASSES THROUGH
	 *  HERE, which is what makes this the single point where the charset can be
	 *  enforced for the whole file rather than for the one rule that happened to
	 *  fail first. The check is a linear scan of ~13 short identifiers, run once
	 *  per typed sentence, and Build is a pure function with no engine state — so
	 *  it is unmeasurable next to the inference call it precedes.
	 *
	 *  ⛔ WHERE THIS GUARD IS LIVE — READ OUT OF UE 5.8's HEADERS, NOT RECALLED.
	 *  This comment previously claimed the guard was "DELIBERATELY NOT COMPILED
	 *  OUT OF SHIPPING". THAT WAS FALSE. It was an assertion about a consuming
	 *  system (UBT/Core) written from memory and never checked — the identical
	 *  mistake, one system over, to the one this function exists to catch. What
	 *  the engine actually does, with the lines it does it on:
	 *
	 *    Build.h:200-203      USE_LOGGING_IN_SHIPPING defaults to 0
	 *    Build.h:350-352      Shipping ⇒ NO_LOGGING = !USE_LOGGING_IN_SHIPPING ⇒ 1
	 *    Build.h:322-324      Test     ⇒ the same
	 *    LogMacros.h:184-194  NO_LOGGING ⇒ UE_LOG "will only log Fatal errors";
	 *                         an Error call becomes an empty if constexpr(false)
	 *    Build.h:205-212      USE_CHECKS_IN_SHIPPING 0, and
	 *                         USE_ENSURES_IN_SHIPPING follows it ⇒ 0
	 *    Build.h:332-334      Shipping ⇒ DO_ENSURE = USE_ENSURES_IN_SHIPPING ⇒ 0
	 *    Build.h:304-306      Test     ⇒ the same
	 *    AssertionMacros.h:467-472
	 *                         DO_ENSURE 0 ⇒ ensureAlwaysMsgf degrades to
	 *                         (LIKELY(!!(expr))) — the condition is still
	 *                         evaluated, but nothing is reported
	 *
	 *  GitClaudeUnrealTest.Target.cs sets NO bUseLoggingInShipping and NO
	 *  bUseChecksInShipping override, so BOTH HALVES OF THIS GUARD COMPILE OUT
	 *  IN SHIPPING AND IN TEST. It is live in Debug, DebugGame and Development,
	 *  editor variants included (Build.h:241-296 gives both UE_BUILD_DEBUG and
	 *  UE_BUILD_DEVELOPMENT NO_LOGGING 0 and DO_ENSURE 1).
	 *
	 *  ✅ AND THAT IS THE CORRECT SCOPE, not a hole being confessed. This is a
	 *  DEVELOPMENT-TIME AND CI check by nature rather than by accident, because
	 *  the property it checks is settled before the program runs:
	 *
	 *   1. NO RULE NAME IS EVER BUILT FROM DATA. Every one of the 13 names in
	 *      Build is a compile-time TEXT("…") literal, and every data-derived
	 *      string — unit kinds, place symbols, intents, ask codes, counts —
	 *      reaches the grammar through GbnfJsonString / GbnfTerminal, i.e.
	 *      INSIDE a quoted terminal, never in identifier position. A live unit
	 *      called `militia_mob` cannot produce an illegal rule name. THIS GUARD
	 *      CANNOT FIRE ON LIVE DATA — only on source.
	 *   2. ⇒ A Shipping build therefore contains exactly the identifiers a
	 *      Development build contained. There is no input that breaks it in a
	 *      player's hands and not on a developer's machine, and that is the
	 *      only thing that would have made a dev-only guard the "silent failure
	 *      wearing a different hat" the old comment was worried about.
	 *   3. WHAT PROTECTS A SHIPPED BUILD IS THE AUTOMATION TEST, NOT THIS LOG:
	 *      `Siegebound.Assistant.Grammar.RuleNameCharset`, in
	 *      Siegebound/Tests/SiegeAssistantGrammarTest.cpp. It runs Build over
	 *      all four builder shapes and asserts this same charset in CI, before
	 *      anything is packaged. THAT is the gate. This function is the fast
	 *      local echo of it that names the offending identifier in a log line
	 *      while you are still at the keyboard.
	 *
	 *  ⚠️ DO NOT "MAKE THE OLD COMMENT TRUE" BY SETTING bUseLoggingInShipping /
	 *  bUseChecksInShipping. Turning Shipping logging on project-wide carries
	 *  performance and log-volume consequences far outside this file, and it is
	 *  not this file's call to make (QA TASK-416 BLOCKER-1; recorded there as an
	 *  option for TASK-423).
	 *
	 *  ⚠️ THE OFFENDING RULE IS STILL EMITTED, NOT DROPPED. Swallowing it would
	 *  leave a dangling reference and a grammar that fails to parse for a second,
	 *  less obvious reason, and the `Siege.Llama.SpikeGrammar` dump would no
	 *  longer show what the code actually intended. llama.cpp will refuse this
	 *  grammar either way; the job here is to make the refusal LOUD and to NAME
	 *  the identifier that caused it, which is the one thing the six bench runs
	 *  could not do for themselves.
	 *
	 *  In the configurations where the guard is live, the two channels do two
	 *  different jobs: the Error log NAMES the offending identifier in the run
	 *  log, and the ensure adds a callstack and an automation-visible error so an
	 *  editor or CI run FAILS on it rather than merely mentioning it. Neither
	 *  survives Shipping or Test — see the table above for why that is the right
	 *  scope for this check and where the shipped build's actual protection is.
	 */
	void AppendRule(FString& Grammar, const TCHAR* RuleName, const FString& Rhs)
	{
		const bool bLegalRuleName = IsLegalGbnfRuleName(RuleName);
		if (!bLegalRuleName)
		{
			UE_LOG(LogSiegeAssistant, Error,
				TEXT("USiegeAssistantGrammar::Build: ILLEGAL GBNF RULE NAME '%s'. llama.cpp rule names are [a-zA-Z0-9-] only, so the WHOLE grammar will fail to parse and generation will run UNCONSTRAINED. Rename the RULE to kebab-case; do NOT rename the JSON key it carries."),
				RuleName);
		}
		ensureAlwaysMsgf(bLegalRuleName,
			TEXT("Illegal GBNF rule name '%s' — the grammar will not parse. See LogSiegeAssistant."), RuleName);

		TArray<FString> References;
		CollectRuleReferences(Rhs, References);

		for (const FString& Reference : References)
		{
			const bool bLegalReference = IsLegalGbnfRuleName(Reference);
			if (!bLegalReference)
			{
				UE_LOG(LogSiegeAssistant, Error,
					TEXT("USiegeAssistantGrammar::Build: rule '%s' REFERENCES the illegal identifier '%s'. llama.cpp rule names are [a-zA-Z0-9-] only, so the WHOLE grammar will fail to parse and generation will run UNCONSTRAINED."),
					RuleName, *Reference);
			}
			ensureAlwaysMsgf(bLegalReference,
				TEXT("Illegal GBNF rule reference '%s' in rule '%s' — the grammar will not parse. See LogSiegeAssistant."),
				*Reference, RuleName);
		}

		Grammar += RuleName;
		Grammar += TEXT(" ::= ");
		Grammar += Rhs;
		Grammar += TEXT("\n");
	}

	/**
	 *  Turns the caller's FNames into the canonical symbol strings the grammar
	 *  will quote.
	 *
	 *  CALLER ORDER IS PRESERVED — deliberately no sort. FName's operator< orders
	 *  by comparison index, which depends on the order names were first
	 *  registered in the process and is therefore NOT stable across runs; sorting
	 *  by it would silently break the determinism guarantee that the whole test
	 *  suite rests on. A caller who wants a particular ordering supplies it.
	 *
	 *  Lower-casing is likewise load-bearing rather than cosmetic: FName reports
	 *  the case it was first constructed with, so the same live game state could
	 *  otherwise yield "Footman" in one process and "footman" in another. It also
	 *  makes the canonical-symbol convention true by construction instead of by
	 *  agreement, and it costs nothing on the parse side because FName comparison
	 *  is case-insensitive.
	 */
	void CanonicalizeSymbols(const TArray<FName>& In, TArray<FString>& Out)
	{
		Out.Reset();
		Out.Reserve(In.Num());

		for (const FName& Symbol : In)
		{
			if (Symbol.IsNone())
			{
				continue;
			}

			const FString Canonical = Symbol.ToString().ToLower();
			if (Canonical.IsEmpty())
			{
				continue;
			}

			// The reserved sentinels are structural, not names of game objects. A
			// unit or place actually called "all" or "none" would collide with the
			// selector/absence markers, so it is dropped rather than allowed to
			// make the grammar ambiguous.
			//
			// Logged at Verbose, not Warning: this is an input class the function
			// is DESIGNED to filter, so dropping it is correct behaviour rather
			// than an anomaly worth interrupting anyone over. (The odd-character
			// case below is the opposite — that one means something upstream is
			// producing non-canonical symbols, and it warns.)
			if (Canonical == SiegeAssistantSymbols::All || Canonical == SiegeAssistantSymbols::None)
			{
				UE_LOG(LogSiegeAssistant, Verbose,
					TEXT("USiegeAssistantGrammar::Build: dropping reserved symbol '%s' — 'all' and 'none' are structural sentinels and may not name a unit or a place."),
					*Canonical);
				continue;
			}

			bool bHasUnexpectedChar = false;
			for (const TCHAR Char : Canonical)
			{
				const bool bIsSafe =
					(Char >= TEXT('a') && Char <= TEXT('z')) ||
					(Char >= TEXT('0') && Char <= TEXT('9')) ||
					Char == TEXT('_');

				if (!bIsSafe)
				{
					bHasUnexpectedChar = true;
					break;
				}
			}

			if (bHasUnexpectedChar)
			{
				// Emitted anyway, correctly escaped — silently dropping a live unit
				// kind would be far worse than an odd-looking alternative. The warning
				// exists so a snapshot that starts producing non-canonical symbols is
				// noticed rather than tolerated.
				UE_LOG(LogSiegeAssistant, Warning,
					TEXT("USiegeAssistantGrammar::Build: symbol '%s' contains characters outside [a-z0-9_]; emitting it escaped. Canonical symbols should be lower snake_case."),
					*Canonical);
			}

			Out.AddUnique(Canonical);
		}
	}
}

FString USiegeAssistantGrammar::Build(const TArray<FName>& UnitKinds, const TArray<FName>& PlaceNames,
	const TArray<FName>& RegionPlaceNames)
{
	TArray<FString> Kinds;
	CanonicalizeSymbols(UnitKinds, Kinds);

	TArray<FString> Places;
	CanonicalizeSymbols(PlaceNames, Places);

	// The REGION-BEARING subset — the places a shipped IsPointInZone can answer
	// for. It arrives already decided (AS-§21.4 gives the snapshot's
	// PlaceVocabulary the single `bHasRegion` column); this builder never judges
	// which places are areas and must not start, because the answer is geometry
	// this pure function has no access to.
	//
	// ⚠️ IT GOES THROUGH THE SAME CanonicalizeSymbols AS THE OTHER TWO, which is
	// what makes "all"/"none"/NAME_None unsamplable inside `in` rather than merely
	// rejected downstream — AS-§21.5's BadRegion class, made unreachable at the
	// sampler instead of caught at the parser.
	TArray<FString> Regions;
	CanonicalizeSymbols(RegionPlaceNames, Regions);

	// With nothing nameable the selection machinery is unreachable, so it is not
	// emitted at all — see the degenerate-input note on Build's declaration.
	const bool bHasKinds = Kinds.Num() > 0;

	// ⛔ THE COUNT THAT GATES `inplace` / `zone` IS THE POST-FILTER ONE, NOT
	// RegionPlaceNames.Num(). A caller that passed only reserved or empty symbols
	// hands us a non-empty array that canonicalizes to NOTHING, and gating on the
	// raw parameter would then emit `zone ::= ` with an EMPTY right-hand side —
	// the whole grammar rejected, generation UNCONSTRAINED, which is the exact
	// `at_least` disaster class this file exists to refuse (TASK-413, two of six
	// bars). Gate on what will actually be written.
	const bool bHasRegions = Regions.Num() > 0;

	FString Grammar;
	Grammar.Reserve(2048);

	// --- root -----------------------------------------------------------------
	// A `question` branch is always reachable. Without it, a model handed an
	// utterance it cannot translate is FORCED by the grammar to invent a command,
	// because declining would not be expressible. Constrained decoding can only
	// make output well-formed, never correct, so "I cannot turn this into an
	// order" must always be sayable.
	AppendRule(Grammar, TEXT("root"), TEXT("command | question"));

	// --- command --------------------------------------------------------------
	// FIXED KEY ORDER, every key ALWAYS present. Nothing is optional: an omitted
	// key would be one more shape the parser has to reason about, and the model
	// spends fewer tokens emitting "none" than it would deciding whether to.
	{
		TArray<FString> Parts;
		Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::Intent));
		Parts.Add(TEXT("intent"));
		Parts.Add(JsonNextKey(SiegeAssistantJsonKeys::Who));
		Parts.Add(TEXT("who"));
		Parts.Add(JsonNextKey(SiegeAssistantJsonKeys::Where));
		Parts.Add(TEXT("where"));
		Parts.Add(JsonNextKey(SiegeAssistantJsonKeys::When));
		Parts.Add(TEXT("when"));
		Parts.Add(GbnfTerminal(TEXT("}")));

		AppendRule(Grammar, TEXT("command"), FString::Join(Parts, TEXT(" ")));
	}

	// --- question -------------------------------------------------------------
	{
		TArray<FString> Parts;
		Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::Ask));
		Parts.Add(TEXT("ask"));
		Parts.Add(GbnfTerminal(TEXT("}")));

		AppendRule(Grammar, TEXT("question"), FString::Join(Parts, TEXT(" ")));
	}

	// --- ask ------------------------------------------------------------------
	{
		TArray<FString> AskCodes;
		SiegeAssistantAskCodes(AskCodes);

		TArray<FString> Alternatives;
		Alternatives.Reserve(AskCodes.Num());
		for (const FString& Code : AskCodes)
		{
			Alternatives.Add(GbnfJsonString(Code));
		}

		AppendRule(Grammar, TEXT("ask"), JoinAlternatives(Alternatives));
	}

	// --- intent ---------------------------------------------------------------
	// Derived from ESiegeAssistantIntent by reflection so the emitted vocabulary
	// and the vocabulary ParseSiegeAssistantCommand accepts cannot drift. Nobody
	// hand-writes these seven strings a second time.
	{
		TArray<FString> IntentSymbols;
		SiegeAssistantIntentSymbols(IntentSymbols);

		TArray<FString> Alternatives;
		Alternatives.Reserve(IntentSymbols.Num());
		for (const FString& Symbol : IntentSymbols)
		{
			Alternatives.Add(GbnfJsonString(Symbol));
		}

		AppendRule(Grammar, TEXT("intent"), JoinAlternatives(Alternatives));
	}

	// --- kind (GENERATED) -----------------------------------------------------
	// THE GROUNDING. Every alternative here is a unit that actually exists right
	// now; anything else is unreachable for the sampler.
	if (bHasKinds)
	{
		TArray<FString> Alternatives;
		Alternatives.Reserve(Kinds.Num());
		for (const FString& Kind : Kinds)
		{
			Alternatives.Add(GbnfJsonString(Kind));
		}

		AppendRule(Grammar, TEXT("kind"), JoinAlternatives(Alternatives));
	}

	// --- where (GENERATED) ----------------------------------------------------
	// "none" is ALWAYS an alternative: the army-wide verbs (charge, fallback,
	// rally) have no destination, and forcing the model to name one would invite
	// exactly the confident-wrong answer this design is built to avoid. It is
	// also what keeps the rule well-formed when there are no places at all.
	{
		TArray<FString> Alternatives;
		Alternatives.Reserve(Places.Num() + 1);
		for (const FString& Place : Places)
		{
			Alternatives.Add(GbnfJsonString(Place));
		}
		Alternatives.Add(GbnfJsonString(SiegeAssistantSymbols::None));

		AppendRule(Grammar, TEXT("where"), JoinAlternatives(Alternatives));
	}

	if (bHasKinds)
	{
		// --- count ------------------------------------------------------------
		// ⚠️ 1..GrammarCountMax, NOT 1..live-max. See GrammarCountMax's comment —
		// capping this at the live roster is the defect, not the fix.
		{
			TArray<FString> Alternatives;
			Alternatives.Reserve(GrammarCountMax + 1);
			for (int32 Quantity = GrammarCountMin; Quantity <= GrammarCountMax; ++Quantity)
			{
				Alternatives.Add(GbnfTerminal(FString::FromInt(Quantity)));
			}
			Alternatives.Add(GbnfJsonString(SiegeAssistantSymbols::All));

			AppendRule(Grammar, TEXT("count"), JoinAlternatives(Alternatives));
		}

		// --- at-least ---------------------------------------------------------
		// The deferred-intent threshold is the same numeric range as `count` but
		// WITHOUT "all": "wait until I have all footmen" is not a condition that
		// can ever become true, so it is made unsayable rather than left for the
		// parser to reject. Same range, generated from the same constants.
		//
		// ⛔ THE RULE IS `at-least`, KEBAB-CASE. THE JSON KEY IT CARRIES IS
		//    `at_least`, SNAKE_CASE. THEY ARE NOT THE SAME STRING AND NEITHER MAY
		//    BE "MADE CONSISTENT" WITH THE OTHER.
		//    - `at_least` as a RULE name does not parse: llama.cpp reads the name
		//      as `at`, then demands `::=` and finds `_least`, and REJECTS THE
		//      WHOLE GRAMMAR. That shipped and cost TASK-413 two of its six bars.
		//    - `at-least` as a JSON KEY breaks the wire format: the output schema
		//      in Zone A, the sealed evaluation corpus (TASK-426) and
		//      ParseSiegeAssistantCommand's SiegeAssistantJsonKeys::AtLeast all
		//      assert `at_least`, and the executor reads it.
		//    See IsLegalGbnfRuleName above; AppendRule now refuses to let the
		//    first half of this recur silently.
		{
			TArray<FString> Alternatives;
			Alternatives.Reserve(GrammarCountMax);
			for (int32 Quantity = GrammarCountMin; Quantity <= GrammarCountMax; ++Quantity)
			{
				Alternatives.Add(GbnfTerminal(FString::FromInt(Quantity)));
			}

			AppendRule(Grammar, TEXT("at-least"), JoinAlternatives(Alternatives));
		}

		// --- item -------------------------------------------------------------
		FString ItemRule;
		{
			TArray<FString> Parts;
			Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::Kind));
			Parts.Add(TEXT("kind"));
			Parts.Add(JsonNextKey(SiegeAssistantJsonKeys::N));
			Parts.Add(TEXT("count"));
			Parts.Add(GbnfTerminal(TEXT("}")));

			ItemRule = FString::Join(Parts, TEXT(" "));
		}
		AppendRule(Grammar, TEXT("item"), ItemRule);

		// --- selection --------------------------------------------------------
		// ⚠️ THE CAP LIVES HERE, IN THE GRAMMAR — not only in the parser (manager
		// ruling 15). This is written as a BOUNDED ALTERNATION of exactly 1, 2 and
		// 3 pairs rather than a repetition rule, because an unbounded repetition
		// is precisely what a small model rambles into, and because a bounded
		// alternation makes "a 4-kind selection is unreachable" visible by reading
		// the grammar instead of by trusting a downstream check. The alternation
		// is generated from SiegeAssistantMaxSelectionKinds, so widening the cap
		// widens the grammar automatically.
		{
			TArray<FString> Alternatives;
			Alternatives.Reserve(SiegeAssistantMaxSelectionKinds);

			for (int32 ItemCount = 1; ItemCount <= SiegeAssistantMaxSelectionKinds; ++ItemCount)
			{
				TArray<FString> Parts;
				Parts.Add(GbnfTerminal(TEXT("[")));
				for (int32 Index = 0; Index < ItemCount; ++Index)
				{
					if (Index > 0)
					{
						Parts.Add(GbnfTerminal(TEXT(",")));
					}
					Parts.Add(TEXT("item"));
				}
				Parts.Add(GbnfTerminal(TEXT("]")));

				Alternatives.Add(FString::Join(Parts, TEXT(" ")));
			}

			AppendRule(Grammar, TEXT("selection"), JoinAlternatives(Alternatives));
		}

		// --- exceptlist -------------------------------------------------------
		// The EXCLUSION list: 1, 2 or 3 BARE kind strings. Built by exactly the
		// same construction as `selection` directly above — a bounded alternation
		// generated from the cap constant, never a repetition operator — so
		// widening SiegeAssistantMaxExclusionKinds widens the grammar
		// automatically and a 4-kind exclusion stays unreachable for the sampler.
		//
		// ⛔⭐ BARE KIND STRINGS, NOT `item`. THIS IS THE DECLINED FEATURE BEING
		// MADE INEXPRESSIBLE RATHER THAN MERELY UNIMPLEMENTED. Referencing `item`
		// here would cost nothing to write and would silently re-introduce the
		// count-controlled variant Jonathan DECLINED — "all except 5 archers" —
		// because {"kind":…,"n":…} would then be a shape the sampler could reach.
		// A feature that a shape cannot express cannot be brought back by a prompt
		// tweak, a tired author, or a model that guessed. Do not "unify" these two
		// rules.
		//
		// ⛔ THE RULE NAME IS `exceptlist`, ONE WORD, AND THAT IS A DECLARED
		// DEPARTURE (SC-§15) FROM THE `except_list` SPELLED IN CONVENTIONS
		// AS-§20.1. llama.cpp reads a rule name as [a-zA-Z0-9-] and STOPS at the
		// underscore, so `except_list` would parse as the name `except`, and the
		// WHOLE grammar would then be rejected and generation would run
		// UNCONSTRAINED — the identical defect `at_least` shipped with, which cost
		// TASK-413 two of its six bars. AS-§20.1 anticipated this and named
		// `exceptlist` as the substitute to use, so this is the sanctioned name
		// rather than a third one invented here. The JSON KEY it carries keeps its
		// underscore (`all_except`) because that is wire format.
		{
			TArray<FString> Alternatives;
			Alternatives.Reserve(SiegeAssistantMaxExclusionKinds);

			for (int32 KindCount = 1; KindCount <= SiegeAssistantMaxExclusionKinds; ++KindCount)
			{
				TArray<FString> Parts;
				Parts.Add(GbnfTerminal(TEXT("[")));
				for (int32 Index = 0; Index < KindCount; ++Index)
				{
					if (Index > 0)
					{
						Parts.Add(GbnfTerminal(TEXT(",")));
					}
					Parts.Add(TEXT("kind"));
				}
				Parts.Add(GbnfTerminal(TEXT("]")));

				Alternatives.Add(FString::Join(Parts, TEXT(" ")));
			}

			AppendRule(Grammar, TEXT("exceptlist"), JoinAlternatives(Alternatives));
		}

		// --- except -----------------------------------------------------------
		// {"all_except":["miner"]} — a `who` VALUE, and deliberately NOT a fourth
		// top-level key. The parser validates an EXACT top-level key set and the
		// prompt law emits every key always, so a new top-level key would have had
		// to appear in every emission at once. As a `who` shape this is strictly
		// additive: every grammar path that existed before still exists unchanged.
		//
		// ⚠️ THE UNDERSCORE IN `all_except` IS INSIDE A TERMINAL, WHICH IS WHY IT IS
		// SAFE. CollectRuleReferences skips terminals as whole units, so the JSON key
		// is never seen in identifier position — exactly the `at_least` / `at-least`
		// split, one rule over.
		{
			TArray<FString> Parts;
			Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::AllExcept));
			Parts.Add(TEXT("exceptlist"));
			Parts.Add(GbnfTerminal(TEXT("}")));

			AppendRule(Grammar, TEXT("except"), FString::Join(Parts, TEXT(" ")));
		}
	}

	// --- inplace / zone (GENERATED) -------------------------------------------
	// {"in":"ancient_ground_near"} — the FIFTH `who` shape: the units STANDING in
	// a place, as opposed to `where`, which is the place they are SENT to. Like
	// `except` it is a `who` VALUE and deliberately NOT a fourth top-level key —
	// the parser validates an EXACT top-level key set and the prompt law emits
	// every key always, so a new top-level key would have had to appear in EVERY
	// emission at once (AS-§21.5, on AS-§20.1's identical argument). As a `who`
	// shape this is strictly additive at the wire: every JSON that parsed before
	// still parses, byte-for-byte.
	//
	// ⛔ THE GATE IS THE REGION LIST, ⛔ NOT bHasKinds — see the `who` rule below
	// for why the asymmetry with `except` is correct rather than an oversight.
	//
	// ⛔ AND NOTHING IS EMITTED WHEN THERE ARE NO REGIONS. An empty alternation
	// would leave `zone` DEFINED-AS-NOTHING and `inplace` referencing it, and
	// llama.cpp answers a grammar it cannot parse by generating UNCONSTRAINED —
	// i.e. the failure is a total loss of the mechanism, not a missing feature.
	// With an empty region list the emitted grammar is byte-identical to the one
	// this builder produced before the shape existed.
	//
	// ⛔ RULE NAMES ARE ONE WORD EACH: `inplace`, `zone`. THIS IS THE SAME
	// DECLARED DEPARTURE (SC-§15) AS `exceptlist` ABOVE, FOR THE SAME REASON —
	// llama.cpp reads a rule name as [a-zA-Z0-9-] and STOPS at an underscore, so
	// `in_place` would parse as the name `in`, and the whole grammar would be
	// rejected. ⛔ Do not "improve" them to `in_place` / `zone_list`. ✅ The JSON
	// KEY needs no departure at all this time: it is `in`, which has no underscore
	// to lose — so unlike `at_least`/`at-least` and `all_except`/`exceptlist`,
	// the key and the rule name coincide here by luck, not by unification. Do not
	// read that coincidence as permission to unify the other two.
	//
	// `inplace` REFERENCES `zone` ONE LINE BEFORE `zone` IS DEFINED, which is
	// legal and already the file's practice: `root ::= command | question` is the
	// first line emitted and both of its references are defined further down.
	// Ordered this way to match the AS-§21.5 rule block character-for-character.
	if (bHasRegions)
	{
		{
			TArray<FString> Parts;
			Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::In));
			Parts.Add(TEXT("zone"));
			Parts.Add(GbnfTerminal(TEXT("}")));

			AppendRule(Grammar, TEXT("inplace"), FString::Join(Parts, TEXT(" ")));
		}

		// ⛔ NO "none" ALTERNATIVE HERE, AND THE CONTRAST WITH `where` IS THE
		// POINT. `where` carries "none" because it is a key that is ALWAYS
		// emitted and the army-wide verbs have no destination. `zone` is reachable
		// only from INSIDE `inplace`, which the model chooses to enter — "no
		// region" is already expressible as any of the other four `who` shapes, so
		// a "none" here would be a second spelling of the same thing and
		// {"in":"none"} is AS-§21.5's BadRegion. Absence is said by not entering.
		{
			TArray<FString> Alternatives;
			Alternatives.Reserve(Regions.Num());
			for (const FString& Region : Regions)
			{
				Alternatives.Add(GbnfJsonString(Region));
			}

			AppendRule(Grammar, TEXT("zone"), JoinAlternatives(Alternatives));
		}
	}

	// --- who ------------------------------------------------------------------
	// FIVE SHAPES: a bounded positive `selection`; an `except` exclusion ("everyone
	// but the miners"); an `inplace` region ("everyone in the mid"); "all", which
	// selects every eligible unit; and "none", for the army-wide verbs, which carry
	// no selection at all.
	//
	// ⛔ THE ALTERNATION ORDER IS PINNED (AS-§21.5) AND IT IS MIRRORED IN PROSE BY
	// ZONE A's `WHO =` LINE (SiegeAssistantSnapshot.cpp), WHICH A TEST ASSERTS
	// AGREES WITH IT: selection | except | inplace | "all" | "none". `inplace` goes
	// THIRD, after `except` and BEFORE the two bare strings, so the three OBJECT/
	// ARRAY shapes stay grouped ahead of the two scalars. ⚠️ There is no
	// compile-time link between this line and Zone A's — this comment and the QA
	// gate are the whole tie, exactly as the mirror comment at
	// SiegeAssistantSnapshot.cpp:806-840 says.
	//
	// ⚠️ `except` IS EMITTED ONLY WHEN THE ROSTER HAS KINDS, on the same reasoning
	// that gates `selection`: an empty roster has nothing to EXCLUDE for exactly the
	// reason it has nothing to select, and emitting the alternative anyway would
	// leave `exceptlist` referencing an undefined `kind` rule and break the whole
	// grammar. With no kinds AND no regions `who` collapses to "all" | "none",
	// unchanged; with no kinds but live regions it is `inplace` | "all" | "none".
	//
	// ⚠️ AT A ONE-KIND ROSTER `exceptlist`'s 2- and 3-kind alternatives can only
	// produce the SAME symbol twice, and the parser refuses that with
	// DuplicateKind. That is degradation to REFUSABLE rather than to UNREACHABLE,
	// and it is deliberate: `selection` has had the identical property since it
	// shipped, and bounding this rule by the live kind COUNT instead of by the cap
	// constant would make the two rules disagree about their own construction for a
	// case the parser already answers.
	//
	// ⛔⚠️ AND THE ASYMMETRY ON THE VERY NEXT LINE IS DELIBERATE, NOT A MISSED
	// `bHasKinds`: `inplace` IS GATED ON REGIONS ONLY. "Everyone in the mid" names
	// NO unit kind — it is the region that supplies the selection — so gating it on
	// the roster would make the shape unreachable on a board with nothing spawned,
	// which is precisely a board where the player is most likely to be pointing at
	// ground rather than at units. `except` is gated on kinds because `exceptlist`
	// literally references the `kind` rule; `zone` references no kind at all
	// (AS-§21.4, and this is a stated QA criterion — do not "fix" it into symmetry).
	{
		TArray<FString> Alternatives;
		if (bHasKinds)
		{
			Alternatives.Add(TEXT("selection"));
			Alternatives.Add(TEXT("except"));
		}
		if (bHasRegions)
		{
			Alternatives.Add(TEXT("inplace"));
		}
		Alternatives.Add(GbnfJsonString(SiegeAssistantSymbols::All));
		Alternatives.Add(GbnfJsonString(SiegeAssistantSymbols::None));

		AppendRule(Grammar, TEXT("who"), JoinAlternatives(Alternatives));
	}

	// --- when -----------------------------------------------------------------
	// "now" or a latched deferred trigger ("wait until I have 2 more footmen,
	// then send"). The deferred branch needs a nameable kind, so with an empty
	// roster only "now" is emitted.
	{
		TArray<FString> Alternatives;
		Alternatives.Add(GbnfJsonString(SiegeAssistantSymbols::Now));

		if (bHasKinds)
		{
			// ⛔ TWO DIFFERENT STRINGS ON PURPOSE, ONE LINE APART: the JSON KEY is
			//    SiegeAssistantJsonKeys::AtLeast == "at_least" (wire format), the
			//    RULE REFERENCE is "at-least" (GBNF charset). See the `at-least`
			//    rule above.
			TArray<FString> Parts;
			Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::Kind));
			Parts.Add(TEXT("kind"));
			Parts.Add(JsonNextKey(SiegeAssistantJsonKeys::AtLeast));
			Parts.Add(TEXT("at-least"));
			Parts.Add(GbnfTerminal(TEXT("}")));

			Alternatives.Add(FString::Join(Parts, TEXT(" ")));
		}

		AppendRule(Grammar, TEXT("when"), JoinAlternatives(Alternatives));
	}

	return Grammar;
}
