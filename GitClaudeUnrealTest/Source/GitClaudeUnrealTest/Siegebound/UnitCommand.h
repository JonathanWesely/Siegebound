// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnitCommand.generated.h"

/**
 *  Player unit-command stance — the "Shield Wall" commands (W1, 2026-07-23;
 *  CONVENTIONS "Unit commands (Shield Wall stances)"). Header-only pure-data
 *  type (the TeamId.h exception in CONVENTIONS "C++") so BOTH
 *  ASiegePlayerController (issues the command, TASK-274) AND ASummonedUnit
 *  (consumes it, TASK-275) can include it without a heavy dependency — nothing
 *  else lives in this file.
 *
 *  Bound to keys T / R / E via IMC_Hero (TASK-273):
 *  - Attack (T): units march the enemy castle, clearing any defenders inside
 *    the enemy spawn box first (local self-defense aggro is unchanged).
 *  - Hold   (R): units hold a player-picked ground location, fighting only
 *    enemies within HoldRadius of that point.
 *  - Defend (E): units fall back toward the own castle, fighting only the
 *    enemies attacking it (within DefendRadius).
 *
 *  The stance is a LATCHED player-wide command (persists until replaced). The
 *  default is Attack, but ASiegePlayerController::HasIssuedCommand() starts
 *  false and units run their LEGACY body until the player first presses a key,
 *  so behavior is byte-identical to today until a command is issued.
 */
UENUM(BlueprintType)
enum class ESiegeUnitCommand : uint8
{
	Attack,
	Hold,
	Defend
};
