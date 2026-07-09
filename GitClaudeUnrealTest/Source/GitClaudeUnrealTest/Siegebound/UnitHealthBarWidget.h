// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UnitHealthBarWidget.generated.h"

/**
 *  C++ base for /Game/UI/WBP_UnitHealthBar (M5.5, TASK-111 duplicates the
 *  WBP_CastleHealthBar donor and reparents it to this class) — the overhead
 *  health bar for units, buildings, and the hero.
 *
 *  UHealthBarComponent (the poll-driven UWidgetComponent added to every combat
 *  actor's base class) drives BOTH events: it pushes SetTeamColor ONCE at init
 *  from the owner's team, and calls OnHPChanged each poll while the bar is shown.
 *
 *  Both are BlueprintImplementableEvents with FLOAT PARAMS ONLY (MCP tooling
 *  cannot author enum/struct BP params — CONVENTIONS widget rule). OnHPChanged
 *  is the identical contract to UCastleHealthBarWidget::OnHPChanged, which is why
 *  WBP_CastleHealthBar is the fastest donor to duplicate.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UUnitHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Implemented by WBP_UnitHealthBar (TASK-111): update the bar fill —
	 *  ProgressBar SetPercent(CurrentHP / MaxHP), guarding MaxHP > 0 before
	 *  dividing. Float params only (MCP cannot author enum BP params).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void OnHPChanged(float CurrentHP, float MaxHP);

	/**
	 *  Implemented by WBP_UnitHealthBar (TASK-111): tint the ProgressBar FILL
	 *  brush from the pushed linear RGB (blue friendly / red enemy). Pushed once
	 *  at init by UHealthBarComponent; fixed regardless of HP. Float params only —
	 *  an FLinearColor BP param is not MCP-authorable (CONVENTIONS widget rule).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void SetTeamColor(float R, float G, float B);
};
