# TASK-545 — the parser, the seventh field, and the third cross-field check

- **assignee:** gameplay-programmer
- **status:** ready-for-qa
- **QA gate:** ⛔ **TASK-550** (`.claude/pipeline/qa/TASK-550.md`) — the batch's only gate, covering 541..549
- **law:** CONVENTIONS `AS-§21.5` · `AS-§21.6` · `AS-§21.9` (the PIN) · `AS-§20.1` · `AS-§2` · `AS-§3`
- **compiled:** ⛔ **NO.** TASK-551 owns the only compile. **Tests:** ⛔ none written — TASK-549 owns them.

## Files touched — exactly two, both exclusively mine

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantCommand.h`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantCommand.cpp`

⛔ Nothing else was opened for writing. The grammar (546), the snapshot (547), the executor (548), the
vocabulary (541) and `Tests/` (549) are untouched by this task.

## 1. The pinned symbols — as written, next to the `AS-§21.9` registry version

| `AS-§21.9` registry | as written in the file | verdict |
|---|---|---|
| `FName RegionPlace = NAME_None;` | `SiegeAssistantCommand.h:266` — `	FName RegionPlace = NAME_None;` under `UPROPERTY()` | ✅ character-for-character |
| `inline constexpr const TCHAR* In = TEXT("in");` | `SiegeAssistantCommand.h:344` — `	inline constexpr const TCHAR* In = TEXT("in");` | ✅ character-for-character |
| `inline constexpr const TCHAR* RegionConflict = TEXT("region_conflict");` | `SiegeAssistantCommand.h:475` | ✅ character-for-character |
| `inline constexpr const TCHAR* BadRegion      = TEXT("bad_region");` | `SiegeAssistantCommand.h:490` — `	inline constexpr const TCHAR* BadRegion = TEXT("bad_region");` | ✅ identical tokens (the registry's alignment padding is whitespace, not a name) |
| `bool SiegeAssistantValidateSelection(const TArray<FName>& Kinds, const TArray<int32>& Counts, FString& OutError,`<br>`                                     const TArray<FName>& ExcludeKinds = TArray<FName>(), FName RegionPlace = NAME_None);` | `SiegeAssistantCommand.h:616-617`, byte-for-byte including the continuation indent | ✅ character-for-character |

**Field ORDER:** `RegionPlace` is the **SEVENTH** member, declared **after** `ExcludeKinds` (`.h:221` then `.h:266`).
The six shipped fields keep their shipped order, their shipped types and their shipped comments.
⛔ Nothing was inserted among them.

**Definition side:** `SiegeAssistantValidateSelection`'s definition (`.cpp:449-450`) takes the same two
trailing parameters with the defaults **only** on the declaration, as C++ requires.

## 2. Every refusal path, and exactly what triggers it

### Parser (`ParseSiegeAssistantCommand`)

| trigger (the `who` value, unless noted) | code emitted | where |
|---|---|---|
| `{"in":"mid","all_except":["miner"]}` — both object keys present | `region_conflict:all_except/in` | `.cpp:801` — checked **before** the dispatch, so the log says "two filters stacked" instead of the useless `unknown_key` |
| `{"in":<non-string>}` (number, array, object, null) | `bad_type:in` | `.cpp` `ParseRegionSymbol` |
| `{"in":""}` (or whitespace-only) | `bad_region:` (empty payload) | `ParseRegionSymbol` |
| `{"in":"none"}` | `bad_region:none` | `ParseRegionSymbol` |
| `{"in":"all"}` | `bad_region:all` | `ParseRegionSymbol` |
| `{"in":"now"}` | `bad_region:now` | `ParseRegionSymbol` — ⚠️ **see §5, the one spec discrepancy** |
| `{"in":"mid","foo":1}` — any extra key beside `in` | `unknown_key:foo` | `ValidateExactKeySet` on the region object |
| a region on `charge` / `fallback` / `rally` | **`region_conflict:<intent>`** e.g. `region_conflict:fallback` | `.cpp:1066-1071` — **CROSS-FIELD CHECK 3 OF 3**, gated by the existing `SiegeAssistantIntentTakesSelection` |
| a region with `who` = `"none"` | `region_conflict:none` | `.cpp:1080` — **defensive**, unreachable from JSON (disjoint shapes), labelled as such in the code |

### Validator (`SiegeAssistantValidateSelection`) — the receive-side / M8-P2 half

| trigger | code emitted |
|---|---|
| `RegionPlace` = `all` or `now` | `bad_region:<symbol>` |
| `RegionPlace` set **and** `Kinds` non-empty | `region_conflict:<kindcount>/<region>` e.g. `region_conflict:2/mid` |
| `RegionPlace` set **and** `ExcludeKinds` non-empty | `region_conflict:all_except/<region>` |

⛔ **`RegionPlace` = `"none"` needs no test and cannot have one:** `FName(TEXT("none"))` **is** `NAME_None`
(FName is case-insensitive), so a peer sending `"none"` is indistinguishable from a peer sending no region.
"No region" is the safe reading — it filters nothing rather than filtering wrongly. This is exactly why the
**parser** catches `"none"` in the STRING, before the FName exists: without that, `{"who":{"in":"none"}}`
would silently become "everyone", i.e. a filter the player typed, dropped in silence.

⛔ **The intent invariant is NOT checked in the validator, and that is not an oversight:** the pinned
signature is never handed the `Intent`, so "is this verb allowed to carry a region?" is unanswerable there.
It is cross-field check 3, in the parser, which does hold the intent. The code says so in place.

### NOT a refusal, by ruling

⛔ **EMPTY-AFTER-REGION IS NOT A PARSE ERROR.** `ParseSiegeAssistantCommand` is PURE — no world, no roster,
no snapshot, never handed a unit position — so *"was anybody standing there?"* is structurally unanswerable
here. It is the **executor's** (TASK-548), on `AS-§20.1`'s EMPTY-AFTER-EXCLUSION precedent applied verbatim.
Stated in the code twice: on the field (`.h:254-263`) and at the region branch (`.cpp:825-830`).
Likewise, whether the symbol names a **live** region is the executor's question, asked through
`ResolvePlaceRegion` — on the `where` precedent four blocks down, which has never validated its place either.

## 3. ⭐ Pre-existing JSON parses BYTE-IDENTICALLY — and how I convinced myself, not "I only added code"

Four independent arguments, each checkable by reading the diff:

1. **THE TOP-LEVEL KEY SET IS PHYSICALLY UNTOUCHED.** `CommandKeys` is still exactly
   `intent` / `who` / `where` / `when` (`.cpp:668-671`, unmodified lines). `In` is a NESTED key, in the same
   class as `all_except` / `kind` / `n` / `at_least`, and it is never added to `CommandKeys`. This is the one
   way additivity could have died silently, and it is the first thing to grep.
2. **NO NEW REQUIRED FIELD.** `RegionPlace` defaults to `NAME_None`; the parser writes it **only** inside the
   new `bHasRegionKey` arm; every new check in both functions is guarded by `if (!RegionPlace.IsNone())` or
   by `bHasRegionKey`. For any input that lacks `{"in":…}` **not one new line executes**.
3. **THE OBJECT BRANCH'S DEFAULT ARM IS THE SHIPPED CODE, UNMOVED,** and the dispatch is
   `if (bHasRegionKey) { new } else { shipped }` — deliberately a fall-through and not a second `if`. Walked
   case by case:
   - `{"all_except":["miner"]}` → `bHasRegionKey` false → shipped arm → identical bytes.
   - `{}` → both false → shipped arm → `missing_key:all_except`, the byte it reported before.
   - `{"foo":1}` → shipped arm → `unknown_key:foo`, unchanged.
   - `{"all_except":[]}` → shipped arm → `exclude_arity:0`, unchanged.
   - `{"in":…}` → **rejected today** (`unknown_key:in`) ⇒ it is not in the "parses today" set, so accepting it
     now cannot break additivity.
   - `{"in":…,"all_except":…}` → **rejected today** (`unknown_key:*`) ⇒ same; only the code changes.
4. **`BadWho` / `WhoArity` / `WhoRequired` ARE UNTOUCHED.** The String arm and the Array arm are not edited at
   all (`.cpp:707-774` are shipped lines); the new branch is another arm on the OBJECT type only. Check 1
   (`WhoRequired`) is byte-identical apart from its `1 OF 2` → `1 OF 3` banner; check 2 (`ExcludeConflict`) is
   byte-identical apart from its banner and an appended comment paragraph.

⚠️ **The one behaviour change to a currently-FAILING input, declared rather than left to be found:**
`{"who":{"in":"mid","all_except":["miner"]}}` moves from `unknown_key:in` to `region_conflict:all_except/in`.
It failed before and fails now; only the reason code changed, and the new one is the true one. No input that
**succeeds** today changes in any way.

## 4. The validator's 5th-parameter cost — named exactly as the 4th was

`SiegeAssistantValidateSelection` **keeps its name** (`AS-§20.1` ruled it; the proposed
`SiegeAssistantValidateCommand` is refused in `AS-§21.9` — 26 references across 5 files for zero behaviour,
destroying the "the diff is additive" property this batch's safety argument rests on) and gains a **fifth
trailing defaulted parameter**. The cost is written into the header (`.h:586-604`) in the fourth parameter's
own words:

> ⇒ A CALLER THAT OMITS THIS ARGUMENT SILENTLY VALIDATES NOTHING ABOUT THE REGION, AND **NO COMPILER
> DIAGNOSTIC STANDS BEHIND IT** — the omission compiles, links, runs and reports SUCCESS. Only this comment
> and the cross-field gate in `ParseSiegeAssistantCommand` stop it.

⛔ **A SIXTH IS FORBIDDEN**, recorded in the same block: the next field-shaped addition takes a
`const FSiegeAssistantCommand&` **overload**, which cannot be under-called at all. Not taken in this batch
(churn at a compile gate).

### ⚠️ THE OPEN CALL SITE QA MUST SCRUTINISE — it is NOT mine to fix

`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp:1039` (the receive-side gate) reads:

```cpp
if (!SiegeAssistantValidateSelection(Command.Kinds, Command.Counts, SelectionError, Command.ExcludeKinds))
```

It holds a **whole** `FSiegeAssistantCommand` and now omits `Command.RegionPlace` ⇒ it validates **nothing**
about the region. It **still compiles** (the parameter is defaulted), which is precisely the hazard the header
warns about. ⛔ **That file is not mine** (TASK-542 writes it now; TASK-548 owns it next) and
**TASK-548 spec item (5) already assigns the fix, naming the receive-side gate explicitly.** Recorded here so
the gap is on the record between now and then rather than discovered at the gate. **TASK-549's suite should
assert the 5-arg form at that call site.**
The two other callers — `Tests/SiegeAssistantGrammarTest.cpp` and `Tests/SiegeAssistantSelectionTest.cpp` —
pass arrays only, never a whole command, so the default is being used as intended and they still compile.

## 5. ⚠️ ONE SPEC DISCREPANCY, RESOLVED AS A SUPERSET AND DECLARED — not quietly chosen

`AS-§21.5` (and TASK-545 spec item 5) enumerates the `BadRegion` triggers as **`""` / `"all"` / `"none"`**
while **justifying** them as *"the three reserved wire symbols (`SiegeAssistantSymbols`) can never name a
place."* Those two statements do not describe the same set: the three reserved symbols are
`none` / `all` / **`now`**, and `""` is not one of them.

**Resolution: I refuse `""` PLUS ALL THREE reserved symbols.** Reasoning, also written into the code
(`ParseRegionSymbol`'s doc comment) so no future reader thinks it was an accident:
- It is a strict **superset** of the enumeration, so **every case the law enumerates behaves exactly as
  pinned** and no pinned assertion can fail.
- It is what the stated **principle** demands, and `USiegeAssistantGrammar::Build` already refuses to emit any
  of the three as a generated place alternative — so `now` can never name a real region either.
- A test written from the enumeration passes; a test written from the justification ("the three reserved
  symbols") also passes. Only a test asserting that `{"in":"now"}` **succeeds** would fail, and that test would
  be asserting that a reserved symbol names a place.

⛔ **If QA rules the enumeration is exhaustive, the fix is deleting one `||` clause in each of two places**
(`ParseRegionSymbol`, and the validator's reserved-symbol test) — I will take that finding without argument.

## 6. What else QA should scrutinise

1. **The additivity argument in §3** — item 1 (the top-level key set) is the one that can kill the batch.
   Grep `CommandKeys` and confirm it holds four entries.
2. **The dispatch order in the object branch** (`.cpp:799-807`): mixed-key check FIRST, then
   `if (bHasRegionKey) … else …`. The `else` must stay the fall-through or `{}` stops reporting
   `missing_key:all_except`.
3. **The third check is gated by `SiegeAssistantIntentTakesSelection` and by nothing else** — there is no
   second list of army-wide verbs anywhere in the file. Grep for `Fallback` in
   `SiegeAssistantCommand.cpp`: it appears only inside `SiegeAssistantIntentTakesSelection`'s `default:`
   comment, never as a new switch.
4. **The stale-comment repair.** Check 2's shipped comment predicted *"a FIFTH `who` shape added later could
   make it reachable."* That shape has now arrived; I re-checked the guard against it rather than assuming, and
   recorded the result in place: the region branch touches neither `bWhoIsNone` nor `ExcludeKinds`, so the
   guard is still unreachable from JSON and stays for the wire. (`SC-§22` stale-divergence, caught in my own
   file.)
5. **Ordering inside the validator's region block:** reserved-symbol test first, then the two conflicts — so
   the same wire value earns the same code down either path (parser or wire).
6. **`region_conflict:` with an empty payload is unreachable**: `SiegeAssistantIntentToSymbol` returns empty
   only for `None`, and a parsed command cannot hold `None` (the intent symbol must resolve). This matches
   check 2's shipped behaviour exactly.

## 7. Downstream notes

- **TASK-546 (grammar)** compiles against `SiegeAssistantJsonKeys::In` — it is landed and spelled `in`, with
  **no underscore**, so no GBNF rule-name split is needed (the `at_least` / `exceptlist` trap does not apply).
- **TASK-548 (executor)** owns: the `IsPointInRegion` call, the fail-closed refusal when `ResolvePlaceRegion`
  fails, the loud empty-after-region refusal with arithmetic, `DescribeCommandForPlayer`'s `all in <place>`
  rendering, **and the 5-argument fix at `SiegeAssistantComponent.cpp:1039`.**
- **TASK-549 (tests)** — the additivity test is the one to write first; §3 lists the exact cases I walked, and
  §2 lists every reason code with its trigger. ⛔ I wrote no tests.
- ⚠️ **`USiegeAssistantSnapshot::ValidateCommandAgainstSnapshot` (`SiegeAssistantSnapshot.cpp:808`) knows
  nothing about `RegionPlace`.** A region-bearing command has empty `Kinds`, so it passes that gate
  unremarked — correct today (region filtering and its refusal are the executor's), but flagged so nobody
  mistakes the silence for a check.

## 8. M8 declaration (verbatim)

📌 **This feature adds no replicated property, no new replicated class, no new relevancy tier.**
The reason is structural: `RegionPlace` is an `FName`, so `FSiegeAssistantCommand` stays
`uint8` / `int32` / `FName` **only** and `AS-§3`'s *"M8 P2 takes this struct as-is over the wire"* property
survives the seventh field intact. That property is why this design is possible at all.
