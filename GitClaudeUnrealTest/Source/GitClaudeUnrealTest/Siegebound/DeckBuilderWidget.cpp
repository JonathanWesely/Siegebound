// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DeckBuilderWidget.h"

#include "Components/HorizontalBox.h"     // TASK-671: the DeckBar container (DECK-§7)
#include "Components/HorizontalBoxSlot.h" // TASK-671: per-entry slot rules on the bar row
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h" // TASK-602: UGameInstance::GetSubsystem — resolve the ACC-§4 account seam at call time
#include "Engine/Texture2D.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckLibrary.h"
#include "Siegebound/DeckSlotEntryWidget.h" // TASK-671: the code-authored bar entry (DECK-§5)
#include "Siegebound/SiegeAccountSubsystem.h" // TASK-602: USiegeAccountSubsystem — the ACC-§4 deck-slot seam (the class is TASK-600's, landing in the same batch — the TASK-442 parallel-header precedent)
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SpellLibrary.h"
#include "Siegebound/SummonedUnit.h" // TASK-379: GetDefault<ASummonedUnit>() needs the COMPLETE type for the two Sorcerer boost getters

// ---------------------------------------------------------------------------
// TASK-602 (ACC-§4 seam law): the deck save slot resolves AT CALL TIME through
// USiegeAccountSubsystem — the profile-scoped slot when a profile is active,
// the bare guest constant otherwise. Fail-safe: an unresolvable GameInstance or
// subsystem yields USiegeDeckSaveGame::SlotName (guest — byte-identical to the
// pre-account behavior, never a crash). Deliberately NO cached slot member and
// no reload machinery: the GameInstance outlives OpenLevel, so a main-menu
// login is naturally live at every later read (ACC-§4 deck-lane clause).
// USiegeDeckSaveGame::SlotName / ::UserIndex themselves stay byte-identical.
// ---------------------------------------------------------------------------
namespace
{
	FString ResolveDeckSlotName(const UGameInstance* GameInstance)
	{
		if (GameInstance)
		{
			if (const USiegeAccountSubsystem* AccountSubsystem = GameInstance->GetSubsystem<USiegeAccountSubsystem>())
			{
				return AccountSubsystem->GetDeckSlotName();
			}
		}
		return USiegeDeckSaveGame::SlotName; // guest fail-safe (ACC-§4)
	}
}

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

	/**
	 *  ⭐⭐ FogCover — the `Fog` card (TASK-999; FOG-§9.1 "completely universal … it will cover the
	 *  ENTIRE battlefield", FOG-§10.3 the state machine, J-F16 refresh-never-stack, J-F19 the
	 *  net-zero refusal). ⛔ UNCONDITIONAL: it names no row magnitude, so this clause CANNOT come
	 *  out blank — which is the whole failure this row exists to close (a card that renders with an
	 *  empty effect line describes nothing to the one person reading it).
	 *
	 *  ⛔⛔ NO NUMBER FOR THE VISION CUT, AND THAT IS DELIBERATE (`SC-§65` — pin the SHAPE, not the
	 *  magnitude). The cut is `FSiegeFogStatics::EffectiveVisionRadius` = min(requested,
	 *  FSiegeFogTuning::FogVisionCeilingUU), and that ceiling is `EditDefaultsOnly` precisely so
	 *  Jonathan can retune it "with no code change and no recompile" (SiegeFogStatics.h says so in
	 *  those words). ⇒ a literal "609.6 units" baked here would go false on a retune that is
	 *  DESIGNED to touch no code, and nothing would report it. The SHAPE — ranged reach collapses,
	 *  melee is untouched — holds for every ceiling the editor will accept (its ClampMin is the
	 *  10 ft onset, still far above the 120 uu melee request), so it is what is stated.
	 *  ⛔ And this file does NOT include SiegeFogStatics.h to interpolate the value:
	 *  SiegeCombatStatics.cpp's own include comment asserts that header is "consumed HERE and in
	 *  no other translation unit", and a second consumer would falsify a claim in a file this row
	 *  does not own.
	 */
	const TCHAR SpellFogCover[] = TEXT("Raises fog over the ENTIRE battlefield the instant you play it - there is no spot to aim at and no radius. While it hangs, every unit on BOTH sides, yours included, only notices enemies close to it: ranged units and towers lose their reach and the fight collapses to arm's length, while melee units are barely affected. Playing it again while it is already up resets the clock rather than adding to it. While the sky is being held clear against fog, playing it is refused outright and costs you nothing.");

	/**
	 *  FogCover's duration; %s = row EffectDuration.
	 *  ⚠️ DECLARED, because it is a real seam rather than a detail: the MECHANISM reads
	 *  AFogVolume::FogDurationSeconds off the CDO and does NOT read this cell today (CardRow.h's
	 *  ESpellEffect::FogCover block says so, and TASK-1016 is the row that makes the cell
	 *  authoritative). The two agree at 300 by CONTRACT, not by coincidence — FogVolume.h states
	 *  "`EffectDuration` on the `Fog` card row MUST match this" — so printing the ROW's own cell is
	 *  both the data-driven branch (GDD §3.0: never hardcode a magnitude this widget can read) and
	 *  the value that stays right when TASK-1016 wires the cell up. ⛔ Its own clause, gated on the
	 *  cell, so a blank cell costs a SENTENCE and never the whole description.
	 */
	constexpr TCHAR SpellFogCoverDurationFmt[] = TEXT("The fog lifts on its own after %s seconds.");

	/**
	 *  ⭐⭐ FogClear — the `BrightSun` card (TASK-999; FOG-§10.1 the card, FOG-§10.3 the three-state
	 *  machine, J-F17 legal with no fog up, J-F14 uncapped, J-F15 sampled once at the cast, J-F18
	 *  the shortening cast is refused, and his one-way door: "even when the … timer ends, the fog
	 *  that was cleared STILL REMAINS CLEAR"). ⛔ UNCONDITIONAL, for the same reason as FogCover.
	 *
	 *  ⛔⛔ NO HEIGHT NUMBERS, AND THE EVIDENCE IS ALREADY ON THE RECORD RATHER THAN THEORETICAL:
	 *  the step was AMENDED BY JONATHAN ON 2026-09-04 FROM 20 ft TO 50 ft (FogVolume.h's
	 *  BrightSunHeightStepUU block). A glossary line that had said "20 feet" would already be a
	 *  lie, in a file nobody would have thought to re-read. `SC-§65`: state the shape - higher is
	 *  longer, read once, no cap - which survives every retune of the three EditDefaultsOnly
	 *  levers that compose the window.
	 */
	const TCHAR SpellFogClear[] = TEXT("Clears every trace of fog the instant you play it - there is no spot to aim at and no radius - and then holds the sky clear, refusing any new fog for a while afterwards. It is worth playing with no fog up at all: that refusal window on its own is half the card.");

	/**
	 *  FogClear's window rules — the half that decides matches, kept as its own line so it reads
	 *  as rules rather than as one long paragraph. ⛔ UNCONDITIONAL (no row magnitude).
	 */
	const TCHAR SpellFogClearWindowRules[] = TEXT("The higher above the flat ground you stand at the moment you cast it, the longer that window runs - your height is read once, at the cast, and there is no upper limit. When the window finally ends the battlefield STAYS clear: fog never returns on its own. Casting it again from lower ground while a window is still running would shorten it, so that play is refused outright and costs you nothing.");

	/**
	 *  FogClear's floor; %s = row EffectDuration. Same seam as SpellFogCoverDurationFmt: the
	 *  MECHANISM reads AFogVolume::BrightSunBaseDurationSeconds, and FOG-§10.1 pins the card row's
	 *  EffectDuration to the same 120 from a second sentence of his. Gated on the cell.
	 */
	constexpr TCHAR SpellFogClearBaseFmt[] = TEXT("Cast from the flat ground it holds fog off for %s seconds, before any height on top of that.");

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
	 *  (U+00B7): "<Type> [dot] Cost <n> gold" (the "Max <n> per deck" clause was
	 *  deleted — CARD-UNCAP 2026-08-28, UNCAP-§5). Composed
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

// ---------------------------------------------------------------------------
// ⭐⭐ THE SPELL COMPOSER (TASK-999). Two defects were repaired here at once, and
// they are the SAME defect wearing two faces: the effect `switch` had no arm for
// the fog effects (a BLANK line), and the aiming guard was a BLACKLIST that then
// printed a reticle sentence anyway (a FALSE line).
//
// ⚖️ A BLANK LINE IS A GAP; A WRONG LINE IS A LIE. `Fog` read, verbatim, as
// "Aimed at a spot on the ground: it goes off where you place the reticle." for a
// no-reticle, map-wide fog, in the one surface Jonathan reads while building a
// deck.
//
// ⛔⛔ WHY IT IS A FREE FUNCTION IN THIS NAMESPACE RATHER THAN A MEMBER: it is
// pure row-in / lines-out — it touches no member of UDeckBuilderWidget — and
// AppendRuleLines, its only shipping caller, is `private:` in a header this row
// does not own. External linkage here is what lets Tests/SiegeCardGlossaryTest.cpp
// forward-declare and CALL the real composer, so the assertion item (3) demands
// tests behaviour instead of source text. ⛔ It is deliberately NOT in the
// anonymous namespace directly above for exactly that reason: internal linkage
// there would make it unreachable from any other translation unit, and the gate
// would come back as a link error somebody "fixes" by deleting the test.
// ---------------------------------------------------------------------------
namespace SiegeboundCardGlossary
{
	void AppendSpellLines(const FCardRow& Row, TArray<FString>& OutLines)
	{
		if (Row.SpellEffect == ESpellEffect::None)
		{
			return; // not a spell row: the card's other clauses describe it
		}

		// HOW it is delivered decides HOW its area reads, so resolve it first — through
		// the ONE delivery brain (it owns the sparse Auto default), never re-derived
		// here.
		//
		// ⛔ COLLAPSED BY TASK-1018, AND THE EQUIVALENCE IS EXACT RATHER THAN
		// APPROXIMATE — say it here because it looks like a behaviour change and is not.
		// This used to be `bLineCapableEffect && ResolvedDelivery == HeroLine`, where
		// `bLineCapableEffect` was `SpellEffect == AoEDamage || == Freeze` (a mirror of
		// USpellLibrary::ResolveSpell's own branch set). ⭐ THE ONLY TWO PLACES THIS
		// FLAG IS READ ARE `case ESpellEffect::AoEDamage:` AND `case ESpellEffect::Freeze:`
		// BELOW — inside which `bLineCapableEffect` is TRUE BY CONSTRUCTION, since the
		// switch has already established the effect. ⇒ the conjunction was a tautology at
		// both of its use sites, and dropping it changes no output for any row.
		// ⇒ WHY IT MATTERS: the mirror now lives in ONE place
		// (USpellLibrary::SpellRequiresAiming), so it cannot drift between two files, and
		// what remains here is exactly USpellLibrary::IsLineDeliverySpell's question —
		// the same question ASiegePlayerController's targeting aim pass asks.
		const ESpellDelivery ResolvedDelivery = USpellLibrary::GetEffectiveDelivery(Row);
		const bool bDeliversAsLine = (ResolvedDelivery == ESpellDelivery::HeroLine);

		// ⛔⛔ THE AIM GATE — DERIVED, NOT LISTED, and as of TASK-1018 it is CALLED
		// rather than spelled out here. The three local bools that used to live at this
		// spot moved VERBATIM into USpellLibrary::SpellRequiresAiming; nothing about the
		// derivation changed, and the header there carries the full rationale (why it is
		// not `ResolvedDelivery == GroundCircle`, why ESpellDelivery cannot answer the
		// question alone, and the cell-then-aim-evidence precedence order).
		//
		// ⛔⛔ WHY IT MOVED, IN ONE SENTENCE, BECAUSE THE NEXT READER WILL WANT TO INLINE
		// IT BACK: ASiegePlayerController's card-play routing asks THIS EXACT QUESTION to
		// decide between targeting mode and an instant resolve, and it used to answer it
		// with its own hard-coded `== ESpellEffect::GoldSteal`. ⚖️ TWO DERIVATIONS OF ONE
		// FACT DO NOT STAY EQUAL — the divergence would present as this panel and the
		// game DISAGREEING, i.e. as two bugs instead of one. ⇒ one definition, two
		// consumers. ⛔ Do not re-inline it, and do not "simplify" it to a delivery
		// comparison that looks equivalent.
		//
		// Measured against the shipped roster, by CardID: Lightning (700) and BattleCry
		// (400) keep the reticle line; Pickpocket (0), Fog (0) and BrightSun (0) lose it
		// — Pickpocket by CONSTRUCTION rather than by being named.
		const bool bAimed = USpellLibrary::SpellRequiresAiming(Row);

		// Each effect prints ONLY when the row carries the magnitudes that effect needs —
		// the same well-formedness the resolver demands before it will resolve at all, so
		// a malformed row describes nothing rather than promising an effect that refuses.
		//
		// ⛔⛔ THERE IS NO `default:` ARM, AND ITS ABSENCE IS THE STRUCTURAL HALF OF THIS
		// ROW. ⚖️ A `default:` CONVERTS A COMPILER ERROR INTO A USER-VISIBLE BLANK — it
		// trades a failure the BUILD catches for one only a PLAYER catches, which is
		// exactly how `Fog` shipped describing nothing. Every declared value is listed,
		// `None` included, so appending an ESpellEffect value is a diagnostic on every
		// toolchain that warns on an unhandled enumerator. ⭐ The belt for the toolchains
		// that do not warn is Tests/SiegeCardGlossaryTest.cpp, which iterates
		// StaticEnum<ESpellEffect>() and fails on the first value that composes nothing.
		switch (Row.SpellEffect)
		{
		case ESpellEffect::None:
			break; // unreachable (guarded above); listed so the switch stays exhaustive

		case ESpellEffect::AoEDamage:
			// on the LINE path the row's radius plays no part at all (the corridor is
			// the bolt's own), so the circle wording would be a lie there
			if (Row.Damage > 0.f && bDeliversAsLine)
			{
				OutLines.Add(FString::Printf(SpellAoEDamageLineFmt,
					*FormatStatValue(Row.Damage)));
			}
			else if (Row.Damage > 0.f && Row.AoERadius > 0.f)
			{
				OutLines.Add(FString::Printf(SpellAoEDamageCircleFmt,
					*FormatStatValue(Row.Damage), *FormatStatValue(Row.AoERadius)));
			}
			break;

		case ESpellEffect::Freeze:
			if (Row.EffectDuration > 0.f && bDeliversAsLine)
			{
				OutLines.Add(FString::Printf(SpellFreezeLineFmt,
					*FormatStatValue(Row.EffectDuration)));
			}
			else if (Row.AoERadius > 0.f && Row.EffectDuration > 0.f)
			{
				OutLines.Add(FString::Printf(SpellFreezeCircleFmt,
					*FormatStatValue(Row.AoERadius), *FormatStatValue(Row.EffectDuration)));
			}
			break;

		case ESpellEffect::TopTargetsDamage:
			if (Row.MaxTargets > 0 && Row.AoERadius > 0.f && Row.Damage > 0.f)
			{
				OutLines.Add(FString::Printf(SpellTopTargetsFmt,
					Row.MaxTargets, *FormatStatValue(Row.AoERadius), *FormatStatValue(Row.Damage)));
			}
			break;

		case ESpellEffect::AllyBuff:
			if (Row.AoERadius > 0.f && Row.EffectDuration > 0.f)
			{
				OutLines.Add(FString::Printf(SpellAllyBuffFmt,
					*FormatStatValue(Row.AoERadius), *FormatStatValue(Row.EffectDuration)));
			}
			break;

		case ESpellEffect::GoldSteal:
			if (Row.GoldSteal > 0)
			{
				OutLines.Add(FString::Printf(SpellGoldStealFmt, Row.GoldSteal));
			}
			break;

		case ESpellEffect::FogCover:
			// ⛔ THE LEAD CLAUSE IS UNCONDITIONAL, ON PURPOSE. Every arm above gates on a
			// magnitude and can therefore compose NOTHING; this effect reads no magnitude
			// to resolve (AFogVolume owns its duration), so gating it on the row would
			// reintroduce the blank line this row exists to delete.
			OutLines.Add(SpellFogCover);
			if (Row.EffectDuration > 0.f)
			{
				OutLines.Add(FString::Printf(SpellFogCoverDurationFmt,
					*FormatStatValue(Row.EffectDuration)));
			}
			break;

		case ESpellEffect::FogClear:
			// Unconditional for the same reason, and the window RULES are a separate
			// clause from the window LENGTH because only the length lives in the row.
			OutLines.Add(SpellFogClear);
			OutLines.Add(SpellFogClearWindowRules);
			if (Row.EffectDuration > 0.f)
			{
				OutLines.Add(FString::Printf(SpellFogClearBaseFmt,
					*FormatStatValue(Row.EffectDuration)));
			}
			break;
		}

		// The aiming line, from the same resolved delivery — printed only for a row that
		// is actually AIMED (see the derivation above). A spell that resolves globally
		// gets NO delivery line rather than a wrong one.
		//
		// ⛔⛔ THE ONE-LINE REPAIR NAMED IN `qa/TASK-1013.md` WARN-1, MADE BY TASK-1018 AS
		// THE SECOND CONSUMER OF THIS DERIVATION. The selector used to carry an extra
		// LINE-CAPABLE-EFFECT conjunct, and it was wrong for exactly one shape: an
		// AUTHORED `HeroLine` cell on a non-line-capable effect printed the GROUND-CIRCLE
		// sentence — ⚖️ THE COMPOSER CONTRADICTING THE CELL IN PRECISELY THE CASE WHOSE
		// WHOLE RATIONALE IS "AGREE WITH THE DATA". ⛔ Unreachable by shipped data today
		// (no row authors that combination), which is exactly why it was worth closing
		// before card authoring turned the lock: an edge that only DATA can reach is a
		// defect with a data-shaped lock on it, not a hypothetical.
		// ⭐ AND IT NOW AGREES WITH THE GAME AS WELL AS WITH THE CELL: this is
		// `USpellLibrary::IsLineDeliverySpell`'s question, the same one
		// ASiegePlayerController's targeting aim pass gates on — so the sentence the
		// player reads and the confirm behaviour they get cannot disagree for any row.
		if (bAimed)
		{
			OutLines.Add(bDeliversAsLine ? DeliveryHeroLine : DeliveryGroundCircle);
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
// The ten-slot model (TASK-670 — CONVENTIONS DECK-§1..§4, signatures DECK-§8)
// ---------------------------------------------------------------------------

void UDeckBuilderWidget::NativeConstruct()
{
	// Super FIRST, deliberately: UUserWidget::NativeConstruct fires the WBP's
	// Event Construct, so any legacy graph-side seed (the seed-then-bind
	// LoadDefaultDeck call) runs BEFORE the model init below and is simply
	// overwritten by it. It cannot clobber a saved slot either: EditingDeckIndex
	// is still INDEX_NONE at that point, so the PersistWorkingDeck guard refuses
	// the auto-save (one Warning — the visible signal that the graph seed is
	// redundant and TASK-672 should cut it per the TASK-669 record).
	Super::NativeConstruct();

	// DECK-§2: the ONE shipping migration call site — builder-open-time only;
	// the match path (SiegePlayerController) never migrates.
	if (USiegeDeckSaveGame* SaveObj = LoadOrCreateSaveGame())
	{
		if (USiegeDeckSaveGame::MigrateToFixedSlots(*SaveObj))
		{
			// caller persists (the migration itself never touches disk) — the
			// ACC-§4 call-time seam, same shape as SaveDeckAs/SetActiveDeck
			const FString DeckSlotName = ResolveDeckSlotName(GetGameInstance());
			if (UGameplayStatics::SaveGameToSlot(SaveObj, DeckSlotName, USiegeDeckSaveGame::UserIndex))
			{
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("UDeckBuilderWidget: migrated slot '%s' to the %d fixed decks (active '%s')."),
					*DeckSlotName, USiegeDeckSaveGame::NumFixedDeckSlots, *SaveObj->ActiveDeckName);
			}
			else
			{
				// non-fatal: the migration is idempotent, so the next builder
				// open simply re-runs it (DECK-§2)
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("UDeckBuilderWidget: SaveGameToSlot('%s') failed — migration NOT persisted; it will re-run next open."),
					*DeckSlotName);
			}
		}
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::NativeConstruct: could not load or create the deck SaveGame — the ten-slot model is unavailable this session."));
	}

	// DECK-§3: the builder opens on the ACTIVE deck selected for editing.
	// GetActiveDeckIndex never returns INDEX_NONE (deck1 fallback), so this
	// always selects a real slot.
	SelectDeckForEdit(GetActiveDeckIndex());

	// ------------------------------------------------------------------------
	// TASK-671 (DECK-§5): the deck bar — ten code-authored UDeckSlotEntryWidget
	// entries in slot order, populated into the WBP-authored DeckBar container.
	// Built AFTER the model init above so the first RefreshDeckBarStates below
	// renders the true post-migration active/editing pair. Idempotent across
	// re-adds to the viewport: cleared and rebuilt every NativeConstruct.
	// ------------------------------------------------------------------------
	if (DeckBar != nullptr)
	{
		DeckBar->ClearChildren();
		DeckBarEntries.Reset();
		DeckBarEntries.Reserve(USiegeDeckSaveGame::NumFixedDeckSlots);

		for (int32 SlotIndex = 0; SlotIndex < USiegeDeckSaveGame::NumFixedDeckSlots; ++SlotIndex)
		{
			UDeckSlotEntryWidget* Entry = CreateWidget<UDeckSlotEntryWidget>(this);
			if (Entry == nullptr)
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("UDeckBuilderWidget: could not create the deck-bar entry for slot %d - that slot is missing from the bar this session."),
					SlotIndex);
				continue;
			}

			// The label is the ONE composer's name (DECK-§1: "deck1".."deck10"
			// are never hand-typed — label = save key = cloud key, triple duty).
			Entry->SetSlotIndexAndLabel(SlotIndex, USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex));

			// Gesture routing (D7 / DECK-§3): LEFT selects the slot for
			// EDITING, RIGHT makes it the ACTIVE match deck. Both bind straight
			// onto the model UFUNCTIONs — the same surfaces TASK-674 drives
			// directly — whose own success paths refresh the bar states, so a
			// gesture and a direct call render identically.
			Entry->OnLeftClicked.BindUObject(this, &UDeckBuilderWidget::SelectDeckForEdit);
			Entry->OnRightClicked.BindUObject(this, &UDeckBuilderWidget::SetActiveDeckBySlot);

			if (UHorizontalBoxSlot* EntrySlot = DeckBar->AddChildToHorizontalBox(Entry))
			{
				// Ten equal Fill shares: the bar divides ANY window width
				// evenly, so all ten entries stay visible at every size the
				// DECK-§6 pixel gate captures — the row never clips an entry.
				EntrySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				EntrySlot->SetPadding(FMargin(2.f, 2.f));
				EntrySlot->SetHorizontalAlignment(HAlign_Fill);
				EntrySlot->SetVerticalAlignment(VAlign_Fill);
			}

			DeckBarEntries.Add(Entry);
		}

		RefreshDeckBarStates();
	}
	else
	{
		// DECK-§5: WBP_DeckBuilder does not carry the DeckBar container yet
		// (TASK-672 authors it in parallel). One Warning, skip the bar —
		// everything else in the builder keeps working. Never a crash.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget: no DeckBar container bound (WBP_DeckBuilder not updated yet?) - the deck bar is skipped this session."));
	}
}

void UDeckBuilderWidget::SelectDeckForEdit(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SelectDeckForEdit(%d): not a fixed slot (0..%d) — refused."),
			SlotIndex, USiegeDeckSaveGame::NumFixedDeckSlots - 1);
		return;
	}

	const FString FixedName = USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex);

	// Load the slot's saved deck into the working model. Post-migration the
	// slot always exists (DECK-§1 always-materialized); the null-safe fallback
	// (no save yet / pre-migration) is an EMPTY working deck stamped with the
	// fixed name — DECK-§3's "empty slot => empty working deck", never a crash.
	WorkingDeck.Cards.Reset();
	WorkingDeck.DeckName = FixedName;
	if (const USiegeDeckSaveGame* SaveObj = LoadSaveGame())
	{
		for (const FDeckList& Deck : SaveObj->SavedDecks)
		{
			if (Deck.DeckName.Equals(FixedName, ESearchCase::IgnoreCase))
			{
				WorkingDeck = Deck;
				WorkingDeck.DeckName = FixedName; // canonical lowercase even off a pre-migration save
				break;
			}
		}
	}

	EditingDeckIndex = SlotIndex;

	// No disk write here: selecting changes WHICH deck is edited, not any
	// deck's content — content mutations reach disk through PersistWorkingDeck
	// (DECK-§4), and the editing selection itself is transient by construction
	// (DECK-§1: the save class gains no field; ActiveDeckName is the only
	// persisted selection and it belongs to SetActiveDeckBySlot).
	OnDeckModelChanged();

	// TASK-671: the EDITING index moved — retint the bar (fill tint follows the
	// edited slot, DECK-§3). Refusals above returned before any state change,
	// so they redraw nothing. No-op until the bar exists (NativeConstruct calls
	// this function BEFORE the bar build; the build's own refresh catches up).
	RefreshDeckBarStates();
}

int32 UDeckBuilderWidget::GetEditingDeckIndex() const
{
	return EditingDeckIndex;
}

void UDeckBuilderWidget::SetActiveDeckBySlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SetActiveDeckBySlot(%d): not a fixed slot (0..%d) — refused."),
			SlotIndex, USiegeDeckSaveGame::NumFixedDeckSlots - 1);
		return;
	}

	// Delegate to the EXISTING strict activation path (byte-compatible API,
	// DECK-§4): deck-exists check, canonical-name store, ACC-§4 call-time slot
	// seam, persist, OnDeckModelChanged — reused, never reimplemented. Post-
	// migration every fixed slot exists, so the strict check always passes;
	// pre-migration (edge) it warns and no-ops, never dangles ActiveDeckName.
	SetActiveDeck(USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex));
}

int32 UDeckBuilderWidget::GetActiveDeckIndex() const
{
	if (const USiegeDeckSaveGame* SaveObj = LoadSaveGame())
	{
		const int32 SlotIndex = USiegeDeckSaveGame::FindFixedDeckIndex(SaveObj->ActiveDeckName);
		if (SlotIndex != INDEX_NONE)
		{
			return SlotIndex;
		}
	}

	// deck1 — the DECK-§3 default: fresh account, pre-migration legacy name,
	// or no save at all. Never INDEX_NONE, so the builder always opens on a
	// real slot. The MATCH side needs no parallel of this — its own curated-
	// default fallback (TASK-114) already absorbs every unresolvable case (D5).
	return 0;
}

void UDeckBuilderWidget::PersistWorkingDeck()
{
	// THE one auto-save funnel (DECK-§4 / fix 3). Everything routes through the
	// EXISTING SaveDeckAs path so the ACC-§4 call-time slot seam (profile-
	// scoped when logged in, guest otherwise) is inherited, never duplicated.
	if (EditingDeckIndex < 0 || EditingDeckIndex >= USiegeDeckSaveGame::NumFixedDeckSlots)
	{
		// A mutation arrived before NativeConstruct selected a slot (e.g. a
		// legacy WBP Event Construct seed). Nothing is lost — the working deck
		// stays in memory — but nothing is written to a slot nobody chose.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::PersistWorkingDeck: no editing slot selected yet — mutation NOT auto-saved (a pre-construct graph seed is the known benign cause)."));
		return;
	}

	SaveDeckAs(USiegeDeckSaveGame::MakeFixedDeckName(EditingDeckIndex));
}

void UDeckBuilderWidget::RefreshDeckBarStates()
{
	// TASK-671 (DECK-§3): the ONE bar-state redraw. Outline = ACTIVE only, fill
	// tint = EDITING only — two separate visual channels writable only through
	// the entry's two pinned setters, so the states coexist on one entry
	// without ever conflating (a second outline is the confusable-signal
	// defect class the law forbids). Idempotent; silent no-op when the bar was
	// never built (DeckBar unbound, or an offline-test widget with no tree).
	if (DeckBarEntries.Num() == 0)
	{
		return;
	}

	// One save read per refresh (not per entry). GetActiveDeckIndex never
	// returns INDEX_NONE (deck1 fallback, DECK-§3), so exactly one entry gets
	// the orange; EditingDeckIndex can be INDEX_NONE only pre-construct, when
	// no bar exists to refresh.
	const int32 ActiveIndex = GetActiveDeckIndex();

	for (int32 SlotIndex = 0; SlotIndex < DeckBarEntries.Num(); ++SlotIndex)
	{
		if (UDeckSlotEntryWidget* Entry = DeckBarEntries[SlotIndex])
		{
			Entry->SetOutlineActive(SlotIndex == ActiveIndex);
			Entry->SetEditingHighlight(SlotIndex == EditingDeckIndex);
		}
	}
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

	// unknown-card gate (§3.0): the row must resolve in DT_Cards — refuse rather
	// than add a card legality would then reject (ResolveCardRow logs the fault
	// once). CARD-UNCAP 2026-08-28 (UNCAP-§4): the Current >= Row->MaxCopies
	// refusal that used to follow is DELETED — any count of a resolvable card
	// may be added. Deliberately NO add-time deck-total guard either (U4):
	// over-50 WORKING decks were already reachable and are handled by the live
	// x/50 counter + the exactly-50 legality gate.
	const FCardRow* Row = ResolveCardRow(CardID);
	if (!Row)
	{
		return;
	}

	const int32 Current = GetCountOf(CardID);
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

	// DECK-§4 auto-save: a SUCCESSFUL add persists immediately (the refusals
	// above returned before any broadcast, so they save nothing). SaveDeckAs
	// fires one more OnDeckModelChanged on success — a redundant re-read, never
	// a wrong one (the getters are the single source the WBP renders from).
	PersistWorkingDeck();
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

	// DECK-§4 auto-save: a SUCCESSFUL remove persists immediately (the
	// remove-at-0 no-op returned before any broadcast and saves nothing).
	PersistWorkingDeck();
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

	// D9 (DECK-§4): under auto-save, Reset persists the curated default to the
	// EDITING slot immediately — SaveDeckAs also re-stamps WorkingDeck.DeckName
	// with the fixed slot name the Reset() above cleared. Pre-construct callers
	// (the legacy WBP Event Construct seed) hit the funnel's no-slot guard and
	// write nothing.
	PersistWorkingDeck();
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
	// CARD-UNCAP 2026-08-28 (UNCAP-§4) COMPAT SHIM: per-card deck copy caps are
	// abolished, but this BlueprintPure signature is pinned (WBP_DeckCardTile
	// greys the "+" when GetCountOf >= this — no WBP graph edit needed). A
	// resolved row now reports the only per-card bound left, the deck size
	// itself (SiegeLegalDeckSize = 50), so the "+" greys exactly at
	// 50-of-one-card. Missing table/row still returns 0 (unchanged).
	const FCardRow* Row = ResolveCardRow(CardID);
	return Row ? SiegeLegalDeckSize : 0;
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

	// TASK-602 (ACC-§4): resolved at call time — profile-scoped when logged in, guest otherwise.
	// The two logs below print the RESOLVED slot so they never lie about which slot was written.
	const FString DeckSlotName = ResolveDeckSlotName(GetGameInstance());
	if (!UGameplayStatics::SaveGameToSlot(SaveObj, DeckSlotName, USiegeDeckSaveGame::UserIndex))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SaveDeckAs: SaveGameToSlot('%s') failed — deck '%s' NOT persisted."),
			*DeckSlotName, *Trimmed);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("UDeckBuilderWidget: saved deck '%s' (%d cards) to slot '%s'."),
		*Trimmed, ToSave.TotalCount(), *DeckSlotName);

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
	// and falls back to the DeckCount default null-safe if it ever fails to resolve).
	//
	// TASK-1270 (DECK-§3 rider): ...and only a LEGAL one. The exists-check and
	// the legality gate are ONE in-memory step (TryActivateSavedDeck — the
	// same UDeckLibrary::IsDeckLegal the match reader runs, never a second
	// rule), taken BEFORE anything is written: a refusal leaves ActiveDeckName
	// exactly as it was, so the orange rim (derived from it by
	// GetActiveDeckIndex) stays where it was, and no disk write happens.
	// MEASURED CAUSE (qa/TASK-1068-verify.md, the TASK-1230 R-DECK ruling): a
	// 68-card deck1 could be right-clicked active, and every match then dealt
	// the curated default at Warning level with nothing on screen. Refusal
	// surface = the builder's refused-save idiom (one Warning naming the
	// reason) + OnDeckActivationRefused for a WBP that wants to show it.
	FString CanonicalName;
	FString RefusalReason;
	if (!TryActivateSavedDeck(*SaveObj, ResolveCardTable(), Name, CanonicalName, RefusalReason))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SetActiveDeck('%s'): %s — refused; the active deck stays '%s'."),
			*Name, *RefusalReason, *SaveObj->ActiveDeckName);
		OnDeckActivationRefused(Name, RefusalReason);
		return;
	}

	// TASK-602 (ACC-§4): resolved at call time — profile-scoped when logged in, guest otherwise
	const FString DeckSlotName = ResolveDeckSlotName(GetGameInstance());
	if (!UGameplayStatics::SaveGameToSlot(SaveObj, DeckSlotName, USiegeDeckSaveGame::UserIndex))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UDeckBuilderWidget::SetActiveDeck('%s'): SaveGameToSlot failed — active deck NOT persisted."), *CanonicalName);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("UDeckBuilderWidget: active deck set to '%s' — the next match will use it."), *CanonicalName);

	OnDeckModelChanged();

	// TASK-671: the ACTIVE deck moved — the orange outline follows it
	// (DECK-§3). Appended on the SUCCESS path only (the refusals above changed
	// nothing); this one site covers SetActiveDeckBySlot's right-click lane AND
	// the shipped D8 "Play with this deck" activation, so the orange follows
	// both. Existing behavior above is untouched (DECK-§4 byte-compatibility).
	RefreshDeckBarStates();
}

bool UDeckBuilderWidget::TryActivateSavedDeck(USiegeDeckSaveGame& Save, const UDataTable* CardTable, const FString& Name, FString& OutCanonicalName, FString& OutRefusalReason)
{
	// TASK-1270 — the activation gate (header: the three steps). Pure over the
	// in-memory save: no disk, no seam, no widget state, so it is assertable
	// on STATE from Tests/SiegeDeckSlotsTest.cpp (SC-§104) without touching
	// the player's slot. Every false return leaves Save untouched.
	OutCanonicalName.Reset();
	OutRefusalReason.Reset();

	// (1) the exists-check — the shipped M6 case-insensitive lookup, verbatim
	const FDeckList* Found = nullptr;
	for (const FDeckList& Deck : Save.SavedDecks)
	{
		if (Deck.DeckName.Equals(Name, ESearchCase::IgnoreCase))
		{
			Found = &Deck;
			break;
		}
	}

	if (!Found)
	{
		OutRefusalReason = TEXT("no saved deck with that name — save it first (SaveDeckAs)");
		return false;
	}

	// (2) THE legality home (UNCAP-§3: IsDeckLegal is untouched — total exactly
	// 50, every CardID resolvable, no negative count). The reason travels
	// VERBATIM so the refusal names the actual violation ("Deck has 68 cards —
	// a legal deck is exactly 50 (GDD 3.4).").
	if (!UDeckLibrary::IsDeckLegal(CardTable, *Found, OutRefusalReason))
	{
		return false;
	}

	// (3) legal ⇒ the canonical STORED name becomes the active deck (in memory;
	// the caller persists)
	OutCanonicalName = Found->DeckName;
	Save.ActiveDeckName = OutCanonicalName;
	return true;
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
	// TASK-602 (ACC-§4): ONE resolve per operation, so the exist-check and the
	// load below can never straddle a profile change — call-time scoping, no cache
	const FString DeckSlotName = ResolveDeckSlotName(GetGameInstance());

	// DoesSaveGameExist first so the normal first-run (no save yet) is SILENT —
	// LoadGameFromSlot on a missing slot would otherwise log an engine warning
	if (!UGameplayStatics::DoesSaveGameExist(DeckSlotName, USiegeDeckSaveGame::UserIndex))
	{
		return nullptr;
	}

	// null-check the cast (TASK-113 carry-forward): a corrupt/foreign slot casts to
	// nullptr rather than crashing
	return Cast<USiegeDeckSaveGame>(
		UGameplayStatics::LoadGameFromSlot(DeckSlotName, USiegeDeckSaveGame::UserIndex));
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
	// CONVENTIONS: "<Type> · Cost <n> gold". CARD-UNCAP 2026-08-28 (UNCAP-§5,
	// truth law): the "Max <n> per deck" clause is DELETED — per-card deck caps
	// no longer exist. The hero-upgrade RULES tail (UpgradeTailFmt, below in
	// AppendRuleLines) still prints MaxCopies as the STACK cap — still true.
	const FString Separator = IdentitySeparator();

	const FString IdentityLine = FString::Printf(TEXT("%s%sCost %d gold"),
		*CardTypeLabel(Row.CardType), *Separator, Row.Cost);

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
	// ⭐ EXTRACTED 2026-09-04 (TASK-999) into SiegeboundCardGlossary::AppendSpellLines,
	// with its behaviour repaired there. It moved for ONE reason: this member is
	// `private:` in DeckBuilderWidget.h, so the assertion TASK-999 item (3) requires —
	// every ESpellEffect value resolves to non-empty text — had no way to reach the
	// composer at all, and the header is another row's file this wave. The free function
	// keeps the logic in this translation unit and gives Tests/SiegeCardGlossaryTest.cpp
	// a door that costs the header nothing.
	SiegeboundCardGlossary::AppendSpellLines(Row, OutLines);

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
