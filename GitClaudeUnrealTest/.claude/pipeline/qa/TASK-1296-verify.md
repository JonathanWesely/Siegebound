Verdict: VERIFIED
verdict-clause: on a real PIE boot of `L_MainMenu` on the 23:41:41 binaries the engine Error `InputMode:UIOnly - Attempting to focus Non-Focusable widget` occurred **0 times**, while the same pattern fires in 33 older logs; and the `TASK-1274` keyboard route still opens the deck builder.
subject: TASK-1296
leg: `TASK-1298` clause (5) — the FIRST 5b verify leg · ⛔ BINDING (`VER-§6` cl. 5)
verifier: playtest-verifier · 2026-09-19 · attempt 1 of 3
⛔ line 1 is deliberately BARE. `TASK-1298` cl. (5) orders *"Verdict line bare (`VER-§6` cl. 5 (iv))"*, so the
one-clause summary the dispatch asked for lives on line 2 as `verdict-clause:` rather than as a suffix that
would break a `head -1` equality test. Flagged, not silently chosen.

# Verification — TASK-1296

**Editor/Aura state:** connected **yes** · editor **PID 14432**, GUI editor, `unreal.is_editor() == True`,
`-nullrhi` absent from its log, ⛔ **no `-game` instance anywhere** · map **`/Game/Maps/L_MainMenu`** (loaded by
`load_level` from `L_Arena`; `discarded_unsaved: false`) · PIE **standalone, 1 client, 1280×720** · **Aura
loaded and serving** — every measurement below was taken *through* Aura's own read-only Python lane, which is
itself the proof the `VER-§7` lane survived `TASK-1294`'s suite-lane exclusion · **attempts used 1 of 3** ·
wall time ≈ 01:29→01:34 local (2026-09-19).

⚠️ **`SC-§118` classification, declared at its real strength.** I could **not** read the boot command-line
string: `unreal.SystemLibrary.get_command_line()` returned **empty**, and the log's first 400 lines contain
**0** lines matching `ommand line`/`ommandLine` (`CMD_HITS=0`). So the `-game` check is **not** a
command-line-token measurement and I will not dress it as one. What I *did* measure, and what the
classification actually rests on: `os.getpid()` **inside the editor's own interpreter** = **14432** (the PID
the dispatch names); that process's log opens with **`Log file open, 09/18/26 23:47:02`**, matching
build-master's 23:47:01 relaunch record to the second; `unreal.is_editor()` **True**; and a live-log census
found **exactly one** editor log being written (`GitClaudeUnrealTest.log`, plus `cef3.log`, which is this same
process's embedded browser) — ⇒ **no second editor, headless or otherwise, existed to confuse the read.**

**Binaries:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` mtime **2026-09-18 23:41:40**, size
9,809,408 — and the session opened its log at **23:47:02**, i.e. *after* the 23:41:41 build finished, so this
process cannot be running anything older. ⛔ I did **not** compile, relaunch, close the editor, or run Git.

⚠️ **Row-status note (declared, not waived).** `VER-§` wants the row at `built` before I test it. On disk the
row reads **`qa-passed`**, because `handoffs/TASK-1298-buildmaster.md` §4 records the `built` flip as
**deliberately deferred** to the commit leg under the orchestrator's one-writer fence on `TASKBOARD.md`. The
rule exists to stop a verifier testing binaries that do not exist; here the binaries provably exist and are
loaded (above). I proceeded and am saying so rather than reporting a blocker over a bookkeeping fence.

---

## THE INSTRUMENT, AND ITS FIRING BASELINE — established BEFORE PIE, because otherwise a zero proves nothing

`SC-§39`, and the build-master's own `NOT FOUND`-that-was-a-regex incident, make this the first section rather
than a footnote.

| control | result |
|---|---|
| the pin, **pre-PIE**, whole active log | `Attempting to focus Non-Focusable widget` = **0** |
| ⭐ **POSITIVE CONTROL — the identical pattern against every other log in `Saved/Logs/`** | **fires in 33 log files** (1–3 hits each), e.g. `…-backup-2026.09.18-05.22.36.log` = 2, `run_suite_bounded_suite_20260918-161724.log` = 1, `M8e_solo.log` = 2 |
| ⛔ **deliberately-WRONG control** (`Attempting to focus Non-Focusable **gizmo**`) | **0** in every one of those 33 files and in this session's log |
| the log file actually read | proven to be **this process's own**: header `Log file open, 09/18/26 23:47:02`, and its tail carries my own Python invocations at 08.29–08.32 UTC |

⇒ **the pattern discriminates**: it matches the real data when the data is there, and the near-miss variant
never matches. ⛔ **The zero reported below is a measured zero, not an empty grep.**

⚠️ **One method note worth the line.** My first instinct — emit a unique marker via `unreal.log` and find which
file contains it — returned **no file**, and that is *not* a flush delay: the Aura read-only lane runs
`log LogPython off` / `log LogPython Log` around each call (visible in the log tail), so its own output never
reaches the file. I switched to the session-header + tail-content proof above instead of trusting the marker's
silence. An absent marker would have been the same trap in a different coat.

**Log cursor (so every count below belongs to THIS PIE instance, `VER-§1` cl. 2):** byte **373,193** / line
**2,745**, taken at 01:32:04 local, immediately before `start_pie`. Post-run the file was 393,883 bytes ⇒ a
**20,690-byte / 133-line slice** written by this PIE run, and the slice contains `LogPlayLevel` ×7,
`Play in editor` ×1 and `LogWorld: Bringing World` ×1 — **the slice covers a real play session**, which is the
other half of "an absence means something".

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| (3) first half | *"from a fresh `L_MainMenu` PIE the engine Error string `InputMode:UIOnly - Attempting to focus Non-Focusable widget` appears **0 times** in THAT instance's log"* | count of the string in the post-cursor log slice, with the firing baseline above | **0** — both the full form `InputMode:UIOnly - Attempting to focus Non-Focusable widget` and the short form `Attempting to focus Non-Focusable widget` count **0** in the 133-line slice **and 0 in the whole 393,883-byte session log**. A wider sweep for *any* line containing `Non-Focusable` **or** `InputMode` returned **`NF_LINE_COUNT=0`** | **pass** |
| (3) second half | *"the `TASK-1274` recipe (`IA_MenuDown` ×2 → `IA_MenuAccept`) still opens the deck builder (`ui_snapshot` shows a `DeckBar` with 10 children)"* | `inject_input_action` ×3 then `ui_snapshot` | injected `/Game/Input/Actions/IA_MenuDown.IA_MenuDown` at PIE t=**25.760** and t=**26.177**, `/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept` at t=**26.594** (all `status: action_injected`). At t=**29.103** `ui_snapshot` root = **`WBP_DeckBuilder_C_0`**, containing `Overlay_19/DeckBar` (`HorizontalBox`) with **exactly 10** children `DeckSlotEntryWidget_0 … DeckSlotEntryWidget_9`, all `realized: true` — evidence `…/VER-TASK-1307-deckbuilder-opened-keyboard-route.png` | **pass** |
| (4) / cl. (5) third line | *"the cold-state focused button's text = 'Play (vs Bot)' if readable, else `unobs`"* — `TASK-1297` WARN left this measurement owed to this leg | `ui_snapshot` of `WBP_MainMenu` taken **before any input** (PIE t=25.682) | **READABLE, and it matches.** `Overlay_19/VerticalBox_0` holds **7** buttons `Button_0…Button_6`; **`Button_0` is the only node with `"focused": true`**, and its child `TextBlock_0` reads **`"Play (vs Bot)"`**. The remaining six read `Sandbox (No Bot)` · `Deck Builder` · `Multiplayer` · `Settings` · `Login` · `Quit`, in slot order top-to-bottom. ⇒ **index 0 of 7, unchanged from `TASK-1274`** | **pass** |

⭐ **The coupling half that makes the zero mean something.** `TASK-1296` deleted the `AddExpectedError` pin in
the same diff, so a still-emitting Error would now red
`Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder`. That test is green ×3 in build-master's runs —
**but `SC-§125` cl. 4 says a suite run is evidence about the class the *process* compiled, not the class on
disk**, which is precisely why this leg exists. This PIE ran in the **GUI editor on the committed asset**
(post-A2-1 `.uasset`, sha `6c143526…e614fa` per the host), and it produced **0**. That is the read the suite
could not give.

---

## Evidence (promoted)

Both files were written **directly** to the promotion path by `capture_pie_frame` (no `Saved/` copy is owed),
and both were read back from that path afterwards — they exist:

- `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1296-mainmenu-cold-boot-focus-state.png` — PIE t=**25.699**, frame 377990, 900×510, mean_luma 195, pct_near_black 0. The `L_MainMenu` backdrop (blue sky, cloud bank, white ground plane) with the menu column centred: seven legible button labels stacked in order, `Play (vs Bot)` at the top, `Quit` at the bottom. No error banner, no black frame, menu fully rendered at cold boot. ⚠️ **The focus determination is the `ui_snapshot` read, not these pixels** — at 900 px wide the button chrome is too small for me to honestly call a focus outline, and I am not going to pretend otherwise.
- `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1307-deckbuilder-opened-keyboard-route.png` — PIE t=**29.119**, frame 378189, 900×510, mean_luma 152, pct_near_black 0.0002. The deck builder occupying the screen after the keyboard route: a top bar of **ten** slot tabs labelled `deck1 … deck10` with `deck1` highlighted, the `Deck Builder` title at top-left, a full horizontal card grid of illustrated card tiles with cost pips, a dark right-hand detail panel reading *"Click a card to see how it works"* with a `Close` button, `Reset to Default` bottom-left, `Exit` bottom-right and a `Play` bar along the bottom. ⛔ **This one PNG is cited by BOTH verify reports** (it is `TASK-1296`'s regression half *and* `TASK-1309`'s open-under-test) — the host should stage it **once**.

---

## Hypotheses (not verdicts)

- The Error's disappearance is *consistent with* route **B′** (the `In Widget to Focus` wire deleted, so
  `SetInputMode_UIOnlyEx` is handed no widget and `FInputModeUIOnly::SetWidgetToFocus`'s complaint at
  `PlayerController.cpp:6345` is never reached). ⛔ **I did not open the Blueprint and did not verify the
  mechanism** — I observed only that the string is absent on a boot that previously produced it.
- Focus still landing on `Button_0` with the wire gone is consistent with `TASK-1274`'s own `FocusButton` path
  (`SiegeMenuInputSubsystem.cpp`) doing the focusing rather than the GameMode. Again: **observed state, not
  measured mechanism.**

## Not examined / limitations this run

- ⛔ **One PIE boot, not a sample.** The Error was historically present on *every* `L_MainMenu` boot (33 logs),
  so one clean boot is a strong signal — but a single run cannot speak to an intermittent emitter.
- ⛔ **The second emitter is untouched and out of scope.** `TASK-1297` measured `WBP_VictoryScreen`
  `bIsFocusable = False` ⇒ the identical engine Error still fires at match end on `L_Arena`
  (`SiegePlayerController.cpp:2264-2270`). It is boarded as `TASK-1311`. **My zero is scoped to `L_MainMenu`;
  it is not a project-wide claim about this Error string.**
- ⛔ **The boot command line was unreadable** (above) — classification rests on PID/log-header/`is_editor`.
- ⛔ **No key was pressed by a human ~~and none could be~~:** `FInputModeUIOnly::ApplyInputMode` calls
  `SetIgnoreInput(true)`, so ~~🧑 Jonathan **cannot** drive Down/Enter on this map~~. Everything here went through
  `inject_input_action`, which bypasses the viewport. His pre-written hand check (the focus outline on
  `Play (vs Bot)`) remains **his**, and `TASK-1297` §G's NIT stands: a "yes" from him would confirm the row's
  *risk* (no focus-brush regression), **not** its *fix* (an engine Error no human can see).
  - 🚨⛔ **CORRECTION 2026-09-22 (`TASK-1385`, law `VER-§8` cl. 11) — STRUCK, NOT DELETED. This was a FLAT
    REFUTATION, not a scope slip.** Source: 🧑 Jonathan, 2026-09-21, verbatim — *"ok, I opened a match, I saw
    the outline on the top menu option, hit the down arrow twice, and hit enter, and I was able to open the deck
    builder, so that menu navigation seems to be working fine."* **He did it.** *"and none could be"* and
    *"🧑 Jonathan **cannot** drive Down/Enter on this map"* were claims about **EVERY** layer of this engine's
    input stack, written from **ONE** layer's measurement; both are refuted and struck above.
  - ✅⛔ **WHAT STANDS, UNRELAXED — THE PREMISE.** `FInputModeUIOnly::ApplyInputMode` → `SetIgnoreInput(true)`
    is **NOT** relaxed: a real key press on `L_MainMenu` still does not reach **Enhanced Input**, and nothing he
    did either measures or refutes that. Only the human conclusion drawn from it was wrong.
  - ✅⭐ **AND THE TRUE HALF THAT FOLLOWS IT IS KEPT, AND IS THE MODEL:** *"Everything here went through
    `inject_input_action`, which bypasses the viewport"* — that sentence is **exactly right** and is **the shape
    the struck clause should have had**. It names the lane **THIS RIG** used and claims nothing whatever about
    his hands. *"I could not reach it"* is always writable; *"he cannot reach it"* almost never is
    (`VER-§8` cl. 11's duty).
  - ⛔ **WHAT I AM ENTITLED TO WRITE, AND NOTHING BROADER (`VER-§8` cl. 11):** *a real key press on `L_MainMenu`
    **does** reach the menu; **which layer carried it is UNMEASURED**.* 🚨 **MECHANISM: UNMEASURED.** Two routes
    predict his identical observable and his sentence discriminates **NEITHER** — **(i)** the focused `SButton` →
    Slate's navigation config → `SButton::OnKeyDown`'s Accept path (the route `handoffs/TASK-1274-programmer.md:74`
    predicted **in advance** and labelled unmeasured), and **(ii)** `USiegeMenuInputSubsystem`'s `IA_Menu*`
    handlers — **which would mean the premise above is wrong.** ⛔ *"It reached Slate"* is **NOT** written here as
    a finding; route (i) is the better-supported **HYPOTHESIS** and stays one (`SC-§101`).
  - ⛔ **SCOPE OF THIS CORRECTION:** line 1's verdict is **byte-untouched**, and every observation, control,
    hypothesis and other limitation in this report is **unaltered** — only the two struck clauses above changed.
- ⛔ **Deck untouched** — see the net-zero block in `qa/TASK-1309-verify.md`; all five `.sav` files are
  byte-identical across this run, mtimes not even touched.
