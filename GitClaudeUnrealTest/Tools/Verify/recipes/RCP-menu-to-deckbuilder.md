# RCP-menu-to-deckbuilder — main menu to the Deck Builder with injected menu actions

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`.

## Source

- `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` **§1(a)** ("Main menu → Deck Builder: menu-nav lane (inject_input_action)"). Session facts come from the same report's `Editor/Aura state:` paragraph.
- Run `PLAYTEST-archer50`, **2026-09-26**, `Verdict: MEASURED`. Ad-hoc run, boarded afterwards as `TASK-1501`.
- Seeded by `TASK-1502`, 2026-09-26. Not re-verified since.

## Plugin version

**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." ⇒ the first use of this recipe re-verifies it in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).

## Preconditions

1. Editor up, Aura connected, no PIE session running: `is_pie_active` → `is_active: false` before starting. `[M: archer50 Editor/Aura state]`
2. Level `/Game/Maps/L_MainMenu`. The source ran `load_level /Game/Maps/L_MainMenu`, which returned `discarded_unsaved: false`. `[M: archer50 Editor/Aura state]`
3. PIE standalone, 1 client, 1280×720 requested → viewport **1280×725**, **DPI 0.6706**. `[M: archer50 Editor/Aura state]`
4. **Starting focus on `Button_0` "Play (vs Bot)".** The source read it at t=4.84 s. The step count below (`IA_MenuDown` ×2) is only correct from that starting focus, so read it before sending anything (Step 0). `[M: archer50 §1(a)]`
5. The main menu's button order, as the source's evidence frame shows it: Play (vs Bot) · Sandbox (No Bot) · Deck Builder · Multiplayer · Settings · Login · Quit. `Button_0` = "Play (vs Bot)" and `Button_2` = "Deck Builder" are the two names the source quotes. `[M: archer50 §1(a) + Evidence]` If the menu gains, loses or reorders an entry, the ×2 is wrong and this recipe must be re-verified (a screen edit, `VER-§13` cl. 3).
6. Optional recording: the source armed the recorder before `start_pie` (`arm_wait_for_pie`), so it started at game t=0.40 s. The output is raw `.h264`; see `README.md` for the remux line. `[M: archer50 Recording paragraph]`

## Steps

**Step 0 — confirm the starting focus.** Read the menu tree and find the node with `focused: true`; expect `Button_0`.
```json
{"widget": "WBP_MainMenu"}
```
Tool: `ui_snapshot`. Source: "At t=4.84 s `Button_0` "Play (vs Bot)" was focused." `[M: archer50 §1(a)]` The source does **not** name the tool that returned the focus flag (`NOT MEASURED`: the reader's name). `ui_snapshot` returns a per-node `focused` field `[S]`, and the same run used `ui_snapshot` on this map (*Not examined*: `ui_snapshot "WBP_MainMenu"`).

**Step 1 — `IA_MenuDown` ×2.**
```json
{"actions": [
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}},
  {"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_MenuDown.IA_MenuDown"}}
]}
```
Tool: `run_verification_sequence`. Source: "`IA_MenuDown` ×2 (t=16.55, 16.97) put focus on `Button_2` "Deck Builder" (`focused: true`, t=17.38)." `[M: archer50 §1(a)]`
- The action path is `[D]`: `Content/Input/Actions/IA_MenuDown.uasset` exists on disk. The source names the action, not its path.
- The envelope (one `run_verification_sequence`) is `[L: VER-§13 cl. 1]` ("a known key sequence, a menu walk — goes into ONE `run_verification_sequence`"). The source does not say whether its two injections were one call or two; they landed 0.42 s apart. `NOT MEASURED`: this exact batched form. If the sequence validator rejects it, send the two as standalone `inject_input_action` calls (the per-action tool §1(a) names) and say so in the report.

**Step 2 — read the focus before accepting.** Same call as Step 0; expect `Button_2` `focused: true`. `[M: archer50 §1(a)]` The source read focus (t=17.38) before it sent Accept (t=21.50). Sending Accept in the same batch as the Downs is `NOT MEASURED`, and a batch cannot branch on its own result (`README.md`, common hazards).

**Step 3 — `IA_MenuAccept`.**
```json
{"action_path": "/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept"}
```
Tool: `inject_input_action`. Source: "`IA_MenuAccept` (t=21.50) opened the builder". `[M: archer50 §1(a)]` Action path `[D]` (`Content/Input/Actions/IA_MenuAccept.uasset`); parameter name `[S]`.

**Step 4 — confirm the builder is open.** Read the builder's live state.
```json
{"widget": "WBP_DeckBuilder", "properties": ["EditingDeckIndex", "WorkingDeck"]}
```
Tool: `get_widget_property_in_pie` `[S]`. Source: "at t=22.53 it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`". The live widget instance is `WBP_DeckBuilder_C_0`. `[M: archer50 §1(a) + A1 row "(live widget read)"]` The source calls this a "live widget read" and does not name the tool (`NOT MEASURED`: the reader's name).

## Read-back

| step | state read that proves it landed | measured value |
|---|---|---|
| 0 | `focused: true` on `Button_0` | t=4.84 s `[M: archer50 §1(a)]` |
| 1–2 | `focused: true` on `Button_2` "Deck Builder" | t=17.38 s `[M: archer50 §1(a)]` |
| 3–4 | `WBP_DeckBuilder_C_0` exists; `EditingDeckIndex = 0`; `WorkingDeck` = `deck1` | t=22.53 s `[M: archer50 §1(a)]` |

`EditingDeckIndex` 0 = deck1 on open. The builder opened on deck1 (Jonathan's deck, untouched throughout the source run). Nothing here edits it. `[M: archer50 §1(a)]`

## Fences — not measured for

- Any starting focus other than `Button_0`, and any menu reached by `Back` from a sub-screen (after Exit the source saw **two** `WBP_MainMenu` instances; see hazards).
- Any other menu target: "Play (vs Bot)" by `IA_MenuAccept` is `TASK-1512`'s to measure (`README.md`, Pending). Sandbox, Multiplayer, Settings, Login and Quit were not driven.
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
- A focus read that does not show `Button_2` means **do not send Accept**: Accept would open whichever screen is focused. Whether that screen can be left without ending the PIE session is outside this recipe (`VER-§12` cl. 5).
