# TASK-1502 — programmer handoff — [VERIFY-RECIPE-LIBRARY]

Marker `TASK-1502-VERIFY-RECIPE-LIBRARY` · gameplay-programmer · 2026-09-26 · status → `ready-for-qa` · gate `TASK-1505` · host `TASK-1506`.

## What was done

Created `Tools/Verify/recipes/` and seeded it with the three measured sequences, from `qa/PLAYTEST-archer50-verify.md` and `qa/TASK-1391-verify.md` only. Docs only: no engine, PIE, compile, source, asset or git action.

## Files written (all NEW)

| path | lines | content |
|---|---|---|
| `Tools/Verify/recipes/README.md` | 79 | what a recipe is / is not (`VER-§13` cl. 3) · the 8-heading format · **provenance tags** · recipe table (file · what · source + section · last-verified) · **Pending** list (`TASK-1511`, `TASK-1512`, plus the placement-entry gap → `TASK-1512`) · common hazards · the `VER-§12` cl. 7b remux line |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | 86 | archer50 §1(a): focus read → `IA_MenuDown` ×2 → focus read → `IA_MenuAccept` → builder read |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | 154 | archer50 §1(b)/(c)/A1 (+ optional exit from §1(e)): resolve with `ui_snapshot` → slot `double_click` dx −45 → calibrate one gesture → batched `double_click`s → read-back / top-up; measured drop table; `name_path` hazard |
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` | 141 | 1391 C2–C6/§3/§4: hand read first → pick the card with its reason → **[entry step NOT MEASURED]** → one batch: control arm then `SetMouseLocation(300,420)` + `LeftMouseButton` with dependent reads |
| `.claude/pipeline/handoffs/TASK-1502-programmer.md` | — | this file |
| `.claude/pipeline/TASKBOARD.md` | 1 line | this row's `status:` only |

Every recipe has all eight headings exactly once, in order (checked with `grep -cx` per heading: 8 × `[1]` in each of the three files). Every ```json block parses (`json.loads`: 7/7, 4/4, 5/5).

## How the "nothing unmeasured" rule is made checkable

The README defines provenance tags, and every value-bearing line carries one:
- `[M: <run> <section>]` — measured; the value is quoted from that section.
- `[S]` — parameter **spelling** from the tool's JSON schema, which I loaded with `ToolSearch` (no tool was called) for `inject_input_action`, `run_verification_sequence`, `ui_perform`, `ui_snapshot`, `get_widget_property_in_pie`, `get_actor_property_in_pie` and `simulate_key_press`. Used only where the source names a tool and a value but not the field name (e.g. `"key": "LeftMouseButton"`). Not a measurement.
- `[D]` — read from disk at authoring: the `Content/Input/Actions/IA_MenuDown.uasset` / `IA_MenuAccept.uasset` paths, and C++ members (`DeckComponent`/`GhostActor` on `ASiegePlayerController`, `Gold` on `ASiegePlayerState`, cited by text per `SC-§126` cl. 12). Not a measurement.
- `[L: clause]` — required by law and not measured by the source (the batching envelope, top-up sizing, the no-total-cap overshoot guard).
- `NOT MEASURED` / `HYPOTHESIS` — the source's silence, and its own labels.

## QA map — every step to the sentence that measured it

**RCP-menu-to-deckbuilder** (all archer50 §1(a) unless noted)
- Step 0 focus `Button_0` ← "At t=4.84 s `Button_0` "Play (vs Bot)" was focused." (reader's tool name: NOT MEASURED)
- Step 1 `IA_MenuDown` ×2 ← "`IA_MenuDown` ×2 (t=16.55, 16.97) put focus on `Button_2`". Batch envelope `[L]`; path `[D]`.
- Step 2 focus `Button_2` ← "(`focused: true`, t=17.38)"
- Step 3 `IA_MenuAccept` ← "`IA_MenuAccept` (t=21.50) opened the builder"
- Step 4 builder read ← "at t=22.53 it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`" (tool name: NOT MEASURED; `[S]`)

**RCP-deckbuilder-slot-and-card-edit**
- Step 1 resolve targets ← *Not examined*: "resolved targets by plain name and verified each one with `ui_snapshot` first"; geometry ← §1(b) "abs 1674.9, 870.9, 121×31", §1(c) tile rects
- Step 2 slot `double_click` dx −45 ← §1(b) table last row (EditingDeckIndex → 3, t=215.33); root name ← §1(b) error text
- Step 3 calibration ← §1(c) "Ogre **50 → 49** (t=304.53)"; offset values NOT MEASURED (not in the report)
- Step 4 batches ← §1(c) the 49/10/30/28-gesture runs with their spacing and reads; envelope `[L]`+`[S]`, NOT MEASURED as a form
- Step 5 read-back/top-up ← A1 (i) "live widget read"; §1(c) 27 → 50; exact-shortfall sizing `[L]`
- Step 6 exit ← §1(e) "`ui_snapshot` `{by:text, value:"Exit"}` → `TextBlock_10`" + "returned to the main menu. Frame t=595.35"
- Read-back disk/log ← A1 (ii)/(iii)

**RCP-play-unit-card-from-hand** (all `qa/TASK-1391-verify.md`)
- Step 1 hand read ← C2 "`get_actor_property_in_pie(component="DeckComponent", properties=["Hand"])`, taken as the **first** action"; actor name ← §1/Not-examined `"name": "SiegePlayerController0"` + `[D]`
- Step 2 card choice ← C2 "CARD + REASON (A) … (B) …"; 0-based slot reading ← C2/C5 quoted arrays
- Step 3 entry ← **none; NOT MEASURED** (see conflict 1)
- Step 4 batch order/indices ← C3; control ← C4, §3; SET op ← Not-examined verbatim `{"op": "call_actor_function", "name": "SiegePlayerController0", "function": "SetMouseLocation", "args": {"X": 300, "Y": 420}}`; reply ← §1; confirm ← §3 "`simulate_key_press "LeftMouseButton"` — identical"; wait ← §4 `wait_pie_seconds(16)`
- Step 5 refusal log ← C6 `total_matches = 2`
- Read-back ← C4, C5, C6, §3

## Conflicts and findings flagged (the row wins; I followed its rule (3) where (1) and (3) pull apart)

1. **`IA_Card<N>` is named by the row, not by the source.** Spec (1) lists "`IA_Card<N>` on the slot holding the card" as a `TASK-1391` step. `qa/TASK-1391-verify.md` never names the action that entered placement mode, its parameters, its batch index, or a slot-to-action mapping (grep: `IA_Card` does not occur in the report; `inject_input_action` appears only in the wall (a) discussion and the dead-end list, never as an action the run took). Its own row spec does not prescribe one either. Under spec (3) and the Acceptance line ("no step the cited report did not measure"), the recipe carries **no entry step**: Step 3 is a declared `NOT MEASURED` gap, with pointers to `VER-§8` cl. 7 ("ENTERING PLACEMENT WORKS") and `VER-§5` cl. 5 labelled as other runs' measurements. ⚠️ **`TASK-1512` A3 relies on this recipe for exactly that step**; the README's Pending list assigns it to `TASK-1512` as a recipe candidate. Manager/orchestrator: decide whether `TASK-1512`'s dispatch should name the entry route explicitly.
2. **Envelope discrepancy on the deck edits.** `VER-§13`'s preamble and the `TASK-1501` row (orchestrator's sighting) say deck editing "ran one gesture per tool call (100 edits)". archer50 §1(c) reports runs of 49, 10, 30 and 28 gestures "at N-frame spacing", which is a batched shape, and does not name the call. I recorded §1(c)'s numbers and did not pick. This matters for `TASK-1512`'s per-segment speed comparison (`SC-§138` (b)). Manager's to resolve (`SC-§101`).
3. **archer50 §1(e) bears on `VER-§12` cl. 5 ((d)(iv)).** That clause says no verify-lane instrument can close a sub-screen and names `ui_perform` a dead end. archer50 §1(e) measured a `ui_perform` `double_click` on the Deck Builder's Exit text returning to the main menu (frame t=595.35, positive-control description in *Evidence*). The 2026-09-26 amendment narrowed `VER-§5` cl. 5 but I found no matching note on `VER-§12` cl. 5. The recipe records only the one measured screen as an optional step. The amendment, if any, is the manager's (`SC-§101`).
4. **`TASK-1512` A3 goes outside this recipe's fence.** It plans "`SetMouseLocation` onto a zone point". The source makes **no accuracy claim** ("Reproducible ≠ calibrated"), measured **one** screen point `(300,420)` from the hero's start camera at the Blue spawn, and its refusal text names the legal regions as "the Blue spawn box and any Blue-owned capture zone". A capture-zone placement is therefore unmeasured and, per that text, needs the zone Blue-owned first. The recipe's Fences say this. It is for the orchestrator and manager to weigh before dispatching `TASK-1512`, not a ruling.
5. **`TASK-1391` attempt B's timeline admits more than one reading.** C2 (hand reads at t=15.1659 and 31.2163), §4 ("one batch, everything inside it … in-batch `wait_pie_seconds(16)`") and C3 ("one call, 25 actions, t=31.1996 → 32.0731 s") do not fit together one way only, and "a batch cannot branch on its own result" (§4) sits awkwardly with a card chosen inside the same batch. The recipe states only the unambiguous parts and prescribes the hand read in a call before the play batch (attempt A's shape).
6. **Schema observation (not a measurement):** `run_verification_sequence`'s published parameter union does not list `component`, `properties` or `widget`, yet 1391 C2 ran `get_actor_property_in_pie` with `component`/`properties` inside a batch. So the union is incomplete, and whether `ui_snapshot`/`ui_perform`/`get_widget_property_in_pie` accept their root `widget` inside a sequence is unmeasured. The menu recipe's batched step therefore contains only `inject_input_action` (`action_path` is in the union), and every widget read is a standalone call.
7. **Schema text only, for the manager's awareness:** `simulate_key_press`'s schema lists an `RMB` shorthand. archer50 explicitly did **not** try `simulate_key_press` for the right button (*Not examined*: "Not tried: `SetMouseLocation` + `simulate_key_press "RightMouseButton"` …"), reasoning that it reaches the player controller rather than Slate, where the right-click handler lives. No recipe was written and nothing is proposed; it is recorded because `VER-§8` cl. 12's scope is `ui_perform` only.

## Assets and paths referenced

- Input actions `[D]`: `/Game/Input/Actions/IA_MenuDown.IA_MenuDown`, `/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept` (both `.uasset` present under `Content/Input/Actions/`).
- Maps: `/Game/Maps/L_MainMenu`, `/Game/Maps/L_Arena` (from the reports).
- Save slot quoted from archer50: `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34` (profile-specific).

## What QA should scrutinise

- The three steps where I filled a parameter **spelling** from the schema (`[S]`): these are the most likely place for a reviewer to call "unmeasured". My position is that the step is measured and only the field name is filled, and each one is tagged.
- The `dy: 0` fill on the slot offset and the `"<from Step 1>"` placeholders for tile offsets (the report gives no tile offset values).
- The "0-based index 1" reading of 1391's "slot 1" (derived from the report's own quoted arrays, not stated in words).
- The optional Step 6 (exit) in the deck-builder recipe: measured in §1(e) but outside the row's listed §1(b)/(c) scope. It can be removed without affecting the rest.

## Not examined / limitations

- **No recipe has been executed.** Every recipe is a claim from its source run (`SC-§101`), and none has been re-run. Plugin version was not read in either source run, so first use re-verifies in-run.
- I read only the two named reports, `CONVENTIONS.md` `VER-§5`/`§8`/`§12`/`§13`, the `TASK-1391` row and the `TASK-1511`/`TASK-1512` rows (to name the pending rows by title, `SC-§141`). I did **not** read `qa/TASK-1352`/`1357`/`1348`/`1390`/`1230` reports, so nothing from them was seeded; where the law cites them, the recipe points at the law and labels it.
- Tool schemas were loaded, not exercised, and some schema descriptions arrived truncated. `[S]` spellings could still be wrong; the first live use will show it.
- `[D]` facts (asset paths, C++ members) were read from the working tree on 2026-09-26. They are not runtime measurements and can drift with edits.
- Tile-button offsets, the census instrument, the log-read tool, the ghost's `bHidden`/location read paths, and the reader tools for focus and builder state are all `NOT MEASURED` in the sources and are marked so in the recipes.
- No `.claude/agents/*`, `CLAUDE.md`, `CONVENTIONS.md`, `qa/*` report, source file, asset, PIE or editor action was taken. Git: no write of any kind; one read-only `git status --porcelain --untracked-files=all` over the paths I wrote, which showed the 4 recipe files and this handoff as untracked (`??`) and `TASKBOARD.md` modified. Those are the host's (`TASK-1506`) to stage. `CLAUDE.md`: not authored, not staged.

## QA loop 1 — 2026-09-26 (`qa/TASK-1505.md` §1: B1 · W1 · N1–N4 · M2)

Status → `ready-for-qa`. Loop 1 of 3. Docs only; no engine, PIE, compile, source, asset or git action. Scope held to `Tools/Verify/recipes/*`, this section, and the row's `status:` line.

### B1 [BLOCKER] — fixed by option (b), with (a)'s fence text inside it
- **What changed in `RCP-play-unit-card-from-hand.md`.**
  - **Primary shape = ONE batch, hand read INSIDE it**, documenting (not selecting) the fixed slot:
    - Steps 1–5 and the Step 4 table (rows 1–5, then the source's arms).
    - The cost exposure is removed by an in-batch `wait_pie_seconds` `[M: 1391 §4]`.
    - The TYPE exposure is declared in the row's spec (new Precondition 8) `[L: VER-§8 cl. 10(b), 2026-09-22]`.
    - Every value traces to a report sentence or a quoted law sentence.
  - **Attempt A's two-call shape** is kept as a bold-labelled *Alternative* at the end of `## Steps`. It is marked "does NOT fit the `t≈60 s` fence, and this recipe does not license it inside it", with its ≈70.11 s cost `[M: 1391 §4]`. It is not a ninth heading, so the 8-heading format still holds.
  - **The untagged `t≈80 s` line is gone.** Precondition 3 now quotes the law by text: "EVERY OBSERVABLE LANDS BEFORE `t≈60 s`" and the round-trip bound "BETWEEN `≈30 s` AND `≈70 s` … BUDGET AT THE UPPER END … A PLAN THAT SPENDS EVEN ONE ROUND TRIP INSIDE THE `t≈60 s` FENCE DOES NOT FIT", both `[L: VER-§8 cl. 10(b)]`. Then 1391 §4's own numbers against it.
  - It also says that the play batch must be the first call after `start_pie`: lifecycle is not in the runner `[S]`, and B's first stamp was t=15.1659 `[M: 1391 C2, Not-examined]`.
- **Why (b), not (a) alone.** (a) would have fixed the number but left the primary shape (a standalone hand read) spending the round trip the new fence line forbids. The recipe would have contradicted itself.
- **The source's ambiguity is kept as a statement about the source** (hazard "Attempt B's timeline …"). The law's reading is cited as the law's (`[L]`), and the primary shape follows it.

### W1 [WARN] — the envelope is now stated honestly, marked `NOT MEASURED`
- **From the source:** archer50 names the tool and gesture. "`ui_perform`'s `double_click` step FIRES `UButton.OnClicked` … This is how all 100 deck edits were made." (*THE ANSWER FIRST*) Its calls carried the root `WBP_DeckBuilder` (§1(b) error text). It does **not** name the envelope: a standalone `ui_perform` with a `steps` array, or `ui_perform` inside `run_verification_sequence`. It also does not say calls per run.
- **Step 4 now says, flagged ⚠️, that it is NOT the tool `VER-§13` cl. 1 names**, and gives three reasons, none of them a measurement:
  1. It is the source's named tool, with its root.
  2. Its schema runs the whole scenario in one call `[S]`, so there is no per-gesture call.
  3. The runner accepts `ui_perform` (`VER-§7` cl. 2 (i)), but whether it accepts the root `widget` inside a step is unquoted, and this step needs the root.
- **Both envelopes are marked `NOT MEASURED`.** If the manager's ruling on M7 picks the runner, only the envelope changes.
- The Source bullet ("Envelope discrepancy, unresolved") is rewritten the same way. It cites QA's M4 arithmetic as the inference QA labelled it. This supersedes finding 2 above in part: the tool is from the source, the envelope is still open.

### M2 (optional) — SEEDED. The entry step is no longer a gap; the COMPOSITION is.
Step 3 now carries `{"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_Card1.IA_Card1"}}`. The dispatch named `TASK-1314`/`TASK-1348`. I read both, and followed their thread into two more reports that quote the call more completely:

| what | source, quoted |
|---|---|
| call → return (effect) | `qa/TASK-1314-verify.md` **LIMB A**, ceilings item 2: "Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*." (card name incidental: cl. 7's 2026-09-20 amendment struck it as an example for that reason) |
| entry's state change | `qa/TASK-1348-verify.md` P4: "Entered: `GhostActor` `"None"` → `StaticMeshActor_34`; log: *"placement mode entered for card 'WatchTower' (cost 30)"*"; P10: "`IA_Card3`/`IA_Card6` entered placement", "`IA_Card1` played an instant spell" |
| exact `action_path` + reply | `qa/TASK-1230-verify.md` row 1: "`inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` → `status: action_injected`, `value_type: Boolean`". That reply came back on a press the game **refused** ("hand slot 0 ('Fog') refused — cost 5000, gold 88."), so the recipe marks it not-the-observable |
| key ↔ hand index | same `1230` row: `IA_Card1` acted on `Fog`, the only `Fog` in the hand read just before, i.e. index 0. **Only `IA_Card1` is measured**; `IA_Card2`…`6` ↔ index 1…5 is `[D]` (`SiegePlayerController.h` comment) |
| inside a `run_verification_sequence` | `qa/TASK-1270-verify.md` row 3: "`IA_Card1` fired at t=01m07.23s, logged at `:3097 … placement mode entered for card 'Footman' (cost 9).`" It was in a sequence **by derivation** from its metrics line (all its calls into PIE were 3 sequences + start/stop). Sandbox (No Bot), and followed by `IA_DiscardAll`/`IA_CancelPlace`, not a confirm |

**cl. 7's 2026-09-20 amendment is carried:** a slot index identifies nothing (`1348` two-gaps item 2 + `[L]`). The card is named from the in-batch hand read and from the entry log line (Step 5), never from `N`.

**What stays `NOT MEASURED`:** the entry composed in one batch with `1391`'s control + aim + confirm (`1391` never names its entry; `1270` never confirmed). Also its position in the batch (mine; it agrees with `1391` C4's census +1 being the ghost, which is a reading, not a stated position), and the spacing before the control press.

**For `TASK-1512` (put this in its dispatch):**
1. deck4 = 50× `Archer` removes the type exposure, since every slot holds the same Unit card.
2. Use `IA_Card1`, the only key with a measured mapping.
3. Gold must cover Archer's 12 **at the card-key press**, or the key is refused before placement `[M: 1230]`.
4. Check the Read-back's new **entry row** (`GhostActor` non-`None` + one entry log line) before reading either arm.
5. Record the composed batch, with the key used, under `## Recipe candidates`.

### NITs
- **N1**: README Pending row 1 no longer carries "both HELD". It now says their status is on the board.
- **N2**: the `survey_pie_scene` census suggestion moved from Step 4 guidance into Fences, as "a candidate, not a step".
- **N3**: `discover=true` invalidity re-tagged `[L: VER-§8 cl. 10(c)]`, with the validator text quoted. `1391` is cited only for not using it.
- **N4**: added one-line notes in Steps 3 and 4 of the deck-builder recipe: substitute the `"<from Step 1>"` placeholder strings with numbers before sending.

### Files changed this loop
| path | lines now | change |
|---|---|---|
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` | 235 | rewritten: B1 (b), M2 Step 3 seed, N2, N3; header, Source, Preconditions 3/4/5/8, Steps 1–5 + Alternative, Read-back entry row, Fences, hazards |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | 164 | W1 (Source bullet + Step 4 envelope bullet), N4 (two notes) |
| `Tools/Verify/recipes/README.md` | 79 | run short names + seeding line; recipe row 3; Pending rows 1 (N1) and 3 (now "the composed batch"); common hazard on batch branching |
| `.claude/pipeline/handoffs/TASK-1502-programmer.md` | — | this appended section only |
| `.claude/pipeline/TASKBOARD.md` | 1 line | this row's `status:` only |

`RCP-menu-to-deckbuilder.md` is untouched. Checks: all three recipes have exactly the 8 `##` headings in order; every ```json block parses (`json.loads`: play 5/5, deck-builder 7/7, menu 4/4); no `t≈80`, "both HELD" or old gap wording remains (grep).

### What QA should scrutinise this loop
- **The four new Step 3 sources.** `1230` and `1270` are beyond the dispatch's named `1314`/`1348`, and beyond the row's original READS. `VER-§13` cl. 3 permits any measured run. Every value from them is tagged, so check the tags.
- **`1270`'s in-sequence claim is a derivation from its call list, not a sentence it wrote.** The recipe says so on its face.
- **The `IA_Card1` ↔ index 0 claim rests on the card name `Fog` appearing once in the hand.** It does not rest on `1230`'s H2.
- **The Step 4 table's rows 1–5 are my composition around measured parts.** The recipe labels the composition `NOT MEASURED`.
- **Precondition 8's "a deck of one Unit card has no type exposure"** is logic, not a measurement.

### Supersessions in the loop-0 text above (not edited, per "append")
- Finding 1 (entry gap): superseded by M2 above.
- Finding 5's last sentence ("prescribes the hand read in a call before the play batch"): superseded by B1.
- The QA map's "Step 3 entry ← none" and "Step 1 hand read" lines: superseded. The hand read is now Step 2, inside the batch.
- *Not examined*'s "I did **not** read `qa/TASK-1352`/`1357`/`1348`/`1390`/`1230` reports": `1348` and `1230` (and `1314`, `1270`) were read this loop. `1352`/`1357`/`1390` still were not.

### Not examined / limitations (this loop)
- No recipe was executed, and no PIE was run. The composed batch is unrun.
- Tool schemas (`inject_input_action`, `run_verification_sequence`, `ui_perform`) were loaded with `ToolSearch` to check spellings and the runner's step enum. **No tool was called.** Descriptions arrived truncated.
- `[D]` facts were read from the working tree on 2026-09-26: the `IA_Card*` soft paths, the six `.uasset`s, `EnterPlacementMode`'s log line and `SpawnPlacementGhost()` call, and the `SiegePlayerController.h` slot comments. They can drift with edits. `TASK-1507` is editing `DeckBuilderWidget.*` concurrently; I touched no source.
- None of the four entry sources recorded an Aura plugin version (grep).
- No `.claude/agents/*`, `CLAUDE.md`, `CONVENTIONS.md`, `qa/*` report, source file, asset, editor or git action. A Bash heredoc append of this section was cut off mid-line at the files table; it was completed with an exact-string edit, and the section was read back whole.
