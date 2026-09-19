# TASK-1311 — [VICTORY-FOCUS-EMITTER] — programmer handoff

**Status delivered: `ready-for-qa`** (gate = `TASK-1312`, host = `TASK-1313`)
**ROUTE TAKEN: ⛔ ROUTE B (the default) — code side. NO `.uasset` in the diff ⇒ 🧑 Jonathan is NOT needed for a hash discharge.**
Marker: `TASK-1311-VICTORY-FOCUS-EMITTER` · programmer: gameplay-programmer · date: 2026-09-19

---

## §0 — THE FOUR READS, MEASURED AT MY INSTANT (`SC-§91`), WITH ACTUAL VALUES

Tree state when I started: `main` at `d818b5e`, **0 ahead of origin**, `git status` clean. `TASK-1298` shipped as `f41fd67` + `d818b5e`. The `blocked-by` serialization fence is discharged.

### (a) `/Game/UI/WBP_VictoryScreen` CDO `bIsFocusable` — **`False`**

Read by me, not inherited from QA. Instrument: `mcp__unreal_inspector__get_asset_meta /Game/UI/WBP_VictoryScreen PropertyValues` (read-only lane; the editor was up and I opened **no** Blueprint editor, ran **no** compile, issued **no** save). Verbatim from the `Default Property Values` block:

```
  uint8 bIsFocusable = False
  FWidgetChild DesiredFocusWidget = ()
```

**Independent corroboration, different instrument** (`SC-§39` — one reading is not a measurement): the string `bIsFocusable` is **absent from the package's name table entirely** — `strings -a Content/UI/WBP_VictoryScreen.uasset | grep Focusab` returns **0 hits**. A UE property name enters the name table only when the value is serialized, i.e. only when it differs from the archetype. ⇒ the property was **never overridden** and the CDO inherits `UUserWidget`'s `false`. Both instruments agree.

`Content/UI/WBP_VictoryScreen.uasset` — **UNTOUCHED BY ME**, recorded for `SC-§68`:
- sha256 `7817dd7f19bcf1028bd21ddfc4d92731c01c14d567c19641cc10e9f1d12bd6ee`
- 153,489 bytes · mtime `2026-07-29 12:23:05 -0700` (unchanged; it is **not** in my diff)

### (b) `SiegePlayerController.cpp` — the shipped block, VERBATIM, pre-edit

⚠️ **THE BOARD'S LINE NUMBERS PREDATE `f41fd67` AND I RE-MEASURED THEM.** The board says `:2264-2270`; the orchestrator's dispatch said the call sat at `:2266`. **Both are wrong about the call itself: the `SetWidgetToFocus` call was at `:2267`.** The board's `2264-2270` is correct as the *block* range (`FInputModeUIOnly InputMode;` at 2264 → `SetInputMode(InputMode);` at 2270). Pre-edit, `2261-2270`:

```cpp
	// UI-only input for the end screen (GDD §3.9)
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	FInputModeUIOnly InputMode;
	if (VictoryWidget)
	{
		InputMode.SetWidgetToFocus(VictoryWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
```

File pre-edit: sha256 `bae4bcf79736fe6b81705e41849a5253705de048cf6b6789e0b57513e5f3848b`, 7,433 lines.

### (c) THE CAUSE CHAIN, READ AT ENGINE SOURCE — both board line numbers CONFIRMED EXACTLY

`C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/UMG/Private/Slate/SObjectWidget.cpp:175`:
```cpp
bool SObjectWidget::SupportsKeyboardFocus() const
{
	if ( CanRouteEvent() )
	{
		return WidgetObject->NativeSupportsKeyboardFocus();
	}
	return false;
}
```

`C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Private/PlayerController.cpp:6340-6347` — **`:6345` is the emitter line, exactly**:
```cpp
FInputModeUIOnly& FInputModeUIOnly::SetWidgetToFocus(TSharedPtr<SWidget> InWidgetToFocus)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (InWidgetToFocus.IsValid() && !InWidgetToFocus->SupportsKeyboardFocus())
	{
		UE_LOGF(LogPlayerController, Error, "InputMode:UIOnly - Attempting to focus Non-Focusable widget %ls!", *InWidgetToFocus->ToString());
	}
#endif
	WidgetToFocus = InWidgetToFocus;
	return *this;
}
```

Chain closes: `bIsFocusable = False` → `NativeSupportsKeyboardFocus()` false → `SObjectWidget::SupportsKeyboardFocus()` false → the `Error` at `:6345`, in every non-Shipping/non-Test build. `qa/TASK-1297-report.md` §E read whole (`SC-§38a`); its WARN paragraph is the origin of this row and its `False` reading matches mine.

### (d) THE PIN CENSUS — **UNPINNED, AND THE `0` IS QUALIFIED**

```
$ grep -rn 'AddExpectedError' Source/GitClaudeUnrealTest/Siegebound/Tests/
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp:1475://  ⛔ ALL now suite-visible. ⛔ DO NOT ADD `AddExpectedError` TO THIS TEST — that would restore the
```

**1 raw hit, and it is a COMMENT forbidding the construct — therefore 0 executable `AddExpectedError` calls in `Tests/`, and 0 pinning ANY string.** That single hit doubles as the **positive control** `SC-§39` wants: the needle fires where the token exists, so the zero is a measured zero and not a broken grep.

Second, narrower census: `grep -rn 'Non-Focusable\|NonFocusable\|SetSuppressLogErrors' Source/` → **0 hits repo-wide**. Nothing in our code references the Error string in any form.

⇒ **THIS EMITTER IS UNPINNED. THERE IS NO COUPLED TEST DELETION IN THIS ROW, AND NO COUPLING CHECK FOR THE GATE TO FIND** — unlike `TASK-1296`, whose entire gate was that coupling. Per `SC-§59` cl. 5 I censused it to prove the zero and stopped; `TASK-1312` should state *"unpinned ⇒ no coupling check applies"* rather than hunt for a missing half.

**And the consequence, because it is why this is a defect and not tidiness** (`SC-§70`): unpinned means any future automation test that ends a match inside a capture window reds on an undeclared `Error` with nothing to absorb it. Here the absence *is* the hazard.

---

## §1 — THE ROUTE CHOICE, AND IT FOLLOWS FROM THE READS

**ROUTE B.** The row's default (`SC-§121` cl. 6, the reversible-path rule), and Route A's gate is **empty**.

### Route A's positive-evidence gate: I looked in all three places the row named. ZERO.

Route A is authorised **only** on positive evidence that a player is *meant* to reach Play Again by key or pad — a `KBD-§` row, an `IMC_`/`IA_` binding, or a GDD sentence, named at `file:line`. My census:

1. **`IMC_`/`IA_` binding — NONE EXISTS, AND THE EXCLUSION IS STRUCTURAL.** 28 `IA_*` assets under `Content/Input/Actions/`; **none** is a Play Again / Restart / end-screen action. The three menu actions that *could* drive a button — `IA_MenuUp` / `IA_MenuDown` / `IA_MenuAccept` — are loaded and bound **only** by `USiegeMenuInputSubsystem`, and that subsystem **hard-returns on any map that is not the main menu**:
   - `SiegeMenuInputSubsystem.cpp:25` — `const TCHAR* USiegeMenuInputSubsystem::MenuMapName = TEXT("L_MainMenu");`
   - `SiegeMenuInputSubsystem.cpp:47` — `if (MapName != MenuMapName) { return; }`
   The victory screen appears on **`L_Arena`**. ⇒ **the only menu-key lane in the project is unreachable from the end screen, by construction.**
2. **GDD — names the feature, never a key.** `Docs/GDD.md:169` (*"Victory/Defeat screen with **Play Again**"*), `:170`, `:491` (*"End screen: Victory / Defeat + \"Play Again\""*). No key, no pad button, anywhere. `Docs/GDD.md:307` goes the other way: it lists Play Again among the things needing *"a keypress, a click, a card or an order"* and records it as **unobserved**, claiming nothing.
3. **`KBD-§` — no row, and the section is a different feature.** `KBD-§4` (`CONVENTIONS.md:2525`) is the letters-only layout-translation table and **deliberately excludes `Enter`, `Space`, `Escape`** and the mouse. It is not a binding register for this screen. Censused against the closed-ruling register per `SC-§121` cl. 2: **no ruling touches this.** I add no key, so no register entry is owed.

⇒ **Route A without that evidence is a BLOCKER by the row's own words.** I took Route B.

### ⭐ A MEASUREMENT THAT CUTS AGAINST ROUTE A HARDER THAN THE BOARD ANTICIPATED — flagged, not acted on

The board's honest comparison says Route A *"removes the `Error` AND makes Play Again key-reachable, which is arguably the real bug."* **My read-back says the second half of that is not true on its own.** From the same `get_asset_meta` call:

```
  FWidgetChild DesiredFocusWidget = ()
```

`DesiredFocusWidget` is **empty**, so `NativeGetDesiredFocusTarget()` has nothing to redirect to and focus under Route A would land on the **widget root**, not on any button. The one `Button` the tool printed in the tree (`Btn_Jump`) reads `IsFocusable=False`.

⇒ **flipping the root's `bIsFocusable` alone would silence the `Error` but would NOT make Play Again activatable by Enter or gamepad-A.** Route A is therefore not the cheap version of the product fix — it is the `Error` fix plus a misleading half-step. That strengthens, rather than weakens, the reversible-path default.

⚠️ **Stated at its true strength (`SC-§101`):** this is a **CDO read**, not a runtime focus observation. I did not (and may not) drive PIE. The tree listing the tool returned also looks partly template-derived (`Btn_Jump`, two `UI_Thumbstick_C`), and the **Play Again button is fenced from me**, so I did not open the asset to reconcile it. What I assert is exactly what the instrument printed: `DesiredFocusWidget` is empty on the CDO.

### 🧑 THE PRODUCT QUESTION — RECORDED FOR THE MANAGER, NOT FOLDED IN

Whether Play Again / the victory screen **should** be keyboard-or-gamepad reachable is a feature with its own row and its own acceptance. **Route B leaves the end screen mouse-only, which is what ships today — nothing regresses.** My measurements bear on it and the manager should have them when boarding it:
- it is **not** a one-property change: the root's `bIsFocusable`, a `DesiredFocusWidget` target, and a focusable Play Again button are **three** edits, at least one of which touches the fenced button;
- there is **no input lane on `L_Arena`** to carry it (`SiegeMenuInputSubsystem.cpp:47` returns), so it likely needs an `IA_`/`IMC_` addition too — which drags in `SC-§121`'s register census and 🧑 his word on the key;
- and it carries the 🧑 **BP-editor hand-save** cost (`SC-§125` cl. 3), because a `.uasset` is unavoidable.

**I did not act on any of this.** No quiet rider.

---

## §2 — THE 🧑 HUMAN-KEYSTROKE COST: **NOT INCURRED**

**Route B carries NO `.uasset` and NO keystroke.** `git status` shows exactly one modified file and it is a `.cpp`. There is **no** hash-must-change discharge owed on this row, so `SC-§125` / `VER-§8` cl. 2's `.uasset` twin does not bite here and `TASK-1313` owes 🧑 Jonathan nothing at 5a.

I made **no** `save_assets` call, **no** `execute_unreal_python` call, **no** metadata-tag write, and opened **no** Blueprint editor. My only engine contact was one **read-only** `get_asset_meta`.

---

## §3 — THE DIFF

`git diff --stat` → **1 file, 6 insertions(+), 4 deletions(-)**. `git status --short` → `M Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` (plus this handoff).

```diff
 	// UI-only input for the end screen (GDD §3.9)
+	// ⛔ NO FOCUS TARGET IS SET ON THIS INPUT MODE, AND THE ABSENCE IS DELIBERATE (TASK-1311).
+	// /Game/UI/WBP_VictoryScreen's CDO is bIsFocusable = False, so the engine's focus-target
+	// setter (PlayerController.cpp:6345, via SObjectWidget.cpp:175) logs an Error and focus
+	// never lands — asking for it was already a no-op, so not asking changes nothing a player
+	// sees. ⛔ Do not re-add it unless the widget root is made focusable first; whether Play
+	// Again should be key/pad-reachable at all is a product question owed to Jonathan.
 	bShowMouseCursor = true;
 	bEnableClickEvents = true;
 	FInputModeUIOnly InputMode;
-	if (VictoryWidget)
-	{
-		InputMode.SetWidgetToFocus(VictoryWidget->TakeWidget());
-	}
 	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 	SetInputMode(InputMode);
```

The `if (VictoryWidget)` block became empty once the call went, so it went with it — the row's Route B text sanctions exactly that.

Post-edit file: sha256 `6a4d107be3d3606e42edca02cd8711cb172219521261dee79d72f17105419305`, 7,439 lines (pre-edit `bae4bcf7…`, 7,433).

### 🚩 ONE DECLARED ADDITION BEYOND THE LITERAL `names:` WORDING — FLAGGED, NOT SMUGGLED (`SC-§121` cl. 5)

The `names:` fence reads *"the `:2264-2270` `SetWidgetToFocus` line **ONLY**"*. **I also added a 6-line comment**, which is a third thing. My reason: the emitter is **unpinned**, so nothing in the suite stops a future reader re-adding the call and restoring the `Error` — `SC-§70`'s "an absence with no enforcer is half a fact" is this row's own framing, and a comment is the only enforcer available to a code-side fix. `SC-§121` cl. 5's positive exhibit praises raising it **at the code site** as well as in the handoff.

**It is zero executable lines and it is a one-line-block delete to undo.** ⚖️ **If `TASK-1312` reads the fence strictly, the board wins and I defer — delete the comment block, nothing else changes.**

### ⚖️ WHERE THE PROMPT AND THE BOARD DISAGREED, AND THE BOARD WON — TWICE

1. **The dispatch prompt said the call was at `:2266`; the board said `:2264-2270`.** I measured **`:2267`** for the call and confirmed `2264-2270` as the block. I trusted neither on faith.
2. **My first draft of the comment spelled `SetWidgetToFocus` twice, which made acceptance (4a)'s `grep -c` return `2` instead of the required `0`.** The board's acceptance clause is literal and measurable; my judgement about a nicely-worded comment is not. **I reworded the comment to avoid the token entirely rather than argue the clause.** The clause now passes on its own terms.

---

## §4 — ACCEPTANCE

**(4a) STATE READ — Route B limb:**
```
$ grep -c 'SetWidgetToFocus' Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
0
$ grep -rn 'SetWidgetToFocus' Source/
(no hits — 0 repo-wide)
```
**Surrounding shipped lines byte-identical**, as the diff above shows with 6 lines of context: `bShowMouseCursor = true;` · `bEnableClickEvents = true;` · `FInputModeUIOnly InputMode;` · `SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);` · `SetInputMode(InputMode);` all unchanged. The `FInputModeUIOnly` posture is untouched (fence (3)(d) clean).

**(4b) RUNTIME — the verifier's, not mine.** Criterion: `InputMode:UIOnly - Attempting to focus Non-Focusable widget` appears **0 times** in the log of a PIE instance that **actually reached a match end on `L_Arena`**. Per the row, a `0` from a run that never ended a match is `SC-§39`'s pin-that-cannot-fail and is **not** this criterion; `UNOBSERVABLE` with the reason is a legitimate verdict here (`VER-§5`, ceiling declared at boarding). I did not drive PIE and I claim nothing about runtime.

**(4c) 🧑 HIS HAND CHECK — reproduced verbatim, parenthesis intact** (`VER-§8` cl. 3(b)): ***"Play a match to the end on the arena; when the Victory/Defeat screen appears, does clicking 'Play Again' restart the match? (The screen merely appearing is a NO. And under route B a keyboard press doing nothing is EXPECTED, not a failure — the only question is whether the CLICK still works.)"***

**(4d) Compile / suite / `git status` / hashes — `TASK-1313`'s**, accepted-as-declared by me. I did **not** compile, did **not** run the suite, did **not** touch Git. This row adds no test ⇒ suite total = baseline ±0, to be reconciled **by name**, not by total (`SC-§104`).

---

## §5 — FENCES, EACH CONFIRMED CLEAN

| Fence | State |
|---|---|
| (3)(a) Play Again button / `OnClicked` / `SetWinner` / `SetLocalVictory` | **untouched** — diff = 0; `SetWinner` (2202-2216) and `SetLocalVictory` (2229-2243) are byte-identical and outside my hunk |
| (3)(b) no duplicate-and-reparent of `WBP_VictoryScreen` | **N/A and answered directly for the gate: I did not open, duplicate, reparent, edit or save any WidgetBlueprint.** No `.uasset` in the diff at all |
| (3)(c) no suppression substituted | **clean** — 0 `AddExpectedError`, 0 `SetSuppressLogErrors`, no verbosity change, no `#if` near the engine site. The **absence of the `Error` is the assertion** |
| (3)(d) `FInputModeUIOnly` posture unchanged | **clean** — all five lines byte-identical (see the diff) |
| (3)(e) out of bounds | **clean** — `WBP_HUD` · `WBP_CardHand` · `L_Arena.umap` · `Content/Dev/**` · `Saved/**` · `TASKBOARD.md` · `CONVENTIONS.md` · `settings.local.json` · the `.uproject` all absent |
| (3)(e) **zero files from `TASK-1298`'s pathspec** | **clean** — `Tools/run_suite_bounded.ps1` · `Tests/SiegeMenuInputTest.cpp` · `DeckBuilderWidget.{h,cpp}` · `Content/Blueprints/BP_MenuGameMode.uasset` **all absent from my diff.** `git status --short` returns exactly one `M` line |
| `TASK-1310` | different file, no shared host — no interaction |

### (3)(f) the older finding, named and left alone — ONE LINE AS INSTRUCTED

`qa/TASK-007-report.md` WARN: `HandleMatchEnd` goes `FInputModeUIOnly` **even when `VictoryWidget` is null**, a soft-lock in the degraded state. **Different defect, older, out of scope — I changed nothing about it.** (Noting only that removing the `if (VictoryWidget)` wrapper neither creates nor worsens it: the posture lines were always unconditional; the guard only ever wrapped the focus call.)

---

## §6 — WHAT QA SHOULD SCRUTINISE

1. **The route choice and its evidence** — §1's three-place census is the whole basis for not taking Route A. If you think a key/pad route to the end screen exists that I missed, `SiegeMenuInputSubsystem.cpp:47` is the line to attack.
2. **The declared comment addition** (§3's 🚩) — it exceeds the literal `names:` WRITES wording. I flagged it rather than ship it silently; rule on it either way and I defer to the board.
3. **A lost side effect? — I checked, and there is none.** The removed expression contained `VictoryWidget->TakeWidget()`, which is **not** pure: it builds and caches the underlying `SObjectWidget`. But `VictoryWidget->AddToViewport(/*ZOrder=*/ 10)` runs at **`:2245`**, *before* this block, and `AddToViewport` takes the widget internally. ⇒ the side effect was already discharged; removing the call drops nothing. Repo-wide `TakeWidget` census after the edit: **0 executable call sites** (6 hits, all comments). This is the one non-obvious way a "behaviour-preserving" deletion could have bitten, so please re-check my ordering claim.
4. **No dangling reference** — `VictoryWidget` remains used at 9 sites (`:2195` create, `:2196`, `:2203`, `:2208`, `:2230`, `:2235`, `:2245` AddToViewport, `:2313`, `:2315-2316` teardown). No unused-member or unused-local warning is introduced.
5. **Reachability of `HandleMatchEnd`** (your check (5), for your own reproduction, not accepted from me): declared at `SiegePlayerController.h:1218`, defined at `SiegePlayerController.cpp:2135`. Two callers, both ours — `SiegeGameMode.cpp:606` (`SiegePC->HandleMatchEnd(Winner)`) and `SiegeGameState.cpp:266` (`SiegePC->HandleMatchEnd(WinningTeam)`, the M8 client fan-out; the direct push loop is marked RETIRED at `SiegeGameMode.cpp:583`). Precondition is a castle reaching 0 HP, behind a `bMatchEnded` latch (`SiegePlayerController.cpp:2165` early-out). **Reproduce it yourself — the defect claim rests on it.**

---

## §7 — BOARD FLIP: ⛔ NOT DONE BY ME, AND IT IS OWED

**I did not flip `TASK-1311`'s `status:` line.** The row's own fence (3)(e) lists **`TASKBOARD.md` as OUT OF BOUNDS ENTIRELY** for this row, and the `names:` WRITES list does not include it. Per `SC-§120` cl. 4 I would have used `Edit` on my own line only — but the fence forbids the file outright, and the board wins.

⇒ **This row is `ready-for-qa` as of this handoff. THE FLIP IS OWED TO THE MANAGER (or to `TASK-1312`, whose own spec already says it flips `TASK-1311` and itself).**

---

## §8 — FILES

- **WROTE:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegePlayerController.cpp` (the `:2261-2270` block only) · this handoff
- **NO `.uasset` WRITTEN.** `Content/UI/WBP_VictoryScreen.uasset` was **read only**, sha256 `7817dd7f19bcf1028bd21ddfc4d92731c01c14d567c19641cc10e9f1d12bd6ee`, 153,489 bytes, unchanged
- **DID NOT:** compile · run the suite · touch Git · open/compile/save any Blueprint · drive PIE · edit `TASKBOARD.md` or `CONVENTIONS.md`

Law applied: `SC-§50` · `SC-§38a` · `SC-§39` · `SC-§59` cl. 5 · `SC-§68` · `SC-§70` · `SC-§91` · `SC-§101` · `SC-§104` · `SC-§120` cl. 4 · `SC-§121` cl. 2/5/6 · `SC-§125` · `VER-§5` · `VER-§8` cl. 2/3/4.
