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
 *  `zoneA_chars=5116` was the figure every Zone-A decision in this feature rested
 *  on — the KV-reuse bar (#3), the "do not trim Zone A" ruling, and the whole
 *  three-zone budget. (⚠️ It is now the SPIKE lane's figure only; the shipped
 *  lane moved to 5424 on 2026-08-04 — see the D4 amendment at the end of this
 *  header.) IT WAS MEASURED ON THE SPIKE LANE, by
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
 *  asserts the lane relationship and an exact CHAR count; the token ceiling stays
 *  a RUNTIME gate, read off the printed `zoneA_tok~=` at a measurement run.
 *
 *  ═══════════════════════════════════════════════════════════════════════════
 *  ⭐ AMENDED 2026-08-04 (TASK-523, batch ASSISTANT-EXCLUDE) — `D4`
 *  ═══════════════════════════════════════════════════════════════════════════
 *
 *  ⛔ THE TWO LANES ARE NO LONGER BYTE-EQUAL, AND THAT IS A RULED, DECLARED
 *  DIVERGENCE — NOT A REGRESSION AND NOT A BUG TO FIX. TASK-521 added 308
 *  characters to the SHIPPED `BuildZoneA` (a 44-char `WHO =` schema shape plus
 *  two rule lines at 111 and 153) on Jonathan's rulings 1 and 3, while
 *  `Plugins/SiegeLlama/**` was DELIBERATELY NOT TOUCHED because it is another
 *  batch's in-flight instrument (FT-§16; TASK-481 is mid-flight against it).
 *  ⇒ shipped 5424, spike 5116.
 *
 *  ⛔⛔ THE TEST WAS NOT "UPDATED TO PASS", AND THE DISTINCTION IS THE WHOLE
 *  SPEC. AS-§20.4 ruled that the original subject — "the shipped prompt is the
 *  prompt the numbers were measured on" — became FALSE the moment the shipped
 *  prompt was deliberately changed, and that no edit can make it true again. So
 *  the equality test below now asserts the SHIPPED LANE AGAINST A NAMED, DATED
 *  BASELINE expressed as THE FROZEN SPIKE FIXTURE PLUS THE THREE RULED EDITS,
 *  each reproduced as its own literal and measured by the compiler. ⛔ The frozen
 *  fixture is untouched; ⛔ `BuildZoneA`'s output is nowhere copied into it. A
 *  fourth edit to Zone A still fails this file, loudly, with a character offset.
 *
 *  ⛔ AND THE TOKEN FIGURES DID NOT MOVE WITH THE CHARACTERS. `zoneA_tok = 1139`,
 *  the `77.1 %` KV-reuse figure and every prefill number derived from them are
 *  relabelled **STALE — PENDING RE-MEASUREMENT ON THE MODEL**. They are ⛔ NOT
 *  recomputed by arithmetic and ⛔ NOT deleted. Characters are countable offline;
 *  tokens are a runtime reading with a measured ±band of unpredictable sign.
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
	 *  It does NOT mean this fixture is out of date. ⛔ EDITING THIS BLOCK TO MAKE
	 *  A TEST GO GREEN DESTROYS THE ONLY EVIDENCE THIS FILE EXISTS TO HOLD, and is
	 *  the single worst thing that can be done here.
	 *
	 *  ⚠️⚠️ AMENDED 2026-08-04 (TASK-523): THE SHIPPED BUILDER HAS NOW MOVED AWAY
	 *  FROM THIS FIXTURE ON PURPOSE — divergence `D4`, +308 chars, see the file
	 *  header. ⇒ A divergence between the two lanes is EXPECTED and is no longer
	 *  evidence of anything. What the equality test asserts instead is that the
	 *  shipped lane equals THIS BLOCK PLUS THE THREE ENUMERATED D4 EDITS, so a
	 *  FOURTH, undeclared edit still fails. ⛔ The rule that this block is never
	 *  re-copied from the shipped builder is UNCHANGED and is now load-bearing in
	 *  a second way: it is the base the declared diff is applied to, so a
	 *  re-copied fixture would make the diff assertion vacuously true.
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
	 *  THE SPIKE LANE'S MEASURED CHARACTER COUNT — 5116. ⛔ UNCHANGED, AND IT MUST
	 *  STAY UNCHANGED.
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
	 *  FILE, AND SINCE 2026-08-04 IT IS ALSO **STALE — PENDING RE-MEASUREMENT ON
	 *  THE MODEL**, along with the `77.1 %` KV-reuse figure and every prefill
	 *  number derived from either. ⛔ They are NOT recomputed by arithmetic and NOT
	 *  deleted: only `Siege.Llama.SpikePrompt` prints them, with the model
	 *  resident. A stale number that says it is stale is safe; a stale number
	 *  wearing a fresh label is how §8 lost two days.
	 */
	static constexpr int32 SpikeLaneZoneAChars = 5116;

	/**
	 *  ⭐ THE SHIPPED LANE'S CHARACTER COUNT AS OF 2026-08-04 — 5424. THE NAMED,
	 *  DATED BASELINE THIS FILE NOW ASSERTS AGAINST (batch ASSISTANT-EXCLUDE,
	 *  TASK-521 authored the edit, TASK-523 re-based these tests).
	 *
	 *  ⚠️ THE CHAR COUNT IS RE-COUNTED, THE TOKEN COUNT IS NOT, AND THAT ASYMMETRY
	 *  IS THE WHOLE RULE (AS-§20.4). Characters are countable offline with no model
	 *  resident, so a deliberate edit's new char figure is a COUNT rather than a
	 *  conversion. Tokens are not: §12g measured a −15/+4 error band with
	 *  UNPREDICTABLE SIGN on exactly this kind of derivation.
	 *
	 *  ⛔ THIS NUMBER WAS NOT TAKEN ON TRUST FROM TASK-521's HANDOFF. It is the
	 *  spike lane's 5116 plus the THREE enumerated edits below, each of which is
	 *  reproduced as its own literal in this file and measured by the compiler:
	 *      +44   the `WHO    =` schema line gains the `{"all_except":[KIND]}` shape
	 *      +111  rule line 1 — `who` is "all" for every-unit orders
	 *      +153  rule line 2 — `who` is {"all_except":[KIND]}, and only on the four
	 *            selection-bearing verbs
	 *      ————
	 *      +308  ⇒ 5116 + 308 = 5424, against AS-§20.4's 325-char Zone-A growth
	 *            budget for the WHOLE batch: 17 chars of slack, spent.
	 */
	static constexpr int32 ShippedZoneAChars = 5424;

	/** The declared D4 growth. Its three components are asserted individually below, so a wrong total cannot hide inside a right one. */
	static constexpr int32 DeclaredD4Delta = ShippedZoneAChars - SpikeLaneZoneAChars;

	// ═══════════════════════════════════════════════════════════════════════════
	//  ⭐ THE DECLARED DIVERGENCE `D4` (2026-08-04) — THE THREE EDITS, SPELLED OUT
	// ═══════════════════════════════════════════════════════════════════════════
	//
	//  ⛔⛔ READ THIS BEFORE TOUCHING THE EQUALITY TEST BELOW.
	//
	//  Editing `BuildZoneA` broke `Siegebound.Assistant.ZoneA.TwoLaneByteEquality`
	//  and `…MeasuredCharCount`, exactly as AS-§20.4 predicted it would. That
	//  clause also wrote the procedure, rather than leaving it to whoever hit the
	//  red bar at 2 a.m.:
	//
	//   · ⛔ `Plugins/SiegeLlama/**` IS NOT TOUCHED BY THIS BATCH. It is ANOTHER
	//     BATCH'S IN-FLIGHT INSTRUMENT (FT-§16 reclassified `SiegeLlamaSpike.cpp`
	//     from throwaway to LOAD-BEARING test infrastructure; TASK-481 is mid-flight
	//     against it). So the shipped lane moved and the spike lane did not: that is
	//     a NEW, DELIBERATE, RULED divergence — `D4` — and it is RECORDED, never
	//     hidden and never "fixed" by editing a transcription until it agrees.
	//
	//   · ⛔⛔ THE FROZEN SPIKE FIXTURE ABOVE IS NOT EDITED, AND THE SHIPPED
	//     BUILDER'S OUTPUT IS NOT COPIED INTO IT. That fixture is the only
	//     surviving record of the bytes `zoneA_chars=5116` was measured on. A test
	//     whose transcription is silently re-copied from the thing it is testing is
	//     a guardrail that reports SAFE — the exact failure §12g rules worse than
	//     having no test at all, and the failure this very file's comments rail
	//     against.
	//
	//  ⇒ SO THE ASSERTION IS RE-PURPOSED RATHER THAN SILENCED: the shipped lane is
	//    asserted to be the frozen spike fixture PLUS EXACTLY THESE THREE EDITS AND
	//    NOTHING ELSE. Each edit is its own literal here, so the compiler measures
	//    it and a wrong total cannot hide inside a right one. ⭐ This is STRICTLY
	//    STRONGER than the byte-equality it replaces: equality only ever said "the
	//    two lanes agree", whereas this says "the two lanes differ by precisely the
	//    diff the manager ruled and by nothing else" — and it still fails, loudly
	//    and with a character offset, on any fourth edit to Zone A.

	/** The `WHO =` schema line AS THE SPIKE LANE STILL PRINTS IT. Three shapes. */
	static const TCHAR* const D4_WhoLineBefore =
		TEXT("WHO    = [{\"kind\":KIND,\"n\":COUNT}] with 1 to 3 entries, or \"all\", or \"none\"\n");

	/**
	 *  The same line after TASK-521 (+44 chars). ⚠️ THIS EDIT IS THE AS-§9c MIRROR
	 *  LAW BEING OBEYED, NOT AN EXTRA: TASK-518 added `except` to the grammar's
	 *  `who` rule, so a Zone A that still enumerated three shapes would have TOLD
	 *  the model the exclusion shape does not exist while the sampler ALLOWED it.
	 *  The alternation order here is the GRAMMAR's order, deliberately.
	 */
	static const TCHAR* const D4_WhoLineAfter =
		TEXT("WHO    = [{\"kind\":KIND,\"n\":COUNT}] with 1 to 3 entries, or {\"all_except\":[KIND]} with 1 to 3 kinds, or \"all\", or \"none\"\n");

	/** The rule line the two new lines are inserted AFTER — an anchor, not an edit. Unchanged in both lanes. */
	static const TCHAR* const D4_RuleAnchor =
		TEXT("- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.\n");

	/**
	 *  NEW RULE 1 (+111 chars) — the `who`:"all" teaching, which is the behaviour
	 *  Jonathan actually complained about. ⭐ AS-§20.4 ruled RULE LINES ONLY and NO
	 *  NEW FEW-SHOT SENTENCES: a new example sentence would have required opening
	 *  the SEALED `assistant_eval_holdout2.csv` for a disjointness certificate,
	 *  and a rule line requires nothing. The seal is not spent.
	 */
	static const TCHAR* const D4_RuleAllUnits =
		TEXT("- Every unit, no exception: who is \"all\", never a list of kinds. charge, fallback and rally still take \"none\".\n");

	/** NEW RULE 2 (+153 chars) — the exclusion shape, gated to the four selection-bearing verbs (manager ruling 3). */
	static const TCHAR* const D4_RuleExclusion =
		TEXT("- Every unit but some kinds: who is {\"all_except\":[KIND]}, only with send, guard, ambush or follow. On charge, fallback or rally: {\"ask\":\"unsupported\"}.\n");

	/**
	 *  The spike lane with the three declared edits applied — i.e. what the SHIPPED
	 *  lane must be, byte for byte.
	 *
	 *  ⛔ EVERY REPLACEMENT IS `ESearchCase::CaseSensitive`. `FString::Replace`
	 *  defaults to CASE-INSENSITIVE, which on a byte-exact derivation would let a
	 *  casing change slip through unnoticed — the same trap `TestEqual`-on-FString
	 *  sets one layer up (SC-§13).
	 *
	 *  @param OutApplied  how many of the three edits actually matched. ⛔ Asserted
	 *                     by the caller: a replacement that silently matched
	 *                     NOTHING would leave this function returning the spike
	 *                     lane unchanged and turn the comparison into an equality
	 *                     test that has already been ruled false.
	 */
	static FString BuildDeclaredD4ShippedLane(int32& OutApplied)
	{
		FString Out = BuildSpikeLaneZoneA();
		OutApplied = 0;

		if (Out.Contains(D4_WhoLineBefore, ESearchCase::CaseSensitive))
		{
			Out.ReplaceInline(D4_WhoLineBefore, D4_WhoLineAfter, ESearchCase::CaseSensitive);
			++OutApplied;
		}

		if (Out.Contains(D4_RuleAnchor, ESearchCase::CaseSensitive))
		{
			const FString Replacement = FString(D4_RuleAnchor) + D4_RuleAllUnits + D4_RuleExclusion;
			Out.ReplaceInline(D4_RuleAnchor, *Replacement, ESearchCase::CaseSensitive);
			OutApplied += 2;
		}

		return Out;
	}

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

	int32 EditsApplied = 0;
	const FString DeclaredShippedLane = BuildDeclaredD4ShippedLane(EditsApplied);

	// Reported before the assertions on purpose: when this test fails, the lengths
	// are the first thing the next reader needs, and an AddError on the comparison
	// alone would bury them.
	AddInfo(FString::Printf(
		TEXT("Zone A lengths - shipped: %d chars / %d UTF-8 bytes (named baseline %d, 2026-08-04); spike lane: %d chars / %d UTF-8 bytes (frozen at %d); declared D4 delta: %d."),
		ShippedLane.Len(), Utf8ByteLength(ShippedLane), ShippedZoneAChars,
		SpikeLane.Len(), Utf8ByteLength(SpikeLane), SpikeLaneZoneAChars,
		DeclaredD4Delta));

	// ── ⛔ THE THREE EDITS MUST ACTUALLY HAVE MATCHED ─────────────────────────
	// A `Replace` that matched nothing returns the input unchanged, which would
	// turn everything below into the byte-equality assertion AS-§20.4 has ruled
	// FALSE — and it would pass or fail for a reason that has nothing to do with
	// what this test claims to measure. So the derivation is checked before it is
	// used, which is the same "did the guard actually run" discipline SC-§21
	// applies to guard placement.
	if (!TestEqual(TEXT("⛔ All THREE declared D4 edits matched the frozen spike fixture (if not, the fixture or the declared diff has moved and NOTHING below is meaningful)"),
		EditsApplied, 3))
	{
		AddError(TEXT("The declared D4 diff no longer applies to the frozen spike fixture. ⛔ Do NOT 'fix' this by re-copying BuildZoneA's output into the fixture - that destroys the only surviving record of the bytes `zoneA_chars=5116` was measured on. Re-derive the diff from the shipped builder BY HAND and write it out edit by edit."));
		return false;
	}

	// ── ⭐ THE RE-PURPOSED ASSERTION ──────────────────────────────────────────
	// The two lanes are EXPECTED to differ now. What is asserted is that they
	// differ by EXACTLY the ruled diff.
	const int32 Divergence = FirstDifference(ShippedLane, DeclaredShippedLane);
	if (Divergence != INDEX_NONE)
	{
		AddError(FString::Printf(
			TEXT("⛔ THE SHIPPED ZONE A IS NOT THE SPIKE LANE PLUS THE DECLARED `D4` DIFF - it first departs at character %d.\n")
			TEXT("  D4 (declared 2026-08-04, batch ASSISTANT-EXCLUDE): the SHIPPED lane gained a 44-char `WHO =` schema shape and two rule lines (+111, +153) = +308, moving it from %d to %d chars. `Plugins/SiegeLlama/**` was DELIBERATELY NOT TOUCHED (it is TASK-481's in-flight instrument, FT-§16), so the spike lane stayed at %d.\n")
			TEXT("  ⛔ `zoneA_tok = 1139` and the 77.1%% KV-reuse figure are STALE - PENDING RE-MEASUREMENT ON THE MODEL. They are NOT recomputed by arithmetic; only Siege.Llama.SpikePrompt prints them.\n")
			TEXT("  ⛔ DO NOT silence this by editing the frozen spike fixture, and DO NOT paste BuildZoneA's output into it. If Zone A was edited on purpose again, ADD the new edit to the D4 diff above, edit by edit, and re-count.\n")
			TEXT("  shipped : ...%s...\n")
			TEXT("  declared: ...%s..."),
			Divergence,
			SpikeLaneZoneAChars, ShippedZoneAChars, SpikeLaneZoneAChars,
			*Window(ShippedLane, Divergence), *Window(DeclaredShippedLane, Divergence)));
	}

	// ⛔⛔ `TestEqualSensitive`, NEVER `TestEqual`, ON EVERY STRING IN THIS FILE.
	// FAutomationTestBase::TestEqual(const FString&, const FString&) forwards to
	// the TCHAR* overload, which is CASE-INSENSITIVE — that is the entire reason
	// TestEqualSensitive exists as a separate API. A byte gate written with
	// TestEqual would pass two Zone As differing in case, i.e. it would report
	// SAFE on a prompt where `own_castle` had become `OWN_CASTLE` and the
	// tokenizer output had changed completely. That is precisely the "automated
	// guardrail that reports safe" failure CONVENTIONS §12g rules worse than
	// having no test at all, so it is called out here rather than left as a silent
	// idiom for the next editor to undo.
	TestEqualSensitive(TEXT("⭐ The shipped BuildZoneA(default vocabulary) is the frozen spike lane PLUS EXACTLY the three declared `D4` edits and nothing else"),
		ShippedLane, DeclaredShippedLane);

	// ── AND THE DIVERGENCE ITSELF IS ASSERTED, NOT MERELY TOLERATED ───────────
	// ⚠️ If the two lanes ever became byte-equal again, this test would pass
	// silently on a premise that is no longer true — either the spike was edited
	// (⛔ another batch's instrument) or the shipped rule lines were reverted (⛔
	// Jonathan's fix, gone). Both are events that must be SEEN, so `D4` is
	// asserted to still be a real, non-empty divergence.
	TestNotEqualSensitive(TEXT("⛔ `D4` IS STILL A REAL DIVERGENCE: the shipped lane and the frozen spike lane are NOT byte-equal. If they are, either the spike was edited (another batch's instrument) or TASK-521's rule lines were reverted."),
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
 *  ⚠️ THIS IS NOT REDUNDANT WITH THE TEST ABOVE, AND THE REASON IS THE ONE
 *  FAILURE MODE A DIFF-SHAPED ASSERTION CANNOT SEE: a shipped lane and a declared
 *  diff that BOTH moved by the same amount stay consistent with each other while
 *  both drift away from the number that was counted. The test above proves the
 *  shipped lane is the spike lane plus the ruled diff; this proves the two lanes
 *  sit on 5424 and 5116 respectively. Only the pair discharges the claim.
 *
 *  ⛔ THE TWO LANES NOW HAVE TWO DIFFERENT NUMBERS, AND THAT IS THE POINT —
 *  divergence `D4`, declared 2026-08-04 (AS-§20.4). ⛔ RE-COUNTED, NOT
 *  RE-DERIVED-BY-TRUST: the components of the +308 are asserted individually
 *  below from the literals in this file, so the total cannot be right by accident.
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

	TestEqual(TEXT("⭐ The SHIPPED Zone A is exactly 5424 characters — the named, dated baseline of 2026-08-04"),
		ShippedLane.Len(), ShippedZoneAChars);
	TestEqual(TEXT("⛔ The SPIKE Zone A is STILL exactly the measured 5116 characters — Plugins/SiegeLlama was deliberately NOT touched (D4, FT-§16)"),
		SpikeLane.Len(), SpikeLaneZoneAChars);

	// The byte figure is asserted separately from the char figure rather than
	// inferred from it. `MaxUtteranceBytes` was renamed by TASK-433 precisely
	// because a constant named "Chars" that measured bytes hid a 3x
	// over-admission; the same conflation is not going to be re-introduced here
	// by assumption.
	TestEqual(TEXT("The SHIPPED Zone A is exactly 5424 UTF-8 BYTES (asserted, not inferred from the char count)"),
		Utf8ByteLength(ShippedLane), ShippedZoneAChars);
	TestEqual(TEXT("The SPIKE Zone A is exactly 5116 UTF-8 BYTES"),
		Utf8ByteLength(SpikeLane), SpikeLaneZoneAChars);

	// ── ⭐ THE +308, COMPONENT BY COMPONENT ───────────────────────────────────
	// ⛔ THIS IS THE RE-COUNT, AND IT IS WHY 5424 IS NOT A NUMBER TAKEN ON TRUST
	// FROM A HANDOFF. Each figure is measured by the compiler off the literal in
	// this file's D4 block, so a transcription error in any one of them fails HERE
	// with the component named, rather than showing up as an unexplained 308 that
	// happens not to match.
	const int32 WhoLineDelta = FCString::Strlen(D4_WhoLineAfter) - FCString::Strlen(D4_WhoLineBefore);
	const int32 RuleAllUnitsLength = FCString::Strlen(D4_RuleAllUnits);
	const int32 RuleExclusionLength = FCString::Strlen(D4_RuleExclusion);

	TestEqual(TEXT("D4 component 1 — the `WHO =` schema line gains exactly 44 chars for the {\"all_except\":[KIND]} shape"), WhoLineDelta, 44);
	TestEqual(TEXT("D4 component 2 — the `who`:\"all\" rule line is exactly 111 chars"), RuleAllUnitsLength, 111);
	TestEqual(TEXT("D4 component 3 — the exclusion rule line is exactly 153 chars"), RuleExclusionLength, 153);
	TestEqual(TEXT("⭐ The three components sum to the declared D4 delta of 308"),
		WhoLineDelta + RuleAllUnitsLength + RuleExclusionLength, DeclaredD4Delta);

	// ⛔ THE BATCH-LEVEL BUDGET, ASSERTED RATHER THAN TRUSTED (AS-§20.4). The
	// Zone-A growth ceiling for the WHOLE batch is ~325 characters, derived at
	// §12g's pinned worst marginal rate from `zoneA_tok <= 1389` with
	// `ContextTokens` frozen at 2048. ⛔ Over budget ⇒ STOP and escalate; it is not
	// a soft target, and the next Zone-A edit has only the remainder to spend.
	static constexpr int32 ZoneAGrowthBudgetChars = 325;
	TestTrue(*FString::Printf(TEXT("⛔ The batch's Zone A growth (%d chars) is within AS-§20.4's %d-char ceiling — %d chars remain for any FUTURE Zone A edit"),
		DeclaredD4Delta, ZoneAGrowthBudgetChars, ZoneAGrowthBudgetChars - DeclaredD4Delta),
		DeclaredD4Delta <= ZoneAGrowthBudgetChars);

	AddInfo(FString::Printf(
		TEXT("⛔ TOKENS ARE NOT RE-COUNTED HERE AND MUST NOT BE. `zoneA_tok = 1139` and the 77.1%% KV-reuse figure are STALE - PENDING RE-MEASUREMENT ON THE MODEL (AS-§20.4). Characters are countable offline; tokens carry a MEASURED -15/+4 error band with UNPREDICTABLE SIGN (§12g), so converting %d chars into a token delta would be a derivation wearing a measurement's authority. Only Siege.Llama.SpikePrompt prints the real figure."),
		DeclaredD4Delta));

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
	//
	// ⚠️⚠️ THIS TEST WAS A THIRD CASUALTY OF THE ZONE-A EDIT AND NOBODY LISTED IT
	// (TASK-523). AS-§20.4, the board and every handoff name TWO broken tests —
	// TwoLaneByteEquality and MeasuredCharCount. This one breaks too, silently and
	// for a DIFFERENT reason: its expectation is DERIVED from the Zone-A char
	// constant, so re-basing that constant without reading this line would have
	// left a red bar with no owner and a message pointing at the vocabulary, which
	// is not what moved.
	//
	// ⛔ THE BASE IS THE SHIPPED FIGURE, NOT THE SPIKE'S. `NullLane` is
	// `Snapshot->BuildZoneA(nullptr)` — the SHIPPED builder — so its base is the
	// SHIPPED length. Using SpikeLaneZoneAChars here would compile, run, and be
	// wrong by exactly 308 (SC-§23: a measurement's BASE is part of the
	// measurement, and a varying value in the right field is still wrong if its
	// base is wrong).
	const FString SynonymTable = Vocabulary->BuildSynonymTable();
	const int32 ExpectedNullLength = ShippedZoneAChars - SynonymTable.Len() + 5; // 5 == Len("none\n")
	TestEqual(TEXT("The null-vocabulary Zone A is the SHIPPED lane with the synonym table swapped for `none`"),
		NullLane.Len(), ExpectedNullLength);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
