// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "SiegePlayerController.generated.h"

class AHeroCharacter;
class AStaticMeshActor;
class UDataTable;
class UInputAction;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UUserWidget;
struct FCardRow;

/**
 *  Broadcast whenever a card play is refused for a player-facing reason
 *  (not enough gold, invalid placement point, card data unavailable).
 *  The HUD (WBP_HUD, TASK-011) may bind here to flash a message; binding
 *  is optional — every refusal is also logged.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardPlayRefused, FName, CardID, FText, Reason);

/**
 *  Siegebound player controller — card play + unit placement mode (GDD §3.5, M1 subset).
 *
 *  M1 scope: ONE always-available Footman card, no hand/deck (TODO(M2)).
 *
 *  - BeginPlay creates and adds /Game/UI/WBP_HUD (soft class, null-safe —
 *    the widget is built in TASK-011; missing = log once and continue).
 *  - EnterPlacementMode(CardID): reads Cost from the /Game/Data/DT_Cards row
 *    (never hardcoded, GDD §3.0), refuses if the player can't afford it, then
 *    enters placement mode: mouse cursor shown, hero melee suppressed
 *    (AHeroCharacter::SetMeleeSuppressed — TASK-003), and a ghost preview
 *    (SM_Footman + dynamic instance of M_Ghost, "GhostColor" green/red)
 *    follows a per-frame cursor-to-ground trace.
 *  - Valid placement = ground hit AND X <= 0 (Blue half; centerline X=0 per
 *    CONVENTIONS world-axes contract).
 *  - Confirm = LMB while in mode: SpendGold(Cost), deferred-spawn the card's
 *    unit class (/Game/Blueprints/Units/BP_Unit_<CardID>, ASummonedUnit
 *    fallback) with InitUnit(Team, CardID), exit mode. An invalid click
 *    refuses, spends nothing, and STAYS in mode.
 *  - Cancel = IA_CancelPlace (RMB/Esc): exit with no cost.
 *  - HandleMatchEnd(Winner): exits placement mode, shows
 *    /Game/UI/WBP_VictoryScreen (soft class, null-safe) and switches to
 *    UI-only input. HandleMatchReset() restores play (TASK-006 PlayAgain).
 *
 *  QA-BINDING (qa/TASK-003-report.md warning 2): AHeroCharacter::ResetHero
 *  deliberately preserves bMeleeSuppressed, so this controller calls
 *  SetMeleeSuppressed(false) on EVERY placement-mode exit path — confirm,
 *  cancel, match end, hero death (OnHeroDied), unpossession, and EndPlay.
 *  ExitPlacementMode() releases the suppression before any early-out.
 *
 *  All content references (widgets, data table, ghost mesh/material, input
 *  actions) are soft and resolved null-safe at runtime — missing assets log
 *  a warning and never block the placement logic itself.
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

	/**
	 *  Starts placement mode for the given card (HUD card button or the
	 *  IA_Card1 key). Reads the card's Cost from /Game/Data/DT_Cards (GDD
	 *  §3.0 — never hardcoded) and refuses (log + OnCardPlayRefused) when the
	 *  data is missing, the player can't afford it, the match has ended, the
	 *  hero is dead, or placement mode is already active.
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
	 *  (TASK-006): removes the victory screen if still up (idempotent with the
	 *  widget's own RemoveFromParent), clears the match-ended latch, and
	 *  restores game-only input with the cursor hidden.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Match")
	void HandleMatchReset();

	/** True while the placement ghost owns the cursor/LMB. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Cards")
	bool IsInPlacementMode() const { return bInPlacementMode; }

	/** True from HandleMatchEnd until HandleMatchReset. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Match")
	bool HasMatchEnded() const { return bMatchEnded; }

protected:

	/** Creates and adds the HUD widget (soft class, null-safe — TASK-011 builds it). */
	virtual void BeginPlay() override;

	/** Defensive placement-mode exit on teardown (releases melee suppression, destroys the ghost). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Binds IA_Card1 / IA_CancelPlace on the enhanced input component (null-safe, soft-resolved by path). */
	virtual void SetupInputComponent() override;

	/** Placement mode per-frame work: cancel poll, cursor-to-ground trace, ghost update, confirm poll. */
	virtual void PlayerTick(float DeltaTime) override;

	/** Subscribes to the hero's OnHeroDied so death always exits placement mode (QA TASK-003 warning 2). */
	virtual void OnPossess(APawn* InPawn) override;

	/** Unsubscribes from the hero and defensively exits placement mode. */
	virtual void OnUnPossess() override;

	/** IA_Card1 pressed: play the M1 card slot (Card1CardID = Footman). */
	void OnCard1Pressed();

	/** IA_CancelPlace pressed (RMB/Esc): leave placement mode at no cost. */
	void OnCancelPlacePressed();

	/** Hero died (FOnHeroDied): exit placement mode so melee suppression is never left behind. */
	UFUNCTION()
	void HandleHeroDied(AHeroCharacter* DeadHero);

protected:

	/** Card played by the IA_Card1 slot. M1's single always-available card (GDD §3.5). TODO(M2): hand/deck. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	FName Card1CardID = FName(TEXT("Footman"));

	/** Card stat table (GDD §3.0). Imported in TASK-008 — resolved null-safe at play time. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/** HUD widget class, /Game/UI/WBP_HUD (TASK-011). Missing = log once, continue. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|UI")
	TSoftClassPtr<UUserWidget> HUDWidgetClass;

	/** End screen widget class, /Game/UI/WBP_VictoryScreen (TASK-011). Missing = log, match still ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|UI")
	TSoftClassPtr<UUserWidget> VictoryScreenClass;

	/** Ghost preview mesh, /Game/Meshes/SM_Footman (TASK-014). Missing = invisible ghost, placement still works. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	TSoftObjectPtr<UStaticMesh> GhostMeshAsset;

	/** Ghost material, /Game/Materials/M_Ghost (TASK-012; vector param "GhostColor"). Missing = default material. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	TSoftObjectPtr<UMaterialInterface> GhostMaterialAsset;

	/** IA_Card1 slot. Left unset, it soft-resolves from Card1ActionAsset (no BP controller exists in M1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Card1Action;

	/** IA_CancelPlace slot. Left unset, it soft-resolves from CancelPlaceActionAsset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CancelPlaceAction;

	/** Soft path for IA_Card1 (/Game/Input/Actions/IA_Card1, created in TASK-009). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> Card1ActionAsset;

	/** Soft path for IA_CancelPlace (/Game/Input/Actions/IA_CancelPlace, created in TASK-009). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CancelPlaceActionAsset;

	/** Placement is valid only at X <= this (Blue half; centerline X=0 per CONVENTIONS). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	float PlacementMaxX = 0.f;

	/** Ghost tint for a valid point (M_Ghost "GhostColor", TASK-012). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FLinearColor ValidGhostColor = FLinearColor(0.f, 1.f, 0.f);

	/** Ghost tint for an invalid point. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	FLinearColor InvalidGhostColor = FLinearColor(1.f, 0.f, 0.f);

	/** Yaw applied to the ghost so the raw SM_Footman faces +X (TASK-014 handoff: mesh needs -90°). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
	float GhostYawOffset = -90.f;

private:

	/** Confirm click (LMB in mode): validate, SpendGold, deferred-spawn + InitUnit, exit. Invalid = refuse, stay. */
	void TryConfirmPlacement();

	/** Per-frame: cursor-to-ground trace, validity (ground AND X <= PlacementMaxX), ghost position + color. */
	void UpdatePlacementGhost();

	/** Spawns the ghost actor (movable, collision off, SM_Footman + M_Ghost MID) — every asset null-safe. */
	void SpawnPlacementGhost();

	/** Destroys the ghost actor and drops the dynamic material instance. */
	void DestroyPlacementGhost();

	/** Cursor-to-world trace on the Visibility channel. True on a blocking hit ("ground hit"). */
	bool TraceCursorToGround(FHitResult& OutHit) const;

	/** Finds the card's DT_Cards row (soft load, null-safe). On failure returns nullptr and fills OutError. */
	const FCardRow* ResolveCardRow(FName CardID, FString& OutError) const;

	/**
	 *  Resolves the unit class for a card: /Game/Blueprints/Units/BP_Unit_<CardID>
	 *  (CONVENTIONS blueprint-subclass pattern; Footman -> BP_Unit_Footman, TASK-010).
	 *  Missing/incompatible class falls back to ASummonedUnit with a log —
	 *  the fallback spawns logic-complete but meshless (VisualMesh unset in C++).
	 */
	UClass* ResolveUnitClass(FName CardID) const;

	/** Logs and broadcasts a player-facing refusal (OnCardPlayRefused). */
	void RefuseCardPlay(FName CardID, const FText& Reason);

	/** Hard slot if assigned, else LoadSynchronous of the soft path; null (with one log) if neither resolves. */
	UInputAction* ResolveInputAction(const TObjectPtr<UInputAction>& HardSlot, const TSoftObjectPtr<UInputAction>& SoftAsset, const TCHAR* ActionName) const;

	/** Applies the placement-mode input state (cursor + Game&UI) or restores game-only input. */
	void ApplyPlacementInputState(bool bEnteringPlacement);

	/** True while placement mode is active. */
	bool bInPlacementMode = false;

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
