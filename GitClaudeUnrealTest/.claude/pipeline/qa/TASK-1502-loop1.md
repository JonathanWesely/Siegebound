PASS
# QA Report — TASK-1502 re-gate, QA loop 1 (recipe library)

Verdict: **PASS**: 0 BLOCKER · 1 WARN · 6 NIT. Board: `TASK-1502` → `qa-passed` (loop 1 of 3 closes here).

Marker `TASK-1502-QA-LOOP-1-PASSED`. Prior gate: `qa/TASK-1505.md` §1 (B1 · W1 · N1–N4 · M2).

**Why a separate file, not an amendment:** no `CONVENTIONS.md` clause fixes where a re-gate is written, and precedent goes both ways (`qa/TASK-430-gate-loop2.md` is a separate file; `qa/TASK-1483.md` appended a loop). `qa/TASK-1505.md` is the report of a different row (`TASK-1505`, `done`), covers three rows, and carries byte anchors a host relies on. A separate file leaves it byte-stable. I followed the dispatch's path.

**What I did, beyond reading text:** four read-only inspector Python queries (files opened `rb`; `hashlib`, `json`, `os.stat` only; no writes, no `subprocess`). I measured file hashes and mtimes, parsed every ```json block, checked the heading sets, and extracted long `CONVENTIONS.md` / report lines verbatim. I held no Bash and no git, and ran no PIE and no recipe. Everything else is a text-level verdict.

---

## §1 — Prior findings: all resolved

| prior | ruling | how checked |
|---|---|---|
| **B1** (budget + standalone hand read) | **RESOLVED (option (b), with (a)'s fence text inside it)** | `t≈80` is gone (string search: 0 hits for `≈80`, `t=80`, `80 s` in every recipe). Precondition 3 quotes `VER-§8` cl. 10(b) **verbatim, ⛔ marks dropped**, against `CONVENTIONS.md` L12359 (2026-09-20: "EVERY OBSERVABLE LANDS BEFORE `t≈60 s`", the `t=82.6 s`/`135.6 s` bound, "ONLY SAFE READING … LOWER BOUND") and L12361 (2026-09-22: "BETWEEN `≈30 s` AND `≈70 s` … BUDGET AT THE UPPER END … DOES NOT FIT … LUCK, NOT A BUDGET"). The primary shape is now one batch with the hand read inside it, which documents and does not select (L12362, quoted at Step 2). Attempt A is kept only as a labelled "does NOT fit" alternative, with its ≈70.11 s cost from 1391 §4. 1391 §4's "half the `t≈60 s` fence" and "so any card in the hand would be affordable" are both verbatim. |
| **W1** (envelope ≠ the law's tool) | **RESOLVED. The labelling is honest, and it is sufficient.** | archer50 names the tool and gesture ("… This is how all 100 deck edits were made.", THE ANSWER FIRST, verbatim), but never the envelope. So `NOT MEASURED` on **both** envelopes is the correct label, not a hedge. Step 4 now says, flagged ⚠️, that it is not cl. 1's tool, and it gives three reasons, each labelled as a non-measurement: (1) is verbatim; (2) is `[S]`, which I cannot check because I do not hold the schema; (3) `VER-§7` cl. 2 (i) is confirmed at L12259 ("ALL THREE WERE ACCEPTED"), and the root-`widget` detail is still unquoted there, as the step says. The sighting quote "ran one gesture per tool call (100 edits)" is verbatim at the `VER-§13` preamble, L12452. **M7 stays with the manager**, and nothing more is owed by this row. |
| **N1** | resolved | README:58: "their status is on the board, not here". |
| **N2** | resolved | The census suggestion now sits in Fences (play:198) as "a candidate, not a step". |
| **N3** | resolved | play:45 is re-tagged `[L: VER-§8 cl. 10(c)]`, and the validator text is verbatim with L12363. |
| **N4** | resolved | Deck-builder recipe lines 61 and 89 each carry a substitute-the-placeholder note. |

**Mechanical checks (measured):** every `RCP-*.md` has exactly the eight `## ` headings, in order. Every ```json block parses: play 5/5, deck-builder 7/7, menu 4/4. Line counts match the handoff's table: 235, 164, 79 and 86.

## §2 — The M2 seed (Step 3): every sentence traced

| recipe claim | source sentence, read at my instant | result |
|---|---|---|
| "Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*." `[M: 1314A ceiling 2]` | `qa/TASK-1314-verify.md` L276. It sits under *CEILINGS MEASURED THIS RUN* item 2, **inside LIMB A** (the LIMB A heading is at L163, dated 2026-09-19). | verbatim ✓ |
| "the card name there is incidental … struck … for exactly this reason" | `CONVENTIONS.md` L12298–12299: "The card name in the struck quote was INCIDENTAL". The **example** was struck. "ENTERING PLACEMENT WORKS" (L12296) stands. | ✓ |
| "Entered: `GhostActor` `"None"` → `StaticMeshActor_34`; log: … 'WatchTower' (cost 30)" `[M: 1348 P4]` | `qa/TASK-1348-verify.md` L37 | verbatim ✓ |
| "`IA_Card3`/`IA_Card6` entered placement"; "`IA_Card1` played an instant spell" `[M: 1348 P10]` | L71 | verbatim ✓ |
| "Across three sessions slot 1 held `BrightSun`, then `Footman`, then `WatchTower`." `[M: 1348 two-gaps 2]` | L87 | verbatim ✓ |
| `inject_input_action /Game/Input/Actions/IA_Card1.IA_Card1` → `status: action_injected`, `value_type: Boolean`; refusal `hand slot 0 ('Fog') refused — cost 5000, gold 88.` `[M: 1230 row 1]` | `qa/TASK-1230-verify.md` L7 | verbatim ✓ |
| "`IA_Card1` fired at t=01m07.23s, logged at `:3097 … placement mode entered for card 'Footman' (cost 9).`" `[M: 1270 row 3]` | `qa/TASK-1270-verify.md` L9 | verbatim ✓ |
| "Tool calls into PIE: 3 `run_verification_sequence` (4 + 37 + 12 actions), 1 `start_pie`, 1 `stop_pie`, 1 `stop_pie_recording`" `[M: 1270 metrics]` | L37 | verbatim ✓. The recipe labels the in-sequence conclusion as its own reading on its face (play:102, "does not say this in words"). **Correctly labelled.** See N-L1-2 for a stronger basis. |
| `[D]` the `IA_Card2..IA_Card6 …` comment · the soft paths · the six `.uasset`s · `SpawnPlacementGhost()` then the entry `UE_LOG` | `SiegePlayerController.h:1695` (verbatim) · `SiegePlayerController.cpp:222–227` · `Content/Input/Actions/IA_Card1..6.uasset` (Glob: 6) · `.cpp:2084` then `:2087–2089` | ✓ |

**Is `action_injected` kept out of the proof of entry? Yes.** Three places handle it:
- Step 3 (play:93) and the hazard at play:226 both say the reply is not the observable, and cite the refused `Fog` press.
- The Read-back's new **entry row** (play:172) rests entry on `GhostActor` `None` → non-`None` **plus** one entry log line naming a clean Unit card, with "If either fails, neither arm is evidence."
- That row's citation list (play:180) correctly **excludes** `1230`.

The one slip is the header count (W-L1-1). There is also a wording over-read (N-L1-1).

**"Only `IA_Card1` has a measured key↔index mapping": the claim holds, and it holds more widely than the recipe needs.**
- I grepped every file under `qa/` for `IA_Card[2-6]` and got four hits:
  - `TASK-1348` P10 names `IA_Card3`/`IA_Card6` entering placement but ties neither to a hand index.
  - `TASK-023`, `TASK-030` and `TASK-810` are code/test QA reviews, not runtime measurements.
- The `IA_Card1` ↔ index 0 claim is sound. The only `Fog` in the hand read sits at index 0. The engine's own line says `hand slot 0 ('Fog')`. `1230` row 1 also states it in-line ("0-based slot 0 = `IA_Card1`"), not only in H2 (N-L1-5).
- The claim is also **not load-bearing** in the primary shape, and the recipe says so. The card is named from the in-batch read and the entry log line, never from `N`.

**The composed batch is honestly fenced.** Entry + control + aim + confirm in one batch is `NOT MEASURED` in the header, in Step 3, in the Step 4 table (row 4), in Fences and in the README's Pending row 3, with `TASK-1512` as the measuring row. The entry's batch position is labelled as the recipe's own, and its agreement with 1391 C4's census `167 → 168` ("the +1 being the ghost") is labelled a reading.

## §3 — Spot-check: `RCP-menu-to-deckbuilder.md` was not touched: CONFIRMED (by mtime; residual stated)
- mtime **2026-09-26 15:29:05.776 local**, creation 15:29:05.775. My `qa/TASK-1505.md` was written at **16:39:07.354**. The file has not been written since before the gate that reviewed it.
- 7802 B, 86 lines (the loop-0 handoff also says 86). The loop-1 files were all written 16:51:15–16:52:01.
- Residual: an mtime can be reset. No loop-0 hash of this file exists to compare against.

## §4 — Handoff integrity: WHOLE
- `handoffs/TASK-1502-programmer.md`: 22867 B, 172 lines, **uniform CRLF** (CR = LF = 172), ends in `\r\n`. The last line is the complete "…the section was read back whole." sentence.
- No heredoc remnant: 0 × `EOF`, 0 × `<<`, 0 × NUL.
- The `### Files changed this loop` table (L143–150) has its header, separator and five rows, with no duplicated or truncated row. It is followed by the checks paragraph, both scrutiny lists, the supersessions and *Not examined*.

---

## Findings (this loop)

- **[WARN] W-L1-1 `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md:8`: the "Read this first" box overcounts the entry evidence.**
  - It says Step 3 "is seeded from four runs that measured `inject_input_action IA_Card<N>` entering placement". Only **three** did: `1314A`, `1348` and `1270` (and `1270`'s in-sequence placement is a reading).
  - `1230`'s press was **refused** and entered nothing. It contributes the path, the reply and the key↔index mapping, which the Source section (play:17) states correctly.
  - This is the delivery-versus-entry conflation the rest of the recipe avoids, and it sits in the box a verifier reads first (`SC-§101`; the `TASK-1505` W2 precedent for a false measured claim).
  - Fix: "…seeded from four runs: three measured entry (`1314A`, `1348`, `1270`); `1230` measured the call, path and reply on a refused press."
  - README:4 ("four further runs that measured it") has the same count, in looser wording; adjust it with the fix.
  - Non-blocking: no step, read-back or fence is wrong.
- **[NIT] N-L1-1 play:93, "so it proves delivery, not entry".**
  - Coming back on a refused press shows the reply is **not** proof of entry. It does not show that the reply proves delivery. In `1230`, delivery was shown by the handler's own refusal line, not by the reply.
  - Fix: "it is not evidence of entry; that the press arrived was shown in `1230` by the handler's refusal line."
- **[NIT] N-L1-2 play:102: `1270`'s in-sequence reading has a stronger basis than the call list.**
  - The derivation assumes that "Tool calls into PIE" is exhaustive. But the same report names `ui_snapshot`, `survey_pie_scene` and `capture_pie_frame` calls that the list does not itemise, so exhaustiveness is itself a reading.
  - `1270` row 3's own stamps give a better basis: a slot read at t=01m07.22s, then `IA_Card1` at t=01m07.23s, then `IA_DiscardAll` at t=01m07.57s. A 0.01 s gap cannot contain an MCP round trip, whose floor is ≈30 s (`VER-§8` cl. 10(b)).
  - Optional: add this as the primary basis.
- **[NIT] N-L1-3 play:43 and play:225: "An unaffordable card key is refused before placement", tagged `[M: 1230 row 1]`.**
  - That measurement is on a **Spell** (`Fog`; `1230`'s header records `CardType=Spell`).
  - That the same gate precedes placement for a Unit card is `[D]`: `PlayHandSlot`'s affordability gate (`SiegePlayerController.cpp:1124–1134`, "affordability gate first … outranks the type refusal") runs before the `CardType` switch (`:1271`).
  - Tag both halves.
- **[NIT] N-L1-4 play:54, "a Building enters placement ('…WatchTower…') `[M: 1348 P4]`".**
  - P4 does not state the card's type. `WatchTower` = Building comes from `1230`'s header `DT_Cards` read (`CardType=Building`) or `[D]`.
  - Tag it.
- **[NIT] N-L1-5 play:62, "a hand read … just before the press".**
  - The read was 12.1 s before (t=01m26.9s vs the press at t=01m39.01s). No play came between, and the hand was unchanged 0.81 s after.
  - `1230` row 1 also states "0-based slot 0 = `IA_Card1`" in-line, not only in H2.
  - The claim stands and is, if anything, understated. Precision only.
- **[NIT] N-L1-6 `RCP-deckbuilder-slot-and-card-edit.md:16` and `:84`, "Every call it quotes carried the root `WBP_DeckBuilder`" / "the one its calls carried".**
  - The source evidences the root for **one** call: the §1(b) error text at t=43.63.
  - Fix: "the one call whose error text it quotes carried the root".

## Notes for build-master (`TASK-1506`)
- **Byte anchors at my instant** (sha256; LF unless stated). If any of these changes before staging, the change is outside this review.

  | file | sha256 | lines | bytes |
  |---|---|---|---|
  | `RCP-play-unit-card-from-hand.md` | `fc1d94dd…d047b8` | 235 | 30225 |
  | `RCP-deckbuilder-slot-and-card-edit.md` | `6dcd0013…976bda15` | 164 | 18336 |
  | `README.md` | `fa5cc2d0…aa2d25` | 79 | 8582 |
  | `RCP-menu-to-deckbuilder.md` | `2f0e485d…117534` | 86 | 7802 |
  | `handoffs/TASK-1502-programmer.md` (CRLF) | `3ab460de…ced57161` | 172 | 22867 |

- **The WARN and NITs do not block.** If the manager has them fixed before `TASK-1506` stages, the edited bytes need a delta check (a one-line re-gate), not a full loop. Carrying them as a follow-up row is equally valid.
- **Pathspec:** `TASK-1506` (1)'s floor list names `qa/TASK-1505.md` but not **`qa/TASK-1502-loop1.md`**. Derive it (`SC-§77`) and stage both.
- **The blocker, as the board reads at my instant:**
  - `TASK-1505` is `done`, `TASK-1503` is `qa-passed`, and `TASK-1502` is `qa-passed` with this flip.
  - A grep of `^- status:` for `in-progress — 5c COMMIT RUNNING` / `integrating` finds 0 rows.
  - The other qa-reviewer's `TASK-1509` (C++, commit B / `TASK-1514`) does not touch these files.

## For the manager (flags, not rulings)
- M7 (does `VER-§13` cl. 1 mean "one tool call"?) is still open. The deck-builder recipe is now written so that a ruling either way changes only Step 4's envelope.
- **For `TASK-1512`'s dispatch:** the handoff's five-point list (QA loop 1 §M2, "For `TASK-1512`") agrees with the recipe as reviewed.
  - Deck4 = 50× `Archer` removes the type exposure, and `IA_Card1` is the measured key.
  - Gold ≥ 12 at the key press is already met at t≈15 s (25 gold measured at t=15.18 on 1391 B; +1/s), so no gold wait is needed.
  - Read the entry row first.

## Not examined / limitations
- No recipe was executed, no PIE was run, and no git command was run. The composed batch is unrun, and the recipe says so.
- `[S]` spellings (the `{"type","params"}` envelope, `action_path`, `key`, `wait`/`frames`, `ui_perform`'s "ENGINE-SIDE in one call") were not checked against live schemas; I do not hold those tools.
- The unchanged sections of the play recipe (Preconditions 1/2/6/7, the Step 4 arm rows, Read-back arm rows, Coordinates, and most hazards) were re-read, but not re-traced sentence by sentence. They were traced at `TASK-1505`, and their 1391 quotes appear unchanged.
- The Step 4 `ui_perform` envelope reasons were not re-traced beyond the three checks in §1.
- The menu recipe's "untouched" status rests on mtime ordering, not on a hash (§3).
- Markdown rendering was not checked.
