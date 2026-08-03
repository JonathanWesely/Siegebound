# TASK-411 — [LLM-1b] SPIKE MEASUREMENT #6: does a focused `UEditableTextBox` starve Enhanced Input of WASD?

- **Owner:** gameplay-programmer
- **Date:** 2026-08-02
- **Status set to:** `ready-for-qa` (gate is TASK-412)
- **Files (BOTH NEW, nothing else touched):**
  - `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantInputProbe.h`
  - `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantInputProbe.cpp`

**⚖️ M8 DECLARATION DUTY (verbatim):** this task adds no replicated property, no new replicated class, no new relevancy tier.

---

## 1. THE ANSWER

### **TRUE — a focused `UEditableTextBox` DOES starve Enhanced Input of WASD in UE 5.8.**
### **Recommendation for Wave 1 B3: ship the console on `FInputModeGameAndUI` + `SetKeyboardFocus()`. The camera stays live.**

⚠️ **READ THE NEXT SECTION BEFORE ACTING ON THAT.** The answer is derived by **reading UE 5.8's shipped Slate source**, not by watching a hero in PIE. The distinction is load-bearing on this project and I am not rounding it up.

### The mechanism, quoted

`Engine/Source/Runtime/Slate/Private/Widgets/Text/SlateEditableTextLayout.cpp:1218`, the final branch of `FSlateEditableTextLayout::HandleKeyDown`:

```cpp
else if (!InKeyEvent.IsAltDown() && !InKeyEvent.IsControlDown() && InKeyEvent.GetKey() != EKeys::Tab && InKeyEvent.GetCharacter() != 0)
{
    // Shift and a character was pressed or a single character was pressed.  We will type something in an upcoming OnKeyChar event.
    // Absorb this event so it is not bubbled and handled by other widgets that could have something bound to the key press.
    Reply = FReply::Handled();
}
```

The engine's own comment states the intent. `W`/`A`/`S`/`D`/space are printable, carry no Alt/Ctrl, are not Tab, and arrive with a non-zero character code, so `HandleKeyDown` returns **Handled**. `SEditableText::OnKeyDown` (`SEditableText.cpp:274`) then never reaches `SWidget::OnKeyDown`, so the event **never bubbles to `SViewport`**, so `UGameViewportClient::InputKey` is never called, so `UPlayerInput`/Enhanced Input never sees the key and `IA_Move` cannot fire.

The `GetCharacter() != 0` condition holds for real hardware: `FWindowsApplication::ProcessDeferredMessage` computes `CharCode = ::MapVirtualKey(Win32Key, MAPVK_VK_TO_CHAR)` and passes it into `OnKeyDown` (`WindowsApplication.cpp:3317`/`:3321`). For `W` that is `0x57`.

Focus reaches the right widget: `UWidget::SetKeyboardFocus()` focuses the `SEditableTextBox`, whose `OnFocusReceived` (`SEditableTextBox.cpp:309`) re-targets focus to its inner `SEditableText`, and `SEditableText::SupportsKeyboardFocus()` returns `true` unconditionally (`SEditableText.cpp:240`).

### What is still OWED, and by whom

I did **not** run this. I cannot: the code is not compiled (that is build-master's lane), and the editor is Jonathan's. **The measured confirmation is TASK-413's**, which the board already assigns ("measurement #6 from TASK-411's probe (`Siege.Assistant.InputProbe`, both modes) run in the same session"). This handoff delivers the instrument plus a source-derived prediction; **the number that goes in the spike report must come from the instrument.**

If the run contradicts this section, **the run wins** and B3's input mode flips to the `UIOnly` fallback. I do not expect that, but the whole point of measurement #6 is that the expectation is not evidence.

---

## 2. What the probe does — and why it runs THREE passes, not two

`Siege.Assistant.InputProbe` (PIE only) creates the throwaway widget, types `wasd send footmen` into it, and reports per pass.

| # | Pass | Posture | Focus | Expected |
|---|------|---------|-------|----------|
| 1 | **CONTROL** | `GameAndUI` | none (viewport) | hero **MOVES** |
| 2 | **MODE A** | `GameAndUI` | `InputBox` | hero does **not** move → PASS |
| 3 | **MODE B** | `UIOnly` | `InputBox` | hero does **not** move → PASS |

**The control pass is the point.** A two-pass probe would report a confident PASS for both modes if the synthetic keystrokes never reached the input stack at all — a zero delta that means nothing. Pass 1 must show movement; if it does not, passes 2 and 3 are stamped **INCONCLUSIVE**, never PASS. Two further self-checks guard the same shape:

- **focus is re-verified every sampled frame** — a pass that silently lost focus is INCONCLUSIVE, not PASS;
- **the typed text is read back out of the box** — if the sentence did not land, nothing was actually typed, so the pass is INCONCLUSIVE.

Per pass it logs: start/end `GetActorLocation()`, **net delta**, **path length** (see below), max speed, max `GetLastInputVector()` magnitude ("did the move action fire"), whether W/A/S/D reached `UPlayerInput`, focus-held frames, the text read-back, and the Escape / Enter / RMB observations. `Siege.Assistant.InputProbeReport` re-prints the table from file-static storage.

**Why path length as well as net delta:** the criterion sentence contains both `w` and `s` and both `a` and `d`, so a hero that walked a full square could land near its start. Net delta is reported because the spec asks for it; **path length is the primary movement evidence because it cannot cancel.**

### How keystrokes are injected
`FSlateApplication::ProcessKeyDownEvent` / `ProcessKeyCharEvent` — the same entry point `FWindowsApplication` calls after the platform message pump. The focus path, the Slate bubble, `SViewport`, `UGameViewportClient::InputKey` and Enhanced Input are all the real shipping ones; only the OS message pump is bypassed, and that is not what is under test. It is **not** `SendInput`, so it works on a locked desktop (the standing TASK-076/112 constraint).

⚠️ **The one trap in the injection, named so nobody re-introduces it:** the absorb branch keys off `GetCharacter() != 0`. A synthetic `FKeyEvent` built with `CharacterCode = 0` would **not** be absorbed and the probe would report a **false FAIL**. The code passes the real `MapVirtualKey` value (uppercase for letters, `0x20` for space) on the key event and the real shift-adjusted char on the follow-up `FCharacterEvent`, exactly as Windows does.

---

## 3. Four side findings B3 needs, all from the same source read

1. **Enter is absorbed and CLEARS FOCUS.** `HandleKeyDown` handles `EKeys::Enter` at `:1092` → `HandleCarriageReturn` → Handled, so Enter never reaches the game. But `UEditableTextBox` defaults `ClearKeyboardFocusOnCommit = true` (`EditableTextBox.cpp:34`), so committing **drops keyboard focus**. B3 must decide explicitly: re-focus for the next order, or close the console on submit. Silently losing focus after the first sentence would read as a bug.

2. **⚠️ Escape is CONDITIONAL, and under `GameAndUI` it usually reaches the game.** `HandleEscape()` (`:1448`) returns `true` only if there is active search text, a selection, or `RevertTextOnEscape && text changed`. `UEditableTextBox` defaults `RevertTextOnEscape = false` (`EditableTextBox.cpp:33`), so **Escape on a plain focused box returns Unhandled and bubbles to the viewport** — the shipped `WasInputKeyJustPressed(EKeys::Escape)` cancel paths in `ASiegePlayerController` (lines 458 / 490 / 518) **still fire while the console is open.** Good for teardown; it also means Escape can cancel a placement/targeting/group-pick behind the console. That is precisely what B1's four mutual-exclusion guards are for. B3 must not assume Escape is consumed.

3. **`KeyUp` always bubbles** — `HandleKeyUp` (`:1244`) returns Unhandled on desktop. So there is **no stuck-key hazard**: a `W` held before the console opens still gets its release. Worth stating because "hold a key, open the console, release it, hero runs forever" is the obvious thing to fear here, and it does not happen.

4. **The `UIOnly` fallback costs more than the camera.** `FInputModeUIOnly::ApplyInputMode` calls `GameViewportClient.SetIgnoreInput(true)` (`PlayerController.cpp:6384`), whereas `FInputModeGameAndUI` calls `SetIgnoreInput(false)` (`:6410`). So `UIOnly` freezes mouse-look **and kills the shipped Escape/RMB cancel routes and every keyboard command** for as long as the console has focus — not just the camera. **If the measured answer ever comes back FALSE, that is the real bill**, and it should be Jonathan's call, not a silent fallback.

---

## 4. Ruling-A rehearsal — what was surprising

The spec asked me to report anything surprising about the code-authored-tree pattern. Three things, and **the first is a real trap B3 would otherwise hit:**

1. **⚠️ THE TREE MUST BE BUILT *BEFORE* `Super::RebuildWidget()`, NOT AFTER.** `UUserWidget::RebuildWidget()` reads `WidgetTree->RootWidget` **as it stands at the moment it is called** and returns an `SSpacer` when it is null. The natural-looking `Super::RebuildWidget(); /* then build */ return Result;` yields a **silently EMPTY widget** — and it still passes every property/tree readback, because the `UWidget` objects all exist and are correctly parented. That is exactly the failure class this project's UMG verification law exists for (ruling A(e): correctness closes on rendered pixels, never on a readback). The probe builds the tree first and comments the reason inline.

2. **The escape hatch is one clean early-return, not per-widget null checks.** `ConstructProbeTree()` returns immediately when `WidgetTree->RootWidget != nullptr` — an asset-authored tree wins whole, and the `BindWidgetOptional` members UMG already resolved are never overwritten. Zero C++ change to take the fallback, as ruling A(b)/(c) promises.

3. **Two defaults the pattern quietly depends on**, neither obvious from its name:
   - `UUserWidget` already defaults `Visibility` to `SelfHitTestInvisible`, so the widget does not block the viewport's mouse. **But the code-authored root container does not** — `UVerticalBox` defaults to hit-testable `Visible` and, filling the screen, would swallow every click. The probe sets it to `SelfHitTestInvisible` explicitly. B3's `RootPanel` needs the same line.
   - `EWidgetTickFrequency::Auto` ticks a widget "if the widget inherits from something other than UserWidget ... so that native C++ or inherited ticks function". A C++ subclass **does** get `NativeTick`; a Blueprint-only one would not.

---

## 5. What QA should scrutinise

- **The honesty boundary in §1** — this is a *source-derived* answer with the measurement explicitly owed to TASK-413. If that reads as a PASS-by-assertion anywhere, fail it.
- **`CharacterCode` fidelity in the injected `FKeyEvent`** (`InjectKeyDown`). Zero there = false FAIL. This is the single highest-leverage line in the file.
- **The control-pass gating in `LeavePass()`** — verify that a `CONTROL-FAILED` really does force INCONCLUSIVE for modes A and B, and that no path can print PASS without it.
- **`FocusHeldSamples` is scored against `FocusWindowSamples`, not `SampledFrames`.** I got this wrong on the first pass and fixed it: the synthetic Enter clears focus, so ~13% of the pass is legitimately unfocused and scoring focus over the whole pass would have reported a **false INCONCLUSIVE on a good PASS**. Worth a second pair of eyes.
- **Teardown:** `EndProbe()` releases any held key/button, restores `bShowMouseCursor` / `bEnableClickEvents` and the matching input mode, and defers `RemoveFromParent()` to the next world tick (it is reached from inside `NativeTick`, i.e. while Slate is walking this widget's tree).
- **The RMB pass** uses `ProcessMouseButtonDownEvent(TSharedPtr<FGenericWindow>(), ...)`. The null platform window is guarded engine-side (`SetCapture` is `IsValid()`-gated). Down/up are always paired, including on abort.
- **Ownership:** `git status --porcelain` on `SiegePlayerController.{h,cpp}`, `SummonedUnit.cpp`, `MinerUnit.cpp`, `SiegeCheatManager.cpp`, `GitClaudeUnrealTest.Build.cs` and `GitClaudeUnrealTest.uproject` returns **zero lines**. `ApplyCursorInputState()` was **read** for the restore posture and never called or edited.

## 6. Compliance notes

- **CONVENTIONS §5:** registered as `FAutoConsoleCommandWithWorld` in a new file, namespace `Siege.Assistant.*`. **No `UFUNCTION(exec)` added to any shipped class.**
- **No `Build.cs` change.** `UMG`, `Slate`, `InputCore` and `EnhancedInput` are already public deps; `Slate` publicly depends on `SlateCore`, so `Framework/Application/SlateApplication.h` resolves. This is the first `FSlateApplication` use in the game module — flagging it so QA can confirm the dep chain rather than take my word.
- **`LogSiegeAssistant`** is *included* from TASK-417's `SiegeAssistantCommand.h`, never redeclared (pinned-registry ruling 8).
- No `Content/` change, no `L_Arena` save, no Git, no compile, no PIE, no editor contact. The plugin, the model and every assistant file were untouched (zero llama dependency, as specced).
- **Throwaway by design:** Wave 1 B3 **deletes both files**.
