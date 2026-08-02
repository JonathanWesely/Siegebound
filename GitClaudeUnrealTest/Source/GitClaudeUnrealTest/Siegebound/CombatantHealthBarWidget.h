// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Siegebound/HealthBarProvider.h"
#include "CombatantHealthBarWidget.generated.h"

/**
 *  C++ base for /Game/UI/WBP_CombatantHealthBar (TASK-131 duplicates the WORKING
 *  WBP_CastleHealthBar donor and reparents it here) — the overhead HP bar for units,
 *  buildings, and the hero. Rebuilt 2026-07-10 (TASK-130) on the castle's proven
 *  PUSH/delegate model; MIRRORS UCastleHealthBarWidget exactly.
 *
 *  Contract (identical to UCastleHealthBarWidget):
 *  - UCombatantHealthBarComponent::BeginPlay calls InitForCombatant(owner) on the
 *    widget instance living in its HPBarWidget component.
 *  - InitForCombatant SEEDS first — OnHPChanged fires immediately with the owner's
 *    current values — THEN binds FOnCombatantHPChanged (seed-then-bind,
 *    qa/TASK-005 major 2: a bind-only consumer created at a value that never changes
 *    again would stay stale forever).
 *  - OnHPChanged / SetTeamColor / SetDamageBoost are BlueprintImplementableEvents with
 *    FLOAT PARAMS ONLY (MCP/CONVENTIONS widget rule). The WBP MUST implement them as TRUE
 *    overrides (bOverrideFunction=true); a K2Node_CustomEvent is DSL-indistinguishable
 *    and NEVER fires from C++ (the defect that hid the bug 5x — TASK-131 verifies the
 *    node class against the working castle widget).
 *  - SetDamageBoost (TASK-362, the boost row) is driven ENTIRELY by
 *    UCombatantHealthBarComponent: it owns the banding math and the seed-then-bind on the
 *    owner's FOnCombatantDamageBoostChanged. This widget deliberately holds NO boost state
 *    and does NOT bind that delegate — one push path, no second source of truth.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UCombatantHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Points this bar at a combat actor: pushes the owner's current HP/MaxHP through
	 *  OnHPChanged immediately (seed), THEN binds its OnHPChanged delegate (bind).
	 *  Null/invalid provider = warn + no-op. Safe to call repeatedly: AddUniqueDynamic
	 *  never double-binds; a different provider replaces the old binding so the bar
	 *  never receives two update streams.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")
	void InitForCombatant(TScriptInterface<IHealthBarProvider> Provider);

	/**
	 *  Implemented by WBP_CombatantHealthBar (TASK-131): update the fill —
	 *  ProgressBar SetPercent(CurrentHP / MaxHP), guard MaxHP > 0 before dividing.
	 *  Float params only (CONVENTIONS widget rule). Identical contract to
	 *  UCastleHealthBarWidget::OnHPChanged.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void OnHPChanged(float CurrentHP, float MaxHP);

	/**
	 *  Implemented by WBP_CombatantHealthBar (TASK-131): tint the ProgressBar FILL
	 *  from the pushed linear RGB (blue friendly / red enemy). Pushed ONCE at init by
	 *  the component; fixed regardless of HP. Float params only — an FLinearColor BP
	 *  param is not authorable (CONVENTIONS widget rule).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void SetTeamColor(float R, float G, float B);

	/**
	 *  Implemented by WBP_CombatantHealthBar (TASK-368): drive the PERMANENT-DAMAGE-BOOST row
	 *  that sits directly above the health bar (BarStack ▸ BoostOutline ▸ BoostBar).
	 *
	 *  ONE atomic event, NOT two — unlike SetTeamColor above, the COLOR CHANGES WITH THE VALUE
	 *  (light blue 0-100% ▸ dark blue 100-200% ▸ purple 200-300% ▸ black 300-400%). Splitting
	 *  fill from color would leave a frame where band-3 purple paints at a band-4 fill, i.e. a
	 *  visibly WRONG boost level. Float params only (CONVENTIONS widget rule — no enums, no
	 *  structs, no FLinearColor).
	 *
	 *  ALL banding math is C++ (UCombatantHealthBarComponent::PushDamageBoost) — the widget is a
	 *  DUMB PIPE with ZERO conditionals. Required EventGraph, one linear exec chain, one
	 *  MakeLinearColor(R, G, B, 1.0) fanned to both consumers (CONVENTIONS §5 / plan §5):
	 *      SetPercent(BoostBar, FillFraction)
	 *        -> SetFillColorAndOpacity(BoostBar, $C)
	 *        -> SetBrushColor(BoostOutline, $C)
	 *        -> SetRenderOpacity(BoostOutline, RowOpacity)
	 *        -> SetRenderOpacity(BoostBar, RowOpacity)
	 *  The alpha literal MUST be 1.0 (read it back). RowOpacity is a FLOAT hide (0 = row gone,
	 *  1 = row shown) — never SetVisibility, so the health bar keeps a constant head offset for
	 *  boosted and unboosted units alike.
	 *
	 *  ⚠️ Like OnHPChanged / SetTeamColor this MUST be authored as a TRUE override
	 *  (bOverrideFunction = true). A K2Node_CustomEvent of the same name is DSL-indistinguishable,
	 *  reports bIsImplemented = true, and NEVER fires from C++ — the exact defect that hid the
	 *  health-bar bug five times (TASK-131). Assert the node's object CLASS via get_node_infos.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void SetDamageBoost(float FillFraction, float R, float G, float B, float RowOpacity);

protected:

	/** FOnCombatantHPChanged handler — forwards to the BP-implemented OnHPChanged (the HandleCastleHPChanged shape). */
	UFUNCTION()
	void HandleHPChanged(float CurrentHP, float MaxHP);

	/** Provider this bar observes. Set by InitForCombatant; used to unbind when re-targeted. */
	UPROPERTY(Transient)
	TScriptInterface<IHealthBarProvider> ObservedProvider;
};
