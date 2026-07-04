// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CardRow.generated.h"

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
};
