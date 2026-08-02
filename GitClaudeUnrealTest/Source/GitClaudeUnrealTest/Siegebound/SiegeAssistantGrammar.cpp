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

	/** Emits one complete rule line: `Name ::= <Rhs>` plus its newline. */
	void AppendRule(FString& Grammar, const TCHAR* RuleName, const FString& Rhs)
	{
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

FString USiegeAssistantGrammar::Build(const TArray<FName>& UnitKinds, const TArray<FName>& PlaceNames)
{
	TArray<FString> Kinds;
	CanonicalizeSymbols(UnitKinds, Kinds);

	TArray<FString> Places;
	CanonicalizeSymbols(PlaceNames, Places);

	// With nothing nameable the selection machinery is unreachable, so it is not
	// emitted at all — see the degenerate-input note on Build's declaration.
	const bool bHasKinds = Kinds.Num() > 0;

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

		// --- at_least ---------------------------------------------------------
		// The deferred-intent threshold is the same numeric range as `count` but
		// WITHOUT "all": "wait until I have all footmen" is not a condition that
		// can ever become true, so it is made unsayable rather than left for the
		// parser to reject. Same range, generated from the same constants.
		{
			TArray<FString> Alternatives;
			Alternatives.Reserve(GrammarCountMax);
			for (int32 Quantity = GrammarCountMin; Quantity <= GrammarCountMax; ++Quantity)
			{
				Alternatives.Add(GbnfTerminal(FString::FromInt(Quantity)));
			}

			AppendRule(Grammar, TEXT("at_least"), JoinAlternatives(Alternatives));
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
	}

	// --- who ------------------------------------------------------------------
	// "all" selects every eligible unit; "none" is for the army-wide verbs, which
	// carry no selection at all.
	{
		TArray<FString> Alternatives;
		if (bHasKinds)
		{
			Alternatives.Add(TEXT("selection"));
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
			TArray<FString> Parts;
			Parts.Add(JsonObjectOpen(SiegeAssistantJsonKeys::Kind));
			Parts.Add(TEXT("kind"));
			Parts.Add(JsonNextKey(SiegeAssistantJsonKeys::AtLeast));
			Parts.Add(TEXT("at_least"));
			Parts.Add(GbnfTerminal(TEXT("}")));

			Alternatives.Add(FString::Join(Parts, TEXT(" ")));
		}

		AppendRule(Grammar, TEXT("when"), JoinAlternatives(Alternatives));
	}

	return Grammar;
}
