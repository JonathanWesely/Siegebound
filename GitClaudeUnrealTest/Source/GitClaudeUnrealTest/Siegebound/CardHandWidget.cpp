// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CardHandWidget.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DeckComponent.h"
#include "Siegebound/SiegeControlsHelpWidget.h"      // ⭐ THE ONE RESOLVER (CARDBAR-§2): FSiegeControlsHelpRegistry + QueryAppliedKeysForRow
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h" // passed THROUGH to the resolver — ⛔ GetPositionalKey is never called in this file
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"

UCardHandWidget::UCardHandWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// content contract (CONVENTIONS data-driven law): /Game/Data/DT_Cards,
	// soft and resolved null-safe at use time — Cost/DisplayName are read from
	// rows here and NEVER typed into UMG (TASK-029 spec)
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));
}

void UCardHandWidget::InitForController(ASiegePlayerController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::InitForController: null controller — hand UI left unbound."));
		return;
	}

	// Re-targeting (precedent: UCastleHealthBarWidget, TASK-018 flagged
	// decision 5, QA-passed): drop every binding on the previous sources so
	// this widget never receives two update streams.
	if (ObservedController && ObservedController != Controller)
	{
		UnbindObservedSources();
	}

	ObservedController = Controller;
	ObservedDeck = Controller->FindComponentByClass<UDeckComponent>();
	ObservedPlayerState = Controller->GetPlayerState<ASiegePlayerState>();

	if (!ObservedDeck)
	{
		// TASK-023 creates the "DeckComponent" subobject on the controller;
		// missing it is a wiring fault, not a normal state. The hand cannot
		// display without a deck — leave the UMG design-time state alone.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::InitForController: controller '%s' has no UDeckComponent (TASK-023 wiring) — hand slots and preview stay unseeded."),
			*GetNameSafe(Controller));
	}

	if (!ObservedPlayerState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::InitForController: controller '%s' has no ASiegePlayerState — every card renders unaffordable and no gold re-grey will arrive."),
			*GetNameSafe(Controller));
	}

	// SEED FIRST (qa/TASK-005-report.md major 2): push the current slots +
	// preview + affordability now, so the widget is correct even if no
	// broadcast ever arrives after binding. Works in ANY init order: seeded
	// before the deck's BuildAndShuffle, the slots read empty and the build's
	// own broadcasts (hand + forced preview, TASK-022 decision 7) refresh
	// them moments later.
	if (ObservedDeck)
	{
		RefreshAllHandSlots();
		PushNextCardPreview(ObservedDeck->PeekNextCardID());
	}

	// ...THEN bind. AddUniqueDynamic: a repeated InitForController on the
	// same sources can never double-bind (a double-bound widget would push
	// every update twice — and double the refusal messages).
	if (ObservedDeck)
	{
		ObservedDeck->OnDeckHandChanged.AddUniqueDynamic(this, &UCardHandWidget::HandleDeckHandChanged);
		ObservedDeck->OnDeckNextCardChanged.AddUniqueDynamic(this, &UCardHandWidget::HandleDeckNextCardChanged);
	}

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnGoldChanged.AddUniqueDynamic(this, &UCardHandWidget::HandleGoldChanged);
	}

	ObservedController->OnCardRefused.AddUniqueDynamic(this, &UCardHandWidget::HandleCardRefused);

	// TASK-1270 loop 1 — NOW THAT THE REFUSAL CHANNEL HAS A LISTENER, spend any notice the
	// controller has been holding for one (the illegal-active-deck match-start notice).
	// ⚠️ This is the moment the hand is sure to hear it: WBP_HUD creates this widget on its
	// first widget Tick, after the controller's BeginPlay queued the notice and after the
	// load frame's timers ran (qa/TASK-1270-verify.md). AFTER the bind, ⛔ never before —
	// delivered first it would broadcast to nobody, which is the loop-0 defect. A no-op when
	// nothing is held (a legal deck, a re-init, a second hand): the controller clears the
	// notice on delivery, so it shows once per match start.
	ObservedController->DeliverPendingMatchStartNotice();
}

void UCardHandWidget::RequestPlaySlot(int32 SlotIndex)
{
	if (!ObservedController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::RequestPlaySlot(%d): no controller bound — call InitForController first."), SlotIndex);
		return;
	}

	// pure pass-through (TASK-029 spec): the controller owns every refusal —
	// gold, empty slot, match ended — and reports it back through
	// OnCardRefused -> OnCardRefusedMessage
	ObservedController->PlayHandSlot(SlotIndex);
}

void UCardHandWidget::RequestDiscardSlot(int32 SlotIndex)
{
	if (!ObservedController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::RequestDiscardSlot(%d): no controller bound — call InitForController first."), SlotIndex);
		return;
	}

	// §3.6: the 1-gold charge and the 0-gold refusal are the controller's
	ObservedController->DiscardHandSlot(SlotIndex);
}

void UCardHandWidget::HandleDeckHandChanged()
{
	// FOnDeckHandChanged carries no payload (TASK-022 contract): coarse
	// refresh — re-pull ALL slots via the getters. This delegate is the
	// per-play/per-discard heartbeat; the preview delegate is value-filtered
	// and must not be used to infer draws.
	RefreshAllHandSlots();
}

void UCardHandWidget::HandleDeckNextCardChanged(FName NextCardID)
{
	// every preview broadcast is authoritative (TASK-022 flagged decision 7:
	// build/reset paths force-fire even on an unchanged value) — push it
	// unconditionally, no self-filtering here
	PushNextCardPreview(NextCardID);
}

void UCardHandWidget::HandleGoldChanged(int32 NewGold)
{
	// NewGold itself is deliberately unused: PushHandSlot recomputes
	// bAffordable through ASiegePlayerState::CanAfford so the seed pass, the
	// hand-change refresh, and this gold re-grey share ONE code path (the
	// broadcast fires after the mutation, so CanAfford already sees NewGold).
	// §3.5: gold dropping below a card's cost flips that slot's bAffordable
	// on this very broadcast.
	RefreshAllHandSlots();
}

void UCardHandWidget::HandleCardRefused(const FString& Reason)
{
	// 1:1 pass-through — exactly one OnCardRefusedMessage per refused action
	// (TASK-029 acceptance). The ~2 s show-then-hide lives in WBP_CardHand.
	//
	// TASK-1270 loop 1: record the receipt first — C++-readable state, because the
	// BIE below has no C++ body and this is the only offline proof a message reached
	// the hand (GetReceivedRefusalCount / GetLastReceivedRefusal).
	++ReceivedRefusalCount;
	LastReceivedRefusal = Reason;

	OnCardRefusedMessage(Reason);
}

void UCardHandWidget::RefreshAllHandSlots()
{
	if (!ObservedDeck)
	{
		return;
	}

	// GetHandSize() (6, GDD §3.4) keeps this loop in sync with the model
	// instead of double-hardcoding the hand size (qa/TASK-022-report.md
	// ruling 4 — that is exactly what the getter exists for)
	const int32 SlotCount = ObservedDeck->GetHandSize();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		PushHandSlot(SlotIndex);
	}
}

void UCardHandWidget::PushHandSlot(int32 SlotIndex)
{
	if (!ObservedDeck)
	{
		return;
	}

	const FName CardID = ObservedDeck->GetHandCardID(SlotIndex);
	if (CardID.IsNone())
	{
		// empty slot — the NORMAL state in the qa/TASK-021-report.md WARN-2
		// window, never an error (TASK-022 contract). The BIE signal is the
		// EMPTY string: NAME_None.ToString() would be the literal "None",
		// which a BP could never distinguish from a card named None.
		OnHandSlotUpdated(SlotIndex, FString(), FString(), 0, false);
		return;
	}

	// display data comes from the DT_Cards row (GDD §3.0 — never typed into
	// UMG); a missing row degrades to the raw CardID at cost 0, greyed, so
	// the slot stays identifiable on screen while the log names the fault
	FString DisplayName = CardID.ToString();
	int32 Cost = 0;
	bool bAffordable = false;

	if (const FCardRow* Row = ResolveCardRow(CardID))
	{
		DisplayName = Row->DisplayName;
		Cost = Row->Cost;

		// §3.5 "greyed when unaffordable": affordability is cosmetic here —
		// the controller re-checks it authoritatively on RequestPlaySlot
		bAffordable = ObservedPlayerState && ObservedPlayerState->CanAfford(Row->Cost);
	}

	OnHandSlotUpdated(SlotIndex, CardID.ToString(), DisplayName, Cost, bAffordable);
}

void UCardHandWidget::PushNextCardPreview(FName NextCardID)
{
	// cache for GetNextCardArtTexture BEFORE the BIE fires (TASK-079):
	// OnNextCardUpdated carries no CardID (byte-identical BIE law), so the
	// BP's handler pulls the preview art through that getter — which must
	// already see the card being pushed. NAME_None is cached too, so the
	// getter goes null (and the BP hides the art) in the empty-deck window.
	LastNextCardID = NextCardID;

	if (NextCardID.IsNone())
	{
		// no next card anywhere (empty-deck window) — empty DisplayName tells
		// the BP to hide the preview slot
		OnNextCardUpdated(FString(), 0);
		return;
	}

	FString DisplayName = NextCardID.ToString();
	int32 Cost = 0;

	if (const FCardRow* Row = ResolveCardRow(NextCardID))
	{
		DisplayName = Row->DisplayName;
		Cost = Row->Cost;
	}

	OnNextCardUpdated(DisplayName, Cost);
}

void UCardHandWidget::UnbindObservedSources()
{
	if (ObservedDeck)
	{
		ObservedDeck->OnDeckHandChanged.RemoveDynamic(this, &UCardHandWidget::HandleDeckHandChanged);
		ObservedDeck->OnDeckNextCardChanged.RemoveDynamic(this, &UCardHandWidget::HandleDeckNextCardChanged);
	}

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnGoldChanged.RemoveDynamic(this, &UCardHandWidget::HandleGoldChanged);
	}

	if (ObservedController)
	{
		ObservedController->OnCardRefused.RemoveDynamic(this, &UCardHandWidget::HandleCardRefused);
	}

	ObservedDeck = nullptr;
	ObservedPlayerState = nullptr;
	ObservedController = nullptr;
}

UTexture2D* UCardHandWidget::GetCardArtTexture(const FString& CardID)
{
	if (CardID.IsEmpty())
	{
		// empty slot — the NORMAL state (mirrors PushHandSlot's empty-CardID
		// contract): no art, no log. The BP hides the art image and keeps the
		// text-only face hidden with the rest of the empty frame.
		return nullptr;
	}

	const FName CardName(*CardID);
	if (CardName.IsNone())
	{
		// defensive: a literal "None" string can never be a real CardID (the
		// empty-slot signal is the EMPTY string, TASK-029 contract) — treat it
		// as empty, silently, rather than warming the missing-row warning
		return nullptr;
	}

	return ResolveCardArtTexture(CardName);
}

UTexture2D* UCardHandWidget::GetNextCardArtTexture()
{
	if (LastNextCardID.IsNone())
	{
		// nothing pushed yet, or the empty-deck window (PushNextCardPreview
		// cached NAME_None) — hide the preview art
		return nullptr;
	}

	return ResolveCardArtTexture(LastNextCardID);
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  THE KEY-LABEL PULL SEAM  (TASK-807; `CARDBAR-§2` + `CARDBAR-§3`)
// ════════════════════════════════════════════════════════════════════════════════════════
//
//  ⛔⛔ THE ONE THING TO KNOW BEFORE EDITING EITHER FUNCTION BELOW, AND IT IS INVISIBLE ON
//      EVERY MACHINE THIS PROJECT IS DEVELOPED ON:
//
//      `USiegeKeyboardLayoutSubsystem::GetPositionalKey` MUST NOT BE CALLED ON THIS PATH.
//
//  It is wrong TWICE. (i) `KBD-§4` deliberately excludes DIGITS from the translation table
//  (SiegeKeyboardLayoutStatics.cpp:57-63 tables the 26 LETTERS and nothing else), so it would
//  buy nothing for `1`..`6` anyway — "the hotkeys weren't remapped" is answered by that clause
//  and is ⛔ not a defect. (ii) `IA_Card1..6` are Enhanced-Input-MAPPED, and the layout
//  subsystem rewrites the APPLIED IMC duplicate's `.Key` fields wholesale
//  (SiegeKeyboardLayoutStatics.cpp:236, applied at HeroCharacter.cpp:271-275) — so
//  QueryKeysMappedToAction's answer IS ALREADY TRANSLATED. A second call applies the map
//  twice: on US-Dvorak `F` -> `U` -> `G`, printing a key that is bound to nothing. That is
//  `HELP-§1`'s named exception, recorded at SiegePlayerController.h:1222-1226.
//
//  ⭐ SO THE LABEL RIDES THE SHIPPED RESOLVER, ⛔ NOT A SECOND ONE: the same
//  QueryAppliedKeysForRow -> ResolveRowDisplayKeys -> ComposeKeyChipLabel chain the TAB
//  controls screen's `Cards.Play` row already renders. ⭐ ONE resolver ⇒ the card bar and the
//  help screen can never disagree, and a rebind of IMC_Hero moves BOTH with zero code edits.
//
//  ⚠️ Guarded by Tests/SiegeCardHandKeyLabelTest.cpp test 2, which drives a LETTER through the
//  applied lane on a simulated US-Dvorak map. A digit fixture would pass either way — that
//  test asserts the digit blindness explicitly so nobody "simplifies" it back.

FString UCardHandWidget::ComposeSlotKeyLabel(
	int32 SlotIndex,
	const USiegeKeyboardLayoutSubsystem* LayoutSubsystem,
	TFunctionRef<TArray<FKey>(const FSiegeControlsHelpAction&)> AppliedKeyProvider)
{
	// ⛔ ONE ROW, NAMED ONCE. Function-local static so the FName never participates in
	// static-init order (the SiegeKeyboardLayoutStatics.cpp:22-30 caution, applied even where
	// it is not strictly required).
	static const FName CardsPlayRowId(TEXT("Cards.Play"));

	const FSiegeControlsHelpAction* const PlayRow = FSiegeControlsHelpRegistry::FindAction(CardsPlayRowId);
	if (PlayRow == nullptr)
	{
		// The registry row was renamed or removed. Degrade-open: the chip disappears, the bar
		// keeps working, and GetSlotKeyLabel says so once per slot.
		return FString();
	}

	// ⛔ RANGE CHECKED AGAINST THE SHIPPED ROW, ⛔ never against a typed 6: the hand size and
	// the action list are one fact, and this way they cannot drift apart. Checked BEFORE the
	// provider runs, so an out-of-range slot never reaches Enhanced Input at all.
	if (SlotIndex < 0 || !PlayRow->Actions.IsValidIndex(SlotIndex))
	{
		return FString();
	}

	// ⭐⭐ COPIED AND NARROWED, ⛔ NOT REBUILT — and the difference is load-bearing. Copying
	// carries `Lane`, `bLiteralKeyLabel` and `bPointerOnly` over from the ONE row the TAB
	// screen resolves, so a future edit to Cards.Play moves the card bar with it. A
	// hand-built synthetic row would silently stop tracking on the first such edit, which is
	// the exact "second copy of the truth" failure `CARDBAR-§2` exists to prevent.
	FSiegeControlsHelpAction SlotRow = *PlayRow;
	SlotRow.Actions = { PlayRow->Actions[SlotIndex] };
	SlotRow.QwertyReferenceKeys = PlayRow->QwertyReferenceKeys.IsValidIndex(SlotIndex)
		? TArray<FKey>{ PlayRow->QwertyReferenceKeys[SlotIndex] }
		: TArray<FKey>();

	// ⛔ THE PLACEHOLDER FENCE, HALF ONE — STRUCTURAL. ComposeKeyChipLabel answers the pointer
	// affordance ("Mouse click") for a pointer-only row REGARDLESS of the keys it is handed,
	// and it is right to: a TAB row with no key must say so out loud (`HELP-§2` mechanism 2).
	// A CARD CHIP must not — `CARDBAR-§3` says empty, and the WBP hides it. Refusing the lane
	// here means that prose can never leak onto the bar, whatever Cards.Play becomes later.
	if (SlotRow.bPointerOnly || SlotRow.Lane != ESiegeInputLane::MappedAction)
	{
		return FString();
	}

	// ⛔⛔ THE ONE TRANSLATION, AND IT HAPPENED BEFORE THIS LINE. The provider returns
	// QueryKeysMappedToAction's answer over the ACTIVE (already-retargeted) context;
	// ResolveRowDisplayKeys' Lane-A primary branch hands it through VERBATIM with zero
	// GetPositionalKey calls. When nothing maps the action, that same call takes its single
	// fallback translation over the reference key — which is the FIRST translation, not a
	// second one, because nothing had touched that key.
	const TArray<FKey> AppliedKeys = AppliedKeyProvider(SlotRow);
	const TArray<FKey> DisplayKeys =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(SlotRow, AppliedKeys, LayoutSubsystem);

	if (DisplayKeys.Num() == 0)
	{
		// Nothing mapped AND no reference key. ⛔ Never a guessed digit (`CARDBAR-§3`).
		return FString();
	}

	const FString Composed = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(SlotRow, DisplayKeys).ToString();

	// ⛔ THE PLACEHOLDER FENCE, HALF TWO — DERIVED, ⛔ NEVER TYPED. Ask the SAME composer what
	// it says when it has nothing ("(not bound)"), and refuse exactly that answer. Typing the
	// string here would be a second copy of a constant that lives in
	// SiegeControlsHelpWidget.cpp and would rot the moment it is reworded; deriving it cannot.
	// ⚠️ Structurally unreachable today (an FKey's ToString is never empty), and it stays
	// because "empty on every fault" is then a property of the code rather than of an argument.
	const FString EmptyAnswer =
		FSiegeControlsHelpRegistry::ComposeKeyChipLabel(SlotRow, TArray<FKey>()).ToString();

	return (Composed == EmptyAnswer) ? FString() : Composed;
}

FString UCardHandWidget::GetSlotKeyLabel(int32 SlotIndex)
{
	// ⛔ RESOLVED EVERY CALL, ⛔ NEVER CACHED — `KBD-§0` ruling 2's shape: a cached key string
	// is how a label survives a rebind or a layout change and starts lying. Cloned from
	// USiegeControlsHelpWidget::ResolveKeyboardLayoutSubsystem (SiegeControlsHelpWidget.cpp:
	// 2890-2907) rather than re-invented. A null subsystem is `KBD-§5`'s FAIL-SAFE, ⛔ not an
	// error: the label degrades to the reference key unchanged.
	const UWorld* const World = GetWorld();
	UGameInstance* const GameInstance = World ? World->GetGameInstance() : nullptr;
	const USiegeKeyboardLayoutSubsystem* const LayoutSubsystem =
		GameInstance ? GameInstance->GetSubsystem<USiegeKeyboardLayoutSubsystem>() : nullptr;

	const ASiegePlayerController* const Controller = ObservedController.Get();

	const FString Label = ComposeSlotKeyLabel(SlotIndex, LayoutSubsystem,
		[Controller](const FSiegeControlsHelpAction& SlotRow)
		{
			// ⛔ THE SHIPPED LIVE QUERY, UNWRAPPED AND UNASSISTED. It is null-safe by contract
			// (no controller / no local player / no subsystem ⇒ EMPTY, never a crash), and it
			// calls GetPositionalKey ZERO times — that is what makes the key arriving here
			// already-translated-exactly-once.
			return USiegeControlsHelpWidget::QueryAppliedKeysForRow(SlotRow, Controller);
		});

	if (Label.IsEmpty() && !WarnedKeyLabelSlots.Contains(SlotIndex))
	{
		// ONCE PER SLOT, ⛔ never per call: the WBP calls this from its OnHandSlotUpdated
		// handler and every slot re-pushes on EVERY gold tick, so a per-call log is a flood.
		// Same guard shape as WarnedCardArtIDs.
		WarnedKeyLabelSlots.Add(SlotIndex);

		// ⛔ THE SLOT INDEX IS PRINTED RAW AND NOTHING IS DERIVED FROM IT. An earlier draft
		// printed "IA_Card%d" from SlotIndex + 1, which is signed-overflow UB on MIN_int32 —
		// a diagnostic must not be the one line in the function that can misbehave on the
		// input it exists to report.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget::GetSlotKeyLabel: slot %d has no resolvable key — the slot is out of range, or no active context maps its IA_Card action and the Cards.Play row carries no reference key for it. The chip is hidden (logged once per slot)."),
			SlotIndex);
	}

	return Label;
}

UTexture2D* UCardHandWidget::ResolveCardArtTexture(FName CardID)
{
	const FCardRow* Row = ResolveCardRow(CardID);
	if (!Row)
	{
		// missing table/row — already logged once by ResolveCardRow; the face
		// stays text-only (graceful fallback, CONVENTIONS "Card artwork
		// (hand UI)")
		return nullptr;
	}

	if (Row->CardArt.IsNull())
	{
		// unset CardArt cell — graceful text-only fallback, logged once per
		// CardID (same spam rationale as the row warnings: slots re-pull art
		// on every hand/gold refresh)
		if (!WarnedCardArtIDs.Contains(CardID))
		{
			WarnedCardArtIDs.Add(CardID);
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UCardHandWidget: DT_Cards row '%s' has no CardArt set — card face stays text-only (logged once per CardID)."),
				*CardID.ToString());
		}
		return nullptr;
	}

	// LoadSynchronous is ACCEPTED for this feature (TASK-079 ruling 4):
	// 512x512 UI textures, at most 7 visible (6 hand slots + preview), loaded
	// on hand refresh — no async streaming machinery.
	UTexture2D* ArtTexture = Row->CardArt.LoadSynchronous();
	if (!ArtTexture)
	{
		// unresolvable path (e.g. T_CardArt_* not imported yet — normal until
		// TASK-078 lands) — text-only fallback, logged once per CardID, never
		// a crash
		if (!WarnedCardArtIDs.Contains(CardID))
		{
			WarnedCardArtIDs.Add(CardID);
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UCardHandWidget: CardArt '%s' for CardID '%s' failed to load — card face stays text-only (logged once per CardID)."),
				*Row->CardArt.ToString(), *CardID.ToString());
		}
		return nullptr;
	}

	return ArtTexture;
}

const FCardRow* UCardHandWidget::ResolveCardRow(FName CardID)
{
	const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
	if (!CardTable)
	{
		// logged ONCE per widget: slots re-push on every gold change (~1/s
		// income tick), so a per-lookup warning would spam the log
		if (!bWarnedMissingTable)
		{
			bWarnedMissingTable = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("UCardHandWidget: card table '%s' not found — slots render raw CardIDs at cost 0, greyed (logged once)."),
				*CardTableAsset.ToString());
		}
		return nullptr;
	}

	const FCardRow* Row = CardTable->FindRow<FCardRow>(CardID, TEXT("UCardHandWidget::ResolveCardRow"), /*bWarnIfRowMissing*/ false);
	if (!Row && !WarnedMissingRowIDs.Contains(CardID))
	{
		// once per unique CardID, same spam rationale as the table warning
		WarnedMissingRowIDs.Add(CardID);
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("UCardHandWidget: no DT_Cards row for CardID '%s' — slot renders the raw CardID at cost 0, greyed (logged once per CardID)."),
			*CardID.ToString());
	}

	return Row;
}
