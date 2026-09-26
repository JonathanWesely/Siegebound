# TASK-1419 — [MENU-NAV-LOGIN] — gameplay-programmer handoff

- **marker:** `TASK-1419-MENU-NAV-LOGIN`
- **date:** 2026-09-24
- **status set by me:** `ready-for-qa`
- **files written:** `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.cpp` (**+243 / −0**) · `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.h` (**+172 / −0**)
- **ZERO DELETIONS** (the `TASK-1415` / `TASK-1417` house shape). Every pre-existing line is byte-identical.
- ⛔ **NO compile · NO PIE · NO MCP · NO editor lifecycle action · NO git · NO `.uasset` · NO `CONVENTIONS.md`.**

---

## 0. THE HEADLINE — THE (3) RULE, AND IT IS THE ENGINE'S OWN

> **Up / Down LEAVE the field. Left / Right STAY (caret). And I wrote no key handler to do it,
> because the engine already does it — measured in the 5.8 source, not assumed.**

**Lane A — the agent lane (`inject_input_action IA_MenuUp` / `IA_MenuDown`), which is this row's
runtime criterion.** The injection enters at
`UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` and reaches
`USiegeMenuInputSubsystem::HandleMenuDown()` → `MoveFocus(+1)` → `FocusWidget()` →
`FSlateApplication::SetUserFocus`. **Slate is never asked whether the text box wants the key.**
A `UEditableTextBox` cannot absorb a key it is never shown ⇒ egress on this lane is *structural*,
not something code of mine grants.

**Lane B — 🧑 his hands (a real key).** On `L_MainMenu` `SetIgnoreInput(true)` keeps a real key out
of Enhanced Input entirely (the premise `TASK-1388` deliberately left standing), so lane B is
Slate's alone — and Slate's own editable text is written to hand Up/Down back:

- `SEditableText::OnKeyDown` (`SEditableText.cpp:274-294`) → `FSlateEditableTextLayout::HandleKeyDown`
  (`SlateEditableTextLayout.cpp:994`).
- Up/Down (`:1037-1057`) → `BoolToReply(MoveCursor(FMoveCursor::Cardinal(Character, FIntPoint(0, ∓1), …)))`.
- `FSlateEditableTextLayout::MoveCursor` (`:2198`), at **`:2261-2266`**, verbatim:

  ```cpp
  else
  {
      // Vertical movement not supported on single-line editable text controls - return false so we fallback to generic widget navigation
      return false;
  }
  ```
  (`SEditableText::IsMultiLineTextEdit()` returns `false`, `SEditableText.cpp:597-600`.)
- `BoolToReply(false)` is `FReply::Unhandled()` (`SlateEditableTextLayout.cpp:46-49`) ⇒
  `FSlateApplication` falls through to its own directional navigation and **the ring leaves the field.**
- Left/Right take the `IsHorizontalMovement()` branch (`:2253-2256`) → `TranslatedLocation` → Handled
  ⇒ **the caret keeps them**, which is the half a typist would riot about losing.

⇒ **The recommended rule is implemented, on both lanes, with zero key handling — which is exactly why
fence (5) can forbid a key handler and still get the behaviour.** A `NativeOnKeyDown` here would have
been a *regression* (it is the wrong-layer defect this epic removes, and it is the one thing that
could absorb `Escape`).

---

## 1. 🚨 THE DEFECT I FOUND, DID **NOT** FIX, AND AM **ESCALATING** (`SC-§101` / `SC-§50`)

**This is the real answer to `TASK-1420`'s one question, and it is NOT the mechanism the row
predicted.** The row expected "a text box eats the arrow keys". It does not. What actually breaks is
**the read-back**, and the broken read-back lives in a **FENCED** file.

**The chain, every link measured:**

1. `USiegeMenuInputSubsystem::FocusWidget` (`SiegeMenuInputSubsystem.cpp:1372-1407`) calls
   `FSlateApplication::SetUserFocus(UserIndex, Widget->GetCachedWidget(), Navigation)`. For a
   `UEditableTextBox` the cached widget is the **`SEditableTextBox`**.
2. `FSlateApplication::SetUserFocus` runs `FReply Reply = NewFocusedWidget->OnFocusReceived(...)` and
   then `ProcessReply(...)` (`SlateApplication.cpp:3167-3171`).
3. **`SEditableTextBox::OnFocusReceived` (`SEditableTextBox.cpp:309-320`) returns
   `FReply::Handled().SetUserFocus(EditableText.ToSharedRef(), Cause)` — it FORWARDS focus to the
   inner `SEditableText`.** ⇒ `FSlateUser::GetFocusedWidget()` is the **inner** text, not the box.
4. `UWidget::HasUserFocus` (`Widget.cpp:624-648`) is **exact-widget**:
   `FSlateUser::HasFocus` is `GetFocusedWidget() == Widget` (`SlateUser.cpp:182-185`).
   ⇒ **`UEditableTextBox::HasUserFocus(PC)` is `false` while the field is visibly wearing the ring.**
5. `USiegeMenuInputSubsystem::GetFocusedNavStop()` (`:507-531`) therefore returns **null**, so
   `MoveFocus` (`:753`) takes `Current = 0` — the "cold" branch — **every single time the ring is in a
   text box.**

**The user-visible consequence, stated so `TASK-1421` can falsify it.** In `CreateForm`
(stops `0 NameInputBox`, `1 PasswordInputBox`, `2 ConfirmPasswordInputBox`, `3 SubmitButton`, `4 BackButton`):

| press | Current read | lands on | note |
|---|---|---|---|
| open | — | `NameInputBox` | ring placed by `RegisterMenuNavTarget` |
| Down | `0` (cold — field unreadable) | `PasswordInputBox` | moves ✔ |
| Down | `0` (cold again) | `PasswordInputBox` | **STUCK at stop 1** |
| Up | `0` (cold) | `BackButton` (stop 4) | escapes to a button ✔ — and from a button everything works |

⇒ **The ring is NOT trapped** (Up reaches the last stop, and once on a `UButton` the ring behaves
perfectly) — but **you cannot walk a form with Down alone**, and `ConfirmPasswordInputBox` /
`SubmitButton` are unreachable by Down. That is the defect, and it is worse than the row feared in one
respect and better in another; both halves are stated rather than rounded.

**THE REMEDY, NAMED AND NOT WRITTEN** (it is `TASK-1406`'s file, read-only to me):

```cpp
// USiegeMenuInputSubsystem::GetFocusedNavStop(), SiegeMenuInputSubsystem.cpp:~524
if (Stop->HasUserFocus(PC) || Stop->HasUserFocusedDescendants(PC))
```
and the twin early-out in `FocusFirstNavStop()` (`:578-584`), which otherwise yanks the ring back to
stop 0 whenever anything re-registers while a field holds it.
`UWidget::HasUserFocusedDescendants` (`Widget.cpp:673-696`) is the strict-descendant twin
(`FSlateUser::HasFocusedDescendants`, `SlateUser.cpp:192-195`: path valid **and** last widget is not
this one **and** path contains this one), so `A || B` is complete and cannot double-count.

⚠️ **It is not only this screen's problem.** Any screen with a `UEditableTextBox` stop inherits it —
`TASK-1423` (deck builder) and `TASK-1425` (session menu) should be told before they predict counts.
I have **not** amended any shared document; this is the manager's ruling to make.

**⛔ AND THIS FILE IS WRITTEN SO IT IS CORRECT EITHER WAY.** My one predicate that asks "does a live
stop still hold the ring?" (`RefreshMenuNavRing`) asks it with
`HasUserFocus(PC) || HasUserFocusedDescendants(PC)` **on this screen's own children** — entirely
inside my fence. Without the `||` my own helper would have concluded "the ring fell off" every time a
player stood in a field and **yanked the ring back to stop 0 mid-typing**: a regression manufactured
by the fix. If QA rules the subsystem patch in, nothing in this file changes.

---

## 2. REGISTER / UNREGISTER — BOTH SHOWN

**REGISTER — ONE site: `NativeConstruct()`, AFTER `RefreshModeFromSubsystem()`** (`AccountMenuWidget.cpp`,
last statement of `NativeConstruct`). The "after" is load-bearing and is **worse here than on Settings
or Graphics**: `RegisterMenuNavTarget` logs the stop count *and* places the ring, both read from live
visible/enabled state, and `RefreshModeFromSubsystem()` is what settles it — it runs `ApplyMode()`,
which **collapses ten of the thirteen possible children**, and on the unhappy path it then runs
`ShowUnavailable()` → `SetFormsEnabled(false)`. **Registering first would log 13 for a screen that has
3**, and could park the ring on a control collapsed or disabled one line later. (Same law `TASK-1417`
measured: registering early logged 24 for a screen that had 2.)

**UNREGISTER — TWO sites, BESIDE the existing teardown, never instead of it:**
- `BackPressed()` — **after** this panel's own teardown (the `UE_LOG` line; Back on this panel is
  deliberately `RemoveFromParent()` *and nothing else*, ACC-§5) and **before** `RemoveFromParent()`.
  Order is the subsystem's own stated contract (`UnregisterMenuNavTarget`'s closing comment:
  *"an unregister runs from `BackPressed`, BEFORE `RemoveFromParent`"*).
- `NativeDestruct()` — **first statement** (LIFO against `NativeConstruct`), **above** and not
  replacing the delegate unbind block. Catch-all for level travel / viewport teardown / a caller that
  removes this widget without going through `BackPressed`.
- Both firing is the normal case and is safe: `UnregisterMenuNavTarget` removes by **identity** and
  logs-not-warns the second call (`:617-655`).

**Viewport ordering** is cited from `TASK-1415`'s 5.8 measurement rather than re-derived
(`GameViewportSubsystem.cpp:158/:177/:183` set the viewport flags **before** `TakeWidget()`, and
`TakeWidget` is what runs `RebuildWidget → OnWidgetRebuilt → NativeConstruct`) ⇒ `IsInViewport()` and
`IsVisible()` are already true on the registration line, which is what `GetRegisteredNavTarget()`
re-validates on every read. **The falsifier is one log line:** a `registered` retarget naming anything
but this widget, or reporting the main menu's 7 stops.

---

## 3. THE THING THIS SCREEN NEEDS THAT SETTINGS AND GRAPHICS DID NOT — `RefreshMenuNavRing()`

Settings and Graphics each have **one** stop set for their whole lifetime. **This panel is a MODE
MACHINE** (`Chooser → CreateForm / LoginForm → LoggedIn → CloudLinkForm`) and **every `ApplyMode()`
collapses the row the ring is standing on.** The very first thing a keyboard user does reproduces it:

> ring opens on `CreateAccountButton` (Chooser stop 0) → Accept → `CreateAccountChosen()` →
> `ApplyMode(CreateForm)` → **`CreateAccountButton` is COLLAPSED**, i.e. the outline vanishes — 🧑 his
> original complaint, reproduced by the feature meant to fix it.

`RefreshMenuNavRing()` is the whole remedy and it is **a no-op unless both guards pass**:
1. `MenuInput->GetActiveNavTarget() == this` — a **live** read, never a cached bool. False before
   registration (so `NativeConstruct`'s own `ApplyMode` cannot register early and defeat §2's ordering
   law), false after `BackPressed` unregisters (so a late HTTP completion cannot re-register a closing
   panel), and false whenever another screen is stacked on top (so it can never steal the ring).
2. No **live** stop of this screen holds the ring, asked with `HasUserFocus || HasUserFocusedDescendants`
   (§1). The list comes from the subsystem's own walk, so a stop the mode change just collapsed or
   disabled is *already gone from it* — which is precisely the discrimination needed, and the reason
   this does **not** simply ask "is anything in my tree focused?" (a collapsed widget keeps Slate focus,
   so that question answers "yes" in the one case that matters).

On both, it calls `RegisterMenuNavTarget(this)` again: re-tops the stack (no duplicate), emits **one**
retarget line carrying the **new** stop count, and places the ring on the new mode's stop 0.

**FIVE CALL SITES, ALL PURE APPENDS AT A FUNCTION TAIL:**

| site | what moved the stop set |
|---|---|
| `ApplyMode()` last statement | every `SetShown`, plus the `RefreshCloudBlock()` immediately above it |
| `ShowUnavailable()` last statement | `SetFormsEnabled(false)` takes Chooser from 3 stops to 1 |
| `HandleCloudStateChanged()` last statement | an async broadcast swaps `LinkCloudButton` ⇄ `SyncNowButton` |
| `StartCloudRequest()` last statement | disables `SubmitButton` — very likely the stop the ring is on |
| `FinishCloudRequest()` last statement | re-enables `SubmitButton` + redraws the cloud rows |

Cost: one tree walk per state change, **no log line unless the ring actually fell off.**

**Re-entrancy checked:** the only thing a focus move can fire on this tree is
`UEditableTextBox::OnTextCommitted` (focus-lost commit) — **this widget binds no `OnTextChanged` or
`OnTextCommitted` handler at all** (measured: `NativeConstruct` binds seven `OnClicked` thunks and two
subsystem delegates, nothing else), so no path re-enters `ApplyMode`.

---

## 4. ⛔ THE EXPECTED STOP LIST — STATED **BEFORE** 5b SO `TASK-1421` CAN FALSIFY IT
(`EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24` cl. 1–4, `TASKBOARD.md:6059`)

**cl. 2's third column is UNIFORM, and that is the measurement: every stop's construction route is
`C++`; there is ⛔ NO `.uasset` AT ALL.** `/Game/UI/WBP_AccountMenu` is **RESERVED and UNAUTHORED**
(ACC-§5(c); re-measured today — `find Content -iname "*AccountMenu*"` returns **nothing**, and
`Content/UI/` holds ten `WBP_*.uasset` files, none of them this one). ⇒ **zero `asset-authored`, zero
`EventGraph`** — stated as *"there is no asset"*, not as a bare zero.

**⛔ THE STOP SET IS NOT ONE LIST — IT IS PER MODE.** In `UWidgetTree::ForEachWidget` order
(depth-first pre-order from `BackdropBorder → RootPanel`, `AddChildToVerticalBox` slot order):

| mode | count | stops, in order (all `UButton` unless marked, **all `C++`**) |
|---|---|---|
| **Chooser** | **3** | 0 `CreateAccountButton` · 1 `LoginExistingButton` · 2 `BackButton` |
| **CreateForm** | **5** | 0 `NameInputBox` *(`UEditableTextBox`)* · 1 `PasswordInputBox` *(`UEditableTextBox`)* · 2 `ConfirmPasswordInputBox` *(`UEditableTextBox`)* · 3 `SubmitButton` · 4 `BackButton` |
| **LoginForm** | **4** | 0 `NameInputBox` *(`UEditableTextBox`)* · 1 `PasswordInputBox` *(`UEditableTextBox`)* · 2 `SubmitButton` · 3 `BackButton` |
| **CloudLinkForm** | **5** | 0 `EmailInputBox` *(`UEditableTextBox`)* · 1 `PasswordInputBox` *(`UEditableTextBox`)* · 2 `ConfirmPasswordInputBox` *(`UEditableTextBox`)* · 3 `SubmitButton` · 4 `BackButton` |
| **LoggedIn** | **2 or 3** | 0 `LogoutButton` · [1 `LinkCloudButton` **XOR** `SyncNowButton`, **only while the cloud is CONFIGURED**] · `BackButton` |

**⇒ THE HEADLINE PREDICTION FOR `TASK-1421`: the screen opens on `Chooser` with `3` stops** (guest is
the default; `RefreshModeFromSubsystem()` sends a logged-in profile to `LoggedIn` instead — say which
was seen). **The ring is ALREADY ON at open ⇒ `Down`×1 = stop 1 (`LoginExistingButton`), NOT stop 0.**
A row expecting stop 0 after one Down mis-reads a working screen as broken.

**Non-stops, named so a short count is diagnosable:** `BackdropBorder` (`UBorder`), `RootPanel`
(`UVerticalBox`), and ten `UTextBlock`s (`TitleText`, `StatusText`, `CloudStatusText` and the seven
button content labels). None is one of the four admitted classes.

**cl. 3 declared:** the count bounds **the defect** (a stop the player cannot reach), **not the cause**.
**cl. 4 declared:** an `UNOBSERVABLE` at `TASK-1421` produces **no count** ⇒ this screen is then
**unmeasured on this axis too**, and the numbers above must **not** stand in for a reading (`VER-§5` cl. 2).

### ⛔ THE UNHAPPY PATH AND ITS DISCRIMINATOR
**Unhappy-path count = 1** (`BackButton` alone). `ShowUnavailable()` → `SetFormsEnabled(false)` kills
the other two Chooser stops; `BackButton` is deliberately absent from that list **in both directions**
(a panel you cannot leave is worse than one that cannot log anyone in).
**DISCRIMINATOR:** a **1 WITH**
`[AccountMenu] USiegeAccountSubsystem could not be resolved - the account forms are disabled...`
(`Warning`, `LogSiegeAccount`) = the ACC-§1 fail-safe working. A **1 WITHOUT** it = the defect.

### 🚨 A LOAD-BEARING INSTRUCTION FOR `TASK-1421` (1)(d)
**In `Chooser` — the mode the verifier will land in — ALL THREE STOPS ARE `UButton`s.** This screen
**cannot** satisfy (d) ("does it reach a NON-`UButton` stop") from the Chooser, and that is **correct
behaviour, not the fence-(c) failure.** To reach a non-button stop the verifier must first
`IA_MenuAccept` on `CreateAccountButton` or `LoginExistingButton` to enter `CreateForm` / `LoginForm`,
where stops 0–2 are `UEditableTextBox`. **A verifier who walks the three Chooser buttons and reports
"only buttons" would be reporting a healthy screen as broken.**

---

## 5. THE `Escape`-UNTOUCHED STATEMENT (spec (4), `AS-§6 A-2`)

**I absorb nothing.** Measured on my own diff: the words `NativeOnKeyDown`, `NativeOnPreviewKeyDown`,
`OnKeyDown`, `OnKeyChar`, `FReply`, `EKeys::`, `SetKeyboardFocus`, `SetNavigationRule`, `IsFocusable`,
`ISiegeMenuNavCloseTarget` and `OnMenuNavBackRequested` appear **zero times outside comments** in
`AccountMenuWidget.{h,cpp}` after this row. The class comment's standing sentence is still true
character-for-character: *"No key handling is overridden anywhere in this class - in particular
`Escape` stays permanently unabsorbed, project-wide (AS-§6 A-2)."*

⚠️ **AND THE HONEST RIDER, SAID OUT LOUD RATHER THAN REPORTED AS A CLEAN ZERO.** The **engine's**
`SEditableTextBox::OnKeyDown` (`SEditableTextBox.cpp:323-333`) *does* handle `Escape` while its inner
text holds keyboard focus:

```cpp
if (Key == EKeys::Escape && EditableText->HasKeyboardFocus())
{
    return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Cleared);
}
```
(reached only when `FSlateEditableTextLayout::HandleEscape()` (`:1448-1477`) declines, i.e. no search
text, no selection, no revert-on-escape). **This is engine code, on a control this screen has shipped
since `TASK-603`; this row neither adds nor removes it, and I did not "fix" it** — the only in-file
remedies would be a key handler (forbidden by (5), and the very thing AS-§6 A-2 exists to prevent) or
a behaviour change to a credential field. **Declared for the manager, not self-adjudicated
(`SC-§101`).** Incidentally it is a *third* egress route from a field.

---

## 6. FENCES HELD

- **WROTE:** `AccountMenuWidget.cpp` (+243/−0) · `AccountMenuWidget.h` (+172/−0) · this handoff · this
  row's `status:` only.
- **READ-ONLY, 0 markers written:** `SiegeMenuInputSubsystem.{cpp,h}` · `SettingsMenuWidget.{cpp,h}` ·
  `SiegeGraphicsMenuWidget.*` (**not opened at all** — I read `TASK-1415`'s diff and the board row
  instead, because `TASK-1418` is reviewing that file this minute) · `SiegePlayerController.cpp`
  (**not opened**) · `FogVolume.cpp` (**not opened**).
- **NOT TOUCHED:** `DeckBuilderWidget.*` (`TASK-1423`) · `SessionMenuWidget.*` (`TASK-1425`) ·
  any `.uasset` · `Build.cs` · `CONVENTIONS.md` · git.
- **⛔ NO CREDENTIAL PATH MOVED (ACC-§2 / ACC-§11 / ACC-§13).** Diff-level check: the added lines
  contain **zero** occurrences of password / email / display-name / hash / salt / token /
  `SetCloudLink` / `CreateAccount` / `Login` / `Logout` / `SignUp` / `SignIn` / `PushAll` / `PullAll` /
  `SyncNow` / `GetText` / `SetText` / `ClearPasswordBoxes`. The only thing my four functions know
  about the four text boxes is **whether Slate focus is inside them** — never what is in them. No
  mode machine value, no `bCloudRequestInFlight`, no `bCloudSessionRefreshAttempted`, no
  `ApplyMode`/`ShowStatus`/`ShowCloudStatus` body is edited.
- **No new member of any kind** — `RefreshMenuNavRing`'s guards are live reads, not cached state.
- Brace/paren balance re-checked on both files after editing: **0 / 0**.

---

## 7. `## Not examined / limitations`

- **L1 — NOT COMPILED, NOT RUN.** Every count above is a **prediction**; §1's defect chain is read
  from engine + project source, not from a running frame.
- **L2 — THE PIXEL LIMB IS NOT MINE** (`VER-§11` cl. 9). A ringless capture at `TASK-1421` is
  `UNOBSERVABLE`, never `VERIFY-FAILED`.
- **L3 — `IA_MenuBack` IS EXPECTED INERT HERE AND I BUILT NO BACK KEY HANDLING.** This screen does
  **not** implement `ISiegeMenuNavCloseTarget` — that is `TASK-1454`'s row, not mine. `TASK-1421`(g3)'s
  prediction (Back does **not** close the screen) stands for this subject.
- **L4 — WHICH MODE THE VERIFIER LANDS IN IS NOT MINE TO CONTROL.** It depends on whether a profile is
  logged in on this machine's save. Both branches are predicted above; **say which one was seen.**
- **L5 — THE `LoggedIn` COUNT (2 vs 3) DEPENDS ON CLOUD CONFIGURATION** I cannot read from here
  (`USiegeCloudClient::IsCloudConfigured()` reads ini). Both are predicted; neither is a defect.
- **L6 — `UWidget::IsVisible()` READS THE WIDGET'S OWN SLATE VISIBILITY, NOT ITS ANCESTORS'**
  (`TASK-1417`'s finding). Benign here — every stop is a direct child of `RootPanel`, which is never
  collapsed — but stated so nobody assumes it was checked by accident.
- **L7 — `GetOwningPlayer()` vs the subsystem's `GetLocalController()`** are two different resolutions
  of "the local player". On `L_MainMenu` there is exactly one, so they coincide; on a split-screen
  world they would not. Not measured.
- **L8 — LANE B (a real key) IS ARGUED FROM ENGINE SOURCE, NOT OBSERVED.** No agent lane delivers a
  real key into Slate (`TASK-1459` D1: `simulate_key_press` reaches Enhanced Input, not Slate), so the
  `:2261-2266` fallthrough is **strong, not airtight** until 🧑 he presses Down in a field himself.
- **L9 — `RefreshMenuNavRing`'s five sites are argued, not exercised.** The cloud three
  (`HandleCloudStateChanged` / `StartCloudRequest` / `FinishCloudRequest`) need a configured cloud to
  fire at all and will almost certainly be dead code in `TASK-1421`'s sitting.
- **L10 — SHIPPING.** Every diagnostic line named here is `Log` verbosity and is compiled out under
  Shipping (`NO_LOGGING`). A reader of a packaged build sees none of it.
- **🚨 L11 — `Log LogSiegeMenuInput Log` AND `Log LogSiegeAccount Log` MUST BE ON, OR THE STOP-COUNT
  INSTRUMENT PRINTS NOTHING — AND AN EMPTY LOG READS AS A FALSE PASS.**

---

## 8. cl. 3(b) — THE FIRST SENTENCE FOR 🧑 HIM, AND THE PARENTHESIS VERBATIM

> "🧑 Open Login and use ⛔ only the keyboard — ⛔ can you reach each field, ⛔ type in it, and ⛔ get
> back out to the buttons ⛔ without touching the mouse?"
>
> *"(⛔ A highlight that appears ⛔ only while the mouse is ⛔ HOVERING a control is a ⛔ NO. ⛔ An
> outline already sitting where you ⛔ left it is a ⛔ NO — ⛔ that is Slate ⛔ restoring old focus,
> ⛔ not our code. ⛔ If you pressed ⛔ any key or ⛔ moved the mouse before looking, the answer ⛔ does
> not count — ⛔ just say so and we'll redo it. ⛔ Roughly what time, so the log line can be found.)"*

---

## 9. WHAT QA (`TASK-1420`) SHOULD SCRUTINISE HARDEST

1. **§1's escalation.** Is the `HasUserFocus`-is-exact-widget chain right, and is
   `HasUserFocusedDescendants` the correct twin? If QA rules the subsystem patch in, it is a
   **new row against `TASK-1406`'s file** — not a bounce of this one — and `TASK-1421` must be told
   which behaviour it is falsifying.
2. **Guard 1 of `RefreshMenuNavRing`.** `GetActiveNavTarget() != this` is the single thing keeping five
   sprinkled calls from stealing the ring or registering a closing panel. Break it and this row is
   worse than no row.
3. **Five call sites is more than any prior screen needed.** Each is justified in §3 — check that none
   of them can fire while the tree is mid-mutation, and that the `ApplyMode` one is genuinely the
   **last** statement (it must run after `RefreshCloudBlock()`, and `ShowUnavailable()` must run after
   it on the unhappy path, which it does — two calls, the second corrects the first).
4. **Zero deletions + zero key handling + zero credential touches** — all three are diff-level checkable
   and all three are asserted in §6 with the exact greps.
