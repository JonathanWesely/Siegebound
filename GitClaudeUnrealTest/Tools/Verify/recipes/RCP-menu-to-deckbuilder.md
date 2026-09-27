# RCP-menu-to-deckbuilder — main menu to the Deck Builder with injected menu actions

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` · `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md` · `1519` = `.claude/pipeline/qa/TASK-1519-verify.md`.

## Source

- `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` **§1(a)** ("Main menu → Deck Builder: menu-nav lane (inject_input_action)"). Session facts come from the same report's `Editor/Aura state:` paragraph.
- Run `PLAYTEST-archer50`, **2026-09-26**, `Verdict: MEASURED`. Ad-hoc run, boarded afterwards as `TASK-1501`.
- Seeded by `TASK-1502`, 2026-09-26.
- **Re-verified in-run by `TASK-1511`**, 2026-09-26, `Verdict: VERIFIED`, 1 PIE session. `1511` *Recipes used*, verbatim: "Steps 0–4 (focus read `Button_0` t=4.89; `IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62; `IA_MenuAccept` t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14) · re-verified in-run **y** (first use; the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections)." Also used: its *Speed data* (sent vs landed) and its `Editor/Aura state:` paragraph.
- **Used in part by `TASK-1512`**, 2026-09-26: "Step 0 only (focus read of `Button_0` "Play (vs Bot)"); the recipe's `IA_MenuDown`×2 was not used." `[M: 1512 Recipes used]`
- Amended by `TASK-1516`, 2026-09-26, to record the two runs above.
- **Used by `TASK-1524`**, 2026-09-26, `Verdict: VERIFIED`, 1 PIE session: "Steps 0–4 · re-verified in-run **y, with a mismatch**: Step 1's batched `IA_MenuDown` ×2 (the recipe's JSON, no wait step) landed **1 of 2** at 1-frame spacing (t=6.81, 6.82); `1511`'s 2/2 was at 3 frames. A top-up of 1 fixed it. Step 4 found `EditingDeckIndex = 2`, not the recipe's `0`/deck1 (the active deck was deck3; see H2). Reported, not patched." `[M: 1524 Recipes used]` Also used: its *Not examined* sent-vs-landed line, *Pre-registration*, row 0, *Editor/Aura state*, *Hypotheses* H2 and *Recipe candidates* 2–3.
- Amended by `TASK-1527`, 2026-09-26, to record `1524`: Step 1 (the second measurement of the batched form), Step 2 (the top-up), Step 4 and the Read-back (the open slot).
- **Used by `TASK-1519`**, 2026-09-27, `Verdict: MEASURED`, 1 PIE session: "Steps 0–4 · re-verified in-run **y**: Step 0 `Button_0` focused (t=6.70); Step 1 run with a `wait_pie_seconds 0.3` between the two Downs (a spacing the recipe marks `NOT MEASURED`) → 2/2 landed, `Button_2` focused t=7.35; Step 3 Accept t=11.42; Step 4 `EditingDeckIndex = 2` with deck3 active (matches the recipe's amended expectation)." `[M: 1519 Recipes used]` Also used: its *Recipe candidates* 2, its *Not examined* sent-vs-landed line, row 0, *Pre-registration* and *Editor/Aura state*.
- Amended by `TASK-1533`, 2026-09-27, to record `1519`: Step 1 (its third measurement, the waited form), Steps 0, 2, 3 and 4 and the Read-back (the stamps above), and `qa/TASK-1528.md` Q1 and Q4.

## Plugin version

**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." `1511`, `1512`, `1524` and `1519` did not read it either ("Aura plugin version not read", each *Not examined*). ⇒ the first use re-verified the recipe in-run (`1511`). The version is still unread, so a plugin update since then cannot be ruled out (`VER-§13` cl. 3, `VER-§8` cl. 5).

## Preconditions

1. Editor up, Aura connected, no PIE session running: `is_pie_active` → `is_active: false` before starting. `[M: archer50 Editor/Aura state]`
2. Level `/Game/Maps/L_MainMenu`. The source ran `load_level /Game/Maps/L_MainMenu`, which returned `discarded_unsaved: false`. `[M: archer50 Editor/Aura state]`
3. PIE standalone, 1 client, 1280×720 requested → viewport **1280×725**, **DPI 0.6706**. `[M: archer50 Editor/Aura state]`
4. **Starting focus on `Button_0` "Play (vs Bot)".** The source read it at t=4.84 s. The step count below (`IA_MenuDown` ×2) is only correct from that starting focus, so read it before sending anything (Step 0). `[M: archer50 §1(a)]` `1511`, `1512` and `1519` found the same starting focus; `1524` does not quote it. Stamps: `1511` at t=4.89, `1512` at menu t=20.42, `1519` at t=6.70. `[M: 1511 Recipes used; 1512 A1; 1519 Recipes used]`
5. The main menu's button order, as the source's evidence frame shows it: Play (vs Bot) · Sandbox (No Bot) · Deck Builder · Multiplayer · Settings · Login · Quit. `Button_0` = "Play (vs Bot)" and `Button_2` = "Deck Builder" are the two names the source quotes. `[M: archer50 §1(a) + Evidence]` If the menu gains, loses or reorders an entry, the ×2 is wrong and this recipe must be re-verified (a screen edit, `VER-§13` cl. 3).
6. Optional recording: the source armed the recorder before `start_pie` (`arm_wait_for_pie`), so it started at game t=0.40 s. The output is raw `.h264`; see `README.md` for the remux line. `[M: archer50 Recording paragraph]`

## Steps

**Step 0 — confirm the starting focus.** Read the menu tree and find the node with `focused: true`; expect `Button_0`.
```json
{"widget": "WBP_MainMenu"}
```
Tool: `ui_snapshot`. Source: "At t=4.84 s `Button_0` "Play (vs Bot)" was focused." `[M: archer50 §1(a)]` The source does **not** name the tool that returned the focus flag. `ui_snapshot` returns a per-node `focused` field `[S]`, and the same run used `ui_snapshot` on this map (*Not examined*: `ui_snapshot "WBP_MainMenu"`).
- **Re-verified:** `1511` read `Button_0` focused at t=4.89 and does not name the reader. `[M: 1511 Recipes used]` `1519` read `Button_0` focused at t=6.70; its *Recipes used* line does not name the reader either. `[M: 1519 Recipes used]`
- **The reader, named:** `1512` read the same focus with `ui_snapshot` and a name selector, which returned `focused: true` and the text "Play (vs Bot)" (menu t=20.42): `[M: 1512 A1; Recipe candidates item 1]`
```json
{"widget": "WBP_MainMenu", "selector": {"by": "name", "value": "Button_0"}, "max_depth": 2}
```
  The object is quoted from `1512` *Recipe candidates* item 1. There it ran as a `run_verification_sequence` step; its step envelope is not quoted.

**Step 1 — `IA_MenuDown` ×2.** Two forms, three measurements of this step: the batched form (the block below) twice, with different results (`1511`, `1524`), and the waited form (the second block) once (`1519`). Step 2's focus read and top-up are mandatory whichever form ran.
```json
{"actions": [
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}}
]}
```
Tool: `run_verification_sequence`. Source: "`IA_MenuDown` ×2 (t=16.55, 16.97) put focus on `Button_2` "Deck Builder" (`focused: true`, t=17.38)." `[M: archer50 §1(a)]`
- The action path is `[D]`: `Content/Input/Actions/IA_MenuDown.uasset` exists on disk. The source names the action, not its path.
- The envelope (one `run_verification_sequence`) is `[L: VER-§13 cl. 1]` ("a known key sequence, a menu walk — goes into ONE `run_verification_sequence`"). `archer50` does not say whether its two injections were one call or two; they landed 0.42 s apart.
- ⚠️ **The batched form: measured TWICE, with different results. It is NOT reliable: Step 2's focus read and top-up are mandatory after it.**
  - **`1511`: 2 of 2.** "`IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62", and "the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections". `[M: 1511 Recipes used]` Sent vs landed: "menu `IA_MenuDown`×2 → 2 landed (focus `Button_2`, 2 subsystem log lines)". `[M: 1511 Speed data]`
  - **`1524`: 1 of 2**, with this JSON and no wait step, at 1-frame spacing (t=6.81, 6.82). `[M: 1524 Recipes used]` Sent vs landed: "menu `IA_MenuDown` ×2 (one batch, no wait, 0.017 s apart) → **1 landed** (focus `Button_1`)". `[M: 1524 Not examined]`
  - **Both runs:** `L_MainMenu`, standalone, 1 client, viewport 1280×725, DPI 0.6706. `[M: 1511 Editor/Aura state; 1524 Editor/Aura state]` Starting focus `Button_0` is quoted by `1511`. `[M: 1511 Recipes used]`
  - `1511` does not quote its action list. Whether its 3 frames came from a wait step or from the runner's own spacing is `NOT MEASURED`, and the JSON above carries no wait step.
  - **Spacing between the two Downs.** The spacings measured where both landed are `1511`'s 3 frames (mechanism above: `NOT MEASURED`), `archer50`'s 0.42 s (t=16.55 → 16.97; whether one call or two is not stated, see above) and `1519`'s `wait_pie_seconds 0.3` between them (the waited form below; the two Downs' own stamps are not quoted). `1524`'s 1 frame lost one. "`wait_pie_seconds 0.3` between the two `IA_MenuDown`" was proposed by `1524` `[M: 1524 Recipe candidates 2]` and measured once by `1519` `[M: 1519 Recipes used, Recipe candidates 2]`. Any other wait is `NOT MEASURED`; a run that uses one names its spacing and labels it.
  - If a later validator rejects the batched form, send the two as standalone `inject_input_action` calls (the per-action tool `archer50` §1(a) names) and say so in the report.

**Step 1, the waited form — measured once (`1519`).** The same two Downs with `wait_pie_seconds 0.3` between them. This recipe prefers this form. That is a choice resting on one run, not a reliability claim.
```json
{"actions": [
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}},
  {"type": "wait_pie_seconds", "params": {"seconds": 0.3}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}}
]}
```
Tool: `run_verification_sequence`.
- **Measured once:** "Step 1 run with a `wait_pie_seconds 0.3` between the two Downs (a spacing the recipe marks `NOT MEASURED`) → 2/2 landed, `Button_2` focused t=7.35". `[M: 1519 Recipes used]` Sent vs landed: "menu `IA_MenuDown` ×2 with `wait_pie_seconds 0.3` between → **2 landed** (`Button_2` focused t=7.35; no top-up needed)". `[M: 1519 Not examined]` The report's candidate says the same: "landed 2/2 here (one run)". `[M: 1519 Recipe candidates 2]`
- **The run:** `L_MainMenu`, standalone, 1 client, viewport 1280×725, DPI 0.6706; starting focus `Button_0` at t=6.70. `[M: 1519 Editor/Aura state, Recipes used]`
- **Not quoted, so `NOT MEASURED`:** `1519` does not quote its action list. This block's envelope (one `run_verification_sequence`) and the two Downs' own stamps are therefore not measured. The wait object is spelled as in `RCP-deckbuilder-set-active-by-keyboard.md` Steps 1–3 (param name `[S]`).
- **What one run does not show:** a drop rate at 0.3 s (`NOT MEASURED`). One 2 of 2 does not show that this form cannot drop a Down, so **Step 2's focus read and top-up stay mandatory after it**, as after the batched form.

**Step 2 — read the focus before accepting (mandatory whichever Step 1 form ran: the batched form has landed 1 of 2).** Same call as Step 0; expect `Button_2` `focused: true`. `[M: archer50 §1(a)]` The source read focus (t=17.38) before it sent Accept (t=21.50). `1511` read `Button_2` focused at t=9.62 and sent Accept at t=14.60 `[M: 1511 Recipes used]`; whether that read shared the Downs' batch is not stated. `1519` read `Button_2` focused at t=7.35 after the waited form, needed no top-up, and sent Accept at t=11.42 `[M: 1519 Recipes used, Not examined]`; whether that read shared the Downs' batch is not stated either. Sending Accept in the same batch as the Downs is `NOT MEASURED`, and a batch cannot branch on its own result (`README.md`, common hazards).
- **If it reads `Button_1`: top up with ONE `IA_MenuDown`** (Step 1's action object, sent once), then read again and expect `Button_2`. Measured once: after Step 1 landed 1 of 2 (focus `Button_1`), "top-up 1 → landed (`Button_2`, t=13.76)". `[M: 1524 Not examined, Recipes used]` The top-up's envelope (a standalone call or a one-action sequence) is not quoted: `NOT MEASURED`.
- **If it reads `Button_2`:** go to Step 3.
- **Anything else** (e.g. `Button_0`, neither Down landed): `NOT MEASURED`. Do not send Accept (Known hazards).

**Step 3 — `IA_MenuAccept`.**
```json
{"action_path": "/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept"}
```
Tool: `inject_input_action`. Source: "`IA_MenuAccept` (t=21.50) opened the builder". `[M: archer50 §1(a)]` Re-verified: "`IA_MenuAccept` t=14.60 → `WBP_DeckBuilder_C_0`"; sent vs landed "`IA_MenuAccept` 1/1 (builder open)". `[M: 1511 Recipes used, Speed data]` Used again: "Step 3 Accept t=11.42"; sent vs landed "`IA_MenuAccept` 1/1". `[M: 1519 Recipes used, Not examined]` Action path `[D]` (`Content/Input/Actions/IA_MenuAccept.uasset`); parameter name `[S]`.

**Step 4 — confirm the builder is open.** Read the builder's live state.
```json
{"widget": "WBP_DeckBuilder", "properties": ["EditingDeckIndex", "WorkingDeck"]}
```
Tool: `get_widget_property_in_pie` `[S]`. Source: "at t=22.53 it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`". The live widget instance is `WBP_DeckBuilder_C_0`. `[M: archer50 §1(a) + A1 row "(live widget read)"]` Re-verified: "`WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14". `[M: 1511 Recipes used]` Neither run names the tool: the source calls this a "live widget read" (`NOT MEASURED`: the reader's name).
- ⚠️ **Expect the ACTIVE deck's slot, not `0`.** `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"*. `[L: DECK-§3]` Measured at two points:
  - `0` when deck1 was active: `archer50` (its `ActiveDeckName` read `deck1`, "unchanged", after the run) and `1511` (its Pre-registration read `ActiveDeckName = deck1`). `[M: archer50 §1(a), Not-examined; 1511 Recipes used, Pre-registration]`
  - `2` when deck3 was active: "Builder open t=19.83: `WBP_DeckBuilder_C_0`, `EditingDeckIndex=2`", with `ActiveDeckName = deck3` in the pre-registration. `[M: 1524 row 0, Pre-registration, Recipes used]` Read again by `1519`: "Builder t=12.24: `WBP_DeckBuilder_C_0`, `EditingDeckIndex=2`", with `ActiveDeckName = deck3` in its pre-registration; its *Recipes used*: "Step 4 `EditingDeckIndex = 2` with deck3 active". `[M: 1519 row 0, Pre-registration, Recipes used]` That is a second observation of the same pair, not a new value.
  - That the builder does this in general is `HYPOTHESIS` H2 in `1524` ("One observation; mechanism not read"). `[M: 1524 H2]` It stays a hypothesis after `1519`.
  - ⇒ Read the value and report it; never assume it. A recipe that walks the deck bar from here counts from this read (`RCP-deckbuilder-set-active-by-keyboard.md` Precondition 4).

## Read-back

| step | state read that proves it landed | measured value |
|---|---|---|
| 0 | `focused: true` on `Button_0` | t=4.84 s `[M: archer50 §1(a)]` · t=4.89 `[M: 1511 Recipes used]` · menu t=20.42 `[M: 1512 A1]` · t=6.70 `[M: 1519 Recipes used]` |
| 1–2 | `focused: true` on `Button_2` "Deck Builder" | t=17.38 s `[M: archer50 §1(a)]` · t=9.62, after the batched Downs `[M: 1511 Recipes used]` · t=13.76, after the batched Downs landed 1 of 2 (focus `Button_1`) and a one-Down top-up `[M: 1524 Not examined]` · t=7.35, after the waited Downs landed 2 of 2, no top-up `[M: 1519 Recipes used, Not examined]` |
| 3–4 | `WBP_DeckBuilder_C_0` exists; `EditingDeckIndex` = the ACTIVE deck's slot (`DECK-§3`); `WorkingDeck` = that deck | `0` / `deck1`, deck1 active: t=22.53 s `[M: archer50 §1(a)]` · t=15.14 `[M: 1511 Recipes used]` · `2`, deck3 active: t=19.83 (`WorkingDeck` not quoted) `[M: 1524 row 0]` · t=12.24 (`WorkingDeck` not quoted) `[M: 1519 row 0]` |

`EditingDeckIndex` on open is the active deck's slot (`DECK-§3`; Step 4). In the source run it was 0 = deck1, which was active (Jonathan's deck, untouched throughout the source run) `[M: archer50 §1(a), Not-examined]`; in `1524` and `1519` it was 2 = deck3, which was active `[M: 1524 row 0, Pre-registration; 1519 row 0, Pre-registration]`. Nothing here edits the deck.

## Fences — not measured for

- Any starting focus other than `Button_0`, and any menu reached by `Back` from a sub-screen (after Exit the source saw **two** `WBP_MainMenu` instances; see hazards).
- Any other menu target. "Play (vs Bot)" by `IA_MenuAccept` from `Button_0` was measured by `1512` (A1: `IA_MenuAccept` at menu t=20.44, then world `L_Arena`; 1/1 in both attempts, *Speed data*). It is its own recipe now: `RCP-vsbot-capture-center-and-summon.md` (`TASK-1515`). Sandbox, Multiplayer, Settings, Login and Quit were not driven.
- **The Downs.** The batched form: two runs, two results, 2 of 2 at 3 frames (`1511`) and 1 of 2 at 1 frame (`1524`). The waited form, `wait_pie_seconds 0.3`: one run, 2 of 2 (`1519`). A drop rate at any spacing is `NOT MEASURED`, and so is any wait other than 0.3 s (Step 1).
- **The top-up.** One `IA_MenuDown` from `Button_1`, measured once (`1524`). A top-up from `Button_0`, or of more than one Down: `NOT MEASURED`.
- **The open slot.** `EditingDeckIndex` at open was read with deck1 active (0) and deck3 active (2) only. Other active decks: `NOT MEASURED` (Step 4).
- `IA_MenuUp`, `IA_MenuLeft`/`Right`/`Back`, gamepad, and real keyboard keys. A human's real Down key is a different lane (`VER-§8` cl. 11) and is not what this recipe drives.
- Any map other than `L_MainMenu`, any PIE mode other than standalone single client, multiplayer PIE.
- Whether the Downs and the Accept can share one batch safely.
- Plugin version: not read.

## Coordinate/resolution-dependent values

**None.** This recipe aims at no `abs_*` target and no screen coordinate: it moves focus with injected actions and reads widget state. The source's viewport and DPI (1280×725, DPI 0.6706) are recorded under Preconditions for context only. `[M: archer50 Editor/Aura state]`
Layout-dependent (not coordinate-dependent): the ×2 depends on the menu's button order (Precondition 5).

## Known hazards

- **`simulate_key_press` is not a menu route on this map.** It is delivered to the player controller, not to Slate. Source: `simulate_key_press "Tab"` returned *"Key Tab was delivered to the player controller, but NOTHING BINDS IT: applied mapping context(s) IMC_MainMenu…"* and focus did not move. `[M: archer50 §1(b)]`; law `VER-§5` cl. 5. Use `inject_input_action`.
- **`binding_found` is not the observable** (`VER-§8` cl. 10). The observable here is focus and builder state. `[M: archer50 Not-examined]`
- **Two `WBP_MainMenu` instances after returning from the builder.** After Exit, `ui_snapshot "WBP_MainMenu"` returned *"matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C"* (the Back handler creates a new menu instance). Observed, not diagnosed. `[M: archer50 Not-examined]` If you re-enter the builder from a returned menu, disambiguate (`selector` with `match_index`) and re-read focus first; that path is `NOT MEASURED`.
- **The batched Downs can drop one.** `1524` landed 1 of 2 with this recipe's no-wait JSON; a one-Down top-up after the focus read fixed it (Step 2). `[M: 1524 Recipes used, Not examined]` The waited form landed 2 of 2 in its one run `[M: 1519 Recipes used]`; that does not show it cannot drop one, so Step 2 runs after it too.
- A focus read that does not show `Button_2` means **do not send Accept**: Accept would open whichever screen is focused. Whether that screen can be left without ending the PIE session is outside this recipe (`VER-§12` cl. 5).
