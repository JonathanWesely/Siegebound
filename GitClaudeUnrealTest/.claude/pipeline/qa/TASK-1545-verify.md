Verdict: MEASURED
# Verification — TASK-1545

> **Line 1 changed a second time, by AMENDMENT 2 2026-09-29 (the last section of this file; `TASK-1591`, under `TASK-1544`'s ruling, marker `TASK-1545-FOCUS-CLICK-ADMISSIBLE-2026-09-29`).** It went `MEASURED` (2026-09-28) → `UNOBSERVABLE` (first 2026-09-29 amendment) → `MEASURED` (amendment 2). No new run was made, and the observations are the same. The earlier notice follows, kept as written (`SC-§120`).
>
> **Line 1 changed by the 2026-09-29 amendment (section at the end of this file).** It was `Verdict: MEASURED` from 2026-09-28 until his answer to the mouse question landed. The body below is kept as first written (`SC-§120`), and every observation in it still stands. Only the letters' standing changed: his answer is void under spec (2)'s parenthesis as written. Read the amendment before relying on the Headline or the Outcome section.

Editor/Aura state: connected y · editor PID 12112, identified by command line (`"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`, created 2026-09-28 16:58:12, no `-game`; the only other Unreal* process was UnrealTraceServer 2768 `--sponsor 12112`, before and after; in-process `os.getpid()` = 12112) · map `/Game/Maps/L_MainMenu`, which I loaded from `L_Arena` (`discarded_unsaved: false`) · PIE standalone, 1 client, 1280×725, started by me at about 00:05:36 UTC. Jonathan ended that session himself at 03:16:47 UTC (see below), so there was nothing for me to stop · attempts used 1 of 3 · wall time about 3 h 15 min, nearly all of it waiting for his limb · Aura credit not surfaced · PIE announcement, per the dispatch: "The orchestrator told Jonathan just now that PIE will be driven on PID 12112 and to keep his hands off" · model (self-reported): "claude-opus-5-5[1m]"

## Headline
- **Down:** in one PIE process, in one log, my injected `IA_MenuDown` printed `HandleMenuDown() entered` at 00:05:50.721 UTC. His real `Down` printed **no** `LogSiegeMenuInput` line at all. His account says the highlight moved to "Sandbox(No Bot)".
- **Enter:** his real `Enter` printed **no** `IA_MenuAccept -> OnClicked.Broadcast()` line, but the sandbox match did open: `StartSandboxMatch` at 03:16:35.793 UTC.
- **Outcome:** (b) for each key, so (b)+(b) for the pair, with no split. Route (i) for both keys, measured against a named control, **on the condition** stated under Limitations 1: his account does not say that he left the mouse alone.

## Acceptance lines → observations
| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs/measured |
|---|---|---|---|---|
| 1 | line 1 `Verdict:` byte-literal | this file's first line | `Verdict: MEASURED` | pass |
| 2 | (0) counts with their control | byte counts of the class-name strings, ASCII and UTF-16LE | `IMC_Hero` (sha256 `168dbf4f1523f2e65c58157d2cc39d65bfe2f81466aaf3c332ecb01ffacda313`, 15013 B): InputModifier 2 / 0 (**control fired**), InputTrigger 0 / 0. `IMC_MainMenu` (sha256 `1a1ee5aff7be1b3485510ab88c2097de6120e34f7d182289132b60eb1db8dbaa`, 7379 B): InputModifier 0 / 0, **InputTrigger 0 / 0**. This agrees with `trg=0` by a second instrument, and nothing named ⇒ no stop | pass |
| 3 | build configuration declared by name before any absence | `get_build_configuration()` + exe name | `Development`; exe `UnrealEditor.exe` without a suffix ⇒ **Development (Editor), PIE in-editor**. `Log`-verbosity `UE_LOG` is compiled in | pass |
| 4 | control arm printed first, with its PIE stamp | `TASK-1394` entry line after an injected `IA_MenuDown` | L2787 `[2026.09.29-00.05.50:721][808]` `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` + L2788 `MoveFocus(+1): focus moved 0 -> 1 of 7 ('Button_1').`; injected at PIE t=13.935. Arming line L2743 `[00.05.36:774]` `IMC_MainMenu applied … IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).` | pass |
| 5 | (1b) focus read on the top entry | standalone `ui_snapshot` (no `ui_perform` anywhere this session) | after an injected `IA_MenuUp` (L2789 `[00.05.51:255]` `MoveFocus(-1): focus moved 1 -> 0 of 7 ('Button_0')`), `Button_0` "Play (vs Bot)" `focused: true` at t=14.985. Control: `Button_1` `focused: false` at t=15.001. At his instant the subsystem had also re-placed focus on the top entry itself: L2887 `[03.16.21:306]` `ApplyInitialFocus: focus placed on the TOP option 'Button_0' ("Play (vs Bot)")` | pass |
| 6 | his answer, verbatim | his words, relayed by the orchestrator as an account | see *His answer* below | pass (recorded) |
| 7 | both keys' lines side by side, same-log statement, log cited by filename + UTC stamps | `LogSiegeMenuInput` lines after L2789, bounded by the travel line | see *Side by side*. Neither key has an instrument line; the travel line stamps his Enter | measured |
| 8 | outcome by letter | `TASK-1395` (3), per key, then the pair | Down **(b)** · Enter **(b)** (the Accept analogue) · pair **(b)+(b)**, no split | measured |
| 9 | `## Not examined / limitations` | this file | present | pass |
| 10 | `VER-§7` cl. 2 declaration | the runner's step vocabulary | present under Limitations | pass |

## His answer — an ACCOUNT, relayed by the orchestrator, recorded verbatim and not merged into the verdict (`VER-§8` cl. 4)
The spec (2) sentence was put to him verbatim in Claude Code at about 17:07 local. Later, the orchestrator asked him a multiple-choice follow-up, and he typed this answer into its free-text field (typos kept):
> "I just did it, it scrolled to the second option in the menu, which was the "Sandbox(No Bot)" option, when I pressed Enter, it selected that option and loaded me into a sandbox match"

His answer reached the orchestrator at 2026-09-28 20:18:21 local = 2026-09-29 03:18:21 UTC. He gave no clock time. **What he said:** he pressed Down, the highlight moved to the second option "Sandbox(No Bot)", he pressed Enter, and the sandbox match loaded. **What he did not say:** whether he touched the mouse, or pressed Down only once. He named no other key. I infer nothing about either.

## Side by side — one process, one log file
Log: `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/Logs/GitClaudeUnrealTest.log`. This is the live log of PID 12112 (the session launched 16:58:12 local), 3298 lines at read time, with stamps in UTC. The file is quoted here because it becomes `GitClaudeUnrealTest-backup-<rotation time>.log` when this editor next exits. Every line below is from the same PIE session (world `UEDPIE_0_L_MainMenu`, created at 00:05:36) and the same process.

| arm | Down (entry line `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).`) | Accept (`IA_MenuAccept -> OnClicked.Broadcast() on '%s' ("%s").` / `IA_MenuAccept declined: …`) |
|---|---|---|
| control (injected by me) | **present**: L2787 `[2026.09.29-00.05.50:721][808]` + L2788 `focus moved 0 -> 1 of 7 ('Button_1')` | not injected (deliberately; `TASK-1395` reasoning) |
| his hands | **absent**: no `LogSiegeMenuInput` line between L2887 `[03.16.21:306]` and L2888 `[03.16.35:793]` (the two are consecutive: **no line of any category** sits between them) | **absent**, but the entry opened: L2888 `[2026.09.29-03.16.35:793][224]LogGitClaudeUnrealTest: [ASiegeGameMode::StartSandboxMatch] Main menu -> opening arena '/Game/Maps/L_Arena.L_Arena' into a fresh SANDBOX match (no bot, ?Sandbox=1, TASK-071).` → L2891 `LoadMap: /Game/Maps/L_Arena?Sandbox=1` → L2903 world `UEDPIE_0_L_Arena` up at 03:16:35.845 → L2904 `Sandbox match (?Sandbox=1, TASK-071): no bot opponent will spawn` |
| time since the control arm | Down is not stamped by any line. It is bounded to (03:16:21.306, 03:16:35.793), i.e. between **3 h 10 m 30.6 s** and **3 h 10 m 45.1 s** after L2787 | Enter's effect at 03:16:35.793 = **3 h 10 m 45.072 s** after L2787 |

The whole window from the control arm to his answer (L2789 → the travel), read in full: only `LogEOSSDK` config polls, 3 × `LogDerivedDataCache` maintenance, `LogAura` recorder summary L2802 `[00.20.36:762]`, 3 × `Cmd: log …` lines at 00:06:03 (my own Python reader), then L2887 and L2888. No `MoveFocus`, `HandleMenuDown`, `IA_MenuAccept` or `declined` line appears anywhere after L2789. Before the travel, the `…NOT armed.` warnings do not appear in this session.

## Outcome by letter (`TASK-1395` (3), subordinate to (2b), which is declared: Development Editor)
- **Down → (b).** The entry line is **absent** while the highlight moved (his account; also consistent with the log, since the entry that opened is Sandbox = `Button_1`, one below the `Button_0` focus placed at 03:16:21.306), **and** the control arm fired in the same log (L2787), **and** the arming line is present (L2743). ⇒ **route (i) measured for Down**: the real key did not reach `HandleMenuDown`.
- **Enter → (b), the Accept analogue.** No Accept line of either shape, fire or decline, yet `StartSandboxMatch` ran ⇒ the button's click was delivered **by a path other than** the subsystem's `IA_MenuAccept` handler. The Accept format strings were read at source this run (`SiegeMenuInputSubsystem.cpp` :1619–1626), and the build is the committed `9b82e8d`.
- **Pair → (b)+(b); no split.** Both of his keys are silent in the subsystem's instrument, and both had a visible effect. ⚠ This is **not** the rider's predicted "positive Down beside absent Accept" pair. The within-process positive in this log is **my injected** `Down`, not his. It is reported as itself.
- **Not (c):** no `declined: menu covered` / `no menu buttons` line exists.
- `VER-§8` cl. 11 is **not amended** (spec (4); that is `TASK-1544`).

## Evidence (promoted)
None. This is a log-line measurement. The lines the report rests on are quoted verbatim above, and the log is gitignored. No PNG is cited, so nothing is owed to a host.
- Film (not promoted): `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/AuraVerify/rec_1790640336435450300_1/recording.h264` (+ `recording_index.json`). nvenc, raw h264, 1280×720, 26105 frames, `stopped_reason: max_duration`, game time 0.40 s → about 900 s (time span 899.55 s), i.e. about 00:05:36 → 00:20:36 UTC. **It holds my control arm and about 14 minutes of idle menu. It does NOT hold his keys** (03:16 UTC, about 3 h later). No frame of it was read. `stop_pie_recording` returned a 2.84 M-char manifest; I read its 30-line header only. My own `start_pie_recording` was refused ("already active or armed") because `start_pie` had auto-armed this film. No other film exists for this run.

## Hypotheses (not verdicts)
1. HYPOTHESIS, mechanism not measured: Slate's own navigation (`FNavigationConfig`, Down → focus next) moved the highlight, and `SButton`'s key handling (Enter as the virtual accept key) clicked the focused `Button_1`. Both happened ahead of, or instead of, the Enhanced Input actions `IA_MenuDown` / `IA_MenuAccept`. This is the route (i) that `TASK-1389` named. No instrument sits on the Slate path.
2. HYPOTHESIS: the L2887 `ApplyInitialFocus` at 03:16:21.306 is the re-entry poll re-placing focus after the menu button lost Slate focus. The source says this line fires only when no menu button holds focus. Something cleared that focus after about 3 h idle, 14 s before his Enter. A window activation (for example a click into the Play window to give it focus) is one candidate. It is **not** measured and is recorded only because it bears on Limitations 1.
3. OBSERVATION, not his limb: after the sandbox opened, the Play window was destroyed at 03:16:47.795 (L3088 `Window 'Siegebound Preview …' being destroyed`). Two more PIE sessions followed, started by `Repeating last play command` at 03:17:18 (ended 03:17:21) and 03:17:57 (ended 03:18:02). Each logged only arming + `ApplyInitialFocus`, and no Down, Accept or travel line. These are his sessions, not mine, and not part of this measurement.

## Not examined / limitations this run
1. **Mouse not disclaimed (the load-bearing condition).** Spec (2)'s parenthesis voids the answer if he clicked. His account names only Down and Enter and does not say either way. A mouse click on `Button_1` would by itself produce exactly this log (no Down line, no Accept line, `StartSandboxMatch`). **So the (b)/(b) letters hold on the reading that he used the keyboard only.** One yes/no from him ("did you click anything in the Play window, including to focus it?") settles it. If he did, both legs become UNOBSERVABLE. No `LogViewport` mouse-mode line appears on the menu before the travel. That is weak corroboration only, because menu mouse modes may not log.
2. **Down count and timing:** his account does not say "once". No line stamps his Down, which is bounded only by L2887/L2888. The highlight's movement rests on his account plus the fact that the second entry opened. No frame covers it, because the film had ended.
3. **Answer channel:** his words arrived as the orchestrator's relay (`SC-§139` cl. 4(b) account), given in reply to a later multiple-choice question and not directly after the spec sentence.
4. `MEASURED` control, named: the injected `IA_MenuDown` in the same process and log (L2787/L2788), with the arming line L2743. This shows the instrument is live, so the silence is real in a Development build.
5. `VER-§7` cl. 2: names reached through `run_verification_sequence`: `ui_snapshot` ×4, `inject_input_action` ×2 (`IA_MenuDown`, `IA_MenuUp`), `wait_pie_seconds` ×2 (one call, `record: false`). No census §5 name was reached: no `pie_scene_edit`, spawn, delete, set_actor_*, teleport or `call_actor_function`. No `ui_perform` in the whole session.
6. Aura plugin version not read.
7. Board status token: the row read `backlog` at dispatch, not `built`/`qa-passed`. I ran it because it is the verifier's own measurement row, dispatched directly with no code subject.
8. `.sav` net zero, **proven**: all 5 files have identical sha256, size and mtime before (00:05, pre-PIE) and after (03:19). SiegeAccounts `2fd96fe1…42b1`/3083/1787982232.308 · SiegeDecks `646d442f…a071`/3814/1785690574.952 · SiegeDecks_4E46… `13f41048…65b9`/6520/1790489442.531 · SiegeSettings `c8555088…f2e7`/2004/1785907478.554 · SiegeSettings_4E46… `684ae64f…1793`/2056/1788986899.330. His sandbox run wrote no save.

## Editor state left behind (re-measured after the run)
| | before | after |
|---|---|---|
| PID | 12112 | **12112** (no lifecycle action by me) |
| `is_pie_active` | false | **false** (his window close ended my session at 03:16:47; his two later sessions also ended) |
| level | `L_Arena` → `L_MainMenu` (my load) | **`/Game/Maps/L_MainMenu`** |
| dirty content / maps | `[]` / `[]` | **`[]` / `[]`** |
| `BS_ERROR` in memory (liveness: histogram) | 0 of 19 · 0 of 27 after load | **0 of 37** (`BS_UNKNOWN` 17 · `BS_UP_TO_DATE` 20) |

## Recipes used
- `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md`: Step 0 only (the `ui_snapshot` name-selector focus read of `Button_0`). Re-verified in-run: **y** (`Button_0` focused "Play (vs Bot)" at t=13.913 and again at t=14.985).

## Recipe candidates (as first written)
- **Top-entry focus restore without `ui_perform`:** one `run_verification_sequence` (`record: false`) with `ui_snapshot Button_0` → `inject IA_MenuDown` → wait 0.5 → `ui_snapshot Button_1` → `inject IA_MenuUp` → wait 0.5 → `ui_snapshot Button_0` + `ui_snapshot Button_1`. It landed 2/2 (L2788/L2789 and the four reads). One run only.

---

## AMENDMENT 2026-09-29: his answer to the mouse question; line 1 MEASURED → UNOBSERVABLE
No PIE was started for this amendment. No new runtime reading was taken; the only new inputs are his answer and source reads.

### His answer (an ACCOUNT relayed by the orchestrator, beside the verdict and never merged into it, `VER-§8` cl. 4)
- **Asked:** a multiple-choice question in Claude Code, 2026-09-28, about 20:25 local. The question, verbatim: "During your Down+Enter just now, did you click anything in the Play window, including clicking it just to give it focus? The log shows focus being reset about 14 seconds before your Enter, which a click can cause."
- **Options, verbatim:** "No, keyboard only" · "Yes, I clicked to focus" (described as "I clicked the Play window once to give it focus, then used only Down and Enter.") · "Yes, I clicked the option" (described as "I clicked \"Sandbox (No Bot)\" or another menu entry with the mouse.") · "Not sure".
- **He chose: "Yes, I clicked to focus".** ⚠ His words are the choice only. The descriptive sentence ("once … then used only Down and Enter") is the orchestrator's wording, which he selected, not a sentence he wrote. He did **not** choose "Yes, I clicked the option".

### What a click in the Play window does to focus (each claim labelled)
1. **READ AT SOURCE (project):** the main menu runs under `FInputModeUIOnly`. `BP_MenuGameMode` BeginPlay → `SetInputMode_UIOnlyEx` (`qa/TASK-1424.md` §"THE CLAIM EVERYTHING RESTS ON", link 1, inspected on the asset graph there; restated in `SessionMenuWidget.cpp` "L_MainMenu's UIOnly + cursor posture is owned by BP_MenuGameMode"). I did not re-read the BP graph this run.
2. **READ AT SOURCE (project):** `ApplyInitialFocus` prints its line **only** inside `if (Buttons.Num() > 0 && !GetFocusedMenuButton())` (`SiegeMenuInputSubsystem.cpp`, the function body), and a looping poll calls it every `FocusReentryPollSeconds = 0.2f` (`SiegeMenuInputSubsystem.h`). ⇒ the L2887 line at 03:16:21.306 means **no menu button held focus at that poll**, and the subsystem then put Slate focus back on `Button_0` (`FocusWidget` → `FSlateApplication::SetUserFocus(…, EFocusCause::Navigation)`).
3. **MEASURED (log):** from L2789 (00:05:51.255) to L2887 (03:16:21.306), no `ApplyInitialFocus` line appears. Menu-button focus therefore survived about 3 h 10 m of the Play window sitting unattended. L2887 is the **only** focus-loss event in that span, and it falls 14.5 s before the travel.
4. **HYPOTHESIS (engine, not read this run):** the mouse-down on the Play window moved Slate keyboard focus off `Button_0` (onto the viewport or the window), and the next poll re-placed it. His account plus item 3's timing make this the most economical explanation. The engine's mouse-down focus rule was **not** read by me, and whether a click inside or outside `Button_0`'s bounds was the trigger is not known.
5. **Consequence (MEASURED log + READ-AT-SOURCE item 2):** whatever the click did, L2887 shows the keyboard's starting state **after** it: Slate focus placed on `Button_0` "Play (vs Bot)", the top entry. This is the state (1b) required.

### Which routes other than `IA_MenuDown` / `IA_MenuAccept` could have moved the highlight and fired the button
- **READ AT SOURCE (engine chain, via `qa/TASK-1424.md` links 2–4, engine lines cited there and not re-read by me):** `FInputModeUIOnly::ApplyInputMode` → `GameViewportClient.SetIgnoreInput(true)` (`PlayerController.cpp:6372-6386`). `UGameViewportClient::InputKey` returns early under `IgnoreInput()` (`GameViewportClient.cpp:767-770`), **before** any Enhanced Input processing. ⇒ on `L_MainMenu` a **real** key cannot reach `IA_MenuDown` / `IA_MenuAccept` at all. An injected action enters downstream of that return, which is why my control arm printed.
- **READ AT SOURCE (engine, via `SiegeMenuInputSubsystem.h` (5) "AND THE CONSEQUENCE THE ROW ASKED ABOUT" and `SiegePlayerController.cpp` at `HandleMatchEnd`; engine lines cited there and not re-read by me):**
  - Slate routes a key down the focus path from the focused widget.
  - `SWidget::OnKeyDown` returns `Handled().SetNavigation(...)` for an arrow key (`SWidget.cpp:416-429`), so **Down on a focused `Button_0` → Slate navigation to the next focusable, `Button_1`**. This is the "highlight moved" with no subsystem line.
  - `SButton::OnKeyDown` handles the Accept keys itself (`SButton.cpp:293-316`, via `FNavigationConfig` Accept rules), so **Enter on a focused `Button_1` → `ExecuteOnClick` → `OnClicked`**. This is the Sandbox travel with no `IA_MenuAccept` line.
- **MEASURED (log):** exactly this pair of effects appears (the second entry opened at 03:16:35.793), with **zero** `LogSiegeMenuInput` lines between L2887 and L2888. **Not measured:** the route itself. There is no instrument on the Slate path, so the Slate route is an at-source prediction that the log is **consistent with**, not a route the log shows.
- **Ruled out by his choice, not by the log:** a mouse click on the option, which would produce the same log. He did not choose "Yes, I clicked the option".

### What the (b)/(b) letters now rest on
- The instrument silence for both keys is **MEASURED** and unchanged: the control arm printed in the same process and log, and the build is Development (Editor).
- The key identity (a real Down, then a real Enter, not a click on the option) rests on **his account alone**. That account is now **"Yes, I clicked to focus"**.
- Spec (2)'s parenthesis, which the row calls load-bearing (`VER-§8` cl. 3(b)), reads verbatim: "If you clicked anything, … the answer does not count". A click to focus is "anything". ⇒ **as written, his limb does not count** ⇒ spec (3): "Any leg missing ⇒ `UNOBSERVABLE`".

### Line 1: decided `UNOBSERVABLE`, and the departure stated
- **Departure:** line 1 was `MEASURED` (2026-09-28). It is now `UNOBSERVABLE`, because the row's pre-written parenthesis voids an answer that includes any click, and a row-specific pre-written rule beats my reading of its purpose after the fact (the same ruling `qa/TASK-1395-verify.md` made against its dispatch). I do not soften the row (`SC-§101`).
- **Not a negative:** nothing measured contradicts route (i). The finding stands as an observation: both keys silent in the instrument, the effects present, the control live.
- **Restorable without a run:** the parenthesis's own stated reason is "a click or Space opens the entry by a different path". His focus click opened nothing, and L2887 re-placed focus on the top entry after it. If the manager rules that a focus-click lies outside the parenthesis (a spec amendment, `SC-§100`, and the manager's call), the (b)/(b) letters and `MEASURED` return on these same observations.
- **Practical note for the manager:** any sitting where the Play window has lost OS focus (he works in Claude Code between PIE start and his limb) will need a click to give it focus. So the parenthesis as written may be unsatisfiable in practice unless PIE starts while he is at the window.
- `VER-§8` cl. 11 **not amended** (→ `TASK-1544`).

### Board
The status token returns to `backlog`: an `UNOBSERVABLE` does not move a status (`VER-§5` cl. 2), and `backlog` was the token before this run. The line records `verify: unobservable`, that the row RAN, and this amendment.

---

## AMENDMENT 2 2026-09-29: TASK-1544's ruling; line 1 UNOBSERVABLE → MEASURED
Written by the playtest-verifier under `TASK-1591` (marker `TASK-1591-MENU-DOWN-ENTER-RELETTER`). This is text only: no PIE, no editor call, no new reading. Everything above is kept as written (`SC-§120`).

### The ruling this rests on
- **Marker `TASK-1545-FOCUS-CLICK-ADMISSIBLE-2026-09-29`.** This is the manager's `SC-§100` amendment of spec (2)'s parenthesis (`TASK-1544`, run inside `TASK-1590`), recorded as a 📎 on `TASK-1545`'s board row. From 2026-09-29, a click that only gives the Play window focus, made before his first key, does not void the answer, on condition that the log or a focus read shows focus on the top entry before his `Down`. A click on any menu entry, or Space, Tab, or any key other than `Down` and `Enter`, still voids it.
- **Disclosed on the row, restated here (`SC-§101`):** the ruling was made **after** the observations were known. It re-letters nothing the log shows.

### Fit check (`TASK-1591` spec (1)), done before any write
The ruling's condition is "focus on the top entry before his `Down`". Nothing recorded above contradicts it. L2887 `[03.16.21:306]` `ApplyInitialFocus: focus placed on the TOP option 'Button_0' ("Play (vs Bot)")` is the only focus-loss or re-placement line in about 3 h 10 m (first amendment, item 3). No further `ApplyInitialFocus` line appears before the travel at L2888 `[03.16.35:793]` (L2887 and L2888 are consecutive). The Side-by-side table bounds his `Down` to that window, as an inference from the log, not a stamp (Limitations 2). The entry that opened is `Button_1`, one below the top. So the ruling fits, and I found no reason to stop.

### What his limb now stands on
1. **His choice, an ACCOUNT relayed by the orchestrator (`VER-§8` cl. 4), kept beside the verdict:** "Yes, I clicked to focus", not "Yes, I clicked the option". That no click landed on a menu entry rests on this choice alone, not on the log.
2. **A log-plus-source reading:** L2887 shows the subsystem placing Slate focus on `Button_0`, the top entry. At source, that line prints only when a poll (`FocusReentryPollSeconds = 0.2f`) finds no menu button focused (first amendment, item 2). It fired once in this log, so the poll is live. Its **silence** for the 14.5 s up to L2888 therefore reads as "a menu button held focus throughout". This is a reading of an absence against a live line, not a stamp of his keys.
3. **Unchanged and MEASURED:** both of his keys are silent in the subsystem's instrument, with the named control live in the same process and log (injected `IA_MenuDown`, L2787 `[00.05.50:721]` + L2788). The arming line is L2743 and the build is Development (Editor). The effects are present: the second entry opened (`StartSandboxMatch`, 03:16:35.793 UTC).

### Letters
- **Down → (b)** · **Enter → (b)** (the Accept analogue) · **pair (b)+(b), no split.** These are the same letters as the 2026-09-28 Outcome section, on the same observations.
- ⇒ **line 1 `Verdict: MEASURED`**, with the control named in point 3 above (`VER-§1` cl. 3a / 5a).
- ⛔ **`MEASURED` is not a pass.** It never blocks and never bounces (`VER-§10` cl. 2). The route (i) mechanism stays a HYPOTHESIS (Hypotheses 1), because no instrument sits on the Slate path.
- Acceptance table row 1 reads the new line 1: `Verdict: MEASURED` (the cell as first written already reads this, and it matches again).
- Limitations 1 (mouse not disclaimed) is **answered**: a focus click, admitted under the ruling. It does **not** make the click-on-the-option exclusion measured; point 1 above says what that exclusion rests on.
- `VER-§8` cl. 11: not amended by me (its 2026-09-29 bullet is `TASK-1544`'s).

### If Jonathan overrules the ruling
The ruling is the manager's, and 🧑 Jonathan may overrule it. If he does, as the row and the ruling state (`TASK-1544` branch (e)):
- this report's line 1 returns to `UNOBSERVABLE`;
- `VER-§8` cl. 11's 2026-09-29 bullet is struck on the same day;
- a fresh sitting is boarded in which he starts PIE himself, so the Play window already has focus.
The observations above survive either way.

### Board
`TASK-1545`'s status line flips per `VER-§1` cl. 5a / `VER-§10` cl. 2 to `verified`, with the verdict word stated as **MEASURED**, not `VERIFIED`, and `verify: measured — <one line>`. The prior state is kept (`SC-§120`).
