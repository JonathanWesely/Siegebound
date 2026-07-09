// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeckLibrary.h"

#include "Engine/DataTable.h"
#include "Siegebound/CardRow.h"

bool UDeckLibrary::IsDeckLegal(const UDataTable* CardTable, const FDeckList& Deck, FString& OutReason)
{
	if (!CardTable)
	{
		// null table ⇒ we cannot resolve any card's MaxCopies — illegal, with a reason (never a crash)
		OutReason = TEXT("No card table (DT_Cards) supplied.");
		return false;
	}

	// Per-CardID running copy total: the copy cap is enforced on the AGGREGATE so
	// a deck that splits one card across duplicate entries still cannot exceed
	// MaxCopies. For the widget's one-entry-per-card deck this equals a per-entry
	// check, and the "first violation" is reported deterministically in entry order.
	TMap<FName, int32> RunningCounts;
	RunningCounts.Reserve(Deck.Cards.Num());

	for (const FDeckCardEntry& Entry : Deck.Cards)
	{
		const FCardRow* Row = CardTable->FindRow<FCardRow>(Entry.CardID, TEXT("UDeckLibrary::IsDeckLegal"), /*bWarnIfRowMissing=*/ false);
		if (!Row)
		{
			OutReason = FString::Printf(TEXT("Unknown card '%s' — no matching row in DT_Cards."), *Entry.CardID.ToString());
			return false;
		}

		if (Entry.Count < 0)
		{
			OutReason = FString::Printf(TEXT("Card '%s' has a negative copy count (%d)."), *Entry.CardID.ToString(), Entry.Count);
			return false;
		}

		int32& Running = RunningCounts.FindOrAdd(Entry.CardID);
		Running += Entry.Count;
		if (Running > Row->MaxCopies)
		{
			OutReason = FString::Printf(TEXT("Card '%s' has %d copies — the cap is %d (MaxCopies)."), *Entry.CardID.ToString(), Running, Row->MaxCopies);
			return false;
		}
	}

	const int32 Total = Deck.TotalCount();
	if (Total != SiegeLegalDeckSize)
	{
		OutReason = FString::Printf(TEXT("Deck has %d cards — a legal deck is exactly %d (GDD 3.4)."), Total, SiegeLegalDeckSize);
		return false;
	}

	OutReason.Reset();
	return true;
}

float UDeckLibrary::GetDeckAverageCost(const UDataTable* CardTable, const FDeckList& Deck)
{
	const int32 Total = Deck.TotalCount();
	if (!CardTable || Total <= 0)
	{
		// empty deck (or nothing to divide by, or no table) ⇒ 0, never a divide-by-zero
		return 0.0f;
	}

	int32 CostSum = 0;
	for (const FDeckCardEntry& Entry : Deck.Cards)
	{
		if (Entry.Count <= 0)
		{
			continue;
		}

		// null-safe: an unresolvable CardID simply contributes 0 cost (§8 is a
		// display-only guide); the denominator stays the deck's TotalCount()
		if (const FCardRow* Row = CardTable->FindRow<FCardRow>(Entry.CardID, TEXT("UDeckLibrary::GetDeckAverageCost"), /*bWarnIfRowMissing=*/ false))
		{
			CostSum += Row->Cost * Entry.Count;
		}
	}

	return static_cast<float>(CostSum) / static_cast<float>(Total);
}
