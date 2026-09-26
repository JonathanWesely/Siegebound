# TASK-1409 — [MENU-NAV-LRB-BINDINGS] — gameplay-programmer handoff

marker: `TASK-1409-MENU-NAV-LRB-BINDINGS`
status on completion: `ready-for-qa` → `TASK-1410`
law: `VER-§8` cl. 2/3(b) · `SC-§39` · `SC-§50` · `SC-§101` · `CITE-BY-TEXT-RULED-2026-09-24`

**Ran:** `Read` / `Grep` / `Glob` / `Edit` / `Write` + read-only shell (`sed`, `ls`, `grep`,
one `python` brace-balance script in the scratchpad). ⛔ No compile, no PIE, no MCP, no asset
write, no git, no editor-lifecycle action. **The running editor (PID 32984) was not touched at all
— not even read.**
**Wrote:** `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp` · `.h` · this file ·
`TASKBOARD.md` — **this row's `status:` line only**.

**🔁 Loop-1 pass (2026-09-24), same grant, same fences.** Additionally **read** (never written) on
this machine's UE 5.8 tree: `Runtime/UMG/Public/Components/Slider.h` ·
`Runtime/UMG/Private/Components/Slider.cpp` · `Runtime/Slate/Private/Widgets/Input/SSlider.cpp` ·
`Runtime/Core/Public/UObject/ScriptDelegates.h`; and in-project `SiegeGraphicsMenuWidget.cpp`
(the bind pass and the three committed handlers). ⛔ Still no compile, no PIE, no MCP, no asset, no
git, no editor-lifecycle action. ⛔ `SettingsMenuWidget.{cpp,h}` (`TASK-1415`, `qa-passed`) were
**not** opened.

---

# 🔁 QA LOOP 1 — `qa/TASK-1410.md` FAIL, ANSWERED. READ THIS SECTION BEFORE THE ORIGINAL NOTE BELOW.

Everything from `## 🚨 §0` down is the **original** first-cut note. It is **left intact** except
where a statement became false; every such place is struck and corrected **in place** and is also
listed here. ⛔ Nothing QA passed was re-done.

## ⛔ BLOCKER-1 — DISCHARGED VIA **(a) FIRE THE COMMIT EDGE**, AND HERE IS THE ARGUMENT

**QA's measurement is correct and I do not dispute one word of it.** I re-measured the whole chain
at engine source myself rather than taking it:

| the thing | where I read it | what it says |
|---|---|---|
| `USlider::SetValue` reaches only the value-changed edge | `Slider.cpp`, `void USlider::SetValue(float InValue)` — `if (MySlider.IsValid()) { MySlider->SetValue(InValue); } if (Value != InValue) { Value = InValue; HandleOnValueChanged(InValue); }` | **confirmed.** No capture-end handler is on that path. |
| the capture-end pair is public and assignable | `Slider.h` — `UPROPERTY(BlueprintAssignable, Category="Widget Event") FOnMouseCaptureEndEvent OnMouseCaptureEnd;` and `FOnControllerCaptureEndEvent OnControllerCaptureEnd;`, both under a `public:` in the "Widget Event" block, both `DECLARE_DYNAMIC_MULTICAST_DELEGATE` (**no parameters**) | **confirmed — in fence, no engine edit, no reinterpretation.** |
| what I broadcast is **byte-for-byte what the engine's own handler broadcasts** | `Slider.cpp` — `void USlider::HandleOnControllerCaptureEnd() { OnControllerCaptureEnd.Broadcast(); }` | ⭐ the whole body is that one line. My call is **identical**, which is the same relationship `OnClicked.Broadcast()` has with `UButton::SlateHandleClicked`. |
| Slate's own origin, so I can say honestly what I am standing in for | `SSlider.cpp` — `ResetControllerState()` is `if (bControllerInputCaptured) { OnControllerCaptureEnd.ExecuteIfBound(); bControllerInputCaptured = false; }`, reached from `OnKeyDown` (lock release), `OnFocusLost`, `OnMouseCaptureLost` and the nav paths | **confirmed unreachable from a programmatic set** — `bControllerInputCaptured` is only ever set by a real Slate interaction, so nothing else will fire this edge for us and there is **no double-commit hazard.** |

**Why (a) and not (b).** (b) ships a control that is knowingly inert and buys a log line plus a
rewritten criterion; (a) makes Right actually change the setting, which is what the row is for. The
three reasons I weighed:

1. ⭐ **(a) is not a new semantic — it is the doctrine already in this diff, applied to the delegate
   this project actually writes on.** I fire the delegate the human input path fires; I invent
   nothing. The reason I got it wrong the first time is worth recording: *"the engine already
   broadcasts, so I must not"* is **true of `OnValueChanged` and irrelevant to the commit edge**,
   and I never asked whether `OnValueChanged` was the signal that WRITES. **It is not — on this
   project's sliders it is explicitly forbidden to write.**
2. **It is safe where nothing is bound.** A dynamic multicast with no bindings broadcasts to nobody,
   and I do not rely on that alone — I ask (`IsBound()`) and log the "no commit edge exists" case as
   a named outcome instead of a silent guess.
3. **It cannot double-write.** The graphics screen's `Handle*SliderCommitted` re-reads the sliders
   and **diffs against the facade** before writing (`HandleGroupSliderCommitted` snapshots only the
   groups whose `SliderValueToLevel` disagrees with `GetQualityGroupLevel`), and the facade refuses
   a no-op of its own accord.

**What landed, in `StepSlider` only:**

```cpp
	if (ReadBack == OldValue) { /* logged; ⛔ no commit edge on a no-op */ return; }

	const bool bControllerEdgeBound = Slider->OnControllerCaptureEnd.IsBound();
	const bool bMouseEdgeBound      = Slider->OnMouseCaptureEnd.IsBound();
	if (!bControllerEdgeBound && !bMouseEdgeBound) { /* logged; one-phase or unbound */ return; }

	const TCHAR* const EdgeName = bControllerEdgeBound ? TEXT("OnControllerCaptureEnd") : TEXT("OnMouseCaptureEnd");
	UE_LOG(... "-> %s.Broadcast() (the commit edge the owning screen writes on ...)" ...);   // ⛔ BEFORE the broadcast
	if (bControllerEdgeBound) { Slider->OnControllerCaptureEnd.Broadcast(); }
	else                      { Slider->OnMouseCaptureEnd.Broadcast(); }
	UE_LOG(... "%s returned ...; value after the commit is %.4f (was %.4f before it)." ...);
```

Five decisions inside that, each with its reason, **all declared rather than assumed**:

- **ONE edge, not both.** Every slider in the project binds the **same** `…Committed` handler to
  *both* capture-end delegates (`SeedAndBind`), so firing both would run the facade write twice.
  `OnControllerCaptureEnd` is the semantically honest edge for a keyboard/gamepad step.
- **`OnMouseCaptureEnd` as a fallback, only when it is the sole bound one.** Not speculation about
  a future screen so much as refusing to make "the controller edge is the one" a silent assumption;
  the log line always names which edge fired.
- **Gated on a real change, compared EXACTLY** (`ReadBack == OldValue`, no epsilon). This is the
  same predicate the engine used to decide whether `OnValueChanged` fired (`if (Value != InValue)`),
  so the change edge and the commit edge stay in one-to-one correspondence. CONVENTIONS "Delegates
  (C++)" — never broadcast on a refused or no-op mutation — same rule `SetCheckBoxChecked` follows.
- **Logged BEFORE the broadcast**, because the handler on the other end runs the facade write, the
  apply and the ini save; if it ever asserts, the line printed first is the only evidence the commit
  was attempted. A second line after it reports the value **after** the commit, which is not
  redundant: a committed write re-seeds the panel and can legitimately move the handle (the overall
  preset slider also rewrites the resolution-scale row), and a verifier must be able to tell *"the
  refresh corrected it"* from *"nothing happened"*.
- ⚠️ **Write cadence, declared, because cl. (3a) is the reason the two-phase design exists.** The
  three actions bind **`ETriggerEvent::Started`**, so a held key is ONE press → ONE step → ONE
  commit. Cl. (3a) forbids a write per *frame of one continuous gesture* (a drag: up to fifty ini
  writes); one write per discrete key press is the same cadence as one mouse drag's capture end.
  ⛔ I did **not** add a debounce timer: it would leave the setting uncommitted for N ms, which is
  the very failure being fixed, just time-shifted, and unobservable to a verifier reading right
  after the press.

⛔ **I took (b)'s instrument half as well** — every branch of the commit path names itself, so
"stepped but not committed" can never again be a silence.

## ⭐ WARN-5 — TAKEN (it was cheap, and it is `TASK-1454`'s single most useful bit)

`HandleMenuBack` now carries the target's **provenance** on both lines that name a target:
`… active target '%s' (%s, %s) …` where the third field is `"from the registration stack"` or
`"the DEFAULT FALLBACK — nothing is registered, so this is the main menu and NOT necessarily the
screen the player is looking at"`. ⛔ Derived by **comparing** `Target` against
`GetRegisteredNavTarget()` rather than re-deriving the target locally, so `GetActiveNavTarget()`
stays the single definition and this cannot drift from it. It is also exactly the signal WARN-1's
latent hazard (Back closing the screen *underneath* an unregistered one) would show up on.

## 📄 WARN-4 — THE TWO BAD CITATIONS, CORRECTED AND CONVERTED TO TEXT

Both were wrong **numbers** attached to **correct text**, and QA is right that nobody should
propagate them. They are struck in §5 below, re-stated by text, and ⚠️ **both have since moved
again** (this pass added ~+100 lines above them), which is the whole argument for
`CITE-BY-TEXT-RULED-2026-09-24`.

## ⛔ WHAT I DID **NOT** TOUCH

`SettingsMenuWidget.{cpp,h}` (`TASK-1415`, `qa-passed`) · `Tests/SiegeMenuInputTest.cpp` ·
`Tests/SiegeGraphicsMenuTest.cpp` (it asserts the graphics slider **binding topology** — my change
binds nothing and unbinds nothing, so every one of its `Contains(...)` assertions is unaffected) ·
`SiegeGraphicsMenuWidget.{cpp,h}` (read only) · `CONVENTIONS.md` · any other row · `TASK-1413`'s
criterion. No compile, no PIE, no MCP, no asset, no git, no editor-lifecycle action.

## 🙋 ROUTING REQUESTS (⛔ not edits I am entitled to make — `SC-§50`)

1. **`TASK-1413`'s runtime criterion must read the FACADE, not the slider node** — QA's WARN-3(c),
   and it stays true *after* this fix: a node read passes on a control that changes nothing, so it
   cannot discriminate this defect from its absence. Suggested: after `inject_input_action
   IA_MenuRight`, assert `USiegeGraphicsSettingsSubsystem::GetQualityGroupLevel(<group>)` (or
   `GetResolutionScalePercent`) **moved**, and keep the node read as corroboration. The log line
   `"… -> OnControllerCaptureEnd.Broadcast() …"` is the cheap discriminator in the meantime.
2. **Both halves of the reachability gap** (QA §5.1): a *registered target* is now live via
   `TASK-1415` (`SettingsMenuWidget.cpp` calls `RegisterMenuNavTarget(this)`); a *focused `USlider`*
   still is not — Settings holds none, and the only sliders are on Graphics, which waits on
   `TASK-1417`.
3. **WARN-1 → `TASK-1454`**: every screen that can open on top of a registered screen must itself
   register, or Back drives the tree underneath it.
4. **WARN-2 → the walker**: collapse a recognised stepper pair to one focus stop (`FindStepperPair`
   is already written and can be reused).

---

## 🚨 §0. WHAT A USER ACTUALLY GETS ON THE DAY THIS LANDS — READ THIS FIRST

| key | on the main menu | on Settings / Graphics / any sub-screen |
|---|---|---|
| `Left` / `Right` / D-pad L/R | **nothing** (seven plain `UButton`s — the deliberate, Verbose-logged nothing) | **nothing yet** — the handlers run, but the walker is pointed at the main menu because **no screen registers** |
| `Backspace` / `Gamepad_FaceButton_Right` | **nothing** (`WBP_MainMenu` implements no close handler) | 🚨 **NOTHING. `IA_MenuBack` IS INERT ON EVERY SCREEN IN THE GAME, INCLUDING THE DECK BUILDER.** |

⛔ **"Back is implemented" ≠ "Back works".** What lands is the **mechanism and the whole of its
diagnostic surface**: the action is bound, the handler runs, and every press prints a line naming
the screen it reached and why nothing happened. The wiring — nine screens calling
`RegisterMenuNavTarget` and implementing the close handler — is **`TASK-1454`** and is **not mine**
(fence (5) is `SiegeMenuInputSubsystem.{cpp,h}` **only**; I widened it for nothing).

⇒ The line a verifier or 🧑 Jonathan will see today, once per press:

```
[USiegeMenuInputSubsystem] IA_MenuBack -> HandleMenuBack() entered.
[USiegeMenuInputSubsystem] IA_MenuBack: active target 'WBP_MainMenu_C_0' (WBP_MainMenu_C, the DEFAULT
FALLBACK — nothing is registered, so this is the main menu and NOT necessarily the screen the player
is looking at) implements no ISiegeMenuNavCloseTarget — Back is INERT for it, and no teardown is guessed.
```

⭐ **(loop 1, WARN-5)** the third field above is new: it says whether the target came from the
**registration stack** or the **default fallback**, which is the one bit `TASK-1454` most needs.

**That pair of lines is the deliverable's proof of life.** They discriminate *"Back did nothing"*
from *"Back never arrived"*, which is the exact confusion `TASK-1454` cannot resolve without them.

⚠️ **Same caveat for Left/Right**: the per-type semantics below are **live and complete**, but
until a screen registers, `GetActiveNavTarget()` is the main menu, whose stops are all plain
buttons. **The first screen that registers (`TASK-1415`, Settings) is where a slider first moves.**
The runtime criterion (`inject_input_action IA_MenuRight` moves a focused `USlider`) is therefore
**gated on `TASK-1415` landing**, not on this row — flagged for `TASK-1413`, not self-adjudicated.

---

## §1. `SC-§71b`-STYLE DECLARATION — I HAVE NO `git`, AND HERE IS THE SUBSTITUTE

My `names:` grant is `Read` / `Grep` / `Edit` only, and the fence says ⛔ no git. **I did not run
`git diff` and I am not implying I did.** The before/after quoted below is taken from my **own full
`Read` of both files at the start of this row**, before any `Edit` — i.e. the pre-edit text is
quoted from the file as it stood at `SiegeMenuInputSubsystem.cpp` mtime `2026-09-24 20:43`
(`TASK-1406`'s landed state, `qa/TASK-1407.md` PASS, `built`). Every `Edit` call used a unique
`old_string`; none used `replace_all`.

**Substitute integrity proof** — the four protected artifacts re-grepped **out of the working copy
after the last edit** (§5).

**Blast radius, measured by mtime rather than by `git status`.** The only files under `Source/` or
`Content/` modified in the hour this row ran are my two **plus**
`Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.{cpp,h}`.
🚨 **Those two are NOT mine — they are `TASK-1415`, which the dispatch said is being written in
parallel right now and will be the first caller of `RegisterMenuNavTarget`.** ⛔ I never opened
either for writing. Recording it here so that whoever reads the eventual diff does not attribute
them to this row. **No asset under `Content/` was touched by anyone in that window.**

---

## §2. (1) THE SIX BINDINGS — WHICH ACTION, WHICH HANDLER, WHICH KEYS

All three are bound **beside the existing three, same idiom, same `ETriggerEvent::Started`**.

| `UInputAction` (asset) | keys (authored by `TASK-1408` in `IMC_MainMenu`, rows 6–11) | handler |
|---|---|---|
| `/Game/Input/Actions/IA_MenuLeft` | `Left` · `Gamepad_DPad_Left` | `HandleMenuLeft()` → `StepFocusedStop(-1)` |
| `/Game/Input/Actions/IA_MenuRight` | `Right` · `Gamepad_DPad_Right` | `HandleMenuRight()` → `StepFocusedStop(+1)` |
| `/Game/Input/Actions/IA_MenuBack` | `Backspace` · `Gamepad_FaceButton_Right` | `HandleMenuBack()` |

⛔ **`Escape` is NOT bound** (🧑 his narrow ruling — card grid only — and it is the editor's
PIE-stop key). `grep -n Escape` over both files returns **exactly one hit**: `.h:258`, the comment
that records the ruling. **No `FKey`, no mapping, no binding, no handler.**

The object paths are the literals `TASK-1408`'s handoff §(1) published ("⛔ These are the literals
`TASK-1409` references. Character-for-character"), and all six `.uasset` files are on disk.

### The arm line was NOT widened — a second line was added instead

`IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).` is quoted as a discriminator in
**nine** pipeline files (measured: `qa/TASK-1274-verify.md`, `qa/TASK-1270-verify.md`,
`qa/TASK-1280-report.md`, `qa/TASK-1389-report.md`, `qa/TASK-1393-verify.md`,
`qa/TASK-1395-verify.md`, `qa/TASK-1399-verify.md`, `handoffs/TASK-1281-buildmaster.md`,
`TASKBOARD.md`). It is **byte-identical**; the new bindings report themselves on their own line:

```cpp
UE_LOG(LogSiegeMenuInput, Log,
    TEXT("[USiegeMenuInputSubsystem] %s: IA_MenuLeft / IA_MenuRight / IA_MenuBack bound (Started): %s / %s / %s."),
    MenuMapName,
    MenuLeftAction  ? TEXT("bound") : TEXT("ABSENT — key inert"),
    MenuRightAction ? TEXT("bound") : TEXT("ABSENT — key inert"),
    MenuBackAction  ? TEXT("bound") : TEXT("ABSENT — key inert"));
```

### The degrade-open branch, quoted verbatim

The three loads sit **below** the existing fatal `return`, so a missing new asset can never take
the shipped three down with it. `bArmed` is set **above** the three new binds, deliberately.

```cpp
	// ─── TASK-1409 (1): THE THREE NEW ACTIONS, LOADED **OUTSIDE** THE FATAL BLOCK ──────────────
	// ⛔ DEGRADE OPEN, and the placement is the whole of the mechanism: these three loads sit
	// BELOW the `return` above, so a missing IA_MenuLeft can never take IA_MenuUp / IA_MenuDown /
	// IA_MenuAccept down with it. Each key is bound only if ITS asset resolved; the others arm
	// regardless, and the absence is named once rather than inferred from a dead key.
	MenuLeftAction  = LoadObject<UInputAction>(nullptr, MenuLeftActionPath);
	MenuRightAction = LoadObject<UInputAction>(nullptr, MenuRightActionPath);
	MenuBackAction  = LoadObject<UInputAction>(nullptr, MenuBackActionPath);
	if (!MenuLeftAction || !MenuRightAction || !MenuBackAction)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: navigation-extension input assets missing (IA_MenuLeft %s, IA_MenuRight %s, IA_MenuBack %s) — THOSE keys are inert; IA_MenuUp / IA_MenuDown / IA_MenuAccept stay armed."),
			MenuMapName,
			MenuLeftAction  ? TEXT("ok") : TEXT("MISSING"),
			MenuRightAction ? TEXT("ok") : TEXT("MISSING"),
			MenuBackAction  ? TEXT("ok") : TEXT("MISSING"));
	}
```

```cpp
	if (MenuLeftAction)
	{
		InputComponent->BindAction(MenuLeftAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuLeft);
	}
	// … MenuRightAction, MenuBackAction — the same one `if` each
```

⇒ **ONE warning naming the missing asset(s), never a crash, never a failed arm for the other
five.** Nothing else in the class dereferences the three pointers.

---

## §3. (2) THE SEMANTICS TABLE — ONE ROW PER FOCUS-STOP TYPE

Dispatch lives in `StepFocusedStop(int32 Direction)`; `Direction` is −1 (Left) / +1 (Right).

| focused stop | Left | Right | Accept | step / mechanism, and the argument |
|---|---|---|---|---|
| `USlider` | value − step | value + step | *(no semantics — one log line)* | **the control's own `GetStepSize()`**, clamped to `[GetMinValue(), GetMaxValue()]`. On the shipped screens that is **0.25 on the eleven 0..1 quality sliders** (`1/MaxQualityLevel`, `MaxQualityLevel = 4`) ⇒ **exactly one detent per press, five detents**; and **1.0 on the 50..100 resolution-scale slider** ⇒ **one percent per press**. ⛔ Nothing invented — `BuildSliderRow` already passes both. |
| `USlider` with `StepSize <= 0` | ← | → | — | fallback `DefaultSliderStepFraction = 0.05f` **of that slider's range**. A *fraction* is the only value correct for both a 0..1 and a 50..100 slider (an absolute step would be 20 % of one and 2 % of the other). 1/20 ⇒ full range in 20 presses. ⛔ Not the engine's own 0.01 default, which needs 100. **Unreachable on today's screens** — every shipped slider declares a real step. |
| `UCheckBox` | **set OFF** | **set ON** | **toggle** | ⛔ **set, never toggle** on the direction keys: a direction key that inverts state means holding `Right` flickers a setting, and a player who cannot see the box still knows which key produces which state. |
| stepper `UButton` **pair** | `OnClicked.Broadcast()` on the `<` member | …on the `>` member | (unchanged — fires the focused button) | the pair `USiegeGraphicsMenuWidget::BuildStepperRow` builds. **Two discriminators, both required to be unambiguous** — §3.1. |
| plain `UButton` | **nothing, deliberately** | **nothing, deliberately** | (unchanged) | logged at **Verbose** (not Log) because it is the common case on the main menu — seven plain buttons — and at Log it would drown the lines that matter. ⚠️ needs `Log LogSiegeMenuInput Verbose`. |
| `UEditableTextBox` / anything else | **one log line** | **one log line** | **one log line** | Left/Right in a text box means *move the caret*, which is Slate's own text handling and not something this subsystem can or should simulate. It is named, not swallowed. |

### §3.1 How a stepper pair is recognised — and why it cannot misfire on `Back` / `Apply`

`FindStepperPair()` requires **all** of:

1. the focused button's **immediate parent** `UPanelWidget` holds **exactly two** `UButton`
   children (the graphics row is `HorizontalBox[LabelText, PrevButton, ValueText, NextButton]`);
2. **and** one of two independent authored discriminators, each taken from the code that builds
   the only steppers this project has:
   - **names** — `<Base>PrevButton` / `<Base>NextButton` with **equal bases**
     (`BuildStepperRow` constructs `FName(*(Base + TEXT("PrevButton")))` and `…NextButton`);
   - **glyphs** — the single-`UTextBlock` contents `"<"` / `">"` (`BuildStepButton(..., TEXT("<"), ...)`),
     read through the existing `GetButtonLabel()` and trimmed.

🚨 **The count guard is a safety property, not tidiness.** Left/Right *press* a button, and a press
is irreversible from here. A structure-only rule ("the other button in my row") would make an arrow
key fire `Apply` or `Back` in any footer that happens to hold two buttons. A panel must look like a
stepper in **both shape and authoring** before an arrow key can fire anything in it.

Both discriminators exist because either can legitimately go absent: that widget's "condition (b)"
says an **asset-authored row wins whole and is never rebuilt**, so a future `WBP_GraphicsMenu`
could keep the pinned *names* while an artist restyles the glyphs, or vice versa. Names are tried
first (they are the pinned, greppable contract); the slot order is **not** trusted — if an author
reverses it, the names/glyphs win.

### §3.2 🚨 THE BROADCAST ASYMMETRY — MEASURED IN THE ENGINE, BOTH WAYS, AND IT IS THE HIGHEST-RISK THING IN THIS DIFF

🚨 **LOOP 1 CORRECTION — THIS TABLE WAS RIGHT ABOUT THE ENGINE AND INCOMPLETE ABOUT THE PROJECT.**
The `USlider` row below is true of **`OnValueChanged`** and says nothing about the delegate this
project actually **writes** on. Read the loop-1 section at the top: `OnValueChanged` is bound to a
**label-only** handler here, the facade write hangs off `OnMouseCaptureEnd`/`OnControllerCaptureEnd`,
and `StepSlider` now fires that **commit edge** on a real change. The row is corrected in place.

| control | does its setter broadcast? | so this row… |
|---|---|---|
| `USlider::SetValue` | **YES for `OnValueChanged`** — it calls `HandleOnValueChanged(InValue)` on a real change, and that function's body is `Value = InValue; OnValueChanged.Broadcast(InValue); BroadcastFieldValueChanged(...)`. **NO for the capture-end pair** — those are raised only from Slate (`SSlider::OnMouseCaptureLost` / `ResetControllerState`). | ⛔ **does NOT re-broadcast `OnValueChanged`** (a second broadcast = every bound handler twice = the LABEL refreshed twice per press) ✅ **and DOES broadcast the commit edge `OnControllerCaptureEnd`** on a real change — without it the handle and the label move and the setting never changes. ⭐ loop 1. |
| `UCheckBox::SetIsChecked` | **NO** — it sets `CheckedState`, pushes to the Slate widget, and fires only `BroadcastFieldValueChanged` / `BroadcastEnumPostStateChange`. `OnCheckStateChanged` is never touched. | ✅ **MUST broadcast** — `OnCheckStateChanged.Broadcast(bNewChecked)`. Without it the box visibly flips and the setting behind it is never applied: implemented-looking and useless. |

⭐ **This project measured the same asymmetry independently**, in
`USiegeGraphicsMenuWidget::SeedAndBind()`: *"MEASURED: USlider::SetValue(float) CALLS
HandleOnValueChanged(InValue) and therefore BROADCASTS OnValueChanged … unlike
UCheckBox::SetIsChecked, which does not."* Two independent reads, same conclusion.

What the check-box path fires is **exactly what a mouse click fires**:
`UCheckBox::SlateOnCheckStateChangedCallback` ends in `OnCheckStateChanged.Broadcast(...)` — the
same relationship `UButton::SlateHandleClicked` has with the Accept path.

⛔ **Never on a no-op** (CONVENTIONS "Delegates (C++)"): the state is compared **as an
`ECheckBoxState`, not as a bool**, so `Undetermined` is a real change that a direction key
resolves, and an already-on box logs *"no change, no broadcast"* rather than re-firing.

---

## §4. (3) BACK ASKS THE TARGET TO CLOSE ITSELF — THE SHAPE, AND THE GRAPHICS CASE

The row offered "a delegate **or** a `BlueprintNativeEvent`". **I built the `BlueprintNativeEvent`**,
on a new interface declared in the fenced header:

```cpp
UINTERFACE(MinimalAPI, Blueprintable)
class USiegeMenuNavCloseTarget : public UInterface
{
	GENERATED_BODY()
};

class ISiegeMenuNavCloseTarget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Siegebound|Menu Input")
	void OnMenuNavBackRequested();
};
```

…and the whole of what Back does with it:

```cpp
	// ⭐ loop 1 (WARN-5): provenance, derived by COMPARISON so GetActiveNavTarget() stays the
	// single definition of "the active target".
	const TCHAR* const TargetOrigin = (Target == GetRegisteredNavTarget())
		? TEXT("from the registration stack")
		: TEXT("the DEFAULT FALLBACK — nothing is registered, so this is the main menu and NOT necessarily the screen the player is looking at");

	if (!Target->Implements<USiegeMenuNavCloseTarget>())
	{
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] IA_MenuBack: active target '%s' (%s, %s) implements no ISiegeMenuNavCloseTarget — Back is INERT for it, and no teardown is guessed."),
			*Target->GetName(), *Target->GetClass()->GetName(), TargetOrigin);
		return;
	}
	…
	ISiegeMenuNavCloseTarget::Execute_OnMenuNavBackRequested(Target);
```

⛔ **`RemoveFromParent` has ZERO CALL SITES in this class** — before this row and after it.

⚠️ **Stated precisely, because the loose version of this sentence is false.** `grep -n
RemoveFromParent` over both files returns **8 hits, all of them inside comments**: five
pre-existing (`.cpp:213`, `:221`, `:387`, `:656`, `.h:303` — TASK-1399/1400/1406 prose about what
*other* code does) and three written by this row (`.h:33`, `.h:40`, `.cpp:1196`, all of the form
"this subsystem must not do it"). **Not one is a statement.** A reviewer running the bare grep will
see 8 and should not read that as a contradiction.

### The graphics-teardown case, named — it is why a generic remove is wrong

`USiegeGraphicsMenuWidget::BackPressed()` runs, **in this order**:

```cpp
	DisarmVideoModeCountdown();
	DiscardStagedVideoMode(ResolveGraphicsSubsystem());
	UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] Back pressed — dismissing the graphics panel only."));
	RemoveFromParent();
```

Its own comment: a staged-but-unconfirmed video mode that outlives the panel *"makes the facade
refuse EVERY SAVE **and** AUTO-DETECT for the rest of the session, silently."* A subsystem that just
removed the widget would skip both calls and leak exactly that. ⇒ **the screen keeps its own
teardown**; its `_Implementation` will be one line (`BackPressed();`).

### Why an interface and not a delegate on the registration API

A delegate has to be **stored**, and the only place is `TASK-1406`'s `NavTargetStack` — which would
have to become a `TArray<FSiegeMenuNavTargetEntry>` (a new `USTRUCT` holding a script-delegate
`UPROPERTY`), rewriting that row's weak-pointer contract and all five of its lambdas to carry a
pointer-to-function. **The interface adds no member and touches no line of that stack**, and reaches
Blueprints the same way (`WBP_DeckBuilder` implements it from Class Settings → Interfaces), which
was the whole reason the registration API is `BlueprintCallable`. The engine's own
`UINTERFACE(MinimalAPI, Blueprintable)` + `UFUNCTION(BlueprintNativeEvent)` shape
(`SequencerAnimationOverride.h`) is copied exactly.

### (4) — and one design decision QA should weigh

⛔ **`HandleMenuBack` is deliberately NOT gated on `IsNavTargetActionable()`**, and it is the one
place the three handlers differ. That gate stops Up/Down/Accept **driving** a tree the player is not
looking at; Back drives nothing — it either finds a close handler or logs. Running the gate would
buy no safety and would **suppress the diagnostic line in the single most important case**: a
sub-screen is open (so the menu reads as *covered*) and did not register. Flagged, not
self-adjudicated (`SC-§101`).

---

## §5. ⛔ THE THREE ROWS' SHIPPED WORK — RE-GREPPED OUT OF THE WORKING COPY AFTER THE LAST EDIT

⚠️ **RE-RUN AFTER THE LOOP-1 PASS**, and ⛔ **every citation below is now BY TEXT.** The loop-1 fix
added ~+100 lines inside `StepSlider`, so **every line number in the original version of this
section moved** — which is the argument `CITE-BY-TEXT-RULED-2026-09-24` makes, demonstrated on my
own note twice in one row. Counts are `grep -c` over the working copy after the last edit.

**`TASK-1394` — all FOUR instrument lines, byte-identical format strings (`grep -c` = 1 each):**

| line (the grep) | present | note |
|---|---|---|
| `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` | ✅ | **still the first statement of `HandleMenuDown()`** — the signature, `{`, the comment block, then this `UE_LOG` |
| `MoveFocus(%+d) declined: menu covered.` | ✅ | untouched |
| `MoveFocus(%+d) declined: no menu buttons.` | ✅ | untouched |
| `MoveFocus(%+d): focus moved %d -> %d of %d ('%s').` | ✅ | untouched — this is the one whose **argument list** `TASK-1406` legitimately changed (`Buttons`→`Stops`); I changed **neither** the string nor the arguments |

**`TASK-1400` — the 0.2 s re-entry poll:** `FocusReentryPollSeconds = 0.2f` and
`FocusReentryPollTimerHandle` intact; armed after the map gate with `bLoop = true`, cleared in
`Deinitialize`; **`ApplyInitialFocus`'s guard `if (Buttons.Num() > 0 && !GetFocusedMenuButton())` is
intact** and that whole function is byte-identical.

> 📄 **WARN-4, struck not deleted.** This paragraph previously cited that guard at ~~`.cpp:1224`~~.
> **Wrong number, correct text** — `:1224` was the `IsMenuUncovered()` early-out's `return;`; QA
> re-measured the guard at `.cpp:1229`, and after the loop-1 pass it has moved again. ⛔ Cite the
> predicate, not an address.

**`TASK-1406` — the registration API and the widened walker:** `RegisterMenuNavTarget` (`.cpp:590`),
`UnregisterMenuNavTarget` (`.cpp:617`), `GetActiveNavTarget`, `GetRegisteredNavTarget`,
`IsNavTargetActionable`, `GetMenuFocusStops`, `GetFocusedNavStop`, `IsNavFocusStop`,
`FocusFirstNavStop`, `FocusWidget`, `NavTargetStack` — **all present, none edited.** The four
admitted classes are unchanged, and **`Slider->IsFocusable` at `.cpp:460` is still the public member
read — I did NOT "fix" it into `GetIsFocusable()`** (there is no such getter on `USlider`;
re-verified against `Slider.h:96`, which carries no `UE_DEPRECATED`).

**The Accept line, cited by its text** (`CITE-BY-TEXT-RULED-2026-09-24`):
`"[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."` — ✅ present
and **byte-identical** (`grep -c` = 1), with its `UButton`-only body and its
`Focused->OnClicked.Broadcast()` unchanged.

> 📄 **WARN-4, struck not deleted.** The original sentence added *"at ~~`.cpp:857`~~"*. **Wrong
> number, correct text** — QA re-measured it at `.cpp:861-863` (`:857` was the (4b) line's argument
> list), and the loop-1 pass has not moved it (my +lines are all **below** it) but I am not quoting
> an address for it again. ⛔ The law exists because a number is stale the next time anyone inserts
> a line above it — including the edit that corrects it.

**`Escape`** — still **exactly one hit** across both files and it is the comment recording 🧑 his
ruling (`" ⛔ \`Escape\` IS NOT BOUND AND MUST NOT BE"`). **No `FKey`, no mapping, no binding, no
handler.** ⛔ Cited by its text; it now sits ~14 lines lower than the original note said.

**The loop-1 broadcast census** (so the commit edge is greppable and bounded):
`Slider->OnControllerCaptureEnd.Broadcast();` = **1 statement**, `Slider->OnMouseCaptureEnd.Broadcast();`
= **1 statement**, and they are the two arms of one `if/else` ⇒ **at most one fires per press**.
`RemoveFromParent` is still **8 hits, all comments** (5 `.cpp` + 3 `.h`) — the loop-1 pass added none.

**`Siegebound/Tests/SiegeMenuInputTest.cpp`:** ⛔ not opened for writing. It compiles against
`GetButtonLabel`, `WrapIndex`, `MenuDownActionPath`, `MenuAcceptActionPath` and `GetMenuButtons` —
**none of those five signatures or bodies is touched by this diff.**

---

## §6. ⚠️ `qa/TASK-1407.md` WARN-6 — ANSWERED, NOT LEFT IN SILENCE

**It is mine, and my (2) work covers it naturally — so I took it and added the line.**

WARN-6: *Accept on a focused `UCheckBox` is a silent no-op; `Cast<UButton>(GetFocusedNavStop())`
returning null now has two causes and a reader cannot tell them apart.* Two things land:

1. **The behaviour half is (2)'s, not a rider**: Accept on a focused `UCheckBox` now **toggles it**
   (state-compared, broadcast on a real change only). So the case WARN-6 was written about is no
   longer a no-op at all.
2. **The silence half is (4b)'s**, and it gets the line QA asked for — one line naming the stop
   **and its class**, in the same shape as (2)'s deliberate `UButton` Left/Right nothing:

```cpp
UE_LOG(LogSiegeMenuInput, Log,
    TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept declined: focused stop '%s' (%s) has no Accept semantics (or nothing is focused)."),
    *GetNameSafe(FocusedStop), *StopClassName);
```

⚠️ **Two deliberate departures from WARN-6's suggested text, both declared:**
(a) its wording *"is not a UButton"* would now be wrong — a `UCheckBox` **is** handled — so the line
says *"has no Accept semantics"*; (b) it called `GetFocusedNavStop()` a **second** time inside the
log; I call it **once** and hold the result, because it walks the whole target tree.

⛔ `USlider` and `UEditableTextBox` get **no** Accept semantics: "Accept on a slider" has no agreed
meaning, and inventing one is the unreviewed-semantic shape this epic exists to remove. They fall to
the line above — which is the point of (4b).

---

## §7. WHAT MOVED IN EACH FILE

**`SiegeMenuInputSubsystem.h`** (+1 include, +1 `UINTERFACE`, +3 statics, +1 constant, +7 private
methods, +3 `UPROPERTY` members, 3 comment blocks):
`UObject/Interface.h` · `class UCheckBox;` / `class USlider;` forward decls · the
`ISiegeMenuNavCloseTarget` interface · `MenuLeft/Right/BackActionPath` · `DefaultSliderStepFraction`
· `HandleMenuLeft/Right/Back`, `StepFocusedStop`, `StepSlider`, `SetCheckBoxChecked`,
`FindStepperPair` · `MenuLeft/Right/BackAction` · a `TASK-1409` section in the class comment ·
`IsArmed()`'s comment now says *three CORE actions* and why the flag was not widened.

**`SiegeMenuInputSubsystem.cpp`** (+1 include, +2 file-local namespaces, +3 static definitions,
+7 function bodies, 4 in-place edits):
`Components/PanelWidget.h` · `SiegeMenuStepper` (the two name suffixes + two glyphs) and
`SiegeMenuCheckBox::StateWord` · the three action paths · **edits**: the degrade-open load block,
the three guarded `BindAction`s + the second arm line, the three `nullptr` clears in
`Deinitialize`, and `HandleMenuAccept`'s check-box branch + (4b) line.

### 🔁 LOOP 1 added, in exactly two functions and nowhere else

| file | function | what changed |
|---|---|---|
| `.cpp` | `StepSlider` | **the commit edge** — the no-op return, the two `IsBound()` reads, the "no capture-end handler bound" return, the pre-broadcast line, the single `OnControllerCaptureEnd`/`OnMouseCaptureEnd` broadcast, the post-commit line, and the block comment that measures all of it. ⛔ The pre-existing `SetValue` / read-back / step-log lines are unchanged except for one added sentence pointing at the new block. |
| `.cpp` | `HandleMenuBack` | `TargetOrigin` + the third `%s` on the two lines that name a target (WARN-5). ⛔ No control flow changed; the entry line is untouched and still the first statement. |
| `.h` | class comment | the single "one asymmetry" paragraph became **two** — (i) the engine's, unchanged in substance; (ii) **this project's two-phase sliders**, which is the one the first cut missed. The `USlider` bullet now points at (ii). |
| `.h` | `StepSlider` declaration comment | says the commit edge is part of the contract, not an extra. |

⛔ **No signature changed, no member added, no include added, no `UPROPERTY` added** in loop 1 —
so UHT's regeneration story (note 2 below) is exactly as `TASK-1410` already described it.

---

## §8. Notes for QA (`TASK-1410`) and build-master

1. **Every new engine symbol was verified against the UE 5.8 headers on this machine**, not assumed:
   `USlider::GetValue/SetValue/GetMinValue/GetMaxValue/GetStepSize` (all public non-deprecated
   `UFUNCTION`s, `Slider.h:125-181`; the *fields* `Value`/`MinValue`/`MaxValue`/`StepSize` are
   `UE_DEPRECATED(5.1)` and **I use none of them**) · `UCheckBox::IsChecked/GetCheckedState/
   SetIsChecked` + the public `OnCheckStateChanged` (`CheckBox.h:79-101`) · `UWidget::GetParent()`
   → `UPanelWidget*`, const (`Widget.h:769`) · `UPanelWidget::GetChildrenCount/GetChildAt`, both
   const (`PanelWidget.h:28/36`).
2. **UHT will regenerate `SiegeMenuInputSubsystem.generated.h`** — a **new `UINTERFACE` + `UCLASS`**
   in the header. ⛔ **Live Coding cannot carry this**; the editor must be closed, compiled, and
   relaunched on the new binaries (the same call `TASK-1407` note 2 made for `TASK-1406`).
3. **`Execute_OnMenuNavBackRequested` is only ever called after `Implements<>()` returns true.**
   ⚠️ For `TASK-1454`: a C++ implementer **must** provide `OnMenuNavBackRequested_Implementation()`
   — UHT's generated default for an interface `BlueprintNativeEvent` is a `check(0)` stub. That is
   the doctrine-correct loud failure, not a silent no-op, and it is compiled out under Shipping.
4. **Verbosity**: everything here is `Log` except the plain-`UButton` Left/Right nothing, which is
   `Verbose`. `Log` is compiled out entirely under **Shipping** (`USE_LOGGING_IN_SHIPPING = 0`, no
   `Target.cs` override) — the same precondition the Accept line and `TASK-1400`'s line carry.
5. **Balance check run** (no compiler available to me): braces / parens / brackets all net **0** in
   both files with comments and string literals stripped. That is a syntax smoke test, **not** a
   compile, and I am not presenting it as one. **Re-run after loop 1** — `.cpp` 135/135 braces,
   516/516 parens, 15/15 brackets, final depth 0 and **minimum running depth 0** (so no early
   close); `.h` 4/4, 54/54, 0/0. ⭐ `TASK-1410` NIT-1 is right and I repeat it: syntax validity is
   5a's to establish, not mine.
6. **Loop-1 format-string audit, done by hand** — four new `UE_LOG`s, specifier counts vs argument
   counts: `%+d`+`%s`+`%.4f` ↔ 3 · `%+d`+`%s`+`%.4f` ↔ 3 · `%+d`+`%s`+`%s` ↔ 3 ·
   `%+d`+`%s`+`%s`+`%.4f`+`%.4f` ↔ 5. `EdgeName` is a `const TCHAR* const` from `TEXT(...)`; floats
   promote to double through varargs as every other `%.4f` in this file already does.
7. **`IsBound()` on a dynamic multicast is not an invention**: `FOnControllerCaptureEndEvent` /
   `FOnMouseCaptureEndEvent` are `DECLARE_DYNAMIC_MULTICAST_DELEGATE`s, which derive from
   `TMulticastScriptDelegate`, whose `inline bool IsBound() const` is declared in
   `Runtime/Core/Public/UObject/ScriptDelegates.h`. Verified on this machine's 5.8 tree.

---

## Not examined / limitations

1. ⛔ **NOT COMPILED, NOT RUN, NOT PIE'd.** No claim in this note is a runtime observation. The
   editor (PID 32984) was never contacted.
2. 🚨 **`IA_MenuBack` is inert everywhere on the day this lands, and so is Left/Right in practice**
   — no screen registers and no screen implements the close handler. See §0. `TASK-1454` (close
   handlers) and `TASK-1415` (first `RegisterMenuNavTarget` caller) are what make either visible.
3. **The runtime criterion cannot be met by this row alone — AND, 🔁 loop 1, IT IS ALSO THE WRONG
   CRITERION.** *"`inject_input_action IA_MenuRight` on a focused `USlider` MOVES its value in a
   node read"* needs a **registered** screen holding a slider: the registered half is now live
   (`TASK-1415`), the slider half waits on `TASK-1417` (Settings has no `USlider`). 🚨 **And a node
   read cannot discriminate this row's own blocker** — it passed on the first cut, which moved the
   handle and committed nothing. It must read the **facade**. ⛔ Flagged for the manager /
   `TASK-1413`, **not** rewritten by me (`SC-§50`). ⛔ And do not discharge it by having a verifier
   call `RegisterMenuNavTarget` itself — `qa/TASK-1407.md` WARN-7 already ruled that measures a
   synthetic driver, not the product.
4. **Each graphics stepper row exposes TWO focus stops**, not one: `PrevButton` and `NextButton` are
   both `UButton`s, so `TASK-1406`'s walker admits both and Down visits each. Left/Right behave
   identically on either, so the row is *usable* from both stops — but the ring will appear to
   "stop twice" on a stepper row. Not a defect I can fix inside this fence (it is the walker's
   vocabulary, `TASK-1406`'s); named so nobody reads it as a bug in this diff.
5. **`FindStepperPair` is structural + name/glyph based and cannot see a stepper that is neither.**
   A future stepper using `◀`/`▶`, `-`/`+`, or three buttons in a row is **not** recognised and
   falls to the deliberate-nothing branch. That is the safe direction of failure and it is the
   reason the glyph set was not widened by guesswork.
6. **`UCheckBox`'s `Undetermined` state on Accept**: the toggle uses `!IsChecked()`, which folds
   `Undetermined` → `Checked`. Defensible, but it *is* a choice; the direction keys handle the
   tri-state correctly (state-compared), and no shipped check box in this project is tri-state.
7. **`USlider::SetValue`'s internal broadcast is guarded on its own cached `Value` member**, while
   `GetValue()` reads the live `SSlider` when one exists. If those two ever disagree, a step could
   move the widget without broadcasting. I could find no path that desynchronises them
   (`HandleOnValueChanged` writes both), so it is recorded rather than defended against — and the
   read-back in the log line is what would expose it.
8. **`MinimalAPI` on the interface** means `USiegeMenuNavCloseTarget` is usable from this module
   (where every menu screen lives) but is not DLL-exported. Matches the project's other two
   interface headers; if a future module needs it, it wants `GITCLAUDEUNREALTEST_API`.
9. **I did not measure how many buttons the non-graphics screens' footers hold.** The exactly-two +
   name/glyph guard is what makes that safe without the census; a census would still be worth having
   before any future row loosens either discriminator.
10. **No asset was read or written** (`TASK-1408` owns `Content/Input/`). The key list in §2 is
    quoted from that row's handoff, not re-measured from the `.uasset`.

### 🔁 Loop-1 limitations — the commit edge, declared rather than implied

11. 🚨 **I broadcast the commit edge; I do not observe the commit.** That the graphics handlers
    write the facade is something I **read** (`Handle*SliderCommitted` → `SetQualityGroupLevel` /
    `SetOverallScalabilityLevel` / `SetResolutionScalePercent`), not something this row can prove
    without running. ⇒ **the fix's own proof is a facade read at runtime**, which is exactly why
    routing request 1 asks `TASK-1413` to assert the facade. ⛔ Do not read my log line
    `"… -> OnControllerCaptureEnd.Broadcast() …"` as proof the setting changed — it proves the edge
    fired, which is one link short.
12. **Whatever is bound to that edge runs.** If a future screen binds `OnControllerCaptureEnd` to
    something that is not a commit, an arrow key will fire it. That is the honest semantics of
    "fire the delegate the human path fires" — a pad user ending an interaction fires the same
    thing — but it **is** a consequence, and it is the same class of exposure the stepper-pair
    `OnClicked.Broadcast()` already carries (which is why `FindStepperPair` is so restrictive).
13. **One press = one facade write = one ini save** on the resolution-scale row. Declared, argued
    against cl. (3a) in the loop-1 section, and ⛔ **not** debounced on purpose. If the manager
    disagrees and wants a debounce, it changes what a verifier can observe immediately after the
    press and should be boarded as its own row.
14. **The post-commit read-back can legitimately differ from the value I set** — a committed write
    re-seeds the panel, and the overall preset slider also rewrites the resolution-scale row. The
    second log line reports it rather than fighting it; ⛔ nobody should "fix" that divergence.
15. **The exact `ReadBack == OldValue` gate inherits `TASK-1410` NIT-3's assumption.** QA found no
    path in 5.8 that desynchronises `USlider`'s cached `Value` from the live `SSlider`, and I found
    none either; if one ever appears, the commit edge could be skipped on a real change (or fired
    without one) in lock-step with `OnValueChanged` doing the same. Both numbers are in the log
    line, which is what would expose it.
16. ⛔ **`Tests/SiegeGraphicsMenuTest.cpp` asserts the slider BINDING TOPOLOGY** (`OnValueChanged`
    is the label handler and **not** the write handler; both capture-end events carry the write).
    My change **binds and unbinds nothing**, so every one of those assertions still describes the
    code. I did not open that file. ⚠️ It is also the test that would catch a future row "fixing"
    this by moving the write onto `OnValueChanged` — which would be the fifty-ini-writes defect.
