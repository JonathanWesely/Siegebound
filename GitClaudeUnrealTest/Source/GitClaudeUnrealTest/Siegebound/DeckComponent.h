// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/DeckTypes.h" // FDeckList/FDeckCardEntry — complete type for the FDeckList PendingDeckList member (M6 TASK-114)
#include "DeckComponent.generated.h"

class UDataTable;

/**
 *  Broadcast whenever any hand slot changes: the initial deal
 *  (BuildAndShuffle / ResetDeck) and every successful ConfirmPlayFromHand /
 *  DiscardFromHand. Exactly ONE broadcast per mutating operation — a full
 *  6-card deal is one broadcast, not six. Carries no payload: consumers
 *  re-read all slots via GetHandCardID(0..GetHandSize()-1). Never fired for
 *  refused calls (out-of-range or empty slot — CONVENTIONS delegate law).
 *  UI consumers must seed from the getters FIRST, then bind (seed-then-bind,
 *  qa/TASK-005-report.md major 2); UCardHandWidget::InitForController
 *  (TASK-029) does exactly that.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeckHandChanged);

/**
 *  Broadcast when the next-card preview (GDD §3.4: the top of the draw pile)
 *  changes. Value-filtered: drawing a Footman off the top while the next card
 *  down is ALSO a Footman fires nothing — the preview VALUE did not change
 *  (CONVENTIONS "actual value change" law). Build/reset paths always fire.
 *  NextCardID is NAME_None when no next card exists anywhere (empty-deck
 *  window, qa/TASK-021-report.md WARN-2).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckNextCardChanged, FName, NextCardID);

/**
 *  Deck & hand model (GDD §3.4) — a draw pile, a discard pile, and a fixed
 *  6-slot hand of CardIDs (DT_Cards row names). Lives on ASiegePlayerController
 *  as default subobject "DeckComponent" (TASK-023), which calls BuildAndShuffle
 *  at match start and ResetDeck on Play Again.
 *
 *  Pure card-flow model — NO gold logic here: the controller owns spend/refund
 *  (play costs §3.5, the 1-gold discard charge §3.6) and resolves gold BEFORE
 *  calling ConfirmPlayFromHand / DiscardFromHand.
 *
 *  - BuildAndShuffle reads EVERY row of /Game/Data/DT_Cards (soft path,
 *    runtime load) and adds DeckCount copies of each row's CardID to the draw
 *    pile (§3.4 default deck — data-driven, never hardcoded, §3.0); logs an
 *    error if the total != ExpectedDeckSize (50) but proceeds with what the
 *    table gives; a missing table = log + EMPTY deck; then shuffles and deals
 *    exactly HandSize cards into the hand.
 *  - Empty slots are NAME_None. Until TASK-031 reimports cards.csv the saved
 *    DT_Cards carries DeckCount=0 on every row (qa/TASK-021-report.md WARN-2),
 *    so the deck builds empty and all 6 slots deal NAME_None — every call
 *    no-ops gracefully, nothing crashes.
 *  - Playing/discarding moves the slot's card to the discard pile and
 *    IMMEDIATELY draws its replacement into the same slot (§3.4).
 *  - Reshuffle is EAGER: the moment the draw pile empties while the discard
 *    pile has cards, the discard is shuffled into a new draw pile. Invariant:
 *    the draw pile is empty only when the discard pile is empty too — which is
 *    what lets the const PeekNextCardID() ALWAYS equal the actual next draw
 *    (§3.4 preview law) without mutating or lying.
 *  - Top of the draw pile = LAST array element (O(1) draw).
 *
 *  Acceptance mapping (§3.4): the hand always has exactly HandSize (6) slots;
 *  ConfirmPlayFromHand draws the replacement immediately; the preview always
 *  equals the actual next draw; after 50 plays/discards the deck has
 *  reshuffled and keeps dealing indefinitely; the built deck contains exactly
 *  the DeckCount copy counts from DT_Cards (12/10/10/8/6/4 per TASK-021).
 */
UCLASS(ClassGroup=(Siegebound), meta=(BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API UDeckComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UDeckComponent();

	/** Fired once per hand mutation (deal, play, discard, reset). Consumers re-read GetHandCardID(0..5). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Deck")
	FOnDeckHandChanged OnDeckHandChanged;

	/** Fired when the §3.4 next-card preview actually changes (and always on build/reset paths). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Deck")
	FOnDeckNextCardChanged OnDeckNextCardChanged;

	/**
	 *  Builds the draw pile from /Game/Data/DT_Cards (DeckCount copies of each
	 *  row's CardID, §3.4), shuffles it, and deals exactly HandSize cards into
	 *  the hand. Logs an error when the built total != ExpectedDeckSize (50)
	 *  and proceeds with what the table gives — an empty or missing table
	 *  yields an empty deck and a hand of NAME_None slots (qa/TASK-021-report.md
	 *  WARN-2 window), never a crash. Clears any previous piles/hand first, so
	 *  a repeat call is a full restart. Always broadcasts OnDeckHandChanged and
	 *  OnDeckNextCardChanged (build/reset path). Called by ASiegePlayerController
	 *  at match start (TASK-023) — NOT self-triggered at BeginPlay.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void BuildAndShuffle();

	/**
	 *  CardID in the given hand slot (0..HandSize-1). NAME_None = empty slot
	 *  (empty/short deck, or before the first BuildAndShuffle) — TASK-023 must
	 *  refuse plays/discards on NAME_None slots. Out-of-range slots log a
	 *  warning and return NAME_None.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FName GetHandCardID(int32 Slot) const;

	/**
	 *  Top of the draw pile — the §3.4 next-card preview. ALWAYS the actual
	 *  next draw thanks to the eager-reshuffle invariant (an empty draw pile
	 *  means the discard pile is empty too). NAME_None when no next card
	 *  exists anywhere.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FName PeekNextCardID() const;

	/**
	 *  Confirmed card play (the card leaves the hand at CONFIRM — M2 ruling):
	 *  moves the slot's card to the discard pile and immediately draws its
	 *  replacement into the same slot (§3.4). Gold was already resolved by the
	 *  controller (§3.5 — no gold logic here). Returns false (log, no state
	 *  change, no broadcast) for an out-of-range or empty slot; callers may
	 *  ignore the return.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	bool ConfirmPlayFromHand(int32 Slot);

	/**
	 *  Discard (§3.6): identical pile movement to ConfirmPlayFromHand — the
	 *  1-gold charge is the controller's job (TASK-023) and must be resolved
	 *  BEFORE calling this. Returns false (log, no state change, no broadcast)
	 *  for an out-of-range or empty slot; callers may ignore the return.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	bool DiscardFromHand(int32 Slot);

	/** Cards remaining in the draw pile. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetDrawPileCount() const { return DrawPile.Num(); }

	/** Cards in the discard pile (drops to 0 the moment an eager reshuffle folds it back into the draw pile). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetDiscardPileCount() const { return DiscardPile.Num(); }

	/** Number of hand slots (6, GDD §3.4). TASK-029's widget iterates slots [0..GetHandSize()-1]. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetHandSize() const { return HandSize; }

	/**
	 *  Play Again (§3.9): full rebuild + reshuffle + redeal via BuildAndShuffle.
	 *  Re-reads DT_Cards, so a reimported table (TASK-031) takes effect on the
	 *  next reset without restarting PIE.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void ResetDeck();

	/**
	 *  Sets a guarded PENDING override deck (M6, TASK-114 — additive, backward-
	 *  compatible). The NEXT BuildAndShuffle (and every later ResetDeck / Play
	 *  Again) builds the draw pile from THIS list (Count copies of each CardID)
	 *  INSTEAD of the DeckCount column — but ONLY when the list passes
	 *  UDeckLibrary::IsDeckLegal against DT_Cards. An unset OR illegal pending list
	 *  falls back to the existing DeckCount build UNCHANGED (empty/unset/illegal =
	 *  today's exact path, so nothing breaks). The pending list PERSISTS across
	 *  ResetDeck (never cleared on reset), so the same match keeps the same deck.
	 *  Call BEFORE BuildAndShuffle — the player controller (from the active saved
	 *  deck) and the bot (from a random curated BotDecks entry) both do so.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SetPendingDeckList(const FDeckList& Deck);

protected:

	/** Card stat table (GDD §3.0/§3.4) — the DeckCount column drives the build. Soft, resolved null-safe at build time (TASK-008 import; TASK-031 reimports with DeckCount populated). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Deck")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/** Visible hand size — the player has a hand of 6 cards (GDD §3.4). Mechanic rule, not a CSV column (CONVENTIONS). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Deck", meta = (ClampMin = "1"))
	int32 HandSize = 6;

	/** Legal deck size — a deck is exactly 50 cards (GDD §3.4). BuildAndShuffle logs an error when the table's DeckCounts sum differently, then proceeds with what it got. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Deck", meta = (ClampMin = "1"))
	int32 ExpectedDeckSize = 50;

private:

	/** Shared play/discard pile movement (identical per §3.4/§3.6): slot card -> discard pile, immediate redraw into the same slot, one hand broadcast + value-filtered preview broadcast. */
	bool MoveHandCardToDiscardAndRedraw(int32 Slot, const TCHAR* Verb);

	/** Pops the top of the draw pile (folding the discard pile in first when needed); NAME_None when no card exists anywhere. Re-checks the eager-reshuffle invariant after the pop so the preview stays truthful. */
	FName DrawNextCard();

	/** Eager §3.4 reshuffle: when the draw pile is empty and the discard pile is not, shuffle the discard into a new draw pile NOW (this is what keeps PeekNextCardID always equal to the actual next draw). */
	void ReshuffleDiscardIntoDrawIfNeeded();

	/** In-place Fisher-Yates shuffle of the draw pile (FMath::RandRange). */
	void ShuffleDrawPile();

	/** Broadcasts OnDeckNextCardChanged when the preview value changed since the last broadcast (or unconditionally when forced on build/reset paths). */
	void BroadcastPreview(bool bForceBroadcast);

	/** Draw pile. TOP = LAST element (O(1) draw). Empty only when DiscardPile is empty too (eager-reshuffle invariant). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Deck", meta = (AllowPrivateAccess = "true"))
	TArray<FName> DrawPile;

	/** Discard pile — played and discarded cards land here until a reshuffle folds them back into the draw pile (§3.4). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Deck", meta = (AllowPrivateAccess = "true"))
	TArray<FName> DiscardPile;

	/** The hand. Exactly HandSize elements once dealt; NAME_None = empty slot. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Deck", meta = (AllowPrivateAccess = "true"))
	TArray<FName> Hand;

	/** Last preview value pushed through OnDeckNextCardChanged (the value filter). */
	FName LastPreviewCardID;

	/** True once SetPendingDeckList has provided an override deck (M6 TASK-114); until then BuildAndShuffle uses the DeckCount default. Persists across ResetDeck (never cleared on reset) so Play Again keeps the same deck. */
	bool bHasPendingDeckList = false;

	/** The pending override deck (M6 TASK-114). Consumed by BuildAndShuffle ONLY when bHasPendingDeckList AND UDeckLibrary::IsDeckLegal passes; otherwise the DeckCount build runs unchanged. Not a UPROPERTY — pure runtime state with no UObject refs (FName/FString/int32 only). */
	FDeckList PendingDeckList;
};
