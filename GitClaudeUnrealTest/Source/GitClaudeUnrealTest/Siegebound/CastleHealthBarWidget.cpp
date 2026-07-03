// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CastleHealthBarWidget.h"

#include "GitClaudeUnrealTest.h"
#include "Siegebound/Castle.h"

void UCastleHealthBarWidget::InitForCastle(ACastle* Castle)
{
	if (!Castle)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("UCastleHealthBarWidget::InitForCastle: null castle — bar left unbound."));
		return;
	}

	// Re-targeting: drop the previous castle's binding so this bar never
	// receives two update streams (dynamic delegates would happily keep both).
	if (ObservedCastle && ObservedCastle != Castle)
	{
		ObservedCastle->OnCastleHPChanged.RemoveDynamic(this, &UCastleHealthBarWidget::HandleCastleHPChanged);
	}
	ObservedCastle = Castle;

	// SEED FIRST (qa/TASK-005-report.md major 2): push the current values now,
	// so the bar is correct even if no broadcast ever arrives after binding...
	OnHPChanged(Castle->GetCurrentHP(), Castle->GetMaxHP());

	// ...THEN bind. AddUniqueDynamic: a repeated InitForCastle on the same
	// castle can never double-bind (a double-bound bar would double-fire
	// OnHPChanged per hit).
	Castle->OnCastleHPChanged.AddUniqueDynamic(this, &UCastleHealthBarWidget::HandleCastleHPChanged);
}

void UCastleHealthBarWidget::HandleCastleHPChanged(float CurrentHP, float MaxHP)
{
	OnHPChanged(CurrentHP, MaxHP);
}
