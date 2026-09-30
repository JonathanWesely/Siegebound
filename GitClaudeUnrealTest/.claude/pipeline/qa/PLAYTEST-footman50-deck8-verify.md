Verdict: VERIFIED
# Verification — PLAYTEST-footman50-deck8 (ad-hoc, Jonathan-directed exploratory playtest; no board row)

## THE ANSWER FIRST

**The agent ran the whole request end to end in ONE PIE session, driving the game the way a player would, and every step was read back.**
- **Deck slot 8 now holds exactly 50× `Footman`.** It was built through the real Deck Builder UI. Three readers agree: the live widget read, the save on disk read mid-PIE and again after PIE, and a frame.
- **Deck 8 was made the active deck** from the keyboard (`IA_MenuSecondary`, the `TASK-1514` route), with no right-click.
- **A vs-bot match was started from the main menu.** The game logged that it used deck8: `active saved deck 'deck8' is legal (50 cards) — using it this match`. The hand was 6× Footman and the draw pile 44× Footman.
- **The hero walked into `CaptureZone_Center` and captured it (`Blue`), and a Footman was played from hand.** Gold went 44 → 35, which is the card's cost of 9. The card left the hand: `DrawPile` 44 → 43, `DiscardPile` 0 → 1. A new Blue `BP_Unit_Footman0` spawned at (−453, −271, 92) by the log (census (−378.9, −248.6, 92) 0.3 s later), inside the zone box X,Y ∈ [−840, 840].

**⚠ Declared, loudly — one input was not a literal player gesture.** After the card key, I aimed the placement cursor with ONE `pie_scene_edit` → `call_actor_function SetMouseLocation(640,420)` on `SiegePlayerController0`. That is the programmatic equivalent of moving the mouse. It is not a scene edit of the world, but it is not a hardware mouse move either. Why I did it: my own builder gestures had last left the cursor on the Exit button, and the recipe's "no aim, unset cursor" placement is a `HYPOTHESIS` (H1 of `TASK-1512`). A ghost read taken immediately before the confirm documented where it would land: (−453.0, −271.4), visible, inside the zone. Nothing else was scene-edited. No actor was spawned, moved, deleted or had a property set by me.

**Why line 1 reads `VERIFIED`:** every acceptance line has a `pass` with a named control, and none has a `fail` or an `unobs` (`VER-§1` cl. 5, rule 2). The archer50 precedent read `MEASURED` only because A2–A4 were unreached; here all four were reached.

Editor/Aura state: Aura connected **y** (`get_headless_status` → `editor_connected`). The editor is **PID 18832**, corroborated from inside (`os.getpid() = 18832` before and after). `SystemLibrary.get_command_line()` returned `''` (the project argument only, as in archer50). The orchestrator's census of this PID by command line (GUI `UnrealEditor.exe` + `.uproject`, no `-game`) is theirs; I hold no shell to enumerate processes, so no `-game` instance was enumerated, driven or touched by me. Received on `L_Arena`, PIE off (`is_pie_active` → `false`), dirty packages `[]`. I loaded `/Game/Maps/L_MainMenu` for the run (`discarded_unsaved: false`) and **restored `L_Arena` at the end** (`discarded_unsaved: false`). PIE: standalone, 1 client, 1280×720 requested / viewport 1280×725, DPI 0.6706. **Attempts used: 1 of 3**, one PIE session (menu t≈0 → 230.5 s, travel, arena t=0 → 64.6 s; `stop_pie` issued by me). Wall time ≈ 5.5 min from my start post (15:52:33 local) to the end-state read (15:58:06); the PIE session itself ran ≈ 15:52:45 → 15:57:40. Credit: none surfaced. The PIE announcement, per the dispatch: "**PIE is announced:** the orchestrator told Jonathan PIE will run on PID 18832 and to keep his hands off (`VER-§3` cl. 6)." model (self-reported): `claude-opus-5-5[1m]` (the model ID my environment states for me).

**`VER-§12` cl. 7g BS_ERROR walk** (`unreal.ObjectIterator(unreal.Blueprint)`, counting `BS_ERROR`, with the resident total as the liveness control):

| when | resident | BS_ERROR | histogram |
|---|---|---|---|
| pre-flight, on `L_Arena` | 44 | **0** | 27 UP_TO_DATE, 17 UNKNOWN |
| after loading `L_MainMenu`, before PIE | 50 | **0** | 33 / 17 |
| after PIE | 51 | **0** | 34 / 17 |
| end state, `L_Arena` restored | 51 | **0** | 34 / 17 |

No PIE-start modal appeared: the first in-PIE read landed at t=6.09.

## Capability scorecard

| line | result | route that worked | routes that failed (and what replaced them) | tool calls into PIE for the segment | PIE clock for the segment |
|---|---|---|---|---|---|
| **A3a** menu → Deck Builder | pass | `RCP-menu-to-deckbuilder.md`, waited form: `IA_MenuDown` ×2 with 0.3 s between, then focus read, then `IA_MenuAccept`. **2/2 Downs landed**, no top-up | none | 2 `run_verification_sequence` | t≈4 → builder open t=10.96 |
| **A1** deck8 = 50× Footman | **pass** | `RCP-deckbuilder-slot-and-card-edit.md`: `ui_snapshot` resolve; `double_click` on `DeckSlotEntryWidget_7` (dx −45); calibration 1 `double_click` on the Footman tile's "+"; batch of **48 sent / 48 landed** at 22-frame spacing (1 → 49); top-up **1 sent / 1 landed** (→ 50) | `ui_snapshot {by:text,"Footman"}` errors (nested text is not indexed: the recipe's `by:name` hazard, text flavour). A full `WrapBox_0` snapshot overflowed the reply (203 k chars), so I read it from the saved file | 3 `ui_snapshot`, 4 `ui_perform`, 3 reads | builder open t=10.96 → 50 read t=127.29 (≈116 s) |
| **A2a** deck8 made active | **pass** | `IA_MenuDown` (re-establishes focus after the `ui_perform`s, per cl. 7f) → `FocusedCardIndex=0` → `IA_MenuBack` → **`DeckSlotEntryWidget_7` SlotButton the only focused slot**, then ONE batch of before-reads + `IA_MenuSecondary` + after-reads | none. ⚠ **Outside the set-active recipe's Precondition 5** ("no `ui_perform` before the walk"): I did 4 `ui_perform`s first, and **no walk** was needed because `IA_MenuBack` lands on the EDITING slot (7). A new measurement, not a recipe use (see *Recipe candidates*) | 2 `run_verification_sequence` | t=127.38 → 136.59 |
| builder → menu, refocus | done | `double_click` on Exit (`TextBlock_10`), then `IA_MenuDown`, read, `IA_MenuUp`, read `Button_0` focused | `ui_snapshot "WBP_MainMenu"` → *"matched 2 widgets"* ×5 (the known two-instance hazard). `ui_snapshot "WBP_MainMenu_C_1"` → *"No UserWidget in PIE matches"*. **Replacement:** read `UButton.HasKeyboardFocus()` on `WBP_MainMenu_C_1`'s buttons through the read-only Python lane (`WBP_MainMenu_C_1` is the one `IsInViewport()=True`, `HasFocusedDescendants()=True`) | 1 `ui_snapshot`, 1 `ui_perform`, 1 sequence, 2 standalone, 2 Python reads | t=152.5 → 192.5 |
| **A3** match vs Bot from the main menu | **pass** | `IA_MenuAccept` on `Button_0` "Play (vs Bot)" as row 1 of THE batch (`RCP-vsbot-capture-center-and-summon.md`) | none | 1 `run_verification_sequence` (36 actions) for A3+A2b+A4 | menu t=230.53 → arena |
| **A2b** match uses deck8 | **pass** | in-batch `DeckComponent` read at arena t=2.70; match-start log lines | none | (same batch) | arena t=2.70 |
| **A4** Footman summoned inside `CaptureZone_Center` | **pass** | recipe walk (`IA_Sprint` hold 29 + `IA_Move (0,−1)` hold 28.5): hero at X=247.03 at t=32.47; zone `Blue` at t=33.70; `IA_Card1` (t=33.79) + **declared `SetMouseLocation(640,420)`** (t=33.80) + wait 0.2 + ghost read (t=34.05) + `simulate_key_press LeftMouseButton` (t=34.07) + post-reads + no-play-interval control | none failed. The recipe's "no aim" step was replaced on purpose by the declared cursor set (THE ANSWER) | (same batch) | arena t=2.95 → 37.57 |

Totals: **1 attempt of 3; 1 PIE session;** A1 took 4 `ui_perform` calls for 50 edits (2 gestures on slot/calibration plus batches of 48 and 1), with no dropped gesture at 22-frame spacing (archer50 lost 3 of 30 at 20 frames). This time the whole arena leg is one call.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| A1 | Deck slot 8 holds exactly 50× Footman and no other card, built through the real Deck Builder UI; confirmed by a live widget read, the on-disk save after PIE, and a frame | (i) `WBP_DeckBuilder_C_0.WorkingDeck` + `EditingDeckIndex`; (ii) `load_game_from_slot("SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34",0)`, in PIE and after PIE; (iii) the auto-save log; (iv) a frame. **Control:** the pre-run read of the same slot, and the calibration gesture | **Before (pre-flight disk read):** `deck8 total=0 []`. (i) t=59.65: `EditingDeckIndex=7`, `WorkingDeck=(DeckName="deck8")`; calibration t=65.56: `(CardID="Footman",Count=1)`; after the 48-batch t=115.86: `Count=49`; after the top-up **t=127.29: `EditingDeckIndex=7`, `WorkingDeck=(DeckName="deck8",Cards=((CardID="Footman",Count=50)))`**, re-read at t=136.60 identical. (ii) in PIE: `DECK deck8 total=50 [('Footman', 50)]`; **after PIE: `DECK deck8 total=50 [('Footman', 50)]`**. (iii) `[2026.09.29-22.54.46:756][204] … saved deck 'deck8' (50 cards) to slot 'SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34'.`, the end of a monotonic run `(46)`…`(49)` → `(50)`. (iv) `Saved/AuraVerify/PLAYTEST-footman50-deck8-frames/VER-PLAYTEST-footman50-deck8-deck8-set-active_t136.62s_f435173.png`: the first tile (Footman) shows "50", every other tile "0", and "Deck: 50/50" bottom-left | **pass** |
| A2 | Deck 8 made active (`ActiveDeckName`), and the vs-bot match uses it: in-match hand and draw pile all Footman | `OutlineBorder.BrushColor` before/after on slot 2 (previously active deck3) and slot 7; the log; `ActiveDeckName` on disk; in-match `DeckComponent.Hand`/`DrawPile` on `SiegePlayerController0`; match-start log. **Control:** `ActiveDeckName` was `deck3` before the run, and deck3 holds **no** Footman (`Fog 10, WatchTower 11, Knight 8, Archer 8, ArrowTower 7, Witch 6`), so an all-Footman hand discriminates which deck was dealt | Before the press (t=136.02/136.03): slot 2 `(R=1,G=0.5,B=0,A=1)`, slot 7 `(0,0,0,0)`. `IA_MenuSecondary` t=136.05. After (t=136.57/136.59): **slot 7 `(R=1,G=0.5,B=0,A=1)`, slot 2 `(0,0,0,0)`**. Log `[22.55.02:377] UDeckBuilderWidget: active deck set to 'deck8' — the next match will use it.` Disk in PIE and after PIE: **`ActiveDeckName=deck8`**. Match: `[22.56.37:574] … active saved deck 'deck8' is legal (50 cards) — using it this match` and `built a 50-card draw pile from the pending OVERRIDE deck 'deck8' (1 entries)`. Arena t=2.70: **`Hand` = 6× `"Footman"`, `DrawPile` = 44× `"Footman"`** (6 + 44 = 50) | **pass** |
| A3 | A match vs Bot is started from the main menu | focus on `Button_0` "Play (vs Bot)" before Accept; `IA_MenuAccept`; the `StartMatch` log; world `L_Arena` | `WBP_MainMenu_C_1.Button_0 focus=True` (Python read after `IA_MenuUp`, menu t≈193). `IA_MenuAccept` at menu t=230.53 → `[22.56.36:862] [ASiegeGameMode::StartMatch] Main menu -> opening arena '/Game/Maps/L_Arena.L_Arena' into a fresh match vs the bot (GDD §7).` The census at arena t=33.75 reads `world_name: L_Arena`. The PIE clock restarted (2.67 at the first arena read) | **pass** |
| A4 | A Footman is summoned by playing the card from hand, placed inside `CaptureZone_Center`: gold spent, the card leaving the hand, the new unit inside the zone box, with a control | zone box + `CaptureOwner`; hero transform; entry (`GhostActor` `None` → non-`None` + entry log); the pre-press ghost read; gold; `BP_Unit_Footman_C` census; `DrawPile`/`DiscardPile`; the unit's `Team`/`CardID`; the `played card` log. **Control:** the no-play interval before and after | Zone: `ZoneHalfExtent (X=840,Y=840)` about `(0,0,0)` ⇒ X,Y ∈ [−840, 840]; `CaptureOwner` `Neutral` (t=2.68, 16.98) → **`Blue` (t=33.70)**, still `Blue` at t=34.40 and 37.57. Hero X −21007.8 (t=2.93) → −10656.5 at 750 uu/s (t=16.96) → **247.03, stopped (t=32.47)**. Pre-play (t=33.72–33.75): `GhostActor None`, gold 43, `DrawPile` 44, census filter "Footman" → 0 classes. Entry: `IA_Card1` t=33.79 → `[22.57.11:126] placement mode entered for card 'Footman' (cost 9).` Aim (declared): `SetMouseLocation(640,420)`, `applied 1, failed 0`, t=33.80. **Pre-press ghost read t=34.05: `StaticMeshActor_34`, `bHidden false`, `(X=-453.007190,Y=-271.406683,Z=0)` → inside**, gold 44. Confirm `LeftMouseButton` t=34.07 → `[22.57.11:465] played card 'Footman' for 9 gold — spawned 'BP_Unit_Footman_C_0' at (-453, -271, 92).` Post (t=34.35–34.40): **gold 35** (44 → 35 = −9, exactly the logged cost); `GhostActor None`; census **`BP_Unit_Footman_C` count 1, `BP_Unit_Footman0` at (−378.88, −248.64, 92)**, inside; **`DrawPile` 43, `DiscardPile ("Footman")`**; `Hand` still 6× Footman (refilled, blind by design). Unit read t=45.21: **`Team Blue`, `CardID Footman`** (by then it had walked to (347.0, −41.8, 92)) | **pass** |

**The A4 control, the no-play interval:**
- **Before the play** (t=2.72 → 33.74, no card key): gold only rose, 12 → 43; the census read 0; `DrawPile` 44 at both t=2.70 and t=33.72.
- **At the play:** gold −9, census 0 → 1, `DrawPile` −1, `DiscardPile` +1, and one `played card` line.
- **After the play** (t=34.35 → 37.57, ≈3.2 s, no card key): census stayed 1; gold rose 35 → 38; `DrawPile` stayed 43 and `DiscardPile` 1; `GhostActor None`.
- Log: `played card` **total_matches = 1** for the whole session.
- The census zero at t=33.75 is a zero, not a void: the same reader with the same filter returned `BP_Unit_Footman_C` at t=34.37 and t=37.52.
- The treatment window is t=33.79 → 34.40 (≈0.6 s). The in-batch post-play interval is ≈3.2 s, which is shorter than "equal" to the ≈31 s pre-play half. Stated from the stamps.

## Save before → after (what changed in his save, in plain words)

**Backup (made by the orchestrator at 15:51 with `cp -p`):** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\AuraVerify\PLAYTEST-footman50-deck8-sav-backup\`. I verified all 5 files byte-equal to the live files before PIE (sha256, size and mtime identical).

| file | before (sha256 · size · mtime) | after | changed? |
|---|---|---|---|
| `SiegeAccounts.sav` | `2fd96fe1…42b1` · 3083 · 2026-08-28 22:43:52 | identical | no |
| `SiegeDecks.sav` (legacy) | `646d442f…a071` · 3814 · 2026-08-02 10:09:34 | identical | no |
| `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `13f41048…65b9` · 6520 · 2026-09-26 23:10:42 | **`49dce23c84d9c98e528ef999241178e869bb804c275e4e7ed44cc606e8e5dab0` · 6617 · 2026-09-29 15:55:02** | **yes** |
| `SiegeSettings.sav` | `c8555088…e2f7` · 2004 · 2026-08-04 22:24:38 | identical | no |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `684ae64f…1793` · 2056 · 2026-09-09 13:48:19 | identical | no |

In plain words:
- **Deck 8 went from empty to 50 Footman.**
- **His active deck went from `deck3` to `deck8`,** so his next match will deal all Footmen.
- **Nothing else moved.** deck1 (51, his), deck2 (80), deck3 (50), deck4 (50× Archer) and deck5/6/7/9/10 (empty) are field-for-field identical to the pre-run read.
- The file's mtime, 15:55:02, is the set-active press.
- **I restored nothing.** To undo, copy the backup's `SiegeDecks_4E46….sav` back, or in game make deck3 active again (keyboard route: `IA_MenuLeft` ×5 from slot 7, then `IA_MenuSecondary`) and clear deck8.

## Films (raw `.h264`; the remux is the orchestrator's)

1. **Menu + builder film:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\AuraVerify\PLAYTEST-footman50-deck8-a1-menu\recording.h264`, 30,014,553 bytes, plus `recording_index.json`.
   - Armed before `start_pie` (`arm_wait_for_pie`), so it starts at PIE start.
   - Its last write is **15:56:36, the same second as the `StartMatch` travel line**, so it ends at the travel.
   - `stop_pie_recording` did not return it (the second call answered "No active or completed recording"); I found it by directory listing.
   - This is a **third sighting of `HYPOTHESIS` H3** ("a pre-start film ends at the level travel"). There is still no control, so it stays a hypothesis.
   - Timing map (PIE clock, menu leg):

     | PIE t | event |
     |---|---|
     | 6.1 | menu focus |
     | 10.0 | builder opens |
     | 54 | deck8 selected |
     | 65 | first Footman |
     | 66 → 116 | the 48-gesture batch |
     | ≈120 | 50th Footman |
     | 127.3 | frame |
     | 136.05 | set-active press |
     | ≈153 | Exit |
     | 192.5 | `IA_MenuUp` |
     | 230.5 | Play (vs Bot) |
2. **Arena film:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\AuraVerify\rec_burst_639263193998810000\recording.h264`, 32,307,450 bytes, plus `recording_index.json`.
   - `capture_mode: nvenc`, `container: h264`, 1512 frames, 30 fps, 1280×720, `duration_seconds: 50.4`, `dropped_stages: 0`.
   - Auto-started by the in-batch `record_burst {"seconds":1}` at arena t=2.67; a second burst `{"seconds":4,"interval_seconds":0.1}` at t=33.77 densified the summon. This is the recipe's measured route (`VER-§12` cl. 7b amendment).
   - **Timing map:** film game-time ≈ arena t − 2.67 (index frame 0 = game 0; frame 888 = game 31.39 → video 29.60 s).

     | arena t | film game | video | event |
     |---|---|---|---|
     | 2.95 | ≈0.3 | ≈0.3 s | walk starts |
     | 32.47 | ≈29.8 | ≈28 s | hero stopped in the zone |
     | 33.70 | — | — | zone `Blue` |
     | 34.07 | **31.40** | **≈29.6 s** | **the summon** |
     | 37.57 | — | — | control ends |
     | ≈53 | — | — | film end |

   - Remux: `ffmpeg -framerate 30 -i recording.h264 -c copy recording.mp4` in each folder. I saw no `.mp4` and claim none.

## Evidence (cited frames; not promoted: no git in this run, promotion owed to a later host)

All three frames are in `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\AuraVerify\PLAYTEST-footman50-deck8-frames\`. I viewed two by `Read` of the absolute path (STEP 6).
- `VER-PLAYTEST-footman50-deck8-deck8-set-active_t136.62s_f435173.png`: composited, 1280×725, `mean_luma 151`, 526,743 B. **Viewed.**
  - The "Deck Builder" screen over sky and a white plain.
  - The top deck bar runs `deck1`…`deck10`, and **`deck8` carries a highlight with a thin orange underline**.
  - The card strip: the **first tile, Footman, reads "50"**, and every other tile reads "0". Bottom-left reads "Deck: 50/50" (small text); the model reads above are authoritative.
  - Right: an empty details panel ("Click a card to see how it works", "Close"). Bottom: "Reset to Default", "Play", "Exit".
- `VER-PLAYTEST-footman50-deck8-after-summon_t34.42s_f442787.png`: composited, 1280×725, `mean_luma 114`, 1,993,740 B. **Viewed; it rendered despite being >1 MB.** It is also the **positive control** for the frame above: a visibly different screen.
  - The arena at dusk: autumn trees, a blue-roofed castle in the distance, and the hero knight (white surcoat, red cross) centre-screen.
  - **"Gold: 35" top-left**, which matches the post-play read.
  - A strip of six small "Footman" card labels bottom-centre.
  - A red **"Video memory has been exhausted (1516.746 MB over budget)"** banner across the middle.
  - A small figure behind the hero's left side is **not identifiable by eye as the Footman**. The pixels do not prove the summon; the reads and the log above do.
- `VER-PLAYTEST-footman50-deck8-builder-50-footman_t127.31s_f434618.png`: 1086×615, `mean_luma 151`. Captured at the 50 read and **not viewed**. It is superseded by the 1280 frame, and I cite nothing from it.

## Hypotheses (not verdicts)

- **H1: `IA_MenuBack` from the card grid lands on the EDITING slot, not the active slot.** Observed once: `EditingDeckIndex=7` with deck3 (slot 2) active → `DeckSlotEntryWidget_7` focused. The recipe's rule was written when the editing and active slots coincided (a fresh open). One observation; mechanism not read.
- **H2: the 22-frame spacing lost nothing (48/48, 1/1)**, where archer50 lost 3/30 at 20 frames. One batch; the difference may be the load or luck. Not a drop rate.
- **H3 (third sighting): a film armed before `start_pie` ends at the level travel.** Its last write is the travel second. No control.
- The small figure in the after-summon frame may be the Footman. It is not identified.

## Not examined / limitations this run

- **`VER-§7` cl. 2 declaration (reachable, not granted):** `pie_scene_edit` was invoked **1 time**, inside `run_verification_sequence` at arena t=33.80: op `call_actor_function`, target `SiegePlayerController0`, function `SetMouseLocation`, args `{"X":640,"Y":420}`, `applied: 1, failed: 0`. No other op; no `spawn_actor`, `delete_actor`, `set_actor_*` or `teleport_player`. That aim is outside every recipe's fences: the play recipe's aim is measured only at the Blue spawn. So **this is the first measured `SetMouseLocation` aim composed with an `IA_Card<N>` entry in one batch that placed inside a capture zone.** It is one run, not calibrated: (640,420) mapped to (−453, −271) from a hero at X=247, camera at X=647, yaw 180.
- **The Python lane was used read-only for:** the `.sav` hashing (`hashlib`), `load_game_from_slot`, the BS_ERROR walks, the dirty-package reads, and `UButton.HasKeyboardFocus()` / `UserWidget.IsInViewport()` / `HasFocusedDescendants()` on the main menu (to get past the two-instance `ui_snapshot` ambiguity). Shell, `subprocess` and `ctypes`: not used.
- **No process census by command line from my side** (no shell). PID 18832 was corroborated from inside only. Any `-game` instance was neither enumerated nor touched.
- `start_pie`'s reply was short this run. Every `ui_perform` reply overflowed (515–623 k chars, because of the before/after snapshots); I read them by grep (`ok`, `handled`, `hit_widget`, step counts). The observable was always the state read after, never the trace. The first `stop_pie_recording` reply overflowed (163 k); I read its head.
- **The batch sizes, sent vs landed:**

  | batch | sent | landed |
  |---|---|---|
  | menu Downs | 2 | 2 |
  | slot select | 1 | 1 (`EditingDeckIndex` 2 → 7) |
  | calibration | 1 | 1 |
  | Footman "+" batch | 48 (counted in the reply) | 48 (1 → 49) |
  | top-up | 1 | 1 (→ 50) |
  | grid arm | 1 | 1 |
  | Back | 1 | 1 |
  | Secondary | 1 | 1 |
  | menu refocus | 1 Down + 1 Up | 1 + 1 |
  | Play | 1 | 1 |

  No overshoot: the batch was sized to the shortfall.
- The first Footman batch was planned as 49 and went out as 48 (my step array). It was caught by the read-back and topped up. It is reported, not hidden.
- The Aura plugin version was not read.
- The unit's own post-spawn location was read at t=34.37 and t=37.52 (it walks). Its spawn point is the log's (−453, −271, 92). Both are inside the zone.
- `RootComponent.RelativeLocation` of the zone is taken as its world centre, which assumes an unattached root (recipe caveat).
- The VRAM banner (1516.7 MB over budget) was present in the arena frame.
- `SiegeSettings_*`, `SiegeAccounts` and the legacy `.sav` files were hashed (unchanged) but not field-read.

## Recipes used (S3)

- `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md`: Steps 0–4, waited form. **Re-verified in-run y**: `Button_0` focused t=6.09; Downs 2/2; `Button_2` focused t=6.74; Accept t=10.01; Step 4 `EditingDeckIndex=2` with deck3 active (matches the amended expectation).
- `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md`: Steps 1–6. **Re-verified in-run y** (its first re-run since seeding): the slot `double_click` at dx −45 selected slot 7; the tile "+" by plain name + offset (dx 14, dy 123, derived from Step 1's rects) fired once per gesture; read-back and top-up; Exit by `double_click` on `TextBlock_10`. New values: Footman = `WBP_DeckCardTile_C_0`, "+" = its `Button_2` at abs (1318.3, 1186.6) 29×20; slot 7 `SlotButton` at abs (2186.9, 870.9) 121×31.
- `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md`: Step 0, Step 1, Step 2 (no Step 3 walk) and Step 4. **Used OUTSIDE Precondition 5** (4 `ui_perform` calls came first; focus re-established with `IA_MenuDown` per `VER-§12` cl. 7f). Every read matched. The target (slot 7) is new to the recipe.
- `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md`: Step 1 rows 2–16 and the Step 3 tail.
  - **Re-verified in-run y** for the walk, the capture and the in-zone summon (their first run since a2).
  - Deviations: row 1 (the `ui_snapshot` focus read) was replaced by a Python `HasKeyboardFocus` read before the batch, because of the two-instance hazard.
  - A declared `SetMouseLocation` aim was added.
  - The card is Footman (cost 9), and the census filter is `"Footman"`.
  - Row 15a (`DrawPile` after the play, in-batch) was **measured for the first time: 43 at t=34.39**.
- `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md`: the Step 3 entry and the pre-press ghost read; the no-play-interval control. The omitted-set control arm was not sent.

## Recipe candidates (for the manager; I write nothing under `Tools/`)

1. **Set active right after an edit, with no re-open.** After a card edit in the builder:
   1. `IA_MenuDown` (re-establish; `FocusedCardIndex` → 0);
   2. `IA_MenuBack` + a `DeckBar` snapshot (expect `DeckSlotEntryWidget_<EditingDeckIndex>` focused);
   3. then Step 4's batch.

   It needs no walk. Measured once here (t=127.38 → 136.59). It widens the set-active recipe's Precondition 5 in one case.
2. **Refocusing the main menu after a builder Exit, when `ui_snapshot "WBP_MainMenu"` is ambiguous.** A Python read-only `ObjectIterator(unreal.Button)` filtered to the in-viewport instance (`WBP_MainMenu_C_1`), then `HasKeyboardFocus()`. `IA_MenuDown` then `IA_MenuUp` walked `Button_1` → `Button_0`. The exact script is quoted in this run's tool call (iterate `unreal.Button`, filter `"WBP_MainMenu_C_1.Button_"` in `get_path_name()`, log `has_keyboard_focus()`).
3. **The in-zone aim:** `SetMouseLocation(640,420)` immediately after `IA_Card1`, with the hero stopped at X≈247, Y≈0 at the spawn camera (yaw 180, camera X≈647). The ghost went to (−453.0, −271.4), inside, and the unit placed. Once only; this is the first datum for the README's pending "calibrated aim into a capture zone" row.

## Tool findings for the manager

- `ui_snapshot {by:"text"}` does not reach nested tile text (it errors with an "Available:" list of top-level names only). Resolve tiles by a subtree snapshot of `WrapBox_0`, which overflows the reply at ≈200 k chars; read it from the saved file.
- `ui_snapshot` / `ui_perform` cannot address one of two same-class widgets: `"WBP_MainMenu_C_1"` is rejected. The read-only Python lane can.
- A `capture_pie_frame` with `max_dim: 1280` inside `run_verification_sequence` **worked** (1280×725 returned). That was `NOT MEASURED` in the set-active recipe.

Fences: no code, asset, compile, git, `CONVENTIONS.md`, `CLAUDE.md` or `TASKBOARD.md` edit. My only writes are this file and the three frames under `Saved/`, plus the requested save change (deck8 contents and `ActiveDeckName`), made through the game's own auto-save.
