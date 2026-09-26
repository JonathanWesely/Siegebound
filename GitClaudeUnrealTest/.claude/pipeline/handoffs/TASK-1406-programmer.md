# TASK-1406 — [MENU-NAV-TARGET-AND-VOCABULARY] programmer handoff (2026-09-24)

Marker: `TASK-1406-MENU-NAV-TARGET-AND-VOCABULARY`
Law: `VER-§8` cl. 2/3(a)/3(b) · `SC-§39` · `SC-§50` · `SC-§101` · `SC-§134` cl. 7(a) · `SC-§126` cite-by-text

**Ran:** `Read` / `Grep` / `Edit` only. ⛔ No compile, ⛔ no PIE, ⛔ no MCP, ⛔ no asset, ⛔ no git, ⛔ no
editor lifecycle action. The running editor (PID 5728) was never touched or read.
**Wrote:** `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp` · `.h` · this file ·
this row's own `status:` line on `TASKBOARD.md`.

**Diff shape:** `.cpp` **+350 / −18** · `.h` **+145 / −2**. Every one of the 20 deleted lines is
inside `MoveFocus` (10), `HandleMenuAccept` (2) or `FocusButton`'s body (6, moved verbatim into
`FocusWidget`), plus the 2 header lines that grew a doc-comment. ⛔ **No line belonging to
`TASK-1394` or `TASK-1400` was deleted, moved or re-worded.**

---

## 0. HEADLINE — WHICH WIDGET CLASSES THE WALKER NOW ADMITS

⭐⭐ **`UButton` · `UCheckBox` · `USlider` · `UEditableTextBox`** — the four measured control types
from `TASK-1398` §3 F4. That is fence (c), and it is what unblocks Settings: the screen 🧑 he named
is built from `UCheckBox` (`SettingsMenuWidget.cpp:225`) and `UButton` (`:302`, `:349`), and Graphics
adds the `USlider` (`SiegeGraphicsMenuWidget.cpp:1052`). A `UButton`-only walker would have collected
Back and Graphics and skipped every checkbox and the slider — implemented-looking, and his exact
complaint.

⭐ **Both fences moved in one commit, as the row required.** Fence (b) = the handlers stopped asking
*"is the main menu uncovered?"* and started asking *"which screen owns menu navigation right now?"*.
Fence (c) = the walker stopped being `UButton`-only. ⛔ **Fence (a) — the `L_MainMenu` map gate — is
untouched; see §7.**

---

## 1. (2) THE REGRESSION CONTRACT — THE BRANCH, NAMED, AND SHOWN IN THE DIFF

**The contract:** with **nothing registered**, behaviour is byte-for-byte today's.

**The branch is `GetRegisteredNavTarget() == nullptr`, and it is consumed in exactly two places.**
Both are new functions; neither of the two old predicates was edited.

```cpp
UUserWidget* USiegeMenuInputSubsystem::GetActiveNavTarget() const
{
	if (UUserWidget* Registered = GetRegisteredNavTarget())
	{
		return Registered;
	}
	return FindMainMenuWidget();          // ⭐ THE DEFAULT — the same call, the same body
}

bool USiegeMenuInputSubsystem::IsNavTargetActionable() const
{
	if (GetRegisteredNavTarget() != nullptr)
	{
		return true;
	}
	return IsMenuUncovered();             // ⭐ THE DEFAULT — the same call, the same body
}
```

### 1.1 The three call sites, before → after, verbatim

| Site | BEFORE | AFTER |
|---|---|---|
| `MoveFocus` gate | `if (!IsMenuUncovered())` | `if (!IsNavTargetActionable())` |
| `MoveFocus` walker | `TArray<UButton*> Buttons;`<br>`GetMenuButtons(Buttons);` | `TArray<UWidget*> Stops;`<br>`GetMenuFocusStops(Stops);` |
| `MoveFocus` read | `if (const UButton* Focused = GetFocusedMenuButton())` | `if (const UWidget* Focused = GetFocusedNavStop())` |
| `MoveFocus` act | `FocusButton(Buttons[Next]);` | `FocusWidget(Stops[Next]);` |
| `HandleMenuAccept` gate | `if (!IsMenuUncovered())` | `if (!IsNavTargetActionable())` |
| `HandleMenuAccept` read | `UButton* Focused = GetFocusedMenuButton();` | `UButton* Focused = Cast<UButton>(GetFocusedNavStop());` |
| `FocusButton` | (30-line body) | `return FocusWidget(Button);` — body moved verbatim into `FocusWidget`, parameter type the only edit |

⛔ **That is the whole behavioural diff.** Everything else added is new, unreached-by-default code.

### 1.2 Byte-identity, PROVEN by comparison against `HEAD` (`40c824b`), not asserted

I extracted each function from `git show HEAD:` and `diff`ed it against the working copy:

| Function | Result |
|---|---|
| `OnWorldBeginPlay` (**holds fence (a)**) | **UNCHANGED**, 115 lines |
| `IsMenuUncovered` | **UNCHANGED**, 30 lines |
| `GetMenuButtons` | **UNCHANGED**, 20 lines |
| `GetFocusedMenuButton` | **UNCHANGED**, 19 lines |
| `FindMainMenuWidget` | **UNCHANGED**, 22 lines |
| `HandleMenuUp` | **UNCHANGED**, 4 lines |
| `ApplyInitialFocus` | **UNCHANGED** — `diff` empty, guard and all |

⚠️ **THE ONE PLACE THE CONTRACT IS AN ARGUMENT RATHER THAN AN IDENTITY,** and I am naming it rather
than letting QA find it: the **vocabulary** widening is *not* conditioned on the branch — the default
target is walked by `GetMenuFocusStops` too. Widening can only **add** stops, so the default path is
identical **iff `WBP_MainMenu`'s tree contains no `UCheckBox` / `USlider` / `UEditableTextBox`**. Two
independent **live** reads say it contains exactly seven `UButton`s and nothing else interactive:
`qa/TASK-1399-verify.md` §5.4 (`Button_0..6`, labels in order, from a live `ui_snapshot`) and
`qa/TASK-671-verify.md` (`focused:false` enumerated over `Button_0..Button_6` **and** `VerticalBox_0`).
Neither is a *complete* tree census, and I could not take one (no PIE, no MCP). See §8.
⛔ **I did NOT condition the widening on the branch to make the sentence literally true**, because a
`UButton`-only default walker beside a widened registered walker is two behaviours for one function —
exactly the "compiles, reviews clean, does nothing" shape this epic exists to remove.

---

## 2. (3) FENCE (c) — THE FOUR COLLECTED TYPES, AND THE OPT-OUT HONOURED

```cpp
bool USiegeMenuInputSubsystem::IsNavFocusStop(const UWidget* Widget)
{
	if (!Widget || !Widget->GetIsEnabled() || !Widget->IsVisible())  // the same three liveness
	{                                                                // conditions, same order,
		return false;                                                // as GetMenuButtons
	}
	if (const UButton*   Button   = Cast<const UButton>(Widget))   { return Button->GetIsFocusable(); }
	if (const UCheckBox* CheckBox = Cast<const UCheckBox>(Widget)) { return CheckBox->GetIsFocusable(); }
	if (const USlider*   Slider   = Cast<const USlider>(Widget))   { return Slider->IsFocusable; }
	if (Widget->IsA<UEditableTextBox>())                           { return true; }
	return false;
}
```

| # | Class | Focus opt-out read as | Engine citation (UE 5.8) |
|---|---|---|---|
| 1 | `UButton` | `GetIsFocusable()` | `Components/Button.h:156`; the field was deprecated in 5.2 (`:67-70`) |
| 2 | `UCheckBox` | `GetIsFocusable()` | `Components/CheckBox.h:131`; field deprecated in 5.2 (`:70-73`) |
| 3 | `USlider` | **the public member `IsFocusable`** | `Components/Slider.h:94-96` — `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction") bool IsFocusable;`, **public, no getter, NOT deprecated**. ⚠️ Copy-pasting `GetIsFocusable()` here would not compile; this asymmetry is real and is commented at the call site |
| 4 | `UEditableTextBox` | **nothing — none exists** | `Components/EditableTextBox.h` has **zero** occurrences of "focusable" (measured). A text box is keyboard-focusable by construction, so there is no authored opt-out to honour and none was invented |

### 2.1 The three cited opt-out sites, each accounted for

| Site | What it opts out | Honoured how |
|---|---|---|
| `SiegeControlsHelpWidget.cpp:177` | a `UButton` (`CloseButton`, applied at `:2829`) via the deprecated field under `PRAGMA_DISABLE_DEPRECATION_WARNINGS` | ✅ `GetIsFocusable()` reads that same field ⇒ the button is **not** collected |
| `SiegeControlsHelpWidget.cpp:2240` | `DetailScrollBox` (`UScrollBox`) | ✅ vacuously — `UScrollBox` is **not one of the four classes**, so the walker never admits it regardless |
| `SiegeControlsHelpWidget.cpp:2787` | `RowScrollBox` (`UScrollBox`) | ✅ same |

⛔ **No opt-out is stomped.** A widening that overrode an author's `IsFocusable=false` would be a
regression wearing a widening's clothes.

### 2.2 A second, independent backstop

`FocusWidget` keeps the shipped `SlateWidget->SupportsKeyboardFocus()` guard, now asked of any stop.
So even if a UMG-side flag says yes, a stop whose **Slate** side cannot take keyboard focus is
refused. `SButton`, `SCheckBox`, `SSlider` and `SEditableTextBox` all answer true when focusable —
which is precisely why these four are the admitted set.

---

## 3. (4) THE ORDER STATEMENT, AND ITS LIMITS

**The traversal:** `UWidgetTree::ForEachWidget` — a **depth-first pre-order** walk: the root widget,
then its named-slot bindings, then every `UPanelWidget`'s children by `GetChildAt(0 .. N-1)` (i.e.
**slot order**), recursing into each child before moving to its next sibling. Measured at source:
`Engine/Source/Runtime/UMG/Private/WidgetTree.cpp`, `UWidgetTree::ForEachWidget` →
`UWidgetTree::ForWidgetAndChildren`.

⭐ **This is the SAME traversal `GetMenuButtons()` has always used**, which is why the main menu's
order is unchanged — `Button_0..6` top-to-bottom, exactly the sequence `qa/TASK-1399-verify.md` §5.4
read live.

**The known limits, named:**

1. ⚠️ **Slot order is AUTHORING order, not laid-out visual order.** For a `UVerticalBox` or a vertical
   `UScrollBox` built **in code** they coincide by construction — the three code-authored panels all
   build their trees with `AddChildTo*` in reading order. For a tree authored in the **UMG designer**
   they coincide only if the author added the rows top-to-bottom. This is the limit the row's (4)
   asked for by name.
2. ⚠️ **`UCanvasPanel` has no visual order at all** — children are absolutely positioned, so slot
   order is arbitrary with respect to what 🧑 he sees. The walker cannot detect this and does not
   pretend to.
3. 🚨 **`UWidgetSwitcher`'s inactive pages are still walked.** `ForWidgetAndChildren` visits every
   child of a `UPanelWidget`, and `UWidget::IsVisible()` reads the **child's own** cached Slate
   visibility (`Widget.cpp`), which the switcher does not change — it simply does not arrange the
   inactive slot. ⇒ on a screen built around one (`SiegeControlsHelpWidget.cpp:2667`) the walker can
   collect an off-screen stop. ⛔ **Not fixed here** — no screen in this wave targets that overlay,
   and it lives on `L_Arena` behind fence (a) anyway. Flagged for whoever boards the controls-help row.
4. 🚨 **Nested `UUserWidget` children are NOT descended into.** `ForWidgetAndChildren` casts to
   `UPanelWidget`; a child `UUserWidget` is not one, so its inner tree is a leaf here. ⇒ a screen
   composed of sub-widgets exposes **zero** stops through this walker. **This matters for the deck
   builder**, whose grid stops are `WBP_DeckCardTile` sub-widgets and whose deck bar is ten
   `UDeckSlotEntryWidget`s (`TASK-1398` §2.3 rows 13–14). ⛔ **Declared for `TASK-1419`/the deck-builder
   row and for the manager: that screen needs more than registration.** It is not a defect in this
   row — it is the shape of the engine call — but a row that assumes otherwise would ship a count of 0.

---

## 4. (5) THE RETARGET LOG LINE, QUOTED IN FULL

```cpp
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] menu nav target %s -> '%s' (%s), %d focus stop(s), %d screen(s) registered."),
		Event,
		*GetNameSafe(GetActiveNavTarget()),
		GetRegisteredNavTarget() ? TEXT("registered screen") : TEXT("default: WBP_MainMenu"),
		Stops.Num(),
		NavTargetStack.Num());
```

`LogSiegeMenuInput` at **`Log`** verbosity (never `Verbose`), one line per retarget, emitted from
`RegisterMenuNavTarget` (`Event` = `registered`) and from `UnregisterMenuNavTarget` (`unregistered`).
It names **the new target** and **the focus-stop count**, as (5) required.

⭐ **Why the count is the payload:** a count of **0** on a screen that visibly has controls is the one
line that discriminates *"the action never arrived"* (no line at all) from *"the walker reached the
tree and the tree admitted nothing"* (a line reading 0). `TASK-1415` (3) is specced to state its
expected count **before** 5b so the verifier can falsify it — this is the line it will be falsified
against.

⚠️ **SHIPPING, carried forward from `qa/TASK-1401.md` WARN-4:** `Log` verbosity is compiled out
entirely under Shipping (`USE_LOGGING_IN_SHIPPING` = 0 ⇒ `NO_LOGGING` = 1, no `Target.cs` override —
documented in-tree at `SiegeAssistantGrammar.cpp:226-270`). In a packaged build this line **does not
exist**. Same property as the Accept line. A reader must declare the build configuration before
reading an absence.

---

## 5. THE PROTECTED ARTIFACTS — ALL SURVIVE, VERIFIED NOT ASSERTED

### 5.1 `TASK-1394`'s FOUR instrument lines (⛔ four, not three)

All four present and byte-identical; `grep`ed back out after the edit:

| # | Line | Where |
|---|---|---|
| (a) | `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` | ✅ **the FIRST STATEMENT of `HandleMenuDown()`**, unconditional, nothing before it — `HandleMenuDown` is byte-identical to `HEAD` |
| (b1) | `MoveFocus(%+d) declined: menu covered.` | `MoveFocus`'s first early-out |
| (b2) | `MoveFocus(%+d) declined: no menu buttons.` | `MoveFocus`'s second early-out |
| (b3) | `MoveFocus(%+d): focus moved %d -> %d of %d ('%s').` | before the focus call, reporting the index `MoveFocus` **chose** |

🚨 **DECLARED, MANAGER CALL, NOT SELF-FIXED — TWO OF THOSE STRINGS ARE NOW MILDLY INACCURATE AND I
KEPT THEM ANYWAY.** `declined: menu covered` now means *"the active nav target is not actionable"*,
which on a **registered** target can never be "covered"; `declined: no menu buttons` now means
*"no focus stops"*. ⛔ **I kept both byte-identical on purpose:** `TASK-1395` and `TASK-1402` read
these four strings as discriminators, and re-wording one buys a cosmetic gain at the cost of breaking
a downstream reader's grep. Both are commented in place. ⛔ **`SC-§101`: flagged, not adjudicated.**

⛔ **The `TASK-1446` trap stays un-armed:** (b3) still never reads `FocusButton`/`FocusWidget`'s
return, so an already-focused `SetUserFocus` `false` is still never called a failure. The new
`FocusFirstNavStop` encodes the same rule — it checks "already focused" **before** requesting, and
returns `false` without retrying.

### 5.2 `TASK-1400`'s 0.2 s re-entry poll

✅ **Survives whole.** `FocusReentryPollSeconds = 0.2f` and `FocusReentryPollTimerHandle` unchanged in
the header; the `SetTimer(..., /*bLoop=*/ true)` arm in `OnWorldBeginPlay` and the `ClearTimer` in
`Deinitialize` both unchanged. ✅ **`ApplyInitialFocus` `diff`s EMPTY against `HEAD`**, including its
`Buttons.Num() > 0 && !GetFocusedMenuButton()` idempotence guard.

⭐ **AND THE POLL IS DELIBERATELY LEFT ON THE OLD PATH, WHICH IS LOAD-BEARING TWICE OVER.**
`ApplyInitialFocus` still calls `IsMenuUncovered()` / `GetMenuButtons()` / `GetFocusedMenuButton()` —
*not* the new widened trio. That means (i) the guard the dispatch flagged as newly load-bearing is
untouched, so no loosened guard can walk into the `SetUserFocus`-already-focused retry loop, and
(ii) **while a screen is registered the poll self-silences**, because a visible sub-screen makes
`IsMenuUncovered()` false — so the poll cannot steal focus back off a registered target. That
interaction is the reason not to "modernise" `ApplyInitialFocus` here.

### 5.3 `qa/TASK-1451.md` WARN-2 — the mouse-cause focus surface

⚠️ **ANSWERING THE DISPATCH'S EXPLICIT QUESTION: my (c) work does NOT change `GetFocusedMenuButton`'s
notion of "focused".** That function is byte-identical (§1.2). The **new** `GetFocusedNavStop` uses
the **same** predicate, `UWidget::HasUserFocus(PC)`, which is `EFocusCause`-agnostic — so WARN-2's
reachable focused-but-ringless steady state is **carried over unchanged into the new function,
neither widened nor narrowed**. ⛔ I did not touch it: widening the guard re-opens the focus-steal
surface, and that adjudication is not this row's.

### 5.4 The Accept line

Re-quoted, and **`diff`-proven byte-identical** against `HEAD` (the three-line `UE_LOG` block
compares empty):

```cpp
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."),
		*Focused->GetName(), *GetButtonLabel(Focused));
```

⛔ **Cited by its text, never by its line number** (`CITE-BY-TEXT-RULED-2026-09-24`). Its line number
moved again under this diff, which is exactly why the clause exists; I corrected nobody's landed
citation (`SC-§101`).

🚨 **ACCEPT'S OWN VOCABULARY DID NOT WIDEN, AND THAT IS DELIBERATE.** `HandleMenuAccept` now reads
`Cast<UButton>(GetFocusedNavStop())`, so it still fires a `UButton` and only a `UButton`. What Accept
should *do* on a focused `UCheckBox` / `USlider` / `UEditableTextBox` is not specified by this row —
`TASK-1409` owns Left/Right for the slider and the stepper — and inventing it inside an infrastructure
row would ship an unreviewed semantic. ⇒ ⛔ **until a row specifies it, Accept on a focused checkbox
is a silent no-op.** Named here so nobody discovers it as a surprise. On the default target every
stop is a `UButton`, so the Cast is the identity.

---

## 6. THE REGISTRATION API (fence (b)) — SHAPE AND SEMANTICS

```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|Menu Input")
void RegisterMenuNavTarget(UUserWidget* Screen);

UFUNCTION(BlueprintCallable, Category = "Siegebound|Menu Input")
void UnregisterMenuNavTarget(UUserWidget* Screen);

UFUNCTION(BlueprintPure, Category = "Siegebound|Menu Input")
UUserWidget* GetActiveNavTarget() const;
```

- **`BlueprintCallable` as the row required** — `WBP_MainMenu`, `WBP_DeckBuilder` and
  `WBP_VictoryScreen` are Blueprints and a C++-only API is unreachable from their graphs
  (`TASK-1398` §4.2). Precedent for a forward-declared `UObject*` parameter on a `UFUNCTION` in this
  very module: `SiegeAssistantComponent.h:17` + `:760`.
- **`UCLASS()` → `UCLASS(BlueprintType)`.** Measured reason, not taste: `UK2Node_GetSubsystem`
  filters through `UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Iter, /*bAssumeBlueprintType*/ true)`
  (`Editor/BlueprintGraph/Private/K2Node_GetSubsystem.cpp`), whose final statement is
  `return bAssumeBlueprintType;` — so the *menu action* appears without the specifier, but neither
  `USubsystem` nor `UWorldSubsystem` carries `BlueprintType` (both are plain `UCLASS(Abstract, MinimalAPI)`),
  so a graph asked to **hold** the reference in a variable or pass it on gets `false`. One specifier
  removes the ambiguity for the API whose entire purpose is Blueprint reachability. ⛔ Pure reflection
  metadata: no runtime behaviour, no replication, no new member.
- **Storage is a STACK**, `TArray<TWeakObjectPtr<UUserWidget>> NavTargetStack`, most recent last,
  because registrations genuinely nest: Settings opens Graphics **on top of itself** (ZOrder 10 → 20,
  `SiegeGraphicsMenuWidget.cpp:585`), so closing Graphics must hand navigation back to **Settings**,
  not to the main menu two layers down.
- **Weak on purpose, and re-validated on every read.** `GetRegisteredNavTarget()` walks top-down and
  skips entries that are dead, not `IsInViewport()`, or not `IsVisible()`. ⇒ a screen destroyed
  **without** its paired `Unregister` degrades to *"the ring goes back to the menu"* rather than
  *"the ring is stuck on an invisible tree and nothing errors"*. `Register` compacts dead entries.
- **Re-registering moves to the top**, never duplicates. **Unregistering removes by identity**, never
  pops, because closes can happen out of order. A second `Unregister` (e.g. `BackPressed` **and**
  `NativeDestruct`, which `TASK-1415` (1) requires both of) is a logged no-op, not a warning.
  ⚠️ `bWasRegistered` is measured with `ContainsByPredicate` **before** the purge rather than taken
  from `RemoveAll`'s count — the same call also drops dead entries, so the count would report
  "unregistered" for a screen that was never on the stack whenever anything stale was sitting there.
- **`Register` places the ring** on the new target's stop 0, guarded by the same "only if nothing here
  already holds focus" rule as `ApplyInitialFocus`. 🚨 **DECLARED AS A DESIGN CHOICE FOR THE MANAGER
  (`SC-§101`), because the row specified a retarget and a log line, not a focus placement:** without
  it a registered screen opens **ringless** and the first Down skips stop 0 — navigable and still 🧑
  his complaint — and the screen rows' fences (`TASK-1415` (4): "`SettingsMenuWidget.{cpp,h}` ONLY ·
  do NOT re-author the tree · do NOT add a key handler") leave them nowhere else to put it.
- **`Unregister` re-places the ring ONLY when another registered screen is taking over.** When the
  stack empties it deliberately does nothing: `TASK-1400`'s poll owns that case and is the only path
  carrying the coverage check, and a focus call from here could land on the main menu while the
  closing screen is still drawn (an unregister runs from `BackPressed`, **before** `RemoveFromParent`).

⭐ **`TASK-1425`'s HUD case needs no special case and I confirmed the mechanism rather than asserting
it:** the HUD (ZOrder 0) and the FPS counter (ZOrder 30) are always-visible top-level widgets that
trip the **old** gate. Under the new model they simply never register, and any screen that does
register short-circuits the coverage question entirely. ⛔ No HUD special case was added.

---

## 7. (7) FENCE (a) IS UNTOUCHED — STATED, AND PROVEN

⛔ **The `L_MainMenu` map gate is exactly as it was.** `OnWorldBeginPlay` — the function that contains
it — `diff`s **identical** against `HEAD` across all 115 lines (§1.2). `MenuMapName` is unchanged.
Nothing in this diff can arm a menu key on `L_Arena`. That move is `TASK-1429`'s, with a collision
census this row does not have.

---

## 8. COST — THE DEFERRED `qa/TASK-1451.md` WARN-1 IS NOT MADE WORSE; ON ONE PATH IT IMPROVES

Counting `TObjectIterator<UUserWidget>` sweeps (`GetAllWidgetsOfClass`), which is what WARN-1 measures:

| Path | BEFORE | AFTER | Δ |
|---|---|---|---|
| **The 0.2 s poll** (`ApplyInitialFocus`) | 3 | **3** | ⭐ **0 — the function is byte-identical and still calls the OLD trio.** The ~15 sweeps/sec WARN-1 deferred is *exactly* unchanged |
| **`MoveFocus`, nothing registered** | 3 (`IsMenuUncovered` 1 + `GetMenuButtons` 1 + `GetFocusedMenuButton` 1) | **3** (`IsNavTargetActionable`→`IsMenuUncovered` 1 + `GetMenuFocusStops` 1 + `GetFocusedNavStop` 1) | 0 |
| **`MoveFocus`, a screen registered** | n/a (inert) | ⭐ **0** | `GetRegisteredNavTarget` is a stack read, so `IsMenuUncovered` is never called and `GetActiveNavTarget` never reaches `FindMainMenuWidget` |
| **`Register` / `Unregister`** | n/a | **0** on a registered target (2 tree walks, 0 sweeps) | once per screen open/close |

⛔ **The vocabulary widening multiplies nothing.** It changes what a tree walk *collects*, not how
many viewport sweeps happen; `GetMenuFocusStops` costs one resolution exactly as `GetMenuButtons`
does. The only added work per registration is one extra `WidgetTree` walk inside the log line — once
per screen open, not per poll.

---

## 9. THE AUTOMATION TEST — NOT EDITED, AND ITS PREMISES TRACED

⛔ `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp` was **read and not written**.
Every API it touches is intact:

| Test call | Status |
|---|---|
| `IsArmed()` | untouched |
| `FindMainMenuWidget()` | **byte-identical** |
| `GetMenuButtons(TArray<UButton*>&)` | **byte-identical** — signature and body. ⭐ This is why the widened collector was added **beside** it rather than replacing it |
| `GetFocusedMenuButton()` | **byte-identical** |
| `GetButtonLabel(const UButton*)` / `WrapIndex` / `MenuDownActionPath` / `MenuAcceptActionPath` | untouched |

**Step 3's premise** — Down ×2 from cold ⇒ the focused button reads `"Deck Builder"` — now runs
through `GetMenuFocusStops`. It holds **iff** `WBP_MainMenu`'s tree contains no non-`UButton` focus
stop, which is what two live reads say (§1.2). ⛔ **I did not stop and escalate**, because on every
measurement available the premise is intact and nothing *breaks* it; I am naming the exact condition
under which it would, so `TASK-1407` and `TASK-1413` can aim at it. **The falsifier is one line of the
new log:** a retarget line on the default target reading anything other than **7 focus stops** means
the tree holds something the buttons-only walker was hiding.

---

## 10. Not examined / limitations

- ⛔ **NOTHING WAS COMPILED, RUN OR VERIFIED.** No compile, no PIE, no MCP, no editor read. Every claim
  here is static: about code that exists, never about what happened in a session. The registration
  path has **never executed**.
- 🚨 **`WBP_MainMenu`'s widget tree was never censused completely** (§1.2, §9). I read
  `qa/TASK-1399-verify.md` §5.4 and `qa/TASK-671-verify.md`, both of which enumerate seven `UButton`s
  and nothing else interactive, and the Aura index record for `WBP_MainMenu` (which carries no tree at
  all — parent `UserWidget`, 1 variable `In String`, 1 function `BuildSandboxButton`). ⛔ **A complete
  census needs a live `ui_snapshot`, which this row may not take.**
- 🚨 **NO SCREEN CALLS `RegisterMenuNavTarget` YET.** This row ships the API; `TASK-1415` / `1417` /
  `1419` and the D-wave rows are the callers. ⇒ ⛔ **until one of them lands, the registered branch is
  unreachable at runtime and every behavioural claim about it is static only.** `TASK-1413`'s
  `TASK-1406` criterion ("a registered non-main-menu target reports a focus-stop count > 0") therefore
  cannot be met by this row alone — it needs a caller, or a verifier willing to call
  `RegisterMenuNavTarget` itself. ⛔ **Flagged for the manager and the verifier; I did not rewrite the
  criterion.**
- ⚠️ **`USlider::IsFocusable` read as a member.** Correct and non-deprecated in 5.8 (`Slider.h:94-96`),
  but if a future engine version deprecates it the way `UButton`/`UCheckBox` were in 5.2, this line
  will warn. Commented in place.
- ⚠️ **`UWidgetSwitcher` inactive pages and nested `UUserWidget` children** — §3 limits 3 and 4. Both
  are engine-call shapes, not defects in this diff, and both will bite a specific future row. Named,
  not fixed.
- ⛔ **Accept semantics for the three new stop types are unimplemented** (§5.4). Deliberate.
- ⛔ **Slate's own `FNavigationConfig`** — a real arrow key may move focus with no project code at all
  (`TASK-1398` §8). This row changes nothing about that lane and cannot discriminate it.
- ⛔ **`UCLASS(BlueprintType)` is unverified in an editor.** The reasoning is read from engine source;
  whether the "Get Siege Menu Input Subsystem" node actually appears in a `WBP_*` graph is an editor
  observation nobody has taken.
- ⛔ **Files read outside the row's enumerated READS list, named rather than hidden:**
  `qa/TASK-1399-verify.md`, `qa/TASK-671-verify.md`, `qa/TASK-1451.md`,
  `Saved/.Aura/indexed_files_aura/qq-*_WBP_MainMenu.json`, `Source/.../Tests/SiegeMenuInputTest.cpp`
  (read-only, to protect its premises), `Source/.../SiegeControlsHelpWidget.cpp` (the three opt-out
  sites), and UE 5.8 engine headers/sources. The row's prohibitions are **write** prohibitions; every
  one of these was read only.
- ⚠️ **Tree state at hand-off, for whoever stages:** `Content/Input/IMC_MainMenu.uasset` is modified
  and `IA_MenuBack/Left/Right.uasset` + `handoffs/TASK-1408-art.md` are untracked — **that is
  `TASK-1408` (art), not mine.** ⛔ The commit host must stage by pathspec, never `-a`.
