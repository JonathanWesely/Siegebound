# Verifier recipe library — `Tools/Verify/recipes/`

Law: `CONVENTIONS.md` `VER-§13` (cl. 3 is this library). Paths: the artefacts table at the top of `CONVENTIONS.md`.
Seeded 2026-09-26 by `TASK-1502` (gameplay-programmer) from two measured runs and nothing else. On QA loop 1 the same day, the play recipe's placement-entry step was seeded from four further runs that measured it (`qa/TASK-1505.md` M2).

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

Run short names used in tags: `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`; `1391` = `.claude/pipeline/qa/TASK-1391-verify.md`. For the play recipe's entry step only: `1230` = `qa/TASK-1230-verify.md`; `1270` = `qa/TASK-1270-verify.md`; `1314A` = `qa/TASK-1314-verify.md` LIMB A; `1348` = `qa/TASK-1348-verify.md`.

## Recipes

| file | what it does | source report + section | last verified |
|---|---|---|---|
| [`RCP-menu-to-deckbuilder.md`](RCP-menu-to-deckbuilder.md) | `L_MainMenu` → Deck Builder with injected menu actions (`IA_MenuDown` ×2, `IA_MenuAccept` on `Button_2`), with the focus and builder reads that confirm each step | `qa/PLAYTEST-archer50-verify.md` §1(a) | 2026-09-26 (source run; not re-verified since) |
| [`RCP-deckbuilder-slot-and-card-edit.md`](RCP-deckbuilder-slot-and-card-edit.md) | Select a deck slot for editing and add/remove cards with `ui_perform` `double_click`, batched, with the read-back / top-up loop; optional exit to the main menu | `qa/PLAYTEST-archer50-verify.md` §1(b), §1(c), A1; exit step §1(e) | 2026-09-26 (source run; not re-verified since) |
| [`RCP-play-unit-card-from-hand.md`](RCP-play-unit-card-from-hand.md) | Play an `ECardType::Unit` card from hand on `L_Arena` in ONE batch that fits the `t≈60 s` fence: an in-batch hand read (documents the slot, does not select it), `IA_Card<N>` entry, an omitted-set control, then aim set + confirm + dependent reads. **The entry, the control, the aim and the confirm have never run together in one batch, so the first use measures it.** The two-call shape is kept as an alternative that does not fit the fence. | `qa/TASK-1391-verify.md` C2–C6, §3, §4; entry step: `qa/TASK-1230-verify.md` row 1, `qa/TASK-1270-verify.md` row 3, `qa/TASK-1314-verify.md` LIMB A ceiling 2, `qa/TASK-1348-verify.md` P4/P10 | 2026-09-22 (play source run; entry sources 2026-09-14…2026-09-20; not re-verified since) |

"Last verified" is the date of the source run. No recipe here has been re-run since it was seeded.

## Pending recipes — sequences not yet measured, and the rows that will measure them

| sequence | why it is not a recipe yet | the row that will measure it |
|---|---|---|
| Set the active deck by keyboard (reach a deck slot with injected menu actions, then `IA_MenuSecondary`) | The route does not exist yet: it is being built by `TASK-1507`/`TASK-1508` (their status is on the board, not here). Today set-active is right-click only, and `ui_perform` has no right mouse button (`VER-§8` cl. 12; `archer50` §1(d)). | `TASK-1511` (5b for `TASK-1507`; its spec asks for the measured A1 sequence as a recipe candidate) |
| vs-bot match on the active deck → `CaptureZone_Center` read → summon inside it | Never reached: `archer50` A2–A4 are `NOT REACHED` (blocked at set-active). | `TASK-1512` |
| The whole play batch as composed in `RCP-play-unit-card-from-hand.md`: in-batch hand read → `IA_Card<N>` entry → omitted-set control → aim + confirm | Each half is measured on its own. The entry (`inject_input_action IA_Card<N>`) comes from `1230`/`1270`/`1314A`/`1348`, and the control + aim + confirm from `1391`. `1391` never names its entry, and no run has sent them together. `IA_Card2`…`IA_Card6` ↔ hand index is `[D]` only. | `TASK-1512` (its A3 uses the play recipe; its report should record the composed batch as a recipe candidate, with the key it used) |

Not pending, because they are ceilings rather than unmeasured sequences: right-click on any widget (`VER-§8` cl. 12); `click` and `press`/`release` on a `UButton` (named dead ends, `VER-§5` cl. 5).

## Hazards common to every recipe

- **`start_pie`'s reply can exceed the tool-output limit.** Measured at 264,336 and 504,242 characters (`1391` Not-examined; `VER-§12` cl. 6). Read the head only; confirm the session with `is_pie_active` or the sequence's own `pie_time_seconds` stamps; rest no finding on the reply's contents.
- **`binding_found` is never the observable** (`VER-§8` cl. 10). In `1391` it read `true` identically on the arm that placed and the arm that refused; in `archer50` it read `false` for a `Tab` that reached nothing. Likewise `ui_perform`'s `handled` trace fields corroborate at most.
- **A batch cannot branch on its own result** (`1391` §4). Anything a later step depends on choosing (which card, which slot) must be known before the batch is sent. Otherwise the batch must tolerate whatever it meets and document it by read, which is the play recipe's primary shape (`VER-§8` cl. 10(b), 2026-09-22). A second call to learn it costs a round trip: ≈30–70 s of PIE clock, budget at the upper end (same clause).
- **Plugin version was not read in either seed run** ⇒ the first use of every recipe after this date re-verifies in-run.

## Recording and the remux line (`VER-§12` cl. 7b)

The PIE recorder writes a **raw `.h264` elementary stream, not an `.mp4`** (`archer50` Recording paragraph: `capture_mode: nvenc`, `container: h264`; no MP4 appeared). The verifier holds no shell, so **the orchestrator remuxes**, in the run's `Saved/AuraVerify/<run>/` folder:

```
ffmpeg -framerate 30 -i recording.h264 -c copy recording.mp4
```

The `.mp4` stays in `Saved/` and is never staged. `recording_index.json` beside it maps frame ↔ game time ↔ video time. A report never claims an `.mp4` path it did not see.
