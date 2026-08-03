# QA Report — TASK-430

**Gate covering TASK-428 (rung 1 + rung 2) and TASK-429 (harness instrumentation).**

Verdict (gate 1, 2026-08-03): **FAIL** — **1 BLOCKER · 6 WARN · 4 NIT**
⚖️ **SUPERSEDED BY `## Re-gate — BLOCKER-1` AT THE FOOT OF THIS FILE (loop 1 of 3): BLOCKER-1 IS CLOSED, TASK-428 IS
`qa-passed`, AND TASK-431'S BAR IS LIFTED.** The gate-1 body below is preserved unaltered as the record of what was
found and why; **§8's WARN/NIT ledger stands in full and travels forward.** Read the re-gate section for the live
verdict.

- **TASK-428 ⇒ `qa-failed`** (BLOCKER-1, disjointness). — ⚖️ **now `qa-passed`; see the re-gate.**
- **TASK-429 ⇒ `qa-passed`** (0 blockers; every specced item verified, both declared scope calls ruled IN).
- ⛔ **TASK-431 MAY NOT COMPILE AND MAY NOT RUN THE DEV GATE.** The blocker changes Zone A's bytes, so any
  number taken now is taken on a prompt that will not be the one measured — and the collision must not reach a
  compiled artifact that TASK-432 then runs against the sealed file. Loop cost is one word in two files.
  — ⚖️ **LIFTED at the re-gate. TASK-431 MAY compile and MAY run the dev gate.**

⚠️ **§12b DISCIPLINE FOR THIS REPORT: I opened `assistant_eval_holdout2.csv` for the disjointness check and for
nothing else. This report quotes, paraphrases and characterises NO holdout row, Id, sentence or expected field.
Every finding below is stated on the FEW-SHOT side.**

---

## 0. What I did, and what I did not

**Read raw, in full, never via Grep for anything structural** (`qa/TASK-416.md` NOTE-1; the trap is now confirmed
three times):

- `SiegeLlamaSpike.cpp` — `AppendZoneA` (`:220-354`), the roster/fixtures (`:404-541`), `AppendZoneB`/`AppendZoneC`
  (`:572-621`), `FGenerationResult` + `RunGeneration` (`:2817-3164`), `FSpikeOptions` + `FormatDeadlineRegime`
  (`:2191-2288`), `DefaultThreadCount` + `EnsureModelLoaded` (`:3191-3455`), `RunPrefillBound` (`:3498-3679`),
  `RunBenchJob` (`:3681-3883`), `RunOneSplit` (`:3889-4064`), `GetArgDouble` + `ParseOptions` (`:4395-4543`),
  `StartJob` (`:4222-4260`), `CmdSpikePrompt` (`:4612-4693`), the command registrations (`:4719-4764`).
- `SiegeAssistantSnapshot.cpp` — `PlaceVocabulary` (`:46-77`), `BuildZoneA` (`:605-861`), `BuildZoneB` head.
- `SiegeAssistantVocabulary.cpp` — **the whole file.**
- The vendored `llama.h` at every anchor TASK-429 cites.
- All three corpus CSVs.

⛔ **Not run: any parser, any compiler, any generator.** Per §9c (*"where an artifact has a PARSER, THE PARSER IS
THE REVIEWER OF RECORD"* and *"what a reviewer owes when the parser was not run: say so"*) — **this review is
READING-LEVEL and is reported as incomplete on exactly two axes**, both named in WARN-5 and WARN-6.

⚠️ **Anchor discipline:** TASK-433 found **7 of 7 game-lane `file:line` anchors in CONVENTIONS stale.** I trusted no
cited line. Every vendored-header anchor TASK-429 quotes was opened and is **EXACT** (`llama.h:313` `n_gpu_layers`;
`:382-385` abort_callback incl. *"currently works only with CPU execution"*; `:551-556` the query-the-actuals NOTE;
`:963-977` the decode codes incl. `2 - aborted`; `:985-989` the two thread getters). TASK-429 added no game-lane
line anchor, which is the right instinct.

---

## 1. ⛔ THE DISJOINTNESS CHECK — the seal, and the one thing only this gate could find

### Method

Every one of the **seven** few-shot sentences (the three landed + the four new) and every **new vocabulary entry**
compared against `assistant_eval_dev.csv` (25), `assistant_eval_holdout.csv` (15, spent) and
`assistant_eval_holdout2.csv` (15, sealed) — by literal substring in both directions **and** by near-paraphrase
(the TASK-412 WARN-1 precedent), plus the harder test the orchestrator set: **is any holdout row answerable purely
by pattern-matching a newly added exemplar, alias or note?**

### ⛔ BLOCKER-1 — `SiegeAssistantSnapshot.cpp:835` · `SiegeLlamaSpike.cpp:346` — few-shot (a)'s invented unit noun is **NOT disjoint** from `assistant_eval_holdout2.csv`

The exemplar **`order: war elephants to the middle` ⇒ `{"ask":"unsupported"}`** is not disjoint from the sealed
file. **The novel lexical item — the invented unit noun, which is the entire load-bearing content of both the
exemplar and its counterpart — is literally identical**, and the surrounding frame (an invented unit carried on an
otherwise perfectly well-formed order with an unambiguous destination) is the same frame. This is not a
near-paraphrase call; it is a literal collision on the one word that decides the answer.

**Consequence if it ships:** a holdout row becomes answerable by surface match against a Zone-A exemplar. That is
the §11 disjointness rule and the §12b failure-shape trap failing together, and per §12a the recovery once the file
has been scored is a **generation-3 corpus**, not a re-run.

**FIX — one line in each lane, and it is the cheap fix:** replace the invented unit noun in that exemplar. Keep
every property TASK-428 argued for and I agree with: **not a siege engine** (the class is *"not in the roster"*, not
*"siege engines are refused"*), an **unambiguous remainder** (so the lesson is that the unit alone decides), and an
**output naming no kind** (§9c seam-free). ⛔ **Do NOT change the holdout** — it is sealed and frozen at handoff
(§12a); the fix is on the few-shot side and only on the few-shot side.

⛔ **AND THE RETIRED WORD IS NOT SIGNAL.** Do not author any exemplar, alias, rule or note that mentions it or any
morphological variant of it. A fix keyed to that word is the memorization defect §12b names and I will fail it again.

**ABSENCE CERTIFICATE (the `qa/TASK-416.md` §2c idiom — learn that a string is absent, learn nothing else).** The
following are **absent from all three corpus files** (literal, case-insensitive, singular and plural), share no
morpheme with any of the 13 roster kinds or with any alias in the shipped table, are not siege engines, and are not
caster-adjacent (so they do not brush the §9b hazard):

```
werewolves   ·   harpies   ·   sand golems
```

This is an **absence certificate, not a recommendation.** The exemplar is the programmer's to author; bring any
word and I will clear it at the re-gate.

### ⚠️ THE FINDING BEHIND THE FINDING — this is not misconduct, and that is the point

**Both agents' seal claims are TRUE, and the collision is evidence FOR the seal rather than against it.** I
confirmed the simultaneity from the artifacts, not the claims:

| claim | corroborated how |
|---|---|
| **TASK-428 opened no holdout** | It shipped a colliding exemplar — a tuner who had read the file could not have. It added **zero unit aliases** while the sealed file leans on colloquials it never checked existed. It left one general case open and flagged it as *"the likeliest holdout-2 miss"* — a peeker would have known whether that guess was right. |
| **TASK-427 read no few-shot / Zone A / synonym table** | Its rows sit **orthogonal** to what the exemplars teach — where an exemplar teaches one direction of a mapping, the file probes the other. **Not one row uses the single alias TASK-428 added.** No row is unlocked by the note it added. |

⇒ **THE STRUCTURAL SEPARATION WORKED EXACTLY AS DESIGNED.** What happened is §9c's law one level up: two
independent agents, given the same public brief (*"a non-existent unit, and NOT a siege engine"*), drew from the
same small pool of salient fantasy units and **converged on the same word from the same head.** Neither could see
it. **The only pass that could see it is this one** — which is the entire argument for the gate existing, and it
should be recorded on the board as the wave's most valuable process result.

### The rest of the surface — CLEAR

| checked | result |
|---|---|
| the other three new few-shots vs holdout2 | **no literal match, no near-paraphrase, none answerable by pattern-match.** One shares a two-word span with a sealed row (WARN-1). |
| the three landed few-shots vs holdout2 | clear. (One has a **dev** problem — WARN-2.) |
| the new alias `nearest ancient ground` | **absent from holdout2 entirely.** Declared reactive to a dev row and correctly so; it unlocks nothing sealed. |
| the `archer != longbowman` note | unlocks nothing. Stated over word SHAPE (*"only a long- word"*), not over the burned strings; the rows it could touch are decided by something else. |
| the **removal** of `gold` | see BLOCKER-adjacent check in §3 — it breaks **no** corpus row in any of the three files. |
| unit aliases added by TASK-428 | **zero.** Every colloquial any corpus file uses was already in the table before this task ⇒ **rung 2's unit work cannot have fitted anything.** |

### ⚠️ WARN-1 — `SiegeAssistantSnapshot.cpp:845` — the economy exemplar shares a two-word possessive span with the sealed file

`order: get two more pikemen with our gold` shares one short possessive phrase with a holdout2 row. **I rule it NOT
a paraphrase and NOT a blocker**, on grounds I will defend:

- the shared span is the **class marker itself** — you cannot teach *"gold is not yours"* without the word `gold`,
  and the **rule** (`- Gold, buying and card play are the player's, never yours`) contains it regardless, so the
  exemplar confers no advantage the rule does not already confer;
- the word was **already burned** in generation 1 and is public on the board;
- §12b's actual requirement — *a different economy verb and a different object* — **holds in both lanes**: the
  verb frames are disjoint and the object classes are disjoint.

⇒ Recorded so the number is quoted honestly, not actioned. **Do not "fix" this by removing the word `gold` from the
rule or the exemplar** — that would trade a cosmetic overlap for the safety class itself.

### ⚠️ WARN-2 — `SiegeAssistantSnapshot.cpp:816` — few-shot #1 is a near-paraphrase of a **dev** row, and TASK-431 must not read that row as evidence

**Pre-existing, unchanged by TASK-428, and not a blocker** — dev is the fitted surface by construction and its
number is never the go/no-go figure (§11, §12a). But it is live for TASK-431's reading:

`order: send ten footmen with a sorcerer to the ancient ground on our side` differs from `DEV-01` in **two tokens**
(a numeral spelled out, and the proximity phrasing). Nine content words are shared.

⇒ ⚠️ **`DEV-01` flipping to a pass at TASK-431 is NOT evidence of generalisation** — the prompt carries a
near-clone of that sentence, and it is also the row the new alias directly targets. It must be excluded from any
"the class fix worked" argument. **Two rows of the predicted 22–24 are structurally uninformative for the same
reason; the honest margin over the gate of 22 is therefore thinner than the handoff's "roughly one row".**

---

## 2. ⚖️ THE CLASS-VS-STRING TEST — per class, and the weakest named

The board forbids four uniform passes. Judged on **public evidence only** — the rule text, the burned five and the
dev split — deliberately **not** on anything I learned from the sealed file (see §7).

| class | verdict | why it generalises to a sentence nobody wrote |
|---|---|---|
| **(c) SELECTION-PRESERVING** | ✅ **STRONGEST** | The rule is quantified over *"units the player names"* — **verb-independent**, so it fires on army-wide cues that are not in the synonym table at all. The exemplar carries a genuine conflict (its verb is a real listed `charge` alias) rather than a manufactured one, and it sits in a minimal pair with the block's final shot. Three measured instances across two corpora, one cause. |
| **(d) DETERMINER → QUANTITY** | ✅ PASS | States **the whole axis in both directions**, because the model was measured missing in both — a one-sided rule would have traded one failure for its mirror. The third clause (*never copy a count from the roster*) is the one no exemplar can teach, fixes the roster-copy failure, **and protects the shortfall rule**, since a model that reads counts off the roster can never produce a detectable shortfall. |
| **(b) ECONOMY / CARD-PLAY** | ✅ PASS (graded hardest, per board §(3)) | Taught in **three** places as required and then some: a plain system-prompt sentence, an exemplar, **and the deletion of the lure** (§3). The rule enumerates the **domain** (`Gold, buying and card play`) not a verb, so spend / buy / play / afford / save / sell / hire all land on it. The exemplar names a kind that **is** on the roster, because the failure is not *"unknown word"* but *"the object is real, so a legal-looking order exists"* — which is exactly what turned a card-play sentence into a live order. **Residual, named:** a card-play sentence naming neither gold nor a purchase verb rests entirely on the two words `card play`. Narrow, and the burned instance of that shape is spent. |
| **(a) OUT-OF-ROSTER REFUSAL** | ⚠️ **WEAKEST — and not for the reason the author expects** | Its *generalisation* is excellent: the operative clause **`Never swap in a listed kind`** names the **action** that failed (substitution) rather than the word that triggered it, so it fires on any unknown noun; and the exemplar deliberately avoids a siege engine so the lesson is not narrowed to one semantic field. **The weakness is elsewhere and it is real — see WARN-3.** |

### ⚠️ WARN-3 — `SiegeAssistantSnapshot.cpp:730` vs the `[notes]` line at `SiegeAssistantVocabulary.cpp:245` — the `unsupported` / `which_unit` seam, and why (a) is the weakest

The board asked me to read these as a pair. I did, and they do not merely sit close — **their antecedents overlap
and they route the overlap to different ask codes:**

```
rule  : "A name that is not a Siegebound unit kind does not exist. …: {"ask":"unsupported"}."
[notes]: "… a unit kind that is not listed there does not exist right now -> ask which_unit."
```

The distinction the author intends — *"not a card at all"* vs *"a card, but none alive"* — is **not derivable from
the prompt**, because the only evidence the model has about what is a card is Zone C's roster, **which lists only
LIVE kinds**. A real card with zero units alive is, from inside the prompt, indistinguishable from an invented one.
So the two lines are in genuine tension on a reachable input, and (a) is the line that creates it.

- **Not a blocker and not a scoring risk:** any question passes a Refuse row, and the ask code is not one of bar
  #5's four scored fields. **The wave's number is unaffected in either direction.**
- **It is a correctness risk downstream:** the ask code selects the player-facing template (§3), so a player who
  owns a card and has none alive is told *"that isn't a thing"* rather than *"which unit?"*.
- **Cheapest close, if it is closed at all:** make the rule's antecedent explicit (*a name that is not a Siegebound
  unit kind **at all***) — a few characters, and it costs nothing in the budget. ⚠️ **Not required for this gate**
  and I am not spending a loop on it; it should be recorded for TASK-423, which inherits both strings.

### The synonym table — the whole class, or a `bowmen` patch?

**The whole class, and the reframing is CONFIRMED FROM THE ARTIFACT rather than from the claim:**

- **`bowmen` was already an alias of `archer`** — `SiegeAssistantVocabulary.cpp:120` carries it, and the spike's
  transcription carries it at `:290`. **The alias was present when the burned row ran, and it still lost.** The
  obvious reading of rung 2 is therefore **provably wrong**, and I confirm it. ✅
- **The real competitor is the canonical symbol `longbowman`, which literally contains "bowman"** —
  `SiegeAssistantVocabulary.cpp:125`. That is the wizard/sorcerer shape (two real cards contending for one player
  word), which this table has always resolved with a `[notes]` line and never with alias surgery. **Deleting
  `longbow`/`longbows` could not have helped: the attractor is the symbol, and the symbol cannot be deleted.** ✅
- ⇒ **Adding no unit aliases after a 13-kind audit is the correct execution of "systematically", not a shortfall.**
  I re-derived the plural/colloquial coverage independently from the table and concur with every one of the audit's
  13 verdicts. **No alias change is required.**
- ✅ **`mage` / `caster` / `spellcaster` remain unaliased** — I read all 13 unit rows; **no unit alias contains
  "mage" as a substring**. §9b HOLDS. `sorcerer` stays thin (3) and `wizard` stays at one alias, both deliberately.
- ✅ **`militiamob`, not `militia_mob`; `own_castle`, not `my_castle`.** Both correct in both lanes.

---

## 3. ✅ THE `gold` DELETION — the mine is still reachable, checked exhaustively

The orchestrator's specific worry — *an over-broad deletion trades a refusal failure for a routing failure* — is
**not realised.** The edit removes **exactly one bare alias** (`SiegeAssistantVocabulary.cpp:186`); the surviving
row is `nearest_mine <- gold mine, mine, the mine, the mines`.

- **Every mine word survives.** `gold mine` · `mine` · `the mine` · `the mines` — nothing was collapsed.
- **The canonical symbol is itself `nearest_mine`**, so any phrasing containing the head noun resolves trivially;
  the DEV-01 finding is that the symbol's pull is if anything *too* strong.
- **I checked all 55 sentences across all three corpus files: not one uses `gold` as a PLACE.** Every mine
  reference in every file goes through a surviving mine word. **The deletion breaks no corpus row anywhere**, and
  it closes the mechanism behind the wave's worst measured failure.
- **The mechanism claim is confirmed:** a bare economy noun mapped onto a place symbol gave every sentence merely
  *containing* that noun a pull toward a mining order. Removing it is the correct class fix, and it is the third
  place the economy refusal is taught (§2, class (b)).

⇒ ✅ **RULED CORRECT. This is the highest-value edit in TASK-428 and it survives scrutiny intact.**
⛔ **Carry the warning into TASK-421 verbatim: `gold` must NOT be re-added to `nearest_mine` in
`DA_AssistantVocabulary`. It will look like an obvious omission and it is the single most damaging line that could
be restored.**

---

## 4. ✅ THE MIRROR — compared by READING BOTH, and by simulating the generator

**Stated plainly, as the board demands: I compared them by READING BOTH IN FULL. I did not trust the handoff, and I
did not run either generator.**

`SiegeLlamaSpike.cpp:220-354` against `SiegeAssistantSnapshot.cpp:605-861`, block by block:

| block | shipped | spike | verdict |
|---|---|---|---|
| header / schema / ask / intents | literals | literals | identical |
| places | **a loop over `PlaceVocabulary[7]`** (`:690-693`) | 7 literals (`:262-269`) | **I rendered the loop by hand: all 7 symbol+description pairs match, in the pinned order.** ✅ |
| rules (4 old + **4 new**) | `:702-749` | `:273-284` | identical, same order ✅ |
| synonyms | **computed by `BuildSynonymTable()`** | hand-transcribed (`:288-327`) | **I simulated the generator** — see below ✅ |
| examples (**7**) | `:815-858` | `:339-353` | identical, same order ✅ |
| wrappers | `"synonyms:\n"` + table + `"\n"` | same | identical (the table already ends in `\n`, so the `EndsWith` branch adds nothing) ✅ |

**The synonym simulation was mechanical, not by eye.** I applied `NormaliseAliases` (trim, lower, de-dup, drop the
canonical, sort by string) and `AppendSection` (rows sorted by lower-cased canonical, empty rows dropped) to all
**27** authored rows and compared each rendered line to the transcription. All 27 match, including the two
orderings that are easy to get wrong and that a careless transcriber would have inverted:

- `near runes` **before** `nearest ancient ground` — at index 4, `' '` (32) sorts before `'e'`;
- `militiamob` **before** `miner` — at index 3, `'l'` before `'n'`.

⇒ **The spike's hand-transcription independently reproduces a computed sort it did not perform. That agreement is
real evidence, and it is the one place in this batch where the two-artifact comparison is genuinely informative.**

### ✅ The char arithmetic — RE-DERIVED BY HAND, not accepted

Board item (7) says *do not accept the arithmetic*. I counted the added bytes myself, per line:

| block | claimed | **my count** |
|---|---|---|
| 4 new rules | +330 | 112 + 83 + 60 + 75 = **330** ✅ |
| 4 new few-shots (order line + JSON line) | +359 | 121 + 57 + 64 + 117 = **359** ✅ |
| synonym edits (−`gold, ` +`nearest ancient ground, ` +the note line) | +108 | −6 + 24 + 90 = **108** ✅ |
| **total** | **+797** | **797** ✅ |
| JSON-only subset of the delta | 212 | 85 + 22 + 22 + 83 = **212** ✅ |

**4314 + 797 = 5111. Every figure in §5a of the handoff is exact to the byte.** The pre-change reconstruction
matching §8's MEASURED 4314 is therefore a real validation of the method, not a coincidence.

### ⚠️ WARN-4 — §9c one level up: what a SHARED Zone-A error looks like, and which of them any check in this batch could see

Comparing two generators of the same authorship **cannot detect an error they share** — and I am now a third
reader of the same two artifacts, which does not fix that either. The three shapes, with an honest verdict on each:

1. **A kind symbol both lanes name that the live grammar forbids.** ✅ **NOT WIDENED on the kinds axis** — the
   emitted JSON still names only `footman`, `sorcerer`, `archer`, exactly the three the original examples named,
   and the two refusal exemplars are **seam-free by construction** (their inputs mention units; their outputs name
   no kind). Confirmed by reading both lanes. ⚠️ **The seam itself stays open and NOTHING in this batch can see
   it** — the fixture fields all 13 kinds, so a green bar #5 is not evidence it is closed (TASK-433's).
2. **A shape both lanes teach that the GBNF rejects.** No new wire shape is introduced. I checked the four new JSON
   lines against the **landed** grammar's productions rather than against the doc: `unsupported` is a registered ask
   code shared by the grammar's `ask` rule and the parser (`SiegeAssistantCommand.h:237`, `:330`;
   `SiegeAssistantGrammar.cpp:462-475`); `where` always carries `"none"` (`:510-519`); `n` accepts `1..30` or
   `"all"`; the key order `intent, who, where, when` matches. **By reading: legal.** ⛔ **Not by parse — see WARN-5.**
3. **A premise both lanes share that the corpora disagree with.** Only a **third artifact** can catch this, and for
   the disjointness axis I am that artifact. **I found one: BLOCKER-1.** That is precisely the class §9c predicts,
   arriving exactly where §9c says to look.

### ⚠️ WARN-5 — the mirror is **unexecutable by construction**, and "MIRROR OK" must not be read as executed

§9c: *a dump is not a parse; where an artifact has a parser, the parser is the reviewer of record.* Two gaps, both
structural and neither this wave's to close:

- **No command prints the SHIPPED `BuildZoneA`.** `Siege.Llama.SpikePrompt` prints the **spike's** Zone A only
  (`SiegeLlamaSpike.cpp:4620`, `:4630`). Combined with TASK-433's finding that `USiegeAssistantSnapshot` has **zero
  callers and zero automation tests**, **the byte-identity claim cannot be executed anywhere in this project
  today** — by me, by TASK-431, or by anyone. It is a reading-level claim and will stay one.
- **The few-shots have not been parsed by the sampler.** TASK-431 closes this incidentally: a non-NULL
  `llama_sampler_init_grammar` plus `parse_failures = 0` is the acceptance evidence §9c demands.

**Close (recorded, not tasked):** one automation test asserting `USiegeAssistantSnapshot::BuildZoneA(V)` equals the
spike's `AppendZoneA` output — cheap, and it would convert the load-bearing claim of this whole wave from *reviewed*
to *tested*. Recommend it be batched to TASK-423.

---

## 5. ⚖️ THE TOKEN BUDGET — the ruling

**The claim:** `zoneA_tok ≈ 1372` against the §12c ceiling **1389**, **17 tokens of slack**, **DERIVED**.

### 5a. Labelling — ✅ COMPLIANT

§12c requires DERIVED, never MEASURED, with the ratio named. The handoff labels §5b *"⚠️ DERIVED, NOT MEASURED. I
have no model"*, names every ratio, and correctly labels the **chars** MEASURED (chars are countable without a
model). **This is the §8 instance-1/instance-6 defect class handled correctly** — no figure wears an authority it
did not earn.

### 5b. The derivation — ⚖️ **RULED SOUND, AND IT IS NOT THE OPTIMISTIC READING**

The objection to rule on is §8's law: *a proxy is conservative only if it assumes the SMALLEST plausible ratio.*
Under a flat 2.71 the result is **1433 — over by 44.**

**I rule for the blend, on arithmetic the handoff did not make:**

- 2.71 is the measured ratio of **Zone B + Zone C** — band labels, `%` signs, underscored roster symbols, numerals.
  The added text is **prose rules, `order:` lines and comma-separated alias lists**. Applying B+C's ratio to
  Zone-A-shaped text is not conservatism; it is using the wrong measurement, and §8's own instance-1 is a warning
  about exactly that — a ratio carried across a content boundary with its name and value unchanged.
- **Zone A's own measured 3.79 is if anything conservative for the prose half.** Zone A = 4314 chars / 1139 tok,
  and Zone A **contains** its symbol-dense schema block and JSON exemplars (~560 chars). Price those at 2.71
  (~207 tok) and Zone A's **prose remainder** tokenizes at ≈ **4.0**, not 3.79. The author applied a ratio *below*
  the one its own content implies, and applied **2.71 to the JSON it did add** — which honours §8's spirit exactly.
- The blend itself is arithmetically exact: `212 / 2.71 + 585 / 3.79 = 232.6 ⇒ +233`. `1139 + 233 = 1372`. ✅

⇒ **The derivation stands. It is not optimistic.** ⚠️ **But 17 tokens is ~1.2 % of the figure, a ~4 % ratio error
erases it, and a DERIVED number is not a pass. It may not be quoted as one.**

### 5c. ✅ Why the missing TASK-423 guardrail is **not load-bearing for this wave**

The orchestrator's concern is that an overshoot lands with no startup assertion to catch it. **Three independent
catches already exist, and I verified all three by reading:**

1. **It is PRINTED before it is scored.** `Siege.Llama.SpikePrompt` prints `zoneA_chars=` (`:4630-4633`) and
   `zoneA_tok~=` (`:4678-4681`). §12c makes the measurement task a **HARD GATE**: *over budget ⇒ STOP, do not score
   anything against an over-budget prompt.* The instrument exists and the gate is written.
2. **An overshoot breaches a POLICY ceiling, not the context.** Even at the worst defensible reading:
   `1433 + 400 + 96 + 13 = 1942 of 2048` — **106 tokens under the ACTUAL context.** Nothing decodes wrong, nothing
   truncates, no measurement is silently corrupted.
3. **The harness refuses an over-budget prompt loudly and by name.** `RunGeneration` checks
   `PromptTokens + MaxOutputTokens > llama_n_ctx(ctx)` — **the ACTUAL, not the request** — and returns a named
   `PROMPT_BUDGET` failure at `Error` level (`SiegeLlamaSpike.cpp:2894-2907`). Verified.

**Plus:** the pre-agreed contingency (~44 tok, ordered: the unverifiable note first, the counts rule second) lands
at ≈ 1329 and **correctly forbids cutting a refusal rule or a refusal exemplar** — the categorical half of the gate.
I endorse that ordering and I endorse the prohibition.

⇒ ⚖️ **RULED: the derivation is accepted, the wave is not stalled, and TASK-431 reads the printed figure BEFORE it
scores.** ⚠️ **BLOCKER-1's fix will move Zone A's byte count** — the re-handoff must restate the char delta and the
derived token figure; the change is a few characters and will not disturb the conclusion.

### ✅ 5d. The two-sided effect — reported, in both directions

§12c: *a handoff that names only the favourable direction is not reporting.* The handoff names **bar #3 improves**
(labelled derived-on-derived, correctly) **and bar #2 gets worse on a bar that already FAILS**, and states the
trade — spending the failing bar to buy the go/no-go bar — as **Jonathan's to overturn at TASK-435, not the
tuner's to bury.** ✅ Correct, and correctly escalated rather than decided.

### ✅ 5e. THE TRIPWIRE — `zoneB_chars = 68` and `zoneC_chars = 887` — **CONFIRMED BY DERIVATION**

Board item (6). I did not accept these; I counted them from the builders' own format strings at t0:

```
Zone B  8 + 19 + 21 + 10 + 10                                    =  68  ✅
Zone C  9 + 99 + 8 + 595 + 18 + 55 + 12 + 14 + 8 + 69            = 887  ✅
        [FORCES] / places / roster: / 13 rows / other_kinds /
        stances / hero / pending / [ORDER] / order line
```

The 13 roster rows are `39 + len(kind)` each (all counts single-digit at t0), and the kind lengths sum to 88 ⇒
`13 × 39 + 88 = 595`. **Both figures hold exactly.** `SpikeFixtureT0` (`:517-524`) matches the board's pinned board
value for value, `AppendZoneB`/`AppendZoneC` are untouched, and nothing in either zone reads Zone A or the
vocabulary. **Bar #3's reproduction and the corpora's premises are intact.** The in-code tripwire at `:4637` will
re-assert this at TASK-431.

---

## 6. TASK-429 — every specced item, verified

### ✅ (1) `deadline=` — default is still **10.0**, and a non-default is unmissable

- `SpikeHardTimeoutSeconds` is **`static constexpr double = 10.0` at `:142`, untouched.** `FSpikeOptions::HardTimeoutSeconds`
  initialises to it (`:2224`) and `ParseOptions` replaces it **only when the argument is present and in
  `0.1..600.0`** (`:4521-4534`). A command line without `deadline=` measures exactly what ships. ✅
- **Rejected values fall back loudly and are NOT clamped** (`:4526-4528`) — the reasoning is right: a silently
  clamped `0.0` from `deadline=abc` would abort every generation and read as catastrophic model failure caused by a
  typo. ✅
- **Announced in five places**, so it cannot be missed: `SPIKE_RUN START` (`:4236`), a dedicated `SPIKE_WARN` when
  non-default (`:4248-4253`), the bench SUMMARY (`:3855`), the hard-timeout warning (`:3146`), the WARN-R2
  suppression (`:3650`).
- **`FormatDeadlineRegime` (`:2274-2288`) always states BOTH facts** — overridden-or-default **and**
  authoritative-or-advisory — with the regime test `GpuLayers == 0`, which is exactly what `tier=cpu` sets. The
  advisory half is quoted from `llama.h:382-385`, **which I opened: exact.** ✅
- ✅ **Correctly NOT in the reload key** (`:3213-3226`), with `Threads` correctly IN it. A deadline sweep therefore
  costs no 2.5 GB reload, and a `threads=` cell correctly forces a fresh load — which is where the new readback
  prints. This is the right call and the comment explains it at the call site.

### ✅ (2) `threads=` — the actual is a REAL QUERY, and the mismatch warning fires

- `ActualThreads = llama_n_threads(GRunner.Context)` and `ActualThreadsBatch = llama_n_threads_batch(...)`
  (`:3379-3380`) — **genuine getters, called after the context exists and after the null check at `:3337`.** Not a
  re-print. ✅ Both symbols are declared at **`llama.h:986` and `:989`** — I opened the header; **the anchor
  `985-989` is exact.**
- The requested pair is captured from `ContextParams` (`:3381-3382`) and `PhysicalCores` from
  `FPlatformMisc::NumberOfCores()` (`:3383`) — **not deprecated in UE 5.8.**
- **The mismatch warning fires on either value** (`:3400-3405`) at `Warning` level, names both figures **and** the
  core count, and says in the log itself that a clamp is *"a CONFIGURATION ROOT CAUSE and not a hardware wall —
  that is a bug to fix, not a requirement to renegotiate."* ✅ Exactly the instrument the ruling asked for.
- ✅ **The requested value is confirmed as 14 from the code, not from the logs:** `DefaultThreadCount()` (`:3191-3197`)
  is `FMath::Max(1, NumberOfCores() - 2)`. The `cores=` field now prints the operand directly, so TASK-431 reads the
  inference instead of repeating it.
- ⚠️ **429 was right not to guess the actual.** I confirm both reasons independently: the vendored artifact is
  **binaries only** (there is no llama.cpp source tree in the repo to read the clamping logic from), and **no
  `llama_log_set` / `ggml_log_set` callback is installed anywhere in the plugin**, so llama.cpp's own thread line
  has never reached a UE log. **Building the instrument rather than asserting an answer is the correct call and it
  is the one this batch has repeatedly failed to make.**

### ✅ (2b) `gpulayers=` — the getter genuinely does not exist, so this is CLOSED, not deferred

**Confirmed exhaustively:** `n_gpu_layers` appears in the vendored header **exactly once** — as the **input** field
`llama_model_params::n_gpu_layers` at **`llama.h:313`** (anchor exact). There is **no** `llama_n_gpu_layers`, no
`llama_model_n_gpu_layers`, no equivalent under any name; the query block at `:551-559` exposes `n_ctx`,
`n_ctx_seq`, `n_batch`, `n_ubatch`, `n_seq_max`, `n_rs_seq` and stops there.

⇒ ✅ **Labelling it `gpulayers=%d(req)/%d` rather than inventing a number is the only honest option, and this line
is CLOSED — nobody should spend time hunting for that getter again.** The in-code comment at `:3407-3418` records
it at the call site, which is where the next reader stands. The fourth family member (`batch=`) also gained its
`(req %d)` so all five now read in one idiom — a readability gain, correctly declared as *not* a truth fix.

### ✅ (3) WARN-R2 — traced in BOTH directions, and the gate is COMPLETION

**This is the item the board says a wrong answer on is a FAIL. It is right in both directions.**

- **The gate is completion, never the drop value.** `bBoundCompleted = Turn1.CompletedNormally() && OutTurn2.CompletedNormally()`
  (`:3588`), where `CompletedNormally() = !bAborted && !bDecodeFailed` (`:2867`). **`DropPercent` appears in no
  predicate anywhere on the suppression path.** ✅
- **DIRECTION 1 — an abort is silenced as a layout finding.** `!bBoundCompleted` ⇒ a `SPIKE_WARN` naming both
  turns' `FailureDetail` and the deadline regime, then **`return false` at `:3653` — which sits ABOVE both guards.**
  The string *"THE PROMPT LAYOUT IS WRONG"* is **unreachable** on that path. Verified by reading the control flow,
  not the summary. ✅
- **DIRECTION 2 — a genuine defect still SHOUTS.** `bBoundCompleted` ⇒ execution falls through to the Zone-C-tail
  over-reuse guard (`:3660`) **and** the `DropPercent < 60.0` verdict (`:3671-3676`), both at `Warning` volume and
  neither reordered. The message now adds *"and BOTH TURNS COMPLETED — so this is a real reuse figure, not an abort
  artifact"*, which is the clause that makes the line quotable again. **No reachable drop percentage is newly
  silenced.** ✅
- **The mechanism trace is correct.** I followed it in the code: a turn-1 **prefill** stop is the only path that
  calls `GRunner.LastPromptTokens.Reset()` (`:3008`) ⇒ turn 2 finds a zero-length previous prompt ⇒ `CommonPrefix = 0`
  ⇒ turn 2 re-prefills everything ⇒ `DROP ≈ 0`. **`reused=0` in the recorded §12e line is the fingerprint of exactly
  that path.** The diagnosis is right.
- **`status=COMPLETED_BOTH_TURNS|INCOMPLETE` is on the `SPIKE_PREFILL` line** (`:3629-3631`) **and the validity
  travels onto the `BAR#3 BOUNDS` summary** (`:3739-3752`). ✅ That second half matters more than the first —
  suppressing inside the function alone would have been undone one line later, on the line a report is most likely
  to be copied from.
- ✅ **The drop number is still printed on the incomplete path, labelled**, and the two incomplete cases are
  labelled **differently** (turn-1 failure ⇒ *"NOT A MEASUREMENT … artifact"*; turn-2 failure ⇒ *"NO VERDICT — the
  drop itself is measured"*). **I endorse the turn-2 judgement call** the handoff flags at its §8.2: the drop
  genuinely is valid there (`PrefillTokens` and `PrefixReused` are both fixed before the first decode), and
  suppressing anyway is conservative in the direction the spec is categorical about. **Following a categorical spec
  rather than inventing a third rule is correct, and the wording keeps the reader from being misled the other way.**
- ✅ **`2 = aborted` verified against `llama.h:963-977`** (`0 success / 1 no KV slot / 2 aborted / -1 invalid batch /
  < -1 fatal`) — the pre-existing `bAborted = (DecodeResult == 2)` is right and was correctly left alone.
- ✅ All **five** stop sites now record a preformatted `FailureDetail` naming the code and the offset/token
  (`:2884`, `:2903`, `:2986`, `:3119`, `:3143`). ⚠️ **NIT-4** records the one stop that is deliberately *not*
  counted as incomplete.

### ⚖️ RULING ON THE DECLARED FOURTH CHANGE (handoff §6) — **IN SCOPE**

`RunOneSplit`'s row line gains ` ROW_DID_NOT_COMPLETE(...)` (`:4019-4026`). **Ruled in scope, and I concur with the
orchestrator's read:**

- **It is WARN-R2's exact defect in the accuracy lane.** A generation the ceiling cuts emits truncated JSON, fails
  to parse, scores as a wrong answer, and was — until this line — **indistinguishable in the log from a model that
  understood the sentence and answered it badly.** Same defect class, same lane-specific consequence.
- **The cost of omitting it is concrete and in-wave.** TASK-431 §(3)'s FAIL route is *"back to the programmer, this
  counts as a ladder loop"*. An aborted row could burn a ladder loop re-tuning a prompt to fix a **decode-rate**
  problem. **Manager ruling 10 exists because a diagnostic accused the wrong component twice** — leaving the
  accuracy lane with the identical hole while fixing the bench lane would be the same mistake, one file apart.
- **It is safe by construction:** one appended field on a `Display` line; **no score, no scoring rule, no corpus, no
  control flow**; empty on every completed row, so a clean run's output is byte-unchanged. One `%s` and one local.
- **The spec's (4) prohibitions are Zone A, tunable defaults and fixtures.** A log-only field is none of them.

✅ Same ruling for the two smaller in-spec additions (`aborted_or_failed=N/M` on the SUMMARY plus its warning, and
the reason carried on the per-iteration ` ABORTED` suffix) — both sit inside (1)/(3)'s declared surface, and the
first converts *"2 of 5 aborted"* from a fact recovered by eye into a printed number. **Declaring the change rather
than slipping it in is exactly the behaviour this pipeline wants and should be said so on the board.**

### ✅ Format specifiers — hand-counted on every new/changed line

Board item (9), and the handoff asks for the big one to be re-counted independently.

| line | specifiers | args |
|---|---|---|
| `SPIKE_LOAD` (`:3420-3432`) | **20** | **20** ✅ |
| thread-clamp `SPIKE_WARN` (`:3403`) | 6 | 6 ✅ |
| `SPIKE_PREFILL` (`:3629`) | 13 | 13 ✅ |
| WARN-R2 suppression (`:3651`) | 5 | 5 ✅ |
| completed-bound verdict (`:3674`) | 3 | 3 ✅ |
| `BAR#3 BOUNDS` (`:3749`) | 7 | 7 ✅ |
| bench SUMMARY (`:3856`) | 11 | 11 ✅ |
| abort-count warning (`:3865`) | 5 | 5 ✅ |
| `SPIKE_LATENCY` iteration (`:3838`) | 15 | 15 ✅ |
| eval raw line (`:4025`) | 7 | 7 ✅ |
| the five `FailureDetail` / `DropField` / `StopNote` / `StopSuffix` builders | match | match ✅ |
| non-default-deadline warning (`:4251`), START (`:4237`), range rejection (`:4527`) | 5 / 7 / 3 | 5 / 7 / 3 ✅ |

Types check: `%d` for `int32` with explicit casts off `int32_t`/`uint32_t`; `%.0f`/`%.1f`/`%.2f`/`%.3f` for
`double`; every `FString` dereferenced. **All format strings are literals** — UE 5.8 `TCheckedFormatString` is
satisfied and the TASK-268 C7595 trap is not re-armed.

⚠️ **The `%`-in-an-argument trap is handled correctly and I checked it specifically**, because it is subtle: the
`Expectation` string at `:3731` contains bare `%` characters (`~70%`, `78%`), is embedded into `DropField` via
`%s`, and `DropField` is then itself an **argument** to another `%s` at `:3629`. Arguments are never re-scanned, so
the bare `%` is correct and **must not be doubled** — and the code carries a comment at each site saying so. The
same applies to `*Generation.Text` (raw model output) at `:4026`. ✅

### ✅ Standing sweeps

- **Comment trap:** every changed region read raw. **No literal `*/` inside any doc comment** in any of the three
  files; the new `⚠️`/`⛔` glyphs are **comments-only**, matching each file's existing practice, and no non-ASCII
  byte appears inside any prompt string literal (this matters — `SiegeAssistantSnapshot.cpp:41-44`'s ASCII law is
  about the byte budget and the char/byte identity a reviewer measures).
- **No per-tick work added.** Every new allocation is per-load, per-job or per-generation. The one call inside the
  decode loop (`FormatDeadlineRegime` at `:3148`) is in a branch that `break`s immediately ⇒ at most once per
  generation. ✅
- **No new `UPROPERTY`, no new GC surface, no new replicated state.** `USiegeAssistantVocabulary`'s constructor
  changes only the contents of existing reflected arrays; no member added, no signature changed;
  `SiegeAssistantSnapshot.h` is unmodified.
- **UE 5.8 API currency:** no deprecated call introduced. `llama_get_memory` / `llama_memory_clear` /
  `llama_memory_seq_rm` are the current vendored spellings (the deprecated `llama_kv_cache_*` family is correctly
  absent — and the stale *comment* naming it was already corrected by the landed TASK-433 pass).
- **M8 declaration present VERBATIM in both handoffs** — TASK-428 line 5, TASK-429 line 6. ✅

### ✅ Hard prohibitions (board item 4) — all clear

`mage` alias ✅ none (no unit alias contains the substring) · corpus CSVs ✅ untouched (headers byte-identical
across all three, row counts 25/15/15, Ids sequential; no code path in either task writes a CSV) · `Notes`
intent-regex ✅ intact · JSON key `"at_least"` ✅ unchanged in both lanes · place set §9a ✅ 7 symbols, pinned order,
both lanes · `SiegeAssistantMaxSelectionKinds` ✅ **3** (`SiegeAssistantCommand.h:96`; spike mirror `:157`) ·
`SiegeAssistantGrammar.{h,cpp}` ✅ not in either changed-file list · `MaxSnapshotChars` ✅ **1085** ·
`ZoneBCharReserve` ✅ **192** · `MaxRosterKinds` ✅ **8** · `ContextTokens` ✅ **2048** · `t0`/`t1` fixtures ✅ intact.

⚠️ `MaxUtteranceChars` **was** renamed — **by the landed TASK-433 BLOCKER-2 fix pass, which predates this wave**
(`SiegeAssistantSnapshot.h:243-271`) and explicitly preserves every measured figure for ASCII. **Not this wave's
change and not a violation.** See NOTE-3.

---

## 7. ⚖️ RULINGS ON THE THREE FLAGGED QUESTIONS

### ⚖️ RULING A — TASK-427's declared exposure (it read the whole dev split): **LEGITIMATE. On the record.**

I concur with the orchestrator, and the grounds are stronger than "harmless":

- **Permitted.** §12a: *"THE DEV SPLIT IS NOT SPENT AND IS NOT GENERATIONAL."* §12b's ⛔ list for a holdout author
  names TASK-428's handoff/files, any Zone-A builder, any few-shot, any synonym/vocabulary table. **Dev is on
  neither list**, and the board's own TASK-427 spec required the header to be matched character-for-character
  against the existing corpus files, so *some* dev read was mandated.
- **It carries nothing from the tuner's side of the seal.** Dev contains sentences and premises authored **before
  any prompt existed, by a different task**. No few-shot, no alias, and no Zone-A byte is recoverable from it.
- **It strictly STRENGTHENS the instrument, and the direction is what makes it safe.** Dev is what TASK-428 tunes
  against, so a holdout2 sentence that cloned a dev row would be a **fitted row wearing a holdout's clothes** — the
  tuner would have seen its twin. 427 could not prove non-collision without seeing all 25, and it **rewrote four of
  its own sentences** that were drifting onto dev frames. Had it not read dev, those four would have shipped, and
  **four of fifteen rows is a ~27 % inflation risk on the go/no-go number.**
- **The exposure is one-directional and was declared as such:** dev → *avoidance only*, never dev → imitation.
  §11's fitting law is about corpus → prompt vs prompt → corpus; this is neither.

⇒ ✅ **RULED LEGITIMATE, and it should be written into §12a as sanctioned practice for every future generation:
a holdout author SHOULD read the dev split in full, precisely to prove non-collision, and MUST declare it.** A
declared exposure that increases independence is not the thing the seal exists to prevent.

### ⚖️ RULING B — `SiegeAssistantVocabulary.cpp` is not in TASK-428's `names:` list: **IN SCOPE. The spec wins.**

- The board's spec **(2)** orders rung 2 — *"author the colloquial + plural class systematically for every kind"* —
  and spec **(3)** orders **BOTH LANES OR NEITHER**. `BuildZoneA` renders the table by calling
  `Vocabulary->BuildSynonymTable()` (`SiegeAssistantSnapshot.cpp:756-770`), so **the table cannot be changed
  anywhere else in the game lane.** A `names:` list that omits it makes spec (2) unexecutable. **The spec is the
  instruction; the `names:` list is an index of it, and an index that contradicts its own text loses.**
- **No ownership conflict exists:** TASK-429 takes the spike *after* TASK-428; TASK-433 is read-only and **done**;
  the in-flight comment fix is in `SiegeAssistantGrammar.cpp`, which neither task opened.
- **The programmer declared rather than assumed**, which is the behaviour that makes this rulable at all.

⇒ ✅ **RULED IN SCOPE.** The board's `names:` for TASK-428 is corrected in the replacement text below.

### ⚖️ RULING C — TASK-429's fourth change: **IN SCOPE.** Full reasoning in §6 above.

---

## 8. WARN / NIT ledger

- **[WARN-1]** `SiegeAssistantSnapshot.cpp:845` · `SiegeLlamaSpike.cpp:348` — the economy exemplar shares a
  two-word possessive span with the sealed file. **Not a paraphrase, not actionable, recorded for honesty.** Do not
  "fix" it by weakening the safety class. (§1)
- **[WARN-2]** `SiegeAssistantSnapshot.cpp:816` · `SiegeLlamaSpike.cpp:340` — landed few-shot #1 is a
  near-paraphrase of a **dev** row (two tokens apart, nine content words shared). Pre-existing. ⇒ **`DEV-01` is not
  evidence of generalisation at TASK-431, and the honest margin over the gate of 22 is thinner than "one row".** (§1)
- **[WARN-3]** `SiegeAssistantSnapshot.cpp:730` vs `SiegeAssistantVocabulary.cpp:245` — the
  `unsupported` / `which_unit` antecedents overlap on an input the prompt cannot disambiguate (a real card with
  zero alive is indistinguishable from an invented one, because Zone C lists only live kinds). **No scoring
  impact**; it selects the player-facing template. Record for TASK-423. (§2)
- **[WARN-4]** §9c one level up — a shared Zone-A error survives every check in this batch on the **kinds-seam**
  axis; the fixture fields all 13 kinds, so **a green bar #5 is not evidence that seam is closed.** (§4)
- **[WARN-5]** The mirror is **unexecutable**: no command prints the shipped `BuildZoneA`, and
  `USiegeAssistantSnapshot` has zero callers and zero tests. **"MIRROR OK" is a reading-level claim and always will
  be until a test asserts it.** Recommend batching that test to TASK-423. (§4)
- **[WARN-6]** §9c — **the few-shots have not been PARSED by the sampler.** Legal by reading against the landed
  grammar's productions; *"it matches the schema"* is not evidence. TASK-431's non-NULL
  `llama_sampler_init_grammar` + `parse_failures = 0` is the acceptance evidence and must be quoted as such. (§4)

- **[NIT-1]** `SiegeAssistantSnapshot.cpp:825` — the §9c seam **is** widened by one symbol, on the **places** axis
  (`own_castle`), which the handoff's claim does not cover (it is precisely true about kinds). The widening is
  inert: `own_castle` can only be absent from the generated `where` alternation once the player's castle is
  destroyed, i.e. after the match has ended. **Recorded for precision, not for action.**
- **[NIT-2]** `SiegeAssistantVocabulary.cpp:209` — `Table.Reserve(2048)` is now undersized (the rendered table is
  ~2.4 KB), so `BuildSynonymTable` reallocates once. Same class of thing the `Reserve` bumps in the other two files
  fixed; emits no different byte. Free to fix, free to leave.
- **[NIT-3]** The `Reserve` bumps (`2048 → 6144` shipped, `4096 → 6144` spike) were nobody's ask. **Accepted**:
  Zone A measured 4314 against a 2048 hint *before* this task, so the reservation was already undersized; they
  change no emitted byte and the sizing is stated at the call site. Not a minimal-diff objection worth a loop.
- **[NIT-4]** `SiegeLlamaSpike.cpp:3105-3110` — a **context-full** stop sets neither `bAborted` nor `bDecodeFailed`,
  so it counts as `CompletedNormally()`. **Deliberate and documented** at `:2860-2866` (*"a natural stop — EOG, the
  token budget, or the context ceiling"*), and defensible (the prefill counters are fixed before decode, so the drop
  is valid). Recorded so a future reader does not discover it as a surprise.

---

## Notes for build-master

⛔ **TASK-431 IS BARRED FROM COMPILING AND FROM RUNNING THE DEV GATE until the re-gate returns PASS.** Grounds:
the fix changes Zone A's bytes, so any dev number taken now is taken on a prompt that will not be the one measured
(§8: *measure a different layout and the number means nothing*); and the collision must not reach a compiled
artifact that TASK-432 then runs against the sealed file, where the only recovery is a generation-3 corpus.
⚖️ **THE RE-GATE RETURNED PASS. THIS BAR IS LIFTED — see the re-gate section's §R10.**

**When the re-gate passes, these carry forward:**

1. ⚖️ **READ `zoneA_tok~` BEFORE SCORING ANYTHING.** `Siege.Llama.SpikePrompt fixture=t0` prints `zoneA_chars=` and
   `zoneA_tok~=`. §12c is a **HARD GATE**: `> 1389` ⇒ **STOP**, apply the contingency cut in the stated order (the
   `archer != longbowman` note first, the counts rule second), re-run. ⛔ **Never cut a refusal rule or a refusal
   exemplar to make budget.** Quote the printed figure beside 1389 every time; **1372 is DERIVED and is not a pass.**
   (⚖️ **the figure is now 1371 / `zoneA_chars=5108` — see §R5.**)
2. ✅ **The t0 tripwire must still print `zoneB_chars=68 zoneC_chars=887`.** I re-derived both by hand and they hold
   in the current tree; the in-code check at `SiegeLlamaSpike.cpp:4637` only fires for the **default order line**,
   so run `SpikePrompt` with no `order=` override.
3. **Cheapest cell first, and it needs no bench:** `Siege.Llama.SpikeLoad tier=cpu gpu=0` ⇒ read
   `threads=A(req 14) threads_batch=B(req 14) cores=N`. **If `SPIKE_WARN: THE CONTEXT CLAMPED THE THREAD REQUEST`
   accompanies it, STOP and report** — the rest of the matrix would be measuring a misconfiguration, not hardware.
4. **`gpulayers=-1(req)` does NOT mean 36 layers landed.** `-1` is the request *"all of them"*. The header ships no
   getter — **I verified this exhaustively; do not spend time hunting for one.**
5. **Any `SPIKE_PREFILL` line with `status=INCOMPLETE`, or a `BAR#3 BOUNDS` line carrying `NOT A BAR #3 RESULT`, is
   not a bar #3 figure.** Do not quote it. §12e's *"THE PROMPT LAYOUT IS WRONG"* line is now unreachable on that
   path by construction.
6. **`wall60_equiv_ms` is an EXTRAPOLATION** and must never be quoted as a measurement (§12d). Report `PRIMARY`
   against the absolute bar with the split's own printed leniency floor beside it; ⛔ **no generation-to-generation
   delta** (§12a — a QA FAIL).
7. ⚠️ **NOTE-3 — the working tree carries more than this wave.** `SiegeAssistantSnapshot.{h,cpp}` already contains
   the **landed TASK-433 fix pass** (the `MaxUtteranceChars` → byte-counted rename, the `llama_kv_cache_seq_rm`
   comment correction) alongside TASK-428's edits. I have **no Git access by design** and could not verify what is
   committed. **Check before staging so TASK-434 does not attribute a third task's work to this wave's commit.**
8. ⚠️ **NOTE-4 — the eval row line prints the row's sentence** (`SiegeLlamaSpike.cpp:4025`, pre-existing, and it is
   the line TASK-429's new `ROW_DID_NOT_COMPLETE` suffix rides on). **When TASK-432 runs the sealed file, the log
   will contain all 15 sealed sentences.** Per §12a a holdout is spent the moment a per-row result reaches a report,
   a handoff or the board — **paste no per-row eval line from that run into any artifact.**
9. ⚠️ **NOTE-5 — a `ROW_DID_NOT_COMPLETE` row is NOT a comprehension failure.** It scores as a wrong answer and is
   a stopped generation. **Do not route a ladder loop against a split containing them** — raise `deadline=` or use a
   tier that finishes, and re-run.

**Re-gate scope, so the loop stays cheap (this is loop 1 of 3):** the returning handoff needs only (i) the replaced
exemplar in **both** lanes, (ii) the re-derived char delta and DERIVED token figure, (iii) a one-line mirror
re-verification of the changed line. **Everything else in this report stands and does not need re-review.**

---
---

# Re-gate — BLOCKER-1

**Loop 1 of 3 · 2026-08-03 · scope: TASK-428 BLOCKER-1 ONLY.** Everything above this line stands, is not reopened
and is not re-argued; §8's WARN/NIT ledger travels forward intact.

**Re-gate verdict: ✅ PASS — 0 BLOCKERS · 1 new WARN · 1 new NIT · 1 escalation recorded.**
⛔ **BLOCKER-1 IS CLOSED.**

**Re-read for this pass:** `handoffs/TASK-428-programmer.md` §R0–R6 · `SiegeAssistantSnapshot.cpp` `BuildZoneA` in
full (`:605-882`) · `SiegeLlamaSpike.cpp` `AppendZoneA` in full (`:220-354`) · `SiegeAssistantVocabulary.cpp`
`:178-249` · **all three corpus CSVs, raw** · nine TASK-429 anchors spanning `:142` → `:4764`.

⚠️ **§12b DISCIPLINE, AGAIN: I re-opened `assistant_eval_holdout2.csv` for the disjointness check and for nothing
else. Nothing below quotes, paraphrases or characterises any holdout row, Id, sentence or expected field. Every
sealed-file statement below is a VERDICT with the evidence deliberately withheld.**

---

## R1. ⛔→✅ DISJOINTNESS — the check only this pass can run

The revised exemplar, in both lanes (`SiegeAssistantSnapshot.cpp:856`, `SiegeLlamaSpike.cpp:346`):

```
order: werewolves to the middle
{"ask":"unsupported"}
```

| check | verdict |
|---|---|
| `werewolves` vs `assistant_eval_holdout2.csv` (**SEALED**) | ✅ **ABSENT** — literal, case-insensitive, singular and plural, and by stem (`werewol` / `wolf` / `wolves`). |
| `werewolves` vs `assistant_eval_holdout.csv` (spent) | ✅ **ABSENT** |
| `werewolves` vs `assistant_eval_dev.csv` | ✅ **ABSENT** — re-run here rather than accepted from §R1c. |
| **every OTHER word in the revised exemplar** (`order:` · `to` · `the` · `middle`) vs the sealed file | ✅ **NO COLLISION.** The only spans shared with the sealed file are function words that carry no information. **The destination noun is absent from the sealed file entirely** — it is a listed `mid` alias, i.e. public vocabulary, and it was already in a landed exemplar before this wave. |
| the output line (unchanged, `{"ask":"unsupported"}`) | ✅ **names no kind** ⇒ the §9c seam-free property BLOCKER-1's fix was required to preserve is preserved. |
| **near-paraphrase test** (TASK-412 WARN-1 precedent) | ✅ **NOT a near-paraphrase of any sealed row.** |
| **the harder test: is any sealed row answerable by surface-matching the revised exemplar?** | ✅ **NO.** |

**On that last row, because it is the one that decides the gate.** The frame — *invented unit + well-formed
remainder ⇒ refuse* — is deliberately shared with the class the sealed split probes, and that is **not** a
disjointness defect: if a shared frame disqualified an exemplar, no exemplar could teach any class the holdout
measures, and §11 would forbid the ladder itself. The BLOCKER-1 defect was narrower and specific: **the
load-bearing lexical item was literally identical**, so a sealed row could be answered by surface match *without
the class having been learned*. With `werewolves` the only route from this exemplar to a sealed row runs **through
the class** — which is exactly what the split exists to measure. ⇒ ✅ **BLOCKER-1 CLOSED.**

---

## R2. ⚖️ RULING D — taking a CERTIFIED word instead of authoring a fresh one: **SOUND.** This is the crux and it holds.

The programmer's two grounds, ruled on separately.

**Ground 1 — "a certificate is a purely negative fact, so it fits nothing to the holdout." ✅ CORRECT, and it is
the right formalisation of why the distinction exists.**

Fitting means *making the prompt likelier to answer sealed rows correctly by virtue of their specific content.* A
certificate says only *"these three strings do not occur."* There is no sealed row about `werewolves`, so the
string's absence cannot be used to answer anything. The exemplar's entire teaching value against the sealed file
derives from the **class**, which was drawn from the public brief and the burned generation-1 data — neither of
which is sealed. **Information transferred from the sealed side: zero.** ✅

**Ground 2 — "an uncertified word of my own is precisely the experiment that just failed." ✅ CORRECT, and
operationally it is the stronger of the two.** BLOCKER-1 was not carelessness; it was convergence from a shared
public brief, invisible to both authors. Re-authoring blind re-runs that experiment at a cost of one loop — and if
it survived a re-gate by luck rather than by check, at the cost of a **generation-3 corpus** once TASK-432 scores.

**⚖️ THE DISTINCTION DID WORK, AND HERE IS THE TEST THAT SHOWS IT.** Look at who supplied what:

- **QA supplied a negative fact** — three strings, cleared against criteria that are **all publicly checkable**
  (absent from all three corpora · share no morpheme with any of the 13 roster kinds or any shipped alias · not
  siege engines · not caster-adjacent per §9b). Anyone holding the corpora can reproduce the certificate. It
  therefore carries no privileged information about the sealed file beyond the absences it names.
- **The tuner supplied the pedagogy** — the bare-plural argument and the attractor argument (R3 below), **neither
  of which I stated and neither of which I could have stated without becoming the author of the prompt.**

**Neither did the other's job. That is the whole design, executed.** ⇒ ✅ **RULED SOUND.**

⚠️ **THE RULE THIS ESTABLISHES, and it should go to §12a for generation 3:** *a QA absence certificate must be
(i) purely negative and (ii) issued against publicly checkable criteria. The moment a reviewer finds itself
explaining why one candidate is pedagogically BETTER, it has crossed from certificate into recommendation and must
stop — because that judgement is made by the one agent that has read the sealed file.* I did not cross it here, and
the programmer's independent selection grounds are the evidence that I did not.

---

## R3. ⚖️ RULING E — the two selection grounds: **the exemplar is STRONGER, and there is ONE new risk, which is bounded and already instrumented.**

**Ground (1) — bare single-word plural. ✅ CORRECT, and this is a genuine improvement rather than a lateral move.**
The retired noun was `<modifier> <noun>`, and a compound invented unit carries the unwanted second lesson *"the
modifier is what disqualified it"* — the same narrowing the no-siege-engine choice exists to avoid. **That was a
latent weakness in the exemplar as originally shipped, and neither the tuner's original pass nor my gate-1 pass
caught it.** The programmer found a defect in its own artifact while fixing an unrelated blocker, and the
replacement is strictly better than the thing it replaces. Recorded, because that is rare in a re-gate. It also
eliminates `sand golems` on grounds wholly independent of anything sealed. ✅

**Ground (3) — keeps a real substitution attractor toward `ogre` via `brute`/`giant`. ✅ THE EXEMPLAR IS SHARPER.**
The measured defect is *an unknown noun snapping onto the nearest listed kind* (trebuchet, catapult ⇒ `sapper`).
An invented unit with **no** roster analogue teaches refusal of the obviously alien — the easy case, and the one
the model was never observed failing. A noun with a plausible pull toward a listed kind teaches refusal **under
temptation**, which is the shape of the failure. ✅ Ruled stronger, and `harpies` is correctly eliminated.

### ⚠️ WARN-7 (NEW) — the sharper test buys a new risk: **refusal bleed onto the neighbouring live kind.** `SiegeAssistantSnapshot.cpp:856` · `SiegeLlamaSpike.cpp:346`

**Temptation runs both ways, and the handoff argues only one direction.** The same semantic proximity that makes
this a sharper refusal test raises the chance that a small model generalises *"beast-ish noun ⇒ unsupported"* and
**refuses a legitimate `ogre` / `brute` / `giant` order.** A word close to a live unit is a sharper test and a
nearer miss; the orchestrator's framing is exactly right and the handoff does not price the second half.

**Bounded, on four counts I verified rather than assumed:**

1. **There is no LEXICAL bleed channel — only a semantic one.** `werewolves` shares no substring, in either
   direction, with any of the 13 canonical kinds or any alias in the shipped table. The nearest lexical neighbours
   in the whole table are `worker`/`workers` (`wor-` vs `wer-`) and `waylay`; different stems, no relation. I
   re-read the table in full to confirm this rather than take §R1c's word.
2. **The prompt supplies the correct discriminator explicitly.** Both refusal rules key on **listedness**, not on
   semantics (*"A name that is not a Siegebound unit kind does not exist"*), and `ogre` is present **twice** as a
   counter-signal: in Zone C's live roster and in the synonym table with `brute, brutes, giant, giants`.
3. **It is DETECTABLE, and the detector is already printed.** `DEV-21` (`send the ogre at their castle`) asserts
   kind, count and place, so an `{"ask":...}` answer to it **passes LENIENT and fails STRICT**. ⇒ ⚠️ **A
   STRICT/LENIENT divergence on DEV-21, or `clarify_rows_passed_by_question` going non-zero, is the fingerprint of
   this bleed** — and it sits inside the over-refusal tripwire the tuner already declared at its §7.3. **TASK-431
   must name DEV-21 specifically when it reads that tripwire.**
4. **The go/no-go number's exposure: I checked the sealed file for this specific risk. ⚖️ VERDICT: the sealed split
   is NOT exposed to it.** Evidence deliberately withheld (§12b); this is stated so TASK-432's number can be read
   without re-opening the question, not so anyone can reconstruct why.

⇒ **WARN, not blocker.** The choice is right on balance — a refusal that is never tempted teaches nothing — and the
one cost it buys is instrumented, bounded and off the critical number.

---

## R4. ✅ THE RETIRED WORD IS SIGNAL-FREE — the claim is TRUE of the source tree, and the exact scope is worth stating

**Verified by direct sweep, not accepted from §R3's table:** the retired noun's stem occurs **ZERO times, case-
insensitively, anywhere under `Source/` or `Plugins/`** — in code, in prompt string literals, **and in comments.**
`SiegeAssistantVocabulary.cpp` is confirmed unreopened (249 lines; `gold` still removed; the `[notes]` line still at
`:242`; `+108` stands).

**⚖️ RULING F — the programmer's call that the retired word must not appear even in a COMMENT: UPHELD, and it is
the correct call for exactly the reason given.** A comment reading *"we retired X because it collided with the
sealed holdout"* writes a **fact about the sealed file into the compiled source tree** — the seal leaking through a
door §12b does not watch, in the one place nobody re-reviews, permanently. The tuner ruled on my seal and ruled
correctly; I am not overturning it.

**✅ And the anti-recurrence comment states the mechanism without the word.** `SiegeAssistantSnapshot.cpp:843-855`
records: same public brief ⇒ two agents converged ⇒ **neither could see it** ⇒ only the pass allowed to open both
sides could ⇒ the replacement was cleared against an explicit absence certificate ⇒ **any future change to this noun
must be cleared the same way by that same pass.** No word, no sealed-file fact, and it names the standing
obligation. **That instruction is worth considerably more than the word would have been** — it converts a one-off
catch into a rule the next author trips over at the call site.

✅ The stale quotation of the retired noun in the §9c seam comment (`:813`) was correctly updated in the same pass;
it now quotes the live word. The spike's comments were correctly **not** touched (its own comment says a mirror that
argues its own case invites drift — and leaving them alone is what preserved TASK-429's anchors; see R7).

---

## R5. ✅ THE RE-DERIVED FIGURES — re-derived BY HAND, exact to the byte, at the standard set at gate 1

⚠️ **I did not accept one number in §R2. Every figure below was counted from the source characters.**

**The changed line, both lanes:**

| | bytes |
|---|---|
| retired order line (incl. `\n`) | **35** ✅ |
| new order line `order: werewolves to the middle\n` | **32** ✅ (`order: ` 7 + 24 + `\n`) |
| **per-line delta** | **−3** ✅ |

**The blocks:**

| block | claimed | **my count** |
|---|---|---|
| 4 new rules | +330 | 112 + 83 + 60 + 75 = **330** ✅ (independently re-derived a second time; identical to gate 1) |
| 4 new few-shots | +356 | order lines 36 + 32 + 42 + 34 = **144**; JSON lines 85 + 22 + 22 + 83 = **212** ⇒ **356** ✅ |
| synonym edits | +108 | −6 + 24 + 90 = **108** ✅ (unchanged — the vocabulary was not reopened, verified) |
| **total delta over the MEASURED 4314** | **+794** | **330 + 356 + 108 = 794** ✅ |
| **Zone A after** | **5108** | **4314 + 794 = 5108** ✅ |
| JSON subset | 212 | **212 — unchanged, correctly**, because an `order:` line moved and no JSON line did ✅ |
| prose subset | 582 | 330 + 144 + 108 = **582** ✅ (was 585) |

**Tokens — ⚠️ DERIVED, NOT MEASURED:**

`212 / 2.71 = 78.2288` + `582 / 3.79 = 153.5620` = **231.7908 ⇒ +232** (round-half-up) ⇒ `1139 + 232` = **1371**
against the §12c ceiling **1389** ⇒ **slack 18.** ✅ All four rows of the sensitivity table re-checked:
`794/3.79 = 209.4987 ⇒ 1348` (+41) · `794/2.71 = 292.9889 ⇒ 1432` (over by 43) · `794/2.13 = 372.7700 ⇒ 1512`.
Full context `1371 + 400 + 96 + ~13 = 1880 of 2048`, **168 margin.** ✅

✅ **The rounding rule is not load-bearing:** under floor the blend gives +231 ⇒ 1370 ⇒ slack 19. The verdict is the
same under either convention, so no one need argue about it.

⇒ ⚖️ **RULED: THE REPLACEMENT IS SHORTER AND NO CONTINGENCY CUT IS OWED.** ⚠️ **And nothing here makes the budget
comfortable.** 18 tokens is ~1.3 % of 1371; a ~4 % ratio error erases it; under the flat 2.71 reading the figure is
**over by 43**, so this PASS still rests entirely on the blend I ruled sound at §5b — a ruling that stands unchanged.
**1371 is DERIVED and is not a pass.** TASK-431 reads the printed `zoneA_tok~=` **before** it scores anything, and
quotes it beside 1389.

---

## R6. ✅ THE MIRROR — one line, re-verified in both lanes

- `SiegeAssistantSnapshot.cpp:856` and `SiegeLlamaSpike.cpp:346` carry the **identical 32-byte literal**, newline
  included. I compared them character by character rather than by length.
- **Nothing else in either Zone A builder moved.** I re-read both builders in full and re-confirmed them against
  the gate-1 block-by-block comparison: header/schema/ask/intents, the 7-place loop vs its 7 literals, 4 + 4 rules,
  the 27-row synonym table, all seven examples in the pinned order, the wrappers. **Identical, and the two
  load-bearing sort orderings (`near runes` before `nearest ancient ground`; `militiamob` before `miner`) still
  hold.**
- **Both lanes at 5108** by the arithmetic in R5, derived independently on each side.
- ✅ **The rule-block count is independently reproduced twice now** — `112 + 83 + 60 + 75 = 330` — which is the
  cross-check the handoff leans on and it is sound.
- ✅ **The ordering law is intact:** both minimal pairs (#2/#3 determiner, #6/#7 selection) survive, the two
  refusals still sit in the middle, and the block still ends on a command. **5 of 7 remain commands.**

⚠️ **WARN-5 CARRIES FORWARD UNCHANGED AND MUST NOT BE READ AS DISCHARGED.** The mirror is still **unexecutable in
this project**: no command prints the shipped `BuildZoneA`. The handoff's `*** MIRROR OK ***` line is a **fourth
reading of the same two artifacts by the same authorship**, not a parse — and its own §R3 records that this script
has previously printed 10 chars of phantom drift. **I accept 5108 because I derived it independently, not because
the script printed it.** The asserted-not-remembered guard the re-gate script added is a real improvement and is
credited; it does not convert the claim from reviewed to tested.

---

## R7. ✅ TASK-429's `qa-passed` WORK IS UNDISTURBED — spot-checked, not accepted

**The spike is literal-only, and the zero-net-line-change claim is TRUE.** `SiegeLlamaSpike.cpp` **ends at line
4765**, confirmed by reading the file's tail. **Nine anchors spot-checked across the full span of the file, every
one EXACT:**

`:142` `SpikeHardTimeoutSeconds = 10.0` ✅ · `:3379-3383` the thread readback ✅ · `:3588` `bBoundCompleted` ✅ ·
`:3629` the `SPIKE_PREFILL` format string ✅ · `:3651` the suppression warning ✅ · `:3653` its `return false` ✅ ·
`:3660` the Zone-C-tail guard ✅ · `:3671-3676` the completed-bound verdict ✅ · `:4019-4026` `ROW_DID_NOT_COMPLETE`
and the eval raw line at `:4025` ✅ · `:4620`/`:4630-4633`/`:4637`/`:4678-4681` `CmdSpikePrompt` ✅ ·
`:4719-4764` the command registrations ✅.

**The snapshot's declared +21 shift is accurate and I re-anchored it:** `BuildZoneA` is now **`:605-882`** (was
`:605-861`); the economy exemplar moved **`:845` → `:866`** exactly as declared; anchors **above** the inserted
comment are unmoved (`:730` the refusal rule ✅, `:816` few-shot #1 ✅, `:825` the `own_castle` JSON line NIT-1 cites
✅). Vocabulary anchors unmoved (`:186`, `:209`, `:242`/`:245` ✅).

⇒ ✅ **Declaring the shift rather than letting it be discovered is the right behaviour and it checked out. TASK-429
remains `qa-passed` on the same evidence; nothing in §6 needs re-reading.**

### ⚠️ NIT-5 (NEW) — `SiegeAssistantSnapshot.cpp:614-615` — the `Reserve` comment still says Zone A is **5111** after this task

It is **5108**. Comment-only, emits no byte, changes no behaviour — but this wave's whole discipline is that a
figure in an artifact must be the figure. **Free to fix on the next touch of the file; ⛔ do not spend a loop or a
compile on it**, and TASK-431's printed `zoneA_chars=` is the authority regardless.

---

## R8. 🚨 ESCALATION — RECORDED FOR THE AUTHOR OF A GENERATION-3 CORPUS

**The programmer flagged a residual it cannot close, and it is real. I confirm it, and I must widen it, because the
leak is larger than the handoff describes.**

The handoff describes the channel as *"the report's line anchor plus Git history."* That is true and it is not the
whole of it. **The retired word is present in PLAINTEXT, today, in the working tree, in two pipeline artifacts:**

- `.claude/pipeline/handoffs/TASK-428-programmer.md` — the original §3 quotes the retired exemplar **verbatim**, and
  §3c tabulates it (9 occurrences).
- `.claude/pipeline/qa/TASK-430.md` — **this file.** BLOCKER-1 (§1) quotes the retired exemplar in order to name
  what had to change.

⇒ **Anyone with repo access can read the retired word AND read, one paragraph away, that it collided with a row in
the sealed generation-2 holdout.** That is a sealed-file fact recoverable without opening the sealed file, and no
Git archaeology is required to get it.

**Whose it is: mine as much as anyone's, and it was not avoidable at the time.** §12b permitted quoting the
**few-shot** side — the tuner's own public artifact — and a blocker that cannot name what must change is not a
blocker. The pairing of that quote with *"it collides with the sealed file"* is what makes it recoverable, and
there was no way to raise the finding without the pairing.

**Why it is NOT a blocker here, stated so nobody re-litigates it:**

1. **No measurement is contaminated.** The model sees Zone A + Zone B + Zone C and never a pipeline `.md`. The
   prompt is clean; TASK-431's and TASK-432's numbers are unaffected in either direction.
2. **The sealed file is untouched and still frozen** (§12a), and this changes no byte of it.
3. **The exposure is an AUTHOR channel to a FUTURE generation**, not a model channel to this one — which makes it a
   process question (§12a), not a code defect, and not something this wave can recover.
4. **Redacting it is worse than recording it.** Editing this report to remove the quotation would break the anchor
   discipline that made the finding possible and would leave the next reviewer unable to reconstruct why TASK-428
   looped.

### ⚖️ THE CHEAP CLOSE, and it is the one I recommend to whoever authors generation 3

**Do not try to contain the leak — DECLARE THE WORD BURNED.** Add the retired noun to the standing burned-surface
list that already carries *trebuchet · spend my gold · bowmen · the wizard · attack with the archers*, so that:

- **no generation-3 corpus row may use it**, and
- **no future few-shot may use it**,

for the same reason and by the same mechanism the project already handles burned strings. That converts an
uncontainable leak into a **declared burn**, which this pipeline already knows how to enforce — and it costs one
line in §12a. ⚠️ **It also means a generation-3 author does NOT need to read `handoffs/TASK-428-programmer.md` or
this report**, and the cleanest form of the rule is that they must not.

**⚖️ AND THE GENERALISATION, which is the part worth keeping:** *line-anchored review under version control cannot
discuss a sealed-file collision without making the colliding string recoverable.* Any future gate that finds a
collision will leak the same way. The structural fix — for whoever revises §12a — is to require that **a collision
finding names the string ONCE, in the report, and that the string is entered into the burned list in the same
motion**, so the leak is closed by declaration at the moment it is created rather than left open indefinitely.

---

## R9. LEDGER ADDITIONS FROM THE RE-GATE

- **[WARN-7]** `SiegeAssistantSnapshot.cpp:856` · `SiegeLlamaSpike.cpp:346` — the invented noun's semantic
  proximity to a live kind makes it a sharper refusal test **and** a nearer miss: risk of refusal bleed onto
  `ogre`/`brute`/`giant`. **No lexical channel exists (verified), the prompt supplies the listedness discriminator,
  and the sealed number is not exposed.** ⇒ ⚠️ **TASK-431: a STRICT/LENIENT divergence on DEV-21, or
  `clarify_rows_passed_by_question` non-zero, is this bleed's fingerprint — name DEV-21 explicitly when reading the
  over-refusal tripwire.** (R3)
- **[NIT-5]** `SiegeAssistantSnapshot.cpp:614-615` — the `Reserve` comment still says **5111**; it is **5108**.
  Comment-only. Fix on next touch; not worth a loop. (R7)
- **[ESCALATION-1]** The retired word + its collision are recoverable in plaintext from
  `handoffs/TASK-428-programmer.md` and this report. **Structural to line-anchored review under version control,
  not anyone's error.** Recommended close: **declare the word burned** in §12a's standing burned-surface list.
  **For the attention of whoever authors a generation-3 corpus.** (R8)

**Carried forward unchanged and still live:** WARN-1 · **WARN-2 (see R10)** · WARN-3 · WARN-4 · WARN-5 · WARN-6 ·
NIT-1 · NIT-2 · NIT-3 · NIT-4.

---

## R10. ⚖️ THE VERDICT, AND THE BAR

### ✅ **TASK-428 IS NOW `qa-passed`.** 0 blockers. BLOCKER-1 closed on the disjointness check only this pass can run.

### ✅ **TASK-431 MAY COMPILE AND MAY RUN THE DEV GATE. THE BAR IS LIFTED.**

Both grounds for the bar are discharged: Zone A's bytes are now final at **5108** (so a number taken from here is
taken on the prompt that will actually be measured), and **no colliding string will reach a compiled artifact that
TASK-432 later runs against the sealed file.** Every carry-forward item in *Notes for build-master* (1)–(9) applies
unchanged, with the figure in item (1) updated to **`zoneA_tok~` DERIVED 1371 / `zoneA_chars=5108`**.

### ⚠️⚠️ AND THE NUMBER MUST ARRIVE WEARING WARN-2 — this is not optional framing, it is how the figure is read

**Few-shot #1 (`SiegeAssistantSnapshot.cpp:816`) is a near-paraphrase of `DEV-01` — two tokens apart, nine content
words shared — and the new `nearest ancient ground` alias targets that same row directly. Two reasons, one row.**
The tuner adopted this **against its own prediction** and against its own interest; it must not be quietly dropped
downstream.

⛔ **TASK-431: `DEV-01` flipping to a pass is NOT evidence of generalisation and must be excluded from any "the
class fix worked" argument.** Two of the predicted 22–24 rows are structurally uninformative for the same reason.

⚖️ **THEREFORE THE READING RULE, and quote it in the TASK-431 handoff verbatim:**

> **A 22 or a 23 means "AT THE GATE WITH THE INFORMATIVE ROWS UNPROVEN" — NOT "passed with room."**

The honest margin over the gate of 22 is **thinner than one row**, not the handoff's original "roughly one row".
A 22 clears the ladder gate and it does **not** license a confident report; **24 or 25 is the first number that
carries real evidence of generalisation**, and any recommendation to Jonathan at TASK-435 must say which of the two
it is standing on. ⛔ **Do not report the dev figure without this sentence attached to it.**

### What TASK-431 additionally owes, all of it already on the board

1. Read `zoneA_tok~=` **before** scoring; quote it beside **1389**; **1371 is DERIVED and is not a pass** (R5).
2. Confirm the t0 tripwire still prints `zoneB_chars=68 zoneC_chars=887` (§5e).
3. Watch the over-refusal tripwire **and name DEV-21** (WARN-7).
4. `parse_failures = 0` + a non-NULL grammar sampler is the acceptance evidence for WARN-6 — quote it as such.
5. ⛔ **No holdout of any generation is opened at TASK-431.** TASK-432 opens holdout 2 exactly once.
