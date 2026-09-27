Verdict: VERIFIED
# Verification — TASK-1511 (5b for TASK-1507)
Editor/Aura state: connected y; map `/Game/Maps/L_MainMenu` (loaded by `load_level`, `discarded_unsaved: false`; the editor world was `L_Arena` before); PIE standalone, 1 client, 1280×720 requested → viewport 1280×725, DPI 0.6706; editor instance = PID 7008 (orchestrator census by command line: `"…\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "…\GitClaudeUnrealTest.uproject"`, the GUI editor, no `-game`; my SC-§118 cl. 9 substitute: `os.getpid()` inside the editor Python = **7008**, and `is_pie_active` = `false` before I started, so no session of his was running); attempts used **1 of 3**; PIE clock 0.40 s → 129.48 s; credit not visible; model (self-reported): "Opus 5.5 (1M context)", model ID `claude-opus-5-5[1m]` (as seen in my own system context).

Recording (raw elementary stream, remux owed to the orchestrator): `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/AuraVerify/TASK-1511/recording.h264` — `capture_mode: nvenc`, `container: h264`, 3547 frames @ 30 fps, 1280×720, 118.23 s video; index `Saved/AuraVerify/TASK-1511/recording_index.json`. Armed before `start_pie` (`arm_wait_for_pie`), first frame at game t=0.40 s. No `.mp4` seen.

## Pre-registration (written BEFORE PIE, 2026-09-26)
Disk read before PIE (editor PID 7008, `load_game_from_slot("SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34", 0)`, class `SiegeDeckSaveGame`):
- `ActiveDeckName = deck1` (expected before-value for A1/A2/A3).
- deck1 = 51 cards (WatchTower 6, BrightSun 6, Wall 7, Fog 6, Archer 10, ArrowTower 3, Miner 3, Footman 9, Cleric 1) — illegal (51/50).
- deck2 = 80 cards (Wall 14, MilitiaMob 6, Pikeman 6, Sapper 4, Cavalry 4, ArrowTower 15, Archer 15, Masons 7, Miner 4, Knight 5).
- deck3 = 50 cards (Fog 10, WatchTower 11, Knight 8, Archer 8, ArrowTower 7, Witch 6).
- deck4 = 50× Archer.
- deck5 … deck10 = empty (0 cards).
Expected after A1: `ActiveDeckName = deck4` live and on disk; every deck's contents unchanged.
Expected under A2/A3 controls: `ActiveDeckName` stays at its value before the control press.
(This section was written to this file before `start_pie` and is reproduced unchanged.)

## Acceptance lines → observations
| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 0 | (precondition, 5a handoff) the seventh binding resolves | builder-open bind log line, read AFTER the open | `[00.12.06:863] UDeckBuilderWidget::BindMenuNavActions: 7 IA_Menu* action(s) bound (Started) on 'PlayerController_0' [IA_MenuUp, IA_MenuDown, IA_MenuLeft, IA_MenuRight, IA_MenuAccept, IA_MenuBack, IA_MenuSecondary]; every IA_Menu* asset resolved.` | pass |
| A1 | the route alone (injected actions only) sets deck4 active; live + disk + orange outline, positive-control frame before | per-step `ui_snapshot` of `DeckBar` (focused `SlotButton`); relay log lines; `OutlineBorder.BrushColor` of slot entries 0 and 3 before/after; set-active log line; `load_game_from_slot` in PIE and after PIE; composited frames `Read` | **Route** (all `inject_input_action`, no pointer gesture, no `pie_scene_edit`): `IA_MenuDown` t=56.36 → `FocusedCardIndex=0`, `WBP_DeckCardTile_C_0 focused:true` · `IA_MenuBack` t=65.69 → only focused node `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` · `IA_MenuRight` t≈66.03 → `DeckSlotEntryWidget_1/…/SlotButton` · t≈66.36 → `_2` · t≈66.69 → `_3` (exactly one `focused: true` per snapshot). Relay lines: `IA_MenuRight -> 'Right' handed to Slate's own key route on deck-bar slot 0 ('deck1'); key-down handled=true; the focused bar slot is now 1 ('deck2')`, then 1→2 ('deck3'), 2→3 ('deck4'). **Before the press** (t=80.80): slot 0 BrushColor `(R=1,G=0.5,B=0,A=1)`, slot 3 `(R=0,G=0,B=0,A=0)` (read t=45.70 and again unchanged at 57.12). **`IA_MenuSecondary` t=80.87** → log `[00.13.13:066] UDeckBuilderWidget: active deck set to 'deck4' — the next match will use it.`; t=81.40 slot 0 BrushColor `(R=0,G=0,B=0,A=0)`, slot 3 `(R=1,G=0.5,B=0,A=1)`. **Live disk read in PIE** (after the press): `ActiveDeckName=deck4`. **Disk after PIE stopped**: `ActiveDeckName=deck4`. `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards (set-active did not re-select for editing). Frames: see Evidence (pixel note below). | pass |
| A2 | control: key with focus NOT on a slot changes nothing (card tile focused; cold open) | same BrushColor reads + `FocusedCardIndex` + log census | **Cold open** (t=45.29, `FocusedCardIndex=-1`, no bar `SlotButton` focused per the t=34.56 `DeckBar` snapshot): slot 0 stays `(1,0.5,0,1)`, slot 3 stays `(0,0,0,0)` at t=45.70; `EditingDeckIndex=0`. **Card tile focused** (t=56.71, `WBP_DeckCardTile_C_0 focused:true`, `FocusedCardIndex=0`): slot 0 stays `(1,0.5,0,1)`, slot 3 stays `(0,0,0,0)` at t=57.12–57.14; `WorkingDeck` unchanged (deck1, same 9 entries, 51 cards). Log census: `LogGitClaudeUnrealTest` held **9** lines for the whole session (1 bind, 2 focus read-backs, 4 relay, 1 set-active for A1, 1 refusal for A3) — **zero** set-active or refusal lines between the builder open (00.12.06) and the first relay (00.12.58), the window that contains both control presses. Disk `ActiveDeckName` was `deck1` before PIE and the first change is A1's. | pass |
| A3 | control: same legality gate as right-click, on an empty slot (deck5–deck10 re-read first) | disk read of deck5–10 in PIE before the press; relay line; BrushColor slots 3/4/0; refusal log line; disk after | Re-read in PIE before the press: deck5…deck10 `total=0`. `IA_MenuRight` t=110.31 → focused `DeckSlotEntryWidget_4/OutlineBorder/SlotButton`; relay `… on deck-bar slot 3 ('deck4'); key-down handled=true; the focused bar slot is now 4 ('deck5')`. `IA_MenuSecondary` t=110.64 → `Warning: UDeckBuilderWidget::SetActiveDeck('deck5'): Deck has 0 cards — a legal deck is exactly 50 (GDD 3.4). — refused; the active deck stays 'deck4'.` BrushColor at t=111.16–111.19: slot 3 `(1,0.5,0,1)`, slot 4 `(0,0,0,0)`, slot 0 `(0,0,0,0)`. Disk after PIE: `ActiveDeckName=deck4`. | pass |
| A4 | right-click still works | — | UNOBSERVABLE by construction: this lane has no right mouse button (`VER-§8` cl. 12). Declared by the row; not re-traced. | unobs |
| A5 | his real `Home` key (Slate door) | — | UNOBSERVABLE by this rig (`VER-§8` cl. 1). Closes on his hand check, which the orchestrator puts to him after `TASK-1512`, not this run. Pre-written sentence with `<N>` filled from this report's disk read: ***"Open the Deck Builder, press Down, then Escape, then Right until the highlight sits on deck3, and press Home — does the orange outline jump to deck3? (deck3's button just highlighting, or the cards below switching to deck3's, is a NO: that is 'select for editing', not 'active'.)"*** — N=3: deck3 = exactly 50 cards on disk after PIE and is not the active deck (deck4 is). | unobs |

**Pixel note (A1 outline).** The full-resolution frame after the press (`a1_after_fullres_t88.69s_f18174.png`, 1280×725) shows a thin orange line along the bottom edge of the `deck4` tab and none under `deck1`; the A3 frame at t=111.21 (1280×725) shows the same orange line still under `deck4` and none under `deck5`. The positive-control frame before the press (`pie_composited_c2_t80.80s_f17736.png`) was captured at the default 1086-px downscale: at that size I could not resolve an orange line under `deck1` by eye (deck1's tab carries the blue editing tint, which also confounds it). The before/after 1086-px pair (`c2` t=80.80 vs `c3` t=81.45) does differ at the `deck4` tab (plain in `c2`, visibly outlined/lighter in `c3`), so the pair discriminates, but the "outline on deck1 before" leg of the pixel control rests on the live `BrushColor` read `(1,0.5,0,1)`, not on pixels. I label that leg as read-proven, not pixel-proven.

## Save hygiene
All ten decks read before PIE, in PIE after A1, and after PIE: identical card lists and totals every time (deck1 51 · deck2 80 · deck3 50 · deck4 50× Archer · deck5–10 0). The only change is `ActiveDeckName` `deck1` → `deck4`. The guest slot `SiegeDecks` read after PIE: `ActiveDeckName=Active`, 1 deck (as before). deck4 left active, as the row requires. The orchestrator's `.sav` re-hash is the byte-level check; I held no hash tool.

## Evidence (promoted)
Promotion **owed to the host row** (`TASK-1514`): I hold no shell and PIE was stopped before I could re-capture straight into `.claude/pipeline/playtest-evidence/2026-09-26/`. No promoted path exists yet. Source files in `Saved/` and their intended names:
- `Saved/AuraVerify/pie_composited_c2_t80.80s_f17736.png` → `VER-TASK-1511-t01m20s-bar-before-press-focus-deck4.png` — 1086×615; deck builder, bar at top, deck1 tab blue-tinted (editing), no visible outline on deck4.
- `Saved/AuraVerify/pie_composited_c3_t81.45s_f17772.png` → `VER-TASK-1511-t01m21s-bar-after-secondary-deck4.png` — 1086×615; same screen, deck4 tab now outlined/lighter.
- `Saved/AuraVerify/TASK-1511/a1_after_fullres_t88.69s_f18174.png` → `VER-TASK-1511-t01m28s-orange-outline-deck4.png` — 1280×725; thin orange line under the deck4 tab, none under deck1.
- `Saved/AuraVerify/pie_composited_c4_t111.21s_f19521.png` → `VER-TASK-1511-t01m51s-empty-deck5-refused-outline-stays-deck4.png` — 1280×725; orange line still under deck4 after the deck5 press.
- `Saved/AuraVerify/pie_composited_c1_t45.21s_f15606.png` (cold-open baseline, 1086×615) — not cited for promotion.

## Hypotheses (not verdicts)
- The relay handing `Right` to `FSlateApplication::ProcessKeyDownEvent` walks the same user's focus as a physical key (the handoff's open "user index" question): HYPOTHESIS supported by each `ui_snapshot` showing exactly one focused `SlotButton` advancing by one, but a physical key was not pressed, so equality with it is not measured.

## Not examined / limitations this run
- A4 and A5 (declared ceilings above). No right mouse button; no physical `Home`.
- Gamepad `Y` (`Gamepad_FaceButton_Top`) was not driven; `IA_MenuSecondary` was injected directly, so the IMC's key rows were not exercised.
- `IA_MenuLeft` on the bar and the bar's ends (slot 0 Left, slot 9 Right) were not driven.
- The stale grid-armed state after a mouse click (QA N4) was not reached: no mouse input was sent at all.
- Aura plugin version not read.
- Evidence promotion owed (above). No `.mp4` seen.
- Wall time from dispatch to PIE is not observable to me; see speed data.

## Speed data (`VER-§13` had landed — commit `36cb4dd`; doctrine and recipe library were on disk and loaded)
- Tool uses before `start_pie` returned: 15 (including row/recipe/handoff reads, pre-PIE disk read, report pre-registration, board flip, `load_level`, recorder arm). Wall time not self-observable.
- PIE sessions: 1. PIE clock at `stop_pie`: 129.48 s. Log time: PIE up ≈00:11:51, builder open 00:12:06, A1 set-active 00:13:13, A3 refusal 00:13:52.
- Total tool uses ≈ 52 (vs the `PLAYTEST-archer50` baseline's 128; not per-segment comparable — that run carried ~100 deck-edit gestures this one did not).
- Batches (sent vs landed, asserted by state reads): menu `IA_MenuDown`×2 → 2 landed (focus `Button_2`, 2 subsystem log lines); `IA_MenuAccept` 1/1 (builder open); cold-open `IA_MenuSecondary` 1 sent, state unchanged by design; `IA_MenuDown` + tile-focused `IA_MenuSecondary` 2 sent, Down landed (`FocusedCardIndex=0`), Secondary changed nothing by design; `IA_MenuBack` + `IA_MenuRight`×3 → 4/4 landed (one focus step per snapshot, 3 relay lines); `IA_MenuSecondary` on deck4 1/1 (set-active line + outline moved); `IA_MenuRight` + `IA_MenuSecondary` on deck5 2/2 (relay line + refusal line). No drops observed at 0.3 s spacing with inject-only input.

## Recipes used
- `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` · Steps 0–4 (focus read `Button_0` t=4.89; `IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62; `IA_MenuAccept` t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14) · re-verified in-run **y** (first use; the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections).

## Recipe candidates
**Set the active deck by keyboard (proposed `RCP-deckbuilder-set-active-by-keyboard.md`)** — measured this run, precondition: builder freshly open via `RCP-menu-to-deckbuilder.md`, no mouse input sent in the builder, `EditingDeckIndex=0`, target slot index `T` (0-based) known before the batch.
1. `inject_input_action /Game/Input/Actions/IA_MenuDown.IA_MenuDown` → read `FocusedCardIndex == 0` (t=56.67).
2. `inject_input_action /Game/Input/Actions/IA_MenuBack.IA_MenuBack` → `ui_snapshot {"widget":"WBP_DeckBuilder","selector":{"by":"name","value":"DeckBar"},"max_depth":3}`; expect `DeckSlotEntryWidget_<EditingDeckIndex>/OutlineBorder/SlotButton focused:true`.
3. `inject_input_action /Game/Input/Actions/IA_MenuRight.IA_MenuRight` × T, `wait_pie_seconds 0.3` + the same `DeckBar` snapshot after each (one relay log line per step: `RelayDeckBarNavigationKeyToSlate: IA_MenuRight -> 'Right' … the focused bar slot is now N`). Steps 2–3 ran as ONE `run_verification_sequence` (4/4 landed).
4. `inject_input_action /Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary`, `wait_pie_seconds 0.5`, then in the SAME batch `get_widget_property_in_pie {"widget":"WBP_DeckBuilder","child":"DeckSlotEntryWidget_<T>","properties":["OutlineBorder.BrushColor"]}` → `(R=1,G=0.5,B=0,A=1)`; previous active slot → `(0,0,0,0)`; log `active deck set to 'deck<T+1>'` (or the `refused; the active deck stays …` Warning for an illegal deck).
5. Disk: `load_game_from_slot("SiegeDecks_<profile>", 0).ActiveDeckName`.
Fences: T=3 (and T=4 refused) measured only; Left, bar ends, gamepad Y, and any builder state after a mouse click not measured. Capture frames with `max_dim` ≥ 1280 — at the default 1086 the outline is not resolvable by eye.
