# RCP-deckbuilder-slot-and-card-edit — select a deck slot and add/remove cards with `double_click`, batched, read back and topped up

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`.

## Source

- `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`:
  - **§1(b)** — selecting deck4 for editing, with the five-shape control table;
  - **§1(c)** — removing 50 Ogres and adding 50 Archers with `double_click` on the tile "−"/"+" buttons;
  - **A1** row of *Acceptance lines → observations* — the three state readers and their agreement;
  - **§1(e)** — leaving the builder (optional Step 6);
  - *Not examined / limitations* — the `name_path` hazard and the frame-geometry note; *Hypotheses* H1/H2.
- Run `PLAYTEST-archer50`, **2026-09-26**, `Verdict: MEASURED` (boarded afterwards as `TASK-1501`). Law that absorbed it: `VER-§5` cl. 5 (2026-09-26 narrowing), `VER-§12` cl. 7a, `VER-§13` cl. 1.
- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` W1, N4). Not re-verified since.
- ⚠️ **The call envelope for the batched edits is `NOT MEASURED`.**
  - **What the source says:** the gesture and the tool. "`ui_perform`'s **`double_click` step FIRES `UButton.OnClicked`** … This is how all 100 deck edits were made." (*THE ANSWER FIRST*) §1(c) then reports runs of 49, 10, 30 and 28 gestures at a stated frame spacing. The one call whose error text it quotes carried the root `WBP_DeckBuilder`: "No widget in root 'WBP_DeckBuilder' matches…" (§1(b)).
  - **What it does not say:** the envelope. It could have been a standalone `ui_perform` whose `steps` array carried a whole run, or `ui_perform` steps inside `run_verification_sequence`. It also does not say how many calls one run took.
  - Two non-measurements bear on it. The orchestrator's sighting (`VER-§13` preamble) says the deck editing "ran one gesture per tool call (100 edits)". QA's arithmetic against that (`qa/TASK-1505.md` M4) is labelled inference there.
  - Step 4 says which envelope this recipe uses and why.

## Plugin version

**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." `VER-§5` cl. 5's narrowing repeats it ("plugin version not read that run"). ⇒ the first use re-verifies in-run (`VER-§13` cl. 3).

## Preconditions

1. The Deck Builder is open, reached by `RCP-menu-to-deckbuilder.md`; on open it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`. `[M: archer50 §1(a)]`
2. Same PIE geometry as the source: viewport **1280×725**, **DPI 0.6706**. Every `abs_*` value below is only meaningful there. `[M: archer50 Editor/Aura state]`
3. **Read the starting contents of every deck you will touch before editing.** The source read deck4 = `[{card_id: "Ogre", count: 50}]` before the run. `[M: archer50 A1]`
4. **The builder auto-saves every landed edit.** The source's log carries one `saved deck 'deck4' (<n> cards)` line per change, a monotonic run ending "(39 cards) → (50 cards)". A mis-aimed or excess edit is therefore **on disk immediately**. `[M: archer50 A1 (iii)]` ⇒ the orchestrator hashes every `.sav` before the run (the verifier has no shell: *Not examined*, "`.sav` integrity: not hashed by me (no shell). The orchestrator holds the pre-run hashes"). `[M: archer50 Not-examined]`

## Steps

**Step 1 — resolve every target with `ui_snapshot` before any gesture** (`VER-§12` cl. 7a).
```json
{"widget": "WBP_DeckBuilder"}
```
Tool: `ui_snapshot`. From the tree, record:
- the deck slot entry for the deck you want. For deck4 it was `DeckSlotEntryWidget_3`, and its `SlotButton` sat at abs (1674.9, 870.9) 121×31. `[M: archer50 §1(b)]` That entry *i* holds deck *i*+1 for other decks is an inference from this one pair: `NOT MEASURED`.
- the tile for each card you will edit (`WBP_DeckCardTile_C_<k>`), identified by what it displays, and the abs rect of its "−" or "+" button. In the source, Ogre was `WBP_DeckCardTile_C_12` (its count text read "50"; "−" at abs (2056.9, 1186.6) 28×20) and Archer was `WBP_DeckCardTile_C_1` (the 2nd tile; "+" at abs (1380.9, 1186.6) 29×20). `[M: archer50 §1(c) + Evidence]` Whether the tile index for a card is stable across runs: `NOT MEASURED`. Resolve it every run.
Source for the method: "From then on I resolved targets by plain name and verified each one with `ui_snapshot` first." `[M: archer50 Not-examined]`

**Step 2 — select the deck slot for editing: ONE `double_click` on the slot button's body.**
```json
{"widget": "WBP_DeckBuilder", "steps": [
  {"type": "double_click", "selector": {"by": "name", "value": "DeckSlotEntryWidget_3"}, "offset": {"dx": -45, "dy": 0}}
]}
```
Tool: `ui_perform`. Source (§1(b) table, last row): "**`double_click`**, offset dx −45 | 210.96 | `SButton` | down `true` / up `false` / **double_click `true` / up `true`** | **3**, `WorkingDeck = (DeckName="deck4",Cards=((CardID="Ogre",Count=50)))` (t=215.33)". `[M: archer50 §1(b)]`
- Root `"WBP_DeckBuilder"`: the source's own error text names it ("No widget in root 'WBP_DeckBuilder' matches…"). `[M: archer50 §1(b)]`
- Selector: the table names `DeckSlotEntryWidget_3` for the centre-click row; the dx −45 rows say only "offset dx −45 (button body, not label)". That they used the same selector is the natural reading, not a quoted fact. `dy`: `NOT MEASURED` (0 here is the author's fill). Step and offset spelling `[S]`.
- This **selects the deck for editing**. It does **not** make it the active deck (right-click only; no route, `VER-§8` cl. 12).

**Step 3 — calibrate: ONE `double_click` on the tile button, then read.**
```json
{"widget": "WBP_DeckBuilder", "steps": [
  {"type": "double_click", "selector": {"by": "name", "value": "WBP_DeckCardTile_C_12"}, "offset": {"dx": "<button centre x − tile centre x, from Step 1>", "dy": "<button centre y − tile centre y, from Step 1>"}}
]}
```
Source: "Calibration, 1 gesture: Ogre **50 → 49** (t=304.53) ⇒ **one gesture = one `OnClicked`**." Each target "was addressed by **plain name + an offset** onto the button body." `[M: archer50 §1(c)]`
- The offset **values** the source used for the tile buttons are `NOT MEASURED` (not in the report). Derive them from Step 1's rects, and put the numbers in place of the `"<…>"` placeholder strings before sending. That `offset` is measured from the target's geometric centre is `[S]` (schema text); the offset's units are never stated by the source (dx −45 on a 121-wide abs rect landed on the button body).
- Read back before batching (Step 5). If the count did not move by exactly 1, stop: the target is wrong.

**Step 4 — the batched edit: N `double_click`s on the same button, spaced by `wait` frames, in ONE call.**
```json
{"widget": "WBP_DeckBuilder", "steps": [
  {"type": "double_click", "selector": {"by": "name", "value": "WBP_DeckCardTile_C_1"}, "offset": {"dx": "<from Step 1>", "dy": "<from Step 1>"}},
  {"type": "wait", "frames": 20},
  {"type": "double_click", "selector": {"by": "name", "value": "WBP_DeckCardTile_C_1"}, "offset": {"dx": "<from Step 1>", "dy": "<from Step 1>"}},
  {"type": "wait", "frames": 20}
]}
```
(repeat the pair N times; N = the shortfall read in Step 5, never more — see overshoot below)
- What the source measured, per batch `[M: archer50 §1(c)]`:

  | batch | button | gestures sent | spacing | count before → after | landed | read at |
  |---|---|---|---|---|---|---|
  | 1 | Ogre "−" | 49 | 3 frames | 49 → 7 | **42 of 49** | t=356.46 |
  | 2 | Ogre "−" | 10 | 30 frames | 7 → 0 (deck empty: `WorkingDeck = (DeckName="deck4")`) | ≥7; drops not measurable (extra "−" at 0 is a no-op) | t=375.48 |
  | 3 | Archer "+" | 30 | 20 frames | 0 → 27 | **27 of 30** | t=412.87 |
  | 4 | Archer "+" | 28 | 22 frames | 27 → 50 | ≥23; drops not measurable ("+" greys at 50) | t=445.25 |

- ⚠️ **Which envelope, and why this one.** `VER-§13` cl. 1 and the verifier's S1 say predictable input goes into "ONE `run_verification_sequence`" `[L]`. Cl. 1's ruling of 2026-09-26 (the manager, on `TASK-1513`), quoted by text in italics (emphasis marks dropped): *"ONE `run_verification_sequence`" MEANS ONE TOOL CALL FOR THE BATCH, not one per gesture. `run_verification_sequence` is the default envelope, and it is REQUIRED whenever a batch mixes tools, which includes every cl. 2 group (a set, its confirm, and the reads that depend on it). A batch of one tool's gestures only may use that tool's own steps array (e.g. `ui_perform` with `steps`), provided the recipe or the report names the envelope and says why.* `[L: VER-§13 cl. 1, 2026-09-26 ruling]` This step is that case, and this paragraph names the envelope: **one standalone `ui_perform` call** whose `steps` array carries the whole run, every step in it a `ui_perform` step (`double_click`, `wait`; spellings `[S]`). Reasons, none of them a measurement:
  1. `ui_perform`'s `double_click` step is the tool and gesture the source names for every edit `[M: archer50 THE ANSWER FIRST]`, and the one call whose error text the source quotes carried the root `WBP_DeckBuilder` `[M: archer50 §1(b)]`.
  2. Its schema runs "the whole ordered scenario … ENGINE-SIDE in one call", with `{type:"wait", frames:N}` as a standalone hold `[S]`. That is one call per batch, not one per gesture, which is what cl. 1 exists to stop.
  3. The law's envelope is available: the runner accepts `ui_perform` as a step (`VER-§7` cl. 2 (i), from `TASK-1390`) `[L]`. But nothing quotes whether the runner accepts `ui_perform`'s root `widget` key inside a step. That is `NOT MEASURED`, and this step needs the root.

  **`NOT MEASURED`: both envelopes, for this batch.** The source does not say which one it used (see Source). The `wait` spelling is `[S]`. The question this step used to leave open (`qa/TASK-1505.md` M7: does cl. 1's "ONE `run_verification_sequence`" mean "one tool call"?) is answered by the ruling above. The ruling **permits** this envelope; it does not **measure** it. Under either envelope the read-back and top-up are owed (Step 5; the same ruling: "Either way the read-back and top-up above are owed").
- ⚠️ Replace every `"<from Step 1>"` placeholder with the number derived in Step 1 before sending. They are strings where the tool expects numbers, and they are there because the source never gave the tile offsets.
- Spacing reduces loss and does not end it: 3 frames lost 7 of 49, 20 frames lost 3 of 30. No zero-loss spacing was found. `[M: archer50 §1(c), H2]`

**Step 5 — read back and top up (mandatory after every batch; `VER-§13` cl. 1).**
```json
{"widget": "WBP_DeckBuilder", "properties": ["EditingDeckIndex", "WorkingDeck"]}
```
Tool: `get_widget_property_in_pie` `[S]` (the source calls it a "live widget read" of `WBP_DeckBuilder_C_0.WorkingDeck` + `EditingDeckIndex` and does not name the tool). `[M: archer50 A1 (i)]`
Loop: read the count → shortfall = target − count → send a Step 4 batch of exactly the shortfall → read again → repeat until the read equals the target. `[L: VER-§13 cl. 1]` The report states **sent vs landed per batch** (`SC-§104`: assert the state, not the tally sent).
What the source actually did: 30 "+" sent → read 27 → 28 more sent → read 50. It sent more than the shortfall of 23, which was safe only because the "+" greys at 50 on a single card. `[M: archer50 §1(c)]` Sizing a top-up to exactly the shortfall is the law's rule, not something the source measured.

**Step 6 (optional) — leave the builder: `double_click` on the Exit text.**
Resolve first:
```json
{"widget": "WBP_DeckBuilder", "selector": {"by": "text", "value": "Exit"}}
```
Tool: `ui_snapshot`. Source: "resolved first with `ui_snapshot` `{by:text, value:"Exit"}` → `TextBlock_10`, t=586.97". The source quotes only the selector; the root `WBP_DeckBuilder` is the author's fill. Then:
```json
{"widget": "WBP_DeckBuilder", "steps": [
  {"type": "double_click", "selector": {"by": "name", "value": "TextBlock_10"}}
]}
```
Source: "A `double_click` on the builder's **Exit** text … returned to the main menu. Frame t=595.35". `[M: archer50 §1(e)]` The selector the source passed to the `double_click` is not quoted (it may have been the text selector itself): `NOT MEASURED`. ⚠️ This measurement bears on `VER-§12` cl. 5 ("no instrument available to the verify lane can close a sub-screen … `ui_perform` a named dead end"), which has not been amended for it. Flagged to the manager; this recipe records only what §1(e) measured, on this one screen.

## Read-back

| step | reader | proves | measured value |
|---|---|---|---|
| 2 | `EditingDeckIndex` (live widget read) | the slot is selected for editing | `0` → **`3`** after the `double_click` (t=215.33); every other shape left it at `0` `[M: archer50 §1(b)]` |
| 3–5 | `WorkingDeck` (live widget read) | each batch's landed count | values in the Step 4 table `[M: archer50 §1(c)]` |
| final, live | `WorkingDeck` + `EditingDeckIndex` | end state in the builder | t=445.25: `EditingDeckIndex = 3`, `WorkingDeck = (DeckName="deck4",Cards=((CardID="Archer",Count=50)))` `[M: archer50 A1 (i)]` |
| final, disk (after PIE) | `GameplayStatics.load_game_from_slot("SiegeDecks_<profile id>", 0)` through the read-only Python lane | the save agrees | `DECK deck4 : [{card_id: "Archer", count: 50}]`; slot in the source: `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34` `[M: archer50 A1 (ii)]` |
| final, log | `LogGitClaudeUnrealTest` line `UDeckBuilderWidget: saved deck '<deck>' (<n> cards) to slot '<slot>'` | the last auto-save matches | `…saved deck 'deck4' (50 cards) to slot 'SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34'.` `[M: archer50 A1 (iii)]` |
| corroboration only | tile count text; a frame you `Read` | pixels agree | tile 2 (Archer) "50", every other tile "0"; "Deck: 50/50" `[M: archer50 A1 (iv), Evidence]` |
| 6 | a frame + `ui_snapshot "WBP_MainMenu"` | back on the main menu | seven-button menu, no deck bar, no card grid (t=595.35) `[M: archer50 §1(e), Evidence]` |

The `ui_perform` trace (`down true / up false / double_click true / up true`) corroborates at most. It is never the observable. `[M: archer50 §1(b)]`; law `VER-§5` cl. 5.

## Fences — not measured for

- Only deck4's `SlotButton` and two tile buttons (Ogre "−" on `WBP_DeckCardTile_C_12`, Archer "+" on `WBP_DeckCardTile_C_1`). Other slots and tiles are the same widget classes; that they behave the same is an inference.
- Only `UButton.OnClicked` on `L_MainMenu`'s Deck Builder. Not measured: any non-`UButton` class (`UCheckBox`, sliders), a button that also binds a Slate double-click handler, any other map (`VER-§5` cl. 5 fences).
- **Setting the active deck.** Right-click only, and `ui_perform` has no right button (`archer50` §1(d); `VER-§8` cl. 12). Pending: `TASK-1511`.
- Editing the deck that is currently active; editing a deck whose total is not 0 or 50; a deck total above 50; the "Reset to Default" and "Play" buttons; closing the card details panel. None was exercised.
- **Why `double_click` works where `click` does not** is `HYPOTHESIS` H1 (the first synthetic up may not be routed to the capturing `SButton`; "MECHANISM NOT MEASURED"). `[M: archer50 Hypotheses]`
- **Why batches drop gestures** is `HYPOTHESIS` H2 (coalescing inside the OS double-click interval; "Not measured"). `[M: archer50 Hypotheses]`
- Drop rates at spacings other than 3 and 20 frames (the 22- and 30-frame batches saturated, so their losses are unknown).
- Plugin version: not read.

## Coordinate/resolution-dependent values

All measured at viewport **1280×725**, **DPI 0.6706**, standalone, 1 client. `[M: archer50 Editor/Aura state]`

| value | where | source |
|---|---|---|
| deck4 `SlotButton` abs (1674.9, 870.9), 121×31 | Step 1–2 | `[M: archer50 §1(b)]` |
| slot offset `dx −45` (`dy` not stated) | Step 2 | `[M: archer50 §1(b)]` |
| Ogre "−" (`WBP_DeckCardTile_C_12`) abs (2056.9, 1186.6), 28×20 | Step 1, 3–4 | `[M: archer50 §1(c)]` |
| Archer "+" (`WBP_DeckCardTile_C_1`) abs (1380.9, 1186.6), 29×20 | Step 1, 4 | `[M: archer50 §1(c)]` |
| "screen centre" where a missed `name_path` lands: (1919.5, 1230) | hazard | `[M: archer50 Not-examined]` |

- **`abs_*` values are not captured-image pixels.** Source: "In the captured frame, tiles and the deck bar sit at a different scale and position than the `abs_*` coordinates imply, yet every gesture aimed by `abs_*` hit its intended button." `[M: archer50 Not-examined]`; law `VER-§12` cl. 7 (measured ratio ≈2.53× on another run). Never aim from a frame.
- Whether `abs_x`/`abs_y` are the top-left or the centre of the rect is not stated by the source.
- Tile indices (`_C_12`, `_C_1`) and slot entry indices are runtime names, not coordinates, but they are layout-dependent: resolve them every run (Step 1).

## Known hazards

- **`click`, `press`/`release`, and a `click` on the button body do NOT fire `OnClicked`.** All three left `EditingDeckIndex` at `0` with the trace `down handled:true / up handled:false`. Named dead ends: do not re-trace. `[M: archer50 §1(b)]`; law `VER-§5` cl. 5.
- **A missed `name_path` selector does not error.** `{name_path: "WBP_DeckCardTile_C_12/…/Button_1"}` silently resolved to the root, delivered the gesture at screen centre (1919.5, 1230), and opened the **`Fireball`** details panel (`SelectedDetailCardID = Fireball`, t=274.09). No deck count changed. `[M: archer50 Not-examined]`; law `VER-§12` cl. 7a. ⇒ act by plain name + offset only; treat any unexpected screen change as a mis-aim until a state read says otherwise. The source caught this mis-aim by reading `SelectedDetailCardID`, and confirmed no edit landed by the Ogre count ("Ogre 50 before and after, t=260.79").
- **`by:name` does not index nested names.** `{by:name, value:"SlotButton", match_index:3}` → *"No widget in root 'WBP_DeckBuilder' matches by=name value='SlotButton'"*. A bad `by:name` errors cleanly. `[M: archer50 §1(b)]`
- **Rapid batches drop gestures** (Step 4 table). Never trust a batch blind: read back and top up. `[M: archer50 §1(c)]`; law `VER-§13` cl. 1.
- **Overshoot.** The tile "+" greys only at the per-card 50 (the source read Ogre's "+" `enabled: false` at 50, which made overshoot on one card impossible). `[M: archer50 §1(c)]` The deck **total** has no cap by ruling. `[L: VER-§13 cl. 1; UNCAP-§4 U4]` ⇒ size every "+" batch to the measured shortfall, never more, and read the total.
- **Edits persist immediately** through the auto-save (Precondition 4). A mis-aim is a disk change.
- **No right mouse button.** `click` + `"button":"right"` and `press`/`release` + `"button":"RightMouseButton"`/`"mouse_button":"right"` were each delivered as a left press; the extra keys were silently ignored. `[M: archer50 §1(d)]`; law `VER-§8` cl. 12.
- **`simulate_key_press` does not reach this screen's widgets.** `Tab` → "delivered to the player controller, but NOTHING BINDS IT"; no `SlotButton` focused; `FocusedCardIndex -1`. `[M: archer50 §1(b)]`
- **After Exit, `WBP_MainMenu` matches two instances.** Observed, not diagnosed. `[M: archer50 Not-examined]`
