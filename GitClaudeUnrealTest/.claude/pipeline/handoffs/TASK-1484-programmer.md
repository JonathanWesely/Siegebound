# TASK-1484 [CONTROLS-HELP-DETAIL-SCROLL] — programmer handoff

**Marker:** `TASK-1484-SHAPE-BETA-ACTIVATION-STOP` · row marker `TASK-1484-CONTROLS-HELP-DETAIL-SCROLL` · ruling marker `TASK-1484-RULED-BLOCKING-2026-09-25`
**Status set:** `ready-for-qa` (self-flip granted on dispatch)
**Files written:** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` · `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` · this file · TASKBOARD row `status:` only.
**Not touched:** `SiegeMenuInputSubsystem.{cpp,h}` (read-only, and I did **not** need it) · no input binding · no `.uasset` · no `CONVENTIONS.md` · no compile · no MCP mutation · no PIE · no git. Editor left exactly as found (**PID 43320** up, no PIE started).

---

## 0. The answer in one sentence

**Shape (β), amended to an ACTIVATION-driven second stop:** the detail page gains one focusable `UButton` — `DetailScrollButton` — whose `OnClicked` runs `USiegeControlsDetailWidget::AdvanceBodyScroll()`, which pages `DetailScrollBox` by **0.85 of its own measured view height** and **wraps to the top** at the end; the agent reaches it with `IA_MenuDown` (a move that now really changes focus, because the page finally has two stops) and presses it with `IA_MenuAccept`, **both on the already-proven lane**, and every press prints one `Log` line carrying **the offset before and the offset after**.

---

## 1. Spec (2) — the shape, with the closed routes named

### ⛔ The barred route, restated so this handoff cannot be mistaken for proposing it

**I did not make `DetailScrollBox` focusable.** It stays `SetIsFocusable(false)`. The row measured it **forbidden** (`TASK-1432` (2)'s standing ruling) **and** **ineffective** (`IsNavFocusStop` admits four classes — `UButton` / `UCheckBox` / `USlider` / `UEditableTextBox` — and `UScrollBox` is none of them, so the flag moves nothing). Re-read at source this pass: `IsNavFocusStop`'s own comment names `SiegeControlsHelpWidget.cpp`'s two scroll-box opt-outs and says the walker *"never admits"* that class anyway. **It would break a rule to accomplish zero.**

### ⛔ (α) a fifth admitted class / a scroll verb in the walker — **NOT TAKEN, and I am not escalating it either**

Pre-fenced on the row (subsystem-side, collides with `TASK-1485`, moves the count on every screen days before 5b). I record only this: **it was not necessary.** The fix did not have to live there, so the escalation the row authorised (*"if you conclude this is the only correct fix, say so and escalate"*) has no premise. Nothing in `SiegeMenuInputSubsystem.{cpp,h}` is edited, and nothing in this row's shape depends on `TASK-1485` landing or not landing.

### ⛔ (δ) Row B's self-driving flag — **DEAD, and killed by its own falsifier in the first ten minutes**

The row wrote the falsifier: *does the flag DELIVER the action to the screen, or merely return early and DROP it?* **It drops it.** Measured at source, not inferred:

- `USiegeMenuInputSubsystem::DeclineIfActiveTargetSelfDriving(...)` logs one line and **`return true`**.
- Every one of its four call sites — `MoveFocus`, `HandleMenuAccept`, `StepFocusedStop`, `FocusFirstNavStop` — does `if (Decline…) { return; }`. There is **no callback, no delegate, no interface dispatch, no "forward to the screen" branch** anywhere on that path.
- The subsystem's own class comment says so in terms: the flag's whole content is *"exactly four things the subsystem **stops** doing"*.
- **Why the deck builder nevertheless hears its keys:** `UDeckBuilderWidget::BindMenuNavActions` **binds the six `IA_Menu*` assets itself** on the shared `UEnhancedInputComponent` and routes them through its own `RouteMenuNavKey`. **That is an input binding** — and an input binding is forbidden by this row's fence (5) and by its `names:` line (*"⛔ NEVER an input binding"*).

⇒ **(δ) is not cheap-but-imperfect; it is unavailable.** Taking it would have required exactly the thing the fence names. One paragraph, as the row asked, and moving on.

### ⛔ (γ) screen-side key handling for scroll keys — **REFUSED, and NOT on relapse grounds: it would have shipped code the agent cannot reach**

I was ready to argue (γ) was not a relapse. The argument was available and I think it was sound: `NativeOnPreviewKeyDown` already exists; `PageUp`/`PageDown` appear **zero** times in all of `Source/`; they are not Slate navigation keys and not `IA_Menu*` actions; and gated on `IsDetailViewActive()` the claim would have covered exactly the one view where those keys are provably no-ops today. **That argument is beside the point, because the shape fails a harder test.**

**`qa/TASK-1459-verify.md` measured the agent's key lane and it is not Slate.** Quoted from that report:

> `simulate_key_press` demonstrably delivers into **Enhanced Input** (`Return` → `binding_found: true` → `IA_MenuAccept`), and equally demonstrably does **not** deliver a key event that a focused `SButton` acts on.

with the control that makes it a real null rather than a blind instrument: same button, same screen, same session — `simulate_key_press SpaceBar` (`binding_found:false`) left the main menu up at `mean_luma 193`; `inject_input_action IA_MenuAccept` opened Settings at `mean_luma 50`.

⇒ **A `NativeOnPreviewKeyDown` claim on a scroll key would never run from the agent lane.** It would compile, review clean, satisfy a code path and **fail his sentence** — `SC-§36.1` exactly. The acceptance is *"an agent can read the text below the fold"*, so **the shape must be reachable by the verbs the agent provably holds**, and those are Enhanced Input actions, not raw keys.

### ✅ (β′) — the shape taken, and the one deviation, declared

**What the agent's vocabulary actually does to a screen** (read at source this pass, `SiegeMenuInputSubsystem.cpp`):
1. `IA_MenuUp`/`IA_MenuDown` → `MoveFocus(±1)` → moves the ring across **focus stops**.
2. `IA_MenuAccept` → `HandleMenuAccept()` → on a focused `UButton`, **`Focused->OnClicked.Broadcast()`** — *"the SAME delegate a mouse click fires"*.
3. `IA_MenuLeft`/`IA_MenuRight` → `StepFocusedStop(±1)` → sliders, check boxes, stepper pairs; **a plain `UButton` deliberately does nothing**.

⇒ the only two things that can reach a screen from the agent lane are **focus** and **a focused button's `OnClicked`**. A `UButton` is therefore not an aesthetic choice; it is the only in-fence widget the acceptance can be observed through.

**The deviation, stated plainly: the board sketched (β) as *"a second stop whose FOCUS drives the scroll"*, and I built *"a second stop whose ACTIVATION drives the scroll"*.** The reason is arithmetic, not preference:

- A **focus**-driven stop scrolls through `EScrollWhenFocusChanges::InstantScroll` → `SScrollBox::OnFocusChanging` → `ScrollDescendantIntoView`, which scrolls **to that one widget**. One stop inside the box ⇒ **exactly one jump**, to wherever it sits. On `Cards.StackUpgrade` (eight paragraphs plus three related blocks) that means the reader lands at the bottom and **the middle is never displayed**. It would satisfy "an offset moved" and still fail "read the text below the fold".
- An **activation**-driven stop pages **repeatedly**, so the length of the prose stops mattering. It costs **no extra capability**: reaching it is `IA_MenuDown`, pressing it is `IA_MenuAccept`, both already proven live in `qa/TASK-1459-verify.md`.
- It also keeps the new stop **outside** `DetailScrollBox` (a sibling in `DetailColumn`), which is why **no `SetScrollWhenFocusChanges` companion was added to that box** — the reason the original comment gives is still true, just for a different arity, and the comment is amended in place rather than struck.

**Wrap instead of a second "scroll up" button:** from the end, the next press returns to offset 0. That makes **every offset on a page of any length reachable from one control**, which is what holds the detail stop count at **2** instead of 3, and it is stated in the button's own label so it cannot surprise a reader.

---

## 2. BINDING — the before/after stop set for BOTH views, as a property with its generating rule

**Generating rule (unchanged by this row):** `CollectNavStopsFromTree` walks the registered screen's `WidgetTree` depth-first **pre-order**, descending into code-authored sub-widgets (`IsCodeAuthoredSubWidget` ⇒ `CLASS_Native`), and keeps a widget iff `IsNavFocusStop` — *enabled* ∧ *own visibility visible* ∧ `HasVisibleSlateAncestry` ∧ *one of four admitted classes* ∧ *the class's own focusable opt-in* ∧ *not the `Next` member of a detected stepper pair*.

| view | before (gated, per `TASK-1478`) | after this row | delta |
|---|---|---|---|
| **LIST** | `{ RowButton of every live USiegeControlsHelpRowWidget, in RowScrollBox slot order } ∪ { CloseButton }`, `CloseButton` **last** ⇒ `RowWidgets.Num() + 1` = **28** (27 registry rows + Close) | **identical — 28** | **none** |
| **DETAIL** | `{ BackButton }` ⇒ **1** | `{ BackButton, DetailScrollButton }` ⇒ **2**, in that order | **1 → 2** |

**Why the list cannot move:** the new widgets live inside `USiegeControlsDetailWidget`'s tree, which `ApplyActiveView` **collapses** while the list is up ⇒ `HasVisibleSlateAncestry` drops them. The list branch is untouched: no row widget, no `CloseButton`, no `RowScrollBox` property changed.

**Why `BackButton` is still detail stop 0:** `DetailScrollButton` is added **last** to `DetailColumn` and the walk is pre-order. Every sentence in this file about *where the ring lands on a detail page* (`ShowDetailForAction`'s `FocusFirstNavStop()` note, `ReturnToList`'s, `RegisterAsMenuNavTarget`'s) therefore stands unchanged. **This is the reason for the slot position; it is not layout taste, and moving that `AddChildToVerticalBox` call earlier would silently move the ring on every detail page.**

**Why the detail count is a property and not a tally:** `ConstructDetailTree` builds **exactly these two `UButton`s** and no other admitted-class widget; `RebuildRelatedBlocks` constructs only `UHorizontalBox` / `UBorder` / `UTextBlock` / `UVerticalBox`. ⇒ the set is `{ BackButton, DetailScrollButton }` for **every row, every page, however many related blocks it grows**.

### 🚨 The hazard this row creates and closes in the same breath — the stepper pair

`DetailColumn` now holds **exactly two `UButton`s**, which **passes** `FindStepperPair`'s count guard (*"exactly two buttons in the immediate parent"*). Only its two discriminators refuse the pair:
- **names** — a pair needs `<Base>PrevButton` + `<Base>NextButton` with an **equal** base (`PrevNameSuffix = "PrevButton"`, `NextNameSuffix = "NextButton"`). `BackButton` / `DetailScrollButton` match neither suffix.
- **labels** — a pair needs the bare glyphs `<` and `>`. Both labels are sentences.

**If either were ever broken, two silent failures land together:** `IsNavFocusStop` **drops** the `Next` member (the page loses a stop — possibly `BackButton` itself), and `StepFocusedStop` makes `IA_MenuLeft`/`IA_MenuRight` **press the partner**, i.e. an arrow key that **leaves the page**. `USettingsMenuWidget`'s footer (`GraphicsButton` + `BackButton`) is the shipped precedent for a two-button parent that is correctly refused. The hazard is written at the button's declaration, at its construction site and at `RegisterAsMenuNavTarget`'s count comment — **three places, because it is a rename away.**

### Anchoring — `UNANCHORED` where it must be

- **`DetailScrollButton` is `UNANCHORED` by object path.** I have run no PIE and hold no object path for it. The log discriminator is the pair of lines: `LogNavTargetRetarget` names the **owning screen** and prints `2 focus stop(s)`, and the following `MoveFocus` line prints `of 2`. **`of 2` immediately after a retarget naming the controls-help widget is the discriminator** — no other screen in the project presents a 2-stop ring behind that retarget line.
- **`BackButton` is likewise `UNANCHORED`**, unchanged from `TASK-1478`.
- `CloseButton` still collides by name with `WBP_WarMap`'s, and all 27 rows still print as `RowButton`; **`of 28` remains the list-side discriminator**, exactly as `TASK-1478` wrote it. Nothing in this row changes either.

---

## 3. Spec (3) — the human path, unchanged byte-for-byte

| authored property | state |
|---|---|
| `DetailScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)` | **untouched** — the wheel still scrolls |
| `DetailScrollBox->SetAlwaysShowScrollbar(true)` | **untouched** — the scrollbar still shows |
| `DetailScrollBox->SetIsFocusable(false)` | **untouched** — the box is still not a stop |
| `DetailScrollBox->SetOrientation(Orient_Vertical)`, its slot padding/alignment, `ScrollToStart()` on every stamp | **untouched** |
| `RowScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll)` | **untouched, and still pre-`Super::RebuildWidget()`** — I added nothing to `ConstructHelpTree` at all |
| every existing `DetailColumn` slot (header, summary, scroll box, Back) | **untouched** — the new slot is appended, nothing is re-padded, re-aligned or reordered |

**The only visible delta is one button below "Back to the controls list"**, labelled `Scroll down — back to the top at the end`, using the Back button's own padding shape (`FMargin(56, 0, 56, 34)`, `HAlign_Center`, `VAlign_Bottom`, content padding `28/8/28/8`, font 20 vs Back's 24). **Declared for 🧑 him rather than adjudicated here** (`AS-§6 A(e)` / `HELP-§6` forbid an agent ruling on a shipped screen's visuals). If he wants it elsewhere, smaller, or glyph-only, **note that glyph-only is barred by the stepper-pair discriminator above** — it would need a rename-proof alternative, which is a new row.

**No key is typed anywhere:** the label names a direction and a behaviour, never a keystroke (`HELP-§1` / `HELP-§4`). `Escape` is untouched.

---

## 4. Spec (4) — the regression list, re-checked

| must still hold | status |
|---|---|
| `TASK-1432`'s `Tab`-to-close repair | **untouched** — `NativeOnPreviewKeyDown` is **byte-identical**; I added **no** key handler to any of the three classes. Its cover argument (the preview tunnel runs along the focused widget's ancestor chain, and every focusable button here is a descendant of the overlay) extends to `DetailScrollButton` **by the same sentence and by construction** — it is a child of `DetailColumn` → `DetailBackdrop` → `DetailView` → `ViewSwitcher` → `BackdropBorder`. |
| `IA_ControlsHelp` closes from **both** pages | **held** — no close route touched; `CloseHelp()` → `ReturnToList()` → `ApplyOpenState(false)` is unedited, and `AdvanceBodyScroll` writes one float on a scroll box. |
| `TASK-1478`'s `ApplyActiveView` collapse | **untouched** — 0 edits to that function; **0** new `SetActiveWidgetIndex`, `SetVisibility` or registration calls anywhere in this row. |
| `TASK-1478`'s trip-wires (absence = BLOCKER per `TASK-1479`) | **both present and now three:** the `RowButton` one (`cpp:1957`-region) and the `BackButton` one (`ConstructDetailTree`) are **unedited**; a **third** is planted at `ApplyButtonFocusable(DetailScrollButton)` because that is a **second line that trips the same wire**. It inherits the mechanism rather than restating it. |
| the show-edge registration (`qa/TASK-1430.md` WARN-2's first non-vacuous satisfaction) | **untouched** — `ApplyOpenState` unedited; registration still taken after `SetVisibility`, never in `NativeConstruct`. |
| `registered ⟺ bHelpOpen` | **untouched** — two call sites (`ApplyOpenState(false)`, `NativeDestruct`), unchanged; the two view switches still **refresh**. |
| `RowScrollBox->SetScrollWhenFocusChanges(InstantScroll)` pre-`Super` | **untouched** — see §3. |
| `HELP-§3` escape hatch | **held** — both new children are `BindWidgetOptional` and constructed **only while still null**, so an asset-authored `/Game/UI/WBP_ControlsDetail` returns early at `WidgetTree->RootWidget != nullptr` and still wins whole with zero C++ change. |
| `KBD-§1`/`§2`, no `SetInputMode`, no `bShowMouseCursor`, no `SetKeyboardFocus` | **held** — census over my diff: 0 of each. |
| no pause, read-only on the world | **held** — the only world-visible effect is one `SetScrollOffset` on a widget. |

---

## 5. 🚨 THE 5b INSTRUMENT — stated before the sitting, as the row requires

**The acceptance is 🧑 his sentence, not a code path: *an agent can read the text below the fold*. The limb must be able to fail, and the claim is an OFFSET THAT CHANGED — read before, read after, both quoted.**

### The offset-read mechanism

`AdvanceBodyScroll()` reads `DetailScrollBox->GetScrollOffset()` **before** the write and **again after it**, both off the live widget (never off the value written), and prints both on **one** `Log` line together with `GetScrollOffsetOfEnd()`, the step and the measured view height:

```
[ControlsHelp] Detail scroll on '<ActionId>': offset 0.0 -> 612.0 (end 1840.5, step 612.0, view 720.0) - MOVED.
```

Terminal words: **`MOVED`** · **`WRAPPED to the top`** (a press at the end) · **`NO MOVEMENT - this page's prose already fits, or it is already at both ends`**.

**Why the read-back is immediate and honest** (measured in 5.8 engine source this pass):
- `UScrollBox::GetScrollOffset()` → `SScrollBox::GetScrollOffset()` → `return DesiredScrollOffset;` — i.e. the same field `SetScrollOffset` assigns, so it does **not** wait for a layout pass.
- `SScrollBox::SetScrollOffset` assigns **raw** (`DesiredScrollOffset = NewScrollOffset;`); **only `ScrollBy` clamps**. ⇒ the function clamps to `[0, GetScrollOffsetOfEnd()]` **itself**, or the line would report a move that the next layout pass silently undid. **A log line that lies is the one outcome this instrument may not have.**
- `SScrollBox::GetScrollOffsetOfEnd()` is `max(ContentSize − ScrollPanelSize, 0)` from cached geometry, so on a page whose prose **fits** it is `0`, the clamp pins the target to `0`, both numbers read `0.0` and the function returns **false**. **That is a real negative, not a silence.**

**Both categories log at `Log`, not `Verbose`** (`LogSiegeControlsHelp` and `LogSiegeMenuInput` are both `(…, Log, All)`), so in Development they print with **no console verb typed first** ⇒ **an absent line is a ZERO and a defect signature, not `UNOBSERVABLE`.**

### Expected sequence for a clean sitting (deltas from `TASK-1478`'s list in **bold**)

1. open: `[ControlsHelp] Rebuilt 27 of 27 registry rows…` → `[ControlsHelp] Overlay opened.` → `menu nav target registered -> '…' (registered screen), 28 focus stop(s), 1 screen(s) registered.` — **unchanged.**
2. `IA_MenuDown` ×k: `MoveFocus(+1): focus moved i -> i+1 of 28 ('RowButton')`, strictly advancing — **unchanged.**
3. `IA_MenuAccept` on a row: `[ControlsHelp] Detail page open for '<id>' (n related control(s)).` → `menu nav target registered -> '…', **2** focus stop(s), 1 screen(s) registered.` — **was `1`.**
4. `IA_MenuDown` on the detail page: **`MoveFocus(+1): focus moved 0 -> 1 of 2 ('DetailScrollButton')`** — **was `0 -> 0 of 1 ('BackButton')`, which TASK-1478 correctly called a 1-stop ring wrapping to itself.** A genuine move is now the expectation, and a repeated `0 -> 0 of 2` would be the swallow signature.
5. **`IA_MenuAccept` on it: `IA_MenuAccept -> OnClicked.Broadcast() on 'DetailScrollButton' ("Scroll down — back to the top at the end")` immediately followed by `[ControlsHelp] Detail scroll on '<id>': offset X -> Y …`.** ⇒ **the pass condition is `Y > X`, both quoted from that one line.**
6. repeat 5 until the line reads `WRAPPED to the top` (`offset <end> -> 0.0`) — proves the whole page was walked.
7. `IA_MenuUp` then `IA_MenuAccept`: back on `BackButton`, `[ControlsHelp]` returns to the list, `28 focus stop(s)`.
8. toggle key from either page: `[ControlsHelp] Overlay closed.` + `unregistered`. The one transient `registered`→`unregistered` pair on the close-from-detail route is **still expected and still not churn**.

**Choose a page whose prose overflows.** `Cards.StackUpgrade` is the longest (eight paragraphs + three related blocks); `Hero.Jump` is short and may legitimately report `NO MOVEMENT`. **A `NO MOVEMENT` on a short page is a correct reading of a page with nothing below the fold, not a failure** — and the `end 0.0` term on the same line is what discriminates the two.

---

## 6. Build-facing predictions

**`.generated.h` — predicted by EXEC-SYMBOL SET, never by size or sha** (UHT embeds source line numbers, so both are unsound).

- **Before** (`Intermediate/Build/Win64/UnrealEditor/Inc/GitClaudeUnrealTest/UHT/SiegeControlsHelpWidget.generated.h`, read this pass): `execCloseHelp`, `execGetSelectedActionId`, `execHandleBackButtonClicked`, `execHandleCloseButtonClicked`, `execHandleRowButtonClicked`, `execIsDetailViewActive`, `execIsHelpOpen`, `execOpenHelp`, `execRefreshRows` — **9**.
- **After:** the same nine **plus `execHandleScrollButtonClicked`** — **10**. Exactly one addition, **zero removals, zero renames**.
- `AdvanceBodyScroll()` is **not** a `UFUNCTION` and generates **no** exec symbol. The two new `UPROPERTY`s (`DetailScrollButton`, `DetailScrollLabelText`) add reflected **property** entries to `USiegeControlsDetailWidget`'s class registration and **no** exec symbol.
- No `UCLASS`, `USTRUCT`, `UENUM` or delegate was added, removed or renamed ⇒ no new generated body id beyond the class's own reflow.

**Deletions, declared by content** (`--numstat` integers are unstable, so none is quoted): **no function, member, property, constant, log line or `#include` was deleted.** Three prose statements were **struck in place and replaced** under `SC-§120`, all of them count claims that this row makes false: (i) `ConstructDetailTree`'s *"EXACTLY ONE STOP … StopSet(detail) = `{ BackButton }`"*; (ii) `RegisterAsMenuNavTarget`'s *"DETAIL view up ⇒ `{ DetailView->BackButton }` — exactly one"*; (iii) the two header/`ApplyOpenState` sentences reading *"the detail page now HAS a ring (`{ BackButton }`)"*. In each case the **argument** is kept and only the **count** is corrected, and the struck text is left visible.

**One comment was amended rather than struck** — `DetailScrollBox`'s *"no `SetScrollWhenFocusChanges` companion is added here"* block. Its premise (*"this page's only stop is `BackButton`"*) expired; its conclusion did not, because the new stop is **also** a sibling of the box. The amendment says which half moved.

---

## 7. cl. 3(b) first sentence, for his sitting, verbatim

> 🧑 Open the Tab screen, go into a row's detail page, and use ⛔ only the keyboard — ⛔ can you read text that starts below the bottom of the panel?
> *(and the parenthesis verbatim)*

---

## 8. ## Not examined / limitations

1. ⛔ **Not compiled, not run, no PIE, no MCP call of any kind, no git.** Every claim here is source-level or quoted from a committed report. The editor was left as found (PID 43320 up, no PIE).
2. 🚨 **The automation suite is NOT cited for this screen, and nothing was added to it.** Re-confirmed this pass: `Tests/SiegeControlsHelpTest.cpp` contains **zero** occurrences of `Switcher` / `ActiveWidgetIndex` / `RowButton` / `BackButton` / `CloseButton` / `IsFocusable` / `Collapsed` / `ScrollWhenFocusChanges` / `ApplyActiveView` — and now also zero of `AdvanceBodyScroll` / `DetailScrollButton`. **A green suite says nothing about this row.** `AdvanceBodyScroll()` was made **public** so a future test *could* drive it with no viewport, but I wrote none (out of fence).
3. **The stop counts are argued, not measured.** 28 and 1→2 come from the generating rule plus the tree's structure; I ran no census and no PIE. The walker (`GetMenuFocusStops` / `IsNavFocusStop` / `HasVisibleSlateAncestry`) is **unedited by this row**, which is why no *other* screen's count can move.
4. **`DetailScrollButton` is `UNANCHORED` by object path** (see §2). Its identity in the log rests on `GetName()` = `DetailScrollButton` plus the preceding retarget line naming the screen and `of 2`. No claim here depends on an index.
5. **The step is 0.85 of a measured view height, and the fallback (320 px) has never run.** In every state a player or an agent can reach, the page is on screen and arranged, so the measured branch is the live one. The fallback exists for the first-build frame and a `-nullrhi` lane; if 5b ever sees `NO CACHED GEOMETRY` on that line, **that** is the finding.
6. **The wrap is a judgement I made, not a ruling I hold.** One control that pages down and wraps was chosen over two controls (down + up) to keep the detail count at 2 and the visual delta at one button. If 🧑 he would rather have a pair, that is a small, separable change — **but note it would re-enter `FindStepperPair`'s territory from the other side (three buttons in the parent fails the count guard, so it is safe — the danger is only at exactly two).**
7. **Visual adjudication is out of fence** (`AS-§6 A(e)` / `HELP-§6`): whether a second button below "Back" reads well, and whether the focus outline is legible on it, are questions for him or the artist. I matched the Back button's padding shape and said so in §3.
8. **No IMC asset was opened** (they are binary). The `PageUp`/`PageDown` census that fed the (γ) analysis was over `Source/` only — it is moot now, since no key is claimed.
9. **`qa/TASK-1459-verify.md`'s measurement is load-bearing for the shape choice and I did not re-run it.** If the agent's key lane ever gains a true Slate-key verb, (γ) becomes available again — but it would then be an *addition*, not a replacement: nothing in this row would need to be undone.
10. **I did not re-diagnose the mechanism** (row clause (1)). The one-stop / `SetUserFocus` early-return / `OnFocusChanging`-never-fires chain is `qa/TASK-1479.md`'s measurement, cited and used, not re-derived.
