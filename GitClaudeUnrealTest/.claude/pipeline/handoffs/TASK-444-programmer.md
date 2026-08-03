# TASK-444 — [W1-B3] `USiegeAssistantConsoleWidget` + the deletion of the input probe

**Agent:** gameplay-programmer · **Date:** 2026-08-03 · **Status on completion:** `ready-for-qa`
**Law read first:** CONVENTIONS "In-match LLM command assistant" **§2, §3, §6 ruling A, §9** · "Settings screen + the assistant CONFIRM STEP…" **§3 (for the contrast only), §8** · "Widgets with C++ bases" · "Input-mode ownership (level-travel law)" · "Delegates (C++)" · "Logging (C++)" · board rulings **9, 10, 12**.

> **M8 DECLARATION DUTY, verbatim: adds no replicated property, no new replicated class, no new relevancy tier.**
> And *why* it is true rather than merely asserted: this widget is **client-local by construction**. It has no authority, no RPC, no `Replicated` UPROPERTY, and its entire output surface is four local delegate broadcasts to a local component. It is an input surface, not a game-state carrier.

---

## 0. ⚠️ THE ONE SENTENCE QA MUST NOT LET ME SOFTEN

**I cannot verify that this renders correctly, and I did not try.** There is no `.uasset` to read back, I ran no editor, no PIE and no MCP call, and this project has repeatedly had **MCP readback pass on visually-broken UMG** (TASK-355: six controls read back 6/6 correct while stacked in a 165×48 px box in a corner — *"a binding readback cannot see geometry"*). Ruling A(e) is explicit that correctness here closes on **rendered pixels or Jonathan's eyes**. Everything below about layout, legibility, size and position is **intent, not observation**. The checklist in §7 is written for TASK-448 for exactly that reason.

Nothing in this task was compiled either (one gate only — TASK-447, quiet-module law). §6 lists what a compile is owed.

---

## 1. Files touched

| File | Action |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h` | **NEW** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` | **NEW** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantInputProbe.h` | **DELETED** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantInputProbe.cpp` | **DELETED** |

**Nothing else.** No controller edit, no component edit, no snapshot edit, no `Build.cs` edit, no `Content/`, no `.ini`, no `.uproject`, no Git, no compile, no editor, no MCP, no PIE. `git status --porcelain` on the module shows my four paths and, as ruling 7 predicts, other lanes' dirt (`SiegePlayerController.{h,cpp}` from the un-committed FOLLOW batch, `SiegeAssistantSnapshot.{h,cpp}` from TASK-441, `SettingsMenuWidget.*` / `SiegeSettingsSaveGame.*` from the settings lane) — **I touched none of it.**

Assets referenced by name: 🔒 **`/Game/UI/WBP_AssistantConsole` — RESERVED, NOT AUTHORED** (ruling A(c)); it appears only in comments and as an optional class argument. `/Game/Input/Actions/IA_AssistantConsole` and `/Game/Input/IMC_Hero` are named **only in prose** — no key, action or mapping is referenced in code.

---

## 2. The widget structure

### 2a. The tree (code-authored, ruling A)

Built in `RebuildWidget()` via `WidgetTree->ConstructWidget<>`, bottom-anchored so it stays off the battlefield:

```
RootPanel            UVerticalBox      [PINNED]  SelfHitTestInvisible, the tree root
├─ ConsoleTopSpacer  USpacer           (internal) slot = Fill  → pushes everything below to the BOTTOM
└─ ConsoleBackdrop   UBorder           (internal) dark plate, SelfHitTestInvisible, slot = Auto/Bottom
   └─ ConsoleColumn  UVerticalBox      (internal) SelfHitTestInvisible
      ├─ TranscriptText  UTextBlock    [PINNED]  auto-wrap, 18 pt — the rolling game-authored dialogue
      ├─ StatusText      UTextBlock    [PINNED]  14 pt — the FSM state label / disabled reason
      ├─ InputBox        UEditableTextBox [PINNED]  ⚠️ the ONLY hit-testable control by default
      └─ ConsoleButtonRow UHorizontalBox (internal)
         ├─ ConfirmButton UButton      [PINNED]  label "Accept"  — Collapsed unless a prompt is up
         └─ CancelButton  UButton      [PINNED]  label "Cancel"  — Collapsed unless a prompt is up
```

The six **[PINNED]** names are `CONVENTIONS §6 ruling A`'s last bullet, character-for-character. The four internal nodes are **not** pinned members and are owned by the `WidgetTree`; they never compete with a future WBP, because the escape hatch skips the whole code branch.

**Ruling A's five conditions, each with where it lands:**
- **(a) SCOPE** — this class only; the header says so and cites §6(a)'s misuse clause.
- **(b) ESCAPE HATCH** — `ConstructConsoleTree()` returns immediately when `WidgetTree->RootWidget != nullptr` (an asset tree wins *whole*), **and** every pinned member is additionally constructed only when it is still null. Both layers are present.
- **(c) RESERVED NAME** — used nowhere; `CreateAndAddToViewport` takes a `TSubclassOf<>` so a future `WBP_AssistantConsole` is adopted with **zero C++ change**.
- **(d) CONTRACT** — `USessionMenuWidget`'s shipped contract cloned: `BindWidgetOptional` members, `BlueprintCallable` wrappers, `UFUNCTION()` click/commit thunks, and **FString/int32/bool/uint8-only BIEs**. FSM state is a `uint8` and is never an enum.
- **(e) PIXEL CHECK** — §0 above, and §7's checklist.

### 2b. The API (this IS the seam — QA should reconcile it against TASK-442/443)

**Inbound — the component drives these:**
`OpenConsole()` · `CloseConsole()` · `ToggleConsole()` · `IsConsoleOpen()` · `IsConsoleEnabled()` ·
`SetAssistantState(uint8 NewState, const FString& StateLabel)` · `GetAssistantState()` ·
`ShowTranscriptLine(const FString&)` · `ClearTranscript()` ·
`ShowConfirmPrompt(const FString& SummaryLine)` · `HideConfirmPrompt()` ·
`SetConsoleEnabled(bool, const FString& DisabledReason)` ·
`static USiegeAssistantConsoleWidget* CreateAndAddToViewport(APlayerController*, TSubclassOf<USiegeAssistantConsoleWidget> = nullptr, int32 ZOrder = 5)`

**Player entries (also the WBP wiring points):** `SubmitPressed(const FString&)` · `ConfirmPressed()` · `CancelPressed()`

**Outbound — the component binds these (`BlueprintAssignable`, `FOn<Owner><Event>` per the delegate law):**
`OnConsoleSubmitted(const FString& Utterance)` · `OnConsoleConfirmed()` · `OnConsoleCancelled()` · `OnConsoleOpenChanged(bool bOpen)`

**BIEs (FString/bool/uint8 only):** `OnConsoleOpenStateChanged(bool)` · `OnAssistantStateChanged(uint8, FString)` · `OnTranscriptLineShown(FString)` · `OnConfirmPromptChanged(bool, FString)` · `OnConsoleEnabledChanged(bool, FString)`

**Tunable:** `MaxTranscriptLines` (`EditDefaultsOnly`, default **6**) — **flagged for Jonathan's feel pass**: it trades readable history against occluded battlefield and only a playtest settles it.

### 2c. Four design decisions QA should weigh, not skim

1. **⛔ THE WIDGET NEVER INTERPRETS THE `uint8`.** It stores and displays it; the confirm buttons are raised by the *semantic* calls `ShowConfirmPrompt`/`HideConfirmPrompt`. Switching on the number would couple this widget to the **order** of an enum in another file owned by another task — and a reordering would move the buttons to the wrong state with **no compile error, no log line and no readback difference**. The alternative costs the component one extra call.
2. **⛔ `CloseConsole()` DOES NOT BROADCAST A CANCELLATION.** "The player closed the window" and "the player discarded the order" are different statements; turning one into the other is an FSM decision. The FSM hears `OnConsoleOpenChanged(false)` and decides. (It *does* fire `OnConfirmPromptChanged(false, …)` so no BIE consumer is left rendering a phantom prompt.)
3. **⛔ `ConfirmPressed()` IS GATED ON A PROMPT ACTUALLY BEING UP.** Accept must never be able to execute an order the player is not being shown — that is the entire point of the confirm step, whose rationale (4 of 5 stable failures are wrong-place/wrong-count) is the measured argument in CONVENTIONS §1 of the settings section.
4. **An empty/whitespace submit is refused locally** — no broadcast, no transcript line. Queue depth is 1; a blank prompt would spend the one in-flight model call to learn nothing.

---

## 3. THE INPUT CONTRACT — what TASK-445 (and the task that binds the key) must satisfy

**This widget hard-codes no key and assumes none.** `Enter` appears nowhere in the code as an input action.

1. **`IA_AssistantConsole`** at `/Game/Input/Actions/IA_AssistantConsole`, mapped in `/Game/Input/IMC_Hero`. ⚠️ **`Enter` is the PROPOSED open key, not an approved one** — TASK-445 runs the conflict check and **flags, never stomps**.
2. **SUBMIT IS ALWAYS ENTER-IN-THE-BOX, AND THAT IS SLATE, NOT ENHANCED INPUT.** A focused `UEditableTextBox` absorbs Enter through `HandleCarriageReturn` (`SlateEditableTextLayout.cpp:1092`) in **both** input postures — the `qa/TASK-411` ruling that corrected TASK-413's contradictory row. **Consequence: whatever key TASK-445 lands on, submit is unaffected**, and Enter is *usable* as the open key because the box is not focused while the console is closed.
   - ⚠️ **Read the provenance honestly.** The conclusive run recorded `Enter → UPlayerInput: NO` in **both** modes and QA ruled Enter is **not** an input-mode cost. An **earlier, INCONCLUSIVE** run's narrative said the opposite for MODE A; `qa/TASK-411` WARN-5 explains it as a lost-focus row and says B3 must not carry it forward. **I have not re-measured either**, and I have designed so that neither reading breaks the widget (see item 4).
3. **THE POSTURE IS `FInputModeGameAndUI` WITH KEYBOARD FOCUS ON THE BOX.** ⚠️ **Inherited measurement (spike bar #6), not re-derived here:** a focused text box does **not** starve Enhanced Input of WASD, so the camera stays live while typing. `qa/TASK-411` also measured that **RMB survives GameAndUI and dies under `UIOnly`** — the fallback's real cost, and it is not incurred.
4. **⛔ THIS WIDGET NEVER CALLS `SetInputMode`, AND THE POSTURE OWNER IS STILL MISSING FROM THE WAVE — see §5 flag (a).** It sets Slate *focus* only (`SetKeyboardFocus()` on open, `SetAllUserFocusToGameViewport()` on close). Input posture in `L_Arena` belongs to `ASiegePlayerController::ApplyCursorInputState()` per the level-travel law, and a second `SetInputMode` caller is precisely how a level ends up stranded in the wrong posture.
5. **ESCAPE IS DELIBERATELY NOT ABSORBED** (`RevertTextOnEscape` stays `false`), so the shipped `WasInputKeyJustPressed(EKeys::Escape)` cancel routes still fire while the console is open — §2's "must not disturb any key", honoured literally. **⇒ Escape does NOT close this console.** Cancel does, and so does the open key when bound as a toggle. If Jonathan wants Escape-to-close at playtest, that is a **deliberate, ruled disturbance of a shipped key**, not a bug fix.
6. **Focus is HELD across a commit** (`ClearKeyboardFocusOnCommit` forced to `false`). The engine default is `true`, which would drop focus after the first sentence — the box would *look* focused and silently not be, and the next Enter would leak into Enhanced Input and re-toggle the console. This is the explicit decision TASK-411's side finding #1 demanded.

---

## 4. PROOF THE PROBE IS FULLY REMOVED

Both files deleted from disk (`git status` shows ` D` on each, unstaged — **staging is build-master's**, not mine). The three console commands lived entirely inside `SiegeAssistantInputProbe.cpp` and are gone with it: **`Siege.Assistant.InputProbe`**, **`Siege.Assistant.InputProbeCancel`**, **`Siege.Assistant.InputProbeReport`** (registered as `FAutoConsoleCommandWithWorldAndArgs`/`FAutoConsoleCommandWithWorld` file-statics — no registration survives a deleted translation unit).

**Repo-wide sweep for `InputProbe` / `SiegeAssistantProbe` / `Siege.Assistant.InputProbe`, case-insensitive:**

| Location | Kind | Verdict |
|---|---|---|
| `Source/` (all `.h`/`.cpp`) | — | **ZERO**, except my own header's §6, which documents the deletion on purpose |
| `Content/` (`.uasset`/`.umap`) | — | **ZERO** — nothing referenced the class (it was `NotBlueprintable` and console-created) |
| `Config/`, `Saved/Config/`, `*.uproject`, `Tools/`, `Docs/` | — | **ZERO** — importantly, no `-ExecCmds`/autoexec entry points at a now-dead command |
| `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs:30` | **comment only** | ⚠️ survives — see below, **not mine to edit** |
| `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/VERSION.md:160` | **comment/history only** | survives — a dated build-log record of a past failure in that file; historically true, not a live reference, and outside my lane |
| `.claude/pipeline/**` | history | survives **correctly** — the record of what was measured and why |

**Zero compiled references remain: no `#include`, no type use, no symbol, no asset, no config.**

### ⛔ THE ONE HAZARD MY DELETION CREATES — READ THIS, TASK-443

`GitClaudeUnrealTest.Build.cs:30` names `SiegeAssistantInputProbe.cpp` as the file that proved **`SlateCore` is a required dependency** (`ETextCommit::Type` in a `UFUNCTION` signature produced 16 unresolved externals led by `Z_Construct_UEnum_SlateCore_ETextCommit`). **`Build.cs` is TASK-443's exclusive file under ruling 10, so I did not edit the comment.**

**The comment is now stale in its example and still correct in its rule** — and I verified the rule still binds:

- After the deletion, `SiegeAssistantConsoleWidget.cpp` is the **ONLY** file in the game module referencing an `FSlateApplication` symbol, and `SiegeAssistantConsoleWidget.h` is the **ONLY** file with `ETextCommit` in a `UFUNCTION` signature (`HandleTextCommitted`). Grep across `Source/GitClaudeUnrealTest/` confirms both: no other hit.
- ⇒ **`SlateCore` must stay in `PublicDependencyModuleNames`.** A "cleanup" that removes it because the named file is gone reproduces the exact 16-unresolved-externals link failure that comment was written to prevent. **Recommendation for TASK-443 (Build.cs owner): update the comment's example from `SiegeAssistantInputProbe.cpp` to `SiegeAssistantConsoleWidget.{h,cpp}` and change nothing else.** No dependency is added or removed by me.

---

## 5. FLAGGED — recorded, not silently decided

**(a) ⚠️ NOTHING IN WAVE 1 OPENS THIS CONSOLE, AND THAT IS A REAL GAP, NOT A COMPLAINT ABOUT MY OWN SCOPE.** TASK-444 builds the widget and TASK-445 creates the input asset, but **no task in the wave (i) binds `IA_AssistantConsole` to a handler, (ii) creates the widget at match start, or (iii) adds the open console as a posture owner in `ApplyCursorInputState()`.** Item (iii) is a `SiegePlayerController` edit, which TASK-440 exclusively owns and whose spec does not include it; I am forbidden from that file. **As the wave stands, the console exists and cannot be opened in-game.** Shape of the fix, for whoever is assigned it: one `EnhancedInputComponent->BindAction(IA_AssistantConsole, …, &ToggleConsole)`, one `CreateAndAddToViewport` at `BeginPlay`, and one `|| bAssistantConsoleOpen` term in `ApplyCursorInputState()`'s `bWantCursor` composition — that last one is how the law says a new posture owner is added, and it composes with placement / targeting / group-pick / `IA_UICursor` rather than fighting them. **⇒ This bears directly on TASK-448: without it there is nothing for Jonathan to type into.**

**(b) ⚠️ A CONFIRM PROMPT RAISED WHILE THE CONSOLE IS CLOSED IS INVISIBLE — and the deferred-intent path is exactly that case.** A deferred order fires up to **120 s** after it was typed, re-resolves and re-enters `AwaitConfirm` on a board the player has not looked at since. If the console is closed at that moment the player is never asked. I **log a Warning naming it** and deliberately did **not** auto-open: auto-opening is FSM policy (and would seize keyboard focus at a moment the player did not ask for it). **TASK-443 owns the decision** — its own spec says the deferred path "NEVER executes blind", which is only true end-to-end if something opens the console.

**(c) The two button labels ("Accept" / "Cancel"), the hint text and the idle "Ready" line are the only player-visible words this file authors.** They are static chrome, never model-derived, and none of them describes an order, unit, place or outcome — §3's "every sentence the player reads is a game-authored template" is about the *dialogue*, which arrives entirely through `ShowTranscriptLine`/`ShowConfirmPrompt` from the component's template table. Recorded so QA can rule on it rather than discover it.

**(d) The backdrop is hit-test INVISIBLE — the opposite of `USettingsMenuWidget`'s `BackdropBorder`, and both are correct.** The settings panel is modal over a menu, so a click-through into "Quit" would be a live defect; this console is **non-modal over live gameplay**, so a hit-testable full-width plate would eat the shipped RMB cancel that `qa/TASK-411` measured as the real casualty of the wrong posture. **⛔ Do not "conform" the two widgets.** Commented at the site so a future reader cannot mistake it for an oversight.

---

## 6. What is NOT verified, absent a compile

I did not compile (TASK-447 is the only gate). Honest inventory:

- **UHT/link:** the class, four `DECLARE_DYNAMIC_MULTICAST_DELEGATE*`s at file scope (house idiom), five BIEs, twelve `UFUNCTION`s and six `BindWidgetOptional` members have **never been through UnrealHeaderTool**. Every parameter type is `FString`/`bool`/`uint8`/`int32`/`FText`/`ETextCommit::Type` and every BIE obeys the widget-param law, but that is a reading, not a build.
- **This file, unusually for this batch, has NO cross-task dependency and should compile alone.** It includes only shipped headers (`SiegeAssistantCommand.h` for `LogSiegeAssistant`, plus UMG/Slate/Engine) and references no TASK-440/441/442/443 symbol. So the board's "will not compile alone and is not expected to" is *stricter than necessary* here — ⚠️ but I am asserting that from a read, not a build, and it is not a licence to compile early.
- **Every engine API used was checked against the UE 5.8 source on this machine**, not from memory: `UEditableTextBox::Set{IsReadOnly,RevertTextOnEscape,ClearKeyboardFocusOnCommit,SelectAllTextWhenFocused,HintText,Text}`, `UTextBlock::{SetFontSize,SetAutoWrapText,SetColorAndOpacity}`, `UContentWidget::SetContent`, `U{Vertical,Horizontal}BoxSlot::{SetPadding,SetSize,Set*Alignment}`, `FSlateChildSize(ESlateSizeRule::Type)`, `USpacer`, `UBorder::{SetBrushColor,SetPadding,Set*Alignment}`, `UWidget::SetKeyboardFocus`, `FSlateApplication::SetAllUserFocusToGameViewport`, `TArray::RemoveAt(Index, Count, EAllowShrinking::No)`. Constructors were read too — a code-constructed `UTextBlock`/`UEditableTextBox`/`UButton` each carries a valid default style/font, which is why no font asset is resolved anywhere.
- **The three traps of the code-authored pattern are handled, and the third is NEW** (the probe's two are inherited with citations):
  1. `ConstructConsoleTree()` runs **before** `Super::RebuildWidget()` — building after yields a **silently EMPTY widget that still passes every readback** (`Widget.cpp:993` / TASK-411 §4.1).
  2. `RootPanel` is explicitly `SelfHitTestInvisible` — a code-authored `UVerticalBox` defaults to hit-testable `Visible` and, filling the screen, would swallow **every** click.
  3. ⚠️ **NEW, found in this task and worth carrying forward: the child wiring CANNOT live in `NativeOnInitialized`.** The shipped `USessionMenuWidget` idiom wires there and is right *for a WBP* (`Initialize()` resolves `BindWidget` members at `UserWidget.cpp:168`, then calls `NativeOnInitialized` at `:175`). But a code-authored tree is not built until `RebuildWidget()` at `TakeWidget()` (`Widget.cpp:993`) — **long after** — so in `NativeOnInitialized` every child is still null and every binding silently no-ops: no error, no log, a console whose Enter and buttons do nothing. The wiring lives in `NativeConstruct` (`UserWidget.cpp:1234`), which is correct for **both** routes. I deliberately did **not** override `NativeOnInitialized` at all, and the header says why so nobody "restores" the shipped idiom. **This is a genuine hazard for the settings widget too (TASK-437), which is code-authored under its own ruling — worth a QA cross-check there.**

---

## 7. THE TASK-448 CHECKLIST (human eyes — nothing on-screen is claimed verified here)

1. The console **appears at the bottom of the screen** when opened, and is **absent** — not merely transparent — when closed.
2. **Text is legible over the battlefield** (dark plate behind it), at both the transcript and status sizes.
3. **Typing does not move the hero**: type `wasd send footmen` with the box focused; the hero must not walk. (This is bar #6's own acceptance sentence, now checked in the shipping console instead of a probe.)
4. **The camera still looks around while the console is open** (`GameAndUI`, not `UIOnly`).
5. **Every keyboard command still works identically with the console closed** — Jonathan's own standing criterion, and the check §2 exists for.
6. **Escape still cancels placement/targeting/group-pick while the console is open** (it is deliberately not absorbed), and **Escape does not close the console** — confirm that reads acceptably or say it should change.
7. **Enter in the box submits and the caret stays in the box** (focus is held across commit); a second sentence can be typed without re-clicking.
8. **Right-click still reaches the game** while the console is open, except when clicking directly on the box or a button.
9. **Accept/Cancel appear only when a confirm prompt is up**, and Cancel with no prompt dismisses the console.
10. `MaxTranscriptLines = 6` — **is that the right amount of history over live combat?** Feel-pass item.

---

## 8. What QA should scrutinise hardest

- **The seam in §2b against TASK-442/443.** Nothing in the pinned registries pins this widget's *methods* — only its child names — so the console/component contract is **reconciled at the TASK-446 gate, by reading, not by a compiler**. If TASK-443 calls something I did not declare, it surfaces there or at TASK-447's link, and that is the cheapest place to catch it.
- **Flag (a): the console cannot be opened by anything in the wave.** I consider this the most consequential thing in this handoff.
- **The `SlateCore` hazard in §4** — a plausible "cleanup" of a stale comment breaks the link.
- **That I really did not interpret the `uint8`** (§2c item 1) and really did not add a second `SetInputMode` owner (§3 item 4).
- **The `NativeConstruct` wiring argument in §6** — if my reading of the `Initialize()`/`RebuildWidget()` order is wrong, the buttons are dead and no test would say so.
- **That no §3 violation crept in:** no player-facing sentence in this file describes an order, and no model output path reaches the UI except through the component's template calls.

---

# 9. ⛔ APPENDED 2026-08-03 — QA LOOP 1 FIX (TASK-447 compile gate, 7 errors). **APPENDED, NOT OVERWRITTEN; §1–§8 above are unaltered.**

**Agent:** gameplay-programmer · **Status on completion:** `ready-for-qa` · **Loop 1 of 3.**
**Read first:** `qa/TASK-446.md` (build-master appendix) · `handoffs/TASK-447-buildmaster.md` · CONVENTIONS **§22** (broadened form) · **§18c**.

> ⛔ **WHAT THIS FIX IS AND IS NOT.** The gate found **compile** defects, not **review** defects. `qa/TASK-446.md`'s
> source-review PASS on this task is untouched by it, and **nothing QA ratified was disturbed** — see §9.5.
> ⛔ **I did not compile, run the editor, touch MCP/PIE, or touch Git.** TASK-447 is the single gate and it re-runs.

## 9.1 The two shapes, and why they are two instances of ONE trap

Both `C2445` sites are the **same defect**: a template pointer wrapper that is implicitly convertible **in both
directions**, placed in a conditional against the raw pointer it wraps. **The compiler is not being fussy — it
genuinely has two equally good common types and the standard requires it to refuse.** Verified at the engine source
rather than from memory:

| wrapper | wrapper ⟵ raw (implicit ctor) | wrapper ⟶ raw (implicit conversion) |
|---|---|---|
| `TSubclassOf<T>` | `TSubclassOf(UClass*)` — `SubclassOf.h:33` | `operator UClass*()` — `:115` |
| `TObjectPtr<T>` | `TObjectPtr(const U&)` — `ObjectPtr.h:594` | `operator T*()` — `:722` |

⇒ **The fix is the same both times: collapse BOTH arms to the raw pointer with `.Get()`**, leaving exactly one
conversion (the assignment back into the wrapper).

- ⛔ **`.Get()` IS NOT A BEHAVIOUR CHANGE, AND I CHECKED RATHER THAN ASSUMED IT.** `TSubclassOf::operator UClass*()`
  and `TSubclassOf::Get()` are **literally the same call** — both are `return **this` (`SubclassOf.h:104/115`), and
  `operator*()` performs the `IsChildOf(T::StaticClass())` check. So the truthiness test `ConsoleClass ?` and the
  taken arm `ConsoleClass.Get()` **cannot disagree**, including on a wrong-class pointer, which both map to null.
- ✅ **`ObjectPtr.h:728`'s `operator U*()` is `explicit`** — which is why `AddChildToVerticalBox(TranscriptText)` and
  the other TObjectPtr-into-`UWidget*` argument passes are **not** this shape: they have a single viable path
  (`operator T*()` + a standard derived-to-base conversion). **Checked, because a sweep that assumes is not a sweep.**

The five `C4458` sites are one shape too: `USiegeAssistantConsoleWidget` → `UUserWidget` → `UWidget`, and
**`UWidget` declares `TObjectPtr<UPanelSlot> Slot` at `Widget.h:264`** — so `Slot` is an **inherited member name**,
not a free one, and any local of that name in a member function shadows it. ⚠️ **Normally a warning; this module
builds warnings-as-errors, so it is fatal.**

📌 **The most useful fact about the five, and it shaped the fix:** this file **already shipped the correct idiom in
three places** — `SpacerSlot`, `BackdropSlot`, `RowSlot`, none of which errored. The five failures are **drift off
the file's own convention**, so I **conformed them to it rather than inventing a name**:
`TranscriptSlot` · `StatusSlot` · `InputSlot` · `ConfirmSlot` · `CancelSlot`.

## 9.2 ⛔ THE §22 SWEEP — **THE RESULT, WHICH IS THE POINT, NOT THE PATCH**

*A finding names where the compiler stopped, never where the defect is.* I swept the whole file for **both** shapes
instead of patching the seven cited lines. ⚠️ **Verified by symbol (§18c), not by the reported line numbers** — the
file has since shifted ~30 lines from my own comment additions, and every number below is re-derived at the current file.

### (a) The conditional-expression sweep — **9 conditionals, ALL classified. The 2 cited are the only ones.**

| current line | arms | verdict |
|---|---|---|
| **`:83`** | `TSubclassOf<…>` vs `UClass*` | ⛔ **AMBIGUOUS — FIXED** |
| **`:226`** | `UVerticalBox*` vs `TObjectPtr<UVerticalBox>` | ⛔ **AMBIGUOUS — FIXED** |
| `:361`–`:362` (×6) | `int` vs `int` — the arms are `1 : 0`; `TObjectPtr != nullptr` is a **comparison**, resolved by the exact-match nullptr overload (`ObjectPtr.h:676` `UEOpEquals(TYPE_OF_NULLPTR)`) | ✅ clean — ⚠️ **NOT the same shape**, and worth saying so because it *looks* like it |
| `:535` | `const TCHAR[N]` vs `const TCHAR[M]` — both decay to `const TCHAR*` | ✅ clean |
| `:686` | `FText` vs `FText` | ✅ clean |
| `:808` | `const TCHAR[19]` vs `const TCHAR*` (`FString::operator*`) | ✅ clean |
| `:828` | `ESlateVisibility` vs `ESlateVisibility` | ✅ clean |
| `:838` | `ESlateVisibility` vs `ESlateVisibility` | ✅ clean |

⇒ ✅ **SWEEP RESULT: no third instance of the ambiguous-ternary shape exists in either file.**

### (b) The member-shadowing sweep — **scripted, whole-hierarchy, with a POSITIVE CONTROL**

⛔ **I did not eyeball this, and I did not trust a bare zero.** I built the inherited data-member name set from
`Widget.h` + `UserWidget.h` + `Visual.h` + `Object.h` + `UObjectBase.h` + `UObjectBaseUtility.h` (**163 names**),
extracted every identifier I declare as a local or parameter in the `.cpp` (**123 names**), and intersected them.

- ✅ **POSITIVE CONTROL FIRST, per the pattern `qa/TASK-446.md` §5 ratified as mandatory for every future
  "nothing references X" claim:** before trusting the zero, I proved the probe **could** find this defect class —
  `Slot` **is** present in the 163-name inherited set. **The instrument works.**
- ✅ **Intersection after the fix: `Slot` is GONE from my declared set, and NO other name collides.** The three raw
  hits the script reported (`Result`, `nullptr`, `this`) are **regex artifacts, triaged individually, not waved off**:
  `Result` occurs **only inside a comment** (the prose `return Result;` in the RebuildWidget-ordering explanation)
  and is not a declaration; `nullptr` and `this` are keywords.
- ✅ **Near-misses checked deliberately, because they are the ones that would bite next:** `UUserWidget::Padding`,
  `::Priority`, `::WidgetTree`, `UWidget::Visibility`, `::Clipping`, `::Navigation`, `::Cursor` all exist as
  inherited members — **I declare no local by any of those names** (my closest is `ButtonVisibility`, which is
  distinct). ⛔ **`Visibility` and `Padding` are the two a later edit is most likely to reach for; they are the
  reason the comment at the first rename site is written as a class-wide prohibition, not a note about one line.**

⇒ ✅ **SWEEP RESULT: the five cited sites were the complete extent of the shadowing shape in both files. Nothing
further was found — and per §22 that is reported as a result, not as silence.**

## 9.3 Files touched — **two edits of substance, five renames, nothing else**

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` | `.Get()` on both ambiguous ternary arms (`:83`, `:226`); 5 × `Slot` → `<Purpose>Slot`; 3 explanatory comment blocks |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h` | ⛔ **UNTOUCHED — no diagnostic named it and it needed nothing.** |

⛔ **No other file in the repo was opened for edit.** No `Content/`, no `Build.cs`, no controller, no component, no
`.ini`, no `.uproject`. **Zero behaviour change is intended and none is introduced:** every edit is a type-resolution
collapse or an identifier rename with no semantic content. **The tree, the slot padding, the alignments, the
visibilities, the delegate wiring and every log string are byte-identical in effect.**

## 9.4 ⚠️ WHAT I CANNOT VERIFY WITHOUT A COMPILE — SAID PLAINLY, NOT AS A FOOTNOTE

1. ⛔ **I do NOT claim this TU now compiles.** A failing TU reports only the errors it reached; **5 of 23 build
   actions never started**, and `handoffs/TASK-447-buildmaster.md` §4.2 explicitly declines the claim
   *"fix these 8 and it builds."* **I decline it too.** These 7 diagnostics are addressed; **whether an eighth
   exists behind them is unknown and unknowable from here.**
2. ⛔ **THE LINK STEP HAS STILL NEVER RUN.** Unresolved externals remain **unproven in either direction**. Nothing in
   this fix adds or removes a symbol, so it should not move that needle — ⚠️ **but "should not" is a reading.**
3. ⚠️ **One reading I want QA to check rather than take from me** (it is the only place I reason about a compiler
   rather than read a header): MSVC's fatal error cap is 100, and the reported diagnostics span `:72`→`:322` in the
   pre-edit file, so the compiler **did** parse past the last cited site. ⇒ the conditionals at old `:333`/`:507`/
   `:657`/`:780`/`:800`/`:810` were **seen and produced no diagnostic**, which corroborates my (a) sweep from the
   build itself. ⛔ **I did not re-run the compiler to confirm this, and it is inference from a log, not a measurement.**
4. ⛔ **NOTHING ON SCREEN IS VERIFIED — §0 above stands unchanged.** These edits do not touch layout, and the
   TASK-448 pixel checklist in §7 is unaffected and still owed.

## 9.5 ⛔ THE DO-NOT-DISTURB LIST — **EACH ONE CHECKED AT THE FILE AFTER MY EDITS, NOT ASSUMED**

| ratified item | state now |
|---|---|
| Tree built **BEFORE** `Super::RebuildWidget()` | ✅ **UNCHANGED** — `ConstructConsoleTree(); return Super::RebuildWidget();` still adjacent, still in that order, ⛔ **do-not-reorder** comment intact |
| Children wire in **`NativeConstruct`**, never `NativeOnInitialized` | ✅ **UNCHANGED** — `WireChildWidgets()` still called from `NativeConstruct`; **still no `NativeOnInitialized` override anywhere in the class** |
| The input-probe deletion + zero surviving references | ✅ **UNCHANGED** — I added no `#include`, no symbol, no console command |
| **`ToggleConsole()` deliberately never called** | ✅ **UNCHANGED and NOT "restored"** — it is still defined, still uncalled, and I added no caller |
| §6 ruling A citation for the code-authored tree | ✅ **UNCHANGED** — every ruling-A comment and all six `BindWidgetOptional` pins are as they were |
| The four declared departures (i-1…i-4) | ✅ **UNCHANGED** — none is implicated by a type collapse or a rename |

## 9.6 What QA should scrutinise hardest **in this loop**

- ⛔ **The sweep results in §9.2, not the seven patches.** If either sweep is wrong, the fix is incomplete in exactly
  the way §22 exists to catch. **The falsifiable claims are: 9 conditionals, 2 ambiguous; 163 × 123 intersection,
  one collision.** Both are re-runnable.
- **That `.Get()` really is behaviour-neutral on the `TSubclassOf` arm** (§9.1) — the one place a "compile fix" could
  have quietly changed a runtime check. The `IsChildOf` argument is the load-bearing half.
- **That the renames are renames.** Five `if (T* X = …)` blocks; every use of the old name inside each block moved
  with it, and **no `Slot` identifier survives in either file** (the single remaining textual occurrence is inside
  my new prohibition comment).
- ⚠️ **That I claimed no compile and no link.** §9.4 is the part I would most like held to.
