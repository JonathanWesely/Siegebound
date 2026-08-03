# QA Report — TASK-430 · `## Gate — loop 2`

⚠️ **FILING NOTE:** this section belongs at the foot of `qa/TASK-430.md`. **I have no `Edit` tool in this run**, and
re-`Write`ing a 1046-line authoritative artifact from a re-read is a corruption hazard I will not take on the file
that carries the wave's verdict. **Orchestrator: concatenate this file onto `qa/TASK-430.md` verbatim and delete it,
or leave it as the named sibling.** Everything in `qa/TASK-430.md` — gate 1's §8 WARN/NIT ledger and the loop-1
re-gate — **stands unaltered and travels forward.**

---

**Ladder loop 2 of 3 · 2026-08-03 · scope: the TASK-428 §L2 delta ONLY.** This is not a re-review of loop 1.

## Verdict: **PASS** — **0 BLOCKERS · 5 WARN · 3 NIT**

- **TASK-428 ⇒ `qa-passed`.**
- ✅ **TASK-431 MAY COMPILE AND MAY RE-RUN THE DEV GATE.** No bar is raised by this gate.

⚠️ **§12b DISCIPLINE:** I opened `assistant_eval_holdout2.csv` (sealed) and `assistant_eval_holdout.csv` (spent) for
the disjointness check and for nothing else. **This section quotes, paraphrases and characterises NO holdout row, Id,
sentence, expected field or Notes text.** Every finding below is stated on the few-shot / rule side, and §1 is a
**verdict with no evidence attached**, by design.

**What I read raw, in full:** `SiegeAssistantSnapshot.cpp:595-1041` (the whole of `BuildZoneA` plus its comment
block) · `SiegeAssistantVocabulary.cpp` **entire** · `SiegeLlamaSpike.cpp:215-364` and `:566-625` · the `where`
generation in `SiegeAssistantGrammar.cpp:410-524` · all three corpus CSVs · `handoffs/TASK-428-programmer.md` §L2
· `handoffs/TASK-431-buildmaster.md` entire. ⛔ **No parser, no compiler, no generator was run** (§9c — this stays a
reading-level review, and WARN-5 is unchanged).

---

## L1. ⛔ DISJOINTNESS — the one check only this pass can make. **CLEAR.**

Loop 2 changed **exactly one prompt sentence** (an `order:` line gained the verb `send`) and **five rule lines**. All
six new/changed strings were compared against **all 25 dev rows, all 15 spent-holdout rows and all 15 sealed rows**,
by literal substring in both directions, by content-word overlap with stopwords removed, and by the harder test:
**is any sealed row answerable by surface-matching a string this loop added?**

| checked | verdict |
|---|---|
| the changed refusal exemplar (`send` added; **noun untouched**) | ✅ **no literal containment either direction · no near-paraphrase · no sealed row answerable by matching it** |
| ⛔ the **retired word** (loop-1 BLOCKER-1's noun), any morphological variant | ✅ **0 occurrences in all three source files, in every comment, and in the rendered prompt.** Nothing is keyed to it. **It is still NOT signal and must stay unused.** |
| the exemplar's frozen noun | ✅ **unchanged, and its loop-1 absence certificate still holds** — re-verified against all three files this pass |
| **R1** (refusal, rewritten) | ✅ clear |
| **R3** (selection→intent, new) | ✅ clear |
| **R4** (counts, reordered) | ✅ clear |
| **R5** (place head-noun, new) | ✅ clear |
| the two `[notes]` **deletions** | ✅ deletions cannot create a collision; **and neither deleted string was load-bearing for any corpus row in any of the three files** |

### ⚖️ RULING ON THE ADDED VERB — the programmer's §L2.7c argument is CORRECT and I am adopting it

The raw Jaccard rise **0.29 → 0.429** is real and was declared before I found it, which is the behaviour that makes
it rulable. **I rule it NOT a §11 event:**

- **the entire rise is function words** — the added token plus the frame words it drags in. With stopwords removed,
  the shot's content overlap is the **second-lowest of all seven** and is far below the pre-existing WARN-1/WARN-2
  figures that were already ruled acceptable;
- §11 governs **fitting the prompt to the corpus**, which is a **content** relation. **Sharing the corpus's dominant
  syntactic frame is the mechanism the change exists to exercise** (§L2.3), not a leak — and a refusal exemplar that
  refuses to use the language's most common verb is the narrower artifact, not the safer one;
- ⚠️ **the deciding check, which I ran because the frame change is the only thing that could have caused it:** the
  added verb moves the exemplar **toward no sealed row in particular**. The sealed row nearest to it on class was
  already nearest at loop 1, and the added token is **absent from that row**. ⇒ the change is **orthogonal** to the
  seal rather than convergent on it.

⇒ ✅ **DISJOINTNESS CLEARED. The seal is intact and TASK-432's one-shot remains unspent and available.**

---

## L2. ⚠️⚠️ THE CRUX — the 4-to-1 count, and whether R3 is a gate or a fifth voice. **BOTH CONFIRMED.**

### L2a. The count is **EXACTLY 4 to 1**, and I counted it from the rendered prompt, not from the claim

| # | instruction | where it lives, verified | says |
|---|---|---|---|
| 1 | `charge = whole army attacks; who and where are "none"` | `SiegeAssistantSnapshot.cpp:687` · spike `:252` | who = **none** |
| 2 | `fallback = whole army defends home; who and where are "none"` | `:688` · spike `:253` | who = **none** |
| 3 | `rally = hero rallies units near him; who and where are "none"` | `:689` · spike `:254` | who = **none** |
| 4 | `... charge, fallback, rally move the whole army or the hero and take who = none. ...` | `SiegeAssistantVocabulary.cpp:243` · spike `:325` | who = **none** |
| — | **retired R3:** `- Units the player names always go in "who", never dropped.` | rung 1, one line | **keep them** |

⇒ ✅ **CONFIRMED, 4 to 1.** And the reading is worse than "out-voted": the four are **specific** (each names its
intent) while the retired rule was **general**, so the model was resolving a conflict in favour of the more specific
and more repeated side — **which is the correct resolution under any ordinary reading of an instruction list.**
**The model was obeying this prompt.** That reframes DEV-07 and DEV-16 from *comprehension failures* into *prompt
defects*, which is a materially different — and much more actionable — finding than loop 1's.

⚠️ **And I confirm the sharper half of it, which is a finding about MY OWN gate-1 report as much as the tuner's:**
rung 1 booked that four-fold repetition as a **saving** (*"already in Zone A twice, and Zone A pays for every
repetition"*), **I read that reasoning at gate 1 and rated the class STRONGEST on the strength of it**, and it was
not a saving — **it was the competitor.** ⇒ **On the record: my §2 ranking did not merely miss; it was built on the
same misreading the tuner made.** See §L9.

### L2b. ⚖️ IS THE NEW RULE A GATE OR A FIFTH VOICE? **RULED: A GATE. It is a different lever, not a louder one.**

```
- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.
```

The four competing lines are **all conditional on the intent** — each has the shape `<army-wide intent> ⇒ who="none"`.
The retired rule **asserted the opposite consequent** and therefore contradicted all four head-on. **R3 attacks the
ANTECEDENT instead:** when units are named, the three army-wide intents are unavailable, so all four conditionals
have a false antecedent and their consequents are **never triggered**. ⇒ ✅ **Structurally this is a gate, not a
fifth assertion.** The contradiction is not out-shouted; it is dissolved.

⚠️ **THE HONEST LIMIT, AND I AM STATING IT BECAUSE IT IS THE ONE THING THAT COULD MAKE THIS LOOP 1 AGAIN:** the four
lines are **still printed**, and they are still true statements. R3's protection depends on the model evaluating
R3's antecedent — a property of the **sentence** — rather than latching the intent off the **verb** first, which is
precisely what DEV-07 (`charge`) and DEV-16 (`back`) were measured doing. **A prompt cannot force evaluation order.**
So the mechanism is right and its efficacy is unproven — but it is unproven in a **new** way, not the old way.

✅ **Two corroborations I ran that the handoff did not claim:**
1. **R3 contradicts none of the seven exemplars.** #1/#6 name units and take `send`; #2/#3 name units and take
   `guard`; #7 names none and takes `charge` with `who:"none"`; the two refusals name no intent. **The only
   exemplar exercising the army-wide branch survives R3's antecedent untouched** — so R3 does not quietly retire
   few-shot #7, which would have traded DEV-25/DEV-13 for DEV-07/DEV-16.
2. **The replace-don't-join call is right.** Keeping the retired line would have restored the 4-way contradiction
   *on top of* the gate, i.e. the worst of both. ✅ Endorsed.

---

## L3. ✅ THE SLOT-FILL CLAIM — **CONFIRMED CHARACTER-FOR-CHARACTER.** It reframes the refusal failure.

Few-shot #1's output line (`SiegeAssistantSnapshot.cpp:951`, spike `:341`) contains, verbatim:

```
{"kind":"sorcerer","n":1}
```

as `...,{"kind":"sorcerer","n":1}],...`. DEV-04's measured output (`handoffs/TASK-431-buildmaster.md` §3) is
`{"intent":"send","who":[{"kind":"sorcerer","n":1}],"where":"enemy_castle","when":"now"}` — **its entire `who` array
is that substring**, and its `"when":"now"}` tail is few-shot #1's tail. **The only content DEV-04's sentence did
not supply is lifted verbatim from the nearest exemplar.**

⇒ ✅ **RULED: DEV-04 is SLOT-FILLING FROM AN EXAMPLE, not a semantic near-miss.** That is a genuinely different
diagnosis from baseline's `sapper` (which *was* a semantic neighbour), and it explains why `- ... Never swap in a
listed kind` could not bite: **from the model's seat there was no swap** — the slot arrived pre-filled. ✅ **The new
second sentence — `Never write a kind the player did not name.` — forbids it in the only terms that cover it, and
it is consistent with all seven exemplars** (every kind each one emits is named in its own `order:` line; the two
refusals emit no kind). ✅ **And R1's new antecedent (`not a kind in [FORCES]`) is a membership test the model can
actually run**, where *"a Siegebound unit kind"* named no list in the prompt. Both mechanisms are sound as written.

---

## L4. ⚖️ THE `nearest_mine` DELETION — 3 → 2 CONFIRMED, and the GBNF claim **RULED SOUND on two grounds, not one**

**Occurrence count in the rendered Zone A, counted by me:**

| # | occurrence | status |
|---|---|---|
| 1 | places block — `nearest_mine = the best gold mine for the player now` | **kept** (defines the symbol at all) |
| 2 | `[places]` alias row — `nearest_mine <- gold mine, mine, the mine, the mines` | **kept** (DEV-23 rides on it) |
| 3 | `[notes]` — the per-mine line | ⛔ **DELETED** |

✅ **3 → 2, exact.** R5 deliberately does not print the symbol; no rule and no exemplar prints it. **No third
occurrence exists anywhere in either lane.**

### ⚠️ The orchestrator's test — *"confirm the GBNF genuinely makes it unnecessary"* — **it does, and there is a second reason the handoff missed**

1. **The grammar covers the second sentence.** `SiegeAssistantGrammar.cpp:510-524`: the `where` rule is **GENERATED**
   — `Alternatives` is built by looping the canonicalised `PlaceNames` and joining them as an alternation. An
   invented per-mine symbol **cannot be sampled**, full stop. ✅ The deleted clause bought a guarantee the sampler
   gives for free.
2. ✅ **AND THE FIRST SENTENCE WAS A RESTATEMENT OF A LINE THAT SURVIVES.** The deleted note opened *"nearest_mine
   already means the best mine for the player right now"*; the places block prints **`nearest_mine = the best gold
   mine for the player now`** and is untouched. **The two say the same thing in near-identical words.** ⇒ the
   deletion removes a duplicate, not a fact.

⇒ ✅ **RULED: no routing bug is traded for a validity bug. The deletion is correct on both halves.** The residual
worry I checked specifically — *could dropping "there is no per-mine symbol" pull the model toward a `which_place`
question on a mine row, breaking DEV-23?* — **is closed by (2):** the surviving places-block line already tells the
model the symbol means the best mine **now**, which is the whole content of the removed reassurance.

✅ **I also endorse the call NOT to delete the alias row** even though it would take the attractor to 1. DEV-23 is a
passing Execute row that resolves through it, and **trading a currently-passing row for a possible gain while the
categorical half of the gate is the binding constraint is the wrong direction.** Correct EV read.

⛔ **CARRIED VERBATIM AND STILL LAW:** `gold` must NOT be re-added to `nearest_mine` in `DA_AssistantVocabulary`
(TASK-421).

---

## L5. ✅ THE EXEMPLAR'S MISSING VERB — **CONFIRMED. The comment was describing a lesson the line did not teach.**

Rung 1's exemplar was `order: <noun> to the middle` — **a bare noun phrase with no verb**, while its own comment
claimed it taught *"a perfectly parseable remainder does not rescue an impossible subject."* ✅ **There was no
remainder to rescue.** The frame defect is real and it is exactly the kind of thing a comment audit finds and a
score never will.

- ✅ **The added verb does not create a new collision** (§L1) and does not disturb the block's design: **noun frozen,
  destination unchanged, position unchanged (#4 of 7), output `{"ask":"unsupported"}` unchanged ⇒ still §9c
  seam-free** (names no kind), **still a bare plural** (so the refusal must still beat the determiner rule), **two
  minimal pairs intact, refusals still in the middle, block still ends on a command, still 5 of 7 commands.** All
  re-verified by reading the block, not from the handoff.
- ✅ **The pedagogical argument holds:** `send` was the frame of exactly one exemplar and that exemplar is a
  **command**, so every send-framed sentence had one shape to inherit — which is what the §L3 slot-fill looks like.
  Putting a **refusal** in the dominant frame makes the unit word the discriminator instead of the frame, and
  **R1's antecedent is exactly that discrimination.** Rule and exemplar now agree.
- ⛔ **THE NOUN STAYS FROZEN.** Re-authoring it needs a fresh absence certificate from this pass. I am not issuing
  one and none is needed.

---

## L6. ✅ MECHANICS — mirror, arithmetic, anchors, zones, ASCII, grammar legality

### L6a. Both lanes byte-identical at **5116** — rebuilt from source, block by block

I compared `SiegeAssistantSnapshot.cpp:624-1040` against `SiegeLlamaSpike.cpp:224-354` **by reading both in full**,
rendering the shipped lane's two computed blocks by hand: the `PlaceVocabulary` loop (`:693-696`, table at `:52-61`,
**7 entries, pinned order, unchanged**) and `BuildSynonymTable()`'s sort/de-dup/canonical-removal over all 27 rows.
**Every block matches, in order: header · schema(command) · schema(question) · intents · places(7) · rules(4+5) ·
synonyms(13 units / 7 places / 7 intents / 4 notes) · examples(7) · wrappers.** ✅ **MIRROR HOLDS.**

⚠️ **WARN-5 IS UNCHANGED and must not be read as discharged:** no command prints the **shipped** `BuildZoneA`, so
`zoneA_chars=5116` at the re-run measures **the spike lane only**. The shipped lane's byte count remains a
reading-level claim.

### L6b. ✅ The arithmetic — **RE-DERIVED BY HAND from TASK-431's MEASURED 5108, not accepted**

| line | loop 1 | **my count, loop 2** | delta |
|---|---|---|---|
| R1 refusal | 112 | **121** | **+9** ✅ |
| R2 economy | 83 | **83** | **0 — BYTE-FROZEN, verified character-for-character** ✅ |
| R3 selection | 60 | **107** | **+47** ✅ |
| R4 counts | 75 | **88** | **+13** ✅ |
| R5 place (new) | — | **111** | **+111** ✅ |
| refusal exemplar `order:` line | 32 | **37** | **+5** ✅ |
| `[notes]` per-mine line | 96 | **0** | **−96** ✅ |
| `[notes]` unit-kind sentence | 81 | **0** | **−81** ✅ |
| | | **net** | **+8** ✅ |

`9 + 0 + 47 + 13 + 111 + 5 − 96 − 81 = 8`. **5108 (MEASURED at TASK-431) + 8 = 5116.** ✅ **Exact to the byte on
every row.** ✅ **R2 is byte-frozen AND position-frozen** — still the second ladder rule, immediately after R1,
exactly as at loop 1 (`:776` R1 → `:785` R2 → `:831` R3 → `:855` R4 → `:883` R5).

**Tokens:** `1356 + 8/3.767 ≈ 1358` against **1389** ⇒ ~31 slack. ✅ Arithmetic correct, ✅ correctly labelled
**DERIVED**, ✅ correctly anchored on TASK-431's **measured** ratio rather than a blend, ✅ the *"not linear in
chars"* caveat and the 1352–1362 band are the right honesty. **No contingency cut is owed.** ⚠️ **1358 is DERIVED
and is not a pass — read the printed `zoneA_tok~=` before scoring, as always.**

### L6c. ✅ `SiegeLlamaSpike.cpp` anchors — **5 spot-checks, 5 EXACT**

The file measures **4764 lines** by `rg`'s line count, not the 4765 the handoff states — **a counting-convention
difference (trailing newline), NOT a shift**, and I proved it the only way that matters: **by resolving anchors on
both sides of `AppendZoneA`.**

| anchor | expected | result |
|---|---|---|
| `:142` | `SpikeHardTimeoutSeconds = 10.0` | ✅ **EXACT** |
| `:572-621` | `AppendZoneB` / `AppendZoneC` | ✅ **EXACT** |
| `:3191-3197` | `DefaultThreadCount()` = `Max(1, Cores-2)` | ✅ **EXACT** |
| `:4019-4026` | the eval row line + `ROW_DID_NOT_COMPLETE` | ✅ **EXACT** |
| `:4637` | the t0 tripwire (`68` / `887`) | ✅ **EXACT** |

⇒ **Anchors at 3191, 4025 and 4637 all sit BELOW `AppendZoneA` (`:220-354`) and all resolve exactly ⇒ the net line
change inside `AppendZoneA` is PROVEN ZERO.** ✅ **Every `file:line` anchor `qa/TASK-430.md` cites for TASK-429 still
resolves. TASK-429's `qa-passed` is undisturbed and not one byte of its work was touched.**

### L6d. ✅ Zone B / Zone C untouched ⇒ the `68` / `887` tripwire must still read

`AppendZoneB` (`:574-579`) and `AppendZoneC` (`:585-621`) are **byte-identical to the versions I derived 68 and 887
from at gate 1** — same format strings, same key order, same fixture reads. `SpikeFixtureT0` untouched.
`PlaceVocabulary` untouched (7 entries, pinned order) so Zone C's `places:` line cannot have moved. **Every loop-2
edit landed inside `BuildZoneA`, inside `AppendZoneA`, or inside the vocabulary's `[notes]`.** ✅ **The tripwire at
`:4637` must print `zoneB_chars=68 zoneC_chars=887`. If it does not, something outside this task's declared surface
moved and the run must stop.**

### L6e. ✅ ASCII · comment trap · wire shape · GC/UE surface

- ✅ **ASCII clean:** a targeted sweep for non-ASCII bytes on **every prompt-emitting line** (`Out +=` / `Out.Appendf`
  / `Table += TEXT`) across the whole repo returns **zero matches**. R5's `-` is a plain hyphen-minus. The `⚠️`/`⛔`
  glyphs are **comments-only**, matching each file's practice. **Char count still equals byte count** — the property
  the whole budget measurement rests on (`SiegeAssistantSnapshot.cpp:41-44`).
- ✅ **Comment trap:** no literal `*/` inside any `//` comment in any `Siege*.cpp`. The new ~90 lines of rationale in
  `BuildZoneA` and the ~35 in the vocabulary are all line comments.
- ✅ **NO NEW WIRE SHAPE.** The only changed few-shot line is an `order:` line; **all seven JSON output lines are
  byte-identical to what TASK-431 parsed with `parse_failures = 0` under a non-NULL grammar sampler.** The only JSON
  in the changed rules is `{"ask":"unsupported"}`, already present at rung 1 and already parsed. ⇒ **loop 1's clean
  first parse must remain clean; `parse_failures = 0` is still the acceptance evidence and must be quoted.**
- ✅ **No UE surface change:** no new `UPROPERTY`, no member added, no signature changed, `SiegeAssistantSnapshot.h`
  unmodified, no new GC surface, no per-tick work, no deprecated UE 5.8 API. Prompt literals and comments only.
- ✅ **Hard prohibitions re-checked:** `mage`/`caster`/`spellcaster` still unaliased (no unit alias contains "mage"
  as a substring) · **corpus CSVs untouched** (headers byte-identical, 25/15/15 rows, Ids sequential) · grammar
  untouched · `militiamob` / `own_castle` correct in both lanes · place set §9a intact (7, pinned) · JSON key
  `at_least` unchanged · `MaxSnapshotChars` 1085 · `ZoneBCharReserve` 192 · `MaxRosterKinds` 8 ·
  `SiegeAssistantMaxSelectionKinds` 3 · `ContextTokens` 2048 · t0/t1 fixtures intact.

---

## L7. ⚠️ REGRESSION WATCH — including the one the dispatch made non-negotiable

### ⚖️ **THE CARD-PLAY / GOLD REFUSE ROW: STRUCTURALLY PROTECTED. I ruled this first and separately.**

Jonathan's standing ruling is the one class that landed, and *"a pass that fixes two rows and breaks the gold row is
a net loss"* is correct. **Three independent protections, all verified in the source:**

1. ✅ **R2 is byte-frozen AND position-frozen** (L6b). The rule that did the work is untouched in both bytes and rank.
2. ✅ **Few-shot #5 (`get two more pikemen with our gold` → `{"ask":"unsupported"}`) is untouched**, and it is the
   third teaching site.
3. ✅ **The `[notes]` deletion cannot reach these rows.** The deleted sentence's antecedent was *a unit kind not
   listed in the state block* — **both economy rows name kinds that ARE listed**, so that route never applied to
   them. Their pass runs entirely through R2. ⇒ **the deletion is not on their path.**

### ⚠️ WARN-L2-1 — `SiegeAssistantSnapshot.cpp:831` vs `:785` — **R3's antecedent OVERLAPS R2's on exactly the two economy rows.** Top watch item, and the handoff's watch list misses it.

Both economy rows **name a listed unit kind**. So on those sentences **both antecedents are true**: R2 says *refuse*,
R3 says *the intent is send, guard, ambush or follow*. **R3 is the only new rule whose antecedent fires on the one
class that is currently working.**

**Why I rule it a WARN and not a BLOCKER — and I want the reasoning on the record because the dispatch put weight here:**

- **R3's consequent is a constraint on the `intent` FIELD, not an instruction to emit a command.** A question has no
  `intent` field, so a model that has already decided to refuse never reaches R3's constraint. R2 names an explicit
  **output**; R3 names none.
- **The block already establishes that a question is an alternative to an intent entirely** — `- If the order is not
  one of the seven intents, return a question instead of guessing.` (pre-existing, untouched).
- ⚖️ **Decisively: few-shot #5 is a worked example of this exact precedence.** It **names a listed kind** in an
  economy frame and **still outputs `{"ask":"unsupported"}`**. The prompt therefore resolves R3-vs-R2 in R2's favour
  **on the very sentence shape at issue**, with a demonstration rather than a rule — and that demonstration was
  measured landing.
- **The failure mode is loud, not silent.** The gate reports all three Refuse rows individually and half (ii) is
  categorical, so a regression here **fails the gate visibly**, spends no seal, and costs the same measurement a
  blocked loop would cost anyway.

⇒ ⚠️ **PRE-REGISTERED DIAGNOSIS, so the next loop is not spent discovering it:** if **DEV-09 or DEV-10 flips to a
command**, the cause is R3's antecedent, **not** R2. ⛔ **Do not touch R2 — it is byte-frozen.** The fix is one clause
scoping R3 to the command branch (e.g. *"When you give a command and the player names units, ..."*), ~+25 chars,
well inside 31 tokens of slack. **Recording it now converts a possible loop 3 into a mechanical edit.**

### ⚠️ WARN-L2-2 — `:776` vs `SiegeAssistantVocabulary.cpp:224` — R1's sharpened antecedent now fires on the ambiguous-caster word, which the first `[notes]` line routes elsewhere

R1's antecedent moved from an abstraction to a **testable membership test against `[FORCES]`**, which is the whole
improvement — **and it therefore fires on a player word that is deliberately not a listed kind and is deliberately
routed to a different ask code by the untouched first `[notes]` line.** Two routes, both questions, one asserted-field
row. Loop 2 also **removed one competing question route** (§L2.5), which pushes the other way. **Net direction:
unknown; exposure: real.**

- **Not a blocker:** it is a prediction about model behaviour, it is instrumented, and the semantically *correct*
  answer on that row is arguably a question anyway — a known §11 overloaded-empty-cell limitation, not a model defect.
- ⚠️ **THE FINGERPRINT IS ALREADY BUILT AND IT IS THE SAME ONE:** if **`clarify_rows_passed_by_question` comes back
  non-zero**, or **STRICT diverges below LENIENT**, that is refusal over-firing. Both were `0` and identical at
  TASK-431. **DEV-06 is the first row to inspect; DEV-22 the second.**

### ⚠️ WARN-L2-3 — `:883` — R5 enumerates head nouns and names only three narrowing words

R5 lists four head nouns and says *"near, nearest and far only say which one."* **`our` / `their` / `enemy` are not
named**, yet they are the sole discriminator between the two castle symbols, which more corpus rows ride on than any
other place pair. A model reading the enumeration as **exhaustive** could wobble there. Two smaller edges of the same
shape: R5's noun for the centre is `centre` while the dominant player surface form is `middle`, and few-shot #1's
narrowing phrase (`on our side`) is not one of R5's three words. **All are covered by the alias table**, which is why
this is a WARN and not a blocker. ⚠️ **If a currently-passing `own_castle` / `enemy_castle` row regresses, R5's
enumeration is the first suspect** — the cheap fix is to add the two words, not to delete the rule.

### ⚠️ The three rows the dispatch named — read against the source

| row | my read |
|---|---|
| **DEV-22** (highest risk per the handoff) | ✅ **Lower risk than the handoff thinks.** Its unit is a listed kind so **R1 does not fire**; its intent is already a unit-taking one so **R3 reinforces rather than threatens**; **R4's reorder helps** (the retired wording said *bare* plural, the new one drops "bare", which is the arm this row needs); and its place field is **not asserted**, so R5 cannot cost it a point. **Exposure is on `who` only, and every changed rule pushes `who` the right way.** |
| **DEV-23** | ✅ **Protected as argued.** R5 is **noun-first**, so the modifier cannot displace the head noun; the alias row and the places-block definition are both deliberately kept; the count is explicit so **R4's antecedent is false**; `guard` is inside R3's allowed set. **Four independent reasons it holds.** |
| **DEV-06** | ⚠️ **The real exposure — see WARN-L2-2.** The handoff's argument (deleting a `which_unit` route pushes it the safe way) is **half the story**; R1's sharpened antecedent pushes the other way and the handoff does not weigh it. **Watch the two fingerprints.** |

### ⚠️ WARN-L2-4 — `:768-775` — R1 CONCENTRATES the `MaxRosterKinds = 8` shipped-truncation hazard into a rule

R1 points the model at `[FORCES]`, which the **shipped** snapshot truncates to 8 kinds while the **spike fixture**
prints all 13. ⇒ **on a >8-kind board the shipped assistant can refuse a kind that is alive and merely collapsed into
`other_kinds`.** ✅ **Declared in-source and in the handoff rather than discovered — credit where due**, and **no
measured number in this wave is affected** (the fixture never truncates). **It is board ruling 7 biting a second
time and it needs TASK-416's constant, not a prompt edit.** ⚠️ **Recorded as a SHIPPED-CORRECTNESS defect that is now
one rung worse than it was, and it should be TASKED rather than carried in a ledger indefinitely.**

### ⚠️ WARN-L2-5 (process, not code) — the retired word's stem is now written into a pipeline artifact

`handoffs/TASK-428-programmer.md` prints the retired stem verbatim in two sweep tables. **The source tree is clean of
it, nothing is keyed to it, and no exemplar/alias/rule/note mentions it ⇒ this is NOT a fitting defect and changes no
byte.** But combined with `qa/TASK-430.md`'s line anchors and Git history it makes the loop-1 pairing recoverable from
the repo. **The programmer's R1d call — keep it out of source comments — was right and I uphold it; the handoff went
one step further than that call intended.** ⇒ **Not actioned. Recorded for §12a as a generation-3 process input:
a line-anchored review plus version control leaks the retired token by construction, and the recovery is a
convention, not a code change.**

---

## L8. ✅ NIT-5 — CLOSED, and closed better than asked

`SiegeAssistantSnapshot.cpp:614-620` now states **5108 as the MEASURED figure at TASK-431**, **names the stale 5111
as the defect and explains why it was stale** (the pre-re-gate draft, before the exemplar noun shortened by 3), and
**records 5116 for this loop.** ✅ Comment-only, emits no byte. **A comment that explains why it was wrong is worth
more than one that is merely right.**

## L8b. NIT ledger (loop 2)

- **[NIT-L2-1]** `SiegeAssistantVocabulary.cpp:244-282` — ~38 lines of deletion rationale now sit above a **one-line**
  emitted string. Correct and valuable (a diff reader would otherwise read the deletions as gaps), but the file's
  comment-to-output ratio in this block is now extreme. **Free to leave.**
- **[NIT-L2-2]** `SiegeAssistantVocabulary.cpp:209` — `Table.Reserve(2048)` **still** undersized against a ~2.4 KB
  rendered table (gate-1 NIT-2, unchanged, one realloc). The two `6144` bumps landed; this one did not. **Free to fix
  on any next touch, emits no different byte.**
- **[NIT-L2-3]** WARN-5 unchanged: `zoneA_chars=5116` at the re-run proves the **spike** lane only. The one automation
  test asserting `USiegeAssistantSnapshot::BuildZoneA(V)` equals `AppendZoneA`'s output would convert this wave's
  load-bearing claim from *reviewed* to *tested*. **Still recommended for TASK-423.**

---

## L9. ⚖️⚖️ THE RULING THE DISPATCH ASKED FOR: is the prompt rung exhausted?

**I am the party whose predictions were inverted. I am not going to hide behind "it is a lottery."**

### L9a. ⛔ I reject the premise that class-level prediction is at chance

The programmer's deciding argument is *"QA rated (c) strongest → failed twice; (a) weakest → still broken; (b)
graded hardest → the only winner ⇒ prediction is at or near chance."* **That inference is wrong, and getting it
right is the most useful thing this gate can contribute.**

**I was not predicting at random. I was ranking the wrong variable.** My §2 ranked each class on **generalisation
quality** — *does the rule state a class rather than a string?* On that variable my ranking was defensible and is
still defensible. But look at what actually separated the four outcomes:

| class | text ADDED | competing instruction REMOVED | outcome |
|---|---|---|---|
| **(b) economy** | rule + exemplar | ✅ **YES** — the `gold → nearest_mine` alias | ✅ **LANDED** |
| (a) refusal | rule + exemplar | ❌ no | ⛔ failed |
| (c) selection | rule + exemplar | ❌ **no — and 4 competing lines were left standing** | ⛔ failed twice |
| (d) determiner | rule + exemplar | ❌ no | ⛔ failed |

⇒ ⚖️ **ONE variable fits 4 of 4 outcomes, and it is not the one I ranked on.** The predictor was **whether a
competing mechanism was deleted**, not whether the teaching was well-generalised. **Class-level prediction was
correct-under-the-wrong-model, which is a completely different thing from chance** — and it is falsifiable, which
chance is not.

### L9b. ⚖️ THE RULING

**Is prompt-level work still a rational lever? — YES, for exactly ONE more measurement, and no further.**

- ⛔ **THE ADDITIVE LEVER IS DEAD AND SHOULD BE DECLARED DEAD IN WRITING.** Three independent measurements kill it:
  **+797 chars (18.5 % of Zone A) bought one row**; a **near-verbatim few-shot plus a pinned alias aimed at the same
  row did not move it**; and an added rule **lost 4-to-1 to text already in the prompt.** ⇒ **No future task may
  propose "add a few-shot" or "add an alias" as a fix on this model without new evidence. That is a finding, not a
  mood.**
- ✅ **THE SUBTRACTIVE LEVER IS UNTESTED, AND LOOP 2 IS ITS CLEAN TEST.** Loop 2 is **three deletions/replacements
  and net +8 chars**: two `[notes]` lines deleted, one rule replaced rather than joined, one rule re-aimed at an
  antecedent instead of a consequent. **It is the first pass in this wave built on the only variable that fits the
  data**, it costs ~2 tokens of a measured 31-token slack, and it is **falsifiable in a single run**.
- ⇒ ⚖️ **MEASURE LOOP 2. It is an experiment, not a lottery ticket** — and that distinction is precisely what §L9a
  buys.

### L9c. ⛔ AND THE STOPPING RULE, SHARPER THAN THE PROGRAMMER'S

The programmer says *"loop 3 should not happen."* **I agree, and I am making the condition testable rather than a
preference:**

> ⛔ **LOOP 3 IS REFUSED UNLESS LOOP 2'S RUN ITSELF PRODUCES A NEW COUNTABLE MECHANISM** — of the kind loop 1
> produced twice (the **4-to-1 instruction count** and the **character-for-character slot-fill substring**). Those
> were *countable facts read off the artifacts*, not hypotheses about the model. **A proposed loop 3 that cannot
> point at one is a lottery ticket and I will fail it at the gate.**

**And the honest finding if loop 2 lands under 22:** **prompt-level tuning on this model tops out around 20–21/25
against a gate of 22.** ⚠️ **That would be a real, publishable result, not a defeat** — it is bought with three
measured mechanisms and it tells Jonathan exactly what his reserved rung is for. **A truthful "this rung is done" is
worth more than a third loop, and I am saying so as the party with the most to lose from saying it.**

⚠️ **ONE READING RULE FOR THE RE-RUN, AND IT MATTERS MORE THAN THE TOTAL:** ⛔ **REPORT WHICH ROWS FLIPPED, NOT JUST
THE SCORE.** The rows now carry unequal information: **DEV-01 has three reasons to flip** (few-shot #1's
near-paraphrase — WARN-2 — plus the pinned alias plus R5), so **its flip proves nothing about R5**; **DEV-07 and
DEV-16 flipping is the clean test of the subtraction thesis**; **DEV-04 is simultaneously the categorical
requirement and the lowest-confidence row.** ⇒ **A 22 carried by DEV-01 is materially weaker than a 22 carried by
DEV-07 + DEV-16, and the two must not be reported as the same number.**

---

## Notes for build-master (TASK-431 re-run)

✅ **YOU MAY COMPILE AND YOU MAY RE-RUN THE DEV GATE.** No bar. **Everything in `qa/TASK-430.md`'s "Notes for
build-master" (items 1–9) still applies**; these are the loop-2 additions.

1. ⚖️ **Read `zoneA_tok~=` before scoring, as always.** Expect **`zoneA_chars=5116`** and **`zoneA_tok~≈1352–1362`**
   against the **1389** ceiling. ⚠️ **If `zoneA_chars` is not exactly 5116, STOP** — the lanes have drifted from
   what this gate reviewed, and the number would be taken on a prompt nobody cleared. **1358 is DERIVED and is not
   a pass.**
2. ✅ **The t0 tripwire must still print `zoneB_chars=68 zoneC_chars=887`.** I re-verified both builders are
   byte-identical to the versions those figures were derived from. Run `SpikePrompt` with **no `order=` override**
   (the in-code check only arms for the default order line).
3. ⛔ **DO NOT RE-RUN THE CPU MATRIX.** §5 of your own handoff is final for this wave (board ruling, and tuning
   cannot move a decode-rate question).
4. ⚠️ **`parse_failures` must stay `0` and the grammar sampler must stay non-NULL.** No new wire shape was
   introduced — the only changed few-shot line is an `order:` line — so a non-zero count would be a **regression,
   not a new-content cost**, and it must be reported as such.
5. ⚠️ **THE TWO OVER-REFUSAL FINGERPRINTS ARE THE SAME TWO AND THEY ARE NOW LOAD-BEARING:**
   **`clarify_rows_passed_by_question` non-zero**, or **STRICT below LENIENT**. Both were `0`/identical at TASK-431.
   ⇒ **If either fires, look at DEV-06 first and DEV-22 second (WARN-L2-2), and say so explicitly** — do not let it
   be absorbed into the total.
6. ⛔ **IF DEV-09 OR DEV-10 FLIPS TO A COMMAND, THE DIAGNOSIS IS PRE-REGISTERED (WARN-L2-1): R3's antecedent, NOT
   R2.** R2 is **byte-frozen** and must not be touched. **Name the row explicitly in the handoff** — that is
   Jonathan's standing ruling and the one class that landed.
7. ⛔ **REPORT WHICH ROWS FLIPPED, WITH THE §L9c INFORMATION WEIGHTING** — DEV-01's flip is uninformative (three
   causes), DEV-07 + DEV-16 are the clean test, DEV-04 is the categorical gate. **A bare total is not a report this
   wave can use.**
8. ⚠️ **NOTE-3 STILL STANDS** — the tree carries TASK-428 + TASK-429 + the landed TASK-433 fix pass. **TASK-434 must
   not attribute the third task's edits to this wave's commit.** ⚠️ **NOTE-4 stands: paste no per-row eval line from
   any holdout run into any artifact.** ⚠️ **NOTE-5 stands: a `ROW_DID_NOT_COMPLETE` row is not a comprehension
   failure — do not route a ladder loop against a split containing them.**
9. ⚠️ **IF THE GATE FAILS AGAIN, THE ROUTE IS NOT AUTOMATICALLY LOOP 3.** Per §L9c, a loop 3 requires a **new
   countable mechanism** from this run's artifacts. **Absent one, escalate to Jonathan with the §L9b finding — the
   next rung is his and he reserved it.**
