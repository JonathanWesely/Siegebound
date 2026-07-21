// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"
#include "CardRow.generated.h"

class UTexture2D;

/**
 *  Card category (GDD section 4). M1 uses Unit only; the rest are defined
 *  up front so cards.csv can grow without struct changes.
 */
UENUM(BlueprintType)
enum class ECardType : uint8
{
	Unit,
	Building,
	Economy,
	Spell,
	HeroUpgrade,
	Utility
};

/**
 *  Unit AI/behavior profile (GDD section 3.8). None for non-unit cards.
 */
UENUM(BlueprintType)
enum class ECardProfile : uint8
{
	None,
	Standard,
	Siege,
	Support
};

/**
 *  Spell resolution dispatch token (GDD section 3.11 / section 4 Set III; M5,
 *  CONVENTIONS "Spells & Set III (M5)"). None for non-spell cards.
 *  USpellLibrary::ResolveSpell (TASK-098) dispatches on this column; CSV cells
 *  use these value names character-for-character.
 */
UENUM(BlueprintType)
enum class ESpellEffect : uint8
{
	None,
	AoEDamage,        // Fireball: Damage over AoERadius at the reticle
	Freeze,           // FrostNova: freezes enemy units/towers for EffectDuration
	TopTargetsDamage, // Lightning: Damage to the MaxTargets highest-current-HP enemies in AoERadius
	AllyBuff,         // BattleCry: friendly units in AoERadius buffed for EffectDuration
	GoldSteal         // Pickpocket: steals GoldSteal gold, instant resolve (M5 ruling 7)
};

/**
 *  Spell delivery selector (CONVENTIONS "Spell delivery overhaul (2026-07-21)",
 *  TASK-236). Chooses HOW a spell's effect reaches its targets; the effect
 *  itself (SpellEffect) and every magnitude are unchanged by delivery.
 *
 *  Auto is the sparse default and resolves PER-EFFECT in
 *  USpellLibrary::GetEffectiveDelivery: AoEDamage and Freeze — today exactly
 *  Fireball and FrostNova, the two cards Jonathan's directive names — deliver
 *  as a HERO-ORIGIN LINE; every other effect keeps the reticle-placed ground
 *  circle. The explicit values exist as the per-card DATA override lever: a
 *  future card can pin GroundCircle or HeroLine in its cards.csv cell
 *  regardless of its effect.
 *
 *  CSV note (flagged, TASK-236): cards.csv is FROZEN this wave outside
 *  TASK-237's single Lightning cell (git-diff confinement at TASK-240), so the
 *  SpellDelivery column header is NOT yet appended to the CSV — every row
 *  deserializes/imports to Auto (the C++ struct default), which reproduces the
 *  directive with zero cell edits. The header append + CONVENTIONS registry
 *  entry ride the next cards.csv wave (manager flagged in
 *  handoffs/TASK-236.md); until then a DT_Cards reimport may log a
 *  missing-column notice for this property — benign, rows keep Auto.
 */
UENUM(BlueprintType)
enum class ESpellDelivery : uint8
{
	Auto,         // per-effect default: AoEDamage/Freeze -> HeroLine (2026-07-21 directive), everything else -> GroundCircle
	GroundCircle, // reticle-placed ground-circle AoE at the confirm point (the M5 delivery)
	HeroLine      // hero-origin line in the air toward the aim point (bot: castle-origin — flagged design default)
};

/**
 *  One row of /Game/Data/DT_Cards, imported from Docs/Data/cards.csv (GDD section 3.0).
 *  Row name = CardID in PascalCase (e.g. Footman).
 *  Property names MUST match the CSV header columns 1:1 - do not rename
 *  without updating cards.csv and re-importing DT_Cards.
 */
USTRUCT(BlueprintType)
struct GITCLAUDEUNREALTEST_API FCardRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Player-facing card name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FString DisplayName;

	/** Card category */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	ECardType CardType = ECardType::Unit;

	/** Gold cost to play the card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	int32 Cost = 0;

	/** Maximum copies of this card allowed in a deck */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	int32 MaxCopies = 0;

	/** Max hit points of the spawned unit/building (0 for stat-less cards) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float HP = 0.0f;

	/** Damage dealt per attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Damage = 0.0f;

	/** Attack range in world units */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Range = 0.0f;

	/** Seconds between attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Cadence = 1.0f;

	/** Movement speed in units/second */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Speed = 0.0f;

	/** AI/behavior profile for spawned units */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	ECardProfile Profile = ECardProfile::None;

	/** Designer notes; not used by gameplay code */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FString Notes;

	/** Copies of this card in the default 50-card deck; 0 = not in the default deck (GDD section 3.4) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	int32 DeckCount = 0;

	/** True if the card's attack is delivered by a homing projectile instead of melee contact (GDD section 3.0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	bool bRanged = false;

	// --- M4 Set II keyword / behavior columns (GDD section 3.0/section 4). Sparse: defaults leave core cards unchanged. ---

	/** Charge keyword: the first attack after >=2s of uninterrupted movement deals 2x (GDD 3.0). Cavalry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	bool bCharge = false;

	/** Slayer keyword: 2x damage vs targets whose MaxHP >= 150 (GDD 3.0). Pikeman. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	bool bSlayer = false;

	/** Suicide keyword: the unit explodes on contact/death, dealing Damage as AoE over AoERadius, then dies (GDD 4). Sapper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	bool bSuicide = false;

	/** Swarm keyword: if >0, playing the card spawns this many copies in a 300-unit circle for one cost (GDD 3.0). Militia Mob = 4. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	int32 SwarmCount = 0;

	/** Splash radius for area attackers; 0 = single target (GDD 4). Sapper 250, Bomb Tower 250. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float AoERadius = 0.0f;

	/** Inner blind-spot radius; the actor cannot fire at targets closer than this (GDD 4). Ballista Tower 300. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float MinRange = 0.0f;

	/** Spawner building: CardID spawned every SpawnInterval seconds (GDD 4). Barracks = Footman. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	FName SpawnCardID = NAME_None;

	/** Spawner building: seconds between spawns (GDD 4). Barracks = 8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	float SpawnInterval = 0.0f;

	/** Spawner building: self-destruct after this many seconds (GDD 4). Barracks = 60. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	float Lifetime = 0.0f;

	// --- Card artwork (TASK-079). Column appended at the END of cards.csv; the CSV importer maps by header name, not order. ---

	/**
	 *  Hand-UI card illustration (CONVENTIONS "Card artwork (hand UI)"):
	 *  full object path /Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>.
	 *  Unset or unresolvable = graceful text-only card face (today's
	 *  presentation), logged once, never a crash. Resolved null-safe by
	 *  UCardHandWidget's art resolvers; LoadSynchronous is accepted for these
	 *  512x512 UI textures (TASK-079 ruling 4).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	TSoftObjectPtr<UTexture2D> CardArt;

	// --- M5 Set III spell columns (GDD section 3.11 / section 4; CONVENTIONS "Spells & Set III (M5)", TASK-097).
	//     Sparse: defaults leave every non-spell card unchanged. Spells REUSE Damage
	//     (Fireball 100, Lightning 200) and AoERadius (Fireball 300, FrostNova 350,
	//     Lightning 400, BattleCry 400) — no duplicate damage/radius columns. ---

	/** Spell dispatch token consumed by USpellLibrary::ResolveSpell (M5); None = not a spell effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	ESpellEffect SpellEffect = ESpellEffect::None;

	/**
	 *  HOW the spell effect is delivered (TASK-236, CONVENTIONS "Spell delivery
	 *  overhaul (2026-07-21)"). Auto (the sparse default — see the enum doc)
	 *  resolves per-effect: AoEDamage/Freeze -> HeroLine, others ->
	 *  GroundCircle; the explicit values are the per-card data override. Column
	 *  header not yet in cards.csv (frozen this wave — flagged); rows default
	 *  to Auto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	ESpellDelivery SpellDelivery = ESpellDelivery::Auto;

	/** Timed-effect duration in seconds (GDD 4: FrostNova 4, BattleCry 8). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	float EffectDuration = 0.0f;

	/** Top-N target count for TopTargetsDamage (GDD 4: Lightning 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	int32 MaxTargets = 0;

	/** Gold stolen from the opponent, capped at what they have (GDD 4: Pickpocket 10). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	int32 GoldSteal = 0;

	/** Chain keyword: total targets hit per attack (GDD 4: CrystalTower 3). Typed column per the M4 keyword-deferral note. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	int32 ChainTargets = 0;

	/** Chain keyword: flat damage reduction per bounce (GDD 4: CrystalTower 5 => 15/10/5 with Damage 15). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	int32 ChainFalloff = 0;
};
