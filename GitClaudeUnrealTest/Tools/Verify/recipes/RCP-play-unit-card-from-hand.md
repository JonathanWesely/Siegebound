# RCP-play-unit-card-from-hand — play an `ECardType::Unit` card from hand on `L_Arena`

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1391` = `.claude/pipeline/qa/TASK-1391-verify.md` (the play) · `1230` = `qa/TASK-1230-verify.md` · `1270` = `qa/TASK-1270-verify.md` · `1314A` = `qa/TASK-1314-verify.md` **LIMB A** · `1348` = `qa/TASK-1348-verify.md` (the last four: the placement-entry step only).

> ⚠️ **Read this first.**
> - **The primary shape fits the `t≈60 s` fence:** ONE batch, with the hand read INSIDE it. The read **documents** what the fixed slot held; it does **not select** it. So the run must tolerate whatever card that slot turns up (Precondition 8).
> - Attempt A's two-call shape (hand read, then a separate play batch) is kept as a labelled **alternative that does NOT fit the fence** (end of Steps).
> - **The entry step (Step 3) is not from `1391`**, which never names it. It is seeded from four runs that measured `inject_input_action IA_Card<N>` entering placement. **Nobody has run the entry, the control, the aim and the confirm together in one batch**, so the first use of this recipe measures that composition.

## Source

- `.claude/pipeline/qa/TASK-1391-verify.md` (probe `[PLACEMENT-COMPOSE-PROBE]`), `Verdict: MEASURED`, run **2026-09-22** (log stamps `2026.09.22`), 2 attempts (session A t=2.97 → 144.73 s, session B t=15.17 → 51.09 s).
  - **C2** hand read + card choice · **C3** one batch, with indices · **C4** the omitted-set control · **C5** the three observables · **C6** the refusal line · **§3** the two arms side by side · **§4** the budget miss and the round-trip cost · *Hypotheses* H1/H2 · *Not examined / limitations* · *Fences*.
- **Step 3 only (placement entry).** Seeded on `TASK-1505` M2. `VER-§13` cl. 3 allows any measured run.
  - `1314A`, *CEILINGS MEASURED THIS RUN* item 2, run 2026-09-19: "Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*."
  - `1348` P4, P10 and the *two gaps* list, evidence dated 2026-09-20: the entry's state change, which card keys entered placement, and the shuffled hand.
  - `1230` row 1, log stamps 2026.09.14: the exact `action_path`, the reply, and the only measured key-to-hand-index correspondence.
  - `1270` row 3 + the metrics bullet, log stamps 2026.09.15: `IA_Card1` injected **inside** a `run_verification_sequence`, with the entry logged.
- The law:
  - `VER-§8` cl. 7, amendment of 2026-09-22 — the only sentence a row may cite as entitled: "Inside ONE batched `run_verification_sequence`, on `L_Arena`, an `ECardType::Unit` card IDENTIFIED FROM A HAND READ TAKEN FIRST was PLAYED FROM HAND …".
  - `VER-§8` cl. 7, amendment of 2026-09-20: a slot index identifies nothing.
  - `VER-§8` cl. 10(b), with its 2026-09-20 and 2026-09-22 corrections: the fence, the round-trip bound, and the law's reading of attempt B.
- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` B1, M2, N2, N3). Not re-verified since.

## Plugin version

**Not read by any source run.** Neither `1391` nor any of the four Step 3 sources records one. Context only, not this recipe's measurement: `VER-§5` cl. 5 records `Aura.uplugin` `1.0.6` read by `TASK-1390` on 2026-09-22. ⇒ the first use re-verifies in-run (`VER-§13` cl. 3).

## Preconditions

1. Level `/Game/Maps/L_Arena`. The source loaded it: `load_level` → `{level: /Game/Maps/L_Arena, previous_level: /Game/Maps/L_MainMenu, already_open: false, loaded: true, discarded_unsaved: false}`, with `discard_unsaved` **not** passed. `[M: 1391 Editor/Aura state]`
2. PIE standalone, 1 client, 1280×720 requested → viewport **1280×725**, `client_index 0`; captures came back 1086×615. `[M: 1391 Editor/Aura state, Not-examined]`
3. **The budget: every observable lands before `t≈60 s`. The play batch is the first call after `start_pie`, and no other round trip is spent inside the fence.** `start_pie` itself cannot go in the batch: "PIE LIFECYCLE IS NOT IN THIS TOOL" (`run_verification_sequence`'s schema) `[S]`.
   - The law, quoted by text (emphasis marks dropped): "EVERY OBSERVABLE LANDS BEFORE `t≈60 s`". The reason: the undriven hero was measured dying "BETWEEN `t=82.6 s` AND `t=135.6 s`", and "THE ONLY SAFE READING OF THE MEASUREMENT IS ITS LOWER BOUND". `[L: VER-§8 cl. 10(b), 2026-09-20 correction]`
   - The planning rule: "a round trip costs BETWEEN `≈30 s` AND `≈70 s` OF PIE CLOCK, EACH END OBSERVED ONCE ⇒ BUDGET AT THE UPPER END. ⇒ A PLAN THAT SPENDS EVEN ONE ROUND TRIP INSIDE THE `t≈60 s` FENCE DOES NOT FIT, AND *"it fit last time"* IS LUCK, NOT A BUDGET." `[L: VER-§8 cl. 10(b), 2026-09-22 correction]`
   - What the source measured against it `[M: 1391 §4]`:
     - Attempt A's one round trip cost **≈70.11 s** (batch 1 ended t=12.1365, batch 2 began t=82.2466). Its observables landed at t=82.2 → 83.1 s, "past the row's `t≈60 s` fence and at/just past the `t=82.6 s` earliest-observed bot kill". The hero was still alive (`pawn_class: BP_HeroCharacter_C` at t=83.084 s), which the report calls "margin I did not have, not margin I planned".
     - Attempt B's one batch finished by t=32.07 s, "half the `t≈60 s` fence".
   - Attempt B confirmed that the session had started from the sequence's own `pie_time_seconds` stamps, not from a separate `is_pie_active` call. Its first in-batch stamp was t=15.1659 s, which is what reaching the batch cost that run. `[M: 1391 Not-examined, C2]`
4. **Gold: the batch makes the card affordable itself.**
   - Passive income measured at **+1/s**: 22 @ t=12.10 → 92 @ t=82.26, and 25 @ t=15.18 → 41 @ t=31.23. `[M: 1391 H2]` Starting gold is `int32 Gold = 10;` in `SiegePlayerState.h`. `[D]`
   - Attempt B raised gold 25 → 41 with an in-batch `wait_pie_seconds(16)` ("costing no round trip") "so any card in the hand would be affordable". `[M: 1391 §4]`
   - ⚠️ **Gold must cover the cost at the card-key press, not only at the confirm.** An unaffordable card key is refused before placement: `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` at gold 88 logged `hand slot 0 ('Fog') refused — cost 5000, gold 88.`, and gold and hand did not change ("nothing spent"; hand "unchanged"). `[M: 1230 row 1]`
5. Property paths are decided **from source code before PIE**: `PlayerState.Gold`, `DeckComponent.Hand`, `GhostActor…`. The source did this and needed no discovery call. `[M: 1391 Not-examined]`
   - `discover=true` is **not** a valid param inside a sequence. The validator's words: "unknown param 'discover'. Valid params: `['client_index', 'component', 'name', 'properties']`". `[L: VER-§8 cl. 10(c)]` (measured on `TASK-1352`, not on `1391`, which only did not use it).
   - `[D]` (author's source read, not the report's): `DeckComponent` and `GhostActor` are members of `ASiegePlayerController` (`TObjectPtr<UDeckComponent> DeckComponent;` and `TObjectPtr<AStaticMeshActor> GhostActor;` in `SiegePlayerController.h`). `Gold` is on `ASiegePlayerState`.
   - ⚠️ `ASiegeBotController` also constructs a `DeckComponent` (`DeckComponent = CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"));` in `SiegeBotController.cpp`). Address the player's controller by its exact label.
6. The player controller's PIE label is **`SiegePlayerController0`**: it is the `name` the source passed to `call_actor_function`, and the reply echoed it. `[M: 1391 §1, Not-examined]`
7. A pre-run `.sav` baseline, taken by the orchestrator. The source had none and could only infer its net-zero save claim. `[M: 1391 Not-examined, Fences]`
8. **The TYPE exposure is declared before dispatch, in the row's spec.** The law, quoted by text (emphasis marks dropped), `[L: VER-§8 cl. 10(b), 2026-09-22 sub-bullet]`: "the recipe must therefore TOLERATE whatever that slot turns up, in BOTH dimensions: COST and TYPE" and "a row that needs an `ECardType::Unit` card SAYS IN ITS SPEC what it does if the slot holds a spell — and reports what it got either way."
   - Cost is handled by Precondition 4.
   - What other types did when measured:
     - an instant spell is **played by the card key itself**: "`IA_Card1` played an instant spell". `[M: 1348 P10]`
     - a Building enters placement ("placement mode entered for card 'WatchTower' (cost 30)"), but its confirm is outside this recipe's fences. `[M: 1348 P4]`
     - `MilitiaMob` is a swarm (`swarmCount: 4`), which the source never exercised. `[M: 1391 Not-examined]`
   - A hand dealt from a deck of one Unit card has no type exposure: every slot holds that card.

## Steps

**Step 1 — before PIE (no call): fix the card key, write down the tolerance, size the gold wait.**
- **Pick `N` for `IA_Card<N>`.** The Step 2 read documents what the slot held, so any `N` works. How keys map to `Hand` indices:
  - `IA_Card1` ↔ `Hand` index 0 is **measured**. `IA_Card1` acted on `Fog`, the only `Fog` in a hand read as `("Fog","Footman","Archer","Wall","Wall","WatchTower")` just before the press, and the log reads `hand slot 0 ('Fog') refused …`. `[M: 1230 row 1]` This rests on the card name. `1230` files its explanation of the log's `slot 0` under its H2.
  - `IA_Card<N>` ↔ index `N−1` for `N` = 2..6 is `[D]` only: "IA_Card2..IA_Card6 pressed (keys "2".."6"): play hand slot 1..5 (bound with the slot as payload)" in `SiegePlayerController.h`. **Not measured at runtime.** `N = 1` is the only key whose mapping is measured.
- **Write the tolerance into the row's spec:** what the run does if the Step 2 read shows a non-Unit or swarm card in that slot (Precondition 8).
- **Size the gold wait** so that gold at the card-key press is at least the dearest card the slot could hold (Precondition 4). Attempt B: 16 s took 25 → 41. `[M: 1391 §4]`
- **Check the budget:** first in-batch stamp (B: t=15.17) + the wait + the batch span (A 0.854 s, B 0.874 s) must land before t≈60 s. `[M: 1391 C2, C3, §4]`; `[L: VER-§8 cl. 10(b)]`

**Step 2 — the hand read is the FIRST action INSIDE the play batch. It documents; it does not select.**
```json
{"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "component": "DeckComponent", "properties": ["Hand"]}}
```
- Source: "`get_actor_property_in_pie(component="DeckComponent", properties=["Hand"])`, taken as the **first** action of the batch, before any input of any kind". `[M: 1391 C2]`
  - The `component` and `properties` values are quoted.
  - `name` is per Precondition 6.
  - The four param names are corroborated by the validator text in Precondition 5. `[L: VER-§8 cl. 10(c)]`
  - The `{"type","params"}` envelope is `[S]`.
- Read gold beside it: `{"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["PlayerState.Gold"]}}`. The path is quoted `[M: 1391 Not-examined]`; the call spelling is `[S]`.
- **Re-read the hand after the gold wait.** Attempt B read it at t=15.1659 and again at t=31.2163, "both before any press, byte-identical". `[M: 1391 C2]` The second read is the arms' "before" value.
- The law's reading of attempt B, which this shape follows: "the hand read goes INSIDE the one batch and DOCUMENTS what the fixed slot HELD — it does NOT SELECT it." `[L: VER-§8 cl. 10(b), 2026-09-22 sub-bullet]`
- **After the batch, name the card from the read, never from `N`, and say why it is (or is not) a clean observable** (cl. 7's 2026-09-20 amendment: a slot index identifies nothing). `[L: VER-§8 cl. 7]` The source's criteria for a clean observable `[M: 1391 C2]`:
  - `cardType: Unit`: the plain-unit spawn path, with no building-clearance gate and no `CanScaleFootprint()` override;
  - `swarmCount: 0`: exactly one actor spawns ⇒ a clean +1 census;
  - affordable at the gold read (A: `Footman`, cost 9, at gold 22; B: `Archer`, cost 12, at gold 41).
- **Slot numbering, read off the source's own quoted arrays (not stated in words):** the report's "slot 1" is **0-based index 1** of the `Hand` array. `[M: 1391 C2, C5]`
  - A's hand `("Longbowman","Footman","Footman",…)` has `Footman` at index 1, and after the play index 1 read `"MilitiaMob"`.
  - B's `("Pikeman","Archer",…)` has `Archer` at index 1, and index 1 read `"MilitiaMob"` after.
  - `1391` does not say which key entered that slot.

**Step 3 — enter placement mode: `inject_input_action IA_Card<N>`, inside the same batch, after the pre-reads.**
```json
{"type": "inject_input_action", "params": {"action_path": "/Game/Input/Actions/IA_Card1.IA_Card1"}}
```
- **The call and its reply, measured:** `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` → `status: action_injected`, `value_type: Boolean`. `[M: 1230 row 1]` ⚠️ **The reply is not the observable.** It came back the same on a press the game **refused** (the `Fog` row, Precondition 4), so it proves delivery, not entry.
- **Path and spelling:**
  - The path for `N = 1` is quoted `[M: 1230 row 1]`.
  - For `N` = 2..6 it is `[D]`: the soft paths `/Game/Input/Actions/IA_Card<N>.IA_Card<N>` in `SiegePlayerController.cpp`, with `IA_Card1`…`IA_Card6` `.uasset`s present under `Content/Input/Actions/`.
  - `action_path` as the in-sequence param name, and the envelope, are `[S]` (the schema's "`inject_input_action`: soft path of the InputAction (required)").
- **Entry, measured on standalone calls:**
  - "Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*." `[M: 1314A ceiling 2]` The card name there is incidental. Cl. 7's 2026-09-20 amendment struck that quote as a worked example for exactly this reason. `[L: VER-§8 cl. 7]`
  - "Entered: `GhostActor` `"None"` → `StaticMeshActor_34`; log: *"placement mode entered for card 'WatchTower' (cost 30)"*". `[M: 1348 P4]`
  - "`IA_Card3`/`IA_Card6` entered placement"; "`IA_Card1` played an instant spell". `[M: 1348 P10]`
- **Entry, measured inside a `run_verification_sequence`:** in that session, "`IA_Card1` fired at t=01m07.23s, logged at `:3097 … placement mode entered for card 'Footman' (cost 9).`" `[M: 1270 row 3]` It was a sequence step: `1270`'s full list of its tool calls into PIE has no standalone `inject_input_action` ("Tool calls into PIE: 3 `run_verification_sequence` (4 + 37 + 12 actions), 1 `start_pie`, 1 `stop_pie`, 1 `stop_pie_recording`"). `[M: 1270 metrics]` The report does not say this in words; it follows from that list.
  - That run was **Sandbox (No Bot)** on `L_Arena`.
  - Its entry was followed by `IA_DiscardAll` (t=01m07.57s) and `IA_CancelPlace`, **not** by a confirm.
- **`NOT MEASURED`: this entry composed in one batch with `1391`'s control arm, aim and confirm.** `1391` does not name its entry, and `1270` never confirmed.
  - **Position.** The position (after the pre-reads, before the control press) is this recipe's, not a source's. It agrees with `1391` C4's control-arm census `167 → 168`, "the +1 being the ghost `StaticMeshActor_34` itself". The pre-read's time is not quoted, so this is a reading, not a measurement.
  - `[D]`: `EnterPlacementMode` calls `SpawnPlacementGhost()` and then logs `"… placement mode entered for card '%s' (cost %d)."` (`SiegePlayerController.cpp`).
  - **Spacing.** The spacing between the entry and the control press is `NOT MEASURED`. The entry read (Step 4) sits between them.
- **A slot index identifies nothing.** The hand is shuffled and redraws after every play: "Across three sessions slot 1 held `BrightSun`, then `Footman`, then `WatchTower`." `[M: 1348 two-gaps item 2]`; `[L: VER-§8 cl. 7, 2026-09-20]` The card is named from the Step 2 read and the Step 5 entry line, never from `N`.

**Step 4 — ONE `run_verification_sequence`: hand read, entry, control arm, then treatment arm, with every dependent read inside it.** `[M: 1391 C3, C4, §3]` for the arms. Rows 1–5 are this recipe's composition (Steps 2–3).

| order | action | source |
|---|---|---|
| 1 | hand read + gold read (Step 2) | `[M: 1391 C2]` |
| 2 (as sized) | `wait_pie_seconds` (B: 16) | `[M: 1391 §4]` |
| 3 | pre-reads: `Hand` again, gold, `GhostActor` (expect `None`), census | `[M: 1391 C2, C4, C5]`; `GhostActor` `None` before entry `[M: 1348 P4]` |
| 4 | **ENTRY**: `inject_input_action IA_Card<N>` (Step 3) | `[M: 1230, 1270, 1314A, 1348]`; the composition is `NOT MEASURED` |
| 5 | entry read: `GhostActor` (expect non-`None`) | `[M: 1348 P4]`; `1391` C4 "`GhostActor` still `StaticMeshActor_34`" at the control |
| control | `simulate_key_press` `LeftMouseButton`, **no set step before it** | `[M: 1391 C4, §3]` |
| control reads | gold, `GhostActor` (+ `bHidden`, location), `Hand`, census | `[M: 1391 C4]` |
| treatment: SET | `pie_scene_edit` → `call_actor_function SetMouseLocation(300,420)` (source indices: A 11, B 15) | `[M: 1391 C3, Not-examined]` |
| dependent read | ghost read (source indices: A 13, B 17) | `[M: 1391 C3]` |
| CONFIRM | `simulate_key_press` `LeftMouseButton` (source indices: A 14, B 18) | `[M: 1391 C3]` |
| post-reads | gold, `Hand`, census (source indices: A 16/17/18, B 20/21/22) | `[M: 1391 C3, C5]` |

The indices are the source's own and will shift with rows 1–5. Measured span of the source batch: A 21 actions, t=82.2466 → 83.1009 s (**0.854 s**); B 25 actions, t=31.1996 → 32.0731 s (**0.874 s**). "No MCP round trip separates any set from its dependent read." `[M: 1391 C3]`

The action objects, copy-paste ready:

- SET: the operation object is **verbatim** from the source; the `pie_scene_edit`/`operations` envelope is `[S]`:
```json
{"type": "pie_scene_edit", "params": {"operations": [{"op": "call_actor_function", "name": "SiegePlayerController0", "function": "SetMouseLocation", "args": {"X": 300, "Y": 420}}]}}
```
  The source's reply for it: `{"op":"call_actor_function","actor":"SiegePlayerController0","function":"SetMouseLocation","ok":true}`, `applied: 1, failed: 0` (A t=82.757 s, B t=31.716 s). `[M: 1391 §1, C1]` The arg names `name`/`function`/`args` are also in `[L: VER-§8 cl. 10(c)]`.
- CONFIRM and CONTROL press (source: `simulate_key_press "LeftMouseButton"`; the `key` field name is `[S]`):
```json
{"type": "simulate_key_press", "params": {"key": "LeftMouseButton"}}
```
- Hand read: Step 2's object.
- Entry: Step 3's object.
- Gold and ghost (the paths `PlayerState.Gold` and `GhostActor` are quoted by the source; the call spelling is `[S]`):
```json
{"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["PlayerState.Gold", "GhostActor"]}}
```
  The ghost's `bHidden` and location were read (C4/§3 quote them), but the source quotes the path only as "`GhostActor…`" and names `RelativeLocation` in H1. The exact read paths are `NOT MEASURED`.
- Census: the source reports totals and per-class counts ("census `168 → 169`", "`BP_Unit_Footman_C` count 1", "`Unit_` classes still only `BP_Unit_Pikeman_C ×1`") but does not name the instrument. `NOT MEASURED`: which tool (see Fences).
- Gold wait: `{"type": "wait_pie_seconds", "params": {"seconds": 16}}` (`[M: 1391 §4]` for the 16; spelling `[S]`; the schema caps one wait at 30 s).

**Step 5 — after the batch, read the log: two lines.** Category `LogGitClaudeUnrealTest`, in `Saved/Logs/GitClaudeUnrealTest.log`.
- **The entry line**, `placement mode entered for card '<CardID>' (cost <n>)`: one per entry, naming the card the key actually entered. `[M: 1314A ceiling 2, 1348 P4, 1270 row 3]`; `[D]` one `UE_LOG` per `EnterPlacementMode`.
  - If it is absent, or names anything but a clean Unit card (Step 2 criteria), the arms measured nothing about a Unit placement.
  - Report what you got (Precondition 8).
- **The refusal line**, substring `placement click refused`. The source found `total_matches = 2`: exactly one per control arm, none for either treatment arm. `[M: 1391 C6]`
- **Tool:** `1391` does not name its log tool. `1230` quoted its refusal line "verbatim from `get_unreal_output_logs`". `[M: 1230 row 1]`

**Alternative — attempt A's two-call shape. It does NOT fit the `t≈60 s` fence, and this recipe does not license it inside it.**
- **What it is.** A standalone hand read first, then the card chosen by that read, then a second batch carrying Steps 3–4 with `N` set from the chosen index. A's hand read was "taken as the **first** action of the batch" at t=12.0865 s, in batch 1, which ended t=12.1365 s. `[M: 1391 C2, §4]`
  - Standalone form: `{"name": "SiegePlayerController0", "component": "DeckComponent", "properties": ["Hand"]}` on `get_actor_property_in_pie`.
  - A's choice: `Footman`, cost 9, "the cheapest card in that hand" at gold 22, with "Slots 1 AND 2 both held `Footman`, so the choice was robust to an off-by-one in the slot mapping". `[M: 1391 C2]`
- **It is the only shape that SELECTS a card** from a mixed hand.
- **Its cost, measured:** the round trip between the two calls cost ≈70.11 s and put the observables at t=82.2–83.1 s. `[M: 1391 §4]` Under the law's bound that "DOES NOT FIT" (Precondition 3). `[L: VER-§8 cl. 10(b)]`
- The key for any index other than 0 is `[D]` (Step 1).
- A row that needs selection is the manager's to spec, with the ceiling declared at boarding (`VER-§8` cl. 2).

## Read-back

The three observables, named in advance, **none of them the absence of a refusal line** (`[M: 1391 C5]`; law `VER-§8` cl. 7 boarding rule (iv)), plus the entry check that decides whether the arms mean anything:

| observable | control arm (set omitted): must NOT move | treatment arm: must move |
|---|---|---|
| **entry (checked first)** | `GhostActor` `None` → non-`None` after row 4 (`1348` P4: `None` → `StaticMeshActor_34`), and one entry log line naming a clean Unit card (Step 5). **If either fails, neither arm is evidence.** | (same entry) |
| gold | A `92 → 92`, B `41 → 41` | A `92 → 84`; B **`41 → 29` = exactly `DT_Cards.Archer.cost` 12** |
| hand (index 1 in the source) | unchanged, both | A `Footman → MilitiaMob`; B `Archer → MilitiaMob` |
| new unit actor | none, both (the census +1 is the ghost `StaticMeshActor_34` itself) | A `BP_Unit_Footman0` (class `BP_Unit_Footman_C`); B `BP_Unit_Archer0` (class `BP_Unit_Archer_C`) |
| `GhostActor` after | still `StaticMeshActor_34` (placement mode alive) | **`None`** ⇒ `ExitPlacementMode` ran |
| ghost before the press | `bHidden: true` at `(0,0,0)` | `bHidden: false` at the aim point (see coordinates) |
| refusal log line | one per arm, quoted (the positive control that the button arrived) | none |

All rows except "entry" are `[M: 1391 C4, C5, C6, §3]`. The entry row is `[M: 1348 P4, 1314A, 1270]` plus this recipe's composition. Pixels corroborate only: both treatment frames show an opaque placed unit, not a translucent ghost. `[M: 1391 Evidence]`

## Fences — not measured for

- **The entry composed in ONE batch with the control, aim and confirm.** Each half is measured separately (Step 3 and `1391`). The whole sequence has never run. The first use measures it and reports it as a recipe candidate if it holds.
- **Card key ↔ hand index for `IA_Card2`…`IA_Card6`**: `[D]` only. Only `IA_Card1` ↔ index 0 is measured (`1230`).
- **`ECardType::Unit` cards only** (`Footman`, `Archer`). "A row that reads this as *'any card can now be played'* has over-read it." `[M: 1391 Not-examined]` Untouched:
  - the Building path (`bPendingIsBuilding`);
  - the upgrade path (`PlacementUpgradeState::Ready → ConfirmStackUpgrade`);
  - the swarm path (`MilitiaMob`, `swarmCount: 4`);
  - the miner-cap path;
  - the spell path (`TryConfirmSpellTarget`, not a `UFUNCTION`). An instant spell is played by the card key itself (`1348` P10), so the in-batch shape can spend gold and a card before the control arm if the slot holds one.
- **One of `TraceCursorToGround`'s three callers** (the placement ghost). The spell reticle and the group-pick reticle were not probed. `[M: 1391 Not-examined]`
- **No accuracy claim.** `(300,420)` is "not proven to be any particular viewport location … Reproducible ≠ calibrated." `[M: 1391 Not-examined]`
- **One aim point, at the Blue spawn, from the hero's starting camera.** Placing anywhere else, **including inside a capture zone**, is `NOT MEASURED`.
  - The refusal text names the legal regions: "outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh". `[M: 1391 C6]`
  - So a zone placement would at least need the zone Blue-owned. Whether one then confirms was never tested.
- **No persistence across a round trip.** Both aims were set and consumed inside one batch; "nothing here licenses a two-call *'set the cursor, then read the ghost'*". `[M: 1391 Not-examined]`
- **The census instrument.** `NOT MEASURED` (the source names none). `survey_pie_scene` with `include: ["census"]` returns per-class counts by its schema `[S]`, but no source run is quoted using it for this census. It is a candidate, not a step.
- **`inject_input_action IA_Attack` as a confirm** is wall (a) and stands unrelaxed: an Enhanced Input action sets the action, not the key state the confirm polls. This recipe confirms with `simulate_key_press` only; its `inject_input_action` is the **entry** (`IA_Card<N>`), never the confirm. `[M: 1391 §2]`; law `VER-§8` cl. 7.
- **Scope of the runs:** `L_Arena` only; standalone single client; `client_index 0`. The one-batch shape (B, as the law reads it) and the two-call alternative (A) only. `1270`'s in-sequence entry was Sandbox (No Bot).
- Attempt A's aim point differs from the canonical one: `HYPOTHESIS` H1, "MECHANISM NOT MEASURED". `[M: 1391 H1]`
- Plugin version: not read.

## Coordinate/resolution-dependent values

| value | measured at | notes |
|---|---|---|
| `SetMouseLocation(300, 420)` (viewport coordinates passed to `APlayerController::SetMouseLocation`) | viewport 1280×725 (1280×720 requested), `client_index 0`, capture 1086×615; **DPI not read** | reproducible, **not calibrated** `[M: 1391 Not-examined]` |
| aim point B `(X=-21382.831129, Y=407.684206, Z=29.000000)` | same, hero at the start camera ("a byte-identical camera") | "BIT-IDENTICAL" to `TASK-1357`'s and `TASK-1352`'s values: three runs, six decimals `[M: 1391 §3]` |
| aim point A `(X=-21440.000107, Y=437.757051, Z=32.938384)` | same screen point, same camera | unexplained (`HYPOTHESIS` H1) `[M: 1391 H1]` |
| spawned unit A `(-21394.41, 405.35, 121.15)`, B `(-21341.37, 377.43, 109.08)` | — | the pale stone ledge at mid-left in both frames `[M: 1391 C5, Evidence]` |

The world point under `(300,420)` depends on the camera, i.e. on where the hero stands and looks. Move the hero, change the viewport, or change the camera, and this recipe's aim is unmeasured. The entry step has no coordinate.

## Known hazards

- **A batch cannot branch on its own result:** "the card cannot be chosen until the hand is known". `[M: 1391 §4]`
  - In the primary shape the batch therefore runs to its end whatever the slot held, and whether the entry happened.
  - Judge the arms only **after** checking the entry row of the Read-back.
  - Attempt A avoided this by splitting into two calls and paid ≈70.11 s for it (Alternative).
- **Attempt B's timeline, as the source states it, admits more than one reading.** C2 has hand reads at t=15.1659 and t=31.2163. §4 says "one batch, everything inside it … with an in-batch `wait_pie_seconds(16)`". C3 says "one call, 25 actions, t=31.1996 → 32.0731 s". `[M: 1391 C2, C3, §4]`
  - Unambiguous: the hand was read before any press; the two B reads 16 s apart were byte-identical (no play ⇒ no redraw in that window); and the set, confirm and dependent reads shared one call.
  - The law reads B as one batch with the hand read inside it, documenting rather than selecting. `[L: VER-§8 cl. 10(b), 2026-09-22]` This recipe's primary shape follows the law's reading.
- **The hand redraws after every play:** index 1 was refilled by `MilitiaMob` both times. `[M: 1391 C5]` Re-read before any second play. A slot index identifies nothing. `[L: VER-§8 cl. 7]`
- **An unaffordable card key is refused before placement**, and gold and hand stay put. `[M: 1230 row 1]` In the primary shape that silently removes the entry, and the entry row of the Read-back catches it.
- **`status: action_injected` is not the observable.** It came back on a refused press. `[M: 1230 row 1]`
- **`binding_found` is actively misleading here:** every `simulate_key_press` returned `binding_found: true`, `bound_actions: ["IA_Attack"]` on the arm that placed and the arm that refused alike. It is not the observable. `[M: 1391 Not-examined]`; law `VER-§8` cl. 10.
- **Unreadable members:** `bPlacementValid`, `PendingCardID`, `PendingCost`, `PlacementLocation` have no `UPROPERTY` and cannot be read. Use `GhostActor`/`bHidden`/gold/hand/census and the entry log line. `[M: 1391 Not-examined]`
- **Gold reads can straddle an income tick.** A's `−8` against a cost of 9 is explained as one +1 tick inside a 0.771 s window, which the source labels "an INFERENCE, not a separate measurement" (H2). `[M: 1391 H2]` Always assert "gold went down". Assert "down by exactly the cost" only when the timestamps rule a tick out.
- **Label vs object name:** the working `name` is the label `SiegePlayerController0`; the log line prints the object name `'SiegePlayerController_0'`. `[M: 1391 §1, C6]`
- **`start_pie`'s reply overflowed** the tool-output limit on both sessions (264,336 and 504,242 characters). Session start was confirmed by `is_pie_active` (A) and by the sequence's own `pie_time_seconds` (B). `[M: 1391 Not-examined]`; law `VER-§12` cl. 6.
- **Grant declaration:** `pie_scene_edit` is not a standalone tool on the verifier's surface. It was reached **only as a step inside `run_verification_sequence`**, twice, one `call_actor_function` op each, no other op. `[M: 1391 §1, Not-examined]`
  - A run using this recipe declares the same, line by line, under *Not examined* (`VER-§7` cl. 2).
  - Whether it becomes a standing grant is Jonathan's and the orchestrator's call (`VER-§8` cl. 7).
- **Environment:** frame A carried a red "Video memory has been exhausted" banner; recorded, not diagnosed. `[M: 1391 Editor/Aura state]`
