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

		/**
		 *  ⚖️ THE REGION-BEARING COLUMN (TASK-547; CONVENTIONS AS-§21.4). TRUE IFF A
		 *  SHIPPED `IsPointInZone` ANSWERS FOR THIS PLACE - that is the whole test,
		 *  and it is a ruling rather than a judgement call.
		 *
		 *  THREE OF THE SEVEN QUALIFY: `mid` (ACaptureZone::IsPointInZone) and both
		 *  ancient grounds (AAncientGround::IsPointInZone) - the same 2D XY box
		 *  idiom, deliberately.
		 *
		 *  ⛔ THE OTHER FOUR ARE `false` AND AN AGENT MAY NOT "FIX" THAT. Castle.h
		 *  carries NO radius property, AGoldNode's radius is a CALLER's parameter and
		 *  the hero's RallyRadius belongs to the Rally ability - so `own_castle`,
		 *  `enemy_castle`, `nearest_mine` and `hero` would each need an INVENTED
		 *  NUMBER. "How far from the castle counts as AT the castle?" is a product
		 *  question with no measured answer, and two of the four MOVE (the mine is
		 *  "the best mine for the player NOW"; the hero moves every frame), so their
		 *  region would shift under the player mid-sentence. AS-§21.4: if Jonathan
		 *  wants castle-region selection he supplies the number - it is his call, not
		 *  a tuner's.
		 *
		 *  ⛔ ZONE A NEVER PRINTS THIS COLUMN, so adding it moved ZERO Zone-A bytes.
		 *  The `places` block prints Symbol + Description and nothing else; the
		 *  `ZONE =` line prints the SYMBOLS this column selects, never the column.
		 */
		bool bHasRegion;
	};

	/**
	 *  ⚠️ `constexpr`, NOT `const` (TASK-547) - AND THE KEYWORD IS LOAD-BEARING
	 *  RATHER THAN TIDYING. It is what lets RegionBearingPlaceCount() below COUNT
	 *  the region-bearing rows AT COMPILE TIME, so the "no region-bearing place
	 *  exists" state - which would emit a `ZONE = ` line with nothing after the
	 *  colon - is caught by a static_assert instead of by a model reading a
	 *  dangling metavariable. Every initialiser here was already a constant
	 *  expression (string literals + bools), so NOT ONE EMITTED BYTE MOVES and the
	 *  table's storage is unchanged.
	 */
	static constexpr FPlaceDefinition PlaceVocabulary[] =
	{
		{ TEXT("own_castle"),          TEXT("the player's castle"),                     false },
		{ TEXT("enemy_castle"),        TEXT("the enemy castle"),                        false },
		{ TEXT("mid"),                 TEXT("the capturable centre zone"),              true  },
		{ TEXT("ancient_ground_near"), TEXT("the ancient ground on the player's side"), true  },
		{ TEXT("ancient_ground_far"),  TEXT("the ancient ground on the enemy side"),    true  },
		{ TEXT("nearest_mine"),        TEXT("the best gold mine for the player now"),   false },
		{ TEXT("hero"),                TEXT("where the player's hero stands"),          false }
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

	/**
	 *  How many rows carry a region primitive - COUNTED FROM THE TABLE, never
	 *  transcribed. A transcribed "3" is the drift defect this file keeps catching
	 *  elsewhere (see UtteranceTruncationMarkerBytes for the same idiom).
	 */
	static constexpr int32 RegionBearingPlaceCount()
	{
		int32 Count = 0;
		for (const FPlaceDefinition& Place : PlaceVocabulary)
		{
			if (Place.bHasRegion)
			{
				++Count;
			}
		}
		return Count;
	}

	static_assert(RegionBearingPlaceCount() > 0,
		"At least one place must be region-bearing: BuildZoneA generates the `ZONE =` metavariable line from this column, and an empty list would print a metavariable with nothing after the colon - teaching the model a shape it can never fill.");

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
	PlaceHalfExtents.Reset();
	RegionPlaceNames.Reset();
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

	// ⚖️ SNAPSHOT-TIME GEOMETRY, EXECUTION-TIME MEMBERSHIP (TASK-547, AS-§21.4).
	// The BOX is captured here beside the centre; the "is this unit inside it"
	// test runs later, in the executor, against live unit positions. THE UNITS
	// MOVE, THE GROUNDS DO NOT - so a unit that walked out of the ground between
	// this capture and the order landing must not be selected, and it is not.
	//
	// ⛔ THIS COSTS NOT ONE NEW TRAVERSAL. Every actor the boxes come from is
	// ALREADY held by a pass below: NearGround / FarGround (pass 3) and the
	// ACaptureZone (pass 5). Both expose GetZoneHalfExtent() publicly, so this is
	// two extra reads off pointers this function already dereferences. A fresh
	// TActorIterator for regions would be a QA FAIL (AS-§21.4), and so would a
	// registry, a cache, a dirty flag or a subscription list (§4).
	//
	// ⚠️ DECLARED RESIDUAL, STATED RATHER THAN LEFT TO BE FOUND: a ground
	// DESTROYED between capture and execution leaves a stale centre here. That is
	// the IDENTICAL staleness ResolvePlace already carries for the destination
	// (the snapshot holds no actor pointers by design - see the class comment), so
	// it is the same risk profile, not a new one. The mitigation is the same one
	// too: Capture() runs once per typed sentence, so the window is the length of
	// one inference call.
	FVector2D ResolvedHalfExtents[static_cast<int32>(EPlaceSlot::Count)];

	for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(EPlaceSlot::Count); ++SlotIndex)
	{
		ResolvedLocations[SlotIndex] = FVector::ZeroVector;
		ResolvedHalfExtents[SlotIndex] = FVector2D::ZeroVector;
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
		// ⚠️ THE CENTRE IS THE ACTOR LOCATION AND THAT IS NOT A COINCIDENCE TO BE
		// "TIDIED": AAncientGround::IsPointInZone tests a 2D XY box ABOUT THE ACTOR
		// ORIGIN (AncientGround.cpp:143-151), so the pair published here is exactly
		// the pair that predicate uses. Reading the box from anywhere else - a
		// component bounds, a decal size - would answer a different question than
		// the shipped membership test.
		ResolvedHalfExtents[static_cast<int32>(EPlaceSlot::AncientGroundNear)] = NearGround->GetZoneHalfExtent();
		bSlotResolved[static_cast<int32>(EPlaceSlot::AncientGroundNear)] = true;
	}

	// The identity guard is for the DEGENERATE map, not the shipped one: a test
	// level with a single ground would otherwise publish two different symbols
	// that resolve to the same spot, and the model would be taught a distinction
	// the world does not have. On the shipped symmetric arena this never fires.
	if (FarGround && FarGround != NearGround)
	{
		ResolvedLocations[static_cast<int32>(EPlaceSlot::AncientGroundFar)] = FarGround->GetActorLocation();
		ResolvedHalfExtents[static_cast<int32>(EPlaceSlot::AncientGroundFar)] = FarGround->GetZoneHalfExtent();
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
		// Same pairing as the grounds above: ACaptureZone::IsPointInZone is a 2D XY
		// box about the actor origin (CaptureZone.cpp:112-118) - a byte-copy of the
		// ancient-ground test, deliberately, so "standing in the zone" reads
		// identically for both zone actors.
		ResolvedHalfExtents[static_cast<int32>(EPlaceSlot::Mid)] = Zone->GetZoneHalfExtent();
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
			const FName Symbol(PlaceVocabulary[SlotIndex].Symbol);

			PlaceNames.Add(Symbol);
			PlaceLocations.Add(ResolvedLocations[SlotIndex]);
			PlaceHalfExtents.Add(ResolvedHalfExtents[SlotIndex]);

			// THE REGION LIST IS THE SUBSET, NOT A SECOND VOCABULARY: a symbol
			// reaches it only by ALSO being in PlaceNames, so the two can never
			// disagree about what exists this match.
			//
			// ⚠️ A REGION-BEARING PLACE WHOSE ACTOR IS ABSENT THIS MATCH IS SIMPLY
			// NOT HERE, AND THAT IS THE CORRECT DEGRADATION: the grammar generates
			// its `zone` alternation from this array, so the model cannot even
			// SPELL a region the world does not have (AS-§21.4 - the same rule that
			// keeps `where` honest).
			//
			// ⛔ AND A DEGENERATE BOX IS DROPPED RATHER THAN PUBLISHED. ZoneHalfExtent
			// is an instance-editable tunable (default (840,840)); at (0,0) the
			// shipped IsPointInZone answers TRUE only for a point exactly on the
			// actor origin, so the region would be offerable, sampleable, and then
			// contain nobody - a refusal the player cannot act on. Dropping it here
			// makes the shape unsayable instead, which is the same fail-closed
			// direction AS-§21.5 takes everywhere else: a region named and not
			// resolved is a refusal, NEVER an unfiltered order.
			if (PlaceVocabulary[SlotIndex].bHasRegion
				&& ResolvedHalfExtents[SlotIndex].X > 0.f
				&& ResolvedHalfExtents[SlotIndex].Y > 0.f)
			{
				RegionPlaceNames.Add(Symbol);
			}
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

			// TASK-441: THE SAME ALREADY-COMPUTED PREDICATE RESULT, RECORDED ON THE
			// ROSTER ROW TOO - no second call, no second traversal, no re-tally.
			// The per-KIND column (KindOrderable) answers the FSM's "may I send
			// these?"; the per-ROW column is what makes the pure guard
			// ValidateCommandAgainstSnapshot able to tell KindNotOrderable from
			// KindUnknown FROM THE ROSTER ARRAY ALONE, which is the property that
			// keeps it testable with no world and no model.
			// `Entry` is still valid here: nothing between its assignment above and
			// this line touches `Roster`, and only `Tallies` has grown. ⚠️ Anyone
			// who adds a Roster insertion between the two must re-fetch Entry.
			++Entry->Orderable;
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

	// ⚠️ THE REGION COUNT IS ON THIS LINE DELIBERATELY (TASK-547) AND IT IS THE
	// ONLY ADDITION TO IT. An EMPTY region list turns the whole spatial-selection
	// feature OFF SILENTLY - the grammar then emits no `inplace` alternative, so
	// the shape is unsampleable and nothing anywhere says why. One %d on a line
	// that already reports the other three published vocabularies is the cheapest
	// artifact that can answer "did the snapshot publish any regions?".
	// ⛔ It stays at Verbose with its neighbours: this fires on EVERY typed
	// sentence, and TASK-542 owns the one line that had to be promoted to Log.
	UE_LOG(LogSiegeAssistant, Verbose,
		TEXT("Snapshot: %d kind(s), %d roster row(s), %d place(s) (%d region-bearing), gold band %d."),
		UnitKinds.Num(), Roster.Num(), PlaceNames.Num(), RegionPlaceNames.Num(), GoldBand);
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

bool USiegeAssistantSnapshot::ResolvePlaceRegion(FName Place, FVector& OutCentre, FVector2D& OutHalfExtent) const
{
	// THE REGION LIST IS THE AUTHORITY, NOT PlaceNames - and asking it FIRST is
	// what makes the four non-region places fail here. `own_castle` resolves
	// perfectly well as a DESTINATION and has no box at all; answering it with the
	// castle's location and a zero extent would be a silent wrong answer wearing a
	// `true`.
	const int32 Index = RegionPlaceNames.Contains(Place) ? PlaceNames.IndexOfByKey(Place) : INDEX_NONE;

	// The three arrays are parallel by construction (one append site, one loop),
	// exactly like UnitKinds / KindOrderable. The IsValidIndex pair is not
	// ceremony: it is the same defensive shape the accessors above use, and it
	// means a future edit that desynchronises them degrades to "not a region"
	// instead of reading off the end.
	if (Index == INDEX_NONE || !PlaceLocations.IsValidIndex(Index) || !PlaceHalfExtents.IsValidIndex(Index))
	{
		// BOTH OUT-PARAMS ARE LEFT UNTOUCHED ON FAILURE, mirroring ResolvePlace
		// rather than ValidateCommandAgainstSnapshot - and the choice is the same
		// one, for the same reason. A caller that ignores the return value keeps
		// its own initialised box; whatever that box is, it selects a set the
		// caller already knew about. ⛔ There is no "whole map" default to fall
		// back to, and inventing one would turn "send everyone in the mid" into
		// "send everyone" - the one open-failure mode AS-§21.5 forbids outright.
		return false;
	}

	OutCentre = PlaceLocations[Index];
	OutHalfExtent = PlaceHalfExtents[Index];
	return true;
}

// ---------------------------------------------------------------------------
// ORDERABILITY ACCESSORS (TASK-441)
//
// ⛔ THESE READ. THEY DO NOT COMPUTE. KindOrderable is filled once per Capture,
// in the SAME per-unit loop that fills KindTotals and KindFollowable, from the
// SHIPPING predicate ASummonedUnit::IsGroupCommandEligible(). Nothing here
// re-derives eligibility, adds a traversal, or caches anything across sentences
// (CONVENTIONS "In-match LLM command assistant" §4 - a registry for this is
// rejected on sight).
// ---------------------------------------------------------------------------

int32 USiegeAssistantSnapshot::GetOrderableCount(FName Kind) const
{
	// UnitKinds is parallel to KindOrderable by construction (both are appended
	// in one pass over the sorted tallies). The IsValidIndex guard is not
	// ceremony: it is the same defensive shape AppendRosterBlock already uses on
	// these arrays, and it means a future edit that desynchronises them degrades
	// to "not orderable" instead of reading off the end.
	const int32 Index = UnitKinds.IndexOfByKey(Kind);
	if (Index == INDEX_NONE || !KindOrderable.IsValidIndex(Index))
	{
		return 0;
	}

	return KindOrderable[Index];
}

bool USiegeAssistantSnapshot::IsKindOrderable(FName Kind) const
{
	return GetOrderableCount(Kind) > 0;
}

// ---------------------------------------------------------------------------
// THE NON-ORDERABLE-KIND GUARD (TASK-441)
//
// ⛔ SHIPPED SAFETY, NOT A ROUTE TO THE GATE. The eval scores the model's
// EMITTED JSON; this refuses an EXECUTED ACTION. It does not make DEV-04 pass
// and it is not accuracy progress. See the header for the full argument.
// ---------------------------------------------------------------------------

namespace SiegeAssistantGuardInternal
{
	/**
	 *  The three verbs `IsGroupCommandEligible()` actually gates, and therefore
	 *  the only ones the ORDERABLE column may refuse.
	 *
	 *  ⚠️ THIS IS NARROWER THAN SiegeAssistantIntentTakesSelection(), ON PURPOSE.
	 *  That predicate also returns true for `Follow`, which is gated by the OTHER
	 *  shipped predicate, `IsFollowCommandEligible()`. The Cleric is the live
	 *  counter-example: it follows and cannot take zone orders, so its roster row
	 *  is `Count > 0, Orderable == 0`, and refusing `follow` on that column would
	 *  refuse "clerics follow me" - a legal shipped order and the eval's own
	 *  DEV-20 utterance. Reusing the wider predicate here would turn a guard into
	 *  a regression, which is why this is spelled out rather than inlined.
	 *
	 *  Executor seam it mirrors (CONVENTIONS §8): send / guard -> CreateUnitGroup(Hold),
	 *  ambush -> CreateUnitGroup(Ambush); follow -> EnrollInDefaultFollowGroup;
	 *  charge / fallback -> SetUnitCommand; rally -> AHeroCharacter::Rally().
	 */
	static bool IntentTakesZoneOrder(ESiegeAssistantIntent Intent)
	{
		return Intent == ESiegeAssistantIntent::Send
			|| Intent == ESiegeAssistantIntent::Guard
			|| Intent == ESiegeAssistantIntent::Ambush;
	}
}

bool ValidateCommandAgainstSnapshot(const FSiegeAssistantCommand& Command,
                                    const TArray<FSiegeAssistantRosterEntry>& Roster,
                                    ESiegeAssistantRejectReason& OutReason,
                                    FName& OutOffendingKind)
{
	using namespace SiegeAssistantGuardInternal;

	// ALWAYS WRITTEN, ON EVERY PATH. An untouched reason code is a STALE reason
	// code from the previous sentence, and a refusal template filled from a stale
	// code tells the player something true about a command they are no longer
	// giving. (ResolvePlace goes the other way for the opposite reason - see its
	// comment.)
	OutReason = ESiegeAssistantRejectReason::None;
	OutOffendingKind = NAME_None;

	// `who:"none"` and `who:"all"` BOTH parse to an empty selection, so army-wide
	// verbs and "everything eligible" arrive here with nothing to validate. They
	// are not refused: there is no kind to be wrong about.
	if (Command.Kinds.Num() == 0)
	{
		return true;
	}

	const bool bZoneOrder = IntentTakesZoneOrder(Command.Intent);

	for (const FName Kind : Command.Kinds)
	{
		// Sum ACROSS ROWS, never a single row. The roster aggregates by
		// (Kind, GroupId), so one kind legitimately appears once per group it is
		// spread over - a footman in a Hold group and a footman following the hero
		// are two rows of the same symbol. Answering off the first matching row
		// would refuse a kind whose orderable units all sit in the second one.
		int32 LiveUnits = 0;
		int32 OrderableUnits = 0;
		for (const FSiegeAssistantRosterEntry& Row : Roster)
		{
			if (Row.Kind == Kind)
			{
				LiveUnits += Row.Count;
				OrderableUnits += Row.Orderable;
			}
		}

		if (LiveUnits <= 0)
		{
			// Not on the board at all. Capture() never emits a zero-Count row, so
			// in production this means "no row for this symbol"; the count test is
			// what makes a hand-populated or wire-received roster behave the same.
			OutReason = ESiegeAssistantRejectReason::KindUnknown;
			OutOffendingKind = Kind;

			// Log, NOT Warning - and the level is a decision, not a default. A
			// refusal here is the MODEL being wrong, which is expected traffic and
			// not a code defect, so Warning would both cry wolf and (because the
			// automation framework treats logged warnings as failures) make this
			// function untestable by its own tests. Log is on by default, is
			// rate-limited by human typing speed, and names the symbol - which is
			// exactly the line TASK-447's first-execution audit and Jonathan's
			// playtest want to see.
			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Guard REFUSED a command: kind `%s` has no live units on the ordering team (roster has %d row(s)). Surfaced as the existing unsupported-ask outcome. NOTE: this refuses an EXECUTED ACTION and changes nothing about what the model EMITS - shipped safety, not eval accuracy."),
				*Kind.ToString(), Roster.Num());

			return false;
		}

		if (bZoneOrder && OrderableUnits <= 0)
		{
			// THE DEV-04 SHAPE, EXACTLY: a well-formed live order for a kind the
			// roster line itself prints as `orderable=0`.
			OutReason = ESiegeAssistantRejectReason::KindNotOrderable;
			OutOffendingKind = Kind;

			UE_LOG(LogSiegeAssistant, Log,
				TEXT("Guard REFUSED a command: kind `%s` is on the board (%d live) but NONE of them may take a zone order (send/guard/ambush) - IsGroupCommandEligible() is false for every one. Surfaced as the existing unsupported-ask outcome. NOTE: this refuses an EXECUTED ACTION and changes nothing about what the model EMITS - shipped safety, not eval accuracy."),
				*Kind.ToString(), LiveUnits);

			return false;
		}
	}

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
	//
	// ⚠️⚠️ 5116 IS NOW THE *PREVIOUS* SHIPPED LENGTH. TASK-521 added 308 chars to
	// this builder (44 to the `WHO =` schema line + two rule lines at 111 and 153),
	// so the shipped Zone A is 5424 chars / 5424 UTF-8 bytes. The 6144 reserve
	// still covers it with 720 spare, so it is NOT re-tuned.
	//
	// ⚠️⚠️ AND 5424 IS NOW THE PREVIOUS FIGURE IN ITS TURN. TWO TASKS MOVED IT IN
	// THE AI-COMMANDER BATCH AND THE ARITHMETIC IS RECORDED COMPONENT BY COMPONENT
	// SO A TRANSCRIPTION ERROR CANNOT HIDE INSIDE A TOTAL:
	//
	//     5424   the TASK-521 shipped figure
	//       -5   TASK-541, the `defend` note repair - it lands in the VOCABULARY's
	//            [notes] block, which Zone A prints through BuildSynonymTable(), so
	//            it is Zone A's byte even though it is another file's line
	//   ------
	//     5419   the RE-BASED baseline this task measured against
	//      +16   TASK-547 component 1 - `, or {"in":ZONE}` on the `WHO =` line
	//      +76   TASK-547 component 2 - the whole `ZONE   = ` line, incl. newline
	//     +147   TASK-547 component 3 - the in-vs-where rule line, incl. newline
	//   ------
	//     5658   chars / 5658 UTF-8 bytes (ASCII-clean, so the two are equal)
	//
	// The +239 is inside the batch's +250 Zone-A ceiling (AS-§21.7) with 11 chars
	// left for any FUTURE edit. The 6144 reserve still covers the string with 486
	// spare, so it is STILL not re-tuned - it exists to stop the builder
	// reallocating, not to bound the prompt. ⛔ THIS IS A DECLARED
	// DIVERGENCE (`D4`): the SPIKE lane in Plugins/SiegeLlama is NOT touched by
	// this batch (it is TASK-481's in-flight instrument, FT-§16), so the two lanes
	// are no longer byte-equal and Siegebound.Assistant.ZoneA.TwoLaneByteEquality /
	// .MeasuredCharCount are RE-BASED BY TASK-523 against 5424 — ⛔ never by
	// re-copying this builder's output into the fixture. ⚠️ BOTH TESTS GO RED AGAIN
	// AT THIS BATCH AND THAT IS EXPECTED, NOT A REGRESSION: TASK-549 owns the
	// re-base, to 5658, and the spike lane STAYS at its measured 5116 because D4
	// still holds — Plugins/SiegeLlama was not touched by this batch either.
	// ⛔ AND THE TOKEN FIGURES
	// ARE **STALE - PENDING RE-MEASUREMENT ON THE MODEL**: `zoneA_tok = 1139`, the
	// 77.1 % KV-reuse figure and every prefill number derived from them are NOT
	// recomputed by arithmetic and NOT deleted. Only Siege.Llama.SpikePrompt prints
	// them. The CHAR count is re-counted because chars are countable without a
	// model; the TOKEN count is not, and that asymmetry is the whole rule.
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
	//   except      {"all_except":[KIND]}, 1..3 BARE kind strings, bounded the
	//               same way (SiegeAssistantMaxExclusionKinds = 3). ⛔ NO counts
	//               in it, BY DESIGN: "all except 5 archers" has no shape in the
	//               grammar to be sampled into (AS-§20.1).
	//   inplace     {"in":ZONE} - the units STANDING in a region. `zone` is a bare
	//               alternation of the LIVE region-bearing place symbols, so the
	//               JSON is {"in":"mid"}, exactly the shape `where` already emits
	//               for a destination.
	//   who         the array, or the except object, or the in object, or "all",
	//               or "none"
	//
	// ⚠️⚠️ THE `except` ALTERNATIVE IS NEW AT TASK-518 AND THE `WHO =` LINE BELOW
	// HAD TO FOLLOW IT — THAT IS THE MIRROR LAW ABOVE BEING OBEYED, NOT AN EXTRA.
	// TASK-518 added `except` to SiegeAssistantGrammar.cpp's `who` rule while this
	// block still enumerated three shapes. Left alone, Zone A would have TOLD the
	// model the exclusion shape does not exist while the sampler ALLOWED it — the
	// exact "the rule was outvoted by the prompt's own lines" failure loop 2
	// measured on who:"none" (see the selection rule below). The alternative order
	// here is the grammar's order, deliberately: selection | except | "all" | "none".
	//
	// ⚠️⚠️ AND THE SAME THING HAPPENED AGAIN AT TASK-546/547 WITH `inplace`, WHICH
	// IS WHY THE ORDER IS NOW WRITTEN DOWN IN BOTH FILES RATHER THAN INFERRED.
	// The shipped `who` alternation is
	//
	//     who ::= selection | except | inplace | "all" | "none"
	//
	// (SiegeAssistantGrammar.cpp, the `--- who ---` block, verified at the artifact
	// rather than taken from a handoff). `inplace` goes THIRD - after `except`, and
	// BEFORE the two bare strings - so the three object/array shapes stay grouped
	// ahead of the two scalars. ⛔ THE `WHO =` LINE BELOW LISTS THE FIVE SHAPES IN
	// THAT ORDER AND A TEST ASSERTS THE TWO AGREE (AS-§21.5).
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
	// ⭐ COMPONENT 1 OF 3 (TASK-547, +16 chars): `, or {"in":ZONE}` inserted in the
	// GRAMMAR'S OWN ORDER - after the `all_except` shape, before the two bare
	// strings. ⛔ Moving it is not a style choice; it breaks the mirror the comment
	// above pins and the test asserts.
	Out += TEXT("WHO    = [{\"kind\":KIND,\"n\":COUNT}] with 1 to 3 entries, or {\"all_except\":[KIND]} with 1 to 3 kinds, or {\"in\":ZONE}, or \"all\", or \"none\"\n");
	Out += TEXT("KIND   = a unit symbol from roster in [FORCES]\n");
	Out += TEXT("COUNT  = 1 to 30, or \"all\"\n");

	// ⭐ COMPONENT 2 OF 3 (TASK-547, 76 chars incl. the newline). THE MODEL CANNOT
	// OTHERWISE KNOW WHICH OF THE SEVEN PLACES ARE AREAS. The grammar makes an
	// invalid one UNSAMPLABLE; this line is what stops the model TRYING, and a
	// blocked attempt still costs the whole turn.
	//
	// ⛔ GENERATED FROM THE PlaceVocabulary TABLE'S `bHasRegion` COLUMN, NEVER A
	// HAND-WRITTEN LIST OF THREE - the table stays the single owner of the place
	// set, so a place that later gains a region primitive appears here by adding
	// ONE bool, and one that loses it disappears the same way. The static_assert
	// beside the table is what makes the empty case impossible.
	//
	// ⛔⛔ AND IT READS NO MEMBER STATE, WHICH IS THE WHOLE REASON IT IS THE TABLE
	// AND NOT GetRegionPlaceNames(). Zone A must be BYTE-IDENTICAL FOR THE LIFE OF
	// THE PROCESS or llama_memory_seq_rm cannot keep the prefix and the measured
	// 77.1 % KV reuse is destroyed silently - a QA FAIL that surfaces as a latency
	// regression rather than a wrong answer. GetRegionPlaceNames() is per-MATCH
	// state (a map with one ancient ground publishes fewer symbols), so printing it
	// here would make Zone A vary. This is exactly the split the `places` block
	// above already ships: ZONE A PRINTS THE FULL FIXED VOCABULARY, THE GRAMMAR
	// ENFORCES WHAT EXISTS THIS MATCH (see GetPlaceNames()'s comment, which states
	// the same rule for `where` in those words).
	//
	// 📌 NAMING NOTE SO NOBODY "FIXES" IT: the placeholder is `ZONE` to mirror the
	// GBNF rule `zone`. It has NOTHING to do with prompt Zones A/B/C - the model
	// never sees that vocabulary.
	Out += TEXT("ZONE   = an area place symbol: ");
	{
		bool bFirstRegion = true;
		for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(EPlaceSlot::Count); ++SlotIndex)
		{
			if (!PlaceVocabulary[SlotIndex].bHasRegion)
			{
				continue;
			}

			if (!bFirstRegion)
			{
				Out += TEXT(", ");
			}

			Out += PlaceVocabulary[SlotIndex].Symbol;
			bFirstRegion = false;
		}
	}
	Out += TEXT("\n");

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
	// ⚠️⚠️ THE HAZARD THIS COMMENT USED TO DECLARE IS CLOSED, AND THE LINE BELOW
	// IS DELIBERATELY LEFT BYTE-FOR-BYTE ALONE. TASK-521 read TASK-517's finished
	// output and ruled a NO-OP here, which is a decision and not an omission:
	//
	// WHAT IT SAID (and it was true when it was written): [FORCES] truncated to
	// MaxRosterKinds = 8 in the shipped snapshot but not in the spike fixture, so
	// on a >8-kind board this rule could refuse a kind that is ALIVE and merely got
	// collapsed into `other_kinds` - and `Sorcerer` is the LAST commandable row in
	// DT_Cards, so it was the first kind collapsed, every time. That is the whole
	// mechanism behind the reported "all units never includes sorcerers".
	//
	// WHY IT IS CLOSED: TASK-517 did two things. The cap is 13 (Jonathan's ruling),
	// and - the durable half - `other_kinds:` now prints the collapsed kinds' NAMES:
	// `other_kinds: sorcerer, cleric (5 units)`. That line is printed INSIDE the
	// [FORCES] block by AppendRosterBlock, so a collapsed symbol IS a symbol in
	// [FORCES] and this rule's antecedent - a literal membership test over the text
	// the model can see - is TRUE again. A collapse now costs the per-kind COUNTS,
	// never a kind's EXISTENCE (CONVENTIONS AS-§20.2 / AS-§20.3).
	//
	// ⛔ SO NO WORDING WAS ADDED HERE, AND THE THREE REASONS ARE RECORDED RATHER
	// THAN LEFT TO BE RE-LITIGATED:
	//  1. The rule is already true as written. A clause saying "other_kinds names
	//     count as [FORCES]" would restate what the block's own layout shows.
	//  2. TASK-521's spec requires any edit HERE to be char-neutral or NEGATIVE,
	//     and no addition can be. The 325-char Zone-A budget is spent on the two
	//     rules below, which teach behaviour the prompt did not have at all.
	//  3. The ladder measured that restating something the prompt already shows has
	//     poor leverage on this model (TASK-431: 19/25, the twice-taught row did not
	//     flip). Spending chars on emphasis here would buy the least per char.
	//
	// ⚠️ THE RESIDUAL, NOT PAPERED OVER: when the trimmer bites (measured: SEVEN
	// extra typed `order:` chars on a 13-kind board), a collapsed kind appears with
	// NO count. The model can still name it and still order it; what it cannot do
	// is read a per-kind tally for it - and the "Never copy a count from the roster"
	// rule below already forbids reading tallies out of the roster anyway, so the
	// two degrade in the same direction. The shortfall stays a GAME-side report.
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

	// ⚠️⚠️ THE TWO LINES BELOW ARE TASK-521, AND THEY ARE RULES ON PURPOSE — THE
	// OBVIOUS FIX (A NEW `who:"all"` FEW-SHOT) IS FORBIDDEN, NOT MERELY DECLINED.
	// A new EXAMPLE SENTENCE needs an absence certificate proving literal
	// disjointness from all three corpus files, and `assistant_eval_holdout2.csv`
	// is SEALED (CONVENTIONS AS-§12a / AS-§20.6). ⛔ TASK-430 already proved the
	// certificate cannot be skipped by care: two agents given the same PUBLIC brief
	// converged on the same invented noun and NEITHER COULD SEE IT. A rule line
	// needs no certificate at all — the disjointness law binds example sentences,
	// not instructions — so the seal stays unspent. ⭐ And it is the better
	// instrument anyway: loop 2 MEASURED a decision-ordering rule beating an
	// exemplar-and-emphasis pass at this exact seam (the block directly above).
	//
	// ⛔ THEY NAME NO KIND SYMBOL. `KIND` is the schema metavariable, not `miner`.
	// The §9c seam warning at the examples block is why: Zone A is byte-identical
	// for process life while the grammar's KIND alternatives come from the LIVE
	// roster, so any concrete symbol written here is one the sampler may FORBID on
	// a board that lacks it, with no log line saying so. Zone A names exactly three
	// kinds today (footman, sorcerer, archer) and this task refuses to make that
	// four — even though the flagship sentence is about miners.
	//
	// ⚠️ THEY ARE A MINIMAL PAIR AND MUST STAY ADJACENT AND IN THIS ORDER. Same
	// opening ("Every unit"), and the ONLY difference is whether an exception was
	// stated — which is the entire distinction being taught, held against a
	// constant frame. That is the same device few-shots #2/#3 use, applied to
	// rules. They sit here, immediately after the selection rule, because the three
	// together are ONE decision ladder read top-down: kinds named -> a selection
	// and a unit-taking intent; everyone, no exception -> "all"; everyone minus
	// some kinds -> the exclusion. ⛔ Do not move them above the byte-frozen
	// economy line — its POSITION is frozen along with its bytes, and its declared
	// neighbours are the [FORCES] rule above it and the selection rule below it.
	//
	// (1) THE `who:"all"` RULE — the one Jonathan actually reported. Nothing in the
	// shipped prompt ever demonstrated the bare "all" sentinel: all seven few-shots
	// emit a <=3-kind array or "none", and the only "everyone" exemplar maps to
	// charge/who:"none". So "all units ..." pulled toward the ARRAY form, the array
	// is capped at SiegeAssistantMaxSelectionKinds = 3 kinds, and a 13-kind board
	// therefore CANNOT reach the Sorcerer through it — structurally, not by
	// mistake. "never a list of kinds" is the half that closes that; the trailing
	// clause is the half that protects the `everyone attack` -> charge few-shot,
	// which is CORRECT and stays. ⚠️ Note the antecedent is deliberately "every
	// unit", not the alias "everyone": an alias is a lookup and TASK-431 measured
	// this model failing lookups (the `ancient_ground_near` alias missing its own
	// exact target string). The synonym table already routes `everyone attack`.
	Out += TEXT("- Every unit, no exception: who is \"all\", never a list of kinds. charge, fallback and rally still take \"none\".\n");

	// (2) THE EXCLUSION RULE. The second sentence is not caution, it is read off
	// the executor: `charge`/`fallback` run through ASiegePlayerController::
	// ApplyArmyWideStance(...) — the same shipped API the T and E keys call — and
	// `rally` through AHeroCharacter::Rally(). ⛔ NONE OF THE THREE WALKS THE
	// CANDIDATE LIST, so an exception handed to them has no code path that could
	// subtract anything: "fall back except the miners" would execute as "fall back
	// INCLUDING the miners", a valid-shaped wrong command that looks obeyed.
	// TASK-518's parser REFUSES `all_except` there (`exclude_conflict`), so without
	// this line the model emits an exclusion the parser then rejects — and the
	// player experiences a refusal, i.e. the feature not working. Naming the three
	// verbs explicitly is the half loop 2 measured beating the verb latch, so they
	// are named here too rather than left to "only with send, guard, ambush or
	// follow" to imply. `{"ask":"unsupported"}` is the shape this block already
	// teaches twice — ⛔ no new ask symbol was invented (AS-§20.1).
	// ⚠️ The arity and the no-counts property are carried by the `WHO =` schema
	// line, not repeated here: the GBNF enforces both structurally (a bounded
	// alternation of 1..3 BARE kind strings), so restating them would spend budget
	// on something the sampler cannot violate.
	Out += TEXT("- Every unit but some kinds: who is {\"all_except\":[KIND]}, only with send, guard, ambush or follow. On charge, fallback or rally: {\"ask\":\"unsupported\"}.\n");

	// ⭐⭐ COMPONENT 3 OF 3 (TASK-547, 147 chars incl. the newline) - AND ON THE
	// MANAGER'S FINDING THIS IS THE HIGHEST-VALUE LINE IN THE BATCH, BECAUSE IT
	// TEACHES THE ONE DISTINCTION THAT *IS* THE FEATURE.
	//
	// ⚠️⚠️ THE PROBLEM IT SOLVES, STATED WITH JONATHAN'S OWN SENTENCE: "send all
	// units currently in an ancient ground to attack a castle" POPULATES BOTH KEYS
	// AT ONCE -
	//
	//     who   = {"in":"ancient_ground_near"}   the units STANDING there
	//     where = "enemy_castle"                 the place they GO to
	//
	// Both values are place symbols out of the SAME seven-symbol vocabulary, and
	// before this line nothing in the prompt said which key takes which. That is a
	// COIN FLIP on the exact distinction the feature exists for, and the two wrong
	// answers are not harmless: {"in":"enemy_castle"} is not even sayable (the
	// castle is not region-bearing, AS-§21.4), and where:"ancient_ground_near"
	// sends the army to the ground it was supposed to be RECRUITED FROM.
	//
	// ⚠️ SO THE LINE MUST SAY "BOTH, IN ONE ORDER" EXPLICITLY. Teaching the shape
	// alone would leave the model choosing between the two keys instead of filling
	// each - and a prompt that never demonstrates two place symbols in one command
	// implicitly teaches that there is only ever one.
	//
	// ⛔ A RULE, NOT AN EXEMPLAR SENTENCE, AND THE CHOICE IS MEASURED RATHER THAN
	// PREFERRED (AS-§20.4 leg 2): loop 2 measured a decision-ordering rule beating
	// an exemplar-and-emphasis pass AT THIS EXACT SEAM (the two lines above). 🔒 A
	// new few-shot would also need an absence certificate against three corpora,
	// one of which is SEALED - so the rule is both the better instrument and the
	// free one.
	//
	// ⚠️ THE ANTECEDENT IS TESTABLE AND IT DELIBERATELY CARRIES THE TRIGGER WORD.
	// "Units already in a place" is a test the model can run against the sentence
	// ("units currently IN an ancient ground"), and the word "in" it matches on is
	// the JSON key it must then emit. An abstraction the model has to already know
	// is what rung 1 measured failing (see the [FORCES] rule above).
	//
	// ⚠️ IT NAMES NO PLACE SYMBOL, ON THE §9c SEAM RULE. `ZONE` is the schema
	// metavariable; writing `mid` or `ancient_ground_near` here would pin a symbol
	// into a byte-frozen Zone A that the sampler may FORBID on a map lacking it.
	// The `ZONE =` line above is where the live-ish vocabulary is named, and it is
	// named from the table for exactly that reason.
	//
	// ⚠️ SCOPED POSITIVELY ONLY, AND THE OMISSION IS DECLARED RATHER THAN
	// OVERLOOKED. The exclusion rule directly above carries BOTH halves ("only
	// with send, guard, ambush or follow" AND "On charge, fallback or rally:
	// {"ask":"unsupported"}"); this line carries only the first, because the second
	// costs 52 more characters and the batch's Zone-A ceiling is +250 against a
	// spend that is already 239. The residual is bounded and known: a region handed
	// to charge / fallback / rally is REFUSED BY THE PARSER (`region_conflict`,
	// AS-§21.6) rather than silently dropped, and AS-§21.11 records that refusal as
	// designed behaviour on Jonathan's playtest sheet. ⇒ The cost of the missing
	// half is a refusal the player sees, never an army that moves unasked.
	//
	// ⚠️ THE OTHER THREE CONFLICTS NEED NO TEACHING AT ALL, WHICH IS WHY THEY ARE
	// ABSENT RATHER THAN FORGOTTEN: `who` is ONE alternation, so a region cannot be
	// sampled together with a selection array, with `all_except`, or with "none".
	// The grammar makes those three unsayable; only the INTENT conflict survives
	// into prose, and that is the half this line spends its characters on.
	//
	// ⚠️ POSITION: it extends the `who`-shape ladder that runs top-down through the
	// three rules above (kinds named -> a selection and a unit-taking intent;
	// everyone, no exception -> "all"; everyone minus some kinds -> the exclusion;
	// and now the units standing somewhere -> the region). ⛔ It goes AFTER the
	// TASK-521 minimal pair, never between them, and the byte-frozen economy line
	// keeps both of its declared neighbours.
	Out += TEXT("- Units already in a place: who is {\"in\":ZONE}, where is still where they go, and one order may set both. Only with send, guard, ambush or follow.\n");

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
	//
	// ⭐ AND THE LINE NAMES WHAT IT HID (TASK-517). It used to print
	// `other_kinds: 5 kinds, 9 units` - counts with NO SYMBOLS - which is the
	// root cause of the defect Jonathan reported as *"whenever I say all units,
	// it doesn't seem to include sorcerers even when they were spawned."* The
	// roster prints in fixed DT_Cards row order and `Sorcerer` is the LAST
	// commandable row, so it is ALWAYS the first kind collapsed; with counts only,
	// the token `sorcerer` never reached the model, and Zone A's "if the unit
	// named is not a kind in [FORCES], answer unsupported" rule then refused a
	// unit that was standing on the board.
	//
	// ⛔ A COLLAPSE MAY HIDE A KIND'S *NUMBERS*. IT MAY NEVER HIDE ITS *NAME*.
	// That is the invariant this line exists to hold, and it holds at ANY cap and
	// on ANY board size - which is what makes it, not MaxRosterKinds, the durable
	// half of the fix (CONVENTIONS AS-§20.2). The grammar has ALWAYS admitted
	// every collapsed kind (GetUnitKinds() is never truncated), so before this the
	// sampler could name a unit the prompt had not shown it; that is the gap being
	// closed.
	//
	// The names are accumulated in the SAME pass that tallies the units - one walk
	// of the collapsed tail, in the roster's own fixed order, so the line can never
	// disagree with the rows above it about what was hidden or in what order.
	int32 CollapsedKinds = 0;
	int32 CollapsedUnits = 0;
	FString CollapsedNames;
	CollapsedNames.Reserve(FMath::Max(0, UnitKinds.Num() - PrintCount) * 14);
	for (int32 KindIndex = PrintCount; KindIndex < UnitKinds.Num(); ++KindIndex)
	{
		if (CollapsedKinds > 0)
		{
			CollapsedNames += TEXT(", ");
		}
		CollapsedNames += UnitKinds[KindIndex].ToString();

		++CollapsedKinds;
		CollapsedUnits += KindTotals.IsValidIndex(KindIndex) ? KindTotals[KindIndex] : 0;
	}

	if (CollapsedKinds == 0)
	{
		Out += TEXT("other_kinds: none\n");
	}
	else
	{
		// `(1 units)` on a single collapsed kind is DELIBERATE: the format is
		// pinned, and a pluralisation branch would spend characters out of a
		// ~6-char headroom to teach a 1.7B model English it does not need.
		Out.Appendf(TEXT("other_kinds: %s (%d units)\n"), *CollapsedNames, CollapsedUnits);
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
	// snapshot (Zone B's reserve + Zone C) fits SnapshotTrimBudgetChars. Shrinking
	// is deterministic - always from the TAIL of the fixed card-row order - so the
	// same board always produces the same bytes.
	//
	// ⚠️ THIS IS THE TRIMMER, NOT THE BUDGET AUTHORITY (TASK-455). The authority is
	// USiegeLlamaSubsystem::MaxSnapshotTokens = 400, counted by llama_tokenize on
	// the worker, and it REJECTS an over-budget turn rather than cutting it. This
	// loop exists so that rejection is rare: a board that would blow the token
	// budget gets a narrower roster and a usable answer instead of a refusal. See
	// SnapshotTrimBudgetChars' comment for the three-role table and for why
	// pointing this at the pre-filter's 3000 would be a QA FAIL rather than a
	// simplification.
	//
	// ⚠️ BOTH PLAYER-TEXT LINES ARE ALREADY SUBTRACTED HERE, VIA Tail. `pending:`
	// and `order:` each spend up to MaxUtteranceBytes of this budget and NEITHER is
	// trimmed by it - the roster absorbs all of it, which is the CONVENTIONS §8 rule
	// that the utterance is never truncated by the snapshot budget.
	//
	// ⚠️ AND SINCE TASK-517 RAISED MaxRosterKinds TO 13, THAT SENTENCE HAS TEETH.
	// The shipped operating point on a 13-kind board is head 108 + roster 621 +
	// tail 158 = 887 chars of an 893-char Zone C budget, i.e. ~6 chars spare on the
	// DEFAULT 61-char `order:` line. About SEVEN more characters of typed text
	// re-collapse the tail. ⛔ That is the reason the collapse line NAMES what it
	// hid rather than counting it: at this cap the trimmer is expected to bite in
	// normal play, and a fix that only worked when it did not bite would be a fix
	// that fails silently for exactly the player who types a long sentence.
	const int32 RosterBudget = SnapshotTrimBudgetChars - ZoneBCharReserve - Head.Len() - Tail.Len();

	const int32 CapKinds = FMath::Min(UnitKinds.Num(), MaxRosterKinds);

	// ⚠️ THE LOOP IS STILL STRICTLY MONOTONIC WITH THE NAMED COLLAPSE LINE, AND
	// THAT IS THE ONE PROPERTY THE NEW FORMAT COULD HAVE BROKEN, SO IT IS PROVED
	// HERE RATHER THAN ASSUMED. A step removes one roster row, which costs
	// 36 + len(symbol) + digits(the three counts) chars, and adds that symbol back
	// on the collapse line for len(symbol) + 2 (the ", " separator) - so the symbol
	// cancels and every step shrinks the block by at least ~29 chars whatever the
	// kind is named. The first step is the shallowest, because it also swaps
	// `none` for ` (N units)`, and it still shrinks: measured 621 -> 588 -> 551 ->
	// ... -> 182 at one kind printed on the 13-kind board. KindsToPrint <= 0
	// remains the unconditional floor either way, so the loop terminates even if a
	// later format change breaks the algebra above.
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
	// accuracy, and it is strictly more likely now that SnapshotTrimBudgetChars
	// binds at a measured budget rather than a generous guess.
	//
	// 📌 THE ACCURACY CLAIM HERE IS NARROWER THAN IT WAS, AND THE NARROWING IS THE
	// TASK-517 FIX RATHER THAN A CONCESSION. This comment used to end "...so the
	// GRAMMAR still admits every collapsed kind and the sampler can name a unit the
	// prompt did not show the model." GetUnitKinds() is still never truncated, so
	// the grammar half is unchanged - but the prompt half is no longer true,
	// because `other_kinds:` now NAMES every collapsed kind. What a collapse costs
	// the model today is the per-kind COUNTS, not the kinds' EXISTENCE. ⛔ It is
	// still worth logging: a model that can see `sorcerer` but not how many there
	// are can still open a clarification turn the snapshot used to be able to
	// answer, and a degraded turn nobody recorded cannot be reconstructed later.
	const int32 CollapsedKinds = UnitKinds.Num() - KindsToPrint;
	if (CollapsedKinds > 0)
	{
		// The two causes are named separately because they call for different
		// fixes: the kind cap is a deliberate aggregation policy, whereas a budget
		// bite means the snapshot genuinely does not fit and something upstream
		// has to aggregate harder.
		//
		// ⚠️ AT MaxRosterKinds = 13 THE CAP BRANCH IS UNREACHABLE TODAY, AND IT IS
		// KEPT RATHER THAN DELETED (TASK-517). DT_Cards has exactly 13 commandable
		// kinds, so CapKinds == UnitKinds.Num() on every live board and the ONLY
		// reachable cause is the character budget - which is precisely why the cap
		// raise alone did not close the defect. The branch survives because a
		// FOURTEENTH kind makes it reachable again on the same day it is added, and
		// a log line that has to be re-derived at that moment is a log line nobody
		// will trust. Both strings were re-read against the new cap and both are
		// still TRUE of the case they name.
		const bool bBudgetBite = KindsToPrint < CapKinds;
		const TCHAR* const Cause = bBudgetBite
			? TEXT("the CHARACTER BUDGET, below the MaxRosterKinds cap")
			: TEXT("the MaxRosterKinds cap");

		// Unlatched per-turn record. Verbose costs nothing in normal play and is
		// what makes a degraded turn RECONSTRUCTABLE afterwards - the Warning
		// below deliberately fires once per escalation, so it cannot tell you
		// which particular sentence was answered against a trimmed roster.
		UE_LOG(LogSiegeAssistant, Verbose,
			TEXT("Snapshot roster degraded: %d of %d kind(s) printed in full, %d collapsed into `other_kinds:` by %s (roster %d chars of a %d-char budget; SnapshotTrimBudgetChars %d)."),
			KindsToPrint, UnitKinds.Num(), CollapsedKinds, Cause,
			RosterBlock.Len(), RosterBudget, SnapshotTrimBudgetChars);

		if (KindsToPrint < WarnedRosterKindsPrinted || CollapsedKinds > WarnedRosterKindsCollapsed)
		{
			WarnedRosterKindsPrinted = FMath::Min(WarnedRosterKindsPrinted, KindsToPrint);
			WarnedRosterKindsCollapsed = FMath::Max(WarnedRosterKindsCollapsed, CollapsedKinds);

			UE_LOG(LogSiegeAssistant, Warning,
				TEXT("Snapshot roster TRUNCATED: %d of %d kind(s) printed in full, %d collapsed into `other_kinds:` by %s. The grammar admits all %d kinds and `other_kinds:` NAMES every collapsed one, so the model can still SEE each symbol - what a collapse costs is the per-kind COUNTS, which degrades an order into a clarification rather than a refusal (TASK-517). Roster %d chars of a %d-char budget (SnapshotTrimBudgetChars %d, ZoneBCharReserve %d). Aggregate harder, or lower the reserve from a PRINTED zoneB_chars reading - do NOT raise the trim budget to hide this. NOTE the trim budget is a proxy: the real cap is the plugin's MaxSnapshotTokens=400, counted by the tokenizer, which REJECTS an over-budget turn instead of trimming it."),
				KindsToPrint, UnitKinds.Num(), CollapsedKinds, Cause, UnitKinds.Num(),
				RosterBlock.Len(), RosterBudget, SnapshotTrimBudgetChars, ZoneBCharReserve);
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
	// the 2.71 chars/token ratio behind SnapshotTrimBudgetChars was calibrated on
	// ASCII while these two lines are the only ones a player can fill with anything
	// else.
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
