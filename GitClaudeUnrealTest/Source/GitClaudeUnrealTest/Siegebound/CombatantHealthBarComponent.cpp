// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CombatantHealthBarComponent.h"

#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
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

	/**
	 *  ⛔ HARD FLOOR on the occlusion poll period (TASK-791). HealthBarOcclusionIntervalSeconds is
	 *  EditDefaultsOnly, so a Blueprint may set it to 0 or negative — and without this floor that
	 *  would silently degrade the cull into the ONE thing VIS-§2 forbids by name: a trace per pawn
	 *  per frame across a 20+ unit roster.
	 *
	 *  ⭐ IT IS A GUARANTEE, NOT A TUNING VALUE, which is why it lives here as a constant rather than
	 *  as a fourth EditDefaultsOnly knob: a floor that a designer can lower is not a floor. Whatever
	 *  the interval is set to, this component traces at most 30×/second.
	 */
	const float MinHealthBarOcclusionPeriodSeconds = 1.f / 30.f;
}

UCombatantHealthBarComponent::UCombatantHealthBarComponent()
{
	// Screen space so the bar reads at any camera angle/distance — the ACastle::HPBarWidget
	// precedent. The WidgetComponent keeps its own render tick; HP updates never poll (they arrive
	// via the owner's OnHPChanged delegate).
	//
	// ⛔ TASK-791 / VIS-§2 — READ BEFORE "FIXING" THIS LINE: a Screen-space widget component is
	// composited as a SLATE OVERLAY and NEVER participates in the depth buffer, BY ENGINE DESIGN.
	// Bars drawing over solid stonework is therefore the DEFINED behaviour of this line, not a
	// broken setting, and there is no depth flag anywhere that would change it. The remedy shipped
	// for VID-004 is the occlusion VISIBILITY CULL below — ⛔ a switch to EWidgetSpace::World is
	// REFUSED (ruling VIS-R1): it costs a RENDER TARGET PER BAR across a 20+ pawn roster and is
	// owed at three call sites (here, ACastle::HPBarWidget, ADamageNumberActor::NumberWidget).
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(CombatantHealthBarDrawSize);

	// Tick every frame (TASK-130 render-fix insurance): a screen-space widget component is
	// (re)added to FWorldWidgetScreenLayer and re-projected to the actor's screen position in
	// UpdateWidgetOnScreen(), which runs at the END of TickComponent — an Automatic/disabled tick
	// can drop the widget off the screen layer. Enabled keeps it hosted + following the actor.
	//
	// ⛔ TASK-791 MADE THIS DOUBLY LOAD-BEARING: the occlusion cull hides the bar via SetVisibility,
	// and it needs a tick to survive in order to ever re-poll and un-hide it. Under
	// ETickMode::Disabled the engine kills the component tick OUTRIGHT — WidgetComponent.cpp:1275-1279,
	// `if (TickMode == Disabled && !bRedrawRequested) { SetComponentTickEnabled(false); return; }` —
	// which would strand a culled bar hidden for ever. ⛔ Do not "optimise" this tick mode.
	//
	// ⚠️ PRECISION, so nobody over-trusts this comment (qa/TASK-801 W-4): the OTHER auto-disable at
	// WidgetComponent.cpp:1264 keys on IsWidgetVisible(), and IsWidgetVisible() (:1074-1090) consults
	// the COMPONENT's IsVisible() only in World space (:1077) — in Screen space it reads the INNER
	// UUserWidget's visibility, which this cull never touches (bPropagateToChildren propagates to child
	// SCENE COMPONENTS, not into the widget). ⇒ under Automatic the cull alone would NOT trip :1264.
	// The Disabled case above is the real hazard, and the TASK-130 render reason below is the other.
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

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  OCCLUSION CULL (TASK-791 — VIS-§2, ruling VIS-R1)
// ═══════════════════════════════════════════════════════════════════════════════════════════

void UCombatantHealthBarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Super FIRST and unconditionally: UWidgetComponent::TickComponent → UpdateWidget() →
	// UpdateWidgetOnScreen(), which is the engine's add/remove of this bar on
	// FWorldWidgetScreenLayer and is gated on IsVisible(). Skipping it for ANY reason would strand
	// the bar's screen projection, which is the TASK-130 render bug this component already survived
	// once. Everything below is additive and runs after.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bOccludeHealthBarWhenBlocked)
	{
		// ⛔ CULL OFF ⇒ ONE BOOL TEST PER FRAME AND NOTHING ELSE: no accumulate, no trace, no
		// SetVisibility. The branch below is a SELF-HEAL, not a per-frame cost: it can only fire on
		// the single frame after the toggle is turned off while a bar happened to be latched
		// occluded, and it exists so "false restores today's behaviour exactly" holds even when the
		// switch is flipped at runtime rather than only at defaults.
		if (bHealthBarOccluded)
		{
			bHealthBarOccluded = false;
			ApplyBarVisibility();
		}
		return;
	}

	if (!bBarShownByOwner)
	{
		// The owner has this bar hidden — dead, or opted out via bShowHealthBar (miners). ⭐ A bar
		// that is not on screen cannot be drawn through stonework, so we spend NO trace on it: an
		// opted-out unit costs exactly two bool tests per frame for the whole of its life.
		// ⚠️ The accumulator is deliberately left alone rather than reset, so a respawn does not
		// also pay a fresh full period before its first poll.
		return;
	}

	// ⛔ NOT A TRACE. This is the entire per-frame cost of the cull: one out-of-line static CALL
	// wrapping one add and one compare. ⚠️ The call is named rather than glossed (qa/TASK-801 N-2) —
	// ShouldPollOcclusion lives in this .cpp, so it is a real call per unit per frame (~20/frame for
	// the roster), not an inlined pair of instructions. Immaterial next to the trace it prevents, but
	// the honest figure is the one that stays true when someone re-measures it.
	if (!ShouldPollOcclusion(OcclusionPollAccumulator, DeltaTime, HealthBarOcclusionIntervalSeconds))
	{
		return;
	}

	UpdateHealthBarOcclusion();
	ApplyBarVisibility();
}

bool UCombatantHealthBarComponent::ShouldPollOcclusion(float& InOutAccumulatedSeconds, float DeltaSeconds, float ConfiguredIntervalSeconds)
{
	InOutAccumulatedSeconds += DeltaSeconds;

	const float Period = FMath::Max(ConfiguredIntervalSeconds, MinHealthBarOcclusionPeriodSeconds);
	if (InOutAccumulatedSeconds < Period)
	{
		return false;
	}

	// ⛔ RESET, NOT `-= Period`: no catch-up. Subtracting would leave a one-second hitch holding
	// ~7 periods of banked time and fire a trace on ~7 CONSECUTIVE frames afterwards — a burst of
	// exactly the cost this design refuses, arriving on the frames the game can least afford it.
	// Dropping the arrears is the right trade for a cull: the answer we skipped is stale anyway.
	InOutAccumulatedSeconds = 0.f;
	return true;
}

bool UCombatantHealthBarComponent::ComputeDesiredBarVisibility(bool bOwnerWantsBarShown, bool bCullEnabled, bool bOccluded)
{
	// ⭐ THE OWNER'S INTENT IS THE OUTER AND — never an OR, and never a plain `!bOccluded`. The cull
	// may only SUBTRACT visibility; there is no path here by which a clear trace shows a bar the
	// owner hid, because that would resurrect dead units' health bars.
	// With bCullEnabled false this collapses to `bOwnerWantsBarShown`, which is bit-for-bit the
	// pre-TASK-791 behaviour of ShowBarIfEnabled()/HideBar().
	return bOwnerWantsBarShown && !(bCullEnabled && bOccluded);
}

void UCombatantHealthBarComponent::UpdateHealthBarOcclusion()
{
	// ⛔ FAIL-OPEN, AND EVERY EARLY-OUT BELOW RELIES ON IT: we clear first, so "we could not tell"
	// resolves to VISIBLE. A missing camera must never blank every health bar on the field — that
	// failure would be far louder than the one this cull is fixing.
	bHealthBarOccluded = false;

	const UWorld* const World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	if (!GetWidget())
	{
		// No widget instance — the logged-once unresolved-class no-bar path in BeginPlay. Nothing is
		// on screen to occlude, so nothing is traced for.
		return;
	}

	AActor* const OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// ⭐ THE OWNING PLAYER'S CAMERA, NOT "PLAYER 0" — and this is the same accessor
	// UWidgetComponent::UpdateWidgetOnScreen() uses to decide whose viewport this bar is projected
	// into. Asking a different camera than the one the bar is drawn for is how a split-screen build
	// would cull player two's bars against player one's view. Consistency here is free; getting it
	// wrong is invisible until someone adds a second local player.
	//
	// ⚠️ WRITTEN AS EARLY-OUTS RATHER THAN AS `Cond ? TObjectPtr<T> : nullptr` CHAINS (qa/TASK-801
	// N-1): the ternary form mixes a TObjectPtr and a nullptr in one conditional expression and leans
	// on overload resolution in UObject/ObjectPtr.h to pick the implicit raw conversion. It compiles
	// today, but it is a construct that depends on TObjectPtr's conversion operators staying exactly
	// as they are. Early-outs remove the construct instead of restating it — and they read the same
	// as every other fail-open above.
	const ULocalPlayer* const OwningPlayer = GetOwnerPlayer();
	if (!OwningPlayer || !OwningPlayer->PlayerController)
	{
		return;
	}

	const APlayerCameraManager* const CameraManager = OwningPlayer->PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		return;
	}

	// ⭐ THE TRACE ENDS AT THE BAR, NOT AT THE PAWN'S FEET: GetComponentLocation() is the owner's
	// root lifted by BarHeightZ, and it is the exact world point FWorldWidgetScreenLayer projects to
	// screen. Culling the thing we actually draw is the whole idea — tracing to the capsule base
	// would hide bars whose owners are standing behind a parapet that their bar clears.
	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const FVector BarAnchorLocation = GetComponentLocation();

	// bTraceComplex=false: simple collision only. Per-triangle accuracy buys nothing for a "is a
	// wall in the way" question and would multiply the cost of the one thing we are rationing.
	// The owner is ignored via the constructor's third argument — a unit must never occlude itself.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatantHealthBarOcclusion), /*bTraceComplex=*/false, OwnerActor);

	// ⛔⛔ THE CAMERA CAN BE INSIDE BLOCKING GEOMETRY, AND THAT WOULD HIDE EVERY BAR ON THE FIELD AT
	// ONCE — TASK-790 makes it a DESIGNED, RECURRING STATE rather than an accident: its near-clip
	// floor deliberately parks the follow camera up to 150 uu INSIDE the watchtower for ≈2 s at every
	// ladder approach (VID-004 01:12.0–01:13.5). In that window every unit's ray starts inside the
	// same hull.
	//
	// ⛔ `QueryParams.bFindInitialOverlaps = false` WAS THE ORIGINAL GUARD AND IT IS INERT ON THIS
	// QUERY. Do not re-add it believing otherwise — three engine lines, UE 5.8:
	//   • CollisionQueryFilterCallback.h:59  — `bDiscardInitialOverlaps = !Params.bFindInitialOverlaps`
	//   • CollisionQueryFilterCallback.cpp:211-217 — PostFilterImp opens `if (!bIsSweep) return Block;`
	//     and only THEN reaches `else if (bIsOverlap && bDiscardInitialOverlaps)` — the flag's ONLY
	//     consumer in the whole engine.
	//   • SceneQuery.cpp:522 — `FCollisionQueryFilterCallback QueryCallback(Params, Traits::GeometryQuery
	//     == ESweepOrRay::Sweep)` ⇒ bIsSweep is FALSE for a raycast.
	// ⇒ on a line trace the flag is never read, and the hit it was meant to suppress is really
	// produced: Chaos::FConvex::RaycastFast starting inside a hull leaves EntryTime at 0 and returns
	// true at OutTime = 0.
	//
	// ⭐ SO THE BURIAL IS DETECTED ON THE HIT INSTEAD OF SUPPRESSED IN THE FILTER, which is why this
	// is the SINGLE form and not the cheaper TEST form: LineTraceTestByChannel returns one bool and
	// CANNOT distinguish "a wall is in the way" from "the camera is standing inside one". That is the
	// entire reason the cheaper query is refused here, and it is worth the cost — see below.
	//
	// ⭐ AND IT IS QUERY-KIND AGNOSTIC, unlike the flag: a start-penetrating hit reports Distance 0
	// whether this is ever a ray or a sweep, so the fail-open survives a future change of instrument.
	FHitResult OcclusionHit;
	const bool bTraceBlocked = World->LineTraceSingleByChannel(
		OcclusionHit,
		CameraLocation,
		BarAnchorLocation,
		HealthBarOcclusionChannel,
		QueryParams);

	// ⚠️ THE COST OF SINGLE OVER TEST, STATED RATHER THAN GLOSSED: the TEST form carries
	// EQueryFlags::AnyHit and EHitFlags::None (SceneQuery.cpp:368, :394), so it stops at the FIRST
	// blocking hit and skips ConvertTraceResults entirely (:569). SINGLE drops AnyHit — the traversal
	// must resolve the NEAREST hit — and pays one hit conversion into one stack FHitResult. ⭐ It is
	// still ONE raycast with ZERO heap allocation, still ≈2.2 queries per frame across a 20-unit
	// roster at the 0.15 s period, and still an order cheaper than the sweep this could have been.
	bHealthBarOccluded = ComputeOcclusionFromTraceResult(
		bTraceBlocked,
		OcclusionHit.bStartPenetrating,
		OcclusionHit.Distance);
}

bool UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(bool bTraceBlocked, bool bTraceStartedInsideGeometry, float HitDistanceUU)
{
	// ⛔ A ZERO-DISTANCE HIT IS A NON-ANSWER, NOT AN OCCLUSION — it means the camera was inside the
	// geometry it "hit", so nothing was ever measured between the camera and the bar. It joins the
	// no-world / no-camera / no-widget early-outs on the FAIL-OPEN side of the ledger.
	//
	// ⭐ The first two clauses are exactly FHitResult::IsValidBlockingHit() (HitResult.h:236-239,
	// `bBlockingHit && !bStartPenetrating`) — the engine's own name for this question. The distance
	// clause is a deliberate SECOND WITNESS read from the raw hit rather than from the conversion
	// layer. On today's path the two cannot disagree (ChaosInterfaceWrapperCore.h:117 defines the
	// initial-overlap flag AS `Distance <= 0.f`, and CollisionConversions.cpp:366 copies it into
	// bStartPenetrating), and that is precisely why it is cheap insurance: this one bool decides
	// whether the entire roster's HUD survives a buried camera, and it should not rest on a single
	// engine-internal bit staying wired the way it is wired today.
	return bTraceBlocked && !bTraceStartedInsideGeometry && HitDistanceUU > 0.f;
}

void UCombatantHealthBarComponent::ApplyBarVisibility()
{
	// ⛔ THE SINGLE WRITER. Unconditional (no "only if it changed" guard) precisely so the
	// cull-disabled path is bit-for-bit the old SetVisibility(bShowHealthBar/false, true) call —
	// the regression guard is an equivalence claim, and a short-circuit here would weaken it for
	// nothing: this runs on owner state changes and at ≈6.7 Hz, never per frame.
	const bool bDesiredVisibility = ComputeDesiredBarVisibility(
		bBarShownByOwner,
		bOccludeHealthBarWhenBlocked,
		bHealthBarOccluded);

	SetVisibility(bDesiredVisibility, /*bPropagateToChildren=*/true);
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
	// TASK-791: this used to call SetVisibility directly. It now records the OWNER'S INTENT and
	// lets ApplyBarVisibility() compose it with the occlusion state — so a unit that respawns
	// behind a wall comes back correctly hidden instead of popping through the stonework, and a
	// unit that respawns in the open comes back visible. With the cull off the composition
	// collapses to SetVisibility(bShowHealthBar, true), i.e. exactly the line this replaced.
	bBarShownByOwner = bShowHealthBar;
	ApplyBarVisibility();
}

void UCombatantHealthBarComponent::HideBar()
{
	// ⛔ THE DEATH PATH OUTRANKS THE CULL, ALWAYS. Latching the intent (rather than writing
	// visibility here) is what guarantees the next occlusion poll cannot un-hide a dead unit's bar:
	// ComputeDesiredBarVisibility ANDs with this and can only ever subtract.
	bBarShownByOwner = false;
	ApplyBarVisibility();
}

FLinearColor UCombatantHealthBarComponent::GetDefaultBlueBarColor()
{
	// ⭐ A static member function may read its own class's protected members — which is the whole
	// reason this seam exists in this shape: BlueBarColor stays protected and no caller gains a
	// way to write it (WR-§6, ruling W4-R5).
	//
	// ⛔ NO NULL BRANCH, DELIBERATELY: GetDefault<T>() on a statically-linked native UCLASS always
	// returns that class's CDO, so a defensive branch here would be untestable dead code.
	return GetDefault<UCombatantHealthBarComponent>()->BlueBarColor;
}

FLinearColor UCombatantHealthBarComponent::GetDefaultRedBarColor()
{
	// Same contract as GetDefaultBlueBarColor() above: class defaults, no null branch, and the
	// field stays protected.
	return GetDefault<UCombatantHealthBarComponent>()->RedBarColor;
}
