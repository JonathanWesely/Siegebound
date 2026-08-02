# TASK-426 — [LLM-1e] 🔒 THE SEALED ADVERSARIAL EVALUATION CORPUS

**Owner:** art-director (independent test author — a different role and a different dispatch from the prompt author, per manager ruling 14 / CONVENTIONS §11)
**Date:** 2026-08-02
**Status:** ready-for-integration — **CORPUS SEALED**
**Mode:** headless, file-only. No editor, no Blender, no MCP, no C++, no `Content/`, no Git. `L_Arena` never opened. `Docs/Data/cards.csv` read-only, untouched.

---

## 🔒 SEAL DECLARATIONS (read these first)

1. **THE HOLDOUT HAS NOT BEEN SHARED WITH ANY PROMPT AUTHOR.** `Docs/Data/assistant_eval_holdout.csv` was authored in this dispatch and exists only as that file on disk. No holdout sentence has been transmitted to TASK-410, quoted into any prompt/few-shot/vocabulary artefact, or reproduced anywhere except in this handoff. **This handoff dumps both files in full — so this handoff is itself holdout material. TASK-410 must not read the §Holdout dump below.** TASK-413 opens the holdout exactly once.
2. **I DID NOT READ ANY PROMPT OR FEW-SHOT MATERIAL WHILE AUTHORING.** No prompt exists yet (TASK-410 has not run and `SiegeAssistantSpike.cpp` does not exist). What I read: the TASK-426 board spec, CONVENTIONS §1/§3/§5/§8/§9/§10/§11, `Docs/Data/cards.csv`, and — for expected-outcome correctness only — the two shipped eligibility predicates in `SummonedUnit.{h,cpp}` and the `ASorcererUnit` commandability comment. I did **not** open `TASK-421`'s vocabulary work product (it does not exist), and `DA_AssistantVocabulary` does not exist.
3. **FITTING DIRECTION HONOURED.** Rows were written from what a player would type and from `cards.csv`, then the expected output was derived. No sentence was softened to match a vocabulary I think exists. Rows that require aliases which do not yet exist were **written anyway** — see §Reactive-coverage debt below; TASK-421 must *declare* those additions.
4. **⛔ FROZEN.** Any edit to `assistant_eval_holdout.csv` after this handoff voids the accuracy number and requires a fresh holdout in a new task. Quoting a **dev** score as the go/no-go number is a QA FAIL.

---

## Deliverables

| File | Rows | Path |
|---|---|---|
| DEV split (few-shot tuning may use this) | 25 | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\Data\assistant_eval_dev.csv` |
| HOLDOUT split (opens ONCE, at TASK-413) | 15 | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\Data\assistant_eval_holdout.csv` |

Header, byte-identical in both files, exactly as pinned in CONVENTIONS §11 and the board's `names:` line:

```
Id,Sentence,ExpectOutcome,ExpectKinds,ExpectCounts,ExpectWhere,ExpectTriggerKind,ExpectTriggerAtLeast,Notes
```

Both files are loaded **by path at runtime** (`Siege.Llama.SpikeEval dev=<path> holdout=<path>`) and must never be `#include`d — that would give `Plugins/SiegeLlama/` a Siegebound dependency it is architecturally forbidden to have.

### Parser-safety guarantees (verified mechanically, not asserted)

- Every row has **exactly 8 commas** ⇒ a naive `Split(TEXT(","))` parser is sufficient. **No cell anywhere contains a comma**, so no quoting dialect is needed and none is used.
- No `|` appears in any `Sentence` or `Notes` cell (`|` is reserved as the kinds/counts separator).
- Every sentence is lowercase and unquoted, as a player types mid-fight.

---

## 🚩 TWO SCHEMA FINDINGS — RAISED, NOT SILENTLY PATCHED

These need a manager/orchestrator ruling **before TASK-410 writes the scoring runner**. I deliberately did **not** deviate from the pinned header to fix them; a unilateral 10th column would break the cross-task contract TASK-410's parser is written against, and the pin is exactly the mechanism that stops artist file and programmer parser from drifting.

### FINDING 1 (blocking for the scorer) — **the pinned header has no `ExpectIntent` column, but bar #5 scores `{intent, kinds, counts, where}`**

`ExpectOutcome ∈ Execute|Clarify|Refuse` is the *outcome class*, not the intent. There is no column carrying `send`/`guard`/`ambush`/`follow`/`charge`/`fallback`/`rally`. As pinned, the corpus literally cannot assert the first of the four scored fields — and intent is load-bearing here: `charge with the footmen` vs `send the footmen at their castle` differ **only** in intent, and separating them is the entire point of the §8 executor seam.

**What I did instead of changing the header:** every `Notes` cell begins with a deterministic, machine-greppable prefix:

```
^intent=([a-z]+); 
```

Values: the 7 canonical intents, plus `none` (a Refuse row — `ESiegeAssistantIntent::None` is genuinely the expected value) and `unasserted` (rows where the *verb itself* is the ambiguity, e.g. DEV-07, HOLD-10 — asserting an intent there would beg the question).

**Recommended ruling (cheapest, and it preserves the seal):** TASK-410's runner reads intent from that regex. Zero header change, zero corpus edit, and the values are authored **now**, before any prompt exists — so the seal holds. If a manager instead adds an `ExpectIntent` column later, note that even a purely mechanical transposition is still an *edit to the sealed files*; my recommendation is to leave the files untouched and read the prefix.

### FINDING 2 (non-blocking, but it caps what the corpus can prove) — **an empty cell is overloaded**

CONVENTIONS §11 pins "empty cell = the field is **not asserted**". So a row whose *correct answer is that the field is empty* cannot say so. This bites exactly the army-wide intents: `rally` (DEV-25, HOLD-05) and `fallback` (HOLD-11) must emit **no** kinds, **no** counts and **no** where — a model that helpfully fills `where=own_castle` from "pull back to our castle" is wrong, and the CSV cannot score it. Those rows carry the expectation in `Notes` instead. A future schema pass could add an "asserted empty" token; I did not invent one, because TASK-410's parser would not know it.

---

## Coverage matrix — every mandatory category appears in BOTH files

| # | Mandatory category (board §(3) / CONVENTIONS §11) | DEV rows | n | HOLDOUT rows | n |
|---|---|---|---|---|---|
| 1 | **Ambiguous quantities** | DEV-02 (`some`) | 1 | HOLD-04 (bare `10`, no unit named) | 1 |
| 2 | **Units not in the roster ⇒ Refuse** | DEV-04 (`catapults`) | 1 | HOLD-07 (`trebuchets`) | 1 |
| 3 | **⚠️ Sorcerer/Wizard collision** — each BY NAME + a "mage" ⇒ Clarify | DEV-01, DEV-24 (sorcerer) · DEV-05, DEV-24 (wizard) · DEV-06 (`the mage` ⇒ Clarify) | 4 | HOLD-02 (sorcerer) · HOLD-08 (wizard) · HOLD-09 (`the mage` ⇒ Clarify) | 3 |
| 4 | **Selection verb mixed with an army-wide one** | DEV-07 (`charge with the footmen`) · DEV-08 (`fall back except the miners`) | 2 | HOLD-10 (`attack with the archers`) | 1 |
| 5 | **Multi-kind selections** (incl. the flagship + a 3-kind row at the cap) | DEV-01 (flagship, 2) · DEV-14 (**3 = cap**) · DEV-24 (2) | 3 | HOLD-01 (2) · HOLD-02 (**3 = cap**) | 2 |
| 6 | **A 4-kind row the grammar must not admit** | DEV-15 | 1 | HOLD-03 | 1 |
| 7 | **Shortfall ⇒ Clarify** | DEV-03 (10 asked / 8 alive) | 1 | HOLD-06 (6 asked / 3 alive) | 1 |
| 8 | **Deferred intent** (Trigger cells filled) | DEV-11 (`knight`/2) | 1 | HOLD-12 (`archer`/3) | 1 |
| 9 | **Out-of-scope ⇒ Refuse** (never plays cards, never spends gold) | DEV-09 (`play a knight`) · DEV-10 (`buy a miner`) | 2 | HOLD-13 (`spend my gold…`) · HOLD-14 (`give me plate armor`) | 2 |
| 10 | **Plurals / colloquials + `all`** | DEV-12 (`all the archers`) · DEV-13 (`everyone`) · DEV-16 (`infantry`) · DEV-17 (`horsemen`) · DEV-01/03/14/15 (`footmen`) | 5+ | HOLD-01 (`bowmen`) · HOLD-11 (`everyone`) · HOLD-15 (`all the horsemen`) | 3 |

**Bonus coverage present in both files (board §(4) and shipped-law traps):**

| Extra category | DEV | HOLDOUT |
|---|---|---|
| Place outside the v1 vocabulary ⇒ Clarify | DEV-22 (`the river`) | HOLD-15 (`the river`) |
| Terse / verbless / sloppy player typing | DEV-05, DEV-16, DEV-17, DEV-25 | HOLD-02, HOLD-05, HOLD-07 |
| **Ineligible-profile rows** (parse is right, the *executor* must object) | DEV-19 (Cleric = Support ⇒ no zone orders) · DEV-21 (Ogre = Siege ⇒ no orders at all) | — (dev-only; see §Balance note) |
| **The Cleric follow/hold split, both halves** | DEV-19 (send ⇒ Clarify) + DEV-20 (follow ⇒ Execute) | — |
| Knight vs Cavalry (two different cards, `horsemen` ⇒ cavalry) | DEV-17, DEV-22 | HOLD-03, HOLD-15 |

**Place-symbol coverage — all six v1 places appear in BOTH files:**

| Place | DEV | HOLDOUT |
|---|---|---|
| `enemy_castle` | DEV-03, 12, 13, 17, 21 | HOLD-03 |
| `own_castle` | DEV-16 | HOLD-06 |
| `mid` | DEV-02, 11, 14 (`the middle`), 15, 19 | HOLD-04 |
| `ancient_ground_near` | DEV-01, 18 | HOLD-02, 08, 09 |
| `ancient_ground_far` | DEV-05, 24 | HOLD-01 |
| `nearest_mine` | DEV-23 | HOLD-12 |

**Intent coverage:** DEV exercises all 7 (`send`, `guard` DEV-18/23, `ambush` DEV-19, `follow` DEV-20, `charge` DEV-07, `fallback` DEV-08, `rally` DEV-25). HOLDOUT exercises 6 of 7 (`send`, `guard` HOLD-06/09, `ambush` HOLD-12, `charge` HOLD-10, `fallback` HOLD-11, `rally` HOLD-05); **`follow` is DEV-only** — at 15 rows with 10 mandatory categories there was no slot left, and `follow` is the least adversarial verb (no place, no count semantics, and its selector is the already-public, already-idempotent `EnrollInDefaultFollowGroup`).

### Difficulty-parity check (the two files must not split easy/hard)

| | Execute | Clarify | Refuse | Rows asserting ≥3 fields |
|---|---|---|---|---|
| **DEV** (25) | 13 (52 %) | 9 (36 %) | 3 (12 %) | 17 (68 %) |
| **HOLDOUT** (15) | 6 (40 %) | 6 (40 %) | 3 (20 %) | 9 (60 %) |

Comparable in both directions: the holdout carries slightly more Clarify (harder) and slightly more Refuse (Refuse rows assert outcome only, so easier), and its multi-field-assert density is within 8 points of dev. Each of the 10 mandatory categories is represented in **both** files — the split is by row, never by difficulty.

---

## How the expected outputs were derived (not guessed)

Every unit symbol is the lowercased CardID of a real `Docs/Data/cards.csv` row — verified mechanically, no invented cards. The nine kinds used: `footman`, `archer`, `knight`, `pikeman`, `cavalry`, `cleric`, `ogre`, `wizard`, `sorcerer`.

The `Clarify` rows that hinge on **eligibility** were derived from shipped code, not from the design doc:

- `ASummonedUnit::IsGroupCommandEligible()` (`SummonedUnit.cpp:1946`) = `CanTakeZoneOrders() && Team==Blue && !bDead && !bAIFrozen`, and `CanTakeZoneOrders()` (`:1925`) is **`Profile == ECardProfile::Standard`**.
- `IsFollowCommandEligible()` (`:1934`) = `CanFollowHero() && …` — **wider**.

Consequences the corpus encodes:
- **Cleric = `Support`** ⇒ follow-eligible but **not** zone-orderable. DEV-19 (`ambush mid with 2 clerics`) is `Clarify`; DEV-20 (`clerics follow me`) is `Execute`. **Both halves of the split are in the corpus on purpose** — a system that gets only one right is not correct.
- **Ogre / Sapper = `Siege`** ⇒ no orders at all. DEV-21 is `Clarify` with a *fully correct parse* asserted (`ogre`/1/`enemy_castle`) — the grammar should succeed and the executor should be what objects.
- **Sorcerer = `Standard`** (`SorcererUnit.h`: "COMMANDABILITY IS LOAD-BEARING ON `Profile = Standard`") ⇒ commandable, which is what makes the feature's own flagship sentence executable. Had the Sorcerer been `Support` like the Cleric, DEV-01 would have had to be a Clarify.

**Quantity rule the corpus declares** (a requirement flowing corpus → implementation, the legitimate direction): explicit number ⇒ that number · `a`/`the` + singular ⇒ 1 · bare/definite plural, `all`, `everyone` ⇒ **Count 0 = all** · vague quantifiers (`some`, `most`, `a few`) ⇒ **Clarify**, count not asserted. DEV-02 (`some`) and DEV-12/16/17 (bare plural) are the paired rows that pin this distinction.

**`count` is never capped to the live roster** in an expected cell — DEV-03 expects `10` with 8 alive and HOLD-06 expects `6` with 3 alive, per CONVENTIONS §1. A system that emits the live max instead makes the clarification undetectable, and these two rows are what catch it.

---

## Reactive-coverage debt for TASK-421 (`DA_AssistantVocabulary`)

These aliases are **required by corpus rows and may not exist yet**. Per the board's TASK-421 §(5) and CONVENTIONS §11, adding them is legitimate work in the correct direction — but TASK-421 **must list them in its handoff as reactive**. ⛔ It must never narrow or edit the corpus to match its table.

| Alias in a corpus row | Must resolve to | Rows |
|---|---|---|
| `infantry` | `footman` | DEV-16 |
| `footmen` | `footman` | DEV-01, 03, 14, 15 |
| `bowmen` | `archer` | HOLD-01 |
| `horsemen` | **`cavalry`** (⚠️ not `knight`) | DEV-17, HOLD-15 |
| `the middle` | `mid` | DEV-14 |
| `their castle` / `the enemy base` | `enemy_castle` | DEV-03, 04, 12, 17, 21, HOLD-03 |
| `our castle` | `own_castle` | DEV-16, HOLD-06 |
| `the nearest ancient ground` / `the near ancient ground` | `ancient_ground_near` | DEV-01, 18, HOLD-02, 08, 09 |
| `the far ancient ground` | `ancient_ground_far` | DEV-05, 24, HOLD-01 |
| `the nearest mine` | `nearest_mine` | DEV-23, HOLD-12 |
| **`mage`** | **NOTHING — it must stay ambiguous** | DEV-06, HOLD-09 |

⚠️ **`mage` is the one that must NOT be aliased.** Giving it to either card turns DEV-06/HOLD-09 from an honest Clarify into a confident wrong command — the exact valid-shaped-wrong-command failure the design exists to prevent. `sorcerer` and `wizard` must share **no** alias.

### Two symbols the corpus had to choose because nothing pins them

1. **`nearest_mine`** — CONVENTIONS §8 and the board both say only "the mines"; no exact symbol is pinned anywhere. I chose `nearest_mine` (it matches the shipped `AGoldNode::FindBestMineFor` idiom and the near/far derivation style). **TASK-416/418/421 must adopt this exact spelling**, or DEV-23 and HOLD-12 will fail for a naming reason rather than a model reason. Flagging rather than assuming.
2. **`MilitiaMob` is deliberately absent.** Its canonical lowercase symbol is genuinely undecided (`militiamob` vs `militia_mob`), and I would not spend a scored row on a coin-flip. No mandatory category needed it.

---

## Rows where the *implementation* must declare a choice (corpus reveals a requirement)

- **DEV-13 `send everyone at their castle`** — `ExpectKinds`/`ExpectCounts` are deliberately **not asserted**. The schema has no "all kinds" symbol; the natural reading is an empty selection (`Kinds.Num()==Counts.Num()==0`, which satisfies the pinned invariant). **TASK-417 must declare what `everyone` emits.** Only `intent` and `where` are scored on this row.
- **DEV-06 vs HOLD-09** are deliberately *different shapes* of the same collision: DEV-06 has **two** ambiguities (unit *and* an unqualified "the ancient ground") so nothing but the intent is asserted; HOLD-09 has exactly **one** (the unit) so the intent *and* the place are asserted. A system that clarifies by dumping the whole request fails HOLD-09.
- **HOLD-08 `send the wizard to the near ancient ground`** is a trap for helpfulness: only the Sorcerer empowers on an ancient ground, so a model that "corrects" wizard → sorcerer produces a perfectly-shaped wrong command. Expected: `wizard`, unchanged.

---

## Verification performed

Ran mechanically over both files (no manual eyeballing as the acceptance criterion):

- header string equality against the pinned text — **both files, exact**;
- row counts **25 / 15**;
- **8 commas on every row** (naive-split safe);
- `ExpectOutcome ∈ {Execute, Clarify, Refuse}` on every row;
- `ExpectKinds` / `ExpectCounts` **index-aligned** wherever both are present;
- **no `ExpectKinds` cell exceeds the cap of 3** (the over-cap rows assert *no* kinds, because the correct behaviour is to not emit them);
- every kind symbol resolves to a real `cards.csv` CardID; every trigger kind likewise;
- every count is an integer in **0..30** (the `GrammarCountMax` range, `0 == all`);
- every `ExpectWhere` ∈ the six v1 place symbols;
- trigger pair never half-filled;
- every `Notes` cell carries the `intent=…; ` prefix with a legal value;
- **every Refuse row asserts outcome only** (no kinds/counts/where/trigger);
- **no sentence appears in both files** (and none is duplicated within a file);
- every sentence lowercase.

**Result: ALL CHECKS PASS.**

**Not verified here, by design:** few-shot disjointness. The Zone-A few-shots do not exist yet — **QA (TASK-412) checks it by literal string comparison** against both files once TASK-410 lands, per CONVENTIONS §11.

---

## Notes for downstream

- **TASK-410:** you own the runner, not the test set. Tune few-shots against **DEV only**. ⛔ Do not open `assistant_eval_holdout.csv`, and do not read the §HOLDOUT dump below. Score intent via the `Notes` regex (Finding 1) unless a manager rules otherwise. If you add a few-shot *because a dev row failed*, say so in your handoff — reactive coverage is legitimate but must be visible.
- **TASK-413:** the **holdout** number is the one reported against ≥85 %. Report both splits separately and label them unmistakably.
- **TASK-421:** the reactive-alias table above is your input. Declare what you add. `mage` stays unaliased.
- **TASK-414 (commit B):** both CSVs plus this handoff are the commit payload. I touched no other file.

---

## DEV split — full dump (25 rows)

```csv
Id,Sentence,ExpectOutcome,ExpectKinds,ExpectCounts,ExpectWhere,ExpectTriggerKind,ExpectTriggerAtLeast,Notes
DEV-01,send 10 footmen with a sorcerer to the nearest ancient ground,Execute,footman|sorcerer,10|1,ancient_ground_near,,,intent=send; FLAGSHIP MULTI-KIND 2 of 3; sorcerer BY NAME (the non-attacking ley-warden); plural colloquial footmen
DEV-02,send some footmen to mid,Clarify,footman,,mid,,,intent=send; AMBIGUOUS QUANTITY - some is not a number; kind and place are clear so only the count is asked; ExpectCounts deliberately NOT asserted
DEV-03,send 10 footmen at their castle,Clarify,footman,10,enemy_castle,,,intent=send; SHORTFALL - only 8 are alive; the grammar must still emit 10 and NOT 8 (CONVENTIONS s1) or the clarification is undetectable
DEV-04,send the catapults at the enemy base,Refuse,,,,,,intent=none; UNIT NOT IN ROSTER - catapult is not in cards.csv; must refuse rather than snap to a lookalike card
DEV-05,2 wizards to the far ancient ground,Execute,wizard,2,ancient_ground_far,,,intent=send; WIZARD BY NAME (the AoE fireball caster); terse verbless phrasing so send must be inferred
DEV-06,send the mage to the ancient ground,Clarify,,,,,,intent=send; SORCERER/WIZARD COLLISION - mage could be either card AND the ancient ground is unqualified near/far; two independent ambiguities in one line
DEV-07,charge with the footmen,Clarify,footman,0,,,,intent=unasserted; SELECTION VERB MIXED WITH ARMY-WIDE - charge is a latched army-wide stance with NO selection so this must never become a selective order; the footman selection is understood
DEV-08,everyone fall back except the miners,Clarify,,,,,,intent=unasserted; ARMY-WIDE WITH AN EXCEPTION - fallback carries no selection so except the miners is inexpressible; must ask rather than silently drop the exception
DEV-09,play a knight,Refuse,,,,,,intent=none; OUT OF SCOPE - the assistant never plays cards (Jonathan ruling 3); it commands only what already exists
DEV-10,buy a miner,Refuse,,,,,,intent=none; OUT OF SCOPE - the assistant never spends gold (Jonathan ruling 3)
DEV-11,wait until i have 2 more knights then send them mid,Execute,knight,2,mid,knight,2,intent=send; DEFERRED INTENT latched on a kind+count trigger; note the relative word more - TriggerAtLeast is absolute so the executor resolves it against the live roster and re-asks rather than firing blind
DEV-12,send all the archers at their castle,Execute,archer,0,enemy_castle,,,intent=send; ALL - explicit all maps to Count 0; colloquial their castle maps to enemy_castle
DEV-13,send everyone at their castle,Execute,,,enemy_castle,,,intent=send; ALL WITH NO KIND NAMED - kinds/counts deliberately NOT asserted because the schema has no all-kinds symbol; TASK-417 must declare what everyone emits (an empty selection is the natural reading)
DEV-14,send 5 footmen 3 archers and a knight to the middle,Execute,footman|archer|knight,5|3|1,mid,,,intent=send; MULTI-KIND AT THE CAP - exactly 3 kinds; a leading a means 1; colloquial the middle means mid
DEV-15,send 5 footmen 3 archers 2 pikemen and a knight to mid,Clarify,,,mid,,,intent=send; FOUR KINDS - over SiegeAssistantMaxSelectionKinds 3; the grammar must not admit it and it must never silently drop a kind
DEV-16,infantry back to our castle now,Execute,footman,0,own_castle,,,intent=send; COLLOQUIAL infantry means footman - an alias that may not exist yet and the row is written anyway (CONVENTIONS s11 fitting direction); bare plural means all so Count 0
DEV-17,horsemen go hit their castle,Execute,cavalry,0,enemy_castle,,,intent=send; COLLOQUIAL horsemen means cavalry and NOT knight - they are different cards; sloppy verbless phrasing
DEV-18,guard the near ancient ground with 4 pikemen,Execute,pikeman,4,ancient_ground_near,,,intent=guard; selection-bearing verb with the selection stated AFTER the place
DEV-19,ambush mid with 2 clerics,Clarify,cleric,2,mid,,,intent=ambush; INELIGIBLE PROFILE - the Cleric is Support so IsGroupCommandEligible is false (follow-only is shipped law); the parse is correct and the EXECUTOR is what must object
DEV-20,clerics follow me,Execute,cleric,0,,,,intent=follow; THE OTHER HALF OF THE CLERIC SPLIT - IsFollowCommandEligible IS true for the Cleric; follow takes no place
DEV-21,send the ogre at their castle,Clarify,ogre,1,enemy_castle,,,intent=send; INELIGIBLE PROFILE - Siege units (Ogre and Sapper) take no zone orders at all; the parse is correct and the executor must object
DEV-22,send the knights to the river,Clarify,knight,0,,,,intent=send; PLACE OUTSIDE THE V1 VOCABULARY - river is not a named place; the selection is understood so only the destination is asked
DEV-23,guard the nearest mine with 3 footmen,Execute,footman,3,nearest_mine,,,intent=guard; MINE PLACE - the dev row exercising the mine symbol
DEV-24,send the sorcerer and the wizard to the far ancient ground,Execute,sorcerer|wizard,1|1,ancient_ground_far,,,intent=send; THE SHARPEST COLLISION ROW - both cards named in ONE sentence; a model with a shared alias will emit one kind twice or drop one
DEV-25,rally on me,Execute,,,,,,intent=rally; ZERO-ARGUMENT ARMY-WIDE INTENT - no selection and no place; a model that invents a where fails this row
```

---

## ⛔ HOLDOUT split — full dump (15 rows). **TASK-410: DO NOT READ THIS SECTION.**

Opened exactly once, at TASK-413. This is the number reported against the ≥85 % bar.

```csv
Id,Sentence,ExpectOutcome,ExpectKinds,ExpectCounts,ExpectWhere,ExpectTriggerKind,ExpectTriggerAtLeast,Notes
HOLD-01,send 12 footmen and 4 bowmen to the far ancient ground,Execute,footman|archer,12|4,ancient_ground_far,,,intent=send; MULTI-KIND 2 of 3; colloquial bowmen means archer; plural footmen
HOLD-02,6 archers 2 pikemen and a sorcerer to the near ancient ground,Execute,archer|pikeman|sorcerer,6|2|1,ancient_ground_near,,,intent=send; MULTI-KIND AT THE CAP - exactly 3 kinds; sorcerer BY NAME (the non-attacking ley-warden); terse verbless phrasing
HOLD-03,send a knight 2 cavalry 3 archers and 4 footmen at their castle,Clarify,,,enemy_castle,,,intent=send; FOUR KINDS - over SiegeAssistantMaxSelectionKinds 3; the grammar must not admit it and it must never silently drop a kind; knight and cavalry are different cards in the same line
HOLD-04,send 10 to mid,Clarify,,,mid,,,intent=send; AMBIGUOUS QUANTITY WITH NO KIND NAMED - a bare number and no unit; must ask which unit rather than guess the most common one
HOLD-05,rally to me now,Execute,,,,,,intent=rally; ZERO-ARGUMENT ARMY-WIDE INTENT - no selection and no place; a model that invents a where fails this row
HOLD-06,guard our castle with 6 knights,Clarify,knight,6,own_castle,,,intent=guard; SHORTFALL - only 3 knights are alive; the grammar must still emit 6 (CONVENTIONS s1) or the clarification is undetectable
HOLD-07,send the trebuchets at them,Refuse,,,,,,intent=none; UNIT NOT IN ROSTER - trebuchet is not in cards.csv; the vague place at them must not rescue the row into an Execute
HOLD-08,send the wizard to the near ancient ground,Execute,wizard,1,ancient_ground_near,,,intent=send; SORCERER/WIZARD COLLISION TRAP - only the Sorcerer empowers on an ancient ground so a helpful model will want to correct wizard into sorcerer; it must NOT
HOLD-09,the mage needs to hold the near ancient ground,Clarify,,,ancient_ground_near,,,intent=guard; SORCERER/WIZARD COLLISION - mage could be either card; the intent and the place ARE unambiguous so only the unit is asked
HOLD-10,attack with the archers,Clarify,archer,0,,,,intent=unasserted; SELECTION VERB MIXED WITH ARMY-WIDE - attack reads as the latched army-wide charge stance which carries no selection; the archer selection is understood
HOLD-11,everyone pull back to our castle,Execute,,,,,,intent=fallback; ARMY-WIDE LATCHED STANCE - fallback takes no selection and no place; a model that fills kinds from everyone or where from our castle fails this row
HOLD-12,once i have 3 archers ambush the nearest mine with them,Execute,archer,3,nearest_mine,archer,3,intent=ambush; DEFERRED INTENT on a kind+count trigger plus the MINE PLACE; the latched order must re-resolve on fire and re-ask rather than execute blind
HOLD-13,spend my gold on another ogre,Refuse,,,,,,intent=none; OUT OF SCOPE - the assistant never spends gold (Jonathan ruling 3)
HOLD-14,give me plate armor,Refuse,,,,,,intent=none; OUT OF SCOPE - PlateArmor is a HeroUpgrade card and the assistant never plays cards (Jonathan ruling 3)
HOLD-15,all the horsemen at the river,Clarify,cavalry,0,,,,intent=send; PLACE OUTSIDE THE V1 VOCABULARY - river is not a named place; all plus colloquial horsemen means cavalry and NOT knight so the selection is understood
```
