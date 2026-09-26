# TASK-1432 — [MENU-NAV-CONTROLS-HELP] — programmer handoff

**Marker:** `TASK-1432-MENU-NAV-CONTROLS-HELP` · **Gate:** `TASK-1433` (QA) → 5a → 5b = `TASK-1436` → commit `TASK-1437`
**Files written (the complete list):** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` · `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` · this handoff · this row's `status:`
**Not touched:** no sub-widget class body, no subsystem, no controller, no test, no `.uasset`, no compile, no PIE, no git.

---

## 0. THE HEADLINE — AND IT IS A DEFECT THIS ROW WOULD HAVE SHIPPED IF IT HAD ONLY DONE WHAT IT WAS ASKED

⛔ **Making `CloseButton` a focus stop KILLS `Tab`-to-close.** MEASURED at four engine sites, not reasoned:

| # | Measured at | What it says |
|---|---|---|
| 1 | `Slate/Private/Framework/Application/NavigationConfig.cpp:9` + `:68-77` | `bTabNavigation` defaults **true**; `GetNavigationDirectionFromKey` maps **`Tab` → `EUINavigation::Next`** when no modifier is held. |
| 2 | project census | **0** `SetNavigationConfig` / `FNullNavigationConfig` calls anywhere in `Source/` ⇒ the default table above is live. |
| 3 | `SlateCore/Private/Widgets/SWidget.cpp:416-429` + `Slate/Private/Widgets/Input/SButton.cpp:293-320` | `SButton::OnKeyDown` forwards every **non-Accept** key to `SWidget::OnKeyDown`, which returns **`FReply::Handled().SetNavigation(...)`** for any navigation key once `SupportsKeyboardFocus()` is true. |
| 4 | `Slate/Private/Framework/Application/SlateApplication.cpp:5065-5069` | `ProcessKeyDownEvent` has **NO game-viewport fallback**. An unhandled key goes to `UnhandledKeyDownEventHandler`, whose **only binder in the entire engine** is the editor main frame (`Editor/MainFrame/Private/Frame/MainFrameActions.cpp:97`). |

⇒ ⛔ **The moment anything in this overlay holds Slate focus, `Tab` is eaten as "focus next" and NEVER reaches `SViewport` → Enhanced Input → `IA_ControlsHelp`.**

That is **not** a nicety. `HELP-§5` states the close list is *"the toggle key and/or its own on-screen Close button. That is the complete list."* and the **shipped R-24 detail prose tells the player so in as many words** (`SiegeControlsHelpWidget.cpp`, the `Interface.ControlsHelp` row: *"It closes on {Interface.ControlsHelp} or its own Close button, and that is the complete list"*). ⇒ a silent flip would have made **shipped player-facing prose FALSE with every gate green** — `HELP-§2` mechanism 4's exact named failure mode, on the one screen that can contradict the game to the player's face.

⭐ **The fix is in this diff:** one `NativeOnPreviewKeyDown` override, answering to **exactly one DERIVED key**. See §4. 🔍 **QA: this is the judgment call to attack hardest.**

---

## 1. (2) THE DE-FOCUS SITES — PER SITE, ONE BY ONE. ⛔ THE SPEC SAID "THREE"; THERE ARE **FIVE**.

The spec named `:177` (the helper) "applied to `CloseButton` `:2829`", plus the two scroll boxes. ⛔ **`ApplyButtonNotFocusable` has THREE call sites, not one.** Full census (`grep -n "ApplyButtonNotFocusable\|SetIsFocusable"`), pre-diff line numbers:

| # | Site | Owner class | Flipped? | The argument |
|---|---|---|---|---|
| 1 | `CloseButton` — `:2829` | `USiegeControlsHelpWidget` | ✅ **YES** | ⛔ **It is the WAY OUT.** A keyboard player who cannot reach it has no on-screen exit, and the ask is literally *"close it without the mouse"*. It is also the **only** admitted-class widget the walker can reach in this tree (rows 3–5 below). |
| 2 | `RowScrollBox->SetIsFocusable(false)` — `:2787` | `USiegeControlsHelpWidget` | ⛔ **NO** | The spec's container reasoning is right, and there is a **stronger, measured** one: a `UScrollBox` is **not one of `IsNavFocusStop`'s four admitted classes** (re-read live at `SiegeMenuInputSubsystem.cpp:773-870`: `UButton` / `UCheckBox` / `USlider` / `UEditableTextBox`). ⇒ flipping it would move **NOTHING** in the ring while **looking like progress**. |
| 3 | `DetailScrollBox->SetIsFocusable(false)` — `:2240` | `USiegeControlsDetailWidget` | ⛔ **NO** | Same class refusal, and it is inside a nested `UUserWidget` (row 5's mechanism) ⇒ **doubly unreachable**. |
| 4 | `RowButton` via `ApplyButtonNotFocusable` — `:1872` | `USiegeControlsHelpRowWidget` | ⛔ **NO** | ⛔ **STRUCTURAL, not stylistic** — see §2. Flipping it would be **INERT**. Its own comment's `Tab`-is-Slate's-focus-key argument is **also correct** and is now measured (§0). |
| 5 | `BackButton` via `ApplyButtonNotFocusable` — `:2330` | `USiegeControlsDetailWidget` | ⛔ **NO** | Same structural refusal, same measurement. |

⛔ **A blanket flip would have been wrong at four of five sites**, and at sites 2/3 it would have been wrong *invisibly* — the code would look changed and the ring would be identical.

---

## 2. (3) THE ROWS — ⛔ **EXPLICITLY NOT STOPS**, AND THE MECHANISM IS MEASURED, NOT ASSUMED

⛔ **Stated out loud rather than left as a silent "buttons only" (the fence-(c) relapse the spec names): `USiegeControlsHelpRowWidget` rows are NOT focus stops in this diff, and they CANNOT BE made stops from inside this file pair.**

**MEASURED** at `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/UMG/Private/WidgetTree.cpp`, `UWidgetTree::ForWidgetAndChildren` — read at source, whole function:

- it descends into **`INamedSlotInterface` named slots**, and
- it descends into **`UPanelWidget` children**,
- **and nothing else.**

A `UUserWidget` is **not** a `UPanelWidget`, and its own `WidgetTree` is a separate tree. The row class declares **no `UNamedSlot`**. ⇒ ⛔ **`GetMenuFocusStops` reaches each `USiegeControlsHelpRowWidget` OBJECT (it is a child of `RowScrollBox`, a `UPanelWidget`) but NEVER ENTERS IT.** The row object itself is refused for the ordinary reason — it is not one of the four admitted classes. `RowButton` is **never collected**, not collected-then-refused.

**What Accept WOULD do on a row, if it were ever a stop:** `SButton` Accept → `RowButton->OnClicked` → `HandleRowButtonClicked` → `ActivateRow` → `OnRowActivated` → `USiegeControlsHelpWidget::HandleRowActivated` → `ShowDetailForAction` — i.e. it would open that row's full-screen detail page. **The wiring already exists and is untouched**; only reachability is missing.

⚖️ **WHY I DID NOT BUILD IT ANYWAY — `SC-§101` (name the defect, do not smuggle a remedy).** Three shapes were available:
1. **Widen the walker to descend into nested `UUserWidget`s** — the *correct* fix, one function, and it is in `SiegeMenuInputSubsystem.cpp` which is ⛔ **READ-ONLY to this row** (a queue is behind it).
2. **Wrap every row in a parent-owned `UButton`** — in-fence but it nests two `SButton`s, changes the click surface and the visuals of a shipped, playtested screen, and `AS-§6 A(e)`/`HELP-§6` forbid any agent calling that "looks right" from a readback.
3. **Restructure the row class so its root is not a `UUserWidget`** — the spec's "sub-widgets only if unavoidable" carve. It is **avoidable**, because (1) exists and is strictly better.

⇒ ⛔ **Shipped (3)-as-declared, and BOARDED THE FINDING.** 📋 **For the manager:** *a follow-up row on `SiegeMenuInputSubsystem` to let `GetMenuFocusStops` descend into nested `UUserWidget` trees would turn this screen from 1 stop into 1 + one-per-registry-row, and would equally unlock any future list screen.* ⚠️ It is not free — it would also newly admit `WBP_HUD`-style nested trees, so it needs its own census, which is exactly why it is a row and not a line in my diff.

⚠️ **CONSEQUENCE, DECLARED:** the **detail view is keyboard-unreachable** today (you cannot activate a row without the mouse) and **has 0 stops when it is up** (the switcher collapses the list branch ⇒ `HasVisibleSlateAncestry(CloseButton)` is false ⇒ `CloseButton` is correctly dropped). ⛔ **That is not a trap:** the toggle key still closes the whole overlay from the detail view via §4's handler, and the overlay's `CloseHelp()` returns to the list first.

---

## 3. (1) WHERE I REGISTER — AND WHY IT IS **NOT** `NativeConstruct`

⛔ **`ApplyOpenState(bool bOpen)` — register on the `true` edge, unregister on the `false` edge. One `if/else`, no new state.**

- ⛔ **Not `NativeConstruct`.** This overlay is added to the viewport **CLOSED** (`CreateAndAddToViewport` → `AddToViewport`; `NativeConstruct` sets `Collapsed`). The arming backstop is **disarm-only with no rising edge on *show*** (`qa/TASK-1430.md` WARN-2) and `GetRegisteredNavTarget()` requires `IsVisible()`. ⇒ a construction-time registration **would never arm**, and would compile, review clean and do nothing (`SC-§36.1`). ⚠️ This **inverts** `TASK-1415`/`1417`/`1419`/`1425`; the inversion is written into the header block comment so the next editor cannot copy the wrong shape.
- ⛔ **Not `OpenHelp`/`CloseHelp`.** `ApplyOpenState` is the **one funnel** both routes pass through and it has a **no-op early-out**, so the edges fire **exactly once per real state change**. On `OpenHelp` a second open would register twice; on `CloseHelp` the controller's `EndPlay` path would unregister a screen that was never open.
- ⛔ **AFTER `SetVisibility`, and that ordering is load-bearing.** `RegisterMenuNavTarget` ends by placing the ring on stop 0, and both the demand predicate and `IsNavFocusStop`'s `HasVisibleSlateAncestry` limb read the **current** Slate visibility attribute. One line earlier and they would be asked about a **Collapsed** tree and answer *"0 stops"*.
- ✅ **Plus a net in `NativeDestruct`** for an overlay destroyed while open. Idempotent by the subsystem's own contract (removes by identity, logs-not-warns). ⛔ A net, not a licence.

⭐ **THE CONTROLLER CANNOT STEAL THE RING — MEASURED, NOT ASSUMED** (`Engine/Private/PlayerController.cpp`):
- On **open**, `OnControlsHelpPressed` runs `SetControlsHelpOpen(true)` → `ApplyCursorInputState()` **BEFORE** `OpenHelp()`, and that applies `FInputModeGameAndUI` with **no `SetWidgetToFocus`**. `FInputModeDataBase::SetFocusAndLocking` (`:6313-6319`) calls `SetUserFocus` **only when a widget was supplied**; `FInputModeGameAndUI::ApplyInputMode` (`:6398-6406`) supplies none. ⇒ **nothing touches focus after my register.**
- On **close**, when the last cursor owner goes away the same function applies `FInputModeGameOnly`, which **unconditionally** `SlateOperations.SetUserFocus(ViewportWidgetRef)` (`:6439-6452`). ⇒ ⭐ **the match gets its keyboard back for free, without this widget touching focus** — which is why there is deliberately no focus call on my unregister path (the subsystem's contract forbids one anyway).
- ⚠️ **DECLARED LIMIT:** if another cursor owner is still up at close (the Alt-held `IA_UICursor`), `bWantCursor` stays true, `FInputModeGameOnly` is not applied and that free restore does not happen; Slate drops focus from the collapsed button on its own. **Named rather than patched** — a focus call here would stomp a nested registration.

---

## 4. THE KEY LANE — ⛔ `Enter` IS NOT MINE, AND I SAY SO OUT LOUD

- ⛔ **In-match `Enter` stays with `IA_AssistantConsole`.** `TASK-1429` shipped `InMatchMenuMappingContextPriority = 0`, **below** the hero ⇒ ⛔ **`IA_MenuAccept` is never built in a match.**
- ⇒ ⭐ **This screen's keyboard Accept is Slate's focused-`SButton` path and nothing else:** `SButton::OnKeyDown` (`SButton.cpp:293-316`) turns `EUINavigationAction::Accept` — which `NavigationConfig.cpp:32-34` defines as **`Enter` / `SpaceBar` / gamepad accept** — into `ExecuteOnClick()` → `CloseButton->OnClicked` → `HandleCloseButtonClicked` → `CloseHelp()`. ⛔ **No `IA_MenuAccept` anywhere on this path.**
- ⭐ **SHIP-CORRECT UNDER EITHER OUTCOME of 🧑 his unresolved `Enter` ruling** (`SiegeMenuInputSubsystem.h`, `0` → `2` — ⛔ not touched by me). **What changes if he flips it:**
  - **Nothing at all while a stop on this screen holds focus.** The focused `SButton` consumes `Enter` before it can reach Enhanced Input (§0 row 4: no viewport fallback), so both `IA_MenuAccept` and `IA_AssistantConsole` are unreachable in that state either way.
  - **The only state that changes** is *overlay open but nothing focused* (a degraded path — e.g. `CloseButton` failed to construct). Today `Enter` there opens the assistant console; after a flip it would fire `IA_MenuAccept` instead. ⛔ Neither is a behaviour this diff relies on.
  - ⛔ **My `NativeOnPreviewKeyDown` is unaffected by the flip in both directions** — it matches only the derived `IA_ControlsHelp` key and refuses `Escape`.

**The `NativeOnPreviewKeyDown` override itself:**
- ⛔ **PREVIEW (tunnel), not `NativeOnKeyDown` (bubble) — forced, not stylistic.** The bubble starts at the focused leaf, so `CloseButton`'s `SButton` would consume `Tab` before any ancestor saw it. The tunnel runs root→leaf along the same focus path (`SlateApplication.cpp:5024-5041`) and `SObjectWidget::OnPreviewKeyDown` forwards here unconditionally (`UMG/Private/Slate/SObjectWidget.cpp:221-229`).
- ⛔⛔ **`Escape` is refused FIRST, unconditionally, in its own statement.** `HELP-§5` names `NativeOnPreviewKeyDown` **by name** as a forbidden route for it. It is a separate statement on purpose: the toggle key is *derived*, so a future remap of `IA_ControlsHelp` onto `Escape` would otherwise be claimed by the match below. This guard is what makes the ruling hold under a remap nobody here can see.
- ⛔ **`KBD-§1`/`KBD-§2` UNTOUCHED — this is a READ, not a binding.** No `MapKey`/`UnmapKey`/`UnmapAll`, no `IMC_*` write, no context mutation, **no key typed**. `IsOwnToggleKey` looks up the **`Interface.ControlsHelp` registry row** and calls the **existing** `QueryAppliedKeysForRow` — the *same* row and the *same* accessor the R-24 key chip and the hint line already read (`HELP-§4`: *the menu documents its own key*). There is no second copy of the truth to drift.
- ⛔ **APPLIED keys, not DISPLAY keys** — `HELP-§1`'s lane audit. `QueryAppliedKeysForRow` answers out of the **active, already-retargeted** context, which is the key the player physically presses and exactly what `FKeyEvent::GetKey()` reports. `ResolveRowDisplayKeys` is the **label** lane and on its fallback answers the QWERTY *reference* key — a key the player is not pressing on a moved layout. ⇒ Dvorak-correct by construction.
- ⛔ **No double-fire is possible, by construction:** the handler runs only when the overlay is in the focus path, and in exactly that state the viewport is **not**, so the Enhanced Input binding cannot also fire. The two lanes are mutually exclusive.
- ✅ `IsRepeat()` is refused, mirroring the shipped `ETriggerEvent::Started` binding.

---

## 5. (4) THE `:2958` LAW AND `KBD-§` — STATED AS THE SPEC DEMANDS

- ✅ ⛔ **NO `SetInputMode` AND NO `bShowMouseCursor` WAS INTRODUCED ANYWHERE IN THIS FILE PAIR.** MEASURED: `grep -n "SetInputMode\|bShowMouseCursor\|SetKeyboardFocus\|SetUserFocus\|MapKey\|UnmapKey\|UnmapAll"` over both files returns **12 hits, ALL OF THEM PROSE IN COMMENTS, ZERO CALL SITES.** `ASiegePlayerController::ApplyCursorInputState` remains the sole posture owner (`HELP-§5`, `TASK-074`).
- ✅ ⛔ **NO INPUT-BINDING EDIT (`KBD-§1`/`§2`).** This screen stays a **read-only consumer** of the layout system. No asset touched, no mapping context created/applied/removed/mutated by this file.
- ✅ ⛔ **`Escape` NOT CLAIMED** (`AS-§6 A-2`, `HELP-§5`): the one occurrence in a handler is a **refusal that returns `Super`**, i.e. Unhandled-by-me.

---

## 6. THE EXPECTED STOP LIST — ⛔ STATED **BEFORE** 5b SO `TASK-1436` CAN FALSIFY IT

🚨 **ANCHORING — READ THIS FIRST, IT IS NOT A FORMALITY.**
⛔ **THE ANCHOR IS `UNANCHORED` BY OBJECT PATH, IN THOSE WORDS, AND IT IS UNANCHORABLE BY CONSTRUCTION.** This screen has **no design-time asset**: `/Game/UI/WBP_ControlsHelp` is `HELP-§3`-RESERVED and **unauthored**, and the whole tree is built at runtime by `WidgetTree->ConstructWidget`. There is no `.uasset` path to cite and the runtime outer is instance-numbered.
⚠️ **AND A BARE NAME IS NOT AN IDENTIFIER HERE — MEASURED:** `CloseButton` exists in **two** places in this project (`SiegeControlsHelpWidget.cpp`'s `ConstructWidget<UButton>(…, TEXT("CloseButton"))` **and** `Content/UI/WBP_WarMap.uasset`), and `MoveFocus` prints a bare `GetName()` with no screen qualifier. ⇒ ⛔ **`TASK-1436` MUST qualify by the OWNING SCREEN** — the subsystem's own retarget line does this for you: it prints `-> '<owner>'` beside the count.

**The stop set, in `ForEachWidget` depth-first pre-order, with the class-level reason for every node:**

| Tree chain (`USiegeControlsHelpWidget::WidgetTree`) | Class | Stop? | Why |
|---|---|---|---|
| `BackdropBorder` | `UBorder` | ⛔ | not an admitted class |
| ` └ ViewSwitcher` | `UWidgetSwitcher` | ⛔ | not an admitted class |
| `   ├ PanelBorder` *(switcher child 0 = list)* | `UBorder` | ⛔ | not an admitted class |
| `   │ └ RootPanel` | `UVerticalBox` | ⛔ | not an admitted class |
| `   │   ├ TitleText` | `UTextBlock` | ⛔ | not an admitted class |
| `   │   ├ HintText` | `UTextBlock` | ⛔ | not an admitted class |
| `   │   ├ RowScrollBox` | `UScrollBox` | ⛔ | not an admitted class (its `IsFocusable=false` is **irrelevant** to the walker) |
| `   │   │  └ category headers + N row widgets` | `UTextBlock` / `USiegeControlsHelpRowWidget` | ⛔ | not admitted classes; ⛔ **and the walker does not enter the row trees** (§2) |
| `   │   └ ` **`CloseButton`** | **`UButton`** | ✅ **THE ONE STOP** | admitted class · enabled · visible · visible ancestry · **`IsFocusable` now `true`** · `FindStepperPair` refuses it (only `UButton` under `RootPanel`, so no pair exists) |
| `   │      └ CloseLabelText` | `UTextBlock` | ⛔ | not an admitted class |
| `   └ DetailView` *(switcher child 1)* | `USiegeControlsDetailWidget` | ⛔ | not an admitted class; ⛔ **and the walker does not enter it** ⇒ `BackButton` is never collected |

⛔ **EXPECTED STOP SET (the property, not an index): `{ the single UButton named "CloseButton" owned by the OPEN USiegeControlsHelpWidget's own WidgetTree }`. Cardinality 1 is a CONSEQUENCE of that property, never the criterion.**

### Acceptance, written as PROPERTIES (⛔ never as an index — a gate written in indices has an expiry date nobody printed on it)

| # | Property | Falsifiable how |
|---|---|---|
| **P0** | *(regression fence)* A match in which the overlay is **never opened** emits **no** `menu nav target registered` line at all, and every hero key behaves byte-identically. | absence of the line + hero input unchanged |
| **P1** | On the **first** open, the subsystem emits **exactly one** line whose target is **this screen**, whose source reads **`(registered screen)`**, and whose stop count is the cardinality of the set above. | `[USiegeMenuInputSubsystem] menu nav target registered -> '<this screen>' (registered screen), 1 focus stop(s), 1 screen(s) registered.` ⛔ A count of **0** is the defect signature. |
| **P2** | With that stop focused, **`Enter` or `SpaceBar`** closes the overlay — via Slate's Accept path, ⛔ not `IA_MenuAccept`. | `[ControlsHelp] Overlay closed.` **and** `menu nav target unregistered` |
| **P3** | With that stop focused, **the derived toggle key still closes it** (§0's regression, fixed). ⛔ **This is the one to try to break.** | same two lines as P2 |
| **P4** | `Escape` while open does **nothing** — the overlay stays up and no cancel route changes. | overlay still drawn; no close line |
| **P5** | On close, focus returns to the game (hero keys live again) **without this widget touching focus**. | hero responds to WASD immediately after close |

**Unhappy-path count: 3, each with a distinct discriminator string — ⛔ discriminated by the STRING, never by a number.**

| Unhappy path | Discriminator (an existing or new log line) | Verdict |
|---|---|---|
| No `USiegeMenuInputSubsystem` on the world (Editor/designer world) | **`[ControlsHelp] No USiegeMenuInputSubsystem on this world - the overlay is mouse-only…`** (new, `Log`) | ⛔ **NOT a defect** — keyboard nav unavailable, not broken |
| `CloseButton` failed to construct | **`[ControlsHelp] Could not construct CloseButton - the overlay can only be closed by its toggle key.`** (pre-existing, `Error`) | ⛔ real defect, already loud |
| An asset-authored `WBP_ControlsHelp` lands with a non-focusable Close control | **`[ControlsHelp] An asset-authored tree is present - the code-authored branch is skipped (HELP-§3).`** (pre-existing, `Log`) + a **0-stop** retarget line | ⛔ the `HELP-§3` escape hatch working as designed; the asset's author owns it |

⚠️ **`Log` verbosity is compiled out under Shipping** (`NO_LOGGING`), as the subsystem's own comment records. That is a precondition on the reader, not a reason to raise it. **5b must run `Log LogSiegeMenuInput Verbose` / `LogSiegeControlsHelp Verbose` or an empty log will read as a zero-stop false pass.**

---

## 7. DIFF SHAPE — ⛔ EVERY ONE OF THE 8 DELETIONS DECLARED

`+428 / −8` across the two files. The house shape is `+N/−0`, so here is all 8, line by line:

| Lines | What | Kind |
|---|---|---|
| **2** | `ApplyButtonNotFocusable(CloseButton);` + its one-line comment | ⭐ **REPLACED IN PLACE** by `ApplyButtonFocusable(CloseButton);` + the argued comment. **This single statement IS the row.** |
| **5** | the `§2` class-comment paragraph claiming *"ALL THREE classes … override NO key handler at all"* | ⭐ **SUPERSEDED IN PLACE** under `SC-§53`. ⛔ The struck sentence is **quoted verbatim** in the replacement and the conclusion it supported is **re-affirmed** — a struck premise shows the next reader the question was asked and answered. |
| **1** | the `§3` citation `SiegePlayerController.cpp:4357` | ⭐ **CORRECTED IN PLACE** to `:6457`. **MEASURED:** the `const bool bWantCursor = …` expression is at `:6457` today; `:4357` now lands in an unrelated formation comment. `SC-§53` / `HELP-§2`'s citation-rot bullet require the re-read in the same batch. |

⛔ **ZERO code statements deleted** other than the one de-focus call this row exists to replace. **7 of the 8 are comment lines.**

**Reflection surface:** ⛔ **no new `UPROPERTY`, no new `UFUNCTION`, no new `UCLASS`/`USTRUCT`, no new member variable, no changed base class.** The four new functions are plain `protected` members and one engine `virtual` override. ⇒ `SiegeControlsHelpWidget.generated.h` is **PREDICTED byte-identical** — ⛔ **a change there is a real finding.**

**C4458 sweep — against the BASE CHAIN, ⛔ not the repository** (a `Source/` grep is a false negative by construction). The chain is **EIGHT**: `UUserWidget` → `UWidget` → `UVisual` → `UObject` → `UObjectBaseUtility` → `UObjectBase`, **plus** `INamedSlotInterface` (on `UUserWidget`, `UserWidget.h:280`) and `INotifyFieldValueChanged` (on `UWidget`, `Widget.h:216`). Every new local — `MenuInput`, `World`, `ToggleRow`, `ToggleKeys`, `InKey`, `Button` — swept against all eight headers: **0 hits each.**

**Line endings:** ⛔ checked, because a mixed-ending file is a QA finding. **`SiegeControlsHelpWidget.cpp` = 3634/3634 CRLF; `.h` = 1192/1192 CRLF.** No mixed endings introduced. The `LF will be replaced by CRLF` git warning is the repo-wide `autocrlf` message about the LF-normalized index blob and is present for untouched files too.

**Suite:** ⛔ **NOT cited as evidence.** `GetMenuButtons()` and `IsNavFocusStop()` have diverged (`TASK-1475` is closing that), and `SiegeControlsHelpTest.cpp` is fenced read-only to me.

---

## 8. 🔍 WHAT QA (`TASK-1433`) SHOULD SCRUTINISE HARDEST

1. ⛔ **The `NativeOnPreviewKeyDown` override is the one thing this row added that no sibling screen has.** Spec item (4) forbids `SetInputMode`/`bShowMouseCursor` and input-binding edits; it does **not** forbid a key handler, and `HELP-§5` bans `NativeOnPreviewKeyDown` **only for `Escape`**. ⛔ **If you judge it out of scope, the correct verdict is not "delete it" — it is "then `CloseButton` must not become focusable either", because §0 shows the two ship or fall together.**
2. ⛔ **Re-measure §0 yourself.** Four engine files, four line ranges, all cited. If `UnhandledKeyDownEventHandler` has a second binder I missed, my whole Tab argument collapses and the handler becomes dead code.
3. ⛔ **Re-measure §2's `ForWidgetAndChildren` claim.** If the walker *does* reach nested `UUserWidget`s, then sites 4 and 5 in §1 were wrongly refused and the stop set in §6 is wrong by N rows.
4. ⚠️ **The 1-stop ring is a real limitation, not a claim of completeness.** 🧑 His sentence is *"move an outline through the screen **and** close it without the mouse"*. ⛔ **This diff delivers the second half fully and the first half only degenerately** (one stop ⇒ the outline appears and `Up`/`Down` wrap to it). ⛔ **I am not calling this screen "navigable" in his sense** — §2 names the exact follow-up that would make it so, and it is one function in a file I may not write.
5. ✅ The per-site decisions in §1 — especially that flipping the two scroll boxes would have been an **invisible no-op**, which is a worse outcome than not flipping them.

---

# ⭐ QA LOOP 1 — APPENDED 2026-09-25 (nothing above is rewritten; every correction is made here by strike-and-restate, `SC-§120`)

Against `.claude/pipeline/qa/TASK-1433.md` — **1 BLOCKER · 2 WARN · 2 NIT**. All five are addressed. **Two files touched, the same two: `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.{cpp,h}`.** No compile, no PIE, no MCP mutation, no editor lifecycle action, no git, no `.uasset`, no `CONVENTIONS.md`, and `SiegeMenuInputSubsystem.{cpp,h}` was **read only and cited by text, never by line** (`TASK-1474` holds it and it will move under me).

## L1.1 — 🚨 BLOCKER-1: the `UWidgetSwitcher` claim was FALSE. I re-measured it myself, it is worse than QA said, and the remedy is in-fence.

**§2's "CONSEQUENCE, DECLARED" and the header paragraph were wrong. STRUCK:**
> ~~"the detail view … has 0 stops when it is up (the switcher collapses the list branch ⇒ `HasVisibleSlateAncestry(CloseButton)` is false ⇒ `CloseButton` is correctly dropped)"~~

**Re-measured at four engine sites — I did not take QA's word for any of them:**

| # | Read | What it actually says |
|---|---|---|
| 1 | `Slate/Private/Widgets/Layout/SWidgetSwitcher.cpp` | `OnArrangeChildren` calls `ArrangeSingleChild` for `GetActiveSlot()` and nothing else. The **only two** occurrences of `Visibility` in the file are a **read** inside `ComputeDesiredSize`. ⇒ it declines to **arrange** the inactive branch; it never **collapses** it. |
| 2 | `UMG/Private/Components/WidgetSwitcher.cpp` | `SetActiveWidgetIndexForSlateWidget` forwards a clamped index. No child visibility is touched. |
| 3 | `UMG/Private/Components/Widget.cpp` | `UWidget::IsVisible()` returns `SafeWidget->GetVisibility().IsVisible()` — the widget's **own** visibility. |
| 4 | ⭐ **`SlateCore/Private/Application/SlateWindowHelper.cpp`, `FindPathToWidget` — the site QA did not take, and it is the decisive one** | The engine's own comment: *"Even if the parent pointer is valid, and even if the visibility is visible, it's possible a widget shows and hides children without ever removing them this is the case with widgets like SWidgetSwitcher"*, immediately followed by `if (!CurWidgetParent->ValidatePathToChild(CurWidget.Get())) { OutWidgetPath.Widgets.Empty(); return false; }`. And `SWidgetSwitcher::ValidatePathToChild` is `return InChild == GetActiveWidget().Get();`. |

⇒ **the refutation is not an inference from an absence of visibility writes — the engine has a dedicated hook whose stated purpose is this exact widget, and it makes `FindPathToWidget` return `false`.** The stop is admitted by the predicate and refused by Slate, which is the swallow condition the subsystem describes in its own words.

**And `CloseButton` really is behind that hook.** The tree `ConstructHelpTree` builds is `BackdropBorder → ViewSwitcher → [0] PanelBorder → RootPanel → CloseButton`, with `DetailView` as slot `[1]`. This screen's **only** stop lives in the list slot.

### ⚖️ The decision QA deliberately did not make for me (`SC-§101`), and my argument for it

**Chosen: the registration's lifetime becomes exactly the ring's — `registered ⟺ bHelpOpen AND the list view is up`.** `ShowDetailForAction` unregisters on a real list→detail edge; `ReturnToList` re-registers on a real detail→list edge; `ApplyOpenState` keeps the open/close edge unchanged. **No new member, no new state** — both edges read `IsDetailViewActive()`, the single source of truth the file already had.

- **Rejected (a) — teach the predicate about `SWidgetSwitcher`.** It is the right *general* fix and it is **one function in `SiegeMenuInputSubsystem.cpp`, which this row may not write** (`TASK-1472` holds it, `TASK-1474` is queued). It would also leave the hazard live until that lands. ⭐ **When it does land, these two edges become belt-and-braces rather than wrong**: they can only ever drop a stop the fixed predicate would also have dropped, so the two agree by construction and nothing has to be unwound. I flagged this in the header so `TASK-1474`'s editor finds it.
- **Rejected (b) — collapse the list branch so the existing ancestor test does the work.** It would make the struck sentence true, and it is the wrong trade: it invents a **second source of truth** for which view is up (exactly what `IsDetailViewActive`'s own comment refuses), and the failure mode of a missed restore is a screen whose **only on-screen exit is invisible**. A cosmetic gain is not worth a new way to strand the player.
- **Rejected (c) — give the detail page a stop of its own.** Not available in-fence: `BackButton` lives inside `USiegeControlsDetailWidget`'s own `WidgetTree`, which the walker never enters, and both ways to change that (re-rooting the detail class, or a wrapper button) change a shipped screen's visuals — `AS-§6 A(e)`/`HELP-§6` forbid any agent adjudicating that.

### ⚠️ What I am declaring about my own fix, before 5b looks

1. **Up/Down are now INERT on the detail page, not DEAD.** With nothing registered, `MoveFocus` has no target and does nothing. That is the honest state: the page has no reachable stop. ⛔ It is **not** a claim that the detail page is navigable.
2. **One transient `registered` → `unregistered` log pair on exactly one route.** Closing **from** the detail page runs `CloseHelp()` → `ReturnToList()` (registers; `FocusFirstNavStop` puts the ring on `CloseButton`) → `ApplyOpenState(false)` (unregisters) inside one call stack. Both lines are true of states the program really passes through and the end state is correct. **I did not reorder `CloseHelp` to suppress them** — swapping a shipped close route's two statements to tidy a log trades a real risk for a cosmetic gain, and the transient focus placement lands on the same button that already holds focus on an ordinary close.
3. **Both edges are placed AFTER the switcher write**, for the same reason `ApplyOpenState` registers after its `SetVisibility`: `UWidgetSwitcher::SetActiveWidgetIndex` assigns synchronously and `UWidgetSwitcher::GetActiveWidgetIndex` reads back through the live Slate widget, so `ValidatePathToChild` already answers with the new slot by the time the subsystem is told.
4. **Both edges are guarded on a REAL transition, not on arrival.** `OpenHelp()` and `CloseHelp()` both call `ReturnToList()` unconditionally; without `bWasOnDetail` every open and every close would register spuriously. `ShowDetailForAction` is `virtual` + `protected`, so `bWasOnList && bHelpOpen` is what stops a later caller unregistering something that was never registered.
5. ⛔ **This is derived from engine source, not observed at runtime** — the same provenance caveat QA put on the finding. **P6 below is what settles it.**

## L1.2 — ⭐ WARN-1: my 5b instruction was WRONG, and the correction FREES the lane. STRUCK.

> ~~"5b must run `Log LogSiegeMenuInput Verbose` / `LogSiegeControlsHelp Verbose` or an empty log will read as a zero-stop false pass."~~

**Measured and conceded in full.** Both categories are declared `DECLARE_LOG_CATEGORY_EXTERN(..., Log, All)` (`SiegeControlsHelpWidget.h`, `SiegeMenuInputSubsystem.h`) and **every acceptance line is emitted at `Log`** — the overlay-open line, the new no-subsystem line, and the subsystem's retarget line. ⇒ **they print in a Development editor/PIE run with no console verb at all.** I conflated Shipping's `NO_LOGGING` (where `Log` really is compiled out) with a Development verbosity raise; they are different facts and only the first is true.

⇒ **`TASK-1436`: do NOT record `UNOBSERVABLE` on the grounds that the verifier cannot raise verbosity. That ground does not exist here.** Read `Saved/Logs/GitClaudeUnrealTest.log` with the ordinary reader. ⛔ **An absent line is a ZERO and a defect signature, not an instrument failure.**

## L1.3 — WARN-2: citation rot, now repaired at **all four** sites, and re-anchored to text

`.h` `§3` was already corrected in the first pass. The three left behind are now fixed **by text and by name**, which is the rule the header itself writes:

| Site | Was | Now |
|---|---|---|
| `.cpp` R-08 block | `the one boolean expression at :4357` | the `const bool bWantCursor = bInPlacementMode ... bUICursorHeld;` statement in `ASiegePlayerController::ApplyCursorInputState` |
| `.cpp` R-19 block | `SiegePlayerController.cpp:4357-4375` | `ASiegePlayerController::ApplyCursorInputState` + the `bWantCursor` statement |
| `.cpp` **R-24 block (this screen's own)** | `SiegePlayerController.cpp:4326-4376, the one expression at :4357` | `ASiegePlayerController::ApplyCursorInputState` **by name** + the statement **by text**, with this row's own `bControlsHelpOpen` term named |

Verified both ends: the statement is the **only** `bWantCursor` declaration in `SiegePlayerController.cpp`, inside `ApplyCursorInputState`; the old `:4357` lands in an unrelated formation comment. Every struck number is quoted in place, never deleted.

⚠️ **AND A SCOPE DECLARATION I AM MAKING RATHER THAN LETTING A READER ASSUME:** while re-anchoring I spot-checked the *neighbouring* numbers in those same `Citations (T1)` blocks and **they are rotted too** — e.g. R-08's `:494-499` ("the Completed+Canceled binding") today lands in the frame-rate-counter construction, and `:783-790` lands in the `IA_DiscardAll` bind. **I fixed only the four anchors QA named, and I have said so at each site in the file.** A full re-anchoring of this file's citation blocks is a real follow-up and **nobody has boarded it** — it is not silently folded in here, because an unbounded comment rewrite inside a QA-loop diff is how a review loses its ability to see the fix. I also re-anchored one number inside a function I was already editing (`ReturnToList`'s `WidgetSwitcher.cpp:49-57`, off by one in UE 5.8) rather than leave rot in a line I had just touched.

## L1.4 — 📎 NIT-1: my line-ending declaration was INVERTED, and I found the instrument that did it

**Struck:** ~~"`SiegeControlsHelpWidget.cpp` = 3634/3634 CRLF; `.h` = 1192/1192 CRLF."~~ QA is right: both files were **pure LF, 0 CRLF**, and my "measurement" reported the total line count.

⭐ **Root cause, and it is worth recording because it will bite someone else:** the naive Git-Bash probe `grep -c` for a carriage-return-at-end-of-line pattern returns **the total line count** on a pure-LF file — I reproduced that live tonight, getting `CRLF=3801` on a file with **zero** carriage returns. The honest probe is a byte count: `tr -cd '\r' < file | wc -c`.

**Re-measured after this pass, with the honest probe:** `.cpp` = **0 CR / 3801 LF**, `.h` = **0 CR / 1238 LF** — **both still pure LF, unchanged, none mixed**, and matching `SiegeMenuInputSubsystem.cpp` (0 CR / 2064 LF) as QA's 160-file census says they should.

## L1.5 — 📎 NIT-2: `ForWidgetAndChildren` was half-stated. Corrected in place, because `TASK-1474` reads that comment first.

**Struck at the flip site:** ~~"A `UUserWidget` is neither, and the row widget declares no `UNamedSlot`."~~

Verified at source: `class UUserWidget : public UWidget, public INamedSlotInterface` (`UMG/Public/Blueprint/UserWidget.h`) ⇒ `Cast<INamedSlotInterface>(RowWidget)` **succeeds** and the **first limb IS entered**. It yields nothing only because `GetSlotNames` comes back empty — the row class declares **no `UNamedSlot`** (zero in the pair). The conclusion is unchanged and now rests on **both limbs failing for different reasons**: limb 1 entered and empty, limb 2 never entered (a `UUserWidget` is not a `UPanelWidget`). ⚠️ The comment now also names the **fragile** half: **one `UNamedSlot` added to the row class would open limb 1**, and that is the thing `TASK-1474` should know before it changes this traversal.

## L1.6 — What did NOT change, stated so the re-review can be short

- ⛔ **`NativeOnPreviewKeyDown` is untouched.** Ruled in scope — *"not a second change, the other half of one change"*. The `Escape`-first refusal, the repeat guard and the derived-key match are byte-identical.
- ⛔ **The `CloseButton` focusable flip is untouched**, and so are the four refusals beside it (only the `RowButton` bullet's *mechanism* prose was tightened per NIT-2).
- ⛔ **Registration still happens on the SHOW edge in `ApplyOpenState`, after `SetVisibility`, never in `NativeConstruct`** — QA recorded this as the first non-vacuous satisfaction of `qa/TASK-1430.md` WARN-2 and I have not disturbed it. `ApplyOpenState`'s comment gained a clause (5) saying it is no longer the *only* registration edge, so clause (3) cannot be read as the whole contract.
- ⛔ **No `SetInputMode`, no `bShowMouseCursor`, no `SetKeyboardFocus`/`SetUserFocus`, no `MapKey`/`UnmapKey`/`UnmapAll`** — the census is unchanged by this pass; the only new statements are two `if`-guarded calls to this file's own register/unregister helpers plus two `const bool` locals.
- ⛔ **Reflection surface still unchanged** — no `UPROPERTY`, no `UFUNCTION`, no new member variable, no base change ⇒ `SiegeControlsHelpWidget.generated.h` is still predicted byte-identical, and a change there is still a real finding.
- ⛔ **No new locals shadow anything** — `bWasOnList` / `bWasOnDetail` do not exist anywhere in the eight-base chain.

## L1.7 — 🔍 For `TASK-1436` (5b): the probe QA earned, plus the two corrections that change how you read the log

1. ⭐ **P6, THE DISCRIMINATING OBSERVATION — run it first.** Open the overlay, **mouse-click a row** into the detail page, then press **Up/Down**.
   - **This diff predicts:** the subsystem logs an **unregister** on the click; arrow keys do **nothing**; pressing the toggle key still closes the overlay; **Back** (mouse) returns to the list and logs a **register** whose retarget line reads **1 focus stop** again, with the outline back on `CloseButton`.
   - **The un-fixed code predicted:** the target stays registered, the retarget line keeps saying 1 stop, and Up/Down **repeat forever on a stop that never takes focus**.
   - ⛔ **If you see the second shape, this fix did not take** — and that is a finding, not an instrument problem.
2. ⛔ **No console verb is needed.** Both categories default to `Log` and every acceptance line is `Log`. ⛔ **Do not record `UNOBSERVABLE` for want of a verbosity raise.** An absent line is a **zero**.
3. ⛔ **Qualify `CloseButton` by owning screen** — `Content/UI/WBP_WarMap.uasset` has one too. Use the retarget line's owner field, never a bare name.
4. ⚠️ **Expect one transient `registered`/`unregistered` pair when closing FROM the detail page** (L1.1 §2). It is declared in advance and is **not** churn.
5. ⛔ **The expected stop set is unchanged and is still a PROPERTY, never an index:** `{ the single UButton named "CloseButton" owned by the OPEN USiegeControlsHelpWidget's OWN WidgetTree }`, **while the list view is up**. Cardinality 1 is a consequence. **On the detail page the correct observation is now "no registered nav target", not "0 stops".**
6. ⛔ **Do not cite the automation suite as evidence** — `TASK-1476` did not lift that prohibition.
