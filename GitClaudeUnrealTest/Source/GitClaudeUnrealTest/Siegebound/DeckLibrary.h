// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Siegebound/DeckTypes.h"
#include "DeckLibrary.generated.h"

class UDataTable;

/**
 *  Deck-rule math for Siegebound (GDD §3.4 legality / §8 average-cost guide) —
 *  the ONE home for these rules, shared by the deck-builder widget
 *  (UDeckBuilderWidget, TASK-116), the DeckComponent build path (TASK-114) and
 *  the bot (TASK-114). Every rule is data-driven from /Game/Data/DT_Cards
 *  (FCardRow.MaxCopies / .Cost), never hardcoded (§3.0). Pure, stateless,
 *  null-safe: a null table or a missing row degrades gracefully — it never
 *  crashes.
 *
 *  C++-only entries (deliberately NOT BlueprintCallable, mirroring
 *  USpellLibrary): both static functions take a raw UDataTable* the widget/
 *  controllers already hold, and the widget exposes its own BlueprintPure
 *  wrappers (GetAverageCost / IsCurrentDeckLegal) over these — a raw table pin
 *  on a static library node invites BP misuse.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UDeckLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  True iff Deck is a legal Siegebound deck against CardTable (DT_Cards):
	 *  every entry's CardID resolves to a row, the running per-CardID copy total
	 *  stays within that row's MaxCopies, and TotalCount() == SiegeLegalDeckSize
	 *  (50, GDD §3.4). OutReason carries the FIRST violation (entry order) for a
	 *  HUD/log line, and is cleared on success. A null table ⇒ false + reason.
	 *
	 *  Copy-cap note: the cap is enforced on the per-CardID AGGREGATE (a running
	 *  sum), so a deck that splits one card across duplicate entries cannot slip
	 *  past MaxCopies. For the normal one-entry-per-card deck the widget builds
	 *  this is identical to a per-entry check.
	 */
	static bool IsDeckLegal(const UDataTable* CardTable, const FDeckList& Deck, FString& OutReason);

	/**
	 *  Average gold cost of the deck (§8 guide — display only, never a hard
	 *  rule): sum(FCardRow.Cost × entry Count) / TotalCount(). Returns 0 for an
	 *  empty deck (or a null table); unresolvable CardIDs contribute 0 cost
	 *  (null-safe), the denominator stays TotalCount().
	 */
	static float GetDeckAverageCost(const UDataTable* CardTable, const FDeckList& Deck);
};
