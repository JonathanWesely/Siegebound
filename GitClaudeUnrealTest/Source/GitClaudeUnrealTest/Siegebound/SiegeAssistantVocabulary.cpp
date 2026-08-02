// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeAssistantVocabulary.h"

namespace
{
	/** Builds one synonym row. Kept local so the default table below reads as data. */
	FSiegeAssistantSynonym MakeSynonym(const TCHAR* Canonical, const TArray<FString>& Aliases)
	{
		FSiegeAssistantSynonym Synonym;
		Synonym.Canonical = FName(Canonical);
		Synonym.Aliases = Aliases;
		return Synonym;
	}

	/**
	 *  Normalises one row's aliases: trimmed, lower-cased, de-duped, sorted, and
	 *  with the canonical symbol itself removed (listing a symbol as its own
	 *  synonym spends tokens to say nothing).
	 */
	void NormaliseAliases(const FSiegeAssistantSynonym& Row, const FString& CanonicalSymbol, TArray<FString>& OutAliases)
	{
		OutAliases.Reset();
		OutAliases.Reserve(Row.Aliases.Num());

		for (const FString& Alias : Row.Aliases)
		{
			const FString Normalised = Alias.TrimStartAndEnd().ToLower();
			if (Normalised.IsEmpty() || Normalised == CanonicalSymbol)
			{
				continue;
			}

			OutAliases.AddUnique(Normalised);
		}

		// Sorted by string, never by FName — FName's ordering is by comparison
		// index, which depends on the order names were first registered in the
		// process and is not stable across runs. Zone A has to be byte-stable.
		OutAliases.Sort([](const FString& Lhs, const FString& Rhs)
		{
			return Lhs < Rhs;
		});
	}

	/** Renders one labelled section of the table. Rows with no aliases are dropped. */
	void AppendSection(FString& Out, const TCHAR* Label, const TArray<FSiegeAssistantSynonym>& Rows)
	{
		Out += TEXT("[");
		Out += Label;
		Out += TEXT("]\n");

		// Deterministic row order regardless of how the DataAsset was authored.
		TArray<FSiegeAssistantSynonym> Sorted = Rows;
		Sorted.Sort([](const FSiegeAssistantSynonym& Lhs, const FSiegeAssistantSynonym& Rhs)
		{
			return Lhs.Canonical.ToString().ToLower() < Rhs.Canonical.ToString().ToLower();
		});

		for (const FSiegeAssistantSynonym& Row : Sorted)
		{
			if (Row.Canonical.IsNone())
			{
				continue;
			}

			const FString CanonicalSymbol = Row.Canonical.ToString().ToLower();

			TArray<FString> Aliases;
			NormaliseAliases(Row, CanonicalSymbol, Aliases);
			if (Aliases.Num() == 0)
			{
				continue;
			}

			Out += CanonicalSymbol;
			Out += TEXT(" <- ");
			Out += FString::Join(Aliases, TEXT(", "));
			Out += TEXT("\n");
		}
	}
}

USiegeAssistantVocabulary::USiegeAssistantVocabulary()
{
	// --- UNITS ---------------------------------------------------------------
	// A kind listed here that is not currently alive simply never appears as a
	// grammar alternative, which is the architecture working as intended rather
	// than a gap — the vocabulary describes the language, the grammar describes
	// reality.
	//
	// ⚠️⚠️ A UNIT SYMBOL IS THE CardID LOWER-CASED, WITH NO SEPARATOR INSERTED.
	// `MilitiaMob` becomes `militiamob`, NOT `militia_mob`. This is not a style
	// choice and it is the one thing in this file that is easy to get wrong:
	// USiegeAssistantSnapshot::CanonicalKind is a PURE, TOTAL derivation —
	// `FName(*CardID.ToString().ToLower())`, no table, no exceptions — and its
	// output is simultaneously what feeds the grammar's `kind` alternatives AND
	// what Zone C prints in the roster. A canonical here that the derivation
	// cannot produce is therefore not merely unused: it teaches the model a
	// spelling the sampler PHYSICALLY FORBIDS, while the same prompt prints a
	// different spelling for the same unit a few lines later. That self-
	// contradiction shows up as a degraded accuracy bar with NO log line naming
	// the cause (TASK-419 BLOCKER-1 — `militia_mob` shipped exactly this bug).
	//
	// ⚠️ THE PLACE BLOCK BELOW IS THE OPPOSITE, AND THAT ASYMMETRY IS THE TRAP.
	// Place symbols (`enemy_castle`, `ancient_ground_near`, `nearest_mine`) ARE
	// hand-authored with underscores, because they come from the snapshot's
	// hand-written PlaceVocabulary table rather than from a CardID. So underscores
	// are correct there and wrong here. `MilitiaMob` is the only multi-word
	// CardType == Unit row today, which is exactly why the mismatch survived
	// review — a future multi-word unit (say `ShieldMaiden` -> `shieldmaiden`)
	// walks straight back into it. Derive it, never spell it by eye.
	//
	// Restricted to CardType == Unit: a building cannot be sent anywhere.
	//
	// ⚠️ "mage", "caster", "spellcaster" and "magic user" appear under NEITHER
	// wizard NOR sorcerer, deliberately. They are genuinely ambiguous between two
	// different cards, and a guess here is a confidently-wrong command. The notes
	// block routes them to a which_unit question instead.
	UnitSynonyms.Add(MakeSynonym(TEXT("archer"), { TEXT("archers"), TEXT("bowman"), TEXT("bowmen"), TEXT("bows"), TEXT("shooters") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("cavalry"), { TEXT("cav"), TEXT("horse"), TEXT("horseman"), TEXT("horsemen"), TEXT("rider"), TEXT("riders") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("cleric"), { TEXT("clerics"), TEXT("healer"), TEXT("healers"), TEXT("medic"), TEXT("priest"), TEXT("priests") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("footman"), { TEXT("foot"), TEXT("footmen"), TEXT("infantry"), TEXT("soldier"), TEXT("soldiers"), TEXT("swordsmen") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("knight"), { TEXT("heavies"), TEXT("heavy"), TEXT("knights") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("longbowman"), { TEXT("longbow"), TEXT("longbowmen"), TEXT("longbows") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("militiamob"), { TEXT("militia"), TEXT("mob"), TEXT("peasants"), TEXT("rabble") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("miner"), { TEXT("miners"), TEXT("worker"), TEXT("workers") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("ogre"), { TEXT("brute"), TEXT("brutes"), TEXT("giant"), TEXT("giants"), TEXT("ogres") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("pikeman"), { TEXT("pikemen"), TEXT("pikes"), TEXT("spearman"), TEXT("spearmen"), TEXT("spears") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("sapper"), { TEXT("bomber"), TEXT("bombers"), TEXT("demolition"), TEXT("sappers") }));
	UnitSynonyms.Add(MakeSynonym(TEXT("sorcerer"), { TEXT("ritualist"), TEXT("ritualists"), TEXT("sorcerers") }));
	// ⚠️ `wizard` deliberately carries ONE alias. "fire mage" was removed
	// (TASK-419 WARN-1): this table is PROMPT TEXT READ BY AN LLM, not an
	// exact-match lookup — nothing in the lane resolves an alias in code — so a
	// model shown `wizard <- fire mage` and then handed "send the mage..." has a
	// real pull toward Wizard. That converts an honest clarification into a
	// confident wrong command, and it is scored: two sealed corpus rows (DEV-06
	// and the HOLDOUT row HOLD-09) exist to punish exactly it. Zone A must not
	// contain one line pulling toward Wizard and a [notes] line pulling toward
	// which_unit. A test asserts no unit alias contains "mage" as a SUBSTRING,
	// which is stricter than the §9b token prohibition on purpose.
	UnitSynonyms.Add(MakeSynonym(TEXT("wizard"), { TEXT("wizards") }));

	// --- PLACES --------------------------------------------------------------
	// The fixed named-place vocabulary (CONVENTIONS §8). Resolved FVectors stay
	// game-side behind USiegeAssistantSnapshot::ResolvePlace — no coordinate ever
	// reaches the prompt (§3).
	//
	// The canonical symbols below MIRROR USiegeAssistantSnapshot's PlaceVocabulary
	// table (TASK-416) exactly. That table is the authority — it is what
	// GetPlaceNames() feeds to the grammar — and this list only teaches the model
	// which player phrases point at those symbols. If TASK-416 adds a place, this
	// list wants the matching aliases; if it does not get them, nothing breaks,
	// the model just has to work harder to guess the symbol. That asymmetry is
	// the architecture: the vocabulary can only ever help the model CHOOSE among
	// symbols the grammar already permits, never widen them.
	PlaceSynonyms.Add(MakeSynonym(TEXT("ancient_ground_far"), { TEXT("far ancient ground"), TEXT("far runes"), TEXT("the far ground"), TEXT("their ancient ground") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("ancient_ground_near"), { TEXT("ancient ground"), TEXT("near ancient ground"), TEXT("near runes"), TEXT("our ancient ground"), TEXT("the runes") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("enemy_castle"), { TEXT("enemy base"), TEXT("enemy castle"), TEXT("red castle"), TEXT("their base"), TEXT("their castle") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("hero"), { TEXT("my hero"), TEXT("my position"), TEXT("where i am") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("mid"), { TEXT("center"), TEXT("centre"), TEXT("middle"), TEXT("the capture zone"), TEXT("the middle") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("nearest_mine"), { TEXT("gold"), TEXT("gold mine"), TEXT("mine"), TEXT("the mine"), TEXT("the mines") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("own_castle"), { TEXT("base"), TEXT("home"), TEXT("my castle"), TEXT("our base"), TEXT("our castle"), TEXT("the keep") }));

	// --- INTENTS -------------------------------------------------------------
	// Canonical symbols are the ESiegeAssistantIntent wire symbols.
	//
	// ⚠️ "defend" appears under NEITHER guard NOR fallback. It sits exactly on the
	// executor seam — guard stations the units you named at a place, fallback
	// swings the WHOLE army back to your own castle — and picking one would move
	// an army the player never mentioned. The notes block routes it to a
	// which_intent question.
	IntentSynonyms.Add(MakeSynonym(TEXT("ambush"), { TEXT("hide"), TEXT("lie in wait"), TEXT("set a trap"), TEXT("trap"), TEXT("waylay") }));
	IntentSynonyms.Add(MakeSynonym(TEXT("charge"), { TEXT("all in"), TEXT("all out attack"), TEXT("everyone attack"), TEXT("full attack"), TEXT("push everything"), TEXT("rush") }));
	IntentSynonyms.Add(MakeSynonym(TEXT("fallback"), { TEXT("everyone back"), TEXT("fall back"), TEXT("pull back"), TEXT("regroup"), TEXT("retreat") }));
	IntentSynonyms.Add(MakeSynonym(TEXT("follow"), { TEXT("come with me"), TEXT("escort me"), TEXT("follow me"), TEXT("on me"), TEXT("stay with me"), TEXT("with me") }));
	IntentSynonyms.Add(MakeSynonym(TEXT("guard"), { TEXT("garrison"), TEXT("hold"), TEXT("protect"), TEXT("station"), TEXT("watch") }));
	IntentSynonyms.Add(MakeSynonym(TEXT("rally"), { TEXT("banner"), TEXT("call to arms"), TEXT("rally up") }));
	IntentSynonyms.Add(MakeSynonym(TEXT("send"), { TEXT("advance"), TEXT("go"), TEXT("march"), TEXT("move"), TEXT("push"), TEXT("take") }));
}

FString USiegeAssistantVocabulary::BuildSynonymTable() const
{
	FString Table;
	Table.Reserve(2048);

	Table += TEXT("SYNONYMS\n");

	AppendSection(Table, TEXT("units"), UnitSynonyms);
	AppendSection(Table, TEXT("places"), PlaceSynonyms);
	AppendSection(Table, TEXT("intents"), IntentSynonyms);

	// --- notes ---------------------------------------------------------------
	// Authored in C++, not in the asset: these encode disambiguation LAW and must
	// survive an artist re-authoring DA_AssistantVocabulary. Written in terse
	// arrow notation rather than sentences — this is model-facing prompt content,
	// and nothing here is ever shown to a player (CONVENTIONS §3: every
	// player-facing string is a game-authored template filled from a reason code).
	Table += TEXT("[notes]\n");
	Table += TEXT("wizard != sorcerer. wizard = ranged fire caster. sorcerer = ritualist, cannot attack, empowers friendlies on an ancient ground. mage / caster / spellcaster = ambiguous -> ask which_unit.\n");
	Table += TEXT("send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend = ambiguous between guard and fallback -> ask which_intent.\n");
	Table += TEXT("nearest_mine already means the best mine for the player right now. there is no per-mine symbol.\n");
	Table += TEXT("use only the place symbols listed in the state block. a unit kind that is not listed there does not exist right now -> ask which_unit.\n");

	return Table;
}
