// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CombatantHealthBarWidget.h"

#include "GitClaudeUnrealTest.h"

void UCombatantHealthBarWidget::InitForCombatant(TScriptInterface<IHealthBarProvider> Provider)
{
	IHealthBarProvider* ProviderPtr = Provider.GetInterface();
	if (!ProviderPtr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCombatantHealthBarWidget::InitForCombatant: null/invalid provider — bar left unbound."));
		return;
	}

	// Re-targeting: drop the previous provider's binding so this bar never receives two
	// update streams (dynamic delegates would happily keep both). Mirrors InitForCastle.
	if (IHealthBarProvider* OldProvider = ObservedProvider.GetInterface())
	{
		if (OldProvider != ProviderPtr)
		{
			OldProvider->GetHPChangedDelegate().RemoveDynamic(this, &UCombatantHealthBarWidget::HandleHPChanged);
		}
	}
	ObservedProvider = Provider;

	// SEED FIRST (qa/TASK-005-report.md major 2): push the current values now, so the bar
	// is correct even if no broadcast ever arrives after binding...
	OnHPChanged(ProviderPtr->GetHealthCurrent(), ProviderPtr->GetHealthMax());

	// ...THEN bind. AddUniqueDynamic: a repeated InitForCombatant on the same provider can
	// never double-bind (a double-bound bar would double-fire OnHPChanged per HP change).
	ProviderPtr->GetHPChangedDelegate().AddUniqueDynamic(this, &UCombatantHealthBarWidget::HandleHPChanged);
}

void UCombatantHealthBarWidget::HandleHPChanged(float CurrentHP, float MaxHP)
{
	OnHPChanged(CurrentHP, MaxHP);
}
