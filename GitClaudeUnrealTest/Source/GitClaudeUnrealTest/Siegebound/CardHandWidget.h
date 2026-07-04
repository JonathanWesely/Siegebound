// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPtr.h"
#include "CardHandWidget.generated.h"

class ASiegePlayerController;
class ASiegePlayerState;
class UDataTable;
class UDeckComponent;
struct FCardRow;

/**
 *  C++ base for /Game/UI/WBP_CardHand (TASK-033 reparents the UMG duplicate to
 *  this class) — the GDD §3.4/§3.5/§3.6/§7 hand bar: 6 card slots (name + cost,
 *  greyed when unaffordable), the next-card preview slot, discard buttons, and
 *  the refusal message line.
 *
 *  Division of labor (CONVENTIONS widget rules): C++ pulls model state and
 *  pushes DISPLAY-READY data through BlueprintImplementableEvents whose params
 *  are float/int/bool/FString only (MCP cannot author enum BP params). The UMG
 *  side (TASK-033) is pure presentation — it never reads DT_Cards, the deck, or
 *  gold itself; costs and display names arrive through the BIEs, sourced from
 *  DT_Cards rows here (GDD §3.0 — never typed into UMG).
 *
 *  Contract (TASK-029 → TASK-033):
 *  - Call InitForController(owning ASiegePlayerController) ONCE, e.g. from the
 *    widget's Event Construct via Cast(GetOwningPlayer()). It SEEDS every hand
 *    slot + the preview + affordability immediately from current state, THEN
 *    binds the update delegates (seed-then-bind law, CONVENTIONS /
 *    qa/TASK-005-report.md major 2). Init order vs the deck's BuildAndShuffle
 *    does not matter: a widget constructed first seeds empty slots and is
 *    refreshed by the build's own broadcasts (OnDeckHandChanged fires on the
 *    initial deal; OnDeckNextCardChanged force-fires on build/reset).
 *  - OnHandSlotUpdated fires once per slot per refresh and must be idempotent:
 *    hand changes, gold changes, and re-inits re-push slots with mostly
 *    identical data. An EMPTY CardID string means an EMPTY slot (hide the card
 *    face; normal state in the qa/TASK-021-report.md WARN-2 window) — never
 *    compare against the literal "None".
 *  - OnNextCardUpdated: empty DisplayName = no next card (hide the preview).
 *    Deliberately carries no affordability flag (§7 preview shows art + cost).
 *  - OnCardRefusedMessage: exactly one call per refused play/discard, carrying
 *    the controller's §3.0-style reason. The ~2 s show-then-hide presentation
 *    is implemented in UMG (TASK-033), not here.
 *  - UMG buttons call RequestPlaySlot / RequestDiscardSlot (0-based slot
 *    index). These are pure null-safe pass-throughs: ALL refusal logic (gold,
 *    caps, placement) lives in the controller (TASK-023/030), whose
 *    OnCardRefused broadcast comes back through OnCardRefusedMessage. The
 *    widget's grey-out is cosmetic only — the controller re-checks
 *    affordability authoritatively.
 *
 *  Model sources (bound in InitForController, all null-safe):
 *  - UDeckComponent (found on the controller, TASK-022/023): slots re-pulled
 *    via GetHandCardID(0..GetHandSize()-1) on every payload-less
 *    OnDeckHandChanged (coarse-refresh contract); the preview updates from
 *    OnDeckNextCardChanged, treating EVERY broadcast as authoritative — the
 *    component already value-filters and force-fires on build/reset (TASK-022
 *    flagged decision 7), so this widget must NOT self-filter.
 *  - ASiegePlayerState::OnGoldChanged: every gold change re-pushes all slots
 *    with recomputed bAffordable (§3.5 "greyed when unaffordable" re-grey).
 *  - ASiegePlayerController::OnCardRefused (TASK-023): message pass-through.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UCardHandWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UCardHandWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 *  Points the hand UI at its controller. SEEDS first — pushes all hand
	 *  slots (OnHandSlotUpdated x GetHandSize()) and the preview
	 *  (OnNextCardUpdated) immediately from current deck/gold state — THEN
	 *  binds OnDeckHandChanged + OnDeckNextCardChanged (deck), OnGoldChanged
	 *  (player state) and OnCardRefused (controller). Seed-then-bind,
	 *  qa/TASK-005-report.md major 2: a bind-only widget created at a pinned
	 *  value would stay stale forever.
	 *
	 *  Null controller = warn + no-op. Safe to call repeatedly: the same
	 *  controller re-seeds without double-binding (AddUniqueDynamic); a
	 *  DIFFERENT controller replaces every previous binding so the widget
	 *  never receives two update streams.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")
	void InitForController(ASiegePlayerController* Controller);

	/**
	 *  UMG card-button pass-through → ASiegePlayerController::PlayHandSlot(Slot)
	 *  (TASK-023). No local checks beyond null-safety — the controller owns
	 *  every refusal (and reports it back via OnCardRefusedMessage).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")
	void RequestPlaySlot(int32 SlotIndex);

	/**
	 *  UMG discard-button pass-through →
	 *  ASiegePlayerController::DiscardHandSlot(Slot) (TASK-023; §3.6 — the
	 *  1-gold charge and the 0-gold refusal are the controller's).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")
	void RequestDiscardSlot(int32 SlotIndex);

	/**
	 *  Implemented by WBP_CardHand (TASK-033): render one hand slot.
	 *  - SlotIndex: 0..GetHandSize()-1 (6 slots, GDD §3.4).
	 *  - CardID: DT_Cards row name as a string; EMPTY string = EMPTY slot →
	 *    hide the card face (normal until the TASK-031 reimport, WARN-2).
	 *  - DisplayName: the row's human name ("Arrow Tower"); falls back to the
	 *    raw CardID when the row is missing.
	 *  - Cost: gold cost from the row (§3.5 top-right); 0 when empty/missing.
	 *  - bAffordable: current gold covers Cost (§3.5 — grey the card when
	 *    false). Recomputed on every gold change. Cosmetic only — key the
	 *    empty-frame decision off CardID being empty, NOT off bAffordable.
	 *  Idempotent by contract: repeat calls with unchanged data must be safe.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void OnHandSlotUpdated(int32 SlotIndex, const FString& CardID, const FString& DisplayName, int32 Cost, bool bAffordable);

	/**
	 *  Implemented by WBP_CardHand (TASK-033): render the §3.4 next-card
	 *  preview (top of the draw pile). Empty DisplayName = no next card →
	 *  hide the preview slot. No affordability param by design (§7: the
	 *  preview shows the card + cost; it is not a playable button).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void OnNextCardUpdated(const FString& DisplayName, int32 Cost);

	/**
	 *  Implemented by WBP_CardHand (TASK-033): surface a refusal reason
	 *  ("Not enough gold", "Miner limit reached", ...) — the §3.0 refund-rule
	 *  HUD message. Show for ~2 s then hide (timing/animation lives in UMG).
	 *  Fires exactly once per refused action (1:1 pass-through of the
	 *  controller's OnCardRefused).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
	void OnCardRefusedMessage(const FString& Reason);

protected:

	/** FOnDeckHandChanged handler — payload-less coarse refresh: re-pulls ALL hand slots (TASK-022 contract). */
	UFUNCTION()
	void HandleDeckHandChanged();

	/** FOnDeckNextCardChanged handler — every broadcast is authoritative (TASK-022 flagged decision 7): push it, never self-filter. */
	UFUNCTION()
	void HandleDeckNextCardChanged(FName NextCardID);

	/** FOnGoldChanged handler — re-pushes all slots so bAffordable re-greys on every gold change (§3.5). */
	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	/** FOnCardRefused handler — 1:1 forward to the BP-implemented OnCardRefusedMessage. */
	UFUNCTION()
	void HandleCardRefused(const FString& Reason);

	/** Card stat table (GDD §3.0) — Cost/DisplayName source for the BIE pushes. Soft, resolved null-safe at use time. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|UI")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/** Controller this hand UI serves. Set by InitForController; used by the Request* pass-throughs and to unbind on re-target. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Siegebound|UI")
	TObjectPtr<ASiegePlayerController> ObservedController;

	/** The controller's UDeckComponent (TASK-022, wired as a subobject by TASK-023). Slot/preview source. */
	UPROPERTY(Transient)
	TObjectPtr<UDeckComponent> ObservedDeck;

	/** The controller's ASiegePlayerState — gold source for bAffordable. */
	UPROPERTY(Transient)
	TObjectPtr<ASiegePlayerState> ObservedPlayerState;

private:

	/** Pushes OnHandSlotUpdated for every slot [0..GetHandSize()-1] — the seed pass, the hand-change refresh, and the gold re-grey all share this path. */
	void RefreshAllHandSlots();

	/** Pushes one slot: deck CardID -> DT_Cards row -> OnHandSlotUpdated(SlotIndex, CardID, DisplayName, Cost, bAffordable). Empty slot = empty CardID string. */
	void PushHandSlot(int32 SlotIndex);

	/** Pushes the preview: CardID -> DT_Cards row -> OnNextCardUpdated(DisplayName, Cost). NAME_None = empty DisplayName + cost 0. */
	void PushNextCardPreview(FName NextCardID);

	/** Removes every delegate binding on the currently observed controller/deck/player state and clears the observed pointers. */
	void UnbindObservedSources();

	/** DT_Cards row lookup (soft load, null-safe). Missing table/row logs ONCE (slots re-push every gold tick — per-lookup logs would spam) and returns nullptr. */
	const FCardRow* ResolveCardRow(FName CardID);

	/** CardIDs whose missing DT_Cards row was already logged (once-per-CardID spam guard). */
	TSet<FName> WarnedMissingRowIDs;

	/** True after the missing-table warning was logged (once-per-widget spam guard). */
	bool bWarnedMissingTable = false;
};
