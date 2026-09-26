# TASK-1496 — [DETAIL-SCROLL-BUTTON-REMOVAL] — programmer handoff

**Marker:** `TASK-1496-DETAIL-SCROLL-BUTTON-REMOVAL`
**Date:** 2026-09-26
**Gate:** `TASK-1497` (QA) → 5a → 5b = `TASK-1498` (regression limb)
**Files written:** exactly two —
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeControlsHelpWidget.cpp`
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeControlsHelpWidget.h`

This is a **deliberate removal of committed code**, not a revert of uncommitted work: `TASK-1484` shipped in
`4620f71`. It is written into the file as a removal **with its reason**, never as an undo.

---

## 1. THE THREE PRESERVATIONS — CHECKED ONE BY ONE, BY NAME

### (i) `ApplyActiveView`'s collapse and its trip-wires — **PRESERVED**

`USiegeControlsHelpWidget::ApplyActiveView` is **untouched**. Both branch writes and the index-last ordering
are byte-identical:

```cpp
if (PanelBorder != nullptr)
{
    PanelBorder->SetVisibility(bDetail ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

if (DetailView != nullptr)
{
    DetailView->SetVisibility(bDetail ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
```

The silent failure it guards is unchanged and still real: a focusable stop parked in an inactive
`SWidgetSwitcher` branch is **admitted by the walker but refuses focus** and **swallows the ring** —
`SWidgetSwitcher::ValidatePathToChild` → `return InChild == GetActiveWidget().Get();`.

**Trip-wires after this row: TWO, both intact, neither edited.**

| # | Site | Guards | Fate |
|---|------|--------|------|
| 1 | `USiegeControlsHelpRowWidget::ConstructRowTree` — `ApplyButtonFocusable(RowButton)` (switcher's **LIST** branch) | the collapse | **STAYS — untouched** |
| 2 | `USiegeControlsDetailWidget::ConstructDetailTree` — `ApplyButtonFocusable(BackButton)` (switcher's **DETAIL** branch) | the collapse | **STAYS — untouched** |
| 3 | the flip at `ApplyButtonFocusable(DetailScrollButton)` | the collapse | **GONE — see §2, stated explicitly** |

`TASK-1479` made a missing trip-wire a BLOCKER. Two remain, one per live switcher branch, which is one per
surviving focusable flip.

### (ii) `registered ⟺ bHelpOpen` — **PRESERVED, UNTOUCHED**

The invariant that resolved `TASK-1479`'s WARN-L1 is not touched. No registration edge was added, removed or
moved. `UnregisterAsMenuNavTarget` still has exactly two call sites (`ApplyOpenState(false)` and
`NativeDestruct`); `ShowDetailForAction` and `ReturnToList` still **refresh** rather than end the
registration. The only change in this area is a comment at `ApplyOpenState`'s clause (6), which previously
cited the stop set as `{ BackButton, DetailScrollButton }`; it now records the count going **1 → 2 → 1** and
states in the same breath that **the invariant did not change at either step**.

### (iii) `RowScrollBox->SetScrollWhenFocusChanges(InstantScroll)`, PRE-`Super::RebuildWidget()` — **IDENTIFIED AND LEFT**

I identified it, confirmed what it is, and **left it**.

- Live call: `SiegeControlsHelpWidget.cpp:3075` — `RowScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);`
- It sits in `USiegeControlsHelpWidget::ConstructHelpTree()`, which `RebuildWidget()` calls **before**
  `Super::RebuildWidget()` (`Initialize(); ConstructHelpTree(); return Super::RebuildWidget();`). PRE-`Super`
  confirmed, not assumed.
- **It is the 28-row LIST's scroll-follows-focus — not the detail page, and not scroll machinery for the
  thing I removed.** It exists because the list ring walks off the bottom of 27 rows. Removing it would
  silently break that walk.
- This is exactly the site a removal row mistakes for part of the feature, so I also added a sentence at
  `DetailScrollBox`'s own block naming the distinction, so the next editor does not have to re-derive it.

`DetailScrollBox` itself also **stays**, with all three authored properties byte-for-byte what shipped at
`TASK-707`: `SetAlwaysShowScrollbar(true)`, `SetIsFocusable(false)`, `SetConsumeMouseWheel(WhenScrollingPossible)`.
The human wheel path was never touched by `TASK-1484` and is not touched by this row.

---

## 2. THE THIRD TRIP-WIRE — STATED EXPLICITLY, NOT VANISHED

**The third trip-wire is GONE, deliberately, and this is the sentence that says so.**

`TASK-1484` planted a third wire at the new focusable flip. It read, in substance: *"this button is focusable
on the line below ⇒ this screen's ring is correct only because `ApplyActiveView` collapses the inactive
`ViewSwitcher` branch."* It guarded **exactly one line**: `ApplyButtonFocusable(DetailScrollButton)`.

That flip is what this row removes, so the wire has nothing left to guard and went with it. The removal is
recorded in the file at the block where the button used to be constructed, under its own heading, including
the reason and an explicit instruction not to remove the other two by analogy — because *a trip-wire that
vanishes without a sentence is indistinguishable from one removed by accident.*

**Wires 1 and 2 were not edited.** Their flips are still there.

---

## 3. THE EXEC-SYMBOL PREDICTION — 10 → 9

`.generated.h` is predicted **by exec-symbol set**, never by size and never by sha (both measured unsound
this session: UHT embeds source line numbers).

Measured **before** the edit, from the live
`Intermediate/Build/Win64/UnrealEditor/Inc/GitClaudeUnrealTest/UHT/SiegeControlsHelpWidget.generated.h`:

```
execCloseHelp            execIsDetailViewActive
execGetSelectedActionId  execIsHelpOpen
execHandleBackButtonClicked    execOpenHelp
execHandleCloseButtonClicked   execRefreshRows
execHandleRowButtonClicked
execHandleScrollButtonClicked      ← the one this row removes
```

**= 10.**

**Predicted after: 9. Single removal `execHandleScrollButtonClicked`. Zero additions.** The exact inverse of
`TASK-1484`'s delta, and a clean discriminator. `HandleScrollButtonClicked` was the only `UFUNCTION` this row
touches; `AdvanceBodyScroll` was a plain member and contributes no exec symbol.

---

## 4. WHAT CAME OUT — DECLARED BY CONTENT, NEVER BY A `--numstat` INTEGER

`--numstat` is measured algorithm-dependent and unstable (same object, same paths, Myers −191 vs histogram
−69), so every deletion is named:

**`SiegeControlsHelpWidget.h`**
1. `bool AdvanceBodyScroll();` + its ~28-line declaration comment.
2. `UFUNCTION() void HandleScrollButtonClicked();` + its 4-line comment.
3. `UPROPERTY(...) TObjectPtr<UButton> DetailScrollButton;` + its ~27-line declaration comment.
4. `UPROPERTY(...) TObjectPtr<UTextBlock> DetailScrollLabelText;` + its 1-line comment.

**`SiegeControlsHelpWidget.cpp`**
5. `SiegeControlsHelpText::ScrollDownLabel` (`"Scroll down — back to the top at the end"`) + its 15-line comment.
6. `DetailScrollPageFraction = 0.85f` and `DetailScrollFallbackStep = 320.f` + their comment block.
7. The whole `DetailScrollButton` construction block in `ConstructDetailTree`: the `DetailScrollLabelText`
   construct, the `DetailScrollButton` construct, `ApplyButtonFocusable(DetailScrollButton)` (**the focusable
   flip**), `SetContent` + slot padding, `AddChildToVerticalBox`, and the `Warning` log for the null case.
8. The `NativeConstruct` bind `DetailScrollButton->OnClicked.AddUniqueDynamic(...)`.
9. The `NativeDestruct` unbind `DetailScrollButton->OnClicked.RemoveDynamic(...)` — **the pair went out
   together, in one diff**.
10. `void USiegeControlsDetailWidget::HandleScrollButtonClicked()` — the whole definition.
11. `bool USiegeControlsDetailWidget::AdvanceBodyScroll()` — the whole definition, ~80 lines.

**Nothing adjacent came out.** No `#if 0`, no commented-out code, no dead symbol left behind — verified by
grep: **zero live-code references** to any of `DetailScrollButton` / `AdvanceBodyScroll` /
`HandleScrollButtonClicked` / `DetailScrollLabelText` / `ScrollDownLabel` / `DetailScrollPageFraction` /
`DetailScrollFallbackStep` remain in either file. Every surviving mention is inside a removal-record comment.

Also verified: **zero references from anywhere else in the repo** (`Source/`, `Tools/`, `Tests/`) — so nothing
outside this file pair can fail to compile on the removal.

---

## 5. SC-§120 — THE REASONING IS STRUCK, NOT SILENTLY DELETED

Per spec item (4), the note is in the file. Quoted from
`USiegeControlsDetailWidget`'s class comment:

> **A SCROLL AFFORDANCE WAS ADDED AT `TASK-1484` AND REMOVED AT HIS RULING ON 2026-09-26 AFTER PIXELS
> MEASURED THAT NO DETAIL PAGE OVERFLOWS.**

The class comment carries the full record: what `TASK-1484` added, why its mechanism was sound (the one-stop
`MoveFocus` → `SetUserFocus` early-return → `OnFocusChanging` never fires → `ScrollWidgetIntoView` has no
subject chain, which is *still true as a mechanism*), and what actually expired — **the fold, not the
mechanism**: `TASK-1494` measured the model's longest page (`Cards.PlacementResize`, predicted 3.19×) at
lowest painted text row **516 of 615** with **57 px of clear panel below it, with the button present**;
six pages gave six different bottom rows (55/69/82/306/424/516) in the model's exact rank order on one
straight line ⇒ nothing is clipped and there is no common bottom edge.

Shorter dated markers sit at each removal site pointing back to it. The two tuning constants' *shape* of
answer (a fraction of the measured viewport, never a pixel count, less than 1.0) is recorded too, in case a
page ever does overflow.

**`TASK-1492` (the related-body collapse) was NOT done and NOT prepared for.** He declined it separately and
on its own merits.

---

## 6. THE COUNTS — STATED AS A PROPERTY WITH ITS GENERATING RULE

*A gate written in indices has an expiry date nobody printed on it*, so these are rules, not integers:

- **Detail page: 2 → 1.** Generating rule, unchanged at every value: `BackButton` is once more the **only**
  `UButton` `ConstructDetailTree` builds. `DetailScrollBox` is a `UScrollBox` (not one of `IsNavFocusStop`'s
  four admitted classes, and authored non-focusable besides); `RebuildRelatedBlocks` constructs only
  `UHorizontalBox` / `UBorder` / `UTextBlock` / `UVerticalBox`. ⇒ **StopSet(detail) = `{ BackButton }`** for
  every row, every page, however many related blocks it grows.
- **`BackButton` remains stop 0.** It was stop 0 under **both** counts — under two because the scroll control
  was `DetailColumn`'s **last** child and `CollectNavStopsFromTree` walks depth-first **pre-order**; under one
  because it is the only member. `TASK-1478`'s *"the ring lands on `BackButton`"* is true across all three
  values and was never at risk.
- **List stays 28.** Untouched: `{ RowButton of every live row, in `RowScrollBox` slot order } ∪ { CloseButton }`,
  `CloseButton` last. Nothing on the list path was edited.
- **`Tab`-to-close still works from both views.** Unchanged by construction: `NativeOnPreviewKeyDown` is not
  edited, and its cover is a property of the **tree's shape** — the preview phase runs the focused widget's
  ancestor chain, and every surviving stop is a descendant of the overlay. Removing a stop cannot remove an
  ancestor. (Slate's unhandled-key hook has exactly one binder in the engine and it is not the game viewport.)
- **`IA_ControlsHelp` still closes the overlay from the detail page** — same argument, same untouched handler.
  `TASK-1479` graded this limb BLOCKER-class; nothing on its path was edited.

**Stepper-pair note, which changes character rather than disappearing:** `FindStepperPair`'s count guard needs
**exactly two** buttons in the immediate parent. `DetailColumn` now holds **one**, so the name/glyph
discriminators that were load-bearing under `TASK-1484` are no longer reachable — the hazard is closed by
arithmetic rather than refused by two discriminators. Both the header and the cpp now warn that adding a
second `UButton` to `DetailColumn` makes that guard live again.

---

## 7. THINGS QA SHOULD SCRUTINISE

1. **That I removed the right trip-wire and only that one.** Count them: there must be exactly **two**
   occurrences of `TRIP-WIRE` guarding a live `ApplyButtonFocusable` call (`RowButton` at
   `ConstructRowTree`, `BackButton` at `ConstructDetailTree`), plus one `TASK-1496` note explaining the
   third's departure. Three `TRIP-WIRE` hits total in the cpp; the third is the explanation, not a wire.
2. **That `RowScrollBox->SetScrollWhenFocusChanges` survives and is still PRE-`Super`.** One live call,
   `cpp:3075`, inside `ConstructHelpTree()`.
3. **That `DetailScrollBox` and its three authored properties survive.** The scrolling region stays; only the
   button that paged it is gone.
4. **A found stale line I did NOT edit, flagged rather than silently touched.** Header `:1054`
   (`NativeOnPreviewKeyDown`'s comment) reads *"THE FOCUSABLE SET GREW FROM ONE BUTTON TO `RowWidgets.Num()`
   + 2"*. That was written at `TASK-1478` and was **correct then** (rows + `CloseButton` + `BackButton`);
   `TASK-1484` made it silently wrong (+3) and **did not update it**; this removal makes it **correct again**.
   I left it alone because it is now true. Worth recording as evidence that `TASK-1484` left at least one
   un-amended count behind — a candidate for `TASK-1480`'s citation-rot sweep, not for this row.
5. **The `ApplyOpenState` clause (6) and the `RegisterMenuNavTarget` stop-set comments**, both of which I
   amended from TWO back to ONE using strike-through rather than deletion.

---

## 8. Not examined / limitations

- **No compile, no PIE, no MCP mutation, no editor lifecycle action, no git write.** The exec-symbol
  prediction (10 → 9) is a **prediction** derived from the pre-edit generated header plus the single
  `UFUNCTION` removed; it is **not** a post-build measurement. `TASK-1497` and 5a own confirming it. The
  editor was up (PID 24652) with no PIE throughout and I did not touch it.
- **I did not run the automation suite and do not cite it.** Coverage for this screen was re-measured four
  times at **0** across the whole `Tests/` tree, so a green suite would say nothing about this change either
  way.
- **The runtime criterion is unverified by me** — detail page 2 stops → 1, `BackButton` stop 0, list 28,
  `Tab`-to-close from both views, `IA_ControlsHelp`-from-detail. Every one of these is argued above from
  **structure** (which `UButton`s the tree builds, pre-order walk, ancestor-chain cover), not from a runtime
  reading. `TASK-1498` is the limb that observes them. Logging is at `Log`, not `Verbose` — both categories
  are `(…, Log, All)` and print in Development with no console verb, so **an absent line is a ZERO, not a
  void**.
- **`AdvanceBodyScroll`'s log line is gone with the function**, so `TASK-1498` must not look for
  `[ControlsHelp] Detail scroll on '…'`. Its **absence is now the expected state**, and is a *zero* rather
  than a void for the same reason.
- **`SiegeMenuInputSubsystem.{cpp,h}` were read only, never written.** I read `FindStepperPair` and
  `IsNavFocusStop` semantics from the existing in-file citations rather than re-deriving them at engine
  source; those citations were `TASK-1478`/`1484`-era and I did not independently re-measure them.
- **`TASK-1480` was `backlog` (boarded, not dispatched) at my instant**, so the same-file serialization fence
  held. I verified this on the board before writing and did not run concurrently with it.
- I did not touch `CONVENTIONS.md`, any `.uasset`, or any file other than the two named above plus this
  handoff and this row's `status:`.
