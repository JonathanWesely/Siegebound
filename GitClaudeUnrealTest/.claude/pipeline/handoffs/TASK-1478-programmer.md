# TASK-1478 — [CONTROLS-HELP-NAVIGABLE] — gameplay-programmer handoff

marker `TASK-1478-CONTROLS-HELP-NAVIGABLE` · written 2026-09-25 · status set to `ready-for-qa` (gate: `TASK-1479`)

**Files written (the complete list):**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeControlsHelpWidget.cpp`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeControlsHelpWidget.h`
- this handoff · this row's `status:` on `TASKBOARD.md`

**Not touched:** `SiegeMenuInputSubsystem.{cpp,h}` (read-only, six rows' work) · all eight other screen files · `Tests/*` · `SiegePlayerController.cpp` · no asset, no compile, no PIE, no MCP mutation, no git, no `CONVENTIONS.md`.

---

## 0. The headline, before the argument

The screen was keyboard-**closable** with a stop count of **1**. It is now keyboard-**navigable**:

| view | stop set (the generating rule) | count |
|---|---|---|
| **list** | `{ RowButton of every live USiegeControlsHelpRowWidget, in RowScrollBox slot order } ∪ { CloseButton }`, `CloseButton` **last** | `RowWidgets.Num() + 1` = **28** today |
| **detail** | `{ DetailView->BackButton }` — the only `UButton` `ConstructDetailTree` builds | **1** (a *different* 1 from before) |

Both halves of the row landed: the **flip** (necessary) and the **switcher containment** (sufficient). Neither works without the other.

---

## 1. Spec (1) — the per-site decision, argued one site at a time

Five de-focus sites exist in this file pair. I did **not** blanket. `TASK-1432` (2)'s ruling that the scroll boxes must not become stops **stands untouched**.

| site | decision | argument |
|---|---|---|
| `CloseButton` (`ConstructHelpTree`) | **unchanged — stays focusable** | Already flipped by `TASK-1432`. Not re-litigated. Its *position* changed (see §5) and that is declared at the site. |
| `RowButton` (`ConstructRowTree`) | ⭐ **FLIPPED** → `ApplyButtonFocusable` | Its two stated reasons both expired, and **neither was overruled on taste**: (i) *"`Tab` is Slate's focus-next key and a focused SButton would eat it"* — killed by `TASK-1432`'s own `NativeOnPreviewKeyDown`, which claims the derived toggle key in the **preview/tunnel** phase on every ancestor of the focused widget, before the focused `SButton` and before `AttemptNavigation`; (ii) *"flipping it would be INERT because the walker never enters a nested `UUserWidget`'s tree"* — measured false by `TASK-1474`'s descent (`CollectNavStopsFromTree` → `IsCodeAuthoredSubWidget`; `USiegeControlsHelpRowWidget` is a `UCLASS()` in `Source/` ⇒ `CLASS_Native` ⇒ entered). **And it is load-bearing, not cosmetic:** `Enter`/`Space` on a focused `SButton` → `ExecuteOnClick()` → `HandleRowButtonClicked` → `ActivateRow` is the **only** keyboard route into a detail page. |
| `BackButton` (`ConstructDetailTree`) | ⭐ **FLIPPED** → `ApplyButtonFocusable` | Same two expiries, same measurements. Without it the detail page has no ring at all and the row's own cl. 3(b) sentence (*"…and back out without the mouse?"*) is unanswerable. |
| `RowScrollBox` | **NOT flipped** | A `UScrollBox` is not one of `IsNavFocusStop`'s four admitted classes ⇒ flipping moves **nothing** while looking like progress. (Its comment's citation of the now-struck `Tab` argument was re-anchored to this measurement instead — `SC-§120`.) |
| `DetailScrollBox` | **NOT flipped** | Identical. Also: **no `SetScrollWhenFocusChanges` companion** was added here, deliberately — this page's only stop is `BackButton`, a **sibling** of the box in `DetailColumn`, never inside it, so there is no focus change within the box to scroll to. A write with no reachable effect is not a safety net. |

### 1b. One addition the flip forced, declared loudly because it is not a flip

`RowScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll)`.

**Measured, not assumed:** *both* defaults are `NoScroll` — Slate's (`Slate/Public/Widgets/Layout/SScrollBox.h`, the `_ScrollWhenFocusChanges` initialiser in `FArguments`) **and** UMG's (`UMG/Private/Components/ScrollBox.cpp`, the `UScrollBox` constructor's initialiser list). With 27 registry rows in a box that shows a handful, the ring would have walked **off the bottom of the visible area** and Jonathan would have watched the outline vanish — navigable on paper, not in his hands. `SScrollBox::OnFocusChanging` calls `ScrollDescendantIntoView` exactly when this attribute is not `NoScroll`.

`InstantScroll` over `AnimatedScroll` **because of the verifier**: an animated scroll settles over several frames, so a 5b lane that injects `IA_MenuDown` and reads the next frame would sample a position mid-flight. A gate that can disagree with itself between frames is not a gate.

**Zero delta on anything shipped:** the attribute fires only on a focus change *inside* this box, and before this row no child of it could take focus at all.

---

## 2. Spec (2) — THE SWITCHER ANSWER, with its mechanism

### The hazard, restated in one sentence
With both branches now holding focusable `UButton`s, the walker reads the registered screen's **whole** `WidgetTree` (and, since `TASK-1474`, descends into nested code-authored `UUserWidget`s), so it collects **both** branches at once. The inactive branch's buttons are **admitted** (every visibility flag on their chain reads visible) and **refuse focus** (`SWidgetSwitcher::ValidatePathToChild` → `return InChild == GetActiveWidget().Get();`, called from `FSlateWindowHelper::FindPathToWidget` under the engine's own comment naming `SWidgetSwitcher`) — and such a stop **SWALLOWS THE RING**, because `MoveFocus` re-reads the index from Slate every press.

### Four shapes measured. Three are not available.

**(α) REJECTED — retarget the registration to `DetailView` while the detail page is up.**
This was the shape I wanted: the walker would see only that widget's own tree, the list branch would be out of scope for free, and **no visibility write would be needed at all**. It is **measured dead, and silently so**:

> `GetRegisteredNavTarget()` accepts a stack entry only when `Screen->IsInViewport() && Screen->IsVisible()`, and `UWidget::IsInViewport()` is `bIsManagedByGameViewportSubsystem` **+** `UGameViewportSubsystem::IsWidgetAdded(this)` — `UMG/Private/Components/Widget.cpp:344-350`, read at source. That flag is set **only** by `AddToViewport` / `AddToPlayerScreen`. `DetailView` is `WidgetTree->ConstructWidget`'d into a switcher slot and is **never** added to the viewport.

⇒ `RegisterMenuNavTarget(DetailView)` would push it onto `NavTargetStack`, **print a "registered" line**, be **skipped on every read**, and navigation would silently fall back to `FindMainMenuWidget()`. It would compile, review clean, log positively and do nothing — `SC-§36.1` in its purest form. **I did not take this from the subsystem's comment; I read the engine function.** A permanent warning against passing a child widget to that API is now written at `RegisterAsMenuNavTarget`'s call.

**(β) REJECTED — teach `IsNavFocusStop` about `SWidgetSwitcher`.** The general fix and the right one eventually, but it is one function in `SiegeMenuInputSubsystem.cpp`, which this row **may not write**. Escalated in §8, not silently skipped.

**(γ) REJECTED — toggle `IsFocusable` on the buttons at the switch edges.** It would work on the walker and **lie to everyone else**: `UButton::RebuildWidget` reads that field **exactly once** (`Button.cpp:84`), so a post-build write changes what the predicate reports and **nothing** about the live `SButton`. It would leave a button whose UMG field says "not focusable" and whose Slate side takes focus on a mouse click — and this file's own 20-line `ApplyButtonNotFocusable` comment is a warning against precisely that misreading.

**(δ) TAKEN — collapse the inactive branch.** `HasVisibleSlateAncestry` walks the Slate parent chain and refuses anything under a `Collapsed`/`Hidden` ancestor, so the **shipped** predicate drops that branch's buttons with **no new predicate written**. It is the **same mechanism `TASK-1469` limb 2 was built for and measured on** (`VideoModeConfirmBorder` Collapsed ⇒ its two buttons excluded from the Graphics ring), not a novel one.

**The real argument for (δ), which is not "it is convenient":** the collapse makes the UMG-visible state **agree with what the switcher was already doing**. `SWidgetSwitcher` already declines to **arrange**, **render** and **hit-test** the inactive branch; the only thing that was untrue of that branch was its visibility **attribute** — and that attribute is exactly what the predicate reads. **We are not modelling the switcher; we are removing the need to, by stopping the tree from lying about a branch the engine had already switched off.**

### The implementation: one function, one writer

`USiegeControlsHelpWidget::ApplyActiveView(int32 InViewIndex)` — new, `protected`, non-`UFUNCTION`. It is now the **only** `ViewSwitcher->SetActiveWidgetIndex(...)` write in the file (grep: one write; the three view switches — `ConstructHelpTree`, `ShowDetailForAction`, `ReturnToList` — all route through it). It writes **both** branch visibilities **and** the index from **one parameter**, every call.

### `TASK-1432`'s REJECTED (b) is being taken after being refused. I say so, and answer both objections mechanically.

- **Objection 1 — *"it invents a SECOND source of truth for which view is up."*** **Answered by shape.** There is no second state. `IsDetailViewActive()` still asks the **switcher** and is still the only reader-facing truth; the two visibility writes are a **derived projection** of one parameter, written in the same statement sequence that writes the index. There is nothing to forget to update, because **a "restore" is not a separate action** — every call writes both branches.
- **Objection 2 — *"the failure mode of a missed restore is a screen whose only on-screen exit is invisible."*** **Answered by construction, on one line:**
  ```cpp
  const bool bDetail =
      (ViewSwitcher != nullptr) && (DetailView != nullptr) && (InViewIndex == DetailViewIndex);
  ```
  `PanelBorder` can be collapsed **only** in the same statement sequence that makes a **built** `DetailView` visible **and** active. There is no reachable state in which the list is hidden and nothing replaces it, and the degraded no-switcher shape `ConstructHelpTree` falls back to **never collapses anything at all**.
- **Residual, declared not denied:** a *future* path writing `SetActiveWidgetIndex` outside `ApplyActiveView` could diverge index from visibility. Two `ensure`s catch it (§3b). Neither can fire today.

### Why the active branch is pixel-identical to what shipped
`PanelBorder`'s restored value is its authored `ESlateVisibility::Visible` (hit-test correctness — the plate absorbs its own clicks so a row click cannot fall through into a live match and place a card). `DetailView` has **never** had its visibility written in this file, so it carries `UWidget`'s `Visible` default, and that is what is restored. `SWidgetSwitcher::OnArrangeChildren` arranges `GetActiveSlot()` and nothing else; `ComputeDesiredSize` reads **only the active slot's** child visibility — never the collapsed one. ⇒ **no layout, no pixel, no hit-test changes on any frame the player sees.**

---

## 3. Spec (3) — THE TRIP-WIRE, and the WARN-L1 interaction RESOLVED rather than inherited

### 3a. I did not inherit the counter-case. I removed it.

`qa/TASK-1433.md` WARN-L1 named **this very flip** as the counter-case: flip `BackButton` and a switcher-aware predicate would keep `{BackButton}` while `TASK-1432`'s edges **unregistered the whole screen** — silently.

**Decision: the register/unregister edges do not stay.** `ShowDetailForAction`'s `UnregisterAsMenuNavTarget()` is now `RegisterAsMenuNavTarget()`. The invariant goes from *`registered ⟺ bHelpOpen AND the list view is up`* to **`registered ⟺ bHelpOpen`**, and `UnregisterAsMenuNavTarget` drops from three call sites to two (`ApplyOpenState(false)` and the `NativeDestruct` net).

**Why that is the right half of the fork, not the convenient one.** The row's own runtime criterion and cl. 3(b) sentence require getting *into* a detail page and *back out* by keyboard. Keeping the edges means the detail page has **no ring at all**, which fails the acceptance by construction. Keeping the ring means keeping the registration — so the switcher must be answered some *other* way, which is §2's collapse. The two halves of the fork are not independent: **choosing "the screen keeps a live ring across the switch" forces (δ), and choosing (δ) is what makes the unregister edge unnecessary.**

**Why a RE-register and not simply doing nothing on the switch.** The ring must be *moved*, not merely permitted. The widget holding focus one line earlier is a `RowButton` now in a collapsed branch, so Slate drops focus and the page would open **ringless**. `RegisterMenuNavTarget` ends in `FocusFirstNavStop()`, whose idempotence guard asks *"is anything among the **current** stops already focused?"* — the old holder is no longer among them, so the guard falls through and focus lands on the new page's stop 0. Re-registering also de-duplicates by identity (`RemoveAll` then `Add`), so nothing accumulates. **Bonus: it is the 5b instrument for free** — `LogNavTargetRetarget` prints the per-page stop count on every switch.

### 3b. The trip-wire itself — quoted verbatim from `ConstructDetailTree`, at the flip site

> ```
> //  ⛔ **IF `BackButton` IS FOCUSABLE — IT IS, ON THE LINE BELOW — THEN THIS SCREEN'S RING
> //  IS CORRECT ⛔ ONLY BECAUSE `USiegeControlsHelpWidget::ApplyActiveView` COLLAPSES
> //  WHICHEVER `ViewSwitcher` BRANCH IS ⛔ INACTIVE. ⛔ REMOVE, BYPASS OR MIS-TARGET THAT
> //  COLLAPSE AND THIS BUTTON BECOMES AN ⛔ ADMITTED-BUT-UNFOCUSABLE STOP THAT ⛔ SWALLOWS
> //  THE RING ⛔ EVERY TIME THE LIST IS UP — ⛔ with every visibility flag reading green and
> //  ⛔ nothing in any log.**
> ```

It is followed in the same block by the named mechanism (`SWidget::ValidatePathToChild` is a no-op `return true` for every widget except `SWidgetSwitcher`; `FindPathToWidget` empties the path; `IsNavFocusStop` models ancestor **visibility** only and says so in its own limb-2 comment) and by the WARN-L1 resolution above.

**The mirror trip-wire is at the `RowButton` flip site**, ending: *"IF THAT COLLAPSE IS REMOVED, BYPASSED OR MIS-TARGETED, THIS WRITE MUST GO BACK TO `ApplyButtonNotFocusable` IN THE SAME DIFF."* Both point at `ApplyActiveView`; `ApplyActiveView` points back at both by name. **A controls-help editor hits one of the three no matter which door they come in by** — which is the thing `TASK-1474` could only approximate from inside the subsystem.

### 3c. `ensure` **and** comment — the spec asked which, and the answer is "one of each, for different failures"

**`ensureMsgf` #1, in `ApplyActiveView` — the slot-order contract.**
```cpp
ensureMsgf(
    ActualListBranch == ExpectedListBranch && ActualDetailBranch == ExpectedDetailBranch,
    TEXT("[ControlsHelp] ViewSwitcher slot order no longer matches ListViewIndex/DetailViewIndex - ApplyActiveView would collapse the WRONG branch and hide the list while it is up."));
```
It earns its place because it guards an invariant that can rot **without anybody editing this function**: `ListViewIndex`/`DetailViewIndex` are the *order of two `AddChild` calls* a hundred lines away. **Before** this row, swapping them showed the wrong page — instantly visible. **After** this row it would also collapse the wrong branch, i.e. hide the list *while the list is up* — exactly the stranding mode REJECTED (b) feared. A comment cannot catch a reorder.

**`ensureMsgf` #2, in `ApplyOpenState(true)` — the divergence detector.** Asserts the **active** branch is not `Collapsed` at the one instant the screen becomes visible. It is **not tautological** the way a check inside `ApplyActiveView` would be: different function, different edge, far from the writer. It catches the §2 residual — a future `SetActiveWidgetIndex` written elsewhere — whose unaided symptom is an **empty overlay** or a **swallowed ring**, neither of which prints anything.

**Both can fire in Development and neither can fire in Shipping**, measured: `DO_ENSURE` is `USE_ENSURES_IN_SHIPPING` (`Core/Public/Misc/Build.h:305`), 0 by default, and with it 0 `ensureMsgf` expands to `(LIKELY(!!(InExpression)))` (`AssertionMacros.h:470`) — the expression is evaluated, **nothing is reported, nothing crashes**. Costs: two `TArray::IsValidIndex` lookups + two pointer compares, and one `GetVisibility()`. **Neither can fire today.** `UPanelWidget::GetChildAt` returns `nullptr` out of range (`PanelWidget.cpp:39-47`), so a half-built tree compares `nullptr == nullptr` and **passes** rather than raising a false alarm on a real degradation.

---

## 4. Spec (4) — the counts, as a PROPERTY, delta'd against `TASK-1474`'s table

**Stated before 5b. Never a bare integer.**

**Generating rule (list view up):** one stop per `USiegeControlsHelpRowWidget` that `RefreshRows` materialised (its `RowButton`, which is that widget's tree **root**), in `RowScrollBox` slot order, followed by `CloseButton`.
⇒ **`|StopSet(list)| = RowWidgets.Num() + 1`.** Today `FSiegeControlsHelpRegistry::GetActions()` holds **27** rows (27 `AddRow(TEXT(...))` calls) and `RefreshRows` filters none — `HELP-§2` mechanism 2 forbids a hidden row — so **28**. The same session's `[ControlsHelp] Rebuilt %d of %d registry rows` line prints `RowWidgets.Num()`, so the figure is **cross-checkable from the log without being hard-coded anywhere**.

**Generating rule (detail view up):** the set of `UButton`s `ConstructDetailTree` builds = **{`BackButton`}**. `DetailScrollBox` is a `UScrollBox`; `RebuildRelatedBlocks` constructs only `UHorizontalBox`/`UBorder`/`UTextBlock`/`UVerticalBox`; everything else is a border, a text block or a box.
⇒ **`|StopSet(detail)| = 1`, for every row, every page, however many related blocks it grows.**

**The cross-page invariant, which is the real gate:**
> `StopSet(list) ∩ StopSet(detail) = ∅`, and **the walker never returns members of both at once** — because `ApplyActiveView` collapses the inactive branch and `HasVisibleSlateAncestry` drops everything beneath it.

**Non-contributors (all measured):** category headers (`UTextBlock`), both `UScrollBox`es, `TitleText`, `HintText`, every `UBorder`/`UVerticalBox`/`UHorizontalBox`, and the nested `UUserWidget`s themselves — none is one of `IsNavFocusStop`'s four admitted classes.
**And no stop is an ancestor of another** (each `RowButton` contains only boxes and text; `CloseButton`/`BackButton` contain one `UTextBlock` each), so `GetFocusedNavStop()`'s documented ancestor-precedence hazard cannot arise on this screen.

### Delta vs `TASK-1474`'s table (and nothing older)

| screen | `TASK-1474` measured | this row predicts | delta |
|---|---|---|---|
| controls help — **list** | **1** (`CloseButton`) | **`RowWidgets.Num()` + 1 = 28** | **+27**, and `CloseButton` moves from stop 0 to stop 27 |
| controls help — **detail** | **1** (`CloseButton`, *admitted but unfocusable* — this was the swallow) | **1** (`BackButton`, *focusable and in the active branch*) | **count unchanged, identity and correctness both changed** — the honest way to state it, because a bare "1 → 1" would read as "nothing happened" |

⚠️ `TASK-1474` did not separate the two pages; its single "1" is the list-page figure and the detail-page figure is derived from the same measurement (the same `CloseButton`, reached through the inactive branch). Flagged rather than smoothed over.

### How 5b should anchor these — the names are NOT unique

**`MoveFocus` prints a bare `GetName()`, and all 27 rows' buttons are named `RowButton`** (each is the root of its own row widget, so the name repeats per outer). **And `CloseButton` also exists in `Content/UI/WBP_WarMap.uasset`.** So:
- **Anchor on the pair of lines, not on a name.** `LogNavTargetRetarget` prints `menu nav target registered -> '<screen object name>' (registered screen), N focus stop(s), M screen(s) registered.` — that names the **owning screen**. `MoveFocus` then prints `focus moved i -> j of N ('<name>')`.
- **`of 28` is the discriminator**: WBP_WarMap's `CloseButton` can never appear in a 28-stop ring whose immediately preceding retarget line names the controls-help widget.
- **Row identity is `UNANCHORED`** — the log cannot distinguish row 3 from row 4. What it *can* prove is the count and the index monotonicity, which is what "the ring moves across more than one stop" means. Do not claim more.
- **Both categories log at `Log`, not `Verbose`** (`LogSiegeControlsHelp` and `LogSiegeMenuInput` are both `(…, Log, All)`), so in Development they print **with no console verb**. ⇒ **an absent line is a ZERO and a defect signature, not `UNOBSERVABLE`.**

### Expected log sequence for a clean 5b sitting
1. open: `[ControlsHelp] Rebuilt 27 of 27 registry rows…` → `[ControlsHelp] Overlay opened.` → `menu nav target registered -> '…' (registered screen), 28 focus stop(s), 1 screen(s) registered.`
2. `IA_MenuDown` ×k: `MoveFocus(+1): focus moved i -> i+1 of 28 ('RowButton')`, **strictly advancing** — a repeated `i -> i+1` with the same `i` is the swallow signature.
3. `Enter` on a row: `[ControlsHelp] Detail page open for '<id>' (n related control(s)).` → `menu nav target registered -> '…' (registered screen), **1** focus stop(s), 1 screen(s) registered.`
4. `IA_MenuDown` on the detail page: `focus moved 0 -> 0 of 1 ('BackButton')` — a 1-stop ring wrapping to itself is **correct**, not a swallow; the discriminator is `of 1`.
5. `Enter` on Back: `menu nav target registered -> '…', 28 focus stop(s)`.
6. toggle key from **either** page: `[ControlsHelp] Overlay closed.` + `menu nav target unregistered -> …`.
⚠️ **One transient `registered` → `unregistered` pair is expected on exactly one route** — closing *from the detail page* runs `CloseHelp()` → `ReturnToList()` (registers) → `ApplyOpenState(false)` (unregisters) in one stack. `TASK-1432` declared it; it is unchanged. **Not churn.**

---

## 5. Spec (5) — what must NOT move, and the toggle-still-closes argument

| law | status |
|---|---|
| no `SetInputMode`, no `bShowMouseCursor` (`.cpp`'s `OpenHelp` rule) | **untouched** — census: 0 occurrences of either in this file pair |
| `KBD-§1`/`§2` — no input binding, no mapping-context mutation | **untouched** — 0 `MapKey`/`UnmapKey`/`AddMappingContext` on any path I wrote |
| `Escape` may never be claimed (`AS-§6 A-2`, `HELP-§5`) | **untouched** — the unconditional first guard in `NativeOnPreviewKeyDown` is byte-identical |
| no `SetKeyboardFocus` from this widget | **untouched** — all focus placement is `FocusFirstNavStop()`, inside the subsystem |
| scroll boxes stay non-stops (`TASK-1432` (2)) | **held** — both still `SetIsFocusable(false)`, reasons re-anchored |
| `HELP-§3` escape hatch | **untouched** — an asset-authored `WBP_ControlsHelp` still returns early and wins whole (see §7) |

### 🚨 `IA_ControlsHelp` still closes the overlay from the detail page (`TASK-1436` P6, BLOCKER-class)

**Re-measured, not assumed** — the row warned that more focusable stops means more widgets that convert `Tab` into navigation, and it was right to.

1. `Tab` **is** Slate's focus-next key (`FNavigationConfig::GetNavigationDirectionFromKey`, `bTabNavigation` true by default, no custom config in this project), so this is a real hazard.
2. `FSlateApplication::ProcessKeyDownEvent` routes **preview** key-down **down the focus path** (tunnel, root → focused widget) **before** the bubbling `OnKeyDown` phase and **before** `AttemptNavigation`. A `Handled()` there short-circuits all of it.
3. The focus path is the **ancestor chain** of the focused widget, and **every** button this row made focusable — each `RowButton`, `BackButton` — is a **descendant** of `USiegeControlsHelpWidget`'s root. ⇒ **this widget is on the path for all of them, by construction.** The cover is a property of the tree's *shape*, not of which button happens to be focusable. **This is the part I would have got wrong by assuming the existing handler "still covers the new set" — it does, but only because the new set is entirely inside the overlay.**
4. **New case the flip creates, and it is covered by the same sentence:** a *mouse click on a row* now gives its `SButton` keyboard focus (it did not before). That focus is still inside the overlay ⇒ same tunnel.
5. **Nothing-in-the-overlay-focused case unchanged from before `TASK-1432`:** the handler never runs, the key reaches Enhanced Input, and the controller's `IA_ControlsHelp` bind closes it.

⇒ Both pages, both routes. And `ApplyActiveView` **cannot** interfere: it writes a visibility and an index, never input and never focus.

**Also preserved:** `Enter`/`Space` are deliberately handed straight on by `NativeOnPreviewKeyDown`, which is what makes a focused `RowButton` (into a page) and a focused `BackButton` (out of one) work at all.

---

## 6. Changes by content (never by a `--numstat` integer)

`--numstat` deletion counts are algorithm-dependent (Myers vs histogram disagree on this file), so here is what changed **by content**.

**Statements replaced — six, all one-for-one:**
1. `ApplyButtonNotFocusable(RowButton);` → `ApplyButtonFocusable(RowButton);`
2. `ApplyButtonNotFocusable(BackButton);` → `ApplyButtonFocusable(BackButton);`
3. `ViewSwitcher->SetActiveWidgetIndex(ListViewIndex);` (in `ConstructHelpTree`) → `ApplyActiveView(ListViewIndex);`
4. `ViewSwitcher->SetActiveWidgetIndex(DetailViewIndex);` (in `ShowDetailForAction`) → `ApplyActiveView(DetailViewIndex);`
5. `ViewSwitcher->SetActiveWidgetIndex(ListViewIndex);` (in `ReturnToList`) → `ApplyActiveView(ListViewIndex);`
6. `UnregisterAsMenuNavTarget();` (in `ShowDetailForAction`) → `RegisterAsMenuNavTarget();`

**Statements added — four:** the `SetScrollWhenFocusChanges` call; the two `ensureMsgf`s; and the body of `ApplyActiveView` (two guarded `SetVisibility` writes + one guarded `SetActiveWidgetIndex`).

**Declarations added — one:** `void ApplyActiveView(int32 InViewIndex);` in the existing `protected:` section. **Not a `UFUNCTION`.**

**Prose deleted — NONE.** Every superseded sentence is struck in place with `~~ ~~` per `SC-§120`. Verified by grep that all seven originals still exist verbatim in the files: the `RowButton` "correctness rather than polish" paragraph, both scroll-box `Tab` citations, `BackButton`'s "see RowButton's comment", the two "NOT flipped" bullets, the "ONLY admitted-class widget" claim, and both spellings of the old `registered ⟺ … AND the list view is up` invariant.

**Post-edit structural check (not a compile):** comment/string-stripped brace and paren balance is **0/0 for both files**.

### `.generated.h` prediction — stated as the EXEC-SYMBOL SET, per the correction that landed this hour

**Prediction: the exec-symbol set of `SiegeControlsHelpWidget.generated.h` is UNCHANGED — same thunks, same set, same signatures.** No `UFUNCTION`, `UPROPERTY`, `UCLASS`, `USTRUCT`, `UENUM`, `UDELEGATE` or `GENERATED_BODY()` was added, removed, renamed or re-signatured; `ApplyActiveView` is a plain C++ member.

I make **no prediction of "byte-identical" or "sha unchanged"** — that proxy was refuted this build. I do record one measurement that bears on it: **all four `GENERATED_BODY()` anchors are unmoved by my edits** (127 / 438 / 580 / 856 — exactly the post-`TASK-1432` values the dispatch quoted), and so are all four `UCLASS()`/`USTRUCT()`/`UENUM()` lines, because every header edit I made is **below** line 856. Since UHT embeds source line numbers in its macro names, this row has removed the mechanism that changed the sha last time — but the **binding claim is the symbol set**, and the sha is an observation, not a gate. **No class in this file crosses line 999**, so the size-change caveat does not apply.

---

## 7. What QA should scrutinise hardest

1. **The `IsInViewport()` refutation of (α)** — it is the load-bearing negative result. If it is wrong, the whole shape should have been a retarget and the collapse is unnecessary complexity. Cited: `UMG/Private/Components/Widget.cpp:344-350`.
2. **Ordering inside `ApplyActiveView`** — visibility writes, then index. I claim nothing observes between them (`SetActiveWidgetIndex` broadcasts no delegate; no re-entrancy). Falsify that and the register that follows in the caller could sample a half-applied state.
3. **`bDetail`'s three conjuncts** — the whole "you cannot hide the list" guarantee rests on them. Check that no path can reach a `DetailViewIndex` state with `DetailView == nullptr`.
4. **`RegisterMenuNavTarget`'s idempotence guard on the switch.** I rely on the old focus holder being *absent from the new `Stops` array* so the guard falls through and focus is re-placed. If a collapsed-branch button somehow survived the predicate, the guard would return early and the page would open **ringless** — a quieter failure than a swallow.
5. **`HELP-§3` escape hatch interaction.** If `/Game/UI/WBP_ControlsHelp` or `/Game/UI/WBP_ControlsDetail` is ever authored, `ConstructHelpTree`/`ConstructDetailTree` return early and **none** of these writes run — including the `SetScrollWhenFocusChanges` and, crucially, **the branch collapse**, while `ApplyActiveView` would still be called and would still collapse whatever `PanelBorder`/`DetailView` resolved to from the asset. Both assets are RESERVED and unauthored today. **Declared, not handled** — handling it would mean adjudicating an asset that does not exist.
6. **`SetVisibility` on `DetailView` during `ConstructHelpTree`**, i.e. before its `SWidget` is built. I assert `UWidget::SetVisibility` stores the UPROPERTY and applies it at realization.
7. **The `TObjectPtr` → `const UWidget*` copy-initialisations** in both `ensure` sites (one user-defined conversion + one standard conversion). I chose named locals over `static_cast` specifically to keep that unambiguous.

## 8. Escalations (`SC-§50` / `SC-§100` — named, not resolved here)

- **(β) belongs in the subsystem.** When `IsNavFocusStop` learns about `SWidgetSwitcher`, this row's collapse becomes **belt and braces rather than load-bearing**: it could then only remove stops the fixed predicate would also remove. Until then the collapse is the *only* thing standing between the two flips and a severed ring, which is why both flip sites carry a trip-wire. **Recommend the manager board it.**
- **`TASK-1474`'s own open escalation is untouched by this row** — the deck builder is self-driving, so its descended stops are counted and not ring-walked. Different screen, different row.

---

## Not examined / limitations

- **No compile, no PIE, no MCP, no editor action, no git.** Everything here is source reading and engine-source measurement. **The claim that this builds is UNTESTED** — that is `TASK-1479` then 5a.
- **`28` is a prediction, not a measurement.** It is derived from 27 `AddRow` calls plus one Close button and the rule that `RefreshRows` filters nothing. If a registry row fails `CreateWidget` at runtime, `RefreshRows` logs and `continue`s and the count is 27 — **which is why the gate is the property `RowWidgets.Num() + 1`, cross-checked against the "Rebuilt %d of %d" line, and not the integer.**
- **Ring *visibility* is not examined and is out of fence.** Whether the focus rectangle reads clearly on a `RowButton` whose background is `FLinearColor(1,1,1,0.06)` is a **visual** judgement on a shipped screen; `AS-§6 A(e)`/`HELP-§6` forbid an agent adjudicating it and this row changed **no** style. If Jonathan finds the outline hard to see on the rows, that is a real finding and a **new row**, not a defect in this one.
- **Returning from a detail page puts the ring at the TOP of the list, not on the row the player came from.** Declared at `ReturnToList`. Fixing it needs per-screen focus memory plus a direct focus call, and this file's own law forbids the second. **A usability question for Jonathan, not a severed ring.**
- **`CloseButton` moved from stop 0 to stop 27** (rows precede it in `RootPanel` slot order and the walker is depth-first pre-order). So the overlay now **opens with the ring on the first row** rather than on Close. Deliberate and unavoidable without a focus call; declared at the `CloseButton` site.
- **Row identity in the log is `UNANCHORED`** — see §4. All 27 row buttons print as `RowButton`.
- **Slot order vs visual order** is the walker's own declared limit and is *satisfied* here by construction (`RowScrollBox` is filled top-to-bottom by `RefreshRows`), but I did not re-verify it against a rendered frame.
- **The `TASK-1474` "detail page = 1" figure is derived, not separately measured** by that row — flagged in §4's table rather than presented as its own number.
- **`Tests/SiegeControlsHelpTest.cpp` asserts nothing** about the switcher, visibility, focusability or stop counts (measured: zero occurrences of `Switcher`/`ActiveWidgetIndex`/`Visibility`/`Collapsed`/`IsFocusable`/`RowButton`/`BackButton` in that file), and `Tests/SiegeMenuInputTest.cpp` works on **synthetic fixtures**, never this screen's real tree. ⇒ **I expect no test to change behaviour, and equally: no existing test covers anything this row did.** The automation suite is **not** citable for this screen's live tree.
- **The gamepad / `IA_MenuAccept` lane is not examined.** `TASK-1432` recorded that `IA_MenuAccept` is never built in a match; this screen's Accept is Slate's own, via `SButton::OnKeyDown`. Unchanged by this row and unverified by it.
