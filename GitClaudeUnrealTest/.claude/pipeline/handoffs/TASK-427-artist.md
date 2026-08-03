# TASK-427 — [LDR-1] THE FRESH SEALED HOLDOUT, GENERATION 2 — artist handoff

**Deliverable:** `Docs/Data/assistant_eval_holdout2.csv` — **SEALED at this handoff.** 15 rows, `HOLD2-01` … `HOLD2-15`.
**Opened exactly once, by TASK-432, on `tier=full`.** Any later edit voids the accuracy number and requires a generation-3 corpus in a new task (CONVENTIONS §12a).

**Runner:** `Siege.Llama.SpikeEval holdout=Docs/Data/assistant_eval_holdout2.csv` — no code change, no default to touch.

---

## 0. ⛔ SEAL DECLARATION — read this before anything else

**I read no few-shot, no synonym/vocabulary table, and no Zone A.** Specifically NOT read, at any point:

- TASK-428's files, its handoff, or any output of it (none existed — we were dispatched in the same message);
- `SiegeLlamaSpike.cpp`'s `AppendZoneA` body and its transcribed few-shot block (**lines ~189–378 of that file were deliberately skipped**; I entered the file at line 380, the roster array, and read forward);
- `SiegeAssistantSnapshot.cpp` in any part, including `BuildZoneA`;
- `DA_AssistantVocabulary` / any synonym table;
- **`Docs/Data/assistant_eval_holdout.csv` (generation 1) — never opened.** Everything I know about generation 1 is the five-row taxonomy in `handoffs/TASK-413-buildmaster.md` §21a, which is burned and public, and the board's own text.

**What I did read**, all of it sanctioned: the board's `#### TASK-427` entry · CONVENTIONS §9/§9a/§9b/§10/§11/§12(a,b,c) · `handoffs/TASK-413-buildmaster.md` §21a · `Docs/Data/cards.csv` · `SiegeLlamaSpike.cpp` from line 380 onward (the `t0`/`t1` fixtures, `SpikePlaces`, `LoadCorpus`, `ScoreRow`, `RunOneSplit`, `ParseSpikeOutput`) · `SiegeAssistantCommand.h`'s intent enum and the selection cap · **`Docs/Data/assistant_eval_dev.csv` in full.**

⚠️ **The one read that needs justifying: I read the WHOLE dev split, not just its header.** §11 permits it (the dev split is not sealed) and the board points at it as the schema reference, but my reason was stronger than layout: **the dev split is the file TASK-428 is tuning against**, so any holdout2 sentence that duplicated or near-cloned a dev row would be a *fitted* row wearing a holdout's clothes. I could not prove non-collision without seeing all 25. **Every one of the 15 sentences was then checked against all 25 dev sentences — literal match programmatically, near-paraphrase by hand — and §5 records the four places where I moved my own draft because it was drifting toward a dev row.** This is a declared exposure with a stated purpose; it carries no information from the tuner's side of the seal.

## 0a. ⚠️ TWO SPEC CLAUSES I DID NOT EXECUTE AS WRITTEN — both declared, neither silent

**(1) The board says this handoff carries "the file dumped as text". I did not dump it, deliberately.** The dispatch superseded that line ("without quoting the sentences, so the handoff itself never becomes a leak") and the dispatch is right on the law: **§12b's reason for barring QA from quoting a holdout row is that *"QA reports are read by the tuner, and one quoted row leaks the seal."* A handoff is read by strictly more people than a QA report.** A dump here would put all 15 sentences in the most-read file of the wave, one directory from the tuner, and the seal would be spent before TASK-432 ran. **The file on disk is the artifact; TASK-432 reads it from there.** Everything below describes the file structurally, and §5's avoidance table proves non-paraphrase **without printing a single new sentence or the novel lexical items themselves** — QA is the one role that may open the file and confirm the descriptions.

**(2) The board's "≤ 2 rows assert ≤ 1 field" is ARITHMETICALLY UNREACHABLE alongside its own "keep 3 Refuse rows". This file has exactly 3, and they are the 3 Refuse rows.** Proof from `LoadCorpus`/`ScoreRow`, not from preference:

- `AssertedFieldCount() = bIntentAsserted + bKindsAsserted + bCountsAsserted + bWhereAsserted`.
- A Refuse row asserts `intent=none`, and `bIntentAsserted = (value != "unasserted")` ⇒ **true** ⇒ 1 field.
- The other three columns describe *a command that must not exist*. They cannot be filled honestly.
- **Filling them anyway would change no outcome and would be pure metric-gaming:** on a Refuse row `bOutcomeOk = Parsed.bIsQuestion`, so a command fails whatever the kind/where cells say, and a question skips field comparison entirely. The number would move; the instrument would not.
- The only other way down is `intent=unasserted`, which produces a **0-field row that passes on any parseable output** — the exact defect the target exists to kill.

⇒ **3 one-field rows is the floor for 3 Refuse rows.** I kept the 3 Refuse rows (the board's explicit instruction; refusal is the safety class) and drove **every other row to ≥ 2 fields, with zero free rows** — generation 1's zero-assertion row has no counterpart here.

---

## 1. Row count, class breakdown, and the computed degenerate floor

| | count |
|---|---|
| rows | **15** |
| `Execute` | **8** |
| `Clarify` | **4** |
| `Refuse` | **3** |

**DEGENERATE FLOOR — the arithmetic, in full.** `RunOneSplit` computes it as *"the number of rows whose `ExpectOutcome` is not `Execute`"*, because a model that answers `{"ask":…}` to every sentence passes exactly those: on a `Refuse` row `bOutcomeOk = bIsQuestion` (true), on a `Clarify` row `bOutcomeOk` is **unconditionally true**, and in both cases a question **skips every field comparison**, leaving `bIntentOk/bKindsOk/bCountsOk/bWhereOk` at their `true` defaults ⇒ `bPassLenient`. It fails all 8 `Execute` rows, where `bOutcomeOk = !bIsQuestion`.

```
floor = (Clarify + Refuse) / rows
      = (4 + 3) / 15
      = 7 / 15
      = 46.666…%   → the runner prints 46.7%
```

**46.7 % ≤ the required 46.7 % ceiling. ✅**

**⚖️ THE BAR IS ABSOLUTE: ≥ 85 %, quoted beside *this file's own* 46.7 % floor, every time.** ⛔ Per §12a I have **not** computed, and nobody may report, a generation-1→generation-2 delta: the two files are different instruments and a delta between them is a category error, not an improvement measure.

**Thin rows: 3** — `HOLD2-13`, `HOLD2-14`, `HOLD2-15`, all `Refuse`, all asserting `intent=none` and nothing else, for the structural reason proved in §0a(2).
**Rows asserting nothing at all: 0.** **Every row in the file asserts at least one field; no row can pass for free.**

**Asserted fields: 46 of a possible 60.** Per-row: `01–08 = 4 each` · `09 = 3` · `10 = 2` · `11 = 2` · `12 = 4` · `13–15 = 1 each`.
**All 8 `Execute` rows assert all four scored fields** (`intent`, `ExpectKinds`, `ExpectCounts`, `ExpectWhere`), as required.

## 1a. ⚠️ A structural consequence of that requirement, recorded so nobody reads it as an omission

**"Every `Execute` row asserts all four fields" mathematically excludes four of the seven intents from the `Execute` class.** `follow`, `rally`, `charge` and `fallback` carry **no place** — `ExpectWhere` would have to be empty, which is "not asserted", which breaks the four-field rule. So holdout2's `Execute` rows are necessarily `send`/`guard`/`ambush` only (5/2/1). `fallback` appears as the army-wide half of `HOLD2-11` with `intent=unasserted`. **`follow`, `rally` and `charge` are not intent-asserted anywhere in this file.** They are covered in the dev split (`DEV-20`, `DEV-25`, `DEV-07`) and are outside both §11's mandatory-coverage list and the board's §(5) list — but a reader must not infer from a green bar #5 that they were measured here. ⚠️ **I also did not assert the `hero` place: §9a states plainly that no corpus row exercises it, and a fresh holdout is the wrong place to be the first — a wrong guess about what `follow` emits in `where` would fail the gate for a naming reason rather than a model reason.**

## 2. Coverage matrix — category × row

Every §11 / board-§(5) category, and where it lives. **No sentences quoted.**

| category (mandatory unless noted) | rows | outcome |
|---|---|---|
| ambiguous quantity | `HOLD2-09` | Clarify |
| unit that does not exist ⇒ Refuse | `HOLD2-13` | Refuse |
| out of scope (gold / card play) ⇒ Refuse | `HOLD2-14` | Refuse |
| **Sorcerer/Wizard collision — both cards named in one line** | `HOLD2-04` | Execute |
| **Sorcerer/Wizard collision — the `mage` clarification** (`mage` stays unaliased, §9b) | `HOLD2-10` | Clarify |
| selection verb mixed with an army-wide latched stance | `HOLD2-11` | Clarify |
| multi-kind **at the cap of 3** | `HOLD2-03` | Execute |
| multi-kind **over the cap (4 kinds)** | `HOLD2-15` | **Refuse** |
| shortfall, true at t0's counts | `HOLD2-12` | Clarify |
| deferred intent (`ExpectTriggerKind`/`AtLeast`, unsatisfied at t0) | `HOLD2-05` | Execute |
| plurals / colloquials | `HOLD2-02`, `HOLD2-03`, `HOLD2-08` | Execute |
| `all` (Count 0) | `HOLD2-03`, `HOLD2-05`, `HOLD2-06`, `HOLD2-07` | Execute |
| determiner → quantity | `HOLD2-01` (singular ⇒ 1), `HOLD2-07` (plural ⇒ all) | Execute |
| dropped unit selection (must not collapse to `who:none`) | `HOLD2-11` | Clarify |
| 2-kind selection | `HOLD2-04`, `HOLD2-08` | Execute |

**Place coverage** (all six usable §9a symbols; `hero` deliberately absent — §1a): `mid` ×2 · `own_castle` ×2 · `enemy_castle` ×2 · `ancient_ground_near` ×2 · `ancient_ground_far` ×1 · `nearest_mine` ×2.
**Kind coverage:** 10 of the 13 t0 kinds are named. The 3 absent — `sapper`, `cleric`, `ogre` — are exactly the zone-order-ineligible ones; eligibility objections are `Clarify`-shaped, all 4 `Clarify` slots were spent on higher-value classes, and dev covers them (`DEV-19`, `DEV-20`, `DEV-21`).

**⚖️ ONE DELIBERATE DESIGN CHANGE WORTH THE READER'S ATTENTION — the 4-kind row is `Refuse`, not `Clarify`.** The board permits either ("refused or clarified"). `Refuse` is **strictly the better instrument**, and the runner's own logic says why: as a `Clarify` row asserting intent+place, a model that **silently drops one of the four kinds** and emits a tidy 3-kind command would match both asserted fields and **PASS** — blessing the precise failure the law forbids ("it must never silently drop a kind"). As `Refuse`, any command fails and only a question passes. Costs one asserted field; buys the row its actual meaning.

## 3. ⚠️ Authored against the FROZEN `t0` fixture — nothing in it was moved

Every count-dependent premise was checked against `SpikeRoster` / `SpikeFixtureT0` as they stand on disk, and **no byte of the fixture was changed to suit a row** (§12a). Verified mechanically:

- **No `Execute` row asks for more units than are alive at t0** — a latent shortfall inside an `Execute` row would make it unpassable for a reason nobody wrote. Checked for all 8; the tightest are three rows that name a kind at *exactly* its t0 stock, which is legal and intentional.
- **The only shortfall in the file is `HOLD2-12`, and it is deliberate and `Clarify`** — matching `DEV-03`'s established reading (the command must carry the number as spoken, and the executor objects). ⚠️ **I considered making it `Execute` for the extra discriminating power and rejected it:** dev blesses a model that asks on a shortfall, so an `Execute` shortfall row here would fail a behaviour the tuning split rewards. That is an unfair instrument, not a strict one.
- **`HOLD2-05`'s deferred trigger is UNSATISFIED at t0** (`at_least` exceeds the live count) and its number is **absolute**, deliberately sidestepping `DEV-11`'s relative-word ("more") resolution trap so the row tests the latch and nothing else.
- **No `Execute` row commands a zone-order-ineligible kind** (`sapper`, `cleric`, `ogre` all have `orderable = 0`); every `Execute` selection is legal under the shipped eligibility split.
- **`HOLD2-15`'s four counts are all inside stock**, so the 3-kind cap is the *sole* reason to object — a single-cause row.

## 4. Verification performed — and the honest limit of it

**The parser is the reviewer of record (§9c), so I did not merely read the file back.** I re-implemented `SplitCsvLine`, `LoadCorpus`'s assertion flags, `AssertedFieldCount()` and `RunOneSplit`'s floor/thin/free counters as a standalone script and ran the file through it (`scratchpad/verify_holdout2.py`). It confirmed, with **zero problems reported**:

- header **byte-identical** to `SpikeCorpusHeader` **and** to the dev file's header; **no BOM, LF-only, pure ASCII, trailing newline** — matching the dev file's on-disk format exactly;
- every line splits to **exactly 9 fields**; **no cell contains a comma**, so no row depends on quoting;
- Ids are `HOLD2-01`…`HOLD2-15` in order; every `ExpectOutcome` is one of the pinned three;
- every `Notes` cell matches **`^intent=([a-z]+); `**, every value inside the 7 intents + `none` + `unasserted`;
- every kind symbol is a **live t0 roster symbol** (`militiamob`, not `militia_mob`) and every place a **pinned §9a symbol** (`own_castle`, not `my_castle`) — both checked against the arrays in the source, never against a message quoting them;
- **no row's kinds/counts lists are length-mismatched**, so the loader drops no count assertion silently;
- no selection exceeds the cap of 3; trigger kind is a roster kind and its `at_least` is genuinely unsatisfied at t0;
- **no sentence collides literally with any of the 25 dev sentences.**

⛔ **THE LIMIT, STATED PLAINLY: the real runner was NOT run — no editor, no model, no `Siege.Llama.SpikeEval`.** My script reproduces the loader and the *row-shape* scoring; it cannot prove the shipped binary agrees, and it generates no model output. **This is "reviewed by re-implementing the parser", which is stronger than reading and weaker than execution — and per §9c a reviewer who did not run the consumer must say so. TASK-432's run is the first true parse.** The one failure mode this leaves open is a `LoadCorpus` behaviour I mirrored wrongly; the header/field-count/intent-prefix checks are where that would surface, and they are the checks most likely to fire loudly rather than silently.

## 5. THE BURNED-FORM AVOIDANCE TABLE (§12b)

⚠️ **Deliberately written so it proves non-paraphrase WITHOUT printing my sentences or the novel words themselves** — see §0a(1). The burned forms are already public; my side is described structurally. **QA may open the file and confirm every claim below.**

| class | burned form (**forbidden**, gen-1 / dev) | what I authored, structurally | why it is NOT a paraphrase |
|---|---|---|---|
| **out-of-roster unit** | *"send the **trebuchets** at them"* (`HOLD-07`); dev also burns *"catapults"* (`DEV-04`) | a **living-creature** unit word, not a siege engine — different semantic field entirely, from a different part of the fantasy-army lexicon; carried on a different verb, a different determiner pattern and a **named place** rather than a bare pronoun object | no shared content word. A model that learned "trebuchet/catapult ⇒ refuse" as a **string** or even as "siege-engine nouns ⇒ refuse" gets **no** help here; only a genuine *"is this on my roster?"* check passes it. The order is otherwise perfectly well formed — the trap is entirely semantic |
| **economy / card-play** | *"**spend my gold** on another ogre"* (`HOLD-13`); dev also burns *"play a knight"* / *"buy a miner"* (`DEV-09`/`DEV-10`) | a **fourth economy verb**, an idiomatic two-word one, distinct from *spend*, *buy* and *play*; the object is a **building card**, not a unit card, so it is not "another `<roster kind>`"; the possessive-gold phrasing is restructured rather than reworded | the verb is not a synonym-swap of any of the three burned ones, and **the object class changes** — a fix keyed to "spend/buy/play + a unit name" misses it completely. ⛔ It targets the one failure that is **safety-shaped, not accuracy-shaped**: the standing ruling that the AI never spends gold and never plays cards |
| **colloquial synonym** | *"**bowmen**" ⇒ archer* (`HOLD-01`); dev also burns *"infantry"*, *"horsemen"* (`DEV-16`/`DEV-17`) | **two** fresh colloquials on **two different kinds**, in two different rows, neither of them a ranged unit; one is derived from the unit's **weapon**, the other from its **mount** — two different colloquial-formation mechanisms, and each maps to a kind whose card name shares no morpheme with the colloquial | *bowmen→archer* is a bow-word to a bow-unit. Neither of mine is a `-men` respelling of its target and neither is ranged, so a single alias row cannot cover them; the deliverable being tested is the **whole colloquial class**, exactly as §12b demands |
| **determiner → quantity** | *"**the** wizard"* ⇒ must be `n:1` (`HOLD-08`) | tested **from both directions in a matched pair**: one row uses an **indefinite singular determiner phrase** (⇒ 1) on a kind with several alive, so `1` and `all` are distinguishable; the paired row uses a **bare definite plural** (⇒ all) — the opposite mapping. Different determiner words, different kinds, different verbs, different frames | the burned form is *definite + singular*. Mine are *indefinite + singular* and *definite + plural* — **neither is the burned combination**, and the pair catches both over-correction directions: a model that patches "the ⇒ 1" now fails the plural row, and one that keeps "⇒ all" fails the singular row. ⚠️ **I removed a definite-singular determiner from a third row mid-draft** once I noticed it was drifting back toward the burned construction; that row now uses a bare numeral so its only test point is the Sorcerer/Wizard collision |
| **dropped selection** | *"attack with the archers"* (`HOLD-10`); dev also burns *"charge with the footmen"* (`DEV-07`) | the **opposite word order** — selection-first, subject position, with a **modal**, rather than *verb + "with the" + kind*; a **different army-wide stance verb** (the other one of the latched pair) and a different kind. Asserts kinds + counts so a `who:"none"` collapse **fails**, exactly as the burned row did | both burned forms share one frame: *`<army-wide verb>` + "with the" + `<plural kind>`*. Mine inverts it and drops the preposition entirely, so a few-shot pattern-matched to that frame does not fire. **This is the pathology PART 2 §13d refused to extrapolate from one anecdote and the holdout then reproduced** — it is retested here on a form that shares no frame with either burned instance |

**Additional avoidance work not required by §12b but done anyway** — because the tuner is scored on dev, so a holdout row that clones a dev row is fitted: **four sentences were rewritten mid-draft** purely because they were converging on a dev row's frame — a `guard <place> with <n> <kind>` clone, a `<n> <kind> to the far ancient ground` clone that additionally shared a numeral and a place with a burned holdout-1 row, an `ambush <place> with <n> <kind>` clone, and a two-kind ancient-ground row that was drifting onto the flagship dev sentence's shape. None of the 15 shipped sentences repeats a dev sentence, literally or as a near-paraphrase.

**Reactive coverage declaration (§11/§12b):** **none.** No row was written because an alias, few-shot or prompt already handled it — I have not seen the prompt, the few-shots or the vocabulary. The fitting direction is corpus → coverage only.

## 6. What the reader of TASK-432 must carry forward

1. **Command:** `Siege.Llama.SpikeEval holdout=Docs/Data/assistant_eval_holdout2.csv`, **`tier=full`** (§12a: the only tier with zero aborts; an aborted generation scores as a wrong answer and corrupts the number).
2. **Quote the number as `X %` against the absolute `≥ 85 %` bar, with `floor = 46.7 %` printed beside it.** ⛔ **No generation-to-generation delta** (§12a — a QA FAIL).
3. **The runner will print three honesty lines.** Expect: `LENIENCY FLOOR … 7/15 = 46.7%` and `3 row(s) assert at most ONE field … HOLD2-13 HOLD2-14 HOLD2-15`. **It will NOT print the zero-assertion Warning — there are no free rows.** If it does, the file is not the one I sealed.
4. ⚠️ **`STRICT` will be lower than `PRIMARY` if the model answers any `Clarify` row with a question**, and that is by design, not a defect: `bPassStrict` requires `AssertedFieldCount() == 0` for a question passing a `Clarify` row, and **all 4 of my `Clarify` rows assert fields.** **`STRICT == PRIMARY` is not a property to expect from this file.** `PRIMARY` (lenient) is the bar's number.
5. **The file is frozen.** QA may open it for the few-shot disjointness check (literal string **and** near-paraphrase, per the TASK-412 WARN-1 precedent) and **may not quote a single sentence, Id or expected field** in its report.

## 7. Scope

**No C++, no editor, no Blender, no MCP, no Git, no Blueprint.** `L_Arena` never opened. `cards.csv` read-only and untouched. `assistant_eval_dev.csv` read-only and untouched. `assistant_eval_holdout.csv` (generation 1) **never opened**. **The task board was NOT edited** — the dispatch reserved that; the orchestrator sets `TASK-427` to `ready-for-integration`. One file created: `Docs/Data/assistant_eval_holdout2.csv`.
