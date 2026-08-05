# TASK-524 — EVAL CORPUS: `DEV-08` re-reasoned, exclusion + Sorcerer rows added

**Agent:** gameplay-programmer · **Date:** 2026-08-04 · **QA gate:** **TASK-525**
**File touched — the ONLY one:** `Docs/Data/assistant_eval_dev.csv`
**M8 DECLARATION (verbatim, as required):** *adds no replicated property, no new replicated class, no new relevancy tier.*

---

## 0. THE SEAL — DISCHARGED BY STATEMENT, NOT BY A READ

🔒 **`Docs/Data/assistant_eval_holdout2.csv` WAS NOT OPENED, NOT READ, NOT `grep`ed, NOT QUOTED AND NOT SCORED AGAINST.** It does not appear in any command I ran except as an unread sibling in one `ls Docs/Data/` listing, which printed its **filename only**.
🔒 **`Docs/Data/assistant_eval_holdout.csv` (generation 1, seal SPENT) WAS ALSO NOT OPENED AND NOT EDITED** — a spent seal is still not an editing surface (`AS-§20.7`).
✅ `git status --porcelain Docs/Data/` shows **one** modified file: `assistant_eval_dev.csv`. Both holdouts are clean.

---

## 1. ⛔ WHAT IS *NOT* MEASURED — READ THIS BEFORE THE ROW TABLE

**(a) THE EXCLUSION ROWS ARE DOCUMENTATION, NOT A SCORE.** `SpikeEval` does not parse the new `except=` prefix and this batch changes no runner (spec ruling 2). I verified this at the artifact: `SiegeLlamaSpike.cpp` extracts **only** `^intent=([a-z]+); ` (`ExtractIntentPrefix`, ~line 1969) and `FCorpusRow` has **no** exclusion field. ⇒ **Until the boarded runner follow-up lands, `except=miner` on `DEV-26` scores nothing. A row that is not scored has not measured anything.**

**(b) `DEV-31` CANNOT POSITIVELY ASSERT THE SORCERER, AND ITS `Notes` SAY SO IN THE ROW ITSELF.** An empty `ExpectKinds` cell is *"not asserted"* and is counted as **skipped** (`AS-§11` known limitation; `ScoreRow` increments `AssertionsSkipped`). The expected answer is `who:"all"`, which parses to an **empty** kinds array — so there is no cell that can hold the claim. **The real instrument is TASK-523's automation test.** I did not pretend otherwise anywhere in the file. ⚠️ **See finding F-1: the orchestrator's brief asked for a positive assertion and the schema cannot carry one.**

**(c) ⛔ ANY SCORE MEASURED AFTER THIS EDIT IS NOT COMPARABLE TO A SCORE MEASURED BEFORE IT.**
- **Date of edit:** 2026-08-04.
- **Rows CHANGED:** `DEV-08` (Notes only — verdict, sentence and all seven other cells byte-identical).
- **Rows ADDED:** `DEV-26 · DEV-27 · DEV-28 · DEV-29 · DEV-30 · DEV-31 · DEV-32`.
- **Corpus size: 25 → 32 rows.** A dev percentage from before this edit and one from after it are **different denominators over different content**. A ladder graph that spans 2026-08-04 without a break in the line is a **false trend**.

**(d) NOT COMPILED, NOT RUN.** No C++, no runner, no grammar, no Git, no editor/MCP/PIE. TASK-526 owns the compile; the eval needs the model and a machine.

---

## 2. THE ROW THAT CHANGED

### `DEV-08` — *"everyone fall back except the miners"* — **`Clarify` KEPT, REASON REPLACED**

Only field 9 moved. The verdict, the sentence and the six expectation cells are untouched.

- **was:** `… ARMY-WIDE WITH AN EXCEPTION - fallback carries no selection so except the miners is inexpressible; must ask rather than silently drop the exception`
- **now:** `… VERDICT UNCHANGED BUT THE REASON WAS REPLACED AT TASK-524; the exception IS expressible now (who all_except from TASK-518) so the old note that the schema has no exclusion is WRONG; what stays inexpressible is an exception on an ARMY-WIDE STANCE VERB - fallback carries no selection and runs through the shipped ApplyArmyWideStance that the E key calls so it never reaches the selector and the parser refuses all_except there; must ask rather than drop the exception because fall back except the miners executed as a plain fallback IS fall back INCLUDING the miners`

**Why the rewrite is the point:** the row was right for a reason that has since become false. A reader who found the old note after TASK-518 lands would read *"the schema has no exclusion"*, see `all_except` in the grammar, conclude the row was stale, and **flip a correct row to `Execute`** — which is the one edit that would turn *"fall back except the miners"* into *"fall back INCLUDING the miners"*, silently.

---

## 3. THE ROWS ADDED

All seven sentences were authored **from the player's sentence and the pinned schema**, before any of the implementation tasks (516–522) were readable. I did not read the exclusion implementation.

| ID | Sentence | Outcome | Asserted cells | `Notes` prefixes | Why that expectation |
|---|---|---|---|---|---|
| **DEV-26** | `send all units except the miners to the middle` | **Execute** | `where=mid` | `intent=send; except=miner; ` | ⭐ **Jonathan's own example, in the verb that can carry it.** `send` is selection-bearing ⇒ `all_except` is legal (`AS-§20.1`). Kinds/counts **empty on purpose**: the expected `who` is the **`all` selection minus miner**, never an enumeration. The Miner *takes zone orders* (`MinerUnit.h:413` — verified) and 3 are alive on t0, so the exception subtracts real units rather than being vacuous. The place is a well-taught colloquialism deliberately, so the exception is the only hard variable. |
| **DEV-27** | `send everything except the miners and the clerics to our castle` | **Execute** | `where=own_castle` | `intent=send; except=miner\|cleric; ` | The **two-kind** rung of the 1..3 alternation, with the kinds the spec named. `everything` is a **third** quantifier word beside *all units* / *everyone*, so the row also measures the quantifier. ⚠️ **The Cleric is `Support` ⇒ `IsGroupCommandEligible` is false** — the row states why that does **not** make it `DEV-19`'s `Clarify`: an ineligible kind **subtracted** costs the order nothing; an ineligible kind **selected** leaves nothing to command. That sentence exists to stop a future reader "fixing" this row against DEV-19. |
| **DEV-28** | `send everyone except the miners the clerics the sappers and the ogres to mid` | **Clarify** | `where=mid` | `intent=send; ` | **Four excluded kinds — over `SiegeAssistantMaxExclusionKinds = 3`.** The grammar must not admit a fourth and **must never silently drop one**. Authored to **mirror `DEV-15` cell-for-cell** (the four-*kind selection* row the spec named as the precedent), including the `where` assertion, so both arity caps are measured the same way. |
| **DEV-29** | `send all except 5 archers at their castle` | **Clarify** | `where=enemy_castle` | `intent=send; ` | ⭐ **The DECLINED variant, recorded as a decision rather than left as a gap.** The except list is bare kind symbols and never `{kind,n}` items, so *"all except 5 archers"* has **no shape to be sampled into**. The row names the failure to watch for: `all_except:["archer"]`, which drops the `5` and excludes **all 6** archers alive on t0 — a valid-shaped wrong command. |
| **DEV-30** | `rally on me except the miners` | **Clarify** | *(none — all cells empty)* | `intent=unasserted; ` | **`DEV-08`'s class stated positively, on the third army-wide verb.** `Rally` executes through `AHeroCharacter::Rally()` and never reaches the selector, so the parser refuses `all_except` (`ExcludeConflict`). ⭐ **Controlled pair with `DEV-25` (`rally on me`): the exception is the ONLY difference between the two sentences**, so a divergence isolates the exclusion path rather than the verb. Asserts nothing for `DEV-08`'s reason — the correct product answer is a question, and no command form is correct. |
| **DEV-31** | `send all units to the far ancient ground` | **Execute** | `where=ancient_ground_far` | `intent=send; ` | ⭐ **The Sorcerer row.** *All units* must include the Sorcerer, which is alive and orderable on **t0** (1 unit) and is the **last `DT_Cards` row**, i.e. the first kind any roster collapse hides (`AS-§20.2`). The place is the Ancient Ground because that is the only place the Sorcerer's ritual does anything, so a human reading the per-row output can judge it. Uses **Jonathan's phrase** (*"all units"*), not `DEV-13`'s *"everyone"*, and a **different place**, so it does not duplicate `DEV-13`. ⛔ **The row's own `Notes` carry the WARNING that it cannot positively assert the Sorcerer and that TASK-523 is the real assertion.** |
| **DEV-32** | `send everyone except the catapults at their castle` | **Refuse** | *(none — `intent=none`)* | `intent=none; ` | **Excluding a kind that is not in the roster.** ⚠️ **On t0 all 13 commandable kinds are alive** (verified against `SpikeRoster`), so *"a kind that is not alive"* can only be written as *"a kind that is not in `[FORCES]`"*. The noun is **`DEV-04`'s, on purpose**: the only difference between the two rows is the **structure** (an exception instead of a selection), so a divergence isolates the exclusion path — and `DEV-04` already measured the model slot-filling `sapper` for *catapults*, which here becomes an **invented exclusion**. ⭐ **This is the only new row today's runner scores in full**, because `Refuse` is the one `ExpectOutcome` that demands a question — see F-2. |

---

## 4. THE CONVENTIONS I HELD, AND HOW I CHECKED THEM (mechanically, not by eye)

| Rule | How discharged |
|---|---|
| ⛔ **NO tenth column** | Header is **byte-identical** to the pin — `cmp` against the literal string: **identical**. **9 columns everywhere: all 33 lines parse to exactly 9 fields.** |
| ⛔ `intent=` prefix not reformatted | Every one of the 32 rows still matches `^intent=([a-z]+); ` (checked with the runner's own acceptance rule, including the `;`-terminator condition `ExtractIntentPrefix` enforces). Values are in the pinned set (7 intents + `none` + `unasserted`). |
| ✅ `except=` prefix pinned format | `except=<kind>\|<kind>; ` sits **immediately after** the intent prefix (the intent regex is `^`-anchored and must stay first). Both rows match `^intent=[a-z]+; except=([a-z\|]*); `. |
| ✅ Exclusion invariants honoured **in the expectations** | Every `except=` row: 1..3 kinds · no repeated symbol · **empty `ExpectKinds`** (never a selection + an exclusion) · intent ∈ {send, guard, ambush, follow}. Checked by script, not by reading. |
| ✅ Kind symbols legal | Every kind and every excluded kind is one of the 13 lowercased card IDs; every `ExpectWhere` is one of the 7 pinned place symbols. |
| ✅ Row IDs stable, appended, not renumbered | `git diff` = **1 deletion (`DEV-08`) + 8 insertions**. `DEV-01..DEV-25` byte-identical apart from `DEV-08`'s `Notes`. IDs unique. |
| ✅ File conventions matched exactly | **ASCII only · no `"` · no `'` · no comma inside any cell · LF endings · trailing newline** — all four are properties of the shipped file (`grep -P` for non-ASCII/quote/apostrophe/CR returns **nothing**). This is why the `Notes` say `CONVENTIONS s20.1` and not `§20.1`: the file's existing rows already spell `§` as `s` (`DEV-03`, `DEV-16`). |
| ✅ Every `Notes` says **why** | All seven new rows and the rewritten `DEV-08` state the reasoning, not just the label — that convention is what made the `DEV-08` diagnosis possible a year later. |
| ⛔ **Few-shot disjointness — CHECKED LITERALLY, AS INSTRUCTED** | I read the **seven** shipped few-shot sentences out of `BuildZoneA` (`SiegeAssistantSnapshot.cpp:1113, 1115, 1121, 1178, 1188, 1197, 1200`) and compared all **32** corpus sentences against them by **string equality AND substring containment**: **zero collisions, zero containments.** The seven are: *send ten footmen with a sorcerer to the ancient ground on our side* · *all archers guard the middle* · *the archer guards our castle* · *send werewolves to the middle* · *get two more pikemen with our gold* · *i want the footmen to rush* · *everyone attack*. ⚠️ **I rejected `everyone attack except the miners` for `DEV-30` for exactly this reason** — it contains few-shot #7 verbatim as a prefix, which would have made the row prompt-fitted; `rally` is why the row uses `rally`. |
| ⛔ Sorcerer/Wizard collision not re-litigated | No new row uses *mage*, *caster*, *magician* or any shared alias. `DEV-06`/`DEV-24` are untouched. `DEV-31` names no kind at all; `DEV-32`'s noun is `catapults`, already cleared by `DEV-04`. |
| ⛔ One file | No `.cpp`, `.h`, `.cs`, grammar, runner or `Content/` asset touched. Source files were **read** (for the 13 kind symbols, the 7 place symbols, the eligibility split, the t0 fixture and the few-shot list) and never written. |

---

## 5. ⚠️ FLAGGED, NOT DECIDED — QA (TASK-525) RULES ON THESE

### F-1 ⭐ THE SORCERER REPAIR STILL HAS NO FAILING **CSV** ROW TO CLOSE, AND THE SCHEMA IS WHY
My dispatch said *"add at least one row that **positively asserts** the Sorcerer is included"* and *"a repair with no failing row to close is not a repair."* **I could not do it, and I do not believe it is doable inside the pinned 9-column schema.** The expected answer to an all-units order is `who:"all"`, which parses to an **empty** kinds array; the only cell that could carry the claim is `ExpectKinds`, and an empty cell means *"not asserted"*. Filling it with the 13 kinds would make the **correct** answer fail. ⇒ `DEV-31` is the strongest row the instrument allows (it asserts intent + place and puts the limitation in its own `Notes`), and the actual failing-then-passing assertion has to be **TASK-523's `Siegebound.Assistant.Selection.RosterShowsAllThirteenKinds`**. **If TASK-523 does not land that test, this defect ships unmeasured again — the CSV will not catch it.** This is `AS-§20.6`'s own position; I am recording that I hit the wall it predicted rather than papering over it.

### F-2 ⭐⭐ THE `Clarify` ROWS CANNOT FAIL A SILENT DROP — THE EXACT FAILURE `AS-§20.1` EXISTS TO PREVENT
Read off the runner (`ScoreRow`): `Clarify` sets `bOutcomeOk = true` for **either** output form, and asserted fields are compared **only if a command was emitted**. ⇒ On `DEV-28`, `DEV-29` and `DEV-30`, a model that emits a **command with the exception dropped** still passes as long as intent/place match. **`Refuse` is the only `ExpectOutcome` the runner scores as "the output MUST be a question."**
- I kept `Clarify` on 28/29/30 because the manager **pinned** those verdicts and because `DEV-08` (same class) is `Clarify` — consistency with the existing row mattered more than my scoring preference, and overturning a pinned verdict is not mine to do.
- **But the honest reading is that `Clarify` is the corpus's weakest cell and every exclusion-refusal row lands in it.** If TASK-525 agrees, the cheap fix is at the **runner** follow-up (score `except=`, and add a "must ask" notion), not at these rows. `DEV-32` is deliberately `Refuse` so that at least **one** new row can fail a silent drop today.

### F-3 THE `except=` PREFIX INHERITS THE OVERLOADED-EMPTY-CELL PROBLEM
Its **absence** means *"not asserted"*, not *"assert no exclusion"* — so no row can currently demand that the model emit **no** exclusion. The pinned regex `([a-z|]*)` does admit a bare `except=; `, which the runner could read as a positive *"expect none"*. **I did not invent that usage** (the pinned format is `except=<kind>|<kind>; ` and it is not mine to widen); flagging it as the obvious cheap win for the runner follow-up.

### F-4 A SEVENTH ROW, DECLARED
The board's `names:` block says **six** new rows (spec items a–f); my dispatch additionally required *"an exclusion naming a kind that isn't alive."* I authored **seven** (`DEV-32` is the extra). It adds **no new symbol** and continues the pinned `DEV-##` numbering, but it is a departure from the `names:` block and I am declaring it rather than letting QA discover it. **If TASK-525 rules against it, `DEV-32` is a clean single-line delete.**

### F-5 ⚠️ THE EVAL LANE CANNOT SEE THE SORCERER DEFECT'S ACTUAL CAUSE — AND NEVER COULD
The eval hardcodes fixture **t0**, and t0's roster prints **all 13 kinds unconditionally** — a *declared* deviation from the shipped `MaxRosterKinds = 8` (the comment above `SpikeRoster` states it and flags it against the game lane). ⇒ **The collapse that hid `sorcerer` from the shipped prompt (`AS-§20.2` leg i) has never happened on the eval lane.** `DEV-31` therefore measures the **comprehension** leg (Zone A's `who:"all"` rule) and **not** the cap/collapse leg. **No corpus row can measure the collapse leg while the fixture is exempt from the cap.** Recording this so nobody reads a green `DEV-31` as *"the Sorcerer defect is fixed."*

### F-6 A PRE-EXISTING ROW I DID **NOT** TOUCH
`DEV-15` asserts `where=mid` on a row whose correct product answer is a **question** — so under STRICT scoring a *correct* clarification cannot pass it (`bPassStrict` requires `AssertedFieldCount() == 0` when a question answers a `Clarify` row). Same shape as F-2. **I mirrored `DEV-15` on `DEV-28` deliberately** — the spec named it as the precedent and consistency between the two arity caps is worth more than my unilateral improvement — **but if QA rules the pattern wrong, 15 and 28 must move together.** ⛔ I did not edit `DEV-15`; existing rows stay stable.

---

## 6. WHAT QA SHOULD SCRUTINISE FIRST

1. **The `DEV-08` `Notes` rewrite** — is the new reason *actually* the reason, and does it survive TASK-518's landed parser? (If the parser does **not** refuse `all_except` on `fallback`, `DEV-08` is wrong and so is `DEV-30` — **and that disagreement is the finding**, per this task's whole premise.)
2. **`DEV-26`'s empty `ExpectKinds`/`ExpectCounts`** — confirm the landed parser really produces an **empty** kinds array for `{"all_except":[…]}` and not a synthesised enumeration. If it enumerates, `DEV-26` fails for a reason that is the implementation's, not the row's.
3. **F-2** — rule on `Clarify` vs `Refuse` for 28/29/30.
4. **F-4** — rule on the seventh row.
5. **The 9-column claim** — re-check it yourself; it is the one property whose breakage is silent and permanent after the seal.
