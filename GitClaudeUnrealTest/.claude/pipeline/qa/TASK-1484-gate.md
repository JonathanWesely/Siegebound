# QA Report — TASK-1484 [CONTROLS-HELP-DETAIL-SCROLL]

**Verdict: PASS** — 0 BLOCKER · 4 WARN · 3 NIT
**Marker:** `TASK-1484-GATE-PASSED-2026-09-25`
**Subject:** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.{cpp,h}` · handoff `.claude/pipeline/handoffs/TASK-1484-programmer.md`
**Gate row:** ⛔ **NONE EXISTS** — see §0. This report is filed at `qa/TASK-1484-gate.md` and I flipped **only** `TASK-1484`'s own `status:`.

---

## 0. ⛔ BOARD GAP — no gate row was minted for this subject

Searched three ways before concluding: (a) `^#### TASK-14(8[4-9]|9[0-9])` over `TASKBOARD.md` → only `TASK-1484` (`:8056`) and `TASK-1485` (`:8083`); (b) literal `TASK-1486` / `TASK-1487` / `TASK-1488` → **0 matches anywhere on the board**; (c) `qa/TASK-1484` as a string across the whole `.claude/pipeline` tree → **0 matches**. There is nothing between `:8081` and `:8083`.

⇒ I did **not** invent a row ID. `TASK-1484`'s own `status:` is the only board cell I wrote. **The manager still owes this subject a gate row**, and when it is minted it should point at this file rather than at a `qa/TASK-14xx.md` that does not exist.

---

## 1. 🧑 THE ACCEPTANCE ANSWER, FIRST, IN HIS TERMS

> **"Can an agent read the text below the fold?"**

**The wire is complete and every link in it was read at source. The reading itself has not been taken — and one gap in the instrument means 5b must not try to take it from the log alone.**

**What I verified, link by link, none of it assumed:**

| # | link | evidence |
|---|---|---|
| 1 | the detail page now really has **two** stops | `ConstructDetailTree` builds exactly two `UButton`s; the whole file contains **4** `ConstructWidget<UButton>` calls (`:1957` RowButton, `:2481` BackButton, `:2644` DetailScrollButton, `:3284` CloseButton) and **zero** `UCheckBox`/`USlider`/`UEditableTextBox`. The detail set is a **property**, not a tally. |
| 2 | `IA_MenuDown` reaches stop 1 | `DetailScrollButton` is in the **active** switcher branch when the detail page is up, and `ApplyButtonFocusable` (`:2649`) is called **in the construction pass** — before `UButton::RebuildWidget` reads the field once. |
| 3 | the action is **not** dropped before it arrives | the screen registers with `RegisterMenuNavTarget(this)` (`:4410`), **not** `RegisterSelfDrivingMenuNavTarget` — the only caller of that is `DeckBuilderWidget.cpp:2155` ⇒ `DeclineIfActiveTargetSelfDriving` returns false here. |
| 4 | Accept is not self-disabled by coverage | `IsNavTargetActionable()` returns `true` **unconditionally** whenever a screen is registered (`SiegeMenuInputSubsystem.cpp:453-468`) ⇒ `qa/TASK-1459-verify.md`'s *"the project's own Accept path self-disables while the menu is covered"* finding **cannot bite on this screen**. |
| 5 | Accept presses the button | `HandleMenuAccept` ends in `Focused->OnClicked.Broadcast()` (`:1618`) — the same delegate a mouse click fires. |
| 6 | the press reaches the code | `HandleScrollButtonClicked` is a `UFUNCTION()` (required — `OnClicked` is a dynamic multicast), bound `AddUniqueDynamic` in `NativeConstruct` (`:2708`) and removed in `NativeDestruct` (`:2721`). |
| 7 | the code moves the box | `AdvanceBodyScroll()` → `DetailScrollBox->SetScrollOffset(Target)` with `Target` clamped to `[0, GetScrollOffsetOfEnd()]`. |
| 8 | focus survives the press, so it **pages repeatedly** | nothing on the path writes focus, visibility, registration or the switcher index; `SScrollBox::SetScrollOffset` only assigns and `Invalidate(Layout)`. And the button is a **sibling** of the box, so scrolling never moves the focused widget out from under the ring. |

**What I did not verify and cannot:** that an offset moved on a running screen. No PIE, no screenshot, no MCP mutation. `qa-passed` here is a **text-level verdict** (`SC-§71b`).

**⛔ And the distinction his sentence turns on, stated plainly, because it is the one thing this row's instrument does not fully close:**

`SScrollBox` keeps the *asked-for* offset and the *painted* offset in **two separate fields**. `GetScrollOffset()` returns `DesiredScrollOffset` — the value the write lands in (`SScrollBox.cpp:496-499`, `:596-602`). What the reader actually sees is `ScrollPanel->PhysicalOffset`, derived each Tick through `GetViewOffsetFraction()`, which **does** clamp (`:587-594`). ⇒ **the shipped log line proves the box was asked to hold a legal new offset. That is one rung nearer the screen than "a handler ran" — it is a claim about the widget — but it is still not "text was displayed."** The row's own clamp is exactly what makes the asked-for value a truthful predictor of the painted one, and I verified that clamp at both ends. **A predictor is not a photograph.**

⇒ **5b must pair the `MOVED` line with a before/after screenshot of the same overflowing page.** Log-only would once again be a claim about the code. Filed as **WARN-2**, not a BLOCKER, because it is a requirement on the *sitting*, not a defect in the *row*: the row was told to state its offset mechanism before 5b and it did so, accurately and at source.

---

## 2. ⭐ THE (γ) MEASUREMENT — RE-RUN AT ITS SOURCE, AND IT HOLDS

The row refused shape (γ) on a quotation it did not re-run and said so (`handoff §8.9`). I re-read `qa/TASK-1459-verify.md` directly. **The quotation is verbatim and the control behind it is sound — this is a real null, not a blind instrument:**

- `SpaceBar` **is** a Slate navigation-Accept key (`NavigationConfig.cpp:33`, cited there) ⇒ a Slate key event arriving at a focused `SButton` **would** have fired `SButton::OnKeyDown` → `ExecuteOnClick()`.
- `SpaceBar` is **not** bound in `IMC_MainMenu` — `binding_found:false`, with the tool's own *"NOTHING BINDS IT"* warning ⇒ the Slate route was the **only** route it could have taken.
- **Slate focus was present.** The report's item 3 says it in terms: *"because Slate focus was present on the main menu (`focused: true`) yet SpaceBar still did nothing, the failure is at key delivery, not at focus."* This is the limb that makes the null mean something.
- Nothing happened — `mean_luma` frozen at **193**, with an evidence PNG.
- The discriminating positive, same button / same screen / same session: `inject_input_action IA_MenuAccept` → Settings opened, `mean_luma` **50**, with its own PNG. The observable that stayed frozen for the key **swung** for the proven verb.

**Ruling: the measurement is right, and it is the right *kind* of right.** The property it establishes belongs to the **verb**, not to that screen: had `simulate_key_press` produced an `FKeyEvent` entering `FSlateApplication::ProcessKeyDownEvent` at all, **both** `SButton::OnKeyDown` **and** the ancestor preview tunnel would have received it. Neither did. ⇒ a `NativeOnPreviewKeyDown` scroll key would be **code the agent lane can never reach** — it would compile, review clean, satisfy a code path and **fail his sentence**. `SC-§36.1` exactly.

⭐ **(γ) was killed by a measurement rather than by taste, and that is the best available reason for a shape.** It is this row's strongest single decision and I am recording it as such: the alternative argument the author had prepared (that widening `NativeOnPreviewKeyDown` was not a relapse) was *available and probably sound*, and he discarded it because it answered the wrong question. That is the correct instinct.

**Forward-compatibility claim — CONFIRMED.** `AdvanceBodyScroll()` is `public`, parameterless, has exactly one caller, and writes nothing but one float on one scroll box. A future `PageDown` clause in `NativeOnPreviewKeyDown` would be a **one-line addition** calling the same function: it removes nothing, unbinds nothing, and does not change the stop set (the button simply remains a second route). ⇒ **if the agent lane ever gains a true Slate-key verb, (γ) is a pure addition and nothing here needs undoing.** One caveat for whoever writes it: the clause must sit **below** the two existing guards, so `Escape` still returns `Super::` first (`HELP-§5`, a closed 🧑 ruling) and the derived toggle key still wins.

---

## 3. 🚨 THE STEPPER HAZARD — RULING: CORRECTLY CREATED AND CORRECTLY CLOSED

`DetailColumn` now holds exactly two `UButton`s ⇒ `FindStepperPair`'s count guard (`SiegeMenuInputSubsystem.cpp:1958`) **passes**. Both discriminators verified **independently**, each sufficient alone:

- **Names** (`:1970-2004`) — requires `EndsWith("PrevButton")` + `EndsWith("NextButton")`, `CaseSensitive`, with **equal bases**, in either slot order. `BackButton` matches neither suffix; `DetailScrollButton` matches neither. **Refused on this discriminator alone.** ✓
- **Glyphs** (`:2006-2023`, constants at `:68-69`) — requires the **trimmed** labels to be exactly `<` and `>`. Measured at source: `BackLabel = TEXT("Back to the controls list")` (`SiegeControlsHelpWidget.cpp:91`) and `ScrollDownLabel = TEXT("Scroll down — back to the top at the end")` (`:108`). **Refused on this discriminator alone.** ✓

Either one would hold if the other were broken. For a hazard whose failure mode is **two silent failures at once** — `IsNavFocusStop` dropping the `Next` member (the page silently loses a stop, possibly `BackButton` itself) *and* `StepFocusedStop` making Left/Right **press the partner**, i.e. an arrow key that leaves the page — redundancy is the correct standard. **Not raised as a WARN.**

**The third trip-wire is planted and it names the mechanism.** `SiegeControlsHelpWidget.cpp:2606-2615`, heading the block that contains `ApplyButtonFocusable(DetailScrollButton)` (`:2649`). It names `USiegeControlsHelpWidget::ApplyActiveView`, names the `ViewSwitcher` collapse as the only thing preventing the failure, and names the failure exactly: *"a SECOND admitted-but-unfocusable stop that SWALLOWS THE RING every time the list is up — with every visibility flag green and nothing in any log."* It **inherits** the `ValidatePathToChild` → `FindPathToWidget` → `SetUserFocus` chain from `BackButton`'s wire by reference rather than restating it — the right call. Both existing wires are **intact and unedited**: `RowButton` at `:1999-2006`, `BackButton` at `:2517-2545`. ⇒ **three focusable flip sites, three wires.**

The hazard is additionally written at **three** places as the handoff claims — declaration (`SiegeControlsHelpWidget.h:770-778`), construction site (`.cpp:2617-2630`), and `RegisterAsMenuNavTarget`'s count comment (`.cpp:4399-4401`). The last is the one that earns its keep: it is where a future reader goes to ask *"why is this two?"* and it tells them a rename makes it silently **one**.

⭐ **Promoting the milestone-level point out of this row, because it is now the fourth of its kind:** Settings footer · Graphics `RootPanel` · `VideoModeConfirmButtonRow` · `DetailColumn`. **All four** are two-`UButton` parents that pass the count guard and are refused only by the **authored** discriminators — `IsNavFocusStop`'s own comment already says this for the Settings footer (`:915-917`). ⇒ **the count guard is not the safety property; the two discriminators are.** A structure-only stepper rule would by now have broken four screens. This belongs in `CONVENTIONS.md` as a standing note, and I flag it for the manager rather than writing it myself.

---

## 4. THE DESIGN — VERIFIED, NOT RE-DERIVED

**Pre-order, checked against the shipped walker rather than against the row's word.** `CollectNavStopsFromTree` (`SiegeMenuInputSubsystem.cpp:975-1008`) runs `Tree->ForEachWidget`, whose implementation is `Predicate(Root)` then `ForWidgetAndChildren`, which calls `Predicate(ChildWidget)` **before** recursing, iterating children in **slot order** (`Engine/Source/Runtime/UMG/Private/WidgetTree.cpp:266-274`). ⇒ **depth-first pre-order, children in slot order.** `BackButton` is added to `DetailColumn` at `:2561`, `DetailScrollButton` at `:2667` ⇒ **`BackButton` is stop 0.** ✓ Every existing sentence about where the ring lands on a detail page survives — including `FocusFirstNavStop()`'s landing and `TASK-1478`'s declaration.

**LIST 28 unchanged** — nothing in the diff touches `RowScrollBox`, any row widget, or `CloseButton`; the detail subtree is `Collapsed` by `ApplyActiveView` while the list is up, so `HasVisibleSlateAncestry` drops both new widgets. ✓

**⭐ THE ENGINE TRAP — the fix is correct at both ends, and the trap is real.** Verified in 5.8 source:
- `SScrollBox::SetScrollOffset` assigns **raw**: `DesiredScrollOffset = NewScrollOffset;` — no clamp (`SScrollBox.cpp:596-602`).
- **Only `ScrollBy` clamps**: `DesiredScrollOffset = FMath::Clamp(DesiredScrollOffset + LocalScrollAmount, ScrollMin, ScrollMax);` (`:1236`).
- `GetScrollOffset()` returns `DesiredScrollOffset` **verbatim** (`:496-499`) — so an unclamped write would be **read back verbatim**, while `GetViewOffsetFraction()` clamps the *rendered* position (`:587-594`) and **`DesiredScrollOffset` itself is never re-clamped by Tick**. ⇒ the lie would be **permanent**, not a one-frame artefact. The author's description is exactly right and the clamp is load-bearing.
- `GetScrollOffsetOfEnd()` = `FMath::Max(ContentSize - PanelSize, 0.0f)` (`:572-577`) ⇒ **never negative**.

**Clamp correctness at both ends:**
- **Upper:** `FMath::Clamp(OffsetBefore + Step, 0.f, OffsetOfEnd)` — pinned to the end. ✓
- **Lower:** `Min = 0 ≤ Max = OffsetOfEnd` always (previous bullet) ⇒ no inverted-range pathology; a negative `OffsetBefore` from an overscroll path is caught by the `0.f` floor. ✓
- **Wrap:** `bAtEnd ⇒ Target = 0`, and `0 ∈ [0, OffsetOfEnd]`. ✓
- **No stuck state:** a reader parked fractionally short of the end gets `Target = OffsetOfEnd` (a short `MOVED`), and the *next* press satisfies `bAtEnd` and wraps. Verified by walking the arithmetic, not assumed. ✓
- **The real negative:** prose fits ⇒ `OffsetOfEnd == 0` ⇒ `bAtEnd` true ⇒ `Target = 0` ⇒ `0.0 -> 0.0`, `bMoved == false`, terminal word `NO MOVEMENT`, and `end 0.0` on the same line discriminates it from a failure. **This limb can fail, and it fails honestly.** ✓

**The 5b instrument is `Log`, not `Verbose`.** `LogSiegeControlsHelp` is declared `(…, Log, All)`, so the line prints in Development with **no console verb typed first** ⇒ **an absent line is a ZERO and a defect signature, never a void.** ✓ Both offsets ride on **one** line with the page's `ActionId`, so a verdict cannot be assembled from two lines that might have come from two presses. ✓ (See WARN-2 for the one thing the line does *not* establish, and NIT-3 for the one pair of states that share a terminal word.)

---

## 5. WHAT MUST NOT HAVE MOVED — CHECKED

| claim | result |
|---|---|
| `NativeOnPreviewKeyDown` unchanged | **Content-verified unchanged** (`:4491-4542`): `Escape` → `Super::` first and unconditionally (`HELP-§5` intact); repeat/`!bHelpOpen` guard; every non-toggle key handed straight on; `CloseHelp()` + `Handled` only for the derived toggle. **No scroll key, no new key, no new `Handled`.** ⚠️ I cannot assert *byte*-identity — I hold no git verb (see §8). |
| ancestor-chain cover extends to the new button | **Confirmed by construction.** Slate's preview tunnel routes root→leaf along the focus path; `DetailScrollButton` sits under `DetailColumn` → `DetailBackdrop` → `DetailView` → `ViewSwitcher` → the overlay's own `SObjectWidget`, so the overlay's `NativeOnPreviewKeyDown` is an ancestor of it. **More focusable stops ⇒ more `Tab` converters, all covered.** ✓ |
| `ApplyActiveView` unedited | ✓ — all **11** `TASK-1484` sites in the `.cpp` are at `:94, 267, 2383, 2499, 2508, 2582, 2679, 2699, 2742, 3609, 4393`; **none** falls inside `ApplyActiveView` (`:3918`+), `NativeOnPreviewKeyDown` (`:4491`+) or `ConstructHelpTree`'s list branch. |
| both existing trip-wires intact | ✓ — `:1999-2006`, `:2517-2545`, unedited. |
| show-edge registration · `registered ⟺ bHelpOpen` | ✓ — `ApplyOpenState`'s only `TASK-1484` touch is a **comment amendment** at `:3609-3611` correcting the count and explicitly stating the invariant did not change. |
| `HELP-§3` escape hatch | ✓ — `ConstructDetailTree` returns at `WidgetTree->RootWidget != nullptr` (`:2231-2234`) **before** any new construction; both new children are `BindWidgetOptional`, built only while still null. An asset-authored `WBP_ControlsDetail` still wins whole with zero C++ change. |
| 🧑 **human path byte-for-byte** | ✓ — `SetAlwaysShowScrollbar(true)` (`:2372`), `SetIsFocusable(false)` (`:2395`), `SetConsumeMouseWheel(WhenScrollingPossible)` (`:2396`), `SetOrientation`, the box's slot padding/alignment/`Fill`, and `ScrollToStart()` on every stamp (`:2886`) — **all present and unedited**; the only change in that block is a comment **amended in place**. `RowScrollBox->SetScrollWhenFocusChanges(InstantScroll)` (`:3246`) untouched and still inside `ConstructHelpTree`, which runs **before** `Super::RebuildWidget()`. |
| `SiegeMenuInputSubsystem.{cpp,h}` not touched by this row | ✓ — **0** occurrences of `TASK-1484` in `Source/` outside the two subject files (20 total: 9 in the `.h`, 11 in the `.cpp`). No behavioural dependency on a subsystem change either: every subsystem function this row relies on was read and is the shipped one. |
| `.generated.h` **by exec-symbol set** | ✓ — on-disk **before** = exactly the predicted **9** (`execHandleRowButtonClicked`, `execHandleBackButtonClicked`, `execHandleCloseButtonClicked`, `execRefreshRows`, `execIsDetailViewActive`, `execGetSelectedActionId`, `execIsHelpOpen`, `execCloseHelp`, `execOpenHelp`). The header now carries **4** plain `UFUNCTION()` thunks (`:474`, `:678`, `:686`, `:1191`) ⇒ **after = 10**, single addition `execHandleScrollButtonClicked`, **zero removals**. `AdvanceBodyScroll()` is deliberately **not** a `UFUNCTION` ⇒ no exec symbol; the two new `UPROPERTY`s add property entries only. **Neither size nor sha was consulted.** |
| deletions **by content** | ✓ — no function, member, property, constant, log line or `#include` removed. **Three** count claims struck in place under `SC-§120` with the argument retained and the struck text left visible: `:2499`/`:2505` (`ConstructDetailTree`), `:4393` (`RegisterAsMenuNavTarget`), `:3609` (`ApplyOpenState`); plus **one** comment *amended* rather than struck at `:2383-2391`, correctly — its premise expired, its conclusion did not. |
| ⛔ automation suite **not** cited | ✓ **Re-measured independently**: `Tests/SiegeControlsHelpTest.cpp` has **0** occurrences of `AdvanceBodyScroll` / `DetailScrollButton` / `DetailScrollBox` / `BackButton` / `RowButton` / `CloseButton` / `ApplyActiveView` / `Switcher`. **A green suite says nothing about this row**, and the handoff says so itself. |

---

## 6. 🧑 VISIBLE FOOTPRINT — IS HE BEING ASKED TO ACCEPT MORE THAN NECESSARY?

**Ruling: no — with one correction he is owed and one disclosure the handoff omits.**

Verified: exactly **one** new visible element. **Nothing existing is re-styled, re-padded, re-ordered or re-parented.** `BackButton`'s slot is unchanged (`FMargin(56,0,56,34)`, `HAlign_Center`, `VAlign_Bottom` — `:2563-2565`) and the new slot copies it exactly. What changes about Back is only that its **unchanged** 34 px bottom margin now reads as a gap *between two buttons* rather than as the panel's bottom inset. The brief's *"a change to Back's padding shape"* is a change in how Back's padding **reads**, not a change to Back.

- ⚠️ **Correction he is owed (WARN-1):** the handoff (`§3`, line 110) tells him *"glyph-only is barred by the stepper-pair discriminator."* **It is not.** The glyph discriminator compares the trimmed label against exactly `TEXT("<")` and `TEXT(">")`. A compact `▼` (or `⌄`) button passes **both** discriminators and would be perfectly safe. ⇒ **a smaller affordance is an open option**, and he should adjudicate the look knowing that.
- ⚠️ **Disclosure the handoff omits (WARN-4):** `DetailScrollBox`'s slot is `ESlateSizeRule::Fill` (`:2403`) and the new slot is auto-sized with 34 px bottom padding ⇒ **the visible prose region shrinks by roughly one button-height + 34 px on every detail page**, which marginally *increases* how often a page overflows. Inherent to shape (β) and small, but it is a **second** visible consequence and §3 declares only the first.
- Against a *horizontal* placement beside Back: that would reparent `BackButton` into a new `UHorizontalBox`, which (i) recreates the identical two-button-parent hazard in a fresh parent and (ii) puts the pre-order stop-0 sentence at risk. **Not smaller in risk.** The vertical append is the right structural choice.
- In favour of the shipped text label on its merits: it names a direction **and** the wrap, types no key (`HELP-§1`/`§4`), and is more discoverable than a glyph on a help screen. The choice is **defensible** — it is simply not *forced*, and the handoff says it is.

⭐ The row **declared** the visual for him rather than adjudicating it (`AS-§6 A(e)` / `HELP-§6`). That was correct and I am not disturbing it.

---

## 7. Findings

- **[WARN-1]** `handoffs/TASK-1484-programmer.md:110` — *"glyph-only is barred by the stepper-pair discriminator"* is **false as written**: `FindStepperPair`'s glyph test matches only the bare `<` / `>` (`SiegeMenuInputSubsystem.cpp:68-69`, `:2010-2023`), so a `▼` glyph button would pass both discriminators. This is not a code defect — it is a claim defect that **narrows 🧑 his option space at the exact moment he is asked to adjudicate the look**. *Fix: correct the sentence to "only the bare glyphs `<` and `>` are barred" when the handoff is next touched, or carry the correction into the sitting brief.*
- **[WARN-2]** `SiegeControlsHelpWidget.cpp:2773` / `:2802` — the before/after reads are both `GetScrollOffset()`, which returns `DesiredScrollOffset` (`SScrollBox.cpp:496-499`) — the same field `SetScrollOffset` assigns. The read-back is **honest but not independent**: it measures the *asked-for* offset, not the *painted* one (`ScrollPanel->PhysicalOffset`). The row's clamp is what makes the two agree, and the clamp is correct — but the log line alone is a claim about the widget, not about the screen. *Fix (on the sitting, not the code): 5b must pair the `MOVED` line with a **before and after screenshot of the same overflowing page** — `Cards.StackUpgrade` is the right subject. Log-only would reproduce the very substitution his sentence rejects.*
- **[WARN-3]** `SiegeControlsHelpWidget.cpp:2764` / `:2779` — `GetCachedGeometry()` and `GetScrollOffsetOfEnd()` are both **previous-frame** reads (`UWidget::GetCachedGeometry`'s own engine comment warns of hysteresis; `SScrollBox::GetScrollOffsetOfEnd` uses `CachedGeometry` + `ScrollPanel->GetArrangedSize()`). A press landing on the first frame after a page stamp could compute `end`/`step` from the **previous** page's arrangement. Unreachable in both the human and the agent lanes (presses arrive many frames apart) and **self-disclosing** — `end`, `step` and `view` are all printed. *Recorded so a 5b reader who sees an implausible `end` reads it as staleness rather than as a defect; no code change wanted.*
- **[WARN-4]** `SiegeControlsHelpWidget.cpp:2403` + `:2667-2672` — the new auto-sized slot takes its height out of the `Fill`-sized `DetailScrollBox`, so **every detail page loses ~one button-height + 34 px of visible prose**. Inherent to the shape and small, but it is a second visible consequence and the handoff's §3 declares only the button. *Fix: name it in 🧑 his sitting brief alongside the button, so the visual he is asked to accept is described completely.*
- **[NIT-1]** `SiegeControlsHelpWidget.cpp:2788` — `bAtEnd` uses `UE_KINDA_SMALL_NUMBER` (1e-4) while the comment justifying the tolerance reasons about *"one pixel short of the end."* 1e-4 uu is not a pixel. Behaviourally harmless (verified: no stuck state — a short `MOVED` is followed by a wrap), but the constant and the comment are arguing about different quantities.
- **[NIT-2]** `SiegeControlsHelpWidget.cpp:2732-2738` — `AdvanceBodyScroll()`'s `bool` is dropped by its only caller, deliberately and with the reason written (a second log line would double every line 5b reads). Correct; noted only so a future reader does not "fix" it.
- **[NIT-3]** `SiegeControlsHelpWidget.cpp:2810-2817` — the terminal word `NO MOVEMENT` covers **two** states: *prose fits* (`end 0.0`, `view` non-zero) and *box not realised* (`view 0.0` + `NO CACHED GEOMETRY`; `UScrollBox::GetScrollOffset`/`GetScrollOffsetOfEnd` both return `0` on an invalid `MyScrollBox` — `ScrollBox.cpp:162-170`, `:192-200`). They **are** discriminable, but only by reading the whole line. *5b must read the parenthesis, not the last word.*

**No BLOCKERs.** Explicitly considered and refuted at source, each by reading rather than by inference: stepper mis-read (§3) · self-driving decline (§1 row 3) · coverage self-disable (§1 row 4) · unclamped offset write (§4) · focus lost on press (§1 row 8) · stop 0 moved (§4) · unreachable key handler (§2) · `Escape` absorbed (§5) · GC / delegate asymmetry (`UPROPERTY TObjectPtr` + `AddUniqueDynamic`/`RemoveDynamic`) · missing `UFUNCTION` on a dynamic-delegate thunk (present, `:686`) · deprecated 5.8 API (`GetCachedGeometry` is live and `BlueprintCallable`; the one deprecated write, `Button->IsFocusable`, goes through the **shipped, unedited** `ApplyButtonFocusable` helper with its existing `PRAGMA_DISABLE_DEPRECATION_WARNINGS` pair) · non-ASCII in a displayed `TEXT()` literal (two shipped precedents in-tree: `:127` in this same file, `SiegeGraphicsMenuWidget.cpp:113`).

---

## 8. Not examined / limitations

1. ⛔ **No runtime evidence of any kind.** No PIE, no screenshot, no offset read from a live widget. The acceptance sentence is **not** answered by this report; it is answered by 5b, under the constraint in WARN-2.
2. ⛔ **"Byte-identical" is asserted nowhere by me.** I hold no git verb in this lane, so `NativeOnPreviewKeyDown`, `ApplyActiveView` and the human-path properties are verified **by content and by marker census**, not by diff. That is strong (11 markers, none in those regions; every property present and correct) but it is **not** a byte comparison, and I decline to launder it into one.
3. **Inspection used, and declared:** engine C++ source read **read-only** via the inspector's `grep` / `get_text_file_contents` (`SScrollBox.cpp`, `ScrollBox.cpp`, `WidgetTree.cpp`, `Widget.h`). ⛔ **No MCP mutation, no PIE, no compile, no git, no editor-lifecycle call.** Editor left exactly as found (**PID 43320** up, no PIE started).
4. **Stop counts are argued from the generating rule + tree structure**, as the row's are — I ran no live census. My independent contribution is that I re-derived both from the **shipped walker** (`ForEachWidget` → `ForWidgetAndChildren`, pre-order, slot order) rather than from the row's word, and I re-counted the file's `UButton` constructions (4 total, 2 in the detail tree).
5. **No `.uasset` opened.** The `IMC_MainMenu` binding facts in §2 are quoted from `qa/TASK-1459-verify.md`'s own dump, not re-read from the asset.
6. **`.generated.h` "after" is a prediction**, verified only in the sense that the "before" matches exactly and the header's `UFUNCTION` census implies the "after". 5a measures it.
7. **The visual is 🧑 his**, not mine. §6 rules only on *whether he is being asked to accept more than necessary*, which is the question I was given; it does not rule on whether the button looks right.

---

## 9. Notes for build-master

- **PASS. Proceed to 5a.** Predicted `.generated.h` delta: **9 → 10 exec symbols**, single addition `execHandleScrollButtonClicked`, **zero removals, zero renames** — measure it **by symbol set**, never by size or sha (UHT embeds source line numbers). `SiegeControlsHelpWidget.cpp` should compile non-unity as it did at `TASK-1478`.
- ⛔ **Do NOT cite the automation suite as evidence for this screen.** Re-measured this pass: `Tests/SiegeControlsHelpTest.cpp` has **0** occurrences of any symbol on it. The suite is a regression check on *everything else*.
- ⛔ **This row has a runtime acceptance criterion and it is 🧑 his sentence.** Do not commit on `built`. Route to 5b.
- **For the 5b brief, three things this gate adds to the handoff's §5 sequence:** (i) **WARN-2 — a screenshot before and after on the same overflowing page, or the verdict is again about code**; (ii) **WARN-3/NIT-3 — read the whole log line (`end`, `step`, `view`), not the terminal word**; (iii) choose `Cards.StackUpgrade`, and treat a `NO MOVEMENT` with `end 0.0` on a short page as a **correct negative**, not a failure.
- **For the manager, two items I am not permitted to write myself:** (a) ⛔ **this subject has no gate row on the board** (§0) — one should be minted pointing at `qa/TASK-1484-gate.md`; (b) ⭐ the **fourth** two-`UButton`-parent near-miss of the milestone argues for a standing `CONVENTIONS.md` note that *the stepper count guard is not the safety property — the name and glyph discriminators are* (§3).
