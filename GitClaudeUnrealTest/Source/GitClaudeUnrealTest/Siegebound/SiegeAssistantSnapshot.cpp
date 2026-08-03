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

	/**
	 *  THE CUT MARKER (TASK-433 BLOCKER-1). Appended in place of the text
	 *  MaxUtteranceBytes dropped, so a shortened `order:` / `pending:` line says so
	 *  IN THE PROMPT and not only in the log. The full design call - why a marker
	 *  at all, why never a new key, and why this exact wording - is recorded on
	 *  USiegeAssistantSnapshot::SanitizeForPrompt's declaration.
	 *
	 *  Plain editorial English on purpose: Zone A is byte-identical for the life of
	 *  the process, so the model can never be TAUGHT a symbol here. An ellipsis
	 *  plus a bracketed word is what a cut quotation looks like everywhere in
	 *  pretraining, so it needs no teaching.
	 */
	static constexpr TCHAR UtteranceTruncationMarker[] = TEXT(" ...[truncated]");

	/** Compile-time enforcement of this file's ASCII law for the ONE literal that now participates in the byte budget. */
	static constexpr bool IsAsciiLiteral(const TCHAR* Text)
	{
		for (; *Text != TEXT('\0'); ++Text)
		{
			if (static_cast<uint32>(*Text) >= 0x80u)
			{
				return false;
			}
		}
		return true;
	}

	static_assert(IsAsciiLiteral(UtteranceTruncationMarker),
		"The truncation marker must be ASCII: its byte cost is taken as its character count, and a multi-byte glyph would silently overrun MaxUtteranceBytes.");

	/**
	 *  The marker's cost in UTF-8 bytes, DERIVED rather than hand-maintained (a
	 *  transcribed length is the drift defect this file keeps catching elsewhere).
	 *  ASCII by the assert above, so one byte per character; -1 drops the null.
	 */
	static constexpr int32 UtteranceTruncationMarkerBytes = UE_ARRAY_COUNT(UtteranceTruncationMarker) - 1;

	static_assert(UtteranceTruncationMarkerBytes * 4 < USiegeAssistantSnapshot::MaxUtteranceBytes,
		"The cut marker must stay small next to the cap, or a truncated line is mostly marker.");

	/**
	 *  UTF-8 byte length of ONE Unicode code point - the unit the snapshot budget
	 *  is actually spent in. FString stores UTF-16 code units on Windows, so
	 *  FString::Len() is NOT this number for anything outside ASCII (TASK-433
	 *  BLOCKER-2): a CJK glyph is 1 unit and 3 bytes, and an emoji is 2 units
	 *  (a surrogate pair) and 4 bytes.
	 */
	static constexpr int32 Utf8LengthOfCodePoint(uint32 CodePoint)
	{
		if (CodePoint < 0x80u)
		{
			return 1;
		}
		if (CodePoint < 0x800u)
		{
			return 2;
		}
		if (CodePoint < 0x10000u)
		{
			return 3;
		}
		return 4;
	}

	static constexpr bool IsHighSurrogate(uint32 Unit) { return Unit >= 0xD800u && Unit <= 0xDBFFu; }
	static constexpr bool IsLowSurrogate(uint32 Unit)  { return Unit >= 0xDC00u && Unit <= 0xDFFFu; }
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
	// same bytes every turn for the life of the process, so llama_memory_seq_rm
	// keeps the prefix and turn two prefills ~150 tokens instead of ~500.
	// (The symbol was `llama_kv_cache_seq_rm` here until TASK-433 - it exists
	// nowhere in the vendored llama.h; see the header's BuildZoneA comment.)
	// 6144, not 2048: Zone A measured 4314 chars before the accuracy ladder and
	// 5108 at TASK-431's MEASURED run. ⚠️ THIS COMMENT SAID 5111 AND THAT WAS
	// STALE (qa NIT-5) - 5111 was the pre-re-gate draft, before the refusal
	// exemplar's noun got 3 chars shorter. Ladder loop 2 lands it at 5116. The
	// old hint had been undersized since well before rung 1 grew it. This changes
	// no emitted byte - it only stops the builder reallocating three times on a
	// string whose length is fixed and known.
	FString Out;
	Out.Reserve(6144);

	Out += TEXT("[RULES]\n");
	Out += TEXT("Turn ONE Siegebound order into ONE JSON command. Output the JSON object only: no prose, no explanation.\n");
	Out += TEXT("\n");

	// ⚠️ THIS BLOCK IS THE MIRROR OF TASK-417's GBNF, AND IT MUST STAY ONE.
	// Zone A teaches the schema; USiegeAssistantGrammar::Build CONSTRAINS it. If
	// they disagree, constrained decoding fights the few-shots on every token
	// and accuracy (go/no-go bar #5) collapses for a reason no log line names.
	// Verified character-for-character against SiegeAssistantGrammar.cpp's root
	// / command / question / who / selection / item / count / at-least rules and
	// the SiegeAssistantJsonKeys + SiegeAssistantAsk symbol tables:
	//   key order   intent, who, where, when   (JsonObjectOpen + JsonNextKey)
	//   pair keys   "kind" and "n"             (NOT "count" - "count" is the
	//                                           GBNF RULE name, "n" is the JSON
	//                                           key, and they differ)
	//   selection   a JSON ARRAY of 1..3 items, bounded by alternation
	//   who         the array, or "all", or "none"
	//   trigger     JSON key "at_least" (snake_case, below), GBNF rule `at-least`
	//               (kebab-case). ⛔ SAME DIVERGENCE AS "n"/"count" AND FOR A
	//               HARDER REASON: llama.cpp rule names are [a-zA-Z0-9-] only, so
	//               `at_least` as a RULE NAME makes the WHOLE grammar
	//               unparseable - which is what shipped, and what left TASK-413
	//               with bars #2 and #5 unmeasured. THE JSON KEY ON LINE ~580 IS
	//               CORRECT AND MUST NOT BE KEBAB-CASED to "match" the rule.
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

	// ⚠️⚠️ THE FIVE RULES BELOW ARE THE PROSE HALF OF THE ACCURACY LADDER'S RUNG
	// 1. Each is paired with a few-shot further down, because the measured
	// evidence is that NEITHER ALONE IS ENOUGH: the [notes] block has always said
	// an unlisted kind should become a question, and the model shipped a command
	// anyway; conversely `bowmen` was already an alias and still lost to a symbol.
	// A rule states the principle so it reaches sentences nobody wrote down; the
	// example shows the shape. They are deliberately redundant with each other.
	//
	// ⚠️⚠️ LADDER LOOP 2 REWROTE FOUR OF THESE FIVE LINES, AND THE REASON IS A
	// MEASUREMENT, NOT A PREFERENCE. TASK-431 scored the rung-1 wave at 19/25
	// against a gate of 22 - ONE row over baseline - and the row that had been
	// taught TWICE OVER (a near-paraphrase few-shot AND a pinned alias) did not
	// flip. That is evidence that ADDING examples has poor leverage on this
	// model, so loop 2 adds none. What it does instead is (i) sharpen these rules
	// from descriptions into EXECUTABLE CONSTRAINTS - an antecedent the model can
	// test, then one instruction - and (ii) DELETE the prompt lines that were
	// contradicting them. Two deletions landed in the vocabulary's [notes] block;
	// see SiegeAssistantVocabulary.cpp. The net cost of the whole loop is +8
	// chars, because the sharpening is paid for by the deletions.
	//
	// ⚠️ THE FIRST TWO ARE SAFETY, NOT ACCURACY, AND THEY ARE NOT INTERCHANGEABLE
	// WITH THE OTHER THREE. Both refusal classes were measured emitting live orders:
	// a unit that does not exist became a real `sapper` order, and an order about
	// gold became a real mining order. The grammar cannot catch either by
	// construction — both are structurally valid commands that are semantically
	// wrong (CONVENTIONS §1: constrain identity hard, leave quantity soft), and
	// `{"ask":"unsupported"}` has always existed in the grammar with nothing
	// routing to it. A silently-executed refused order is worse than no assistant.
	//
	// ⚠️ AND THEY ARE SCOPED NARROWLY ON PURPOSE — "when unsure, ask" IS NOT
	// WRITTEN HERE AND MUST NOT BE. The scorer counts a question as SKIPPING every
	// asserted field, so a model that learns to ask more often converts rows that
	// currently pass STRICT into rows that pass only LENIENT. STRICT == LENIENT is
	// a property this feature's whole measurement leans on. Teaching refusal for
	// TWO named situations buys the safety class without spending that property.

	// ⚠️⚠️ REWRITTEN AT LOOP 2. The rung-1 wording ("A name that is not a
	// Siegebound unit kind does not exist. Never swap in a listed kind") was
	// MEASURED failing on DEV-04 - and failing informatively. Two things came out
	// of that run and both are in the line below:
	//
	//  1. IT NEVER TOLD THE MODEL WHERE TO LOOK. "a Siegebound unit kind" is an
	//     abstraction the model cannot test; there is no list in this prompt with
	//     that name. The roster under [FORCES] IS that list, and the schema block
	//     above already points `KIND` at it. The rule now points at the same
	//     place, so the antecedent is a membership test the model can actually
	//     run instead of a fact it has to already know.
	//  2. ⚠️ THE SUBSTITUTION IS SLOT-FILLING FROM AN EXAMPLE, NOT A SEMANTIC
	//     NEIGHBOUR. Baseline answered "catapults" with `sapper` (demolition -
	//     the plausible near-miss). After rung 1 it answered `sorcerer`, and
	//     {"kind":"sorcerer","n":1} is a CHARACTER-FOR-CHARACTER SUBSTRING of
	//     few-shot #1's output. The model was not reaching for the nearest unit,
	//     it was copying a slot out of the nearest example. "Never swap in a
	//     listed kind" does not forbid that, because from the model's seat it is
	//     not a swap - nothing was there to swap. The second sentence below
	//     forbids it directly and in the only terms that cover it: the kind has
	//     to have come from the PLAYER.
	//
	// ⚠️ SHIPPED-VS-MEASURED HAZARD, DECLARED: [FORCES] is truncated to
	// MaxRosterKinds (8) in the shipped snapshot but NOT in the spike fixture,
	// which prints all 13. So on a >8-kind board this rule can refuse a kind that
	// is alive and simply got collapsed into `other_kinds`. The prompt already
	// carried that hazard (the [notes] line loop 2 deleted said the same thing);
	// this concentrates it into a rule, which makes it worse, not better. It is
	// board ruling 7's condition biting a second time and it is TASK-416's
	// constant to move, not mine.
	Out += TEXT("- If the unit named is not a kind in [FORCES], answer {\"ask\":\"unsupported\"}. Never write a kind the player did not name.\n");
	// ⛔⛔ THIS LINE IS BYTE-FROZEN AT LOOP 2 AND MUST NOT BE TOUCHED. It is the
	// one taught class TASK-431 measured LANDING: the card-play/economy refusal
	// went from 2-of-3 Refuse rows emitting live orders to 1-of-3, and the row it
	// fixed is the one that was breaking Jonathan's standing ruling that the
	// assistant never spends gold and never plays cards. Loop 2 changed the rule
	// above it and the rule below it; this one keeps its exact bytes AND its exact
	// position in the block, because position is the only other variable a
	// rewrite could have moved.
	Out += TEXT("- Gold, buying and card play are the player's, never yours: {\"ask\":\"unsupported\"}.\n");

	// ⚠️⚠️ THE SELECTION RULE, REWRITTEN AT LOOP 2 - AND THIS IS THE HIGHEST-VALUE
	// LINE IN THE BLOCK, BECAUSE THE OLD ONE WAS NOT MERELY WEAK, IT WAS OUTVOTED.
	//
	// QA rated this class STRONGEST on public evidence. TASK-431 measured it
	// failing TWICE - DEV-07 ("charge with the footmen") and DEV-16 ("infantry
	// back to our castle now"), both collapsing to who:"none". Reading the two
	// outputs next to this prompt says why, and it is not that the model missed
	// the rule. COUNT THE INSTRUCTIONS:
	//
	//   who = "none"   intents block, `charge` line          (1)
	//   who = "none"   intents block, `fallback` line        (2)
	//   who = "none"   intents block, `rally` line           (3)
	//   who = "none"   vocabulary [notes], the intents line  (4)
	//   keep the units the OLD rule here                     (1)
	//
	// ⇒ Once the model latched `charge` off the verb or `fallback` off "back",
	// this prompt ORDERED it to emit who:"none", four times over, and the old
	// rule asked it not to, once. IT WAS OBEYING. The rung-1 handoff counted that
	// four-fold repetition as a saving ("already said twice, and Zone A pays for
	// every repetition") - it was not a saving, it was the competitor.
	//
	// ⚠️ SO THE FIX IS NOT MORE EMPHASIS ON "who", IT IS TO ORDER THE DECISION.
	// The real law is that the SELECTION PICKS THE INTENT, not the verb: if the
	// player named units, an army-wide intent is not available at all, so
	// who:"none" is never reached and there is nothing left to contend with. The
	// line below states exactly that, as a constraint with a testable antecedent,
	// and it names the three forbidden intents explicitly because naming them is
	// the half that beats the verb latch. It supplies the MISSING IMPLICATION
	// DIRECTION: [notes] already says send/guard/ambush/follow take a unit list
	// and charge/fallback/rally take none; nothing anywhere said that naming
	// units therefore RULES OUT the second group. Now something does.
	//
	// ⚠️ IT ALSO FIXES DEV-16 ENTIRELY ON ITS OWN THREE BROKEN FIELDS. That row
	// failed intent, kinds AND counts, and all three are downstream of the single
	// wrong choice of `fallback` - `own_castle` was already right. One rule, one
	// decision, three fields.
	//
	// ⚠️ REPLACES rather than joins the old line, deliberately: the old rule is
	// the one that was measured losing, it states the goal where this states the
	// mechanism, and every row where it would still apply on its own (a
	// unit-taking intent already chosen, e.g. DEV-22) is a row the model already
	// gets right. Keeping both would spend tokens to re-state a lost argument.
	// Few-shot #6 ("i want the footmen to rush" -> send) is this rule's exemplar
	// and already sits in the block, unchanged.
	Out += TEXT("- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.\n");

	// The quantity rule, which is one axis with three ways to fall off it — the
	// model was observed missing in BOTH directions, so a one-sided rule would
	// have traded one failure for its mirror image. "the wizard" became "all",
	// while "clerics follow me" became n:2 — and 2 is the number of clerics ON THE
	// BOARD, i.e. the model read the quantity out of the roster in Zone C instead
	// of out of the sentence. That last clause is the one that generalises: the
	// roster says what EXISTS, never what was ASKED FOR, and reading a count from
	// it also makes the shortfall the rule above deliberately preserves invisible.
	//
	// ⚠️ LOOP 2 REORDERED THE CLAUSES, AND THE REORDER IS THE WHOLE EDIT. The
	// third clause WORKED: DEV-20 stopped answering n:2, so the roster copy is
	// dead. It then answered n:1, which is the OTHER branch of this same rule -
	// so the model reached the rule, read it, and took the wrong arm of it. Two
	// reasons it would: the antecedent ("no number was said") was never stated,
	// it was only implied by the word "bare"; and "Singular = 1" was written
	// FIRST, which is also the arm few-shot #3 demonstrates most recently. The
	// rewrite states the antecedent up front and puts the plural arm first.
	// ⚠️ Confidence here is honestly MEDIUM-LOW - this is a word-order change to
	// a rule the model demonstrably already read, and DEV-17 ("horsemen go hit
	// their castle" -> all) proves it can take the plural arm unaided. If DEV-20
	// still comes back n:1, the finding is that clause ORDER does not steer this
	// model and no further rewording of this line is worth a loop.
	Out += TEXT("- No number said: a plural = \"all\", a singular = 1. Never copy a count from the roster.\n");

	// ⚠️ NEW AT LOOP 2 - AND IT IS A CLASS FIX WHERE RUNG 1 SHIPPED AN INSTANCE.
	// DEV-01 ("...to the nearest ancient ground") resolved to `nearest_mine`, and
	// rung 2 answered it with a pinned alias, `ancient_ground_near <- nearest
	// ancient ground`. ⛔ TASK-431 MEASURED THAT ALIAS FAILING TO FIX ITS OWN
	// EXACT TARGET STRING. An alias is a lookup and this model does not do
	// lookups, it does association - and in association space the token "nearest"
	// was bound to the SYMBOL `nearest_mine`, which the prompt printed THREE
	// times (places block, [places] alias row, [notes] line) against one buried
	// mid-list occurrence of "nearest ancient ground". 3 to 1, and the alias lost.
	//
	// ⛔ THE SYMBOL CANNOT BE RENAMED (CONVENTIONS §9a pins the place set, the
	// corpora assert the spellings), so the only two moves available are to cut
	// the attractor's occurrences and to state the head-noun law as a rule. Loop
	// 2 does both: the redundant [notes] line is deleted in
	// SiegeAssistantVocabulary.cpp (3 occurrences -> 2), and this line states the
	// mechanism - THE NOUN DECIDES, THE MODIFIER ONLY NARROWS.
	//
	// ⚠️ IT IS WORDED TO PROTECT DEV-23, WHICH CURRENTLY PASSES. "guard the
	// nearest mine with 3 footmen" legitimately wants `nearest_mine`, so a rule
	// saying "nearest never picks a place" would have bought DEV-01 and paid for
	// it with DEV-23. Noun-first buys both: nearest MINE -> the mine, nearest
	// ANCIENT GROUND -> the ancient ground. It also generalises to "closest",
	// which rung 1 cut for budget and named as its most likely holdout miss.
	// ⚠️ It deliberately does NOT print `nearest_mine` - writing the wrong answer
	// next to the trigger word is how you feed an attractor, not how you starve
	// one, and the whole point of this line is to stop feeding it.
	Out += TEXT("- Choose a place by its noun - ancient ground, mine, castle, centre. near, nearest and far only say which one.\n");
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

	// SEVEN few-shots. The count was pinned at three by CONVENTIONS §8, and the
	// clause directly below the pin said what would retire it: "if the bar-#5 run
	// shows the model never declining, a fourth shot is the first thing to try."
	// The run happened. The model never declined — 2 of 3 refusal rows came back
	// as executable orders — so the pin is spent exactly as its author intended,
	// under Jonathan's ruling to climb the prompt ladder (rung 1) before reaching
	// for a bigger model. The original three are UNCHANGED and still first: they
	// cover the three `who` SHAPES (multi-kind array / "all" sentinel / "none"),
	// which is what a small model actually generalises from, and every row that
	// passes today passes because of them.
	//
	// The four new ones teach the four MECHANISMS that were measured failing.
	// ⚠️ ORDERING IS LOAD-BEARING TWICE OVER, so nothing here may be resorted:
	//   · #3 sits immediately after #2 to form a MINIMAL PAIR — same verb, same
	//     kind, and the ONLY difference is the determiner, which flips the count
	//     from "all" to 1. A minimal pair is the cheapest way to teach a small
	//     model a distinction, because everything except the distinction is held
	//     constant across two adjacent lines.
	//   · #6 sits immediately before #7 to form the second one — units named
	//     (keep them) against no units named (charge with "none"). That contrast
	//     IS the failure: the army-wide verb was swallowing the selection.
	//   · The two refusals sit in the MIDDLE and the block still ENDS on a
	//     command. Recency pulls a small model toward the last example it read,
	//     and a block ending in {"ask":...} would raise refusals on rows that must
	//     execute — trading the accuracy bar for the safety one instead of buying
	//     both. Five of seven remain commands for the same reason.
	//
	// ⚠️ §9c SEAM — DELIBERATELY NOT WIDENED. The emitted JSON below names only
	// `footman`, `sorcerer` and `archer`: exactly the three symbols the original
	// three examples already named, and not one more. Zone A is byte-identical for
	// process life while the grammar's KIND alternatives come from the LIVE
	// roster, so any kind named here is a symbol the sampler may forbid on a board
	// that lacks it — with no log line saying so. That seam is open and is
	// TASK-433's to read; this task refuses to make it wider for the sake of
	// pedagogical variety. Note the two refusals are seam-FREE by construction:
	// their sentences mention units ("werewolves", "pikemen") but their OUTPUT
	// names no kind at all, so an input word can never become a forbidden symbol.
	Out += TEXT("examples:\n");
	Out += TEXT("order: send ten footmen with a sorcerer to the ancient ground on our side\n");
	Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1}],\"where\":\"ancient_ground_near\",\"when\":\"now\"}\n");
	Out += TEXT("order: all archers guard the middle\n");
	Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":\"all\"}],\"where\":\"mid\",\"when\":\"now\"}\n");

	// (d) DETERMINER -> QUANTITY. The minimal pair with the line above. Also the
	// only example that emits `own_castle`, which is the canonical symbol that has
	// already cost two QA loops in its `my_castle` spelling.
	Out += TEXT("order: the archer guards our castle\n");
	Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":1}],\"where\":\"own_castle\",\"when\":\"now\"}\n");

	// (a) OUT-OF-ROSTER REFUSAL. The place is deliberately unambiguous ("the
	// middle" is a real alias of `mid`): the lesson is that the refusal is decided
	// by the UNIT ALONE and a perfectly parseable remainder does not rescue it —
	// which is precisely how the measured failure went wrong, snapping an unknown
	// siege engine onto the nearest listed kind and shipping the rest verbatim.
	// The unit named is NOT a siege engine, on purpose: the class is "not in the
	// roster", not "siege engines are refused", and a siege-engine exemplar would
	// have taught the narrower rule. It is also a BARE SINGLE-WORD PLURAL on
	// purpose: a <modifier> <noun> invented unit carries a second, unwanted lesson
	// ("the modifier is what disqualified it"), which is the same narrowing the
	// no-siege-engine choice exists to avoid. And it keeps a plausible pull toward
	// a listed kind (ogre <- brute, brutes, giant, giants), so the shot teaches
	// refusal UNDER TEMPTATION rather than refusal of something obviously alien —
	// and temptation is the measured failure: an unknown noun snapping onto the
	// nearest listed kind.
	//
	// ⛔⛔ DO NOT RE-AUTHOR THIS NOUN FROM IMAGINATION. An earlier draft named a
	// different invented unit, and the TASK-430 gate found it collided LITERALLY
	// with a row in the SEALED holdout corpus — a file this task may not open and
	// did not open. Neither side was careless: two agents given the same public
	// brief ("a non-existent unit, and NOT a siege engine") drew from the same
	// small pool of salient fantasy units and converged on the same word, and
	// NEITHER COULD SEE IT. Only the one pass allowed to open both sides could.
	// The noun below was cleared against an explicit absence certificate covering
	// all three corpus files. Any future change to it must be cleared the same
	// way, by that same pass; changing it "for variety" re-runs the experiment
	// that produced the collision. ⚠️ The retired word is deliberately NOT named
	// here — naming it would write a fact about the SEALED file into the source
	// tree, which is the seal leaking by a different door.
	//
	// ⚠️⚠️ LOOP 2 CHANGED THE FRAME AND NOT THE NOUN, AND THE SPLIT IS DELIBERATE.
	// ⛔ THE NOUN IS FROZEN: re-authoring it from imagination is the exact
	// experiment that produced the TASK-430 collision, and this pass cannot get a
	// fresh absence certificate because it may not open either holdout. It stays.
	//
	// The FRAME is a different matter, and TASK-431 measured it as a defect. The
	// comment above claims this shot teaches that "a perfectly parseable remainder
	// does not rescue an impossible subject" - but as rung 1 wrote it, the shot
	// had NO VERB, so it never contained a parseable remainder to be rescued by.
	// It did not demonstrate the lesson it was designed around. Meanwhile the
	// measured failure (DEV-04) arrives in `send X at Y`, and `send X to Y` was
	// the frame of exactly ONE example in this block - few-shot #1 - which is a
	// COMMAND. So every send-framed sentence in the corpus pattern-matched to the
	// one send-framed exemplar and inherited its shape, which is precisely what
	// the {"kind":"sorcerer","n":1} slot-fill looks like. Adding `send` here puts
	// a REFUSAL in the dominant frame, so the frame stops deciding and the unit
	// word starts: send + a listed kind -> command (#1), send + a word that is
	// not a kind -> {"ask":"unsupported"} (here). That contrast IS the lesson.
	//
	// ⚠️ It stays a BARE plural ("send werewolves", not "send the werewolves") on
	// purpose - the determiner rule must still lose to the refusal, which is what
	// the bare form tested at rung 1, and the bare form is also one token further
	// from the dev sentence. Word-overlap against DEV-04 is 0.22 (Jaccard), inside
	// the 0.25 band the whole block was cleared at; no burned surface form is used.
	Out += TEXT("order: send werewolves to the middle\n");
	Out += TEXT("{\"ask\":\"unsupported\"}\n");

	// (b) ECONOMY / CARD-PLAY REFUSAL — the highest-value line in this block.
	// It names a kind that IS on the roster (`pikemen`), because the failure this
	// teaches against is not "unknown word" but "the object is real, so the model
	// finds a legal-looking order for it". It also contains the word "gold" while
	// resolving to a refusal, which is the second half of removing the bare `gold`
	// place alias in SiegeAssistantVocabulary.cpp: the alias made "gold" pull
	// toward a mine, and this line makes it pull toward a refusal instead.
	Out += TEXT("order: get two more pikemen with our gold\n");
	Out += TEXT("{\"ask\":\"unsupported\"}\n");

	// (c) SELECTION-PRESERVING. "rush" is a listed `charge` alias, so the sentence
	// contains a genuine army-wide cue AND a named selection — the exact conflict
	// that was resolving as who:"none". The selection wins and the place, which
	// the player never gave, becomes "none" rather than being invented; that is
	// the same shape the model already produces correctly when a destination is
	// unrecognised, so this teaches a reuse of a behaviour it has, not a new one.
	Out += TEXT("order: i want the footmen to rush\n");
	Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":\"all\"}],\"where\":\"none\",\"when\":\"now\"}\n");

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
	//
	// ⚠️ BOTH LINES NOW REPORT THEIR OWN TRUNCATION (TASK-433 BLOCKER-1). The cap
	// used to cut mid-word with no log, no latch and no marker - which made the two
	// strings that come from OUTSIDE this file the only truncations in the whole
	// snapshot that nothing observed. Each line carries its own latch: they mean
	// different things and are fixed in different places, so a deep cut on one must
	// not be able to hide behind a deeper cut on the other.
	int32 PendingFlattenedBytes = 0;
	const FString SafePending = SanitizeForPrompt(PendingLine, PendingFlattenedBytes);
	ReportLineTruncation(TEXT("pending"), PendingFlattenedBytes, WarnedPendingBytes);
	Tail.Appendf(TEXT("pending: %s\n"), SafePending.IsEmpty() ? TEXT("none") : *SafePending);

	int32 UtteranceFlattenedBytes = 0;
	const FString SafeUtterance = SanitizeForPrompt(Utterance, UtteranceFlattenedBytes);
	ReportLineTruncation(TEXT("order"), UtteranceFlattenedBytes, WarnedUtteranceBytes);
	Tail += TEXT("[ORDER]\n");
	Tail.Appendf(TEXT("order: %s\n"), SafeUtterance.IsEmpty() ? TEXT("none") : *SafeUtterance);

	// The elastic middle. Start at the kind cap, then shrink until the whole
	// snapshot (Zone B's reserve + Zone C) fits MaxSnapshotChars. Shrinking is
	// deterministic - always from the TAIL of the fixed card-row order - so the
	// same board always produces the same bytes.
	const int32 RosterBudget = MaxSnapshotChars - ZoneBCharReserve - Head.Len() - Tail.Len();

	const int32 CapKinds = FMath::Min(UnitKinds.Num(), MaxRosterKinds);

	int32 KindsToPrint = CapKinds;
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

	// ⚠️ THE TEST IS "DID ANY KIND FAIL TO PRINT IN FULL", NOT "DID THE CHARACTER
	// BUDGET BITE" (TASK-419 WARN-5, closed by TASK-413). The old condition
	// compared KindsToPrint against the cap, and those are EQUAL at exactly the
	// cap - so the common case, a >MaxRosterKinds board collapsing its tail into
	// `other_kinds:`, logged nothing at all. That is the case that costs
	// accuracy: GetUnitKinds() is never truncated, so the GRAMMAR still admits
	// every collapsed kind and the sampler can name a unit the prompt did not
	// show the model. Silence is the worst possible way for that to happen, and
	// it is strictly more likely now that MaxSnapshotChars binds at a measured
	// budget rather than a generous guess.
	const int32 CollapsedKinds = UnitKinds.Num() - KindsToPrint;
	if (CollapsedKinds > 0)
	{
		// The two causes are named separately because they call for different
		// fixes: the kind cap is a deliberate aggregation policy, whereas a budget
		// bite means the snapshot genuinely does not fit and something upstream
		// has to aggregate harder.
		const bool bBudgetBite = KindsToPrint < CapKinds;
		const TCHAR* const Cause = bBudgetBite
			? TEXT("the CHARACTER BUDGET, below the MaxRosterKinds cap")
			: TEXT("the MaxRosterKinds cap");

		// Unlatched per-turn record. Verbose costs nothing in normal play and is
		// what makes a degraded turn RECONSTRUCTABLE afterwards - the Warning
		// below deliberately fires once per escalation, so it cannot tell you
		// which particular sentence was answered against a trimmed roster.
		UE_LOG(LogSiegeAssistant, Verbose,
			TEXT("Snapshot roster degraded: %d of %d kind(s) printed in full, %d collapsed into `other_kinds:` by %s (roster %d chars of a %d-char budget; MaxSnapshotChars %d)."),
			KindsToPrint, UnitKinds.Num(), CollapsedKinds, Cause,
			RosterBlock.Len(), RosterBudget, MaxSnapshotChars);

		if (KindsToPrint < WarnedRosterKindsPrinted || CollapsedKinds > WarnedRosterKindsCollapsed)
		{
			WarnedRosterKindsPrinted = FMath::Min(WarnedRosterKindsPrinted, KindsToPrint);
			WarnedRosterKindsCollapsed = FMath::Max(WarnedRosterKindsCollapsed, CollapsedKinds);

			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Snapshot roster TRUNCATED: %d of %d kind(s) printed in full, %d collapsed into `other_kinds:` by %s. The grammar still admits all %d kinds, so the model can name a unit the prompt never showed it. Roster %d chars of a %d-char budget (MaxSnapshotChars %d, ZoneBCharReserve %d). Aggregate harder or re-measure the cap - do NOT raise it to hide this."),
				KindsToPrint, UnitKinds.Num(), CollapsedKinds, Cause, UnitKinds.Num(),
				RosterBlock.Len(), RosterBudget, MaxSnapshotChars, ZoneBCharReserve);
		}
	}

	return Head + RosterBlock + Tail;
}

FString USiegeAssistantSnapshot::SanitizeForPrompt(const FString& In, int32& OutFlattenedBytes)
{
	using namespace SiegeAssistantSnapshotInternal;

	// The prompt layout is LINE-ORIENTED, so a newline in player text could
	// forge a key ("order: hi\nplaces: enemy_castle") and a pasted paragraph
	// could swamp the context. Flatten, collapse, cap - and, since TASK-433, SAY SO
	// when the cap bites, on both channels: the log and the prompt itself.
	//
	// ⚠️ TWO PASSES, AND THE SPLIT IS THE POINT. The old single pass stopped the
	// instant the output reached the cap, so it could never know how much text
	// there had actually been - which is both the number that makes a truncation
	// report worth reading and the magnitude the escalating latch compares. The
	// extra walk is over one line a human typed or pasted, which is nothing beside
	// the inference call this prompt is being built for.

	// ── PASS 1 - FLATTEN, UNCAPPED ──
	FString Flattened;
	Flattened.Reserve(In.Len() + 1);

	bool bPreviousWasSpace = true; // leading whitespace is dropped
	for (int32 Index = 0; Index < In.Len(); ++Index)
	{
		const TCHAR Character = In[Index];

		const bool bIsSpace = (Character == TEXT('\n')) || (Character == TEXT('\r'))
			|| (Character == TEXT('\t')) || (Character == TEXT(' '));
		if (bIsSpace)
		{
			if (!bPreviousWasSpace)
			{
				Flattened.AppendChar(TEXT(' '));
				bPreviousWasSpace = true;
			}
			continue;
		}

		// ⚠️ A SURROGATE PAIR MOVES AS ONE (TASK-433 BLOCKER-2, second half). An
		// emoji is TWO FString code units and ONE code point; carrying them
		// together here is what lets pass 2's cut land only on code-point
		// boundaries, so the prompt can never be handed half a character.
		const uint32 Unit = static_cast<uint32>(Character);
		if (IsHighSurrogate(Unit) && (Index + 1) < In.Len() && IsLowSurrogate(static_cast<uint32>(In[Index + 1])))
		{
			Flattened.AppendChar(Character);
			Flattened.AppendChar(In[Index + 1]);
			++Index;
			bPreviousWasSpace = false;
			continue;
		}

		if (IsHighSurrogate(Unit) || IsLowSurrogate(Unit))
		{
			// An UNPAIRED surrogate is not text and has no UTF-8 encoding at all.
			// Dropping it is the only option that leaves the line convertible;
			// forwarding it would hand the downstream UTF-8 conversion something it
			// can only guess about. Unreachable from ASCII input, so this cannot
			// move any measured figure.
			continue;
		}

		Flattened.AppendChar(Character);
		bPreviousWasSpace = false;
	}

	Flattened.TrimEndInline();

	// ── PASS 2 - MEASURE IN UTF-8 BYTES, THE UNIT THE BUDGET IS ACTUALLY SPENT IN ──
	// ⚠️ FString::Len() counts UTF-16 CODE UNITS, and that equals the byte count
	// ONLY for ASCII - which is exactly the assumption BLOCKER-2 was about, because
	// the 2.71 chars/token ratio behind MaxSnapshotChars was calibrated on ASCII
	// while these two lines are the only ones a player can fill with anything else.
	// Every high surrogate reaching here is paired, by construction in pass 1.
	const int32 ContentBudgetBytes = MaxUtteranceBytes - UtteranceTruncationMarkerBytes;

	OutFlattenedBytes = 0;
	int32 CutIndex = INDEX_NONE;

	for (int32 Index = 0; Index < Flattened.Len(); )
	{
		uint32 CodePoint = static_cast<uint32>(Flattened[Index]);
		int32 UnitsConsumed = 1;

		if (IsHighSurrogate(CodePoint) && (Index + 1) < Flattened.Len())
		{
			const uint32 LowUnit = static_cast<uint32>(Flattened[Index + 1]);
			CodePoint = 0x10000u + ((CodePoint - 0xD800u) << 10) + (LowUnit - 0xDC00u);
			UnitsConsumed = 2;
		}

		const int32 CodePointBytes = Utf8LengthOfCodePoint(CodePoint);

		// The LAST boundary that still leaves room for the marker. Recorded on the
		// way past rather than searched for afterwards, so the whole measurement is
		// one walk.
		if (CutIndex == INDEX_NONE && (OutFlattenedBytes + CodePointBytes) > ContentBudgetBytes)
		{
			CutIndex = Index;
		}

		OutFlattenedBytes += CodePointBytes;
		Index += UnitsConsumed;
	}

	if (OutFlattenedBytes <= MaxUtteranceBytes)
	{
		// ⚠️ THE PATH THAT MUST NOT MOVE. For an all-ASCII line - which is every
		// line any measured figure in this file was taken from - this returns
		// EXACTLY what the pre-TASK-433 sanitiser returned, byte for byte: the
		// flattening rules above are unchanged, and the old cap could never fire
		// below 240 output units either.
		return Flattened;
	}

	// ── TRUNCATING ──
	// ⚠️ THE MARKER'S BYTES COME OUT OF THE CAP, NEVER ON TOP OF IT. A capped line
	// is therefore at most MaxUtteranceBytes exactly as it was before TASK-433, so
	// Zone C's tail, the roster budget derived from it and every figure computed
	// off those are all untouched. Paying for visibility out of the payload is the
	// whole reason this is affordable.
	FString Out = Flattened.Left((CutIndex == INDEX_NONE) ? 0 : CutIndex);
	Out.TrimEndInline();
	Out += UtteranceTruncationMarker;

	return Out;
}

void USiegeAssistantSnapshot::ReportLineTruncation(const TCHAR* LineKey, int32 FlattenedBytes, int32& WarnedBytes) const
{
	using namespace SiegeAssistantSnapshotInternal;

	if (FlattenedBytes <= MaxUtteranceBytes)
	{
		return;
	}

	const int32 OverByBytes = FlattenedBytes - MaxUtteranceBytes;
	const int32 KeptTextBytes = MaxUtteranceBytes - UtteranceTruncationMarkerBytes;

	// Unlatched per-turn record, the same shape the roster collapse uses: the
	// Warning below fires once per escalation, so it can never tell you WHICH
	// sentence was answered against a cut line.
	UE_LOG(LogSiegeAssistant, Verbose,
		TEXT("Snapshot `%s:` line cut: %d flattened UTF-8 byte(s), %d over the %d-byte MaxUtteranceBytes cap; the model sees at most a %d-byte prefix followed by `%s`."),
		LineKey, FlattenedBytes, OverByBytes, MaxUtteranceBytes, KeptTextBytes, UtteranceTruncationMarker);

	// Escalating, never one-shot - a new and DEEPER cut can never hide behind an
	// earlier, milder one, and a steady state still logs once.
	if (FlattenedBytes > WarnedBytes)
	{
		WarnedBytes = FlattenedBytes;

		UE_LOG(LogSiegeAssistant, Warning,
			TEXT("Snapshot `%s:` line TRUNCATED by the MaxUtteranceBytes cap (%d): %d flattened UTF-8 byte(s), %d over. The model sees at most a %d-byte prefix plus `%s`. On `order:` this is the player's own sentence - the one string in the snapshot that comes from outside the program - and the grammar still produces a well-formed command from half of it, which is the valid-shaped-wrong-command failure CONVENTIONS 1 exists to prevent. On `pending:` it is the FSM's carried clarification state, which nothing downstream can recover. Shorten the line upstream; do NOT raise the cap to hide this - it is a proxy for the 400-token budget, and TASK-423 replaces it with the tokenizer."),
			LineKey, MaxUtteranceBytes, FlattenedBytes, OverByBytes, KeptTextBytes, UtteranceTruncationMarker);
	}
}
