// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "SiegeGameMode.generated.h"

class ACastle;
class AHeroCharacter;
class ASiegeBotController;
class ASiegePlayerController;

/**
 *  Siegebound game mode (GDD §3.9 / §3.1 / §3.2, M2 scope) — win condition,
 *  match-end world freeze, hero respawn, and the Play Again full reset. Set as
 *  the project default via Config/DefaultEngine.ini
 *  (GlobalDefaultGameMode = /Script/GitClaudeUnrealTest.SiegeGameMode).
 *
 *  Class defaults:
 *  - GameStateClass         = ASiegeGameState (TASK-024 match clock + overtime)
 *  - PlayerStateClass       = ASiegePlayerState (TASK-005 gold economy, TASK-024 rate composition)
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
 *  Blue in M1). Before the end screen goes up, FreezeWorldAtMatchEnd() freezes
 *  the world under it (TASK-024, closing qa/TASK-006-report.md finding 2):
 *  units FreezeAI'd, tower fire loops silenced, in-flight projectiles cleared,
 *  income paused, match clock stopped. The winner is then pushed to every
 *  ASiegePlayerController via HandleMatchEnd. A bMatchEnded latch guarantees
 *  the match ends at most once, and NOTHING else can end a match.
 *
 *  Hero respawn (GDD §3.1): the hero's FOnHeroDied (bound in SetPlayerDefaults,
 *  which runs for every pawn this mode hands to a player) schedules a respawn
 *  exactly HeroRespawnDelay (5 s) later: teleport to the PlayerStart (L_Arena
 *  places it on the Blue side, TASK-015) or — when no PlayerStart exists — next
 *  to the hero's own castle, then repossess and ResetHero() (full HP, input
 *  restored). After match end the hero stays down; PlayAgain() revives it.
 *
 *  Timer policy (QA-BINDING, TASKBOARD TASK-006 qa-note from qa/TASK-005-report.md):
 *  in PlayAgain() this class clears ONLY the specific FTimerHandle it owns
 *  (HeroRespawnTimerHandle), BEFORE ResetGold() (which restarts the income
 *  timer) — never a world-wide clear, and never another system's timer:
 *  ASiegePlayerState's income timer and the units' AI timers belong to those
 *  objects (they clean themselves up in their EndPlay). ONE deliberate, narrow
 *  exception (TASK-024, fixing qa/TASK-027-report.md WARN-1): the MATCH-END
 *  freeze silences every ATower's fire loop via the public
 *  FTimerManager::ClearAllTimersForObject — Tower.h/.cpp are frozen qa-passed
 *  contracts with a private timer handle and no public stop hook, the fire
 *  loop is the only timer a tower ever arms (qa/TASK-027 verified), nothing
 *  can legitimately re-arm it (stats bind exactly once), and PlayAgain
 *  destroys all buildings regardless. The invariant this policy protects is
 *  untouched: the income timer's owner is never targeted on any path — the
 *  match-end freeze pauses income through ASiegePlayerState's OWN PauseIncome().
 *
 *  Bot opponent (GDD §4 / §9-3, M3 — TASK-045): BeginPlay spawns EXACTLY ONE
 *  ASiegeBotController (an AAIController that possesses no pawn) and tags its
 *  auto-created ASiegePlayerState Team=Red — completing the TASK-043
 *  InitNewPlayer forward-ref (player PS Blue, bot PS Red; the mode sets both).
 *  The bot then owns a Red economy identical to the player's; PlayAgain() calls
 *  ResetBot() on it (deck + economy + decision timer). Adding this second
 *  PlayerState does not perturb the Blue player: every ASiegePlayerState owns an
 *  independent gold/timer/miner state, the PlayerArray freeze/reset loops are
 *  already generic over all of them, and the player-controller loops skip the
 *  AIController (so it never takes the end screen, and its deck reset is
 *  ResetBot's — not the player's HandleMatchReset path).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ASiegeGameMode();

	/**
	 *  Full match reset (GDD §3.9, M2 scope — TASK-006 base + TASK-024 v2) —
	 *  called by WBP_VictoryScreen's Play Again button (TASK-011). In order:
	 *    1. clears this mode's own pending hero-respawn timer (BEFORE ResetGold —
	 *       qa-note ordering; only our own handle, never other systems' timers),
	 *    2. destroys every ASummonedUnit,
	 *    2b. destroys every ABuilding (§3.9 "buildings"; ATower::EndPlay clears
	 *        its own fire timer synchronously, and dead walls heal the navmesh),
	 *    2c. destroys every in-flight AProjectile (qa/TASK-026-report.md WARN-1 —
	 *        load-bearing for a MID-MATCH reset, where units/towers may have
	 *        fired this very frame; a no-op after a normal match end),
	 *    3. ResetCastle() on every ACastle (back to 2000/2000, re-armed),
	 *    3b. ASiegeGameState::ResetClock() — clock to 0, overtime latch cleared
	 *        (MUST precede step 4: ResetEconomy re-derives the rate against it),
	 *    4. per ASiegePlayerState: ResetEconomy() (miners 0, rate re-derived),
	 *       ResetGold() (back to 50; restarts income), ResumeIncome() (lifts the
	 *       match-end pause; idempotent when never paused),
	 *    5. re-arms the win condition and restores the hero at its start with
	 *       full HP, repossessed, input enabled,
	 *    6. ASiegePlayerController::HandleMatchReset() (removes the end screen,
	 *       restores game-only input — TASK-007 contract), then a fresh deck +
	 *       hand via the controller's UDeckComponent::ResetDeck() (§3.9 "deck,
	 *       hand"; reached by component class, null-safe until TASK-023 lands).
	 *  Safe against double invocation: re-entrant calls are dropped by a guard,
	 *  and a second sequential call just re-runs steps that are all idempotent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void PlayAgain();

	/**
	 *  Main-menu level-flow entry (GDD §7 / §9-3, TASK-047 → consumed by
	 *  WBP_MainMenu in TASK-049): travels to the arena map (ArenaLevel, default
	 *  /Game/Maps/L_Arena — CONVENTIONS map) to START a fresh match vs the bot.
	 *
	 *  STATIC + WorldContext deliberately: L_MainMenu runs its OWN (menu) game
	 *  mode (TASK-049), NOT an ASiegeGameMode, so the menu widget must be able to
	 *  start a match WITHOUT an ASiegeGameMode instance present — a non-static
	 *  member would force the caller to find-and-cast a game mode that is not
	 *  there. The arena map path is read from the CDO's ArenaLevel UPROPERTY so it
	 *  stays designer-editable while the function stays static-callable from any
	 *  Blueprint. Opening the level boots a clean ASiegeGameMode in L_Arena, whose
	 *  BeginPlay spawns the bot and deals both decks — a brand-new match, so no
	 *  in-place reset (PlayAgain) is needed on this path. Null-safe: logs and
	 *  no-ops if the world context or the arena path cannot be resolved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match", meta = (WorldContext = "WorldContextObject"))
	static void StartMatch(const UObject* WorldContextObject);

	/**
	 *  Dev/test main-menu entry (CONVENTIONS "Dev / test tooling", TASK-071):
	 *  identical to StartMatch, but opens the arena with the ?Sandbox=1 URL option
	 *  so the fresh L_Arena world's InitGame latches bSandboxMatch — no AI opponent
	 *  spawns (SpawnBot early-returns) and the Blue player starts with the generous
	 *  SandboxStartingGold. A calm test bench for the full 22-card roster against a
	 *  static Castle_Red target dummy. STATIC + WorldContext for the same reason as
	 *  StartMatch (L_MainMenu runs its own menu game mode, no ASiegeGameMode
	 *  instance to find). This is a SEPARATE entry point; StartMatch (Play vs Bot)
	 *  keeps its exact signature and behaviour — untouched.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match", meta = (WorldContext = "WorldContextObject"))
	static void StartSandboxMatch(const UObject* WorldContextObject);

	/**
	 *  True from the first castle destruction until PlayAgain(). Native override
	 *  of AGameModeBase::HasMatchEnded — the base declares it as a UFUNCTION
	 *  (BlueprintCallable, Category=Game), and UHT forbids a new UFUNCTION macro
	 *  on an override, so the Blueprint node comes from the inherited base
	 *  declaration; virtual dispatch returns this latch to all callers.
	 */
	virtual bool HasMatchEnded() const override { return bMatchEnded; }

protected:

	/** Binds OnCastleDestroyed on every ACastle in the level (all present at BeginPlay — placed at integration), then spawns the single Red bot opponent (SpawnBot, TASK-045). */
	virtual void BeginPlay() override;

	/** Clears this mode's own respawn timer. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Lazy, null-safe hero pawn class: BP_HeroCharacter when it exists, else AHeroCharacter (see class comment). */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	/**
	 *  Latches the dev Sandbox flag from the level-open URL (CONVENTIONS "Dev /
	 *  test tooling", TASK-071). InitGame runs exactly once, at the very start of
	 *  the world's life and BEFORE BeginPlay/SpawnBot, so bSandboxMatch is
	 *  authoritative for the whole match. It reads UGameplayStatics::HasOption(
	 *  Options, TEXT("Sandbox")) — set true only when the arena was opened via
	 *  StartSandboxMatch (?Sandbox=1). The flag persists for the life of the
	 *  L_Arena world (PlayAgain is an in-place reset that never re-runs InitGame),
	 *  so a sandbox match stays sandbox across Play Again.
	 */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 *  Player-creation hook (TASK-043 multi-team economy): tags the local
	 *  player's ASiegePlayerState Team=Blue at creation — the CONVENTIONS team
	 *  contract, the identity ASiegeGameState::GetPlayerStateForTeam resolves.
	 *  Runs once per real player login, after the PlayerState is created and
	 *  assigned (the engine itself dereferences NewPlayerController->PlayerState
	 *  here). The default is already Blue, so this is belt-and-braces for the
	 *  player and the M2 economy is untouched. The bot's Red PS is NOT set here —
	 *  this hook only runs for player logins; TASK-045 tags it where the bot is
	 *  spawned.
	 */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

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

	/**
	 *  Class of the AI opponent spawned at match start (GDD §4, TASK-045).
	 *  Defaults to ASiegeBotController; a designer may swap in a subclass. The
	 *  bot possesses no pawn — it plays cards for the Red team through its own
	 *  economy + deck (see the class comment).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Classes")
	TSubclassOf<ASiegeBotController> BotControllerClass;

	/**
	 *  Arena map opened by StartMatch (GDD §7 main-menu flow, TASK-047 →
	 *  TASK-049). Defaults to /Game/Maps/L_Arena (CONVENTIONS map). A soft world
	 *  reference so the menu never force-loads the arena until Play is pressed;
	 *  StartMatch reads it from the CDO (it is static). Editable so a designer can
	 *  point the menu at a different arena without a recompile.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Match")
	TSoftObjectPtr<UWorld> ArenaLevel;

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

	/**
	 *  Starting gold granted to the Blue player in a Sandbox match (TASK-071 —
	 *  dev/test tooling only; the normal Play-vs-Bot match ignores this). Granted
	 *  once at match start and again on each sandbox Play Again, through the
	 *  ASiegePlayerState gold API (never a raw field write). // dev sandbox — full
	 *  roster freely playable. NOTE: ASiegePlayerState::MaxGold (999) is the hard
	 *  cap, so this is clamped to 999 in practice — still a full generous pile.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Sandbox", meta = (ClampMin = "0"))
	int32 SandboxStartingGold = 9999;

private:

	/**
	 *  Resolves and caches the hero pawn class from HeroPawnClassAsset; returns
	 *  AHeroCharacter when the blueprint is unavailable (warn once). A failed
	 *  resolve is NOT cached, so a blueprint imported later in an editor session
	 *  is picked up by the next spawn.
	 */
	UClass* ResolveHeroPawnClass();

	/**
	 *  Match-end world freeze (§3.9 / M2 exit criteria, TASK-024) — runs once
	 *  from OnCastleDestroyedHandler, BEFORE the end screen goes up, closing
	 *  qa/TASK-006-report.md finding 2 and both M2 carry-forwards:
	 *    1. FreezeAI() on every ASummonedUnit (TASK-028 contract — permanent,
	 *       idempotent; frozen units idle until PlayAgain destroys them),
	 *    2. silences every ATower's fire loop (qa/TASK-027-report.md WARN-1)
	 *       via FTimerManager::ClearAllTimersForObject — the one sanctioned
	 *       foreign-timer exception, see the class comment,
	 *    3. destroys every in-flight AProjectile (qa/TASK-026-report.md WARN-1 —
	 *       none may land damage under the Victory screen),
	 *    4. PauseIncome() on every ASiegePlayerState,
	 *    5. StopClock() on the ASiegeGameState.
	 *  After this, nothing in the world can deal damage, spawn a projectile,
	 *  accrue gold, or advance the clock until PlayAgain().
	 */
	void FreezeWorldAtMatchEnd();

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
	 *  ≈(-6800, 0, 100) yaw 0 — moved outward with the ±8000 castle in the M6.5
	 *  4× widening, TASK-136; was (-1700, 0, 100) pre-M6.5); else next to the hero's own-team castle
	 *  offset toward the centerline; else the arena origin (logged).
	 *  FindPlayerStart's WorldSettings fallback is rejected — it is not a spawn point.
	 */
	void GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation);

	/** First ASiegePlayerController in the world (M1 is single local player), or nullptr. */
	ASiegePlayerController* FindLocalSiegeController() const;

	/**
	 *  Spawns the single Red bot opponent (GDD §4, TASK-045) at match start and
	 *  tags its auto-created ASiegePlayerState Team=Red (the TASK-043 forward-ref).
	 *  Runs from BeginPlay, after GameState exists and the local player has logged
	 *  in (so PlayerStateClass is set and the bot's PlayerState resolves). Guarded
	 *  against spawning a second bot — exactly one exists per match, reused across
	 *  Play Again (which is in-place; BeginPlay never re-runs).
	 */
	void SpawnBot();

	/**
	 *  Grants SandboxStartingGold to the Blue player (TASK-071), null-safe: resolves
	 *  the Blue ASiegePlayerState via ASiegeGameState::GetPlayerStateForTeam(Blue)
	 *  and tops its gold up through the gold API (routes through the player state's
	 *  gold choke point — clamp + broadcast honored, never a raw field write). The
	 *  BASE gold rate is untouched (normal +2/s economy stands). No-op + log when
	 *  not a sandbox match or the Blue player state is not yet resolvable.
	 *  Called deferred-next-tick from BeginPlay (after the Blue PS has seeded its
	 *  own gold) and synchronously from PlayAgain (the PS already exists there).
	 */
	void GrantSandboxStartingGold();

	/** Resolved hero pawn class (BP_HeroCharacter once loaded). Never a failed resolve. */
	UPROPERTY(Transient)
	TSubclassOf<APawn> ResolvedHeroPawnClass;

	/** The hero pawn this mode last handed to a player — respawn target. TODO(M8): per-player hero/timer tracking for multiplayer (single-hero assumption holds through M7, qa/TASK-006-report.md finding 6). */
	UPROPERTY(Transient)
	TObjectPtr<AHeroCharacter> TrackedHero;

	/** The single Red bot opponent, spawned in SpawnBot and reset in PlayAgain (GDD §4, TASK-045). Null until spawned; one per match. */
	UPROPERTY(Transient)
	TObjectPtr<ASiegeBotController> BotController;

	/** Double match-end guard: latched by the first castle destruction, cleared only by PlayAgain(). */
	bool bMatchEnded = false;

	/**
	 *  Dev Sandbox latch (CONVENTIONS "Dev / test tooling", TASK-071): true when
	 *  L_Arena was opened with ?Sandbox=1 (via StartSandboxMatch). Set once in
	 *  InitGame and never cleared for the life of the world — gates SpawnBot
	 *  (no AI opponent) and the SandboxStartingGold grant. PlayAgain never re-runs
	 *  InitGame, so a sandbox match stays sandbox across Play Again.
	 */
	bool bSandboxMatch = false;

	/** Re-entrancy guard for PlayAgain (e.g. a double-clicked button dispatching twice). */
	bool bPlayAgainInProgress = false;

	/** One-shot guard for the missing-BP_HeroCharacter warning. */
	bool bWarnedHeroClassMissing = false;

	/** Pending 5 s hero respawn. The ONLY timer this class owns (see timer policy in the class comment). */
	FTimerHandle HeroRespawnTimerHandle;
};
