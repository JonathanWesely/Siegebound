// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/DeckTypes.h"
#include "DeckBuilderWidget.generated.h"

class UDataTable;
class UTexture2D;
class USiegeDeckSaveGame;
struct FCardRow;

/**
 *  C++ base for /Game/UI/WBP_DeckBuilder (TASK-118 reparents the UMG duplicate
 *  to this class) — the GDD §7 deck-builder screen: browse the 28-card pool,
 *  add/remove copies with per-card MaxCopies enforced, a live x/50 counter, the
 *  §8 average-cost readout, and save/load of NAMED decks (SaveGame).
 *
 *  Division of labor (CONVENTIONS "Deck-builder & saved decks (M6)" + the widget
 *  rule): this C++ base owns ALL model/logic; WBP_DeckBuilder is layout + calls.
 *  It NEVER reads DT_Cards, the SaveGame, or the deck itself — everything the
 *  grid needs (card names/costs/caps/art, the collection, the counters, the
 *  legality gate) arrives through the BlueprintCallable/Pure getters here
 *  (UObject/struct returns are allowed on these). The BlueprintImplementableEvents
 *  it fires carry float/int/bool/FString params ONLY (MCP cannot author enum/
 *  struct BP params) — the UCardHandWidget precedent.
 *
 *  Reuses the ONE deck infra (never reinvented): the working deck is an
 *  FDeckList; legality/average-cost come from UDeckLibrary (data-driven from
 *  DT_Cards, §3.0); saved decks persist through USiegeDeckSaveGame's fixed slot.
 *
 *  Seed-then-bind (qa/TASK-005-report.md major 2): WBP_DeckBuilder calls
 *  LoadDefaultDeck() (or LoadDeck) from Event Construct to SEED the working
 *  model, then reads the getters; every mutation re-broadcasts OnDeckModelChanged
 *  ("re-read the getters") so the screen stays correct without binding to any
 *  delegate. No gameplay/combat coupling — this only edits/persists FDeckLists.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UDeckBuilderWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDeckBuilderWidget(const FObjectInitializer& ObjectInitializer);

	// --- Deck editing (mutating) -------------------------------------------------

	/**
	 *  Add one copy of CardID to the working deck. REFUSED (no-op, no broadcast)
	 *  when the card is already at its DT_Cards MaxCopies (data-driven, §3.0 —
	 *  never hardcoded) or when the table/row cannot be resolved. On success
	 *  fires OnDeckSlotCountChanged(CardID, newCount) then OnDeckModelChanged().
	 *  The WBP greys the "+" at the cap; this is the authoritative backstop.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void AddCopy(FName CardID);

	/**
	 *  Remove one copy of CardID from the working deck (down to 0; the entry is
	 *  dropped when it hits 0). No-op (no broadcast) when the count is already 0.
	 *  On success fires OnDeckSlotCountChanged(CardID, newCount) then
	 *  OnDeckModelChanged(). Needs no table — removing is always legal.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void RemoveCopy(FName CardID);

	/**
	 *  Seed the working deck from the curated DeckCount default column of
	 *  DT_Cards (the "reset to default" template, M6 ruling 3). Clears the deck
	 *  name (an unsaved working deck). Null-safe: a missing table leaves the deck
	 *  empty (logged once). Fires OnDeckModelChanged().
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void LoadDefaultDeck();

	// --- Deck model reads (const, cheap — DT_Cards is cached after first load) ---

	/** Copies of CardID currently in the working deck (0 if absent). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetCountOf(FName CardID) const;

	/** Total cards in the working deck (the x/50 counter numerator). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetTotalCount() const;

	/** §8 average gold cost of the working deck (display-only guide) via UDeckLibrary. 0 for an empty deck. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	float GetAverageCost() const;

	/** True iff the working deck is a legal 50-card, cap-respecting deck via UDeckLibrary::IsDeckLegal (gates "Play with this deck"). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	bool IsCurrentDeckLegal() const;

	/** All DT_Cards row names (the 28-card collection) so the WBP builds the browser grid. Empty when the table is missing. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	TArray<FName> GetCollectionCardIDs() const;

	// --- Per-card display resolvers (additive; keep the WBP out of DT_Cards) -----
	// The grid needs each card's name/cost/cap/art to render a cell and grey the
	// "+" at the cap. Per "C++ base owns ALL logic" + the UCardHandWidget rule
	// (the WBP never reads DT_Cards), these are resolved here, null-safe.

	/** DT_Cards DisplayName for CardID; falls back to the raw CardID string when the row is missing. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FString GetCardDisplayName(FName CardID) const;

	/** DT_Cards gold Cost for CardID (0 when the row is missing). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetCardCost(FName CardID) const;

	/** DT_Cards MaxCopies for CardID (0 when the row is missing) — the WBP greys the "+" when GetCountOf >= this. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetCardMaxCopies(FName CardID) const;

	/**
	 *  Null-safe card-art resolver (CONVENTIONS "Card artwork (hand UI)"): CardID
	 *  → DT_Cards row → CardArt soft path → loaded UTexture2D. Returns nullptr
	 *  (hide the art, text-only cell) for an empty/unknown CardID, an unset
	 *  CardArt cell, or an unresolvable path (logged once per CardID). Uses
	 *  LoadSynchronous — accepted for these 512x512 UI textures (TASK-079 ruling
	 *  4). BlueprintCallable (not Pure) because it loads: call it once per cell.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	UTexture2D* GetCardArtTexture(FName CardID);

	// --- Saved decks (SaveGame, null-safe) --------------------------------------

	/**
	 *  Save the working deck under Name into USiegeDeckSaveGame (fixed slot,
	 *  overwrite-on-collision by name, case-insensitive — M6 ruling 2). An empty/
	 *  whitespace name is refused (logged). No legality gate here — the §8 guide
	 *  never blocks saving; "Play with this deck" is the only 50-card gate. Fires
	 *  OnDeckModelChanged() on success (the saved-names list changed).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SaveDeckAs(const FString& Name);

	/**
	 *  Load a saved deck by Name into the working deck (case-insensitive match).
	 *  No-op + warn when no SaveGame exists or no deck matches. Fires
	 *  OnDeckModelChanged() on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void LoadDeck(const FString& Name);

	/**
	 *  Names of every saved deck (for the WBP's load list). Empty when no
	 *  SaveGame exists. BlueprintCallable (not Pure) because it reads from disk —
	 *  the WBP should call it once and cache, not re-evaluate a pure node.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	TArray<FString> GetSavedDeckNames() const;

	/**
	 *  Mark a SAVED deck (by Name) as the active deck the next match uses
	 *  (persisted in USiegeDeckSaveGame::ActiveDeckName; TASK-114 reads it).
	 *  STRICT: only activates a deck that actually exists in the SaveGame, so
	 *  ActiveDeckName never dangles — persist the working deck with SaveDeckAs
	 *  FIRST, then SetActiveDeck. No-op + warn when the name has no saved match.
	 *  Fires OnDeckModelChanged() on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SetActiveDeck(const FString& Name);

	// --- Widget events (float/int/bool/FString params ONLY — the widget rule) ---

	/**
	 *  "The deck model changed — re-read the getters." Fired after any mutation
	 *  (add/remove/load/save/activate). WBP_DeckBuilder re-reads GetTotalCount
	 *  (x/50), GetAverageCost (§8), IsCurrentDeckLegal (Play gate), and each
	 *  cell's GetCountOf; it may re-read GetSavedDeckNames for the load list.
	 *  Sufficient on its own for a full refresh (seed-then-bind law).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
	void OnDeckModelChanged();

	/**
	 *  Fine-grained per-card signal (fired alongside OnDeckModelChanged on a
	 *  single add/remove) so the WBP can update just that card cell's counter
	 *  without re-reading all 28. CardID = the DT_Cards row name as a string;
	 *  Count = its new copy count in the working deck.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
	void OnDeckSlotCountChanged(const FString& CardID, int32 Count);

protected:

	/**
	 *  Card stat table (GDD §3.0) — /Game/Data/DT_Cards, the source for MaxCopies/
	 *  Cost/DisplayName/CardArt/DeckCount and the collection row names. Soft,
	 *  resolved null-safe at use time (the UCardHandWidget / UDeckComponent
	 *  precedent). EditDefaultsOnly so a BP can retarget the table without code.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Deck")
	TSoftObjectPtr<UDataTable> CardTableAsset;

private:

	/** The deck currently being edited (in-memory model; not persisted until SaveDeckAs). */
	UPROPERTY(Transient)
	FDeckList WorkingDeck;

	/** Index of CardID in WorkingDeck.Cards, or INDEX_NONE. */
	int32 IndexOfCard(FName CardID) const;

	/** DT_Cards, loaded null-safe (missing ⇒ nullptr, logged once). */
	const UDataTable* ResolveCardTable() const;

	/** DT_Cards row for CardID, null-safe (missing table/row ⇒ nullptr, logged once per CardID). */
	const FCardRow* ResolveCardRow(FName CardID) const;

	/** Loads the deck SaveGame from the fixed slot; nullptr when no file exists yet (the normal first-run state — silent). */
	USiegeDeckSaveGame* LoadSaveGame() const;

	/** Loads the deck SaveGame, or creates a fresh one when none exists (for write paths). */
	USiegeDeckSaveGame* LoadOrCreateSaveGame() const;

	/** True after the missing-table warning was logged (once-per-widget spam guard). Mutable — read by const getters. */
	mutable bool bWarnedMissingTable = false;

	/** CardIDs whose missing DT_Cards row was already logged (once-per-CardID). Mutable — set by const ResolveCardRow. */
	mutable TSet<FName> WarnedMissingRowIDs;

	/** CardIDs whose unset/unresolvable CardArt was already logged (once-per-CardID, mirrors UCardHandWidget). */
	TSet<FName> WarnedCardArtIDs;
};
