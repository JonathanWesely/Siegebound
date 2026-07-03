// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "SiegeGameMode.generated.h"

class ACastle;
class AHeroCharacter;
class ASiegePlayerController;

/**
 *  Siegebound game mode (GDD §3.9 / §3.1, M1 scope) — win condition, hero
 *  respawn, and the Play Again full reset. Set as the project default via
 *  Config/DefaultEngine.ini (GlobalDefaultGameMode = /Script/GitClaudeUnrealTest.SiegeGameMode).
 *
 *  Class defaults:
 *  - PlayerStateClass       = ASiegePlayerState (TASK-005 gold economy)
 *  - PlayerControllerClass  = ASiegePlayerController (TASK-007 card play / end screen)
 *  - DefaultPawnClass       = /Game/Blueprints/BP_HeroCharacter (TASK-009), resolved
 *    LAZILY in GetDefaultPawnClassForController via TSoftClassPtr::LoadSynchronous
 *    with a null-safe fallback to the raw AHeroCharacter. The blueprint is produced
 *    in parallel and may not exist yet, so a constructor-time
 *    ConstructorHelpers::FClassFinder (which logs a load error on every CDO
 *    construction while the asset is missing) is deliberately NOT used — lazy
 *    soft-class resolution matches the null-safe content pattern used across
 *    the Siegebound module (ACastle, ASiegePlayerController).
 *
 *  Win condition (GDD §3.9): BeginPlay binds FOnCastleDestroyed on every ACastle
 *  in the level (Castle_Blue / Castle_Red, placed at integration). The team whose
 *  castle fell loses — Red castle destroyed => Winner = Blue (Victory), Blue
 *  destroyed => Winner = Red (Defeat variant, wired even though nothing damages
 *  Blue in M1). The winner is pushed to every ASiegePlayerController via
 *  HandleMatchEnd. A bMatchEnded latch guarantees the match ends at most once,
 *  and NOTHING else can end a match.
 *
 *  Hero respawn (GDD §3.1): the hero's FOnHeroDied (bound in SetPlayerDefaults,
 *  which runs for every pawn this mode hands to a player) schedules a respawn
 *  exactly HeroRespawnDelay (5 s) later: teleport to the PlayerStart (L_Arena
 *  places it on the Blue side, TASK-015) or — when no PlayerStart exists — next
 *  to the hero's own castle, then repossess and ResetHero() (full HP, input
 *  restored). After match end the hero stays down; PlayAgain() revives it.
 *
 *  Timer policy (QA-BINDING, TASKBOARD TASK-006 qa-note from qa/TASK-005-report.md):
 *  this class clears ONLY the specific FTimerHandle it owns (HeroRespawnTimerHandle).
 *  It never calls ClearAllTimersForObject on foreign objects or any world-wide
 *  clear — ASiegePlayerState's income timer and the units' AI timers belong to
 *  those objects (they clean themselves up in their EndPlay). In PlayAgain() the
 *  own-timer clear runs BEFORE ResetGold(), which restarts the income timer.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ASiegeGameMode();

	/**
	 *  Full match reset (GDD §3.9 M1 scope) — called by WBP_VictoryScreen's
	 *  Play Again button (TASK-011). In order:
	 *    1. clears this mode's own pending hero-respawn timer (BEFORE ResetGold —
	 *       qa-note ordering; only our own handle, never other systems' timers),
	 *    2. destroys every ASummonedUnit,
	 *    3. ResetCastle() on every ACastle (back to 2000/2000, re-armed),
	 *    4. ResetGold() on every ASiegePlayerState (back to 50; restarts income),
	 *    5. re-arms the win condition and restores the hero at its start with
	 *       full HP, repossessed, input enabled,
	 *    6. ASiegePlayerController::HandleMatchReset() (removes the end screen,
	 *       restores game-only input — TASK-007 contract).
	 *  Safe against double invocation: re-entrant calls are dropped by a guard,
	 *  and a second sequential call just re-runs steps that are all idempotent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void PlayAgain();

	/**
	 *  True from the first castle destruction until PlayAgain(). Native override
	 *  of AGameModeBase::HasMatchEnded — the base declares it as a UFUNCTION
	 *  (BlueprintCallable, Category=Game), and UHT forbids a new UFUNCTION macro
	 *  on an override, so the Blueprint node comes from the inherited base
	 *  declaration; virtual dispatch returns this latch to all callers.
	 */
	virtual bool HasMatchEnded() const override { return bMatchEnded; }

protected:

	/** Binds OnCastleDestroyed on every ACastle in the level (all present at BeginPlay — placed at integration). */
	virtual void BeginPlay() override;

	/** Clears this mode's own respawn timer. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Lazy, null-safe hero pawn class: BP_HeroCharacter when it exists, else AHeroCharacter (see class comment). */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/**
	 *  Runs for every pawn this mode hands to a player (initial spawn and any
	 *  RestartPlayer fallback) — binds FOnHeroDied on the new pawn and tracks it
	 *  for respawn. AddUniqueDynamic keeps repeated restarts idempotent.
	 */
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

	/**
	 *  FOnCastleDestroyed handler (TASK-002 contract): the team whose castle fell
	 *  loses, the other team wins. Latches bMatchEnded (double-end guard), cancels
	 *  any pending hero respawn (PlayAgain owns hero restoration from here), and
	 *  calls HandleMatchEnd(Winner) on every ASiegePlayerController.
	 */
	UFUNCTION()
	void OnCastleDestroyedHandler(ACastle* DestroyedCastle, ETeamId CastleTeam);

	/**
	 *  FOnHeroDied handler (TASK-003 contract): schedules RespawnHero exactly
	 *  HeroRespawnDelay seconds out (§3.1: back within 5-6 s). After match end
	 *  no respawn is scheduled — the hero stays down until PlayAgain().
	 */
	UFUNCTION()
	void HandleHeroDied(AHeroCharacter* DeadHero);

protected:

	/**
	 *  Soft class of the player pawn blueprint, /Game/Blueprints/BP_HeroCharacter
	 *  (TASK-009). Missing/incompatible = warn once and fall back to the raw
	 *  AHeroCharacter (meshless but fully playable — its input refs are null-safe).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Classes")
	TSoftClassPtr<AHeroCharacter> HeroPawnClassAsset;

	/** Seconds between hero death and respawn (GDD §3.1: exactly 5). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float HeroRespawnDelay = 5.0f;

	/**
	 *  Respawn offset from the hero's own castle, used only when the level has no
	 *  PlayerStart. X is applied toward the centerline (X=0, CONVENTIONS world
	 *  axes) so the spawn clears the castle's ~810-unit footprint (TASK-013).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Hero")
	FVector HeroSpawnCastleOffset = FVector(600.0f, 0.0f, 100.0f);

private:

	/**
	 *  Resolves and caches the hero pawn class from HeroPawnClassAsset; returns
	 *  AHeroCharacter when the blueprint is unavailable (warn once). A failed
	 *  resolve is NOT cached, so a blueprint imported later in an editor session
	 *  is picked up by the next spawn.
	 */
	UClass* ResolveHeroPawnClass();

	/** Respawn-timer callback. */
	void RespawnHero();

	/**
	 *  Shared by the 5 s respawn and PlayAgain: teleports the hero to its start
	 *  (while still hidden, so the death spot never flashes), repossesses if
	 *  needed (BEFORE ResetHero — its EnableInput mirrors death's DisableInput
	 *  against the possessing controller, TASK-003 handoff), then ResetHero()
	 *  for full HP + visibility + collision + movement + input. If the pawn was
	 *  destroyed entirely (defensive; no M1 flow does), falls back to
	 *  RestartPlayerAtPlayerStart, whose SetPlayerDefaults re-binds OnHeroDied.
	 */
	void RestoreHeroAtStart();

	/**
	 *  Spawn point resolution, in order: the level's PlayerStart (L_Arena:
	 *  (-1700, 0, 100) yaw 0, TASK-015); else next to the hero's own-team castle
	 *  offset toward the centerline; else the arena origin (logged).
	 *  FindPlayerStart's WorldSettings fallback is rejected — it is not a spawn point.
	 */
	void GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation);

	/** First ASiegePlayerController in the world (M1 is single local player), or nullptr. */
	ASiegePlayerController* FindLocalSiegeController() const;

	/** Resolved hero pawn class (BP_HeroCharacter once loaded). Never a failed resolve. */
	UPROPERTY(Transient)
	TSubclassOf<APawn> ResolvedHeroPawnClass;

	/** The hero pawn this mode last handed to a player — respawn target. TODO(M8): per-player hero/timer tracking for multiplayer (single-hero assumption holds through M7, qa/TASK-006-report.md finding 6). */
	UPROPERTY(Transient)
	TObjectPtr<AHeroCharacter> TrackedHero;

	/** Double match-end guard: latched by the first castle destruction, cleared only by PlayAgain(). */
	bool bMatchEnded = false;

	/** Re-entrancy guard for PlayAgain (e.g. a double-clicked button dispatching twice). */
	bool bPlayAgainInProgress = false;

	/** One-shot guard for the missing-BP_HeroCharacter warning. */
	bool bWarnedHeroClassMissing = false;

	/** Pending 5 s hero respawn. The ONLY timer this class owns (see timer policy in the class comment). */
	FTimerHandle HeroRespawnTimerHandle;
};
