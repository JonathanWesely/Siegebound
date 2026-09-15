// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/DeckTypes.h"
#include "DeckBuilderWidget.generated.h"

class UDataTable;
class UDeckSlotEntryWidget;
class UTexture2D;
class USiegeDeckSaveGame;
struct FCardRow;

/**
 *  C++ base for /Game/UI/WBP_DeckBuilder (TASK-118 reparents the UMG duplicate
 *  to this class) — the GDD §7 deck-builder screen: browse the 28-card pool,
 *  add/remove copies (per-card copy caps ABOLISHED, CARD-UNCAP 2026-08-28
 *  UNCAP-§4 — only the exactly-50 deck total binds), a live x/50 counter, the
 *  §8 average-cost readout, and save/load of NAMED decks (SaveGame).
 *
 *  Division of labor (CONVENTIONS "Deck-builder & saved decks (M6)" + the widget
 *  rule): this C++ base owns ALL model/logic; WBP_DeckBuilder is layout + calls.
 *  It NEVER reads DT_Cards, the SaveGame, or the deck itself — everything the
 *  grid needs (card names/costs/caps/art, the collection, the counters, the
 *  legality gate) arrives through the BlueprintCallable/Pure getters here
 *  (UObject/struct returns are allowed on these). The BlueprintImplementableEvents
 *  it fires carry float/int/bool/FString params ONLY (MCP cannot author enum/
 *  struct BP params) — the UCardHandWidget precedent.
 *
 *  Reuses the ONE deck infra (never reinvented): the working deck is an
 *  FDeckList; legality/average-cost come from UDeckLibrary (data-driven from
 *  DT_Cards, §3.0); saved decks persist through USiegeDeckSaveGame's fixed slot.
 *
 *  Seed-then-bind (qa/TASK-005-report.md major 2): WBP_DeckBuilder calls
 *  LoadDefaultDeck() (or LoadDeck) from Event Construct to SEED the working
 *  model, then reads the getters; every mutation re-broadcasts OnDeckModelChanged
 *  ("re-read the getters") so the screen stays correct without binding to any
 *  delegate. No gameplay/combat coupling — this only edits/persists FDeckLists.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UDeckBuilderWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDeckBuilderWidget(const FObjectInitializer& ObjectInitializer);

	// --- The ten-slot model (TASK-670) + the deck bar (TASK-671) ----------------
	// (CONVENTIONS DECK-§1..§5, signatures DECK-§8)
	// The ten fixed decks "deck1".."deck10" (USiegeDeckSaveGame::MakeFixedDeckName)
	// with the active deck (orange outline, next match) persisted in
	// ActiveDeckName and the EDITING selection transient (derived from active on
	// open — the save class gains no field, DECK-§1). Every content mutation
	// auto-saves through the ONE funnel PersistWorkingDeck() (DECK-§4) — there
	// is no manual save in the model's contract.
	// M8: adds no replicated property, no new replicated class, no new
	// relevancy tier, no RPC — client-local UI/model state only.

	/**
	 *  Migrate the save to the ten fixed slots (USiegeDeckSaveGame::
	 *  MigrateToFixedSlots — the ONE shipping call site, DECK-§2), persist iff
	 *  it mutated, then select the ACTIVE deck for editing (DECK-§3: the
	 *  builder opens on the match deck). Runs AFTER Super::NativeConstruct() on
	 *  purpose: Super fires the WBP's Event Construct, so any legacy graph-side
	 *  seed (LoadDefaultDeck) lands BEFORE the model init and is overwritten by
	 *  it — and cannot auto-save, because no editing slot is selected yet (the
	 *  PersistWorkingDeck guard). The TASK-671 deck-bar build then lands LAST:
	 *  ten UDeckSlotEntryWidget entries into DeckBar (null container ⇒ one
	 *  Warning, bar skipped, everything else works — DECK-§5), followed by one
	 *  RefreshDeckBarStates so the bar opens showing the true active/editing
	 *  states.
	 */
	virtual void NativeConstruct() override;                          // migrate -> persist if mutated -> select active for edit -> build bar

	/**
	 *  Left-click meaning (D7, DECK-§3): load fixed slot SlotIndex (0-based)
	 *  into the working deck for EDITING (an empty slot ⇒ an empty working
	 *  deck stamped with the fixed name). Out-of-range is refused (logged).
	 *  No disk write — selection of the editing slot is transient; content
	 *  mutations persist via the funnel, and the ACTIVE deck choice belongs to
	 *  SetActiveDeckBySlot. Fires OnDeckModelChanged (re-read the getters).
	 */
	UFUNCTION(BlueprintCallable, Category="Siegebound|Deck") void  SelectDeckForEdit(int32 SlotIndex);

	/** The 0-based fixed slot currently loaded for editing; INDEX_NONE only before NativeConstruct has run. */
	UFUNCTION(BlueprintPure,     Category="Siegebound|Deck") int32 GetEditingDeckIndex() const;

	/**
	 *  Right-click meaning (DECK-§3): mark fixed slot SlotIndex as the ACTIVE
	 *  deck the next match uses. Delegates to the EXISTING strict SetActiveDeck
	 *  path (kept byte-compatible, DECK-§4) — deck-exists check, the TASK-1270
	 *  legality gate, ACC-§4 call-time slot seam, persist, OnDeckModelChanged —
	 *  never reimplemented. Out-of-range is refused (logged). An ILLEGAL saved
	 *  deck is refused too (TASK-1270): the orange rim does not move and
	 *  ActiveDeckName is not written — see SetActiveDeck.
	 */
	UFUNCTION(BlueprintCallable, Category="Siegebound|Deck") void  SetActiveDeckBySlot(int32 SlotIndex);

	/**
	 *  TASK-1270 (DECK-§3 rider) — THE ACTIVATION GATE, the in-memory half of
	 *  SetActiveDeck, factored out so it is assertable on STATE (SC-§104)
	 *  without a widget, a world, a GameInstance or the player's real slot:
	 *    (1) find the saved deck named Name in Save.SavedDecks (case-insensitive
	 *        — the shipped M6 idiom); none ⇒ false, OutRefusalReason names it;
	 *    (2) run UDeckLibrary::IsDeckLegal (THE one legality home — reused,
	 *        never duplicated) on THAT deck against CardTable; illegal ⇒ false,
	 *        OutRefusalReason = IsDeckLegal's OutReason VERBATIM;
	 *    (3) legal ⇒ Save.ActiveDeckName = the deck's canonical stored name,
	 *        OutCanonicalName = that name, returns true.
	 *  On ANY false return Save is untouched (ActiveDeckName unchanged ⇒ the
	 *  rim index GetActiveDeckIndex derives from it is unchanged) and
	 *  OutCanonicalName is empty. Never touches disk — the CALLER persists
	 *  (SetActiveDeck, through the ACC-§4 seam). A null CardTable is illegal by
	 *  IsDeckLegal's own contract, so activation is refused with its reason.
	 *  C++-only (raw pointer / reference params — the UDeckLibrary shape).
	 */
	static bool TryActivateSavedDeck(USiegeDeckSaveGame& Save, const UDataTable* CardTable, const FString& Name, FString& OutCanonicalName, FString& OutRefusalReason);

	/**
	 *  The 0-based fixed slot of the ACTIVE deck (ActiveDeckName resolved via
	 *  FindFixedDeckIndex). 0 (deck1 — the DECK-§3 default) when no save
	 *  exists yet, pre-migration, or the name is not a fixed name — never
	 *  INDEX_NONE, so the builder always opens on a real slot.
	 */
	UFUNCTION(BlueprintPure,     Category="Siegebound|Deck") int32 GetActiveDeckIndex() const;

	/**
	 *  The deck-bar container (TASK-671; DECK-§5/§7): ONE horizontal row at the
	 *  very top of WBP_DeckBuilder, authored THERE by TASK-672 — empty at
	 *  design time, populated here in NativeConstruct with the ten
	 *  UDeckSlotEntryWidget entries in slot order. Optional binding by law
	 *  (the TASK-646 BindWidgetOptional shape): a WBP without the container —
	 *  672 lands in parallel — costs one Warning and the bar only; the rest of
	 *  the builder keeps working. Never a crash.
	 */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UHorizontalBox> DeckBar;   // authored in WBP by TASK-672; null-safe

	// --- Deck editing (mutating) -------------------------------------------------

	/**
	 *  Add one copy of CardID to the working deck. REFUSED (no-op, no broadcast)
	 *  ONLY when the table/row cannot be resolved. CARD-UNCAP 2026-08-28
	 *  (UNCAP-§4): the per-card MaxCopies refusal is DELETED — any count of a
	 *  resolvable card may be added, and deliberately NO add-time deck-total
	 *  guard exists (U4; the x/50 counter + the exactly-50 legality gate carry
	 *  the invariant). On success fires OnDeckSlotCountChanged(CardID, newCount)
	 *  then OnDeckModelChanged(). The WBP greys the "+" at GetCardMaxCopies'
	 *  shim value (50).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void AddCopy(FName CardID);

	/**
	 *  Remove one copy of CardID from the working deck (down to 0; the entry is
	 *  dropped when it hits 0). No-op (no broadcast) when the count is already 0.
	 *  On success fires OnDeckSlotCountChanged(CardID, newCount) then
	 *  OnDeckModelChanged(). Needs no table — removing is always legal.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void RemoveCopy(FName CardID);

	/**
	 *  Seed the working deck from the curated DeckCount default column of
	 *  DT_Cards (the "reset to default" template, M6 ruling 3). Clears the deck
	 *  name (an unsaved working deck). Null-safe: a missing table leaves the deck
	 *  empty (logged once). Fires OnDeckModelChanged().
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void LoadDefaultDeck();

	// --- Deck model reads (const, cheap — DT_Cards is cached after first load) ---

	/** Copies of CardID currently in the working deck (0 if absent). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetCountOf(FName CardID) const;

	/** Total cards in the working deck (the x/50 counter numerator). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetTotalCount() const;

	/** §8 average gold cost of the working deck (display-only guide) via UDeckLibrary. 0 for an empty deck. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	float GetAverageCost() const;

	/** True iff the working deck is a legal exactly-50-card deck via UDeckLibrary::IsDeckLegal (gates "Play with this deck"; per-card caps abolished, CARD-UNCAP 2026-08-28). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	bool IsCurrentDeckLegal() const;

	/** All DT_Cards row names (the 28-card collection) so the WBP builds the browser grid. Empty when the table is missing. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	TArray<FName> GetCollectionCardIDs() const;

	// --- Per-card display resolvers (additive; keep the WBP out of DT_Cards) -----
	// The grid needs each card's name/cost/art (and the GetCardMaxCopies shim
	// value the "+" greys at — 50 since CARD-UNCAP 2026-08-28) to render a cell.
	// Per "C++ base owns ALL logic" + the UCardHandWidget rule (the WBP never
	// reads DT_Cards), these are resolved here, null-safe.

	/** DT_Cards DisplayName for CardID; falls back to the raw CardID string when the row is missing. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FString GetCardDisplayName(FName CardID) const;

	/** DT_Cards gold Cost for CardID (0 when the row is missing). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetCardCost(FName CardID) const;

	/**
	 *  CARD-UNCAP 2026-08-28 (UNCAP-§4) COMPAT SHIM — per-card deck copy caps
	 *  are abolished, but the signature (and its WBP_DeckCardTile caller, which
	 *  greys the "+" when GetCountOf >= this) is pinned. Returns
	 *  SiegeLegalDeckSize (50) for a resolved row — the only per-card bound
	 *  left is the deck size itself — and 0 when the table/row is missing
	 *  (unchanged). No longer reads the row's MaxCopies column (that column is
	 *  the hero-upgrade STACK cap only, UNCAP-§2).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetCardMaxCopies(FName CardID) const;

	/**
	 *  Null-safe card-art resolver (CONVENTIONS "Card artwork (hand UI)"): CardID
	 *  → DT_Cards row → CardArt soft path → loaded UTexture2D. Returns nullptr
	 *  (hide the art, text-only cell) for an empty/unknown CardID, an unset
	 *  CardArt cell, or an unresolvable path (logged once per CardID). Uses
	 *  LoadSynchronous — accepted for these 512x512 UI textures (TASK-079 ruling
	 *  4). BlueprintCallable (not Pure) because it loads: call it once per cell.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	UTexture2D* GetCardArtTexture(FName CardID);

	// --- Card details ("how it works", TASK-268) --------------------------------
	// CONVENTIONS "Deck-builder card details — click-a-card 'how it works'". Purely
	// ADDITIVE: no existing getter, mutation or event changes. The tile's card face
	// calls SelectCardForDetails; the details panel reads GetCardDescription plus the
	// existing GetCardDisplayName / GetCardCost / GetCardArtTexture. The "+"/"−"
	// buttons, the x/50 counter, the average-cost guide and the exactly-50 play gate
	// are untouched by everything in this block.

	/**
	 *  The player-facing "how this card works" body for CardID, GENERATED from the
	 *  DT_Cards row every call — never authored per card (the anti-drift ruling: a
	 *  balance edit to cards.csv updates all 28 descriptions for free). The
	 *  designer-only Notes column is NEVER surfaced.
	 *
	 *  Composition (any line whose source field is 0/None/not applicable is OMITTED):
	 *    identity line "<Type> · Cost <n> gold" (the "Max <n> per deck" clause was
	 *                   DELETED — CARD-UNCAP 2026-08-28, UNCAP-§5; the hero-upgrade
	 *                   rules line still states the stack cap, which stays true)
	 *    (blank)
	 *    stat block   — health, damage (+ splash), attack cadence, range (melee vs
	 *                   homing shot vs instant hit), blind spot, move speed
	 *    (blank)
	 *    rules block  — one plain-English line per applicable clause: the card's role
	 *                   (economy/repair/hero upgrade/structure/tower), Charge, Slayer,
	 *                   Suicide, Swarm, Chain, spawner, spell effect, spell delivery,
	 *                   targeting profile, and castle/building damage scaling.
	 *
	 *  Every magnitude that exists as a DT_Cards column is read from the row (§3.0 —
	 *  never hardcoded); the only literals are the keyword glossary strings in the
	 *  .cpp, and the few magnitudes that live as gameplay UPROPERTY defaults instead
	 *  of CSV columns are mirrored there under the CONVENTIONS glossary-mirror rule.
	 *
	 *  Null-safe: an empty/unknown CardID or a missing table returns an EMPTY string
	 *  (the panel shows its own hint), logged once through the same spam guards the
	 *  rest of this widget uses. Never crashes, never ensures.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FString GetCardDescription(FName CardID) const;

	/**
	 *  Select CardID as the card the details panel is showing and fire
	 *  OnCardDetailsRequested with its string form. An unknown row or NAME_None
	 *  CLEARS the selection instead and still fires (with an EMPTY string) so the
	 *  panel can fall back to its empty-state hint. Deck-neutral: it adds/removes
	 *  nothing and deliberately does NOT fire OnDeckModelChanged (the deck model did
	 *  not change).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SelectCardForDetails(FName CardID);

	/** Clear the details selection and fire OnCardDetailsRequested with an empty string (the panel shows its hint). Deck-neutral. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void ClearCardDetails();

	/** The CardID the details panel is currently showing, or NAME_None when nothing is selected. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FName GetSelectedDetailCardID() const;

	/**
	 *  "Show the details for this card." CardID = the DT_Cards row name as a string,
	 *  EMPTY when the selection was cleared (or the CardID could not be resolved).
	 *  The WBP re-reads GetCardDescription / GetCardDisplayName / GetCardCost /
	 *  GetCardArtTexture off the back of it — it never receives a struct (BIE params
	 *  stay float/int/bool/FString only, the widget rule).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
	void OnCardDetailsRequested(const FString& CardID);

	// --- Saved decks (SaveGame, null-safe) --------------------------------------

	/**
	 *  Save the working deck under Name into USiegeDeckSaveGame (fixed slot,
	 *  overwrite-on-collision by name, case-insensitive — M6 ruling 2). An empty/
	 *  whitespace name is refused (logged). No legality gate here — the §8 guide
	 *  never blocks saving; "Play with this deck" is the only 50-card gate. Fires
	 *  OnDeckModelChanged() on success (the saved-names list changed).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SaveDeckAs(const FString& Name);

	/**
	 *  Load a saved deck by Name into the working deck (case-insensitive match).
	 *  No-op + warn when no SaveGame exists or no deck matches. Fires
	 *  OnDeckModelChanged() on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void LoadDeck(const FString& Name);

	/**
	 *  Names of every saved deck (for the WBP's load list). Empty when no
	 *  SaveGame exists. BlueprintCallable (not Pure) because it reads from disk —
	 *  the WBP should call it once and cache, not re-evaluate a pure node.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	TArray<FString> GetSavedDeckNames() const;

	/**
	 *  Mark a SAVED deck (by Name) as the active deck the next match uses
	 *  (persisted in USiegeDeckSaveGame::ActiveDeckName; TASK-114 reads it).
	 *  STRICT: only activates a deck that actually exists in the SaveGame, so
	 *  ActiveDeckName never dangles — persist the working deck with SaveDeckAs
	 *  FIRST, then SetActiveDeck. No-op + warn when the name has no saved match.
	 *  TASK-1270: ALSO only activates a LEGAL deck (UDeckLibrary::IsDeckLegal,
	 *  via TryActivateSavedDeck) — an illegal one (e.g. 68 cards) is REFUSED:
	 *  one Warning carrying IsDeckLegal's reason verbatim (the builder's
	 *  refused-save idiom), OnDeckActivationRefused fired, ActiveDeckName NOT
	 *  written, the orange rim NOT moved. Before this gate the match would have
	 *  silently dealt the curated default while the rim sat on the illegal
	 *  slot. Fires OnDeckModelChanged() on success only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SetActiveDeck(const FString& Name);

	// --- Widget events (float/int/bool/FString params ONLY — the widget rule) ---

	/**
	 *  "The deck model changed — re-read the getters." Fired after any mutation
	 *  (add/remove/load/save/activate). WBP_DeckBuilder re-reads GetTotalCount
	 *  (x/50), GetAverageCost (§8), IsCurrentDeckLegal (Play gate), and each
	 *  cell's GetCountOf; it may re-read GetSavedDeckNames for the load list.
	 *  Sufficient on its own for a full refresh (seed-then-bind law).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
	void OnDeckModelChanged();

	/**
	 *  Fine-grained per-card signal (fired alongside OnDeckModelChanged on a
	 *  single add/remove) so the WBP can update just that card cell's counter
	 *  without re-reading all 28. CardID = the DT_Cards row name as a string;
	 *  Count = its new copy count in the working deck.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
	void OnDeckSlotCountChanged(const FString& CardID, int32 Count);

	/**
	 *  TASK-1270: "activating DeckName was REFUSED — here is why." Fired by
	 *  SetActiveDeck (and so by the right-click lane SetActiveDeckBySlot) on
	 *  every refusal past the no-save-file check: no saved deck of that name,
	 *  or the deck is illegal (Reason = UDeckLibrary::IsDeckLegal's OutReason
	 *  verbatim, e.g. "Deck has 68 cards — a legal deck is exactly 50 (GDD
	 *  3.4)."). Nothing changed when this fires: ActiveDeckName is unwritten,
	 *  the rim is where it was, OnDeckModelChanged is deliberately NOT fired
	 *  (broadcast-on-success-only). FString params only (the widget rule).
	 *  ⚠️ Unbound in WBP_DeckBuilder as shipped by TASK-1270 (no asset edit
	 *  this wave) — the Warning log line is the refusal's guaranteed surface;
	 *  a WBP binding to this event is the zero-code way to show it on screen.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
	void OnDeckActivationRefused(const FString& DeckName, const FString& Reason);

protected:

	/**
	 *  Card stat table (GDD §3.0) — /Game/Data/DT_Cards, the source for MaxCopies
	 *  (the hero-upgrade STACK cap only — CARD-UNCAP 2026-08-28, UNCAP-§2)/
	 *  Cost/DisplayName/CardArt/DeckCount and the collection row names. Soft,
	 *  resolved null-safe at use time (the UCardHandWidget / UDeckComponent
	 *  precedent). EditDefaultsOnly so a BP can retarget the table without code.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Deck")
	TSoftObjectPtr<UDataTable> CardTableAsset;

private:

	/** The deck currently being edited (in-memory model; every content mutation persists to the editing slot via PersistWorkingDeck — DECK-§4). */
	UPROPERTY(Transient)
	FDeckList WorkingDeck;

	/**
	 *  0-based fixed slot the working deck edits (TASK-670). INDEX_NONE only
	 *  before NativeConstruct selects the active slot — the PersistWorkingDeck
	 *  guard makes any pre-construct mutation (e.g. a legacy WBP Event
	 *  Construct seed) unable to write a slot nobody chose. Transient by
	 *  DECK-§1: the save class gains no field; the only persisted selection is
	 *  ActiveDeckName (DECK-§3), and opening derives editing from active.
	 */
	UPROPERTY(Transient)
	int32 EditingDeckIndex = INDEX_NONE;

	/**
	 *  THE ONE AUTO-SAVE FUNNEL (DECK-§4 / fix 3): persists the working deck to
	 *  its fixed editing slot by calling the EXISTING SaveDeckAs path —
	 *  SaveDeckAs(USiegeDeckSaveGame::MakeFixedDeckName(EditingDeckIndex)) —
	 *  so the ACC-§4 call-time slot seam is inherited, never reimplemented.
	 *  Called from successful AddCopy / RemoveCopy / LoadDefaultDeck (refused
	 *  no-ops save nothing). No editing slot selected ⇒ refuses with one
	 *  Warning (nothing is written to a slot nobody chose).
	 */
	void PersistWorkingDeck();

	/**
	 *  TASK-671 (DECK-§3): redraw the ten bar entries' two states — the ORANGE
	 *  OUTLINE on the ACTIVE slot ONLY (SetOutlineActive), the fill tint on the
	 *  EDITING slot ONLY (SetEditingHighlight) — from the live model (one
	 *  GetActiveDeckIndex read per refresh, plus EditingDeckIndex). Idempotent,
	 *  and a silent no-op when the bar was never built (DeckBar null path /
	 *  offline-test widgets). Called wherever either index can move: the bar
	 *  build in NativeConstruct, SelectDeckForEdit, and SetActiveDeck's success
	 *  path (which also covers SetActiveDeckBySlot and the D8 "Play with this
	 *  deck" activation — the orange follows it).
	 */
	void RefreshDeckBarStates(); // outline = active, fill tint = editing

	/**
	 *  The ten bar entries in slot order (TASK-671), kept so RefreshDeckBarStates
	 *  redraws states without re-creating widgets. Rebuilt (cleared + repopulated)
	 *  on every NativeConstruct; stays empty when DeckBar is unbound.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDeckSlotEntryWidget>> DeckBarEntries;

	/**
	 *  The card the details panel is showing (TASK-268), or NAME_None for "nothing
	 *  selected". Selection state ONLY — it never participates in the deck model,
	 *  legality or persistence.
	 */
	UPROPERTY(Transient)
	FName SelectedDetailCardID;

	/** Index of CardID in WorkingDeck.Cards, or INDEX_NONE. */
	int32 IndexOfCard(FName CardID) const;

	// --- GetCardDescription composers (TASK-268; all row-driven, never per card) ---

	/** Identity line: "<Type> · Cost <n> gold" (the Max-per-deck clause deleted — CARD-UNCAP 2026-08-28, UNCAP-§5). Always emits exactly one line. */
	void AppendIdentityLines(const FCardRow& Row, TArray<FString>& OutLines) const;

	/** Stat block: health / damage (+ splash) / cadence / range / blind spot / move speed — each line omitted when its field is 0 or does not apply to the card's kind. */
	void AppendStatLines(const FCardRow& Row, TArray<FString>& OutLines) const;

	/** Rules block: one glossary line per applicable clause, in the CONVENTIONS composition order. May emit nothing (a plain melee unit has no special rules). */
	void AppendRuleLines(FName CardID, const FCardRow& Row, TArray<FString>& OutLines) const;

	/** DT_Cards, loaded null-safe (missing ⇒ nullptr, logged once). */
	const UDataTable* ResolveCardTable() const;

	/** DT_Cards row for CardID, null-safe (missing table/row ⇒ nullptr, logged once per CardID). */
	const FCardRow* ResolveCardRow(FName CardID) const;

	/** Loads the deck SaveGame from the fixed slot; nullptr when no file exists yet (the normal first-run state — silent). */
	USiegeDeckSaveGame* LoadSaveGame() const;

	/** Loads the deck SaveGame, or creates a fresh one when none exists (for write paths). */
	USiegeDeckSaveGame* LoadOrCreateSaveGame() const;

	/** True after the missing-table warning was logged (once-per-widget spam guard). Mutable — read by const getters. */
	mutable bool bWarnedMissingTable = false;

	/** CardIDs whose missing DT_Cards row was already logged (once-per-CardID). Mutable — set by const ResolveCardRow. */
	mutable TSet<FName> WarnedMissingRowIDs;

	/** CardIDs whose unset/unresolvable CardArt was already logged (once-per-CardID, mirrors UCardHandWidget). */
	TSet<FName> WarnedCardArtIDs;
};
