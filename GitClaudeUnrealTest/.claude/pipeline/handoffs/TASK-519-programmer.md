# TASK-519 — [AX-4] CONSOLE UX: no Cancel button, no Accept button, `Z` accepts — programmer handoff

- **Task:** TASK-519 (batch ASSISTANT-EXCLUDE) · **assignee:** gameplay-programmer
- **Status set to:** `ready-for-qa`
- **QA gate:** **TASK-525** (`.claude/pipeline/qa/TASK-525.md`). ⛔ No other gate file exists for this batch.
- **Law implemented:** CONVENTIONS **`AS-§20.5`** · **`AS-§6` RULING A (pinned children, amended 2026-08-04) + RULING A-2 (Escape CLOSED; close-is-a-cancel amended)** · `AS-§2` · `AS-§3` · `AS-§11` · **`KBD-§8` (the `GetPositionalKey` pin)** · `KBD-§0` ruling 1 · `KBD-§5` (fail-safe) · `SC-§15` (declared departures, §8 below)
- **Date:** 2026-08-04
- ⛔ **NOT COMPILED** — TASK-526 owns the batch's only compile. ⛔ No tests written — TASK-523 owns those. ⛔ No Git, no editor/MCP/PIE.

## M8 DECLARATION (verbatim, as required)

**adds no replicated property, no new replicated class, no new relevancy tier.**

Reason, unchanged from the file's shipped declaration (which is still in the class comment): this is a
client-local input surface whose only outputs are delegate broadcasts to a local component. The one
new member is an `FString` display cache; the one new engine seam is a Slate preview-key override,
which is a client-side UI event by construction.

## Files touched — TWO, and only two

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h` | Cancel button + thunk deleted (retirement comment in their place) · `NativeOnPreviewKeyDown` override declared · `ConfirmButton` re-documented as pinned-but-not-constructed · `CancelPressed()` documented as deliberately uncalled · three private helpers + one `FString` member · `InputCoreTypes.h` include + `USiegeKeyboardLayoutSubsystem` forward decl · class comment §2/§2b/§4 updated. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` | button row + both buttons no longer constructed · Cancel bind + thunk + labels + `ButtonLabelSize` + `HorizontalBox` includes deleted · `NativeOnPreviewKeyDown` / `ResolveKeyboardLayoutSubsystem` / `GetAcceptKey` / `RefreshStatusLine` added · `OpenConsole` refreshes the layout once · `SetAssistantState` + `ApplyConsoleVisualState` route through the status-line composer · new `ConfirmHintText` chrome string. |

⛔ **`SiegeAssistantComponent.{h,cpp}` WAS NOT OPENED FOR EDIT.** No controller edit, no `Content/`, no `Build.cs`, no `.uasset` (the tree is code-authored under ruling A — there is none).

## 1. THE PREVIEW HANDLER, AS WRITTEN

```cpp
FReply USiegeAssistantConsoleWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const bool bAcceptIsLive = bConsoleOpen && bConsoleEnabled && bConfirmPromptVisible;

    const bool bIsUnmodified =
        !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown() && !InKeyEvent.IsCommandDown();

    if (bAcceptIsLive && bIsUnmodified && InKeyEvent.GetKey() == GetAcceptKey())
    {
        UE_LOG(LogSiegeAssistant, Log, TEXT("[AssistantConsole] Accept key pressed ..."), *InKeyEvent.GetKey().ToString());

        ConfirmPressed();

        return FReply::Handled();
    }

    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
```

**Why `Handled` is not a lie, and why it is NOT re-checked after the call.** `ConfirmPressed()` has
exactly one refusal — `if (!bConfirmPromptVisible)` — and that is the condition the branch has already
established as true, so the call cannot be refused. ⚠️ **Re-reading the flag afterwards would be
WRONG, not merely redundant:** a consumer of `OnConsoleConfirmed` may synchronously raise a NEW prompt
(the deferred-intent path re-enters `AwaitConfirm`), which would leave the flag true again and make a
post-check report "not accepted" — letting the same physical press ALSO type a `z` into the box after
it had already executed an order. **Acceptance is proven from the PRE-condition.**

## 2. THE KEY FALL-THROUGH TABLE (what is consumed, what is never touched)

| key / state | verdict | route |
|---|---|---|
| **`Escape`, always, in every state** | ⛔ **NEVER CONSUMED** | falls to `Super` → `FReply::Unhandled()`. **The token `Escape` does not appear anywhere in the implementation.** |
| accept key (positional QWERTY-`Z`), console **open** + **enabled** + `bConfirmPromptVisible` **true**, no Ctrl/Alt/Cmd | ✅ **CONSUMED** | `ConfirmPressed()` then `FReply::Handled()` |
| same key, console **closed** | untouched | `Super` → Unhandled (and the widget is `Collapsed` + unfocused, so the tunnel does not reach it at all) |
| same key, console open but **no confirm prompt** | untouched — **types a normal `z`** | `Super` → Unhandled |
| same key, console open + prompt up, but **console disabled** | untouched | `Super` → Unhandled (mirrors the old button's `SetIsEnabled(bConsoleEnabled)`) |
| **`Ctrl`+Z / `Ctrl`+`Shift`+Z / `Alt`+Z / `Win`+Z**, prompt up | ⛔ **NOT CONSUMED** | `Super` → Unhandled. See §8(a): these are the text box's undo/redo. |
| **`Shift`+Z**, prompt up | ✅ consumed | still "the Z key" to a human |
| `Enter` (open / submit / close-route-4) | untouched | `Super` → Unhandled ⇒ `HandleTextCommitted` and the controller toggle behave byte-identically |
| **every other key on the keyboard, in every state** | untouched | `Super` → Unhandled |

⛔ **THE FALL-THROUGH DELIBERATELY GOES THROUGH `Super`, NOT A BARE `FReply::Unhandled()`.**
`UUserWidget::NativeOnPreviewKeyDown` is `return OnPreviewKeyDown(InGeometry, InKeyEvent).NativeReply;`
(`UserWidget.cpp:2500-2503`) — the `BlueprintImplementableEvent` route. Nothing implements it in v1, and
an unimplemented BIE returns a default-constructed `FEventReply` whose `NativeReply` is
`FReply::Unhandled()` (`SlateWrapperTypes.h:134-137`), **so this IS Unhandled today**. Hard-coding
`Unhandled` would silently delete a Blueprint entry point from every future subclass — including
ruling A(b)'s `WBP_AssistantConsole`. ⚠️ **The residual duty is written into the code:** a future WBP
author who implements `OnPreviewKeyDown` and returns Handled for `Escape` would overturn Jonathan's
ruling from Blueprint.

## 3. ENGINE-SOURCE VERIFICATION (quoted, per AS-§20.5 — this mechanism has zero precedent in `Source/`)

Read from the installed UE 5.8 tree on this machine, not remembered:

1. **`FSlateApplication::ProcessKeyDownEvent` runs the TUNNEL pass FIRST**, and the bubble pass only
   `if (!Reply.IsEventHandled())` — `SlateApplication.cpp:5024-5046`:
   ```cpp
   // Tunnel the keyboard event
   Reply = FEventRouter::RouteAlongFocusPath(this, FEventRouter::FTunnelPolicy(EventPath), InKeyEvent,
       [](const FArrangedWidget& CurrentWidget, const FKeyEvent& Event)
       { ... CurrentWidget.Widget->OnPreviewKeyDown(CurrentWidget.Geometry, Event) ... });

   // Send out key down events.
   if ( !Reply.IsEventHandled() ) { ... FBubblePolicy ... OnKeyDown ... }
   ```
2. **`FTunnelPolicy` walks the focus path ROOT → LEAF** — `SlateApplication.cpp:347-366`: it is
   constructed `WidgetIndex(0)`, `ShouldKeepGoing()` is `WidgetIndex < RoutingPath.Widgets.Num()` and
   `Next()` is `++WidgetIndex`. ⇒ **an ANCESTOR of the focused widget is offered the key before the
   focused `SEditableText` can turn it into the character `z`.**
3. **`SObjectWidget` forwards that pass into this class** — `SObjectWidget.cpp:221-228`:
   ```cpp
   FReply SObjectWidget::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
   {
       if ( CanRouteEvent() ) { return WidgetObject->NativeOnPreviewKeyDown( MyGeometry, InKeyEvent ); }
       return FReply::Unhandled();
   }
   ```

⇒ **`NativeOnKeyDown` alone is confirmed NOT viable**: the bubble pass starts at the focused leaf,
which has already handled a printable key.

### ⛔ `SetIsFocusable(true)` IS **NOT** CALLED, AND THAT IS THE VERIFIED ANSWER

⚠️ **The dispatch prompt said to add it ("plus `SetIsFocusable(true)`"). The BOARD SPEC clause (4) said
to VERIFY whether it is required and *"if it is not, say so rather than adding it 'to be safe'"*. I
verified: IT IS NOT REQUIRED.** Focusability decides whether a widget can *be* the focus target; the
tunnel pass walks the focus **path**, and every ancestor of the focused box is on that path regardless.
None of the three quotes above consults `SupportsKeyboardFocus()`. Adding it would be live risk for
zero gain — `UUserWidget`'s own property comment says `bIsFocusable` *"is only set at construction and
is not modifiable at runtime"* (`UserWidget.h:1030`), and a focusable console competing with its own
`InputBox` is the one thing this widget cannot survive. **Flagging the divergence from the dispatch
text explicitly so QA rules on it rather than discovering it.**

### ⚠️ THE ONE REAL PRECONDITION, STATED SO IT IS NOT MISTAKEN FOR A BUG LATER

The tunnel only reaches this widget while **keyboard focus is somewhere inside it**. `OpenConsole()` →
`FocusInputBox()` puts it there and `ApplyInputBoxContract` item 3 (`ClearKeyboardFocusOnCommit=false`)
keeps it there across a submit. **If the player clicks the world and focus leaves the box, `Z` stops
accepting until the box is focused again** — the same condition that already governs typing at all.
This is inherent to the ruled mechanism, not a defect in it.

## 4. THE REFRESH STRATEGY I CHOSE — per-open refresh, live per-press resolve, NO delegate binding

- ✅ **`OpenConsole()` calls `RefreshKeyboardLayout()` ONCE per open**, placed **after** the re-open
  suppression guard (a refused open is not an open and must not pay for a probe). This is `KBD-§8`'s
  ruled *"the caller refreshes, the accessor reads"*, by the exact function the pin names.
- ⛔ **`OnKeyboardLayoutChanged` is NOT bound** — `KBD-§8` forbids a widget binding it, and it would be
  new lifetime state to unbind wrongly.
- ⭐ **The key is resolved LIVE on each qualifying press (`GetAcceptKey()`), never cached in a member.**
  That is the answer to *"what about a layout change mid-session"*: the subsystem's 1 Hz poll
  (`siege.Input.LayoutPollEnabled`, default ON) re-probes a `Win+Space` within ~1 s and the accept key
  follows it with **no binding and no cached `FKey` to go stale**. The per-open refresh is what keeps
  it correct **even when that poll has been switched off for testing**. Cost per press: one
  `GetWorld()->GetGameInstance()->GetSubsystem<>()` + one `TMap::Find`, and `&&` short-circuiting means
  it only runs for an unmodified press while a prompt is actually up.
- ⛔ **Fail-safe:** no world / no `GameInstance` / no subsystem ⇒ **plain `EKeys::Z`**. Never
  `EKeys::Invalid`, never "accept unavailable".
- ⛔ **Direction is the pinned one:** `InKeyEvent.GetKey() == GetPositionalKey(EKeys::Z)`, never a
  reverse lookup. On US-Dvorak that compares against `EKeys::Semicolon`.

## 5. THE PLAYER IS TOLD — and the string says `Z` on every layout

New chrome constant beside the existing ones (`.cpp` `SiegeAssistantConsole` namespace):

```cpp
static const TCHAR* ConfirmHintText = TEXT("Press Z to accept, or close this box to discard");
```

- It is rendered on **`StatusText`**, on its own line under the FSM state label, for **exactly as long
  as `bConfirmPromptVisible` is true** — i.e. on the identical lifecycle the two buttons had. No new
  widget, no new pinned child, no art.
- ⛔ **It is a LITERAL `Z` and is never built from `GetAcceptKey()`** (`KBD-§8` last bullet,
  `KBD-§0` ruling 1). Nothing in the diff can render `"Semicolon"` to a player.
- ✅ **`AS-§3` intact:** static chrome naming no order, unit, place or outcome. Every sentence with game
  meaning still arrives from the component's reason-code template table.
- **It names BOTH halves deliberately**: with both buttons gone there is no longer any control on
  screen that reads as "cancel", so the discard gesture would otherwise be undiscoverable.

**Mechanism:** `SetAssistantState` now stores `LastStateLabel` and both it and `ApplyConsoleVisualState`
call a single composer, `RefreshStatusLine()`. **One writer, because two would race** — an FSM state
push arriving while a prompt is up would otherwise erase the only thing telling the player how to accept.

## 6. THE TWO TASK-520 CONSEQUENCES — answered

### (1) "Make the discard legible at the moment of the gesture" — ⛔ **NOT DONE, AND I BELIEVE IT CANNOT BE DONE INSIDE THIS FILE'S FENCES**

`CloseConsole()` sets `SetVisibility(Collapsed)` before it broadcasts, so the FSM's `Cancelled` line is
pushed into an invisible widget and is read on the *next* open. Every fix inside this file requires
changing **`CloseConsole()`** — delaying the collapse, deferring the broadcast, or holding the panel up
for a beat. ⛔ **`AS-§6` A-2's amendment pins that function shut in as many words: *"the widget's
`CloseConsole()` stays byte-unchanged"*.** I did not touch it. ⇒ **This is a manager call.**

✅ **What I did instead, which is within the fences and I think buys most of it:** the discard is now
**announced in advance** rather than confirmed in arrears — the hint on screen for the whole life of the
prompt says *"or close this box to discard"*. The player knows what the gesture does before they make
it. **That is prediction, not confirmation, and I am not claiming it is the same thing.**

### (2) ⛔⛔ **`OnConsoleCancelled` NOW HAS ZERO LIVE BROADCASTERS — ESCALATED, NOT FIXED**

Verified from the artifact: the only thing that broadcasts it is `USiegeAssistantConsoleWidget::CancelPressed()`,
and after this task **nothing calls `CancelPressed()`**. The delegate, the widget function, the component's
binding and the component's `CancelPressed()` all survive as public API (ruled), but:

> ⇒ **A PLAYER CANNOT CANCEL A LATCHED `Deferred` ORDER FROM THE CONSOLE AT ALL.** The FSM's
> `Deferred`-drop and `Thinking`-abort branches behind `OnConsoleCancelled` are now **unreachable from
> the console**.

⛔ **I did not fix this — it is out of scope and lands in `SiegeAssistantComponent.{h,cpp}`, which
TASK-522 owns.** ⚠️ **It needs a decision BEFORE Jonathan's playtest**: a deferred order that fires up
to 120 s later, on a board he has not looked at since, currently has no console-reachable cancel. It is
recorded on the task board's status line, in this widget's class comment §4, at `CancelPressed()`'s
declaration, and by TASK-520 at the component's binding site.

## 7. ⚠️⚠️ A CONFLICT BETWEEN THE DISPATCH PROMPT AND THE FILES — I FOLLOWED THE FILES

**The dispatch prompt said:** *"`CancelButton` is struck from the pinned child-widget list, **not
deleted from the world** — the UPROPERTY stays as `BindWidgetOptional` … It is simply no longer
constructed."*

**CONVENTIONS `AS-§6` RULING A's amendment says the opposite, in as many words:**
> *"⇒ **The `CancelButton` member, its `HandleCancelClicked` thunk and its `CancelLabelText` all go.**
> ⛔ **A future `WBP_AssistantConsole` MUST NOT re-introduce a widget named `CancelButton` expecting it
> to bind** — the C++ no longer declares it, so a WBP child of that name would bind to nothing and look
> wired. **The strikethrough stays instead of a deletion precisely so the next author meets the
> retirement rather than an absence.**"

**And the TASKBOARD spec clause (1) agrees:** *"Delete the `CancelButton` member, its
`HandleCancelClicked` thunk, its `CancelLabelText`, its construction, its binding and its visibility
handling."*

⇒ **I DELETED THE MEMBER.** Reasoning: (a) two written authorities beat a paraphrase, and files are the
contract; (b) the "retirement rather than an absence" the law asks for is **the strikethrough in the
CONVENTIONS pin**, not a surviving UPROPERTY; (c) a `BindWidgetOptional` that nothing constructs,
nothing binds (the thunk is gone) and nothing shows is *precisely* the "binds to nothing and looks
wired" state the law forbids. ✅ **I honoured the dispatch's intent at zero cost anyway: a
`//~ RETIRED 2026-08-04` block stands exactly where the member was declared**, naming the retirement and
the WBP prohibition, so a future author meets the retirement **in the code** as well as in the law.

⚠️ **If the manager prefers the dispatch reading, restoring the UPROPERTY is a three-line edit — but it
would then need `AS-§6` ruling A re-amended, because as written the law forbids it.**

## 8. DECLARED DEPARTURES / ADDITIONS FOR QA TO RULE ON (`SC-§15` — flagged, not slipped in)

**(a) A modifier guard the spec did not ask for.** `Ctrl`/`Alt`/`Cmd` presses are NOT treated as the
accept key. **This makes the grab strictly NARROWER, which is what `AS-§20.5` demands, and it closes a
real defect:** `Ctrl+Z` / `Ctrl+Shift+Z` are the text box's own **undo and redo** —
`FSlateEditableTextLayout::HandleKeyDown:1168-1176` reads
`(Key == EKeys::Z && InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown())` for redo, beside the undo
binding. Consuming those would kill undo in the box **and** execute an order the player never asked to
execute. `Shift` is deliberately allowed.

**(b) `bConsoleEnabled` is in the accept gate.** Not named in the spec's two-term gate, but it is a
faithful port of the removed button's `SetIsEnabled(bConsoleEnabled)`. Belt-and-braces:
`SetConsoleEnabled(false)` already lowers the prompt.

**(c) One new private member, `FString LastStateLabel`,** plus `RefreshStatusLine()` /
`ResolveKeyboardLayoutSubsystem()` / `GetAcceptKey()`. Needed because the status line now carries two
things and two writers would race. **No new UPROPERTY, no new pinned child, no new BIE, no new
delegate.**

**(d) Two small behaviour changes that fall out of the single status-line writer, called out rather
than slipped in:**
 - `SetAssistantState` no longer overwrites `StatusText` while the console is **disabled** — the fault
   reason now survives an FSM state push. (Previously the reason was clobbered.)
 - `SetConsoleEnabled(true)` now **restores** the last state label over the stale fault reason.
   (Previously the disabled reason stayed on screen until the FSM happened to push another state.)
 Both are strict improvements; both are documented at their sites. ⛔ **The fault latch's own write is
 untouched and still wins while disabled** — `RefreshStatusLine()` returns early on `!bConsoleEnabled`,
 and that is safe **by order**, since `SetConsoleEnabled` assigns the flag before it closes or writes.

**(e) `Components/HorizontalBox.h` + `HorizontalBoxSlot.h` includes and `ButtonLabelSize` deleted** —
nothing references them any more and this module builds warnings-as-errors.

## 9. THE TASK-444 / TASK-505 FENCES — ALL VERIFIED UNDISTURBED

| fence | state |
|---|---|
| tree built **before** `Super::RebuildWidget()` | ⛔ untouched (`ConstructConsoleTree(); return Super::RebuildWidget();`) |
| children wired in **`NativeConstruct`**, never `NativeOnInitialized` | ⛔ untouched |
| `ToggleConsole()` stays uncalled | ⛔ untouched |
| **Slate focus only, never `SetInputMode`** | ⛔ untouched — no `SetInputMode` anywhere in the diff |
| **CLOSE ROUTE 4** (`Enter` on an empty box) + `ReopenSuppressionSeconds` + `LastRoute4CloseRealTimeSeconds` | ⛔ untouched, byte-for-byte |
| `CloseConsole()` **byte-unchanged** (`AS-§6` A-2) | ✅ verified — not one character |
| `ConfirmPressed()` / `CancelPressed()` bodies | ✅ byte-unchanged |
| `HandleTextCommitted` | ✅ byte-unchanged |
| ruling A(b) escape hatch | ✅ intact and now **load-bearing**: `ConfirmButton` is still `BindWidgetOptional`, still bound in `WireChildWidgets`, still shown/hidden in `ApplyConsoleVisualState` — a WBP wins with zero C++ change |

## 10. ⚠️ TASK-527's CHECK — THREE LINES, AND I CANNOT CLOSE IT (`AS-§6` ruling A(e))

⛔ **Nothing on screen is verified by me. Report nothing here as passing.** Look at:

1. **With a confirm prompt up:** the status line reads **"Press Z to accept, or close this box to discard"** under the state label, and **NO Accept or Cancel button is anywhere on the console**.
2. **Press `Z`:** the order executes and the hint line disappears — and the same press does **not** also leave a `z` in the input box.
3. **With no prompt up, type `zzz` in the box:** three `z`s appear normally; and pressing **`Escape`** at any time still cancels a placement / spell target exactly as it does with the console closed, and does **not** close the console.

## 11. WHAT QA SHOULD SCRUTINISE

1. ⛔ **`Escape`.** Grep the diff for `Escape` — it appears only in comments and never in a condition.
   Then confirm the fall-through returns `Super`, and that `Super` is Unhandled in v1 (§2 quotes prove it).
2. ⛔ **The narrowness of the gate.** All three terms present; `FReply::Handled()` returned on exactly
   one path; every other path reaches the `Super` return.
3. ⛔ **`Handled` only after an accept.** Read §1's pre-condition argument and decide whether you agree
   a post-check would be *worse* — I claim it would double-serve the key under re-entrancy.
4. ⛔ **The positional lookup direction** — `InKeyEvent.GetKey() == GetPositionalKey(EKeys::Z)`, never
   the reverse; and **no path renders the resolved key to the player**.
5. ⛔ **The `Z` literal in `ConfirmHintText`** is a literal, not a format of `GetAcceptKey()`.
6. ⛔ **`RefreshKeyboardLayout()` is called exactly once per open** and sits **after** the suppression
   guard; **`OnKeyboardLayoutChanged` is bound nowhere.**
7. ⛔ **§7's `CancelButton` conflict** — rule on it. It is the one place I chose the files over the prompt.
8. ⛔ **§8(a)'s modifier guard** — an addition, declared. Reject it if you think the spec's two-term gate
   was exhaustive; the code still works without it (worse, in my view).
9. **§8(d)'s two status-line behaviour changes** — confirm the fault reason still wins while disabled.
10. **No `Slot`-named local was introduced** (the inherited-member C4458 trap this file documents);
    no `TObjectPtr`/`TSubclassOf` conditional was introduced (the C2445 trap it documents twice).
11. **`GetSubsystem<>()` on a `const UGameInstance*` is legal** — `GameInstance.h:439-440` declares it
    `const`, and `HeroCharacter.cpp:267-269` is the shipped in-repo precedent for this exact hop.
