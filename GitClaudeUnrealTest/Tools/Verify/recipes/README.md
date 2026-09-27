# Verifier recipe library — `Tools/Verify/recipes/`

Law: `CONVENTIONS.md` `VER-§13` (cl. 3 is this library). Paths: the artefacts table at the top of `CONVENTIONS.md`.
Seeded 2026-09-26 by `TASK-1502` (gameplay-programmer) from two measured runs and nothing else. On QA loop 1 the same day, the play recipe's placement-entry step was seeded from four further runs (`qa/TASK-1505.md` M2):
- three measured entry: `1314A`, `1348` and `1270`;
- `1230` measured the call, path and reply on a refused press.

The count was corrected per `qa/TASK-1502-loop1.md` W-L1-1. Two recipes were added the same day by `TASK-1515`, each from one measured run: `qa/TASK-1511-verify.md` and `qa/TASK-1512-verify.md`.

## What a recipe is

- A **proven step sequence** that `playtest-verifier` **loads instead of re-deriving** (`VER-§13` cl. 3).
- **Seeded only from a measured run**, cited by report path + section. Every step in a recipe points at the sentence in its source report that measured it.
- **A claim, not a fact (`SC-§101`).** On its first use after an Aura plugin update (the `VER-§8` cl. 5 trigger) or after an edit to the screen it drives, the run that uses it **re-verifies it in-run**. A recipe that fails is **reported, never patched in flight**.
- Written by `gameplay-programmer` rows and QA-gated. The verifier never writes here; it proposes new sequences as `## Recipe candidates` in its own report, and the manager boards the promotion (`VER-§13` cl. 3).

## What a recipe is not

- Not a verdict and not evidence. A run that follows a recipe still reads back every step it depends on (`VER-§13` cl. 1) and still reports what it observed, with its own timestamps.
- Not wider than its source (`SC-§130`). A recipe's `Fences` section is part of the recipe. Using it outside those fences is a new measurement, not a use of the recipe.
- Not a grant. A recipe that reaches a tool through `run_verification_sequence`'s step vocabulary does not change any `tools:` line or permission (`VER-§7` cl. 2, `VER-§8` cl. 7).
- Not a hypothesis store. A hypothesis in the source report stays labelled `HYPOTHESIS` in the recipe; where the source is silent, the recipe says `NOT MEASURED`.

## Recipe format — eight headings, in this order, in every recipe

1. `## Source` — report path + section(s) + run date.
2. `## Plugin version` — as read that run, or "not read".
3. `## Preconditions`
4. `## Steps` — exact tool + parameters, copy-paste ready, each citing its source section.
5. `## Read-back` — the state read that proves each step landed.
6. `## Fences — not measured for`
7. `## Coordinate/resolution-dependent values` — every `abs_*` target and screen coordinate, with the viewport and DPI it was measured at.
8. `## Known hazards`

### Provenance tags (used on every line that carries a value)

| tag | meaning |
|---|---|
| `[M: <run> <section>]` | **Measured** in the cited report; the value or behaviour is quoted from it. |
| `[S]` | **Parameter spelling** taken from the tool's JSON schema as loaded on 2026-09-26 by the authoring row. The source report did not quote the spelling. Not a measurement. |
| `[D]` | Read from the repository on disk at authoring (a `Content/` asset path, a C++ member). Not a measurement of runtime behaviour. |
| `[L: <clause>]` | Required by law; not measured by the source run. |
| `NOT MEASURED` | The source is silent. Do not treat the gap as either a yes or a no. |
| `HYPOTHESIS` | Labelled a hypothesis in the source report; stays one here. |

Run short names used in tags: `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`; `1391` = `.claude/pipeline/qa/TASK-1391-verify.md`. For the play recipe's entry step only: `1230` = `qa/TASK-1230-verify.md`; `1270` = `qa/TASK-1270-verify.md`; `1314A` = `qa/TASK-1314-verify.md` LIMB A; `1348` = `qa/TASK-1348-verify.md`. For the two recipes added by `TASK-1515`: `1511` = `qa/TASK-1511-verify.md`; `1512` = `qa/TASK-1512-verify.md`; `1509` = `qa/TASK-1509.md`. `1509` is a pre-compile code review, cited for code reads only and never as a runtime measurement. For the menu and set-active recipes' 2026-09-26 amendments (`TASK-1527`): `1524` = `qa/TASK-1524-verify.md`.

## Recipes

| file | what it does | source report + section | last verified |
|---|---|---|---|
| [`RCP-menu-to-deckbuilder.md`](RCP-menu-to-deckbuilder.md) | `L_MainMenu` → Deck Builder with injected menu actions (`IA_MenuDown` ×2, a focus read with a one-`IA_MenuDown` top-up if it shows `Button_1`, `IA_MenuAccept` on `Button_2`), with the focus and builder reads that confirm each step. The builder opens on the ACTIVE deck's slot (`DECK-§3`); the recipe reads it | `qa/PLAYTEST-archer50-verify.md` §1(a); the top-up and the open slot: `qa/TASK-1524-verify.md` *Recipes used*, *Not examined*, *Recipe candidates* 2–3 | 2026-09-26 (source run). **Re-verified in-run 2026-09-26 by `TASK-1511`**: Steps 0–4, and the batched `IA_MenuDown` ×2 landed 2/2 (`qa/TASK-1511-verify.md` *Recipes used*, *Speed data*). `TASK-1512` used Step 0 only. `TASK-1516` recorded both in the recipe (its row, (3)). **Used again 2026-09-26 by `TASK-1524`**, "re-verified in-run **y, with a mismatch**": the batched `IA_MenuDown` ×2 landed 1 of 2, and Step 4 read `EditingDeckIndex = 2`, not `0` (`qa/TASK-1524-verify.md` *Recipes used*). `TASK-1527` recorded both in the recipe. |
| [`RCP-deckbuilder-slot-and-card-edit.md`](RCP-deckbuilder-slot-and-card-edit.md) | Select a deck slot for editing and add/remove cards with `ui_perform` `double_click`, batched, with the read-back / top-up loop; optional exit to the main menu | `qa/PLAYTEST-archer50-verify.md` §1(b), §1(c), A1; exit step §1(e) | 2026-09-26 (source run; not re-verified since) |
| [`RCP-play-unit-card-from-hand.md`](RCP-play-unit-card-from-hand.md) | Play an `ECardType::Unit` card from hand on `L_Arena` in ONE batch that fits the `t≈60 s` fence: an in-batch hand read (documents the slot, does not select it), `IA_Card<N>` entry, then a mandatory ghost read immediately before every press. That pre-press read is the gate: the omitted-set press counts as a control only if it shows `bHidden: true`. Then aim set + pre-press ghost read + confirm + dependent reads. The no-play-interval control rides every batch. The two-call shape is kept as an alternative that does not fit the fence. ⚠️ **What `TASK-1512` measured with it:** the entry, the confirm and the post-reads ran together in one batch once, with **no aim set** (`qa/TASK-1512-verify.md` A3). The planned omitted-set control **placed the unit**, because the ghost was already visible at a legal point; that is why the gate exists. The aim has still never run in one batch with the entry. `TASK-1516` amended the recipe for this (its row, (1)(a)–(b)). | `qa/TASK-1391-verify.md` C2–C6, §3, §4; entry step: `qa/TASK-1230-verify.md` row 1, `qa/TASK-1270-verify.md` row 3, `qa/TASK-1314-verify.md` LIMB A ceiling 2, `qa/TASK-1348-verify.md` P4/P10; the gate, the no-play-interval control and the one composed batch: `qa/TASK-1512-verify.md` A2, A3, ctl | 2026-09-22 (play source run; entry sources 2026-09-14…2026-09-20). **Used in part 2026-09-26 by `TASK-1512`**, outside its aim fence; it re-verified the recipe "partly" (`qa/TASK-1512-verify.md` *Recipes used*). |
| [`RCP-deckbuilder-set-active-by-keyboard.md`](RCP-deckbuilder-set-active-by-keyboard.md) | Make a deck the ACTIVE deck from a freshly opened Deck Builder, with injected actions only. The sequence: `IA_MenuDown` to arm the grid; then `IA_MenuBack` + one `IA_MenuRight` per slot in one batch, counted from the slot the builder opened on (`EditingDeckIndex`, the active deck's slot, read, never assumed), with a `DeckBar` snapshot per step; then `IA_MenuSecondary` + the `OutlineBorder.BrushColor` reads in one batch; then a disk read. A variation walks back with `IA_MenuLeft` to restore the previous active deck. It drives door 3 only: not the real `Home` key and not gamepad Y. **It changes the player's real save.** | `qa/TASK-1511-verify.md` rows 0, A1–A3; *Recipe candidates*; *Speed data*; *Pixel note*; *Save hygiene*; the open slot and the `IA_MenuLeft` variation: `qa/TASK-1524-verify.md` row 0, A1, *Recipes used*, *Recipe candidates* 1, 3 | 2026-09-26 (source run). **Used 2026-09-26 by `TASK-1524`**, "re-verified in-run **y, with a mismatch**": Precondition 4 read `EditingDeckIndex` 2, not 0 (`qa/TASK-1524-verify.md` *Recipes used*). `TASK-1527` recorded it and added the `IA_MenuLeft` variation from the same run. |
| [`RCP-vsbot-capture-center-and-summon.md`](RCP-vsbot-capture-center-and-summon.md) | From `L_MainMenu`, all in ONE `run_verification_sequence`: "Play (vs Bot)", walk the hero into `CaptureZone_Center`, capture it, then `IA_Card1` + confirm inside the zone. An in-batch `record_burst` after the level travel films the arena; the control is the no-play interval. **There is no aim step: the one measured summon placed where an unset cursor traced (H1).** | `qa/TASK-1512-verify.md` A1–A3, ctl; *Attempt 1*; *Recording*; *Recipe candidates* | 2026-09-26 (source run; not re-verified since) |

"Last verified" is the date of the source run, plus any in-run use that a later report records under its *Recipes used*. As of 2026-09-26:
- `RCP-menu-to-deckbuilder.md` was re-verified in-run by `TASK-1511`, and used again by `TASK-1524` with a mismatch (see the table);
- `RCP-play-unit-card-from-hand.md` was used in part by `TASK-1512` (see the table);
- `RCP-deckbuilder-set-active-by-keyboard.md` was used by `TASK-1524` with a mismatch (see the table);
- the other two have not been re-run since they were seeded.

## Pending recipes — sequences not yet measured, and the rows that will measure them

| sequence | why it is not a recipe yet | the row that will measure it |
|---|---|---|
| A calibrated `SetMouseLocation` aim into a capture zone, i.e. a placement point inside the zone set on purpose, not left to the cursor | `TASK-1512`'s one in-zone summon was placed where an unset cursor traced (`qa/TASK-1512-verify.md` H1, "MECHANISM NOT MEASURED"). Its two `SetMouseLocation` calls ran after the placement had exited and aimed nothing (*Not examined*). The play recipe's own aim, `SetMouseLocation(300,420)` from `1391`, is measured only at the Blue spawn from the start camera. That aim has also never run in one batch with an `IA_Card<N>` entry: `TASK-1512` sent the entry and the confirm with no aim set. | no measuring row boarded |
| The right mouse button through `simulate_key_press` (`RMB` / `RightMouseButton`) | `VER-§8` cl. 12 measured the right button absent through `ui_perform` only. `simulate_key_press`'s schema lists an `RMB` shorthand, which arms a re-measure and proves nothing (cl. 12's 2026-09-26 scope note). The prior, not a verdict: a negative is expected. | `TASK-1519` |

**Moved out 2026-09-26 (`TASK-1515`), and where each went:**
- *Set the active deck by keyboard* → `RCP-deckbuilder-set-active-by-keyboard.md`, from `TASK-1511` (`VERIFIED`).
- *vs-bot match on the active deck → `CaptureZone_Center` read → summon inside it* → `RCP-vsbot-capture-center-and-summon.md`, from `TASK-1512` (`VERIFIED`).
- *The whole play batch as composed in `RCP-play-unit-card-from-hand.md`* → `TASK-1512` a2 ran the entry, the confirm and the post-reads in one batch once, with no aim and outside that recipe's aim fence. `TASK-1516` amended the recipe's labels.
  - Its remaining gap, the aim composed with the entry, is folded into the first pending row above.
  - Its other open point, `IA_Card2`…`IA_Card6` ↔ hand index (`[D]` only), stays a fence of that recipe.

Not pending, because they are ceilings rather than unmeasured sequences: the right mouse button through `ui_perform` (`VER-§8` cl. 12; the `simulate_key_press` form is the pending row above); `click` and `press`/`release` on a `UButton` (named dead ends, `VER-§5` cl. 5).

## Hazards common to every recipe

- **`start_pie`'s reply can exceed the tool-output limit.** Measured at 264,336 and 504,242 characters (`1391` Not-examined; `VER-§12` cl. 6). Read the head only; confirm the session with `is_pie_active` or the sequence's own `pie_time_seconds` stamps; rest no finding on the reply's contents.
- **`binding_found` is never the observable** (`VER-§8` cl. 10). In `1391` it read `true` identically on the arm that placed and the arm that refused; in `archer50` it read `false` for a `Tab` that reached nothing. Likewise `ui_perform`'s `handled` trace fields corroborate at most.
- **A batch cannot branch on its own result** (`1391` §4). Anything a later step depends on choosing (which card, which slot) must be known before the batch is sent. Otherwise the batch must tolerate whatever it meets and document it by read, which is the play recipe's primary shape (`VER-§8` cl. 10(b), 2026-09-22). A second call to learn it costs a round trip: budget ≈70 s of PIE clock (same clause). Values below ≈30 s have been measured, so ≈30 s is not the cheapest a round trip can be; for the observed values, read the clause's 2026-09-26 amendment, marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`.
- **Plugin version was not read in any source run**: not in `archer50` or `1391` (the first two seeds), and not in `1511`, `1512` or `1524` (each report's *Not examined*: "Aura plugin version not read"). ⇒ the first use of every recipe after this date re-verifies in-run.

## Recording and the remux line (`VER-§12` cl. 7b)

The PIE recorder writes a **raw `.h264` elementary stream, not an `.mp4`** (`archer50` Recording paragraph: `capture_mode: nvenc`, `container: h264`; no MP4 appeared). The verifier holds no shell, so **the orchestrator remuxes**, in the run's `Saved/AuraVerify/<run>/` folder:

```
ffmpeg -framerate 30 -i recording.h264 -c copy recording.mp4
```

The `.mp4` stays in `Saved/` and is never staged. `recording_index.json` beside it maps frame ↔ game time ↔ video time. A report never claims an `.mp4` path it did not see.

**A film armed before `start_pie` may not survive a level travel** (`VER-§12` cl. 7b, 2026-09-26 amendment; `HYPOTHESIS` H3 in `qa/TASK-1512-verify.md`). A run whose acceptance happens after a travel arms a recorder AFTER the travel, and names every film it finds by path. The measured route is an in-batch `record_burst`; see `RCP-vsbot-capture-center-and-summon.md`.
