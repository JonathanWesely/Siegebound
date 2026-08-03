# TASK-463 — gameplay-programmer handoff (W1-VOCAB)

**Date:** 2026-08-03 · **Agent:** gameplay-programmer
**STATUS: ✅ DIAGNOSED — ANSWER IS (a) · ✅ FIXED IN THE GAME LANE · ✅ ITEM (3) DONE
· ⛔ `ZoneA.TwoLaneByteEquality` NOT RUN — NO TOOL EXISTS IN THIS SESSION · ⛔ NOT COMPILED**

> **M8 DECLARATION DUTY, VERBATIM AS REQUIRED:**
> **"adds no replicated property, no new replicated class, no new relevancy tier."**

---

## 1. ⭐ THE DIAGNOSIS, WITH ITS EVIDENCE — **IT IS (a), AND IT IS NOT CLOSE**

> **(a) The asset does not resolve AND the C++ fallback is unwired.** ⇒ **MINE. Fixed below.**
> ⛔ **NOT (b)** — the C++ defaults are richly populated.
> ⛔ **NOT (c)** — there is no asset to be empty or misauthored. **art-director is not blocked and not implicated.**

### 1a. THE ASSET IS ABSENT FROM DISK — with a §14 positive control

```
$ find Content -iname "*AssistantVocabulary*"      ->  (no output)
$ find Content -iname "DA_Assistant*"              ->  (no output)
$ find Content/Data -type f                        ->  Content/Data/DA_BattlefieldScatter.uasset
                                                       Content/Data/DT_Cards.uasset
```
✅ **POSITIVE CONTROL (§14): the same search shape FINDS a `DA_` asset that does exist**
(`DA_BattlefieldScatter.uasset`, plus `Content/Blueprints/BP_BattlefieldScatter.uasset`). ⇒ **The
negative is about the repository, not about my search.** `/Game/Data/DA_AssistantVocabulary` has never
been authored — it is TASK-421's undelivered output, exactly as `LoadSynchronous` reported.

### 1b. THE C++ DEFAULTS ARE **NOT** EMPTY — this is what kills hypothesis (b)

`USiegeAssistantVocabulary::USiegeAssistantVocabulary()` (`SiegeAssistantVocabulary.cpp:84-204`)
populates **13 unit rows, 7 place rows, 7 intent rows**, plus a 4-line `[notes]` block authored in
`BuildSynonymTable`. The class comment has said all along that it *"ships with sane C++ defaults so the
feature works before any asset exists."* ⇒ ⚖️ **TASK-417 DELIVERED THE DEFAULTS. THE DEFECT IS THAT NO
CALL SITE EVER USED THEM.**

### 1c. ⭐ THE UNWIRED CALL SITE — the actual defect, one line

`USiegeAssistantComponent::GetVocabulary()` returned the raw soft-pointer load and nothing else:

```cpp
bVocabularyResolved = true;
ResolvedVocabulary = VocabularyAsset.LoadSynchronous();   // asset absent => nullptr, forever
...
return ResolvedVocabulary;                                 // => BuildZoneA(nullptr)
```

`BuildZoneA(nullptr)` then takes its documented, correct `else` branch and emits `synonyms:\nnone\n`.
⚖️ **Every individual component behaved exactly as specified. The defect lives in the seam between
them** — which is why no reader caught it and only execution did.

### 1d. ⭐⭐ THE ARITHMETIC CLOSES EXACTLY — 3029 IS PROVABLY THE NULL LANE

I re-implemented `BuildSynonymTable()` offline from the constructor rows (sort, lower-case, trim,
de-dup, drop-empty-rows, join) with no engine resident, and got:

| quantity | value |
|---|---|
| `BuildSynonymTable()` length | **2092 chars** (ASCII-clean ⇒ 2092 UTF-8 bytes) |
| `MeasuredZoneAChars` (pinned in the test) | **5116** |
| predicted null lane = `5116 − 2092 + Len("none\n")` | **3029** |
| ⭐ **`zoneA_chars` PIE actually printed** | ⭐ **3029** |

✅ **EXACT MATCH, TO THE CHARACTER.** And it is not my formula — it is the one already asserted by
`Siegebound.Assistant.ZoneA.NullVocabularyIsNotTheMeasuredLane`
(`SiegeAssistantZoneATest.cpp:603`), which computes `ExpectedNullLength = MeasuredZoneAChars −
SynonymTable.Len() + 5` and pins `NullLane.Len()` to it.

⇒ ⛔ **THIS IS NO LONGER AN INFERENCE. The shipped lane provably ran `BuildZoneA(nullptr)`, and the
2092 characters missing from the player's prompt are precisely the synonym table.** A project that had
never written that test would still be arguing about the cause.

### 1e. ⚠️ THE TEST FILE HAD ALREADY NAMED THIS GAP IN WRITING

`SiegeAssistantZoneATest.cpp:310-317`, on the two-lane test, states verbatim that passing the asset
*"would compare whatever an artist last saved"* and — **"⚠️ THAT SECOND CASE IS A REAL, OPEN GAP AND IT
IS NAMED IN THE HANDOFF."** ⇒ **The hazard was correctly recorded by TASK-423 and simply never routed
to a fix task.** ✅ **The labelling worked; the follow-through is what was missing.**

---

## 2. WHAT I CHANGED — **TWO FUNCTIONS, ONE FILE PAIR. NOTHING ELSE.**

⚠️ **TASK-465 collision note, stated precisely as asked:** I touched **`BeginPlay`** and
**`GetVocabulary`** in `SiegeAssistantComponent.cpp`, and **five doc-comment blocks** in
`SiegeAssistantComponent.h`. ⛔ **I did NOT touch the reason-code template table, `PushMessage`,
`ShortfallCount`, the FSM, or the executor** — TASK-465's lane is clear of mine.

### 2a. `GetVocabulary()` — the fix (`SiegeAssistantComponent.cpp`, symbol, ~:2732-2819)

- Asset resolves ⇒ unchanged behaviour, but now returns early and logs that **the ASSET lane** is live,
  with the §12a caveat that an asset **overrides the defaults wholesale** and is therefore **not**
  necessarily the measured lane.
- Asset absent ⇒ **`ResolvedVocabulary = NewObject<USiegeAssistantVocabulary>(this)`.**
- Allocation failed (only remaining path to an empty table) ⇒ still **`Warning`**, still deterministic.

⚖️ **WHY `NewObject`, NOT `GetMutableDefault` — an evidentiary reason, not a stylistic one.**
`ZoneA.TwoLaneByteEquality` builds its comparison object with `NewObject<USiegeAssistantVocabulary>()`.
Using the **same construction** in the shipped path makes that test's PASS a statement about **the
object the shipped lane actually renders**, not merely about one of the same class. It also keeps a
process-global CDO pointer out of a **non-const** `TObjectPtr` member, where a later edit through
`ResolvedVocabulary` would corrupt the defaults for every component in the process. Outered to `this`
and held by the existing `UPROPERTY(Transient)` ⇒ GC-rooted twice over, dies with the component.

### 2b. 📌 ONE DECLARED DEPARTURE (§15) — **A SEVERITY CHANGE, FLAGGED RATHER THAN BURIED**

**The missing-asset log drops from `Warning` to `Log`.** ⚖️ **Mechanism, checkable without trusting me:**
before the fallback existed, a missing asset silently gutted Zone A and `Warning` was correct. **After
the fallback, a missing asset yields the C++ default table — which IS the lane every eval number was
measured on.** A `Warning` would then flag **the correct state** as a fault, which is the
*"evidence-shaped false warning"* **§22** ranks as the worse half of a stale claim. ⛔ **The line still
names the lane**, because §12a binds eval claims to it. **QA should rule on this explicitly; I will take
either verdict.**

### 2c. §22 SWEEP — I SEARCHED THE **CLAIM**, NOT THE COMMENT

Swept `holds no reference|no plugin reference|prints \`none\`|stays null` across `Source/` and the
plugin. **Exactly three artifacts asserted the plugin-reference claim — matching QA-464 WARN-3's count:**

| site | disposition |
|---|---|
| `SiegeAssistantComponent.cpp` BeginPlay **comment** | ✅ corrected — I am editing that function |
| `SiegeAssistantComponent.cpp` BeginPlay **`UE_LOG`** (prints every match) | ✅ corrected — §22's priority surface |
| `SiegeAssistantComponent.h:141` class comment | ✅ corrected |

⚠️ **THE SHAPE §22 CALLS HARDEST TO CATCH, AND THIS IS IT: THE CONCLUSION WAS STILL TRUE AND ONLY THE
MECHANISM WAS DEAD.** *"A missing GGUF never blocks match start"* remains correct — but *"holds no
reference into the plugin at all"* was **already false before I arrived** (`NotifyConsoleOpened`,
`DispatchTurnToModel`, `AbortInFlightRequest` all resolve the subsystem) and my item (3) adds a fourth
caller. ⇒ I replaced the dead mechanism with the **real, checkable** one: the pointer is **resolved live
and never cached**, every call is **null-tolerant**, and the plugin's entry points are **non-blocking**
(`RequestCompletion` *"returns false IMMEDIATELY when !IsReady()"*, `SiegeLlamaSubsystem.h:263`).

⛔ **I did not touch `SiegeAssistantSnapshot.h:441`** (*"null ⇒ the synonym block prints `none`"*) —
**that is still TRUE**: `BuildZoneA`'s contract is unchanged and is pinned by the null-vocabulary test.
⚖️ **Fixing it would have been the plausible-looking wrong move (§20).**

---

## 3. ITEM (3) — THE UNARMED WINDOW. **ARMED AT `BeginPlay`. WHEN, NEVER WHAT.**

`EnsureStaticPrefixRegistered(ResolveLlamaSubsystem())` is now the **last statement of `BeginPlay`**.
⛔ **The function itself is byte-unmodified** and the string it registers is the same `GetCachedZoneA()`
every other caller passes. **Nothing about what the assertion checks moved.**

### ⚖️ WHAT IT ACTUALLY BUYS — stated honestly, because a bare call would be oversold

- ✅ **MATCH 2+ IN A SESSION, and any level travel or restart: the window closes COMPLETELY.**
  `USiegeLlamaSubsystem` is a **GameInstance** subsystem and **survives travel**, so the model is
  already loaded when `BeginPlay` runs, `IsReady()` is true, and the prefix arms **before anything can
  open a console.**
- ⚠️ **COLD FIRST MATCH: this is a DELIBERATE NO-OP.** The model is still loading (measured **1874 ms**,
  async on a below-normal worker), so the `IsReady()` gate returns silently **without latching** —
  correct, because `ApplyPendingStaticPrefix` **discards** a prefix that arrives before the weights
  land. The existing console-open and dispatch retries still do the work.
- ⛔ **THE RESIDUAL COLD-START WINDOW IS UNREACHABLE, NOT MERELY SMALL — AND THAT IS WHY I ADDED NO
  TIMER, POLL OR TICK.** While `!IsReady()` the plugin **refuses to hold a prefix at all**, *and*
  `RequestCompletion` **returns false immediately** ⇒ **there is no interleaving in which a request
  reaches `llama_decode` unguarded during it.** Adding a tick to chase it would break this component's
  **"never ticks"** law (§4) to buy nothing.

### ⚠️ ONE SIDE EFFECT I AM DECLARING RATHER THAN LETTING QA FIND

Because `EnsureStaticPrefixRegistered` calls `GetCachedZoneA()`, **on the already-ready path only**
(match 2+), the **non-Shipping Zone-A byte-identity re-check now fires at `BeginPlay`** instead of on
the second turn. ✅ **This is a small win, not a cost:** that §8 QA criterion had **never executed**
(the first live run never opened a console), and it now runs at match start for the price of one extra
~5 KB string build, outside Shipping only. ⛔ **On a cold start it does not fire at all** — the
`IsReady()` gate returns *before* `GetCachedZoneA()` is reached, so `BeginPlay` costs exactly **one map
read and one bool read.**

---

## 4. ⛔ ITEM (4) — `ZoneA.TwoLaneByteEquality` **WAS NOT RUN. STATED PLAINLY, AS REQUIRED.**

**I did not run it, and no substitute is offered.**

⚖️ **AND THE REASON MATTERS, BECAUSE THE OBVIOUS OBJECTION IS WRONG:** this test does **not** depend on
my diff. It compares `BuildZoneA(NewObject<Vocabulary>())` against the frozen spike fixture — **neither
side goes through `GetVocabulary()` or the component.** ⇒ **A run on the CURRENT binaries would have
been valid for the post-change code**, which is why I tried.

**What stopped it — tooling, not schedule:**
- ⛔ **The `mcp__unreal-mcp__*` toolset is not present in this session.** ✅ **§14 positive control:** a
  `select:` query naming `mcp__unreal-mcp__call_tool`, `mcp__unreal-mcp__list_toolsets` **and**
  `mcp__claude_ai_Slack__slack_send_message` returned **only the Slack tool** — so the query shape works
  and the absence is real.
- ⛔ Independently, **TASK-447 enumerated all 19 toolsets on that surface and found NO console-exec
  tool.** `Automation RunTests` is a console command. **The gap is the same one that blocked every smoke
  row.**
- ⛔ A headless `UnrealEditor-Cmd -ExecCmds="Automation RunTests …"` would spawn a **second editor
  process** against a project **PID 9828 already holds**, while **build-master is mid-commit**. That is
  the disturbance I was told to avoid, and it is a build-lane action besides.

### ⇒ WHAT THE NEXT RUNNER SHOULD EXPECT (⚠️ **PREDICTIONS, LABELLED AS SUCH — I COMPILED NOTHING**)

| check | prediction | basis |
|---|---|---|
| `ZoneA.TwoLaneByteEquality` | **PASS, unaffected by this diff** | neither lane routes through my change |
| `ZoneA.MeasuredCharCount` | **PASS at 5116** | same |
| BeginPlay `zoneA_chars=` | ⭐ **3029 → 5116** | `3029 − Len("none\n") + 2092` |
| BeginPlay `vocabulary=` | `none` → `SiegeAssistantVocabulary_0` | `NewObject` default naming |
| `LLM_BUDGET ASSERTION IS INCOMPLETE` at match start | **still fires on a COLD first match**; gone on match 2+ | the `IsReady()` gate, §3 above |

⚖️ **CONSEQUENCE FOR §12a, AND I AM NOT CLAIMING IT IS DISCHARGED.** Its clause (a) reads *"`DA_AssistantVocabulary`
RESOLVES."* **My fix does not make the asset resolve** — it makes the shipped lane carry the **same
table** by the code path TASK-417 always intended. ⇒ 📌 **A MANAGER RULING IS OWED: is (a) satisfied in
SUBSTANCE (the shipped prompt is now the measured prompt) or must it be satisfied LITERALLY (TASK-421
authors the asset)?** ⛔ **I am not ruling on it and I am reporting no eval number.**

---

## 5. ⚠️ A NEW FINDING THIS WORK SURFACED — **AUTHORING THE ASSET WILL BREAK BYTE-EQUALITY**

⛔ **`UnitSynonyms` / `PlaceSynonyms` / `IntentSynonyms` are serialised `UPROPERTY`s, so a saved
DataAsset REPLACES every row rather than adding to them.** ⇒ **The moment TASK-421 authors
`DA_AssistantVocabulary`, the shipped Zone A silently becomes whatever the artist saved — and it will
NOT be byte-equal to the spike lane unless it reproduces all 27 rows character-for-character.**

⚖️ **So this task's fix has an unexpected property worth stating out loud: RIGHT NOW, "no asset" is the
CORRECT and MEASURED configuration.** Authoring the asset is **a Zone-A change and a §12a event**, not
content work. ⇒ 📌 **TASK-421 should be re-specced or held.** ⛔ **Manager's call — I am flagging, not
ruling, and I have not touched TASK-421 or anything under `Content/`.**

---

## 6. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp` | `GetVocabulary()` — the fallback · `BeginPlay()` — prefix arming + §22 correction |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` | 5 doc blocks: class comment §2 clause · `GetVocabulary` · `EnsureStaticPrefixRegistered` ("two places" → three) · `VocabularyAsset` / `ResolvedVocabulary` / `bWarnedMissingVocabulary` |

⛔ **NOT touched:** `SiegeAssistantVocabulary.{h,cpp}` (the defaults were already correct — **the fix
was never here**) · `SiegeAssistantSnapshot.{h,cpp}` (`BuildZoneA`'s null contract is pinned by a test)
· 🔒 **`SiegeLlamaSpike.cpp` (§16)** · the plugin · `Content/` · the test files · Git.

---

## 7. ⛔ WHAT I COULD NOT VERIFY — stated, not omitted

1. ⛔ **NOTHING WAS COMPILED.** Every claim here is about **source**. New symbols used:
   `NewObject<USiegeAssistantVocabulary>` (type complete — already `#include`d at `:18`) and
   `ResolveLlamaSubsystem()` (existing member, already called from three sites in this TU).
2. ⛔ **NOTHING WAS EXECUTED.** `zoneA_chars=5116` is **arithmetic on published operands**, not a
   measurement. ⚠️ **Do not re-tell it as one.**
3. ⚠️ **My offline `BuildSynonymTable` re-implementation is a TRANSCRIPTION and shares my authorship
   with nothing else** — but it is the §9c same-authorship risk in miniature. ✅ **It is corroborated by
   landing on 3029 independently**, which is a figure I did not choose. **The compile-time truth is what
   the test prints.**
4. ⛔ **`ZoneA.TwoLaneByteEquality` STILL HAS NEVER RUN.** §12a's gate on eval reporting **stays shut.**
5. ⚠️ **I verified brace/paren balance and non-ASCII `TEXT()` literals mechanically** (both functions
   close at depth 0; whole file balanced; the 9 non-ASCII `TEXT()` literals are all **pre-existing** and
   far from my edits). ⛔ **That is a syntax smoke check, NOT a compiler.**
