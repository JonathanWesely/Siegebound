// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeBotController.h"

#include "CollisionQueryParams.h" // TASK-265 spawn-Z diagnostic ground trace (FCollisionQueryParams) — BattlefieldScatter::GroundZAt precedent
#include "Components/CapsuleComponent.h" // TASK-265: GetScaledCapsuleHalfHeight on the spawned unit — complete type required
#include "Engine/DataTable.h"
#include "Engine/HitResult.h" // TASK-265 spawn-Z diagnostic (FHitResult) — same precedent
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/Building.h"
#include "Siegebound/CaptureZone.h" // TASK-262: Red-owned mid zone spawn gate (CanTeamSpawnHere) — complete type, methods dereferenced
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DeckComponent.h"
#include "Siegebound/DeckLibrary.h" // UDeckLibrary::IsDeckLegal / GetDeckAverageCost — validate + log the chosen curated bot deck (M6 TASK-114)
#include "Siegebound/GoldNode.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeCombatStatics.h" // TASK-851 (WITCH-§8): FSiegeCombatStatics::IsAgentVisibleTo — the ONE veil rule, shared with the acquisition funnel. ⛔ NOT a second implementation.
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
	 *  candidate: the bot BANKS toward it (e.g. an Ogre needs 36 gold post-TASK-278 ×3, §4), so cycling
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
	// EditDefaultsOnly defaults. Each is legal against DT_Cards — sum(Count)==50
	// (CARD-UNCAP 2026-08-28: legality no longer bounds per-card copies; the
	// compositions below simply predate the uncap and happen to also sit within
	// the OLD per-card caps — Footman 12, MilitiaMob/Pikeman/Knight 6, Archer 10,
	// Cavalry/Sapper/Miner/BombTower/BallistaTower 4, Ogre/DeepMine 2,
	// Barracks/CrystalTower/Cleric 3, Wall 10, ArrowTower 8, Longbowman 4,
	// Lightning 2 — kept here as the historical record of how they were sized).
	// Both are composed ONLY of bot-PLAYABLE types — Unit/Building/
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

	// ⚠️ THE COST ANNOTATIONS BELOW WERE RE-DERIVED 2026-08-15 (TASK-578), AND THEY HAD
	// BEEN FALSE SINCE 2026-07-24 — this is handoffs/TASK-278.md §114's SECOND LIMB,
	// owed since that day and now closed. TASK-278 tripled all 28 card costs in
	// Docs/Data/cards.csv; these inline "cost x count" notes and BOTH "avg cost" figures
	// went on quoting the PRE-triple values (Footman 3, Ogre 12, avg ~4.72/~7.02).
	// ⛔ NONE OF IT IS READ AT RUNTIME, which is precisely why it rotted unnoticed for
	// three weeks: deck legality is sum(Count) == 50 (plus, until CARD-UNCAP
	// 2026-08-28 abolished it, Count <= MaxCopies) — COUNTS,
	// not costs — and the logged average comes from UDeckLibrary::GetDeckAverageCost
	// reading DT_Cards LIVE. Re-derived from the shipped cards.csv Cost column; this
	// class's own AttackBankThreshold = 36 (h) is the same ×3 scaling seen from the
	// other side. Format is unchanged: cost x count = subtotal.

	// [0] AGGRO RUSH — cheap, fast, unit-heavy pressure (avg cost 14.16 = 708/50). Sum 50.
	FDeckList AggroDeck;
	AggroDeck.DeckName = TEXT("Bot Aggro Rush");
	AggroDeck.Cards =
	{
		Entry(TEXT("Footman"),    12), //  9 x12 = 108
		Entry(TEXT("MilitiaMob"),  6), // 15 x6  = 90 (swarm: 4 bodies per copy)
		Entry(TEXT("Pikeman"),     6), // 15 x6  = 90
		Entry(TEXT("Knight"),      6), // 18 x6  = 108
		Entry(TEXT("Archer"),      6), // 12 x6  = 72
		Entry(TEXT("Cavalry"),     4), // 21 x4  = 84 (charge)
		Entry(TEXT("Sapper"),      4), // 15 x4  = 60 (siege suicide)
		Entry(TEXT("Wall"),        4), // 12 x4  = 48
		Entry(TEXT("Miner"),       2), // 24 x2  = 48 (minimal economy)
	};

	// [1] DEFENSIVE ECONOMY — towers, walls, full economy, heavy finishers (avg cost 21.06 = 1053/50). Sum 50.
	FDeckList FortressDeck;
	FortressDeck.DeckName = TEXT("Bot Defensive Economy");
	FortressDeck.Cards =
	{
		Entry(TEXT("Wall"),           8), // 12 x8 = 96 (TASK-252: 10 → 8, donor for Lightning ×2)
		Entry(TEXT("ArrowTower"),     8), // 15 x8 = 120
		Entry(TEXT("Knight"),         6), // 18 x6 = 108
		Entry(TEXT("BombTower"),      4), // 24 x4 = 96
		Entry(TEXT("BallistaTower"),  4), // 21 x4 = 84
		Entry(TEXT("Miner"),          4), // 24 x4 = 96 (full economy)
		Entry(TEXT("Barracks"),       3), // 30 x3 = 90 (Footman spawner)
		Entry(TEXT("CrystalTower"),   3), // 27 x3 = 81 (chain tower)
		Entry(TEXT("Cleric"),         3), // 18 x3 = 54 (heals)
		Entry(TEXT("Ogre"),           2), // 36 x2 = 72 (siege finisher)
		Entry(TEXT("DeepMine"),       2), // 45 x2 = 90 (raidable economy)
		Entry(TEXT("Lightning"),      2), // 24 x2 = 48 (spell — rule-3b tower-killer; TASK-252 per the M6 QA rec)
		Entry(TEXT("Longbowman"),     1), // 18 x1 = 18
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

				// The direction the tower is actually placed along. Folded out of the
				// old trailing ternary so the face measurement below and the placement
				// below CANNOT disagree about which way "toward the intruder" is; the
				// degenerate case (intruder atop the castle) keeps its -X fallback and
				// the result is arithmetically identical to the old expression.
				const FVector PlacementDir2D = Dir2D.IsNearlyZero() ? FVector(-1.f, 0.f, 0.f) : Dir2D;

				// TASK-575 / CONVENTIONS WR-§2b row C, ruling W2-R3 — THE STANDOFF IS
				// NOW MEASURED FROM THE WALL FACE, NOT THE CASTLE CENTRE. A centre
				// standoff smaller than the thing it is centred on cannot mean anything:
				// the authored 750 put the tower 343 uu OUTSIDE the M1 wall (as designed),
				// 469 uu INSIDE after CASTLE-3X, and 2,907 uu INSIDE at the 9× castle. A
				// x3 is banned here and the arithmetic is why — 2,250 is still 1,407 uu
				// inside. FaceDistance is the live colliding-bounds face along this exact
				// direction, and is 0 when the castle is unresolvable/degenerate, which
				// degrades every term below to its pre-TASK-575 value.
				const float FaceDistance = ResolveCastleFaceDistance(FVector2D(PlacementDir2D.X, PlacementDir2D.Y));

				// The floor. TASK-349 preserved the retired plinth keep-out's derived
				// floor as the literal 570 (= CastlePlinthClearance 420 + 150) to keep bot
				// decision output byte-identical; TASK-575 DELIBERATELY GIVES THAT
				// GUARANTEE UP, because 570 was sized against a ~814-uu castle and now
				// sits 3,087 uu inside the 9x footprint — it preserved a number that can
				// no longer mean what it meant. What survives is the 150-uu margin, which
				// was always the body-scale half of 420 + 150 and therefore does NOT scale
				// with the castle (CONVENTIONS WR-§1). The 570 literal is kept ONLY as the
				// degenerate-bounds floor, where it reproduces today's behaviour exactly.
				// This is still a standoff HEURISTIC, not a placement refusal — actual
				// validity remains ComputeValidBotSpawnPoint's nav projection + clearances.
				constexpr float TowerStandoffFaceMargin = 150.f;
				constexpr float TowerStandoffDegenerateFloor = 570.f;
				const float MinStandoff = FMath::Max(TowerStandoffDegenerateFloor, FaceDistance + TowerStandoffFaceMargin);

				// TowerDefenseStandoff is now a BAND PAST THE FACE, so it is added to the
				// face rather than used as the distance itself. The upper bound (short of
				// the intruder) and the FMath::Max guard on it are unchanged.
				const float DesiredStandoff = FaceDistance + TowerDefenseStandoff;
				const float Standoff = FMath::Clamp(DesiredStandoff, MinStandoff, FMath::Max(MinStandoff, IntruderDist - 100.f));
				Desired = CastleRed + PlacementDir2D * Standoff;
				Desired.Z = CastleRed.Z;
			}
			else
			{
				// Toward the centerline (X=0) whichever half the castle sits on — the
				// same sign convention the miner approach uses. TASK-575: the offset is
				// resolved from the castle's LIVE colliding bounds (wall face +
				// BotCastleSpawnOffset), so the unit materializes in FRONT of the castle
				// at every castle size instead of inside the hall at the 9x one.
				const float TowardCenterSign = (CastleRed.X >= 0.f) ? -1.f : 1.f;
				Desired = CastleRed + FVector(TowardCenterSign * ResolveCastleFrontAnchorOffset(),
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
					return; // rule 1 fired: it owns this tick (TASK-267: return only on a CONFIRMED play)
				}
			}
			else
			{
				// TASK-267 audit: rule 1 shares the IDENTICAL abandon-the-tick trap. With an intruder present and an
				// affordable defensive card, a persistent spawn failure used to return and starve rules 3/4/5 (the
				// Fireball/Lightning/attack the bot wants precisely when it CANNOT place a blocker). Fall through
				// instead; no gold spent, no card confirmed. Verbose kept - the once-per-streak Log promotion is
				// rule-2-specific per the TASK-267 spec.
				UE_LOG(LogGitClaudeUnrealTest, Verbose,
					TEXT("ASiegeBotController '%s': Rule 1 wanted '%s' but found no valid spawn point - falling through to the lower rules this tick (TASK-267)."),
					*GetNameSafe(this), *Chosen.CardID.ToString());
			}
			// TASK-267: no unconditional return - a rule-1 spawn FAILURE now falls through to rules 3/4/5 (rule 2
			// is skipped below while the intruder stands). Only a CONFIRMED play returns (added in the branch above).
		}
		// no AFFORDABLE defensive card → rule 1 did NOT fire; fall through
	}

	// ---- Rule 2: ECONOMY — half is clear; a Miner toward the best available mine, or a Deep Mine ----
	// W1-PREP mirrored mines (TASK-256): the two castle-adjacent per-team nodes are
	// GONE (deleted in TASK-257) — 6 neutral depleting mines (3/side, TASK-255)
	// replace them, and BOTH economy sub-rules anchor on AGoldNode::FindBestMineFor,
	// THE single finder the miners themselves retarget through (TASK-253/254):
	// tier-1 = nearest mine the bot's team can mine NOW, tier-2 = nearest enemy-
	// occupied non-depleted mine (a WAIT target — the spawned miner queues at the
	// ring and auto-claims when it frees), null = every mine depleted or none exist.
	if (!NearestIntruder)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector CastleRed = GetCastleRedLocation();
			AGoldNode* BestMine = AGoldNode::FindBestMineFor(World, BotTeam, CastleRed);

			// Mine-lockout trace — ONCE PER STATE CHANGE, never per 2 s tick (the
			// bLoggedMineLockout latch). A skip is a non-decision diagnostic, so both
			// lines stay OFF LogSiegeBot (one-line-per-FIRED-rule law).
			if (!BestMine && !bLoggedMineLockout)
			{
				bLoggedMineLockout = true;
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("ASiegeBotController '%s': FindBestMineFor returned null (every mine depleted or none exist) — rule 2a (Miner) SKIPPED until a mine is available again (never buy a doomed miner; all-depleted endgame = base income + Deep Mine)."),
					*GetNameSafe(this));
			}
			else if (BestMine && bLoggedMineLockout)
			{
				bLoggedMineLockout = false;
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("ASiegeBotController '%s': mine '%s' is available again — rule 2a (Miner) re-enabled."),
					*GetNameSafe(this), *GetNameSafe(BestMine));
			}

			// 2a) MINER — the TASK-046 gates byte-for-byte (ALIVE miners under the
			//     target AND the §3.3 hard cap allows one more) PLUS the TASK-256 mine
			//     gate: finder null ⇒ 2a is skipped ENTIRELY — a miner with no mine to
			//     walk to is a doomed purchase (it would idle forever on dead income).
			if (BestMine && BotState->GetAliveMinerCount() < TargetMinerCount && BotState->CanAddMiner())
			{
				const int32 CardIndex = FindAffordableCardByID(HandCards, Gold, MinerCardID);
				if (CardIndex != INDEX_NONE)
				{
					const FBotHandCard& Chosen = HandCards[CardIndex];

					// Materialize MinerNodeApproachOffset SHORT of the mine on its
					// own-castle side (2D) so the miner walks the last stretch in, then
					// clamp the desired point to the bot's own half (spawn law: the bot
					// NEVER spawns on the Blue half — ComputeValidBotSpawnPoint REJECTS
					// off-half points rather than clamping, and its widening ring tops
					// out at 1,100 uu, so an unclamped Blue-half desired point would
					// stall rule 2a forever instead of walking). For a Blue-half mine
					// the clamp lands the spawn at the centerline and the miner WALKS
					// the field to the mine — cross-field walks are CORRECT behavior
					// (plan-of-record; TASK-258 watch list, not a bug).
					const FVector MineLocation = BestMine->GetActorLocation();
					const FVector ApproachDir = FVector(CastleRed.X - MineLocation.X, CastleRed.Y - MineLocation.Y, 0.f).GetSafeNormal();
					FVector Desired = ApproachDir.IsNearlyZero()
						? MineLocation // degenerate (mine at the castle point): the ring search walks it clear
						: MineLocation + ApproachDir * MinerNodeApproachOffset;
					Desired.Z = MineLocation.Z;
					// KEPT AS AUTHORED (TASK-265). Stale-claim fix, TASK-575 (WR-§2b row G):
					// the box this note cited was 840 at TASK-265, went 2460 at TASK-349 and
					// is (7380,7380) since TASK-557. The CLAIM still holds at 7380 and that
					// was re-checked, not assumed: the box clamp limit is 7,340 about
					// Castle_Red (+25,000), so a box-clamped X never falls below 17,660 —
					// far own-half of BotHalfBoundaryX (0), so this half clamp cannot bind
					// after it. It is not dead code: it still binds on the capture-zone
					// pass-through path, where the box clamp deliberately does not run.
					if (!IsOnOwnHalf(Desired.X))
					{
						Desired.X = BotHalfBoundaryX;
					}

					FVector SpawnPoint;
					if (ComputeValidBotSpawnPoint(Desired, /*bIsBuilding=*/ false, SpawnPoint))
					{
						const int32 GoldBefore = Gold;
						if (SpawnBotCardActor(Chosen.CardID, /*bIsBuilding=*/ false, SpawnPoint, *BotState, Chosen.Row->Cost, /*SwarmCount=*/ 0))
						{
							Deck->ConfirmPlayFromHand(Chosen.Slot);
							UE_LOG(LogSiegeBot, Log,
								TEXT("[Bot %s] Rule 2 (Economy): played Miner '%s' (cost %d) toward mine '%s' at (%.0f, %.0f, %.0f) — miners now %d/%d, gold %d->%d."),
								*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
								*GetNameSafe(BestMine),
								SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z,
								BotState->GetAliveMinerCount(), TargetMinerCount, GoldBefore, BotState->GetGold());
							bRule2SpawnFailureLogged = false; // TASK-267: a successful rule-2 spawn clears the failure streak
							return; // rule 2 fired (Miner) - it owns this tick
						}
					}
					else
					{
						// TASK-267: no valid spawn point. FALL THROUGH to rules 3/4/5 instead of ABANDONING the tick (the old
						// return below permanently re-stalled the ladder: the Miner stayed in hand, AliveMinerCount stayed 0,
						// rule 2's precondition stayed satisfied, and rules 3/4/5 never ran again). No gold spent, no card
						// confirmed. Promoted Verbose -> Log, emitted at most once per contiguous failure streak (the latch).
						if (!bRule2SpawnFailureLogged)
						{
							bRule2SpawnFailureLogged = true;
							UE_LOG(LogGitClaudeUnrealTest, Log,
								TEXT("ASiegeBotController '%s': Rule 2 wanted a Miner but found no valid spawn point - FALLING THROUGH to the lower rules this tick (TASK-267; logged once per failure streak)."),
								*GetNameSafe(this));
						}
					}
					// no return: a failed rule-2a spawn falls through to 2b / rules 3-5 (TASK-267)
				}
			}

			// 2b) DEEP MINE — a building-routed economy play (§4 M4). No miner-cap
			//     interaction (Deep Mine is not a miner) and deliberately NO finder-
			//     null skip: a Deep Mine needs no gold mine to produce, so it stays
			//     the bot's all-depleted endgame economy (plan §T-F(7): base income +
			//     overtime + Deep Mine keep the endgame winnable). Anchored at the
			//     SAME finder result when one exists (the economy clusters where the
			//     miners work), else castle-front (BotCastleSpawnOffset toward the
			//     centerline); honors the §3.5 building clearance via
			//     ComputeValidBotSpawnPoint(bIsBuilding).
			{
				const int32 CardIndex = FindAffordableEconomyBuildingCard(HandCards, Gold, BuildingEconomyCardIDs);
				if (CardIndex != INDEX_NONE)
				{
					const FBotHandCard& Chosen = HandCards[CardIndex];

					FVector Desired;
					if (BestMine)
					{
						// Same own-half clamp as 2a — a Blue-half best mine anchors the
						// building at the centerline, never across it.
						Desired = BestMine->GetActorLocation();
						// KEPT AS AUTHORED (TASK-265). Stale-claim fix, TASK-575 (WR-§2b row G):
						// same correction as rule 2a above — the cited 840 box is now
						// (7380,7380) (840 -> 2460 at TASK-349 -> 7380 at TASK-557), and the
						// claim survives the change for the identical reason (a box-clamped X
						// never falls below 17,660, so this half clamp cannot bind after it).
						// A Deep Mine needs no mine adjacency, so building it inside the
						// castle box remains mechanically identical.
						if (!IsOnOwnHalf(Desired.X))
						{
							Desired.X = BotHalfBoundaryX;
						}
					}
					else
					{
						// TASK-575: same live-bounds castle-front resolve as the rule-1
						// unit and the rule-4 wave. This site MUST convert with them —
						// BotCastleSpawnOffset is now a band past the wall face, so
						// reading it as a bare centre offset here would put the Deep Mine
						// deeper inside the 9x keep than the stale value ever did.
						const float TowardCenterSign = (CastleRed.X >= 0.f) ? -1.f : 1.f;
						Desired = CastleRed + FVector(TowardCenterSign * ResolveCastleFrontAnchorOffset(), 0.f, 0.f);
						Desired.Z = CastleRed.Z;
					}

					FVector SpawnPoint;
					if (ComputeValidBotSpawnPoint(Desired, /*bIsBuilding=*/ true, SpawnPoint))
					{
						const int32 GoldBefore = Gold;
						if (SpawnBotCardActor(Chosen.CardID, /*bIsBuilding=*/ true, SpawnPoint, *BotState, Chosen.Row->Cost, /*SwarmCount=*/ 0))
						{
							Deck->ConfirmPlayFromHand(Chosen.Slot);
							const FString AnchorDesc = BestMine
								? FString::Printf(TEXT("near mine '%s'"), *GetNameSafe(BestMine))
								: FString(TEXT("castle-front (no available mine)"));
							UE_LOG(LogSiegeBot, Log,
								TEXT("[Bot %s] Rule 2 (Economy): played Deep Mine '%s' (cost %d) %s at (%.0f, %.0f, %.0f) — gold %d->%d."),
								*GetNameSafe(this), *Chosen.CardID.ToString(), Chosen.Row->Cost,
								*AnchorDesc,
								SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z, GoldBefore, BotState->GetGold());
							bRule2SpawnFailureLogged = false; // TASK-267: a successful rule-2 spawn clears the failure streak
							return; // rule 2 fired (Deep Mine) - it owns this tick
						}
					}
					else
					{
						// TASK-267: no valid spawn point. FALL THROUGH to rules 3/4/5 instead of ABANDONING the tick (same
						// permanent re-stall trap as 2a). No gold spent, no card confirmed. Promoted Verbose -> Log; shares
						// the bRule2SpawnFailureLogged streak latch with 2a (one line per streak covers both sub-rules).
						if (!bRule2SpawnFailureLogged)
						{
							bRule2SpawnFailureLogged = true;
							UE_LOG(LogGitClaudeUnrealTest, Log,
								TEXT("ASiegeBotController '%s': Rule 2 wanted a Deep Mine but found no valid spawn point - FALLING THROUGH to the lower rules this tick (TASK-267; logged once per failure streak)."),
								*GetNameSafe(this));
						}
					}
					// no return: a failed rule-2b spawn falls through to rules 3-5 (TASK-267)
				}
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
			// TASK-575 / ruling W2-R2: "in front of" is now MEASURED, not transcribed —
			// the offset is the castle's live colliding face + BotCastleSpawnOffset, so
			// a wave appearing INSIDE the Red hall is a DEFECT (CONVENTIONS WR-§9
			// outcome 9), not the designed behaviour.
			// Flagged follow-up (Standing backlog): "adaptive bot spawn positioning
			// by strategy" — not designed.
			const FVector CastleRed = GetCastleRedLocation();
			const float TowardCenterSign = (CastleRed.X >= 0.f) ? -1.f : 1.f;
			FVector Desired = CastleRed + FVector(TowardCenterSign * ResolveCastleFrontAnchorOffset(),
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
					return; // rule 4 fired - it owns this tick
				}
			}
			else
			{
				// TASK-267 audit: rule 4 shares the IDENTICAL abandon-the-tick trap. With gold >= AttackBankThreshold
				// (gold only accrues, so the gate stays satisfied) a persistent attack-spawn failure used to return
				// and starve rule 5 (Cycle) for the rest of the match. Fall through instead; no gold spent, no card
				// confirmed. Verbose kept - the once-per-streak Log promotion is rule-2-specific per the TASK-267 spec.
				UE_LOG(LogGitClaudeUnrealTest, Verbose,
					TEXT("ASiegeBotController '%s': Rule 4 wanted '%s' but found no valid spawn point - falling through to rule 5 this tick (TASK-267)."),
					*GetNameSafe(this), *Chosen.CardID.ToString());
			}
			// no return: a failed rule-4 spawn falls through to rule 5 (TASK-267)
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

		// ⭐⭐ TASK-851 (WITCH-§8), SCAN 1 of 5 — THE VEIL, ASKED WITH THE ⛔ SAME RULE THE FUNNEL USES.
		// This iterator NEVER touches FSiegeCombatStatics::GatherHostileAgents: it is CLASS-based
		// (ASummonedUnit) with a half-of-map term, while the funnel is INTERFACE-based (ITeamAgent)
		// and returns towers and heroes too. ⛔ Routing it through the gather would CHANGE WHAT THE
		// BOT REACTS TO — a behaviour change wearing a refactor's clothes (TASK-851(4)). So the scan
		// stays, and honours the veil by calling the ONE shipped predicate instead.
		// ⛔ DO NOT read the veil flag inline here and DO NOT re-express the rule: two implementations
		// of "can this side see that unit" is exactly the divergence WITCH-§1 exists to prevent, and
		// IsAgentVisibleTo's one-team/one-actor signature makes an argument SWAP untypeable.
		// ⚠️ HIDDEN, ⛔ NOT INVULNERABLE (J-W2): this drops the unit from ACQUISITION only. A veiled
		// unit the bot has already engaged stays attackable, and a blast still catches it.
		if (!FSiegeCombatStatics::IsAgentVisibleTo(BotTeam, Unit))
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
	//
	// ⛔⛔ TASK-851 (WITCH-§8), SCAN 2 of 5 — ⭐ THE OMISSION HERE IS ⛔ DECIDED, ⛔ NOT AN OVERSIGHT,
	// AND THAT IS WHY THIS COMMENT EXISTS. 🧑 `J-W10` rules the HERO ⛔ NOT VEILABLE — "invisible
	// units" are UNITS only, because WITCH-§4 targets "nearest friendly UNIT" and nothing can reach
	// the hero. ⇒ an IsAgentVisibleTo call in THIS loop would be ⛔ DEAD CODE THAT CONTRADICTS A LIVE
	// RULING: the predicate casts to ASummonedUnit and returns TRUE for everything else, so the
	// branch could never once be false, while reading as if the hero could be hidden.
	// ⚠️ IF J-W10 IS EVER REVERSED, this loop is the FIRST place to change — and it is a MANAGER
	// amendment to WITCH-§3, never a quiet edit here.
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
	//
	// ⭐⭐ TASK-851 (WITCH-§8), SCAN 3 of 5 — ⛔ AND THIS IS WHERE THE DESIGN IS, SO READ THE WHOLE
	// PARAGRAPH BEFORE "SIMPLIFYING" IT. ⚖️ CHOOSING WHERE TO THROW A FIREBALL IS AN ⛔ ACT OF
	// SEEING; ⛔ THE EXPLOSION IS NOT (`WITCH-§2`, 🧑 `J-W2`). ⇒ the veiled unit is dropped from the
	// snapshot the bot AIMS with — it cannot be picked as a cluster, and it cannot pad a cluster
	// centred on somebody else — ⛔ BUT THE BLAST ITSELF IS UNTOUCHED. FSiegeCombatStatics::
	// ApplyRadialDamage asks its gather for ESiegeVeilPolicy::IncludeVeiled, so a Fireball the bot
	// aimed at something else ⛔ STILL CATCHES a veiled unit standing in it.
	// ⛔⛔ A DIFF THAT ALSO MADE THE BLAST MISS VEILED UNITS WOULD HAVE DELETED THE CARD'S ONLY
	// COUNTER. That exemption lives in SiegeCombatStatics.cpp and is ⛔ NOT this task's to touch —
	// the suppression here is purely about the AIM POINT.
	const ETeamId EnemyTeam = (BotTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;
	TArray<FVector> EnemyLocations;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		const ASummonedUnit* Unit = *It;
		if (IsValid(Unit) && !Unit->IsUnitDead() && Unit->GetTeamId() == EnemyTeam
			&& FSiegeCombatStatics::IsAgentVisibleTo(BotTeam, Unit))
		{
			EnemyLocations.Add(Unit->GetActorLocation());
		}
	}
	if (EnemyLocations.Num() < MinUnits)
	{
		return false; // not enough VISIBLE player units alive anywhere — no cluster possible
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
	//
	// ⭐⭐⭐ TASK-851 (WITCH-§8), SCAN 4 of 5 — ⛔⛔ AND THIS ONE IS ⛔ NOT IN `WITCH-§8`'s TABLE.
	// ⚠️⚠️ MEASURED BY TASK-851, DECLARED IN ITS HANDOFF §3, AND FLAGGED FOR QA AS THE ONE SCOPE
	// JUDGEMENT IN THE PASS. The law counted THREE scans across TWO functions and boarded the fix
	// for two of them. ⛔ MEASURED TOTAL: ⛔ FOUR ASummonedUnit scans across ⛔ FOUR functions plus one
	// AHeroCharacter scan — ⛔ THREE of the five are threat/aiming reads, and this is the third of
	// those (the fourth ASummonedUnit scan is IsBotHalfPointClear's PHYSICAL clearance test, which
	// is deliberately ⛔ not suppressed — its own comment says why).
	// ⇒ leaving this one alone would have shipped a bot that honours the veil when it aims
	// FIREBALL and detects veiled units when it aims LIGHTNING — from ⛔ the same rule, ⛔ in the
	// same file, ⛔ five lines apart in shape.
	// ⚖️ IT IS THE SAME CATEGORY AS SCAN 3, ⛔ NOT A NEW ONE: rule 3b counts enemy units around a
	// tower to CHOOSE where the bolt goes. Choosing is seeing (`WITCH-§2`, 🧑 `J-W2`) — the identical
	// sentence that justifies suppressing the Fireball cluster.
	// ⚠️ AND IT WAS THE WORSE HALF OF THE LEAK: Lightning RESOLVES through SpellLibrary's gather,
	// which is ⛔ ALREADY veil-suppressed (`FOG-§7` row 3). So an unsuppressed count here would have
	// made the bot spend 40 gold aiming a bolt at units the resolver ⛔ cannot damage — detecting
	// them AND whiffing on them.
	// ⭐ THE STANDING LESSON, ⛔ FROM `WITCH-§8`'s OWN CLOSING LINE, WHICH ITS TABLE THEN BROKE:
	// AN ACQUISITION SURFACE IS SIZED BY ⛔ WHO ENUMERATES UNITS, ⛔ NEVER BY A COUNT SOMEBODY ELSE
	// TOOK. Re-measure by SYMBOL (`SC-§38`), including when the count is in a law.
	const ETeamId EnemyTeam = (BotTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;
	TArray<FVector> EnemyLocations;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		const ASummonedUnit* Unit = *It;
		if (IsValid(Unit) && !Unit->IsUnitDead() && Unit->GetTeamId() == EnemyTeam
			&& FSiegeCombatStatics::IsAgentVisibleTo(BotTeam, Unit))
		{
			EnemyLocations.Add(Unit->GetActorLocation());
		}
	}
	if (EnemyLocations.Num() < MinUnits)
	{
		return nullptr; // not enough VISIBLE player units alive — no tower can qualify
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

const ACastle* ASiegeBotController::GetCastleRedActor() const
{
	// TASK-575: lifted verbatim out of GetCastleRedLocation (same iterator, same
	// IsValid + team filter, same first-match wins) so the castle's BOUNDS and its
	// LOCATION are always read off the SAME actor. Behaviour of the location getter
	// below is unchanged.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACastle> It(World); It; ++It)
		{
			const ACastle* Castle = *It;
			if (IsValid(Castle) && Castle->GetTeamId() == BotTeam)
			{
				return Castle;
			}
		}
	}
	return nullptr;
}

FVector ASiegeBotController::GetCastleRedLocation() const
{
	const ACastle* Castle = GetCastleRedActor();
	return Castle ? Castle->GetActorLocation() : CastleRedFallbackLocation;
}

float ASiegeBotController::ResolveCastleFaceDistance(const FVector2D& Direction2D)
{
	// TASK-575 / CONVENTIONS WR-§2b rulings W2-R2 + W2-R3 — the ONE place the bot
	// measures its own castle. Every castle-derived distance in this controller is
	// now (this) + an authored band, so a castle resize carries them all with it
	// (SC-§34's structural escape; the shipped model is ASiegeGameMode::
	// GetHeroStartTransform branch 3, which has survived two castle resizes
	// untouched).
	const ACastle* Castle = GetCastleRedActor();

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	if (Castle)
	{
		// bOnlyCollidingComponents = true: what matters is what BLOCKS a unit, not
		// the render/widget bounds — ACastle's HP-bar widget sits 9,450 uu up
		// (TASK-557) and must never inflate this.
		Castle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, BoundsOrigin, BoxExtent);
	}

	// FVector components are DOUBLE in UE5; keep the whole derivation in double and
	// narrow once at the return (CONVENTIONS compile traps — FMath::Min/Max are
	// single-type templates and will not deduce across float/double).
	const double ExtentX = FMath::Abs(BoxExtent.X);
	const double ExtentY = FMath::Abs(BoxExtent.Y);
	const FVector2D Direction = Direction2D.GetSafeNormal();
	const double AbsDirX = FMath::Abs(Direction.X);
	const double AbsDirY = FMath::Abs(Direction.Y);

	if (!Castle || ExtentX <= UE_KINDA_SMALL_NUMBER || ExtentY <= UE_KINDA_SMALL_NUMBER ||
		(AbsDirX <= UE_KINDA_SMALL_NUMBER && AbsDirY <= UE_KINDA_SMALL_NUMBER))
	{
		// Degrade to "no measurable castle" and let every caller fall back to its
		// authored band used as a bare centre-relative offset — the pre-TASK-575
		// shape, so a missing/unloaded castle can never brick the bot (house
		// null-safety law). Warned ONCE, and NOT on LogSiegeBot: that category is
		// one line per FIRED rule and this is a diagnostic, not a decision.
		if (!bWarnedNoCastleBounds)
		{
			bWarnedNoCastleBounds = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegeBotController '%s': no usable Castle_Red colliding bounds (castle %s, measured half-extent %.1f x %.1f) — castle-front spawn anchors and the rule-1 tower standoff fall back to their authored bands as bare centre offsets (pre-TASK-575 behavior). Logged once."),
				*GetNameSafe(this), Castle ? TEXT("found") : TEXT("NOT found"), ExtentX, ExtentY);
		}
		return 0.f;
	}

	// World AABB ray-exit from the centre: the boundary along a unit direction is the
	// NEAREST axis crossing. Exact in every direction — max(Ex, Ey) would be the
	// INSCRIBED square and would leave a 45-degree approach up to ~730 uu inside the
	// 9x footprint. Direction is unit and non-zero here, so at most one component can
	// be ~0 and neither division below can blow up.
	double FaceDistance = 0.0;
	if (AbsDirX <= UE_KINDA_SMALL_NUMBER)
	{
		FaceDistance = ExtentY / AbsDirY; // pure +/-Y
	}
	else if (AbsDirY <= UE_KINDA_SMALL_NUMBER)
	{
		FaceDistance = ExtentX / AbsDirX; // pure +/-X (the castle-front anchor case)
	}
	else
	{
		FaceDistance = FMath::Min(ExtentX / AbsDirX, ExtentY / AbsDirY);
	}

	return static_cast<float>(FaceDistance);
}

float ASiegeBotController::ResolveCastleFrontAnchorOffset()
{
	// The centerline-facing face is a +/-X crossing, and BoxExtent.X is a HALF-extent,
	// so the same magnitude serves both signs — the caller applies TowardCenterSign.
	// TASK-575 / ruling W2-R2: BotCastleSpawnOffset is the BAND PAST THE FACE, so at
	// the 9x castle this resolves 3,656.85 + 1,343.15 = 5,000 (and 1,750.15 at the M1
	// castle Jonathan authored 1,750 against — the mechanism reproduces his choice).
	// A degenerate/unresolvable castle returns 0 above, leaving the band alone.
	return ResolveCastleFaceDistance(FVector2D(1.0, 0.0)) + BotCastleSpawnOffset;
}

FVector ASiegeBotController::ClampAnchorToBotSpawnRegion(const FVector& Desired) const
{
	// ⚠ PASS-THROUGH CARVE-OUT — LOAD-BEARING, never make this unconditional.
	// An anchor that is ALREADY spawn-eligible (inside the Red spawn box, or inside
	// a RED-OWNED CaptureZone_Center) is returned untouched. That is precisely what
	// keeps the TASK-264-verified behavior alive: while Red holds the mid zone the
	// bot may stage there, and an unconditional clamp would drag those anchors back
	// to the castle box and delete the spawn-forward play the capture zone exists
	// for. The eligibility test is the SAME pair the spawn gate itself uses
	// (IsBotHalfPointClear), so "clamped" and "eligible" can never disagree.
	if (IsPointInBotSpawnBox(Desired) || IsPointInCapturedZone(Desired))
	{
		return Desired;
	}

	// Ineligible anchor ⇒ pull it into the spawn region (W1-PREP appendix 3a
	// anchor-clamp law, TASK-265). Per-axis clamp of the castle-relative delta into
	// ±(SpawnBoxHalfExtent - SpawnBoxAnchorInset): the inset parks the anchor just
	// inside the edge so the widening ring below has room on BOTH sides of it
	// (an anchor pinned exactly on the boundary throws half its candidate ring out
	// of the box — the one-sliver pile-up this task exists to remove). The Z is
	// preserved as authored; ProjectPointToNavigation owns the final Z. Any
	// refused sample (clearance/box/off-navmesh) is owned by the ring walk-out
	// below (the plinth keep-out this note once covered is RETIRED — TASK-349).
	const FVector CastleRed = GetCastleRedLocation();
	const double BoxLimitX = FMath::Max(0.0, static_cast<double>(SpawnBoxHalfExtent.X) - static_cast<double>(SpawnBoxAnchorInset));
	const double BoxLimitY = FMath::Max(0.0, static_cast<double>(SpawnBoxHalfExtent.Y) - static_cast<double>(SpawnBoxAnchorInset));

	FVector BoxClamped = Desired;
	BoxClamped.X = CastleRed.X + FMath::Clamp(Desired.X - CastleRed.X, -BoxLimitX, BoxLimitX);
	BoxClamped.Y = CastleRed.Y + FMath::Clamp(Desired.Y - CastleRed.Y, -BoxLimitY, BoxLimitY);

	// --- FLAGGED DEVIATION (documented in handoffs/TASK-265.md — manager/QA ruling
	// welcome; deleting this block reverts to the board's castle-box-only clamp) ---
	// The bot's spawn REGION is "castle box OR Red-owned capture zone" (that is the
	// gate IsBotHalfPointClear enforces, and this helper is named for the REGION,
	// not the box). Clamping every ineligible anchor to the castle box would make
	// the bot STRUCTURALLY unable to ever spawn in a zone it owns: no anchor in this
	// class is computed inside the mid zone, so the pass-through above can never
	// fire on its own. TASK-264 PIE result (f) — "the bot demonstrably staged 2
	// units mid-field only while Red held the zone" — was produced by the rule-2
	// mine anchors' ring-search REACHING the zone, and a box-only clamp deletes it,
	// which fails TASK-266 acceptance (e) and denies the bot the very ability
	// Jonathan's directive grants ("when captured, you can spawn units there").
	// So: clamp to whichever eligible region is NEARER to the desired anchor.
	// Castle-relative anchors (rule-1 unit, rule-4 attack waves) are always nearer
	// the castle box, so M7.6 ruling #1 "spawn castle-front and MARCH" is untouched
	// and the bot never gets a free forward spawn for its army; only the far-flung
	// rule-2 economy anchors can prefer a mid zone, and only while Red holds it.
	// With no zone placed, a Neutral zone, or a Blue-owned zone this block is inert
	// and the castle-box clamp above stands — byte-identical to the board's spec.
	const ACaptureZone* MidZone = nullptr;
	if (UWorld* World = GetWorld())
	{
		// The single level-placed CaptureZone_Center (TASK-260) — same first-instance
		// idiom as IsPointInCapturedZone, and null-safe when none is placed.
		TActorIterator<ACaptureZone> ZoneIt(World);
		if (ZoneIt)
		{
			MidZone = *ZoneIt;
		}
	}

	if (MidZone)
	{
		const FVector ZoneOrigin = MidZone->GetActorLocation();
		const FVector2D ZoneHalf = MidZone->GetZoneHalfExtent();
		const double ZoneLimitX = FMath::Max(0.0, static_cast<double>(ZoneHalf.X) - static_cast<double>(SpawnBoxAnchorInset));
		const double ZoneLimitY = FMath::Max(0.0, static_cast<double>(ZoneHalf.Y) - static_cast<double>(SpawnBoxAnchorInset));

		FVector ZoneClamped = Desired;
		ZoneClamped.X = ZoneOrigin.X + FMath::Clamp(Desired.X - ZoneOrigin.X, -ZoneLimitX, ZoneLimitX);
		ZoneClamped.Y = ZoneOrigin.Y + FMath::Clamp(Desired.Y - ZoneOrigin.Y, -ZoneLimitY, ZoneLimitY);

		// CanTeamSpawnHere is the TASK-260 seam and folds BOTH tests in one call —
		// "inside the zone" AND "Red owns it". A Neutral or Blue-owned zone returns
		// false here, so ownership is never duplicated or second-guessed locally.
		if (MidZone->CanTeamSpawnHere(ETeamId::Red, ZoneClamped) &&
			FVector::DistSquared2D(ZoneClamped, Desired) < FVector::DistSquared2D(BoxClamped, Desired))
		{
			return ZoneClamped;
		}
	}

	return BoxClamped;
}

bool ASiegeBotController::ComputeValidBotSpawnPoint(const FVector& Desired, bool bIsBuilding, FVector& OutPoint)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// W1-PREP appendix 3a (TASK-265) — THE single application point of the anchor
	// clamp. Every caller (rule-1 unit/tower, rule-2a miner, rule-2b Deep Mine,
	// rule-4 attack wave) funnels its desired point through here, so one edit
	// covers all five sites and NO call site does anchor math. Anchors that are
	// already eligible pass through byte-unchanged (see the carve-out above).
	//
	// Stale-claim fix, TASK-575 (WR-§2b row G). This note used to say the castle-front
	// anchor was one of the INELIGIBLE ones — "BotCastleSpawnOffset 1,750 ⇒ ~910 uu
	// outside an 840 box". BOTH halves are dead: the box has been 2460 (TASK-349) and
	// is now (7380,7380) (TASK-557), and the castle-front anchor is no longer 1,750 —
	// it RESOLVES from live castle bounds (TASK-575, ruling W2-R2) to ~5,000 from the
	// castle centre at the 9x castle, i.e. X ~ 20,000 for Red. That is inside the box
	// [17,620, 32,380], so the castle-front anchor now takes the PASS-THROUGH and this
	// clamp never touches it (it is also < the 7,340 clamp limit, so even the clamped
	// path would be a no-op — checked both ways deliberately). What the clamp still
	// owns is the far-flung rule-2 mine anchors, thousands of uu away; those land in
	// the box's centerline-facing front band and MARCH out from there, and M7.6 ruling
	// #1's intent is now carried by the anchor itself rather than by this clamp.
	// ⚖️ ROT-§4 rider (TASK-665, 2026-08-27): post-CASTLE-ROTATION the extent swap
	// resolves the castle-front anchor to ~5,035.33 from the centre ⇒ X ≈ 19,964.67
	// (TASK-663 §6 measured the wave live at 19,965 — in front of Red's rotated
	// GATE face). Both containment claims above re-checked at the new figure and
	// still hold (19,964.67 ∈ [17,620, 32,380]; 5,035.33 < 7,340) — the
	// pass-through carve-out still governs and this clamp still never touches it.
	// The ~20,000 figures above stay as authored — pre-ROT record.
	const FVector Anchor = ClampAnchorToBotSpawnRegion(Desired);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys || !NavSys->GetDefaultNavDataInstance())
	{
		// No nav system / data: degrade OPEN to the box/zone gate (+ clearance)
		// rule with one warning (house null-safety law — a missing system must
		// never brick the bot). L_Arena always has nav data, so this never fires there.
		if (!bWarnedNoNavData)
		{
			bWarnedNoNavData = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegeBotController '%s': no navigation data — bot spawn navmesh projection (GDD §3.5) skipped, using the spawn box/zone + clearance rules only."),
				*GetNameSafe(this));
		}
		if (IsBotHalfPointClear(Anchor, bIsBuilding))
		{
			OutPoint = Anchor;
			return true;
		}
		return false;
	}

	// Deterministic candidate ring: the clamped anchor first, then widening rings —
	// so a clearance / box failure walks outward to the nearest clear,
	// on-navmesh spot instead of stalling forever on one refused point. With the
	// UnitSpawnClearance rule live (appendix 3a) this walk is also what spreads a
	// wave: unit N takes the anchor, unit N+1 is refused there and steps to the
	// next free ring sample, so successive spawns no longer share one point.
	static const float RingRadii[] = { 0.f, 250.f, 500.f, 800.f, 1100.f };
	static const int32 RingDirections = 8;
	for (float Radius : RingRadii)
	{
		const int32 NumSamples = (Radius <= 0.f) ? 1 : RingDirections;
		for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
		{
			const double Angle = (2.0 * PI * SampleIndex) / RingDirections;
			const FVector Candidate = Anchor + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);

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
	// W1-PREP additions 3 (TASK-262 — the bot mirror of TASK-261): the spawn gate is
	// no longer the whole own-half. A point is spawn-eligible ONLY if it lies inside
	// the Red spawn box around Castle_Red OR inside a Red-owned capture zone. This
	// REPLACES the old `!IsOnOwnHalf(Point.X)` early-out; the unit/building
	// clearances below are UNCHANGED and still apply. (IsOnOwnHalf's OTHER callers
	// — miner-approach clamp, own-half unit/hero iteration — are TARGET/APPROACH
	// logic and stay half-based; only THIS spawn gate moves.)
	// TASK-349 (plinth-retirement law): the castle plinth keep-out loop that stood
	// here — the bot mirror of the player's IsPointInsideCastlePlinth — is RETIRED.
	// Spawn-inside the own hollow castle is now the feature; spawn truth = this box
	// gate + ComputeValidBotSpawnPoint's nav projection + the clearances below.
	if (!IsPointInBotSpawnBox(Point) && !IsPointInCapturedZone(Point))
	{
		return false; // outside both the Red castle box and any Red-owned mid zone
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Unit spawn clearance (NON-buildings only — W1-PREP appendix 3a, TASK-265):
	// >= UnitSpawnClearance (2D) from every live ASummonedUnit of EITHER team. This
	// is the anti-stacking rule: without it the ring-search happily re-served the
	// SAME point to every unit of a wave (the observed identical-XY pile-up), and
	// the capsules then collision-adjusted upward off each other. With it, the
	// deterministic ring in ComputeValidBotSpawnPoint walks to a genuinely FREE
	// slot. Buildings deliberately keep the BuildingClearance rule below and are
	// NOT subject to this one (a tower may sit next to friendly bodies). 0 disables.
	// Mirrors the BuildingClearance loop's shape exactly (precomputed square, 2D).
	//
	// ⛔⛔ TASK-851 (WITCH-§8), SCAN 5 of 5 — ⭐ THE SECOND ⛔ DECIDED OMISSION, WRITTEN DOWN FOR THE
	// SAME REASON AS THE HERO ARM: ⛔ an omission a reader cannot tell from an oversight will be
	// "fixed" by the next person. This scan is ⛔ NOT a threat read and ⛔ NOT an act of seeing — it
	// is a ⛔ PHYSICAL OCCUPANCY test, the anti-stacking rule of TASK-265. Three reasons it stays:
	//   (1) ⚖️ SAME CATEGORY AS THE BLAST (🧑 `J-W2`): a veiled unit still has a CAPSULE. Presence is
	//       not perception, and the veil hides a unit — ⛔ it does not make it incorporeal.
	//   (2) ⛔ IT IS NOT TEAM-FILTERED — the loop rejects a point near a live unit of ⛔ EITHER team.
	//       IsAgentVisibleTo always returns true for the viewer's OWN team, so a consult here would
	//       suppress ⛔ only enemies and turn a symmetric physics rule into an asymmetric one.
	//   (3) ⛔ IT WOULD BE AN EXPLOIT, NOT A FIX: park a veiled unit in the bot's spawn box and the
	//       bot would spawn its wave ⛔ INSIDE it — re-opening the identical-XY pile-up TASK-265 fixed.
	// ⚠️ The residual is real and tiny, and is declared rather than denied: the bot's ring search
	// silently steps around a veiled body, so a rejected candidate point is a hair of information.
	// ⛔ It is unobservable to the player (only the CHOSEN point is ever rendered) and it costs a
	// physics regression to close.
	if (!bIsBuilding && UnitSpawnClearance > 0.f)
	{
		const double UnitClearanceSq = FMath::Square(static_cast<double>(UnitSpawnClearance));
		for (TActorIterator<ASummonedUnit> It(World); It; ++It)
		{
			const ASummonedUnit* Unit = *It;
			if (!IsValid(Unit) || Unit->IsUnitDead())
			{
				continue;
			}
			if (FVector::DistSquared2D(Unit->GetActorLocation(), Point) < UnitClearanceSq)
			{
				return false;
			}
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

bool ASiegeBotController::IsPointInBotSpawnBox(const FVector& Point) const
{
	// Red spawn box: a 2D square centered on Castle_Red, half-extent SpawnBoxHalfExtent.
	// GetCastleRedLocation() is the SAME live team-filtered TActorIterator<ACastle> the
	// controller already uses and returns CastleRedFallbackLocation (+25000,0) when no
	// Red castle is found — so the existing +25000 fallback is preserved here. This box
	// replaces the old whole-own-half spawn gate (W1-PREP additions 3, TASK-262).
	const FVector CastleRed = GetCastleRedLocation();
	return FMath::Abs(Point.X - CastleRed.X) <= SpawnBoxHalfExtent.X &&
		FMath::Abs(Point.Y - CastleRed.Y) <= SpawnBoxHalfExtent.Y;
}

bool ASiegeBotController::IsPointInCapturedZone(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// The single mid capture zone (TASK-260); null-safe if absent = pre-capture
	// behavior (mid unspawnable). CanTeamSpawnHere folds the box test AND the
	// Red-ownership match into one call (Neutral/Blue owner => false).
	for (TActorIterator<ACaptureZone> It(World); It; ++It)
	{
		const ACaptureZone* Zone = *It;
		return Zone && Zone->CanTeamSpawnHere(ETeamId::Red, Point);
	}
	return false;
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

	// --- W1-PREP appendix 3a spawn-Z DIAGNOSTIC (TASK-265) — confirm before fixing ---
	// The ~215-232 uu float observed at the TASK-264 PIE is very likely a SYMPTOM of
	// units spawning inside each other (capsule collision-adjust lifting encroaching
	// spawns), which the UnitSpawnClearance rule above removes at the cause. So this
	// pass MEASURES instead of guessing: one line per bot unit spawn with the chosen
	// point Z, a traced ground Z, their delta, and the spawned actor's Z minus its
	// capsule half-height (= the real float above ground; a grounded capsule sits
	// exactly half-height above the floor, so ~0 here means NO float). TASK-266's PIE
	// reads these values and only a surviving >~50 uu residual justifies adding the
	// capped SnapPointToGround. Deliberately on LogGitClaudeUnrealTest, NEVER on
	// LogSiegeBot (one-line-per-FIRED-rule decision-trace law is inviolate).
	if (bLogSpawnZDiagnostic)
	{
		// Downward ECC_WorldStatic trace (the BattlefieldScatter::GroundZAt recipe),
		// bounded around the chosen point and ignoring the units we just spawned so a
		// freshly placed capsule cannot be mistaken for the floor. NOTE: scatter hills
		// IGNORE ECC_WorldStatic by the scatter-channel law, so on a hill this reports
		// the FLOOR under the hill, not the hill surface — which is exactly why any
		// future ground snap must stay capped (MaxGroundSnapDrop) and why a hill-side
		// residual belongs on the Ogre-near-hill spawn-lift WATCH, not here.
		const FVector TraceStart(SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z + 1000.0);
		const FVector TraceEnd(SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z - 5000.0);
		FCollisionQueryParams GroundParams(TEXT("BotSpawnZDiagnostic"), /*bTraceComplex=*/ false, this);
		for (const ASummonedUnit* SpawnedUnit : SwarmUnits)
		{
			if (IsValid(SpawnedUnit))
			{
				GroundParams.AddIgnoredActor(SpawnedUnit);
			}
		}

		FHitResult GroundHit;
		const bool bHitGround = World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, GroundParams);
		const double GroundZ = bHitGround ? GroundHit.ImpactPoint.Z : SpawnPoint.Z;

		const ASummonedUnit* Representative = SwarmUnits[0];
		const UCapsuleComponent* Capsule = IsValid(Representative) ? Representative->GetCapsuleComponent() : nullptr;
		const double ActorZ = IsValid(Representative) ? Representative->GetActorLocation().Z : SpawnPoint.Z;
		const double CapsuleHalfHeight = Capsule ? static_cast<double>(Capsule->GetScaledCapsuleHalfHeight()) : 0.0;

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegeBotController '%s': [SpawnZ] unit '%s' x%d — chosen Z %.1f, ground Z %.1f (%s), chosen-vs-ground delta %.1f; actor Z %.1f, capsule half-height %.1f, FLOAT above ground %.1f (TASK-265 diagnostic — bLogSpawnZDiagnostic)."),
			*GetNameSafe(this), *CardID.ToString(), SwarmUnits.Num(),
			SpawnPoint.Z, GroundZ, bHitGround ? TEXT("trace hit") : TEXT("TRACE MISS — chosen Z assumed"),
			SpawnPoint.Z - GroundZ,
			ActorZ, CapsuleHalfHeight, ActorZ - CapsuleHalfHeight - GroundZ);
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

	// TASK-267: clear the rule-2 spawn-failure streak latch so the first spawn failure of the fresh
	// match logs once (the bLoggedMineLockout latch self-heals on the first rule-2 tick of the match).
	bRule2SpawnFailureLogged = false;

	// 3) A clean decision cadence for the new match (clears any running/stale handle
	//    first). TASK-047 stops the timer at match end; Play Again restarts it here.
	StartDecisionTimer();
}
