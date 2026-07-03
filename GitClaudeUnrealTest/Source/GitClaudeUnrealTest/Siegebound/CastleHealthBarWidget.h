// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CastleHealthBarWidget.generated.h"

class ACastle;

/**
 *  C++ base for /Game/UI/WBP_CastleHealthBar (TASK-019 reparents the UI_LifeBar
 *  donor duplicate to this class) — the overhead castle HP bar (playtest R1
 *  finding 2, TASK-018).
 *
 *  Contract:
 *  - ACastle::BeginPlay calls InitForCastle(this) on the widget instance living
 *    in its HPBarWidget component.
 *  - InitForCastle SEEDS first — OnHPChanged fires immediately with the castle's
 *    current values — THEN binds FOnCastleHPChanged (seed-then-bind,
 *    qa/TASK-005-report.md major 2: a bind-only consumer created at a value that
 *    never changes again would stay stale forever).
 *  - OnHPChanged is a BlueprintImplementableEvent with float params only (MCP
 *    cannot author enum BP params — CONVENTIONS widget rules). WBP_CastleHealthBar
 *    implements it as ProgressBar SetPercent(CurrentHP / MaxHP), guarding MaxHP > 0.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UCastleHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Points this bar at a castle: pushes the castle's current HP/MaxHP through
	 *  OnHPChanged immediately (seed), THEN binds OnCastleHPChanged (bind).
	 *  Null castle = warn + no-op. Safe to call repeatedly: same castle never
	 *  double-binds (AddUniqueDynamic); a different castle replaces the old
	 *  binding so the bar never receives two update streams.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")
	void InitForCastle(ACastle* Castle);

	/**
	 *  Implemented by WBP_CastleHealthBar (TASK-019): update the bar visuals —
	 *  ProgressBar SetPercent(CurrentHP / MaxHP), guard MaxHP > 0 before dividing.
	 *  Float params only: MCP tooling cannot author enum BP params (CONVENTIONS).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void OnHPChanged(float CurrentHP, float MaxHP);

protected:

	/** FOnCastleHPChanged handler — forwards to the BP-implemented OnHPChanged. */
	UFUNCTION()
	void HandleCastleHPChanged(float CurrentHP, float MaxHP);

	/** Castle this bar observes. Set by InitForCastle; used to unbind when re-targeted. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Siegebound|UI")
	TObjectPtr<ACastle> ObservedCastle;
};
