// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CombatantHealthBarComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Actor.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/CombatantHealthBarWidget.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/TeamId.h"

namespace
{
	/** DrawSize of the overhead bar in screen pixels (compact — units read at ~150 px, CONVENTIONS). */
	const FVector2D CombatantHealthBarDrawSize(90.f, 12.f);
}

UCombatantHealthBarComponent::UCombatantHealthBarComponent()
{
	// Screen space so the bar reads at any camera angle/distance — the ACastle::HPBarWidget
	// precedent. The WidgetComponent keeps its own render tick; only gameplay never ticks
	// (there is NO poll timer — updates arrive via the owner's OnHPChanged delegate).
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(CombatantHealthBarDrawSize);

	// Tick every frame (TASK-130 render-fix insurance): a screen-space widget component is
	// (re)added to FWorldWidgetScreenLayer and re-projected to the actor's screen position in
	// UpdateWidgetOnScreen(), which runs at the END of TickComponent — an Automatic/disabled tick
	// can drop the widget off the screen layer. Enabled keeps it hosted + following the actor.
	SetTickMode(ETickMode::Enabled);

	// UI-only: never collides, never blocks traces (placement cursor trace, unit acquisition,
	// hero melee all query ECC_Pawn — this must be invisible to them).
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	// Do NOT start hidden — castle parity: ACastle::HPBarWidget is visible from construction
	// and renders correctly. The initial shown state is decided in BeginPlay from bShowHealthBar;
	// the owner drives hide-on-death / show-on-respawn (HideBar / ShowBarIfEnabled).

	// Default widget class per CONVENTIONS names block; null-safe soft ref (asset built in
	// TASK-131). The _C suffix is the runtime generated-class path (the ACastle::HPBarWidgetClass
	// precedent). A BP may override or clear this.
	HealthBarWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_CombatantHealthBar.WBP_CombatantHealthBar_C")));
}

void UCombatantHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Lift the bar above the actor root; applied here (not in the constructor) so a BP
	// override of BarHeightZ — which lands before BeginPlay — is honored.
	SetRelativeLocation(FVector(0.f, 0.f, BarHeightZ));

	// WBP_CombatantHealthBar is built in TASK-131 and may not exist yet — a missing/unset
	// class is a SILENT no-bar (LoadSynchronous returns nullptr for unset paths and absent
	// assets alike). Log ONCE across the run so 60+ actors don't spam.
	UClass* LoadedWidgetClass = HealthBarWidgetClass.IsNull() ? nullptr : HealthBarWidgetClass.LoadSynchronous();
	if (!LoadedWidgetClass)
	{
		static bool bLoggedMissingCombatantHealthBarClass = false;
		if (!bLoggedMissingCombatantHealthBarClass)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("UCombatantHealthBarComponent: HealthBarWidgetClass ('%s') unresolved — overhead health bars are disabled (WBP_CombatantHealthBar not built yet?). Logged once."),
				*HealthBarWidgetClass.ToString());
			bLoggedMissingCombatantHealthBarClass = true;
		}
		// No widget class => nothing to bind or show.
		return;
	}

	// Post-BeginPlay SetWidgetClass creates the user widget instance synchronously (the
	// component has begun play) — the ACastle::InitHPBarWidget pattern, which renders.
	SetWidgetClass(LoadedWidgetClass);
	BarWidget = Cast<UCombatantHealthBarWidget>(GetWidget());

	AActor* OwnerActor = GetOwner();
	if (BarWidget)
	{
		// SEED-THEN-BIND via the owner's push delegate — mirrors ACastle::InitHPBarWidget →
		// UCastleHealthBarWidget::InitForCastle. A non-provider owner (defensive) is skipped.
		if (IHealthBarProvider* Provider = Cast<IHealthBarProvider>(OwnerActor))
		{
			TScriptInterface<IHealthBarProvider> ProviderInterface;
			ProviderInterface.SetObject(OwnerActor);
			ProviderInterface.SetInterface(Provider);
			BarWidget->InitForCombatant(ProviderInterface);

			// TASK-130 render-fix: the COMPONENT ALSO binds (additive to the widget's own bind), so
			// every HP change drives the CURRENT on-screen GetWidget() and forces a redraw — proof
			// against a widget-instance mismatch AND a World-space stale render (see HandleOwnerHPChanged).
			Provider->GetHPChangedDelegate().AddUniqueDynamic(this, &UCombatantHealthBarComponent::HandleOwnerHPChanged);
		}

		// One-time team tint (CONVENTIONS: tint is DATA, never hardcoded in logic). Read the
		// owner's team through the SEPARATE ITeamAgent interface: RED enemy, BLUE friendly.
		const ITeamAgent* TeamAgent = Cast<ITeamAgent>(OwnerActor);
		const FLinearColor BarColor = (TeamAgent && TeamAgent->GetTeamId() == ETeamId::Red) ? RedBarColor : BlueBarColor;
		BarWidget->SetTeamColor(BarColor.R, BarColor.G, BarColor.B);
	}

	// Always-visible while alive (reversed hide-at-full law); an opted-out owner stays hidden.
	// The owner re-shows on respawn / hides on death (ShowBarIfEnabled / HideBar).
	ShowBarIfEnabled();
}

void UCombatantHealthBarComponent::HandleOwnerHPChanged(float CurrentHP, float MaxHP)
{
	// Drive the CURRENT on-screen widget instance directly — identity-proof: if the widget's own
	// seed-then-bind is updating a stale GetWidget() from an earlier frame, this always targets the
	// live one that FWorldWidgetScreenLayer added via GetUserWidgetObject()->TakeWidget().
	UUserWidget* CurrentWidget = GetWidget();
	if (UCombatantHealthBarWidget* LiveBar = Cast<UCombatantHealthBarWidget>(CurrentWidget))
	{
		LiveBar->OnHPChanged(CurrentHP, MaxHP);
	}

	// Force a repaint. For Screen space the inner widget is LIVE Slate (this is a no-op), but for a
	// World-space render-target host SetPercent alone will NOT re-render — RequestRedraw is required.
	RequestRedraw();
}

void UCombatantHealthBarComponent::ShowBarIfEnabled()
{
	SetVisibility(bShowHealthBar, /*bPropagateToChildren=*/true);
}

void UCombatantHealthBarComponent::HideBar()
{
	SetVisibility(false, /*bPropagateToChildren=*/true);
}
