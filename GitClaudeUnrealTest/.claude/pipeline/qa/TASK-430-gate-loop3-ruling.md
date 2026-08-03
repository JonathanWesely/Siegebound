⚠️ **FILING NOTE — READ FIRST.** This file is the `## Loop-3 ruling` section of `qa/TASK-430-gate-loop2.md`. **I have
`Write` but no `Edit` in this run**, and appending would mean re-`Write`ing 503 lines of an authoritative measured
record from a transcription — a corruption hazard I will not take on the artifact that carries this wave's gate.
**Orchestrator: concatenate this file verbatim onto the foot of `qa/TASK-430-gate-loop2.md` and delete it, or leave it
as the named sibling.** Its first heading below is `## Loop-3 ruling`, so a literal `cat` produces exactly what was
asked. This is the same precedent the loop-2 section itself was filed under, relative to `qa/TASK-430.md`.

---

## Loop-3 ruling

**Date:** 2026-08-03 · **Scope:** the §L9c stopping condition ONLY. **Input:** `handoffs/TASK-431-buildmaster.md` §LOOP 2.

⚠️ **THIS SECTION CHANGES NO VERDICT.** The loop-2 gate stands exactly as written: **PASS · 0 BLOCKERS · 5 WARN · 3 NIT**,
**TASK-428 remains `qa-passed`**, and **the loop-2 delta is safe and must NOT be reverted**. What follows rules on one
question: **is the §L9c condition for a loop 3 met, or is the prompt rung exhausted?**

### ⚖️ THE RULING, FIRST

> ⛔ **LOOP 3 IS REFUSED. THE PROMPT RUNG IS EXHAUSTED.**
> ⇒ **ESCALATE TO JONATHAN with the §L9b finding. The next rung is his and he reserved it.**
> ⛔ **HOLDOUT 2 STAYS SEALED. TASK-432 STAYS BLOCKED.** Nothing in this ruling authorises spending it; only a gate PASS
> or Jonathan's own instruction can, and neither exists.

**I am ruling this myself, on the artifacts.** Build-master argued against its own opportunity to run again and I weigh
that as good faith — but I do not defer to it, and §1–§4 below are my own adjudication, including a steelman of the
loop 3 it declined to ask for.

---

### 1. THE §L9c BAR, APPLIED TO THE THREE COUNTABLE FACTS

My own bar, quoted from §L9c so it is judged as written and not as remembered:

> ⛔ **LOOP 3 IS REFUSED UNLESS LOOP 2'S RUN ITSELF PRODUCES A NEW COUNTABLE MECHANISM** — of the kind loop 1 produced
> twice (the **4-to-1 instruction count** and the **character-for-character slot-fill substring**). Those were
> *countable facts read off the artifacts*, not hypotheses about the model. **A proposed loop 3 that cannot point at one
> is a lottery ticket and I will fail it at the gate.**

The word doing the work is **mechanism**. Both loop-1 exemplars were (a) countable off the artifact **and** (b) a causal
route that a specific edit could open or close, with a predicted direction. The 4-to-1 count named *delete the
competitor*; the slot-fill substring named *forbid writing a kind the player did not name*. Judged on both halves:

| # | fact from loop 2 | countable? | names a route an edit can act on? | verdict |
|---|---|---|---|---|
| 1 | **23 of 25 rows byte-identical to loop 1** (mechanical string diff) | ✅ yes | ⛔ **no** — it is the aggregate measurement that edits of this class do not propagate to output at all | ⛔ **does not qualify** |
| 2 | **The DEV-04 slot-fill route is closed** (`{"kind":"sapper"` = 0 occurrences in rendered Zone A) **and the row still fails** | ✅ yes | ⛔ **no** — the route is already shut; there is nothing left to close | ⛔ **does not qualify** |
| 3 | **R3 present + antecedent satisfied + competitor deleted ⇒ zero byte change on DEV-07** | ✅ yes | ⛔ **no** — a mechanism measured present with zero effect | ⛔ **does not qualify** |

⇒ **Build-master's reading is correct and I adopt it on my own analysis: none of the three names an edit.** But the
adjudication is sharper than "three near-misses", and this is the part that decides it:

> ⚖️ **FACTS 2 AND 3 ARE THE MEASURED FALSIFICATIONS OF THE TWO FACTS THAT DID QUALIFY AT LOOP 1.** Fact 2 retires the
> slot-fill mechanism; fact 3 retires the 4-to-1 competing-instruction mechanism. **The ledger of qualifying mechanisms
> did not fail to grow from 2 — it went from 2 to 0.**

**The condition is not missed narrowly. It is missed in the wrong direction.** A countable fact that shows a lever is
inert does not fund another pull of it; it retires it — and this run retired both levers that funded loop 2.

---

### 2. THE STEELMAN — because a refusal that never built the case is not a ruling

**The strongest loop 3 available**, constructed from my own §L2b honest limit: *the four `who="none"` lines are still
printed*. So strike their consequents — delete `who and where are "none"` from the three intent-definition lines
(`SiegeAssistantSnapshot.cpp:687-689` + the vocabulary line at `:243`), the deletion loop 2 did **not** perform.
Subtractive, cheap, aimed at the two-row class. **It fails on three grounds, and the third is arithmetic:**

**(a) It is funded by a hypothesis about the model, which §L9c excludes by name.** The claim underneath it — *the model
latches intent off the verb before evaluating an antecedent* — is an **inference** from fact 3, not a reading off an
artifact. DEV-07's `charge` and DEV-16's `back` are verbs in the input sentence; the four printed lines are not what
supplied them. **The edit does not act on the thing the measurement identified.**

**(b) It spends this wave's only durable asset.** Those four lines are the sole definition of the army-wide branch's
shape. Few-shot #7, DEV-25 and DEV-13 ride it (§L2b corroboration 1). **Zero regressions across two loops** is the one
outcome this wave can put in front of Jonathan without a caveat; this edit stakes it on a class that has now failed
twice with a mechanism I certified structurally correct.

**(c) ⚖️ DECISIVELY — A PERFECT LOOP 3 ON THE HIGHEST-VALUE LEAD CANNOT OPEN THE GATE.** The arithmetic:

```
20/25 now  +  DEV-07  +  DEV-16   =   22/25  =  88.0 %
half (i)  MET, exactly, with zero margin  -- and by my own WARN-2 reading rule a 22 is
          "AT THE GATE WITH THE INFORMATIVE ROWS UNPROVEN", not "passed with room"
half (ii) DEV-04 UNTOUCHED BY THIS EDIT  =>  CATEGORICAL FAIL  =>  GATE NOT MET
                                         =>  HOLDOUT STAYS SEALED  =>  NOTHING UNLOCKS
```

⇒ **A loop 3 that succeeds completely on the two rows with the best measured lead changes no state.** To be worth
running at all it must *also* fix DEV-04 — and DEV-04 is the emptiest row in the wave:

- **four prompt-level attempts**: a rule naming the behaviour · a rule naming the list (`[FORCES]` membership) · a
  purpose-built refusal exemplar · an explicit prohibition on writing a kind the player did not name;
- **two distinct routes measured shut on separate loops**: the semantic-neighbour route and the verbatim slot-fill route;
- **destination oscillating `sapper` → `sorcerer` → `sapper` while the behaviour is invariant** — the attractor is the
  substitution *behaviour*, and nothing in three runs has touched it;
- ⇒ **there is no candidate edit for DEV-04 with a measured mechanism behind it. Not a weak one. None.**

⇒ ⛔ **The steelman is a lottery ticket by my own definition, and I fail it at the gate as promised.**

---

### 3. ⚖️ THE SUBTRACTION THESIS — RULED SQUARELY. **FALSIFIED. I WITHDRAW IT.**

§L9a rejected *"class-level prediction is at chance"* and replaced it with a better variable: **was a competing
mechanism deleted?** — which fit **4 of 4** loop-1 outcomes. Loop 2 was built as its clean test, and the test was clean:
deletion-based, net **+8 chars**, mechanism **verified physically present in the rendered prompt**, competitor
**deleted rather than out-shouted**, exemplar using the **identical noun phrase**, `parse_failures=0`. **It produced
zero byte change on both rows it was designed for.**

⇒ ⛔ **A variable that fits 4 of 4 retrospectively and 0 of 2 prospectively was an overfit to four data points, not a
predictor. On the record: my §L9a replacement model is measured wrong, and I do not get to keep it because it was more
sophisticated than the thing it replaced.**

⚠️ **AND THE SHARPER READING, WHICH IS THE ACTUAL FINDING — the one row that flipped was flipped by the lever I
declared DEAD.** `DEV-11` moved on **R4's new antecedent clause** (*"No number said:"*), **+13 chars, additive**, on the
**knowingly conceded** row. So loop 2 does not read *"subtractive dead too"*. It reads:

> ⚖️ **BOTH LEVERS MOVE ABOUT ONE ROW PER WAVE, AND NEITHER STEERS WHICH ROW.**
> **Loop 1:** +797 chars, additive ⇒ **1 row**, and it *was* a targeted row.
> **Loop 2:** +8 chars net, subtractive ⇒ **1 row**, and it was the row that had been **written off**.
> **The rate is constant across a 100× difference in spend. The steering went from present to absent.**

This subsumes both of my earlier models and explains why every class-ranking exercise (§2, §L9a) kept inverting: **I was
ranking *which* rows would move, and which-rows is the property that is not under prompt control on this model. Rate is
under control; selection is not.**

⇒ ⛔ **RECORDED AS LAW FOR ANY FUTURE TASK ON THIS MODEL: no prompt-level task may be justified by naming the rows it
will fix.** The measured expected yield is ~1 unsteerable row per wave, against a 2-row deficit **plus** a categorical
row that has never once moved. That is not a plan, and a task proposing it should be failed at the gate.

---

### 4. ⚠️ THE DERIVATION'S ERROR DIRECTION FLIPPED — RULED, AND IT PRODUCED THE ONE REAL NEW MECHANISM OF THIS RUN

**The data, both loops:**

| loop | derived | measured | error | direction | on a delta of |
|---|---|---|---|---|---|
| 1 | 1371 | **1356** | **−15** | ✅ **safe** (over-predicted cost) | **+797 chars** |
| 2 | 1358 | **1362** | **+4** | ⛔ **UNSAFE** (under-predicted cost) | **+8 chars** |

**(a) ⛔ NOTHING IN THIS WAVE IS INVALIDATED, and I want that stated before anything else.** No decision in either loop
rested on a derived figure. The standing rule — *read the printed `zoneA_tok~=` before scoring; **DERIVED is not a
pass*** — was honoured both times, and both loops were scored on a **measured** number. **The sign flip is a
confirmation of that rule, not a breach of it.** ✅ **The rule holds and does not change.**

**(b) ✅ WHAT DOES CHANGE: derived headroom may no longer be treated as a floor — and I am the party who came closest to
treating it as one.** At §L6b I quoted **"~31 slack"**. The truth was **27**. My own figure was 4 optimistic, in the
unsafe direction, in the same report that told build-master not to trust it.

**(c) ⚖️ THE MECHANISM, AND IT IS COUNTABLE — the error is NOT proportional to the edit, and here is exactly why:**

```
loop 1 marginal:  +797 chars bought  1356 - 1139 = 217 tokens   =>  3.67 chars/token   (~= whole-prompt 3.77)
loop 2 marginal:  +  8 chars bought  1362 - 1356 =   6 tokens   =>  1.33 chars/token   (2.8x WORSE)
my derivation assumed the whole-prompt ratio 3.767  =>  8/3.767 = 2.1 tokens  =>  1358.  Truth: 1362.
```

⇒ **The whole +4 error is the marginal ratio collapsing from 3.77 to 1.33 on a small edit.** A large addition
re-tokenises mostly itself, so its marginal rate tracks the whole-prompt rate; **a small addition is dominated by
boundary re-tokenisation of text it did not touch**, and the added characters land far denser than the average.
⚠️ **This is the only genuinely new countable mechanism the run produced — and it is a mechanism of the measuring
instrument, not of the model, which is why it does not rescue loop 3.** It discharges into §4(e) instead.

**(d) ⚖️ THE OPERATIONAL RULE I REGISTER:**

1. **A derived `zoneA_tok` may decide whether to *attempt* an edit. It may never certify one.** Unchanged, and
   re-affirmed.
2. **Budget added prompt text at the measured worst marginal rate — ~1.3 chars/token — never at the whole-prompt
   ~3.76.** That is a **2.8× safety factor and it is measured, not assumed.** ⚠️ n = 1 for a small edit and boundary
   effects can push either way, so it is a floor on caution, not a constant.
3. **Observed error band: −15 to +4 tokens, sign unpredictable, on n = 2.** ⛔ **Any edit whose derived headroom is
   under ~20 tokens is UNDECIDED until measured.** Had this loop's 2.8× marginal error occurred on a ~100-char edit it
   would have cost ~19 tokens against 27 of slack — i.e. **the next edit of ordinary size is the one this would have
   bitten.**
4. **Re-fit the band on every future measurement.** Loop 2 already widened it once and reversed its sign.

**(e) ⚖️ THE BEARING ON TASK-423's BUDGET ASSERTION — which does not exist yet, so this is a spec input, not a
correction.** WARN-5 / NIT-L2-3 recommends TASK-423 carry the automation test asserting the shipped
`USiegeAssistantSnapshot::BuildZoneA(V)` equals `AppendZoneA`'s output. **When that task is written:**

- ✅ **The assertion must be on CHARACTERS (equivalently bytes), never on tokens.** Chars are exact, countable
  offline, and the repo's verified ASCII-clean property (`SiegeAssistantSnapshot.cpp:41-44`) makes char count == byte
  count. Both lanes' char counts have now matched a hand-derivation **to the byte, twice** (5108, 5116) — chars are the
  reliable quantity in this system.
- ⛔ **A derived token constant must NOT be baked into any test.** It would be an assertion whose error sign is
  **unknown** — precisely the defect §12c exists to prevent, and the §8 instances-1-and-6 defect class in test form,
  where it is worse because it looks automated.
- ✅ **The token ceiling stays a RUNTIME gate read off the printed `zoneA_tok~=`.** Two artifacts, two different
  quantities: **the test asserts lane equality + an exact char count; the run reads the token figure and stops on it.**

---

### 5. 📌 ALSO RECORDED

**(a) ⚠️ THE NON-ORDERABLE KIND — RECOMMEND IT BE TASKED (manager).** The roster prints
`- sapper: 2 total, 0 orderable, 2 followable`, and loop 2's DEV-04 output is a live `send` order naming **`sapper`**.
So the failure is worse than *"ordered a unit that isn't in the roster"*: **the model emitted an executable order for a
kind the board's own state says cannot be ordered.**

- ⛔ **This is NOT a prompt defect and must not be routed as one.** Four prompt attempts have failed on this row.
- ⛔ **And it is NOT a grammar fix.** §1's central law is *constrain identity hard, leave quantity soft*; **orderability
  is per-match STATE, not identity**, and the sampler cannot know it. Putting it in the grammar introduces the defect §1
  names.
- ⇒ ✅ **It is a missing third layer: the grammar constrains identity, the prompt asks for policy, and only the command
  layer can enforce state. DEV-04 is the proof that the third layer does not exist.**
- **Recommended shape, so the manager can spec it:** the authoritative command path **rejects any order whose
  `who[].kind` is not orderable in the current snapshot**, validated against the same roster data the snapshot prints,
  **before execution, independent of the model**; surfaced as the existing unsupported-ask outcome rather than a silent
  drop; **unit-testable with no model resident.**
- ⚠️ **SCOPE CAVEAT, AND IT MATTERS: this does NOT make DEV-04 pass.** The eval scores the model's emitted JSON, not the
  executed action. **It is a shipped-safety fix, not a route to the gate, and nobody may report it as one.**

**(b) ⚠️ WARN-5 UNCHANGED — NOT DISCHARGED.** `zoneA_chars=5116` proves the **spike** lane only; no command prints the
shipped `BuildZoneA`, so the shipped lane's byte count is **still a reading-level claim**, carried across two loops and
two gates. ⛔ **It must not be read as discharged by the fact that the measurement matched a hand-derivation to the byte
twice — both matches were on the spike lane.** §4(e)'s test is what would close it.

**(c) 📌 CARRIED, UNCHANGED AND STILL LAW:** ⛔ **R2 stays byte- AND position-frozen** (the one class that works, and
WARN-L2-1 was measured not to fire) · ⛔ **R4's counts line must not be reworded again** (its own pre-registered
falsification fired) · ⛔ **`gold` must NOT be re-added to `nearest_mine`** (TASK-421) · ⛔ **the retired noun stays
unused** · ⚠️ **WARN-L2-4** (R1 concentrates the `MaxRosterKinds = 8` shipped-truncation hazard) **still wants
TASK-416's constant and should be TASKED rather than carried in a ledger** · **NIT-L2-1 / NIT-L2-2** free to fix on any
next touch · **NIT-5 closed.**

---

### 6. 🧑 THE FINDING, IN THE FORM JONATHAN WILL READ

> **Prompt-level tuning on this model tops out at 20 out of 25, against a gate of 22. Three waves: 18 → 19 → 20 — one
> row each.**
>
> **The first row cost +797 characters of prompt. The second cost +8.** So **budget is not the constraint** (27 of the
> allowed tokens are still unspent) and **neither is effort** — the cheap wave bought exactly what the expensive one
> did.
>
> **And we cannot aim.** Wave 2 targeted five specific rows, moved **none** of them, and moved a sixth we had already
> written off. **The rate is about one row per wave; which row is not something the prompt controls.**
>
> **The harder fact is the categorical half.** One row — an order for a unit type you do not have — must refuse, and it
> has **never** refused: across four prompt attempts and **two separate routes we measured shut**. The gate cannot be
> passed by a score alone; that row has to stop, and it has not.
>
> **What the work bought, and it is real:**
> - ✅ **Zero regressions across two loops.** Nothing that worked stopped working.
> - ✅ **The gold / card-play refusal is FIXED** — the one that broke your standing ruling that the assistant never
>   spends your gold and never plays your cards. It holds, byte-identically, across both loops.
> - ✅ **The exemplar slot-fill route is CLOSED** — the model no longer copies a unit name straight out of an example.
> - ✅ **The sealed holdout was never opened.** Your one-shot measurement is unspent and still clean for whenever you
>   want it.
>
> ⇒ **The next rung is a bigger model. You reserved that decision and this is the evidence you reserved it for.** ⚠️
> **This is a result, not a defeat** — it is bought with measured mechanisms rather than impressions.
>
> **Two things travelling with it:** the **CPU answer is unchanged** (the fallback works and is merely slow — a tunable
> deadline artifact, ~1.6× outside the latency bar; that is TASK-435's, not this ruling's). And **one shipping defect
> worth a task regardless of which rung you pick:** the assistant emitted an order for a unit type the board itself
> says cannot be ordered — **the command layer should refuse that on its own, no matter what any model says.**

---

### 7. ROUTING CONSEQUENCES

| item | consequence |
|---|---|
| **TASK-428** | ✅ **`qa-passed` STANDS — FINAL for this wave.** The loop-2 delta is **safe, +1 row, 27 tokens unspent ⇒ NO REVERT.** ⛔ **No further Zone-A or vocabulary edits in this wave.** Carries to **TASK-434** unchanged. |
| **TASK-429** | ✅ Undisturbed — `qa-passed` at gate 1, anchors re-resolved at §L6c, its WARN-R2 fix confirmed in the field twice. |
| **TASK-430** | ✅ **DONE.** Gate 1 PASS (re-gate) · loop-2 gate PASS (0 blockers) · this loop-3 ruling. |
| **TASK-431** | ✅ Done, two loops delivered. ⛔ **No loop-3 run is authorised.** ⛔ **CPU matrix stays not-re-run.** |
| **TASK-432** | ⛔ **BLOCKED. HOLDOUT 2 STAYS SEALED AND UNSPENT.** Neither this ruling nor the escalation authorises spending it. |
| **TASK-433 / TASK-434** | ⚠️ **NOTE-3 STANDS** — the tree carries **three** tasks (428 + 429 + the landed TASK-433 fix pass). **TASK-434 must not attribute the third task's edits to this wave's commit.** ⚠️ **NOTE-4 stands: no holdout row in any artifact.** |
| **The ladder** | ⛔ **TERMINATED AT LOOP 2 OF 3, BY THE PRE-REGISTERED RULE — not by exhausting the loop budget.** Per CLAUDE.md routing rule 4 this escalates to the user; per §L9c it escalates **with a finding**, which is the point of having pre-registered the condition. |
| **New work** | 📌 **Two tasks recommended to the manager:** (1) **the command-layer non-orderable-kind rejection** (§5a) — shipped-safety, not a gate route; (2) **the `MaxRosterKinds = 8` truncation constant** (WARN-L2-4), which needs TASK-416's constant, not a prompt edit. |

⚠️ **AND THE ONE THING I WILL NOT DO:** I am not proposing a fifth prompt attempt under another name. **The bar I
pre-registered exists precisely so that the party with the most to lose from saying "this rung is done" says it anyway.
I am that party — my class ranking was inverted at loop 1 and my replacement model was falsified at loop 2 — and the
ruling is the same either way.**
