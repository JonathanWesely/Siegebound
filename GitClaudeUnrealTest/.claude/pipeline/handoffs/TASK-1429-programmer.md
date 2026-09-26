# TASK-1429 — [MENU-NAV-INMATCH-ARMING] — programmer handoff

**Marker:** `TASK-1429-MENU-NAV-INMATCH-ARMING`
**Gate:** `TASK-1430` (qa-reviewer). **Then** 5a → 5b = `TASK-1436` → commit `TASK-1437`.
**Files written (two, and nothing else):**
`Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp` · `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h`
⛔ No `IMC_Hero` edit. ⛔ No asset of any kind. ⛔ No gameplay input path. ⛔ No `Tests/`. ⛔ No other screen's file. ⛔ No `CONVENTIONS.md`. ⛔ No compile, no PIE, no MCP mutation, no editor lifecycle action, no git.
**Read-only instruments used:** engine source under `UE_5.8/Engine/Source`; `get_input_mapping_context_keys` (two calls); `execute_unreal_python_readonly` (two read-only reads); a `grep` over `Content/*.uasset`.

---

## 0. The one-paragraph answer

The map gate is not moved; it is **bypassed by a second, later door that only a registered in-match screen can open.** `OnWorldBeginPlay`'s `if (MapName != MenuMapName) { return; }` is **untouched**, so a match in which nothing registers is byte-for-byte the match that shipped — no asset loaded, no action bound, no context applied, no timer armed. `RegisterMenuNavTarget` is the rising edge; `UnregisterMenuNavTarget` is the falling edge; a disarm-only backstop poll catches a screen that vanished without unregistering. The collision census came back with **exactly one hit — `Enter`** — so I did **not** take the priority the spec asked for: the context is applied **below** `IMC_Hero`, which stomps nothing, and the one-key question is escalated by letter in §3. And (5) is **measured, not inherited**: in-match the input mode is `GameAndUI`/`GameOnly`, both of which call `SetIgnoreInput(**false**)`, so Enhanced Input is **live** in a match — the opposite of the menu map.

---

## 1. (1) THE DEFAULT-STATE BRANCH, NAMED

**The branch is `OnWorldBeginPlay`'s existing `return`, and the deliverable is that it did not change.**

```
const FString MapName = FPackageName::GetShortName(UWorld::RemovePIEPrefix(InWorld.GetOutermost()->GetName()));
if (MapName != MenuMapName)
{
    return;
}
```

Re-found **by content**, not by number (`CITE-BY-TEXT-RULED-2026-09-24`): `SiegeMenuInputSubsystem.cpp`, the first statement pair of `OnWorldBeginPlay` after `Super::`. My diff there is **a comment block and nothing else** — zero statements added, moved, reordered or removed.

⇒ **On `L_Arena` (or any map but `L_MainMenu`), at level boot, this subsystem still does literally nothing.** There is no code between that `return` and the top of the function that this row added, so there is no "harmlessly applied" context to argue about. That is the whole of `TASK-1430`'s (1), and it is checkable by looking at the diff hunk rather than by reading a paragraph.

**THE EXACT PREDICATE that distinguishes "an in-match screen is open" from "the player is playing"**, implemented once, in `USiegeMenuInputSubsystem::IsInMatchScreenOpen()`:

```
GetRegisteredNavTarget() != nullptr
```

i.e. **some entry of `NavTargetStack` is a live `UUserWidget` that `IsInViewport()` AND `IsVisible()`** (all three re-validated on every read — that is `GetRegisteredNavTarget()`'s shipped body, unmodified).

⭐ **It is a DEMAND test, not a survey.** The only way to make it true is an explicit `RegisterMenuNavTarget(self)`. Nothing becomes "open" by being drawn, by being on top, or by existing. That property is what answers (6) with no HUD special case, and it is why the predicate is a named function rather than an inline `if`.

**Where it is read:**

| Site | `bMayArm` | Effect |
|---|---|---|
| `RegisterMenuNavTarget`, after `NavTargetStack.Add(Screen)` | `true` | the ONLY rising edge in the project |
| `UnregisterMenuNavTarget`, after `RemoveAll`, **above** the `!bWasRegistered` early-return | `true` | falling edge; stays armed if a nested screen is still live |
| `PollInMatchDemand` (backstop, `InMatchDemandPollSeconds = 0.2f`) | **`false`** | one-way valve — can only give the keys back |
| `Deinitialize` | — | calls `DisarmInMatchMenuVocabulary` directly, before the asset pointers are nulled |

**The menu map is excluded by `bArmed`, not by a second map test.** `bArmed` is assigned `true` in exactly one place — `OnWorldBeginPlay`, *below* the map gate — so it is true on `L_MainMenu` and nowhere else. A second copy of the gate would be a second thing to drift, and this file has already paid for a predicate that existed twice (`TASK-1469` limb 1(b)). ⇒ **the five `built` main-menu screens are behaviourally untouched: every reconcile they trigger returns on line 1.**

---

## 2. (2) THE KEY-COLLISION CENSUS — ⛔ ONE HIT, AND I DID NOT TAKE IT

Read **live off the assets** 2026-09-25 (`get_input_mapping_context_keys`), not inferred from source.

**Scope first, because a census against `IMC_Hero` alone would be incomplete by construction.** Four `InputMappingContext` assets exist in the project. Their asset referencers (`AssetRegistry::get_referencers`):

| Context | Referencers | Live in a match? |
|---|---|---|
| `/Game/Input/IMC_Hero` | `/Game/Blueprints/BP_HeroCharacter`, `/Game/Blueprints/BP_SiegeGhostPawn` | ✅ **YES** — priority **1** (`HeroCharacter.cpp:53` `HeroMappingContextPriority`; `SiegeGhostPawn.cpp:37` mirrors it) |
| `/Game/Input/IMC_Default` | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController` | ❌ no — `ASiegePlayerController : public APlayerController` (`SiegePlayerController.h:204`), **not** the template controller |
| `/Game/Input/IMC_MouseLook` | 3 template controllers (ThirdPerson / Combat / Platforming) | ❌ no |
| `/Game/Input/IMC_MainMenu` | **none** — loaded by path in C++ | n/a |

⇒ **the only context live in a Siegebound match is `IMC_Hero`.**

**`IMC_MainMenu` = 12 mappings. `IMC_Hero` = 28 mappings. Every one of the twelve, checked:**

| `IMC_MainMenu` key | → action | In `IMC_Hero`? | Verdict |
|---|---|---|---|
| `Up` | `IA_MenuUp` | no | ✅ clean |
| `Gamepad_DPad_Up` | `IA_MenuUp` | no | ✅ clean |
| `Down` | `IA_MenuDown` | no | ✅ clean |
| `Gamepad_DPad_Down` | `IA_MenuDown` | no | ✅ clean |
| 🚨 **`Enter`** | **`IA_MenuAccept`** | 🚨 **YES → `IA_AssistantConsole`** | ⛔ **COLLISION** |
| `Gamepad_FaceButton_Bottom` | `IA_MenuAccept` | no | ✅ clean |
| `Left` | `IA_MenuLeft` | no | ✅ clean |
| `Gamepad_DPad_Left` | `IA_MenuLeft` | no | ✅ clean |
| `Right` | `IA_MenuRight` | no | ✅ clean |
| `Gamepad_DPad_Right` | `IA_MenuRight` | no | ✅ clean |
| `Backspace` | `IA_MenuBack` | no | ✅ clean |
| `Gamepad_FaceButton_Right` | `IA_MenuBack` | no | ✅ clean |

**Eleven clean, one collision.** Both actions carry `bConsumeInput = true` — read off the assets, and it is the engine default (`InputAction.h:96`). All six `IA_Menu*` actions read `bConsumeInput = true`, `bTriggerWhenPaused = false`, `Triggers = []`.

⚠️ **AN EXPIRY DATE, PRINTED ON IT.** `IMC_Default` maps `Up`/`Down`/`Left`/`Right` → `IA_Move` and `Gamepad_FaceButton_Bottom` → `IA_Jump`. If any future row puts a template controller (or that context) into the arena, **this census is wrong by five keys, not by one.** The census is anchored to "which contexts are applied in-match", not to "which keys the hero uses".

**THE MECHANISM, MEASURED AT ENGINE SOURCE — the consequence of a collision is not "both fire":**
`IEnhancedInputSubsystemInterface::RebuildControlMappings` sorts applied contexts **descending by priority** (`EnhancedInputSubsystemInterface.cpp:947`), keeps a `TSet<FKey> AppliedKeys` (`:949`), adds a mapping **only if** `!AppliedKeys.Contains(Mapping.Key)` (`:1041`), records the key when `Mapping.Action->bConsumeInput` (`:1054`), and appends that context's claimed keys after each context (`:1091`).
⇒ **the higher-priority mapping does not merely win — the lower one is never built at all.** And binding is irrelevant to this: consumption is decided by the *mapping*, so declining to `BindAction` an action would not spare the key.

---

## 3. (3) THE PRIORITY — ⛔ **BELOW** THE HERO, AND THE LETTER ASKING TO CHANGE THAT

**Shipped:** `InMatchMenuMappingContextPriority = 0`, a **new constant** (not a reuse of `MenuMappingContextPriority`, which happens to share its value but is justified by a different fact — "nothing else is applied on `L_MainMenu`"). `AHeroCharacter::HeroMappingContextPriority = 1`. ⇒ **the menu context sits below the hero's.**

**The row asked for ABOVE. Here is why it shipped BELOW, and it is not a preference.**

Given the mechanism in §2: for the **eleven** keys no live context maps, `AppliedKeys` can never contain them, so the menu mapping is built **either way** — the priority number is a **behavioural no-op** for 11 of 12 keys. For the twelfth, "above" **is** precisely and only the act of taking `Enter` away from `IA_AssistantConsole` for the whole time a screen is open. Spec (2) and `HELP-§4` make that 🧑 **a for-Jonathan row, not an agent's call** — and `TASK-1430`'s own acceptance (3) asks that *"no conflicting key was TAKEN"*. Shipping above would fail that clause to satisfy clause (3) of the spec; shipping below satisfies both for eleven keys and defers exactly one.

**⭐ AND BELOW BUYS TWO SAFETY PROPERTIES ABOVE CANNOT — this is the part that matters on the highest-risk diff in the milestone:**

1. **The worst case of a leaked arm is bounded.** If the context were ever left applied with nothing registered (a bug in my edges, a backstop that never fired), it would still take **no key from the player**, because it claims no key `IMC_Hero` claims. The handlers would decline anyway (`IsNavTargetActionable()` is false in-match with nothing registered — `FindMainMenuWidget()` is null and `IsMenuUncovered()` returns false at the first visible non-menu top-level widget, i.e. `WBP_HUD`). **The failure mode is log noise, not a stolen match.** At priority 2 the same bug would be a live match whose `Enter` is dead.
2. **The disarm cannot re-trigger a hero action.** `RemoveMappingContext` hands keys back to lower-priority contexts. At priority 0 this context never held one, so there is nothing to hand back and no hero action can fire as a side effect of a screen closing.

### ⚖️ THE LETTER — ESCALATED TO THE MANAGER, FOR 🧑 JONATHAN

> **The question:** while an in-match menu screen is open, should `Enter` mean **"activate the focused control"** (`IA_MenuAccept`) or keep meaning **"toggle the assistant console"** (`IA_AssistantConsole`)? It cannot mean both — Enhanced Input builds only the higher-priority mapping.
>
> **What is at stake, concretely.** *Opening* the console is unaffected either way (nothing is registered at that moment, so no context is applied). What changes is (a) whether `Enter` can **close** a console that has registered itself as a nav target — `TASK-1434`'s screen — and (b) whether `Enter` still opens the console while some other in-match overlay (`TASK-1432`'s controls help) is open and registered.
>
> **What I shipped, pending the ruling:** below the hero ⇒ `Enter` keeps its shipped in-match meaning, and `IA_MenuAccept` is reachable in-match by **`Gamepad_FaceButton_Bottom`** (not mapped by `IMC_Hero`) and by injection. **A keyboard user is not left without an Accept** — see the Slate finding in §4, which is the reason this costs so little.
>
> **The change if he rules the other way is ONE LINE:** `InMatchMenuMappingContextPriority` `0` → `2`. No other edit anywhere.
>
> ⛔ I did not board this, take it, or write it into any other row (`SC-§101`).

---

## 4. (5) THE UNMEASURED PREMISE — ⛔ **MEASURED**, AND `TASK-1424` **ONLY NARROWS IT**

**Does `qa/TASK-1424.md` settle my (5)? ⛔ NO — it NARROWS it, and I am saying so explicitly as the rider asks.**
It measured the chain at four links and found it **TRUE**: `BP_MenuGameMode` BeginPlay → `SetInputMode_UIOnlyEx` → `FInputModeUIOnly::ApplyInputMode` → `SetIgnoreInput(true)` → `UGameViewportClient::InputKey` early return. ⛔ **But it measured it on `L_MainMenu`, under `BP_MenuGameMode`.** In-match the game mode, the controller class and the input mode are all different objects, so that reading tells me what happens under `UIOnly` — which is exactly half of what I need — and nothing about which mode a match is in. I re-walked the second half only (`SC-§138`: I did not re-derive the four links it already walked).

**MEASURED FOR THE IN-MATCH MODE — the answer is the opposite of the menu map's:**

`ASiegePlayerController::ApplyCursorInputState()` (`SiegePlayerController.cpp:6410-6476`) is the **one** input-mode owner in a match (CONVENTIONS "Input-mode ownership (level-travel law)"; every in-match widget is grepped clean of `SetInputMode`). It applies exactly one of two modes:

| In-match posture | Engine site | `SetIgnoreInput` |
|---|---|---|
| `FInputModeGameAndUI` (cursor up: placement / targeting / group pick / console / war map / controls help / `IA_UICursor`) | `PlayerController.cpp:6398-6413` | **`false`** (`:6410`) |
| `FInputModeGameOnly` (ordinary play) | `PlayerController.cpp:6439-6451` | **`false`** (`:6449`) |
| *(contrast)* `FInputModeUIOnly` | `PlayerController.cpp:6372-6387` | `true` (`:6384`) |

⇒ 🚨 **BRANCH (i): IN A MATCH THE VIEWPORT IS LIVE AND ENHANCED INPUT HEARS REAL KEYS.** No input-mode change is needed and **none is proposed**. Nothing is escalated under (5)(ii) **for the match itself**.

⚠️ **THE ONE EXCEPTION, NAMED RATHER THAN GLOSSED — THE VICTORY SCREEN.** `HandleMatchEnd` latches `bMatchEnded` (which makes `ApplyCursorInputState` early-out at `:6414`) and applies `FInputModeUIOnly` at `SiegePlayerController.cpp:2368` **once a victory widget exists** ⇒ on the end screen, and only there, a **real** key is swallowed before Enhanced Input. Injection is unaffected (`InjectInputForAction` never touches the viewport). ⛔ **This is not a new finding and needs no escalation:** `SiegePlayerController.cpp:2370-2377` already documents it and already ships the remedy — Slate focus on the Play Again button, which is that screen's live keyboard route. (`TASK-1319` also degrades the *no-widget* end state to `GameAndUI` at `:2302-2310`, so the deaf state requires a widget to be present.)

### ⭐⭐ THE CONSEQUENCE THE DISPATCH FLAGGED: "if both lanes are live, could one real key drive both?" — **MEASURED: NO.**

The dispatch was right that this is the dangerous part, and it is the one place where being wrong would double every keypress. Measured at Slate source:

1. `FSlateApplication::ProcessKeyDownEvent` routes a key **down the FOCUS PATH** — tunnel (`PreviewKeyDown`), then **bubble** (`OnKeyDown`) from the focused widget upward (`SlateApplication.cpp:5016-5073`). There is **no separate navigation pass**: the function ends at `UnhandledKeyDownEventHandler`.
2. Arrow-key navigation is produced **by the focused widget itself**: `SWidget::OnKeyDown` returns `FReply::Handled().SetNavigation(Direction, Keyboard)` for any focusable widget (`SlateCore/Private/Widgets/SWidget.cpp:416-429`). `SButton::OnKeyDown` handles the **Accept** keys itself and otherwise falls through to that (`SButton.cpp:293-316`).
3. `SViewport` — the **only** door to `UGameViewportClient::InputKey`, and therefore the only door to Enhanced Input (`SViewport.cpp:337-340` → `FSceneViewport::OnKeyDown`, `SceneViewport.cpp:1266-1290`) — is an **ancestor** of every UMG widget added to the viewport, so it is **late** in the bubble.

⇒ **WITH A STOP FOCUSED, THE BUBBLE STOPS AT THE STOP AND NEVER REACHES THE VIEWPORT.** Slate navigates, Enhanced Input never sees the key, **nothing fires twice.** The two lanes are still **disjoint in-match — but for a completely different reason than on the menu map** (there the viewport is deaf; here the key is consumed upstream).

⇒ **AND THE CONVERSE, WHICH IS WHY THE DEMAND GATE IS LOAD-BEARING AND NOT BELT-AND-BRACES:** with **nothing** focusable focused — ordinary play, where `FInputModeGameOnly` puts focus on `SViewport` itself — the viewport **is** the focused widget, `FSceneViewport::OnKeyDown` forwards to `InputKey`, and **Enhanced Input does see the arrow keys.** ⇒ a context applied with nothing open would genuinely hand a live match to a menu. **That is the regression the fence names, and it is real, not theoretical.**

⚠️ **IT ALSO RESIZES THE `Enter` COLLISION HONESTLY, and I would rather record this than let §3 read as bigger than it is:** with a stop focused, `SButton::OnKeyDown` consumes `Enter` before the viewport **at any priority**, so `IA_AssistantConsole` would not have fired at that instant either way. **The dominant mechanism for "Enter stops toggling the console while a screen is open" is `TASK-1406`'s already-shipped focus placement, not this row's mapping context.** The collision is still real and still his call — it decides the *no-stop-focused* case — but "below" costs correspondingly little.

---

## 5. (6) + (6b) THE HUD AND THE FPS COUNTER — CONFIRMED IN A MATCH-SHAPED CASE, NO SPECIAL CASE ADDED

**(6) CONFIRMED.** `TASK-1406`'s active-target model handles both, and the reason is structural rather than incidental:

- `IsNavTargetActionable()` short-circuits on `GetRegisteredNavTarget() != nullptr`. The HUD (ZOrder 0, never collapsed, `SiegePlayerController.cpp:444`) and the FPS counter (ZOrder 30) **never call `RegisterMenuNavTarget`**, so they can never make the gate true and can never arm anything.
- `GetMenuFocusStops()` walks `GetActiveNavTarget()`'s **own `WidgetTree`** — the registered screen's, never the viewport's. A walk therefore **never enters the HUD's tree at all**, whether or not something else is registered.
- With nothing registered in-match, `GetActiveNavTarget()` is `FindMainMenuWidget()` = **null** (no `WBP_MainMenu` on `L_Arena`) ⇒ zero stops; and `IsMenuUncovered()` returns **false** at the first visible non-menu top-level widget, which in a match is the HUD. Both layers agree, independently.

⛔ **No HUD clause exists in the diff.** Grep it: the strings `WBP_HUD`, `FPS`, `Btn_Jump` appear nowhere in `SiegeMenuInputSubsystem.{cpp,h}`.

**(6b) — WHAT MY (6) ANSWER DOES WITH `WBP_HUD`'s `Btn_Jump`, stated as the rider demands (a):**

🚨 **It is a stop that is NEVER COLLECTED — not a stop that is collected and then refused.** The two are indistinguishable in a bare count, which is exactly why this says which one it is. The mechanism is the one above: **no walk ever enters the HUD's tree**, so the `IsFocusable == false` opt-out is never even consulted. It is not excluded by a limb; it is out of scope of every walk.
- Anchored **by object path**, per the rider: `WBP_HUD:Overlay_19/SizeBox_0/Btn_Jump`. ⛔ The bare name `Btn_Jump` is **not** an identifier in this project — it exists at the identical chain in `WBP_VictoryScreen`, `WBP_HUD` and `WBP_CardHand` (`BTN-JUMP-NAME-COLLISION-STRUCK-2026-09-25`), and the live census found it in nine design-time trees.
- **(b)** ⛔ I did **not** touch the walker. `TASK-1406`'s `IsNavFocusStop` honours `IsFocusable == false` correctly (QA re-measured it at the 5.8 headers and it passed). Nothing in this diff is a walker change.
- **(c)** `WBP_CardHand:Overlay_19/SizeBox_0/Btn_Jump` is `Collapsed`, and is additionally out of scope for the same never-walked reason. I did not go looking for a second live one.
- ⛔ **The remedy is not mine and I am not taking it.** I do **not** conclude the flag must move — under this design it is inert, so there is nothing to escalate. If a future row ever registers the HUD tree as a nav target, that row inherits the question, and flipping the property is a hand-save in the Blueprint editor (`[[ue-blueprint-compile-does-not-dirty-package]]`), i.e. a manager escalation, never an agent's asset edit.

---

## 6. ⛔ EVERY EDGE, ANSWERED

| Edge | What happens |
|---|---|
| **Screen opens mid-input** (a menu key already held when the context arrives) | The key gets its `ETriggerEvent::Started` edge on the first tick under the new mapping ⇒ **one** extra menu step (e.g. opening with `Down` held moves the ring one stop). Bounded at one step, on a screen the player just opened, and `RegisterMenuNavTarget` places the ring on stop 0 in the same call. ⛔ It can never be a *gameplay* action: at this priority the context claims no hero key. **Declared, not hidden.** |
| **Screen closes while a key is held** | All six bindings are `ETriggerEvent::Started`; the edge has already passed, so **nothing of ours fires**. `RemoveMappingContext` hands keys back to lower-priority contexts — at priority 0 we held none of the hero's, so **nothing is handed back and no hero action is re-triggered** either. The hero's own key resumes on its next press. |
| **Two screens stacked** (help → console, Settings → Graphics) | `NavTargetStack` nests, as shipped. The predicate is "is there ≥1 live registered target", so the context is applied **once** on the first registration (`ArmInMatchMenuVocabulary` early-returns when already armed — load-bearing, because `BindAction` **appends**) and removed only when the **last** one goes. Unregistering the inner screen leaves the outer live ⇒ reconcile keeps it armed. |
| **Match ends with a screen open** | `HandleMatchEnd` applies `FInputModeUIOnly` ⇒ real keys are swallowed by the viewport; our handlers remain reachable by injection only. The open screen's own close path unregisters (falling edge) or the backstop notices the widget has gone; either way the context comes off. `Deinitialize` removes it unconditionally on level travel, **before** the asset pointers are nulled. |
| **Screen destroyed / removed / hidden without unregistering** | `GetRegisteredNavTarget()` already skips dead, out-of-viewport and hidden entries — but **nothing drives a read**, which is the whole reason the backstop exists. `PollInMatchDemand` (0.2 s, armed *with* the vocabulary and retired *with* it) notices and disarms, up to one poll interval late. |
| **Pawn changes (hero dies → ghost → respawn)** | `ASiegeGhostPawn` re-adds `IMC_Hero` at the same priority 1; the controller and its input component are unchanged, so our bindings and our context survive untouched. |
| **The input component is replaced under us** | `BoundInMatchInputComponent` compares against the **live** component, not a bool, so we re-bind instead of silently going dead. |
| **Menu map (`L_MainMenu`)** | Every reconcile returns on `bArmed`. The five `built` main-menu screens are behaviourally untouched; `Deinitialize`'s disarm early-returns on `bInMatchVocabularyArmed == false`. |
| **Menu-map arm FAILED (no controller / missing asset)** | `bArmed` stays false, so a registration there would take the in-match path — which applies the same context at the same priority the menu map wanted anyway. **It degrades toward repair, not toward breakage.** Named rather than hidden. |

---

## 7. 🚨 ACCEPTANCE AS A **PROPERTY**, NEVER AS AN INDEX — FOR `TASK-1436` (5b)

⛔ House law from tonight: *a gate written in indices has an expiry date nobody printed on it.* These are written so a **correct** build cannot fail them.

### P1 — DEFAULT STATE (the negative control). ⚠️ **IT HAS TWO CORRECT SIGNATURES. A GATE THAT ACCEPTS ONLY ONE WILL FAIL A CORRECT BUILD.**

*In a live match with no in-match screen registered, an injected `IA_MenuDown` changes **no focus reading**.*

| State | Correct log signature |
|---|---|
| **(a) Fresh match, no screen has EVER registered** | ⛔ **NO `[USiegeMenuInputSubsystem]` LINE AT ALL.** Nothing is loaded and nothing is bound, so the injected action reaches no handler. |
| **(b) A screen was opened and closed earlier in the same match** | The entry line `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` **followed by** `MoveFocus(+1) declined: menu covered.` — **two lines, and no `focus moved` line.** The bindings persist after a disarm by design (nothing is ever unbound; the context removal is what makes real keys inert, and injection bypasses key mappings entirely). |

⛔ **BOTH ARE PASSES.** The discriminator between them is the presence of the entry line; the discriminator for the *property* is the **absence of any `focus moved` line** and an unchanged focus reading.
⛔ **FAIL** = a `MoveFocus(+1): focus moved …` line, **or** an `IN-MATCH MENU VOCABULARY ARMED` line with nothing registered.

### P2 — ARMED
*While at least one live registered in-match screen exists: exactly one `IN-MATCH MENU VOCABULARY ARMED (…)` line has been emitted since the last disarm, and an injected `IA_MenuDown` moves the focus reading to a different stop **of that same registered screen**.*
⛔ **Two** arm lines without an intervening disarm = double-binding = FAIL.

### P3 — DISARM
*After the last registered screen unregisters, an `IN-MATCH MENU VOCABULARY DISARMED (…)` line is emitted naming the reason, and the state returns to P1(b).*

### P4 — 🚨 **THE PLAYER'S KEYS — THIS IS THE ONE THAT PROTECTS THE MATCH**
*At no point does any `IMC_Hero` key change meaning.* Observable: **`Enter` still toggles the assistant console in a live match, screen open or not.** Statically checkable in the same breath: `InMatchMenuMappingContextPriority` (0) is strictly less than `HeroMappingContextPriority` (1), so `AppliedKeys` can never contain an `IMC_Hero` key when the menu context is walked.

### 🚨 P0 — **THE EXPECTED STOP-NAME LIST, STATED BEFORE 5b SO IT CAN BE FALSIFIED**

**Names · classes · construction route · tree order, for a live `L_Arena` match in this binary:**

> ⛔ **THE SET IS EMPTY, IN EVERY IN-MATCH STATE REACHABLE TODAY. Zero stops. No names, no classes, no construction route, no tree order — because there is no tree.**

**Why, and it is measured by two independent instruments, not assumed:**
1. **Source census** — the only callers of `RegisterMenuNavTarget` in `Source/` are `AccountMenuWidget.cpp:1677`/`:1761`, `DeckBuilderWidget.cpp:2132`, `SessionMenuWidget.cpp:349`, `SettingsMenuWidget.cpp:762`, `SiegeGraphicsMenuWidget.cpp:3160`. **All five are main-menu screens.**
2. **Blueprint census** — `grep -rl "RegisterMenuNavTarget" --include=*.uasset Content/` returns **ZERO**. A Blueprint graph that called it would carry the function name in its package name table, so this is a real negative and not an absence of looking.

⇒ **No in-match screen registers today.** The in-match screens are `TASK-1432` (controls help) and `TASK-1434` (assistant console), **both gated on this row's `qa-passed`**, and the victory screen registers nothing.

**⛔ THEREFORE — AND THIS IS DECLARED AT AUTHORING TIME, NOT SPRUNG AT THE SITTING:**
**P2 and P3 are `UNOBSERVABLE` in this binary.** There is no way to make `IsInMatchScreenOpen()` true in a match until `TASK-1432` or `TASK-1434` lands, and `RegisterMenuNavTarget` is not in the verifier's tool verbs. ⛔ **5b must not be scored against the positive limb, and a `VERIFY-FAILED` on P2/P3 would be a failure of the fixture, not of this diff.** What 5b **can** falsify today: **P0, P1 and P4** — which together are the whole of the regression fence, i.e. the risk this row actually carries.

**THE UNHAPPY-PATH COUNT AND ITS DISCRIMINATOR** (both **0 stops**, distinguished by the log, never by a number):

| Path | Stops | Discriminator |
|---|---|---|
| **Happy** — nothing registered, correct | **0** | `MoveFocus(+1) declined: menu covered.` — or no line at all in state (a) |
| **Unhappy** — something registered with an empty or unreachable tree | **0** | `MoveFocus(+1) declined: no menu buttons.` ⛔ **This string is the defect signature.** The two declines are byte-distinct and already shipped; no new instrumentation is needed. |
| **Unhappy** — something registered that should not be | ≥1 | any `MoveFocus(+1): focus moved …` line with no preceding `IN-MATCH MENU VOCABULARY ARMED` |

⛔ **ANCHORING:** every widget name that reaches a report from this row must carry its owning blueprint or be marked `UNANCHORED`. `MoveFocus` prints a bare `GetName()` with no screen qualifier — I did **not** change that line (it is a published discriminator for `TASK-1395`/`TASK-1402`), so **any name it prints is UNANCHORED by construction** and must be paired with the `IN-MATCH MENU VOCABULARY ARMED` line, which **does** name the registered screen. That pairing is the anchor.

---

## 8. THE DIFF

### `SiegeMenuInputSubsystem.h` (+266 lines, **1 deletion**)
- `class UEnhancedInputComponent;` forward declaration (+ why a `TWeakObjectPtr` needs only that).
- Class comment: new section **"🚨⭐⭐ THE IN-MATCH ARM (TASK-1429)"** — the predicate, the default-state proof, the full census, the (3) priority argument, the (5) measurement, the dual-lane finding, the (6)/(6b) answer, the backstop's one-way property.
- Class comment: the fence bullet *"Nothing here runs on any map but `L_MainMenu`"* **qualified, not deleted** — it would otherwise have become a lie in the file's own documentation.
- `MenuMapName` doc widened to *"the only map this subsystem arms on **AT LEVEL BOOT**"*.
- New constants `InMatchMenuMappingContextPriority = 0`, `InMatchDemandPollSeconds = 0.2f`, each with its own justification and each deliberately **separate from** the same-valued menu-map constant.
- `RegisterMenuNavTarget` / `UnregisterMenuNavTarget` public docs: what the call now costs off the menu map — written there because that is the declaration `TASK-1432`/`TASK-1434`'s authors will read.
- New private methods `IsInMatchScreenOpen` · `ReconcileInMatchArming` · `ArmInMatchMenuVocabulary` · `DisarmInMatchMenuVocabulary` · `PollInMatchDemand`.
- New private members `InMatchDemandPollTimerHandle` · `BoundInMatchInputComponent` · `bInMatchVocabularyArmed`.

### `SiegeMenuInputSubsystem.cpp` (+287 lines, **0 deletions**)
- `OnWorldBeginPlay`: **comment only.** ⛔ Zero statements changed.
- `Deinitialize`: `DisarmInMatchMenuVocabulary(TEXT("Deinitialize"))` + `BoundInMatchInputComponent.Reset()`, placed **before** the asset pointers are nulled.
- New block after `IsNavTargetActionable()`: the five functions.
- `RegisterMenuNavTarget`: one `ReconcileInMatchArming(..., true)` after the stack add.
- `UnregisterMenuNavTarget`: one `ReconcileInMatchArming(..., true)` after the `RemoveAll`, **above** the `!bWasRegistered` early-return (that return is about whether to *log*; the input layer must reconcile either way).

### ⛔ DELETION CENSUS — **EXACTLY ONE, AND IT IS A COMMENT**
```
-	/** The ONLY map this subsystem arms on (short package name, PIE prefix stripped). */
```
Replaced by a six-line block whose first sentence preserves it verbatim. **No code statement, no log string, no declaration was deleted by this row.**
*(Verified: `git diff` vs HEAD shows 21 deleted lines across the two files; the other 20 are `TASK-1406`/`1409`/`1469`'s already-`built` work — `Buttons`→`Stops`, `Button`→`Widget`, `SlateButton`→`SlateWidget`, `UCLASS()`→`UCLASS(BlueprintType)`, and two doc comments.)*

### ⚠️ THE C4458 SWEEP — SWEPT AGAINST THE **BASE CHAIN**, NOT AGAINST THE REPOSITORY
The compile trap is a local shadowing an **engine** base-class member, and a `Source/` grep is a false negative by construction. Swept against `USiegeMenuInputSubsystem`'s actual chain — `UWorldSubsystem` → `USubsystem` → `UObject` → `UObjectBaseUtility` → `UObjectBase` (verified: no interface, no secondary base). **Their complete data-member set:** `bHasCalledPostInitialize`, `bHasCalledBeginPlay` · `InternalOwningSubsystem` · `NamePrivate`, `ClassPrivate`, `OuterPrivate`, `ObjectFlags`, `InternalIndex`, `ObjectListInternalIndex`.
- **Genuinely new names introduced:** `Reason`, `bMayArm`, `bWantArmed`. Grepped against all five base headers ⇒ **zero occurrences**. ✅
- **Reused names:** `World`, `PC`, `LocalPlayer`, `InputSubsystem`, `InputComponent` — all already locals in `OnWorldBeginPlay` **in this same file**, which compiled clean under `-WarningsAsErrors` at 02:42:30 (`TASK-1469` 5a, action `[7/11]`, 0 diagnostics). That is an *actual compile*, not a grep. ✅
- **Own-class members:** no new local shadows any of the 20 members or statics of `USiegeMenuInputSubsystem`. ✅
- **Encoding:** the new log literals contain only `—` (U+2014), the identical non-ASCII codepoint already present in 19 pre-existing `TEXT()` literals in this file. (Files are CRLF throughout, no BOM, unchanged — matching every neighbour.)

### ⚠️ THE REFLECTION SURFACE IS UNCHANGED
**No new `UPROPERTY`, no new `UFUNCTION`, no new `UCLASS`/`USTRUCT`/`UENUM`, no new GC surface.** The three new members are plain C++ (a `FTimerHandle`, a `TWeakObjectPtr`, a `bool`) — exactly the shape `bArmed` and `FocusReentryPollTimerHandle` already have. `BoundInMatchInputComponent` is deliberately **not** a `UPROPERTY`: a weak pointer needs no reflection to be safe, and adding one would move the very surface this row asserts it does not move. ⇒ **`SiegeMenuInputSubsystem.generated.h` should be byte-identical after UHT re-parses the header.** The build-master measures exactly that; this is the prediction to check it against, and a change there would be a real finding.

---

## 9. What `TASK-1430` should scrutinise hardest

1. **The default-state branch (its (1)).** The claim is that `OnWorldBeginPlay`'s diff is *comment only*. That is checkable in one hunk — please check it rather than accept it.
2. **`bArmed` as the menu-map guard.** The whole safety of leaving the five `built` screens untouched rests on `bArmed` being true on `L_MainMenu` and nowhere else. Please verify there is exactly one `bArmed = true` and that it is below the map gate.
3. **The reconcile call placed *above* the `!bWasRegistered` early-return in `UnregisterMenuNavTarget`.** I argue it belongs there because that return governs *logging*, and because the same `RemoveAll` also drops dead entries and can empty the stack on a call that "found nothing". If you disagree, that is the line to argue about.
4. **The bind-once ledger.** `BindAction` appends; the in-match arm can be reached many times in one world. If `BoundInMatchInputComponent` is wrong, every handler fires N times per press after N opens.
5. **The `Enter` decision (its (3)/(4)).** ⛔ I deliberately did **not** ship the priority the spec named. §3 is the argument and the letter. If you think spec (3) binds over spec (2) here, that is a `FAIL` and I will take it back — but please rule on it rather than pass it through silently.
6. **`DisarmInMatchMenuVocabulary`'s early-return branch** — the one path where the function does not do what its name says. It logs; is that the right call, or should it warn?

---

## 10. `## Not examined / limitations`

- ⛔ **No compile, no PIE, no injection, no MCP mutation, no git.** Nothing in this handoff is a runtime observation of *this* diff; every measurement is a source read or a read-only asset query. `TASK-1436` owns runtime.
- ⛔ **P2/P3 are `UNOBSERVABLE` in this binary** (§7) — no in-match screen registers today. Declared here, at authoring time, so it cannot surprise the sitting.
- ⛔ **The suite is not cited as evidence.** `GetMenuButtons()` and `IsNavFocusStop()` have diverged and agree only on the main menu (`TASK-1475` is boarded to close it), so a green suite is not evidence for walker behaviour anywhere else. I make **one structural claim** about the test, which is not an appeal to its result: `SiegeMenuInputTest.cpp` opens `/Game/Maps/L_MainMenu` (`:118`) and never calls `RegisterMenuNavTarget`, so every code path this row added early-returns on `bArmed` for it.
- ⚠️ **The census is scoped to contexts applied *in a match today*.** It would be wrong by five keys if a template controller (`IMC_Default`) ever entered the arena. The expiry condition is written into the header beside the census.
- ⚠️ **The held-key-at-arm edge (one extra menu step) is reasoned from `ETriggerEvent::Started` semantics, not measured.** It is bounded by the priority choice (it can never be a gameplay action), but the exact behaviour of a key already down when a context arrives is an Enhanced Input detail I did not walk to the bottom.
- ⚠️ **The backstop is disarm-only**, which means a registered screen that is *hidden and then re-shown* without re-registering would lose the context and not get it back until something re-registers. No shipped screen does that, and `UAccountMenuWidget::ApplyMode`'s re-registration would re-arm. I chose the one-way property over covering that case, and I would rather be told I chose wrong than have it discovered later.
- ⚠️ **`RemoveMappingContext`'s interaction with a key held across the removal** is argued from the priority choice (we displaced nothing, so nothing is handed back) rather than from reading Enhanced Input's rebuild path for the removal case specifically.
- ⛔ **I did not census Blueprint graphs for calls to any subsystem function other than `RegisterMenuNavTarget`.** The name-table grep is a good instrument for that one name; I did not run it for the whole API.
- ⛔ **The `Enter` ruling is not mine and is not resolved.** Two rows (`TASK-1432`, `TASK-1434`) are released by my `qa-passed` and both should be told the current answer: in-match, `Enter` stays with `IA_AssistantConsole`.
- ⛔ I did not implement, stub, or leave a hook for Row B's (`TASK-1471`) per-screen "this screen drives itself" flag. Grep: no such flag, member, constant or comment-promise exists in this diff.
