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
	// ⚠️ "nearest ancient ground" IS ADDED BECAUSE THE SYMBOL NAMES THEMSELVES
	// COMPETE, NOT BECAUSE THE ALIAS LIST WAS SHORT. Measured on the dev split:
	// "send 10 footmen with a sorcerer to the nearest ancient ground" resolved to
	// `nearest_mine` — the model matched the word "nearest" against the SYMBOL
	// `nearest_mine` rather than against the head noun "ancient ground". The
	// alias below beats that pull for the near ground specifically; the general
	// case (any proximity word in front of any place noun) is not closed here and
	// is recorded in the handoff as a residual risk.
	PlaceSynonyms.Add(MakeSynonym(TEXT("ancient_ground_near"), { TEXT("ancient ground"), TEXT("near ancient ground"), TEXT("near runes"), TEXT("nearest ancient ground"), TEXT("our ancient ground"), TEXT("the runes") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("enemy_castle"), { TEXT("enemy base"), TEXT("enemy castle"), TEXT("red castle"), TEXT("their base"), TEXT("their castle") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("hero"), { TEXT("my hero"), TEXT("my position"), TEXT("where i am") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("mid"), { TEXT("center"), TEXT("centre"), TEXT("middle"), TEXT("the capture zone"), TEXT("the middle") }));
	// ⚠️⚠️ THE BARE ALIAS "gold" IS REMOVED, AND IT IS THE HIGHEST-CONSEQUENCE
	// EDIT IN THIS FILE. It mapped an ECONOMY word onto a PLACE symbol, so any
	// sentence merely CONTAINING the word "gold" acquired a pull toward a mining
	// order. That is not a hypothetical: the spent generation-1 holdout row
	// "spend my gold on another ogre" — an order the assistant must REFUSE under
	// Jonathan's standing ruling that it never spends gold and never plays cards
	// — came back as `send miner x1 -> nearest_mine`, and this alias is the
	// mechanism that made a mining order the locally plausible reading.
	//
	// The class, not the string: "gold" is a RESOURCE noun, and the player says
	// it constantly without naming a destination ("we have gold", "save gold",
	// "how much gold"). The mine as a PLACE is always reachable through a mine
	// word — "gold mine", "mine", "the mine", "the mines" all survive — so the
	// removal costs no legitimate phrasing while closing every economy sentence
	// that used to leak into a place. The refusal itself is taught in Zone A's
	// rules block and by a few-shot; this removes the lure that competed with it.
	PlaceSynonyms.Add(MakeSynonym(TEXT("nearest_mine"), { TEXT("gold mine"), TEXT("mine"), TEXT("the mine"), TEXT("the mines") }));
	PlaceSynonyms.Add(MakeSynonym(TEXT("own_castle"), { TEXT("base"), TEXT("home"), TEXT("my castle"), TEXT("our base"), TEXT("our castle"), TEXT("the keep") }));

	// --- INTENTS -------------------------------------------------------------
	// Canonical symbols are the ESiegeAssistantIntent wire symbols.
	//
	// ⚠️ "defend" is still an alias of NEITHER guard NOR fallback, and it must not
	// become one — AS-§21.3 refuses promoting it to an eighth intent for the same
	// reason. It sits exactly on the executor seam: guard stations the units you
	// named at a place, fallback swings the WHOLE army back to your own castle.
	//
	// ⚠️ WHAT CHANGED, AND THIS IS THE PART WORTH KEEPING. The [notes] line below
	// used to route EVERY "defend" to a which_intent question, on the argument
	// that picking one would move an army the player never mentioned. That
	// argument was TRUE WHEN IT WAS WRITTEN, and it was falsified later by an
	// edit somewhere ELSE in the prompt: USiegeAssistantSnapshot::BuildZoneA's
	// rules block now teaches, unconditionally, "If the player names units, the
	// intent is send, guard, ambush or follow, never charge, fallback or rally"
	// (SiegeAssistantSnapshot.cpp:1050 as of 2026-08-05). When a selection is
	// present `fallback` is therefore ALREADY unreachable, so the harm this note
	// was guarding against is structurally impossible — the note was competing
	// with a rule that had already won, and nobody re-read it. Two shipped lines
	// asserted contradictory things about the same input class until TASK-541.
	//
	// ⇒ The note now splits on the antecedent that actually decides it — a
	// selection is present, or it is not. The which_intent route SURVIVES exactly
	// where the original objection still holds: no units named, so fallback
	// really is reachable and the choice really is unforced.
	//
	// ⛔ THE GENERAL LESSON (AS-§21.1): a prompt line is not a constant, it is an
	// ASSERTION ABOUT THE REST OF THE PROMPT, and it can be falsified by an edit
	// in another file that never touches this one. Any future line that conditions
	// on a harm must name the other lines capable of removing that harm.
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
	// ⚠️ THE SECOND COLLIDING PAIR, AND IT NEEDS A NOTE FOR THE SAME REASON THE
	// FIRST ONE DOES — NOT ANOTHER ALIAS.
	//
	// The spent generation-1 holdout answered "bowmen" with `longbowman` when the
	// answer was `archer`. The tempting read is "the alias was missing", and it is
	// WRONG: `bowmen` was already an alias of `archer` when that row ran, and it
	// still lost. So adding aliases could never have fixed it. What actually
	// competes is the SYMBOL `longbowman`, which literally contains "bowman" — the
	// same shape as wizard/sorcerer, where two real cards contend for one player
	// word, and which this table has always handled with a note rather than with
	// alias surgery. Deleting `longbow`/`longbows` from the longbowman row would
	// not have helped either: the attractor is the canonical symbol, which cannot
	// be deleted.
	//
	// Stated as a rule over word SHAPE ("only a long- word") rather than as a list
	// of the four burned strings, so it decides bow-words this project has never
	// written down.
	Table += TEXT("archer != longbowman. bow, bows, bowman, bowmen = archer. only a long- word = longbowman.\n");
	Table += TEXT("send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend with units -> guard. defend alone -> ask which_intent.\n");
	// ⚠️⚠️ TWO LINES WERE DELETED HERE AT LADDER LOOP 2, AND DELETION IS THE POINT.
	// TASK-431 measured the rung-1 wave moving the dev score by exactly ONE row,
	// with the double-taught row not flipping - so adding prompt content has poor
	// leverage on this model and REMOVING CONTENT THAT COMPETES is the trade with
	// better odds. Both deletions are recorded here rather than silently, because
	// a reader diffing this block will otherwise reasonably think they are gaps.
	//
	// ⛔ DELETED 1 - "nearest_mine already means the best mine for the player right
	// now. there is no per-mine symbol." (96 chars.) It defended against the model
	// inventing a per-mine symbol, which the GBNF PHYSICALLY FORBIDS: `where` is
	// generated from the live place list, so an invented symbol cannot be sampled
	// and this line was buying a guarantee the sampler already gives for free. It
	// was also the THIRD printed occurrence of the token `nearest_mine` in Zone A,
	// and that token is a measured attractor - DEV-01 resolves "the nearest ancient
	// ground" to it, and rung 2's pinned alias did NOT beat it. The one lever
	// available against an attractor whose symbol cannot be renamed is to print it
	// less often. 3 occurrences -> 2. ⚠️ The remaining two are load-bearing and
	// must NOT be chased: the places-block definition teaches the symbol at all,
	// and the alias row below is what DEV-23 ("guard the nearest mine") rides on.
	// The head-noun law that replaces this line is a RULE now, in BuildZoneA.
	//
	// ⛔ DELETED 2 - the second sentence of the line below, "a unit kind that is
	// not listed there does not exist right now -> ask which_unit." It contradicted
	// the refusal rule in BuildZoneA, which routes the same situation to
	// `unsupported`: one situation, two ask codes, and a model given two routes
	// weights each less. Worse, "does not EXIST RIGHT NOW" frames an unknown unit
	// as real-but-absent, which is an invitation to reach for a present substitute
	// - and substitution is the exact measured defect (DEV-04 answered "catapults"
	// with a live `sorcerer` order). The phrasing was licensing the failure.
	// ⚠️ DECLARED TRADE: `which_unit` was arguably the better ASK CODE for a kind
	// that is real but dead, and that nuance is now gone - everything unknown says
	// `unsupported`. No corpus row scores the difference (any question passes a
	// Refuse row), the ask code only picks a player-facing template, and no model
	// can tell the two cases apart from a roster anyway - which is why the seam
	// produced two rules instead of one. Cheap to restore if Jonathan wants the
	// softer template back; it must then be restored as ONE route, not two.
	// ⚠️ The `mage / caster / spellcaster -> ask which_unit` route is UNTOUCHED -
	// it is on the first [notes] line and DEV-06/HOLD-09 both ride on it.
	Table += TEXT("use only the place symbols listed in the state block.\n");

	return Table;
}
