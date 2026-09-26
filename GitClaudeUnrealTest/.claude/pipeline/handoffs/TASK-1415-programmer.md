# TASK-1415 — [MENU-NAV-SETTINGS] — programmer handoff

marker `TASK-1415-MENU-NAV-SETTINGS`
author: gameplay-programmer · 2026-09-24
routing: `ready-for-qa` → **TASK-1416** (QA) → 5a → 5b = **TASK-1421** → commit **TASK-1422**

> 🧑 *"I noticed there also does not exist an outline that is scrollable with buttons in the setting
> menu either… we want to make sure everywhere in the menu can be scrollable with the outline and
> arrow keys such that an agent can navigate the entire menu."*

---

## 0. THE HEADLINE — THE EXPECTED STOP COUNT IS **3**, AND THE NAMES ARE

| ring position | widget (pinned name) | class |
|---|---|---|
| **stop 0** | `ConfirmToggleCheckBox` | **`UCheckBox`** ← 🧑 **the control a `UButton`-only walker would have skipped** |
| **stop 1** | `GraphicsButton` | `UButton` |
| **stop 2** | `BackButton` | `UButton` |

**Stated BEFORE 5b so TASK-1421 can falsify it.** The (5) retarget line TASK-1406 emits on my
`Register` call must read, at `LogSiegeMenuInput`/`Log`:

```
[USiegeMenuInputSubsystem] menu nav target registered -> '<SettingsMenuWidget…>' (registered screen), 3 focus stop(s), 1 screen(s) registered.
```

- `3` is the number. **`2` means the check box was dropped — that is the defect this milestone exists
  to prevent** (one benign cause exists and is named in §4).
- `7` with a name reading `WBP_MainMenu` means the registration did not take (see §6 limitation L2).
- `(registered screen)`, not `(default: WBP_MainMenu)`.

⭐ **This discharges spec (3b) — the criterion moved here from TASK-1406 by the manager (`TASK-1407`
WARN-7): _a registered NON-main-menu target reports a focus-stop count > 0_. It is met by
`USettingsMenuWidget::NativeConstruct()` calling the product's own registration — ⛔ **not** by a
verifier calling `RegisterMenuNavTarget` itself (the `SC-§137` `MEASURED`-at-best trap). I am the
first caller of that API in the tree; before this row the registered branch was unreachable at
runtime.**

---

## 1. FILES TOUCHED — TWO, BOTH MINE, AND THE DIFF IS **PURE ADDITION**

| file | +/− |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.cpp` | **+141 / −0** |
| `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.h` | **+66 / −0** |
| **total** | **+207 / −0** |

`git diff --numstat` reports **zero deletions**, which is a stronger statement than "I was careful":
**every pre-existing line in both files is byte-identical to `HEAD` (`40c824b`).** Nothing was
re-authored, nothing was reflowed, no existing behaviour was edited — the three insertion points are
three added call lines plus comments.

⛔ **`SiegeMenuInputSubsystem.{cpp,h}` was READ ONLY and NOT WRITTEN** (it is TASK-1406's file and
TASK-1409 is editing it right now). ⛔ No other screen's file. ⛔ No compile, no PIE, no MCP, no
asset, no git, no editor lifecycle action — PID 32984 was never touched.

Of the 207 added lines, **13 are executable**; the rest are comments carrying the measurements below.

---

## 2. WHERE I REGISTER AND UNREGISTER

### Register — ONE site

`NativeConstruct()`, **after `SeedAndBind()`**:

```cpp
	SeedAndBind();
	…
	RegisterAsMenuNavTarget();          // → MenuInput->RegisterMenuNavTarget(this)
```

**The order is load-bearing, not stylistic.** `RegisterMenuNavTarget` logs the stop count *and*
places the ring, and both read the tree's live enabled state. `SeedAndBind()` is what settles that
state: on the unhappy path it calls `ShowRowUnavailable()`, which does
`ConfirmToggleCheckBox->SetIsEnabled(false)`. Registering first would log `3` for a screen that
really has 2 stops and could park the ring on a control disabled one line later.

### Unregister — TWO sites, both spec'd, and the double call is safe by the API's own contract

1. **`BackPressed()`** — inserted **between the existing log line and `RemoveFromParent()`**, i.e.
   *before* the removal. That order is the subsystem's own stated contract, quoted from
   `SiegeMenuInputSubsystem.cpp`'s `UnregisterMenuNavTarget` closing comment: *"an unregister runs
   from `BackPressed`, BEFORE `RemoveFromParent`"*.
2. **`NativeDestruct()`** — first statement, before `UnbindAll()` (LIFO against `NativeConstruct`).
   This is the catch-all for every non-`BackPressed` way the panel can stop existing.

Both firing on an ordinary Back **is the normal case and is safe**: `UnregisterMenuNavTarget` removes
by *identity* and, for a screen already off the stack, logs (does not warn) *"not registered (already
unregistered, or never was) — no change."* The subsystem's own comment names this exact pairing as
the reason that branch exists.

⛔ I did **not** rely on TASK-1406's dead-entry re-validation as a substitute for unregistering. It is
a net; both paired sites are present.

### ⛔ NOT a site: `GraphicsPressed()` — and its absence is the nesting fix

Trap #2 from the dispatch, answered directly. `GraphicsPressed()` stacks Graphics at ZOrder 20 and
**does not remove this panel**. `NavTargetStack` is a stack, so the correct shape is:

```
  register(Settings)   -> [Settings]              ring on Settings
  register(Graphics)   -> [Settings, Graphics]    ring on Graphics       (TASK-1417's call)
  unregister(Graphics) -> [Settings]              ring BACK on Settings  ← hands back HERE
```

`GetRegisteredNavTarget()` scans **top-down** for the first live entry, and
`UnregisterMenuNavTarget` explicitly calls `FocusFirstNavStop()` *when another registered screen is
taking over*. So Graphics' Back returns the ring to **Settings**, not to the main menu two layers
down. Unregistering in `GraphicsPressed()` would have broken exactly that. A comment block at the
insertion point records this so nobody "fixes" it later.

⚠️ **The other half of the nesting is not in my file:** `USiegeGraphicsMenuWidget` must register
itself (**TASK-1417**). Until that row lands, opening Graphics leaves *Settings* on top of the stack
and the ring stays on Settings underneath Graphics — navigable, wrong, and the reachability debt the
board already records against TASK-1417, not a defect here. Conversely **this row unblocks
TASK-1417's verifiability**: Graphics' only route in is my `GraphicsButton`, which is now stop 1 and
reachable by `IA_MenuDown` + Accept.

---

## 3. THE CONTROL CENSUS — EVERY WIDGET IN THE TREE ACCOUNTED FOR

The tree is built by `ConstructSettingsTree()` (`.cpp:111`+). `GetMenuFocusStops` walks it with
`UWidgetTree::ForEachWidget` = **depth-first pre-order**, `UPanelWidget` children in
`GetChildAt(0..N-1)` slot order (re-measured this row at
`UMG/Private/WidgetTree.cpp:203` `ForEachWidget` → `:246` `ForWidgetAndChildren`). `UButton` and
`UCheckBox` both derive `UContentWidget : UPanelWidget` (`CheckBox.h:31`, `ContentWidget.h:12`), so
their content text blocks **are** visited — and correctly rejected.

Full traversal, all **10** widgets, in order:

| # | widget | class | stop? | why |
|---|---|---|---|---|
| 1 | `BackdropBorder` | `UBorder` | ✗ | not one of the four admitted classes (it is the hit-test-visible modal plate + tree root) |
| 2 | `RootPanel` | `UVerticalBox` | ✗ | layout panel, not admitted |
| 3 | `TitleText` | `UTextBlock` | ✗ | static text, not admitted |
| 4 | **`ConfirmToggleCheckBox`** | **`UCheckBox`** | ✅ **stop 0** | admitted class; enabled + visible; `IsFocusable` at its engine default `true` |
| 5 | `ConfirmToggleLabelText` | `UTextBlock` | ✗ | the check box's *content* — walked, not admitted |
| 6 | `ConfirmToggleHintText` | `UTextBlock` | ✗ | static text |
| 7 | **`GraphicsButton`** | **`UButton`** | ✅ **stop 1** | admitted; `IsFocusable` default `true` |
| 8 | `GraphicsLabelText` | `UTextBlock` | ✗ | button content |
| 9 | **`BackButton`** | **`UButton`** | ✅ **stop 2** | admitted; `IsFocusable` default `true` |
| 10 | `BackLabelText` | `UTextBlock` | ✗ | button content |

**Spec (2) cross-check — the three controls the board named, by line:** `UCheckBox` `:225` =
`ConfirmToggleCheckBox` ✅ · `UButton` `:302` = `GraphicsButton` ✅ · `UButton` `:349` = `BackButton`
✅. (Those line numbers are `HEAD`'s; my insertions are all below `:380`, so they are unmoved.)
**All three are focus stops. None is excluded, so there is no "why not" owed.**

**Slot order == visual order here, by construction:** `RootPanel` is a `UVerticalBox` whose
`AddChildToVerticalBox` calls run in source order (TitleText → check box → hint → Graphics → Back),
so the ring walks **top-to-bottom as 🧑 he sees it**. This screen dodges the `UCanvasPanel` /
designer-authored caveat TASK-1406 declared.

### Fence (c) verified at source — all three controls are inside the four admitted classes

⛔ The dispatch required this to be checked, not inherited. `IsNavFocusStop`
(`SiegeMenuInputSubsystem.cpp:333`) admits `UButton`, `UCheckBox`, `USlider`, `UEditableTextBox`.
This screen uses only `UCheckBox` and `UButton` — **both admitted**. **Nothing on this screen falls
outside the vocabulary, so there is nothing to escalate and I did not widen the walker.**

### Opt-out audit — no `IsFocusable` write exists on this screen, and the defaults are `true`

- `grep -n "IsFocusable" SettingsMenuWidget.{cpp,h}` → **zero hits outside my own new comments.**
- Engine defaults measured: `UButton::UButton` sets `IsFocusable = true` (`Button.cpp:48`);
  `UCheckBox::UCheckBox` sets `IsFocusable = true` (`CheckBox.cpp:41`). `ConstructWidget<T>` copies
  the CDO, so all three controls carry `true`.

---

## 4. 🚨 THE TWO MEASURED TRAPS FROM THE DISPATCH

### Trap 1 — `qa/TASK-1407.md` WARN-2, the asset-authored `IsFocusable` opt-out. **RE-MEASURED, NOT INHERITED (`SC-§138`).**

The WARN-2 census was `Source/`-only and cannot see a `.uasset`-authored opt-out. TASK-1398 says
Settings has no asset; **I confirmed it myself rather than taking that on trust:**

- `ls Content/UI/` → 10 `WBP_*.uasset`: `WBP_CardHand`, `WBP_CastleHealthBar`,
  `WBP_CombatantHealthBar`, `WBP_DeckBuilder`, `WBP_DeckCardTile`, `WBP_HUD`, `WBP_MainMenu`,
  `WBP_SessionMenu`, `WBP_VictoryScreen`, `WBP_WarMap`. **`WBP_SettingsMenu` is not among them.**
- `find Content -iname "*Settings*"` → **2 hits, both unrelated**
  (`Fire_Magic/…/Mannequin_LODSettings.uasset`, `Ice_Magic/…/Mannequin_LODSettings.uasset`).

⇒ `/Game/UI/WBP_SettingsMenu` is **reserved and unauthored**, `WidgetTree->RootWidget` is null when
`ConstructSettingsTree()` runs, the condition-(b) escape hatch does **not** fire, and the
code-authored branch is the live path. **There is no `.uasset` on this screen that could carry a
hidden `IsFocusable=False`, so the WARN-2 failure mode ("registers and reports ZERO stops while every
code read looks correct") cannot occur here.** This is measurement, not inference.

### Trap 2 — nesting, and the stack. **Answered in §2** (no unregister in `GraphicsPressed`;
unregister hands back to Settings, not to the main menu). I did **not** lean on the re-validation
net: both paired unregister sites exist.

### The one benign way the count is 2 instead of 3 — declared up front so it is not read as the defect

`ShowRowUnavailable()` (`.cpp:616`) fires when `USiegeSettingsSubsystem` cannot be resolved and does
`ConfirmToggleCheckBox->SetIsEnabled(false)`. `IsNavFocusStop` rejects disabled widgets, so the count
would legitimately read **2** (`GraphicsButton`, `BackButton`). **That is correct behaviour** — a
dead row must not eat a keypress — and it is exactly why I register *after* `SeedAndBind()`. It is
distinguishable in the log without ambiguity: the same run emits
`[SettingsMenu] USiegeSettingsSubsystem could not be resolved - the confirm row is disabled.` at
`Warning`. **A `2` with that Warning present = healthy. A `2` without it = the defect.**

---

## 5. WHAT TASK-1421 (5b) SHOULD DRIVE — BOTH LIMBS, NEITHER SUBSTITUTING FOR THE OTHER

### STATE limb (the gate — machine-checkable)

Runtime criterion: *with Settings open, `inject_input_action IA_MenuDown` MOVES `focused: true`
across its own controls (incl. at least one non-`UButton`).*

⚠️ **EXPECT THE RING TO BE ON ALREADY.** TASK-1406's `RegisterMenuNavTarget` ends in
`FocusFirstNavStop()`, so Settings **opens with `ConfirmToggleCheckBox` (stop 0) already focused**.
The **first** `IA_MenuDown` therefore moves to **stop 1 = `GraphicsButton`**, not to stop 0. A row
expecting stop 0 after one Down will mis-read a working screen as broken.

| after | expected `focused: true` |
|---|---|
| Settings opens (no input) | `ConfirmToggleCheckBox` |
| Down ×1 | `GraphicsButton` |
| Down ×2 | `BackButton` |
| Down ×3 | `ConfirmToggleCheckBox` (wraps — `WrapIndex`, ring of 3) |

**The non-`UButton` limb is satisfied at open and again at Down ×3: `ConfirmToggleCheckBox` is a
`UCheckBox`.**

Corroborating log lines (all `LogSiegeMenuInput`; **`Log LogSiegeMenuInput Verbose` must be set first
or the `HandleMenuDown` instrument prints nothing and an empty log reads as a false pass**):
`HandleMenuDown() entered` · `focus moved %d -> %d of %d` with **`of 3`**.

### PIXEL limb

The ring itself. ⚠️ **`VER-§11` cl. 9** — *"is a ring present"* is machine-checkable only under a
four-part bar this row does not spec ⇒ **a ringless capture is `UNOBSERVABLE`, never a
`VERIFY-FAILED`, and is not this row's failure.**

### cl. 3(b) first sentence, for 🧑 him, verbatim as the board pins it

> 🧑 Open Settings from the main menu and press Down a few times — does the outline walk every row
> including the checkboxes, not just the buttons at the bottom?

(+ the parenthesis verbatim.)

---

## 6. `## Not examined / limitations`

- **L1 — NOT COMPILED, NOT RUN.** Fenced. I did not compile, did not start PIE, did not touch MCP or
  the editor (PID 32984). Everything in §0/§3 is derived statically from the code-authored tree plus
  `IsNavFocusStop`'s rules. **The stop count 3 is a prediction, not an observation** — that is the
  point of stating it before 5b.
- **L2 — THE CREATION ROUTE IS ASSERTED FROM COMMENTS, NOT READ.** `USettingsMenuWidget` is created
  from `WBP_MainMenu`'s `Btn_Settings` graph (`CreateWidget` → `AddToViewport(ZOrder 10)`, per the
  class comment and GFX-§). **I could not read that Blueprint graph** (MCP is fenced this row), and a
  repo-wide grep finds **no C++ creation site**. This matters because `GetRegisteredNavTarget()`
  re-validates `IsInViewport() && IsVisible()` on every read: if the panel is ever added as a *child
  of another widget* (a named slot) instead of to the viewport, `IsInViewport()` is false and this
  screen is skipped entirely. **Falsifier: the "registered" retarget line naming `WBP_MainMenu` /
  reporting 7 stops.** QA/5b can settle it with one log line.
- **L3 — THE `NativeConstruct`-TIME VIEWPORT ORDERING IS MEASURED IN ENGINE SOURCE, NOT AT RUNTIME.**
  Registering in `NativeConstruct` only works if `IsInViewport()` is *already* true there; otherwise
  `RegisterMenuNavTarget`'s own `FocusFirstNavStop()` would fall through to `FindMainMenuWidget()`
  and put the ring on the main menu. I read UE 5.8: `UGameViewportSubsystem::AddToScreen` sets
  `bIsManagedByGameViewportSubsystem = true` (`GameViewportSubsystem.cpp:158`) and
  `SlotInfo.FullScreenWidget = FullScreenCanvas` (`:177`) **before**
  `RawSlot->AttachWidget(Widget->TakeWidget())` (`:183`), and `TakeWidget` is what runs
  `RebuildWidget → OnWidgetRebuilt → NativeConstruct` (`Widget.cpp:1094-1096`,
  `UserWidget.cpp:1219-1234`). `IsInViewport()` is exactly those two facts (`Widget.cpp:344` →
  `GameViewportSubsystem.cpp:89`), and `IsVisible()` reads a slate widget cached at
  `Widget.cpp:1023` with `SynchronizeProperties()` already run at `:1094`. ⇒ **both true at my call
  site.** Static reading of engine source; not observed live. Same falsifier as L2.
- **L4 — A THIRD EXIT PATH EXISTS IN PRINCIPLE AND IS DECLARED, NOT CLOSED (`SC-§101`).** A caller
  that does `RemoveFromParent()` on this widget *without* going through `BackPressed()` would not hit
  my `BackPressed` site. Measured mitigations: (a) **no such C++ caller exists** — a grep over
  `Source/` finds `RemoveFromParent` used on the assistant console / war map / controls help / FPS
  counter / victory widgets and **never** on the settings panel; (b) this project already has a
  landed measurement that `RemoveFromParent` *is* what causes `NativeDestruct` to run
  (`SiegePlayerController.cpp:582`), so my `NativeDestruct` site catches it; (c) TASK-1406's
  `IsInViewport()` re-validation degrades the gap to "the ring goes back to the menu". **A
  `virtual void RemoveFromParent()` override is available as belt-and-braces (`UWidget::RemoveFromParent`
  IS virtual, `Widget.h:776`) — I did NOT take it**, because the spec named two sites and the fence
  says escalate rather than widen. **QA's call, not mine.**
- **L5 — NO PIXEL CLAIM.** Nothing in this note asserts anything about what appears on screen. The
  ring is the PIXEL limb and `VER-§11` cl. 9 governs it.
- **L6 — GRAPHICS NESTING IS ONLY HALF-BUILT.** Verified statically for the Settings side only. The
  `[Settings, Graphics]` stack shape cannot be observed until **TASK-1417** registers the Graphics
  widget. Testing Graphics nesting at *this* row's 5b would measure a half-wired feature.
- **L7 — ACCEPT ON A CHECKBOX IS STILL A SILENT NO-OP.** TASK-1406 declared it (item (2)): the Accept
  vocabulary did not widen with the focus vocabulary. So `IA_MenuAccept` on stop 0 does nothing
  today; Left/Right (`TASK-1409`) and any checkbox-toggle semantics are **not this row's**, and I
  added none. Reaching stop 0 with the ring is what this row promises.
- **L8 — `IA_MenuBack` IS EXPECTED INERT.** Per the dispatch, it stays inert until screens register
  (`TASK-1454`). **I built no Back key handling of my own** — I registered, and TASK-1409's Back will
  find this screen through the stack.
