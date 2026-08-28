// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckLibrary.h"
#include "Siegebound/DeckTypes.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for the ten-slot deck model (wave DECK-BUILDER 3-FIX,
 *  TASK-670; CONVENTIONS DECK-§1/§2, defaults D1/D2, signatures DECK-§8).
 *
 *  ⚠️ WRITTEN AGAINST THE DECK-§8 PINNED SIGNATURE REGISTRY — the cross-task
 *  contract, character-for-character. The compile gate (TASK-674) is where the
 *  sides reconcile: a test failing there against a registry-conformant
 *  implementation is MY defect; one failing against a registry deviation is a
 *  FINDING.
 *
 *  WHY THIS IS THE PURE-FUNCTION/UNIT LANE (offline-only, ZERO network): the
 *  slot-name statics and the migration are static functions over an in-memory
 *  USaveGame object — no UWorld, no PIE, no widget, no cloud. The one disk
 *  touch (the round-trip test) writes ONLY the scratch slot below. What is NOT
 *  assertable here is NAMED in handoffs/TASK-670-programmer.md rather than
 *  implied: the widget-side NativeConstruct sequence, the auto-save funnel
 *  firing per mutation, and the ACC-§4 seam resolution close at TASK-673
 *  review (code inspection) + TASK-674 live verify — driving UDeckBuilderWidget
 *  mutators from a test would resolve the deck slot through the REAL seam and
 *  could write the player's actual guest slot, which no test may do, and the
 *  widget has no slot-override seam to add without deviating from DECK-§8.
 *
 *  ⛔ THE TESTS NEVER WRITE THE PLAYER'S REAL SLOTS, MECHANICALLY: the only
 *  SaveGameToSlot target is "SiegeDecks_AutomationScratch" — asserted to
 *  differ from USiegeDeckSaveGame::SlotName, impossible as a profile slot
 *  (profile suffixes are digit-only, ACC-§3), and deleted on the way in AND
 *  out by FDeckScratchGuard.
 *
 *  M8: adds no replicated property, no new replicated class, no new relevancy
 *  tier, no RPC.
 */

namespace SiegeDeckSlotsTestUtils
{
	/** ⛔ NEVER the shipped guest slot; asserted in the round-trip test, not assumed. Non-digit suffix ⇒ never a profile slot either (ACC-§3). */
	static const TCHAR* ScratchDeckSlotName = TEXT("SiegeDecks_AutomationScratch");

	/** Deletes the scratch deck slot on the way in AND out (the SiegeAccountTest janitor idiom). */
	struct FDeckScratchGuard
	{
		FDeckScratchGuard()
		{
			DeleteScratchSlot();
		}

		~FDeckScratchGuard()
		{
			DeleteScratchSlot();
		}

		static void DeleteScratchSlot()
		{
			if (UGameplayStatics::DoesSaveGameExist(ScratchDeckSlotName, USiegeDeckSaveGame::UserIndex))
			{
				UGameplayStatics::DeleteGameInSlot(ScratchDeckSlotName, USiegeDeckSaveGame::UserIndex);
			}
		}
	};

	/** A legacy-shaped named deck with one card entry (enough to prove content survives migration by value). */
	static FDeckList MakeLegacyDeck(const FString& DeckName, const FName CardID, int32 Count)
	{
		FDeckList Deck;
		Deck.DeckName = DeckName;
		FDeckCardEntry Entry;
		Entry.CardID = CardID;
		Entry.Count = Count;
		Deck.Cards.Add(Entry);
		return Deck;
	}

	/** Fresh in-memory save object (transient outer; single RunTest frame ⇒ no GC window). */
	static USiegeDeckSaveGame* MakeSave()
	{
		return NewObject<USiegeDeckSaveGame>();
	}

	/**
	 *  Deep state equality over the class's ENTIRE serialized surface — its only
	 *  UPROPERTYs are SavedDecks and ActiveDeckName, so field equality here IS
	 *  the DECK-§2 "byte-stable" claim. Name compares are CASE-SENSITIVE on
	 *  purpose (a case flip is a real mutation under the triple-duty law).
	 */
	static bool StatesEqual(const TArray<FDeckList>& DecksA, const FString& ActiveA,
		const USiegeDeckSaveGame& SaveB)
	{
		if (!ActiveA.Equals(SaveB.ActiveDeckName, ESearchCase::CaseSensitive) ||
			DecksA.Num() != SaveB.SavedDecks.Num())
		{
			return false;
		}
		for (int32 DeckIndex = 0; DeckIndex < DecksA.Num(); ++DeckIndex)
		{
			const FDeckList& DeckA = DecksA[DeckIndex];
			const FDeckList& DeckB = SaveB.SavedDecks[DeckIndex];
			if (!DeckA.DeckName.Equals(DeckB.DeckName, ESearchCase::CaseSensitive) ||
				DeckA.Cards.Num() != DeckB.Cards.Num())
			{
				return false;
			}
			for (int32 CardIndex = 0; CardIndex < DeckA.Cards.Num(); ++CardIndex)
			{
				if (DeckA.Cards[CardIndex].CardID != DeckB.Cards[CardIndex].CardID ||
					DeckA.Cards[CardIndex].Count != DeckB.Cards[CardIndex].Count)
				{
					return false;
				}
			}
		}
		return true;
	}
}

/**
 *  THE FIXED-NAME CONTRACT (DECK-§1). The ten names are load-bearing three ways
 *  at once (bar label = save key = cloud deck_name), so they are asserted
 *  byte-for-byte, with the index round trip, the case-insensitive inverse, and
 *  the non-fixed rejections the ONE composer must give.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckFixedNameContractTest,
	"Siegebound.Deck.FixedNameContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckFixedNameContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("NumFixedDeckSlots is 10 (Jonathan's directive verbatim)"),
		USiegeDeckSaveGame::NumFixedDeckSlots, 10);

	// all ten names byte-exact + the round trip name -> index
	for (int32 SlotIndex = 0; SlotIndex < USiegeDeckSaveGame::NumFixedDeckSlots; ++SlotIndex)
	{
		const FString Expected = FString::Printf(TEXT("deck%d"), SlotIndex + 1);
		const FString Made = USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex);
		TestEqualSensitive(*FString::Printf(TEXT("MakeFixedDeckName(%d) is exactly \"%s\" (lowercase — triple-duty law)"), SlotIndex, *Expected),
			Made, Expected);
		TestEqual(*FString::Printf(TEXT("FindFixedDeckIndex round-trips \"%s\" back to %d"), *Made, SlotIndex),
			USiegeDeckSaveGame::FindFixedDeckIndex(Made), SlotIndex);
	}

	// case-insensitive inverse (DECK-§8: "case-insensitive")
	TestEqual(TEXT("FindFixedDeckIndex(\"DECK1\") is 0 (case-insensitive)"),
		USiegeDeckSaveGame::FindFixedDeckIndex(TEXT("DECK1")), 0);
	TestEqual(TEXT("FindFixedDeckIndex(\"Deck10\") is 9 (case-insensitive)"),
		USiegeDeckSaveGame::FindFixedDeckIndex(TEXT("Deck10")), 9);

	// non-fixed names -> INDEX_NONE, every near-miss shape
	const TCHAR* NonFixedNames[] = {
		TEXT(""), TEXT("deck"), TEXT("deck0"), TEXT("deck11"), TEXT("deck01"),
		TEXT(" deck1"), TEXT("deck1 "), TEXT("deck1x"), TEXT("MyDeck"), TEXT("1deck")
	};
	for (const TCHAR* NonFixed : NonFixedNames)
	{
		TestEqual(*FString::Printf(TEXT("FindFixedDeckIndex(\"%s\") is INDEX_NONE (not a fixed name)"), NonFixed),
			USiegeDeckSaveGame::FindFixedDeckIndex(NonFixed), static_cast<int32>(INDEX_NONE));
	}

	// out-of-range composer contract: empty string, never a phantom name
	TestTrue(TEXT("MakeFixedDeckName(-1) is empty (out of range)"),
		USiegeDeckSaveGame::MakeFixedDeckName(-1).IsEmpty());
	TestTrue(TEXT("MakeFixedDeckName(10) is empty (out of range — slots are 0-based)"),
		USiegeDeckSaveGame::MakeFixedDeckName(USiegeDeckSaveGame::NumFixedDeckSlots).IsEmpty());

	return true;
}

/**
 *  MIGRATION OF A FRESH (EMPTY) SAVE — the first-boot / fresh-account case.
 *  All ten slots materialize EMPTY (D1) and ActiveDeckName defaults to "deck1"
 *  (DECK-§2 clause 6 — Jonathan's fresh-account default; the orange outline
 *  starts on deck1).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckMigrationFreshSaveTest,
	"Siegebound.Deck.MigrationFreshSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckMigrationFreshSaveTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Fresh save object constructed"), Save))
	{
		return false;
	}

	TestTrue(TEXT("Migrating an empty save mutates it (returns true)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));

	TestEqual(TEXT("All ten slots exist after migration (always-materialized, DECK-§1)"),
		Save->SavedDecks.Num(), USiegeDeckSaveGame::NumFixedDeckSlots);
	for (int32 SlotIndex = 0; SlotIndex < Save->SavedDecks.Num(); ++SlotIndex)
	{
		TestEqualSensitive(*FString::Printf(TEXT("Slot %d carries its canonical fixed name"), SlotIndex),
			Save->SavedDecks[SlotIndex].DeckName, USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex));
		TestEqual(*FString::Printf(TEXT("Slot %d starts EMPTY (D1 — never a copy of deck1)"), SlotIndex),
			Save->SavedDecks[SlotIndex].TotalCount(), 0);
	}

	TestEqualSensitive(TEXT("ActiveDeckName defaults to \"deck1\" (clause 6 fresh-account default)"),
		Save->ActiveDeckName, FString(TEXT("deck1")));

	return true;
}

/**
 *  MIGRATION OF LEGACY NAMED DECKS (DECK-§2 clauses 2/3/6, default D2): the
 *  previously-active deck lands in deck1 (the lowest empty slot), the rest fill
 *  deck2.. in SavedDecks order with their CONTENT intact, and ActiveDeckName is
 *  rewritten to the fixed name. Second save: a DANGLING active name counts as
 *  "no active deck" and defaults to deck1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckMigrationLegacyMappingTest,
	"Siegebound.Deck.MigrationLegacyMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckMigrationLegacyMappingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}

	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("War Deck"), FName(TEXT("Knight")), 5));
	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("Rush"), FName(TEXT("Goblin")), 12));
	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("Econ"), FName(TEXT("Miner")), 3));
	Save->ActiveDeckName = TEXT("rush"); // case-variant on purpose — the M6 idiom matches case-insensitively

	TestTrue(TEXT("Migrating a legacy save mutates it (returns true)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	TestEqual(TEXT("Exactly ten slots exist after migration"),
		Save->SavedDecks.Num(), USiegeDeckSaveGame::NumFixedDeckSlots);
	if (Save->SavedDecks.Num() != USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		return false;
	}

	// clause 2: the ACTIVE legacy deck ("Rush") -> deck1, content intact
	TestEqualSensitive(TEXT("deck1 carries its canonical name"),
		Save->SavedDecks[0].DeckName, FString(TEXT("deck1")));
	TestEqual(TEXT("deck1 holds the previously-ACTIVE deck's card (Goblin)"),
		Save->SavedDecks[0].Cards.Num() > 0 ? Save->SavedDecks[0].Cards[0].CardID : NAME_None,
		FName(TEXT("Goblin")));
	TestEqual(TEXT("deck1 kept the active deck's copy count (12)"),
		Save->SavedDecks[0].TotalCount(), 12);

	// clause 3: the remaining legacy decks fill deck2.. in SavedDecks order
	TestEqual(TEXT("deck2 holds the first remaining legacy deck's card (Knight, from 'War Deck')"),
		Save->SavedDecks[1].Cards.Num() > 0 ? Save->SavedDecks[1].Cards[0].CardID : NAME_None,
		FName(TEXT("Knight")));
	TestEqual(TEXT("deck3 holds the second remaining legacy deck's card (Miner, from 'Econ')"),
		Save->SavedDecks[2].Cards.Num() > 0 ? Save->SavedDecks[2].Cards[0].CardID : NAME_None,
		FName(TEXT("Miner")));

	// clause 5: slots 4..10 materialized empty
	for (int32 SlotIndex = 3; SlotIndex < USiegeDeckSaveGame::NumFixedDeckSlots; ++SlotIndex)
	{
		TestEqual(*FString::Printf(TEXT("Slot %d materialized EMPTY"), SlotIndex),
			Save->SavedDecks[SlotIndex].TotalCount(), 0);
	}

	// clause 6: ActiveDeckName rewritten to the fixed name its deck landed in
	TestEqualSensitive(TEXT("ActiveDeckName rewritten to \"deck1\" (where the active deck landed)"),
		Save->ActiveDeckName, FString(TEXT("deck1")));

	// --- second save: DANGLING active name = "no active deck" (clause 6) ----
	USiegeDeckSaveGame* DanglingSave = MakeSave();
	if (!TestNotNull(TEXT("Dangling-active save object constructed"), DanglingSave))
	{
		return false;
	}
	DanglingSave->SavedDecks.Add(MakeLegacyDeck(TEXT("Alpha"), FName(TEXT("Archer")), 4));
	DanglingSave->ActiveDeckName = TEXT("Ghost"); // matches no deck — a hand-edited/corrupt save shape

	TestTrue(TEXT("Migrating the dangling-active save mutates it"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*DanglingSave));
	TestEqualSensitive(TEXT("A dangling active name falls back to \"deck1\" (clause 6 default)"),
		DanglingSave->ActiveDeckName, FString(TEXT("deck1")));
	TestEqual(TEXT("The one legacy deck still landed (in deck1, by clause 3 fill order)"),
		DanglingSave->SavedDecks.Num() > 0 && DanglingSave->SavedDecks[0].Cards.Num() > 0
			? DanglingSave->SavedDecks[0].Cards[0].CardID : NAME_None,
		FName(TEXT("Archer")));

	return true;
}

/**
 *  A LEGACY DECK ALREADY NAMED "Deck3" KEEPS ITS SLOT (DECK-§2 clause 1,
 *  case-insensitive) — and its name is CANONICALIZED to lowercase "deck3"
 *  (triple-duty law: the match reader and the cloud merge compare
 *  case-sensitively, so the byte form matters). The active non-fixed deck then
 *  takes the lowest EMPTY slot (deck1), and the next legacy deck fills deck2.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckMigrationFixedNameRespectedTest,
	"Siegebound.Deck.MigrationFixedNameRespected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckMigrationFixedNameRespectedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}

	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("Deck3"), FName(TEXT("Catapult")), 2)); // fixed name, case variant
	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("Alpha"), FName(TEXT("Knight")), 6));
	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("Beta"), FName(TEXT("Goblin")), 8));
	Save->ActiveDeckName = TEXT("Alpha");

	TestTrue(TEXT("Migration mutates (returns true)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	TestEqual(TEXT("Exactly ten slots exist"),
		Save->SavedDecks.Num(), USiegeDeckSaveGame::NumFixedDeckSlots);
	if (Save->SavedDecks.Num() != USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		return false;
	}

	// clause 1: "Deck3" kept slot index 2 and was canonicalized byte-exact
	TestEqualSensitive(TEXT("Slot 2 is named exactly \"deck3\" (canonicalized lowercase)"),
		Save->SavedDecks[2].DeckName, FString(TEXT("deck3")));
	TestEqual(TEXT("\"Deck3\" kept its slot: slot 2 holds its card (Catapult)"),
		Save->SavedDecks[2].Cards.Num() > 0 ? Save->SavedDecks[2].Cards[0].CardID : NAME_None,
		FName(TEXT("Catapult")));

	// clause 2: active "Alpha" -> lowest empty slot = deck1
	TestEqual(TEXT("The active deck (\"Alpha\") landed in deck1 (Knight)"),
		Save->SavedDecks[0].Cards.Num() > 0 ? Save->SavedDecks[0].Cards[0].CardID : NAME_None,
		FName(TEXT("Knight")));
	TestEqualSensitive(TEXT("ActiveDeckName rewritten to \"deck1\""),
		Save->ActiveDeckName, FString(TEXT("deck1")));

	// clause 3: "Beta" -> the next empty slot = deck2
	TestEqual(TEXT("\"Beta\" filled deck2 (Goblin)"),
		Save->SavedDecks[1].Cards.Num() > 0 ? Save->SavedDecks[1].Cards[0].CardID : NAME_None,
		FName(TEXT("Goblin")));

	return true;
}

/**
 *  OVERFLOW (DECK-§2 clause 4, default D2): twelve legacy decks into ten slots
 *  — the active deck and the first nine others land, the last two are DROPPED
 *  with exactly ONE Warning listing them, and exactly ten slots remain.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckMigrationOverflowTest,
	"Siegebound.Deck.MigrationOverflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckMigrationOverflowTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	// REQUIRED, exactly once: the clause-4 drop log. Anything else (silent
	// drop, or a warning per deck) is a failure.
	AddExpectedMessagePlain(TEXT("MigrateToFixedSlots: dropped"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 1);

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}

	for (int32 LegacyIndex = 1; LegacyIndex <= 12; ++LegacyIndex)
	{
		Save->SavedDecks.Add(MakeLegacyDeck(
			FString::Printf(TEXT("Legacy%02d"), LegacyIndex), FName(TEXT("Knight")), LegacyIndex));
	}
	Save->ActiveDeckName = TEXT("Legacy05");

	TestTrue(TEXT("Migration mutates (returns true)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	TestEqual(TEXT("Exactly ten slots survive (overflow dropped, never eleven decks)"),
		Save->SavedDecks.Num(), USiegeDeckSaveGame::NumFixedDeckSlots);
	if (Save->SavedDecks.Num() != USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		return false;
	}

	// the active deck ("Legacy05", Count 5) took deck1 first (clause 2)...
	TestEqual(TEXT("deck1 holds the active legacy deck (its Count marker is 5)"),
		Save->SavedDecks[0].TotalCount(), 5);
	TestEqualSensitive(TEXT("ActiveDeckName is \"deck1\""),
		Save->ActiveDeckName, FString(TEXT("deck1")));

	// ...then Legacy01..04 and 06..10 filled deck2..deck10 in order (clause 3);
	// their Count markers identify them: 1,2,3,4,6,7,8,9,10.
	const int32 ExpectedCounts[] = { 5, 1, 2, 3, 4, 6, 7, 8, 9, 10 };
	for (int32 SlotIndex = 0; SlotIndex < USiegeDeckSaveGame::NumFixedDeckSlots; ++SlotIndex)
	{
		TestEqual(*FString::Printf(TEXT("Slot %d holds the expected legacy deck (Count marker %d)"),
				SlotIndex, ExpectedCounts[SlotIndex]),
			Save->SavedDecks[SlotIndex].TotalCount(), ExpectedCounts[SlotIndex]);
	}

	// Legacy11/Legacy12 (Count markers 11/12) are gone — dropped, not renamed
	for (const FDeckList& Deck : Save->SavedDecks)
	{
		TestTrue(TEXT("No surviving slot carries an overflow deck (Count marker > 10)"),
			Deck.TotalCount() <= 10);
	}

	return true;
}

/**
 *  IDEMPOTENCE (DECK-§2): the second call returns false and the state is
 *  byte-stable — deep-compared over the class's entire serialized surface
 *  (SavedDecks + ActiveDeckName are its only UPROPERTYs, so field equality IS
 *  byte stability). A hand-built already-canonical save returns false on the
 *  FIRST call too.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckMigrationIdempotenceTest,
	"Siegebound.Deck.MigrationIdempotence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckMigrationIdempotenceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	// --- legacy save: migrate once (true), snapshot, migrate again (false) ---
	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}
	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("War Deck"), FName(TEXT("Knight")), 5));
	// an already-fixed name in the mix — exercises clause 1 inside the idempotence pair
	Save->SavedDecks.Add(MakeLegacyDeck(TEXT("deck4"), FName(TEXT("Goblin")), 7));
	Save->ActiveDeckName = TEXT("War Deck");

	TestTrue(TEXT("First migration of a legacy save mutates (returns true)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));

	const TArray<FDeckList> SnapshotDecks = Save->SavedDecks;   // deep copy (FDeckList is a value type)
	const FString SnapshotActive = Save->ActiveDeckName;

	TestFalse(TEXT("Second migration returns false (fixed form recognized)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	TestTrue(TEXT("Second migration is byte-stable (deep state equality over SavedDecks + ActiveDeckName)"),
		StatesEqual(SnapshotDecks, SnapshotActive, *Save));

	// --- a hand-built canonical save: false on the FIRST call, untouched -----
	USiegeDeckSaveGame* CanonicalSave = MakeSave();
	if (!TestNotNull(TEXT("Canonical save object constructed"), CanonicalSave))
	{
		return false;
	}
	for (int32 SlotIndex = 0; SlotIndex < USiegeDeckSaveGame::NumFixedDeckSlots; ++SlotIndex)
	{
		FDeckList Deck;
		Deck.DeckName = USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex);
		CanonicalSave->SavedDecks.Add(Deck);
	}
	CanonicalSave->ActiveDeckName = TEXT("deck7");

	const TArray<FDeckList> CanonicalDecks = CanonicalSave->SavedDecks;
	const FString CanonicalActive = CanonicalSave->ActiveDeckName;

	TestFalse(TEXT("A hand-built canonical save is a no-op on the FIRST call"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*CanonicalSave));
	TestTrue(TEXT("...and is byte-stable"),
		StatesEqual(CanonicalDecks, CanonicalActive, *CanonicalSave));
	TestEqualSensitive(TEXT("...its active deck choice (\"deck7\") survives untouched"),
		CanonicalSave->ActiveDeckName, FString(TEXT("deck7")));

	return true;
}

/**
 *  THE D5 FALLBACK MECHANISM, DOCUMENTED (DECK-§3 / default D5): an EMPTY fixed
 *  slot has TotalCount 0 and can NEVER pass UDeckLibrary::IsDeckLegal — with or
 *  without a card table — which is exactly why the match reader
 *  (SiegePlayerController.cpp:241-290, untouched this wave) falls back to the
 *  curated 50-card default instead of blocking match entry. No new code
 *  enforces D5; this test pins the existing mechanism that provides it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckEmptySlotIllegalTest,
	"Siegebound.Deck.EmptySlotIllegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckEmptySlotIllegalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}
	TestTrue(TEXT("Fresh save migrates"), USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	if (Save->SavedDecks.Num() < 2)
	{
		return false;
	}

	const FDeckList& EmptySlot = Save->SavedDecks[1]; // deck2 — any empty slot proves it
	TestEqual(TEXT("An empty fixed slot has TotalCount 0"), EmptySlot.TotalCount(), 0);

	// null table: illegal with a reason (the DeckLibrary null-safe branch)
	FString Reason;
	TestFalse(TEXT("An empty slot is illegal with NO card table"),
		UDeckLibrary::IsDeckLegal(nullptr, EmptySlot, Reason));
	TestFalse(TEXT("...and the refusal carries a reason"), Reason.IsEmpty());

	// a real (empty) table with the FCardRow struct: STILL illegal — 0 cards
	// can never equal the exactly-50 rule, whatever the table holds. This is
	// the actual clause the match reader's fallback rides (D5).
	UDataTable* EmptyTable = NewObject<UDataTable>();
	if (TestNotNull(TEXT("Scratch card table constructed"), EmptyTable))
	{
		EmptyTable->RowStruct = FCardRow::StaticStruct();
		FString TableReason;
		TestFalse(TEXT("An empty slot is illegal against a real table too (0 != 50)"),
			UDeckLibrary::IsDeckLegal(EmptyTable, EmptySlot, TableReason));
		TestFalse(TEXT("...with a reason (the exactly-50 rule)"), TableReason.IsEmpty());
	}

	return true;
}

/**
 *  PERSISTENCE ROUND TRIP ON A SCRATCH SLOT (the SiegeAccountTest scratch-slot
 *  idiom): a migrated ten-slot save with content in deck4 and ActiveDeckName
 *  "deck4" survives SaveGameToSlot -> LoadGameFromSlot byte-meaningfully —
 *  names, cards, counts, and the active-deck choice all round-trip through
 *  tagged-property serialization. ⛔ Writes ONLY "SiegeDecks_AutomationScratch"
 *  (asserted different from the shipped guest slot; non-digit suffix can never
 *  be a profile slot), deleted on the way in and out.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckScratchSlotRoundTripTest,
	"Siegebound.Deck.ScratchSlotRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckScratchSlotRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	// the hermeticity guarantee, mechanical: the scratch name is a different
	// string from the shipped guest slot, so no assert below can touch it
	TestNotEqual(TEXT("The scratch slot is NOT the shipped guest slot"),
		FString(ScratchDeckSlotName), FString(USiegeDeckSaveGame::SlotName));

	FDeckScratchGuard ScratchGuard;

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}
	TestTrue(TEXT("Fresh save migrates to the ten slots"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	if (Save->SavedDecks.Num() != USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		return false;
	}

	// content into deck4 + make it the active deck (the post-migration writer
	// shapes: content mutation + activation)
	FDeckCardEntry Entry;
	Entry.CardID = FName(TEXT("TestCard"));
	Entry.Count = 7;
	Save->SavedDecks[3].Cards.Add(Entry);
	Save->ActiveDeckName = USiegeDeckSaveGame::MakeFixedDeckName(3);

	TestTrue(TEXT("SaveGameToSlot(scratch) succeeds"),
		UGameplayStatics::SaveGameToSlot(Save, ScratchDeckSlotName, USiegeDeckSaveGame::UserIndex));

	const USiegeDeckSaveGame* Loaded = Cast<USiegeDeckSaveGame>(
		UGameplayStatics::LoadGameFromSlot(ScratchDeckSlotName, USiegeDeckSaveGame::UserIndex));
	if (!TestNotNull(TEXT("LoadGameFromSlot(scratch) returns a USiegeDeckSaveGame"), Loaded))
	{
		return false;
	}

	TestEqual(TEXT("All ten slots round-tripped"),
		Loaded->SavedDecks.Num(), USiegeDeckSaveGame::NumFixedDeckSlots);
	if (Loaded->SavedDecks.Num() == USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		for (int32 SlotIndex = 0; SlotIndex < USiegeDeckSaveGame::NumFixedDeckSlots; ++SlotIndex)
		{
			TestEqualSensitive(*FString::Printf(TEXT("Slot %d name round-tripped byte-exact"), SlotIndex),
				Loaded->SavedDecks[SlotIndex].DeckName, USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex));
		}
		TestEqual(TEXT("deck4's card round-tripped (CardID)"),
			Loaded->SavedDecks[3].Cards.Num() > 0 ? Loaded->SavedDecks[3].Cards[0].CardID : NAME_None,
			FName(TEXT("TestCard")));
		TestEqual(TEXT("deck4's copy count round-tripped (7)"),
			Loaded->SavedDecks[3].TotalCount(), 7);
	}
	TestEqualSensitive(TEXT("The active-deck choice (\"deck4\") round-tripped byte-exact"),
		Loaded->ActiveDeckName, FString(TEXT("deck4")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
