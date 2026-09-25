Verdict: MEASURED
# Verification — TASK-1402 (5b runtime verification of TASK-1400, the menu re-entry focus re-arm)

**Lead sentence, because it is the gate this row was dispatched on:** the boot placement and the
post-return placement **could not be compared, because no return could be driven** — every placement
line this run produced names the **same** menu instance `WBP_MainMenu_C_0`, and that is a
consequence of the lane, not of the code. What *was* measured, twice, with a controlled negative:
**the new 0.2 s re-arm poll exists, fires, detects "menu uncovered + nothing focused", and puts focus
back on the TOP option `Button_0` ("Play (vs Bot)") with no input from me, logging TASK-1400's new
line each time.**

## Editor / Aura state

| item | value |
|---|---|
| Aura / MCP connected | **yes** (`get_headless_status` = `editor_connected`) |
| Editor process (`SC-§118`) | **PID 5728**, read in-process (`os.getpid()`); executable read in-process (`sys.executable`) = `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`. Matches build-master's 5a declaration exactly. |
| ⚠️ command-line caveat | `unreal.SystemLibrary.get_command_line()` returned an **empty string**, and this session's log records **no** `LogInit: Command Line:` line (first 250 lines scanned). ⇒ the identification rests on **PID + executable path + in-process configuration**, read from the running process, **not** on a literal command-line string. Declared rather than glossed. |
| **Build configuration (`VER-§9`)** | **`Development` (Editor)** — established two independent ways: (a) in-process `unreal.SystemLibrary.get_build_configuration()` = `Development`; (b) the executable is the **unsuffixed** `UnrealEditor.exe` (a Shipping target would be `UnrealEditor-Win64-Shipping.exe`). Engine `5.8.0-55116800`, build version `++UE5+Release-5.8-CL-55116800`. ⇒ **`UE_LOG(..., Log, ...)` is compiled IN**, so an absence in *this* session's log is a **real** absence. |
| Other Unreal processes | none touched. 🧑 his `-game` sessions were **not** interacted with in any way; no process was started or stopped except the two PIE sessions I started myself. |
| Map | I loaded `/Game/Maps/L_MainMenu` **myself** (`load_level`: `previous_level: /Game/Maps/L_Arena`, `already_open: false`, `loaded: true`, `discarded_unsaved: false`). **This is my declared state change**, per build-master's hand-off note. No absence was read before that load. |
| PIE mode | standalone, 1 client, window 1280×720 (reported viewport 1280×725), `client_index 0`. Two sessions, both started **and** stopped by me. ⛔ No PIE session I did not start was ever touched. |
| Dirty packages | **before: `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`** · **after: `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`**. Zero writes to any package. |
| Attempts | **2 of 3** (PIE session 1: the round-trip attempt; PIE session 2: the poll probe + its replication). Session 1's evidence is kept. |
| Wall time | ≈ 28 min |
| Editor left | **up, PID 5728, on `/Game/Maps/L_MainMenu`, PIE stopped (`get_game_world()` = `None`), 0 dirty packages.** |

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence) | pass/fail/unobs |
|---|---|---|---|---|
| (1) | is the deck builder's Exit reachable by an injectable action? — pre-branched (i)/(ii) | drive `IA_MenuDown`×2 + `IA_MenuAccept` in, then exit | **BRANCH (ii) — NOT REACHABLE.** Entry works and is re-proven: `MoveFocus(+1): focus moved 0 -> 1` then `1 -> 2 ('Button_2')`, `IA_MenuAccept -> OnClicked.Broadcast() on 'Button_2' ("Deck Builder")`, builder on screen at PIE t=33.50. **Exit:** no Enhanced Input action exists for it — `DeckBuilderWidget.cpp` binds **raw keys** in `NativeOnKeyDown`, and `Escape`/`Gamepad_FaceButton_Right`/`Virtual_Gamepad_Back` are **fenced to `if (bGridFocused)` → `ExitCardGridFocus()`** (`.cpp:1727-1733`), i.e. they leave the *card grid*, never the builder. The Exit is a `UButton` (`Button_3`, child `TextBlock_10` = "Exit") whose handler is pure BP. | **unobs** |
| (1b) | can the Exit be driven by the pointer-actuation lane instead? | `ui_perform` click on `Button_3` | **NO — 3 attempts, all identical:** ① click at the text centre → `down handled:true, hit_widget:"STextBlock"`, **`up handled:false`**; ② `press` → `wait 6 frames` → `release` → same, **`up handled:false`**; ③ click at `dx:-24` so the hit widget was the **`SButton` itself** → `down handled:true, hit_widget:"SButton"`, **`up handled:false`**. Builder still on screen after each (frames at PIE t=102.89 / 244.11 / 515.43). ⇒ `OnClicked` never fires. | **unobs** |
| (1c) | **CONTROL for (1b)** — is the lane dead, or is this button special? | same click on a *different* button, different parent (`Close`, the info-panel button) | **The lane is dead for clicks, not the button:** `down handled:true, hit_widget:"STextBlock"`, **`up handled:false`**, panel unchanged (frame at PIE t=338.89). **Two different buttons, two different hit-widget classes, one signature.** | control fired |
| (2) | read TASK-1400's new log line — present/absent, file + line + UTC stamp, **after declaring the build configuration** | grep every `Saved/Logs/*.log` (83 files) for `focus placed on the TOP option` | **PRESENT — 6 occurrences, all in `Saved/Logs/GitClaudeUnrealTest.log` (PID 5728, Development Editor):** `line 2753` `[2026.09.25-00.45.29:874][446]` · `line 3841` `[2026.09.25-00.56.01:015][631]` · **`line 3899` `[2026.09.25-00.56.27:825][100]`** · **`line 3900` `[2026.09.25-00.56.28:025][112]`** · **`line 4065` `[2026.09.25-00.58.04:518][337]`** · **`line 4066` `[2026.09.25-00.58.04:812][349]`**. Full text of line 3899: `LogSiegeMenuInput: [USiegeMenuInputSubsystem] ApplyInitialFocus: focus placed on the TOP option 'Button_0' ("Play (vs Bot)"), index 0 of 7, in menu instance 'WBP_MainMenu_C_0'.` (stamps are the engine's **UTC**; local clock was 2026-09-24 17:45 / 17:56 / 17:58). | **pass** |
| (2b) | the same line in 🧑 **his own `-game` sitting** | the other **82** log files, cited by backup filename | **ABSENT from all 82** — and that absence carries **no weight**, by measurement: the newest non-current log is `GitClaudeUnrealTest-backup-2026.09.25-00.35.36.log` (mtime **2026-09-24 17:35:36**), which is the **pre-build** editor rotating out; 5a's editor opened at **17:36:51**. ⇒ **no session of any kind has run on the new binaries except mine.** Additionally, per `VER-§9`, for his historical `-game` logs I cannot establish the build configuration from in-log evidence, so those absences would be `UNOBSERVABLE` even if the code had predated them. | **unobs (expected)** |
| (3) | STATE limb — `Button_0` `focused: true` after a **successfully driven return**; `false` or the wrong button ⇒ `VERIFY-FAILED` | `ui_snapshot` focus read on `Overlay_19/VerticalBox_0` | **NOT EXERCISED — the pre-decision does not trigger, because no return was ever driven** (rows (1)/(1b)/(1c)). ⛔ This is **not** a `VERIFY-FAILED`: nothing was observed failing. | **unobs** |
| (3b) | **the mechanism under (3), reached another way** — does the new 0.2 s poll re-place focus on the TOP option when the menu is uncovered and nothing on it is focused? | knock focus off with a pointer **down** on the viewport background (`hit_widget: "SPIEViewport"`, the one pointer phase this lane *does* deliver), then read focus + the log | **OBSERVED, TWICE.** **Trial 1:** `Button_0 focused: true` @ t=25.676 → click @ t=25.876 (the perform's own before/after snapshots both read `Button_0 focused: false`) → **`focused: false` @ t=25.976** → **`focused: true` @ t=26.692**, no input in between; log lines `00:56:27.825` and `00:56:28.025` — **200 ms apart, exactly `FocusReentryPollSeconds = 0.2f`**. **Trial 2 (replication):** click @ t=119.57 (before/after snapshots both `focused: false`) → `focused: true` again by t=119.706 → still true @ t=120.607; log lines `00:58:04.518` and `00:58:04.812`. **The only other caller of `ApplyInitialFocus` repo-wide is `SetTimerForNextTick` in `OnWorldBeginPlay` (`.cpp:116`), which had run ~26 s / ~120 s earlier.** | **pass** |
| (3c) | focus **CAUSE** (gate `TASK-1451` WARN-2) | source + drive history | **`Navigation`, not `Mouse`.** The placements were made by `FocusButton()`, which calls `FSlateApplication::SetUserFocus(..., EFocusCause::Navigation)` (`.cpp:495`, fallback `:501`), and each placement is logged. WARN-2's ringless-but-`Mouse`-focused steady state was **not** the state measured: the only pointer events I issued landed on `SPIEViewport` (background) and on deck-builder widgets, never on a menu button. | **pass (declared)** |
| (4) | PIXEL limb — is a focus ring drawn? | composited capture, default `max_dim` 1086 on a 1280-wide viewport (1.18× downscale), read by eye | **NO RING RESOLVED** on `Button_0` in any frame, including the frame taken while the structured read said `Button_0 focused: true` 24 ms earlier. ⛔ **`UNOBSERVABLE` on the pixel limb only, per `VER-§11` cl. 9(b) and this row's consolidated branch (β): this row specs NONE of the four parts** (explicit `max_dim` ≥ viewport long edge · `max(B−R)` scoring · regions derived once · a floor written to disk before the run + a discriminating control). *Eligible ≠ specced* — I did not upgrade my capture at the keyboard. ⛔ **This does not bounce `TASK-1400` and is recorded BESIDE the state result, never merged into it** (`VER-§8` cl. 4). (γ) does not apply: my capture did **not** show a ring, so there is no corroboration to report. | **unobs** |
| (5) | 🧑 his half — returned, not driven | — | Returned verbatim below. **Not asked by me** (`VER-§8` cl. 3(b)/3(c)). | returned |

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1402-a2-top-option-focused-before-viewport-click.png` — PIE t=118.458, frame 71337, 1086×615, mean_luma 195: the seven-entry main menu over the sky/plain; `Play (vs Bot)` at the top of the column carries **no visible outline** although the structured read had it focused. (`Multiplayer`/`Button_3` again shows a brighter fill — the known area artefact `TASK-1446` §3, **not** focus.)
- `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1402-a2-top-option-focus-restored-by-poll.png` — PIE t=120.631, frame 71376, 1086×615, mean_luma 195: pixel-indistinguishable from the frame above, taken 24 ms after the structured read `Button_0 focused: true` that the **poll** had just restored. The pair is the pixel-limb finding: **the state changes and the pixels do not, at this capture configuration.**

⚠️ **ONE PROMOTED FILE IS MIS-SLUGGED AND I DO NOT CITE IT — declared rather than left to be found** (same fault `TASK-1399` was corrected for; a slug is a claim, and I have no shell to delete the file):
`.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1402-a2-menu-unfocused-after-viewport-click.png` (PIE t=119.723) — its slug asserts an unfocused menu, but the structured read 17 ms **earlier** (t=119.706) already read `Button_0 focused: true`: **the poll beat my capture.** The frame is real, the slug is false. **Deletion is owed to the host row (`TASK-1403`).** The fact it records is itself signal — the re-arm can complete in under ~130 ms.

Everything else stays in `Saved/` (gitignored): `Saved/AuraVerify/ver1402_boot_t10.05s_f29991.png`, `…/ver1402_deckbuilder_t33.50s_f31344.png`, `…/ver1402_reentry_t102.89s_f35142.png`, `…/ver1402_reentry2_t244.11s_f43236.png`, `…/ver1402_reentry3_t515.43s_f58673.png`, `…/ver1402_control_close_t338.89s_f48589.png`, plus two auto-armed background recordings (`Saved/AuraVerify/rec_*`) I neither collected nor needed.

## 🧑 His half — returned UNTRIMMED, not merged into any verdict (`VER-§8` cl. 3(b)/3(c)/4)

Verbatim from the `TASK-1400` section header, including the parenthesis:

> 🧑⛔⭐ **TWO-LIMB ACCEPTANCE BINDS — ⛔ READ IT ⛔ ONLY THROUGH `TWO-LIMB-CONDITIONED-BY-VER-11-CL-9-2026-09-24` — ⛔ STATE limb (🤖 machine) ⛔ AND PIXEL limb ~~(🧑 his eye)~~ ⛔ **(🧑 his eye for ⛔ *"does it LOOK RIGHT"* — ⛔ FOREVER; 🤖 ⛔ MACHINE-ELIGIBLE for ⛔ *"is a ring PRESENT"* ⛔ under the ⛔ four-part bar, `VER-§11` cl. 9(a)/(c))**, ⛔ NAMED SEPARATELY, ⛔ NEITHER STANDING IN FOR THE OTHER** ⚙️🧑⛔⭐⭐ **🧑 HIS NAMED GAP: ⛔ RETURNING TO THE MAIN MENU LEAVES ⛔ NO RING. ⛔ MEASURE ⛔ WHY, ⛔ FIX IT, AND ⛔ SHIP ⛔ ONE LOG LINE SO 🧑 HIS OWN SITTING ⛔ PRODUCES EVIDENCE.**

⛔ An `UNOBSERVABLE`/`MEASURED` plus his confirmation is **not** a `VERIFIED`. His answer is recorded alongside this verdict, never inside it.

## Why the verdict word is `MEASURED` and not `UNOBSERVABLE`, with the control named

`MEASURED` is earned by a control that discriminated; `UNOBSERVABLE` means the lane could not see at
all. A row that writes `MEASURED` without naming its control is writing `UNOBSERVABLE` in a better
suit — so the control is named:

1. **Instrument proven alive, same session, seconds before the negative (`SC-§137`):** the identical
   `ui_snapshot` focus read returned `Button_0 focused: true` at PIE t=25.676 and t=118.458, and had
   returned `Button_2 focused: true` after two injected `IA_MenuDown`s. It discriminates focused from
   unfocused, on these exact nodes, minutes apart.
2. **The controlled negative:** immediately after the viewport-background click the same read
   returned `Button_0 focused: false` (t=25.976), corroborated independently by the perform's own
   before/after snapshots in both trials.
3. **The positive, with no input:** `focused: true` at t=26.692, and a matched pair of
   `focus placed on the TOP option` log lines **200 ms apart** — the poll's exact period.
4. **Replicated** once, same shape, ~93 s later.

⇒ the lane **did** see, and what it saw was the shipped mechanism working. What it could **not**
reach is the *trigger* the row names (a return that destroys and rebuilds `WBP_MainMenu`) and
therefore the *fresh-instance* discrimination. `VERIFIED` would overclaim that; `VERIFY-FAILED` is
false (nothing failed); `UNOBSERVABLE` would erase a discriminating measurement.
Per the manager's `TASK-1399-MEASURED-TOKEN-RULED-2026-09-24` ruling, `MEASURED` flips this row's
`status:` to `verified` with the word `MEASURED` in the first sentence of the status line, and it
**never blocks and never bounces** — `TASK-1403` is not held by this row.

## Hypotheses (not verdicts)

- **H-A (why the click lane is dead):** `FSlateApplication::SetUserFocus`-style routing aside, an
  `SButton` fires `OnClicked` from `OnMouseButtonUp` only while it **holds mouse capture**; the
  synthetic `down` reported `handled: true` but the matching `up` reported `handled: false` on every
  one of four attempts, which is the signature of a `down` whose `FReply::CaptureMouse()` was never
  applied. **HYPOTHESIS.** It is a lane property, not a project property, and it is **not mine to
  fix** — but it means **no sub-screen in this project can currently be closed by an agent**, which
  is a ceiling every remaining `MENU-NAV` verification row inherits.
- **H-B (why the placement line appears in pairs):** the first poll's `SetUserFocus` returned false
  and deferred through `LocalPlayer->GetSlateOperations()` (`.cpp:501`), so the next poll still saw
  nothing focused and re-placed. This is exactly the code's own documented "a repeat is signal"
  note; it is **benign here** (it self-terminates in one extra tick) but it is the first runtime
  sighting of that path. **HYPOTHESIS.**
- **H-C:** if H-A is right, 🧑 his own hands are currently the **only** way to produce the
  post-return placement line — which is precisely what deliverable (4) was shipped for, and the line
  is confirmed to exist and to name the instance.

## Not examined / limitations this run

- ⛔ **The row's headline gate was not reached:** boot instance vs post-return instance could not be
  compared. All six placement lines name `WBP_MainMenu_C_0` because the widget was never destroyed
  and rebuilt. **Do not read the matching instance names as evidence of anything** — they are one
  widget observed six times.
- ⛔ **Neither return path was driven.** The deck-builder Exit is unreachable (above). The session
  menu's return (`SessionMenuWidget.cpp:151-165`) was not attempted: reaching it needs a match and
  its own button click, which H-A says would fail the same way.
- ⛔ **The two-coexisting-`WBP_MainMenu` finding was NOT observed and NOT chased** (per the dispatch).
  Only one instance ever existed in either session; the condition requires a completed Exit.
- ⛔ **`VER-§7` cl. 2 declaration:** the runner steps I reached were `wait_pie_seconds`,
  `inject_input_action`, `ui_snapshot`, `ui_perform`, `capture_pie_frame` — **all five are also
  directly-granted top-level tools in my own set**, so none was reached *only* through the runner.
  ⛔ I did **not** use `pie_scene_edit` or any census §5 PIE-world mutation name, and I issued no
  spawn, delete, property write or function call into the PIE world.
- ⛔ **No pixel-limb claim of any kind is made**, positive or negative (`VER-§11` cl. 9(b)).
- ⛔ Not examined: `MoveFocus` wrap-around, `IA_MenuUp`, the Settings/Login/Session screens, the
  poll's per-tick cost (gate WARN-1), and the `Tests/SiegeMenuInputTest.cpp` race (gate WARN-3).
- ⚠️ The engine log stamps are **UTC** (`2026.09.25-00.xx`); the local run date is **2026-09-24**,
  which is the evidence folder used.
- ⚠️ One MCP round trip measured as much as ~70 s of PIE clock; every reading above is quoted with
  the PIE time it was taken at, never inferred from an adjacent call.
