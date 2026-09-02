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
class ASiegeGhostPawn;
class ASiegePlayerController;

/**
 *  Per-controller DEATH STATE (TASK-750, GHOST-§2/§3): the ghost this controller
 *  is currently driving and the hero it stands in for.
 *
 *  ⛔ A PLAIN STRUCT, DELIBERATELY NOT A USTRUCT: nothing reflects it, nothing
 *  saves it and nothing replicates it — it is transient by nature, exactly like
 *  this class's bMatchEnded latch and its HeroRespawnTimers map (whose key type it
 *  mirrors so the two maps can never drift apart in shape).
 *
 *  ⚠️ BOTH HALVES ARE WEAK ON PURPOSE. A strong pointer to the corpse would keep a
 *  dead hero alive for the full three minutes and would HIDE the destroyed-pawn
 *  case that RestoreHeroAtStart is explicitly written to survive; a strong pointer
 *  to the ghost would do the same for a ghost swept by anything else.
 */
struct FSiegeGhostState
{
	/** The pawn the player is driving while dead. Spawned in SpawnAndPossessGhost, destroyed in RetireGhostFor. */
	TWeakObjectPtr<ASiegeGhostPawn> Ghost;

	/** The hero the ghost stands in for — the pawn the 180 s respawn re-possesses. Never re-spawned, never duplicated. */
	TWeakObjectPtr<AHeroCharacter> Hero;
};

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
 *  Hero respawn (GDD §3.1 → SUPERSEDED BY Jonathan's GHOST-§ ruling, TASK-750):
 *  the hero's FOnHeroDied (bound in SetPlayerDefaults, which runs for every pawn
 *  this mode hands to a player) schedules a respawn exactly HeroRespawnDelay later:
 *  teleport to the PlayerStart (L_Arena places it on the Blue side, TASK-015) or —
 *  when no PlayerStart exists — next to the hero's own castle, then repossess and
 *  ResetHero() (full HP, input restored). After match end the hero stays down;
 *  PlayAgain() revives it.
 *
 *  DEATH → GHOST → RESPAWN (TASK-750, CONVENTIONS GHOST-§1/§2/§3/§4 — Jonathan's
 *  directive: "If the player dies at any point during the match, they are dead for
 *  3 minutes. During this time they instead get a ghost creature ... When the 3
 *  minutes are up they respawn back at the castle."). The respawn MECHANIC is
 *  unchanged — this mode adds exactly one thing on top of it: while the respawn is
 *  pending the player possesses an ASiegeGhostPawn instead of staring at a corpse.
 *    - HandleHeroDied spawns the ghost at the death location and possesses it,
 *      recording {ghost, hero} in ActiveGhosts (keyed exactly like HeroRespawnTimers).
 *    - HandleHeroRespawnTimer retires the ghost (re-possess the SAME hero actor,
 *      destroy the ghost) and then runs the SHIPPED RestoreHeroAtStart unchanged.
 *    - ⭐ A GHOST EXISTS IF AND ONLY IF A RESPAWN IS PENDING. Both halves are gated
 *      by the ONE pure predicate ShouldEnterGhostState, so "a permanent ghost" and
 *      "180 seconds with no pawn at all" are unrepresentable rather than unlikely.
 *    - ⛔ "CANNOT BE ATTACKED" IS NOT IMPLEMENTED HERE AND IS NOT IMPLEMENTED
 *      ANYWHERE: ASiegeGhostPawn does not implement ITeamAgent, and both target
 *      acquirers (ASummonedUnit::AcquireTarget, ATower) enumerate ONLY through
 *      GetAllActorsWithInterface(UTeamAgent) — GHOST-§1. There is deliberately no
 *      invulnerability flag, no damage guard and no targeting filter in this class.
 *
 *  M8 DECLARATION for the ghost (GHOST-§6 — "tier not declared is a QA FAIL", and a
 *  declaration is not an exemption): ASiegeGhostPawn is a REPLICATED-RELEVANT class
 *  — it is possessed by a player controller and G-4 requires the ENEMY to see it, so
 *  in a P1 session it must be a server-spawned, replicated actor whose possession
 *  travels the normal engine path (the same tier as the hero pawn). NO RPC and NO
 *  replicated property are authored in this batch (ACC-§8's discipline): the shape is
 *  declared and reserved. ActiveGhosts is the per-controller server-side shape — the
 *  same shape HeroRespawnTimers already documents, for the same reason.
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
 *  TASK-750 rider: the policy is UNCHANGED and the ghost teardown OBEYS it —
 *  RetireGhostFor is called from exactly the four sites that already clear this
 *  class's own respawn handles, and it touches only actors this class spawned.
 *  ⚠️ GHOST-§2's reason for restating it: at 180 s a leaked handle survives 36×
 *  longer than it did at 5 s, so the policy matters 36× more than it used to.
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
	 *    1b. retires every live death ghost (TASK-750) — re-possess the hero, destroy
	 *        the ghost. ⛔ IT MUST PRECEDE STEP 5 AND THAT IS LOAD-BEARING, NOT TIDINESS:
	 *        step 5 reaches the hero through IterPC->GetPawn(), so a Play Again pressed
	 *        while a player is ghosted would otherwise miss ResetUpgrades() entirely and
	 *        the very next ResetHero() would re-apply the OLD stacks onto the "reset"
	 *        hero — upgrades surviving a full match reset,
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
	 *       hand"; reached by component class, null-safe until TASK-023 lands),
	 *    6b. USiegeMapMarkSubsystem::ClearMarks() per local player (TASK-744's
	 *        MARK-§ M-4). ⛔ IT MUST BE DONE FROM HERE: that subsystem is a
	 *        ULocalPlayerSubsystem and therefore OUTLIVES an in-place PlayAgain, so
	 *        without this call last match's numbered circles are still on next
	 *        match's map and still nameable to the AI commander,
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

	/**
	 *  ⭐⭐ THE ONE DEATH-LIFECYCLE PREDICATE (TASK-750, GHOST-§2 / GHOST-§3) — PURE,
	 *  STATIC, ⛔ no world, ⛔ no actors, ⛔ no member state. It answers ONE question
	 *  for BOTH halves of the death transition: does this death put the player into
	 *  the ghost state (and therefore also schedule the respawn that ends it)?
	 *
	 *  ⭐ ONE PREDICATE IS THE POINT, NOT AN ECONOMY. The ghost's existence and the
	 *  respawn timer's existence are the SAME condition, so routing both through one
	 *  function makes "a ghost with no timer" (a permanent ghost) and "a timer with no
	 *  ghost" (three minutes of a hidden corpse and no pawn to drive) UNREPRESENTABLE
	 *  rather than merely unlikely. A second copy of these two clauses is the drift
	 *  defect this codebase keeps paying for.
	 *
	 *  false when bInMatchEnded — ⛔ GHOST-§2's ALREADY-RULED rule, inherited verbatim
	 *  and ⛔ NOT a second match-end rule: "After match end no respawn is scheduled",
	 *  so there is no ghost either; the hero stays down and PlayAgain() revives it.
	 *  false with no owning controller — the shipped guard, unchanged in effect: there
	 *  is nobody to possess a ghost and nobody to respawn.
	 *
	 *  ⚠️ It takes plain bools rather than reading members precisely so it can be
	 *  exercised headlessly across the whole truth table (SHIP-§9c — every assertion
	 *  must be able to FAIL). Every call site passes live state.
	 */
	static bool ShouldEnterGhostState(bool bInMatchEnded, bool bHasOwningController);

	/**
	 *  ⭐ WHICH HERO A RESPAWN RESTORES (TASK-750) — PURE, STATIC, ⛔ no world.
	 *  Returns the hero actor the controller must be holding again, given the pawn it
	 *  currently possesses and the hero recorded at death time.
	 *
	 *  - The possessed pawn IS the hero on every non-ghost path (a live hero at
	 *    PlayAgain; any pre-TASK-750 flow) ⇒ returned unchanged, byte-identical to
	 *    the shipped Cast<AHeroCharacter>(Player->GetPawn()).
	 *  - The GHOST is possessed ⇒ the tracked hero is returned.
	 *  - Neither resolves ⇒ nullptr, which is the signal RestoreHeroAtStart already
	 *    handles by restarting the player with a fresh pawn.
	 *
	 *  ⛔⛔ THE MIDDLE ROW IS WHY THIS FUNCTION EXISTS, AND IT IS THE DOUBLE-APPLY
	 *  GUARD. Without it a respawn taken while the ghost is possessed sees a non-hero
	 *  pawn, falls into RestoreHeroAtStart's defensive RestartPlayer branch and spawns
	 *  a SECOND hero — leaving the first one orphaned in the world still carrying every
	 *  upgrade stack, while ResetHero() re-applies those stacks onto the corpse rather
	 *  than onto the pawn the player is driving. One hero actor lives across the whole
	 *  death, so ResetHero() runs EXACTLY ONCE per respawn on EXACTLY ONE pawn.
	 *
	 *  ⛔ It can never return the ghost: the return type is AHeroCharacter*, and
	 *  ASiegeGhostPawn is not an AHeroCharacter (GHOST-§1).
	 */
	static AHeroCharacter* ResolveHeroToRestore(APawn* PossessedPawn, AHeroCharacter* TrackedHero);

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
	 *  TASK-750 also binds FOnHeroRecallArrived here: it is the same class of seam
	 *  (the hero announces, this mode owns the destination), so it belongs at the same
	 *  site rather than in a second binding pass that could drift out of step.
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
	 *  controller a respawn exactly HeroRespawnDelay seconds out (M8 doc §3.4.4 —
	 *  per-controller timer map, the weak controller rides the delegate) and, in the
	 *  same breath and under the same predicate, hands the player the ghost to drive
	 *  while it waits (TASK-750). After match end no respawn is scheduled and no ghost
	 *  is spawned — heroes stay down until PlayAgain().
	 *
	 *  ⛔ The "§3.1: back within 5-6 s" note that used to sit here is DELETED rather
	 *  than left lying: HeroRespawnDelay is 180 s by Jonathan's ruling (GHOST-§0).
	 */
	UFUNCTION()
	void HandleHeroDied(AHeroCharacter* DeadHero);

	/**
	 *  ⭐⭐ FOnHeroRecallArrived handler — **TASK-748's CROSS-TASK CONTRACT, DISCHARGED
	 *  HERE BECAUSE THIS CLASS IS THE ONE THAT NAMES IT** (declared scope addition, see
	 *  the TASK-750 handoff). `HeroCharacter.cpp`'s own unbound-destination warning
	 *  reads, verbatim: *"The teleport-home owner must bind this delegate
	 *  (TASK-750)."* — and until something does, a COMPLETED 10 s recall channel moves
	 *  and heals nothing at all, so RECALL ships inert.
	 *
	 *  ⭐ THE REASON THE SEAM POINTS HERE IS THE POINT OF THE SEAM: "back at the
	 *  castle" is ONE rule, and it is GetHeroStartTransform — the team-keyed
	 *  PlayerStart when it lies on the hero's own side and outside its own castle's
	 *  colliding bounds, else beside that castle. Recall is a channel in front of a
	 *  destination rule that already exists. ⇒ the recall teleport and the 180 s
	 *  respawn teleport now demonstrably resolve through the SAME function, so they
	 *  can never disagree about where home is.
	 *
	 *  ⛔⛔ IT TELEPORTS AND DOES NOTHING ELSE — the binder contract is explicit and
	 *  this is the trap it exists to stop: ⛔ it may NEVER call ResetHero(). ResetHero
	 *  is the DEATH path; on a LIVE hero it re-applies every cumulative upgrade
	 *  modifier onto a freshly restored base and re-arms a running War Banner aura —
	 *  a double application. The heal is the hero's own and is applied by EndRecall
	 *  immediately after this broadcast returns.
	 */
	UFUNCTION()
	void HandleHeroRecallArrived(AHeroCharacter* RecallingHero);

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

	/**
	 *  ⭐⭐ SECONDS BETWEEN HERO DEATH AND RESPAWN. **180 — THREE MINUTES — AND THE
	 *  NUMBER IS JONATHAN'S** (CONVENTIONS GHOST-§0/§2, TASK-750). His words, verbatim:
	 *  *"If the player dies at any point during the match, they are dead for 3 minutes
	 *  ... When the 3 minutes are up they respawn back at the castle."*
	 *
	 *  ⛔ THE OLD COMMENT HERE SAID "GDD §3.1: exactly 5" AND IT IS REWRITTEN RATHER
	 *  THAN LEFT LYING (the TASK-517 / HIGH-§1 idiom — a shipped comment that
	 *  contradicts the shipped value is the drift defect this project keeps paying
	 *  for). ⚠️ **GDD §3.1's sentence "back within 5-6 s" IS NOW FALSE**, and that is
	 *  a deliberate, recorded divergence: ⛔ no agent edits Docs/GDD.md — the GDD is
	 *  corrected by Jonathan, and the divergence rides as a FOR-JONATHAN row.
	 *
	 *  ⚠️⚠️ THE SCALE, STATED SO IT IS NEVER SOFTENED BY ACCIDENT: this is **36× the
	 *  shipped 5 s death penalty**, and GHOST-§0 records that an unattended castle has
	 *  been observed to fall inside a comparable window (~4 min, weak evidence and very
	 *  likely LOWER at the current build — TASK-752 re-derives it). ⛔ NO AGENT MAY
	 *  SOFTEN THIS VALUE: it is a design call he made explicitly.
	 *
	 *  ⭐ THIS IS THE **SINGLE TUNABLE** FOR THE WHOLE FEATURE AND THAT IS BY DESIGN.
	 *  The ghost's lifetime is not a second number — the ghost lives exactly as long as
	 *  this timer, because RetireGhostFor is what the timer's own callback runs. If
	 *  TASK-752's measurement moves his ruling, editing THIS ONE VALUE moves the whole
	 *  lifecycle, in the editor, with no recompile. A test asserts that no second
	 *  property on this class or on ASiegePlayerController holds 180.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float HeroRespawnDelay = 180.0f;

	/**
	 *  Soft class of the death ghost pawn (TASK-750, GHOST-§5) — the pawn the player
	 *  drives for HeroRespawnDelay seconds after dying. Mirrors the HeroPawnClassAsset
	 *  pattern above: resolved LAZILY and null-safely, missing/incompatible ⇒ warn once
	 *  and fall back to the raw C++ ASiegeGhostPawn.
	 *
	 *  ⭐ IT SHIPS **SET**, to BP_SiegeGhostPawn's generated class — authored by TASK-763
	 *  and pointed at in the constructor by TASK-764 (SiegeGameMode.cpp).
	 *
	 *  ⛔ THE OLD COMMENT HERE SAID "IT SHIPS **UNSET**, AND THAT IS THE CORRECT DEFAULT"
	 *  AND IT IS REWRITTEN RATHER THAN LEFT LYING (the TASK-517 / HIGH-§1 idiom — a
	 *  shipped comment that contradicts the shipped value is the drift defect this
	 *  project keeps paying for). That blank was correct while it stood: no task in the
	 *  GHOST batch produced a ghost blueprint, so a path to an asset nobody creates would
	 *  have fired a missing-asset warning on every single match forever. ⇒ the reason is
	 *  now SPENT, not forgotten — the asset exists, so the resolver lands on its
	 *  authored-and-loaded branch and NEITHER log fires.
	 *
	 *  ⚠️ THE `_C` SUFFIX ON THE AUTHORED PATH IS LOAD-BEARING: without it the path names
	 *  the Blueprint ASSET rather than its generated class, which is non-null, reads as
	 *  correctly authored, and still fails to load through a TSoftClassPtr — i.e. the
	 *  ghost silently never spawns. A test pins the full path literally, `_C` included,
	 *  rather than merely asserting it is non-null.
	 *
	 *  ⚠️ The resolver nonetheless still distinguishes UNSET (logged at Log once — the
	 *  designer clearing this field is a legitimate way back to the raw C++ ghost) from
	 *  AUTHORED-BUT-UNRESOLVABLE (a real mis-configuration, Warning once) — see
	 *  ResolveGhostPawnClass. Both branches remain live and neither is dead code.
	 *
	 *  ⛔ A MISSING ASSET IS NEVER A CRASH AND NEVER A DEAD 180 SECONDS WITH NO PAWN:
	 *  the raw C++ ASiegeGhostPawn remains the fallback under both failure branches.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Classes")
	TSoftClassPtr<ASiegeGhostPawn> GhostPawnClassAsset;

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
	 *  ⚖️ ROT-§4 rider (TASK-665, 2026-08-27, recomputed from
	 *  handoffs/TASK-663-buildmaster.md §1): the CASTLE-ROTATION wave swapped the
	 *  colliding X/Y half-extents — the resolver now reads X half **3,692.18**
	 *  ⇒ derived distance **3,992** (663's live resolver log: "half-extent 3692 +
	 *  clearance 300 => spawn distance 3992"). The floor stays inert by an even
	 *  wider margin; the mechanism auto-followed the rotation with NO edit,
	 *  exactly as this block argues it must. The pre-ROT figures above stay as
	 *  authored — historical record.
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
	 *  ⚖️ ROT-§4 rider (TASK-665, 2026-08-27): the CASTLE-ROTATION extent swap
	 *  makes the live X half **3,692.18** ⇒ this now derives **3,992** — measured
	 *  live by TASK-663 §1. Auto-followed again, third resize/repose in a row
	 *  with no edit.
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
	 *  Resolves and caches the ghost pawn class from GhostPawnClassAsset (TASK-750);
	 *  returns ASiegeGhostPawn when no blueprint is configured or the configured one
	 *  cannot be loaded. Warn-once on each of the two cases, with DIFFERENT messages
	 *  because they mean different things (see GhostPawnClassAsset). A failed resolve
	 *  is NOT cached, mirroring ResolveHeroPawnClass exactly.
	 */
	UClass* ResolveGhostPawnClass();

	/**
	 *  Spawns the death ghost at the dead hero's transform and possesses it
	 *  (TASK-750, GHOST-§1/§3/§4). Called from HandleHeroDied and ONLY after
	 *  ShouldEnterGhostState has said yes.
	 *
	 *  ⛔ IT IS IDEMPOTENT PER CONTROLLER: a controller already in ActiveGhosts is left
	 *  exactly as it is (a second ghost would strand the first and lose the hero
	 *  reference the respawn needs).
	 *
	 *  ⛔ A FAILED SPAWN IS NEVER FATAL: the player simply keeps the dead hero
	 *  possessed for the wait — the pre-TASK-750 experience — and the respawn timer,
	 *  which was already armed, still fires. ⛔ Never a crash, ⛔ never a dead 180 s.
	 *
	 *  ⛔ CURSOR/INPUT POSTURE IS NOT TOUCHED HERE. The possession hand-off is reported
	 *  to ASiegePlayerController::HandleGhostPossessionChanged, which routes it through
	 *  ApplyCursorInputState() — the ONE owner (HELP-§5 / GHOST-§4). ⛔ There is no
	 *  SetInputMode and no bShowMouseCursor write anywhere in this class.
	 */
	void SpawnAndPossessGhost(AController* Player, AHeroCharacter* DeadHero);

	/**
	 *  Ends the ghost state for one controller (TASK-750): re-possesses the SAME hero
	 *  actor, destroys the ghost, and drops the ActiveGhosts entry. A controller with
	 *  no ghost is a clean no-op, so every caller may call it unconditionally.
	 *
	 *  ⭐ CALLED FROM EXACTLY THE PLACES THAT ALREADY CLEAR THE RESPAWN TIMER, AND THAT
	 *  IS THE INVARIANT: the respawn timer expiring, match end, PlayAgain and EndPlay.
	 *  Ghost teardown obeys the SAME QA-binding policy as the timers it sits beside —
	 *  only state this class owns, ⛔ never a world-wide sweep (GHOST-§2's warning: a
	 *  180 s timer makes a leaked handle survive 36× longer than a 5 s one did).
	 *
	 *  ⛔ POSSESS FIRST, DESTROY SECOND — never the reverse. Destroying a possessed
	 *  pawn drives the controller through PawnPendingDestroy into the Inactive state
	 *  and moves the view target to the death spot; possessing the hero first makes the
	 *  engine unpossess the ghost cleanly and hands the camera straight back.
	 */
	void RetireGhostFor(AController* Player, const TCHAR* Reason);

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
	 *  TASK-750: retires that controller's ghost FIRST (re-possessing the same hero
	 *  actor), then runs the SHIPPED restore unchanged.
	 */
	void HandleHeroRespawnTimer(TWeakObjectPtr<AController> WeakController);

	/**
	 *  Shared by the timed respawn and PlayAgain — M8 (TASK-356 doc §3.4.4):
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
	 *  offset toward the centerline, facing the OWN castle — the yaw derived at
	 *  runtime, atan2 toward the castle centre (TASK-665, ROT ACTIVATION RULING
	 *  item 5: pre-ROT this branch faced the enemy half, and 663 measured the
	 *  hero spawning 180° away from his own rotated gate — CONVENTIONS ROT-§4);
	 *  (4) else the arena origin (logged). The side test is data-driven (castle
	 *  X sign), never a Blue/Red hardcode. FindPlayerStart's WorldSettings
	 *  fallback is rejected — it is not a spawn point.
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

	/** Resolved ghost pawn class (TASK-750) — only ever a SUCCESSFUL blueprint load; the raw-C++ fallback is deliberately not cached, so a blueprint imported later in the session is picked up by the next death. */
	UPROPERTY(Transient)
	TSubclassOf<APawn> ResolvedGhostPawnClass;

	//~ TrackedHero + HeroRespawnTimerHandle RETIRED by TASK-356 (the TODO(M8) on
	//~ this exact member, closed — doc §3.4.4/D10): a P1 session has TWO heroes
	//~ (host + client), so death/respawn/Play-Again restore is now tracked
	//~ per-controller in HeroRespawnTimers; the respawn timer delegate carries the
	//~ weak owning controller. In standalone the map simply holds one entry.

	/** Per-controller pending hero-respawn timers (M8 doc §3.4.4). The ONLY timers this class owns (timer policy unchanged — each entry cleared on fire/match-end/PlayAgain/EndPlay, never a world-wide clear). */
	TMap<TWeakObjectPtr<AController>, FTimerHandle> HeroRespawnTimers;

	/**
	 *  Per-controller live death ghosts (TASK-750, GHOST-§2/§3) — the exact key type
	 *  and the exact lifetime of HeroRespawnTimers above, on purpose: an entry is added
	 *  where a timer is armed and removed where that timer is cleared (fire /
	 *  match-end / PlayAgain / EndPlay). ⇒ ⭐ the two maps hold the same controllers at
	 *  all times, which IS the "a ghost exists iff a respawn is pending" invariant.
	 *  ⛔ Not a UPROPERTY, deliberately: FSiegeGhostState holds only WEAK pointers, so
	 *  nothing here keeps a corpse or a ghost alive (see FSiegeGhostState).
	 */
	TMap<TWeakObjectPtr<AController>, FSiegeGhostState> ActiveGhosts;

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

	/** One-shot guard for the ghost-pawn-class resolve message (TASK-750) — covers BOTH the unset and the unresolvable case, so a match logs about the ghost class at most once. */
	bool bWarnedGhostClassMissing = false;

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
