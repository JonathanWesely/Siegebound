# TASK-1314 — [PLAYAGAIN-KEY-REACHABLE] — programmer handoff

**Status delivered: `ready-for-qa`** (gate = `TASK-1315`, host = `TASK-1316`, verify = `playtest-verifier` on (7b))
**ROUTE TAKEN: ⛔ ROUTE K-2 — FORCED BY (0)(d), NOT CHOSEN.** The Play Again button carries an **authored `IsFocusable=False`**, so the row's own conditional (*"Route K-2 … ONLY IF (0)(d) measures the button as NOT focusable on its own"*) fires.
**🧑 JONATHAN IS NEEDED — ONE BP-EDITOR PROPERTY + SAVE.** Declared at §3, not discovered at 5a.
**⛔ NO `.uasset` IS IN MY DIFF.** Per spec (3) I **named the cost and stopped**: no MCP save, no metadata-tag dirty, no `execute_unreal_python`.
Marker: `TASK-1314-PLAYAGAIN-KEY-REACHABLE` · programmer: gameplay-programmer · date: 2026-09-19

**Reads performed whole (`SC-§38a`):** `PLAYAGAIN-KEYBOARD-RULING` **incl. AMENDMENTS A and B** · `#### TASK-1314` row + `names:` · `handoffs/TASK-1311-programmer.md` · `qa/TASK-1312-report.md` (esp. §5) · `SiegePlayerController.cpp:2130-2290` · `Docs/GDD.md:169`/`:491` · `CONVENTIONS.md` `DECK-§9` cl. 1-9.

**Tree at my start:** `main` at `f37bd28`, **7 ahead of origin**, tree clean. Blocker `TASK-1313` **committed** (`1c93610` fix + `f37bd28` board flips) ⇒ the `(1e)` hard file fence is discharged.

---

## §0 — THE FIVE READS, MEASURED AT MY INSTANT (`SC-§91`), WITH ACTUAL VALUES

### 🚨 (a) THE PLAY AGAIN BUTTON'S EXACT WIDGET NAME — **`Btn_Jump`**

⛔ **`Btn_PlayAgain` WAS A GUESS AND IT IS WRONG. THE SHIPPED NAME IS `Btn_Jump`** — a third-person-template leftover that was never renamed. Read back from the asset's own widget tree, **not** typed from the board.

Instrument: `mcp__unreal_inspector__get_asset_meta /Game/UI/WBP_VictoryScreen.WBP_VictoryScreen [WidgetTree, PropertyDeclarations, PropertyValues]` (read-only lane; I opened **no** Blueprint editor, ran **no** compile, issued **no** save). Verbatim:

```
Widget Tree:
Overlay "Overlay_19"
  SizeBox "SizeBox_0"
    WidthOverride=260.000000
    HeightOverride=80.000000
    Button "Btn_Jump" [Variable]
      WidgetStyle=(Normal=(...VirtualJoystick_Thumb...))
      IsFocusable=False
  UI_Thumbstick_C "Thumbstick_Move" [Variable]   Visibility=Collapsed
  UI_Thumbstick_C "Thumbstick_Aim"  [Variable]   Visibility=Collapsed
```

**Second instrument, because one reading is not a measurement (`SC-§39`) — the EventGraph proves `Btn_Jump` IS Play Again, it is not merely the only candidate:**
`get_asset_graph … ["EventGraph"]` shows `Event Construct` → `GenericCreateObject(Class=TextBlock)` → `AddChild` whose **Target is `Btn Jump`** and whose Content is that TextBlock, with `Conv_StringToText(In String="Play Again")` → `SetText`. The same graph has `AssignDelegate` with **Target `Btn Jump`** binding `OnClicked_Event` → `Cast: SiegePlayerController` → **`RequestPlayAgain`** → `RemoveFromParent`.
⇒ **The label "Play Again" is added to `Btn_Jump` at runtime and `Btn_Jump`'s click calls `RequestPlayAgain`. That is the button.**

⛔ **The name is a booby trap and I have fenced it in the code comment:** nothing about `Btn_Jump` says "play again", and fence (6e) independently notes that a `Btn_Jump` in `WBP_CardHand` is load-bearing for the card bar. **These are different widgets in different assets** — I touched neither asset.

### (b) EVERY OTHER FOCUSABLE CONTROL ON THAT SCREEN — **THERE ARE NONE. LITERALLY ONE BUTTON.**

From the same tree: the only `Button` is `Btn_Jump`. The two `UI_Thumbstick_C` are **`Visibility=Collapsed`** (collapsed widgets are not hit-tested and not focus-navigable). `Overlay_19` / `SizeBox_0` are layout panels with no focus support. The `TextBlock`s are created at runtime and one is explicitly `SetVisibility(Not Hit-Testable (Self & All Children))`.

⇒ **The GDD's *"Victory / Defeat + 'Play Again'"* (`Docs/GDD.md:169`, `:491`) is LITERALLY one button, confirmed from the asset and not from the GDD**, as (b) demanded.
⇒ **The `Enter`-hits-the-wrong-handler risk is ZERO on this screen** — there is no second focusable control for Accept to land on.
⚠️ **One thing (b) turned up that the board does not record:** the widget declares **seven** events — `Construct` plus `OnClicked_Event` and `OnClicked_Event_0…_4`. Only `OnClicked_Event` is wired (it is the one `AssignDelegate` targets). **`OnClicked_Event_0…_4` are orphan stubs with no delegate and no exec input** — dead, pre-existing, and **not mine to clean** (fence (6a): the button's behaviour is untouchable). Recorded for QA, changed nothing.

### (c) THE ROOT's `bIsFocusable` — **`False`** (re-read at my instant, NOT cited from QA)

```
  uint8 bIsFocusable = False
  FWidgetChild DesiredFocusWidget = ()
```
Both re-read by me from the live CDO. Agrees with QA's 2026-09-18 reading and with AMENDMENT A2. **I did not act on it** — see §1.

### 🚨 (d) THE BUTTON'S OWN FOCUSABILITY — **NOT FOCUSABLE. THIS IS THE ROUTE DECISION AND IT COSTS 🧑 HIM A KEYSTROKE.**

The row asked for two engine quotes; both, at `file:line`, plus the instance value that overrides them:

| What | Where | Value |
|---|---|---|
| `UButton::IsFocusable` **UPROPERTY default** | `UMG/Private/Components/Button.cpp:48` | `IsFocusable = true;` (in the constructor) |
| the UPROPERTY itself | `UMG/Public/Components/Button.h:67-70` | `UE_DEPRECATED(5.2, "Direct access to IsFocusable is deprecated. Please use the getter.")` / `UPROPERTY(EditAnywhere, BlueprintReadOnly, Getter, Category="Interaction")` / `bool IsFocusable;` — comment: *"Sometimes a button should only be mouse-clickable and never keyboard focusable."* |
| pushed into Slate at construction | `Button.cpp:84` | `.IsFocusable(IsFocusable)` inside `RebuildWidget()` |
| `SButton::SupportsKeyboardFocus()` | `Slate/Private/Widgets/Input/SButton.cpp:271-274` | `bool SButton::SupportsKeyboardFocus() const { … return bIsFocusable; }` |
| Slate arg default | `Slate/Public/Widgets/Input/SButton.h:62` | `, _IsFocusable( true )` |
| **THE INSTANCE VALUE — the one that decides** | `/Game/UI/WBP_VictoryScreen` widget tree, `Btn_Jump` | ⛔ **`IsFocusable=False`** |

⇒ ⛔ **THE ENGINE DEFAULT IS `true` AND THIS ASSET EXPLICITLY OVERRIDES IT TO `False`.** A UMG property is serialized into the widget tree only when it differs from the archetype, so the printed `IsFocusable=False` **is** an authored override, not an echo of a default.
⇒ ⛔ **THE BUTTON IS NOT FOCUSABLE ON ITS OWN ⇒ (1)'s K-2 CONDITION IS MET ⇒ ROUTE K-2, AND THE ROW COSTS HIM A KEYSTROKE.**

🚨 **AND THERE IS NO CODE-ONLY ESCAPE, WHICH IS WHY K-1 ALONE CANNOT DELIVER THIS FEATURE — I checked before conceding the keystroke:** `UButton` exposes **no runtime setter**. The only writer is `UMG_API void InitIsFocusable(bool)` at **`Button.h:205-206`**, whose own comment is *"Initialize IsFocusable in the constructor **before the SWidget is constructed**"*, and the value is baked into the `SButton` at `Button.cpp:84` during `RebuildWidget()`. The property is `BlueprintReadOnly` with a `Getter` and **no `Setter`**.
⚠️ **The one thing that COULD have avoided him, declared rather than hidden:** `SButton::SetIsFocusable(bool)` **does** exist and is public at **`SButton.h:299-301`**, so C++ could reach through `TakeWidget()`, `StaticCastSharedRef<SButton>` and flip the Slate object at runtime. ⛔ **I DID NOT DO THAT, DELIBERATELY** — it is a third route the row never authorised; it would leave the asset saying `False` forever while the game behaved as `True` (exactly the silent asset-vs-behaviour divergence this project has been burned by); and the cast makes a fragile assumption about UMG's wrapper. **Flagged for the gate as a real option I refused, not as an option I missed.**

### 🚨 (e) THE INPUT ROUTE — **SLATE'S OWN LANE IS LIVE. AMENDMENT A1 BRANCH (i). NO STOP.**

`FInputModeUIOnly::ApplyInputMode` (`Engine/Private/PlayerController.cpp:6372-6387`), quoted for what it does to **game** input:

```cpp
TSharedPtr<SViewport> ViewportWidget = GameViewportClient.GetGameViewportWidget();
if (ViewportWidget.IsValid())
{
    SetFocusAndLocking(SlateOperations, WidgetToFocus, bLockMouseToViewport, ViewportWidget.ToSharedRef());
    SlateOperations.ReleaseMouseCapture();
    GameViewportClient.SetMouseLockMode(MouseLockMode);
    GameViewportClient.SetIgnoreInput(true);            // :6384  ⇐ game input is OFF
    GameViewportClient.SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
}
```
and `FInputModeDataBase::SetFocusAndLocking` (`:6313-6317`) issues `SlateOperations.SetUserFocus(InWidgetToFocus.ToSharedRef())` **only when the pointer is valid**.

**⛔ THE ONE-SENTENCE ANSWER (0)(e) DEMANDS:** under `FInputModeUIOnly` a real `Enter` press travels **OS → `FSlateApplication` → the Slate keyboard-focus path of the focused widget → `SButton::OnKeyDown`** (`SButton.cpp:293`, which asks `FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept` at `:296` and calls `ExecuteOnClick()` at `:303`) — **it never enters the PlayerController / Enhanced Input lane at all, because `SetIgnoreInput(true)` at `:6384` has switched that lane off**; so the press reaches Play Again **if and only if the focused Slate widget is the Play Again `SButton` itself**.

⇒ ✅ **AMENDMENT A1's pre-declared branch (i): Slate's route is LIVE under `FInputModeUIOnly` on `L_Arena` ⇒ the feature is BUILDABLE. No STOP, no `blocked`, no escalation.** And AMENDMENT B2's prediction is confirmed at mechanism level: focusing the button's own `SWidget` means `FSlateApplication::SetUserFocus`'s upward walk never runs, so focus stops at the **button** and not at `SViewport`.

---

## §1 — THE ROUTE, DECLARED FROM (0)'s VALUES

**ROUTE K-2.** Not a preference — **(0)(d) measured the button as not focusable on its own, which is the exact condition (1) attaches to K-2.** K-1 was the default and I would have taken it if the button had carried the engine's `true`; it does not.

**K-2 is two halves, and `names:` says so** (`WRITES:` the `.cpp` unconditionally, `OPTIONALLY (ROUTE K-2 ONLY) … WBP_VictoryScreen.uasset`):

| Half | Owner | State |
|---|---|---|
| **(i) the code** — resolve `Btn_Jump`, guard, focus it | me | ✅ **DONE**, 1 file, +64/−6 |
| **(ii) the asset** — `Btn_Jump.IsFocusable` `False → True` | 🧑 **Jonathan** | ⛔ **OWED — see §3** |

⛔ **I did NOT focus the root** (RULING 1(b)/1(d), A2, B1): `VictoryWidget->TakeWidget()` appears **0** times in the file.
⛔ **I did NOT take both routes**: K-1's code shape *is* K-2's code shape; the delta between the routes is solely whether the `.uasset` property must also be flipped, and (0)(d) says it must.

---

## §2 — THE INERT-FEATURE FENCE (spec (2)) — **HELD, AND MEASURED, NOT ASSERTED**

```
added lines in my diff matching IA_|IMC_|BindAction|UEnhancedInput   →   0
```
The route is **Slate-side only**: focus + `SButton`'s own Accept handling. I authored **no** `IA_`, **no** `IMC_`, **no** `BindAction`, **no** `UEnhancedInput*` call.

⭐ **And (0)(e) is why the fence is right rather than merely obeyed:** an Enhanced Input binding here would be dead **twice over** — `SetIgnoreInput(true)` at `PlayerController.cpp:6384` kills the lane under this input mode, and `USiegeMenuInputSubsystem` never runs on `L_Arena` at all (`SiegeMenuInputSubsystem.cpp:25` pins `MenuMapName = TEXT("L_MainMenu")`, `:47` `if (MapName != MenuMapName) { return; }` — **both re-measured by me at my instant**, not inherited from A1).

---

## §3 — 🧑 `SC-§125`: THE HUMAN-KEYSTROKE COST, DECLARED HERE

⛔ **THIS ROW NEEDS JONATHAN. ONE PROPERTY, ONE SAVE.**

> **In the Unreal Editor: open `/Game/UI/WBP_VictoryScreen` → select the button named `Btn_Jump` in the Hierarchy → Details → Interaction → tick **`Is Focusable`** (`False → True`) → **Compile** → **`Ctrl+S`**.**
> **Change nothing else.** Not the root's `Is Focusable`, not `DesiredFocusWidget`, not the button's `OnClicked`, not its style, not the thumbsticks.

**Why the programmatic lane cannot do it, per spec (3) and `UE-§ bp-compile-does-not-dirty`:** a UE5 Blueprint compile does **not** dirty the package, so `AssetTools.save_assets` returns `true` and writes **nothing** (sha256, bytes and mtime all unchanged). **I made no attempt** — no MCP save, no metadata-tag write to force a dirty, no `execute_unreal_python`. I named it and stopped, exactly as (3) orders.

**`SC-§68` — the asset, named, with its hash, UNTOUCHED BY ME:**
`Content/UI/WBP_VictoryScreen.uasset` — sha256 **`7817dd7f19bcf1028bd21ddfc4d92731c01c14d567c19641cc10e9f1d12bd6ee`**, **153,489 bytes**, mtime **`2026-07-29 12:23:05 -0700`**.
⭐ **Identical to the value `TASK-1311` recorded and `TASK-1313` re-verified at its commit instant** ⇒ independently confirms I wrote no package byte.
**The hash-must-change discharge is owed against THIS pre-image:** after his save, the new sha256 must **differ** from `7817dd7f…2bd6ee`. A save that leaves it equal did not happen.

---

## §4 — 🚨 THE ONE DEVIATION, FLAGGED UNDER `SC-§121` cl. 5 — **BUILT AND FLAGGED, NEVER SMUGGLED**

**The spec did not order a focusability guard. I added one, and it is the most important line in the diff.**

```cpp
if (PlayAgainSlate->SupportsKeyboardFocus())
{
    InputMode.SetWidgetToFocus(PlayAgainSlate);
}
else { /* Warning, no focus target */ }
```

**Why, and it is not defensive padding.** K-2 lands in **two commits by two different hands** — my code now, his save later. In the window between them the asset still says `IsFocusable=False`. **Without this guard, the committed code would hand a non-focusable `SWidget` to the engine's focus-target setter, whose `!InWidgetToFocus->SupportsKeyboardFocus()` test at `PlayerController.cpp:6343` would log `InputMode:UIOnly - Attempting to focus Non-Focusable widget Btn_Jump!` at EVERY REAL MATCH END — reinstating the exact engine `Error` `TASK-1311` removed at `1c93610` and breaking this row's own acceptance (7b) (*"`TASK-1311`'s gain may not regress"*).**

The guard asks **the identical predicate on the identical object** the engine is about to ask, so the site is **structurally incapable** of emitting that `Error` — not "unlikely to", *cannot*, whatever the asset says. It also makes the diff **order-independent**: correct before his save (silent, mouse-only, exactly today's behaviour) and correct after it (focus on the button), with no second code change needed.

**Cost of the deviation:** 4 executable lines (`if` / `else` / one `UE_LOG`), one-block delete to undo. **I am flagging it rather than relying on it passing unnoticed** — the gate should rule on it explicitly, as `TASK-1312` RIDER 1 did for `TASK-1311`'s comment.

⚠️ **One consequence to rule on, stated because it is the guard's price:** until his save, **every match end logs one `Warning`**. I chose `Warning` over `Log` because a half-landed feature should be audible; the file already emits `Warning` at three sibling branches in this same function. **If the gate prefers `Log`, that is a one-word change and I will take the finding.**

---

## §5 — `SC-§123`: REACHABILITY IS NOT CORRECTNESS, IN MY OWN WORDS

⛔ **A test that calls `HandleMatchEnd` directly and then asserts focus proves the MECHANISM, NOT THE ROUTE — it is the entry point calling itself, which is no evidence that anything in a real match ever calls it.** I therefore make no reachability claim of my own and **cite `TASK-1312` (5)** rather than re-deriving it, as (5) instructs:

> `ACastle.cpp:1234` `OnCastleDestroyed.Broadcast` → bound at `SiegeGameMode.cpp:134` → handler `:523` → `SiegeGameState->SetMatchResult` at `:593` → `SiegeGameState.cpp:204`/`:219` `NotifyLocalControllersMatchEnd` → **`SiegeGameState.cpp:266` `SiegePC->HandleMatchEnd(WinningTeam)`**, gated on `IsLocalController()` at `:264`. Fallback: `SiegeGameMode.cpp:606`. **Precondition: a castle reaches 0 HP.**

Inside the function, my block sits **after** the `bMatchEnded` latch and **after** the `if (!IsLocalController()) return;` guard ⇒ **it runs once per match, on the local controller only.**

### The automation test — ⛔ **DECLARED `UNTESTABLE-IN-SUITE`. SUITE TOTAL = BASELINE ±0. NO TEST ADDED.**

**The one sentence with the reason (5) asks for:** a headless `-nullrhi` lane has **no `SViewport` game-viewport widget**, and the *entire* body of `FInputModeUIOnly::ApplyInputMode` — `SetFocusAndLocking` included — is wrapped in `if (ViewportWidget.IsValid())` at `PlayerController.cpp:6374`, so **no focus is ever applied in that lane at all** and an assertion that `Btn_Jump` holds Slate keyboard focus could only ever return false regardless of whether the feature works.

⇒ Such a test **could not fail for the right reason and could not pass for any reason** — `SC-§39`'s test-that-cannot-discriminate, and a green pin over it would be **worse than no test because it would spend the claim**. I wrote none.
**Reconciled BY NAME (`SC-§104`):** zero test cases added, zero removed, zero renamed; `Siegebound.Victory.PlayAgainHoldsFocusAtMatchEnd` **does not exist** and was deliberately not created. No existing test is disturbed — `grep -rln "HandleMatchEnd\|VictoryScreen\|VictoryWidget" Source/GitClaudeUnrealTest/Siegebound/Tests/` returns **no files**.

---

## §6 — `SC-§121`: THE KEY CENSUS

### (a) THE ACTIVATION IS THE ENGINE'S, NOT OURS — CONFIRMED AT SOURCE

`SButton::OnKeyDown` (`Slate/Private/Widgets/Input/SButton.cpp:293`):
```cpp
if (IsEnabled() && FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
{
    Press();
    if (PressMethod == EButtonPressMethod::ButtonPress) { Reply = ExecuteOnClick(); }   // :303
```
`FNavigationConfig`'s defaults (`Slate/Private/Framework/Application/NavigationConfig.cpp:31-34`), verbatim:
```cpp
// By default, enter, space, and gamepad accept are all counted as accept
KeyActionRules.Emplace(EKeys::Enter, EUINavigationAction::Accept);                              // :32
KeyActionRules.Emplace(EKeys::SpaceBar, EUINavigationAction::Accept);                           // :33
KeyActionRules.Emplace(EKeys::Virtual_Gamepad_Accept.GetVirtualKey(), EUINavigationAction::Accept); // :34
```
⇒ ✅ **`Enter`, `SpaceBar` and gamepad Accept are the ENGINE's mapping. Our code does NOT name a key, and does NOT need to.** The (a) STOP condition (*"if our code must NAME a key — STOP AND FLAG"*) **did not fire**.

### (b) CENSUS AGAINST THE REGISTER (`CONVENTIONS.md`), RESULT WRITTEN OUT

| Token | Hits | Disposition |
|---|---|---|
| `EKeys::Enter` | **0** | censused against the register, **no ruling** (`SC-§70`) |
| `EKeys::SpaceBar` | **0** | censused against the register, **no ruling** |
| `EKeys::Escape` | **6** | ⚖️ **RULING EXISTS — `AS-§6 A-2` + `DECK-§9` cl. 5.** *"`AS-§6 A-2` STILL BINDS EVERYWHERE ELSE: this is a GRANTED exception with a register entry, not a repeal. A second site absorbing `Escape` is still an automatic QA FAIL and still needs his word."* **HOW I COMPLY: I add no `Escape` — see (d).** |
| `EKeys::Tab` | **3** | `DECK-§9` cl. 4 — `Tab` is *"the one deliberate absence"*. **I bind no `Tab`; Slate's `Next` navigation is left exactly as inherited.** |
| `Virtual_Accept` | 1 | `DECK-§9` cl. 5 — call-shape ruling only (`FPlatformInput::GetGamepadAcceptKey()`). **Not a rebind, and I author neither spelling.** |
| `FaceButton_Bottom` | 1 | same row of `DECK-§9`. **Untouched.** |
| `NativeOnKeyDown` | 9 | absorb shape — **I add none.** My route is focus-only; I override no key handler anywhere. |
| `NativeOnPreviewKeyDown` | 7 | absorb shape — **I add none.** |
| *"never absorbed"* | 1 | `SC-§121` cl. 2's own text. **No absorb is added by this row, so nothing is at risk of becoming absorbed.** |
| *"automatic QA FAIL"* | 24 | the `Escape` enforcement among them — **complied with by (d).** |

⭐ **The structural reason this row is quiet against the register: I add no key handler at all.** I move **focus**; the engine's pre-existing `FNavigationConfig` decides what a key means. **Nothing this row does can absorb a key that previously fell through**, because no new `OnKeyDown`/`OnPreviewKeyDown` exists to absorb it.

### (c) RECONCILE THE COUNT — ⛔ **`20 → 20`. RE-DERIVED AT MY INSTANT FROM WORKING-TREE BYTES.**

`DeckBuilderWidget.cpp`, the seven executable lines, counted by me (not quoted from `DECK-§9`):

| Line | n | text |
|---|---|---|
| `:1225` | 3 | `if (Key == EKeys::Left \|\| Key == EKeys::Gamepad_DPad_Left \|\| Key == EKeys::Gamepad_LeftStick_Left)` |
| `:1229` | 3 | `… EKeys::Right \| Gamepad_DPad_Right \| Gamepad_LeftStick_Right` |
| `:1233` | 3 | `… EKeys::Up \| Gamepad_DPad_Up \| Gamepad_LeftStick_Up` |
| `:1237` | 3 | `… EKeys::Down \| Gamepad_DPad_Down \| Gamepad_LeftStick_Down` |
| `:1639` | 3 | `… EKeys::Enter \| Virtual_Gamepad_Accept.GetVirtualKey() \| Gamepad_FaceButton_Bottom` |
| `:1663` | 2 | `… EKeys::Delete \| Gamepad_FaceButton_Left` |
| `:1727` | 3 | `… EKeys::Escape \| Gamepad_FaceButton_Right \| Virtual_Gamepad_Back.GetVirtualKey()` |
| **total** | **20** | on **7** executable lines |

`EKeys::Tab` in `DeckBuilderWidget.cpp` = **0** (`DECK-§9` cl. 9's invariant, still true).
⇒ ⛔ **20 before, 20 after. THIS ROW MOVES THE COUNT BY ZERO** — it adds no `EKeys::` expression anywhere. **The expected answer, met.** (I quote **20**, not the pre-ruling `19`.)

### (d) `Escape` — ⛔ **FENCED OUT, AND MEASURED**

```
EKeys::Escape ADDED by my diff  →  0
```
(The file's 4 pre-existing `EKeys::Escape` occurrences are untouched and are not mine.) He said *"Play Again keyboard reachable"*; he did **not** say *"Escape closes the end screen."* ⛔ **No `Escape` absorb was added.**
⚠️ **Worth QA's eye, and I am naming it rather than letting it be discovered:** `NavigationConfig.cpp:37` maps `EKeys::Escape` to `EUINavigationAction::**Back**` (not Accept) — re-measured at my instant; `:36` is that block's comment line. Once `Btn_Jump` holds focus, `Escape` is handled by **Slate's own navigation**, not by us, and `SButton::OnKeyDown` explicitly ignores anything that is not `Accept` (`SButton.cpp:296`). **We author nothing there and absorb nothing** — but the gate should confirm it agrees that engine-default `Back` behaviour is not a "new `Escape` absorb" under `AS-§6 A-2`. **My reading: it is not, because no line of ours names or consumes `Escape`.**

---

## §7 — FENCES (spec (6)), EACH ANSWERED DIRECTLY

- **(a) Play Again button's behaviour — ✅ UNTOUCHED.** No `.uasset` in my diff. `OnClicked`, its binding, `SetWinner`, `SetLocalVictory` unmodified (`SetWinner`/`SetLocalVictory` sit above my hunk, structurally intact). **I read the button's name and take its `SWidget`; I change nothing about what it does.** The asset change I ask of Jonathan is `IsFocusable` **only** — a focusability flag, not behaviour.
- **(b) Never duplicate-and-reparent `WBP_VictoryScreen` — ✅.** I opened **no** Blueprint editor, duplicated nothing, reparented nothing, saved nothing. My only asset contact was **two read-only inspector calls**. Corroborated by the unchanged sha256 in §3.
- **(c) `FInputModeUIOnly` posture byte-identical — ✅ ALL FIVE, IN ORDER, QUOTED POST-EDIT:**
  ```
  2273:	bShowMouseCursor = true;
  2274:	bEnableClickEvents = true;
  2275:	FInputModeUIOnly InputMode;
  2329:	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
  2330:	SetInputMode(InputMode);
  ```
  ⛔ **Only the distance between them changed** — my focus block necessarily sits between the `InputMode` constructor and `SetInputMode`, because that is the only place a focus target can be attached (spec (1) orders exactly this shape). **The mouse path is unchanged: cursor shown, click events on, `DoNotLock`, same `InputMode` object passed to the same `SetInputMode`.** 🧑 **His hand check (7c) depends on the mouse path being unchanged, and it is.**
- **(d) The stale comment — ✅ REWRITTEN, AND IT WAS NOT OPTIONAL.** The sentence *"whether Play Again should be key/pad-reachable at all is a product question owed to Jonathan"* is **GONE**. The new block records **the ANSWER** (*"Jonathan ruled 'yes, make Play Again keyboard reachable'"*), **the DATE** (`2026-09-19`) and **this row's marker** (`TASK-1314-PLAYAGAIN-KEY-REACHABLE`).
  **I preserved the good half and sharpened it, per RIDER 2 / NIT-1 / NIT-2:** *"Do not re-add it unless the widget root is made focusable first"* became *"⛔ Do not focus the root here … making the root focusable is necessary-but-NOT-sufficient"* (A2: `DesiredFocusWidget` is empty), and NIT-1's imprecision is corrected in the comment's own words — *"Slate never discards a non-focusable target either — it re-homes UPWARD to the nearest ancestor supporting keyboard focus, i.e. `SViewport`, which already holds focus in-match. That, not a discard, is why `TASK-1311`'s removed call was a true no-op."*
- **(e) Out of bounds — ✅ ALL CLEAR.** No `WBP_HUD`, no `WBP_CardHand` (⛔ its `Btn_Jump` is a **different widget in a different asset** and I did not go near it), no `L_Arena.umap`, no `Content/Dev/**`, no `Saved/**`, no `TASKBOARD.md`, no `CONVENTIONS.md`, no `settings.local.json`, no `.uproject`, no `Tools/run_suite_bounded.ps1`.
- **(f) `qa/TASK-007-report.md`'s WARN, in one line and changed nothing — ✅.** `HandleMatchEnd` still goes `FInputModeUIOnly` even when `VictoryWidget` is null; that is **`TASK-1319`'s**, and my focus block is correctly inside `if (VictoryWidget)` so it neither creates nor worsens it. **Untouched.**

---

## §8 — ACCEPTANCE

### (7a) STATE, QUOTED

- **Route:** **K-2**, from (0)(d)'s measured `IsFocusable=False` on `Btn_Jump` against `Button.cpp:48`'s default of `true`.
- **Measured button name, from a read-back:** **`Btn_Jump`** (§0(a), two instruments).
- ⛔ **`grep -c 'SetWidgetToFocus' Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` = `1`** ✅
- **The hunk, with its argument naming the BUTTON:**
  ```cpp
  static const FName PlayAgainButtonName(TEXT("Btn_Jump"));
  if (UWidget* PlayAgainButton = VictoryWidget->GetWidgetFromName(PlayAgainButtonName))
  {
      const TSharedRef<SWidget> PlayAgainSlate = PlayAgainButton->TakeWidget();
      if (PlayAgainSlate->SupportsKeyboardFocus())
      {
          InputMode.SetWidgetToFocus(PlayAgainSlate);      // :2312
      }
      ...
  ```
  ⇒ the argument chain is `Btn_Jump` → `GetWidgetFromName` → `TakeWidget()`. ⛔ **`VictoryWidget->TakeWidget()` count = `0`** — the RULING 1(b) blocker shape is **absent**. ✅
- **Null-safety (1)'s blocker clause:** lookup failure ⇒ `Warning` naming the widget sought, **no focus target set**, **never dereferenced**, ⛔ **never falls back to the root**. ✅
- **Four/five posture lines byte-identical:** quoted at §7(c). ✅
- **Route K-2 asset hashes:** ⛔ **N/A FOR MY DIFF — no `.uasset` written.** The pre-image is recorded at §3 (`7817dd7f…2bd6ee`, 153,489 bytes) **so his save can be measured against it.**

### (7b) RUNTIME — for the `playtest-verifier`

- **`UNOBSERVABLE` is pre-authorised on the keypress half** (RULING 2, A6): `simulate_key_press` lands below Slate and `inject_input_action` has nothing to bind to on `L_Arena`. ⛔ **Do not re-trace that named dead end** (`VER-§5` cl. 5).
- **The discriminating half, as AMENDMENT B3 amended it — a NAMED PAIR:** at a PIE that **actually reached a match end on `L_Arena`**, read **which** widget holds Slate keyboard focus. **BEFORE/negative = `SViewport` / the game-viewport widget. AFTER/positive = the Play Again button (`Btn_Jump`) BY NAME.** `SViewport` at a real match end is a **`VERIFY-FAILED`, not an `UNOBSERVABLE`.**
- 🚨 **ORDERING, AND IT DECIDES WHAT A `VERIFY-FAILED` MEANS — READ THIS BEFORE VERIFYING:** ⛔ **If Jonathan has NOT yet flipped `Btn_Jump.IsFocusable` (§3), focus CANNOT land on the button and `SViewport` is the CORRECT and EXPECTED reading.** That is the code behaving exactly as designed, **not** a defect. ⇒ **5b must run AFTER his save, or its `SViewport` reading must be recorded as "asset half not yet landed" rather than as a `VERIFY-FAILED` against the code.** ⭐ **A run before his save is still worth taking as AMENDMENT B3's free positive control** — it proves the instrument discriminates on *this* screen.
- **And the regression line, which holds in BOTH orderings:** the engine string `InputMode:UIOnly - Attempting to focus Non-Focusable widget` must **still be 0**. §4's guard makes that structurally true before *and* after his save.

### (7c) 🧑 HIS HAND CHECK — verbatim, parenthesis intact, ⛔ **ONLY VALID AFTER §3's SAVE**

> ***"Play a match to the end on the arena, then — without touching the mouse at all — press Enter once: does the match actually restart (arena back, units gone, gold reset)? (A highlight or outline appearing on 'Play Again' is a NO — that is Slate's own navigation and it draws with every line of this feature dead. The screen just sitting there is a NO. And if you moved or clicked the mouse at any point the answer does not count, because the click already worked before this change.)"***

⛔ **Asking him this BEFORE he flips `Btn_Jump.IsFocusable` guarantees a "no" that means nothing.** His answer is recorded **alongside** the verdict, **never merged into it** (`VER-§8` cl. 4).

### (7d) THE REST

- **Compile:** ⛔ **NOT RUN BY ME — `TASK-1316` is the host and owns it.** `Result: Succeeded` must be read **FROM THE LOG**, never `$LASTEXITCODE` (`UE-§ exit-code-lies`).
  **Compile-risk notes for the host:** ⛔ **no new `#include` was needed and none was added** — `Blueprint/UserWidget.h` (already at `:5`) pulls in `Widgets/SWidget.h` at its `:14` and `Components/Widget.h` at its `:18`, so `UWidget`, `GetWidgetFromName` (`UserWidget.h:1426`) and `SWidget::SupportsKeyboardFocus` are all complete types already. **No deprecated API is touched** — I deliberately avoided `UButton::IsFocusable` (deprecated 5.2) and never include `Components/Button.h`.
- **Suite total:** ⛔ **baseline ±0** — **no test added, none removed** (§5, reconciled by name).
- **`git status` — only the named file is mine:**
  ```
   M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp   ← MINE
   M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1                                       ← TASK-1324's, NOT mine
  ?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1324-programmer.md                 ← TASK-1324's, NOT mine
  ```
  🚨 **HOST: COMMIT BY EXPLICIT PATHSPEC, never `-a`, and verify the COMMIT with `git show --stat HEAD`, never the index** (`SC-§102`; `UE-§ git-plugin-autostages-index` — the editor's `Provider=Git` auto-stages). `TASK-1324` is live in the tree on a file that is out of bounds to me.
- **Diff:** 1 file, **+64 / −6**. Line endings verified **7,493/7,493 CRLF, 0 LF-only** — no line-ending flip.
- **Every `.uasset` named with its sha256:** §3. **None written.**

---

## §9 — 🚨 WHAT THE GATE SHOULD SCRUTINISE HARDEST

1. ⛔ **THE BUTTON IS NAMED `Btn_Jump`, NOT `Btn_PlayAgain`.** Verify it yourself from the asset — this is the single fact everything else rests on, and the name gives no hint of its function.
2. ⛔ **THE ROUTE IS K-2 AND 🧑 JONATHAN IS NEEDED.** Verify `IsFocusable=False` on `Btn_Jump` yourself; if you read `True`, my route choice is wrong and the row is K-1.
3. 🚨 **RULE EXPLICITLY ON §4's UNORDERED GUARD** (`SC-§121` cl. 5). Without it the diff regresses `TASK-1311` at every match end until his save. I believe a bounce here would be the expensive error (`SC-§59` cl. 5), but it is the gate's call, not mine.
4. ⛔ **THE FEATURE IS HALF-LANDED BY DESIGN AND I HAVE SAID SO EVERYWHERE.** This is **not** the `TASK-1286` inert-feature shape — the entry path is **measured live** at (0)(e), and the remaining gap is **one declared, named, human property flip**, not a dead lane. Check that distinction rather than pattern-matching on "doesn't work yet".
5. ⛔ **`SetWidgetToFocus` appears EXACTLY ONCE** and I kept it out of every comment deliberately (`TASK-1312` NOTE-2's trap).
6. ⚠️ **§6(d)'s `Escape`-is-`Back` observation** — engine default, nothing authored by us, but worth your ruling.
7. ⚠️ **The `Warning` severity** in the not-focusable branch fires once per match end until his save (§4).

---

## §10 — BOARD FLIP — ⛔ **I DID NOT MAKE IT, AND I AM SAYING SO PLAINLY**

⛔ **Fence (6e) puts `TASKBOARD.md` OUT OF BOUNDS to this row**, and `names:` repeats it (*"NEVER … `TASKBOARD.md`"*). ⇒ **I did not flip `TASK-1314`'s `status:` from `backlog` to `ready-for-qa`.** My dispatch asked me to update the board; **the fence and the law win** (`TL-§5e` cl. 7c), so I declined and am reporting it instead. 🧑 **The orchestrator must route the flip to the manager.**

Same reason: I wrote **nothing** to `CONVENTIONS.md`, though §6's census and (0)(a)'s name would both plausibly earn register entries. **That is the manager's file alone** (`SC-§82`).
