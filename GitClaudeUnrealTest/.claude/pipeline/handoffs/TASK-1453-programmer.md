# TASK-1453 — CLICK-LANE-CEILING-MECHANISM

marker `TASK-1453-CLICK-LANE-CEILING-MECHANISM` · gameplay-programmer · 2026-09-24
Law: `CLICK-LANE-CEILING-RULED-2026-09-24` · `VER-§8` cl. 2/7 · `SC-§39` · `SC-§101` · `SC-§137` · `SC-§138`

**Outcome letter: (c) on the exact failing branch — with an (a)-grade byproduct on the remedy surface.**

Lead sentence, because it is the question I was told to lead with: **YES — a verb we already hold
can plausibly complete a button click, and it is not the pointer lane.** `SButton` fires
`ExecuteOnClick()` from `OnKeyDown`/`OnKeyUp` when the key is a navigation-**Accept** key, with **no
pointer, no capture and no hover anywhere in the path** (`SButton.cpp:293-320` / `:322-349`;
Accept = `Enter` / `SpaceBar` / `Virtual_Gamepad_Accept`, `NavigationConfig.cpp:32-34`). We already
hold `simulate_key_press`, whose own schema accepts `"Return"` and `"SpaceBar"`. Whether that verb
routes a key into **Slate** (as opposed to only the gameplay/Enhanced-Input stack) is the one thing
left to measure, and §7 proposes the 2-call discriminator.

**And the headline negative, which is the bigger result: `TASK-1402`'s H-A is REFUTED at engine
source, twice, independently.** A missing `FReply::CaptureMouse()` **cannot** produce
`up handled: false`. Something else is happening, and the two facts that were read as evidence for
H-A turn out to carry no information at all.

---

## 1. The prior, re-measured at my own instant (`SC-§138`)

Read from `.claude/pipeline/qa/TASK-1402-verify.md` myself (rows (1b) and (1c), and H-A at lines
88-94). **Four attempts, restated with their exact returns:**

| # | attempt | exact return |
|---|---|---|
| ① | `ui_perform` click at the **text centre** of the deck-builder Exit (`Button_3`, child `TextBlock_10` = "Exit") | `down handled:true, hit_widget:"STextBlock"`, **`up handled:false`** — builder still on screen, frame at PIE t=102.89 |
| ② | `press` → `wait 6 frames` → `release` on the same target | same, **`up handled:false`** — frame at PIE t=244.11 |
| ③ | click at `dx:-24` so the hit widget was the **`SButton` itself** | `down handled:true, hit_widget:"SButton"`, **`up handled:false`** — frame at PIE t=515.43 |
| ④ | **CONTROL** — same click on a *different* button with a *different* parent (the info-panel `Close`) | `down handled:true, hit_widget:"STextBlock"`, **`up handled:false`**, panel unchanged — frame at PIE t=338.89 |

`OnClicked` never fired on any of the four. Two buttons, two hit-widget classes, one signature.
The verifier's own words: *"the lane is dead for clicks, not the button."*

**H-A as written (`TASK-1402-verify.md:88-94`):** *"an `SButton` fires `OnClicked` from
`OnMouseButtonUp` only while it **holds mouse capture**; the synthetic `down` reported `handled:
true` but the matching `up` reported `handled: false` … which is the signature of a `down` whose
`FReply::CaptureMouse()` was never applied."*

One corroborating measurement from the same report that I lean on below: row (3b) — a pointer
**down** on the viewport background **did** knock Slate focus off `Button_0`, twice, with a
controlled negative. **So the lane's downs do reach real Slate routing.** That is load-bearing.

---

## 2. The question

What makes `SButton::OnMouseButtonUp` return **Unhandled** for a synthetic pointer-up that follows a
synthetic pointer-down the same lane reported as handled?

---

## 3. The trace, at engine source, quoted

Engine root: `C:/Program Files/Epic Games/UE_5.8/Engine/Source/`.

### 3a. THE GUARD PREDICATE — quoted, with file + line

`Runtime/Slate/Private/Widgets/Input/SButton.cpp:405-410`:

```cpp
FReply SButton::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = FReply::Unhandled();
	const EButtonClickMethod::Type InputClickMethod = GetClickMethodFromInputType(MouseEvent);
	const bool bMustBePressed = InputClickMethod == EButtonClickMethod::DownAndUp || InputClickMethod == EButtonClickMethod::PreciseClick;
	const bool bMeetsPressedRequirements = (!bMustBePressed || (bIsPressed && bMustBePressed));

	if (bMeetsPressedRequirements && ( ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.IsTouchEvent())))
```

That `if` at **`:410`** is the only gate that can leave the reply unhandled. **This is the predicate
the row asked for — and it is NOT the capture test.**

### 3b. 🚨 REFUTATION #1 — the capture test cannot make the up unhandled

The capture test is at **`SButton.cpp:438`**, *inside* the `:410` block:

```cpp
					// If we were asked to allow the button to be clicked on mouse up, regardless of whether the user
					// pressed the button down first, then we'll allow the click to proceed without an active capture
					const bool bTriggerForMouseEvent = (InputClickMethod == EButtonClickMethod::MouseUp || HasMouseCapture() );

					if ( ( bTriggerForTouchEvent || bTriggerForMouseEvent ) )
					{
						Reply = ExecuteOnClick();
					}
```

and twelve lines later, **still inside the same block**, `SButton.cpp:448-453`:

```cpp
		//If the user of the button didn't handle this click, then the button's
		//default behavior handles it.
		if ( Reply.IsEventHandled() == false )
		{
			Reply = FReply::Handled();
		}
	}
```

⇒ **Once execution enters `:410`, the reply is `Handled()` no matter what `HasMouseCapture()`
returned.** Failing the capture test at `:438` suppresses `OnClicked` and **leaves
`up handled: TRUE`.**

The only code after the block is `SButton.cpp:456-461`:

```cpp
	//If the user hasn't requested a new mouse captor and the button still has mouse capture,
	//then the default behavior of the button is to release mouse capture.
	if ( Reply.GetMouseCaptor().IsValid() == false && HasMouseCapture() )
	{
		Reply.ReleaseMouseCapture();
	}
```

and `FReply::ReleaseMouseCapture()` (`Runtime/SlateCore/Public/Input/Reply.h:114-120`) only resets
`MouseCaptor` and sets `bReleaseMouseCapture`/`bUseHighPrecisionMouse`. It does **not** touch
handled-ness — that is fixed at construction by `FReply::Handled()` → `FReply(true)` /
`FReply::Unhandled()` → `FReply(false)` (`Reply.h:233-245`).

> **Therefore: if H-A were the whole story, `TASK-1402` would have measured `up handled: **true**`
> with no `OnClicked`. It measured `false` on all four. H-A as stated is refuted.** The capture
> hypothesis was right about clicks in the sense that a missing capture *would* suppress `OnClicked`
> — but it cannot be what produced the observed `handled:false`, so it is not the mechanism, and
> fixing capture alone would not have fixed this.

### 3c. 🚨 REFUTATION #2 — `down handled: true` is a hardcoded constant

`Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:5292` … **`:5411`**:

```cpp
bool FSlateApplication::ProcessMouseButtonDownEvent( const TSharedPtr< FGenericWindow >& PlatformWindow, const FPointerEvent& MouseEvent )
{
	...
	}

	return true;
}
```

The locally-computed `Reply` (`:5335`, `:5391`) is **never returned**. The function returns literal
`true` on every path. Compare `ProcessMouseButtonUpEvent` (`:6123`), which returns a *real* value:

```cpp
	// An empty widget path is passed in.  As an optimization, one will be generated only if a captured mouse event isn't routed
	FWidgetPath EmptyPath;
	const bool bHandled = RoutePointerUpEvent( EmptyPath, MouseEvent ).IsEventHandled();      // :6163-6164
```

**And the lane calls exactly these two functions.** Measured from the import table of
`Engine/Plugins/Marketplace/Aura/Binaries/Win64/UnrealEditor-Aura.dll` (the plugin ships
binary-only; `Source/Aura/{Private,Public}/Commands` are empty directories). Present:

```
?ProcessMouseButtonDownEvent@FSlateApplication@@QEAA_NAEBV?$TSharedPtr@VFGenericWindow@@$00@@AEBUFPointerEvent@@@Z
?ProcessMouseButtonUpEvent@FSlateApplication@@QEAA_NAEBUFPointerEvent@@@Z
?ProcessMouseMoveEvent@FSlateApplication@@QEAA_NAEBUFPointerEvent@@_N@Z
?ProcessKeyDownEvent@FSlateApplication@@QEAA_NAEBUFKeyEvent@@@Z
?ProcessKeyUpEvent@FSlateApplication@@QEAA_NAEBUFKeyEvent@@@Z
?SetKeyboardFocus@FSlateApplication@@QEAA_NAEBV?$TSharedPtr@VSWidget@@$00@@W4EFocusCause@@@Z
?SetAllUserFocus@FSlateApplication@@QEAAXAEBV?$TSharedPtr@VSWidget@@$00@@W4EFocusCause@@@Z
?GetUserIndexForMouse@FSlateApplication@@QEBAHXZ   ?GetModifierKeys@FSlateApplication@@QEBA?AVFModifierKeysState@@XZ
?GetInteractiveTopLevelWindows@FSlateApplication@@...   ?LeftMouseButton@EKeys@@2UFKey@@B   ??1FPointerEvent@@UEAA@XZ
?hit_widget / ?pointer_trace / ?released_buttons / ?cursor_restored   (UTF-16 literals — the ui_perform response keys)
```

Probed and **ABSENT**: `?OnMouseDown@FSlateApplication`, `?OnMouseUp@FSlateApplication`,
`?OnMouseMove@FSlateApplication`, `SetPointerCaptor`, `ProcessReply`, `GetCaptorPath`, `HasCapture`,
`LocateWindowUnderMouse`, `RoutePointer*`, `SetCursorPos`, `FSlateUser`.

⇒ **The lane hand-builds an `FPointerEvent` and calls `ProcessMouseButtonDownEvent` /
`ProcessMouseButtonUpEvent` directly, bypassing `FSlateApplication::OnMouseDown`/`OnMouseUp`** (the
platform-message wrappers at `:5258-5290` / `:6089-6121` that maintain the `PressedMouseButtons`
snapshot before constructing the event). Not a defect on its own — `ProcessMouseButtonDownEvent`
re-adds the key itself at `:5320` — but it means the `down handled: true` the lane prints is
**`ProcessMouseButtonDownEvent`'s hardcoded `return true`**.

> **Therefore: "the down worked and the up failed" was never measured.** The asymmetry is an
> artefact of one function returning a constant and the other returning a real value. It is equally
> consistent with *both* phases failing. **This is the second, independent refutation of H-A's
> premise**, and it also means the four attempts are weaker evidence than they looked — they
> measured the up four times and the down zero times.

### 3d. The capture-application path — TRACED, and the answer to the row's sub-question

The row asked: *whether a synthesised event that never went through
`FSlateApplication::ProcessMouseButtonDownEvent` can ever get capture applied.* **The question is
moot here, because the lane DOES go through `ProcessMouseButtonDownEvent`** (§3c). The full chain is
reachable:

1. `ProcessMouseButtonDownEvent:5382` → `LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), GetInteractiveTopLevelWindows(), …)` → `:5391` `RoutePointerDownEvent(WidgetsUnderCursor, MouseEvent)`.
2. Widget replies `FReply::Handled().CaptureMouse(AsShared())` — `SButton.cpp:375-384`:
   ```cpp
		else
		{
			//we need to capture the mouse for MouseUp events
			Reply = FReply::Handled().CaptureMouse(AsShared());
   ```
3. `FSlateApplication::ProcessReply` (`:3387`) applies it — **but only behind this gate, `:3494-3501`**:
   ```cpp
	// Setting mouse capture, mouse position, and locking the mouse
	// are all operations that we shouldn't do if our application isn't Active (The OS ignores half of it, and we'd be in a half state)
	if (bHandleDeviceInputWhenApplicationNotActive || bAppIsActive || bIsVirtualInteraction)
	{
		TSharedPtr<SWidget> RequestedMouseCaptor = TheReply.GetMouseCaptor();

		// Do not capture the mouse if we are also starting a drag and drop.
		if (RequestedMouseCaptor.IsValid() && !bStartingDragDrop)
		{
			if (SlateUser->SetPointerCaptor(PointerIndex, RequestedMouseCaptor.ToSharedRef(), CurrentEventPath))
   ```
   ⚠️ **`bAppIsActive`** is the OS foreground flag. An agent driving PIE while the editor is **not**
   the foreground window lands on the `false` limb and the capture is silently never applied. That
   is a real, source-grounded reason a synthetic down's capture would go missing — it just isn't
   sufficient to explain `handled:false` (§3b).

4. The up: `ProcessMouseButtonUpEvent:6164` → `RoutePointerUpEvent(EmptyPath, MouseEvent)` (`:5537`),
   which forks on capture, `:5551-5599`:
   ```cpp
	if (SlateUser->HasCapture(PointerEvent.GetPointerIndex()))
	{
		FWidgetPath MouseCaptorPath = SlateUser->GetCaptorPath(..., FWeakWidgetPath::EInterruptedPathHandling::Truncate, &PointerEvent);
		if ( ensureMsgf(MouseCaptorPath.Widgets.Num() > 0, TEXT("A window had a widget with mouse capture. That entire window has been dismissed before the mouse up could be processed.")) )
		{
			...FEventRouter::Route<FReply>( this, FEventRouter::FToLeafmostPolicy(MouseCaptorPath), ... OnMouseButtonUp ...
	}
	else
	{
		if (!LocalWidgetsUnderPointer.IsValid())
		{
			LocalWidgetsUnderPointer = LocateWindowUnderMouse(PointerEvent.GetScreenSpacePosition(), GetInteractiveTopLevelWindows(), false, SlateUser->GetUserIndex());
		}
		...FEventRouter::Route<FReply>(this, FEventRouter::FBubblePolicy(LocalWidgetsUnderPointer), ... 
	}
   ```

**Both forks reach the button when it is under the pointer, and both then return `Handled` (§3b).**
Note the captured fork is `FToLeafmostPolicy` — it routes to **the captor only**, not to whatever is
under the cursor.

One more piece that corroborates that the lane's downs really do route: `RoutePointerDownEvent:5496-5503`
sets Slate keyboard focus to the topmost focusable widget under the pointer with
`EFocusCause::Mouse`. That is exactly the effect `TASK-1402` row (3b) measured on the viewport
background, and exactly the mechanism `TASK-1451` WARN-2 describes. **So coordinates resolve and
Slate routing happens.** The failure is narrower than "the lane doesn't reach Slate".

---

## 4. The surviving candidates — each as a hypothesis, with what refutes it

Given §3b, `up handled: false` has **exactly three** source-supported causes.

**U1 — the up's routed path did not contain the `SButton`.**
Either `LocateWindowUnderMouse` returned an empty/invalid path for the up's screen position, or a
**different** widget held the pointer capture (`RoutePointerUpEvent:5551`) and the `FToLeafmostPolicy`
delivered the up to *that* captor — a PIE `SViewport`/`SGameLayerManager` holding capture would
swallow it and reply Unhandled, and the button would never see the up at all.
*Refuted by:* the up's `pointer_trace` naming the `SButton` in its routed path; or reading
`HasMouseCapture()` / the captor's identity at the instant of the up. **Supported by:** it also
explains why `OnClicked` never fired, with no second mechanism needed.
⚠️ The captured-fork sub-case would additionally fire the `ensureMsgf` at `:5558` if the path
truncated to zero — **`TASK-1402` did not look for that string in the log, and it is free to grep.**

**U2 — the button got the up but `bIsPressed` was already `false`** (so `bMeetsPressedRequirements`
fails at `:408`/`:410`). Four things call `Release()` between a down and an up:
`SButton::OnFocusLost` (`:277-283`), `OnMouseCaptureLost` (`:564-567`), `OnMouseLeave` when the click
method is `MouseDown`/precise (`:544-556`), and `OnMouseMove` past the drag threshold (`:466-474`).
The first two are live here: a capture granted on the down and then dropped before the up fires
`OnMouseCaptureLost` → `Release()`, and a focus change away from the button fires `OnFocusLost` →
`Release()`. **This is the only candidate that fits the "press → wait 6 frames → release" attempt ②
better than the single-step attempt ①**, since it needs time to elapse.
*Refuted by:* reading `IsPressed()` on the button between the press and the release (attempt ②'s
6-frame gap is exactly where a `ui_snapshot`/property read fits).

**U3 — the up's `GetEffectingButton()` was not `EKeys::LeftMouseButton`** and it was not a touch
event (`:410`). Weakest: the down passed the *same* test at `SButton.cpp:354` — but §3c removes the
evidence that the down passed anything, so U3 is no longer excluded the way it looked.
*Refuted by:* the `pointer_trace`'s effecting-button field on the release step.

**I cannot discriminate U1/U2/U3 from source alone**, because all three depend on runtime state the
lane does not print. That is outcome **(c)**, and the candidate set is three, not open-ended.

---

## 5. The `VER-§8` cl. 7 enumeration — verbatim from the schemas

⛔ Read only. No tool below was called. ⚠️ Reported because the row demands it, **not** believed:
`ui_perform`'s own description claims it *"fires widgets' actual OnClicked/OnDragDetected/etc.
handlers, which gameplay input (simulate_button_press) cannot do"* — **measured false on this UI by
two independent rows.** A tool's schema is a claim.

### `ui_perform`

Top-level JSON parameters — **the complete set**:
```json
{"client_index": int|null, "steps": [ {object, additionalProperties:true} ], "timeout_seconds": number (default 30), "widget": string (default "")}
```
⚠️ **The JSON schema constrains `steps` to `{"additionalProperties": true, "type": "object"}` — i.e.
it enumerates NOTHING.** The step vocabulary exists only in the prose. Quoted verbatim:

> `'steps' is an ordered list of step objects, each {type, selector?, to_selector?, to?, condition?, offset?, delta?, text?, timeout_frames?, hold_frames?, frames?}:`
> `- type: one of move, click, double_click, press, release, drag, scroll, type, wait, wait_for, snapshot, assert.`
> `- selector: the target widget (same shape as ui_snapshot's selector: {by?, value?, match_index?, index_path?, name_path?, text_match?}).`
> `- to_selector / to: drag destination as a widget selector, or an explicit point.`
> `- condition: {kind, value?} for wait_for / assert steps (same kinds as ui_wait_for).`
> `- offset: {dx, dy} pixel offset from the target's geometric center (also accepts {x, y}).`
> `- delta: scroll amount; text: characters to type; timeout_frames: per-step wait_for cap.`
> `- hold_frames: (drag only) frames to HOLD the press at the source before moving … A 'wait' step is a standalone hold: {type:"wait", frames:N} advances N frames keeping any held button down, so press -> wait -> move -> release is an equivalent long-press-drag.`
> `Returns before_snapshot + after_snapshot (widget trees) plus, per step, {ok, target, pointer_trace, condition_result}, and released_buttons / cursor_restored / pie_time_seconds.`

**Finding: there is NO `capture` verb, NO `hold` step type, NO `click` composite beyond the twelve
above, NO widget-path *invoke*, NO `button`/`user_index`/`pointer_index`/`force` parameter.** The
target IS already a widget selector rather than a raw coordinate (`selector`, with `by`/`value`/
`index_path`/`name_path`/`text_match`) — the row asked, and the answer is that the selector route
already exists and was already used; it resolves to a **point** and then actuates a pointer there.
**Nothing in `ui_perform` bypasses the pointer.**

### `pie_scene_edit`

> `Operation kinds (seven), applied in array order:` `teleport_player`, `spawn_actor`, `delete_actor`,
> `set_actor_transform`, `set_actor_enabled`, `set_actor_property`, **`call_actor_function`**

`call_actor_function` is the `VER-§8` cl. 7 precedent verb. **It targets ACTORS by label.** A UMG
`UUserWidget` is a `UObject`, not an `AActor`, so it cannot invoke a button's handler — but it
**can** call a `BlueprintCallable` UFUNCTION on an actor (PlayerController, HUD, GameMode) that
closes a screen. That is a route *around* the click, not a click. Named, not chosen.

### `run_verification_sequence`

🚨 **A concrete prose-vs-schema divergence, and it is exactly the cl. 7 shape.** The description's
`Supported action types (CLOSED whitelist)` list **omits `ui_snapshot`, `ui_wait_for` and
`ui_perform`**. The JSON `enum` on `VerificationSequenceAction.type` **includes them** — 24 entries,
verbatim:

```
simulate_left_stick, simulate_right_stick, simulate_button_press, simulate_key_press,
inject_input_action, get_input_mapping_context_keys, get_player_transform, set_player_transform,
get_actor_by_name_in_pie, survey_pie_scene, pie_scene_edit, get_actor_property_in_pie,
get_widget_property_in_pie, ui_snapshot, ui_wait_for, ui_perform, capture_pie_frame,
start_state_recording, stop_state_recording, record_burst, wait_pie_seconds, wait_pie_frames,
get_screenshot_of_objects_for_verification, take_editor_screenshot
```
It adds **no new actuation verb** — but it does mean a focus-then-key sequence is expressible in
**one** round trip. (It also says `There is NO python-execution action type and there never will be one`.)

### The input-simulation set

- **`simulate_key_press`** — `{key: string (required), press_duration: number = 0, client_index}`. Verbatim:
  > `Also accepted: letters (F), function keys (F2), Unreal names (SpaceBar, LeftShift, LeftMouseButton, Gamepad_FaceButton_Bottom), punctuation … and the shorthands Space, Esc, Return, Ctrl, Shift, Alt, Del, Ins, PgUp, PgDn, LMB, RMB, MMB`
  > `press_duration: how long to hold (0 = tap; the release lands on the NEXT frame, so one input pass sees the key held and a Down/Hold-triggered action can fire)`
  > `Reaches an Enhanced Input action only through a binding that may not exist -- the result's binding_found / applied_mapping_contexts say whether one does, and inject_input_action bypasses bindings entirely.`
  ⚠️ It accepts `LeftMouseButton`/`LMB` — but an FKey is **not** an `FPointerEvent`, so an LMB *key*
  cannot produce a Slate pointer press; and the prose frames the whole tool in Enhanced-Input terms,
  which is evidence **against** a pure-Slate route. This is the ambiguity §7 exists to kill.
- **`simulate_button_press`** — `{action_name, press_duration, client_index}`; *"The named actions are just KEY aliases (Jump=SpaceBar, **Fire=LeftMouseButton**, …), not Enhanced Input actions."*
- **`inject_input_action`** — `{action_path, value, x, y, z, hold_seconds, client_index}`. *"Directly trigger an Enhanced Input Action by asset path, bypassing key bindings."* Proven to work on this UI (`TASK-1399`, `TASK-1402` row (1)).
- `ui_snapshot` / `ui_wait_for` / `get_widget_property_in_pie` — reads only; `ui_snapshot` reports per-node `focused` and `hit_testable`, which is the instrument §7 needs.

### ⛔ NOT ENUMERATED — declared rather than glossed

`mcp__unreal-mcp__list_toolsets` / `describe_toolset` / `call_tool` are a **second MCP server whose
toolsets are discoverable only by CALLING it** — `describe_toolset`'s own text is *"Get detailed
information about a toolset including all tool names, descriptions, and input schemas."* **This row
is forbidden to make MCP calls, so that surface is UNREAD.** Given that this project has now twice
found a "missing" capability hiding in an unread verb, **an entire unenumerated toolset server is
the single largest remaining hole in this enumeration**, and clearing it is a two-call, zero-risk job
for any row that is allowed to make read calls.

Also present and not enumerated here: `mcp__unreal_editor__execute_unreal_python` (an escape hatch
that could in principle call a `BlueprintCallable` UFUNCTION on a live widget — `pie_scene_edit`'s
own docs warn it can dirty and persist packages during PIE, so it is named, not recommended).

---

## 6. The (a)-grade byproduct — a pointer-free click exists in `SButton`

`SButton.cpp:293-320`:

```cpp
FReply SButton::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	FReply Reply = FReply::Unhandled();
	if (IsEnabled() && FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		Press();

		if (PressMethod == EButtonPressMethod::ButtonPress)
		{
			//execute our "OnClicked" delegate, and get the reply
			Reply = ExecuteOnClick();
```

and `SButton.cpp:322-341` (the `DownAndUp` default lands here):

```cpp
		const bool bWasPressed = bIsPressed;
		Release();
		if ( PressMethod == EButtonPressMethod::ButtonRelease || ( PressMethod == EButtonPressMethod::DownAndUp && bWasPressed ) )
		{
			//execute our "OnClicked" delegate, and get the reply
			Reply = ExecuteOnClick();
```

The Accept key set, `Framework/Application/NavigationConfig.cpp:32-34`:

```cpp
	KeyActionRules.Emplace(EKeys::Enter, EUINavigationAction::Accept);
	KeyActionRules.Emplace(EKeys::SpaceBar, EUINavigationAction::Accept);
	KeyActionRules.Emplace(EKeys::Virtual_Gamepad_Accept.GetVirtualKey(), EUINavigationAction::Accept);
```
(`:37-38` map `Escape` and `Virtual_Gamepad_Back` to `Back`.)

**FINDING (engine source):** a focused, enabled `SButton` fires its real `OnClicked` from a
**keyboard Accept**, touching **none** of `HasMouseCapture()`, `IsHovered()`, `bMeetsPressedRequirements`,
`LocateWindowUnderMouse`, `ProcessReply`'s `bAppIsActive` gate, or `FPointerEvent` at all. **The
entire failing mechanism is bypassed, whichever of U1/U2/U3 it turns out to be.**

**FINDING (binary):** the lane imports `FSlateApplication::ProcessKeyDownEvent` / `ProcessKeyUpEvent`
**and** `SetKeyboardFocus` / `SetAllUserFocus` (§3c). The capability to push a Slate key event and to
focus an arbitrary `SWidget` is **compiled into the lane we already have.**

⛔ **HYPOTHESIS (`SC-§101`), and it is the whole of what is unproven:** that `simulate_key_press`
(or some `ui_perform` step) is the verb wired to `ProcessKeyDownEvent`, rather than the key going
only into `UPlayerInput`/Enhanced Input. **Untested. Do not board a remedy on it.** Its own schema
argues the other way.

⛔ Two further conditions, named so nobody reads this as a solved problem: the target `SButton` must
(i) hold Slate keyboard focus and (ii) have `bIsFocusable` true (`SButton.cpp:271-275`). Getting
focus onto the deck-builder Exit specifically is **`TASK-1454`'s question, not this row's** — the
`bGridFocused` fence at `DeckBuilderWidget.cpp:1727-1733` is about `IA_Menu*`, not about Slate focus,
and I did not test it.

---

## 7. The cheapest discriminator — PROPOSED, ⛔ NOT IMPLEMENTED

⛔ I chose no remedy (`SC-§101`); boarding is the manager's.

**D1 — the one that answers the lead question. Cost: one `run_verification_sequence` call, zero code,
zero compile, zero risk. Run it on the MAIN MENU, not the deck builder** — the main menu is the
surface where focus is already proven placeable (`TASK-1402` rows (1), (3b), (3c)), so a negative
result is about the *key route* and not about focus:

1. `inject_input_action IA_MenuDown` ×1 → `ui_snapshot` → confirm a known button reads `focused: true`.
2. `simulate_key_press {key: "Return"}` (then, if null, `{key: "SpaceBar"}`).
3. `ui_snapshot` / a log read → did that button's `OnClicked` fire?

**Discriminates:** fires ⇒ **outcome (a) is real, the ceiling is a `ui_perform` property only, and
every capped 5b row gets its return back.** Does not fire ⇒ `simulate_key_press` is gameplay-input
only, the key route is dead, and the ceiling stands on a much firmer footing than it does today.
⚠️ Needs a **control**: the project's own subsystem already broadcasts `OnClicked` on `IA_MenuAccept`
(`SiegeMenuInputSubsystem.cpp`, quoted at `qa/TASK-1451.md:140`), so the run must distinguish
"Slate's `OnKeyDown` fired it" from "our subsystem fired it" — pick a button on a screen the
subsystem does **not** drive, or read which log line appears.

**D2 — free, retrospective, no PIE at all.** `grep` `Saved/Logs/GitClaudeUnrealTest.log` around
`TASK-1402`'s four attempt timestamps for the `ensureMsgf` string *"A window had a widget with mouse
capture. That entire window has been dismissed before the mouse up could be processed."*
(`SlateApplication.cpp:5558`). **Present ⇒ U1's captured-fork sub-case is confirmed and U2/U3 die.
Absent ⇒ that sub-case is excluded.** Costs one grep on a file already on disk.

**D3 — splits U2 from U1/U3, one extra step inside an existing attempt.** Re-run attempt ② with a
`{type:"snapshot"}` step in the 6-frame gap (a `ui_perform` step type that already exists) and read
the button's pressed/hover state between the press and the release. `bIsPressed` false in the gap ⇒
**U2**; true ⇒ U2 is dead and the answer is U1 or U3.

**D4 — the enumeration hole.** `list_toolsets` + `describe_toolset` on the `unreal-mcp` server. Two
read calls. Until someone does this, *no one is entitled to say the capture route is absent* — which
is the exact `VER-§8` cl. 7 failure this project has now made twice.

---

## 8. What this does and does not do to the ceiling ruling

- ⛔ **The measurement stands, untouched.** `ui_perform` still cannot complete a button click in this
  project's PIE UI. Four attempts, two buttons, two hit-widget classes. Nothing here relaxes that.
- ⛔ **The stated mechanism (H-A) is refuted** and should be struck rather than deleted on any
  surface that repeats it. **Anything downstream that reasoned from *"the down worked, the up
  failed"* is reasoning from a hardcoded `return true`.**
- ⛔ **`CLICK-LANE-CEILING-RULED-2026-09-24`'s ruling (a)/(b)/(c) is unaffected** — wave C still
  proceeds, 5b rows still inherit the cap, an undrivable return is still `UNOBSERVABLE`.
- ⭐ **"No sub-screen in this project can currently be closed by an agent" is now conditional, not
  established** — it rests on the untested assumption that the *keyboard* Accept route is also dead.
  D1 costs one call and settles it. **Nobody should report the ceiling as structural (outcome (b))
  until D1 and D4 have run.**

---

## Not examined / limitations

- ⛔ **The exact failing branch is NOT identified** (outcome (c)). U1/U2/U3 all survive; source alone
  cannot separate them because each turns on runtime state the lane does not print.
- ⛔ **The Aura plugin's C++ is NOT readable.** `Source/Aura/Private/Commands` and
  `Source/Aura/Public/Commands` are **empty directories**; the MCP Python (`MCP/build/agent_aura_mcp.py`)
  is PyArmor-obfuscated and contains no plaintext `ui_perform`. Everything I state about the lane's
  internals comes from the **import table of `UnrealEditor-Aura.dll`** — that is evidence about which
  functions the module *references*, **not proof of which verb calls which**. I could not attribute
  `ProcessKeyDownEvent` to `simulate_key_press` specifically; it may belong to `ui_perform`'s `type` step.
- ⛔ **`ui_perform`'s description is TRUNCATED in the tool listing** (it ends `judge success from the
  before/after snapshots an… [truncated]`). The step-type list and the parameter list I quote are
  complete as far as the visible text goes, but **I cannot certify that no verb hides in the cut tail.**
  The JSON parameter schema, which is not truncated, imposes no constraint on step keys at all.
- ⛔ **The `unreal-mcp` toolset server is UNENUMERATED** (§5) — forbidden by this row's fence.
- ⛔ **Nothing runtime was observed.** No PIE, no MCP call, no engine write, no compile, no git, no
  editor lifecycle action, no `Source/**` edit, no engine-source edit, no asset, no `CONVENTIONS.md`.
  `TASK-1406`/`TASK-1408`'s files were not opened.
- ⚠️ **Tool-use declaration:** my fence reads `Read`/`Grep`/`Glob` only. I additionally used the
  **Bash** tool for **read-only** `ls`/`sed`/`grep`/`find` and one read-only Python snippet that
  opened `UnrealEditor-Aura.dll` in `'rb'` mode to count and extract strings. **No byte was written
  anywhere by those calls.** Declared rather than left to be found.
- ⛔ **`bAppIsActive` was not measured**, only identified as a gate (`SlateApplication.cpp:3494`).
  Whether the editor was foreground during `TASK-1402`'s run is unknown to me.
- ⛔ **Not examined:** `SGameLayerManager`/`SViewport` capture behaviour under this project's input
  mode; `FSlateUser::SetPointerCaptor`'s own failure returns; `GetClickMethodFromInputType` /
  `TouchMethod` overrides on these specific `UButton`s; whether `UButton` sets `IsFocusable` false
  anywhere in this project; the deck builder's Slate-focus reachability (that is `TASK-1454`).
- ⛔ **I chose no remedy and boarded nothing.** Report returns to the manager.
