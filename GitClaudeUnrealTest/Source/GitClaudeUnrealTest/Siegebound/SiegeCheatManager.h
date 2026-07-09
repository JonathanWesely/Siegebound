// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "SiegeCheatManager.generated.h"

/**
 *  Siegebound debug-exec cheat manager (TASK-121, CONVENTIONS "Dev / test
 *  tooling" — headless-verification affordance). Set as
 *  ASiegePlayerController::CheatClass in that controller's constructor.
 *
 *  WHY THIS IS SAFE / NON-SHIPPING BY CONSTRUCTION: the engine only ever
 *  instantiates a UCheatManager in NON-shipping builds with cheats enabled
 *  (APlayerController::EnableCheats / AddCheats; never in a Shipping build), so
 *  every command below is unreachable in a shipped game — this class is purely
 *  additive and cannot alter normal play. It exists so build-master can drive
 *  PIE verification on Jonathan's LOCKED desktop, where SendInput is unavailable
 *  (the recurring TASK-076/112 WATCH cause) and the Blue player therefore cannot
 *  play cards, aim, or spend gold by hand during a headless run.
 *
 *  EVERY command is null-safe and routes through the SAME shipping code paths
 *  the player/bot use — never a raw field write, never a bespoke spawn:
 *   - SummonTestUnit  → ASiegePlayerController::SpawnUnitSwarm (the shared
 *                       static unit-spawn entry the player confirm path AND the
 *                       bot both call), on the composed /Game/Blueprints/Units/
 *                       BP_Unit_<CardID> class (CONVENTIONS composed soft-class law).
 *   - ApplyTestDamage → UGameplayStatics::ApplyDamage (the normal engine
 *                       TakeDamage entry) on the actor under the crosshair, else
 *                       the nearest enemy — inducing overhead health-bar +
 *                       castle-HP changes.
 *   - AddTestGold     → ASiegePlayerState::AddGold (the gold choke-point API —
 *                       clamp + OnGoldChanged broadcast preserved).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:

	/**
	 *  Spawns ONE card unit for the given team via the shared shipping spawn path
	 *  (ASiegePlayerController::SpawnUnitSwarm). CardID is a DT_Cards row name in
	 *  PascalCase (e.g. "Footman"); bRed picks the Red team (else Blue = the local
	 *  player's team). Resolves /Game/Blueprints/Units/BP_Unit_<CardID> (must be an
	 *  ASummonedUnit — building/spell CardIDs have no BP_Unit_ class and are refused
	 *  with a log). The unit is placed at the surface under the crosshair (camera
	 *  forward trace), falling back to the controlled pawn's location. Null-safe:
	 *  a missing controller/world/class logs and returns, spawning nothing.
	 */
	UFUNCTION(exec)
	void SummonTestUnit(FString CardID, bool bRed);

	/**
	 *  Applies Amount damage through the normal TakeDamage path
	 *  (UGameplayStatics::ApplyDamage) to the combat actor under the crosshair
	 *  (camera forward trace), or — when nothing combat-relevant is under the
	 *  crosshair — the nearest ENEMY combat actor. Instigator and causer are BOTH
	 *  null on purpose, so the receiver resolves this as WORLD damage (the
	 *  no-friendly-fire team check is skipped) and it lands regardless of the
	 *  target's team, at base UDamageType (100%, no fortification scaling). A
	 *  non-positive Amount, or no resolvable target, logs and does nothing.
	 */
	UFUNCTION(exec)
	void ApplyTestDamage(float Amount);

	/**
	 *  Grants Amount gold to the local (Blue) player through the gold choke-point
	 *  API (ASiegePlayerState::AddGold) — never a raw Gold field write, so the
	 *  [0, MaxGold] clamp and OnGoldChanged broadcast still fire. AddGold itself
	 *  refuses+logs a non-positive amount (grants must be positive). Null-safe when
	 *  no ASiegePlayerState is resolvable.
	 */
	UFUNCTION(exec)
	void AddTestGold(int32 Amount);
};
