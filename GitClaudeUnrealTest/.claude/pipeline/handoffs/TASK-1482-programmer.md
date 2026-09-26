# TASK-1482 — [VICTORY-SCREEN-NAVIGABLE] — programmer handoff

**Marker:** `TASK-1482-VICTORY-SCREEN-NAVIGABLE`
**Assignee:** gameplay-programmer · **Gate:** `TASK-1483`
**Date:** 2026-09-25
**Files written:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` (4 hunks) · this handoff · `TASKBOARD.md` (this row's `status:` only)
**Nothing else.** No `.uasset` write, no `SiegeMenuInputSubsystem.*` edit, no input binding, no engine source, no compile, no PIE, no git, no `CONVENTIONS.md`.

---

## 1. 🚨 SPEC (1) — THE `UIOnly` ANSWER, FIRST AND EXPLICITLY

### **THE ACCEPT PATH IS SLATE'S FOCUSED-`SButton` PATH. IT IS NOT ENHANCED INPUT. THIS ROW ADDS NO KEY, NO INPUT ACTION, NO MAPPING AND NO BINDING OF ANY KIND.**

The chain, re-confirmed at source this row (not inherited on trust):

| link | site | effect |
|---|---|---|
| 1 | `SiegePlayerController.cpp` — `FInputModeUIOnly InputMode;` … `SetInputMode(InputMode);` (victory path, `HandleMatchEnd`) | the screen runs UI-only |
| 2 | `PlayerController.cpp:6384` — `FInputModeUIOnly::ApplyInputMode` | `GameViewportClient.SetIgnoreInput(true)` |
| 3 | `GameViewportClient.cpp:767-770` — first statement of `UGameViewportClient::InputKey` | `if (IgnoreInput()) { return …; }` — **before any Enhanced Input processing** |
| 4 | ⇒ | **`IA_MenuUp` / `IA_MenuDown` / `IA_MenuAccept` / `IA_MenuLeft` / `IA_MenuRight` / `IA_MenuBack` can never fire on this screen** |

⇒ the live route is the one the shipped code already chose: **`Enter` / `SpaceBar` / gamepad Accept → `SButton::OnKeyDown` (`SButton.cpp:293-316`, via `FNavigationConfig`'s Accept rules) → `ExecuteOnClick`**, reached because `FInputModeUIOnly::SetWidgetToFocus(PlayAgainSlate)` put Slate keyboard focus on the button itself. Arrow keys on that screen are Slate's own navigation (`SWidget::OnKeyDown` → `Handled().SetNavigation(...)`), also not Enhanced Input.

⛔ **`TASK-1429`'s in-match arming does NOT cover this screen, and I did not build on it.** The arm is real (my registration triggers it), and it is *inert here* for the reason above. That is stated in code, not only here — see hunk 2 below.

---

## 2. ⭐ SPEC (3) — THE ALREADY-EXISTS AUDIT. **MOST OF THIS ROW WAS ALREADY DONE.**

### 2.1 What I found already working and did **NOT** rebuild

| thing | where | state I found it in | what I did |
|---|---|---|---|
| `Btn_Jump`.`IsFocusable` in `/Game/UI/WBP_VictoryScreen` | the asset (`1d433ca`) | **`True`** | **nothing.** Not re-flipped, not saved, not opened. |
| `static const FName PlayAgainButtonName(TEXT("Btn_Jump"))` + `GetWidgetFromName` lookup + its miss-Warning | victory path | present, correct | **untouched** |
| `PlayAgainButton->TakeWidget()` | victory path | present | **untouched** |
| `if (PlayAgainSlate->SupportsKeyboardFocus())` **and its "⛔ THIS GUARD IS LOAD-BEARING" block** | victory path | present | **BYTE-IDENTICAL. Not one character.** |
| `InputMode.SetWidgetToFocus(PlayAgainSlate)` | victory path | present | **untouched** |
| the `SupportsKeyboardFocus() == false` else-branch Warning | victory path | present | **untouched** |
| the two published `match ended — winner %s.` log sites (`SC-§135`) | both paths | present, duplicated deliberately | **both untouched, still byte-identical to each other** |
| the `!VictoryWidget` degraded `GameAndUI` arm (`TASK-1319`) | victory path | present | **untouched** |

### 2.2 ⭐ The guard now **passes** where it previously declined-and-logged — confirmed at source

The `else` branch's Warning is the only thing that would ever announce a decline. The predicate it guards is `SWidget::SupportsKeyboardFocus()` on `Btn_Jump`'s `SButton`. `UButton`'s `SButton` is constructed with `bIsFocusable = InitIsFocusable`, `InitIsFocusable` defaults to `true` (`Button.cpp:48`), and `TASK-1431` read the live asset **by object path** (`/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump` → `IsFocusable=True`) with `WBP_HUD` and `WBP_CardHand` reading `False` as discriminating controls. ⇒ **the guard's true-branch is the live path today.**

⛔ **And the guard still has to stay**, exactly as `TASK-1464` measured: `Widget.h` exposes no `Focusable` member and `UButton` exposes `InitIsFocusable` **constructor-time only, with no `SetIsFocusable`** ⇒ *"C++ cannot re-assert this if the asset regresses"* is **TRUE**, and this guard is the only backstop. I re-read `Button.h`'s accessor surface via the existing in-file citation rather than re-deriving it; flagged in §7.

### 2.3 ⛔ So what was actually missing? **A registration. That is the whole defect.**

The screen was **REACHABLE** (focus is placed on the right button, Accept works through Slate) and **NOT ENUMERABLE**: nothing ever named `WBP_VictoryScreen` to `USiegeMenuInputSubsystem`, so

- `GetActiveNavTarget()` never pointed at it,
- `GetMenuFocusStops()` never walked its tree,
- **`LogNavTargetRetarget` never printed a focus-stop count for it** — the instrument the board's `EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24` ruling makes binding, and the only line any lane can read for this screen.

**The honest size of the code I added is two calls** (`RegisterSelfDrivingMenuNavTarget`, `UnregisterMenuNavTarget`), their null-safe subsystem resolution, one degradation log line, and one include.

---

## 3. ⛔ SPEC (2) — REGISTRATION IS ON **OPEN** AND **CLOSE**, AND THERE IS NO `NativeConstruct` ANYWHERE

`WBP_VictoryScreen` has **no C++ class** — it is a pure WidgetBlueprint — so there is no `NativeConstruct` for me to get this wrong in. The edges are the controller's, and there are **exactly two sites in the whole class that touch `VictoryWidget`'s lifetime**, verified by census (`grep -n "VictoryWidget"` → 2 lifetime sites, 0 others):

| edge | function | placement | why there |
|---|---|---|---|
| **OPEN** | `ASiegePlayerController::HandleMatchEnd` | **after `AddToViewport`, after `SetInputMode(InputMode)`**, before the published log line | `qa/TASK-1430.md` WARN-2: the arming backstop is **DISARM-ONLY**, so a screen that registers **while hidden never arms and nothing errors**. At this site `IsInMatchScreenOpen()`'s `IsInViewport() && IsVisible()` re-read sees the true post-open state, and the retarget line cannot describe a screen the viewport has not got yet. |
| **CLOSE** | `ASiegePlayerController::HandleMatchReset` | inside the existing `if (VictoryWidget)`, **before `RemoveFromParent()`** | the subsystem's own stated shape: `UnregisterMenuNavTarget`'s comment reads *"an unregister runs from `BackPressed`, BEFORE `RemoveFromParent`"*. |

⛔ **I did not copy the main-menu screens (`1415`/`1417`/`1419`/`1425`).** They register in `NativeConstruct` because they are layered over a menu that is already up. This is `TASK-1432`'s pattern: register on open, unregister on close.

**Every exit path is covered because there is only one.** `VictoryWidget` is assigned in exactly one place (`CreateWidget`, `HandleMatchEnd`) and cleared in exactly one place (`HandleMatchReset`), and `ASiegeGameMode::PlayAgain` calls `HandleMatchReset` on every controller. The two residual cases both self-heal and are declared, not hidden:

1. **`WBP_VictoryScreen`'s own graph calls `RemoveFromParent` on the Play Again click before `HandleMatchReset` runs** (the existing "idempotent with" comment says it does). Then `GetRegisteredNavTarget()`'s per-read `IsInViewport()` re-validation drops the entry by itself, the disarm-only backstop poll reconciles, and my later `UnregisterMenuNavTarget` removes **by identity** and **logs — does not warn —** that there was nothing to remove.
2. **PC/world teardown with the screen still up.** `NavTargetStack` holds `TWeakObjectPtr`, entries are compacted on every read and registration, and the subsystem dies with the world.

---

## 4. 🚨 THE ONE JUDGEMENT CALL ON THIS ROW — **`RegisterSelfDrivingMenuNavTarget`, NOT `RegisterMenuNavTarget`**

I want QA to look hard at this, because the subsystem's own one-line test points the other way and I chose against it deliberately.

**The header's test:** *"does it bind the `IA_Menu*` actions itself, or navigate a structure the walker cannot see?"* — the victory screen answers **no** to both, which is the answer Settings/Graphics/Login/Session/Controls Help give, and they all use the plain call.

**Why I still chose self-driving — it is mechanical, not aesthetic.** The declaration suppresses exactly four things:

| suppressed | live on this screen without the flag? | consequence of suppressing it |
|---|---|---|
| `MoveFocus(±1)` | ⛔ **NO** — needs `IA_MenuUp/Down`, deaf under `SetIgnoreInput(true)` | subtracts nothing that could ever run |
| `HandleMenuAccept()` | ⛔ **NO** — needs `IA_MenuAccept`, same | subtracts nothing |
| `StepFocusedStop(±1)` | ⛔ **NO** — needs `IA_MenuLeft/Right`, same | subtracts nothing |
| **`FocusFirstNavStop()`** | ✅ **YES** — it runs **from C++ inside `RegisterMenuNavTarget` itself**, so `SetIgnoreInput` does not touch it | **this is the whole reason** |

With the plain call, `RegisterMenuNavTarget` **ends in `FocusFirstNavStop()`**, which would place the generic ring on this tree's **stop 0** — a *second* focus placer on a screen whose focus target was already chosen **by name** twenty lines earlier (`SetWidgetToFocus(PlayAgainSlate)`), and whose correctness would then depend on (a) stop 0 happening to be `Btn_Jump` and (b) the relative ordering of an **immediate** `FSlateApplication::SetUserFocus` against `SetInputMode`'s **deferred** `SlateOperations` reply — an ordering nobody in this project has measured and nobody enforces. `RegisterSelfDrivingMenuNavTarget`'s own gate-4 comment describes precisely this hazard: *"a declared screen would open with the focus rectangle already painted on its stop 0 — a control the player never pointed at, wearing the ring, on a screen whose own navigation has just placed itself somewhere else."*

**And the flag keeps everything this row exists for** — the stack entry, the in-match arm, and `LogNavTargetRetarget`'s focus-stop count. The header says so itself: *"the registration is what makes the screen ENUMERABLE … This flag keeps both and removes only the walking."*

⚖️ **This is a third case the one-line test does not cover:** *the ring is structurally deaf here, and the only part of it that can still act is the part that would fight the shipped placement.* **If QA rules the plain call is required, the fix is one identifier** (`RegisterSelfDrivingMenuNavTarget` → `RegisterMenuNavTarget`, one line, one site) — but I would want the stop-0-vs-`SetWidgetToFocus` race answered first, because nothing on the board has measured it.

---

## 5. ⛔ SPEC (4) — THE EXPECTED STOP SET, AS A **PROPERTY WITH ITS GENERATING RULE**

### 5.1 The rule (not a number)

> **Expected stop set** = `{ w ∈ WBP_VictoryScreen.WidgetTree, in `CollectNavStopsFromTree` traversal order (TASK-1474: descends into code-authored native sub-widgets only, `IsCodeAuthoredSubWidget` refusing Blueprint classes) : IsNavFocusStop(w) }`
>
> where `IsNavFocusStop(w)` (`SiegeMenuInputSubsystem.cpp:855`) requires **all** of:
> 1. `w->GetIsEnabled()`
> 2. `w->IsVisible()`
> 3. `HasVisibleSlateAncestry(w)`
> 4. `w` is one of the four admitted classes — `UButton`, `UCheckBox`, `USlider`, `UEditableTextBox`
> 5. and, **for a `UButton`**, `w->GetIsFocusable()`

### 5.2 Applied to this asset — the admitted-class population is a **singleton**

Measured off `Content/UI/WBP_VictoryScreen.uasset` (153,825 bytes, mtime 2026-09-20 01:20 — the `1d433ca` file) by extracting its identifier strings, **read-only, no editor, no MCP, no write**:

| admitted class | occurrences in the package's name table |
|---|---|
| `Button` | **1** (`Btn_Jump`) |
| `CheckBox` | **0** |
| `Slider` | **0** |
| `EditableTextBox` | **0** |

The only other widget classes present are `Overlay`, `SizeBox`, `Image`, `TextBlock`, `UserWidget` and `UI_Thumbstick_C` (+ their slot types). The only literal `Btn_*` instance name in the package is **`Btn_Jump`**.

⇒ **EXPECTED: `1 focus stop(s)`** on the `LogNavTargetRetarget` line for this screen.

### 5.3 🚨 The anchor — **OBJECT PATH, because `Btn_Jump` collides nine ways**

- **Asset-side object path:** `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump`
- **Runtime parent chain:** `WBP_VictoryScreen_C_<n> → Overlay_19 / SizeBox_0 / Btn_Jump`
- **Authored:** `class=Button · IsFocusable=True · Visibility=Visible · bIsEnabled=True` (`TASK-1431`, live by object path)
- ⛔ **Do not read a bare `Btn_Jump`.** The identical chain `Overlay_19/SizeBox_0/Btn_Jump` exists in `WBP_HUD` (`IsFocusable=False`, **on screen during the match**), `WBP_CardHand` (`False`, **`Collapsed`**), `WBP_MainMenu`, `WBP_DeckBuilder`, `WBP_DeckCardTile` and three touch-interface assets. `MoveFocus` prints a bare `GetName()` with no screen qualifier.
- ⭐ **The retarget line is safe to read without that ambiguity**, because it names the **screen** (`GetNameSafe(GetActiveNavTarget())` → `WBP_VictoryScreen_C_<n>`) and a **count**, not a button name.

### 5.4 Delta against `TASK-1474`'s table

**`+1 row. +0 to every existing row.`** This row adds no walker code, edits no admission predicate and touches no other screen's tree — the only change to any count anywhere is that a screen which previously produced **no line at all** now produces one. `WBP_MainMenu` stays 7 → 7; the deck builder's count is untouched.

### 5.5 ⚠️ The `UI_Thumbstick_C` rider — it contributes **zero**, for a reason already proven

`UI_Thumbstick_C` is in this package's name table (the same UE mobile-template leftover that put `Btn_Jump` there). It contributes **0 stops** on two independent grounds: a `UUserWidget` is **not one of the four admitted classes**, and `IsCodeAuthoredSubWidget` **refuses Blueprint classes** so the walker does not descend into it. `TASK-1474` proved exactly this on `WBP_MainMenu` (*"`WBP_MainMenu`'s two nested user widgets are `UI_Thumbstick_C` — Blueprint ⇒ the boundary refuses them. 7 → 7"*).

### 5.6 ⚠️ SWITCHER-CLASS HAZARD — **DECLARED: NONE FOUND**

`TASK-1474` warned that a stop admitted but unable to take focus **swallows the ring** (`SiegeMenuInputSubsystem.cpp:848-851`), and that the walker does not model `SWidgetSwitcher`. **There is no `WidgetSwitcher` anywhere in `WBP_VictoryScreen.uasset`'s identifier set, and no collapsed branch or deferred-realization panel** — the tree is `Overlay` / `SizeBox` / `Button` / `TextBlock` / `Image`. ⛔ And the hazard is doubly moot here anyway: the ring **cannot be driven on this screen at all** (Enhanced Input deaf), and the self-driving declaration refuses `MoveFocus` outright.

### 5.7 The falsifiers, stated in advance — ⭐ *a silence is not one thing*

`LogNavTargetRetarget` prints at `UE_LOG(LogSiegeMenuInput, Log, …)` and `LogSiegeMenuInput` is `DECLARE_LOG_CATEGORY_EXTERN(…, Log, All)` ⇒ **it prints in Development with no console verb**, so:

| observation | reading |
|---|---|
| `menu nav target registered -> 'WBP_VictoryScreen_C_0' (registered screen), 1 focus stop(s), 1 screen(s) registered.` | ✅ expected |
| a line reading **`0 focus stop(s)`** | ⛔ the walk reached the tree and the tree **admitted nothing** — `Btn_Jump` collapsed / disabled / de-focused at runtime |
| **no line at all** | ⛔ **registration never ran** — a ZERO and a defect signature, not "quiet" |
| `RegisterSelfDrivingMenuNavTarget(null) ignored` (Warning) | ⛔ `VictoryWidget` null at the call — impossible past the `!VictoryWidget` early return, so it would mean the early return moved |
| `no USiegeMenuInputSubsystem on this world` (my new `LogGitClaudeUnrealTest` Log line) | the world declined the subsystem; screen still mouse- and Slate-usable |

---

## 6. ⛔ SPEC (5) — THE BLUEPRINT DECLARATION, MADE EITHER WAY

**`WBP_VictoryScreen` is a real `.uasset` with an event graph I cannot read under this row's fence** (`Read`/`Grep`/`Edit` only — no MCP, no editor).

**⭐ I did not need to, and I am not escalating.** The open/close path is **fully C++-reachable** and is not BP-driven: the controller creates the widget, adds it to the viewport, applies the input mode and removes it, all in `SiegePlayerController.cpp`. Both of my edges hang off code I can read and change. **No Blueprint edit was authored, and none is required** — there is no 🧑 human keystroke hidden in this row.

**What the graph does do, from the in-file contract I did not verify myself:** `Construct` parents a "Play Again" `TextBlock` into `Btn_Jump` and binds `Btn_Jump->OnClicked` to `RequestPlayAgain`; the graph may also call `RemoveFromParent` on itself. **One consequence worth QA's eye:** because the click handler lives in the graph, the Slate Accept path reaches it via `ExecuteOnClick` → `OnClicked` → the graph's `RequestPlayAgain` node — i.e. **keyboard Accept and mouse click converge on the same broadcast**, which is why no new activation code is needed. ⚠️ **I could not read that binding directly** (§7).

---

## 7. ## Not examined / limitations

1. ⛔ **`WBP_VictoryScreen`'s event graph was never read.** Out of fence. Everything I say about `Construct`, the "Play Again" TextBlock, the `OnClicked → RequestPlayAgain` binding and the self-`RemoveFromParent` is **quoted from the in-file comments at `SiegePlayerController.cpp`'s victory path**, which cite `TASK-011`/`TASK-1314`. I did not re-verify any of it.
2. ⛔ **The live widget tree was never walked.** §5.2's singleton is a **static string-table scan of the package**, not a runtime enumeration. It cannot see a widget the graph **creates at runtime** and parents into the tree. It is strong against *authored* controls (a `UCheckBox` would have to appear in the name table) and weaker against *constructed* ones. **`1 focus stop(s)` is a prediction with a stated mechanism, not a measurement.**
3. ⛔ **`Overlay_19` / `SizeBox_0` do not appear as literal strings** in my scan — UE stores them as FName base + number (`Overlay` + 20, `SizeBox` + 1). I am taking that chain from `TASK-1431`'s **live** object-path read, not from my own scan.
4. 🚨 **`SupportsKeyboardFocus()` has still never been exercised at runtime.** `TASK-1431`'s closure rider said so and **this row does not change that** — I added no runtime observation. It remains 5b's job. My change does not make the guard any more or less exercised; it was already on the path.
5. ⛔ **The `FocusFirstNavStop()`-vs-`SetWidgetToFocus` ordering race is ARGUED, NOT MEASURED** (§4). I avoided it rather than resolving it. If QA overrules §4's choice, that race must be measured before the plain call is safe.
6. ⛔ **Nothing was compiled, no PIE was run, no editor state was touched, no MCP call was made, no git command was run.** The four hunks are unbuilt.
7. ⛔ **`Button.h:205-206` / `Button.cpp:48` were not re-opened.** I relied on the in-file citations from `TASK-1314`/`TASK-1464` for "`InitIsFocusable` is constructor-time only, default `true`".
8. ⚠️ **Hunk 1 (the include) is the one hunk not literally inside the victory-screen path.** It is the mechanical precondition for hunks 2 and 4 to compile, and it follows the pattern the other four screens used verbatim (an include + a task-tagged comment naming the API it is for). Declared here so QA's fence check (5) does not have to discover it.
9. ⚠️ **Hunk 3 is a comment-only correction** I chose to make rather than ship a contradiction — see §8. It is on the victory path, but it is not code. If QA prefers the diff narrower, hunk 3 reverts independently of the other three.

---

## 8. The four hunks, declared by CONTENT (⛔ not by `--numstat`, which is algorithm-dependent)

**File: `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`. No other source file was opened for writing.**

| # | where | content of the change | deletions |
|---|---|---|---|
| **1** | include block, immediately **above** `#include "Siegebound/SiegePlayerState.h"` | **ADDED** one `#include "Siegebound/SiegeMenuInputSubsystem.h"` with a `TASK-1482` comment declaring it a **read-only dependency** (this controller calls the public API and never edits that file). | **none** |
| **2** | `HandleMatchEnd`, immediately **after** `SetInputMode(InputMode);` and **before** the `SC-§135` published-log comment block | **ADDED** the open edge: `GetWorld()` → `World->GetSubsystem<USiegeMenuInputSubsystem>()` → `RegisterSelfDrivingMenuNavTarget(VictoryWidget)`, with an `else` emitting **one** `UE_LOG(LogGitClaudeUnrealTest, Log, …)` "no subsystem on this world" line; plus the block comment recording the `SetIgnoreInput` chain, why self-driving, and why this site rather than a constructor. | **none** |
| **3** | `HandleMatchEnd`, the `TASK-1314` comment paragraph **above** the `if (VictoryWidget)` focus block | **CORRECTED, NOT DELETED (`SC-§120`):** the clause *"and on L_Arena USiegeMenuInputSubsystem hard-returns before binding anything at all (SiegeMenuInputSubsystem.cpp:47)"* is struck with `~~…~~` and kept in place, followed by a `TASK-1482` note: `TASK-1429` made that subsystem **arm on demand off the menu map**, and **my own registration below is such a demand** ⇒ the clause is now false and *my change is what falsified it*. **The sentence it supported is unchanged and now rests on the stronger reason** (`GameViewportClient.cpp:767-770`), which I also added as an explicit citation. | ⛔ **no text deleted** — struck in place |
| **4** | `HandleMatchReset`, inside the existing `if (VictoryWidget)`, **above** `VictoryWidget->RemoveFromParent();` | **ADDED** the close edge: `GetWorld()` → `World->GetSubsystem<USiegeMenuInputSubsystem>()` → `UnregisterMenuNavTarget(VictoryWidget)`, **silent on a null subsystem**, with the comment recording that one call drops the stack entry + the self-driving mark + `IMC_MainMenu`, and why it goes before `RemoveFromParent`. | **none** |

### ⛔ Deletions across the whole row, declared by content: **ZERO.**
No line of existing code was removed, reworded or re-indented. **No existing log string was reworded** (`TASK-1395`/`TASK-1402` read four of them as grep discriminators, and the two `SC-§135` published lines are untouched **and still byte-identical to each other**). **The "⛔ THIS GUARD IS LOAD-BEARING" block is byte-identical.** The only text that changed at all, outside additions, is hunk 3's `~~strike~~` markers around a **comment** clause that my own change falsified.

### `.generated.h` prediction — **BY EXEC-SYMBOL SET**
`SiegePlayerController.h` was **not opened for writing**. This row adds **no `UCLASS`, no `USTRUCT`, no `UENUM`, no `UFUNCTION`, no `UPROPERTY`, and no reflected member of any kind**. ⇒ **the exec-symbol set of `SiegePlayerController.generated.h` is UNCHANGED** — no `DECLARE_FUNCTION`/`execXxx` added, removed or renamed, and the RPC/replication surface is untouched.
⚠️ ⛔ **Do not gate on that file's `sha256` or size.** Measured on the board this hour: a header came back **size-identical and API-identical with a CHANGED `sha256`**, because **UHT embeds source line numbers in its macro names** — and hunks 1–4 shift every line below them in the `.cpp`. **A changed `.generated.h` hash here is expected and is NOT evidence of an API change.** The exec-symbol set is the gate.

**M8 declaration:** adds no replicated property, no replicated class, no RPC, no relevancy tier. Everything here is local-player UI state, inside the existing `IsLocalController()` guards on both edges.

---

## 9. What QA (`TASK-1483`) should scrutinise hardest

1. 🚨 **§4 — the self-driving choice.** It is the one place I went against a documented one-line test. My argument is that three of the four suppressed behaviours are unreachable under `SetIgnoreInput(true)` and the fourth is a live conflict with a shipped focus placement. **Overrule me if that is wrong; it is a one-identifier change.**
2. **§5.2's singleton is a static scan, not a live walk** (§7 item 2). If `1 focus stop(s)` is to be *asserted* rather than *predicted*, it needs 5b.
3. **Hunk 3 is comment-only and optional** (§7 item 9) — confirm it is wanted before it counts against the fence.
4. **Hunk 1 is the only non-victory-path hunk** and it is an include (§7 item 8).
5. **The `SC-§135` byte-identity pair** — both copies of `match ended — winner %s.` and their argument lines are untouched; please confirm, since my hunk 2 sits directly above one of them.

---

# ⭐ QA LOOP 1 — FIX APPENDED 2026-09-25 (nothing above this line was edited)

**Report answered:** `.claude/pipeline/qa/TASK-1483.md` — 1 BLOCKER, 4 WARN, 4 NIT.
**Files written this loop:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` (victory path + its own include comment) · this appended section · `TASKBOARD.md` (this row's `status:` only).
**Nothing else.** No `SiegeMenuInputSubsystem.{cpp,h}` edit, no `SiegeControlsHelpWidget.*` (TASK-1479 holds it), no `.uasset`, no input binding, no engine source, no compile, no PIE, no MCP, no git, no `CONVENTIONS.md`.

## L1.1 🚨 BLOCKER-1 — FIXED. **AND MY §4 WAS WRONG FOR A REASON I AM RECORDING, NOT EXCUSING.**

`MenuInput->RegisterSelfDrivingMenuNavTarget(VictoryWidget);` → **`MenuInput->RegisterMenuNavTarget(VictoryWidget);`** — one identifier, one site (`HandleMatchEnd`). I re-verified QA's two refutations at source myself rather than accepting the ruling on trust:

| my §4 premise | what I measured this loop | verdict |
|---|---|---|
| "the three suppressed handlers are structurally unreachable here" | `SiegeMenuInputSubsystem.h` — *"⚠️ THE ONE EXCEPTION, AND IT IS THE VICTORY SCREEN … Injection is unaffected (`InjectInputForAction` never touches the viewport)"*, and *"What DOES reach them: `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` (Aura's `inject_input_action`, and the automation test)"* with *"no real-input lane exists for an agent"* alongside it. `HandleMenuAccept`'s first statement is `DeclineIfActiveTargetSelfDriving(TEXT("IA_MenuAccept"))`. | ⛔ **FALSE.** True of **real keys only**. Injection enters at the `UEnhancedInputComponent`, downstream of the `IgnoreInput()` early return I measured — so the flag made this screen deaf **to the agent**, the only lane that has a verb here at all. |
| "the `FocusFirstNavStop` race is unmeasured" | `FocusWidget` (read this loop): immediate `FSlateApplication::SetUserFocus(UserIndex, SlateWidget, EFocusCause::Navigation)` → on success `DelayedSlateOperations.CancelFocusRequest()`, else `DelayedSlateOperations.SetUserFocus(SlateWidget, Navigation)`. `RegisterMenuNavTarget` ends in `FocusFirstNavStop()`, which early-returns `false` on `Stops.Num() == 0` **without cancelling**. Stop set = `{Btn_Jump}` (QA's live asset read). Registration sits **after** `SetInputMode`. | ⛔ **NO LONGER UNMEASURED, AND IT CONVERGES.** Win or lose, the same widget ends focused with cause `Navigation`; zero stops degrades to today's behaviour verbatim. **No branch of the plain call is worse.** |

⭐ **The discriminator I am carrying forward, now written into the file:** the flag declares *"this screen drives its own navigation."* **The victory screen has no driver.** The deck builder holds it legitimately because it does — `HandleMenuNavAccept → HandleCardGridKey → AcceptFocusedCard`, a separate delegate on the same action (`qa/TASK-1472.md`). ⛔ **Self-driving without a driver is just deaf.**

⭐ **And the cost I never priced, which QA found and which is 🧑 the original ask:** `EFocusCause::Navigation` is the only cause that paints a ring (`ShowFocus = (InCause == Navigation)` → `QueryShowFocus` under the engine-default `ERenderFocusRule::NavigationOnly`, no project override → `SWidget::Paint` draws `GetFocusBrush()`), and `SetWidgetToFocus` deposits its request with the **default** cause `SetDirectly`. The flag forfeited the **only** `Navigation` placement on this path ⇒ **the button could never wear the dashed outline.** The plain call restores it. I had argued the ring away as a hazard; it was the deliverable.

## L1.2 ⛔ `SetWidgetToFocus` — **KEPT, UNMOVED, UNREORDERED.** It is the zero-stop fallback

`InputMode.SetWidgetToFocus(PlayAgainSlate)` and the whole `if (PlayAgainSlate->SupportsKeyboardFocus())` block are **character-identical** to loop 0 (grep-verified: guard comment, predicate, call, both miss-Warnings). `FocusFirstNavStop()` returning `false` on an empty walk **without cancelling anything** is precisely why it must stay — it is the fallback, not a duplicate, and the new comment says so in those words.

## L1.3 WARN-1 — the false premise is corrected **where it was taught**, in two places, **struck in place**

| site | what changed | what did NOT |
|---|---|---|
| the register block (hunk 2's comment) | rewritten. The old conclusion — *"⇒ the subsystem's IA_Menu\* handlers CANNOT fire on this screen, so three of the four behaviours … subtracts nothing that could ever have run."* — is quoted **verbatim inside `~~…~~`** and answered: `SetIgnoreInput(true)` deafens **real keys only**; `InjectInputForAction` **never touches the viewport**; the header paragraph that already said so is named **by text**. Then the discriminator, the convergence measurement, the zero-stop fallback, and the ring cost. | — |
| hunk 3's `TASK-1482` note | a narrowing rider added **below** the strike: the surviving sentence is true **of a real key only**, injection is live here for the agent lane, header named by text. NIT-1's by-text anchor added for the rotted `SiegeMenuInputSubsystem.cpp:47` citation. | ⛔ **hunk 3's `~~…~~` strike is character-identical** and ⛔ **not one character of TASK-1314's own prose was edited.** The correction is by annotation, never by rewording someone else's sentence. |

⚠️ **Declared, because the fence named it:** the dispatch said *do not touch hunk 3's strike* — I did not. What I edited in hunk 3 is **only the `TASK-1482` note I myself added in loop 0**, because leaving it would have left the file asserting the exact premise the register block twenty lines below now refutes. If QA reads that as widening, it reverts independently of everything else.

**Two further one-identifier corrections, declared rather than slipped in** — both are loop-0 text of mine that named an API this file no longer calls:
- the include comment (hunk 1): `RegisterSelfDrivingMenuNavTarget / Unregister…` → `RegisterMenuNavTarget / Unregister…`;
- the close-edge comment: *"one call does all three halves"* → **two**, with the self-driving half `~~struck in place~~` rather than deleted, plus a note that `UnregisterMenuNavTarget` was **already the correct pairing for both entry points, so not one character of the call itself moved.**

## L1.4 NIT-2 — the overstatement is gone from the new text

The new register-block comment says the return is *"ABOVE the `OnInputKeyEvent.Broadcast` / player-routing block — not literally its first statement"*. ⚠️ TASK-1314's own line still reads *"the FIRST statement"*; that is **pre-existing prose I am not allowed to reword**, and the material claim (before Enhanced Input or any player routing) is correct either way.

## L1.5 ⭐ THE NEW TRUTH, STATED — **NOT the old one repeated**

**Stop set: still `1`.** `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump` — ⛔ **object-path-anchored, NOT `UNANCHORED`**: a bare `Btn_Jump` collides **nine** ways (`WBP_HUD`'s copy is on screen during the match). Expected line, and 5b must read the **parenthetical** as well as the integer:

`menu nav target registered -> 'WBP_VictoryScreen_C_<n>' (registered screen), 1 focus stop(s), N screen(s) registered.`

⇒ `0 … (default: WBP_MainMenu)` means **the registration did not take**, a different defect from `0 … (registered screen)` (WARN-3, accepted).

### 🚨 THE ACCEPTANCE, ANSWERED IN QA'S OWN WORDS

> **Does anything a human can do on this screen remain agent-unreachable?**

### **NO.**

| what a human can do | the agent, after this fix |
|---|---|
| see which control is focused | ✅ the ring is **painted** — `FocusFirstNavStop → FocusWidget` places it with `EFocusCause::Navigation`, the one cause `QueryShowFocus` admits under the engine-default `NavigationOnly` — **and** the `1 focus stop(s)` line is readable |
| press **Play Again** (mouse, or `Enter`/`Space`/gamepad Accept via `SButton::OnKeyDown`) | ✅ injected `IA_MenuAccept` → `HandleMenuAccept` (**no longer declined**) → `IsNavTargetActionable()` true → `GetFocusedNavStop()` = `Btn_Jump` → `OnClicked.Broadcast()` → the graph's `OnClicked_Event` → `RequestPlayAgain` |
| move the ring with the arrows | ✅ reachable — `MoveFocus` no longer declines. On a **one-stop** ring it is an honest no-op, which is exactly what the human's arrow keys do too |
| leave / back out | there is nothing else to press; `IA_MenuBack` stays **inert** (the asset implements no `ISiegeMenuNavCloseTarget`) — **unchanged, and identical for both** |

⇒ **the verdict sentence moves from *enumerable* to *navigable*: "registered, one stop, ring placed and visible, Accept reaches the button."**

## L1.6 The instrument verbosity, checked rather than assumed

Both categories are `Log, All` — `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMenuInput, Log, All)` and `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)`. My degradation line is `UE_LOG(…, Log, …)`, **not `Verbose`** ⇒ it prints in Development with **no console verb**, so ⛔ **an absent line is a ZERO and a defect signature, never "quiet".** ⭐ *A silence is not one thing* — and WARN-3 is the second face of that: a `0` with the wrong parenthetical is a different silence again.

## L1.7 Deletions and the generated header

**Deletions of pre-existing content: ZERO, declared by content** (⛔ not by `--numstat`, which is algorithm-dependent and unstable). Verified present and unchanged this loop: the `THIS GUARD IS LOAD-BEARING` block · `SupportsKeyboardFocus()` · `SetWidgetToFocus(PlayAgainSlate)` · both miss-Warnings · the `TASK-1319` degraded arm · **both `SC-§135` copies** (format literal and argument line **character-identical** to each other at both sites) · hunk 3's `~~…~~` strike · every line of TASK-1314 prose. The only text replaced anywhere is **my own loop-0 commentary**, and its falsified claims are `~~struck~~`, not removed.

**`.generated.h` — predicted by EXEC-SYMBOL SET, which is UNCHANGED.** No `UCLASS`/`USTRUCT`/`UENUM`/`UFUNCTION`/`UPROPERTY` added, removed or renamed; `SiegePlayerController.h` was not opened for writing; no RPC or replication surface touched. ⛔ **Do not gate on that file's size or `sha256`** — both were measured unsound this session, because UHT embeds source line numbers and this loop shifts lines.

**M8:** unchanged — no replicated property, class, RPC or relevancy tier. Both edges stay inside the existing `IsLocalController()` guards.

## L1.8 ## Not examined / limitations (loop 1)

1. ⛔ **Nothing compiled, no PIE, no MCP, no editor state touched, no git command run.** Every claim here is source reading.
2. ⛔ **The ring is argued at source, never seen on a pixel.** Chain: `Navigation` cause → `ShowFocus` → `QueryShowFocus` under `NavigationOnly` → `GetFocusBrush()`. **5b owns the pixel.** If no ring appears, the mechanism is wrong — ⛔ but the **injected-Accept** limb of this fix stands independently of it.
3. ⛔ **`1 focus stop(s)` is still not a live walk.** It is QA's authored-tree + `Construct`-graph measurement (materially stronger than my loop-0 string scan). `GetMenuFocusStops()` has not been executed.
4. ⛔ **The injected-Accept path is traced, not injected.** I hold no injection verb. `inject_input_action(IA_MenuAccept)` on the live end screen is **5b's** assertion, and a `HandleMenuAccept` decline line in the log would mean the flag came back.
5. ⛔ **The five orphan `OnClicked_Event_*` strands remain unread** (QA's NIT-3 / limitation 5). Unchanged by this loop.
6. ⚠️ **`SupportsKeyboardFocus()` has still never been exercised at runtime.** Unchanged by this loop, as in loop 0.
7. ⚠️ **No diff was taken** (no git verb). "Victory path only" rests on a `TASK-1482` marker census (include + three victory-path sites) plus reading both functions end to end.
8. ⚠️ **`WBP_VictoryScreen`'s `.uasset` was not opened, read, saved or flipped** in either loop.
