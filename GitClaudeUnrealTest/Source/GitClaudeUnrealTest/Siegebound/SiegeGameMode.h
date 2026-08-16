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
 *  in PlayAgain() this class clears ONLY the specific timer handles it owns
 *  (M8: the per-controller HeroRespawnTimers map), BEFORE ResetGold() (which restarts the income
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
	 *  Player-creation hook (TASK-043 multi-team economy → M8 SEAT LATCH,
	 *  TASK-356 doc §2.1/D3): assigns each real player login a team seat —
	 *  first login Blue (the host: on a listen server the local player always
	 *  logs in first), second Red (the joiner), third+ Red with a warning (P1
	 *  has no kick logic). Logins serialize on the server game thread ⇒
	 *  deterministic; PlayAgain is in-place (no re-login) ⇒ seats persist across
	 *  resets. InitNewPlayer (not PostLogin) keeps the tag at its existing site
	 *  and runs BEFORE RestartPlayer, so the team is settled before the hero
	 *  spawns (PossessedBy and the spawn-transform resolve both read it).
	 *  Standalone: exactly one login ⇒ Blue — byte-identical to the retired
	 *  unconditional tag. The bot's Red PS is still tagged in SpawnBot (which a
	 *  networked match gates OFF — doc §2.2).
	 */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

	/**
	 *  M8 per-player spawn resolve (TASK-356 doc §3.4.4/D10): routes EVERY player
	 *  (re)start through the team-keyed GetHeroStartTransform — the Blue/host path
	 *  resolved the level PlayerStart exactly as the engine did (§10 byte-identity;
	 *  QA-scrutinized site) until the 9× castle swallowed that PlayerStart, and
	 *  since TASK-573 it takes the SAME castle-relative fallback the Red client
	 *  takes whenever the start lies inside its own keep (no Red PlayerStart
	 *  exists in L_Arena either — the fallback IS the design, and still no level
	 *  edit). Null-safe: an unresolvable ASiegePlayerState defers to Super.
	 */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/**
	 *  M8 spawn-failure safety net (TASK-356 loop-2, the BLOCKER-5 lesson): a
	 *  player must NEVER end up pawnless. The engine's implementation
	 *  (AGameModeBase::SpawnDefaultPawnAtTransform_Implementation) spawns with a
	 *  bare FActorSpawnParameters, so the pawn class's own collision-handling
	 *  method governs — and BP_HeroCharacter's refuses a colliding spawn, which
	 *  is exactly how a mis-sized offset turned into `pawn=None` for the joining
	 *  player. This override calls Super FIRST (so the succeeding path — every
	 *  standalone spawn, byte-identity intact — is completely unchanged) and only
	 *  on a NULL result retries the SAME transform with
	 *  `AdjustIfPossibleButAlwaysSpawn`, logging loudly. A future geometry change
	 *  then degrades to a nudged spawn instead of an unplayable seat.
	 */
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

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
	 *  FOnHeroDied handler (TASK-003 contract): schedules THAT hero's owning
	 *  controller a respawn exactly HeroRespawnDelay seconds out (§3.1: back
	 *  within 5-6 s; M8 doc §3.4.4 — per-controller timer map, the weak
	 *  controller rides the delegate). After match end no respawn is scheduled —
	 *  heroes stay down until PlayAgain().
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
	 *  Castle-relative hero spawn offset, used when no PlayerStart serves this
	 *  hero's team (the Red client's path — doc §3.4.4). X is applied toward the
	 *  centerline (X=0, CONVENTIONS world axes), Y/Z verbatim.
	 *
	 *  ⚠️ X IS A FLOOR, NOT THE DISTANCE (TASK-356 loop-2, BLOCKER-5 fix). The
	 *  authored 600 was derived from the M1 castle's ~810-uu footprint and ROTTED
	 *  when the 3× remaster tripled it: TASK-357 measured that castle's colliding
	 *  half-extent at **1,219 uu** (span 23,781…26,219), so a 600 offset put the
	 *  Red spawn 819 uu INSIDE its own castle — `SpawnActor failed because of
	 *  collision` and the joining player got NO pawn at all. The resolver now
	 *  DERIVES the distance from the castle's live bounds
	 *  (`HeroSpawnCastleClearance` past the measured half-extent) and uses this X
	 *  only as the floor, so a geometry change cannot rot it again — and it did
	 *  not: at the 9× castle (half-extent **3,656.85**) the derived distance is
	 *  ≈**3,957** and this floor is simply inert (TASK-557 row S9 measured that
	 *  with NO edit, which is why this value is deliberately UNCHANGED — see the
	 *  clearance field below for why scaling it would be the defect).
	 *
	 *  ⛔ RETIRED CLAIM (TASK-573, recorded rather than deleted so a future tuner
	 *  who finds it in git history knows it was refuted): this block used to argue
	 *  *"the level's own Blue PlayerStart sits 1,200 uu out and spawns cleanly
	 *  every time, so 1,200 is the empirical floor and 1,500 is that with
	 *  margin."* At the 9× castle that PlayerStart is **2,456.85 uu INSIDE the
	 *  keep** and spawns cleanly never — 1,200 is not an empirical floor, it is
	 *  the distance from Castle_Blue (-25000) to a PlayerStart (≈-23800) that the
	 *  castle has since swallowed. **1,500 stands as a no-bounds last resort, not
	 *  as a value derived from that PlayerStart.**
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Hero")
	FVector HeroSpawnCastleOffset = FVector(1500.0f, 0.0f, 100.0f);

	/**
	 *  Clearance ADDED to the own-castle's measured colliding half-extent when
	 *  resolving a castle-relative hero spawn (TASK-356 loop-2). 300 uu past the
	 *  geometry: with the live 9× castle (half-extent **3,656.85**) this derives
	 *  ≈**3,957**; with the retired 3× castle (half-extent 1,219) it derived
	 *  1,519. ⭐ **It AUTO-FOLLOWED the 9× resize with no edit at all** — TASK-557
	 *  row S9 measured exactly that, and CONVENTIONS WR-§2b names this field the
	 *  MODEL the rest of that ledger is repaired against.
	 *
	 *  ⛔ VALUE DELIBERATELY UNCHANGED AT 9× (SC-§34 human-scale exemption,
	 *  WR-§1): 300 is keyed to a BODY, not to the castle — the hero capsule
	 *  (r≈42) plus slack for the castle's real, tighter-than-box-bound 22-hull
	 *  UCX. **Units did not grow.** Multiplying it by 3 would be the defect this
	 *  whole wave exists to stop.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float HeroSpawnCastleClearance = 300.0f;

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

	/**
	 *  Respawn-timer callback (M8 per-player, TASK-356 doc §3.4.4): the weak
	 *  controller captured at death time rides the timer delegate; a controller
	 *  gone by fire time is a logged no-op. Cleans its own map entry.
	 */
	void HandleHeroRespawnTimer(TWeakObjectPtr<AController> WeakController);

	/**
	 *  Shared by the 5 s respawn and PlayAgain — M8 (TASK-356 doc §3.4.4):
	 *  PARAMETERIZED on the owning controller (was: the single TrackedHero +
	 *  first-controller resolve; both retired). Teleports that controller's hero
	 *  to its team-keyed start (while still hidden, so the death spot never
	 *  flashes), repossesses if needed (BEFORE ResetHero — its EnableInput
	 *  mirrors death's DisableInput against the possessing controller, TASK-003
	 *  handoff), then ResetHero() for full HP + visibility + collision +
	 *  movement + input. If the pawn was destroyed entirely (defensive; no
	 *  shipped flow does), falls back to RestartPlayer, whose override +
	 *  SetPlayerDefaults re-bind OnHeroDied on the fresh pawn.
	 */
	void RestoreHeroAtStart(AController* Player);

	/**
	 *  Spawn point resolution — **TEAM-GOVERNED since TASK-356 loop-1** (the
	 *  BLOCKER-3 fix: the old order consulted the PlayerStart BEFORE HeroTeam, so
	 *  the castle-relative branch was dead code and BOTH heroes stacked on the
	 *  Blue PlayerStart). In order: (1) resolve the hero's own-team castle — it
	 *  defines this team's side of the centerline; (1b) read that castle's
	 *  colliding bounds ONCE, for both of the branches that need them (TASK-573);
	 *  (2) the level's PlayerStart (L_Arena: ≈(-23800, 0, 98) yaw 0 on the Blue
	 *  side, M7.6 ±25000 widening) **only when it lies on that same side AND is
	 *  not inside that castle's colliding bounds** (or when the level has no
	 *  castle at all — the pre-M8 behavior); (3) else next to the own-team castle,
	 *  offset toward the centerline, facing the enemy half; (4) else the arena
	 *  origin (logged). The side test is data-driven (castle X sign), never a
	 *  Blue/Red hardcode. FindPlayerStart's WorldSettings fallback is rejected —
	 *  it is not a spawn point.
	 *
	 *  ⚠️ THE FOOTPRINT TEST IS WHY (2) IS NO LONGER UNCONDITIONALLY
	 *  BYTE-IDENTICAL, AND IT IS THE POINT (TASK-573, CONVENTIONS WR-§2b row A):
	 *  at the 9× castle L_Arena's only PlayerStart sits **2,456.85 uu inside
	 *  Castle_Blue**, so accepting it spawns the hero in the keep or leaves the
	 *  player pawnless. Standalone is byte-identical for every geometry in which
	 *  that PlayerStart lies OUTSIDE the castle box — the new test can only
	 *  reject, and only in the case that was already broken. It also cannot fire
	 *  at all without usable colliding bounds, so a missing/unstreamed castle
	 *  degrades to the old behavior rather than to the arena origin.
	 */
	void GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation);

	//~ FindLocalSiegeController RETIRED by TASK-356 (M8 doc §3.4.4/§3.7): the
	//~ "first ASiegePlayerController is THE player" helper was the ban-shaped
	//~ single-player assumption (audit §1a#4). Hero restore is now parameterized
	//~ per controller; team-keyed lookups go through
	//~ ASiegePlayerController::FindControllerForTeam.

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

	//~ TrackedHero + HeroRespawnTimerHandle RETIRED by TASK-356 (the TODO(M8) on
	//~ this exact member, closed — doc §3.4.4/D10): a P1 session has TWO heroes
	//~ (host + client), so death/respawn/Play-Again restore is now tracked
	//~ per-controller in HeroRespawnTimers; the respawn timer delegate carries the
	//~ weak owning controller. In standalone the map simply holds one entry.

	/** Per-controller pending hero-respawn timers (M8 doc §3.4.4). The ONLY timers this class owns (timer policy unchanged — each entry cleared on fire/match-end/PlayAgain/EndPlay, never a world-wide clear). */
	TMap<TWeakObjectPtr<AController>, FTimerHandle> HeroRespawnTimers;

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

	/**
	 *  M8 networked-match latch (TASK-356 doc §1.3/D2 — dual latch, signed as-is
	 *  at the TASK-353 sign-off §9.4): InitGame reads the `listen` URL option
	 *  (the real `open L_Arena?listen` travel), BeginPlay ORs in the NetMode belt
	 *  (`GetNetMode() != NM_Standalone` — catches PIE "Play As Listen Server",
	 *  whose URL-option plumbing through InitGame is not guaranteed; the net
	 *  driver exists by BeginPlay on every listen path). Consumers in P1: the
	 *  SpawnBot gate (doc §2.2) and the sandbox refusal (a networked sandbox is
	 *  forced OFF in InitGame, audit §9 flag 5 accepted). Transient by nature
	 *  (plain member, the bMatchEnded pattern); standalone: false ⇒ every
	 *  consumer byte-identical.
	 */
	bool bNetworkedMatch = false;

	/** M8 seat latch (doc §2.1): true once the Blue seat (first login) is taken. */
	bool bBlueSeatTaken = false;

	/** M8 seat latch (doc §2.1): true once the Red seat (second login) is taken. Third+ logins warn and pile on Red (unsupported in P1). */
	bool bRedSeatTaken = false;
};
