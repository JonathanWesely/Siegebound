// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/StringConv.h"
#include "UObject/StrongObjectPtr.h"

#include "Siegebound/SiegeAssistantSnapshot.h"
#include "Siegebound/SiegeAssistantVocabulary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ZONE A — THE TWO-LANE BYTE-EQUALITY GATE (TASK-423; CONVENTIONS "In-match LLM
 *  command assistant" §8, §12g, and the standing WARN-5).
 *
 *  M8 DECLARATION DUTY (stated verbatim as required):
 *  "adds no replicated property, no new replicated class, no new relevancy tier."
 *
 *  ── WHAT THIS FILE EXISTS TO FIX, IN ONE PARAGRAPH ──
 *
 *  `zoneA_chars=5116` is the figure every Zone-A decision in this feature rests
 *  on — the KV-reuse bar (#3), the "do not trim Zone A" ruling, and the whole
 *  three-zone budget. IT WAS MEASURED ON THE SPIKE LANE, by
 *  `Siege.Llama.SpikePrompt` against SiegeLlamaSpike.cpp's `AppendZoneA`. ⛔ NO
 *  COMMAND HAS EVER PRINTED THE SHIPPED `USiegeAssistantSnapshot::BuildZoneA`,
 *  so the shipped lane's byte count was a READING-LEVEL CLAIM carried across two
 *  loops and two gates (CONVENTIONS §12g's standing WARN). Two byte-exact hand
 *  derivations did not close it, because BOTH WERE ON THE SPIKE LANE, and an
 *  agreement between a derivation and a measurement of the SAME lane says
 *  nothing about the other one.
 *
 *  This file is the instrument that closes it: it runs BOTH lanes and compares
 *  them BYTE FOR BYTE, with no model resident and no PIE session.
 *
 *  ── ⛔ THE ASSERTION IS ON CHARACTERS AND BYTES. THERE IS NO TOKEN CONSTANT IN
 *     THIS FILE, AND ADDING ONE IS A QA FAIL (CONVENTIONS §12g) ──
 *
 *  A token figure derived from a character count has a MEASURED error band of
 *  −15 to +4 tokens WITH UNPREDICTABLE SIGN (§12g, n=2: a large edit
 *  over-predicted by 15, an 8-char edit under-predicted by 4 because its cost
 *  was paid by the re-tokenised text it did not touch). Baking such a figure
 *  into an automated test would be an assertion whose ERROR DIRECTION IS
 *  UNKNOWN, wearing the authority of a green test — which is strictly worse
 *  than no test, because a guardrail that reports safe stops anyone looking.
 *
 *  Characters are the opposite kind of quantity: exact, countable offline with
 *  no model resident, and — because this repo's prompt literals are ASCII-clean
 *  — identical to UTF-8 bytes. THAT PREMISE IS ITSELF ASSERTED HERE
 *  (AsciiCleanliness), because §12g leans on it everywhere and until now nothing
 *  checked it.
 *
 *  ⚖️ TWO ARTIFACTS, TWO QUANTITIES, NEITHER DOING THE OTHER'S JOB: this TEST
 *  asserts lane equality and an exact CHAR count; the token ceiling stays a
 *  RUNTIME gate, read off the printed `zoneA_tok~=` at a measurement run.
 */

namespace SiegeAssistantZoneATestFixture
{
	/**
	 *  ⛔⛔ VERBATIM TRANSCRIPTION OF THE SPIKE LANE. FROZEN. DO NOT "FIX" IT.
	 *
	 *  Every line below is a CHARACTER-FOR-CHARACTER COPY of an emitting line in
	 *  `AppendZoneA` in
	 *      Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp
	 *  in source order, comments excluded. That function is the lane on which
	 *  `zoneA_chars=5116` was MEASURED, so these bytes ARE the measurement's
	 *  subject. They are reproduced here for one reason: the spike is throwaway
	 *  by construction and is scheduled for deletion, and deleting the only
	 *  instrument that can corroborate the number BEFORE the corroboration is
	 *  taken would strand the claim permanently (TASK-423 amendment A).
	 *
	 *  ⚠️ HOW TO RE-VERIFY THIS FIXTURE WITHOUT TRUSTING ITS AUTHOR — and while
	 *  SiegeLlamaSpike.cpp still exists, THIS IS THE CHECK QA SHOULD ACTUALLY
	 *  RUN. Extract every `Out += TEXT(...)` line from the spike's `AppendZoneA`
	 *  and diff it against the block below. It is a plain text diff: no compiler,
	 *  no model, no engine. They must match exactly, 98 lines, in order.
	 *
	 *  ⛔ WHEN THE SPIKE IS DELETED THIS BLOCK BECOMES THE ONLY SURVIVING RECORD
	 *  OF THE MEASURED BYTES, and its status changes from "copy" to "evidence".
	 *  At that point a divergence reported by the equality test below means THE
	 *  SHIPPED BUILDER MOVED AWAY FROM THE MEASUREMENT — i.e. `zoneA_chars=5116`
	 *  and every figure derived from it have gone stale and must be RE-MEASURED
	 *  on the model. It does NOT mean this fixture is out of date. ⛔ EDITING
	 *  THIS BLOCK TO MAKE THE TEST GO GREEN DESTROYS THE ONLY EVIDENCE THE TEST
	 *  EXISTS TO HOLD, and is the single worst thing that can be done to this
	 *  file. Re-measure instead, then move both sides together with the new
	 *  number.
	 *
	 *  ⚠️ IT IS DELIBERATELY A DUMB LITERAL BLOCK AND NOT A CALL INTO THE PLUGIN.
	 *  The game module does not depend on SiegeLlama, `AppendZoneA` is a file-
	 *  static in a private .cpp, and the plugin must never include a Siegebound
	 *  header (CONVENTIONS §8 — the whole API surface between the lanes is
	 *  `(prompt, gbnf) -> string`). So the two lanes CANNOT be linked into one
	 *  process by any legal arrangement, and a frozen copy of the measured bytes
	 *  is the strongest instrument the architecture permits. That limit is
	 *  restated plainly in the handoff; it is the reason this test is a strong
	 *  result rather than a total one.
	 */
	static FString BuildSpikeLaneZoneA()
	{
		FString Out;
		Out.Reserve(6144);

		Out += TEXT("[RULES]\n");
		Out += TEXT("Turn ONE Siegebound order into ONE JSON command. Output the JSON object only: no prose, no explanation.\n");
		Out += TEXT("\n");
		Out += TEXT("schema (a command):\n");
		Out += TEXT("{\"intent\":INTENT,\"who\":WHO,\"where\":WHERE,\"when\":WHEN}\n");
		Out += TEXT("INTENT = send | guard | ambush | follow | charge | fallback | rally\n");
		Out += TEXT("WHO    = [{\"kind\":KIND,\"n\":COUNT}] with 1 to 3 entries, or \"all\", or \"none\"\n");
		Out += TEXT("KIND   = a unit symbol from roster in [FORCES]\n");
		Out += TEXT("COUNT  = 1 to 30, or \"all\"\n");
		Out += TEXT("WHERE  = a place symbol from places in [FORCES], or \"none\"\n");
		Out += TEXT("WHEN   = \"now\", or {\"kind\":KIND,\"at_least\":1 to 30}\n");
		Out += TEXT("\n");
		Out += TEXT("schema (a question, when the order cannot be translated):\n");
		Out += TEXT("{\"ask\":ASK}\n");
		Out += TEXT("ASK = which_unit | which_place | how_many | which_intent | unsupported\n");
		Out += TEXT("\n");
		Out += TEXT("intents:\n");
		Out += TEXT("send = move the selected units to a place\n");
		Out += TEXT("guard = station them at a place and hold it\n");
		Out += TEXT("ambush = station them at a place and let them chase kills\n");
		Out += TEXT("follow = they follow the hero\n");
		Out += TEXT("charge = whole army attacks; who and where are \"none\"\n");
		Out += TEXT("fallback = whole army defends home; who and where are \"none\"\n");
		Out += TEXT("rally = hero rallies units near him; who and where are \"none\"\n");
		Out += TEXT("\n");
		Out += TEXT("places (fixed vocabulary; only those listed in [FORCES] exist this match):\n");
		Out += TEXT("own_castle = the player's castle\n");
		Out += TEXT("enemy_castle = the enemy castle\n");
		Out += TEXT("mid = the capturable centre zone\n");
		Out += TEXT("ancient_ground_near = the ancient ground on the player's side\n");
		Out += TEXT("ancient_ground_far = the ancient ground on the enemy side\n");
		Out += TEXT("nearest_mine = the best gold mine for the player now\n");
		Out += TEXT("hero = where the player's hero stands\n");
		Out += TEXT("\n");
		Out += TEXT("rules:\n");
		Out += TEXT("- Symbols only. Never a coordinate, distance, direction or actor name.\n");
		Out += TEXT("- Ask for the count the player said even if the roster holds fewer; the game reports the shortfall.\n");
		Out += TEXT("- One order in, one command out. You never see an earlier turn.\n");
		Out += TEXT("- If the order is not one of the seven intents, return a question instead of guessing.\n");
		Out += TEXT("- If the unit named is not a kind in [FORCES], answer {\"ask\":\"unsupported\"}. Never write a kind the player did not name.\n");
		Out += TEXT("- Gold, buying and card play are the player's, never yours: {\"ask\":\"unsupported\"}.\n");
		Out += TEXT("- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.\n");
		Out += TEXT("- No number said: a plural = \"all\", a singular = 1. Never copy a count from the roster.\n");
		Out += TEXT("- Choose a place by its noun - ancient ground, mine, castle, centre. near, nearest and far only say which one.\n");
		Out += TEXT("\n");
		Out += TEXT("synonyms:\n");
		Out += TEXT("SYNONYMS\n");
		Out += TEXT("[units]\n");
		Out += TEXT("archer <- archers, bowman, bowmen, bows, shooters\n");
		Out += TEXT("cavalry <- cav, horse, horseman, horsemen, rider, riders\n");
		Out += TEXT("cleric <- clerics, healer, healers, medic, priest, priests\n");
		Out += TEXT("footman <- foot, footmen, infantry, soldier, soldiers, swordsmen\n");
		Out += TEXT("knight <- heavies, heavy, knights\n");
		Out += TEXT("longbowman <- longbow, longbowmen, longbows\n");
		Out += TEXT("militiamob <- militia, mob, peasants, rabble\n");
		Out += TEXT("miner <- miners, worker, workers\n");
		Out += TEXT("ogre <- brute, brutes, giant, giants, ogres\n");
		Out += TEXT("pikeman <- pikemen, pikes, spearman, spearmen, spears\n");
		Out += TEXT("sapper <- bomber, bombers, demolition, sappers\n");
		Out += TEXT("sorcerer <- ritualist, ritualists, sorcerers\n");
		Out += TEXT("wizard <- wizards\n");
		Out += TEXT("[places]\n");
		Out += TEXT("ancient_ground_far <- far ancient ground, far runes, the far ground, their ancient ground\n");
		Out += TEXT("ancient_ground_near <- ancient ground, near ancient ground, near runes, nearest ancient ground, our ancient ground, the runes\n");
		Out += TEXT("enemy_castle <- enemy base, enemy castle, red castle, their base, their castle\n");
		Out += TEXT("hero <- my hero, my position, where i am\n");
		Out += TEXT("mid <- center, centre, middle, the capture zone, the middle\n");
		Out += TEXT("nearest_mine <- gold mine, mine, the mine, the mines\n");
		Out += TEXT("own_castle <- base, home, my castle, our base, our castle, the keep\n");
		Out += TEXT("[intents]\n");
		Out += TEXT("ambush <- hide, lie in wait, set a trap, trap, waylay\n");
		Out += TEXT("charge <- all in, all out attack, everyone attack, full attack, push everything, rush\n");
		Out += TEXT("fallback <- everyone back, fall back, pull back, regroup, retreat\n");
		Out += TEXT("follow <- come with me, escort me, follow me, on me, stay with me, with me\n");
		Out += TEXT("guard <- garrison, hold, protect, station, watch\n");
		Out += TEXT("rally <- banner, call to arms, rally up\n");
		Out += TEXT("send <- advance, go, march, move, push, take\n");
		Out += TEXT("[notes]\n");
		Out += TEXT("wizard != sorcerer. wizard = ranged fire caster. sorcerer = ritualist, cannot attack, empowers friendlies on an ancient ground. mage / caster / spellcaster = ambiguous -> ask which_unit.\n");
		Out += TEXT("archer != longbowman. bow, bows, bowman, bowmen = archer. only a long- word = longbowman.\n");
		Out += TEXT("send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend = ambiguous between guard and fallback -> ask which_intent.\n");
		Out += TEXT("use only the place symbols listed in the state block.\n");
		Out += TEXT("\n");
		Out += TEXT("examples:\n");
		Out += TEXT("order: send ten footmen with a sorcerer to the ancient ground on our side\n");
		Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1}],\"where\":\"ancient_ground_near\",\"when\":\"now\"}\n");
		Out += TEXT("order: all archers guard the middle\n");
		Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":\"all\"}],\"where\":\"mid\",\"when\":\"now\"}\n");
		Out += TEXT("order: the archer guards our castle\n");
		Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":1}],\"where\":\"own_castle\",\"when\":\"now\"}\n");
		Out += TEXT("order: send werewolves to the middle\n");
		Out += TEXT("{\"ask\":\"unsupported\"}\n");
		Out += TEXT("order: get two more pikemen with our gold\n");
		Out += TEXT("{\"ask\":\"unsupported\"}\n");
		Out += TEXT("order: i want the footmen to rush\n");
		Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":\"all\"}],\"where\":\"none\",\"when\":\"now\"}\n");
		Out += TEXT("order: everyone attack\n");
		Out += TEXT("{\"intent\":\"charge\",\"who\":\"none\",\"where\":\"none\",\"when\":\"now\"}\n");

		return Out;
	}

	/**
	 *  THE MEASURED CHARACTER COUNT OF ZONE A — 5116.
	 *
	 *  ⚠️ PROVENANCE, BECAUSE THIS PROJECT HAS TWICE PAID FOR A NUMBER WHOSE
	 *  PROVENANCE WAS ASSUMED (CONVENTIONS §8 TOKEN PROVENANCE, §12g). This is a
	 *  MEASURED figure: `Siege.Llama.SpikePrompt` prints `zoneA_chars=` directly
	 *  from `AppendZoneA(...).Len()`, and the value has been reproduced by two
	 *  independent byte-exact hand derivations. It is a COUNT OF CHARACTERS
	 *  PRODUCED BY CODE, not a conversion of one unit into another, so it carries
	 *  no error band at all — which is exactly why the assertion lives on this
	 *  quantity and not on tokens.
	 *
	 *  ⛔ THE COMPANION FIGURE `zoneA_tok~=1139` IS DELIBERATELY ABSENT FROM THIS
	 *  FILE. It is a RUNTIME READING off the shipping tokenizer with the model
	 *  resident; it belongs to the measurement run, never to a test. See the file
	 *  header.
	 *
	 *  ⚠️ IF ZONE A IS EDITED ON PURPOSE, THIS NUMBER MOVES — and moving it is a
	 *  RE-MEASUREMENT, not an arithmetic update. Run the spike command (or its
	 *  successor), read the printed `zoneA_chars=`, and put THAT here. ⛔ Do not
	 *  compute the new value by adding your diff's character count to 5116: that
	 *  is exactly the derivation-in-place-of-measurement move §12g exists to stop,
	 *  and the token figure that travels with it does not scale linearly.
	 */
	static constexpr int32 MeasuredZoneAChars = 5116;

	/** True when every code unit is ASCII — the premise that makes char count == UTF-8 byte count. */
	static bool IsAsciiClean(const FString& In, int32& OutFirstNonAsciiIndex)
	{
		OutFirstNonAsciiIndex = INDEX_NONE;
		for (int32 Index = 0; Index < In.Len(); ++Index)
		{
			if (static_cast<uint32>(In[Index]) > 127u)
			{
				OutFirstNonAsciiIndex = Index;
				return false;
			}
		}
		return true;
	}

	/** UTF-8 byte length, excluding any terminator. Asserted against Len() rather than assumed equal to it. */
	static int32 Utf8ByteLength(const FString& In)
	{
		FTCHARToUTF8 Converted(*In);
		return Converted.Length();
	}

	/** Index of the first differing character, or INDEX_NONE when the common prefix runs to the end of the shorter string. */
	static int32 FirstDifference(const FString& Lhs, const FString& Rhs)
	{
		const int32 Shortest = FMath::Min(Lhs.Len(), Rhs.Len());
		for (int32 Index = 0; Index < Shortest; ++Index)
		{
			if (Lhs[Index] != Rhs[Index])
			{
				return Index;
			}
		}
		return (Lhs.Len() == Rhs.Len()) ? INDEX_NONE : Shortest;
	}

	/**
	 *  A readable window around a divergence. A bare "the strings differ" on a
	 *  5116-character prompt is unactionable, and the whole value of this gate is
	 *  that a FAILURE tells the next reader WHERE the two lanes parted.
	 */
	static FString Window(const FString& In, int32 At, int32 Radius = 60)
	{
		const int32 Start = FMath::Max(0, At - Radius);
		const int32 Count = FMath::Min(In.Len() - Start, Radius * 2);
		return (Count > 0) ? In.Mid(Start, Count).ReplaceCharWithEscapedChar() : FString(TEXT("<end of string>"));
	}

	/** A default-constructed vocabulary: the C++ constructor defaults, which are the rows the spike lane transcribed. */
	static TStrongObjectPtr<USiegeAssistantVocabulary> MakeDefaultVocabulary()
	{
		return TStrongObjectPtr<USiegeAssistantVocabulary>(NewObject<USiegeAssistantVocabulary>());
	}

	/** A snapshot object that has never had Capture() run on it. Zone A reads no member state, so that is the point. */
	static TStrongObjectPtr<USiegeAssistantSnapshot> MakeSnapshot()
	{
		return TStrongObjectPtr<USiegeAssistantSnapshot>(NewObject<USiegeAssistantSnapshot>());
	}
}

// ===========================================================================
//  1. THE TWO-LANE BYTE EQUALITY — the assertion this whole file exists for
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantZoneATwoLaneEqualityTest,
	"Siegebound.Assistant.ZoneA.TwoLaneByteEquality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 *  ⚠️ THE VOCABULARY ARGUMENT IS LOAD-BEARING AND IT IS NOT `nullptr`.
 *
 *  The shipped Zone A's synonym block is the only part of it that varies, and it
 *  varies with the vocabulary object. The spike transcribed the output of
 *  `USiegeAssistantVocabulary`'s C++ CONSTRUCTOR DEFAULTS run through
 *  `BuildSynonymTable()`, so the comparable shipped call is the one passed a
 *  DEFAULT-CONSTRUCTED vocabulary. Passing null would print `none` and compare a
 *  different string; passing the /Game/Data/DA_AssistantVocabulary asset would
 *  compare whatever an artist last saved. ⚠️ THAT SECOND CASE IS A REAL, OPEN
 *  GAP AND IT IS NAMED IN THE HANDOFF: this test proves the CODE-DEFAULT lane
 *  matches the measurement; it says nothing about the ASSET lane, and the asset
 *  overrides the defaults wholesale at runtime.
 */
bool FSiegeAssistantZoneATwoLaneEqualityTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantZoneATestFixture;

	TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot = MakeSnapshot();
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary = MakeDefaultVocabulary();

	if (!TestTrue(TEXT("A snapshot object was created"), Snapshot.IsValid())
		|| !TestTrue(TEXT("A default vocabulary object was created"), Vocabulary.IsValid()))
	{
		return false;
	}

	const FString ShippedLane = Snapshot->BuildZoneA(Vocabulary.Get());
	const FString SpikeLane = BuildSpikeLaneZoneA();

	// Reported before the equality assertion on purpose: when this test fails,
	// the two lengths are the first thing the next reader needs, and an
	// AddError on the comparison alone would bury them.
	AddInfo(FString::Printf(
		TEXT("Zone A lengths - shipped: %d chars / %d UTF-8 bytes; spike: %d chars / %d UTF-8 bytes; measured reference: %d chars."),
		ShippedLane.Len(), Utf8ByteLength(ShippedLane),
		SpikeLane.Len(), Utf8ByteLength(SpikeLane),
		MeasuredZoneAChars));

	const int32 Divergence = FirstDifference(ShippedLane, SpikeLane);
	if (Divergence != INDEX_NONE)
	{
		AddError(FString::Printf(
			TEXT("THE TWO ZONE A LANES HAVE DIVERGED at character %d. `zoneA_chars=%d` and everything derived from it are now STALE and must be RE-MEASURED on the model - do NOT edit the frozen spike fixture to silence this.\n")
			TEXT("  shipped: ...%s...\n")
			TEXT("  spike  : ...%s..."),
			Divergence, MeasuredZoneAChars,
			*Window(ShippedLane, Divergence), *Window(SpikeLane, Divergence)));
	}

	// ⛔⛔ `TestEqualSensitive`, NEVER `TestEqual`, ON EVERY STRING IN THIS FILE.
	// FAutomationTestBase::TestEqual(const FString&, const FString&) forwards to
	// the TCHAR* overload, which is CASE-INSENSITIVE — that is the entire reason
	// TestEqualSensitive exists as a separate API. A byte-equality gate written
	// with TestEqual would pass two Zone As differing in case, i.e. it would
	// report SAFE on a prompt where `own_castle` had become `OWN_CASTLE` and the
	// tokenizer output had changed completely. That is precisely the
	// "automated guardrail that reports safe" failure CONVENTIONS §12g rules
	// worse than having no test at all, so it is called out here rather than
	// left as a silent idiom for the next editor to undo.
	TestEqualSensitive(TEXT("The shipped BuildZoneA(default vocabulary) is BYTE-IDENTICAL to the spike lane that `zoneA_chars=5116` was measured on"),
		ShippedLane, SpikeLane);

	return true;
}

// ===========================================================================
//  2. THE MEASURED CHARACTER COUNT — pinned on BOTH lanes
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantZoneAMeasuredCharCountTest,
	"Siegebound.Assistant.ZoneA.MeasuredCharCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 *  ⚠️ THIS IS NOT REDUNDANT WITH THE EQUALITY TEST, AND THE REASON IS THE ONE
 *  FAILURE MODE EQUALITY ALONE CANNOT SEE: two lanes that BOTH moved by the same
 *  edit stay equal to each other while both drift away from the number that was
 *  measured. Equality proves the lanes agree; this proves they agree ON 5116.
 *  Only the pair discharges the claim.
 */
bool FSiegeAssistantZoneAMeasuredCharCountTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantZoneATestFixture;

	TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot = MakeSnapshot();
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary = MakeDefaultVocabulary();

	if (!TestTrue(TEXT("A snapshot object was created"), Snapshot.IsValid())
		|| !TestTrue(TEXT("A default vocabulary object was created"), Vocabulary.IsValid()))
	{
		return false;
	}

	const FString ShippedLane = Snapshot->BuildZoneA(Vocabulary.Get());
	const FString SpikeLane = BuildSpikeLaneZoneA();

	TestEqual(TEXT("The SHIPPED Zone A is exactly the measured 5116 characters"),
		ShippedLane.Len(), MeasuredZoneAChars);
	TestEqual(TEXT("The SPIKE Zone A is exactly the measured 5116 characters"),
		SpikeLane.Len(), MeasuredZoneAChars);

	// The byte figure is asserted separately from the char figure rather than
	// inferred from it. `MaxUtteranceBytes` was renamed by TASK-433 precisely
	// because a constant named "Chars" that measured bytes hid a 3x
	// over-admission; the same conflation is not going to be re-introduced here
	// by assumption.
	TestEqual(TEXT("The SHIPPED Zone A is exactly 5116 UTF-8 BYTES (asserted, not inferred from the char count)"),
		Utf8ByteLength(ShippedLane), MeasuredZoneAChars);
	TestEqual(TEXT("The SPIKE Zone A is exactly 5116 UTF-8 BYTES"),
		Utf8ByteLength(SpikeLane), MeasuredZoneAChars);

	return true;
}

// ===========================================================================
//  3. ASCII CLEANLINESS — the premise that makes chars and bytes the same thing
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantZoneAAsciiCleanTest,
	"Siegebound.Assistant.ZoneA.AsciiCleanliness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 *  CONVENTIONS §12g's entire char-based assertion rests on "this repo's prompt
 *  literals are verified ASCII-clean, so char == byte". THAT PROPERTY HAD NEVER
 *  BEEN CHECKED BY ANYTHING — it was a reviewer's reading of the source, and it
 *  is exactly the kind of premise that stops being true silently: one em dash or
 *  one curly apostrophe pasted into a rule line by a later editor breaks it, and
 *  every char-based figure in the feature quietly starts over-admitting. The
 *  comment blocks in these files are FULL of non-ASCII (⚠️, —, §), so the
 *  distance between a safe file and a broken one is one careless copy-paste
 *  across the quote marks.
 */
bool FSiegeAssistantZoneAAsciiCleanTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantZoneATestFixture;

	TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot = MakeSnapshot();
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary = MakeDefaultVocabulary();

	if (!TestTrue(TEXT("A snapshot object was created"), Snapshot.IsValid())
		|| !TestTrue(TEXT("A default vocabulary object was created"), Vocabulary.IsValid()))
	{
		return false;
	}

	const FString ShippedLane = Snapshot->BuildZoneA(Vocabulary.Get());
	const FString SpikeLane = BuildSpikeLaneZoneA();

	int32 FirstNonAscii = INDEX_NONE;
	if (!IsAsciiClean(ShippedLane, FirstNonAscii))
	{
		AddError(FString::Printf(
			TEXT("The SHIPPED Zone A contains a non-ASCII character at index %d - char count and UTF-8 byte count have SEPARATED, and every char-derived budget figure in this feature is now over-admitting. Context: ...%s..."),
			FirstNonAscii, *Window(ShippedLane, FirstNonAscii)));
	}
	TestTrue(TEXT("The SHIPPED Zone A is ASCII-clean"), FirstNonAscii == INDEX_NONE);

	FirstNonAscii = INDEX_NONE;
	if (!IsAsciiClean(SpikeLane, FirstNonAscii))
	{
		AddError(FString::Printf(
			TEXT("The SPIKE Zone A fixture contains a non-ASCII character at index %d. Context: ...%s..."),
			FirstNonAscii, *Window(SpikeLane, FirstNonAscii)));
	}
	TestTrue(TEXT("The SPIKE Zone A fixture is ASCII-clean"), FirstNonAscii == INDEX_NONE);

	// The consequence of ASCII-cleanliness, asserted as its own statement so a
	// future reader can see the implication being CHECKED rather than assumed.
	TestEqual(TEXT("char count == UTF-8 byte count for the shipped Zone A"),
		Utf8ByteLength(ShippedLane), ShippedLane.Len());
	TestEqual(TEXT("char count == UTF-8 byte count for the spike Zone A"),
		Utf8ByteLength(SpikeLane), SpikeLane.Len());

	return true;
}

// ===========================================================================
//  4. THE STATIC-PREFIX CONTRACT — the header's stated QA CRITERION, untested
//     until now, and the thing bar #3's KV reuse actually depends on
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantZoneADeterminismTest,
	"Siegebound.Assistant.ZoneA.StaticPrefixContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 *  SiegeAssistantSnapshot.h states this in so many words — "QA CRITERION:
 *  calling BuildZoneA twice in one process returns byte-identical strings" — and
 *  nothing has ever run it. It is the property `llama_memory_seq_rm` prefix
 *  retention depends on: if Zone A's bytes move between turns, the cached prefix
 *  is thrown away and the ~70% prefill drop (bar #3) silently stops happening.
 *  ⚠️ THAT FAILURE SHOWS UP AS A LATENCY REGRESSION, NEVER AS A WRONG ANSWER,
 *  which is why it needs a test rather than a playtest.
 */
bool FSiegeAssistantZoneADeterminismTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantZoneATestFixture;

	TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot = MakeSnapshot();
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary = MakeDefaultVocabulary();

	if (!TestTrue(TEXT("A snapshot object was created"), Snapshot.IsValid())
		|| !TestTrue(TEXT("A default vocabulary object was created"), Vocabulary.IsValid()))
	{
		return false;
	}

	// Every comparison below is *Sensitive — see the note in the two-lane test:
	// the plain TestEqual string overload is case-insensitive and cannot carry a
	// byte-identity claim.
	const FString First = Snapshot->BuildZoneA(Vocabulary.Get());
	const FString Second = Snapshot->BuildZoneA(Vocabulary.Get());
	TestEqualSensitive(TEXT("Calling BuildZoneA twice in one process returns byte-identical strings (the header's QA CRITERION)"),
		Second, First);

	// ZONE A READS NO MEMBER STATE, BY CONSTRUCTION. A SECOND, INDEPENDENT
	// snapshot object must therefore produce the same bytes — neither has had
	// Capture() run, but that is not what makes this pass: if Zone A ever grew a
	// dependency on a member, two distinct objects would be the cheapest way to
	// catch it without needing a UWorld.
	TStrongObjectPtr<USiegeAssistantSnapshot> Other = MakeSnapshot();
	if (TestTrue(TEXT("A second snapshot object was created"), Other.IsValid()))
	{
		TestEqualSensitive(TEXT("Two independent snapshot objects emit the same Zone A (it reads no member state)"),
			Other->BuildZoneA(Vocabulary.Get()), First);
	}

	// The CALLER CONTRACT, stated in the header: passing a different vocabulary
	// object between turns changes Zone A and throws the prefix away. Asserting
	// that two SEPARATE default-constructed vocabularies agree is what makes
	// "pass the same object every turn" a convenience rather than a trap for the
	// default case — and it pins BuildSynonymTable's normalisation as the thing
	// providing that guarantee.
	TStrongObjectPtr<USiegeAssistantVocabulary> SecondVocabulary = MakeDefaultVocabulary();
	if (TestTrue(TEXT("A second vocabulary object was created"), SecondVocabulary.IsValid()))
	{
		TestEqualSensitive(TEXT("Two separately constructed default vocabularies emit the same Zone A"),
			Snapshot->BuildZoneA(SecondVocabulary.Get()), First);
	}

	return true;
}

// ===========================================================================
//  5. THE NULL-VOCABULARY BRANCH — deterministic, and NOT the measured lane
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantZoneANullVocabularyTest,
	"Siegebound.Assistant.ZoneA.NullVocabularyIsNotTheMeasuredLane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 *  ⚠️ THE POINT OF THIS TEST IS THE SECOND HALF OF ITS NAME. `BuildZoneA(nullptr)`
 *  is a supported, documented, deterministic call — but it produces a Zone A that
 *  IS NOT THE ONE ANY MEASUREMENT WAS TAKEN ON, because the synonym block
 *  collapses to `none`. Anyone who reads `zoneA_chars=5116` and then measures a
 *  null-vocabulary run gets a different number for a reason that has nothing to
 *  do with a regression, so the difference is pinned here deliberately.
 *
 *  The expected length is DERIVED AT RUNTIME from the live synonym table rather
 *  than baked in as a second magic constant: the null branch differs from the
 *  measured lane by exactly (the table) replaced by ("none\n"), and computing it
 *  that way means a legitimate vocabulary edit moves this expectation
 *  automatically instead of failing a test that was never about the vocabulary.
 */
bool FSiegeAssistantZoneANullVocabularyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantZoneATestFixture;

	TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot = MakeSnapshot();
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary = MakeDefaultVocabulary();

	if (!TestTrue(TEXT("A snapshot object was created"), Snapshot.IsValid())
		|| !TestTrue(TEXT("A default vocabulary object was created"), Vocabulary.IsValid()))
	{
		return false;
	}

	const FString WithVocabulary = Snapshot->BuildZoneA(Vocabulary.Get());
	const FString NullLane = Snapshot->BuildZoneA(nullptr);

	// *Sensitive throughout — see the note in the two-lane test.
	TestEqualSensitive(TEXT("BuildZoneA(nullptr) is byte-stable across calls"),
		Snapshot->BuildZoneA(nullptr), NullLane);

	TestTrue(TEXT("BuildZoneA(nullptr) prints the deterministic `none` synonym block"),
		NullLane.Contains(TEXT("synonyms:\nnone\n"), ESearchCase::CaseSensitive));

	TestNotEqualSensitive(TEXT("BuildZoneA(nullptr) is NOT the lane `zoneA_chars=5116` was measured on"),
		NullLane, WithVocabulary);

	// The whole difference between the two branches, stated as an identity so a
	// future divergence points at WHICH part moved.
	const FString SynonymTable = Vocabulary->BuildSynonymTable();
	const int32 ExpectedNullLength = MeasuredZoneAChars - SynonymTable.Len() + 5; // 5 == Len("none\n")
	TestEqual(TEXT("The null-vocabulary Zone A is the measured lane with the synonym table swapped for `none`"),
		NullLane.Len(), ExpectedNullLength);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
