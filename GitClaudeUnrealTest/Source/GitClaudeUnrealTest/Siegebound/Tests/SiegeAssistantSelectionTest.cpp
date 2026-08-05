// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Siegebound/SiegeAssistantCommand.h"
#include "Siegebound/SiegeAssistantGrammar.h"
#include "Siegebound/SiegeAssistantSnapshot.h"
#include "Siegebound/SiegeKeyboardLayoutStatics.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ────────────────────────────────────────────────────────────────────────────
 *  THE EXCLUSION + ROSTER-VISIBILITY MEASUREMENT (batch ASSISTANT-EXCLUDE,
 *  TASK-523 — CONVENTIONS AS-§20, especially AS-§20.2, AS-§20.4, AS-§20.6 and
 *  the AS-§20.7 naming law that pins this file's PATH, its NAMESPACE
 *  (`Siegebound.Assistant.Selection.<Name>`) and its QA gate (TASK-525)).
 *  ────────────────────────────────────────────────────────────────────────────
 *
 *  M8 DECLARATION DUTY (stated verbatim as required):
 *  "adds no replicated property, no new replicated class, no new relevancy tier."
 *
 *  ── ⭐ WHY THIS FILE IS THE ONLY INSTRUMENT FOR JONATHAN'S SORCERER DEFECT ──
 *
 *  He reported: "whenever I say all units, it doesn't seem to include sorcerers
 *  even when they were spawned." AS-§20.2 found the cause in the PROMPT: the
 *  roster is elastic, `Sorcerer` is the LAST DT_Cards commandable row, so it is
 *  ALWAYS the first kind the trimmer collapses — and the collapse line used to
 *  print COUNTS WITH NO SYMBOLS (`other_kinds: 5 kinds, 9 units`). The token
 *  `sorcerer` therefore never reached the model, and Zone A's "if the unit named
 *  is not a kind in [FORCES], answer unsupported" rule then refused a unit that
 *  was standing on the board.
 *
 *  ⛔ THE EVALUATION CORPUS STRUCTURALLY CANNOT ASSERT THIS, AND TASK-524 HIT
 *  THAT WALL AND REPORTED IT (its finding F-1). The correct model answer to an
 *  "all units" order is `who:"all"`, which parses to an EMPTY kinds array; an
 *  empty expectation cell means "NOT ASSERTED" (AS-§11's known limitation), and
 *  filling the cell with 13 kinds would make the CORRECT answer fail. TASK-524's
 *  finding F-5 goes further: the eval lane hardcodes fixture `t0`, whose roster
 *  prints all 13 kinds UNCONDITIONALLY, so THE COLLAPSE THAT HID THE SORCERER
 *  HAS NEVER HAPPENED ON THAT LANE. A green corpus row is therefore not evidence
 *  about the collapse leg at all.
 *
 *  ⇒ AS-§20.6 names the three instruments this batch actually owes, and they are
 *    all here: (a) a >8-kind board PRINTS `sorcerer`; (b) the COLLAPSE path
 *    prints the collapsed kinds BY NAME; (c) parser tests for every exclusion
 *    accept/refuse case. ⛔ "A repair with no failing test to close is not a
 *    repair" — before this file, both of Jonathan's reported defects were
 *    unmeasurable.
 *
 *  ── ⛔ THE ASSERTIONS RUN AGAINST THE SHIPPED BUILDER, NOT AGAINST A COPY ──
 *
 *  Every roster claim below is made against `USiegeAssistantSnapshot::BuildZoneC`
 *  and, through it, the private `AppendRosterBlock` — the real elastic trimmer,
 *  the real fixed-key order, the real collapse line. ⛔ NOTHING here transcribes
 *  the builder's output into a fixture and compares the builder to it: that is
 *  the "guardrail that reports SAFE" shape AS-§12g rules worse than no test, and
 *  it is the exact trap the sibling ZoneA file's own comments rail against.
 *
 *  ⚠️ HOW THE ROSTER IS POPULATED WITHOUT A WORLD, AND WHY IT IS HONEST.
 *  `UnitKinds` / `KindTotals` / `KindOrderable` / `KindFollowable` / `PlaceNames`
 *  are PRIVATE members filled only by `Capture(UWorld*, ETeamId)`, and TASK-523's
 *  spec forbids building a world fixture ("that is a different, larger task and
 *  it is not authorised here"). They are, however, every one of them
 *  `UPROPERTY(Transient)`, so this file writes them THROUGH THE REFLECTION
 *  SYSTEM — the same values `Capture()` would have written, into the same fields,
 *  after which the SHIPPED builder runs unmodified.
 *
 *  ⛔ AND THE FAILURE MODE OF THAT TECHNIQUE IS GUARDED RATHER THAN ASSUMED: a
 *  renamed or retyped field makes `FindFProperty` return null, which would leave
 *  the snapshot EMPTY and let a collapse assertion pass VACUOUSLY. So every
 *  reflection write is checked, the inner property TYPE is checked too, and a
 *  miss is an explicit AddError naming the field. A test that silently stops
 *  testing is the thing this whole batch exists to prevent.
 *
 *  ⚠️ WHAT THIS FILE CANNOT REACH, STATED UP FRONT RATHER THAN LEFT TO BE
 *  DISCOVERED (TASK-523 spec item 7, and TASK-522's handoff says the same thing
 *  from the other side):
 *   - THE EXECUTOR'S EXCLUSION FILTER. `SelectUnitsForOrder` walks a `UWorld`
 *     for `ASummonedUnit` actors. These are EditorContext SIMPLE tests: no world,
 *     no actors. Executor exclusion is covered by TASK-522's three log lines
 *     (subtracted / subtracted-nobody / emptied-and-refused) and by Jonathan at
 *     TASK-527 — ⛔ NOT by a test, and this file does not pretend otherwise.
 *   - `NativeOnPreviewKeyDown`. It is `protected`, its whole gate
 *     (`bConsoleOpen` / `bConsoleEnabled` / `bConfirmPromptVisible`) is
 *     `private`, and reaching those states needs a player controller, Slate
 *     focus and a live FSM. What IS reachable is the ACCEPT-KEY RESOLUTION the
 *     handler compares against, and that is asserted below.
 *   - THE `Cancelled` TRANSCRIPT LINE (TASK-520). `NotifyConsoleClosed` only
 *     prints it from `AwaitConfirm`, and `AwaitConfirm` is entered only through
 *     the private `EnterAwaitConfirm()` at the end of a full model turn. There is
 *     no headless path into that state.
 *
 *  ⛔⛔ `TestEqualSensitive`, NEVER `TestEqual`, FOR EVERY FString CLAIM.
 *  `FAutomationTestBase::TestEqual(const FString&, const FString&)` forwards to
 *  the TCHAR* overload, which is CASE-INSENSITIVE (SC-§13). A shipped, QA-passed
 *  test in this project once asserted nothing for exactly that reason. Symbol
 *  claims here are about BYTES the model reads, so case is load-bearing:
 *  `sorcerer` and `Sorcerer` tokenize differently.
 */

namespace SiegeAssistantSelectionTestFixture
{
	// ═══════════════════════════════════════════════════════════════════════════
	//  THE BOARD
	// ═══════════════════════════════════════════════════════════════════════════

	/**
	 *  THE 13 COMMANDABLE KINDS, IN DT_Cards ROW ORDER — read off Docs/Data/cards.csv
	 *  and lower-cased exactly as `USiegeAssistantSnapshot`'s `CanonicalKind` does
	 *  (`FName(*CardID.ToString().ToLower())`).
	 *
	 *  ⛔ THE ORDER IS THE TEST, NOT DECORATION. `Sorcerer` is the LAST commandable
	 *  row in the table, which is the entire mechanism of Jonathan's defect: the
	 *  shrink loop always drops from the TAIL, so the Sorcerer is the first kind
	 *  hidden on every board, every time. Sorting this array alphabetically (or
	 *  "tidying" it) would move `sorcerer` to the middle and quietly convert the
	 *  collapse tests into tests of a case that cannot happen.
	 *
	 *  ⚠️ The buildings and spells between these rows (ArrowTower, Wall, BombTower,
	 *  Barracks, the spell cards…) are deliberately absent: `Capture()` only ever
	 *  tallies live `ASummonedUnit`s, so a roster can hold exactly these 13.
	 */
	static TArray<FName> ThirteenKindsInCardRowOrder()
	{
		return TArray<FName>{
			TEXT("footman"), TEXT("archer"), TEXT("knight"), TEXT("miner"),
			TEXT("militiamob"), TEXT("pikeman"), TEXT("sapper"), TEXT("cavalry"),
			TEXT("longbowman"), TEXT("cleric"), TEXT("ogre"), TEXT("wizard"),
			TEXT("sorcerer")
		};
	}

	/** The canonical symbol the whole defect is about. Spelled once so a typo cannot make a test pass by testing the wrong noun. */
	static const TCHAR* const SorcererSymbol = TEXT("sorcerer");

	/**
	 *  The seven place symbols of AS-§9a's pinned v1 vocabulary. Present so
	 *  `BuildZoneC`'s HEAD measures its shipped 108 characters rather than the
	 *  22 an empty place list would give — the roster budget is
	 *  `1085 - 192 - Head - Tail`, so a short head would hand the trimmer ~86
	 *  characters it does not have in play and the collapse tests would be
	 *  measuring a board this game never produces.
	 */
	static TArray<FName> SevenPlaces()
	{
		return TArray<FName>{
			TEXT("own_castle"), TEXT("enemy_castle"), TEXT("mid"),
			TEXT("ancient_ground_near"), TEXT("ancient_ground_far"),
			TEXT("nearest_mine"), TEXT("hero")
		};
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  REFLECTION WRITES INTO THE SNAPSHOT'S PRIVATE, Capture()-OWNED STATE
	// ═══════════════════════════════════════════════════════════════════════════

	/**
	 *  ⚠️ EVERY LOOKUP CHECKS THE INNER PROPERTY TYPE, NOT JUST THE NAME. A field
	 *  renamed OR retyped (say `TArray<FName>` -> `TArray<FString>`) must fail
	 *  LOUDLY here, because the alternative is a snapshot that stays empty and a
	 *  collapse assertion that passes because there was nothing to collapse.
	 */
	static TArray<FName>* FindNameArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner || !ArrayProperty->Inner->IsA<FNameProperty>())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<FName>>(Object);
	}

	static TArray<int32>* FindIntArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner || !ArrayProperty->Inner->IsA<FIntProperty>())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<int32>>(Object);
	}

	static int32* FindIntField(UObject* Object, const TCHAR* FieldName)
	{
		FIntProperty* const IntProperty = FindFProperty<FIntProperty>(Object->GetClass(), FieldName);
		return IntProperty ? IntProperty->ContainerPtrToValuePtr<int32>(Object) : nullptr;
	}

	/** A snapshot plus whichever field name (if any) could not be reached — reported by name so a rename is actionable in one read. */
	struct FScratchSnapshot
	{
		TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot;
		FString MissingField;

		bool IsUsable() const { return Snapshot.IsValid() && MissingField.IsEmpty(); }
	};

	/**
	 *  Builds a snapshot in exactly the state `Capture()` leaves for a board where
	 *  every named kind is alive and fully orderable.
	 *
	 *  @param Kinds  the canonical symbols, IN THE ORDER Capture() would have
	 *                sorted them (DT_Cards row order). The tail of this array is
	 *                what the trimmer collapses.
	 *  @param PerKindTotal  live units of each kind.
	 *
	 *  ⚠️⚠️ THE MAGNITUDE OF THIS NUMBER IS PART OF THE MEASUREMENT, AND IT IS THE
	 *  ONE THING A READER WILL GET WRONG. Every roster row prints the count THREE
	 *  TIMES (total / orderable / followable), so a board with TWO-DIGIT counts is
	 *  3 chars wider PER ROW — 39 chars across 13 kinds — than the same board with
	 *  single-digit counts. AS-§20.3's famous "~6 chars of headroom" (887 of 893)
	 *  is the SINGLE-DIGIT figure: head 108 / roster 621 / tail 158. At
	 *  PerKindTotal >= 10 the same 13-kind block measures 660, and 660 does NOT fit
	 *  the 627-char budget left by the shipped 61-char `order:` line.
	 *
	 *  ⇒ 9 IS USED WHEREVER A TEST NEEDS THE UNCOLLAPSED CASE, so those tests
	 *  reproduce AS-§20.3's own operating point rather than a wider one, and their
	 *  slack is reported by AddInfo rather than asserted. ⛔ The consequence — that
	 *  an ordinary two-digit board is ALREADY collapsing at the default sentence
	 *  length — is deliberately NOT asserted anywhere: it is a live measurement
	 *  reported to TASK-525, and pinning it as a test would make TASK-528's
	 *  ZoneBCharReserve repair fail this file for succeeding.
	 */
	static FScratchSnapshot MakeSnapshotWithRoster(const TArray<FName>& Kinds, int32 PerKindTotal)
	{
		FScratchSnapshot Scratch;
		Scratch.Snapshot.Reset(NewObject<USiegeAssistantSnapshot>());
		if (!Scratch.Snapshot.IsValid())
		{
			Scratch.MissingField = TEXT("<the snapshot object itself>");
			return Scratch;
		}

		USiegeAssistantSnapshot* const Object = Scratch.Snapshot.Get();

		TArray<FName>* const UnitKinds = FindNameArrayField(Object, TEXT("UnitKinds"));
		TArray<int32>* const Totals = FindIntArrayField(Object, TEXT("KindTotals"));
		TArray<int32>* const Orderable = FindIntArrayField(Object, TEXT("KindOrderable"));
		TArray<int32>* const Followable = FindIntArrayField(Object, TEXT("KindFollowable"));
		TArray<FName>* const Places = FindNameArrayField(Object, TEXT("PlaceNames"));
		int32* const StanceFree = FindIntField(Object, TEXT("StanceFree"));

		// Reported as ONE name so the message says which field moved, rather than
		// "something failed".
		if (!UnitKinds)        { Scratch.MissingField = TEXT("UnitKinds (TArray<FName>)"); return Scratch; }
		if (!Totals)           { Scratch.MissingField = TEXT("KindTotals (TArray<int32>)"); return Scratch; }
		if (!Orderable)        { Scratch.MissingField = TEXT("KindOrderable (TArray<int32>)"); return Scratch; }
		if (!Followable)       { Scratch.MissingField = TEXT("KindFollowable (TArray<int32>)"); return Scratch; }
		if (!Places)           { Scratch.MissingField = TEXT("PlaceNames (TArray<FName>)"); return Scratch; }
		if (!StanceFree)       { Scratch.MissingField = TEXT("StanceFree (int32)"); return Scratch; }

		*UnitKinds = Kinds;

		Totals->Reset();
		Orderable->Reset();
		Followable->Reset();
		for (int32 Index = 0; Index < Kinds.Num(); ++Index)
		{
			// Parallel arrays, exactly as Capture() fills them in one pass. Every
			// kind fully orderable and followable keeps the ROW WIDTH at its
			// widest realistic value, which is the conservative direction for a
			// budget test: a narrower row would give the trimmer slack the shipped
			// board does not have.
			Totals->Add(PerKindTotal);
			Orderable->Add(PerKindTotal);
			Followable->Add(PerKindTotal);
		}

		*Places = SevenPlaces();
		*StanceFree = PerKindTotal * Kinds.Num();

		return Scratch;
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  READING ZONE C BACK
	// ═══════════════════════════════════════════════════════════════════════════

	/** The value of the first line beginning `<Key>: `, or an empty string when the key is absent. ⛔ Case-sensitive: these are prompt bytes. */
	static FString ValueOfKey(const FString& ZoneC, const TCHAR* Key)
	{
		TArray<FString> Lines;
		ZoneC.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		const FString Prefix = FString(Key) + TEXT(": ");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return Line.RightChop(Prefix.Len());
			}
		}
		return FString();
	}

	/** True when a `<Key>:` line exists at all — the fixed-key law's actual claim, which is weaker than "it has a value". */
	static bool HasKeyLine(const FString& ZoneC, const TCHAR* Key)
	{
		TArray<FString> Lines;
		ZoneC.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		const FString Prefix = FString(Key) + TEXT(":");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 *  The symbols PRINTED AS FULL ROSTER ROWS, in printed order. A row is
	 *  `- <symbol>: <n> total, <n> orderable, <n> followable`.
	 *
	 *  ⚠️ `- none` is deliberately NOT collected: it is the empty-roster sentinel,
	 *  not a kind, and counting it would let a zero-row block masquerade as a
	 *  one-kind block.
	 */
	static TArray<FString> PrintedRosterSymbols(const FString& ZoneC)
	{
		TArray<FString> Symbols;

		TArray<FString> Lines;
		ZoneC.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		for (const FString& Line : Lines)
		{
			if (!Line.StartsWith(TEXT("- "), ESearchCase::CaseSensitive))
			{
				continue;
			}

			int32 ColonIndex = INDEX_NONE;
			if (!Line.FindChar(TEXT(':'), ColonIndex))
			{
				continue;
			}

			Symbols.Add(Line.Mid(2, ColonIndex - 2));
		}

		return Symbols;
	}

	/**
	 *  The symbols NAMED on the collapse line. `other_kinds: none` yields an empty
	 *  array; `other_kinds: wizard, sorcerer (3 units)` yields [wizard, sorcerer].
	 */
	static TArray<FString> CollapsedSymbols(const FString& ZoneC)
	{
		TArray<FString> Symbols;

		FString Value = ValueOfKey(ZoneC, TEXT("other_kinds"));
		if (Value.IsEmpty() || Value.Equals(TEXT("none"), ESearchCase::CaseSensitive))
		{
			return Symbols;
		}

		// Drop the trailing " (N units)" aggregate; everything before it is the
		// comma-separated symbol list.
		int32 ParenIndex = INDEX_NONE;
		if (Value.FindChar(TEXT('('), ParenIndex))
		{
			Value = Value.Left(ParenIndex).TrimEnd();
		}

		Value.ParseIntoArray(Symbols, TEXT(", "), /*InCullEmpty*/ true);
		return Symbols;
	}

	/**
	 *  A player sentence of exactly N characters — the lever that drives the
	 *  trimmer, because `order:` lives in BuildZoneC's untrimmable Tail.
	 *
	 *  ⚠️ THE SHAPE OF THE STRING IS LOAD-BEARING, NOT COSMETIC. `SanitizeForPrompt`
	 *  drops leading whitespace, COLLAPSES runs of it, and caps on UTF-8 BYTES:
	 *   - ASCII only, so the requested length and the spent budget cannot differ;
	 *   - short words separated by SINGLE spaces, so nothing collapses;
	 *   - ⛔ never a leading or trailing space, or the sanitiser would return
	 *     N-1 characters and every budget figure in these tests would be off by
	 *     one in a direction nothing reports.
	 */
	static FString OrderLineOfLength(int32 Characters)
	{
		FString Out;
		Out.Reserve(Characters);
		while (Out.Len() < Characters)
		{
			Out += (Out.Len() % 5 == 4) ? TEXT(" ") : TEXT("x");
		}

		Out.LeftInline(Characters);

		// The modulo can land a space on the final character; swap it so the
		// sanitiser has nothing to trim.
		if (Out.Len() > 0 && Out[Out.Len() - 1] == TEXT(' '))
		{
			Out[Out.Len() - 1] = TEXT('x');
		}

		return Out;
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  PARSER HELPERS
	// ═══════════════════════════════════════════════════════════════════════════

	/** One well-formed command JSON with an arbitrary `who` payload spliced in, so every test varies exactly one thing. */
	static FString CommandJson(const TCHAR* Intent, const TCHAR* WhoPayload)
	{
		return FString::Printf(
			TEXT("{\"intent\":\"%s\",\"who\":%s,\"where\":\"mid\",\"when\":\"now\"}"),
			Intent, WhoPayload);
	}

	/** The bare reason CODE, with any ':detail' payload stripped — what the FSM switches on. */
	static FString CodeOf(const FString& Reason)
	{
		return SiegeAssistantReasonCode(Reason);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 — ⭐ Siegebound.Assistant.Selection.RosterShowsAllThirteenKinds
//
//  THE TEST JONATHAN'S FIRST COMPLAINT MAPS ONTO, AND THE ONE THE CORPUS CANNOT
//  WRITE. Before it, nothing anywhere asserted that a board holding a Sorcerer
//  shows the model the token `sorcerer`.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRosterShowsAllThirteenKindsTest,
	"Siegebound.Assistant.Selection.RosterShowsAllThirteenKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRosterShowsAllThirteenKindsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ⚠️ THE COLLAPSE WARNING IS EXPECTED TRAFFIC ON THE HEADROOM SUB-CASE BELOW,
	// NOT A FAILURE. `BuildZoneC` reports every degradation at Warning by design
	// (the TASK-419 WARN-5 visibility latch — a >MaxRosterKinds board used to
	// degrade the prompt silently forever). ⛔ Occurrences < 0 means "silently
	// ignore", chosen over 0 ("must be seen") because THIS test's main path
	// deliberately does NOT collapse. ⛔ It is scoped to this one message: a
	// blanket warning suppression would hide the two player-text truncation
	// latches as well.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();

	// The cap is HIS ruling (AS-§20, ruling 1) and half the fix. Asserted here
	// because a silent revert to 8 would make every other assertion in this file
	// pass while the defect came back.
	TestEqual(TEXT("MaxRosterKinds is 13 — Jonathan's ruling 1, the D2 escalation he spent by name"),
		USiegeAssistantSnapshot::MaxRosterKinds, 13);
	TestEqual(TEXT("The fixture board holds all 13 commandable DT_Cards kinds"), Kinds.Num(), 13);
	TestEqualSensitive(TEXT("`sorcerer` is the LAST kind in DT_Cards row order — the mechanism of the defect"),
		Kinds.Last().ToString(), FString(SorcererSymbol));

	// PerKindTotal 9 — AS-§20.3's own single-digit operating point. See
	// MakeSnapshotWithRoster's comment for why the magnitude is load-bearing.
	FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 9);
	if (!TestTrue(*FString::Printf(
			TEXT("The snapshot's Capture()-owned roster fields were reachable by reflection (missing: %s). ⛔ If this fails the field was RENAMED or RETYPED and every collapse assertion in this file would otherwise pass VACUOUSLY on an empty roster."),
			*Scratch.MissingField),
		Scratch.IsUsable()))
	{
		return false;
	}

	// A short, ordinary sentence: the operating point a player actually types.
	const FString ZoneC = Scratch.Snapshot->BuildZoneC(TEXT("send all units to the middle"), FString());

	AddInfo(FString::Printf(TEXT("Zone C on a 13-kind board with a 28-char order: %d chars (SnapshotTrimBudgetChars %d, ZoneBCharReserve %d ⇒ Zone C budget %d)."),
		ZoneC.Len(), USiegeAssistantSnapshot::SnapshotTrimBudgetChars, USiegeAssistantSnapshot::ZoneBCharReserve,
		USiegeAssistantSnapshot::SnapshotTrimBudgetChars - USiegeAssistantSnapshot::ZoneBCharReserve));

	// ⚠️ THE HEADROOM, MEASURED AND REPORTED RATHER THAN ASSERTED. AS-§20.3
	// records ~6 chars at the shipped 61-char `order:` line, and the number moves
	// with the board. Asserting it would make TASK-528's ZoneBCharReserve repair
	// FAIL this file for succeeding, so it is instrumentation: the figure lands in
	// the run log where the next reader of the risk table can compare it.
	{
		FScratchSnapshot Shipped = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 9);
		FScratchSnapshot Realistic = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 12);
		if (Shipped.IsUsable() && Realistic.IsUsable())
		{
			// 61 chars: the DEFAULT `order:` line the shipped 887-of-893 figure was
			// derived on.
			const FString DefaultOrder = OrderLineOfLength(61);
			const FString ShippedZoneC = Shipped.Snapshot->BuildZoneC(DefaultOrder, FString());
			const FString RealisticZoneC = Realistic.Snapshot->BuildZoneC(DefaultOrder, FString());

			AddInfo(FString::Printf(
				TEXT("HEADROOM READING at the shipped 61-char `order:` line — single-digit counts: Zone C %d chars, %d row(s) printed, %d collapsed. TWO-digit counts (an ordinary mid-match board): Zone C %d chars, %d row(s) printed, %d collapsed. ⚠️ Every roster row carries the count THREE times, so two-digit counts widen the 13-kind block by ~39 chars against a ~6-char headroom."),
				ShippedZoneC.Len(), PrintedRosterSymbols(ShippedZoneC).Num(), CollapsedSymbols(ShippedZoneC).Num(),
				RealisticZoneC.Len(), PrintedRosterSymbols(RealisticZoneC).Num(), CollapsedSymbols(RealisticZoneC).Num()));

			// ⭐ AND THE INVARIANT HOLDS EITHER WAY — which is the whole point of
			// TASK-517's names fix, and the reason the reading above is a note and
			// not a failure.
			TestTrue(TEXT("⭐ `sorcerer` is visible on a two-digit board too, collapsed or not — the names fix is what makes the headroom survivable"),
				RealisticZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));
		}
	}

	// ⭐⭐ THE CLAIM. Case-sensitive because it is a claim about the BYTES the
	// tokenizer sees: `Sorcerer` and `sorcerer` are different tokens, and Zone A
	// teaches the model lower-case symbols.
	TestTrue(TEXT("⭐ Zone C CONTAINS the literal `sorcerer` on a board where a Sorcerer is alive — the token that never reached the model before TASK-517"),
		ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));

	const TArray<FString> Printed = PrintedRosterSymbols(ZoneC);
	TestEqual(TEXT("All 13 kinds print as FULL roster rows at this operating point"), Printed.Num(), 13);

	// Row-by-row, in order — a set comparison would pass on a roster that printed
	// the right symbols in the wrong order, and the ORDER is what makes the
	// collapse deterministic.
	for (int32 Index = 0; Index < Kinds.Num() && Index < Printed.Num(); ++Index)
	{
		TestEqualSensitive(*FString::Printf(TEXT("Roster row %d is the DT_Cards row-order symbol"), Index),
			Printed[Index], Kinds[Index].ToString());
	}

	TestEqualSensitive(TEXT("Nothing collapsed, so `other_kinds:` reads exactly `none`"),
		ValueOfKey(ZoneC, TEXT("other_kinds")), FString(TEXT("none")));

	// The whole row, not just the symbol — this is what proves the model is given
	// the Sorcerer's NUMBERS as well as its name at the uncollapsed operating
	// point, which is the difference between an executable order and a
	// clarification.
	TestTrue(TEXT("The Sorcerer's full roster row is present with its counts"),
		ZoneC.Contains(TEXT("- sorcerer: 9 total, 9 orderable, 9 followable"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 — ⭐⭐ Siegebound.Assistant.Selection.CollapseNamesTheHiddenKinds
//
//  THE HIGHEST-VALUE TEST IN THE BATCH, AND THE ONE NO OTHER INSTRUMENT REACHES.
//  TASK-524's finding F-5: the eval lane's fixture prints all 13 kinds
//  unconditionally, so the collapse has NEVER happened there and a green eval row
//  says nothing about this leg. AS-§20.2: at 13 kinds the roster measures ~887 of
//  ~893, so roughly SEVEN extra typed characters re-collapse the tail — and the
//  tail is the Sorcerer.
//
//  ⛔ THE ASSERTION IS ON THE NAME, NOT THE COUNT. The shipped defect had the
//  right count (`other_kinds: 5 kinds, 9 units`) and no name.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionCollapseNamesTheHiddenKindsTest,
	"Siegebound.Assistant.Selection.CollapseNamesTheHiddenKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionCollapseNamesTheHiddenKindsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ⭐ Occurrences == 0 means "one or more, no upper limit" — so this line is not
	// merely a suppression, it is an ASSERTION that the shipped visibility latch
	// FIRED. A collapse that degrades the prompt without saying so is the TASK-419
	// WARN-5 defect (a >MaxRosterKinds board degrading silently forever), and it
	// would otherwise be invisible to this test: `other_kinds:` would still read
	// correctly while the developer-facing half of the contract had gone missing.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 0);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();

	FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 9);
	if (!TestTrue(*FString::Printf(TEXT("The roster fields were reachable by reflection (missing: %s)"), *Scratch.MissingField),
		Scratch.IsUsable()))
	{
		return false;
	}

	// ⚠️ THE LEVER IS THE PLAYER'S OWN SENTENCE, WHICH IS THE POINT. `order:` and
	// `pending:` live in BuildZoneC's Tail, are subtracted from the roster budget
	// BEFORE the trimmer runs, and are never themselves trimmed by it (the
	// CONVENTIONS §8 rule that the utterance is never truncated by the snapshot
	// budget). So a long sentence is exactly how a real player forces this path —
	// which is why "it only breaks when you type a lot" was so hard to see.
	const FString LongOrder = OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes);
	const FString LongPending = OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes);

	const FString ZoneC = Scratch.Snapshot->BuildZoneC(LongOrder, LongPending);

	const TArray<FString> Printed = PrintedRosterSymbols(ZoneC);
	const TArray<FString> Collapsed = CollapsedSymbols(ZoneC);

	AddInfo(FString::Printf(TEXT("Forced collapse: %d kind(s) printed in full, %d named on `other_kinds:`. Zone C is %d chars. Collapse line: `other_kinds: %s`."),
		Printed.Num(), Collapsed.Num(), ZoneC.Len(), *ValueOfKey(ZoneC, TEXT("other_kinds"))));

	// ⛔ THE PRE-CONDITION IS ASSERTED, NOT ASSUMED. If the budget arithmetic ever
	// moves so far that a 240-byte order no longer collapses anything, this test
	// would pass on the uncollapsed path while claiming to have measured the
	// collapse — a green bar for a case that never ran. That is the failure shape
	// AS-§12g names, so it is refused here explicitly.
	if (!TestTrue(TEXT("⛔ PRE-CONDITION: a 240-byte order + a 240-byte pending line DOES force the trimmer to collapse at least one kind. If this fails the test below proves NOTHING and must not be read as a pass."),
		Collapsed.Num() > 0))
	{
		return false;
	}

	TestTrue(TEXT("The collapse is genuinely partial — some kinds still print in full"), Printed.Num() > 0);

	// ⭐⭐ THE DURABLE HALF OF THE FIX (AS-§20.2): a collapse may hide a kind's
	// NUMBERS; it may never hide its NAME. This closes the class at ANY cap and
	// ANY board size, which is why it — and not MaxRosterKinds = 13 — is the
	// root-cause repair.
	TestTrue(TEXT("⭐ `sorcerer` appears SOMEWHERE in Zone C even though it was collapsed — the name survives what the numbers do not"),
		ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));

	TestTrue(TEXT("⭐⭐ `sorcerer` is named BY NAME on the `other_kinds:` line, not merely counted"),
		Collapsed.Contains(FString(SorcererSymbol)));

	// The Sorcerer is last in card-row order and the trimmer drops from the tail,
	// so it must be the LAST symbol on the collapse line: the line reproduces the
	// roster's own order rather than an arbitrary one.
	TestEqualSensitive(TEXT("The collapse line preserves DT_Cards row order — `sorcerer`, the last row, is last"),
		Collapsed.Last(), FString(SorcererSymbol));

	// ⛔ EVERY collapsed kind, not just the one the bug was reported on. A fix that
	// named only the newest kind would pass the assertion above and still hide
	// `wizard`, `ogre`, `cleric`… on the next board.
	for (int32 Index = Printed.Num(); Index < Kinds.Num(); ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Collapsed kind `%s` is NAMED on the `other_kinds:` line"), *Kinds[Index].ToString()),
			Collapsed.Contains(Kinds[Index].ToString()));
	}

	TestEqual(TEXT("Printed rows + named collapsed kinds account for the WHOLE roster — nothing vanished"),
		Printed.Num() + Collapsed.Num(), Kinds.Num());

	// ⛔ THE REGRESSION GUARD ON THE OLD FORMAT. `other_kinds: 5 kinds, 9 units`
	// is the string that shipped, and it is the defect. Asserting its ABSENCE means
	// a revert to counts-only fails here even if some future format still happens
	// to contain the substring `sorcerer` elsewhere in the block.
	TestFalse(TEXT("⛔ The retired counts-only format (`N kinds, M units`) is GONE — that string IS the defect"),
		ZoneC.Contains(TEXT(" kinds, "), ESearchCase::CaseSensitive));

	// The aggregate is still carried, in the pinned `(<N> units)` shape. `(1 units)`
	// on a single collapsed kind is deliberate (AS-§20.2) — a pluralisation branch
	// would spend characters out of a ~6-char headroom.
	TestTrue(TEXT("The collapse line still carries the aggregate unit count in the pinned `(<N> units)` shape"),
		ValueOfKey(ZoneC, TEXT("other_kinds")).EndsWith(TEXT(" units)"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 — Siegebound.Assistant.Selection.FixedKeyOrderSurvives
//
//  The fixed-key law: the KEYS never vary, because the executor and the FSM parse
//  nothing in Zone C and a missing key would teach the model that a key is
//  OPTIONAL. TASK-517 changed the VALUE of `other_kinds:`; this asserts it did
//  not change the key set or their order.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionFixedKeyOrderSurvivesTest,
	"Siegebound.Assistant.Selection.FixedKeyOrderSurvives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionFixedKeyOrderSurvivesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// Two of the three boards below collapse on purpose; the shipped visibility
	// latch reports that at Warning. Ignored here rather than asserted, because
	// this test's subject is the KEY SET, not the collapse (TASK-523 spec item 2.3).
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	// THREE boards that exercise all three shapes of the block: uncollapsed,
	// collapsed, and empty. The key order must be identical in all three.
	struct FCase
	{
		const TCHAR* Label;
		TArray<FName> Kinds;
		FString Order;
		FString Pending;
	};

	TArray<FCase> Cases;
	Cases.Add({ TEXT("13 kinds, short order (nothing collapses)"), ThirteenKindsInCardRowOrder(), TEXT("send all units to the middle"), FString() });
	Cases.Add({ TEXT("13 kinds, maximal order (the trimmer bites)"), ThirteenKindsInCardRowOrder(),
		OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes), OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes) });
	Cases.Add({ TEXT("empty roster (match start / null-world capture)"), TArray<FName>(), TEXT("send everyone"), FString() });

	for (const FCase& Case : Cases)
	{
		FScratchSnapshot Scratch = MakeSnapshotWithRoster(Case.Kinds, /*PerKindTotal*/ 9);
		if (!TestTrue(*FString::Printf(TEXT("[%s] roster fields reachable (missing: %s)"), Case.Label, *Scratch.MissingField), Scratch.IsUsable()))
		{
			continue;
		}

		const FString ZoneC = Scratch.Snapshot->BuildZoneC(Case.Order, Case.Pending);

		// ⛔ ALWAYS EMITTED, INCLUDING ON THE EMPTY ROSTER AND INCLUDING WHEN
		// NOTHING COLLAPSED. This is the assertion TASK-523's spec item (2.3) asks
		// for by name, and it is the one that stops a future "only print
		// other_kinds when something collapsed" tidy-up.
		TestTrue(*FString::Printf(TEXT("[%s] the `other_kinds:` key is emitted"), Case.Label),
			HasKeyLine(ZoneC, TEXT("other_kinds")));

		for (const TCHAR* Key : { TEXT("places"), TEXT("roster"), TEXT("other_kinds"), TEXT("stances"), TEXT("hero"), TEXT("pending"), TEXT("order") })
		{
			TestTrue(*FString::Printf(TEXT("[%s] the `%s:` key is emitted"), Case.Label, Key), HasKeyLine(ZoneC, Key));
		}

		// ORDER, not merely presence — a set check would pass on a Zone C whose
		// keys had been shuffled, and the prompt's shape is what the model learns.
		int32 Cursor = 0;
		bool bOrdered = true;
		FString FirstOutOfOrderKey;
		for (const TCHAR* Key : { TEXT("[FORCES]\n"), TEXT("places:"), TEXT("roster:"), TEXT("other_kinds:"), TEXT("stances:"), TEXT("hero:"), TEXT("pending:"), TEXT("[ORDER]\n"), TEXT("order:") })
		{
			const int32 Found = ZoneC.Find(Key, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
			if (Found == INDEX_NONE)
			{
				bOrdered = false;
				FirstOutOfOrderKey = Key;
				break;
			}
			Cursor = Found + 1;
		}
		TestTrue(*FString::Printf(TEXT("[%s] the fixed key order holds: [FORCES] places roster other_kinds stances hero pending [ORDER] order (first key out of place: `%s`)"),
			Case.Label, *FirstOutOfOrderKey), bOrdered);

		// The empty roster prints the `- none` sentinel and STILL prints the
		// collapse key — the state a null-world or match-start Capture() leaves.
		if (Case.Kinds.Num() == 0)
		{
			TestTrue(*FString::Printf(TEXT("[%s] an empty roster prints the `- none` sentinel"), Case.Label),
				ZoneC.Contains(TEXT("roster:\n- none\n"), ESearchCase::CaseSensitive));
			TestEqualSensitive(*FString::Printf(TEXT("[%s] an empty roster still prints `other_kinds: none`"), Case.Label),
				ValueOfKey(ZoneC, TEXT("other_kinds")), FString(TEXT("none")));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 — Siegebound.Assistant.Selection.ShrinkLoopNeverHidesASymbol
//
//  ⭐ THE INVARIANT AS-§20.2 ACTUALLY CLAIMS, MEASURED ACROSS THE WHOLE SHRINK
//  RANGE RATHER THAN AT ONE POINT: for EVERY sentence length, every kind on the
//  board is visible to the model — as a full row, or by name on the collapse line.
//
//  Test 2 measures one collapse. This measures ALL of them, and it is what would
//  catch a format change that names the collapsed kinds correctly at depth 1 and
//  drops one at depth 9.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionShrinkLoopNeverHidesASymbolTest,
	"Siegebound.Assistant.Selection.ShrinkLoopNeverHidesASymbol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionShrinkLoopNeverHidesASymbolTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// The sweep drives ~124 collapses on purpose (a FRESH snapshot per rung, so
	// each one's Warning latch starts clean). Ignored rather than asserted: Test 2
	// is where the latch itself is the claim.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();

	// BOTH board magnitudes. A roster row prints its count THREE times, so a
	// two-digit board is 39 chars wider across 13 kinds and reaches every rung of
	// the ladder at a SHORTER sentence. Sweeping both means the invariant is
	// measured on the operating point AS-§20.3 recorded AND on the one an
	// ordinary mid-match board actually produces.
	const int32 BoardMagnitudes[] = { 9, 12 };

	int32 DeepestCollapseOverall = 0;

	for (const int32 PerKindTotal : BoardMagnitudes)
	{
		int32 PreviousPrinted = MAX_int32;
		int32 DeepestCollapse = 0;
		int32 FirstCollapseLength = INDEX_NONE;

		// Sweeps the ONE variable a player controls, from a short sentence to the
		// MaxUtteranceBytes cap, in steps small enough to catch every rung of the
		// ladder (AS-§20.2 measures ~7 characters between the first two rungs).
		for (int32 OrderLength = 0; OrderLength <= USiegeAssistantSnapshot::MaxUtteranceBytes; OrderLength += 4)
		{
			FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, PerKindTotal);
			if (!Scratch.IsUsable())
			{
				AddError(FString::Printf(TEXT("The roster fields were not reachable by reflection (missing: %s)."), *Scratch.MissingField));
				return false;
			}

			const FString ZoneC = Scratch.Snapshot->BuildZoneC(OrderLineOfLength(OrderLength), FString());

			const TArray<FString> Printed = PrintedRosterSymbols(ZoneC);
			const TArray<FString> Collapsed = CollapsedSymbols(ZoneC);

			// ⭐ THE INVARIANT. Union == the whole board, at every rung.
			if (Printed.Num() + Collapsed.Num() != Kinds.Num())
			{
				AddError(FString::Printf(
					TEXT("⛔ A SYMBOL VANISHED at order length %d (board of %d per kind): %d printed + %d collapsed != %d kinds. A collapse may hide a kind's NUMBERS; it may NEVER hide its NAME (AS-§20.2). Collapse line: `other_kinds: %s`."),
					OrderLength, PerKindTotal, Printed.Num(), Collapsed.Num(), Kinds.Num(), *ValueOfKey(ZoneC, TEXT("other_kinds"))));
				return false;
			}

			for (const FName& Kind : Kinds)
			{
				const FString Symbol = Kind.ToString();
				if (!Printed.Contains(Symbol) && !Collapsed.Contains(Symbol))
				{
					AddError(FString::Printf(TEXT("⛔ `%s` is INVISIBLE to the model at order length %d (board of %d per kind) — neither a roster row nor a named collapse."),
						*Symbol, OrderLength, PerKindTotal));
					return false;
				}
			}

			// MONOTONIC. A longer sentence can only ever shrink the roster, never
			// grow it: the budget is `TrimBudget - ZoneBReserve - Head - Tail` and
			// the sentence is in the Tail. A non-monotonic step would mean the
			// collapse line's own bytes had begun to fight the loop that produces
			// it — the ONE property TASK-517's new format could have broken, and
			// the reason its handoff argues the symbol "cancels" between the two.
			if (Printed.Num() > PreviousPrinted)
			{
				AddError(FString::Printf(
					TEXT("⛔ THE SHRINK LOOP IS NOT MONOTONIC (board of %d per kind): order length %d prints %d kinds where the shorter sentence printed %d. A longer utterance must never widen the roster."),
					PerKindTotal, OrderLength, Printed.Num(), PreviousPrinted));
				return false;
			}
			PreviousPrinted = Printed.Num();

			if (Collapsed.Num() > 0 && FirstCollapseLength == INDEX_NONE)
			{
				FirstCollapseLength = OrderLength;
			}
			DeepestCollapse = FMath::Max(DeepestCollapse, Collapsed.Num());

			// The Sorcerer, called out by name at every single rung, because it is
			// the kind Jonathan reported and the one the tail always drops first.
			if (!ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive))
			{
				AddError(FString::Printf(TEXT("⛔ `sorcerer` disappeared from Zone C at order length %d (board of %d per kind) — this is Jonathan's reported defect, reproduced."),
					OrderLength, PerKindTotal));
				return false;
			}
		}

		AddInfo(FString::Printf(
			TEXT("Board of %d per kind: swept order lengths 0..%d. First collapse at order length %d. Deepest collapse: %d kind(s) named on `other_kinds:`. Roster narrowed monotonically throughout and every symbol stayed visible."),
			PerKindTotal, USiegeAssistantSnapshot::MaxUtteranceBytes, FirstCollapseLength, DeepestCollapse));

		DeepestCollapseOverall = FMath::Max(DeepestCollapseOverall, DeepestCollapse);
	}

	// ⛔ THE SWEEP MUST ACTUALLY HAVE EXERCISED THE COLLAPSE. Without this the
	// whole test would pass on a build where the trimmer never fires, reporting
	// SAFE about a path that never ran.
	TestTrue(TEXT("⛔ The sweep genuinely reached the collapse path (at least one kind was collapsed at some length) — otherwise nothing above was measured"),
		DeepestCollapseOverall > 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 — Siegebound.Assistant.Selection.ExclusionParses
//
//  The accept side of Jonathan's ruling 3: "all except X (and Y)" is a GENUINE
//  EXECUTABLE ORDER, not a clarify-only fix.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionParsesTest,
	"Siegebound.Assistant.Selection.ExclusionParses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionParsesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── ONE KIND — Jonathan's own example, "(All units except miners)" ──────────
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\"]}")), Command, Error);

		TestTrue(*FString::Printf(TEXT("⭐ `send all units except the miners` parses (error: %s)"), *Error), bParsed);
		TestEqual(TEXT("ExcludeKinds holds exactly the one named kind"), Command.ExcludeKinds.Num(), 1);
		if (Command.ExcludeKinds.Num() == 1)
		{
			TestEqualSensitive(TEXT("The excluded symbol is `miner`"), Command.ExcludeKinds[0].ToString(), FString(TEXT("miner")));
		}

		// ⭐ THE SEAM THE EXECUTOR KEYS ON, AND TASK-524's DEV-26 DEPENDS ON IT:
		// an exclusion is a MODIFIED "all", so Kinds stays EMPTY and the command
		// flows through the executor's existing `Kinds.Num() == 0` branch. ⛔ If
		// the parser ever synthesised an enumeration here instead, the exclusion
		// would silently become a positive selection capped at 3 kinds.
		TestEqual(TEXT("⭐ Kinds stays EMPTY — an exclusion is a modified `all`, never a synthesised enumeration"), Command.Kinds.Num(), 0);
		TestEqual(TEXT("Counts stays empty alongside it"), Command.Counts.Num(), 0);
		TestTrue(TEXT("The intent survives the exclusion shape"), Command.Intent == ESiegeAssistantIntent::Send);
	}

	// ── 1, 2 AND 3 KINDS ALL ACCEPTED, ON EVERY SELECTION-BEARING VERB ─────────
	const TCHAR* const SelectionBearingIntents[] = { TEXT("send"), TEXT("guard"), TEXT("ambush"), TEXT("follow") };
	const TCHAR* const Payloads[] = {
		TEXT("{\"all_except\":[\"miner\"]}"),
		TEXT("{\"all_except\":[\"miner\",\"cleric\"]}"),
		TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\"]}")
	};

	for (const TCHAR* Intent : SelectionBearingIntents)
	{
		for (int32 Arity = 0; Arity < UE_ARRAY_COUNT(Payloads); ++Arity)
		{
			FSiegeAssistantCommand Command;
			FString Error;
			const bool bParsed = ParseSiegeAssistantCommand(CommandJson(Intent, Payloads[Arity]), Command, Error);

			TestTrue(*FString::Printf(TEXT("`%s` accepts a %d-kind exclusion (error: %s)"), Intent, Arity + 1, *Error), bParsed);
			TestEqual(*FString::Printf(TEXT("`%s` keeps all %d excluded kinds"), Intent, Arity + 1), Command.ExcludeKinds.Num(), Arity + 1);
			TestEqual(*FString::Printf(TEXT("`%s` leaves Kinds empty at arity %d"), Intent, Arity + 1), Command.Kinds.Num(), 0);
		}
	}

	// ── THE CAP IS THE CONSTANT, NOT THE LITERAL 3 ─────────────────────────────
	// Widening SiegeAssistantMaxExclusionKinds is a MANAGER RULING (AS-§20.1's
	// ruling-15 precedent). Pinned here so a tuner's edit fails a test instead of
	// slipping through, and pinned against the SELECTION cap it deliberately
	// mirrors.
	TestEqual(TEXT("SiegeAssistantMaxExclusionKinds is 3 — widening it is a manager ruling, not a tuner's edit"),
		SiegeAssistantMaxExclusionKinds, 3);
	TestEqual(TEXT("It mirrors SiegeAssistantMaxSelectionKinds deliberately"),
		SiegeAssistantMaxExclusionKinds, SiegeAssistantMaxSelectionKinds);

	// ── ⛔ ADDITIVITY AT THE WIRE, ASSERTED AS A BEHAVIOUR (SC-§18) ────────────
	// AS-§20.1's central claim is that the new `who` shape is STRICTLY ADDITIVE:
	// "every JSON the model emits today stays valid, byte-for-byte, and nothing
	// that already passes starts failing." A command carrying no exclusion must
	// therefore leave ExcludeKinds empty rather than defaulting to anything.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The pre-existing `who`:\"all\" shape still parses unchanged"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), Command, Error));
		TestEqual(TEXT("…and leaves ExcludeKinds EMPTY (the normal state of every command that excludes nothing)"),
			Command.ExcludeKinds.Num(), 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 — Siegebound.Assistant.Selection.ExclusionArityRefused
//
//  ⚠️ THE EMPTY LIST IS THE INTERESTING HALF. `{"all_except":[]}` is the model
//  saying "everyone except —" and stopping. Quietly PROMOTING that to a plain
//  "all" is how an exception gets dropped without anybody noticing, so it is
//  REFUSED. That is a deliberate ruling, and it is the one a "helpful"
//  simplification would undo first.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionArityRefusedTest,
	"Siegebound.Assistant.Selection.ExclusionArityRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionArityRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	struct FRefusal
	{
		const TCHAR* Label;
		const TCHAR* Payload;
	};

	const FRefusal Refusals[] = {
		{ TEXT("⛔ ZERO kinds — REFUSED, never promoted to a plain `all`"), TEXT("{\"all_except\":[]}") },
		{ TEXT("FOUR kinds — one past the cap"), TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\",\"ogre\"]}") },
		{ TEXT("FIVE kinds"), TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\",\"ogre\",\"wizard\"]}") }
	};

	for (const FRefusal& Refusal : Refusals)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), Refusal.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is refused"), Refusal.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s reports `exclude_arity`"), Refusal.Label),
			CodeOf(Error), FString(SiegeAssistantReason::ExcludeArity));

		// ⛔ NEVER PARTIALLY FILLED. A caller that ignores the return value must be
		// left holding an inert None command, not half an order.
		TestTrue(*FString::Printf(TEXT("%s leaves the command fully reset"), Refusal.Label),
			Command.Intent == ESiegeAssistantIntent::None && Command.ExcludeKinds.Num() == 0 && Command.Kinds.Num() == 0);
	}

	// ⚠️ THE EMPTY-LIST REFUSAL, RESTATED AS THE THING IT ACTUALLY PREVENTS. This
	// assertion is the whole reason the case above is not "harmless": if
	// `{"all_except":[]}` parsed, it would parse to a command IDENTICAL to
	// `who:"all"` — an order that moves the units the player just excluded.
	{
		FSiegeAssistantCommand Empty;
		FString EmptyError;
		const bool bEmptyParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[]}")), Empty, EmptyError);

		FSiegeAssistantCommand All;
		FString AllError;
		const bool bAllParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), All, AllError);

		TestTrue(TEXT("⛔ `{\"all_except\":[]}` must NOT parse to the same command as `who`:\"all\" — promoting it would execute the order the exception was refusing"),
			bAllParsed && !bEmptyParsed);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 — Siegebound.Assistant.Selection.ExclusionConflictRefused
//
//  ⛔ NEVER A MERGE. "Send 10 footmen except the miners" is a confused sentence,
//  and silently picking one half of it to honour is the valid-shaped-wrong-command
//  class this whole design exists to stop.
//
//  ⚠️ AND THE REACHABILITY IS STATED HONESTLY: from JSON the two `who` shapes are
//  DISJOINT (a selection is an Array, an exclusion is an Object), so this state
//  CANNOT be produced by any model output. It is reachable only over the M8 P2
//  wire, which is exactly why `SiegeAssistantValidateSelection` — the receive-side
//  gate — is where this is tested rather than through the parser.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionConflictRefusedTest,
	"Siegebound.Assistant.Selection.ExclusionConflictRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionConflictRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── CASE 1 — A SELECTION AND AN EXCLUSION AT ONCE ──────────────────────────
	{
		const TArray<FName> Kinds{ TEXT("footman") };
		const TArray<int32> Counts{ 10 };
		const TArray<FName> Excludes{ TEXT("miner") };

		FString Error;
		const bool bValid = SiegeAssistantValidateSelection(Kinds, Counts, Error, Excludes);

		TestFalse(TEXT("⛔ A positive selection PLUS an exclusion is refused — never merged"), bValid);
		TestEqualSensitive(TEXT("…and it reports `exclude_conflict`"), CodeOf(Error), FString(SiegeAssistantReason::ExcludeConflict));
	}

	// ── CASE 2 — `who`:"none" WITH AN EXCLUSION ────────────────────────────────
	// ⚠️ STRUCTURALLY UNREACHABLE FROM JSON AND SAID SO RATHER THAN FAKED: the
	// parser sets `bWhoIsNone` only in its String branch and fills ExcludeKinds
	// only in its Object branch, so no single `who` value can produce both. The
	// shipped guard is honestly labelled DEFENSIVE in the source. What IS
	// reachable — and is asserted here — is the pre-existing behaviour it sits
	// beside, unchanged by this batch.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"none\"")), Command, Error);

		TestFalse(TEXT("`who`:\"none\" on a selection-bearing verb is still refused (pre-existing, unchanged)"), bParsed);
		TestEqualSensitive(TEXT("…still with `who_required`, NOT the new `exclude_conflict` — the change is additive"),
			CodeOf(Error), FString(SiegeAssistantReason::WhoRequired));
	}
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("`who`:\"none\" on an ARMY-WIDE verb is still accepted (pre-existing, unchanged)"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("charge"), TEXT("\"none\"")), Command, Error));
	}

	// ── CASE 3 — A REPEATED EXCLUDED SYMBOL ────────────────────────────────────
	// A repeat is not merely redundant: it spends one of only three exclusion
	// slots saying nothing, which means the player's sentence and the order we
	// built have already diverged.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\",\"miner\"]}")), Command, Error);

		TestFalse(TEXT("A repeated excluded kind is refused"), bParsed);
		TestEqualSensitive(TEXT("…reusing the existing `duplicate_kind` code, not a second one"),
			CodeOf(Error), FString(SiegeAssistantReason::DuplicateKind));
	}

	// ⚠️ CASE-INSENSITIVE REPEAT. Symbol VALUES are compared case-insensitively
	// (FName is), so `["miner","MINER"]` is the SAME kind twice. Asserted because
	// a naive string-set implementation would accept it and hand the executor two
	// slots for one kind.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("A repeat that differs only in CASE is still a repeat"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\",\"MINER\"]}")), Command, Error));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 — ⭐ Siegebound.Assistant.Selection.ExclusionRefusedOnArmyWideIntents
//
//  ⛔⛔ THE RULING-3 ASSERTION, AND THE ONE THAT STOPS THE SILENT-DROP FAILURE.
//  charge / fallback / rally execute through
//  `ASiegePlayerController::ApplyArmyWideStance` and `AHeroCharacter::Rally()` —
//  the same shipped APIs the T / E / R keys call — and NONE of them walks the
//  candidate list. An ExcludeKinds handed to them has no code path that could
//  subtract anything, so it would be parsed and then DROPPED IN SILENCE:
//  "fall back except the miners" executing as "fall back INCLUDING the miners".
//  A valid-shaped wrong command that LOOKS obeyed is strictly worse than a
//  refusal, because nothing in the game or the log would contradict it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionRefusedOnArmyWideIntentsTest,
	"Siegebound.Assistant.Selection.ExclusionRefusedOnArmyWideIntents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionRefusedOnArmyWideIntentsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	const TCHAR* const ArmyWideIntents[] = { TEXT("charge"), TEXT("fallback"), TEXT("rally") };

	for (const TCHAR* Intent : ArmyWideIntents)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(Intent, TEXT("{\"all_except\":[\"miner\"]}")), Command, Error);

		TestFalse(*FString::Printf(TEXT("⛔ `%s ... except the miners` is REFUSED — it can never be honoured, so it is never accepted"), Intent), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("`%s` reports `exclude_conflict`"), Intent),
			CodeOf(Error), FString(SiegeAssistantReason::ExcludeConflict));

		// The payload NAMES THE OFFENDING VERB, which is what lets the FSM and the
		// log say which half of the sentence could not be kept.
		TestTrue(*FString::Printf(TEXT("The refusal's payload names `%s`, so the log says WHICH verb could not carry the exception"), Intent),
			Error.Contains(Intent, ESearchCase::CaseSensitive));

		// ⛔ AND THE ORDER IS NOT SILENTLY DEGRADED TO THE UNEXCEPTED ONE. This is
		// the assertion that distinguishes "refused" from "accepted with the
		// exception dropped" — the exact failure this whole feature exists to stop.
		TestTrue(*FString::Printf(TEXT("⛔ `%s` is NOT accepted with the exception quietly discarded — the command is fully reset"), Intent),
			Command.Intent == ESiegeAssistantIntent::None && Command.ExcludeKinds.Num() == 0);
	}

	// ── THE GATE IS THE SHIPPED PREDICATE, NOT A SECOND LIST OF VERBS ──────────
	// Reusing `SiegeAssistantIntentTakesSelection` means there is no parallel
	// army-wide list to drift out of step with the executor seam it describes.
	// Pinned here so a future verb lands on the right side automatically.
	TestTrue(TEXT("Send / Guard / Ambush / Follow take a selection"),
		SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Send)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Guard)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Ambush)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Follow));
	TestFalse(TEXT("Charge / Fallback / Rally do NOT — they never reach the selector"),
		SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Charge)
		|| SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Fallback)
		|| SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Rally));

	// ✅ HIS OWN EXAMPLE IS FULLY SERVED, and this is the control that proves the
	// refusal above is about the VERB and not about exclusions in general.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("✅ The same exception on `send` — Jonathan's own sentence — is ACCEPTED"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\"]}")), Command, Error));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 9 — Siegebound.Assistant.Selection.ExclusionSymbolsRefused
//
//  The malformed-payload paths, by reason code and by name. Symbol validation
//  REUSES `ParseKindSymbol` — ⛔ no second kind-validation path — so these also
//  assert that the exclusion list did not grow one.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionSymbolsRefusedTest,
	"Siegebound.Assistant.Selection.ExclusionSymbolsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionSymbolsRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	struct FCase
	{
		const TCHAR* Label;
		const TCHAR* Payload;
		const TCHAR* ExpectedCode;
	};

	const FCase Cases[] = {
		// The list is not an array at all.
		{ TEXT("`all_except` holding a bare string"), TEXT("{\"all_except\":\"miner\"}"), SiegeAssistantReason::BadType },
		{ TEXT("`all_except` holding an object"),     TEXT("{\"all_except\":{\"kind\":\"miner\"}}"), SiegeAssistantReason::BadType },

		// ⛔ THE DECLINED FEATURE, REFUSED AT THE PARSER TOO. Jonathan declined
		// "all except 5 archers"; the grammar makes it unsayable and this makes it
		// unparseable, so neither a prompt tweak nor a hand-written JSON can
		// re-introduce it.
		{ TEXT("⛔ a count-controlled item `{\"kind\":…,\"n\":…}` — the DECLINED variant"),
		  TEXT("{\"all_except\":[{\"kind\":\"archer\",\"n\":5}]}"), SiegeAssistantReason::BadType },

		// Reserved sentinels and empties are not kinds.
		{ TEXT("the reserved sentinel `none` as an excluded kind"), TEXT("{\"all_except\":[\"none\"]}"), SiegeAssistantReason::BadKind },
		{ TEXT("an empty excluded symbol"),                        TEXT("{\"all_except\":[\"\"]}"),     SiegeAssistantReason::BadKind },
		{ TEXT("a whitespace-only excluded symbol"),               TEXT("{\"all_except\":[\"   \"]}"),  SiegeAssistantReason::BadKind },

		// An unknown key inside the exclusion object — the exact-key-set rule.
		{ TEXT("an unknown key beside `all_except`"), TEXT("{\"all_except\":[\"miner\"],\"n\":5}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("a misspelled `all_except`"),          TEXT("{\"allexcept\":[\"miner\"]}"),          SiegeAssistantReason::UnknownKey }
	};

	for (const FCase& Case : Cases)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), Case.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is refused"), Case.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s reports `%s`"), Case.Label, Case.ExpectedCode),
			CodeOf(Error), FString(Case.ExpectedCode));
	}

	// ── SYMBOL CASE IS NORMALISED, NOT REJECTED ────────────────────────────────
	// The grammar only ever emits lower case, but FName is case-insensitive and
	// the parser lower-cases every symbol. Pinned so a future "strict" pass does
	// not start refusing a model that capitalised one letter.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("An excluded symbol in mixed case still parses"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"Miner\"]}")), Command, Error));
		if (Command.ExcludeKinds.Num() == 1)
		{
			TestEqualSensitive(TEXT("…and is stored LOWER-CASE, the canonical wire form"),
				Command.ExcludeKinds[0].ToString(), FString(TEXT("miner")));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 10 — Siegebound.Assistant.Selection.PreExistingWhoShapesUnchanged
//
//  ⭐ THE ADDITIVITY CLAIM, MEASURED. AS-§20.1: "every JSON the model emits today
//  stays valid, byte-for-byte, and nothing that already passes starts failing."
//  SC-§18: "additive" is a claim about BEHAVIOUR, not about diff arithmetic — so
//  it is asserted as behaviour, on the three PRE-EXISTING `who` shapes and the
//  three PRE-EXISTING refusal codes the new branch sits beside.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionPreExistingWhoShapesUnchangedTest,
	"Siegebound.Assistant.Selection.PreExistingWhoShapesUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionPreExistingWhoShapesUnchangedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── THE THREE PRE-EXISTING SHAPES STILL PARSE, IDENTICALLY ─────────────────
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The 1-item selection array still parses"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("[{\"kind\":\"footman\",\"n\":10}]")), Command, Error));
		TestEqual(TEXT("…with its kind"), Command.Kinds.Num(), 1);
		TestEqual(TEXT("…and its count"), Command.Counts.Num() == 1 ? Command.Counts[0] : -1, 10);
		TestEqual(TEXT("…and NO exclusion appears from nowhere"), Command.ExcludeKinds.Num(), 0);
	}
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The 3-item selection array — the cap — still parses"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"),
				TEXT("[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1},{\"kind\":\"archer\",\"n\":\"all\"}]")), Command, Error));
		TestEqual(TEXT("…with all three kinds"), Command.Kinds.Num(), 3);
		TestEqual(TEXT("…and `all` still maps to count 0"), Command.Counts.Num() == 3 ? Command.Counts[2] : -1, 0);
	}
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("`who`:\"all\" still parses to an EMPTY selection"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), Command, Error));
		TestEqual(TEXT("…Kinds empty"), Command.Kinds.Num(), 0);
		TestEqual(TEXT("…ExcludeKinds empty"), Command.ExcludeKinds.Num(), 0);
	}

	// ── THE THREE PRE-EXISTING REFUSALS STILL REPORT THEIR OWN CODES ───────────
	// ⛔ NOT `exclude_arity` OR `exclude_conflict`. If the new branch had widened
	// one of these, an FSM switching on the code would route a familiar failure to
	// a new, wrong clarification — a regression with no compiler diagnostic.
	struct FCase
	{
		const TCHAR* Label;
		const TCHAR* Intent;
		const TCHAR* Payload;
		const TCHAR* ExpectedCode;
	};

	const FCase Cases[] = {
		{ TEXT("`who` = a string that is neither `all` nor `none`"), TEXT("send"), TEXT("\"everyone\""),
		  SiegeAssistantReason::BadWho },
		{ TEXT("`who` = an EMPTY selection array"), TEXT("send"), TEXT("[]"),
		  SiegeAssistantReason::WhoArity },
		{ TEXT("`who` = a 4-item selection array (one past the cap)"), TEXT("send"),
		  TEXT("[{\"kind\":\"footman\",\"n\":1},{\"kind\":\"archer\",\"n\":1},{\"kind\":\"cleric\",\"n\":1},{\"kind\":\"ogre\",\"n\":1}]"),
		  SiegeAssistantReason::WhoArity },
		{ TEXT("`who` = \"none\" on a selection-bearing verb"), TEXT("send"), TEXT("\"none\""),
		  SiegeAssistantReason::WhoRequired },
		{ TEXT("a repeated kind inside a positive selection"), TEXT("send"),
		  TEXT("[{\"kind\":\"footman\",\"n\":1},{\"kind\":\"footman\",\"n\":2}]"),
		  SiegeAssistantReason::DuplicateKind },
		{ TEXT("`who` = a number"), TEXT("send"), TEXT("7"), SiegeAssistantReason::BadType }
	};

	for (const FCase& Case : Cases)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(Case.Intent, Case.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is still refused"), Case.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s still reports `%s` — NOT one of the new exclusion codes"), Case.Label, Case.ExpectedCode),
			CodeOf(Error), FString(Case.ExpectedCode));
	}

	// ── THE TOP-LEVEL KEY SET DID NOT CHANGE ───────────────────────────────────
	// ⛔ AS-§20.1 rejected a fourth top-level key (`"except":…`) precisely because
	// `ValidateExactKeySet` demands an exact set and the prompt law emits every key
	// always. Asserted so nobody "improves" it back later.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			TEXT("{\"intent\":\"send\",\"who\":\"all\",\"except\":[\"miner\"],\"where\":\"mid\",\"when\":\"now\"}"), Command, Error);

		TestFalse(TEXT("⛔ `except` as a FOURTH TOP-LEVEL KEY is refused — the exclusion is a `who` SHAPE, never a new key"), bParsed);
		TestEqualSensitive(TEXT("…reported as `unknown_key`"), CodeOf(Error), FString(SiegeAssistantReason::UnknownKey));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 11 — ⭐ Siegebound.Assistant.Selection.ValidatorExclusionArgumentIsLoadBearing
//
//  ⛔⛔ THE HIGHEST-RISK LINE IN THE BATCH, PINNED.
//
//  `SiegeAssistantValidateSelection` gained `ExcludeKinds` as a TRAILING DEFAULTED
//  parameter — a shape FORCED by file ownership (a required 4th argument would
//  have broken the compile in two files TASK-518 could not edit), not chosen. The
//  cost: A CALLER THAT OMITS THE ARGUMENT COMPILES CLEANLY AND VALIDATES NOTHING
//  ABOUT EXCLUSION. Removing `Command.ExcludeKinds` from the call in
//  `USiegeAssistantComponent::HandleModelCompletion` would raise NO compiler
//  diagnostic anywhere.
//
//  ⚠️ WHAT THIS TEST HONESTLY IS, AND IS NOT. It CANNOT observe that call site:
//  `HandleModelCompletion` needs a live component, a world and the Thinking state.
//  What it does is MEASURE THE DELTA the argument buys, so the hazard is a red bar
//  the moment anyone changes what the default means, and so the size of what is
//  lost by dropping it is written down as an executable fact rather than as a
//  comment. ⛔ A test whose name is a stronger claim than its comparator is
//  documentation, not a test — so the name says "load-bearing", which is exactly
//  what is measured.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionValidatorExclusionArgumentIsLoadBearingTest,
	"Siegebound.Assistant.Selection.ValidatorExclusionArgumentIsLoadBearing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionValidatorExclusionArgumentIsLoadBearingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// Three commands that are INVALID because of their exclusion alone. Each is a
	// state the M8 P2 wire path can deliver, since a peer passed through nobody's
	// grammar and nobody's parser.
	struct FCase
	{
		const TCHAR* Label;
		TArray<FName> Kinds;
		TArray<int32> Counts;
		TArray<FName> Excludes;
		const TCHAR* ExpectedCode;
	};

	TArray<FCase> Cases;
	Cases.Add({ TEXT("four excluded kinds (one past the cap)"),
		TArray<FName>(), TArray<int32>(),
		TArray<FName>{ TEXT("miner"), TEXT("cleric"), TEXT("sorcerer"), TEXT("ogre") },
		SiegeAssistantReason::ExcludeArity });
	Cases.Add({ TEXT("a repeated excluded kind"),
		TArray<FName>(), TArray<int32>(),
		TArray<FName>{ TEXT("miner"), TEXT("miner") },
		SiegeAssistantReason::DuplicateKind });
	Cases.Add({ TEXT("a selection AND an exclusion at once"),
		TArray<FName>{ TEXT("footman") }, TArray<int32>{ 10 },
		TArray<FName>{ TEXT("miner") },
		SiegeAssistantReason::ExcludeConflict });

	for (const FCase& Case : Cases)
	{
		// (a) THE FOUR-ARGUMENT CALL — the one every caller holding a whole
		//     FSiegeAssistantCommand must make.
		FString CheckedError;
		const bool bCheckedValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, CheckedError, Case.Excludes);

		TestFalse(*FString::Printf(TEXT("[%s] the 4-argument call REFUSES it"), Case.Label), bCheckedValid);
		TestEqualSensitive(*FString::Printf(TEXT("[%s] …reporting `%s`"), Case.Label, Case.ExpectedCode),
			CodeOf(CheckedError), FString(Case.ExpectedCode));

		// (b) THE DEFAULTED THREE-ARGUMENT CALL — what a dropped argument becomes.
		FString UncheckedError;
		const bool bUncheckedValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, UncheckedError);

		// ⛔ THIS IS THE MEASUREMENT. The two calls DISAGREE about the same command,
		// and the disagreement is precisely what the fourth argument buys. If a
		// future edit makes them agree — by making the parameter required, or by
		// giving the default a non-empty value — this assertion fires and the next
		// reader is sent to the call site rather than discovering it in a playtest.
		if (!TestTrue(*FString::Printf(
				TEXT("⛔ [%s] THE DEFAULTED 3-ARGUMENT CALL VALIDATES NOTHING ABOUT EXCLUSION and reports this command VALID. That is why USiegeAssistantComponent::HandleModelCompletion MUST pass Command.ExcludeKinds — dropping it compiles clean and silently turns this refusal into an acceptance."),
				Case.Label),
			bUncheckedValid && !bCheckedValid))
		{
			AddError(FString::Printf(
				TEXT("[%s] The trailing-default hazard has CHANGED SHAPE: 4-arg=%s, 3-arg=%s. Re-read SiegeAssistantValidateSelection's signature and every caller of it before touching anything else."),
				Case.Label, bCheckedValid ? TEXT("valid") : TEXT("refused"), bUncheckedValid ? TEXT("valid") : TEXT("refused")));
		}
	}

	// ── AND THE CONVERSE: THE 4-ARG CALL DID NOT BREAK THE ARRAYS-ONLY CALLERS ──
	// The default exists FOR the arrays-only call sites. A valid selection with no
	// exclusion must validate identically through both forms, or the "strictly
	// additive" claim is false for the callers that never asked for the feature.
	{
		const TArray<FName> Kinds{ TEXT("footman"), TEXT("archer") };
		const TArray<int32> Counts{ 10, 0 };

		FString ThreeArgError;
		FString FourArgError;
		const bool bThree = SiegeAssistantValidateSelection(Kinds, Counts, ThreeArgError);
		const bool bFour = SiegeAssistantValidateSelection(Kinds, Counts, FourArgError, TArray<FName>());

		TestTrue(TEXT("A valid exclusion-free selection passes BOTH call forms identically"), bThree && bFour);
		TestEqualSensitive(TEXT("…with the same (empty) error on both"), ThreeArgError, FourArgError);
	}

	// ── A LEGAL EXCLUSION IS NOT REFUSED BY THE NEW INVARIANTS ─────────────────
	{
		FString Error;
		TestTrue(TEXT("Three excluded kinds against an empty selection is VALID — the cap is inclusive"),
			SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), Error,
				TArray<FName>{ TEXT("miner"), TEXT("cleric"), TEXT("sorcerer") }));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 12 — Siegebound.Assistant.Selection.GrammarAdmitsExceptOnlyWithKinds
//
//  ⚠️ VERIFIED AGAINST THE REAL EMITTER, NOT AGAINST A HANDOFF. TASK-518's
//  handoff carried a HAND-TRANSCRIBED GBNF block, and a transcription is evidence
//  about the transcriber. Every claim below reads `USiegeAssistantGrammar::Build`.
//
//  ⛔ THE RULE NAME IS `exceptlist`, ONE WORD — a DECLARED DEPARTURE (SC-§15)
//  from AS-§20.1's `except_list`, because llama.cpp reads a rule name as
//  [a-zA-Z0-9-] and STOPS at the underscore, which is the defect `at_least`
//  shipped with and which cost TASK-413 two of its six bars. AS-§20.1 named
//  `exceptlist` as the sanctioned substitute, so this is pinned, not invented.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionGrammarAdmitsExceptOnlyWithKindsTest,
	"Siegebound.Assistant.Selection.GrammarAdmitsExceptOnlyWithKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionGrammarAdmitsExceptOnlyWithKindsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	auto RuleRhs = [](const FString& Grammar, const TCHAR* RuleName) -> FString
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
	};

	auto Alternatives = [](const FString& Rhs) -> TArray<FString>
	{
		TArray<FString> Out;
		Rhs.ParseIntoArray(Out, TEXT(" | "), /*InCullEmpty*/ true);
		return Out;
	};

	auto ReferenceCount = [](const FString& Alternative, const TCHAR* RuleName) -> int32
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
	};

	const TArray<FName> Places = SevenPlaces();

	// ── A POPULATED ROSTER — the `except` alternative is emitted ───────────────
	{
		const FString Grammar = USiegeAssistantGrammar::Build(ThirteenKindsInCardRowOrder(), Places);

		TestTrue(TEXT("A non-empty roster defines the `except` rule"), !RuleRhs(Grammar, TEXT("except")).IsEmpty());
		TestTrue(TEXT("A non-empty roster defines the `exceptlist` rule"), !RuleRhs(Grammar, TEXT("exceptlist")).IsEmpty());

		// ⛔ THE RULE NAME'S CHARSET IS LAW ZERO. `except_list` would parse as the
		// name `except`, after which llama.cpp rejects the WHOLE grammar and
		// generation runs UNCONSTRAINED — every other property in this file would
		// still be true of a string the sampler never loads.
		TestFalse(TEXT("⛔ The underscore spelling `except_list` is NOT a rule name — it would make the whole grammar unparseable"),
			Grammar.Contains(TEXT("except_list ::="), ESearchCase::CaseSensitive));

		// The JSON KEY keeps its underscore: it is wire format, and it is safe
		// because it lives inside a TERMINAL rather than in identifier position.
		TestTrue(TEXT("The `all_except` JSON key appears (inside a terminal, which is why its underscore is safe)"),
			Grammar.Contains(TEXT("all_except"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("`except` references `exceptlist`"), ReferenceCount(RuleRhs(Grammar, TEXT("except")), TEXT("exceptlist")) == 1);

		// ── `who` NOW OFFERS FOUR ALTERNATIVES ────────────────────────────────
		const FString WhoRule = RuleRhs(Grammar, TEXT("who"));
		TestEqual(TEXT("`who` offers FOUR alternatives: selection | except | \"all\" | \"none\""),
			Alternatives(WhoRule).Num(), 4);
		TestTrue(TEXT("`who` references `selection`"), ReferenceCount(WhoRule, TEXT("selection")) == 1);
		TestTrue(TEXT("`who` references `except`"), ReferenceCount(WhoRule, TEXT("except")) == 1);

		// Order matters: Zone A's `WHO =` line enumerates the shapes in the
		// grammar's own order (selection | except | "all" | "none"), and the two
		// are one contract (AS-§9c). A silent re-ordering here would leave the
		// prompt describing a different grammar than the sampler enforces.
		const TArray<FString> WhoAlternatives = Alternatives(WhoRule);
		if (WhoAlternatives.Num() == 4)
		{
			TestEqualSensitive(TEXT("`who` alternative 1 is `selection`"), WhoAlternatives[0], FString(TEXT("selection")));
			TestEqualSensitive(TEXT("`who` alternative 2 is `except`"), WhoAlternatives[1], FString(TEXT("except")));
		}

		// ── ⛔ THE DECLINED FEATURE, ENFORCED BY SHAPE ────────────────────────
		// `exceptlist` is BARE KIND STRINGS, never `item`. Referencing `item` here
		// would cost nothing to write and would silently re-introduce
		// "all except 5 archers" — the variant Jonathan DECLINED — because
		// {"kind":…,"n":…} would become a shape the sampler could reach.
		const FString ExceptListRule = RuleRhs(Grammar, TEXT("exceptlist"));
		const TArray<FString> ExceptAlternatives = Alternatives(ExceptListRule);

		TestEqual(TEXT("`exceptlist` offers exactly one alternative per permitted arity (1, 2, 3)"),
			ExceptAlternatives.Num(), SiegeAssistantMaxExclusionKinds);

		for (int32 Index = 0; Index < ExceptAlternatives.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("`exceptlist` alternative %d holds %d bare `kind` references"), Index + 1, Index + 1),
				ReferenceCount(ExceptAlternatives[Index], TEXT("kind")), Index + 1);
			TestEqual(*FString::Printf(TEXT("⛔ `exceptlist` alternative %d references `item` ZERO times — no count-controlled shape exists"), Index + 1),
				ReferenceCount(ExceptAlternatives[Index], TEXT("item")), 0);
		}

		TestFalse(TEXT("⛔ There is NO `n` key anywhere in the exclusion rules — the declined feature is INEXPRESSIBLE, not merely unimplemented"),
			ExceptListRule.Contains(TEXT("\\\"n\\\""), ESearchCase::CaseSensitive) || RuleRhs(Grammar, TEXT("except")).Contains(TEXT("\\\"n\\\""), ESearchCase::CaseSensitive));

		// ⛔ BOUNDED ALTERNATION, NEVER A REPETITION OPERATOR. An unbounded
		// repetition is exactly what a small model rambles into.
		TestFalse(TEXT("The grammar contains no '*' repetition operator"), Grammar.Contains(TEXT("*")));
		TestFalse(TEXT("The grammar contains no '+' repetition operator"), Grammar.Contains(TEXT("+")));
		TestFalse(TEXT("The grammar contains no '?' optional operator"), Grammar.Contains(TEXT("?")));

		// Determinism survives the new rules: same state in, byte-identical
		// grammar out. This is what keeps the sampler's constraint stable across
		// turns, and *Sensitive is required because it is a byte claim.
		TestEqualSensitive(TEXT("The grammar is still byte-deterministic with the exclusion rules present"),
			USiegeAssistantGrammar::Build(ThirteenKindsInCardRowOrder(), Places), Grammar);
	}

	// ── A ONE-KIND ROSTER — degradation to REFUSABLE, not to UNREACHABLE ───────
	// At one kind, `exceptlist`'s 2- and 3-kind alternatives can only produce the
	// SAME symbol twice, which the parser refuses with DuplicateKind. That is
	// deliberate and it is `selection`'s own long-standing property; pinned here
	// so nobody "fixes" it by bounding the rule on the live kind count and making
	// the two rules disagree about their own construction.
	{
		const FString Grammar = USiegeAssistantGrammar::Build(TArray<FName>{ TEXT("footman") }, Places);
		TestEqual(TEXT("A one-kind roster still emits all three `exceptlist` arities (bounded by the CAP, not by the board)"),
			Alternatives(RuleRhs(Grammar, TEXT("exceptlist"))).Num(), SiegeAssistantMaxExclusionKinds);

		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("…and the parser refuses the degenerate `[\"footman\",\"footman\"]` it admits"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"footman\",\"footman\"]}")), Command, Error));
	}

	// ── AN EMPTY ROSTER — the rule is OMITTED and `who` falls back to two ──────
	// An empty roster has nothing to EXCLUDE for exactly the reason it has nothing
	// to select, and emitting the alternative anyway would leave `exceptlist`
	// referencing an UNDEFINED `kind` rule — which breaks the whole grammar.
	{
		const FString Grammar = USiegeAssistantGrammar::Build(TArray<FName>(), Places);

		TestFalse(TEXT("An empty roster OMITS `except`"), Grammar.Contains(TEXT("except ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("An empty roster OMITS `exceptlist`"), Grammar.Contains(TEXT("exceptlist ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("An empty roster OMITS `kind` (which `exceptlist` would otherwise dangle on)"),
			Grammar.Contains(TEXT("kind ::="), ESearchCase::CaseSensitive));

		TestEqual(TEXT("An empty roster leaves `who` with the two whole-army selectors — UNCHANGED by this batch"),
			Alternatives(RuleRhs(Grammar, TEXT("who"))).Num(), 2);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 13 — Siegebound.Assistant.Selection.PositionalAcceptKeyResolves
//
//  THE CONFIRM UX's ONE HEADLESSLY-REACHABLE SEAM (TASK-516 / TASK-519,
//  AS-§20.5, KBD-§8). `USiegeAssistantConsoleWidget::GetAcceptKey()` is
//  `LayoutSubsystem->GetPositionalKey(EKeys::Z)` with an `EKeys::Z` fallback;
//  the widget's own handler is unreachable from an EditorContext test (see the
//  file header), but the ACCESSOR IT COMPARES AGAINST is not.
//
//  ⛔ THE `EKeys::Invalid` PIN IS THE POINT. TASK-516 flagged that the "obvious
//  simplification" `TranslationMap.FindRef(QwertyKey)` returns a
//  default-constructed FKey on a miss, whose KeyName is NAME_None — and
//  `EKeys::Invalid` IS `FKey(NAME_None)`, compared by KeyName alone. So FindRef
//  returns EXACTLY the one value this function is forbidden to return, ON THE
//  SINGLE MOST COMMON PATH: a QWERTY host, where the map is EMPTY and every
//  lookup misses. A console whose accept key is Invalid accepts nothing, and the
//  player cannot confirm an order.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionPositionalAcceptKeyResolvesTest,
	"Siegebound.Assistant.Selection.PositionalAcceptKeyResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionPositionalAcceptKeyResolvesTest::RunTest(const FString& Parameters)
{
	// ⛔ The test seam announces itself at Warning ON PURPOSE — its own comment says
	// "this line must never appear in a shipped session" — so it is expected
	// traffic here and nowhere else.
	AddExpectedMessagePlain(TEXT("AUTOMATION OVERRIDE"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	// UGameInstanceSubsystem is UCLASS(Within = GameInstance), so the outer is not
	// optional — a bare NewObject trips the ClassWithin check. Initialize() is
	// never called, which is deliberate: no OS probe and no 1 Hz timer run here, so
	// the only translation in play is the one injected below.
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GetTransientPackageAsObject()));
	if (!TestTrue(TEXT("A throwaway UGameInstance was created"), GameInstance.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<USiegeKeyboardLayoutSubsystem> Layout(NewObject<USiegeKeyboardLayoutSubsystem>(GameInstance.Get()));
	if (!TestTrue(TEXT("A keyboard-layout subsystem was created inside it"), Layout.IsValid()))
	{
		return false;
	}

	// ── (a) THE QWERTY HOST — AN EMPTY MAP, WHICH IS THE MISS PATH ─────────────
	Layout->SetTranslationMapForAutomationTests(TMap<FKey, FKey>());
	{
		const FKey Resolved = Layout->GetPositionalKey(EKeys::Z);

		TestTrue(TEXT("⭐ On a QWERTY host GetPositionalKey(Z) is IDENTITY — it returns Z"), Resolved == EKeys::Z);

		// ⛔ THE FORBIDDEN VALUE, ASSERTED BY NAME. This is the assertion that
		// fails the day someone "simplifies" the Find-then-fallback into FindRef.
		TestFalse(TEXT("⛔ It is NEVER EKeys::Invalid — the exact value TMap::FindRef would return here (TASK-516's flagged simplification)"),
			Resolved == EKeys::Invalid);
		TestTrue(TEXT("⛔ The resolved accept key is a VALID FKey"), Resolved.IsValid());

		// ⛔ AND IT IS NEVER `Escape`. AS-§6 A-2 is CLOSED on "leave Escape alone,
		// permanently" and names NativeOnPreviewKeyDown as a way to break it. The
		// handler consumes exactly one key — the one resolved here — so an accept
		// key that resolved to Escape would absorb it and overturn the ruling.
		TestFalse(TEXT("⛔ The accept key is NEVER EKeys::Escape — AS-§6 A-2 is CLOSED and Escape must stay unabsorbed"),
			Resolved == EKeys::Escape);
	}

	// ── (b) A NON-QWERTY HOST — THE HIT PATH, AND THE DIRECTION ───────────────
	// US-Dvorak prints `;` at the physical position QWERTY prints `Z` on, so the
	// map is SOURCE (QWERTY) -> what the ACTIVE layout yields THERE.
	// ⛔ Getting the direction backwards compiles and silently binds the wrong key.
	{
		TMap<FKey, FKey> Dvorak;
		Dvorak.Add(EKeys::Z, EKeys::Semicolon);
		Dvorak.Add(EKeys::W, EKeys::Comma);
		Layout->SetTranslationMapForAutomationTests(Dvorak);

		const FKey Resolved = Layout->GetPositionalKey(EKeys::Z);

		TestTrue(TEXT("⭐ On US-Dvorak GetPositionalKey(Z) TRANSLATES to Semicolon — the key at the physical Z position"), Resolved == EKeys::Semicolon);
		TestFalse(TEXT("⛔ Still never EKeys::Invalid on the hit path"), Resolved == EKeys::Invalid);
		TestFalse(TEXT("⛔ Still never EKeys::Escape on the hit path"), Resolved == EKeys::Escape);

		// A key ABSENT from a NON-EMPTY map still falls back to identity — the
		// second miss path, and the one a FindRef "simplification" would also break.
		TestTrue(TEXT("A letter absent from a populated map still resolves to ITSELF, not to Invalid"),
			Layout->GetPositionalKey(EKeys::Q) == EKeys::Q);
	}

	// ── (c) EVERY LETTER POSITION, ON BOTH MAPS ───────────────────────────────
	// ⛔ KBD-§4 scopes the feature to LETTERS ONLY, all 26. Sweeping the shipped
	// scan-code table means the two forbidden values are excluded for every key the
	// accept resolver could ever be pointed at, not just for `Z`.
	{
		const TArray<FSiegePositionalKeyProbe> Probes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();
		TestEqual(TEXT("The shipped table still covers all 26 letter positions"), Probes.Num(), 26);

		Layout->SetTranslationMapForAutomationTests(TMap<FKey, FKey>());

		int32 InvalidCount = 0;
		int32 EscapeCount = 0;
		for (const FSiegePositionalKeyProbe& Probe : Probes)
		{
			const FKey Resolved = Layout->GetPositionalKey(Probe.QwertyKey);
			if (!Resolved.IsValid() || Resolved == EKeys::Invalid)
			{
				++InvalidCount;
			}
			if (Resolved == EKeys::Escape)
			{
				++EscapeCount;
			}
		}

		TestEqual(TEXT("⛔ NOT ONE of the 26 letter positions resolves to EKeys::Invalid on a QWERTY host"), InvalidCount, 0);
		TestEqual(TEXT("⛔ NOT ONE of the 26 letter positions resolves to EKeys::Escape — Escape cannot become an absorbed key by this route"), EscapeCount, 0);
	}

	// ── (d) THE PLAYER-FACING PROMPT STILL SAYS `Z` ───────────────────────────
	// AS-§20.5: the key is resolved POSITIONALLY but the prompt says `Z` on every
	// layout, because the player is looking at a keycap. Restated here as the
	// reason the two must NOT be derived from one another; the on-screen string
	// itself is TASK-527's pixel check, not this file's.
	AddInfo(TEXT("The accept key is resolved positionally (GetPositionalKey(EKeys::Z)); the player-facing prompt says `Z` on every layout by design (AS-§20.5). The on-screen string is a human pixel check at TASK-527 and is deliberately NOT asserted here."));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
