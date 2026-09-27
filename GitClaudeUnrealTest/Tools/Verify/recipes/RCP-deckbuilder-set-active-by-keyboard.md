# RCP-deckbuilder-set-active-by-keyboard — make a deck the ACTIVE deck from the Deck Builder with injected menu actions only

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` (the run) · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (downstream corroboration only) · `1509` = `.claude/pipeline/qa/TASK-1509.md` (the pre-compile code review of the route; its sentences are code reads, **not** runtime measurements, and are cited by finding number).

> ⚠️ **Read this first.**
> - **This recipe drives door 3 only:** `inject_input_action IA_MenuSecondary` on a focused deck-bar slot. It is **not** evidence for the real `Home` key or for gamepad Y. Those are code-proven, not runtime-proven. `[L: DECK-§9 cl. 10; VER-§8 cl. 1]`
> - **It changes the player's REAL active deck, on disk, at the press.** `1511` read `ActiveDeckName=deck4` on disk while PIE was still running, right after the press. `[M: 1511 A1]` The run states the value before and after, and the orchestrator hashes the `.sav` files.
> - **Measured once:** one run, one set-active (target slot 3), one refusal (slot 4, empty deck5). `[M: 1511 A1, A3]`

## Source

- `.claude/pipeline/qa/TASK-1511-verify.md`, `Verdict: VERIFIED`, run **2026-09-26**. 1 attempt of 3; PIE clock 0.40 s → 129.48 s. Log times: builder open 00:12:06, set-active 00:13:13, refusal 00:13:52 (*Speed data*).
  - *Editor/Aura state* · *Pre-registration* · *Acceptance lines* rows **0**, **A1**, **A2**, **A3** (A4/A5 are declared ceilings) · *Pixel note* · *Save hygiene* · *Hypotheses* · *Not examined / limitations* · *Speed data* · *Recipes used* · *Recipe candidates* (steps 1–5 + the Fences line).
- The law:
  - `VER-§8` cl. 12, 2026-09-26 amendment (marker `VER-8-12-SET-ACTIVE-ACTUABLE-BY-IA-MENUSECONDARY`): the route, its fences, and what stays a ceiling.
  - `DECK-§9` cl. 10 (marker `DECK-9-10-SET-ACTIVE-ROW`): the route's state label, what is not measured, and `1509` W1 recorded as open.
  - `VER-§12` cl. 7e: capture thin UI at `max_dim` ≥ 1280. `VER-§13` cl. 1, 2026-09-26 ruling: one tool call per batch.
- Code review (`1509`), cited where the run could not see: N4 (a mouse click before the walk), W1 (a held key), and *For 5b* (an absent relay line).
- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26. Not re-verified since; `1511` is the only run.

## Plugin version

**Not read.** `1511` *Not examined*: "Aura plugin version not read." ⇒ the first use of this recipe re-verifies it in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).

## Preconditions

1. Level `/Game/Maps/L_MainMenu`, loaded by `load_level` (`discarded_unsaved: false`). PIE standalone, 1 client, 1280×720 requested → viewport **1280×725**, **DPI 0.6706**. `is_pie_active` read `false` before the run started. `[M: 1511 Editor/Aura state]`
2. **The binaries carry `TASK-1507`'s route.** `DECK-§9` cl. 10 labels it working-tree only until `TASK-1514` commits; read that row's status line for the hash. `[L: DECK-§9 cl. 10]` Check it at runtime: read the builder-open bind line **after** the builder opens. `[M: 1511 row 0]`
   > `UDeckBuilderWidget::BindMenuNavActions: 7 IA_Menu* action(s) bound (Started) on 'PlayerController_0' [IA_MenuUp, IA_MenuDown, IA_MenuLeft, IA_MenuRight, IA_MenuAccept, IA_MenuBack, IA_MenuSecondary]; every IA_Menu* asset resolved.`

   If the line names fewer than seven actions, or reports an unresolved asset, stop: the route is not in this build.
3. **The builder is freshly open, reached by `RCP-menu-to-deckbuilder.md` Steps 0–4.** `1511` re-verified that recipe in the same run: `Button_0` focused at t=4.89; `IA_MenuDown` ×2 in ONE `run_verification_sequence` → `Button_2` `focused:true` at t=9.62; `IA_MenuAccept` at t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14. `[M: 1511 Recipes used]`
4. **`EditingDeckIndex = 0`.** After Step 2, focus lands on `DeckSlotEntryWidget_<EditingDeckIndex>` `[M: 1511 Recipe candidates step 2]`, so the walk counts from that slot. Only 0 was measured. Any other starting value: `NOT MEASURED`.
5. **No mouse input inside the builder before the walk.** `[L: VER-§8 cl. 12, 2026-09-26 amendment]` The reason is a code read: after a mouse click puts focus on a bar slot while `FocusedCardIndex` is still armed, injected `Home`/`Left`/`Right` act on the grid while a real key acts on the bar (`1509` N4). `1511` sent no mouse input at all. `[M: 1511 Not examined]`
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

**Step 2 + Step 3 — ONE sequence: `IA_MenuBack`, then `IA_MenuRight` × T, with a `DeckBar` snapshot after each step.** Shown for T = 3, the measured case:
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
- "`inject_input_action /Game/Input/Actions/IA_MenuRight.IA_MenuRight` × T, `wait_pie_seconds 0.3` + the same `DeckBar` snapshot after each … Steps 2–3 ran as ONE `run_verification_sequence` (4/4 landed)." `[M: 1511 Recipe candidates step 3]`
- **Measured stamps:** `IA_MenuBack` t=65.69 → only `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` focused. Then `IA_MenuRight` t≈66.03 → `_1`, t≈66.36 → `_2`, t≈66.69 → `_3`, "exactly one `focused: true` per snapshot". `[M: 1511 A1]`
- **The wait after `IA_MenuBack`:** not quoted. The stamps (t=65.69 → first Right t≈66.03) fit the same 0.3 s spacing.
- **One relay log line per `IA_MenuRight`** (category `LogGitClaudeUnrealTest`), for example: `IA_MenuRight -> 'Right' handed to Slate's own key route on deck-bar slot 0 ('deck1'); key-down handled=true; the focused bar slot is now 1 ('deck2')`. `[M: 1511 A1]` The candidate text gives the line's prefix as `RelayDeckBarNavigationKeyToSlate:`. `[M: 1511 Recipe candidates step 3]` `1511`'s log census counts 4 relay lines for the whole session `[M: 1511 A2]`, which matches its 4 `IA_MenuRight` presses (3 in A1, 1 in A3). So `IA_MenuBack` printed no relay line. That is a reading from the count, not stated in words.

**Step 4 — ONE sequence: before-reads, `IA_MenuSecondary`, wait 0.5 s, after-reads.** Shown for T = 3, with the active deck before the press on slot 0 (deck1):
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
  - That the before-reads shared this batch is a reading from the stamps, not stated in words. 0.07 s separates the before-read from the press, while this run's separate calls were about 8.5 s or more apart (e.g. t=57.14 → 65.69).
- **Which slot to read as "previous active":** the slot of the Step 0 `ActiveDeckName` (slot map in Precondition 6).
- **The log line** for a legal target: `[00.13.13:066] UDeckBuilderWidget: active deck set to 'deck4' — the next match will use it.` `[M: 1511 A1]` The log reader `1511` used is `NOT MEASURED`.
- **Frames (optional, corroboration only):** `capture_pie_frame` with layer `composited`, at `max_dim` ≥ 1280. `[L: VER-§12 cl. 7e]` `1511`'s before/after pair in this batch came back at the default 1086×615 (`c2` t=80.80, `c3` t=81.45). At that size the before-leg outline under deck1 could not be resolved by eye. `[M: 1511 Pixel note]`
  - Spelling: `max_dim` is a parameter of the standalone `capture_pie_frame` `[S]`. It is not in `run_verification_sequence`'s published parameter union, which does not make it absent (`VER-§12` cl. 7c). Passing it inside a sequence is `NOT MEASURED`.
  - How `1511` obtained its 1280×725 frames (`a1_after_fullres_t88.69s`, `c4` t=111.21) is not quoted.

**Step 5 — read the save again: in PIE after the press, and after PIE.** Same call as Step 0. Source: "**Live disk read in PIE** (after the press): `ActiveDeckName=deck4`. **Disk after PIE stopped**: `ActiveDeckName=deck4`." `[M: 1511 A1]` Every deck's list and total was identical before PIE, in PIE after the press, and after PIE; the only change was `ActiveDeckName` `deck1` → `deck4`. `[M: 1511 Save hygiene]`

**Controls — measured by `1511`, optional, and named in the report if used.**
- **C1, cold open** (A2). Send `IA_MenuSecondary` right after the builder opens, before Step 1. `1511` read `FocusedCardIndex=-1` at t=45.29 (no bar `SlotButton` focused per the t=34.56 `DeckBar` snapshot). Slot 0 stayed `(1,0.5,0,1)` and slot 3 stayed `(0,0,0,0)` at t=45.70; `EditingDeckIndex=0`. `[M: 1511 A2]` Speed data: "cold-open `IA_MenuSecondary` 1 sent, state unchanged by design". `[M: 1511 Speed data]`
- **C2, card tile focused** (A2). In the Step 1 batch, after the `FocusedCardIndex` read, send `IA_MenuSecondary` + the Step 4 reads. `1511`: with `WBP_DeckCardTile_C_0 focused:true` (t=56.71), slot 0 stayed `(1,0.5,0,1)` and slot 3 stayed `(0,0,0,0)` at t=57.12–57.14; `WorkingDeck` was unchanged. `[M: 1511 A2]` The wait inside that batch is not quoted.
- **C3, illegal (empty) target** (A3). After Step 4, one more `IA_MenuRight` + `IA_MenuSecondary` in one batch (2/2 landed), then the BrushColor reads. First re-read the target's total **in PIE**: `1511` read deck5 … deck10 `total=0` before the press. `[M: 1511 A3, Speed data]`
  - `IA_MenuRight` t=110.31 → focus on `DeckSlotEntryWidget_4/OutlineBorder/SlotButton`, relay line `… on deck-bar slot 3 ('deck4'); key-down handled=true; the focused bar slot is now 4 ('deck5')`.
  - `IA_MenuSecondary` t=110.64 → `Warning: UDeckBuilderWidget::SetActiveDeck('deck5'): Deck has 0 cards — a legal deck is exactly 50 (GDD 3.4). — refused; the active deck stays 'deck4'.`
  - At t=111.16–111.19: slot 3 `(1,0.5,0,1)`, slot 4 `(0,0,0,0)`, slot 0 `(0,0,0,0)`. Disk after PIE: `ActiveDeckName=deck4`.
- **The zero carries its control.** `LogGitClaudeUnrealTest` held 9 lines for the whole session (1 bind, 2 focus read-backs, 4 relay, 1 set-active, 1 refusal). There were zero set-active or refusal lines between the builder open (00.12.06) and the first relay (00.12.58), the window that holds both C1 and C2. `[M: 1511 A2]` The same reader returned the A1 set-active line and the A3 refusal line, so that zero is a zero, not a void.

## Read-back

| step | state read that proves it landed | measured value |
|---|---|---|
| precondition | builder-open bind line: 7 `IA_Menu*` actions, every asset resolved | row 0, log `[00.12.06:863]` `[M: 1511 row 0]` |
| 1 | `FocusedCardIndex` | `0` (t=56.67); `WBP_DeckCardTile_C_0 focused:true` `[M: 1511 A1, Recipe candidates step 1]` |
| 2 | `DeckBar` `ui_snapshot`: exactly one `focused: true` | `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` (t=65.69) `[M: 1511 A1]` |
| 3 (each step) | `DeckBar` `ui_snapshot` + one relay log line | `_1`, `_2`, `_3` (t≈66.03, 66.36, 66.69); relay lines 0→1 ('deck2'), 1→2 ('deck3'), 2→3 ('deck4') `[M: 1511 A1]` |
| 4 | `OutlineBorder.BrushColor`, target slot and previous active slot, before and after | slot 3 `(0,0,0,0)` → `(1,0.5,0,1)`; slot 0 `(1,0.5,0,1)` → `(0,0,0,0)` (t=80.80 → 81.40) `[M: 1511 A1]` |
| 4 | log line | `active deck set to 'deck4' — the next match will use it.` `[M: 1511 A1]` |
| 5 | `ActiveDeckName` on disk, in PIE and after PIE | `deck4` both times; every deck's contents unchanged `[M: 1511 A1, Save hygiene]` |
| side effect, checked | `EditingDeckIndex`, `WorkingDeck` | unchanged: `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards ("set-active did not re-select for editing") `[M: 1511 A1]` |
| downstream (another run) | the next match used the deck | `1512`'s match-start log: `active saved deck 'deck4' is legal (50 cards) — using it this match` `[M: 1512 A1]` |
| corroboration only | a frame you `Read` at ≥ 1280 px | "a thin orange line along the bottom edge of the `deck4` tab and none under `deck1`" (1280×725, t=88.69) `[M: 1511 Pixel note]` |

The observable is the `BrushColor` read + the log line + the disk read. Pixels corroborate at most. `[M: 1511 Pixel note]`; `[L: VER-§12 cl. 7e]`

## Fences — not measured for

- **One set-active target: slot 3 (deck4, a legal 50-card deck).** One refusal: slot 4 (deck5, 0 cards), reached by ONE more `IA_MenuRight` from slot 3 after the set-active, in a later batch, not by a four-step walk from slot 0. `[M: 1511 A3, Speed data]` Other `T`: `NOT MEASURED` as a set-active. The relay steps 0→1→2→3 and 3→4 are measured. `[L: VER-§8 cl. 12, 2026-09-26 amendment]`
- **Illegal targets other than an empty deck.** deck1 (51 cards) and deck2 (80 cards) were not driven as targets. That the same gate refuses them rests on the refusal line's own text ("a legal deck is exactly 50"), not on a runtime press. `NOT MEASURED` here.
- **`IA_MenuLeft` on the bar, and the bar's ends** (slot 0 Left, slot 9 Right): not driven. `[M: 1511 Not examined]`
- **Gamepad Y** (`Gamepad_FaceButton_Top`): not driven. `IA_MenuSecondary` was injected directly, so the IMC's key rows were not exercised. `[M: 1511 Not examined]`
- **The real `Home` key** (the Slate doors): unobservable by this rig. It closes on 🧑 his hand check (`1511` A5), recorded beside the verdict and never merged into it. `[M: 1511 A5]`; `[L: DECK-§9 cl. 10; VER-§8 cl. 1, cl. 4]`
- **Right-click** (`1511` A4): unobservable by construction through `ui_perform`. This recipe says nothing about right-click. `[L: VER-§8 cl. 12]`
- **Any builder state after a mouse click** (`1509` N4): not reached, because no mouse input was sent. `[M: 1511 Not examined]`
- **A held press.** Every injection here is a tap. `1509` W1 (code review) says door 3 is `Started`-only, so the rig cannot see the held-key repeat; the held real `Home`/Y repeat is recorded OPEN. `[L: DECK-§9 cl. 10]` `hold_seconds` on `IA_MenuSecondary`: `NOT MEASURED`.
- **Spacing:** 0.3 s only. "No drops observed at 0.3 s spacing with inject-only input." `[M: 1511 Speed data]` Other spacings: `NOT MEASURED`.
- **That the relay walks the same user's focus as a physical key:** `HYPOTHESIS` in the source ("a physical key was not pressed, so equality with it is not measured"). `[M: 1511 Hypotheses]`
- **Scope of the run:** `L_MainMenu`'s Deck Builder; standalone single client; one PIE session; starting `EditingDeckIndex` 0; profile slot `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34`.
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
  - Restoring a previous active deck means running this route again on that deck's slot, and the gate only accepts a legal (exactly 50) deck. `[M: 1511 A3]` deck1 read **51** cards before the run `[M: 1511 Pre-registration]`, so it cannot be made active again by this route until it reads 50. That the gate refuses a 51-card deck is the refusal text's reading here, not a press on deck1 (Fences).
- **Set-active is not select-for-editing.** `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards after the press. `[M: 1511 A1]` Never read `WorkingDeck` as proof of set-active. The converse: a `double_click` on a slot selects it for editing and does not make it active (`RCP-deckbuilder-slot-and-card-edit.md` Step 2).
- **An absent relay line means the relay never ran; it is not a zero** (`1509` *For 5b*, code review). The per-step proof is the snapshot's single `focused: true` plus the relay line. `[M: 1511 A1]`
- **Mouse input before the walk** can split injected keys from real keys (`1509` N4; Precondition 5). Keep the builder mouse-free until Step 4 has landed.
- **A batch cannot branch.** The walk runs to its end whatever the snapshots show, so `T` is fixed before sending. Judge each step from its snapshot after the batch returns. (`README.md` common hazards)
- **Thin UI in small frames.** At 1086 px the outline under deck1 was lost, and deck1's tab also carries the blue editing tint, which confounds it. `[M: 1511 Pixel note]` A leg the frame cannot resolve is labelled by the instrument that did prove it (the `BrushColor` read). `[L: VER-§12 cl. 7e]`
- **No right mouse button through `ui_perform`.** This recipe is the verifier's set-active route. Do not re-trace the `ui_perform` right-button shapes already measured. `simulate_key_press` with `RMB` is being re-measured by `TASK-1519`. `[L: VER-§8 cl. 12]`
- **Leaving the builder.** `1511` did not leave it; it stopped PIE. `RCP-deckbuilder-slot-and-card-edit.md` Step 6 (`double_click` on Exit) is the measured way out. It is a mouse gesture, so use it only after this recipe's last read.
