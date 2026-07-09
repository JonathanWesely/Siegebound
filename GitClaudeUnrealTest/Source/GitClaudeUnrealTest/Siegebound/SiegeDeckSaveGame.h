// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Siegebound/DeckTypes.h"
#include "SiegeDeckSaveGame.generated.h"

/**
 *  Cross-session store for player-authored named decks AND the menu→match
 *  handoff (M6 ruling 1). Persisted with UGameplayStatics::SaveGameToSlot /
 *  LoadGameFromSlot to the fixed slot USiegeDeckSaveGame::SlotName ("SiegeDecks")
 *  at user index USiegeDeckSaveGame::UserIndex (0) →
 *  Saved/SaveGames/SiegeDecks.sav. Chosen over a DataAsset (not runtime-writable
 *  in a packaged build) and a hand-rolled .json (SaveGame is the idiomatic
 *  runtime-writable, cross-session, package-safe store).
 *
 *  Reader contract (TASK-114 / TASK-116): no save file / unresolvable ⇒ treat as
 *  an empty SavedDecks list + empty ActiveDeckName (null-safe), so the match
 *  falls back to the curated DeckCount default and nothing crashes. The
 *  SavedDecks / ActiveDeckName members are UPROPERTY so the SaveGame archive
 *  serializes them (FDeckList/FDeckCardEntry members are likewise UPROPERTY, so
 *  tagged-property serialization round-trips the nested decks).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeDeckSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	/** Fixed SaveGame slot name — the ONE slot every reader/writer shares, character-for-character (M6 ruling 1). Defined in the .cpp. */
	static const FString SlotName;

	/** User index for the SaveGame slot (single local player ⇒ 0). */
	static constexpr int32 UserIndex = 0;

	/** All named decks the player has saved. Keyed by FDeckList::DeckName (overwrite-on-collision, M6 ruling 2). */
	UPROPERTY(BlueprintReadWrite, Category = "Siegebound|Deck")
	TArray<FDeckList> SavedDecks;

	/** Name of the deck the next match uses; empty ⇒ no active deck ⇒ curated DeckCount fallback. */
	UPROPERTY(BlueprintReadWrite, Category = "Siegebound|Deck")
	FString ActiveDeckName;
};
