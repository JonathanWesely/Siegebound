// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/HealthBarComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "TimerManager.h"
#include "Siegebound/HealthBarTarget.h"
#include "Siegebound/TeamId.h"
#include "Siegebound/UnitHealthBarWidget.h"

namespace
{
	/** DrawSize of the overhead bar in screen pixels (compact — units read at ~150 px, CONVENTIONS). */
	const FVector2D HealthBarDrawSize(90.f, 12.f);

	/**
	 *  HP within this of MaxHP counts as "full" — the bar hides at/above it. Guards
	 *  a sliver of bar from float drift at exactly full HP (hide-at-full law).
	 */
	constexpr float HealthBarFullEpsilon = 0.01f;

	/** Timer floor so a mis-set PollInterval <= 0 never CLEARS the timer instead of looping (qa/TASK-021 WARN-1 lesson). */
	constexpr float MinPollInterval = 0.02f;
}

UHealthBarComponent::UHealthBarComponent()
{
	// Screen space so the bar reads at any camera angle/distance — the
	// ACastle::HPBarWidget precedent. The WidgetComponent keeps its own render
	// tick (it must follow the actor on screen); only gameplay never ticks.
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(HealthBarDrawSize);

	// UI-only: never collides, never blocks traces (placement cursor trace,
	// unit acquisition, hero melee all query ECC_Pawn — this must be invisible to them).
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	// Hidden until the first poll decides to show it (hide-at-full default — no flash at full HP).
	SetVisibility(false);

	// Default widget class per CONVENTIONS names block; null-safe soft ref (asset
	// built in TASK-111). The _C suffix is the runtime generated-class path (the
	// ACastle::HPBarWidgetClass precedent). A BP may override or clear this.
	HealthBarWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_UnitHealthBar.WBP_UnitHealthBar_C")));
}

void UHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Lift the bar above the actor root; applied here (not in the constructor) so a
	// BP override of BarHeightZ — which lands before BeginPlay — is honored.
	SetRelativeLocation(FVector(0.f, 0.f, BarHeightZ));

	// WBP_UnitHealthBar is built in TASK-111 and may not exist yet — a missing/unset
	// class is a SILENT no-bar (LoadSynchronous returns nullptr for unset paths and
	// absent assets alike). Log ONCE across the run so 60+ actors don't spam.
	UClass* LoadedWidgetClass = HealthBarWidgetClass.IsNull() ? nullptr : HealthBarWidgetClass.LoadSynchronous();
	if (!LoadedWidgetClass)
	{
		static bool bLoggedMissingHealthBarClass = false;
		if (!bLoggedMissingHealthBarClass)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("UHealthBarComponent: HealthBarWidgetClass ('%s') unresolved — overhead health bars are disabled (WBP_UnitHealthBar not built yet?). Logged once."),
				*HealthBarWidgetClass.ToString());
			bLoggedMissingHealthBarClass = true;
		}
		// No widget class => nothing to poll or show; never arm the timer.
		return;
	}

	// Post-BeginPlay SetWidgetClass triggers InitWidget, creating the user widget
	// instance synchronously (components have begun play — Super::BeginPlay ran above).
	SetWidgetClass(LoadedWidgetClass);
	BarWidget = Cast<UUnitHealthBarWidget>(GetWidget());

	// One-time team tint (CONVENTIONS: tint is DATA, never hardcoded in logic). Read
	// the owner's team through the SEPARATE ITeamAgent interface and push the linear
	// RGB once. A mis-authored widget of another class leaves BarWidget null and is
	// skipped, not a crash.
	if (BarWidget)
	{
		const ITeamAgent* TeamAgent = Cast<ITeamAgent>(GetOwner());
		const FLinearColor BarColor = (TeamAgent && TeamAgent->GetTeamId() == ETeamId::Red) ? RedBarColor : BlueBarColor;
		BarWidget->SetTeamColor(BarColor.R, BarColor.G, BarColor.B);
	}

	// Poll the HP getters on a repeating timer (units/buildings/hero carry no
	// HP-changed delegate). First poll immediately so a mid-damage spawn shows at once.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PollTimerHandle, this, &UHealthBarComponent::PollHealth,
			FMath::Max(PollInterval, MinPollInterval), /*bLoop=*/true, /*FirstDelay=*/0.f);
	}
}

void UHealthBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UHealthBarComponent::PollHealth()
{
	// The owner always outlives this component and the timer is cleared in EndPlay,
	// so GetOwner() is valid here; a non-target owner (defensive) => never show.
	const IHealthBarTarget* Target = Cast<IHealthBarTarget>(GetOwner());
	if (!Target)
	{
		return;
	}

	const bool bAlive = Target->IsHealthBarActorAlive();
	const float Current = Target->GetHealthCurrent();
	const float Max = Target->GetHealthMax();

	// Hide-at-full (CONVENTIONS behavior law): the bar shows ONLY while opted-in,
	// alive, and actually damaged. A statless actor (Max <= 0) satisfies
	// Current(0) >= Max(0) − eps, so it also stays hidden.
	const bool bShouldShow = bShowHealthBar && bAlive && (Current < Max - HealthBarFullEpsilon);

	if (!bShouldShow)
	{
		// Screen-space widget components do NOT follow the owner's hidden-in-game
		// state (the hero HIDES rather than destroys on death), so hide explicitly.
		// Toggle off our own latch so we never redundantly dirty the render state.
		if (bBarShown)
		{
			SetVisibility(false, /*bPropagateToChildren=*/true);
			bBarShown = false;
		}
		return;
	}

	if (!bBarShown)
	{
		SetVisibility(true, /*bPropagateToChildren=*/true);
		bBarShown = true;
	}

	// Drive the fill each poll while shown (float-only BIE; the WBP guards Max > 0).
	if (BarWidget)
	{
		BarWidget->OnHPChanged(Current, Max);
	}
}
