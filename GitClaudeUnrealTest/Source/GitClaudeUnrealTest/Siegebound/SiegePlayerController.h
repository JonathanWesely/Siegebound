// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/CardRow.h" // ECardType (the PendingCardType member; FCardRow comes along)
#include "Siegebound/TeamId.h"
#include "SiegePlayerController.generated.h"

class ACastle;
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
 *    NavProjectionExtent (refuses castle-roof and plinth-top hits — the M1
 *    carry-over) AND outside every castle's plinth keep-out box
 *    (CastlePlinthClearance). Building cards additionally require
 *    >= BuildingClearance from the nearest other ABuilding (§3.5; castles
 *    are NOT buildings for that rule). Violations show the red ghost.
 *  - Confirm = LMB while in mode: re-gate the miner cap, resolve the card's
 *    BP class by CardType (Unit/Economy →
 *    /Game/Blueprints/Units/BP_Unit_<CardID>; Building →
 *    /Game/Blueprints/Buildings/BP_Building_<CardID>; missing BP = refuse +
 *    exit, NO gold spent), then SpendGold(Cost), InitUnit/InitBuilding(Team,
 *    CardID) + FinishSpawning at the clicked point, and
 *    ConfirmPlayFromHand(PendingHandSlot). An invalid click refuses, spends
 *    nothing, and STAYS in mode.
 *  - Cancel = IA_CancelPlace (RMB/Esc): exit with no cost.
 *  - HandleMatchEnd(Winner): exits placement mode, shows
 *    /Game/UI/WBP_VictoryScreen (soft class, null-safe) and switches to
 *    UI-only input. HandleMatchReset() restores play (TASK-006 PlayAgain)
 *    and defensively exits placement mode FIRST (qa/TASK-023-report.md WARN).
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

	/** Fired on every player-facing card-play refusal (gold, invalid spot, missing data). HUD may bind (TASK-011). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Cards")
	FOnCardPlayRefused OnCardPlayRefused;

	/** Fired on EVERY refused play or discard with the §3.0-style reason string. The hand HUD (TASK-033) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Cards")
	FOnCardRefused OnCardRefused;

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
	 *  Masons is refused with NO gold moved (§3.0/§3.10 full refund). Spell → M5
	 *  (GDD §3.11). Post-match and mid-placement presses are ignored (no broadcast),
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

	/** True while the placement ghost owns the cursor/LMB. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Cards")
	bool IsInPlacementMode() const { return bInPlacementMode; }

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

	/** Placement is valid only at X <= this (Blue half; centerline X=0 per CONVENTIONS). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	float PlacementMaxX = 0.f;

	/**
	 *  Minimum 2D distance from the nearest other ABuilding for a
	 *  Building-card placement — closer shows the red ghost and refuses the
	 *  click with no gold spent. Castles are NOT buildings for this rule
	 *  (class-disjoint; the plinth keep-out covers them). Mechanic rule, not a
	 *  CSV column (CONVENTIONS registry). // GDD §3.5 — buildings require 200
	 *  units of clearance from any other building
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement", meta = (ClampMin = "0"))
	float BuildingClearance = 200.f;

	/**
	 *  Box half-extent for the placement navmesh projection (GDD §3.5 via
	 *  TASK-030): a point is placeable only if it projects onto the navmesh
	 *  within this extent — refusing castle-roof and plinth-top cursor hits
	 *  (the M1 carry-over). The vertical half-extent MUST stay well below the
	 *  ~90-unit SM_Castle plinth height, or a plinth-top point could project
	 *  down to the ground navmesh and read as valid.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FVector NavProjectionExtent = FVector(50.f, 50.f, 50.f);

	/**
	 *  Castle plinth keep-out, applied as a 2D box half-extent around every
	 *  ACastle: points inside are refused for ALL cards. M1 carry-over —
	 *  SM_Castle's plinth collision spans ~814x820 units (~90 high, half-extent
	 *  ~410); the box guarantees plinth points always read invalid even where
	 *  the navmesh leaves walkable islands on the plinth rim that
	 *  ProjectPointToNavigation alone would accept. 420 = half-extent + margin
	 *  (the margin band is already navmesh-eroded by agent radius).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement", meta = (ClampMin = "0"))
	float CastlePlinthClearance = 420.f;

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

	/** Why the latest traced cursor point is invalid — drives the confirm-refusal message (the §3.5 clearance rule gets its own reason). */
	enum class EPlacementInvalidReason : uint8
	{
		None,      // point is valid
		Point,     // no ground hit / enemy half / off the navmesh / castle plinth
		Clearance  // building within BuildingClearance of another building
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

	/** Per-frame: cursor-to-ground trace, validity v2 (ground, own half, navmesh projection, plinth keep-out, building clearance), ghost position + color. */
	void UpdatePlacementGhost();

	/** Spawns the ghost actor (movable, collision off, per-card SM_<CardID> or the fallback sphere + M_Ghost MID) — every asset null-safe. */
	void SpawnPlacementGhost();

	/** Destroys the ghost actor and drops the dynamic material instance. */
	void DestroyPlacementGhost();

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

	/** True when Point lies inside any ACastle's plinth keep-out box (CastlePlinthClearance 2D half-extents) — refused for all cards, castle HP irrelevant. */
	bool IsPointInsideCastlePlinth(const FVector& Point) const;

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

	/**
	 *  Hand slot the active placement came from (set by PlayHandSlot just
	 *  before EnterPlacementMode), or INDEX_NONE on the M1 paths (WBP_HUD
	 *  Footman button / key-1 empty-hand fallback), which bypass the hand
	 *  entirely. The card leaves the hand only at CONFIRM (M2 ruling):
	 *  TryConfirmPlacement consumes it via ConfirmPlayFromHand; every
	 *  placement exit clears it.
	 */
	int32 PendingHandSlot = INDEX_NONE;

	/** Hero whose melee we suppressed — un-suppressed on EVERY exit path (QA TASK-003 warning 2). */
	UPROPERTY(Transient)
	TObjectPtr<AHeroCharacter> PlacementHero;

	/** Ghost preview actor (transient, collision off — never blocks the cursor trace). */
	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> GhostActor;

	/** Dynamic instance of M_Ghost driving the "GhostColor" parameter (TASK-012 contract). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMID;

	/** HUD widget instance (created at BeginPlay when WBP_HUD exists). */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	/** Victory screen instance (created by HandleMatchEnd). */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> VictoryWidget;
};
