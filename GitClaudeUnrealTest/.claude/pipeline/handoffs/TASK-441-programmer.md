# TASK-441 — [W1-GUARD] Orderability accessors + the NON-ORDERABLE-KIND GUARD + automation tests

**Assignee:** gameplay-programmer · **Status on completion:** `ready-for-qa` · **Date:** 2026-08-03
**Law followed:** CONVENTIONS "Settings screen + the assistant CONFIRM STEP + the non-orderable-kind guard (2026-08-03)" **§2, §6, §8** · "In-match LLM command assistant" **§1, §3, §4, §9, §9c**

---

## 0. ⛔ THE TWO SENTENCES THIS HANDOFF IS MOST LIKELY TO GET WRONG, STATED FIRST

1. ⛔ **THIS GUARD DOES NOT MAKE `DEV-04` PASS THE EVAL, AND IT IS NOT PROGRESS ON BAR #5.**
   **The eval scores the model's EMITTED JSON; this guard refuses an EXECUTED ACTION.** It changes what the
   game *does*; it changes nothing about what the model *emits*. On `"send the catapults at the enemy base"`
   the model will still emit `{"intent":"send","who":[{"kind":"sapper","n":1}],"where":"enemy_castle","when":"now"}`
   and the eval will still score that row **FAIL**. **This is SHIPPED SAFETY, full stop.** Nothing in this
   handoff, in the code, or in the tests may be filed under an accuracy heading.
   ⛔ **Bar #5 stands where it stood: 20/25 against a gate of 22 AND zero refuse-class failures. It never cleared.**
   Wave 1 is un-gated **because Jonathan weighed the failure modes and overrode his own gate**, never because
   the ladder worked.
2. ✅ **THE TESTS RUN WITH NO MODEL RESIDENT — THAT IS THE POINT OF PUTTING THE RULE IN THE COMMAND LAYER.**
   Every assertion in `SiegeAssistantGuardTest.cpp` runs against a **hand-populated roster**: no GGUF, no
   `UWorld`, no `Capture()`, no PIE, no editor world required by the validator. That property comes directly
   from the pinned signature taking `TArray<FSiegeAssistantRosterEntry>` rather than the snapshot object.

**M8 DECLARATION DUTY, verbatim as required:** *"adds no replicated property, no new replicated class, no new
relevancy tier."* It is true here by construction: everything added is a `const` read of a client-local survey
object plus one pure free function over plain arrays. Nothing crosses the wire. (`FSiegeAssistantRosterEntry`
gains one `int32` — it is a `Transient` survey row that is re-derived from scratch on every capture and is not
replicated, so the statement holds unchanged.)

---

## 1. THE PUBLISHED SIGNATURES — TASK-443 COMPILES AGAINST THESE

Character-for-character as landed. **`SiegeAssistantSnapshot.h`, additive to the §9 pin, nothing renamed.**

```cpp
// ── USiegeAssistantSnapshot, public — declared in the §8 registry's order ──
bool  IsKindOrderable(FName Kind) const;        // reads the roster Capture() already tallies
int32 GetOrderableCount(FName Kind) const;      // 0 == not orderable

// ── file scope, AFTER the UCLASS. Plain `enum class`, NOT a UENUM (as pinned) ──
enum class ESiegeAssistantRejectReason : uint8 { None, KindNotOrderable, KindUnknown };

// ── free function, NOT a member. No API macro (the ParseSiegeAssistantCommand idiom) ──
bool ValidateCommandAgainstSnapshot(const FSiegeAssistantCommand& Command,
                                    const TArray<FSiegeAssistantRosterEntry>& Roster,
                                    ESiegeAssistantRejectReason& OutReason,
                                    FName& OutOffendingKind);
```

`SiegeAssistantSnapshot.h` now `#include`s `Siegebound/SiegeAssistantCommand.h` (for `FSiegeAssistantCommand`
and `ESiegeAssistantIntent`). That header is the sanctioned pure-data header — its own doc comment says every
consumer may include it without a heavy dependency — and there is no include cycle (`SiegeAssistantCommand.h`
includes only `CoreMinimal.h` + its own `.generated.h`).

### Behaviour TASK-443 must route

| Return | `OutReason` | `OutOffendingKind` | When |
|---|---|---|---|
| `true` | `None` | `NAME_None` | `Kinds` empty (`who:"none"` / `who:"all"`), or every named kind is legal |
| `false` | `KindUnknown` | the first offending symbol | a named kind has **no live units** on the ordering team — checked for **every** intent |
| `false` | `KindNotOrderable` | the first offending symbol | a named kind is present but **0 orderable** — checked for **`Send` / `Guard` / `Ambush` only** |

- **Both out-params are written on every path, including success.** ⚠️ This is the **opposite** of
  `ResolvePlace`, deliberately: an untouched `FVector` cannot become a plausible-looking origin an army
  marches to, whereas an untouched reason code *can* become last sentence's refusal reason filling this
  sentence's template.
- **A multi-kind order with one illegal kind is refused AS A WHOLE**, immediately, naming the **first**
  offender in `Kinds` order (deterministic — pinned by a test). The good kinds are neither executed nor
  silently dropped.
- **You return a reason code; TASK-443 routes it to the EXISTING `{"ask":"unsupported"}` outcome.**
  ⛔ No player-facing string is produced in this file, and none may be invented downstream (§3).

---

## 2. FILES TOUCHED

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h` | **EDIT** — include; `FSiegeAssistantRosterEntry::Orderable`; the two accessors; `ESiegeAssistantRejectReason`; `ValidateCommandAgainstSnapshot`. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp` | **EDIT** — one line inside `Capture`'s existing eligibility branch; the two accessor bodies; `SiegeAssistantGuardInternal::IntentTakesZoneOrder`; the validator body. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGuardTest.cpp` | **NEW FILE** — 9 automation tests. |

⛔ **Nothing else.** No component code, no controller edits, **no `SiegeAssistantGrammar.cpp`** (tightening the
grammar to encode board state is introducing the defect §1 names), no `Content/`, no `.ini`, **no Git, no
compile, no editor, no MCP, no PIE.**

### ⚠️ ON THE NEW FILE — DECLARED, NOT SLIPPED IN
My `names:` line lists `SiegeAssistantSnapshot.{h,cpp}` only, and the tests had to live somewhere. I put them
in a **new** file under the shipped precedent `Siegebound/Tests/` (the only automation tests on this project,
`SiegeAssistantGrammarTest.cpp`, live there — and TASK-436 independently landed `Tests/SiegeSettingsTest.cpp`
in the same wave, so the directory is this batch's convention). **It is owned by no other task, collides with
no filename, and is file-disjoint from all eight parallel tasks.** The alternative — ~500 lines of test code
appended to a 1,455-line shipped `.cpp` that TASK-447's first-execution audit has to read — is worse. **If QA
rules the other way, moving it is a file move and zero code change.**

---

## 3. ⚠️ THE ONE DESIGN CALL THAT DEVIATES FROM A PINNED STRUCT — READ THIS, DO NOT SKIM IT

**`FSiegeAssistantRosterEntry` gained one field: `int32 Orderable = 0;` (appended after `GroupId`).**

**Why it was unavoidable, stated as arithmetic rather than preference.** Two pins are in play:

- **CONVENTIONS "In-match LLM command assistant" §9** pins the struct as
  `{ FName Kind = NAME_None; int32 Count = 0; int32 GroupId = INDEX_NONE; }`.
- **"Settings screen…" §8** pins the validator to take **`const TArray<FSiegeAssistantRosterEntry>&`** and
  requires it to distinguish **`KindNotOrderable`** (present, none eligible) from **`KindUnknown`** (absent) —
  and the spec's own test list says so in as many words: *"a kind present but `0 orderable` is rejected with
  `KindNotOrderable`"* · *"a kind absent entirely is rejected with `KindUnknown`"*.

**Those two are not simultaneously satisfiable.** The three pinned fields answer *"is it here"* and cannot
answer *"may it be ordered"* — the orderability tally lives only in the private parallel array `KindOrderable`,
which the signature deliberately does not receive. **The alternatives were considered and each breaks something
explicitly forbidden:** changing the parameter to `const USiegeAssistantSnapshot*` is *"rejected on sight"* (§8)
and would delete the no-world testability this task exists to buy; adding a 5th parameter breaks the pinned
signature TASK-443 compiles against; collapsing the two reasons into one breaks the pinned three-value enum.

⇒ **One appended field is the minimum change that makes the pinned signature implementable.** It is strictly
additive: **nothing renamed, nothing re-typed, no existing reader changed.**

**QA should scrutinise this and rule on it.** My argument that it is safe:
- ✅ **THE PROMPT IS BYTE-UNCHANGED.** `Roster` is used by `Capture()` for aggregation and by `GetRoster()` for
  the executor — **no zone prints it.** `AppendRosterBlock` prints from the parallel
  `UnitKinds`/`KindTotals`/`KindOrderable`/`KindFollowable` arrays. ⇒ **`zoneB_chars = 68` and
  `zoneC_chars = 887` cannot move, and the t0 tripwire cannot fire.** (Verified by reading every `Roster`
  reference in the `.cpp` — lines 203, 459/466 (aggregation), 539 (sort), 587 (a Verbose count).)
- ✅ **NO NEW CONSUMER BREAKS.** `FSiegeAssistantRosterEntry` and `GetRoster()` have **zero references anywhere
  else in `Source/` or `Plugins/`** — the only hit is a comment in `SiegeLlamaSpike.cpp` saying its fixture row
  *"mirrors FSiegeAssistantRosterEntry's printed form, not its struct"*, which is the printed form and is
  unaffected.
- ✅ **NO RE-TALLY, NO EXTRA TRAVERSAL.** The field is filled by **one added line inside the branch that
  already calls `Unit->IsGroupCommandEligible()`** — the same already-computed result, recorded on a second
  aggregate. The predicate is not called twice, no pass is added, and **no unit registry / actor cache / dirty
  flag** is introduced (CONVENTIONS §4 — rejected on sight, cited).
- ⚠️ **ONE LATENT HAZARD, COMMENTED IN PLACE:** `Entry` is a pointer into `Roster` obtained ~30 lines earlier.
  It is valid at the increment because nothing between the two touches `Roster` (only `Tallies` grows) —
  **anyone who inserts a `Roster` write between them must re-fetch `Entry`.** Said in the code, not only here.
- ⚠️ **NO `Followable` COMPANION WAS ADDED.** The pinned reason enum has no *"cannot follow"* value and
  TASK-443 has no template for one, so a followable column would be unused state today. If a
  `KindNotFollowable` reason is ever pinned, it arrives **with** its column.

---

## 4. ⚠️ THE VERIFICATION THAT ACTUALLY MATTERED — ORDERABLE vs FOLLOWABLE

The spec warned that *"exposing the wrong one is a silent wrong-answer defect"*, so I read the code that fills
the tally rather than trusting the name (`SiegeAssistantSnapshot.cpp`, the `ASummonedUnit` loop in `Capture`):

- `KindOrderable` ← `Unit->IsGroupCommandEligible()` — **"may I SEND these?"** — the **zone** orders
  (send / guard / ambush). **This is the column I exposed.** ✅
- `KindFollowable` ← `Unit->IsFollowCommandEligible()` — **"may these FOLLOW?"** — untouched.

**AND THAT DISTINCTION FORCED A REAL SCOPING DECISION, WHICH IS THE MOST IMPORTANT THING ON THIS PAGE:**

> ⛔ **A guard that refused every `0 orderable` kind for every verb would have refused `"clerics follow me"` —
> a legal shipped order, and verbatim the eval's own `DEV-20` utterance.**

The Cleric is Support: it **follows** and **cannot take zone orders** (the shipped Cleric ruling), so a Cleric
roster row is `Count > 0, Orderable == 0`. Refusing `Follow` on the orderable column would have shipped a
**regression wearing a safety fix's clothes** — the exact failure mode this batch's ruling 3 names.

⇒ **The `KindNotOrderable` check is scoped to `Send` / `Guard` / `Ambush`** via a file-local
`IntentTakesZoneOrder`, which is **narrower than the shipped `SiegeAssistantIntentTakesSelection()`** (that one
also returns true for `Follow`). Spelled out in the code with its reasoning so nobody "simplifies" it back.

**`KindUnknown` is NOT scoped** — it applies to every intent, because a hallucinated unit is a hallucinated
unit whatever verb carries it, and no eligibility column could rescue a kind with zero live units.

**⚠️ THE DECLARED v1 GAP, so it is a decision and not a later discovery:** a `Follow` order naming a
**non-followable** kind (Ogre / Sapper) is **not** refused by this guard. It is **not an open hole** — the
executor's selector filters on `IsFollowCommandEligible()` and the order lands on the **shortfall /
clarification** path instead. There is an assertion pinning this in the test file, written so that **it is the
one that must flip** if a `KindNotFollowable` reason is ever pinned.

---

## 5. WHAT THE GUARD DELIBERATELY DOES **NOT** CHECK

| Not checked | Why |
|---|---|
| **Counts** | *"You asked for 10 and 8 exist"* is the shortfall/clarification path and belongs to the FSM. Refusing or clamping here would make that clarification **undetectable** — the defect §1 names when it says *constrain identity hard, leave quantity soft*. |
| **`Where`** | Place existence is the grammar's (the `where` alternation is generated from live resolvable places); resolution is `ResolvePlace`'s game-side airlock. |
| **`TriggerKind`** | ⚠️ **Correct, not an oversight.** A deferred intent means *"fire once at least N of these EXIST"*, so a trigger kind absent **right now** is the whole point of waiting. Validating it as a `who[]` entry would refuse every deferred order that was doing its job. Pinned by a test. |
| **`Kinds.Num() == Counts.Num()`** | Already owned by `SiegeAssistantValidateSelection`. Not duplicated — ruling 3 requires that check to keep running, not to run twice. |

⚠️ **Ruling 3 compliance, stated plainly: this guard is a MACHINE CHECK, so the confirm toggle must never skip
it.** It runs identically with the toggle ON and OFF. A future task that short-circuits it on the toggle-off
path has converted a UX preference into a safety regression.

---

## 6. THE TEST LIST — `Siegebound/Tests/SiegeAssistantGuardTest.cpp`, 9 tests

All under `#if WITH_DEV_AUTOMATION_TESTS`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST`,
flags `EditorContext | EngineFilter` (matching the shipped `SiegeAssistantGrammarTest.cpp`).

The fixture roster mirrors this project's shipped eligibility rules: `footman` **×2 rows** (grouped +
ungrouped — the aggregation is by `(Kind, GroupId)`), `archer`, `cleric` (`Count 2, Orderable 0`),
`sapper` (`Count 1, Orderable 0` — **the DEV-04 row**), and **no `catapult` at all**.

| # | Test | Asserts |
|---|---|---|
| 1 | `Siegebound.Assistant.Guard.OrderableKindPasses` | `Orderable > 0` passes under **all three** zone verbs; reason `None`, offender `NAME_None`; a multi-kind all-legal selection passes. |
| 2 | `…Guard.NonOrderableKindRefused` | **THE DEV-04 REPRODUCTION.** `sapper` present with 0 orderable ⇒ refused under all three zone verbs, `KindNotOrderable`, **names `sapper`**; and is **not** reported as `KindUnknown`. |
| 3 | `…Guard.UnknownKindRefused` | `catapult` (absent) ⇒ `KindUnknown`, names it; absent under `Follow` too; a zero-`Count` row does **not** count as present. |
| 4 | `…Guard.MultiKindRefusedAsAWhole` | One bad kind refuses the **whole** command in **both** orderings — nothing silently dropped; with two offenders the **first in `Kinds` order** is reported, with **its own** reason (both directions). |
| 5 | `…Guard.EmptyRoster` | Any named kind ⇒ `KindUnknown`; an army-wide verb against an empty roster is **not** refused. |
| 6 | `…Guard.ArmyWideVerbsNotRefused` | `Charge` / `Fallback` / `Rally` with an empty selection pass; `Send` with `who:"all"` (also an empty selection) passes. |
| 7 | `…Guard.FollowIsNotGatedByTheOrderableColumn` | **THE REGRESSION TEST.** `"clerics follow me"` passes; the **same cleric under a zone verb is refused** (so the verb selects the column, not the kind); the declared v1 Follow gap is pinned. |
| 8 | `…Guard.ContractDetails` | Out-params **reset on the success path** (pre-dirtied); orderable units in a **later** roster row still allow the order (summed across groups) and all-zero rows still refuse; an **over-count** request is not refused; an **absent `TriggerKind`** is not refused. |
| 9 | `Siegebound.Assistant.Snapshot.OrderabilityAccessors` | An **un-captured** snapshot answers `0` / `false` for a real kind, an invented kind and `NAME_None`, has an empty roster, and does not crash; `IsKindOrderable == (GetOrderableCount() > 0)`. |

**Two test-design decisions worth QA's eye:**
- **The guard logs at `Log`, never `Warning`.** A refusal is the **model** being wrong — expected traffic, not
  a code defect — and the automation framework treats a logged **warning** as a test failure, which would have
  made this function untestable by its own tests. `Log` is on by default, is rate-limited by human typing
  speed, and names the offending symbol, which is the line TASK-447's audit and TASK-448's playtest want.
- **`Capture(nullptr, …)` is deliberately NOT exercised** — it logs at `Warning` by design. Test 9 covers the
  same null-safety surface (an un-captured snapshot) without provoking it.

---

## 7. ⚠️ WHAT I COULD NOT VERIFY WITHOUT A COMPILE — STATED PLAINLY

**Nothing in this task has been compiled, and nothing has been run.** One compile gate only (TASK-447),
quiet-module law, seven tasks in one UBT module. So:

- ⛔ **"The tests pass" is a claim NOBODY may make yet. The only true statement is "the tests are written."**
  This is §9c's promoted law — *where an artifact has a parser, the parser is the reviewer of record* — and
  here the parser is UBT plus the automation runner, neither of which has seen a byte of this.
- **Unverified by me, in order of how likely they are to bite:**
  1. **UHT's reaction to `SiegeAssistantSnapshot.h` including `SiegeAssistantCommand.h`.** Reasoned safe (no
     cycle; `.generated.h` stays last) but not proven by a run.
  2. **The `TestEqual` / `TestNotNull` overload set on this engine version.** I avoided `TestEqual` on `bool`
     and `TestNotEqual` on `FString` for exactly this reason and rewrote both as `TestTrue`/`TestFalse`; the
     remaining uses are `int32` and `FString`, which are the safest overloads. `TestNotNull`'s return value is
     not consumed, so it compiles whether it returns `void` or `bool`.
  3. **`ESiegeAssistantRejectReason` as a plain `enum class` in a `.generated.h`-bearing header.** Pinned that
     way, at file scope, outside every reflected type — reasoned safe, unproven.
  4. **That `Orderable` is actually filled on a live board.** The tests use a hand-populated roster, so they
     assert the guard's behaviour **given** a roster; they do **not** assert `Capture()` fills the column
     correctly on a real match. **That needs a world ⇒ TASK-447's first-execution audit and TASK-448.**
- **Also unverified, by design:** every on-screen/played claim. Nothing here renders and nothing here is a
  player-facing string.

### 📌 For TASK-447's first-execution audit
When `USiegeAssistantSnapshot` executes for the first time ever, the cheap extra observation is
**`GetRoster()`'s rows with their `Orderable` values beside the printed roster line** — if
`sum(Roster[kind].Orderable)` does not equal the `orderable=` figure the roster line prints for that kind, the
column is wrong and the guard is answering off a bad number. **Both are derived from the same per-unit call, so
they must agree exactly.** That is a one-line check and it is the only live proof this task cannot self-serve.

### 📌 Recorded, not acted on
`SiegeAssistantSnapshot.cpp`'s `BuildZoneA` comment and the header's `BuildZoneA` comment still carry the
**TASK-433 comment-only correction debt** noted in CONVENTIONS §8 (the dead `llama_kv_cache_seq_rm` spelling is
already corrected in this file; the Zone-A char figure hint is TASK-431/434's lane). **I did not touch either —
they are outside this task's scope and inside a file another audit is about to read.**

---

## 8. STATUS
`ready-for-qa` → **TASK-446** (the QA gate covering TASK-440 · 441 · 443 · 444).
**TASK-443 is unblocked on my half:** the validator signature above is final and published.
