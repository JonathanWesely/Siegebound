# RCP-vsbot-capture-center-and-summon — start a vs-bot match from the menu, walk the hero into `CaptureZone_Center`, capture it, and summon a Unit card inside it, in ONE batch

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (the run; **a2** = its attempt 2, the measured route; **a1** = its attempt 1, the measured failure) · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md` (Precondition 2 only).

> ⚠️ **Read this first.**
> - **The whole route is ONE `run_verification_sequence`:** menu → level travel → walk → capture → card entry → confirm → post-reads.
>   - a1 split it across 3 calls. Its round trips (≈20 s and ≈36 s of PIE clock) put the play at t≈83 s, after the hero had died, and the summon was refused. a2 ran it as one 48-action sequence and summoned at t=33.80. `[M: 1512 Attempt 1, Speed data]`; `[L: VER-§13 cl. 1, 2026-09-26 ruling]`
> - **There is NO aim step. The one measured summon placed wherever the unset cursor traced.** Why the cursor traced into the zone is `HYPOTHESIS` H1 and not reproducible. The in-batch ghost read taken **before** the confirm documents where the confirm will place. The batch cannot branch on that read, so it is judged after the batch returns.
> - **The control is the no-play interval.** The omitted-set confirm is **not** a control here: in a2 it placed the unit. `[M: 1512 A3, ctl]`
> - **Measured once:** one run, one summon.

## Source

- `.claude/pipeline/qa/TASK-1512-verify.md`, `Verdict: VERIFIED`, run **2026-09-26**, 2 attempts (2 PIE sessions).
  - *Editor/Aura state* · *Recording* · *Pre-registration* · *Acceptance lines* rows **A1**, **A2**, **A3**, **ctl** · *Attempt 1* · *Evidence (promoted)* · *Hypotheses* H1–H3 · *Not examined / limitations* · *Save hygiene* · *Speed data* · *Recipes used* · *Recipe candidates* (steps 1–6 + the Fences line).
- The law:
  - `VER-§13` cl. 1, 2026-09-26 ruling: one tool call per batch, required whenever a batch mixes tools; it cites a1 vs a2 as the measured cost.
  - `VER-§12` cl. 7b amendment: a film armed before `start_pie` may not survive a level travel; arm a recorder after it.
  - `VER-§12` cl. 7c (a parameter missing from the schema is not an absent capability) and cl. 7e (the VRAM banner).
  - `VER-§8` cl. 10(b): the `t≈60 s` fence, and the round-trip budget (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`; Step 4).
  - `VER-§7` cl. 2: the grant declaration, if a run adds `pie_scene_edit`.
- Recipes it touches:
  - `RCP-menu-to-deckbuilder.md` Step 0: the focus read. a2 used only that step. `[M: 1512 Recipes used]`
  - `RCP-play-unit-card-from-hand.md`: the entry and the confirm. `1512` used it in part and outside its aim fence; its control arm is amended by `TASK-1516`.
  - `RCP-deckbuilder-set-active-by-keyboard.md`: makes the all-Unit deck active.
- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26. ~~Not re-verified since; a2 is the only run.~~ **Used in part 2026-09-27 by `TASK-1493`**, re-verified in-run **y** for **Step 1 rows 1–4 only** ("the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, in-batch `record_burst {"seconds":1}`"): `Button_0` read `focused: true`, "Play (vs Bot)" at menu t=3.85; the travel to `L_Arena` landed (the game world read `UEDPIE_0_L_Arena` at t=6.02); the burst auto-started the arena film. "No walk, capture or summon was done." `[M: qa/TASK-1493-verify.md Recipes used]` Every other step and row has not been re-verified since; for them a2 is still the only run. The report states no run date: 2026-09-27 is the date of the marker it ran after, `TASK-1493-NOT-SETTLED-BY-1491-2026-09-27` (its line 10). Recorded by `TASK-1554`, 2026-09-27.
- Amended by `TASK-1527`, 2026-09-26, on `qa/TASK-1518.md` W1–W3 and N4–N7 (W2: option (b), row 15a), and Precondition 2's wording (the active deck is read, never assumed).
- Amended by `TASK-1533`, 2026-09-27, on `qa/TASK-1528.md` Q2: row 15a produces the `DrawPile` half of "the card left the hand" (row 15a, Step 4).

## Plugin version

**Not read.** `1512` *Not examined*: "Aura plugin version not read." ⇒ the first use re-verifies in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).

## Preconditions

1. Editor world `/Game/Maps/L_MainMenu`; PIE standalone, 1 client, viewport **1280×725**, **DPI 0.6706**; `is_pie_active` = `false` before the start. `[M: 1512 Editor/Aura state]`
2. **The active deck must be an all-Unit, single-card, legal deck: deck4 = 50× `Archer`.** `1512` read it on disk before PIE: `ActiveDeckName = deck4`. `[M: 1512 Pre-registration]`
   - A hand dealt from one Unit card has no type exposure (`RCP-play-unit-card-from-hand.md` Precondition 8).
   - To make deck4 active, use `RCP-deckbuilder-set-active-by-keyboard.md`: `1511` set it and left it active, and `1512` read it there. A later run's pre-registration read a different deck active `[M: 1524 Pre-registration]`, so Step 0's read decides, never this line.
   - Any other deck: `NOT MEASURED`.
3. **A fresh PIE session on the main menu, with `Button_0` "Play (vs Bot)" focused.** a2 read that focus inside the batch (Step 1, row 1). `[M: 1512 A1]`
4. **Gold needs no wait.** From t=2.8 to 33.6, gold rose 12 → 43 `[M: 1512 ctl]`. The Archer costs 12, per the entry line `placement mode entered for card 'Archer' (cost 12).` `[M: 1512 A3]` Gold at the card key was 43 (t=33.79). `[M: 1512 A3]`
5. **Recording.**
   - a2 armed a film before `start_pie`. It held the main menu only (2.54 MB). `HYPOTHESIS` H3: it ended at the level travel. `[M: 1512 Recording, H3]`
   - The arena was filmed only because an in-batch `record_burst` after the travel auto-started a new film (Step 1, row 4). `[M: 1512 Recording]`; `[L: VER-§12 cl. 7b amendment]`
6. **Save hygiene:** the orchestrator hashes the `.sav` files before and after. Expected: no change. `1512` observed none (all ten decks identical, `ActiveDeckName = deck4` both times). `[M: 1512 Save hygiene]`
7. Property paths are the ones a2 used. Address the player's controller by its exact label, `SiegePlayerController0`: the bot's controller also carries a `DeckComponent` (`RCP-play-unit-card-from-hand.md` Precondition 5, `[D]`).

## Steps

**Step 0 — before PIE (no PIE call).** Read the save (`load_game_from_slot("SiegeDecks_<profile id>", 0)` through the read-only editor Python lane) and write the expected values into the report. `[M: 1512 Pre-registration, Save hygiene]` Then `start_pie`. It cannot go in the batch: "PIE LIFECYCLE IS NOT IN THIS TOOL" `[S]`.

**Step 1 — THE batch: ONE `run_verification_sequence`.** The rows are in the order a2 ran them. Each row cites what measured it.

| row | action | source |
|---|---|---|
| 1 | `ui_snapshot` of `Button_0` on `WBP_MainMenu` | a2 menu t=20.42 → `Button_0` `focused: true`, text `"Play (vs Bot)"` `[M: 1512 A1; Recipe candidates step 1]` |
| 2 | `inject_input_action IA_MenuAccept` | menu t=20.44 → world `L_Arena`; the arena clock restarts `[M: 1512 A1]` |
| 3 | `wait_pie_seconds 3` | `[M: 1512 Recipe candidates step 2]` |
| 4 | `record_burst {"seconds": 1}` | the arena film was "auto-started by an in-batch `record_burst` at arena t=2.69 s" `[M: 1512 Recording]` |
| 5 | reads: `CaptureZone_Center` (`CaptureOwner`, `ZoneHalfExtent`, `RootComponent.RelativeLocation`); `DeckComponent` `Hand`/`DrawPile`; `PlayerState.Gold`, `GhostActor` | t=2.73–2.74: `Neutral`; `ZoneHalfExtent (X=840.000000,Y=840.000000)`, `RelativeLocation (X=0,Y=0,Z=0)`; `Hand` = 6× `"Archer"`, `DrawPile` = 44× `"Archer"` `[M: 1512 A1, A2; Recipe candidates step 3]` |
| 6 | `inject_input_action IA_Sprint`, `hold_seconds 29` + `inject_input_action IA_Move`, `x 0, y -1, hold_seconds 28.5` | "Sprint+Move 2/2 (X −21007.8 → 250.9, velocity 750)" `[M: 1512 Recipe candidates step 4; Speed data]` |
| 7 | `wait_pie_seconds 14`, transform read, `CaptureOwner` read | t=16.79: hero X=−10710.2, velocity 750; `Neutral` `[M: 1512 A2; Recipe candidates step 4]` |
| 8 | `wait_pie_seconds 15.5`, transform read | hero stopped at X=250.89, Y≈0 "since t≤32.31"; `CurrentHP 200` at t=32.33 `[M: 1512 A2]` |
| 9 | `wait_pie_seconds 1.2`, `CaptureOwner` read | **`Blue` at t=33.54** `[M: 1512 A2; Recipe candidates step 5]` |
| 10 | pre-reads: `Hand`, gold, `GhostActor`; `survey_pie_scene` census filtered to `Archer` | `GhostActor` `None` (t=33.58); `BP_Unit_Archer_C` census 0 (t=33.59); gold 43 `[M: 1512 A3; Recipe candidates step 6]` |
| 11 | `inject_input_action IA_Card1` | t=33.61 `[M: 1512 A3]` |
| 12 | `wait_pie_seconds 0.2` | `[M: 1512 Recipe candidates step 6]` |
| 13 | ghost reads: `GhostActor`, `GhostActor.bHidden`, `GhostActor.RootComponent.RelativeLocation` | t=33.79: `StaticMeshActor_34`, `bHidden false`, `RelativeLocation (X=-449.150572,Y=-1.707544,Z=0)`. The candidate says "this dotted path WORKS; it was measured non-null this run" `[M: 1512 A3; Recipe candidates step 6]` |
| 14 | **CONFIRM:** `simulate_key_press LeftMouseButton` | t=33.80 → placed (log below) `[M: 1512 A3]` |
| 15 | post-reads: gold, `GhostActor`, census | gold 32 (t=34.05); census 1 (t=34.09): `BP_Unit_Archer0` at `(-382.86, -1.58, 92)`; `GhostActor` → `None` `[M: 1512 A3]` |
| 15a | post-play `DeckComponent` read, `["DrawPile"]`: the in-batch producer of the `DrawPile` half of "the card left the hand" (`qa/TASK-1518.md` W2, option (b); `qa/TASK-1528.md` Q2) | the read object is row 5's, measured in-batch at t=2.74 (`DrawPile` = 44× `"Archer"`) `[M: 1512 A1]`. **Its position here, after the play: `NOT MEASURED`.** a2 read `DrawPile` 43 after the play only at t≈55, outside the batch (Step 4) `[M: 1512 A3]` |
| 16 | control-interval reads (see the control below) | `CaptureOwner` `Blue` at t=34.59 and 35.30; census 1 at t=35.28 and 37.43; the batch ended at arena t=37.45 `[M: 1512 A2, ctl, Speed data]` |

The action objects, copy-paste ready (rows 1–15, then row 15a last):
```json
{"actions": [
  {"type": "ui_snapshot", "params": {"widget": "WBP_MainMenu", "selector": {"by": "name", "value": "Button_0"}, "max_depth": 2}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 3}},
  {"type": "record_burst", "params": {"seconds": 1}},
  {"type": "get_actor_property_in_pie", "params": {"name": "CaptureZone_Center", "properties": ["CaptureOwner", "ZoneHalfExtent", "RootComponent.RelativeLocation"]}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "component": "DeckComponent", "properties": ["Hand", "DrawPile"]}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["PlayerState.Gold", "GhostActor"]}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_Sprint.IA_Sprint", "hold_seconds": 29}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_Move.IA_Move", "x": 0, "y": -1, "hold_seconds": 28.5}},
  {"type": "wait_pie_seconds", "params": {"seconds": 14}},
  {"type": "get_player_transform", "params": {}},
  {"type": "get_actor_property_in_pie", "params": {"name": "CaptureZone_Center", "properties": ["CaptureOwner"]}},
  {"type": "wait_pie_seconds", "params": {"seconds": 15.5}},
  {"type": "get_player_transform", "params": {}},
  {"type": "wait_pie_seconds", "params": {"seconds": 1.2}},
  {"type": "get_actor_property_in_pie", "params": {"name": "CaptureZone_Center", "properties": ["CaptureOwner"]}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "component": "DeckComponent", "properties": ["Hand"]}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["PlayerState.Gold", "GhostActor"]}},
  {"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_Card1.IA_Card1"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.2}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["GhostActor", "GhostActor.bHidden", "GhostActor.RootComponent.RelativeLocation"]}},
  {"type": "simulate_key_press", "params": {"key": "LeftMouseButton"}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["PlayerState.Gold", "GhostActor"]}},
  {"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "component": "DeckComponent", "properties": ["DrawPile"]}}
]}
```
What in these objects is quoted and what is not:
- **Quoted from `1512`:**
  - the `ui_snapshot` object, the `IA_MenuAccept` path, `record_burst {"seconds":1}` and the `survey_pie_scene` object;
  - the property names (`CaptureOwner`, `ZoneHalfExtent`, `RootComponent.RelativeLocation`, `Hand`, `DrawPile`, `PlayerState.Gold`, `GhostActor`, `GhostActor.bHidden`, `GhostActor.RootComponent.RelativeLocation`), the actor label `CaptureZone_Center`, and `DeckComponent`;
  - the hold values (`29`, `28.5`, `x 0`, `y -1`), the waits (3, 14, 15.5, 1.2, 0.2), `IA_Card1`, and `simulate_key_press LeftMouseButton`.
- **`[D]`:** the full paths of `IA_Sprint`, `IA_Move` and `IA_Card1`. `1512` names the actions only; `Content/Input/Actions/IA_Sprint.uasset`, `IA_Move.uasset` and `IA_Card1.uasset` are the only assets with those names on disk. The `IA_Card1` path is also quoted by `qa/TASK-1230-verify.md` row 1 (see the play recipe).
- **`[S]`:**
  - the envelope and every param name (`hold_seconds`, `x`, `y`, `key`, `class_filter`, `include`);
  - "non-blocking: the call returns at once and the hold persists across your following actions/waits" (`inject_input_action`'s schema). That agrees with a2's stamps: the walk ran from t≈2.8 to about t=32 while the sequence's waits advanced.
  - The `component`/`properties` keys are not in the sequence's published parameter union; measured use says they are accepted inside a sequence. `[L: VER-§12 cl. 7c]` `name` is in the validator's own "Valid params" list, `['client_index', 'component', 'name', 'properties']`. `[L: VER-§8 cl. 10(c)]`
- **`NOT MEASURED`:**
  - **The transform reader.** `1512` reports position and velocity but names neither the reader nor the field. `get_player_transform` is on the sequence whitelist and returns location and velocity by its schema `[S]`. It is not quoted as a2's reader. If the sequence validator rejects it, drop both `get_player_transform` objects; the walk does not depend on them (`handoffs/TASK-1515-programmer.md` flag 1; `qa/TASK-1518.md` N4).
  - **Row 15a's position.** The `DrawPile` read object is row 5's, measured in-batch at t=2.74 `[M: 1512 A1]`. No run has read it after the play inside a batch.
  - **The `CurrentHP` read** (200 at t=32.33): the property name is quoted; the actor label and the reader are not.
  - **The `CaptureOwner` read at row 7.** a2 read it at t=16.79; whether the read shared row 7's slot in the batch is not stated.
  - **A wait between the confirm and the post-reads.** The press went in at t=33.80 and the post-reads landed at t=34.05 (gold) and t=34.09 (census). Whether a wait sat between them is not quoted, so none is written above.
- **Not reproduced from a2:** a2's 48 actions also held two `pie_scene_edit` `SetMouseLocation` calls (t=34.10, 34.35), a second `LeftMouseButton` (t=34.72) and two frame captures. The two `SetMouseLocation` calls aimed nothing: placement mode had already exited. `[M: 1512 A3, Not examined]` This recipe drops the two calls; the second press belongs to the control. The action count here therefore differs from 48.

**Step 2 — after the batch returns: judge the ghost read (row 13) BEFORE reading anything else.**
- The confirm places wherever the ghost is. `[M: 1512 Recipe candidates step 6]`
- **Inside the zone** ⇔ |X − 0| ≤ 840 **and** |Y − 0| ≤ 840, using the zone box read at row 5. `[M: 1512 A2]`
  - `[D]`: `ACaptureZone::IsPointInZone` is an axis-aligned XY box about `GetActorLocation()`: `FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X && FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y`. Z is ignored.
  - a2 read `RootComponent.RelativeLocation`, not the actor location. That the two agree assumes an unattached root: `NOT MEASURED`.
- **A legal zone spawn also needs `CaptureOwner` = the player's team** (`CanTeamSpawnHere`: inside the box AND the owner matches) `[D: CaptureZone.h]`. `1512`'s pre-registration states the same expectation. `[M: 1512 Pre-registration]`
- a2: ghost at (−449.15, −1.71) ⇒ inside, and `Blue` at t=33.54, the last owner read before the press. `[M: 1512 A2, A3]`
- **If the ghost read is outside the zone, `None`, or hidden,** the confirm is not an in-zone summon, whatever it did. Report what it did and stop: the run has measured nothing about this route. A batch cannot branch, so the press has already been sent (`README.md` common hazards).

**Step 3 — the no-play control: an equal interval with no card key.** `[M: 1512 ctl]`
- **The measured control:** from t=34.05 to t=54.60 (≈20.5 s). It included a second `simulate_key_press LeftMouseButton` at t=34.72 with `GhostActor` `None`. Over it:
  - the `Archer` census stayed **1** (t=35.28, 37.43; the unit walked X 35.69 → 319.4 → 531.93);
  - `DrawPile` stayed **43** (read at t≈55, by Step 4's call; the tail below has no `DrawPile` read);
  - gold went 32 → 33 → 35 → 52, **only rising**;
  - no second `played card` line appeared.
- **The pre-play half:** from t=2.8 to 33.6 gold rose 12 → 43, the census read 0 at t=33.59, and the hand did not change. `[M: 1512 ctl]`
- **Where the reads ran:** the interval's reads up to t=37.43 were inside the batch, which ended at t=37.45. Its later reads (t≈54.55–55) came from a separate call `[M: 1512 A2, A3, ctl, Speed data]`. The first of them landed ≈17 s after the batch ended (arithmetic from the stamps). Putting the whole interval inside the batch is `NOT MEASURED`.
- **The in-batch tail:** append these objects to the Step 1 `actions` array, after row 15a. The waits between these reads are not quoted; a2's reads landed at t=34.59, 35.28, 35.30 and 37.43.
  - **The tail's ORDER is this recipe's composition, not a2's.** a2 read `CaptureOwner` at t=34.59, **before** its second `LeftMouseButton` at t=34.72 `[M: 1512 A2, ctl]`; the tail below sends the press first. (`qa/TASK-1518.md` N5)
  - **The interval's length.** a2's in-batch half of the interval ran t=34.05 → 37.45 (≈3.4 s, arithmetic from the stamps) `[M: 1512 ctl, Speed data]`. This tail writes no waits, so its own length is `NOT MEASURED`. The report states the interval's length, from its own stamps, next to the treatment window (the ctl row asks for "an equal interval"). `[M: 1512 ctl]`
```json
[
  {"type": "simulate_key_press", "params": {"key": "LeftMouseButton"}},
  {"type": "get_actor_property_in_pie", "params": {"name": "CaptureZone_Center", "properties": ["CaptureOwner"]}},
  {"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}},
  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["PlayerState.Gold", "GhostActor"]}}
]
```
- ⛔ **Not the omitted-set confirm.** In a2 the press planned as that control placed the unit, because the ghost was already visible at a legal in-zone point. `[M: 1512 A3]`

**Step 4 — the later call (optional, same session).** Everything here is a read; nothing depends on a set (`VER-§13` cl. 2). It is optional because row 15a produces the `DrawPile` half of "the card left the hand" inside the batch (`qa/TASK-1518.md` W2, option (b); `qa/TASK-1528.md` Q2). A 44 at row 15a is not a negative: its position is `NOT MEASURED`, and when the refill happens is unread, so a 44 there sends the run to Step 4. Step 4 adds `DiscardPile`. Step 4 is where a2 measured the values below, and it is the only producer of the control's `DrawPile` leg (Step 3).
- The new unit's `Team` = `Blue` and `CardID` = `Archer` (read t=54.55; reader not named). `[M: 1512 A3]`
- `CaptureOwner` (`Blue` at t=54.60). `[M: 1512 A2]`
- `DrawPile` / `DiscardPile`: 43 / 1, read once through read-only editor Python on the PIE world ("to count `Hand`/`DrawPile`/`DiscardPile` = 6/43/1"). `[M: 1512 A3, Not examined]` The same `DrawPile` read inside the batch after the play is row 15a, at a position `NOT MEASURED`. The pre-play in-batch `DrawPile` read is measured (row 5). `DiscardPile` has no in-batch reader in this recipe.
- Budget: a1's hero died ≈73 s after the arena opened, by log time `[M: 1512 Attempt 1]`. That it held the zone alone from ≈t=50 is `HYPOTHESIS` H2 (Fences). Land this call well inside that.
  - By the law's planning rule this call does not fit inside the `t≈60 s` fence: budget a round trip at ≈70 s (`VER-§8` cl. 10(b), marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`). a2's landed ≈17 s after its batch ended (Step 3). This is why the `DrawPile` half of "the card left the hand" has an in-batch producer (row 15a), and why Step 4 stays optional unless row 15a reads 44 (above; `qa/TASK-1528.md` Q2).

**Step 5 — the log, after the batch.** Category `LogGitClaudeUnrealTest`, except the `StartMatch` line. `1512` does not name its log reader: `NOT MEASURED`.
- `[ASiegeGameMode::StartMatch] Main menu -> opening arena '/Game/Maps/L_Arena.L_Arena' into a fresh match vs the bot` `[M: 1512 Editor/Aura state]`
- `active saved deck 'deck4' is legal (50 cards) — using it this match` · `built a 50-card draw pile from the pending OVERRIDE deck 'deck4' (1 entries)` `[M: 1512 A1]`
- `placement mode entered for card 'Archer' (cost 12).` `[M: 1512 A3]`
- `played card 'Archer' for 12 gold — spawned 'BP_Unit_Archer_C_0' at (-449, -2, 92).` `[M: 1512 A3]`
- Exactly one `played card` line in the session, the play's (t≈33.80), and none after it through the control interval (`1512` ctl: "no second `played card` line"). `[M: 1512 A3, ctl]`

**Step 6 — after PIE.** `stop_pie`, then `stop_pie_recording`. `stop_pie_recording`'s replies overflowed the tool limit in this run ("both `stop_pie_recording` replies"): read the head only. `[M: 1512 Not examined]`; `[L: VER-§12 cl. 7b amendment]` Find every film by directory listing and name it by path `[M: 1512 Recording]`; `[L: VER-§12 cl. 7b amendment]`. Re-read the save: expected unchanged. `[M: 1512 Save hygiene]`

## Read-back

| observable | reader | a2 value |
|---|---|---|
| menu focus before Accept | `ui_snapshot` (row 1) | `Button_0` `focused: true`, "Play (vs Bot)" (menu t=20.42) `[M: 1512 A1]` |
| the match started, on the active deck | world `L_Arena`; `StartMatch` + deck log lines | the lines in Step 5 `[M: 1512 A1, Editor/Aura state]` |
| the hand is all `Archer` | `DeckComponent` `Hand`/`DrawPile` (row 5) | 6× + 44× `"Archer"` = 50 (t=2.74) `[M: 1512 A1]` |
| the zone box | row 5 | half extent (840, 840) about (0, 0, 0) ⇒ X,Y ∈ [−840, 840] `[M: 1512 A2]` |
| the walk | transform reads (rows 7–8). The start value, X −21007.8 at t=2.73, was read by a2 beside the row-5 zone reads; no row of this recipe's JSON takes a transform read before row 7 (`qa/TASK-1518.md` N7) | X −21007.8 (t=2.73) → −10710.2 at velocity 750 (t=16.79) → 250.89, stopped (t≤32.31); `CurrentHP 200` (t=32.33) `[M: 1512 A2]` |
| the capture | `CaptureOwner` (rows 7, 9) | `Neutral` (t=2.73, 16.79) → **`Blue`** (t=33.54) `[M: 1512 A2]` |
| **entry (checked first)** | `GhostActor` `None` → non-`None` + the entry log line | `None` (t=33.58) → `StaticMeshActor_34` (t=33.79), `bHidden false`; `… 'Archer' (cost 12).` `[M: 1512 A3]` |
| **ghost inside the zone before the press** | row 13, judged in Step 2 | (−449.150572, −1.707544, 0): inside `[M: 1512 A3]` |
| gold spent | `PlayerState.Gold` | 43 (t=33.79) → 32 (t=34.05). The log's cost is 12, so the net −11 includes one +1 income tick; the report labels that "an inference" `[M: 1512 A3]` |
| a new Blue Archer inside the zone | `survey_pie_scene` census; the unit's `Team`/`CardID` | 0 (t=33.59) → 1 (t=34.09), `BP_Unit_Archer0` at (−382.86, −1.58, 92); spawn log point (−449, −2); both inside `[M: 1512 A3]` |
| the card left the hand | `DrawPile` (NOT the `Hand` array): row 5 before, row 15a after (in-batch; row 15a's position `NOT MEASURED`). Step 4 re-reads `DrawPile` and adds `DiscardPile` | `DrawPile` 44 (t=2.74, row 5) → 43 and `DiscardPile` 1, both read at t≈55 by Step 4's call `[M: 1512 A1, A3]` |
| placement exited | `GhostActor` | `None` after the confirm `[M: 1512 A3]` |
| control | Step 3 (its `DrawPile` leg: Step 4) | census stayed 1, `DrawPile` stayed 43, gold only rising, no second `played card` line `[M: 1512 ctl]` |

Pixels proved nothing here. In both a2 frames "The Archer is not identifiable by eye"; "The pixels do not prove the summon; the reads and log above do." `[M: 1512 Evidence]`

## Fences — not measured for

- **One run (a2), one summon.** No second run exists, so other days, map states and bot behaviours are untested.
- **The hero alive and the zone uncontested at t≈33 s.** HP was 200 at t=32.33 and the zone read `Blue` from t=33.54 to at least 54.60. `[M: 1512 A2]`
  - In a1 the hero died ≈73 s after the arena opened, by log time, and the ghost pawn spawned at (798, −0, 98), inside the a2 box (arithmetic). The report's own fences sentence says it "died at ≈t=73 holding the zone". `[M: 1512 Recipe candidates Fences; Attempt 1]`
  - **a1 never read the zone `Blue`.** `CaptureOwner` read `Neutral` at t=35.47 and 45.48, and `Red` at t=83.41, 84.57 and 85.28, with Red Cavalry, Knight, Footman and Archer in the census. `[M: 1512 Attempt 1]`
  - That it held the zone **alone, from ≈t=50**, is `HYPOTHESIS` H2 ("it was there from ≈t=50 to ≈t=73"). The killer was not read. `[M: 1512 H2]`
  - Anything later than a2's timeline is `NOT MEASURED` for this route.
- **No aim.** The ghost's in-zone location came from a cursor nobody set. `HYPOTHESIS` H1: the OS/Slate cursor sat near the horizontal centre of the PIE viewport; "MECHANISM NOT MEASURED: the cursor position was not read." `[M: 1512 H1]` An unset cursor is not a reproducible aim. `[M: 1512 Recipe candidates Fences]`
- **A calibrated `SetMouseLocation` into the zone: `NOT MEASURED`.** a2's `(300,420)` and `(640,420)` ran after the placement had exited and aimed nothing. `[M: 1512 Not examined]` The play recipe's `(300,420)` was measured at the Blue spawn from the start camera, not from the zone.
- **The omitted-set control:** invalid whenever the ghost is visible at a legal point. `[M: 1512 A3, Recipe candidates Fences]`
- **Deck, card and key:** deck4 (50× `Archer`) only; `IA_Card1` only; one `ECardType::Unit` card at cost 12. Mixed hands, other cards and other keys: `NOT MEASURED` here.
- **The walk:** from the spawn (−21007.8, 0) at the spawn camera (yaw 180); `IA_Move` `y -1` with `IA_Sprint`; one hold length (28.5 s); straight along Y≈0; stopped at X=250.89. `[M: 1512 A2, Attempt 1, Recipe candidates step 4]` Other start points, cameras, hold lengths and stop points: `NOT MEASURED`. How the camera or hero rotation changed during the walk was not read.
- **`CaptureZone_Center` only**, in a vs-bot match. Other zones and Sandbox (No Bot): `NOT MEASURED` for this route.
- **Films:** the arena film came from the in-batch `record_burst`. That a pre-start film ends at the travel is `HYPOTHESIS` H3. `[M: 1512 H3]`
- **Scope:** `L_Arena` reached from `L_MainMenu` by the menu; standalone single client; the one profile `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34`.
- Plugin version: not read.

## Coordinate/resolution-dependent values

All at viewport **1280×725**, **DPI 0.6706**, standalone, 1 client. `[M: 1512 Editor/Aura state]`

| value | measured at | notes |
|---|---|---|
| hero spawn (X=−21007.8, Y=0) | a2 t=2.73 | a1 read X=−21471 after a −463 uu `IA_Move (0,1)` probe. −21471 + 463 ≈ −21008, which fits the same spawn: arithmetic, not stated `[M: 1512 A2, Attempt 1]` |
| spawn camera yaw 180: `IA_Move (0,1)` = −X, `(0,−1)` = +X | a1 probe; a2 walk | "forward is −X at the spawn camera (yaw 180)" `[M: 1512 Attempt 1, Recipe candidates step 4]` |
| sprint speed ≈750 uu/s | a2 velocity 750 (t=16.79); a1 "X −21471 (t=20.24) → −17851 (t=25.44) → −10341 (t=35.45) → −2828 (t=45.47) ⇒ ≈751 uu/s" | with `IA_Sprint` held `[M: 1512 A2, Attempt 1]` |
| stop point X=250.89, Y≈0 | a2, t≤32.31, after a 28.5 s `IA_Move` hold | inside the zone `[M: 1512 A2]` |
| zone box: half extent (840, 840) about (0, 0, 0) | a2 t=2.73 | ⇒ X,Y ∈ [−840, 840] `[M: 1512 A2]`; eval every 0.5 s (`CaptureEvalInterval = 0.5f`) `[D: CaptureZone.h]` |
| ghost at (−449.150572, −1.707544, 0) with NO cursor set | a2 t=33.79, hero at (250.9, 0), camera looking −X | `HYPOTHESIS` H1; **not an aim value** `[M: 1512 A3, Not examined, H1]` |
| spawned unit (−449, −2, 92) per the log; census (−382.86, −1.58, 92) at t=34.09 | a2 | both inside the box `[M: 1512 A3]` |
| `SetMouseLocation (300,420)`, `(640,420)` | a2 t=34.10 / 34.35 | `applied: 1, failed: 0`; no effect (no placement mode) `[M: 1512 Not examined]` |

Where the confirm places depends on the cursor, the camera and the hero's position. Change any of them and the placement point is unmeasured.

## Known hazards

- **Splitting the route kills it.** a1's round trips cost ≈20 s and ≈36 s of PIE clock, and the play landed at t≈83 on a dead hero: `PlayHandSlot(0) refused — the hero is dead and the ghost cannot play cards (GHOST-§ G-5).` `GhostActor` stayed `None`, and gold and hand did not change. `[M: 1512 Attempt 1]` a2's first post-batch read landed ≈17 s after the batch ended (t=37.45 → 54.55, arithmetic from the stamps). Put every observable the verdict needs inside the batch. `[L: VER-§13 cl. 1; VER-§8 cl. 10(b)]`
- **A batch cannot branch.** The focus read (row 1), the `CaptureOwner` read (row 9) and the ghost read (row 13) only document; the Accept, the card key and the confirm go in whatever they show. Judge after (Step 2).
  - Checking focus first in a separate call is `RCP-menu-to-deckbuilder.md` Step 0. That step was measured on its own (`qa/PLAYTEST-archer50-verify.md` §1(a), t=4.84) and again in-run by `qa/TASK-1511-verify.md` (t=4.89). The extra call is spent on the menu clock, because the arena clock restarts at the travel. `[M: 1512 A1]` Doing it in this route is `NOT MEASURED`.
- **The omitted-set confirm places a unit** when the ghost is visible at a legal point. `[M: 1512 A3]` Never use it as the control here.
- **Someone else's mouse can be the aim.** "If Jonathan's mouse was over the PIE window, the aim came from his cursor." `HYPOTHESIS` H1. `[M: 1512 H1]` Check the pre-press ghost read, never the intent.
- **`LeftMouseButton` outside placement mode is the hero's attack** (the second press; "the second `LeftMouseButton` = `IA_Attack`"). It places nothing. `[M: 1512 Evidence, ctl]`
- **An all-Archer hand refills to an identical array.** "The card leaves the hand" rests on `DrawPile` −1 and `DiscardPile` +1, never on `Hand`. `[M: 1512 A3, Not examined]`
- **Gold can straddle an income tick** (+≈1/s). Assert "went down", and take the cost from the `played card` line. `[M: 1512 A3, ctl]`
- **Films.**
  - The film armed before `start_pie` held the menu only (H3), and `stop_pie_recording` did not collect it; the verifier found it by directory listing. `[M: 1512 Recording]`
  - The output is raw `.h264`, remuxed by the orchestrator. `[L: VER-§12 cl. 7b]`
  - a1's arena play is on no film. `[M: 1512 Recording]`
- **Reply overflow:** the a1 walk batch reply and both `stop_pie_recording` replies overflowed the tool limit and were read by grep/head only. `start_pie`'s replies were short that run. `[M: 1512 Not examined]`
- **The VRAM banner.** "Video memory has been exhausted" appeared in every frame (1058–1062 MB over in a1, 182 MB in a2). Name it when present. `[M: 1512 Not examined]`; `[L: VER-§12 cl. 7e]`
- **Frame names must match where the frame sits in the batch.** `1512` chose its names before the frames existed, and "two of them are **false** about their content": frames named `…before-confirm` were captured after the confirm. `[M: 1512 Evidence]` A promoted frame is never renamed afterwards. `[L: VER-§4 cl. 2, 2026-09-20 ruling]`
- **Grant declaration.** This recipe uses no `pie_scene_edit`. A run that adds one (e.g. a `SetMouseLocation` aim) is outside the fences, and it declares every op line by line under *Not examined*, as `1512` did for its four calls. `[M: 1512 Not examined]`; `[L: VER-§7 cl. 2]`
- **The zone's owner latches and flips** `[D: CaptureZone.h]`:
  - empty ⇒ unchanged (it latches the last owner);
  - contested ⇒ Neutral;
  - Red-only ⇒ Red.

  A `Blue` read proves the zone was Blue at that read, not at the press.
