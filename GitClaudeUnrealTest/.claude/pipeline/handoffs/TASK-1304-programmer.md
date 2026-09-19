# TASK-1304 — DECKGRID-FOCUS-ENTRY · gameplay-programmer handoff

subject: TASK-1304 · gate: TASK-1305 · host: TASK-1306
author: gameplay-programmer · date: 2026-09-18
base: `6bd1aa5` (tree clean at dispatch ⇒ disk == commit; every HEAD line number below was
re-measured against those bytes **before** the first edit, never copied from the board)

> ⛔ **ONE DIFF, ONE COMPILE, THREE AUTHORITIES.** Every changed line belongs to exactly one
> block and every block names exactly one authority. A reader asking *"who authorised this
> line?"* gets one answer.
>
> | block | what | authority |
> |---|---|---|
> | **A** | the entry path — `NativeOnPreviewKeyDown` + the builder actually on the focus path (incl. one `.uasset` property) | the **manager's spec defect** on `TASK-1286` + 🧑 his `TASK-1303` ruling *"Fix it next — TASK-1304 chain first"* |
> | **B** | `Escape` restored to the exit gesture | 🧑 **Jonathan, 2026-09-18, `TASK-1300`** — *"Scoped — Escape may exit the card grid"* |
> | **C** | the two `C4996` migrations | **the compiler** (`SC-§122`); spec = `TASK-1301` whole |

## Files touched

| path | blocks | shape |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` | A · B · C | declarations + prose |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | A · B · C | the only executable changes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp` | B | **prose only**, +4 lines, 0 tests added, 0 removed, `EKeys::` count still **0** |
| `Content/UI/WBP_DeckBuilder.uasset` | A | **exactly one property**, edited **in place** |

⛔ **Not touched, by name:** `SiegeMenuInputSubsystem.{h,cpp}` · `WBP_DeckCardTile` · any `KBD-§`
layout table · `Config/**` · git (no command run). ⚠️ `WBP_DeckSlotEntry` **does not exist as an
asset** — an asset-registry sweep of `/Game/UI` returns exactly two deck assets,
`WBP_DeckCardTile` and `WBP_DeckBuilder`; the bar entries are code-authored
(`CreateWidget<UDeckSlotEntryWidget>`). Reported rather than claimed-left-alone.

---

# 🚨 READ THIS FIRST — THE ONE PREMISE I COULD NOT MEASURE, AND IT IS A BLOCKER, NOT A NOTE

`SC-§123` cl. 5: *a declared-unmeasured premise the feature depends on is a **BLOCKER**, not a
caveat — disclosure is not discharge.* So I am not filing this as a caveat.

**The premise:** *with the builder open, after ONE `Down`, `FocusedCardIndex != -1` and the focus
path contains the builder.* (Row clause (5), second half.)

**Why this row's instruments cannot reach it, stated mechanically:** the code that establishes it
does not exist in the running binaries, and this row is fenced from compiling (`⛔ do NOT compile`).
A PIE session against the editor that is up right now exercises `6bd1aa5`, in which
`AcquireBuilderFocus` does not exist — so a PIE run here could only re-confirm the negative control
(`FocusedCardIndex == -1`) that `qa/TASK-1286-verify.md` already holds. **It cannot return a
non-negative answer no matter how it is run.** The row asks this row for a measurement that only a
compiled binary can produce.

**Why the suite cannot reach it either, and this one is new information:** I wrote a route test that
drives the *real* Slate overrides, then **withdrew it** — `UUserWidget` declares both
`NativeOnPreviewKeyDown` and `NativeOnKeyDown` **`protected`** (`UserWidget.h:1572 protected:`,
declarations at `:1607-1608`). An automation test cannot call them without widening engine access
purely for a test, which is the test-only seam this class refuses everywhere else (it is why
Accept/Remove call `AddCopy`/`RemoveCopy` and why `ExitCardGridFocus` has no "test variant").
⇒ **The suite delta is 0 and that is structural, not a choice about effort.**

**What settles it, named exactly, so nobody re-derives it:** at `TASK-1306`'s **5b** leg, on the
newly compiled binaries, on `L_MainMenu`, with the deck builder open:

1. Read `WBP_DeckBuilder_C` instance `bIsFocusable` ⇒ must be **True**.
2. Read the **Log** line this row adds, printed once per builder open at default verbosity
   (⭐ `Log`, **not** `Verbose` — deliberately, see §A3):
   `UDeckBuilderWidget::AcquireBuilderFocus: focus TAKEN|DEFERRED … focused widget is '<type>' (this builder's Slate widget is '<type>') …`
   ⇒ the two type names must be the **same widget**. If the focused widget is an ancestor, block A
   part (a) did not take and the row is inert again.
3. Inject **one** `Down`. Read `FocusedCardIndex` ⇒ must be **≠ -1** (the negative control is `-1`
   through every press, 0/34 tiles focused — `qa/TASK-1286-verify.md`).
4. 🧑 **The human acceptance, verbatim, and it is the only question worth asking:**
   ***"With a card outlined, does pressing `Enter` raise the deck total by one — or does it only
   open the card's details panel?"*** ⛔ Only the first is this feature.

⚠️ **(3) and (4) are the whole row.** An outline appearing is **not** evidence: Slate's own
navigation walks focus into a tile's `Btn_CardFace` with a visible focus rectangle and `Enter`
there fires that button's existing `OnClicked` (the details panel) through `SButton`'s Accept path.
**The builder will feel navigable while every line of this feature is dead.** That is how `TASK-1286`
passed two gates, a clean compile and 561/561.

⚠️⚠️ **FOR THE VERIFIER — `Escape` IS THE EDITOR'S DEFAULT STOP-PIE KEY.** An injected `Escape`
may END the PIE session rather than exercise block B. ⇒ **spend zero attempts injecting it.** The
`Escape` leg is a **hand** check (🧑 his own keyboard), or it is left `UNOBSERVABLE` and said so.

---

# BLOCK A — THE ENTRY PATH
*Authority: the manager's spec defect on `TASK-1286` + 🧑 his `TASK-1303` ruling.*

Two halves. ⛔ **Either alone is still inert**, and the handoff says why for each.

## A1. The preview door — `UDeckBuilderWidget::NativeOnPreviewKeyDown`

`DeckBuilderWidget.h:668` (declaration) · `DeckBuilderWidget.cpp:1715` (definition).

The class previously overrode **only** `NativeOnKeyDown` (`.h:639` at HEAD — re-measured, the
board's number is correct). The bubble pass starts at the **focused leaf** and decrements, and
`SWidget::OnKeyDown` converts an arrow to navigation and returns `Handled` right there — so no
ancestor ever saw a `Down`. The **tunnel** pass has no such conversion. Read at source:

```
SWidget.cpp:411-414   FReply SWidget::OnPreviewKeyDown(...) { return FReply::Unhandled(); }
SWidget.cpp:415-429   FReply SWidget::OnKeyDown(...) { if (bCanSupportFocus && SupportsKeyboardFocus())
                        { ... return FReply::Handled().SetNavigation(Direction, Genesis); } ... }
```

⇒ **the preview is the only pass an ancestor can win on.** That is the entire reason block A exists
and it is one line of engine source, not an argument.

## A2. Both doors are ONE decision — `HandleCardGridKey`

`TASK-1286`'s handler body was **moved**, not rewritten, into a private
`FReply UDeckBuilderWidget::HandleCardGridKey(const FKeyEvent&)` (`.h:844`, `.cpp:1562`). The two
`return Super::…` tails moved out to the two callers, because each `Super` differs; the guards,
the precedence and the key table are otherwise byte-identical. **No second key table exists.**

**What the two doors claim, as named state:**

| state | claimed | falls through |
|---|---|---|
| `IsCardGridFocusLive()` **true** | the 4 directions (move) · Accept · Remove · the exit gesture | everything else |
| **not** live | `Down` / `Gamepad_DPad_Down` / `Gamepad_LeftStick_Down`, **and only with `GetCollectionCardIDs().Num() > 0`** | **everything else, incl. `Left` / `Right` / `Up` / `Enter` / gamepad **B** / `Escape`** |

🚨 **THE FENCE THE ROW NAMES, answered in its own three terms:**
* **on the deck bar** — grid not live ⇒ `Left`/`Right`/`Enter` return `Unhandled` on **both**
  passes ⇒ `DECK-§3`'s horizontal bar keeps every key it has, and 🧑 the navigation he confirmed
  by hand on `TASK-1274` is untouched. `Down` is the one key taken, and the bar is horizontal —
  `Down` does nothing there today, which is the already-gated `TASK-1290`/`TASK-1299` design.
* **in the card grid** — the preview claims the table and Slate's navigation never runs, which is
  the point: a focused tile can no longer eat the arrow.
* **in neither** — the builder is not on the focus path at all, so neither door is invoked. And
  when some *other* top-level widget holds focus (the main menu), **this widget is not on that
  path** — the menu and the builder are siblings under the viewport, not ancestor/descendant —
  so `NativeOnPreviewKeyDown` is not called at all. The main menu cannot be affected by this row
  even in principle.

## A3. Putting the builder on the focus path — `AcquireBuilderFocus()`

`.h:877` (declaration) · `.cpp:1472` (definition) · **call site: `NativeConstruct`,
`DeckBuilderWidget.cpp:670`, last, after the deck bar is built.**

⛔ `USiegeMenuInputSubsystem::ApplyInitialFocus` is **not** used and **not** edited — its
`IsMenuUncovered()` gate refuses this widget by design and names the deck builder in its own
comment at `SiegeMenuInputSubsystem.cpp:191-193`. That subsystem serves the shipped,
hand-confirmed main menu. No stop-and-flag was needed: the fix did not require touching it.

**The asset half is load-bearing and here is the mechanism, read at source:**

```
SlateApplication.cpp:3019-3036   for (WidgetIndex = Num-1; WidgetIndex >= 0; --WidgetIndex)
                                   if (WidgetToFocus.Widget->SupportsKeyboardFocus()) { NewFocusedWidget = …; break; }
SObjectWidget.cpp:175-182         bool SObjectWidget::SupportsKeyboardFocus() const { return WidgetObject->NativeSupportsKeyboardFocus(); }
UserWidget.cpp:2411-2414          bool UUserWidget::NativeSupportsKeyboardFocus() const { return bIsFocusable; }
```

⇒ the leaf→root walk stops on this widget **iff** `bIsFocusable`. With `False` it skips the builder
and lands on an **ancestor** — exactly the state `TASK-1286` was measured in.

⭐ **And a premise I had to settle rather than assume, because getting it wrong would have made the
focus request target the wrong widget:** for a `UUserWidget`, `GetCachedWidget()` returns
**`MyGCWidget` — the `SObjectWidget`** — whenever it is valid (`Widget.cpp:1102-1107`), *not* the
inner root. `MyGCWidget` is assigned at `Widget.cpp:1022`, **before** `TakeWidget_Private` calls
`OnWidgetRebuilt()` at `:1096`, which is what calls `NativeConstruct()` (`UserWidget.cpp:1234`).
⇒ it is valid inside `NativeConstruct`, and it is the widget that reads `bIsFocusable`.

**`EFocusCause::SetDirectly`, deliberately the opposite of `FocusCardTile`'s `Navigation`:**
`SetUserFocus` sets `ShowFocus = (InCause == Navigation)` (`SlateApplication.cpp:3099`) and
`SWidget::Paint` only draws the focus brush when `ShowUserFocus` is true (`SWidget.cpp:1746-1751`)
⇒ `Navigation` here would paint a focus rectangle around the **whole screen**. The outline belongs
on a card tile.

**The deferred lane, and why a `false` return is the normal case, not a failure:** `Construct` runs
from `TakeWidget_Private` **before** `AddToViewport` has parented the widget into the game window,
so `SetUserFocus` refuses. The engine's own answer is
`ULocalPlayer::GetSlateOperations()`, flushed every frame by
`FEngineLoop::ProcessLocalPlayerSlateOperations` (`LaunchEngineLoop.cpp:5231`, ticked at `:5918`)
through `FSlateApplication::ProcessExternalReply` (`:3351`). This is the same fallback
`UWidget::SetUserFocus` uses (`Widget.cpp:742-750`).

**Read-back, not assumption** (the row's explicit instruction): the function reads
`FSlateApplication::Get().GetUserFocusedWidget(UserIndex)` back and logs **which widget actually
holds focus** next to this builder's own Slate widget, at **`Log`** verbosity — one line per
builder open. ⛔ `Verbose` was rejected on purpose: a `Verbose` line needs
`Log LogGitClaudeUnrealTest Verbose` typed first, and an empty log reads as a passing measurement.

## A4. The `.uasset` — MEASURED, in place, one property

* **Pre-state**, read on the **read-only** lane against the live editor (PID 8288) before any write:
  `Default__WBP_DeckBuilder_C` → **`is_focusable = False`** (matches the board's PID-22564 reading).
* **Edit:** `set_editor_property("is_focusable", True)` on the CDO → `compile_blueprint` → `save_asset`.
* **Post-state**, read back on the **independent read-only** lane: **`is_focusable = True`**.
* **In place, proven, not asserted:** asset path still `/Game/UI/WBP_DeckBuilder.WBP_DeckBuilder`,
  generated class still **`WBP_DeckBuilder_C`**, CDO still `isinstance(..., DeckBuilderWidget)` ⇒
  **no duplicate, no reparent.** (The duplicate+reparent failure mode silently breaks *runtime*
  repaint while design time looks perfect; it was avoided by construction, not by inspection.)
* **Not inherited:** the native `UDeckBuilderWidget` CDO still reads `is_focusable = False`, so the
  `True` is this asset's own override.
* **Fenced siblings:** `WBP_DeckCardTile` → `is_focusable = False`, untouched (correct — the tiles
  are made focusable at runtime by `FocusCardTile`). `WBP_DeckSlotEntry` does not exist.
* **Editor left clean:** `DIRTY_PACKAGES: []`, `DIRTY_COUNT: 0`. Nothing else saved.
* **Disk:** `Content/UI/WBP_DeckBuilder.uasset` mtime `2026-09-18 15:41:05 -0700` = my save.

### ⚠️ ONE ANOMALY, REPORTED RATHER THAN SMOOTHED OVER
The **write** lane's own `print("PRE …")` echoed `True`, although the independent read-only lane had
read `False` minutes earlier and the native parent default is `False`. Best explanation: the write
tool executed the script more than once (it returns a `TransactionId`) and returned the **last**
run's output — the second run naturally observed the first run's result. **The pre-state claim in
this handoff rests on the read-only lane's independent measurement, not on the write lane's echo.**
The script is idempotent (one property, then compile+save), so a double run changes nothing.
⚠️ **For QA/the host:** `compile_blueprint` was called before the save, which is the normal editor
action after a CDO change but may re-serialise more bytes than the single property. The `.uasset`
diff is binary; **I can assert the property and the identity, I cannot assert a minimal byte delta.**

## A5. THE CIRCULARITY — how it breaks (`SC-§123` cl. 4)

At HEAD: `MoveCardFocus` and `SetFocusedCardIndex` had exactly **one** non-test caller each
(`:1536`, `:1547`), **both inside the `Down` branch of a handler nothing could reach** ⇒
`FocusedCardIndex` was `INDEX_NONE` forever ⇒ `IsCardGridFocusLive()` permanently false ⇒ Accept,
Remove and the exit were all dead behind it.

**What arms the grid now, as a chain with no gap:**

| step | mechanism | read at |
|---|---|---|
| 1 | `NativeConstruct` calls `AcquireBuilderFocus()` | `DeckBuilderWidget.cpp:670` — **a caller in our code** |
| 2 | Slate focus lands on **this widget** (not an ancestor) because `bIsFocusable` is now `true` | `SlateApplication.cpp:3019-3036` → `SObjectWidget.cpp:175-182` → `UserWidget.cpp:2411-2414`; the flag is set at `.uasset` (§A4) **and** re-asserted at `.cpp:1486` |
| 3 | a key press routes along `SlateUser->GetFocusPath()`, which now **contains this widget** | `FSlateApplication::ProcessKeyDownEvent`, `SlateApplication.cpp:5015-5017` |
| 4 | the **tunnel** runs first, root→leaf, calling `OnPreviewKeyDown` on every enabled widget on the path | `SlateApplication.cpp:5021-5044`; `FTunnelPolicy` starts at index 0 and increments, `:347-361` |
| 5 | the bubble runs **only** `if (!Reply.IsEventHandled())` | `SlateApplication.cpp:5046` |
| 6 | on this widget the tunnel calls `SObjectWidget::OnPreviewKeyDown` → `UUserWidget::NativeOnPreviewKeyDown` → **our override** | `SObjectWidget.cpp:221-229` → `UserWidget.cpp:2500-2503` |
| 7 | cold-start `Down` ⇒ `SetFocusedCardIndex(0)` ⇒ `FocusedCardIndex != INDEX_NONE` ⇒ `IsCardGridFocusLive()` can be true ⇒ Accept / Remove / exit are live | `DeckBuilderWidget.cpp:1697-1699` |

**Second, independent route** (belt and braces, and it is why `NativeOnKeyDown` is kept): when the
**builder itself** holds the focus it is the path's **leaf**, and `SObjectWidget::OnKeyDown` calls
`NativeOnKeyDown` **before** falling through to `SCompoundWidget::OnKeyDown`
(`SObjectWidget.cpp:231-239`) — i.e. before `SWidget::OnKeyDown` can turn the arrow into
navigation. So the cold-start `Down` survives even if the tunnel is somehow not reached.

## A6. (5) REACHABILITY EVIDENCE — every engine-invoked handler in this diff

`SC-§123` cl. 3 shape: **caller at file:line, OR the engine routine read at source, PLUS the
precondition and the line that establishes it.**

| handler | dispatcher, read at source | precondition | established at |
|---|---|---|---|
| `UDeckBuilderWidget::NativeOnPreviewKeyDown` | `FSlateApplication::ProcessKeyDownEvent` tunnel, `SlateApplication.cpp:5021-5044` → `SObjectWidget::OnPreviewKeyDown` `:221-229` → `UUserWidget::NativeOnPreviewKeyDown` `UserWidget.cpp:2500-2503` | the widget is on `SlateUser->GetFocusPath()` **and** `IsEnabled()` | `AcquireBuilderFocus()` at `DeckBuilderWidget.cpp:670` + `bIsFocusable = true` (§A4) · ⛔ **the runtime half of this is the unmeasured premise at the top of this file** |
| `UDeckBuilderWidget::NativeOnKeyDown` | `ProcessKeyDownEvent` bubble, `SlateApplication.cpp:5046-5068` → `SObjectWidget::OnKeyDown` `:231-239` | same focus path, **and** the tunnel returned `Unhandled` (`:5046`) | same as above |
| `UDeckBuilderWidget::NativeConstruct` | `UWidget::TakeWidget_Private` → `OnWidgetRebuilt()` `Widget.cpp:1096` → `UUserWidget::NativeConstruct` `UserWidget.cpp:1234` | the widget is constructed / added to screen | unchanged from `TASK-1286`; already shipped and observed (its deck-bar build is on screen) |
| `UDeckBuilderWidget::HandleCardGridKey` | ⛔ **not engine-invoked.** Two callers in our code | — | `DeckBuilderWidget.cpp:1728` (preview) and `:1745` (bubble) |
| `UDeckBuilderWidget::AcquireBuilderFocus` | ⛔ **not engine-invoked.** One caller in our code | — | `DeckBuilderWidget.cpp:670` |

⇒ **no handler in this diff is left with *"the engine calls this"* as its evidence.** The one
residual gap is the *runtime confirmation* of the focus-path precondition, escalated at the top.

---

# BLOCK B — `Escape` RESTORED
*Authority: 🧑 **Jonathan, 2026-09-18, `TASK-1300`** — verbatim label **"Scoped — Escape may exit the
card grid"**; A-2 was about the assistant console and the in-match cancel routes, not a menu-side
grid. `CONVENTIONS.md` `AS-§6` carries it as the **A-2 SCOPE** bullet (`:797`); the old
"read at its widest" sentence (`:796`) is **struck**.*

## B1. The line, as finally written, with its line number re-measured

**HEAD:** `DeckBuilderWidget.cpp:1521` — re-measured by grepping the condition, **not** counted.
⛔ The board's `:1514` and `:1507` are both stale; the A-2 comment block is `:1504-1520`. Corroborated
independently by the compiler itself: the build log names `DeckBuilderWidget.cpp(1521,63)`.

**Now, one line, `DeckBuilderWidget.cpp:1673`, quoted once (blocks B and C are the same physical line):**

```cpp
if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Virtual_Gamepad_Back.GetVirtualKey())
```

**Three tokens, two authorities:** `EKeys::Escape` is 🧑 **his** (block B) ·
`Virtual_Gamepad_Back.GetVirtualKey()` is **the compiler's** (block C) ·
`Gamepad_FaceButton_Right` is `TASK-1286`'s, unmoved.

## B2. 🚨 THE FENCE SENTENCE

**`Escape` reaches `FReply::Handled()` IF AND ONLY IF `IsCardGridFocusLive()` (the enclosing
`bGridFocused` block) AND `ExitCardGridFocus()` returned `true` — the identical predicate the
gamepad half already uses. `Escape` on the deck bar, on a cold builder, or anywhere else on
`L_MainMenu` falls straight through, on the preview pass as well as the bubble.**

⛔ **And the non-obvious half the row flags as a BLOCKER, answered explicitly: `Escape` IS in block
A's preview key set** — because the preview and the bubble share one decision function and there is
no separate preview table. **It is fenced there by that same `bGridFocused` block**, which is
structurally the only way it can be reached at all: the exit `if` is nested inside it. There is no
code path on which the preview can claim `Escape` with the grid cold. A `Handled` on a Back or
`Escape` press that changed no state remains the defect being removed, untouched by his ruling.

## B3. The `:1517` in-code prescription — re-derived, and it held

The struck comment claimed *"the re-add is `Key == EKeys::Escape ||` on the line below and nothing
else — the tests drive `ExitCardGridFocus()` directly and need no change."*

* **First half: TRUE but incomplete.** The token is exactly that, but it is *not* "nothing else":
  block A's preview override makes the same line reachable on an **earlier and wider** pass, which
  the 2026-09-17 author could not have known. The prescription was written for a bubble-only class.
* **Second half: TRUE, re-derived against the current test file.** `Tests/SiegeDeckSlotsTest.cpp`
  contains **zero** `EKeys::` tokens — verified, not assumed — and drives `ExitCardGridFocus()`
  directly at `:1476`, `:1488` and `:1503`. No test needed a change. **Suite delta from block B: 0.**

## B4. The `Escape` prose census — RE-DERIVED, reported against the manager's list

Measured at HEAD by `grep -rn "Escape"` over the three code files: **24 lines**.

| file | HEAD lines found | on the manager's list? |
|---|---|---|
| `DeckBuilderWidget.h` | 227, **237**, 262, 266, 267, 269, 270, 275, 277, 282, **346** | all except `:227` |
| `DeckBuilderWidget.cpp` | 1507, 1508, 1509, 1511, 1512, 1516, 1517, 1519 (all inside `:1504-1520`), 1557 (inside `:1552-1557`) | yes |
| `Tests/SiegeDeckSlotsTest.cpp` | 1394, 1395, 1396, 1496 | yes |

⭐ **ONE SITE THE MANAGER'S LIST DOES NOT NAME: `DeckBuilderWidget.h:227`** —
*"…deliberately excludes digits/punctuation/Enter/Escape BY DESIGN, CONVENTIONS:2519-2522"*.
That sentence is about **`KBD-§4`'s remap table** excluding `Escape`, which is **still true and
must not change** (`KBD-§` tables diff 0). ⛔ **Left deliberately unedited** — it is a different
claim about a different instrument, and inverting it would have been wrong.

**Every other site was rewritten so that no struck sentence survives.** Specifically gone from the
diff: *"Until he answers, A-2 is read at its widest"*, *"If he later scopes A-2 out, the re-add is…"*,
*"THE EXIT IS GAMEPAD-ONLY"*, *"the keyboard has NO exit key out of the grid"*, and
*"`Escape` in EVERY state, always"* — all now false, all replaced.

⛔ **THE `AS-§6 A-2` CITATION STAYS AT EVERY SITE**, rewritten from a *prohibition* record into a
**GRANT** record: *A-2 scoped by 🧑 Jonathan 2026-09-18; `Escape` is permitted **here**,
conditionally, and **nowhere else**.* A-2 is recorded as **unrelaxed** for the assistant console and
for `ASiegePlayerController`'s in-match cancel routes at both code sites. Deleting the pointer would
have deleted the only thing that caught this in the first place.

Post-edit counts: `.h` 16 lines · `.cpp` 15 · tests 7 (prose grew; **code** occurrences of
`EKeys::Escape`: production **1**, tests **0**).

## B5. ⚠️ A-2 named `NativeOnPreviewKeyDown` explicitly — and that is now quoted, not hidden

The original A-2 text (`CONVENTIONS ~:789`) names the exact route block A introduces:

> *"Any future task that absorbs `Escape` — `NativeOnKeyDown`, **`NativeOnPreviewKeyDown`**, an
> Enhanced Input action, a Slate `FReply::Handled()` on `EKeys::Escape`, or a viewport-level
> intercept — is overturning a Jonathan ruling and is an automatic QA FAIL."*
> *"…a preview-key handler that returns `Handled` for `Escape`, even 'harmlessly', breaks this clause."*

⇒ This row does the one thing that clause forbids, **on his own written scoping of it**, and the
code comment at `.cpp:1630-1672` quotes the prohibition, his ruling, the date and the row before
restoring the token — so the register shows the exception was **GRANTED**, not **MISSED**
(`SC-§121` cl. 6).

---

# BLOCK C — TASK-1301
*Authority: **the compiler** (`SC-§122`). Spec = `TASK-1301` whole.*

## C1. (0)(i) The two warning lines, quoted, with the log path

Found on disk — **`C:\Users\wesel\AppData\Local\UnrealBuildTool\Log.txt`**, mtime
`2026-09-17 21:21:14 -0700` (the `TASK-1291` 5a run):

```
DeckBuilderWidget.cpp(1462,44): warning C4996: 'EKeys::Virtual_Accept': Use Virtual_Gamepad_Accept.GetVirtualKey() instead - Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.
DeckBuilderWidget.cpp(1521,63): warning C4996: 'EKeys::Virtual_Back': Use Virtual_Gamepad_Back.GetVirtualKey() instead - Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.
```

**(0)(v) BASELINE, measured from that log, for the host's delta:** `Result: Succeeded` ·
**total warning lines = 2** · distinct codes = `C4996 ×2` · **zero other warnings of any code**.
⇒ **the host's target after this row is 0 warnings.** ⚠️ That baseline is from a *different diff*;
the host measures its own total and reports the delta (`SC-§122` cl. 6).

⭐ The columns are an independent corroboration of my re-measured HEAD sites: `(1462,44)` and
`(1521,63)` — the compiler and my grep agree.

## C2. (0)(ii) The deprecation MARKER, at file:line, with a FIRING POSITIVE CONTROL

**Instrument:** print the line immediately above each `static INPUTCORE_API const FKey <Sym>;`
declaration in `InputCoreTypes.h` and count `UE_DEPRECATED` hits.

| symbol | declaration | marker hits | the line above |
|---|---|---|---|
| `Virtual_Accept` | `InputCoreTypes.h:745` | **1** | `UE_DEPRECATED(5.7, "Use Virtual_Gamepad_Accept.GetVirtualKey() instead")` — `:744` |
| `Virtual_Back` | `InputCoreTypes.h:747` | **1** | `UE_DEPRECATED(5.7, "Use Virtual_Gamepad_Back.GetVirtualKey() instead")` — `:746` |
| `Virtual_Gamepad_Accept` | `:749` | **0** | — |
| `Virtual_Gamepad_Back` | `:750` | **0** | — |
| `Gamepad_FaceButton_Bottom` | `:516` | **0** | — |
| `Escape` | `:368` | **0** | — |

⇒ ⭐ **the control FIRES**: the same method returns **1** on both known-deprecated symbols and **0**
on four known-good ones, including the two replacements. It could have returned another number, and
on the good symbols it did.

**The replacement's declaration and what `GetVirtualKey()` returns:**
* `static INPUTCORE_API const FKey Virtual_Gamepad_Accept;` / `Virtual_Gamepad_Back;` — type **`FKey`**.
* `FKey FKey::GetVirtualKey() const` — `InputCoreTypes.cpp:1445-1449` — returns **`FKey` by value**:
  `ConditionalLookupKeyDetails(); return (KeyDetails.IsValid() ? KeyDetails->GetVirtualKey() : FKey());`

## C3. 🚨 (0)(iii) THE EQUIVALENCE — SAME `FKey`, PROVED FROM ENGINE SOURCE. NO HARD STOP TAKEN.

```
InputCoreTypes.cpp:423   const FKey EKeys::Virtual_Accept = FPlatformInput::GetGamepadAcceptKey();
InputCoreTypes.cpp:424   const FKey EKeys::Virtual_Back   = FPlatformInput::GetGamepadBackKey();

InputCoreTypes.cpp:728   AddVirtualKey(FKeyDetails(EKeys::Virtual_Gamepad_Accept, …,
                             FKeyDetails::GamepadKey | FKeyDetails::Virtual, …),
                             FPlatformInput::GetGamepadAcceptKey());
InputCoreTypes.cpp:729   AddVirtualKey(FKeyDetails(EKeys::Virtual_Gamepad_Back,   …,
                             FKeyDetails::GamepadKey | FKeyDetails::Virtual, …),
                             FPlatformInput::GetGamepadBackKey());

InputCoreTypes.cpp:1017  void EKeys::AddVirtualKey(const FKeyDetails& VirtualKeyDetails, const FKey& VirtualKeyValue)
InputCoreTypes.cpp:1031      InputKeys[VirtualKeyDetails.GetKey()]->VirtualKeyValue = VirtualKeyValue;

InputCoreTypes.h:208     inline const FKey& FKeyDetails::GetVirtualKey() const { return bIsVirtual ? VirtualKeyValue : Key; }
InputCoreTypes.cpp:464       bIsVirtual = ((InKeyFlags & EKeyFlags::Virtual) != 0);      // set by the Virtual flag at :728/:729
InputCoreTypes.cpp:1445  FKey FKey::GetVirtualKey() const { ConditionalLookupKeyDetails(); return (KeyDetails.IsValid() ? KeyDetails->GetVirtualKey() : FKey()); }
```

⇒ `EKeys::Virtual_Gamepad_Accept.GetVirtualKey()` → its `FKeyDetails` → `bIsVirtual` is **true**
(the `FKeyDetails::Virtual` flag at `:728`) → `VirtualKeyValue`, stored at `:1031` as
`FPlatformInput::GetGamepadAcceptKey()` — **exactly what `EKeys::Virtual_Accept` is** at `:423`.
Identically for Back at `:729`/`:424`. **SAME `FKey`. The migration is a call-shape change.
⛔ Nothing is rebound; 🧑 his **X**/**B** ruling and the key table are untouched.
⛔ The `TASK-1301` HARD STOP was NOT triggered and was NOT weakened.**

⚠️ One honest behavioural note: the old symbol was resolved at **static-init**; the new expression
resolves through a **runtime key-details lookup**, cached in a mutable member by
`ConditionalLookupKeyDetails` on first use. Same value, one map lookup once, game thread only.

## C4. (1) The edit — exactly two executable sites

| was (HEAD) | is now |
|---|---|
| `:1462` `Key == EKeys::Virtual_Accept` | `:1585` `Key == EKeys::Virtual_Gamepad_Accept.GetVirtualKey()` |
| `:1521` `Key == EKeys::Virtual_Back` | `:1673` `Key == EKeys::Virtual_Gamepad_Back.GetVirtualKey()` |

Surrounding predicate, operator and **operand order unchanged**. No reformatting, no refactor.
(The line numbers moved only because blocks A and B added lines above them.)

## C5. (2) The comments — prose made to match the code

Four sites named `Virtual_Accept` / `Virtual_Back` and now say the call shape changed and the
binding did not, naming `TASK-1304` block C: `DeckBuilderWidget.h:233`/`:236` (the key table) · `.h:244-248`
(the 2026-09-17 amendment's `Virtual_Back` citation, kept accurate as *history* and annotated) ·
`.cpp:1571-1584` (the equivalence chain at the Accept site) · `.cpp:1593-1597` and `:1617-1618`.
⛔ The `AS-§6 A-2` pointer prose was **not** touched by block C — block B owns it.

## C6. (0)(iv) Repo-wide `EKeys::Virtual_` census

`grep -rn "EKeys::Virtual_" Source/` → **10 hits, all in this feature's two files**:
`.cpp:1572, 1575, 1576, **1585**, 1593, 1597, 1666, **1673**` and `.h:244, 248`.
**Executable: exactly the two migrated sites (`:1585`, `:1673`). Comments: the other eight.**
⇒ **ZERO sites outside `DeckBuilderWidget.{h,cpp}`.** Nothing out of scope to name, nothing else touched.

## C7. (4) No runtime acceptance criterion for block C

The migrated pair is **gamepad-only** and the verifier's lane has no raw face-button injection
(`TASK-1291` cl. (5) already records **B**/**X** as `UNOBSERVABLE` by construction). ⇒ block C's
instruments are **the compiler's own warning count** and the equivalence quote at §C3. No PIE
criterion is invented for it.

---

# THE CENSUS — AS NAMED STATE, NOT A TALLY (`SC-§104`)

**20 bound key expressions, on 7 executable lines in `DeckBuilderWidget.cpp`:**

| line | expressions |
|---|---|
| `:1225` | `EKeys::Left` · `EKeys::Gamepad_DPad_Left` · `EKeys::Gamepad_LeftStick_Left` |
| `:1229` | `EKeys::Right` · `EKeys::Gamepad_DPad_Right` · `EKeys::Gamepad_LeftStick_Right` |
| `:1233` | `EKeys::Up` · `EKeys::Gamepad_DPad_Up` · `EKeys::Gamepad_LeftStick_Up` |
| `:1237` | `EKeys::Down` · `EKeys::Gamepad_DPad_Down` · `EKeys::Gamepad_LeftStick_Down` |
| `:1585` | `EKeys::Enter` · `EKeys::Virtual_Gamepad_Accept.GetVirtualKey()` · `EKeys::Gamepad_FaceButton_Bottom` |
| `:1609` | `EKeys::Delete` · `EKeys::Gamepad_FaceButton_Left` |
| `:1673` | **`EKeys::Escape`** · `EKeys::Gamepad_FaceButton_Right` · `EKeys::Virtual_Gamepad_Back.GetVirtualKey()` |

**= 3+3+3+3+3+2+3 = 20** (was 19; block B adds exactly one, block C's net contribution is **0** —
two names change, nothing is added or removed).

* `EKeys::Escape` **in code = 1** (`:1673`) — **REQUIRED** by this row, was a BLOCKER before it.
* `EKeys::Tab` = **0** — inherited and unbound, and it stays that way.
* Letters = **0** · digits = **0** · `KBD-§` remap tables diff = **0**.
* `EKeys::` in `Tests/SiegeDeckSlotsTest.cpp` = **0** (unchanged).

⚠️ **THE TWO INSTRUMENTS DIVERGE, AND THAT IS LEGITIMATE — enumerated, not adjusted.**
A raw `grep -c "EKeys::" DeckBuilderWidget.cpp` returns **16** now, **11** at HEAD. That is a
*line* count over a *different scope*, and the gap is comments:

| instrument | HEAD | now |
|---|---|---|
| lines containing `EKeys::` | 11 | 16 |
| ├ executable | 7 | 7 |
| └ **comment-only** | **4** (`:1470, 1508, 1512, 1517`) | 9 |
| **bound key expressions** | **19** | **20** |

⛔ **The dispatching row states the HEAD comment count as 3; I measured 4.** Off by one —
`:1470` (`EKeys::Virtual_Back` in the Remove block's citation) is easy to miss because it sits with
the gamepad prose rather than the `Escape` prose. Reported because a number nobody re-derives is
how the last one of these started. **Nothing was adjusted to make a number come out right.**

---

# WHAT THIS ROW DID NOT RE-LITIGATE (`SC-§100`)

Untouched and unopened: the key table **as a set of choices** · 🧑 his **X**/**B** ruling · the
Accept/Remove logic · the exit clause's **four state conditions** and the precedence rule (block B
adds a key to the condition; it changes none of the four) · the `DECK-§6` `ScrollWidgetIntoView`
fix · `IsCardGridFocusLive()`'s live-Slate/no-tile fallback, which **stays `false`**
(`TASK-1290` WARN-3, folded — verified still `return false`) · the `+`/`−` button bodies (diff 0) ·
Accept/Remove still call the **same** `AddCopy`/`RemoveCopy` the mouse calls.
**They were always sound. They were never reachable.**

# NOTES FOR THE GATE (`TASK-1305`) AND THE HOST (`TASK-1306`)

1. ⛔ **The `.uasset` fence is 1, and it is authorised** — `Content/UI/WBP_DeckBuilder.uasset`,
   one property. `TASK-1286`'s zero-`.uasset` fence is deliberately broken by this row's spec (4).
2. ⚠️ **The editor's Git provider auto-stages saved assets.** I ran **no** git command and staged
   nothing knowingly; the host commits **by pathspec** and verifies the **commit**, never the index.
3. ⚠️ **The editor (PID 8288) is UP and must be closed for the compile** — the orchestrator owns
   the bounce. I left `DIRTY_COUNT 0`, so nothing is lost by a hard close.
4. **Suite baseline 561.** Expected delta **0** — no test added, none removed, prose only. If the
   count moves, something else moved.
5. **Warning delta target: 0** against the measured 2-warning baseline in §C1.
6. ⛔ **`Escape` is the editor's stop-PIE key** — see the escalation at the top. Zero injection
   attempts.
7. 🧑 **The acceptance question goes to him verbatim:** *"With a card outlined, does pressing
   `Enter` raise the deck total by one — or does it only open the card's details panel?"*
