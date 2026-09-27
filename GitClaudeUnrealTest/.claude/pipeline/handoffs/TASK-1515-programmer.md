# TASK-1515 — programmer handoff — [RECIPE-PROMOTION-NEW]

Row: `TASKBOARD.md` `#### TASK-1515` (marker `TASK-1515-RECIPE-PROMOTION-NEW`). Gate: `TASK-1518`. Host: `TASK-1520`. No runtime criterion ⇒ no 5a/5b.

## What was done

- **Two new recipes**, seeded only from `qa/TASK-1511-verify.md` and `qa/TASK-1512-verify.md` plus the law their rows cite:
  - `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md`, from `TASK-1511`'s `## Recipe candidates` (steps 1–5, Fences), rows 0/A1/A2/A3, *Speed data*, *Pixel note*, *Save hygiene*.
  - `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md`, from `TASK-1512`'s `## Recipe candidates` (steps 1–6, Fences), rows A1/A2/A3/ctl, *Attempt 1*, *Recording*, *Hypotheses*, *Not examined*.
- **`Tools/Verify/recipes/README.md`**, targeted edits only:
  - the two new table rows;
  - the menu and play rows' "last verified" cells, which record `TASK-1511`'s re-verification and `TASK-1512`'s partial use;
  - the Pending list rewritten per spec (3);
  - the W-L1-1 count fixed at README:4.
  - Four consequential lines are listed under "Beyond the literal spec" below.
- Nothing else. The three existing `RCP-*.md` files are `TASK-1516`'s and were not opened for writing. Their byte sizes changed during my session; that was `TASK-1516` working. No editor, no PIE, no compile, no git.

## Files written

| file | state | bytes | lines | CR / LF | sha256 |
|---|---|---|---|---|---|
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | NEW | 24012 | 178 | 0 / 178 | `a85fa9db44632280cd7c36821056ba25238bc9d5fb0cf2771c0a0d6037595fba` |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | NEW | 28223 | 239 | 0 / 239 | `8f76fd3d505361851c70bb44b06ad4ebc237709678aa4eebd591434eff96366c` |
| `Tools/Verify/recipes/README.md` | edited | 12511 | 96 | 0 / 96 | `24c77b1caafbadf091f9ce428aa3960ac62842da90ce7943e520ed23d25f8b81` |
| `.claude/pipeline/handoffs/TASK-1515-programmer.md` | NEW | — | — | — | (this file) |
| `.claude/pipeline/TASKBOARD.md` | this row's `status:` line only | — | — | — | — |

- README before my first edit: sha256 `fa5cc2d021bd4ca365115b49d026017fae5370cf01f1fa4c3878dd289aaa2d25`, 79 lines, 8582 B. That is `qa/TASK-1502-loop1.md`'s own anchor, so nobody had touched it between that gate and me.
- CR/LF were counted as bytes, not with a regex (`VER-§12` cl. 7d).

**Mechanical checks (measured by me, Python `rb`):**
- Both new files carry exactly the eight `## ` headings, in the README's order: Source · Plugin version · Preconditions · Steps · Read-back · Fences — not measured for · Coordinate/resolution-dependent values · Known hazards.
- Every ```json block parses: set-active 3/3, vs-bot 2/2.
- The vs-bot "in-batch tail" was a bare fragment in my first draft. It is now a JSON array, so it parses.

## README — the table and the Pending list, before and after (acceptance)

### BEFORE (verbatim, from the pristine copy)

```
| [`RCP-menu-to-deckbuilder.md`](RCP-menu-to-deckbuilder.md) | … | `qa/PLAYTEST-archer50-verify.md` §1(a) | 2026-09-26 (source run; not re-verified since) |
| [`RCP-deckbuilder-slot-and-card-edit.md`](RCP-deckbuilder-slot-and-card-edit.md) | … | `qa/PLAYTEST-archer50-verify.md` §1(b), §1(c), A1; exit step §1(e) | 2026-09-26 (source run; not re-verified since) |
| [`RCP-play-unit-card-from-hand.md`](RCP-play-unit-card-from-hand.md) | Play an `ECardType::Unit` card from hand on `L_Arena` in ONE batch that fits the `t≈60 s` fence: an in-batch hand read (documents the slot, does not select it), `IA_Card<N>` entry, an omitted-set control, then aim set + confirm + dependent reads. **The entry, the control, the aim and the confirm have never run together in one batch, so the first use measures it.** The two-call shape is kept as an alternative that does not fit the fence. | … | 2026-09-22 (play source run; entry sources 2026-09-14…2026-09-20; not re-verified since) |

"Last verified" is the date of the source run. No recipe here has been re-run since it was seeded.

## Pending recipes — sequences not yet measured, and the rows that will measure them
| Set the active deck by keyboard (reach a deck slot with injected menu actions, then `IA_MenuSecondary`) | The route does not exist yet: it is being built by `TASK-1507`/`TASK-1508` (their status is on the board, not here). Today set-active is right-click only, and `ui_perform` has no right mouse button (`VER-§8` cl. 12; `archer50` §1(d)). | `TASK-1511` (5b for `TASK-1507`; its spec asks for the measured A1 sequence as a recipe candidate) |
| vs-bot match on the active deck → `CaptureZone_Center` read → summon inside it | Never reached: `archer50` A2–A4 are `NOT REACHED` (blocked at set-active). | `TASK-1512` |
| The whole play batch as composed in `RCP-play-unit-card-from-hand.md`: in-batch hand read → `IA_Card<N>` entry → omitted-set control → aim + confirm | Each half is measured on its own. The entry (`inject_input_action IA_Card<N>`) comes from `1230`/`1270`/`1314A`/`1348`, and the control + aim + confirm from `1391`. `1391` never names its entry, and no run has sent them together. `IA_Card2`…`IA_Card6` ↔ hand index is `[D]` only. | `TASK-1512` (its A3 uses the play recipe; its report should record the composed batch as a recipe candidate, with the key it used) |

Not pending, because they are ceilings rather than unmeasured sequences: right-click on any widget (`VER-§8` cl. 12); `click` and `press`/`release` on a `UButton` (named dead ends, `VER-§5` cl. 5).
```
(`…` = cells I did not change, elided here only. The README itself carries them in full.)

### AFTER (verbatim)

```
| [`RCP-menu-to-deckbuilder.md`](RCP-menu-to-deckbuilder.md) | … | `qa/PLAYTEST-archer50-verify.md` §1(a) | 2026-09-26 (source run). **Re-verified in-run 2026-09-26 by `TASK-1511`**: Steps 0–4, and the batched `IA_MenuDown` ×2 landed 2/2 (`qa/TASK-1511-verify.md` *Recipes used*, *Speed data*). `TASK-1512` used Step 0 only. Recording that in the recipe itself is `TASK-1516`'s (its row, (3)). |
| [`RCP-deckbuilder-slot-and-card-edit.md`](RCP-deckbuilder-slot-and-card-edit.md) | … (unchanged) | … | 2026-09-26 (source run; not re-verified since) |
| [`RCP-play-unit-card-from-hand.md`](RCP-play-unit-card-from-hand.md) | Play an `ECardType::Unit` card from hand on `L_Arena` in ONE batch that fits the `t≈60 s` fence: an in-batch hand read (documents the slot, does not select it), `IA_Card<N>` entry, an omitted-set control, then aim set + confirm + dependent reads. The two-call shape is kept as an alternative that does not fit the fence. ⚠️ **What `TASK-1512` measured with it:** the entry, the confirm and the post-reads ran together in one batch once, with **no aim set** (`qa/TASK-1512-verify.md` A3). The planned omitted-set control **placed the unit**, because the ghost was already visible at a legal point. The aim has still never run in one batch with the entry. The amendment to the recipe itself is `TASK-1516`'s (its row, (1)(a)–(b)). | … | 2026-09-22 (play source run; entry sources 2026-09-14…2026-09-20). **Used in part 2026-09-26 by `TASK-1512`**, outside its aim fence; it re-verified the recipe "partly" (`qa/TASK-1512-verify.md` *Recipes used*). |
| [`RCP-deckbuilder-set-active-by-keyboard.md`](RCP-deckbuilder-set-active-by-keyboard.md) | Make a deck the ACTIVE deck from a freshly opened Deck Builder, with injected actions only. The sequence: `IA_MenuDown` to arm the grid; then `IA_MenuBack` + `IA_MenuRight` × T in one batch, with a `DeckBar` snapshot per step; then `IA_MenuSecondary` + the `OutlineBorder.BrushColor` reads in one batch; then a disk read. It drives door 3 only: not the real `Home` key and not gamepad Y. **It changes the player's real save.** | `qa/TASK-1511-verify.md` rows 0, A1–A3; *Recipe candidates*; *Speed data*; *Pixel note*; *Save hygiene* | 2026-09-26 (source run; not re-verified since) |
| [`RCP-vsbot-capture-center-and-summon.md`](RCP-vsbot-capture-center-and-summon.md) | From `L_MainMenu`, all in ONE `run_verification_sequence`: "Play (vs Bot)", walk the hero into `CaptureZone_Center`, capture it, then `IA_Card1` + confirm inside the zone. An in-batch `record_burst` after the level travel films the arena; the control is the no-play interval. **There is no aim step: the one measured summon placed where an unset cursor traced (H1).** | `qa/TASK-1512-verify.md` A1–A3, ctl; *Attempt 1*; *Recording*; *Recipe candidates* | 2026-09-26 (source run; not re-verified since) |

"Last verified" is the date of the source run, plus any in-run use that a later report records under its *Recipes used*. As of 2026-09-26:
- `RCP-menu-to-deckbuilder.md` was re-verified in-run by `TASK-1511`;
- `RCP-play-unit-card-from-hand.md` was used in part by `TASK-1512` (see the table);
- the other three have not been re-run since they were seeded.

## Pending recipes — sequences not yet measured, and the rows that will measure them
| A calibrated `SetMouseLocation` aim into a capture zone, i.e. a placement point inside the zone set on purpose, not left to the cursor | `TASK-1512`'s one in-zone summon was placed where an unset cursor traced (`qa/TASK-1512-verify.md` H1, "MECHANISM NOT MEASURED"). Its two `SetMouseLocation` calls ran after the placement had exited and aimed nothing (*Not examined*). The play recipe's own aim, `SetMouseLocation(300,420)` from `1391`, is measured only at the Blue spawn from the start camera. That aim has also never run in one batch with an `IA_Card<N>` entry: `TASK-1512` sent the entry and the confirm with no aim set. | no measuring row boarded |
| The right mouse button through `simulate_key_press` (`RMB` / `RightMouseButton`) | `VER-§8` cl. 12 measured the right button absent through `ui_perform` only. `simulate_key_press`'s schema lists an `RMB` shorthand, which arms a re-measure and proves nothing (cl. 12's 2026-09-26 scope note). The prior, not a verdict: a negative is expected. | `TASK-1519` |

**Moved out 2026-09-26 (`TASK-1515`), and where each went:**
- *Set the active deck by keyboard* → `RCP-deckbuilder-set-active-by-keyboard.md`, from `TASK-1511` (`VERIFIED`).
- *vs-bot match on the active deck → `CaptureZone_Center` read → summon inside it* → `RCP-vsbot-capture-center-and-summon.md`, from `TASK-1512` (`VERIFIED`).
- *The whole play batch as composed in `RCP-play-unit-card-from-hand.md`* → `TASK-1512` a2 ran the entry, the confirm and the post-reads in one batch once, with no aim and outside that recipe's aim fence. The recipe's labels are `TASK-1516`'s to amend.
  - Its remaining gap, the aim composed with the entry, is folded into the first pending row above.
  - Its other open point, `IA_Card2`…`IA_Card6` ↔ hand index (`[D]` only), stays a fence of that recipe.

Not pending, because they are ceilings rather than unmeasured sequences: the right mouse button through `ui_perform` (`VER-§8` cl. 12; the `simulate_key_press` form is the pending row above); `click` and `press`/`release` on a `UButton` (named dead ends, `VER-§5` cl. 5).
```

### README:4 (W-L1-1), before → after

- **Before:** "On QA loop 1 the same day, the play recipe's placement-entry step was seeded from four further runs that measured it (`qa/TASK-1505.md` M2)."
- **After:** "…was seeded from four further runs (`qa/TASK-1505.md` M2):", followed by two bullets:
  - "three measured entry: `1314A`, `1348` and `1270`;"
  - "`1230` measured the call, path and reply on a refused press."

  Then: "The count was corrected per `qa/TASK-1502-loop1.md` W-L1-1." This is QA's own fix text, split into two bullets.

### Beyond the literal spec: four README lines, each changed to keep the file true (flag for QA)

1. **The paragraph under the table.** "No recipe here has been re-run since it was seeded" became false the moment `TASK-1511` re-verified the menu recipe. It is replaced by the per-recipe statement quoted above. Spec (3) asks me to record that re-verification, and this is the line that denied it.
2. **Run short names** (the line under the provenance table): it adds `1511`, `1512` and `1509`, which the new recipes use in their tags. It labels `1509` "a pre-compile code review, cited for code reads only and never as a runtime measurement".
3. **The common hazard "Plugin version was not read in either seed run"** now names all four source runs. Each report's *Not examined* says it was not read.
4. **The Recording section** gains one paragraph pointing at `VER-§12` cl. 7b's 2026-09-26 amendment (arm the recorder after a level travel). Law, with `TASK-1512`'s H3 labelled `HYPOTHESIS`.

Also inside the Pending section: the "Not pending" line's "right-click on any widget" is narrowed to "the right mouse button through `ui_perform`". `VER-§8` cl. 12's 2026-09-26 scope note orders exactly this: "write '`ui_perform` has no right mouse button', not 'this lane has none'". The spec's new pending row (`TASK-1519`) would otherwise contradict the old line.

### How I read spec (3)'s Pending instruction (flag for QA)

The spec says "move out the two entries now measured" and then names the complete residual: the in-zone aim and the right mouse button. The old row 3 (the composed play batch) was partly measured by `TASK-1512` a2 (entry + confirm + post-reads, no aim). `TASK-1513` (3) calls it "the composed batch now measured once", and `TASK-1516` (1)(b) moves it to measured-once in the recipe. So I moved all three old rows out, and listed each with its destination.

Row 3 still had two open points, and I kept both instead of dropping them:
- the aim composed with the entry, folded into the aim row's "why" cell;
- `IA_Card2..6`, left as a fence of the play recipe, which already carries it.

If QA or the manager reads "two entries" as rows 1–2 only, the fix is to restore row 3 in its narrowed form. No recipe changes.

## QA map: every step → the sentence that measured it

### `RCP-deckbuilder-set-active-by-keyboard.md` (`1511` = `qa/TASK-1511-verify.md`)

| recipe step | source sentence(s) | notes |
|---|---|---|
| Pre 2, bind line | row 0: "`[00.12.06:863] UDeckBuilderWidget::BindMenuNavActions: 7 IA_Menu* action(s) bound (Started) on 'PlayerController_0' […]; every IA_Menu* asset resolved.`" | verbatim |
| Pre 3, menu recipe re-verified | *Recipes used*: "Steps 0–4 (focus read `Button_0` t=4.89; `IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62; `IA_MenuAccept` t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14)" | verbatim |
| Pre 4, start slot | candidates step 2: "expect `DeckSlotEntryWidget_<EditingDeckIndex>/OutlineBorder/SlotButton focused:true`" | only 0 measured, stated |
| Pre 5, no mouse | `VER-§8` cl. 12 amendment: "Keep mouse input out of the builder before the walk (`qa/TASK-1509.md` N4)"; `1511` *Not examined*: "no mouse input was sent at all" | `[L]` + `[M]` |
| Pre 6, slot → deck map | A1 / A3 relay lines ('deck1'…'deck5') | slots 5–9 `NOT MEASURED` |
| Step 0, disk read | *Pre-registration*: `load_game_from_slot("SiegeDecks_4E46…E34", 0)`, class `SiegeDeckSaveGame`, the values | lane named from `1512` *Save hygiene* |
| Step 1 | candidates step 1: "`inject_input_action /Game/Input/Actions/IA_MenuDown.IA_MenuDown` → read `FocusedCardIndex == 0` (t=56.67)"; A1: "`IA_MenuDown` t=56.36 → `FocusedCardIndex=0`" | the 0.3 s wait is labelled "not quoted for this step"; the reader's name is `NOT MEASURED` |
| Steps 2–3 | candidates steps 2–3 (quoted in full in the recipe); A1 stamps 65.69 / 66.03 / 66.36 / 66.69; "Steps 2–3 ran as ONE `run_verification_sequence` (4/4 landed)" | the wait after `IA_MenuBack` is labelled "not quoted" |
| Step 4 | candidates step 4 (quoted); A1: before t=80.80, press t=80.87, after t=81.40, log `[00.13.13:066] … active deck set to 'deck4'` | the before-reads sharing the batch is labelled a reading from the stamps |
| Step 5 | A1: "Live disk read in PIE (after the press): `ActiveDeckName=deck4`. Disk after PIE stopped: `ActiveDeckName=deck4`." + *Save hygiene* | verbatim |
| C1 / C2 | A2 rows (cold open t=45.29/45.70; tile focused t=56.71/57.12–57.14) + *Speed data* | verbatim |
| C3 | A3 (Right t=110.31, Secondary t=110.64, refusal Warning quoted, reads t=111.16–111.19) + *Speed data* "2/2" | verbatim |
| zero-with-control | A2 log census: 9 lines, 0 in the window 00.12.06–00.12.58 | verbatim |
| Fences | candidates Fences line + *Not examined* + A4/A5 + *Hypotheses* | each tagged |
| frames | *Pixel note* (1086 vs 1280); `VER-§12` cl. 7e | `max_dim` spelling `[S]`; inside a sequence `NOT MEASURED` |

### `RCP-vsbot-capture-center-and-summon.md` (`1512` = `qa/TASK-1512-verify.md`)

| recipe step | source sentence(s) | notes |
|---|---|---|
| one batch, and why | *Speed data*: "attempt 2 = ONE `run_verification_sequence`, 48 actions … Attempt 1 used 3 calls, and its two round trips (≈20 s, ≈36 s of PIE clock) put the play at t=83 s, after the hero's death"; `VER-§13` cl. 1 ruling (cites the same) | verbatim |
| rows 1–3 | candidates steps 1–2; A1: "`ui_snapshot` at menu t=20.42 → `Button_0` `focused: true` … `IA_MenuAccept` injected at menu t=20.44, then world `L_Arena`" | verbatim |
| row 4, `record_burst` | *Recording*: "auto-started by an in-batch `record_burst` at arena t=2.69 s"; candidates step 2 `record_burst {"seconds":1}` | + `VER-§12` cl. 7b amendment |
| row 5 | candidates step 3; A1 (`Hand` 6×, `DrawPile` 44×, t=2.74); A2 (box, t=2.73) | verbatim |
| rows 6–9, the walk | candidates steps 4–5 (hold 29 / 28.5, `x 0, y -1`, waits 14 + 15.5 + 1.2); A2 stamps (−21007.8 @2.73, −10710.2 v750 @16.79, 250.89 stopped t≤32.31, `Blue` @33.54) | the transform reader is `NOT MEASURED` (`get_player_transform` `[S]`); the `CurrentHP` reader is `NOT MEASURED` |
| rows 10–15 | candidates step 6 (quoted); A3 (entry t=33.61, ghost t=33.79 with location, LMB t=33.80, gold 43→32, census 0→1, log lines) | no wait after the press is written, because none is quoted |
| Step 2, ghost-in-zone judgement | candidates step 6: "check that the ghost location is inside the zone BEFORE the press, because the press confirms wherever the ghost is"; A2 box | `[D]` `IsPointInZone` quoted by its text; judged after the batch because a batch cannot branch |
| Step 3, control | `ctl` row (quoted); A3 deviation; candidates Fences "use the no-play interval control instead" | the split between in-batch reads and the later call is stated from the stamps |
| Step 4, later call | A3 (Team/CardID t=54.55; `DrawPile`/`DiscardPile` t≈55); *Not examined* (editor Python once) | an in-batch post-play `DrawPile` read is `NOT MEASURED` |
| Step 5, log | *Editor/Aura state* (`StartMatch` line); A1; A3; `ctl` | verbatim |
| Step 6 | *Recording*; *Not examined* (`stop_pie_recording` replies overflowed); *Save hygiene* | + `VER-§12` cl. 7b amendment |
| Fences / hazards | candidates Fences line; *Attempt 1*; H1/H2/H3; *Not examined*; *Evidence* | each tagged |

### `[D]` reads I made (repository, not runtime)

- `DeckBuilderWidget.cpp` `HandleMenuNavBack`: `RouteMenuNavKey(EKeys::Gamepad_FaceButton_Right, TEXT("IA_MenuBack"))`. That is the key on `DECK-§9`'s "Exit the grid" row, and it is the reason Step 1 cannot be skipped.
- `CaptureZone.cpp` `ACaptureZone::IsPointInZone` (the XY box about `GetActorLocation()`, Z ignored).
- `CaptureZone.h` `CaptureEvalInterval = 0.5f`, the Blue/Red/contested/empty rule, and `CanTeamSpawnHere`.
- `Content/Input/Actions/IA_Sprint.uasset`, `IA_Move.uasset` and `IA_Card1.uasset` are the only assets of those names.

### `[S]` spellings

Taken from the tool schemas loaded in this session on 2026-09-26: `run_verification_sequence`, `inject_input_action` (`hold_seconds`, "non-blocking"), `ui_snapshot`, `get_widget_property_in_pie`, `capture_pie_frame` (`max_dim`), `record_burst`, `simulate_key_press`, `get_actor_property_in_pie`, `survey_pie_scene`, `wait_pie_seconds`, `get_player_transform`. I read the schemas and called none of these tools.

## What QA should scrutinise

1. **The vs-bot recipe's `get_player_transform` rows (7–8).** `1512` measured transform reads at those positions ("`wait_pie_seconds` 14 + 15.5 with transform reads between") but did not name the reader. I wrote `get_player_transform`, tagged `[S]`, with the reader's name `NOT MEASURED`. That is the pattern `RCP-menu-to-deckbuilder.md` Steps 0/4 used and that passed QA. If you rule that a step whose tool is `[S]` has no measuring sentence, the fix is to drop the two objects and keep the prose. The walk does not depend on them.
2. **Set-active Step 4's before-reads in the same batch** rest on stamps (0.07 s to the press), not on words. They are labelled so.
3. **The "the ghost read is judged after the batch" wording.** The report's candidate says "check … BEFORE the press". A batch cannot branch, so I kept the read before the press and moved the judgement after the batch, and said so in the read-first box, in Step 2 and in the hazards. Check that this does not contradict the report. My reading is that it says what the report measured: the read was taken before the press, and nothing gated the press.
4. **The set-active recipe's three controls (C1–C3)** are included as optional steps. The spec's step list does not name them, but each is a measured A2/A3 row. Check that nothing in them goes past A2/A3.
5. **The Pending-list interpretation** (above).
6. **Timing words:** "≈17 s" is arithmetic from the stamps (t=37.45 → 54.55), and says so. The a1 spawn match (−21471 + 463) is arithmetic, and says so.

## Assets and paths referenced (not created)

`/Game/Input/Actions/IA_MenuDown|IA_MenuBack|IA_MenuRight|IA_MenuSecondary|IA_MenuAccept|IA_Card1` (quoted by the reports) · `/Game/Input/Actions/IA_Sprint.IA_Sprint`, `/Game/Input/Actions/IA_Move.IA_Move` (`[D]`) · `WBP_DeckBuilder`, `DeckBar`, `DeckSlotEntryWidget_<i>`, `OutlineBorder.BrushColor`, `WBP_MainMenu`, `Button_0`, `CaptureZone_Center`, `SiegePlayerController0`, `DeckComponent` (runtime names from the reports).

## Not examined / limitations

- **No recipe was executed.** No PIE, no editor call, no compile, no git. Every recipe is a claim until its first use re-verifies it (`VER-§13` cl. 3). Neither source read the plugin version.
- **Tool schemas were read, not exercised.** `max_dim` inside a sequence, and whether `get_player_transform` returns velocity in this plugin build, are `NOT MEASURED`. The schema lists `velocity`.
- **I viewed no frame or film** from `1511` or `1512`. The pixel statements are the reports' own words.
- **Illegal-target cases not measured at runtime:** a set-active on deck1 (51) or deck2 (80) was never driven. The recipe says the refusal of those rests on the gate's text, not on a press. `qa/TASK-1270-verify.md` row 2 carries the right-click refusal by a state test, not by runtime.
- **`TASK-1516`'s edits to the three existing recipes were not read or re-traced.** They were in flight during my session. My README cells describe what `TASK-1516`'s row assigns it ("its row, (3)", "(1)(a)–(b)"), not what it wrote. QA should check the README's play/menu cells against `TASK-1516`'s final text.
- **The rotation of the zone box was not read.** `IsPointInZone` ignores rotation (`[D]`). The report's `RootComponent.RelativeLocation` equals the actor location only for an unattached root, which is `NOT MEASURED`, and the recipe says so.
- Markdown rendering was not checked.
