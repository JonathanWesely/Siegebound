// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Siegebound/SiegeAssistantCommand.h"
#include "Siegebound/SiegeAssistantSnapshot.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for the NON-ORDERABLE-KIND GUARD and the orderability
 *  accessors (batch SETTINGS+CONFIRM, TASK-441; CONVENTIONS "Settings screen +
 *  the assistant CONFIRM STEP + the non-orderable-kind guard (2026-08-03)"
 *  §6, §8).
 *
 *  ── WHAT THESE TESTS ARE FOR, AND THE HONEST LIMIT FIRST ──
 *
 *  ⛔ THE GUARD IS SHIPPED SAFETY AND IS **NOT** A ROUTE TO THE EVAL GATE. The
 *  eval scores the model's EMITTED JSON; the guard refuses an EXECUTED ACTION.
 *  It does not make `DEV-04` pass and it is not progress on bar #5. Nothing in
 *  this file measures accuracy, and no result here may be reported as accuracy.
 *
 *  ✅ WHAT THEY DO PROVE: that a well-formed command naming a unit the live
 *  board cannot be ordered to move is REFUSED BEFORE EXECUTION, INDEPENDENTLY OF
 *  THE MODEL. That is the property §12f measured as missing - `DEV-04` emitted
 *  `{"intent":"send","who":[{"kind":"sapper","n":1}],"where":"enemy_castle"}`
 *  three times out of three, for a kind the roster line itself prints as
 *  `orderable=0`, and the command layer executed it.
 *
 *  ⚠️ AND THE REASON THIS FILE CAN EXIST AT ALL IS THE SIGNATURE'S SHAPE.
 *  ValidateCommandAgainstSnapshot takes the ROSTER ARRAY, not the snapshot
 *  object - so every test below runs with NO MODEL RESIDENT, NO GGUF, NO UWorld,
 *  NO Capture() and NO PIE, against a hand-populated roster. A later "tidy-up"
 *  that changes the parameter to `const USiegeAssistantSnapshot*` deletes that
 *  property and every test in this file with it.
 *
 *  ⚠️ WHAT IS **NOT** COVERED HERE, STATED SO NOBODY READS MORE INTO A GREEN RUN:
 *   - Nothing in this file has been COMPILED at authoring time (the batch has one
 *     compile gate, TASK-447, under the quiet-module law), so "these tests pass"
 *     is not a claim anyone may make yet - only "these tests are written".
 *   - The roster used here is HAND-POPULATED. It asserts the guard's behaviour
 *     given a roster; it does not assert that Capture() fills `Orderable`
 *     correctly on a live board - that needs a world and is TASK-447's
 *     first-execution audit plus TASK-448's playtest.
 *   - Capture(nullptr, ...) is deliberately NOT exercised: it logs at Warning,
 *     and the automation framework treats a logged warning as a failure. The
 *     empty-snapshot accessor test covers the same null-safety surface without
 *     provoking it.
 */

namespace SiegeAssistantGuardTestUtils
{
	/** Readable enum names, so a failure message says which refusal happened rather than which integer. */
	static FString ReasonToString(ESiegeAssistantRejectReason Reason)
	{
		switch (Reason)
		{
		case ESiegeAssistantRejectReason::None:             return TEXT("None");
		case ESiegeAssistantRejectReason::KindNotOrderable:  return TEXT("KindNotOrderable");
		case ESiegeAssistantRejectReason::KindUnknown:       return TEXT("KindUnknown");
		default:                                             return TEXT("<unpinned>");
		}
	}

	static FSiegeAssistantRosterEntry MakeRow(const TCHAR* Kind, int32 Count, int32 Orderable, int32 GroupId = INDEX_NONE)
	{
		FSiegeAssistantRosterEntry Row;
		Row.Kind = FName(Kind);
		Row.Count = Count;
		Row.Orderable = Orderable;
		Row.GroupId = GroupId;
		return Row;
	}

	/**
	 *  A PLAUSIBLE MID-MATCH BLUE BOARD, and every row is the shape a real
	 *  Capture() produces on this project's shipped eligibility rules:
	 *
	 *   - `footman` - Standard. Zone-orderable. TWO ROWS on purpose (one in a
	 *     group, one ungrouped), because the roster aggregates by (Kind, GroupId)
	 *     and a guard that reads only the first matching row would be wrong.
	 *   - `archer`  - Standard. Zone-orderable.
	 *   - `cleric`  - Support. FOLLOWS but CANNOT take zone orders (the shipped
	 *     Cleric ruling), so `Orderable == 0` with `Count > 0`. This row is what
	 *     makes the Follow-scoping test a real regression test.
	 *   - `sapper`  - Siege. Neither follows nor takes zone orders, so
	 *     `Orderable == 0`. THIS IS THE DEV-04 ROW.
	 *   - `catapult` is absent from the board and from the deck - it is the unit
	 *     Jonathan's utterance named and does not own.
	 */
	static TArray<FSiegeAssistantRosterEntry> MidMatchRoster()
	{
		return TArray<FSiegeAssistantRosterEntry>
		{
			MakeRow(TEXT("footman"), 6, 6, /*GroupId*/ 3),
			MakeRow(TEXT("footman"), 4, 4),
			MakeRow(TEXT("archer"),  3, 3),
			MakeRow(TEXT("cleric"),  2, 0),
			MakeRow(TEXT("sapper"),  1, 0)
		};
	}

	/** One selection-bearing command. Counts are the player's REQUEST and are deliberately never checked by the guard. */
	static FSiegeAssistantCommand MakeCommand(ESiegeAssistantIntent Intent, const TArray<FName>& Kinds, const TCHAR* Where = TEXT("enemy_castle"))
	{
		FSiegeAssistantCommand Command;
		Command.Intent = Intent;
		Command.Kinds = Kinds;
		Command.Counts.Reserve(Kinds.Num());
		for (int32 Index = 0; Index < Kinds.Num(); ++Index)
		{
			Command.Counts.Add(1);
		}
		Command.Where = FName(Where);
		return Command;
	}

	/** The three zone verbs the ORDERABLE column gates. Follow is deliberately NOT in this list - see the Follow test. */
	static TArray<ESiegeAssistantIntent> ZoneOrderIntents()
	{
		return TArray<ESiegeAssistantIntent>
		{
			ESiegeAssistantIntent::Send,
			ESiegeAssistantIntent::Guard,
			ESiegeAssistantIntent::Ambush
		};
	}

	static FString IntentToString(ESiegeAssistantIntent Intent)
	{
		switch (Intent)
		{
		case ESiegeAssistantIntent::Send:     return TEXT("Send");
		case ESiegeAssistantIntent::Guard:    return TEXT("Guard");
		case ESiegeAssistantIntent::Ambush:   return TEXT("Ambush");
		case ESiegeAssistantIntent::Follow:   return TEXT("Follow");
		case ESiegeAssistantIntent::Charge:   return TEXT("Charge");
		case ESiegeAssistantIntent::Fallback: return TEXT("Fallback");
		case ESiegeAssistantIntent::Rally:    return TEXT("Rally");
		default:                              return TEXT("None");
		}
	}
}

// ---------------------------------------------------------------------------
// 1. THE PASS CASE - a kind with Orderable > 0 is not refused
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardOrderableKindPassesTest,
	"Siegebound.Assistant.Guard.OrderableKindPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardOrderableKindPassesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> Roster = MidMatchRoster();

	for (const ESiegeAssistantIntent Intent : ZoneOrderIntents())
	{
		// Pre-dirty both outs so a function that forgot to write them is caught
		// rather than accidentally passing on a zero-initialised local.
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindUnknown;
		FName Offending = FName(TEXT("stale"));

		const FSiegeAssistantCommand Command = MakeCommand(Intent, { FName(TEXT("footman")) });
		const bool bAllowed = ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending);

		TestTrue(*FString::Printf(TEXT("%s of an orderable kind is allowed"), *IntentToString(Intent)), bAllowed);
		TestEqualSensitive(*FString::Printf(TEXT("%s of an orderable kind reports no reason"), *IntentToString(Intent)),
			ReasonToString(Reason), FString(TEXT("None")));
		TestTrue(*FString::Printf(TEXT("%s of an orderable kind names no offender"), *IntentToString(Intent)),
			Offending.IsNone());
	}

	// A multi-kind selection where EVERY kind is orderable - the feature's own
	// flagship sentence shape ("10 footmen with a sorcerer"), here with two kinds
	// the fixture board actually fields.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindUnknown;
		FName Offending = FName(TEXT("stale"));

		const FSiegeAssistantCommand Command =
			MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")), FName(TEXT("archer")) });

		TestTrue(TEXT("A multi-kind selection of orderable kinds is allowed"),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
		TestTrue(TEXT("...and names no offender"), Offending.IsNone());
	}

	return true;
}

// ---------------------------------------------------------------------------
// 2. THE DEV-04 REPRODUCTION - present on the board, ZERO orderable
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardNonOrderableKindRefusedTest,
	"Siegebound.Assistant.Guard.NonOrderableKindRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardNonOrderableKindRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> Roster = MidMatchRoster();

	// THE MEASURED CASE, VERBATIM. Jonathan typed "send the catapults at the
	// enemy base"; the model emitted a live order for `sapper` at
	// `enemy_castle`, identically on all three runs, while the roster line it was
	// shown printed sapper as `orderable=0`. The guard refuses it BEFORE
	// execution and WITHOUT the model. It does NOT make that eval row pass -
	// the eval scores the emitted JSON, which is unchanged.
	for (const ESiegeAssistantIntent Intent : ZoneOrderIntents())
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = FName(TEXT("stale"));

		const FSiegeAssistantCommand Command = MakeCommand(Intent, { FName(TEXT("sapper")) });
		const bool bAllowed = ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending);

		TestFalse(*FString::Printf(TEXT("%s of a 0-orderable kind is REFUSED"), *IntentToString(Intent)), bAllowed);
		TestEqualSensitive(*FString::Printf(TEXT("%s of a 0-orderable kind reports KindNotOrderable"), *IntentToString(Intent)),
			ReasonToString(Reason), FString(TEXT("KindNotOrderable")));
		TestEqualSensitive(*FString::Printf(TEXT("%s of a 0-orderable kind NAMES the offending kind"), *IntentToString(Intent)),
			Offending.ToString(), FString(TEXT("sapper")));
	}

	// KindNotOrderable is NOT KindUnknown, and the distinction is the entire
	// reason the roster row carries an orderable column: the sapper IS on the
	// board. Confusing the two would produce a refusal that tells the player
	// something false about their own army.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;
		ValidateCommandAgainstSnapshot(
			MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("sapper")) }), Roster, Reason, Offending);

		TestFalse(TEXT("A present-but-ineligible kind is NOT reported as unknown"),
			ReasonToString(Reason) == FString(TEXT("KindUnknown")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 3. A KIND ABSENT ENTIRELY
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardUnknownKindRefusedTest,
	"Siegebound.Assistant.Guard.UnknownKindRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardUnknownKindRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> Roster = MidMatchRoster();

	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		const FSiegeAssistantCommand Command = MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("catapult")) });

		TestFalse(TEXT("A kind that is not on the board is REFUSED"),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...as KindUnknown"), ReasonToString(Reason), FString(TEXT("KindUnknown")));
		TestEqualSensitive(TEXT("...naming the offending kind"), Offending.ToString(), FString(TEXT("catapult")));
	}

	// The unknown-kind check is NOT limited to the zone verbs. A hallucinated
	// unit is a hallucinated unit whatever verb carries it, and there is no
	// eligibility column that could rescue a kind with no units at all.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		TestFalse(TEXT("Follow of an absent kind is REFUSED too"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Follow, { FName(TEXT("catapult")) }, TEXT("hero")),
				Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...as KindUnknown"), ReasonToString(Reason), FString(TEXT("KindUnknown")));
	}

	// A row that exists with zero live units is "not on the board", not
	// "present". Capture() never emits such a row, so this pins the behaviour for
	// a hand-populated or (in M8 P2) wire-received roster.
	{
		const TArray<FSiegeAssistantRosterEntry> ZeroCountRoster{ MakeRow(TEXT("footman"), 0, 0) };

		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		TestFalse(TEXT("A zero-count row does not count as present"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")) }),
				ZeroCountRoster, Reason, Offending));
		TestEqualSensitive(TEXT("...and reports KindUnknown"), ReasonToString(Reason), FString(TEXT("KindUnknown")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 4. MULTI-KIND: ONE BAD KIND REFUSES THE WHOLE COMMAND
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardMultiKindRefusedAsAWholeTest,
	"Siegebound.Assistant.Guard.MultiKindRefusedAsAWhole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardMultiKindRefusedAsAWholeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> Roster = MidMatchRoster();

	// ⛔ THE LAW THIS TEST EXISTS FOR: a multi-kind order with one illegal kind is
	// refused AS A WHOLE. It is never partially executed and the bad kind is
	// never silently dropped - a player who asked for footmen AND a sapper and
	// silently got only footmen was answered wrongly, confidently, which is the
	// valid-shaped-wrong-command failure this architecture exists to prevent.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		const FSiegeAssistantCommand Command =
			MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")), FName(TEXT("sapper")) });

		TestFalse(TEXT("Good kind FIRST, bad kind second: the whole command is refused"),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...as KindNotOrderable"), ReasonToString(Reason), FString(TEXT("KindNotOrderable")));
		TestEqualSensitive(TEXT("...naming the BAD kind, not the good one"), Offending.ToString(), FString(TEXT("sapper")));
	}

	// Order-independence of the verdict: the bad kind first refuses just as hard.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		const FSiegeAssistantCommand Command =
			MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("sapper")), FName(TEXT("footman")) });

		TestFalse(TEXT("Bad kind FIRST: the whole command is refused"),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...naming the bad kind"), Offending.ToString(), FString(TEXT("sapper")));
	}

	// TWO offenders of DIFFERENT classes: the reported one is the FIRST in Kinds
	// order, deterministically. Pinned so a later "report the worst one" change is
	// a deliberate decision rather than an accident nobody noticed.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		const FSiegeAssistantCommand Command =
			MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("sapper")), FName(TEXT("catapult")) });

		TestFalse(TEXT("Two offenders: still refused"),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("The FIRST offender in Kinds order is the one reported"),
			Offending.ToString(), FString(TEXT("sapper")));
		TestEqualSensitive(TEXT("...with ITS reason, not the later kind's"),
			ReasonToString(Reason), FString(TEXT("KindNotOrderable")));
	}

	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		const FSiegeAssistantCommand Command =
			MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("catapult")), FName(TEXT("sapper")) });

		ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending);
		TestEqualSensitive(TEXT("Reversed: the first offender is the absent kind"),
			Offending.ToString(), FString(TEXT("catapult")));
		TestEqualSensitive(TEXT("...with KindUnknown"), ReasonToString(Reason), FString(TEXT("KindUnknown")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 5. THE EMPTY ROSTER
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardEmptyRosterTest,
	"Siegebound.Assistant.Guard.EmptyRoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardEmptyRosterTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> EmptyRoster;

	// An empty roster is the state a null-world or match-start Capture() leaves
	// behind. Every named kind is unknown - and the guard must not crash, index
	// off the end, or fall through to "allowed".
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		TestFalse(TEXT("Any named kind against an empty roster is refused"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")) }),
				EmptyRoster, Reason, Offending));
		TestEqualSensitive(TEXT("...as KindUnknown"), ReasonToString(Reason), FString(TEXT("KindUnknown")));
		TestEqualSensitive(TEXT("...naming the kind"), Offending.ToString(), FString(TEXT("footman")));
	}

	// ...but an army-wide verb against an empty roster still has no kind to be
	// wrong about, so it is NOT refused here. Whether there is anything to
	// command is the executor's and the FSM's shortfall question, not this
	// guard's.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindUnknown;
		FName Offending = FName(TEXT("stale"));

		FSiegeAssistantCommand Charge;
		Charge.Intent = ESiegeAssistantIntent::Charge;

		TestTrue(TEXT("An army-wide verb against an empty roster is not refused"),
			ValidateCommandAgainstSnapshot(Charge, EmptyRoster, Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
		TestTrue(TEXT("...and names no offender"), Offending.IsNone());
	}

	return true;
}

// ---------------------------------------------------------------------------
// 6. ARMY-WIDE VERBS AND `who:"none"` / `who:"all"` HAVE NO KIND TO VALIDATE
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardArmyWideVerbsNotRefusedTest,
	"Siegebound.Assistant.Guard.ArmyWideVerbsNotRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardArmyWideVerbsNotRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> Roster = MidMatchRoster();

	// `who:"none"` (army-wide) and `who:"all"` (every eligible unit) BOTH parse to
	// an empty Kinds/Counts pair - so the empty-selection path is the one that
	// carries both, and refusing it would break `charge`, `fallback` and `rally`
	// outright.
	const TArray<ESiegeAssistantIntent> ArmyWide
	{
		ESiegeAssistantIntent::Charge,
		ESiegeAssistantIntent::Fallback,
		ESiegeAssistantIntent::Rally
	};

	for (const ESiegeAssistantIntent Intent : ArmyWide)
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindNotOrderable;
		FName Offending = FName(TEXT("stale"));

		FSiegeAssistantCommand Command;
		Command.Intent = Intent;
		Command.Where = FName(TEXT("enemy_castle"));

		TestTrue(*FString::Printf(TEXT("%s with no selection is not refused"), *IntentToString(Intent)),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(*FString::Printf(TEXT("%s reports no reason"), *IntentToString(Intent)),
			ReasonToString(Reason), FString(TEXT("None")));
		TestTrue(*FString::Printf(TEXT("%s names no offender"), *IntentToString(Intent)),
			Offending.IsNone());
	}

	// A SELECTION-BEARING verb with an empty selection is `who:"all"` - "send
	// everything at the castle". Also nothing to validate.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindNotOrderable;
		FName Offending = FName(TEXT("stale"));

		FSiegeAssistantCommand SendAll;
		SendAll.Intent = ESiegeAssistantIntent::Send;
		SendAll.Where = FName(TEXT("enemy_castle"));

		TestTrue(TEXT("Send with who:\"all\" (an empty selection) is not refused"),
			ValidateCommandAgainstSnapshot(SendAll, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 7. FOLLOW IS NOT GATED BY THE ORDERABLE COLUMN
//    (the regression this guard is most able to cause)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardFollowNotGatedByOrderableTest,
	"Siegebound.Assistant.Guard.FollowIsNotGatedByTheOrderableColumn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardFollowNotGatedByOrderableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	const TArray<FSiegeAssistantRosterEntry> Roster = MidMatchRoster();

	// ⚠️ THE CLERIC IS THE COUNTER-EXAMPLE THAT DECIDES THE GUARD'S SHAPE.
	// IsGroupCommandEligible() covers the ZONE orders only, and the shipped
	// Cleric ruling is "follows, cannot take zone orders" - so a Cleric row is
	// Count > 0 with Orderable == 0. A guard that refused every 0-orderable kind
	// for every verb would refuse "clerics follow me", which is a legal shipped
	// order AND is verbatim the eval's own DEV-20 utterance. That would be a
	// regression shipped under the name of a safety fix.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindNotOrderable;
		FName Offending = FName(TEXT("stale"));

		const FSiegeAssistantCommand Command =
			MakeCommand(ESiegeAssistantIntent::Follow, { FName(TEXT("cleric")) }, TEXT("hero"));

		TestTrue(TEXT("\"clerics follow me\" is NOT refused by the orderable column"),
			ValidateCommandAgainstSnapshot(Command, Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
		TestTrue(TEXT("...and names no offender"), Offending.IsNone());
	}

	// The same Cleric under a ZONE verb IS refused - which is the other half of
	// the same rule and proves the verb is what selects the column, not the kind.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		TestFalse(TEXT("A cleric under a ZONE verb is refused"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("cleric")) }),
				Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...as KindNotOrderable"), ReasonToString(Reason), FString(TEXT("KindNotOrderable")));
		TestEqualSensitive(TEXT("...naming the cleric"), Offending.ToString(), FString(TEXT("cleric")));
	}

	// ⚠️ THE DECLARED v1 GAP, ASSERTED SO IT IS A DECISION AND NOT A DISCOVERY.
	// A Follow order naming a kind that cannot follow either (the Sapper) is NOT
	// refused here: the pinned ESiegeAssistantRejectReason has no
	// KindNotFollowable value and TASK-443 has no template for one. It is not an
	// open hole - the executor's selector filters on IsFollowCommandEligible()
	// and the order arrives at the SHORTFALL path instead. If a
	// KindNotFollowable reason is ever pinned, THIS ASSERTION IS THE ONE THAT
	// MUST FLIP, and it is written this way so that it does.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindNotOrderable;
		FName Offending = FName(TEXT("stale"));

		TestTrue(TEXT("v1 SCOPE: Follow naming a non-followable kind is not refused by THIS guard"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Follow, { FName(TEXT("sapper")) }, TEXT("hero")),
				Roster, Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 8. THE OUT-PARAMS ARE ALWAYS WRITTEN, AND ROWS ARE SUMMED ACROSS GROUPS
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantGuardContractDetailsTest,
	"Siegebound.Assistant.Guard.ContractDetails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantGuardContractDetailsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantGuardTestUtils;

	// (a) BOTH OUT-PARAMS ARE WRITTEN ON THE SUCCESS PATH. An untouched reason
	// code is a stale refusal from the previous sentence, and a template filled
	// from a stale code tells the player something true about a command they are
	// no longer giving.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindNotOrderable;
		FName Offending = FName(TEXT("stale_from_the_last_sentence"));

		TestTrue(TEXT("Success path returns true"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("archer")) }),
				MidMatchRoster(), Reason, Offending));
		TestEqualSensitive(TEXT("Success RESETS the reason - it is never left stale"),
			ReasonToString(Reason), FString(TEXT("None")));
		TestTrue(TEXT("Success RESETS the offending kind - it is never left stale"), Offending.IsNone());
	}

	// (b) A KIND SPREAD OVER SEVERAL GROUPS IS SUMMED. The roster aggregates by
	// (Kind, GroupId), so one symbol legitimately occupies several rows. A guard
	// that answered off the FIRST matching row would refuse a kind whose only
	// orderable units sit in a later one - a false refusal that would look
	// exactly like a model failure.
	{
		const TArray<FSiegeAssistantRosterEntry> SplitRoster
		{
			MakeRow(TEXT("footman"), 2, 0, /*GroupId*/ 1),   // frozen/ineligible slice, listed first
			MakeRow(TEXT("footman"), 5, 5, /*GroupId*/ 2)
		};

		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindUnknown;
		FName Offending = FName(TEXT("stale"));

		TestTrue(TEXT("Orderable units in a LATER row still allow the order"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")) }),
				SplitRoster, Reason, Offending));
		TestEqualSensitive(TEXT("...and report no reason"), ReasonToString(Reason), FString(TEXT("None")));
	}

	// ...and the mirror: every row zero means every row zero.
	{
		const TArray<FSiegeAssistantRosterEntry> AllIneligible
		{
			MakeRow(TEXT("footman"), 2, 0, /*GroupId*/ 1),
			MakeRow(TEXT("footman"), 5, 0, /*GroupId*/ 2)
		};

		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::None;
		FName Offending = NAME_None;

		TestFalse(TEXT("No orderable unit in ANY row refuses the order"),
			ValidateCommandAgainstSnapshot(
				MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")) }),
				AllIneligible, Reason, Offending));
		TestEqualSensitive(TEXT("...as KindNotOrderable, because the units ARE on the board"),
			ReasonToString(Reason), FString(TEXT("KindNotOrderable")));
	}

	// (c) COUNTS ARE NOT THIS GUARD'S BUSINESS. "You asked for 10 and 8 exist" is
	// the shortfall/clarification path. Clamping or refusing here would make that
	// clarification undetectable - the defect CONVENTIONS §1 names when it says
	// to constrain identity hard and leave quantity soft.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindUnknown;
		FName Offending = FName(TEXT("stale"));

		FSiegeAssistantCommand Overdraft = MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("archer")) });
		Overdraft.Counts[0] = 30;   // the board fields 3

		TestTrue(TEXT("An over-count request is NOT refused by this guard"),
			ValidateCommandAgainstSnapshot(Overdraft, MidMatchRoster(), Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
	}

	// (d) `TriggerKind` IS NOT VALIDATED, AND THAT IS CORRECT. A deferred intent
	// means "fire once at least N of these EXIST", so a trigger kind absent RIGHT
	// NOW is the whole point of waiting. Validating it as a who[] entry would
	// refuse every deferred order that was doing its job.
	{
		ESiegeAssistantRejectReason Reason = ESiegeAssistantRejectReason::KindUnknown;
		FName Offending = FName(TEXT("stale"));

		FSiegeAssistantCommand Deferred = MakeCommand(ESiegeAssistantIntent::Send, { FName(TEXT("footman")) });
		Deferred.TriggerKind = FName(TEXT("catapult"));   // absent from the board, on purpose
		Deferred.TriggerAtLeast = 3;

		TestTrue(TEXT("A trigger kind that is absent RIGHT NOW does not refuse the order"),
			ValidateCommandAgainstSnapshot(Deferred, MidMatchRoster(), Reason, Offending));
		TestEqualSensitive(TEXT("...and reports no reason"), ReasonToString(Reason), FString(TEXT("None")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 9. THE ACCESSORS ON A SNAPSHOT THAT HAS NEVER CAPTURED
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSnapshotOrderabilityAccessorsTest,
	"Siegebound.Assistant.Snapshot.OrderabilityAccessors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSnapshotOrderabilityAccessorsTest::RunTest(const FString& Parameters)
{
	// An un-captured snapshot is the state the FSM holds before the first
	// sentence and the state a failed survey leaves behind. Both accessors must
	// answer honestly and must not index off the end of an empty array.
	//
	// ⚠️ Capture(nullptr, ...) is deliberately NOT called: it logs at Warning by
	// design, and the automation framework treats a logged warning as a test
	// failure. This covers the same null-safety surface without provoking it.
	USiegeAssistantSnapshot* const Snapshot = NewObject<USiegeAssistantSnapshot>();
	TestNotNull(TEXT("The snapshot object was created"), Snapshot);
	if (!Snapshot)
	{
		return false;
	}

	TestEqual(TEXT("An un-captured snapshot reports 0 orderable for a real kind"),
		Snapshot->GetOrderableCount(FName(TEXT("footman"))), 0);
	TestFalse(TEXT("...and IsKindOrderable agrees"),
		Snapshot->IsKindOrderable(FName(TEXT("footman"))));

	TestEqual(TEXT("An un-captured snapshot reports 0 orderable for an invented kind"),
		Snapshot->GetOrderableCount(FName(TEXT("catapult"))), 0);
	TestFalse(TEXT("...and IsKindOrderable agrees"),
		Snapshot->IsKindOrderable(FName(TEXT("catapult"))));

	TestEqual(TEXT("NAME_None is not orderable and does not crash"),
		Snapshot->GetOrderableCount(NAME_None), 0);
	TestFalse(TEXT("...and IsKindOrderable agrees"), Snapshot->IsKindOrderable(NAME_None));

	TestEqual(TEXT("An un-captured snapshot has an empty roster"), Snapshot->GetRoster().Num(), 0);

	// The two accessors are one answer, not two: IsKindOrderable is defined as
	// GetOrderableCount() > 0, and a later edit that gives them separate bodies
	// is exactly how they drift apart.
	TestTrue(TEXT("IsKindOrderable is GetOrderableCount() > 0, by construction"),
		Snapshot->IsKindOrderable(FName(TEXT("archer")))
			== (Snapshot->GetOrderableCount(FName(TEXT("archer"))) > 0));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
