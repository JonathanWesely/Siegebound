// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/CardRow.h" // ECardType (the PendingCardType member; FCardRow comes along)
#include "Siegebound/TeamId.h"
#include "Siegebound/UnitCommand.h" // ESiegeUnitCommand — the latched unit-command stance (CurrentCommand member + FOnUnitCommandChanged param; TASK-274)
#include "SiegePlayerController.generated.h"

class ACastle;
class ADecalActor;
class AHeroCharacter;
class AStaticMeshActor;
class ASiegePlayerState;
class ASummonedUnit;
class UDataTable;
class UDeckComponent;
class UInputAction;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UUserWidget;

/**
 *  Broadcast whenever a card play is refused for a player-facing reason
 *  (not enough gold, invalid placement point, card data unavailable).
 *  The HUD (WBP_HUD, TASK-011) may bind here to flash a message; binding
 *  is optional — every refusal is also logged.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardPlayRefused, FName, CardID, FText, Reason);

/**
 *  M2 refusal surface (TASK-023): broadcast on EVERY refused card play or
 *  discard with a short player-facing, §3.0-style reason ("Not enough gold",
 *  "No card in that hand slot", "Miner limit reached" [TASK-030], ...). The
 *  hand HUD (TASK-033) binds here to flash refusal messages. Play refusals
 *  ALSO fire the M1 OnCardPlayRefused (with the card context); discard
 *  refusals fire only this delegate — a discard is not a card play.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardRefused, const FString&, Reason);

/**
 *  Unit-command stance changed (Shield Wall commands, W1 TASK-274). Broadcast by
 *  SetUnitCommand every time the player latches Attack/Hold/Defend, carrying the
 *  new stance as a byte. The HUD command indicator (TASK-276) binds here
 *  (seed-then-bind from GetCurrentCommand); nothing else needs to this pass. The
 *  consuming units (TASK-275) read the LIVE controller state each tick, not this
 *  delegate. Delegate law (CONVENTIONS): FOn<Owner><Event> / On<Owner><Event>.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitCommandChanged, ESiegeUnitCommand, NewCommand);

/**
 *  Group-order pick prompt (TASK-344, CONVENTIONS "Group orders — 3-zone HOLD +
 *  AMBUSH"): broadcast on every 3-stage pick transition with a short
 *  player-facing stage prompt ("HOLD: circle your units — scroll to resize",
 *  ..., "HOLD set: 5 units"), and with an EMPTY string when the pick ends or a
 *  release clears the groups. The WBP_HUD bind (TASK-345) is ADDITIVE: it shows
 *  a non-empty prompt and falls back to the existing stance display on empty.
 *  Every prompt is ALSO logged, so the feature ships without the BP edit.
 *  Delegate law (CONVENTIONS): FOn<Owner><Event> / On<Owner><Event>.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandPromptChanged, const FString&, Prompt);

/**
 *  Siegebound player controller — hand play, discard, and card placement mode
 *  (GDD §3.4/§3.5/§3.6; M1 placement subset + TASK-023 hand v2 + TASK-030
 *  placement v2).
 *
 *  Deck & hand (M2, TASK-022/023): UDeckComponent default subobject
 *  "DeckComponent" — built (BuildAndShuffle) at BeginPlay = match start (the
 *  component never self-builds, TASK-022 flagged decision 12) and rebuilt
 *  (ResetDeck) in HandleMatchReset, which ASiegeGameMode::PlayAgain already
 *  calls. PlayHandSlot(0..5) plays a hand slot (keys 1..6); the card leaves
 *  the hand only at placement CONFIRM (M2 ruling). DiscardHandSlot charges
 *  the fixed 1-gold §3.6 fee, then moves the card. Holding IA_UICursor
 *  (Left Alt, TASK-032) shows the mouse cursor for HUD clicks and suspends
 *  camera look; releasing restores M1 game-only free-look.
 *
 *  Placement v2 (GDD §3.5, TASK-030):
 *  - BeginPlay creates and adds /Game/UI/WBP_HUD (soft class, null-safe —
 *    the widget is built in TASK-011; missing = log once and continue).
 *  - EnterPlacementMode(CardID): reads the /Game/Data/DT_Cards row (never
 *    hardcoded, GDD §3.0); refuses when the player can't afford the Cost,
 *    the hero is dead, or — Miner card only — the §3.3 alive-miner cap is
 *    full (ASiegePlayerState::CanAddMiner, "Miner limit reached", checked
 *    BEFORE any gold or mode state moves). Then enters placement mode:
 *    mouse cursor shown, hero melee suppressed
 *    (AHeroCharacter::SetMeleeSuppressed — TASK-003), and a ghost preview
 *    (/Game/Meshes/SM_<CardID>, engine-sphere fallback; dynamic instance of
 *    M_Ghost, "GhostColor" green/red) follows a per-frame cursor trace.
 *  - Valid placement (ALL cards) = ground hit AND X <= 0 (Blue half per
 *    CONVENTIONS) AND the point projects onto the navmesh within
 *    NavProjectionExtent (refuses castle-roof hits — the M1 carry-over; the
 *    plinth keep-out was RETIRED by TASK-349, spawn-inside is the feature).
 *    Building cards additionally require >= BuildingClearance from the
 *    nearest other ABuilding (§3.5; castles are NOT buildings for that
 *    rule). Violations show the red ghost.
 *  - Confirm = LMB while in mode: re-gate the miner cap, resolve the card's
 *    BP class by CardType (Unit/Economy →
 *    /Game/Blueprints/Units/BP_Unit_<CardID>; Building →
 *    /Game/Blueprints/Buildings/BP_Building_<CardID>; missing BP = refuse +
 *    exit, NO gold spent), then SpendGold(Cost), InitUnit/InitBuilding(Team,
 *    CardID) + FinishSpawning at the clicked point, and
 *    ConfirmPlayFromHand(PendingHandSlot). An invalid click refuses, spends
 *    nothing, and STAYS in mode.
 *  - Cancel = IA_CancelPlace (RMB/Esc): exit with no cost.
 *
 *  Placement v3 (GDD §5 M4.5 terrain, TASK-093): BUILDING cards gain two
 *  terrain gates, evaluated per-frame with the ghost and consumed by the
 *  confirm click — both pre-checked BEFORE any gold moves (§3.0 net-zero
 *  refusal law):
 *  - Slope: a straight-down line trace at the candidate point (±500 Z,
 *    ECC_Visibility — the cursor-trace channel) must hit, and the angle
 *    between its ImpactNormal and +Z must be <= MaxPlacementSlopeDegrees
 *    (20°), else refuse "Too steep"; a trace miss refuses (fail-closed).
 *  - Obstacle clearance: no actor carrying tag "Obstacle" (trees AND rocks —
 *    Fab amendment; tags set by TASK-095) within ObstaclePlacementClearance
 *    (150, 2D) of the candidate, else refuse "Too close to obstacles".
 *  Units/miners are exempt from both (a navmesh-valid point suffices — M4.5
 *  ruling 7). The ghost projects to the traced SURFACE height under the
 *  cursor (hill crowns and flanks included — the terrain blocks Visibility)
 *  and stays upright: yaw-only rotation, NO normal alignment.
 *
 *  Targeting mode (GDD §3.5/§3.11/§7, M5 ruling 8, TASK-100) — the SIBLING of
 *  placement mode for Spell cards; the two modes are mutually exclusive (each
 *  Enter* silently ignores while the other is live):
 *  - PlayHandSlot routes CardType Spell: SpellEffect GoldSteal resolves
 *    INSTANTLY on play (ruling 7 — no reticle for a global effect;
 *    deduct-then-resolve, refusal-safe); every other SpellEffect enters
 *    targeting mode.
 *  - Cursor posture mirrors placement mode exactly (M2 TASK-023 + TASK-074
 *    normalization; NO new input assets): visible cursor + GameAndUI via
 *    ApplyCursorInputState (targeting is the third cursor owner), hero melee
 *    suppressed while the mode owns the LMB, RMB/Esc cancel through the same
 *    IA_CancelPlace binding AND the same PlayerTick key poll.
 *  - The reticle point is TraceCursorToGround's ImpactPoint — the SURFACE
 *    under the cursor (M4.5 terrain carry-in LAW: never the Z=0 plane; hill
 *    crowns and flanks included). NO half restriction, NO navmesh projection,
 *    NO slope/obstacle/clearance gates: spells land ANYWHERE a surface
 *    answers the trace, enemy half included (§3.5). Reticle visual = a
 *    transient decal (soft-referenced /Game/Materials/M_SpellReticle,
 *    null-safe: missing material ⇒ targeting still works, log once) sized to
 *    the spell's own AoERadius.
 *  - LMB confirm = deduct Cost THEN USpellLibrary::ResolveSpell (the pinned
 *    M5 resolver; card-leaves-hand-at-CONFIRM law). Resolver false ⇒ FULL
 *    refund (§3.0 net-zero), HUD reason on the existing refusal path, card
 *    kept, mode exited (resolver refusals are position-independent — the
 *    missing-BP-class placement precedent). A trace-miss click refuses free
 *    and STAYS in mode. RMB/Esc cancel is free.
 *  - Spell delivery overhaul (TASK-236, 2026-07-21): HeroLine spells
 *    (Fireball/FrostNova) fire FROM the hero toward the reticle — the reticle
 *    is their AIM indicator and the confirm needs only a DIRECTION (see
 *    TryConfirmSpellTarget's aim pass). The reticle visuals are kept as-is
 *    for now (TASK-238/239 may restyle). GroundCircle spells (Lightning,
 *    BattleCry) are byte-untouched.
 *  - HandleMatchEnd(Winner): exits placement AND targeting mode, shows
 *    /Game/UI/WBP_VictoryScreen (soft class, null-safe) and switches to
 *    UI-only input. HandleMatchReset() restores play (TASK-006 PlayAgain)
 *    and defensively exits both modes FIRST (qa/TASK-023-report.md WARN).
 *
 *  QA-BINDING (qa/TASK-003-report.md warning 2): AHeroCharacter::ResetHero
 *  deliberately preserves bMeleeSuppressed, so this controller calls
 *  SetMeleeSuppressed(false) on EVERY placement-mode exit path — confirm,
 *  confirm-time refusal exits (miner cap / missing BP class), cancel, match
 *  end, hero death (OnHeroDied), unpossession, match reset, and EndPlay.
 *  Every one funnels through ExitPlacementMode(), which releases the
 *  suppression before any early-out.
 *
 *  All content references (widgets, data table, ghost meshes/material, input
 *  actions, card BP classes) are soft and resolved null-safe at runtime —
 *  missing assets log and never crash; a missing card BP refuses the play
 *  cleanly with no gold spent (CONVENTIONS composed soft-class law).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegePlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ASiegePlayerController();

	/**
	 *  ⚖️ NET RELEVANCY TIER: **A — engine-owned, OWNER-SCOPED** (declared per the
	 *  CONVENTIONS NET RELEVANCY LAW declaration duty; nothing is set here beyond
	 *  the explicit `bReplicates = true` the constructor now carries). A
	 *  PlayerController replicates ONLY to its own connection —
	 *  `AController::AController` sets `bOnlyRelevantToOwner = true` (engine
	 *  Controller.cpp:67) and the engine treats a connection's own PC as always
	 *  relevant to it — which is exactly right: this actor is one player's private
	 *  command surface, never world state. No distance band applies or should.
	 *  The ctor's `bReplicates` is the FINDING-4 / corollary hardening (see there),
	 *  NOT a tier change.
	 */

	/** Fired on every player-facing card-play refusal (gold, invalid spot, missing data). HUD may bind (TASK-011). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Cards")
	FOnCardPlayRefused OnCardPlayRefused;

	/** Fired on EVERY refused play or discard with the §3.0-style reason string. The hand HUD (TASK-033) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Cards")
	FOnCardRefused OnCardRefused;

	/**
	 *  Fired by SetUnitCommand every time the player latches a new stance
	 *  (Attack/Hold/Defend), carrying the new command as a byte (Shield Wall
	 *  commands, W1 TASK-274). The HUD command indicator (TASK-276) binds here.
	 *  The consuming units (TASK-275) read the live getters each tick instead.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Commands")
	FOnUnitCommandChanged OnUnitCommandChanged;

	/** Fired with each group-pick stage prompt; empty = pick over / groups released — the HUD falls back to its stance display (TASK-344; WBP_HUD binds additively in TASK-345). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Commands")
	FOnCommandPromptChanged OnCommandPromptChanged;

	/** The active latched unit-command stance (Attack/Hold/Defend). Defaults to Attack, but HasIssuedCommand() is false until the player first presses a key (TASK-274/275). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Commands")
	ESiegeUnitCommand GetCurrentCommand() const { return CurrentCommand; }

	/** True once the player has issued ANY command this match — while false the summoned units run their legacy body (TASK-275 gate; zero behavior change until the first key). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Commands")
	bool HasIssuedCommand() const { return bHasIssuedCommand; }

	/**
	 *  Live view of a group order (TASK-344): the group with the given id, or
	 *  nullptr when no such group exists any more (T/E release, all-dead prune,
	 *  steal-emptied, Play Again) — the unit-side null result SELF-HEALS the
	 *  unit back to the legacy stance gate. The pointer aliases into UnitGroups:
	 *  read it within the current call stack only, NEVER cache it (the array
	 *  mutates on confirm/steal/prune).
	 */
	const FSiegeUnitGroup* FindUnitGroup(int32 GroupId) const;

	//~ ─── FOLLOW command (TASK-395; CONVENTIONS "FOLLOW command + the
	//~     DEFAULT-STANCE law + the MINER command rework (2026-08-02)" §1/§2/§4,
	//~     signatures PINNED character-for-character by §7) ───
	//~
	//~ ⚠️ This block is `public:` ON PURPOSE and the access level is part of the
	//~ pin: the UNIT side (TASK-396 ASummonedUnit::UpdateStateFollow + its
	//~ BeginPlay auto-enroll) and the MINER side (TASK-397/398) call all three
	//~ from OUTSIDE this class, having resolved this controller through
	//~ FindControllerForTeam. The whole FOLLOW-COMMAND batch compiles as ONE UBT
	//~ module against that list.

	/**
	 *  THE DEFAULT FOLLOW GROUP (CONVENTIONS §2) — lazily creates, and returns
	 *  the id of, the ONE follow group this controller owns.
	 *
	 *  THERE IS EXACTLY ONE FOLLOW GROUP PER CONTROLLER. Pressing C never makes
	 *  a second one: it ADDS the circled units to this same group (stealing them
	 *  out of any Hold/Ambush group, per the shipped re-selection law). That is
	 *  what makes "the spawn default" and "the C command" the same object, and
	 *  it is why Jonathan's "there is only one circle used for this" is
	 *  satisfiable. The group is a NORMAL FSiegeUnitGroup with Type == Follow,
	 *  zero radii, zero centers and NULL marker decals — a follow group owns no
	 *  ground, so it gets no persistent ground marker.
	 *
	 *  LIFECYCLE (spec item 6): destroyed with every other group by the T/E
	 *  release law (ClearAllUnitGroups), by Play Again (HandleMatchReset), and
	 *  by the ≤1 s PruneUnitGroups reaper once its last member dies. EVERY one
	 *  of those paths resets DefaultFollowGroupId to INDEX_NONE, AND this
	 *  function re-validates the stored id against the live array before
	 *  returning it — belt-and-braces, so a stale id can never leak and the
	 *  group simply RE-DERIVES LAZILY on the next enroll or C press.
	 *
	 *  Returns INDEX_NONE on a non-authority caller (the M8 D5 observer posture:
	 *  group state is host-side in P1) — callers treat that as "skip silently".
	 */
	int32 EnsureDefaultFollowGroup();

	/**
	 *  Enrolls one unit in the default follow group (CONVENTIONS §2) — the SEAM
	 *  the unit-side spawn auto-enroll (TASK-396, on ASummonedUnit::BeginPlay)
	 *  and the C-key confirm both funnel through, so there is exactly one place
	 *  that knows how a unit joins Follow.
	 *
	 *  Does, in order: eligibility gate (Unit->IsFollowCommandEligible() — the
	 *  TASK-396 predicate: CanFollowHero() && Blue && alive && not frozen, which
	 *  is what keeps the Ogre/Sapper and every Red unit out) · lazy group
	 *  creation · STEAL out of any other group · append · assign a deterministic
	 *  golden-angle sunflower GroupStationOffset inside FollowFormationRadius,
	 *  computed ONCE here · AssignCommandGroup · reap any group the steal
	 *  emptied.
	 *
	 *  IDEMPOTENT: a unit already in the follow group keeps its station and is
	 *  not re-stationed (so a C press over already-following units is a no-op
	 *  for them). NULL-SAFE AND SILENT on every refusal — a missed enroll must
	 *  degrade to today's behavior, never to a crash or a stall.
	 */
	void EnrollInDefaultFollowGroup(ASummonedUnit* Unit);

	/**
	 *  THE HERO ANCHOR (CONVENTIONS §4) — this controller's live pawn, or
	 *  nullptr when there is no live hero to follow.
	 *
	 *  ⚠️ RESOLVE LIVE, EVERY STATE TICK, AND NEVER CACHE THE RESULT. The
	 *  respawn path may hand back a DIFFERENT pawn actor, and a cached pointer
	 *  would follow a corpse forever; resolving live is exactly what makes hero
	 *  respawn work for free. Callers reach this controller through
	 *  FindControllerForTeam(World, Team) — GetFirstPlayerController() is BANNED
	 *  in gameplay code (M8 TEAM LAW).
	 *
	 *  HERO-DEATH RULING (manager, CONVENTIONS §4 flag): a DEAD hero is NOT an
	 *  anchor. This returns nullptr while the pawn is missing, pending-kill or
	 *  AHeroCharacter::IsDead(), and the follow body's contract on nullptr is to
	 *  HOLD POSITION (EnterIdle, no target, no march, no attack) and resume the
	 *  instant a live pawn resolves again — including a brand-new post-respawn
	 *  pawn. Rejected and recorded: marching to the corpse; falling back to
	 *  Defend (that would make them fight, breaking "following units never
	 *  attack").
	 */
	AActor* GetFollowAnchor() const;

	/**
	 *  ⚠️ THE ANTI-REPATH BAND (CONVENTIONS §4, manager ruling 10) — the follow
	 *  body may re-issue EnterAdvanceToLocation ONLY when the recomputed station
	 *  has drifted more than this from the goal it last issued.
	 *
	 *  THIS IS A HARD REQUIREMENT, NOT POLISH. The anchor MOVES, and
	 *  EnterAdvanceToLocation's own re-path guard is a 1 uu Equals() test
	 *  (SummonedUnit.cpp — "bPointChanged"), so a station recomputed from a
	 *  walking hero clears it EVERY 0.25 s state tick. Re-pathing at a moving
	 *  goal every tick is precisely the mill that produced TASK-280 ("units
	 *  freeze just past midfield") and TASK-282 ("halt just short of the
	 *  castle"): each request restarts path-following before the previous one
	 *  produced motion. An unconditional re-path is a QA FAIL.
	 *
	 *  Read by the unit side off the resolved owning-team controller (the tunable
	 *  is pinned as ASiegePlayerController::FollowRepathTolerance and lives here
	 *  so one feel-pass value drives every follower). FLAGGED tunable.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float FollowRepathTolerance = 250.f;

	/**
	 *  Radius of the ring the following squad spreads in around the hero
	 *  (CONVENTIONS §8) — the sunflower stations computed at enroll all lie
	 *  inside it. Public for the same cross-task reason as
	 *  FollowRepathTolerance. FLAGGED tunable.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float FollowFormationRadius = 900.f;

	/** BlueprintPure mirror of FollowRepathTolerance (the member itself is the pinned C++ seam; this is the BP/getter-style read). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Commands")
	float GetFollowRepathTolerance() const { return FollowRepathTolerance; }

	/** BlueprintPure mirror of FollowFormationRadius (see GetFollowRepathTolerance). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Commands")
	float GetFollowFormationRadius() const { return FollowFormationRadius; }

	/**
	 *  Latches a new unit-command stance (Shield Wall, W1 TASK-274): sets
	 *  CurrentCommand, marks bHasIssuedCommand true (so the units switch off the
	 *  legacy body), and broadcasts OnUnitCommandChanged(NewCommand). Called by
	 *  the T/E immediate handlers (the HOLD stance is SUPERSEDED by group orders
	 *  — TASK-344 — and no longer latched by anything; the enum member survives
	 *  for WBP_HUD's switch pins). Idempotent — re-issuing the same stance
	 *  re-affirms the HUD.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Commands")
	void SetUnitCommand(ESiegeUnitCommand NewCommand);

	/**
	 *  Plays the card in hand slot 0..5 (GDD §3.5; keys 1..6 / TASK-033 card
	 *  buttons). Refuses (log + OnCardRefused, play refusals also
	 *  OnCardPlayRefused) on: empty/out-of-range slot, missing DT_Cards row, or
	 *  gold < Cost (grey-out is the widget's job). Routing by CardType:
	 *  Unit/Building/Economy → placement mode for the slot's CardID (entry can
	 *  additionally refuse a dead hero "Hero is down" or a capped Miner "Miner
	 *  limit reached", §3.3 — TASK-030); the card leaves the hand ONLY at placement
	 *  CONFIRM (M2 ruling), so cancel costs nothing. HeroUpgrade/Utility → resolve
	 *  IMMEDIATELY with NO placement step (TASK-059): the effect lands, the Cost is
	 *  spent, and a replacement is drawn — an over-cap upgrade or an unrepairable
	 *  Masons is refused with NO gold moved (§3.0/§3.10 full refund). Spell (GDD
	 *  §3.11, M5 TASK-100) → SpellEffect GoldSteal resolves INSTANTLY on play
	 *  (ruling 7: deduct-then-resolve, refusal-safe — a resolver refusal fully
	 *  refunds); every other SpellEffect enters TARGETING mode, where the card
	 *  leaves the hand only at LMB CONFIRM (cancel costs nothing). Post-match,
	 *  mid-placement, and mid-targeting presses are ignored (no broadcast),
	 *  mirroring the M1 EnterPlacementMode early-outs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Cards")
	void PlayHandSlot(int32 Slot);

	/**
	 *  Discards the card in hand slot 0..5 for the fixed DiscardCost (1 gold,
	 *  GDD §3.6) and draws its replacement. Refuses (log + OnCardRefused) on:
	 *  empty/out-of-range slot — checked BEFORE any gold moves
	 *  (qa/TASK-022-report.md WARN-1 guard) — or SpendGold refusal at 0 gold.
	 *  Ignored after match end; refused during placement mode (discarding the
	 *  slot being placed would desync the pending confirm).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Cards")
	void DiscardHandSlot(int32 Slot);

	/** Deck & hand model (TASK-022). Never null (default subobject). TASK-029/033 widgets seed-then-bind from it. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	UDeckComponent* GetDeckComponent() const { return DeckComponent; }

	/**
	 *  Starts placement mode for the given card (PlayHandSlot, the HUD card
	 *  button, or the IA_Card1 key). Reads the card's row from
	 *  /Game/Data/DT_Cards (GDD §3.0 — never hardcoded) and refuses (log +
	 *  both refusal delegates) when the data is missing, the player can't
	 *  afford it, the hero is dead, or — Miner card only — the §3.3
	 *  alive-miner cap is full (CanAddMiner pre-check, "Miner limit reached",
	 *  no gold moves — TASK-030). Post-match / already-placing calls are
	 *  silent ignores (M1 early-outs).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Cards")
	void EnterPlacementMode(FName CardID);

	/**
	 *  Leaves placement mode: destroys the ghost, restores game-only input,
	 *  and ALWAYS releases the hero melee suppression (QA TASK-003 warning 2)
	 *  — the release happens before any early-out, so every caller (confirm,
	 *  cancel, match end, hero death, unpossess, EndPlay) is a safe exit path.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Cards")
	void ExitPlacementMode();

	/**
	 *  Starts targeting mode for the given SPELL card (M5 ruling 8, TASK-100)
	 *  — placement mode's sibling. Reads the card's row from /Game/Data/
	 *  DT_Cards (GDD §3.0 — never hardcoded) and refuses (log + both refusal
	 *  delegates) when the data is missing, the CardType is not Spell, the
	 *  player can't afford it, or the hero is dead; a GoldSteal card never
	 *  targets and reroutes to the ruling-7 instant resolve instead (hand-less
	 *  on this direct path — PlayHandSlot owns hand plays). Post-match /
	 *  already-placing / already-targeting calls are silent ignores (the M1
	 *  early-out pattern). Gold moves ONLY at LMB confirm.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Cards")
	void EnterTargetingMode(FName CardID);

	/**
	 *  Leaves targeting mode: destroys the reticle decal, restores game-only
	 *  input, and ALWAYS releases the hero melee suppression BEFORE any
	 *  early-out (the ExitPlacementMode law, qa/TASK-003-report.md warning 2)
	 *  — every exit path (confirm, resolver-false confirm exit, cancel via
	 *  action or polled RMB/Esc, match end, hero death, unpossess, match
	 *  reset, EndPlay) funnels through here. Cancel is FREE: no gold has
	 *  moved before confirm (M5 ruling 8).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Cards")
	void ExitTargetingMode();

	/**
	 *  Match over (GDD §3.9) — called by ASiegeGameMode (TASK-006) with the
	 *  winning team. Exits placement mode FIRST, then shows WBP_VictoryScreen
	 *  (soft class, null-safe) and switches to UI-only input.
	 *
	 *  Widget contract (TASK-011): if WBP_VictoryScreen implements a function
	 *  named exactly "SetWinner" taking a single ETeamId parameter, it is
	 *  called (by name, null-safe) after CreateWidget and BEFORE AddToViewport
	 *  so the widget can branch Victory/Defeat on the value.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void HandleMatchEnd(ETeamId Winner);

	/**
	 *  Play Again support (GDD §3.9) — called by ASiegeGameMode::PlayAgain
	 *  (TASK-006): defensively exits placement mode FIRST
	 *  (qa/TASK-023-report.md WARN — an out-of-contract mid-placement call
	 *  must never rebuild the hand under a live PendingHandSlot), removes the
	 *  victory screen if still up (idempotent with the widget's own
	 *  RemoveFromParent), clears the match-ended latch and any stuck
	 *  IA_UICursor hold, resets the deck to a fresh §3.4 deal
	 *  (DeckComponent->ResetDeck — the controller-side deck-reset entry point
	 *  TASK-024's PlayAgain v2 consumes), and restores game-only input with
	 *  the cursor hidden.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void HandleMatchReset();

	/**
	 *  M8 Play-Again entry (TASK-356 doc §3.4.2/§4.2 — the WBP_VictoryScreen
	 *  button rewires to THIS at TASK-355; host-only fallback recorded): on the
	 *  authority (host/standalone) it resolves the GameMode and calls PlayAgain()
	 *  directly — the same call the widget made, byte-identical (doc §10); on a
	 *  CLIENT it routes through ServerRequestPlayAgain (the ONE P1 RPC — the
	 *  GameMode does not exist on clients, D6).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void RequestPlayAgain();

	/**
	 *  THE one P1 server RPC (M8 doc §4.2/D6, CONVENTIONS RPC law —
	 *  Server<Verb><Noun>, Reliable, WithValidation): relays the client's
	 *  Play-Again press to the server GameMode. Validation returns true (no
	 *  input payload); the implementation's HasMatchEnded() check IS the intent
	 *  validation — a mid-match spam press reaches nothing. TASK-356 loop-1: the
	 *  implementation additionally REFUSES (with a precise error) if it ever runs
	 *  without authority — a Server RPC body executing on the caller means the
	 *  callspace resolved Local and the host never heard it (FINDING-4).
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestPlayAgain();

	/**
	 *  M8 client-local reset mirror (TASK-356 doc §3.4.2): the OnRep-driven
	 *  counterpart of ASiegeGameMode::PlayAgain step 6, which walks SERVER-side
	 *  controllers and can never reach a remote machine's local PC. Called by
	 *  ASiegeGameState's match-reset notify on the false edge of bMatchEnded.
	 *  Delegates to HandleMatchReset() — which already carries the local deck
	 *  ResetDeck() as the single controller-side §3.9 deck-reset entry point, so
	 *  the client's local display deck resets on the same path. Standalone:
	 *  never called (no OnRep fires — doc §10).
	 */
	void PerformLocalMatchReset();

	/**
	 *  M8 owning-team-controller resolve (TASK-356 doc §3.7 — the ruling-4
	 *  GetFirstPlayerController REPLACEMENT pattern): first ASiegePlayerController
	 *  whose ASiegePlayerState carries Team. Server-side it sees both PCs (the
	 *  host's and the server copy of the client's) and resolves either team;
	 *  in standalone the one (local, Blue) PC is exactly what the old
	 *  first-controller call returned (doc §10). Null-safe: no world / no match ⇒
	 *  nullptr. Forward-compatible with P2's server-side group/stance migration
	 *  (the resolve already reads the server copy).
	 */
	static ASiegePlayerController* FindControllerForTeam(UWorld* World, ETeamId Team);

	/** True while the placement ghost owns the cursor/LMB. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Cards")
	bool IsInPlacementMode() const { return bInPlacementMode; }

	/** True while the spell reticle owns the cursor/LMB (M5 targeting mode, TASK-100). Mutually exclusive with placement mode. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Cards")
	bool IsInTargetingMode() const { return bInTargetingMode; }

	/** True from HandleMatchEnd until HandleMatchReset. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	bool HasMatchEnded() const { return bMatchEnded; }

	/**
	 *  Shared unit-spawn entry (GDD §3.0 Swarm, TASK-059) reused by the player
	 *  confirm path AND the bot (TASK-060) so both produce identical swarms. Spawns
	 *  Count copies of UnitClass for CardID/Team: Count > 1 arranges them evenly on a
	 *  circle of Radius around Center with EACH ring point navmesh-projected (falling
	 *  back to the raw ring point when there is no nav data / projection fails);
	 *  Count <= 1 spawns a single unit AT Center with no reprojection (the confirm
	 *  path already validated it on the navmesh — the M1/M2 single-unit spawn stays
	 *  byte-for-byte). Every copy is capsule-lifted onto the ground and
	 *  deferred-spawned → InitUnit(Team, CardID) → FinishSpawning (stats bind before
	 *  BeginPlay, TASK-004/030). Static + fully parameterized (no controller state)
	 *  so the bot can call it as ASiegePlayerController::SpawnUnitSwarm(...). Returns
	 *  the spawned units (empty on total failure); the CALLER owns the gold gate —
	 *  this helper never touches gold.
	 */
	static TArray<ASummonedUnit*> SpawnUnitSwarm(UWorld* World, UClass* UnitClass, FName CardID, ETeamId Team, AActor* SpawnOwner, APawn* SpawnInstigator, const FVector& Center, int32 Count, float Radius);

protected:

	/**
	 *  Normalizes the input posture FIRST (TASK-074, CONVENTIONS "Input-mode
	 *  ownership (level-travel law)"): SetInputMode state persists on the
	 *  UGameViewportClient across OpenLevel travel, so the controller applies
	 *  its own GameOnly free-look + hidden cursor via ApplyCursorInputState()
	 *  instead of trusting whatever posture the traveling level (e.g.
	 *  L_MainMenu's UIOnly) left behind. Then builds the deck at match start
	 *  (GDD §3.4, TASK-022 timing contract) and creates and adds the HUD widget
	 *  (soft class, null-safe — TASK-011 builds it).
	 */
	virtual void BeginPlay() override;

	/** Defensive placement-mode exit on teardown (releases melee suppression, destroys the ghost, ends any IA_UICursor hold). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Binds IA_Card1..IA_Card6, IA_UICursor and IA_CancelPlace on the enhanced input component (all null-safe, soft-resolved by path; missing assets skip their binding). */
	virtual void SetupInputComponent() override;

	/** Placement mode per-frame work: cancel poll, cursor-to-ground trace, ghost update, confirm poll. */
	virtual void PlayerTick(float DeltaTime) override;

	/** Subscribes to the hero's OnHeroDied so death always exits placement mode (QA TASK-003 warning 2). */
	virtual void OnPossess(APawn* InPawn) override;

	/** Unsubscribes from the hero and defensively exits placement mode. */
	virtual void OnUnPossess() override;

	/**
	 *  IA_Card1 pressed (key "1"): plays hand slot 0 when it holds a card;
	 *  falls back to the M1 always-available Footman placement while the hand
	 *  is empty (the qa/TASK-021-report.md WARN-2 window — deck builds empty
	 *  until the TASK-031 reimport), so the M1 key-1 flow keeps working
	 *  unchanged until the real deck data lands.
	 */
	void OnCard1Pressed();

	/** IA_Card2..IA_Card6 pressed (keys "2".."6"): play hand slot 1..5 (bound with the slot as payload). */
	void OnCardSlotKeyPressed(int32 Slot);

	/** IA_UICursor (Left Alt) pressed: show the cursor (GameAndUI) and suspend camera look for HUD clicks (M2 input ruling). */
	void OnUICursorPressed();

	/** IA_UICursor released (Completed AND Canceled): restore game-only free-look unless placement mode still owns the cursor. */
	void OnUICursorReleased();

	/** IA_CancelPlace pressed (RMB/Esc): leave placement mode at no cost. */
	void OnCancelPlacePressed();

	/** IA_CmdAttack pressed (key T, TASK-273): immediately latch the Attack stance (SetUnitCommand). Cancels an in-progress group pick AND clears every group order first (the TASK-344 release law); ignored after match end. */
	void OnCmdAttackPressed();

	/** IA_CmdHold pressed (key R, TASK-273): enter the 3-stage HOLD group pick (BeginGroupPick — TASK-344 replaces the old one-circle stance pick). */
	void OnCmdHoldPressed();

	/** IA_CmdAmbush pressed (key F, asset lands in TASK-345): enter the 3-stage AMBUSH group pick (BeginGroupPick). F stays INERT until IA_CmdAmbush exists — the binding is skipped null-safe. */
	void OnCmdAmbushPressed();

	/** IA_CmdDefend pressed (key E, TASK-273): immediately latch the Defend stance (SetUnitCommand). Cancels an in-progress group pick AND clears every group order first (the TASK-344 release law); ignored after match end. */
	void OnCmdDefendPressed();

	/** Hero died (FOnHeroDied): exit placement mode so melee suppression is never left behind. */
	UFUNCTION()
	void HandleHeroDied(AHeroCharacter* DeadHero);

protected:

	/**
	 *  Deck & hand model (GDD §3.4, TASK-022) — default subobject named exactly
	 *  "DeckComponent". Built by this controller at BeginPlay (match start) and
	 *  reset in HandleMatchReset (the PlayAgain flow) — the component never
	 *  self-builds (TASK-022 flagged decision 12).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Deck")
	TObjectPtr<UDeckComponent> DeckComponent;

	/** M1 fallback card for key "1" while the hand is empty (WARN-2 window). TASK-033 retires the fallback with the HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	FName Card1CardID = FName(TEXT("Footman"));

	/**
	 *  CardID whose plays are gated by the alive-miner cap
	 *  (ASiegePlayerState::CanAddMiner — checked at placement entry AND
	 *  confirm, refusing "Miner limit reached" with zero gold movement).
	 *  // GDD §3.3 — active cap 6 miners; a 7th Miner card is refused per the
	 *  §3.0 refund rule (net-zero pre-check, M2 ruling)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	FName MinerCardID = FName(TEXT("Miner"));

	/** Utility Instant CardID that repairs the friendly castle over time (Masons, GDD §4). PlayHandSlot routes it through ResolveInstantPlay → ACastle::HealOverTime. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	FName MasonsCardID = FName(TEXT("Masons"));

	/** Masons: total castle HP repaired (GDD §4: 300). Mechanic magnitude → class UPROPERTY, not a CSV column (CONVENTIONS). // GDD §4 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards", meta = (ClampMin = "0"))
	float MasonsHealAmount = 300.f; // GDD §4

	/** Masons: seconds the repair is spread over (GDD §4: 10). // GDD §4 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards", meta = (ClampMin = "0.05"))
	float MasonsHealDuration = 10.f; // GDD §4

	/**
	 *  Economy-typed cards whose ACTOR is a building (TASK-059 carry-forward of the
	 *  TASK-057 routing note): Deep Mine's row is CardType Economy for the §8
	 *  raidable-economy semantics, but ADeepMine derives ABuilding and lives at
	 *  /Game/Blueprints/Buildings/. Listing its CardID here routes it down the
	 *  BUILDING spawn path + the §3.5 building-clearance rule instead of the unit
	 *  path (IsBuildingCard). Editable so a future Economy-building needs no code change.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	TArray<FName> BuildingEconomyCardIDs = { FName(TEXT("DeepMine")) };

	/** Fixed discard charge — discarding costs 1 gold and is refused below it (GDD §3.6). Mechanic rule, not a CSV column (CONVENTIONS registry). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards", meta = (ClampMin = "0"))
	int32 DiscardCost = 1;

	/** Card stat table (GDD §3.0). Imported in TASK-008 — resolved null-safe at play time. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/** HUD widget class, /Game/UI/WBP_HUD (TASK-011). Missing = log once, continue. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|UI")
	TSoftClassPtr<UUserWidget> HUDWidgetClass;

	/** End screen widget class, /Game/UI/WBP_VictoryScreen (TASK-011). Missing = log, match still ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|UI")
	TSoftClassPtr<UUserWidget> VictoryScreenClass;

	/**
	 *  Ghost fallback mesh, /Engine/BasicShapes/Sphere (engine asset,
	 *  read-only), used when the card's own /Game/Meshes/SM_<CardID> is
	 *  missing (TASK-030 ghost generalization; per-card meshes arrive in
	 *  TASK-014/037/038). Both missing = invisible ghost, placement still
	 *  works (M1 degradation rule).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	TSoftObjectPtr<UStaticMesh> GhostFallbackMeshAsset;

	/** Ghost material, /Game/Materials/M_Ghost (TASK-012; vector param "GhostColor"). Missing = default material. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	TSoftObjectPtr<UMaterialInterface> GhostMaterialAsset;

	/**
	 *  Spell reticle decal material, /Game/Materials/M_SpellReticle
	 *  (DeferredDecal domain, built in TASK-108). Null-safe per M5 ruling 8:
	 *  missing material ⇒ NO reticle visual, targeting still works, logged
	 *  once (bWarnedNoReticleMaterial).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Targeting")
	TSoftObjectPtr<UMaterialInterface> SpellReticleMaterialAsset;

	/**
	 *  Reticle footprint radius used when the spell row has no AoERadius
	 *  (the reticle is normally sized to the spell's OWN AoERadius so the
	 *  ring shows the true blast area — data-driven, GDD §3.0). Visual-only
	 *  fallback, not a CSV column.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Targeting", meta = (ClampMin = "0"))
	float SpellReticleDefaultRadius = 150.f;

	/** IA_Card1 slot. Left unset, it soft-resolves from Card1ActionAsset (no BP controller exists in M1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card1Action;

	/** IA_Card2 slot (key "2" -> hand slot 1). Left unset, it soft-resolves from Card2ActionAsset (asset arrives in TASK-032). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card2Action;

	/** IA_Card3 slot (key "3" -> hand slot 2). Left unset, it soft-resolves from Card3ActionAsset (TASK-032). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card3Action;

	/** IA_Card4 slot (key "4" -> hand slot 3). Left unset, it soft-resolves from Card4ActionAsset (TASK-032). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card4Action;

	/** IA_Card5 slot (key "5" -> hand slot 4). Left unset, it soft-resolves from Card5ActionAsset (TASK-032). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card5Action;

	/** IA_Card6 slot (key "6" -> hand slot 5). Left unset, it soft-resolves from Card6ActionAsset (TASK-032). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card6Action;

	/** IA_UICursor slot (hold Left Alt = cursor for HUD clicks, M2 input ruling). Left unset, it soft-resolves from UICursorActionAsset (TASK-032). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> UICursorAction;

	/** IA_CancelPlace slot. Left unset, it soft-resolves from CancelPlaceActionAsset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CancelPlaceAction;

	/** IA_CmdAttack slot (key T -> Attack stance, TASK-273). Left unset, it soft-resolves from CmdAttackActionAsset (asset may not exist yet — binding is skipped null-safe). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CmdAttackAction;

	/** IA_CmdHold slot (key R -> 3-stage HOLD group pick, TASK-273/344). Left unset, it soft-resolves from CmdHoldActionAsset (null-safe). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CmdHoldAction;

	/** IA_CmdDefend slot (key E -> Defend stance, TASK-273). Left unset, it soft-resolves from CmdDefendActionAsset (null-safe). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CmdDefendAction;

	/** IA_CmdAmbush slot (key F -> AMBUSH group pick, TASK-344; asset created in TASK-345). Left unset, it soft-resolves from CmdAmbushActionAsset — a missing asset skips the binding and leaves F inert (never a crash). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CmdAmbushAction;

	/** IA_CmdFollow slot (key C -> the ONE-STAGE FOLLOW pick, TASK-395; asset created in TASK-399). Left unset, it soft-resolves from CmdFollowActionAsset — a missing asset skips the binding and leaves C inert (never a crash). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CmdFollowAction;

	/** Soft path for IA_Card1 (/Game/Input/Actions/IA_Card1, created in TASK-009). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card1ActionAsset;

	/** Soft path for IA_Card2 (/Game/Input/Actions/IA_Card2, created in TASK-032). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card2ActionAsset;

	/** Soft path for IA_Card3 (/Game/Input/Actions/IA_Card3, created in TASK-032). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card3ActionAsset;

	/** Soft path for IA_Card4 (/Game/Input/Actions/IA_Card4, created in TASK-032). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card4ActionAsset;

	/** Soft path for IA_Card5 (/Game/Input/Actions/IA_Card5, created in TASK-032). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card5ActionAsset;

	/** Soft path for IA_Card6 (/Game/Input/Actions/IA_Card6, created in TASK-032). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card6ActionAsset;

	/** Soft path for IA_UICursor (/Game/Input/Actions/IA_UICursor, created in TASK-032). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> UICursorActionAsset;

	/** Soft path for IA_CancelPlace (/Game/Input/Actions/IA_CancelPlace, created in TASK-009). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CancelPlaceActionAsset;

	/** Soft path for IA_CmdAttack (/Game/Input/Actions/IA_CmdAttack, created in TASK-273 — T key). Null-safe: a missing asset skips its binding, logs once, never crashes. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CmdAttackActionAsset;

	/** Soft path for IA_CmdHold (/Game/Input/Actions/IA_CmdHold, created in TASK-273 — R key). Null-safe (see CmdAttackActionAsset). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CmdHoldActionAsset;

	/** Soft path for IA_CmdDefend (/Game/Input/Actions/IA_CmdDefend, created in TASK-273 — E key). Null-safe (see CmdAttackActionAsset). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CmdDefendActionAsset;

	/** Soft path for IA_CmdAmbush (/Game/Input/Actions/IA_CmdAmbush, created in TASK-345 — F key). Null-safe (see CmdAttackActionAsset): until the asset lands, F is simply inert. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CmdAmbushActionAsset;

	/**
	 *  Soft path for IA_CmdFollow (/Game/Input/Actions/IA_CmdFollow, created in
	 *  TASK-399 — C key, mapped in /Game/Input/IMC_Hero by the same task). NULL-
	 *  SAFE IS LAW HERE AND IT IS THE DESIGNED STATE AT COMPILE TIME (TASK-395
	 *  ships before TASK-399): an unresolved asset skips the binding, logs ONE
	 *  line through ResolveInputAction, and leaves C completely inert — never a
	 *  crash, and every other key keeps working.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CmdFollowActionAsset;

	/**
	 *  Half-extent (XY) of the player's spawn box — a 2D square centered on the
	 *  owned Castle_Blue that REPLACES the retired X<=0 half-line spawn gate
	 *  (W1-PREP additions 3, TASK-261). Default (2460,2460) — re-derived 840 → 2460
	 *  by TASK-349 (CONVENTIONS "Castle 3× HOLLOW" paired-tunable law: half-extent
	 *  ≈ the 3× castle's full 2460 width, preserving the original "box ≈ castle
	 *  width" intent), so the box now COVERS the castle's walkable interior and
	 *  card placement inside works by construction (placement truth = nav
	 *  projection + collision + existing clearances; the plinth dead-zone is
	 *  RETIRED). Placement is valid inside this box OR inside a Blue-owned capture
	 *  zone; the downstream navmesh / slope / clearance checks are unchanged and
	 *  still apply. PAIRED-TUNABLE (3-way law): ≡ ACastle::SpawnBoxHalfExtent ≡
	 *  ASiegeBotController::SpawnBoxHalfExtent — keep the three in lockstep. The
	 *  mid ACaptureZone::ZoneHalfExtent deliberately STAYS (840,840): its "same
	 *  size as the spawn box" origin was descriptive, never a pairing law (FLAGGED
	 *  to Jonathan).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FVector2D SpawnBoxHalfExtent = FVector2D(2460.f, 2460.f);

	/**
	 *  Group-order pick tunables (TASK-344, CONVENTIONS "Group orders — 3-zone
	 *  HOLD + AMBUSH"): ALL SIX are flagged for Jonathan's feel-pass. The wheel
	 *  steps the ACTIVE pick circle's radius by GroupRadiusWheelStep per scroll
	 *  notch, clamped to [GroupRadiusMin, GroupRadiusMax]; each stage opens at
	 *  its own default radius.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Commands", meta = (ClampMin = "1"))
	float GroupRadiusWheelStep = 100.f;

	/** Smallest radius the wheel can shrink any pick circle to (TASK-344). FLAGGED tunable. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float GroupRadiusMin = 200.f;

	/** Largest radius the wheel can grow any pick circle to (TASK-344). FLAGGED tunable. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float GroupRadiusMax = 5000.f;

	/** Stage-1 SELECT circle default radius: eligible units inside it (2D) at confirm join the group (TASK-344). FLAGGED tunable. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float GroupSelectRadiusDefault = 1200.f;

	/** Stage-2 POSITION zone default radius: the station zone the group sunflower-spreads inside, and the tier-2 engage disc (TASK-344). FLAGGED tunable. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float GroupPositionRadiusDefault = 700.f;

	/** Stage-3 ATTACK zone default radius: the tier-1 engage trigger (TASK-344). FLAGGED tunable. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Commands", meta = (ClampMin = "0"))
	float GroupAttackRadiusDefault = 1500.f;

	/**
	 *  Minimum 2D distance from the nearest other ABuilding for a
	 *  Building-card placement — closer shows the red ghost and refuses the
	 *  click with no gold spent. Castles are NOT buildings for this rule
	 *  (class-disjoint) and since TASK-349 carry NO placement clearance of their
	 *  own at all — the plinth keep-out that once covered them is RETIRED
	 *  (spawn-inside the own castle is the feature; the ENEMY side is fenced by
	 *  the team gating, not by placement rules). Mechanic rule, not a CSV column
	 *  (CONVENTIONS registry). // GDD §3.5 — buildings require 200 units of
	 *  clearance from any other building
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement", meta = (ClampMin = "0"))
	float BuildingClearance = 200.f;

	/**
	 *  Box half-extent for the placement navmesh projection (GDD §3.5 via
	 *  TASK-030): a point is placeable only if it projects onto the navmesh
	 *  within this extent — refusing castle-roof and other far-above-navmesh
	 *  cursor hits (the M1 carry-over; since TASK-349 this projection IS the
	 *  placement truth — the plinth keep-out is retired). Keep the vertical
	 *  half-extent tight (50): a hit on elevated non-walkable geometry must NOT
	 *  project down to ground navmesh and read as valid, while ground-level
	 *  interior-floor hits (the hollow castle's spawn-inside surface) project
	 *  within it by construction.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FVector NavProjectionExtent = FVector(50.f, 50.f, 50.f);

	//~ CastlePlinthClearance (420, M1 carry-over) RETIRED by TASK-349 (CONVENTIONS
	//~ "Castle 3× HOLLOW" plinth-retirement law): interior spawn/placement is now
	//~ the FEATURE, so the own-castle keep-out would refuse exactly what the
	//~ directive asks for. Placement truth = navmesh projection + collision +
	//~ existing clearances (the hollow castle's interior floor is navmesh'd; the
	//~ solid plinth that spawned walkable rim islands no longer exists). The
	//~ ENEMY interior is unreachable via the team gating (GateBlockerVolume +
	//~ UNavFilter_Team*), never via a dead zone. IsPointInsideCastlePlinth is
	//~ retired with it.

	/**
	 *  Maximum ground slope, in degrees from horizontal, a BUILDING card may
	 *  be placed on (M4.5 ruling 7) — steeper candidate points show the red
	 *  ghost and refuse the confirm click ("Too steep") with no gold spent.
	 *  Slope is measured by a straight-down line trace at the candidate point
	 *  (±500 Z, ECC_Visibility — the same channel as the cursor trace, so the
	 *  measured surface IS the surface the ghost stands on); slope = angle
	 *  between the hit's ImpactNormal and +Z; a trace miss refuses
	 *  (fail-closed). Units/miners are exempt — a navmesh-valid point
	 *  suffices, unchanged. Mechanic rule, not a CSV column (CONVENTIONS
	 *  registry). // GDD §5 (M4.5) — buildings refused on ground steeper
	 *  than 20°; hill crowns (<=10°) stay legally placeable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement", meta = (ClampMin = "0", ClampMax = "90"))
	float MaxPlacementSlopeDegrees = 20.f; // GDD §5 (M4.5)

	/**
	 *  Minimum 2D distance from any actor carrying tag "Obstacle" (trees AND
	 *  rocks — Fab amendment; tags set by TASK-095) for a BUILDING placement
	 *  (M4.5 ruling 7) — closer shows the red ghost and refuses the confirm
	 *  click ("Too close to obstacles") with no gold spent. The check is
	 *  tag-driven, so new obstacle types never require code changes.
	 *  Units/miners are exempt. Mechanic rule, not a CSV column (CONVENTIONS
	 *  registry). // GDD §5 (M4.5) — buildings need 150 units of clearance
	 *  from obstacles
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement", meta = (ClampMin = "0"))
	float ObstaclePlacementClearance = 150.f; // GDD §5 (M4.5)

	/** Ghost tint for a valid point (M_Ghost "GhostColor", TASK-012). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FLinearColor ValidGhostColor = FLinearColor(0.f, 1.f, 0.f);

	/** Ghost tint for an invalid point. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FLinearColor InvalidGhostColor = FLinearColor(1.f, 0.f, 0.f);

	/** Yaw applied to the ghost so raw SM_<CardID> meshes face +X — the whole family shares SM_Footman's export orientation and -90° fix (TASK-014/037/038 handoffs). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	float GhostYawOffset = -90.f;

	/** Swarm ring radius (GDD §3.0): a Unit card with SwarmCount > 0 spawns its copies on a circle of this radius around the placement point (Militia Mob = 4 at 300). // GDD §3.0 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement", meta = (ClampMin = "0"))
	float SwarmSpawnRadius = 300.f; // GDD §3.0

private:

	/** Why the latest traced cursor point is invalid — drives the confirm-refusal message (each building-only rule gets its own reason). */
	enum class EPlacementInvalidReason : uint8
	{
		None,      // point is valid
		Point,     // no ground hit / outside the spawn box+zone / off the navmesh (plinth keep-out retired, TASK-349)
		Slope,     // building on ground steeper than MaxPlacementSlopeDegrees ("Too steep", M4.5)
		Obstacle,  // building within ObstaclePlacementClearance of an "Obstacle"-tagged actor ("Too close to obstacles", M4.5)
		Clearance  // building within BuildingClearance of another building
	};

	/**
	 *  Stage of the group-order pick (TASK-344): None = no pick live (the wheel
	 *  poll and the pick branch of PlayerTick are inert). Select → Position →
	 *  AttackZone, each stage a wheel-resizable cursor circle; LMB confirms a
	 *  stage, RMB/Esc cancels the WHOLE flow at any stage.
	 *
	 *  FOLLOW (TASK-395) reuses this machinery for ONE stage only: it enters at
	 *  Select and CONFIRMS THERE (ConfirmFollowPick), so Position and AttackZone
	 *  are unreachable for it. The enum is unchanged — Follow is a shorter path
	 *  through the same states, NOT a 3-stage flow with two stages disabled.
	 */
	enum class EGroupPickStage : uint8
	{
		None,
		Select,
		Position,
		AttackZone
	};

	/**
	 *  Confirm click (LMB in mode): reason-mapped refusal on an invalid point
	 *  (stay in mode); miner-cap re-gate and BP-class resolve refusals exit the
	 *  mode with no gold spent (nothing a different click could fix); otherwise
	 *  SpendGold, deferred-spawn by CardType (InitUnit / InitBuilding — the
	 *  TASK-027 contract passes the REAL CardID, qa/TASK-027 WARN-2),
	 *  FinishSpawning, ConfirmPlayFromHand, exit.
	 */
	void TryConfirmPlacement();

	/** Per-frame: cursor-to-ground trace, validity v4 (ground, spawn box / captured zone, navmesh projection — plinth keep-out retired, TASK-349; buildings add slope, obstacle clearance, building clearance — TASK-093), ghost position + color. */
	void UpdatePlacementGhost();

	/** Spawns the ghost actor (movable, collision off, per-card SM_<CardID> or the fallback sphere + M_Ghost MID) — every asset null-safe. */
	void SpawnPlacementGhost();

	/** Destroys the ghost actor and drops the dynamic material instance. */
	void DestroyPlacementGhost();

	/**
	 *  Confirm click (LMB in targeting mode, M5 ruling 8 / TASK-100): a
	 *  trace-miss point refuses free and STAYS in mode (a different point can
	 *  succeed); otherwise deduct Cost THEN USpellLibrary::ResolveSpell at
	 *  the reticle point (card-leaves-hand-at-CONFIRM law). Resolver false ⇒
	 *  FULL refund (§3.0 net-zero), HUD reason, card kept, mode EXITED —
	 *  resolver refusals are position-independent by the SpellLibrary
	 *  contract (the missing-BP-class placement precedent). On success the
	 *  hand slot is consumed (ConfirmPlayFromHand) and the mode exits.
	 *
	 *  AIM PASS (TASK-236, CONVENTIONS "Spell delivery overhaul 2026-07-21"):
	 *  for HeroLine spells (Fireball/FrostNova — USpellLibrary::
	 *  IsLineDeliverySpell) the reticle point is the AIM-POINT, not an impact
	 *  center, and a surface hit is NOT required — a trace-miss confirm
	 *  synthesizes the aim-point from the deprojected cursor ray (flattened
	 *  horizontal, hero-anchored). The free stay-in-mode refusals generalize
	 *  to "no aim direction": failed deproject, vertical ray, or a reticle
	 *  with zero horizontal offset from the hero. GroundCircle spells keep
	 *  the M5 confirm gate byte-for-byte.
	 */
	void TryConfirmSpellTarget();

	/**
	 *  Per-frame targeting work: cursor-to-surface trace (REUSING
	 *  TraceCursorToGround — the M4.5 carry-in LAW: the reticle Z comes from
	 *  the traced surface, never the Z=0 plane) + reticle position/visibility.
	 *  Deliberately NO half restriction, NO navmesh projection, NO
	 *  slope/obstacle gates: spells land anywhere a surface answers (§3.5).
	 */
	void UpdateSpellReticle();

	/** Spawns the transient reticle decal actor (soft M_SpellReticle, null-safe — missing material means no visual, targeting continues; sized to the spell's AoERadius). */
	void SpawnSpellReticle();

	/** Destroys the reticle decal actor. */
	void DestroySpellReticle();

	/**
	 *  Enters the group-order pick (TASK-344): the R (Hold) / F (Ambush) entry
	 *  into the 3-stage SELECT → POSITION → ATTACK zone flow, and — since
	 *  TASK-395 — the C (Follow) entry into the ONE-STAGE flow that confirms at
	 *  SELECT and never opens a zone. Follows the placement /
	 *  targeting cursor posture (visible cursor + hero melee suppressed via
	 *  GroupPickHero so the confirm clicks don't also swing). Silently ignored
	 *  while placement OR spell targeting is live (the codebase's mutual-ignore
	 *  exclusivity — both directions), while a pick is already live, and after
	 *  match end. The pick owns the LMB (stage confirm), the WHEEL (polled
	 *  resize) and RMB/Esc (full-flow cancel); no stance or group state moves
	 *  until the stage-3 confirm.
	 */
	void BeginGroupPick(ESiegeGroupCommandType Type);

	/**
	 *  Per-frame pick work: cursor-to-surface trace (REUSING TraceCursorToGround
	 *  — the surface-projection law; never the Z=0 plane) into GroupPickLocation
	 *  / bGroupPickSurfaceValid, and moves the ACTIVE stage circle (hidden while
	 *  the cursor is on the sky — the confirm refuses on the same flag).
	 */
	void UpdateGroupPickReticle();

	/**
	 *  POLLED wheel resize (CONVENTIONS wheel law: NO new InputAction — the
	 *  wheel is globally unbound and must stay INERT outside the pick). Runs
	 *  ONLY from the pick branch of PlayerTick: each MouseScrollUp/Down notch
	 *  steps the ACTIVE circle's radius by GroupRadiusWheelStep, clamped to
	 *  [GroupRadiusMin, GroupRadiusMax], and resizes its decal in place.
	 */
	void ApplyGroupPickWheel();

	/**
	 *  LMB confirm for the CURRENT stage (TASK-344). A trace-miss (cursor on the
	 *  sky) refuses free and STAYS in the stage (the placement/targeting
	 *  trace-miss precedent). Select: sweeps the eligible units inside the circle
	 *  (2D) — IsGroupCommandEligible for the ZONE types, IsFollowCommandEligible
	 *  for Follow (CONVENTIONS §3's split predicate) — an EMPTY sweep
	 *  refuses-and-stays with a HUD reason; for FOLLOW the flow then TERMINATES in
	 *  ConfirmFollowPick (TASK-395, the one-stage law) and for Hold/Ambush the
	 *  circle is dropped in place and stage 2 opens. Position:
	 *  records the station zone, drops its circle, opens stage 3. AttackZone
	 *  (final): builds the FSiegeUnitGroup, STEALS re-selected units from older
	 *  groups (steal-emptied groups die immediately, markers included), computes
	 *  + nav-projects the golden-angle sunflower stations ONCE and pushes them
	 *  to the units, transfers the Position + Attack circles to the group as
	 *  persistent markers, and tears the pick down (the Select circle dies).
	 */
	void ConfirmGroupPickStage();

	/**
	 *  Leaves the pick flow with NO group/stance change (RMB/Esc at any stage,
	 *  and the defensive teardown paths). Follows the ExitTargetingMode law:
	 *  releases the GroupPickHero melee suppression BEFORE any early-out, then
	 *  destroys every pick circle this flow still owns, broadcasts an EMPTY
	 *  prompt (HUD falls back to the stance display) and restores the cursor
	 *  input state. Idempotent / no-op safe. The stage-3 confirm funnels through
	 *  here too — it nulls the transferred marker refs first, so only the Select
	 *  circle dies on a completed flow.
	 *
	 *  STAGE-AGNOSTIC BY CONSTRUCTION, which is why FOLLOW (TASK-395) needed no
	 *  new teardown site: this resets GroupPickStage to None whatever it held and
	 *  destroys every circle the flow still owns, so all nine existing teardown
	 *  callers (EndPlay · the PlayerTick polled RMB/Esc · OnUnPossess ·
	 *  OnCancelPlacePressed · OnCmdAttackPressed · OnCmdDefendPressed ·
	 *  HandleHeroDied · HandleMatchEnd · HandleMatchReset) already cover a Follow
	 *  pick. ConfirmFollowPick funnels through here too and — unlike the stage-3
	 *  confirm — nulls NOTHING first, so its transient select circle is destroyed
	 *  (a follow group owns no ground and gets no persistent marker).
	 */
	void CancelGroupPick();

	/**
	 *  Spawns ONE wheel-resizable ground circle for the pick (TASK-344) — the
	 *  SpawnSpellReticle recipe, cloned: null-safe M_SpellReticle (missing ⇒ no
	 *  visual, the pick still works off the trace; the shared warn-once latch),
	 *  IDENTITY spawn then ABSOLUTE -90 pitch (the TASK-100 composition lesson),
	 *  DecalSize=(500,R,R). Applies the optional per-stage tint through an MID
	 *  ("StageTint" — a silent no-op until TASK-345 adds the parameter). Returns
	 *  nullptr on any degrade; callers stay null-safe.
	 */
	ADecalActor* SpawnGroupCircleDecal(float Radius);

	/**
	 *  IA_CmdFollow pressed (key C, TASK-395; asset lands in TASK-399): enters
	 *  the ONE-STAGE FOLLOW pick via BeginGroupPick(Follow), which owns every
	 *  guard (match-ended, placement/targeting mutual exclusion, already-picking,
	 *  the M8 D5 client lockout). PRIVATE per the CONVENTIONS §7 pin — the
	 *  binding is taken inside SetupInputComponent, so the access level costs
	 *  nothing.
	 */
	void OnCmdFollowPressed();

	/**
	 *  THE ONE-STAGE TERMINAL CONFIRM (CONVENTIONS §1) — the whole difference
	 *  from Hold/Ambush. Called from ConfirmGroupPickStage's Select case when
	 *  GroupPickType == Follow, INSTEAD of opening stage 2: enrolls every swept
	 *  unit in the default follow group, tears the pick down through the ONE
	 *  teardown call (CancelGroupPick, which destroys the transient select
	 *  circle — a follow group owns no ground, so it gets no persistent marker),
	 *  and broadcasts the completion prompt afterwards so it is what remains on
	 *  the HUD. The pick can therefore NEVER reach Position or AttackZone.
	 */
	void ConfirmFollowPick();

	/**
	 *  Deterministic golden-angle sunflower station offset for the StationIndex-th
	 *  enrollment in the follow group — the shipped squad-spread recipe
	 *  re-anchored on a MOVING point (hero) instead of a fixed circle. Radius
	 *  R·sqrt((slot+0.5)/FollowFormationSlots), angle StationIndex·golden, with
	 *  slot = StationIndex modulo the nominal slot count: the RADIUS wraps so an
	 *  arbitrarily large squad always stays inside FollowFormationRadius, while
	 *  the golden angle keeps every ANGLE distinct, so no two live followers
	 *  share a station. Per-unit scalars only — no arrays on units (shipped law).
	 *
	 *  Deliberately NOT nav-projected (the one deviation from the stage-3 station
	 *  recipe): this is an OFFSET from a point that moves, so a projection taken
	 *  at enroll against a stale hero position would be meaningless. The unit's
	 *  EnterAdvanceToLocation already passes bProjectDestinationToNavigation.
	 */
	FVector ComputeFollowStationOffset(int32 StationIndex) const;

	/** Non-const FindUnitGroup for the enroll path (same aliasing rule: use within the call stack, never cache — the array mutates on confirm/steal/prune). */
	FSiegeUnitGroup* FindUnitGroupMutable(int32 GroupId);

	/**
	 *  1 s maintenance reaper (TASK-344, the ≤1 s marker-removal law): compacts
	 *  stale/dead Members out of every group and destroys any group with none
	 *  left — its markers die with it. Also invoked synchronously by the stage-3
	 *  steal so a steal-emptied group never outlives the confirm that emptied it.
	 */
	void PruneUnitGroups();

	/**
	 *  The RELEASE law (TASK-344): destroys EVERY group order — members cleared
	 *  explicitly (immediate, no self-heal wait), markers destroyed, prompt
	 *  emptied. Called by T/E (before SetUnitCommand) and by HandleMatchReset
	 *  (Play Again).
	 */
	void ClearAllUnitGroups();

	/** Broadcasts OnCommandPromptChanged AND logs the prompt (the ships-without-the-BP-bind law). Empty prompts broadcast silently (Verbose log). */
	void BroadcastCommandPrompt(const FString& Prompt);

	/**
	 *  Instant spell resolution (M5 ruling 7 — GoldSteal/Pickpocket): NO
	 *  reticle for a global effect. Deduct Cost THEN ResolveSpell,
	 *  refusal-safe: resolver false ⇒ FULL refund + HUD reason with the card
	 *  kept (§3.0 net-zero). Slot INDEX_NONE = hand-less direct entry (no
	 *  draw step). SiegeState is the already-resolved player state
	 *  (affordability pre-checked by the caller in the same call stack).
	 */
	void ResolveSpellInstant(int32 Slot, FName CardID, const FCardRow& Row, ASiegePlayerState& SiegeState);

	/** Cursor-to-world trace on the Visibility channel. True on a blocking hit ("ground hit"). */
	bool TraceCursorToGround(FHitResult& OutHit) const;

	/** Finds the card's DT_Cards row (soft load, null-safe). On failure returns nullptr and fills OutError. */
	const FCardRow* ResolveCardRow(FName CardID, FString& OutError) const;

	/**
	 *  Resolves the BP class to spawn for a card by its CardType (CONVENTIONS
	 *  composed soft-class paths, TASK-030): Unit/Economy →
	 *  /Game/Blueprints/Units/BP_Unit_<CardID> (must be an ASummonedUnit;
	 *  TASK-010/034); Building → /Game/Blueprints/Buildings/
	 *  BP_Building_<CardID> (must be an ABuilding; TASK-035). Missing or
	 *  incompatible = nullptr — the caller refuses the play with NO gold spent
	 *  (the M1 meshless-ASummonedUnit fallback is retired per the spec).
	 */
	UClass* ResolveCardActorClass(FName CardID, ECardType CardType) const;

	/**
	 *  True when the card spawns an ABuilding: every Building card, plus Economy
	 *  cards whose actor is a building (Deep Mine — CardType Economy, but ADeepMine
	 *  under /Blueprints/Buildings/, TASK-057). The single source of truth for the
	 *  confirm spawn branch, the §3.5 building-clearance rule, and the BP-class path
	 *  (BuildingEconomyCardIDs drives the Economy exception — TASK-059).
	 */
	bool IsBuildingCard(FName CardID, ECardType CardType) const;

	/**
	 *  Resolves a HeroUpgrade/Utility Instant IMMEDIATELY (GDD §3.5/§3.10/§4,
	 *  TASK-059) — no placement step. HeroUpgrade → AHeroCharacter::ApplyUpgrade
	 *  (TASK-058): spend Cost + draw only on Applied; RefusedAtMaxStacks →
	 *  "… at max stacks" with no spend; RefusedInvalidCard → refuse. Utility Masons
	 *  → repair the friendly castle (ACastle::HealOverTime) then spend + draw; any
	 *  other Utility CardID is refused. EVERY refusal path moves NO gold (§3.0 full
	 *  refund). SiegeState is the already-resolved player state (affordability was
	 *  pre-checked in PlayHandSlot).
	 */
	void ResolveInstantPlay(int32 Slot, FName CardID, const FCardRow& Row, ASiegePlayerState& SiegeState);

	/** Instant resolution's hand step: ConfirmPlayFromHand(Slot) → discard + redraw (§3.4), null-safe, with a tripwire log on the impossible false return. */
	void ConfirmInstantDraw(int32 Slot, FName CardID);

	/** First living (non-destroyed) ACastle on FriendlyTeam, or nullptr (Masons repair target lookup, TASK-059). */
	ACastle* FindFriendlyCastle(ETeamId FriendlyTeam) const;

	/** Ghost mesh for a card: /Game/Meshes/SM_<CardID> (CONVENTIONS per-card visual contract), else GhostFallbackMeshAsset, else nullptr (invisible ghost). */
	UStaticMesh* ResolveGhostMesh(FName CardID) const;

	/**
	 *  True when Point projects onto the navmesh within NavProjectionExtent
	 *  (GDD §3.5 placement rule, TASK-030). No nav system / no nav data in the
	 *  world = degrade OPEN to the M1 half+ground rule with ONE warning (house
	 *  null-safety law: a missing system never bricks placement) — NOT a
	 *  refusal. Non-const only for the warn-once latch.
	 */
	bool IsPointOnNavmesh(const FVector& Point);

	/** True when Point is >= BuildingClearance (2D) from every live ABuilding (§3.5 building rule; dying buildings skipped via IsBuildingDestroyed). */
	bool HasBuildingClearance(const FVector& Point) const;

	/**
	 *  True when the ground at Point is flat enough for a BUILDING (M4.5
	 *  ruling 7, TASK-093): a straight-down line trace (Point ± 500 Z,
	 *  ECC_Visibility — the cursor-trace channel, documented flagged decision)
	 *  must produce a blocking hit whose ImpactNormal is within
	 *  MaxPlacementSlopeDegrees of +Z. A trace miss returns false (fail-closed,
	 *  Verbose log) — a point the world cannot answer for is not placeable.
	 *  // GDD §5 (M4.5)
	 */
	bool IsGroundSlopePlaceable(const FVector& Point) const;

	/**
	 *  True when Point is >= ObstaclePlacementClearance (2D) from every actor
	 *  carrying tag "Obstacle" (exact FName — trees AND rocks per the Fab
	 *  amendment; tags set by TASK-095). Plain world-actor iteration, no
	 *  caching: obstacle counts are ~20 (M4.5 ruling 6) and the check only
	 *  runs during placement mode (documented flagged decision).
	 *  // GDD §5 (M4.5)
	 */
	bool HasObstacleClearance(const FVector& Point) const;

	//~ IsPointInsideCastlePlinth RETIRED by TASK-349 (plinth-retirement law — see
	//~ the CastlePlinthClearance retirement note above; no placement path may
	//~ refuse the own castle's interior).

	/**
	 *  True when Point lies inside the player's spawn box — a 2D (XY) square
	 *  centered on the owned Castle_Blue (found via the team-filtered
	 *  TActorIterator<ACastle> pattern) with half-extent SpawnBoxHalfExtent.
	 *  This is the first spawn/region gate that REPLACES the retired
	 *  X<=PlacementMaxX half-test (W1-PREP additions 3, TASK-261). Null-safe: no
	 *  Blue castle in the world => refuse (warn once — polled per tick during
	 *  placement mode). Non-const only for the warn-once latch (mirrors
	 *  IsPointOnNavmesh).
	 */
	bool IsPointInOwnSpawnBox(const FVector& Point);

	/**
	 *  True when Point lies inside the single ACaptureZone AND Blue currently
	 *  owns it — the capture-spawn clause (W1-PREP additions 3, TASK-261). Finds
	 *  the one CaptureZone_Center via TActorIterator<ACaptureZone> (null-safe if
	 *  absent = pre-capture behavior, mid unspawnable) and defers the whole test
	 *  to ACaptureZone::CanTeamSpawnHere(ETeamId::Blue, Point) (TASK-260 API),
	 *  which folds the box test AND the Blue-owner match.
	 */
	bool IsPointInCapturedZone(const FVector& Point) const;

	/** Broadcasts a play refusal on BOTH delegates: OnCardPlayRefused (M1 card context) and OnCardRefused (M2 reason string). */
	void RefuseCardPlay(FName CardID, const FText& Reason);

	/** Broadcasts OnCardRefused only — the shared M2 refusal surface (discard refusals land here without the play delegate). */
	void BroadcastRefusal(const FText& Reason);

	/** Ends the IA_UICursor hold if active: clears bUICursorHeld and decrements the ignore-look counter exactly once. */
	void ClearUICursorHold();

	/** Hard slot if assigned, else LoadSynchronous of the soft path; null (with one log naming the creating task) if neither resolves. */
	UInputAction* ResolveInputAction(const TObjectPtr<UInputAction>& HardSlot, const TSoftObjectPtr<UInputAction>& SoftAsset, const TCHAR* ActionName, const TCHAR* CreatedInTask) const;

	/**
	 *  Applies the input state implied by the current cursor owners: placement
	 *  mode OR a held IA_UICursor = GameAndUI + visible cursor; neither = M1
	 *  game-only free-look with the cursor hidden. Never runs after match end —
	 *  HandleMatchEnd owns the UI-only end-screen state until HandleMatchReset
	 *  clears the latch.
	 */
	void ApplyCursorInputState();

	/** True while placement mode is active. */
	bool bInPlacementMode = false;

	/** True while IA_UICursor is held — pairs the SetIgnoreLookInput +1/-1 exactly once (the engine API is counter-based). */
	bool bUICursorHeld = false;

	/** Latched by HandleMatchEnd, cleared by HandleMatchReset. Blocks card plays while up. */
	bool bMatchEnded = false;

	/** Result of the latest cursor trace: ground hit AND on the Blue half. */
	bool bPlacementValid = false;

	/** Ground point of the latest valid-or-not cursor trace (unit spawn point on confirm). */
	FVector PlacementLocation = FVector::ZeroVector;

	/** Card being placed (set on EnterPlacementMode). */
	FName PendingCardID;

	/** Cost read from DT_Cards on EnterPlacementMode — spent only on a confirmed valid click. */
	int32 PendingCost = 0;

	/** CardType read from DT_Cards on EnterPlacementMode — selects the confirm spawn path and the §3.5 building clearance rule (TASK-030). */
	ECardType PendingCardType = ECardType::Unit;

	/** SwarmCount read from DT_Cards on EnterPlacementMode (TASK-059): >0 spawns that many copies at confirm for one Cost (Militia Mob = 4). 0/1 = single unit. */
	int32 PendingSwarmCount = 0;

	/** True when the pending card spawns an ABuilding (Building cards + Economy building cards like Deep Mine) — the single source for the confirm spawn branch AND the §3.5 clearance rule (TASK-059). */
	bool bPendingIsBuilding = false;

	/** Reason the latest traced point is invalid (None while bPlacementValid; recomputed with it every frame in placement mode). */
	EPlacementInvalidReason PlacementInvalidReason = EPlacementInvalidReason::Point;

	/** One-shot latch for the no-navmesh degrade-open warning (IsPointOnNavmesh). */
	bool bWarnedNoNavData = false;

	/** One-shot latch for the missing-Blue-castle spawn-box warning (IsPointInOwnSpawnBox, TASK-261). */
	bool bWarnedMissingSpawnCastle = false;

	/**
	 *  Hand slot the active placement came from (set by PlayHandSlot just
	 *  before EnterPlacementMode), or INDEX_NONE on the M1 paths (WBP_HUD
	 *  Footman button / key-1 empty-hand fallback), which bypass the hand
	 *  entirely. The card leaves the hand only at CONFIRM (M2 ruling):
	 *  TryConfirmPlacement consumes it via ConfirmPlayFromHand; every
	 *  placement exit clears it.
	 */
	int32 PendingHandSlot = INDEX_NONE;

	// --- M5 targeting-mode state (TASK-100). Deliberately DISJOINT from the
	//     Pending* placement members above so the two sibling modes can never
	//     cross-contaminate; ExitTargetingMode resets every one of these. ---

	/** True while targeting mode is active (mutually exclusive with bInPlacementMode — each Enter* ignores while the other is live). */
	bool bInTargetingMode = false;

	/** Result of the latest targeting cursor trace: some surface answered under the cursor (the ONLY positional gate — M5 ruling 8). */
	bool bTargetingSurfaceValid = false;

	/** Surface point of the latest targeting cursor trace (the resolver TargetPoint on confirm). Z comes from the trace — never assumed 0 (M4.5 carry-in LAW). */
	FVector TargetingLocation = FVector::ZeroVector;

	/** Spell being targeted (set on EnterTargetingMode). */
	FName TargetingCardID;

	/** Cost read from DT_Cards on EnterTargetingMode — deducted only at LMB confirm (ruling 8). */
	int32 TargetingCost = 0;

	/**
	 *  Row SNAPSHOT taken on EnterTargetingMode for the confirm-time
	 *  USpellLibrary::ResolveSpell call — the placement scalar-snapshot
	 *  pattern (PendingCost/PendingCardType) generalized, because the pinned
	 *  resolver consumes the whole row. Plain value member (FCardRow holds no
	 *  hard UObject pointers — soft paths only — so no UPROPERTY needed).
	 */
	FCardRow TargetingRow;

	/**
	 *  Hand slot the active targeting came from (set by PlayHandSlot just
	 *  before EnterTargetingMode — the PendingHandSlot pattern), or
	 *  INDEX_NONE on direct hand-less entries. Consumed at CONFIRM via
	 *  ConfirmPlayFromHand; every targeting exit clears it.
	 */
	int32 TargetingHandSlot = INDEX_NONE;

	/** One-shot latch for the missing-M_SpellReticle warning (SpawnSpellReticle — "log once" per M5 ruling 8). */
	bool bWarnedNoReticleMaterial = false;

	// --- W1 unit-command (Shield Wall) stance state (TASK-274): CurrentCommand /
	//     bHasIssuedCommand are the AUTHORITATIVE latched stance the units read
	//     live (TASK-275). The one-circle HOLD pick scratch that used to live here
	//     is SUPERSEDED by the group-order pick below (TASK-344). ---

	/** The latched unit-command stance (Attack/Hold/Defend). Default Attack, but units run the legacy body until bHasIssuedCommand flips true. Play Again resets it (HandleMatchReset). Hold survives as a declared member only (WBP_HUD pins) — nothing latches it since TASK-344. */
	ESiegeUnitCommand CurrentCommand = ESiegeUnitCommand::Attack;

	/** True once the player has issued ANY command this match. Starts false (zero behavior change until the first key press, Q3 default); reset false on Play Again. */
	bool bHasIssuedCommand = false;

	// --- Group-order state (TASK-344, CONVENTIONS "Group orders — 3-zone HOLD +
	//     AMBUSH"). UnitGroups is the AUTHORITATIVE live-group store the units
	//     resolve each state tick (FindUnitGroup). The GroupPick* members are the
	//     3-stage pick scratch, kept DISJOINT from the placement + targeting
	//     scratch (the same no-cross-contamination discipline) — the three cursor
	//     modes stay mutually exclusive, so at most one is ever live. ---

	/** Every live group order. Multiple concurrent groups; re-selected units are STOLEN from older groups; emptied groups + their markers are destroyed (stage-3 steal synchronously, all-dead within ≤1 s by PruneUnitGroups). UPROPERTY so the marker TObjectPtrs stay GC-visible. */
	UPROPERTY(Transient)
	TArray<FSiegeUnitGroup> UnitGroups;

	/** Next group id handed out by the stage-3 confirm — never reused within this controller's lifetime, so a stale unit-side id can never alias a NEW group. */
	int32 NextUnitGroupId = 1;

	/**
	 *  Id of THE ONE default follow group (TASK-395), or INDEX_NONE when none
	 *  exists yet. TRANSIENT scratch, never replicated — it is a plain index into
	 *  UnitGroups' id space, re-derived lazily by EnsureDefaultFollowGroup.
	 *
	 *  ⚠️ ANTI-LEAK INVARIANT (spec item 6): this MUST be reset to INDEX_NONE
	 *  wherever its group is destroyed — PruneUnitGroups (the ≤1 s reaper, which
	 *  legitimately reaps an EMPTY follow group) and ClearAllUnitGroups (T/E
	 *  release + Play Again) both do so explicitly, and EnsureDefaultFollowGroup
	 *  re-validates the id against the live array as a second line of defence.
	 *  Without that, a stale id would either alias nothing (silent dead group) or
	 *  — since ids are never reused — leave the spawn default permanently broken.
	 */
	int32 DefaultFollowGroupId = INDEX_NONE;

	/**
	 *  Monotonic enrollment ordinal feeding ComputeFollowStationOffset, reset to
	 *  0 each time the default follow group is (re)created.
	 *
	 *  WHY NOT the raw Members index: PruneUnitGroups COMPACTS the member array,
	 *  so array indices are RECYCLED — and with Follow as the spawn default the
	 *  group churns constantly (every unit spawns into it, fights, dies), which
	 *  would make two living followers share one station the common case rather
	 *  than an edge. A never-reused ordinal keeps every live follower's angle
	 *  distinct for the group's whole lifetime at the cost of one int.
	 */
	int32 NextFollowStationIndex = 0;

	/** Drives PruneUnitGroups every second (armed once at BeginPlay; trivially cheap while no groups exist). */
	FTimerHandle UnitGroupPruneTimerHandle;

	/** Current pick stage — None = no pick live (the PlayerTick pick branch and the wheel poll are inert). */
	EGroupPickStage GroupPickStage = EGroupPickStage::None;

	/**
	 *  Command type this pick will create (Hold via R, Ambush via F, Follow via
	 *  C). Meaningful only while GroupPickStage != None. Follow is the ONE-STAGE
	 *  type: it confirms at Select and never advances, so while this reads Follow
	 *  the stage can only ever be None or Select (ConfirmGroupPickStage carries
	 *  tripwires on the other two cases).
	 */
	ESiegeGroupCommandType GroupPickType = ESiegeGroupCommandType::Hold;

	/** Radius of the ACTIVE stage circle — seeded per stage from the Group*RadiusDefault tunables, wheel-stepped by ApplyGroupPickWheel. */
	float GroupPickRadius = 0.f;

	/** Result of the latest pick cursor trace: some surface answered under the cursor (the only positional gate — mirrors bTargetingSurfaceValid). */
	bool bGroupPickSurfaceValid = false;

	/** Surface point of the latest pick cursor trace (the stage's center on confirm). Z comes from the trace, never assumed 0 (the surface-projection law). */
	FVector GroupPickLocation = FVector::ZeroVector;

	/** Units captured by the stage-1 SELECT confirm (weak — they may die mid-flow; the stage-3 confirm re-filters). */
	TArray<TWeakObjectPtr<ASummonedUnit>> GroupPickSelectedMembers;

	/** POSITION zone recorded by the stage-2 confirm (center + radius) — consumed by the stage-3 group build. */
	FVector GroupPickPositionCenter = FVector::ZeroVector;

	/** Radius half of the stage-2 record. */
	float GroupPickPositionRadius = 0.f;

	/** The ACTIVE cursor-following stage circle. Dropped in place on each stage confirm; the stage-3 circle transfers to the group as its attack marker. */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> GroupPickActiveDecal;

	/** The dropped stage-1 SELECT circle (stays visible through the flow; destroyed at the final confirm/cancel — it never becomes a marker). */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> GroupPickSelectDecal;

	/** The dropped stage-2 POSITION circle (transfers to the group as its position marker at the final confirm). */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> GroupPickPositionDecal;

	/** Hero whose melee the group pick suppressed — released on EVERY pick exit path BEFORE any early-out (the PlacementHero/TargetingHero pattern; its OWN record so a defensive ExitPlacement/ExitTargeting call can never strand a live pick suppression — the QA TASK-003 warning-2 law). */
	UPROPERTY(Transient)
	TObjectPtr<AHeroCharacter> GroupPickHero;

	/** Hero whose melee we suppressed — un-suppressed on EVERY exit path (QA TASK-003 warning 2). */
	UPROPERTY(Transient)
	TObjectPtr<AHeroCharacter> PlacementHero;

	/** Hero whose melee TARGETING mode suppressed — released on every targeting exit path (the PlacementHero pattern; kept separate so a defensive ExitPlacementMode call can never strand a live targeting suppression). */
	UPROPERTY(Transient)
	TObjectPtr<AHeroCharacter> TargetingHero;

	/** Spell reticle decal actor (transient; decals carry no collision, so it can never block the cursor trace). Null when M_SpellReticle is missing — targeting works without it. */
	UPROPERTY(Transient)
	TObjectPtr<ADecalActor> SpellReticleActor;

	/** Ghost preview actor (transient, collision off — never blocks the cursor trace). */
	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> GhostActor;

	/** Dynamic instance of M_Ghost driving the "GhostColor" parameter (TASK-012 contract). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMID;

	/** HUD widget instance (created at BeginPlay when WBP_HUD exists). */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	/**
	 *  M8 HUD PS-retry (TASK-356 doc §3.2): creates the HUD once the owning
	 *  ASiegePlayerState is resolvable — on a CLIENT the PS proxy may arrive a
	 *  few frames after PC BeginPlay, and a HUD constructed before it would seed
	 *  from nothing. Bounded next-tick retries (HUDInitAttempts vs the cpp cap);
	 *  exhaustion logs and creates the HUD anyway (its own binds are null-safe).
	 *  Standalone: the PS exists on the first check ⇒ the HUD is created
	 *  synchronously inside BeginPlay exactly as before (doc §10).
	 */
	void TryInitHUD();

	/** Retry counter for TryInitHUD (client PS-proxy arrival wait). */
	int32 HUDInitAttempts = 0;

	/** Victory screen instance (created by HandleMatchEnd). */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> VictoryWidget;
};
