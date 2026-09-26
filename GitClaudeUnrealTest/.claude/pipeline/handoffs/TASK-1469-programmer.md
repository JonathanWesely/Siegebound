# TASK-1469 — handoff (gameplay-programmer)

**Row:** `[MENU-NAV-FOCUS-READ-AND-WALKER-REPAIR]` — Row A, position 1 in `SUBSYSTEM-QUEUE-ORDER-RULED-2026-09-25`
**Marker:** `TASK-1469-MENU-NAV-FOCUS-READ-AND-WALKER-REPAIR`
**Date:** 2026-09-25 · **Status set:** `ready-for-qa` (gate = `TASK-1470`)
**Files written — TWO, and nothing else:**
- `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp`
- `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h`

⛔ No compile, no PIE, no MCP mutation, no editor lifecycle action, no git, no asset, no other `.cpp`, no engine-source edit, no `SiegeMenuInputTest.cpp` edit, no `CONVENTIONS.md`, no other row's line. `grep -rln "TASK-1469" Source/` returns exactly the two files above.

---

## §0 — 🚨 THE (2a) ANSWER, FIRST AND EXPLICIT: **(A) — ONE LIMB, NOT TWO. AND THE REALIZATION HALF WAS ALREADY IN THE CODE BEFORE I TOUCHED IT.**

> **`realized: false` is NOT a third limb. It is not even a NEW test — `IsNavFocusStop` has tested it since `TASK-1406`, through `UWidget::IsVisible()`. The severed Graphics ring is ancestor visibility, full stop.**

**The argument is two independent halves, and either one is sufficient.**

**(A-i) AT SOURCE — the existing clause already excludes an unrealized widget.** `UWidget::IsVisible()` (UE 5.8, `Runtime/UMG/Private/Components/Widget.cpp`) is:

```cpp
bool UWidget::IsVisible() const
{
    TSharedPtr<SWidget> SafeWidget = GetCachedWidget();
    if ( SafeWidget.IsValid() ) { return SafeWidget->GetVisibility().IsVisible(); }
    return false;                       // ← NO CACHED SWIDGET ⇒ FALSE
}
```

`IsNavFocusStop`'s first clause has always been `!Widget->IsVisible() ⇒ reject`. ⇒ **a widget with no live Slate widget could never have been admitted as a stop in the first place.**

**(A-ii) AND THE EVIDENCE SAYS SO TOO — this is the part that turns an argument into a measurement.** `TASK-1413` printed, six times:

```
MoveFocus(+1): focus moved 17 -> 18 of 24 ('KeepSettingsButton').
```

That line **only exists** because `KeepSettingsButton` was **in the stops array**, i.e. because `IsNavFocusStop` returned `true`, i.e. because `Widget->IsVisible()` returned `true`, i.e. **because `GetCachedWidget()` was valid**. ⇒ **the button WAS realized in the UMG sense at walker time.** The runtime evidence refutes the literal reading of its own `realized: false` field.

⇒ **`ui_snapshot`'s `realized: false` is an ARRANGEMENT-level reading** — the node is absent from the arranged/painted tree — **which is exactly and only what a Collapsed ancestor produces.** Same condition, different vantage point. `TASK-1413` was right about the *behaviour* and its own §7 corroborates this reading in pixels: *"No Keep/Revert confirm bar is drawn."* A Collapsed subtree is not drawn and not arranged; its `SWidget`s still exist.

**THE MECHANISM, MEASURED AT ENGINE SOURCE — this is WHY my change crosses the 18–20 dead zone:**

1. `FSlateApplication::SetUserFocus(UserIndex, WidgetToFocus, Cause)` (`Slate/Private/Framework/Application/SlateApplication.cpp:2721-2747`) succeeds **only** if `FSlateWindowHelper::FindPathToWidget` finds the widget. Its own failure message: *"Attempting to focus a widget that isn't in the tree **and visible**"* (`:2743`).
2. That search is `FWidgetPath::SearchForWidgetRecursively` (`SlateCore/Public/Layout/WidgetPath.inl:23-52`), which descends via `InCandidate.Widget->ArrangeChildren(Geometry, FArrangedChildren(VisibilityFilter), ...)` with the default `VisibilityFilter = EVisibility::Visible` (`WidgetPath.h:137, 202`).
3. A `Collapsed` (or `Hidden`) panel contributes **no arranged children** ⇒ nothing beneath it is reachable ⇒ `FindPathToWidget` fails ⇒ `SetUserFocus` returns `false`.
4. `USiegeGraphicsMenuWidget::BuildVideoModeConfirmRow` builds `VideoModeConfirmBorder` and sets `ESlateVisibility::Collapsed` on it (own comment: *"COLLAPSED, NOT HIDDEN… WRITER 1 OF 4"*). `KeepSettingsButton` / `RevertSettingsButton` live inside it, with their **own** visibility left `Visible`.
5. `MoveFocus` re-reads `Current` from Slate every press, so a focus request that fails leaves the index unmoved ⇒ **`17 -> 18` forever.** Not "two wasted presses" — a **severed ring**.

**⇒ The fix is one predicate, and it is chosen to MIRROR rule (2) exactly:** `HasVisibleSlateAncestry` walks the **Slate** parent chain (`SWidget::GetParentWidget()`) testing `GetVisibility().IsVisible()`. That test is **bit-for-bit** `FArrangedChildren`'s filter — both are `0 != (Value & VIS_Visible)` (`Layout/Visibility.h`; `EVisibility::DoesVisibilityPassFilter`). A UMG `UWidget::GetParent()` walk would **not** mirror it: it stops dead at every `UUserWidget` boundary and never sees the wrapper `SWidget`s the real path traverses.

**⭐ THE PROPERTY THAT MAKES THIS PROVABLY NON-REGRESSIVE:** every stop that has ever **successfully taken focus** necessarily satisfied `FindPathToWidget`, and therefore satisfies the new predicate. **The new clause can only remove stops that could never have been focused.** That is not a hope — it is why §3's "main menu stays 7" is a derivation and not a wish.

**I did NOT need answer (B) or (C).** No second test is implemented, because there is no second condition to test: the realization short-circuit is the `!SelfSlate.IsValid()` line, and it is the same short-circuit `IsVisible()` already had, restated so the helper is correct standing alone.

---

## §1 — THE DIFF, LIMB BY LIMB

### Limb 1 — the text-box focus read (`TASK-1420` / `qa/TASK-1426.md` WARN-1)

**(a) `GetFocusedNavStop()`** — was one loop over `Stop->HasUserFocus(PC)`. Now **two passes**: exact focus asked of every stop first, then `HasUserFocusedDescendants(PC)`.

🚨 **THE TWO PASSES ARE NOT STYLE — THEY CLOSE A HAZARD THE `||` ALONE WOULD OPEN.** `FSlateUser::HasFocusedDescendants` (`Slate/Private/Framework/Application/SlateUser.cpp:192-195`) is `WeakFocusPath.IsValid() && GetLastWidget() != Widget && WeakFocusPath.ContainsWidget(&Widget)` — the focus path is the focused widget's **whole ancestor chain**, so **every ancestor answers true**. `GetMenuFocusStops` returns depth-first **pre-order**, ancestors first. A single fused `||` loop would return the *container* in preference to the control actually wearing the ring, the first time any screen nests one admitted class inside another. Exact focus is authoritative, so it is asked of all stops before the descendant question is asked of any. Cost: one extra pass over a `TArray` already in hand.

**(b) the twin early-out in `FocusFirstNavStop()`** — the same blind predicate, inlined there so the tree is walked once. Widened to `Stop->HasUserFocus(PC) || Stop->HasUserFocusedDescendants(PC)`. **Left alone it would have been strictly worse than the bug it mirrors:** a player standing in a text box reads as "nothing focused", the idempotence guard falls through, and `FocusWidget(Stops[0])` **yanks the ring out of the field mid-typing** — `RegisterMenuNavTarget` ends in this call and `UAccountMenuWidget::ApplyMode` re-registers on every mode change. **One `||`, not two passes**, because this loop asks "is *anything* focused?", which is order-independent — it never returns *which*.

Mechanism cited, not re-derived, per the row: `SEditableTextBox::OnFocusReceived` (`Slate/Private/Widgets/Input/SEditableTextBox.cpp:309-320`) forwards focus to its inner `SEditableText`; `UWidget::HasUserFocus` is exact-widget; `UWidget::HasUserFocusedDescendants` is the strict-descendant twin the engine has always shipped and this project never called. The shape is the one `AccountMenuWidget.cpp`'s **guard 2** already ships — *"🚨 THE `||` IS LOAD-BEARING AND IT IS MEASURED, NOT DEFENSIVE"* — moved to where every caller sees it.

### Limb 2 — ancestor visibility in `IsNavFocusStop` (`qa/TASK-1418.md`'s walker ask)

New private static `USiegeMenuInputSubsystem::HasVisibleSlateAncestry(const UWidget*)`, called as a second clause immediately after the untouched `!Widget || !Widget->GetIsEnabled() || !Widget->IsVisible()` line. Full argument in §0.

⚠️ **NOT a permanent exclusion.** `ArmVideoModeCountdown()` sets `VideoModeConfirmBorder` back to `Visible`; on that frame both confirm buttons become stops again and the Graphics count goes 19 → 21. **The ring tracks the screen.** A verifier who arms a video-mode change and then counts will correctly read 21.

### Limb 3 — the stepper double-stop (`qa/TASK-1410.md` WARN-2)

`IsNavFocusStop` now refuses the **`Next`** member of a pair that `FindStepperPair` recognises. The `Prev` member survives; `StepFocusedStop` already calls `FindStepperPair` on whichever member holds focus and presses the correct one, so **Left/Right drive the whole row from one stop with zero change to that function**.

**Shape chosen with an argument (`SC-§101` — the row named the defect, not the remedy). Three candidates:**
- **(i) admit neither member** — rejected: it makes a real control keyboard-unreachable, which is the defect this epic exists to remove.
- **(ii) synthesise a stop for the parent `RowBox`** — rejected **at source**: a `UHorizontalBox`'s `SWidget` does not support keyboard focus, so `FocusWidget`'s own backstop (`SlateWidget->SupportsKeyboardFocus()`) would refuse it and I would have manufactured a **second severed ring** while claiming to fix the first.
- **(iii) admit exactly one member** — shipped. **`Prev` is kept** because it is first in slot order, so the ring's order is a strict **subset** of the old order with nothing permuted (that is what makes the count table attributable), and because `BuildStepperRow` puts it immediately after the label, so the outline lands beside the words naming the row.

🚨 **REUSING `FindStepperPair` RATHER THAN WRITING A NEW STRUCTURAL RULE IS ITSELF THE SAFETY PROPERTY, AND THIS IS THE FINDING I MOST WANT QA TO CHECK.** A structure-only rule ("drop the second button in a two-button row") would have deleted:
- **`USettingsMenuWidget::BackButton`** — its `RootPanel` holds **exactly two** `UButton`s, `GraphicsButton` and `BackButton`;
- **`USiegeGraphicsMenuWidget::BackButton`** — `RootPanel`'s only two **direct** `UButton` children are `AutoDetectButton` and `BackButton` (the stepper buttons live inside their own `RowBox`es, Keep/Revert inside the confirm Border).

**That second one is `BackButton` on the very screen the runtime criterion is about.** `FindStepperPair` refuses both pairs because **both discriminators fail**: names are `AutoDetectButton`/`BackButton` and `GraphicsButton`/`BackButton` (neither ends in `PrevButton`/`NextButton` with a matching base), and labels are `"Auto-Detect Quality"` / `"Back"` / `"Graphics"` — none is `"<"` or `">"`. ✅ Both `BackButton`s survive; Settings keeps all three stops.

---

## §2 — 🚨 THE ONE COUNT TABLE (six screens, before / after / attributed limb)

**Read the `Provenance` column before citing a number.** `MEASURED` = read at runtime by `TASK-1413` or off the `.uasset` by `qa/TASK-1426.md`. `DERIVED` = read out of the construction source by me for this row.

| Screen | Before | After | Δ | Attributed limb | Provenance |
|---|---|---|---|---|---|
| **Main menu** (`WBP_MainMenu`) | **7** | **7** | 0 | — (see §3) | before MEASURED (`TASK-1413` §1, by name, with the wrap); after DERIVED + proven by the §0 non-regression property |
| **Settings** (`USettingsMenuWidget`) | **3** (2 on the unhappy path) | **3** (2 unhappy) | 0 | — · limb 3 **considered and refused**: `RootPanel`'s two buttons fail both stepper discriminators | DERIVED (`ConfirmToggleCheckBox` · `GraphicsButton` · `BackButton`; unhappy path = `SetIsEnabled(false)` on the check box) |
| **Graphics** (`USiegeGraphicsMenuWidget`) | **24** | 🚨 **19** | **−5** | **−2 limb 2** (`KeepSettingsButton`, `RevertSettingsButton` — inside the Collapsed `VideoModeConfirmBorder`) · **−3 limb 3** (`ScreenResolutionNextButton`, `WindowModeNextButton`, `FrameRateLimitNextButton`) | before **MEASURED** (`of 24`, and the index map below reproduces all four of `TASK-1413`'s observed indices); after DERIVED |
| **Login** (`UAccountMenuWidget`) — `Chooser` | **3** | **3** | 0 | — | DERIVED from `ApplyMode` |
| … `CreateForm` | **5** | **5** | 0 | — | DERIVED |
| … `LoginForm` | **4** | **4** | 0 | — | DERIVED |
| … `CloudLinkForm` | **5** | **5** | 0 | — | DERIVED |
| … `LoggedIn` | **2–3** | **2–3** | 0 | — (`LinkCloudButton`/`SyncNowButton` owned by `RefreshCloudBlock`) | DERIVED |
| **Deck builder** (`WBP_DeckBuilder`) | **3–4** | **3** details closed / **4** details open | **−1 when the details panel is closed** | **limb 2** (`Btn_DetailsClose`, inside the Collapsed `DetailsPanel` Border — `qa/TASK-1424.md` WARN-3) | before per `qa/TASK-1424.md`; after DERIVED — ⚠️ see §6 |
| **Session** (`WBP_SessionMenu`) | **4** | **4** | 0 | — · limb 1 changes **reachability**, not the count (see §4) | before MEASURED off the `.uasset` (`qa/TASK-1426.md` §0/§5); after DERIVED |

**⭐ WHY THE LOGIN SCREEN DOES NOT MOVE, STATED BECAUSE IT LOOKS LIKE IT SHOULD:** `UAccountMenuWidget::ApplyMode` collapses **the controls themselves** (`SetShown(CreateAccountButton, bChooser)` …), never a container. The pre-existing own-visibility clause already handled that correctly. Limb 2 finds nothing to remove there. Limb 3 finds nothing: `RootPanel` holds **seven** `UButton`s, so `FindStepperPair`'s `Siblings.Num() != 2` guard refuses outright.

### §2.1 — The Graphics index map, before and after (this is the verifier's crib sheet)

`RootPanel` child order ⇒ `UWidgetTree::ForEachWidget` depth-first pre-order:

| idx | BEFORE (24) | idx | AFTER (19) |
|---|---|---|---|
| 0 | ShowFrameRateCounterCheckBox | 0 | ShowFrameRateCounterCheckBox |
| 1 | AutoDetectButton | 1 | AutoDetectButton |
| 2 | OverallQualitySlider | 2 | OverallQualitySlider |
| 3–12 | ten quality-group sliders | 3–12 | ten quality-group sliders |
| 13 | ResolutionScaleSlider | 13 | ResolutionScaleSlider |
| 14 | ScreenResolutionPrevButton | 14 | ScreenResolutionPrevButton |
| **15** | **ScreenResolutionNextButton** | 15 | WindowModePrevButton |
| 16 | WindowModePrevButton | 16 | VSyncCheckBox |
| **17** | **WindowModeNextButton** | 17 | FrameRateLimitPrevButton |
| **18** | **KeepSettingsButton** ⛔ | **18** | 🚨 **BackButton** |
| **19** | **RevertSettingsButton** ⛔ | — | — |
| 20 | VSyncCheckBox | — | — |
| 21 | FrameRateLimitPrevButton | — | — |
| **22** | **FrameRateLimitNextButton** | — | — |
| 23 | BackButton | — | — |

**The BEFORE column reproduces every index `TASK-1413` printed** — `17 -> 18 ('KeepSettingsButton')`, `20 -> 19 ('RevertSettingsButton')`, `BackButton` at 23, and the total `of 24`. Four independent confirmations that the model is the binary.

🚨🚨 **THE ONE THING THAT WILL FOOL A VERIFIER, AND IT IS A BOOBY TRAP I AM DEFUSING IN ADVANCE.** `TASK-1421`'s rider says *"a SEVENTH consecutive `17 -> 18` is a `VERIFY-FAILED`."* **After this fix, `17 -> 18` is the CORRECT line** — index 17 is now `FrameRateLimitPrevButton` and index 18 is `BackButton`. ⇒ **THE DISCRIMINATOR IS NOT THE NUMBERS. IT IS THE TWO FIELDS BESIDE THEM:**

| Reading | Verdict |
|---|---|
| `focus moved 17 -> 18 of **19** ('**BackButton**')` | ✅ **PASS** — the ring crossed |
| `focus moved 17 -> 18 of **24** ('**KeepSettingsButton**')` | ⛔ **VERIFY-FAILED** — old binary, or the fix did not take |

**Score on `of N` and on the quoted NAME, never on the two integers.** And the whole-ring falsifier is cheaper still: from stop 0, **`IA_MenuDown` ×18 must reach `BackButton`, and ×19 must wrap to `ShowFrameRateCounterCheckBox`.**

---

## §3 — (6) THE MAIN MENU IS STILL 7, AND THIS IS A DERIVATION, NOT AN ASSERTION

- **Limb 1** changes no stop's admission — it changes only which stop is *reported as focused*. Count untouched by construction.
- **Limb 2** cannot remove any of the seven: `TASK-1413` walked the full main-menu ring **in both directions with the wrap**, which means `FSlateApplication::SetUserFocus` **succeeded on all seven**, which means `FindPathToWidget` reached all seven, which means every ancestor of all seven passed the `EVisibility::Visible` filter — **the exact predicate limb 2 now applies.** I can name no collapsed ancestor because there is none.
- **Limb 3** cannot touch them: the seven buttons are children of one `VerticalBox` ⇒ `FindStepperPair`'s `Siblings.Num() != 2` guard refuses before either discriminator is consulted.

**`Tests/SiegeMenuInputTest.cpp` is unedited and still passes by the same argument:** it runs on `L_MainMenu` and asserts through `GetMenuButtons()` / `GetFocusedMenuButton()` — **neither of which this diff touches** — plus `MoveFocus` behaviour on a 7-stop ring that does not move.

---

## §4 — (5) THE TWO EXTRA CALLERS: **FIXED, AND FIXED AT THE ROOT RATHER THAN AT THE SITES**

`HandleMenuAccept` and `StepFocusedStop` **do not carry their own blind read.** Both call `GetFocusedNavStop()`:

- `HandleMenuAccept`: `UWidget* FocusedStop = GetFocusedNavStop();`
- `StepFocusedStop`: `UWidget* Focused = GetFocusedNavStop();`

⇒ **limb 1(a) repairs all three callers in one place.** After this diff neither can log `'None' (none focused)` at a ring that is visibly in a field:
- `HandleMenuAccept`'s `"…has no Accept semantics (or nothing is focused)"` line will now name the text box and its class — **an honest "Accept has no semantics for a `UEditableTextBox`"**, which is `TASK-1409`'s deliberate design, instead of a false "nothing is focused".
- `StepFocusedStop`'s `"declined: no focus stop holds focus"` line will no longer fire on a focused text box; it now falls through to the `(4b)` line naming the box and its class.

**A FOURTH blind read exists and is DELIBERATELY NOT CHANGED — declared, not missed:** `GetFocusedMenuButton()` iterates `Button->HasUserFocus(PC)` over `GetMenuButtons()`. It is refused because (i) it is `UButton`-only and `SButton` does **not** forward focus to a descendant, so the false negative is unreachable there; and (ii) it is the predicate `TASK-1400`'s `ApplyInitialFocus` guard and `SiegeMenuInputTest.cpp` both read through, and moving it is a behaviour change with no defect behind it.

---

## §5 — (1a) THE TEXT-BOX PREDICTION'S STATUS: **NEITHER CONFIRMED NOR REFUTED**

Recorded exactly as the row demands. `TASK-1413` **tested for reachability and did not assume it**: a 29-node main-menu `ui_snapshot` and whole-tree class searches on Settings and Graphics all returned **zero `UEditableTextBox`**. ⇒ **the prediction was not reachable in that binary. That is not a pass.**

It goes **live** for the first registered screen carrying a text-box stop, and **two such screens are already authored**: `UAccountMenuWidget` (four `UEditableTextBox`es: `NameInputBox`, `EmailInputBox`, `PasswordInputBox`, `ConfirmPasswordInputBox`) and `WBP_SessionMenu` (`AddressTextBox`, stop 3 of 4). The **session screen is where limb 1 pays out first and visibly**: `qa/TASK-1426.md` §6 derives that with the blind read, once the ring enters `AddressTextBox` the reachable set collapses to `{Join, Back, Address}` and **`HostButton` — the screen's primary action — goes keyboard-unreachable**. Limb 1 restores it. That is runtime criterion (ii).

⚖️ **`qa/TASK-1418.md`'s WARN on limb 2 is SUPERSEDED ON EVIDENCE, NOT RE-OPENED AS A QA ERROR.** Its reasoning — Accept on a phantom stop is a guarded no-op — **is still true**. What aged is the **SEVERITY**, not the finding: it modelled the cost as two wasted presses and predicted *"the outline appears to vanish for exactly two presses"*; the measured cost is a severed ring. The gate ruled **pre-compile, from source**, and nothing available to it could have shown traversal failing. `SC-§39`: the example aged, the finding did not.

---

## §6 — ⚠️ DELETION CENSUS (complete; deletions are expected on this row)

| # | File | Deleted | Replaced by | Behaviour change? |
|---|---|---|---|---|
| 1 | `.cpp` `IsNavFocusStop` | `return Button->GetIsFocusable();` (1 line) | `if (!Button->GetIsFocusable()) { return false; }` + limb-3 block + `return true;` | **None for any non-`Next`-member button.** Identical truth value. |
| 2 | `.cpp` `GetFocusedNavStop` | the single loop `for (UWidget* Stop : Stops) { if (Stop->HasUserFocus(PC)) return Stop; }` (6 lines) | two loops; pass 1 is the old loop **plus a `Stop &&` null guard**, pass 2 is the descendant question | **None except the intended one.** Pass 1 is byte-equivalent in effect; pass 2 only ever fires where the old code returned `nullptr`. |
| 3 | `.cpp` `GetFocusedNavStop` | 3 comment lines of the *"DELIBERATELY UNCHANGED"* block reworded | same claim, re-attributed to `TASK-1406` + a note that `EFocusCause` is still untouched | **Comment only.** The old wording (*"This row neither widens nor narrows that"*) would have read as a contradiction beside the new limb. |
| 4 | `.cpp` `FocusFirstNavStop` | `if (Stop->HasUserFocus(PC))` (1 line) | `if (Stop && (Stop->HasUserFocus(PC) \|\| Stop->HasUserFocusedDescendants(PC)))` | **Intended** — limb 1(b). |
| 5 | `.h` | the one-line `/** The focus stop holding … */` doc | block doc describing the two-pass read | **Comment only.** |
| 6 | `.h` | the one-line `/** True when `Widget` is one of the four admitted classes AND is visible… */` doc | block doc naming effective visibility + the stepper rule | **Comment only.** |

**Additions:** `.cpp` — one `#include "Widgets/SWidget.h"`, the `HasVisibleSlateAncestry` definition + its header comment, the limb-2 clause, the limb-3 block, the limb-1(a)/(b) comment blocks. `.h` — the `HasVisibleSlateAncestry` declaration (private static) + doc, and the three doc-comment expansions. **No new member, no new `UPROPERTY`, no new GC surface, no `UFUNCTION`, no reflection change, no log-string change.**

**⛔ THE `TASK-1394` INSTRUMENT LINES AND THE ACCEPT LINE ARE BYTE-UNTOUCHED**, including the one cited by its text: `"[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."` **No `UE_LOG` format string anywhere in this file was added, removed or altered by this diff.**

**C4458 SWEEP (the trap that cost `TASK-1423` a build).** New locals: `SelfSlate`, `Ancestor`, `StepperNext`, `PairPrev`, `PairNext`. Checked against the **base-class** chain `UWorldSubsystem` → `USubsystem` → `UObject`/`UObjectBase` (not against the project, which is the false-negative-by-construction mistake): no member named `Next`, `Prev`, `Slate`, `Ancestor`, `Stepper*` or any prefix of these exists. Checked against `USiegeMenuInputSubsystem`'s own members: no collision. `Stop`, `Stops`, `PC`, `Button` are names this file already compiles with.

**Const-correctness verified at engine headers:** `UWidget::GetCachedWidget() const` (`Components/Widget.h:857`), `SWidget::GetParentWidget() const` (`Widgets/SWidget.h:704`), `SWidget::GetVisibility() const` (`:1065`), `UWidget::HasUserFocusedDescendants(APlayerController*) const`. `FindStepperPair` and `GetButtonLabel` are already `static`, so the `static` `IsNavFocusStop` may call them.

---

## §7 — RUNTIME CRITERIA, RESTATED BEFORE 5b (both pre-written so they can falsify me)

**(i) GRAPHICS — the ring must CROSS.** From stop 0, repeated `IA_MenuDown` reaches `BackButton` and wraps; `IA_MenuUp` crosses the other way.
- ✅ PASS shape: `MoveFocus(+1): focus moved 17 -> 18 of 19 ('BackButton').`
- ⛔ FAIL shape: any line reading `of 24`, or any repeat of `('KeepSettingsButton')` / `('RevertSettingsButton')` while no video-mode change is pending.
- Total presses from stop 0: **18 Down to reach `BackButton`, 19 to wrap.** Stepper rows are now **one** stop each, so the ring is `checkbox · AutoDetect · 11 sliders · ScreenRes · WindowMode · VSync · FrameRateLimit · Back`.

**(ii) SESSION — `Down` ×4 from `HostButton` must end `3 -> 0 ('HostButton')`, NOT `0 -> 1 ('JoinButton')`.** The 4th press starts from `AddressTextBox` (stop 3). Sound walker ⇒ `Current` reads **3** and wraps to 0. Defective walker ⇒ `Current` reads cold **0** and goes to 1. **`MoveFocus` already prints the index it read, so this discriminator costs zero instrumentation.** Quote the line verbatim either way — it is this epic's only direct witness for the walker defect, and `TASK-1473` (the slot reorder) would make the cold-branch fallback *coincidentally correct* and mask it forever.

**cl. 3(b) — THE HAND SENTENCE FOR 🧑 JONATHAN, verbatim from the row:**

> *"🧑 Open Settings → Graphics and hold Down — does the outline get all the way to the Back button at the bottom, or does it stick partway?"*

**+ the false-positive parenthesis.** ⚠️ **DECLARED: the row supplied the sentence but NOT a parenthesis, so I authored this one to the cl. 3(b) requirement; the manager may overrule the wording, but the sentence must not ship without one.**

> *(Keyboard only — if you touch or click the mouse at any point the answer does not count, because clicking places the ring directly and can reach a stop that Down could never walk to. And if it does stick, say **which row it sticks on**, not just that it stuck: sticking at the Frame Rate Limit row means one thing and sticking two rows above the confirm bar means another.)*

---

## Not examined / limitations

1. **⛔ NOT COMPILED, NOT RUN, NOT PIE'd.** That is the fence. Everything above is source-level derivation plus `TASK-1413`'s and `qa/TASK-1426.md`'s prior measurements.
2. **`SWidgetSwitcher` IS NOT MODELLED.** Limb 2 is ancestor **visibility**. `SWidgetSwitcher` arranges only its **active** child regardless of the inactive children's own visibility, so a stop parked in an inactive switcher slot would still be admitted and would still refuse focus. **No registered nav screen uses one today** — the project's only `UWidgetSwitcher` is `SiegeControlsHelpWidget`'s `ViewSwitcher`, and that overlay never registers. **Latent hazard for a FUTURE screen; declared, not fixed** (`SC-§50`: a declared gap with no row ships).
3. **ANCESTOR *ENABLED*-NESS IS NOT TESTED,** only ancestor visibility. `SWidget::IsEnabled()` does not propagate through the parent chain the way visibility does, and `FindPathToWidget`'s default `EWidgetPathSearchPurpose::Standard` does **not** apply its enabled short-circuit (only `FocusHandling` does). Out of this row's scope; named so nobody assumes it was covered.
4. **A HALF-COLLAPSED STEPPER PAIR WOULD LOSE ITS ROW.** If an author ever collapses only the `Prev` member and leaves `Next` visible, limb 2 drops `Prev` and limb 3 drops `Next` ⇒ the row gets **zero** stops. No shipped screen does this (`BuildStepperRow` never sets visibility on either member). Declared.
5. **THE DECK-BUILDER "AFTER" IS A PREDICTION, NOT A MEASUREMENT** — and ⚠️ **`TASK-1423` was editing `DeckBuilderWidget.{cpp,h}` while I wrote this.** My reading of that screen is from `qa/TASK-1424.md` WARN-2/WARN-3 plus source as it stood at my instant. If `TASK-1423`'s diff adds, removes or re-parents a button, **my deck-builder row is the one to re-derive**, and the Δ I attribute to limb 2 (`Btn_DetailsClose`) is the one to re-check.
6. **`Btn_Jump` DOES NOT REGRESS, AND THE LIMB THAT EXCLUDES IT IS THE PRE-EXISTING ONE.** It is `Collapsed` **in its own right** (`WBP_DeckBuilder`'s `Construct`), so the untouched first clause `!Widget->IsVisible()` refuses it exactly as before — **not** limb 2, which never gets asked. `TASK-1413` corroborated this at runtime and the widened walker did not admit it. `qa/TASK-1407.md` WARN-3's ambiguity stays resolved in the same direction.
7. **THE DECK BUILDER'S 34-TILE GRID RING IS UNAFFECTED.** Those tiles are `WBP_CardTile_C` **`UUserWidget`s**, and `UWidgetTree::ForEachWidget` recurses only into `UPanelWidget`s — a nested `UUserWidget`'s own tree is never walked. They were never subsystem stops and still are not. The grid ring is the deck builder's own navigation (`AcquireBuilderFocus`), untouched by this row.
8. **THE SETTINGS / LOGIN / DECK-BUILDER "BEFORE" NUMBERS ARE DERIVED, NOT MEASURED.** Only **main menu (7)**, **Graphics (24)** and **Session (4)** have independent measurement behind their BEFORE column. A 5b reading that disagrees with a DERIVED number is a **finding about my derivation**, not automatically a defect in the binary.
9. **PER-CALL COST WENT UP, BY A BOUNDED AMOUNT, AND IT BELONGS TO `TASK-1456`.** `IsNavFocusStop` now pins one `TWeakPtr` and walks a parent chain (~8 deep) for **every** enabled+visible widget in the tree, and calls `FindStepperPair` (a sibling scan plus up to two label walks) for every focusable `UButton`. On the worst screen (Graphics, ~60 walked widgets) that is a few hundred weak-pointer pins per `GetMenuFocusStops()`, and `MoveFocus` calls it twice per press. Measured in microseconds, not a hot path — **but it is a real increase and `TASK-1456` is the row that owns it.** I did not optimise the ordering (the class check is cheaper than the ancestry walk and could precede it) because restructuring the four class returns would add deletions and review surface for no correctness gain.
10. **`GetFocusedMenuButton()` LEFT BLIND ON PURPOSE** — §4 gives the reason.
11. **NO `SC-§50` DRIFT INTO ROW B.** The per-screen *"this screen drives itself"* flag (`TASK-1471`) is **not implemented and not stubbed**. No screen file, no `.uasset`, no `DeckBuilderWidget` line.

---

## What QA should scrutinise hardest

1. **The (2a) letter (A) argument in §0** — specifically premise (A-ii): *does the existence of the log line `17 -> 18 of 24 ('KeepSettingsButton')` really prove `GetCachedWidget()` was valid?* If that inference is wrong, the whole "one limb" answer is wrong and a realization test is owed.
2. **The two-pass read in `GetFocusedNavStop()`** — is the ancestor-precedence hazard real, and is two passes the right answer rather than a fused `||`?
3. **§2.1's booby trap** — confirm the `of 19` / `('BackButton')` discriminator is written where `TASK-1421`'s verifier will see it before pressing Down.
4. **Limb 3's `Prev`-survives choice**, and the near-miss in §1: verify independently that `FindStepperPair` really refuses `AutoDetectButton`/`BackButton` and `GraphicsButton`/`BackButton`. **If it does not, this diff deletes a `BackButton` and I have shipped the defect I was sent to fix.**
5. **The Graphics index map** — it is a source derivation that happens to reproduce four measured facts. Check the `RootPanel` child order independently.
