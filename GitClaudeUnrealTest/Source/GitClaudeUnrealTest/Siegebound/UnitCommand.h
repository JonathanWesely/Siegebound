// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnitCommand.generated.h"

class ADecalActor;
class ASummonedUnit;

/**
 *  Player unit-command stance — the "Shield Wall" commands (W1, 2026-07-23;
 *  CONVENTIONS "Unit commands (Shield Wall stances)"). Header-only pure-data
 *  type (the TeamId.h exception in CONVENTIONS "C++") so BOTH
 *  ASiegePlayerController (issues the command, TASK-274) AND ASummonedUnit
 *  (consumes it, TASK-275) can include it without a heavy dependency.
 *
 *  Bound to keys T / E via IMC_Hero (TASK-273):
 *  - Attack (T): units march the enemy castle, clearing any defenders inside
 *    the enemy spawn box first (local self-defense aggro is unchanged).
 *  - Defend (E): units fall back toward the own castle, fighting only the
 *    enemies attacking it (within DefendRadius).
 *
 *  SUPERSESSION (TASK-344, CONVENTIONS "Group orders — 3-zone HOLD + AMBUSH"):
 *  the team-wide Hold STANCE flow (the one-circle R pick) is REPLACED by the
 *  per-unit group orders below. The Hold member STAYS DECLARED — WBP_HUD's
 *  stance switch pins depend on this enum's byte layout — but nothing latches
 *  it any more; R now opens the 3-stage group pick instead.
 *
 *  The surviving stances are LATCHED player-wide commands (persist until
 *  replaced). The default is Attack, but ASiegePlayerController::
 *  HasIssuedCommand() starts false and units run their LEGACY body until the
 *  player first presses a key, so behavior is byte-identical to today until a
 *  command is issued.
 */
UENUM(BlueprintType)
enum class ESiegeUnitCommand : uint8
{
	Attack,
	Hold,
	Defend
};

/**
 *  Group-order command type (TASK-344; CONVENTIONS "Group orders — 3-zone HOLD
 *  + AMBUSH"). Both types share the 3-stage circle pick (SELECT units →
 *  POSITION zone → ATTACK zone) and the priority ladder; they differ ONLY in
 *  the leash:
 *  - Hold   (R): drop the target the tick it exits BOTH zones (disengage and
 *    return to the station).
 *  - Ambush (F): the zone drop-test is SKIPPED while a live target exists —
 *    finish the kill, then the ladder resumes.
 */
UENUM(BlueprintType)
enum class ESiegeGroupCommandType : uint8
{
	Hold,
	Ambush
};

/**
 *  One live group order (TASK-344), owned by ASiegePlayerController::UnitGroups.
 *  Created at the stage-3 pick confirm; destroyed by the release law (T/E /
 *  Play Again), by re-selection stealing its last member, or by the 1 s prune
 *  once every member is dead. Units store only their group id (+ a precomputed
 *  station offset) and resolve this struct LIVE each state tick via
 *  ASiegePlayerController::FindUnitGroup — a null result self-heals them back
 *  to the legacy stance gate.
 */
USTRUCT()
struct FSiegeUnitGroup
{
	GENERATED_BODY()

	/** Unique per-controller group id (INDEX_NONE = invalid). Never reused within a controller's lifetime. */
	UPROPERTY()
	int32 GroupId = INDEX_NONE;

	/** HOLD (both-zones leash) or AMBUSH (chase-to-the-kill leash-exemption). */
	UPROPERTY()
	ESiegeGroupCommandType Type = ESiegeGroupCommandType::Hold;

	/** Center of the POSITION zone (stage-2 circle): the station zone the group spreads inside, and the tier-2 engage disc. */
	UPROPERTY()
	FVector PositionCenter = FVector::ZeroVector;

	/** Radius of the POSITION zone (2D disc). */
	UPROPERTY()
	float PositionRadius = 0.f;

	/** Center of the ATTACK zone (stage-3 circle): the tier-1 engage trigger. */
	UPROPERTY()
	FVector AttackCenter = FVector::ZeroVector;

	/** Radius of the ATTACK zone (2D disc). */
	UPROPERTY()
	float AttackRadius = 0.f;

	/**
	 *  The group's units — WEAK: the group never owns a unit's lifetime. A dead
	 *  or destroyed member goes stale and is compacted by the controller's 1 s
	 *  prune (an all-dead group is destroyed, markers included). Re-selection
	 *  by a newer pick STEALS a unit out of this list.
	 */
	UPROPERTY()
	TArray<TWeakObjectPtr<ASummonedUnit>> Members;

	/** Persistent POSITION-zone ground marker — the stage-2 circle decal, transferred from the pick at the final confirm. Dies with the group. */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> PositionMarkerDecal;

	/** Persistent ATTACK-zone ground marker — the stage-3 circle decal, transferred from the pick at the final confirm. Dies with the group. */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> AttackMarkerDecal;
};
