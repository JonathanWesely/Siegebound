// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeckBuilderWidget.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckLibrary.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SpellLibrary.h"
#include "Siegebound/SummonedUnit.h" // TASK-379: GetDefault<ASummonedUnit>() needs the COMPLETE type for the two Sorcerer boost getters

// ---------------------------------------------------------------------------
// Card-details glossary (TASK-268) — the ONLY authored player-facing copy in
// this widget, and the only place it may live (CONVENTIONS "Deck-builder card
// details"). One string per KEYWORD / MECHANIC, never one per card: EVERY card
// description is composed from these plus the card's OWN row numbers, so a
// balance edit to Docs/Data/cards.csv re-derives every description for free and
// the designer-only Notes column is never surfaced.
//
// GLOSSARY-MIRROR RULE (CONVENTIONS): every number BAKED INTO a string below is
// a mechanic magnitude that lives as a gameplay UPROPERTY default rather than a
// cards.csv column — each carries a "// mirrors <Class>::<Property>" comment and
// MUST be updated whenever that gameplay value changes. Anything that IS a CSV
// column is interpolated from the row at runtime and never appears here (§3.0).
//   ✅ THE EXCEPTION IS CLOSED (TASK-379). TASK-364 had to state the Sorcerer's two
//   magnitudes QUALITATIVELY — the newer, more specific CONVENTIONS law "Ancient
//   Grounds + Sorcerer" §8 forbids BAKING a mechanic magnitude, and this widget
//   could not reach either value while both sat in ASummonedUnit's `protected:`
//   block. The public getters added there put SorcererGroundBoostFmt on §8's
//   PREFERRED branch: it INTERPOLATES both, so a retune of either lever re-derives
//   the card text for free. Prefer this shape over a mirrors-comment for any future
//   magnitude that is publicly readable.
//
// TRUTH LAW: every line states behavior the shipping code actually implements;
// the verification site for each clause is recorded in handoffs/TASK-268.md
// (TASK-364 for the two Sorcerer clauses).
// ---------------------------------------------------------------------------
namespace SiegeboundCardGlossary
{
	// --- card-role clauses ---------------------------------------------------

	/** Miner (Economy). // mirrors ASiegePlayerState::MinerGoldPerTick (1 per 1 s tick) + ASiegePlayerState::MaxActiveMiners (6) */
	const TCHAR MinerRole[] = TEXT("Walks to an open gold mine on its own and, once it claims a spot there, adds +1 gold per second - it earns nothing until it arrives. Killing it, or the mine running dry, ends that income. You may have 6 miners alive at once.");

	/** Deep Mine (Economy building). // mirrors ADeepMine::DeepMineIncome (+2 gold/s) */
	const TCHAR DeepMineRole[] = TEXT("Raises your income by +2 gold per second the moment it is built - no walk needed, and it does not use up one of your miner slots. It is a destructible structure: raze it and that income is gone.");

	/** Masons (Utility instant). // mirrors ASiegePlayerController::MasonsHealAmount (300) + ::MasonsHealDuration (10 s) */
	const TCHAR MasonsRole[] = TEXT("Instant repair: heals your own castle 300 health over 10 seconds. With no castle left standing it is refused and costs you nothing.");

	/** Sorcerer, half 1 of 2 - the never-attacks seal. // mirrors ASorcererUnit::CanEverAttack (false) + its ctor's AggroRadius/DefendRadius 0, enforced at ASummonedUnit::EnterAttack / ::UpdateStateGrouped / ::PerformAttack */
	const TCHAR SorcererRole[] = TEXT("It never attacks - no order will make it strike, and an enemy walking into it is ignored - so it deals no damage of its own. It still takes your unit orders like anything else you play, which is how you walk it onto an ancient ground.");

	/**
	 *  Sorcerer, half 2 of 2 - the ancient-ground boost. // mirrors AAncientGround's 1 Hz boost tick (BoostTickInterval, friendly-only, sorcerers never self-boost) + ASummonedUnit::CanReceiveDamageBoost (attackers only) + ::ClearPermanentDamageStacks in HandleDeath
	 *
	 *  ✅ THE MAGNITUDES ARE INTERPOLATED (TASK-379) — CONVENTIONS "Ancient Grounds +
	 *  Sorcerer" §8's PREFERRED branch: *"magnitudes that exist as UPROPERTY mechanic
	 *  rules are interpolated from those properties or stated qualitatively — never a
	 *  hardcoded number that can drift."* TASK-364 shipped the qualitative FALLBACK
	 *  only because both values sat in ASummonedUnit's `protected:` block and a
	 *  `GetDefault<ASummonedUnit>()` read would not compile; the two public
	 *  BlueprintPure getters added by TASK-379 removed that obstacle, and this is the
	 *  line that changed.
	 *
	 *  %s #1 = the PER-SECOND gain, 100 x ASummonedUnit::GetPermanentDamageBonusPerStack()
	 *          (0.05 ⇒ "5", one stack per sorcerer per 1 Hz boost tick).
	 *  %s #2 = the CEILING, that same per-stack percent x ::GetMaxPermanentDamageStacks()
	 *          (5 x 80 ⇒ "400").
	 *
	 *  ⚠️ NEVER re-bake these as literals: both are FLAGGED balance levers
	 *  (CONVENTIONS §4) and retuning them is EXPECTED. The operand order
	 *  (100.f FIRST) is the order ASummonedUnit::GetDamageBoostPercent() itself uses
	 *  and is exact in single precision at the shipped values — qa/TASK-365-report.md
	 *  "BOUNDARY EXACTNESS" proves 100.f x 0.05f == exactly 5.0f and 5.0f x 80 ==
	 *  exactly 400.0f, so FormatStatValue prints "5" / "400" with no decimal tail.
	 *  Literal `%%` is the player-facing percent sign (SpellAllyBuffFmt precedent).
	 */
	constexpr TCHAR SorcererGroundBoostFmt[] = TEXT("While it stands inside an ancient ground, every friendly unit that fights standing in that same ground hits +%s%% harder for each second it spends there. The gain is permanent - kept in full when that unit walks back out, and lost only when it dies - and it stacks up second after second to a hard ceiling of +%s%%. A second sorcerer in the same ground builds it twice as fast. Units that never attack - miners, healers and sorcerers themselves - gain nothing.");

	/** Sharpened Blade. // mirrors AHeroCharacter::MeleeDamageBonus (+10 per stack) */
	const TCHAR UpgradeSharpenedBlade[] = TEXT("Instantly upgrades your hero: +10 damage on every melee swing.");

	/** Plate Armor. // mirrors AHeroCharacter::MaxHPBonus (+100 per stack, healed immediately) */
	const TCHAR UpgradePlateArmor[] = TEXT("Instantly upgrades your hero: +100 maximum health, and it heals 100 straight away.");

	/** Swift Boots. // mirrors AHeroCharacter::MoveSpeedBonus (0.25 = +25%) */
	const TCHAR UpgradeSwiftBoots[] = TEXT("Instantly upgrades your hero: +25% movement speed, walking and sprinting alike.");

	/** War Banner. // mirrors AHeroCharacter::WarBannerAuraRadius (600) + ::WarBannerDamageBonus (0.20 = +20%) */
	const TCHAR UpgradeWarBanner[] = TEXT("Instantly upgrades your hero: friendly units within 600 units of it deal +20% more damage.");

	/** Shared hero-upgrade tail; %d = the row's own stack cap (AHeroCharacter::GetStackCapForUpgrade reads MaxCopies from the row, so the CSV column IS the cap). */
	constexpr TCHAR UpgradeTailFmt[] = TEXT("Stacking: your hero can hold %d of these; a further copy is refused and costs you nothing. Upgrades last the rest of the match and survive your hero's death.");

	/** Any Building card. */
	const TCHAR StructureRole[] = TEXT("Stationary structure: it never moves, physically blocks the ground it stands on, and holds until it is destroyed.");

	/** Auto-firing tower (a Building that attacks). */
	const TCHAR TowerRole[] = TEXT("Fires on its own at the nearest enemy unit or hero within range - it never shoots castles, walls or other structures.");

	// --- keyword clauses -----------------------------------------------------

	/** bCharge. // mirrors ASummonedUnit::ChargeMoveSeconds (2 s) + ASummonedUnit::ChargeMultiplier (2x) */
	const TCHAR Charge[] = TEXT("Charge: its first attack after 2 seconds of uninterrupted advancing deals double damage. Being blocked or stopping loses the momentum, and it must be rebuilt.");

	/** bSlayer. // mirrors ASummonedUnit::SlayerHPThreshold (150) + ASummonedUnit::SlayerMultiplier (2x) */
	const TCHAR Slayer[] = TEXT("Slayer: deals double damage to any target with 150 or more maximum health - the big units, towers and castles.");

	/** bSuicide; %s = row Damage, %s = row AoERadius. */
	constexpr TCHAR SuicideFmt[] = TEXT("Explodes the moment it reaches its target - or if it is killed on the way in - dealing %s damage to every enemy within %s units, then dies. It can only blow up once.");

	/** SwarmCount; %d = row SwarmCount. // mirrors ASiegePlayerController::SwarmSpawnRadius (300) */
	constexpr TCHAR SwarmFmt[] = TEXT("Swarm: one play puts %d of them on the field at once, spread around a 300-unit circle, for a single card and a single cost.");

	/** ChainTargets/ChainFalloff; %d = ChainTargets, %s = the damage sequence. // mirrors ATower::ChainBounceRadius (350) */
	constexpr TCHAR ChainFmt[] = TEXT("Chain: every shot is an instant zap that arcs through up to %d enemies, weakening as it goes (%s damage in turn). Each arc only reaches an enemy within 350 units of the previous one, so a lone target takes just the first hit.");

	/** Spawner; %s = the spawned card's display name, %s = SpawnInterval. */
	constexpr TCHAR SpawnerFmt[] = TEXT("Summons a free %s every %s seconds, on your side and at no extra cost - the first one arrives a full interval after it is built.");

	/** Spawner lifetime; %s = row Lifetime. */
	constexpr TCHAR SpawnerLifetimeFmt[] = TEXT("It falls apart on its own after %s seconds; everything it already summoned stays on the field.");

	// --- spell clauses -------------------------------------------------------

	/** AoEDamage delivered as a ground circle; %s = row Damage, %s = row AoERadius. */
	constexpr TCHAR SpellAoEDamageCircleFmt[] = TEXT("Deals %s damage to every enemy within %s units of the spot you target. Your own side is never hit.");

	/** AoEDamage delivered as a hero line; %s = row Damage. The row's radius plays NO part on this path - the corridor is the bolt's own (see the delivery line). */
	constexpr TCHAR SpellAoEDamageLineFmt[] = TEXT("Deals %s damage to every enemy the bolt passes through - units, heroes, structures and castles alike. Your own side is never hit.");

	/** Freeze delivered as a ground circle; %s = row AoERadius, %s = row EffectDuration. */
	constexpr TCHAR SpellFreezeCircleFmt[] = TEXT("Freezes enemy units and enemy structures within %s units of the spot you target for %s seconds - frozen targets cannot move, attack or fire. Castles and heroes are immune.");

	/** Freeze delivered as a hero line; %s = row EffectDuration. */
	constexpr TCHAR SpellFreezeLineFmt[] = TEXT("Freezes every enemy unit and structure the bolt passes through for %s seconds - frozen targets cannot move, attack or fire. Castles and heroes are immune.");

	/** TopTargetsDamage; %d = row MaxTargets, %s = row AoERadius, %s = row Damage. */
	constexpr TCHAR SpellTopTargetsFmt[] = TEXT("Strikes the %d enemies with the most health left within %s units for %s damage each. Castles are never struck; towers and other structures take the full amount, which is why it kills them outright.");

	/** AllyBuff; %s = row AoERadius, %s = row EffectDuration. // mirrors ASummonedUnit::BattleCryAttackSpeedBonus (0.5) + ::BattleCryMoveSpeedBonus (0.25) */
	constexpr TCHAR SpellAllyBuffFmt[] = TEXT("Friendly units within %s units of the spot you target attack 50%% faster and move 25%% faster for %s seconds.");

	/** GoldSteal; %d = row GoldSteal. */
	constexpr TCHAR SpellGoldStealFmt[] = TEXT("Steals %d gold from your opponent the instant you play it - no aiming, no target. If they hold less than that, you take everything they have.");

	/** HeroLine delivery. // mirrors ASpellLineSweep::LineRange (900) + ASpellLineSweep::LineHalfWidth (100 to either side) */
	const TCHAR DeliveryHeroLine[] = TEXT("Aimed from your hero: it flies out as a bolt roughly 900 units long, catching anything within 100 units to either side, and passes straight through walls and bodies. A bolt that catches nothing is still spent.");

	/** GroundCircle delivery. */
	const TCHAR DeliveryGroundCircle[] = TEXT("Aimed at a spot on the ground: it goes off where you place the reticle.");

	// --- targeting profile ---------------------------------------------------

	/** ECardProfile::Siege. */
	const TCHAR ProfileSiege[] = TEXT("Siege: it ignores enemy units and the enemy hero completely, walking past them for the nearest enemy structure - and for the castle when none is left.");

	/** ECardProfile::Support; %s = row Range, %s = row Damage. */
	constexpr TCHAR ProfileSupportFmt[] = TEXT("Support: it never attacks. It follows your line and heals the most hurt friendly unit within %s units for %s health per second.");

	// --- fortification damage scaling ---------------------------------------

	/** Siege-tagged attacker. // mirrors ACastle::TakeDamage + ABuilding::TakeDamage (Siege x2) */
	const TCHAR ScalingSiege[] = TEXT("Its damage lands on castles and structures at DOUBLE the listed amount.");

	/** Ranged unit (homing shot). // mirrors ACastle::TakeDamage (projectile x0.5; buildings take the full amount) */
	const TCHAR ScalingRangedVsCastle[] = TEXT("Shots hit a castle for HALF the listed damage - units, heroes and structures take the full amount.");

	/** Damaging spell. // mirrors ACastle::TakeDamage (spell x0.5; buildings take the full amount) */
	const TCHAR ScalingSpellVsCastle[] = TEXT("A castle caught in it takes HALF damage; everything else takes the full amount.");
}

namespace
{
	/**
	 *  CardIDs whose mechanic is a class rule rather than a row column, so the
	 *  composer has to name them (TASK-268). Each mirrors the CardID the SHIPPING
	 *  code itself matches on — this is the same keying, not a second source of
	 *  truth. CONVENTIONS: CardID = the DT_Cards row name.
	 */
	const FName GlossaryCardID_Miner(TEXT("Miner"));                     // mirrors ASiegePlayerController::MinerCardID
	const FName GlossaryCardID_DeepMine(TEXT("DeepMine"));               // mirrors ASiegePlayerController::BuildingEconomyCardIDs
	const FName GlossaryCardID_Masons(TEXT("Masons"));                   // mirrors ASiegePlayerController::MasonsCardID
	const FName GlossaryCardID_Sorcerer(TEXT("Sorcerer"));               // mirrors the per-card spawn path BP_Unit_<CardID> ⇒ ASorcererUnit — the mechanic is CLASS identity (CanEverAttack / IsAncientGroundEmpowerer), and this row name is what resolves to that class
	const FName GlossaryCardID_SharpenedBlade(TEXT("SharpenedBlade"));   // mirrors AHeroCharacter.cpp UpgradeCardID_SharpenedBlade
	const FName GlossaryCardID_PlateArmor(TEXT("PlateArmor"));           // mirrors AHeroCharacter.cpp UpgradeCardID_PlateArmor
	const FName GlossaryCardID_SwiftBoots(TEXT("SwiftBoots"));           // mirrors AHeroCharacter.cpp UpgradeCardID_SwiftBoots
	const FName GlossaryCardID_WarBanner(TEXT("WarBanner"));             // mirrors AHeroCharacter.cpp UpgradeCardID_WarBanner

	/**
	 *  Separator for the identity line — the CONVENTIONS-mandated MIDDLE DOT
	 *  (U+00B7): "<Type> [dot] Cost <n> gold [dot] Max <n> per deck". Composed
	 *  from its CODE POINT, not typed as a literal glyph, so this source file
	 *  stays pure ASCII inside string literals: comments in this module carry raw
	 *  UTF-8 harmlessly, but a mis-decoded string literal would ship mojibake into
	 *  the player-facing panel.
	 */
	FString IdentitySeparator()
	{
		return FString::Printf(TEXT(" %c "), static_cast<TCHAR>(0x00B7));
	}

	/** Player-facing stat number: whole values print clean ("120"), fractional ones keep one decimal ("1.5"). Never scientific notation, never "120.000000". */
	FString FormatStatValue(float Value)
	{
		if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value)))
		{
			return FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));
		}
		return FString::Printf(TEXT("%.1f"), Value);
	}

	/** Player-facing card-type prose for the identity line (never the enum name). */
	FString CardTypeLabel(ECardType CardType)
	{
		switch (CardType)
		{
		case ECardType::Unit:        return TEXT("Unit");
		case ECardType::Building:    return TEXT("Building");
		case ECardType::Economy:     return TEXT("Economy");
		case ECardType::Spell:       return TEXT("Spell");
		case ECardType::HeroUpgrade: return TEXT("Hero Upgrade");
		case ECardType::Utility:     return TEXT("Utility");
		default:                     return TEXT("Card");
		}
	}
}

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
// Card details ("how it works" panel, TASK-268)
// ---------------------------------------------------------------------------

FString UDeckBuilderWidget::GetCardDescription(FName CardID) const
{
	if (CardID.IsNone())
	{
		// "nothing selected" is a NORMAL state, not a fault — the panel shows its own
		// static hint. Silent, exactly like GetCardArtTexture's None early-out.
		return FString();
	}

	const FCardRow* Row = ResolveCardRow(CardID);
	if (!Row)
	{
		// missing table / unknown row: ResolveCardRow already logged it ONCE through
		// the existing bWarnedMissingTable / WarnedMissingRowIDs spam guards. An empty
		// body (the panel falls back to its hint) — never a crash, never an ensure,
		// and never a half-composed description that reads like a real card.
		return FString();
	}

	// Composed fresh from the row on every call (the anti-drift ruling): a balance
	// edit to Docs/Data/cards.csv changes what the player reads with no code change.
	TArray<FString> IdentityLines;
	TArray<FString> StatLines;
	TArray<FString> RuleLines;
	AppendIdentityLines(*Row, IdentityLines);
	AppendStatLines(*Row, StatLines);
	AppendRuleLines(CardID, *Row, RuleLines);

	// ONE blank line between blocks — never a leading, trailing or doubled blank: a
	// spell has no stat block and a plain melee unit has no rules block, and neither
	// may punch a hole in the panel.
	TArray<FString> Blocks;
	Blocks.Reserve(3);
	if (IdentityLines.Num() > 0)
	{
		Blocks.Add(FString::Join(IdentityLines, TEXT("\n")));
	}
	if (StatLines.Num() > 0)
	{
		Blocks.Add(FString::Join(StatLines, TEXT("\n")));
	}
	if (RuleLines.Num() > 0)
	{
		Blocks.Add(FString::Join(RuleLines, TEXT("\n")));
	}

	return FString::Join(Blocks, TEXT("\n\n"));
}

void UDeckBuilderWidget::SelectCardForDetails(FName CardID)
{
	// An unknown row (or NAME_None) CLEARS the selection rather than parking the
	// panel on a card that cannot be described — and STILL fires, with an empty
	// string, so the panel can show its hint instead of stale text. ResolveCardRow
	// carries the once-per-CardID logging.
	const FCardRow* Row = CardID.IsNone() ? nullptr : ResolveCardRow(CardID);
	SelectedDetailCardID = Row ? CardID : NAME_None;

	// Deck-neutral by construction: nothing here touches WorkingDeck, and
	// OnDeckModelChanged is deliberately NOT fired (the deck model did not change —
	// the M6 counters/legality gate must not churn on a details click).
	OnCardDetailsRequested(SelectedDetailCardID.IsNone() ? FString() : SelectedDetailCardID.ToString());
}

void UDeckBuilderWidget::ClearCardDetails()
{
	SelectedDetailCardID = NAME_None;
	OnCardDetailsRequested(FString());
}

FName UDeckBuilderWidget::GetSelectedDetailCardID() const
{
	return SelectedDetailCardID;
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

// ---------------------------------------------------------------------------
// GetCardDescription composers (TASK-268)
//
// Every magnitude below that exists as a cards.csv column is read from the ROW
// (§3.0 — never hardcoded); the only literals are the glossary strings at the
// top of this file. Player-facing text carries no GDD refs, no class names and
// no property names.
// ---------------------------------------------------------------------------

void UDeckBuilderWidget::AppendIdentityLines(const FCardRow& Row, TArray<FString>& OutLines) const
{
	// CONVENTIONS: "<Type> · Cost <n> gold · Max <n> per deck".
	const FString Separator = IdentitySeparator();

	FString IdentityLine = FString::Printf(TEXT("%s%sCost %d gold"),
		*CardTypeLabel(Row.CardType), *Separator, Row.Cost);

	if (Row.MaxCopies > 0)
	{
		// the per-card deck cap the "+" greys out at — the SAME column AddCopy enforces
		IdentityLine += FString::Printf(TEXT("%sMax %d per deck"), *Separator, Row.MaxCopies);
	}

	OutLines.Add(IdentityLine);
}

void UDeckBuilderWidget::AppendStatLines(const FCardRow& Row, TArray<FString>& OutLines) const
{
	const bool bIsSpell = (Row.CardType == ECardType::Spell);
	const bool bIsSupport = (Row.Profile == ECardProfile::Support);
	const bool bIsChain = (Row.ChainTargets > 0);

	if (Row.HP > 0.f)
	{
		OutLines.Add(FString::Printf(TEXT("Health: %s"), *FormatStatValue(Row.HP)));
	}

	// A spell's Damage/AoERadius describe its EFFECT, a Support unit's Damage is a
	// HEAL RATE (it never attacks at all) and a suicide unit's Damage is its blast —
	// each is stated by its own rules line below, so none of the three prints an
	// "attack" stat here. Printing "Damage 8 per attack" for a Cleric would be a
	// truth-law failure, not a nit.
	const bool bHasAttackStats = !bIsSpell && !bIsSupport && !Row.bSuicide && Row.Damage > 0.f;
	if (bHasAttackStats)
	{
		FString DamageLine = FString::Printf(TEXT("Damage: %s per attack"), *FormatStatValue(Row.Damage));
		if (Row.AoERadius > 0.f)
		{
			// splash attacker (a tower with an AoE shot) — the suicide blast took the
			// !bSuicide branch above
			DamageLine += FString::Printf(TEXT(" (splash: every enemy within %s units of the hit)"),
				*FormatStatValue(Row.AoERadius));
		}
		OutLines.Add(DamageLine);

		if (Row.Cadence > 0.f)
		{
			// "%ss" (1s / 1.2s), never "%s seconds" — a Cadence of 1 would read "1 seconds"
			OutLines.Add(FString::Printf(TEXT("Attacks every %ss"), *FormatStatValue(Row.Cadence)));
		}
	}

	// Range wording follows the delivery the ROW actually authors: a chain row hits
	// instantly, a bRanged row throws a homing shot, a suicide row has to arrive, and
	// everything else with damage is melee contact. A Support unit's Range is a HEAL
	// radius, so it is stated in that unit's rules line instead of here.
	if (Row.Range > 0.f && !bIsSupport)
	{
		FString RangeLine;
		if (Row.bSuicide)
		{
			RangeLine = FString::Printf(TEXT("Range: %s units (it has to reach its target to detonate)"), *FormatStatValue(Row.Range));
		}
		else if (bIsChain)
		{
			RangeLine = FString::Printf(TEXT("Range: %s units (instant hit - there is no shot to dodge)"), *FormatStatValue(Row.Range));
		}
		else if (Row.bRanged)
		{
			RangeLine = FString::Printf(TEXT("Range: %s units (fires a homing shot)"), *FormatStatValue(Row.Range));
		}
		else if (Row.Damage > 0.f)
		{
			RangeLine = FString::Printf(TEXT("Range: %s units (melee - it must close to contact)"), *FormatStatValue(Row.Range));
		}
		else
		{
			RangeLine = FString::Printf(TEXT("Range: %s units"), *FormatStatValue(Row.Range));
		}
		OutLines.Add(RangeLine);
	}

	if (Row.MinRange > 0.f)
	{
		OutLines.Add(FString::Printf(TEXT("Blind spot: it cannot hit anything closer than %s units"), *FormatStatValue(Row.MinRange)));
	}

	if (Row.Speed > 0.f)
	{
		OutLines.Add(FString::Printf(TEXT("Move speed: %s units per second"), *FormatStatValue(Row.Speed)));
	}
}

void UDeckBuilderWidget::AppendRuleLines(FName CardID, const FCardRow& Row, TArray<FString>& OutLines) const
{
	// --- the card's ROLE ------------------------------------------------------
	// Cards whose mechanic lives in a class rather than a row column. The CardID
	// keying mirrors the shipping play path's own keying (see the glossary CardID
	// block) — it is the same source of truth, not a second one.
	if (CardID == GlossaryCardID_Miner)
	{
		OutLines.Add(SiegeboundCardGlossary::MinerRole);
	}
	else if (CardID == GlossaryCardID_DeepMine)
	{
		OutLines.Add(SiegeboundCardGlossary::DeepMineRole);
	}
	else if (CardID == GlossaryCardID_Masons)
	{
		OutLines.Add(SiegeboundCardGlossary::MasonsRole);
	}
	else if (CardID == GlossaryCardID_Sorcerer)
	{
		// The ONE card whose row columns describe nothing: Damage/Range/Cadence are all
		// 0, so AppendStatLines prints no attack block (correctly — it has no attack)
		// and every keyword/spell/profile clause below is skipped. Without these two
		// lines a 60-gold card reads as three stats and no rules, which fails the truth
		// law in the OTHER direction from a false claim. Its mechanic lives entirely in
		// ASorcererUnit + AAncientGround, so it is CardID-keyed exactly like the three
		// role clauses above. TWO lines, the shipped multi-clause-role shape
		// (StructureRole + TowerRole; UpgradeSharpenedBlade + UpgradeTailFmt): the seal
		// is a permanent property of the unit, the boost is conditional on where it
		// stands, and one run-on sentence would bury the second.
		//
		// TASK-379 — the boost line's two magnitudes are DERIVED, never baked
		// (CONVENTIONS §8's preferred branch). CDO read: this is the deck-BUILDER, a
		// menu screen with no unit in the world, so the class DEFAULT is exactly the
		// right authority — it is the value every unit spawns with, and the one
		// Jonathan edits when he retunes the lever. (The gameplay/cheat paths read the
		// INSTANCE instead, so a per-Blueprint override is honoured there.)
		// GetDefault<T>() on a statically-linked UCLASS never returns null.
		const ASummonedUnit* UnitCDO = GetDefault<ASummonedUnit>();
		const float PerStackPercent = 100.f * UnitCDO->GetPermanentDamageBonusPerStack();
		const float CeilingPercent = PerStackPercent * static_cast<float>(UnitCDO->GetMaxPermanentDamageStacks());

		OutLines.Add(SiegeboundCardGlossary::SorcererRole);
		OutLines.Add(FString::Printf(SiegeboundCardGlossary::SorcererGroundBoostFmt,
			*FormatStatValue(PerStackPercent), *FormatStatValue(CeilingPercent)));
	}
	else if (Row.CardType == ECardType::HeroUpgrade)
	{
		const TCHAR* UpgradeLine = nullptr;
		if (CardID == GlossaryCardID_SharpenedBlade)
		{
			UpgradeLine = SiegeboundCardGlossary::UpgradeSharpenedBlade;
		}
		else if (CardID == GlossaryCardID_PlateArmor)
		{
			UpgradeLine = SiegeboundCardGlossary::UpgradePlateArmor;
		}
		else if (CardID == GlossaryCardID_SwiftBoots)
		{
			UpgradeLine = SiegeboundCardGlossary::UpgradeSwiftBoots;
		}
		else if (CardID == GlossaryCardID_WarBanner)
		{
			UpgradeLine = SiegeboundCardGlossary::UpgradeWarBanner;
		}

		// an unknown HeroUpgrade CardID says NOTHING rather than guess an effect the
		// hero cannot apply (the play path refuses it too)
		if (UpgradeLine)
		{
			OutLines.Add(UpgradeLine);
			// the stack cap IS the row's MaxCopies (the hero reads that same column),
			// so it is interpolated, never hardcoded
			OutLines.Add(FString::Printf(SiegeboundCardGlossary::UpgradeTailFmt, Row.MaxCopies));
		}
	}
	else if (Row.CardType == ECardType::Building)
	{
		OutLines.Add(SiegeboundCardGlossary::StructureRole);

		// a building that attacks is an auto-firing tower; its targeting rule is
		// unit/hero-only, which is exactly why no castle-scaling line follows below
		if (Row.Damage > 0.f && Row.Cadence > 0.f)
		{
			OutLines.Add(SiegeboundCardGlossary::TowerRole);
		}
	}

	// --- keyword clauses (CONVENTIONS composition order) ----------------------
	if (Row.bCharge)
	{
		OutLines.Add(SiegeboundCardGlossary::Charge);
	}

	if (Row.bSlayer)
	{
		OutLines.Add(SiegeboundCardGlossary::Slayer);
	}

	if (Row.bSuicide && Row.Damage > 0.f && Row.AoERadius > 0.f)
	{
		OutLines.Add(FString::Printf(SiegeboundCardGlossary::SuicideFmt,
			*FormatStatValue(Row.Damage), *FormatStatValue(Row.AoERadius)));
	}

	if (Row.SwarmCount > 1)
	{
		// > 1, not > 0: a SwarmCount of 1 is a single unit and the play path treats it
		// as one, so "one play puts 1 of them on the field" is never printed
		OutLines.Add(FString::Printf(SiegeboundCardGlossary::SwarmFmt, Row.SwarmCount));
	}

	if (Row.ChainTargets > 0)
	{
		// the EXACT falloff the tower applies: hit n takes Damage - n x ChainFalloff,
		// floored at 0. Derived from the row, so a balance edit to Damage or
		// ChainFalloff re-derives the printed sequence with no code change.
		FString FalloffSequence;
		for (int32 HitIndex = 0; HitIndex < Row.ChainTargets; ++HitIndex)
		{
			const float HitDamage = FMath::Max(
				Row.Damage - static_cast<float>(HitIndex) * static_cast<float>(Row.ChainFalloff), 0.f);
			if (HitIndex > 0)
			{
				FalloffSequence += TEXT(" / ");
			}
			FalloffSequence += FormatStatValue(HitDamage);
		}

		OutLines.Add(FString::Printf(SiegeboundCardGlossary::ChainFmt, Row.ChainTargets, *FalloffSequence));
	}

	// --- spawner --------------------------------------------------------------
	if (!Row.SpawnCardID.IsNone() && Row.SpawnInterval > 0.f)
	{
		// the spawned card's DISPLAY NAME, never the raw CardID — resolved through the
		// existing null-safe resolver (an unknown row falls back to its ID there)
		OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpawnerFmt,
			*GetCardDisplayName(Row.SpawnCardID), *FormatStatValue(Row.SpawnInterval)));

		if (Row.Lifetime > 0.f)
		{
			OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpawnerLifetimeFmt, *FormatStatValue(Row.Lifetime)));
		}
	}

	// --- spell effect + delivery ---------------------------------------------
	// Each effect prints ONLY when the row carries the magnitudes that effect needs —
	// the same well-formedness the resolver demands before it will resolve at all, so
	// a malformed row describes nothing rather than promising an effect that refuses.
	if (Row.SpellEffect != ESpellEffect::None)
	{
		// HOW it is delivered decides HOW its area reads, so resolve it first — through
		// the ONE delivery brain (it owns the sparse Auto default), never re-derived
		// here. The resolver only BRANCHES on delivery for the two area effects; the
		// others are reticle-placed whatever their cell says, so a line reading is only
		// ever taken for an effect that can actually be delivered as one.
		const bool bLineCapableEffect =
			(Row.SpellEffect == ESpellEffect::AoEDamage || Row.SpellEffect == ESpellEffect::Freeze);
		const bool bDeliversAsLine = bLineCapableEffect
			&& USpellLibrary::GetEffectiveDelivery(Row) == ESpellDelivery::HeroLine;

		switch (Row.SpellEffect)
		{
		case ESpellEffect::AoEDamage:
			// on the LINE path the row's radius plays no part at all (the corridor is
			// the bolt's own), so the circle wording would be a lie there
			if (Row.Damage > 0.f && bDeliversAsLine)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellAoEDamageLineFmt,
					*FormatStatValue(Row.Damage)));
			}
			else if (Row.Damage > 0.f && Row.AoERadius > 0.f)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellAoEDamageCircleFmt,
					*FormatStatValue(Row.Damage), *FormatStatValue(Row.AoERadius)));
			}
			break;

		case ESpellEffect::Freeze:
			if (Row.EffectDuration > 0.f && bDeliversAsLine)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellFreezeLineFmt,
					*FormatStatValue(Row.EffectDuration)));
			}
			else if (Row.AoERadius > 0.f && Row.EffectDuration > 0.f)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellFreezeCircleFmt,
					*FormatStatValue(Row.AoERadius), *FormatStatValue(Row.EffectDuration)));
			}
			break;

		case ESpellEffect::TopTargetsDamage:
			if (Row.MaxTargets > 0 && Row.AoERadius > 0.f && Row.Damage > 0.f)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellTopTargetsFmt,
					Row.MaxTargets, *FormatStatValue(Row.AoERadius), *FormatStatValue(Row.Damage)));
			}
			break;

		case ESpellEffect::AllyBuff:
			if (Row.AoERadius > 0.f && Row.EffectDuration > 0.f)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellAllyBuffFmt,
					*FormatStatValue(Row.AoERadius), *FormatStatValue(Row.EffectDuration)));
			}
			break;

		case ESpellEffect::GoldSteal:
			if (Row.GoldSteal > 0)
			{
				OutLines.Add(FString::Printf(SiegeboundCardGlossary::SpellGoldStealFmt, Row.GoldSteal));
			}
			break;

		default:
			break;
		}

		// The aiming line, from the same resolved delivery. GoldSteal resolves instantly
		// with no aim and no reticle at all, so it gets NO delivery line rather than a
		// wrong one.
		if (Row.SpellEffect != ESpellEffect::GoldSteal)
		{
			OutLines.Add(bDeliversAsLine
				? SiegeboundCardGlossary::DeliveryHeroLine
				: SiegeboundCardGlossary::DeliveryGroundCircle);
		}
	}

	// --- targeting profile ----------------------------------------------------
	if (Row.Profile == ECardProfile::Siege)
	{
		OutLines.Add(SiegeboundCardGlossary::ProfileSiege);
	}
	else if (Row.Profile == ECardProfile::Support && Row.Range > 0.f && Row.Damage > 0.f)
	{
		OutLines.Add(FString::Printf(SiegeboundCardGlossary::ProfileSupportFmt,
			*FormatStatValue(Row.Range), *FormatStatValue(Row.Damage)));
	}

	// --- castle / structure damage scaling ------------------------------------
	// Only stated where it can actually happen: a tower never targets a castle at all
	// (its own rule line says so), so no scaling line is printed for one.
	if (Row.Profile == ECardProfile::Siege)
	{
		OutLines.Add(SiegeboundCardGlossary::ScalingSiege);
	}
	else if (Row.CardType == ECardType::Unit && Row.bRanged && Row.Damage > 0.f)
	{
		OutLines.Add(SiegeboundCardGlossary::ScalingRangedVsCastle);
	}
	else if (Row.SpellEffect == ESpellEffect::AoEDamage && Row.Damage > 0.f)
	{
		OutLines.Add(SiegeboundCardGlossary::ScalingSpellVsCastle);
	}
}
