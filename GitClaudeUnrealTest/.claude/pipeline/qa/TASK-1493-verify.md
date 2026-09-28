Verdict: VERIFIED
# Verification — TASK-1493 [UI-PERFORM-CLEARS-FOCUS-PROBE]

**H1 HOLDS, on both fixtures, with no split.** A `ui_perform` call took keyboard focus off the focused widget on **8 of 8** runs (7 of 7 counting only runs with a read-back taken right before). `simulate_key_press` in the same place left focus on **5 of 5**. Each arm was unanimous, and the two arms disagree on every fixture. A standalone `ui_snapshot` also left focus on (**2 of 2**).

> Line 1 note: the row's acceptance asks for `Verdict:` "H1 HOLDS / REFUTED / UNOBSERVABLE". `VER-§1` cl. 1 fixes line 1 to the four byte-literal words, so line 1 reads `VERIFIED` (every acceptance line observed, none failed). **The row's answer, H1 HOLDS, is this paragraph's first sentence.**

Editor/Aura state: Aura connected **y** (`get_headless_status` → `editor_connected`). Editor identified per `SC-§118` cl. 9 by command line before PIE, from inside the answering process (`os.getpid() = 3108`, and a `Win32_Process` query for `UnrealEditor%` listing exactly one process): `3108|"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`. That is the GUI editor; there was **no `-game` instance**. Map `/Game/Maps/L_MainMenu`, dirty maps `[]`, dirty content `[]`, `is_pie_active` → `false` on arrival, so no session of his existed or was touched. PIE: standalone, 1 client, viewport 1280×725, DPI 0.6706. Route: menu → "Play (vs Bot)" → `L_Arena`, a live vs-bot match. **Attempts used: 1 of 3** (one PIE session, arena t≈0–193 s). Wall time ≈ 25 min. Credit: not visible in any tool reply. PIE announcement, as the dispatch worded it: *"Jonathan is present; the PIE announcement was made in Claude Code; standing grant `VER-§3` cl. 6."* (`VER-3-6-THE-REPORT-RECORDS-THE-ANNOUNCEMENT`). His batch approval as the dispatch quoted it: *"yes go ahead and run all those tasks"*. After the run: PIE stopped, editor world `L_MainMenu`, game world `None`, dirty `[]` / `[]`, PID 3108 left up. model (self-reported): "Opus 5.5 (1M context)" (id `claude-opus-5-5[1m]`, as shown in my environment block).

Row status on arrival was `backlog`, marked "BOARDED, NOT DISPATCHED". This row is a probe owned by playtest-verifier, not a verification of someone else's binaries. It was dispatched explicitly, with `TASK-1491` and `TASK-1404` recorded as done first and `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` recorded, so I ran it. Spec (1) does not fire, per that manager marker.

**Recording:** `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/AuraVerify/rec_burst_639261314611080000/recording.h264`. It was auto-started by the in-batch `record_burst` after the level travel: nvenc, `container: h264`, 3261 frames at 30 fps, `duration_seconds 108.7`, `time_span_seconds 119.97`, `stopped_reason: max_duration`, and index `recording_index.json` beside it. ⚠ It covers arena t≈0–120 only, so the whole console fixture and the first detail-page run (D-U1). **The rest of the detail-page fixture (t≈123–193) is on no film.** A second film, armed at `start_pie`, holds the menu only: `Saved/AuraVerify/rec_1790534653513102700_11/recording.h264`. I saw no `.mp4`; the remux is the orchestrator's.

## Instrument, and its two-sided control

- **Reader:** `execute_unreal_python_readonly` on the live PIE world. I found the widgets by `unreal.ObjectIterator` under the `GameInstance_4` outer and called `UWidget::HasKeyboardFocus()`, `HasFocusedDescendants()` and `HasUserFocusedDescendants(pc)` on them. This is the `TASK-1489` §2.2 reader, re-used.
  - **Console fixture:** `…GameInstance_4.SiegeAssistantConsoleWidget_0` → `InputBox` (`EditableTextBox`), plus the root's `HasFocusedDescendants`.
  - **Detail fixture:** `…SiegeControlsHelpWidget_0.WidgetTree.DetailView.WidgetTree.BackButton` (`Button`), plus the help root's `HasFocusedDescendants`. `BackButton.HasUserFocusedDescendants(pc)` read `False` even when focused, which is expected for a leaf. The verdict rests on `HasKeyboardFocus` and the root's `HasFocusedDescendants`, and those two agreed at every sample.
- **The reader moved in both directions within this session, on the same widgets.**
  - `True` → `False` after every `ui_perform`.
  - `False` → `True` after every re-establish. On the console that was `IA_AssistantConsole` ×2 (close, then open; `OpenConsole` → `FocusInputBox`, `SiegeAssistantConsoleWidget.cpp:620`), 4 of 4. On the detail page it was `IA_MenuDown` on the one-stop ring, 3 of 3 read back.
  - So it is not a constant.
- **Contamination check at every read:** I listed every `GameInstance` `UserWidget` whose name contains `Victory`. The result was `[]` at every one of 26 reads, through arena t=183.89. The victory screen never registered, so no read was taken on a contaminated world (the `TASK-1489` H3 / `TASK-1498` hazard).
- **Spacing:** every read, act and re-establish was a separate MCP round trip, ≥ 2.5 s apart on the PIE clock. In-batch waits were 0.4–0.8 s.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, PIE t) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | (1) did `TASK-1491` already settle it? | manager marker `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` | Not settled (a second uncontrolled sighting, no `simulate_key_press` arm). ⇒ the probe ran. | pass |
| 2 | (2) probe arm: focus `True` → ONE `ui_perform` step → read | the reader above, before and after | **Console `InputBox`:** U1 `type "x"` (act t=9.52): `True` (t=6.02) → **`False`** (t=15.74) · U2 `type "x"` (t=42.45): `True` (38.71) → **`False`** (48.31) · U3 `wait 1 frame` (t=59.04): `True` (56.10) → **`False`** (64.27) · U4 `type "x"` (t=91.15): `True` (86.94) → **`False`** (96.64). **Detail `BackButton`:** D-U1 `scroll delta -1, selector DetailScrollBox` (t=116.85; step target resolved to `DetailScrollBox`): `True` (110.59) → **`False`** (123.38) · D-U2 `wait 1 frame` (t=140.24): `True` (136.98) → **`False`** (147.08) · D-U3 `move, selector BackButton` (t=167.32; target resolved to `BackButton`): `True` (162.61) → **`False`** (172.72) · D-U4 `wait 1 frame` **inside `run_verification_sequence`, no `widget` param** (t=181.62): no read-back right before (it followed an `IA_MenuDown` re-establish at t=180.77, which gave `True` 3 of 3 elsewhere) → **`False`** (183.89). In every "after" read the screen was still up: console `IsConsoleOpen True`; help root `is_in_viewport True`. | pass |
| 3 | (2) control arm: same sequence with `simulate_key_press` in place | same reader | `simulate_key_press "K"` (deliberately unbound; reply `binding_found: false`, "delivered to the player controller"). **Console:** K1 (t=29.41) `True` (26.12) → **`True`** (32.53) · K2 (t=35.57) `True` (32.53) → **`True`** (38.71) · K3 (t=76.69) `True` (73.92) → **`True`** (79.79). **Detail:** D-K1 (t=134.03) `True` (131.19) → **`True`** (136.98) · D-K2 (t=159.67) `True` (153.73) → **`True`** (162.61). | pass |
| 4 | (3) each arm repeated; counts; unanimity | tally of rows 2–3 | **`ui_perform`: 8 runs, 8 cleared** (7 of 7 with a read-back right before; D-U4 is lower grade). Unanimous. **`simulate_key_press`: 5 runs, 5 kept.** Unanimous. **No split.** Extra arm, not in the spec: **standalone `ui_snapshot`, 2 runs, 2 kept** (console depth 1 at t=83.70: `True` 79.79 → `True` 86.94; detail in-sequence at t=151.13: `True` 153.73 after, and the snapshot itself read `BackButton` `"focused": true`). | pass |
| 5 | (4) H1 holds ⇒ blast radius: landed reports that took a focus reading after a `ui_perform` | census of `qa/*-verify.md`, read-only: 20 files mention `ui_perform`, 24 carry a focus reading; I read the intersection by run order | List under **§ Blast radius** below: **4 reports affected, 2 checked and cleared, 1 already self-handled.** Not re-opened and not re-verdicted; handed to the manager (`SC-§101`). | pass |
| 6 | `## Not examined / limitations` | — | present | pass |

Row rule (`VER-§1` cl. 5): no `fail`, ≥1 `pass` ⇒ `VERIFIED`, 6 of 6 observable.

## ⭐ The shape finding: it is the CALL, not the step

The spec asked to "name which `ui_perform` shapes were tested". The result is broader than any one shape.

- **Shapes tested:** `type`, `scroll`, `move` and **`wait` (1 frame, a step that actuates nothing)**. Each cleared focus every time it ran.
- **Call forms tested:** standalone `ui_perform` with `widget` set, and `ui_perform` inside `run_verification_sequence` with no `widget`. Both cleared.
- **The clear shows up in `ui_perform`'s own `before_snapshot`.** On `BackButton` the reader read `True` 3.3 s, 6.3 s and 4.7 s before D-U2, D-U1 and D-U3. Each perform's `before_snapshot` reported `BackButton … focused: false`, with no other input in between. On the same node, a standalone `ui_snapshot` read `"focused": true` (t=151.13). **So the `ui_snapshot` reader can say `true` for this node, and the perform's own before-snapshot did not.** ⇒ Focus is gone by the time `ui_perform` takes its own before-snapshot. This is an observation on 3 of 3 runs. When the clear happens inside the call, and why, is under Hypotheses.
- ⛔ **Not tested:** `click`, `double_click`, `press`/`release`, `drag`, `assert`, `wait_for`, `snapshot`-as-a-step. Since a no-op `wait` step clears focus, I expect these do too, but that is **not measured**. Do not generalise past the four shapes above.

## Blast radius (spec (4)) — for the MANAGER; no row re-opened, no verdict changed

What counts: a **Slate** focus reading (`ui_snapshot`/`ui_perform` `focused`, `HasKeyboardFocus`) taken after a `ui_perform` in the same session, with **no focus-setting input in between**. Readings that follow an `IA_MenuDown`/`IA_MenuAccept`/console open are **not** affected: this run measured that those put focus back, 7 of 7. Log lines from `USiegeMenuInputSubsystem` (`MoveFocus … of N`) track the subsystem's own index, not Slate focus, and are **not** affected.

**Affected (4):**
1. **`qa/TASK-1489-verify.md` §4 "Reason A".** Its "measured **false negative**" says `ui_snapshot`'s `focused` field is blind for a `UEditableTextBox`. It rests on `ui_perform`'s snapshot reading `InputBox … focused: false` "while the Python predicate had just read `HasKeyboardFocus = True` … seconds earlier". **By this run, the `ui_perform` itself clears focus before its own snapshot.** ⇒ That comparison no longer shows the field is blind. It may be blind, or it may have read a focus the perform had just removed. **Reason A is now UNSUPPORTED, not refuted.** No standalone `ui_snapshot` of `InputBox` with focus proven on was taken this run: my console `ui_snapshot` was depth 1 and did not reach the box. `1489`'s verdict (focus-on-open `True` ×3 by `HasKeyboardFocus`) is **not** touched. Its H1 sighting is what this row confirms.
2. **`qa/TASK-1402-verify.md` row (3b).** It says focus was "knocked off with a pointer **down** on the viewport background", "the one pointer phase this lane *does* deliver". Its own words: "the perform's own before/after snapshots both read `Button_0 focused: false`". The before-snapshot already read `false`, which by this run is the perform's clear, not the click's. ⇒ **The cause it gives for the knock-off is suspect.** The row's finding, that the 0.2 s poll puts focus back, is **not** weakened: the `False → True` transition, the 200 ms log pair and the no-input window all stand, whatever cleared focus first.
3. **`qa/PLAYTEST-archer50-verify.md` §1(b), the `Tab` row.** "no `SlotButton` `focused`" at t=85.11 came after `ui_perform` clicks at t=43.63 and t=57.13, with no refocus in between. ⇒ **That `false` may be the perform's.** It was corroboration only: the row's effect reader was `EditingDeckIndex`, which is unaffected.
4. **`qa/TASK-1436-verify.md` row 2e.** `InputBox.focused: false` came from the `ui_snapshot`/`ui_perform` lane in a session where `ui_perform type "focusprobe"` also ran. The row is already `UNOBSERVABLE`, and `1489` superseded it. I did not establish the order of its reads; listed for completeness.

**Checked and cleared (2):** `qa/TASK-1390-verify.md`: its focus reads after the pointer arms (t=142.53, 163.60) each follow an `IA_MenuDown`, and H3 cites the subsystem index. `qa/TASK-1270-verify.md`, `qa/TASK-671-verify.md`, `qa/TASK-787-verify.md`: 1270 and 671 ran no `ui_perform` in-session, and 787 took no focus reading.

**Already self-handled (1):** `qa/TASK-1491-verify.md` re-ran its affected press pair after "a `ui_perform` disturbed the focus ring" (its line 67), and its item 7 is itself a sighting of H1.

⚠ **Also affected: every past use of `ui_perform`'s own `before_snapshot`/`after_snapshot` `focused` fields**, in any report. By this run, those fields read after the clear. I found them cited in `1402` and `1489`. The census did not open every `ui_perform` trace in all 20 files; see Not examined.

⚠ **A gesture-level implication, not measured this run:** `TASK-1491` item 7 saw an `IA_MenuDown` + `IA_MenuAccept` after a `ui_perform` "land on the list page". If a report's navigation step came straight after a `ui_perform`, the thing it navigated from may not have been the thing it read. Worth a manager look at `RCP-deckbuilder-slot-and-card-edit.md`, which chains `ui_perform` `double_click` batches with injected navigation.

## Evidence

**Nothing promoted.** No frame proves this finding. The pair I captured (below) is a **pixel null**: the button looks the same with and without focus, which matches `TASK-1402` row (4) on the main menu. The evidence is the quoted reads above, each stamped with PIE time.

Captured, left in `Saved/` and cited only as that null:
- `Saved/AuraVerify/t1493-detail-backbutton-focused-before_t181.27s_f2730829.png` (composited 1280×725): the "Move" detail page over the dimmed arena and a grey "Back to the controls list" button at bottom centre. HUD readout "46 FPS · 21.7 ms"; VRAM banner visible.
- `Saved/AuraVerify/t1493-detail-backbutton-after-uiperform_t182.01s_f2730857.png`: the same page after D-U4. **Positive control:** it is a different frame ("36 FPS · 28.0 ms", and the VRAM banner figure changed). The button is **visually identical by eye**, with no ring in either frame. ⇒ **Focus cannot be seen in the pixels on this build.**

## Hypotheses (not verdicts)

- **H-M1 (mechanism): `ui_perform` puts Slate user focus onto the game viewport, or its virtual pointer, at scenario start.** Its trace shows the pointer at (1223, 1244) even for a `wait` step, with `cursor_restored: true`. **When** the clear happens is observed (before its own `before_snapshot`, 3 of 3). **How** it happens is **NOT MEASURED**; I read no Aura plugin source.
- **H-M2:** the console's `OpenConsole` "re-assert focus" branch (`cpp:506-514`) would recover focus from a single `IA_AssistantConsole` press. The controller toggle is close-first, though, so the recovery that works is two presses (close + open), 4 of 4. I did not read the controller.

## Not examined / limitations this run

- **Shapes not tested:** `click`, `double_click`, `press`/`release`, `drag` (with or without `hold_frames`), `assert`, `wait_for`, `snapshot` as a step. The four tested shapes are the full scope of this result.
- **The console fixture got no `scroll`/`move` run, and the detail fixture got no `type` run.** The shapes were spread across the two fixtures rather than crossed on both.
- **D-U4 has no read-back right before it** (it came after an `IA_MenuDown` in the same batch). It counts as lower grade: 8 of 8 overall, 7 of 7 strict.
- **No pure no-action interval control.** The `simulate_key_press` arm stands in for one: the same round-trip latency and a similar 3–6 s elapsed time, and focus survived 5 of 5. `TASK-1489` row 6 had already shown console focus survive 20.3 s untouched.
- **`ui_snapshot`'s `focused` field on `UEditableTextBox` was not re-tested** with focus proven on (my console snapshot was depth 1). So `1489` Reason A is unsupported, not refuted, and a depth-4 standalone `ui_snapshot` of `InputBox` right after a `True` read would settle it. **A recipe candidate for the next sitting.**
- **The blast-radius census is by grep and targeted reads** of the files that both mention `ui_perform` and carry a focus reading. I did not open every `ui_perform` trace in all 20 files, and non-`-verify` reports (`qa/TASK-###.md` reviews, handoffs) were not censused.
- **Film gap:** arena t≈120–193 (most of the detail fixture) is on no film (`max_duration` stop).
- **`pie_scene_edit` / `call_actor_function`: not used, 0 calls.** Tools reached through `run_verification_sequence` this run: `ui_snapshot`, `inject_input_action`, `record_burst`, `wait_pie_seconds`, `capture_pie_frame`, `ui_perform`. The reachable-not-granted declaration (`VER-§7` cl. 2) has nothing to list.
- `stop_pie_recording`'s reply overflowed the tool limit (351,876 chars); I read the head and grepped for `video_path`/`stopped_reason`.
- Aura plugin version: not read.
- **`.sav` net zero, proven.** sha256 (first 16 hex) and mtime were identical before and after for all five files: `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` `13F410482847DA5B` 09-26 23:10:42 (6520 B); `SiegeAccounts.sav` `2FD96FE18AF9A4CF`; `SiegeDecks.sav` `646D442FC11C2770`; `SiegeSettings.sav` `C8555088E2918C51`; `SiegeSettings_4E46…34.sav` `684AE64F9378BDF3`. Nothing was written, so nothing needed restoring.

## Recipes used

- `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md`: **Step 1 rows 1–4 only** (the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, in-batch `record_burst {"seconds":1}`). **Re-verified in-run: y** for those rows. `Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85. The travel to `L_Arena` landed (the game world read `UEDPIE_0_L_Arena` at t=6.02), and the burst auto-started the arena film. No walk, capture or summon was done.
- `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md`: not used (the match route replaced it).

## Recipe candidates

1. **Focus-probe read (Slate focus, class-agnostic), for any "is X focused" line.** `execute_unreal_python_readonly`: iterate `unreal.ObjectIterator(unreal.EditableTextBox | unreal.Button)`, filter the path on `GameInstance` + the owning widget name, then read `has_keyboard_focus()` and the owning root's `has_focused_descendants()`. Measured this run: 26 reads, both directions, two widget classes. ⛔ **Fence: never take it after a `ui_perform` without a focus-setting input in between.**
2. **Console focus re-establish:** `IA_AssistantConsole` ×2 with a 0.5 s gap (close, then open) → `True`, 4 of 4.
3. **Detail-page focus re-establish:** `IA_MenuDown` on the one-stop `BackButton` ring → `True`, 3 of 3 read back.
4. **Law candidate (for the manager, not a recipe):** "Any Slate focus reading, including `ui_perform`'s own snapshots, taken after a `ui_perform` with no focus-setting input in between is void." Measured 8 of 8 against a 5 of 5 control.
