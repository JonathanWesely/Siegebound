Verdict: MEASURED
# Verification — PLAYTEST-archer50 (ad-hoc, Jonathan-directed exploratory playtest)

## THE ANSWER FIRST

**Deck slot 4 now holds exactly 50× `Archer`. It was built through the real Deck Builder UI, click by click, and the save on disk confirms it.** The run then **stopped at the next sub-step, "make deck 4 the deck the match will use"**, because that step has **no route on this agent's toolset**. The only writer of `ActiveDeckName` is a **right-click** on the deck-bar slot. That gesture is mouse-only by Jonathan's DECK-§3 ruling, and `ui_perform` has **no right mouse button**: two attempts were made, and `ActiveDeckName` read `deck1` on disk after both. Per the dispatch ("stop at that step … never fake or proxy a later step"), **no match was started, and A2–A4 were NOT reached.** A match on `deck1` (which holds 10 Archers) would have proxied A2. I did not run one.

⭐ **A new finding the manager needs, reported rather than ruled:** `ui_perform`'s **`double_click` step FIRES `UButton.OnClicked`**. The same button under `click`, under `press`/`release`, and under a click aimed at the button body rather than its label did **nothing**, which reproduces `TASK-1390`'s signature (`down handled=true / up handled=false`). `double_click` adds a second up that comes back `handled: true`, and the handler runs **exactly once per gesture** (calibrated: Ogre 50 → 49 on one gesture). This is how all 100 deck edits were made. It narrows the pointer ceiling recorded by `VER-§5` cl. 5 / `TASK-1390`. **Any amendment belongs to the manager (`SC-§101`), and I wrote none.**

**Why line 1 reads `MEASURED`:** there is no code under test and no board row. A1 was observed passing, with a control that discriminated (single-click vs double-click on the same button, same state reader). A2–A4 were not reached because of a rig ceiling, not a game defect, so **nothing was observed failing** and `VERIFY-FAILED` would be false. `VERIFIED` would also be false, since three lines with a runtime signal were never observed. The run DID observe things, so this is not `UNOBSERVABLE` in the "no runtime signal" sense.

Editor/Aura state: Aura connected **y** (`get_headless_status` → `editor_connected`). Editor **PID 19396**. The orchestrator identified it by **command line** before dispatch (`UnrealEditor.exe "…\GitClaudeUnrealTest.uproject"`, GUI editor, no `-game` instance). I corroborated it from inside per `SC-§118` cl. 9: `os.getpid() = 19396` both before and after, and `LogInit: Command Line:` is empty (project argument only). Pre-flight `is_pie_active` → **`is_active: false`**, so no session of Jonathan's existed and nothing I did not start was stopped. Dirty packages **`[]` before and after**. Map: received on `L_Arena`. `load_level /Game/Maps/L_MainMenu` returned `discarded_unsaved: false`. **Restored to `L_Arena` at the end** (`discarded_unsaved: false`). PIE: standalone, 1 client, 1280×720 requested / viewport 1280×725, DPI 0.6706. **Attempts used: 1 of 3**, one PIE session from t=0 to t=630.45 s (`stop_pie` issued by me at t=630.45; `is_pie_active` → `false` afterwards). Wall time is about 25 min including pre-flight. Credit: none surfaced in any tool reply. Jonathan was present; his request was the go (dispatch + `VER-§3` cl. 6).

**Recording:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\AuraVerify\PLAYTEST-archer50-a1\recording.h264`. It is a **raw H.264 elementary stream, NOT an MP4**: `capture_mode: nvenc`, `container: h264`, 17,872 frames, 30 fps, 1280×720, `duration_seconds: 595.73`, `dropped_stages: 0`, **83,235,128 bytes** (read with `os.path.getsize` in the editor process). Timing map: `…\PLAYTEST-archer50-a1\recording_index.json` (frame ↔ game time ↔ video time). The tool says raw streams are "remuxed to MP4 out-of-band"; **no MP4 appeared in that folder**, and the remux is owed by someone with a shell, e.g. `ffmpeg -framerate 30 -i recording.h264 -c copy recording.mp4`. VLC plays the `.h264` as-is. The recording was armed before `start_pie` (`arm_wait_for_pie`), so it starts at game t=0.40 s.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| A1 | Deck slot 4 contains exactly 50× `Archer` and no other card | (i) `WBP_DeckBuilder_C_0.WorkingDeck` + `EditingDeckIndex` (live widget read); (ii) the on-disk save, read with `GameplayStatics.load_game_from_slot("SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34", 0)` after PIE ended; (iii) the auto-save log line; (iv) a frame | (i) **t=445.25 s:** `EditingDeckIndex = 3`, `WorkingDeck = (DeckName="deck4",Cards=((CardID="Archer",Count=50)))`. (ii) **after PIE:** `DECK deck4 : [{card_id: "Archer", count: 50}]`. (iii) `[2026.09.26-21.49.57:988][640]LogGitClaudeUnrealTest: UDeckBuilderWidget: saved deck 'deck4' (50 cards) to slot 'SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34'.`, the last of a monotonic run `(39 cards)` → `(50 cards)`. (iv) `…/2026-09-26/VER-PLAYTEST-archer50-deck4-builder-50-archers_t453.67s_f61435.png`: the 2nd tile (Archer) shows **50** and every other tile **0**. Starting point, read before the run: `deck4 : [{card_id: "Ogre", count: 50}]` | **pass** |
| A2 | The vs-bot match used deck 4: in-match hand/draw pile all `Archer` | `ActiveDeckName` (the only thing `ASiegePlayerController` reads at match start, `SiegePlayerController.cpp:319`), then the in-match `DeckComponent.Hand`/`DrawPile` | **NOT REACHED.** `ActiveDeckName` read **`deck1`** on disk after both right-click attempts (§1c), so a match would have loaded `deck1`. No match was started (see *THE ANSWER*) | **unobs (blocked by rig ceiling)** |
| A3 | `CaptureZone_Center.CaptureOwner` read `Blue` before the summon | the zone's `CaptureOwner` | **NOT REACHED.** Pre-read from the editor world only: `CaptureZone_Center` (`CaptureZone_0`) at `Location=(0)`, no instance modifications ⇒ `ZoneHalfExtent` default **(840, 840)**, i.e. box X∈[−840, 840], Y∈[−840, 840] (`CaptureZone.h:190`). No runtime read | **unobs (blocked)** |
| A4 | Blue `Archer` spawned via hand-play inside the zone, gold −cost, card leaves hand, with a control arm | gold / hand / census | **NOT REACHED.** `pie_scene_edit`/`call_actor_function` was invoked **0 times** in this run | **unobs (blocked)** |

## §1 — What was driven, and how (every route, including the ones that did nothing)

**(a) Main menu → Deck Builder: menu-nav lane (inject_input_action).** At t=4.84 s `Button_0` "Play (vs Bot)" was focused. `IA_MenuDown` ×2 (t=16.55, 16.97) put focus on **`Button_2` "Deck Builder" (`focused: true`, t=17.38)**. `IA_MenuAccept` (t=21.50) opened the builder, and at t=22.53 it read `EditingDeckIndex = 0`, `WorkingDeck = deck1` (Jonathan's deck, untouched throughout).

**(b) Selecting deck4 for editing: the pointer lane, with the control that makes it mean something.** All shapes targeted deck4's `SlotButton` (abs 1674.9, 870.9, 121×31). The effect reader is `EditingDeckIndex`.

| shape | PIE t | hit | pointer trace | `EditingDeckIndex` after |
|---|---|---|---|---|
| `click`, selector `{by:name, value:"SlotButton", match_index:3}` | 43.63 | — | step error: *"No widget in root 'WBP_DeckBuilder' matches by=name value='SlotButton'"* (nested names are not indexed) | 0 (t=48.68) |
| `click` on `DeckSlotEntryWidget_3` centre | 57.13 | `STextBlock` | down `handled:true` / up `handled:false` | **0** (t=61.06) |
| `simulate_key_press "Tab"` (Slate-route probe) | 84.78 | — | *"Key Tab was delivered to the player controller, but NOTHING BINDS IT: applied mapping context(s) IMC_MainMenu…"*, `binding_found:false` | **0**; no `SlotButton` `focused`; `FocusedCardIndex -1` (t=85.11) |
| `click`, offset dx −45 (button body, not label) | 201.91 | **`SButton`** | down `handled:true` / up `handled:false` | **0** (t=205.73) |
| **`double_click`**, offset dx −45 | 210.96 | `SButton` | down `true` / up `false` / **double_click `true` / up `true`** | **3**, `WorkingDeck = (DeckName="deck4",Cards=((CardID="Ogre",Count=50)))` (t=215.33) |

⇒ **The instrument discriminated.** Every shape shared the same button, state and reader; only `double_click` moved it. `simulate_key_press` is **not** a Slate lane: it is delivered to the player controller, which on `L_MainMenu` reaches nothing.

**(c) Removing the Ogres / adding the Archers: `double_click` on the tile's "−" / "+" `UButton`s.** Both are bound to `OnClicked` (the tile `.uasset` holds one `OnPressed`/`OnReleased` pair, which belongs to the touch `Btn_Jump` events). Targets: `WBP_DeckCardTile_C_12` (Ogre, count text "50") "−" at abs (2056.9, 1186.6) 28×20, and `WBP_DeckCardTile_C_1` (Archer) "+" at abs (1380.9, 1186.6) 29×20. Each was addressed by **plain name + an offset** onto the button body.
- Calibration, 1 gesture: Ogre **50 → 49** (t=304.53) ⇒ **one gesture = one `OnClicked`**.
- 49 gestures, 3-frame spacing: 49 → **7** (t=356.46), so only 42 landed. 10 more at 30-frame spacing: deck emptied, `WorkingDeck = (DeckName="deck4")` (t=375.48). Extra "−" at 0 is a documented no-op.
- 30 "+" gestures, 20-frame spacing: Archer **27** (t=412.87). 28 more at 22-frame spacing: Archer **50** (t=445.25). The "+" greys at 50 (Ogre's "+" read `enabled: false` at 50), which made overshoot impossible. The tile's count text and the save agree.

**(d) Make deck4 ACTIVE: right-click on the slot. BLOCKED.** Source: the right-click is caught in `UDeckSlotEntryWidget::NativeOnMouseButtonDown` on `EKeys::RightMouseButton` → `SetActiveDeckBySlot`. That path and save migration (`SiegeDeckSaveGame.cpp:205`) are the only writers of `ActiveDeckName`, and DECK-§3 keeps the gesture mouse-only by ruling. Attempts:
1. `click` + `"button":"right"` (t=463.77): trace identical to a left click (`hit_widget: SButton`, down `true` / up `false`). The key was **silently ignored**.
2. `press`/`release` + `"button":"RightMouseButton"`, `"mouse_button":"right"`: again a left press on `SButton`.
After both, `ActiveDeckName` read **`deck1`** on disk. Log substring `SetActiveDeck` → **0 lines**, and the same reader returns 100 lines for `deck4` ⇒ **a ZERO, not a void.** The Aura plugin ships binaries only (`Engine/Plugins/Marketplace/Aura`), so the step schema could not be read from source. **⇒ Rig ceiling: `ui_perform` has no right button this run could find.**

**(e) Leaving the UI clean.** A `double_click` on the builder's **Exit** text (resolved first with `ui_snapshot` `{by:text, value:"Exit"}` → `TextBlock_10`, t=586.97) returned to the main menu. Frame t=595.35 (see Evidence). Then `stop_pie_recording` and `stop_pie`.

## Evidence (promoted)

Both frames were written **directly** into the promoted folder. `capture_pie_frame` appends its own `_t…s_f…` stamp, so **no copy-out is owed.**

- `.claude/pipeline/playtest-evidence/2026-09-26/VER-PLAYTEST-archer50-deck4-builder-50-archers_t453.67s_f61435.png` — composited, 1280×725, `mean_luma 151`. The "Deck Builder" screen over sky. Top deck bar `deck1`…`deck10`: **`deck4` carries the editing fill tint**, and **`deck1` still carries the active outline** (visible corroboration that the active deck never moved). The card grid has **"50" on the second tile (Archer)** and "0" on every other tile. The bottom-left counter reads "Deck: 50/50" (small text; the model reads above are authoritative). Right: a details panel for **Fireball**, opened by the mis-aimed gesture (§ *Not examined*). Bottom: "Reset to Default", "Play", "Exit".
- `.claude/pipeline/playtest-evidence/2026-09-26/VER-PLAYTEST-archer50-builder-exit-main-menu_t595.35s_f69928.png` — composited, 1086×615, `mean_luma 194`. **Positive control:** the seven-button main menu (Play (vs Bot) · Sandbox (No Bot) · Deck Builder · Multiplayer · Settings · Login · Quit) over sky and a white plain. There is **no deck bar, no card grid and no details panel**, so it is visibly a different screen from the frame above, which rules out a stale or cached image.

Both were viewed by `Read` of the absolute path (STEP 6); neither claim rests on `add_to_context`.

## Hypotheses (not verdicts)

- **H1 — why `double_click` works where `click` does not.** In every shape the first synthetic up comes back `handled:false`. Only the up that follows the synthetic `double_click` phase comes back `handled:true`. Leading suspect: the first up is not routed to the `SButton` that captured on down, while the double-click path re-presses and its up does reach the captor. **MECHANISM NOT MEASURED.**
- **H2 — lost gestures in rapid batches.** At 3-frame spacing 42/49 landed; at 20-frame spacing 27/30. Suspect: consecutive gestures inside the OS double-click interval are coalesced. **Not measured**, and spacing is only a mitigation (it still lost 3/30).
- **H3 — `ui_perform` probably has no mouse-button field at all**, rather than a differently spelled one; unknown keys are accepted and ignored. Not proven: the schema text delivered to me is truncated and the plugin source is absent.

## Not examined / limitations this run

- **A2, A3, A4 were never reached** (above). None of the in-match routes (`IA_MenuAccept` on "Play (vs Bot)", `IA_Move` toward the zone, `IA_CardN` + `SetMouseLocation` + `LeftMouseButton`) was exercised this run. Prior reports (`TASK-1421`, `TASK-1436`, `TASK-1391`) measured them separately; I assert nothing about them here.
- **`VER-§7` cl. 2 declaration:** `pie_scene_edit` was invoked **0 times**, and `call_actor_function` / `SetMouseLocation` **0 times**. No `spawn_actor`/`delete_actor`/`set_actor_*`/`teleport_player`.
- **Not tried:** `SetMouseLocation` + `simulate_key_press "RightMouseButton"` on the menu. Reason: the Tab probe measured that `simulate_key_press` is delivered to the player controller, not to Slate, and the right-click handler is a Slate `NativeOnMouseButtonDown`. That is an inference from one measured key, not a measurement of RMB.
- ⚠️ **SELECTOR HAZARD, measured:** a `ui_perform` selector `{name_path: "WBP_DeckCardTile_C_12/…/Button_1"}` did **not** error. It **silently resolved to the ROOT** and delivered the gesture at screen centre (1919.5, 1230). That hit a card-face button and opened the details panel for **`Fireball`** (`SelectedDetailCardID = Fireball`, t=274.09). **No deck count changed** (Ogre 50 before and after, t=260.79). From then on I resolved targets by plain name and verified each one with `ui_snapshot` first. A bad `by:name` selector, by contrast, errors cleanly.
- **Frame geometry ≠ `ui_snapshot` geometry.** In the captured frame, tiles and the deck bar sit at a different scale and position than the `abs_*` coordinates imply, yet every gesture aimed by `abs_*` hit its intended button (the counts prove it). Declared, not diagnosed.
- After Exit, `ui_snapshot "WBP_MainMenu"` returned *"matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C"* (the Back handler creates a new menu instance). Observed, not diagnosed.
- **`.sav` integrity:** not hashed by me (no shell). The orchestrator holds the pre-run hashes and will re-hash. What I measured by read: `deck1`, `deck2` and `deck3` are **identical** to the pre-run read, `deck5`–`deck10` are empty as before, `deck4` changed Ogre×50 → Archer×50 (the authorized outcome), and `ActiveDeckName` is **`deck1`, unchanged**. `SiegeSettings_*`, `SiegeAccounts`, and the legacy `.sav` files were not read.
- `binding_found` was reported once (`false`, Tab) and is not an observable here (`VER-§8` cl. 10).
- The Aura plugin version was not read this run.
- Shell / `subprocess` / `ctypes` through the read-only inspector: **declined**. `os.getpid()` and `os.path.getsize()` were used read-only.

## To finish A2–A4 (for the orchestrator — options, not rulings)

1. **Fastest:** Jonathan right-clicks **`deck4`** once in the Deck Builder, in any PIE session (the choice persists to the save and `deck4` is already legal at 50/50). Then re-dispatch the verifier for A2–A4 only.
2. **Durable:** a keyboard/agent route for "set active deck". That is a code task through the manager, and it touches his DECK-§3 "right-click stays mouse-only" ruling, so it is **his call**.
3. **For the manager:** the `double_click` finding (§1b/§1c) bears on `VER-§5` cl. 5 / `TASK-1390`'s pointer ceiling. The same run also shows **no right button**, so the right-click half of that ceiling still stands.

Fences: no code, asset, compile, git, `CONVENTIONS.md`, `CLAUDE.md`, or `TASKBOARD.md` edit. My only writes are this file and the two evidence PNGs, plus the requested `deck4` save change made through the game's own auto-save.
