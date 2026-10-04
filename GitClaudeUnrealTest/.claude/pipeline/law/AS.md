<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ AI-COMMANDER ROBUSTNESS — spatial selection + the `defend` routing repair (2026-08-05) — namespace **`AS-§21`**

Added 2026-08-05 on Jonathan's directive, verbatim: *"ok, we need to make the AI commander much more robust. I tried to tell it send all units units except miners to defend the castle and it couldn't do that, I also tried to tell it to send all units currently in an ancient ground to attack a castle and it couldn't do that, how do you think we can make the AI commander better?"*

Design authority = the approved plan `C:\Users\wesel\.claude\plans\ok-there-are-a-cheerful-mccarthy.md` (plan mode, every claim verified first-hand against shipped source, ExitPlanMode-approved). ⛔ **The plan file wins over any board summary; the architecture is NOT re-litigated.** This section is the naming/behaviour law derived from it plus the batch's link contract.

> ### ⛔⛔ **THE PLAN-FILE SLOT HAS NOW BEEN OVERWRITTEN *TWICE*. AT THE TIME OF WRITING IT HOLDS **THIS** (ASSISTANT) PLAN — AND `KBD-§1` AND `NAV-§`'s HEADER BOTH CITE THE SAME PATH.**
> - **Generation 1 = the KEYBOARD-LAYOUT plan (GONE).** **Generation 2 = the UNIT-PATHING plan (`NAV-§`) (GONE).** **Generation 3 = this one.**
> - ⇒ ⛔ **A reader following `KBD-§`'s OR `NAV-§`'s design-authority citation now lands in the AI-COMMANDER plan — a THIRD unrelated feature.** ✅ **Neither of them needs it: `KBD-§0..§11` and `NAV-§0..§14` are each complete on their own.**
> - ⛔ **DO NOT "RECONCILE" THE MISMATCH BY EDITING ANY OF THE THREE SECTIONS.** ⚖️ **Three laws, one filename, zero overlap — the collision is in the PATH, never in the law.**
> - ⚠️ **AND THE STANDING LESSON, NOW MEASURED THREE TIMES: a plan-file path is a MUTABLE SLOT, not an archive.** ⇒ 📌 **Every future law section citing a plan file states the generation and copies its binding content inline. A citation to that path is provenance, ⛔ never a live pointer.**

📌 **THIS SECTION IS BORN WITH ITS NAMESPACE PREFIX (`AS-§21`), per the `KBD-§N` / `NAV-§N` precedent and `FT-§12b`'s structural fix.** ⚖️ Cite as `AS-§21.4`, never as a bare `§21` — **a bare `§21` already resolves to `SC-§21` (guard placement) in this same file.**

### AS-§21.0 ⚖️ JONATHAN'S THREE RULINGS — DECIDED VIA `AskUserQuestion`, ⛔ BINDING, ⛔ NOT RE-OPENABLE BY ANY AGENT

1. ⛔ **BUILD IT ALL IN ONE BATCH; MEASURE AT THE END.** He was offered the measure-first sequencing (take Stage 5's numbers, *then* write code against a known budget) and **declined it**. ⇒ **No task may re-introduce a measurement gate ahead of the authoring lane.**
2. ⭐⛔ **RAISE THE CONTEXT LIMIT RATHER THAN TRIM TEACHING CONTENT. `ContextTokens` 2048 → 3072.** ⚠️ **`SiegeLlamaSubsystem.h:210` was FROZEN BY HIS OWN EARLIER RULING** (*"It is not a tuner's lever; it moves on a measurement, on Jonathan's call"*) — ⇒ ⭐ **THE ONLY PERSON WHO COULD UNFREEZE IT DID. He is named as the author in the constant's own comment, and an agent proposing a further move needs his word again.**
3. ⛔ **BOTH LANES RUN IN PARALLEL** — this schema batch, while he writes **TASK-482** (60–80 orders in his own words) and rules **TASK-483** (the two contracts). ⇒ **TASK-482 and TASK-483 are UN-HELD and are in front of him now.**

> ### ⭐⛔ **RULINGS 1 AND 2 INTERLOCK, AND THAT INTERLOCK IS THE ENTIRE REASON "BUILD NOW" IS SAFE. RECORD THE REASONING, NOT JUST THE VERDICT.**
> **"Build now" was risky for exactly ONE reason: Zone A's remaining growth budget is *17 characters* — asserted in a SHIPPED test** (`Tests/SiegeAssistantZoneATest.cpp:638-641`, `ZoneAGrowthBudgetChars = 325` against `DeclaredD4Delta = 308`) — **against a minimum package of ~157 chars.** ⇒ **Writing code against that budget would have meant discovering the overflow at the gate.** ⭐ **Raising the ceiling dissolves the constraint BY CONSTRUCTION, so no line in this batch is authored against an unknown budget.** ⚖️ **Neither ruling is safe alone: (1) without (2) writes into a 17-char hole; (2) without (1) buys headroom nobody spends.**

### AS-§21.1 ⭐ THE DIAGNOSIS — THE TWO SENTENCES FAIL FOR **OPPOSITE** REASONS, AND ONLY ONE OF THEM IS ABOUT THE MODEL

**⭐ SENTENCE A — *"send all units except miners to defend the castle"* — ALREADY WORKS. ONE PROMPT LINE BLOCKS IT.**
- ✅ `{"intent":"guard","who":{"all_except":["miner"]},"where":"own_castle","when":"now"}` **parses, validates, selects and executes TODAY** (shipped `5a07b96`; `AS-§20.1` says in terms *"JONATHAN'S OWN EXAMPLE IS FULLY SERVED BY THIS"*). Miners really are subtracted — `AMinerUnit::CanTakeZoneOrders()` returns `true` (`MinerUnit.h:414`), so they are IN the eligible set and can therefore be taken OUT of it.
- ⛔ **But `defend` is not an intent symbol.** It is a keyboard stance reachable only through the **army-wide** `fallback`, and the prompt actively routes the word away: **`SiegeAssistantVocabulary.cpp:243`** teaches *"defend = ambiguous between guard and fallback -> ask which_intent."* **(VERIFIED first-hand at the artifact, 2026-08-05.)**
- ⭐⭐ **THAT NOTE CONTRADICTS A NEWER, UNCONDITIONAL RULE.** **`SiegeAssistantSnapshot.cpp:1050`** (verified): *"If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally."* ⇒ **The note's stated harm — *"picking one would move an army the player never mentioned"* (`SiegeAssistantVocabulary.cpp:192-195`) — is STRUCTURALLY IMPOSSIBLE when a selection is present, because `fallback` is already unreachable there.**
- ⚖️ **THE GENERAL SHAPE, AND IT IS THE PART WORTH KEEPING: the note was TRUE WHEN WRITTEN; a later line made its antecedent false; nobody re-read it.** ⇒ 📌 **A prompt line is not a constant — it is an assertion about the rest of the prompt, and it can be falsified by an edit somewhere else in the file.** ⛔ **Every future prompt-line addition that conditions on a harm must name the OTHER lines that could remove that harm.**

**⛔ SENTENCE B — *"send all units currently in an ancient ground to attack a castle"* — STRUCTURALLY INEXPRESSIBLE. AND THE FIX COSTS ZERO ZONE-C BUDGET.**
- ⛔ **There is NO spatial predicate anywhere.** `who` has four shapes, all type-symbol shapes; `FSiegeAssistantCommand` carries no region; **the selector uses unit position ONLY TO SORT** (`SiegeAssistantComponent.cpp:1977` computes `DistSq`, consumed only by the sort at `:1986`). `Where` is a **destination**, read at three sites, all feeding `CreateUnitGroup`, and **never read by the selector**.
- ⭐ **THE INSIGHT THAT MAKES IT CHEAP: `{"in":"ancient_ground_near"}` NAMES A PLACE SYMBOL, NOT UNITS.** That is the **identical operation the model already performs for `where`** — and `where` **has never had positional evidence either.** `SiegeAssistantCommand.h:117-120`: *"Every field is a SYMBOL, never a coordinate… the model sees `ancient_ground_near` and never sees a number that means a position."*
- ⭐⭐ **THIS IS THE COORDINATE AIRLOCK APPLIED TO THE SELECTOR INSTEAD OF THE DESTINATION.** Membership is answered **game-side** by `AAncientGround::IsPointInZone` (`AncientGround.cpp:143-151` — a 2D XY box, two float compares, no collision primitive), exactly as destination is answered game-side by `ResolvePlace`. ⇒ ✅ **Zone C is BYTE-UNCHANGED and no per-unit or per-region datum is ever printed. The whole prompt cost lands in Zone A's schema mirror, and it is a handful of characters.**

### AS-§21.2 ⛔ THE `defend` NOTE REPAIR — AND THE `AS-§12f` SURVIVAL ARGUMENT, WRITTEN **INTO** THE LAW RATHER THAN ASSUMED

**The edit — ⚠️ LOCATE BY SYMBOL, ⛔ NEVER BY LINE OFFSET (`SC-§18c`):**
```diff
-… take who = none. defend = ambiguous between guard and fallback -> ask which_intent.
+… take who = none. defend with units -> guard. defend alone -> ask which_intent.
```
**Δ = −5 chars. Zone A 5424 → 5419.** House arrow notation preserved; it **REMOVES a competitor** rather than adding emphasis — the only lever with recorded evidence behind it (`AS-§20.4`: *"THE WAVE ALREADY MEASURED THAT A DECISION-ORDERING RULE BEATS AN EXEMPLAR HERE"*).

> ### ⛔ **THE LAW THIS EDIT MUST SURVIVE — `AS-§12f`, `CONVENTIONS.md:1208-1209`, ⚠️ NOW QUOTED VERBATIM:**
> > ***"THE LAW — IT BINDS EVERY FUTURE PROMPT-LEVEL TASK ON THIS MODEL: NO SUCH TASK MAY BE JUSTIFIED BY NAMING THE ROWS IT WILL FIX."*** … ***"RATE IS UNDER PROMPT CONTROL; SELECTION IS NOT."***
>
> ⚠️ **CORRECTED 2026-08-05 (`qa/TASK-550.md` NIT-2, raised by TASK-541 and confirmed at both artifacts): the earlier rendering compressed *"NO **SUCH** TASK"* into *"NO PROMPT-LEVEL TASK"* while keeping the quote marks.** ⚖️ **The substance was identical and nothing turned on it — which is exactly why it is worth fixing: a quote inside quote marks is a claim about BYTES, and a compression is a paraphrase that has lost its own label.** (The line range was correct; only the transcription was not.)

⭐ **IT SURVIVES BECAUSE ITS CASE NAMES NO ROW.** The justification, in full, and it is the only justification permitted:
> **Two shipped prompt lines assert contradictory things about the same input class. One is unconditional; the other is conditioned on a harm the first makes impossible. The deliverable is the REMOVAL OF A CONTRADICTION — verifiable as a diff, at NEGATIVE character cost.**

- ⛔⛔ **NO ACCURACY FIGURE MAY BE ATTACHED TO THIS EDIT.** ⛔ Not in the task, not in the handoff, not in the QA report, not in the commit message. **A prediction here would be the exact defect `AS-§12f` closed** — and the honest statement is that **we do not know what the model emits for either sentence, because nobody has ever seen it** (which is what `AS-§21.8` exists to fix).
- ✅ **THE DELIVERABLE IS THE DIFF.** A test asserts the `[notes]` line no longer contains `ambiguous between guard and fallback` and that the table is exactly 5 chars shorter. **That is the whole claim.**

### AS-§21.3 ⛔ REJECTED: MAKING `defend` A REAL **EIGHTH INTENT** — RECORDED WITH THE REASON SO IT IS NOT RE-PROPOSED

**It would be the ONLY intent carrying an implicit `where`** (`defend` ⇒ *your own castle*), giving **two spellings for one order** — `{"intent":"defend","where":"none"}` and `{"intent":"guard","where":"own_castle"}` meaning the identical thing. ⚖️ **That is precisely the ambiguity the seven-verb split exists to remove**, and it would additionally: rewrite the `INTENT =` schema line, widen the grammar's intent alternation, invalidate every corpus row's expected JSON for the class, and force `SiegeAssistantIntentTakesSelection` to grow an eighth case. ⇒ ⛔ **A 5-character deletion and an 8th enum value are not two options at different sizes; they are a repair and a schema change, and only one of them is what the evidence supports.**

### AS-§21.4 ⚖️ THE REGION-BEARING RULING — **A PLACE IS REGION-BEARING IFF A SHIPPED `IsPointInZone` ANSWERS FOR IT**

> ### ⛔ **THREE OF THE SEVEN PLACES QUALIFY. THE OTHER FOUR WOULD EACH NEED AN *INVENTED RADIUS* — A NUMBER NOBODY CHOSE — WHICH IS THE CLASS `AS-§20.1` MAKES STRUCTURALLY UNREACHABLE RATHER THAN MERELY UNIMPLEMENTED.**

| place symbol | region-bearing? | the primitive that answers, or why not |
|---|---|---|
| `ancient_ground_near` | ✅ **YES** | `AAncientGround::IsPointInZone` (`AncientGround.cpp:143-151`) — 2D XY box from `GetZoneHalfExtent()` |
| `ancient_ground_far` | ✅ **YES** | same class, the far instance |
| `mid` | ✅ **YES** | `ACaptureZone::IsPointInZone` (`CaptureZone.cpp:112-118`) — the same 2D XY box idiom, deliberately |
| `own_castle` | ⛔ no | no region primitive. *"In the castle"* would need an invented radius |
| `enemy_castle` | ⛔ no | same |
| `nearest_mine` | ⛔ no | same — and it is a **moving referent** ("the best mine for the player NOW"), so its region would change under the player |
| `hero` | ⛔ no | same, and worst: the hero moves every frame |

- ⛔ **AN AGENT MAY NOT INVENT A RADIUS FOR THE FOUR.** ⚖️ *"How far from the castle counts as at the castle?"* is a **product question with no measured answer**, and a number picked to make a feature compile is the defect `AS-§20.1` names. ⇒ **If Jonathan later wants castle-region selection, he supplies the number or approves one — it is his call, not a tuner's.**
- ✅ **THE LIST IS GENERATED, NEVER HARD-CODED.** `PlaceVocabulary` (`SiegeAssistantSnapshot.cpp:52-61`) stays the **single owner** and gains **one `bHasRegion` column**. ⛔ **Zone A never prints that column, so the column itself moves ZERO bytes.**
- ⛔⛔ **TWO CONSUMERS, TWO DIFFERENT SOURCES — AND MIXING THEM IS A QA FAIL. ⚠️ CORRECTED 2026-08-05 (`qa/TASK-550.md` RULING 2); the earlier wording, and the board item derived from it, led TASK-547 toward exactly the failure below and 547 REFUSED it correctly.**
  - ✅ **ZONE A's `ZONE = ` LINE IS GENERATED FROM THE `PlaceVocabulary` TABLE'S `bHasRegion` COLUMN — a `static constexpr` table, read at build time, ⛔ NEVER from `GetRegionPlaceNames()`.** ⚖️ **`GetRegionPlaceNames()` is a `Transient` member filled by `Capture()` — PER-MATCH STATE, legitimately shorter on a map with one ancient ground — and `BuildZoneA`'s own contract is that it *"READS NO MEMBER STATE, BY CONSTRUCTION."*** ⛔ **A state-dependent Zone A destroys the measured 77.1 % KV prefix reuse SILENTLY, surfacing as a latency regression rather than a wrong answer.** ⭐ **That is why it is generated and yet fixed: *generated, never hard-coded* means "no literal list of three in the file", ⛔ it does NOT mean "read the live member".** The `static_assert` beside the table is what makes the empty case impossible.
  - ✅ **THE GRAMMAR's `zone` ALTERNATION IS GENERATED FROM THE LIVE `GetRegionPlaceNames()`**, exactly as `where` already is (`SiegeAssistantGrammar.cpp:510-525`). ⇒ **A region the map lacks stays UNSAMPLABLE even though the prompt names it.**
  - ⚖️ **THIS IS THE SPLIT THE `places` BLOCK ALREADY SHIPS, AND IT IS THE GENERAL RULE: ZONE A PRINTS THE FULL FIXED VOCABULARY; THE GRAMMAR ENFORCES WHAT EXISTS THIS MATCH.** ⛔ **Any future prompt line that wants a live list is asking the byte-frozen prefix to vary — refuse it there and put the enforcement in the grammar.**
- ⛔ **SNAPSHOT-TIME GEOMETRY, EXECUTION-TIME MEMBERSHIP.** The half-extents are captured with everything else; the **membership test runs in the executor** — ⚖️ **the units move, the grounds do not**, and a unit that walked out of the ground between capture and execution must not be selected.
- ⛔ **NO NEW SNAPSHOT TRAVERSAL.** All three actors are **already held by existing passes** (`NearGround`/`FarGround` from pass 3, `Zone` from pass 5) and expose `GetZoneHalfExtent()` publicly (`AncientGround.h:117`, `CaptureZone.h:131`). **A new `TActorIterator` in this batch is a finding.**
- ⛔ **THE GRAMMAR GATE IS THE REGION LIST, ⛔ NOT `bHasKinds`.** *"everyone in the mid"* names **no kind**. ⚠️ **And the alternation is emitted ONLY when the region list is non-empty** — an empty alternation leaves `zone` undefined and **breaks the whole grammar**, which llama.cpp answers by generating UNCONSTRAINED (the `at_least` disaster that cost TASK-413 two of its six bars).
- ⛔ **RULE NAMES ARE `[a-zA-Z0-9-]` AND llama.cpp STOPS AT AN UNDERSCORE** (verified, `SiegeAssistantGrammar.cpp:636-645`, the shipped `exceptlist` departure). ⇒ **`inplace` and `zone` are one word each, by construction. ⛔ Do not "improve" them to `in_place` / `zone_list`.**

### AS-§21.5 ⛔ THE **FIFTH `who` SHAPE** — A SHAPE FOR AN EXISTING KEY, ⛔ NEVER A FOURTH TOP-LEVEL KEY

> ### **`"who": {"in": "ancient_ground_near"}` — a FIFTH shape for an existing key. The top-level key set does NOT change.**

- ⛔ **A fourth top-level key (`"in": …` beside `intent`/`who`/`where`/`when`) is REJECTED on this schema's own rules, exactly as `AS-§20.1` rejected `"except"`:** the parser validates an **EXACT key set** (`ValidateExactKeySet`) and the prompt law says **every key is always emitted** — so a new top-level key would have to appear in **every** emission, rewriting all seven few-shots, every corpus row's expected JSON, and the parser's key set **at once.**
- ✅ **STRICTLY ADDITIVE AT THE WIRE: every JSON that parses today parses BYTE-IDENTICALLY after.** ⭐ **That is the FIRST property the test suite checks** — not the last.
- **THE GRAMMAR ORDER IS PINNED AND ZONE A MIRRORS IT CHARACTER-FOR-CHARACTER** (`AS-§9c`, the mirror law):
  ```
  inplace ::= "{" "\"in\"" ":" zone "}"
  zone    ::= <generated, one alternative per live region-bearing place>
  who     ::= selection | except | inplace | "all" | "none"
  ```
  ⚠️ **`inplace` goes THIRD, after `except`** — the shipped order is `selection | except | "all" | "none"` (`SiegeAssistantGrammar.cpp:709-719`, verified) and **appending before the two bare strings keeps the object shapes together.** ⛔ **Zone A's `WHO =` line lists the five shapes in the GRAMMAR'S OWN ORDER; a test asserts they match.**
- **SEMANTICS, PINNED:** `RegionPlace` is meaningful **only** as a standalone `who` shape. ⛔ **`Kinds` non-empty AND `RegionPlace` set is a parse FAILURE (`RegionConflict`), never a merge** — *"send 10 footmen in the mid"* is a **different feature** (a filtered count), it was not asked for, and guessing which half to honour is the valid-shaped-wrong-command class. **`who:"none"` plus a region is the same failure. `all_except` plus a region is the same failure.**
- ⛔ **`""` PLUS THE THREE RESERVED WIRE SYMBOLS — `"none"` / `"all"` / `"now"` — INSIDE `in` ARE `BadRegion`.** ⚠️ **CORRECTED 2026-08-05 (`qa/TASK-550.md` RULING 4 / NIT-1): the original bullet enumerated `""` / `"all"` / `"none"` while justifying them as *"the three reserved wire symbols"* — but the third reserved symbol is **`now`**, not `""`. The enumeration and its own justification described different sets.** ✅ **TASK-545 shipped the SUPERSET (`""` + all three, `SiegeAssistantCommand.cpp:306-313`) and the gate RATIFIED it — every case the law enumerates behaves exactly as pinned.**
  - ⭐⛔ **AND THE SUPERSET IS LOAD-BEARING, ⛔ NOT MERELY HARMLESS — WHICH IS WHY THE ENUMERATION HAD TO BE FIXED RATHER THAN THE CODE TRIMMED TO MATCH IT.** `CanonicalizeSymbols` (`SiegeAssistantGrammar.cpp`) drops `NAME_None`, empty, `all` and `none` — it does ⛔ **NOT drop `now`.** ⇒ **The parser's `now` clause is the ONLY guard against it, not a redundant belt beside the grammar's.** ⛔ **DO NOT DELETE THE `now` CLAUSE FROM THE PARSER.** ⚠️ **No behaviour is at risk today (`PlaceVocabulary` cannot contain `now`) — which is precisely what would make the clause look deletable to a reader who trusted the old wording.**
- ⛔ **EMPTY-AFTER-REGION IS NOT A PARSE ERROR.** `ParseSiegeAssistantCommand` is **PURE** — no world, no roster, no snapshot — so *"did that region contain anybody?"* is a question it **structurally cannot answer**. It is the **executor's**, and the answer is a **loud refusal with the arithmetic in the log**, routed through the **existing unsupported-ask outcome**. ⛔ **NO NEW `ask` SYMBOL** (the ask alternatives are grammar the model samples from). ⛔ **NEVER a silent no-op** — *"nothing happened and nothing was said"* is the shape that reads as *"the assistant ate my order"*. **This is `AS-§20.1`'s EMPTY-AFTER-EXCLUSION clause, applied verbatim.**
- ⛔⭐ **AND THE HARD RULE THE WHOLE FEATURE RESTS ON: A REGION NAMED AND NOT RESOLVED IS A REFUSAL — ⛔ NEVER AN UNFILTERED ORDER.** *"Send everyone in the mid"* degrading to *"send everyone"* is an army moving that the player never asked to move. ⚖️ **Fail closed. There is no acceptable open-failure mode here.**

### AS-§21.6 ⛔ THE **THIRD CROSS-FIELD CHECK** — AND IT EARNS ITS PLACE ON CHECK 2's IDENTICAL ARGUMENT

**`RegionPlace` is legal ONLY where `SiegeAssistantIntentTakesSelection(Intent)` is true — i.e. `send` / `guard` / `ambush` / `follow`. ⛔ THE PARSER REFUSES IT FOR `charge` / `fallback` / `rally` WITH `RegionConflict`.**

- ⚖️ **THE ARGUMENT IS NOT "BE CONSERVATIVE" — IT IS READ OFF THE EXECUTOR, AND IT IS WORD-FOR-WORD THE ONE THAT JUSTIFIED CHECK 2 (`AS-§20.1`):** `Charge`/`Fallback` execute through **`ASiegePlayerController::ApplyArmyWideStance`** — the same shipped API the `T` and `E` keys call — and `Rally` calls **`AHeroCharacter::Rally()`**. ⛔ **None of the three passes through the selector.** ⇒ **A region handed to them would be PARSED AND THEN SILENTLY DROPPED**, and *"fall back, but only the ones in the mid"* would execute as *"fall back, EVERYONE."*
- ⭐ **THE PARSER ALREADY CARRIES A COMMENT SAYING for an army-wide verb *"the executor ignores the selection"* — harmless for a selection, CATASTROPHIC for a FILTER.** **An exclusion and a region are both filters, and both fail the same way for the same reason.** ⇒ ✅ **This is not a new principle; it is the second instance of one, which is exactly why it earns its place instead of being special-cased.**
- ⛔ **REFUSE AT THE PARSER, gated by the existing `SiegeAssistantIntentTakesSelection` — ⛔ no second intent-classification path.** **Silently ignoring `RegionPlace` on any path is an automatic QA FAIL.**
- 📌 **CONSEQUENCE, DELIBERATE AND ON THE PLAYTEST SHEET: *"everyone in the mid, fall back"* is REFUSED BY DESIGN (`region_conflict:fallback`) — the same verdict, for the same reason, as `DEV-08`.** ⚠️ **It is not a bug and must not be reported as one** (`AS-§21.11`).

### AS-§21.7 ⭐ THE ZONE-A BUDGET — ⛔ **DERIVED, NOT MEASURED** — AND THE CONTEXT-CEILING RAISE THAT DISSOLVES IT

**⛔ THE RECORD IS CORRECTED, AND IT IS A REASON, ⛔ NOT A FINDING (`AS-§12g` forbids treating a derivation as a reading):**
- `CONVENTIONS.md:1859` (`AS-§20.4`) derives the **325-char** batch budget from `zoneA_tok ≤ 1389` minus a **1139** baseline. **But the ladder MEASURED `1139 → 1356 → 1362`** (`:1229-1230`), and `AS-§12f` records *"27 of the allowed tokens still unspent"*. ⇒ **The room was ~27 tokens, not ~250** — and **TASK-521 then spent +308 chars against it.**
- ⛔⛔ **THIS IS A DERIVATION FROM TWO MEASURED NUMBERS, ⛔ NOT A READING. IT MAY BE CITED ONLY AS *THE REASON THE CEILING IS BEING RAISED*, ⛔ NEVER AS A FINDING, ⛔ NEVER AS AN OVERFLOW REPORT, AND ⛔ NEVER AS EVIDENCE THAT ANY SHIPPED PROMPT IS BROKEN.**
- **TWO MITIGATIONS, STATED SO THE PICTURE IS HONEST IN BOTH DIRECTIONS:** the **code-enforced** ceiling is looser than the doc one (`2048 − 400 − 96 − 48 = 1504`), and **nothing has ever reported `PROMPT BUDGET OVERFLOW`** — ⚠️ **but that assertion may never have EXECUTED, and `SC-§32` is explicit that a mechanism never observed to function is not known to function. Its silence is not evidence.**
- ⇒ ⭐ **RAISING `ContextTokens` IS THE RIGHT CALL REGARDLESS OF WHAT THE MEASUREMENT LATER SAYS, WHICH IS EXACTLY WHY BUILDING NOW IS SAFE.**

**THE RAISE — `SiegeLlamaSubsystem.h:210`, `2048 → 3072`, with the freeze comment REWRITTEN to record WHO unfroze it and WHY.** Occupancy today is ~`1362 + 400 + 96 + 48 = 1906` of 2048 (**~142 spare**); at 3072 the slack is **~1045**. ⚠️ **Both figures are DERIVED and are labelled as such in the comment.**

**⚠️ TWO CONSEQUENCES TO *HANDLE*, ⛔ NOT TO DISCOVER:**
1. **KV-CACHE VRAM SCALES WITH CONTEXT.** Derived (⛔ not measured): 36 layers × 8 KV heads × 128 head-dim × 2 (K+V) × 2 bytes ≈ **147 KB/token** ⇒ ~302 MB at 2048, ~453 MB at 3072, **≈ +151 MB**. Against the recorded **6893 MiB free at `tier=full`** this is comfortable; at `tier=partial` (**3038 MiB free**) it is tighter. ⚠️ **The 128 head-dim is an ASSUMPTION; Stage 5 confirms it.**
2. ⛔ **THE TIER SELECTOR KEYS ON FREE VRAM AND ITS ARITHMETIC IS RE-CHECKED IN THE SAME TASK.** `FSiegeLlamaWorker::ChooseTier` promotes to `FullOffload` when `FreeMiB >= FullOffloadCostMiB(2703) + VramGameReserveMiB(768)` and to `Partial` at `PartialOffloadCostMiB(1264) + 768` (verified, `SiegeLlamaSubsystem.cpp:76-104`, `:885-904`). ⭐ **A BIGGER CONTEXT MUST NOT PUSH A MACHINE INTO A TIER IT CANNOT RUN** — and *"the tier selector picks a tier it cannot run in"* is **already a named defect on this project** (`FT-§6`, `CONVENTIONS.md:2023`).

> ### ⛔⛔ **AND THE RULING THAT KEEPS THAT RE-CHECK HONEST: `FullOffloadCostMiB` AND `PartialOffloadCostMiB` ARE **MEASUREMENTS**. ⛔ THEY MAY NOT BE MOVED BY ARITHMETIC.**
> - **Both were measured at `ContextTokens = 2048`** (`SiegeLlamaSubsystem.cpp:52-77`, TASK-413's tier table, with `AS-§12h`'s spread travelling with them). **Raising the context makes them stale-LOW — i.e. stale in the UNSAFE direction**, because the selector promotes on them.
> - ⛔ **Editing `2703` to `2854` is a DERIVATION WEARING A MEASUREMENT'S AUTHORITY** — the precise defect `AS-§23` names (*"a measurement's BASE is part of the measurement"*) and `AS-§12g` bans.
> - ✅ **THE SANCTIONED SHAPE: add ONE new, separately-named, explicitly-DERIVED constant — `ContextGrowthVramReserveMiB` — and ADD it to BOTH comparisons.** Its comment states: the operands, that it is derived and not measured, that it errs HIGH on purpose (`AS-§19`'s safe-direction rule: a gate that promotes must assume the LARGER cost), and that **Stage 5's live `vram_delta` reading replaces it.** ⇒ **The measured constants keep their provenance intact, and the uncertainty is visible in the source rather than folded into a number that looks measured.**
> - ⛔ **SCOPE FENCE: the CPU-tier fall-through defect (the selector returning `CpuOnly`, a tier that MEASURABLY FAILS — 2 of 5 generations hit the ceiling) IS NOT FIXED IN THIS BATCH.** It is a separate, known concern. **This task re-checks arithmetic against a new context size and nothing else.**

**ZONE A's BUDGET FOR *THIS* BATCH — a discipline, ⛔ not a derived limit:**
- ⛔ **NO NEW FEW-SHOT *SENTENCES*. RULE LINES AND SCHEMA-MIRROR LINES ONLY.** ⚖️ **`AS-§20.4`'s reasoning carries over UNCHANGED and the binding leg is still the first one: 🔒 `Docs/Data/assistant_eval_holdout2.csv` IS SEALED, the few-shot law requires literal disjointness from all three corpora, and TASK-430 proved a new sentence cannot be cleared without opening them.** ✅ **A rule line requires no certificate. The seal stays unspent, for free.**
- **BOARDED CEILING: +250 chars for the whole batch, measured against the RE-BASED 5419** (post-`AS-§21.2`). ⛔ **The snapshot task reports the delta COMPONENT BY COMPONENT and a test asserts each component's exact length** — the `ZoneATest.cpp:617-641` idiom, which exists so a transcription error fails with the component named. ⛔ **Over budget ⇒ STOP and escalate; do not trim a shipped few-shot to make room.**
- ⛔ **TOKENS ARE NOT RE-COUNTED BY ARITHMETIC. `zoneA_tok = 1139` and the 77.1 % KV-reuse figure stay labelled STALE — PENDING RE-MEASUREMENT ON THE MODEL.** Only `Siege.Llama.SpikePrompt` prints the real figure, and Stage 5 is where it is printed.
- ⚠️ **`AS-§21.2`'s −5 BREAKS `Siegebound.Assistant.ZoneA.MeasuredCharCount` (5424 → 5419) AND `TwoLaneByteEquality` — EXPECTED, NOT A REGRESSION.** ⛔ **Re-base against a NAMED, DATED baseline that states in its own message that the spike lane (5116) has diverged and why (D4).** ⛔ **NEVER re-copy the builder's output into the fixture** — a test whose transcription is copied from its subject is a guardrail that reports SAFE.

### AS-§21.8 ⛔ LOG THE RAW MODEL OUTPUT — THE ONE FAILURE CLASS WITH **NO ARTIFACT**

> ### ⛔ **THE MODEL'S OUTPUT IS LOGGED NOWHERE IN THE SHIPPED LANE. VERIFIED AT THE ARTIFACT 2026-08-05: `USiegeAssistantComponent::HandleModelCompletion` (`SiegeAssistantComponent.cpp:946-980`) takes `Output` and passes it straight to `ParseSiegeAssistantCommand` — no `UE_LOG` on any success path prints a byte of it.**

- ⭐ **EVERY OTHER OUTCOME NAMES ITSELF.** A parse failure logs its reason code; a subsystem failure logs its error; a stale turn logs its turn id. **The ONE class with no artifact is *"well-formed but means something nobody asked for"*** — which is **precisely the class `AS-§20.1` calls an automatic QA FAIL and which `AS-§20.4` says must be FOUND, NOT GUESSED.**
- ⇒ **ONE `UE_LOG`, before `ParseSiegeAssistantCommand`, at verbosity `Log`.** ⛔ **NOT `Verbose`** — the report arrives after the session and nobody reproduces with a raised verbosity. ⛔ **NOT `Warning`** — the automation runner reads `Warning` as failure and this line fires on the HAPPY path.
- ⛔ **`Output` STAYS A LOCAL.** No new member, no new state, no retention — **`AS-§1`'s "never keep a word of it" is untouched**, and the file's own comment (`:975-976`) says why. **Zero prompt bytes.**
- ⭐ **THIS IS THE BATCH'S CHEAPEST HIGH-VALUE ITEM: with it landed, Stage 5's log shows EXACTLY what the model emitted for Jonathan's two sentences — which either CONFIRMS the `AS-§21.1` diagnosis or FALSIFIES it.** ⚖️ **A diagnosis that can be falsified by one log line and is not is not a diagnosis, it is a preference.**

### AS-§21.9 📌 PINNED CROSS-TASK SIGNATURE REGISTRY — ⛔ EVERY TASK COMPILES AGAINST THIS **CHARACTER-FOR-CHARACTER**

⚠️ **UBT compiles the whole module.** "Improving" a pinned name breaks the link and is an **automatic QA FAIL.**

```cpp
// ── SiegeAssistantCommand.h ───────────────────────────────────────────────

struct FSiegeAssistantCommand
{
    // ... the SEVEN shipped UPROPERTY members, UNCHANGED, IN THEIR SHIPPED ORDER ...
    // (Intent · Kinds · Counts · Where · TriggerKind · TriggerAtLeast · ExcludeKinds)

    /** The region whose occupants are the selection. NAME_None == no region named.
     *  ⚠️ THE EIGHTH `UPROPERTY`, APPENDED AFTER THE SEVEN — never inserted among them.
     *  Non-None ONLY when Kinds AND ExcludeKinds are both empty, and ONLY when
     *  SiegeAssistantIntentTakesSelection(Intent). */
    UPROPERTY()
    FName RegionPlace = NAME_None;
};

namespace SiegeAssistantJsonKeys { inline constexpr const TCHAR* In = TEXT("in"); }

namespace SiegeAssistantReason
{
    inline constexpr const TCHAR* RegionConflict = TEXT("region_conflict");  // with a selection, an exclusion, "none", or an army-wide intent
    inline constexpr const TCHAR* BadRegion      = TEXT("bad_region");       // "", "all" or "none" inside {"in": …}
}

/** ⛔ THE NAME IS UNCHANGED (AS-§20.1 ruled it keeps its name) and gains a FIFTH
 *  TRAILING DEFAULTED PARAMETER, on the identical precedent as the fourth. */
bool SiegeAssistantValidateSelection(const TArray<FName>& Kinds, const TArray<int32>& Counts, FString& OutError,
                                     const TArray<FName>& ExcludeKinds = TArray<FName>(), FName RegionPlace = NAME_None);

// ── SiegeAssistantGrammar.h ───────────────────────────────────────────────
/** ⛔ A THIRD, DEFAULTED parameter — every existing call site stays byte-identical.
 *  ⚠️⛔ AND THAT SENTENCE IS THE BENEFIT, NEVER THE AUDIT (SC-§33, adopted 2026-08-05
 *  ON THIS EXACT LINE): the module's ONE shipped caller, ComposeTurnGrammar, omitted
 *  the argument and would have shipped a grammar with NO `inplace` rule — compiling
 *  clean, suite green, {"in":…} unsamplable. TASK-548 caught it by reading.
 *  ⇒ THE ADDING TASK PASTES A CALL-SITE GREP; THE GATE RE-RUNS IT. */
static FString Build(const TArray<FName>& UnitKinds, const TArray<FName>& PlaceNames,
                     const TArray<FName>& RegionPlaceNames = TArray<FName>());

// ── SiegeAssistantSnapshot.h ──────────────────────────────────────────────
const TArray<FName>& GetRegionPlaceNames() const;
bool ResolvePlaceRegion(FName Place, FVector& OutCentre, FVector2D& OutHalfExtent) const;

// ── SiegeAssistantRegionStatics.h (NEW) ───────────────────────────────────
struct GITCLAUDEUNREALTEST_API FSiegeAssistantRegionStatics
{
    /** 2D XY box, Z IGNORED. Boundary INCLUSIVE (<=), mirroring AAncientGround::IsPointInZone. */
    static bool IsPointInRegion(const FVector& Point, const FVector& Centre, const FVector2D& HalfExtent);
};
```

- ⚠️ **THE ORDINAL, CORRECTED 2026-08-05 (`qa/TASK-550.md` NIT-3) — AND SAY WHICH COUNT YOU MEAN:** `RegionPlace` is **the EIGHTH `UPROPERTY` member in declaration order** (`Intent` · `Kinds` · `Counts` · `Where` · `TriggerKind` · `TriggerAtLeast` · `ExcludeKinds` · `RegionPlace` — a `UPROPERTY` grep on the struct returns **8**). ⚠️ **It is *"the seventh FIELD"* only under the prose count the shipped header uses (`SiegeAssistantCommand.h`, the *"SEVEN FIELDS"* block), which collapses one adjacent pair — and that count has been ONE LOW since `AS-§20.1`'s pin said *"the five shipped fields"* when six `UPROPERTY`s were shipped.** ⇒ ⛔ **Cite the countable thing (`UPROPERTY`s in declaration order) or name the convention you are counting under; ⛔ never an unqualified ordinal.** ✅ **NOTHING ELSE MOVES: the load-bearing claim — appended LAST, no prior member relocated, wire layout `uint8`/`int32`/`FName` preserved — is TRUE and was verified at the artifact.**
- ⛔⛔ **`SiegeAssistantValidateCommand` IS **REJECTED** AS A NAME AND THE REJECTION IS RECORDED HERE BECAUSE IT WAS PROPOSED.** **No such symbol exists** (verified: 26 references, all `SiegeAssistantValidateSelection`, across 5 files). **`AS-§20.1` explicitly ruled the function KEEPS ITS NAME**, and renaming it now would produce a 26-site diff across two files this batch does not otherwise need to touch, for **zero behaviour** — destroying the *"the diff is additive"* review property that is this batch's main safety argument. ⇒ **The name stays.**
- ⚠️ **THE FIFTH TRAILING DEFAULT IS ACCEPTED *WITH ITS COST NAMED*, AND A SIXTH IS FORBIDDEN.** The shipped comment (`SiegeAssistantCommand.h:464-467`) already warns that *"a caller that omits the argument silently validates NOTHING"*. ⇒ ⛔ **ANY CALLER HOLDING A WHOLE `FSiegeAssistantCommand` MUST PASS BOTH `Command.ExcludeKinds` AND `Command.RegionPlace`** — a QA criterion, not a style note. 📌 **The right long-term shape is an overload taking `const FSiegeAssistantCommand&`; it is RECORDED as a follow-up and ⛔ NOT taken in this batch (churn at a gate). The NEXT field-shaped addition takes the overload instead of a sixth default.**
  - ⛔⭐ **AND THIS BULLET IS NOW SUPERSEDED AS A *CONTROL* BY `SC-§33` (THE TRAILING-DEFAULT LAW, adopted 2026-08-05).** ⚖️ **It is kept because it is true — and because it is the evidence: this warning was written BEFORE the second instance, in the document the batch was authored from, and it did not prevent it.** ⇒ **Prose describing the hazard is not a control. The control is the pasted call-site grep in the handoff plus the gate's re-run, and `SC-§33` owns it.**
- 📌 **M8: `RegionPlace` is an `FName`, so `FSiegeAssistantCommand` stays `uint8` / `int32` / `FName` only and `AS-§3`'s *"M8 P2 takes this struct as-is over the wire"* property SURVIVES.**
- 📌 **`FSiegeAssistantRegionStatics` SHIPS WITH ZERO CALL SITES** and gains its only caller in the executor task — **the `FSiegeNavDiagnostics` precedent (`NAV-§` RULING 2): a diff that is one new file pair proves behaviour-freedom in ONE LOOK.**

### AS-§21.10 NAMING + FOLDER LAW (this batch's cross-task contract)

| thing | law |
|---|---|
| new statics pair | **`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantRegionStatics.{h,cpp}`** — plain static library, ⛔ **NOT a UObject** (`FSiegeCombatStatics` precedent, `SiegeCombatStatics.h:23`), `GITCLAUDEUNREALTEST_API`. |
| tests | ⚠️ **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp` ALREADY EXISTS** (created by the ASSISTANT-EXCLUDE batch) — this batch **EXTENDS** it. ⛔ **Do NOT create a second file.** New test names under **`Siegebound.Assistant.Selection.<Name>`**; `#if WITH_DEV_AUTOMATION_TESTS`, `EditorContext \| EngineFilter`. |
| QA gate report | **`.claude/pipeline/qa/TASK-550.md`** — named for the **GATE** task (`qa/TASK-506.md` / `qa/TASK-525.md` / `qa/TASK-537.md` precedent, `SC-§29`). ⛔ **No other gate file exists for this batch; do not grep for one.** |
| touched, existing | `SiegeAssistantVocabulary.cpp` · `SiegeAssistantCommand.{h,cpp}` · `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantComponent.{h,cpp}` · `Tests/SiegeAssistantSelectionTest.cpp` · `Tests/SiegeAssistantZoneATest.cpp` · `Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h` (+ its `.cpp` for the tier constant only) |
| ⛔ **NOT touched** | ⛔ **`Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` (D4 — it is `FT-§16`'s in-flight instrument and it stays declared-divergent)** · 🔒 **`Docs/Data/assistant_eval_holdout2.csv` (SEALED)** · `Docs/Data/assistant_eval_holdout.csv` (seal SPENT — still not an editing surface) · `Docs/Data/assistant_eval_dev.csv` (⛔ **no corpus edit in this batch**) · `SiegePlayerController.{h,cpp}` · `HeroCharacter.{h,cpp}` · `AncientGround.{h,cpp}` · `CaptureZone.{h,cpp}` · `cards.csv` / `DT_Cards` · **any `.umap`** · **any `Build.cs`** · any `Content/` asset |

⛔ **The `names:` block of each task is the single source of truth (the standing cross-discipline rule). Every symbol above appears there character-for-character.**

### AS-§21.11 ⚠️ KNOWN, DESIGNED OUTCOMES — ⛔ **NONE OF THESE ARE BUGS**, and they go on Jonathan's playtest sheet so a report of one is read correctly

1. ⛔ **`everyone in the mid, fall back` IS REFUSED BY DESIGN** — `region_conflict:fallback`, `AS-§21.6`. **Same verdict, same reason, as `DEV-08`.**
2. ⛔ **OGRES, SAPPERS AND CLERICS NEVER TAKE ZONE ORDERS AT ALL** — `ASummonedUnit::CanTakeZoneOrders()` returns `Profile == ECardProfile::Standard` **only** (verified, `SummonedUnit.cpp:2007-2014`). ⇒ **"all units except miners" correctly moves STANDARD units only, and miners (which override to `true`) are the thing being subtracted.** ⚠️ **Pre-existing design from the FOLLOW batch — it is NOT the exclusion failing, and it is NOT this feature.**
3. ⛔ **`own_castle` / `enemy_castle` / `nearest_mine` / `hero` ARE NOT SELECTABLE REGIONS** — `AS-§21.4`. *"everyone at my castle"* will not filter, by ruling.
4. ⚠️ **A REGION THAT CONTAINS NOBODY PRODUCES A LOUD REFUSAL WITH ARITHMETIC, NOT SILENCE AND NOT AN UNFILTERED ARMY** — that is the feature working.
5. ⛔ **NOT PROMISED: that the model reliably EMITS either shape.** `AS-§20.4` names this: *"a rule line is a WEAKER teaching signal than an exemplar for a brand-new output SHAPE the model has never seen — this is the batch's #1 accuracy risk."* **A fifth `who` shape is exactly that.** ⭐ **The strongest HONEST statement available: the model's job becomes TRANSCRIBING A PLACE SYMBOL IT ALREADY TRANSCRIBES FOR `where`, out of the same vocabulary it already reads in `[FORCES]`.** ⚠️ Jonathan pre-authorised a few-shot spend for exactly this case, but it needs a **mechanical absence certificate against all three corpora** and `holdout2` is sealed ⇒ **DEFERRED, NOT FORGOTTEN.**

### AS-§21.12 📌 M8 DECLARATION (batch-level; every code task repeats it verbatim)

⛔ **This feature adds no replicated property, no new replicated class, and no new relevancy tier.** ✅ **And the reason is structural: `RegionPlace` is an `FName` on a struct that is already `uint8`/`int32`/`FName`-only, `FSiegeAssistantRegionStatics` is a non-UObject static library, and the membership test runs where the selector already runs — on the authority.** *"There is nothing to declare"* only counts when it is stated.

