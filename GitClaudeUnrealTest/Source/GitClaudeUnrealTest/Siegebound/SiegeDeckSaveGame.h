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
 *
 *  THE TEN-SLOT MODEL (TASK-670, CONVENTIONS DECK-§1/§2): the class is NOT
 *  restructured — this wave ADDS statics only. The ten deck names are
 *  "deck1".."deck10" (lowercase, character-for-character; label = save key =
 *  cloud decks.deck_name — the triple-duty law). MakeFixedDeckName /
 *  FindFixedDeckIndex are the ONE slot-index↔name implementation — no other
 *  site composes "deck" + number. MigrateToFixedSlots is the ONE migration
 *  implementation; it runs from UDeckBuilderWidget::NativeConstruct only
 *  (builder-open-time), and the match path (SiegePlayerController) NEVER
 *  migrates — a pre-migration save still resolves by name there. Post-
 *  migration, all ten entries ALWAYS exist in SavedDecks (an empty slot = an
 *  FDeckList with the fixed name and 0 cards — no absent-vs-empty ambiguity).
 *
 *  M8: adds no replicated property, no new replicated class, no new relevancy
 *  tier, no RPC — everything here is client-local USaveGame state.
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

	// --- The ten fixed deck slots (TASK-670; DECK-§8 pinned signatures) ---------

	/** The fixed slot count — Jonathan's directive verbatim: ten decks (DECK-§1). */
	static constexpr int32 NumFixedDeckSlots = 10;

	/**
	 *  0-based slot index → the fixed deck name "deck1".."deck10" (lowercase,
	 *  character-for-character — DECK-§1 triple duty: bar label = save key =
	 *  cloud deck_name). The ONE site that composes "deck" + number. An
	 *  out-of-range index returns an EMPTY string (callers treat it as "no such
	 *  slot"; the SaveDeckAs empty-name refusal is the downstream backstop).
	 */
	static FString MakeFixedDeckName(int32 SlotIndex);            // 0-based -> "deck1".."deck10"

	/**
	 *  The inverse: a deck name → its 0-based fixed-slot index, matched
	 *  case-insensitively against the ten fixed names; INDEX_NONE for anything
	 *  that is not a fixed name ("deck0", "deck11", "deck01", "MyDeck", "", …).
	 *  Implemented as a compare against MakeFixedDeckName so the composition
	 *  stays in ONE place (DECK-§1).
	 */
	static int32   FindFixedDeckIndex(const FString& DeckName);   // case-insensitive; INDEX_NONE when not fixed

	/**
	 *  THE ONE MIGRATION IMPLEMENTATION (DECK-§2; defaults D1/D2). Rewrites Save
	 *  in place to the fixed ten-slot form:
	 *    (1) legacy decks ALREADY bearing a fixed name (case-insensitive) keep
	 *        their slot (name canonicalized to lowercase — triple-duty law);
	 *    (2) the previously-ACTIVE legacy deck → the lowest empty slot
	 *        (normally deck1);
	 *    (3) remaining legacy decks fill remaining empty slots in SavedDecks
	 *        order;
	 *    (4) overflow beyond ten is DROPPED with one Warning listing the
	 *        dropped names;
	 *    (5) every still-empty slot materializes as an empty fixed-name deck
	 *        (D1: decks 2-10 start EMPTY, never copies of deck1);
	 *    (6) ActiveDeckName is rewritten to the fixed name its deck landed in —
	 *        "deck1" when there was no active deck (fresh-account default), the
	 *        name dangled, or the active deck itself was overflow-dropped.
	 *  Returns true iff it mutated (the CALLER persists — this function never
	 *  touches disk). IDEMPOTENT: a save already in fixed form returns false
	 *  and is byte-stable (proven in Tests/SiegeDeckSlotsTest.cpp). Shipping
	 *  caller: UDeckBuilderWidget::NativeConstruct ONLY (builder-open-time);
	 *  the match path never migrates (DECK-§2).
	 */
	static bool    MigrateToFixedSlots(USiegeDeckSaveGame& Save); // DECK-§2 mapping; true = mutated (caller persists)

	/** All named decks the player has saved. Keyed by FDeckList::DeckName (overwrite-on-collision, M6 ruling 2). */
	UPROPERTY(BlueprintReadWrite, Category = "Siegebound|Deck")
	TArray<FDeckList> SavedDecks;

	/** Name of the deck the next match uses; empty ⇒ no active deck ⇒ curated DeckCount fallback. */
	UPROPERTY(BlueprintReadWrite, Category = "Siegebound|Deck")
	FString ActiveDeckName;
};
