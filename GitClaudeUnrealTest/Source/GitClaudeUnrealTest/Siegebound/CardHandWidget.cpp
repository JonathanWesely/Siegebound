// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CardHandWidget.h"

#include "Engine/DataTable.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckComponent.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"

UCardHandWidget::UCardHandWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// content contract (CONVENTIONS data-driven law): /Game/Data/DT_Cards,
	// soft and resolved null-safe at use time — Cost/DisplayName are read from
	// rows here and NEVER typed into UMG (TASK-029 spec)
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));
}

void UCardHandWidget::InitForController(ASiegePlayerController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::InitForController: null controller — hand UI left unbound."));
		return;
	}

	// Re-targeting (precedent: UCastleHealthBarWidget, TASK-018 flagged
	// decision 5, QA-passed): drop every binding on the previous sources so
	// this widget never receives two update streams.
	if (ObservedController && ObservedController != Controller)
	{
		UnbindObservedSources();
	}

	ObservedController = Controller;
	ObservedDeck = Controller->FindComponentByClass<UDeckComponent>();
	ObservedPlayerState = Controller->GetPlayerState<ASiegePlayerState>();

	if (!ObservedDeck)
	{
		// TASK-023 creates the "DeckComponent" subobject on the controller;
		// missing it is a wiring fault, not a normal state. The hand cannot
		// display without a deck — leave the UMG design-time state alone.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::InitForController: controller '%s' has no UDeckComponent (TASK-023 wiring) — hand slots and preview stay unseeded."),
			*GetNameSafe(Controller));
	}

	if (!ObservedPlayerState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::InitForController: controller '%s' has no ASiegePlayerState — every card renders unaffordable and no gold re-grey will arrive."),
			*GetNameSafe(Controller));
	}

	// SEED FIRST (qa/TASK-005-report.md major 2): push the current slots +
	// preview + affordability now, so the widget is correct even if no
	// broadcast ever arrives after binding. Works in ANY init order: seeded
	// before the deck's BuildAndShuffle, the slots read empty and the build's
	// own broadcasts (hand + forced preview, TASK-022 decision 7) refresh
	// them moments later.
	if (ObservedDeck)
	{
		RefreshAllHandSlots();
		PushNextCardPreview(ObservedDeck->PeekNextCardID());
	}

	// ...THEN bind. AddUniqueDynamic: a repeated InitForController on the
	// same sources can never double-bind (a double-bound widget would push
	// every update twice — and double the refusal messages).
	if (ObservedDeck)
	{
		ObservedDeck->OnDeckHandChanged.AddUniqueDynamic(this, &UCardHandWidget::HandleDeckHandChanged);
		ObservedDeck->OnDeckNextCardChanged.AddUniqueDynamic(this, &UCardHandWidget::HandleDeckNextCardChanged);
	}

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnGoldChanged.AddUniqueDynamic(this, &UCardHandWidget::HandleGoldChanged);
	}

	ObservedController->OnCardRefused.AddUniqueDynamic(this, &UCardHandWidget::HandleCardRefused);
}

void UCardHandWidget::RequestPlaySlot(int32 SlotIndex)
{
	if (!ObservedController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::RequestPlaySlot(%d): no controller bound — call InitForController first."), SlotIndex);
		return;
	}

	// pure pass-through (TASK-029 spec): the controller owns every refusal —
	// gold, empty slot, match ended — and reports it back through
	// OnCardRefused -> OnCardRefusedMessage
	ObservedController->PlayHandSlot(SlotIndex);
}

void UCardHandWidget::RequestDiscardSlot(int32 SlotIndex)
{
	if (!ObservedController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::RequestDiscardSlot(%d): no controller bound — call InitForController first."), SlotIndex);
		return;
	}

	// §3.6: the 1-gold charge and the 0-gold refusal are the controller's
	ObservedController->DiscardHandSlot(SlotIndex);
}

void UCardHandWidget::HandleDeckHandChanged()
{
	// FOnDeckHandChanged carries no payload (TASK-022 contract): coarse
	// refresh — re-pull ALL slots via the getters. This delegate is the
	// per-play/per-discard heartbeat; the preview delegate is value-filtered
	// and must not be used to infer draws.
	RefreshAllHandSlots();
}

void UCardHandWidget::HandleDeckNextCardChanged(FName NextCardID)
{
	// every preview broadcast is authoritative (TASK-022 flagged decision 7:
	// build/reset paths force-fire even on an unchanged value) — push it
	// unconditionally, no self-filtering here
	PushNextCardPreview(NextCardID);
}

void UCardHandWidget::HandleGoldChanged(int32 NewGold)
{
	// NewGold itself is deliberately unused: PushHandSlot recomputes
	// bAffordable through ASiegePlayerState::CanAfford so the seed pass, the
	// hand-change refresh, and this gold re-grey share ONE code path (the
	// broadcast fires after the mutation, so CanAfford already sees NewGold).
	// §3.5: gold dropping below a card's cost flips that slot's bAffordable
	// on this very broadcast.
	RefreshAllHandSlots();
}

void UCardHandWidget::HandleCardRefused(const FString& Reason)
{
	// 1:1 pass-through — exactly one OnCardRefusedMessage per refused action
	// (TASK-029 acceptance). The ~2 s show-then-hide lives in WBP_CardHand.
	OnCardRefusedMessage(Reason);
}

void UCardHandWidget::RefreshAllHandSlots()
{
	if (!ObservedDeck)
	{
		return;
	}

	// GetHandSize() (6, GDD §3.4) keeps this loop in sync with the model
	// instead of double-hardcoding the hand size (qa/TASK-022-report.md
	// ruling 4 — that is exactly what the getter exists for)
	const int32 SlotCount = ObservedDeck->GetHandSize();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		PushHandSlot(SlotIndex);
	}
}

void UCardHandWidget::PushHandSlot(int32 SlotIndex)
{
	if (!ObservedDeck)
	{
		return;
	}

	const FName CardID = ObservedDeck->GetHandCardID(SlotIndex);
	if (CardID.IsNone())
	{
		// empty slot — the NORMAL state in the qa/TASK-021-report.md WARN-2
		// window, never an error (TASK-022 contract). The BIE signal is the
		// EMPTY string: NAME_None.ToString() would be the literal "None",
		// which a BP could never distinguish from a card named None.
		OnHandSlotUpdated(SlotIndex, FString(), FString(), 0, false);
		return;
	}

	// display data comes from the DT_Cards row (GDD §3.0 — never typed into
	// UMG); a missing row degrades to the raw CardID at cost 0, greyed, so
	// the slot stays identifiable on screen while the log names the fault
	FString DisplayName = CardID.ToString();
	int32 Cost = 0;
	bool bAffordable = false;

	if (const FCardRow* Row = ResolveCardRow(CardID))
	{
		DisplayName = Row->DisplayName;
		Cost = Row->Cost;

		// §3.5 "greyed when unaffordable": affordability is cosmetic here —
		// the controller re-checks it authoritatively on RequestPlaySlot
		bAffordable = ObservedPlayerState && ObservedPlayerState->CanAfford(Row->Cost);
	}

	OnHandSlotUpdated(SlotIndex, CardID.ToString(), DisplayName, Cost, bAffordable);
}

void UCardHandWidget::PushNextCardPreview(FName NextCardID)
{
	if (NextCardID.IsNone())
	{
		// no next card anywhere (empty-deck window) — empty DisplayName tells
		// the BP to hide the preview slot
		OnNextCardUpdated(FString(), 0);
		return;
	}

	FString DisplayName = NextCardID.ToString();
	int32 Cost = 0;

	if (const FCardRow* Row = ResolveCardRow(NextCardID))
	{
		DisplayName = Row->DisplayName;
		Cost = Row->Cost;
	}

	OnNextCardUpdated(DisplayName, Cost);
}

void UCardHandWidget::UnbindObservedSources()
{
	if (ObservedDeck)
	{
		ObservedDeck->OnDeckHandChanged.RemoveDynamic(this, &UCardHandWidget::HandleDeckHandChanged);
		ObservedDeck->OnDeckNextCardChanged.RemoveDynamic(this, &UCardHandWidget::HandleDeckNextCardChanged);
	}

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnGoldChanged.RemoveDynamic(this, &UCardHandWidget::HandleGoldChanged);
	}

	if (ObservedController)
	{
		ObservedController->OnCardRefused.RemoveDynamic(this, &UCardHandWidget::HandleCardRefused);
	}

	ObservedDeck = nullptr;
	ObservedPlayerState = nullptr;
	ObservedController = nullptr;
}

const FCardRow* UCardHandWidget::ResolveCardRow(FName CardID)
{
	const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
	if (!CardTable)
	{
		// logged ONCE per widget: slots re-push on every gold change (~1/s
		// income tick), so a per-lookup warning would spam the log
		if (!bWarnedMissingTable)
		{
			bWarnedMissingTable = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UCardHandWidget: card table '%s' not found — slots render raw CardIDs at cost 0, greyed (logged once)."),
				*CardTableAsset.ToString());
		}
		return nullptr;
	}

	const FCardRow* Row = CardTable->FindRow<FCardRow>(CardID, TEXT("UCardHandWidget::ResolveCardRow"), /*bWarnIfRowMissing*/ false);
	if (!Row && !WarnedMissingRowIDs.Contains(CardID))
	{
		// once per unique CardID, same spam rationale as the table warning
		WarnedMissingRowIDs.Add(CardID);
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget: no DT_Cards row for CardID '%s' — slot renders the raw CardID at cost 0, greyed (logged once per CardID)."),
			*CardID.ToString());
	}

	return Row;
}
