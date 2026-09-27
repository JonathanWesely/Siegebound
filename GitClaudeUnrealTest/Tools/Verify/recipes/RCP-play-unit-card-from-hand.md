# RCP-play-unit-card-from-hand — play an `ECardType::Unit` card from hand on `L_Arena`

Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1391` = `.claude/pipeline/qa/TASK-1391-verify.md` (the play) · `1230` = `qa/TASK-1230-verify.md` · `1270` = `qa/TASK-1270-verify.md` · `1314A` = `qa/TASK-1314-verify.md` **LIMB A** · `1348` = `qa/TASK-1348-verify.md` (the last four: the placement-entry step only) · `1512` = `qa/TASK-1512-verify.md` (the control-arm finding, the pre-press ghost read, the no-play-interval control, and the composed batch measured once).

> ⚠️ **Read this first.**
> - **The primary shape fits the `t≈60 s` fence:** ONE batch, with the hand read INSIDE it. The read **documents** what the fixed slot held; it does **not select** it. So the run must tolerate whatever card that slot turns up (Precondition 8).
> - Attempt A's two-call shape (hand read, then a separate play batch) is kept as a labelled **alternative that does NOT fit the fence** (end of Steps).
> - **The entry step (Step 3) is not from `1391`**, which never names it. It is seeded from four runs: three measured entry (`1314A`, `1348`, `1270`); `1230` measured the call, path and reply on a refused press. **`1512` a2 ran the entry, a confirm and the post-reads in one batch, once, with NO aim set** (Step 3). **Never run:** the entry composed with an aim that placed (SET → dependent read → confirm inside a live placement mode), and the entry composed with an omitted-set press that was refused.
> - ⚠️ **The omitted-set press is a control ONLY when the ghost is hidden immediately before it.** In `1512` the unset cursor already traced to (−449.15, −1.71), a legal point inside a Blue-owned zone, and the press planned as the control placed `BP_Unit_Archer_C_0` `[M: 1512 A3]`. `1391` C4, this arm's source, had the ghost `bHidden: true` at `(0,0,0)`. So: read the ghost immediately before **every** press (Step 4, the pre-press ghost reads), count the omitted-set arm as a control only when that read shows `bHidden: true`, and carry the no-play-interval control (`1512` ctl) in every batch. A batch cannot branch, so the gate is judged after the batch, from its reads.

## Source

- `.claude/pipeline/qa/TASK-1391-verify.md` (probe `[PLACEMENT-COMPOSE-PROBE]`), `Verdict: MEASURED`, run **2026-09-22** (log stamps `2026.09.22`), 2 attempts (session A t=2.97 → 144.73 s, session B t=15.17 → 51.09 s).
  - **C2** hand read + card choice · **C3** one batch, with indices · **C4** the omitted-set control · **C5** the three observables · **C6** the refusal line · **§3** the two arms side by side · **§4** the budget miss and the round-trip cost · *Hypotheses* H1/H2 · *Not examined / limitations* · *Fences*.
- **Step 3 only (placement entry).** Seeded on `TASK-1505` M2. `VER-§13` cl. 3 allows any measured run.
  - `1314A`, *CEILINGS MEASURED THIS RUN* item 2, run 2026-09-19: "Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*."
  - `1348` P4, P10 and the *two gaps* list, evidence dated 2026-09-20: the entry's state change, which card keys entered placement, and the shuffled hand.
  - `1230` row 1, log stamps 2026.09.14: the exact `action_path`, the reply, and the only measured key-to-hand-index correspondence.
  - `1270` row 3 + the metrics bullet, log stamps 2026.09.15: `IA_Card1` injected **inside** a `run_verification_sequence`, with the entry logged.
- **The control-arm finding and the composed batch.** `.claude/pipeline/qa/TASK-1512-verify.md`, `Verdict: VERIFIED`, run **2026-09-26**, attempt 2 (`a2`) unless a line says attempt 1.
  - **A2** the zone box and `CaptureOwner` · **A3** entry → confirm → post-reads in one batch, and its declared deviation (the press planned as the control placed the unit) · **ctl** the no-play-interval control · *Attempt 1* (the dead-hero refusal) · *Hypotheses* H1 · *Not examined* · *Speed data* · *Recipes used* · *Recipe candidates* item 6 (the read paths, the census call, the `wait 0.2`).
  - It used this recipe **outside its aim fence** (a capture zone, not the Blue spawn) and with **no aim set**, and it reported the control failure rather than patching the recipe (`VER-§13` cl. 3).
- The law:
  - `VER-§8` cl. 7, amendment of 2026-09-22 — the only sentence a row may cite as entitled: "Inside ONE batched `run_verification_sequence`, on `L_Arena`, an `ECardType::Unit` card IDENTIFIED FROM A HAND READ TAKEN FIRST was PLAYED FROM HAND …".
  - `VER-§8` cl. 7, amendment of 2026-09-20: a slot index identifies nothing.
  - `VER-§8` cl. 10(b), with its 2026-09-20 and 2026-09-22 corrections: the fence, the round-trip bound, and the law's reading of attempt B.
- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` B1, M2, N2, N3).
- Used in part by `TASK-1512`, 2026-09-26, and re-verified in-run **partly**: "Measured: the entry composed in one batch (`None` → `StaticMeshActor_34` + entry line), and gold/census/log read-backs. **Failed:** the omitted-set control arm is **not** a control when the unset cursor already traces to a legal point." `[M: 1512 Recipes used]`
- Amended by `TASK-1516`, 2026-09-26, on `1512` and on `qa/TASK-1502-loop1.md` (W-L1-1, N-L1-1 … N-L1-5).
- Amended by `TASK-1527`, 2026-09-26, on `qa/TASK-1518.md` N9 and N10, and on `VER-§8` cl. 10(b)'s 2026-09-26 amendment (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`).

## Plugin version

**Not read by any source run.** Neither `1391`, nor any of the four Step 3 sources, nor `1512` ("Aura plugin version not read", *Not examined*) records one. Context only, not this recipe's measurement: `VER-§5` cl. 5 records `Aura.uplugin` `1.0.6` read by `TASK-1390` on 2026-09-22. ⇒ the first use re-verifies in-run (`VER-§13` cl. 3).

## Preconditions

1. Level `/Game/Maps/L_Arena`. The source loaded it: `load_level` → `{level: /Game/Maps/L_Arena, previous_level: /Game/Maps/L_MainMenu, already_open: false, loaded: true, discarded_unsaved: false}`, with `discard_unsaved` **not** passed. `[M: 1391 Editor/Aura state]`
2. PIE standalone, 1 client, 1280×720 requested → viewport **1280×725**, `client_index 0`; captures came back 1086×615. `[M: 1391 Editor/Aura state, Not-examined]`
3. **The budget: every observable lands before `t≈60 s`. The play batch is the first call after `start_pie`, and no other round trip is spent inside the fence.** `start_pie` itself cannot go in the batch: "PIE LIFECYCLE IS NOT IN THIS TOOL" (`run_verification_sequence`'s schema) `[S]`.
   - The law, quoted by text (emphasis marks dropped): "EVERY OBSERVABLE LANDS BEFORE `t≈60 s`". The reason: the undriven hero was measured dying "BETWEEN `t=82.6 s` AND `t=135.6 s`", and "THE ONLY SAFE READING OF THE MEASUREMENT IS ITS LOWER BOUND". `[L: VER-§8 cl. 10(b), 2026-09-20 correction]`
   - The planning rule: "a round trip costs BETWEEN `≈30 s` AND `≈70 s` OF PIE CLOCK, EACH END OBSERVED ONCE ⇒ BUDGET AT THE UPPER END. ⇒ A PLAN THAT SPENDS EVEN ONE ROUND TRIP INSIDE THE `t≈60 s` FENCE DOES NOT FIT, AND *"it fit last time"* IS LUCK, NOT A BUDGET." `[L: VER-§8 cl. 10(b), 2026-09-22 correction]` That is the 2026-09-22 text, quoted as written. Read it with the clause's 2026-09-26 amendment beside it (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`).
   - What the source measured against it `[M: 1391 §4]`:
     - Attempt A's one round trip cost **≈70.11 s** (batch 1 ended t=12.1365, batch 2 began t=82.2466). Its observables landed at t=82.2 → 83.1 s, "past the row's `t≈60 s` fence and at/just past the `t=82.6 s` earliest-observed bot kill". The hero was still alive (`pawn_class: BP_HeroCharacter_C` at t=83.084 s), which the report calls "margin I did not have, not margin I planned".
     - Attempt B's one batch finished by t=32.07 s, "half the `t≈60 s` fence".
   - Attempt B confirmed that the session had started from the sequence's own `pie_time_seconds` stamps, not from a separate `is_pie_active` call. Its first in-batch stamp was t=15.1659 s, which is what reaching the batch cost that run. `[M: 1391 Not-examined, C2]`
4. **Gold: the batch makes the card affordable itself.**
   - Passive income measured at **+1/s**: 22 @ t=12.10 → 92 @ t=82.26, and 25 @ t=15.18 → 41 @ t=31.23. `[M: 1391 H2]` Starting gold is `int32 Gold = 10;` in `SiegePlayerState.h`. `[D]`
   - Attempt B raised gold 25 → 41 with an in-batch `wait_pie_seconds(16)` ("costing no round trip") "so any card in the hand would be affordable". `[M: 1391 §4]`
   - ⚠️ **Gold must cover the cost at the card-key press, not only at the confirm.** An unaffordable card key is refused before placement: `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` at gold 88 logged `hand slot 0 ('Fog') refused — cost 5000, gold 88.`, and gold and hand did not change ("nothing spent"; hand "unchanged"). `[M: 1230 row 1]`
     - That measurement is on a **Spell**: `1230`'s header records its `DT_Cards` read as `Fog: Cost=5000 CardType=Spell`. `[M: 1230 header, row 1]`
     - That the same gate runs before placement for a **Unit** card is `[D]` only. In `ASiegePlayerController::PlayHandSlot`, the block commented `// affordability gate first (§3.5 spec order: the gold refusal outranks the type refusal …)` (`if (!SiegeState->CanAfford(Row->Cost))`, then `return;`) comes before `switch (Row->CardType)`.
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
     - a Building enters placement ("placement mode entered for card 'WatchTower' (cost 30)") `[M: 1348 P4]`, but its confirm is outside this recipe's fences. P4 does not state the card's type: that `WatchTower` is a Building comes from `1230`'s header `DT_Cards` read (`WatchTower: Cost=30 CardType=Building`). `[M: 1230 header]`
     - `MilitiaMob` is a swarm (`swarmCount: 4`), which the source never exercised. `[M: 1391 Not-examined]`
   - A hand dealt from a deck of one Unit card has no type exposure: every slot holds that card. Measured once: deck4 = 50× `Archer` gave `Hand` = 6× `"Archer"` and `DrawPile` = 44× `"Archer"`. `[M: 1512 A1]` ⚠️ The same deck blinds the hand observable, because the hand refills to an identical array (Read-back; Known hazards).

## Steps

**Step 1 — before PIE (no call): fix the card key, write down the tolerance, size the gold wait.**
- **Pick `N` for `IA_Card<N>`.** The Step 2 read documents what the slot held, so any `N` works. How keys map to `Hand` indices:
  - `IA_Card1` ↔ `Hand` index 0 is **measured**. `IA_Card1` acted on `Fog`, the only `Fog` in a hand read as `("Fog","Footman","Archer","Wall","Wall","WatchTower")` at t=01m26.9s. That read was 12.1 s before the press (t=01m39.01s), with no play between, and the hand read the same 0.81 s after the press. The log reads `hand slot 0 ('Fog') refused …`. `[M: 1230 row 1]` This rests on the card name. `1230` also states the mapping in-line in row 1 ("0-based slot 0 = `IA_Card1`"), not only in its H2.
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
- **The call and its reply, measured:** `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` → `status: action_injected`, `value_type: Boolean`. `[M: 1230 row 1]` ⚠️ **The reply is not the observable.** It came back the same on a press the game **refused** (the `Fog` row, Precondition 4), so it is not evidence of entry; that the press arrived was shown in `1230` by the handler's refusal line.
- **Path and spelling:**
  - The path for `N = 1` is quoted `[M: 1230 row 1]`.
  - For `N` = 2..6 it is `[D]`: the soft paths `/Game/Input/Actions/IA_Card<N>.IA_Card<N>` in `SiegePlayerController.cpp`, with `IA_Card1`…`IA_Card6` `.uasset`s present under `Content/Input/Actions/`.
  - `action_path` as the in-sequence param name, and the envelope, are `[S]` (the schema's "`inject_input_action`: soft path of the InputAction (required)").
- **Entry, measured on standalone calls:**
  - "Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*." `[M: 1314A ceiling 2]` The card name there is incidental. Cl. 7's 2026-09-20 amendment struck that quote as a worked example for exactly this reason. `[L: VER-§8 cl. 7]`
  - "Entered: `GhostActor` `"None"` → `StaticMeshActor_34`; log: *"placement mode entered for card 'WatchTower' (cost 30)"*". `[M: 1348 P4]`
  - "`IA_Card3`/`IA_Card6` entered placement"; "`IA_Card1` played an instant spell". `[M: 1348 P10]`
- **Entry, measured inside a `run_verification_sequence`:** in that session, "`IA_Card1` fired at t=01m07.23s, logged at `:3097 … placement mode entered for card 'Footman' (cost 9).`" `[M: 1270 row 3]` It was a sequence step. The report does not say so in words; two bases, both readings:
  - **Primary: `1270` row 3's own stamps.** A read of the HUD notice slot at t=01m07.22s, `IA_Card1` at t=01m07.23s, then `IA_DiscardAll` at t=01m07.57s. `[M: 1270 row 3]` A 0.01 s gap cannot contain an MCP round trip: `1512` measured round trips of ≈20 s and ≈36 s of PIE clock (`1512` *Speed data*), and the law lists every value observed so far (`VER-§8` cl. 10(b), marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`).
  - **Secondary: the call list.** `1270`'s list of its tool calls into PIE has no standalone `inject_input_action` ("Tool calls into PIE: 3 `run_verification_sequence` (4 + 37 + 12 actions), 1 `start_pie`, 1 `stop_pie`, 1 `stop_pie_recording`"). `[M: 1270 metrics]` That list may not be exhaustive: the same report names `ui_snapshot`, `survey_pie_scene` and `capture_pie_frame` calls it does not itemise (`qa/TASK-1502-loop1.md` N-L1-2).
  - That run was **Sandbox (No Bot)** on `L_Arena`.
  - Its entry was followed by `IA_DiscardAll` (t=01m07.57s) and `IA_CancelPlace`, **not** by a confirm.
- **The entry composed in one batch with a confirm and its post-reads: measured ONCE, with NO aim set.** `1391` does not name its entry, and `1270` never confirmed. `1512` a2 ran, in one `run_verification_sequence`: `GhostActor` `None` (t=33.58) → `IA_Card1` (t=33.61) → `GhostActor` `StaticMeshActor_34`, `bHidden false`, `RelativeLocation (X=-449.150572,Y=-1.707544,Z=0)` (t=33.79) → `simulate_key_press LeftMouseButton` (t=33.80) → post-reads. `[M: 1512 A3]`
  - Log: `placement mode entered for card 'Archer' (cost 12).`, then `played card 'Archer' for 12 gold — spawned 'BP_Unit_Archer_C_0' at (-449, -2, 92).` Gold 43 (t=33.79) → 32 (t=34.05); `BP_Unit_Archer_C` census 0 (t=33.59) → 1 (t=34.09); `GhostActor` → `None` after. `[M: 1512 A3]`
  - **Fenced to that run:** vs-bot, reached through the main menu; deck4 = 50× `Archer`; the hero walked to X=250.9 inside a Blue-owned `CaptureZone_Center`. The press that placed was the one **planned as the control**, and no `SetMouseLocation` had run in that session before it (`1512` A3 deviation).
  - **Still `NOT MEASURED`:** the entry composed with an aim that placed (SET → dependent read → confirm inside a live placement mode), and the entry composed with an omitted-set press that was refused. `1512`'s two `SetMouseLocation` steps ran after the play, "found no placement mode and aimed nothing". `[M: 1512 A3]`
  - **Position.** After the pre-reads, before the first press: ran once in `1512` a2 (pre-reads t=33.58–33.59, entry t=33.61). `[M: 1512 A3]` Its agreement with `1391` C4's control-arm census `167 → 168` ("the +1 being the ghost `StaticMeshActor_34` itself") stays a reading: `1391` does not quote the pre-read's time.
  - `[D]`: `EnterPlacementMode` calls `SpawnPlacementGhost()` and then logs `"… placement mode entered for card '%s' (cost %d)."` (`SiegePlayerController.cpp`).
  - **Spacing, measured once:** entry t=33.61 → `wait 0.2` → ghost read t=33.79 → press t=33.80. The ghost was visible and traced when read. `[M: 1512 A3; Recipe candidates item 6 for the wait]` Any other spacing is `NOT MEASURED`.
- **A slot index identifies nothing.** The hand is shuffled and redraws after every play: "Across three sessions slot 1 held `BrightSun`, then `Footman`, then `WatchTower`." `[M: 1348 two-gaps item 2]`; `[L: VER-§8 cl. 7, 2026-09-20]` The card is named from the Step 2 read and the Step 5 entry line, never from `N`.

**Step 4 — ONE `run_verification_sequence`: hand read, entry, a pre-press ghost read before EVERY press, the control arm (a control only if its gate held), the treatment arm, and the no-play-interval control, with every dependent read inside it.** `[M: 1391 C3, C4, §3]` for the arms; `[M: 1512 A3, ctl]` for the gate and the interval control. Rows 1–6 are this recipe's composition (Steps 2–3); rows 3–6, one press and the post-reads ran together once in `1512` a2.

| order | action | source |
|---|---|---|
| 1 | hand read + gold read (Step 2) | `[M: 1391 C2]` |
| 2 (as sized) | `wait_pie_seconds` (B: 16) | `[M: 1391 §4]` |
| 3 | pre-reads: `Hand` again, gold, `GhostActor` (expect `None`), census. These are also the "before" half of the no-play-interval control. | `[M: 1391 C2, C4, C5]`; `GhostActor` `None` before entry `[M: 1348 P4, 1512 A3]`; the pre-play half of the interval control `[M: 1512 ctl]` |
| 4 | **ENTRY**: `inject_input_action IA_Card<N>` (Step 3) | `[M: 1230, 1270, 1314A, 1348]`; composed with a confirm and post-reads once `[M: 1512 A3]` |
| 5 | `wait_pie_seconds` 0.2 | `[M: 1512 Recipe candidates item 6]`; A3 stamps entry t=33.61 → read t=33.79 |
| 6 | **PRE-PRESS GHOST READ (mandatory) = the control's GATE**, as the step immediately before the control press: `GhostActor`, `GhostActor.bHidden`, `GhostActor.RootComponent.RelativeLocation`. It is also the entry check (`GhostActor` non-`None`). | paths and values `[M: 1512 A3, Recipe candidates item 6]`; entry `[M: 1348 P4]` |
| control (**conditional**) | `simulate_key_press` `LeftMouseButton`, **no set step before it**. It counts as a control **only if row 6 read `bHidden: true`** (the `1391` C4 condition). If row 6 read `bHidden: false`, this press is an **un-aimed confirm**: in `1512` it placed the unit, and the rows after it found no placement mode. | arm `[M: 1391 C4, §3]`; failure `[M: 1512 A3]` |
| control reads | gold, `GhostActor` (+ `bHidden`, location), `Hand`, census | `[M: 1391 C4]` |
| treatment: SET | `pie_scene_edit` → `call_actor_function SetMouseLocation(300,420)` (source indices: A 11, B 15) | `[M: 1391 C3, Not-examined]` |
| **PRE-PRESS GHOST READ (mandatory) = the dependent read** | the row 6 read again, immediately before the confirm (source indices: A 13, B 17). The confirm places wherever the ghost is. | `[M: 1391 C3]`; paths and "the press confirms wherever the ghost is" `[M: 1512 Recipe candidates item 6]`, ghost `(-449.15, -1.71)` → spawn `(-449, -2, 92)` `[M: 1512 A3]` |
| CONFIRM | `simulate_key_press` `LeftMouseButton` (source indices: A 14, B 18) | `[M: 1391 C3]` |
| post-reads | gold, `Hand`, census (source indices: A 16/17/18, B 20/21/22); add `DeckComponent.DrawPile` when the hand can refill to an identical array (Known hazards) | `[M: 1391 C3, C5]`; `DrawPile` in-batch read `[M: 1512 A1]` (t=2.74) |
| **no-play interval** | the same reads (gold, census) again after the last press, with no card key sent; then the log (Step 5). **Length:** `1512`'s ran t=34.05 → 54.60 (≈20.5 s), of which only t=34.05 → 37.45 was inside the batch (Read-back). The length inside this batch is `NOT MEASURED`; the report states it, from its own stamps, next to the treatment window (the ctl row asks for "an equal interval") (`qa/TASK-1518.md` N10) | `[M: 1512 ctl, Speed data]` |

**Choosing whether to send the control press at all is the row's call; neither choice was measured as a whole batch.** A batch cannot branch, so row 6 is judged **after** the batch.
- **Send it:** if the gate held, the arm discriminates the aim, as in `1391` C4. If it did not hold, the press confirms un-aimed and the treatment arm is lost, as in `1512` A3.
- **Skip it:** the SET → read → confirm then runs on a live placement mode, and the no-play interval is the only control. That control "discriminates spend/spawn from income" `[M: 1512 ctl]`. It says nothing about the aim (this recipe's reading: in `1512` no aim was set).

The indices are `1391`'s own and will shift with rows 1–6. Measured span of the source batch: A 21 actions, t=82.2466 → 83.1009 s (**0.854 s**); B 25 actions, t=31.1996 → 32.0731 s (**0.874 s**). "No MCP round trip separates any set from its dependent read." `[M: 1391 C3]` `1512` a2's batch, which also carried the menu route and the walk: "ONE `run_verification_sequence`, 48 actions, menu t=20.42 → arena t=37.45". `[M: 1512 Speed data]`

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
  `1391` read the ghost's `bHidden` and location (C4/§3 quote them), but quotes the path only as "`GhostActor…`" and names `RelativeLocation` in H1.
- **Pre-press ghost read (rows 6 and the dependent read):**
```json
{"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["GhostActor", "GhostActor.bHidden", "GhostActor.RootComponent.RelativeLocation"]}}
```
  - The three property paths are quoted from `1512`: "`GhostActor`, `GhostActor.bHidden`, `GhostActor.RootComponent.RelativeLocation` (this dotted path WORKS; it was measured non-null this run)". A3 gives the values: `StaticMeshActor_34`, `bHidden false`, `RelativeLocation (X=-449.150572,Y=-1.707544,Z=0)`. `[M: 1512 Recipe candidates item 6, A3]`
  - `1512` does not quote the params object. Putting the three paths in one read is this recipe's choice; `name` is per Precondition 6; the envelope is `[S]`.
- **Census:** `1391` reports totals and per-class counts ("census `168 → 169`", "`BP_Unit_Footman_C` count 1", "`Unit_` classes still only `BP_Unit_Pikeman_C ×1`") but does not name the instrument. `1512` names it, class-filtered:
```json
{"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}}
```
  - The `include`/`class_filter` values are quoted from `1512` *Recipe candidates* item 6. It read `BP_Unit_Archer_C` 0 (t=33.59) → 1 (t=34.09), with `BP_Unit_Archer0` at `(-382.86, -1.58, 92)`. `[M: 1512 A3]` The envelope is `[S]`.
  - Measured once, with `"Archer"` only. Another card's filter value is this recipe's substitution (`NOT MEASURED`). An unfiltered total like `1391`'s is `NOT MEASURED` from this tool.
- Gold wait: `{"type": "wait_pie_seconds", "params": {"seconds": 16}}` (`[M: 1391 §4]` for the 16; spelling `[S]`; the schema caps one wait at 30 s).

**Step 5 — after the batch, read the log: three lines.** Category `LogGitClaudeUnrealTest`, in `Saved/Logs/GitClaudeUnrealTest.log`.
- **The entry line**, `placement mode entered for card '<CardID>' (cost <n>)`: one per entry, naming the card the key actually entered. `[M: 1314A ceiling 2, 1348 P4, 1270 row 3, 1512 A3]`; `[D]` one `UE_LOG` per `EnterPlacementMode`.
  - If it is absent, or names anything but a clean Unit card (Step 2 criteria), the arms measured nothing about a Unit placement.
  - Report what you got (Precondition 8).
- **The refusal line**, substring `placement click refused`. The source found `total_matches = 2`: exactly one per control arm, none for either treatment arm. `[M: 1391 C6]`
- **The play line**, substring `played card '`. `1512` a2 logged `played card 'Archer' for 12 gold — spawned 'BP_Unit_Archer_C_0' at (-449, -2, 92).` after its one placing press, and "no second `played card` line" over the no-play interval. `[M: 1512 A3, ctl]` The general form `… played card '%s' for %d gold — spawned '%s' at (%.0f, %.0f, %.0f).` is `[D]` (`SiegePlayerController.cpp`).
  - If a play line follows the **control** press, that press confirmed and the arm is not a control, whatever row 6 read (Read-back).
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

The three observables, named in advance, **none of them the absence of a refusal line** (`[M: 1391 C5]`; law `VER-§8` cl. 7 boarding rule (iv)). Two checks come first: the entry check decides whether the arms mean anything, and the gate decides whether the control arm is a control.

| observable | control arm (set omitted): must NOT move, **and it is a control only if the gate held** | treatment arm: must move |
|---|---|---|
| **entry (checked first)** | `GhostActor` `None` → non-`None` after row 4 (`1348` P4: `None` → `StaticMeshActor_34`), and one entry log line naming a clean Unit card (Step 5). **If either fails, neither arm is evidence.** | (same entry) |
| **the gate (checked second): the ghost read immediately before the press** | **`bHidden: true`** (`1391` C4: `bHidden: true` at `(0,0,0)`). If it read `bHidden: false`, **this column is not a control**: report the press as an un-aimed confirm (below) and use the no-play-interval control. | `bHidden: false` at the aim point (see coordinates) |
| gold | A `92 → 92`, B `41 → 41` | A `92 → 84`; B **`41 → 29` = exactly `DT_Cards.Archer.cost` 12** |
| hand (index 1 in the source) | unchanged, both | A `Footman → MilitiaMob`; B `Archer → MilitiaMob`. ⚠️ With a one-card deck the hand refills to the identical array and cannot show the play: use `DrawPile` −1 (`1512` A3, Known hazards). |
| new unit actor | none, both (the census +1 is the ghost `StaticMeshActor_34` itself) | A `BP_Unit_Footman0` (class `BP_Unit_Footman_C`); B `BP_Unit_Archer0` (class `BP_Unit_Archer_C`) |
| `GhostActor` after | still `StaticMeshActor_34` (placement mode alive) | **`None`** ⇒ `ExitPlacementMode` ran |
| refusal log line | one per arm, quoted (the positive control that the button arrived) | none |

All rows except "entry" and the gate are `[M: 1391 C4, C5, C6, §3]`. The entry row is `[M: 1348 P4, 1314A, 1270, 1512 A3]` plus this recipe's composition. The gate's `bHidden: false` failure case is `[M: 1512 A3]`. Pixels corroborate only: both `1391` treatment frames show an opaque placed unit, not a translucent ghost. `[M: 1391 Evidence]` In `1512` the placed Archer was "not identifiable by eye" in either frame, and the reads and log carried the finding. `[M: 1512 Evidence]`

**The gate's timing in the sources.** `1391` C4 stamps its control-arm ghost read **after** its press (A: press t=82.4905, read t=82.7072; B: press t=31.4496, read t=31.6663), and §3 tabulates that value as "ghost before the press". `[M: 1391 C4, §3]` The first read taken **before** a press is `1512` A3's (read t=33.79, press t=33.80). `[M: 1512 A3]`

**When the gate fails (measured once, `1512` A3).** The ghost read `bHidden false` at `(X=-449.150572,Y=-1.707544,Z=0)` inside a Blue-owned zone. The planned control press then placed: gold 43 → 32, `BP_Unit_Archer_C` 0 → 1, `DrawPile` 44 → 43, `GhostActor` → `None`, and the log line `played card 'Archer' for 12 gold — spawned 'BP_Unit_Archer_C_0' at (-449, -2, 92).` The SET steps after it "found no placement mode and aimed nothing", and the second `LeftMouseButton` placed nothing (the hero attacked: "the second `LeftMouseButton` = `IA_Attack`"). `[M: 1512 A3, ctl, Speed data, Evidence]` ⇒ the batch measured an un-aimed play, not the aim.

**The no-play-interval control (`1512` ctl): the observables must NOT move while no card key is sent.**

| observable | before the play (no card key) | at the play | after the play (no card key) |
|---|---|---|---|
| gold | 12 → 43, t=2.8 → 33.6 | 43 → 32 | 32 → 33 → 35 → 52 over t=34.05 → 54.60, "only rising" |
| unit census (`BP_Unit_Archer_C`) | 0 at t=33.59 | 0 → 1 | 1 at t=35.28 and t=37.43 |
| `DrawPile` | 44 (t=2.74, `1512` A1) | 44 → 43 | 43 at t≈55 |
| `played card` log line | — | one | "no second `played card` line" |
| hand | "did not change" | refilled to 6× `Archer` (blind: one-card deck) | — |

All cells `[M: 1512 ctl, A1, A3]`. The interval ran from t=34.05 to t=54.60 (≈20.5 s) and "includes a second `LeftMouseButton` at t=34.72 with `GhostActor` `None`". `[M: 1512 ctl]`
- **In the batch or not:** the 48-action batch ended at arena t=37.45 `[M: 1512 Speed data]`. Every read after that, including `DrawPile` 43 at t≈55 and the interval's end at t=54.60, came from a later call. The `Hand`/`DrawPile`/`DiscardPile` count 6/43/1 was taken by read-only editor Python. `[M: 1512 A3, Not-examined]` Inside the batch, the post-play half of the interval covered t=34.05 → 37.45, and its evidence there is the gold and census reads.
- **What it proves:** it "discriminates spend/spawn from income, and it shows the treatment's −gold/+unit/−draw only at the play". `[M: 1512 ctl]` It does **not** discriminate the aim. That needs the omitted-set arm with its gate held (`1391` C4). This last sentence is this recipe's reading.

## Fences — not measured for

- **The entry composed in ONE batch with an aim that placed, or with a control press that was refused.** Never run. What ran once is entry → pre-press ghost read → un-aimed confirm → post-reads (`1512` a2; Step 3). The first use that sets an aim on a live placement mode measures the rest, and reports it as a recipe candidate if it holds.
- **The gate, beyond one failure.** `bHidden: true` before the press has never been read and then followed by a refused press in the same batch. `1391` C4's `bHidden: true` was read after its press (Read-back). The gate rests on `1391` C4, on `1512` A3's failure case, and on `[D]` (Known hazards).
- **Card key ↔ hand index for `IA_Card2`…`IA_Card6`**: `[D]` only. Only `IA_Card1` ↔ index 0 is measured (`1230`).
- **`ECardType::Unit` cards only** (`Footman`, `Archer`). "A row that reads this as *'any card can now be played'* has over-read it." `[M: 1391 Not-examined]` Untouched:
  - the Building path (`bPendingIsBuilding`);
  - the upgrade path (`PlacementUpgradeState::Ready → ConfirmStackUpgrade`);
  - the swarm path (`MilitiaMob`, `swarmCount: 4`);
  - the miner-cap path;
  - the spell path (`TryConfirmSpellTarget`, not a `UFUNCTION`). An instant spell is played by the card key itself (`1348` P10), so the in-batch shape can spend gold and a card before the control arm if the slot holds one.
- **One of `TraceCursorToGround`'s three callers** (the placement ghost). The spell reticle and the group-pick reticle were not probed. `[M: 1391 Not-examined]`
- **No accuracy claim.** `(300,420)` is "not proven to be any particular viewport location … Reproducible ≠ calibrated." `[M: 1391 Not-examined]`
- **One aim point, at the Blue spawn, from the hero's starting camera.** Aiming anywhere else with `SetMouseLocation`, **including into a capture zone**, is `NOT MEASURED`.
  - The refusal text names the legal regions: "outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh". `[M: 1391 C6]`
  - A placement **inside** a Blue-owned zone confirmed once, **with no aim set**. `CaptureOwner` read `Blue` at t=33.54, the ghost sat at the unset cursor's trace (−449.15, −1.71), and the press at t=33.80 spawned `BP_Unit_Archer_C_0` at (−449, −2, 92). `[M: 1512 A2, A3]` That is not an aim point: "an unset cursor is not a reproducible aim", and its mechanism is `HYPOTHESIS` H1 (`1512` *Recipe candidates* fences, H1). A calibrated `SetMouseLocation` into the zone is `NOT MEASURED`. `[M: 1512 Not-examined]`
- **No persistence across a round trip.** Both aims were set and consumed inside one batch; "nothing here licenses a two-call *'set the cursor, then read the ghost'*". `[M: 1391 Not-examined]`
- **The census instrument.** `1391` names none. `survey_pie_scene` with `include: ["census"]` and `class_filter: "Archer"` was used once, by `1512` (Step 4). Other filter values, and an unfiltered total like `1391`'s, are `NOT MEASURED` from it.
- **`inject_input_action IA_Attack` as a confirm** is wall (a) and stands unrelaxed: an Enhanced Input action sets the action, not the key state the confirm polls. This recipe confirms with `simulate_key_press` only; its `inject_input_action` is the **entry** (`IA_Card<N>`), never the confirm. `[M: 1391 §2]`; law `VER-§8` cl. 7.
- **Scope of the runs:** `L_Arena` only; standalone single client; `client_index 0`. The one-batch shape (B, as the law reads it) and the two-call alternative (A) only. `1270`'s in-sequence entry was Sandbox (No Bot). `1512`'s composed batch was vs-bot, reached through the main menu (`IA_MenuAccept`, not `load_level`), with deck4 = 50× `Archer`, the hero walked to X=250.9, and one run. `[M: 1512 A1, A2]`
- Attempt A's aim point differs from the canonical one: `HYPOTHESIS` H1, "MECHANISM NOT MEASURED". `[M: 1391 H1]`
- Plugin version: not read.

## Coordinate/resolution-dependent values

| value | measured at | notes |
|---|---|---|
| `SetMouseLocation(300, 420)` (viewport coordinates passed to `APlayerController::SetMouseLocation`) | viewport 1280×725 (1280×720 requested), `client_index 0`, capture 1086×615; **DPI not read** | reproducible, **not calibrated** `[M: 1391 Not-examined]` |
| aim point B `(X=-21382.831129, Y=407.684206, Z=29.000000)` | same, hero at the start camera ("a byte-identical camera") | "BIT-IDENTICAL" to `TASK-1357`'s and `TASK-1352`'s values: three runs, six decimals `[M: 1391 §3]` |
| aim point A `(X=-21440.000107, Y=437.757051, Z=32.938384)` | same screen point, same camera | unexplained (`HYPOTHESIS` H1) `[M: 1391 H1]` |
| spawned unit A `(-21394.41, 405.35, 121.15)`, B `(-21341.37, 377.43, 109.08)` | — | the pale stone ledge at mid-left in both frames `[M: 1391 C5, Evidence]` |
| `1512`'s ghost point with the cursor **unset**: `(X=-449.150572, Y=-1.707544, Z=0)`; spawn `(-449, -2, 92)` | viewport 1280×725, **DPI 0.6706**, `client_index 0`; hero stopped at X=250.89, Y≈0; camera yaw 180; cursor position **not read** | **not an aim**: "an unset cursor is not a reproducible aim"; mechanism `HYPOTHESIS` H1 `[M: 1512 A3, H1, Not-examined, Editor/Aura state]` |
| `CaptureZone_Center` box: centre `RelativeLocation (X=0,Y=0,Z=0)`, `ZoneHalfExtent (X=840, Y=840)` ⇒ X,Y ∈ [−840, 840] | live read, a2 t=2.73 | a world box, not a screen value. ⚠️ Its centre is `(0,0,0)`, the same point `1391` C4 read the hidden ghost at (Known hazards) `[M: 1512 A2]` |

The world point under `(300,420)` depends on the camera, i.e. on where the hero stands and looks. Move the hero, change the viewport, or change the camera, and this recipe's aim is unmeasured. An **unset** cursor "is not a reproducible aim" (`1512` *Recipe candidates* fences; mechanism `HYPOTHESIS` H1). The entry step has no coordinate.

## Known hazards

- **A batch cannot branch on its own result:** "the card cannot be chosen until the hand is known". `[M: 1391 §4]`
  - In the primary shape the batch therefore runs to its end whatever the slot held, and whether the entry happened.
  - Judge the arms only **after** checking the entry row and then the gate row of the Read-back.
  - Attempt A avoided this by splitting into two calls and paid ≈70.11 s for it (Alternative).
- **The omitted-set press confirms whenever the ghost is visible at a legal point.** `1512`: "No `SetMouseLocation` had run in this session before it. The ghost was already visible at an in-zone, legal point (−449, −2), so the "control" press placed the unit." `[M: 1512 A3]` The unset cursor gave `bHidden: true` at the Blue spawn from the start camera (`1391` C4) and a visible in-zone point at X=250.9 (`1512` A3). Which one a run gets is not known before the batch (`1512` H1: "If Jonathan's mouse was over the PIE window, the aim came from his cursor").
- **`(0,0,0)` is not the gate; `bHidden` is.** `1391` C4 read the hidden ghost at `(0,0,0)`, and `(0,0,0)` is also the centre of `CaptureZone_Center` `[M: 1512 A2]` (A2 read `RootComponent.RelativeLocation`, which equals the actor location only for an unattached root: `NOT MEASURED`), a legal point whenever that zone is Blue-owned (`1391` C6's refusal text). So a visible ghost at `(0,0,0)` does not meet the C4 condition. The rest of this bullet is `[D]`, from `UpdatePlacementGhost` and `ResolvePlacementUpgradeState` in `SiegePlayerController.cpp`:
  - `GhostActor->SetActorHiddenInGame(!bGroundHit);`, so hidden means no ground hit that frame.
  - `bool bValid = bGroundHit && (IsPointInOwnSpawnBox(Hit.ImpactPoint) || IsPointInCapturedZone(Hit.ImpactPoint));`, so no ground hit means invalid.
  - The upgrade override cannot turn that valid for a Unit card: `if (!bPendingCardIsBuilding || PendingCard.IsNone())` returns `EPlacementUpgradeState::None`.
  - `GhostActor->SetActorLocation(PlacementLocation);` runs only inside `if (bGroundHit)`, so a hidden ghost's location is stale and carries no information.
- **The gate read and the press are different frames** (`[D]`, plus this recipe's reading). The ghost is re-traced every frame (`// per-frame cursor-to-ground trace + ghost position/color (GDD §3.5)`), and the confirm is polled after that, in the same function (`// Confirm: LMB polled while in mode.`). The press is judged on the cursor at its own frame, so a cursor that moves between the read and the press is not caught. Keep the read the step immediately before the press, as `1512` did (read t=33.79, press t=33.80). `[M: 1512 A3]`
- **A one-card deck blinds the hand observable.** "`Hand` is still 6× `Archer` because it refilled, so with an all-Archer deck the hand array cannot show the change by itself." `1512` rested "the card left the hand" on `DrawPile` 44 → 43 and `DiscardPile` = 1. `[M: 1512 A3, Not-examined]` Read `DeckComponent.DrawPile` before and after in such a run.
- **A dead hero cannot play cards.** Past the fence, `1512` attempt 1's `IA_Card1` at t=83.48 left `GhostActor` `None` and logged `PlayHandSlot(0) refused — the hero is dead and the ghost cannot play cards (GHOST-§ G-5).` By log time the hero had died ≈73 s after the arena opened. `[M: 1512 Attempt 1]` That it died to Red units while holding the zone alone is `HYPOTHESIS` H2 ("The killer was not read"). The entry row of the Read-back catches it.
- **Attempt B's timeline, as the source states it, admits more than one reading.** C2 has hand reads at t=15.1659 and t=31.2163. §4 says "one batch, everything inside it … with an in-batch `wait_pie_seconds(16)`". C3 says "one call, 25 actions, t=31.1996 → 32.0731 s". `[M: 1391 C2, C3, §4]`
  - Unambiguous: the hand was read before any press; the two B reads 16 s apart were byte-identical (no play ⇒ no redraw in that window); and the set, confirm and dependent reads shared one call.
  - The law reads B as one batch with the hand read inside it, documenting rather than selecting. `[L: VER-§8 cl. 10(b), 2026-09-22]` This recipe's primary shape follows the law's reading.
- **The hand redraws after every play:** index 1 was refilled by `MilitiaMob` both times. `[M: 1391 C5]` Re-read before any second play. A slot index identifies nothing. `[L: VER-§8 cl. 7]`
- **An unaffordable card key is refused before placement**, and gold and hand stay put. Measured on a Spell (`Fog`) `[M: 1230 header, row 1]`; for a Unit card, `[D]` only (Precondition 4). In the primary shape that silently removes the entry, and the entry row of the Read-back catches it.
- **`status: action_injected` is not the observable.** It came back on a refused press. `[M: 1230 row 1]`
- **`binding_found` is actively misleading here:** every `simulate_key_press` returned `binding_found: true`, `bound_actions: ["IA_Attack"]` on the arm that placed and the arm that refused alike. It is not the observable. `[M: 1391 Not-examined]`; law `VER-§8` cl. 10.
- **Unreadable members:** `bPlacementValid`, `PendingCardID`, `PendingCost`, `PlacementLocation` have no `UPROPERTY` and cannot be read. Use `GhostActor`/`bHidden`/gold/hand/census and the entry log line. `[M: 1391 Not-examined]`
- **Gold reads can straddle an income tick.** A's `−8` against a cost of 9 is explained as one +1 tick inside a 0.771 s window, which the source labels "an INFERENCE, not a separate measurement" (H2). `[M: 1391 H2]` `1512` saw the same: 43 → 32 against a logged cost of 12 inside 0.26 s, "which is an inference" there too. `[M: 1512 A3]` Always assert "gold went down". Assert "down by exactly the cost" only when the timestamps rule a tick out.
- **Label vs object name:** the working `name` is the label `SiegePlayerController0`; the log line prints the object name `'SiegePlayerController_0'`. `[M: 1391 §1, C6]`
- **`start_pie`'s reply overflowed** the tool-output limit on both sessions (264,336 and 504,242 characters). Session start was confirmed by `is_pie_active` (A) and by the sequence's own `pie_time_seconds` (B). `[M: 1391 Not-examined]`; law `VER-§12` cl. 6.
- **Grant declaration:** `pie_scene_edit` is not a standalone tool on the verifier's surface. It was reached **only as a step inside `run_verification_sequence`**, twice, one `call_actor_function` op each, no other op. `[M: 1391 §1, Not-examined]` `1512` reached it the same way, 4 times, all `SetMouseLocation` on `SiegePlayerController0`, and "**None of them aimed a placement**". `[M: 1512 Not-examined]`
  - A run using this recipe declares the same, line by line, under *Not examined* (`VER-§7` cl. 2).
  - Whether it becomes a standing grant is Jonathan's and the orchestrator's call (`VER-§8` cl. 7).
- **Environment:** frame A carried a red "Video memory has been exhausted" banner; recorded, not diagnosed. `[M: 1391 Editor/Aura state]` The same banner appeared in every `1512` frame (1058–1062 MB over in attempt 1, 182 MB in a2). `[M: 1512 Not-examined]`
