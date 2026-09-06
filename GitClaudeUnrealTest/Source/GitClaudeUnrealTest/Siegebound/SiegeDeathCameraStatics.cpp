// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeDeathCameraStatics.h"

/**
 *  ═══ TASK-1102 (DEATH-CAM-ROLL): THE TWO PURE CAMERA-ROLL RULES — IMPLEMENTATION ═══
 *
 *  The header carries the diagnosis, the named write site and the two-halves argument; this file
 *  carries only the two bodies. Both are TOTAL and PURE: every `FRotator` is a valid input, there
 *  is no world, no actor, no allocation, no clock and no failure mode — which is exactly what lets
 *  `SiegeRespawnLifecycleTest.cpp` call them with the measured `roll 89.9` and get a real red.
 *
 *  ⛔ NO `FRotator::Normalize()` IN EITHER BODY, and that is a decision, not an omission: both
 *  functions CONSTRUCT their result rather than mutating the input, so no denormalised value can
 *  be produced by them, and normalising the caller's pitch/yaw would silently change a value this
 *  task has no mandate over (half (b)'s contract is "roll returns to 0", full stop).
 */

FRotator FSiegeDeathCameraStatics::MakeDeathViewRotation(const FRotator& DeadPawnRotation)
{
	// Yaw-only — the FACING and nothing else. This single expression is what stops the dead
	// pawn's roll from reaching (1) the ghost's spawn rotation, (2) the control rotation copied
	// implicitly by AController::OnPossess, and (3) the explicit SetControlRotation at
	// SiegeGameMode.cpp:862. ⛔ Do not "restore" pitch here: see the header's PITCH IS DROPPED note.
	return FRotator(0.f, DeadPawnRotation.Yaw, 0.f);
}

FRotator FSiegeDeathCameraStatics::LevelViewRoll(const FRotator& CurrentViewRotation)
{
	// Roll to 0; pitch and yaw copied through UNCHANGED. ⛔ Never FRotator::ZeroRotator — a reset
	// that also snapped the player's look direction would be a new defect wearing this one's name.
	return FRotator(CurrentViewRotation.Pitch, CurrentViewRotation.Yaw, 0.f);
}
