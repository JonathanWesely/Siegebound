# TASK-1516 — programmer handoff: amend the three existing recipes (RECIPE-AMEND-EXISTING)

Marker `TASK-1516-RECIPE-AMEND-EXISTING` · gameplay-programmer · 2026-09-26 · status → `ready-for-qa` (gate `TASK-1518`, host `TASK-1520`).

Docs-only. No source, asset, editor, PIE, compile or git write. Files written: the three `RCP-*.md` named by the row, this handoff, and my own row's `status:` line. ⛔ `Tools/Verify/recipes/README.md` was **not** touched by this row (it is `TASK-1515`'s; its hash moved during my run from `fa5cc2d0…aa2d25` to `a49693ea…deeb40`, and that change is `TASK-1515`'s). ⛔ The two new recipe files (`RCP-deckbuilder-set-active-by-keyboard.md`, `RCP-vsbot-capture-center-and-summon.md`) were not touched.

## Files touched

| file | before (sha256 · lines · bytes) | after (sha256 · lines · bytes) | lines +/− |
|---|---|---|---|
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` | `fc1d94ddb0731d50dbc93c49ca5d787f02513c927de23d9c8cba67291cd047b8` · 235 · 30225 | `4964961cc6ec0b95850fa93ea13bc1bbc2f5ea49f4de42d6421af713ca8bd092` · 297 · 48301 | +102 / −40 |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | `6dcd0013ae7a5471ca2074c0eca4d97be2a5f7710c4768f73ba3bf47976bda15` · 164 · 18336 | `16eb942d03f6f58b04f62a4060b951343ee84c0ec5f84886a68213e869ee740b` · 164 · 19202 | +4 / −4 |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | `2f0e485decb7d98008e78937c14d887266b5550860682cd80b8a895256117534` · 86 · 7802 | `6f783f3d67b0e5910be11acb88d83365b3a23c33e7ab5418d5a9408e79be9992` · 100 · 11190 | +27 / −13 |

The "before" hashes equal `qa/TASK-1502-loop1.md`'s byte anchors (and the committed `36cb4dd` state). All three files stay LF, UTF-8. Line counts: `\n` count. +/− from `diff` against a snapshot taken before the first edit (not git).

## How the control was redesigned, and what it rests on

**The finding (`qa/TASK-1512-verify.md` A3 + `ctl`):** the omitted-set `LeftMouseButton` press is a control only when the confirm cannot succeed without the aim. In `1512` a2, no `SetMouseLocation` had run, and the unset cursor already traced the ghost to `(−449.150572, −1.707544, 0)`: visible, inside the Blue-owned `CaptureZone_Center`. The press planned as the control placed `BP_Unit_Archer_C_0`. The later SET steps "found no placement mode and aimed nothing". The recipe's source for the arm (`qa/TASK-1391-verify.md` C4) had read the ghost `bHidden: true` at `(0,0,0)`.

**The redesign, in the recipe:**
1. **A pre-press ghost read before EVERY press** (Step 4 row 6 before the control press; the dependent read before the confirm). It reads `GhostActor`, `GhostActor.bHidden` and `GhostActor.RootComponent.RelativeLocation`, the paths `1512` measured (*Recipe candidates* item 6: "this dotted path WORKS"; values in A3). It is the step immediately before the press, as in `1512` (read t=33.79, press t=33.80).
2. **The gate:** the omitted-set arm counts as a control only if its pre-press read shows `bHidden: true` (the C4 condition). If it read `bHidden: false`, the press is reported as an un-aimed confirm, and the control is the no-play interval.
3. **The no-play-interval control is carried in every batch** (Step 4's last row; a Read-back table with `1512` ctl's values). A batch cannot branch (`1391` §4), so the gate is judged after the batch, and the fallback control must already be in it. The recipe states what this control proves ("discriminates spend/spawn from income", `1512` ctl) and, labelled as the recipe's reading, what it does not prove: the aim.
4. **Whether to send the omitted-set press at all is left to the row**, with both consequences stated from measurement: sent with the gate held, it discriminates the aim (`1391` C4); sent with the gate failed, it confirms un-aimed and loses the treatment arm (`1512` A3). The whole-batch outcome of either choice is labelled not measured.
5. **The gate's mechanism** is `[D]`, cited by code text (`SC-§126` cl. 12), from `UpdatePlacementGhost` / `ResolvePlacementUpgradeState`. `GhostActor->SetActorHiddenInGame(!bGroundHit);` means hidden = no ground hit. `bool bValid = bGroundHit && (…)` means no ground hit = invalid. For a Unit card the upgrade override returns `None`. The ghost's location is written only inside `if (bGroundHit)`.

## Change index (spec item → hunk numbers in the quotes section below)

**(1) `RCP-play-unit-card-from-hand.md`**
- **(a) control arm:** hunks 2 (Read-this-first bullet), 3 (Source), 14–18 (Step 4 heading, table, conditional control, mandatory pre-press reads, no-play-interval row, the choosing paragraph), 19 (pre-press ghost read JSON), 21 (play log line), 22–27 (Read-back: gate row, gate timing, gate-fail case, interval table; hunk 26 deletes the old "ghost before the press" row, merged into the gate row), 28 (gate fence), 33–36 (coordinates; hazards: the misfire, `(0,0,0)` ≠ gate, read vs press frames).
- **(b) composed batch measured once:** hunks 2, 3, 8 (one-card deck), 12–13 (Step 3: composition + position + spacing), 15 (row 4/5 labels), 19 (census JSON), 20, 25, 27, 28–32 (fences: composition, zone placement, census, scope), 33, 36 (one-card deck, dead hero), 38–40.
  - The labels changed **only that far**: the entry → pre-press read → confirm → post-reads composition, the read paths and the class-filtered census are now measured-once. The aim (SET composed with the entry), an omitted-set press that was refused, the calibrated in-zone aim, and other census filters all stay `NOT MEASURED`.
- **(c) `qa/TASK-1502-loop1.md`:**
  - W-L1-1: hunk 2, using QA's fix text verbatim.
  - N-L1-1: hunk 10, QA's text.
  - N-L1-2 (optional, done): hunk 11.
  - N-L1-3: hunks 6 and 37.
  - N-L1-4: hunk 7.
  - N-L1-5: hunk 9.
- Bookkeeping: hunks 1 (short name `1512`), 4 (revision lines), 5 (plugin version: `1512` did not read it either).

**(2) `RCP-deckbuilder-slot-and-card-edit.md`**
- N-L1-6: hunk 1 (line 16), and hunk 2's reason 1 (line 84), using QA's text "the one call whose error text it quotes carried the root".
- Step 4 cites `VER-§13` cl. 1's 2026-09-26 ruling, quoted by text: hunks 2–3.
- **No other change**: 4 lines changed (16, 83, 84, 88), confirmed by `diff`.

**(3) `RCP-menu-to-deckbuilder.md`**
- `1511`'s in-run re-verification of Steps 0–4, recorded: hunk 2 (Source, verbatim from `1511` *Recipes used*), and hunks 7–10 (Steps 2–4, Read-back).
- The batched `IA_MenuDown` ×2 moves to measured-once, fenced to that run: hunk 6 (Step 1) and hunk 11 (a new fence).
- "`TASK-1512` used Step 0 only": hunks 2 and 5.
- Also recorded:
  - hunk 5: `1512` names the Step 0 reader (`ui_snapshot` + name selector), quoted JSON from its *Recipe candidates* item 1.
  - hunk 3: plugin version still unread.
  - hunk 4: starting focus re-seen twice.
  - hunk 11: "Play (vs Bot)" was measured by `1512` A1, and promoting it is `TASK-1515`'s row.

## The eight headings, still present, in order, in every file (checked by script)

Each of the three files has exactly these `## ` headings, in this order: `## Source` · `## Plugin version` · `## Preconditions` · `## Steps` · `## Read-back` · `## Fences — not measured for` · `## Coordinate/resolution-dependent values` · `## Known hazards`.

Every ```json block parses: play **7/7** (was 5; +pre-press ghost read, +census), deck-builder **7/7**, menu **5/5** (was 4; +`1512`'s Step 0 selector form).

## For QA (`TASK-1518`): scrutinise these

1. **The gate is `bHidden: true` ALONE. The row's spec (1)(a) says "hidden or at (0,0,0)".**
   - I did not adopt the "or at (0,0,0)" disjunct (`SC-§101`: a prescribed remedy is a claim). `(0,0,0)` is the measured centre of `CaptureZone_Center` (`1512` A2: `RelativeLocation (X=0,Y=0,Z=0)`, half-extent 840). A visible ghost at `(0,0,0)` is therefore a legal point whenever that zone is Blue-owned (`1391` C6's refusal text).
   - In the zone use case the disjunct would re-admit exactly the failure this row fixes. A hidden ghost's location is stale by code (`[D]`: location written only on a ground hit), so it adds nothing.
   - The recipe says this in a hazard ("`(0,0,0)` is not the gate; `bHidden` is") and in the Coordinates row. If you rule the disjunct must stay, it is a one-cell change in the Step 4 table, the Read-back gate row, the box, and that hazard.
2. **`1391` C4's "before the press" ghost value was read AFTER the press.** A: press t=82.4905, read t=82.7072. B: press t=31.4496, read t=31.6663. `1391` §3 tabulates it as "ghost before the press". I recorded this in the Read-back ("The gate's timing in the sources") and in a fence ("The gate, beyond one failure"). The first read taken before a press is `1512`'s (t=33.79 → 33.80). So a gate that held (`bHidden: true` read before a press that was then refused) has **never** been observed in one batch. The recipe says so.
3. **N-L1-2's "floor ≈30 s" was NOT copied.** `1512` *Speed data* measured round trips of ≈20 s and ≈36 s of PIE clock, and ≈20 s is below `VER-§8` cl. 10(b)'s "BETWEEN ≈30 s AND ≈70 s". The recipe cites `1512`'s ≈20 s as the shortest measured, and the law's range separately. Also, `1270` row 3's t=01m07.22s read is of the **HUD notice slot** (`TextBlock_21`), not a hand slot, and the recipe says "HUD notice slot".
4. **The no-play-interval control spans the batch boundary** in `1512`: the batch ended at arena t=37.45, and `DrawPile` 43 (t≈55) came from a later call / read-only editor Python. The recipe states which cells were in-batch (the pre-play half and gold/census to t=37.45) and which were not.
5. **Step 4's "choosing" paragraph** (send the omitted-set press or skip it) is guidance, labelled as not measured as a whole batch. Each consequence it states is cited (`1391` C4; `1512` A3; `1512` ctl). The sentence "It says nothing about the aim" is labelled as this recipe's reading.
6. **Scope beyond the letter of (1)(b), all measured and cited:**
   - Step 5's third log line (`played card …`, with the `[D]` format string).
   - The one-card-deck blind-hand hazard (`1512` A3 quote).
   - The dead-hero refusal (`1512` Attempt 1 line; its cause stays `HYPOTHESIS` H2).
   - `1512`'s banner and grant-declaration counts, and the gold-tick inference.
   - Please check each against (4), "nothing unmeasured is added".
7. **The census JSON** `{"include":["census"],"class_filter":"Archer"}` is quoted from `1512` *Recipe candidates* item 6. The `params` envelope is `[S]`. Any filter value other than `"Archer"` is labelled `NOT MEASURED`.
8. **The pre-press read's grouping** (three paths in one `get_actor_property_in_pie`) is labelled as the recipe's choice. `1512` quotes the three paths, not its params object.
9. **Menu recipe Step 1:** `1511` says "3 frames between injections" but does not quote its action list. The recipe keeps the no-wait JSON and labels the source of the 3 frames (a wait step, or the runner's own spacing) `NOT MEASURED`.

## For the manager (flags only; outside this row's files or its "No other change")

- **`RCP-deckbuilder-slot-and-card-edit.md` still carries two stale lines that (2)'s "No other change" kept me from touching:**
  - Step 2 (line 52): "It does **not** make it the active deck (right-click only; no route, `VER-§8` cl. 12)."
  - Fences (line 131): "**Setting the active deck.** Right-click only … Pending: `TASK-1511`."
  - Both are superseded by `1511` A1 (`IA_MenuSecondary` on a focused bar slot) and by `VER-§8` cl. 12's amendment. A one-line follow-up row could point both at `RCP-deckbuilder-set-active-by-keyboard.md` once `TASK-1515` lands.
- **`VER-§8` cl. 10(b)'s round-trip range (≈30–70 s) has a measured value below its lower end:** `1512`'s ≈20 s. The upper-end budgeting rule is unaffected. Whether the clause's text should record it is yours.
- **`1391` C4 / §3's "ghost before the press" label** (see QA item 2) is a wording issue in a filed report. Recorded in the recipe, not patched in the report (`qa/` is forbidden to this row).

## Not examined / limitations

- No recipe was executed. No PIE, editor, compile or git write. Every value comes from reading the cited reports, `CONVENTIONS.md` `VER-§13`, and C++ source text (`[D]` lines only).
- I read `1512`, `1511`, `1391` (C1–C8, §3), `1230` (header + row 1), `1270` (row 3 excerpt) and `qa/TASK-1502-loop1.md` at my instant.
  - I did **not** re-read `1314A` or `1348`. Their quotes in the recipe are unchanged, and QA loop 1 traced them.
  - The claim that `1270`'s report "names `ui_snapshot`, `survey_pie_scene` and `capture_pie_frame` calls" its call list does not itemise is QA loop 1's finding (N-L1-2). The recipe cites it as such; I did not re-trace it.
- `[S]` spellings (the `{"type","params"}` envelope, `survey_pie_scene` params, `get_actor_property_in_pie` params) were not checked against a live schema. I hold no schema for those tools.
- The `[D]` gate mechanism was read from `SiegePlayerController.cpp` on disk at my instant, uncompiled; code text is cited, not line numbers. That the pre-press read and the press see the same frame is **not** measured, and the recipe says so.
- `README.md` changed during my run (`TASK-1515`'s). I did not read its new content against my recipes. A cross-file consistency check (for example, its Pending rows versus my "measured-once" labels) is for `TASK-1518`.
- Markdown rendering was not checked. The only structural checks run were heading order and JSON parse.
- The handoff's before/after quotes are generated by `difflib` against a snapshot taken before my first edit. Lines are quoted whole.

## Before/after quotes (every change, whole lines)

#### RCP-play-unit-card-from-hand.md — hunk 1: before L3–3 → after L3–3 (replace)
Before:
~~~~text
Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1391` = `.claude/pipeline/qa/TASK-1391-verify.md` (the play) · `1230` = `qa/TASK-1230-verify.md` · `1270` = `qa/TASK-1270-verify.md` · `1314A` = `qa/TASK-1314-verify.md` **LIMB A** · `1348` = `qa/TASK-1348-verify.md` (the last four: the placement-entry step only).
~~~~
After:
~~~~text
Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1391` = `.claude/pipeline/qa/TASK-1391-verify.md` (the play) · `1230` = `qa/TASK-1230-verify.md` · `1270` = `qa/TASK-1270-verify.md` · `1314A` = `qa/TASK-1314-verify.md` **LIMB A** · `1348` = `qa/TASK-1348-verify.md` (the last four: the placement-entry step only) · `1512` = `qa/TASK-1512-verify.md` (the control-arm finding, the pre-press ghost read, the no-play-interval control, and the composed batch measured once).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 2: before L8–8 → after L8–9 (replace)
Before:
~~~~text
> - **The entry step (Step 3) is not from `1391`**, which never names it. It is seeded from four runs that measured `inject_input_action IA_Card<N>` entering placement. **Nobody has run the entry, the control, the aim and the confirm together in one batch**, so the first use of this recipe measures that composition.
~~~~
After:
~~~~text
> - **The entry step (Step 3) is not from `1391`**, which never names it. It is seeded from four runs: three measured entry (`1314A`, `1348`, `1270`); `1230` measured the call, path and reply on a refused press. **`1512` a2 ran the entry, a confirm and the post-reads in one batch, once, with NO aim set** (Step 3). **Never run:** the entry composed with an aim that placed (SET → dependent read → confirm inside a live placement mode), and the entry composed with an omitted-set press that was refused.
> - ⚠️ **The omitted-set press is a control ONLY when the ghost is hidden immediately before it.** In `1512` the unset cursor already traced to (−449.15, −1.71), a legal point inside a Blue-owned zone, and the press planned as the control placed `BP_Unit_Archer_C_0` `[M: 1512 A3]`. `1391` C4, this arm's source, had the ghost `bHidden: true` at `(0,0,0)`. So: read the ghost immediately before **every** press (Step 4, the pre-press ghost reads), count the omitted-set arm as a control only when that read shows `bHidden: true`, and carry the no-play-interval control (`1512` ctl) in every batch. A batch cannot branch, so the gate is judged after the batch, from its reads.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 3: before L19–18 → after L20–22 (insert)
Before:
~~~~text
(nothing: new text)
~~~~
After:
~~~~text
- **The control-arm finding and the composed batch.** `.claude/pipeline/qa/TASK-1512-verify.md`, `Verdict: VERIFIED`, run **2026-09-26**, attempt 2 (`a2`) unless a line says attempt 1.
  - **A2** the zone box and `CaptureOwner` · **A3** entry → confirm → post-reads in one batch, and its declared deviation (the press planned as the control placed the unit) · **ctl** the no-play-interval control · *Attempt 1* (the dead-hero refusal) · *Hypotheses* H1 · *Not examined* · *Speed data* · *Recipes used* · *Recipe candidates* item 6 (the read paths, the census call, the `wait 0.2`).
  - It used this recipe **outside its aim fence** (a capture zone, not the Blue spawn) and with **no aim set**, and it reported the control failure rather than patching the recipe (`VER-§13` cl. 3).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 4: before L23–23 → after L27–29 (replace)
Before:
~~~~text
- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` B1, M2, N2, N3). Not re-verified since.
~~~~
After:
~~~~text
- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` B1, M2, N2, N3).
- Used in part by `TASK-1512`, 2026-09-26, and re-verified in-run **partly**: "Measured: the entry composed in one batch (`None` → `StaticMeshActor_34` + entry line), and gold/census/log read-backs. **Failed:** the omitted-set control arm is **not** a control when the unset cursor already traces to a legal point." `[M: 1512 Recipes used]`
- Amended by `TASK-1516`, 2026-09-26, on `1512` and on `qa/TASK-1502-loop1.md` (W-L1-1, N-L1-1 … N-L1-5).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 5: before L27–27 → after L33–33 (replace)
Before:
~~~~text
**Not read by any source run.** Neither `1391` nor any of the four Step 3 sources records one. Context only, not this recipe's measurement: `VER-§5` cl. 5 records `Aura.uplugin` `1.0.6` read by `TASK-1390` on 2026-09-22. ⇒ the first use re-verifies in-run (`VER-§13` cl. 3).
~~~~
After:
~~~~text
**Not read by any source run.** Neither `1391`, nor any of the four Step 3 sources, nor `1512` ("Aura plugin version not read", *Not examined*) records one. Context only, not this recipe's measurement: `VER-§5` cl. 5 records `Aura.uplugin` `1.0.6` read by `TASK-1390` on 2026-09-22. ⇒ the first use re-verifies in-run (`VER-§13` cl. 3).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 6: before L44–43 → after L50–51 (insert)
Before:
~~~~text
(nothing: new text)
~~~~
After:
~~~~text
     - That measurement is on a **Spell**: `1230`'s header records its `DT_Cards` read as `Fog: Cost=5000 CardType=Spell`. `[M: 1230 header, row 1]`
     - That the same gate runs before placement for a **Unit** card is `[D]` only. In `ASiegePlayerController::PlayHandSlot`, the block commented `// affordability gate first (§3.5 spec order: the gold refusal outranks the type refusal …)` (`if (!SiegeState->CanAfford(Row->Cost))`, then `return;`) comes before `switch (Row->CardType)`.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 7: before L54–54 → after L62–62 (replace)
Before:
~~~~text
     - a Building enters placement ("placement mode entered for card 'WatchTower' (cost 30)"), but its confirm is outside this recipe's fences. `[M: 1348 P4]`
~~~~
After:
~~~~text
     - a Building enters placement ("placement mode entered for card 'WatchTower' (cost 30)") `[M: 1348 P4]`, but its confirm is outside this recipe's fences. P4 does not state the card's type: that `WatchTower` is a Building comes from `1230`'s header `DT_Cards` read (`WatchTower: Cost=30 CardType=Building`). `[M: 1230 header]`
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 8: before L56–56 → after L64–64 (replace)
Before:
~~~~text
   - A hand dealt from a deck of one Unit card has no type exposure: every slot holds that card.
~~~~
After:
~~~~text
   - A hand dealt from a deck of one Unit card has no type exposure: every slot holds that card. Measured once: deck4 = 50× `Archer` gave `Hand` = 6× `"Archer"` and `DrawPile` = 44× `"Archer"`. `[M: 1512 A1]` ⚠️ The same deck blinds the hand observable, because the hand refills to an identical array (Read-back; Known hazards).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 9: before L62–62 → after L70–70 (replace)
Before:
~~~~text
  - `IA_Card1` ↔ `Hand` index 0 is **measured**. `IA_Card1` acted on `Fog`, the only `Fog` in a hand read as `("Fog","Footman","Archer","Wall","Wall","WatchTower")` just before the press, and the log reads `hand slot 0 ('Fog') refused …`. `[M: 1230 row 1]` This rests on the card name. `1230` files its explanation of the log's `slot 0` under its H2.
~~~~
After:
~~~~text
  - `IA_Card1` ↔ `Hand` index 0 is **measured**. `IA_Card1` acted on `Fog`, the only `Fog` in a hand read as `("Fog","Footman","Archer","Wall","Wall","WatchTower")` at t=01m26.9s. That read was 12.1 s before the press (t=01m39.01s), with no play between, and the hand read the same 0.81 s after the press. The log reads `hand slot 0 ('Fog') refused …`. `[M: 1230 row 1]` This rests on the card name. `1230` also states the mapping in-line in row 1 ("0-based slot 0 = `IA_Card1`"), not only in its H2.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 10: before L93–93 → after L101–101 (replace)
Before:
~~~~text
- **The call and its reply, measured:** `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` → `status: action_injected`, `value_type: Boolean`. `[M: 1230 row 1]` ⚠️ **The reply is not the observable.** It came back the same on a press the game **refused** (the `Fog` row, Precondition 4), so it proves delivery, not entry.
~~~~
After:
~~~~text
- **The call and its reply, measured:** `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` → `status: action_injected`, `value_type: Boolean`. `[M: 1230 row 1]` ⚠️ **The reply is not the observable.** It came back the same on a press the game **refused** (the `Fog` row, Precondition 4), so it is not evidence of entry; that the press arrived was shown in `1230` by the handler's refusal line.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 11: before L102–102 → after L110–112 (replace)
Before:
~~~~text
- **Entry, measured inside a `run_verification_sequence`:** in that session, "`IA_Card1` fired at t=01m07.23s, logged at `:3097 … placement mode entered for card 'Footman' (cost 9).`" `[M: 1270 row 3]` It was a sequence step: `1270`'s full list of its tool calls into PIE has no standalone `inject_input_action` ("Tool calls into PIE: 3 `run_verification_sequence` (4 + 37 + 12 actions), 1 `start_pie`, 1 `stop_pie`, 1 `stop_pie_recording`"). `[M: 1270 metrics]` The report does not say this in words; it follows from that list.
~~~~
After:
~~~~text
- **Entry, measured inside a `run_verification_sequence`:** in that session, "`IA_Card1` fired at t=01m07.23s, logged at `:3097 … placement mode entered for card 'Footman' (cost 9).`" `[M: 1270 row 3]` It was a sequence step. The report does not say so in words; two bases, both readings:
  - **Primary: `1270` row 3's own stamps.** A read of the HUD notice slot at t=01m07.22s, `IA_Card1` at t=01m07.23s, then `IA_DiscardAll` at t=01m07.57s. `[M: 1270 row 3]` A 0.01 s gap cannot contain an MCP round trip: `1512` measured round trips of ≈20 s and ≈36 s of PIE clock (`1512` *Speed data*), and the law's range is ≈30–70 s (`VER-§8` cl. 10(b)).
  - **Secondary: the call list.** `1270`'s list of its tool calls into PIE has no standalone `inject_input_action` ("Tool calls into PIE: 3 `run_verification_sequence` (4 + 37 + 12 actions), 1 `start_pie`, 1 `stop_pie`, 1 `stop_pie_recording`"). `[M: 1270 metrics]` That list may not be exhaustive: the same report names `ui_snapshot`, `survey_pie_scene` and `capture_pie_frame` calls it does not itemise (`qa/TASK-1502-loop1.md` N-L1-2).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 12: before L105–106 → after L115–119 (replace)
Before:
~~~~text
- **`NOT MEASURED`: this entry composed in one batch with `1391`'s control arm, aim and confirm.** `1391` does not name its entry, and `1270` never confirmed.
  - **Position.** The position (after the pre-reads, before the control press) is this recipe's, not a source's. It agrees with `1391` C4's control-arm census `167 → 168`, "the +1 being the ghost `StaticMeshActor_34` itself". The pre-read's time is not quoted, so this is a reading, not a measurement.
~~~~
After:
~~~~text
- **The entry composed in one batch with a confirm and its post-reads: measured ONCE, with NO aim set.** `1391` does not name its entry, and `1270` never confirmed. `1512` a2 ran, in one `run_verification_sequence`: `GhostActor` `None` (t=33.58) → `IA_Card1` (t=33.61) → `GhostActor` `StaticMeshActor_34`, `bHidden false`, `RelativeLocation (X=-449.150572,Y=-1.707544,Z=0)` (t=33.79) → `simulate_key_press LeftMouseButton` (t=33.80) → post-reads. `[M: 1512 A3]`
  - Log: `placement mode entered for card 'Archer' (cost 12).`, then `played card 'Archer' for 12 gold — spawned 'BP_Unit_Archer_C_0' at (-449, -2, 92).` Gold 43 (t=33.79) → 32 (t=34.05); `BP_Unit_Archer_C` census 0 (t=33.59) → 1 (t=34.09); `GhostActor` → `None` after. `[M: 1512 A3]`
  - **Fenced to that run:** vs-bot, reached through the main menu; deck4 = 50× `Archer`; the hero walked to X=250.9 inside a Blue-owned `CaptureZone_Center`. The press that placed was the one **planned as the control**, and no `SetMouseLocation` had run in that session before it (`1512` A3 deviation).
  - **Still `NOT MEASURED`:** the entry composed with an aim that placed (SET → dependent read → confirm inside a live placement mode), and the entry composed with an omitted-set press that was refused. `1512`'s two `SetMouseLocation` steps ran after the play, "found no placement mode and aimed nothing". `[M: 1512 A3]`
  - **Position.** After the pre-reads, before the first press: ran once in `1512` a2 (pre-reads t=33.58–33.59, entry t=33.61). `[M: 1512 A3]` Its agreement with `1391` C4's control-arm census `167 → 168` ("the +1 being the ghost `StaticMeshActor_34` itself") stays a reading: `1391` does not quote the pre-read's time.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 13: before L108–108 → after L121–121 (replace)
Before:
~~~~text
  - **Spacing.** The spacing between the entry and the control press is `NOT MEASURED`. The entry read (Step 4) sits between them.
~~~~
After:
~~~~text
  - **Spacing, measured once:** entry t=33.61 → `wait 0.2` → ghost read t=33.79 → press t=33.80. The ghost was visible and traced when read. `[M: 1512 A3; Recipe candidates item 6 for the wait]` Any other spacing is `NOT MEASURED`.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 14: before L111–111 → after L124–124 (replace)
Before:
~~~~text
**Step 4 — ONE `run_verification_sequence`: hand read, entry, control arm, then treatment arm, with every dependent read inside it.** `[M: 1391 C3, C4, §3]` for the arms. Rows 1–5 are this recipe's composition (Steps 2–3).
~~~~
After:
~~~~text
**Step 4 — ONE `run_verification_sequence`: hand read, entry, a pre-press ghost read before EVERY press, the control arm (a control only if its gate held), the treatment arm, and the no-play-interval control, with every dependent read inside it.** `[M: 1391 C3, C4, §3]` for the arms; `[M: 1512 A3, ctl]` for the gate and the interval control. Rows 1–6 are this recipe's composition (Steps 2–3); rows 3–6, one press and the post-reads ran together once in `1512` a2.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 15: before L117–120 → after L130–134 (replace)
Before:
~~~~text
| 3 | pre-reads: `Hand` again, gold, `GhostActor` (expect `None`), census | `[M: 1391 C2, C4, C5]`; `GhostActor` `None` before entry `[M: 1348 P4]` |
| 4 | **ENTRY**: `inject_input_action IA_Card<N>` (Step 3) | `[M: 1230, 1270, 1314A, 1348]`; the composition is `NOT MEASURED` |
| 5 | entry read: `GhostActor` (expect non-`None`) | `[M: 1348 P4]`; `1391` C4 "`GhostActor` still `StaticMeshActor_34`" at the control |
| control | `simulate_key_press` `LeftMouseButton`, **no set step before it** | `[M: 1391 C4, §3]` |
~~~~
After:
~~~~text
| 3 | pre-reads: `Hand` again, gold, `GhostActor` (expect `None`), census. These are also the "before" half of the no-play-interval control. | `[M: 1391 C2, C4, C5]`; `GhostActor` `None` before entry `[M: 1348 P4, 1512 A3]`; the pre-play half of the interval control `[M: 1512 ctl]` |
| 4 | **ENTRY**: `inject_input_action IA_Card<N>` (Step 3) | `[M: 1230, 1270, 1314A, 1348]`; composed with a confirm and post-reads once `[M: 1512 A3]` |
| 5 | `wait_pie_seconds` 0.2 | `[M: 1512 Recipe candidates item 6]`; A3 stamps entry t=33.61 → read t=33.79 |
| 6 | **PRE-PRESS GHOST READ (mandatory) = the control's GATE**, as the step immediately before the control press: `GhostActor`, `GhostActor.bHidden`, `GhostActor.RootComponent.RelativeLocation`. It is also the entry check (`GhostActor` non-`None`). | paths and values `[M: 1512 A3, Recipe candidates item 6]`; entry `[M: 1348 P4]` |
| control (**conditional**) | `simulate_key_press` `LeftMouseButton`, **no set step before it**. It counts as a control **only if row 6 read `bHidden: true`** (the `1391` C4 condition). If row 6 read `bHidden: false`, this press is an **un-aimed confirm**: in `1512` it placed the unit, and the rows after it found no placement mode. | arm `[M: 1391 C4, §3]`; failure `[M: 1512 A3]` |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 16: before L123–123 → after L137–137 (replace)
Before:
~~~~text
| dependent read | ghost read (source indices: A 13, B 17) | `[M: 1391 C3]` |
~~~~
After:
~~~~text
| **PRE-PRESS GHOST READ (mandatory) = the dependent read** | the row 6 read again, immediately before the confirm (source indices: A 13, B 17). The confirm places wherever the ghost is. | `[M: 1391 C3]`; paths and "the press confirms wherever the ghost is" `[M: 1512 Recipe candidates item 6]`, ghost `(-449.15, -1.71)` → spawn `(-449, -2, 92)` `[M: 1512 A3]` |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 17: before L125–125 → after L139–140 (replace)
Before:
~~~~text
| post-reads | gold, `Hand`, census (source indices: A 16/17/18, B 20/21/22) | `[M: 1391 C3, C5]` |
~~~~
After:
~~~~text
| post-reads | gold, `Hand`, census (source indices: A 16/17/18, B 20/21/22); add `DeckComponent.DrawPile` when the hand can refill to an identical array (Known hazards) | `[M: 1391 C3, C5]`; `DrawPile` in-batch read `[M: 1512 A1]` (t=2.74) |
| **no-play interval** | the same reads (gold, census) again after the last press, with no card key sent; then the log (Step 5) | `[M: 1512 ctl]` |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 18: before L127–127 → after L142–146 (replace)
Before:
~~~~text
The indices are the source's own and will shift with rows 1–5. Measured span of the source batch: A 21 actions, t=82.2466 → 83.1009 s (**0.854 s**); B 25 actions, t=31.1996 → 32.0731 s (**0.874 s**). "No MCP round trip separates any set from its dependent read." `[M: 1391 C3]`
~~~~
After:
~~~~text
**Choosing whether to send the control press at all is the row's call; neither choice was measured as a whole batch.** A batch cannot branch, so row 6 is judged **after** the batch.
- **Send it:** if the gate held, the arm discriminates the aim, as in `1391` C4. If it did not hold, the press confirms un-aimed and the treatment arm is lost, as in `1512` A3.
- **Skip it:** the SET → read → confirm then runs on a live placement mode, and the no-play interval is the only control. That control "discriminates spend/spawn from income" `[M: 1512 ctl]`. It says nothing about the aim (this recipe's reading: in `1512` no aim was set).

The indices are `1391`'s own and will shift with rows 1–6. Measured span of the source batch: A 21 actions, t=82.2466 → 83.1009 s (**0.854 s**); B 25 actions, t=31.1996 → 32.0731 s (**0.874 s**). "No MCP round trip separates any set from its dependent read." `[M: 1391 C3]` `1512` a2's batch, which also carried the menu route and the walk: "ONE `run_verification_sequence`, 48 actions, menu t=20.42 → arena t=37.45". `[M: 1512 Speed data]`
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 19: before L146–147 → after L165–177 (replace)
Before:
~~~~text
  The ghost's `bHidden` and location were read (C4/§3 quote them), but the source quotes the path only as "`GhostActor…`" and names `RelativeLocation` in H1. The exact read paths are `NOT MEASURED`.
- Census: the source reports totals and per-class counts ("census `168 → 169`", "`BP_Unit_Footman_C` count 1", "`Unit_` classes still only `BP_Unit_Pikeman_C ×1`") but does not name the instrument. `NOT MEASURED`: which tool (see Fences).
~~~~
After:
~~~~text
  `1391` read the ghost's `bHidden` and location (C4/§3 quote them), but quotes the path only as "`GhostActor…`" and names `RelativeLocation` in H1.
- **Pre-press ghost read (rows 6 and the dependent read):**
```json
{"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "properties": ["GhostActor", "GhostActor.bHidden", "GhostActor.RootComponent.RelativeLocation"]}}
~~~~
  - The three property paths are quoted from `1512`: "`GhostActor`, `GhostActor.bHidden`, `GhostActor.RootComponent.RelativeLocation` (this dotted path WORKS; it was measured non-null this run)". A3 gives the values: `StaticMeshActor_34`, `bHidden false`, `RelativeLocation (X=-449.150572,Y=-1.707544,Z=0)`. `[M: 1512 Recipe candidates item 6, A3]`
  - `1512` does not quote the params object. Putting the three paths in one read is this recipe's choice; `name` is per Precondition 6; the envelope is `[S]`.
- **Census:** `1391` reports totals and per-class counts ("census `168 → 169`", "`BP_Unit_Footman_C` count 1", "`Unit_` classes still only `BP_Unit_Pikeman_C ×1`") but does not name the instrument. `1512` names it, class-filtered:
```json
{"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}}
~~~~
  - The `include`/`class_filter` values are quoted from `1512` *Recipe candidates* item 6. It read `BP_Unit_Archer_C` 0 (t=33.59) → 1 (t=34.09), with `BP_Unit_Archer0` at `(-382.86, -1.58, 92)`. `[M: 1512 A3]` The envelope is `[S]`.
  - Measured once, with `"Archer"` only. Another card's filter value is this recipe's substitution (`NOT MEASURED`). An unfiltered total like `1391`'s is `NOT MEASURED` from this tool.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 20: before L150–151 → after L180–181 (replace)
Before:
~~~~text
**Step 5 — after the batch, read the log: two lines.** Category `LogGitClaudeUnrealTest`, in `Saved/Logs/GitClaudeUnrealTest.log`.
- **The entry line**, `placement mode entered for card '<CardID>' (cost <n>)`: one per entry, naming the card the key actually entered. `[M: 1314A ceiling 2, 1348 P4, 1270 row 3]`; `[D]` one `UE_LOG` per `EnterPlacementMode`.
~~~~
After:
~~~~text
**Step 5 — after the batch, read the log: three lines.** Category `LogGitClaudeUnrealTest`, in `Saved/Logs/GitClaudeUnrealTest.log`.
- **The entry line**, `placement mode entered for card '<CardID>' (cost <n>)`: one per entry, naming the card the key actually entered. `[M: 1314A ceiling 2, 1348 P4, 1270 row 3, 1512 A3]`; `[D]` one `UE_LOG` per `EnterPlacementMode`.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 21: before L155–154 → after L185–186 (insert)
Before:
~~~~text
(nothing: new text)
~~~~
After:
~~~~text
- **The play line**, substring `played card '`. `1512` a2 logged `played card 'Archer' for 12 gold — spawned 'BP_Unit_Archer_C_0' at (-449, -2, 92).` after its one placing press, and "no second `played card` line" over the no-play interval. `[M: 1512 A3, ctl]` The general form `… played card '%s' for %d gold — spawned '%s' at (%.0f, %.0f, %.0f).` is `[D]` (`SiegePlayerController.cpp`).
  - If a play line follows the **control** press, that press confirmed and the arm is not a control, whatever row 6 read (Read-back).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 22: before L168–168 → after L200–200 (replace)
Before:
~~~~text
The three observables, named in advance, **none of them the absence of a refusal line** (`[M: 1391 C5]`; law `VER-§8` cl. 7 boarding rule (iv)), plus the entry check that decides whether the arms mean anything:
~~~~
After:
~~~~text
The three observables, named in advance, **none of them the absence of a refusal line** (`[M: 1391 C5]`; law `VER-§8` cl. 7 boarding rule (iv)). Two checks come first: the entry check decides whether the arms mean anything, and the gate decides whether the control arm is a control.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 23: before L170–170 → after L202–202 (replace)
Before:
~~~~text
| observable | control arm (set omitted): must NOT move | treatment arm: must move |
~~~~
After:
~~~~text
| observable | control arm (set omitted): must NOT move, **and it is a control only if the gate held** | treatment arm: must move |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 24: before L173–172 → after L205–205 (insert)
Before:
~~~~text
(nothing: new text)
~~~~
After:
~~~~text
| **the gate (checked second): the ghost read immediately before the press** | **`bHidden: true`** (`1391` C4: `bHidden: true` at `(0,0,0)`). If it read `bHidden: false`, **this column is not a control**: report the press as an un-aimed confirm (below) and use the no-play-interval control. | `bHidden: false` at the aim point (see coordinates) |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 25: before L174–174 → after L207–207 (replace)
Before:
~~~~text
| hand (index 1 in the source) | unchanged, both | A `Footman → MilitiaMob`; B `Archer → MilitiaMob` |
~~~~
After:
~~~~text
| hand (index 1 in the source) | unchanged, both | A `Footman → MilitiaMob`; B `Archer → MilitiaMob`. ⚠️ With a one-card deck the hand refills to the identical array and cannot show the play: use `DrawPile` −1 (`1512` A3, Known hazards). |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 26: before L177–177 → after L210–209 (delete)
Before:
~~~~text
| ghost before the press | `bHidden: true` at `(0,0,0)` | `bHidden: false` at the aim point (see coordinates) |
~~~~
After:
~~~~text
(removed)
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 27: before L180–180 → after L212–230 (replace)
Before:
~~~~text
All rows except "entry" are `[M: 1391 C4, C5, C6, §3]`. The entry row is `[M: 1348 P4, 1314A, 1270]` plus this recipe's composition. Pixels corroborate only: both treatment frames show an opaque placed unit, not a translucent ghost. `[M: 1391 Evidence]`
~~~~
After:
~~~~text
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
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 28: before L184–184 → after L234–235 (replace)
Before:
~~~~text
- **The entry composed in ONE batch with the control, aim and confirm.** Each half is measured separately (Step 3 and `1391`). The whole sequence has never run. The first use measures it and reports it as a recipe candidate if it holds.
~~~~
After:
~~~~text
- **The entry composed in ONE batch with an aim that placed, or with a control press that was refused.** Never run. What ran once is entry → pre-press ghost read → un-aimed confirm → post-reads (`1512` a2; Step 3). The first use that sets an aim on a live placement mode measures the rest, and reports it as a recipe candidate if it holds.
- **The gate, beyond one failure.** `bHidden: true` before the press has never been read and then followed by a refused press in the same batch. `1391` C4's `bHidden: true` was read after its press (Read-back). The gate rests on `1391` C4, on `1512` A3's failure case, and on `[D]` (Known hazards).
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 29: before L194–194 → after L245–245 (replace)
Before:
~~~~text
- **One aim point, at the Blue spawn, from the hero's starting camera.** Placing anywhere else, **including inside a capture zone**, is `NOT MEASURED`.
~~~~
After:
~~~~text
- **One aim point, at the Blue spawn, from the hero's starting camera.** Aiming anywhere else with `SetMouseLocation`, **including into a capture zone**, is `NOT MEASURED`.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 30: before L196–196 → after L247–247 (replace)
Before:
~~~~text
  - So a zone placement would at least need the zone Blue-owned. Whether one then confirms was never tested.
~~~~
After:
~~~~text
  - A placement **inside** a Blue-owned zone confirmed once, **with no aim set**. `CaptureOwner` read `Blue` at t=33.54, the ghost sat at the unset cursor's trace (−449.15, −1.71), and the press at t=33.80 spawned `BP_Unit_Archer_C_0` at (−449, −2, 92). `[M: 1512 A2, A3]` That is not an aim point: "an unset cursor is not a reproducible aim", and its mechanism is `HYPOTHESIS` H1 (`1512` *Recipe candidates* fences, H1). A calibrated `SetMouseLocation` into the zone is `NOT MEASURED`. `[M: 1512 Not-examined]`
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 31: before L198–198 → after L249–249 (replace)
Before:
~~~~text
- **The census instrument.** `NOT MEASURED` (the source names none). `survey_pie_scene` with `include: ["census"]` returns per-class counts by its schema `[S]`, but no source run is quoted using it for this census. It is a candidate, not a step.
~~~~
After:
~~~~text
- **The census instrument.** `1391` names none. `survey_pie_scene` with `include: ["census"]` and `class_filter: "Archer"` was used once, by `1512` (Step 4). Other filter values, and an unfiltered total like `1391`'s, are `NOT MEASURED` from it.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 32: before L200–200 → after L251–251 (replace)
Before:
~~~~text
- **Scope of the runs:** `L_Arena` only; standalone single client; `client_index 0`. The one-batch shape (B, as the law reads it) and the two-call alternative (A) only. `1270`'s in-sequence entry was Sandbox (No Bot).
~~~~
After:
~~~~text
- **Scope of the runs:** `L_Arena` only; standalone single client; `client_index 0`. The one-batch shape (B, as the law reads it) and the two-call alternative (A) only. `1270`'s in-sequence entry was Sandbox (No Bot). `1512`'s composed batch was vs-bot, reached through the main menu (`IA_MenuAccept`, not `load_level`), with deck4 = 50× `Archer`, the hero walked to X=250.9, and one run. `[M: 1512 A1, A2]`
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 33: before L212–211 → after L263–264 (insert)
Before:
~~~~text
(nothing: new text)
~~~~
After:
~~~~text
| `1512`'s ghost point with the cursor **unset**: `(X=-449.150572, Y=-1.707544, Z=0)`; spawn `(-449, -2, 92)` | viewport 1280×725, **DPI 0.6706**, `client_index 0`; hero stopped at X=250.89, Y≈0; camera yaw 180; cursor position **not read** | **not an aim**: "an unset cursor is not a reproducible aim"; mechanism `HYPOTHESIS` H1 `[M: 1512 A3, H1, Not-examined, Editor/Aura state]` |
| `CaptureZone_Center` box: centre `RelativeLocation (X=0,Y=0,Z=0)`, `ZoneHalfExtent (X=840, Y=840)` ⇒ X,Y ∈ [−840, 840] | live read, a2 t=2.73 | a world box, not a screen value. ⚠️ Its centre is `(0,0,0)`, the same point `1391` C4 read the hidden ghost at (Known hazards) `[M: 1512 A2]` |
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 34: before L213–213 → after L266–266 (replace)
Before:
~~~~text
The world point under `(300,420)` depends on the camera, i.e. on where the hero stands and looks. Move the hero, change the viewport, or change the camera, and this recipe's aim is unmeasured. The entry step has no coordinate.
~~~~
After:
~~~~text
The world point under `(300,420)` depends on the camera, i.e. on where the hero stands and looks. Move the hero, change the viewport, or change the camera, and this recipe's aim is unmeasured. An **unset** cursor "is not a reproducible aim" (`1512` *Recipe candidates* fences; mechanism `HYPOTHESIS` H1). The entry step has no coordinate.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 35: before L219–219 → after L272–272 (replace)
Before:
~~~~text
  - Judge the arms only **after** checking the entry row of the Read-back.
~~~~
After:
~~~~text
  - Judge the arms only **after** checking the entry row and then the gate row of the Read-back.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 36: before L221–220 → after L274–282 (insert)
Before:
~~~~text
(nothing: new text)
~~~~
After:
~~~~text
- **The omitted-set press confirms whenever the ghost is visible at a legal point.** `1512`: "No `SetMouseLocation` had run in this session before it. The ghost was already visible at an in-zone, legal point (−449, −2), so the "control" press placed the unit." `[M: 1512 A3]` The unset cursor gave `bHidden: true` at the Blue spawn from the start camera (`1391` C4) and a visible in-zone point at X=250.9 (`1512` A3). Which one a run gets is not known before the batch (`1512` H1: "If Jonathan's mouse was over the PIE window, the aim came from his cursor").
- **`(0,0,0)` is not the gate; `bHidden` is.** `1391` C4 read the hidden ghost at `(0,0,0)`, and `(0,0,0)` is also the centre of `CaptureZone_Center` `[M: 1512 A2]`, a legal point whenever that zone is Blue-owned (`1391` C6's refusal text). So a visible ghost at `(0,0,0)` does not meet the C4 condition. The rest of this bullet is `[D]`, from `UpdatePlacementGhost` and `ResolvePlacementUpgradeState` in `SiegePlayerController.cpp`:
  - `GhostActor->SetActorHiddenInGame(!bGroundHit);`, so hidden means no ground hit that frame.
  - `bool bValid = bGroundHit && (IsPointInOwnSpawnBox(Hit.ImpactPoint) || IsPointInCapturedZone(Hit.ImpactPoint));`, so no ground hit means invalid.
  - The upgrade override cannot turn that valid for a Unit card: `if (!bPendingCardIsBuilding || PendingCard.IsNone())` returns `EPlacementUpgradeState::None`.
  - `GhostActor->SetActorLocation(PlacementLocation);` runs only inside `if (bGroundHit)`, so a hidden ghost's location is stale and carries no information.
- **The gate read and the press are different frames** (`[D]`, plus this recipe's reading). The ghost is re-traced every frame (`// per-frame cursor-to-ground trace + ghost position/color (GDD §3.5)`), and the confirm is polled after that, in the same function (`// Confirm: LMB polled while in mode.`). The press is judged on the cursor at its own frame, so a cursor that moves between the read and the press is not caught. Keep the read the step immediately before the press, as `1512` did (read t=33.79, press t=33.80). `[M: 1512 A3]`
- **A one-card deck blinds the hand observable.** "`Hand` is still 6× `Archer` because it refilled, so with an all-Archer deck the hand array cannot show the change by itself." `1512` rested "the card left the hand" on `DrawPile` 44 → 43 and `DiscardPile` = 1. `[M: 1512 A3, Not-examined]` Read `DeckComponent.DrawPile` before and after in such a run.
- **A dead hero cannot play cards.** Past the fence, `1512` attempt 1's `IA_Card1` at t=83.48 left `GhostActor` `None` and logged `PlayHandSlot(0) refused — the hero is dead and the ghost cannot play cards (GHOST-§ G-5).` By log time the hero had died ≈73 s after the arena opened. `[M: 1512 Attempt 1]` That it died to Red units while holding the zone alone is `HYPOTHESIS` H2 ("The killer was not read"). The entry row of the Read-back catches it.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 37: before L225–225 → after L287–287 (replace)
Before:
~~~~text
- **An unaffordable card key is refused before placement**, and gold and hand stay put. `[M: 1230 row 1]` In the primary shape that silently removes the entry, and the entry row of the Read-back catches it.
~~~~
After:
~~~~text
- **An unaffordable card key is refused before placement**, and gold and hand stay put. Measured on a Spell (`Fog`) `[M: 1230 header, row 1]`; for a Unit card, `[D]` only (Precondition 4). In the primary shape that silently removes the entry, and the entry row of the Read-back catches it.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 38: before L229–229 → after L291–291 (replace)
Before:
~~~~text
- **Gold reads can straddle an income tick.** A's `−8` against a cost of 9 is explained as one +1 tick inside a 0.771 s window, which the source labels "an INFERENCE, not a separate measurement" (H2). `[M: 1391 H2]` Always assert "gold went down". Assert "down by exactly the cost" only when the timestamps rule a tick out.
~~~~
After:
~~~~text
- **Gold reads can straddle an income tick.** A's `−8` against a cost of 9 is explained as one +1 tick inside a 0.771 s window, which the source labels "an INFERENCE, not a separate measurement" (H2). `[M: 1391 H2]` `1512` saw the same: 43 → 32 against a logged cost of 12 inside 0.26 s, "which is an inference" there too. `[M: 1512 A3]` Always assert "gold went down". Assert "down by exactly the cost" only when the timestamps rule a tick out.
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 39: before L232–232 → after L294–294 (replace)
Before:
~~~~text
- **Grant declaration:** `pie_scene_edit` is not a standalone tool on the verifier's surface. It was reached **only as a step inside `run_verification_sequence`**, twice, one `call_actor_function` op each, no other op. `[M: 1391 §1, Not-examined]`
~~~~
After:
~~~~text
- **Grant declaration:** `pie_scene_edit` is not a standalone tool on the verifier's surface. It was reached **only as a step inside `run_verification_sequence`**, twice, one `call_actor_function` op each, no other op. `[M: 1391 §1, Not-examined]` `1512` reached it the same way, 4 times, all `SetMouseLocation` on `SiegePlayerController0`, and "**None of them aimed a placement**". `[M: 1512 Not-examined]`
~~~~

#### RCP-play-unit-card-from-hand.md — hunk 40: before L235–235 → after L297–297 (replace)
Before:
~~~~text
- **Environment:** frame A carried a red "Video memory has been exhausted" banner; recorded, not diagnosed. `[M: 1391 Editor/Aura state]`
~~~~
After:
~~~~text
- **Environment:** frame A carried a red "Video memory has been exhausted" banner; recorded, not diagnosed. `[M: 1391 Editor/Aura state]` The same banner appeared in every `1512` frame (1058–1062 MB over in attempt 1, 182 MB in a2). `[M: 1512 Not-examined]`
~~~~

#### RCP-deckbuilder-slot-and-card-edit.md — hunk 1: before L16–16 → after L16–16 (replace)
Before:
~~~~text
  - **What the source says:** the gesture and the tool. "`ui_perform`'s **`double_click` step FIRES `UButton.OnClicked`** … This is how all 100 deck edits were made." (*THE ANSWER FIRST*) §1(c) then reports runs of 49, 10, 30 and 28 gestures at a stated frame spacing. Every call it quotes carried the root `WBP_DeckBuilder`: its own error text reads "No widget in root 'WBP_DeckBuilder' matches…" (§1(b)).
~~~~
After:
~~~~text
  - **What the source says:** the gesture and the tool. "`ui_perform`'s **`double_click` step FIRES `UButton.OnClicked`** … This is how all 100 deck edits were made." (*THE ANSWER FIRST*) §1(c) then reports runs of 49, 10, 30 and 28 gestures at a stated frame spacing. The one call whose error text it quotes carried the root `WBP_DeckBuilder`: "No widget in root 'WBP_DeckBuilder' matches…" (§1(b)).
~~~~

#### RCP-deckbuilder-slot-and-card-edit.md — hunk 2: before L83–84 → after L83–84 (replace)
Before:
~~~~text
- ⚠️ **Which envelope, and why this one. It is NOT the tool the law names.** `VER-§13` cl. 1 and the verifier's S1 say predictable input goes into "ONE `run_verification_sequence`" `[L]`. This step uses **one standalone `ui_perform` call** whose `steps` array carries the whole run. Reasons, none of them a measurement:
  1. `ui_perform`'s `double_click` step is the tool and gesture the source names for every edit `[M: archer50 THE ANSWER FIRST]`, and the root `WBP_DeckBuilder` is the one its calls carried `[M: archer50 §1(b)]`.
~~~~
After:
~~~~text
- ⚠️ **Which envelope, and why this one.** `VER-§13` cl. 1 and the verifier's S1 say predictable input goes into "ONE `run_verification_sequence`" `[L]`. Cl. 1's ruling of 2026-09-26 (the manager, on `TASK-1513`), quoted by text in italics (emphasis marks dropped): *"ONE `run_verification_sequence`" MEANS ONE TOOL CALL FOR THE BATCH, not one per gesture. `run_verification_sequence` is the default envelope, and it is REQUIRED whenever a batch mixes tools, which includes every cl. 2 group (a set, its confirm, and the reads that depend on it). A batch of one tool's gestures only may use that tool's own steps array (e.g. `ui_perform` with `steps`), provided the recipe or the report names the envelope and says why.* `[L: VER-§13 cl. 1, 2026-09-26 ruling]` This step is that case, and this paragraph names the envelope: **one standalone `ui_perform` call** whose `steps` array carries the whole run, every step in it a `ui_perform` step (`double_click`, `wait`; spellings `[S]`). Reasons, none of them a measurement:
  1. `ui_perform`'s `double_click` step is the tool and gesture the source names for every edit `[M: archer50 THE ANSWER FIRST]`, and the one call whose error text the source quotes carried the root `WBP_DeckBuilder` `[M: archer50 §1(b)]`.
~~~~

#### RCP-deckbuilder-slot-and-card-edit.md — hunk 3: before L88–88 → after L88–88 (replace)
Before:
~~~~text
  **`NOT MEASURED`: both envelopes, for this batch.** The source does not say which one it used (see Source). The `wait` spelling is `[S]`. Whether cl. 1's "ONE `run_verification_sequence`" means "one tool call" is a question for the manager (`qa/TASK-1505.md` M7). If the ruling picks the runner, only this step's envelope changes; the steps inside it stay the same.
~~~~
After:
~~~~text
  **`NOT MEASURED`: both envelopes, for this batch.** The source does not say which one it used (see Source). The `wait` spelling is `[S]`. The question this step used to leave open (`qa/TASK-1505.md` M7: does cl. 1's "ONE `run_verification_sequence`" mean "one tool call"?) is answered by the ruling above. The ruling **permits** this envelope; it does not **measure** it. Under either envelope the read-back and top-up are owed (Step 5; the same ruling: "Either way the read-back and top-up above are owed").
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 1: before L3–3 → after L3–3 (replace)
Before:
~~~~text
Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`.
~~~~
After:
~~~~text
Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` · `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md`.
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 2: before L9–9 → after L9–12 (replace)
Before:
~~~~text
- Seeded by `TASK-1502`, 2026-09-26. Not re-verified since.
~~~~
After:
~~~~text
- Seeded by `TASK-1502`, 2026-09-26.
- **Re-verified in-run by `TASK-1511`**, 2026-09-26, `Verdict: VERIFIED`, 1 PIE session. `1511` *Recipes used*, verbatim: "Steps 0–4 (focus read `Button_0` t=4.89; `IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62; `IA_MenuAccept` t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14) · re-verified in-run **y** (first use; the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections)." Also used: its *Speed data* (sent vs landed) and its `Editor/Aura state:` paragraph.
- **Used in part by `TASK-1512`**, 2026-09-26: "Step 0 only (focus read of `Button_0` "Play (vs Bot)"); the recipe's `IA_MenuDown`×2 was not used." `[M: 1512 Recipes used]`
- Amended by `TASK-1516`, 2026-09-26, to record the two runs above.
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 3: before L13–13 → after L16–16 (replace)
Before:
~~~~text
**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." ⇒ the first use of this recipe re-verifies it in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).
~~~~
After:
~~~~text
**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." `1511` and `1512` did not read it either ("Aura plugin version not read", each *Not examined*). ⇒ the first use re-verified the recipe in-run (`1511`). The version is still unread, so a plugin update since then cannot be ruled out (`VER-§13` cl. 3, `VER-§8` cl. 5).
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 4: before L20–20 → after L23–23 (replace)
Before:
~~~~text
4. **Starting focus on `Button_0` "Play (vs Bot)".** The source read it at t=4.84 s. The step count below (`IA_MenuDown` ×2) is only correct from that starting focus, so read it before sending anything (Step 0). `[M: archer50 §1(a)]`
~~~~
After:
~~~~text
4. **Starting focus on `Button_0` "Play (vs Bot)".** The source read it at t=4.84 s. The step count below (`IA_MenuDown` ×2) is only correct from that starting focus, so read it before sending anything (Step 0). `[M: archer50 §1(a)]` Both later runs found the same starting focus: `1511` at t=4.89, `1512` at menu t=20.42. `[M: 1511 Recipes used; 1512 A1]`
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 5: before L30–30 → after L33–39 (replace)
Before:
~~~~text
Tool: `ui_snapshot`. Source: "At t=4.84 s `Button_0` "Play (vs Bot)" was focused." `[M: archer50 §1(a)]` The source does **not** name the tool that returned the focus flag (`NOT MEASURED`: the reader's name). `ui_snapshot` returns a per-node `focused` field `[S]`, and the same run used `ui_snapshot` on this map (*Not examined*: `ui_snapshot "WBP_MainMenu"`).
~~~~
After:
~~~~text
Tool: `ui_snapshot`. Source: "At t=4.84 s `Button_0` "Play (vs Bot)" was focused." `[M: archer50 §1(a)]` The source does **not** name the tool that returned the focus flag. `ui_snapshot` returns a per-node `focused` field `[S]`, and the same run used `ui_snapshot` on this map (*Not examined*: `ui_snapshot "WBP_MainMenu"`).
- **Re-verified:** `1511` read `Button_0` focused at t=4.89 and does not name the reader. `[M: 1511 Recipes used]`
- **The reader, named:** `1512` read the same focus with `ui_snapshot` and a name selector, which returned `focused: true` and the text "Play (vs Bot)" (menu t=20.42): `[M: 1512 A1; Recipe candidates item 1]`
```json
{"widget": "WBP_MainMenu", "selector": {"by": "name", "value": "Button_0"}, "max_depth": 2}
~~~~
  The object is quoted from `1512` *Recipe candidates* item 1. There it ran as a `run_verification_sequence` step; its step envelope is not quoted.
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 6: before L41–41 → after L50–54 (replace)
Before:
~~~~text
- The envelope (one `run_verification_sequence`) is `[L: VER-§13 cl. 1]` ("a known key sequence, a menu walk — goes into ONE `run_verification_sequence`"). The source does not say whether its two injections were one call or two; they landed 0.42 s apart. `NOT MEASURED`: this exact batched form. If the sequence validator rejects it, send the two as standalone `inject_input_action` calls (the per-action tool §1(a) names) and say so in the report.
~~~~
After:
~~~~text
- The envelope (one `run_verification_sequence`) is `[L: VER-§13 cl. 1]` ("a known key sequence, a menu walk — goes into ONE `run_verification_sequence`"). `archer50` does not say whether its two injections were one call or two; they landed 0.42 s apart.
- **The batched form: measured ONCE, by `1511`.** "`IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62", and "the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections". `[M: 1511 Recipes used]` Sent vs landed: "menu `IA_MenuDown`×2 → 2 landed (focus `Button_2`, 2 subsystem log lines)". `[M: 1511 Speed data]`
  - **Fenced to that run:** `L_MainMenu`, standalone, 1 client, viewport 1280×725, DPI 0.6706, starting focus `Button_0`. `[M: 1511 Editor/Aura state, Recipes used]`
  - `1511` does not quote its action list. Whether the 3 frames came from a wait step or from the runner's own spacing is `NOT MEASURED`, and the JSON above carries no wait step.
  - If a later validator rejects the batched form, send the two as standalone `inject_input_action` calls (the per-action tool `archer50` §1(a) names) and say so in the report.
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 7: before L43–43 → after L56–56 (replace)
Before:
~~~~text
**Step 2 — read the focus before accepting.** Same call as Step 0; expect `Button_2` `focused: true`. `[M: archer50 §1(a)]` The source read focus (t=17.38) before it sent Accept (t=21.50). Sending Accept in the same batch as the Downs is `NOT MEASURED`, and a batch cannot branch on its own result (`README.md`, common hazards).
~~~~
After:
~~~~text
**Step 2 — read the focus before accepting.** Same call as Step 0; expect `Button_2` `focused: true`. `[M: archer50 §1(a)]` The source read focus (t=17.38) before it sent Accept (t=21.50). `1511` read `Button_2` focused at t=9.62 and sent Accept at t=14.60 `[M: 1511 Recipes used]`; whether that read shared the Downs' batch is not stated. Sending Accept in the same batch as the Downs is `NOT MEASURED`, and a batch cannot branch on its own result (`README.md`, common hazards).
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 8: before L49–49 → after L62–62 (replace)
Before:
~~~~text
Tool: `inject_input_action`. Source: "`IA_MenuAccept` (t=21.50) opened the builder". `[M: archer50 §1(a)]` Action path `[D]` (`Content/Input/Actions/IA_MenuAccept.uasset`); parameter name `[S]`.
~~~~
After:
~~~~text
Tool: `inject_input_action`. Source: "`IA_MenuAccept` (t=21.50) opened the builder". `[M: archer50 §1(a)]` Re-verified: "`IA_MenuAccept` t=14.60 → `WBP_DeckBuilder_C_0`"; sent vs landed "`IA_MenuAccept` 1/1 (builder open)". `[M: 1511 Recipes used, Speed data]` Action path `[D]` (`Content/Input/Actions/IA_MenuAccept.uasset`); parameter name `[S]`.
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 9: before L55–55 → after L68–68 (replace)
Before:
~~~~text
Tool: `get_widget_property_in_pie` `[S]`. Source: "at t=22.53 it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`". The live widget instance is `WBP_DeckBuilder_C_0`. `[M: archer50 §1(a) + A1 row "(live widget read)"]` The source calls this a "live widget read" and does not name the tool (`NOT MEASURED`: the reader's name).
~~~~
After:
~~~~text
Tool: `get_widget_property_in_pie` `[S]`. Source: "at t=22.53 it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`". The live widget instance is `WBP_DeckBuilder_C_0`. `[M: archer50 §1(a) + A1 row "(live widget read)"]` Re-verified: "`WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14". `[M: 1511 Recipes used]` Neither run names the tool: the source calls this a "live widget read" (`NOT MEASURED`: the reader's name).
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 10: before L61–63 → after L74–76 (replace)
Before:
~~~~text
| 0 | `focused: true` on `Button_0` | t=4.84 s `[M: archer50 §1(a)]` |
| 1–2 | `focused: true` on `Button_2` "Deck Builder" | t=17.38 s `[M: archer50 §1(a)]` |
| 3–4 | `WBP_DeckBuilder_C_0` exists; `EditingDeckIndex = 0`; `WorkingDeck` = `deck1` | t=22.53 s `[M: archer50 §1(a)]` |
~~~~
After:
~~~~text
| 0 | `focused: true` on `Button_0` | t=4.84 s `[M: archer50 §1(a)]` · t=4.89 `[M: 1511 Recipes used]` · menu t=20.42 `[M: 1512 A1]` |
| 1–2 | `focused: true` on `Button_2` "Deck Builder" | t=17.38 s `[M: archer50 §1(a)]` · t=9.62, after the batched Downs `[M: 1511 Recipes used]` |
| 3–4 | `WBP_DeckBuilder_C_0` exists; `EditingDeckIndex = 0`; `WorkingDeck` = `deck1` | t=22.53 s `[M: archer50 §1(a)]` · t=15.14 `[M: 1511 Recipes used]` |
~~~~

#### RCP-menu-to-deckbuilder.md — hunk 11: before L70–70 → after L83–84 (replace)
Before:
~~~~text
- Any other menu target: "Play (vs Bot)" by `IA_MenuAccept` is `TASK-1512`'s to measure (`README.md`, Pending). Sandbox, Multiplayer, Settings, Login and Quit were not driven.
~~~~
After:
~~~~text
- Any other menu target. "Play (vs Bot)" by `IA_MenuAccept` from `Button_0` was measured by `1512` (A1: `IA_MenuAccept` at menu t=20.44, then world `L_Arena`; 1/1 in both attempts, *Speed data*). Promoting it to a recipe is `TASK-1515`'s row, not this one. Sandbox, Multiplayer, Settings, Login and Quit were not driven.
- **The batched Downs beyond one run.** `1511`'s single landing (2/2) is the only measurement. Its gap between the two injections is quoted as "3 frames" and nothing more (Step 1).
~~~~

