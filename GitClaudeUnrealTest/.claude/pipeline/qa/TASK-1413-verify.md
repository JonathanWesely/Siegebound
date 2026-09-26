Verdict: VERIFIED
# Verification — TASK-1413 [WAVE-B-VERIFY]

**Editor/Aura state:** Aura connected `editor_connected` · editor identified **IN-PROCESS**, not by census —
`os.getpid()` returned **44872** from inside the editor that answered the MCP call (matches the dispatch's
PID); `sys.executable` = `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` (the
**GUI editor**, not a `-game` session — it exposes `UnrealEditorSubsystem` and an editor world);
`Paths.get_project_file_path()` = `…/GitClaudeUnrealTest.uproject`. ⚠️ `FCommandLine::Get()` returned the
**empty string**, so the `SC-§118` identification here rests on in-process PID + exe path + editor-world
presence rather than on a command-line string — declared, not glossed.
Map: editor booted on `/Game/Maps/L_Arena` (the build-master declared this and left the level load to me);
I called `load_level /Game/Maps/L_MainMenu` myself — `previous_level: /Game/Maps/L_Arena`,
`discarded_unsaved: false`.
PIE mode: standalone, 1 client, 1280×720 requested → viewport **1280×725**. `is_pie_active` read
**`is_active: false` before I started**, so the session was mine to start and mine to stop; stopped at
`pie_time_seconds: 546.59`, `is_active: false` confirmed after. **Editor PID 44872 left UP.**
Attempts used: **1 of 3.** PIE wall time ≈ 9 min (t=2.7 s → t=546.6 s).
🧑 Jonathan's explicit go for PIE this sitting was carried in the dispatch (*"Go now."*).

---

## 🚨 INSTRUMENT LIVENESS — CHECKED FIRST, BECAUSE AN EMPTY LOG READS AS A ZERO OFFSET

The dispatch instructed `Log LogSiegeMenuInput Log` before injecting. **That console write was not needed
and was not made** (I hold no console-command verb, and `execute_unreal_python_readonly` is read-only):
`SiegeMenuInputSubsystem.h:29` declares `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMenuInput, Log, All)` ⇒ the
category's **default runtime verbosity is already `Log`**, and every instrument line below is
`UE_LOG(LogSiegeMenuInput, Log, …)`.

I did not trust that reasoning either — I **saw the lines**. First three of the session:

```
[2026.09.25-08.26.09:228] LogSiegeMenuInput: […] L_MainMenu: IMC_MainMenu applied at priority 0 on
      'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).
[2026.09.25-08.26.09:228] LogSiegeMenuInput: […] L_MainMenu: IA_MenuLeft / IA_MenuRight / IA_MenuBack
      bound (Started): bound / bound / bound.
[2026.09.25-08.26.09:250] LogSiegeMenuInput: […] ApplyInitialFocus: focus placed on the TOP option
      'Button_0' ("Play (vs Bot)"), index 0 of 7, in menu instance 'WBP_MainMenu_C_0'.
```

⇒ The instrument is **demonstrably alive**, and a silence read later in this run is a **real silence**.
41 `LogSiegeMenuInput` lines were produced across the session.

⛔ **ONE VERBOSITY GAP, DECLARED RATHER THAN HIDDEN:** the *"plain UButton … Left/Right do nothing,
deliberately"* line is `UE_LOG(…, **Verbose**, …)` (`SiegeMenuInputSubsystem.cpp:960-962`, with the
author's own comment *"⚠️ Verbose needs `Log LogSiegeMenuInput Verbose` to appear at all"*). It therefore
**did not print** in this run. My negative control does **not** rest on seeing it — see §4.

Pre-registered falsifiers were written to disk **before PIE started** (while `is_pie_active: false`):
`…\scratchpad\TASK-1413-prereg.md`. The ordering is the point; nothing below was decided after a number
was seen.

---

## §1 — PER-SUBJECT VERDICT TABLE (the commit host reads THIS, not line 1)

| TASK | subject's own criterion (as the row words it) | observable chosen | observed — quoted value + node name + PIE `t=` | verdict |
|---|---|---|---|---|
| **TASK-1406** | **REGRESSION ONLY** — the main menu still walks: `IA_MenuDown` moves `focused: true` as before. *(The second limb is STRUCK on the row and lives on TASK-1415; I did not test it and did not substitute a proxy.)* | ➊ the `TASK-1394` (b3) instrument line, walked a full ring, **STOP NAMES enumerated — not a count** (row (1b)); ➋ `ui_snapshot` `focused:` on `WBP_MainMenu_C` | **7 of 7 named, ring closed.** `Button_0`(t=22.68 s)→`Button_1`→`Button_2`→`Button_3`→`Button_4`→`Button_5`→`Button_6`→wrap `6 -> 0` (t=23.98 s). `ui_snapshot` at t=22.66 s: `Button_0` `focused: true`; at t=24.30 s after the 7-press wrap: `Button_0` `focused: true` again. Every line reads `of 7`. | **VERIFIED** |
| **TASK-1408** | `get_input_mapping_context_keys` on `IMC_MainMenu` reads back **12** mappings incl. the three new actions **by name** | the asset read itself (no PIE needed — taken first) | `mapping_count: **12**`; `IA_MenuLeft` ← `Left` / `Gamepad_DPad_Left`, `IA_MenuRight` ← `Right` / `Gamepad_DPad_Right`, `IA_MenuBack` ← `Backspace` / `Gamepad_FaceButton_Right`; original six (`IA_MenuUp`, `IA_MenuDown`, `IA_MenuAccept` × 2 keys each) present and **first in order**. Corroborated at runtime: *"IA_MenuLeft / IA_MenuRight / IA_MenuBack bound (Started): **bound / bound / bound**"* (t≈0, 08:26:09). | **VERIFIED** |
| **TASK-1409** | `IA_MenuRight` **moves a focused `USlider`'s value** — read at the **FACADE**, per `qa/TASK-1410.md` BLOCKER-1 (*a node read PASSED on the broken cut*); node read is corroboration only | live `USiegeGraphicsSettingsSubsystem::GetQualityGroupLevel` / `GetResolutionScalePercent` read off the **PIE** game instance | Focus on `ShadingQualitySlider` (stop 12 of 24, t≈250.8 s). **ONE** `IA_MenuRight` at **t=271.71 s**. ⭐ **FACADE `ShadingQuality` 2 → 3**; all nine other groups **unchanged at 2**; `ResolutionScalePercent` **unchanged at 87.0000**. Node read (corroboration): `ShadingQualitySlider.Value` **0.5 → 0.75** (t=271.70 s → t=272.54 s). Commit edge fired: *"slider 'ShadingQualitySlider' → **OnControllerCaptureEnd.Broadcast()**"*. Third, independent corroboration **on pixels**: the Graphics row label reads **"Epic"** for Shading while all nine others read **"High"**, and Overall Quality reads **"Custom"** (capture, t=321.65 s). | **VERIFIED** |

**Row-level line 1 = `VERIFIED`**: every acceptance line with a runtime signal was observed passing.

---

## §2 — TASK-1406 (1b): THE STOP **NAMES**, NOT THE COUNT

The row rewrote this criterion because *"a reading of 8 cannot discriminate a widening defect from a
pre-existing collection"* (`qa/TASK-1407.md` WARN-3 — `Btn_Jump` is a `UButton` in the authored tree).
**Enumerated by name, default nav target `WBP_MainMenu_C_0`:**

| idx | stop name | label | class |
|---|---|---|---|
| 0 | `Button_0` | "Play (vs Bot)" | Button |
| 1 | `Button_1` | "Sandbox (No Bot)" | Button |
| 2 | `Button_2` | "Deck Builder" | Button |
| 3 | `Button_3` | "Multiplayer" | Button |
| 4 | `Button_4` | "Settings" | Button |
| 5 | `Button_5` | "Login" | Button |
| 6 | `Button_6` | "Quit" | Button |

⭐ **`Btn_Jump` IS NOT A STOP.** It is present in the live tree (`ui_snapshot` t=22.66 s:
`Overlay_19/SizeBox_0/Btn_Jump`, class `Button`, `visibility: Collapsed`, `realized: false`) and the
walker **did not admit it**. The widening added **nothing** to the default target. The two nested
`UI_Thumbstick_C` user widgets (also Collapsed) likewise contributed no stop.
⇒ The row's rewritten falsifier — *a name outside the live `Button_0..Button_6` set* — **did not fire**.

---

## §3 — THE TWO SCREENS REACHED, AND WHY THEY ARE **CORROBORATION**, NOT MY CRITERIA

`TASK-1406`'s registered-target limb is **STRUCK on my row and lives on `TASK-1415`**. I record what the
registration printed because I had to pass through it to reach a slider — **I am not scoring it, and I did
NOT call `RegisterMenuNavTarget` myself** (the `TASK-1407` WARN-7 / `SC-§137` trap: a synthetic driver
earns `MEASURED` at best). Every registration below was performed **by the product**, from an
`IA_MenuAccept` on a focused button.

```
t≈54.5 s  IA_MenuAccept -> OnClicked.Broadcast() on 'Button_4' ("Settings").
t≈54.5 s  menu nav target registered -> 'SettingsMenuWidget_0' (registered screen),
          3 focus stop(s), 1 screen(s) registered.
t≈218.5 s IA_MenuAccept -> OnClicked.Broadcast() on 'GraphicsButton' ("Graphics").
t≈218.5 s menu nav target registered -> 'SiegeGraphicsMenuWidget_0' (registered screen),
          24 focus stop(s), 2 screen(s) registered.
```

**Settings — 3 of 3 stops, by name** (t≈76.9–77.3 s): `0 ConfirmToggleCheckBox` (**CheckBox** — the
widening's new class, reached by the product) · `1 GraphicsButton` · `2 BackButton`.

**Graphics — all 24 stops, by name** (Down walk t≈249.3–250.8 s; Up walk t≈522.1–524.9 s):

| idx | name | idx | name | idx | name |
|---|---|---|---|---|---|
| 0 | `ShowFrameRateCounterCheckBox` | 8 | `PostProcessQualitySlider` | 16 | `WindowModePrevButton` |
| 1 | `AutoDetectButton` | 9 | `TextureQualitySlider` | 17 | `WindowModeNextButton` |
| 2 | `OverallQualitySlider` | 10 | `EffectsQualitySlider` | **18** | **`KeepSettingsButton`** |
| 3 | `ViewDistanceQualitySlider` | 11 | `FoliageQualitySlider` | **19** | **`RevertSettingsButton`** |
| 4 | `AntiAliasingQualitySlider` | 12 | `ShadingQualitySlider` | 20 | `VSyncCheckBox` |
| 5 | `ShadowQualitySlider` | 13 | `ResolutionScaleSlider` | 21 | `FrameRateLimitPrevButton` |
| 6 | `GlobalIlluminationQualitySlider` | 14 | `ScreenResolutionPrevButton` | 22 | `FrameRateLimitNextButton` |
| 7 | `ReflectionQualitySlider` | 15 | `ScreenResolutionNextButton` | 23 | `BackButton` |

---

## §4 — THE NEGATIVE CONTROL (`SC-§137`) — **IT DISCRIMINATED**

**Declared before it was run** (see the pre-registration file): the same verb, the same lane, the same
instrument, on a stop that must *not* move the facade.

| t (PIE) | focused stop | action | instrument | facade `ShadingQuality` / `ResolutionScalePercent` |
|---|---|---|---|---|
| 176.78 | `GraphicsButton` (plain `UButton`) | — | `MoveFocus(+1): focus moved 0 -> 1 of 3 ('GraphicsButton')` | 2 / 87.0000 |
| **177.10** | `GraphicsButton` | **`IA_MenuRight`** | **`StepFocusedStop(+1) entered.`** and **no further `Log`-level line** | **2 / 87.0000 — UNCHANGED** |
| 271.71 | `ShadingQualitySlider` (`USlider`) | **`IA_MenuRight`** | `slider 'ShadingQualitySlider' 0.5000 -> 0.7500 (step 0.2500 …); read back 0.7500` + `OnControllerCaptureEnd.Broadcast()` | **3 / 87.0000 — MOVED** |

**Why the control's silence is a reading and not a blind instrument.** The `entered` line
(`…cpp:892-893`, `Log`) **printed**, so the action reached `StepFocusedStop`. From there every branch
except one logs at `Log` verbosity, and **none of them appeared**: `declined: nav target not actionable`
(`:899-900`), `declined: no focus stop holds focus` (`:912-913`), the slider lines (`:1028`, `:1076`,
`:1090`, `:1101`, `:1118`), the check-box lines (`:1140`, `:1158`), and `has no Left/Right semantics —
nothing done` (`:971-972`). The **only** branch that is silent at default verbosity is the
plain-`UButton` one at `:960` (`Verbose`). ⇒ By elimination the control took exactly the intended branch,
did nothing, and **left the facade where it was** — while the identical verb on a slider moved it.
**The control discriminated.**

⭐ **It also answers `qa/TASK-1410.md` WARN-8 for the injection lane:** one press produced **one** detent
(2 → 3), not two. ⚠️ WARN-8 was conditioned on a **pad Accept lock** (`SSlider::bControllerInputCaptured`);
I drove Enhanced Input, not a physical pad, so WARN-8's own scenario is **untested** — see limitations.

---

## §5 — THE `TASK-1419` SOURCE-READ PREDICTION: **TESTED FOR REACHABILITY, AND NOT REACHABLE IN THIS BINARY**

The dispatch carried a source-read prediction (`SEditableTextBox::OnFocusReceived` forwards focus to its
inner `SEditableText` while `UWidget::HasUserFocus` is exact-widget ⇒ a ring-wearing `UEditableTextBox`
reports `HasUserFocus() == false` ⇒ `MoveFocus` takes the cold `Current = 0` branch) and told me to
**verify the reachability claim rather than inherit it.** I verified it:

- **Main menu** — full `ui_snapshot` (29 nodes, t=22.66 s): classes present are `Overlay`, `SizeBox`,
  `Button`, `TextBlock`, `VerticalBox`, `Border`, `Image`, `UI_Thumbstick_C`. **Zero `EditableTextBox`.**
- **Settings** — whole-tree class search (t=476.94 s) returned *"No widget in root 'SettingsMenuWidget'
  matches by=class value='EditableTextBox'"*, with the complete 10-node name list.
  **Zero `EditableTextBox`.**
- **Graphics** — same search (t=476.75 s): *"No widget in root 'SiegeGraphicsMenuWidget' matches
  by=class value='EditableTextBox'"*. **Zero `EditableTextBox`.**
- The account screen (`TASK-1419`) is **not in this binary** and was not touched.

⇒ **NOT REACHABLE IN THIS BINARY. This is not a pass**, and the prediction is neither confirmed nor
refuted by this run. It stays live for the first row that lands a registered screen with a text-box stop.

---

## §6 — 🚨 A FINDING THIS RUN MEASURED THAT IS **NOT** ONE OF MY ACCEPTANCE LINES

**The Graphics screen's focus ring is SEVERED at a two-stop dead zone: `KeepSettingsButton` (18) and
`RevertSettingsButton` (19) can never take focus, and neither direction can cross them.**

Measured, both directions, at a **fixed** screen with nothing else changing:

```
Down, six consecutive presses, t≈379.2 → 380.7 s  (six identical lines, not a copy-paste):
    MoveFocus(+1): focus moved 17 -> 18 of 24 ('KeepSettingsButton').   ×6
Up,   two consecutive presses,  t≈524.7 → 524.8 s:
    MoveFocus(-1): focus moved 20 -> 19 of 24 ('RevertSettingsButton'). ×2
```

`MoveFocus` reads the CURRENT index from Slate each press (`…cpp:790-794`). It keeps reporting
`17 -> 18` because after `FocusWidget(Stops[18])` the focus is **still** on 17 ⇒ stop 18 never took it.

**The obvious explanation is refuted by measurement.** `KeepSettingsButton` reports
`Visibility: **Visible**`, `bIsEnabled: **true**`, `IsFocusable: **true**` (t=415.40 s) — so it is *not*
opted out and *not* self-collapsed. What it actually is: `ui_snapshot` on that node (t=429.33 s) reports
**`realized: false`** — it has **no live Slate widget** (its confirm bar is collapsed while no video-mode
change is pending; it is absent from the capture at t=321.65 s). `FSlateApplication::SetUserFocus` cannot
focus a widget that was never constructed.

**Consequence, stated as reach and not as blame:** Down from stop 0 reaches 1…17 and then stops forever;
Up from stop 0 wraps to 23 and reaches 22, 21, 20, and then stops forever. Every other stop is reachable
**by choosing a direction**, but continuous travel in one direction is blocked, and the row's own Back
button (`BackButton`, stop 23) is unreachable by pressing **Down** from the top of the screen.

⚖️ **THIS IS A HYPOTHESIS ABOUT MECHANISM AND A MEASUREMENT ABOUT BEHAVIOUR — AND IT IS NOT A VERDICT ON
ANY OF MY THREE SUBJECTS.** `TASK-1406`'s criterion on this row is the **main-menu regression only**, and
the main menu walked its full ring cleanly. The stop-admission rule lives in `GetMenuFocusStops`
(`TASK-1406`) and the screen is `TASK-1417`'s. **Routed to the manager**, not self-adjudicated
(`SC-§50`, `SC-§101`). *Candidate mechanism, unproven:* the walker admits by class + `IsFocusable` and
does not test `TakeWidget()`/realization or **effective** (parent-inherited) visibility.

---

## §7 — THE PIXEL LIMB (`TWO-LIMB-CONDITIONED-BY-VER-11-CL-9-2026-09-24`)

**STATE limb and PIXEL limb are recorded side by side and are NOT merged.**

- 🤖 **STATE limb — the gate — PASSED** on all three subjects. See §1.
- 🧑 **PIXEL limb — `UNOBSERVABLE`. THE FOUR-PART BAR IS NOT SPECCED ON THIS ROW.** Clause 4 of the
  conditioning sub-block is explicit that the **row or its dispatch** specs the four parts and that a
  verifier **may not decide at the keyboard that its capture qualifies**; my row's own header says this
  row *"does NOT spec the four-part bar"*. So: no ring score was computed, no floor was pre-registered
  for pixels, and **the absence of a visible ring in my capture is NOT written as a `VERIFY-FAILED`**
  (cl. 9(b)). Saying the bar was not specced is the deliverable, not a shortfall.
  ⚠️ For the record and for whichever row buys the capability: my capture **did** satisfy part (i) by
  accident — requested `max_dim: 1920` against a 1280-px long edge, returned **1280×725 at 1:1, scale
  factor 1.000** — but parts (ii)–(iv) were neither specced nor performed, so the bar is **not** met.
- 🧑 **(2b) *"does it look right?"* remains Jonathan's, forever.** The promoted frame is offered for that
  and for nothing else.

---

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1413-t04m33s-graphics-shadingqualityslider-stepped-by-ia-menuright.png`
  — 1280×725 composited PIE frame, `mean_luma 13.8`, `pct_near_black 0.9004`: the **Graphics** screen on a
  black backdrop, "60 FPS · 16.7 ms" under the title; eleven quality rows each with a slider and a value
  label — **ten read "High" and the "Shading" row reads "Epic"**, with "Overall Quality" reading
  "Custom"; "Resolution Scale" reads **87%**; below them Screen Resolution `1024 x 768`, Window Mode
  `Borderless Window`, Frame Rate Limit `Unlimited`, and a "Back" button. **No Keep/Revert confirm bar is
  drawn** — the pixel counterpart of §6's `realized: false`. The dimmed Settings screen shows through
  behind it.
  🚨 **THE FILENAME'S TIME STAMP IS WRONG AND I CANNOT FIX IT.** The frame is **PIE `t = 321.654 s`
  (= 05m21s)**, not `04m33s`; I chose the name before the capture returned its stamp. I hold **no tool
  that moves or renames bytes** (`VER-§7` cl. 4), so the rename is **owed to the commit host
  (`TASK-1414`)** — the correct name is
  `VER-TASK-1413-t05m21s-graphics-shadingqualityslider-stepped-by-ia-menuright.png`. Recording the wrong
  stamp here rather than letting a mis-named file pass as correct.

Non-promoted, cited above and left where they are (gitignored):
`Saved/Logs/GitClaudeUnrealTest.log` (all 41 `LogSiegeMenuInput` lines of this session, 08:26:09 →
08:34:54) · the auto-armed background recording at
`Saved/AuraVerify/rec_1790324768727394900_39` (never collected — every reading in this report is a log
line, a node read or a facade read, not a frame).
Pre-registration (outside the repo, by design): `…\scratchpad\TASK-1413-prereg.md`.

---

## Hypotheses (NOT verdicts)

1. **§6's mechanism:** the confirm-bar buttons are unfocusable because their parent bar is collapsed, so
   they have no Slate widget (`realized: false`) while their own `Visibility`/`bIsEnabled`/`IsFocusable`
   all read true. **Measured:** the stall, and `realized: false`. **Hypothesis:** that the stop-admission
   rule should test realization / effective visibility. Not my call to prescribe (`SC-§101`).
2. The three `IA_Menu{Left,Right,Back}` assets resolving at runtime (*"bound / bound / bound"*) suggests
   `TASK-1408`'s asset work and `TASK-1409`'s binding work are consistent end-to-end; I verified the
   **mapping context** and **one** Right press, not Left, not Back, not the other 21 Graphics stops.

---

## `VER-§7` cl. 2 DECLARATION — every name reached THROUGH the runner

Inside `run_verification_sequence` I used exactly four step types: **`ui_snapshot`**,
**`inject_input_action`**, **`wait_pie_seconds`**, **`get_widget_property_in_pie`**. All four are
**also standalone tools on my own `tools:` line** — no step reached a name I do not hold directly.
⛔ **I invoked NO census §5 PIE-world-mutation name: no `pie_scene_edit` (and therefore no
`spawn_actor` / `delete_actor` / `set_actor_property` / `set_actor_transform` / `set_actor_enabled` /
`teleport_player`), no `call_actor_function`, no `set_player_transform`, no `start_state_recording`.**

**`execute_unreal_python_readonly` — every call was a read**, enumerated so it can be audited:
`os.getpid()` · `SystemLibrary.get_command_line()` · `EditorLevelLibrary.get_editor_world()` ·
`hasattr()` reflection on `unreal.SiegeMenuInputSubsystem` / `unreal.SiegeGraphicsSettingsSubsystem` ·
`get_editor_subsystem(UnrealEditorSubsystem)` + `get_game_world()` ·
`GameplayStatics.get_game_instance()` · `unreal.find_object()` ·
`SiegeGraphicsSettingsSubsystem.get_quality_group_names()` (static) ·
the two `BlueprintPure` getters `get_quality_group_level()` / `get_resolution_scale_percent()`.
**No property was written, no asset touched, no editor state mutated, nothing saved.**

Other granted verbs used: `load_level` (the build-master explicitly left the level load to this row),
`start_pie` / `stop_pie` **on the session I started myself**, `is_pie_active`, `capture_pie_frame`,
`attach_pie_frames`, `get_input_mapping_context_keys`, `get_unreal_output_logs`, `ui_snapshot`,
`get_widget_property_in_pie`, `mcp__unreal_inspector__grep`, `Read`/`Grep`/`Write`/`Edit`.
⛔ No code edit, no asset write, no compile, no git, no editor-lifecycle action. **PID 44872 left UP.**

---

## Not examined / limitations this run

1. ⛔ **THE THREE UNCOMPILED SCREENS WERE NOT TESTED AND THEIR ABSENCE IS NOT A DEFECT:** `TASK-1419`
   (`AccountMenuWidget`) · `TASK-1423` (`DeckBuilderWidget`) · `TASK-1425` (`SessionMenuWidget`) were
   written to disk **after** the 23:00:59 link and are **not in PID 44872's binary**. I never opened the
   Login, Deck Builder or Multiplayer screens. Those are `TASK-1421`/`TASK-1427`, after the next compile.
2. ⛔ **`TASK-1406`'s SECOND LIMB WAS NOT TESTED** — it is struck on this row and lives on `TASK-1415`.
   The registration readings in §3 are corroboration I passed through, not a scored criterion, and
   **I did not call `RegisterMenuNavTarget` myself** (`SC-§137` / `TASK-1407` WARN-7).
3. ⛔ **PIXEL LIMB `UNOBSERVABLE` — four-part bar not specced on this row.** See §7.
4. ⚠️ **THE BUDGET WAS OVERRUN AND I AM SAYING SO.** The row required every observable before `t≈60 s`.
   `TASK-1408` landed before PIE existed and **all of `TASK-1406`'s readings landed by t=24.30 s**, but
   `TASK-1409` needs a focused `USlider`, which exists only behind Main menu → Settings → Graphics; that
   chain's own registration and open times put the criterion at **t=271.71 s**. It cannot be compressed
   by batching — it is three screens deep. Front-loaded and batched as far as the route allows.
5. ⚠️ **ONE SLIDER, ONE DIRECTION, ONE PRESS.** I stepped `ShadingQualitySlider` **+1** once. I did **not**
   test `IA_MenuLeft`, did not test the other ten sliders, did not test a range bound (the
   *"did not move … no commit edge fired"* branch), and did not test `IA_MenuBack` at all (it is inert
   everywhere — no `ISiegeMenuNavCloseTarget` implementer — which is `TASK-1454`'s).
6. ⚠️ **`qa/TASK-1410.md` WARN-8 IS STILL OPEN.** One injected press produced exactly one detent, but
   WARN-8's scenario is a **physical pad Accept lock** driving `SSlider::OnNavigation` in parallel. I have
   no pad. **Untested, not cleared.**
7. ⚠️ **A SETTING WAS CHANGED BY THE FEATURE UNDER TEST AND MAY HAVE PERSISTED.** `ShadingQuality` went
   2 → 3 through the product's real commit edge, which is wired to the screen's facade writer and its
   save path. If that reached `Saved/Config/…/GameUserSettings.ini`, it is a machine-local, gitignored
   file — but it is a **real state change**, not a sandbox, and I hold no tool to revert it and must not
   edit it. Declared so nobody discovers it as a mystery.
8. ⚠️ **`SC-§118` IDENTIFICATION IS PARTIAL** — `FCommandLine::Get()` returned empty, so the editor was
   identified by in-process PID + exe path + editor-world presence. No `-game` session of Jonathan's was
   present (`get_game_world()` was `None` before I started PIE) and **nothing was killed, closed or
   relaunched by me.**
9. ⚠️ **`Construct` GRAPHS WERE NOT READ.** The class censuses in §5 are of the **live** widget trees at
   the instants stated, which is stronger than the authored-tree read `qa/TASK-1407.md` could take — but
   a widget created later by a Blueprint graph would not appear in them.
10. ⛔ **NOT A COMMIT.** Nothing was staged or committed; 🧑 Jonathan commits this project himself. The
    evidence-file rename in the Evidence section is **owed to `TASK-1414`**.
