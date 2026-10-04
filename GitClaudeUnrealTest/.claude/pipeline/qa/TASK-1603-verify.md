Verdict: VERIFIED
# Verification — TASK-1603 (5b for TASK-1600, the dev-only bot switch `SetBotEnabled`)

Marker `TASK-1603-BOT-SWITCH-5B`. 2026-10-04. playtest-verifier.

Editor/Aura state: connected y. Build configuration, by name: **Development Editor** (`GitClaudeUnrealTestEditor Win64 Development`, DLL `UnrealEditor-GitClaudeUnrealTest.dll` sha256 `fd938813…241ec8` per `handoffs/TASK-1602-buildmaster.md` §7; `LogSiegeBot` is declared at `Log` and its lines appear in the editor log, `VER-§9`). Editor instance: served Python `os.getpid()` = **30480** at the start and again at the end of the run, `unreal.SystemLibrary.get_command_line()` = empty string (a GUI launch on the plain `.uproject`, class EDITOR per `SC-§118` cl. 9(iii); matches the handoff's PID 30480 and its `LogInit: Command Line: ` empty). **Census PARTIAL:** the process list could not be enumerated from my lane (`import subprocess` is rejected by Aura's read-only validator: "Aura disallows import of subprocess module!"), so parts (iii)/(iv) of the four-part census — every other `UnrealEditor*.exe` and any `-game` instance — are NOT read; no `-game` instance was driven, PIE'd into or closed, because every PIE call went to the editor's own PIE on PID 30480. Editor world at my instant: `/Game/Maps/L_Arena` → I ran `load_level /Game/Maps/L_MainMenu` (`discarded_unsaved: false`) before PIE. PIE standalone, 1 client, viewport 1280×725, DPI 0.6706; `is_pie_active` false before each start. **Attempts used: 1 of 3** (one attempt = two PIE sessions: ARM A and ARM B). Wall time: PIE 1 opened ≈12:25:20Z (StartMatch 12:25:29.750Z) and was stopped at PIE t=215.86; PIE 2 StartMatch 12:29:40.317Z, stopped at PIE t=44.41; the whole run ≈12:22Z → 12:31Z. Credit: not visible. PIE announcement — the dispatch said, quoted: "**PIE is ANNOUNCED:** Jonathan is present and was told in Claude Code that PIE will run on PID 30480 and to keep his hands off; report, do not wait (`VER-§3` cl. 6)." The "Video memory has been exhausted" banner is in every arena frame (1114.297 MB over at t=60.07; 533.645 MB at t=135.10; 469.645 MB at t=135.60) — recorded, not diagnosed. Aura plugin version: not read. model (self-reported): "claude-opus-5-5[1m]".

Log source for every log citation: the live editor log `Saved/Logs/GitClaudeUnrealTest.log` (PID 30480's, opened 10/04/26 05:15:07), read with `get_unreal_output_logs`; it carried **zero** `LogSiegeBot` lines before my first PIE (read at the start of the run: `total_matches=0`). Stamps are UTC from the log; PIE times are the sequence's own `pie_time_seconds` (arena clock).

## Pre-registration (before PIE)

- Active deck read off the save through the read-only lane: `load_game_from_slot("SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34", 0)` → `SiegeDeckSaveGame`, `ActiveDeckName=deck8`. Not changed. The match confirmed it: `active saved deck 'deck8' is legal (50 cards) — using it this match (M6 TASK-114).` (12:25:31.425Z). The Blue hand read 6× `"Footman"`, the draw pile 44× `"Footman"` (t=59.53).
- `.sav` baseline (sha256 + size + mtime UTC), taken before PIE and re-read after both sessions — see *Save hygiene*.

## Timeline anchors (ARM A, PIE 1)

| event | log stamp (UTC) | PIE t (arena) |
|---|---|---|
| t0 — `[ASiegeGameMode::StartMatch] Main menu -> opening arena '/Game/Maps/L_Arena.L_Arena' into a fresh match vs the bot (GDD §7).` | 12:25:29.750 [935] | — (menu clock) |
| `[Bot SiegeBotController_0] Deck select: chose curated deck 0 of 2 'Bot Aggro Rush' (50 cards, avg cost 14.16) — pushed as pending override.` | 12:25:30.700 [936] | — |
| call 1 `SetBotEnabled(false)` → `ASiegePlayerController 'SiegePlayerController_0': SetBotEnabled(false) — forwarding to bot 'SiegeBotController_0' (dev-only switch, TASK-1600).` | 12:25:33.162 [957] | 0.017 |
| `[Bot SiegeBotController_0] bot disabled by SetBotEnabled` | 12:25:33.163 [957] | 0.017 |
| Blue play (P5) `played card 'Footman' for 9 gold — spawned 'BP_Unit_Footman_C_0' at (-21099, 6, 92).` | 12:27:53.342 [564] | 135.22 |
| call 2 `SetBotEnabled(true)` → `… SetBotEnabled(true) — forwarding to bot 'SiegeBotController_0' (dev-only switch, TASK-1600).` | 12:27:55.761 [702] | 137.74 |
| `[Bot SiegeBotController_0] bot enabled by SetBotEnabled` | 12:27:55.761 [702] | 137.74 |
| first decision line after it: `Rule 4 (Attack): played unit 'Cavalry' (cost 21) castle-front (19965, -709, 20) — marching (M7.6 ruling #1) — gold 148->127.` | 12:27:56.127 [719] | ≈138.1 |

Disabled window: 12:25:33.163 → 12:27:55.761 = **142.6 s wall**, PIE t 0.017 → 137.74 = **137.7 s arena** (≥120 s both ways).

**Declared deviation — the call landed at arena t≈0.02, not t≈5.** The batch was the recipe's menu route (`ui_snapshot Button_0` → `IA_MenuAccept` → `wait_pie_seconds 3` → `record_burst`), and the `wait 3` elapsed across the level travel: every action from the `record_burst` to the call stamped arena `pie_time_seconds` 0.016870. The call landed 21 frames after the bot's deck-select line ([936] → [957]) and before any decision line. It changes no property's meaning: ARM B's first bot play came at +14.4 s, so a disable at t≈0.02 and one at t≈5 both precede the bot's first play.

## Census and gold tables (ARM A)

Instruments: (1) **in-batch class census** `survey_pie_scene {"include":["census"], "class_filter":"BP_" or "BP_Unit"}` (team-blind; it lists every `BP_Unit_*` and `BP_Building_*` class present); (2) **team-split census** by read-only editor Python on the PIE world (`get_all_actors_of_class(gw, unreal.SummonedUnit / unreal.Building)`, `Team` per actor, `GameplayStatics.get_time_seconds` stamp) — a separate call, so its stamp is its own; (3) the bot's `DeckComponent` `Hand` / `DrawPile` / `DiscardPile` (in-batch); (4) Red gold = `SiegeBotController0` `PlayerState.Gold` (its `PlayerState.Team` read `Red`; the player's read `Blue`).

| label | PIE t | Red `SummonedUnit` / `Building` (names) | class census (`BP_Unit*`/`BP_Building*` classes) | bot `DiscardPile` / `Hand` | Red gold |
|---|---|---|---|---|---|
| C0 / G0 | 0.017 (in-batch, immediately before call 1) | — (team split not readable in-batch) | none (only `BP_Torch_C` 12, `BP_CommanderNpc_C` 2, `BP_BattlefieldScatter_C` 1, `BP_HeroCharacter_C` 1; 153 actors) | empty / `("MilitiaMob","Wall","Cavalry","Knight","Sapper","Pikeman")`, `DrawPile` 44 entries | **10** |
| (after call) | 0.017 | — | — | — | 10; `bBotEnabled` **false** |
| C1 / G1 | 22.74–22.77 | — | none (153 actors) | empty / same hand, same 44-entry draw pile | **32** |
| C1′ (Python) | 33.073 | **0 / 0** — `SummonedUnit none`, `Building none` | — | — | 43 (`SiegePlayerState_1 team=RED gold=43`) |
| — | 47.47 | — | — | — | 57 |
| C2 / G2 | 59.48–59.52 | — | none (`BP_Unit` filter: 0 classes, 153 actors) | empty / same hand, same draw pile | **69** |
| C3 / G3 | 85.00–85.04 | — | none (`BP_Unit` filter: 0 classes, 154 actors — the +1 is the ghost `StaticMeshActor_34`, below) | empty / same hand, same draw pile | **94** |
| — | 114.01 | — | — | — | 123 |
| C4 / G4 | 134.93–134.97 | — | none (154 actors) | empty | **144** |
| (pre-call-2) | 137.69–137.72 | — | `BP_Unit_Footman_C` 1 (Blue — my P5 play), nothing else; 155 actors | empty | 147; `bBotEnabled` false |
| (post-call-2) | 137.76 | — | — | — | 147; `bBotEnabled` **true** |
| C5 | 142.57–142.61 | — | `BP_Unit_MilitiaMob_C` 4, `BP_Unit_Cavalry_C` 1, `BP_Unit_Knight_C` 1, `BP_Unit_Footman_C` 1; 167 actors | **non-empty** | 98 |
| — | 152.44–152.48 | — | MilitiaMob 8, Pikeman 2, Sapper 2, Cavalry 1, Footman 1, Knight 1; 183 actors | non-empty | 33 |
| — | 165.71–165.74 | — | MilitiaMob 8, Knight 2, Pikeman 2, Sapper 2, Cavalry 1, Footman 1; 185 actors | non-empty | 28 |
| C5′ (Python) | 185.303 | **Red 19** `SummonedUnit`: `BP_Unit_Cavalry_C_0`, `BP_Unit_Knight_C_0`, `_C_1`, `BP_Unit_MilitiaMob_C_0`…`_C_11`, `BP_Unit_Pikeman_C_0`, `_C_1`, `BP_Unit_Sapper_C_0`, `_C_1`; **Blue 1** `BP_Unit_Footman_C_0@(-20873,24)`; `Building none` | — | — | — |

The +30/+60/+90/+120 checkpoints landed at +22.7, +59.5, +85.0 and +134.9 (sequence waits are game-time-approximate across the travel and the round trips); every value is quoted with its own stamp.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| P1 | the `bot disabled by SetBotEnabled` line appears EXACTLY ONCE between t0 and +120 s | count of `LogSiegeBot` lines containing `bot disabled by SetBotEnabled` in the editor log for PIE 1 | **1**: `[2026.10.04-12.25.33:163][957]LogSiegeBot: [Bot SiegeBotController_0] bot disabled by SetBotEnabled` — the only such line in the whole log (`get_unreal_output_logs` substring `SetBotEnabled`: 4 matches = 2 controller lines + this + the enabled line) | pass |
| P2 | NO `LogSiegeBot` decision line between the disabled line and the re-enable call (≥120 s) | every `LogSiegeBot` line, head-ordered | between 12:25:33.163 and 12:27:55.761 (142.6 s wall / 137.7 s arena): **0** lines of any kind on `LogSiegeBot`. The full ordered list for PIE 1 is: deck-select, disabled, enabled, then 11 `Rule 4 (Attack)` lines from 12:27:56.127. Corroborated in-batch: the bot's `DiscardPile` stayed empty and its `Hand` and 44-entry `DrawPile` byte-identical at t=0.017, 22.76, 59.50, 85.02; `DiscardPile` empty at 134.95 and 137.71 | pass |
| P3 | the Red census never rises above C0 across C1..C4 | Red `SummonedUnit`+`Building` (Python, team split) and the in-batch class census | C0 = no unit/building class at all (t=0.017). C1′ Python t=33.07: Red 0 / 0. In-batch class census at 22.77, 59.52, 85.04, 134.97: no `BP_Unit_*` / `BP_Building_*` class; at 137.72 only `BP_Unit_Footman_C` 1, which is Blue (my P5 play; Python at 185.3 reads it `TeamId.BLUE`, the log names it the player's). Never above C0 | pass |
| P4 | Red gold rises monotonically G0 < G1 < G2 < G3 < G4 | `SiegeBotController0` `PlayerState.Gold` in-batch | **10 (0.017) < 32 (22.74) < 69 (59.48) < 94 (85.00) < 144 (134.93)**; intermediate reads 43 (33.07), 57 (47.47), 123 (114.01), 147 (137.69) all rising | pass |
| P5 | control on the census instrument: play ONE unit card ≈+60 s; Blue census +1 with its play line | `IA_Card1` + ghost read + `simulate_key_press LeftMouseButton`; `BP_Unit` class census; Blue `DrawPile`/`DiscardPile`; the play line | **Try 1, t=59.57–59.70: refused.** Ghost entered (`placement mode entered for card 'Footman' (cost 9).` 12:26:35.271), pre-press ghost read `StaticMeshActor_34`, `bHidden false`, `(X=-26379.999951,Y=62.697410,Z=362.376606)` — the unset cursor traced into the Blue castle gate — and the press logged `placement click refused for 'Footman' — no ground hit, outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh …` (12:26:35.440); gold 69→70, draw pile unchanged, class census 0. **Try 2, t=135.22: placed.** After `inject_input_action IA_Look {x:0, y:1, hold_seconds:0.4}` (t=113.15; control pitch → 302.5, camera pitch −57.5) the ghost read `bHidden false` at `(X=-21099.213464,Y=5.777367,Z=0.000000)` at t=135.20 (the step immediately before the press); press t=135.22 → `played card 'Footman' for 9 gold — spawned 'BP_Unit_Footman_C_0' at (-21099, 6, 92).` (12:27:53.342); `GhostActor` → `None` (135.55); Blue gold 145 → 136; class census `BP_Unit_Footman_C` **0 → 1** (59.52 / 85.04 / 134.97 → 135.58); Blue `DiscardPile` empty → non-empty. Frames: hero-alive (no footman) `…-t02m15s-armA-hero-alive-bot-off.png` vs footman placed `…-t02m15s-armA-blue-footman-placed.png`. The census instrument moved. **Deviation:** the play landed at +135 s, not ≈+60 s (try 1 was at +59.7) | pass |
| P6 | at +120 s call `SetBotEnabled(true)`: `bot enabled` prints ONCE; within 30 s a decision line AND Red census C5 > C4 | the enabled line count; the first `Rule` line; class census + Python team census | call 2 at t=137.74. `bot enabled by SetBotEnabled` count **1** (12:27:55.761). First decision line **0.366 s** later (12:27:56.127, `Rule 4 (Attack): played unit 'Cavalry' …`); 9 decision lines inside the following 30 s wall (12:27:56.127 … 12:28:14.417). Red census: class census at 142.61 (4.87 s after the call) `BP_Unit_MilitiaMob_C` 4 + `BP_Unit_Cavalry_C` 1 + `BP_Unit_Knight_C` 1 beside the one Blue footman (C4 = 0 Red ⇒ C5 ≥ 6); bot `DiscardPile` non-empty; Red gold 147 → 98. Team-split Python at 185.30: Red 19 `SummonedUnit`. Frame `…-t03m09s-armA-red-cavalry-after-reenable.png`; the frame call sampled `BP_Unit_Cavalry0` `Team: Red`, `CardID: Cavalry` | pass |
| P7 | the hero is alive at +120 s | `SiegePlayerController0` `Pawn` + `Pawn.CurrentHP` | `Pawn` = `BP_HeroCharacter_C_0`, `CurrentHP` **200.0** at t=135.08 (+135 s after the disable); also 200.0 at 0.017, 22.79, 59.55, 85.05 and at 165.76 (28 s after re-enable). Frame `…-t02m15s-armA-hero-alive-bot-off.png` shows the hero standing on grass with the green placement ghost beside him | pass (recorded; property Jonathan is buying holds) |
| ARM B | the control: same entry, no call — a decision line within 30 s of t0 AND the Red census rises above its t0 value within 30 s | PIE 2 log + in-batch class census + Python team split | t0 `StartMatch` 12:29:40.317 (deck select 12:29:40.439). No `SetBotEnabled` line in PIE 2. First decision line `[2026.10.04-12.29.54:707][948]LogSiegeBot: [Bot SiegeBotController_0] Rule 2 (Economy): played Miner 'Miner' (cost 24) toward mine 'GoldNode_5' at (23410, 7340, 44) — miners now 1/3, gold 24->0.` = **+14.4 s** after t0. Class census `BP_Unit` 0 classes at t=2.76, 7.78, 12.82 → `BP_Unit_Miner_C` 1 at **t=17.74** (and 22.77, 27.81, 32.07); Red gold 12, 17, 22 → **3** at 17.72; `bBotEnabled` true at 2.75 and 32.05. Python t=37.40: `SummonedUnit <TeamId.RED: 1> n=1: BP_Unit_Miner_C_0`; the frame call sampled `BP_Unit_Miner0` `Team: Red`, `CardID: Miner`. **Fired** | pass (control discriminated) |

Verdict derivation (`VER-§1` cl. 5): no `fail`; P1–P7 and ARM B `pass` ⇒ `VERIFIED`. The row's own rule ("`VERIFIED` only with P1–P6 all pass and ARM B fired") is met.

## Evidence (promoted)

⛔ **Promotion owed to the 5c host** — I hold no shell and `Write` cannot copy a PNG, so none of the paths below exists yet. The source PNGs are on disk in `Saved/AuraVerify/TASK-1603/`; each was `Read` by me in this run.

| promote to `.claude/pipeline/playtest-evidence/2026-10-04/` | from `Saved/AuraVerify/TASK-1603/` | one-line pixel description |
|---|---|---|
| `VER-TASK-1603-t02m15s-armA-hero-alive-bot-off.png` | `a1-hero-alive-bot-off_t135.10s_f41562.png` | the white-cloaked hero standing on grass, a translucent green placement ghost at his left shoulder, no unit beside him, the castle wall's dark edge across the top, VRAM banner across the middle (533.645 MB) |
| `VER-TASK-1603-t02m15s-armA-blue-footman-placed.png` | `a1-blue-footman-placed_t135.60s_f41581.png` | the same view 0.5 s later: the green ghost is gone and an opaque blue-hooded footman stands where it was, at the hero's left shoulder (469.645 MB banner) |
| `VER-TASK-1603-t03m09s-armA-red-cavalry-after-reenable.png` | `a1-red-units-after-reenable_three_quarter_t189.68s_f44658.png` | three-quarter view of a red-armoured rider on a dark horse galloping across open grass with scattered tufts; no banner (scene-capture camera) |

Positive control for the frame reader: the first two frames share camera and hero pose and differ in exactly one region — a translucent green ghost in frame 1, an opaque blue footman in frame 2 — so the reader returned two different images, each matching its stamp's reads (`GhostActor` live at 135.20; `None` + census 1 at 135.55–135.58).

Looked at, not promoted: `a1-after-blue-play_t60.07s_f37430.png` (the hero facing the castle gate, no ghost visible — try 1's trace target) and `b-control-red-miner_three_quarter_t42.31s_f49499.png` (ARM B: the frame is filled by a gold-crystal mine mesh; the miner is not identifiable by eye — the reads carry ARM B, not this frame).

## Recording (`VER-§12` cl. 7b — raw `.h264`, the orchestrator remuxes)

`stop_pie_recording` returned `success: false` — "No active or completed recording." — after PIE 1's stop; every film below was found by directory listing (sizes and last-write UTC read by the read-only lane):

- PIE 1 menu film, auto-armed by `start_pie`: `Saved/AuraVerify/rec_1791116719565458300_1/recording.h264` (967,545 B; index game_time 0.40 → 8.94 s, 217 frames — the menu only; a third sighting of `HYPOTHESIS` H3, no control).
- **PIE 1 arena film (ARM A), auto-started by the in-batch `record_burst`:** `Saved/AuraVerify/rec_burst_639267135328030000/recording.h264` (24,585,904 B; index game_time 0 → 119.72 s, 3,228 frames, last write 12:27:37.9Z). It ends at arena t=119.7, before the P5 play (135.2) and the re-enable (137.7).
- `Saved/AuraVerify/rec_1791116872880681400_5/recording.h264` (7,192,905 B; index game_time 0 → 30.83 s, 853 frames; last write 12:28:25.0Z). Who started it is not known; `HYPOTHESIS`: by its write time it covers the re-enable window (12:27:55Z →), and its index restarts game_time at 0.
- PIE 2 menu film: `Saved/AuraVerify/rec_1791116971730783600_6/recording.h264` (634,644 B).
- **PIE 2 arena film (ARM B):** `Saved/AuraVerify/rec_burst_639267137833180000/recording.h264` (8,279,199 B).

No `.mp4` was seen for any of them.

## Save hygiene (`SC-§125`)

All five `.sav` files, before PIE 1 and after PIE 2 stopped — sha256, size and mtime (UTC) identical:

| file | bytes | mtime UTC | sha256 |
|---|---|---|---|
| `SiegeAccounts.sav` | 3083 | 2026-08-29T05:43:52.307531 | `2fd96fe1…54d442b1` |
| `SiegeDecks.sav` | 3814 | 2026-08-02T17:09:34.952297 | `646d442f…4039a071` |
| `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | 6617 | 2026-09-29T22:55:02.377213 | `49dce23c…e8e5dab0` |
| `SiegeSettings.sav` | 2004 | 2026-08-05T05:24:38.554086 | `c8555088…cc06f2e7` |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | 2056 | 2026-09-09T20:48:19.329635 | `684ae64f…eeb41793` |

Net zero, proven by both. `ActiveDeckName` read `deck8` before; unchanged file ⇒ unchanged.

## Hypotheses (not verdicts)

- H1 — why try 1 refused and try 2 placed: the unset OS/Slate cursor traced into the castle gate from the spawn camera (pitch 0, ghost at (−26380, 63, 362)); pitching the camera down with `IA_Look` moved that same screen point's trace onto the grass beside the hero, inside the Blue spawn box. **MECHANISM NOT MEASURED: the cursor position was not read**, and whether Jonathan's mouse sat over the PIE window is unknown.
- H2 — the arena film's end at game_time 119.72 is a cap of the burst-started film, not a crash. Not measured.
- H3 — `rec_…_5` covers the re-enable window (above). Not measured.
- The 0.366 s from the enabled line to the first play fits the handoff's design (the decision timer kept ticking; the gate re-reads the effective state each tick). Mechanism read from the handoff (§2, §7), not measured here.

## Not examined / limitations this run

- **`VER-§7` cl. 2 declaration — `pie_scene_edit` reached through `run_verification_sequence`, not granted.** Op `call_actor_function` ONLY, **2 invocations, 1 op each**, target actor `SiegePlayerController0` (class `SiegePlayerController`, found by class; its object name in the log `SiegePlayerController_0`), function `SetBotEnabled`: (1) args `{"bEnabled": false}` at arena t=0.017 (reply `applied: 1, failed: 0`, `"ok": true`); (2) args `{"bEnabled": true}` at arena t=137.74 (same reply). NO `spawn_actor`, `delete_actor`, `set_actor_property`, `set_actor_transform`, `set_actor_enabled`, `teleport_player`, and NO `SetMouseLocation`. The `bBotEnabled` readback (false at 0.017, true at 137.76) is corroboration only, never the verdict.
- **Other inputs sent (all on my granted surface, none a `pie_scene_edit`):** `inject_input_action` `IA_MenuAccept` ×2 (one per PIE), `IA_Card1` ×1, `IA_Look {x:0,y:1,hold_seconds:0.4}` ×1; `simulate_key_press LeftMouseButton` ×2 (the refused try-1 press and the placing try-2 press; placement mode stayed open between them, `GhostActor` `StaticMeshActor_34` throughout); `record_burst` ×2; `capture_pie_frame` ×5. `load_level /Game/Maps/L_MainMenu` ×1 before PIE.
- **Census partial** (`SC-§118` cl. 9): PID and command line of my own editor read; the process list was not (subprocess barred).
- **`VER-§8` cl. 2 declarations made at boarding:** the Shipping no-op (`#if !UE_BUILD_SHIPPING` bodies) is not observable in PIE and is `TASK-1601`'s, text-level; `-ExecCmds="siege.BotEnabled 0"` at launch is not driven (the verifier launches nothing) and stays unmeasured.
- Also not exercised: the console variable `siege.BotEnabled` at runtime, the console `SetBotEnabled 0/1` exec, `USiegeCheatManager::SetBotEnabled`, a same-value repeat call (the "prints nothing" half of the once-per-transition rule), the no-bot Warning path (Sandbox / pre-spawn), and `ResetBot` / Play Again persistence.
- **P5 timing deviation:** the placing play landed at +135 s, not ≈+60 s (try 1 at +59.7 was refused, above). **Disable timing deviation:** call 1 at arena t≈0.02, not t≈5 (above). **Checkpoint timing:** +22.7 / +59.5 / +85.0 / +134.9 instead of +30 / +60 / +90 / +120.
- The team-split census is a separate read-only Python call, so its two readings (t=33.07, 185.30) carry their own stamps and are not inside the SET's batch; inside the batches the Red rise is shown by the class census, the bot's `DiscardPile`, Red gold and the log. Castles (`ACastle`) are outside both census classes.
- `get_unreal_output_logs` cannot isolate one PIE session by session id; sessions are separated by their `StartMatch` stamps.
- The third ARM-A sequence's reply (447,439 characters) overflowed the tool limit and was read by regex extraction from the saved reply file; the bot's array values after t=137 were read only as empty / non-empty.
- Aura plugin version not read.

## Recipes used

- `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` · Step 1 rows 1–4 only (`ui_snapshot Button_0` on `WBP_MainMenu`, `IA_MenuAccept`, `wait_pie_seconds 3`, in-batch `record_burst {"seconds":1}`), in both PIE sessions; no walk, capture or summon · re-verified in-run **y**: `Button_0` `focused: true`, text `"Play (vs Bot)"` (menu t=8.96 and 7.83); the travel landed (`UEDPIE_0_L_Arena`); the burst auto-started an arena film both times (`auto_started_recording: true`). Mismatch noted: after the `wait 3` the arena clock read t=0.017 (PIE 1) and 2.73 (PIE 2), so the wait straddled the travel. Outside the recipe's Precondition 2 (deck8 = 50× `Footman`, not deck4).
- `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` · Step 2 (hand read), Step 3 (`IA_Card1` entry, `wait 0.2`), the mandatory pre-press ghost read, the `LeftMouseButton` confirm and its post-reads, the `DrawPile`/`DiscardPile` read for a one-card deck · re-verified in-run **partly**: entry measured (`None` → `StaticMeshActor_34` + entry line); the un-aimed confirm was REFUSED once (ghost visible but on the castle gate) and PLACED once after an `IA_Look` pitch — both outside the recipe's aim fence (no `SetMouseLocation`; the declaration forbids it here). The pre-press read was the step immediately before each press, as the recipe requires.

## Recipe candidates

Taker (`SC-§50`): the manager mints the `RCP-vsbot-bot-disabled-run.md` row when this report lands (the row's own instruction).

1. **`RCP-vsbot-bot-disabled-run.md`** — "Play vs Bot, `SetBotEnabled(false)` by `call_actor_function` on the player controller, then a long-horizon capture with no hero-death clock; `SetBotEnabled(true)` to hand the match back."
   - Preconditions: editor world `/Game/Maps/L_MainMenu` (`load_level`), PIE standalone 1 client; Development build.
   - Steps (one `run_verification_sequence`): `ui_snapshot` `WBP_MainMenu` `Button_0` → `inject_input_action /Game/Input/Actions/IA_MenuAccept.IA_MenuAccept` → `wait_pie_seconds 3` → `record_burst {"seconds":1}` → reads (`SiegeBotController` `PlayerState.Gold`, `PlayerState.Team`, `bBotEnabled`; `component: DeckComponent` `Hand`/`DrawPile`/`DiscardPile`) → `pie_scene_edit {"operations":[{"op":"call_actor_function","name":"SiegePlayerController0","function":"SetBotEnabled","args":{"bEnabled":false}}]}` → `get_actor_property_in_pie SiegeBotController ["bBotEnabled"]` (read `false`). Later batches: any waits/reads; the hand-back is the same op with `{"bEnabled": true}` followed in-batch by the class census and the bot's `DiscardPile`.
   - Read-back: one `bot disabled by SetBotEnabled` line; zero `LogSiegeBot` lines while off; Red gold rising ≈+1/s (10 → 147 over 137.7 s); hero `CurrentHP` 200 at +135 s and still at +166 s; on hand-back one `bot enabled …` line and a `Rule` line within 0.4 s.
   - Measured timings: call landed at arena t=0.017 (the `wait 3` straddles the travel); the bot's first play in the control run came at +14.4 s, so this placement precedes it.
   - Fences: one run; `L_Arena` vs bot from the menu; curated bot deck `Bot Aggro Rush`; player deck8; the hero undriven at the spawn; no cvar or console path; the actor label `SiegeBotController` resolved by substring to `SiegeBotController0`; the switch's effect on Play Again not measured.
   - Hazard: after hand-back the bot plays ≈9 units in 30 s against an undriven hero — the hero-death clock restarts.
2. **A non-`SetMouseLocation` route onto legal ground from the Blue spawn** (for the play recipe's pending aim row): at the spawn camera (pitch 0, yaw 180) the unset cursor traced to (−26380, 63, 362) and the confirm was refused; after `inject_input_action IA_Look {"x":0,"y":1,"hold_seconds":0.4}` (control pitch 302.5) the ghost read (−21099.2, 5.8, 0) and the confirm placed `BP_Unit_Footman_C_0` at (−21099, 6, 92). One run; the cursor position was not read (H1), so it is not yet an aim.
