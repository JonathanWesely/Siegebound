# TASK-1319 — [MATCHEND-UIONLY-SOFTLOCK] — gameplay-programmer handoff

**Marker:** `TASK-1319-MATCHEND-UIONLY-SOFTLOCK`
**Status when written:** work complete, **`ready-for-qa` NOT FLIPPED BY ME** — see §11.
**Gate:** TASK-1320 · **Host:** TASK-1321 · **5b:** `UNOBSERVABLE`, pre-authorised at boarding (§9).
**One-line result:** `HandleMatchEnd` no longer applies `FInputModeUIOnly` when no end screen exists; the
`VictoryWidget == nullptr` path now takes the controller's own in-match `FInputModeGameAndUI` + visible-cursor
posture and returns, so the viewport stops swallowing every key in a state where nothing can receive them.
**Files:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` (**66 added, 0 deleted, one hunk**) + this file.

---

## 1. (0)(a) — THE TWO DEGRADED BRANCHES, QUOTED, AND THE FALL-THROUGH QUOTED

Re-read whole at my own instant (`SC-§91`). The addresses **had** moved: at boarding the fall-through was cited at
`:2268`–`:2272`; after `1d433ca` the posture block starts at **`:2338`**. Quoted, never addressed by line alone.

**Degraded branch A — `CreateWidget` returned null** (inner `else`, was `:2247`–`:2252`):

```cpp
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': failed to create the victory screen from '%s'."),
				*GetNameSafe(this), *VictoryScreenClass.ToString());
		}
```

**Degraded branch B — the victory-screen class did not resolve** (outer `else`, was `:2254`–`:2259`):

```cpp
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': victory screen class '%s' not found (built in TASK-011) — match ended with no end screen."),
			*GetNameSafe(this), *VictoryScreenClass.ToString());
	}
```

**THE FALL-THROUGH AS IT STOOD AT `1d433ca` — quoted, not described.** Both branches above logged and then ran
straight into this, with nothing between them and it:

```cpp
	// UI-only input for the end screen (GDD §3.9)
	...
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	FInputModeUIOnly InputMode;

	// TASK-1314: put Slate keyboard focus on the Play Again button ...
	if (VictoryWidget)
	{
		...
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
```

⭐ **TASK-1314 made the contrast sharper exactly as the boarding analysis predicted:** the new focus block is itself
guarded by `if (VictoryWidget)`, so on the degraded path it is skipped — and then `SetInputMode(InputMode)` ran
anyway, applying **UI-only input with no focus target and no UI**.

**The predicate is sound and it was checked, not assumed (`SC-§70`):** `VictoryWidget` has exactly **two**
assignments in the file — `:2195` `VictoryWidget = CreateWidget<UUserWidget>(this, ScreenClass);` (inside the
class-resolved branch only) and `:2374` `VictoryWidget = nullptr;` in `HandleMatchReset`, under its
`if (IsLocalController())` guard — and `AddToViewport` is the **last, unconditional** statement of the
create-success branch. `HandleMatchEnd` itself returns early for a non-local controller, so the reset pairing holds.
⇒ **at that point, `VictoryWidget == nullptr` is equivalent to "nothing was put on screen this match".**

---

## 2. (0)(b) 🚨 WHAT A PLAYER CAN ACTUALLY STILL DO IN THAT STATE — THE SEVERITY, MEASURED AT SOURCE

**Answer: in the shipped (Shipping) build — NOTHING. Not one key. The claim is NOT downgraded; it is confirmed,
and it is worse in the package than in the editor.** The reads that establish it:

1. **UI-only turns game input off at the viewport.** `FInputModeUIOnly::ApplyInputMode` —
   `Engine/Source/Runtime/Engine/Private/PlayerController.cpp:6384`:
   `GameViewportClient.SetIgnoreInput(true);` (also `SetMouseCaptureMode(NoCapture)`, `SetMouseLockMode`).
2. **What `IgnoreInput()` does to a key — the enforcer, quoted** (`SC-§70`: an absence without its enforcer is half
   a fact). `UGameViewportClient::InputKey`, `GameViewportClient.cpp`:
   ```cpp
   if (IgnoreInput())
   {
       return ViewportConsole ? ViewportConsole->InputKey(EventArgs.InputDevice, EventArgs.Key, EventArgs.Event, EventArgs.AmountDepressed, EventArgs.IsGamepad()) : false;
   }
   ```
   This is **before** `OnInputKeyEvent`, before the override callback and before
   `TargetPlayer->PlayerController->InputKey(EventArgs)`. ⇒ **no key reaches the PlayerController, therefore none
   reaches Enhanced Input.** Every input action in the project is dead in this state — the same mechanism that let
   `TASK-1286` ship an entry path it could not execute.
3. **The one exception is the developer console — and it is compiled out of the package.** `ViewportConsole` is
   constructed under `#if ALLOW_CONSOLE` (`GameViewportClient.cpp:2807-2809`), and
   `Core/Public/Misc/Build.h` defines `ALLOW_CONSOLE` as `ALLOW_CONSOLE_IN_SHIPPING` for Shipping, with
   `#define ALLOW_CONSOLE_IN_SHIPPING 0` (Build.h:215, 348). ⇒ **in `Siegebound-Win64-Shipping` there is no console
   object at all**, so that branch returns `false` and the key is simply gone.
4. **No UI can receive it either**, because the premise of the state is that no widget was created. The HUD is still
   in the viewport and the cursor is up, so a mouse click can still hit a HUD widget — but every card play is
   refused while `bMatchEnded` is latched, so nothing a click reaches does anything.
5. **No other surface can be opened**, and this is refusal by design, not by accident:
   `CanOpenAssistantConsole` (`:6370`), `CanOpenWarMap` (`:6657`) and `CanOpenControlsHelp` (`:7072`) each
   begin `return !bMatchEnded && ...` — and their entry actions are input actions, which (2) already killed.
6. **`ApplyCursorInputState` cannot rescue it**: it early-outs on `bMatchEnded` (`:6293`) by design — *"HandleMatchEnd
   owns the UI-only end-screen state"*. Nothing else in the class owns the posture after match end.

⇒ **SEVERITY, STATED PLAINLY: in the editor/Development the player retains the tilde console and nothing else; in
the shipped game the degraded state is a total input blackout — no screen, no button, no key, only Alt+F4.** That is
a genuine soft-lock, and the WARN's *"it does not crash"* is the only mitigation that survives inspection.

---

## 3. (0)(c) — `qa/TASK-007-report.md`'s WARN, VERBATIM (the provenance this row discharges)

> - **[WARN] SiegePlayerController.cpp:386-394** — `HandleMatchEnd` switches to `FInputModeUIOnly` even when
> `VictoryWidget` is null (WBP_VictoryScreen missing, pre-TASK-011): no UI exists to click and no game input
> remains — a soft-lock in the degraded state. Suggest applying UIOnly only when the widget was created (else keep
> GameAndUI + cursor so the session stays inspectable). Not a blocker: it cannot occur once TASK-011 ships, it is
> logged, and it does not crash.

Declared there; named a second time by `TASK-1311` (3f) and correctly scoped **out**; named a third time by the
boarding analysis — **three declarations, zero rows** until this one. `SC-§50`, precedent `qa/TASK-849.md` N-5.

---

## 4. (1) THE REMEDY — CHOSEN, WITH THE ONES I REJECTED

### 4.1 The posture decision, in one sentence, from measured code

**When there is no UI to receive input, the controller must not take input away from the game: the degraded path now
applies the controller's own in-match cursor posture — `FInputModeGameAndUI` + `DoNotLock` +
`SetHideCursorDuringCapture(false)` with `bShowMouseCursor`/`bEnableClickEvents` true, copied term for term from
`ApplyCursorInputState`'s GameAndUI arm (`:6345`-`:6348`) — and it cannot strand the player because
`FInputModeGameAndUI::ApplyInputMode` calls `SetIgnoreInput(false)` (`PlayerController.cpp:6410`), so every key the
player had one second earlier is delivered again instead of being discarded at the viewport.**

### 4.2 Taken: shape (i)+(ii) — apply UI-only **only** when a widget exists, and keep game input live otherwise

- It is the WARN's **own** suggestion, so this row discharges the declaration in the terms it was declared.
- It reuses a posture that already ships in this class on five other surfaces — **zero new behaviour invented**.
- It binds **nothing** (§5) and is a strict superset of what the player could do before: `SetIgnoreInput(false)`
  restores delivery; no route is removed.
- ⛔ **It opens no NEW route out, and that is deliberate.** The match-end refusals in §2 item 5 are untouched. I did
  not widen them: that would be a product change nobody asked for, in a row about an input posture.

### 4.3 Rejected

- **(iii) "synthesise a minimal exit"** (a key that returns to the menu / restarts) — **REJECTED as out of scope and
  fenced.** It would be a fresh key binding, which `SC-§121` says must be censused and put to Jonathan before it is
  written, and the dispatch is explicit that this row binds nothing. It is also **structurally impossible in the
  shape this row is allowed to take**: under any UI-only posture the key would be eaten at the viewport (§2 item 2),
  so it would have to change the posture anyway — which is what 4.2 already does, without the key.
- **An `Escape` absorb** — **REJECTED, fenced at boarding, and a BLOCKER by the spec's own words.** `AS-§6 A-2`
  binds everywhere outside his `TASK-1300` scoping (main-menu widgets), and the in-match cancel routes are the part
  A-2 still binds *unrelaxed*. There is no `EKeys::Escape` in this diff (§5).
- **Calling `ApplyCursorInputState()` instead of stating the posture inline** — **REJECTED because it is a no-op
  here**: it early-outs on `bMatchEnded`, which was latched ~40 lines above. Calling it would have compiled, reviewed
  clean, and done nothing — the `SC-§36.1` shape. The rejection is recorded in the code comment so the next reader
  does not "simplify" it back.
- **Removing the now-redundant `if (VictoryWidget)` guard around TASK-1314's focus block** — **REJECTED on the
  byte-identity fence.** See §7 and §10.

---

## 5. (2)(a) + `SC-§121` — THE KEY CENSUS, STATED

**Result: the set of keys this row makes live is EMPTY.** No key is bound, rebound, absorbed or made live, so there
is nothing to census against the closed-ruling register and **nothing to escalate to Jonathan**. This is the
expected answer named in the dispatch. Measured on the added lines of the diff (`git diff -U0 | grep "^+[^+]"`):

| token on added lines | count |
|---|---|
| `EKeys::` | **0** |
| `IA_` | **0** |
| `IMC_` | **0** |
| `BindAction` | **0** |
| `UEnhancedInput` | **0** |
| `VictoryWidget->TakeWidget` | **0** |
| `SetWinner` / `SetLocalVictory` | **0** / **0** |

Whole-file `EKeys::` count is **12 — identical to the pre-edit baseline** (measured 12 before editing). An earlier
draft of one *comment* line mentioned those token names in prose and pushed the file count to 13, which would have
red-flagged a grep census; I reworded the comment and the count is back at 12. That is the only reason the diff is
66 lines and not 65.

**`AS-§6 A-2`:** untouched, not relied on, not cited as authority for anything in the code. The comment names the
ruling only to record that it was checked.

---

## 6. (2)(b) 🚨 THE SUCCESS PATH IS BYTE-IDENTICAL — PROVEN BY CONSTRUCTION, NOT ASSERTED

```
$ git diff --numstat -- .../Siegebound/SiegePlayerController.cpp
66      0       GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp

$ git diff -U0 -- .../SiegePlayerController.cpp | grep -c "^-[^-]"
0

$ git diff -- .../SiegePlayerController.cpp | grep "^@@"
@@ -2258,6 +2258,71 @@ void ASiegePlayerController::HandleMatchEnd(ETeamId Winner)
```

**Zero deleted lines, one hunk, pure insertion.** No pre-existing byte of the file changed — not the posture, not
the focus block, not a tab of indentation. The four posture lines TASK-1314's acceptance pins, quoted as they stand
**after** my edit (now at `:2338`, `:2339`, `:2394`, `:2395`):

```cpp
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	FInputModeUIOnly InputMode;
	...
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
```

The shipped happy path is intact end to end: victory widget created, `SetWinner`/`SetLocalVictory` called by name,
`AddToViewport(10)`, `GetWidgetFromName("Btn_Jump")`, the `SupportsKeyboardFocus()` guard, `SetWidgetToFocus`, then
`FInputModeUIOnly` with that focus target. 🧑 Jonathan hand-confirmed **both** routes live (*"clicking Play Again
does restart the match"*, *"pressing enter does trigger the 'play again' button"*) — nothing here can regress that,
because on the widget-exists path **not one instruction changed**.

⛔ `VictoryWidget->TakeWidget()` occurrences in the file: **0** (the only `TakeWidget` is TASK-1314's
`PlayAgainButton->TakeWidget()`, which I did not touch). TASK-1311's deleted call was **not** re-added.

---

## 7. (4)(a) THE DEGRADED-PATH HUNK, QUOTED WHOLE (code lines; the comment block sits above it in the file)

Inserted between the outer `else`'s closing brace and the `// UI-only input for the end screen (GDD §3.9)` comment:

```cpp
	if (!VictoryWidget)
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;

		FInputModeGameAndUI DegradedInputMode;
		DegradedInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		DegradedInputMode.SetHideCursorDuringCapture(false);
		SetInputMode(DegradedInputMode);

		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': match ended with NO end screen — input left Game-and-UI with the cursor up instead of UI-only, because UI-only with nothing on screen swallows every key (qa/TASK-007-report.md WARN; TASK-1319)."),
			*GetNameSafe(this));

		// ⛔ THE SAME COMPLETION RECORD THE SUCCESS PATH WRITES BELOW, RE-EMITTED DELIBERATELY RATHER
		// THAN SHARED: hoisting it above that block would re-order the success path's logs, and
		// wrapping that block in an else would re-indent it — and TASK-1314 shipped it at 1d433ca on
		// the promise that it stays BYTE-IDENTICAL (TASK-1320 check 1 measures exactly that). A
		// duplicated three-line log is the cheaper of the two costs.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': match ended — winner %s."),
			*GetNameSafe(this), Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
		return;
	}
```

**New behaviour on the degraded path:** cursor up, clicks enabled, `FInputModeGameAndUI` with `DoNotLock` and the
cursor visible through captures, **game input delivered** (`SetIgnoreInput(false)`), one Warning that names the
state and its provenance, the unchanged winner record, and an early return so the UI-only block is never reached.
**New behaviour on the success path: none.**

**Why an early return rather than an `if/else` around the UI-only block:** an `else` would have re-indented every
line TASK-1314 shipped hours ago, breaking the byte-identity the gate measures. The cost is that the
`match ended — winner %s.` log now has **two** sites instead of one. I checked before duplicating: the string had
exactly one site and **no** consumer anywhere in `Source/`, `Tools/` or the suite, so nothing reads it that could
drift. If QA prefers a single site at the price of re-indentation, say so and I will do it — that is a deliberate
trade, not an oversight.

---

## 8. (4)(b) THE TEST — **DECLARED `UNTESTABLE-IN-SUITE`. SUITE TOTAL = BASELINE ±0. NO TEST ADDED.**

**Suite: 561 → 561. Reconciled BY NAME (`SC-§104`): the set of test cases added is EMPTY and the set removed is
EMPTY**, so there is no name to reconcile on either side. I ran no suite (not my leg).

The spec's lead — *force the victory-screen class not to resolve* — **I measured it, and it does not reach the
observable**, which is the honest reason and the whole reason:

> The one thing this row changes is **which input mode is applied**, and that is written by
> `APlayerController::SetInputMode`, whose **entire body** is inside
> `if (GameViewportClient && LocalPlayer)` with `GameViewportClient = GetWorld()->GetGameViewport()` — so in a
> headless `-nullrhi` automation world, which has no game viewport client, `SetInputMode` is a **no-op for both the
> old and the new code**, and a test asserting the posture would read identical state before and after the fix.

Corroborating measurements, so this is not a guess: the suite's controller tests are CDO/static-pure probes
(`SiegePlacementTest.cpp` — *"All four are floats/vectors in, one value out: no `UWorld`, no `AActor`"*), and while
the house rule of "not one `UWorld::CreateWorld`" was broken once on purpose (`SiegeFogVisualTest.cpp`, TASK-1173),
a fabricated world still has no `UGameViewportClient`. The two members that *are* writable headlessly —
`bShowMouseCursor` / `bEnableClickEvents` — are set **true on both the old and the new degraded path**, so they
cannot discriminate either. ⇒ such a test **could not fail on unmodified `HEAD` and could not pass for the right
reason** — `SC-§39`'s test that cannot discriminate, and a green pin over it would spend the claim. I wrote none.

**A source-text probe was also considered and rejected**: a regex over `SiegePlayerController.cpp` asserting "the
UI-only `SetInputMode` sits behind a `VictoryWidget` guard" would red on `HEAD` (so it discriminates *today*), but it
can pass for the wrong reason (a present-but-inverted guard), and it would red spuriously on the next legitimate
touch of a function three rows have edited in two days. A brittle green is not worth a real guard's reputation.

---

## 9. (4)(c) THE 5b LEG — `UNOBSERVABLE` IS PRE-AUTHORISED; THERE IS NO 🧑 HAND CHECK TO WRITE

Restated here so the verify leg does not have to rediscover it (`VER-§8` cl. 2, declared at boarding):
**the degraded state only occurs when the victory screen fails to load, which is exactly what does not happen in a
healthy build.** PIE on `L_Arena` reaches a match end *with* a screen, so the branch this row adds is never entered
and the fix is not runtime-observable. ⇒ **record `verify: unobservable (declared at boarding — degraded path
unreachable in a healthy build)` and route to 5c. It is never read as a pass.**
⛔ **And there is deliberately no hand check: asking Jonathan to break his own install to watch a fallback is not an
ask.** The success path he already confirmed by hand is byte-identical (§6), so nothing he has verified is at risk.

---

## 10. WHAT QA SHOULD SCRUTINISE (named by me, not left to be found)

1. ⭐ **The `if (VictoryWidget)` guard around TASK-1314's focus block (`:2344`) is now provably always-true**, since
   my early return makes the null case unreachable. **I left it deliberately**: removing it would re-indent the
   focus block and break the byte-identity fence (§6), and a redundant-but-correct guard on a path that three rows
   have rewritten in two days is cheap insurance. Flag it as a NIT if you disagree — do not read it as an oversight.
2. **The duplicated `match ended — winner %s.` log** (§7) — deliberate, justified in-code, measured to have no
   consumer. It is the one piece of drift this shape buys.
3. **The posture choice itself**: `FInputModeGameAndUI` + cursor, *not* `FInputModeGameOnly`. Rationale in §4.1/4.2
   (the WARN's own words, plus the HUD stays clickable). If the house preference is game-only free-look on a dead
   match, that is a one-word change — but it would hide the cursor, and the WARN asked for the opposite.
4. **`bMatchEnded` is still latched on the degraded path** and every match-end refusal still fires. The fix restores
   *input delivery*; it does not re-open placement, the war map, the assistant console or the controls overlay.
5. **Not verified by me, out of my fence:** whether any widget still in the viewport at that moment offers a useful
   click. I only establish that clicks are *possible* again.

---

## 11. FENCES, STATUS AND THE TREE

- **Files I wrote: exactly two** — `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` and this handoff.
- ⛔ **I did NOT flip my own status.** My `names` block reads *"NEVER … `TASKBOARD.md`"*, so the `ready-for-qa` flip
  must be routed by the orchestrator/manager. **This row is complete and awaiting that flip.**
- **Untouched and NAMED, dirty at my instant, none of it mine (`SC-§102` · `SC-§106`):**
  `.claude/pipeline/TASKBOARD.md` (3/3) · `Tools/run_suite_bounded.ps1` (45/13 — **TASK-1335 is editing it in
  parallel**) · untracked `.claude/pipeline/handoffs/TASK-1333-buildmaster.md`. I staged nothing and ran no git
  write command of any kind.
- ⛔ **No `Content/`, no `.uasset`, no `Saved/`, no `CONVENTIONS.md`, no `.uproject`, no `settings.local.json`.**
  This row needed no asset keystroke (`SC-§125` does not apply — nothing to declare).
- ⛔ **I did not compile, did not run the suite, did not touch git, and did not launch an editor** (none was running
  and none is needed). Compile + suite + 5c are **TASK-1321**'s.
- **Line endings:** the file is LF on disk and stayed LF; git's `LF will be replaced by CRLF` notice is the repo's
  pre-existing checkout normalisation and appears for every dirty file in the tree, including ones I never opened.

**Engine reads that back this handoff** (UE 5.8, `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/`):
`Engine/Private/PlayerController.cpp:6372-6387` (`FInputModeUIOnly::ApplyInputMode`) ·
`:6398-6414` (`FInputModeGameAndUI::ApplyInputMode`) · `APlayerController::SetInputMode` (the
`GameViewportClient && LocalPlayer` guard) · `Engine/Private/GameViewportClient.cpp` (`InputKey`'s `IgnoreInput()`
early-out; `ViewportConsole` under `#if ALLOW_CONSOLE` at `:2807-2809`) · `Core/Public/Misc/Build.h:215,348`.
