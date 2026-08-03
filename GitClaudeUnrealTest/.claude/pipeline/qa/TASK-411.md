# QA Report — TASK-411 (full review of the REWORK)
Verdict: **PASS** — 0 BLOCKER · 6 WARN · 6 NIT

Scope: `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantInputProbe.h` (553 lines) ·
`SiegeAssistantInputProbe.cpp` (1843 lines) · `handoffs/TASK-411-programmer.md` §Rework.
This is TASK-411's **first full review** — `qa/TASK-412.md` was a targeted confirmation and
explicitly scoped itself out of the probe's logic.

Method: both project files read **raw, in full** (never through Grep). Every engine claim
re-derived from installed UE 5.8 source at `C:\Program Files\Epic Games\UE_5.8\...`, not from
the handoff. Relayed diagnoses were treated as leads until the line was read.

---

## 1. THE CLAIMED ROOT CAUSE — INDEPENDENTLY CONFIRMED, ALL SIX CITATIONS

The programmer's single-root-cause claim (the old CONTROL pass's `ClearKeyboardFocus`) **holds**,
and it supersedes TASK-413's `-ExecCmds`-frame-0 diagnosis. Read, not recalled:

| claim | engine source | verdict |
|---|---|---|
| `ProcessKeyDownEvent` routes ONLY along the focus path | `SlateApplication.cpp:4961` (fn), **`:5018`** `TSharedRef<FWidgetPath> EventPathRef = SlateUser->GetFocusPath();` → tunnel `:5025` / bubble `:5047`, both `FEventRouter::RouteAlongFocusPath(EventPath)` | ✅ CONFIRMED — nothing else receives the key |
| an unhandled key falls through to the editor handler | **`:5069`** `if (!Reply.IsEventHandled() && UnhandledKeyDownEventHandler.IsBound())` | ✅ CONFIRMED |
| that handler is the editor's, and reaches the play-world actions | `MainFrameActions.cpp:97` binds `SetUnhandledKeyDownEventHandler(&FMainFrameActionCallbacks::OnUnhandledKeyDownEvent)`; **`:275`** → **`:283`** `FPlayWorldCommands::GlobalPlayWorldActions->ProcessCommandBindings` | ✅ CONFIRMED |
| `StopPlaySession` is bound to the Escape chord | `DebuggerCommands.cpp:358` `UI_COMMAND(StopPlaySession, "Stop", "Stop", ..., FInputChord(EKeys::Escape));` | ✅ CONFIRMED |
| a plain `UEditableTextBox` does **not** absorb Escape | `SlateEditableTextLayout.cpp:1102` `Reply = BoolToReply(HandleEscape());` → **`:1448-1476`** returns `false` with no search text, no selection, and `ShouldRevertTextOnEscape()==false` | ✅ CONFIRMED |
| an unconsumed key keeps bubbling past the viewport | `SceneViewport.cpp:1285-1289` — `CurrentReplyState = FReply::Unhandled()` when `ClientPtr->InputKey(...)` returns false | ✅ CONFIRMED |
| the input mode does **not** imply focus | `PlayerController.cpp:6313-6318` `SetFocusAndLocking` calls `SetUserFocus` **only** `if (InWidgetToFocus.IsValid())`; `:6446` `FInputModeGameOnly` is the only mode that focuses the viewport itself | ✅ CONFIRMED |

**Both of the programmer's corrections to TASK-413 stand:**

1. **"Passes 2 and 3 would have survived" is REFUTED.** Escape is Unhandled from a plain focused
   box (`:1448`), `FSceneViewport::OnKeyDown` returns Unhandled (`:1288`), and the event then hits
   the unhandled handler (`:5069`) → `StopPlaySession`. **Escape ends PIE from any pass, focused or
   not.** The new design's Escape handling is therefore correctly built, not over-built.
2. **The frame-0 theory is not the cause.** `-ExecCmds` firing on frame 0 is a real hazard (now
   gated), but the old `BeginProbe` already refused to run without a possessed pawn and the control
   typed ~4 s later. The empty focus path explains `PATH length 0.00` exactly; frame 0 does not.

---

## 2. THE THREE FIXES — GATED

### Fix 1 — CONTROL focuses **and verifies** the game viewport widget · ✅ PASS

- `EnterPass` (cpp:839-847) takes the control branch, sets `bFocusRequested = false`, calls
  `FocusGameViewport()`; `NativeTick`'s Arming case (cpp:1366-1371) **re-asserts every frame**;
  `bViewportFocusEstablished` is recorded **once**, at the instant typing begins (cpp:1385-1393).
- `LeavePass` (cpp:1136-1143) reaches the harness-fault branch first and words it exactly as
  specced — *"this is a HARNESS fault, not evidence about the input stack"* — before the
  hero-did-not-move branch at `:1151`.
- **Cannot mislabel either way.** A harness fault can only be reported as "the hero did not move"
  if `bViewportFocusEstablished` were true while the harness had no route; but `IsGameViewportFocused()`
  returning true *means* the focus path terminates at `SViewport`, which is precisely the route to
  `UGameViewportClient::InputKey`. The converse mislabel (real movement failure stamped a harness
  fault) is conservative — it refuses rather than answers.
- **The box exclusion is intact and unbypassable.** `IsGameViewportFocused()` performs
  `if (IsInputBoxFocused()) { return false; }` at **cpp:582**, *before* the ancestor walk at
  `:595`. This matters: `AddToViewport` parents the probe under the viewport's game layers, so a
  naive ancestor walk would report "viewport focused" while the box holds focus. There is no code
  path to the walk that skips the exclusion.
- Engine cross-check: `SViewport` is a legitimate keyboard-focus target — the engine itself does
  `SlateOperations.SetUserFocus(ViewportWidgetRef)` at `PlayerController.cpp:6446` and
  `SceneViewport.cpp:740`.

### Fix 2 — Escape is never injected in a PIE world · ✅ PASS

- Policy resolved **once** in `BeginProbe` (cpp:705-735); `Auto ⇒ !bInPIEWorld`.
- `PressCurrentAction` (cpp:1028-1035): suppressed ⇒ `bInjected = false`, **nothing injected**,
  `bActionHeld` still true so the phase machine is timing-identical.
- `ReleaseCurrentAction` (cpp:1066): `if (Current.bInjected)` — no key-up for a suppressed slot.
- `SampleFrame` (cpp:995): `if (Current.bInjected && ...)` — `UPlayerInput` is **not** read for a
  suppressed action, so an unrelated Escape elsewhere cannot be adopted as this probe's observation.
- `LeavePass` (cpp:1110-1114) carries `bInjected` into the row; `DescribeObservation` (cpp:252-258)
  branches on `!bInjected` **first** and prints `NOT INJECTED — <reason>`.
- **I traced every printer of an Escape/Enter/RMB result. `DescribeObservation` is the only one**
  (cpp:349-356); the BANKED line prints no observation; the footer (cpp:371-373) states in the
  probe's own words that `NOT INJECTED` is not a swallow. **There is no path by which a suppressed
  observation can surface as a measured negative.** This is the exact silent-false-success shape,
  and it is closed.

### Fix 3 — the command ARMS, and cannot pass readiness early · ✅ PASS

- `SiegeAssistantInputProbeCommand` (cpp:1760-1805) only registers an `FTSTicker`; the log line
  says `ARMED`, deliberately not "starting".
- `bHardReady` (cpp:1658-1664) = world + first PC + **possessed pawn** + `PlayerInput` + valid
  game-viewport widget + `FSlateApplication::IsInitialized()` + `(GFrameCounter - ArmFrame) >= 4`.
  `GFrameCounter` is monotonic, so the unsigned subtraction is safe.
- `bWarm` (cpp:1668-1669) = `WaitedSeconds >= WarmUpSeconds` (5 s) **AND** 45 consecutive frames
  ≤ 50 ms. With defaults this cannot be satisfied early. `force` skips only the frame count, not
  the wall-clock minimum.
- Budget expiry **with** hard requirements ⇒ starts and says so loudly, and stamps the report
  `warm-frame window ⚠️ NOT SATISFIED` (cpp:1586-1596). Budget expiry **without** ⇒ ABORT naming
  every failed condition individually + `NOTHING WAS MEASURED` (cpp:1693-1704). World lost after
  arming ⇒ immediate abort (cpp:1639-1646) rather than idling a ticker for two minutes.
- **Control retry is correctly bounded and cannot corrupt the gate.** The retry branch
  (cpp:1197-1221) requires `Working.bRan && Pass == Control && Verdict == "CONTROL-FAILED"` **AND**
  `(ControlAttempt + 1) < MaxControlAttempts` **AND** `(now - ProbeStartRealSeconds) < ControlBudgetSeconds`
  — both bounds, as specced. It `return`s at `:1220`, **before** `GetResults().Add(Working)` at
  `:1229`, so retries never bank and **`GetResults()[0]` is always the final CONTROL row**. It is
  unreachable for MODE A/B (guarded on `Pass == Control`) and never advances `PassIndex`.
- **Exhaustion reports INCONCLUSIVE, never a default answer.** Retries spent ⇒ the CONTROL-FAILED
  row is banked ⇒ MODE A/B hit `!bControlOk` (cpp:1161-1165) ⇒ `INCONCLUSIVE`. There is no
  "the answer is X" line anywhere in the probe for a default to leak into.
- Timing sanity: 4 attempts × (~4.9 s at 60 FPS / ~8 s at 3 FPS) + 3 × 1.5 s cooldown ≈ 26–38 s,
  inside the 60 s control budget. The bound is reachable, not decorative.

---

## 3. ALSO VERIFIED

- **Rows banked one at a time + PARTIAL banner** — ✅ `GetResults().Add` at cpp:1229 is followed
  *immediately* by the `BANKED row N/3` `UE_LOG` at `:1235`. `LogReport` prints
  `"No probe has produced a row in this session"` **only** when `Results.Num() == 0` (cpp:280-284),
  and otherwise prints a loud `⚠️ PARTIAL RUN — %d of %d passes banked` whenever `!GetCompleted()`
  (cpp:295-300). The TASK-413 reporting bug that lost a whole run's evidence is fixed.
- **Every refusal guard survives, and all fail closed** — `LeavePass` orders them
  control → focus → text → moved (cpp:1157-1185). `bFocusHeld` requires `FocusWindowSamples > 0`,
  so a zero-sample window is INCONCLUSIVE, not vacuously true. `bTypedTextLanded` is an exact
  **case-sensitive** match of the sentence. `MODE A PASS` therefore requires the conjunction
  *control proved injection reaches the game* ∧ *≥90 % focus over the typing window* ∧ *the exact
  sentence landed in the box* ∧ *no movement and no W/A/S/D on `UPlayerInput`*. **I could not
  construct a false-PASS path.** The rework did not make the probe more willing to answer.
- **`FocusHeldSamples` is scored against `FocusWindowSamples`** (cpp:970-977, 1158-1159), and the
  typing window closes at the first non-typing action (cpp:1013-1016) — correct, since
  `ClearKeyboardFocusOnCommit` legitimately drops focus after the synthetic Enter.
- **`focus-at-press` per observation** — ✅ sampled **before** injection (cpp:1024-1026), carried
  per-tag into the row (cpp:1108-1127), printed per observation, and a contaminated row says so in
  words (cpp:262-263). A contaminated row is visible rather than silently wrong.
- **MODE A/B Enter + RMB still measured in every environment** — ✅ neither is ever suppressed
  (cpp:900-913). See WARN-5 for a correction to how the *Enter* half must be read.
- **`FAutoConsoleCommand` family only** — ✅ one `FAutoConsoleCommandWithWorldAndArgs` + two
  `FAutoConsoleCommandWithWorld` (cpp:1829-1842), all in this new file, namespace `Siege.Assistant.*`.
  No `UFUNCTION(exec)` added anywhere. Matches CONVENTIONS §5 (`CONVENTIONS.md:634`).
- **C4458 `Walker` rename survives in BOTH helpers** — ✅ `IsInputBoxFocused` cpp:**540** and
  `IsGameViewportFocused` cpp:**595**. No local named `Cursor` anywhere in either file. I also
  re-swept every new local in the rework against `UWidget`/`UUserWidget` members — no new shadow.
- **M8 declaration duty, verbatim in the header** — ✅ `.h:42-44` contains
  *"adds no replicated property, no new replicated class, no new relevancy tier"* character-for-character
  (CONVENTIONS.md:714).
- **No per-tick work beyond need** — ✅ the arming ticker exists only while armed and unregisters
  on start / abort / cancel (returns `false`; `Disarm(bFromTicker=true)` correctly skips
  `RemoveTicker`, cpp:1528-1539). `NativeTick` returns immediately when `!bProbeRunning`
  (cpp:1336-1339). Cooldown samples nothing.
- **`Options` / `LastFrameSeconds` copied before `Disarm`** — ✅ cpp:1672-1673, used at both start
  sites. `ProbeWorld` is a raw local and survives `Disarm` clearing `WeakWorld`.
- **No deprecated UE 5.8 API** — uses `RemoveFromParent()` (not the deprecated
  `RemoveFromViewport`), `FTSTicker` (not `FTicker`), the live 7-arg `FPointerEvent` ctor
  (`Events.h:721`), and `auto` for `GetCursorPos()`'s `UE::Slate::FDeprecateVector2DResult` — that
  last one is correct and deliberate; naming the type is what the deprecation shim forbids.
- **Compile/link surface** — every signature checked against 5.8: `FKeyEvent` 6-arg (matches the
  engine's own use at `SlateApplication.cpp:5081`), `FCharacterEvent` (`Events.h:619`; the explicit
  `uint32` cast avoids the `FInputDeviceId` overload), `FPointerEvent` (`Events.h:721`; the 7-arg
  `:810` overload is not viable, so no ambiguity), `ProcessMouseButtonDownEvent(TSharedPtr<FGenericWindow>, ...)`
  (`SlateApplication.h:1302`; the null window is engine-guarded at `.cpp:5313`),
  `SetUserFocusToGameViewport(uint32, EFocusCause)` (`:644`), `SetKeyboardFocus(TSharedPtr<SWidget>, EFocusCause)`
  (`:724`), `FTSTicker::FDelegateHandle = TWeakPtr<FElement>` + `static RemoveTicker` (`Ticker.h:32/66`),
  `UWorld::bIsTearingDown` **public** (`World.h:1347`), `GetGameViewport() const` (`World.h:2810`),
  `GetGameViewportWidget() const` (`GameViewportClient.h:162`), `APlayerController::PlayerInput`
  public (`PlayerController.h:370`), `IsInputKeyDown(FKey) const` (`:1594`),
  `SetRevertTextOnEscape` / `SetClearKeyboardFocusOnCommit` / `SetIsReadOnly` / `SetHintText` all
  present (`EditableTextBox.h:203/210/232/179`), `FOnEditableTextBoxCommittedEvent` matches
  `HandleProbeTextCommitted`'s signature exactly (`:34`). `Build.cs` already carries
  `Slate` + **`SlateCore`** + `UMG` + `InputCore` as public deps, so the link gap that bit the last
  pass is closed; **the file needs no `Build.cs` change and did not make one.**
- **The widget really will tick.** `UserWidget.cpp:2356-2361`: with `EWidgetTickFrequency::Auto`,
  `bCanTick |= !WidgetBPClass` — `Cast<UWidgetBlueprintGeneratedClass>` is null for this native
  class, so ticking is on. (Had this been false, the whole probe would have been a silent no-op.)
- **Every injected key resolves to the intended `FKey`.** `GetKeyFromCodes(0x57, 0x57)` → `EKeys::W`
  via `KeyMapCharToEnum` (Windows registers **uppercase** char codes,
  `GenericPlatformInput.cpp:23-51`, `WindowsPlatformInput.cpp:126`); space → `VK_SPACE` in
  `KeyMapVirtualToEnum` (`WindowsPlatformInput.cpp:28`). No synthetic-key fallback is hit, so the
  `CharCode = MapVirtualKey` fidelity the handoff calls "the single highest-leverage line" is
  genuinely correct — and it also satisfies the `GetCharacter() != 0` absorb condition at
  `SlateEditableTextLayout.cpp:1218`.
- **Low-FPS sampling order is correct and load-bearing.** In the Acting case, `SampleFrame` runs
  **before** the release check (cpp:1408 vs 1421). At ~3 FPS the reverse order would release the
  key before it was ever observed down on `UPlayerInput`, silently zeroing `bMoveKeyReachedPlayerInput`
  on every pass. Also verified the injection lands one world-tick ahead of the sample, so the key is
  genuinely down for at least one Enhanced Input evaluation.
- **No momentum bleed between passes (a false-FAIL path I specifically hunted).** MODE A's
  measurement starts after CONTROL's 0.35 s settle **plus** MODE A's 0.35 s arming window, and
  `StartLocation` / `LastSampledLocation` are both captured at the *end* of arming (cpp:1375-1379),
  so braking distance is excluded. Cross-checked the shipped controller: RMB is **cancel-only**
  (`SiegePlayerController.cpp:458/490/518`), never a click-to-move, so the control cannot leave the
  hero under a standing order. ✅ Not an issue.
- **Every-frame focus re-assertion during Arming is load-bearing, not belt-and-braces.** MODE A's
  RMB observation can reach the shipped `WasInputKeyJustPressed(EKeys::RightMouseButton)` cancel at
  `SiegePlayerController.cpp:458/490/518` → `ApplyCursorInputState()` → `SetInputMode(...)`, whose
  `FReply` is deferred to the next frame and can steal focus before MODE B starts. The re-assertion
  loop at cpp:1359-1371 absorbs exactly that.
- **Comment trap, re-run on raw file reads (never Grep).** ✅ **Zero unterminated block comments,
  zero premature `*/` inside a doc block, zero nested `/*`.** Both files parse to a clean close
  (`.h` ends at `};` line 553; `.cpp` ends at the three console-command registrations, 1829-1842),
  and every inline `/*bIsRepeat*/` / `/*WheelDelta*/` / `/*PointerIndex*/` / `/*bFromTicker*/` /
  `/*bWarm*/` is closed on its own line. Deliberately re-checked the strings that Grep renders
  wrongly — `"W/A/S/D on PlayerInput"` (cpp:335), `"PASS / FAIL / INCONCLUSIVE / ..."` (h:373),
  `"TASK-076/112"` (h:133), `"this .h/.cpp pair"` (h:40) — all are ordinary text, none manufacture
  or mask a marker. See NIT-5 on the asserted count.

---

## 4. RULING ON THE OPEN CAVEAT (TASK-413's `MODE A: Enter -> PlayerInput: YES`)

**The programmer's hypothesis is SOUND — and it is stronger than a hypothesis: the source forces it.**

`FSlateEditableTextLayout::HandleKeyDown:1092-1097` is
`else if (Key == EKeys::Enter && !OwnerWidget->IsTextReadOnly()) { ... Reply = FReply::Handled(); }`
— **unconditional on modifiers**, and the probe's box is explicitly `SetIsReadOnly(false)`
(cpp:467). A genuinely focused, editable box therefore **always** absorbs Enter-down, which can then
never bubble to `SViewport` and never reach `UPlayerInput`.

⇒ `Enter -> PlayerInput: YES` **entails** the box did not hold focus at that press. There is no
third possibility. Escape losing the box its focus is not merely the *most likely* explanation, it
is the only class of explanation consistent with the engine.

A concrete mechanical route exists for it, which I verified end to end: the unabsorbed Escape
(`:1448` → Unhandled) bubbles to `SViewport` → `UGameViewportClient::InputKey` → the shipped
`WasInputKeyJustPressed(EKeys::Escape)` cancel paths at `SiegePlayerController.cpp:458/490/518` →
`ApplyCursorInputState()` → `SetInputMode(...)`, and `FInputModeGameOnly::ApplyInputMode` does
`SlateOperations.SetUserFocus(ViewportWidgetRef)` (`PlayerController.cpp:6446`) while
`FInputModeGameAndUI` with no `WidgetToFocus` restores nothing (`:6315-6318`). Either branch leaves
the box unfocused before the Enter press.

**`focus-at-press` is the correct instrument and is the first thing to read on the re-run.**

⚠️ **Design consequence for Wave 1 — read this before B3's open-key decision.** With Escape now
suppressed in PIE, MODE A's Enter row is expected to read **`NO — never reached UPlayerInput
(box focused at press: yes — the box absorbs Enter via HandleCarriageReturn)`**. That is the
*correct* result, not a regression. **Enter does not reach the game from a focused box, in either
input mode.** The console's open key is therefore safe to be Enter (the box is not focused when
closed), and while the console is open Enter means *submit* — but B3 must **not** carry TASK-413's
"MODE A fires Enter" into its cost model. See WARN-5.

---

## Findings

- **[WARN]** `SiegeAssistantInputProbe.cpp:1268-1292` — `EndProbe` restores the controller posture
  (`bShowMouseCursor`, `bEnableClickEvents`, `SetInputMode`) **unconditionally**, while the focus
  restore immediately below it *is* correctly gated on `bWorldIsLive` (`:1302-1304`). `EndProbe` is
  reachable from `NativeDestruct` during world teardown/GC, and `GetProbeController()` /
  `GetProbePawn()` are null-checked but never `IsValid()`-checked. — **Fix:** hoist the
  `bWorldIsLive` computation above `:1268` and wrap the restore block in it; use
  `IsValid(Controller)` rather than `!= nullptr` in `GetProbeController`/`GetProbePawn`.
- **[WARN]** `SiegeAssistantInputProbe.cpp:1319-1327` — the `else` branch calls `RemoveFromParent()`
  **synchronously**, defeating the very deferral the `if` branch exists for (the comment at
  `:1316-1318` correctly identifies that `EndProbe` runs while Slate is walking this widget's tree).
  That branch is reachable from `NativeTick`, not only from `NativeDestruct`: `bIsTearingDown` is set
  at the start of the PIE-stop sequence while the controller and pawn are still alive and the widget
  still ticks. — **Fix:** always defer (fall back to a one-shot `FTSTicker` when the world timer
  manager is unavailable), or document why the synchronous path is safe.
- **[WARN]** `SiegeAssistantInputProbe.cpp:747-749` vs `:280-300` / `:1811-1819` — **stale-report
  hazard.** `GetResults()`, `GetCompleted()`, `GetEnvironmentLine()` and `GetWarmUpLine()` are reset
  only in `BeginProbe`. An **arming abort** (`:1639`/`:1693`) or `InputProbeCancel` therefore leaves a
  previous run's rows intact — and if that earlier run *completed*, `Siege.Assistant.InputProbeReport`
  prints a full three-row table with **no PARTIAL banner and no run identity**, immediately after a
  run that measured nothing. This is the last remaining shape of "report evidence that was not
  measured now". Mitigating: the authoritative `BANKED` lines are timestamped by `UE_LOG`, and the
  programmer documents the cross-session case at `:1823-1825`. — **Fix:** stamp a run serial +
  `FDateTime::Now()` into `GetEnvironmentLine()` at `BeginProbe` and print it with every report, so a
  stale table identifies itself.
- **[WARN]** `SiegeAssistantInputProbe.cpp:1037-1046`, `:357-358` — the RMB observation depends on an
  **uncontrolled OS cursor position**. RMB travels the pointer path
  (`SlateApplication.cpp:5382`, `LocateWindowUnderMouse(ScreenSpacePosition)`), so if the cursor sits
  over the Output Log / another window — very likely right after typing a console command, and the
  arming wait does not move it — **both** MODE A and MODE B report `NO`, inviting the false
  conclusion "GameAndUI also swallows RMB". The coordinates are printed, but nothing stamps the row
  contaminated. Contrast the care taken over `focus-at-press`. — **Fix:** before the RMB press, test
  the cursor against the game viewport widget's cached geometry and set
  `bRightMouseObservationContaminated` (report it in those words), or `SetCursorPos` to the viewport
  centre and say so in the row.
- **[WARN]** `SiegeAssistantInputProbe.cpp:367-370` (report footer) — the A/B mechanism attribution is
  **wrong for Enter**. A focused box absorbs Enter at `SlateEditableTextLayout.cpp:1092` *regardless
  of input mode*, so the Enter half of "MODE B swallows both / MODE A fires both" cannot isolate
  `SetIgnoreInput` — both modes will read `NO` for different reasons. The per-row `AbsorbedNote`
  (cpp:352) says this correctly, but only when `bFocusedAtPress` is true; the footer invites the
  reader to price Enter as an input-mode cost anyway. **Corollary: TASK-413's `MODE A Enter → YES`
  was necessarily a lost-focus row, and B3 must not carry it forward.** The **RMB** half of the claim
  *is* sound and remains the real cost of the fallback. — **Fix:** in the footer, split the two:
  RMB isolates the input mode; Enter is absorbed by the box in both modes and only its
  `focus-at-press` field is informative.
- **[WARN]** `SiegeAssistantInputProbe.cpp:982-986` — `bMoveKeyReachedPlayerInput` is sampled every
  frame of every pass with no operator-input isolation, and it alone forces `bMoved` (`:1130-1132`).
  A human touching W/A/S/D (or the RMB) during MODE A/B produces a confident **FAIL** — the
  expensive direction, since FAIL flips B3 to the `UIOnly` fallback that also kills mouse-look and
  the shipped Escape/RMB cancel routes. Nothing warns the runner, and R4's self-distrust list omits
  this case. — **Fix:** print a loud "DO NOT TOUCH THE KEYBOARD OR MOUSE UNTIL THE PROBE REPORTS" at
  ARM and at each pass start, and add it to R4.
- **[NIT]** `SiegeAssistantInputProbe.cpp:335` — `W/A/S/D on PlayerInput: no — Slate absorbed them`
  is printed on the CONTROL row too, where (with `bViewportFocusEstablished == false`) the true
  meaning is "the harness had no route". The verdict line above it is correct and loud, so the
  verdict cannot mislabel — but the gloss contradicts it. Make the suffix conditional on the pass.
- **[NIT]** `SiegeAssistantInputProbe.cpp:420-425` — a second `RebuildWidget()` on the same instance
  logs *"an asset-authored tree is present — the code-authored branch is skipped (ruling A escape
  hatch)"*, which is false: it is the tree this class built. Track it with a separate bool.
- **[NIT]** `SiegeAssistantInputProbe.cpp:1607-1608` — `AddToViewport` before `BeginProbe` is correct
  and necessary (`RebuildWidget` must run for `InputBox` to exist), but if `BeginProbe` refuses via
  any of its five early-returns (`:662-691`) the widget is left in the viewport permanently with a
  live, hit-testable text box. — **Fix:** `if (!ProbeWidget->IsProbeRunning()) { ProbeWidget->RemoveFromParent(); }`.
- **[NIT]** `SiegeAssistantInputProbe.cpp:650-656` — `CommittedText` is an unused named parameter
  while the two console commands comment theirs out as `UWorld* /*World*/`. **Not a build break** —
  `UnusedParameterWarningLevel` defaults to `Off` ⇒ `/wd4100`
  (`UnrealBuildTool/Configuration/Rules/CppCompileWarnings.cs:910-912`). Consistency only.
- **[NIT]** Comment-count claim — I verified the **safety property** on raw reads (zero unterminated,
  zero premature terminators, no nesting). I hand-count **88** block-comment openers in the `.h`, not
  the asserted 89; I make no claim on the `.cpp` figure. Immaterial to correctness — flagged only
  because a number was asserted, and the count is not the property that matters.
- **[NIT]** `SiegeAssistantInputProbe.cpp:367-369` — MODE B's RMB swallow is attributed to
  `SetIgnoreInput(true)`. The *proximate* mechanism is `SetMouseCaptureMode(NoCapture)`
  (`PlayerController.cpp:6385`), which makes `bTemporaryCapture` false at `SceneViewport.cpp:692-703`
  so `ClientPtr->InputKey` is never called; `IgnoreInput()` then separately gates the focus/capture
  acquisition at `:718`. Same conclusion, more precise citation.

---

## Notes for build-master

1. **Bar #6 may be de-PROVISIONALised on my side — this is a PASS.** The instrument is sound and,
   more importantly, its *refusals* are sound: I could not construct a false-PASS path, and every
   exhaustion route terminates in INCONCLUSIVE rather than a default answer.
2. **Run it in standalone `-game` if you want the Escape matrix at all.** In PIE the Escape row is
   *by design* `NOT INJECTED` and is not evidence. Do not quote it as a swallow.
3. **Read in this order:** (a) the CONTROL row's `viewport focus` line — anything other than
   `VERIFIED` means the run is a harness fault and nothing below it is evidence; (b) `control
   attempts` + `earlier attempts` — a control that only passes on attempt 3–4 while earlier attempts
   show `viewportFocus=yes` means something is *intermittently* eating input and the whole run is
   measuring a moving target; (c) **`box focused at press` on the Enter row** — this is the field
   that settles the TASK-413 `Enter → YES` contradiction (see §4).
4. **Expect MODE A's Enter row to read `NO`.** That is correct (`SlateEditableTextLayout.cpp:1092`),
   not a regression, and it is *not* attributable to the input mode. See WARN-5.
5. **Before the run: park the mouse cursor over the game viewport, and keep hands off the keyboard
   until the report prints.** Both are unguarded confounds (WARN-4, WARN-6). Then sanity-check the
   printed `RMB cursor was at (x, y)` against the viewport rect.
6. **`pass timing` mean > 50 ms ⇒ treat every row from that pass as indicative only** — the probe
   prints its own `BELOW 20 FPS` warning. Also check `warm-frame window` in the `ARMING:` line: if it
   says `⚠️ NOT SATISFIED`, the budget expired and the run started anyway.
7. **If a run is interrupted, prefer the inline `BANKED row N/3` log lines over
   `Siege.Assistant.InputProbeReport`** — the banked lines are timestamped and unambiguous, whereas a
   re-printed table carries no run identity (WARN-3).
8. **No `Build.cs` change is needed** — `Slate` + `SlateCore` + `UMG` + `InputCore` are already public
   deps and every include path and signature in this file was checked against installed 5.8 source.
   No new replicated property, class, or relevancy tier.
9. **Behavioural, non-negotiable:** the CONTROL pass delivers 17 real keystrokes plus a real RMB into
   the live match. Any binding on `w a s d e n f o t m` and the shipped RMB cancel **will fire**. This
   does not affect the measurement but it is not a no-op on match state — do not run it mid-anything
   you care about.

---

**Verdict: PASS.** 0 BLOCKER. None of the 6 WARNs can produce a false answer to the binary question
(*does a focused `UEditableTextBox` starve Enhanced Input of WASD?*); they affect the secondary RMB /
Enter comparison, teardown robustness, and report hygiene. The three fixes do what they claim, the
root cause is confirmed against UE 5.8 source rather than relayed, and every refusal guard the
probe's prior INCONCLUSIVE depended on is intact.

*Reviewed by qa-reviewer, 2026-08-02. Read-only: no file in `Source/` was modified.*
