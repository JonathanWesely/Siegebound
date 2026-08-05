# TASK-520 — [AX-5] A close that discards an order must SAY SO — the `Cancelled` transcript line

**Agent:** gameplay-programmer · **Date:** 2026-08-04 · **Status:** ready-for-qa · **QA gate:** TASK-525
**Law applied:** CONVENTIONS `AS-§6` RULING A-2 (the 2026-08-04 close-is-a-cancel amendment) · `AS-§3` (templates only) · `AS-§20.5`

> **M8 DECLARATION, verbatim:** *"adds no replicated property, no new replicated class, no new relevancy tier."*

---

## 1. What changed — the whole of it

Two files, **insert-only** (the single deleted line in the `.h` is one doc line that was rewritten in place). No function was renamed, no signature changed, no behaviour removed.

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp` | `NotifyConsoleClosed()` — sample `bDiscardedPendingOrder` before the discard; push the existing `Cancelled` template after it. `AttachConsoleWidget()` — a do-not-clean-me note at the `OnConsoleCancelled` binding site (comment only). |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` | doc only — `NotifyConsoleClosed()`, `CancelPressed()`, the FSM state table, the delegate wiring table. |

⛔ **Nothing else was touched.** No widget edit (TASK-519), no grammar/parser (TASK-518), no snapshot (TASK-517), no selector/executor and **no `ExcludeKinds` work** (TASK-522 — the file pair is now RELEASED to it). No test (TASK-523). No compile, no Git, no editor/MCP/PIE.

### The functional diff (`SiegeAssistantComponent.cpp`, `NotifyConsoleClosed`)

Comments elided here for readability; they ship in full in the file.

```cpp
 void USiegeAssistantComponent::NotifyConsoleClosed()
 {
 	bConsoleOpen = false;

 	if (State == ESiegeAssistantState::Deferred)
 	{
 		UE_LOG(LogSiegeAssistant, Verbose, TEXT("Console closed with a deferred intent latched - the latch is PRESERVED."));
 		return;
 	}

+	const bool bDiscardedPendingOrder = (State == ESiegeAssistantState::AwaitConfirm);   // ← sampled BEFORE the discard
+
 	switch (State)
 	{
 	case ESiegeAssistantState::Thinking:
 		AbortInFlightRequest();
 		break;

 	case ESiegeAssistantState::AwaitConfirm:
 		ClearConfirmPreview();
 		break;

 	default:
 		break;
 	}

 	ClearPendingIntent();
 	SetState(ESiegeAssistantState::Idle);
+
+	if (bDiscardedPendingOrder)
+	{
+		PushMessage(ESiegeAssistantReasonCode::Cancelled);   // ← the EXISTING template, byte-for-byte CancelPressed()'s call
+	}
 }
```

That is the entire behavioural delta of this task. **Three lines of code.**

---

## 2. The four things the spec told me to get right, and where each one is

**(a) The existing template, never a new string.** The push is `PushMessage(ESiegeAssistantReasonCode::Cancelled)` — the **no-args overload**, character-for-character the call `CancelPressed()` already makes at `SiegeAssistantComponent.cpp:882`. The row it resolves to is untouched:

```cpp
case ESiegeAssistantReasonCode::Cancelled:
{
    static const FText T = NSLOCTEXT("Siegebound", "Assistant_Cancelled", "Order cancelled.");
    ...
```

⛔ **No `NSLOCTEXT` was added, edited or moved. No literal was authored at the call site.** The reason-code table (`SiegeAssistantReasonTemplate`, `.cpp:329`) is byte-unchanged, so `AS-§3` is satisfied structurally rather than by claim.

**(b) Sampled BEFORE, pushed AFTER.** `bDiscardedPendingOrder` is read at the top because `ClearPendingIntent()` + `SetState(Idle)` destroy the evidence — after them the answer is always "nothing happened". The **push** is last, after preview-down → pending-cleared → state-set, so the player is told the order is gone only once it is. The shipped ordering (`ClearConfirmPreview` → `ClearPendingIntent` → `SetState`) is **unchanged**; the push is appended, not interleaved.

**(c) The widget broadcasts nothing new.** `SiegeAssistantConsoleWidget.{h,cpp}` was **not opened for editing**. `CloseConsole()` still broadcasts only `OnConsoleOpenChanged(false)`; `HandleConsoleOpenChanged` still translates arity and no policy; `NotifyConsoleClosed` is still the only thing that turns a close into a discard. The half `AS-§6` A-2 protects is intact.

**(d) `CancelPressed()` survives.** Signature, `UFUNCTION(BlueprintCallable)`, body and behaviour all unchanged — **only its doc comment gained the "deliberately uncalled" note** the route-2 amendment requires.

---

## 3. ⭐ WHEN THE LINE FIRES AND WHEN IT STAYS SILENT (read this as the test matrix)

`NotifyConsoleClosed()` has exactly one live caller: `HandleConsoleOpenChanged(false)`, from `OnConsoleOpenChanged.Broadcast(false)` in `USiegeAssistantConsoleWidget::CloseConsole()`. It is also `BlueprintCallable`, so BP can call it directly.

| state at close | preview | pending intent | state after | **`Cancelled` line?** |
|---|---|---|---|---|
| **`AwaitConfirm`** | cleared | cleared | `Idle` | ✅ **YES — the only firing case** |
| `Deferred` | untouched | **PRESERVED** | `Deferred` (early return) | ⛔ no — nothing was discarded |
| `Thinking` | n/a | cleared | `Idle` | ⛔ no — see the note below |
| `Composing` | n/a | none | `Idle` | ⛔ no |
| `Idle` | n/a | none | `Idle` (no-op `SetState`) | ⛔ no |
| `Clarify` | n/a | cleared (the held question) | `Idle` | ⛔ no |
| `Failed` | n/a | already cleared | `Idle` | ⛔ no |

**Route coverage** — every close route in `AS-§6` A-2 reaches the same function, so the matrix above is complete for all of them:

1. **Open key pressed while open** (controller toggle) → `CloseConsole()` → fires **iff** `AwaitConfirm`.
2. **`CancelPressed()` with no prompt up** (widget) → `CloseConsole()` → by construction the state is not `AwaitConfirm`, so **silent**. ✅ Correct: that route is a dismiss, not a discard.
3. **`SetConsoleEnabled(false)`** (the fault latch) → `CloseConsole()` → **silent, and verified rather than assumed**: `MarkAssistantFaulted` (`.cpp:1018`) calls `ClearConfirmPreview()` + `ClearPendingIntent()` + `SetState(Failed)` **before** it broadcasts `OnAssistantAvailabilityChanged`, so by the time the widget closes the state is `Failed`, not `AwaitConfirm`. ⛔ **No double message** — the player gets the fault sentence, not a spurious "Order cancelled." on top of it.
4. **`Enter` on an empty box** (route 4) → `CloseConsole()` → fires **iff** `AwaitConfirm`. ⚠️ QA: this is a real and intended firing path — a player with a confirm prompt up who hits `Enter` on an empty box gets the discard *and* now gets told.

**`EndPlay` does NOT fire it.** `EndPlay` calls `ClearConfirmPreview` / `ClearPendingIntent` / `DetachConsoleWidget` directly and never routes through `NotifyConsoleClosed`, so a level teardown pushes no transcript line into a dying component.

**Repeat closes are safe.** A second `NotifyConsoleClosed()` sees `Idle` and is silent; `CloseConsole()` itself early-returns when already closed, so the delegate does not even re-broadcast.

### ⚠️ The one asymmetry with `CancelPressed()`, stated plainly rather than left for QA to find
`CancelPressed()` **does** print `Cancelled` when it aborts a `Thinking` turn. `NotifyConsoleClosed()` **does not**. That is deliberate and it is what `AS-§6` A-2 says — *"pushed on the AwaitConfirm→Idle close path ONLY"*. My reading of why it is right: in `Thinking` the model has not answered, so **there is no order to lose** — nothing was on screen, nothing was previewed, nothing is being taken away. In `AwaitConfirm` there is a described order and ghost circles on the ground, and that is the thing whose silent disappearance reads as *"the assistant ate my order"*. ⚠️ **If the manager wants the `Thinking` abort narrated too, that is a one-word change to the boolean — but it is a scope change and I did not take it.**

---

## 4. ⛔ ANSWERING SPEC ITEM (5): DOES `OnConsoleCancelled` HAVE ANY LIVE CALLER?

**Read from the artifact, not assumed.**

- **Today (before TASK-519 lands):** ✅ yes, one. `USiegeAssistantConsoleWidget::CancelPressed()` broadcasts it, and `HandleCancelClicked()` (the `CancelButton` `OnClicked` thunk) calls `CancelPressed()`.
- **After TASK-519 lands** (it deletes `CancelButton`, `HandleCancelClicked` and `CancelLabelText`): ⛔ **NO. `OnConsoleCancelled` will have ZERO live broadcasters in the shipped v1 tree.** The widget's `CancelPressed()` survives as `BlueprintCallable` public API with no caller, so the delegate is reachable **only** from Blueprint, a future `WBP_AssistantConsole`, a gamepad path or an accessibility path.
- **Consequence, named:** the component's `CancelPressed()` — and specifically its **`Thinking`-abort** and **`Deferred`-drop** branches — become **unreachable from the console**. ⛔ **They were NOT deleted.** The `Deferred` drop in particular now has *no* console route at all: `NotifyConsoleClosed` deliberately **preserves** a latched deferred intent, and the only other way to drop one is `SubmitUtterance`'s Deferred-admission path (which pushes `DeferredCancelled`). **A v1 player cannot cancel a latched deferred order from the console.** That is a pre-existing consequence of removing the button, not something this task introduced — but it is now true, and it is written here so nobody discovers it in a playtest.
- Said at the binding site (`AttachConsoleWidget`, `.cpp:~2354`) and in both header doc blocks, so the next reader meets the retirement rather than an absence.

---

## 5. ⚠️ WHAT QA SHOULD SCRUTINISE

1. **`AwaitConfirm` ⟺ "something to discard".** I used the **state alone** as the test, with no secondary `PendingArgs.Command.Intent != None` guard. Justification: `EnterAwaitConfirm()` is the sole entry to that state and is reached with `PendingArgs` already filled, so the two are the same statement — a second clause would be dead and would invite a future reader to think the state is unreliable. ⚠️ **If QA can name a path that reaches `AwaitConfirm` with an empty `PendingArgs`, this is wrong and I want to hear it.**
2. **⭐ THE LINE IS NOT VISIBLE AT THE MOMENT OF THE CLOSE, AND I COULD NOT MAKE IT SO WITHIN THIS TASK'S LAW.** `CloseConsole()` runs `ApplyConsoleVisualState()` (hiding the widget) **before** it broadcasts `OnConsoleOpenChanged(false)`, so when `PushMessage` fires the console is already hidden. The line lands in `USiegeAssistantConsoleWidget::TranscriptLines` (which is **not** cleared on close — `ClearTranscript()` has no caller) and is read **the next time the player opens the console**. It is also re-seeded via `LastMessage` if the widget is re-attached. ⛔ **The two ways to make it instantly visible are both forbidden here:** broadcasting from the widget (an explicit `AS-§6` A-2 QA FAIL) or adding a new on-screen surface (out of scope, and an art/UI decision I am not the one to make). ⚠️ **This is the single thing about this task I would escalate: the player still gets no feedback *at the instant* they close. Whether that satisfies Jonathan's intent is a product call, and I flag it rather than silently solve it.**
3. **Double-message hunting.** I traced the fault path (item 3 in §3 above) and found no double. Worth a second pair of eyes on any path that could reach `AwaitConfirm` and then close *without* going through `MarkAssistantFaulted`.
4. **`OnAssistantMessage` re-entrancy.** The push is last, so a handler that re-enters the component sees a fully settled `Idle`. Nothing in the shipped tree does this (the only bound handler is `ShowTranscriptLine`), but the ordering is what makes it safe rather than lucky.
5. **File release.** `SiegeAssistantComponent.{h,cpp}` is **released to TASK-522**. I stayed strictly inside `NotifyConsoleClosed` and one comment block in `AttachConsoleWidget`; I did **not** touch the selector, the executor, `ExecutePendingCommand`, or anything `ExcludeKinds`-shaped, so TASK-522 should merge cleanly.

## 6. Not done, by instruction
⛔ Not compiled (**TASK-526** owns the only compile) · ⛔ no tests (**TASK-523**) · ⛔ no Git · ⛔ no editor / MCP / PIE · ⛔ no `Content/` asset.
