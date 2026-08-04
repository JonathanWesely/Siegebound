# TASK-505 — [CC-1] `Enter` on an EMPTY input box closes the console

**Agent:** gameplay-programmer · **Date:** 2026-08-03 · **Status on completion:** `ready-for-qa` (gate **TASK-506**)
**Law read first:** TASKBOARD `## CONSOLE-CLOSE` (all 4 manager rulings) · CONVENTIONS `AS-§6` **RULING A + RULING A-2** · `AS-§2` · `handoffs/TASK-444-programmer.md` (incl. its §9 QA-loop appendix).

> **M8 DECLARATION DUTY, verbatim: adds no replicated property, no new replicated class, no new relevancy tier.**
> Why it is true rather than asserted: the change adds one `double` and one `float` to a **client-local `UUserWidget`** that has no authority, no RPC and no `Replicated` UPROPERTY. Neither new member is a UPROPERTY that replicates; `ReopenSuppressionSeconds` is `EditDefaultsOnly` config, `LastRoute4CloseRealTimeSeconds` is transient local state.

---

## 0. ⛔ THE SENTENCE QA MUST NOT LET ME SOFTEN

**I did not compile, did not open the editor, ran no MCP call and no PIE, and touched no Git.** TASK-507 is the compile gate. **Nothing about on-screen behaviour is verified here** — `AS-§6` A(e) closes on Jonathan's eyes, and §6 below is the 3-line check.

⚠️ **And the thing I most want held to: the ordering this fix covers is STILL UNMEASURED.** I did not measure it and I could not — measuring it needs PIE. I built for both worlds instead. See §3.

---

## 1. Files touched — **ONE PAIR, and that is the whole diff**

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h` | 4 hunks: close-route enumeration (comment) · `HandleTextCommitted` doc (comment) · **NEW** `ReopenSuppressionSeconds` · **NEW** `LastRoute4CloseRealTimeSeconds` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` | 4 hunks: `#include "HAL/PlatformTime.h"` · **NEW** `MaxReopenSuppressionSeconds` constant · suppression guard in `OpenConsole()` · **route 4** in `HandleTextCommitted()` |

**`git diff --stat`: 2 files, 219 insertions, 3 deletions.** ⛔ **All 3 deletions are COMMENT lines** (2 in the Escape bullet I expanded, 1 the one-line `HandleTextCommitted` doc I expanded) — **zero executable lines were removed.** Verified by `git diff | grep -E "^-[^-]"`.

⛔ **No controller edit, no component edit, no `Content/`, no `.ini`, no `Build.cs`, no Git, no compile, no editor/MCP/PIE.** `SiegePlayerController.cpp` was **opened READ-ONLY and cited**, never touched.

⚠️ **The working tree also carries dirt that is NOT MINE and that I did not disturb:** `SiegeLlamaSpike.cpp`, `SiegeAssistantComponent.{h,cpp}`, `SiegeCheatManager.{h,cpp}` (the TASK-481 Stage-A hold and the TASK-479 lane), plus `CONVENTIONS.md` / `TASKBOARD.md`. **The hunk map proves the console pair contains exactly my 8 edits and nothing pre-existing.**

---

## 2. The change

### 2a. The trigger — `HandleTextCommitted`, **not** `SubmitPressed` (ruling 1)

After the existing, unmoved `CommitMethod != ETextCommit::OnEnter` filter:

- Trim a **copy** of the committed text; if empty ⇒ arm the window, log one line, `CloseConsole()`, return.
- Non-empty ⇒ `SubmitPressed(CommittedText.ToString())` — **the RAW, untrimmed text, exactly as before.** The trimmed copy exists **only** to answer "is the box empty". `SubmitPressed` still does its own trimming.

⛔ **`SubmitPressed` is BYTE-UNCHANGED**, including its empty-string branch (clear · re-focus · no broadcast · no model call). Proven mechanically: `git diff` over the `.cpp` returns **only comment lines** matching `SubmitPressed|CancelPressed|OnConsoleSubmitted|Broadcast|SetConsoleEnabled`.

⛔ **The close is NOT gated on `bConsoleEnabled`** — deliberately. A close that can be refused is a close that can strand the cursor, which is the same reason A-2 pins the controller toggle's close half un-gated.

### 2b. The re-open suppression window (ruling 2)

`OpenConsole()` gains a guard **after** the two existing guards (so the disabled-refusal log and the already-open re-focus path are untouched):

```cpp
if (LastRoute4CloseRealTimeSeconds >= 0.0)
{
    const double ElapsedSinceClose = FPlatformTime::Seconds() - LastRoute4CloseRealTimeSeconds;
    const double SuppressionWindow = static_cast<double>(
        FMath::Clamp(ReopenSuppressionSeconds, 0.0f, SiegeAssistantConsole::MaxReopenSuppressionSeconds));

    LastRoute4CloseRealTimeSeconds = -1.0;   // ⛔ DISARM FIRST, DECIDE SECOND

    if (ElapsedSinceClose < SuppressionWindow) { UE_LOG(...one line...); return; }
}
```

**Default window 0.12 s** (spec's 0.05–0.15 band). **Only route 4 arms it** — routes 1/2/3 are byte-unchanged, because only route 4 can have a re-open racing the same keypress.

**⛔ THE WINDOW CANNOT STICK. It is bounded TWO INDEPENDENT WAYS and either bound alone ends it:**
1. **TIME** — a monotonic clock (below).
2. **ONE SHOT** — the stamp is **consumed by the first open attempt after the close, whatever the verdict**. At most **one** open is ever suppressed per close. Even a clock that misbehaved could not produce a console that refuses to open twice. Disarming *before* the branch is what makes this structural rather than a promise.

**Plus a third bound QA should note: the window is HARD-CLAMPED IN CODE** to `MaxReopenSuppressionSeconds = 0.25f` on every read — not merely a `ClampMax` in the details panel. **No `.ini`, Blueprint default or future editing hand can configure a long window.** Setting it to `0` is a clean off switch (`Elapsed < 0.0` is never true).

### 2c. ⭐ Two engine facts I checked rather than remembered, and **both changed the code**

1. ⛔ **`CoreMinimal.h` does NOT include `HAL/PlatformTime.h`** — grepped at the UE 5.8 source on this machine: **zero hits**. Without the explicit include I added, `FPlatformTime::Seconds()` would not resolve. **This would have been a compile error.**
2. ⛔ **`FWindowsPlatformTime::GetSecondsTimeOffset()` returns `16777216.0` == 2^24** (`WindowsPlatformTime.h:48`), and the engine states the purpose at `:28`: *"add big number to make bugs apparent where return value is being passed to float"*. **A `float` has a 24-bit mantissa, so at 2^24 its ULP is exactly 1.0 SECOND** — a `float` timestamp would quantize to whole seconds and turn a 0.12 s window into a 0-or-1 s window. ⇒ **`LastRoute4CloseRealTimeSeconds` is a `double`, and the header says why so nobody "tidies" it to `float`.** The *duration* (`ReopenSuppressionSeconds`) is correctly a `float`; only the *timestamp* must be a `double`.

**Why this clock and not the world's:** `FPlatformTime::Seconds()` is QueryPerformanceCounter-based (`WindowsPlatformTime.h:23`) — monotonic, never paused, never time-dilated, never reset by level travel, and **needs no `UWorld`**. ⛔ **`UWorld::GetTimeSeconds()` has none of those properties: it STOPS while the game is paused**, so a window armed just before a pause would **never expire and the console would refuse to open forever** — the exact defect class TASK-506 item (3) is told to hunt. `GetRealTimeSeconds()` still needs a non-null world and resets across travel.

### 2d. ✅ The controller rollback — **QUOTED, not asserted** (spec (2) demanded this)

Verified **by symbol** (`ASiegePlayerController::OnAssistantConsolePressed`, §18c), currently at `SiegePlayerController.cpp:4365`:

```cpp
if (!Console->IsConsoleOpen())
{
    SetAssistantConsoleOpen(false);
```

⇒ **A suppressed re-open cannot strand the cursor in `GameAndUI`.** The controller re-reads the widget after `OpenConsole()` and releases the posture it took. **That rollback is the property this guard depends on; the code comment says the two must change together if it is ever removed.**

I also verified the corroborating toggle at `:4270` — `if (bAssistantConsoleOpen || (AssistantConsoleWidget && AssistantConsoleWidget->IsConsoleOpen())) { … CloseConsole(); … return; }` — **close-first and un-gated**, exactly as A-2 describes.

---

## 3. ⛔ THE ORDERING I RELIED ON, AND WHAT HAPPENS UNDER THE OTHER (ruling 2 / spec (2))

**I RELIED ON NEITHER. The fix is built so the player sees the same thing in both, and that was the design goal rather than a hedge.** What I relied on is the *rollback quoted in §2d*, which is measured code, not an ordering.

| | **WORLD A** — Slate consumes `Enter`; Enhanced Input never sees it | **WORLD B (B1)** — both receive it, **Slate first** | **WORLD B (B2)** — both receive it, **Enhanced Input first** |
|---|---|---|---|
| Empty box + `Enter` | Route 4 closes. Posture released via the broadcast. **Guard never fires.** | Route 4 closes (stamp armed) → controller takes its OPEN branch → `OpenConsole()` **refuses** → `:4365` rolls the posture back. | Controller's close branch already closed it → then route 4 finds `bConsoleOpen == false`, **does not stamp**, `CloseConsole()` early-returns. |
| **What the player sees** | ✅ **Console closes, cursor back to the game.** | ✅ **Console closes, cursor back to the game — IDENTICAL.** | ✅ **Console closes.** |
| Flicker? | n/a | ⛔ **No.** `OpenConsole()` refuses **before** `ApplyConsoleVisualState()`, so visibility is never set back — the refusal is same-frame, pre-paint. | n/a |

⭐ **Which world is real:** Jonathan's report is **evidence for A** — the controller toggle at `:4270` is already close-first and un-gated, so **if Enhanced Input were receiving `Enter` while the box held focus, the console would already have been closing on him.** His complaint that it does not is therefore evidence it is not. ⚠️ **That is CORROBORATION, NOT MEASUREMENT, and I am recording it as corroboration.**

✅ **AND THE FIX SHIPS ITS OWN MEASUREMENT.** The suppression log line **can only be reached in World B**. If it never appears in a log, `Enter` is not reaching Enhanced Input while the box has focus — that settles the question TASK-445 could only reason about.

### ⚠️ 3a. WHAT MY WINDOW DOES **NOT** COVER — a PRE-EXISTING World-B defect I found and deliberately did NOT fix

**In World B (B1), a NON-EMPTY submit would still close the console:** the player types an order and presses `Enter`; `SubmitPressed` runs and the console stays open; then Enhanced Input's toggle finds it **open** and takes its **close** branch. **The symptom reads as *"the assistant ate my order"*.**

⛔ **This is pre-existing, my change neither creates nor worsens it, and I could not fix it inside this task's fences:**
- Suppressing it would mean **gating a CLOSE**, and A-2 pins the close half un-gated precisely because a refusable close can strand the cursor. ⛔ I will not gate a close.
- Any other fix is a `SiegePlayerController` edit, which spec item (5) makes **read-only to me**.

⇒ **It is exactly TASK-508 check (a), and that check is the right instrument.** If Jonathan sees a submit close or flicker the console, we are in World B and the *submit* case needs a separate, controller-side task. ⛔ **Do not let a World-B submit symptom be read as a regression from this fix.**

---

## 4. ⛔ PROOF NOTHING RATIFIED MOVED — each checked **at the file after my edits**, not assumed

| Ratified item (TASK-444 gate) | State now |
|---|---|
| Tree built **BEFORE** `Super::RebuildWidget()` | ✅ **UNCHANGED** — `ConstructConsoleTree(); return Super::RebuildWidget();` still adjacent, still in that order, ⛔ do-not-reorder comment intact |
| Children wired in **`NativeConstruct`**, never `NativeOnInitialized` | ✅ **UNCHANGED** — `WireChildWidgets()` still called from `NativeConstruct`; **still no `NativeOnInitialized` override anywhere** (grep: comments only) |
| **`ToggleConsole()` deliberately UNCALLED** | ✅ **UNCHANGED and NOT "restored"** — still defined, still declared, **still zero call sites**; I added none |
| **Slate focus only, NEVER `SetInputMode`** | ✅ **UNCHANGED** — grep finds `SetInputMode` in **comments only**, in both files |
| ⛔ **`Escape` STAYS UNABSORBED** | ✅ **UNCHANGED** — `SetRevertTextOnEscape(false)` untouched, **no `NativeOnKeyDown` override, no `EKeys::Escape` handling added.** ⛔ **I did not take Jonathan's open ruling.** I *strengthened the comment* to say it may not be taken |
| **A close is not a cancel** | ✅ `CloseConsole()` **reused verbatim**, **no new broadcast on the close path** |
| `SubmitPressed` empty-string branch | ✅ **BYTE-UNCHANGED** (mechanical grep over the diff) |
| Code-authored tree / `BindWidgetOptional` pins / ruling-A comments | ✅ **UNCHANGED** — no hunk touches them |

### 4a. The shadowing sweep, **with a positive control first** (§14 — this file lost 5 build errors to `Slot` at TASK-447)

- ✅ **POSITIVE CONTROL RUN BEFORE TRUSTING THE ZERO:** the probe was pointed at the inherited member set (`Widget.h`, `UserWidget.h`, `Visual.h`, `Object.h`, `UObjectBase.h`, `UObjectBaseUtility.h`) and **found the known collision `Slot`** (`TObjectPtr<UPanelSlot> Slot;`), plus `Cursor`, `Visibility`, `Padding`. **The instrument works.**
- ✅ **All six new identifiers return 0 inherited hits:** `ElapsedSinceClose` · `SuppressionWindow` · `CommittedTrimmed` · `LastRoute4CloseRealTimeSeconds` · `ReopenSuppressionSeconds` · `MaxReopenSuppressionSeconds`. **No shadowing.** (Reported as a result, not as silence — §22.)

### 4b. The ambiguous-ternary shape (the other TASK-447 defect class)

✅ **I introduced no conditional expression at all.** My two `if` statements have no ternary; the only wrapper types in scope are untouched. **`FMath::Clamp(const float, const float, const float)` is a NON-TEMPLATE exact-match overload** (`UnrealMathUtility.h:600`), so my three-`float` call performs **no template deduction** and cannot reproduce the C2445 shape. Checked at the engine source.

---

## 5. ⚠️ WHAT I CANNOT VERIFY WITHOUT A COMPILE — plainly, not as a footnote

1. ⛔ **I do NOT claim this compiles.** UHT has never seen the new `UPROPERTY(EditDefaultsOnly, meta=(ClampMin/ClampMax/UIMin/UIMax)) float`. Every type is `float`/`double`/`FString` and the meta keys are standard, **but that is a reading, not a build.**
2. ⚠️ **Warnings-as-errors is live in this module.** I believe the diff is warning-free (no shadowing per §4a, no unused locals, `%.0f` fed a `double`), **but only TASK-507 can say so.**
3. ⛔ **Nothing on screen is verified.** No PIE, no pixels. The ordering in §3 is **traced from source, never observed**.
4. ⚠️ **The `%.0f` format token with a `double`** is the one line I would most like QA to eyeball — it is correct by printf promotion rules, and it is the only formatted log I added.

---

## 6. THE TASK-508 CHECK — three lines, for a man reading it standing up

1. **Open the console, type nothing, press `Enter` ⇒ it closes**, the cursor returns to the game, every key works.
2. **Open it again, type a real order, press `Enter` ⇒ it still submits**, the caret stays in the box, and ⚠️ **the console must NOT close or flicker** (if it does, say so — that is §3a, not this fix).
3. **Re-open it immediately after a close ⇒ it opens.** (If it ever refuses to re-open, that is the window, and it is one clamped number.)

---

## 7. What QA should scrutinise hardest

- **§3's both-worlds table, not the patch.** If the B1 trace is wrong, the fix is wrong in exactly the way ruling 2 exists to catch. The falsifiable claim: **a refused `OpenConsole()` returns before `ApplyConsoleVisualState()`, so nothing paints.**
- **The disarm-before-decide ordering in §2b.** It is what makes "the window cannot stick" structural. Move it below the `if` and the one-shot bound is gone.
- **§2c item 2 — the `double`-not-`float` argument.** It is the one place a "tidy-up" would silently destroy the window while still compiling.
- **§2d — that I QUOTED the rollback rather than asserting it** (the relayed-diagnosis law; this seam has broken it once already).
- ⚠️ **§3a — the World-B *submit* case I did NOT fix.** I would rather be told I was wrong to leave it than have it discovered in play.
- **That `Escape` is still unabsorbed** and I only hardened the comment around Jonathan's open ruling.

## 8. Flagged, not silently decided

**(a) I did NOT change the hint text** (`"Type an order, then press Enter"`), which no longer mentions that an empty `Enter` closes. Discoverability is a **player-facing wording decision** and §3 governs authored chrome — **it is Jonathan's, not mine.** Offered, not taken; it is a one-string change whenever he wants it.
**(b) `ReopenSuppressionSeconds` is `EditDefaultsOnly` on purpose.** Because the ordering is unmeasured, if 0.12 s turns out wrong he can tune it **without a code task or a compile** — and the hard clamp means he cannot tune it into a soft-lock.
