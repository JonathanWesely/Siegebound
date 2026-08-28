// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Siegebound/DeckTypes.h" // FDeckList — complete type for the TArray<FDeckList> BotDecks UPROPERTY (M6 TASK-114)
#include "SiegeBotController.generated.h"

class ACastle;
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
	 *                 Building (incl. the Set II towers) → cheapest castle-front
	 *                 (unit — BotCastleSpawnOffset in front of Castle_Red, M7.6
	 *                 ruling #1) or between the intruder and Castle_Red (building).
	 *    2. ECONOMY — half clear → 2a a Miner (under the target + §3.3 cap) toward
	 *                 the BEST AVAILABLE MINE (AGoldNode::FindBestMineFor — THE
	 *                 single finder miners retarget through; W1-PREP mirrored
	 *                 mines, TASK-256). Finder null = every mine depleted or none
	 *                 exist ⇒ 2a is SKIPPED entirely (never buy a doomed miner;
	 *                 logged once per state change, not per tick). Else 2b a Deep
	 *                 Mine (building-routed economy, no cap; §4 M4) anchored at
	 *                 the SAME finder result, castle-front fallback when no mine
	 *                 is available (the all-depleted endgame economy).
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
	 *  The bot's two DISTINCT curated decks (M6 ruling 4, TASK-114). EditDefaultsOnly
	 *  C++ constructor defaults — a deck composition is CONTENT, not a per-card stat,
	 *  so it is a UPROPERTY default and NOT a CSV column (mirrors the mechanic-rule-
	 *  as-UPROPERTY precedent; a BP can retune them). At BeginPlay the bot RANDOMLY
	 *  picks one (FMath::RandRange), validates it via UDeckLibrary::IsDeckLegal, and
	 *  pushes it through UDeckComponent::SetPendingDeckList BEFORE BuildAndShuffle; a
	 *  missing entry OR an illegal pick ⇒ the curated DeckCount fallback (null-safe).
	 *  Both defaults are legal (sum 50, each Count <= that card's MaxCopies) and
	 *  deliberately distinct — [0] aggro-rush, [1] defensive-economy fortress — and
	 *  differ from the player's TASK-115 curated DeckCount default.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot")
	TArray<FDeckList> BotDecks;

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

	/** Rule 2a target: while ALIVE miners are fewer than this AND the half is clear AND a mine is available (FindBestMineFor non-null — TASK-256), the bot builds economy. Distinct from the §3.3 hard cap of 6 (CanAddMiner). // GDD §4 — reach ~3 miners while idle */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "0"))
	int32 TargetMinerCount = 3;

	/** Rule 4 attack gate: the bot banks to at least this much gold before committing an offensive unit — this is what makes waves GROW as income scales. Scaled 12 -> 36 with the 2026-07-24 all-cards-×3 cost triple (TASK-278) = the new Ogre cost (12×3), preserving "bank toward the priciest bankable unit" so waves still grow toward Knight 18 / Cavalry 21 / Ogre 36 instead of dumping on the cheapest affordable unit. // GDD §4 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot", meta = (ClampMin = "0"))
	int32 AttackBankThreshold = 36;

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

	/**
	 *  BAND PAST CASTLE_RED'S WALL FACE (toward the centerline) at which the bot's
	 *  UNITS materialize before MARCHING out (Y fanned across ±BotSpawnLaneSpread).
	 *
	 *  ⚠ THE SEMANTIC CHANGED IN TASK-575 AND THE NAME DID NOT: this is NO LONGER a
	 *  distance from the castle CENTRE. The anchor offset is now
	 *      (live colliding half-extent on X) + this band,
	 *  resolved every decision through ResolveCastleFrontAnchorOffset() — so the
	 *  number stored here is the CLEARANCE Jonathan actually chose, and the castle's
	 *  size is measured instead of transcribed (CONVENTIONS SC-§34's structural
	 *  escape; the same shape ASiegeGameMode::GetHeroStartTransform branch 3
	 *  already ships).
	 *
	 *  M7.6 ruling #1 (Jonathan, 2026-07-18): at the 10× arena (castles ±25,000) the
	 *  old mid-field BotCenterlineSpawnX=350 materialize is REPLACED — the bot spawns
	 *  CASTLE-RELATIVE and marches the field like the player's units do, so an attack
	 *  wave's first contact is a real march (~9 min accepted "for now"; the W1 watch
	 *  sanity-checks that pacing live). Resolved against the LIVE castle every play
	 *  (GetCastleRedActor — the same live-resolve the defense path uses), so a moved
	 *  OR RESIZED castle now carries the spawn with it. Applies to rule-4 attack waves,
	 *  rule-1 defensive units AND the rule-2b Deep Mine castle-front fallback — all
	 *  three read this knob through the ONE helper, so no call site does castle math.
	 *  FLAGGED follow-up (Standing backlog, NOT designed): "adaptive bot spawn
	 *  positioning by strategy" — a later pass may choose spawn/stage points per
	 *  strategy (defend vs push vs flank).
	 *
	 *  TASK-575 / CONVENTIONS WR-§2b row F — MANAGER RULING W2-R2, which is the ruling
	 *  TASK-557 flagged this row for and correctly refused to pre-empt. BOTH recorded
	 *  rulings STAND and NEITHER is retired, because they answer different questions:
	 *  M7.6 ruling #1 is a DIRECTIVE (units materialize IN FRONT and MARCH); CASTLE-3X
	 *  ruling 3 (see SpawnBoxHalfExtent below) is a PERMISSION (an anchor that lands
	 *  inside the hollow castle is no longer REJECTED). A permission does not become
	 *  the specification. What is RETIRED is the READING that ruling 3 authorised
	 *  leaving 1,750 unchanged at 9×; that reading may not be cited again.
	 *
	 *  WHY 1,343.15 AND ⚠ NOT 5,250 (the ×3) — the arithmetic is the argument:
	 *    • castle colliding half-depth on X: ~407 uu at M7.6 (ruling #1's authoring
	 *      castle), 1,218.95 uu after CASTLE-3X, 3,656.85 uu at the 9× castle.
	 *    • the authored 1,750 therefore sat 1,343 uu IN FRONT of the wall face when
	 *      Jonathan chose it, only 531 uu in front after CASTLE-3X (never re-derived —
	 *      TASK-349 checked box-containment only), and at 9× it is 1,907 uu INSIDE the
	 *      footprint ⇒ "castle-FRONT materialize, then march the field" had become
	 *      "materialize in the hall and funnel out through one gate."
	 *    • ⚠ 5,250 (the ×3) preserves the 531-uu clearance — but NOBODY CHOSE 531. It is
	 *      what 1,750 decayed into when CASTLE-3X failed to re-derive it, and
	 *      multiplying an accident by three preserves the accident.
	 *    • 1,343.15 RESTORES the human-chosen quantity: 1,750 − ~407 at the castle
	 *      ruling #1 was written against, equivalently 5,000 − 3,656.85 at the 9× one.
	 *    ⚠ ruling #1's quoted band "~1,500–2,000" is a CASTLE-RELATIVE band whose castle
	 *      has since grown 9× in volume; it cannot be read as an absolute today, which
	 *      is exactly why the band — not the total — is what this property stores.
	 *
	 *  RESOLVED OFFSET FROM THE CASTLE CENTRE (was a flat 1,750 at every scale):
	 *      M1        407    + 1,343.15 = 1,750.15   (reproduces Jonathan's own value)
	 *      CASTLE-3X 1,218.95 + 1,343.15 = 2,562.10
	 *      9×        3,656.85 + 1,343.15 = 5,000.00
	 *  — the same 1,343-uu face clearance at all three, and it cannot rot at the next
	 *  resize. Degenerate/unresolvable castle bounds fall back to this band used as a
	 *  bare offset (today's shape), logged once — see ResolveCastleFaceDistance.
	 *
	 *  VERIFIED INERT AGAINST THE ANCHOR-CLAMP LAW (W1-PREP appendix 3a, TASK-265;
	 *  cross-note first written by TASK-265, refreshed by TASK-349, by TASK-557 for the
	 *  7380 box, and re-checked here against the RESOLVED value): at 9× the Red anchor
	 *  resolves to X ≈ 25,000 − 5,000 = 20,000, inside the SpawnBoxHalfExtent (7380)
	 *  box [17,620, 32,380] ⇒ IsPointInBotSpawnBox passes ⇒ ClampAnchorToBotSpawnRegion
	 *  returns it UNTOUCHED via the pass-through carve-out (5,000 is also < the 7,340
	 *  clamp limit, 7380 − SpawnBoxAnchorInset 40, so even the clamped path would be a
	 *  no-op). The clamp, SpawnBoxAnchorInset and BotSpawnLaneSpread are UNCHANGED by
	 *  TASK-575 — the anchor-clamp law reads the tunables and stays structurally sound.
	 *
	 *  ⚖️ ROT-§4 rider (TASK-665, 2026-08-27, recomputed from
	 *  handoffs/TASK-663-buildmaster.md §6): the CASTLE-ROTATION wave swapped
	 *  Castle_Red's colliding X/Y half-extents — ResolveCastleFaceDistance(±X) now
	 *  measures **3,692.18**, so the resolved offset is 3,692.18 + 1,343.15 =
	 *  **5,035.33** and the wave anchor materializes at X = 25,000 − 5,035.33 =
	 *  **19,964.67** (measured live in 663's SIE session: castle-front (19965, 418,
	 *  20)). ⛔ NOTHING here was edited to make that happen — the live-bounds
	 *  derivation self-corrected, which is this block's whole argument working as
	 *  designed; the 9× worked rows above (…= 5,000.00 ⇒ X ≈ 20,000) stay as
	 *  authored — pre-ROT historical record. Truth-change worth recording: with
	 *  Red's gate rotated onto its −X face, the centerline-facing face this band
	 *  fronts IS now the GATE face — the wave materializes in front of Red's own
	 *  gate and marches out (663 §6), which serves M7.6 ruling #1's intent
	 *  directly. Anchor-clamp claims re-checked at the new figure: 19,964.67 ∈
	 *  [17,620, 32,380] and 5,035.33 < 7,340 — both still hold.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float BotCastleSpawnOffset = 1343.15f;

	/** Half-width of the Y band units spawn across so waves fan out instead of stacking on one point. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float BotSpawnLaneSpread = 900.f;

	/**
	 *  Swarm fan radius — a SwarmCount card (Militia Mob = 4) spawns its copies on a
	 *  circle of this radius around the validated spawn point, via the shared
	 *  ASiegePlayerController::SpawnUnitSwarm (TASK-059) so the bot's Militia Mob
	 *  matches the player's. M7.6: with the castle-relative spawn a 300-radius fan can
	 *  never cross onto the Blue half; value kept at 300 (spawn rings are on the M7.6
	 *  keep-list). ⚠ TASK-575 refreshed the worked number this note used to quote
	 *  ("~23,250 from the centerline" = 25,000 − the then-flat 1,750): the castle-front
	 *  anchor is now RESOLVED from live castle bounds, so it is ~20,000 from the
	 *  centerline at the 9× castle. The claim is unaffected — 300 is nowhere near
	 *  either figure — and it is stated as a relationship here so it cannot rot again.
	 *  // GDD §3.0
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float SwarmSpawnRadius = 300.f; // GDD §3.0

	/**
	 *  How far SHORT of the rule-2a target mine (2D, toward the bot's own castle)
	 *  a miner materializes, so it walks the last stretch to the mine like the
	 *  player's miners. W1-PREP mirrored mines (TASK-256): formerly GoldNode_Red-
	 *  relative — the target is now whatever AGoldNode::FindBestMineFor returns,
	 *  and the desired point then clamps to the bot's own half (spawn law: the
	 *  bot NEVER spawns on the Blue half). For a Blue-half mine the clamp lands
	 *  the spawn at the centerline and the miner WALKS the field to the mine —
	 *  cross-field walks are CORRECT behavior (plan-of-record; TASK-258 watch
	 *  list, not a bug).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float MinerNodeApproachOffset = 400.f;

	/**
	 *  BAND PAST CASTLE_RED'S WALL FACE, along the direction of the nearest intruder,
	 *  where a rule-1 defensive TOWER is dropped (then clamped short of the intruder,
	 *  and never nearer the castle than the derived floor — see the .cpp call site).
	 *
	 *  ⚠ THE SEMANTIC CHANGED IN TASK-575 AND THE NAME DID NOT: this is NO LONGER a
	 *  standoff from the castle CENTRE. The placement distance is now
	 *      (live colliding face distance along the intruder direction) + this band,
	 *  via ResolveCastleFaceDistance(), so 750 means "750 uu outside the wall" at every
	 *  castle size instead of meaning it only at the M1 castle it was sized against.
	 *
	 *  TASK-575 / CONVENTIONS WR-§2b row C — MANAGER RULING W2-R3. ⚠ THIS ROTTED AT
	 *  CASTLE-3X, NOT AT THE 9× PASS; it is repaired here anyway because a batch that
	 *  makes a latent defect three times worse owns it. Measured from the CENTRE, the
	 *  authored 750 put the tower:
	 *    • at the M1 castle (half-depth ~407):    343 uu OUTSIDE the wall — as designed;
	 *    • after CASTLE-3X (half-depth 1,218.95): 469 uu INSIDE the footprint, never
	 *      re-derived;
	 *    • at 9× (half-depth 3,656.85):         2,907 uu INSIDE — so rule 1's defensive
	 *      placement was meaningless and the ring search was silently doing all the work.
	 *  ⚠ A ×3 IS BANNED HERE AND THE ARITHMETIC IS WHY: 2,250 is STILL 1,407 uu inside.
	 *  This is the row that proves WR-§2b's governing principle — a castle-derived
	 *  constant is not repaired by multiplying it, it is repaired by deriving it from
	 *  the castle's live colliding bounds with the authored value demoted to a band/floor.
	 *
	 *  RESOLVED DISTANCE FROM THE CASTLE CENTRE, intruder due-centerline (was a flat 750):
	 *      M1        407    + 750 = 1,157      (was 750  =   343 uu outside the face)
	 *      CASTLE-3X 1,218.95 + 750 = 1,968.95 (was 750  =   469 uu INSIDE)
	 *      9×        3,656.85 + 750 = 4,406.85 (was 750  = 2,907 uu INSIDE)
	 *  — 750 uu outside the wall face at all three, which is what the number always meant.
	 *
	 *  ⚠ The value is UNCHANGED at 750 on purpose: the ruling changes the MECHANISM, not
	 *  Jonathan's right to the number. It stays EditDefaultsOnly and stays his feel knob.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float TowerDefenseStandoff = 750.f;

	/** 2D clearance the bot honors between its towers/walls and any other building — mirrors the player's §3.5 rule so bot placements obey the same 200-unit law. // GDD §3.5 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float BuildingClearance = 200.f;

	//~ CastlePlinthClearance (420, the bot mirror of the player's keep-out) RETIRED
	//~ by TASK-349 (CONVENTIONS "Castle 3× HOLLOW" plinth-retirement law): the bot
	//~ may now spawn units/buildings right up to and INSIDE its own castle — spawn
	//~ truth = the spawn box gate + nav projection + collision + the existing
	//~ clearances (IsBotHalfPointClear). The symbol STAYS RETIRED (CONVENTIONS WR-§2
	//~ row 12) — it is NOT resurrected because the castle got bigger; interior
	//~ placement is the FEATURE.
	//~
	//~ ⚠ TASK-575 AMENDS THE SECOND HALF OF THIS TOMBSTONE. It used to end: "The
	//~ rule-1 tower-standoff floor it once derived (420 + 150) is preserved
	//~ numerically as a literal at that call site so bot decision output is
	//~ byte-identical." THAT BYTE-IDENTITY GUARANTEE IS DELIBERATELY GIVEN UP HERE,
	//~ and the reason is that the guarantee itself became the defect: 570 was
	//~ 420 + 150 measured against a ~814-uu castle, so at the 9× castle the floor it
	//~ preserved sits 3,087 uu INSIDE the footprint — it "preserved" a number that
	//~ can no longer mean what it meant (CONVENTIONS WR-§2b row C / ruling W2-R3).
	//~ The floor is now DERIVED at the call site — (live colliding face distance) +
	//~ the 150-uu margin that was always the body-scale half of 420 + 150 — with the
	//~ 570 literal kept ONLY as the degenerate-bounds floor, where it reproduces
	//~ today's behaviour exactly. See ResolveCastleFaceDistance and the rule-1
	//~ building branch in SiegeBotController.cpp.

	/** Half-extent for snapping a SYNTHETIC spawn point onto the navmesh (generous vertical so a guessed ground Z still finds the floor; mirrors the §3.5 ProjectPointToNavigation rule). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	FVector NavProjectionExtent = FVector(200.f, 200.f, 1000.f);

	/** Fallback Castle_Red world location when no Red ACastle is found (world axes, M7.6 10× scale-up: +25000,0). Live actor lookup is preferred. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	FVector CastleRedFallbackLocation = FVector(25000.f, 0.f, 0.f);

	/**
	 *  2D half-extent of the Red spawn box centered on Castle_Red (W1-PREP
	 *  additions 3, TASK-262 — the bot mirror of the player box). The spawn gate is
	 *  this box (or a Red-owned capture zone) instead of the whole own-half.
	 *  Default (7380,7380) — re-derived 840 → 2460 by TASK-349 (Castle 3× HOLLOW)
	 *  and 2460 → 7380 by TASK-557 (CONVENTIONS WR-§2 row 1, the 9× castle), the
	 *  SAME paired-tunable law both times: half-extent ≈ the castle's full width
	 *  (9× bounds 7313.7 × 7384.5 × 8082.6 uu), so bot wave anchors may legally
	 *  resolve INSIDE the hollow castle (ruling 3 — fine). PAIRED-TUNABLE (3-way
	 *  law): ≡ ACastle::SpawnBoxHalfExtent ≡ ASiegePlayerController::SpawnBoxHalfExtent
	 *  — keep the three in lockstep. // CONVENTIONS WR-§2
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	FVector2D SpawnBoxHalfExtent = FVector2D(7380.f, 7380.f);

	/**
	 *  How far INSIDE the box edge an INELIGIBLE spawn anchor is parked when
	 *  ClampAnchorToBotSpawnRegion pulls it in (W1-PREP appendix 3a, TASK-265):
	 *  the per-axis clamp limit is (SpawnBoxHalfExtent - this). Keeping the
	 *  clamped anchor off the exact box edge leaves the ComputeValidBotSpawnPoint
	 *  ring-search room on BOTH sides of it — an anchor pinned exactly on the
	 *  boundary would have half its candidate ring outside the box, which is what
	 *  produced the observed one-sliver pile-up in the first place.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float SpawnBoxAnchorInset = 40.f;

	/**
	 *  Anti-stacking 2D spacing the bot honors between a NON-BUILDING spawn point
	 *  and every live ASummonedUnit of EITHER team (W1-PREP appendix 3a, TASK-265).
	 *  Checked in IsBotHalfPointClear next to the BuildingClearance rule (units get
	 *  the unit rule, buildings keep the building rule), so the deterministic
	 *  widening ring in ComputeValidBotSpawnPoint WALKS to a genuinely free slot
	 *  instead of re-serving one already-occupied point to every unit in a wave
	 *  (the observed identical-XY stack). 0 disables the rule entirely.
	 *  NOTE: this is deliberately BOT-ONLY this pass — the player's placement path
	 *  (ASiegePlayerController) is untouched by design.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement", meta = (ClampMin = "0"))
	float UnitSpawnClearance = 150.f;

	/**
	 *  W1 spawn-Z diagnostic (W1-PREP appendix 3a, TASK-265 — diagnose BEFORE
	 *  fixing). When true, every bot UNIT spawn emits ONE LogGitClaudeUnrealTest
	 *  Log line with the chosen point Z, a traced ground Z, their delta, and the
	 *  spawned actor's Z / capsule half-height so the residual float above ground
	 *  is directly readable. Deliberately NOT on LogSiegeBot — that category is
	 *  exactly one line per FIRED decision rule (M3 decision-trace law) and must
	 *  stay grep-clean. Default true for the W1 build; flip off once the float
	 *  question is closed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Bot|Placement")
	bool bLogSpawnZDiagnostic = true;

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

	/**
	 *  Live Castle_Red ACTOR (the FIRST same-team ACastle the world iterator yields),
	 *  or nullptr when none exists. TASK-575 lifted this out of GetCastleRedLocation
	 *  so the castle's BOUNDS and its LOCATION are always read off the SAME actor —
	 *  the whole point of deriving the spawn/standoff geometry from live bounds is
	 *  lost if the two can disagree. Iteration order and team filter are unchanged.
	 */
	const ACastle* GetCastleRedActor() const;

	/** Live Castle_Red world location (nearest same-team ACastle), else CastleRedFallbackLocation. */
	FVector GetCastleRedLocation() const;

	/**
	 *  TASK-575 / CONVENTIONS WR-§2b rulings W2-R2 + W2-R3 — THE ONE PLACE THE BOT
	 *  MEASURES ITS OWN CASTLE. Returns the distance from Castle_Red's CENTRE to the
	 *  boundary of its LIVE colliding bounds along Direction2D (world XY), i.e. "how
	 *  far away the wall face is, that way".
	 *
	 *  bOnlyCollidingComponents = true is load-bearing, not incidental: what matters
	 *  is what BLOCKS a unit, not the render/widget bounds — ACastle's HP-bar widget
	 *  sits 9,450 uu up (TASK-557) and must never inflate this. Same call and same
	 *  reasoning as ASiegeGameMode::GetHeroStartTransform branch 3, deliberately,
	 *  so the project has ONE castle-measuring idiom.
	 *
	 *  The bounds are a world axis-aligned box, so the face distance along a unit
	 *  direction is the NEAREST axis crossing, min(Ex/|dx|, Ey/|dy|) — exact for every
	 *  direction, where max(Ex, Ey) would leave a corner-approach placement up to
	 *  ~730 uu INSIDE the 9× footprint. Measured from GetCastleRedLocation() (the
	 *  actor origin), matching the spawn box and every other anchor in this class.
	 *
	 *  Returns 0 when no castle resolves or the bounds are degenerate — chosen so
	 *  callers can add it unconditionally and degrade to their authored band alone,
	 *  which is exactly the pre-TASK-575 behaviour. Logs ONCE (bWarnedNoCastleBounds).
	 *  Non-const for that warn-once latch, mirroring ComputeValidBotSpawnPoint.
	 */
	float ResolveCastleFaceDistance(const FVector2D& Direction2D);

	/**
	 *  Castle-front anchor offset for the THREE call sites that materialize on the
	 *  centerline-facing face (rule-1 defensive unit, rule-2b Deep Mine fallback,
	 *  rule-4 attack wave): the live ±X face distance + BotCastleSpawnOffset.
	 *  Sign-free — the caller still applies TowardCenterSign, exactly as before.
	 *  Kept as its own helper so the band can never be added at two of three sites
	 *  (the failure mode that makes a semantic change worse than the stale constant).
	 *  Non-const: it funnels through ResolveCastleFaceDistance's warn-once latch.
	 */
	float ResolveCastleFrontAnchorOffset();

	/**
	 *  Finds a placement-valid spawn point near Desired by snapping onto the
	 *  navmesh (ProjectPointToNavigation) and honoring the mirrored §3.5 rules —
	 *  the Red spawn box / Red-owned capture zone (TASK-262), unit spawn
	 *  clearance (units) and 200-unit building clearance (buildings); the castle
	 *  plinth keep-out was RETIRED by TASK-349 (spawn-inside is the feature).
	 *  Searches the CLAMPED anchor (ClampAnchorToBotSpawnRegion —
	 *  W1-PREP appendix 3a, TASK-265: every caller's Desired point is funnelled
	 *  through the clamp HERE, in one place, so no call site does anchor math)
	 *  plus a deterministic widening ring; the first valid snapped point wins.
	 *  False (retry next tick) when nothing qualifies. Non-const for the warn-once
	 *  no-navmesh latch.
	 */
	bool ComputeValidBotSpawnPoint(const FVector& Desired, bool bIsBuilding, FVector& OutPoint);

	/**
	 *  W1-PREP appendix 3a (TASK-265) — the ANCHOR-CLAMP law. Returns a desired
	 *  spawn anchor moved INTO the bot's spawn region when (and ONLY when) it is
	 *  not already spawn-eligible.
	 *
	 *  ⚠ PASS-THROUGH CARVE-OUT (load-bearing, do NOT make this unconditional):
	 *  an anchor already inside the Red spawn box OR inside a Red-OWNED capture
	 *  zone is returned UNCHANGED. That is what preserves the TASK-264-verified
	 *  behavior where the bot stages mid-field while Red holds CaptureZone_Center
	 *  — an unconditional clamp would yank those anchors back to the castle and
	 *  destroy the emergent spawn-forward play the capture zone exists for.
	 *
	 *  Otherwise: per-axis clamp of (Desired - Castle_Red) into
	 *  ±(SpawnBoxHalfExtent - SpawnBoxAnchorInset), Z preserved (the navmesh
	 *  projection owns Z). Any refused sample is owned by the existing ring
	 *  walk-out in ComputeValidBotSpawnPoint (the plinth special-case this note
	 *  once disclaimed is RETIRED — TASK-349).
	 *
	 *  FLAGGED DEVIATION from the board's literal wording (see handoffs/TASK-265.md):
	 *  the clamp targets the NEARER of the two eligible regions — the castle box, or
	 *  a RED-OWNED capture zone — rather than always the castle box. A box-only
	 *  clamp would make the bot structurally unable to ever spawn in a zone it owns
	 *  (no anchor in this class is computed inside the mid zone, so the pass-through
	 *  above could never fire), which deletes TASK-264 PIE result (f) and denies the
	 *  bot Jonathan's "when captured, you can spawn units there". Castle-relative
	 *  anchors are always nearer the box, so rule-1/rule-4 waves are unaffected and
	 *  M7.6 ruling #1 stands; with no zone / a Neutral zone / a Blue-owned zone the
	 *  behavior is byte-identical to the board's spec.
	 *
	 *  Why it was needed (HISTORY — the box was 840 then): TASK-262 shrank the spawn
	 *  GATE to the 840 box but the anchors stayed pre-shrink (castle-front at the
	 *  then-1,750 BotCastleSpawnOffset in front of Castle_Red, and the rule-2 mine
	 *  anchors thousands of uu away), while the ring search tops out at 1,100 uu — so
	 *  those anchors either piled every wave onto the single ring sample that cleared
	 *  the box edge, or failed outright and stalled the rule-2 economy ladder.
	 *  ⚠ WHERE THAT STANDS TODAY (TASK-349 → TASK-557 → TASK-575): the box is now
	 *  (7380,7380) and the castle-front anchor RESOLVES to ~5,000 from the castle
	 *  centre at the 9× castle, so it is INSIDE the box and takes the pass-through
	 *  above untouched. The clamp still owns the far-flung rule-2 mine anchors, which
	 *  is the case it was actually written for.
	 */
	FVector ClampAnchorToBotSpawnRegion(const FVector& Desired) const;

	/** Spawn-region gate (+ unit spawn clearance for units / building clearance for buildings) test on an already-on-navmesh point. The old whole-own-half gate is now the Red spawn box OR a Red-owned capture zone (W1-PREP additions 3, TASK-262); the UnitSpawnClearance anti-stack rule is appendix 3a (TASK-265); the castle plinth keep-out was RETIRED by TASK-349. */
	bool IsBotHalfPointClear(const FVector& Point, bool bIsBuilding) const;

	/** True if Point lies inside the Red spawn box — a 2D square centered on Castle_Red (live team-filtered ACastle lookup via GetCastleRedLocation, else the +25000 fallback), half-extent SpawnBoxHalfExtent. Replaces the old own-half spawn gate (W1-PREP additions 3, TASK-262). */
	bool IsPointInBotSpawnBox(const FVector& Point) const;

	/** True if Point lies inside a Red-OWNED mid capture zone — single TActorIterator<ACaptureZone> (null-safe if absent = pre-capture behavior); CanTeamSpawnHere(Red, Point) folds the box test AND the Red-ownership match (TASK-260 API). */
	bool IsPointInCapturedZone(const FVector& Point) const;

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

	/**
	 *  One-shot latch for the "castle bounds unusable" warning (TASK-575,
	 *  ResolveCastleFaceDistance): no same-team ACastle resolved, or its colliding
	 *  bounds came back degenerate, so the castle-front anchor and the rule-1 tower
	 *  standoff fall back to their authored bands used as bare centre-relative
	 *  offsets — the pre-TASK-575 shape. Deliberately NOT cleared in ResetBot, and
	 *  deliberately on LogGitClaudeUnrealTest rather than LogSiegeBot: it is an
	 *  environment fault, not a decision, and the one-line-per-FIRED-rule law owns
	 *  LogSiegeBot. Same one-shot posture as bWarnedNoNavData above.
	 */
	bool bWarnedNoCastleBounds = false;

	/**
	 *  Rule-2 mine-lockout log latch (W1-PREP mirrored mines, TASK-256): set
	 *  after logging that AGoldNode::FindBestMineFor returned null (every mine
	 *  depleted or none exist — rule 2a skipped), cleared (with one recovery
	 *  line) when a mine is available again — so the lockout is logged ONCE PER
	 *  STATE CHANGE, never per 2 s tick. Diagnostics only, so both lines stay on
	 *  LogGitClaudeUnrealTest (LogSiegeBot's one-line-per-FIRED-rule law).
	 *  Self-heals across Play Again: the first rule-2 tick of a fresh match
	 *  observes the freshly scattered mines and clears it.
	 */
	bool bLoggedMineLockout = false;

	/**
	 *  Rule-2 spawn-failure streak latch (TASK-267). The rule-2 ladder no longer
	 *  ABANDONS the decision tick when a Miner (2a) / Deep Mine (2b) cannot find a
	 *  valid spawn point — it FALLS THROUGH to rules 3/4/5 (manager ruling: the
	 *  failure path spends no gold and confirms no card, so falling through strictly
	 *  ADDS reachable behavior). The two failure lines are promoted from Verbose to
	 *  Log so a persistent stall is visible at default verbosity, but this latch
	 *  emits them at most ONCE per contiguous failure streak: set on the first rule-2
	 *  spawn failure, cleared on the next SUCCESSFUL rule-2 spawn (2a or 2b) and on
	 *  match reset (ResetBot) — so a re-failing rule 2 cannot spam the 2 s cadence.
	 *  Diagnostics only (LogGitClaudeUnrealTest); NOT a UPROPERTY and not editor-
	 *  exposed. Transient — reset state, never serialized.
	 */
	bool bRule2SpawnFailureLogged = false;
};
