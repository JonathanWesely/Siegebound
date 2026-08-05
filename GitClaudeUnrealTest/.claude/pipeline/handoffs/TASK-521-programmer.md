# TASK-521 — [AX-6] ZONE A — the two rule lines

**Agent:** gameplay-programmer · **Date:** 2026-08-04 · **Status:** `ready-for-qa` · **QA gate:** TASK-525
**Law:** CONVENTIONS `AS-§20.4` (the budget + the no-new-few-shot ruling + the test trap) · `AS-§20.1` · `AS-§20.2` · `AS-§12c` · `AS-§12g` · `AS-§8` · `AS-§12a` · `FT-§16`

## M8 DECLARATION DUTY (verbatim, as required)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Reason: this task emits **text**. It adds no field, no class and no `UPROPERTY` of any kind.

---

## 1. THE TWO RULE LINES, VERBATIM (emitted characters, not source escapes)

**Rule 1 — the `who:"all"` decision-ordering rule (111 chars incl. the trailing `\n`):**

```
- Every unit, no exception: who is "all", never a list of kinds. charge, fallback and rally still take "none".
```

**Rule 2 — the exclusion rule (153 chars incl. the trailing `\n`):**

```
- Every unit but some kinds: who is {"all_except":[KIND]}, only with send, guard, ambush or follow. On charge, fallback or rally: {"ask":"unsupported"}.
```

Both sit **immediately after** the loop-2 selection rule (`- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.`) and **before** `- No number said: …`. Source: `SiegeAssistantSnapshot.cpp:1095` and `:1115`; the amended `WHO =` line is `:850`; the untouched `[FORCES]` refusal rule is now `:995`.

### Why they read the way they do

- **They are a MINIMAL PAIR.** Same opening (`Every unit`), and the only difference between the two antecedents is whether an exception was stated — which is exactly the distinction being taught, held against a constant frame. That is the device few-shots #2/#3 already use, applied to rules; it is the cheapest way to teach a small model a distinction and it costs nothing extra in chars.
- **They complete a decision LADDER with the rule above them**, read top-down: *kinds named* → a selection + a unit-taking intent (the existing loop-2 rule) · *every unit, no exception* → `"all"` · *every unit minus some kinds* → the exclusion. That is the instrument `AS-§20.4` says was measured winning at this seam, not an exemplar.
- ⛔ **Neither names a kind symbol.** `KIND` is the schema metavariable, never `miner`. The examples block's **§9c seam** warning is why: Zone A is byte-identical for process life while the grammar's `KIND` alternatives come from the **live** roster, so a concrete symbol written into Zone A is one the sampler may **forbid** on a board that lacks it, with no log line saying so. Zone A names exactly three kinds today (`footman`, `sorcerer`, `archer`) and this task refused to make it four **even though Jonathan's flagship sentence is about miners.**
- **Rule 1's antecedent is deliberately `every unit`, not the alias `everyone`.** TASK-431 measured this model failing lookups (the `ancient_ground_near` alias missed its own exact target string); the synonym table already routes `everyone attack` → `charge`.
- **Rule 1 does not contradict the `everyone attack` → `charge`/`who:"none"` few-shot.** The trailing clause names the three army-wide verbs explicitly, which is the half loop 2 measured beating the verb latch. That shot is correct and is **untouched**.
- **Rule 2's intent restriction is read off the executor, not chosen for safety.** `charge`/`fallback` run through `ASiegePlayerController::ApplyArmyWideStance(...)` — the same shipped API `T`/`E` call — and `rally` through `AHeroCharacter::Rally()`. None walks the candidate list. Without the restriction the model emits an exclusion TASK-518's parser then refuses (`exclude_conflict`), and the player experiences that refusal as *the feature not working*. `{"ask":"unsupported"}` reuses the shape this block already teaches twice — ⛔ **no new `ask` symbol was invented.**
- **Rule 2 does not repeat the arity or the no-counts property.** The `WHO =` schema line carries both, and the GBNF enforces both **structurally** (a bounded alternation of 1..3 **bare** kind strings), so restating them would spend budget on something the sampler cannot violate.

---

## 2. ⭐ A THIRD EDIT THE SPEC DID NOT ASK FOR — DECLARED, NOT SMUGGLED

**The `WHO =` schema line was amended (+44 chars):**

```
- WHO    = [{"kind":KIND,"n":COUNT}] with 1 to 3 entries, or "all", or "none"
+ WHO    = [{"kind":KIND,"n":COUNT}] with 1 to 3 entries, or {"all_except":[KIND]} with 1 to 3 kinds, or "all", or "none"
```

**Why it is not optional.** The comment that governs that block says, in the shipped source: *"⚠️ THIS BLOCK IS THE MIRROR OF TASK-417's GBNF, AND IT MUST STAY ONE. Zone A teaches the schema; `USiegeAssistantGrammar::Build` CONSTRAINS it. If they disagree, constrained decoding fights the few-shots on every token and accuracy (go/no-go bar #5) collapses for a reason no log line names. … ⚠️ A LATER EDIT TO EITHER FILE MUST EDIT BOTH. There is no compile-time link between them - this comment and the QA gate are the whole tie."*

**TASK-518 edited the grammar's `who` rule** (`who ::= selection | except | "all" | "none"`) and, correctly, did not touch Zone A. Had I written only the two rules, the schema block would have gone on **enumerating three `who` shapes** — actively telling the model the exclusion shape does not exist — while the sampler allowed it. That is precisely the failure loop 2 measured and documented at the selection rule: **the rule was not missed, it was OUTVOTED by the prompt's own competing lines, 4 to 1.** Rule 2 would have shipped into the same trap.

- It is **not** a few-shot sentence, so `AS-§20.4`'s ruling is not touched. The budget clause ("*the total new Zone-A text is ≤ 325 characters*") covers it and it is counted below.
- The alternative order is **the grammar's order**, deliberately: `selection | except | "all" | "none"`.
- ⚠️ **QA should rule on this explicitly.** The dispatch said "two rule lines"; I shipped two rule lines **plus** a schema-mirror repair, inside budget. If TASK-525 disagrees, the revert is one line and costs 44 chars back — but the mirror then stays broken and someone should board that.

---

## 3. CHAR ACCOUNTING — MEASURED, AND THE BASELINE WAS RE-COUNTED FIRST

I did **not** take 5116 on faith. Before editing anything I extracted the frozen fixture `BuildSpikeLaneZoneA()` (`Tests/SiegeAssistantZoneATest.cpp:105-202`), un-escaped every `TEXT()` literal, and counted:

```
BASELINE (frozen fixture, 98 literals)   5116 chars / 5116 UTF-8 bytes   <-- reproduces the shipped constant
```

Then, per edit:

```
  WHO schema line amendment            +44
  Rule 1  (who:"all")                 +111
  Rule 2  (all_except)                +153
  -----------------------------------------
  TOTAL ADDED                         +308      budget 325   SLACK 17
                                                             (5.2 % under)

  NEW ZONE A TOTAL                    5424 chars / 5424 UTF-8 bytes
```

⇒ ⛔ **THE NUMBER TASK-523 RE-BASES AGAINST IS `5424`, chars AND UTF-8 bytes (they are equal — every added character is ASCII).**

**Independently checked, three ways, and each check is re-runnable:**

1. Every one of the three new strings appears **exactly once** in `SiegeAssistantSnapshot.cpp` after un-escaping, and the old `WHO =` line appears **zero** times.
2. A structural diff of `BuildZoneA`'s ordered emitted literals against the frozen fixture's returns **exactly** one replacement and two insertions — **nothing else in Zone A moved a byte.** (The extractor also reports the two `none\n` synonym-fallback literals, which are the `if (Vocabulary)` / `else` branches the fixture inlines rather than branches; they are not sequential emissions.)
3. The byte-frozen economy line and the `:939` `[FORCES]` refusal line both still hash out **character-for-character identical** to the fixture.

⛔ **`ContextTokens` STAYS 2048.** ⛔ **No few-shot was trimmed to make room.** The 17 chars of slack were left unspent rather than absorbed into nicer prose — `AS-§20.4`'s ceiling is a ceiling, not a target.

---

## 4. THE `:939` [FORCES] REFUSAL RULE — **A DELIBERATE NO-OP, AND HERE IS WHY**

**Ruling: the rule's bytes are UNCHANGED. Its comment was rewritten.**

The rule still reads:

```
- If the unit named is not a kind in [FORCES], answer {"ask":"unsupported"}. Never write a kind the player did not name.
```

**What the hazard was.** The comment above it declared: `[FORCES]` was truncated to `MaxRosterKinds = 8` in the shipped snapshot but not in the spike fixture, so on a >8-kind board the rule could **refuse a kind that is alive** and merely got collapsed into `other_kinds` — and `Sorcerer` is the **last** commandable `DT_Cards` row, so it was the first kind collapsed, every time. That is the whole mechanism behind *"all units never includes sorcerers"*.

**Why it is closed — read off TASK-517's finished artifact, not relayed.** `AppendRosterBlock` now emits `other_kinds: sorcerer, cleric (5 units)` — **names, not just counts** — and that line is printed **inside the `[FORCES]` block**. So a collapsed symbol **is** a symbol in `[FORCES]`, and this rule's antecedent — a literal membership test over text the model can see — is **TRUE again**. A collapse now costs the per-kind **counts**, never a kind's **existence**.

**The three reasons for no wording (all recorded in-source so they are not re-litigated):**

1. The rule is already true as written. A clause saying *"`other_kinds` names count as `[FORCES]`"* would restate what the block's own layout already shows.
2. The spec requires any edit **here** to be char-neutral or **negative**, and no addition can be. The 325-char budget is better spent on two rules teaching behaviour the prompt did not have at all.
3. TASK-431 measured that restating something the prompt already shows has **poor leverage on this model** (19/25; the row taught twice over did not flip). Emphasis here would buy the least per character.

⚠️ **The residual, not papered over.** When the trimmer bites — measured by TASK-517 at **seven** extra typed `order:` chars on a 13-kind board — a collapsed kind appears with **no count**. The model can still name it and still order it; what it cannot read is a per-kind tally. The neighbouring `Never copy a count from the roster` rule already forbids reading tallies out of the roster anyway, so the two degrade in the same direction, and the shortfall stays a **game-side** report.

📌 **This answers TASK-517's handoff §7.7 directly** ("*Zone A currently frames `[FORCES]` as the roster rows; a reader-model could plausibly treat the `other_kinds:` line as not part of the roster — that is TASK-521's call*"). **My call: it is part of `[FORCES]` because it is printed inside `[FORCES]`, and I am not spending chars to assert a fact the layout already carries.** ⚠️ It is a **judgement**, and it is the single cheapest thing for TASK-525 to overrule if it disagrees — 40-ish chars exist in the slack to say it explicitly.

**Char-neutral repair actually made:** the stale hazard comment (which still said `MaxRosterKinds (8)`, *"the spike fixture prints all 13"*, and *"it is TASK-416's constant to move, not mine"*) was replaced with the above. **0 emitted characters.** A comment declaring a hazard that has been fixed is how a future reader re-opens a closed defect.

---

## 5. ⛔ THE TRAP — `D4`, DECLARED

> **`Siegebound.Assistant.ZoneA.TwoLaneByteEquality` and `Siegebound.Assistant.ZoneA.MeasuredCharCount` WILL NOW FAIL. That is correct, expected, and must not be silenced.**

- ⛔ **`Plugins/SiegeLlama/**` WAS NOT TOUCHED.** It is another batch's in-flight instrument (`FT-§16`, TASK-481). The shipped lane moved; the spike lane did not.
- ⇒ **`D4` — a NEW, DECLARED divergence between the shipped Zone A and the spike-lane Zone A, created deliberately under `AS-§20.4`'s ruling.** Divergence begins at the `WHO =` line (character **~289** of Zone A, the first byte of `{"all_except"…`) and the shipped lane is **308 chars longer** overall.
- ⛔ **I DID NOT EDIT THE TEST FILE.** TASK-523 owns it. And it must not be "updated to pass": the equality test's subject — *"the shipped prompt is the prompt the numbers were measured on"* — became **false** the moment we deliberately changed the shipped prompt, and no edit makes it true again. **Re-purpose it; do not re-copy this builder's output into the fixture.** A test whose transcription is silently re-copied from the thing it tests is a guardrail that reports SAFE.
- **What TASK-523 needs from me, stated as numbers it can assert rather than guess:**

| quantity | value | provenance |
|---|---|---|
| shipped Zone A, **before** | **5116** chars / **5116** bytes | re-counted by me off the frozen fixture; matches `MeasuredZoneAChars` |
| shipped Zone A, **after** | **5424** chars / **5424** bytes | counted, this task |
| delta | **+308** | 44 + 111 + 153 |
| spike-lane Zone A | **5116**, unchanged | `Plugins/SiegeLlama/**` not touched |
| emitted lines changed | **1 replaced, 2 inserted, 0 deleted** | structural diff of the ordered literals |

- ⛔ **TOKENS ARE NOT RE-COMPUTED.** `zoneA_tok = 1139`, the **77.1 %** KV-reuse figure and every prefill number derived from them are **STALE — PENDING RE-MEASUREMENT ON THE MODEL**. They are **not** deleted and **not** replaced with arithmetic; only `Siege.Llama.SpikePrompt` prints them. I relabelled them in-source at the `Out.Reserve` comment. **Chars are countable without a model; tokens are not — that asymmetry is the whole rule.**
- ⚠️ **A derived sanity note, offered as a bound and NOT as a measurement:** at `AS-§12g`'s pinned worst marginal rate (~1.3 chars/tok) the 308 new chars are **≤ 237 tokens**, which keeps `zoneA_tok` under the `≤ 1389` ceiling **even if the stale 1139 were accurate**. ⛔ **That is a budget check, not a token count, and it must not be written down anywhere as `zoneA_tok`.**
- `Out.Reserve(6144)` was **not** re-tuned — it still covers 5424 with 720 spare.

---

## 6. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp` | **`BuildZoneA` only.** 3 emitted-text edits (`WHO =` line, +2 rule lines) and 4 comment blocks (the GBNF-mirror comment, the `:939` hazard comment, the two new rules' rationale, the `Out.Reserve` staleness note). |

⛔ **NOT touched, verified:** `SiegeAssistantSnapshot.h` · `BuildZoneB` / `BuildZoneC` / `AppendRosterBlock` (TASK-517's, frozen) · `SiegeAssistantCommand.*` / `SiegeAssistantGrammar.*` (TASK-518) · `SiegeAssistantComponent.*` (TASK-520/522) · `SiegeAssistantConsoleWidget.*` (TASK-519) · `Tests/**` (TASK-523) · `Docs/Data/*.csv` (TASK-524) · **`Plugins/SiegeLlama/**` (D4, never)** · `SiegePlayerController.*` · `HeroCharacter.*` · any `Content/` asset.

⛔ **No compile, no Git, no editor/MCP/PIE.** TASK-526 owns the only compile. **No asset is referenced** — this task is pure prompt text.

🔒 **`Docs/Data/assistant_eval_holdout2.csv` was NOT opened, not scored against, not quoted.** ✅ **The seal is unspent** — and the reason it stayed free is structural, not disciplinary: **no new example sentence exists in this change, so no absence certificate was ever required.** `assistant_eval_dev.csv` was read (2 rows, `DEV-08` / `DEV-13`) to check my rules against the corpus's expected verdicts; it is the regression instrument and is never trained on.

---

## 7. WHAT QA SHOULD SCRUTINISE

1. ⭐ **The `WHO =` schema amendment — is the third edit accepted?** §2 above. It is inside budget and it repairs a mirror TASK-518 left broken, but the dispatch said "two rule lines". **Rule on it explicitly rather than letting it pass unremarked.**
2. ⭐ **The `:939` no-op.** §4. A deliberate no-op is a finding; check my reasoning against `AppendRosterBlock`'s actual output rather than against my summary of it.
3. ⚠️ **A wording adjacency I am flagging myself rather than letting QA find:** rule 1 says *"never a list of kinds"* and rule 2's own shape **contains** a list of kinds. The antecedents are disjoint (`no exception` vs `but some kinds`) and rule 2 sits on the very next line, so a top-down read resolves it — but a model that latches the negative could suppress rule 2. **This is the sharpest residual risk in the change** and I chose adjacency + disjoint antecedents over spending ~14 chars to qualify the negative. Overrulable inside the 17-char slack only if the qualifier is short.
4. **Both rules against the GBNF, character for character.** `{"all_except":[KIND]}` vs `except ::= "{" "\"all_except\"" ":" except_list "}"`; the intent list vs `SiegeAssistantIntentTakesSelection`; `{"ask":"unsupported"}` vs the `ask` alternatives. **The mirror law is the whole tie — there is no compile-time link.**
5. **That nothing else in Zone A moved.** Re-run the check yourself: the seven few-shots, the byte-frozen economy line **and its position** (its declared neighbours are the `[FORCES]` rule above and the selection rule below — both still adjacent to it), and the frozen invented noun.
6. **The placement.** The two rules go **after** the selection rule, not between the `[FORCES]` rule and the economy line — the economy line's *position* is frozen along with its bytes.
7. ⚠️ **`AS-§20.4`'s named #1 accuracy risk still stands and this task does not discharge it:** *a rule line is a weaker teaching signal than an exemplar for a brand-new output SHAPE the model has never seen.* Both `who:"all"` and `{"all_except":…}` are shapes **no exemplar in this prompt demonstrates**. ⛔ **Nothing here has been MEASURED on the model** — TASK-526 compiles, and Jonathan's playtest is the gate. **If exclusion or "all units" is still missing there, the next pass may spend a few-shot, but only on HIS instruction to run the absence certificate.**

---

## 8. ANYTHING IN THE SPEC I THINK IS WRONG

**One real gap, one qualifier, one correction. Nothing blocking.**

1. ⭐ **THE GAP: the spec scoped this task to "two rule lines" and did not account for the `WHO =` schema line, which TASK-518 obsoleted.** `AS-§20.7` lists both `SiegeAssistantGrammar.cpp` and `SiegeAssistantSnapshot.cpp` as touched files but nothing in `AS-§20` states that **the grammar edit forces a Zone-A schema edit**, even though the shipped source says so in capitals. **Between two tasks that each correctly stayed in their own file, the mirror had nobody.** I closed it and declared it (§2), but the *law* should carry a line: **any change to `SiegeAssistantGrammar.cpp`'s `root`/`command`/`who`/`selection`/`item`/`count`/`at-least` rules obliges the same-batch Zone-A schema-block edit, and the QA gate checks the pair.** Worth boarding for the manager.
2. **The spec's "⭐ `who:"all"` is never demonstrated … all seven few-shots emit either a ≤3-kind array or `who:"none"`" is exactly right, and the diagnosis is confirmed at the artifact** — but the sharper statement of *why* it is fatal is worth recording: the array is capped at `SiegeAssistantMaxSelectionKinds = 3`, so on a 13-kind board the array form **structurally cannot** reach the Sorcerer. It is not that the model *tends* to omit him; through that shape it **cannot** include him. That is why rule 1's operative half is the negative (`never a list of kinds`) and not the positive.
3. **A small correction to the dispatch's line numbers**, so QA does not chase them: the few-shots were at `:1112-1201` and the refusal rule at `:939` **before** my edit. After it they are at **`:1233-1322`** and **`:995`**. The dispatch's numbers were correct when written.

**Two things the spec got right that I want on the record because they changed my output:**

- ⛔ **The no-new-few-shot ruling is load-bearing and I would have violated it.** A `who:"all"` exemplar was my first instinct too. TASK-430's precedent — two agents on the same public brief converging on the same invented noun, **neither able to see it** — is the argument, and it is not about care.
- ⭐ **Forbidding the test edit is what makes the char count worth anything.** Had I been allowed to re-base `MeasuredZoneAChars` myself, the number in this handoff would have been *derived from my own edit and asserted against my own edit*. Because a separate task re-bases it against a number I measured and published **before** it reads my code, the two can disagree — and a guardrail that can disagree is the only kind worth having.
