# TASK-1417 — [MENU-NAV-GRAPHICS] — gameplay-programmer handoff

- **Marker:** `TASK-1417-MENU-NAV-GRAPHICS`
- **Date:** 2026-09-24
- **Files written:** `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` (**+339 / −0**) · `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.h` (**+194 / −0**) · this note · this row's `status:`
- **`git diff --numstat` reports ZERO DELETIONS in both files** ⇒ every pre-existing line is byte-identical to `HEAD` (`40c824b`). The change is purely additive.
- ⛔ **NO compile · NO PIE · NO MCP · NO asset · NO git · NO editor lifecycle action.** The running editor was never touched. `SiegeMenuInputSubsystem.{cpp,h}` and `SettingsMenuWidget.{cpp,h}` were **READ ONLY**, never written (marker census below).

---

## ⭐⭐ 1. THE EXPECTED STOP COUNT — **A NAME LIST, NOT A TALLY**

Binding marker `EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24`, clauses 1–4. Stated **BEFORE 5b (`TASK-1421`) so it can be FALSIFIED**, derived from this screen's own control census, **never from a walk**.

### ✅ FIRST, THE MEASUREMENT THE MARKER'S FREE PASS ASKS FOR — AND IT IS **"NO ASSET"**, NOT "AN ASSET WITH NO OPT-OUT"

**`USiegeGraphicsMenuWidget` is 100 % C++ with no `.uasset` of any kind.** `GFX-§2` *reserves* `/Game/UI/WBP_GraphicsMenu` and `TASK-1461` measured it **unauthored**; `ConstructGraphicsTree()` builds the whole tree in code and its condition-(b) escape hatch (`if (WidgetTree->RootWidget != nullptr) return;`) has nothing to fire against. ⇒ **there is no asset here that *could* carry an `IsFocusable=False`, and no EventGraph that *could* carry an opt-out node.** That is a *different and stronger* statement than "I looked in an asset and found no opt-out", and it is stated that way deliberately.

Corroborating source-side audit: `IsFocusable` / `SetIsFocusable` / `SetKeyboardFocus` / `SetUserFocus` / `NativeOnKeyDown` have **ZERO executable hits** in either file — the only grep hits are my own new comments saying why they are absent.

### THE 24 NAMES, IN `UWidgetTree::ForEachWidget` ORDER (depth-first pre-order, slot order)

| # | Name | Class | Construction route |
|---|------|-------|--------------------|
| 0 | `ShowFrameRateCounterCheckBox` | `UCheckBox` | **C++** |
| 1 | `AutoDetectButton` | `UButton` | **C++** |
| 2 | `OverallQualitySlider` | `USlider` | **C++** |
| 3 | `ViewDistanceQualitySlider` | `USlider` | **C++** |
| 4 | `AntiAliasingQualitySlider` | `USlider` | **C++** |
| 5 | `ShadowQualitySlider` | `USlider` | **C++** |
| 6 | `GlobalIlluminationQualitySlider` | `USlider` | **C++** |
| 7 | `ReflectionQualitySlider` | `USlider` | **C++** |
| 8 | `PostProcessQualitySlider` | `USlider` | **C++** |
| 9 | `TextureQualitySlider` | `USlider` | **C++** |
| 10 | `EffectsQualitySlider` | `USlider` | **C++** |
| 11 | `FoliageQualitySlider` | `USlider` | **C++** |
| 12 | `ShadingQualitySlider` | `USlider` | **C++** |
| 13 | `ResolutionScaleSlider` | `USlider` | **C++** |
| 14 | `ScreenResolutionPrevButton` | `UButton` | **C++** |
| 15 | `ScreenResolutionNextButton` | `UButton` | **C++** |
| 16 | `WindowModePrevButton` | `UButton` | **C++** |
| 17 | `WindowModeNextButton` | `UButton` | **C++** |
| 18 | `KeepSettingsButton` | `UButton` | **C++** | ⚠️ state-dependent — see §5 |
| 19 | `RevertSettingsButton` | `UButton` | **C++** | ⚠️ state-dependent — see §5 |
| 20 | `VSyncCheckBox` | `UCheckBox` | **C++** |
| 21 | `FrameRateLimitPrevButton` | `UButton` | **C++** |
| 22 | `FrameRateLimitNextButton` | `UButton` | **C++** |
| 23 | `BackButton` | `UButton` | **C++** |

⇒ **EXPECTED COUNT = 24** on the healthy path (both subsystems resolve, no countdown running).

**Every construction route in this table is `C++`. There is not one `asset-authored` and not one `EventGraph` name on this screen** — which is why a short count here would point at the walker or at enabled-state, never at a census blind spot.

### ⚠️ clause 3 — WHAT THE COUNT BOUNDS AND WHAT IT DOES NOT

**It bounds THE DEFECT (a stop the player cannot reach). It does NOT bound THE CAUSE** (opted out in an asset vs opted out in a graph vs never created vs disabled at seed time). The defect is what ships; the cause is diagnosis, and diagnosis is cheap once a *name* is missing rather than a *number* being short.

### ⚠️ clause 4 — THE RESIDUAL THE COUNT CANNOT CLOSE

If `TASK-1421` returns **`UNOBSERVABLE`** it produces **no count**, and this screen is then **UNMEASURED on this axis too**. The 24 above must **not** stand in for a reading (`VER-§5` cl. 2). Graphics is the screen the census could *not* reach at all (`TASK-1399` §2 row 4), so this is a live possibility, not a formality.

### ⚠️ THE UNHAPPY-PATH COUNT, DECLARED SO A LOW NUMBER IS NOT MIS-READ AS THE DEFECT

With no `USiegeGraphicsSettingsSubsystem`, `SeedAndBind()` → `ShowPanelUnavailable()` → `SetAllControlsEnabled(false)` disables **22** of the 24 (all ten group sliders + Overall + ResolutionScale + VSync + AutoDetect + the six display stepper buttons + Keep + Revert). Survivors: `ShowFrameRateCounterCheckBox` (a *different* store, `GFX-§3`'s named exception, deliberately absent from that list) and `BackButton` (explicitly re-enabled — "a panel you cannot leave is worse than a panel that cannot change anything").
⇒ **a count of 2 WITH `[GraphicsMenu] USiegeGraphicsSettingsSubsystem could not be resolved` (Warning) in the same run is HEALTHY. A 2 WITHOUT it is the defect.**

### ⚠️ EXPECT THE COUNT TO LOOK HIGH — THE STEPPER DOUBLE-STOP (`qa/TASK-1410.md` WARN-2)

Six of the 24 (#14–#17, #21, #22) are the `<` / `>` pairs of the three stepper rows: each row is **two** `UButton`s, so the ring stops **twice per stepper row**. ⛔ **Not fixable inside this row's fence and not this row's defect** — routed to the walker. Reported here by name so the count reads as expected rather than inflated.

### ⚠️ ALSO EXPECTED AT 5b — THE RING IS **ALREADY ON** AT OPEN

`TASK-1406`'s `RegisterMenuNavTarget` ends in `FocusFirstNavStop()`, so this panel opens with **stop 0 (`ShowFrameRateCounterCheckBox`) already outlined**. ⇒ **Down ×1 = `AutoDetectButton` (stop 1)**, not stop 0. Down ×24 wraps back to stop 0. **A row expecting stop 0 after one Down will mis-read a WORKING screen as BROKEN.**

---

## ⭐ 2. THE SCROLL-INTO-VIEW WIRING — **QUOTED**, AND WHY IT IS TWO MECHANISMS THAT AGREE

### 🚨 WHY IT WAS NEEDED AT ALL — MEASURED AT ENGINE SOURCE, NOT ASSUMED

`UScrollBox`'s constructor sets **`ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)`** (`UMG/Private/Components/ScrollBox.cpp`, `UScrollBox::UScrollBox`). **The engine default is to NOT follow focus.** With 24 stops in a column that does not fit 1080p, the outline walks off the bottom and 🧑 he sees it vanish. *A ring the player cannot see is indistinguishable from no ring.*

### MECHANISM A — the engine's own hook, turned on (`ConfigureScrollFollowsFocus()`, `.cpp:3193`)

```cpp
RootScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
RootScrollBox->SetNavigationDestination(EDescendantScrollDestination::IntoView);
```

With that flag raised, `SScrollBox::OnFocusChanging` (`Slate/Private/Widgets/Layout/SScrollBox.cpp:1443`) runs
`ScrollDescendantIntoView(NewWidgetPath.GetLastWidget(), …)` for **every** focus change whose new path contains this scroll box — which is **every one of the 24 stops**, because all of them are its descendants. That the subsystem's focus move reaches this hook is measured, not hoped: `USiegeMenuInputSubsystem` focuses through `FSlateApplication::Get().SetUserFocus(UserIndex, SlateWidget, EFocusCause::Navigation)` (`SiegeMenuInputSubsystem.cpp:1400`, with the `DelayedSlateOperations` fallback at `:1406`), and `FSlateApplication` dispatches `OnFocusChanging` to **every widget in both the old and the new focus path** (`SlateApplication.cpp:3061` / `:3082`).

⛔ **`InstantScroll`, not `AnimatedScroll`** — the enum has exactly three members, `NoScroll` / `InstantScroll` / `AnimatedScroll` (`SScrollBox.h:63`). `SScrollBox::OnFocusChanging` passes `AnimateScroll = (mode == AnimatedScroll)`, so `InstantScroll` is byte-for-byte the same request Mechanism B makes. An animated catch-up would lag a held key and would make the offset a verifier reads a sample of an interpolation in progress.

⛔ **Set in TWO places, on purpose.** Once in `ConstructGraphicsTree()` (`.cpp:716`) **before** `Super::RebuildWidget()`, because `UScrollBox::RebuildWidget` bakes `ScrollWhenFocusChanges` / `NavigationDestination` straight into the `SNew(SScrollBox)` arguments and **`SynchronizeProperties` never re-pushes them**; and once in `NativeConstruct()` (`.cpp:1582`), because `ConstructGraphicsTree()` **returns early, whole**, on the `GFX-§2` condition-(b) path — a future asset-authored `WBP_GraphicsMenu` binding `RootScrollBox` by name would otherwise ship with `NoScroll`. The setter forwards to the live `MyScrollBox` when one exists, so the second call is idempotent, not a conflict.

### MECHANISM B — the explicit hook the spec named (`.cpp:2232` pre-edit → the same API, now also on focus)

`NativeOnFocusChanging` (`.cpp:3234`) → `ScrollFocusStopIntoView` (`.cpp:3304`):

```cpp
RootScrollBox->ScrollWidgetIntoView(FocusStop, /*AnimateScroll*/ false,
    EDescendantScrollDestination::IntoView, /*Padding*/ 0.0f);
```

⭐ **This is literally the same call `ArmVideoModeCountdown()` already makes for the revert prompt** (now `.cpp:2324`, `RootScrollBox->ScrollWidgetIntoView(VideoModeConfirmBorder.Get(), /*AnimateScroll*/ false)`) — one mechanism on this screen, not two.

The override, in full shape:

```cpp
Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);   // never skipped
if (RootScrollBox == nullptr || !NewWidgetPath.IsValid()) return;               // GetLastWidget() is check(IsValid())
const TSharedPtr<SWidget> ScrollSlate = RootScrollBox->GetCachedWidget();
if (!ScrollSlate.IsValid() || !NewWidgetPath.ContainsWidget(ScrollSlate.Get())) return;  // ring landed elsewhere
UWidget* FocusedStop = FindOwnWidgetForSlateWidget(NewWidgetPath.GetLastWidget());
if (FocusedStop == nullptr) return;
ScrollFocusStopIntoView(FocusedStop);
```

- ⛔ **It is a FOCUS hook, not a KEY handler.** It reads nothing from the keyboard, consumes nothing, returns nothing, decides no navigation. **`NativeOnKeyDown` / `SetKeyboardFocus` / `SetUserFocus` have zero code hits in both files** — the wrong-layer relapse did not happen.
- The `ContainsWidget` guard matters: this same override also fires for the **old** focus path, i.e. when focus *leaves* this screen for Settings underneath, and scrolling then would move a list the player is no longer looking at.
- The `FocusedStop == nullptr` early-out is **not a gap**: the focus leaf can be an inner Slate widget with no `UWidget` of its own, and Mechanism A operates on that Slate widget **directly** and cannot miss it. B is the explicit, loggable half; A is the exhaustive half. They request the **same destination with the same animation flag**, so they cannot disagree and the result does not depend on which Slate services last.
- `FindOwnWidgetForSlateWidget` uses **`ForEachWidget`, not `ForEachWidgetUntil`** — measured, not stylistic: `WidgetTree.h:78` declares `ForEachWidget` with `UMG_API` and `:96` declares `ForEachWidgetUntil` **without** it, so the early-out variant is not exported from UMG and would compile cleanly and **fail at LINK**. The `Found != nullptr` guard is the early-out, done by hand.

### THE INSTRUMENT, AND ITS HONESTY CAVEAT

`UE_LOG(LogSiegeGraphics, Verbose, "[GraphicsMenu] Focus moved to '%s' (%s) - asked RootScrollBox to scroll it into view (offset before the request: %.1f).")`

⚠️ **The offset printed is the one BEFORE the request, and it is labelled as such.** `ScrollWidgetIntoView` *queues* a request that `SScrollBox` services on its next Tick once it has geometry; reading `GetScrollOffset()` on that line and calling it the result would be a fabricated measurement.
🚨 **`Log LogSiegeGraphics Verbose` FIRST, or this line prints nothing — and AN EMPTY LOG READS AS A FALSE PASS.** The retarget/stop-count line is `LogSiegeMenuInput` at **`Log`** verbosity (visible by default in a Development editor); only this per-focus line needs raising.

---

## 3. REGISTER / UNREGISTER — **BESIDE THE COUNTDOWN TEARDOWN, NEVER INSTEAD OF IT**

| Site | File:line | Order |
|---|---|---|
| **REGISTER** | `NativeConstruct()` — `.cpp:1617` | **LAST**, after `SeedAndBind()` **and** `ArmFrameRateReadout()` |
| **UNREGISTER (ordinary exit)** | `BackPressed()` — `.cpp:2141` | **AFTER** `DisarmVideoModeCountdown()` + `DiscardStagedVideoMode(...)`, **BEFORE** `RemoveFromParent()` |
| **UNREGISTER (catch-all)** | `NativeDestruct()` — `.cpp:1632` | **FIRST statement**, LIFO, before `UnbindAll()` |

- ⛔ **`BackPressed()` keeps both original lines, unchanged, first.** `DisarmVideoModeCountdown()` and `DiscardStagedVideoMode(ResolveGraphicsSubsystem())` still run exactly as `TASK-1118` shipped them. **No third revert, no second facade call, count unchanged.**
- ⛔ **`NativeDestruct()` replaces nothing below it.** `UnbindAll()`, `DisarmFrameRateReadout()`, `DisarmVideoModeCountdown()`, `DiscardStagedVideoMode()` all still run, in their original order, for their original reasons.
- ⛔ **Unregister before `RemoveFromParent()`** is the subsystem's own stated contract (`UnregisterMenuNavTarget`'s closing comment: *"an unregister runs from `BackPressed`, BEFORE `RemoveFromParent`"*).
- Both firing is normal and safe: `UnregisterMenuNavTarget` removes by **identity** and **logs, not warns**, a second call.

### ⭐ WHY REGISTER IS **AFTER** `SeedAndBind()` — AND IT IS THE SAME HAZARD `TASK-1415` FLAGGED, ONLY BIGGER

`RegisterMenuNavTarget()` logs the stop count **and** places the ring on stop 0, and both read the tree's **live enabled state**. `SeedAndBind()` is what settles it: on the null-facade path it calls `ShowPanelUnavailable()` → `SetAllControlsEnabled(false)`, which disables **22 of 24**. Registering first would log **24** for a screen that has **2** and could park the ring on a control disabled a few lines later. The dispatch asked me to check whether this screen has the same hazard: **it does, and it is worse here than on Settings (22 rows vs 1).**

### ⭐ NESTING — THE HALF `TASK-1415` DELIBERATELY LEFT FOR THIS ROW, AND IT IS NOW SUPPLIED

`USettingsMenuWidget::GraphicsPressed()` is intentionally **not** an unregister site. With this row landed the stack reads:

```
register(Settings)    -> [Settings]              ring on Settings
register(Graphics)    -> [Settings, Graphics]    ring on Graphics   ← THIS ROW
unregister(Graphics)  -> [Settings]              ring BACK on Settings ← THIS ROW
```

`TASK-1406`'s `UnregisterMenuNavTarget` re-places focus **only when `GetRegisteredNavTarget()` is still non-null** — exactly this case — so **Graphics' Back hands the ring back to the SETTINGS panel underneath, ⛔ not to `WBP_MainMenu` two layers down.** Settings was never removed (it is at ZOrder 10; Graphics is added on top at ZOrder 20), so the ring lands on a live screen.

---

## 4. THE COUNTDOWN — **STATE THAT YOU CHECKED** ✅

Spec (4) requires that a navigation change must **NOT disarm, extend or re-enter** the 10-second keep-or-revert countdown (`GFX-§`). Checked, three ways:

1. **`ArmVideoModeCountdown()` / `DisarmVideoModeCountdown()` / `TickVideoModeCountdown()` / `VideoModeCountdownTimerHandle` / `bVideoModeCountdownActive` / `VideoModeCountdownSecondsRemaining` are UNTOUCHED.** `git diff --numstat` = **0 deletions**, and a bounded grep of this row's entire new section (`.cpp:3114`–`:3358`) for `Timer`, `Arm`, `Disarm`, `Apply`, `Save`, `Broadcast`, `SetVisibility`, `SetIsEnabled`, `Graphics->`, `Settings->` returns **nothing but two comment mentions**.
2. **The only new call on the Back path is `UnregisterAsMenuNavTarget()`, placed AFTER the disarm + discard.** It cannot re-arm, cannot extend, cannot re-enter — it removes one weak pointer from an array in another subsystem.
3. **Keep and Revert are still focus stops "like any other"** — they are #18 and #19 in the list above, and nothing here changed their focusability, their bindings, or their handlers.

### ⚠️ `bSuppressRowEcho` — THE LATCH THE DISPATCH ASKED ABOUT, ANSWERED

The latch is raised at **`.cpp:1924`** and lowered at **`.cpp:2063`** (re-measured after this row's own insertions; the dispatch's `:1852` / `:1991` are the pre-edit numbers and are now stale). Both are inside `RefreshAllRows()`, whose only reachable caller from the construct path is `SeedAndBind()` — **a synchronous, non-re-entrant span that has RETURNED before `RegisterAsMenuNavTarget()` runs.**

**⇒ NO registration work can run between the set and the clear.** The only callback registration can trigger is the focus change from `FocusFirstNavStop()`, and this screen's handler for that (`NativeOnFocusChanging`) asks the scroll box for a **layout** change and touches no row, no slider and no facade. **The latch cannot be latched by anything this row added, and no keyboard commit can be swallowed in silence because of it.**

---

## 5. ⚖️ **THE ONE FLAGGED DECISION — DECLARED, NOT SELF-ADJUDICATED (`SC-§101`)**

### 🚨 KEEP / REVERT ARE ADMITTED AS FOCUS STOPS **WHILE THE PROMPT IS COLLAPSED**, AND I DID **NOT** FIX IT

**The measurement, at engine source, not an inference:**
- `KeepSettingsButton` and `RevertSettingsButton` live inside `VideoModeConfirmBorder`, which is `ESlateVisibility::Collapsed` whenever no countdown is running (the 99 % case; four documented writer sites, now `.cpp:1428` (build) / `:1689` (seed) / `:2316` (arm, the only one that makes it VISIBLE) / `:2377` (disarm)).
- `USiegeMenuInputSubsystem::IsNavFocusStop` tests **`Widget->IsVisible()`**.
- **`UWidget::IsVisible()` returns `GetCachedWidget()->GetVisibility().IsVisible()`** (`UMG/Private/Components/Widget.cpp`) — the widget's **OWN** Slate visibility. `SWidget::GetVisibility()` is `VisibilityAttribute.Get()` (`SlateCore/Public/Widgets/SWidget.h:1065`). **Neither consults ancestors.**
- A `Collapsed` **parent** does not change a child's own `EVisibility::Visible`, and UMG builds the child's Slate widget regardless.

**⇒ the ring stops TWICE on rows the player cannot see, between `WindowModeNextButton` (#17) and `VSyncCheckBox` (#20).** That is exactly the "ring the player cannot see" shape spec (2) names — so it is reported loudly rather than buried.

**Why I did not take the fix:** the manager's own handling of the *sibling* case in this dispatch is the precedent — the stepper double-stop is a walker-level over-admission and is *"routed to the walker, not fixable inside your fence, report it in your stop list."* **This is the same category:** the walker does not look at ancestor visibility. The two available remedies are (a) **the walker learns ancestor visibility** — `SiegeMenuInputSubsystem.{cpp,h}`, `TASK-1406`/`TASK-1409`'s file, **fenced from this row**; or (b) **mirror the border's visibility onto the two buttons** at the four documented `WRITER n OF 4` sites — which adds two writers to the countdown's surface that spec (4) told this row to leave alone. **⛔ Escalate rather than widen.** The `TASK-1415` L4 precedent (the `RemoveFromParent()` override that *was* available and was *not* taken, and QA ruled the author right) is the shape I followed.

**The exact patch, so a ruling is cheap:** add a private `void SetVideoModeConfirmRowVisible(bool bVisible)` that sets the border **and** `KeepSettingsButton` / `RevertSettingsButton` to the same `ESlateVisibility`, and call it at the four writer sites in place of the bare `VideoModeConfirmBorder->SetVisibility(...)`. It disarms nothing, extends nothing and re-enters nothing — but it is **four edits to `TASK-1118`'s documented invariant**, so it is QA's call, not mine.

**If QA rules (a):** the expected stop count becomes **22** on the healthy path (drop #18 and #19) and **2** on the unhappy path. `TASK-1421` should be told which number it is falsifying.

---

## 6. WHAT QA SHOULD SCRUTINISE

1. **The two-phase slider fence.** I claim **ZERO second facade writers**. Falsifier: any `Graphics->`, `SetQuality*`, `Apply*`, `Save*` or `Broadcast` inside `.cpp:3114`–`:3358` (the section runs from its banner at `:3114` to the end of `FindOwnWidgetForSlateWidget`, immediately above `SetGraphicsSubsystemForAutomationTests` at `:3360`). My bounded grep returns two comment mentions and no code. Please re-run it rather than take it.
2. **§5, the flagged decision.** It is the one place I chose to ship a known ring defect rather than widen. Overrule me if the fence is wider than I read it.
3. **`NativeOnFocusChanging` is an override of a `UMG_API virtual`** declared in `protected:` here while public on the base. Legal (access is checked on the static type, and Slate calls it through `UUserWidget*` in `SObjectWidget.cpp:207`), but worth a second pair of eyes.
4. **`ForEachWidget` vs `ForEachWidgetUntil`.** I claim the latter is unexported (`WidgetTree.h:96` lacks `UMG_API`) and would fail at link. If that reading is wrong, the early-out variant is the better code.
5. **The two `ConfigureScrollFollowsFocus()` calls.** I claim the second is needed only for a hypothetical asset-authored tree. If QA reads it as dead code, deleting the `NativeConstruct` one costs one line and loses only condition-(b) coverage.
6. **`EScrollWhenFocusChanges::InstantScroll`.** I originally wrote `NoAnimateScroll` from memory, checked `SScrollBox.h:63`, and found the enum has only `NoScroll` / `InstantScroll` / `AnimatedScroll`. Recorded because it is exactly the class of error a reviewer should assume is still present elsewhere.

---

## 7. FENCES HELD

- ✅ **`SiegeGraphicsMenuWidget.{cpp,h}` ONLY.** Marker census over `Source/`: `TASK-1417` appears in **3 files** — `SiegeGraphicsMenuWidget.cpp` (9) and `.h` (4), **both mine**, plus `SettingsMenuWidget.cpp` (2). ⚠️ **THOSE TWO ARE NOT MINE AND I AM SAYING SO RATHER THAN REPORTING A CLEAN ZERO:** they are `TASK-1415`'s own forward reference at `:644` / `:647` (*"it is USiegeGraphicsMenuWidget registering ITSELF (TASK-1417)"*), and `git diff` shows both lines carrying a `+` in **that row's** uncommitted diff, not mine. **ZERO** in `SiegeMenuInputSubsystem.{cpp,h}` (measured, both files), `SettingsMenuWidget.h`, `CONVENTIONS.md` or any other screen. ⚠️ What that does **not** entitle: it cannot detect an *unmarked* byte edit — the zero-deletion `numstat` above is the stronger claim, and both of the fenced files are untouched in `git status` beyond the pre-existing `TASK-1409`/`TASK-1415` modifications that were already there when I started.
- ✅ **NO new key handler.**
- ✅ **NO change to any quality-group value, detent or apply path.** `BuildSliderRow`'s `StepSize` / `bDetented` arguments, `MouseUsesStep`, `LevelToSliderValue` / `SliderValueToLevel`, and every `Handle*Committed` body are byte-identical.
- ✅ **NO compile, NO PIE, NO MCP, NO asset, NO git, NO editor lifecycle action.** I ride the existing convoy; I opened no second build.
- ✅ **`GitClaudeUnrealTest.Build.cs` NOT touched** — `SlateCore` is already a public dependency, which is what `FWidgetPath::ContainsWidget` (`SLATECORE_API`) needs to link.

---

## 8. ## Not examined / limitations

- **L1 — NOTHING HERE HAS BEEN COMPILED OR RUN.** The stop count is a **PREDICTION**, not a reading.
- **L2 — THE PIXEL LIMB IS NOT MINE.** `VER-§11` cl. 9: a ringless capture is **`UNOBSERVABLE`**, ⛔ never `VERIFY-FAILED`, ⛔ never this row's failure. I own the STATE limb only.
- **L3 — REACHABILITY DEPENDS ON `TASK-1415` BEING LIVE.** This screen's only route is Settings' `GraphicsButton`, and Accept there was measured **inert** before `TASK-1415`. If `TASK-1415` is not in the same binary, 5b cannot reach this screen at all and owes `UNOBSERVABLE`, not a fail.
- **L4 — `ScrollWidgetIntoView` AT `NativeConstruct` TIME IS DEFERRED, NOT IMMEDIATE.** `RegisterMenuNavTarget` places the ring during `NativeConstruct`, before the first layout pass; `SScrollBox` services the queued request on a later Tick. I expect the panel to open scrolled to the top with stop 0 already in view (it is the first row), so this should be invisible — but it is **not measured**.
- **L5 — WARN-8 (pad double-detent) IS NOT MINE AND IS NOT INTRODUCED HERE.** After a pad Accept lock, Slate's own `OnNavigation` may also step ⇒ possibly two detents per press with a gamepad. Not a corruption (the handlers diff against the facade, which refuses a no-op). Routed to the verifier.
- **L6 — THE `UCanvasPanel` CAVEAT DOES NOT APPLY HERE, BUT IS RECORDED.** `GetMenuFocusStops` walks **slot** order, which is authoring order, not laid-out visual order. On this screen the two coincide **by construction** (one `UVerticalBox` filled in reading order inside a vertical `UScrollBox`). A future asset-authored `WBP_GraphicsMenu` using a `UCanvasPanel` would break that correspondence silently, and the walker cannot detect it.
- **L7 — THE 24 ASSUMES ALL TEN QUALITY GROUPS BUILD.** The list comes from `USiegeGraphicsSettingsSubsystem::GetQualityGroupNames()` (ten names, `SiegeGraphicsSettingsSubsystem.cpp:84-99`) and each row is built only if `ResolveGroupRow(...).IsComplete()`. A group added to or removed from the facade moves the count by one, automatically and correctly.
- **L8 — `IA_MenuBack` BEHAVIOUR ON THIS SCREEN IS NOT MINE AND I BUILT NONE.** `USiegeGraphicsMenuWidget` does **not** implement `ISiegeMenuNavCloseTarget`, so Back-by-key is expected **inert** here. If 🧑 he wants Backspace to close Graphics, that is a new row, not a silent addition to this one.
- **L9 — I DID NOT VERIFY THAT `SScrollBox::OnFocusChanging` FIRES ON THE EXACT FRAME THE SUBSYSTEM'S DELAYED-FOCUS FALLBACK RESOLVES.** `DelayedSlateOperations.SetUserFocus(...)` (`SiegeMenuInputSubsystem.cpp:1406`) still routes through `FSlateApplication`, so the dispatch is the same; the **timing** is not measured.
