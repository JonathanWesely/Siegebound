// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"    // TASK-1286: FKey — the card-focus key table (arrows/Enter/Delete/gamepad; ⛔ no letter, no digit)
#include "Types/SlateEnums.h"  // TASK-1286: EUINavigation — the direction parameter of MoveCardFocus
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/DeckTypes.h"
#include "DeckBuilderWidget.generated.h"

class UDataTable;
class UDeckSlotEntryWidget;
class UTexture2D;
class USiegeDeckSaveGame;
// TASK-1423 [MENU-NAV-DECKBUILDER-RELAYER]: the IA_Menu* entry point's two
// collaborators. ⛔ Forward declarations only — this header gains NO Enhanced
// Input include, and the subsystem is CALLED (Register/Unregister + its public
// asset-path constants), ⛔ NEVER edited: USiegeMenuInputSubsystem is
// TASK-1406's / TASK-1409's file.
class UInputAction;
class USiegeMenuInputSubsystem;
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

	// --- TASK-1286: keyboard / gamepad card actions (the KEYBOARD twin of the
	//     "+" / "−" MOUSE buttons — NOT of DECK-§3's right-click, which stays
	//     mouse-only by ruling) --------------------------------------------------
	//
	//  🧑 HIS ASK (2026-09-14, boarded as TASK-1286; his go 2026-09-17): arrows to
	//  move a focus outline across the CARDS, Enter to add a copy, Delete (or
	//  gamepad B) to remove one, same rules as the mouse.
	//
	//  ⭐ THE ROUTE, AND THE MEASUREMENT THAT CHOSE IT (deliverable (0), SC-§101 —
	//  a route asserted without its measurement is the failure this law names):
	//
	//   (a) WBP_DeckCardTile has NO design-time buttons. Its WidgetTree holds ONE
	//       subobject, SizeBox_0; AddBtn / RemoveBtn / Btn_CardFace are BP
	//       variables built by the tile's own Construct graph at runtime, so they
	//       are created at UButton's CDO defaults — and UButton's CDO reads
	//       is_focusable = TRUE (live editor read). No `bIsFocusable` property
	//       name occurs anywhere in WBP_DeckCardTile.uasset / WBP_DeckBuilder.uasset
	//       / WBP_MainMenu.uasset, so nothing unchecks it. ⇒ THE THREE BUTTONS ARE
	//       FOCUSABLE. What is NOT focusable is the tile ROOT (the tile's own CDO
	//       reads is_focusable = FALSE) — that is why the tile is not a focus stop.
	//
	//   (b) ⭐ SObjectWidget::SupportsKeyboardFocus() returns
	//       WidgetObject->NativeSupportsKeyboardFocus() (SObjectWidget.cpp:175-182),
	//       which is `return bIsFocusable;` (UserWidget.cpp:2411-2414) — ASKED LIVE
	//       ON EVERY CALL, never baked into the Slate widget at construction. So
	//       UUserWidget::SetIsFocusable(true) (UserWidget.cpp:2421-2425) takes
	//       effect on an ALREADY-CONSTRUCTED tile. ⛔ The 5.2 deprecation note on
	//       bIsFocusable ("only set at construction and is not modifiable at
	//       runtime", UserWidget.h:1030) is true of UButton — which bakes its flag
	//       into SButton at RebuildWidget — and FALSE of UUserWidget. Do not
	//       "correct" this back.
	//
	//   (c) SWidget::Paint draws the dashed FocusRectangle for any widget with
	//       bCanSupportFocus && SupportsKeyboardFocus() (SWidget.cpp:1746-1751);
	//       bCanSupportFocus defaults TRUE (SWidget.cpp:217) and SObjectWidget::
	//       Construct never clears it; PLATFORM_UI_NEEDS_FOCUS_OUTLINES = 1 on
	//       Windows (HAL/Platform.h:539-540). ⇒ focusing the TILE ROOT wears
	//       Slate's own outline around the WHOLE TILE — "the tile as the focus
	//       unit" literally, and an outline on the tile border rather than one
	//       buried inside card art.
	//
	//  ⇒ ROUTE (0)-A, SLATE-NATIVE: the tile is made focusable IN CODE at focus
	//  time and focused with EFocusCause::Navigation. ⛔ NO IA_DeckAdd /
	//  IA_DeckRemove, ⛔ no new .uasset, ⛔ not even the WBP_DeckCardTile focusable
	//  flag edit the row permitted — this diff is C++ only.
	//
	//  ⛔ WHY NOT THE IA_ ROUTE, on a measured fact and not on taste:
	//  handoffs/TASK-1274-programmer.md §7 measured that BP_MenuGameMode's
	//  FInputModeUIOnly calls GameViewportClient::SetIgnoreInput(true), and
	//  UGameViewportClient::InputKey returns early on IgnoreInput() — so on
	//  L_MainMenu (which hosts this builder) a REAL key press never reaches
	//  Enhanced Input at all. An IA_DeckAdd would have been reachable ONLY by the
	//  verifier's inject_input_action: inert in Jonathan's hands, which is the
	//  opposite of what he asked for. Slate's key route (SObjectWidget::OnKeyDown,
	//  SObjectWidget.cpp:231-239) is unaffected by SetIgnoreInput and is the path
	//  his own arrow keys already ran on this screen (his words, TASK-1274 status).
	//
	//  THE KEY TABLE — ⛔ no letter and ⛔ no digit anywhere, so KBD-§4's remap
	//  table is NEVER entered (it tables all 26 letters and deliberately excludes
	//  digits/punctuation/Enter/Escape BY DESIGN — CONVENTIONS `KBD-§4`, SECOND
	//  BULLET. ⛔ The SECTION is the citation; the line number is corroboration
	//  only. This claim read CONVENTIONS:2519-2522 when the comment was written
	//  and reads CONVENTIONS.md:2528 — ONE line, not a four-line range — on
	//  2026-09-18, three inserts later in that same file on that same day. ⇒ a
	//  `§` does not drift when the file grows; a line number drifts every
	//  time); ⛔ Tab
	//  untouched; every consumed key fires ONLY while a card tile actually holds
	//  Slate focus, so nothing is consumed anywhere else on this screen:
	//    move focus  Left/Right/Up/Down · Gamepad_DPad_* · Gamepad_LeftStick_*
	//    enter grid  Down (the deck bar is a HORIZONTAL row — Down does nothing
	//                there today, so nothing is taken away from DECK-§3's bar)
	//    Accept      Enter · Virtual_Gamepad_Accept · Gamepad_FaceButton_Bottom
	//                                                              → AddCopy
	//    Remove      Delete · Gamepad_FaceButton_Left  (X)       → RemoveCopy
	//    EXIT GRID   Escape · Gamepad_FaceButton_Right (B) · Virtual_Gamepad_Back
	//                                                          → ExitCardGridFocus
	//                ⭐ Escape IS bound — but ONLY here and ONLY while the grid is
	//                live (🧑 his 2026-09-18 A-2 SCOPE ruling, block below)
	//
	//  ⭐ 2026-09-17 AMENDMENT — 🧑 HIS RULING, ON QA's WARN-1/WARN-2
	//  (qa/TASK-1290-report.md). The first cut bound REMOVE to
	//  Gamepad_FaceButton_Right, which is Slate's universal BACK gesture
	//  (EKeys::Virtual_Back = FPlatformInput::GetGamepadBackKey(),
	//  InputCoreTypes.cpp:424 → GenericPlatformInput.h:32-35; mapped to
	//  EUINavigationAction::Back at NavigationConfig.cpp:38 — that symbol is
	//  UE_DEPRECATED(5.7) and the code now spells the SAME FKey
	//  EKeys::Virtual_Gamepad_Back.GetVirtualKey(), TASK-1304 block C) — so B
	//  deleted a card, auto-saved, and there was no way out of the grid at all.
	//  His words: "B stops being destructive and starts meaning Back, which is
	//  what every other screen already does."
	//    · Remove's gamepad half moved B → X. ⛔ The B→Remove binding is DELETED.
	//      X is measured to carry no prior meaning: Slate's whole default table is
	//      NavigationConfig.cpp:19-38 (only Accept and Back) and
	//      Gamepad_FaceButton_Left does not appear in it; a project census returns
	//      0 outside this file, on an instrument proven able to see B.
	//    · EXIT THE GRID is NEW behaviour, and it is a NESTED Back, not a second
	//      meaning: the grid consumes Back IFF a tile really holds focus AND the
	//      exit changed state (ExitCardGridFocus's bool) — otherwise the key falls
	//      straight through. ⇒ once leaves the GRID, again leaves the BUILDER.
	//    · ⚠️ Tab is an INHERITED exit, ⛔ NEVER a bound key — the Tab key literal
	//      appears zero times in this feature, deliberately (the census greps for
	//      it). Tab falls through to Super and Slate's own `Next` navigation walks
	//      the focus off the tile. Documented behaviour, ⛔ never implemented.
	//
	//  ⭐🧑 ESCAPE — BOUND BY A GRANT, NOT BY AN OVERSIGHT. AS-§6 A-2 IS SCOPED.
	//  A-2 (CONVENTIONS ~:789, CLOSED 2026-08-04, cited as project-wide at
	//  DeckSlotEntryWidget.h:60 and at CONVENTIONS:7519/:8316/:8595) names a Slate
	//  FReply::Handled() on EKeys::Escape — and NativeOnPreviewKeyDown explicitly —
	//  as an automatic QA FAIL. TASK-1286's first cut bound Escape here; the
	//  collision was FLAGGED rather than quietly deviated from (SC-§97 forbids an
	//  agent substituting 🧑 his key choice) and amendment 2 (2026-09-17) DROPPED
	//  Escape before this ever compiled. The open half — does A-2 bind a
	//  MAIN-MENU grid? — was boarded for him as TASK-1300.
	//    🧑 HE RULED IT 2026-09-18, verbatim label:
	//         "Scoped — Escape may exit the card grid"
	//       — A-2 was about the assistant console and the in-match cancel routes,
	//       NOT a menu-side grid. Recorded at TASK-1300; written into CONVENTIONS
	//       AS-§6 as the "A-2 SCOPE" bullet (~:797), which SUPERSEDES the struck
	//       "read at its widest" sentence above it (~:796).
	//  ⇒ Escape is restored to the exit gesture at TASK-1304 block B ON HIS
	//    AUTHORITY. ⛔ THE CITATION STAYS: it is now a GRANT record, not a
	//    prohibition record — A-2 is permitted HERE, CONDITIONALLY, and NOWHERE
	//    ELSE. Deleting the pointer would delete the only thing that caught this.
	//  ⛔⛔ THE EDGE OF THE GRANT, AND IT IS NARROW: he scoped A-2 for a CARD
	//    GRID, not for this widget generally. Escape reaches FReply::Handled()
	//    IFF IsCardGridFocusLive() AND ExitCardGridFocus() returned true — the
	//    IDENTICAL predicate the gamepad half already uses. Escape on the deck
	//    bar, on a cold builder or anywhere on L_MainMenu falls straight through,
	//    in the preview pass as well as the bubble. A-2 remains UNRELAXED for the
	//    assistant console and for ASiegePlayerController's in-match cancel
	//    routes (placement, spell targeting, group-pick) — untouched by this.
	//  ⛔ A-2 names Escape ONLY, so the gamepad half above — his headline
	//  "B stops being destructive and starts meaning Back" — ships in full.
	//  ⭐ Consequence, replacing the "no keyboard exit" note this block used to
	//  carry: the keyboard NOW HAS an exit key out of the grid (Escape), beside
	//  Tab's inherited Slate `Next` (above) and a mouse click.
	//
	//  ⛔ Accept and Remove call AddCopy / RemoveCopy — the SAME entry points the
	//  "+" / "−" buttons call — so OnDeckSlotCountChanged and the DECK-§4 auto-save
	//  funnel run unchanged. The "+" / "−" button bodies are NOT touched.

	/**
	 *  Grid-order index of the focused card within GetCollectionCardIDs(), or
	 *  INDEX_NONE when the card grid is not focused. Model state only — it takes
	 *  part in no deck mutation, no legality gate and no persistence.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	int32 GetFocusedCardIndex() const;

	/** The focused card's DT_Cards row name, or NAME_None when nothing is focused (or the index no longer resolves). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
	FName GetFocusedCardID() const;

	/**
	 *  Focus the card at CardIndex in grid order (GetCollectionCardIDs() order,
	 *  which the WBP builds the WrapBox from). Any index outside [0, count) —
	 *  INDEX_NONE included — CLEARS the focus. Best-effort on the visual side:
	 *  when a live WBP_DeckCardTile exists for that card it is made focusable and
	 *  given Slate focus with EFocusCause::Navigation (the dashed outline); with
	 *  no live grid (an offline automation widget) the model still moves, which is
	 *  exactly what the test asserts.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void SetFocusedCardIndex(int32 CardIndex);

	/**
	 *  Step the card focus one tile in Direction, wrapping at BOTH ends (a ring
	 *  over the flat grid order). From "nothing focused" it ARMS: Down/Right on
	 *  the first tile, Up/Left on the last. Up/Down step by the live row width
	 *  (ResolveGridColumns); when that cannot be measured — a WrapBox that has not
	 *  painted yet, or no grid at all — the width is 1 and Up/Down behave as
	 *  Left/Right, which is the honest degradation, never a guessed column count.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void MoveCardFocus(EUINavigation Direction);

	/**
	 *  Accept on the focused tile = EXACTLY what left-clicking its "+" does:
	 *  AddCopy(GetFocusedCardID()). Silent no-op when nothing is focused. Every
	 *  AddCopy rule (the unknown-row refusal, OnDeckSlotCountChanged, the DECK-§4
	 *  auto-save) is inherited, never re-implemented.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void AcceptFocusedCard();

	/**
	 *  Remove on the focused tile = EXACTLY what left-clicking its "−" does:
	 *  RemoveCopy(GetFocusedCardID()). Silent no-op when nothing is focused, and
	 *  RemoveCopy's own remove-at-0 no-op (which saves nothing) is inherited.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	void RemoveFocusedCard();

	/**
	 *  LEAVE THE CARD GRID (🧑 his 2026-09-17 ruling — the gesture behind gamepad
	 *  B / Virtual_Gamepad_Back — AND, since 🧑 his 2026-09-18 A-2 SCOPE ruling
	 *  ("Scoped — Escape may exit the card grid", TASK-1300), the keyboard's
	 *  Escape as well: see the key-table block above for the grant and its edge).
	 *  ⭐ THE ONE ENTRY POINT: the key handler and the
	 *  automation suite both call THIS — there is deliberately no "test variant",
	 *  the same principle that makes Accept/Remove call AddCopy/RemoveCopy.
	 *
	 *  Returns TRUE iff it actually left a focused grid, which is what makes the
	 *  back-precedence ruling enforceable — and, for Escape, what keeps the grant
	 *  inside its scope: HandleCardGridKey returns FReply::Handled() ONLY on a
	 *  true, on BOTH the preview and the bubble pass, so a Back/Escape press that
	 *  changed no state falls through untouched to Slate and to whatever the
	 *  screen already does.
	 *
	 *  After a TRUE, all four of the spec's exit conditions hold:
	 *    (i)   GetFocusedCardIndex() == INDEX_NONE
	 *    (ii)  no card tile holds Slate focus (HasAnyUserFocus /
	 *          HasFocusedDescendants false for every tile CollectCardTiles returns)
	 *    (iii) IsCardGridFocusLive() == false
	 *    (iv)  the Slate focus LANDS somewhere named — the deck-bar entry's own
	 *          SlotButton for EditingDeckIndex, else the first bar entry's, else
	 *          the builder's own root widget. ⛔ Focus is never left nowhere.
	 *  (i) is the suite's assertion; (ii) and (iii) close at the verify leg — see
	 *  the SC-§39 note on the exit test, which is why (iii) is NOT asserted there.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
	bool ExitCardGridFocus();

	/**
	 *  THE PURE RING STEP (the whole of MoveCardFocus's arithmetic, factored out
	 *  so it is assertable with no world, no widget tree and no Slate —
	 *  SC-§104 STATE). Returns the new flat grid index.
	 *
	 *  CardCount <= 0                  ⇒ INDEX_NONE (an empty collection has no focus)
	 *  CurrentIndex outside [0,Count)  ⇒ ARM: Count-1 for Up/Left, 0 otherwise
	 *  Left/Right                      ⇒ ±1,             wrapped modulo CardCount
	 *  Up/Down                         ⇒ ∓/±ColumnsPerRow, wrapped modulo CardCount
	 *  any other EUINavigation         ⇒ CurrentIndex unchanged
	 *
	 *  ColumnsPerRow is clamped into [1, CardCount] — a caller that cannot measure
	 *  the live row width passes 1 and gets a plain 1-D ring.
	 */
	static int32 StepCardFocusIndex(int32 CurrentIndex, int32 CardCount, int32 ColumnsPerRow, EUINavigation Direction);

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

	/**
	 *  TASK-1286: the card-tile class the WBP builds the browser grid from —
	 *  /Game/UI/WBP_DeckCardTile (measured: WBP_DeckBuilder's graph loops
	 *  GetCollectionCardIDs → CreateWidget(WBP_DeckCardTile) → AddChildToWrapBox).
	 *  Used ONLY to recognise a tile while walking the live panel tree, so the
	 *  keyboard focus can land on one. Soft + EditDefaultsOnly, resolved null-safe
	 *  at use time (the CardTableAsset precedent): an unresolvable class costs one
	 *  Warning and the VISUAL half only — the focus model, Accept and Remove keep
	 *  working. ⛔ Never a crash, ⛔ never a hard reference into Content from code.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Deck")
	TSoftClassPtr<UUserWidget> CardTileClass;

	/**
	 *  🚨⭐ TASK-1304 BLOCK A — THE PREVIEW (TUNNEL) DOOR, AND IT IS THE ONE THAT
	 *  MAKES THE ARROWS REACHABLE AT ALL. ⛔ TASK-1286 shipped with only the
	 *  bubble door below and was INERT, because the bubble starts at the FOCUSED
	 *  LEAF (FEventRouter::FBubblePolicy, SlateApplication.cpp:388-406) and
	 *  SWidget::OnKeyDown (SWidget.cpp:415-429) converts an arrow to NAVIGATION
	 *  and returns Handled right there — so no ancestor ever saw a Down.
	 *
	 *  FSlateApplication::ProcessKeyDownEvent tunnels FIRST, root→leaf, over the
	 *  SAME focus path (SlateApplication.cpp:5021-5044; FTunnelPolicy starts at
	 *  index 0 and increments, :347-361), calling OnPreviewKeyDown on every
	 *  enabled widget, and only runs the bubble `if (!Reply.IsEventHandled())`
	 *  (:5046). SWidget::OnPreviewKeyDown is a bare `return FReply::Unhandled();`
	 *  (SWidget.cpp:411-414) — ⛔ the preview pass does NO navigation conversion,
	 *  which is precisely why an ancestor CAN win there and cannot win below.
	 *  SObjectWidget::OnPreviewKeyDown (SObjectWidget.cpp:221-229) forwards to
	 *  UUserWidget::NativeOnPreviewKeyDown (UserWidget.cpp:2500-2503).
	 *
	 *  ⛔ Both doors call the SAME HandleCardGridKey and so consume EXACTLY the
	 *  same keys in exactly the same states — there is no second key table and no
	 *  preview-only behaviour. Everything it does not claim returns Unhandled, so
	 *  Slate's own navigation (the deck bar's Left/Right/Enter, DECK-§3; the main
	 *  menu, which is not even on this focus path) is byte-for-byte unchanged.
	 */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/**
	 *  TASK-1286: THE BUBBLE KEY DOOR for the card grid. Reached through Slate
	 *  (SObjectWidget::OnKeyDown → NativeOnKeyDown, SObjectWidget.cpp:231-239) as
	 *  the key event bubbles from the focused tile up through this widget — a path
	 *  FInputModeUIOnly's SetIgnoreInput does NOT close, unlike Enhanced Input.
	 *  Handles ONLY the key table above, and only in the states documented there;
	 *  everything else falls through to Super so the WBP's own OnKeyDown and
	 *  Slate's default navigation (the deck bar's existing Left/Right/Enter,
	 *  DECK-§3) keep today's behaviour byte-for-byte.
	 *
	 *  ⭐ KEPT, NOT SUPERSEDED, and TASK-1304 measured why: when the BUILDER ITSELF
	 *  is the focused widget (the state AcquireBuilderFocus establishes on open)
	 *  it is the focus path's LEAF, so SObjectWidget::OnKeyDown calls this BEFORE
	 *  it falls through to SCompoundWidget::OnKeyDown (SObjectWidget.cpp:231-239)
	 *  — i.e. before the arrow is turned into navigation. The preview above wins
	 *  first in practice; this door is the leaf-focused builder's route and must
	 *  stay in step with it. Both delegate to HandleCardGridKey so they cannot
	 *  drift apart. ⛔ No automation test drives either door: UUserWidget declares
	 *  both handlers PROTECTED (UserWidget.h:1572 protected:, :1607-1608) and
	 *  widening that for a test would be the test-only seam this class refuses
	 *  everywhere else. ⇒ the routing claim is measured at RUNTIME, never here.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/**
	 *  ⭐ TASK-1307 — THE ONLY MOMENT FROM WHICH THE FOCUS READ-BACK CAN TELL THE
	 *  TRUTH. ⛔ This override exists for the INSTRUMENT and for nothing else: it
	 *  changes WHEN AND HOW WE LOOK, never what happens. It reads no input,
	 *  touches no deck, moves no focus.
	 *
	 *  ⛔ WHY NOT THE NEXT-TICK TIMER THE ROW OFFERED FIRST — MEASURED IN THE
	 *  ENGINE LOOP, NOT ASSUMED. One FEngineLoop::Tick iteration runs, in order:
	 *    (a) GEngine->Tick()                       LaunchEngineLoop.cpp:5859  <- world tick; FTimerManager, so SetTimerForNextTick lands HERE
	 *    (b) ProcessLocalPlayerSlateOperations()   LaunchEngineLoop.cpp:5918  <- ⭐ THE FOCUS FLUSH
	 *    (c) Slate Tick(PlatformAndInput)          LaunchEngineLoop.cpp:5921  <- a mouse click on the menu button runs HERE
	 *    (d) Slate Tick(TimeAndWidgets)            LaunchEngineLoop.cpp:5991  <- ⭐ SObjectWidget::Tick -> NativeTick lands HERE
	 *    (e) GFrameCounter++                       LaunchEngineLoop.cpp:6131  <- ⇒ (a)..(d) of one iteration ALL see the SAME counter
	 *  ⇒ if the builder was opened from a Slate click, the focus op is queued at
	 *  (c) of frame N and is flushed at (b) of frame N+1 — which a next-tick
	 *  timer, firing at (a) of frame N+1, comes in BEFORE. ⛔ The timer route is
	 *  pre-flush in exactly the case a human produces, so it would have shipped
	 *  the same defect in a different frame.
	 *  ✅ (d) of any frame STRICTLY LATER than the construct frame is post-flush
	 *  in EVERY case, because (b) precedes both (c) and (d) within its own
	 *  iteration. That, and only that, is what FocusRequestFrameCounter guards.
	 *
	 *  ⛔ NOT A NEW TICK COST: this widget already ticks. UUserWidget::
	 *  UpdateCanTick sets bCanTick from WidgetBPClass->ClassRequiresNativeTick()
	 *  (UserWidget.cpp:2360), and that flag is `!NativeParent->HasMetaData
	 *  ("DisableNativeTick")` (WidgetBlueprint.cpp:1563) — UDeckBuilderWidget
	 *  carries no such meta, and WBP_DeckBuilder's TickFrequency was read live on
	 *  2026-09-18 as AUTO. ⇒ the tick was already happening; this adds one
	 *  guarded branch while the builder is open, and nothing once it has fired.
	 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 *  ⭐ TASK-1307 — TEARDOWN SAFETY FOR THE DEFERRED READ-BACK, and it is a
	 *  DISARM rather than a cancel. Choosing NativeTick over a timer means there
	 *  is no handle to dangle and no lambda to outlive this UObject — a tick
	 *  cannot be delivered to a destroyed widget — so the crash class the row
	 *  warned about is removed STRUCTURALLY, not cleaned up after.
	 *  ⛔ This override still exists, because a UUserWidget is REUSED across
	 *  RemoveFromParent/AddToViewport: without the disarm, a builder torn down
	 *  before its first post-flush tick would carry a stale armed flag into its
	 *  NEXT open and log a read-back it never requested. ⇒ per-construct state,
	 *  per DECK/TASK-1307 requirement (i), never per class.
	 */
	virtual void NativeDestruct() override;

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

	/**
	 *  TASK-1286: the keyboard/gamepad card focus — an index into
	 *  GetCollectionCardIDs(), INDEX_NONE for "the grid is not focused". Focus
	 *  state ONLY: like SelectedDetailCardID it never participates in the deck
	 *  model, legality or persistence, and it is Transient because a focus ring
	 *  is a per-session cursor, never something a save carries.
	 */
	UPROPERTY(Transient)
	int32 FocusedCardIndex = INDEX_NONE;

	/** True after the unresolvable-CardTileClass warning was logged (once-per-widget spam guard, the bWarnedMissingTable shape). */
	mutable bool bWarnedMissingTileClass = false;

	/**
	 *  ⭐ TASK-1307 — THE ONCE-PER-OPEN ARM. Set by AcquireBuilderFocus (i.e. from
	 *  NativeConstruct, so it re-arms on EVERY open of this instance), cleared by
	 *  the first post-flush NativeTick that consumes it and by NativeDestruct.
	 *  ⛔ Instrument state only: nothing reads it but the read-back, and no
	 *  gameplay, focus or deck behaviour branches on it.
	 *  ⛔ Deliberately NOT a UPROPERTY and NOT Transient-annotated: it is a plain
	 *  per-instance bool with no reflection, serialization or Blueprint surface —
	 *  the bWarnedMissingTileClass shape one field above.
	 */
	bool bFocusReadbackPending = false;

	/**
	 *  ⭐ TASK-1307 — GFrameCounter at the instant the focus request was made.
	 *  ⛔ The guard is `GFrameCounter > FocusRequestFrameCounter`, i.e. STRICTLY
	 *  LATER FRAME, never `>=`: within one FEngineLoop::Tick iteration the world
	 *  tick, the focus flush, the input tick and the widget tick all observe the
	 *  SAME counter (it is incremented once, at LaunchEngineLoop.cpp:6131), so a
	 *  same-frame tick may still be pre-flush. See NativeTick's comment for the
	 *  phase ordering this is derived from.
	 */
	uint64 FocusRequestFrameCounter = 0;

	/**
	 *  ⭐ TASK-1307 — THE LINE THAT ACTUALLY ANSWERS "did the builder end up with
	 *  Slate focus?". Called from NativeTick EXACTLY ONCE per builder open, at a
	 *  moment proven to be after ProcessLocalPlayerSlateOperations has flushed
	 *  the deferred request.
	 *  ⛔ The verdict is POINTER IDENTITY against GetCachedWidget(), never a type
	 *  name: SNew(SObjectWidget, Widget) wraps EVERY UUserWidget (Widget.cpp:975
	 *  and :980), SNew stringifies the type into SetDebugInfo (Declarative
	 *  SyntaxSupport.h:37-38 -> :929), SetDebugInfo assigns TypeOfWidget
	 *  (SWidget.cpp:1400) and GetTypeAsString returns it (SWidget.cpp:1116-1119)
	 *  ⇒ "SObjectWidget" is the type string of every UMG widget in the tree and
	 *  discriminates NOTHING. The type names stay in the line as context beside
	 *  the verdict, never as the verdict.
	 *  ⛔ Log verbosity, never Verbose: one line per open, and an empty log must
	 *  read as "the call site was never reached", never as a quiet pass.
	 */
	void LogPostFlushFocusReadback();

	/** Index of CardID in WorkingDeck.Cards, or INDEX_NONE. */
	int32 IndexOfCard(FName CardID) const;

	// --- TASK-1286 card-focus internals (all null-safe, all no-ops without a live grid) ---

	/**
	 *  Every live WBP_DeckCardTile under this widget, in panel-tree order (which
	 *  IS grid order: the WBP adds them to the WrapBox in GetCollectionCardIDs()
	 *  order). Walks the LIVE panel hierarchy from GetRootWidget() rather than
	 *  this->WidgetTree, deliberately: the tiles are created by the WBP's graph at
	 *  runtime, so they are children of a panel but are NOT entries in any
	 *  design-time widget tree. Descends into a nested UUserWidget's own root, and
	 *  stops AT a tile (a tile's insides are never focus stops of ours). Empty —
	 *  never an error — when the grid does not exist (an offline test widget).
	 */
	void CollectCardTiles(TArray<UUserWidget*>& OutTiles) const;

	/**
	 *  The live tile showing CardID. Matches on the tile's own CardID variable
	 *  (read by reflection, the one fact the tile owns about itself) so the
	 *  outline can never land on a different card than the one Accept would add;
	 *  falls back to FallbackIndex ONLY when no tile exposes a readable CardID.
	 *  nullptr when there is no grid.
	 */
	UUserWidget* FindTileForCard(FName CardID, int32 FallbackIndex) const;

	/** Make the tile at CardIndex focusable and give it Slate focus with EFocusCause::Navigation (the dashed outline). False when there is nothing to focus. */
	bool FocusCardTile(int32 CardIndex);

	/** The live grid's row width, measured from the painted tiles' absolute Y. 1 when unmeasurable — ⛔ never a guessed column count. */
	int32 ResolveGridColumns() const;

	/**
	 *  TASK-1286 (2026-09-17 amendment): where the Slate focus goes when the grid
	 *  is left — spec block (D)(iv)'s "somewhere NAMED", resolved in one place so
	 *  the preference order is readable rather than scattered:
	 *    1. the deck-bar entry for EditingDeckIndex — its own SlotButton, found by
	 *       walking the entry's root (⛔ the member is protected and that class is
	 *       DECK-§5-pinned, so nothing there is touched);
	 *    2. the first non-null deck-bar entry's SlotButton;
	 *    3. this widget's own root widget — the declared fallback.
	 *  Null only when this widget has no root at all, i.e. an offline automation
	 *  widget, which the caller has already handled.
	 */
	class UWidget* ResolveGridExitFocusTarget() const;

	/**
	 *  True when the card grid really holds the focus right now — the model index
	 *  is set AND (when Slate is up and a live tile exists) that tile has the user
	 *  focus or a focused descendant. This is the guard that stops Accept/Remove
	 *  and the arrow keys from stealing a meaning while the focus is on the deck
	 *  bar, the Play button or Exit. With no Slate and no grid (automation) the
	 *  model IS the truth.
	 */
	bool IsCardGridFocusLive() const;

	/** The key table's direction mapping — arrows / D-pad / left stick ONLY. EUINavigation::Invalid for every other key (⛔ no letter, ⛔ no digit). */
	static EUINavigation NavigationFromKey(const FKey& Key);

	/**
	 *  ⭐ TASK-1304 BLOCK A — THE ONE DECISION FUNCTION BEHIND BOTH SLATE DOORS.
	 *  NativeOnPreviewKeyDown (tunnel, root→leaf) and NativeOnKeyDown (bubble,
	 *  leaf→root) both call THIS and nothing else, so the key table exists once
	 *  and the two passes cannot diverge. It is the whole of TASK-1286's handler
	 *  body, moved — ⛔ not rewritten: the gated logic, the guards and the
	 *  precedence are byte-identical, only the two `return Super::...` tails moved
	 *  out to the callers (each Super differs, which is exactly why they did).
	 *
	 *  Returns Handled ONLY for a key the table owns in a state the table names:
	 *    · IsCardGridFocusLive()  ⇒ Accept · Remove · the exit gesture · the four
	 *                               directions (movement inside the grid);
	 *    · NOT live               ⇒ Down ALONE, and only with cards to enter —
	 *                               the deck bar is horizontal (DECK-§3), so this
	 *                               takes nothing Left/Right/Up/Enter ever had;
	 *    · anything else          ⇒ FReply::Unhandled(), so Slate's navigation and
	 *                               the WBP's own graph keep every other key.
	 *  ⛔ The asymmetry above IS the fence on the preview: a preview that claimed
	 *  arrows unconditionally would break the deck-bar and main-menu navigation 🧑
	 *  he confirmed by hand on TASK-1274.
	 *
	 *  ⭐⭐ TASK-1423 [MENU-NAV-DECKBUILDER-RELAYER] — THIS IS NOW THE ONE
	 *  IMPLEMENTATION BEHIND **THREE** DOORS, NOT TWO, AND THE WHOLE POINT OF THAT
	 *  ROW IS THAT THE THIRD DOOR ADDED **NO SECOND KEY TABLE**. The body below is
	 *  byte-identical to what TASK-1304 shipped; the ONLY change is that the FKey
	 *  now ARRIVES as the parameter instead of being extracted from an FKeyEvent on
	 *  the first line — because the third door has no FKeyEvent to extract it from.
	 *    · door 1  NativeOnPreviewKeyDown  (Slate tunnel, root→leaf)  — 🧑 his keys
	 *    · door 2  NativeOnKeyDown         (Slate bubble, leaf→root)  — 🧑 his keys
	 *    · door 3  RouteMenuNavKey         (Enhanced Input IA_Menu*)  — 🤖 the rig
	 *  ⛔ A COPIED KEY TABLE WOULD BE THE DEFECT, NOT THE FIX: two tables WILL
	 *  diverge, and a behaviour duplicated on two layers is exactly how the bug
	 *  this epic removes was born in the first place.
	 */
	FReply HandleCardGridKey(const FKey& Key);

	/**
	 *  ⛔ TASK-1423: THE SLATE ADAPTER, AND IT IS DELIBERATELY ONE LINE. It exists
	 *  so the two shipped Slate doors above keep calling
	 *  `HandleCardGridKey(InKeyEvent)` BYTE-IDENTICALLY — the hand-confirmed Slate
	 *  path is not edited at its call sites, only given a thinner floor.
	 */
	FReply HandleCardGridKey(const FKeyEvent& InKeyEvent);

	/**
	 *  ⭐ TASK-1304 BLOCK A — PUT THIS WIDGET ON SLATE'S FOCUS PATH WHEN IT OPENS.
	 *  ⛔ Without this, nothing else in this feature can run at ALL:
	 *  FSlateApplication::ProcessKeyDownEvent routes a key ONLY along
	 *  SlateUser->GetFocusPath() (SlateApplication.cpp:5015-5017), and TASK-1286
	 *  measured that NativeConstruct and SelectDeckForEdit made ZERO focus calls
	 *  while USiegeMenuInputSubsystem::ApplyInitialFocus refuses this widget BY
	 *  DESIGN (its IsMenuUncovered() gate names the deck builder in its own
	 *  comment, SiegeMenuInputSubsystem.cpp:191-193) ⇒ the builder was never on
	 *  the path and NativeOnKeyDown could not fire on any route.
	 *  ⛔ That subsystem is NOT touched here (it serves the shipped, hand-confirmed
	 *  main menu); this widget acquires its own focus instead.
	 *
	 *  ⚠️ THE OTHER HALF IS AN ASSET PROPERTY, and it is load-bearing:
	 *  FSlateApplication::SetUserFocus walks the path LEAF→ROOT and focuses the
	 *  FIRST widget whose SupportsKeyboardFocus() is true (SlateApplication.cpp:
	 *  3019-3036). SObjectWidget::SupportsKeyboardFocus() returns
	 *  WidgetObject->NativeSupportsKeyboardFocus() (SObjectWidget.cpp:175-182) =
	 *  `return bIsFocusable;` (UserWidget.cpp:2411-2414). With bIsFocusable false
	 *  the walk skips this widget and lands on an ANCESTOR — which is what the
	 *  live CDO read found (WBP_DeckBuilder_C is_focusable = False). TASK-1304
	 *  flips that one property on Content/UI/WBP_DeckBuilder.uasset AND asserts it
	 *  here at runtime, so an instance built before the asset edit still works.
	 *
	 *  Returns true when Slate took the focus this frame; false when the request
	 *  was deferred to ULocalPlayer::GetSlateOperations() (the engine flushes it
	 *  next frame in FEngineLoop::ProcessLocalPlayerSlateOperations,
	 *  LaunchEngineLoop.cpp:5231) or when there is no Slate at all (automation).
	 *  ⛔ A false is NOT a failure — it is the normal NativeConstruct case, because
	 *  the widget is not yet parented into a live window when Construct runs.
	 *
	 *  ⭐🚨 TASK-1307 — WHERE THE ANSWER NOW COMES FROM. This function's own log
	 *  line is PRE-FLUSH by construction and is labelled as such: it is taken in
	 *  the same synchronous moment as the request, so in the deferred case it can
	 *  only ever report the PRE-EXISTING focus holder. It arms
	 *  LogPostFlushFocusReadback(), which NativeTick fires EXACTLY ONCE per open,
	 *  in a frame strictly later than this one — past the flush — and THAT line
	 *  carries the MATCH / NO-MATCH verdict, by pointer identity.
	 *  ⛔ The return expression is UNCHANGED and always compared pointers; only
	 *  the log was ever weaker than the return.
	 */
	bool AcquireBuilderFocus();

	// ═══════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-1423 [MENU-NAV-DECKBUILDER-RELAYER] — THE THIRD DOOR.
	//
	//  ⛔ THE DIAGNOSIS FIRST, BECAUSE IT DICTATES THE SHAPE. TASK-1399 §2 row 2
	//  measured FocusedCardIndex reading -1 → -1 → -1 across TWO injected
	//  IA_MenuDown presses, and TASK-1306 measured the same -1 while the builder's
	//  root demonstrably held Slate focus. The row asked which of the two failures
	//  that is — "the handler ran and computed -1" or "the handler never ran". It
	//  is THE SECOND, and it is provable from the call graph without any runtime:
	//    inject_input_action
	//      → UEnhancedInputLocalPlayerSubsystem::InjectInputForAction
	//      → the PC's UEnhancedInputComponent's action bindings
	//  and there is NO edge from that component into UUserWidget::NativeOnKeyDown
	//  or NativeOnPreviewKeyDown — Slate key routing starts at
	//  FSlateApplication::ProcessKeyDownEvent over SlateUser->GetFocusPath(), a
	//  path an injected ACTION never enters. ⇒ HandleCardGridKey was never called
	//  on that lane, on any run, healthy or broken. ⛔ The logic was never wrong;
	//  ONLY THE ENTRY POINT WAS MISSING.
	//
	//  ⇒ THE FIX IS A RELAY, NOT A REWRITE: bind the six IA_Menu* actions on the
	//  owning player's UEnhancedInputComponent and hand each one the FKey the
	//  SHIPPED table already names. ⛔ No key gains a meaning it did not have, no
	//  rule is re-stated, and Remove's Delete / Gamepad_FaceButton_Left stay
	//  Slate-only because TASK-1408 authored no IA_ action for them (declared, not
	//  smuggled in as a seventh binding).
	//
	//  ⛔ WHAT THIS DOES NOT TOUCH: NativeOnPreviewKeyDown and NativeOnKeyDown are
	//  unchanged at their call sites and unchanged in behaviour, so 🧑 his REAL
	//  arrow keys keep the route TASK-1304 hand-confirmed. That matters more here
	//  than anywhere else in this epic, because BP_MenuGameMode's FInputModeUIOnly
	//  calls SetIgnoreInput(true) ⇒ a real key NEVER reaches Enhanced Input on
	//  L_MainMenu. The two lanes are not redundant; they are DISJOINT, and deleting
	//  either one would ship a screen that works for exactly one of us.
	// ═══════════════════════════════════════════════════════════════════════════

	/**
	 *  Bind the six IA_Menu* actions on the owning player's UEnhancedInputComponent.
	 *  Called from NativeConstruct AFTER the deck bar is built (TASK-1417's measured
	 *  law: arm against the SEEDED tree, never the pre-seed one).
	 *
	 *  ⛔ DEGRADE OPEN, per-action, the TASK-1409 idiom: an IA_ asset that does not
	 *  load makes ITS key inert with one line naming it and never takes the other
	 *  five down with it. ⛔ No Enhanced Input mapping CONTEXT is added here — the
	 *  keys are already mapped in IMC_MainMenu, which USiegeMenuInputSubsystem
	 *  applied at OnWorldBeginPlay; this widget only listens.
	 *  ⛔ It unbinds first, so a second NativeConstruct on a re-added instance
	 *  cannot double-bind (a UUserWidget survives RemoveFromParent/AddToViewport —
	 *  the same reuse fact TASK-1307's disarm already guards against).
	 */
	void BindMenuNavActions();

	/**
	 *  Remove exactly the bindings BindMenuNavActions added, by handle. Called as
	 *  NativeDestruct's opening pair (LIFO against NativeConstruct) — ⛔ BESIDE the
	 *  existing teardown, never instead of it.
	 *  ⚠️ NOT belt-and-braces: the bindings live on the PLAYER CONTROLLER's input
	 *  component, which OUTLIVES this widget. A builder closed without this would
	 *  leave six bindings pointing at a dead UObject on the controller for the rest
	 *  of the session — the exact shape of the leak TASK-1400's timer comment
	 *  describes for the game instance's timer manager.
	 */
	void UnbindMenuNavActions();

	/** IA_MenuUp    → the shipped table's EKeys::Up. */
	void HandleMenuNavUp();
	/** IA_MenuDown  → the shipped table's EKeys::Down (the ENTRY key into the grid). */
	void HandleMenuNavDown();
	/** IA_MenuLeft  → the shipped table's EKeys::Left. */
	void HandleMenuNavLeft();
	/** IA_MenuRight → the shipped table's EKeys::Right. */
	void HandleMenuNavRight();
	/** IA_MenuAccept→ the shipped table's EKeys::Enter (⇒ AcceptFocusedCard ⇒ AddCopy). */
	void HandleMenuNavAccept();
	/**
	 *  IA_MenuBack → the shipped table's EKeys::Gamepad_FaceButton_Right, i.e. the
	 *  EXIT-THE-GRID gesture, and nothing else.
	 *  ⛔ NOT EKeys::Escape. 🧑 His A-2 SCOPE ruling grants Escape to the card grid
	 *  CONDITIONALLY and "nowhere else in this project"; IA_MenuBack is authored on
	 *  Backspace + Gamepad_FaceButton_Right (TASK-1408) and this row does not widen
	 *  that by one key. ⛔ And it does NOT close the BUILDER: this widget implements
	 *  no ISiegeMenuNavCloseTarget (see the (4) declaration in the handoff), so the
	 *  two-press semantics 🧑 he already has are preserved exactly — once leaves
	 *  the GRID, and the second press falls through with the grid unfocused.
	 */
	void HandleMenuNavBack();

	/**
	 *  ⭐ THE RELAY ITSELF — the ONE place the third door meets the one
	 *  implementation. Everything above is six names for this call.
	 *
	 *  ⚠️ IT CARRIES ONE PRECONDITION REPAIR AND THAT REPAIR IS NOT A SECOND KEY
	 *  RULE — say it out loud so a reviewer can hold it to that. HandleCardGridKey
	 *  decides through IsCardGridFocusLive(), which asks SLATE whether the focused
	 *  tile still holds focus. On this lane a competing focus setter can have run
	 *  EARLIER IN THE SAME PRESS: USiegeMenuInputSubsystem binds its own
	 *  IA_MenuDown at OnWorldBeginPlay, i.e. BEFORE this widget's NativeConstruct,
	 *  so its delegate is earlier in the component's array and its MoveFocus(+1)
	 *  runs FIRST. Once this screen is registered, that walker can place the ring
	 *  on one of the builder's OWN UButtons and the model would then disagree with
	 *  Slate for the rest of the press. ⇒ before dispatching, if the MODEL says the
	 *  grid is armed and Slate disagrees, the tile's focus is re-asserted with the
	 *  SAME FocusCardTile(FocusedCardIndex) call AcceptFocusedCard and
	 *  RemoveFocusedCard already make for the WBP-rebuilt-the-grid case. It is the
	 *  house idiom, already shipped, applied at a new moment — ⛔ never a new key,
	 *  never a new state, and ⛔ deliberately NOT added to the Slate doors, whose
	 *  behaviour must stay byte-identical.
	 */
	void RouteMenuNavKey(const FKey& Key, const TCHAR* ActionName);

	/**
	 *  TASK-1423 (1): tell USiegeMenuInputSubsystem that THIS screen owns menu
	 *  navigation (the USettingsMenuWidget / USiegeGraphicsMenuWidget trio, cloned).
	 *
	 *  ⚠️ WHAT IT BUYS HERE IS DIFFERENT FROM WHAT IT BUYS ON SETTINGS, AND THE
	 *  DIFFERENCE IS THE ONE THING A REVIEWER MUST CHECK. On Settings the
	 *  subsystem's ring IS the screen's navigation. Here it is NOT: this screen
	 *  navigates a 2-D grid of nested WBP_DeckCardTile UUserWidgets, which are not
	 *  one of the walker's four admitted classes and are not even reached by it
	 *  (UWidgetTree::ForEachWidget descends through UPanelWidget only). Registration
	 *  is taken for the two things it DOES buy:
	 *    (a) LogNavTargetRetarget prints THIS SCREEN'S focus-stop count — the
	 *        instrument the EVENTGRAPH-CENSUS-GAP ruling (TASKBOARD marker
	 *        EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24) makes binding on every
	 *        remaining screen row. Without a registration there is no count to
	 *        falsify the handoff's stop list against, on any lane;
	 *    (b) it is the seam TASK-1454 needs when it wires a real close target.
	 *  ⛔ AND THE COST WAS DECLARED, NOT HIDDEN — AND IT HAS SINCE BEEN PAID OFF.
	 *  ~~registration turns the subsystem's generic ring ON over a screen that
	 *  drives itself. RouteMenuNavKey's repair above is what makes that
	 *  composition converge on the grid every press; the clean fix is
	 *  subsystem-side and is ESCALATED in the handoff rather than taken
	 *  silently (SC-§50).~~
	 *  ⛔ TASK-1474 (2026-09-25) — struck, not deleted (SC-§120). The escalation
	 *  LANDED: TASK-1471 shipped the subsystem-side fix, and this class's
	 *  RegisterAsMenuNavTarget now calls RegisterSelfDrivingMenuNavTarget
	 *  (DeckBuilderWidget.cpp, the one statement that row changed here). ⇒ the
	 *  generic ring is NOT turned on over this screen: Up/Down move no focus on
	 *  it, Accept presses none of its buttons, Left/Right step none of its
	 *  controls, and registration places no ring on its stop 0. RouteMenuNavKey's
	 *  repair is KEPT as a net, not as the mechanism. ⚠️ What registration still
	 *  buys is (a) and (b) above, unchanged — and (a) got sharper: TASK-1474's
	 *  bounded descent means LogNavTargetRetarget's count now includes the deck
	 *  bar's ten SlotButtons, so the number finally describes the whole screen.
	 */
	void RegisterAsMenuNavTarget();

	/**
	 *  Give menu navigation back. Called as NativeDestruct's FIRST statement.
	 *  ⭐ THIS IS THE LEG 🧑 HIS OWN COMPLAINT RIDES ON — "if you exit the deck
	 *  builder and go back to the main menu, the outline is no longer there."
	 *  TASK-1400's 0.2 s re-entry poll re-arms the main menu, but only while
	 *  IsMenuUncovered() is true, and TASK-1406's UnregisterMenuNavTarget re-places
	 *  focus only when ANOTHER registered screen is taking over. A builder that
	 *  registered and never unregistered would leave the stack holding it.
	 *  ⛔ Reachable even though WBP_DeckBuilder's Exit is BP-opaque: that graph's
	 *  RemoveFromParent(self) releases this widget's Slate resources, which is what
	 *  runs NativeDestruct. ⛔ No Blueprint edit is authored by this row ((4)).
	 *  ⚠️ And there is a NET underneath it, which is why a missed call degrades
	 *  rather than sticks: GetRegisteredNavTarget() re-validates
	 *  IsInViewport() && IsVisible() on EVERY read and skips a removed screen.
	 */
	void UnregisterAsMenuNavTarget();

	/** The world's USiegeMenuInputSubsystem, or null (Editor world / no world). The USettingsMenuWidget::ResolveMenuInputSubsystem shape, cloned. */
	USiegeMenuInputSubsystem* ResolveMenuInputSubsystem() const;

	/**
	 *  Handles of the IA_Menu* bindings this widget added to the owning player's
	 *  input component, so UnbindMenuNavActions removes EXACTLY those and nothing
	 *  else (the component is shared with USiegeMenuInputSubsystem, whose bindings
	 *  ⛔ must survive this widget). Empty ⇒ nothing is bound.
	 *  ⛔ Not a UPROPERTY: uint32 handles carry no object reference (the
	 *  bFocusReadbackPending / FocusRequestFrameCounter shape above).
	 */
	TArray<uint32> MenuNavBindingHandles;

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
