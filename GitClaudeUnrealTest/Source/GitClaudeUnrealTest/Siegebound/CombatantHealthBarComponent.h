// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "UObject/SoftObjectPtr.h"
#include "CombatantHealthBarComponent.generated.h"

class UUserWidget;
class UCombatantHealthBarWidget;

/**
 *  Push-model overhead health bar (TASK-130, castle-parity REBUILD of the retired
 *  poll system). A screen-space UWidgetComponent subclass, added ONCE in-constructor
 *  to ASummonedUnit, ABuilding, and AHeroCharacter (each names its instance
 *  HPBarWidget; subclasses — AMinerUnit, ATower, ABarracks, ADeepMine — inherit it).
 *
 *  MIRRORS ACastle::HPBarWidget: at BeginPlay it soft-resolves the widget class,
 *  reads the owner as IHealthBarProvider + ITeamAgent, and calls
 *  UCombatantHealthBarWidget::InitForCombatant (SEED-THEN-BIND) + SetTeamColor once.
 *  There is NO poll timer (the failed poll system is retired); the bar updates only
 *  when the owner BROADCASTS its OnHPChanged delegate.
 *
 *  Visibility (reversed hide-at-full law): the bar is ALWAYS VISIBLE while the owner
 *  is alive and opted-in (bShowHealthBar). Hide/show on death/respawn is the OWNER's
 *  job (HideBar / ShowBarIfEnabled) — the ACastle::HandleDestroyed / ResetCastle
 *  parity, since screen-space widget components do not follow actor hidden-in-game
 *  state and there is no poll to observe death here.
 *
 *  Boost row (TASK-362, ancient grounds): the SAME widget grew a second bar above the
 *  health bar showing the owner's permanent damage boost. This component owns the whole
 *  boost path — it SEEDS UNCONDITIONALLY at BeginPlay (so a non-boostable owner's row is
 *  driven to opacity 0 instead of sitting at its design-time state — the qa/TASK-005
 *  major-2 seed-then-bind law), THEN binds FOnCombatantDamageBoostChanged only if the
 *  owner provides one, and it does ALL the banding math before pushing the single atomic
 *  SetDamageBoost BIE. DrawSize is (90, 22): ~8 px boost row + the original ~12 px health
 *  bar. Still NO poll, still no gameplay tick.
 *
 *  Everything null-safe: a missing widget class is a silent no-bar (logged once),
 *  never a crash. Zero combat/stat behavior change — it only reads HP getters + binds.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API UCombatantHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UCombatantHealthBarComponent();

	/** Shows the bar iff opted-in (bShowHealthBar). Called by the owner on respawn/reset (ACastle::ResetCastle parity). */
	void ShowBarIfEnabled();

	/** Hides the bar. Called by the owner on death/destruction (ACastle::HandleDestroyed parity) — screen-space widgets do not follow SetActorHiddenInGame. */
	void HideBar();

	/**
	 *  ⭐ THE TEAM PALETTE'S PUBLIC READ SEAM — WR-§6, manager ruling W4-R5 (added at TASK-560's
	 *  C2248 repair). These return this component's CLASS DEFAULTS (the CDO's BlueBarColor /
	 *  RedBarColor), so any display that must match a health bar's team tint — UWarMapWidget's
	 *  ally and enemy dots are the first caller — reads the ONE shipped owner instead of
	 *  re-typing the literals and drifting the day the palette moves.
	 *
	 *  ⛔ STATIC, AND THAT IS THE ENTIRE MECHANISM: a static member may read its own class's
	 *  protected members, so BlueBarColor/RedBarColor STAY protected below and gain NO writable
	 *  surface. ⛔ Plain C++ statics, NOT UFUNCTIONs — a palette read is not a Blueprint API and
	 *  reflecting it would invite a second caller. ⛔ ZERO parameters, deliberately: a
	 *  team-parameterised accessor would force Siegebound/TeamId.h into this header, which today
	 *  only the .cpp includes. (Zero parameters also means SC-§33 cannot fire structurally.)
	 *
	 *  ⚠️ CLASS DEFAULTS, ⛔ NOT AN INSTANCE READ, AND THE DIFFERENCE IS DELIBERATE: the per-bar
	 *  team tint applied in BeginPlay reads THIS INSTANCE's fields, which a BP subclass may
	 *  legitimately override. That read is a different question and is left exactly as it is —
	 *  routing it through these accessors would silently delete per-BP tint overrides.
	 */
	static FLinearColor GetDefaultBlueBarColor();
	static FLinearColor GetDefaultRedBarColor();

protected:

	/**
	 *  Soft-resolves the widget class, seeds+binds via the owner's OnHPChanged delegate, pushes the
	 *  team tint once, then seeds the boost row UNCONDITIONALLY and binds
	 *  FOnCombatantDamageBoostChanged iff the owner provides one (TASK-362). All null-safe.
	 */
	virtual void BeginPlay() override;

	/**
	 *  Component-side delegate handler (TASK-130 render-fix). Bound to the owner's OnHPChanged in
	 *  BeginPlay, ADDITIVE to the widget's own seed-then-bind. On each HP change it drives the
	 *  CURRENT on-screen `GetWidget()` directly (identity-proof: if the widget's own binding is
	 *  updating a stale/offscreen instance from an earlier frame, this always targets the live one)
	 *  and calls `RequestRedraw()` (World-space stale-render safety; a no-op for Screen-space live Slate).
	 */
	UFUNCTION()
	void HandleOwnerHPChanged(float CurrentHP, float MaxHP);

	/**
	 *  FOnCombatantDamageBoostChanged handler (TASK-362). Bound in BeginPlay ONLY when the owner
	 *  actually provides a delegate (a building/hero returns nullptr). Forwards straight to
	 *  PushDamageBoost — every broadcast re-bands and re-pushes, there is no cached boost state.
	 */
	UFUNCTION()
	void HandleOwnerDamageBoostChanged(float BoostPercent);

	/**
	 *  Bands BoostPercent (0 = none, 100 = +100%, 400 = the cap) and pushes the whole boost row
	 *  through the widget's single atomic SetDamageBoost BIE. ALL banding math lives here — the
	 *  widget has ZERO conditionals (CONVENTIONS §5). Drives the CURRENT on-screen GetWidget()
	 *  (identity-proof, the HandleOwnerHPChanged precedent) and RequestRedraw()s after.
	 *  BoostPercent <= 0 ⇒ RowOpacity 0, which is how a non-boostable actor's row is hidden.
	 */
	void PushDamageBoost(float BoostPercent);

	/** Band index (1-4) → its EditDefaultsOnly tint. Out-of-range clamps to band 4 (the strongest). */
	FLinearColor GetBoostBandColor(int32 Band) const;

	/**
	 *  Widget class shown by this bar (default /Game/UI/WBP_CombatantHealthBar, built
	 *  in TASK-131). Soft — resolved null-safe at BeginPlay; a missing asset means no
	 *  bar (logged once), never a crash. A BP may retarget or clear it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	TSoftClassPtr<UUserWidget> HealthBarWidgetClass;

	/** Per-actor/per-BP opt-out (default true). When false the bar stays hidden — a zero-code way to suppress clutter (e.g. miners). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	bool bShowHealthBar = true;

	/** Relative Z (units) of the bar above the actor root. Default 120; buildings/hero BPs may raise it. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	float BarHeightZ = 120.f;

	/** Fill tint for a friendly (Blue) owner — §6 palette / MI_TeamColor linear values. Pushed to SetTeamColor once at init. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	FLinearColor BlueBarColor = FLinearColor(0.05f, 0.30f, 1.00f);

	/** Fill tint for an enemy (Red) owner — §6 palette / MI_TeamColor linear values. Pushed to SetTeamColor once at init. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	FLinearColor RedBarColor = FLinearColor(1.00f, 0.10f, 0.05f);

	//~ Begin permanent damage boost row (TASK-362, ancient grounds). Tint is DATA — these four
	//  colors are pushed to the widget every update and are NEVER hardcoded in WBP_CombatantHealthBar.
	//  The ramp darkens monotonically (light → navy → purple → black) so "deeper = stronger" reads
	//  without a tooltip; band 3 is a DEEP purple on purpose (a bright violet computes to ~1.06:1
	//  against the boost track and vanishes). Contrast figures are vs the BoostBarTrackColor below.

	/** Band 1, 0-100% boost — light blue. ≈#C4E7FF, 2.99:1 on the boost track. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand1Color = FLinearColor(0.55f, 0.80f, 1.00f);

	/** Band 2, 100-200% boost — dark blue. ≈#1927A0, 2.96:1. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand2Color = FLinearColor(0.010f, 0.020f, 0.350f);

	/** Band 3, 200-300% boost — deep purple. ≈#7C19AD, 2.09:1. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand3Color = FLinearColor(0.200f, 0.010f, 0.420f);

	/** Band 4, 300-400% boost — near black. ≈#191920, 4.50:1 (the best on the bar). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand4Color = FLinearColor(0.010f, 0.010f, 0.014f);

	/**
	 *  The BoostBar's own MEDIUM-GREY track. RECORDED HERE FOR THE RECORD ONLY — this component
	 *  never pushes it; the artist authors BoostBar's background brush tint from THIS number at
	 *  TASK-368. It exists as data so the value has exactly one home. Why BoostBar gets its own
	 *  track instead of reusing Bar's near-black (0.03,0.03,0.03)@0.7: band-4 black on near-black
	 *  is ~1.3:1, i.e. the STRONGEST unit would get the WORST indicator.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBarTrackColor = FLinearColor(0.22f, 0.22f, 0.24f, 0.85f);

	/**
	 *  Floor for the boost fill fraction so a just-crossed band (e.g. 100.1% ⇒ 0.001) still paints
	 *  a visible sliver of the NEW band color instead of an empty bar. Purely cosmetic: the band
	 *  outline, not the fill, is what disambiguates exactly-100% from just-past-100%.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	float MinBoostFillFraction = 0.04f;

	//~ End permanent damage boost row

private:

	/** The widget instance cached from GetWidget() after SetWidgetClass; used for the seed-then-bind + team-tint setup in BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<UCombatantHealthBarWidget> BarWidget;
};
