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
 *    enemies inside a disc centred on that castle.
 *    ⚠️ THE MECHANISM CHANGED IN TASK-574 (CONVENTIONS WR-§2b row B) AND THIS IS
 *    THE HEADER A READER OPENS TO LEARN WHAT DEFEND *MEANS*, so it is stated
 *    here rather than left to the unit: DefendRadius IS NO LONGER THAT DISC. It
 *    is the BAND PAST THE OWN CASTLE'S WALL FACE, and the acquisition radius is
 *    DERIVED at every decision — the castle's LIVE colliding half-width plus the
 *    band — by ASummonedUnit::ResolveDefendEngagementRadius, which is the ONLY
 *    supported reader of the value. ⛔ Never read DefendRadius as a centre
 *    radius: at the 9× castle the colliding half-width is ≈3,657 uu, so the old
 *    2,500 disc lay ENTIRELY INSIDE THE KEEP and DEFEND acquired nobody, ever.
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
 *  + AMBUSH", EXTENDED by "FOLLOW command + the DEFAULT-STANCE law …
 *  (2026-08-02)" §1). Hold and Ambush share the 3-stage circle pick (SELECT
 *  units → POSITION zone → ATTACK zone) and the priority ladder; they differ
 *  ONLY in the leash:
 *  - Hold   (R): drop the target the tick it exits BOTH zones (disengage and
 *    return to the station).
 *  - Ambush (F): the zone drop-test is SKIPPED while a live target exists —
 *    finish the kill, then the ladder resumes.
 *
 *  FOLLOW (C, TASK-395) is the THIRD type and is deliberately NOT a variant of
 *  the other two:
 *  - ONE STAGE, ONE CIRCLE. The pick enters at EGroupPickStage::Select and
 *    CONFIRMS THERE — there is no position zone and no attack zone, and the
 *    pick may never advance past Select. Jonathan: "There is only one mouse
 *    scroll circle used for this, and it is just the circle used to indicate
 *    what units follow."
 *  - The anchor is the HERO, not a piece of ground, so a follow group carries
 *    zero radii, zero centers and NULL marker decals (see FSiegeUnitGroup); the
 *    select circle is a transient pick visual destroyed at confirm.
 *  - Following units NEVER attack (a per-BODY seal in UpdateStateFollow —
 *    TASK-396 — not the per-CLASS CanEverAttack seal).
 *  - Follow is also the SPAWN DEFAULT for every follow-eligible Blue unit
 *    (CONVENTIONS §2), which is why exactly ONE follow group exists per
 *    controller: ASiegePlayerController::EnsureDefaultFollowGroup.
 *
 *  ⚠️ Follow is APPENDED so Hold == 0 and Ambush == 1 stay byte-preserved.
 *  ESiegeUnitCommand above (the STANCE enum, whose byte layout WBP_HUD's switch
 *  pins depend on) is NOT touched — Follow is a group order, never a stance.
 */
UENUM(BlueprintType)
enum class ESiegeGroupCommandType : uint8
{
	Hold,
	Ambush,
	Follow
};

/**
 *  One live group order (TASK-344), owned by ASiegePlayerController::UnitGroups.
 *  Created at the stage-3 pick confirm; destroyed by the release law (T/E /
 *  Play Again), by re-selection stealing its last member, or by the 1 s prune
 *  once every member is dead. Units store only their group id (+ a precomputed
 *  station offset) and resolve this struct LIVE each state tick via
 *  ASiegePlayerController::FindUnitGroup — a null result self-heals them back
 *  to the legacy stance gate.
 *
 *  FOLLOW (TASK-395) reuses this struct UNCHANGED. A follow group carries
 *  Type == Follow, PositionRadius == AttackRadius == 0, both centers
 *  ZeroVector, and both marker decals null — its anchor is the live hero pawn
 *  (ASiegePlayerController::GetFollowAnchor), never a piece of ground. Every
 *  zone test in UpdateStateGrouped is unreachable for it because a follow group
 *  NEVER ENTERS that function (the hoisted follow dispatch, TASK-396) — not
 *  because a zero radius happens to fail a test. A follow group is also created
 *  by ASiegePlayerController::EnsureDefaultFollowGroup rather than by a pick
 *  confirm, since Follow is the spawn default.
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
