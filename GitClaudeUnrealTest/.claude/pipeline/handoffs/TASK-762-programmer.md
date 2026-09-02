# TASK-762 — the three red mark tests repaired · the dead `circle_4` control RE-ARMED

- **Agent:** gameplay-programmer
- **Date:** 2026-09-01
- **Source:** `qa/TASK-753.md` (build-master's TASK-742 phase-1 suite record)
- **Status on delivery:** `ready-for-qa`
- **File touched:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp` — **ONE file, and only this file.**
- **⛔ NOT touched (fence, verified):** `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · every other test file · `SiegeAssistantZoneATest.cpp`.
- ⛔ No compile (TASK-742 owns the only one) · ⛔ no editor · ⛔ no MCP · ⛔ no Git. **The editor was DOWN when I started and is still DOWN — I did not boot it.**

---

## 1. I VERIFIED THE RULING BEFORE ACTING — the emitted bytes, quoted

The orchestrator ruled *the test is wrong and the emitter is right*. **I re-derived that from the source rather than accepting it,** and it holds.

`USiegeAssistantGrammar::Build` sends every generated symbol through `GbnfJsonString`
(`SiegeAssistantGrammar.cpp:38-52`) → `GbnfTerminal` (`:15-29`) — two layers, documented at `:31-37`
as deliberate. Places go in at `:543`, intents at `:512`, kinds at `:527` — **the same helper for all
three.** Running that escaping exactly as written produces, for `circle_1`, **fourteen characters**:

```
"   \   "   c   i   r   c   l   e   _   1   \   "   "        i.e.   "\"circle_1\""
```

The character **after the `1` is a BACKSLASH**, not a quote. The needle the three tests searched for
was the ten-character `"circle_1"` (C++ literal `TEXT("\"circle_1\"")`). It **cannot match**, in any
grammar, ever.

**⭐⭐ The decisive fact, confirmed independently:** the identical search fails for the shipped
intents `guard` and `ambush`, which have been translating Jonathan's sentences for months. A search
that cannot find `guard` in a working grammar is a broken search. ⇒ **the grammar is right.**

**⭐ A second corroboration the QA record did not cite, found in the same file:** line 3389 of
`SiegeAssistantSelectionTest.cpp` already asserts the **correct** form —
`Grammar.Contains(TEXT("\\\"in\\\""), ESearchCase::CaseSensitive)` — i.e. `\"in\"`. **This very file
gets the escaping right in the region tests and drifted only in the three mark tests.** That is
strong evidence the mark tests are the anomaly, not the emitter.

## 2. ⭐⭐ THE DEAD NEGATIVE CONTROL — measured, not asserted

At the old `:4381` the `TestFalse` claiming the never-drawn `circle_4` is absent **passed for the
wrong reason.** I proved it by simulation rather than by argument — the old needle against a grammar
that **does** contain `circle_4`:

```
OLD TestFalse passed with circle_4 ABSENT   ('"circle_4"' not found)   -> green
OLD TestFalse ALSO passed with circle_4 PRESENT                        -> green   <== INERT
```

⇒ the guard protecting *"a mark Jonathan never drew is unsayable by the AI"* **could not fail.** The
same was true of the twin at `:5434`. This is the finding worth more than the three red tests, and
re-arming it is the substance of this task.

## 3. THE FIX — the spelling is DERIVED FROM THE EMITTER, ⛔ never hand-typed

⛔ **I did not type `\"circle_1\"` anywhere.** A hand-typed escaped literal is a second copy of the
escaping rule, free to drift from `GbnfJsonString` — it re-creates this exact defect the next time
the escaping changes. ⛔ **I also did not mirror `GbnfJsonString` in the test file**, for the same
reason and because this file's own header (AS-§12g) forbids transcribing a builder's behaviour into
a fixture.

Instead the wrapper is **MEASURED off the shipped emitter's own output, at run time, on every run.**
New fixture block in `SiegeAssistantSelectionTestFixture` (immediately before the namespace close):

| Symbol | What it does |
|---|---|
| `IsGrammarQuotingChar` | the two-character quoting alphabet (`"` and `\`) |
| `FGrammarSpelling` | the measured `Prefix`/`Suffix` + `Of()`, `IsIn()`, `CountIn()`, `IsWrapper()` |
| `CalibrateGrammarSpelling(Grammar, KnownSymbol)` | finds a known-present symbol in the REAL grammar and walks outward over the quoting alphabet to capture the wrapper |
| `ShippedGuardSymbol()` / `ShippedAmbushSymbol()` | `SiegeAssistantIntentToSymbol(ESiegeAssistantIntent::Guard / ::Ambush)` — read off the **shipped enum**, ⛔ not typed |
| `GrammarWithExtraPlace()` | ⭐ arming control: the same board + one extra place, through the same shipped `Build` |
| `GrammarWithExtraRegion()` | ⭐ arming control for the M-6 count: the mark also declared region-bearing |

Measured result: `Prefix = "\"` · `Suffix = \""` ⇒ `Of("circle_1")` == `"\"circle_1\""`, **byte-identical
to what `GbnfJsonString("circle_1")` emits.**

**Why an INTENT is the calibration symbol.** `intent` is the one generated-vocabulary rule `Build`
emits **unconditionally** (`kind`, `where` and `zone` are each gated on the board having some), and
its alternatives come from `ESiegeAssistantIntent` by reflection. So the calibration symbol is
neither invented by the test nor able to vanish without the feature itself being gone. (Checked: no
ask code, unit kind or place on any fixture board contains the substring `guard` — `which_unit`,
`which_place`, `how_many`, `which_intent`, `unsupported`.)

## 4. THE REPAIRED ASSERTIONS

**`MarkSymbolsReachThePlacesLine`**
- `:4581` calibrate · `:4584` positive control `guard` · `:4586` positive control `ambush` · `:4588` `IsWrapper()`
- `:4591` `circle_1` present · `:4593` `circle_2` present *(were the two reds)*
- `:4600` `circle_4` absent *(was the INERT control)*
- `:4613` ⭐ **ARMING PROOF** · `:4615` `circle_5` still absent from the armed control

**`MarkIsNeverARegion`**
- `:4882-4889` calibrate + both positive controls + `IsWrapper()`
- `:4901` `circle_1` appears **exactly once** *(was the red — the old counter could only ever return 0)*
- `:4914` ⭐ **ARMING PROOF** — the counter reads **2** on a grammar where `circle_1` is also a region

**`MarkSentencesParseEndToEnd`**
- `:5618-5621` calibrate + `IsWrapper()`
- `:5624` `guard` · `:5626` `ambush` · `:5628` `circle_1` · `:5630` `circle_2` *(were the four reds)*
- `:5696` `circle_4` unsayable *(was the second INERT control)* · `:5706` ⭐ **ARMING PROOF**

## 5. ⭐⭐ PROOF THE `circle_4` CONTROL IS GENUINELY ARMED — what would now make it fail

The absence assertion and the arming proof use **the same object, the same method, the same needle**;
the only difference is the grammar. `GrammarWithExtraPlace` rebuilds the **same board** through the
**same shipped `USiegeAssistantGrammar::Build`** with `circle_4` appended to `PlaceNames`:

```
circle_4 ABSENT  (real board)      -> Spelling.IsIn(Grammar, "circle_4")      == false   TestFalse PASSES
circle_4 PRESENT (arming control)  -> Spelling.IsIn(ArmedGrammar, "circle_4") == true    TestTrue  PASSES
```

⇒ **if `circle_4` ever appeared in the real grammar, `:4600` and `:5696` would go RED.** That is the
property the old needle did not have. `:4615` additionally pins that the control adds exactly one
symbol (`circle_5` is still absent), so the arming proof cannot pass because the needle matches
everything.

For `MarkIsNeverARegion` the arming proof is **semantically exact**: the control grammar is one where
`circle_1` really has leaked into `zone`, which is precisely the M-6 violation the count exists to
catch — and the same counter reads **2** there. So `exactly once` is now a live measurement.

**Fail-safe if calibration itself ever breaks** (say `guard` is renamed): `Of()` degrades to the
**bare** symbol, which is a **broader** search — so an absence assertion becomes *more* likely to
fire, never less — and `IsWrapper()` fails loudly and names the measured prefix/suffix in its
message. `IsIn` is deliberately **not** gated on `bCalibrated`; gating it would re-create a
silent-pass. ⛔ **Fail loud in both directions, never fail silent.**

## 6. THE POSITIVE-CONTROL WIRING

Every one of the three tests now asserts, **before** it trusts any needle, that the derived spelling
finds `guard` **and** `ambush` in the grammar under test. `ambush` is the cross-check: the wrapper was
measured on `guard`, so finding `ambush` with it proves the calibration generalises rather than
self-confirms. In `MarkSentencesParseEndToEnd` these are simultaneously the test's own claim and its
control, which makes that stage self-checking. **An instrument that silently stops matching now
cannot stay silent.**

## 7. COUNTS + FENCES

- ✅ **Suite total stays `248`.** ⛔ No test added, ⛔ none removed. `IMPLEMENT_SIMPLE_AUTOMATION_TEST`
  in this file: **38 before, 38 after.** Everything I added is assertions **inside three existing
  bodies** plus fixture helpers in the fixture namespace.
- ✅ **`ZoneA.MeasuredCharCount` stays `5658`.** That assertion lives in
  `SiegeAssistantZoneATest.cpp` (`FSiegeAssistantZoneAMeasuredCharCountTest`, `:808-833`) — a file I
  am fenced out of and **did not open for writing**. Nothing I changed can move a Zone A byte: I
  touched no prompt builder and no snapshot.
- ✅ Brace/paren balance re-checked across all 5,718 lines with strings and comments stripped: **balanced**.
- ✅ No new log traffic: `circle_4` / `circle_5` are canonical lower snake_case and non-reserved, so
  `CanonicalizeSymbols` emits them without the Warning it reserves for odd characters. No new
  `AddExpectedMessagePlain` is owed.

## 8. ⚠️ WHAT THIS DOES **NOT** PROVE — say it plainly

⛔ **This does not prove the AI understands `circle_1`.** It proves the symbol **reaches the grammar
in the form the emitter actually writes**. Whether **llama.cpp parses that grammar and samples the
token** is a **RUNTIME** question that no `EditorContext` automation test in this suite can answer —
the build-master was right to say so. ⭐ **Jonathan's sitting is what tests comprehension.** Do not
let a green suite here be read as evidence the assistant can act on a drawn circle.

## 9. ⚑ FOR THE GATE

- **⚠️ Assertion-count discrepancy, raised not resolved.** The record says **6** failing assertions;
  I count **7** sites that must have been red: `:4373`, `:4375` (2) · `:4663` (1) · `:5362`, `:5364`,
  `:5366`, `:5368` (4). The two `TestFalse`s (`:4382`, `:5434`) were green-but-inert and are not in
  either count. All 7 are repaired regardless, so this changes nothing about the fix — but the next
  suite run should reconcile it rather than assume the report was exact.
- **Scope note, declared:** the remedy adds assertions to three existing test bodies (positive
  controls + arming proofs). That was required by the dispatch (*"your remedy MUST re-arm it"*) and
  it moves **no test count**. Flagging it so nobody reads the extra assertions as scope creep.
- **Judgement call:** I chose run-time calibration over a test-local mirror of `GbnfJsonString`.
  A mirror would have been simpler to read but is a second copy of a rule that already has one
  authority, and this file's header explicitly rules that shape worse than no test (AS-§12g).
