# TASK-022 Handoff — Deck & hand model: UDeckComponent

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no board edits — per M2 dispatch gates while Jonathan is away)

## Files created (the complete change set)

1. `Source/GitClaudeUnrealTest/Siegebound/DeckComponent.h` — UDeckComponent (UActorComponent), delegates FOnDeckHandChanged / FOnDeckNextCardChanged, full spec API.
2. `Source/GitClaudeUnrealTest/Siegebound/DeckComponent.cpp` — implementation.

No other file touched. CardRow.h and Castle.* read-only upstream contracts untouched; no Build.cs edit needed (UDataTable is in the Engine module, already a dependency since M1 — verified by SiegePlayerController's existing DT_Cards use).

## What it does (GDD §3.4)

Draw pile + discard pile + fixed 6-slot hand of CardIDs (FNames = DT_Cards row names). `BuildAndShuffle()` reads every row of `/Game/Data/DT_Cards` (soft path `/Game/Data/DT_Cards.DT_Cards`, LoadSynchronous, null-safe), adds `DeckCount` copies of each row's CardID (TASK-021 data: 12/10/10/8/6/4 = 50), logs an Error if the total != 50 but proceeds, Fisher-Yates-shuffles, deals exactly 6. Playing/discarding moves the slot's card to the discard pile and immediately draws its replacement into the same slot. When the draw pile empties and the discard has cards, the discard is shuffled into a new draw pile **eagerly** (see flagged decision 1). `ResetDeck()` = full rebuild + reshuffle + redeal (delegates to BuildAndShuffle; re-reads the table, so a TASK-031 reimport takes effect on the next Play Again). NO gold logic anywhere in the component (controller owns spend/refund).

Empty-deck grace (qa/TASK-021-report.md WARN-2 window — DT_Cards has DeckCount=0 on every row until the post-sign-off TASK-031 reimport): build logs one Error naming WARN-2/TASK-031, deals a hand of 6 `NAME_None` slots, and every getter/play/discard no-ops gracefully. Nothing crashes, nothing asserts. Missing table entirely = same grace with its own Error line.

## Exact contract for downstream tasks

### Signatures (all on UDeckComponent, category "Siegebound|Deck")

```cpp
void  BuildAndShuffle();                    // BlueprintCallable
FName GetHandCardID(int32 Slot) const;      // BlueprintPure; NAME_None = empty slot; out-of-range logs + NAME_None
FName PeekNextCardID() const;               // BlueprintPure; ALWAYS the actual next draw; NAME_None = no next card
bool  ConfirmPlayFromHand(int32 Slot);      // BlueprintCallable; false = refused (out-of-range / empty slot)
bool  DiscardFromHand(int32 Slot);          // BlueprintCallable; identical pile movement; false = refused
int32 GetDrawPileCount() const;             // BlueprintPure
int32 GetDiscardPileCount() const;          // BlueprintPure
int32 GetHandSize() const;                  // BlueprintPure; returns HandSize (6) — see flagged decision 4
void  ResetDeck();                          // BlueprintCallable; full rebuild+reshuffle+redeal
```

Delegates (UPROPERTY(BlueprintAssignable)):
- `FOnDeckHandChanged OnDeckHandChanged` — no params; fired exactly ONCE per successful mutation (initial deal, each play, each discard, each reset). Consumers re-read all slots via GetHandCardID.
- `FOnDeckNextCardChanged OnDeckNextCardChanged` — `(FName NextCardID)`; fired when the preview VALUE changes, plus unconditionally on build/reset paths.

Broadcast order within one mutation: hand first, then preview.

### TASK-023 (controller hand-play) consumes

- `CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"))` in the ASiegePlayerController constructor; `#include "Siegebound/DeckComponent.h"`. Component does NOT self-build at BeginPlay — call `BuildAndShuffle()` at match start and `ResetDeck()` from the PlayAgain flow (per your spec).
- `PlayHandSlot(Slot)`: read `GetHandCardID(Slot)`; `NAME_None` means empty slot — refuse (your DT_Cards row lookup on NAME_None fails anyway, same refusal path). On placement CONFIRM call `ConfirmPlayFromHand(Slot)`; return value may be ignored (you already validated the slot at entry — but note the slot can only have changed if something else mutated the hand between entry and confirm, so checking the bool costs nothing and is safer).
- `DiscardHandSlot(Slot)`: **check `GetHandCardID(Slot).IsNone()` BEFORE `SpendGold(1)`** — your spec's literal order (spend, then discard) would leak 1 gold on an empty slot in the WARN-2 window. Alternatively spend first and refund when `DiscardFromHand` returns false. Either closes the leak; pick one and note it in your handoff.
- Refusal messaging (OnCardRefused, reasons) is entirely yours; the component only logs.

### TASK-029 (hand widget base) consumes

- Seed-then-bind (CONVENTIONS law): in `InitForController`, first read `GetHandCardID(0..GetHandSize()-1)` + `PeekNextCardID()` and push the BIEs, THEN `AddDynamic` to `OnDeckHandChanged` and `OnDeckNextCardChanged`.
- `OnDeckHandChanged` carries no payload — on each fire, re-read all 6 slots and re-push `OnHandSlotUpdated` for each.
- Do NOT infer draws from `OnDeckNextCardChanged`: it is value-filtered (drawing a Footman while the next card down is also a Footman fires nothing). Bind both delegates; the hand delegate always fires per play/discard.
- `NAME_None` slot = empty (render an empty frame / hide the card face); `NAME_None` preview = no next card. Both are the normal state in the WARN-2 window, so the widget must not treat them as errors.
- Cost/DisplayName lookups from DT_Cards rows are your job; the component hands you CardIDs only.

### TASK-030 (placement v2) consumes

- `ConfirmPlayFromHand(Slot)` on the generalized confirm path, after SpendGold — same call TASK-023 wires; nothing extra needed from this component.

## Flagged decisions — QA must rule on each

1. **Eager reshuffle** (the moment the draw pile empties while the discard has cards — inside `DrawNextCard`, checked both before and after the pop) instead of lazy reshuffle-on-next-draw. Rationale: the spec's preview law ("PeekNextCardID must always be the ACTUAL next draw") is unsatisfiable with lazy reshuffle — a const Peek over an empty draw pile with a full discard pile would have to either lie (NAME_None) or mutate. Eager reshuffle yields the invariant "draw pile empty implies discard pile empty", making Peek const, truthful, and trivial. Side effect: GetDrawPileCount/GetDiscardPileCount jump at the reshuffle moment (44/0 style), exactly like a physical deck.
2. **"Owns three piles + hand" implemented as TWO piles (draw, discard) + the hand.** GDD §3.4 defines exactly two piles; no third container exists in the design (I read the spec's "three" as counting the hand collection or as a slip). No cached "built deck" list either — ResetDeck re-reads the table, which is strictly better (picks up reimported balance data on Play Again).
3. **`ConfirmPlayFromHand` / `DiscardFromHand` return `bool`** (true = pile movement happened). The spec is silent on returns; TASK-023's written call sites may ignore the value, but the bool gives the controller a no-cost way to detect the empty-slot refusal (closes the 1-gold discard leak in the WARN-2 window, see contract above).
4. **`GetHandSize()` added beyond the names-block function list** (BlueprintPure, returns the HandSize UPROPERTY). Keeps TASK-029's slot loop in sync with the actual hand size instead of double-hardcoding 6. Additive only.
5. **HandSize=6 and ExpectedDeckSize=50 are EditDefaultsOnly UPROPERTYs** with GDD §3.4 citations, per the CONVENTIONS mechanic-rule registry ("hand of 6", "a deck is exactly 50 cards" are rules, not CSV columns — same treatment as miner cap 6). ExpectedDeckSize is used ONLY for the total!=50 error log; the build always proceeds with what the table gives.
6. **One `OnDeckHandChanged` broadcast per mutating operation** — the initial 6-card deal fires ONCE, not six times. The delegate carries no payload and consumers re-read all slots, so per-slot broadcasts would be 6 identical refreshes. Spec text "whenever any hand slot changes" read as per-mutation, not per-slot.
7. **`OnDeckNextCardChanged` is value-filtered** (fires only when the preview FName actually changes), and **forced on build/reset paths** — direct application of the CONVENTIONS delegate law ("every ACTUAL value change and on reset paths"). Consequence documented for TASK-029 (decision 6 + contract above): the hand delegate, not the preview delegate, is the per-draw heartbeat.
8. **Discard-first-then-draw order inside a play/discard**: the outgoing card enters the discard pile BEFORE its replacement is drawn, so it participates in a reshuffle triggered by that same draw. Only observable with degenerate decks smaller than the hand (never with the real 50), but it matches the physical-card rule and the GDD's "played/discarded cards go to a discard pile; when the draw pile empties, the discard pile is shuffled".
9. **Refused mutations (out-of-range slot, empty/NAME_None slot, pre-build calls) log a Warning, return false, change nothing, and broadcast nothing** (CONVENTIONS: never broadcast refused mutations). The empty-slot Warning will appear if anyone mashes 1-6 during the WARN-2 window — intentional and load-bearing, not spam to be removed.
10. **Log severities**: missing table = Error; built total != 50 = Error (spec says "log an error"), with the 0-card case appending an explicit pointer to qa/TASK-021-report.md WARN-2 / TASK-031 so the PIE log self-diagnoses the known window. Reshuffle and successful build = Log. Chosen over the M1 missing-asset Warning convention because DT_Cards has existed since TASK-008 — a miss here is a real fault, not a parallel-dev gap.
11. **Negative DeckCount clamped to 0 with a Warning** — pure defense; TASK-021's audited data has none.
12. **No BeginPlay override / no self-initialization** — the component is inert until TASK-023 calls BuildAndShuffle at match start (its spec owns that timing). Constructor only disables tick and sets the soft table path.
13. **Piles/hand are `UPROPERTY(VisibleInstanceOnly, Transient)`** — inspectable live in PIE for the TASK-040 verification pass, never serialized. `LastPreviewCardID` is a plain member (pure cache).
14. **Shuffle = hand-rolled Fisher-Yates over `FMath::RandRange`** (inclusive bounds, swap-guarded) rather than Algo::RandomShuffle — byte-for-byte reviewable pre-compile and pins the RNG source explicitly.

## Things QA should scrutinize

- The eager-reshuffle invariant proof: `ReshuffleDiscardIntoDrawIfNeeded` is called (a) before the pop and (b) after the pop in `DrawNextCard`, and cards only ever leave the draw pile via `DrawNextCard` — so no code path can end a public call with draw pile empty + discard non-empty. `PeekNextCardID`'s correctness rests entirely on this.
- Acceptance walk-through against §3.4: 50-card build (12/10/10/8/6/4 from data), deal leaves 44; 44 plays later the pile empties mid-`DrawNextCard` and the discard (50-6=44 accumulated... at that point 44 played cards) folds back in; hand never has other than 6 elements after the first build.
- `TArray::Pop()` no-arg call (UE 5.8: resolves to the EAllowShrinking overload; the deprecated bool overload takes an explicit argument, so no ambiguity).
- `UDataTable::ForeachRow<FCardRow>` — validates the row struct and no-ops with an engine-side error on mismatch (extra grace layer if DT_Cards were ever re-pointed at a different struct).
- Not compiled (standing gate — batch compile is TASK-039 post-sign-off). Header is self-contained; only Engine-module types used; no Build.cs change.

## Board note

TASKBOARD.md deliberately not edited (dispatch hard rule; orchestrator owns board writes). Requested status: `in-progress` -> `ready-for-qa`.
