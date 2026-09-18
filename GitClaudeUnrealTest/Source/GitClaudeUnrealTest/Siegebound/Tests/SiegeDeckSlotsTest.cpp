// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/CardHandWidget.h" // TASK-1270 loop 1: the REAL late listener (InitForController binds, then spends the held notice)
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckBuilderWidget.h" // TASK-1270: UDeckBuilderWidget::TryActivateSavedDeck — the static activation gate
#include "Siegebound/DeckComponent.h" // TASK-1270 loop 1: derives whether the world-free hand logs its no-deck Warning
#include "Siegebound/DeckLibrary.h"
#include "Siegebound/DeckTypes.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SiegePlayerController.h" // TASK-1270 loop 1: the match-start notice mailbox
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

// ⛔ TASK-1270 — THE ONE COUPLING TO THE MATCH-START NOTICE COMPOSER. Declared,
// never defined here: the definition is a free function with EXTERNAL LINKAGE
// in SiegePlayerController.cpp (the SiegeboundCardGlossary::AppendSpellLines
// precedent — the controller header is not on the row's write list). If the
// signature moves, or the definition is "tidied" into an anonymous namespace,
// this file fails to LINK — the intended failure mode, repaired at the
// definition, ⛔ never by deleting this declaration.
namespace SiegeboundDeckNotice
{
	FText MakeIllegalActiveDeckNoticeText(const FString& DeckName, int32 CardCount);
}

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
 *
 *  CARD-UNCAP 2026-08-28 (TASK-676, CONVENTIONS UNCAP-§7 tests law): the four
 *  Siegebound.Deck.Uncap* cases below pin the amended IsDeckLegal — per-card
 *  copy caps abolished, the exactly-50 total EXACT (UNCAP-§1, never <=50),
 *  unknown-CardID and negative-Count refusals surviving. They run on a
 *  TRANSIENT in-test UDataTable (NewObject + FCardRow rows — ⛔ never the
 *  shipped DT_Cards asset; commandlet-safe, zero disk, ZERO network).
 *
 *  TASK-1270 (DECK-ILLEGAL-ACTIVE, the TASK-1230 R-DECK finding): the two
 *  Siegebound.Deck.IllegalDeck* cases pin (a) the activation gate — an illegal
 *  saved deck can NOT become the active deck (ActiveDeckName unchanged, hence
 *  the rim index GetActiveDeckIndex derives from it unchanged), a legal one
 *  can — asserted on STATE (SC-§104) through the static in-memory half
 *  UDeckBuilderWidget::TryActivateSavedDeck (the widget's SetActiveDeck calls
 *  exactly this, then persists on success only); and (b) the match-start HUD
 *  notice's exact string. Same lane as everything above: in-memory save,
 *  transient table, no widget instance, no world, no slot write — which is
 *  precisely why the gate was factored to a static (driving the widget's
 *  mutators would resolve the REAL seam, see the paragraph above). What this
 *  lane can NOT assert is named in handoffs/TASK-1270-programmer.md: the
 *  BeginPlay arm firing the broadcast once (PIE — the verifier's log line).
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
	 *  Transient in-test card table (UNCAP-§7 tests law: NewObject + FCardRow
	 *  rows — ⛔ never the shipped DT_Cards; commandlet-safe). Transient outer;
	 *  single RunTest frame ⇒ no GC window (the MakeSave idiom).
	 */
	static UDataTable* MakeScratchCardTable()
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FCardRow::StaticStruct();
		return Table;
	}

	/**
	 *  Add one FCardRow to the scratch table. MaxCopies is set DELIBERATELY (the
	 *  old-cap value) so the uncap tests prove legality IGNORES it (UNCAP-§2:
	 *  the column survives as the hero-upgrade stack cap only).
	 */
	static void AddScratchCard(UDataTable& Table, const TCHAR* CardID, int32 MaxCopies)
	{
		FCardRow Row;
		Row.MaxCopies = MaxCopies;
		Table.AddRow(FName(CardID), Row);
	}

	/** Append one (CardID, Count) entry to a deck under test. */
	static void AddDeckEntry(FDeckList& Deck, const TCHAR* CardID, int32 Count)
	{
		FDeckCardEntry Entry;
		Entry.CardID = FName(CardID);
		Entry.Count = Count;
		Deck.Cards.Add(Entry);
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

/**
 *  CARD-UNCAP CASE 1 (UNCAP-§1/§3): 50 COPIES OF ONE CARD IS LEGAL. The scratch
 *  row carries the OLD cap (MaxCopies = 12, the historical Footman value) to
 *  prove legality now IGNORES the column entirely — including the old AGGREGATE
 *  path: the same 50 split across two entries of the same CardID is legal too
 *  (the RunningCounts map is gone, not just relaxed).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckUncapFiftyOfOneCardLegalTest,
	"Siegebound.Deck.UncapFiftyOfOneCardLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckUncapFiftyOfOneCardLegalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	UDataTable* Table = MakeScratchCardTable();
	if (!TestNotNull(TEXT("Scratch card table constructed"), Table))
	{
		return false;
	}
	AddScratchCard(*Table, TEXT("Footman"), /*MaxCopies (the OLD cap — must be ignored)*/ 12);

	// one entry, Count 50 — 50-of-one-card, far past the row's MaxCopies
	FDeckList Deck;
	AddDeckEntry(Deck, TEXT("Footman"), 50);
	TestEqual(TEXT("The deck totals exactly SiegeLegalDeckSize (50)"),
		Deck.TotalCount(), SiegeLegalDeckSize);

	FString Reason;
	TestTrue(TEXT("50 copies of ONE card is LEGAL (per-card caps abolished, UNCAP-§1)"),
		UDeckLibrary::IsDeckLegal(Table, Deck, Reason));
	TestTrue(TEXT("...and the reason string is cleared on success"), Reason.IsEmpty());

	// the old AGGREGATE check is gone too: the same card split across duplicate
	// entries (25 + 25) no longer trips a running-count cap
	FDeckList SplitDeck;
	AddDeckEntry(SplitDeck, TEXT("Footman"), 25);
	AddDeckEntry(SplitDeck, TEXT("Footman"), 25);

	FString SplitReason;
	TestTrue(TEXT("The same 50 split across duplicate entries of one card is LEGAL (RunningCounts removed)"),
		UDeckLibrary::IsDeckLegal(Table, SplitDeck, SplitReason));
	TestTrue(TEXT("...with a cleared reason"), SplitReason.IsEmpty());

	return true;
}

/**
 *  CARD-UNCAP CASE 2 (UNCAP-§1, the U5 pin): the deck total is EXACTLY 50,
 *  never <=50 — 51 refuses AND 49 refuses (both directions of the pin), each
 *  with the exact-50 reason.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckUncapExactFiftyPinnedTest,
	"Siegebound.Deck.UncapExactFiftyPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckUncapExactFiftyPinnedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	UDataTable* Table = MakeScratchCardTable();
	if (!TestNotNull(TEXT("Scratch card table constructed"), Table))
	{
		return false;
	}
	AddScratchCard(*Table, TEXT("Footman"), 12);

	// 51 total — one over — illegal with the exact-50 reason
	FDeckList OverDeck;
	AddDeckEntry(OverDeck, TEXT("Footman"), 51);

	FString OverReason;
	TestFalse(TEXT("A 51-card deck is ILLEGAL (the exactly-50 rule survives the uncap)"),
		UDeckLibrary::IsDeckLegal(Table, OverDeck, OverReason));
	TestTrue(TEXT("...for the exact-50 reason (the reason names 'exactly 50')"),
		OverReason.Contains(TEXT("exactly 50")));

	// 49 total — one under — ILLEGAL TOO: the pin is EXACT, not <=50 (U5)
	FDeckList UnderDeck;
	AddDeckEntry(UnderDeck, TEXT("Footman"), 49);

	FString UnderReason;
	TestFalse(TEXT("A 49-card deck is ILLEGAL (EXACTLY 50, never <=50 — the U5 pin)"),
		UDeckLibrary::IsDeckLegal(Table, UnderDeck, UnderReason));
	TestTrue(TEXT("...for the same exact-50 reason"),
		UnderReason.Contains(TEXT("exactly 50")));

	return true;
}

/**
 *  CARD-UNCAP CASE 3 (UNCAP-§3 clause a): an UNKNOWN CardID still refuses — the
 *  row lookup survived the uncap as the unknown-card gate. The deck would total
 *  50 without the violation, proving the refusal is the unknown-CardID clause,
 *  not the total.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckUncapUnknownCardStillIllegalTest,
	"Siegebound.Deck.UncapUnknownCardStillIllegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckUncapUnknownCardStillIllegalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	UDataTable* Table = MakeScratchCardTable();
	if (!TestNotNull(TEXT("Scratch card table constructed"), Table))
	{
		return false;
	}
	AddScratchCard(*Table, TEXT("Footman"), 12);

	// 50 resolvable copies + one unresolvable entry (Count 0, so the TOTAL is
	// still exactly 50 — only the unknown-CardID clause can refuse this deck)
	FDeckList Deck;
	AddDeckEntry(Deck, TEXT("Footman"), 50);
	AddDeckEntry(Deck, TEXT("NoSuchCard"), 0);
	TestEqual(TEXT("The deck totals exactly 50 (isolating the unknown-CardID clause)"),
		Deck.TotalCount(), SiegeLegalDeckSize);

	FString Reason;
	TestFalse(TEXT("A deck holding an unknown CardID is STILL ILLEGAL after the uncap"),
		UDeckLibrary::IsDeckLegal(Table, Deck, Reason));
	TestTrue(TEXT("...for the unknown-card reason (the reason names 'Unknown card')"),
		Reason.Contains(TEXT("Unknown card")));

	return true;
}

/**
 *  CARD-UNCAP CASE 4 (UNCAP-§3 clause b): a NEGATIVE Count still refuses. The
 *  entries sum to exactly 50 (55 - 5) so the total check cannot be what refuses
 *  it — a negative entry can never launder a deck to legality.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckUncapNegativeCountStillIllegalTest,
	"Siegebound.Deck.UncapNegativeCountStillIllegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckUncapNegativeCountStillIllegalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	UDataTable* Table = MakeScratchCardTable();
	if (!TestNotNull(TEXT("Scratch card table constructed"), Table))
	{
		return false;
	}
	AddScratchCard(*Table, TEXT("Footman"), 12);
	AddScratchCard(*Table, TEXT("Archer"), 10);

	// 55 + (-5) = exactly 50 — only the negative-Count clause can refuse this
	FDeckList Deck;
	AddDeckEntry(Deck, TEXT("Footman"), 55);
	AddDeckEntry(Deck, TEXT("Archer"), -5);
	TestEqual(TEXT("The deck SUMS to exactly 50 (isolating the negative-Count clause)"),
		Deck.TotalCount(), SiegeLegalDeckSize);

	FString Reason;
	TestFalse(TEXT("A deck holding a negative Count is STILL ILLEGAL after the uncap"),
		UDeckLibrary::IsDeckLegal(Table, Deck, Reason));
	TestTrue(TEXT("...for the negative-count reason (the reason names 'negative copy count')"),
		Reason.Contains(TEXT("negative copy count")));

	return true;
}

namespace SiegeDeckSlotsTestUtils
{
	/**
	 *  TASK-1270: Jonathan's REAL deck1 shape as measured by the TASK-1230
	 *  pilot (R-DECK: 8+28+3+3+8+6+6+6 = 68 across eight cards). Card names
	 *  are scratch rows in the transient table, never the shipped DT_Cards.
	 */
	static void AddSixtyEightCardShape(FDeckList& Deck)
	{
		AddDeckEntry(Deck, TEXT("Footman"),    8);
		AddDeckEntry(Deck, TEXT("Archer"),     28);
		AddDeckEntry(Deck, TEXT("Cleric"),     3);
		AddDeckEntry(Deck, TEXT("Catapult"),   3);
		AddDeckEntry(Deck, TEXT("Knight"),     8);
		AddDeckEntry(Deck, TEXT("Miner"),      6);
		AddDeckEntry(Deck, TEXT("Fog"),        6);
		AddDeckEntry(Deck, TEXT("WatchTower"), 6);
	}

	/** The eight scratch rows the 68-card shape resolves against (MaxCopies irrelevant post-uncap; set to the old cap so legality is proven to ignore it). */
	static void AddSixtyEightShapeCards(UDataTable& Table)
	{
		AddScratchCard(Table, TEXT("Footman"),    12);
		AddScratchCard(Table, TEXT("Archer"),     10);
		AddScratchCard(Table, TEXT("Cleric"),     4);
		AddScratchCard(Table, TEXT("Catapult"),   3);
		AddScratchCard(Table, TEXT("Knight"),     8);
		AddScratchCard(Table, TEXT("Miner"),      6);
		AddScratchCard(Table, TEXT("Fog"),        6);
		AddScratchCard(Table, TEXT("WatchTower"), 6);
	}
}

/**
 *  TASK-1270 CASE 1 (DECK-§3 rider; the TASK-1230 R-DECK finding): AN ILLEGAL
 *  SAVED DECK CANNOT BECOME THE ACTIVE DECK; A LEGAL ONE CAN. Driven through
 *  UDeckBuilderWidget::TryActivateSavedDeck — the exact in-memory step
 *  SetActiveDeck (and so the right-click lane SetActiveDeckBySlot) takes
 *  before it persists — on a migrated in-memory save with a transient table.
 *
 *  ⭐ STATE, NOT TALLIES (SC-§104): every refusal is asserted as
 *  "ActiveDeckName is byte-identical to before" + "the rim index
 *  (FindFixedDeckIndex of it — what GetActiveDeckIndex/RefreshDeckBarStates
 *  derive the orange outline from) is unchanged" + "the whole save is deep-
 *  equal to its pre-image" (nothing else moved either). Probes: a 51-card deck
 *  (one over), Jonathan's real 68-card shape, an EMPTY slot, an unknown name,
 *  a null table; then the 50-card deck DOES activate (case-insensitively, to
 *  its canonical stored name), and a refusal AFTER a success leaves the NEW
 *  state alone.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckIllegalDeckCannotBecomeActiveTest,
	"Siegebound.Deck.IllegalDeckCannotBecomeActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckIllegalDeckCannotBecomeActiveTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	UDataTable* Table = MakeScratchCardTable();
	if (!TestNotNull(TEXT("Scratch card table constructed"), Table))
	{
		return false;
	}
	AddSixtyEightShapeCards(*Table);

	USiegeDeckSaveGame* Save = MakeSave();
	if (!TestNotNull(TEXT("Save object constructed"), Save))
	{
		return false;
	}
	TestTrue(TEXT("Fresh save migrates to the ten slots (active = deck1, DECK-§2 clause 6)"),
		USiegeDeckSaveGame::MigrateToFixedSlots(*Save));
	if (Save->SavedDecks.Num() != USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		return false;
	}

	// deck2 = 51 Footman (one over), deck3 = 50 Footman (legal), deck4 = the
	// real 68-card shape, deck5 stays EMPTY (0 cards — the DECK-§1 empty slot)
	AddDeckEntry(Save->SavedDecks[1], TEXT("Footman"), 51);
	AddDeckEntry(Save->SavedDecks[2], TEXT("Footman"), 50);
	AddSixtyEightCardShape(Save->SavedDecks[3]);
	TestEqual(TEXT("deck4 is the measured 68-card shape"), Save->SavedDecks[3].TotalCount(), 68);

	// the pre-image: deck1 active, rim index 0
	TestEqualSensitive(TEXT("Pre-image: the active deck is \"deck1\""), Save->ActiveDeckName, FString(TEXT("deck1")));
	TestEqual(TEXT("Pre-image: the rim index is 0"), USiegeDeckSaveGame::FindFixedDeckIndex(Save->ActiveDeckName), 0);
	const TArray<FDeckList> DecksBefore = Save->SavedDecks;
	const FString ActiveBefore = Save->ActiveDeckName;

	// ---- refusal 1: the 51-card deck2 -------------------------------------
	{
		FString Canonical, Reason;
		TestFalse(TEXT("REFUSED: a 51-card deck cannot become active"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, Table, TEXT("deck2"), Canonical, Reason));
		TestTrue(TEXT("...with IsDeckLegal's exact-50 reason verbatim (names 'exactly 50')"), Reason.Contains(TEXT("exactly 50")));
		TestTrue(TEXT("...and the reason names the 51 (it is the count clause, not another)"), Reason.Contains(TEXT("51 cards")));
		TestTrue(TEXT("...and no canonical name is reported"), Canonical.IsEmpty());
		TestEqualSensitive(TEXT("STATE: ActiveDeckName is still \"deck1\""), Save->ActiveDeckName, FString(TEXT("deck1")));
		TestEqual(TEXT("STATE: the rim index is still 0"), USiegeDeckSaveGame::FindFixedDeckIndex(Save->ActiveDeckName), 0);
		TestTrue(TEXT("STATE: the whole save is deep-equal to its pre-image (nothing moved)"), StatesEqual(DecksBefore, ActiveBefore, *Save));
	}

	// ---- refusal 2: Jonathan's 68-card deck4 (the shipped defect's exact input)
	{
		FString Canonical, Reason;
		TestFalse(TEXT("REFUSED: the 68-card deck cannot become active"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, Table, TEXT("deck4"), Canonical, Reason));
		TestTrue(TEXT("...with the reason naming '68 cards'"), Reason.Contains(TEXT("68 cards")));
		TestEqualSensitive(TEXT("STATE: ActiveDeckName is still \"deck1\" after the 68-card refusal"), Save->ActiveDeckName, FString(TEXT("deck1")));
		TestEqual(TEXT("STATE: the rim index is still 0 after the 68-card refusal"), USiegeDeckSaveGame::FindFixedDeckIndex(Save->ActiveDeckName), 0);
		TestTrue(TEXT("STATE: deep-equal to the pre-image after the 68-card refusal"), StatesEqual(DecksBefore, ActiveBefore, *Save));
	}

	// ---- refusal 3: the EMPTY deck5 (0 cards — illegal, EmptySlotIllegal precedent)
	{
		FString Canonical, Reason;
		TestFalse(TEXT("REFUSED: an empty slot cannot become active"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, Table, TEXT("deck5"), Canonical, Reason));
		TestTrue(TEXT("...with the exact-50 reason"), Reason.Contains(TEXT("exactly 50")));
		TestTrue(TEXT("STATE: deep-equal to the pre-image after the empty-slot refusal"), StatesEqual(DecksBefore, ActiveBefore, *Save));
	}

	// ---- refusal 4: a name that is not a saved deck (the shipped strict check survives)
	{
		FString Canonical, Reason;
		TestFalse(TEXT("REFUSED: an unknown deck name cannot become active"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, Table, TEXT("deck11"), Canonical, Reason));
		TestTrue(TEXT("...with the no-such-deck reason (names 'no saved deck')"), Reason.Contains(TEXT("no saved deck")));
		TestTrue(TEXT("STATE: deep-equal to the pre-image after the unknown-name refusal"), StatesEqual(DecksBefore, ActiveBefore, *Save));
	}

	// ---- refusal 5: no card table — IsDeckLegal's own null contract (illegal, with a reason)
	{
		FString Canonical, Reason;
		TestFalse(TEXT("REFUSED: with no card table even the 50-card deck is not activated"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, /*CardTable=*/ nullptr, TEXT("deck3"), Canonical, Reason));
		TestFalse(TEXT("...and a reason is given (never a silent false)"), Reason.IsEmpty());
		TestTrue(TEXT("STATE: deep-equal to the pre-image after the null-table refusal"), StatesEqual(DecksBefore, ActiveBefore, *Save));
	}

	// ---- SUCCESS: the 50-card deck3, addressed case-insensitively ------------
	{
		FString Canonical, Reason;
		TestTrue(TEXT("ACTIVATED: a 50-card deck CAN become active (addressed as \"DECK3\")"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, Table, TEXT("DECK3"), Canonical, Reason));
		TestTrue(TEXT("...with the reason cleared"), Reason.IsEmpty());
		TestEqualSensitive(TEXT("...reporting the CANONICAL stored name \"deck3\""), Canonical, FString(TEXT("deck3")));
		TestEqualSensitive(TEXT("STATE: ActiveDeckName is now \"deck3\" (canonical, not the caller's spelling)"), Save->ActiveDeckName, FString(TEXT("deck3")));
		TestEqual(TEXT("STATE: the rim index moved to 2"), USiegeDeckSaveGame::FindFixedDeckIndex(Save->ActiveDeckName), 2);
		TestFalse(TEXT("STATE: the save is no longer equal to the pre-image (exactly the activation changed it)"), StatesEqual(DecksBefore, ActiveBefore, *Save));
		TestTrue(TEXT("STATE: ...and ONLY ActiveDeckName changed (the decks themselves are untouched)"), StatesEqual(DecksBefore, TEXT("deck3"), *Save));
	}

	// ---- a refusal AFTER a success leaves the NEW state alone ---------------
	{
		FString Canonical, Reason;
		TestFalse(TEXT("REFUSED again: the 51-card deck2 after deck3 became active"),
			UDeckBuilderWidget::TryActivateSavedDeck(*Save, Table, TEXT("deck2"), Canonical, Reason));
		TestEqualSensitive(TEXT("STATE: ActiveDeckName stays \"deck3\" (the refusal did not revert or move it)"), Save->ActiveDeckName, FString(TEXT("deck3")));
		TestEqual(TEXT("STATE: the rim index stays 2"), USiegeDeckSaveGame::FindFixedDeckIndex(Save->ActiveDeckName), 2);
	}

	return true;
}

/**
 *  TASK-1270 CASE 2: THE MATCH-START HUD NOTICE. When the active saved deck is
 *  illegal, ASiegePlayerController::BeginPlay's fallback arm broadcasts
 *  SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(name, count) through
 *  the shipped BroadcastRefusal lane (loop 1: HELD on the controller until the
 *  hand binds — the delivery is CASE 3 below, this case is the text). The
 *  arm needs a world + the player's slot, so the unit lane pins the two things
 *  it can: (a) the arm's CONDITION is taken on the measured input (the 68-card
 *  shape is illegal by IsDeckLegal, with the count reason), and (b) the exact
 *  player-facing STRING, character-for-character, for two different (name,
 *  count) pairs — so a format failure ("{0}"/"{1}" leaking), a baked name or
 *  a baked count all fail here. The once-per-match firing is the PIE
 *  verifier's log line (handoffs/TASK-1270-programmer.md names it).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckIllegalActiveDeckMatchStartNoticeTest,
	"Siegebound.Deck.IllegalActiveDeckMatchStartNotice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckIllegalActiveDeckMatchStartNoticeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	// (a) the arm's condition on the measured input: IsDeckLegal refuses the
	// 68-card shape for the count reason — that is the branch that broadcasts
	UDataTable* Table = MakeScratchCardTable();
	if (!TestNotNull(TEXT("Scratch card table constructed"), Table))
	{
		return false;
	}
	AddSixtyEightShapeCards(*Table);

	FDeckList Deck1;
	Deck1.DeckName = TEXT("deck1");
	AddSixtyEightCardShape(Deck1);
	TestEqual(TEXT("The measured deck1 totals 68"), Deck1.TotalCount(), 68);

	FString LegalityReason;
	TestFalse(TEXT("The fallback arm's condition holds: the 68-card deck1 is NOT legal"),
		UDeckLibrary::IsDeckLegal(Table, Deck1, LegalityReason));
	TestTrue(TEXT("...for the count reason (names '68 cards')"), LegalityReason.Contains(TEXT("68 cards")));

	// (b) the exact string, the measured pair first
	const FString Notice = SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(Deck1.DeckName, Deck1.TotalCount()).ToString();
	TestEqualSensitive(TEXT("The HUD notice for (\"deck1\", 68) is exactly the pinned text"),
		Notice, FString(TEXT("Deck 'deck1' has 68 cards — playing the default deck")));
	TestFalse(TEXT("...with no unformatted placeholder leaking ({0})"), Notice.Contains(TEXT("{0}")));
	TestFalse(TEXT("...with no unformatted placeholder leaking ({1})"), Notice.Contains(TEXT("{1}")));

	// a second pair — both arguments are interpolated, neither is baked
	const FString OtherNotice = SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(TEXT("deck7"), 49).ToString();
	TestEqualSensitive(TEXT("The HUD notice for (\"deck7\", 49) interpolates both arguments"),
		OtherNotice, FString(TEXT("Deck 'deck7' has 49 cards — playing the default deck")));
	TestNotEqual(TEXT("The two notices differ (neither the name nor the count is baked)"), Notice, OtherNotice);

	return true;
}

/**
 *  TASK-1270 CASE 3 (LOOP 1): A LISTENER THAT SUBSCRIBES AFTER THE NOTICE WAS RAISED
 *  STILL RECEIVES IT — EXACTLY ONCE.
 *
 *  What failed (qa/TASK-1270-verify.md, VERIFY-FAILED): loop 0 broadcast the notice on a
 *  next-tick timer, which fired in the load frame's world tick; the channel's only
 *  listener, WBP_CardHand, is created by WBP_HUD's first widget Tick and binds in
 *  UCardHandWidget::InitForController — AFTER the broadcast. The slot stayed empty.
 *  Loop 1 holds the notice on the controller (QueueMatchStartNotice) and spends it
 *  (DeliverPendingMatchStartNotice) only when OnCardRefused has a listener; the hand
 *  asks for it the moment it binds.
 *
 *  World-free, the house idiom (the NewObject<ASiegeGhostPawn> / NewObject<UCardHandWidget>
 *  precedents): transient controllers and REAL UCardHandWidget listeners bound through the
 *  shipped InitForController — no world, no BeginPlay, no save slot, zero disk. Every row
 *  reads STATE (SC-§104): the controller's held notice (HasPending/GetPending),
 *  OnCardRefused.IsBound(), and what each hand RECEIVED (GetReceivedRefusalCount +
 *  GetLastReceivedRefusal — the receipt is recorded before the BIE).
 *   (a) raised with NOBODY listening ⇒ held, not spent (the loop-0 moment);
 *   (b) the hand subscribes AFTER ⇒ it receives the exact text, once, and the hold clears;
 *   (c) a re-init, a second hand, a direct Deliver ⇒ nobody receives it again;
 *   (d) the reverse order (hand first, then BeginPlay's Queue+Deliver pair) ⇒ received at once;
 *   (e) nothing held (the legal-deck case) or an empty FText ⇒ nothing is received.
 *  ⚠️ NOT assertable here, named: (1) the Blueprint half — WBP_CardHand's handler writing
 *  RefusalText — the verifier observable is `WBP_CardHand_C_0.RefusalText` (`TextBlock_21`)
 *  `Text` = the notice early in the arena; (2) the Log-verbosity line count — in UE 5.8
 *  FAutomationTestMessageFilter::SerializeRecord matches expected messages on Warning/Error
 *  records only (AutomationTest.cpp:305-323), so a Log-level expectation cannot be counted;
 *  the verifier counts `HUD notice broadcast (TASK-1270)` = 1 per match start.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckIllegalDeckNoticeReachesLateListenerOnceTest,
	"Siegebound.Deck.IllegalDeckNoticeReachesALateListenerExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckIllegalDeckNoticeReachesLateListenerOnceTest::RunTest(const FString& Parameters)
{
	// The notice under test is the shipped composer's output for the verifier's measured save.
	const FText Notice = SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(TEXT("deck1"), 51);
	const FString NoticeString = Notice.ToString();
	TestEqualSensitive(TEXT("SELF-CHECK: the notice is the pinned text for (deck1, 51)"),
		NoticeString, FString(TEXT("Deck 'deck1' has 51 cards — playing the default deck")));

	TStrongObjectPtr<ASiegePlayerController> Controller(NewObject<ASiegePlayerController>(
		GetTransientPackageAsObject(), ASiegePlayerController::StaticClass(), NAME_None, RF_Transient));
	TStrongObjectPtr<ASiegePlayerController> ReverseController(NewObject<ASiegePlayerController>(
		GetTransientPackageAsObject(), ASiegePlayerController::StaticClass(), NAME_None, RF_Transient));
	TStrongObjectPtr<UCardHandWidget> LateHand(NewObject<UCardHandWidget>(GetTransientPackageAsObject()));
	TStrongObjectPtr<UCardHandWidget> SecondHand(NewObject<UCardHandWidget>(GetTransientPackageAsObject()));
	TStrongObjectPtr<UCardHandWidget> EarlyHand(NewObject<UCardHandWidget>(GetTransientPackageAsObject()));
	if (!TestTrue(TEXT("SELF-CHECK: two world-free controllers and three hands were created"),
		Controller.IsValid() && ReverseController.IsValid() && LateHand.IsValid() && SecondHand.IsValid() && EarlyHand.IsValid()))
	{
		return false;
	}

	// A world-free hand logs its DESIGNED Warnings on every InitForController: always the
	// no-player-state one (no PlayerState outside a world), and the no-deck one only if the
	// DeckComponent subobject is not discoverable world-free — derived here, not guessed.
	// Four InitForController calls below.
	const int32 InitCalls = 4;
	AddExpectedMessagePlain(TEXT("has no ASiegePlayerState"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, InitCalls);
	if (Controller->FindComponentByClass<UDeckComponent>() == nullptr)
	{
		AddExpectedMessagePlain(TEXT("has no UDeckComponent"), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, InitCalls);
	}

	// PREMISE: a fresh controller holds nothing and has no listener.
	TestFalse(TEXT("PREMISE: a fresh controller holds no notice"), Controller->HasPendingMatchStartNotice());
	TestFalse(TEXT("PREMISE: a fresh controller's OnCardRefused has no listener"), Controller->OnCardRefused.IsBound());
	TestFalse(TEXT("PREMISE: Deliver with nothing held spends nothing"), Controller->DeliverPendingMatchStartNotice());

	// (a) RAISED WITH NOBODY LISTENING — BeginPlay's exact pair (Queue, then Deliver).
	Controller->QueueMatchStartNotice(Notice);
	const bool bSpentWithNoListener = Controller->DeliverPendingMatchStartNotice();
	TestFalse(TEXT("(a) with no listener, Deliver does NOT spend the notice (loop 0 broadcast it to nobody here)"), bSpentWithNoListener);
	TestTrue(TEXT("(a) STATE: the notice is still held"), Controller->HasPendingMatchStartNotice());
	TestEqualSensitive(TEXT("(a) STATE: the held notice is the exact text"),
		Controller->GetPendingMatchStartNotice().ToString(), NoticeString);

	// (b) THE HAND SUBSCRIBES AFTER — through the shipped bind path.
	TestEqual(TEXT("(b) PREMISE: the late hand has received nothing yet"), LateHand->GetReceivedRefusalCount(), 0);
	LateHand->InitForController(Controller.Get());
	TestTrue(TEXT("(b) the hand's bind put a listener on OnCardRefused"), Controller->OnCardRefused.IsBound());
	TestEqual(TEXT("(b) STATE: the late-subscribing hand RECEIVED the notice exactly once"), LateHand->GetReceivedRefusalCount(), 1);
	TestEqualSensitive(TEXT("(b) STATE: ...carrying the exact text"), LateHand->GetLastReceivedRefusal(), NoticeString);
	TestFalse(TEXT("(b) STATE: the controller no longer holds it (spent on delivery)"), Controller->HasPendingMatchStartNotice());
	TestTrue(TEXT("(b) STATE: the held text is empty"), Controller->GetPendingMatchStartNotice().IsEmpty());

	// (c) EXACTLY ONCE — nothing re-delivers it.
	LateHand->InitForController(Controller.Get());
	TestEqual(TEXT("(c) a re-init of the same hand does not receive it again"), LateHand->GetReceivedRefusalCount(), 1);
	SecondHand->InitForController(Controller.Get());
	TestEqual(TEXT("(c) a second hand binding later receives nothing"), SecondHand->GetReceivedRefusalCount(), 0);
	TestFalse(TEXT("(c) a direct Deliver afterwards spends nothing"), Controller->DeliverPendingMatchStartNotice());
	TestEqual(TEXT("(c) STATE: the first hand's receipt count is still 1"), LateHand->GetReceivedRefusalCount(), 1);
	TestEqual(TEXT("(c) STATE: the second hand's receipt count is still 0"), SecondHand->GetReceivedRefusalCount(), 0);

	// (d) THE REVERSE ORDER — a hand already bound, then BeginPlay's pair.
	EarlyHand->InitForController(ReverseController.Get());
	TestEqual(TEXT("(d) PREMISE: binding with nothing held delivers nothing (the legal-deck case)"), EarlyHand->GetReceivedRefusalCount(), 0);
	ReverseController->QueueMatchStartNotice(Notice);
	TestTrue(TEXT("(d) with a listener already bound, Deliver spends the notice at once"), ReverseController->DeliverPendingMatchStartNotice());
	TestEqual(TEXT("(d) STATE: the early hand received it exactly once"), EarlyHand->GetReceivedRefusalCount(), 1);
	TestEqualSensitive(TEXT("(d) STATE: ...carrying the exact text"), EarlyHand->GetLastReceivedRefusal(), NoticeString);
	TestFalse(TEXT("(d) STATE: nothing is held afterwards"), ReverseController->HasPendingMatchStartNotice());

	// (e) AN EMPTY FText IS NOT A NOTICE.
	ReverseController->QueueMatchStartNotice(FText::GetEmpty());
	TestFalse(TEXT("(e) STATE: queueing an empty FText holds nothing"), ReverseController->HasPendingMatchStartNotice());
	TestFalse(TEXT("(e) ...and Deliver spends nothing"), ReverseController->DeliverPendingMatchStartNotice());
	TestEqual(TEXT("(e) STATE: the early hand's receipt count is still 1"), EarlyHand->GetReceivedRefusalCount(), 1);

	return true;
}

/**
 *  TASK-1286 — THE KEYBOARD CARD ACTIONS (🧑 his deck-builder ask, his go
 *  2026-09-17). Asserts STATE (SC-§104): a focused tile, Accept, the deck model
 *  one copy heavier; Remove, the deck model back exactly where it was.
 *
 *  ⛔ WHY THIS ONE MAY DRIVE A REAL UDeckBuilderWidget WHEN THE FILE HEADER SAYS
 *  DRIVING ITS MUTATORS "COULD WRITE THE PLAYER'S ACTUAL GUEST SLOT" — read this
 *  before relaxing anything here. The header's warning is about a CONSTRUCTED
 *  builder. This widget is NewObject'd and NativeConstruct NEVER RUNS, so
 *  EditingDeckIndex stays INDEX_NONE, and THE ONE AUTO-SAVE FUNNEL refuses on
 *  exactly that index (DeckBuilderWidget.cpp PersistWorkingDeck: "no editing slot
 *  selected yet — mutation NOT auto-saved"). Nothing reaches SaveDeckAs, so the
 *  ACC-§4 seam is never resolved and no slot — real, profile or guest — is
 *  opened for writing. That is a MECHANICAL guarantee, not a convention, and it
 *  is ASSERTED rather than assumed, two ways:
 *    · the INDEX_NONE precondition is a TestEqual below, and
 *    · the refusal Warning is pinned to EXACTLY 2 occurrences (one per successful
 *      mutation) — AddExpectedMessagePlain with Occurrences > 0 fails the test on
 *      0 or on 3+, so a future edit that makes PersistWorkingDeck actually write
 *      turns this test RED instead of silently touching Jonathan's deck.
 *  The FDeckScratchGuard below is the third belt: it deletes the scratch slot on
 *  the way in and out, and the test asserts the scratch slot is never created.
 *
 *  Zero network, zero PIE, zero widget tree. The one asset touched is the shipped
 *  /Game/Data/DT_Cards, READ through the widget's own soft pointer — the
 *  SiegeCardRosterTest.cpp:378 / SiegeCardArtRosterTest.cpp:431 precedent for
 *  this lane.
 *
 *  M8: adds no replicated property, no new replicated class, no RPC.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckKeyboardFocusAcceptAndRemoveTest,
	"Siegebound.Deck.KeyboardFocusedTileAcceptAddsOneCopyAndRemoveTakesItBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckKeyboardFocusAcceptAndRemoveTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	FDeckScratchGuard ScratchGuard;

	// ---------------------------------------------------------------------
	// (0) THE PURE RING — StepCardFocusIndex is the whole of the movement
	//     arithmetic, factored out so it is assertable with no world, no
	//     widget tree and no Slate. "Wrap at the ends" is the row's wording.
	// ---------------------------------------------------------------------
	{
		// A 28-card collection laid out 7 wide is the shipped shape.
		const int32 Count = 28;
		const int32 Columns = 7;

		TestEqual(TEXT("(0) an empty collection has no focus at all"),
			UDeckBuilderWidget::StepCardFocusIndex(0, 0, Columns, EUINavigation::Right), (int32)INDEX_NONE);
		TestEqual(TEXT("(0) arming with Down lands on the FIRST tile"),
			UDeckBuilderWidget::StepCardFocusIndex(INDEX_NONE, Count, Columns, EUINavigation::Down), 0);
		TestEqual(TEXT("(0) arming with Right lands on the FIRST tile"),
			UDeckBuilderWidget::StepCardFocusIndex(INDEX_NONE, Count, Columns, EUINavigation::Right), 0);
		TestEqual(TEXT("(0) arming with Up lands on the LAST tile"),
			UDeckBuilderWidget::StepCardFocusIndex(INDEX_NONE, Count, Columns, EUINavigation::Up), Count - 1);
		TestEqual(TEXT("(0) arming with Left lands on the LAST tile"),
			UDeckBuilderWidget::StepCardFocusIndex(INDEX_NONE, Count, Columns, EUINavigation::Left), Count - 1);
		TestEqual(TEXT("(0) Right steps one along grid order"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, Columns, EUINavigation::Right), 4);
		TestEqual(TEXT("(0) Left steps one back"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, Columns, EUINavigation::Left), 2);
		TestEqual(TEXT("(0) Down steps one ROW (the measured column count)"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, Columns, EUINavigation::Down), 10);
		TestEqual(TEXT("(0) Up steps one row back"),
			UDeckBuilderWidget::StepCardFocusIndex(10, Count, Columns, EUINavigation::Up), 3);
		TestEqual(TEXT("(0) WRAP: Right off the last tile returns to the first"),
			UDeckBuilderWidget::StepCardFocusIndex(Count - 1, Count, Columns, EUINavigation::Right), 0);
		TestEqual(TEXT("(0) WRAP: Left off the first tile returns to the last"),
			UDeckBuilderWidget::StepCardFocusIndex(0, Count, Columns, EUINavigation::Left), Count - 1);
		TestEqual(TEXT("(0) WRAP: Up from the top row wraps into the bottom, same column"),
			UDeckBuilderWidget::StepCardFocusIndex(2, Count, Columns, EUINavigation::Up), 23);
		TestEqual(TEXT("(0) WRAP: Down from the bottom row wraps into the top, same column"),
			UDeckBuilderWidget::StepCardFocusIndex(23, Count, Columns, EUINavigation::Down), 2);
		TestEqual(TEXT("(0) an UNMEASURABLE row width (1) degrades Down to Right — ⛔ never a guessed column count"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, 1, EUINavigation::Down), 4);
		TestEqual(TEXT("(0) a nonsense row width is clamped, not trusted"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, -99, EUINavigation::Down), 4);
		TestEqual(TEXT("(0) Next/Previous/Invalid move nothing (only the four cardinals are mapped)"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, Columns, EUINavigation::Next), 3);
		TestEqual(TEXT("(0) ...Invalid likewise"),
			UDeckBuilderWidget::StepCardFocusIndex(3, Count, Columns, EUINavigation::Invalid), 3);
		TestEqual(TEXT("(0) ⛔ an unmapped direction can NOT arm the grid from nothing"),
			UDeckBuilderWidget::StepCardFocusIndex(INDEX_NONE, Count, Columns, EUINavigation::Next), (int32)INDEX_NONE);
		TestEqual(TEXT("(0) ...Invalid cannot arm it either"),
			UDeckBuilderWidget::StepCardFocusIndex(INDEX_NONE, Count, Columns, EUINavigation::Invalid), (int32)INDEX_NONE);
	}

	// ---------------------------------------------------------------------
	// (1) THE FEATURE, on a real widget with a real collection
	// ---------------------------------------------------------------------
	TStrongObjectPtr<UDeckBuilderWidget> Builder(NewObject<UDeckBuilderWidget>(GetTransientPackageAsObject()));
	if (!TestTrue(TEXT("SELF-CHECK: a world-free deck builder was created"), Builder.IsValid()))
	{
		return false;
	}

	// ⛔ THE NO-WRITE PRECONDITION, ASSERTED. NativeConstruct never ran, so no
	// editing slot was ever selected and PersistWorkingDeck refuses every
	// mutation below before it can reach SaveDeckAs.
	TestEqual(TEXT("PREMISE: no editing slot is selected — the auto-save funnel is mechanically closed"),
		Builder->GetEditingDeckIndex(), (int32)INDEX_NONE);

	// ...and pinned: EXACTLY 2 refusals, one per successful mutation (the Accept
	// and the Remove below). 0 or 3+ fails this test, which is how a future edit
	// that lets the funnel through announces itself instead of writing his deck.
	AddExpectedMessagePlain(TEXT("PersistWorkingDeck: no editing slot selected yet"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 2);

	const TArray<FName> Collection = Builder->GetCollectionCardIDs();
	if (!TestTrue(TEXT("SELF-CHECK: the shipped DT_Cards collection resolved with at least two cards"), Collection.Num() >= 2))
	{
		return false;
	}

	// "Tile K" — deliberately a MIDDLE tile, so a bug that silently focuses index
	// 0 (or the first tile it finds) cannot pass by coincidence.
	const int32 TileK = Collection.Num() / 2;
	const FName CardK = Collection[TileK];

	TestEqual(TEXT("PREMISE: a fresh builder has nothing focused"), Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);
	TestTrue(TEXT("PREMISE: ...and GetFocusedCardID is None"), Builder->GetFocusedCardID().IsNone());

	Builder->SetFocusedCardIndex(TileK);
	TestEqual(TEXT("STATE: tile K is the focused tile"), Builder->GetFocusedCardIndex(), TileK);
	TestEqual(TEXT("STATE: the focused card is DT_Cards row K, by name"), Builder->GetFocusedCardID(), CardK);

	const int32 CountBefore = Builder->GetCountOf(CardK);
	const int32 TotalBefore = Builder->GetTotalCount();

	// (a) ACCEPT = what left-clicking "+" does.
	Builder->AcceptFocusedCard();
	TestEqual(TEXT("(a) STATE: Accept added EXACTLY one copy of card K"), Builder->GetCountOf(CardK), CountBefore + 1);
	TestEqual(TEXT("(a) STATE: ...and the x/50 total is +1"), Builder->GetTotalCount(), TotalBefore + 1);
	TestEqual(TEXT("(a) STATE: Accept did not move the focus"), Builder->GetFocusedCardIndex(), TileK);

	// (b) REMOVE = what left-clicking "−" does — both back.
	Builder->RemoveFocusedCard();
	TestEqual(TEXT("(b) STATE: Remove took the copy back"), Builder->GetCountOf(CardK), CountBefore);
	TestEqual(TEXT("(b) STATE: ...and the x/50 total is back"), Builder->GetTotalCount(), TotalBefore);
	TestEqual(TEXT("(b) STATE: Remove did not move the focus"), Builder->GetFocusedCardIndex(), TileK);

	// (c) WITH NOTHING FOCUSED BOTH ARE SILENT NO-OPS — the guard that stops the
	//     keys meaning anything while the focus is on the deck bar, Play or Exit.
	//     (These two calls must NOT add a third PersistWorkingDeck refusal.)
	Builder->SetFocusedCardIndex(INDEX_NONE);
	TestEqual(TEXT("(c) STATE: the focus cleared"), Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);
	Builder->AcceptFocusedCard();
	Builder->RemoveFocusedCard();
	TestEqual(TEXT("(c) STATE: with nothing focused, Accept/Remove changed no count"), Builder->GetCountOf(CardK), CountBefore);
	TestEqual(TEXT("(c) STATE: ...and no total"), Builder->GetTotalCount(), TotalBefore);

	// (d) AN OUT-OF-RANGE INDEX CLEARS RATHER THAN CLAMPS (⛔ never silently
	//     focuses a neighbouring card).
	Builder->SetFocusedCardIndex(Collection.Num());
	TestEqual(TEXT("(d) STATE: an index past the end clears the focus"), Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);
	Builder->SetFocusedCardIndex(-7);
	TestEqual(TEXT("(d) STATE: a negative index clears the focus"), Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);

	// (e) MoveCardFocus ARMS from nothing and then walks — driven through the
	//     public mutator, not the static, so the widget's own wiring is covered.
	Builder->MoveCardFocus(EUINavigation::Down);
	TestEqual(TEXT("(e) STATE: Down from nothing arms on the first tile"), Builder->GetFocusedCardIndex(), 0);
	Builder->MoveCardFocus(EUINavigation::Right);
	TestEqual(TEXT("(e) STATE: Right walks to the second tile"), Builder->GetFocusedCardIndex(), 1);
	Builder->MoveCardFocus(EUINavigation::Left);
	Builder->MoveCardFocus(EUINavigation::Left);
	TestEqual(TEXT("(e) STATE: Left off the first tile WRAPS to the last"), Builder->GetFocusedCardIndex(), Collection.Num() - 1);

	// (f) THE REAL SAVE WAS NEVER OPENED — the scratch slot the guard owns was
	//     never created either, so nothing in this test wrote a deck anywhere.
	TestFalse(TEXT("(f) STATE: the scratch deck slot does not exist — this test wrote no save at all"),
		UGameplayStatics::DoesSaveGameExist(ScratchDeckSlotName, USiegeDeckSaveGame::UserIndex));

	return true;
}

/**
 *  TASK-1286, 2026-09-17 AMENDMENT — LEAVING THE CARD GRID (🧑 his ruling: gamepad
 *  B stops deleting cards and starts meaning Back). Asserts STATE (SC-§104): the
 *  focused index is a real middle tile K, then INDEX_NONE.
 *
 *  ⛔⛔ THE EXIT IS GAMEPAD-ONLY — Gamepad_FaceButton_Right / Virtual_Back, and
 *  NOTHING ELSE. The Escape key was DROPPED from this binding under AS-§6 A-2,
 *  which at its current width leaves the Escape key permanently unabsorbed
 *  project-wide: nothing in this project handles it. ⛔ Do NOT bind the Escape
 *  key here to "complete" what a comment seems to describe — a Slate
 *  FReply::Handled() on that key overturns a CLOSED Jonathan ruling and is an
 *  automatic QA FAIL. (A-2's scope is his own open question at TASK-1300; until
 *  he rules, A-2 stands at its widest and this lane stays gamepad-only.)
 *
 *  ⛔⛔ WHY THE ASSERTION IS THE INDEX AND ⛔ NOT IsCardGridFocusLive() — READ THIS
 *  BEFORE "STRENGTHENING" THIS TEST (SC-§39, and the manager named this trap in
 *  the row itself). The amendment folds QA's WARN-3, so IsCardGridFocusLive()
 *  now returns FALSE in this lane BY CONSTRUCTION: FSlateApplication IS
 *  initialised under EditorContext, and no WBP_DeckCardTile resolves for a
 *  NewObject'd builder, which is exactly the live-Slate/no-tile branch that was
 *  just changed to `return false`. ⇒ an assertion "IsCardGridFocusLive() is false
 *  after the exit" ⛔ CANNOT FAIL, and a test that cannot fail is not evidence.
 *  The index CAN report both values, and this test proves it does:
 *    · the POSITIVE CONTROL — GetFocusedCardIndex() == K BEFORE the exit, so the
 *      instrument is shown able to report a non-INDEX_NONE value at all; then
 *    · GetFocusedCardIndex() == INDEX_NONE after.
 *  Exit conditions (ii) "no tile holds Slate focus" and (iii) close at the VERIFY
 *  leg on a real grid, ⛔ never here.
 *
 *  ⛔ NO EXPECTED-MESSAGE PIN IS ADDED, AND THAT IS MEASURED, NOT ASSUMED:
 *  ExitCardGridFocus() mutates no deck — it clears an index and moves Slate focus
 *  — so it reaches neither AddCopy nor RemoveCopy and therefore adds ZERO
 *  PersistWorkingDeck refusals. The sibling test's Occurrences-2 pin lives in its
 *  own RunTest scope and is untouched by this file's second test (an off-by-one
 *  pin would be a RED for the wrong reason, which is why this is stated).
 *
 *  Same no-write architecture as the sibling: NewObject'd builder, NativeConstruct
 *  never runs, EditingDeckIndex stays INDEX_NONE, the DECK-§4 auto-save funnel is
 *  mechanically closed. Zero network, zero PIE, zero widget tree.
 *
 *  M8: adds no replicated property, no new replicated class, no RPC.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeckExitCardGridTest,
	"Siegebound.Deck.ExitingCardGridClearsTheFocusedIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeckExitCardGridTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeckSlotsTestUtils;

	FDeckScratchGuard ScratchGuard;

	TStrongObjectPtr<UDeckBuilderWidget> Builder(NewObject<UDeckBuilderWidget>(GetTransientPackageAsObject()));
	if (!TestTrue(TEXT("SELF-CHECK: a world-free deck builder was created"), Builder.IsValid()))
	{
		return false;
	}

	// The same mechanical no-write guarantee the sibling test asserts.
	TestEqual(TEXT("PREMISE: no editing slot is selected — the auto-save funnel is mechanically closed"),
		Builder->GetEditingDeckIndex(), (int32)INDEX_NONE);

	const TArray<FName> Collection = Builder->GetCollectionCardIDs();
	if (!TestTrue(TEXT("SELF-CHECK: the shipped DT_Cards collection resolved with at least two cards"), Collection.Num() >= 2))
	{
		return false;
	}

	// Tile K is a MIDDLE tile again, so "the exit cleared it" cannot be confused
	// with "it was 0 all along".
	const int32 TileK = Collection.Num() / 2;

	TestEqual(TEXT("PREMISE: a fresh builder has nothing focused"), Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);

	// Captured BEFORE anything runs, so (f) below compares against a value taken
	// at a different time — ⛔ never against a second read of itself.
	const int32 TotalBefore = Builder->GetTotalCount();
	const int32 CountOfKBefore = Builder->GetCountOf(Collection[TileK]);

	// (a) ⛔ THE (C) RULING'S OTHER HALF, ASSERTED FIRST: with nothing focused the
	//     exit changes no state and REFUSES to claim the gesture. This is the
	//     sentence that keeps a Back press aimed at the SCREEN falling through
	//     instead of being silently swallowed — NativeOnKeyDown returns
	//     FReply::Handled() only on a `true` from here.
	TestFalse(TEXT("(a) STATE: with nothing focused the exit acts on nothing and reports false — the Back key is NOT consumed"),
		Builder->ExitCardGridFocus());
	TestEqual(TEXT("(a) STATE: ...and the focus is still nothing"), Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);

	// (b) THE POSITIVE CONTROL — the getter CAN report a real tile index. Without
	//     this line the INDEX_NONE below would be indistinguishable from a getter
	//     that never reports anything else.
	Builder->SetFocusedCardIndex(TileK);
	TestEqual(TEXT("(b) POSITIVE CONTROL: tile K is the focused tile BEFORE the exit"),
		Builder->GetFocusedCardIndex(), TileK);

	// (c) THE EXIT — it acted, so it claims the gesture...
	TestTrue(TEXT("(c) STATE: exiting a focused grid reports true — the Back key IS consumed, exactly once"),
		Builder->ExitCardGridFocus());
	// ...and (D)(i), the assertion this test exists for.
	TestEqual(TEXT("(c) STATE: the focused index is cleared to INDEX_NONE"),
		Builder->GetFocusedCardIndex(), (int32)INDEX_NONE);
	TestTrue(TEXT("(c) STATE: ...and GetFocusedCardID reports None with it"),
		Builder->GetFocusedCardID().IsNone());

	// (d) A SECOND PRESS FALLS THROUGH — the whole point of the nested-Back
	//     ruling: gamepad B once leaves the grid, B again leaves the builder,
	//     because the second press finds nothing to act on and does not claim the
	//     key. ⛔ B ONLY (Gamepad_FaceButton_Right / Virtual_Back) — the Escape
	//     key is NOT bound to either leg and must not be, per AS-§6 A-2.
	TestFalse(TEXT("(d) STATE: a SECOND exit acts on nothing and does not consume the key"),
		Builder->ExitCardGridFocus());

	// (e) ⛔ THE EXIT MUST NOT BRICK THE GRID — re-entry still works, so a player
	//     who leaves by mistake is not locked out of the cards.
	Builder->MoveCardFocus(EUINavigation::Down);
	TestEqual(TEXT("(e) STATE: Down re-arms the grid on the first tile after an exit"),
		Builder->GetFocusedCardIndex(), 0);

	// (f) ⛔ THE EXIT IS NOT A DECK MUTATION — it moves focus, never cards. A
	//     regression that routed it through RemoveCopy (the binding it REPLACED on
	//     gamepad B) would show up right here as a missing copy.
	TestEqual(TEXT("(f) STATE: the x/50 total is untouched by three exits"),
		Builder->GetTotalCount(), TotalBefore);
	TestEqual(TEXT("(f) STATE: ...and card K still has exactly the copies it started with"),
		Builder->GetCountOf(Collection[TileK]), CountOfKBefore);
	TestFalse(TEXT("(f) STATE: the scratch deck slot does not exist — this test wrote no save at all"),
		UGameplayStatics::DoesSaveGameExist(ScratchDeckSlotName, USiegeDeckSaveGame::UserIndex));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
