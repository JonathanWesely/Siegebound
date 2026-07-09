// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeckBuilderWidget.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckLibrary.h"
#include "Siegebound/SiegeDeckSaveGame.h"

UDeckBuilderWidget::UDeckBuilderWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// content contract (CONVENTIONS data-driven law + UCardHandWidget / UDeckComponent
	// precedent): /Game/Data/DT_Cards, soft and resolved null-safe at use time. Every
	// card stat (MaxCopies/Cost/DisplayName/DeckCount) is read from rows here, NEVER
	// hardcoded (§3.0) and never read in UMG.
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));
}

// ---------------------------------------------------------------------------
// Deck editing
// ---------------------------------------------------------------------------

void UDeckBuilderWidget::AddCopy(FName CardID)
{
	if (CardID.IsNone())
	{
		return;
	}

	// data-driven cap (§3.0): resolve MaxCopies from DT_Cards. A missing table/row
	// means we cannot validate the cap — refuse rather than build an illegal deck
	// (ResolveCardRow logs the fault once).
	const FCardRow* Row = ResolveCardRow(CardID);
	if (!Row)
	{
		return;
	}

	const int32 Current = GetCountOf(CardID);
	if (Current >= Row->MaxCopies)
	{
		// at the cap — refuse silently (the WBP greys the "+"; this is the
		// authoritative backstop). Never broadcast on a refused mutation
		// (CONVENTIONS delegate law).
		return;
	}

	const int32 NewCount = Current + 1;
	const int32 Index = IndexOfCard(CardID);
	if (Index == INDEX_NONE)
	{
		FDeckCardEntry NewEntry;
		NewEntry.CardID = CardID;
		NewEntry.Count = NewCount;
		WorkingDeck.Cards.Add(NewEntry);
	}
	else
	{
		WorkingDeck.Cards[Index].Count = NewCount;
	}

	OnDeckSlotCountChanged(CardID.ToString(), NewCount);
	OnDeckModelChanged();
}

void UDeckBuilderWidget::RemoveCopy(FName CardID)
{
	if (CardID.IsNone())
	{
		return;
	}

	const int32 Index = IndexOfCard(CardID);
	if (Index == INDEX_NONE || WorkingDeck.Cards[Index].Count <= 0)
	{
		// nothing to remove — no state change, no broadcast
		return;
	}

	const int32 NewCount = WorkingDeck.Cards[Index].Count - 1;
	if (NewCount <= 0)
	{
		// drop the entry so the model stays canonical (one entry per held card)
		WorkingDeck.Cards.RemoveAt(Index);
	}
	else
	{
		WorkingDeck.Cards[Index].Count = NewCount;
	}

	OnDeckSlotCountChanged(CardID.ToString(), NewCount);
	OnDeckModelChanged();
}

void UDeckBuilderWidget::LoadDefaultDeck()
{
	// "reset to default" template (M6 ruling 3): seed from the curated DeckCount
	// column, mirroring UDeckComponent::BuildAndShuffle's row walk. cards.csv stays
	// the single source of truth (§3.0) — no deck is hardcoded here.
	WorkingDeck.Cards.Reset();
	WorkingDeck.DeckName.Reset(); // an unsaved working deck until SaveDeckAs names it

	if (const UDataTable* Table = ResolveCardTable())
	{
		Table->ForeachRow<FCardRow>(TEXT("UDeckBuilderWidget::LoadDefaultDeck"),
			[this](const FName& CardID, const FCardRow& Row)
			{
				if (Row.DeckCount > 0)
				{
					FDeckCardEntry Entry;
					Entry.CardID = CardID;
					Entry.Count = Row.DeckCount;
					WorkingDeck.Cards.Add(Entry);
				}
			});

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("UDeckBuilderWidget: seeded working deck from the curated DeckCount default — %d cards across %d entries."),
			WorkingDeck.TotalCount(), WorkingDeck.Cards.Num());
	}
	// missing table ⇒ ResolveCardTable logged once; the working deck stays empty.

	OnDeckModelChanged();
}

// ---------------------------------------------------------------------------
// Deck model reads
// ---------------------------------------------------------------------------

int32 UDeckBuilderWidget::GetCountOf(FName CardID) const
{
	const int32 Index = IndexOfCard(CardID);
	return Index == INDEX_NONE ? 0 : WorkingDeck.Cards[Index].Count;
}

int32 UDeckBuilderWidget::GetTotalCount() const
{
	return WorkingDeck.TotalCount();
}

float UDeckBuilderWidget::GetAverageCost() const
{
	// the ONE average-cost home (§8 guide) — never duplicated here
	return UDeckLibrary::GetDeckAverageCost(ResolveCardTable(), WorkingDeck);
}

bool UDeckBuilderWidget::IsCurrentDeckLegal() const
{
	// the ONE legality home (§3.4) — the reason string is surfaced by the WBP if
	// it wants it; the gate itself only needs the bool
	FString Reason;
	return UDeckLibrary::IsDeckLegal(ResolveCardTable(), WorkingDeck, Reason);
}

TArray<FName> UDeckBuilderWidget::GetCollectionCardIDs() const
{
	TArray<FName> Result;
	if (const UDataTable* Table = ResolveCardTable())
	{
		// DT_Cards row-map order (import/CSV order); the browser grid renders them
		// in this order
		Result = Table->GetRowNames();
	}
	return Result;
}

// ---------------------------------------------------------------------------
// Per-card display resolvers
// ---------------------------------------------------------------------------

FString UDeckBuilderWidget::GetCardDisplayName(FName CardID) const
{
	if (const FCardRow* Row = ResolveCardRow(CardID))
	{
		// fall back to the raw CardID so an empty DisplayName never renders a blank cell
		return Row->DisplayName.IsEmpty() ? CardID.ToString() : Row->DisplayName;
	}
	return CardID.ToString();
}

int32 UDeckBuilderWidget::GetCardCost(FName CardID) const
{
	const FCardRow* Row = ResolveCardRow(CardID);
	return Row ? Row->Cost : 0;
}

int32 UDeckBuilderWidget::GetCardMaxCopies(FName CardID) const
{
	const FCardRow* Row = ResolveCardRow(CardID);
	return Row ? Row->MaxCopies : 0;
}

UTexture2D* UDeckBuilderWidget::GetCardArtTexture(FName CardID)
{
	if (CardID.IsNone())
	{
		return nullptr;
	}

	const FCardRow* Row = ResolveCardRow(CardID);
	if (!Row)
	{
		// missing table/row — already logged once by ResolveCardRow; text-only cell
		return nullptr;
	}

	if (Row->CardArt.IsNull())
	{
		// unset CardArt — graceful text-only fallback, logged once per CardID
		if (!WarnedCardArtIDs.Contains(CardID))
		{
			WarnedCardArtIDs.Add(CardID);
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UDeckBuilderWidget: DT_Cards row '%s' has no CardArt set — browser cell stays text-only (logged once per CardID)."),
				*CardID.ToString());
		}
		return nullptr;
	}

	// LoadSynchronous accepted for these 512x512 UI textures (TASK-079 ruling 4)
	UTexture2D* ArtTexture = Row->CardArt.LoadSynchronous();
	if (!ArtTexture)
	{
		if (!WarnedCardArtIDs.Contains(CardID))
		{
			WarnedCardArtIDs.Add(CardID);
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UDeckBuilderWidget: CardArt '%s' for CardID '%s' failed to load — browser cell stays text-only (logged once per CardID)."),
				*Row->CardArt.ToString(), *CardID.ToString());
		}
		return nullptr;
	}

	return ArtTexture;
}

// ---------------------------------------------------------------------------
// Saved decks (SaveGame)
// ---------------------------------------------------------------------------

void UDeckBuilderWidget::SaveDeckAs(const FString& Name)
{
	const FString Trimmed = Name.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SaveDeckAs: empty deck name — refused. Name the deck before saving."));
		return;
	}

	USiegeDeckSaveGame* SaveObj = LoadOrCreateSaveGame();
	if (!SaveObj)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SaveDeckAs: could not create the deck SaveGame — deck '%s' NOT saved."), *Trimmed);
		return;
	}

	// persist a copy of the working deck stamped with the entered name
	FDeckList ToSave = WorkingDeck;
	ToSave.DeckName = Trimmed;

	// overwrite-on-collision, case-insensitive (M6 ruling 2 — no silent duplicates)
	int32 Existing = INDEX_NONE;
	for (int32 DeckIndex = 0; DeckIndex < SaveObj->SavedDecks.Num(); ++DeckIndex)
	{
		if (SaveObj->SavedDecks[DeckIndex].DeckName.Equals(Trimmed, ESearchCase::IgnoreCase))
		{
			Existing = DeckIndex;
			break;
		}
	}

	if (Existing != INDEX_NONE)
	{
		SaveObj->SavedDecks[Existing] = ToSave;
	}
	else
	{
		SaveObj->SavedDecks.Add(ToSave);
	}

	// reflect the name onto the live working deck
	WorkingDeck.DeckName = Trimmed;

	if (!UGameplayStatics::SaveGameToSlot(SaveObj, USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SaveDeckAs: SaveGameToSlot('%s') failed — deck '%s' NOT persisted."),
			*USiegeDeckSaveGame::SlotName, *Trimmed);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("UDeckBuilderWidget: saved deck '%s' (%d cards) to slot '%s'."),
		*Trimmed, ToSave.TotalCount(), *USiegeDeckSaveGame::SlotName);

	OnDeckModelChanged();
}

void UDeckBuilderWidget::LoadDeck(const FString& Name)
{
	const USiegeDeckSaveGame* SaveObj = LoadSaveGame();
	if (!SaveObj)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::LoadDeck('%s'): no saved decks exist yet."), *Name);
		return;
	}

	for (const FDeckList& Deck : SaveObj->SavedDecks)
	{
		if (Deck.DeckName.Equals(Name, ESearchCase::IgnoreCase))
		{
			WorkingDeck = Deck;
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("UDeckBuilderWidget: loaded deck '%s' (%d cards) into the builder."),
				*Deck.DeckName, Deck.TotalCount());
			OnDeckModelChanged();
			return;
		}
	}

	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("UDeckBuilderWidget::LoadDeck('%s'): no saved deck with that name."), *Name);
}

TArray<FString> UDeckBuilderWidget::GetSavedDeckNames() const
{
	TArray<FString> Names;
	if (const USiegeDeckSaveGame* SaveObj = LoadSaveGame())
	{
		Names.Reserve(SaveObj->SavedDecks.Num());
		for (const FDeckList& Deck : SaveObj->SavedDecks)
		{
			Names.Add(Deck.DeckName);
		}
	}
	return Names;
}

void UDeckBuilderWidget::SetActiveDeck(const FString& Name)
{
	USiegeDeckSaveGame* SaveObj = LoadSaveGame();
	if (!SaveObj)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SetActiveDeck('%s'): no saved decks exist — save the deck first (SaveDeckAs)."), *Name);
		return;
	}

	// STRICT: only activate a deck that actually exists, and store its canonical
	// name, so ActiveDeckName never dangles (TASK-114 reads it, then legality-checks
	// and falls back to the DeckCount default null-safe if it ever fails to resolve)
	FString CanonicalName;
	for (const FDeckList& Deck : SaveObj->SavedDecks)
	{
		if (Deck.DeckName.Equals(Name, ESearchCase::IgnoreCase))
		{
			CanonicalName = Deck.DeckName;
			break;
		}
	}

	if (CanonicalName.IsEmpty())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SetActiveDeck('%s'): no saved deck with that name — save it first (SaveDeckAs)."), *Name);
		return;
	}

	SaveObj->ActiveDeckName = CanonicalName;

	if (!UGameplayStatics::SaveGameToSlot(SaveObj, USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SetActiveDeck('%s'): SaveGameToSlot failed — active deck NOT persisted."), *CanonicalName);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("UDeckBuilderWidget: active deck set to '%s' — the next match will use it."), *CanonicalName);

	OnDeckModelChanged();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int32 UDeckBuilderWidget::IndexOfCard(FName CardID) const
{
	for (int32 CardIndex = 0; CardIndex < WorkingDeck.Cards.Num(); ++CardIndex)
	{
		if (WorkingDeck.Cards[CardIndex].CardID == CardID)
		{
			return CardIndex;
		}
	}
	return INDEX_NONE;
}

const UDataTable* UDeckBuilderWidget::ResolveCardTable() const
{
	const UDataTable* Table = CardTableAsset.LoadSynchronous();
	if (!Table && !bWarnedMissingTable)
	{
		// logged ONCE per widget: the getters re-resolve the table on every model
		// re-read, so a per-call warning would spam the log
		bWarnedMissingTable = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget: card table '%s' not found — the collection shows empty and AddCopy refuses (logged once)."),
			*CardTableAsset.ToString());
	}
	return Table;
}

const FCardRow* UDeckBuilderWidget::ResolveCardRow(FName CardID) const
{
	const UDataTable* Table = ResolveCardTable();
	if (!Table)
	{
		return nullptr;
	}

	const FCardRow* Row = Table->FindRow<FCardRow>(CardID, TEXT("UDeckBuilderWidget::ResolveCardRow"), /*bWarnIfRowMissing=*/ false);
	if (!Row && !WarnedMissingRowIDs.Contains(CardID))
	{
		WarnedMissingRowIDs.Add(CardID);
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget: no DT_Cards row for CardID '%s' (logged once per CardID)."),
			*CardID.ToString());
	}
	return Row;
}

USiegeDeckSaveGame* UDeckBuilderWidget::LoadSaveGame() const
{
	// DoesSaveGameExist first so the normal first-run (no save yet) is SILENT —
	// LoadGameFromSlot on a missing slot would otherwise log an engine warning
	if (!UGameplayStatics::DoesSaveGameExist(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex))
	{
		return nullptr;
	}

	// null-check the cast (TASK-113 carry-forward): a corrupt/foreign slot casts to
	// nullptr rather than crashing
	return Cast<USiegeDeckSaveGame>(
		UGameplayStatics::LoadGameFromSlot(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex));
}

USiegeDeckSaveGame* UDeckBuilderWidget::LoadOrCreateSaveGame() const
{
	USiegeDeckSaveGame* SaveObj = LoadSaveGame();
	if (!SaveObj)
	{
		SaveObj = Cast<USiegeDeckSaveGame>(
			UGameplayStatics::CreateSaveGameObject(USiegeDeckSaveGame::StaticClass()));
	}
	return SaveObj;
}
