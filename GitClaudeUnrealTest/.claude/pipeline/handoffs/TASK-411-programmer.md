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

---
---

# TASK-411 — REWORK (2026-08-02, after TASK-413's run)

- **Owner:** gameplay-programmer · **Status set to:** `ready-for-qa`
- **Files touched (BOTH, and nothing else):** `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantInputProbe.h` · `.cpp`
- **NOT COMPILED.** The quiet-module law is in force (a build-master is fetching a 2.5 GB model and will compile in one pass afterward). No build, no Git, no editor contact, no `Content/` change, no `L_Arena` save.

**⚖️ M8 DECLARATION DUTY (verbatim):** this task adds no replicated property, no new replicated class, no new relevancy tier. Stated verbatim in the header, as before.

---

## R1. THE HEADLINE: TASK-413's two failures are ONE root cause, and it is not the one that was diagnosed

TASK-413 reported two distinct obstacles — PIE self-terminating, and standalone `-game` CONTROL-FAILED "because `-ExecCmds` fires on frame 0". **The second attribution is wrong, and both symptoms come from a single line in my own file:**

```cpp
FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);   // the old CONTROL pass
```

`FSlateApplication::ProcessKeyDownEvent` routes a key event **only along `SlateUser->GetFocusPath()`** — tunnel then bubble, nothing else (`SlateApplication.cpp:5018-5066`, UE 5.8, read not recalled). With focus cleared that path is **empty**, so:

| consequence | evidence in TASK-413's own log |
|---|---|
| **(a)** no synthetic key reached any widget ⇒ never `SViewport` ⇒ `UGameViewportClient::InputKey` never called ⇒ `UPlayerInput` never saw W/A/S/D ⇒ the hero could not move | `-game` CONTROL: `PATH length 0.00`, `W/A/S/D on PlayerInput: no` |
| **(b)** the event fell through to `UnhandledKeyDownEventHandler` (`:5069`) → `FMainFrameActionCallbacks::OnUnhandledKeyDownEvent` (`MainFrameActions.cpp:275`) → `FPlayWorldCommands::GlobalPlayWorldActions`, where **`StopPlaySession` is bound to `FInputChord(EKeys::Escape)`** (`DebuggerCommands.cpp:358`) | PIE: probe abort + `BeginTearingDown` in the same frame, right after pass 1's Escape |

**⚠️ The frame-0 theory does not survive contact with the timeline.** `-ExecCmds` does fire on frame 0, and that IS a real hazard I have now gated — but the CONTROL pass types for ~4 s starting 0.35 s after `BeginProbe`, and `BeginProbe` already refused to run without a possessed pawn. Enhanced Input was live by then. The empty focus path explains the zero exactly; frame 0 does not.

**Two corrections to TASK-413's §6 that the re-run must not inherit:**

1. **"Passes 2 and 3 would have survived" is FALSE.** A plain `UEditableTextBox` does **not** absorb Escape: `FSlateEditableTextLayout::HandleKeyDown` routes Escape to `HandleEscape()` (`:1102`), which returns **false** when `RevertTextOnEscape == false` with no selection and no search text (`:1448-1476`) ⇒ **Unhandled** ⇒ it bubbles to `SViewport`, and `FSceneViewport::OnKeyDown` returns **Unhandled** whenever the viewport client does not consume the key (`SceneViewport.cpp:1288`) ⇒ on to `SGlobalPlayWorldActions`. **Escape ends PIE from ANY pass, focused or not.**
2. **`FInputModeGameAndUI` with no `WidgetToFocus` focuses NOTHING.** `FInputModeDataBase::SetFocusAndLocking` sets focus only when `InWidgetToFocus.IsValid()` (`PlayerController.cpp:6313-6318`); only `FInputModeGameOnly` focuses the viewport itself (`:6446`). My own old inline comment claimed the opposite. Corrected in the file.

---

## R2. What changed — how a pass now STARTS and ENDS, per environment

### How a run starts (both environments)

`Siege.Assistant.InputProbe` **arms**; it no longer measures. It registers an `FTSTicker` that waits for **hard requirements** — a game world, a first player controller, a **possessed pawn**, `PlayerInput != nullptr`, a valid **game viewport widget**, `FSlateApplication` initialised, at least 4 frames since arming — and then a **soft requirement**, a settled frame rate: `warmup` seconds of wall time (default **5 s**) plus **45 consecutive frames at or under 50 ms** (20 FPS). Budget **120 s**. This is the explicit guard for *"PIE on `L_Arena` sits at ~3 FPS for the first ~25 s"*.

- Budget expires with hard requirements met ⇒ **it starts anyway and says so, loudly**, and the report prints `warm-frame window ⚠️ NOT SATISFIED`. Refusing to run at all would be a different kind of unhelpful.
- Budget expires with hard requirements unmet ⇒ **ABORT naming the exact condition that failed** (`world=… controller=… pawn=… playerInput=… viewportWidget=… slate=…`), and `NOTHING WAS MEASURED`.
- World disappears after arming (PIE stopped) ⇒ abort immediately rather than idling a ticker for two minutes.
- Args: `warmup=<s> budget=<s> warmframes=<n> controlattempts=<n> force escape noescape`. An unrecognised token is **reported**, not silently ignored. New `Siege.Assistant.InputProbeCancel` drops an armed run.

**On top of that, CONTROL is itself the readiness test.** A failed control is retried up to **4 times** (1.5 s apart, 60 s budget), and **every failed attempt is printed and carried into the banked row** (`earlier attempts : #1 path=0.00cm viewportFocus=NO wasd=no;`). Retrying a *positive proof* is legitimate — it can only ever be satisfied by the hero actually moving — but it is never hidden.

### How a pass ends

| | PIE | standalone `-game` |
|---|---|---|
| typing window | 17 keys, unchanged | 17 keys, unchanged |
| **Escape** | **NOT INJECTED** (slot kept for timing parity; reported `NOT INJECTED — PIE world — EKeys::Escape is the editor's StopPlaySession chord…`) | **injected** |
| Enter | injected | injected |
| RMB | injected | injected |
| settle | 0.35 s, then the row is banked | same |

So in PIE a pass ends on **Enter → RMB → settle**, and nothing in that sequence can end the session. In `-game` the full matrix runs. `escape` forces injection in PIE with a loud warning that the session will end; `noescape` bans it anywhere.

### Rows are banked and logged one at a time

Each row is `UE_LOG`'d the instant it exists (`BANKED row 2/3 — MODE A | PASS | path=0.00cm …`). `LogReport` prints any rows it has, under a **PARTIAL RUN** banner when fewer than three were banked, instead of the old `"No probe has run in this session."` — which is what TASK-413 saw after losing a run whose rows only ever existed in memory.

---

## R3. The one result TASK-413 established — preserved, and made harder to misread

**MODE B (`FInputModeUIOnly`) swallows RMB and Enter; MODE A (`GameAndUI`) fires both.** Still measured in every environment (neither Enter nor RMB is ever suppressed), and the mechanism is now cited in the report footer: `FInputModeUIOnly::ApplyInputMode` calls `SetIgnoreInput(true)` (`PlayerController.cpp:6384`) where `FInputModeGameAndUI` calls `SetIgnoreInput(false)` (`:6410`). The cost the design has to price in — RMB cancels group picks, Enter is the console's own open key — is spelled out in the printed footer, not left to the reader.

Two additions that make that row trustworthy rather than merely present:

- **focus-at-press is recorded per observation.** Escape is *not* absorbed by the box, so it can bubble into the game and change the posture before Enter and RMB are observed. If the box had already lost focus at the press, the report says `this observation is contaminated, the box had already lost focus` instead of quietly reporting a routing result.
- **the OS cursor position at the RMB press is printed.** RMB travels the **pointer** path, not the focus path, so it only means anything if the cursor was over the game viewport.

⚠️ **Honest caveat on the A/B comparison, which I am not able to close from a file:** in TASK-413's `-game` log, MODE A reported `Enter -> PlayerInput: YES` — but `FSlateEditableTextLayout::HandleKeyDown` **handles** Enter for a non-read-only box (`:1092-1097`), so a genuinely focused box should have absorbed it. The most likely explanation is that Escape (injected immediately before) bubbled into the game and cost the box its focus. **That is exactly what the new focus-at-press field will show**, and it is the first thing to read on the re-run.

---

## R4. What would make me distrust my own result

Named deliberately, because the failure mode this batch keeps hitting is a confident wrong green.

1. **`CONTROL-OK` while `viewport focus : ⛔ NOT ESTABLISHED`.** Impossible by construction — if it ever prints, the focus check and the movement check disagree and neither can be trusted.
2. **`CONTROL-OK` with `PATH length` in the low single-digit cm.** `MoveEpsilonCm` is 5 cm and `bMoved` also trips on `bMoveKeyReachedPlayerInput` alone. A control that "passes" only via the key-state bool with a near-zero path proves the key reached `UPlayerInput` but **not** that `IA_Move` is bound — read `max move-input mag` before believing it.
3. **MODE A `PASS` while `focus held (typing)` is well under its window** — the 90 % gate should have stamped it INCONCLUSIVE. If PASS and a low focus ratio ever coexist, the gate is broken.
4. **A control that only passes on attempt 3 or 4 while the earlier attempts show `viewportFocus=yes`.** Then something is *intermittently* eating the input and the whole run is measuring a moving target — not a warm-up artefact.
5. **`pass timing` mean above ~50 ms.** The probe prints a `BELOW 20 FPS` warning per pass. At ~3 FPS a 0.15 s hold becomes one ~330 ms frame and the pass no longer resembles human typing; treat every row from such a pass as indicative only.
6. **MODE A and MODE B both PASS with identical everything.** Expected, but it is also exactly what a dead injection path looks like — which is why rule 1 (the control) exists. If the control is anything other than a clean `CONTROL-OK`, **the answer is INCONCLUSIVE and must be reported as such.**
7. **Any row where an observation reads `NOT INJECTED`, quoted as a swallow.** It is not one. In PIE the Escape row is *never* evidence; the Escape matrix is a standalone `-game` measurement.
8. **A `-game` run whose CONTROL passes but whose MODE A focus ratio is 100 % and text read-back is empty.** Contradictory: focus held but nothing landed means the character events are not reaching the box, and `bTypedTextLanded` should already have forced INCONCLUSIVE.

**And the structural one:** the probe still measures with `FSlateApplication::ProcessKeyDownEvent` rather than the OS message pump. That is deliberate and unchanged (it works on a locked desktop, TASK-076/112), but it means **the one thing this probe can never test is the platform layer itself.** If the shipped console ever behaves differently from this measurement, the message pump is the first suspect.

---

## R5. What QA should scrutinise in the rework

- **`IsGameViewportFocused()`** — the **box exclusion** is the load-bearing half. The probe's widget is a *descendant* of the game viewport widget, so a naive ancestor walk would report "viewport focused" while the text box holds focus and turn a focused pass into a fake control. Verify the `IsInputBoxFocused()` early-`return false` cannot be bypassed.
- **The suppressed-action path.** `PressCurrentAction` must set `bInjected = false` and inject nothing; `ReleaseCurrentAction` must not send a key-up for it; `SampleFrame` must not read `UPlayerInput` for it (otherwise an unrelated Escape press elsewhere gets reported as this probe's observation); `LeavePass` must carry `bInjected` into the row so the report can say NOT INJECTED.
- **The control-retry branch in `LeavePass`** — it must be unreachable for MODE A/B, must not bank a row, must not advance `PassIndex`, and must be bounded by BOTH the attempt count and the wall-clock budget.
- **`GetResults()[0]` is still the CONTROL row** after retries (retries do not bank), so the INCONCLUSIVE gate for MODE A/B is unchanged.
- **`Disarm(bFromTicker)`** — `RemoveTicker` is skipped when called from inside the ticker (returning `false` is what unregisters it); `Options`/`LastFrameSeconds` are copied *before* `Disarm` at both start sites.
- **`EndProbe` now re-focuses the game viewport.** Without it the last pass leaves focus on a text box that is about to be removed and `FInputModeGameAndUI` focuses nothing — the session would be left unplayable, which would read as "the probe broke the game". Skipped when the world is tearing down.
- **The C4458 fix survives:** the parent-chain local is still `Walker`, never `Cursor` (`UWidget::Cursor`), in **both** `IsInputBoxFocused()` and the new `IsGameViewportFocused()`.
- **Comment-trap scan: PASS.** Run mechanically over raw file contents, not through a text search: **89 block comments in the .h, 44 in the .cpp, zero unterminated, zero premature terminators inside a doc block**, longest block 116 lines (the header doc) closing exactly once.
- **⚠️ Behavioural caveat for whoever runs it:** the CONTROL pass now genuinely delivers 17 keystrokes plus RMB into the live game (that is the point). Any single-key binding on `w a s d e n f o t m` and any RMB order **will fire** during the control. That is expected and does not affect the measurement, but it is not a no-op on the match state.

## R6. Compliance

- **CONVENTIONS §5:** `FAutoConsoleCommandWithWorldAndArgs` + two `FAutoConsoleCommandWithWorld`, all in this new file. **No `UFUNCTION(exec)` on any shipped class** — `USiegeCheatManager` and `ASiegePlayerController` remain untouched.
- **No new module dependency.** `Widgets/SViewport.h` comes from Slate, `Engine/GameViewportClient.h` and `Engine/Engine.h` from Engine, `Containers/Ticker.h` / `Misc/Parse.h` / `HAL/PlatformTime.h` from Core — all already public deps. **`GitClaudeUnrealTest.Build.cs` NOT edited** (TASK-417 owns it).
- **No per-tick work beyond the probe's own need:** the arming ticker exists only while armed and unregisters itself the moment it starts, aborts or is cancelled; `NativeTick` returns immediately when no probe is running; the `Cooldown` phase samples nothing.
- **Throwaway by design:** Wave 1 B3 still deletes both files.
