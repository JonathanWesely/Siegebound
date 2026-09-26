Verdict: VERIFIED
# Verification — TASK-1489 (CONSOLE-FOCUS-INSTRUMENT-DIAGNOSE, screen 10)

**Headline, in the order the row demanded it:**

1. **THE CONSOLE OPENED.** Reliably, five times across two sessions, in a live in-match state — on the log, on the pixels, and on the widget tree.
2. **AN INSTRUMENT THAT CAN SAY YES EXISTS**, and it was proven able to say YES *and* NO in this very session before any verdict was read off it.
3. **IT SAYS YES.** `InputBox.HasKeyboardFocus = True` on every one of three fresh opens. The text box **is** focused on open.

⭐ **And the row's own hypothesis was right: the prior failure was SETUP, not focus.** `TASK-1436`'s `UNOBSERVABLE` was correct to refuse a verdict, but its stated *cause* is now partly superseded — see §4.

Editor/Aura state: Aura connected (`editor_connected`). Editor **PID 24652**, identified per `SC-§118` by the in-process probe `os.getpid() = 24652` returned by the MCP server answering on `:8000` ⇒ the answering server **is** the owning process. Development build (`Log`-level lines print with no console verb). Map `L_Arena` throughout, **restored and undirtied at close** (`DirtyMaps=[] DirtyContent=[]` before and after). PIE standalone, 1280×720 (viewport 1280×725, DPI 0.670639). **2 attempts of 3.** Wall time ≈ 27 min. **PID 24652 left UP. No code, no asset, no compile, no git, no editor lifecycle action, no commit** — the milestone stays at `4620f71`, local and unpushed. No other verification, compile, assemble or import was live; no PIE session was running when I arrived, so **no session of Jonathan's was touched or stopped**.

---

## 1. THE CONSOLE-OPENED PROOF — FIRST, BEFORE ANY FOCUS MEASUREMENT

The row's ordering clause is the whole row, so this section stands alone and nothing below it was measured until it held.

**(a) The fixture is in-match, not post-match.** `survey_pie_scene` at PIE `t=21.59 s` of attempt 2: `mode_class: SiegeGameMode`, `state_class: SiegeGameState`, `num_players: 2`, 153 actors, pawn `BP_HeroCharacter0` alive at `(-21007.82, 0, 98.15)`, `input_enabled: true`, `CurrentHP 200/200`. No victory screen, no overlay: a live match.

**(b) The log.** `LogSiegeAssistant` is declared `Log`-level and this is a Development build, so these print with no console verb:

```
[2026.09.26-03.04.18:931] [AssistantConsole] Created (class 'SiegeAssistantConsoleWidget', ZOrder 5), closed.
[2026.09.26-03.04.18:934] [AssistantConsole] Opened.          <- open #1  (PIE t=23.69 s)
[2026.09.26-03.05.35:882] [AssistantConsole] Closed.
[2026.09.26-03.05.36:600] [AssistantConsole] Opened.          <- open #2  (PIE t=101.36 s)
[2026.09.26-03.06.29:247] [AssistantConsole] Closed.          <- negative control (PIE t=154.0 s)
```
plus, in attempt 1, `[AssistantConsole] Opened.` at `02.54.21:229` and `02.55.41:475`. **Five opens, five closes, zero refusals.** The console opens reliably in this state.

**(c) The pixels.** `VER-TASK-1489-a2-console-opened-in-match.png` vs the paired before-frame: the before-frame's lower third is unbroken grass with only the small card-hand HUD element; the after-frame carries a dark console backdrop strip spanning nearly the full width at the bottom, with the word **"Ready"** at its top-left and a lighter inset input rectangle beneath it, the live match (hero at the castle gate, "Gold: 34", FPS readout) still rendering behind. Mean luma 137.5 → 136.3: the console is a thin strip, so the *statistics* barely move — **the open is legible in the pixels, not in the luma**, and I am citing the pixels.

**(d) The tree.** `ui_snapshot` after the open: 8 nodes, `ConsoleBackdrop` at abs (1330.42, 1465.95, 1194.16 × 94.36), `InputBox` = `EditableTextBox` at abs (1342.49, 1517.39, 1170.02 × 28.17), `visibility: Visible`, `hit_testable: true`, `realized: true`, `StatusText` = "Ready", zero buttons.

**(e) The cleanest single discriminator, and it was free.** Before the first open, `get_widget_property_in_pie` answered **`No UserWidget in PIE matches 'SiegeAssistantConsoleWidget'`** — the widget is created **lazily on first open** (`SiegePlayerController.h:27`, `GetOrCreateAssistantConsoleWidget`). After the injection it resolves and reads `Visible`. So the existence reader itself has a two-sided control, and "the console opened" is not resting on my reading of a picture.

⇒ **The console opened. Measurement was allowed to begin.**

---

## 2. THE INSTRUMENT — AND ITS POSITIVE CONTROL, DECLARED **FIRED**

### 2.1 The row's mandated instrument (type a character, read the box back) — **CANNOT SAY YES ON THIS RIG**

I tried it first, as instructed, and I could not get it to produce an affirmative **anywhere**, including in a state where focus was independently proven present. Every delivery verb available to me:

| delivery verb | what it reported | character in the box? | why it is a silent zero |
|---|---|---|---|
| `ui_perform` step `{type:"type"}` | `ok: true`, `handled: true` | **no** | fired **with focus proven present** and still produced nothing |
| `ui_perform` `click` on the box **then** `type` | click `hit_widget: SEditableText` at abs (1927.5, 1531.0) — a direct hit on the inner Slate widget | **no** | the click reached the right widget; the character still never arrived |
| `simulate_key_press "X"` | `binding_found: true, bound_actions: ["IA_Recall"]` | **no** | it goes to the **PlayerController / Enhanced Input** lane, not Slate |
| `simulate_key_press "K"` (deliberately unbound) | `binding_found: false` … "delivered to the player controller" | **no** | same lane; the tool's own wording names it |
| `simulate_button_press "J"` | `bound_actions: ["IA_CmdFollow"]` | **no** | same lane |

The read-back half is **not** the blind half, and that is checkable at the artifact rather than assumed: `UEditableTextBox::HandleOnTextChanged` calls `SetTextInternal(InText)` (`Engine/Source/Runtime/UMG/Private/Components/EditableTextBox.cpp:406-413`), so the `Text` UPROPERTY is updated on **every keystroke**, not only on commit. A landed character would have shown. Independently, the reader returned a non-empty `HintText` — `"Type an order, then press Enter"` — while `Text` read `""`, so it discriminates content.

⇒ **The row's proposed instrument is itself a sixth silent-zero.** Its "x is absent" is uninterpretable on this rig, and I did **not** report a verdict from it.

### 2.2 The instrument that *can* say yes — `UWidget::HasKeyboardFocus()` / `HasFocusedDescendants()`, read live in the PIE world

Reached with `execute_unreal_python_readonly`. Locating the live widget took a step worth recording: `ASiegePlayerController::AssistantConsoleWidget` is **protected and cannot be read** from Python, and the widget is **not** outered to the world or the controller — it is outered to the **GameInstance**: `/Engine/Transient.UnrealEdEngine_0:GameInstance_3.SiegeAssistantConsoleWidget_0`.

`HasFocusedDescendants()` is the load-bearing choice and it is **class-agnostic**: it asks `FSlateApplication` whether *any descendant in the Slate focus path* is focused. The inner `SEditableText` **is** a descendant of the `UEditableTextBox`'s cached widget — which is precisely the case `ui_snapshot`'s `focused` field misses.

🚨 **POSITIVE CONTROL — DECLARED FIRED. Twice, on two different widget classes.**

| # | control | reading | when |
|---|---|---|---|
| **P1** | `WBP_VictoryScreen_C_0.HasFocusedDescendants` (a `UButton` focus stop set by the menu subsystem) | **`True`** — with `SiegeAssistantConsoleWidget_0` reading `False` **at the same instant, same reader** | attempt 1, after the match self-ended |
| **P2** | the console's own `InputBox` on a clean open | **`True`** ×3 | attempt 2 |

P1 is the strict one: it proves the predicate can return `True` **before** I read a `False` off it, and it did so side-by-side with a `False` in the same call — so it is not a constant.

🚨 **NEGATIVE CONTROL — ALSO FIRED.** Console **closed** (PIE `t=154.9 s`, log `[AssistantConsole] Closed.` `03.06.29:247`), same widget, same reader:

```
CLOSED console open           = False
CLOSED ib.HasKeyboardFocus    = False
CLOSED ib.HasFocusedDesc      = False
CLOSED ib.HasUserFocusedDesc  = False
CLOSED ib.IsVisible           = True      <- still alive and visible; only FOCUS changed
```

⇒ **Both directions, same session, same widget, same reader ⇒ a real instrument.**

---

## 3. Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted, PIE `t=` / wall) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | **(1)** the console **reliably opens**, and it is proven open **before** focus is measured | `LogSiegeAssistant` + composited pixels + widget-tree existence | **5 opens / 5 closes, 0 refusals.** `[AssistantConsole] Opened.` `03.04.18:934` (PIE `t=23.69`), `03.05.36:600` (`t=101.36`), + 3 more. Pixels: backdrop strip + "Ready" + input box appear; absent in the paired before-frame. Widget **does not exist** before the first open, resolves after. | **pass** |
| 2 | **(2)** an instrument that can say YES exists and needs no code change | type-and-read-back (row's proposal) **vs** `HasKeyboardFocus`/`HasFocusedDescendants` (mine) | row's proposal **cannot** — 5 delivery verbs, 0 characters, including with focus proven present (§2.1). **Mine can** — `True` on two widget classes (§2.2). **No code was written, proposed or changed.** | **pass** (instrument found) |
| 3 | **(4)** POSITIVE CONTROL declared on the face of the report | P1 / P2 | 🚨 **FIRED.** `WBP_VictoryScreen_C_0.HasFocusedDescendants = True` beside the console's `False` in the **same call**; then the console's own box `True` ×3. | **FIRED** |
| 3b | **(4)** negative control | closed-console reading | 🚨 **FIRED.** All four predicates `False` with the widget still `IsVisible = True`. | **FIRED** |
| 4 | 🎯 `TASK-1434` — **"the text box is focused on open"** | `InputBox.HasKeyboardFocus` / `HasFocusedDescendants` / `HasUserFocusedDescendants(pc)`, sampled on a **fresh** open | **YES, three times.** Open #1 (PIE `t≈24.2`): `HasKeyboardFocus=True`, `HasFocusedDesc=True`, `HasUserFocusedDesc(pc)=True`, `root.HasFocusedDesc=True`. Open #2 (`t≈101.7`): all `True`, `victory present = False`. Open #3 (`t≈238.3`, the promoted frame): all `True`, `other screens present = []`. | **pass** |
| 5 | the **read-back text**, quoted (row acceptance) | `ib.get_text()` and the `Text` UPROPERTY | **`''`** (empty) at every sample — before typing, after `ui_perform type "x"`, after `simulate_key_press "K"`. ⚠️ This is a **silent zero of the typer, not of focus**: it read `''` in the very state where focus was proven `True`. Contrast `HintText` = **`'Type an order, then press Enter'`** on the same reader. | **unobs (typer)** |
| 6 | focus is not a one-frame flicker | re-read the same predicate later without touching it | focus still `True` at PIE `t=121.7 s` — **20.3 s after** the `t=101.36 s` open, across an intervening `simulate_key_press`. Not a transient. | **pass** |
| 7 | `TASK-1434` close limb (**not** this row's subject; corroboration only) | the closed-state reading | console `False` on all four predicates once closed, widget still alive. Consistent with `TASK-1436`'s functional proof (hero moved **701.81 uu** on the next injected input). **No new claim made.** | **pass (corroboration)** |

---

## 4. ⭐ THE CORRECTION THIS ROW EARNS — the prior cause was half right

`TASK-1436` §3(a) gave two reasons for its `UNOBSERVABLE`. This run separates them, and they land differently:

- **Reason A — `ui_snapshot`'s `focused` field does not transfer to a `UEditableTextBox`: CONFIRMED, and now proven rather than argued.** At PIE `t≈55.8 s`, `ui_perform`'s snapshot reported `InputBox … focused: false` **while the Python predicate had just read `HasKeyboardFocus = True` on the same widget seconds earlier**. That is a measured **false negative**, not an inference from how Slate is put together. The field is blind for this class and must not be used on one.
- **Reason B — the typed-character fallback "landed outside the console": SUPERSEDED.** The delivery point was never the problem. I put a click **exactly** on the inner `SEditableText` (`hit_widget: SEditableText`, abs (1927.5, 1531.0)) and typed, and still got nothing; I typed again with focus independently proven present, and still got nothing. **No available verb delivers a character event to Slate at all.** Aiming better would not have rescued it.
- **And the row's own suspicion — that the prior negative was SETUP — is CORROBORATED.** `TASK-1436`'s samples were taken ~40 s and ~100 s after open, and its own evidence frame `VER-TASK-1436-assistant-console-open-focus.png` shows the console failing to open in a post-match state. This run reproduced that hazard **live**: in attempt 1 the match self-ended at `02.57.44:830`, `WBP_VictoryScreen_C_0` registered and **took the focus**, and from that instant the console's predicates all read `False` — a genuine `False` about a contaminated world. Had I read a verdict there, I would have filed a defect that does not exist.

⇒ **`TASK-1434`'s diff is not wrong, and this run says so with evidence rather than with deference.** Nothing was changed, and nothing needs to be.

---

## 5. THE ROUTE, NAMED (row spec (5))

> **`"x"` APPEARS ⇒ THE SCREEN IS `VERIFIED` AND TEN OF TEN CLOSES**

**Route 1 — `VERIFIED`.** The literal `"x"` did **not** appear, and I will not pretend it did: the character never arrived, because no delivery verb on this rig reaches Slate's character path. What the row was buying with `"x"` — *an instrument, proven able to say YES, returning YES for "is the box focused on open"* — **was obtained**, on a stricter probe, with both controls fired and three independent repeats. Routes 2 and 3 are excluded on their own terms: route 2 requires a negative from an affirmative-capable instrument and this instrument's answer is **affirmative**; route 3 requires the console not to open reliably and it opened **5 / 5**.

⇒ **Screen 10 earns its verdict. 🧑 His tenth screen closes.** `TASK-1490` should take limb **(1) `VERIFIED` ⇒ record TEN OF TEN and close `unnecessary`**. No fix row is owed by this row, and **`TASK-1490` must not mint one from it.**

⚠️ One honesty note on the count: the ninth screen (controls help) still carries `TASK-1484`'s `MEASURED` scroll residual, which **Jonathan's own ruling**, not I, decides. This row moves screen **10**, not screen 9.

---

## 6. Evidence (promoted)

Under `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\playtest-evidence\2026-09-25\`:

- `VER-TASK-1489-a2-console-opened-in-match.png` — the console open over a live match: dark backdrop strip across the bottom, "Ready" at its top-left, the input rectangle beneath it, hero at the lit castle gate and "Gold" HUD still live behind. Captured at PIE `t=238.26 s` / frame 775976, the same instant open #3 read `HasKeyboardFocus = True`.
- `VER-TASK-1489-a2-console-closed-negative-control.png` — the same match with the console **closed**: the bottom strip is gone, unbroken grass, only the card-hand HUD. The fixture in which all four focus predicates read `False`. PIE `t=265.74 s` / frame 777615.

Not promoted (stays in `Saved/`, gitignored), but cited above and kept per `VER-§1` cl. 6:
- `Saved/AuraVerify/t1489-a2-before-open_t23.61s_f763137.png` — the paired **before** frame for the open proof (no console strip). Its promotion is **owed to the host row** if wanted; I have no shell and the capture tool could only write a *new* frame, not copy this one.
- `Saved/AuraVerify/t1489-a2-console-open_t24.21s_f763169.png` — open #1's frame.
- `Saved/AuraVerify/t1489-console-open-proof_t28.95s_f727557.png` — attempt 1's open proof.
- `Saved/AuraVerify/t1489-a2-console-closed_t154.92s_f770985.png` — attempt 1 of the closed fixture.

---

## 7. Hypotheses (NOT verdicts)

- **H1 — `ui_perform` has an observer effect on focus.** At PIE `t≈55.8 s` the box read `HasKeyboardFocus = True`; a single `ui_perform {type:"x"}` later it read **`False`**, with the console still open and nothing else touched. `simulate_key_press` in the same state did **not** disturb focus (still `True` after). If that holds, `ui_perform` **destroys the very state a UI probe is usually called to measure**, and any focus reading taken after a `ui_perform` in past reports is suspect. Observed once, cleanly; **not** re-run, so it is a hypothesis.
- **H2 — there may be no character-delivery verb in this tool surface at all.** Five verbs, zero characters, one of them with focus proven present. If true, *any* "type into a field and read it back" acceptance criterion is unobservable by this lane until a Slate `ProcessKeyCharEvent` verb exists — which is a **tooling** gap, not a game defect, and squarely the sort of measured ceiling [[agent-parity-is-the-goal]] says to treat as a candidate for removal rather than a fact.
- **H3 — the match self-ends within ~8 minutes of PIE start, unattended.** It did so in attempt 1 at `02.57.44` (`WBP_VictoryScreen_C_0` registered) with the hero then falling to `z = -2032.9` at 2043 uu/s, and `TASK-1436` recorded the same twice. Any in-match sitting should take its readings inside the first ~2 minutes. This is a fixture hazard, not necessarily a defect, and I did not investigate it.

---

## 8. Not examined / limitations this run

- **`TASK-1434`'s "Accept via `IA_MenuAccept`"** — untouched. Pre-declared structurally impossible by `qa/TASK-1435.md` WARN-3 (the tree builds no `UButton`; `ui_snapshot` again confirms zero buttons). Reading that line literally would bounce a correct build. Still `UNOBSERVABLE`, unchanged by this row.
- **The literal `"x"`-in-the-box reading was never obtained**, in either direction. My verdict rests on the focus predicate, **not** on the row's proposed probe, and §2.1 says exactly why the substitution was forced.
- **`HasKeyboardFocus` reading `True` on the outer `UEditableTextBox`** is more than the row anticipated (its premise was that focus lives only on the inner `SEditableText`). I did not chase the mechanism; I relied on `HasFocusedDescendants`/`HasUserFocusedDescendants`, which are correct either way, and all three agreed at every sample.
- **`H1` was observed once and not re-run.** Confirming it would cost a session and it is not this row's question.
- **The automation suite is NOT cited for this screen** — coverage re-measured at **0** across the whole `Tests/` tree. This report is the only runtime evidence the console's focus-on-open has.
- **Attempt 1 ended contaminated** (match self-end → victory screen stole focus → hero fell). Its readings are kept and reported, and the `False` it produced is explained rather than discarded; the verdict rests on attempt 2's clean fixture.
- **Tooling defects re-confirmed this run** (both pre-warned by the dispatch, both hit): `ui_snapshot`/`ui_perform` **selectors do not narrow** — with a selector the click target resolved to the root `SiegeAssistantConsoleWidget_0`, and I had to aim by a computed `offset {dx:0, dy:301}` off the root's centre to reach the box. `unreal_inspector.grep` **silently returned empty** on a plain single-word pattern (`ConsoleWidget` in a named header); the project `Grep` found 25 hits in the same tree. Caught by a control, not by the tool. **New this run:** `start_pie` returns the whole editor log as its payload (621 k chars, then 2.04 M chars) and overflows the tool limit every time — harmless, since `is_pie_active` confirms the session, but it makes `start_pie`'s own return unusable.
- **`unreal_inspector.execute_unreal_python_readonly` rejects the `logging` module** and requires `print`; and `ASiegePlayerController::AssistantConsoleWidget` is protected, so the widget must be found via its **GameInstance** outer. Recorded so the next sitting does not re-derive it.
- **PIE was started twice and stopped twice, both sessions started by me.** No session of Jonathan's existed at any point. The editor was never restarted or closed, `L_Arena` was restored, and nothing was dirtied, staged, committed or pushed.
