// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeDeckSaveGame.h"

#include "GitClaudeUnrealTest.h"

// Fixed slot name shared by every reader/writer (M6 ruling 1). One definition
// here so all translation units link to the same string.
const FString USiegeDeckSaveGame::SlotName = TEXT("SiegeDecks");

// ---------------------------------------------------------------------------
// The ten fixed deck slots (TASK-670 — CONVENTIONS DECK-§1/§2)
// ---------------------------------------------------------------------------

FString USiegeDeckSaveGame::MakeFixedDeckName(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= NumFixedDeckSlots)
	{
		// "no such slot" — silent by contract (BlueprintPure-adjacent hot path;
		// callers guard the range and SaveDeckAs refuses an empty name anyway)
		return FString();
	}

	// the ONE "deck" + number composition site (DECK-§1) — lowercase verbatim
	return FString::Printf(TEXT("deck%d"), SlotIndex + 1);
}

int32 USiegeDeckSaveGame::FindFixedDeckIndex(const FString& DeckName)
{
	// Ten Equals calls against the ONE composer instead of hand-rolled parsing:
	// zero suffix edge cases ("deck01", "deck 1", " deck1" are all correctly
	// non-fixed) and the composition stays in one place (DECK-§1).
	for (int32 SlotIndex = 0; SlotIndex < NumFixedDeckSlots; ++SlotIndex)
	{
		if (DeckName.Equals(MakeFixedDeckName(SlotIndex), ESearchCase::IgnoreCase))
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

bool USiegeDeckSaveGame::MigrateToFixedSlots(USiegeDeckSaveGame& Save)
{
	// ---- The idempotence gate (DECK-§2): a save already in CANONICAL fixed
	// form returns false without touching anything — byte-stable by
	// construction, because this early-out writes nothing. Canonical means:
	// exactly ten decks, slot i named MakeFixedDeckName(i) CASE-SENSITIVELY
	// (the triple-duty law makes the lowercase byte form load-bearing — the
	// match reader at SiegePlayerController.cpp:272 and the cloud pull merge
	// both compare case-sensitively), and ActiveDeckName one of the ten
	// canonical names.
	if (Save.SavedDecks.Num() == NumFixedDeckSlots)
	{
		bool bCanonical = true;
		for (int32 SlotIndex = 0; bCanonical && SlotIndex < NumFixedDeckSlots; ++SlotIndex)
		{
			bCanonical = Save.SavedDecks[SlotIndex].DeckName.Equals(
				MakeFixedDeckName(SlotIndex), ESearchCase::CaseSensitive);
		}
		if (bCanonical)
		{
			bool bActiveCanonical = false;
			for (int32 SlotIndex = 0; !bActiveCanonical && SlotIndex < NumFixedDeckSlots; ++SlotIndex)
			{
				bActiveCanonical = Save.ActiveDeckName.Equals(
					MakeFixedDeckName(SlotIndex), ESearchCase::CaseSensitive);
			}
			if (bActiveCanonical)
			{
				return false; // fixed form — no-op (idempotent)
			}
		}
	}

	// ---- The DECK-§2 mapping (defaults D1/D2). Work on a snapshot so the
	// rebuild below can move decks without aliasing the array it reads.
	TArray<FDeckList> LegacyDecks = MoveTemp(Save.SavedDecks);
	Save.SavedDecks.Reset();

	// The previously-ACTIVE legacy deck: FIRST case-insensitive name match
	// (the shipped M6 activation idiom). A dangling name (no match) counts as
	// "no active deck" for clause 6.
	int32 ActiveLegacyIndex = INDEX_NONE;
	if (!Save.ActiveDeckName.IsEmpty())
	{
		for (int32 LegacyIndex = 0; LegacyIndex < LegacyDecks.Num(); ++LegacyIndex)
		{
			if (LegacyDecks[LegacyIndex].DeckName.Equals(Save.ActiveDeckName, ESearchCase::IgnoreCase))
			{
				ActiveLegacyIndex = LegacyIndex;
				break;
			}
		}
	}

	TArray<FDeckList> NewDecks;
	NewDecks.SetNum(NumFixedDeckSlots);
	TArray<bool> SlotOccupied;
	SlotOccupied.Init(false, NumFixedDeckSlots);
	TArray<bool> LegacyPlaced;
	LegacyPlaced.Init(false, LegacyDecks.Num());
	int32 ActiveSlotIndex = INDEX_NONE;

	// Clause 1: legacy decks ALREADY bearing a fixed name (case-insensitive)
	// keep their slot; the name is canonicalized to lowercase (triple-duty
	// law). First claimant wins — a duplicate fixed name (impossible from the
	// shipped overwrite-on-collision writers) falls through to clauses 3/4.
	for (int32 LegacyIndex = 0; LegacyIndex < LegacyDecks.Num(); ++LegacyIndex)
	{
		const int32 SlotIndex = FindFixedDeckIndex(LegacyDecks[LegacyIndex].DeckName);
		if (SlotIndex != INDEX_NONE && !SlotOccupied[SlotIndex])
		{
			NewDecks[SlotIndex] = MoveTemp(LegacyDecks[LegacyIndex]);
			NewDecks[SlotIndex].DeckName = MakeFixedDeckName(SlotIndex);
			SlotOccupied[SlotIndex] = true;
			LegacyPlaced[LegacyIndex] = true;
			if (LegacyIndex == ActiveLegacyIndex)
			{
				ActiveSlotIndex = SlotIndex;
			}
		}
	}

	// Clause 2: the previously-active legacy deck (when not already placed by
	// clause 1) → the LOWEST empty slot — normally deck1 (D2).
	if (ActiveLegacyIndex != INDEX_NONE && !LegacyPlaced[ActiveLegacyIndex])
	{
		for (int32 SlotIndex = 0; SlotIndex < NumFixedDeckSlots; ++SlotIndex)
		{
			if (!SlotOccupied[SlotIndex])
			{
				NewDecks[SlotIndex] = MoveTemp(LegacyDecks[ActiveLegacyIndex]);
				NewDecks[SlotIndex].DeckName = MakeFixedDeckName(SlotIndex);
				SlotOccupied[SlotIndex] = true;
				LegacyPlaced[ActiveLegacyIndex] = true;
				ActiveSlotIndex = SlotIndex;
				break;
			}
		}
		// no empty slot ⇒ ten fixed-named legacy decks already claimed every
		// slot; the active deck falls through to clause 4 (dropped) and clause
		// 6 applies its no-active default.
	}

	// Clause 3: remaining legacy decks fill remaining empty slots in SavedDecks
	// order. Clause 4: whatever finds no slot is DROPPED, names collected for
	// the one Warning below (D2 — overflow is logged, never silent).
	TArray<FString> DroppedNames;
	for (int32 LegacyIndex = 0; LegacyIndex < LegacyDecks.Num(); ++LegacyIndex)
	{
		if (LegacyPlaced[LegacyIndex])
		{
			continue;
		}

		int32 TargetSlot = INDEX_NONE;
		for (int32 SlotIndex = 0; SlotIndex < NumFixedDeckSlots; ++SlotIndex)
		{
			if (!SlotOccupied[SlotIndex])
			{
				TargetSlot = SlotIndex;
				break;
			}
		}

		if (TargetSlot == INDEX_NONE)
		{
			DroppedNames.Add(LegacyDecks[LegacyIndex].DeckName);
			continue;
		}

		NewDecks[TargetSlot] = MoveTemp(LegacyDecks[LegacyIndex]);
		NewDecks[TargetSlot].DeckName = MakeFixedDeckName(TargetSlot);
		SlotOccupied[TargetSlot] = true;
		LegacyPlaced[LegacyIndex] = true;
	}

	if (DroppedNames.Num() > 0)
	{
		// ONE Warning listing every dropped name (DECK-§2 clause 4). ASCII-only
		// literal (the module's string-literal law); the law cite lives here in
		// the comment, not in the log text.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("USiegeDeckSaveGame::MigrateToFixedSlots: dropped %d legacy deck(s) beyond the %d fixed slots: %s"),
			DroppedNames.Num(), NumFixedDeckSlots, *FString::Join(DroppedNames, TEXT(", ")));
	}

	// Clause 5: every still-empty slot materializes as an EMPTY fixed-name deck
	// (D1) — post-migration all ten entries always exist (DECK-§1).
	for (int32 SlotIndex = 0; SlotIndex < NumFixedDeckSlots; ++SlotIndex)
	{
		if (!SlotOccupied[SlotIndex])
		{
			NewDecks[SlotIndex].DeckName = MakeFixedDeckName(SlotIndex);
			// NewDecks[SlotIndex].Cards stays empty
		}
	}

	// Clause 6: ActiveDeckName becomes the fixed name its deck landed in —
	// "deck1" (slot 0) when there was no active deck, the name dangled, or the
	// active deck itself was overflow-dropped (Jonathan's fresh-account
	// default).
	Save.SavedDecks = MoveTemp(NewDecks);
	Save.ActiveDeckName = MakeFixedDeckName(ActiveSlotIndex != INDEX_NONE ? ActiveSlotIndex : 0);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeDeckSaveGame::MigrateToFixedSlots: migrated to the %d fixed slots — active deck '%s'."),
		NumFixedDeckSlots, *Save.ActiveDeckName);

	return true;
}
