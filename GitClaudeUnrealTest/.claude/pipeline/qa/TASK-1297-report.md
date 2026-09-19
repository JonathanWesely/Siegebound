# QA Report — TASK-1297
Verdict: **PASS** — the diff is correct, complete and coupled. **0 BLOCKERS against the diff.**
⛔ **CONDITIONAL: 1 BLOCKER-GRADE STOP CONDITION owned by `TASK-1298` / 🧑 Jonathan, not by the programmer.** This PASS covers the **text + asset state** of `TASK-1296`. It is **VOID as authorisation to run the suite or commit** until §B below is discharged. A reader who takes "PASS" as "the wave may proceed" has misread it.

subject: TASK-1296
reviewer: qa-reviewer · 2026-09-18 · marker `TASK-1297-MENU-FOCUS-GATE`
⛔ No code, asset, test or setting was edited by this review. ⛔ No compile, no Git, no PIE, no editor-lifecycle tool. Engine editor PID observed live (GUI, MCP `:8000`); every measurement below is **read-only**.

---

## §A — THE COUPLING (check one), and it is **SATISFIED IN BOTH DIRECTIONS**

The pin's own comment left the instruction in the file (quoted from `handoffs/TASK-1274-programmer.md:109-113`, which authored it):

> **"when the Blueprint is fixed (the owed follow-up row) THIS LINE GOES WITH IT."**

and the call it guarded:

```cpp
AddExpectedError(TEXT("InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget"),
    EAutomationExpectedErrorFlags::Contains, /*Occurrences*/ 1);
```

`Occurrences` is pinned to **exactly 1**, and `AutomationTest.cpp:1808-1865` (`ExpectedNumberOfOccurrences > 0 && != Actual` ⇒ fail) makes the coupling **total in both directions** — the test reds on **0 or 2+**. Half a fix is a red build either way.

| half | state | how I measured it |
|---|---|---|
| the Blueprint fix | ✅ **present** | `get_asset_graph /Game/Blueprints/BP_MenuGameMode.BP_MenuGameMode EventGraph` — `CreateWidget (K2Node_CreateWidget_3)` now emits **only** `` `Return Value`->Target(K2Node_CallFunction_10) ``. The `` `Return Value`->`In Widget to Focus`(K2Node_CallFunction_11) `` link is **gone**. |
| the pin deletion | ✅ **present** | `Grep 'AddExpectedError\|SetSuppressLogErrors\|AddExpectedMessage' Source/` ⇒ **0 hits in `SiegeMenuInputTest.cpp`**, 0 `SetSuppressLogErrors` anywhere in the project. |
| same working tree | ✅ **both modified, neither staged** | the session's own `git status` snapshot lists `M Content/Blueprints/BP_MenuGameMode.uasset` **and** `M Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp` together — independent of the handoff's quoted porcelain. |

**Exec chain intact** (same read): `ReceiveBeginPlay → VariableSet(Show Mouse Cursor=true) → CreateWidget(WBP_MainMenu_C) → AddToViewport(ZOrder 0) → SetInputMode_UIOnlyEx(In Mouse Lock Mode=Do Not Lock, Flush Input=false)`. ⛔ No node added, removed or re-defaulted. `GetPlayerController`'s three wires unchanged.

**No suppression substituted.** The other `AddExpectedMessagePlain` hits in the grep are pre-existing, in **other** test files (`SiegeDeckSlotsTest.cpp`, `SiegeAssistantSelectionTest.cpp`, …), untouched by this diff. The absence of the Error is now asserted for free by the framework redding on an undeclared `Error`.

---

## §B — ⛔ THE STOP CONDITION — **BLOCKER-grade, and it is the host's to discharge, not the programmer's**

**[BLOCKER — owner: `TASK-1298` / 🧑 Jonathan] `BP_MenuGameMode`'s generated class is NOT recompiled.**
Measured **by me at review time**, not accepted as declared — `execute_unreal_python_readonly`:

```
BP_MenuGameMode Status = <BlueprintStatus.BS_DIRTY: 1>
dirty content packages = 0 []      dirty map packages = 0 []
Blueprints/BP_MenuGameMode.uasset  size=41634  sha256=fd9817f004a5f476211a47ca2c47dc43926cdf15f7a20f0c67e4bf2266954962
```

⇒ the graph edit is **on disk** (dirty census 0 ⇒ disk matches memory) but the class was compiled from the **old** graphs. What executes is bytecode. Uncompiled, this diff is **red-making**, exactly as its author said.

Every link of the programmer's argument verified at source, and **all four land exactly on the cited line**:

| claim | verified |
|---|---|
| `UBlueprint::Status` is not serialized ⇒ reloads as `BS_Unknown` | `Blueprint.h:507` — `UPROPERTY(transient, Category=BlueprintOptions, BlueprintReadOnly)` on `Status` (`:508`) ✅ |
| a reloaded BP is not skipped by the sweep's earlier guards | `Blueprint.h:786-789` `IsUpToDate()` = `BS_UpToDate\|\|BS_UpToDateWithWarnings` (false for `BS_Unknown`); `:791-794` `IsPossiblyDirty()` = `BS_Dirty\|\|BS_Unknown` (**true**) ✅ |
| …so only the explicit guard excludes it | `PlayLevel.cpp:1290` — `&& (Blueprint->Status != BS_Unknown)` ✅ |
| the modal exists | `PlayLevel.cpp:1311` — *"One or more blueprints have been modified without being recompiled"*, `FMessageDialog::Open(EAppMsgType::YesNo, …)` at `:1313` ✅ |

### ⭐ TWO CORRECTIONS THE HOST NEEDS — the named hazard is the wrong one, and the unnamed one is worse

**[WARN] The modal will NOT fire in this project.** `PlayLevel.cpp:2656` calls `ResolveDirtyBlueprints(!EditorPlaySettings->AutoRecompileBlueprints, …)`, and `bAutoCompile = !bPromptForCompile` (`:1261`). Measured: **no project override exists** — I walked `Config/`, `Saved/Config/WindowsEditor/`, `Saved/Config/Windows/` (`found any AutoRecompileBlueprints override: False`) — so the engine default applies: `Engine/Config/BaseEditorPerProjectUserSettings.ini:242` → **`AutoRecompileBlueprints=True`** ⇒ `bPromptForCompile = false` ⇒ **no dialog**. The handoff hedged this correctly (*"if `bPromptForCompile` is on"*); the board's `status:` line dropped the "if" and states the hang as a live hazard. A host preparing for a modal is preparing for the wrong failure.

**[BLOCKER — the real one, named nowhere] In the LIVE editor the opposite happens, and it is a FALSE GREEN that ships a broken asset.** With `bAutoCompile = true` and `Status = BS_DIRTY` (≠ `BS_Unknown`, so the `:1290` guard does **not** exclude it), PIE will **silently auto-recompile `BP_MenuGameMode` in memory** at suite start. ⇒ `TASK-1298`'s suite can go **GREEN** while the `.uasset` on disk still carries the **stale, uncompiled class**. A recompile marks the package dirty in memory; under this project's never-save posture nothing writes it back. **A commit by pathspec would then ship `fd9817f0…954962` — the uncompiled asset — behind a green suite.**

### ⇒ What the host must confirm, in this order, before anything is authoritative (`SC-§124` — the invariant, not just the predicate)

**The invariant: the class that the committed `.uasset` contains must be the class compiled from the fixed graph.** The predicate that checks it:

1. Compile `/Game/Blueprints/BP_MenuGameMode` **and save it**. (⛔ Not mine and not the programmer's: their `compile_blueprint` / `execute_unreal_python` were refused by the session classifier — *[Modify Shared Resources]* — and they correctly stopped rather than probing for a tool that slips through. That refusal is 🧑 Jonathan's to resolve.)
2. ⛔ **Re-record the sha256, and require it to have CHANGED from `fd9817f004a5f476211a47ca2c47dc43926cdf15f7a20f0c67e4bf2266954962`.** A post-compile hash **equal** to that value means the compile did not land on disk — ⛔ that is a STOP, irrespective of a green suite. This is the one machine-checkable gate that discriminates the false green above.
3. Only then run the suite ×3, and only then read acceptance (3)/(4) as evidence. A green from a PIE that auto-compiled in memory proves the **fix**, not the **artifact**.
4. ⛔ Do **not** rely on an editor relaunch to rescue it — measured above, it will not (and after a relaunch the suite would legitimately go **red**, which is the honest failure, not the dangerous one).

---

## §C — ⚖️ THE RULING ON THE ROUTE DEVIATION (B′): **the enumeration did NOT bind. B′ is correct.**

The programmer took **B′** — point `WidgetToFocus` at **nothing** — and **flagged it rather than hiding it**, citing `SC-§58`. I rule the deviation **justified**, and per `SC-§97` the test is not that I agree but that **the enumerated routes are measurably wrong**. They are. I checked both at source, and the strongest one against the live asset:

**Route (B) literal is refuted by the asset itself — the widget path the spec names does not exist.** The row says *"the first `Button_*` of `Overlay_19/VerticalBox_0`"*. I read the authored tree (`get_asset_meta /Game/UI/WBP_MainMenu WidgetTree`): it is **`Overlay_19 → SizeBox_0 → Btn_Jump`** plus `UI_Thumbstick_C ×2` — **there is no `VerticalBox_0` and there are no menu buttons in it.** `get_asset_graph … Construct` shows why: `GenericCreateObject(Class=VerticalBox)` → `AddChildToOverlay`, then seven runtime-built buttons (`Play (vs Bot)` · Sandbox via `BuildSandboxButton` · `Deck Builder` · `Multiplayer` · `Settings` · `Login` · `Quit`, each `GenericCreateObject(Class=Button)` → `AddChildToVerticalBox`). ⇒ **a design-time pin in `BP_MenuGameMode` cannot reference an object that does not exist until `Construct` runs.** An enumerated branch that is unconstructible cannot bind. (The third conceivable route, `UUserWidget::DesiredFocusWidget`, dies the same way — it resolves an `FWidgetChild` **by name from the authored tree**, and the buttons are not in it. Read back as `DesiredFocusWidget = ()`.)

**Route (B) literal's second refutation — the silent regression — holds, and it is the strongest argument in the report.** Chain, each link read at source:
- `FInputModeDataBase::SetFocusAndLocking` → `SlateOperations.SetUserFocus(InWidgetToFocus.ToSharedRef())` — **no cause argument** (`PlayerController.cpp`, the `FInputModeUIOnly::ApplyInputMode` caller below it).
- default cause is `EFocusCause::SetDirectly` — `Reply.h:51`, `SetUserFocus(TSharedRef<SWidget>, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly, bool bInAllUsers = false)`. ✅ exact line.
- `ShowFocus = InCause == EFocusCause::Navigation;` — `SlateApplication.cpp:3099`. ✅ exact line ⇒ **`SetDirectly` paints no focus brush.**
- and because a button would then hold focus, `ApplyInitialFocus`'s guard `if (Buttons.Num() > 0 && !GetFocusedMenuButton())` (`SiegeMenuInputSubsystem.cpp:368`) goes **false** ⇒ the `Navigation`-cause focus that *draws* the rectangle (`:397`) never runs.
- **Corroborated by the downstream feature's own shipped words** — `handoffs/TASK-1274-programmer.md:49`: *"`UWidget::SetUserFocus` was NOT used because it passes `SetDirectly`, **which paints nothing**."* ⇒ (B) literal would have silently regressed `TASK-1274` deliverable (3), the boot focus brush.

**[NIT] One precision, and it cuts *for* the ruling, not against.** The handoff states that regression as certain. Strictly it is **ordering-dependent**: `SlateOperations` is a deferred `FReply`, so whether the button holds focus before the next-tick `ApplyInitialFocus` is a race. That makes (B) literal *worse*, not better — a boot-path defect that passes review and fails intermittently in front of Jonathan. Conclusion unchanged.

**Route (A) is refuted structurally — it grants the bad request instead of removing it.** `FSlateApplication::SetUserFocusPath` iterates `for (int32 WidgetIndex = InFocusPath.Widgets.Num() - 1; WidgetIndex >= 0; --WidgetIndex)` and takes the first widget that `SupportsKeyboardFocus()` (`SlateApplication.cpp:3016-3030`) — the path runs root→target, so this walks **upward from the target**; it never descends to a child. ⇒ making the root focusable makes **the container** the focused widget at boot — a state `GetFocusedMenuButton()` can never return (it returns a `UButton`) and in which `HandleMenuAccept` deliberately no-ops (`SiegeMenuInputSubsystem.cpp:341-347`, read: *"Cold Accept does nothing, deliberately"*). Plus it edits `WBP_MainMenu`, the widget class this project has broken ~9 times, for no benefit.

**Why B′ is the right shape, in the project's own terms:** the Error is the engine saying *"you asked me to focus something that cannot be focused"*, and this project **already has a correct focus owner** — `ApplyInitialFocus`, armed next-tick at `SiegeMenuInputSubsystem.cpp:116`, focusing `Buttons[0]` with `EFocusCause::Navigation` at `:397`. The pin was a **second, wrong author of the same state**. B′ deletes the wrong author; (A) and (B) would have kept two. The undo is one wire and (A) remains one property flip away.

---

## §D — ⚖️ THE RULING ON THE DECLARED-UNMEASURED COLD-STATE FOCUS INDEX: **not depended upon ⇒ WARN, not BLOCKER**

The dispatch is right that *a declared-unmeasured premise the fix DEPENDS on is a BLOCKER*. This one is **not depended upon**, on three independent grounds:

1. **The test tolerates both outcomes.** `SiegeMenuInputTest.cpp:142` — `TestTrue(TEXT("cold precondition: no menu button focused, or index 0"), FocusedIndex == INDEX_NONE || FocusedIndex == 0);`. Neither value can red the test.
2. **The prediction is *derived from code I verified*, not assumed.** The removed request could only ever resolve to an **ancestor** of the menu root (`SlateApplication.cpp:3016-3030`, upward walk) — never a descendant button — so `GetFocusedMenuButton()` was null at that instant **before** the fix and is null **after** it ⇒ `ApplyInitialFocus`'s guard (`:368`) receives identical input and focuses `Buttons[0]` either way. `GetMenuButtons` (`SiegeMenuInputSubsystem.cpp:201-220`) walks `ForEachWidget` in slot order top-to-bottom, so `Buttons[0]` is the topmost = `"Play (vs Bot)"` — index 0 of 7, unchanged.
3. **The genuinely unmeasured thing — the runtime outcome — was not claimed; it was assigned.** The handoff declares plainly that it did not run PIE and that acceptance (3)/(4) belong to the verify leg. That is the correct disposition, not a laundered caveat. The single behavioural delta is that one deferred `SetUserFocus(<menu root>, SetDirectly)` request is no longer issued.

⇒ **[WARN]** the re-measurement is genuinely owed, it is already boarded on `TASK-1298` cl. (5), and it must not be skipped — but it does not block this gate.

---

## §E — REACHABILITY CENSUS (`SC-§123`) — **reproduced by me, not accepted as declared**

ripgrep skips binary `.uasset`, so I reproduced `grep -rl` by reading package bytes (`execute_unreal_python_readonly`, `os.walk` over `Content/`, needle search):

```
packages scanned: 4402
SetInputMode_UIOnlyEx  -> 1 package(s): ['Blueprints\BP_MenuGameMode.uasset']
SetInputMode_UIOnly    -> 1 package(s): ['Blueprints\BP_MenuGameMode.uasset']
SetInputMode_GameAndUIEx -> 4 package(s): UI_TouchSimple, UI_TouchInterface_Combat/_Platforming/_SideScrolling
SetInputMode_GameOnly  -> 0
```

⇒ **exactly one emitter in `Content/`**, and the four `GameAndUIEx` hits are the stock touch-interface assets, none on `L_MainMenu`. The engine string exists at **one** site, `PlayerController.cpp:6345`, inside `#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)`. The cause chain closes: `SObjectWidget::SupportsKeyboardFocus()` (`SObjectWidget.cpp:175`) → `WidgetObject->NativeSupportsKeyboardFocus()` → `bIsFocusable`, and `WBP_MainMenu`'s CDO reads **`bIsFocusable = False`** (my read-back). ⇒ on `L_MainMenu` `BP_MenuGameMode` was the **single** emitter.

**[WARN — out of this row's scope, correctly untouched, and I have now SETTLED it for the manager]** The look-alike at `SiegePlayerController.cpp:2264-2270`:
```cpp
FInputModeUIOnly InputMode;
if (VictoryWidget) { InputMode.SetWidgetToFocus(VictoryWidget->TakeWidget()); }
```
The handoff said *"nothing in this row's scope proves it either way."* One read-only call proves it: **`WBP_VictoryScreen`'s CDO is `bIsFocusable = False`**, identical to `WBP_MainMenu`. ⇒ **the match-end victory screen on `L_Arena` emits the identical engine Error, today, unpinned.** Out of scope here (different map; this test never ends a match) — ⛔ but it is a **confirmed** second emitter and wants its own row, and any future automation test that ends a match inside a capture window will red on an undeclared `Error` with no pin to absorb it.

---

## §F — THE TEST FILE IS OTHERWISE FROZEN — verified against what `TASK-1274` shipped, not just against a hunk count

Declared: 1 hunk `@@ -114,21 +114,6 @@`, **15 deletions / 0 insertions** (12-line comment + 2-line call + 1 blank = 15 ✓ arithmetic consistent, and consistent with the pin having sat at `:117-130` before `AutomationOpenMap`). I cannot re-run `git diff` (this gate forbids `Bash`), so I verified the **end state** against `handoffs/TASK-1274-programmer.md:53` instead — every assertion it enumerates is present and intact:

| shipped by TASK-1274 | present now |
|---|---|
| test name `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` | `:110` ✅ |
| flags `EditorContext \| EngineFilter` | `:111` ✅ |
| `AutomationOpenMap("/Game/Maps/L_MainMenu")` | `:118` ✅ |
| `IsArmed()` · live `WBP_MainMenu` · ≥3 buttons | `:129` · `:130` · `:134` ✅ |
| cold precondition · 0 builders before input | `:142` · `:143` ✅ |
| label `"Deck Builder"` after Down ×2 | `:168-169` ✅ |
| exactly one `UDeckBuilderWidget` after Accept | `:183` ✅ |
| wrap ring 0+2→2 · 0−1→6 · 6+1→0 · count 0→`INDEX_NONE` | `:190-193` ✅ |
| waits 1.0 / 0.25 / 0.25 / 0.5 · `FExitGameCommand()` | `:121,153,159,180` · `:198` ✅ |

**[NIT]** The deletion leaves no orphaned include — the removed call was a `FAutomationTestBase` method from `Misc/AutomationTest.h` (`:3`), still required by `IMPLEMENT_SIMPLE_AUTOMATION_TEST`. No include became dead.

---

## §G — `VER-§8` CEILING: **CONFIRMED — this row does NOT hit it. A `VERIFY-FAILED` here would be a real finding, not an `UNOBSERVABLE`.**

Acceptance (3) is (i) a **log-line count** — a read, not an input — and (ii) the `TASK-1274` recipe through `inject_input_action` → `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction`, the same door `SiegeMenuInputTest.cpp:86` uses. `VER-§8` cl. 1's **own control pair settles it in this row's favour**: *"`Tab` (Slate's own `Next` key, `binding_found: false`) moved focus NOT AT ALL, while `Down` (`binding_found: true`, `IA_MenuDown`) DID move it."* — `IA_MenuDown` **is** this feature, on this map. The ceiling is Slate-level keys and UMG clicks; this row needs neither. Accept path is `Focused->OnClicked.Broadcast()` in **our** C++ (`SiegeMenuInputSubsystem.cpp:356`), not a Slate click. ⇒ ⛔ **the verify leg may not return `UNOBSERVABLE` on a ceiling that does not apply here.**

⛔ **The ceiling IS real for 🧑 his hands, and the handoff is right to say so:** `FInputModeUIOnly::ApplyInputMode` calls `GameViewportClient.SetIgnoreInput(true)` — read at source ⇒ on `L_MainMenu` a real key press does not reach Enhanced Input at all. **Nobody may ask Jonathan to press Down/Enter on that map.**

**The pre-written hand check discriminates — I judge it sound.** *"Boot to the main menu and, before touching the mouse or keyboard, look at the top button 'Play (vs Bot)' — is there a focus outline drawn on it? (A highlight that appears only when the mouse is over the button is a NO — move the pointer off the menu first; and an outline that appears only after you click something is a NO.)"*
- It asks for the effect **only our code produces**: the dashed `FocusRectangle` brush, drawn only because `ShowFocus` is true, which is true only for `EFocusCause::Navigation`, which only `FocusButton` (`:397`) passes.
- The parenthesis is load-bearing and correct: `SButton`'s hover brush and a click-induced focus are the two look-alikes, both excluded.
- It targets the **right** button: `GetMenuButtons` (`:201-220`) walks slot order top-to-bottom, so `Buttons[0]` = topmost = `"Play (vs Bot)"`.
- **[NIT]** It is honestly scoped — it covers the only human-visible **risk** (a focus-brush regression), not the thing the row **fixes** (an engine Error, invisible to a human). That asymmetry should be said out loud when it is handed to him, so a "yes" is not read as confirming the fix.

---

## §H — FENCES AND `.uasset` DISCIPLINE — all clear, and independently corroborated

| fence | result | source |
|---|---|---|
| `IMC_MainMenu` / `IA_MenuUp` / `IA_MenuDown` / `IA_MenuAccept` diff = 0 | ✅ | absent from the session's own `git status` snapshot |
| no button's `OnClicked` body changed | ✅ | `WBP_MainMenu` not modified at all (below) |
| no WidgetBlueprint duplicated-and-reparented | ✅ | the widget was **not written**; the edit was one `disconnect_blueprint_nodes` on the GameMode |
| `Saved/**` absent · `CONVENTIONS.md` (`KBD-§` tables) diff = 0 | ✅ | absent from the `git status` snapshot |
| `.uasset` named with sha256 (`SC-§68`) | ✅ **and it matches disk, measured by me** | `BP_MenuGameMode.uasset` = `fd9817f004a5f476211a47ca2c47dc43926cdf15f7a20f0c67e4bf2266954962` (declared = measured) |
| `WBP_MainMenu.uasset` untouched | ✅ **two ways** | measured sha256 = `03cc0a395ebe072d4248f6429027622a9feccb99752ea8d5f3cd35da42564ae3` — **exactly the declared value** — **and** the file is absent from the `git status` modified list |
| working tree = only the named files | ✅ | snapshot shows the two subject files + the two sibling lanes (`DeckBuilderWidget.*` = TASK-1307, `run_suite_bounded.ps1` = TASK-1294) — ⛔ **no cross-lane contamination**, `SC-§102` clean |

⛔ Note for the host: `fd9817f0…954962` is the **pre-compile** hash and **must not survive** the compile (§B step 2).

---

## Findings

- **[BLOCKER — owner `TASK-1298`/🧑, not the programmer]** `Content/Blueprints/BP_MenuGameMode.uasset` — generated class is `BS_DIRTY`, saved but never recompiled; uncompiled, the stale class still emits the Error and, with the pin now gone, `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` **reds**. — **Fix:** §B steps 1-4; the machine-checkable gate is that the sha256 must **change** from `fd9817f0…954962`. *(measured: `execute_unreal_python_readonly` → `Status = BS_DIRTY`, `dirty content packages = 0`, sha256 read from disk)*
- **[BLOCKER — unnamed anywhere until now, same owner]** `PlayLevel.cpp:1290` + `:2656` + `BaseEditorPerProjectUserSettings.ini:242` — with `AutoRecompileBlueprints=True` and no project override, the **live** editor will silently auto-recompile at PIE start ⇒ **a green suite over a stale committed `.uasset`**. — **Fix:** require the post-compile hash change *before* trusting the suite; do not treat green as proof of the artifact. *(measured: engine ini walk + `Config/`+`Saved/Config/` walk ⇒ `found any AutoRecompileBlueprints override: False`)*
- **[WARN]** `TASKBOARD.md:3439` (`TASK-1296` `status:`) — states the pre-PIE **modal** as a live hang hazard; in this project `bPromptForCompile = false`, so it will not fire. The handoff hedged correctly; the board line dropped the hedge. Not mine to edit beyond the status flip — flagged for the row's owner. *(measured as above)*
- **[WARN]** `SiegePlayerController.cpp:2264-2270` — confirmed second emitter: `WBP_VictoryScreen` CDO `bIsFocusable = False` ⇒ the identical engine Error fires at match end on `L_Arena`, unpinned. Out of this row's scope and correctly untouched; **wants its own board row.** *(measured: `get_asset_meta /Game/UI/WBP_VictoryScreen PropertyValues`)*
- **[WARN]** cold-state focus index (`index 0 of 7`) is **predicted from source, not re-measured** — acceptable here (§D: not depended upon; the test accepts `INDEX_NONE` **or** `0`), but `TASK-1298` cl. (5)'s verify leg still owes the measurement.
- **[NIT]** `handoffs/TASK-1296-programmer.md:69` — the (B)-literal focus-brush regression is stated as certain; it is ordering-dependent (deferred `FReply`). This strengthens the rejection, it does not weaken it.
- **[NIT]** the hand check covers the row's **risk**, not its **fix**; say so when handing it to Jonathan so a "yes" is not over-read.
- ✅ **No finding against the diff itself.** Route measured before chosen (`SC-§101`): property values quoted and independently re-read by me. Deprecation census (`SC-§122`) accurate and moot — this diff adds **no new C++ call at all**.

## Notes for build-master (`TASK-1298`)

1. ⛔ **Do not start the suite until `/Game/Blueprints/BP_MenuGameMode` is compiled AND saved, and the `.uasset` sha256 has CHANGED from `fd9817f004a5f476211a47ca2c47dc43926cdf15f7a20f0c67e4bf2266954962`.** An unchanged hash = the compile did not land = STOP, even if the suite is green.
2. The two halves **ride or bounce together** — `BP_MenuGameMode.uasset` and `SiegeMenuInputTest.cpp`. ⛔ Committing either alone reds the suite. Commit by exact pathspec; the working tree also holds `TASK-1294`'s and `TASK-1307`'s files — do not sweep them.
3. Suite expectation: total = baseline (this diff adds/removes **no test**), and `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` **GREEN with the pin gone** ⇒ that green *is* the proof the Error is gone.
4. The verify leg (`playtest-verifier`, acceptance (3)) is **in reach** — ⛔ not `UNOBSERVABLE` (§G). Its log read must come from a PIE that ran **after** the compile.
5. 🧑 Jonathan's one sentence stands as written (§G) — and ⛔ he must **not** be asked to press Down/Enter on `L_MainMenu`: `SetIgnoreInput(true)` makes that impossible regardless of this row.
