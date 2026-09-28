# RCP-deckbuilder-set-active-by-keyboard — make a deck the ACTIVE deck from the Deck Builder with injected menu actions only

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` (the run) · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (downstream corroboration only) · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md` (a later use of this recipe: the open slot and the `IA_MenuLeft` variation) · `1519` = `.claude/pipeline/qa/TASK-1519-verify.md` (a later use of this recipe as a control route: the walk 2→3→4 in one batch and two deck5 refusals; it set no deck) · `1509` = `.claude/pipeline/qa/TASK-1509.md` (the pre-compile code review of the route; its sentences are code reads, **not** runtime measurements, and are cited by finding number).

> ⚠️ **Read this first.**
> - **This recipe drives door 3 only:** `inject_input_action IA_MenuSecondary` on a focused deck-bar slot. It is **not** evidence for the real `Home` key or for gamepad Y. Those are code-proven, not runtime-proven. `[L: DECK-§9 cl. 10; VER-§8 cl. 1]`
> - **It changes the player's REAL active deck, on disk, at the press.** `1511` read `ActiveDeckName=deck4` on disk while PIE was still running, right after the press. `[M: 1511 A1]` The run states the value before and after, and the orchestrator hashes the `.sav` files.
> - **Measured in `1511`:** one set-active (target slot 3), one refusal (slot 4, empty deck5). `[M: 1511 A1, A3]` **Used again by `1524`**, which opened on slot 2, set slot 3 with one `IA_MenuRight`, and later walked back to slot 2 with `IA_MenuLeft` and set it (the variation after Step 5). `[M: 1524 A1, Recipes used]` **Used again by `1519`** as a control route: it walked 2→3→4 in one batch and pressed the deck5 refusal twice. Those two refusals were its only `IA_MenuSecondary` presses, so it set no deck (`ActiveDeckName` deck3 before and after). `[M: 1519 Recipes used, C-a, Not examined, Save hygiene]`
> - **The walk counts from the slot the builder opened on** (`EditingDeckIndex`, the active deck's slot, `DECK-§3`), read at open and never assumed to be 0 (Precondition 4).

## Source

- `.claude/pipeline/qa/TASK-1511-verify.md`, `Verdict: VERIFIED`, run **2026-09-26**. 1 attempt of 3; PIE clock 0.40 s → 129.48 s. Log times: builder open 00:12:06, set-active 00:13:13, refusal 00:13:52 (*Speed data*).
  - *Editor/Aura state* · *Pre-registration* · *Acceptance lines* rows **0**, **A1**, **A2**, **A3** (A4/A5 are declared ceilings) · *Pixel note* · *Save hygiene* · *Hypotheses* · *Not examined / limitations* · *Speed data* · *Recipes used* · *Recipe candidates* (steps 1–5 + the Fences line).
- The law:
  - `VER-§8` cl. 12, 2026-09-26 amendment (marker `VER-8-12-SET-ACTIVE-ACTUABLE-BY-IA-MENUSECONDARY`): the route, its fences, and what stays a ceiling.
  - `DECK-§9` cl. 10 (marker `DECK-9-10-SET-ACTIVE-ROW`): the route's state label and what is not measured. For the state of `1509` W1 (the held-key repeat), read that clause's W1 bullet (marker `DECK-9-10-W1-CLOSED-0500D51`); this recipe does not restate it (`SC-§126` cl. 7).
  - `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"* (Precondition 4).
  - `VER-§12` cl. 7e: capture thin UI at `max_dim` ≥ 1280. `VER-§13` cl. 1, 2026-09-26 ruling: one tool call per batch.
- Code review (`1509`), cited where the run could not see: N4 (a mouse click before the walk), W1 (a held key), and *For 5b* (an absent relay line).
- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26.
- **Used by `1524`**, 2026-09-26, `Verdict: VERIFIED`, 1 PIE session: "Steps 1–5 … + Control C3 … re-verified in-run **y, with a mismatch**: Precondition 4 (`EditingDeckIndex = 0`) did not hold — it read 2 — and `IA_MenuBack` put focus on `DeckSlotEntryWidget_2`, consistent with the recipe's own rule … so T counted from slot 2 (one Right to slot 3). Everything else matched". `[M: 1524 Recipes used]` Cited here for: row 0, A1 (both legs), A2, *Editor/Aura state*, *Pre-registration*, *Hypotheses* H2, *Not examined*, *Recipe candidates* 1 and 3.
- Amended by `TASK-1527`, 2026-09-26: Precondition 4 and the walk count (`1524`, `DECK-§3`), the `IA_MenuLeft` variation (`1524`), and `qa/TASK-1518.md` N1–N3 plus the `DECK-§9` cl. 10 pointers.
- **Used by `1519`**, 2026-09-27, `Verdict: MEASURED`, 1 PIE session, a census probe of the right mouse button that used this recipe for its control route: "Step 0 (disk read), Step 1 (arm grid, `FocusedCardIndex=0`), Steps 2–3 (Back → slot 2, Right ×2 → slot 4), Control C3 (refusal on deck5, used here as control (a), twice) · re-verified in-run **y**: every read matched (walk counted from E = 2; refusal text identical). The walk 2→3→4 in one batch from a fresh open is new relative to the recipe's measured walks (it measured 2→3 and 3→4 separately) and landed 2/2." `[M: 1519 Recipes used]` Cited here for: row 0, C-a, *Pre-registration*, *Editor/Aura state*, *Save hygiene*, *Not examined*.
- Amended by `TASK-1533`, 2026-09-27: `1519`'s use (Precondition 4, Steps 2–3, Control C3, Read-back, Fences), the right-button hazard and fence (a pointer to `VER-§8` cl. 12), and `qa/TASK-1528.md` Q3–Q4.

## Plugin version

**Not read.** `1511` *Not examined*: "Aura plugin version not read." `1524` and `1519` did not read it either (same words, each *Not examined*). ⇒ the first use of this recipe re-verifies it in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).

## Preconditions

1. Level `/Game/Maps/L_MainMenu`, loaded by `load_level` (`discarded_unsaved: false`). PIE standalone, 1 client, 1280×720 requested → viewport **1280×725**, **DPI 0.6706**. `is_pie_active` read `false` before the run started. `[M: 1511 Editor/Aura state]`
2. **The binaries carry `TASK-1507`'s route**, committed in `0a5b8a7` (`DECK-§9` cl. 10, marker `DECK-9-10-RELABELLED-0A5B8A7`). `[L: DECK-§9 cl. 10]` Check it at runtime: read the builder-open bind line **after** the builder opens. `[M: 1511 row 0]`
   > `UDeckBuilderWidget::BindMenuNavActions: 7 IA_Menu* action(s) bound (Started) on 'PlayerController_0' [IA_MenuUp, IA_MenuDown, IA_MenuLeft, IA_MenuRight, IA_MenuAccept, IA_MenuBack, IA_MenuSecondary]; every IA_Menu* asset resolved.`

   If the line names fewer than seven actions, or reports an unresolved asset, stop: the route is not in this build.
3. **The builder is freshly open, reached by `RCP-menu-to-deckbuilder.md` Steps 0–4.** `1511` re-verified that recipe in the same run: `Button_0` focused at t=4.89; `IA_MenuDown` ×2 in ONE `run_verification_sequence` → `Button_2` `focused:true` at t=9.62; `IA_MenuAccept` at t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14 (deck1 was active in that run, `1511` *Pre-registration*). `[M: 1511 Recipes used]` That recipe's Step 1 can land 1 of 2 Downs; its Step 2 reads focus and tops up (`1524`). `[M: 1524 Recipes used]`
4. **Read `EditingDeckIndex` at open (`RCP-menu-to-deckbuilder.md` Step 4) and call it E. It is the ACTIVE deck's slot, not a fixed 0.** `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"*. `[L: DECK-§3]`
   - Measured: E = `0` when deck1 was active (`1511`: `EditingDeckIndex=0` at t=15.14, `ActiveDeckName = deck1` in its Pre-registration) `[M: 1511 Recipes used, Pre-registration]`; E = `2` when deck3 was active (`1524`: "Builder open t=19.83: `WBP_DeckBuilder_C_0`, `EditingDeckIndex=2`", `ActiveDeckName = deck3` in its Pre-registration) `[M: 1524 row 0, Pre-registration]`. `1519` read E = `2` with deck3 active again ("Builder t=12.24: `WBP_DeckBuilder_C_0`, `EditingDeckIndex=2`", `ActiveDeckName = deck3` in its Pre-registration) `[M: 1519 row 0, Pre-registration]`. That is a second observation of the same pair, not a new value. That the builder does this in general is `HYPOTHESIS` H2 in `1524` ("One observation; mechanism not read") `[M: 1524 H2]`, and it stays a hypothesis after `1519`.
   - After Step 2, focus lands on `DeckSlotEntryWidget_<EditingDeckIndex>` `[M: 1511 Recipe candidates step 2]`. `1524`: "`IA_MenuBack` put focus on `DeckSlotEntryWidget_2`, consistent with the recipe's own rule". `[M: 1524 Recipes used]` `1519`: "Back → slot 2". `[M: 1519 Recipes used]`
   - ⇒ **The walk counts from E, never from an assumed 0:** `IA_MenuRight` × (T − E) when T > E. `1524` sent one Right from slot 2 to reach slot 3 ("T counted from slot 2 (one Right to slot 3)"). `[M: 1524 Recipes used]` `1519` sent two Rights from slot 2 to reach slot 4 ("Right ×2 → slot 4"; "walk counted from E = 2"). `[M: 1519 Recipes used]` T < E needs `IA_MenuLeft` (measured only as the variation after Step 5). At a fresh open, T = E is the active deck itself (`DECK-§3`; Fences: target already active).
   - Starting values other than 0 and 2: `NOT MEASURED`.
5. **No mouse input inside the builder before the walk.** `[L: VER-§8 cl. 12, 2026-09-26 amendment]` The reason is a code read: after a mouse click puts focus on a bar slot while `FocusedCardIndex` is still armed, injected `Home`/`Left`/`Right` act on the grid while a real key acts on the bar (`1509` N4). `1511` sent no mouse input at all. `[M: 1511 Not examined]` ⚠️ **Added 2026-09-27 (`TASK-1548`): nor any `ui_perform` call before the walk,** including one with no pointer step. `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) measured a `ui_perform` clearing Slate keyboard focus 8 of 8 for `type`, `scroll`, `move` and a 1-frame `wait` (its other shapes are not measured), and Steps 1–3 move and read focus: after one, the walk may start from a focus no read established. No record in this recipe rests on such a read: `1511` and `1524` record no `ui_perform`, and `1519` records no `ui_perform` call: its one mention (*Not examined*) says its `ui_perform` right-button shapes were not re-traced, and its pointer input in the builder was `SetMouseLocation` (a cursor move, no click), "after control a1, never before the keyboard walk". `[M: 1519 Not examined]`
6. **The target is fixed before any batch is sent** (a batch cannot branch on its own result, `README.md` common hazards):
   - `T` = the target's 0-based bar slot, and its deck `deck<T+1>`.
   - Slot → deck, as the relay lines printed it: slot 0 'deck1', 1 'deck2', 2 'deck3', 3 'deck4', 4 'deck5'. `[M: 1511 A1, A3]` Slots 5–9: `NOT MEASURED`.
   - Decide from the Step 0 disk read that the target is legal: exactly 50 cards. That is the gate's own text in the measured refusal line: "a legal deck is exactly 50 (GDD 3.4)". `[M: 1511 A3]`
7. **Save hygiene.** The orchestrator hashes every `.sav` before and after; the verifier holds no hash tool. `[M: 1511 Save hygiene]`
8. Optional recording: `1511` armed the recorder before `start_pie` (`arm_wait_for_pie`), and the film ran from game t=0.40 s: raw `.h264`, 3547 frames, 118.23 s. There is no level travel in this recipe. `[M: 1511 Recording]` For the remux line see `README.md`.

## Steps

**Step 0 — before PIE (no PIE call): read the save.**
- Call: `load_game_from_slot("SiegeDecks_<profile id>", 0)`, class `SiegeDeckSaveGame`. Read `ActiveDeckName`, and every deck's cards and total. `[M: 1511 Pre-registration]`
  - Slot in the source: `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34`.
  - Values in the source: `ActiveDeckName = deck1`; deck1 = 51 (illegal), deck2 = 80, deck3 = 50, deck4 = 50× Archer, deck5 … deck10 = 0.
- Lane: the read-only editor Python lane. `1511` quotes the call but not the tool; `1512` *Save hygiene* names it for the same call ("editor Python, `load_game_from_slot`").
- Write the expected after-state into the report before PIE, as `1511` did (its Pre-registration was written before `start_pie`).

**Step 1 — arm the grid: `IA_MenuDown`, then read `FocusedCardIndex`.**
```json
{"actions": [
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.3}},
  {"type": "get_widget_property_in_pie", "params": {"widget": "WBP_DeckBuilder", "properties": ["FocusedCardIndex"]}}
]}
```
Tool: `run_verification_sequence`.
- **Source:** "`inject_input_action /Game/Input/Actions/IA_MenuDown.IA_MenuDown` → read `FocusedCardIndex == 0` (t=56.67)." `[M: 1511 Recipe candidates step 1]` A1: "`IA_MenuDown` t=56.36 → `FocusedCardIndex=0`, `WBP_DeckCardTile_C_0 focused:true`". `[M: 1511 A1]`
- **The wait:** not quoted for this step. The 0.3 s is Step 3's quoted spacing. The stamps show 0.31 s between the Down and the read (t=56.36 → 56.67).
- **The reader:** the report names the property, not the reader (`NOT MEASURED`: the reader's name). `get_widget_property_in_pie` with `widget` and `properties` ran inside a sequence in the same run (Step 4's read). `[M: 1511 Recipe candidates step 4]`; `[L: VER-§12 cl. 7c]` Param spelling `[S]`.
- **The envelope in the source:** this Down shared one batch with the tile-focused control press (Control C2). *Speed data*: "`IA_MenuDown` + tile-focused `IA_MenuSecondary` 2 sent, Down landed (`FocusedCardIndex=0`), Secondary changed nothing by design". `[M: 1511 Speed data]`
- **Why this step cannot be skipped:** `IA_MenuBack` is routed as `Gamepad_FaceButton_Right`: `HandleMenuNavBack` calls `RouteMenuNavKey(EKeys::Gamepad_FaceButton_Right, TEXT("IA_MenuBack"))`. `[D]` That is the key on `DECK-§9`'s "Exit the grid" row, whose precondition is "a card tile holds Slate focus". `[L: DECK-§9 cl. 1 table]` At cold open `FocusedCardIndex` read `-1`, with no bar `SlotButton` focused. `[M: 1511 A2]` What `IA_MenuBack` does at cold open: `NOT MEASURED`.

**Step 2 + Step 3 — ONE sequence: `IA_MenuBack`, then `IA_MenuRight` × (T − E), with a `DeckBar` snapshot after each step.** E is the `EditingDeckIndex` read at open (Precondition 4). Shown for T = 3 from E = 0, `1511`'s case (three Rights). With E = 2, `1524` sent one Right `[M: 1524 Recipes used]`: send one Right/wait/snapshot triple per slot of T − E.
```json
{"actions": [
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuBack.IA_MenuBack"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.3}},
  {"type": "ui_snapshot", "params": {"widget": "WBP_DeckBuilder", "selector": {"by": "name", "value": "DeckBar"}, "max_depth": 3}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuRight.IA_MenuRight"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.3}},
  {"type": "ui_snapshot", "params": {"widget": "WBP_DeckBuilder", "selector": {"by": "name", "value": "DeckBar"}, "max_depth": 3}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuRight.IA_MenuRight"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.3}},
  {"type": "ui_snapshot", "params": {"widget": "WBP_DeckBuilder", "selector": {"by": "name", "value": "DeckBar"}, "max_depth": 3}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuRight.IA_MenuRight"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.3}},
  {"type": "ui_snapshot", "params": {"widget": "WBP_DeckBuilder", "selector": {"by": "name", "value": "DeckBar"}, "max_depth": 3}}
]}
```
Tool: `run_verification_sequence`.
- **Source:** "`inject_input_action /Game/Input/Actions/IA_MenuBack.IA_MenuBack` → `ui_snapshot {"widget":"WBP_DeckBuilder","selector":{"by":"name","value":"DeckBar"},"max_depth":3}`; expect `DeckSlotEntryWidget_<EditingDeckIndex>/OutlineBorder/SlotButton focused:true`." `[M: 1511 Recipe candidates step 2]`
- "`inject_input_action /Game/Input/Actions/IA_MenuRight.IA_MenuRight` × T, `wait_pie_seconds 0.3` + the same `DeckBar` snapshot after each … Steps 2–3 ran as ONE `run_verification_sequence` (4/4 landed)." `[M: 1511 Recipe candidates step 3]` In `1511` E was 0, so its "× T" is × (T − E) (Precondition 4).
- **Measured stamps:** `IA_MenuBack` t=65.69 → only `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` focused. Then `IA_MenuRight` t≈66.03 → `_1`, t≈66.36 → `_2`, t≈66.69 → `_3`, "exactly one `focused: true` per snapshot". `[M: 1511 A1]`
- **From E = 2 (`1524`):** `IA_MenuBack` focused `DeckSlotEntryWidget_2` `[M: 1524 Recipes used]`; one Right then gave "focus on `DeckSlotEntryWidget_3/OutlineBorder/SlotButton` (snapshot t=26.60, relay line `…on deck-bar slot 2 ('deck3'); key-down handled=true; the focused bar slot is now 3 ('deck4')`)". `[M: 1524 A1]`
- **From E = 2 to slot 4 in one batch (`1519`):** relay lines `[17.22.19:066] …slot 2 ('deck3') … now 3 ('deck4')` and `[17.22.19:399] …slot 3 ('deck4') … now 4 ('deck5')`, and `DeckSlotEntryWidget_4/OutlineBorder/SlotButton` `focused: true`, the only focused slot (t=20.35). `[M: 1519 C-a]` Sent vs landed: "`IA_MenuBack` 1/1 (slot 2 focused)", "`IA_MenuRight` 2/2 (relay line each)". `[M: 1519 Not examined]` The per-step snapshot after the first Right is not quoted.
- **The wait after `IA_MenuBack`:** not quoted. The stamps (t=65.69 → first Right t≈66.03) fit the same 0.3 s spacing.
- **One relay log line per `IA_MenuRight`** (category `LogGitClaudeUnrealTest`), for example: `IA_MenuRight -> 'Right' handed to Slate's own key route on deck-bar slot 0 ('deck1'); key-down handled=true; the focused bar slot is now 1 ('deck2')`. `[M: 1511 A1]` The candidate text gives the line's prefix as `RelayDeckBarNavigationKeyToSlate:`. `[M: 1511 Recipe candidates step 3]` `1511`'s log census counts 4 relay lines for the whole session `[M: 1511 A2]`, which matches its 4 `IA_MenuRight` presses (3 in A1, 1 in A3). So `IA_MenuBack` printed no relay line. That is a reading from the count, not stated in words.

**Step 4 — ONE sequence: before-reads, `IA_MenuSecondary`, wait 0.5 s, after-reads.** Shown for T = 3, with the active deck before the press on slot 0 (deck1), as in `1511`:
```json
{"actions": [
  {"type": "get_widget_property_in_pie", "params": {"widget": "WBP_DeckBuilder", "child": "DeckSlotEntryWidget_0", "properties": ["OutlineBorder.BrushColor"]}},
  {"type": "get_widget_property_in_pie", "params": {"widget": "WBP_DeckBuilder", "child": "DeckSlotEntryWidget_3", "properties": ["OutlineBorder.BrushColor"]}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.5}},
  {"type": "get_widget_property_in_pie", "params": {"widget": "WBP_DeckBuilder", "child": "DeckSlotEntryWidget_3", "properties": ["OutlineBorder.BrushColor"]}},
  {"type": "get_widget_property_in_pie", "params": {"widget": "WBP_DeckBuilder", "child": "DeckSlotEntryWidget_0", "properties": ["OutlineBorder.BrushColor"]}}
]}
```
Tool: `run_verification_sequence`.
- **Source:** "`inject_input_action /Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary`, `wait_pie_seconds 0.5`, then in the SAME batch `get_widget_property_in_pie {"widget":"WBP_DeckBuilder","child":"DeckSlotEntryWidget_<T>","properties":["OutlineBorder.BrushColor"]}` → `(R=1,G=0.5,B=0,A=1)`; previous active slot → `(0,0,0,0)`". `[M: 1511 Recipe candidates step 4]`
- **The before-reads:** "Before the press (t=80.80): slot 0 BrushColor `(R=1,G=0.5,B=0,A=1)`, slot 3 `(R=0,G=0,B=0,A=0)`". `IA_MenuSecondary` went in at t=80.87; after it, at t=81.40, slot 0 read `(R=0,G=0,B=0,A=0)` and slot 3 `(R=1,G=0.5,B=0,A=1)`. `[M: 1511 A1]`
  - That the before-reads shared this batch is a reading from the stamps, not stated in words. 0.07 s separates the before-read from the press, while a boundary between two of this run's *Speed data* batches spans ≈8.5 s (e.g. t=57.14, the last read of the tile-focused control batch → t=65.69, the `IA_MenuBack` of the next). `[M: 1511 A1, A2, Speed data]` Not every stamp gap in `1511` is a call boundary (`qa/TASK-1518.md` N1).
- **Which slot to read as "previous active":** the slot of the Step 0 `ActiveDeckName` (slot map in Precondition 6).
- **The log line** for a legal target: `[00.13.13:066] UDeckBuilderWidget: active deck set to 'deck4' — the next match will use it.` `[M: 1511 A1]` The log reader `1511` used is `NOT MEASURED`.
- **Frames (optional, corroboration only):** `capture_pie_frame` with layer `composited`, at `max_dim` ≥ 1280. `[L: VER-§12 cl. 7e]` `1511`'s before/after pair in this batch came back at the default 1086×615 (`c2` t=80.80, `c3` t=81.45). At that size the before-leg outline under deck1 could not be resolved by eye. `[M: 1511 Pixel note]`
  - Spelling: `max_dim` is a parameter of the standalone `capture_pie_frame` `[S]`. It is not in `run_verification_sequence`'s published parameter union, which does not make it absent (`VER-§12` cl. 7c). Passing it inside a sequence is `NOT MEASURED`.
  - How `1511` obtained its 1280×725 frames (`a1_after_fullres_t88.69s`, `c4` t=111.21) is not quoted.

**Step 5 — read the save again: in PIE after the press, and after PIE.** Same call as Step 0. Source: "**Live disk read in PIE** (after the press): `ActiveDeckName=deck4`. **Disk after PIE stopped**: `ActiveDeckName=deck4`." `[M: 1511 A1]` Every deck's list and total was identical before PIE, in PIE after the press, and after PIE; the only change was `ActiveDeckName` `deck1` → `deck4`. `[M: 1511 Save hygiene]`

**Variation — return to the previous active deck with `IA_MenuLeft` (after Step 5, same session).** Measured once, by `1524` (A1 leg 2). `[M: 1524 A1, Not examined, Recipe candidates 1]` Use it to restore the before-value, P = the slot of Step 0's `ActiveDeckName` (slot map in Precondition 6).
- **The walk back:** from the slot that holds focus, F (the last snapshot's single `focused: true`), send `IA_MenuLeft` × (F − P) as ONE sequence with Step 3's shape: the action path `/Game/Input/Actions/IA_MenuLeft.IA_MenuLeft` in place of `IA_MenuRight`, a wait and a snapshot after each press.
- **What `1524` measured:** from slot 4 (focused after the deck5 refusals), `IA_MenuLeft` ×2 → "slot 3 focused (t=62.63) then slot 2 focused, slot 3 not (t=62.96/62.98)", with relay lines "`…slot 4 ('deck5') … now 3 ('deck4')` and `…slot 3 ('deck4') … now 2 ('deck3')`". `[M: 1524 A1]` The two presses were at t=62.31 and 62.64, "relay line per step". `[M: 1524 Recipe candidates 1]` Sent vs landed: "`IA_MenuLeft` 2/2". `[M: 1524 Not examined]` So `IA_MenuLeft` moves one slot per press, as `IA_MenuRight` does.
- **Then Step 4's batch with target P**, reading the slot this run set as the "previous active" slot. `1524`: before (t=68.43/68.44) slot 3 `(1,0.5,0,1)`, slot 2 `(0,0,0,0)`; `IA_MenuSecondary` t=68.46; after (t=68.98–69.01) slot 2 `(1,0.5,0,1)`, slot 3 `(0,0,0,0)`, slot 4 `(0,0,0,0)`; log `active deck set to 'deck3' — the next match will use it.`; `ActiveDeckName=deck3` on disk in PIE and after PIE, every deck's card list identical by field to the pre-registration read: "Restored: YES". `[M: 1524 A1]`
- **Spellings and waits:** the action path is `[D]` (`Content/Input/Actions/IA_MenuLeft.uasset`; the bind line in Precondition 2 names `IA_MenuLeft`). `1524`'s candidate names "`wait_pie_seconds 0.3` + a `ui_snapshot` of the expected `DeckSlotEntryWidget_<k>` after each" `[M: 1524 Recipe candidates 1]`; the stamps fit that spacing (press t=62.31 → focus read t=62.63). The snapshot's params object is not quoted: Step 2's `DeckBar` snapshot, used here, is this recipe's substitution (`NOT MEASURED` after a Left).
- **Save hygiene for a run that sets and then restores:** `VER-§3` cl. 6(c), marker `VER-3-6C-A-RESTORED-WRITE-PROVES-THE-PAYLOAD`. This recipe does not restate it.

**Controls — measured by `1511`, optional, and named in the report if used.**
- **C1, cold open** (A2). Send `IA_MenuSecondary` right after the builder opens, before Step 1. `1511` read `FocusedCardIndex=-1` at t=45.29 (no bar `SlotButton` focused per the t=34.56 `DeckBar` snapshot). Slot 0 stayed `(1,0.5,0,1)` and slot 3 stayed `(0,0,0,0)` at t=45.70; `EditingDeckIndex=0`. `[M: 1511 A2]` Speed data: "cold-open `IA_MenuSecondary` 1 sent, state unchanged by design". `[M: 1511 Speed data]`
- **C2, card tile focused** (A2). In the Step 1 batch, after the `FocusedCardIndex` read, send `IA_MenuSecondary` + the Step 4 reads. `1511`: with `WBP_DeckCardTile_C_0 focused:true` (t=56.71), slot 0 stayed `(1,0.5,0,1)` and slot 3 stayed `(0,0,0,0)` at t=57.12–57.14; `WorkingDeck` was unchanged. `[M: 1511 A2]` The wait inside that batch is not quoted.
- **C3, illegal (empty) target** (A3). After Step 4, one more `IA_MenuRight` + `IA_MenuSecondary` in one batch (2/2 landed), then the BrushColor reads. First re-read the target's total **in PIE**: `1511` read deck5 … deck10 `total=0` before the press. `[M: 1511 A3, Speed data]`
  - `IA_MenuRight` t=110.31 → focus on `DeckSlotEntryWidget_4/OutlineBorder/SlotButton`, relay line `… on deck-bar slot 3 ('deck4'); key-down handled=true; the focused bar slot is now 4 ('deck5')`.
  - `IA_MenuSecondary` t=110.64 → `Warning: UDeckBuilderWidget::SetActiveDeck('deck5'): Deck has 0 cards — a legal deck is exactly 50 (GDD 3.4). — refused; the active deck stays 'deck4'.`
  - At t=111.16–111.19: slot 3 `(1,0.5,0,1)`, slot 4 `(0,0,0,0)`, slot 0 `(0,0,0,0)`. Disk after PIE: `ActiveDeckName=deck4`.
  - **`1519` used C3 as its positive control, twice, with no Step 4 before it** (slot 4 reached by the walk 2→3→4 from the open slot). `IA_MenuSecondary` t=25.03 → `[17.22.24:394] Warning: UDeckBuilderWidget::SetActiveDeck('deck5'): Deck has 0 cards — a legal deck is exactly 50 (GDD 3.4). — refused; the active deck stays 'deck3'.` Slot 4 `(0,0,0,0)` and slot 2 `(1,0.5,0,1)` before (t=25.00/25.02) and after (t=25.55/25.57). Again at t=139.80 → `[17.24.19:157]`, identical text; slot 4 `(0,0,0,0)`, slot 2 `(1,0.5,0,1)` after (t=140.31/140.33). `[M: 1519 C-a]`
- **The zero carries its control.** `LogGitClaudeUnrealTest` held 9 lines for the whole session (1 bind, 2 focus read-backs, 4 relay, 1 set-active, 1 refusal). There were zero set-active or refusal lines between the builder open (00.12.06) and the first relay (00.12.58), the window that holds both C1 and C2. `[M: 1511 A2]` The same reader returned the A1 set-active line and the A3 refusal line, so that zero is a zero, not a void.

## Read-back

| step | state read that proves it landed | measured value |
|---|---|---|
| precondition | builder-open bind line: 7 `IA_Menu*` actions, every asset resolved | row 0, log `[00.12.06:863]` `[M: 1511 row 0]` |
| 1 | `FocusedCardIndex` | `0` (t=56.67); `WBP_DeckCardTile_C_0 focused:true` `[M: 1511 A1, Recipe candidates step 1]` |
| 2 | `DeckBar` `ui_snapshot`: exactly one `focused: true`, on `DeckSlotEntryWidget_<E>` | `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` (t=65.69; E = 0) `[M: 1511 A1]` · `DeckSlotEntryWidget_2` (E = 2) `[M: 1524 Recipes used]` · slot 2 focused (E = 2; reader not quoted) `[M: 1519 Recipes used, Not examined]` |
| 3 (each step) | `DeckBar` `ui_snapshot` + one relay log line | `_1`, `_2`, `_3` (t≈66.03, 66.36, 66.69); relay lines 0→1 ('deck2'), 1→2 ('deck3'), 2→3 ('deck4') `[M: 1511 A1]` · 2→3→4 in one batch: relay lines 2→3 ('deck4'), 3→4 ('deck5'); `DeckSlotEntryWidget_4` the only focused slot at t=20.35 (the snapshot after the first Right is not quoted) `[M: 1519 C-a]` |
| 4 | `OutlineBorder.BrushColor`, target slot and previous active slot, before and after | slot 3 `(0,0,0,0)` → `(1,0.5,0,1)`; slot 0 `(1,0.5,0,1)` → `(0,0,0,0)` (t=80.80 → 81.40) `[M: 1511 A1]` |
| 4 | log line | `active deck set to 'deck4' — the next match will use it.` `[M: 1511 A1]` |
| 5 | `ActiveDeckName` on disk, in PIE and after PIE | `deck4` both times; every deck's contents unchanged `[M: 1511 A1, Save hygiene]` |
| side effect, checked | `EditingDeckIndex`, `WorkingDeck` | unchanged from the open value: `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards ("set-active did not re-select for editing") `[M: 1511 A1]` · `EditingDeckIndex` stayed 2 through both of `1524`'s set-actives `[M: 1524 A1]` |
| variation (each Left) | `DeckBar` snapshot + one relay log line | slot 3 focused (t=62.63), then slot 2 focused and slot 3 not (t=62.96/62.98); relay `…slot 4 ('deck5') … now 3 ('deck4')`, `…slot 3 ('deck4') … now 2 ('deck3')` `[M: 1524 A1]` |
| variation (restore) | Step 4's reads + Step 5's disk reads on target P | slot 2 `(0,0,0,0)` → `(1,0.5,0,1)`, slot 3 `(1,0.5,0,1)` → `(0,0,0,0)` (t=68.43 → 69.01); `active deck set to 'deck3'`; disk `ActiveDeckName=deck3` in PIE and after PIE `[M: 1524 A1]` |
| downstream (another run) | the next match used the deck | `1512`'s match-start log: `active saved deck 'deck4' is legal (50 cards) — using it this match` `[M: 1512 A1]` |
| corroboration only | a frame you `Read` at ≥ 1280 px | "a thin orange line along the bottom edge of the `deck4` tab and none under `deck1`" (1280×725, t=88.69) `[M: 1511 Pixel note]` |

The observable is the `BrushColor` read + the log line + the disk read. Pixels corroborate at most. `[M: 1511 Pixel note]`; `[L: VER-§12 cl. 7e]`

## Fences — not measured for

- **Set-active targets: slot 3 (deck4) and slot 2 (deck3), each a legal 50-card deck.** Slot 3 was set from E = 0 by `1511` and from E = 2 by `1524`; slot 2 was set by `1524` through the `IA_MenuLeft` variation. `[M: 1511 A1; 1524 A1]` Refusals: slot 4 (deck5, 0 cards). `1511` and `1524` reached it by ONE more `IA_MenuRight` from slot 3 after a set-active, in a later batch. `1511` pressed it once `[M: 1511 A3, Speed data]`; `1524` pressed it twice `[M: 1524 A2]`. Other `T`: `NOT MEASURED` as a set-active. The relay steps 0→1→2→3 and 3→4 (`1511`) and 2→3, 3→4, 4→3, 3→2 (`1524`) are measured. `[L: VER-§8 cl. 12, marker VER-8-12-FENCES-AGREE-WITH-DECK-9-10]` `[M: 1524 A1, A2, Not examined]`
  - **New with `1519`: the walk 2→3→4 in ONE batch from a fresh open**, 2 of 2 with a relay line each. It is new relative to the separately measured 2→3 and 3→4: "The walk 2→3→4 in one batch from a fresh open is new relative to the recipe's measured walks (it measured 2→3 and 3→4 separately) and landed 2/2." `[M: 1519 Recipes used, C-a, Not examined]`
  - **The deck5 refusals gain `1519`'s two:** its control (a), bracketing its probes (t=25.03 and t=139.80), each refused with the active deck staying 'deck3' (Control C3). `[M: 1519 C-a]` They add no set-active target: `1519` set no deck.
- **A target that is already the active deck: `NOT MEASURED`.** No run pressed `IA_MenuSecondary` on the slot that already held the orange outline. The Step 4 before/after reads could not tell a landed press from a dropped one there, because no `BrushColor` would move either way (this recipe's reading; `qa/TASK-1518.md` N3). Pick a target other than Step 0's `ActiveDeckName`.
- **Illegal targets other than an empty deck.** deck1 (51 cards) and deck2 (80 cards) were not driven as targets. That the same gate refuses them rests on the refusal line's own text ("a legal deck is exactly 50"), not on a runtime press. `NOT MEASURED` here. The nearest record is `qa/TASK-1270-verify.md` row 2 (a right-click on a 68-card slot): `unobs` in that report and "carried by the state test `Siegebound.Deck.IllegalDeckCannotBecomeActive`". That is a state test, not a PIE press (`qa/TASK-1518.md` N2).
- **`IA_MenuLeft` on the bar: measured by `1524`, one slot per press, 2 of 2** (4→3, 3→2), one relay line per step, a focus read after each (the variation after Step 5). `[M: 1524 A1, Not examined, Recipe candidates 1]` Left from other slots: `NOT MEASURED`.
- **The bar's two ends** (slot 0 Left, slot 9 Right): not driven. `[M: 1511 Not examined]` Still `NOT MEASURED`: `1524` did not drive them either.
- **Gamepad Y** (`Gamepad_FaceButton_Top`): not driven. `IA_MenuSecondary` was injected directly, so the IMC's key rows were not exercised. `[M: 1511 Not examined]`
- **The real `Home` key** (the Slate doors): unobservable by this rig. It closes on 🧑 his hand check (`1511` A5), recorded beside the verdict and never merged into it. `[M: 1511 A5]`; `[L: DECK-§9 cl. 10; VER-§8 cl. 1, cl. 4]`
- **Right-click** (`1511` A4): unobservable by construction through `ui_perform`, and measured negative through `simulate_key_press` (`VER-§8` cl. 12, marker `VER-8-12-SIMULATE-KEY-PRESS-RMB-MEASURED-NEGATIVE`). This recipe says nothing about right-click. `[L: VER-§8 cl. 12]`
- **Any builder state after a mouse click** (`1509` N4): not reached, because no mouse input was sent. `[M: 1511 Not examined]`
- **A held press.** Every injection here is a tap. `1509` W1 (code review) says door 3 is `Started`-only, so the rig cannot see the held-key repeat. For the held real `Home`/Y repeat, read `DECK-§9` cl. 10's W1 bullet (marker `DECK-9-10-W1-CLOSED-0500D51`); this recipe does not restate its state (`SC-§126` cl. 7). `[L: DECK-§9 cl. 10]` `hold_seconds` on `IA_MenuSecondary`: `NOT MEASURED`.
- **Spacing:** 0.3 s only. "No drops observed at 0.3 s spacing with inject-only input." `[M: 1511 Speed data]` Other spacings: `NOT MEASURED`.
- **That the relay walks the same user's focus as a physical key:** `HYPOTHESIS` in the source ("a physical key was not pressed, so equality with it is not measured"). `[M: 1511 Hypotheses]`
- **Scope of the runs:** `L_MainMenu`'s Deck Builder; standalone single client; one PIE session each; starting `EditingDeckIndex` 0 (`1511`) and 2 (`1524`, `1519`), other starting values `NOT MEASURED`; profile slot `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34`. `[M: 1511 Editor/Aura state; 1524 Editor/Aura state, row 0, Pre-registration; 1519 Editor/Aura state, row 0, Pre-registration]`
- Plugin version: not read.

## Coordinate/resolution-dependent values

**None aimed.** This recipe sends no pointer gesture and aims at no `abs_*` target or screen coordinate; it moves focus with injected actions and reads widget state. The viewport (1280×725) and DPI (0.6706) are recorded under Preconditions for context. `[M: 1511 Editor/Aura state]`

Resolution-dependent, for frames only:

| value | measured at | notes |
|---|---|---|
| composited frame at the default size: 1086×615 | viewport 1280×725 | the orange outline under deck1 was **not** resolvable by eye `[M: 1511 Pixel note]` |
| composited / full-resolution frame: 1280×725 | same | the outline under deck4 **was** visible `[M: 1511 Pixel note]`; law: capture thin UI at `max_dim` ≥ 1280 `[L: VER-§12 cl. 7e]` |

Layout-dependent, not coordinate-dependent: the slot entry names `DeckSlotEntryWidget_0` … `_4` and their deck mapping (Precondition 6), and the tile name `WBP_DeckCardTile_C_0`.

## Known hazards

- **It writes the player's real save.** The active deck changes on disk at the press, not at PIE end. `[M: 1511 A1]`
  - `1511` left deck4 active, as its row required. `[M: 1511 Save hygiene]`
  - Restoring a previous active deck means running this route again on that deck's slot (the `IA_MenuLeft` variation after Step 5, measured once by `1524`), and the gate only accepts a legal (exactly 50) deck. `[M: 1511 A3; 1524 A1]` deck1 read **51** cards before the run `[M: 1511 Pre-registration]`, so it cannot be made active again by this route until it reads 50. That the gate refuses a 51-card deck is the refusal text's reading here, not a press on deck1 (Fences; `qa/TASK-1270-verify.md` row 2's state test is a state test, not a PIE press).
- **Set-active is not select-for-editing.** `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards after the press. `[M: 1511 A1]` In `1524`, `EditingDeckIndex` stayed 2 through both set-actives. `[M: 1524 A1]` Never read `WorkingDeck` as proof of set-active. The converse: a `double_click` on a slot selects it for editing and does not make it active (`RCP-deckbuilder-slot-and-card-edit.md` Step 2).
- **An absent relay line means the relay never ran; it is not a zero** (`1509` *For 5b*, code review). The per-step proof is the snapshot's single `focused: true` plus the relay line. `[M: 1511 A1]`
- **Mouse input before the walk** can split injected keys from real keys (`1509` N4; Precondition 5). Keep the builder mouse-free until Step 4 has landed.
- **A batch cannot branch.** The walk runs to its end whatever the snapshots show, so `T` is fixed before sending. Judge each step from its snapshot after the batch returns. (`README.md` common hazards)
- **Thin UI in small frames.** At 1086 px the outline under deck1 was lost, and deck1's tab also carries the blue editing tint, which confounds it. `[M: 1511 Pixel note]` A leg the frame cannot resolve is labelled by the instrument that did prove it (the `BrushColor` read). `[L: VER-§12 cl. 7e]`
- **No right mouse button through `ui_perform` or `simulate_key_press`.** This recipe is the verifier's set-active route. Do not re-trace the right-button shapes already measured on either tool. See `VER-§8` cl. 12 (marker `VER-8-12-SIMULATE-KEY-PRESS-RMB-MEASURED-NEGATIVE`) and `qa/TASK-1519-verify.md` line 1 `Verdict: MEASURED`. `[L: VER-§8 cl. 12]`
- **Leaving the builder.** `1511` did not leave it; it stopped PIE. `RCP-deckbuilder-slot-and-card-edit.md` Step 6 (`double_click` on Exit) is the measured way out. It is a mouse gesture, so use it only after this recipe's last read.
