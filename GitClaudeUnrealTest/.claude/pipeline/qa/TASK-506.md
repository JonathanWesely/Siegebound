# QA Report — TASK-506 (gate on TASK-505, empty-`Enter` closes the console)

**Verdict: PASS — 0 BLOCKERS · 4 WARN · 3 NIT**
Reviewer: qa-reviewer · 2026-08-03 · read-only, no code edited, no Git, no engine.

⛔ **THE LIMIT, FIRST AND UNSOFTENED: NOTHING HERE IS COMPILED AND NOTHING HAS BEEN SEEN ON SCREEN.**
This PASS says the change is **correct as source** against UE 5.8 semantics I re-derived at the engine
files. It does **not** say it builds (TASK-507) and it does **not** say it works (TASK-508, `AS-§6` A(e) —
Jonathan's eyes). Every trace below is read from source; **no ordering was measured, by me or by anyone.**

Files under review (the whole diff, and it is one pair):
`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h`
`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp`

---

## 1. THE CENTRAL CLAIM — "I relied on neither world" — **VERIFIED, PER WORLD, AT THE SYMBOL**

I traced all three orderings myself against the current files. The claim holds.

| | trace I verified | result |
|---|---|---|
| **World A** (Slate consumes; Enhanced Input never sees it) | route 4 arms (`cpp:785`) → `CloseConsole()` (`:797`) → `:613` clears `bConsoleOpen`, `:622` collapses, `:623` releases focus, `:636` broadcasts → controller `HandleAssistantConsoleOpenChanged(false)` → `SetAssistantConsoleOpen(false)` (`SiegePlayerController.cpp:4386`) → `ApplyCursorInputState()` → `FInputModeGameOnly` | ✅ closes, cursor returns, **guard never fires** |
| **World B1** (both see it, Slate first) | as above, then `OnAssistantConsolePressed` (`:4261`) finds both halves false → takes the OPEN branch → `SetAssistantConsoleOpen(true)` (`:4298`) → `AttachConsoleWidget` (`:4348`) → `Console->OpenConsole()` (`:4357`) → **guard refuses** (`cpp:578-587`) → `:4365 if (!Console->IsConsoleOpen()) { SetAssistantConsoleOpen(false); }` | ✅ closes, posture rolled back — **same thing on screen** |
| **World B2** (both see it, Enhanced Input first) | controller close branch (`:4270-4283`) already closed it → route 4 hits `if (!bConsoleOpen) return;` (`cpp:771-779`) — **no stamp armed**, and `CloseConsole()` would early-return anyway (`:608-611`) | ✅ closes; window not armed, so no later open is stolen |

### ⛔ THE NO-FLICKER ARGUMENT — **CONFIRMED, AND IT IS STRONGER THAN 505 CLAIMED**
- **Widget side (505's claim):** the refusal `return` is at `cpp:586`; `ApplyConsoleVisualState()` is at `:597`
  and `FocusInputBox()` at `:598`. ✅ **The refusal returns before BOTH** — so on a suppressed re-open the
  visibility is never re-set **and keyboard focus is never stolen back into a collapsed box.** Nothing paints.
- **Controller side (505 did not state this, and it is the half that could actually flicker):**
  `SetAssistantConsoleOpen(true)` at `:4298` **does** call `ApplyCursorInputState()` → `FInputModeGameAndUI` +
  `bShowMouseCursor = true` (`:4178-4190`). ✅ **But the rollback at `:4367` executes inside the SAME
  synchronous `OnAssistantConsolePressed()` call** — there is no tick and no paint between them, so the
  transient posture is never presented. **No cursor flicker either.** The rollback is quoted correctly and
  sits exactly where the handoff said (`:4365-4367`).

### 📌 THE REACHABILITY CLAIM — **NOT AIRTIGHT. See WARN-1.**
`OpenConsole()` has exactly **one** C++ caller (`SiegePlayerController.cpp:4357`; `ToggleConsole()` is
uncalled), so the guard is only reachable from the open key. That makes the *absence* of the log line sound
evidence for World A. The *presence* of it is not unconditional evidence for World B — **a HELD `Enter`
(auto-repeat) can reach it in World A.** Detail and the exact wording correction in WARN-1.

---

## 2. THE BOUNDED WINDOW — **ALL THREE BOUNDS AND THE TYPE, EACH CHECKED**

1. ✅ **MONOTONIC CLOCK.** `FPlatformTime::Seconds()` (`cpp:569`) → `FWindowsPlatformTime::Seconds()`,
   `QueryPerformanceCounter`-based (`WindowsPlatformTime.h:23-30`). Not pausable, not dilated, no `UWorld`.
   The rejection of `UWorld::GetTimeSeconds()` is correct: it stops under pause, which would make an armed
   window immortal — the exact soft-lock class this gate hunts.
2. ✅ **ONE-SHOT CONSUME, AND THE DISARM PRECEDES THE BRANCH.** `LastRoute4CloseRealTimeSeconds = -1.0;` at
   **`cpp:576`**; the deciding `if` is at **`cpp:578`**. **Disarm-before-decide confirmed** — every path out
   of the block, refusal included, leaves the window closed. At most one open is ever suppressed per close.
3. ✅ **HARD IN-CODE CLAMP ON EVERY READ.** `cpp:570-571` clamps to
   `SiegeAssistantConsole::MaxReopenSuppressionSeconds = 0.25f` (`cpp:54`), applied at the use site — not a
   details-panel `ClampMax`. No `.ini`, Blueprint default or later editing hand can exceed it.
   `0` is a clean off switch: `Elapsed < 0.0` is never true.
4. ✅ **SENTINEL IS UNAMBIGUOUS.** `-1.0` can never collide with a real stamp: `FPlatformTime::Seconds()`
   returns QPC seconds **plus 16777216.0**, so it is never negative. Guard `>= 0.0` at `cpp:567` is right.
5. ✅ **THE TYPE IS `double`** — `double LastRoute4CloseRealTimeSeconds = -1.0;` at **`.h:537`**, with the
   reason recorded in the header so it cannot be "tidied". The *duration* `ReopenSuppressionSeconds` is
   correctly a `float` (`.h:454`).
6. ✅ **RE-ENTRANCY COVERED.** The stamp is armed at `cpp:785` **before** `CloseConsole()`, so a consumer that
   re-opened synchronously from inside `OnConsoleOpenChanged.Broadcast(false)` is suppressed too.
7. ✅ **GUARD PLACEMENT AFTER THE ALREADY-OPEN EARLY-OUT IS SAFE.** `bConsoleOpen` is assigned `true` at
   exactly one site (`cpp:590`, inside `OpenConsole()` **after** the guard); the other three assignments are
   `false` (`:468`, `:487`, `:613`). A live stamp therefore cannot coexist with an open console, as the
   comment at `:513-515` claims.

### ⭐ THE TWO ENGINE FACTS — **RE-DERIVED AT THE ENGINE SOURCE, NOT ACCEPTED**
1. ✅ **`CoreMinimal.h` does NOT include `HAL/PlatformTime.h`.** I read the whole file
   (`UE_5.8/Engine/Source/Runtime/Core/Public/CoreMinimal.h`, 155 lines) — the include list carries
   `HAL/PlatformCrt.h`, `PlatformMisc.h`, `PlatformMemory.h`, `PlatformAtomics.h`, `PlatformMath.h`,
   `PlatformString.h`, `PlatformProperties.h`, `PlatformTLS.h` — **and no `PlatformTime.h`.**
   ⇒ **`#include "HAL/PlatformTime.h"` at `cpp:24` is load-bearing; without it this would not compile.**
   Header exists at that path in 5.8 (verified). Placement is alphabetical with the file's existing block. ✅
2. ✅ **`FWindowsPlatformTime::GetSecondsTimeOffset()` returns `16777216.0` (= 2²⁴)** —
   `WindowsPlatformTime.h:46-49`, with the engine's stated purpose at `:28` verbatim: *"add big number to
   make bugs apparent where return value is being passed to float"*. The conclusion (**the timestamp must be
   a `double`**) is correct and, if anything, understated — see NIT-1.

---

## 3. THE REFUSAL TO FIX THE WORLD-B *SUBMIT* CASE — ⚖️ **RULING: THE REFUSAL IS CORRECT**

505 found that in World B a **non-empty** submit would still be closed by the controller's toggle, declared
it, and routed it to TASK-508 check (a) instead of reaching outside its file. **I rule that correct**, and
both alternatives it rejected are genuinely barred:

- **Gating a close** is forbidden by `AS-§6` **A-2** in as many words (*"a close that can be refused can
  strand the cursor"*), and the controller pins the same property twice (`SiegePlayerController.cpp:4221-4222`,
  `:4263-4267`). A refusable close is the one defect class this seam must never be able to produce.
- **A controller edit** is barred by TASK-505 spec item (5) (`SiegePlayerController.cpp` READ-ONLY to it).
- ✅ **The symptom is genuinely pre-existing.** The toggle's close-first, un-gated branch at `:4270-4283`
  predates this task and is unchanged. TASK-505 touches no controller file, adds no broadcast, and changes
  nothing on the non-empty path (`cpp:805` passes the **RAW** text to `SubmitPressed`, exactly as before).
  ⇒ **A submit that also closes is a World-B *input-routing* symptom, not a regression from this fix.**

### ⛔ BOARD LANGUAGE I REQUIRE BEFORE JONATHAN READS IT (he will see the board, not a handoff)
Add to **TASK-508 check (a)**, verbatim:
> ⚠️ **IF A TYPED ORDER SUBMITS *AND* THE CONSOLE CLOSES/FLICKERS, THAT IS A KNOWN PRE-EXISTING INPUT-ROUTING
> BEHAVIOUR (the open key's toggle also receiving the same `Enter`) — ⛔ NOT a regression from TASK-505, which
> changes nothing on the non-empty path. It is the World-B answer, it needs a separate controller-side task,
> and it does not send this fix back.**

---

## 4. ALSO CONFIRMED (each at the current file, not from the handoff)

| Check | Result |
|---|---|
| Trigger in `HandleTextCommitted`, under the `ETextCommit::OnEnter` filter only | ✅ `cpp:739-806`, filter at `:744-747`; close is **not** routed through `SubmitPressed` (ruling 1) |
| `SubmitPressed` empty-string branch intact (clear · re-focus · no broadcast · no model call) | ✅ `cpp:655-677` — see NIT-3 on the word *byte*-unchanged |
| Non-empty passes the **RAW** text, so `SubmitPressed` trims exactly as before | ✅ `cpp:805` `SubmitPressed(CommittedText.ToString())`; the trimmed copy at `:766-767` is used **only** for the emptiness test |
| Close is not a cancel (widget level) | ✅ `CloseConsole()` reused verbatim (`cpp:797`), **no new broadcast**; `CloseConsole` (`:606-637`) still fires only `OnConfirmPromptChanged(false)` + `OnConsoleOpenChanged(false)`, never `OnConsoleCancelled`. **System-level consequence: see WARN-4** |
| Not gated on `bConsoleEnabled` | ✅ deliberate and consistent with A-2 (`cpp:763-765`) |
| Fence 1 — tree built **before** `Super::RebuildWidget()` | ✅ `cpp:138-139`, do-not-reorder comment intact |
| Fence 2 — wiring in `NativeConstruct`, **no** `NativeOnInitialized` | ✅ `cpp:464`; repo-wide grep: this class has no such override (only `SessionMenuWidget`) |
| Fence 3 — `ToggleConsole()` still **uncalled** | ✅ decl `.h:239`, def `cpp:639`, **zero call sites repo-wide** |
| Fence 4 — Slate focus only, never `SetInputMode` | ✅ `SetInputMode` appears in this pair in **comments only** (`.h:113`, `cpp:1016-1018`) |
| Fence 5 — ⛔ `Escape` still unabsorbed (Jonathan's open ruling) | ✅ `SetRevertTextOnEscape(false)` at `cpp:430`; **no `NativeOnKeyDown`, no `EKeys::Escape`** anywhere in the pair. The comment at `.h:95-96` was hardened to say the ruling **may not be taken** — the ruling was **not** taken |
| One file pair | ✅ repo-wide grep for `ReopenSuppression` / `LastRoute4Close` / route-4 markers hits **only** the two widget files; `SiegePlayerController.{h,cpp}` and `SiegeAssistantComponent.{h,cpp}` carry no reference to the suppression. ⚠️ Verified by content, not by `git status` — QA has no Git by design |

### 4a. THE SHADOWING SWEEP — **RE-RUN BY ME, POSITIVE CONTROL FIRST (§14)**
- ✅ **Positive control:** probe pointed at the inherited chain found the known collision —
  `TObjectPtr<UPanelSlot> Slot;` at **`UMG/Public/Components/Widget.h:264`**. **The instrument works.**
- ✅ **Then the zero:** all six new identifiers — `ElapsedSinceClose`, `SuppressionWindow`,
  `CommittedTrimmed`, `LastRoute4CloseRealTimeSeconds`, `ReopenSuppressionSeconds`,
  `MaxReopenSuppressionSeconds` — return **zero hits across the entire `Engine/Source/Runtime` tree**, which
  is a strictly wider sweep than the inherited chain. **No shadowing; C4458 cannot fire on this diff.**

### 4b. THE OTHER TASK-447 DEFECT CLASS + the compile-shaped risks I can read
- ✅ **No ternary introduced.** `FMath::Clamp(const float, const float, const float)` is a **non-template
  exact-match overload** at `UnrealMathUtility.h:600` (I read it — the handoff's citation is exact). Three
  `float` arguments ⇒ no deduction, no C2445 shape.
- ✅ **`%.0f` fed `ElapsedSinceClose * 1000.0`** (`cpp:583-585`) — a `double` is exactly what varargs `%f`
  expects. Correct.
- ✅ **Non-ASCII inside `TEXT()` at `:584` (`§`) is not a new risk class** — this file already ships `⚠️`/`—`
  inside `TEXT()` literals (`:378`, `:954`) and linked green at `cd5f4ed` / TASK-447.
- ✅ **UHT surface:** `UPROPERTY(EditDefaultsOnly, meta=(ClampMin/ClampMax/UIMin/UIMax)) float` is standard;
  `LastRoute4CloseRealTimeSeconds` is a non-UObject `double` and needs no reflection or GC pin.
- ⚠️ **Still a reading, not a build.** TASK-507 is the only thing that can say "compiles".

---

## Findings

### BLOCKERS — **none**

### WARN
- **[WARN-1] `SiegeAssistantConsoleWidget.cpp:580-585` — the "reachable ONLY in World B" measurement claim is
  not airtight, and the log line asserts the conclusion as fact.** In **World A**, the first `Enter` is eaten
  by the focused box; route 4 closes and `ReleaseKeyboardFocusToGame()` (`:623` → `:1035`) hands focus to the
  viewport. A **held** `Enter` then delivers auto-repeat key-downs to the viewport → `UPlayerInput::InputKey`
  for a key it never saw go down → the `ETriggerEvent::Started` binding (`SiegePlayerController.cpp:482`) can
  fire → `OpenConsole()` → **the suppression line logs, in World A.** Behaviourally this is correct and
  intended (`.h:440-441` says suppressing auto-repeat is right), so **no code change is requested** — but the
  inference must be stated directionally or Jonathan's test reads backwards:
  ✅ **line NEVER appears ⇒ World A (sound).** ⚠️ **line appears ⇒ World B ONLY IF `Enter` was TAPPED, not
  held.** *Fix (docs only, no compile):* add "**tap `Enter`, do not hold it**" to TASK-508 check (a), and
  soften the board's *"reachable ONLY in World B"* to *"reachable only in World B for a tapped `Enter`"*.
  The log line's own wording (*"The same keypress reached BOTH Slate and Enhanced Input"*) should lose its
  certainty the next time this file is opened for another reason — **not worth a compile cycle on its own.**
- **[WARN-2] `SiegeAssistantConsoleWidget.h:452-454` — the handoff's flagged item (b) overstates tunability:
  today this number CANNOT be changed without a code edit and a compile.** `ReopenSuppressionSeconds` is
  `EditDefaultsOnly` on a class with **no Blueprint subclass** (`WBP_AssistantConsole` is RESERVED and
  deliberately not authored — ruling A(c)) and is **not** marked `config`. ⇒ there is no details panel to
  show it and no `.ini` key to set it. The shipped default of `0.12` is sound and nothing is broken; the
  **claim** is what is wrong. Same shape is pre-existing on `MaxTranscriptLines` (`.h:431-432`), so this is a
  house pattern, not a new mistake. *Fix:* correct the sentence on the board; if real tuning is ever wanted
  it is `UCLASS(config=Game)` + `config` on the property, or authoring the reserved WBP — either is its own
  task with its own compile.
- **[WARN-3] World A and World B are identical in the CLOSE but not in the aftermath — and the difference is
  one duplicated transcript line.** In World B the controller runs `AttachConsoleWidget`
  (`SiegePlayerController.cpp:4346-4349`) **before** the refused `OpenConsole()` (`:4357`), and attach
  re-seeds (`SiegeAssistantComponent.cpp:2277-2280`) by re-appending `LastMessage` to the transcript. ⇒ each
  empty-`Enter` close in World B silently costs one duplicated assistant line, visible at the **next** open.
  Pre-existing behaviour of the attach path (it re-seeds on every open in both worlds), **not created by
  TASK-505**, and cosmetic (transcript is capped at 6). The confirm half of the same re-seed is moot:
  `NotifyConsoleClosed()` has already cleared the pending confirm by then. ⭐ **Useful to Jonathan as a
  SECOND World-B tell:** last assistant line duplicated after an empty-`Enter` close ⇒ World B.
- **[WARN-4] `SiegeAssistantComponent.cpp:684-701` — route 4 makes empty-`Enter` a ONE-KEY DISCARD of a
  pending confirmation and a one-key abort of an in-flight turn. This is RATIFIED LAW, not a defect — and
  Jonathan should still be told in one line.** At the **widget** level 505 is fully compliant (no new
  broadcast; the FSM decides). The FSM's ratified decision on a close is: `Thinking → AbortInFlightRequest()`,
  `AwaitConfirm → ClearConfirmPreview()`, then `ClearPendingIntent()` + `Idle`; only `Deferred` survives.
  `AS-§6` A-2 ruled this explicitly (*"Empty-`Enter` closes even with a prompt up — that is the point of the
  feature; the FSM semantics are not yours to change"*), so ⛔ **I am not overturning it.** The feel risk
  worth one press of his time: with a confirm prompt up, **Accept is mouse-only** (`ConfirmButton` click —
  there is no keyboard route to Accept), so a player who presses `Enter` meaning *"yes"* gets *"discard and
  close"*. *Suggested TASK-508 addendum, one line, no code:* **"With a confirm prompt up, is `Enter`-on-empty
  = discard the right feel, or should Accept get a key?"** — his ruling, like Escape.

### NIT
- **[NIT-1] `SiegeAssistantConsoleWidget.h:524-526` — the ULP number is off by one power of two, in the
  direction that makes the code MORE right.** A `float`'s spacing is `1.0` on `[2²³, 2²⁴)`; **at and above
  2²⁴ it is `2.0` seconds**, and the engine's offset puts every real stamp at ≥ 2²⁴. So a `float` here would
  quantise to **two-second** steps, not one. **Conclusion unchanged and strengthened: the `double` at
  `.h:537` is required.** Reword whenever the file is next opened; not worth a compile.
- **[NIT-2] Frame-budget of the default window, so a future report is not misdiagnosed.** If Enhanced Input
  delivers the press a frame *later* than Slate, `0.12 s` covers the gap down to ~8 fps (the `0.25 s` ceiling
  covers ~4 fps). Below that a World-B re-open would slip the window and the console would re-open. Fine as
  shipped; noted only so "it re-opened once during a hitch" is read as the window, not as the fix failing.
- **[NIT-3] "`SubmitPressed` is BYTE-unchanged" is the programmer's mechanical claim and remains his.** QA
  has no Git by design, so I cannot produce a diff. What I verified is **behavioural and structural at the
  artifact** (`cpp:655-697`): enabled-check → trim → empty ⇒ clear + refocus + **no broadcast, no model
  call** → non-empty ⇒ echo, clear, refocus, log, `OnConsoleSubmitted.Broadcast(Trimmed)`. That matches the
  ratified description in the TASK-506 spec and TASK-444 exactly. Recorded as attribution, not as doubt.

---

## Notes for build-master (TASK-507)

1. **Two symbols are new to the compiler and both are the likely first errors if anything is wrong:**
   `FPlatformTime::Seconds()` (needs `cpp:24`'s include — verified present and verified NOT inherited from
   `CoreMinimal.h`) and the `UPROPERTY(EditDefaultsOnly, meta=(ClampMin…)) float ReopenSuppressionSeconds`
   (`.h:452-454`) which UHT has never seen. Everything else in the diff is `double`/`float`/`FString`.
2. **Warnings-as-errors:** I re-ran the shadowing sweep with a positive control and found **zero** inherited
   collisions for all six new identifiers; there is no new ternary and no unused local. I expect zero new
   warnings — **expect, not promise.**
3. ⛔ **Compile only.** No Git, no MCP, no PIE, no `SpikeEval` — and the editor closes on **Jonathan's** word,
   never yours. The uncommitted TASK-481 Stage-A source in the tree is compiled alongside **on purpose**;
   ⛔ never `checkout`/`reset`/`stash` it.
4. **If it fails to compile:** hand back to gameplay-programmer with the errors. Per ruling 3, the **console
   fix** is the thing that stops — **never Stage A.**

## Board flips I am requesting (stated, not applied — orchestrator to write)

- **TASK-506:** `backlog` → **`qa-passed`** · report `qa/TASK-506.md` · **PASS, 0 blockers, 4 WARN, 3 NIT.**
- **TASK-505:** `ready-for-qa` → **`qa-passed` / ready-for-integration (TASK-507)**.
- **TASK-507:** unblocked — needs the quiet module **and** Jonathan's consent to close the editor.
- **TASK-508:** add the two sentences above — the ⛔ pre-existing **submit-then-close** disclaimer (§3), and
  **"tap `Enter`, do not hold it"** (WARN-1). Optional third line: the Accept-has-no-key question (WARN-4).
- **CONSOLE-CLOSE section:** soften *"reachable ONLY in World B"* → *"reachable only in World B **for a
  tapped `Enter`**"* (WARN-1), and drop the *"tune without a compile"* claim (WARN-2).
