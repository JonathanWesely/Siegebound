// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeAssistantSnapshot.h"

#include "EngineUtils.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "UObject/SoftObjectPtr.h"

#include "Siegebound/AncientGround.h"
#include "Siegebound/CaptureZone.h"
#include "Siegebound/Castle.h"
#include "Siegebound/GoldNode.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeAssistantCommand.h"     // TASK-417 (pinned link): LogSiegeAssistant - NEVER a second category
#include "Siegebound/SiegeAssistantVocabulary.h"  // TASK-417 (pinned link): the Zone-A synonym block
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/UnitCommand.h"

namespace SiegeAssistantSnapshotInternal
{
	/**
	 *  THE FIXED PLACE VOCABULARY - ONE DEFINITION, USED TWICE.
	 *
	 *  Zone A prints this table (symbol + description) and Capture() fills a
	 *  slot-indexed location array from it, so the prompt's vocabulary and the
	 *  code's FNames CANNOT DRIFT APART. That drift is the whole reason this is a
	 *  table rather than a run of string literals in the Zone-A block: a symbol
	 *  the model is taught but the game cannot resolve is a guaranteed
	 *  valid-shaped-wrong-command.
	 *
	 *  ⚠️ ORDER IS PART OF THE CONTRACT. Zone A prints in this order (and Zone A
	 *  must be byte-identical for the life of the process), GetPlaceNames()
	 *  returns in this order, and TASK-417's grammar generates its `where`
	 *  alternation from that array. Appending is safe; reordering rewrites Zone A
	 *  and throws away every cached prefix.
	 *
	 *  ⚠️ ASCII ONLY in every prompt literal in this file. The prompt is a byte
	 *  budget and a tokenizer input, not a doc comment: a stray multi-byte glyph
	 *  costs tokens, and makes the character-count cap stop matching the byte
	 *  count a QA reviewer measures.
	 */
	struct FPlaceDefinition
	{
		const TCHAR* Symbol;
		const TCHAR* Description;
	};

	static const FPlaceDefinition PlaceVocabulary[] =
	{
		{ TEXT("own_castle"),          TEXT("the player's castle") },
		{ TEXT("enemy_castle"),        TEXT("the enemy castle") },
		{ TEXT("mid"),                 TEXT("the capturable centre zone") },
		{ TEXT("ancient_ground_near"), TEXT("the ancient ground on the player's side") },
		{ TEXT("ancient_ground_far"),  TEXT("the ancient ground on the enemy side") },
		{ TEXT("nearest_mine"),        TEXT("the best gold mine for the player now") },
		{ TEXT("hero"),                TEXT("where the player's hero stands") }
	};

	/** Slot indices into PlaceVocabulary. Kept beside the table so a new place cannot be added to one and not the other. */
	enum class EPlaceSlot : int32
	{
		OwnCastle = 0,
		EnemyCastle,
		Mid,
		AncientGroundNear,
		AncientGroundFar,
		NearestMine,
		Hero,
		Count
	};

	static_assert(static_cast<int32>(UE_ARRAY_COUNT(PlaceVocabulary)) == static_cast<int32>(EPlaceSlot::Count),
		"PlaceVocabulary and EPlaceSlot must describe the same fixed place vocabulary.");

	/** DT_Cards, resolved null-safe at use time (the house soft-path law, GDD 3.0 data-driven rule). */
	static const TCHAR* const CardTablePath = TEXT("/Game/Data/DT_Cards.DT_Cards");

	/** Per-kind aggregate, sorted into DT_Cards row order before it reaches any member array. */
	struct FKindTally
	{
		FName Kind = NAME_None;
		int32 RowOrder = MAX_int32;
		int32 Total = 0;
		int32 Orderable = 0;
		int32 Followable = 0;
	};

	/**
	 *  Card row name -> canonical prompt symbol: the row name LOWER-CASED
	 *  ("Footman" -> "footman"). Lower case is purely a PROMPT decision (a
	 *  consistent case costs fewer tokens and stops the model inventing
	 *  capitalisation variants); FName comparison is case-insensitive, so
	 *  "footman" still finds the "Footman" row in any lookup.
	 */
	static FName CanonicalKind(FName CardID)
	{
		if (CardID.IsNone())
		{
			return NAME_None;
		}
		return FName(*CardID.ToString().ToLower());
	}

	/** HP as a 10% band. A STANDING castle never reports 0 - that would read as destroyed, and destroyed castles are dropped from the vocabulary entirely. */
	static int32 QuantizeHealthBand(float CurrentHP, float MaxHP)
	{
		if (MaxHP <= 0.f)
		{
			return INDEX_NONE;
		}

		const float Percent = FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f) * 100.f;
		const int32 Band = FMath::RoundToInt(Percent / 10.f) * 10;

		return (CurrentHP > 0.f) ? FMath::Max(Band, 10) : 0;
	}

	/**
	 *  Gold FLOORED to the nearest 10 - floored, never rounded. A band that
	 *  rounds up tells the model the player can afford something they cannot,
	 *  and "the AI told me I had the gold" is a worse failure than a slightly
	 *  pessimistic number.
	 */
	static int32 QuantizeGoldBand(int32 Gold)
	{
		return (FMath::Max(Gold, 0) / 10) * 10;
	}
}

void USiegeAssistantSnapshot::ResetSnapshot()
{
	Roster.Reset();
	PlaceNames.Reset();
	PlaceLocations.Reset();
	UnitKinds.Reset();
	KindTotals.Reset();
	KindOrderable.Reset();
	KindFollowable.Reset();

	StanceFree = 0;
	StanceFollowing = 0;
	StanceHolding = 0;
	StanceAmbushing = 0;

	OwnCastleHPBand = INDEX_NONE;
	EnemyCastleHPBand = INDEX_NONE;
	GoldBand = INDEX_NONE;

	MidOwner = EMidOwner::Absent;
	HeroPresence = EHeroPresence::Absent;

	// The one-shot log latches deliberately SURVIVE a reset: "logs once" means
	// once per process, not once per typed sentence.
}

void USiegeAssistantSnapshot::Capture(UWorld* World, ETeamId Team)
{
	using namespace SiegeAssistantSnapshotInternal;

	// A failed survey must leave an EMPTY snapshot, never a half-filled one -
	// every fixed key then prints `none`, which is a true statement, where a
	// stale row from the previous sentence would be a lie the model acts on.
	ResetSnapshot();

	if (!World)
	{
		UE_LOG(LogSiegeAssistant, Warning, TEXT("Snapshot::Capture called with a null world - the snapshot stays empty."));
		return;
	}

	const ETeamId EnemyTeam = (Team == ETeamId::Blue) ? ETeamId::Red : ETeamId::Blue;

	// Slot-indexed place resolution. A slot stays unset until something real
	// resolves for it, and only SET slots reach PlaceNames - so a destroyed
	// castle or a map with no capture zone simply drops out of the vocabulary
	// and can never be named.
	FVector ResolvedLocations[static_cast<int32>(EPlaceSlot::Count)];
	bool bSlotResolved[static_cast<int32>(EPlaceSlot::Count)];
	for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(EPlaceSlot::Count); ++SlotIndex)
	{
		ResolvedLocations[SlotIndex] = FVector::ZeroVector;
		bSlotResolved[SlotIndex] = false;
	}

	// ── PASS 1 - THE ORDERING TEAM'S CONTROLLER AND HERO ──
	// FindControllerForTeam is the M8 TEAM LAW resolve; GetFirstPlayerController
	// is BANNED in gameplay code. The controller is ALSO how group orders are
	// read below, so this one resolve serves both.
	const ASiegePlayerController* const OwningController = ASiegePlayerController::FindControllerForTeam(World, Team);

	const AHeroCharacter* Hero = OwningController ? Cast<AHeroCharacter>(OwningController->GetPawn()) : nullptr;
	if (!Hero)
	{
		// Fallback traversal - the only pass that is conditional. A hero can be
		// un-possessed or mid-respawn, and losing the hero anchor silently would
		// make `follow` and `rally` quietly unsayable.
		for (TActorIterator<AHeroCharacter> HeroIt(World); HeroIt; ++HeroIt)
		{
			const AHeroCharacter* const Candidate = *HeroIt;
			if (IsValid(Candidate) && Candidate->GetTeamId() == Team)
			{
				Hero = Candidate;
				break;
			}
		}
	}

	if (Hero)
	{
		// The hero-death ruling: a DEAD hero is not an anchor, so it is present
		// but not a nameable place.
		HeroPresence = Hero->IsDead() ? EHeroPresence::Down : EHeroPresence::Alive;
		if (HeroPresence == EHeroPresence::Alive)
		{
			ResolvedLocations[static_cast<int32>(EPlaceSlot::Hero)] = Hero->GetActorLocation();
			bSlotResolved[static_cast<int32>(EPlaceSlot::Hero)] = true;
		}
	}

	// Every "nearest" below is measured from here. World origin is the deliberate
	// no-hero fallback: it is the arena centre, so the finders still return the
	// sensible middle-of-the-map answer instead of whatever sits at index 0.
	const FVector ReferenceLocation = Hero ? Hero->GetActorLocation() : FVector::ZeroVector;

	// ── PASS 2 - CASTLES (the SHIPPED helper, never a fresh iteration) ──
	// ACastle::FindNearestCastleForTeam already skips DESTROYED castles, so
	// own_castle / enemy_castle / the ancient-ground derivation all inherit that
	// for free and a rubble heap is never a name the player can use. Writing a
	// castle TActorIterator in this file is a QA FAIL - one search, one owner.
	const ACastle* const OwnCastle = ACastle::FindNearestCastleForTeam(World, Team, ReferenceLocation);
	const ACastle* const EnemyCastle = ACastle::FindNearestCastleForTeam(World, EnemyTeam, ReferenceLocation);

	if (OwnCastle)
	{
		ResolvedLocations[static_cast<int32>(EPlaceSlot::OwnCastle)] = OwnCastle->GetActorLocation();
		bSlotResolved[static_cast<int32>(EPlaceSlot::OwnCastle)] = true;
		OwnCastleHPBand = QuantizeHealthBand(OwnCastle->GetCurrentHP(), OwnCastle->GetMaxHP());
	}

	if (EnemyCastle)
	{
		ResolvedLocations[static_cast<int32>(EPlaceSlot::EnemyCastle)] = EnemyCastle->GetActorLocation();
		bSlotResolved[static_cast<int32>(EPlaceSlot::EnemyCastle)] = true;
		EnemyCastleHPBand = QuantizeHealthBand(EnemyCastle->GetCurrentHP(), EnemyCastle->GetMaxHP());
	}

	// ── PASS 3 - ANCIENT GROUNDS: DERIVED, NEVER STORED ──
	// Under the 180-degree rotational-symmetry law there are EXACTLY TWO grounds
	// and they are rotational twins, so "nearest to my castle" and "nearest to
	// the enemy castle" are the two distinct grounds EXACTLY, by construction -
	// no distance banding, no tie-break, no extra state. The static is
	// team-neutral on purpose (a contested ground empowers both sides through
	// their own sorcerers); near/far is entirely the caller's framing, which is
	// this function.
	const FVector NearReference = OwnCastle ? OwnCastle->GetActorLocation() : ReferenceLocation;
	const AAncientGround* const NearGround = AAncientGround::FindNearestAncientGround(World, NearReference);
	const AAncientGround* const FarGround = EnemyCastle
		? AAncientGround::FindNearestAncientGround(World, EnemyCastle->GetActorLocation())
		: nullptr;

	if (NearGround)
	{
		ResolvedLocations[static_cast<int32>(EPlaceSlot::AncientGroundNear)] = NearGround->GetActorLocation();
		bSlotResolved[static_cast<int32>(EPlaceSlot::AncientGroundNear)] = true;
	}

	// The identity guard is for the DEGENERATE map, not the shipped one: a test
	// level with a single ground would otherwise publish two different symbols
	// that resolve to the same spot, and the model would be taught a distinction
	// the world does not have. On the shipped symmetric arena this never fires.
	if (FarGround && FarGround != NearGround)
	{
		ResolvedLocations[static_cast<int32>(EPlaceSlot::AncientGroundFar)] = FarGround->GetActorLocation();
		bSlotResolved[static_cast<int32>(EPlaceSlot::AncientGroundFar)] = true;
	}

	// ── PASS 4 - THE BEST MINE (the SHIPPED miner-selection rule, reused) ──
	// FindBestMineFor already encodes "minable now beats enemy-occupied" and
	// skips depleted nodes. Re-deriving it here to filter differently would give
	// the assistant a different idea of "the mine" than the miners have.
	if (const AGoldNode* const BestMine = AGoldNode::FindBestMineFor(World, Team, ReferenceLocation))
	{
		ResolvedLocations[static_cast<int32>(EPlaceSlot::NearestMine)] = BestMine->GetActorLocation();
		bSlotResolved[static_cast<int32>(EPlaceSlot::NearestMine)] = true;
	}

	// ── PASS 5 - THE MID ZONE ──
	// ONE level instance (CaptureZone_Center at the origin); the FIRST valid
	// instance wins, matching how the bot resolves it. Ownership is reported
	// RELATIVE to the ordering team - the prompt never says Blue or Red.
	for (TActorIterator<ACaptureZone> ZoneIt(World); ZoneIt; ++ZoneIt)
	{
		const ACaptureZone* const Zone = *ZoneIt;
		if (!IsValid(Zone))
		{
			continue;
		}

		ResolvedLocations[static_cast<int32>(EPlaceSlot::Mid)] = Zone->GetActorLocation();
		bSlotResolved[static_cast<int32>(EPlaceSlot::Mid)] = true;

		const ECaptureState Owner = Zone->GetCaptureOwner();
		const ECaptureState OwnState = (Team == ETeamId::Blue) ? ECaptureState::Blue : ECaptureState::Red;
		if (Owner == ECaptureState::Neutral)
		{
			MidOwner = EMidOwner::Neutral;
		}
		else
		{
			MidOwner = (Owner == OwnState) ? EMidOwner::Own : EMidOwner::Enemy;
		}
		break;
	}

	// Publish the resolvable places in FIXED VOCABULARY ORDER.
	for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(EPlaceSlot::Count); ++SlotIndex)
	{
		if (bSlotResolved[SlotIndex])
		{
			PlaceNames.Add(FName(PlaceVocabulary[SlotIndex].Symbol));
			PlaceLocations.Add(ResolvedLocations[SlotIndex]);
		}
	}

	// ── PASS 6 - THE ROSTER ──
	// Card row order is what the roster sorts by: it is FIXED for the life of the
	// table, where sorting by count or by distance would reorder mid-match and
	// invalidate the cached prefix on every sentence. GetRowNames() is that fixed
	// order (stable for a loaded table); a reimport is a content-lock-time change,
	// exactly like a vocabulary change.
	TMap<FName, int32> CardRowOrder;
	{
		// ⚠️ BRACES, NOT PARENTHESES - AND THIS IS A FIX, NOT A STYLE CHOICE.
		// Written with parentheses this line is C++'s MOST VEXING PARSE:
		// `FSoftObjectPath` is a TYPE and `CardTablePath` is an identifier, so
		// `FSoftObjectPath(CardTablePath)` parses as a PARAMETER DECLARATION
		// (the redundant parens around a parameter name are legal), and the whole
		// statement declares a FUNCTION named CardTableAsset returning
		// `const TSoftObjectPtr<UDataTable>`. The next line then applies `.` to a
		// function name - MSVC C2228 "left of '.LoadSynchronous' must have
		// class/struct/union", with C2737 on the same line as the knock-on,
		// because the failed initializer leaves the `const` local uninitialised.
		// Braces cannot be parsed as a parameter list, so the ambiguity is gone.
		// Every shipped call site in this repo dodges it by passing a string
		// LITERAL (which can never be a parameter name); this one hoists the path
		// to a named constant, which is what re-opened the trap.
		const TSoftObjectPtr<UDataTable> CardTableAsset{ FSoftObjectPath(CardTablePath) };
		if (const UDataTable* const CardTable = CardTableAsset.LoadSynchronous())
		{
			// FName comparison is case-insensitive, so these "Footman"-cased keys
			// are found by the canonical "footman" symbols without a second map.
			const TArray<FName> RowNames = CardTable->GetRowNames();
			for (int32 RowIndex = 0; RowIndex < RowNames.Num(); ++RowIndex)
			{
				CardRowOrder.Add(RowNames[RowIndex], RowIndex);
			}
		}
		else if (!bWarnedMissingCardTable)
		{
			bWarnedMissingCardTable = true;
			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Snapshot::Capture could not resolve %s - the roster falls back to lexical kind order (still fixed, still deterministic)."),
				CardTablePath);
		}
	}

	TArray<FKindTally> Tallies;

	for (TActorIterator<ASummonedUnit> UnitIt(World); UnitIt; ++UnitIt)
	{
		const ASummonedUnit* const Unit = *UnitIt;
		if (!IsValid(Unit) || Unit->GetTeamId() != Team || Unit->IsUnitDead())
		{
			continue;
		}

		const FName Kind = CanonicalKind(Unit->GetCardID());
		if (Kind.IsNone())
		{
			// A unit whose stats never bound has no symbol the player can say,
			// so it is not something the snapshot may offer.
			continue;
		}

		const int32 GroupId = Unit->GetCommandGroupId();

		// (Kind, GroupId) aggregation - NEVER a row per unit (CONVENTIONS 8).
		FSiegeAssistantRosterEntry* Entry = Roster.FindByPredicate(
			[Kind, GroupId](const FSiegeAssistantRosterEntry& Candidate)
			{
				return Candidate.Kind == Kind && Candidate.GroupId == GroupId;
			});
		if (!Entry)
		{
			Entry = &Roster.AddDefaulted_GetRef();
			Entry->Kind = Kind;
			Entry->GroupId = GroupId;
			Entry->Count = 0;
		}
		++Entry->Count;

		// ⚠️ ELIGIBILITY IS READ FROM THE SHIPPING PREDICATES, NEVER REIMPLEMENTED.
		// IsGroupCommandEligible / IsFollowCommandEligible already encode the
		// Cleric-follows-but-cannot-hold split, the Siege exclusion, the Miner's
		// two overrides and the team/alive/frozen gates. Two separate counts,
		// because one merged number would be wrong for one of the two verbs.
		FKindTally* Tally = Tallies.FindByPredicate(
			[Kind](const FKindTally& Candidate) { return Candidate.Kind == Kind; });
		if (!Tally)
		{
			Tally = &Tallies.AddDefaulted_GetRef();
			Tally->Kind = Kind;
			const int32* const FoundOrder = CardRowOrder.Find(Kind);
			Tally->RowOrder = FoundOrder ? *FoundOrder : MAX_int32;
		}
		++Tally->Total;
		if (Unit->IsGroupCommandEligible())
		{
			++Tally->Orderable;
		}
		if (Unit->IsFollowCommandEligible())
		{
			++Tally->Followable;
		}

		// Stance tally. FindUnitGroup's pointer ALIASES INTO UnitGroups - read it
		// inside this iteration and never cache it (the shipped aliasing rule).
		const FSiegeUnitGroup* const Group =
			(OwningController && GroupId != INDEX_NONE) ? OwningController->FindUnitGroup(GroupId) : nullptr;
		if (!Group)
		{
			// No group, or a group that has already been released/pruned - the
			// same self-heal the unit itself performs on a null lookup.
			++StanceFree;
		}
		else
		{
			switch (Group->Type)
			{
			case ESiegeGroupCommandType::Follow:
				++StanceFollowing;
				break;
			case ESiegeGroupCommandType::Hold:
				++StanceHolding;
				break;
			case ESiegeGroupCommandType::Ambush:
				++StanceAmbushing;
				break;
			default:
				++StanceFree;
				break;
			}
		}
	}

	// Fixed order: card row, then group id, then the symbol itself. The last term
	// only exists to make the order TOTAL (TArray::Sort is not stable), so two
	// kinds sharing a row order can never swap between sentences.
	Tallies.Sort([](const FKindTally& A, const FKindTally& B)
	{
		if (A.RowOrder != B.RowOrder)
		{
			return A.RowOrder < B.RowOrder;
		}
		return A.Kind.Compare(B.Kind) < 0;
	});

	Roster.Sort([&CardRowOrder](const FSiegeAssistantRosterEntry& A, const FSiegeAssistantRosterEntry& B)
	{
		const int32* const OrderA = CardRowOrder.Find(A.Kind);
		const int32* const OrderB = CardRowOrder.Find(B.Kind);
		const int32 RowA = OrderA ? *OrderA : MAX_int32;
		const int32 RowB = OrderB ? *OrderB : MAX_int32;
		if (RowA != RowB)
		{
			return RowA < RowB;
		}
		const int32 KindCompare = A.Kind.Compare(B.Kind);
		if (KindCompare != 0)
		{
			return KindCompare < 0;
		}
		return A.GroupId < B.GroupId;
	});

	UnitKinds.Reserve(Tallies.Num());
	KindTotals.Reserve(Tallies.Num());
	KindOrderable.Reserve(Tallies.Num());
	KindFollowable.Reserve(Tallies.Num());
	for (const FKindTally& Tally : Tallies)
	{
		UnitKinds.Add(Tally.Kind);
		KindTotals.Add(Tally.Total);
		KindOrderable.Add(Tally.Orderable);
		KindFollowable.Add(Tally.Followable);
	}

	// Gold. The controller's own player state first (silent); the game state's
	// team resolve is the fallback and LOGS when a team has no player state, so
	// asking it first would warn every sentence in a bot match.
	const ASiegePlayerState* PlayerState = OwningController ? OwningController->GetPlayerState<ASiegePlayerState>() : nullptr;
	if (!PlayerState)
	{
		if (const ASiegeGameState* const GameState = World->GetGameState<ASiegeGameState>())
		{
			PlayerState = GameState->GetPlayerStateForTeam(Team);
		}
	}
	if (PlayerState)
	{
		GoldBand = QuantizeGoldBand(PlayerState->GetGold());
	}

	UE_LOG(LogSiegeAssistant, Verbose,
		TEXT("Snapshot: %d kind(s), %d roster row(s), %d place(s), gold band %d."),
		UnitKinds.Num(), Roster.Num(), PlaceNames.Num(), GoldBand);
}

bool USiegeAssistantSnapshot::ResolvePlace(FName Place, FVector& OutLocation) const
{
	const int32 Index = PlaceNames.IndexOfByKey(Place);
	if (Index == INDEX_NONE || !PlaceLocations.IsValidIndex(Index))
	{
		// OutLocation is deliberately left UNTOUCHED on failure - a caller that
		// ignores the return value gets its own initialised value, never a
		// plausible-looking origin it might march an army to.
		return false;
	}

	OutLocation = PlaceLocations[Index];
	return true;
}

FString USiegeAssistantSnapshot::BuildZoneA(const USiegeAssistantVocabulary* Vocabulary) const
{
	using namespace SiegeAssistantSnapshotInternal;

	// ⚠️ NOTHING BELOW READS MEMBER STATE. That is the whole contract of Zone A:
	// same bytes every turn for the life of the process, so llama_kv_cache_seq_rm
	// keeps the prefix and turn two prefills ~150 tokens instead of ~500.
	FString Out;
	Out.Reserve(2048);

	Out += TEXT("[RULES]\n");
	Out += TEXT("Turn ONE Siegebound order into ONE JSON command. Output the JSON object only: no prose, no explanation.\n");
	Out += TEXT("\n");

	// ⚠️ THIS BLOCK IS THE MIRROR OF TASK-417's GBNF, AND IT MUST STAY ONE.
	// Zone A teaches the schema; USiegeAssistantGrammar::Build CONSTRAINS it. If
	// they disagree, constrained decoding fights the few-shots on every token
	// and accuracy (go/no-go bar #5) collapses for a reason no log line names.
	// Verified character-for-character against SiegeAssistantGrammar.cpp's root
	// / command / question / who / selection / item / count / at_least rules and
	// the SiegeAssistantJsonKeys + SiegeAssistantAsk symbol tables:
	//   key order   intent, who, where, when   (JsonObjectOpen + JsonNextKey)
	//   pair keys   "kind" and "n"             (NOT "count" - "count" is the
	//                                           GBNF RULE name, "n" is the JSON
	//                                           key, and they differ)
	//   selection   a JSON ARRAY of 1..3 items, bounded by alternation
	//   who         the array, or "all", or "none"
	// ⚠️ A LATER EDIT TO EITHER FILE MUST EDIT BOTH. There is no compile-time
	// link between them - this comment and the QA gate are the whole tie.
	//
	// THE SELECTION IS MULTI-KIND (manager ruling 15, CONVENTIONS §9 as
	// corrected 2026-08-02): the feature's flagship sentence - "send 10 footmen
	// WITH A SORCERER to the nearest ancient ground" - cannot be expressed by a
	// singular kind, so `who` carries up to SiegeAssistantMaxSelectionKinds (3)
	// pairs, which the parser splits into the index-aligned Kinds/Counts arrays.
	Out += TEXT("schema (a command):\n");
	Out += TEXT("{\"intent\":INTENT,\"who\":WHO,\"where\":WHERE,\"when\":WHEN}\n");
	Out += TEXT("INTENT = send | guard | ambush | follow | charge | fallback | rally\n");
	Out += TEXT("WHO    = [{\"kind\":KIND,\"n\":COUNT}] with 1 to 3 entries, or \"all\", or \"none\"\n");
	Out += TEXT("KIND   = a unit symbol from roster in [FORCES]\n");
	Out += TEXT("COUNT  = 1 to 30, or \"all\"\n");
	Out += TEXT("WHERE  = a place symbol from places in [FORCES], or \"none\"\n");
	Out += TEXT("WHEN   = \"now\", or {\"kind\":KIND,\"at_least\":1 to 30}\n");
	Out += TEXT("\n");

	// The question branch is always reachable in the grammar, so it must be
	// taught here too: a model that CANNOT decline is forced to invent a
	// command, and an invented command is the failure this whole design exists
	// to prevent. No few-shot is spent on it (the three-example count is pinned
	// by CONVENTIONS §8) - the schema line carries it.
	Out += TEXT("schema (a question, when the order cannot be translated):\n");
	Out += TEXT("{\"ask\":ASK}\n");
	Out += TEXT("ASK = which_unit | which_place | how_many | which_intent | unsupported\n");
	Out += TEXT("\n");

	// The intent glossary is the block that most directly buys accuracy on
	// go/no-go bar #5, because `send` vs `charge` is the one distinction a small
	// model gets wrong by default: Attack/Defend are latched ARMY-WIDE stances
	// with no selection (CONVENTIONS §8 executor seam), so "send 10 footmen at
	// the castle" is NOT the attack stance. If Zone A ever has to shrink, this
	// block and one few-shot are the two levers - in that order.
	Out += TEXT("intents:\n");
	Out += TEXT("send = move the selected units to a place\n");
	Out += TEXT("guard = station them at a place and hold it\n");
	Out += TEXT("ambush = station them at a place and let them chase kills\n");
	Out += TEXT("follow = they follow the hero\n");
	Out += TEXT("charge = whole army attacks; who and where are \"none\"\n");
	Out += TEXT("fallback = whole army defends home; who and where are \"none\"\n");
	Out += TEXT("rally = hero rallies units near him; who and where are \"none\"\n");
	Out += TEXT("\n");

	Out += TEXT("places (fixed vocabulary; only those listed in [FORCES] exist this match):\n");
	for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(EPlaceSlot::Count); ++SlotIndex)
	{
		Out.Appendf(TEXT("%s = %s\n"), PlaceVocabulary[SlotIndex].Symbol, PlaceVocabulary[SlotIndex].Description);
	}
	Out += TEXT("\n");

	// Rule 2 is CONVENTIONS §1 made visible to the model: the grammar's count
	// range is 1..30, NOT the live roster max, precisely so a request for 10
	// when 8 exist stays DETECTABLE. A model that "helpfully" shrinks the number
	// makes the shortfall invisible, which is the exact valid-shaped-wrong-
	// command failure this whole design exists to prevent.
	Out += TEXT("rules:\n");
	Out += TEXT("- Symbols only. Never a coordinate, distance, direction or actor name.\n");
	Out += TEXT("- Ask for the count the player said even if the roster holds fewer; the game reports the shortfall.\n");
	Out += TEXT("- One order in, one command out. You never see an earlier turn.\n");
	Out += TEXT("- If the order is not one of the seven intents, return a question instead of guessing.\n");
	Out += TEXT("\n");

	// The synonym table is the ONLY part of Zone A that varies, and it varies
	// only with DA_AssistantVocabulary - a content-lock-time change, never a
	// per-request one. CALLER CONTRACT: pass the same object every turn.
	Out += TEXT("synonyms:\n");
	if (Vocabulary)
	{
		FString SynonymTable = Vocabulary->BuildSynonymTable();
		if (SynonymTable.IsEmpty())
		{
			Out += TEXT("none\n");
		}
		else
		{
			if (!SynonymTable.EndsWith(TEXT("\n")))
			{
				SynonymTable += TEXT("\n");
			}
			Out += SynonymTable;
		}
	}
	else
	{
		Out += TEXT("none\n");
	}
	Out += TEXT("\n");

	// THREE few-shots (the count is pinned by CONVENTIONS §8), chosen to cover
	// the three `who` SHAPES rather than three verbs - which is what a small
	// model actually generalises from:
	//   1. a multi-kind selection array (the feature's flagship sentence)
	//   2. a single-kind array using the "all" count sentinel
	//   3. who == "none" for an army-wide verb, with where == "none" too
	// The fourth shape - the {"ask":...} question branch - is taught by the
	// schema block above rather than by an example, because the example count is
	// pinned. If TASK-413's bar-#5 run shows the model never declining, a fourth
	// shot is the first thing to try: Zone A changes are content-lock-time and
	// cost one cache warm-up, not a code change anywhere else.
	Out += TEXT("examples:\n");
	Out += TEXT("order: send ten footmen with a sorcerer to the ancient ground on our side\n");
	Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1}],\"where\":\"ancient_ground_near\",\"when\":\"now\"}\n");
	Out += TEXT("order: all archers guard the middle\n");
	Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":\"all\"}],\"where\":\"mid\",\"when\":\"now\"}\n");
	Out += TEXT("order: everyone attack\n");
	Out += TEXT("{\"intent\":\"charge\",\"who\":\"none\",\"where\":\"none\",\"when\":\"now\"}\n");

	return Out;
}

FString USiegeAssistantSnapshot::BuildZoneB() const
{
	// FOUR FIXED KEYS, ALWAYS ALL FOUR, ALWAYS IN THIS ORDER. Every value is a
	// BAND: a unit taking 1 damage or one income tick must not move a single
	// token here, because any change invalidates the cached prefix from this
	// point on.
	FString Out;
	Out.Reserve(128);

	Out += TEXT("[MATCH]\n");

	if (OwnCastleHPBand == INDEX_NONE)
	{
		Out += TEXT("own_castle_hp: none\n");
	}
	else
	{
		Out.Appendf(TEXT("own_castle_hp: %d%%\n"), OwnCastleHPBand);
	}

	if (EnemyCastleHPBand == INDEX_NONE)
	{
		Out += TEXT("enemy_castle_hp: none\n");
	}
	else
	{
		Out.Appendf(TEXT("enemy_castle_hp: %d%%\n"), EnemyCastleHPBand);
	}

	switch (MidOwner)
	{
	case EMidOwner::Neutral:
		Out += TEXT("mid: neutral\n");
		break;
	case EMidOwner::Own:
		Out += TEXT("mid: ours\n");
		break;
	case EMidOwner::Enemy:
		Out += TEXT("mid: theirs\n");
		break;
	default:
		Out += TEXT("mid: none\n");
		break;
	}

	if (GoldBand == INDEX_NONE)
	{
		Out += TEXT("gold: none\n");
	}
	else
	{
		Out.Appendf(TEXT("gold: %d\n"), GoldBand);
	}

	// Tripwire, not an expectation: four short fixed keys cannot approach the
	// reserve. It fires only if a later task adds a key here without raising
	// ZoneBCharReserve, which would silently steal budget from the roster.
	if (Out.Len() > ZoneBCharReserve && !bWarnedZoneBOverReserve)
	{
		bWarnedZoneBOverReserve = true;
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Zone B is %d chars, over its %d-char reserve - raise ZoneBCharReserve or the roster budget is understated."),
			Out.Len(), ZoneBCharReserve);
	}

	return Out;
}

void USiegeAssistantSnapshot::AppendRosterBlock(FString& Out, int32 KindsToPrint) const
{
	const int32 PrintCount = FMath::Clamp(KindsToPrint, 0, UnitKinds.Num());

	Out += TEXT("roster:\n");
	if (PrintCount == 0)
	{
		Out += TEXT("- none\n");
	}
	else
	{
		for (int32 KindIndex = 0; KindIndex < PrintCount; ++KindIndex)
		{
			// THREE numbers per kind, and the second and third are the point:
			// `orderable` answers "can I send these?", `followable` answers "can
			// these follow?", and they differ for the Cleric. A multi-kind
			// selection ("10 footmen with a sorcerer") needs BOTH kinds'
			// availability in front of the model in ONE turn - that is what
			// stops the clarification loop from opening for a question the
			// snapshot could already answer.
			Out.Appendf(TEXT("- %s: %d total, %d orderable, %d followable\n"),
				*UnitKinds[KindIndex].ToString(),
				KindTotals.IsValidIndex(KindIndex) ? KindTotals[KindIndex] : 0,
				KindOrderable.IsValidIndex(KindIndex) ? KindOrderable[KindIndex] : 0,
				KindFollowable.IsValidIndex(KindIndex) ? KindFollowable[KindIndex] : 0);
		}
	}

	// ALWAYS EMITTED, `none` when nothing was collapsed - the fixed-key law. The
	// tail collapses into ONE line rather than spilling: aggregate harder, never
	// exceed the budget.
	int32 CollapsedKinds = 0;
	int32 CollapsedUnits = 0;
	for (int32 KindIndex = PrintCount; KindIndex < UnitKinds.Num(); ++KindIndex)
	{
		++CollapsedKinds;
		CollapsedUnits += KindTotals.IsValidIndex(KindIndex) ? KindTotals[KindIndex] : 0;
	}

	if (CollapsedKinds == 0)
	{
		Out += TEXT("other_kinds: none\n");
	}
	else
	{
		Out.Appendf(TEXT("other_kinds: %d kinds, %d units\n"), CollapsedKinds, CollapsedUnits);
	}
}

FString USiegeAssistantSnapshot::BuildZoneC(const FString& Utterance, const FString& PendingLine) const
{
	// FIXED KEY ORDER: places, roster, other_kinds, stances, hero, pending,
	// order. Zone C is regenerated every turn by definition (the utterance is in
	// it), so it is the only zone allowed to vary in length - but the KEYS never
	// vary, because the executor and the FSM parse nothing here and a missing key
	// would teach the model that a key is optional.

	// The head and the tail are NEVER trimmed. Only the roster block between them
	// is elastic (CONVENTIONS 8: truncate the roster tail, never the utterance).
	FString Head;
	Head.Reserve(256);
	Head += TEXT("[FORCES]\n");
	Head += TEXT("places: ");
	if (PlaceNames.Num() == 0)
	{
		Head += TEXT("none");
	}
	else
	{
		for (int32 PlaceIndex = 0; PlaceIndex < PlaceNames.Num(); ++PlaceIndex)
		{
			if (PlaceIndex > 0)
			{
				Head += TEXT(", ");
			}
			Head += PlaceNames[PlaceIndex].ToString();
		}
	}
	Head += TEXT("\n");

	FString Tail;
	Tail.Reserve(512);
	Tail.Appendf(TEXT("stances: free %d, following %d, holding %d, ambushing %d\n"),
		StanceFree, StanceFollowing, StanceHolding, StanceAmbushing);

	switch (HeroPresence)
	{
	case EHeroPresence::Alive:
		Tail += TEXT("hero: alive\n");
		break;
	case EHeroPresence::Down:
		Tail += TEXT("hero: down\n");
		break;
	default:
		Tail += TEXT("hero: none\n");
		break;
	}

	// The pending line is GAME-AUTHORED (CONVENTIONS 1): it is how a
	// clarification turn carries context forward WITHOUT ever feeding the model
	// its own previous output. Sanitised anyway - one stray newline here would
	// let a line forge a key.
	const FString SafePending = SanitizeForPrompt(PendingLine);
	Tail.Appendf(TEXT("pending: %s\n"), SafePending.IsEmpty() ? TEXT("none") : *SafePending);

	const FString SafeUtterance = SanitizeForPrompt(Utterance);
	Tail += TEXT("[ORDER]\n");
	Tail.Appendf(TEXT("order: %s\n"), SafeUtterance.IsEmpty() ? TEXT("none") : *SafeUtterance);

	// The elastic middle. Start at the kind cap, then shrink until the whole
	// snapshot (Zone B's reserve + Zone C) fits MaxSnapshotChars. Shrinking is
	// deterministic - always from the TAIL of the fixed card-row order - so the
	// same board always produces the same bytes.
	const int32 RosterBudget = MaxSnapshotChars - ZoneBCharReserve - Head.Len() - Tail.Len();

	int32 KindsToPrint = FMath::Min(UnitKinds.Num(), MaxRosterKinds);
	FString RosterBlock;
	for (;;)
	{
		RosterBlock.Reset();
		AppendRosterBlock(RosterBlock, KindsToPrint);

		if (RosterBlock.Len() <= RosterBudget || KindsToPrint <= 0)
		{
			break;
		}
		--KindsToPrint;
	}

	if (KindsToPrint < FMath::Min(UnitKinds.Num(), MaxRosterKinds) && !bWarnedSnapshotTruncated)
	{
		bWarnedSnapshotTruncated = true;
		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Snapshot over budget: roster trimmed to %d of %d kind(s) to stay within %d chars. Aggregate harder or re-measure MaxSnapshotChars against the spike's real token count."),
			KindsToPrint, UnitKinds.Num(), MaxSnapshotChars);
	}

	return Head + RosterBlock + Tail;
}

FString USiegeAssistantSnapshot::SanitizeForPrompt(const FString& In)
{
	// The prompt layout is LINE-ORIENTED, so a newline in player text could
	// forge a key ("order: hi\nplaces: enemy_castle") and a pasted paragraph
	// could swamp the context. Flatten, collapse, cap.
	FString Out;
	Out.Reserve(FMath::Min(In.Len(), MaxUtteranceChars) + 1);

	bool bPreviousWasSpace = true; // leading whitespace is dropped
	for (const TCHAR Character : In)
	{
		if (Out.Len() >= MaxUtteranceChars)
		{
			break;
		}

		const bool bIsSpace = (Character == TEXT('\n')) || (Character == TEXT('\r'))
			|| (Character == TEXT('\t')) || (Character == TEXT(' '));
		if (bIsSpace)
		{
			if (!bPreviousWasSpace)
			{
				Out.AppendChar(TEXT(' '));
				bPreviousWasSpace = true;
			}
			continue;
		}

		Out.AppendChar(Character);
		bPreviousWasSpace = false;
	}

	Out.TrimEndInline();
	return Out;
}
