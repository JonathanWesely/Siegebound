# TASK-1446 — [FOCUS-RING-LANE-DIVERGENCE-MEASURE]

**Marker:** `TASK-1446-FOCUS-RING-LANE-DIVERGENCE-MEASURE`
**Agent:** gameplay-programmer · 2026-09-24
**Method:** `Read` / `Grep` only. ⛔ Zero code, zero asset, zero engine action, zero PIE, zero MCP, zero git, no editor lifecycle action. The GUI editor was not touched.
**Law:** `SC-§39` · `SC-§50` · `SC-§101` · `SC-§137` · `VER-§8` cl. 4

---

## 0. ANSWERS FIRST

**(6) THE LETTER: β.** A **test-rig fix**. On the evidence I can reach by reading, there is **no product defect** — the ring is correctly asked for, correctly configured, and correctly drawn in both lanes. What differs is **how big it is rendered and how well the capture can see it**. See §6 for the argument, including the one rider that keeps α alive at low probability.

**(3a) THE FOCUS CAUSE: `EFocusCause::Navigation`, passed EXPLICITLY — twice, on both branches. Not defaulted.**

**🚨 (4) THE DISPATCH'S OWN LEADING CANDIDATE IS REFUTED, AND I SAY IT FIRST AND LOUDEST.** The hypothesis *"Slate distinguishes focus set by navigation from focus set directly, and the visual follows the cause"* is **correct as a description of Slate** and **inapplicable to this code**, because this code already passes the navigation cause. Three independent source facts kill it (§2). A row that could only confirm its own dispatch is not a measurement; this one refutes it.

---

## 1. (3a) `FocusButton` — QUOTED VERBATIM

`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeMenuInputSubsystem.cpp:374-405`

```cpp
bool USiegeMenuInputSubsystem::FocusButton(UButton* Button) const
{
	APlayerController* PC = GetLocalController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Button || !LocalPlayer || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	TSharedPtr<SWidget> SlateButton = Button->GetCachedWidget();
	if (!SlateButton.IsValid() || !SlateButton->SupportsKeyboardFocus())
	{
		return false;
	}

	// EFocusCause::Navigation, NOT UWidget::SetUserFocus's SetDirectly: FSlateApplication::
	// SetUserFocus sets ShowFocus = (InCause == Navigation) (SlateApplication.cpp:3099), and
	// that flag is what makes SWidget::Paint draw the "FocusRectangle" brush (SWidget.cpp:1748)
	// — the visible focus state of deliverable (3). Same user-index resolution as
	// UWidget::SetUserFocus (Widget.cpp), same fallback to the local player's deferred
	// Slate operations if the immediate set is refused this frame.
	const int32 UserIndex = FSlateApplication::Get().GetUserIndexForController(LocalPlayer->GetControllerId());
	FReply& DelayedSlateOperations = LocalPlayer->GetSlateOperations();
	if (FSlateApplication::Get().SetUserFocus(UserIndex, SlateButton, EFocusCause::Navigation))
	{
		DelayedSlateOperations.CancelFocusRequest();
		return true;
	}

	DelayedSlateOperations.SetUserFocus(SlateButton.ToSharedRef(), EFocusCause::Navigation);
	return false;
}
```

**API:** `FSlateApplication::Get().SetUserFocus(UserIndex, SlateButton, EFocusCause::Navigation)` at **:397**, with a deferred fallback `FReply::SetUserFocus(SlateButton, EFocusCause::Navigation)` at **:403**.

**FOCUS CAUSE: `EFocusCause::Navigation` — EXPLICIT on both branches.** Neither call relies on a default. (For contrast: `FReply::SetUserFocus`'s *default* cause is `SetDirectly`; this code overrides it.)

The author's own comment at :389-394 already predicted the mechanism the dispatch names, and **wrote the code to satisfy it**. Whatever is wrong is downstream of this function.

---

## 2. (3c) WHERE THE RING ACTUALLY DRAWS — NAMED, WITH `file:line`

Engine root read (⛔ read only, never edited): `C:/Program Files/Epic Games/UE_5.8/Engine/Source/`

| # | Site | `file:line` | What it does |
|---|---|---|---|
| 1 | **THE DRAW** | `Runtime/SlateCore/Private/Widgets/SWidget.cpp:1744-1765` | `#if PLATFORM_UI_NEEDS_FOCUS_OUTLINES` → `if (bCanSupportFocus && SupportsKeyboardFocus())` → `if (FSlateApplicationBase::Get().ShowUserFocus(SharedThis(this)))` → `FSlateDrawElement::MakeBox(..., GetFocusBrush(), ...)` on `AllottedGeometry.ToPaintGeometry()` |
| 2 | **the platform gate** | `Runtime/Core/Public/HAL/Platform.h:539-540` | `PLATFORM_UI_NEEDS_FOCUS_OUTLINES` defaults to **1**; only Android (`AndroidPlatform.h:61`) and iOS (`IOSPlatform.h:50`) zero it ⇒ **compiled IN on Win64, both lanes** |
| 3 | **the brush lookup** | `Runtime/SlateCore/Private/Widgets/SWidget.cpp:1012-1015` | `SWidget::GetFocusBrush()` → `FAppStyle::Get().GetBrush("FocusRectangle")` |
| 4 | **THE BRUSH ITSELF — this is his blue outline** | `Runtime/SlateCore/Private/Styling/StarshipCoreStyle.cpp:217` | `Style->Set("FocusRectangle", new FSlateRoundedBoxBrush(FStyleColors::Transparent, InputFocusRadius, FStyleColors::Primary, InputFocusThickness));` |
| 5 | the brush's numbers | `Runtime/SlateCore/Private/Styling/CoreStyle.cpp:61-62` (decl. `Public/Styling/CoreStyle.h:125-126`) | `InputFocusRadius = 4.f` · **`InputFocusThickness = 1.0f`** |
| 6 | the brush's colour | `Runtime/SlateCore/Private/Styling/StyleColors.cpp:38, 101` | `FStyleColors::Primary` = `EStyleColor::Primary` = **`#0070E0FF`** — **a clear blue** |
| 7 | the show-gate | `Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:4792-4802` | `FSlateApplication::ShowUserFocus(Widget)` — iterates **all** `Users`, true if **any** `User->ShouldShowFocus(Widget)` |
| 8 | the per-user predicate | `Runtime/Slate/Private/Framework/Application/SlateUser.cpp:187-190` | `ShouldShowFocus = HasFocus(Widget).IsSet() && bShowFocus` |
| 9 | where `bShowFocus` is written — **the only writer** | `Runtime/Slate/Private/Framework/Application/SlateUser.cpp:778-785` | `FSlateUser::SetFocusPath(NewFocusPath, InCause, bInShowFocus)` |
| 10 | **who computes it** | `Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:3094-3111` | `ShowFocus = InCause == EFocusCause::Navigation;` then a **reverse walk** of the focus path takes the first `OnQueryShowFocus(InCause)` that is set, and **that answer wins** |
| 11 | **who overrides it for a game viewport** | `Runtime/Slate/Private/Widgets/SViewport.cpp:449-452` → `Runtime/Engine/Private/Slate/SceneViewport.cpp:1246-1258` → `Runtime/Engine/Private/GameViewportClient.cpp:2618-2630` | `UGameViewportClient::QueryShowFocus` — returns `false` under `RenderFocusRule == Never`, or `NonPointer` + Mouse cause, or **`NavigationOnly` + cause ≠ Navigation**; else **`true`** |
| 12 | the rule in force | `Runtime/Engine/Private/UserInterfaceSettings.cpp:16` | default `RenderFocusRule(ERenderFocusRule::NavigationOnly)` — **the project does not override it**: `Config/DefaultEngine.ini:359-367`'s `[/Script/Engine.UserInterfaceSettings]` block sets only `bAllowHighDPIInGameMode=True`, and a sweep of `Config/*.ini` for `RenderFocusRule` returns **zero hits** |

**Reading the chain end to end for our own call:** cause = `Navigation` ⇒ `ShowFocus` starts **true** (:3094) ⇒ the reverse walk reaches `SViewport` (our UMG buttons are descendants of the game viewport widget) ⇒ `UGameViewportClient::QueryShowFocus(Navigation)` under `NavigationOnly` ⇒ **`true`** ⇒ `bShowFocus = true` ⇒ `ShowUserFocus` **true** ⇒ `SWidget::Paint` **draws the ring**.

⇒ **Nothing in the focus-cause chain suppresses our ring. It is asked for, and by the book it is drawn.**

---

## 3. HYPOTHESES — DEAD FIRST, THEN SURVIVORS, EACH WITH ITS REFUTER (`SC-§101`)

### ⛔ H1 — DEAD. *"The runtime-built buttons carry default styling with no focused appearance."*

**REFUTED three independent ways.**
1. 🧑 **Jonathan's photograph** — the ring exists and renders (the evidence `TASK-1399` did not have).
2. **The ring is not a button style at all.** It is `SWidget::Paint`'s platform focus outline (SWidget.cpp:1744-1765) drawn from `FAppStyle::Get().GetBrush("FocusRectangle")` (SWidget.cpp:1012-1015). **No `FButtonStyle` can produce it and none can suppress it.** The style asset is irrelevant to this question.
3. **`SButton` has no focus visual of its own.** `SButton.cpp:271-289`: `SupportsKeyboardFocus()` returns `bIsFocusable`; `OnFocusReceived`/`OnFocusLost` only fire delegates. `SButton::GetBorder()` never consults focus — only hovered/pressed/disabled.

⭐ **Consequence the manager should carry forward: `TASK-1399` §6's warning that "`TASK-1400` can succeed completely and he will still see no ring" is now DEFUSED.** It rested on H1. H1 is dead.

### ⛔ H2 — DEAD. *(the dispatch's own leading candidate)* *"Programmatic focus passes a non-Navigation cause, so `ShowFocus` is false."*

**REFUTED at source.** `SiegeMenuInputSubsystem.cpp:397` and `:403` both pass `EFocusCause::Navigation` **explicitly**. Engine-side, `SlateApplication.cpp:3094` computes `ShowFocus = (InCause == Navigation)` → true, and the viewport override `GameViewportClient.cpp:2618-2630` under the in-force `NavigationOnly` rule also returns true. **The mechanism the dispatch named is real in Slate and already handled in this code.**

### ⛔ H2b — DEAD. *"A repeat `FocusButton` on the already-focused button clears the ring."*

**REFUTED, and it matters in the other direction.** `SlateApplication.cpp:3028-3033` early-returns `false` **before** touching `bShowFocus` when the target is already the focused widget. So a same-widget re-set **cannot** clear the ring. ⚠️ **But record the trap it does create, for `TASK-1400`'s benefit, not for mine to fix:** `FocusButton` returns **`false`** in that case, and its own comment reads that `false` as *"the immediate set was refused this frame"* and queues a **deferred** focus request. **`FocusButton() == false` does not mean failure — it also means "already focused."** `TASK-1400` is boarded to re-fire `ApplyInitialFocus()`; today `ApplyInitialFocus` is guarded by `!GetFocusedMenuButton()` (`:368`) so it never hits this, but a re-fire that drops that guard would.

---

### ✅ H3 — **LIVE, LEADING. The ring is rendered ~3× thinner in our lane than in his, and it is a 1-pixel stroke to begin with.**

This is arithmetic, not a defect.

| | 🧑 his lane | 🤖 our lane |
|---|---|---|
| process | `-game` standalone, PID 39980 | PIE inside the editor |
| render surface | `-ResX=3200 -ResY=1800`, `bAllowHighDPIInGameMode=True` (`DefaultEngine.ini:367`) | **1280×725 viewport** (`TASK-1399` §6) |
| UMG DPI scale (`UIScaleRule=ShortestSide` + `UIScaleCurve`, engine default at `BaseEngine.ini:1474-1475`; **project overrides neither** — zero hits for `UIScale` in `Config/*.ini`) | shortest side **1800** → between keys (1080, 1.0) and (8640, 8.0) → **≈ 1.67** | shortest side **725** → between keys (720, 0.666) and (1080, 1.0) → **≈ 0.67** |
| ring stroke, `InputFocusThickness = 1.0f` × scale | **≈ 1.67 device px of solid `#0070E0`** | **≈ 0.67 device px — SUB-PIXEL**, rendered by the rounded-box shader as a partial-alpha blend |
| then the capture | none — his eye, 1:1, on the physical display | **1086×615 from 1280×725 = a further ×0.848** → **≈ 0.57 px of blue** |
| **net** | crisp blue line | **~2.9× thinner, sub-pixel, then resampled again** |

And the ring is drawn on `AllottedGeometry.ToPaintGeometry()` (SWidget.cpp:1755) — **exactly the button's own outer edge**, on top of the button's existing border pixel. A sub-pixel blue blend landing on an already-dark edge pixel and then downscaled is a **hue shift on one pixel row**, not "an outline."

🚨 **AND THIS IS WHERE `TASK-1399`'s CONTROL FAILS (`SC-§39`).** Its escape-closer was: *"it resolves `Button_3`'s style difference perfectly well, so it is not too coarse."* But `Button_3` renders **"wider and lighter"** — an **area/fill** signal. **A fill and a 1-px stroke are not the same detection problem.** An area signal is invariant under resampling; a sub-pixel stroke is destroyed by it. **The control discriminated a class of signal it was not being used to license.** That is a false zero in exactly `SC-§39`'s shape, and `TASK-1399` could not have known it, because nobody had read `InputFocusThickness`.

**What would refute H3:** re-examine the two promoted `TASK-1399` frames — `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1399-mainmenu-focus-state-{before-injection,after-4-menudown}.png` — with a **per-pixel search for `#0070E0`-family blue along each button's bounding rectangle**, instead of by eye. If **no** blue-shifted edge pixels appear on `Button_0` in the first frame and on `Button_4` in the second, H3 is dead and the ring genuinely was not drawn. If they appear on exactly the focused row in each frame and nowhere else, **H3 is confirmed and the whole divergence is instrument-side.**

### ✅ H4 — **LIVE. The experiment is confounded: three variables moved at once.**

The board frames this as *"real keys vs `inject_input_action`."* It is not. His observation and ours differ in **(a)** input origin, **(b)** process and render mode (`-game` standalone @3200×1800 high-DPI vs PIE @1280×725), **(c)** observation instrument (his retina, 1:1 vs `capture_pie_frame` @0.848×). **Nothing measured so far entitles anyone to attribute the difference to (a)**, and (b)+(c) together already supply a sufficient mechanism (H3).

**What would refute H4:** any **single-variable** swap. Either (i) run the same `inject_input_action` sequence in a `-game` standalone session at his resolution and look at it with a human eye — if the ring appears, input origin is exonerated; or (ii) have 🧑 press real `Down` keys **inside PIE** at 1280×725 and capture with the same tool — if the ring is still invisible there, input origin is exonerated the other way.

### ✅ H5 — **LIVE BUT WEAK. Focus left the button and came back with a non-Navigation cause, clearing `bShowFocus`.**

`bShowFocus` is a **single flag per `FSlateUser`** (SlateUser.cpp:783), written only when the focused widget **actually changes** (the early return at SlateApplication.cpp:3029 blocks same-widget rewrites). So the only way to keep `HasUserFocus == true` while `bShowFocus == false` is a **round trip**: focus → some other widget → back to the button, with the return leg carrying `SetDirectly` / `Mouse` / `WindowActivate`. The PIE lane is far more exposed to this than his (the editor window is repeatedly activated/deactivated by our tooling; a stale mouse hover sits over the viewport — `TASK-1399` §6 H2 already suspected `Button_3`'s constant lighter render is a **Hovered** brush from a stale pointer, which is direct evidence that the mouse IS parked over that column). Round-tripping inside the 16 ms between the structured read and the capture is unlikely, which is why I rank it below H3.

**What would refute H5:** read the focus **cause** at capture time, not the focus **state**. `TASK-1399` recorded `focused: true` and never recorded the cause or `ShowUserFocus`. If a read on the captured frame reports cause `Navigation` and `ShowUserFocus == true`, H5 is dead.

### ✅ H6 — **LIVE, WEAKEST. `capture_pie_frame` does not composite the layer the ring is emitted into.**

The outline is emitted at `NewLayerId` into the **same** `FSlateWindowElementList` as every other widget (SWidget.cpp:1752-1760), so a compositor that captured the buttons should have captured the ring. Kept on the list only because it has not been excluded.

**What would refute H6:** the same per-pixel blue search as H3 — the two hypotheses are separated by *where* the blue is missing (rendered-but-lost vs never-emitted), which needs H5's cause read to split.

### 🟡 H7 — **OPEN, AND NOBODY HAS CHECKED IT: his real keys may not go through this code at all.**

Slate's own `FNavigationConfig` maps arrow keys to `EUINavigation` and performs navigation focus moves **without** Enhanced Input. Two `Down` presses land on index 2 (`"Deck Builder"`) under **either** path — our `MoveFocus` (`:304-332`, and its own comment at `:318-320` says cold Down ×2 lands on index 2) **or** native Slate navigation. **The outcome does not discriminate the path**, so his photograph does not prove `IA_MenuDown` was involved. If Slate navigation handled it, the two lanes share almost no code and "same state, different pixels" was never the right frame.

**What would refute H7:** his log, or one keystroke. `LogSiegeMenuInput` prints on **Accept** only (`:349-351`). Ask 🧑 to press `Down` ×2 then `Enter` in that session: if the log names the button, his keys are on our path and H7 dies.

---

## 4. (3b) HOW THE SEVEN BUTTONS ARE BUILT, AND WHY IT TURNED OUT NOT TO MATTER

`WBP_MainMenu`'s `Event Construct` adds `Button_0..6` to `VerticalBox_0` at runtime (`SiegeMenuInputSubsystem.cpp:210-211`; `TASK-1398` handoff line 167; count and index 2 = `"Deck Builder"` measured by `TASK-1274`). **I could not read their assigned style**, and I did not try to: `CONVENTIONS` clause (d) records `function_graphs`/`ubergraph_pages` as **not exposed to Python** and CDO reflection as **blind to BP graph functions** — two named dead instruments I am forbidden to re-buy, and this row is `Read`/`Grep` only in any case.

⭐ **It does not matter.** Per H1's refutation #2/#3, the focus ring is drawn by `SWidget::Paint` from `FAppStyle`'s `"FocusRectangle"`, entirely outside `FButtonStyle`. **Whatever style `Construct` assigns cannot change whether the ring draws.** The only button-side inputs to the draw are `bCanSupportFocus && SupportsKeyboardFocus()` (SWidget.cpp:1746) — and `GetMenuButtons` already filters on `GetIsFocusable()` (`:215`), and `FocusButton` re-checks `SupportsKeyboardFocus()` (`:384`), and focus demonstrably landed. Both are satisfied.

---

## 5. (5) THE CHEAPEST DISCRIMINATOR — PROPOSED, ⛔ NOT IMPLEMENTED

**It is not a code change, and it costs nothing.** Re-open the two PNGs already on disk at `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1399-mainmenu-focus-state-{before-injection,after-4-menudown}.png` and run a **per-pixel scan for `#0070E0`-family blue confined to each button's bounding rectangle**, comparing the focused row against the six unfocused rows in the same frame (a built-in control: the other six must be clean). Blue on `Button_0` in frame 1 and on `Button_4` in frame 2, and nowhere else ⇒ **H3 confirmed, H5/H6 dead, the ring always drew and the instrument could not see it** ⇒ the remedy is a **capture** change, not a code change. No blue on either ⇒ H3 dead, and the question becomes H5 vs H6. **Cost: zero engine, zero PIE, zero code, zero editor — one script over two files already committed to the repo.**

**What it cannot see:** (i) it cannot separate H5 from H6 if the answer is "no blue" — that needs a second step; (ii) it is retrospective and inherits whatever the capture tool already did to those frames, so a **negative** result is only as good as the PNGs' fidelity; (iii) it says nothing about H7.

**Only if that returns "no blue"** is a code discriminator worth buying, and then it is **one line** inside the file fence: log `FSlateApplication::Get().ShowUserFocus(SlateButton)` **and** the focus cause on the frame *after* the set (not at the call, which is when the current comment reasons about it). That boolean splits *"the ring was never asked for"* (H5) from *"the ring drew and the camera missed it"* (H3/H6). It cannot prove the pixels reached the capture — it only splits render-side from capture-side.

⛔ **Per `SC-§101` both of these are claims, not prescriptions. The manager boards them or does not.**

---

## 6. (6) THE SECOND-ORDER QUESTION — **β**, ARGUED

**Does 🧑 his complaint need this fixed? No. His lane already draws the ring, and I can now say *why* rather than merely *that*:** the cause is right (`Navigation`, explicit), the rule is right (`NavigationOnly`, engine default, unmodified by this project), the platform gate is compiled in on Win64, the brush exists and is blue, and at his DPI it is ~1.67 device pixels of `#0070E0` viewed 1:1. **Every link in the chain checks out for him.** There is no α on the evidence I have.

**The lane that cannot see it is ours, and the cost of that is concrete and already boarded.** `CONVENTIONS` clause (g)(ii) / `TASKBOARD:6889` makes the verifier's pixel limb **blind by construction** and forbids a ringless capture from being written as `VERIFY-FAILED`. That blindness currently sits on **six** boarded verification rows — `TASK-1402` · `1413` · `1421` · `1427` · `1436` · `1449` — every one of which carries a pixel limb that today can only return `UNOBSERVABLE`. **β buys back a machine-checkable pixel limb on all six.** That is not "no fix needed"; it is the most leveraged thing on this page.

🚨 **AND THE RIDER THE MANAGER NEEDS BEFORE ACTIVATING `TASK-1447`, WHICH IS THE PART I WOULD MOST LIKE READ:**

1. **If H3 is right, the β remedy is an INSTRUMENT change — capture at native viewport resolution and/or pixel-search for `#0070E0` — which lives ENTIRELY OUTSIDE `SiegeMenuInputSubsystem.cpp`.** `TASK-1447` (2) fences that row to exactly one file and says an outside-the-fence mechanism is an **ESCALATE, not a diff**. On my leading hypothesis, **`TASK-1447` as fenced is the wrong vehicle.**
2. **This row hands `TASK-1447` FIVE surviving candidates, not one.** Its own clause (1) says: *"If its handoff names two surviving candidates, implement NEITHER and escalate — a coin flip in a diff is a guess wearing a fix's clothes."* **By `TASK-1447`'s own rule it must not run as a fix row yet.** It should be preceded by §5's zero-cost discriminator, which needs no editor, no PIE and no code, and which can collapse five candidates to one or two in a single pass.

⇒ **My recommendation, which is a claim and not a decision: board the §5 discriminator (zero-cost, zero-risk, parallel-safe vs everything) and hold `TASK-1447` at `backlog` until it returns. Letter: β.**

---

## 7. FILES TOUCHED

- **Written:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1446-programmer.md` (this file)
- **Edited:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\TASKBOARD.md` — **this row's `status:` line only**
- **Read, never edited:** `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.{cpp,h}` · `Config/DefaultEngine.ini` · `C:/Program Files/Epic Games/UE_5.8/Engine/Source/**` (9 files) · `C:/Program Files/Epic Games/UE_5.8/Engine/Config/BaseEngine.ini` · `qa/TASK-1399-verify.md` · `handoffs/TASK-1398-programmer.md`
- ⛔ **No code edit. No engine-source edit. No asset. No compile. No PIE. No MCP. No git. No `CONVENTIONS.md`. No other row's line. No dispatch.**

---

## Not examined / limitations

1. **The DPI-scale arithmetic in H3 is INFERRED, NOT MEASURED — and it is the load-bearing number of my leading hypothesis.** Two links are unverified: (a) that the UMG DPI scale multiplies `FSlateRoundedBoxBrush`'s `OutlineWidth` (I infer it from the scaler being a layout render scale in the widget path; I did not trace the rounded-box shader's thickness term to a geometry scale); (b) the curve's interpolation mode between keys — I assumed linear, so **0.67 and 1.67 are approximations**, and if the keys are `CIM_CurveAuto` they will differ somewhat. The *direction* and *order of magnitude* (his lane ~2–3× thicker) survive both uncertainties; the exact figures do not.
2. **The PIE viewport's effective Slate application scale was not measured.** I used the UMG DPI curve; a PIE window can also carry an editor-side application scale I did not account for.
3. **I did not open the two PNGs.** §5's per-pixel scan is *proposed*, not run — running it would have been image analysis, which is inside `Read` but outside this row's four named deliverables, and its result belongs to whichever row the manager boards.
4. **`WBP_MainMenu`'s `Construct` graph is unread** and stays unread — the two instruments that could read it are recorded dead in `CONVENTIONS` (d), and §4 argues it is irrelevant to this question. If some future finding makes the button style relevant again, **this limitation becomes load-bearing.**
5. **Whether the UMG widget tree is truly a descendant of `SViewport`** (the premise that makes `UGameViewportClient::QueryShowFocus` the deciding override at SlateApplication.cpp:3100-3108) is asserted from the standard `SGameLayerManager` arrangement, **not traced in this project**. If `WBP_MainMenu` is added outside the viewport widget, that override never fires — and the outcome would be the *same* (`ShowFocus` stays `true` from the cause), so **this does not change any conclusion**; recording it so nobody re-derives it as new.
6. **H7 is untested and I had no way to test it** — it needs either 🧑's log from PID 39980 or one keystroke from him. **His photograph does not establish that his keys went through `IA_MenuDown`.**
7. **`FAppStyle`'s resolution of `"FocusRectangle"` may differ between the editor process and a cooked/standalone process.** I confirmed the Starship CoreStyle definition and that the editor style set defines no competing `"FocusRectangle"` (a sweep of `Engine/Source/Editor/**` returns only `SCurveEditor`'s unrelated `OnQueryShowFocus`), so a parented editor style falls through to the same blue brush. **I did not verify the parent-style fall-through at runtime.** If the two processes resolve different brushes, that is a **sixth** candidate and it has not been excluded.
8. **No runtime observation of any kind was made by this row.** Every statement above is a reading of source text and committed reports. `SC-§39` cuts both ways: **a mechanism derived by reading is a hypothesis, not a measurement.**
