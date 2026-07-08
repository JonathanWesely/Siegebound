// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "SiegeBotController.generated.h"

class ASiegePlayerState;
class UDataTable;
class UDeckComponent;

/**
 *  Decision-trace log category for the M3 rule-based bot (CONVENTIONS Logging).
 *  Exactly ONE line per FIRED §4 decision rule (rule # + card + location) so the
 *  GDD §4 "logged decision trace" acceptance is grep-able. Declared here (the
 *  bot is its only user) and defined in SiegeBotController.cpp — TASK-046. All
 *  non-decision diagnostics (missing BP, no valid spawn point this tick) stay on
 *  LogGitClaudeUnrealTest so this category holds exactly one line per actual
 *  play/discard.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeBot, Log, All);

/**
 *  Siegebound AI opponent brain (GDD §4 Bot Opponent / §9-3, M3 scope — the
 *  SHELL; the §4 ordered decision RULES arrive in TASK-046).
 *
 *  An AAIController that possesses NO pawn (§4 "controls no hero"): it plays
 *  cards for the Red team exactly as the local Blue player does, through the
 *  same economy + deck systems.
 *
 *  - bWantsPlayerState = true (set in the constructor) makes the engine
 *    auto-create a PlayerState of ASiegeGameMode::PlayerStateClass
 *    (= ASiegePlayerState) for this controller in PostInitializeComponents —
 *    so the M2 economy (accrual, §3.2 overtime rate via the shared
 *    ASiegeGameState, §3.3 miner income, Pause/Resume/ResetEconomy) is reused
 *    VERBATIM for the bot, with no new economy code. ASiegeGameMode tags that
 *    player state Team=Red right after it spawns the bot (the TASK-043
 *    forward-ref), so ASiegeGameState::GetPlayerStateForTeam(Red) resolves the
 *    bot's economy and Red miners feed it (TASK-044/046).
 *  - Owns a UDeckComponent default subobject "DeckComponent" (TASK-022 — the
 *    component is controller-agnostic), built (BuildAndShuffle) at BeginPlay =
 *    match start and rebuilt (ResetDeck) on Play Again via ResetBot(). This is
 *    the identical deck the Blue player uses — a 50-card deck dealt to a hand
 *    of 6 (§3.4).
 *  - A repeating DecisionIntervalSeconds (2 s // GDD §4) timer ticks
 *    EvaluateDecisions(). In THIS task that body is EMPTY (the bot is a no-op):
 *    it exists, owns a Red economy, holds a deck+hand, and its timer ticks —
 *    nothing more. TASK-046 fills EvaluateDecisions with the §4 ordered rules.
 *
 *  Lifecycle wiring (all in ASiegeGameMode):
 *  - SPAWN: exactly one bot at match start (SpawnBot from BeginPlay); the mode
 *    tags its PlayerState Team=Red.
 *  - PLAY AGAIN: the mode's PlayAgain() calls ResetBot() — ResetDeck +
 *    ResetEconomy + a fresh decision timer. (The bot's gold-to-50 and
 *    income-timer restart come for free from PlayAgain's generic per-
 *    PlayerState loop, which already iterates every ASiegePlayerState including
 *    the bot's — see ResetBot's note; ResetBot's ResetEconomy is idempotent
 *    belt-and-braces so the entry is self-contained.)
 *  - MATCH-END FREEZE: the mode's FreezeWorldAtMatchEnd already pauses income
 *    on every ASiegePlayerState (the bot's included). Stopping the bot's
 *    DECISION timer at match end is TASK-047's job — StopDecisionTimer() is the
 *    public hook it calls. In this shell the timer keeps ticking an empty body
 *    after match end, which is harmless.
 *
 *  M2 is untouched: each ASiegePlayerState owns an independent gold value,
 *  income timer, and miner counts, so adding this second (Red) player state
 *  cannot perturb the Blue player's accrual. The mode's player-controller
 *  loops (end screen, deck reset via HandleMatchReset) iterate APlayerControllers
 *  only — an AAIController is skipped, so the bot never steals the end screen and
 *  (correctly) is not reset by the player's HandleMatchReset path, which is
 *  exactly why ResetBot owns the bot's deck reset.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeBotController : public AAIController
{
	GENERATED_BODY()

public:

	ASiegeBotController();

	/** The team this bot plays for — Red (CONVENTIONS team contract; §4). TASK-046 uses it for spawn geometry + GetPlayerStateForTeam; ASiegeGameMode tags the bot PlayerState with it at spawn. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Bot")
	ETeamId GetBotTeam() const { return BotTeam; }

	/** The bot's auto-created ASiegePlayerState (Team=Red), or nullptr if it is not yet created / not an ASiegePlayerState. Named locals avoid shadowing the inherited PlayerState UPROPERTY (CONVENTIONS C4458 law). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Bot")
	ASiegePlayerState* GetBotPlayerState() const;

	/** The bot's deck & hand model (TASK-022). Never null (default subobject). TASK-046 drives plays/discards through it. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	UDeckComponent* GetDeckComponent() const { return DeckComponent; }

	/**
	 *  Play Again reset entry (GDD §3.9) — the bot's equivalent of the player's
	 *  ASiegePlayerController::HandleMatchReset, called by ASiegeGameMode::PlayAgain
	 *  (the bot is an AAIController, so the mode's player-controller reset loop
	 *  never reaches it). In order:
	 *    1. UDeckComponent::ResetDeck() — a fresh §3.4 deck + hand of 6,
	 *    2. ASiegePlayerState::ResetEconomy() on the bot PS — miner/rate state
	 *       back to base. NOTE: the bot PS is also in GameState->PlayerArray, so
	 *       PlayAgain's generic per-player-state loop already ran ResetEconomy +
	 *       ResetGold + ResumeIncome on it (gold back to 50, income restarted);
	 *       this call is idempotent belt-and-braces so ResetBot is self-contained.
	 *    3. StartDecisionTimer() — a fresh DecisionIntervalSeconds cadence for
	 *       the new match (clears any running/stale handle first, so it never
	 *       stacks). This is the "resets the timer" in the acceptance: after a
	 *       Play Again the bot resumes deciding at a clean 2 s beat.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Bot")
	void ResetBot();

	/**
	 *  Halts the decision loop (clears DecisionTimerHandle). Public hook for the
	 *  match-end freeze (TASK-047 wires it into ASiegeGameMode's freeze so the
	 *  bot stops deciding under the Victory screen). Idempotent; ResetBot /
	 *  StartDecisionTimer restart it for the next match.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Bot")
	void StopDecisionTimer();

protected:

	/** Tags nothing here (the mode owns Team) — builds the deck (§3.4) and starts the DecisionIntervalSeconds decision timer for match start. */
	virtual void BeginPlay() override;

	/** Clears the decision timer (own-timer hygiene; the DeckComponent tears itself down). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  The §4 bot decision loop, fired every DecisionIntervalSeconds. Plays the FIRST
	 *  ordered rule whose conditions hold, then returns (one rule owns the tick).
	 *  Rule order per M5 ruling 10 (defend=1, miners=2, SPELLS=3, big unit=4,
	 *  discard=5 — TASK-102 inserted rule 3 and renumbered the trace labels):
	 *    1. DEFEND  — enemy intruder on the bot half + an affordable defensive Unit/
	 *                 Building (incl. the Set II towers) → cheapest at the centerline
	 *                 (unit) or between the intruder and Castle_Red (building).
	 *    2. ECONOMY — half clear → a Miner (under the target + §3.3 cap) or a Deep
	 *                 Mine (building-routed economy, no cap; §4 M4).
	 *    3. SPELLS  — §4 M5 extension (TASK-102): 3a Fireball at a cluster of >=
	 *                 FireballClusterMinUnits player units (cluster radius = the
	 *                 Fireball row's AoERadius), cast at the cluster centroid; else
	 *                 3b Lightning at a player tower with >= LightningTowerMinUnits
	 *                 player units within the Lightning row's AoERadius, cast at the
	 *                 tower. Hand + affordability checks PRECEDE the world scans;
	 *                 casts resolve DIRECTLY through USpellLibrary::ResolveSpell
	 *                 (targeting mode is a human affordance — ruling 10).
	 *                 FrostNova/BattleCry/Pickpocket have NO cast rule (GDD silent)
	 *                 and fall through to rule-5 discard economics.
	 *    4. ATTACK  — banked to AttackBankThreshold → the most-expensive affordable
	 *                 UNIT (now reaching Ogre/Cavalry/Pikeman/…; Militia Mob spawns
	 *                 SwarmCount copies via the shared swarm path, TASK-059).
	 *    5. CYCLE   — a card the bot can NEVER play (HeroUpgrade/Utility/non-castable
	 *                 Spell — the bot has no hero; §4 M4/M5) + gold >= the fee →
	 *                 discard the most-expensive such card (hardened: discard-first,
	 *                 charge-on-success). Fireball/Lightning are PLAYABLE for this
	 *                 bot and are NEVER cycled — it holds them awaiting a target
	 *                 (the rule-4 bank-toward-it precedent).
	 *  Plays route through GetDeckComponent() (ConfirmPlayFromHand / DiscardFromHand)
	 *  + SpendGold on GetBotPlayerState(), spawning the composed BP_Unit_/BP_Building_
	 *  by CardID (Team=Red, team material via TASK-044) on a navmesh-projected, own-
	 *  half (X>=0), clearance-valid point; spell casts resolve at a scanned world
	 *  point instead (§3.5 — spells land anywhere, no placement rules). NEVER plays
	 *  an unaffordable card; NEVER spawns on the Blue half; exactly one LogSiegeBot
	 *  line per fired rule.
	 */
	void EvaluateDecisions();

	/** (Re)starts the repeating decision timer at DecisionIntervalSeconds. SetTimer on the same handle replaces any existing timer, so it never stacks. */
	void StartDecisionTimer();

	/**
	 *  The bot's deck & hand model (GDD §3.4, TASK-022) — default subobject named
	 *  exactly "DeckComponent". Built by this controller at BeginPlay (match
	 *  start) and reset in ResetBot (Play Again); the component never self-builds
	 *  (TASK-022 flagged decision 12). Controller-agnostic — the same class the
	 *  Blue player controller uses.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Deck")
	TObjectPtr<UDeckComponent> DeckComponent;

	/**
	 *  Team the bot plays for — Red (CONVENTIONS team contract; §4 "the bot").
	 *  Single source of truth: ASiegeGameMode tags the bot's PlayerState with
	 *  THIS value at spawn, and TASK-046 uses it for spawn geometry +
	 *  GetPlayerStateForTeam. // GDD §4 — the AI opponent is the Red team
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Bot")
	ETeamId BotTeam = ETeamId::Red;

	/** Seconds between bot decisions — the §4 cadence. Mechanic rule, not a CSV column (CONVENTIONS). // GDD §4 — the bot evaluates its options every 2 s */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "0.05"))
	float DecisionIntervalSeconds = 2.f;

	// --- §4 ordered-rule tuning (mechanic rules → UPROPERTY defaults, not CSV columns — CONVENTIONS) ---

	/** Rule 2 target: while ALIVE miners are fewer than this AND the half is clear, the bot builds economy. Distinct from the §3.3 hard cap of 6 (CanAddMiner). // GDD §4 — reach ~3 miners while idle */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "0"))
	int32 TargetMinerCount = 3;

	/** Rule 4 attack gate: the bot banks to at least this much gold before committing an offensive unit — this is what makes waves GROW as income scales. // GDD §4 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "0"))
	int32 AttackBankThreshold = 12;

	/** Rule 5 discard fee (mirrors the player's §3.6 1-gold charge); rule 5 needs at least this much gold. // GDD §3.6 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "0"))
	int32 BotDiscardCost = 1;

	/** CardID of the §3.3 economy card (rule 2). Matches the player controller's MinerCardID + the DT_Cards row. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot")
	FName MinerCardID = FName(TEXT("Miner"));

	/**
	 *  Economy-typed cards whose ACTOR is a building (rule 2 Deep Mine, §4 M4 —
	 *  mirrors ASiegePlayerController::BuildingEconomyCardIDs / TASK-059). Deep Mine's
	 *  DT_Cards row is CardType Economy (§8 raidable economy) but ADeepMine derives
	 *  ABuilding under /Game/Blueprints/Buildings/, so the bot routes it down the
	 *  BUILDING spawn path with NO miner-cap interaction. Editable so a future
	 *  economy-building needs no code change.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot")
	TArray<FName> BuildingEconomyCardIDs = { FName(TEXT("DeepMine")) };

	// --- Rule 3 (SPELLS, §4 M5 extension — TASK-102) tuning. The minimum-unit gates are
	//     mechanic rules (UPROPERTY defaults, not CSV columns — CONVENTIONS). The cluster /
	//     tower-adjacency SEARCH RADII are deliberately NOT declared here: they are the
	//     spells' own AoERadius column (Fireball 300, Lightning 400 — GDD §4 Set III), read
	//     from DT_Cards at decision time per the data-driven law, so the bot only casts when
	//     the found targets actually fit inside the real blast. ---

	/** CardID of the rule-3a spell. Matches the DT_Cards Set III row (CONVENTIONS "Spells & Set III (M5)"). // GDD §4 M5 — "bot casts Fireball at 3+ clustered player units" */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot")
	FName FireballCardID = FName(TEXT("Fireball"));

	/** CardID of the rule-3b spell. Matches the DT_Cards Set III row (CONVENTIONS "Spells & Set III (M5)"). // GDD §4 M5 — "Lightning at a tower adjacent to 2+ units" */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot")
	FName LightningCardID = FName(TEXT("Lightning"));

	/** Rule 3a gate: minimum player-team units in one Fireball-radius cluster before the bot casts. // GDD §4 M5 — Fireball at 3+ clustered player units */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "1"))
	int32 FireballClusterMinUnits = 3;

	/** Rule 3b gate: minimum player-team units within the Lightning radius of a player tower before the bot casts. // GDD §4 M5 — Lightning at a tower adjacent to 2+ units */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "1"))
	int32 LightningTowerMinUnits = 2;

	/** Card stat table (GDD §3.0) — the bot reads Cost/CardType per hand card to select a rule; NEVER hardcodes a stat. Soft, resolved null-safe each decision. Matches /Game/Data/DT_Cards. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	// --- Bot spawn geometry (REIMPLEMENTS the TASK-030 placement validity for the Red half; SiegePlayerController is NOT touched) ---

	/** Arena centerline (CONVENTIONS world axes: X=0). The bot's own half is X >= this (Red); the enemy half is X < this. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	float BotHalfBoundaryX = 0.f;

	/** Just inside the bot half where offensive/defensive UNITS spawn before advancing on Castle_Blue (mirrors the player summoning near the centerline). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float BotCenterlineSpawnX = 350.f;

	/** Half-width of the Y band units spawn across so waves fan out instead of stacking on one point. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float BotSpawnLaneSpread = 900.f;

	/**
	 *  Swarm fan radius — a SwarmCount card (Militia Mob = 4) spawns its copies on a
	 *  circle of this radius around the validated centerline point, via the shared
	 *  ASiegePlayerController::SpawnUnitSwarm (TASK-059) so the bot's Militia Mob
	 *  matches the player's. Kept < BotCenterlineSpawnX (350) so a centered fan never
	 *  crosses onto the Blue half (min copy X = 350 - 300 = 50 >= 0). // GDD §3.0
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float SwarmSpawnRadius = 300.f; // GDD §3.0

	/** How far in FRONT of GoldNode_Red (toward the centerline) a rule-2 miner spawns, so it walks the last stretch to the node like the player's miners. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float MinerNodeApproachOffset = 400.f;

	/** Standoff from Castle_Red toward the nearest intruder where a rule-1 defensive TOWER is dropped (clamped outside the plinth keep-out and short of the intruder). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float TowerDefenseStandoff = 750.f;

	/** 2D clearance the bot honors between its towers/walls and any other building — mirrors the player's §3.5 rule so bot placements obey the same 200-unit law. // GDD §3.5 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float BuildingClearance = 200.f;

	/** Castle plinth keep-out (2D half-extent) the bot avoids for EVERY spawn — mirrors the player's CastlePlinthClearance so bot units/buildings never land on a plinth (M1 carry-over). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float CastlePlinthClearance = 420.f;

	/** Half-extent for snapping a SYNTHETIC spawn point onto the navmesh (generous vertical so a guessed ground Z still finds the floor; mirrors the §3.5 ProjectPointToNavigation rule). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	FVector NavProjectionExtent = FVector(200.f, 200.f, 1000.f);

	/** Fallback Castle_Red world location when no Red ACastle is found (CONVENTIONS world axes: +2000,0). Live actor lookup is preferred. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	FVector CastleRedFallbackLocation = FVector(2000.f, 0.f, 0.f);

	/** Fallback GoldNode_Red world location when no Red AGoldNode is found (CONVENTIONS world axes: +1200,0). Live actor lookup is preferred. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	FVector GoldNodeRedFallbackLocation = FVector(1200.f, 0.f, 0.f);

private:

	/** Handle for the repeating decision timer. The only timer this class owns. */
	FTimerHandle DecisionTimerHandle;

	// --- TASK-046 §4 decision-loop internals (all bot-internal; no other class is edited) ---

	/**
	 *  True while the match is live. The bot must NOT act under the Victory
	 *  screen / after match end (the TASK-045 forward-dependency). Reads
	 *  ASiegeGameMode::HasMatchEnded(); permissive ONLY when no SiegeGameMode is
	 *  resolvable (degenerate world) — TASK-047 also wires StopDecisionTimer()
	 *  into the match-end freeze, so this gate is the belt-and-suspenders half.
	 */
	bool IsMatchActive() const;

	/** True if X (a world-space FVector component, double in UE5) lies on the bot's own half (Red → X >= BotHalfBoundaryX; a Blue-configured bot flips the sign). */
	bool IsOnOwnHalf(double X) const;

	/**
	 *  Nearest alive ENEMY intruder — a summoned unit OR the enemy hero —
	 *  standing on the bot's half, measured to Castle_Red; nullptr when the half
	 *  is clear. Drives rule 1 (and its absence gates rule 2). The hero counts so
	 *  "player pushes onto the bot half → a defensive play" holds whether the
	 *  player advances with units or their own hero (flagged decision, handoff).
	 */
	AActor* FindNearestEnemyIntruderOnBotHalf() const;

	/**
	 *  Rule 3a (§4 M5, TASK-102) target search — the densest cluster of alive
	 *  PLAYER-team summoned units, anywhere on the map (§3.5 — spells have no half
	 *  restriction). Algorithm (documented per the task spec): a cluster exists when
	 *  ANY unit (the anchor) has >= MinUnits total player units (itself included,
	 *  i.e. >= MinUnits-1 OTHERS) within ClusterRadius, 2D distance (M4.5 hills must
	 *  not break grouping). Winner = the anchor with the MOST members; ties broken
	 *  by anchor distance to Castle_Red (nearest = biggest threat — deterministic).
	 *  OutCentroid = the mean of the winning cluster's member locations (the blast
	 *  centers on the group, not the anchor); OutClusterSize = its member count.
	 *  Units only — the enemy HERO is not a "player unit" and is not counted
	 *  (flagged decision; the resolver may still damage it if inside the blast).
	 *  Called ONLY from EvaluateDecisions, after hand+affordability pass (no scans
	 *  outside the 2 s cadence). False when no qualifying cluster exists.
	 */
	bool FindFireballClusterTarget(float ClusterRadius, int32 MinUnits, FVector& OutCentroid, int32& OutClusterSize) const;

	/**
	 *  Rule 3b (§4 M5, TASK-102) target search — a live PLAYER-team ATower with
	 *  >= MinUnits alive player-team summoned units within SearchRadius of it (2D).
	 *  "Tower" is the ATower family ONLY (Arrow/Bomb/Ballista/Crystal); walls,
	 *  Barracks and Deep Mines are ABuildings but deliberately do not qualify.
	 *  Winner = the qualifying tower with the MOST nearby units; ties broken by
	 *  tower distance to Castle_Red (nearest = biggest threat — deterministic).
	 *  OutNearbyUnitCount reports the winner's count (0 when none). Called ONLY
	 *  from EvaluateDecisions after hand+affordability pass. nullptr when no tower
	 *  qualifies.
	 */
	AActor* FindLightningTowerTarget(float SearchRadius, int32 MinUnits, int32& OutNearbyUnitCount) const;

	/** Live Castle_Red world location (nearest same-team ACastle), else CastleRedFallbackLocation. */
	FVector GetCastleRedLocation() const;

	/** Live GoldNode_Red world location (nearest same-team AGoldNode), else GoldNodeRedFallbackLocation. */
	FVector GetGoldNodeRedLocation() const;

	/**
	 *  Finds a placement-valid spawn point near Desired by snapping onto the
	 *  navmesh (ProjectPointToNavigation) and honoring the mirrored §3.5 rules —
	 *  own half (X >= 0), castle plinth keep-out, and, for buildings, 200-unit
	 *  clearance. Searches Desired plus a deterministic widening ring; the first
	 *  valid snapped point wins. False (retry next tick) when nothing qualifies.
	 *  Non-const for the warn-once no-navmesh latch.
	 */
	bool ComputeValidBotSpawnPoint(const FVector& Desired, bool bIsBuilding, FVector& OutPoint);

	/** Own-half + plinth keep-out (+ building clearance when bIsBuilding) test on an already-on-navmesh point. */
	bool IsBotHalfPointClear(const FVector& Point, bool bIsBuilding) const;

	/**
	 *  Resolves + spawns the composed BP for CardID with Team = BotTeam (TASK-044
	 *  recolors it Red at BeginPlay), spending Cost as the LAST gate so gold moves
	 *  iff at least one actor appears (the TASK-030 destroy-on-fail pattern). bIsBuilding
	 *  selects /Game/Blueprints/Buildings/BP_Building_<CardID> vs
	 *  /Game/Blueprints/Units/BP_Unit_<CardID>. UNITS route through the shared
	 *  ASiegePlayerController::SpawnUnitSwarm (TASK-059) so SwarmCount>1 (Militia Mob
	 *  = 4) spawns that many copies on a SwarmSpawnRadius circle for ONE Cost and the
	 *  bot's swarm matches the player's; SwarmCount<=1 (and every building) spawns a
	 *  single actor. SpawnPoint is the GROUND point (the swarm helper applies the
	 *  capsule lift; buildings spawn flush). Null-safe: a missing/incompatible BP or a
	 *  zero-spawn logs and returns nullptr with NO gold spent. Returns a representative
	 *  spawned actor.
	 */
	AActor* SpawnBotCardActor(FName CardID, bool bIsBuilding, const FVector& SpawnPoint, ASiegePlayerState& BotState, int32 Cost, int32 SwarmCount);

	/** Composed soft-class resolve (CONVENTIONS): BP_Unit_<CardID> (must be ASummonedUnit) or BP_Building_<CardID> (must be ABuilding). nullptr + log if missing/incompatible. */
	UClass* ResolveBotCardActorClass(FName CardID, bool bIsBuilding) const;

	/** One-shot latch for the no-navmesh degrade-open warning (ComputeValidBotSpawnPoint). */
	bool bWarnedNoNavData = false;
};
