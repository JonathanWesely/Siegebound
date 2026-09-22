# TASK-1296 — [MENU-FOCUS-NONFOCUSABLE] — gameplay-programmer handoff

**Marker:** `TASK-1296-MENU-FOCUS-NONFOCUSABLE` · **Status set:** `ready-for-qa` · **Gate:** `TASK-1297` · **Host:** `TASK-1298`
**Date:** 2026-09-18 · Editor PID 1812 (GUI, MCP `:8000`) live throughout; ⛔ no C++ compile, ⛔ no Git run, ⛔ no other row touched.

---

## 🚨⛔⛔ BLOCKER FIRST — READ BEFORE ANYTHING ELSE. THE DIFF IS COMPLETE; **ONE EDITOR ACTION IS OWED AND IT IS NOT OPTIONAL.**

**`BP_MenuGameMode` IS SAVED WITH THE FIXED GRAPH BUT ITS GENERATED CLASS IS ⛔ NOT RECOMPILED (`Status = BS_DIRTY`, read back after the save).**
**Somebody with the permission must press Compile on `/Game/Blueprints/BP_MenuGameMode` (or call `compile_blueprint`, which also saves) ⛔ BEFORE `TASK-1298` runs the suite.**

* **Why I could not:** `mcp__unreal_editor__compile_blueprint` **and** `mcp__unreal_editor__execute_unreal_python` were both refused by the session's auto-mode classifier — *"Permission for this action was denied … Reason: [Modify Shared Resources]"*. `disconnect_blueprint_nodes` was **allowed** and it wrote the package to disk by itself, so the graph edit landed while the compile did not. I stopped probing for a mutation tool that would slip through rather than work around the denial.
* **Why it is a BLOCKER and not a caveat (`SC-§123` cl. 5 — a declared-unmeasured premise the fix DEPENDS on):** what executes at runtime is the **bytecode**, not the graph. `BS_DIRTY` means, literally, the graphs changed after the class was compiled ⇒ **the saved class still pushes `WBP_MainMenu` into `SetInputMode_UIOnlyEx`** ⇒ the Error still fires ⇒ **and the pin that used to absorb it is gone in this same diff ⇒ `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` REDS.** Uncompiled, this diff is not inert — it is red-making.
* **⛔ AND A RELAUNCH DOES NOT SAVE US — MEASURED, NOT ASSUMED.** `UBlueprint::Status` is `UPROPERTY(transient)` (`Blueprint.h:507`) ⇒ it is **not serialized**, so after a fresh editor launch the blueprint comes back as `BS_Unknown`. The pre-PIE auto-recompile sweep at `PlayLevel.cpp:1270–1300` collects `IsPossiblyDirty()` blueprints but is **explicitly guarded by `&& (Blueprint->Status != BS_Unknown)`** ⇒ **a relaunched editor will NOT recompile it and will believe it is fine.** Do not count on compile-on-load.
* **⚠️ SECOND HAZARD FOR THE HOST, while it is still dirty in the LIVE editor:** the same sweep runs on every PIE start, and if `bPromptForCompile` is on it opens a **modal Yes/No dialog** (`PlayLevel.cpp:1311`, *"One or more blueprints have been modified without being recompiled"*). A suite run started against the current live editor could **hang on that modal**. Compiling discharges this too.
* **Cost to discharge:** one call / one button. Afterwards `BP_MenuGameMode.uasset`'s sha256 changes again — **re-record it**; the hash below is the pre-compile one.

---

## (0) MEASURED FIRST — the property reads, quoted, and the route named with its reason

### What I read (all read-only: `unreal_inspector`, plus engine source at `UE_5.8/Engine/Source`)

**`BP_MenuGameMode` EventGraph, before the edit (verbatim from `get_asset_graph`):**

```
Node: Event: ReceiveBeginPlay            -> (K2Node_VariableSet_2)
Node: VariableSet (K2Node_VariableSet_2)  `Show Mouse Cursor`=true   -> (K2Node_CreateWidget_3)
Node: CreateWidget (K2Node_CreateWidget_3) `Class`=WBP_MainMenu_C
      `Return Value`->Target(K2Node_CallFunction_10)
      `Return Value`->`In Widget to Focus`(K2Node_CallFunction_11)      <-- THE DEFECT
Node: Function Call: AddToViewport (K2Node_CallFunction_10) `ZOrder`=0  -> (K2Node_CallFunction_11)
Node: Function Call: SetInputMode_UIOnlyEx (K2Node_CallFunction_11)
      `In Mouse Lock Mode`=Do Not Lock   `Flush Input`=false
```

**`WBP_MainMenu_C` CDO property values (quoted from the read-back):**

| property | value |
|---|---|
| `bIsFocusable` | **`False`** |
| `Visibility` | `SlateVisibility.SELF_HIT_TEST_INVISIBLE` |
| `bStopAction` | `False` |
| `Priority` | `0` |
| parent class | `UserWidget` (plain — no project C++ parent) |

**The emission chain, closed end to end at source (no step inferred):**

1. `UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx` (`UMG/Private/WidgetBlueprintLibrary.cpp`) — **`if (InWidgetToFocus != nullptr) { InputMode.SetWidgetToFocus(InWidgetToFocus->TakeWidget()); }`** ⇒ the call is **guarded by a null check**.
2. `FInputModeUIOnly::SetWidgetToFocus` (`Engine/Private/PlayerController.cpp:6343–6347`) — `#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)` + `if (InWidgetToFocus.IsValid() && !InWidgetToFocus->SupportsKeyboardFocus())` ⇒ `UE_LOGF(LogPlayerController, Error, "InputMode:UIOnly - Attempting to focus Non-Focusable widget %ls!", …)`.
3. `SObjectWidget::SupportsKeyboardFocus()` (`UMG/Private/Slate/SObjectWidget.cpp:175`) ⇒ `return WidgetObject->NativeSupportsKeyboardFocus();` ⇒ `UUserWidget::NativeSupportsKeyboardFocus()` ⇒ `return bIsFocusable;` ⇒ **`False`** ⇒ the Error. **The string exists at EXACTLY ONE site in the whole engine** (`grep -rn` over `UE_5.8/Engine/Source` = 1 hit, `PlayerController.cpp:6345`).

### ✅ THE ROUTE TAKEN — **(B′): delete the `In Widget to Focus` wire in `BP_MenuGameMode`. `WBP_MainMenu` is not touched at all.**

This is the **(B) branch** of the row's fork — *leave the widget non-focusable, change the caller* — **taken to its limit: point `WidgetToFocus` at nothing rather than at another widget.** ⛔ **I am flagging the deviation from the row's literal enumeration explicitly for the gate (`SC-§58` etiquette: take the narrower/safer action, report the disagreement), because literal (B) is refuted BY MEASUREMENT below, and the deviation's undo is one wire.**

**Why this is right, in one line:** the Error is the engine saying *"you asked me to focus something that cannot be focused"* — and **this project already has a focus owner that does the job correctly**: `USiegeMenuInputSubsystem::ApplyInitialFocus` (`SiegeMenuInputSubsystem.cpp:359`), armed next-tick at `:116`, which focuses `Buttons[0]` through `FSlateApplication::SetUserFocus(UserIndex, SlateButton, EFocusCause::Navigation)` (`:397`). The `WidgetToFocus` pin was a **second, wrong author of the same state**. The fix deletes the wrong one.

### ⛔ WHY THE OTHER TWO ROUTES ARE WRONG — measured, not preferred

**Route (A) — make `WBP_MainMenu` focusable — REJECTED.**
* It does silence the Error (`SupportsKeyboardFocus()` would return `true`), but it **grants the request instead of removing it**: `FInputModeDataBase::SetFocusAndLocking` (`PlayerController.cpp:6313–6316`) then issues `SlateOperations.SetUserFocus(<menu root>)` and **that focus now SUCCEEDS on the container**. `FSlateApplication::SetUserFocus` walks the path **from the target upward** to the first widget that `SupportsKeyboardFocus()` (`SlateApplication.cpp:3016–3030`) — it never walks *down* to a button — so route (A) makes **the first widget focused at boot a container that `GetFocusedMenuButton()` can never return**, i.e. exactly the state in which `HandleMenuAccept` deliberately does nothing (`SiegeMenuInputSubsystem.cpp:340–346`). Cold state then depends on whether that deferred reply resolves before or after the next-tick `ApplyInitialFocus` — **an ordering race introduced into the boot path of a shipped feature, for no benefit.**
* It is also the wrong object to change: the root's job is layout (`Visibility = SelfHitTestInvisible`), and ⚠️ this project has a documented history of breaking `WBP_MainMenu`-class widgets (~9 wasted fixes). **The widget's sha256 is unchanged, and that is deliberate.**
* (Route (A) remains available if the gate rules the row's enumeration binding: it is one property flip on `WBP_MainMenu`, and this diff would then revert cleanly. ⛔ `SetIsFocusable()` — not the deprecated `bIsFocusable` UPROPERTY — would be the C++ spelling; see the deprecation census below.)

**Route (B) literal — point `WidgetToFocus` at the first `Button_*` — REJECTED, and the measurement kills it twice.**
1. ⛔ **There is no button to point at.** `WBP_MainMenu`'s **authored** WidgetTree is `Overlay_19 → SizeBox_0 → Btn_Jump` plus two `UI_Thumbstick_C` — mobile-overlay leftovers. **`VerticalBox_0` and all seven menu buttons are created at RUNTIME** by the `Construct` event via `BuildSandboxButton(UVerticalBox* ParentBox)`. The GameMode would have to reach into a runtime-built tree and cast — graph surgery in a boot asset, the largest of the three diffs.
2. ⛔ **It would silently kill the visible focus rectangle.** `SetFocusAndLocking` calls `SlateOperations.SetUserFocus(Widget)` with the **default** cause — `FReply::SetUserFocus(…, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly, …)` (`SlateCore/Public/Input/Reply.h:51`) — and `ShowFocus = InCause == EFocusCause::Navigation` (`SlateApplication.cpp:3099`). So the button would be focused **with no focus brush**, AND because a button *is* then focused, `ApplyInitialFocus`'s guard `if (Buttons.Num() > 0 && !GetFocusedMenuButton())` goes **false**, so the `Navigation`-cause focus that draws the rectangle **never runs**. That is a regression of `TASK-1274`'s deliverable (3), and the subsystem's own comment at `:389–395` is about exactly this distinction.

---

## (1) THE FIX — the asset

**File:** `Content/Blueprints/BP_MenuGameMode.uasset`
**sha256 before:** `35b26cd779e16fea0f2b8f1b9e50fc8f92778fb17f03959c60b5c3e4dec82fb1`
**sha256 after (⛔ PRE-COMPILE — re-record after the owed compile):** `fd9817f004a5f476211a47ca2c47dc43926cdf15f7a20f0c67e4bf2266954962`

**Edit, in place, via `mcp__unreal_editor__disconnect_blueprint_nodes` (transaction `13B137B743F325DA6F601999D4AA28E1`):** removed the single link `K2Node_CreateWidget_3.'Return Value'` → `K2Node_CallFunction_11.'In Widget to Focus'`. ⛔ No node added, removed, renamed or re-defaulted; ⛔ no duplicate-and-reparent anywhere; ⛔ `IMC_MainMenu` / `IA_Menu*` untouched; ⛔ no `OnClicked` body touched.

**Read-back after the edit (`get_asset_graph`, verbatim):** the `CreateWidget` node now lists **only** `` `Return Value`->Target(K2Node_CallFunction_10) `` and the `SetInputMode_UIOnlyEx` node still reads `` `In Mouse Lock Mode`=Do Not Lock `` / `` `Flush Input`=false ``. The exec chain BeginPlay → `Show Mouse Cursor=true` → CreateWidget(`WBP_MainMenu_C`) → AddToViewport(`ZOrder 0`) → SetInputMode_UIOnlyEx is **unchanged**. `get_dirty_content_packages()` = **0** ⇒ disk matches memory.

**`Content/UI/WBP_MainMenu.uasset` — NOT TOUCHED.** sha256 `03cc0a395ebe072d4248f6429027622a9feccb99752ea8d5f3cd35da42564ae3`, identical before and after.

**What the fix changes at runtime, exactly:** `InWidgetToFocus == nullptr` ⇒ `SetWidgetToFocus` is **never called** ⇒ the only line in the engine that can print this Error is unreachable from this map. `SetLockMouseToViewportBehavior(DoNotLock)`, `SetInputMode`, `SetIgnoreInput(true)`, `SetMouseCaptureMode(NoCapture)` and the cursor write all run **byte-identically**; the single behavioural delta is that one deferred `SetUserFocus(<menu root>, SetDirectly)` request is no longer issued.

## (2) THE PIN DELETION — **SAME DIFF, and here is the proof**

**File:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp` (⛔ the long path — verified with `find`, **exactly one match in the repo**; the short `Tests/SiegeMenuInputTest.cpp` does not exist).

```
$ grep -c 'AddExpectedError' Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp
0
$ git diff --stat -- .../Tests/SiegeMenuInputTest.cpp
 1 file changed, 15 deletions(-)
$ git diff -- .../Tests/SiegeMenuInputTest.cpp | grep -c '^@@'
1
```

**ONE hunk, `@@ -114,21 +114,6 @@`, 15 deletions, ⛔ ZERO insertions** — the 12-line comment block, the two-line `AddExpectedError(…, Contains, /*Occurrences*/ 1);` call, and the one blank line that separated it. **Every assertion, every latent command, every `TestNotNull`/`TestEqual`, the test's name and its flags are byte-identical.** ⛔ No `SetSuppressLogErrors`, ⛔ no weaker expectation, ⛔ no replacement of any kind — the **absence** of the Error is now what the framework asserts for free by redding on an undeclared `Error`.

**Both halves are in one working-tree diff; neither may land alone.** (`git status` below shows them together.)

## (3) REACHABILITY / `SC-§123` — what I measured, and what I did NOT

✅ **MEASURED — the census of every site that can emit this Error:**
* **Blueprints:** `grep -rl 'SetInputMode_UIOnlyEx' Content/` ⇒ **exactly one package: `Content/Blueprints/BP_MenuGameMode.uasset`.** (Widening to any `SetInputMode_*` adds only the four stock touch-interface assets, none on `L_MainMenu`.)
* **C++:** one other `FInputModeUIOnly` + `SetWidgetToFocus` caller — `SiegePlayerController.cpp:2264–2270`, the **match-end victory screen** (`VictoryWidget->TakeWidget()`). ⚠️ **It is on `L_Arena` at match end, not on `L_MainMenu`, and the test never ends a match — so it is outside this row and I did not touch it.** ⛔ **It is an open look-alike, flagged for the board, not for me:** if `WBP_VictoryScreen` is also non-focusable it prints the identical Error at match end. Nothing in this row's scope proves it either way.
* **Engine:** the message exists at **one** site, `PlayerController.cpp:6345`, compiled only outside Shipping/Test.
⇒ on `L_MainMenu` boot and throughout the test's recipe (which opens the deck builder — `UDeckBuilderWidget` contains **no** `SetInputMode` call, comments only), **`BP_MenuGameMode` was the single emitter**, which matches the host's measurement of exactly one line per run ×2.

✅ **MEASURED — why the cold-state focus index cannot move.** Today's request is resolved by `FSlateApplication::SetUserFocus` walking **upward** from the non-focusable menu root to the first focusable **ancestor** (`SlateApplication.cpp:3016–3030`) — it can never reach a descendant button. The cold state `TASK-1274` recorded, *"focused index 0 ('Play (vs Bot)') of 7 buttons"*, is therefore produced by `ApplyInitialFocus` alone, on the next tick, unconditionally when no button holds focus. Removing a request that (a) targets an ancestor and (b) is demonstrably not holding focus at cold state (or `index 0` could not have been the measurement) **cannot change that index**. ⇒ predicted cold state after the fix: **unchanged, index 0 of 7, focus brush drawn (`EFocusCause::Navigation`)**.

⛔ **DECLARED UNMEASURED BY ME, and it belongs to the verify leg, acceptance (3):** I did **not** run PIE. I have not observed `0` occurrences of the Error in a live `L_MainMenu` log, and I have not re-run `IA_MenuDown ×2 → IA_MenuAccept`. ⇒ **the cold-state index quoted above is a PREDICTION FROM SOURCE, not a re-measurement**, and the row's acceptance (4) is not discharged by me. ⛔ **And it cannot be measured at all until the owed Blueprint compile lands** — against the current stale class the rig would observe the Error still present and be right to fail the row.

## (4) ⛔ THE CEILING, DECLARED NOW (`VER-§8`) — and the honest answer is **this row does not hit it**

* **Acceptance (3) is fully within the rig's reach and must NOT be returned `UNOBSERVABLE`.** Its two halves are (i) a **log-line count** — a read, not an input — and (ii) the `TASK-1274` recipe driven through **`inject_input_action` → `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction`**, the same door `SiegeMenuInputTest.cpp`'s `InjectMenuAction` uses and the door `qa/TASK-787-verify.md` proved reaches our mappings. **`VER-§8`'s ceiling is Slate-level keys; this row needs neither a Slate keypress nor a UMG click.** A `VERIFY-FAILED` here would be a real finding.
* ⛔ ~~**BUT THE CEILING IS REAL FOR JONATHAN'S HANDS, and it is worth stating so nobody asks him for the impossible:**~~ ✅ **THE ENHANCED-INPUT CEILING IS REAL — AND IT IS A CEILING ON THAT ONE LANE, NOT ON HIS HANDS (framing struck 2026-09-21, see the correction below; the measurement that follows is UNTOUCHED):** `FInputModeUIOnly::ApplyInputMode` calls **`GameViewportClient.SetIgnoreInput(true)`** (`PlayerController.cpp:6377`, read at source) ⇒ on `L_MainMenu` a **real key press does not reach Enhanced Input at all**. ⛔ ~~**Do not ask him to "press Down twice and Enter on the main menu" — that cannot work on that map regardless of this row.**~~
  * 🚨🧑 **CORRECTION — 2026-09-21, `TASK-1383`** (law: `VER-§8` cl. 11 `A LANE CEILING IS NOT A HUMAN CEILING` · `SC-§136` cl. 6 struck-not-deleted · `SC-§101` · `SC-§39`). **SOURCE: 🧑 Jonathan's own hands at the 2026-09-21 sitting, verbatim, relayed through the orchestrator lane — an ACCOUNT, not a measurement any agent took (`VER-§0` cl. 4):** *"ok, I opened a match, I saw the outline on the top menu option, hit the down arrow twice, and hit enter, and I was able to open the deck builder, so that menu navigation seems to be working fine."*
  * ✅ **THE PREMISE ABOVE SURVIVES, UNRELAXED, AND IS STILL LOAD-BEARING — IT IS STRUCK NOWHERE.** `FInputModeUIOnly::ApplyInputMode` still calls `GameViewportClient.SetIgnoreInput(true)`, `UGameViewportClient::InputKey` still returns early on `IgnoreInput()` (`GameViewportClient.cpp:767`), and a real key press still does **not** reach **Enhanced Input** on `L_MainMenu`. **Nothing he did measures this, and nothing he did refutes it.**
  * ⛔ **WHAT IS REFUTED IS THE CONCLUSION ALONE** — the leap from *"the Enhanced-Input lane is dead on this map"* to *"the ceiling is real for HIS HANDS"* and *"do not ask him"*. **He was asked, he did it, it worked.** `SetIgnoreInput(true)` switches off **ONE ARROW** in `OS → FSlateApplication → focused-widget key path → UGameViewportClient::InputKey → PlayerController → Enhanced Input`, **and it is not the first one** — so this handoff was entitled to *"the Enhanced Input lane cannot carry it"* and to **nothing whatever about his hands**.
  * 🚨 **THE MECHANISM IS UNMEASURED, AND THIS DOCUMENT DOES NOT CLAIM *"it reached Slate"* AS A FINDING.** The entitled sentence, cited here and nothing broader: ***A REAL KEY PRESS ON `L_MainMenu` DOES REACH THE MENU; WHICH LAYER CARRIED IT IS UNMEASURED.*** **Two routes predict his identical observable: (i)** the focused `SButton` → Slate's own navigation config → `SButton::OnKeyDown`'s Accept path (`SButton.cpp:296`) — the route `handoffs/TASK-1274-programmer.md` §7 predicted **in advance** and labelled *"UNMEASURED here"*; **(ii)** `USiegeMenuInputSubsystem`'s `IA_Menu*` handlers — **which would mean the premise above is wrong.** **His sentence discriminates NEITHER.** Route (i) is the better-supported hypothesis and is **still a hypothesis** (`SC-§101`): matching a prediction is **not** discriminating between two mechanisms that predict the same thing.
  * ⛔ **WHY THIS IS STRUCK AND NOT DELETED (`SC-§136` cl. 6): a silent correction would destroy the evidence that the defect had a mechanism.** The mechanism: `TASK-1274` §7 carried **both** halves (Enhanced-Input death **and** the Slate route, explicitly unmeasured); this line carried the first half forward, **dropped the second**, and hardened the remainder into a **prohibition** — a wrong fact wearing a prohibition is **self-sealing**, because it forbids the one experiment that refutes it, and here it sealed off the only witness on this project who can press a real key. ⇒ *"I could not reach it"* is always writable; *"he cannot reach it"* almost never is.
* 🧑 **HIS ONE-SENTENCE HAND CHECK, specced in advance, with the false positives in the parenthesis** — it targets the only human-visible risk this diff carries (a regression of the boot focus brush):
  > **"Boot to the main menu and, before touching the mouse or keyboard, look at the top button 'Play (vs Bot)' — is there a focus outline drawn on it? (A highlight that appears only when the mouse is over the button is a NO — move the pointer off the menu first; and an outline that appears only after you click something is a NO.)"**
  The parenthesis is load-bearing: `SButton`'s own hover brush and a click-induced `SetDirectly` focus both look like the thing being asked about while the boot focus path is dead.

## (5) Deprecation census (`SC-§122` — marker by name at declaration **and** definition, with a firing control)

Not strictly needed — **this diff contains no new C++ call at all** — but recorded because the route argument rests on it and because route (A) would need it:

| symbol | declaration | definition | marker |
|---|---|---|---|
| `UUserWidget::bIsFocusable` (UPROPERTY) | `UserWidget.h:1030–1033` | — | ⛔ **`UE_DEPRECATED(5.2, "Direct access to bIsFocusable is deprecated…")` PRESENT** |
| `UUserWidget::SetIsFocusable(bool)` | `UserWidget.h:1104` | `UserWidget.cpp:2421` | ✅ **no marker at either site** |
| `UUserWidget::IsFocusable() const` | `UserWidget.h:1102` | `UserWidget.cpp:2416` | ✅ **no marker at either site** |

**Firing positive control:** the same grep over the same header returns **16** `UE_DEPRECATED` occurrences, one of them on `bIsFocusable` itself ⇒ the method finds markers where they exist and is not merely returning zero.
⭐ **And the deprecation note's claim is FALSE for `UUserWidget`, confirmed at source:** *"only set at construction and is not modifiable at runtime"* — but `SObjectWidget::SupportsKeyboardFocus()` (`SObjectWidget.cpp:175`) calls `WidgetObject->NativeSupportsKeyboardFocus()` **live, every time, with no cache**, and that returns `bIsFocusable` directly. The note is true of `UButton`, not of `UUserWidget`.

## (6) `git status` — only the named files (git root is ONE level up, `SC-§102`)

```
 M GitClaudeUnrealTest/Content/Blueprints/BP_MenuGameMode.uasset          <- MINE
 M GitClaudeUnrealTest/Source/.../Siegebound/Tests/SiegeMenuInputTest.cpp <- MINE
 M GitClaudeUnrealTest/Source/.../Siegebound/DeckBuilderWidget.cpp        <- TASK-1307 lane
 M GitClaudeUnrealTest/Source/.../Siegebound/DeckBuilderWidget.h          <- TASK-1307 lane
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1                        <- TASK-1294 lane
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1294-programmer.md  <- TASK-1294 lane
```
**Nothing staged** (`git diff --cached --name-only` empty) — the UE Git plugin did not auto-add this save, but the host should still commit **by exact pathspec** and verify the **commit**, not the index.
⛔ **I touched neither parallel lane's files.** ⛔ No Git command beyond `status` / `diff` was run; ⛔ no C++ compile was run.

---

## What QA should scrutinise (`TASK-1297`)

1. ⛔ **The coupling, first:** both halves are above and in one working tree — the wire is gone from the graph **and** `AddExpectedError` = 0, in the same diff. Neither can land without the other.
2. ⛔ **The BLOCKER at the top** — the gate should decide whether `ready-for-qa` is even the right flip while `BP_MenuGameMode` is `BS_DIRTY`. My reading: the code review can proceed, but **`TASK-1298` must not start the suite until the compile lands**, and the row's status line says so.
3. ⚖️ **The route deviation (B′ vs the row's literal (A)/(B))** — the two refutations of literal (B) (runtime-built buttons; `SetDirectly` killing the focus brush) and of (A) (focus granted to a container + an ordering race) are the load-bearing claims. If the gate rules the enumeration binding, route (A) is one property flip away and this diff reverts cleanly.
4. ⛔ **`SiegeMenuInputTest.cpp` is a pure deletion** — 1 hunk, 15/0. Anything else in that file would be out of contract.
5. ⚠️ **The victory-screen look-alike** (`SiegePlayerController.cpp:2264`) — same engine call, different map, out of this row's scope; worth a board row, not a silent fix here.

**Law touched:** `SC-§60` (the pin that encoded a defect as expected behaviour is now gone with its cause) · `SC-§68` (hashes named) · `SC-§101` (the route was chosen after the read, not before) · `SC-§102` (git root one level up) · `SC-§104` · `SC-§122` (census above) · `SC-§123` (reachability measured; the runtime half declared, and the compile gap raised as a BLOCKER, not a caveat) · `VER-§8` (ceiling declared at hand-off time, with the hand check pre-written) · `KBD-§4` (untouched — no `IMC_MainMenu` / `IA_Menu*` edit).
