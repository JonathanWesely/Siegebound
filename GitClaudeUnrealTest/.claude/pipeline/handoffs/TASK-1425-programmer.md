# TASK-1425 — [MENU-NAV-SESSION] — gameplay-programmer handoff

- **Author:** gameplay-programmer · 2026-09-24 · marker `TASK-1425-MENU-NAV-SESSION`
- **Status set by me:** `ready-for-qa` → routes to ⭐ `TASK-1426`, then 5a, then 5b = ⭐ `TASK-1427`, commit ⭐ `TASK-1428`.
- **Files written:** `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.h` (**+112 / −0**) · `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.cpp` (**+169 / −0**).
- **Zero deletions**, the house shape ⭐ `TASK-1415` / ⭐ `TASK-1417` both shipped. Every pre-existing line is byte-identical (`git diff -U0 | grep -c '^-[^-]'` ⇒ **0**).
- **No compile, no PIE, no MCP, no editor lifecycle action, no git, no `.uasset`, no `CONVENTIONS.md`.** `build-master` was relaunching the editor; I touched nothing outside my two source files, my row's `status:` line and this note.

---

## 1. 🚨 THE HEADLINE — **⭐ `TASK-1400`'s RE-ARM ALREADY COVERS THE FRESH INSTANCE, SO I ADDED NOTHING FOR THE RETURN LEG**

Spec (2) said it in advance: *"IF ⭐ `TASK-1400`'s re-arm already covers this, SAY SO AND ADD NOTHING — two mechanisms racing for the same ring is worse than one."* **It covers it.** This file's entire contribution to the return leg is that it **stops claiming the ring**.

**And I did not assume it — here is the chain, each link read at source this row:**

| # | Link | Where I read it |
|---|---|---|
| 1 | `BackPressed()` loads the class **by the literal string** `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` | `SessionMenuWidget.cpp:151` (unchanged by me) |
| 2 | The poll's three resolvers match on **that same class-path string**, and filter by `IsInViewport()` | `SiegeMenuInputSubsystem.cpp:302-323` (`FindMainMenuWidget`), `:325-354` (`IsMenuUncovered`), `MainMenuWidgetClassPath` is defined `= TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C")` at `:40` |
| 3 | ⇒ the instance `BackPressed` **just created** is the one the poll finds; the instance this screen was opened from was `RemoveFromParent`'d by the BP open transition and **drops out of the same set**. There is **no pointer to go stale** | `SiegeMenuInputSubsystem.cpp:205-234` states the property; I verified it against the three call sites rather than taking the comment's word |
| 4 | The re-arm is a **looping 0.2 s** world timer (`FocusReentryPollSeconds`), armed **after** the `L_MainMenu` map gate, not a one-shot | `SiegeMenuInputSubsystem.cpp:234-236`; `.h:312-330` |
| 5 | It focuses **`Buttons[0]`** — literally the TOP option — and logs the placement **with the menu instance's name** | `ApplyInitialFocus`, `SiegeMenuInputSubsystem.cpp:1322-1362` |

⇒ **the runtime criterion (`focused: true` on the NEW main menu's TOP option after Back) is satisfied by a mechanism that is already committed, and my row's job was to not break it and not duplicate it.**

### 1.1 The four ways it could have raced, and why none of them fires — **this is the (1) answer ⭐ `TASK-1426` is told to demand**

1. **Could the poll steal the ring while my panel is open?** ⛔ No. The open transition **removes** `WBP_MainMenu` from the viewport (`SessionMenuWidget.cpp:138-141` records the BP shape: `RemoveFromParent(self) → CreateWidget(WBP_SessionMenu_C) → AddToViewport`). `IsMenuUncovered()` walks the top-level set; it meets my panel, which is visible and is **not** the menu class, and **returns false on the spot** (`:346-351`). `ApplyInitialFocus`'s **first statement** is that guard. ⇒ the poll is a no-op for the whole time this screen is up — *regardless of my registration*.
2. **Could the poll fire in the window where BOTH widgets are in the viewport?** ⛔ No. `MainMenu->AddToViewport();` and `RemoveFromParent();` are **consecutive statements in one call**; a timer needs a tick, and no tick runs between them.
3. **Could my `UnregisterMenuNavTarget` itself place focus and collide?** ⛔ No, and this is the subsystem's **design**, not a gap: it calls `FocusFirstNavStop()` **only** `if (GetRegisteredNavTarget() != nullptr)` (`:653-658`). On this screen the stack drops to **empty** (nothing else is registered — the session menu is opened straight from `WBP_MainMenu`, which is not a registrant), so **no focus call is made from the unregister**. The subsystem's own comment gives the reason and it is exactly this screen's: *"an unregister runs from `BackPressed`, BEFORE `RemoveFromParent`"*.
4. **Could a stale pointer to the destroyed main menu survive anywhere?** ⛔ No — **I store no widget pointer at all.** The only state this row adds to the nav system is `this` inside the subsystem's `TWeakObjectPtr` stack, and it is removed before the panel goes. ⭐ `TASK-1426` (2) is answered by absence: `grep` this diff for a `TObjectPtr`, a raw `UUserWidget*` member or a cached menu — there is none.

⇒ **ONE mechanism owns the return leg, and it is the committed one.**

---

## 2. WHAT I ACTUALLY CHANGED — four insertions, and the ordering argument for each

### 2.1 `NativeConstruct()` — `RegisterAsMenuNavTarget()` as the **last** statement

Obeys ⭐ `TASK-1417`'s measured law (**register AFTER the seed/bind call, never before** — registering against the pre-seed tree logged **24** stops for a screen that had **2**).

⚠️ **And I am declaring that the law buys LESS here than it did there, rather than implying I dodged a bullet:** this screen's tree is **asset-authored**, so all twelve widgets already exist with their authored enabled/visible state before `NativeConstruct` runs, and the bind block above **mutates no stop** — `ShowLocalError` only pushes text into `ErrorTextBlock` (a `UTextBlock`, not an admitted class) and fires a `BlueprintImplementableEvent` that a **zero-graph** BP does not implement. **The count is four either side of the bind.** I kept the ordering anyway, because it costs nothing and because the day somebody adds a `SetIsEnabled` to that block is the day it starts mattering silently.

**Viewport-ordering dependency, inherited from ⭐ `TASK-1415`'s reading of the 5.8 source rather than re-asserted:** `AddToScreen` sets `bIsManagedByGameViewportSubsystem` and the slot **before** `TakeWidget()`, and `TakeWidget` is what runs `RebuildWidget → OnWidgetRebuilt → NativeConstruct` ⇒ `IsInViewport()` and `IsVisible()` are already true at the registration line, which is what `GetRegisteredNavTarget()` re-validates on **every** read. **The falsifier is one line of log:** a `registered` retarget naming anything but this widget, or reporting **7** stops (the main menu's count), means that ordering changed.

### 2.2 `BackPressed()` — `UnregisterAsMenuNavTarget()` between `AddToViewport()` and `RemoveFromParent()`

🚨 **THE ORDERING IS LOAD-BEARING HERE IN A WAY IT WAS NOT ON SETTINGS OR GRAPHICS, BECAUSE THE PARENT IS BEING DESTROYED TOO. What the stack holds at each step — the dispatch asked for this explicitly:**

| Step | Nav stack | Why |
|---|---|---|
| before `BackPressed` | `[SessionMenu]` | registered at `NativeConstruct` |
| the `bNetActive \|\| bPendingConnection` branch → `LeaveMatch(); return;` | `[SessionMenu]` — **deliberately unchanged**, see §3 | the panel is **not** removed on that leg |
| after the `!MainMenu` early return (`ShowLocalError`) | `[SessionMenu]` — **UNCHANGED, and this is WHY the call sits below that guard** | a failed `LoadClass`/`CreateWidget` leaves **this** panel up and clickable, so it must **keep** the ring. Unregistering above the guard would blind a panel that is still the only UI on screen. |
| after `MainMenu->AddToViewport()` | `[SessionMenu]` | both widgets are in the viewport for three statements; **no tick observes it**, so the order of my call against `AddToViewport` is *not* itself load-bearing — only its position relative to the guard is |
| **after my line** | `[]` — **EMPTY** | ⇒ `UnregisterMenuNavTarget`'s `if (GetRegisteredNavTarget() != nullptr) FocusFirstNavStop();` does **not** fire. Design, not gap (§1.1 item 3). |
| after `RemoveFromParent()` | `[]` | `NativeDestruct` calls Unregister a **second** time; the subsystem answers with one `Log` line (*"not registered … no change"*) and no state change — the pairing the API documents. |
| ≤ 0.2 s later | `[]` | the poll finds the **fresh** menu uncovered with nothing focused, focuses `Buttons[0]`, logs the instance name. |

### 2.3 `NativeDestruct()` — `UnregisterAsMenuNavTarget()` as the **first** statement

LIFO against `NativeConstruct`: navigation goes back **before** the delegates come down, because registration was the last thing taken. ⭐ `TASK-1417`'s law, verbatim. This is the **catch-all** half: it covers the `LeaveMatch` level travel, viewport teardown, and any caller that removes this widget without going through `BackPressed`.

### 2.4 Three new members + one include

`RegisterAsMenuNavTarget()` / `UnregisterAsMenuNavTarget()` / `ResolveMenuInputSubsystem() const`, plus `#include "SiegeMenuInputSubsystem.h"` and a forward declaration. **`ResolveMenuInputSubsystem` is a byte-for-byte clone of ⭐ `TASK-1415`'s** (`SettingsMenuWidget.cpp:781-789`) including the const-`UWorld*` / `GetSubsystem<>()` shape that has already compiled in this module — the one deliberate difference from `ResolveSessionSubsystem` is that this subsystem lives on the **world**, not the game instance.

---

## 3. ⛔ THE ONE PLACE I DELIBERATELY DID **NOT** UNREGISTER — and it is measured, not forgotten

**The `LeaveMatch()` leg of `BackPressed` (`.cpp:119-125`) gets no unregister call.**

`USiegeSessionSubsystem::LeaveMatch` (`SiegeSessionSubsystem.cpp:130-153`) is **`UGameplayStatics::OpenLevel(..., bAbsolute = true)`** — a **deferred** travel processed at the end of the tick — with **no `RemoveFromParent` on this panel anywhere on that path**. ⇒ the panel **stays on screen, and rightly keeps the ring**, for the rest of that frame and beyond. Unregistering there would **strand the ring on a screen 🧑 he is still looking at** — the precise failure mode the subsystem's `GetRegisteredNavTarget()` re-validation exists to make survivable, which is not a reason to cause it. The world — and this `UWorldSubsystem` with it — then tears down, and `NativeDestruct` is the honest hook.

⭐ **This is the cheap-looking third call site that would have been WRONG, and I want QA to check my reasoning rather than my symmetry.**

---

## 4. ⛔ THE EXPECTED STOP LIST — **STATED BEFORE 5b SO ⭐ `TASK-1427` CAN FALSIFY IT** (binding, marker at `TASKBOARD.md:6059`)

### 4.1 How I got it — **read out of the asset, not guessed**

I could not call MCP (fenced), so I parsed `Content/UI/WBP_SessionMenu.uasset` **offline, read-only**: package name table → export table (55 exports, 112-byte records) → the root `UCanvasPanel`'s **`Slots` array** (8 entries) → each `CanvasPanelSlot`'s **`Content`** object reference. That gives **slot order**, which is exactly what `UWidgetTree::ForEachWidget` walks (`SiegeMenuInputSubsystem.cpp:482-497`: depth-first pre-order, `UPanelWidget` children in `GetChildAt(0..N-1)` order).

**Independent corroboration that the parse is right:** 1 root `CanvasPanel` + 8 canvas children + 3 button label children = **12 widgets walked**, + this `UUserWidget` root = **13 nodes** — ⭐ **exactly the "all 13 nodes" ⭐ `TASK-1399` §2 row 6 counted live.** Two instruments, one number.

### 4.2 The prediction

**FOUR stops, in `UWidgetTree::ForEachWidget` order:**

| stop | name | class | construction route |
|---|---|---|---|
| **0** | `HostButton` | `UButton` | **asset-authored** |
| **1** | `JoinButton` | `UButton` | **asset-authored** |
| **2** | `BackButton` | `UButton` | **asset-authored** |
| **3** | `AddressTextBox` | `UEditableTextBox` | **asset-authored** |

**The eight non-stops, in the same walk, so a short count names a miss rather than a number:**

| # in walk | name | class | why not a stop |
|---|---|---|---|
| 1 | `CanvasPanel` | `UCanvasPanel` | tree root, layout panel — not admitted |
| 2a | `HostLabelText` | `UTextBlock` | `HostButton`'s **content** — walked (`UButton : UContentWidget : UPanelWidget`), correctly rejected |
| 3a | `JoinLabelText` | `UTextBlock` | `JoinButton`'s content |
| 4a | `BackLabelText` | `UTextBlock` | `BackButton`'s content |
| 5 | `StatusTextBlock` | `UTextBlock` | not admitted |
| 6 | `ErrorTextBlock` | `UTextBlock` | not admitted |
| 7 | `BackdropBorder` | `UBorder` | not admitted (it is the `HIT_TEST_INVISIBLE` plate, ⭐ `TASK-355`) |
| 8 | `TitleText` | `UTextBlock` | not admitted |

**Measured canvas slot order (the raw reading, so the prediction is auditable):** `HostButton` → `JoinButton` → `BackButton` → `StatusTextBlock` → `ErrorTextBlock` → `AddressTextBox` → `BackdropBorder` → `TitleText`.

### 4.3 ⛔ THE EMPTY-LIST COUNT AND THE POPULATED RULE — **they are the SAME number, and that is a finding, not a dodge**

The dispatch required these separately and pre-supposed a runtime-populated session list. **There is none.** Joining on this screen is a **typed IPv4 address** (`AddressTextBox` + `ParseJoinAddress`, IPv4-only by ruling 2), **not a server browser**. There is no `UScrollBox` anywhere in the asset (the import table lists exactly `Border`, `Button`, `ButtonSlot`, `CanvasPanel`, `CanvasPanelSlot`, `EditableTextBox`, `TextBlock`) ⇒ **⭐ `TASK-1417`'s `EScrollWhenFocusChanges::NoScroll` finding and its `NativeOnFocusChanging` → `ScrollWidgetIntoView` hook have NO SUBJECT on this screen, and I added neither.**

- **empty-list count: 4** · **populated rule: 4, always** — the stop set is **static**.

### 4.4 ⛔ THE UNHAPPY-PATH COUNT AND THE DISCRIMINATOR

**Unhappy path = `ResolveSessionSubsystem()` returns null** ⇒ `ShowLocalError("Session system unavailable.")`. **Expected stop count on that path: still 4.** Nothing is disabled or collapsed — only `ErrorTextBlock`'s *text* changes.

⇒ **the discriminator that separates a healthy low count from the defect:**
- **4 = healthy**, in *every* state of this screen.
- **3 with `AddressTextBox` missing** ⇒ the `UEditableTextBox` admission at `IsNavFocusStop` (`:466-472`) did not fire — a walker/vocabulary fault, **not** this screen's.
- **3 with a button missing** ⇒ that button carries an authored `IsFocusable = false` (the `WBP_HUD` `Btn_Jump` shape) — an **asset** fault, escalate, do not "fix" the walker.
- **7** ⇒ the registration did not take and the walk fell back to `WBP_MainMenu` (see §2.1's falsifier).
- **0** ⇒ registration returned a target whose `IsInViewport() && IsVisible()` re-validation failed.
- **12** ⇒ the four admitted classes stopped being four.

⭐ **And the negative control is already banked:** ⭐ `TASK-1399` §2 row 6 measured **all 13 nodes `focused:false`** and the tree **byte-identical** after an injected `Down`. **When this lands, that same probe MUST differ.** A byte-identical tree at 5b is a `VERIFY-FAILED`, not an `UNOBSERVABLE` — the instrument is proven alive on this map by the very `IA_MenuAccept` that opens the screen.

---

## 5. THE SPEC (3) STATEMENT — **which blind-spot instrument I relied on**

⛔ **I relied on BOTH, and they agree:**
1. **Aura index** (⭐ `TASK-1398` §4.2): `WBP_SessionMenu` = parent `SessionMenuWidget`, **0 variables, 0 functions**.
2. **A second, independent instrument** (⭐ `TASK-1399` §5.1 table): **ZERO** graph nodes — *"a genuinely empty graph"*.
3. **A third, mine, this row:** the package's own **export table** carries an `EventGraph` export with **three `K2Node_Event` children and nothing else** — i.e. the three bare lifecycle stubs (`Construct` / `PreConstruct` / `Tick` appear in the name table), no call nodes, no `UbergraphPages` logic.

⇒ **the blind spot is effectively CLOSED here, subject only to ⭐ `TASK-1398` §4.1's UNPROVEN OVERRIDE CAVEAT** — the index is *proven* not to enumerate EventGraph **events**, and it is *unproven* whether it would list an overridden UMG **function** such as `On Key Down` (which returns `FReply` and gets its own graph). ⛔ **I am not claiming that caveat away.** What I can add is that the third instrument narrows it: an overridden `OnKeyDown` would be a **separate `EdGraph` export** in this package, and the export table contains exactly **one** graph export (`EventGraph`, index 33). That is a **stronger** reading than the index gives, and it is still not a proof that a `UWidgetBlueprintGeneratedClass` function graph must serialize as its own export — **so the caveat stands, narrowed, not discharged.**

---

## 6. THE NETWORK-UNTOUCHED STATEMENT (spec (4))

⛔ **No session / host / join / network path was changed.** `HostPressed`, `JoinPressed`, `HandleHostClicked`, `HandleJoinClicked`, `HandleSessionStatus`, `HandleSessionError`, `ShowLocalError`, `ResolveSessionSubsystem`, the `bNetActive` / `bPendingConnection` probe, the `LeaveMatch` branch and every line of `USiegeSessionSubsystem` are **byte-identical** (0 deleted lines, and the only statement added inside `BackPressed` sits **below** the network branch's own `return`). **M8 is untouched.**

⛔ **No new key handler** (spec (4)): there is no `NativeOnKeyDown`, no `NativeOnPreviewKeyDown`, no `SetKeyboardFocus`, no `IsFocusable` write and no navigation table in this diff. ⭐ **`IA_MenuAccept` on `BackButton` reaches `BackPressed` through the EXISTING wiring** — the subsystem broadcasts `OnClicked`, which is the same delegate a mouse click fires and which `NativeOnInitialized` already bound to `HandleBackClicked`. **Nothing new was needed for Accept to work.**

⛔ **No asset, no compile, no PIE, no MCP, no git.** ⛔ **The subsystem was read only.**

---

## 7. ⚖️ TWO THINGS I AM **ESCALATING** RATHER THAN TAKING (⭐ `SC-§50`, ⭐ `SC-§101`)

### 7.1 🚨 **THE RING ORDER IS NOT VISUAL ORDER ON THIS SCREEN — and it is the first screen in the epic where that bites**

⭐ `TASK-1406` **declared** this limit where it is produced (`GetMenuFocusStops`, `:490-497`): *"for a `UCanvasPanel` there is no visual order at all — its children are absolutely positioned, so slot order is arbitrary with respect to what 🧑 he sees. This walker cannot detect that case and does not pretend to."* ⭐ `TASK-1415` recorded that Settings **dodged** it by construction (a code-built `UVerticalBox`). **This screen does not dodge it.**

| | order |
|---|---|
| **ring** (slot order, stops only) | `HostButton` → `JoinButton` → `BackButton` → `AddressTextBox` → *(wrap)* |
| **what 🧑 he sees**, top to bottom (design-time centres, ⭐ `TASK-355`) | `AddressTextBox` (y −165) → `HostButton` (−65) → `JoinButton` (+25) → `BackButton` (+125) |

⇒ **all four stops are reachable and the ring wraps — the DEFECT (a stop the player cannot reach) is absent.** What is wrong is the **SEQUENCE**: the ring opens on `Host` rather than the visually topmost control, and the last `Down` jumps from the bottom button back **up** to the address box.

⛔ **NOT FIXED HERE, and the reason is a fence, not a preference:** the fix is a **slot reorder inside `WBP_SessionMenu.uasset`**, and my `names:` line reads *"NEVER a network path, NEVER the subsystem"* with spec (4) scoping me to `SessionMenuWidget.{cpp,h}` and forbidding assets. A C++ runtime re-parent of the canvas children would also move **paint order**, which is product-visible (the `BackdropBorder` plate's `HIT_TEST_INVISIBLE` geometry is load-bearing, ⭐ `TASK-355` §"3 warnings"). **⚖️ Manager row, please — and note it needs a 🧑 human UMG save, so it is an art-director/human-gate shape, not a code row.**

### 7.2 `IA_MenuAccept` and `IA_MenuLeft/Right` are **inert on the `AddressTextBox` stop** — read, declared, not changed

Measured in the subsystem: `HandleMenuAccept` (`:1230`-ish) casts to `UCheckBox` then `UButton`; a `UEditableTextBox` falls through to *"has no Accept semantics"* + one `Log` line. `StepFocusedStop` likewise ends at *"has no Left/Right semantics — nothing done."* ⇒ **a player who rings onto the address box can type into it (correct) but cannot "commit" from it — they must ring on to `JoinButton` and Accept there.** That is defensible and it is **the subsystem's vocabulary, not this screen's**, so I did not widen anything. Flagged so nobody reads the silent Accept as this row's bug.

---

## 8. 🚨 WHAT QA SHOULD SCRUTINISE

1. **§3 — the missing third call site.** I argue the `LeaveMatch` leg must *not* unregister because `OpenLevel` is deferred and the panel stays up. **If that reading of `OpenLevel`'s deferral is wrong, my omission is wrong.**
2. **§2.2 — the guard position.** The unregister sits **below** the `!MainMenu` early return. Confirm that a failed resolve leaves the panel both on screen **and** registered.
3. **§1.1 item 3 — the "no focus call on empty stack" claim.** It rests on `SiegeMenuInputSubsystem.cpp:653-658`. Re-read that `if`.
4. **§4.2 — the stop list.** It is a **prediction from an offline binary parse**, not a runtime read. ⭐ `TASK-1427` should falsify it. **If the order comes back different, the SET should still be those four names** — a set mismatch is the serious finding; an order mismatch is §7.1 and is already declared.
5. **The double-mechanism question (⭐ `TASK-1426` (1)).** I add **no** second re-arm. The check is mechanical: `grep -n "Focus\|SetUserFocus\|SetKeyboardFocus" SessionMenuWidget.cpp` ⇒ the only hits are inside comments.

---

## 9. `## Not examined / limitations`

- ⛔ **Nothing was run.** No compile, no PIE, no MCP, no editor. Every claim here is from **source, or from an offline read-only parse of the `.uasset` bytes**. The runtime half belongs to ⭐ `TASK-1427`.
- ⛔ **The stop ORDER is a binary-parse prediction.** I decoded the export table and the `Slots` array by hand against the UE5 package layout; the 13-node corroboration (§4.1) makes me confident in the **set** and reasonably confident in the **order**, but a hand parse is a hand parse. **It is stated first precisely so it can be falsified.**
- ⛔ **⭐ `TASK-1398` §4.1's unproven-override caveat is NARROWED, NOT DISCHARGED** (§5). I have no positive control proving a UMG function override must appear as its own `EdGraph` export.
- ⛔ **`BP_MenuGameMode` is still BP-opaque**, so `L_MainMenu`'s input posture is asserted from `SessionMenuWidget.cpp:146-147`'s own comment, not measured by me. ⚠️ **Relevant to ⭐ `TASK-1429` (5), not to me:** that row must measure whether `FInputModeUIOnly` ⇒ `SetIgnoreInput(true)` makes Enhanced Input deaf. **On THIS map the lane is demonstrably NOT deaf** — ⭐ `TASK-1399` §3's log shows five `IA_MenuAccept -> OnClicked.Broadcast()` lines on `L_MainMenu`, one of which is the `Button_3` ("Multiplayer") press that **opens this very screen**. That is a data point *for* ⭐ `TASK-1429`, not a measurement of its premise.
- ⛔ **Whether `RemoveFromParent()` invokes `NativeDestruct` synchronously was not verified.** It does not matter: the `BackPressed` unregister runs first either way, and the subsystem's double-call contract makes the second call a logged no-op.
- ⛔ **`ISiegeMenuNavCloseTarget` is NOT implemented** — deliberately. The subsystem header says *"IMPLEMENTING THIS IS NOT THIS ROW'S WORK … ⭐ `TASK-1454` wires the screens"*, and neither ⭐ `TASK-1415` (Settings) nor ⭐ `TASK-1417` (Graphics) implements it. ⇒ **`IA_MenuBack` is inert on this screen today**, exactly as on every other screen, and that is ⭐ `TASK-1454`'s row. **The `Backspace`/`Gamepad_FaceButton_Right` key will log "implements no `ISiegeMenuNavCloseTarget` — Back is INERT for it" and do nothing.** 🧑 He must click `Back`, or ring onto `BackButton` and press Accept.
- ⛔ **§7.1 (ring order ≠ visual order) is NOT fixed** and needs a manager decision plus a 🧑 human UMG save.
- ⛔ **I did not measure what happens if `AddressTextBox` holds Slate focus and `Left`/`Right` are pressed** — `SEditableText` may consume them as caret movement before Enhanced Input sees the key. Either outcome is acceptable (caret move, or the logged no-op of §7.2); it is named so a 5b reading of "Left did nothing visible" is not mis-filed as a defect.
