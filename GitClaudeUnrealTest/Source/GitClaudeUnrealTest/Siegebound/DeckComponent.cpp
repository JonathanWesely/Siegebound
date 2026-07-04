// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeckComponent.h"

#include "Engine/DataTable.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/CardRow.h"

UDeckComponent::UDeckComponent()
{
	// pure data model — no per-frame work
	PrimaryComponentTick.bCanEverTick = false;

	// content contract (TASK-022 names block): /Game/Data/DT_Cards, soft and
	// resolved null-safe at build time (TASK-008 import; TASK-031 reimports it
	// with the DeckCount column populated)
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));
}

void UDeckComponent::BuildAndShuffle()
{
	// full restart — BuildAndShuffle (match start) and ResetDeck (Play Again,
	// §3.9) share this path, so any previous piles/hand are dropped first
	DrawPile.Reset();
	DiscardPile.Reset();
	Hand.Reset();

	// --- build: DeckCount copies of every row's CardID (GDD §3.4). Copy
	// counts live in DT_Cards, never hardcoded (GDD §3.0) ---
	if (const UDataTable* CardTable = CardTableAsset.LoadSynchronous())
	{
		int32 RowsContributing = 0;
		CardTable->ForeachRow<FCardRow>(TEXT("UDeckComponent::BuildAndShuffle"),
			[this, &RowsContributing](const FName& CardID, const FCardRow& Row)
			{
				if (Row.DeckCount < 0)
				{
					// defensive only — TASK-021's data has no negative counts
					UE_LOG(LogGitClaudeUnrealTest, Warning,
						TEXT("UDeckComponent on '%s': row '%s' has negative DeckCount %d — treated as 0."),
						*GetNameSafe(GetOwner()), *CardID.ToString(), Row.DeckCount);
					return;
				}

				if (Row.DeckCount > 0)
				{
					++RowsContributing;
					for (int32 Copy = 0; Copy < Row.DeckCount; ++Copy)
					{
						DrawPile.Add(CardID);
					}
				}
			});

		if (DrawPile.Num() != ExpectedDeckSize)
		{
			// spec: log an error but still proceed with what the table gives.
			// The 0-card case is the known pre-reimport window: the saved
			// DT_Cards carries DeckCount=0 on every row until TASK-031 reimports
			// Docs/Data/cards.csv (qa/TASK-021-report.md WARN-2).
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("UDeckComponent on '%s': DT_Cards DeckCounts built a %d-card deck, expected %d (GDD §3.4) — proceeding with what the table gives.%s"),
				*GetNameSafe(GetOwner()), DrawPile.Num(), ExpectedDeckSize,
				DrawPile.Num() == 0
					? TEXT(" Deck is EMPTY — every hand slot deals NAME_None (expected until the TASK-031 reimport, qa/TASK-021-report.md WARN-2).")
					: TEXT(""));
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("UDeckComponent on '%s': built a %d-card draw pile from %d card rows (GDD §3.4)."),
				*GetNameSafe(GetOwner()), DrawPile.Num(), RowsContributing);
		}
	}
	else
	{
		// spec: log-and-empty if missing — the hand still deals (all NAME_None)
		// and every play/discard refuses gracefully
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("UDeckComponent on '%s': card table '%s' not found — deck is EMPTY; the hand deals %d empty slots and every play/discard refuses gracefully."),
			*GetNameSafe(GetOwner()), *CardTableAsset.ToString(), HandSize);
	}

	// --- shuffle ---
	ShuffleDrawPile();

	// --- deal exactly HandSize cards into the hand (§3.4). A short/empty deck
	// leaves NAME_None slots; the hand ALWAYS has exactly HandSize elements ---
	Hand.Init(NAME_None, HandSize);
	for (int32 Slot = 0; Slot < HandSize; ++Slot)
	{
		Hand[Slot] = DrawNextCard();
	}

	// build/reset path: always broadcast both (CONVENTIONS delegate law).
	// Hand first, then preview, so a consumer reacting to the preview event
	// already sees the dealt hand through the getters.
	OnDeckHandChanged.Broadcast();
	BroadcastPreview(/*bForceBroadcast=*/ true);
}

FName UDeckComponent::GetHandCardID(int32 Slot) const
{
	if (Slot < 0 || Slot >= HandSize)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckComponent on '%s': GetHandCardID(%d) is out of range [0..%d] — returning None."),
			*GetNameSafe(GetOwner()), Slot, HandSize - 1);
		return NAME_None;
	}

	// before the first BuildAndShuffle the hand array is empty — every slot
	// reads as empty rather than asserting
	return Hand.IsValidIndex(Slot) ? Hand[Slot] : NAME_None;
}

FName UDeckComponent::PeekNextCardID() const
{
	// eager-reshuffle invariant (ReshuffleDiscardIntoDrawIfNeeded): the draw
	// pile is empty only when the discard pile is empty too, so the top of the
	// draw pile IS the actual next draw (§3.4 preview law)
	return DrawPile.Num() > 0 ? DrawPile.Last() : NAME_None;
}

bool UDeckComponent::ConfirmPlayFromHand(int32 Slot)
{
	return MoveHandCardToDiscardAndRedraw(Slot, TEXT("ConfirmPlayFromHand"));
}

bool UDeckComponent::DiscardFromHand(int32 Slot)
{
	// identical pile movement to a play (§3.6) — the 1-gold charge already
	// happened controller-side (TASK-023)
	return MoveHandCardToDiscardAndRedraw(Slot, TEXT("DiscardFromHand"));
}

void UDeckComponent::ResetDeck()
{
	// Play Again (§3.9): full rebuild + reshuffle + redeal. Re-reads DT_Cards
	// so a reimported table (TASK-031) takes effect without restarting PIE.
	BuildAndShuffle();
}

bool UDeckComponent::MoveHandCardToDiscardAndRedraw(int32 Slot, const TCHAR* Verb)
{
	if (Slot < 0 || Slot >= HandSize)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckComponent on '%s': %s(%d) refused — slot out of range [0..%d]."),
			*GetNameSafe(GetOwner()), Verb, Slot, HandSize - 1);
		return false;
	}

	if (!Hand.IsValidIndex(Slot) || Hand[Slot].IsNone())
	{
		// expected while the deck is empty (qa/TASK-021-report.md WARN-2 window)
		// or before the first BuildAndShuffle — refuse without state change and
		// WITHOUT broadcasting (CONVENTIONS: never broadcast refused mutations)
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckComponent on '%s': %s(%d) refused — hand slot is empty."),
			*GetNameSafe(GetOwner()), Verb, Slot);
		return false;
	}

	// §3.4: the played/discarded card goes to the discard pile FIRST so it
	// participates in any reshuffle its own replacement draw triggers (only
	// observable with degenerate < hand-size decks, but it is the physical
	// card rule — the card is face-up in the discard before you draw)
	DiscardPile.Add(Hand[Slot]);
	Hand[Slot] = DrawNextCard();

	// exactly one hand broadcast per successful §3.4 mutation; the preview
	// broadcast is value-filtered (drawing into an identical next CardID fires
	// nothing — the preview VALUE did not change)
	OnDeckHandChanged.Broadcast();
	BroadcastPreview(/*bForceBroadcast=*/ false);
	return true;
}

FName UDeckComponent::DrawNextCard()
{
	ReshuffleDiscardIntoDrawIfNeeded();

	if (DrawPile.Num() == 0)
	{
		// nothing anywhere (empty-table window / degenerate deck): the slot
		// becomes NAME_None — "hand always has 6 slots" holds, the slot is empty
		return NAME_None;
	}

	const FName Drawn = DrawPile.Pop();

	// eager §3.4 reshuffle AFTER the pop too: if that was the last draw-pile
	// card, fold the discard in NOW so PeekNextCardID keeps telling the truth
	ReshuffleDiscardIntoDrawIfNeeded();

	return Drawn;
}

void UDeckComponent::ReshuffleDiscardIntoDrawIfNeeded()
{
	if (DrawPile.Num() > 0 || DiscardPile.Num() == 0)
	{
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("UDeckComponent on '%s': draw pile empty — shuffling %d discarded cards into a new draw pile (GDD §3.4)."),
		*GetNameSafe(GetOwner()), DiscardPile.Num());

	DrawPile = MoveTemp(DiscardPile);
	DiscardPile.Reset();
	ShuffleDrawPile();
}

void UDeckComponent::ShuffleDrawPile()
{
	// Fisher-Yates; FMath::RandRange is inclusive on both ends
	for (int32 Index = DrawPile.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		if (SwapIndex != Index)
		{
			DrawPile.Swap(Index, SwapIndex);
		}
	}
}

void UDeckComponent::BroadcastPreview(bool bForceBroadcast)
{
	const FName NextCard = PeekNextCardID();
	if (bForceBroadcast || NextCard != LastPreviewCardID)
	{
		LastPreviewCardID = NextCard;
		OnDeckNextCardChanged.Broadcast(NextCard);
	}
}
