# TASK-417 — [LLM-A4] GBNF grammar builder + command schema + parser + vocabulary + automation tests

**Agent:** gameplay-programmer
**Status:** ready-for-qa (QA gate = TASK-419)
**Built against:** CONVENTIONS §1, §3, §8, §9 (pinned registry), §10, §11 — **including manager ruling 15 (multi-kind selection), which arrived mid-task.**
**No compile. No Git. No editor. No MCP. No `Content/` asset. `L_Arena` never opened.**

> **⚖️ M8 DECLARATION DUTY:** adds no replicated property, no new replicated class, no new relevancy tier.

---

## 0. PRE-FLIGHT (the mandated check)

```
$ git status --porcelain Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs
(no output — CLEAN)
```

The file was clean, so the two-word addition was safe to make. No other lane's work was merged over.

---

## 1. ⚠️ BUILT AGAINST RULING 15 — read this first

The dispatch arrived with the **singular** `FName Kind` / `int32 Count` pin. The manager corrected it mid-task to
**index-aligned parallel arrays with a hard cap of 3**, and everything below is built against the **corrected** pin.
The singular header had been written and was rewritten before any other file was authored — no residue.

Three consequences QA should verify at TASK-419:

1. **`TArray<FName> Kinds` + `TArray<int32> Counts`, parallel arrays, never an array of structs.** The payload stays
   `FName`/`int32` only, so the M8 P2 "takes it as-is over the wire" claim survives.
2. **The cap is expressed IN THE GRAMMAR, not only in the parser.** `selection` is a **bounded alternation of exactly
   1, 2 and 3 `item`s** — not a repetition rule. A 4-kind selection is unreachable by the sampler, not merely rejected
   afterwards. The test asserts the grammar contains **no `*`, no `+` and no `?` operator anywhere**, which is what
   keeps that true under future edits.
3. **`Kinds.Num() != Counts.Num()` is a hard failure, never a truncation** — `SiegeAssistantValidateSelection`, called
   as the parser's final gate.

---

## 2. Files touched

| File | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantCommand.h` | **NEW.** Pure-data header: `ESiegeAssistantIntent`, `FSiegeAssistantCommand`, `SiegeAssistantMaxSelectionKinds = 3`, `LogSiegeAssistant`, the JSON-key / symbol / ask-code / reason-code namespaces, `ParseSiegeAssistantCommand` + 6 helper free functions. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantCommand.cpp` | **NEW.** `DEFINE_LOG_CATEGORY`, the strict parser, the selection validator, the enum-reflection helpers. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.h` | **NEW.** `USiegeAssistantGrammar` + `GrammarCountMin/Max`. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp` | **NEW.** The generator. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantVocabulary.h` | **NEW.** `FSiegeAssistantSynonym`, `USiegeAssistantVocabulary`. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantVocabulary.cpp` | **NEW.** C++ defaults (13 units / 7 places / 7 intents) + `BuildSynonymTable`. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` | **NEW.** 11 automation tests. New `Tests/` directory — UBT globs it, no `Build.cs` change needed for it. |
| `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` | **EDIT (2 entries).** `+ "Json", "JsonUtilities"`. |

Nothing else was touched. **`SiegePlayerController.*`, `SummonedUnit.*`, `MinerUnit.*` were never opened** — the FOLLOW
batch owns them and is mid-gate.

**`Build.cs` note for QA:** the two modules went into **`PublicDependencyModuleNames`**, matching the file's existing
style (that module declares *every* dependency public and has an empty private list). `Private` would also have worked
today since the parse lives in a `.cpp`, but Wave 1's B2/B3 may want JSON types in a header and public costs nothing.
**Neither `HTTP` nor `Sockets` was added**, and the comment in the file says why so a future reader does not re-open it.

---

## 3. THE WORKED EXAMPLE — a realistic mid-match grammar

Inputs, exactly what `USiegeAssistantSnapshot::GetUnitKinds()` / `GetPlaceNames()` produce for a Blue player who has
footmen, archers, a sorcerer and a cleric alive:

```cpp
UnitKinds  = { "footman", "archer", "sorcerer", "cleric" }
PlaceNames = { "own_castle", "enemy_castle", "mid", "ancient_ground_near",
               "ancient_ground_far", "nearest_mine", "hero" }
```

`USiegeAssistantGrammar::Build(UnitKinds, PlaceNames)` returns exactly:

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
kind ::= "\"footman\"" | "\"archer\"" | "\"sorcerer\"" | "\"cleric\""
where ::= "\"own_castle\"" | "\"enemy_castle\"" | "\"mid\"" | "\"ancient_ground_near\"" | "\"ancient_ground_far\"" | "\"nearest_mine\"" | "\"hero\"" | "\"none\""
count ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "10" | "11" | "12" | "13" | "14" | "15" | "16" | "17" | "18" | "19" | "20" | "21" | "22" | "23" | "24" | "25" | "26" | "27" | "28" | "29" | "30" | "\"all\""
at_least ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "10" | "11" | "12" | "13" | "14" | "15" | "16" | "17" | "18" | "19" | "20" | "21" | "22" | "23" | "24" | "25" | "26" | "27" | "28" | "29" | "30"
item ::= "{\"kind\":" kind ",\"n\":" count "}"
selection ::= "[" item "]" | "[" item "," item "]" | "[" item "," item "," item "]"
who ::= selection | "\"all\"" | "\"none\""
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at_least "}"
```

Read the two generated rules and the grounding claim is visible: **`kind` has exactly four alternatives because exactly
four unit kinds are alive.** If the sorcerer dies, `"\"sorcerer\""` leaves the rule and the model *physically cannot*
name one — every token path that spells it is masked at the sampler. Grounding lives in the grammar, not the weights.

### The degenerate roster (no live commandable units)

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
where ::= "\"own_castle\"" | ... | "\"none\""
who ::= "\"all\"" | "\"none\""
when ::= "\"now\""
```

`kind`, `count`, `at_least`, `item` and `selection` are **omitted entirely** rather than emitted as a dangling
`kind ::= "\"none\""` that `item` would then reference. With nothing nameable those rules are unreachable, and a
grammar that references an undefined rule does not compile in llama.cpp. What survives is exactly what is true: you can
still charge, fall back, rally, or ask a question. **A test asserts every referenced rule is defined in all four
input shapes** (populated / no kinds / no places / neither), which is the check that would have caught the dangling-rule
version.

---

## 4. THE EXACT JSON THE SCHEMA ACCEPTS

Compact, no whitespace, fixed key order, **every key always present**.

```jsonc
// 1 kind
{"intent":"send","who":[{"kind":"footman","n":10}],"where":"ancient_ground_near","when":"now"}

// 2 kinds — THE FEATURE'S FLAGSHIP SENTENCE, "send 10 footmen with a sorcerer
// to the nearest ancient ground". This is the row the singular pin could not express.
{"intent":"send","who":[{"kind":"footman","n":10},{"kind":"sorcerer","n":1}],"where":"ancient_ground_near","when":"now"}

// 3 kinds, exactly at the cap
{"intent":"guard","who":[{"kind":"footman","n":8},{"kind":"archer","n":4},{"kind":"cleric","n":1}],"where":"mid","when":"now"}

// "all" of a kind  ->  Counts[0] == 0
{"intent":"send","who":[{"kind":"footman","n":"all"}],"where":"mid","when":"now"}

// army-wide verbs carry no selection
{"intent":"charge","who":"none","where":"none","when":"now"}
{"intent":"fallback","who":"all","where":"own_castle","when":"now"}
{"intent":"rally","who":"none","where":"none","when":"now"}

// deferred intent: "when I have 10 footmen, send them at the enemy castle"
{"intent":"send","who":[{"kind":"footman","n":10}],"where":"enemy_castle","when":{"kind":"footman","at_least":10}}

// the question branch — parses to FALSE with OutError == "ask:which_unit"
{"ask":"which_unit"}
```

**✅ CROSS-CHECK WITH TASK-416, WHICH LANDED IN PARALLEL:** `SiegeAssistantSnapshot.cpp:653` writes its Zone-A few-shot
as `{"intent":"send","who":[{"kind":"footman","n":10},{"kind":"sorcerer","n":1}],"where":"ancient_ground_near","when":"now"}`
— **byte-identical to my flagship row, `"n"` key included.** The prompt teaches exactly the schema the grammar enforces
and the parser accepts. That was independently arrived at from the same CONVENTIONS text and is the strongest evidence
the three tasks link.

---

## 5. ⚠️ THE ONE DESIGN SUBTLETY — `count` is 1–30, NOT 1–live-max

This is in a code comment on `USiegeAssistantGrammar::GrammarCountMax`, in the test that guards it, and here, because it
is exactly the thing a later reader "tightens" into a defect.

If the grammar capped counts at the live maximum, a player asking for 10 when 8 exist would get a **silently-emitted 8**
— the exact valid-shaped-wrong-command failure this whole design exists to prevent — and the clarification
("there are only 8, is that OK?") would become **undetectable**, because nothing downstream could tell a genuine request
for 8 from a clamped request for 10.

**Identity is closed-world; quantity is open. The executor detects the shortfall.**

The guarding test deliberately builds with a **one-kind roster** and still asserts the full 1..30 range, so the
regression fails loudly rather than looking like a sensible optimisation.

> **The law: grammar guarantees existence, executor guarantees legality, FSM owns the conversation.**

---

## 6. The test list — 11 tests, and what each asserts

Run with `Siegebound.Assistant.*` in the Session Frontend. All are `IMPLEMENT_SIMPLE_AUTOMATION_TEST` under
`#if WITH_DEV_AUTOMATION_TESTS`, flags `EditorContext | EngineFilter`. No world, no PIE, no model.

| # | Test | Asserts |
|---|---|---|
| 1 | `Grammar.Determinism` | Two builds from equal inputs are **byte-identical**. **Symbol casing does not change the output** (`{"Footman","ARCHER"}` ⇒ same bytes as `{"footman","archer"}`) — the guarantee behind the `.ToLower()`. Duplicates collapse. Reserved sentinels `all`/`none` are dropped from generated alternatives. |
| 2 | `Grammar.Grounding` | A live sorcerer **is** a `kind` alternative; a dead one is **absent from the whole grammar string**. Survivors remain. A one-entry place list yields `where` with exactly 2 alternatives (the place + `none`); an unlisted place is absent. |
| 3 | `Grammar.Intents` | `intent` has **exactly 7** alternatives, all seven named, and **`none` is not among them**. The reflected symbol list has 7 entries and every one **round-trips** through `SiegeAssistantIntentToSymbol`/`FromSymbol` — the ONE-source guarantee. `"none"` and `"teleport"` are refused. |
| 4 | `Grammar.CountRange` | ⚠️ Built with a **1-kind roster** and still: `count` has 30 numbers + `"all"`, **every** integer 1..30 present, **31 absent, 0 absent**. `at_least` has exactly 30 numbers and **no `"all"`**. |
| 5 | `Grammar.SelectionCap` | `selection` has exactly `SiegeAssistantMaxSelectionKinds` alternatives; alternative *N* references `item` exactly *N* times. **The grammar contains no `*`, `+` or `?`** — the cap is a bounded alternation, not a repetition. `who` = selection \| all \| none. |
| 6 | `Grammar.DegenerateInputs` | Empty roster ⇒ `kind`/`item`/`selection`/`count`/`at_least` **omitted**, core rules kept, `who` down to 2 alternatives, `when` down to 1. Empty places ⇒ `where` = just `none`. Both empty ⇒ still well-formed and still deterministic. **A mini GBNF validator asserts every referenced rule is defined and `root` exists, in all four shapes.** |
| 7 | `Command.RoundTrip` | 1-kind, **2-kind flagship**, 3-kind at cap, `"all"`⇒0, both whole-army selectors, `where:"none"`⇒`NAME_None`, the deferred trigger, and the question branch (returns **false** with `OutError == "ask:which_unit"`). |
| 8 | `Command.Rejection` | **37 malformed inputs**, each asserted to be refused **with the right reason code AND to leave the command default-constructed.** Includes the 4-kind selection (`who_arity`), the repeated kind (`duplicate_kind`), `send`+`who:"none"` (`who_required`), `at_least:"all"`, counts 0/31/−4/2.5/`"5"`, a case-wrong key, an array root, and a question with an extra key. |
| 9 | `Command.NeverPartiallyFills` | A **pre-poisoned** out-param holding a previous valid order is wiped by a **late** failure (intent/who/where all parse, `when` fails). The success path also clears stale arrays and triggers. |
| 10 | `Command.SelectionInvariants` | `SiegeAssistantValidateSelection` rejects a **length mismatch** (payload `selection_mismatch:2/1`), an over-cap selection, a repeated kind and a `NAME_None` kind; accepts empty and full-cap-distinct. Plus the 7-verb selection-bearing / army-wide split. |
| 11 | `Vocabulary.SynonymTable` | `BuildSynonymTable` is **byte-stable across calls** and **invariant to row and alias ORDER** (asserted by reversing every array on a fresh instance). **The Sorcerer/Wizard collision:** neither claims `mage`/`caster`/`spellcaster`/`magic user`, and neither aliases the other's name. `defend` is claimed by neither `guard` nor `fallback`. Every intent row names a real intent. The notes block ships. |

---

## 7. Decisions I made that QA should scrutinise

These are the places where the spec left room and I chose. Each is also commented at the code site.

1. **`root ::= command | question`, and the `question` branch is REAL.** The dispatch listed it but did not define it.
   Without it, a model handed an untranslatable utterance is **forced by the grammar to invent a command**, because
   declining is not expressible. Constrained decoding makes output well-formed, never correct, so "I cannot turn this
   into an order" must stay reachable. The five `ask` codes are symbols, not prose (§3).
   ⚠️ **`FSiegeAssistantCommand` has no field for an ask code and the struct is pinned**, so a question surfaces through
   the existing `OutError` channel as **`ask:<code>`** with the parser returning **false** and the command reset. The FSM
   routes `ask:*` to Clarify and everything else to Failed. **If the manager would rather the struct carried it, that is
   a §9 registry change and a ruling — flagged, not taken.**
2. **The parser performs exactly ONE cross-field check, and only because that pair is representationally ambiguous.**
   `who:"none"` and `who:"all"` both land on an empty `Kinds`/`Counts`, so accepting `who:"none"` with a
   selection-bearing verb would silently turn *"send nobody"* into *"send everybody"*. Every other cross-field question
   — is the place resolvable, are there enough units, is this legal now, does this player have authority — is left to
   the **executor**, deliberately.
3. **Duplicate kinds are rejected (`duplicate_kind`).** Not in the spec. `[{"kind":"footman","n":5},{"kind":"footman","n":3}]`
   would leave the executor holding two quantities for one kind with no rule for which wins — a silent discard, i.e.
   the same failure class the ruling-15 truncation ban targets.
4. **`at_least` is a separate numeric rule with no `"all"`.** *"Wait until I have all footmen"* is a condition that can
   never become true. `count ::= "1" | … | "30" | "\"all\""` is still emitted **literally and greppable** (the §1 QA
   criterion reads as a shape), and both rules are generated from the same two constants.
5. **Symbols are lower-cased before emission.** Not cosmetic: `FName` reports the case it was **first constructed with
   anywhere in the process**, so identical game state could otherwise produce two different grammars depending on load
   order. Test 1 asserts this. Round-trip is unaffected — `FName` comparison is case-insensitive.
6. **Caller order is preserved; nothing is sorted.** `FName::operator<` orders by *comparison index*, which depends on
   registration order and is **not stable across processes** — sorting by it would silently break determinism.
   `BuildSynonymTable` does sort, but **by string**, for the same reason.
7. **The emitted grammar carries no comments and no `ws` rule.** GBNF comment support is a property of whichever
   llama.cpp build TASK-409 vendors, and this string has to parse there; the annotated copy is §3 above instead. No
   whitespace rule because optional whitespace spends tokens from a 96-token budget and adds sampling branches.
8. **`GITCLAUDEUNREALTEST_API` is on the three classes** even though §9 omits it. The registry clearly abbreviates (it
   omits `GENERATED_BODY()` and every `UPROPERTY` too), and **every other header in this module carries the macro**.
9. **`SiegeAssistantValidateSelection` is a public free function, not a private parser detail.** Nothing out of the
   parser can violate the invariant — it fills both arrays in one walk — but **in M8 P2 this struct arrives over the
   wire from a peer that never passed through anyone's grammar.** This is the receive-side validator, written now so P2
   has one, and it is what makes "a mismatch is a failure, never a truncation" an **executable check** rather than a
   comment. Test 10 exercises it directly.
10. **Six additive free functions** (`SiegeAssistantReasonCode`, `…IntentToSymbol`, `…IntentFromSymbol`,
    `…IntentSymbols`, `…AskCodes`, `…IntentTakesSelection`) beyond the §9 list. All additive, no pinned signature
    changed. They exist so the seven intent strings and the executor's verb split have **one** home rather than being
    re-derived in the grammar, the parser and the FSM.

---

## 8. Notes for the downstream tasks

- **TASK-421 (`DA_AssistantVocabulary`):** the C++ defaults ship 13 unit / 7 place / 7 intent rows and the asset
  overrides them wholesale. **The notes block is authored in C++ on purpose** and always ships — it encodes
  disambiguation LAW (Sorcerer≠Wizard, guard≠fallback) and must survive an artist re-authoring the asset.
- **⚠️ DECLARED REACTIVE COVERAGE (CONVENTIONS §11):** my `PlaceSynonyms` defaults were **adjusted after reading
  TASK-416's shipped `PlaceVocabulary` table** — I added `nearest_mine` and `hero` and dropped a placeholder note about
  per-mine symbols, because 416 pinned **one** resolved mine symbol rather than `mine_1..6`. That is
  *pinned-place-list → vocabulary coverage*, i.e. the legitimate direction; **no corpus was consulted and none exists
  yet.** Recording it because §11 requires reactive coverage to be visible.
- **Wave 1 B2 (the FSM/executor):** call `SiegeAssistantValidateSelection` on any command you did not parse yourself.
  Route `SiegeAssistantReasonCode(Error) == "ask"` to Clarify, not Failed. And **pass the LIVE roster to
  `Build`** — widening `UnitKinds` toward the whole deck would trade the grounding guarantee away; the
  not-yet-spawned case is the deferred-intent path's job, not the grammar's.
- **TASK-410/413 (the spike):** the grammar string is the thing to feed `llama_sampler_init_grammar` with root `"root"`.
  If that build's GBNF parser rejects anything here it will do so loudly at grammar-compile time, not silently.

---

## 9. Known gaps — stated, not hidden

- **Not compiled.** This is a file-lane task by spec; the integration compile is TASK-420. The likeliest compile risks
  are (a) the exact `EAutomationTestFlags` spelling in UE 5.8 and (b) `UEnum::GetNameStringByIndex` returning a
  qualified name — **(b) is already defended** (`ShortEnumEntryName` strips any `Enum::` prefix, so both behaviours work).
- **The GBNF is not machine-validated against a real llama.cpp parser** — TASK-409 has not vendored one yet. The
  in-test validator checks structure (every referenced rule defined, no dangling rules, balanced terminals), which is
  the part that can be checked without the library.
- **No unit test feeds a symbol containing a quote or backslash.** The escaping is two-layer and correct by
  construction, but it is untested because no canonical symbol can contain those characters; `CanonicalizeSymbols`
  logs a `Warning` if one ever does.

---

## 10. ADDENDUM — one real compile error, found and fixed PRE-GATE

TASK-401 (the FOLLOW lane's compile gate) ran while these files were mid-write. `Source/GitClaudeUnrealTest/` is **one
UBT module**, so an in-flight `.cpp` of mine surfaced in a gate belonging to a different batch. The orchestrator has
recorded the sequencing as its own error and written the law into CONVENTIONS (**file-disjointness is not
build-disjointness**). **The diagnostic itself was genuine and mine, and it is fixed.** Recorded here so TASK-419 sees
it was caught before the gate rather than wondering why the code moved.

```
SiegeAssistantCommand.cpp(58) : error C4172 — returning address of local variable or temporary  (FindFieldExact)
```

**It was a real dangling pointer, not a pedantic warning.** The helper was:

```cpp
const TSharedPtr<FJsonValue>* FindFieldExact(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
{
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object->Values)   // <-- converts
        if (Field.Key.Equals(Key, ESearchCase::CaseSensitive))
            return &Field.Value;                                                 // <-- into a temporary
```

`FJsonObject::Values` is a `TMap`, and **its iterator does not yield exactly `TPair<FString, TSharedPtr<FJsonValue>>&`**.
Naming that type as the loop variable therefore does not bind — it **converts**, materialising a temporary pair each
iteration. `&Field.Value` was the address of a member of that temporary, which dies at the end of the iteration. It
read correctly and would usually *behave* correctly, because the freed bytes normally still hold the right value —
which is exactly why it would have survived QA and then misbehaved only under optimisation.

**Fixed structurally, so the bug class cannot come back:**

| Before | After |
|---|---|
| `FindFieldExact` returned `const TSharedPtr<FJsonValue>*` into the map | **Deleted.** Split into `HasFieldExact(...) -> bool` (presence) and `FindFieldValue(...) -> TSharedPtr<FJsonValue>` **by value** |
| Callers compared the returned pointer against `nullptr` | Callers use the bool, or check `.IsValid()` on an owning handle |
| Map loops named the pair type explicitly | Map loops use **`auto&`**, so no conversion — and therefore no temporary — can occur whatever element type the container iterates as |

A `TSharedPtr` returned **by value is a shared OWNER**, so the returned value's lifetime no longer depends on the map
at all. Nothing in these files returns a pointer or reference into a container any more.

**Two things came out of the re-check, both worth having:**

1. **`ValidateExactKeySet` had the same explicit-pair loop binding.** It never returned an address, so it was not a
   bug — but it was silently **copying an FString and churning a `TSharedPtr` refcount for every key of every object
   parsed**. Now `auto&`, so it genuinely binds.
2. **Presence and validity are now properly distinct.** `HasFieldExact` answers "is the key there"; `FindFieldValue`
   answers "give me the value". A key explicitly present with a JSON `null` yields a *valid* `FJsonValueNull` and so is
   reported as `bad_type`, not as a missing key — which is the correct reading and was ambiguous in the old
   pointer-based version.

**Sweep results for the two related patterns the coordinator flagged:**

| Check | Result |
|---|---|
| Any remaining `FindFieldExact` | **None** — fully removed |
| Any function in these 7 files returning a pointer or reference | **None** — every one returns by value or uses out-params |
| Any `return &` / `return *` | **None** |
| Any explicit `TPair` loop binding | **None** — the only remaining `TPair` is inside the explanatory comment |
| ⚠️ Any `TSoftObjectPtr` / `TSoftClassPtr` / `LoadSynchronous` / `FSoftObjectPath` (the TASK-416 `C2228`/`C2737` complete-type class, QA criterion 10) | **None — these files contain no soft pointers at all**, so that trap cannot apply here. My include list was re-verified against every type whose members I actually touch: `Dom/JsonObject.h`, `Dom/JsonValue.h`, `Serialization/JsonReader.h`, `Serialization/JsonSerializer.h`, `UObject/Class.h` (the `UEnum` calls), `UObject/ReflectedTypeAccessors.h` (`StaticEnum<>`), and `SiegeAssistantGrammar.h` (the `GrammarCountMin/Max` statics). |

**One unrelated tidy-up in the same sweep:** `SiegeAssistantVocabulary.cpp` carried `#include "SiegeAssistantCommand.h"`
that it never used — the only match in the file was the word `ESiegeAssistantIntent` inside a comment. Removed. Zero
risk: nothing in that translation unit referenced it, and every other file that needs `SiegeAssistantCommand.h`
includes it directly (including TASK-416's snapshot).

**Still not compiled** — TASK-420 owns the compile, and compiles are serialized now.

### 10b. FOLLOW-ON — the four errors the C4172 fix exposed underneath

The next gate came back with four diagnostics, all in `SiegeAssistantCommand.cpp`, all mine:

```
(72,18)  error C2039 : 'Equals': is not a member of 'UE::TSharedString<TCHAR>'
(96,18)  error C2039 : 'Equals': is not a member of 'UE::TSharedString<TCHAR>'
(116,19) error C2039 : 'Equals': is not a member of 'UE::TSharedString<TCHAR>'
(125,16) error C2664 : cannot convert 'FJsonObjectSharedStringStorage::FStringType' to 'const FString&'
```

**These confirm §10's diagnosis rather than contradicting it, and they are the same root cause wearing a different
hat.** The compiler has now named the key type: **a JSON key is not an `FString`.** In UE 5.8,
`FJsonObject::Values` is a `TMap<FJsonObject::FStringType, TSharedPtr<FJsonValue>>` where `FStringType` is
**`UE::FSharedString`** — keys are interned in a shared string set so many objects parsed from one document share one
copy of each key.

That is *exactly* why `TPair<FString, TSharedPtr<FJsonValue>>&` **converted rather than bound**: it was silently
building an `FString` from the shared key on every iteration, which is what materialised the temporary pair whose
address escaped. Removing the conversion fixed the dangling pointer **and** stopped papering over the real key type at
each use site — so the four sites that had been quietly relying on the implicit conversion became visible at once.
A latent dangling pointer was traded for four loud compile errors, which is the right direction.

**Fixed at the type level, in one place, per the standing instruction** — not by sprinkling conversions at call sites:

```cpp
// THE ONLY TWO PLACES IN THIS FILE THAT TOUCH A JSON KEY'S CONCRETE TYPE.
FStringView JsonKeyView  (const FJsonObject::FStringType& Key) { return FStringView(*Key, Key.Len()); }
FString     JsonKeyString(const FJsonObject::FStringType& Key) { return FString(*Key); }
```

All seven key uses now route through these two. `FStringView::Equals(const TCHAR*, ESearchCase::Type)` gives the
case-sensitive comparison the strictness rule needs (`StringView.h:389`), and the `FString` form is used only on the
`unknown_key` error path.

**⚠️ NEITHER HELPER NAMES THE CONCRETE TYPE, AND THAT IS THE POINT.** Both go through `FJsonObject::FStringType`, and
both use only `operator*` and `Len()` — **the two operations `UE::FSharedString` and `FString` both provide.** The
engine still chooses between the two storage classes with `UE_JSONOBJECT_LEGACY_STRING_KEYS` (`JsonObject.h:240` keeps
an FString-keyed storage alive), so **this file now compiles under either setting** and will survive Epic flipping that
switch. `#include "Containers/StringView.h"` added for the complete-type law.

**Sweep — every JSON-key use in all seven files, not just the four reported:**

| Check | Result |
|---|---|
| Files that include any Json header | **`SiegeAssistantCommand.cpp` only** — `FJsonObject` is confined to one translation unit |
| Every `.Key` / `->Values` use anywhere (7 sites) | **All route through `JsonKeyView` / `JsonKeyString`.** No `.Equals` is called on a key type any more |
| Other `Json` matches in the remaining six files | All innocuous — a local `FString JsonEscaped`, the `const FString& Json` parameter name, `Case.Json` (a `const TCHAR*` test field). **None touch `FJsonObject`** |
| Any remaining pointer/reference return, `return &`, or explicit `TPair` binding | **None** (re-confirmed after this edit) |

**Nothing here needed a ruling** — it was a straight type error with a correct type-level answer. No behaviour changed:
key matching is still case-sensitive, the reason codes and payloads are identical, and no test needed touching.

**Chain summary for TASK-419, so this reads as one story:** C4172 dangling pointer → deleted the pointer-returning
helper and switched the loops to `auto&` → that removed an implicit `UE::FSharedString`→`FString` conversion → the four
sites relying on that conversion surfaced → routed all key handling through two type-level helpers. **One root cause
(the key is not an `FString`), three visible stages.** Still not compiled; TASK-420 owns that.

---

## 11. QA LOOP 1 — `qa/TASK-419.md` FAIL closed (BLOCKER-1 + WARN-1 + WARN-5)

All three fixed. **The corpus, Zone A's size, and `count`'s range were not touched.**

### BLOCKER-1 — `militia_mob` was unreachable. Fixed to `militiamob`.

I re-derived it from the three artifacts before changing anything, and QA is right:

- `Docs/Data/cards.csv` row 8 is **`MilitiaMob`**.
- `SiegeAssistantSnapshot.cpp:99-106` — `CanonicalKind` is `FName(*CardID.ToString().ToLower())`: **a pure, total
  derivation with no table and no exceptions.** It can only ever produce **`militiamob`**.
- That same FName is simultaneously what feeds the grammar's `kind` alternatives **and** what Zone C prints.

So Zone A was teaching the model a spelling the sampler **physically forbids**, while the same prompt printed a
different spelling for the same unit a few lines later. **Took QA's option 1** (`militia_mob` → `militiamob`). I did
**not** take option 2 — putting a CardID→symbol exception into `CanonicalKind` would trade a total derivation for a
special case to satisfy a cosmetic preference, and it needs a manager ruling I was right not to assume.

**On the process point, which is the part worth remembering:** TASK-426 had flagged this spelling as *undecided*, and
I resolved it silently — and resolved it against the only artifact that actually produces the symbol. The lesson is
not "check spellings", it is **when a sibling task flags something open, that is a ruling request, not an invitation
to pick.**

**The root cause is an asymmetry, and it is now written into the file so it cannot recur.** Unit symbols are CardIDs
lower-cased with **no separator inserted**; place symbols are hand-authored in the snapshot's `PlaceVocabulary` and
**do** use underscores. Underscores are correct three lines below and wrong here, which is exactly why `militia_mob`
looked reasonable on the page. **`MilitiaMob` is the only multi-word `CardType == Unit` row today**, so a future
`ShieldMaiden` walks straight back into it.

**Verified after the fix — from the artifacts, not from this note:**

| Check | Result |
|---|---|
| `militia_mob` anywhere in `Source/` or `Docs/Data/` | **Only in explanatory comments.** No live symbol |
| All 13 unit canonicals diffed against `cards.csv` | **Every one derivable by `CardID.ToLower()`.** The single set-difference is `miner`, whose row is `CardType == Economy` rather than `Unit` — CardID `Miner` still yields `miner`, so it is correct, not a second instance |
| Underscore in any unit canonical | **None** |

### WARN-1 — `"fire mage"` dropped from `wizard`

Removed. `mage` / `caster` / `spellcaster` now appear in **no alias anywhere** — only in the `[notes]` line that routes
them to `which_unit`. QA's reasoning is the part that matters and is now recorded at the site: **this table is prompt
text read by an LLM, not an exact-match lookup** — nothing in the lane resolves an alias in code — so `fire mage`
pulled toward Wizard just as a bare `mage` would, and Zone A contained two lines pulling opposite ways. It threatened
`DEV-06` and the **holdout** row `HOLD-09`, both `Clarify` rows, where an alias converts an honest clarification into a
confident wrong command.

### WARN-5 — the three stale `~350 tok` comments

Corrected at all three sites, **with the reason attached** rather than just the number, because the bare number is the
bait:

- `SiegeAssistantSnapshot.h:93` (the zone table) · `SiegeAssistantSnapshot.h:146` (`MaxSnapshotChars`) ·
  `SiegeAssistantVocabulary.h:92` (`BuildSynonymTable`).
- Each now records that Zone A is **~600 tok as built** (2158 chars; ~1180 with the vocabulary asset) and **must not be
  trimmed toward 350**, with the arithmetic that makes it stick: bar #3's ratio is `(B+C)/(A+B+C)`, so the shipped sizes
  give `165/765` = **78%, passing**, while cutting Zone A to 350 gives `165/515` = **68%, marginally failing.** Zone A is
  prefilled once and cached, so its size costs turn-1 TTFT only and barely touches bar #2.

### ⚠️ DECLARED CROSS-OWNER EDIT

Two of the three WARN-5 sites are in **`SiegeAssistantSnapshot.h`, which CONVENTIONS ruling 6 pins as TASK-416's
single-owner file.** I edited it because QA named the lines and the orchestrator routed the finding to me, TASK-416 is
`qa-passed` and not running, and the module was quiet. **Both edits are comment-only — zero behaviour change, zero
compile risk.** Declaring it rather than letting it show up as an unexplained diff in someone else's file.

### New mechanical guards, so neither defect can return silently

Test 11 (`Vocabulary.SynonymTable`) gained two assertions:

1. **No unit canonical may contain `_`.** This is the one that would have caught BLOCKER-1 mechanically. It is
   deliberately **not** applied to `PlaceSynonyms`, where underscores are correct — the comment says so, because a
   later reader "generalising" it to places would break the place vocabulary.
2. **No unit alias may contain `mage` / `caster` / `spellcaster` as a SUBSTRING.** Deliberately stricter than §9b's
   token-level prohibition: `"fire mage"` passed the old whole-string check, which is precisely how it shipped.

Both are pure, need no world, and cost nothing. **Test count is still 11** — no test was removed or weakened.

**Not compiled** — TASK-420 owns that, and the module is being kept quiet for it.

---

## 12. LINK FIX — `SlateCore` added to `Build.cs` (one line)

The game module **linked for the first time** (an earlier compile abort had been masking it) and failed with **16
unresolved externals**, all SlateCore-owned — 15 from `SiegeAssistantInputProbe.cpp.obj` and 1 from the UHT glue in
`Module.GitClaudeUnrealTest.1.cpp.obj`, led by **`Z_Construct_UEnum_SlateCore_ETextCommit`**.

`Build.cs` is TASK-417's single-owner file (ruling 6), so the fix is mine even though it is one line:
**`"SlateCore"` added to `PublicDependencyModuleNames`.** No logic change.

**Why it hid for so long, which is the part worth keeping:** `Slate` publicly depends on `SlateCore`, so it propagates
SlateCore's **include paths** — every file compiled clean — but that does **not** hand the module SlateCore's **import
library**, so nothing could ever *link* a SlateCore symbol. The gap is invisible until some file references an actual
**symbol** rather than merely a type. `SiegeAssistantInputProbe` was the first: `ETextCommit::Type` in a `UFUNCTION`
signature (UHT then emits a cross-module reflection reference) plus `Widgets/SWidget.h`. Epic's own commented-out
boilerplate at the bottom of the file pairs `Slate` and `SlateCore` for exactly this reason. **This is a different
defect class from anything a shadow/API sweep can see, which is why TASK-411's review was thorough and still missed
it — and it is not a regression; it was always latent.**

### The sanity-check over the rest of the dependency list

Every engine include in **every new file this batch added** was mapped to its owning engine module by locating the
header under `Engine/Source/Runtime/*/Public/`:

| Header (new files only) | Owning module | Listed? |
|---|---|---|
| `Types/SlateEnums.h` (`ETextCommit`), `Widgets/SWidget.h` | **SlateCore** | **← the gap, now ADDED** |
| `Framework/Application/SlateApplication.h` | Slate | ✓ |
| `Blueprint/UserWidget.h`, `Blueprint/WidgetTree.h`, `Components/EditableTextBox.h` / `TextBlock.h` / `VerticalBox.h` / `VerticalBoxSlot.h` | UMG | ✓ |
| `Misc/AutomationTest.h`, `Algo/Reverse.h`, `Containers/StringView.h`, `HAL/IConsoleManager.h` | Core | ✓ |
| `UObject/Object.h`, `UObject/Class.h`, `UObject/ReflectedTypeAccessors.h` | CoreUObject | ✓ |
| `Engine/World.h`, `Engine/DataAsset.h`, `Engine/DataTable.h`, `GameFramework/*` | Engine | ✓ |
| `Dom/JsonObject.h`, `Dom/JsonValue.h`, `Serialization/Json*.h` | **Json** | ✓ (added at TASK-417) |

**The automation tests confirmed to need NO new module** — `Misc/AutomationTest.h` is in `Core`, exactly as the board
stated.

**One include deliberately NOT acted on, with the reasoning, so nobody adds a module we do not need:**
`SiegeAssistantInputProbe.cpp:17` includes `GenericPlatform/GenericWindow.h`, which **ApplicationCore** owns and which
is **not** in the dependency list. It is correct as-is. Its only use is line 171,
`SlateApp.ProcessMouseButtonDownEvent(TSharedPtr<FGenericWindow>(), PointerEvent)` — a **default-constructed empty**
`TSharedPtr`, which never emits a constructor, destructor, deleter or vtable reference, so it needs the *complete type*
(the include is right, per the complete-type law) but produces **no link symbol**. The linker is the proof: it reports
*all* unresolved externals in one pass, and **not one of the 16 was ApplicationCore-owned.**

**Verdict: `SlateCore` was the only gap.** Not compiled — build-master owns the rebuild.
