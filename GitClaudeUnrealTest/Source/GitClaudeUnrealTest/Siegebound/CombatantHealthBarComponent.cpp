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
	/**
	 *  DrawSize of the overhead bar in screen pixels (compact — units read at ~150 px, CONVENTIONS).
	 *  Grew 12 -> 22 at TASK-362 for the boost row: the widget root became a VerticalBox "BarStack"
	 *  holding BoostOutline(Border) ▸ BoostBar at Fill 1.0 and the EXISTING Bar at Fill 2.0, i.e.
	 *  ~8 px of boost row (a ~5 px bar inside a 1.5 px frame) above the health bar at its ORIGINAL
	 *  ~12 px height. The width is unchanged.
	 */
	const FVector2D CombatantHealthBarDrawSize(90.f, 22.f);
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
		// Hoisted out of the `if` below (TASK-362) because the boost row reads it too — a
		// non-provider owner (defensive) stays null and simply gets the zero-boost seed.
		IHealthBarProvider* Provider = Cast<IHealthBarProvider>(OwnerActor);

		// SEED-THEN-BIND via the owner's push delegate — mirrors ACastle::InitHPBarWidget →
		// UCastleHealthBarWidget::InitForCastle. A non-provider owner (defensive) is skipped.
		if (Provider)
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

		// --- Boost row (TASK-362): SEED UNCONDITIONALLY, THEN bind. ---
		// UNCONDITIONALLY is the whole point and is NOT redundant: this push is what drives a
		// non-boostable owner's row (buildings, the hero — IHealthBarProvider's DEFAULTED
		// GetDamageBoostPercent() returns 0) to RowOpacity 0, instead of leaving it at whatever
		// design-time state WBP_CombatantHealthBar was authored with. It also covers a boosted
		// actor whose bar is (re)created after the boost was granted. Bind-only would go stale
		// forever — qa/TASK-005 major 2, the same law the HP seed above obeys.
		PushDamageBoost(Provider ? Provider->GetDamageBoostPercent() : 0.f);

		// ...THEN bind, and ONLY if the owner actually has a boost delegate. The accessor returns a
		// POINTER precisely so "not boostable" is expressible: ABuilding/AHeroCharacter inherit the
		// nullptr default and never bind. AddUniqueDynamic can never double-bind.
		if (Provider)
		{
			if (FOnCombatantDamageBoostChanged* BoostDelegate = Provider->GetDamageBoostChangedDelegate())
			{
				BoostDelegate->AddUniqueDynamic(this, &UCombatantHealthBarComponent::HandleOwnerDamageBoostChanged);
			}
		}
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

void UCombatantHealthBarComponent::HandleOwnerDamageBoostChanged(float BoostPercent)
{
	// No cached boost state — every broadcast re-bands from scratch and re-pushes the whole row.
	PushDamageBoost(BoostPercent);
}

void UCombatantHealthBarComponent::PushDamageBoost(float BoostPercent)
{
	// Drive the CURRENT on-screen widget instance, for the same identity-proof reason as
	// HandleOwnerHPChanged: the live widget is the one FWorldWidgetScreenLayer took, which is not
	// necessarily the instance cached in BarWidget at BeginPlay.
	UCombatantHealthBarWidget* LiveBar = Cast<UCombatantHealthBarWidget>(GetWidget());
	if (!LiveBar)
	{
		// No widget (unresolved class / not yet created) — nothing to push. Never a crash.
		return;
	}

	if (BoostPercent <= 0.f)
	{
		// No boost ⇒ HIDE THE ROW via RowOpacity 0 (a float pin — SetRenderOpacity, never
		// SetVisibility, so the health bar keeps a constant head offset for every unit). The fill
		// and color are still fully specified (empty, band 1) so the widget stays a dumb pipe with
		// no stale band-3 purple waiting to flash if the row is ever re-shown.
		LiveBar->SetDamageBoost(0.f, BoostBand1Color.R, BoostBand1Color.G, BoostBand1Color.B, 0.f);
		RequestRedraw();
		return;
	}

	// BANDING — CONVENTIONS §5, and it lives HERE so WBP_CombatantHealthBar has ZERO conditionals.
	// CeilToInt is UPPER-INCLUSIVE BY DESIGN, not a rounding accident: 100% is the FULL light-blue
	// band-1 bar, 100.1% is a nearly-empty band-2 bar in a DARK BLUE frame, and 400% is the full
	// black band-4 bar — no special case anywhere. That exact-boundary ambiguity ("full bar of the
	// lower color" vs "nearly-empty bar of the higher color") is what the band-colored outline
	// resolves, which is why the outline gets the SAME color as the fill.
	const int32 Band = FMath::Clamp(FMath::CeilToInt(BoostPercent / 100.f), 1, 4);
	const float Frac = FMath::Max((BoostPercent - (Band - 1) * 100.f) / 100.f, MinBoostFillFraction);

	// Defensive upper clamp only. The Clamp above caps the band INDEX but not the numerator, so a
	// hypothetical out-of-cap BoostPercent (> 400) would compute Frac > 1. Unreachable through the
	// shipping path (ASummonedUnit::MaxPermanentDamageStacks = 80 == exactly +400%, and the exec
	// cheat clamps too) and it changes NO in-range value: for 0 < BoostPercent <= 400 the
	// expression already lands in [MinBoostFillFraction, 1].
	const float FillFraction = FMath::Min(Frac, 1.f);

	const FLinearColor BandColor = GetBoostBandColor(Band);
	LiveBar->SetDamageBoost(FillFraction, BandColor.R, BandColor.G, BandColor.B, 1.f);

	// Same rationale as HandleOwnerHPChanged: a no-op for live Screen-space Slate, required if this
	// component is ever hosted World-space on a render target.
	RequestRedraw();
}

FLinearColor UCombatantHealthBarComponent::GetBoostBandColor(int32 Band) const
{
	switch (Band)
	{
	case 1:  return BoostBand1Color;
	case 2:  return BoostBand2Color;
	case 3:  return BoostBand3Color;
	default: return BoostBand4Color;
	}
}

void UCombatantHealthBarComponent::ShowBarIfEnabled()
{
	SetVisibility(bShowHealthBar, /*bPropagateToChildren=*/true);
}

void UCombatantHealthBarComponent::HideBar()
{
	SetVisibility(false, /*bPropagateToChildren=*/true);
}
