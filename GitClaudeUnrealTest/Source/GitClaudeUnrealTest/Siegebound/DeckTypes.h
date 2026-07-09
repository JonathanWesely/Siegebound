// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DeckTypes.generated.h"

/**
 *  One card slot of a deck: a CardID (a DT_Cards row name, PascalCase) and how
 *  many copies of it the deck holds. Pure data — it stores, it never validates.
 *  The legal range of Count ([0..the row's MaxCopies]) and the exactly-50 rule
 *  live in the ONE validator, UDeckLibrary::IsDeckLegal (never duplicated here).
 *
 *  Members are UPROPERTY so they serialize inside USiegeDeckSaveGame's
 *  TArray<FDeckList> (tagged-property serialization recurses into USTRUCT
 *  UPROPERTYs) and EditAnywhere so ASiegeBotController's EditDefaultsOnly
 *  BotDecks (TASK-114) are tunable per-BP in the details panel.
 */
USTRUCT(BlueprintType)
struct GITCLAUDEUNREALTEST_API FDeckCardEntry
{
	GENERATED_BODY()

	/** DT_Cards row name (CardID). NAME_None = an empty/unset slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siegebound|Deck")
	FName CardID = NAME_None;

	/** Copies of this card in the deck. Legal range is [0..row MaxCopies] (enforced by UDeckLibrary::IsDeckLegal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siegebound|Deck")
	int32 Count = 0;
};

/**
 *  A named deck: a player-entered (or curated) name plus its card entries
 *  (GDD §3.4). Keyed by DeckName in USiegeDeckSaveGame::SavedDecks (overwrite-
 *  on-collision, M6 ruling 2). A deck is LEGAL when TotalCount() ==
 *  SiegeLegalDeckSize AND every entry respects its row's MaxCopies — that rule
 *  is UDeckLibrary::IsDeckLegal, not this struct. This struct only stores.
 */
USTRUCT(BlueprintType)
struct GITCLAUDEUNREALTEST_API FDeckList
{
	GENERATED_BODY()

	/** Player-entered deck name; also the SavedDecks key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siegebound|Deck")
	FString DeckName;

	/** Card entries. Order is presentation-only; the validator reads Count per CardID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siegebound|Deck")
	TArray<FDeckCardEntry> Cards;

	/** Sum of every entry's Count — the value IsDeckLegal compares to SiegeLegalDeckSize. */
	int32 TotalCount() const
	{
		int32 Total = 0;
		for (const FDeckCardEntry& Entry : Cards)
		{
			Total += Entry.Count;
		}
		return Total;
	}
};

/**
 *  GDD §3.4 legal deck size — a Siegebound deck is exactly 50 cards. The shared
 *  constant referenced by UDeckLibrary::IsDeckLegal and by UDeckComponent's
 *  build path (TASK-114). Internal linkage per translation unit (header-only).
 */
static constexpr int32 SiegeLegalDeckSize = 50;
