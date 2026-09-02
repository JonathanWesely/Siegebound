// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"        // GEngine — the ULocalPlayer outer's own `Within = Engine`.
#include "Engine/LocalPlayer.h"   // ULocalPlayer — the REQUIRED outer (see the fixture note).
#include "Siegebound/SiegeMapMark.h"
#include "Siegebound/SiegeMapMarkSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Templates/TypeCompatibleBytes.h"   // BitCast — the quiet-NaN fixture in test 6.
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the MAP-MARK STORE ═══
 *  (MARKS batch, TASK-744: `FSiegeMapMark` + `USiegeMapMarkSubsystem`. Law: `MARK-§1`,
 *   `MARK-§3` (M-1..M-6), `MARK-§5`, `MARK-§6`, `HIGH-§1`, `SHIP-§9c`, `SC-§13`.
 *   QA gate: TASK-753. Compile + suite gate: TASK-754.)
 *
 *  ⛔⛔ `SHIP-§9c` IS THE STANDARD THIS FILE IS WRITTEN TO: **an assertion that cannot FAIL is
 *      a status line, not a gate.** Every claim below was chosen against a SPECIFIC wrong
 *      implementation that a reviewer would plausibly wave through, and each test says which
 *      one at its head. ⛔ Nothing here is asserted against a value the code under test
 *      computed for the expectation as well as for the answer.
 *
 *  ── WHAT EACH TEST WOULD CATCH, IN ONE LINE EACH (the reason the file exists) ──────────────
 *    1. SymbolIsCircleUnderscoreNumber ....... a BARE DIGIT symbol (`"1"`), which Zone A has
 *                                              already taught the model means a COUNT
 *                                              (`MARK-§2`); a case/spelling drift; `circle_0`
 *                                              handed out for an invalid number.
 *    2. SequentialAssignment ................. 0-based numbering; the array INDEX used as the
 *                                              identity; a symbol that disagrees with the number.
 *    3. DeleteLeavesTheHoleAndNeverRenumbers . ⭐ THE `M-1` ASSERTION — a compaction/renumber
 *                                              pass that silently redirects an order the player
 *                                              has composed but not yet sent.
 *    4. LowestFreeNumberIsReused ............. `Num() + 1` or `Highest + 1` allocation, both of
 *                                              which skip the hole and exhaust the cap early.
 *    5. CapRefusesAndMutatesNothing .......... an off-by-one cap; a SILENT no-op that returns
 *                                              true; a refused add that still clobbers OutMark.
 *    6. RadiusIsClamped ...................... no clamp at all; a clamp that pins everything to
 *                                              one bound; a NaN reaching a stored mark.
 *    7. ClearMarksEmptiesAndNumberingRestarts  a monotonic "next id" member that survives the
 *                                              clear, so the new match's first circle is not 1.
 *    8. RemoveRefusesUnknownNumbers .......... ⭐ REMOVE-BY-INDEX WEARING REMOVE-BY-NUMBER'S
 *                                              CLOTHES: `RemoveMark(0)` deleting the first mark.
 *    9. StoreIsLocalPlayerScoped ............. a reparent to `UGameInstanceSubsystem` /
 *                                              `UWorldSubsystem`, which would break `M-2` and
 *                                              void the `MARK-§6` M8 declaration STRUCTURALLY;
 *                                              and a cap silently raised past Jonathan's 9.
 *
 *  ⛔ WHAT THESE TESTS CANNOT PROVE, STATED SO NOBODY MISTAKES GREEN FOR DONE (`SC-§32`):
 *    • Nothing here opens PIE, builds a Slate tree, clicks a map or turns a wheel. That a mark
 *      DRAWS where it was clicked closes on TASK-745's own tests and, for looks, on pixels or
 *      Jonathan's eyes (`AS-§6` A(e)).
 *    • Nothing here builds a prompt. That `circle_N` is published into the per-match place list
 *      and answered by `ResolvePlace` is TASK-746's gate, and the ZONE-A FREEZE
 *      (`Siegebound.Assistant.ZoneA.MeasuredCharCount` = 5658) is asserted by that file, not
 *      this one — ⛔ this batch does not touch Zone A at all (`MARK-§1`).
 *    • ⛔ NO CALLER OF `ClearMarks()` IS ASSERTED HERE, because this task ships none — see the
 *      handoff's DECLARED GAP (`M-4`'s match-reset hook lives in `ASiegeGameMode::PlayAgain`,
 *      a file TASK-750 sole-owns).
 *
 *  M8 DECLARATION (verbatim): adds no replicated property, no new replicated class, no RPC, no
 *  new relevancy tier. This is a test file; it adds no shipped surface at all.
 */

namespace SiegeMapMarkTestFixture
{
	/**
	 *  A radius comfortably INSIDE `[MinMarkRadiusUU, MaxMarkRadiusUU]`, so that every test
	 *  that is not about clamping gets its value back byte-for-byte. ⚠️ If a future retune
	 *  moves the shipped bounds past this number, test 6's first assertion goes red and names
	 *  the reason — which is the correct outcome, not a flake.
	 */
	static constexpr float InRangeRadiusUU = 1000.f;

	/** Float comparison tolerance for radii, in uu. Generous, because no assertion here turns on a sub-uu difference. */
	static constexpr float RadiusToleranceUU = 0.01f;

	/** Double comparison tolerance for world coordinates, in uu. Same reasoning. */
	static constexpr double PositionToleranceUu = 0.01;

	/**
	 *  A `USiegeMapMarkSubsystem` inside a throwaway `ULocalPlayer`.
	 *
	 *  ⚠️ THE OUTER IS NOT OPTIONAL: `ULocalPlayerSubsystem` is `UCLASS(Abstract, Within =
	 *  LocalPlayer)` (Engine/Public/Subsystems/LocalPlayerSubsystem.h), so a bare `NewObject`
	 *  lands in the transient package and trips the `ClassWithin` check in
	 *  `StaticAllocateObject` (UObjectGlobals.cpp:3313). Same shape as
	 *  `SiegeControlsHelpTest.cpp:104-135`'s `UGameInstance` fixture, cloned rather than
	 *  re-invented — only the outer class differs.
	 *
	 *  ⭐ THE `ULocalPlayer` IS SAFE TO FABRICATE AND IT WAS CHECKED AT THE SOURCE, ⛔ not
	 *  assumed: its constructor (LocalPlayer.cpp:232-237) only sets
	 *  `PendingLevelPlayerControllerClass`, and its subsystem COLLECTION is initialised in
	 *  `PlayerAdded` (:262), which nothing here calls. ⇒ no viewport, no controller, no world,
	 *  and ⛔ no second copy of the subsystem under test.
	 *
	 *  ⛔ `Initialize(FSubsystemCollectionBase&)` IS NEVER CALLED ON THE SUBSYSTEM, and that is
	 *  correct rather than a shortcut: this class deliberately has no override (see its header)
	 *  — the array default-constructs empty, so a `NewObject` instance is already the exact
	 *  object the game uses.
	 */
	struct FScratchStore
	{
		TStrongObjectPtr<ULocalPlayer>           LocalPlayer;
		TStrongObjectPtr<USiegeMapMarkSubsystem> Marks;

		bool IsValid() const { return LocalPlayer.IsValid() && Marks.IsValid(); }
	};

	static FScratchStore MakeScratchStore()
	{
		FScratchStore Scratch;

		if (GEngine == nullptr)
		{
			// ⛔ Deliberately NOT a silent skip: the caller turns an invalid fixture into a
			// FAILED test. A suite that quietly passes when it never ran is the fake gate
			// `SHIP-§9c` exists to forbid.
			return Scratch;
		}

		// ULocalPlayer is `UCLASS(Within = Engine)`, so GEngine is its required outer.
		Scratch.LocalPlayer.Reset(NewObject<ULocalPlayer>(GEngine));

		if (Scratch.LocalPlayer.IsValid())
		{
			Scratch.Marks.Reset(NewObject<USiegeMapMarkSubsystem>(Scratch.LocalPlayer.Get()));
		}

		return Scratch;
	}

	/** A distinct, recognisable world position per index — so "which mark is this?" is answerable from its coordinates alone. */
	static FVector2D PositionForIndex(const int32 Index)
	{
		return FVector2D(1000.0 * static_cast<double>(Index + 1), -500.0 * static_cast<double>(Index + 1));
	}

	/** The numbers of the live marks, in array order. The array is held ascending by `Number`, so this doubles as an ordering check. */
	static TArray<int32> LiveNumbers(const USiegeMapMarkSubsystem& Store)
	{
		TArray<int32> Numbers;
		Numbers.Reserve(Store.GetMarks().Num());

		for (const FSiegeMapMark& Mark : Store.GetMarks())
		{
			Numbers.Add(Mark.Number);
		}

		return Numbers;
	}

	/** "1, 3, 4" — readable in a failure message, which is most of what makes a red test useful. */
	static FString DescribeNumbers(const TArray<int32>& Numbers)
	{
		TArray<FString> Parts;
		Parts.Reserve(Numbers.Num());

		for (const int32 Number : Numbers)
		{
			Parts.Add(FString::FromInt(Number));
		}

		return FString::Join(Parts, TEXT(", "));
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. THE SYMBOL SEAM — `circle_1`, by BYTE-SENSITIVE string equality
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkSymbolTest,
	"Siegebound.MapMarks.SymbolIsCircleUnderscoreNumber",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkSymbolTest::RunTest(const FString& Parameters)
{
	// ⭐ THIS IS THE ONE SEAM THE WIDGET (745) AND THE SNAPSHOT (746) MUST AGREE ON, and it is
	// asserted with ⛔ no world, ⛔ no widget and ⛔ no snapshot — which is exactly why
	// `MakeSymbol` is a pure static (`MARK-§5`).
	//
	// ⛔ `TestEqualSensitive`, ⛔ NEVER `TestEqual` (`SC-§13`): `TestEqual` on two FStrings
	// compares case-INSENSITIVELY, so `"CIRCLE_1"` would sail through it and a byte claim made
	// with it would be vacuous.
	TestEqualSensitive(TEXT("MakeSymbol(1) is exactly \"circle_1\""), FSiegeMapMark::MakeSymbol(1), FString(TEXT("circle_1")));
	TestEqualSensitive(TEXT("MakeSymbol(2) is exactly \"circle_2\""), FSiegeMapMark::MakeSymbol(2), FString(TEXT("circle_2")));
	TestEqualSensitive(TEXT("MakeSymbol(9) is exactly \"circle_9\" (the shipped cap)"), FSiegeMapMark::MakeSymbol(9), FString(TEXT("circle_9")));

	// ⛔ THE COLLISION THIS SPELLING EXISTS TO AVOID (`MARK-§2`): Zone A already ships
	// `COUNT = 1 to 30`, so a bare `1` in the `where` field is a token the model has been
	// taught means a QUANTITY. A symbol that is a bare digit is the defect, not a shorter name.
	TestNotEqual(TEXT("MakeSymbol(1) is NOT the bare digit \"1\" — that token already means a COUNT in Zone A"),
		FSiegeMapMark::MakeSymbol(1), FString(TEXT("1")));

	// The shipped underscore family (`own_castle`, `nearest_mine`, `ancient_ground_near`) — a
	// space or a hyphen here would break the place-symbol shape while still "looking fine".
	// ⛔ Counted rather than compared: `FString::operator==` is case-INSENSITIVE (`SC-§13`), so
	// a second `==` claim here would add nothing the sensitive assertions above have not made.
	const FString Symbol3 = FSiegeMapMark::MakeSymbol(3);

	int32 UnderscoreCount = 0;
	for (const TCHAR Character : Symbol3)
	{
		if (Character == TEXT('_'))
		{
			++UnderscoreCount;
		}
	}

	TestEqual(TEXT("The symbol carries exactly ONE underscore, between the word and the number"), UnderscoreCount, 1);
	TestFalse(TEXT("The symbol contains no space"), Symbol3.Contains(TEXT(" ")));
	TestFalse(TEXT("The symbol contains no hyphen"), Symbol3.Contains(TEXT("-")));

	// ⛔ AN INVALID NUMBER IS THE EMPTY STRING, ⛔ NEVER `circle_0` / `circle_-1`. A
	// well-formed-but-meaningless symbol is this project's named failure class
	// (valid-shaped-wrong-command): it would look real in the input box, sample legally out of
	// a grammar built from a list containing it, and then resolve to nothing.
	TestTrue(TEXT("MakeSymbol(0) is the EMPTY string, not \"circle_0\""), FSiegeMapMark::MakeSymbol(0).IsEmpty());
	TestTrue(TEXT("MakeSymbol(-1) is the EMPTY string, not \"circle_-1\""), FSiegeMapMark::MakeSymbol(-1).IsEmpty());

	// The allocator's floor and the symbol's floor are the SAME named constant — if they ever
	// diverged, the store would hand out a number whose symbol is the empty string.
	TestEqual(TEXT("FirstMarkNumber is 1 — numbering starts where a human counts"), FSiegeMapMark::FirstMarkNumber, 1);
	TestFalse(TEXT("The FIRST legal number does produce a symbol (the two floors agree)"),
		FSiegeMapMark::MakeSymbol(FSiegeMapMark::FirstMarkNumber).IsEmpty());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. SEQUENTIAL ASSIGNMENT — the first circle is 1, the next 2, the next 3
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkSequentialAssignmentTest,
	"Siegebound.MapMarks.SequentialAssignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkSequentialAssignmentTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;

	TestEqual(TEXT("A fresh store holds no marks"), Store.GetMarks().Num(), 0);

	// ⛔ THE EXPECTATIONS ARE LITERALS, ⛔ NOT `MakeSymbol` CALLED A SECOND TIME. Asserting
	// `MakeSymbol(Added.Number) == MakeSymbol(Index + 1)` would be an equality the CODE UNDER
	// TEST produces on BOTH sides — it would only ever restate the number check above, and it
	// is exactly the "equal by construction" assertion `SHIP-§9c` calls a status line.
	const TCHAR* const ExpectedSymbols[] = { TEXT("circle_1"), TEXT("circle_2"), TEXT("circle_3") };

	// Jonathan's own words: *"The first circle you create is circle 1, the next number you
	// create is circle 2. The next number you create is circle 3."*
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSiegeMapMark Added;
		const bool bAdded = Store.AddMark(PositionForIndex(Index), InRangeRadiusUU, Added);

		TestTrue(*FString::Printf(TEXT("Add %d succeeds"), Index + 1), bAdded);
		TestEqual(*FString::Printf(TEXT("Add %d is numbered %d — ⛔ not %d (0-based) and ⛔ not its array index"),
			Index + 1, Index + 1, Index), Added.Number, Index + 1);

		// ⭐ THE SEAM, END TO END: the string the map would insert and the snapshot would
		// publish for the mark that was just placed. A store that returned a plausible number
		// but filed a different one fails here even if it passed the line above.
		TestEqualSensitive(*FString::Printf(TEXT("Add %d publishes exactly \"%s\""), Index + 1, ExpectedSymbols[Index]),
			FSiegeMapMark::MakeSymbol(Added.Number), FString(ExpectedSymbols[Index]));

		// The position and radius survive the store verbatim (in-range radius ⇒ no clamping).
		TestEqual(TEXT("The stored X is the clicked X"), Added.WorldXY.X, PositionForIndex(Index).X, PositionToleranceUu);
		TestEqual(TEXT("The stored Y is the clicked Y"), Added.WorldXY.Y, PositionForIndex(Index).Y, PositionToleranceUu);
		TestEqual(TEXT("An in-range radius is stored unchanged"), Added.RadiusUU, InRangeRadiusUU, RadiusToleranceUU);
	}

	const TArray<int32> Numbers = LiveNumbers(Store);
	TestEqual(TEXT("Three marks are live"), Store.GetMarks().Num(), 3);

	if (Numbers.Num() == 3)
	{
		TestEqual(TEXT("The live numbers are 1, 2, 3 in ascending order — slot 1"), Numbers[0], 1);
		TestEqual(TEXT("The live numbers are 1, 2, 3 in ascending order — slot 2"), Numbers[1], 2);
		TestEqual(TEXT("The live numbers are 1, 2, 3 in ascending order — slot 3"), Numbers[2], 3);
	}

	// `FindMark` answers by NUMBER, and it answers with the mark whose POSITION was clicked for
	// that number — the pairing, not merely the count.
	const FSiegeMapMark* const Second = Store.FindMark(2);
	if (TestNotNull(TEXT("FindMark(2) resolves"), Second))
	{
		TestEqual(TEXT("FindMark(2) is the mark clicked SECOND — X"), Second->WorldXY.X, PositionForIndex(1).X, PositionToleranceUu);
		TestEqual(TEXT("FindMark(2) is the mark clicked SECOND — Y"), Second->WorldXY.Y, PositionForIndex(1).Y, PositionToleranceUu);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⭐⭐ THE `M-1` ASSERTION — A DELETE LEAVES THE HOLE AND RENUMBERS NOTHING
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkDeleteLeavesHoleTest,
	"Siegebound.MapMarks.DeleteLeavesTheHoleAndNeverRenumbers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkDeleteLeavesHoleTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	// ⚖️ THIS IS THE ASSERTION THAT MATTERS MOST IN THE FILE, AND THE REASON IS THE AIRLOCK,
	// ⛔ NOT ERGONOMICS (`M-1`): the map writes a symbol into the console input box and THE
	// PLAYER SENDS IT HIMSELF, so an arbitrary amount of time passes between composing
	// `circle_3` and pressing Enter. A store that compacted 3 → 2 after mark 2 was deleted
	// would silently redirect an order the player has ALREADY GIVEN onto different ground.
	// A renumbering implementation passes every count-based check and fails exactly here.
	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSiegeMapMark Added;
		Store.AddMark(PositionForIndex(Index), InRangeRadiusUU, Added);
	}

	if (!TestEqual(TEXT("Precondition: marks 1, 2 and 3 are live"), Store.GetMarks().Num(), 3))
	{
		return false;
	}

	TestTrue(TEXT("RemoveMark(2) — the MIDDLE of the list — succeeds"), Store.RemoveMark(2));
	TestEqual(TEXT("Two marks survive"), Store.GetMarks().Num(), 2);

	const TArray<int32> Numbers = LiveNumbers(Store);

	// ⛔ THE SURVIVORS ARE 1 AND **3**. A renumbering store would answer 1 and 2 here — the
	// same COUNT, the same ORDER, and a completely different meaning.
	if (TestEqual(TEXT("Two numbers survive"), Numbers.Num(), 2))
	{
		TestEqual(*FString::Printf(TEXT("The first survivor is still 1 (live numbers: %s)"), *DescribeNumbers(Numbers)),
			Numbers[0], 1);
		TestEqual(*FString::Printf(TEXT("⭐ The second survivor is still **3**, NOT compacted to 2 (live numbers: %s)"), *DescribeNumbers(Numbers)),
			Numbers[1], 3);
	}

	// ⭐ AND THE NUMBER STILL POINTS AT THE SAME GROUND. This is the half of `M-1` that a
	// numbers-only assertion would miss: a store that renumbered 3 → 2 AND kept a set of size
	// two would still be handing `circle_2` a different position than the player drew it at.
	const FSiegeMapMark* const Third = Store.FindMark(3);
	if (TestNotNull(TEXT("FindMark(3) still resolves after mark 2 was deleted"), Third))
	{
		TestEqual(TEXT("Mark 3 still denotes the ground it was drawn on — X"), Third->WorldXY.X, PositionForIndex(2).X, PositionToleranceUu);
		TestEqual(TEXT("Mark 3 still denotes the ground it was drawn on — Y"), Third->WorldXY.Y, PositionForIndex(2).Y, PositionToleranceUu);
	}

	const FSiegeMapMark* const First = Store.FindMark(1);
	if (TestNotNull(TEXT("FindMark(1) still resolves"), First))
	{
		TestEqual(TEXT("Mark 1 still denotes the ground it was drawn on — X"), First->WorldXY.X, PositionForIndex(0).X, PositionToleranceUu);
	}

	// ⛔ THE DELETED NUMBER RESOLVES TO NOTHING — it does NOT quietly resolve to a neighbour.
	// That `nullptr` is what makes a stale `circle_2` sitting in the player's input box mean
	// "no such place" instead of "somebody else's ground".
	TestNull(TEXT("FindMark(2) — the HOLE — resolves to nothing at all"), Store.FindMark(2));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. LOWEST-FREE REUSE — a new circle takes the HOLE, never Num()+1
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkLowestFreeNumberTest,
	"Siegebound.MapMarks.LowestFreeNumberIsReused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkLowestFreeNumberTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSiegeMapMark Added;
		Store.AddMark(PositionForIndex(Index), InRangeRadiusUU, Added);
	}

	Store.RemoveMark(2);

	// ⛔ 2, ⛔ NOT 3 (`Num() + 1`) AND ⛔ NOT 4 (`Highest + 1`). Both of those are the "obvious"
	// implementation, both compile, and both would exhaust Jonathan's cap of 9 in a match where
	// he only ever has three circles on screen.
	FSiegeMapMark Reused;
	const FVector2D NewGround(-4242.0, 777.0);
	TestTrue(TEXT("A new mark is accepted while a hole exists"), Store.AddMark(NewGround, InRangeRadiusUU, Reused));
	TestEqual(TEXT("⭐ The new mark takes the HOLE — number 2, not 4 (Num()+1) and not 4 (Highest+1)"), Reused.Number, 2);

	// The reused number denotes the NEW ground — proving the hole was FILLED, not that the old
	// mark was resurrected.
	const FSiegeMapMark* const Filled = Store.FindMark(2);
	if (TestNotNull(TEXT("FindMark(2) resolves again once the hole is filled"), Filled))
	{
		TestEqual(TEXT("Mark 2 now denotes the NEWLY clicked ground — X"), Filled->WorldXY.X, NewGround.X, PositionToleranceUu);
		TestEqual(TEXT("Mark 2 now denotes the NEWLY clicked ground — Y"), Filled->WorldXY.Y, NewGround.Y, PositionToleranceUu);
	}

	const TArray<int32> Refilled = LiveNumbers(Store);
	if (TestEqual(TEXT("Three marks are live again"), Refilled.Num(), 3))
	{
		// The ascending-by-Number invariant: the refilled 2 sits BETWEEN 1 and 3, not at the end.
		TestEqual(*FString::Printf(TEXT("Live numbers read 1, 2, 3 in ascending order (got: %s)"), *DescribeNumbers(Refilled)), Refilled[0], 1);
		TestEqual(*FString::Printf(TEXT("Live numbers read 1, 2, 3 in ascending order (got: %s)"), *DescribeNumbers(Refilled)), Refilled[1], 2);
		TestEqual(*FString::Printf(TEXT("Live numbers read 1, 2, 3 in ascending order (got: %s)"), *DescribeNumbers(Refilled)), Refilled[2], 3);
	}

	// ⭐ "LOWEST free", ⛔ not "some free": with 1 and 3 open, the next add must take 1.
	// An allocator that scanned from the highest, or that remembered where it stopped last
	// time, passes everything above and fails here.
	TestTrue(TEXT("RemoveMark(1) succeeds"), Store.RemoveMark(1));
	TestTrue(TEXT("RemoveMark(3) succeeds"), Store.RemoveMark(3));
	TestEqual(TEXT("Only mark 2 is left"), Store.GetMarks().Num(), 1);

	FSiegeMapMark Lowest;
	TestTrue(TEXT("An add succeeds with holes at 1 and 3"), Store.AddMark(PositionForIndex(7), InRangeRadiusUU, Lowest));
	TestEqual(TEXT("⭐ It takes 1 — the LOWEST free number, not 3 and not 4"), Lowest.Number, 1);

	FSiegeMapMark Next;
	TestTrue(TEXT("A further add succeeds"), Store.AddMark(PositionForIndex(8), InRangeRadiusUU, Next));
	TestEqual(TEXT("The one after that takes 3 — the next lowest free number"), Next.Number, 3);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. THE CAP — it REFUSES at MaxMapMarks, and a refusal mutates NOTHING
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkCapTest,
	"Siegebound.MapMarks.CapRefusesAndMutatesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkCapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;
	const int32 Cap = Store.MaxMapMarks;

	// ⛔ The loop runs to the TUNABLE, not to a literal 9 — the cap is `EditDefaultsOnly`
	// precisely so Jonathan can retune it, and a test hardcoding 9 HERE would go red for the
	// wrong reason on a legitimate retune. (The shipped DEFAULT of 9 is pinned separately, in
	// test 9, where re-basing it is a named decision rather than a silent one.)
	for (int32 Index = 0; Index < Cap; ++Index)
	{
		FSiegeMapMark Added;
		TestTrue(*FString::Printf(TEXT("Add %d of %d (up to the cap) succeeds"), Index + 1, Cap),
			Store.AddMark(PositionForIndex(Index), InRangeRadiusUU, Added));
		TestEqual(*FString::Printf(TEXT("Add %d is numbered %d"), Index + 1, Index + 1), Added.Number, Index + 1);
	}

	TestEqual(TEXT("The store now holds exactly MaxMapMarks marks"), Store.GetMarks().Num(), Cap);

	// ⛔ THE REFUSAL. A SENTINEL out-parameter proves the "mutates NOTHING" half of the pinned
	// contract: a refused add that still wrote to OutMark would hand the caller a plausible
	// mark it must not draw, and TASK-745 would draw it.
	FSiegeMapMark Sentinel;
	Sentinel.Number   = -777;
	Sentinel.WorldXY  = FVector2D(-123456.0, 654321.0);
	Sentinel.RadiusUU = -1.f;

	const bool bRefused = Store.AddMark(FVector2D(999.0, 999.0), InRangeRadiusUU, Sentinel);

	TestFalse(TEXT("⭐ The add PAST the cap returns FALSE — ⛔ never a silent no-op that returns true"), bRefused);
	TestEqual(TEXT("The store still holds exactly MaxMapMarks marks — the refusal added nothing"), Store.GetMarks().Num(), Cap);

	TestEqual(TEXT("A refused add leaves OutMark.Number untouched (the sentinel survives)"), Sentinel.Number, -777);
	TestEqual(TEXT("A refused add leaves OutMark.WorldXY untouched — X"), Sentinel.WorldXY.X, -123456.0, PositionToleranceUu);
	TestEqual(TEXT("A refused add leaves OutMark.WorldXY untouched — Y"), Sentinel.WorldXY.Y, 654321.0, PositionToleranceUu);
	TestEqual(TEXT("A refused add leaves OutMark.RadiusUU untouched"), Sentinel.RadiusUU, -1.f, RadiusToleranceUU);

	// ⛔ AND NO NUMBER WAS CONSUMED BY THE REFUSAL: freeing one slot must make exactly one add
	// possible again, at the freed number. An allocator that had incremented a counter on the
	// refused attempt would answer with a different number here.
	TestTrue(*FString::Printf(TEXT("RemoveMark(%d) frees the top slot"), Cap), Store.RemoveMark(Cap));

	FSiegeMapMark AfterFree;
	TestTrue(TEXT("One add is possible again after one removal"), Store.AddMark(FVector2D(1.0, 2.0), InRangeRadiusUU, AfterFree));
	TestEqual(TEXT("It takes the freed number — the refusal consumed nothing"), AfterFree.Number, Cap);

	FSiegeMapMark SecondSentinel;
	SecondSentinel.Number = -999;
	TestFalse(TEXT("And the store is full again immediately — the cap is a limit, not a one-shot"),
		Store.AddMark(FVector2D(3.0, 4.0), InRangeRadiusUU, SecondSentinel));
	TestEqual(TEXT("The second refusal also left its OutMark untouched"), SecondSentinel.Number, -999);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. THE RADIUS GATE — clamped on add AND on resize, NaN included
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkRadiusClampTest,
	"Siegebound.MapMarks.RadiusIsClamped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkRadiusClampTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;
	const float MinRadius = Store.MinMarkRadiusUU;
	const float MaxRadius = Store.MaxMarkRadiusUU;

	// Fixture self-check: the assertions below are only meaningful if the shipped bounds
	// actually bracket the in-range value. ⛔ Without this, "clamped to min" and "unchanged"
	// could be the same number and the whole test would be vacuous.
	if (!TestTrue(TEXT("Fixture: the shipped bounds bracket InRangeRadiusUU (min < in-range < max)"),
		MinRadius < InRangeRadiusUU && InRangeRadiusUU < MaxRadius))
	{
		return false;
	}

	// ── ON ADD ────────────────────────────────────────────────────────────────
	FSiegeMapMark TooSmall;
	TestTrue(TEXT("An add with a zero radius still succeeds"), Store.AddMark(PositionForIndex(0), 0.f, TooSmall));
	TestEqual(TEXT("⭐ A zero radius is raised to MinMarkRadiusUU — ⛔ not stored as 0"), TooSmall.RadiusUU, MinRadius, RadiusToleranceUU);

	FSiegeMapMark TooLarge;
	TestTrue(TEXT("An add with an absurd radius still succeeds"), Store.AddMark(PositionForIndex(1), 1.0e9f, TooLarge));
	TestEqual(TEXT("⭐ An absurd radius is lowered to MaxMarkRadiusUU"), TooLarge.RadiusUU, MaxRadius, RadiusToleranceUU);

	FSiegeMapMark InRange;
	TestTrue(TEXT("An add with an in-range radius succeeds"), Store.AddMark(PositionForIndex(2), InRangeRadiusUU, InRange));
	// ⭐ THE ASSERTION THAT STOPS A CLAMP FROM BEING A CONSTANT: a "clamp" that returned the
	// minimum for everything would pass both lines above and fail this one.
	TestEqual(TEXT("An in-range radius is stored EXACTLY, not snapped to a bound"), InRange.RadiusUU, InRangeRadiusUU, RadiusToleranceUU);

	// ⛔ A NaN NEVER REACHES A MARK. Note it must not clamp UP: `FMath::Clamp` is
	// `Max(Min(X, Max), Min)` (UnrealMathUtility.h:592-595), and `Min(NaN, Max)` yields Max, so
	// an unguarded implementation would store the MAXIMUM — the largest possible circle — for a
	// NaN input.
	//
	// ⚠️⚠️ THIS FILE CONSTRUCTS A NaN WHERE TWO SHIPPED TEST FILES DELIBERATELY REFUSED TO, SO
	// THE DIFFERENCE IS STATED RATHER THAN LEFT TO LOOK LIKE AN OVERSIGHT:
	//   • `SiegeAssistantSelectionTest.cpp:2986-2992` refused because it would have asserted a
	//     NaN COMPARISON, whose behaviour depends on the module's floating-point model ⇒ a
	//     result about the compiler, not about the code.
	//   • `SiegeLadderClimbTest.cpp:228-232` refused because merely CONSTRUCTING an `FVector`
	//     holding a NaN calls `TVector::DiagnosticCheckNaN()` and raises an engine error ⇒ the
	//     test would fail on its own fixture.
	// ✅ NEITHER APPLIES HERE, AND BOTH REASONS WERE CHECKED AT THE SOURCE: (a) the NaN is built
	// from its BIT PATTERN — no arithmetic for a fast-math build to fold, and no comparison is
	// asserted; (b) both the shipped guard (`FMath::IsFinite`) and the check below
	// (`FMath::IsNaN`) are BIT-MASK tests (GenericPlatformMath.h:573-586), not comparisons; and
	// (c) the NaN goes into a bare `float`, ⛔ never into the `FVector2D` — so no
	// `DiagnosticCheckNaN` is ever reached.
	const uint32 QuietNaNBits = 0x7FC00000U;               // sign 0, exponent all-ones, quiet bit set
	const float  NaNRadius    = BitCast<float>(QuietNaNBits);
	if (!TestTrue(TEXT("Fixture self-check: the constructed value really IS a NaN"), FMath::IsNaN(NaNRadius)))
	{
		return false;
	}

	FSiegeMapMark NotANumber;
	TestTrue(TEXT("An add with a NaN radius still succeeds"), Store.AddMark(PositionForIndex(3), NaNRadius, NotANumber));
	TestFalse(TEXT("⭐ The stored radius is NOT NaN"), FMath::IsNaN(NotANumber.RadiusUU));
	TestEqual(TEXT("⭐ A NaN radius becomes the MINIMUM — ⛔ not the maximum, which is where an unguarded Clamp would put it"),
		NotANumber.RadiusUU, MinRadius, RadiusToleranceUU);

	// ── ON RESIZE (the wheel path) ────────────────────────────────────────────
	TestTrue(TEXT("SetMarkRadius on a live mark succeeds"), Store.SetMarkRadius(InRange.Number, InRangeRadiusUU * 2.f));
	{
		const FSiegeMapMark* const Resized = Store.FindMark(InRange.Number);
		if (TestNotNull(TEXT("The resized mark still resolves"), Resized))
		{
			TestEqual(TEXT("An in-range resize is stored EXACTLY"), Resized->RadiusUU, InRangeRadiusUU * 2.f, RadiusToleranceUU);
		}
	}

	TestTrue(TEXT("SetMarkRadius below the floor still returns true — clamping is ⛔ NOT a refusal"),
		Store.SetMarkRadius(InRange.Number, -50.f));
	{
		const FSiegeMapMark* const Floored = Store.FindMark(InRange.Number);
		if (TestNotNull(TEXT("The mark still resolves after an under-range resize"), Floored))
		{
			TestEqual(TEXT("A below-floor resize is clamped to MinMarkRadiusUU"), Floored->RadiusUU, MinRadius, RadiusToleranceUU);
		}
	}

	TestTrue(TEXT("SetMarkRadius above the ceiling still returns true"), Store.SetMarkRadius(InRange.Number, MaxRadius * 10.f));
	{
		const FSiegeMapMark* const Capped = Store.FindMark(InRange.Number);
		if (TestNotNull(TEXT("The mark still resolves after an over-range resize"), Capped))
		{
			TestEqual(TEXT("An above-ceiling resize is clamped to MaxMarkRadiusUU"), Capped->RadiusUU, MaxRadius, RadiusToleranceUU);
		}
	}

	// ⛔ A RESIZE OF A NUMBER THAT DOES NOT EXIST IS A REFUSAL THAT TOUCHES NOTHING — including
	// the number 0, which an index-based implementation would happily resize.
	const int32 CountBefore = Store.GetMarks().Num();

	// ⛔ GUARDED BEFORE INDEXING: `TArray::operator[]` range-CHECKS, so on a broken store this
	// would CRASH the run rather than fail this test — and a crashed suite reports nothing at
	// all about the other eight.
	if (!TestEqual(TEXT("Precondition: the four marks added above are all live"), CountBefore, 4))
	{
		return false;
	}

	const float FirstRadiusBefore = Store.GetMarks()[0].RadiusUU;

	TestFalse(TEXT("SetMarkRadius on a hole/unknown number returns false"), Store.SetMarkRadius(500, InRangeRadiusUU));
	TestFalse(TEXT("SetMarkRadius(0, ...) returns false — 0 is not a mark, it is not an index"), Store.SetMarkRadius(0, InRangeRadiusUU));

	TestEqual(TEXT("A refused resize changed no count"), Store.GetMarks().Num(), CountBefore);
	TestEqual(TEXT("A refused resize changed no other mark's radius"), Store.GetMarks()[0].RadiusUU, FirstRadiusBefore, RadiusToleranceUU);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. CLEAR — the match-reset hook empties the store AND resets the numbering
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkClearTest,
	"Siegebound.MapMarks.ClearMarksEmptiesAndNumberingRestarts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkClearTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSiegeMapMark Added;
		Store.AddMark(PositionForIndex(Index), InRangeRadiusUU, Added);
	}

	Store.RemoveMark(2);   // leave a hole, so the clear has something non-trivial to reset

	Store.ClearMarks();

	TestEqual(TEXT("ClearMarks empties the store (M-4: marks are cleared on match reset)"), Store.GetMarks().Num(), 0);
	TestNull(TEXT("No number resolves after a clear"), Store.FindMark(1));
	TestNull(TEXT("No number resolves after a clear — the previously live 3 either"), Store.FindMark(3));

	// ⭐ THE ASSERTION A COUNT CHECK WOULD MISS: numbering must RESTART at 1. A store that kept
	// a monotonic "next id" member would empty correctly here and then hand the new match's
	// first circle the number 4 — and Jonathan would be looking at a circle labelled 4 with
	// nothing else on the map.
	FSiegeMapMark AfterClear;
	TestTrue(TEXT("An add succeeds after a clear"), Store.AddMark(PositionForIndex(0), InRangeRadiusUU, AfterClear));
	TestEqual(TEXT("⭐ The first mark of the new match is numbered 1 again"), AfterClear.Number, 1);
	TestEqualSensitive(TEXT("⭐ ...and its symbol is exactly \"circle_1\""),
		FSiegeMapMark::MakeSymbol(AfterClear.Number), FString(TEXT("circle_1")));

	// Clearing an already-empty store is a legal no-op, not a crash or an error.
	Store.ClearMarks();
	Store.ClearMarks();
	TestEqual(TEXT("Clearing twice is idempotent"), Store.GetMarks().Num(), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. REMOVE IS BY NUMBER — never by index, and unknown numbers refuse
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkRemoveRefusalTest,
	"Siegebound.MapMarks.RemoveRefusesUnknownNumbers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkRemoveRefusalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeMapMarkTestFixture;

	FScratchStore Scratch = MakeScratchStore();
	if (!TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		return false;
	}

	USiegeMapMarkSubsystem& Store = *Scratch.Marks;

	// An empty store refuses everything and survives it.
	TestFalse(TEXT("RemoveMark on an EMPTY store returns false"), Store.RemoveMark(1));
	TestEqual(TEXT("...and the store is still empty"), Store.GetMarks().Num(), 0);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSiegeMapMark Added;
		Store.AddMark(PositionForIndex(Index), InRangeRadiusUU, Added);
	}

	// ⭐⭐ THE INDEX-CONFUSION ASSERTION. `RemoveMark(0)` MUST refuse: 0 is not a legal mark
	// number, but it IS a legal array index — an implementation that reached for
	// `Marks.RemoveAt(Number)` would compile, read plausibly, and delete `circle_1` here.
	TestFalse(TEXT("⭐ RemoveMark(0) refuses — 0 is not a mark number, and it must not be read as an INDEX"), Store.RemoveMark(0));
	TestEqual(TEXT("⭐ ...and all three marks survive it"), Store.GetMarks().Num(), 3);
	TestNotNull(TEXT("⭐ ...mark 1 in particular is still there (an index-based remove would have eaten it)"), Store.FindMark(1));

	TestFalse(TEXT("RemoveMark(-1) refuses"), Store.RemoveMark(-1));
	TestFalse(TEXT("RemoveMark(4) refuses — no such number is live"), Store.RemoveMark(4));
	TestFalse(TEXT("RemoveMark(1000) refuses"), Store.RemoveMark(1000));
	TestEqual(TEXT("None of the refusals removed anything"), Store.GetMarks().Num(), 3);

	// A double delete is a refusal, not a second removal.
	TestTrue(TEXT("RemoveMark(2) succeeds once"), Store.RemoveMark(2));
	TestFalse(TEXT("RemoveMark(2) a second time refuses — the hole is not a mark"), Store.RemoveMark(2));
	TestEqual(TEXT("Two marks survive the double delete"), Store.GetMarks().Num(), 2);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. THE LAW, PINNED AT THE TYPE — per-player by CONSTRUCTION, and the cap is Jonathan's
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeMapMarkStoreScopeTest,
	"Siegebound.MapMarks.StoreIsLocalPlayerScoped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeMapMarkStoreScopeTest::RunTest(const FString& Parameters)
{
	// ⭐ ASSERTED AT THE TYPE, so the day someone "simplifies" the base class the suite goes red
	// and NAMES the ruling — the TASK-749 idiom. `M-2` (per-player) and the `MARK-§6` M8
	// declaration (no replication, no RPC, no relevancy tier) are STRUCTURAL PROPERTIES OF THIS
	// BASE CLASS, ⛔ not promises anybody keeps by discipline: a `ULocalPlayerSubsystem` lives
	// inside one client's `ULocalPlayer` and has no path to the wire.
	UClass* const StoreClass = USiegeMapMarkSubsystem::StaticClass();

	if (!TestNotNull(TEXT("USiegeMapMarkSubsystem::StaticClass() resolves"), StoreClass))
	{
		return false;
	}

	TestTrue(TEXT("⭐ USiegeMapMarkSubsystem is a ULocalPlayerSubsystem — ⛔ NOT a GameInstance or World subsystem (M-2)"),
		StoreClass->IsChildOf(ULocalPlayerSubsystem::StaticClass()));

	// `UCLASS(Within = LocalPlayer)` is inherited from the base. A reparent to
	// `UGameInstanceSubsystem` would silently change this to `UGameInstance` — which is exactly
	// the change that would let two local players share one set of circles.
	TestTrue(TEXT("⭐ Its ClassWithin is ULocalPlayer — one store per LOCAL PLAYER, structurally"),
		StoreClass->ClassWithin == ULocalPlayer::StaticClass());

	const USiegeMapMarkSubsystem* const Defaults = GetDefault<USiegeMapMarkSubsystem>();

	if (!TestNotNull(TEXT("The class default object resolves"), Defaults))
	{
		return false;
	}

	// ⛔ JONATHAN'S CAP (`M-5`). This is pinned deliberately: `MARK-§2` measured that each mark
	// costs 10 characters of Zone C against ~6 of headroom, and 9 is the number that fits the
	// ~121 characters the `ZoneBCharReserve` lever buys. ⚠️ A LEGITIMATE RETUNE RE-BASES THIS
	// LINE AND NAMES THE DECISION — that is the point of asserting it, not an obstacle to it.
	TestEqual(TEXT("⛔ MaxMapMarks ships at 9 — Jonathan's ruling M-5, costed in MARK-§2"), Defaults->MaxMapMarks, 9);

	// The radius fence's shipped shape. A swapped retune (min above max) would make every mark
	// the same size, silently.
	TestTrue(TEXT("The shipped radius bounds are ordered (min < max)"), Defaults->MinMarkRadiusUU < Defaults->MaxMarkRadiusUU);
	TestTrue(TEXT("The shipped floor is positive — a zero-radius mark is not a place"), Defaults->MinMarkRadiusUU > 0.f);

	// A fresh instance starts empty — no CDO state leaks into a new player's store.
	SiegeMapMarkTestFixture::FScratchStore Scratch = SiegeMapMarkTestFixture::MakeScratchStore();
	if (TestTrue(TEXT("Fixture: a ULocalPlayer-outered USiegeMapMarkSubsystem was constructed"), Scratch.IsValid()))
	{
		TestEqual(TEXT("A freshly constructed store holds no marks"), Scratch.Marks->GetMarks().Num(), 0);
		TestNull(TEXT("...and no number resolves in it"), Scratch.Marks->FindMark(1));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
