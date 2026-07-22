// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeBotController.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/Building.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DeckComponent.h"
#include "Siegebound/DeckLibrary.h" // UDeckLibrary::IsDeckLegal / GetDeckAverageCost — validate + log the chosen curated bot deck (M6 TASK-114)
#include "Siegebound/GoldNode.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeGameMode.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SpellLibrary.h" // TASK-098 (same M5 parallel file wave) — pinned ResolveSpell signature per CONVENTIONS "Spells & Set III (M5)"; compiles at the TASK-103 batch
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/Tower.h"

DEFINE_LOG_CATEGORY(LogSiegeBot);

namespace
{
	/** One resolvable hand card the bot can reason about — empty slots and rows that fail to resolve are dropped before the §4 rules scan. */
	struct FBotHandCard
	{
		int32 Slot = INDEX_NONE;
		FName CardID = NAME_None;
		const FCardRow* Row = nullptr;
	};

	/** Unit or Building = a "defensive play" (a body to block / a tower to shoot); Economy/Instant/Spell are not. */
	bool IsDefensiveType(ECardType Type)
	{
		return Type == ECardType::Unit || Type == ECardType::Building;
	}

	/**
	 *  Card types the bot can NEVER play (GDD §4 M4 extension — the rule-5 discard
	 *  set): the bot controls no hero (HeroUpgrade), and neither spells nor
	 *  Instants (Utility/Masons) are actionable by an AI with no hero. These are the
	 *  ONLY cards rule 5 cycles. Unit/Building/Economy are playable TYPES and are never
	 *  discarded here — an UNAFFORDABLE Unit/Building/Economy is classified "unplayable
	 *  THIS tick" by the PLAY rules (rules 1-4 only ever select an affordable card — the
	 *  never-play-unaffordable invariant), but it is deliberately NOT a discard
	 *  candidate: the bot BANKS toward it (e.g. an Ogre needs 12 gold, §4), so cycling
	 *  it away would break the "growing Set II waves incl. Ogres" acceptance.
	 *
	 *  M5 (TASK-102): Spell stays in this TYPE-level set, but the bot's two CASTABLE
	 *  spells (Fireball/Lightning — rule 3, M5 ruling 10) are exempted BY CARDID in
	 *  FindMostExpensiveUnplayableCard: the bot HOLDS them awaiting a target (the same
	 *  bank-toward-it precedent as an unaffordable Ogre), while FrostNova/BattleCry/
	 *  Pickpocket — for which the GDD gives the bot NO cast rule — still fall through
	 *  to rule-5 discard economics (TASK-102 spec point 3).
	 */
	bool IsUnplayableByBot(ECardType Type)
	{
		return Type == ECardType::Spell || Type == ECardType::HeroUpgrade || Type == ECardType::Utility;
	}

	/**
	 *  Cheapest AFFORDABLE Unit/Building card; ties prefer a Unit (always
	 *  placeable castle-front, no clearance constraint). Returns the index
	 *  INTO HandCards (not the deck slot), or INDEX_NONE. bOutIsBuilding reports
	 *  the winner's family for the caller's spawn geometry.
	 */
	int32 FindCheapestDefensiveCard(const TArray<FBotHandCard>& HandCards, int32 Gold, bool& bOutIsBuilding)
	{
		int32 BestIndex = INDEX_NONE;
		for (int32 Index = 0; Index < HandCards.Num(); ++Index)
		{
			const FCardRow* Row = HandCards[Index].Row;
			if (!IsDefensiveType(Row->CardType) || Row->Cost > Gold)
			{
				continue;
			}
			if (BestIndex == INDEX_NONE)
			{
				BestIndex = Index;
				continue;
			}
			const FCardRow* Best = HandCards[BestIndex].Row;
			const bool bCheaper = Row->Cost < Best->Cost;
			const bool bTiePreferUnit = (Row->Cost == Best->Cost) &&
				(Best->CardType == ECardType::Building && Row->CardType == ECardType::Unit);
			if (bCheaper || bTiePreferUnit)
			{
				BestIndex = Index;
			}
		}
		if (BestIndex != INDEX_NONE)
		{
			bOutIsBuilding = (HandCards[BestIndex].Row->CardType == ECardType::Building);
		}
		return BestIndex;
	}

	/** Most-expensive AFFORDABLE Unit card (rule 4 — units only, not buildings). Index into HandCards, or INDEX_NONE. */
	int32 FindMostExpensiveUnitCard(const TArray<FBotHandCard>& HandCards, int32 Gold)
	{
		int32 BestIndex = INDEX_NONE;
		for (int32 Index = 0; Index < HandCards.Num(); ++Index)
		{
			const FCardRow* Row = HandCards[Index].Row;
			if (Row->CardType != ECardType::Unit || Row->Cost > Gold)
			{
				continue;
			}
			if (BestIndex == INDEX_NONE || Row->Cost > HandCards[BestIndex].Row->Cost)
			{
				BestIndex = Index;
			}
		}
		return BestIndex;
	}

	/**
	 *  First AFFORDABLE card in hand with the given row ID. Shared by rule 2 (Miner)
	 *  and rule 3a/3b (Fireball/Lightning — TASK-102 generalized the former
	 *  FindAffordableMinerCard, behavior byte-for-byte for rule 2). Index into
	 *  HandCards, or INDEX_NONE.
	 */
	int32 FindAffordableCardByID(const TArray<FBotHandCard>& HandCards, int32 Gold, FName RowID)
	{
		for (int32 Index = 0; Index < HandCards.Num(); ++Index)
		{
			if (HandCards[Index].CardID == RowID && HandCards[Index].Row->Cost <= Gold)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/**
	 *  First AFFORDABLE Economy card that is BUILDING-routed (Deep Mine — GDD §4 M4;
	 *  mirrors ASiegePlayerController::BuildingEconomyCardIDs / TASK-059). Index into
	 *  HandCards, or INDEX_NONE. No miner-cap interaction — Deep Mine is not a miner.
	 */
	int32 FindAffordableEconomyBuildingCard(const TArray<FBotHandCard>& HandCards, int32 Gold, const TArray<FName>& BuildingEconomyIDs)
	{
		for (int32 Index = 0; Index < HandCards.Num(); ++Index)
		{
			const FBotHandCard& Card = HandCards[Index];
			if (Card.Row->CardType == ECardType::Economy && Card.Row->Cost <= Gold && BuildingEconomyIDs.Contains(Card.CardID))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/**
	 *  Most-expensive card in the rule-5 discard set: an unplayable TYPE
	 *  (IsUnplayableByBot) that is NOT one of the bot's two castable spell CardIDs
	 *  (Fireball/Lightning — those are rule-3 PLAYS, held awaiting a target, never
	 *  cycled; M5 ruling 10 / TASK-102). Index into HandCards, or INDEX_NONE.
	 */
	int32 FindMostExpensiveUnplayableCard(const TArray<FBotHandCard>& HandCards, FName CastableFireballID, FName CastableLightningID)
	{
		int32 BestIndex = INDEX_NONE;
		for (int32 Index = 0; Index < HandCards.Num(); ++Index)
		{
			const FBotHandCard& Card = HandCards[Index];
			if (!IsUnplayableByBot(Card.Row->CardType))
			{
				continue;
			}
			if (Card.CardID == CastableFireballID || Card.CardID == CastableLightningID)
			{
				continue; // castable by rule 3 — hold it for a target, never discard
			}
			if (BestIndex == INDEX_NONE || Card.Row->Cost > HandCards[BestIndex].Row->Cost)
			{
				BestIndex = Index;
			}
		}
		return BestIndex;
	}
}

ASiegeBotController::ASiegeBotController()
{
	// §4 "controls no hero": this AIController possesses nothing. bWantsPlayerState
	// makes the engine auto-create a PlayerState of ASiegeGameMode::PlayerStateClass
	// (= ASiegePlayerState) for it in PostInitializeComponents — reusing the M2
	// economy verbatim (accrual, §3.2 overtime, §3.3 miner income) for the bot.
	bWantsPlayerState = true;

	// The bot never possesses a pawn, so its AI logic is never gated on possession
	// — the decision loop runs on a timer regardless (defensive: this flag governs
	// behavior-tree logic, which the bot does not use, but keeping it off avoids any
	// possess-driven start/stop that a future component pass might introduce).
	bStartAILogicOnPossess = false;

	// Deck & hand model (GDD §3.4, TASK-022) — subobject name is a spec contract.
	// The component self-defaults its CardTableAsset to /Game/Data/DT_Cards, so the
	// bot's deck builds with no extra wiring. It never self-builds; BeginPlay does.
	DeckComponent = CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"));

	// The decision loop (TASK-046) reads each hand card's Cost/CardType from the
	// SAME table (GDD §3.0 — never hardcodes a stat). Soft, resolved null-safe per
	// decision; matches ASiegePlayerController's CardTableAsset path (TASK-008).
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));

	// --- M6 (TASK-114 / ruling 4): TWO distinct legal curated bot decks as
	// EditDefaultsOnly defaults. Each is legal against DT_Cards — sum(Count)==50 and
	// every Count <= that card's MaxCopies (Footman 12, MilitiaMob/Pikeman/Knight 6,
	// Archer 10, Cavalry/Sapper/Miner/BombTower/BallistaTower 4, Ogre/DeepMine 2,
	// Barracks/CrystalTower/Cleric 3, Wall 10, ArrowTower 8, Longbowman 4,
	// Lightning 2). Both are composed ONLY of bot-PLAYABLE types — Unit/Building/
	// Economy plus the decision loop's rule-3 castable Spells (TASK-252 added
	// Lightning ×2 to [1] per the M6 QA recommendation, making rule 3b reachable
	// in curated play) — so the bot never wastes a decision cycling an unplayable
	// card, and both differ from the player's TASK-115 curated DeckCount default.
	// Legality is re-checked at pick time (IsDeckLegal) — an edited-illegal BP
	// entry degrades to the DeckCount fallback. ---
	auto Entry = [](const TCHAR* InCardID, int32 InCount)
	{
		FDeckCardEntry Result;
		Result.CardID = FName(InCardID);
		Result.Count = InCount;
		return Result;
	};

	// [0] AGGRO RUSH — cheap, fast, unit-heavy pressure (avg cost ~4.72). Sum 50.
	FDeckList AggroDeck;
	AggroDeck.DeckName = TEXT("Bot Aggro Rush");
	AggroDeck.Cards =
	{
		Entry(TEXT("Footman"),    12), // 3 x12 = 36
		Entry(TEXT("MilitiaMob"),  6), // 5 x6  = 30 (swarm: 4 bodies per copy)
		Entry(TEXT("Pikeman"),     6), // 5 x6  = 30
		Entry(TEXT("Knight"),      6), // 6 x6  = 36
		Entry(TEXT("Archer"),      6), // 4 x6  = 24
		Entry(TEXT("Cavalry"),     4), // 7 x4  = 28 (charge)
		Entry(TEXT("Sapper"),      4), // 5 x4  = 20 (siege suicide)
		Entry(TEXT("Wall"),        4), // 4 x4  = 16
		Entry(TEXT("Miner"),       2), // 8 x2  = 16 (minimal economy)
	};

	// [1] DEFENSIVE ECONOMY — towers, walls, full economy, heavy finishers (avg cost ~7.02). Sum 50.
	FDeckList FortressDeck;
	FortressDeck.DeckName = TEXT("Bot Defensive Economy");
	FortressDeck.Cards =
	{
		Entry(TEXT("Wall"),           8), // 4 x8  = 32 (TASK-252: 10 → 8, donor for Lightning ×2)
		Entry(TEXT("ArrowTower"),     8), // 5 x8  = 40
		Entry(TEXT("Knight"),         6), // 6 x6  = 36
		Entry(TEXT("BombTower"),      4), // 8 x4  = 32
		Entry(TEXT("BallistaTower"),  4), // 7 x4  = 28
		Entry(TEXT("Miner"),          4), // 8 x4  = 32 (full economy)
		Entry(TEXT("Barracks"),       3), // 10 x3 = 30 (Footman spawner)
		Entry(TEXT("CrystalTower"),   3), // 9 x3  = 27 (chain tower)
		Entry(TEXT("Cleric"),         3), // 6 x3  = 18 (heals)
		Entry(TEXT("Ogre"),           2), // 12 x2 = 24 (siege finisher)
		Entry(TEXT("DeepMine"),       2), // 15 x2 = 30 (raidable economy)
		Entry(TEXT("Lightning"),      2), // 8 x2  = 16 (spell — rule-3b tower-killer; TASK-252 per the M6 QA rec)
		Entry(TEXT("Longbowman"),     1), // 6 x1  = 6
	};

	BotDecks = { AggroDeck, FortressDeck };
}

void ASiegeBotController::BeginPlay()
{
	Super::BeginPlay();

	// Build the bot's deck + deal its hand of 6 (GDD §3.4) at match start. The
	// component never self-builds (TASK-022 flagged decision 12) — the controller
	// owns the timing, exactly as ASiegePlayerController does for the player.
	if (DeckComponent)
	{
		// M6 (TASK-114 / ruling 4): pick ONE curated bot deck at RANDOM and push it
		// as the pending override BEFORE the build. Data-driven legality gate
		// (UDeckLibrary::IsDeckLegal against DT_Cards); a missing/illegal pick leaves
		// the component unset so BuildAndShuffle uses the curated DeckCount default
		// (null-safe). RandRange over [0..Num-1] (not a literal 0..1) so a BP that
		// edits BotDecks to any size can never index out of range. Exactly ONE
		// grep-able LogSiegeBot line records the choice (like the §4 decision trace).
		if (BotDecks.Num() > 0)
		{
			const int32 DeckIndex = FMath::RandRange(0, BotDecks.Num() - 1);
			const FDeckList& ChosenDeck = BotDecks[DeckIndex];
			const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
			FString LegalityReason;
			if (UDeckLibrary::IsDeckLegal(CardTable, ChosenDeck, LegalityReason))
			{
				DeckComponent->SetPendingDeckList(ChosenDeck);
				UE_LOG(LogSiegeBot, Log,
					TEXT("[Bot %s] Deck select: chose curated deck %d of %d '%s' (%d cards, avg cost %.2f) — pushed as pending override."),
					*GetNameSafe(this), DeckIndex, BotDecks.Num(), *ChosenDeck.DeckName,
					ChosenDeck.TotalCount(), UDeckLibrary::GetDeckAverageCost(CardTable, ChosenDeck));
			}
			else
			{
				UE_LOG(LogSiegeBot, Log,
					TEXT("[Bot %s] Deck select: curated deck %d '%s' is illegal (%s) — falling back to the DeckCount default."),
					*GetNameSafe(this), DeckIndex, *ChosenDeck.DeckName,
					LegalityReason.IsEmpty() ? TEXT("no reason") : *LegalityReason);
			}
		}
		else
		{
			UE_LOG(LogSiegeBot, Log,
				TEXT("[Bot %s] Deck select: no BotDecks configured — using the curated DeckCount default."),
				*GetNameSafe(this));
		}

		DeckComponent->BuildAndShuffle();
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegeBotController '%s': DeckComponent subobject missing (TASK-022/045) — the bot has no deck or hand this match."),
			*GetNameSafe(this));
	}

	// Start the §4 decision cadence. EvaluateDecisions is a no-op in this shell
	// (TASK-045); TASK-046 fills it. ASiegeGameMode tags this controller's
	// PlayerState Team=Red right after spawning it — that identity is set before
	// any miner or decision needs it (no miner spawns during match-start BeginPlay).
	StartDecisionTimer();
}

void ASiegeBotController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Own-timer hygiene only — this class never touches other objects' timers.
	StopDecisionTimer();

	Super::EndPlay(EndPlayReason);
}

ASiegePlayerState* ASiegeBotController::GetBotPlayerState() const
{
	// GetPlayerState<T> reads the inherited PlayerState UPROPERTY without shadowing
	// it (CONVENTIONS C4458 law: no local named PlayerState). Null until the engine
	// creates it in PostInitializeComponents; valid by BeginPlay.
	return GetPlayerState<ASiegePlayerState>();
}

void ASiegeBotController::StartDecisionTimer()
{
	if (UWorld* World = GetWorld())
	{
		// SetTimer on the same handle replaces any existing timer, so repeated
		// calls (BeginPlay, then ResetBot on each Play Again) never stack.
		World->GetTimerManager().SetTimer(
			DecisionTimerHandle, this, &ASiegeBotController::EvaluateDecisions,
			DecisionIntervalSeconds, /*bLoop*/ true);
	}
}

void ASiegeBotController::StopDecisionTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DecisionTimerHandle);
	}
}

void ASiegeBotController::EvaluateDecisions()
{
	// (0) MATCH-ACTIVE GATE (TASK-045 forward-dep / GDD §3.9): never act under the
	// Victory screen. TASK-047 ALSO calls StopDecisionTimer() in the match-end
	// freeze — this is the bot-internal half of that belt-and-suspenders.
	if (!IsMatchActive())
	{
		return;
	}

	UDeckComponent* Deck = GetDeckComponent();
	ASiegePlayerState* BotState = GetBotPlayerState();
	if (!Deck || !BotState)
	{
		// Shell not fully wired (TASK-045 BeginPlay already logged a missing deck);
		// a null Red PlayerState is transient at match start — no-op, retry next tick.
		return;
	}

	const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
	if (!CardTable)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegeBotController '%s': DT_Cards ('%s') unavailable — bot cannot read card stats this tick (no play)."),
			*GetNameSafe(this), *CardTableAsset.ToString());
		return;
	}

	// Gather the resolvable hand ONCE (skip empty slots + rows that don't resolve —
	// e.g. the qa/TASK-021 WARN-2 empty-deck window before TASK-031 reimports).
	TArray<FBotHandCard> HandCards;
	const int32 HandSize = Deck->GetHandSize();
	HandCards.Reserve(HandSize);
	for (int32 SlotIndex = 0; SlotIndex < HandSize; ++SlotIndex)
	{
		const FName CardID = Deck->GetHandCardID(SlotIndex);
		if (CardID.IsNone())
		{
			continue;
		}
		const FCardRow* Row = CardTable->FindRow<FCardRow>(CardID, TEXT("ASiegeBotController::EvaluateDecisions"), /*bWarnIfRowMissing=*/ false);
		if (Row)
		{
			HandCards.Add(FBotHandCard{ SlotIndex, CardID, Row });
		}
	}

	if (HandCards.Num() == 0)
	{
		return; // empty / unresolved hand — nothing to decide this tick
	}

	const int32 Gold = BotState->GetGold();
	AActor* NearestIntruder = FindNearestEnemyIntruderOnBotHalf();

	// ================= §4 ordered rules — play the FIRST that fires =================

	// ---- Rule 1: DEFEND — enemy units on the bot half AND an affordable defensive play ----
	if (NearestIntruder)
	{
		bool bChosenIsBuilding = false;
		const int32 CardIndex = FindCheapestDefensiveCard(HandCards, Gold, bChosenIsBuilding);
		if (CardIndex != INDEX_NONE)
		{
			const FBotHandCard& Chosen = HandCards[CardIndex];

			// A tower goes BETWEEN the nearest intruder and Castle_Red; a unit
			// materializes CASTLE-FRONT (BotCastleSpawnOffset in front of Castle_Red
			// toward the centerline) and marches to meet the intruder — M7.6 ruling
			// #1 (no mid-field materialize; the old centerline spawn is gone). Both
			// clamp to the bot half + navmesh below.
			const FVector CastleRed = GetCastleRedLocation();
			FVector Desired;
			if (bChosenIsBuilding)
			{
				const FVector IntruderLocation = NearestIntruder->GetActorLocation();
				const FVector ToIntruder2D = FVector(IntruderLocation.X - CastleRed.X, IntruderLocation.Y - CastleRed.Y, 0.f);
				const FVector Dir2D = ToIntruder2D.GetSafeNormal();
				const float IntruderDist = static_cast<float>(ToIntruder2D.Size());
				const float MinStandoff = CastlePlinthClearance + 150.f; // clear of the plinth keep-out
				const float Standoff = FMath::Clamp(TowerDefenseStandoff, MinStandoff, FMath::Max(MinStandoff, IntruderDist - 100.f));
				Desired = Dir2D.IsNearlyZero()
					? CastleRed + FVector(-Standoff, 0.f, 0.f) // intruder atop the castle: fall back toward the centerline
					: CastleRed + Dir2D * Standoff;
				Desired.Z = CastleRed.Z;
			}
			else
			{
				// Toward the centerline (X=0) whichever half the castle sits on — the
				// same sign convention the miner approach uses.
				const float TowardCenterSign = (CastleRed.X >= 0.f) ? -1.f : 1.f;
				Desired = CastleRed + FVector(TowardCenterSign * BotCastleSpawnOffset,
					FMath::FRandRange(-BotSpawnLaneSpread, BotSpawnLaneSpread), 0.f);
				Desired.Z = CastleRed.Z;
			}

			FVector SpawnPoint;
			if (ComputeValidBotSpawnPoint(Desired, bChosenIsBuilding, SpawnPoint))
			{
				const int32 GoldBefore = Gold;
				if (SpawnBotCardActor(Chosen.CardID, bChosenIsBuilding, SpawnPoint, *BotState, Chosen.Row->Cost, Chosen.Row->SwarmCount))
				{
					Deck->ConfirmPlayFromHand(Chosen.Slot);
					UE_LOG(LogSiegeBot, Log,
						TEXT("[Bot %s] Rule 1 (Defend): played %s '%s' (cost %d) at (%.0f, %.0f, %.0f) vs intruder '%s' — gold %d->%d."),
						*GetNameSafe(this), bChosenIsBuilding ? TEXT("building") : TEXT("unit"),
						*Chosen.CardID.ToString(), Chosen.Row->Cost,
						SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z,
						*GetNameSafe(NearestIntruder), GoldBefore, BotState->GetGold());
				}
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Verbose,
					TEXT("ASiegeBotController '%s': Rule 1 wanted '%s' but found no valid spawn point this tick — retrying next tick."),
					*GetNameSafe(this), *Chosen.CardID.ToString());
			}
			return; // rule 1 fired: it owns this tick (a refused point simply retries next tick)
		}
		// no AFFORDABLE defensive card → rule 1 did NOT fire; fall through
	}

	// ---- Rule 2: ECONOMY — half is clear; a Miner (under the target + cap) or a Deep Mine ----
	if (!NearestIntruder)
	{
		// 2a) MINER — byte-for-byte with TASK-046: only while ALIVE miners are under
		//     the target AND the §3.3 hard cap (CanAddMiner) still allows one more.
		if (BotState->GetAliveMinerCount() < TargetMinerCount && BotState->CanAddMiner())
		{
			const int32 CardIndex = FindAffordableCardByID(HandCards, Gold, MinerCardID);
			if (CardIndex != INDEX_NONE)
			{
				const FBotHandCard& Chosen = HandCards[CardIndex];

				// Spawn just in FRONT of GoldNode_Red (toward the centerline) so the
				// miner walks the last stretch, then activates its +1/s at the node.
				const float ApproachSign = (BotTeam == ETeamId::Red) ? -1.f : 1.f;
				const FVector Desired = GetGoldNodeRedLocation() + FVector(ApproachSign * MinerNodeApproachOffset, 0.f, 0.f);

				FVector SpawnPoint;
				if (ComputeValidBotSpawnPoint(Desired, /*bIsBuilding=*/ false, SpawnPoint))
				{
					const int32 GoldBefore = Gold;
					if (SpawnBotCardActor(Chosen.CardID, /*bIsBuilding=*/ false, SpawnPoint, *BotState, Chosen.Row->Cost, /*SwarmCount=*/ 0))
					{
						Deck->ConfirmPlayFromHand(Chosen.Slot);
						UE_LOG(LogSiegeBot, Log,
							TEXT("[Bot %s] Rule 2 (Economy): played Miner '%s' (cost %d) toward GoldNode_Red at (%.0f, %.0f, %.0f) — miners now %d/%d, gold %d->%d."),
							*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
							SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z,
							BotState->GetAliveMinerCount(), TargetMinerCount, GoldBefore, BotState->GetGold());
					}
				}
				else
				{
					UE_LOG(LogGitClaudeUnrealTest, Verbose,
						TEXT("ASiegeBotController '%s': Rule 2 wanted a Miner but found no valid spawn point this tick."),
						*GetNameSafe(this));
				}
				return; // rule 2 fired (Miner)
			}
		}

		// 2b) DEEP MINE — a building-routed economy play (§4 M4). No miner-cap
		//     interaction (Deep Mine is not a miner): it just needs the half clear
		//     (guaranteed by the enclosing !NearestIntruder) and an affordable Deep
		//     Mine in hand. Spawned near GoldNode_Red, deep in the bot half, honoring
		//     the §3.5 building clearance via ComputeValidBotSpawnPoint(bIsBuilding).
		{
			const int32 CardIndex = FindAffordableEconomyBuildingCard(HandCards, Gold, BuildingEconomyCardIDs);
			if (CardIndex != INDEX_NONE)
			{
				const FBotHandCard& Chosen = HandCards[CardIndex];
				const FVector Desired = GetGoldNodeRedLocation();

				FVector SpawnPoint;
				if (ComputeValidBotSpawnPoint(Desired, /*bIsBuilding=*/ true, SpawnPoint))
				{
					const int32 GoldBefore = Gold;
					if (SpawnBotCardActor(Chosen.CardID, /*bIsBuilding=*/ true, SpawnPoint, *BotState, Chosen.Row->Cost, /*SwarmCount=*/ 0))
					{
						Deck->ConfirmPlayFromHand(Chosen.Slot);
						UE_LOG(LogSiegeBot, Log,
							TEXT("[Bot %s] Rule 2 (Economy): played Deep Mine '%s' (cost %d) near GoldNode_Red at (%.0f, %.0f, %.0f) — gold %d->%d."),
							*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
							SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z, GoldBefore, BotState->GetGold());
					}
				}
				else
				{
					UE_LOG(LogGitClaudeUnrealTest, Verbose,
						TEXT("ASiegeBotController '%s': Rule 2 wanted a Deep Mine but found no valid spawn point this tick."),
						*GetNameSafe(this));
				}
				return; // rule 2 fired (Deep Mine)
			}
		}
	}

	// ---- Rule 3: SPELLS (§4 M5 extension, TASK-102) — 3a Fireball at a clustered push, else 3b Lightning at a defended player tower ----
	// M5 ruling 10: the bot resolves DIRECTLY through USpellLibrary::ResolveSpell —
	// targeting mode is a human affordance; the resolver owns no-friendly-fire, castle
	// scaling and the VFX contract (TASK-098). Hand + affordability checks come FIRST
	// so the world scans below only run when a cast is actually possible, and all
	// scanning stays inside this 2 s cadence (spec point 5). Gold + discard-pile
	// accounting mirror unit plays: resolve first (the spell's "spawn"), gold as the
	// LAST gate, then ConfirmPlayFromHand moves the card to the discard pile.
	if (UWorld* World = GetWorld())
	{
		// 3a) FIREBALL at >= FireballClusterMinUnits clustered player units. The cluster
		//     radius is the Fireball ROW's own AoERadius (300 — GDD §4; data-driven law,
		//     never hardcoded), so the bot only casts when the cluster fits the blast.
		{
			const int32 CardIndex = FindAffordableCardByID(HandCards, Gold, FireballCardID);
			if (CardIndex != INDEX_NONE)
			{
				const FBotHandCard& Chosen = HandCards[CardIndex];
				FVector ClusterCentroid = FVector::ZeroVector;
				int32 ClusterSize = 0;
				if (FindFireballClusterTarget(Chosen.Row->AoERadius, FireballClusterMinUnits, ClusterCentroid, ClusterSize))
				{
					const int32 GoldBefore = Gold;
					// TASK-236 call-site flag (CONVENTIONS "Spell delivery overhaul
					// 2026-07-21"): Fireball is now a HeroLine spell — the centroid is
					// passed as the AIM-POINT and the resolver fires a line FROM THIS
					// BOT'S CASTLE toward it (the bot has no hero — flagged design
					// default). A cluster beyond ASpellLineSweep::LineRange of the
					// castle therefore WHIFFS (spent, no hits — the whiffed-Fireball
					// rule); recorded on the TASK-240 playtest WATCH list ("bot-origin
					// feel"). Decision logic deliberately unchanged this wave.
					if (USpellLibrary::ResolveSpell(World, Chosen.CardID, *Chosen.Row, BotTeam, ClusterCentroid))
					{
						// Same this-tick invariant as rule 5's fee: affordability held above
						// and income only ADDS between checks, so SpendGold cannot fail; a
						// false return is a hard-invariant tripwire (the spell already
						// resolved — at worst one free cast, never a double-charge).
						if (!BotState->SpendGold(Chosen.Row->Cost))
						{
							UE_LOG(LogGitClaudeUnrealTest, Warning,
								TEXT("ASiegeBotController '%s': Rule 3a resolved '%s' but SpendGold(%d) refused at gold %d — should be unreachable (affordability held this tick)."),
								*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost, GoldBefore);
						}
						Deck->ConfirmPlayFromHand(Chosen.Slot);
						UE_LOG(LogSiegeBot, Log,
							TEXT("[Bot %s] Rule 3a (Spell-Fireball): cast '%s' (cost %d) at cluster centroid (%.0f, %.0f, %.0f) — %d player units within %.0f, gold %d->%d."),
							*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
							ClusterCentroid.X, ClusterCentroid.Y, ClusterCentroid.Z,
							ClusterSize, Chosen.Row->AoERadius, GoldBefore, BotState->GetGold());
					}
					else
					{
						// Resolver refusal (bad row/degenerate state — TASK-098 semantics):
						// no gold moved, the card stays in hand; NOT a decision-trace line.
						UE_LOG(LogGitClaudeUnrealTest, Verbose,
							TEXT("ASiegeBotController '%s': Rule 3a found a %d-unit cluster but ResolveSpell('%s') refused — no gold spent, card retained; retrying next tick."),
							*GetNameSafe(this), ClusterSize, *Chosen.CardID.ToString());
					}
					return; // rule 3 fired: a FOUND target owns this tick (a refused resolve simply retries)
				}
				// no qualifying cluster → 3a did not fire; consider 3b
			}
		}

		// 3b) LIGHTNING at a player tower with >= LightningTowerMinUnits player units
		//     within the Lightning ROW's own AoERadius (700 — GDD §4; data-driven law):
		//     tower + defenders die to one bolt — the §4 "tower-killer" played as the
		//     GDD prescribes ("Lightning at a tower adjacent to 2+ units").
		{
			const int32 CardIndex = FindAffordableCardByID(HandCards, Gold, LightningCardID);
			if (CardIndex != INDEX_NONE)
			{
				const FBotHandCard& Chosen = HandCards[CardIndex];
				int32 NearbyUnitCount = 0;
				if (AActor* TowerTarget = FindLightningTowerTarget(Chosen.Row->AoERadius, LightningTowerMinUnits, NearbyUnitCount))
				{
					const FVector TargetPoint = TowerTarget->GetActorLocation();
					const int32 GoldBefore = Gold;
					// TASK-236 call-site flag: Lightning stays GroundCircle — TargetPoint
					// remains the impact center, byte-untouched by the delivery overhaul
					// (its radius change is TASK-237, data-only).
					if (USpellLibrary::ResolveSpell(World, Chosen.CardID, *Chosen.Row, BotTeam, TargetPoint))
					{
						if (!BotState->SpendGold(Chosen.Row->Cost))
						{
							UE_LOG(LogGitClaudeUnrealTest, Warning,
								TEXT("ASiegeBotController '%s': Rule 3b resolved '%s' but SpendGold(%d) refused at gold %d — should be unreachable (affordability held this tick)."),
								*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost, GoldBefore);
						}
						Deck->ConfirmPlayFromHand(Chosen.Slot);
						UE_LOG(LogSiegeBot, Log,
							TEXT("[Bot %s] Rule 3b (Spell-Lightning): cast '%s' (cost %d) at player tower '%s' (%.0f, %.0f, %.0f) — %d player units within %.0f, gold %d->%d."),
							*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
							*GetNameSafe(TowerTarget), TargetPoint.X, TargetPoint.Y, TargetPoint.Z,
							NearbyUnitCount, Chosen.Row->AoERadius, GoldBefore, BotState->GetGold());
					}
					else
					{
						UE_LOG(LogGitClaudeUnrealTest, Verbose,
							TEXT("ASiegeBotController '%s': Rule 3b found defended tower '%s' but ResolveSpell('%s') refused — no gold spent, card retained; retrying next tick."),
							*GetNameSafe(this), *GetNameSafe(TowerTarget), *Chosen.CardID.ToString());
					}
					return; // rule 3 fired (see 3a note)
				}
				// no qualifying tower → 3b did not fire; fall through to rule 4
			}
		}
	}

	// ---- Rule 4: ATTACK — banked to the threshold, most-expensive affordable UNIT, castle-front ----
	if (Gold >= AttackBankThreshold)
	{
		const int32 CardIndex = FindMostExpensiveUnitCard(HandCards, Gold);
		if (CardIndex != INDEX_NONE)
		{
			const FBotHandCard& Chosen = HandCards[CardIndex];

			// M7.6 ruling #1 (Jonathan, 2026-07-18): attack waves materialize
			// CASTLE-RELATIVE — BotCastleSpawnOffset in front of Castle_Red toward
			// the centerline, Y fanned across ±BotSpawnLaneSpread — and MARCH the
			// 10× field (replaces the old BotCenterlineSpawnX=350 mid-field commit;
			// resolved from the LIVE castle location like the defense path).
			// Flagged follow-up (Standing backlog): "adaptive bot spawn positioning
			// by strategy" — not designed.
			const FVector CastleRed = GetCastleRedLocation();
			const float TowardCenterSign = (CastleRed.X >= 0.f) ? -1.f : 1.f;
			FVector Desired = CastleRed + FVector(TowardCenterSign * BotCastleSpawnOffset,
				FMath::FRandRange(-BotSpawnLaneSpread, BotSpawnLaneSpread), 0.f);
			Desired.Z = CastleRed.Z;

			FVector SpawnPoint;
			if (ComputeValidBotSpawnPoint(Desired, /*bIsBuilding=*/ false, SpawnPoint))
			{
				const int32 GoldBefore = Gold;
				if (SpawnBotCardActor(Chosen.CardID, /*bIsBuilding=*/ false, SpawnPoint, *BotState, Chosen.Row->Cost, Chosen.Row->SwarmCount))
				{
					Deck->ConfirmPlayFromHand(Chosen.Slot);
					UE_LOG(LogSiegeBot, Log,
						TEXT("[Bot %s] Rule 4 (Attack): played unit '%s' (cost %d) castle-front (%.0f, %.0f, %.0f) — marching (M7.6 ruling #1) — gold %d->%d."),
						*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
						SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z, GoldBefore, BotState->GetGold());
				}
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Verbose,
					TEXT("ASiegeBotController '%s': Rule 4 wanted '%s' but found no valid spawn point this tick."),
					*GetNameSafe(this), *Chosen.CardID.ToString());
			}
			return; // rule 4 fired
		}
	}

	// ---- Rule 5: CYCLE — a card the bot can NEVER play in hand AND the discard fee available ----
	// HARDENED (folds the M3 TASK-046 WARN-2, now LIVE — Set II adds HeroUpgrade/
	// Utility/Instant cards the bot cannot play): (a) the fee is charged ONLY when
	// gold >= BotDiscardCost (guarded in the condition below — never at 0 gold), and
	// (b) the card is DISCARDED FIRST and the fee charged ONLY if the discard actually
	// happened (DiscardFromHand return-checked), so a no-op discard never bleeds a
	// gold charge. Rule 5 charges at most once then returns → no double-charge.
	// M5 (TASK-102): the bot's castable spells (Fireball/Lightning) are exempted from
	// the discard set — held for a rule-3 target, never cycled.
	{
		const int32 CardIndex = FindMostExpensiveUnplayableCard(HandCards, FireballCardID, LightningCardID);
		if (CardIndex != INDEX_NONE && Gold >= BotDiscardCost)
		{
			const FBotHandCard& Chosen = HandCards[CardIndex];
			const int32 GoldBefore = Gold;
			if (Deck->DiscardFromHand(Chosen.Slot))
			{
				// The gold >= BotDiscardCost gate above holds this same tick (income
				// only adds between checks), so SpendGold cannot fail here; a false
				// return is a hard-invariant tripwire (the card already left the hand —
				// at worst one free cycle, never a double-charge).
				if (!BotState->SpendGold(BotDiscardCost))
				{
					UE_LOG(LogGitClaudeUnrealTest, Warning,
						TEXT("ASiegeBotController '%s': Rule 5 discarded '%s' but SpendGold(%d) refused at gold %d — should be unreachable (gold >= fee held this tick)."),
						*GetNameSafe(this), *Chosen.CardID.ToString(), BotDiscardCost, GoldBefore);
				}
				UE_LOG(LogSiegeBot, Log,
					TEXT("[Bot %s] Rule 5 (Cycle): discarded unplayable '%s' (cost %d) for %d gold — gold %d->%d."),
					*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost, BotDiscardCost, GoldBefore, BotState->GetGold());
			}
			return; // rule 5 fired (a no-op discard still owns the tick; retry next tick)
		}
	}

	// No rule fired — bank gold and wait (no decision-trace line; a harmless idle tick).
}

bool ASiegeBotController::IsMatchActive() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const ASiegeGameMode* Mode = Cast<ASiegeGameMode>(World->GetAuthGameMode()))
		{
			return !Mode->HasMatchEnded();
		}
	}
	// No SiegeGameMode resolvable (degenerate world): permit — TASK-047's
	// StopDecisionTimer() is the authoritative match-end freeze regardless.
	return true;
}

bool ASiegeBotController::IsOnOwnHalf(double X) const
{
	// Red's own half is X >= boundary (CONVENTIONS world axes); a Blue bot flips it.
	return (BotTeam == ETeamId::Red) ? (X >= BotHalfBoundaryX) : (X <= BotHalfBoundaryX);
}

AActor* ASiegeBotController::FindNearestEnemyIntruderOnBotHalf() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const ETeamId EnemyTeam = (BotTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;
	const FVector CastleRed = GetCastleRedLocation();

	AActor* Nearest = nullptr;
	double NearestDistSq = TNumericLimits<double>::Max();

	// Enemy summoned units standing on the bot half (the §4 "enemy units").
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		ASummonedUnit* Unit = *It;
		if (!IsValid(Unit) || Unit->IsUnitDead() || Unit->GetTeamId() != EnemyTeam)
		{
			continue;
		}
		const FVector UnitLocation = Unit->GetActorLocation();
		if (!IsOnOwnHalf(UnitLocation.X))
		{
			continue;
		}
		const double DistSq = FVector::DistSquared2D(UnitLocation, CastleRed);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Unit;
		}
	}

	// The enemy hero also counts as pushing onto the bot half — so "player pushes
	// onto the bot half → a defensive play" holds whether they advance with units
	// OR their own hero (flagged decision; see handoffs/TASK-046.md). Alive only.
	for (TActorIterator<AHeroCharacter> It(World); It; ++It)
	{
		AHeroCharacter* Hero = *It;
		if (!IsValid(Hero) || Hero->IsDead() || Hero->GetTeamId() != EnemyTeam)
		{
			continue;
		}
		const FVector HeroLocation = Hero->GetActorLocation();
		if (!IsOnOwnHalf(HeroLocation.X))
		{
			continue;
		}
		const double DistSq = FVector::DistSquared2D(HeroLocation, CastleRed);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Hero;
		}
	}

	return Nearest;
}

bool ASiegeBotController::FindFireballClusterTarget(float ClusterRadius, int32 MinUnits, FVector& OutCentroid, int32& OutClusterSize) const
{
	OutClusterSize = 0;

	UWorld* World = GetWorld();
	if (!World || ClusterRadius <= 0.f)
	{
		// A zero/negative radius row can never cluster >1 unit — bad data, no cast
		// (null-safe: never a crash, the rule just does not fire).
		return false;
	}

	// Snapshot alive PLAYER-team (enemy) summoned unit locations ONCE; the pairwise
	// pass below is O(N^2) on this small snapshot only, runs solely inside the 2 s
	// cadence, and only after the caller's hand + affordability checks passed.
	// Units ONLY — the enemy hero is not a "player unit" (GDD §4 M5: "3+ clustered
	// player units"); miners ARE summoned units and deliberately count (a mining
	// cluster is a legitimate Fireball target — flagged decision).
	const ETeamId EnemyTeam = (BotTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;
	TArray<FVector> EnemyLocations;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		const ASummonedUnit* Unit = *It;
		if (IsValid(Unit) && !Unit->IsUnitDead() && Unit->GetTeamId() == EnemyTeam)
		{
			EnemyLocations.Add(Unit->GetActorLocation());
		}
	}
	if (EnemyLocations.Num() < MinUnits)
	{
		return false; // not enough player units alive anywhere — no cluster possible
	}

	// Cluster algorithm (TASK-102 spec: "cluster = any unit having >=2 other player
	// units within 300 — document"): every unit anchors a candidate cluster = all
	// player units (itself included) within ClusterRadius of it, 2D (M4.5 hills must
	// not break grouping). A candidate qualifies at MemberCount >= MinUnits (anchor +
	// MinUnits-1 others). Winner = the qualifying anchor with the MOST members; ties
	// broken by anchor distance to Castle_Red (nearest = biggest threat) — fully
	// deterministic for a given world state. The cast point is the winning cluster's
	// member CENTROID (the blast centers on the group, not on the anchor).
	const FVector CastleRed = GetCastleRedLocation();
	const double RadiusSq = FMath::Square(static_cast<double>(ClusterRadius));
	int32 BestCount = 0;
	double BestAnchorDistSq = TNumericLimits<double>::Max();
	FVector BestCentroid = FVector::ZeroVector;

	for (const FVector& Anchor : EnemyLocations)
	{
		int32 MemberCount = 0;
		FVector MemberSum = FVector::ZeroVector;
		for (const FVector& Candidate : EnemyLocations) // includes the anchor itself (DistSq 0)
		{
			if (FVector::DistSquared2D(Anchor, Candidate) <= RadiusSq)
			{
				++MemberCount;
				MemberSum += Candidate;
			}
		}
		if (MemberCount < MinUnits)
		{
			continue;
		}
		const double AnchorDistSq = FVector::DistSquared2D(Anchor, CastleRed);
		if (MemberCount > BestCount || (MemberCount == BestCount && AnchorDistSq < BestAnchorDistSq))
		{
			BestCount = MemberCount;
			BestAnchorDistSq = AnchorDistSq;
			BestCentroid = MemberSum / static_cast<double>(MemberCount);
		}
	}

	if (BestCount < MinUnits)
	{
		return false;
	}
	OutCentroid = BestCentroid;
	OutClusterSize = BestCount;
	return true;
}

AActor* ASiegeBotController::FindLightningTowerTarget(float SearchRadius, int32 MinUnits, int32& OutNearbyUnitCount) const
{
	OutNearbyUnitCount = 0;

	UWorld* World = GetWorld();
	if (!World || SearchRadius <= 0.f)
	{
		return nullptr; // bad radius row = no cast (null-safe, never a crash)
	}

	// Snapshot alive PLAYER-team unit locations once (shared across the tower loop;
	// same unit semantics as the Fireball scan — summoned units only, hero excluded).
	const ETeamId EnemyTeam = (BotTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;
	TArray<FVector> EnemyLocations;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		const ASummonedUnit* Unit = *It;
		if (IsValid(Unit) && !Unit->IsUnitDead() && Unit->GetTeamId() == EnemyTeam)
		{
			EnemyLocations.Add(Unit->GetActorLocation());
		}
	}
	if (EnemyLocations.Num() < MinUnits)
	{
		return nullptr; // not enough player units alive — no tower can qualify
	}

	// "A player tower" (GDD §4 M5: "Lightning at a tower adjacent to 2+ units") = a
	// live enemy ATower (the Arrow/Bomb/Ballista/Crystal family TASK-101 owns); walls,
	// Barracks and Deep Mines are ABuildings but NOT towers and do not qualify.
	// Winner = the qualifying tower with the MOST player units within SearchRadius
	// (2D); ties broken by tower distance to Castle_Red (nearest = biggest threat) —
	// deterministic. Lightning then resolves AT the tower: the tower itself sits at
	// distance 0 from the reticle, so the resolver's ruling-4 top-current-HP pick
	// includes it — 200 spell damage kills a 150 HP Arrow Tower (the "tower-killer").
	const FVector CastleRed = GetCastleRedLocation();
	const double RadiusSq = FMath::Square(static_cast<double>(SearchRadius));
	AActor* BestTower = nullptr;
	int32 BestCount = 0;
	double BestTowerDistSq = TNumericLimits<double>::Max();

	for (TActorIterator<ATower> It(World); It; ++It)
	{
		ATower* Tower = *It;
		if (!IsValid(Tower) || Tower->IsBuildingDestroyed() || Tower->GetTeamId() != EnemyTeam)
		{
			continue;
		}
		const FVector TowerLocation = Tower->GetActorLocation();
		int32 NearbyCount = 0;
		for (const FVector& UnitLocation : EnemyLocations)
		{
			if (FVector::DistSquared2D(TowerLocation, UnitLocation) <= RadiusSq)
			{
				++NearbyCount;
			}
		}
		if (NearbyCount < MinUnits)
		{
			continue;
		}
		const double TowerDistSq = FVector::DistSquared2D(TowerLocation, CastleRed);
		if (NearbyCount > BestCount || (NearbyCount == BestCount && TowerDistSq < BestTowerDistSq))
		{
			BestCount = NearbyCount;
			BestTowerDistSq = TowerDistSq;
			BestTower = Tower;
		}
	}

	OutNearbyUnitCount = BestCount;
	return BestTower;
}

FVector ASiegeBotController::GetCastleRedLocation() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACastle> It(World); It; ++It)
		{
			const ACastle* Castle = *It;
			if (IsValid(Castle) && Castle->GetTeamId() == BotTeam)
			{
				return Castle->GetActorLocation();
			}
		}
	}
	return CastleRedFallbackLocation;
}

FVector ASiegeBotController::GetGoldNodeRedLocation() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGoldNode> It(World); It; ++It)
		{
			const AGoldNode* Node = *It;
			if (IsValid(Node) && Node->GetTeam() == BotTeam)
			{
				return Node->GetActorLocation();
			}
		}
	}
	return GoldNodeRedFallbackLocation;
}

bool ASiegeBotController::ComputeValidBotSpawnPoint(const FVector& Desired, bool bIsBuilding, FVector& OutPoint)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys || !NavSys->GetDefaultNavDataInstance())
	{
		// No nav system / data: degrade OPEN to the half + plinth (+ clearance)
		// rule with one warning (house null-safety law — a missing system must
		// never brick the bot). L_Arena always has nav data, so this never fires there.
		if (!bWarnedNoNavData)
		{
			bWarnedNoNavData = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegeBotController '%s': no navigation data — bot spawn navmesh projection (GDD §3.5) skipped, using the half/plinth rule only."),
				*GetNameSafe(this));
		}
		if (IsBotHalfPointClear(Desired, bIsBuilding))
		{
			OutPoint = Desired;
			return true;
		}
		return false;
	}

	// Deterministic candidate ring: the desired point first, then widening rings —
	// so a plinth / clearance / half failure walks outward to the nearest clear,
	// on-navmesh spot instead of stalling forever on one refused point.
	static const float RingRadii[] = { 0.f, 250.f, 500.f, 800.f, 1100.f };
	static const int32 RingDirections = 8;
	for (float Radius : RingRadii)
	{
		const int32 NumSamples = (Radius <= 0.f) ? 1 : RingDirections;
		for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
		{
			const double Angle = (2.0 * PI * SampleIndex) / RingDirections;
			const FVector Candidate = Desired + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);

			FNavLocation Projected;
			if (!NavSys->ProjectPointToNavigation(Candidate, Projected, NavProjectionExtent))
			{
				continue;
			}
			if (IsBotHalfPointClear(Projected.Location, bIsBuilding))
			{
				OutPoint = Projected.Location;
				return true;
			}
		}
	}
	return false;
}

bool ASiegeBotController::IsBotHalfPointClear(const FVector& Point, bool bIsBuilding) const
{
	if (!IsOnOwnHalf(Point.X))
	{
		return false; // NEVER the enemy (Blue) half
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Castle plinth keep-out (mirrors ASiegePlayerController): no spawn inside any
	// castle's ~814x820 plinth footprint (2D box, CastlePlinthClearance half-extent).
	for (TActorIterator<ACastle> It(World); It; ++It)
	{
		const ACastle* Castle = *It;
		if (!IsValid(Castle))
		{
			continue;
		}
		const FVector CastleLocation = Castle->GetActorLocation();
		if (FMath::Abs(Point.X - CastleLocation.X) <= CastlePlinthClearance &&
			FMath::Abs(Point.Y - CastleLocation.Y) <= CastlePlinthClearance)
		{
			return false;
		}
	}

	// Building clearance (buildings only): >= BuildingClearance (2D) from every
	// live building — the §3.5 200-unit rule applied to bot placements too.
	if (bIsBuilding)
	{
		const double ClearanceSq = FMath::Square(static_cast<double>(BuildingClearance));
		for (TActorIterator<ABuilding> It(World); It; ++It)
		{
			const ABuilding* Building = *It;
			if (!IsValid(Building) || Building->IsBuildingDestroyed())
			{
				continue;
			}
			if (FVector::DistSquared2D(Building->GetActorLocation(), Point) < ClearanceSq)
			{
				return false;
			}
		}
	}
	return true;
}

UClass* ASiegeBotController::ResolveBotCardActorClass(FName CardID, bool bIsBuilding) const
{
	// CONVENTIONS composed soft-class paths (the SAME assets the player uses; the
	// spawn tags Team=Red and TASK-044 recolors at BeginPlay — no Red BP duplicate).
	const FString CardName = CardID.ToString();
	const FString ClassPath = bIsBuilding
		? FString::Printf(TEXT("/Game/Blueprints/Buildings/BP_Building_%s.BP_Building_%s_C"), *CardName, *CardName)
		: FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);
	UClass* RequiredBase = bIsBuilding ? ABuilding::StaticClass() : ASummonedUnit::StaticClass();

	UClass* ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath)).LoadSynchronous();
	if (ActorClass && ActorClass->IsChildOf(RequiredBase))
	{
		return ActorClass;
	}

	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("ASiegeBotController '%s': card class '%s' missing or not a %s (built in TASK-034/035) — bot play skipped, no gold spent (CONVENTIONS composed soft-class law)."),
		*GetNameSafe(this), *ClassPath, *RequiredBase->GetName());
	return nullptr;
}

AActor* ASiegeBotController::SpawnBotCardActor(FName CardID, bool bIsBuilding, const FVector& SpawnPoint, ASiegePlayerState& BotState, int32 Cost, int32 SwarmCount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UClass* ActorClass = ResolveBotCardActorClass(CardID, bIsBuilding);
	if (!ActorClass)
	{
		return nullptr; // logged in the resolver — NO gold spent
	}

	if (bIsBuilding)
	{
		// Building spawns flush with the projected ground; root is Static-mobility,
		// so AlwaysSpawn keeps it exactly where the point sits. Instigator is
		// deliberately nullptr (tower shots stay unattributable — TASK-027 property).
		const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnPoint);
		ABuilding* Building = World->SpawnActorDeferred<ABuilding>(
			ActorClass, SpawnTransform, /*Owner=*/ this, /*Instigator=*/ nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Building)
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegeBotController '%s': SpawnActorDeferred failed for building '%s' — no gold spent."),
				*GetNameSafe(this), *CardID.ToString());
			return nullptr;
		}

		// Gold is the LAST gate: a refusal destroys the half-spawned actor so exactly
		// Cost is deducted iff a building appears (TASK-030 destroy-on-fail pattern).
		if (!BotState.SpendGold(Cost))
		{
			Building->Destroy();
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegeBotController '%s': SpendGold(%d) refused at spawn for '%s' (the CanAfford pre-check should prevent this) — building discarded, no gold spent."),
				*GetNameSafe(this), Cost, *CardID.ToString());
			return nullptr;
		}

		Building->InitBuilding(BotTeam, CardID);
		Building->FinishSpawning(SpawnTransform);
		return Building;
	}

	// Unit/Economy: spawn via the SHARED swarm entry (TASK-059) so the bot's swarms
	// match the player's. SpawnPoint is the GROUND point (already navmesh-projected +
	// own-half/clearance-validated by ComputeValidBotSpawnPoint); SpawnUnitSwarm
	// applies the capsule lift. SwarmCount>1 (Militia Mob = 4) fans copies on a
	// SwarmSpawnRadius circle for ONE Cost; SwarmCount<=1 spawns a single unit AT
	// SpawnPoint (byte-for-byte with the M1/M2 single-unit spawn). Instigator is
	// deliberately nullptr (the bot has no pawn — unit team attribution resolves via
	// each unit's own ITeamAgent, TASK-002/004 chain).
	TArray<ASummonedUnit*> SwarmUnits = ASiegePlayerController::SpawnUnitSwarm(
		World, ActorClass, CardID, BotTeam,
		/*SpawnOwner=*/ this, /*SpawnInstigator=*/ nullptr,
		SpawnPoint, FMath::Max(1, SwarmCount), SwarmSpawnRadius);
	if (SwarmUnits.Num() == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegeBotController '%s': SpawnUnitSwarm produced no units for '%s' — no gold spent."),
			*GetNameSafe(this), *CardID.ToString());
		return nullptr;
	}

	// Gold is the LAST gate (destroy-on-fail): a refusal unwinds EVERY spawned copy,
	// so exactly Cost is deducted iff the swarm appears — the TASK-030 pattern
	// generalized to N (the same discipline the player's confirm path uses, TASK-059).
	if (!BotState.SpendGold(Cost))
	{
		for (ASummonedUnit* SwarmUnit : SwarmUnits)
		{
			if (IsValid(SwarmUnit))
			{
				SwarmUnit->Destroy();
			}
		}
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegeBotController '%s': SpendGold(%d) refused at spawn for '%s' (the CanAfford pre-check should prevent this) — %d unit(s) unwound, no gold spent."),
			*GetNameSafe(this), Cost, *CardID.ToString(), SwarmUnits.Num());
		return nullptr;
	}

	// Representative actor — all copies are one play (one LogSiegeBot line at the rule).
	return SwarmUnits[0];
}

void ASiegeBotController::ResetBot()
{
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegeBotController '%s': ResetBot (Play Again, GDD §3.9) — fresh deck, economy, decision timer."),
		*GetNameSafe(this));

	// 1) Fresh §3.4 deck + hand of 6 for the new match. The bot is an AAIController,
	//    NOT an ASiegePlayerController, so ASiegeGameMode::PlayAgain's player-
	//    controller loop (which resets the player's deck via HandleMatchReset) never
	//    reaches it — ResetBot is the bot's §3.9 deck-reset entry point.
	if (DeckComponent)
	{
		DeckComponent->ResetDeck();
	}

	// 2) Miner/rate economy back to base. Idempotent belt-and-braces: the bot's
	//    ASiegePlayerState is also in GameState->PlayerArray, so PlayAgain's generic
	//    per-player-state loop already ran ResetEconomy + ResetGold + ResumeIncome on
	//    it (gold to 50, income timer restarted) after ASiegeGameState::ResetClock(),
	//    so the rate re-derives against a cleared overtime latch there. This call
	//    keeps ResetBot self-contained if ever invoked on its own.
	if (ASiegePlayerState* BotPS = GetBotPlayerState())
	{
		BotPS->ResetEconomy();
	}

	// 3) A clean decision cadence for the new match (clears any running/stale handle
	//    first). TASK-047 stops the timer at match end; Play Again restarts it here.
	StartDecisionTimer();
}
